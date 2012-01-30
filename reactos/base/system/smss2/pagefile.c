/*
 * PROJECT:         ReactOS Windows-Compatible Session Manager
 * LICENSE:         BSD 2-Clause License
 * FILE:            base/system/smss/smss.c
 * PURPOSE:         Main SMSS Code
 * PROGRAMMERS:     Alex Ionescu
 */

/* INCLUDES *******************************************************************/

#include "smss.h"
#define NDEBUG
#include "debug.h"

/* GLOBALS ********************************************************************/

//
// Taken from ASSERTs
//
#define STANDARD_PAGING_FILE_NAME       L"\\??\\?:\\pagefile.sys"
#define STANDARD_DRIVE_LETTER_OFFSET    4

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
    PWCHAR p, ArgBuffer;
    PLIST_ENTRY NextEntry;
    UNICODE_STRING PageFileName, Arguments;

    /* Make sure we don't have too many */
    if (SmpNumberOfPagingFiles >= 16)
    {
        DPRINT1("SMSS:PFILE: Too many paging files specified - %d\n",
                SmpNumberOfPagingFiles);
        return STATUS_TOO_MANY_PAGING_FILES;
    }

    /* Parse the specified and get the name and arguments out of it */
    DPRINT1("SMSS:PFILE: Paging file specifier `%wZ' \n", PageFileToken);
    Status = SmpParseCommandLine(PageFileToken,
                                 NULL,
                                 &PageFileName,
                                 NULL,
                                 &Arguments);
    if (!NT_SUCCESS(Status))
    {
        /* Fail */
        DPRINT1("SMSS:PFILE: SmpParseCommandLine(%wZ) failed with status %X \n",
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
    if ((Arguments.Buffer) || (ZeroSize))
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
        ArgBuffer = Arguments.Buffer;
        p = ArgBuffer;
        while (*p++)
        {
            /* Which should be a space... */
            if (*p == L' ')
            {
                /* And use the rest of the arguments as a maximum size */
                Arguments.Length -= ((PCHAR)p - (PCHAR)ArgBuffer);
                Arguments.Buffer = ArgBuffer;
                Status = RtlUnicodeStringToInteger(&Arguments, 0, &MaxSize);
                if (!NT_SUCCESS(Status))
                {
                    /* Fail */
                    RtlFreeUnicodeString(&PageFileName);
                    RtlFreeUnicodeString(&Arguments);
                    return Status;
                }

                /* We have both min and max, restore argument buffer */
                Arguments.Buffer = ArgBuffer; // Actual Windows Bug in faillure case above.
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
    Descriptor->MinSize.QuadPart = MinSize * 0x100000;
    Descriptor->MaxSize.QuadPart = MaxSize * 0x100000;
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
            DPRINT1("SMSS:PFILE: Created descriptor for `%wZ' (`%wZ') \n",
                    PageFileToken, &Descriptor->Name);
            return STATUS_SUCCESS;
        }

        /* Keep going until we find a duplicate, unless we are in "any" mode */
        ListDescriptor = CONTAINING_RECORD(NextEntry, SMP_PAGEFILE_DESCRIPTOR, Entry);
        NextEntry = NextEntry->Flink;
    } while (!(ListDescriptor->Flags & SMP_PAGEFILE_ON_ANY_DRIVE) ||
             !(Descriptor->Flags & SMP_PAGEFILE_ON_ANY_DRIVE));

    /* We found a duplicate, so skip this descriptor/pagefile and fail */
    DPRINT1("SMSS:PFILE: Skipping duplicate specifier `%wZ' \n", PageFileToken);
    RtlFreeUnicodeString(&PageFileName);
    RtlFreeHeap(RtlGetProcessHeap(), 0, Descriptor);
    return STATUS_INVALID_PARAMETER;
}

NTSTATUS
NTAPI
SmpCreatePagingFileOnFixedDrive(IN PSMP_PAGEFILE_DESCRIPTOR Descriptor,
                                IN PLARGE_INTEGER FuzzFactor,
                                IN PLARGE_INTEGER MinimumSize)
{
    DPRINT1("Should create fixed pagefile of sizes: %I64d %I64d\n",
            FuzzFactor->QuadPart, MinimumSize->QuadPart);
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
SmpCreatePagingFileOnAnyDrive(IN PSMP_PAGEFILE_DESCRIPTOR Descriptor,
                              IN PLARGE_INTEGER FuzzFactor,
                              IN PLARGE_INTEGER MinimumSize)
{
    DPRINT1("Should create 'any' pagefile of sizes: %I64d %I64d\n",
            FuzzFactor->QuadPart, MinimumSize->QuadPart);
    return STATUS_SUCCESS;
}

VOID
NTAPI
SmpMakeDefaultPagingFileDescriptor(IN PSMP_PAGEFILE_DESCRIPTOR Descriptor)
{
    /* The default descriptor uses 128MB as a pagefile size */
    Descriptor->Flags |= SMP_PAGEFILE_DEFAULT;
    Descriptor->MinSize.QuadPart = 0x8000000;
    Descriptor->MaxSize.QuadPart = 0x8000000;
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
        DPRINT1("SMSS:PFILE: NtQuerySystemInformation failed with %x \n", Status);
        SmpMakeDefaultPagingFileDescriptor(Descriptor);
        return;
    }

    /* Chekc how much RAM we have and set three times this amount as maximum */
    Ram = BasicInfo.NumberOfPhysicalPages * BasicInfo.PageSize;
    MaximumSize = 3 * Ram;

    /* If we have more than 1GB, use that as minimum, otherwise, use 1.5X RAM */
    MinimumSize = (Ram >= 0x40000000) ? Ram : MaximumSize / 2;

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
    DPRINT1("SMSS:PFILE: Validating sizes for `%wZ' %I64X %I64X\n",
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
        if (MinSize > 0xFFF00000)
        {
            /* Trim it, this isn't allowed */
            WasTooBig = TRUE;
            MinSize = 0xFFF00000;
        }

        /* Check if the maximum is more then 4095 MB */
        if (MaxSize > 0xFFF00000)
        {
            /* Trim it, this isn't allowed */
            WasTooBig = TRUE;
            MaxSize = 0xFFF00000;
        }
    }

    /* Did we trim? */
    if (WasTooBig)
    {
        /* Notify debugger output and write a flag in the descriptor */
        DPRINT1("SMSS:PFILE: Trimmed size of `%wZ' to maximum allowed \n",
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
    FuzzFactor.QuadPart = 0x1000000;

    /* Create the descriptor for it (mainly the right sizes) and validate */
    SmpMakeSystemManagedPagingFileDescriptor(Descriptor);
    SmpValidatePagingFileSizes(Descriptor);

    /* Use either the minimum size in the descriptor, or 16MB in minimal mode */
    Size.QuadPart = DecreaseSize ? 0x1000000 : Descriptor->MinSize.QuadPart;

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
    ULONG Length;
    WCHAR Buffer[32];

    /* Allocate a descriptor */
    Descriptor = RtlAllocateHeap(RtlGetProcessHeap(),
                                 HEAP_ZERO_MEMORY,
                                 sizeof(SMP_PAGEFILE_DESCRIPTOR));
    if (!Descriptor) return STATUS_NO_MEMORY;

    /* Initialize it */
    RtlInitUnicodeString(&Descriptor->Name, NULL);
    RtlInitUnicodeString(&Descriptor->Token, NULL);

    /* Copy the default pagefile name */
    wcscpy(Buffer, STANDARD_PAGING_FILE_NAME);
    Length = wcslen(Buffer) * sizeof(WCHAR);
    ASSERT(sizeof(Buffer) > Length);

    /* Fill the rest of the descriptor out */
    Descriptor->Name.Buffer = Buffer;
    Descriptor->Name.Length = Length;
    Descriptor->Name.MaximumLength = Length + sizeof(UNICODE_NULL);
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
    return STATUS_SUCCESS;
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
        /* The list should be empty -- nothign to do */
        ASSERT(IsListEmpty(&SmpPagingFileDescriptorList));
        DPRINT1("SMSS:PFILE: No paging file was requested \n");
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
    FuzzFactor.QuadPart = 0x1000000;

    /* Loop the descriptor list */
    NextEntry = SmpPagingFileDescriptorList.Flink;
    while (NextEntry != &SmpPagingFileDescriptorList)
    {
        /* Check what kind of descriptor this is */
        Descriptor = CONTAINING_RECORD(NextEntry, SMP_PAGEFILE_DESCRIPTOR, Entry);
        if (Descriptor->Flags & SMP_PAGEFILE_SYSTEM_MANAGED)
        {
            /* This is a system-managed descriptor. Create the correct file */
            DPRINT1("SMSS:PFILE: Creating a system managed paging file (`%wZ')\n",
                    &Descriptor->Name);
            Status = SmpCreateSystemManagedPagingFile(Descriptor, FALSE);
            if (!NT_SUCCESS(Status))
            {
                /* We failed -- try again, with size minimization this time */
                DPRINT1("SMSS:PFILE: Trying lower sizes for (`%wZ') \n",
                        &Descriptor->Name);
                Status = SmpCreateSystemManagedPagingFile(Descriptor, TRUE);
            }
        }
        else
        {
            /* This is a manually entered descriptor. Validate its size first */
            SmpValidatePagingFileSizes(Descriptor);

            /* Check if this is an ANY pagefile or a FIXED pagefile */
            DPRINT1("SMSS:PFILE: Creating a normal paging file (`%wZ') \n",
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
                    DPRINT1("SMSS:PFILE: Trying lower sizes for (`%wZ') \n",
                            &Descriptor->Name);
                    Size.QuadPart = 0x1000000;
                    Status = SmpCreatePagingFileOnAnyDrive(Descriptor,
                                                           &FuzzFactor,
                                                           &Size);
                }
            }
            else
            {
                /* It's a fixed pagefile: override the minimum and use ours */
                Size.QuadPart = 0x1000000;
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
        DPRINT1("SMSS:PFILE: Creating emergency paging file. \n");
        Status = SmpCreateEmergencyPagingFile();
    }

    /* All done */
    return Status;
}
