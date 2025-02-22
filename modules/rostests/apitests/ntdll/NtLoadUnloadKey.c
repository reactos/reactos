/*
 * PROJECT:         ReactOS API Tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Test for NtLoadKey and NtUnloadKey
 * PROGRAMMERS:     Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

#include "precomp.h"

/* See xdk/cmtypes.h */
#define REG_CREATED_NEW_KEY     1
#define REG_OPENED_EXISTING_KEY 2

#define REG_FORCE_UNLOAD        1

#if 1

    #define NDEBUG
    #include <debug.h>

#else

    #define DPRINT(fmt, ...)  printf("(%s:%d) " fmt, __FILE__, __LINE__, ##__VA_ARGS__);
    #define DPRINT1(fmt, ...) printf("(%s:%d) " fmt, __FILE__, __LINE__, ##__VA_ARGS__);

#endif

static NTSTATUS (NTAPI *pNtUnloadKey2)(POBJECT_ATTRIBUTES, ULONG);

static BOOLEAN
RetrieveCurrentModuleNTDirectory(
    OUT PUNICODE_STRING NtPath)
{
    WCHAR ModulePath[MAX_PATH];
    PWSTR PathSep;

    /* Retrieve the current path where the test is running */
    GetModuleFileNameW(NULL, ModulePath, _countof(ModulePath));
    PathSep = wcsrchr(ModulePath, L'\\');
    if (!PathSep)
        PathSep = ModulePath + wcslen(ModulePath);
    *PathSep = UNICODE_NULL;

    /* Convert the path to NT format and work with it for now on */
    return RtlDosPathNameToNtPathName_U(ModulePath, NtPath, NULL, NULL);
}

static NTSTATUS
CreateRegKey(
    OUT PHANDLE KeyHandle,
    IN HANDLE RootKey OPTIONAL,
    IN PUNICODE_STRING KeyName,
    IN ULONG CreateOptions,
    OUT PULONG Disposition OPTIONAL)
{
    OBJECT_ATTRIBUTES ObjectAttributes;

    InitializeObjectAttributes(&ObjectAttributes,
                               KeyName,
                               OBJ_CASE_INSENSITIVE,
                               RootKey,
                               NULL);
    return NtCreateKey(KeyHandle,
                       KEY_ALL_ACCESS,
                       &ObjectAttributes,
                       0,
                       NULL,
                       CreateOptions,
                       Disposition);
}

static NTSTATUS
CreateProtoHive(
    OUT PHANDLE KeyHandle)
{
    NTSTATUS Status;
    UNICODE_STRING KeyName;

    RtlInitUnicodeString(&KeyName, L"\\Registry\\Machine\\SYSTEM\\$$$PROTO.HIV");
    Status = CreateRegKey(KeyHandle,
                          NULL,
                          &KeyName,
                          REG_OPTION_NON_VOLATILE,
                          NULL);
    if (!NT_SUCCESS(Status))
        return Status;

    NtFlushKey(KeyHandle);
    return Status;
}

static VOID
DestroyProtoHive(
    IN HANDLE KeyHandle)
{
    NtDeleteKey(KeyHandle);
    NtClose(KeyHandle);
}

static NTSTATUS
OpenDirectoryByHandleOrPath(
    OUT PHANDLE RootPathHandle,
    IN HANDLE RootDirectory OPTIONAL,
    IN PUNICODE_STRING RootPath OPTIONAL)
{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatusBlock;

    *RootPathHandle = NULL;

    /*
     * RootDirectory and RootPath cannot be either both NULL
     * or both non-NULL, when being specified.
     */
    if ((!RootDirectory && !RootPath) ||
        ( RootDirectory &&  RootPath))
    {
        return STATUS_INVALID_PARAMETER;
    }

    if (!RootDirectory && RootPath)
    {
        /* Open the root directory path */
        InitializeObjectAttributes(&ObjectAttributes,
                                   RootPath,
                                   OBJ_CASE_INSENSITIVE,
                                   NULL,
                                   NULL);
        Status = NtOpenFile(RootPathHandle,
                            // FILE_TRAVERSE is needed to be able to use the handle as RootDirectory for future InitializeObjectAttributes calls.
                            FILE_LIST_DIRECTORY | FILE_ADD_FILE /* | FILE_ADD_SUBDIRECTORY */ | FILE_TRAVERSE | SYNCHRONIZE,
                            &ObjectAttributes,
                            &IoStatusBlock,
                            FILE_SHARE_READ | FILE_SHARE_WRITE,
                            FILE_SYNCHRONOUS_IO_NONALERT | FILE_DIRECTORY_FILE /* | FILE_OPEN_FOR_BACKUP_INTENT */);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("NtOpenFile(%wZ) failed, Status 0x%08lx\n", RootPath, Status);
            return Status;
        }

        /* Mark the handle as being opened locally */
        *RootPathHandle = (HANDLE)((ULONG_PTR)*RootPathHandle | 1);
    }
    else if (RootDirectory && !RootPath)
    {
        *RootPathHandle = RootDirectory;
    }
    // No other cases possible

    return STATUS_SUCCESS;
}

/*
 * Should be called under privileges
 */
static NTSTATUS
CreateRegistryFile(
    IN HANDLE RootDirectory OPTIONAL,
    IN PUNICODE_STRING RootPath OPTIONAL,
    IN PCWSTR RegistryKey,
    IN HANDLE ProtoKeyHandle)
{
    NTSTATUS Status;
    HANDLE RootPathHandle, FileHandle;
    UNICODE_STRING FileName;
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatusBlock;

    /* Open the root directory */
    Status = OpenDirectoryByHandleOrPath(&RootPathHandle, RootDirectory, RootPath);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("OpenDirectoryByHandleOrPath failed, Status 0x%08lx\n", Status);
        return Status;
    }

    /* Create the file */
    RtlInitUnicodeString(&FileName, RegistryKey);
    InitializeObjectAttributes(&ObjectAttributes,
                               &FileName,
                               OBJ_CASE_INSENSITIVE,
                               (HANDLE)((ULONG_PTR)RootPathHandle & ~1), // Remove the opened-locally flag
                               NULL);
    Status = NtCreateFile(&FileHandle,
                          FILE_GENERIC_WRITE /* | DELETE */,
                          &ObjectAttributes,
                          &IoStatusBlock,
                          NULL,
                          FILE_ATTRIBUTE_NORMAL /* | FILE_FLAG_DELETE_ON_CLOSE */,
                          0,
                          FILE_OVERWRITE_IF,
                          FILE_SYNCHRONOUS_IO_NONALERT | FILE_NON_DIRECTORY_FILE,
                          NULL,
                          0);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtCreateFile(%wZ) failed, Status 0x%08lx\n", &FileName, Status);
        goto Cleanup;
    }

    /* Save the selected hive into the file */
    Status = NtSaveKeyEx(ProtoKeyHandle, FileHandle, REG_LATEST_FORMAT);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtSaveKeyEx(%wZ) failed, Status 0x%08lx\n", &FileName, Status);
    }

    /* Close the file, the root directory (if opened locally), and return */
    NtClose(FileHandle);
Cleanup:
    if ((ULONG_PTR)RootPathHandle & 1) NtClose((HANDLE)((ULONG_PTR)RootPathHandle & ~1));
    return Status;
}

/*
 * Should be called under privileges
 */
static NTSTATUS
MyDeleteFile(
    IN HANDLE RootDirectory OPTIONAL,
    IN PUNICODE_STRING RootPath OPTIONAL,
    IN PCWSTR FileName,
    IN BOOLEAN ForceDelete) // ForceDelete can be used to delete read-only files
{
    NTSTATUS Status;
    HANDLE RootPathHandle;
    UNICODE_STRING NtPath;
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatusBlock;
    HANDLE FileHandle;
    FILE_DISPOSITION_INFORMATION FileDispInfo;
    BOOLEAN RetryOnce = FALSE;

    /* Open the root directory */
    Status = OpenDirectoryByHandleOrPath(&RootPathHandle, RootDirectory, RootPath);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("OpenDirectoryByHandleOrPath failed, Status 0x%08lx\n", Status);
        return Status;
    }

    /* Open the directory name that was passed in */
    RtlInitUnicodeString(&NtPath, FileName);
    InitializeObjectAttributes(&ObjectAttributes,
                               &NtPath,
                               OBJ_CASE_INSENSITIVE,
                               RootPathHandle,
                               NULL);

Retry: /* We go back there once if RetryOnce == TRUE */
    Status = NtOpenFile(&FileHandle,
                        DELETE | FILE_READ_ATTRIBUTES |
                        (RetryOnce ? FILE_WRITE_ATTRIBUTES : 0),
                        &ObjectAttributes,
                        &IoStatusBlock,
                        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                        FILE_NON_DIRECTORY_FILE | FILE_OPEN_FOR_BACKUP_INTENT);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtOpenFile failed with Status 0x%08lx\n", Status);
        return Status;
    }

    if (RetryOnce)
    {
        FILE_BASIC_INFORMATION FileInformation;

        Status = NtQueryInformationFile(FileHandle,
                                        &IoStatusBlock,
                                        &FileInformation,
                                        sizeof(FILE_BASIC_INFORMATION),
                                        FileBasicInformation);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("NtQueryInformationFile failed with Status 0x%08lx\n", Status);
            NtClose(FileHandle);
            return Status;
        }

        FileInformation.FileAttributes = FILE_ATTRIBUTE_NORMAL;
        Status = NtSetInformationFile(FileHandle,
                                      &IoStatusBlock,
                                      &FileInformation,
                                      sizeof(FILE_BASIC_INFORMATION),
                                      FileBasicInformation);
        NtClose(FileHandle);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("NtSetInformationFile failed with Status 0x%08lx\n", Status);
            return Status;
        }
    }

    /* Ask for the file to be deleted */
    FileDispInfo.DeleteFile = TRUE;
    Status = NtSetInformationFile(FileHandle,
                                  &IoStatusBlock,
                                  &FileDispInfo,
                                  sizeof(FILE_DISPOSITION_INFORMATION),
                                  FileDispositionInformation);
    NtClose(FileHandle);

    if (!NT_SUCCESS(Status))
        DPRINT1("Deletion of file '%S' failed, Status 0x%08lx\n", FileName, Status);

    // FIXME: Check the precise value of Status!
    if (!NT_SUCCESS(Status) && ForceDelete && !RetryOnce)
    {
        /* Retry once */
        RetryOnce = TRUE;
        goto Retry;
    }

    /* Return result to the caller */
    return Status;
}

/*
 * Should be called under privileges
 */
static NTSTATUS
ConnectRegistry(
    IN HANDLE RootKey OPTIONAL,
    IN PCWSTR RegMountPoint,
    IN HANDLE RootDirectory OPTIONAL,
    IN PUNICODE_STRING RootPath OPTIONAL,
    IN PCWSTR RegistryKey)
{
    NTSTATUS Status;
    HANDLE RootPathHandle;
    UNICODE_STRING KeyName, FileName;
    OBJECT_ATTRIBUTES KeyObjectAttributes;
    OBJECT_ATTRIBUTES FileObjectAttributes;

    /* Open the root directory */
    Status = OpenDirectoryByHandleOrPath(&RootPathHandle, RootDirectory, RootPath);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("OpenDirectoryByHandleOrPath failed, Status 0x%08lx\n", Status);
        return Status;
    }

    RtlInitUnicodeString(&KeyName, RegMountPoint);
    InitializeObjectAttributes(&KeyObjectAttributes,
                               &KeyName,
                               OBJ_CASE_INSENSITIVE,
                               RootKey,
                               NULL);

    RtlInitUnicodeString(&FileName, RegistryKey);
    InitializeObjectAttributes(&FileObjectAttributes,
                               &FileName,
                               OBJ_CASE_INSENSITIVE,
                               (HANDLE)((ULONG_PTR)RootPathHandle & ~1), // Remove the opened-locally flag
                               NULL);

    /* Mount the registry hive in the registry namespace */
    Status = NtLoadKey(&KeyObjectAttributes, &FileObjectAttributes);

    /* Close the root directory (if opened locally), and return */
    if ((ULONG_PTR)RootPathHandle & 1) NtClose((HANDLE)((ULONG_PTR)RootPathHandle & ~1));
    return Status;
}

/*
 * Should be called under privileges
 */
static NTSTATUS
DisconnectRegistry(
    IN HANDLE RootKey OPTIONAL,
    IN PCWSTR RegMountPoint,
    IN ULONG Flags)
{
    UNICODE_STRING KeyName;
    OBJECT_ATTRIBUTES ObjectAttributes;

    RtlInitUnicodeString(&KeyName, RegMountPoint);
    InitializeObjectAttributes(&ObjectAttributes,
                               &KeyName,
                               OBJ_CASE_INSENSITIVE,
                               RootKey,
                               NULL);
    if (!pNtUnloadKey2)
    {
        win_skip("NtUnloadKey2 unavailable, using NtUnloadKey. Flags %lu\n", Flags);
        return NtUnloadKey(&ObjectAttributes);
    }
    return pNtUnloadKey2(&ObjectAttributes, Flags);
}


START_TEST(NtLoadUnloadKey)
{
    typedef struct _HIVE_LIST_ENTRY
    {
        PCWSTR HiveName;
        PCWSTR RegMountPoint;
    } HIVE_LIST_ENTRY;

    static const HIVE_LIST_ENTRY RegistryHives[] =
    {
        { L"TestHive1", L"\\Registry\\Machine\\TestHive1" },
        { L"TestHive2", L"\\Registry\\Machine\\TestHive2" },
    };

    NTSTATUS Status;
    UNICODE_STRING NtTestPath;
    UNICODE_STRING KeyName;
    HANDLE KeyHandle;
    ULONG Disposition;
    UINT i;
    BOOLEAN PrivilegeSet[2] = {FALSE, FALSE};
    WCHAR PathBuffer[MAX_PATH];

    pNtUnloadKey2 = (PVOID)GetProcAddress(GetModuleHandleW(L"ntdll.dll"), "NtUnloadKey2");

    /* Retrieve our current directory */
    RetrieveCurrentModuleNTDirectory(&NtTestPath);

    /* Acquire restore privilege */
    Status = RtlAdjustPrivilege(SE_RESTORE_PRIVILEGE, TRUE, FALSE, &PrivilegeSet[0]);
    if (!NT_SUCCESS(Status))
    {
        skip("RtlAdjustPrivilege(SE_RESTORE_PRIVILEGE) failed (Status 0x%08lx)\n", Status);
        /* Exit prematurely here.... */
        // goto Cleanup;
        RtlFreeUnicodeString(&NtTestPath);
        return;
    }

    /* Acquire backup privilege */
    Status = RtlAdjustPrivilege(SE_BACKUP_PRIVILEGE, TRUE, FALSE, &PrivilegeSet[1]);
    if (!NT_SUCCESS(Status))
    {
        skip("RtlAdjustPrivilege(SE_BACKUP_PRIVILEGE) failed (Status 0x%08lx)\n", Status);
        RtlAdjustPrivilege(SE_RESTORE_PRIVILEGE, PrivilegeSet[0], FALSE, &PrivilegeSet[0]);
        /* Exit prematurely here.... */
        // goto Cleanup;
        RtlFreeUnicodeString(&NtTestPath);
        return;
    }

    /* Create the template proto-hive */
    Status = CreateProtoHive(&KeyHandle);
    if (!NT_SUCCESS(Status))
    {
        skip("CreateProtoHive() failed to create the proto-hive; Status 0x%08lx\n", Status);
        goto Cleanup;
    }

    /* Create two registry hive files from it */
    for (i = 0; i < _countof(RegistryHives); ++i)
    {
        Status = CreateRegistryFile(NULL, &NtTestPath,
                                    RegistryHives[i].HiveName,
                                    KeyHandle);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("CreateRegistryFile(%S) failed, Status 0x%08lx\n", RegistryHives[i].HiveName, Status);
            /* Exit prematurely here.... */
            break;
        }
    }

    /* That is now done, remove the proto-hive */
    DestroyProtoHive(KeyHandle);

    /* Exit prematurely here if we failed */
    if (!NT_SUCCESS(Status))
        goto Cleanup;


/***********************************************************************************************/


    /* Now, mount the first hive */
    Status = ConnectRegistry(NULL, RegistryHives[0].RegMountPoint,
                             NULL, &NtTestPath,
                             RegistryHives[0].HiveName);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("ConnectRegistry('%wZ\\%S', '%S') failed, Status 0x%08lx\n",
                &NtTestPath, RegistryHives[0].HiveName, RegistryHives[0].RegMountPoint, Status);
    }

    /* Create or open a key inside the mounted hive */
    StringCchPrintfW(PathBuffer, _countof(PathBuffer), L"%s\\%s", RegistryHives[0].RegMountPoint, L"MyKey_1");
    RtlInitUnicodeString(&KeyName, PathBuffer);

    KeyHandle = NULL;
    Status = CreateRegKey(&KeyHandle,
                          NULL,
                          &KeyName,
                          REG_OPTION_NON_VOLATILE,
                          &Disposition);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("CreateRegKey(%wZ) failed (Status %lx)\n", &KeyName, Status);
    }
    else
    {
        DPRINT1("CreateRegKey(%wZ) succeeded to %s the key (Status %lx)\n",
                &KeyName,
                Disposition == REG_CREATED_NEW_KEY ? "create" : /* REG_OPENED_EXISTING_KEY */ "open",
                Status);
    }

    /* The key handle must be valid here */
    Status = NtFlushKey(KeyHandle);
    ok_ntstatus(Status, STATUS_SUCCESS);

    /* Attempt to unmount the hive, with the handle key still opened */
    Status = DisconnectRegistry(NULL, RegistryHives[0].RegMountPoint, 0); // Same as NtUnloadKey(&ObjectAttributes);
    DPRINT1("Unmounting '%S' %s\n", RegistryHives[0].RegMountPoint, NT_SUCCESS(Status) ? "succeeded" : "failed");
    ok_ntstatus(Status, STATUS_CANNOT_DELETE);

    /* The key handle should still be valid here */
    Status = NtFlushKey(KeyHandle);
    ok_ntstatus(Status, STATUS_SUCCESS);

    /* Force-unmount the hive, with the handle key still opened */
    Status = DisconnectRegistry(NULL, RegistryHives[0].RegMountPoint, REG_FORCE_UNLOAD);
    DPRINT1("Force-unmounting '%S' %s\n", RegistryHives[0].RegMountPoint, NT_SUCCESS(Status) ? "succeeded" : "failed");
    ok_hex(Status, STATUS_SUCCESS);

    /* The key handle should not be valid anymore */
    Status = NtFlushKey(KeyHandle);
    if (Status != STATUS_KEY_DELETED    /* Win2k3 */ &&
        Status != STATUS_HIVE_UNLOADED  /* Win7+  */)
    {
        ok_ntstatus(Status, STATUS_KEY_DELETED);
    }

    /* The key handle should not be valid anymore */
    Status = NtDeleteKey(KeyHandle);
    ok_ntstatus(Status, STATUS_SUCCESS);

    /* Close by principle the handle, but should this fail? */
    Status = NtClose(KeyHandle);
    ok_ntstatus(Status, STATUS_SUCCESS);


/***********************************************************************************************/


    /* Now, mount the first hive, again */
    Status = ConnectRegistry(NULL, RegistryHives[0].RegMountPoint,
                             NULL, &NtTestPath,
                             RegistryHives[0].HiveName);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("ConnectRegistry('%wZ\\%S', '%S') failed, Status 0x%08lx\n",
                &NtTestPath, RegistryHives[0].HiveName, RegistryHives[0].RegMountPoint, Status);
    }

    /* Create or open a key inside the mounted hive */
    StringCchPrintfW(PathBuffer, _countof(PathBuffer), L"%s\\%s", RegistryHives[0].RegMountPoint, L"MyKey_2");
    RtlInitUnicodeString(&KeyName, PathBuffer);

    KeyHandle = NULL;
    Status = CreateRegKey(&KeyHandle,
                          NULL,
                          &KeyName,
                          REG_OPTION_NON_VOLATILE,
                          &Disposition);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("CreateRegKey(%wZ) failed (Status %lx)\n", &KeyName, Status);
    }
    else
    {
        DPRINT1("CreateRegKey(%wZ) succeeded to %s the key (Status %lx)\n",
                &KeyName,
                Disposition == REG_CREATED_NEW_KEY ? "create" : /* REG_OPENED_EXISTING_KEY */ "open",
                Status);
    }

    /* The key handle must be valid here */
    Status = NtFlushKey(KeyHandle);
    ok_ntstatus(Status, STATUS_SUCCESS);

    /* Delete the key, this should succeed */
    Status = NtDeleteKey(KeyHandle);
    ok_ntstatus(Status, STATUS_SUCCESS);

    /* Close the handle, this should succeed */
    Status = NtClose(KeyHandle);
    ok_ntstatus(Status, STATUS_SUCCESS);

    /* Attempt to unmount the hive (no forcing), this should succeed */
    Status = DisconnectRegistry(NULL, RegistryHives[0].RegMountPoint, 0); // Same as NtUnloadKey(&ObjectAttributes);
    DPRINT1("Unmounting '%S' %s\n", RegistryHives[0].RegMountPoint, NT_SUCCESS(Status) ? "succeeded" : "failed");
    ok_ntstatus(Status, STATUS_SUCCESS);

    /* Force-unmount the hive (it is already unmounted), this should fail */
    Status = DisconnectRegistry(NULL, RegistryHives[0].RegMountPoint, REG_FORCE_UNLOAD);
    DPRINT1("Force-unmounting '%S' %s\n", RegistryHives[0].RegMountPoint, NT_SUCCESS(Status) ? "succeeded" : "failed");
    ok_hex(Status, STATUS_INVALID_PARAMETER);

#if 0
    /* Close by principle the handle, but should this fail? */
    Status = NtClose(KeyHandle);
    ok_ntstatus(Status, STATUS_SUCCESS);
#endif


/***********************************************************************************************/


Cleanup:

    /* Destroy the hive files */
    for (i = 0; i < _countof(RegistryHives); ++i)
    {
        Status = MyDeleteFile(NULL, &NtTestPath,
                              RegistryHives[i].HiveName, TRUE);
        if (!NT_SUCCESS(Status))
            DPRINT1("MyDeleteFile(%S) failed, Status 0x%08lx\n", RegistryHives[i].HiveName, Status);
    }

    /* Remove restore and backup privileges */
    RtlAdjustPrivilege(SE_BACKUP_PRIVILEGE, PrivilegeSet[1], FALSE, &PrivilegeSet[1]);
    RtlAdjustPrivilege(SE_RESTORE_PRIVILEGE, PrivilegeSet[0], FALSE, &PrivilegeSet[0]);

    RtlFreeUnicodeString(&NtTestPath);
}
