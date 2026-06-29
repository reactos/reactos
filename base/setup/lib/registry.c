/*
 *  ReactOS kernel
 *  Copyright (C) 2003 ReactOS Team
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
 * PROJECT:         ReactOS Setup Library
 * FILE:            base/setup/lib/registry.c
 * PURPOSE:         Registry creation functions
 * PROGRAMMERS:     ...
 *                  Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

/* INCLUDES *****************************************************************/

#include "precomp.h"
#include "filesup.h"
#include "infsupp.h"
#include "regutil.h"

#include "registry.h"

#define NDEBUG
#include <debug.h>


// #ifdef __REACTOS__
#if 1 // FIXME: Disable if setupapi.h is included in the code...
#define FLG_ADDREG_BINVALUETYPE           0x00000001
#define FLG_ADDREG_NOCLOBBER              0x00000002
#define FLG_ADDREG_DELVAL                 0x00000004
#define FLG_ADDREG_APPEND                 0x00000008
#define FLG_ADDREG_KEYONLY                0x00000010
#define FLG_ADDREG_OVERWRITEONLY          0x00000020
#define FLG_ADDREG_TYPE_SZ                0x00000000
#define FLG_ADDREG_TYPE_MULTI_SZ          0x00010000
#define FLG_ADDREG_TYPE_EXPAND_SZ         0x00020000
#define FLG_ADDREG_TYPE_BINARY           (0x00000000 | FLG_ADDREG_BINVALUETYPE)
#define FLG_ADDREG_TYPE_DWORD            (0x00010000 | FLG_ADDREG_BINVALUETYPE)
#define FLG_ADDREG_TYPE_NONE             (0x00020000 | FLG_ADDREG_BINVALUETYPE)
#define FLG_ADDREG_TYPE_MASK             (0xFFFF0000 | FLG_ADDREG_BINVALUETYPE)
#endif

/* GLOBALS ******************************************************************/

#define REGISTRY_SETUP_MACHINE  L"\\Registry\\Machine\\SYSTEM\\USetup_Machine\\"
#define REGISTRY_SETUP_USER     L"\\Registry\\Machine\\SYSTEM\\USetup_User\\"

typedef struct _ROOT_KEY
{
    PCWSTR Name;
    PCWSTR MountPoint;
    HANDLE Handle;
} ROOT_KEY, *PROOT_KEY;

ROOT_KEY RootKeys[] =
{
    { L"HKCR", REGISTRY_SETUP_MACHINE L"SOFTWARE\\Classes\\", NULL },   /* "\\Registry\\Machine\\SOFTWARE\\Classes\\" */ // HKEY_CLASSES_ROOT
    { L"HKCU", REGISTRY_SETUP_USER    L".DEFAULT\\"         , NULL },   /* "\\Registry\\User\\.DEFAULT\\" */             // HKEY_CURRENT_USER
    { L"HKLM", REGISTRY_SETUP_MACHINE                       , NULL },   /* "\\Registry\\Machine\\"        */             // HKEY_LOCAL_MACHINE
    { L"HKU" , REGISTRY_SETUP_USER                          , NULL },   /* "\\Registry\\User\\"           */             // HKEY_USERS
#if 0
    { L"HKR", NULL, NULL },
#endif
};

/* FUNCTIONS ****************************************************************/

#define IsPredefKey(HKey)       \
    (((ULONG_PTR)(HKey) & 0xF0000000) == 0x80000000)

#define GetPredefKeyIndex(HKey) \
    ((ULONG_PTR)(HKey) & 0x0FFFFFFF)

HANDLE
GetRootKeyByPredefKey(
    IN HANDLE KeyHandle,
    OUT PCWSTR* RootKeyMountPoint OPTIONAL)
{
    ULONG_PTR Index = GetPredefKeyIndex(KeyHandle);

    if (!IsPredefKey(KeyHandle))
        return NULL;
    if (Index >= ARRAYSIZE(RootKeys))
        return NULL;

    if (RootKeyMountPoint)
        *RootKeyMountPoint = RootKeys[Index].MountPoint;
    return RootKeys[Index].Handle;
}

HANDLE
GetRootKeyByName(
    IN PCWSTR RootKeyName,
    OUT PCWSTR* RootKeyMountPoint OPTIONAL)
{
    UCHAR i;

    for (i = 0; i < ARRAYSIZE(RootKeys); ++i)
    {
        if (!_wcsicmp(RootKeyName, RootKeys[i].Name))
        {
            if (RootKeyMountPoint)
                *RootKeyMountPoint = RootKeys[i].MountPoint;
            return RootKeys[i].Handle;
        }
    }

    return NULL;
}


/***********************************************************************
 *            append_multi_sz_value
 *
 * Append a multisz string to a multisz registry value.
 */
// NOTE: Synced with setupapi/install.c ; see also mkhive/reginf.c
#if 0
static void
append_multi_sz_value (HANDLE hkey,
                       const WCHAR *value,
                       const WCHAR *strings,
                       DWORD str_size )
{
    DWORD size, type, total;
    WCHAR *buffer, *p;

    if (RegQueryValueExW( hkey, value, NULL, &type, NULL, &size )) return;
    if (type != REG_MULTI_SZ) return;

    if (!(buffer = HeapAlloc( GetProcessHeap(), 0, size + str_size * sizeof(WCHAR) ))) return;
    if (RegQueryValueExW( hkey, value, NULL, NULL, (BYTE *)buffer, &size )) goto done;

    /* compare each string against all the existing ones */
    total = size;
    while (*strings)
    {
        int len = strlenW(strings) + 1;

        for (p = buffer; *p; p += strlenW(p) + 1)
            if (!strcmpiW( p, strings )) break;

        if (!*p)  /* not found, need to append it */
        {
            memcpy( p, strings, len * sizeof(WCHAR) );
            p[len] = 0;
            total += len;
        }
        strings += len;
    }
    if (total != size)
    {
        TRACE( "setting value %s to %s\n", debugstr_w(value), debugstr_w(buffer) );
        RegSetValueExW( hkey, value, 0, REG_MULTI_SZ, (BYTE *)buffer, total );
    }
 done:
    HeapFree( GetProcessHeap(), 0, buffer );
}
#endif

/***********************************************************************
 *            delete_multi_sz_value
 *
 * Remove a string from a multisz registry value.
 */
#if 0
static void delete_multi_sz_value( HKEY hkey, const WCHAR *value, const WCHAR *string )
{
    DWORD size, type;
    WCHAR *buffer, *src, *dst;

    if (RegQueryValueExW( hkey, value, NULL, &type, NULL, &size )) return;
    if (type != REG_MULTI_SZ) return;
    /* allocate double the size, one for value before and one for after */
    if (!(buffer = HeapAlloc( GetProcessHeap(), 0, size * 2 * sizeof(WCHAR) ))) return;
    if (RegQueryValueExW( hkey, value, NULL, NULL, (BYTE *)buffer, &size )) goto done;
    src = buffer;
    dst = buffer + size;
    while (*src)
    {
        int len = strlenW(src) + 1;
        if (strcmpiW( src, string ))
        {
            memcpy( dst, src, len * sizeof(WCHAR) );
            dst += len;
        }
        src += len;
    }
    *dst++ = 0;
    if (dst != buffer + 2*size)  /* did we remove something? */
    {
        TRACE( "setting value %s to %s\n", debugstr_w(value), debugstr_w(buffer + size) );
        RegSetValueExW( hkey, value, 0, REG_MULTI_SZ,
                        (BYTE *)(buffer + size), dst - (buffer + size) );
    }
 done:
    HeapFree( GetProcessHeap(), 0, buffer );
}
#endif

/***********************************************************************
 *            do_reg_operation
 *
 * Perform an add/delete registry operation depending on the flags.
 */
static BOOLEAN
do_reg_operation(HANDLE KeyHandle,
                 PUNICODE_STRING ValueName,
                 PINFCONTEXT Context,
                 ULONG Flags)
{
  WCHAR EmptyStr = 0;
  ULONG Type;
  ULONG Size;

  if (Flags & FLG_ADDREG_DELVAL)  /* deletion */
    {
#if 0
      if (ValueName)
        {
          RegDeleteValueW( KeyHandle, ValueName );
        }
      else
        {
          RegDeleteKeyW( KeyHandle, NULL );
        }
#endif
      return TRUE;
    }

  if (Flags & FLG_ADDREG_KEYONLY)
    return TRUE;

#if 0
  if (Flags & (FLG_ADDREG_NOCLOBBER | FLG_ADDREG_OVERWRITEONLY))
    {
      BOOL exists = !RegQueryValueExW( hkey, ValueName, NULL, NULL, NULL, NULL );
      if (exists && (flags & FLG_ADDREG_NOCLOBBER))
        return TRUE;
      if (!exists & (flags & FLG_ADDREG_OVERWRITEONLY))
        return TRUE;
    }
#endif

  switch (Flags & FLG_ADDREG_TYPE_MASK)
    {
      case FLG_ADDREG_TYPE_SZ:
        Type = REG_SZ;
        break;

      case FLG_ADDREG_TYPE_MULTI_SZ:
        Type = REG_MULTI_SZ;
        break;

      case FLG_ADDREG_TYPE_EXPAND_SZ:
        Type = REG_EXPAND_SZ;
        break;

      case FLG_ADDREG_TYPE_BINARY:
        Type = REG_BINARY;
        break;

      case FLG_ADDREG_TYPE_DWORD:
        Type = REG_DWORD;
        break;

      case FLG_ADDREG_TYPE_NONE:
        Type = REG_NONE;
        break;

      default:
        Type = Flags >> 16;
        break;
    }

  if (!(Flags & FLG_ADDREG_BINVALUETYPE) ||
      (Type == REG_DWORD && SpInfGetFieldCount(Context) == 5))
    {
      PWCHAR Str = NULL;

      if (Type == REG_MULTI_SZ)
        {
          if (!SpInfGetMultiSzField(Context, 5, NULL, 0, &Size))
            Size = 0;

          if (Size)
            {
              Str = (WCHAR*) RtlAllocateHeap(ProcessHeap, 0, Size * sizeof(WCHAR));
              if (Str == NULL)
                return FALSE;

              SpInfGetMultiSzField(Context, 5, Str, Size, NULL);
            }

          if (Flags & FLG_ADDREG_APPEND)
            {
              if (Str == NULL)
                return TRUE;

              DPRINT1("append_multi_sz_value '%S' commented out, WHY??\n", ValueName);
//            append_multi_sz_value( hkey, value, str, size );

              RtlFreeHeap (ProcessHeap, 0, Str);
              return TRUE;
            }
          /* else fall through to normal string handling */
        }
      else
        {
          if (!SpInfGetStringField(Context, 5, NULL, 0, &Size))
            Size = 0;

          if (Size)
            {
              Str = (WCHAR*)RtlAllocateHeap(ProcessHeap, 0, Size * sizeof(WCHAR));
              if (Str == NULL)
                return FALSE;

              SpInfGetStringField(Context, 5, Str, Size, NULL);
            }
        }

      if (Type == REG_DWORD)
        {
          ULONG dw = Str ? wcstoul (Str, NULL, 0) : 0;

          DPRINT("setting dword %wZ to %lx\n", ValueName, dw);

          NtSetValueKey (KeyHandle,
                         ValueName,
                         0,
                         Type,
                         (PVOID)&dw,
                         sizeof(ULONG));
        }
      else
        {
          DPRINT("setting value %wZ to %S\n", ValueName, Str);

          if (Str)
            {
              NtSetValueKey (KeyHandle,
                             ValueName,
                             0,
                             Type,
                             (PVOID)Str,
                             Size * sizeof(WCHAR));
            }
          else
            {
              NtSetValueKey (KeyHandle,
                             ValueName,
                             0,
                             Type,
                             (PVOID)&EmptyStr,
                             sizeof(WCHAR));
            }
        }
      RtlFreeHeap (ProcessHeap, 0, Str);
    }
  else  /* get the binary data */
    {
      PUCHAR Data = NULL;

      if (!SpInfGetBinaryField(Context, 5, NULL, 0, &Size))
        Size = 0;

      if (Size)
        {
          Data = (unsigned char*) RtlAllocateHeap(ProcessHeap, 0, Size);
          if (Data == NULL)
            return FALSE;

          DPRINT("setting binary data %wZ len %lu\n", ValueName, Size);
          SpInfGetBinaryField(Context, 5, Data, Size, NULL);
        }

      NtSetValueKey (KeyHandle,
                     ValueName,
                     0,
                     Type,
                     (PVOID)Data,
                     Size);

      RtlFreeHeap (ProcessHeap, 0, Data);
    }

  return TRUE;
}

/***********************************************************************
 *            registry_callback
 *
 * Called once for each AddReg and DelReg entry in a given section.
 */
static BOOLEAN
registry_callback(HINF hInf, PCWSTR Section, BOOLEAN Delete)
{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING Name, Value;
    PUNICODE_STRING ValuePtr;
    UINT Flags;
    WCHAR Buffer[MAX_INF_STRING_LENGTH];

    INFCONTEXT Context;
    PCWSTR RootKeyName;
    HANDLE RootKeyHandle, KeyHandle;
    BOOLEAN Ok;

    Ok = SpInfFindFirstLine(hInf, Section, NULL, &Context);
    if (!Ok)
        return TRUE; /* Don't fail if the section isn't present */

    for (;Ok; Ok = SpInfFindNextLine(&Context, &Context))
    {
        /* get root */
        if (!SpInfGetStringField(&Context, 1, Buffer, sizeof(Buffer)/sizeof(WCHAR), NULL))
            continue;
        RootKeyHandle = GetRootKeyByName(Buffer, &RootKeyName);
        if (!RootKeyHandle)
            continue;

        /* get key */
        if (!SpInfGetStringField(&Context, 2, Buffer, sizeof(Buffer)/sizeof(WCHAR), NULL))
            *Buffer = 0;

        DPRINT("KeyName: <%S\\%S>\n", RootKeyName, Buffer);

        /* get flags */
        if (!SpInfGetIntField(&Context, 4, (PINT)&Flags))
            Flags = 0;

        DPRINT("Flags: %lx\n", Flags);

        RtlInitUnicodeString(&Name, Buffer);
        InitializeObjectAttributes(&ObjectAttributes,
                                   &Name,
                                   OBJ_CASE_INSENSITIVE,
                                   RootKeyHandle,
                                   NULL);

        if (Delete || (Flags & FLG_ADDREG_OVERWRITEONLY))
        {
            Status = NtOpenKey(&KeyHandle,
                               KEY_ALL_ACCESS,
                               &ObjectAttributes);
            if (!NT_SUCCESS(Status))
            {
                DPRINT1("NtOpenKey(%wZ) failed (Status %lx)\n", &Name, Status);
                continue;  /* ignore if it doesn't exist */
            }
        }
        else
        {
            Status = CreateNestedKey(&KeyHandle,
                                     KEY_ALL_ACCESS,
                                     &ObjectAttributes,
                                     REG_OPTION_NON_VOLATILE);
            if (!NT_SUCCESS(Status))
            {
                DPRINT1("CreateNestedKey(%wZ) failed (Status %lx)\n", &Name, Status);
                continue;
            }
        }

        /* get value name */
        if (SpInfGetStringField(&Context, 3, Buffer, sizeof(Buffer)/sizeof(WCHAR), NULL))
        {
            RtlInitUnicodeString(&Value, Buffer);
            ValuePtr = &Value;
        }
        else
        {
            ValuePtr = NULL;
        }

        /* and now do it */
        if (!do_reg_operation(KeyHandle, ValuePtr, &Context, Flags))
        {
            NtClose(KeyHandle);
            return FALSE;
        }

        NtClose(KeyHandle);
    }

    return TRUE;
}

BOOLEAN
ImportRegistryFile(
    IN PCWSTR SourcePath,
    IN PCWSTR FileName,
    IN PCWSTR Section,
    IN LCID LocaleId,
    IN BOOLEAN Delete)
{
    HINF hInf;
    UINT ErrorLine;
    WCHAR FileNameBuffer[MAX_PATH];

    /* Load the INF file from the installation media */
    CombinePaths(FileNameBuffer, ARRAYSIZE(FileNameBuffer), 2,
                 SourcePath, FileName);

    hInf = SpInfOpenInfFile(FileNameBuffer,
                            NULL,
                            INF_STYLE_WIN4,
                            LocaleId,
                            &ErrorLine);
    if (hInf == INVALID_HANDLE_VALUE)
    {
        DPRINT1("SpInfOpenInfFile() failed\n");
        return FALSE;
    }

#if 0
    if (!registry_callback(hInf, L"DelReg", FALSE))
    {
        DPRINT1("registry_callback() failed\n");
        SpInfCloseInfFile(hInf);
        return FALSE;
    }
#endif

    if (!registry_callback(hInf, L"AddReg", FALSE))
    {
        DPRINT1("registry_callback() failed\n");
        SpInfCloseInfFile(hInf);
        return FALSE;
    }

    if (!registry_callback(hInf, L"AddReg.NT" INF_ARCH, FALSE))
    {
        DPRINT1("registry_callback() failed\n");
        SpInfCloseInfFile(hInf);
        return FALSE;
    }

    SpInfCloseInfFile(hInf);
    return TRUE;
}


typedef enum _HIVE_UPDATE_STATE
{
    Create, // Create a new hive file and save possibly existing old one with a .old extension.
    Repair, // Re-create a new hive file and save possibly existing old one with a .brk extension.
    Update  // Hive update, do not need to be recreated.
} HIVE_UPDATE_STATE;

typedef struct _HIVE_LIST_ENTRY
{
    PCWSTR HiveName;            // HiveFileName;
    PCWSTR HiveRegistryPath;    // HiveRegMountPoint;
    HANDLE PredefKeyHandle;
    PCWSTR RegSymLink;
    HIVE_UPDATE_STATE State;
    // PUCHAR SecurityDescriptor;
    // ULONG  SecurityDescriptorLength;
} HIVE_LIST_ENTRY, *PHIVE_LIST_ENTRY;

#define NUMBER_OF_STANDARD_REGISTRY_HIVES   3

HIVE_LIST_ENTRY RegistryHives[/*NUMBER_OF_STANDARD_REGISTRY_HIVES*/] =
{
    { L"SYSTEM"  , L"\\Registry\\Machine\\USetup_SYSTEM"  , HKEY_LOCAL_MACHINE, L"SYSTEM"  , Create /* , SystemSecurity  , sizeof(SystemSecurity)   */ },
    { L"SOFTWARE", L"\\Registry\\Machine\\USetup_SOFTWARE", HKEY_LOCAL_MACHINE, L"SOFTWARE", Create /* , SoftwareSecurity, sizeof(SoftwareSecurity) */ },
    { L"DEFAULT" , L"\\Registry\\User\\USetup_DEFAULT"    , HKEY_USERS        , L".DEFAULT", Create /* , SystemSecurity  , sizeof(SystemSecurity)   */ },

//  { L"BCD"     , L"\\Registry\\Machine\\USetup_BCD", HKEY_LOCAL_MACHINE, L"BCD00000000", Create /* , BcdSecurity     , sizeof(BcdSecurity)      */ },
};
C_ASSERT(_countof(RegistryHives) == NUMBER_OF_STANDARD_REGISTRY_HIVES);

#define NUMBER_OF_SECURITY_REGISTRY_HIVES   2

/** These hives are created by LSASS during 2nd stage setup */
HIVE_LIST_ENTRY SecurityRegistryHives[/*NUMBER_OF_SECURITY_REGISTRY_HIVES*/] =
{
    { L"SAM"     , L"\\Registry\\Machine\\USetup_SAM"     , HKEY_LOCAL_MACHINE, L"SAM"     , Create /* , SystemSecurity  , sizeof(SystemSecurity)   */ },
    { L"SECURITY", L"\\Registry\\Machine\\USetup_SECURITY", HKEY_LOCAL_MACHINE, L"SECURITY", Create /* , NULL            , 0                        */ },
};
C_ASSERT(_countof(SecurityRegistryHives) == NUMBER_OF_SECURITY_REGISTRY_HIVES);


NTSTATUS
VerifyRegistryHives(
    IN PUNICODE_STRING NtSystemRoot,
    OUT PBOOLEAN ShouldRepairRegistry)
{
    NTSTATUS Status;
    BOOLEAN PrivilegeSet[2] = {FALSE, FALSE};
    UINT i;

    /* Suppose first the registry hives do not have to be fully recreated */
    *ShouldRepairRegistry = FALSE;

    /* Acquire restore privilege */
    Status = RtlAdjustPrivilege(SE_RESTORE_PRIVILEGE, TRUE, FALSE, &PrivilegeSet[0]);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("RtlAdjustPrivilege(SE_RESTORE_PRIVILEGE) failed (Status 0x%08lx)\n", Status);
        /* Exit prematurely here.... */
        return Status;
    }

    /* Acquire backup privilege */
    Status = RtlAdjustPrivilege(SE_BACKUP_PRIVILEGE, TRUE, FALSE, &PrivilegeSet[1]);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("RtlAdjustPrivilege(SE_BACKUP_PRIVILEGE) failed (Status 0x%08lx)\n", Status);
        RtlAdjustPrivilege(SE_RESTORE_PRIVILEGE, PrivilegeSet[0], FALSE, &PrivilegeSet[0]);
        /* Exit prematurely here.... */
        return Status;
    }

    for (i = 0; i < ARRAYSIZE(RegistryHives); ++i)
    {
        Status = VerifyRegistryHive(NtSystemRoot, RegistryHives[i].HiveName);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("Registry hive '%S' needs repair!\n", RegistryHives[i].HiveName);
            RegistryHives[i].State = Repair;
            *ShouldRepairRegistry = TRUE;
        }
        else
        {
            RegistryHives[i].State = Update;
        }
    }

    /** These hives are created by LSASS during 2nd stage setup */
    for (i = 0; i < ARRAYSIZE(SecurityRegistryHives); ++i)
    {
        Status = VerifyRegistryHive(NtSystemRoot, SecurityRegistryHives[i].HiveName);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("Registry hive '%S' needs repair!\n", SecurityRegistryHives[i].HiveName);
            SecurityRegistryHives[i].State = Repair;
            /*
             * Note that it's not the role of the 1st-stage installer to fix
             * the security hives. This should be done at 2nd-stage installation
             * by LSASS.
             */
        }
        else
        {
            SecurityRegistryHives[i].State = Update;
        }
    }

    /* Reset the status (we succeeded in checking all the hives) */
    Status = STATUS_SUCCESS;

    /* Remove restore and backup privileges */
    RtlAdjustPrivilege(SE_BACKUP_PRIVILEGE, PrivilegeSet[1], FALSE, &PrivilegeSet[1]);
    RtlAdjustPrivilege(SE_RESTORE_PRIVILEGE, PrivilegeSet[0], FALSE, &PrivilegeSet[0]);

    return Status;
}

NTSTATUS
RegInitializeRegistry(
    IN PUNICODE_STRING NtSystemRoot)
{
    NTSTATUS Status;
    HANDLE KeyHandle;
    UNICODE_STRING KeyName;
    OBJECT_ATTRIBUTES ObjectAttributes;
    BOOLEAN PrivilegeSet[2] = {FALSE, FALSE};
    ULONG Disposition;
    UINT i;

    /* Acquire restore privilege */
    Status = RtlAdjustPrivilege(SE_RESTORE_PRIVILEGE, TRUE, FALSE, &PrivilegeSet[0]);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("RtlAdjustPrivilege(SE_RESTORE_PRIVILEGE) failed (Status 0x%08lx)\n", Status);
        /* Exit prematurely here.... */
        return Status;
    }

    /* Acquire backup privilege */
    Status = RtlAdjustPrivilege(SE_BACKUP_PRIVILEGE, TRUE, FALSE, &PrivilegeSet[1]);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("RtlAdjustPrivilege(SE_BACKUP_PRIVILEGE) failed (Status 0x%08lx)\n", Status);
        RtlAdjustPrivilege(SE_RESTORE_PRIVILEGE, PrivilegeSet[0], FALSE, &PrivilegeSet[0]);
        /* Exit prematurely here.... */
        return Status;
    }

    /*
     * Create the template proto-hive.
     *
     * Use a dummy root key name:
     * - On 2k/XP/2k3, this is "$$$PROTO.HIV"
     * - On Vista+, this is "CMI-CreateHive{guid}"
     * See https://github.com/libyal/winreg-kb/blob/main/docs/sources/windows-registry/Files.md
     * for more information.
     */
    RtlInitUnicodeString(&KeyName, L"\\Registry\\Machine\\SYSTEM\\$$$PROTO.HIV");
    InitializeObjectAttributes(&ObjectAttributes,
                               &KeyName,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);
    Status = NtCreateKey(&KeyHandle,
                         KEY_ALL_ACCESS,
                         &ObjectAttributes,
                         0,
                         NULL,
                         REG_OPTION_NON_VOLATILE,
                         NULL);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtCreateKey() failed to create the proto-hive (Status %lx)\n", Status);
        goto Quit;
    }
    NtFlushKey(KeyHandle);

    for (i = 0; i < ARRAYSIZE(RegistryHives); ++i)
    {
        if (RegistryHives[i].State != Create && RegistryHives[i].State != Repair)
            continue;

        Status = CreateRegistryFile(NtSystemRoot,
                                    RegistryHives[i].HiveName,
                                    RegistryHives[i].State != Repair, // RegistryHives[i].State == Create,
                                    KeyHandle);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("CreateRegistryFile(%S) failed, Status 0x%08lx\n", RegistryHives[i].HiveName, Status);
            /* Exit prematurely here.... */
            /* That is now done, remove the proto-hive */
            NtDeleteKey(KeyHandle);
            NtClose(KeyHandle);
            goto Quit;
        }
    }

    /* That is now done, remove the proto-hive */
    NtDeleteKey(KeyHandle);
    NtClose(KeyHandle);


    /*
     * Prepare the registry root keys. Since we cannot create real registry keys
     * inside the master keys (\Registry, \Registry\Machine or \Registry\User),
     * we need to perform some SymLink tricks instead.
     */

    /* Our offline HKLM '\Registry\Machine' is inside '\Registry\Machine\SYSTEM\USetup_Machine' */
    RtlInitUnicodeString(&KeyName, RootKeys[GetPredefKeyIndex(HKEY_LOCAL_MACHINE)].MountPoint);
    InitializeObjectAttributes(&ObjectAttributes,
                               &KeyName,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);
    KeyHandle = NULL;
    Status = NtCreateKey(&KeyHandle,
                         KEY_ALL_ACCESS,
                         &ObjectAttributes,
                         0,
                         NULL,
                    // FIXME: Using REG_OPTION_VOLATILE works OK on Windows,
                    // but I need to check whether it works OK on ReactOS too.
                         REG_OPTION_NON_VOLATILE, // REG_OPTION_VOLATILE,
                         &Disposition);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtCreateKey(%wZ) failed (Status 0x%08lx)\n", &KeyName, Status);
        // return Status;
    }
    RootKeys[GetPredefKeyIndex(HKEY_LOCAL_MACHINE)].Handle = KeyHandle;

    /* Our offline HKU '\Registry\User' is inside '\Registry\Machine\SYSTEM\USetup_User' */
    RtlInitUnicodeString(&KeyName, RootKeys[GetPredefKeyIndex(HKEY_USERS)].MountPoint);
    InitializeObjectAttributes(&ObjectAttributes,
                               &KeyName,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);
    KeyHandle = NULL;
    Status = NtCreateKey(&KeyHandle,
                         KEY_ALL_ACCESS,
                         &ObjectAttributes,
                         0,
                         NULL,
                    // FIXME: Using REG_OPTION_VOLATILE works OK on Windows,
                    // but I need to check whether it works OK on ReactOS too.
                         REG_OPTION_NON_VOLATILE, // REG_OPTION_VOLATILE,
                         &Disposition);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtCreateKey(%wZ) failed (Status 0x%08lx)\n", &KeyName, Status);
        // return Status;
    }
    RootKeys[GetPredefKeyIndex(HKEY_USERS)].Handle = KeyHandle;


    /*
     * Now properly mount the offline hive files
     */
    for (i = 0; i < ARRAYSIZE(RegistryHives); ++i)
    {
        // if (RegistryHives[i].State != Create && RegistryHives[i].State != Repair)
            // continue;

        if (RegistryHives[i].State == Create || RegistryHives[i].State == Repair)
        {
            Status = ConnectRegistry(NULL,
                                     RegistryHives[i].HiveRegistryPath,
                                     NtSystemRoot,
                                     RegistryHives[i].HiveName
                                     /* SystemSecurity, sizeof(SystemSecurity) */);
            if (!NT_SUCCESS(Status))
            {
                DPRINT1("ConnectRegistry(%S) failed, Status 0x%08lx\n",
                        RegistryHives[i].HiveName, Status);
            }

            /* Create the registry symlink to this key */
            Status = CreateSymLinkKey(RootKeys[GetPredefKeyIndex(RegistryHives[i].PredefKeyHandle)].Handle,
                                      RegistryHives[i].RegSymLink,
                                      RegistryHives[i].HiveRegistryPath);
            if (!NT_SUCCESS(Status))
            {
                DPRINT1("CreateSymLinkKey(%S) failed, Status 0x%08lx\n",
                        RegistryHives[i].RegSymLink, Status);
            }
        }
        else
        {
            /* Create *DUMMY* volatile hives just to make the update procedure working */

            RtlInitUnicodeString(&KeyName, RegistryHives[i].RegSymLink);
            InitializeObjectAttributes(&ObjectAttributes,
                                       &KeyName,
                                       OBJ_CASE_INSENSITIVE,
                                       RootKeys[GetPredefKeyIndex(RegistryHives[i].PredefKeyHandle)].Handle,
                                       NULL);
            KeyHandle = NULL;
            Status = NtCreateKey(&KeyHandle,
                                 KEY_ALL_ACCESS,
                                 &ObjectAttributes,
                                 0,
                                 NULL,
                            // FIXME: Using REG_OPTION_VOLATILE works OK on Windows,
                            // but I need to check whether it works OK on ReactOS too.
                                 REG_OPTION_NON_VOLATILE, // REG_OPTION_VOLATILE,
                                 &Disposition);
            if (!NT_SUCCESS(Status))
            {
                DPRINT1("NtCreateKey(%wZ) failed (Status 0x%08lx)\n", &KeyName, Status);
                // return Status;
            }
            NtClose(KeyHandle);
        }
    }


    /* HKCU is a handle to 'HKU\.DEFAULT' */
#if 0
    RtlInitUnicodeString(&KeyName, L".DEFAULT");
    InitializeObjectAttributes(&ObjectAttributes,
                               &KeyName,
                               OBJ_CASE_INSENSITIVE,
                               RootKeys[GetPredefKeyIndex(HKEY_USERS)].Handle,
                               NULL);
#else
    RtlInitUnicodeString(&KeyName, RootKeys[GetPredefKeyIndex(HKEY_CURRENT_USER)].MountPoint);
    InitializeObjectAttributes(&ObjectAttributes,
                               &KeyName,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);
#endif
    KeyHandle = NULL;
    Status = NtOpenKey(&KeyHandle,
                       KEY_ALL_ACCESS,
                       &ObjectAttributes);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtOpenKey(%wZ) failed (Status %lx)\n", &KeyName, Status);
    }
    RootKeys[GetPredefKeyIndex(HKEY_CURRENT_USER)].Handle = KeyHandle;


    /* HKCR is a handle to 'HKLM\Software\Classes' */
#if 0
    RtlInitUnicodeString(&KeyName, L"Software\\Classes");
    InitializeObjectAttributes(&ObjectAttributes,
                               &KeyName,
                               OBJ_CASE_INSENSITIVE,
                               RootKeys[GetPredefKeyIndex(HKEY_LOCAL_MACHINE)].Handle,
                               NULL);
#else
    RtlInitUnicodeString(&KeyName, RootKeys[GetPredefKeyIndex(HKEY_CLASSES_ROOT)].MountPoint);
    InitializeObjectAttributes(&ObjectAttributes,
                               &KeyName,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);
#endif
    KeyHandle = NULL;
    /* We use NtCreateKey instead of NtOpenKey because Software\Classes doesn't exist originally */
    Status = NtCreateKey(&KeyHandle,
                         KEY_ALL_ACCESS,
                         &ObjectAttributes,
                         0,
                         NULL,
                         REG_OPTION_NON_VOLATILE,
                         &Disposition);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtCreateKey(%wZ) failed (Status %lx)\n", &KeyName, Status);
    }
    else
    {
        DPRINT("NtCreateKey() succeeded to %s the %wZ key (Status %lx)\n",
               Disposition == REG_CREATED_NEW_KEY ? "create" : /* REG_OPENED_EXISTING_KEY */ "open",
               &KeyName, Status);
    }
    RootKeys[GetPredefKeyIndex(HKEY_CLASSES_ROOT)].Handle = KeyHandle;


    Status = STATUS_SUCCESS;


    /* Create the 'HKLM\SYSTEM\ControlSet001' key */
    // REGISTRY_SETUP_MACHINE L"SYSTEM\\ControlSet001"
    RtlInitUnicodeString(&KeyName, L"SYSTEM\\ControlSet001");
    InitializeObjectAttributes(&ObjectAttributes,
                               &KeyName,
                               OBJ_CASE_INSENSITIVE,
                               RootKeys[GetPredefKeyIndex(HKEY_LOCAL_MACHINE)].Handle,
                               NULL);
    Status = NtCreateKey(&KeyHandle,
                         KEY_ALL_ACCESS,
                         &ObjectAttributes,
                         0,
                         NULL,
                         REG_OPTION_NON_VOLATILE,
                         &Disposition);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtCreateKey() failed to create the ControlSet001 key (Status %lx)\n", Status);
        // return Status;
    }
    else
    {
        DPRINT("NtCreateKey() succeeded to %s the ControlSet001 key (Status %lx)\n",
               Disposition == REG_CREATED_NEW_KEY ? "create" : /* REG_OPENED_EXISTING_KEY */ "open",
               Status);
    }
    NtClose(KeyHandle);

    /* Create the 'HKLM\SYSTEM\CurrentControlSet' symlink */
    Status = CreateSymLinkKey(RootKeys[GetPredefKeyIndex(HKEY_LOCAL_MACHINE)].Handle,
                              L"SYSTEM\\CurrentControlSet",
                              REGISTRY_SETUP_MACHINE L"SYSTEM\\ControlSet001");
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("CreateSymLinkKey(CurrentControlSet) failed, Status 0x%08lx\n", Status);
    }


    Status = STATUS_SUCCESS;


Quit:
    /* Remove restore and backup privileges */
    RtlAdjustPrivilege(SE_BACKUP_PRIVILEGE, PrivilegeSet[1], FALSE, &PrivilegeSet[1]);
    RtlAdjustPrivilege(SE_RESTORE_PRIVILEGE, PrivilegeSet[0], FALSE, &PrivilegeSet[0]);

    return Status;
}

VOID
RegCleanupRegistry(
    IN PUNICODE_STRING NtSystemRoot)
{
    NTSTATUS Status;
    HANDLE KeyHandle;
    UNICODE_STRING KeyName;
    OBJECT_ATTRIBUTES ObjectAttributes;
    BOOLEAN PrivilegeSet[2] = {FALSE, FALSE};
    UINT i;
    WCHAR SrcPath[MAX_PATH];
    WCHAR DstPath[MAX_PATH];

    /* Acquire restore privilege */
    Status = RtlAdjustPrivilege(SE_RESTORE_PRIVILEGE, TRUE, FALSE, &PrivilegeSet[0]);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("RtlAdjustPrivilege(SE_RESTORE_PRIVILEGE) failed (Status 0x%08lx)\n", Status);
        /* Exit prematurely here.... */
        return;
    }

    /* Acquire backup privilege */
    Status = RtlAdjustPrivilege(SE_BACKUP_PRIVILEGE, TRUE, FALSE, &PrivilegeSet[1]);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("RtlAdjustPrivilege(SE_BACKUP_PRIVILEGE) failed (Status 0x%08lx)\n", Status);
        RtlAdjustPrivilege(SE_RESTORE_PRIVILEGE, PrivilegeSet[0], FALSE, &PrivilegeSet[0]);
        /* Exit prematurely here.... */
        return;
    }

    /*
     * To keep the running system clean we need first to remove the symlinks
     * we have created and then unmounting the hives. Finally we delete the
     * master registry keys.
     */

    for (i = 0; i < ARRAYSIZE(RegistryHives); ++i)
    {
        if (RegistryHives[i].State == Create || RegistryHives[i].State == Repair)
        {
            /* Delete the registry symlink to this key */
            Status = DeleteSymLinkKey(RootKeys[GetPredefKeyIndex(RegistryHives[i].PredefKeyHandle)].Handle,
                                      RegistryHives[i].RegSymLink);
            if (!NT_SUCCESS(Status))
            {
                DPRINT1("DeleteSymLinkKey(%S) failed, Status 0x%08lx\n",
                        RegistryHives[i].RegSymLink, Status);
            }

            /* Unmount the hive */
            Status = DisconnectRegistry(NULL,
                                        RegistryHives[i].HiveRegistryPath,
                                        1 /* REG_FORCE_UNLOAD */);
            if (!NT_SUCCESS(Status))
            {
                DPRINT1("Unmounting '%S' failed\n", RegistryHives[i].HiveRegistryPath);
            }

            /* Switch the hive state to 'Update' */
            RegistryHives[i].State = Update;
        }
        else
        {
            /* Delete the *DUMMY* volatile hives created for the update procedure */

            RtlInitUnicodeString(&KeyName, RegistryHives[i].RegSymLink);
            InitializeObjectAttributes(&ObjectAttributes,
                                       &KeyName,
                                       OBJ_CASE_INSENSITIVE,
                                       RootKeys[GetPredefKeyIndex(RegistryHives[i].PredefKeyHandle)].Handle,
                                       NULL);
            KeyHandle = NULL;
            Status = NtOpenKey(&KeyHandle,
                               DELETE,
                               &ObjectAttributes);
            if (!NT_SUCCESS(Status))
            {
                DPRINT1("NtOpenKey(%wZ) failed, Status 0x%08lx\n", &KeyName, Status);
                // return;
            }

            NtDeleteKey(KeyHandle);
            NtClose(KeyHandle);
        }
    }

    /*
     * FIXME: Once force-unloading keys is correctly fixed, I'll fix
     * this code that closes some of the registry keys that were opened
     * inside the hives we've just unmounted above...
     */

    /* Remove the registry root keys */
    for (i = 0; i < ARRAYSIZE(RootKeys); ++i)
    {
        if (RootKeys[i].Handle)
        {
            /**/NtFlushKey(RootKeys[i].Handle);/**/ // FIXME: Why does it hang? Answer: because we have some problems in CMAPI!
            NtDeleteKey(RootKeys[i].Handle);
            NtClose(RootKeys[i].Handle);
            RootKeys[i].Handle = NULL;
        }
    }

    //
    // RegBackupRegistry()
    //
    /* Now backup the hives into .sav files */
    for (i = 0; i < ARRAYSIZE(RegistryHives); ++i)
    {
        if (RegistryHives[i].State != Create && RegistryHives[i].State != Repair)
            continue;

        CombinePaths(SrcPath, ARRAYSIZE(SrcPath), 3,
                     NtSystemRoot->Buffer, L"System32\\config", RegistryHives[i].HiveName);
        RtlStringCchCopyW(DstPath, ARRAYSIZE(DstPath), SrcPath);
        RtlStringCchCatW(DstPath, ARRAYSIZE(DstPath), L".sav");

        DPRINT1("Copy hive: %S ==> %S\n", SrcPath, DstPath);
        Status = SetupCopyFile(SrcPath, DstPath, FALSE);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("SetupCopyFile() failed (Status %lx)\n", Status);
            // return Status;
        }
    }

    /* Remove restore and backup privileges */
    RtlAdjustPrivilege(SE_BACKUP_PRIVILEGE, PrivilegeSet[1], FALSE, &PrivilegeSet[1]);
    RtlAdjustPrivilege(SE_RESTORE_PRIVILEGE, PrivilegeSet[0], FALSE, &PrivilegeSet[0]);
}

/* EOF */
