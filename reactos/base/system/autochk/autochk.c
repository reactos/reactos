/* $Id$
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            base/system/autochk/autochk.c
 * PURPOSE:         Filesystem checker
 * PROGRAMMERS:     Aleksey Bragin
 *                  Eric Kohl
 */

/* INCLUDES *****************************************************************/

//#define NDEBUG
#include <debug.h>

#include <stdio.h>
#define WIN32_NO_STATUS
#include <windows.h>
#define NTOS_MODE_USER
#include <ndk/ntndk.h>
#include <fmifs/fmifs.h>

/* DEFINES ******************************************************************/

#define FS_ATTRIBUTE_BUFFER_SIZE (MAX_PATH * sizeof(WCHAR) + sizeof(FILE_FS_ATTRIBUTE_INFORMATION))


/* FUNCTIONS ****************************************************************/
//
// FMIFS function
//
typedef
VOID
(STDCALL *PCHKDSK)(PWCHAR           DriveRoot,
                   PWCHAR           Format,
                   BOOLEAN          CorrectErrors,
                   BOOLEAN          Verbose,
                   BOOLEAN          CheckOnlyIfDirty,
                   BOOLEAN          ScanDrive,
                   PVOID            Unused2,
                   PVOID            Unused3,
                   PFMIFSCALLBACK   Callback);

PCHKDSK     ChkdskFunc = NULL;

void
DisplayString(LPCWSTR lpwString)
{
    UNICODE_STRING us;

    RtlInitUnicodeString(&us, lpwString);
    NtDisplayString(&us);
}

void
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
HANDLE
OpenDirectory(LPCWSTR DirName,
              BOOLEAN Write)
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
        Write ? FILE_WRITE_ATTRIBUTES : FILE_READ_ATTRIBUTES,
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

NTSTATUS
GetFileSystem(LPCWSTR Drive,
              LPWSTR FileSystemName,
              ULONG FileSystemNameSize)
{
    HANDLE FileHandle;
    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatusBlock;
    PFILE_FS_ATTRIBUTE_INFORMATION FileFsAttribute;
    UCHAR Buffer[FS_ATTRIBUTE_BUFFER_SIZE];

    FileFsAttribute = (PFILE_FS_ATTRIBUTE_INFORMATION)Buffer;

    FileHandle = OpenDirectory(Drive, FALSE);
    if (FileHandle == INVALID_HANDLE_VALUE)
        return STATUS_INVALID_PARAMETER;

    Status = NtQueryVolumeInformationFile(FileHandle,
                                          &IoStatusBlock,
                                          FileFsAttribute,
                                          FS_ATTRIBUTE_BUFFER_SIZE,
                                          FileFsAttributeInformation);
    NtClose(FileHandle);

    if (NT_SUCCESS(Status))
    {
        if (FileSystemNameSize * sizeof(WCHAR) >= FileFsAttribute->FileSystemNameLength + sizeof(WCHAR))
        {
            memcpy(FileSystemName,
                   FileFsAttribute->FileSystemName,
                   FileFsAttribute->FileSystemNameLength);
            FileSystemName[FileFsAttribute->FileSystemNameLength / sizeof(WCHAR)] = 0;
        }
        else
            return STATUS_BUFFER_TOO_SMALL;
    }
    else
        return Status;

    return STATUS_SUCCESS;
}

// This is based on SysInternal's ChkDsk app
BOOLEAN
STDCALL
ChkdskCallback(CALLBACKCOMMAND Command,
               DWORD Modifier,
               PVOID Argument)
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
        DPRINT("UNKNOWN2\r");
        break;

    case UNKNOWN3:
        DPRINT("UNKNOWN3\r");
        break;

    case UNKNOWN4:
        DPRINT("UNKNOWN4\r");
        break;

    case UNKNOWN5:
        DPRINT("UNKNOWN5\r");
        break;

    case UNKNOWN7:
        DPRINT("UNKNOWN7\r");
        break;

    case UNKNOWN8:
        DPRINT("UNKNOWN8\r");
        break;

    case UNKNOWN9:
        DPRINT("UNKNOWN9\r");
        break;

    case UNKNOWNA:
        DPRINT("UNKNOWNA\r");
        break;

    case UNKNOWNC:
        DPRINT("UNKNOWNC\r");
        break;

    case UNKNOWND:
        DPRINT("UNKNOWND\r");
        break;

    case INSUFFICIENTRIGHTS:
        DPRINT("INSUFFICIENTRIGHTS\r");
        break;

    case STRUCTUREPROGRESS:
        DPRINT("STRUCTUREPROGRESS\r");
        break;

    case DONEWITHSTRUCTURE:
        DPRINT("DONEWITHSTRUCTURE\r");
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
        if (*Status == TRUE)
        {
            PrintString("Autochk was unable to complete successfully.\n\n");
            //Error = TRUE;
        }
        break;
    }
    return TRUE;
}


/* Native image's entry point */
int
_cdecl
_main(int argc,
      char *argv[],
      char *envp[],
      int DebugFlag)
{
    PROCESS_DEVICEMAP_INFORMATION DeviceMap;
    ULONG i;
    NTSTATUS Status;
    WCHAR FileSystem[128];
    WCHAR DrivePath[128];

    PrintString("Autochk 0.0.2\n");

    // Win2003 passes the only param - "*". Probably means to check all drives
    /*
    DPRINT("Got %d params\n", argc);
    for (i=0; i<argc; i++)
        DPRINT("Param %d: %s\n", i, argv[i]);
    */

    Status = NtQueryInformationProcess(NtCurrentProcess(),
                                       ProcessDeviceMap,
                                       &DeviceMap.Query,
                                       sizeof(DeviceMap.Query),
                                       NULL);

    if(NT_SUCCESS(Status))
    {
        for (i = 0; i < 26; i++)
        {
            if ((DeviceMap.Query.DriveMap & (1 << i)) &&
                (DeviceMap.Query.DriveType[i] == DOSDEVICE_DRIVE_FIXED))
            {
                swprintf(DrivePath, L"%c:\\", 'A'+i);
                Status = GetFileSystem(DrivePath,
                                       FileSystem,
                                       sizeof(FileSystem));
                PrintString("  Checking drive %c: \n", 'A'+i);

                if (!NT_SUCCESS(Status))
                {
                    DPRINT1("Error getting FS information, Status=0x%08X\n",
                        Status);
                    // skip to the next volume
                    continue;
                }

                // FS type known, show it to user and then call chkdsk routine
                PrintString("   Filesystem type ");
                DisplayString(FileSystem);
                PrintString("\n");

                /*ChkdskFunc(DrivePath,
                       FileSystem,
                       TRUE, // FixErrors
                       TRUE, // Verbose
                       FALSE, // SkipClean
                       FALSE,// ScanSectors
                       NULL,
                       NULL,
                       ChkdskCallback);*/

                PrintString("      OK\n");
            }
        }
        PrintString("\n");
        return 0;
    }
    else
    {
        DPRINT1("NtQueryInformationProcess() failed with status=0x%08X\n",
            Status);
    }

    return 1;
}

/* EOF */
