/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Setup Library
 * FILE:            base/setup/lib/regutil.c
 * PURPOSE:         Registry utility functions
 * PROGRAMMERS:     Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

/* INCLUDES *****************************************************************/

#include "precomp.h"
#include "filesup.h"

#include "regutil.h"

#define NDEBUG
#include <debug.h>

/* FUNCTIONS ****************************************************************/

/*
 * This function is similar to the one in dlls/win32/advapi32/reg/reg.c
 * TODO: I should review both of them very carefully, because they may need
 * some adjustments in their NtCreateKey calls, especially for CreateOptions
 * stuff etc...
 */
NTSTATUS
CreateNestedKey(PHANDLE KeyHandle,
                ACCESS_MASK DesiredAccess,
                POBJECT_ATTRIBUTES ObjectAttributes,
                ULONG CreateOptions)
{
    OBJECT_ATTRIBUTES LocalObjectAttributes;
    UNICODE_STRING LocalKeyName;
    ULONG Disposition;
    NTSTATUS Status;
    USHORT FullNameLength;
    PWCHAR Ptr;
    HANDLE LocalKeyHandle;

    Status = NtCreateKey(KeyHandle,
                         KEY_ALL_ACCESS,
                         ObjectAttributes,
                         0,
                         NULL,
                         CreateOptions,
                         &Disposition);
    DPRINT("NtCreateKey(%wZ) called (Status %lx)\n", ObjectAttributes->ObjectName, Status);
    if (Status != STATUS_OBJECT_NAME_NOT_FOUND)
    {
        if (!NT_SUCCESS(Status))
            DPRINT1("CreateNestedKey: NtCreateKey(%wZ) failed (Status %lx)\n", ObjectAttributes->ObjectName, Status);

        return Status;
    }

    /* Copy object attributes */
    RtlCopyMemory(&LocalObjectAttributes,
                  ObjectAttributes,
                  sizeof(OBJECT_ATTRIBUTES));
    RtlCreateUnicodeString(&LocalKeyName,
                           ObjectAttributes->ObjectName->Buffer);
    LocalObjectAttributes.ObjectName = &LocalKeyName;
    FullNameLength = LocalKeyName.Length;

    /* Remove the last part of the key name and try to create the key again. */
    while (Status == STATUS_OBJECT_NAME_NOT_FOUND)
    {
        Ptr = wcsrchr(LocalKeyName.Buffer, '\\');
        if (Ptr == NULL || Ptr == LocalKeyName.Buffer)
        {
            Status = STATUS_UNSUCCESSFUL;
            break;
        }
        *Ptr = (WCHAR)0;
        LocalKeyName.Length = wcslen(LocalKeyName.Buffer) * sizeof(WCHAR);

        Status = NtCreateKey(&LocalKeyHandle,
                             KEY_CREATE_SUB_KEY,
                             &LocalObjectAttributes,
                             0,
                             NULL,
                             REG_OPTION_NON_VOLATILE,
                             &Disposition);
        DPRINT("NtCreateKey(%wZ) called (Status %lx)\n", &LocalKeyName, Status);
        if (!NT_SUCCESS(Status) && Status != STATUS_OBJECT_NAME_NOT_FOUND)
            DPRINT1("CreateNestedKey: NtCreateKey(%wZ) failed (Status %lx)\n", LocalObjectAttributes.ObjectName, Status);
    }

    if (!NT_SUCCESS(Status))
    {
        RtlFreeUnicodeString(&LocalKeyName);
        return Status;
    }

    /* Add removed parts of the key name and create them too. */
    while (TRUE)
    {
        if (LocalKeyName.Length == FullNameLength)
        {
            Status = STATUS_SUCCESS;
            *KeyHandle = LocalKeyHandle;
            break;
        }
        NtClose(LocalKeyHandle);

        LocalKeyName.Buffer[LocalKeyName.Length / sizeof(WCHAR)] = L'\\';
        LocalKeyName.Length = wcslen(LocalKeyName.Buffer) * sizeof(WCHAR);

        Status = NtCreateKey(&LocalKeyHandle,
                             KEY_ALL_ACCESS,
                             &LocalObjectAttributes,
                             0,
                             NULL,
                             CreateOptions,
                             &Disposition);
        DPRINT("NtCreateKey(%wZ) called (Status %lx)\n", &LocalKeyName, Status);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("CreateNestedKey: NtCreateKey(%wZ) failed (Status %lx)\n", LocalObjectAttributes.ObjectName, Status);
            break;
        }
    }

    RtlFreeUnicodeString(&LocalKeyName);

    return Status;
}


/*
 * Should be called under SE_BACKUP_PRIVILEGE privilege
 */
NTSTATUS
CreateRegistryFile(
    IN PUNICODE_STRING InstallPath,
    IN PCWSTR RegistryKey,
    IN BOOLEAN IsHiveNew,
    IN HANDLE ProtoKeyHandle
/*
    IN PUCHAR Descriptor,
    IN ULONG DescriptorLength
*/
    )
{
    /* '.old' is for old valid hives, while '.brk' is for old broken hives */
    static PCWSTR Extensions[] = {L"old", L"brk"};

    NTSTATUS Status;
    HANDLE FileHandle;
    UNICODE_STRING FileName;
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatusBlock;
    PCWSTR Extension;
    WCHAR PathBuffer[MAX_PATH];
    WCHAR PathBuffer2[MAX_PATH];

    CombinePaths(PathBuffer, ARRAYSIZE(PathBuffer), 3,
                 InstallPath->Buffer, L"System32\\config", RegistryKey);

    Extension = Extensions[IsHiveNew ? 0 : 1];

    //
    // FIXME: The best, actually, would be to rename (move) the existing
    // System32\config\RegistryKey file to System32\config\RegistryKey.old,
    // and if it already existed some System32\config\RegistryKey.old, we should
    // first rename this one into System32\config\RegistryKey_N.old before
    // performing the original rename.
    //

    /* Check whether the registry hive file already existed, and if so, rename it */
    if (DoesFileExist(NULL, PathBuffer))
    {
        // UINT i;

        DPRINT1("Registry hive '%S' already exists, rename it\n", PathBuffer);

        // i = 1;
        /* Try first by just appending the '.old' extension */
        StringCchPrintfW(PathBuffer2, ARRAYSIZE(PathBuffer2), L"%s.%s", PathBuffer, Extension);
#if 0
        while (DoesFileExist(NULL, PathBuffer2))
        {
            /* An old file already exists, increments its index, but not too much */
            if (i <= 0xFFFF)
            {
                /* Append '_N.old' extension */
                StringCchPrintfW(PathBuffer2, ARRAYSIZE(PathBuffer2), L"%s_%lu.%s", PathBuffer, i, Extension);
                ++i;
            }
            else
            {
                /*
                 * Too many old files exist, we will rename the file
                 * using the name of the oldest one.
                 */
                StringCchPrintfW(PathBuffer2, ARRAYSIZE(PathBuffer2), L"%s.%s", PathBuffer, Extension);
                break;
            }
        }
#endif

        /* Now rename the file (force the move) */
        Status = SetupMoveFile(PathBuffer, PathBuffer2, MOVEFILE_REPLACE_EXISTING);
    }

    /* Create the file */
    RtlInitUnicodeString(&FileName, PathBuffer);
    InitializeObjectAttributes(&ObjectAttributes,
                               &FileName,
                               OBJ_CASE_INSENSITIVE,
                               NULL,  // Could have been InstallPath, etc...
                               NULL); // Descriptor

    Status = NtCreateFile(&FileHandle,
                          FILE_GENERIC_WRITE,
                          &ObjectAttributes,
                          &IoStatusBlock,
                          NULL,
                          FILE_ATTRIBUTE_NORMAL,
                          0,
                          FILE_OVERWRITE_IF,
                          FILE_SYNCHRONOUS_IO_NONALERT | FILE_NON_DIRECTORY_FILE,
                          NULL,
                          0);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtCreateFile(%wZ) failed, Status 0x%08lx\n", &FileName, Status);
        return Status;
    }

    /* Save the selected hive into the file */
    Status = NtSaveKeyEx(ProtoKeyHandle, FileHandle, REG_LATEST_FORMAT);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtSaveKeyEx(%wZ) failed, Status 0x%08lx\n", &FileName, Status);
    }

    /* Close the file and return */
    NtClose(FileHandle);
    return Status;
}

BOOLEAN
CmpLinkKeyToHive(
    IN HANDLE RootLinkKeyHandle OPTIONAL,
    IN PCWSTR LinkKeyName,
    IN PCWSTR TargetKeyName)
{
    static UNICODE_STRING CmSymbolicLinkValueName =
        RTL_CONSTANT_STRING(L"SymbolicLinkValue");

    NTSTATUS Status;
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING KeyName;
    HANDLE TargetKeyHandle;
    ULONG Disposition;

    /* Initialize the object attributes */
    RtlInitUnicodeString(&KeyName, LinkKeyName);
    InitializeObjectAttributes(&ObjectAttributes,
                               &KeyName,
                               OBJ_CASE_INSENSITIVE,
                               RootLinkKeyHandle,
                               NULL);

    /* Create the link key */
    Status = NtCreateKey(&TargetKeyHandle,
                         KEY_SET_VALUE | KEY_CREATE_LINK,
                         &ObjectAttributes,
                         0,
                         NULL,
                         REG_OPTION_VOLATILE | REG_OPTION_CREATE_LINK,
                         &Disposition);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("CmpLinkKeyToHive: couldn't create %S, Status = 0x%08lx\n",
                LinkKeyName, Status);
        return FALSE;
    }

    /* Check if the new key was actually created */
    if (Disposition != REG_CREATED_NEW_KEY)
    {
        DPRINT1("CmpLinkKeyToHive: %S already exists!\n", LinkKeyName);
        NtClose(TargetKeyHandle);
        return FALSE;
    }

    /* Set the target key name as link target */
    RtlInitUnicodeString(&KeyName, TargetKeyName);
    Status = NtSetValueKey(TargetKeyHandle,
                           &CmSymbolicLinkValueName,
                           0,
                           REG_LINK,
                           KeyName.Buffer,
                           KeyName.Length);

    /* Close the link key handle */
    NtClose(TargetKeyHandle);

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("CmpLinkKeyToHive: couldn't create symbolic link for %S, Status = 0x%08lx\n",
                TargetKeyName, Status);
        return FALSE;
    }

    return TRUE;
}

/*
 * Should be called under SE_RESTORE_PRIVILEGE privilege
 */
NTSTATUS
ConnectRegistry(
    IN HKEY RootKey OPTIONAL,
    IN PCWSTR RegMountPoint,
    // IN HANDLE RootDirectory OPTIONAL,
    IN PUNICODE_STRING InstallPath,
    IN PCWSTR RegistryKey
/*
    IN PUCHAR Descriptor,
    IN ULONG DescriptorLength
*/
    )
{
    UNICODE_STRING KeyName, FileName;
    OBJECT_ATTRIBUTES KeyObjectAttributes;
    OBJECT_ATTRIBUTES FileObjectAttributes;
    WCHAR PathBuffer[MAX_PATH];

    RtlInitUnicodeString(&KeyName, RegMountPoint);
    InitializeObjectAttributes(&KeyObjectAttributes,
                               &KeyName,
                               OBJ_CASE_INSENSITIVE,
                               RootKey,
                               NULL);   // Descriptor

    CombinePaths(PathBuffer, ARRAYSIZE(PathBuffer), 3,
                 InstallPath->Buffer, L"System32\\config", RegistryKey);
    RtlInitUnicodeString(&FileName, PathBuffer);
    InitializeObjectAttributes(&FileObjectAttributes,
                               &FileName,
                               OBJ_CASE_INSENSITIVE,
                               NULL, // RootDirectory,
                               NULL);

    /* Mount the registry hive in the registry namespace */
    return NtLoadKey(&KeyObjectAttributes, &FileObjectAttributes);
}

/*
 * Should be called under SE_RESTORE_PRIVILEGE privilege
 */
NTSTATUS
DisconnectRegistry(
    IN HKEY RootKey OPTIONAL,
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

    // NOTE: NtUnloadKey == NtUnloadKey2 with Flags == 0.
    return NtUnloadKey2(&ObjectAttributes, Flags);
}

/*
 * Should be called under SE_RESTORE_PRIVILEGE privilege
 */
NTSTATUS
VerifyRegistryHive(
    // IN HKEY RootKey OPTIONAL,
    // // IN HANDLE RootDirectory OPTIONAL,
    IN PUNICODE_STRING InstallPath,
    IN PCWSTR RegistryKey /* ,
    IN PCWSTR RegMountPoint */)
{
    NTSTATUS Status;

    /* Try to mount the specified registry hive */
    Status = ConnectRegistry(NULL,
                             L"\\Registry\\Machine\\USetup_VerifyHive",
                             InstallPath,
                             RegistryKey
                             /* NULL, 0 */);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("ConnectRegistry(%S) failed, Status 0x%08lx\n", RegistryKey, Status);
    }

    DPRINT1("VerifyRegistryHive: ConnectRegistry(%S) returns Status 0x%08lx\n", RegistryKey, Status);

    //
    // TODO: Check the Status error codes: STATUS_SUCCESS, STATUS_REGISTRY_RECOVERED,
    // STATUS_REGISTRY_HIVE_RECOVERED, STATUS_REGISTRY_CORRUPT, STATUS_REGISTRY_IO_FAILED,
    // STATUS_NOT_REGISTRY_FILE, STATUS_CANNOT_LOAD_REGISTRY_FILE ;
    //(STATUS_HIVE_UNLOADED) ; STATUS_SYSTEM_HIVE_TOO_LARGE
    //

    if (Status == STATUS_REGISTRY_HIVE_RECOVERED) // NT_SUCCESS is still FALSE in this case!
        DPRINT1("VerifyRegistryHive: Registry hive %S was recovered but some data may be lost (Status 0x%08lx)\n", RegistryKey, Status);

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("VerifyRegistryHive: Registry hive %S is corrupted (Status 0x%08lx)\n", RegistryKey, Status);
        return Status;
    }

    if (Status == STATUS_REGISTRY_RECOVERED)
        DPRINT1("VerifyRegistryHive: Registry hive %S succeeded recovered (Status 0x%08lx)\n", RegistryKey, Status);

    /* Unmount the hive */
    Status = DisconnectRegistry(NULL,
                                L"\\Registry\\Machine\\USetup_VerifyHive",
                                0);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("DisconnectRegistry(%S) failed, Status 0x%08lx\n", RegistryKey, Status);
    }

    return Status;
}

/* EOF */
