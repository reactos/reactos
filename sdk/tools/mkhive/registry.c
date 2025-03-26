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
 * PROGRAMMERS:     Hervé Poussineau
 *                  Hermès Bélusca-Maïto
 */

/* INCLUDES *****************************************************************/

#define NDEBUG
#include "mkhive.h"

/* DATA *********************************************************************/

typedef struct _REPARSE_POINT
{
    LIST_ENTRY ListEntry;
    PCMHIVE SourceHive;
    HCELL_INDEX SourceKeyCellOffset;
    PCMHIVE DestinationHive;
    HCELL_INDEX DestinationKeyCellOffset;
} REPARSE_POINT, *PREPARSE_POINT;

typedef struct _MEMKEY
{
    /* Information on hard disk structure */
    HCELL_INDEX KeyCellOffset;
    PCMHIVE RegistryHive;
} MEMKEY, *PMEMKEY;

#define HKEY_TO_MEMKEY(hKey) ((PMEMKEY)(hKey))
#define MEMKEY_TO_HKEY(memKey) ((HKEY)(memKey))

static CMHIVE RootHive;
static PMEMKEY RootKey;

static CMHIVE SystemHive;   /* \Registry\Machine\SYSTEM */
static CMHIVE SoftwareHive; /* \Registry\Machine\SOFTWARE */
static CMHIVE DefaultHive;  /* \Registry\User\.DEFAULT */
static CMHIVE SamHive;      /* \Registry\Machine\SAM */
static CMHIVE SecurityHive; /* \Registry\Machine\SECURITY */
static CMHIVE BcdHive;      /* \Registry\Machine\BCD00000000 */

//
// TODO: Write these values in a more human-readable form.
// See http://amnesia.gtisc.gatech.edu/~moyix/suzibandit.ltd.uk/MSc/Registry%20Structure%20-%20Appendices%20V4.pdf
// Appendix 12 "The Registry NT Security Descriptor" for more information.
//
// These SECURITY_DESCRIPTORs were obtained by dumping the security block "sk"
// of registry hives created by setting their permissions to be the same as
// the ones of the BCD, SOFTWARE, or SYSTEM, SAM and .DEFAULT system hives.
// A cross-check was subsequently done with the system hives to verify that
// the security descriptors were the same.
//
static UCHAR BcdSecurity[] =
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

static UCHAR SoftwareSecurity[] =
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
static UCHAR SystemSecurity[] =
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

/* GLOBALS ******************************************************************/

HIVE_LIST_ENTRY RegistryHives[/*MAX_NUMBER_OF_REGISTRY_HIVES*/] =
{
    /* Special Setup system registry hive */
    // WARNING: Please *keep* it in first position!
    { "SETUPREG", L"Registry\\Machine\\SYSTEM"     , &SystemHive  , SystemSecurity  , sizeof(SystemSecurity)   },

    /* Regular registry hives */
    { "SYSTEM"  , L"Registry\\Machine\\SYSTEM"     , &SystemHive  , SystemSecurity  , sizeof(SystemSecurity)   },
    { "SOFTWARE", L"Registry\\Machine\\SOFTWARE"   , &SoftwareHive, SoftwareSecurity, sizeof(SoftwareSecurity) },
    { "DEFAULT" , L"Registry\\User\\.DEFAULT"      , &DefaultHive , SystemSecurity  , sizeof(SystemSecurity)   },
    { "SAM"     , L"Registry\\Machine\\SAM"        , &SamHive     , SystemSecurity  , sizeof(SystemSecurity)   },
    { "SECURITY", L"Registry\\Machine\\SECURITY"   , &SecurityHive, NULL            , 0                        },
    { "BCD"     , L"Registry\\Machine\\BCD00000000", &BcdHive     , BcdSecurity     , sizeof(BcdSecurity)      },
};
C_ASSERT(_countof(RegistryHives) == MAX_NUMBER_OF_REGISTRY_HIVES);

/* FUNCTIONS ****************************************************************/

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

LIST_ENTRY CmiHiveListHead;
LIST_ENTRY CmiReparsePointsHead;

static LONG
RegpCreateOrOpenKey(
    IN HKEY hParentKey,
    IN PCWSTR KeyName,
    IN BOOL AllowCreation,
    IN BOOL Volatile,
    OUT PHKEY Key)
{
    NTSTATUS Status;
    PWSTR LocalKeyName;
    PWSTR End;
    UNICODE_STRING KeyString;
    PREPARSE_POINT CurrentReparsePoint;
    PMEMKEY CurrentKey;
    PCMHIVE ParentRegistryHive;
    HCELL_INDEX ParentCellOffset;
    PCM_KEY_NODE ParentKeyCell;
    PLIST_ENTRY Ptr;
    HCELL_INDEX BlockOffset;

    DPRINT("RegpCreateOrOpenKey('%S')\n", KeyName);

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
            return ERROR_GEN_FAILURE; // STATUS_UNSUCCESSFUL;

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
        else // if (BlockOffset == HCELL_NIL)
        {
            Status = STATUS_OBJECT_NAME_NOT_FOUND;
        }

        HvReleaseCell(&ParentRegistryHive->Hive, ParentCellOffset);

        if (!NT_SUCCESS(Status))
        {
            DPRINT("RegpCreateOrOpenKey('%S'): Could not create or open subkey '%.*S', Status 0x%08x\n",
                   KeyName, (int)(KeyString.Length / sizeof(WCHAR)), KeyString.Buffer, Status);
            return ERROR_GEN_FAILURE; // STATUS_UNSUCCESSFUL;
        }

        ParentCellOffset = BlockOffset;
        if (End)
            LocalKeyName = End + 1;
        else
            break;
    }

    CurrentKey = CreateInMemoryStructure(ParentRegistryHive, ParentCellOffset);
    if (!CurrentKey)
        return ERROR_NOT_ENOUGH_MEMORY; // STATUS_NO_MEMORY;

    *Key = MEMKEY_TO_HKEY(CurrentKey);

    return ERROR_SUCCESS;
}

LONG WINAPI
RegCloseKey(
    IN HKEY hKey)
{
    PMEMKEY Key = HKEY_TO_MEMKEY(hKey); // ParentKey

    /* Free the object */
    free(Key);

    return ERROR_SUCCESS;
}

LONG WINAPI
RegCreateKeyW(
    IN HKEY hKey,
    IN LPCWSTR lpSubKey,
    OUT PHKEY phkResult)
{
    return RegpCreateOrOpenKey(hKey, lpSubKey, TRUE, FALSE, phkResult);
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
    return RegpCreateOrOpenKey(hKey,
                               lpSubKey,
                               TRUE,
                               (dwOptions & REG_OPTION_VOLATILE) != 0,
                               phkResult);
}

LONG WINAPI
RegDeleteKeyW(
    IN HKEY hKey,
    IN LPCWSTR lpSubKey)
{
    LONG rc;
    NTSTATUS Status;
    HKEY hTargetKey;
    PMEMKEY Key; // ParentKey
    PHHIVE Hive;
    PCM_KEY_NODE KeyNode; // ParentNode
    PCM_KEY_NODE Parent;
    HCELL_INDEX ParentCell;

    if (lpSubKey)
    {
        rc = RegOpenKeyW(hKey, lpSubKey, &hTargetKey);
        if (rc != ERROR_SUCCESS)
            return rc;
    }
    else
    {
        hTargetKey = hKey;
        rc = ERROR_SUCCESS;
    }

    /* Don't allow deleting the root */
    if (hTargetKey == RootKey)
    {
        /* Fail */
        rc = ERROR_ACCESS_DENIED; // STATUS_CANNOT_DELETE;
        goto Quit;
    }

    /* Get the hive and node */
    Key = HKEY_TO_MEMKEY(hTargetKey);
    Hive = &Key->RegistryHive->Hive;

    /* Get the key node */
    KeyNode = (PCM_KEY_NODE)HvGetCell(Hive, Key->KeyCellOffset);
    if (!KeyNode)
    {
        rc = ERROR_GEN_FAILURE; // STATUS_UNSUCCESSFUL;
        goto Quit;
    }

    ASSERT(KeyNode->Signature == CM_KEY_NODE_SIGNATURE);

    /* Check if we don't have any children */
    if (!(KeyNode->SubKeyCounts[Stable] + KeyNode->SubKeyCounts[Volatile]) &&
        !(KeyNode->Flags & KEY_NO_DELETE))
    {
        /* Get the parent and free the cell */
        ParentCell = KeyNode->Parent;
        Status = CmpFreeKeyByCell(Hive, Key->KeyCellOffset, TRUE);
        if (NT_SUCCESS(Status))
        {
            /* Get the parent node */
            Parent = (PCM_KEY_NODE)HvGetCell(Hive, ParentCell);
            if (Parent)
            {
                /* Make sure we're dirty */
                ASSERT(HvIsCellDirty(Hive, ParentCell));

                /* Update the write time */
                KeQuerySystemTime(&Parent->LastWriteTime);

                /* Release the cell */
                HvReleaseCell(Hive, ParentCell);
            }

            rc = ERROR_SUCCESS;
        }
        else
        {
            /* Fail */
            rc = ERROR_GEN_FAILURE; // STATUS_UNSUCCESSFUL;
        }
    }
    else
    {
        /* Fail */
        rc = ERROR_ACCESS_DENIED; // STATUS_CANNOT_DELETE;
    }

    /* Release the cell */
    HvReleaseCell(Hive, Key->KeyCellOffset);

Quit:
    if (lpSubKey)
        RegCloseKey(hTargetKey);

    return rc;
}

LONG WINAPI
RegOpenKeyW(
    IN HKEY hKey,
    IN LPCWSTR lpSubKey,
    OUT PHKEY phkResult)
{
    return RegpCreateOrOpenKey(hKey, lpSubKey, FALSE, FALSE, phkResult);
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
    ULONG ChildIndex;
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
            return ERROR_INVALID_PARAMETER; // STATUS_INVALID_PARAMETER;

        DestKey = HKEY_TO_MEMKEY(*(PHKEY)lpData);

        // FIXME: Add additional checks for the validity of DestKey

        /* Create the link in registry hive (if applicable) */
        if (Key->RegistryHive != DestKey->RegistryHive)
            return ERROR_SUCCESS;

        DPRINT1("Save link to registry\n");
        return ERROR_INVALID_FUNCTION; // STATUS_NOT_IMPLEMENTED;
    }

    if ((cbData & ~CM_KEY_VALUE_SPECIAL_SIZE) != cbData)
        return ERROR_GEN_FAILURE; // STATUS_UNSUCCESSFUL;

    Hive = &Key->RegistryHive->Hive;

    KeyNode = (PCM_KEY_NODE)HvGetCell(Hive, Key->KeyCellOffset);
    if (!KeyNode)
        return ERROR_GEN_FAILURE; // STATUS_UNSUCCESSFUL;

    ASSERT(KeyNode->Signature == CM_KEY_NODE_SIGNATURE);

    /* Mark the parent as dirty since we are going to create a new value in it */
    HvMarkCellDirty(Hive, Key->KeyCellOffset, FALSE);

    /* Initialize value name string */
    RtlInitUnicodeString(&ValueNameString, lpValueName);
    if (!CmpFindNameInList(Hive,
                           &KeyNode->ValueList,
                           &ValueNameString,
                           &ChildIndex,
                           &CellIndex))
    {
        /* Sanity check */
        ASSERT(CellIndex == HCELL_NIL);
        /* Fail */
        Status = STATUS_INSUFFICIENT_RESOURCES;
    }
    if (CellIndex == HCELL_NIL)
    {
        /* The value doesn't exist, create a new one */
        Status = CmiAddValueKey(Key->RegistryHive,
                                KeyNode,
                                ChildIndex,
                                &ValueNameString,
                                &ValueCell,
                                &CellIndex);
    }
    else
    {
        /* The value already exists, use it. Get the value cell. */
        ValueCell = (PCM_KEY_VALUE)HvGetCell(&Key->RegistryHive->Hive, CellIndex);
        ASSERT(ValueCell != NULL);
        Status = STATUS_SUCCESS;
    }

    // /**/HvReleaseCell(Hive, CellIndex);/**/

    if (!NT_SUCCESS(Status))
        return ERROR_GEN_FAILURE; // STATUS_UNSUCCESSFUL;

    /* Get size of the allocated cell (if any) */
    if (!(ValueCell->DataLength & CM_KEY_VALUE_SPECIAL_SIZE) &&
         (ValueCell->DataLength & ~CM_KEY_VALUE_SPECIAL_SIZE) != 0)
    {
        DataCell = HvGetCell(Hive, ValueCell->Data);
        if (!DataCell)
            return ERROR_GEN_FAILURE; // STATUS_UNSUCCESSFUL;

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
                return ERROR_GEN_FAILURE; // STATUS_UNSUCCESSFUL;
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


// Synced with freeldr/ntldr/registry.c
static
VOID
RepGetValueData(
    IN PHHIVE Hive,
    IN PCM_KEY_VALUE ValueCell,
    OUT PULONG Type OPTIONAL,
    OUT PUCHAR Data OPTIONAL,
    IN OUT PULONG DataSize OPTIONAL)
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

// Similar to RegQueryValue in freeldr/ntldr/registry.c
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
        return ERROR_GEN_FAILURE; // STATUS_UNSUCCESSFUL;

    ASSERT(KeyNode->Signature == CM_KEY_NODE_SIGNATURE);

    /* Initialize value name string */
    RtlInitUnicodeString(&ValueNameString, lpValueName);
    CellIndex = CmpFindValueByName(Hive, KeyNode, &ValueNameString);
    if (CellIndex == HCELL_NIL)
        return ERROR_FILE_NOT_FOUND; // STATUS_OBJECT_NAME_NOT_FOUND;

    /* Get the value cell */
    ValueCell = (PCM_KEY_VALUE)HvGetCell(Hive, CellIndex);
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
    LONG rc;
    NTSTATUS Status;
    PMEMKEY Key = HKEY_TO_MEMKEY(hKey); // ParentKey
    PHHIVE Hive = &Key->RegistryHive->Hive;
    PCM_KEY_NODE KeyNode; // ParentNode
    PCM_KEY_VALUE ValueCell;
    HCELL_INDEX CellIndex;
    ULONG ChildIndex;
    UNICODE_STRING ValueNameString;

    KeyNode = (PCM_KEY_NODE)HvGetCell(Hive, Key->KeyCellOffset);
    if (!KeyNode)
        return ERROR_GEN_FAILURE; // STATUS_UNSUCCESSFUL;

    ASSERT(KeyNode->Signature == CM_KEY_NODE_SIGNATURE);

    /* Initialize value name string */
    RtlInitUnicodeString(&ValueNameString, lpValueName);
    if (!CmpFindNameInList(Hive,
                           &KeyNode->ValueList,
                           &ValueNameString,
                           &ChildIndex,
                           &CellIndex))
    {
        /* Sanity check */
        ASSERT(CellIndex == HCELL_NIL);
    }
    if (CellIndex == HCELL_NIL)
    {
        rc = ERROR_FILE_NOT_FOUND; // STATUS_OBJECT_NAME_NOT_FOUND;
        goto Quit;
    }

    /* We found the value, mark all relevant cells dirty */
    HvMarkCellDirty(Hive, Key->KeyCellOffset, FALSE);
    HvMarkCellDirty(Hive, KeyNode->ValueList.List, FALSE);
    HvMarkCellDirty(Hive, CellIndex, FALSE);

    /* Get the key value */
    ValueCell = (PCM_KEY_VALUE)HvGetCell(Hive, CellIndex);
    ASSERT(ValueCell);

    /* Mark it and all related data as dirty */
    if (!CmpMarkValueDataDirty(Hive, ValueCell))
    {
        /* Not enough log space, fail */
        rc = ERROR_NO_LOG_SPACE; // STATUS_NO_LOG_SPACE;
        goto Quit;
    }

    /* Sanity checks */
    ASSERT(HvIsCellDirty(Hive, KeyNode->ValueList.List));
    ASSERT(HvIsCellDirty(Hive, CellIndex));

    /* Remove the value from the child list */
    Status = CmpRemoveValueFromList(Hive, ChildIndex, &KeyNode->ValueList);
    if (!NT_SUCCESS(Status))
    {
        /* Set known error */
        rc = ERROR_NO_SYSTEM_RESOURCES; // STATUS_INSUFFICIENT_RESOURCES;
        goto Quit;
    }

    /* Remove the value and its data itself */
    if (!CmpFreeValue(Hive, CellIndex))
    {
        /* Failed to free the value, fail */
        rc = ERROR_NO_SYSTEM_RESOURCES; // STATUS_INSUFFICIENT_RESOURCES;
        goto Quit;
    }

    /* Set the last write time */
    KeQuerySystemTime(&KeyNode->LastWriteTime);

    /* Sanity check */
    ASSERT(HvIsCellDirty(Hive, Key->KeyCellOffset));

    /* Check if the value list is empty now */
    if (!KeyNode->ValueList.Count)
    {
        /* Then clear key node data */
        KeyNode->MaxValueNameLen = 0;
        KeyNode->MaxValueDataLen = 0;
    }

    /* Change default Status to success */
    rc = ERROR_SUCCESS;

Quit:
    /* Check if we had a value */
    if (ValueCell)
    {
        /* Release the child cell */
        ASSERT(CellIndex != HCELL_NIL);
        HvReleaseCell(Hive, CellIndex);
    }

    /* Release the parent cell, if any */
    if (KeyNode)
        HvReleaseCell(Hive, Key->KeyCellOffset);

    return rc;
}


static BOOL
ConnectRegistry(
    IN HKEY RootKey,
    IN PCWSTR Path,
    IN PCMHIVE HiveToConnect,
    IN PUCHAR SecurityDescriptor,
    IN ULONG SecurityDescriptorLength)
{
    NTSTATUS Status;
    LONG rc;
    PREPARSE_POINT ReparsePoint;
    PMEMKEY NewKey;

    ReparsePoint = (PREPARSE_POINT)malloc(sizeof(*ReparsePoint));
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
                                  SecurityDescriptor, SecurityDescriptorLength);
    if (!NT_SUCCESS(Status))
        DPRINT1("Failed to add security for root key '%S'\n", Path);

    /* Create the key */
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

static BOOL
CreateSymLink(
    IN PCWSTR LinkKeyPath OPTIONAL,
    IN OUT PHKEY LinkKeyHandle OPTIONAL,
    // IN PCWSTR TargetKeyPath OPTIONAL,
    IN HKEY TargetKeyHandle)
{
    LONG rc;
    PMEMKEY LinkKey, TargetKey;
    PREPARSE_POINT ReparsePoint;

    ReparsePoint = (PREPARSE_POINT)malloc(sizeof(*ReparsePoint));
    if (!ReparsePoint)
        return FALSE;

    if (LinkKeyPath && !(LinkKeyHandle && *LinkKeyHandle))
    {
        /* Create the link key */
        rc = RegCreateKeyExW(NULL,
                             LinkKeyPath,
                             0,
                             NULL,
                             REG_OPTION_VOLATILE,
                             0,
                             NULL,
                             (PHKEY)&LinkKey,
                             NULL);
        if (rc != ERROR_SUCCESS)
        {
            free(ReparsePoint);
            return FALSE;
        }
    }
    else if (LinkKeyHandle)
    {
        /* Use the user-provided link key handle */
        LinkKey = HKEY_TO_MEMKEY(*LinkKeyHandle);
    }

    if (LinkKeyHandle)
        *LinkKeyHandle = MEMKEY_TO_HKEY(LinkKey);

    TargetKey = HKEY_TO_MEMKEY(TargetKeyHandle);

    ReparsePoint->SourceHive = LinkKey->RegistryHive;
    ReparsePoint->SourceKeyCellOffset = LinkKey->KeyCellOffset;
    ReparsePoint->DestinationHive = TargetKey->RegistryHive;
    ReparsePoint->DestinationKeyCellOffset = TargetKey->KeyCellOffset;
    InsertTailList(&CmiReparsePointsHead, &ReparsePoint->ListEntry);

    return TRUE;
}

VOID
RegInitializeRegistry(
    IN PCSTR HiveList)
{
    NTSTATUS Status;
    UINT i;
    HKEY ControlSetKey;

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

    for (i = 0; i < _countof(RegistryHives); ++i)
    {
        /* Skip this registry hive if it's not in the list */
        if (!strstr(HiveList, RegistryHives[i].HiveName))
            continue;

        /* Create the registry key */
        ConnectRegistry(NULL,
                        RegistryHives[i].HiveRegistryPath,
                        RegistryHives[i].CmHive,
                        RegistryHives[i].SecurityDescriptor,
                        RegistryHives[i].SecurityDescriptorLength);

        /* If we happen to deal with the special setup registry hive, stop there */
        // if (strcmp(RegistryHives[i].HiveName, "SETUPREG") == 0)
        if (i == 0)
            break;
    }

    /* Create the 'ControlSet001' key */
    RegCreateKeyW(NULL,
                  L"Registry\\Machine\\SYSTEM\\ControlSet001",
                  &ControlSetKey);

    /* Create the 'CurrentControlSet' key as a symlink to 'ControlSet001' */
    CreateSymLink(L"Registry\\Machine\\SYSTEM\\CurrentControlSet",
                  NULL, ControlSetKey);

    RegCloseKey(ControlSetKey);

#if 0
    /* Link SECURITY to SAM */
    CmpLinkKeyToHive(L"\\Registry\\Machine\\Security\\SAM", L"\\Registry\\Machine\\SAM\\SAM");
    /* Link S-1-5-18 to .Default */
    CmpLinkKeyToHive(L"\\Registry\\User\\S-1-5-18", L"\\Registry\\User\\.Default");
#endif
}

VOID
RegShutdownRegistry(VOID)
{
    PLIST_ENTRY Entry;
    PREPARSE_POINT ReparsePoint;

    /* Clean up the reparse points list */
    while (!IsListEmpty(&CmiReparsePointsHead))
    {
        Entry = RemoveHeadList(&CmiReparsePointsHead);
        ReparsePoint = CONTAINING_RECORD(Entry, REPARSE_POINT, ListEntry);
        free(ReparsePoint);
    }

    /* FIXME: clean up the complete hive */

    free(RootKey);
}

/* EOF */
