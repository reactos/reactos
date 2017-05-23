/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS text-mode setup
 * FILE:            base/setup/usetup/filesup.c
 * PURPOSE:         File support functions
 * PROGRAMMER:      Eric Kohl
 *                  Casper S. Hornstrup (chorns@users.sourceforge.net)
 */

/* INCLUDES *****************************************************************/

#include "usetup.h"

#define NDEBUG
#include <debug.h>

/* FUNCTIONS ****************************************************************/

static BOOLEAN HasCurrentCabinet = FALSE;
static WCHAR CurrentCabinetName[MAX_PATH];
static CAB_SEARCH Search;

static
NTSTATUS
SetupCreateSingleDirectory(
    PWCHAR DirectoryName)
{
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatusBlock;
    UNICODE_STRING PathName;
    HANDLE DirectoryHandle;
    NTSTATUS Status;

    if(!RtlCreateUnicodeString(&PathName, DirectoryName))
        return STATUS_NO_MEMORY;

    if (PathName.Length > sizeof(WCHAR) &&
        PathName.Buffer[PathName.Length / sizeof(WCHAR) - 2] == L'\\' &&
        PathName.Buffer[PathName.Length / sizeof(WCHAR) - 1] == L'.')
    {
        PathName.Length -= sizeof(WCHAR);
        PathName.Buffer[PathName.Length / sizeof(WCHAR)] = 0;
    }

    if (PathName.Length > sizeof(WCHAR) &&
        PathName.Buffer[PathName.Length / sizeof(WCHAR) - 1] == L'\\')
    {
        PathName.Length -= sizeof(WCHAR);
        PathName.Buffer[PathName.Length / sizeof(WCHAR)] = 0;
    }

    InitializeObjectAttributes(&ObjectAttributes,
                               &PathName,
                               OBJ_CASE_INSENSITIVE | OBJ_INHERIT,
                               NULL,
                               NULL);

    Status = NtCreateFile(&DirectoryHandle,
                          DIRECTORY_ALL_ACCESS,
                          &ObjectAttributes,
                          &IoStatusBlock,
                          NULL,
                          FILE_ATTRIBUTE_DIRECTORY,
                          FILE_SHARE_READ | FILE_SHARE_WRITE,
                          FILE_OPEN_IF,
                          FILE_DIRECTORY_FILE,
                          NULL,
                          0);
    if (NT_SUCCESS(Status))
    {
        NtClose(DirectoryHandle);
    }

    RtlFreeUnicodeString(&PathName);

    return Status;
}

NTSTATUS
SetupCreateDirectory(
    PWCHAR PathName)
{
    PWCHAR PathBuffer = NULL;
    PWCHAR Ptr, EndPtr;
    ULONG BackslashCount;
    ULONG Size;
    NTSTATUS Status = STATUS_SUCCESS;

    Size = (wcslen(PathName) + 1) * sizeof(WCHAR);
    PathBuffer = RtlAllocateHeap(RtlGetProcessHeap(), HEAP_ZERO_MEMORY, Size);
    if (PathBuffer == NULL)
        return STATUS_INSUFFICIENT_RESOURCES;

    wcscpy(PathBuffer, PathName);
    EndPtr = PathBuffer + wcslen(PathName);

    Ptr = PathBuffer;

    /* Skip the '\Device\HarddiskX\PartitionY\ part */
    BackslashCount = 0;
    while (Ptr < EndPtr && BackslashCount < 4)
    {
        if (*Ptr == L'\\')
            BackslashCount++;

        Ptr++;
    }

    while (Ptr < EndPtr)
    {
        if (*Ptr == L'\\')
        {
            *Ptr = 0;

            DPRINT("PathBuffer: %S\n", PathBuffer);
            if (!DoesPathExist(NULL, PathBuffer))
            {
                DPRINT("Create: %S\n", PathBuffer);
                Status = SetupCreateSingleDirectory(PathBuffer);
                if (!NT_SUCCESS(Status))
                    goto done;
            }

            *Ptr = L'\\';
        }

        Ptr++;
    }

    if (!DoesPathExist(NULL, PathBuffer))
    {
        DPRINT("Create: %S\n", PathBuffer);
        Status = SetupCreateSingleDirectory(PathBuffer);
        if (!NT_SUCCESS(Status))
            goto done;
    }

done:
    DPRINT("Done.\n");
    if (PathBuffer != NULL)
        RtlFreeHeap(RtlGetProcessHeap(), 0, PathBuffer);

    return Status;
}

NTSTATUS
SetupCopyFile(
    PWCHAR SourceFileName,
    PWCHAR DestinationFileName)
{
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE FileHandleSource;
    HANDLE FileHandleDest;
    static IO_STATUS_BLOCK IoStatusBlock;
    FILE_STANDARD_INFORMATION FileStandard;
    FILE_BASIC_INFORMATION FileBasic;
    ULONG RegionSize;
    UNICODE_STRING FileName;
    NTSTATUS Status;
    PVOID SourceFileMap = 0;
    HANDLE SourceFileSection;
    SIZE_T SourceSectionSize = 0;
    LARGE_INTEGER ByteOffset;

    RtlInitUnicodeString(&FileName,
                         SourceFileName);

    InitializeObjectAttributes(&ObjectAttributes,
                               &FileName,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    Status = NtOpenFile(&FileHandleSource,
                        GENERIC_READ,
                        &ObjectAttributes,
                        &IoStatusBlock,
                        FILE_SHARE_READ,
                        FILE_SEQUENTIAL_ONLY);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtOpenFile failed: %x, %wZ\n", Status, &FileName);
        goto done;
    }

    Status = NtQueryInformationFile(FileHandleSource,
                                    &IoStatusBlock,
                                    &FileStandard,
                                    sizeof(FILE_STANDARD_INFORMATION),
                                    FileStandardInformation);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtQueryInformationFile failed: %x\n", Status);
        goto closesrc;
    }

    Status = NtQueryInformationFile(FileHandleSource,
                                    &IoStatusBlock,&FileBasic,
                                    sizeof(FILE_BASIC_INFORMATION),
                                    FileBasicInformation);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtQueryInformationFile failed: %x\n", Status);
        goto closesrc;
    }

    Status = NtCreateSection(&SourceFileSection,
                             SECTION_MAP_READ,
                             NULL,
                             NULL,
                             PAGE_READONLY,
                             SEC_COMMIT,
                             FileHandleSource);
    if (!NT_SUCCESS(Status))
    {
      DPRINT1("NtCreateSection failed: %x, %S\n", Status, SourceFileName);
      goto closesrc;
    }

    Status = NtMapViewOfSection(SourceFileSection,
                                NtCurrentProcess(),
                                &SourceFileMap,
                                0,
                                0,
                                NULL,
                                &SourceSectionSize,
                                ViewUnmap,
                                0,
                                PAGE_READONLY );
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtMapViewOfSection failed: %x, %S\n", Status, SourceFileName);
        goto closesrcsec;
    }

    RtlInitUnicodeString(&FileName,
                         DestinationFileName);

    InitializeObjectAttributes(&ObjectAttributes,
                               &FileName,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    Status = NtCreateFile(&FileHandleDest,
                          GENERIC_WRITE | SYNCHRONIZE,
                          &ObjectAttributes,
                          &IoStatusBlock,
                          NULL,
                          FILE_ATTRIBUTE_NORMAL,
                          0,
                          FILE_OVERWRITE_IF,
                          FILE_NO_INTERMEDIATE_BUFFERING |
                          FILE_SEQUENTIAL_ONLY |
                          FILE_SYNCHRONOUS_IO_NONALERT,
                          NULL,
                          0);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtCreateFile failed: %x, %wZ\n", Status, &FileName);
        goto unmapsrcsec;
    }

    RegionSize = (ULONG)PAGE_ROUND_UP(FileStandard.EndOfFile.u.LowPart);
    IoStatusBlock.Status = 0;
    ByteOffset.QuadPart = 0ULL;
    Status = NtWriteFile(FileHandleDest,
                         NULL,
                         NULL,
                         NULL,
                         &IoStatusBlock,
                         SourceFileMap,
                         RegionSize,
                         &ByteOffset,
                         NULL);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtWriteFile failed: %x:%x, iosb: %p src: %p, size: %x\n", Status, IoStatusBlock.Status, &IoStatusBlock, SourceFileMap, RegionSize);
        goto closedest;
    }

    /* Copy file date/time from source file */
    Status = NtSetInformationFile(FileHandleDest,
                                  &IoStatusBlock,
                                  &FileBasic,
                                  sizeof(FILE_BASIC_INFORMATION),
                                  FileBasicInformation);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtSetInformationFile failed: %x\n", Status);
        goto closedest;
    }

    /* shorten the file back to it's real size after completing the write */
    Status = NtSetInformationFile(FileHandleDest,
                                  &IoStatusBlock,
                                  &FileStandard.EndOfFile,
                                  sizeof(FILE_END_OF_FILE_INFORMATION),
                                  FileEndOfFileInformation);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtSetInformationFile failed: %x\n", Status);
    }

closedest:
    NtClose(FileHandleDest);

unmapsrcsec:
    NtUnmapViewOfSection(NtCurrentProcess(), SourceFileMap);

closesrcsec:
    NtClose(SourceFileSection);

closesrc:
    NtClose(FileHandleSource);

done:
    return Status;
}

#ifdef __REACTOS__
NTSTATUS
SetupExtractFile(
    PWCHAR CabinetFileName,
    PWCHAR SourceFileName,
    PWCHAR DestinationPathName)
{
    ULONG CabStatus;

    DPRINT("SetupExtractFile(CabinetFileName %S, SourceFileName %S, DestinationPathName %S)\n",
           CabinetFileName, SourceFileName, DestinationPathName);

    if (HasCurrentCabinet)
    {
        DPRINT("CurrentCabinetName: %S\n", CurrentCabinetName);
    }

    if ((HasCurrentCabinet) && (wcscmp(CabinetFileName, CurrentCabinetName) == 0))
    {
        DPRINT("Using same cabinet as last time\n");

        /* Use our last location because the files should be sequential */
        CabStatus = CabinetFindNextFileSequential(SourceFileName, &Search);
        if (CabStatus != CAB_STATUS_SUCCESS)
        {
            DPRINT("Sequential miss on file: %S\n", SourceFileName);

            /* Looks like we got unlucky */
            CabStatus = CabinetFindFirst(SourceFileName, &Search);
        }
    }
    else
    {
        DPRINT("Using new cabinet\n");

        if (HasCurrentCabinet)
        {
            CabinetCleanup();
        }

        wcscpy(CurrentCabinetName, CabinetFileName);

        CabinetInitialize();
        CabinetSetEventHandlers(NULL, NULL, NULL);
        CabinetSetCabinetName(CabinetFileName);

        CabStatus = CabinetOpen();
        if (CabStatus == CAB_STATUS_SUCCESS)
        {
            DPRINT("Opened cabinet %S\n", CabinetGetCabinetName());
            HasCurrentCabinet = TRUE;
        }
        else
        {
            DPRINT("Cannot open cabinet (%d)\n", CabStatus);
            return STATUS_UNSUCCESSFUL;
        }

        /* We have to start at the beginning here */
        CabStatus = CabinetFindFirst(SourceFileName, &Search);
    }

    if (CabStatus != CAB_STATUS_SUCCESS)
    {
        DPRINT1("Unable to find '%S' in cabinet '%S'\n", SourceFileName, CabinetGetCabinetName());
        return STATUS_UNSUCCESSFUL;
    }

    CabinetSetDestinationPath(DestinationPathName);
    CabStatus = CabinetExtractFile(&Search);
    if (CabStatus != CAB_STATUS_SUCCESS)
    {
        DPRINT("Cannot extract file %S (%d)\n", SourceFileName, CabStatus);
        return STATUS_UNSUCCESSFUL;
    }

    return STATUS_SUCCESS;
}
#endif


BOOLEAN
IsValidPath(
    IN PWCHAR InstallDir,
    IN ULONG Length)
{
    UINT i;

    // TODO: Add check for 8.3 too.

    /* Check for whitespaces */
    for (i = 0; i < Length; i++)
    {
        if (iswspace(InstallDir[i]))
            return FALSE;
    }

    return TRUE;
}

/* EOF */
