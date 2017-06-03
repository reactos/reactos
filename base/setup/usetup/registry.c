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
 * PROJECT:         ReactOS text-mode setup
 * FILE:            base/setup/usetup/registry.c
 * PURPOSE:         Registry creation functions
 * PROGRAMMER:      Eric Kohl
 */

/* INCLUDES *****************************************************************/

#include "usetup.h"

#define NDEBUG
#include <debug.h>

#ifdef __REACTOS__
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

#ifdef _M_IX86
#define Architecture L"x86"
#elif defined(_M_AMD64)
#define Architecture L"amd64"
#elif defined(_M_IA64)
#define Architecture L"ia64"
#elif defined(_M_ARM)
#define Architecture L"arm"
#elif defined(_M_PPC)
#define Architecture L"ppc"
#endif

/* FUNCTIONS ****************************************************************/

typedef struct _ROOT_KEY
{
    PCWSTR Name;
    PCWSTR MountPoint;
    HANDLE Handle;
} ROOT_KEY, *PROOT_KEY;

ROOT_KEY RootKeys[] =
{
    // L"\\Registry\\Machine\\SYSTEM\\USetup_Machine\\SOFTWARE\\Classes\\"
    { L"HKCR", L"\\Registry\\Machine\\USetup_SOFTWARE\\Classes\\", NULL },    /* "\\Registry\\Machine\\SOFTWARE\\Classes\\" */  // HKEY_CLASSES_ROOT
    { L"HKCU", L"\\Registry\\User\\USetup_DEFAULT\\"             , NULL },    /* "\\Registry\\User\\.DEFAULT\\" */              // HKEY_CURRENT_USER
    { L"HKLM", L"\\Registry\\Machine\\SYSTEM\\USetup_Machine\\"  , NULL },    /* "\\Registry\\Machine\\"        */              // HKEY_LOCAL_MACHINE
    { L"HKU" , L"\\Registry\\Machine\\SYSTEM\\USetup_User\\"     , NULL },    /* "\\Registry\\User\\"           */              // HKEY_USERS
#if 0
    { L"HKR", NULL, NULL },
#endif
};

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
    if (GetPredefKeyIndex(KeyHandle) >= ARRAYSIZE(RootKeys))
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
      (Type == REG_DWORD && SetupGetFieldCount (Context) == 5))
    {
      PWCHAR Str = NULL;

      if (Type == REG_MULTI_SZ)
        {
          if (!SetupGetMultiSzFieldW (Context, 5, NULL, 0, &Size))
            Size = 0;

          if (Size)
            {
              Str = (WCHAR*) RtlAllocateHeap(ProcessHeap, 0, Size * sizeof(WCHAR));
              if (Str == NULL)
                return FALSE;

              SetupGetMultiSzFieldW (Context, 5, Str, Size, NULL);
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
          if (!SetupGetStringFieldW(Context, 5, NULL, 0, &Size))
            Size = 0;

          if (Size)
            {
              Str = (WCHAR*)RtlAllocateHeap(ProcessHeap, 0, Size * sizeof(WCHAR));
              if (Str == NULL)
                return FALSE;

              SetupGetStringFieldW(Context, 5, Str, Size, NULL);
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

      if (!SetupGetBinaryField (Context, 5, NULL, 0, &Size))
        Size = 0;

      if (Size)
        {
          Data = (unsigned char*) RtlAllocateHeap(ProcessHeap, 0, Size);
          if (Data == NULL)
            return FALSE;

          DPRINT("setting binary data %wZ len %lu\n", ValueName, Size);
          SetupGetBinaryField (Context, 5, Data, Size, NULL);
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

NTSTATUS
CreateNestedKey(PHANDLE KeyHandle,
                ACCESS_MASK DesiredAccess,
                POBJECT_ATTRIBUTES ObjectAttributes)
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
                         0,
                         &Disposition);
    DPRINT("NtCreateKey(%wZ) called (Status %lx)\n", ObjectAttributes->ObjectName, Status);
    if (Status != STATUS_OBJECT_NAME_NOT_FOUND)
        return Status;

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
        Ptr = wcsrchr (LocalKeyName.Buffer, '\\');
        if (Ptr == NULL || Ptr == LocalKeyName.Buffer)
        {
            Status = STATUS_UNSUCCESSFUL;
            break;
        }
        *Ptr = (WCHAR)0;
        LocalKeyName.Length = wcslen (LocalKeyName.Buffer) * sizeof(WCHAR);

        Status = NtCreateKey(&LocalKeyHandle,
                             KEY_ALL_ACCESS,
                             &LocalObjectAttributes,
                             0,
                             NULL,
                             0,
                             &Disposition);
        DPRINT("NtCreateKey(%wZ) called (Status %lx)\n", &LocalKeyName, Status);
    }

    if (!NT_SUCCESS(Status))
    {
        RtlFreeUnicodeString (&LocalKeyName);
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
        LocalKeyName.Length = wcslen (LocalKeyName.Buffer) * sizeof(WCHAR);

        Status = NtCreateKey(&LocalKeyHandle,
                             KEY_ALL_ACCESS,
                             &LocalObjectAttributes,
                             0,
                             NULL,
                             0,
                             &Disposition);
        DPRINT("NtCreateKey(%wZ) called (Status %lx)\n", &LocalKeyName, Status);
        if (!NT_SUCCESS(Status))
            break;
    }

    RtlFreeUnicodeString(&LocalKeyName);

    return Status;
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

    Ok = SetupFindFirstLineW(hInf, Section, NULL, &Context);
    if (!Ok)
        return TRUE; /* Don't fail if the section isn't present */

    for (;Ok; Ok = SetupFindNextLine (&Context, &Context))
    {
        /* get root */
        if (!SetupGetStringFieldW(&Context, 1, Buffer, sizeof(Buffer)/sizeof(WCHAR), NULL))
            continue;
        RootKeyHandle = GetRootKeyByName(Buffer, &RootKeyName);
        if (!RootKeyHandle)
            continue;

        /* get key */
        if (!SetupGetStringFieldW(&Context, 2, Buffer, sizeof(Buffer)/sizeof(WCHAR), NULL))
            *Buffer = 0;

        DPRINT("KeyName: <%S\\%S>\n", RootKeyName, Buffer);

        /* get flags */
        if (!SetupGetIntField(&Context, 4, (PINT)&Flags))
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
                                     &ObjectAttributes);
            if (!NT_SUCCESS(Status))
            {
                DPRINT1("CreateNestedKey(%wZ) failed (Status %lx)\n", &Name, Status);
                continue;
            }
        }

        /* get value name */
        if (SetupGetStringFieldW(&Context, 3, Buffer, sizeof(Buffer)/sizeof(WCHAR), NULL))
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
    PWSTR Filename,
    PWSTR Section,
    LCID LocaleId,
    BOOLEAN Delete)
{
    WCHAR FileNameBuffer[MAX_PATH];
    HINF hInf;
    UINT ErrorLine;

    /* Load inf file from install media. */
    CombinePaths(FileNameBuffer, ARRAYSIZE(FileNameBuffer), 2,
                 SourcePath.Buffer, Filename);

    hInf = SetupOpenInfFileW(FileNameBuffer,
                             NULL,
                             INF_STYLE_WIN4,
                             LocaleId,
                             &ErrorLine);
    if (hInf == INVALID_HANDLE_VALUE)
    {
        DPRINT1("SetupOpenInfFile() failed\n");
        return FALSE;
    }

#if 0
    if (!registry_callback(hInf, L"DelReg", FALSE))
    {
        DPRINT1("registry_callback() failed\n");
        InfCloseFile(hInf);
        return FALSE;
    }
#endif

    if (!registry_callback(hInf, L"AddReg", FALSE))
    {
        DPRINT1("registry_callback() failed\n");
        InfCloseFile(hInf);
        return FALSE;
    }

    if (!registry_callback(hInf, L"AddReg.NT" Architecture, FALSE))
    {
        DPRINT1("registry_callback() failed\n");
        InfCloseFile(hInf);
        return FALSE;
    }

    InfCloseFile(hInf);
    return TRUE;
}

/*
 * Should be called under privileges
 */
// static
NTSTATUS
CreateRegistryFile(
    IN PUNICODE_STRING InstallPath,
    IN PCWSTR RegistryKey,
    IN HANDLE ProtoKeyHandle)
{
    NTSTATUS Status;
    HANDLE FileHandle;
    UNICODE_STRING FileName;
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatusBlock;
    WCHAR PathBuffer[MAX_PATH];

    /* Create the file */
    CombinePaths(PathBuffer, ARRAYSIZE(PathBuffer), 3,
                 InstallPath->Buffer, L"System32\\config", RegistryKey);
    RtlInitUnicodeString(&FileName, PathBuffer);
    InitializeObjectAttributes(&ObjectAttributes,
                               &FileName,
                               OBJ_CASE_INSENSITIVE,
                               NULL, // Could have been installpath, etc...
                               NULL);

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

static BOOLEAN
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
 * Should be called under privileges
 */
// static
NTSTATUS
ConnectRegistry(
    IN HKEY RootKey OPTIONAL,
    // IN HANDLE RootDirectory OPTIONAL,
    IN PUNICODE_STRING InstallPath,
    IN PCWSTR RegistryKey,
    // IN PUCHAR Descriptor,
    // IN ULONG DescriptorLength,
    IN PCWSTR RegMountPoint)
{
    NTSTATUS Status;
    UNICODE_STRING KeyName, FileName;
    OBJECT_ATTRIBUTES KeyObjectAttributes;
    OBJECT_ATTRIBUTES FileObjectAttributes;
    WCHAR PathBuffer[MAX_PATH];

    RtlInitUnicodeString(&KeyName, RegMountPoint);
    InitializeObjectAttributes(&KeyObjectAttributes,
                               &KeyName,
                               OBJ_CASE_INSENSITIVE,
                               RootKey,
                               NULL);

    CombinePaths(PathBuffer, ARRAYSIZE(PathBuffer), 3,
                 InstallPath->Buffer, L"System32\\config", RegistryKey);
    RtlInitUnicodeString(&FileName, PathBuffer);
    InitializeObjectAttributes(&FileObjectAttributes,
                               &FileName,
                               OBJ_CASE_INSENSITIVE,
                               NULL, // RootDirectory,
                               NULL);

#if 0
    IN PCMHIVE HiveToConnect;
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
#endif

    /* Mount the registry hive in the registry namespace */
    Status = NtLoadKey(&KeyObjectAttributes, &FileObjectAttributes);

    return Status;
}

NTSTATUS
RegInitializeRegistry(
    IN PUNICODE_STRING InstallPath)
{
    NTSTATUS Status;
    HANDLE KeyHandle;
    UNICODE_STRING KeyName;
    OBJECT_ATTRIBUTES ObjectAttributes;
    BOOLEAN PrivilegeSet[2] = {FALSE, FALSE};
    ULONG Disposition;
    UINT i;
    PCWSTR RegistryKeys[] =
    {
        L"SYSTEM",
        L"SOFTWARE",
        L"DEFAULT", // L".DEFAULT",
        // L"SAM",
        // L"SECURITY",
        // L"BCD00000000",
    };

#if 0
    /* Initialize the current session registry */
    Status = NtInitializeRegistry(CM_BOOT_FLAG_SETUP);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtInitializeRegistry() failed (Status %lx)\n", Status);
        return Status;
    }
#endif

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
     * See https://github.com/libyal/winreg-kb/blob/master/documentation/Registry%20files.asciidoc
     * for more information.
     */
    RtlInitUnicodeString(&KeyName,
                         L"\\Registry\\Machine\\SYSTEM\\$$$PROTO.HIV");
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

    for (i = 0; i < ARRAYSIZE(RegistryKeys); ++i)
    {
        Status = CreateRegistryFile(InstallPath,
                                    RegistryKeys[i],
                                    KeyHandle);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("CreateRegistryFile(%S) failed, Status 0x%08lx\n", RegistryKeys[i], Status);
            /* Exit prematurely here.... */
            /* That is now done, clean everything up! */
            NtDeleteKey(KeyHandle);
            NtClose(KeyHandle);
            goto Quit;
        }
    }

    /* That is now done, clean everything up! */
    NtDeleteKey(KeyHandle);
    NtClose(KeyHandle);


    /*
     * Prepare the installation roots. Since we cannot create real registry keys
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
                         REG_OPTION_VOLATILE,
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
                         REG_OPTION_VOLATILE,
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

    /* Create SYSTEM key */
    Status =
    ConnectRegistry(NULL,
                    InstallPath,
                    RegistryKeys[0],
                    // SystemSecurity, sizeof(SystemSecurity),
                    L"\\Registry\\Machine\\USetup_SYSTEM");
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("ConnectRegistry(SYSTEM) failed, Status 0x%08lx\n", Status);
    }

    /* Create the 'HKLM\SYSTEM' symlink to this key */
    if (!CmpLinkKeyToHive(RootKeys[GetPredefKeyIndex(HKEY_LOCAL_MACHINE)].Handle,
                          L"SYSTEM",
                          L"\\Registry\\Machine\\USetup_SYSTEM"))
    {
        DPRINT1("CmpLinkKeyToHive(SYSTEM) failed!\n");
    }


    /* Create SOFTWARE key */
    Status =
    ConnectRegistry(NULL,
                    InstallPath,
                    RegistryKeys[1],
                    // SoftwareSecurity, sizeof(SoftwareSecurity),
                    L"\\Registry\\Machine\\USetup_SOFTWARE");
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("ConnectRegistry(SOFTWARE) failed, Status 0x%08lx\n", Status);
    }

    /* Create the 'HKLM\Software' symlink to this key */
    if (!CmpLinkKeyToHive(RootKeys[GetPredefKeyIndex(HKEY_LOCAL_MACHINE)].Handle,
                          L"Software",
                          L"\\Registry\\Machine\\USetup_SOFTWARE"))
    {
        DPRINT1("CmpLinkKeyToHive(SOFTWARE) failed!\n");
    }


    /* Create DEFAULT key */
    Status =
    ConnectRegistry(NULL,
                    InstallPath,
                    RegistryKeys[2],
                    // SystemSecurity, sizeof(SystemSecurity),
                    L"\\Registry\\User\\USetup_DEFAULT");
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("ConnectRegistry(DEFAULT) failed, Status 0x%08lx\n", Status);
    }

    /* Create the 'HKU\.DEFAULT' symlink to this key */
    if (!CmpLinkKeyToHive(RootKeys[GetPredefKeyIndex(HKEY_USERS)].Handle,
                          L".DEFAULT",
                          L"\\Registry\\User\\USetup_DEFAULT"))
    {
        DPRINT1("CmpLinkKeyToHive(DEFAULT) failed!\n");
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
    Status = NtCreateKey(&KeyHandle,
                         KEY_ALL_ACCESS,
                         &ObjectAttributes,
                         0,
                         NULL,
                         0,
                         &Disposition);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtCreateKey(%wZ) failed (Status %lx)\n", &KeyName, Status);
    }
    else
    {
        DPRINT1("NtCreateKey() succeeded to %s the %wZ key (Status %lx)\n",
                Disposition == REG_CREATED_NEW_KEY ? "create" : /* REG_OPENED_EXISTING_KEY */ "open",
                &KeyName, Status);
    }
    RootKeys[GetPredefKeyIndex(HKEY_CLASSES_ROOT)].Handle = KeyHandle;


#if 0
    /* Create SAM key */
    ConnectRegistry(NULL,
                    &SamHive,
                    // SystemSecurity, sizeof(SystemSecurity),
                    L"\\Registry\\Machine\\USetup_SAM");

    /* Create SECURITY key */
    ConnectRegistry(NULL,
                    &SecurityHive,
                    // NULL, 0,
                    L"\\Registry\\Machine\\USetup_SECURITY");

    /* Create BCD key */
    ConnectRegistry(NULL,
                    &BcdHive,
                    // BcdSecurity, sizeof(BcdSecurity),
                    L"\\Registry\\Machine\\USetup_BCD00000000");
#endif

    Status = STATUS_SUCCESS;


    /* Create the 'HKLM\SYSTEM\ControlSet001' key */
    // L"\\Registry\\Machine\\SYSTEM\\USetup_Machine\\SYSTEM\\ControlSet001"
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
        DPRINT1("NtCreateKey() succeeded to %s the ControlSet001 key (Status %lx)\n",
                Disposition == REG_CREATED_NEW_KEY ? "create" : /* REG_OPENED_EXISTING_KEY */ "open",
                Status);
    }
    NtClose(KeyHandle);

    /* Create the 'HKLM\SYSTEM\CurrentControlSet' symlink */
    if (!CmpLinkKeyToHive(RootKeys[GetPredefKeyIndex(HKEY_LOCAL_MACHINE)].Handle,
                          L"SYSTEM\\CurrentControlSet",
                          L"\\Registry\\Machine\\SYSTEM\\USetup_Machine\\SYSTEM\\ControlSet001"))
    {
        DPRINT1("CmpLinkKeyToHive(CurrentControlSet) failed!\n");
    }


Quit:
    /* Remove restore and backup privileges */
    RtlAdjustPrivilege(SE_BACKUP_PRIVILEGE, PrivilegeSet[1], FALSE, &PrivilegeSet[1]);
    RtlAdjustPrivilege(SE_RESTORE_PRIVILEGE, PrivilegeSet[0], FALSE, &PrivilegeSet[0]);

    return Status;
}

VOID
RegCleanupRegistry(VOID)
{
    NTSTATUS Status;
    UNICODE_STRING KeyName;
    OBJECT_ATTRIBUTES KeyObjectAttributes;
    BOOLEAN PrivilegeSet[2] = {FALSE, FALSE};
    UCHAR i;

    for (i = 0; i < ARRAYSIZE(RootKeys); ++i)
    {
        if (RootKeys[i].Handle)
        {
            NtClose(RootKeys[i].Handle);
            RootKeys[i].Handle = NULL;
        }
    }

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

    InitializeObjectAttributes(&KeyObjectAttributes,
                               &KeyName,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    RtlInitUnicodeString(&KeyName, L"\\Registry\\Machine\\USetup_SYSTEM");
    Status = NtUnloadKey(&KeyObjectAttributes);

    RtlInitUnicodeString(&KeyName, L"\\Registry\\Machine\\USetup_SOFTWARE");
    Status = NtUnloadKey(&KeyObjectAttributes);

    RtlInitUnicodeString(&KeyName, L"\\Registry\\User\\USetup_DEFAULT");
    Status = NtUnloadKey(&KeyObjectAttributes);

#if 0
    RtlInitUnicodeString(&KeyName, L"\\Registry\\Machine\\USetup_SAM");
    Status = NtUnloadKey(&KeyObjectAttributes);

    RtlInitUnicodeString(&KeyName, L"\\Registry\\Machine\\USetup_SECURITY");
    Status = NtUnloadKey(&KeyObjectAttributes);

    RtlInitUnicodeString(&KeyName, L"\\Registry\\Machine\\USetup_BCD00000000");
    Status = NtUnloadKey(&KeyObjectAttributes);
#endif

    /* Remove restore and backup privileges */
    RtlAdjustPrivilege(SE_BACKUP_PRIVILEGE, PrivilegeSet[1], FALSE, &PrivilegeSet[1]);
    RtlAdjustPrivilege(SE_RESTORE_PRIVILEGE, PrivilegeSet[0], FALSE, &PrivilegeSet[0]);
}

VOID
SetDefaultPagefile(
    WCHAR Drive)
{
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING KeyName = RTL_CONSTANT_STRING(L"SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Memory Management");
    UNICODE_STRING ValueName = RTL_CONSTANT_STRING(L"PagingFiles");
    WCHAR ValueBuffer[] = L"?:\\pagefile.sys 0 0\0";
    HANDLE KeyHandle;
    NTSTATUS Status;

    InitializeObjectAttributes(&ObjectAttributes,
                               &KeyName,
                               OBJ_CASE_INSENSITIVE,
                               RootKeys[GetPredefKeyIndex(HKEY_LOCAL_MACHINE)].Handle,
                               NULL);
    Status = NtOpenKey(&KeyHandle,
                       KEY_ALL_ACCESS,
                       &ObjectAttributes);
    if (!NT_SUCCESS(Status))
        return;

    ValueBuffer[0] = Drive;

    NtSetValueKey(KeyHandle,
                  &ValueName,
                  0,
                  REG_MULTI_SZ,
                  (PVOID)&ValueBuffer,
                  sizeof(ValueBuffer));

    NtClose(KeyHandle);
}

/* EOF */
