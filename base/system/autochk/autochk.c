/*
 * PROJECT:     ReactOS AutoChk
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     FileSystem checker in Native mode.
 * COPYRIGHT:   Copyright 2002-2018 Eric Kohl
 *              Copyright 2006-2018 Aleksey Bragin
 *              Copyright 2006-2018 Hervé Poussineau
 *              Copyright 2008-2018 Pierre Schweitzer
 *              Copyright 2018 Hermes Belusca-Maito
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

HANDLE KeyboardHandle = NULL;

/* FUNCTIONS ****************************************************************/

//
// FMIFS function
//

static INT
PrintString(char* fmt,...)
{
    INT Len;
    char buffer[512];
    va_list ap;
    UNICODE_STRING UnicodeString;
    ANSI_STRING AnsiString;

    va_start(ap, fmt);
    Len = vsprintf(buffer, fmt, ap);
    va_end(ap);

    RtlInitAnsiString(&AnsiString, buffer);
    RtlAnsiStringToUnicodeString(&UnicodeString,
                                 &AnsiString,
                                 TRUE);
    NtDisplayString(&UnicodeString);
    RtlFreeUnicodeString(&UnicodeString);

    return Len;
}

static VOID
EraseLine(
    IN INT LineLength)
{
    INT Len;
    UNICODE_STRING UnicodeString;
    WCHAR buffer[512];

    if (LineLength <= 0)
        return;

    /* Go to the beginning of the line */
    RtlInitUnicodeString(&UnicodeString, L"\r");
    NtDisplayString(&UnicodeString);

    /* Fill the buffer chunk with spaces */
    Len = min(LineLength, ARRAYSIZE(buffer));
    while (Len > 0) buffer[--Len] = L' ';

    RtlInitEmptyUnicodeString(&UnicodeString, buffer, sizeof(buffer));
    while (LineLength > 0)
    {
        /* Display the buffer */
        Len = min(LineLength, ARRAYSIZE(buffer));
        LineLength -= Len;
        UnicodeString.Length = Len * sizeof(WCHAR);
        NtDisplayString(&UnicodeString);
    }

    /* Go to the beginning of the line */
    RtlInitUnicodeString(&UnicodeString, L"\r");
    NtDisplayString(&UnicodeString);
}

static NTSTATUS
OpenKeyboard(
    OUT PHANDLE KeyboardHandle)
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
    return NtOpenFile(KeyboardHandle,
                      FILE_ALL_ACCESS,
                      &ObjectAttributes,
                      &IoStatusBlock,
                      FILE_OPEN,
                      0);
}

static NTSTATUS
WaitForKeyboard(
    IN HANDLE KeyboardHandle,
    IN LONG TimeOut)
{
    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatusBlock;
    LARGE_INTEGER Offset, Timeout;
    KEYBOARD_INPUT_DATA InputData;
    INT Len = 0;

    /* Skip the wait if there is no timeout */
    if (TimeOut <= 0)
        return STATUS_TIMEOUT;

    /* Attempt to read a down key-press from the keyboard */
    do
    {
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
        if (!NT_SUCCESS(Status))
        {
            /* Something failed, bail out */
            return Status;
        }
        if ((Status != STATUS_PENDING) && !(InputData.Flags & KEY_BREAK))
        {
            /* We have a down key-press, return */
            return Status;
        }
    } while (Status != STATUS_PENDING);

    /* Perform the countdown of TimeOut seconds */
    Status = STATUS_TIMEOUT;
    while (TimeOut > 0)
    {
        /*
         * Display the countdown string.
         * Note that we only do a carriage return to go back to the
         * beginning of the line to possibly erase it. Note also that
         * we display a trailing space to erase any last character
         * when the string length decreases.
         */
        Len = PrintString("Press any key within %d second(s) to cancel and resume startup. \r", TimeOut);

        /* Decrease the countdown */
        --TimeOut;

        /* Wait one second for a key press */
        Timeout.QuadPart = -10000000;
        Status = NtWaitForSingleObject(KeyboardHandle, FALSE, &Timeout);
        if (Status != STATUS_TIMEOUT)
            break;
    }

    /* Erase the countdown string */
    EraseLine(Len);

    if (Status == STATUS_TIMEOUT)
    {
        /* The user did not press any key, cancel the read */
        NtCancelIoFile(KeyboardHandle, &IoStatusBlock);
    }
    else
    {
        /* Otherwise, return some status */
        Status = IoStatusBlock.Status;
    }

    return Status;
}

static NTSTATUS
GetFileSystem(
    IN PUNICODE_STRING VolumePathU,
    IN OUT PWSTR FileSystemName,
    IN SIZE_T FileSystemNameSize)
{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE FileHandle;
    IO_STATUS_BLOCK IoStatusBlock;
    PFILE_FS_ATTRIBUTE_INFORMATION FileFsAttribute;
    UCHAR Buffer[sizeof(FILE_FS_ATTRIBUTE_INFORMATION) + MAX_PATH * sizeof(WCHAR)];

    FileFsAttribute = (PFILE_FS_ATTRIBUTE_INFORMATION)Buffer;

    InitializeObjectAttributes(&ObjectAttributes,
                               VolumePathU,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    Status = NtCreateFile(&FileHandle,
                          FILE_GENERIC_READ,
                          &ObjectAttributes,
                          &IoStatusBlock,
                          NULL,
                          0,
                          FILE_SHARE_READ | FILE_SHARE_WRITE,
                          FILE_OPEN,
                          0,
                          NULL,
                          0);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Could not open volume '%wZ' to obtain its file system, Status 0x%08lx\n",
                VolumePathU, Status);
        return Status;
    }

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

    if (FileSystemNameSize >= FileFsAttribute->FileSystemNameLength + sizeof(WCHAR))
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
    IN PCWSTR VolumePath,
    IN LONG TimeOut,
    IN BOOLEAN CheckOnlyIfDirty)
{
    NTSTATUS Status;
    PCWSTR DisplayName;
    UNICODE_STRING VolumePathU;
    ULONG Count;
    WCHAR FileSystem[128];

    RtlInitUnicodeString(&VolumePathU, VolumePath);

    /* Get a drive string for display purposes only */
    if (wcslen(VolumePath) == 6 &&
        VolumePath[0] == L'\\'  &&
        VolumePath[1] == L'?'   &&
        VolumePath[2] == L'?'   &&
        VolumePath[3] == L'\\'  &&
        VolumePath[5] == L':')
    {
        /* DOS drive */
        DisplayName = &VolumePath[4];
    }
    else
    {
        DisplayName = VolumePath;
    }

    DPRINT1("AUTOCHK: Checking %wZ\n", &VolumePathU);
    PrintString("Verifying the file system on %S\r\n", DisplayName);

    /* Get the file system */
    Status = GetFileSystem(&VolumePathU,
                           FileSystem,
                           sizeof(FileSystem));
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("GetFileSystem() failed, Status 0x%08lx\n", Status);
        PrintString("  Unable to detect the file system of volume %S\r\n", DisplayName);
        goto Quit;
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
        DPRINT1("File system %S not supported\n", FileSystem);
        PrintString("  Unable to verify the volume. The %S file system is not supported.\r\n", FileSystem);
        Status = STATUS_DLL_NOT_FOUND;
        goto Quit;
    }

    /* Check whether the volume is dirty */
    Status = FileSystems[Count].ChkdskFunc(&VolumePathU,
                                           FALSE, // FixErrors
                                           TRUE,  // Verbose
                                           TRUE,  // CheckOnlyIfDirty
                                           FALSE, // ScanDrive
                                           ChkdskCallback);

    /* Perform the check either when the volume is dirty or a check is forced */
    if ((Status == STATUS_DISK_CORRUPT_ERROR) || !CheckOnlyIfDirty)
    {
        /* Let the user decide whether to repair */
        if (Status == STATUS_DISK_CORRUPT_ERROR)
        {
            PrintString("The file system on volume %S needs to be checked for problems.\r\n", DisplayName);
            PrintString("You may cancel this check, but it is recommended that you continue.\r\n");
        }
        else
        {
            PrintString("A volume check has been scheduled.\r\n");
        }

        if (!KeyboardHandle || WaitForKeyboard(KeyboardHandle, TimeOut) == STATUS_TIMEOUT)
        {
            PrintString("The system will now check the file system.\r\n\r\n");
            Status = FileSystems[Count].ChkdskFunc(&VolumePathU,
                                                   TRUE,  // FixErrors
                                                   TRUE,  // Verbose
                                                   CheckOnlyIfDirty,
                                                   FALSE, // ScanDrive
                                                   ChkdskCallback);
        }
        else
        {
            PrintString("The file system check has been skipped.\r\n");
        }
    }

Quit:
    PrintString("\r\n\r\n");
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
    NTSTATUS Status;
    LONG TimeOut;
    ULONG i;
    BOOLEAN CheckAllVolumes = FALSE;
    BOOLEAN CheckOnlyIfDirty = TRUE;
    PROCESS_DEVICEMAP_INFORMATION DeviceMap;
    PCHAR SkipDrives = NULL;
    ANSI_STRING VolumePathA;
    UNICODE_STRING VolumePathU;
    WCHAR VolumePath[128] = L"";

    /*
     * Parse the command-line: optional command switches,
     * then the NT volume name (or drive letter) to be analysed.
     * If "*" is passed this means that we check all the volumes.
     */
    if (argc <= 1)
    {
        /* Only one parameter (the program name), bail out */
        return 1;
    }
    for (i = 1; i < argc; ++i)
    {
        if (argv[i][0] == '/' || argv[i][0] == '-')
        {
            DPRINT("Parameter %d: %s\n", i, argv[i]);
            switch (toupper(argv[i][1]))
            {
                case 'K': /* List of drive letters to skip */
                {
                    /* Check for the separator */
                    if (argv[i][2] != ':')
                        goto Default;

                    SkipDrives = &argv[i][3];
                    break;
                }

                case 'R': /* Repair the errors, implies /P */
                case 'P': /* Check even if not dirty */
                {
                    if (argv[i][2] != ANSI_NULL)
                        goto Default;

                    CheckOnlyIfDirty = FALSE;
                    break;
                }

                default: Default:
                    DPRINT1("Unknown switch %d: %s\n", i, argv[i]);
                    break;
            }
        }
        else
        {
            DPRINT("Parameter %d - Volume specification: %s\n", i, argv[i]);
            if (strcmp(argv[i], "*") == 0)
            {
                CheckAllVolumes = TRUE;
            }
            else
            {
                RtlInitEmptyUnicodeString(&VolumePathU,
                                          VolumePath,
                                          sizeof(VolumePath));
                RtlInitAnsiString(&VolumePathA, argv[i]);
                Status = RtlAnsiStringToUnicodeString(&VolumePathU,
                                                      &VolumePathA,
                                                      FALSE);
            }

            /* Stop the parsing now */
            break;
        }
    }

    /*
     * FIXME: We should probably use here the mount manager to be
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

    /* Filter out the skipped drives from the map */
    if (SkipDrives && *SkipDrives)
    {
        DPRINT1("Skipping drives:");
        while (*SkipDrives)
        {
#if DBG
            DbgPrint(" %c:", *SkipDrives);
#endif
            /* Retrieve the index and filter the drive out */
            i = toupper(*SkipDrives) - 'A';
            if (0 <= i && i <= 'Z'-'A')
                DeviceMap.Query.DriveMap &= ~(1 << i);

            /* Go to the next drive letter */
            ++SkipDrives;
        }
        DbgPrint("\n");
    }

    /* Query the timeout */
    TimeOut = 3;
    QueryTimeout(&TimeOut);

    /* Open the keyboard */
    Status = OpenKeyboard(&KeyboardHandle);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("OpenKeyboard() failed, Status 0x%08lx\n", Status);
        /* Ignore keyboard interaction */
    }

    if (CheckAllVolumes)
    {
        /* Enumerate and check each of the available volumes */
        for (i = 0; i <= 'Z'-'A'; i++)
        {
            if ((DeviceMap.Query.DriveMap & (1 << i)) &&
                (DeviceMap.Query.DriveType[i] == DOSDEVICE_DRIVE_FIXED))
            {
                swprintf(VolumePath, L"\\??\\%c:", L'A' + i);
                CheckVolume(VolumePath, TimeOut, CheckOnlyIfDirty);
            }
        }
    }
    else
    {
        /* Retrieve our index and analyse the volume */
        if (wcslen(VolumePath) == 6 &&
            VolumePath[0] == L'\\'  &&
            VolumePath[1] == L'?'   &&
            VolumePath[2] == L'?'   &&
            VolumePath[3] == L'\\'  &&
            VolumePath[5] == L':')
        {
            i = toupper(VolumePath[4]) - 'A';
            if ((DeviceMap.Query.DriveMap & (1 << i)) &&
                (DeviceMap.Query.DriveType[i] == DOSDEVICE_DRIVE_FIXED))
            {
                CheckVolume(VolumePath, TimeOut, CheckOnlyIfDirty);
            }
        }
        else
        {
            /* Just perform the check on the specified volume */
            // TODO: Check volume type using NtQueryVolumeInformationFile(FileFsDeviceInformation)
            CheckVolume(VolumePath, TimeOut, CheckOnlyIfDirty);
        }
    }

    /* Close the keyboard */
    if (KeyboardHandle)
        NtClose(KeyboardHandle);

    // PrintString("Done\r\n\r\n");
    return 0;
}

/* EOF */
