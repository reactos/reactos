/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Setup Library
 * FILE:            base/setup/lib/regutil.h
 * PURPOSE:         Registry utility functions
 * PROGRAMMERS:     Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

#pragma once

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
                ULONG CreateOptions);

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
    );

BOOLEAN
CmpLinkKeyToHive(
    IN HANDLE RootLinkKeyHandle OPTIONAL,
    IN PCWSTR LinkKeyName,
    IN PCWSTR TargetKeyName);

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
    );

/*
 * Should be called under SE_RESTORE_PRIVILEGE privilege
 */
NTSTATUS
DisconnectRegistry(
    IN HKEY RootKey OPTIONAL,
    IN PCWSTR RegMountPoint,
    IN ULONG Flags);

/*
 * Should be called under SE_RESTORE_PRIVILEGE privilege
 */
NTSTATUS
VerifyRegistryHive(
    // IN HKEY RootKey OPTIONAL,
    // // IN HANDLE RootDirectory OPTIONAL,
    IN PUNICODE_STRING InstallPath,
    IN PCWSTR RegistryKey /* ,
    IN PCWSTR RegMountPoint */);

/* EOF */
