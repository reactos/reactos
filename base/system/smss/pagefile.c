/*
 * PROJECT:         ReactOS Windows-Compatible Session Manager
 * LICENSE:         BSD 2-Clause License
 * FILE:            base/system/smss/pagefile.c
 * PURPOSE:         Main SMSS Code
 * PROGRAMMERS:     Alex Ionescu
 */

/* INCLUDES *******************************************************************/

#include "smss.h"

#define NDEBUG
#include <debug.h>

/* GLOBALS ********************************************************************/

//
// Constants
//
#define STANDARD_PAGING_FILE_NAME       L"\\??\\?:\\pagefile.sys"
#define STANDARD_DRIVE_LETTER_OFFSET    4
#define MEGABYTE                        0x100000UL
#define MAXIMUM_PAGEFILE_SIZE           (4095 * MEGABYTE)
/* This should be 32 MB, but we need more than that for 2nd stage setup */
#define MINIMUM_TO_KEEP_FREE            (64 * MEGABYTE)
#define FUZZ_FACTOR                     (16 * MEGABYTE)

//
// Structure and flags describing each pagefile
//
#define SMP_PAGEFILE_CREATED            0x01
#define SMP_PAGEFILE_DEFAULT            0x02
#define SMP_PAGEFILE_SYSTEM_MANAGED     0x04
#define SMP_PAGEFILE_WAS_TOO_BIG        0x08
#define SMP_PAGEFILE_ON_ANY_DRIVE       0x10
#define SMP_PAGEFILE_EMERGENCY          0x20
#define SMP_PAGEFILE_DUMP_PROCESSED     0x40
typedef struct _SMP_PAGEFILE_DESCRIPTOR
{
    LIST_ENTRY Entry;
    UNICODE_STRING Name;
    UNICODE_STRING Token;
    LARGE_INTEGER MinSize;
    LARGE_INTEGER MaxSize;
    LARGE_INTEGER ActualMinSize;
    LARGE_INTEGER ActualMaxSize;
    ULONG Flags;
} SMP_PAGEFILE_DESCRIPTOR, *PSMP_PAGEFILE_DESCRIPTOR;

//
// Structure and flags describing each volume
//
#define SMP_VOLUME_INSERTED             0x01
#define SMP_VOLUME_PAGEFILE_CREATED     0x04
#define SMP_VOLUME_IS_BOOT              0x08
typedef struct _SMP_VOLUME_DESCRIPTOR
{
    LIST_ENTRY Entry;
    USHORT Flags;
    USHORT PageFileCount;
    WCHAR DriveLetter;
    LARGE_INTEGER FreeSpace;
    FILE_FS_DEVICE_INFORMATION DeviceInfo;
} SMP_VOLUME_DESCRIPTOR, *PSMP_VOLUME_DESCRIPTOR;

LIST_ENTRY SmpPagingFileDescriptorList, SmpVolumeDescriptorList;
BOOLEAN SmpRegistrySpecifierPresent;
ULONG SmpNumberOfPagingFiles;

/* FUNCTIONS ******************************************************************/

VOID
NTAPI
SmpPagingFileInitialize(VOID)
{
    /* Initialize the two lists */
    InitializeListHead(&SmpPagingFileDescriptorList);
    InitializeListHead(&SmpVolumeDescriptorList);
}

NTSTATUS
NTAPI
SmpCreatePagingFileDescriptor(IN PUNICODE_STRING PageFileToken)
{
    NTSTATUS Status;
    ULONG MinSize = 0, MaxSize = 0;
    BOOLEAN SystemManaged = FALSE, ZeroSize = TRUE;
    PSMP_PAGEFILE_DESCRIPTOR Descriptor, ListDescriptor;
    ULONG i;
    WCHAR c;
    PLIST_ENTRY NextEntry;
    UNICODE_STRING PageFileName, Arguments, SecondArgument;

    /* Make sure we don't have too many */
    if (SmpNumberOfPagingFiles >= 16)
    {
        DPRINT1("SMSS:PFILE: Too many paging files specified - %lu\n",
                SmpNumberOfPagingFiles);
        return STATUS_TOO_MANY_PAGING_FILES;
    }

    /* Parse the specified and get the name and arguments out of it */
    DPRINT("SMSS:PFILE: Paging file specifier `%wZ'\n", PageFileToken);
    Status = SmpParseCommandLine(PageFileToken,
                                 NULL,
                                 &PageFileName,
                                 NULL,
                                 &Arguments);
    if (!NT_SUCCESS(Status))
    {
        /* Fail */
        DPRINT1("SMSS:PFILE: SmpParseCommandLine( %wZ ) failed - Status == %lx\n",
                PageFileToken, Status);
        return Status;
    }

    /* Set the variable to let everyone know we have a pagefile token */
    SmpRegistrySpecifierPresent = TRUE;

    /* Parse the arguments, if any */
    if (Arguments.Buffer)
    {
        /* Parse the pagefile size */
        for (i = 0; i < Arguments.Length / sizeof(WCHAR); i++)
        {
            /* Check if it's zero */
            c = Arguments.Buffer[i];
            if ((c != L' ') && (c != L'\t') && (c != L'0'))
            {
                /* It isn't, break out */
                ZeroSize = FALSE;
                break;
            }
        }
    }

    /* Was a pagefile not specified, or was it specified with no size? */
    if (!(Arguments.Buffer) || (ZeroSize))
    {
        /* In this case, the system will manage its size */
        SystemManaged = TRUE;
    }
    else
    {
        /* We do have a size, so convert the arguments into a number */
        Status = RtlUnicodeStringToInteger(&Arguments, 0, &MinSize);
        if (!NT_SUCCESS(Status))
        {
            /* Fail */
            RtlFreeUnicodeString(&PageFileName);
            RtlFreeUnicodeString(&Arguments);
            return Status;
        }

        /* Now advance to the next argument */
        for (i = 0; i < Arguments.Length / sizeof(WCHAR); i++)
        {
            /* Found a space -- second argument must start here */
            if (Arguments.Buffer[i] == L' ')
            {
                /* Use the rest of the arguments as a maximum size */
                SecondArgument.Buffer = &Arguments.Buffer[i];
                SecondArgument.Length = (USHORT)(Arguments.Length -
                                        i * sizeof(WCHAR));
                SecondArgument.MaximumLength = (USHORT)(Arguments.MaximumLength -
                                               i * sizeof(WCHAR));
                Status = RtlUnicodeStringToInteger(&SecondArgument, 0, &MaxSize);
                if (!NT_SUCCESS(Status))
                {
                    /* Fail */
                    RtlFreeUnicodeString(&PageFileName);
                    RtlFreeUnicodeString(&Arguments);
                    return Status;
                }

                break;
            }
        }
    }

    /* We are done parsing arguments */
    RtlFreeUnicodeString(&Arguments);

    /* Now we can allocate our descriptor */
    Descriptor = RtlAllocateHeap(RtlGetProcessHeap(),
                                 HEAP_ZERO_MEMORY,
                                 sizeof(SMP_PAGEFILE_DESCRIPTOR));
    if (!Descriptor)
    {
        /* Fail if we couldn't */
        RtlFreeUnicodeString(&PageFileName);
        return STATUS_NO_MEMORY;
    }

    /* Capture all our data into the descriptor */
    Descriptor->Token = *PageFileToken;
    Descriptor->Name = PageFileName;
    Descriptor->MinSize.QuadPart = MinSize * MEGABYTE;
    Descriptor->MaxSize.QuadPart = MaxSize * MEGABYTE;
    if (SystemManaged) Descriptor->Flags |= SMP_PAGEFILE_SYSTEM_MANAGED;
    Descriptor->Name.Buffer[STANDARD_DRIVE_LETTER_OFFSET] =
    RtlUpcaseUnicodeChar(Descriptor->Name.Buffer[STANDARD_DRIVE_LETTER_OFFSET]);
    if (Descriptor->Name.Buffer[STANDARD_DRIVE_LETTER_OFFSET] == '?')
    {
        Descriptor->Flags |= SMP_PAGEFILE_ON_ANY_DRIVE;
    }

    /* Now loop the existing descriptors */
    NextEntry = SmpPagingFileDescriptorList.Flink;
    do
    {
        /* Are there none, or have we looped back to the beginning? */
        if (NextEntry == &SmpPagingFileDescriptorList)
        {
            /* This means no duplicates exist, so insert our descriptor! */
            InsertTailList(&SmpPagingFileDescriptorList, &Descriptor->Entry);
            SmpNumberOfPagingFiles++;
            DPRINT("SMSS:PFILE: Created descriptor for `%wZ' (`%wZ')\n",
                    PageFileToken, &Descriptor->Name);
            return STATUS_SUCCESS;
        }

        /* Keep going until we find a duplicate, unless we are in "any" mode */
        ListDescriptor = CONTAINING_RECORD(NextEntry, SMP_PAGEFILE_DESCRIPTOR, Entry);
        NextEntry = NextEntry->Flink;
    } while (!(ListDescriptor->Flags & SMP_PAGEFILE_ON_ANY_DRIVE) ||
             !(Descriptor->Flags & SMP_PAGEFILE_ON_ANY_DRIVE));

    /* We found a duplicate, so skip this descriptor/pagefile and fail */
    DPRINT1("SMSS:PFILE: Skipping duplicate specifier `%wZ'\n", PageFileToken);
    RtlFreeUnicodeString(&PageFileName);
    RtlFreeHeap(RtlGetProcessHeap(), 0, Descriptor);
    return STATUS_INVALID_PARAMETER;
}

NTSTATUS
NTAPI
SmpGetPagingFileSize(IN PUNICODE_STRING FileName,
                     OUT PLARGE_INTEGER Size)
{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatusBlock;
    HANDLE FileHandle;
    FILE_STANDARD_INFORMATION StandardInfo;

    DPRINT("SMSS:PFILE: Trying to get size for `%wZ'\n", FileName);
    Size->QuadPart = 0;

    InitializeObjectAttributes(&ObjectAttributes,
                               FileName,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);
    Status = NtOpenFile(&FileHandle,
                        FILE_READ_ATTRIBUTES | SYNCHRONIZE,
                        &ObjectAttributes,
                        &IoStatusBlock,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        FILE_SYNCHRONOUS_IO_NONALERT);
    if (!NT_SUCCESS(Status)) return Status;

    Status = NtQueryInformationFile(FileHandle,
                                    &IoStatusBlock,
                                    &StandardInfo,
                                    sizeof(StandardInfo),
                                    FileStandardInformation);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("SMSS:PFILE: Failed query for size potential pagefile `%wZ' with status %X\n",
                FileName, Status);
        NtClose(FileHandle);
        return Status;
    }

    NtClose(FileHandle);
    Size->QuadPart = StandardInfo.AllocationSize.QuadPart;
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
SmpDeletePagingFile(IN PUNICODE_STRING FileName)
{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatusBlock;
    HANDLE FileHandle;
    FILE_DISPOSITION_INFORMATION Disposition;

    /* Open the page file */
    InitializeObjectAttributes(&ObjectAttributes,
                               FileName,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);
    Status = NtOpenFile(&FileHandle,
                        DELETE,
                        &ObjectAttributes,
                        &IoStatusBlock,
                        FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
                        FILE_NON_DIRECTORY_FILE);
    if (NT_SUCCESS(Status))
    {
        /* Delete it */
        Disposition.DeleteFile = TRUE;
        Status = NtSetInformationFile(FileHandle,
                                      &IoStatusBlock,
                                      &Disposition,
                                      sizeof(Disposition),
                                      FileDispositionInformation);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("SMSS:PFILE: Failed to delete page file `%wZ' (status %X)\n",
                    FileName, Status);
        }
        else
        {
            DPRINT1("SMSS:PFILE: Deleted stale paging file - %wZ\n", FileName);
        }

        /* Close the handle */
        NtClose(FileHandle);
    }
    else
    {
        DPRINT1("SMSS:PFILE: Failed to open for deletion page file `%wZ' (status %X)\n",
                FileName, Status);
    }

    /* All done */
    return Status;
}

NTSTATUS
NTAPI
SmpGetVolumeFreeSpace(IN PSMP_VOLUME_DESCRIPTOR Volume)
{
    NTSTATUS Status;
    LARGE_INTEGER FreeSpace, FinalFreeSpace;
    FILE_FS_SIZE_INFORMATION SizeInfo;
    IO_STATUS_BLOCK IoStatusBlock;
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING VolumeName;
    HANDLE VolumeHandle;
    WCHAR PathString[32];
    ASSERT(Volume->Flags & SMP_VOLUME_IS_BOOT); // ASSERT says "BootVolume == 1"

    /* Build the standard path */
    wcscpy(PathString, L"\\??\\A:\\");
    RtlInitUnicodeString(&VolumeName, PathString);
    VolumeName.Buffer[STANDARD_DRIVE_LETTER_OFFSET] = Volume->DriveLetter;
    DPRINT("SMSS:PFILE: Querying volume `%wZ' for free space\n", &VolumeName);

    /* Open the volume */
    InitializeObjectAttributes(&ObjectAttributes,
                               &VolumeName,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);
    Status = NtOpenFile(&VolumeHandle,
                        FILE_READ_ATTRIBUTES | FILE_WRITE_ATTRIBUTES | SYNCHRONIZE,
                        &ObjectAttributes,
                        &IoStatusBlock,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        FILE_SYNCHRONOUS_IO_NONALERT | FILE_DIRECTORY_FILE);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("SMSS:PFILE: Open volume `%wZ' failed with status %X\n", &VolumeName, Status);
        return Status;
    }

    /* Now get size information on the volume */
    Status = NtQueryVolumeInformationFile(VolumeHandle,
                                          &IoStatusBlock,
                                          &SizeInfo,
                                          sizeof(SizeInfo),
                                          FileFsSizeInformation);
    if (!NT_SUCCESS(Status))
    {
        /* We failed */
        DPRINT1("SMSS:PFILE: Query volume `%wZ' (handle %p) for size failed"
                " with status %X\n",
                &VolumeName,
                VolumeHandle,
                Status);
        NtClose(VolumeHandle);
        return Status;
    }
    NtClose(VolumeHandle);

    /* Compute how much free space we have */
    FreeSpace.QuadPart = SizeInfo.AvailableAllocationUnits.QuadPart *
                         SizeInfo.SectorsPerAllocationUnit;
    FinalFreeSpace.QuadPart = FreeSpace.QuadPart * SizeInfo.BytesPerSector;

    /* Check if there's less than 32MB free so we don't starve the disk */
    if (FinalFreeSpace.QuadPart <= MINIMUM_TO_KEEP_FREE)
    {
        /* In this case, act as if there's no free space  */
        Volume->FreeSpace.QuadPart = 0;
    }
    else
    {
        /* Trim off 32MB to give the disk a bit of breathing room */
        Volume->FreeSpace.QuadPart = FinalFreeSpace.QuadPart -
                                     MINIMUM_TO_KEEP_FREE;
    }

    return STATUS_SUCCESS;
}

PSMP_VOLUME_DESCRIPTOR
NTAPI
SmpSearchVolumeDescriptor(IN WCHAR DriveLetter)
{
    WCHAR UpLetter;
    PSMP_VOLUME_DESCRIPTOR Volume = NULL;
    PLIST_ENTRY NextEntry;

    /* Use upper case to reduce differences */
    UpLetter = RtlUpcaseUnicodeChar(DriveLetter);

    /* Loop each volume */
    NextEntry = SmpVolumeDescriptorList.Flink;
    while (NextEntry != &SmpVolumeDescriptorList)
    {
        /* Grab the entry */
        Volume = CONTAINING_RECORD(NextEntry, SMP_VOLUME_DESCRIPTOR, Entry);

        /* Make sure it's a valid entry with an uppcase drive letter */
        ASSERT(Volume->Flags & SMP_VOLUME_INSERTED); // Volume->Initialized in ASSERT
        ASSERT(Volume->DriveLetter >= L'A' && Volume->DriveLetter <= L'Z');

        /* Break if it matches, if not, keep going */
        if (Volume->DriveLetter == UpLetter) break;
        NextEntry = NextEntry->Flink;
    }

    /* Return the volume if one was found */
    if (NextEntry == &SmpVolumeDescriptorList) Volume = NULL;
    return Volume;
}

NTSTATUS
NTAPI
SmpCreatePagingFile(IN PUNICODE_STRING Name,
                    IN PLARGE_INTEGER MinSize,
                    IN PLARGE_INTEGER MaxSize,
                    IN ULONG Priority)
{
    NTSTATUS Status;

    /* Tell the kernel to create the pagefile */
    Status = NtCreatePagingFile(Name, MinSize, MaxSize, Priority);
    if (NT_SUCCESS(Status))
    {
        DPRINT("SMSS:PFILE: NtCreatePagingFile (%wZ, %I64X, %I64X) succeeded.\n",
                Name,
                MinSize->QuadPart,
                MaxSize->QuadPart);
    }
    else
    {
        DPRINT1("SMSS:PFILE: NtCreatePagingFile (%wZ, %I64X, %I64X) failed with %X\n",
                Name,
                MinSize->QuadPart,
                MaxSize->QuadPart,
                Status);
    }

    /* Return the status */
    return Status;
}

NTSTATUS
NTAPI
SmpCreatePagingFileOnFixedDrive(IN PSMP_PAGEFILE_DESCRIPTOR Descriptor,
                                IN PLARGE_INTEGER FuzzFactor,
                                IN PLARGE_INTEGER MinimumSize)
{
    PSMP_VOLUME_DESCRIPTOR Volume;
    BOOLEAN ShouldDelete;
    NTSTATUS Status;
    LARGE_INTEGER PageFileSize;
    ASSERT(Descriptor->Name.Buffer[STANDARD_DRIVE_LETTER_OFFSET] != L'?');

    /* Try to find the volume descriptor for this drive letter */
    ShouldDelete = FALSE;
    Volume = SmpSearchVolumeDescriptor(Descriptor->Name.Buffer[STANDARD_DRIVE_LETTER_OFFSET]);
    if (!Volume)
    {
        /* Couldn't find it, fail */
        DPRINT1("SMSS:PFILE: No volume descriptor for `%wZ'\n",
                &Descriptor->Name);
        return STATUS_INVALID_PARAMETER;
    }

    /* Check if this is the boot volume */
    if (Volume->Flags & SMP_VOLUME_IS_BOOT)
    {
        /* Check if we haven't yet processed a crash dump on this volume */
        if (!(Descriptor->Flags & SMP_PAGEFILE_DUMP_PROCESSED))
        {
            /* Try to find a crash dump and extract it */
            DPRINT("SMSS:PFILE: Checking for crash dump in `%wZ' on boot volume\n",
                    &Descriptor->Name);
            SmpCheckForCrashDump(&Descriptor->Name);

            /* Update how much free space we have now that we extracted a dump */
            Status = SmpGetVolumeFreeSpace(Volume);
            if (!NT_SUCCESS(Status))
            {
                DPRINT1("SMSS:PFILE: Failed to query free space for boot volume `%wC'\n",
                        Volume->DriveLetter);
            }
            else
            {
                DPRINT("Queried free space for boot volume `%wC: %I64x'\n",
                        Volume->DriveLetter, Volume->FreeSpace.QuadPart);
            }

            /* Don't process crashdump on this volume anymore */
            Descriptor->Flags |= SMP_PAGEFILE_DUMP_PROCESSED;
        }
    }
    else
    {
        /* Crashdumps can only be on the boot volume */
        DPRINT("SMSS:PFILE: Skipping crash dump checking for `%wZ' on non boot"
                "volume `%wC'\n",
                &Descriptor->Name,
                Volume->DriveLetter);
    }

    /* Update the size after dump extraction */
    Descriptor->ActualMinSize = Descriptor->MinSize;
    Descriptor->ActualMaxSize = Descriptor->MaxSize;

    /* Check how big we can make the pagefile */
    Status = SmpGetPagingFileSize(&Descriptor->Name, &PageFileSize);
    if (NT_SUCCESS(Status) && PageFileSize.QuadPart > 0) ShouldDelete = TRUE;
    DPRINT("SMSS:PFILE: Detected size %I64X for future paging file `%wZ'\n",
            PageFileSize,
            &Descriptor->Name);
    DPRINT("SMSS:PFILE: Free space on volume `%wC' is %I64X\n",
            Volume->DriveLetter,
            Volume->FreeSpace.QuadPart);

    /* Now update our size and make sure none of these are too big */
    PageFileSize.QuadPart += Volume->FreeSpace.QuadPart;
    if (Descriptor->ActualMinSize.QuadPart > PageFileSize.QuadPart)
    {
        Descriptor->ActualMinSize = PageFileSize;
    }
    if (Descriptor->ActualMaxSize.QuadPart > PageFileSize.QuadPart)
    {
        Descriptor->ActualMaxSize = PageFileSize;
    }
    DPRINT("SMSS:PFILE: min %I64X, max %I64X, real min %I64X\n",
            Descriptor->ActualMinSize.QuadPart,
            Descriptor->ActualMaxSize.QuadPart,
            MinimumSize->QuadPart);

    /* Keep going until we've created a pagefile of the right size */
    while (Descriptor->ActualMinSize.QuadPart >= MinimumSize->QuadPart)
    {
        /* Call NT to do it */
        Status = SmpCreatePagingFile(&Descriptor->Name,
                                     &Descriptor->ActualMinSize,
                                     &Descriptor->ActualMaxSize,
                                     0);
        if (NT_SUCCESS(Status))
        {
            /* We're done, update flags and increase the count */
            Descriptor->Flags |= SMP_PAGEFILE_CREATED;
            Volume->Flags |= SMP_VOLUME_PAGEFILE_CREATED;
            Volume->PageFileCount++;
            break;
        }

        /* We failed, try a slighly smaller pagefile */
        Descriptor->ActualMinSize.QuadPart -= FuzzFactor->QuadPart;
    }

    /* Check if we weren't able to create it */
    if (Descriptor->ActualMinSize.QuadPart < MinimumSize->QuadPart)
    {
        /* Delete the current page file and fail */
        if (ShouldDelete)
        {
            SmpDeletePagingFile(&Descriptor->Name);

            /* FIXFIX: Windows Vista does this, and it seems like we should too, so try to see if this fixes KVM */
            Volume->FreeSpace.QuadPart = PageFileSize.QuadPart;
        }
        DPRINT1("SMSS:PFILE: Failing for min %I64X, max %I64X, real min %I64X\n",
                Descriptor->ActualMinSize.QuadPart,
                Descriptor->ActualMaxSize.QuadPart,
                MinimumSize->QuadPart);
        Status = STATUS_DISK_FULL;
    }

    /* Return the status */
    return Status;
}

NTSTATUS
NTAPI
SmpCreatePagingFileOnAnyDrive(IN PSMP_PAGEFILE_DESCRIPTOR Descriptor,
                              IN PLARGE_INTEGER FuzzFactor,
                              IN PLARGE_INTEGER MinimumSize)
{
    PSMP_VOLUME_DESCRIPTOR Volume;
    NTSTATUS Status = STATUS_DISK_FULL;
    PLIST_ENTRY NextEntry;
    ASSERT(Descriptor->Name.Buffer[STANDARD_DRIVE_LETTER_OFFSET] == L'?');

    /* Loop the volume list */
    NextEntry = SmpVolumeDescriptorList.Flink;
    while (NextEntry != &SmpVolumeDescriptorList)
    {
        /* Get the volume */
        Volume = CONTAINING_RECORD(NextEntry, SMP_VOLUME_DESCRIPTOR, Entry);

        /* Make sure it's inserted and on a valid drive letter */
        ASSERT(Volume->Flags & SMP_VOLUME_INSERTED); // Volume->Initialized in ASSERT
        ASSERT(Volume->DriveLetter >= L'A' && Volume->DriveLetter <= L'Z');

        /* Write the drive letter to try creating it on this volume */
        Descriptor->Name.Buffer[STANDARD_DRIVE_LETTER_OFFSET] = Volume->DriveLetter;
        Status = SmpCreatePagingFileOnFixedDrive(Descriptor,
                                                 FuzzFactor,
                                                 MinimumSize);
        if (NT_SUCCESS(Status)) break;

        /* It didn't work, make it an any pagefile again and keep going */
        Descriptor->Name.Buffer[STANDARD_DRIVE_LETTER_OFFSET] = L'?';
        NextEntry = NextEntry->Flink;
    }

    /* Return disk full or success */
    return Status;
}

VOID
NTAPI
SmpMakeDefaultPagingFileDescriptor(IN PSMP_PAGEFILE_DESCRIPTOR Descriptor)
{
    /* The default descriptor uses 128MB as a pagefile size */
    Descriptor->Flags |= SMP_PAGEFILE_DEFAULT;
    Descriptor->MinSize.QuadPart = 128 * MEGABYTE;
    Descriptor->MaxSize.QuadPart = 128 * MEGABYTE;
}

VOID
NTAPI
SmpMakeSystemManagedPagingFileDescriptor(IN PSMP_PAGEFILE_DESCRIPTOR Descriptor)
{
    NTSTATUS Status;
    LONGLONG MinimumSize, MaximumSize, Ram;
    SYSTEM_BASIC_INFORMATION BasicInfo;

    /* Query the page size of the system, and the amount of RAM */
    Status = NtQuerySystemInformation(SystemBasicInformation,
                                      &BasicInfo,
                                      sizeof(BasicInfo),
                                      NULL);
    if (!NT_SUCCESS(Status))
    {
        /* If we failed, use defaults since we have no idea otherwise */
        DPRINT1("SMSS:PFILE: NtQuerySystemInformation failed with %x\n", Status);
        SmpMakeDefaultPagingFileDescriptor(Descriptor);
        return;
    }

    /* Chekc how much RAM we have and set three times this amount as maximum */
    Ram = BasicInfo.NumberOfPhysicalPages * BasicInfo.PageSize;
    MaximumSize = 3 * Ram;

    /* If we have more than 1GB, use that as minimum, otherwise, use 1.5X RAM */
    MinimumSize = (Ram >= 1024 * MEGABYTE) ? Ram : MaximumSize / 2;

    /* Write the new sizes in the descriptor and mark it as system managed */
    Descriptor->MinSize.QuadPart = MinimumSize;
    Descriptor->MaxSize.QuadPart = MaximumSize;
    Descriptor->Flags |= SMP_PAGEFILE_SYSTEM_MANAGED;
}

NTSTATUS
NTAPI
SmpValidatePagingFileSizes(IN PSMP_PAGEFILE_DESCRIPTOR Descriptor)
{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONGLONG MinSize, MaxSize;
    BOOLEAN WasTooBig = FALSE;

    /* Capture the min and max */
    MinSize = Descriptor->MinSize.QuadPart;
    MaxSize = Descriptor->MaxSize.QuadPart;
    DPRINT("SMSS:PFILE: Validating sizes for `%wZ' %I64X %I64X\n",
             &Descriptor->Name, MinSize, MaxSize);

    /* Don't let minimum be bigger than maximum */
    if (MinSize > MaxSize) MaxSize = MinSize;

    /* On PAE we can have bigger pagefiles... */
    if (SharedUserData->ProcessorFeatures[PF_PAE_ENABLED])
    {
        /* But we don't support that yet */
        DPRINT1("ReactOS does not support PAE yet... assuming sizes OK\n");
    }
    else
    {
        /* Check if the minimum is more then 4095 MB */
        if (MinSize > MAXIMUM_PAGEFILE_SIZE)
        {
            /* Trim it, this isn't allowed */
            WasTooBig = TRUE;
            MinSize = MAXIMUM_PAGEFILE_SIZE;
        }

        /* Check if the maximum is more then 4095 MB */
        if (MaxSize > MAXIMUM_PAGEFILE_SIZE)
        {
            /* Trim it, this isn't allowed */
            WasTooBig = TRUE;
            MaxSize = MAXIMUM_PAGEFILE_SIZE;
        }
    }

    /* Did we trim? */
    if (WasTooBig)
    {
        /* Notify debugger output and write a flag in the descriptor */
        DPRINT("SMSS:PFILE: Trimmed size of `%wZ' to maximum allowed\n",
                &Descriptor->Name);
        Descriptor->Flags |= SMP_PAGEFILE_WAS_TOO_BIG;
    }

    /* Now write the (possibly trimmed) sizes back */
    Descriptor->MinSize.QuadPart = MinSize;
    Descriptor->MaxSize.QuadPart = MaxSize;
    return Status;
}

NTSTATUS
NTAPI
SmpCreateSystemManagedPagingFile(IN PSMP_PAGEFILE_DESCRIPTOR Descriptor,
                                 IN BOOLEAN DecreaseSize)
{
    LARGE_INTEGER FuzzFactor, Size;

    /* Make sure there's at least 1 paging file and that we are system-managed */
    ASSERT(SmpNumberOfPagingFiles >= 1);
    ASSERT(!IsListEmpty(&SmpPagingFileDescriptorList));
    ASSERT(Descriptor->Flags & SMP_PAGEFILE_SYSTEM_MANAGED); // Descriptor->SystemManaged == 1 in ASSERT.

    /* Keep decreasing the pagefile by this amount if we run out of space */
    FuzzFactor.QuadPart = FUZZ_FACTOR;

    /* Create the descriptor for it (mainly the right sizes) and validate */
    SmpMakeSystemManagedPagingFileDescriptor(Descriptor);
    SmpValidatePagingFileSizes(Descriptor);

    /* Use either the minimum size in the descriptor, or 16MB in minimal mode */
    Size.QuadPart = DecreaseSize ? 16 * MEGABYTE : Descriptor->MinSize.QuadPart;

    /* Check if this should be a fixed pagefile or an any pagefile*/
    if (Descriptor->Name.Buffer[STANDARD_DRIVE_LETTER_OFFSET] == '?')
    {
        /* Find a disk for it */
        return SmpCreatePagingFileOnAnyDrive(Descriptor, &FuzzFactor, &Size);
    }

    /* Use the disk that was given */
    return SmpCreatePagingFileOnFixedDrive(Descriptor, &FuzzFactor, &Size);
}

NTSTATUS
NTAPI
SmpCreateEmergencyPagingFile(VOID)
{
    PSMP_PAGEFILE_DESCRIPTOR Descriptor;
    WCHAR Buffer[32];

    /* Allocate a descriptor */
    Descriptor = RtlAllocateHeap(RtlGetProcessHeap(),
                                 HEAP_ZERO_MEMORY,
                                 sizeof(SMP_PAGEFILE_DESCRIPTOR));
    if (!Descriptor) return STATUS_NO_MEMORY;

    /* Initialize it */
    RtlInitUnicodeString(&Descriptor->Token, NULL);

    /* Copy the default pagefile name */
    ASSERT(sizeof(Buffer) >= sizeof(STANDARD_PAGING_FILE_NAME));
    wcscpy(Buffer, STANDARD_PAGING_FILE_NAME);

    /* Fill the rest of the descriptor out */
    RtlInitUnicodeString(&Descriptor->Name, Buffer);
    Descriptor->Name.Buffer[STANDARD_DRIVE_LETTER_OFFSET] = '?';
    Descriptor->Flags |= SMP_PAGEFILE_SYSTEM_MANAGED |
                         SMP_PAGEFILE_EMERGENCY |
                         SMP_PAGEFILE_ON_ANY_DRIVE;

    /* Insert it into the descriptor list */
    InsertHeadList(&SmpPagingFileDescriptorList, &Descriptor->Entry);
    SmpNumberOfPagingFiles++;

    /* Go ahead and create it now, with the minimal size possible */
    return SmpCreateSystemManagedPagingFile(Descriptor, TRUE);
}

NTSTATUS
NTAPI
SmpCreateVolumeDescriptors(VOID)
{
    NTSTATUS Status;
    UNICODE_STRING VolumePath;
    BOOLEAN BootVolumeFound = FALSE;
    WCHAR StartChar, Drive, DriveDiff;
    HANDLE VolumeHandle;
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatusBlock;
    PROCESS_DEVICEMAP_INFORMATION ProcessInformation;
    FILE_FS_DEVICE_INFORMATION DeviceInfo;
    FILE_FS_SIZE_INFORMATION SizeInfo;
    PSMP_VOLUME_DESCRIPTOR Volume;
    LARGE_INTEGER FreeSpace, FinalFreeSpace;
    WCHAR Buffer[32];

    /* We should be starting with an empty list */
    ASSERT(IsListEmpty(&SmpVolumeDescriptorList));

    /* Query the device map so we can get the drive letters */
    Status = NtQueryInformationProcess(NtCurrentProcess(),
                                       ProcessDeviceMap,
                                       &ProcessInformation,
                                       sizeof(ProcessInformation),
                                       NULL);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("SMSS:PFILE: Query(ProcessDeviceMap) failed with status %X\n",
                Status);
        return Status;
    }

    /* Build the volume string, starting with A: (we'll edit this in place) */
    wcscpy(Buffer, L"\\??\\A:\\");
    RtlInitUnicodeString(&VolumePath, Buffer);

    /* Start with the C drive except on weird Japanese NECs... */
    StartChar = SharedUserData->AlternativeArchitecture ? L'A' : L'C';
    for (Drive = StartChar, DriveDiff = StartChar - L'A'; Drive <= L'Z'; Drive++, DriveDiff++)
    {
        /* Skip the disk if it's not in the drive map */
        if (!((1 << DriveDiff) & ProcessInformation.Query.DriveMap)) continue;

        /* Write the drive letter and try to open the volume */
        VolumePath.Buffer[STANDARD_DRIVE_LETTER_OFFSET] = Drive;
        InitializeObjectAttributes(&ObjectAttributes,
                                   &VolumePath,
                                   OBJ_CASE_INSENSITIVE,
                                   NULL,
                                   NULL);
        Status = NtOpenFile(&VolumeHandle,
                            FILE_READ_ATTRIBUTES | FILE_WRITE_ATTRIBUTES | SYNCHRONIZE,
                            &ObjectAttributes,
                            &IoStatusBlock,
                            FILE_SHARE_READ | FILE_SHARE_WRITE,
                            FILE_SYNCHRONOUS_IO_NONALERT | FILE_DIRECTORY_FILE);
        if (!NT_SUCCESS(Status))
        {
            /* Skip the volume if we failed */
            DPRINT1("SMSS:PFILE: Open volume `%wZ' failed with status %X\n",
                    &VolumePath, Status);
            continue;
        }

        /* Now query device information on the volume */
        Status = NtQueryVolumeInformationFile(VolumeHandle,
                                              &IoStatusBlock,
                                              &DeviceInfo,
                                              sizeof(DeviceInfo),
                                              FileFsDeviceInformation);
        if (!NT_SUCCESS(Status))
        {
            /* Move to the next volume if we failed */
            DPRINT1("SMSS:PFILE: Query volume `%wZ' (handle %p) for device info"
                    " failed with status %X\n",
                    &VolumePath,
                    VolumeHandle,
                    Status);
            NtClose(VolumeHandle);
            continue;
        }

        /* Check if this is a fixed disk */
        if (DeviceInfo.Characteristics & (FILE_FLOPPY_DISKETTE |
                                          FILE_READ_ONLY_DEVICE |
                                          FILE_REMOTE_DEVICE |
                                          FILE_REMOVABLE_MEDIA))
        {
            /* It isn't, so skip it */
            DPRINT1("SMSS:PFILE: Volume `%wZ' (%X) cannot store a paging file\n",
                    &VolumePath,
                    DeviceInfo.Characteristics);
            NtClose(VolumeHandle);
            continue;
        }

        /* We found a fixed volume, allocate a descriptor for it */
        Volume = RtlAllocateHeap(RtlGetProcessHeap(),
                                 HEAP_ZERO_MEMORY,
                                 sizeof(SMP_VOLUME_DESCRIPTOR));
        if (!Volume)
        {
            /* Failed to allocate memory, try the next disk */
            DPRINT1("SMSS:PFILE: Failed to allocate a volume descriptor (%u bytes)\n",
                    sizeof(SMP_VOLUME_DESCRIPTOR));
            NtClose(VolumeHandle);
            continue;
        }

        /* Save the drive letter and device information */
        Volume->DriveLetter = Drive;
        Volume->DeviceInfo = DeviceInfo;

        /* Check if this is the boot volume */
        if (RtlUpcaseUnicodeChar(Drive) ==
            RtlUpcaseUnicodeChar(SharedUserData->NtSystemRoot[0]))
        {
            /* Save it */
            ASSERT(BootVolumeFound == FALSE);
            Volume->Flags |= SMP_VOLUME_IS_BOOT;
            BootVolumeFound = TRUE;
        }

        /* Now get size information on the volume */
        Status = NtQueryVolumeInformationFile(VolumeHandle,
                                              &IoStatusBlock,
                                              &SizeInfo,
                                              sizeof(SizeInfo),
                                              FileFsSizeInformation);
        if (!NT_SUCCESS(Status))
        {
            /* We failed -- keep going */
            DPRINT1("SMSS:PFILE: Query volume `%wZ' (handle %p) for size failed"
                    " with status %X\n",
                    &VolumePath,
                    VolumeHandle,
                    Status);
            RtlFreeHeap(RtlGetProcessHeap(), 0, Volume);
            NtClose(VolumeHandle);
            continue;
        }

        /* Done querying volume information, close the handle */
        NtClose(VolumeHandle);

        /* Compute how much free space we have */
        FreeSpace.QuadPart = SizeInfo.AvailableAllocationUnits.QuadPart *
                             SizeInfo.SectorsPerAllocationUnit;
        FinalFreeSpace.QuadPart = FreeSpace.QuadPart * SizeInfo.BytesPerSector;

        /* Check if there's less than 32MB free so we don't starve the disk */
        if (FinalFreeSpace.QuadPart <= MINIMUM_TO_KEEP_FREE)
        {
            /* In this case, act as if there's no free space  */
            Volume->FreeSpace.QuadPart = 0;
        }
        else
        {
            /* Trim off 32MB to give the disk a bit of breathing room */
            Volume->FreeSpace.QuadPart = FinalFreeSpace.QuadPart -
                                         MINIMUM_TO_KEEP_FREE;
        }

        /* All done, add this volume to our descriptor list */
        InsertTailList(&SmpVolumeDescriptorList, &Volume->Entry);
        Volume->Flags |= SMP_VOLUME_INSERTED;
        DPRINT("SMSS:PFILE: Created volume descriptor for`%wZ'\n", &VolumePath);
    }

    /* We must've found at least the boot volume */
    ASSERT(BootVolumeFound == TRUE);
    ASSERT(!IsListEmpty(&SmpVolumeDescriptorList));
    if (!IsListEmpty(&SmpVolumeDescriptorList)) return STATUS_SUCCESS;

    /* Something is really messed up if we found no disks at all */
    return STATUS_UNEXPECTED_IO_ERROR;
}

NTSTATUS
NTAPI
SmpCreatePagingFiles(VOID)
{
    NTSTATUS Status;
    PSMP_PAGEFILE_DESCRIPTOR Descriptor;
    LARGE_INTEGER Size, FuzzFactor;
    BOOLEAN Created = FALSE;
    PLIST_ENTRY NextEntry;

    /* Check if no paging files were requested */
    if (!(SmpNumberOfPagingFiles) && !(SmpRegistrySpecifierPresent))
    {
        /* The list should be empty -- nothing to do */
        ASSERT(IsListEmpty(&SmpPagingFileDescriptorList));
        DPRINT1("SMSS:PFILE: No paging file was requested\n");
        return STATUS_SUCCESS;
    }

    /* Initialize the volume descriptors so we can know what's available */
    Status = SmpCreateVolumeDescriptors();
    if (!NT_SUCCESS(Status))
    {
        /* We can't make decisions without this, so fail */
        DPRINT1("SMSS:PFILE: Failed to create volume descriptors (status %X)\n",
                Status);
        return Status;
    }

    /* If we fail creating pagefiles, try to reduce by this much each time */
    FuzzFactor.QuadPart = FUZZ_FACTOR;

    /* Loop the descriptor list */
    NextEntry = SmpPagingFileDescriptorList.Flink;
    while (NextEntry != &SmpPagingFileDescriptorList)
    {
        /* Check what kind of descriptor this is */
        Descriptor = CONTAINING_RECORD(NextEntry, SMP_PAGEFILE_DESCRIPTOR, Entry);
        if (Descriptor->Flags & SMP_PAGEFILE_SYSTEM_MANAGED)
        {
            /* This is a system-managed descriptor. Create the correct file */
            DPRINT("SMSS:PFILE: Creating a system managed paging file (`%wZ')\n",
                    &Descriptor->Name);
            Status = SmpCreateSystemManagedPagingFile(Descriptor, FALSE);
            if (!NT_SUCCESS(Status))
            {
                /* We failed -- try again, with size minimization this time */
                DPRINT1("SMSS:PFILE: Trying lower sizes for (`%wZ')\n",
                        &Descriptor->Name);
                Status = SmpCreateSystemManagedPagingFile(Descriptor, TRUE);
            }
        }
        else
        {
            /* This is a manually entered descriptor. Validate its size first */
            SmpValidatePagingFileSizes(Descriptor);

            /* Check if this is an ANY pagefile or a FIXED pagefile */
            DPRINT("SMSS:PFILE: Creating a normal paging file (`%wZ')\n",
                    &Descriptor->Name);
            if (Descriptor->Name.Buffer[STANDARD_DRIVE_LETTER_OFFSET] == L'?')
            {
                /* It's an any pagefile, try to create it wherever possible */
                Size = Descriptor->MinSize;
                Status = SmpCreatePagingFileOnAnyDrive(Descriptor,
                                                       &FuzzFactor,
                                                       &Size);
                if (!NT_SUCCESS(Status))
                {
                    /* We failed to create it. Try again with a smaller size */
                    DPRINT1("SMSS:PFILE: Trying lower sizes for (`%wZ')\n",
                            &Descriptor->Name);
                    Size.QuadPart = 16 * MEGABYTE;
                    Status = SmpCreatePagingFileOnAnyDrive(Descriptor,
                                                           &FuzzFactor,
                                                           &Size);
                }
            }
            else
            {
                /* It's a fixed pagefile: override the minimum and use ours */
                Size.QuadPart = 16 * MEGABYTE;
                Status = SmpCreatePagingFileOnFixedDrive(Descriptor,
                                                         &FuzzFactor,
                                                         &Size);
            }
        }

        /* Go to the next descriptor */
        if (NT_SUCCESS(Status)) Created = TRUE;
        NextEntry = NextEntry->Flink;
    }

    /* Check if none of the code in our loops above was able to create it */
    if (!Created)
    {
        /* Build an emergency pagefile ourselves */
        DPRINT1("SMSS:PFILE: Creating emergency paging file.\n");
        Status = SmpCreateEmergencyPagingFile();
    }

    /* All done */
    return Status;
}
