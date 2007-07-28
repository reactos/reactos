/* PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            base/system/autochk/autochk.c
 * PURPOSE:         Filesystem checker
 * PROGRAMMERS:     Aleksey Bragin
 *                  Eric Kohl
 *                  Hervé Poussineau
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
GetFileSystem(
    IN LPCWSTR Drive,
    IN OUT LPWSTR FileSystemName,
    IN SIZE_T FileSystemNameSize)
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
            CopyMemory(FileSystemName,
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
static BOOLEAN NTAPI
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

    case FSNOTSUPPORTED:
        DPRINT("FSNOTSUPPORTED\r");
        break;

    case VOLUMEINUSE:
        DPRINT("VOLUMEINUSE\r");
        break;

    case STRUCTUREPROGRESS:
        DPRINT("STRUCTUREPROGRESS\r");
        break;

    case DONEWITHSTRUCTURE:
        DPRINT("DONEWITHSTRUCTURE\r");
        break;

    case CLUSTERSIZETOOSMALL:
        DPRINT("CLUSTERSIZETOOSMALL\r");
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

/* Load the provider associated with this file system */
static PVOID
LoadProvider(
    IN PWCHAR FileSystem)
{
    UNICODE_STRING ProviderDll = RTL_CONSTANT_STRING(L"ufat.dll");
    PVOID BaseAddress;
    NTSTATUS Status;

    /* FIXME: add more providers here */

    if (wcscmp(FileSystem, L"FAT") != 0
     && wcscmp(FileSystem, L"FAT32") != 0)
    {
        return NULL;
    }

    Status = LdrLoadDll(NULL, NULL, &ProviderDll, &BaseAddress);
    if (!NT_SUCCESS(Status))
        return NULL;
    return BaseAddress;
}

static NTSTATUS
CheckVolume(
    IN PWCHAR DrivePath)
{
    WCHAR FileSystem[128];
    ANSI_STRING ChkdskFunctionName = RTL_CONSTANT_STRING("ChkdskEx");
    PVOID Provider;
    CHKDSKEX ChkdskFunc;
    UNICODE_STRING DrivePathU;
    NTSTATUS Status;

    /* Get the file system */
    Status = GetFileSystem(DrivePath,
                           FileSystem,
                           sizeof(FileSystem) / sizeof(FileSystem[0]));
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("GetFileSystem() failed with status 0x%08lx\n", Status);
        PrintString("  Unable to get file system of %S\n", DrivePath);
        return Status;
    }

    /* Load the provider which will do the chkdsk */
    Provider = LoadProvider(FileSystem);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("LoadProvider() failed\n");
        PrintString("  Unable to verify a %S volume\n", FileSystem);
        return STATUS_DLL_NOT_FOUND;
    }

    /* Get the Chkdsk function address */
    Status = LdrGetProcedureAddress(Provider,
                                    &ChkdskFunctionName,
                                    0,
                                    (PVOID*)&ChkdskFunc);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("LdrGetProcedureAddress() failed with status 0x%08lx\n", Status);
        PrintString("  Unable to verify a %S volume\n", FileSystem);
        LdrUnloadDll(Provider);
        return Status;
    }

    /* Call provider */
    //PrintString("  Verifying volume %S\n", DrivePath);
    RtlInitUnicodeString(&DrivePathU, DrivePath);
    Status = ChkdskFunc(&DrivePathU,
                        TRUE, // FixErrors
                        TRUE, // Verbose
                        FALSE, // CheckOnlyIfDirty
                        FALSE,// ScanDrive
                        ChkdskCallback);

    LdrUnloadDll(Provider);
    return Status;
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
    WCHAR DrivePath[128];

    // Win2003 passes the only param - "*". Probably means to check all drives
    /*
    DPRINT("Got %d params\n", argc);
    for (i=0; i<argc; i++)
        DPRINT("Param %d: %s\n", i, argv[i]);
    */

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
        DPRINT1("NtQueryInformationProcess() failed with status 0x%08lx\n",
            Status);
        return 1;
    }

    for (i = 0; i < 26; i++)
    {
        if ((DeviceMap.Query.DriveMap & (1 << i))
         && (DeviceMap.Query.DriveType[i] == DOSDEVICE_DRIVE_FIXED))
        {
            swprintf(DrivePath, L"%c:\\", L'A'+i);
            CheckVolume(DrivePath);
        }
    }
    //PrintString("  Done\n\n");
    return 0;
}

/* EOF */
