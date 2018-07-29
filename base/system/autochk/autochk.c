/*
 * PROJECT:     ReactOS AutoChk
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     FileSystem checker in Native mode.
 * COPYRIGHT:   Copyright 2002-2018 Eric Kohl
 *              Copyright 2006-2018 Aleksey Bragin
 *              Copyright 2006-2018 Hervé Poussineau
 *              Copyright 2008-2018 Pierre Schweitzer
 */

/* INCLUDES *****************************************************************/

#include <stdio.h>

#define WIN32_NO_STATUS
#include <windef.h>
#include <winbase.h>
#include <ntddkbd.h>

#define NTOS_MODE_USER
#include <ndk/exfuncs.h>
#include <ndk/iofuncs.h>
#include <ndk/obfuncs.h>
#include <ndk/psfuncs.h>
#include <ndk/rtlfuncs.h>
#include <fmifs/fmifs.h>

#include <fslib/vfatlib.h>
#include <fslib/ntfslib.h>
#include <fslib/ext2lib.h>
#include <fslib/btrfslib.h>
#include <fslib/reiserfslib.h>
#include <fslib/ffslib.h>
#include <fslib/cdfslib.h>

#define NDEBUG
#include <debug.h>

/* DEFINES ******************************************************************/

typedef struct _FILESYSTEM_CHKDSK
{
    WCHAR Name[10];
    CHKDSKEX ChkdskFunc;
} FILESYSTEM_CHKDSK, *PFILESYSTEM_CHKDSK;

FILESYSTEM_CHKDSK FileSystems[10] =
{
    { L"FAT", VfatChkdsk },
    { L"FAT32", VfatChkdsk },
    { L"NTFS", NtfsChkdsk },
    { L"EXT2", Ext2Chkdsk },
    { L"EXT3", Ext2Chkdsk },
    { L"EXT4", Ext2Chkdsk },
    { L"Btrfs", BtrfsChkdskEx },
    { L"RFSD", ReiserfsChkdsk },
    { L"FFS", FfsChkdsk },
    { L"CDFS", CdfsChkdsk },
};

HANDLE KeyboardHandle;

/* FUNCTIONS ****************************************************************/

//
// FMIFS function
//

static VOID
PrintString(char* fmt,...)
{
    char buffer[512];
    va_list ap;
    UNICODE_STRING UnicodeString;
    ANSI_STRING AnsiString;

    va_start(ap, fmt);
    vsprintf(buffer, fmt, ap);
    va_end(ap);

    RtlInitAnsiString(&AnsiString, buffer);
    RtlAnsiStringToUnicodeString(&UnicodeString,
                                 &AnsiString,
                                 TRUE);
    NtDisplayString(&UnicodeString);
    RtlFreeUnicodeString(&UnicodeString);
}

// this func is taken from kernel32/file/volume.c
static HANDLE
OpenDirectory(
    IN LPCWSTR DirName,
    IN BOOLEAN Write)
{
    UNICODE_STRING NtPathU;
    OBJECT_ATTRIBUTES ObjectAttributes;
    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatusBlock;
    HANDLE hFile;

    if (!RtlDosPathNameToNtPathName_U(DirName,
                                      &NtPathU,
                                      NULL,
                                      NULL))
    {
        DPRINT1("Invalid path!\n");
        return INVALID_HANDLE_VALUE;
    }

    InitializeObjectAttributes(
        &ObjectAttributes,
        &NtPathU,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL);

    Status = NtCreateFile(
        &hFile,
        Write ? FILE_GENERIC_WRITE : FILE_GENERIC_READ,
        &ObjectAttributes,
        &IoStatusBlock,
        NULL,
        0,
        FILE_SHARE_READ|FILE_SHARE_WRITE,
        FILE_OPEN,
        0,
        NULL,
        0);

    RtlFreeHeap(RtlGetProcessHeap(), 0, NtPathU.Buffer);

    if (!NT_SUCCESS(Status))
    {
        return INVALID_HANDLE_VALUE;
    }

    return hFile;
}

static NTSTATUS
OpenKeyboard(VOID)
{
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatusBlock;
    UNICODE_STRING KeyboardName = RTL_CONSTANT_STRING(L"\\Device\\KeyboardClass0");

    /* Just open the class driver */
    InitializeObjectAttributes(&ObjectAttributes,
                               &KeyboardName,
                               0,
                               NULL,
                               NULL);
    return NtOpenFile(&KeyboardHandle,
                      FILE_ALL_ACCESS,
                      &ObjectAttributes,
                      &IoStatusBlock,
                      FILE_OPEN,
                      0);
}

static NTSTATUS
WaitForKeyboard(
    IN LONG TimeOut)
{
    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatusBlock;
    LARGE_INTEGER Offset, Timeout;
    KEYBOARD_INPUT_DATA InputData;

    /* Attempt to read from the keyboard */
    Offset.QuadPart = 0;
    Status = NtReadFile(KeyboardHandle,
                        NULL,
                        NULL,
                        NULL,
                        &IoStatusBlock,
                        &InputData,
                        sizeof(KEYBOARD_INPUT_DATA),
                        &Offset,
                        NULL);
    if (Status == STATUS_PENDING)
    {
        /* Wait TimeOut seconds */
        Timeout.QuadPart = TimeOut * -10000000;
        Status = NtWaitForSingleObject(KeyboardHandle, FALSE, &Timeout);
        /* The user didn't enter anything, cancel the read */
        if (Status == STATUS_TIMEOUT)
        {
            NtCancelIoFile(KeyboardHandle, &IoStatusBlock);
        }
        /* Else, return some status */
        else
        {
            Status = IoStatusBlock.Status;
        }
    }

    return Status;
}

static NTSTATUS
GetFileSystem(
    IN LPCWSTR Drive,
    IN OUT LPWSTR FileSystemName,
    IN SIZE_T FileSystemNameSize)
{
    HANDLE FileHandle;
    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatusBlock;
    PFILE_FS_ATTRIBUTE_INFORMATION FileFsAttribute;
    UCHAR Buffer[sizeof(FILE_FS_ATTRIBUTE_INFORMATION) + MAX_PATH * sizeof(WCHAR)];

    FileFsAttribute = (PFILE_FS_ATTRIBUTE_INFORMATION)Buffer;

    FileHandle = OpenDirectory(Drive, FALSE);
    if (FileHandle == INVALID_HANDLE_VALUE)
        return STATUS_INVALID_PARAMETER;

    Status = NtQueryVolumeInformationFile(FileHandle,
                                          &IoStatusBlock,
                                          FileFsAttribute,
                                          sizeof(Buffer),
                                          FileFsAttributeInformation);
    NtClose(FileHandle);

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtQueryVolumeInformationFile() failed, Status 0x%08lx\n", Status);
        return Status;
    }

    if (FileSystemNameSize * sizeof(WCHAR) >= FileFsAttribute->FileSystemNameLength + sizeof(WCHAR))
    {
        RtlCopyMemory(FileSystemName,
                      FileFsAttribute->FileSystemName,
                      FileFsAttribute->FileSystemNameLength);
        FileSystemName[FileFsAttribute->FileSystemNameLength / sizeof(WCHAR)] = UNICODE_NULL;
    }
    else
    {
        return STATUS_BUFFER_TOO_SMALL;
    }

    return STATUS_SUCCESS;
}

/* This is based on SysInternals' ChkDsk application */
static BOOLEAN
NTAPI
ChkdskCallback(
    IN CALLBACKCOMMAND Command,
    IN ULONG Modifier,
    IN PVOID Argument)
{
    PDWORD      Percent;
    PBOOLEAN    Status;
    PTEXTOUTPUT Output;

    //
    // We get other types of commands,
    // but we don't have to pay attention to them
    //
    switch(Command)
    {
    case UNKNOWN2:
        DPRINT("UNKNOWN2\n");
        break;

    case UNKNOWN3:
        DPRINT("UNKNOWN3\n");
        break;

    case UNKNOWN4:
        DPRINT("UNKNOWN4\n");
        break;

    case UNKNOWN5:
        DPRINT("UNKNOWN5\n");
        break;

    case UNKNOWN9:
        DPRINT("UNKNOWN9\n");
        break;

    case UNKNOWNA:
        DPRINT("UNKNOWNA\n");
        break;

    case UNKNOWNC:
        DPRINT("UNKNOWNC\n");
        break;

    case UNKNOWND:
        DPRINT("UNKNOWND\n");
        break;

    case INSUFFICIENTRIGHTS:
        DPRINT("INSUFFICIENTRIGHTS\n");
        break;

    case FSNOTSUPPORTED:
        DPRINT("FSNOTSUPPORTED\n");
        break;

    case VOLUMEINUSE:
        DPRINT("VOLUMEINUSE\n");
        break;

    case STRUCTUREPROGRESS:
        DPRINT("STRUCTUREPROGRESS\n");
        break;

    case DONEWITHSTRUCTURE:
        DPRINT("DONEWITHSTRUCTURE\n");
        break;

    case CLUSTERSIZETOOSMALL:
        DPRINT("CLUSTERSIZETOOSMALL\n");
        break;

    case PROGRESS:
        Percent = (PDWORD) Argument;
        PrintString("%d percent completed.\r", *Percent);
        break;

    case OUTPUT:
        Output = (PTEXTOUTPUT) Argument;
        PrintString("%s", Output->Output);
        break;

    case DONE:
        Status = (PBOOLEAN)Argument;
        if (*Status != FALSE)
        {
            PrintString("The file system check was unable to complete successfully.\r\n\r\n");
            // Error = TRUE;
        }
        break;
    }
    return TRUE;
}

static NTSTATUS
CheckVolume(
    IN PWCHAR DrivePath,
    IN LONG TimeOut)
{
    WCHAR FileSystem[128];
    WCHAR NtDrivePath[64];
    UNICODE_STRING DrivePathU;
    NTSTATUS Status;
    DWORD Count;

    swprintf(NtDrivePath, L"\\??\\");
    wcscat(NtDrivePath, DrivePath);
    NtDrivePath[wcslen(NtDrivePath)-1] = 0;
    RtlInitUnicodeString(&DrivePathU, NtDrivePath);

    DPRINT1("AUTOCHK: Checking %wZ\n", &DrivePathU);
    PrintString("  Checking file system on %S\r\n", DrivePath);

    /* Get the file system */
    Status = GetFileSystem(DrivePath,
                           FileSystem,
                           ARRAYSIZE(FileSystem));
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("GetFileSystem() failed, Status 0x%08lx\n", Status);
        PrintString("  Unable to detect file system of %S\r\n", DrivePath);
        return Status;
    }

    PrintString("  The file system type is %S.\r\n\r\n", FileSystem);

    /* Find a suitable file system provider */
    for (Count = 0; Count < RTL_NUMBER_OF(FileSystems); ++Count)
    {
        if (wcscmp(FileSystem, FileSystems[Count].Name) == 0)
            break;
    }
    if (Count >= RTL_NUMBER_OF(FileSystems))
    {
        DPRINT1("File system not supported\n");
        PrintString("  Unable to check the file system. %S is not supported.\r\n", FileSystem);
        return STATUS_DLL_NOT_FOUND;
    }

    /* First, check whether the volume is dirty */
    Status = FileSystems[Count].ChkdskFunc(&DrivePathU,
                                           FALSE, // FixErrors
                                           TRUE,  // Verbose
                                           TRUE,  // CheckOnlyIfDirty
                                           FALSE, // ScanDrive
                                           ChkdskCallback);
    /* It is */
    if (Status == STATUS_DISK_CORRUPT_ERROR)
    {
        NTSTATUS WaitStatus;

        /* Let the user decide whether to repair */
        PrintString("  The file system on this volume needs to be checked for problems.\r\n");
        PrintString("  You may cancel this check, but it's recommended that you continue.\r\n\r\n");
        PrintString("  Press any key within %d second(s) to cancel and resume startup.\r\n\r\n", TimeOut);

        /* Timeout == fix it! */
        WaitStatus = WaitForKeyboard(TimeOut);
        if (WaitStatus == STATUS_TIMEOUT)
        {
            PrintString("  The system will now check the file system.\r\n\r\n");
            Status = FileSystems[Count].ChkdskFunc(&DrivePathU,
                                                   TRUE,  // FixErrors
                                                   TRUE,  // Verbose
                                                   TRUE,  // CheckOnlyIfDirty
                                                   FALSE, // ScanDrive
                                                   ChkdskCallback);
        }
        else
        {
            PrintString("  File system check has been skipped.\r\n");
        }
    }

    return Status;
}

static VOID
QueryTimeout(
    IN OUT PLONG TimeOut)
{
    RTL_QUERY_REGISTRY_TABLE QueryTable[2];

    RtlZeroMemory(QueryTable, sizeof(QueryTable));
    QueryTable[0].Flags = RTL_QUERY_REGISTRY_DIRECT;
    QueryTable[0].Name = L"AutoChkTimeOut";
    QueryTable[0].EntryContext = TimeOut;

    RtlQueryRegistryValues(RTL_REGISTRY_CONTROL, L"Session Manager", QueryTable, NULL, NULL);
    /* See: https://docs.microsoft.com/en-us/windows-server/administration/windows-commands/autochk */
    *TimeOut = min(max(*TimeOut, 0), 259200);
}

INT
__cdecl
_main(
    IN INT argc,
    IN PCHAR argv[],
    IN PCHAR envp[],
    IN ULONG DebugFlag)
{
    PROCESS_DEVICEMAP_INFORMATION DeviceMap;
    ULONG i;
    NTSTATUS Status;
    WCHAR DrivePath[128];
    LONG TimeOut;

    // Win2003 passes the only param - "*". Probably means to check all drives
    /*
    DPRINT("Got %d params\n", argc);
    for (i=0; i<argc; i++)
        DPRINT("Param %d: %s\n", i, argv[i]);
    */

    /* Query timeout */
    TimeOut = 3;
    QueryTimeout(&TimeOut);

    /* FIXME: We should probably use here the mount manager to be
     * able to check volumes which don't have a drive letter.
     */
    Status = NtQueryInformationProcess(NtCurrentProcess(),
                                       ProcessDeviceMap,
                                       &DeviceMap.Query,
                                       sizeof(DeviceMap.Query),
                                       NULL);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtQueryInformationProcess() failed, Status 0x%08lx\n", Status);
        return 1;
    }

    /* Open keyboard */
    Status = OpenKeyboard();
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("OpenKeyboard() failed, Status 0x%08lx\n", Status);
        return 1;
    }

    for (i = 0; i < 26; i++)
    {
        if ((DeviceMap.Query.DriveMap & (1 << i))
         && (DeviceMap.Query.DriveType[i] == DOSDEVICE_DRIVE_FIXED))
        {
            swprintf(DrivePath, L"%c:\\", L'A'+i);
            CheckVolume(DrivePath, TimeOut);
        }
    }

    /* Close keyboard */
    NtClose(KeyboardHandle);

    // PrintString("Done\r\n\r\n");
    return 0;
}

/* EOF */
