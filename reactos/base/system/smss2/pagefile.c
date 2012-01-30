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

#define STANDARD_DRIVE_LETTER_OFFSET 4

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
SmpCreatePagingFiles(VOID)
{
    return STATUS_SUCCESS;
}
