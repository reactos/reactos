/*
 *  ReactOS kernel
 *  Copyright (C) 2006 ReactOS Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS hive maker
 * FILE:            tools/mkhive/registry.c
 * PURPOSE:         Registry code
 * PROGRAMMER:      Hervé Poussineau
 */

/*
 * TODO:
 *   - Implement RegDeleteKeyW() and RegDeleteValueW()
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define NDEBUG
#include "mkhive.h"

static CMHIVE RootHive;
static PMEMKEY RootKey;
CMHIVE DefaultHive;  /* \Registry\User\.DEFAULT */
CMHIVE SamHive;      /* \Registry\Machine\SAM */
CMHIVE SecurityHive; /* \Registry\Machine\SECURITY */
CMHIVE SoftwareHive; /* \Registry\Machine\SOFTWARE */
CMHIVE SystemHive;   /* \Registry\Machine\SYSTEM */
CMHIVE BcdHive;      /* \Registry\Machine\BCD00000000 */

//
// TODO: Write these values in a more human-readable form.
// See http://amnesia.gtisc.gatech.edu/~moyix/suzibandit.ltd.uk/MSc/Registry%20Structure%20-%20Appendices%20V4.pdf
// Appendix 12 "The Registry NT Security Descriptor" for more information.
//
// Those SECURITY_DESCRIPTORs were obtained by dumping the security block "sk"
// of registry hives created by setting their permissions to be the same as
// the ones of the BCD, SOFTWARE, or SYSTEM, SAM and .DEFAULT system hives.
// A cross-check was subsequently done with the system hives to verify that
// the security descriptors were the same.
//
UCHAR BcdSecurity[] =
{
    // SECURITY_DESCRIPTOR_RELATIVE
    0x01,                   // Revision
    0x00,                   // Sbz1
    0x04, 0x94,             // Control: SE_SELF_RELATIVE        (0x8000) |
                            //          SE_DACL_PROTECTED       (0x1000) |
                            //          SE_DACL_AUTO_INHERITED  (0x0400) |
                            //          SE_DACL_PRESENT         (0x0004)
    0x48, 0x00, 0x00, 0x00, // Owner
    0x58, 0x00, 0x00, 0x00, // Group
    0x00, 0x00, 0x00, 0x00, // Sacl (None)
    0x14, 0x00, 0x00, 0x00, // Dacl

    // DACL
    0x02,       // AclRevision
    0x00,       // Sbz1
    0x34, 0x00, // AclSize
    0x02, 0x00, // AceCount
    0x00, 0x00, // Sbz2

    // (1st ACE)
    0x00,                   // AceType : ACCESS_ALLOWED_ACE_TYPE
    0x02,                   // AceFlags: CONTAINER_INHERIT_ACE
    0x18, 0x00,             // AceSize
    0x19, 0x00, 0x06, 0x00, // ACCESS_MASK: "Write DAC"         (0x00040000) |
                            //              "Read Control"      (0x00020000) |
                            //              "Notify"            (0x00000010) |
                            //              "Enumerate Subkeys" (0x00000008) |
                            //              "Query Value"       (0x00000001)
    // (SidStart: S-1-5-32-544 "Administrators")
    0x01, 0x02, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x05,
    0x20, 0x00, 0x00, 0x00,
    0x20, 0x02, 0x00, 0x00,

    // (2nd ACE)
    0x00,                   // AceType : ACCESS_ALLOWED_ACE_TYPE
    0x02,                   // AceFlags: CONTAINER_INHERIT_ACE
    0x14, 0x00,             // AceSize
    0x3F, 0x00, 0x0F, 0x00, // ACCESS_MASK: "Full Control" (0x000F003F)
    // (SidStart: S-1-5-18 "Local System")
    0x01, 0x01, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x05,
    0x12, 0x00, 0x00, 0x00,

    // Owner SID (S-1-5-32-544 "Administrators")
    0x01, 0x02, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x05,
    0x20, 0x00, 0x00, 0x00,
    0x20, 0x02, 0x00, 0x00,

    // Group SID (S-1-5-21-domain-513 "Domain Users")
    0x01, 0x05, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x05,
    0x15, 0x00, 0x00, 0x00,
    0xAC, 0xD0, 0x49, 0xCB,
    0xE6, 0x52, 0x47, 0x9C,
    0xE4, 0x31, 0xDB, 0x5C,
    0x01, 0x02, 0x00, 0x00
};

UCHAR SoftwareSecurity[] =
{
    // SECURITY_DESCRIPTOR_RELATIVE
    0x01,                   // Revision
    0x00,                   // Sbz1
    0x04, 0x94,             // Control: SE_SELF_RELATIVE        (0x8000) |
                            //          SE_DACL_PROTECTED       (0x1000) |
                            //          SE_DACL_AUTO_INHERITED  (0x0400) |
                            //          SE_DACL_PRESENT         (0x0004)
    0xA0, 0x00, 0x00, 0x00, // Owner
    0xB0, 0x00, 0x00, 0x00, // Group
    0x00, 0x00, 0x00, 0x00, // Sacl (None)
    0x14, 0x00, 0x00, 0x00, // Dacl

    // DACL
    0x02,       // AclRevision
    0x00,       // Sbz1
    0x8C, 0x00, // AclSize
    0x06, 0x00, // AceCount
    0x00, 0x00, // Sbz2

    // (1st ACE)
    0x00,                   // AceType : ACCESS_ALLOWED_ACE_TYPE
    0x02,                   // AceFlags: CONTAINER_INHERIT_ACE
    0x18, 0x00,             // AceSize
    0x3F, 0x00, 0x0F, 0x00, // ACCESS_MASK: "Full Control" (0x000F003F)
    // (SidStart: S-1-5-32-544 "Administrators")
    0x01, 0x02, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x05,
    0x20, 0x00, 0x00, 0x00,
    0x20, 0x02, 0x00, 0x00,

    // (2nd ACE)
    0x00,                   // AceType : ACCESS_ALLOWED_ACE_TYPE
    0x0A,                   // AceFlags: INHERIT_ONLY_ACE | CONTAINER_INHERIT_ACE
    0x14, 0x00,             // AceSize
    0x3F, 0x00, 0x0F, 0x00, // ACCESS_MASK: "Full Control" (0x000F003F)
    // (SidStart: S-1-3-0 "Creator Owner")
    0x01, 0x01, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x03,
    0x00, 0x00, 0x00, 0x00,

    // (3rd ACE)
    0x00,                   // AceType : ACCESS_ALLOWED_ACE_TYPE
    0x02,                   // AceFlags: CONTAINER_INHERIT_ACE
    0x14, 0x00,             // AceSize
    0x3F, 0x00, 0x0F, 0x00, // ACCESS_MASK: "Full Control" (0x000F003F)
    // (SidStart: S-1-5-18 "Local System")
    0x01, 0x01, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x05,
    0x12, 0x00, 0x00, 0x00,

    // (4th ACE)
    0x00,                   // AceType : ACCESS_ALLOWED_ACE_TYPE
    0x02,                   // AceFlags: CONTAINER_INHERIT_ACE
    0x14, 0x00,             // AceSize
    0x1F, 0x00, 0x03, 0x00, // ACCESS_MASK: "Read Control"      (0x00020000) |
                            //              "Delete"            (0x00010000) |
                            //              "Notify"            (0x00000010) |
                            //              "Enumerate Subkeys" (0x00000008) |
                            //              "Create Subkey"     (0x00000004) |
                            //              "Set Value"         (0x00000002) |
                            //              "Query Value"       (0x00000001)
    // (SidStart: S-1-5-13 "Terminal Server Users")
    0x01, 0x01, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x05,
    0x0D, 0x00, 0x00, 0x00,

    // (5th ACE)
    0x00,                   // AceType : ACCESS_ALLOWED_ACE_TYPE
    0x02,                   // AceFlags: CONTAINER_INHERIT_ACE
    0x18, 0x00,             // AceSize
    0x19, 0x00, 0x02, 0x00, // ACCESS_MASK: "Read Control"      (0x00020000) |
                            //              "Notify"            (0x00000010) |
                            //              "Enumerate Subkeys" (0x00000008) |
                            //              "Query Value"       (0x00000001)
    // (SidStart: S-1-5-32-545 "Users")
    0x01, 0x02, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x05,
    0x20, 0x00, 0x00, 0x00,
    0x21, 0x02, 0x00, 0x00,

    // (6th ACE)
    0x00,                   // AceType : ACCESS_ALLOWED_ACE_TYPE
    0x02,                   // AceFlags: CONTAINER_INHERIT_ACE
    0x18, 0x00,             // AceSize
    0x1F, 0x00, 0x03, 0x00, // ACCESS_MASK: "Read Control"      (0x00020000) |
                            //              "Delete"            (0x00010000) |
                            //              "Notify"            (0x00000010) |
                            //              "Enumerate Subkeys" (0x00000008) |
                            //              "Create Subkey"     (0x00000004) |
                            //              "Set Value"         (0x00000002) |
                            //              "Query Value"       (0x00000001)
    // (SidStart: S-1-5-32-547 "Power Users")
    0x01, 0x02, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x05,
    0x20, 0x00, 0x00, 0x00,
    0x23, 0x02, 0x00, 0x00,

    // Owner SID (S-1-5-32-544 "Administrators")
    0x01, 0x02, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x05,
    0x20, 0x00, 0x00, 0x00,
    0x20, 0x02, 0x00, 0x00,

    // Group SID (S-1-5-21-domain-513 "Domain Users")
    0x01, 0x05, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x05,
    0x15, 0x00, 0x00, 0x00,
    0xAC, 0xD0, 0x49, 0xCB,
    0xE6, 0x52, 0x47, 0x9C,
    0xE4, 0x31, 0xDB, 0x5C,
    0x01, 0x02, 0x00, 0x00
};

// Same security for SYSTEM, SAM and .DEFAULT
UCHAR SystemSecurity[] =
{
    // SECURITY_DESCRIPTOR_RELATIVE
    0x01,                   // Revision
    0x00,                   // Sbz1
    0x04, 0x94,             // Control: SE_SELF_RELATIVE        (0x8000) |
                            //          SE_DACL_PROTECTED       (0x1000) |
                            //          SE_DACL_AUTO_INHERITED  (0x0400) |
                            //          SE_DACL_PRESENT         (0x0004)
    0x8C, 0x00, 0x00, 0x00, // Owner
    0x9C, 0x00, 0x00, 0x00, // Group
    0x00, 0x00, 0x00, 0x00, // Sacl (None)
    0x14, 0x00, 0x00, 0x00, // Dacl

    // DACL
    0x02,       // AclRevision
    0x00,       // Sbz1
    0x78, 0x00, // AclSize
    0x05, 0x00, // AceCount
    0x00, 0x00, // Sbz2

    // (1st ACE)
    0x00,                   // AceType : ACCESS_ALLOWED_ACE_TYPE
    0x02,                   // AceFlags: CONTAINER_INHERIT_ACE
    0x18, 0x00,             // AceSize
    0x3F, 0x00, 0x0F, 0x00, // ACCESS_MASK: "Full Control" (0x000F003F)
    // (SidStart: S-1-5-32-544 "Administrators")
    0x01, 0x02, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x05,
    0x20, 0x00, 0x00, 0x00,
    0x20, 0x02, 0x00, 0x00,

    // (2nd ACE)
    0x00,                   // AceType : ACCESS_ALLOWED_ACE_TYPE
    0x0A,                   // AceFlags: INHERIT_ONLY_ACE | CONTAINER_INHERIT_ACE
    0x14, 0x00,             // AceSize
    0x3F, 0x00, 0x0F, 0x00, // ACCESS_MASK: "Full Control" (0x000F003F)
    // (SidStart: S-1-3-0 "Creator Owner")
    0x01, 0x01, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x03,
    0x00, 0x00, 0x00, 0x00,

    // (3rd ACE)
    0x00,                   // AceType : ACCESS_ALLOWED_ACE_TYPE
    0x02,                   // AceFlags: CONTAINER_INHERIT_ACE
    0x14, 0x00,             // AceSize
    0x3F, 0x00, 0x0F, 0x00, // ACCESS_MASK: "Full Control" (0x000F003F)
    // (SidStart: S-1-5-18 "Local System")
    0x01, 0x01, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x05,
    0x12, 0x00, 0x00, 0x00,

    // (4th ACE)
    0x00,                   // AceType : ACCESS_ALLOWED_ACE_TYPE
    0x02,                   // AceFlags: CONTAINER_INHERIT_ACE
    0x18, 0x00,             // AceSize
    0x19, 0x00, 0x02, 0x00, // ACCESS_MASK: "Read Control"      (0x00020000) |
                            //              "Notify"            (0x00000010) |
                            //              "Enumerate Subkeys" (0x00000008) |
                            //              "Query Value"       (0x00000001)
    // (SidStart: S-1-5-32-545 "Users")
    0x01, 0x02, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x05,
    0x20, 0x00, 0x00, 0x00,
    0x21, 0x02, 0x00, 0x00,

    // (5th ACE)
    0x00,                   // AceType : ACCESS_ALLOWED_ACE_TYPE
    0x02,                   // AceFlags: CONTAINER_INHERIT_ACE
    0x18, 0x00,             // AceSize
    0x19, 0x00, 0x02, 0x00, // ACCESS_MASK: "Read Control"      (0x00020000) |
                            //              "Notify"            (0x00000010) |
                            //              "Enumerate Subkeys" (0x00000008) |
                            //              "Query Value"       (0x00000001)
    // (SidStart: S-1-5-32-547 "Power Users")
    0x01, 0x02, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x05,
    0x20, 0x00, 0x00, 0x00,
    0x23, 0x02, 0x00, 0x00,

    // Owner SID (S-1-5-32-544 "Administrators")
    0x01, 0x02, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x05,
    0x20, 0x00, 0x00, 0x00,
    0x20, 0x02, 0x00, 0x00,

    // Group SID (S-1-5-21-domain-513 "Domain Users")
    0x01, 0x05, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x05,
    0x15, 0x00, 0x00, 0x00,
    0xAC, 0xD0, 0x49, 0xCB,
    0xE6, 0x52, 0x47, 0x9C,
    0xE4, 0x31, 0xDB, 0x5C,
    0x01, 0x02, 0x00, 0x00
};

static PMEMKEY
CreateInMemoryStructure(
    IN PCMHIVE RegistryHive,
    IN HCELL_INDEX KeyCellOffset)
{
    PMEMKEY Key;

    Key = (PMEMKEY)malloc(sizeof(MEMKEY));
    if (!Key)
        return NULL;

    Key->RegistryHive = RegistryHive;
    Key->KeyCellOffset = KeyCellOffset;
    return Key;
}

LIST_ENTRY CmiReparsePointsHead;

static LONG
RegpOpenOrCreateKey(
    IN HKEY hParentKey,
    IN PCWSTR KeyName,
    IN BOOL AllowCreation,
    IN BOOL Volatile,
    OUT PHKEY Key)
{
    PWSTR LocalKeyName;
    PWSTR End;
    UNICODE_STRING KeyString;
    NTSTATUS Status;
    PREPARSE_POINT CurrentReparsePoint;
    PMEMKEY CurrentKey;
    PCMHIVE ParentRegistryHive;
    HCELL_INDEX ParentCellOffset;
    PCM_KEY_NODE ParentKeyCell;
    PLIST_ENTRY Ptr;
    PCM_KEY_NODE SubKeyCell;
    HCELL_INDEX BlockOffset;

    DPRINT("RegpCreateOpenKey('%S')\n", KeyName);

    if (*KeyName == OBJ_NAME_PATH_SEPARATOR)
    {
        KeyName++;
        ParentRegistryHive = RootKey->RegistryHive;
        ParentCellOffset = RootKey->KeyCellOffset;
    }
    else if (hParentKey == NULL)
    {
        ParentRegistryHive = RootKey->RegistryHive;
        ParentCellOffset = RootKey->KeyCellOffset;
    }
    else
    {
        ParentRegistryHive = HKEY_TO_MEMKEY(hParentKey)->RegistryHive;
        ParentCellOffset = HKEY_TO_MEMKEY(hParentKey)->KeyCellOffset;
    }

    LocalKeyName = (PWSTR)KeyName;
    for (;;)
    {
        End = (PWSTR)strchrW(LocalKeyName, OBJ_NAME_PATH_SEPARATOR);
        if (End)
        {
            KeyString.Buffer = LocalKeyName;
            KeyString.Length = KeyString.MaximumLength =
                (USHORT)((ULONG_PTR)End - (ULONG_PTR)LocalKeyName);
        }
        else
        {
            RtlInitUnicodeString(&KeyString, LocalKeyName);
            if (KeyString.Length == 0)
            {
                /* Trailing path separator: we're done */
                break;
            }
        }

        ParentKeyCell = (PCM_KEY_NODE)HvGetCell(&ParentRegistryHive->Hive, ParentCellOffset);
        if (!ParentKeyCell)
            return STATUS_UNSUCCESSFUL;

        VERIFY_KEY_CELL(ParentKeyCell);

        BlockOffset = CmpFindSubKeyByName(&ParentRegistryHive->Hive, ParentKeyCell, &KeyString);
        if (BlockOffset != HCELL_NIL)
        {
            Status = STATUS_SUCCESS;

            /* Search for a possible reparse point */
            Ptr = CmiReparsePointsHead.Flink;
            while (Ptr != &CmiReparsePointsHead)
            {
                CurrentReparsePoint = CONTAINING_RECORD(Ptr, REPARSE_POINT, ListEntry);
                if (CurrentReparsePoint->SourceHive == ParentRegistryHive &&
                    CurrentReparsePoint->SourceKeyCellOffset == BlockOffset)
                {
                    ParentRegistryHive = CurrentReparsePoint->DestinationHive;
                    BlockOffset = CurrentReparsePoint->DestinationKeyCellOffset;
                    break;
                }
                Ptr = Ptr->Flink;
            }
        }
        else if (AllowCreation) // && (BlockOffset == HCELL_NIL)
        {
            Status = CmiAddSubKey(ParentRegistryHive,
                                  ParentCellOffset,
                                  &KeyString,
                                  Volatile,
                                  &BlockOffset);
        }

        HvReleaseCell(&ParentRegistryHive->Hive, ParentCellOffset);

        if (!NT_SUCCESS(Status))
            return ERROR_UNSUCCESSFUL;

        ParentCellOffset = BlockOffset;
        if (End)
            LocalKeyName = End + 1;
        else
            break;
    }

    CurrentKey = CreateInMemoryStructure(ParentRegistryHive, ParentCellOffset);
    if (!CurrentKey)
        return ERROR_OUTOFMEMORY;

    *Key = MEMKEY_TO_HKEY(CurrentKey);

    return ERROR_SUCCESS;
}

LONG WINAPI
RegCreateKeyW(
    IN HKEY hKey,
    IN LPCWSTR lpSubKey,
    OUT PHKEY phkResult)
{
    return RegpOpenOrCreateKey(hKey, lpSubKey, TRUE, FALSE, phkResult);
}

LONG WINAPI
RegDeleteKeyW(
    IN HKEY hKey,
    IN LPCWSTR lpSubKey)
{
    DPRINT1("RegDeleteKeyW(0x%p, '%S') is UNIMPLEMENTED!\n",
            hKey, (lpSubKey ? lpSubKey : L""));
    return ERROR_SUCCESS;
}

LONG WINAPI
RegOpenKeyW(
    IN HKEY hKey,
    IN LPCWSTR lpSubKey,
    OUT PHKEY phkResult)
{
    return RegpOpenOrCreateKey(hKey, lpSubKey, FALSE, FALSE, phkResult);
}

LONG WINAPI
RegCreateKeyExW(
    IN HKEY hKey,
    IN LPCWSTR lpSubKey,
    IN DWORD Reserved,
    IN LPWSTR lpClass OPTIONAL,
    IN DWORD dwOptions,
    IN REGSAM samDesired,
    IN LPSECURITY_ATTRIBUTES lpSecurityAttributes OPTIONAL,
    OUT PHKEY phkResult,
    OUT LPDWORD lpdwDisposition OPTIONAL)
{
    return RegpOpenOrCreateKey(hKey,
                               lpSubKey,
                               TRUE,
                               (dwOptions & REG_OPTION_VOLATILE) != 0,
                               phkResult);
}

LONG WINAPI
RegSetValueExW(
    IN HKEY hKey,
    IN LPCWSTR lpValueName OPTIONAL,
    IN ULONG Reserved,
    IN ULONG dwType,
    IN const UCHAR* lpData,
    IN ULONG cbData)
{
    PMEMKEY Key = HKEY_TO_MEMKEY(hKey); // ParentKey
    PHHIVE Hive;
    PCM_KEY_NODE KeyNode; // ParentNode
    PCM_KEY_VALUE ValueCell;
    HCELL_INDEX CellIndex;
    UNICODE_STRING ValueNameString;

    PVOID DataCell;
    ULONG DataCellSize;
    NTSTATUS Status;

    if (dwType == REG_LINK)
    {
        PMEMKEY DestKey;

        /* Special handling of registry links */
        if (cbData != sizeof(PVOID))
            return STATUS_INVALID_PARAMETER;

        DestKey = HKEY_TO_MEMKEY(*(PHKEY)lpData);

        // FIXME: Add additional checks for the validity of DestKey

        /* Create the link in registry hive (if applicable) */
        if (Key->RegistryHive != DestKey->RegistryHive)
            return STATUS_SUCCESS;

        DPRINT1("Save link to registry\n");
        return STATUS_NOT_IMPLEMENTED;
    }

    if ((cbData & ~CM_KEY_VALUE_SPECIAL_SIZE) != cbData)
        return STATUS_UNSUCCESSFUL;

    Hive = &Key->RegistryHive->Hive;

    KeyNode = (PCM_KEY_NODE)HvGetCell(Hive, Key->KeyCellOffset);
    if (!KeyNode)
        return ERROR_UNSUCCESSFUL;

    ASSERT(KeyNode->Signature == CM_KEY_NODE_SIGNATURE);

    /* Mark the parent as dirty since we are going to create a new value in it */
    HvMarkCellDirty(Hive, Key->KeyCellOffset, FALSE);

    /* Initialize value name string */
    RtlInitUnicodeString(&ValueNameString, lpValueName);
    CellIndex = CmpFindValueByName(Hive, KeyNode, &ValueNameString);
    if (CellIndex == HCELL_NIL)
    {
        /* The value doesn't exist, create a new one */
        Status = CmiAddValueKey(Key->RegistryHive,
                                KeyNode,
                                &ValueNameString,
                                &ValueCell,
                                &CellIndex);
    }
    else
    {
        /* The value already exists, use it. Get the value cell. */
        ValueCell = HvGetCell(&Key->RegistryHive->Hive, CellIndex);
        ASSERT(ValueCell != NULL);
        Status = STATUS_SUCCESS;
    }

    // /**/HvReleaseCell(Hive, CellIndex);/**/

    if (!NT_SUCCESS(Status))
        return ERROR_UNSUCCESSFUL;

    /* Get size of the allocated cell (if any) */
    if (!(ValueCell->DataLength & CM_KEY_VALUE_SPECIAL_SIZE) &&
         (ValueCell->DataLength & ~CM_KEY_VALUE_SPECIAL_SIZE) != 0)
    {
        DataCell = HvGetCell(Hive, ValueCell->Data);
        if (!DataCell)
            return ERROR_UNSUCCESSFUL;

        DataCellSize = (ULONG)(-HvGetCellSize(Hive, DataCell));
    }
    else
    {
        DataCell = NULL;
        DataCellSize = 0;
    }

    if (cbData <= sizeof(HCELL_INDEX))
    {
        /* If data size <= sizeof(HCELL_INDEX) then store data in the data offset */
        DPRINT("ValueCell->DataLength %u\n", ValueCell->DataLength);
        if (DataCell)
            HvFreeCell(Hive, ValueCell->Data);

        RtlCopyMemory(&ValueCell->Data, lpData, cbData);
        ValueCell->DataLength = (cbData | CM_KEY_VALUE_SPECIAL_SIZE);
        ValueCell->Type = dwType;
    }
    else
    {
        if (cbData > DataCellSize)
        {
            /* New data size is larger than the current, destroy current
             * data block and allocate a new one. */
            HCELL_INDEX NewOffset;

            DPRINT("ValueCell->DataLength %u\n", ValueCell->DataLength);

            NewOffset = HvAllocateCell(Hive, cbData, Stable, HCELL_NIL);
            if (NewOffset == HCELL_NIL)
            {
                DPRINT("HvAllocateCell() has failed!\n");
                return ERROR_UNSUCCESSFUL;
            }

            if (DataCell)
                HvFreeCell(Hive, ValueCell->Data);

            ValueCell->Data = NewOffset;
            DataCell = (PVOID)HvGetCell(Hive, NewOffset);
        }

        /* Copy new contents to cell */
        RtlCopyMemory(DataCell, lpData, cbData);
        ValueCell->DataLength = (cbData & ~CM_KEY_VALUE_SPECIAL_SIZE);
        ValueCell->Type = dwType;
        HvMarkCellDirty(Hive, ValueCell->Data, FALSE);
    }

    HvMarkCellDirty(Hive, CellIndex, FALSE);

    /* Check if the maximum value name length changed, update it if so */
    if (KeyNode->MaxValueNameLen < ValueNameString.Length)
        KeyNode->MaxValueNameLen = ValueNameString.Length;

    /* Check if the maximum data length changed, update it if so */
    if (KeyNode->MaxValueDataLen < cbData)
        KeyNode->MaxValueDataLen = cbData;

    /* Save the write time */
    KeQuerySystemTime(&KeyNode->LastWriteTime);

    return ERROR_SUCCESS;
}


// Synced with freeldr/windows/registry.c
static
VOID
RepGetValueData(
    IN PHHIVE Hive,
    IN PCM_KEY_VALUE ValueCell,
    OUT ULONG* Type OPTIONAL,
    OUT PUCHAR Data OPTIONAL,
    IN OUT ULONG* DataSize OPTIONAL)
{
    ULONG DataLength;
    PVOID DataCell;

    /* Does the caller want the type? */
    if (Type != NULL)
        *Type = ValueCell->Type;

    /* Does the caller provide DataSize? */
    if (DataSize != NULL)
    {
        // NOTE: CmpValueToData doesn't support big data (the function will
        // bugcheck if so), FreeLdr is not supposed to read such data.
        // If big data is needed, use instead CmpGetValueData.
        // CmpGetValueData(Hive, ValueCell, DataSize, &DataCell, ...);
        DataCell = CmpValueToData(Hive, ValueCell, &DataLength);

        /* Does the caller want the data? */
        if ((Data != NULL) && (*DataSize != 0))
        {
            RtlCopyMemory(Data,
                          DataCell,
                          min(*DataSize, DataLength));
        }

        /* Return the actual data length */
        *DataSize = DataLength;
    }
}

// Similar to RegQueryValue in freeldr/windows/registry.c
LONG WINAPI
RegQueryValueExW(
    IN HKEY hKey,
    IN LPCWSTR lpValueName,
    IN PULONG lpReserved,
    OUT PULONG lpType OPTIONAL,
    OUT PUCHAR lpData OPTIONAL,
    IN OUT PULONG lpcbData OPTIONAL)
{
    PMEMKEY ParentKey = HKEY_TO_MEMKEY(hKey);
    PHHIVE Hive = &ParentKey->RegistryHive->Hive;
    PCM_KEY_NODE KeyNode;
    PCM_KEY_VALUE ValueCell;
    HCELL_INDEX CellIndex;
    UNICODE_STRING ValueNameString;

    KeyNode = (PCM_KEY_NODE)HvGetCell(Hive, ParentKey->KeyCellOffset);
    if (!KeyNode)
        return ERROR_UNSUCCESSFUL;

    ASSERT(KeyNode->Signature == CM_KEY_NODE_SIGNATURE);

    /* Initialize value name string */
    RtlInitUnicodeString(&ValueNameString, lpValueName);
    CellIndex = CmpFindValueByName(Hive, KeyNode, &ValueNameString);
    if (CellIndex == HCELL_NIL)
        return ERROR_FILE_NOT_FOUND;

    /* Get the value cell */
    ValueCell = HvGetCell(Hive, CellIndex);
    ASSERT(ValueCell != NULL);

    RepGetValueData(Hive, ValueCell, lpType, lpData, lpcbData);

    HvReleaseCell(Hive, CellIndex);

    return ERROR_SUCCESS;
}

LONG WINAPI
RegDeleteValueW(
    IN HKEY hKey,
    IN LPCWSTR lpValueName OPTIONAL)
{
    DPRINT1("RegDeleteValueW(0x%p, '%S') is UNIMPLEMENTED!\n",
            hKey, (lpValueName ? lpValueName : L""));
    return ERROR_UNSUCCESSFUL;
}


static BOOL
ConnectRegistry(
    IN HKEY RootKey,
    IN PCMHIVE HiveToConnect,
    IN PUCHAR Descriptor,
    IN ULONG DescriptorLength,
    IN LPCWSTR Path)
{
    NTSTATUS Status;
    PREPARSE_POINT ReparsePoint;
    PMEMKEY NewKey;
    LONG rc;

    ReparsePoint = (PREPARSE_POINT)malloc(sizeof(REPARSE_POINT));
    if (!ReparsePoint)
        return FALSE;

    /*
     * Use a dummy root key name:
     * - On 2k/XP/2k3, this is "$$$PROTO.HIV"
     * - On Vista+, this is "CMI-CreateHive{guid}"
     * See https://github.com/libyal/winreg-kb/blob/master/documentation/Registry%20files.asciidoc
     * for more information.
     */
    Status = CmiInitializeHive(HiveToConnect, L"$$$PROTO.HIV");
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("CmiInitializeHive() failed with status 0x%08x\n", Status);
        free(ReparsePoint);
        return FALSE;
    }

    /*
     * Add security to the root key.
     * NOTE: One can implement this using the lpSecurityAttributes
     * parameter of RegCreateKeyExW.
     */
    Status = CmiCreateSecurityKey(&HiveToConnect->Hive,
                                  HiveToConnect->Hive.BaseBlock->RootCell,
                                  Descriptor, DescriptorLength);
    if (!NT_SUCCESS(Status))
        DPRINT1("Failed to add security for root key '%S'\n", Path);

    /* Create key */
    rc = RegCreateKeyExW(RootKey,
                         Path,
                         0,
                         NULL,
                         REG_OPTION_VOLATILE,
                         0,
                         NULL,
                         (PHKEY)&NewKey,
                         NULL);
    if (rc != ERROR_SUCCESS)
    {
        free(ReparsePoint);
        return FALSE;
    }

    ReparsePoint->SourceHive = NewKey->RegistryHive;
    ReparsePoint->SourceKeyCellOffset = NewKey->KeyCellOffset;
    NewKey->RegistryHive = HiveToConnect;
    NewKey->KeyCellOffset = HiveToConnect->Hive.BaseBlock->RootCell;
    ReparsePoint->DestinationHive = NewKey->RegistryHive;
    ReparsePoint->DestinationKeyCellOffset = NewKey->KeyCellOffset;
    InsertTailList(&CmiReparsePointsHead, &ReparsePoint->ListEntry);
    return TRUE;
}

LIST_ENTRY CmiHiveListHead;

VOID
RegInitializeRegistry(VOID)
{
    UNICODE_STRING RootKeyName = RTL_CONSTANT_STRING(L"\\");
    NTSTATUS Status;
    PMEMKEY ControlSetKey, CurrentControlSetKey;
    PREPARSE_POINT ReparsePoint;

    InitializeListHead(&CmiHiveListHead);
    InitializeListHead(&CmiReparsePointsHead);

    Status = CmiInitializeHive(&RootHive, L"");
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("CmiInitializeHive() failed with status 0x%08x\n", Status);
        return;
    }

    RootKey = CreateInMemoryStructure(&RootHive,
                                      RootHive.Hive.BaseBlock->RootCell);

    /* Create DEFAULT key */
    ConnectRegistry(NULL,
                    &DefaultHive,
                    SystemSecurity, sizeof(SystemSecurity),
                    L"Registry\\User\\.DEFAULT");

    /* Create SAM key */
    ConnectRegistry(NULL,
                    &SamHive,
                    SystemSecurity, sizeof(SystemSecurity),
                    L"Registry\\Machine\\SAM");

    /* Create SECURITY key */
    ConnectRegistry(NULL,
                    &SecurityHive,
                    NULL, 0,
                    L"Registry\\Machine\\SECURITY");

    /* Create SOFTWARE key */
    ConnectRegistry(NULL,
                    &SoftwareHive,
                    SoftwareSecurity, sizeof(SoftwareSecurity),
                    L"Registry\\Machine\\SOFTWARE");

    /* Create BCD key */
    ConnectRegistry(NULL,
                    &BcdHive,
                    BcdSecurity, sizeof(BcdSecurity),
                    L"Registry\\Machine\\BCD00000000");

    /* Create SYSTEM key */
    ConnectRegistry(NULL,
                    &SystemHive,
                    SystemSecurity, sizeof(SystemSecurity),
                    L"Registry\\Machine\\SYSTEM");

    /* Create 'ControlSet001' key */
    RegCreateKeyW(NULL,
                  L"Registry\\Machine\\SYSTEM\\ControlSet001",
                  (HKEY*)&ControlSetKey);

    /* Create 'CurrentControlSet' key */
    RegCreateKeyExW(NULL,
                    L"Registry\\Machine\\SYSTEM\\CurrentControlSet",
                    0,
                    NULL,
                    REG_OPTION_VOLATILE,
                    0,
                    NULL,
                    (HKEY*)&CurrentControlSetKey,
                    NULL);

    /* Connect 'CurrentControlSet' to 'ControlSet001' */
    ReparsePoint = (PREPARSE_POINT)malloc(sizeof(REPARSE_POINT));
    ReparsePoint->SourceHive = CurrentControlSetKey->RegistryHive;
    ReparsePoint->SourceKeyCellOffset = CurrentControlSetKey->KeyCellOffset;
    ReparsePoint->DestinationHive = ControlSetKey->RegistryHive;
    ReparsePoint->DestinationKeyCellOffset = ControlSetKey->KeyCellOffset;
    InsertTailList(&CmiReparsePointsHead, &ReparsePoint->ListEntry);
}

VOID
RegShutdownRegistry(VOID)
{
    /* FIXME: clean up the complete hive */

    free(RootKey);
}

/* EOF */
