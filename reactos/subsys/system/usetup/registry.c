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
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS text-mode setup
 * FILE:            subsys/system/usetup/registry.c
 * PURPOSE:         Registry creation functions
 * PROGRAMMER:      Eric Kohl
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <ntdll/rtl.h>

#include "usetup.h"
#include "registry.h"
#include "infcache.h"



#define FLG_ADDREG_DELREG_BIT             0x00008000
#define FLG_ADDREG_BINVALUETYPE           0x00000001
#define FLG_ADDREG_NOCLOBBER              0x00000002
#define FLG_ADDREG_DELVAL                 0x00000004
#define FLG_ADDREG_APPEND                 0x00000008
#define FLG_ADDREG_KEYONLY                0x00000010
#define FLG_ADDREG_OVERWRITEONLY          0x00000020
#define FLG_ADDREG_64BITKEY               0x00001000
#define FLG_ADDREG_KEYONLY_COMMON         0x00002000
#define FLG_ADDREG_32BITKEY               0x00004000
#define FLG_ADDREG_TYPE_SZ                0x00000000
#define FLG_ADDREG_TYPE_MULTI_SZ          0x00010000
#define FLG_ADDREG_TYPE_EXPAND_SZ         0x00020000
#define FLG_ADDREG_TYPE_BINARY           (0x00000000 | FLG_ADDREG_BINVALUETYPE)
#define FLG_ADDREG_TYPE_DWORD            (0x00010000 | FLG_ADDREG_BINVALUETYPE)
#define FLG_ADDREG_TYPE_NONE             (0x00020000 | FLG_ADDREG_BINVALUETYPE)
#define FLG_ADDREG_TYPE_MASK             (0xFFFF0000 | FLG_ADDREG_BINVALUETYPE)

#define FLG_DELREG_VALUE                 (0x00000000)
#define FLG_DELREG_TYPE_MASK             FLG_ADDREG_TYPE_MASK
#define FLG_DELREG_TYPE_SZ               FLG_ADDREG_TYPE_SZ
#define FLG_DELREG_TYPE_MULTI_SZ         FLG_ADDREG_TYPE_MULTI_SZ
#define FLG_DELREG_TYPE_EXPAND_SZ        FLG_ADDREG_TYPE_EXPAND_SZ
#define FLG_DELREG_TYPE_BINARY           FLG_ADDREG_TYPE_BINARY
#define FLG_DELREG_TYPE_DWORD            FLG_ADDREG_TYPE_DWORD
#define FLG_DELREG_TYPE_NONE             FLG_ADDREG_TYPE_NONE
#define FLG_DELREG_64BITKEY              FLG_ADDREG_64BITKEY
#define FLG_DELREG_KEYONLY_COMMON        FLG_ADDREG_KEYONLY_COMMON
#define FLG_DELREG_32BITKEY              FLG_ADDREG_32BITKEY
#define FLG_DELREG_OPERATION_MASK        (0x000000FE)
#define FLG_DELREG_MULTI_SZ_DELSTRING    (FLG_DELREG_TYPE_MULTI_SZ | FLG_ADDREG_DELREG_BIT | 0x00000002)


/* FUNCTIONS ****************************************************************/


static BOOLEAN
GetRootKey (PWCHAR Name)
{
  if (!_wcsicmp (Name, L"HKCR"))
    {
      wcscpy (Name, L"\\Registry\\Machine\\SOFTWARE\\Classes\\");
      return TRUE;
    }

  if (!_wcsicmp (Name, L"HKCU"))
    {
      wcscpy (Name, L"\\Registry\\User\\.DEFAULT\\");
      return TRUE;
    }

  if (!_wcsicmp (Name, L"HKLM"))
    {
      wcscpy (Name, L"\\Registry\\Machine\\");
      return TRUE;
    }

  if (!_wcsicmp (Name, L"HKU"))
    {
      wcscpy (Name, L"\\Registry\\User\\");
      return TRUE;
    }

#if 0
  if (!_wcsicmp (Name, L"HKR"))
    return FALSE;
#endif

  return FALSE;
}


/***********************************************************************
 *            append_multi_sz_value
 *
 * Append a multisz string to a multisz registry value.
 */
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

    if (!(buffer = HeapAlloc( GetProcessHeap(), 0, (size + str_size) * sizeof(WCHAR) ))) return;
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
  WCHAR EmptyStr = (WCHAR)0;
  ULONG Type;
  ULONG Size;
  NTSTATUS Status;

  if (Flags & (FLG_ADDREG_DELREG_BIT | FLG_ADDREG_DELVAL))  /* deletion */
    {
#if 0
      if (ValueName && !(flags & FLG_DELREG_KEYONLY_COMMON))
	{
	  if ((Flags & FLG_DELREG_MULTI_SZ_DELSTRING) == FLG_DELREG_MULTI_SZ_DELSTRING)
	    {
                WCHAR *str;

                if (!SetupGetStringFieldW( context, 5, NULL, 0, &size ) || !size) return TRUE;
                if (!(str = HeapAlloc( GetProcessHeap(), 0, size * sizeof(WCHAR) ))) return FALSE;
                SetupGetStringFieldW( context, 5, str, size, NULL );
                delete_multi_sz_value( hkey, value, str );
                HeapFree( GetProcessHeap(), 0, str );
	    }
	  else
	    {
	      RegDeleteValueW( hkey, value );
	    }
	}
      else
	{
	  RegDeleteKeyW( hkey, NULL );
	}
#endif
      return TRUE;
    }

  if (Flags & (FLG_ADDREG_KEYONLY | FLG_ADDREG_KEYONLY_COMMON))
    return TRUE;

#if 0
  if (Flags & (FLG_ADDREG_NOCLOBBER | FLG_ADDREG_OVERWRITEONLY))
    {
      BOOL exists = !RegQueryValueExW( hkey, value, NULL, NULL, NULL, NULL );
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
      (Type == REG_DWORD && InfGetFieldCount (Context) == 5))
    {
      PWCHAR Str = NULL;

      if (Type == REG_MULTI_SZ)
	{
	  if (!InfGetMultiSzField (Context, 5, NULL, 0, &Size))
	    Size = 0;

	  if (Size)
	    {
	      Str = RtlAllocateHeap (ProcessHeap, 0, Size * sizeof(WCHAR));
	      if (Str == NULL)
		return FALSE;

	      InfGetMultiSzField (Context, 5, Str, Size, NULL);
	    }

	  if (Flags & FLG_ADDREG_APPEND)
	    {
	      if (Str == NULL)
		return TRUE;

//	      append_multi_sz_value( hkey, value, str, size );

	      RtlFreeHeap (ProcessHeap, 0, Str);
	      return TRUE;
	    }
	  /* else fall through to normal string handling */
	}
      else
	{
	  if (!InfGetStringField (Context, 5, NULL, 0, &Size))
	    Size = 0;

	  if (Size)
	    {
	      Str = RtlAllocateHeap (ProcessHeap, 0, Size * sizeof(WCHAR));
	      if (Str == NULL)
		return FALSE;

	      InfGetStringField (Context, 5, Str, Size, NULL);
	    }
	}

      if (Type == REG_DWORD)
	{
	  ULONG dw = Str ? wcstol (Str, NULL, 0) : 0;

	  DPRINT1("setting dword %wZ to %lx\n", ValueName, dw);

	  Status = NtSetValueKey (KeyHandle,
				  ValueName,
				  0,
				  Type,
				  (PVOID)&dw,
				  sizeof(ULONG));
	  if (!NT_SUCCESS(Status))
	    {
	      DPRINT1("NtSetValueKey() failed (Status %lx)\n", Status);
	    }
	}
      else
	{
	  DPRINT1("setting value %wZ to %S\n", ValueName, Str);

	  if (Str)
	    {
	      Status = NtSetValueKey (KeyHandle,
			     ValueName,
			     0,
			     Type,
			     (PVOID)Str,
			     Size * sizeof(WCHAR));
	      if (!NT_SUCCESS(Status))
		{
		  DPRINT1("NtSetValueKey() failed (Status %lx)\n", Status);
		}
	    }
	  else
	    {
	      Status = NtSetValueKey (KeyHandle,
			     ValueName,
			     0,
			     Type,
			     (PVOID)&EmptyStr,
			     sizeof(WCHAR));
	      if (!NT_SUCCESS(Status))
		{
		  DPRINT1("NtSetValueKey() failed (Status %lx)\n", Status);
		}
	    }
	}
      RtlFreeHeap (ProcessHeap, 0, Str);
    }
  else  /* get the binary data */
    {
      PUCHAR Data = NULL;

      if (!InfGetBinaryField (Context, 5, NULL, 0, &Size))
	Size = 0;

      if (Size)
	{
	  Data = RtlAllocateHeap (ProcessHeap, 0, Size);
	  if (Data == NULL)
	    return FALSE;

	  DPRINT1("setting binary data %wZ len %lu\n", ValueName, Size);
	  InfGetBinaryField (Context, 5, Data, Size, NULL);
	}

      Status = NtSetValueKey (KeyHandle,
			      ValueName,
			      0,
			      Type,
			      (PVOID)Data,
			      Size);
      if (!NT_SUCCESS(Status))
	{
	  DPRINT1("NtSetValueKey() failed (Status %lx)\n", Status);
	}

      RtlFreeHeap (ProcessHeap, 0, Data);
    }

  return TRUE;
}


NTSTATUS
CreateNestedKey (PHANDLE KeyHandle,
		 ACCESS_MASK DesiredAccess,
		 POBJECT_ATTRIBUTES ObjectAttributes)
{
  OBJECT_ATTRIBUTES LocalObjectAttributes;
  UNICODE_STRING LocalKeyName;
  ULONG Disposition;
  NTSTATUS Status;
  ULONG FullNameLength;
  ULONG Length;
  PWCHAR Ptr;
  HANDLE LocalKeyHandle;

  Status = NtCreateKey (KeyHandle,
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
  RtlCopyMemory (&LocalObjectAttributes,
		 ObjectAttributes,
		 sizeof(OBJECT_ATTRIBUTES));
  RtlCreateUnicodeString (&LocalKeyName,
			  ObjectAttributes->ObjectName->Buffer);
  LocalObjectAttributes.ObjectName = &LocalKeyName;
  FullNameLength = LocalKeyName.Length / sizeof(WCHAR);

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

      Status = NtCreateKey (&LocalKeyHandle,
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
  Length = wcslen (LocalKeyName.Buffer);
  while (TRUE)
    {
      if (Length == FullNameLength)
	{
	  Status == STATUS_SUCCESS;
	  *KeyHandle = LocalKeyHandle;
	  break;
	}
      NtClose (LocalKeyHandle);

      LocalKeyName.Buffer[Length] = L'\\';
      Length = wcslen (LocalKeyName.Buffer);
      LocalKeyName.Length = Length * sizeof(WCHAR);

      Status = NtCreateKey (&LocalKeyHandle,
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

  RtlFreeUnicodeString (&LocalKeyName);

  return Status;
}


/***********************************************************************
 *            registry_callback
 *
 * Called once for each AddReg and DelReg entry in a given section.
 */
static BOOLEAN
registry_callback (HINF hInf, PCWSTR Section, BOOLEAN Delete)
{
  OBJECT_ATTRIBUTES ObjectAttributes;
  WCHAR Buffer[MAX_INF_STRING_LENGTH];
  UNICODE_STRING Name;
  UNICODE_STRING Value;
  PUNICODE_STRING ValuePtr;
  NTSTATUS Status;
  ULONG Flags;
  ULONG Length;

  INFCONTEXT Context;
  HANDLE KeyHandle;
  BOOLEAN Ok;


  Ok = InfFindFirstLine (hInf, Section, NULL, &Context);

  for (;Ok; Ok = InfFindNextLine (&Context, &Context))
    {
      /* get root */
      if (!InfGetStringField (&Context, 1, Buffer, MAX_INF_STRING_LENGTH, NULL))
	continue;
      if (!GetRootKey (Buffer))
	continue;

      /* get key */
      Length = wcslen (Buffer);
      if (!InfGetStringField (&Context, 2, Buffer + Length, MAX_INF_STRING_LENGTH - Length, NULL))
	*Buffer = 0;

      DPRINT1("KeyName: <%S>\n", Buffer);

      /* get flags */
      if (!InfGetIntField (&Context, 4, (PLONG)&Flags))
	Flags = 0;

      DPRINT1("Flags: %lx\n", Flags);

      if (!Delete)
	{
	  if (Flags & FLG_ADDREG_DELREG_BIT)
	    continue;  /* ignore this entry */
	}
      else
	{
	  if (!Flags)
	    Flags = FLG_ADDREG_DELREG_BIT;
	  else if (!(Flags & FLG_ADDREG_DELREG_BIT))
	    continue;  /* ignore this entry */
	}

      RtlInitUnicodeString (&Name,
			    Buffer);

      InitializeObjectAttributes (&ObjectAttributes,
				  &Name,
				  OBJ_CASE_INSENSITIVE,
				  NULL,
				  NULL);

      if (Delete || (Flags & FLG_ADDREG_OVERWRITEONLY))
	{
	  Status = NtOpenKey (&KeyHandle,
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
	  Status = CreateNestedKey (&KeyHandle,
				    KEY_ALL_ACCESS,
				    &ObjectAttributes);
	  if (!NT_SUCCESS(Status))
	    {
	      DPRINT1("CreateNestedKey(%wZ) failed (Status %lx)\n", &Name, Status);
	      continue;
	    }
	}

      /* get value name */
      if (InfGetStringField (&Context, 3, Buffer, MAX_INF_STRING_LENGTH, NULL))
	{
	  RtlInitUnicodeString (&Value,
				Buffer);
	  ValuePtr = &Value;
	}
      else
	{
	  ValuePtr = NULL;
	}

      /* and now do it */
      if (!do_reg_operation (KeyHandle, ValuePtr, &Context, Flags))
	{
	  NtClose (KeyHandle);
	  return FALSE;
	}

      NtClose (KeyHandle);
    }

  return TRUE;
}


BOOLEAN
ImportRegistryData(PWSTR Filename)
{
  WCHAR FileNameBuffer[MAX_PATH];
  UNICODE_STRING FileName;
  HINF hInf;
  NTSTATUS Status;
  ULONG ErrorLine;

  /* Load inf file from install media. */
  wcscpy(FileNameBuffer, SourceRootPath.Buffer);
  wcscat(FileNameBuffer, L"\\install\\");
  wcscat(FileNameBuffer, Filename);

  RtlInitUnicodeString(&FileName,
		       FileNameBuffer);

  Status = InfOpenFile(&hInf,
		       &FileName,
		       &ErrorLine);
  if (!NT_SUCCESS(Status))
    {
      DPRINT1("InfOpenFile() failed (Status %lx)\n", Status);
      return FALSE;
    }

  if (!registry_callback (hInf, L"AddReg", FALSE))
    {
      DPRINT1("registry_callback() failed\n");
    }

  InfCloseFile (hInf);

  return TRUE;
}


NTSTATUS
SetupUpdateRegistry(VOID)
{
#if 0
  OBJECT_ATTRIBUTES ObjectAttributes;
  UNICODE_STRING KeyName;
  UNICODE_STRING ValueName;
  HANDLE KeyHandle;
  NTSTATUS Status;

  RtlInitUnicodeStringFromLiteral(&KeyName,
				  L"\\Registry\\Machine\\SYSTEM\\CurrentControlSet\\Control");
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
      DPRINT1("NtCreateKey() failed (Status %lx)\n", Status);
    }

  NtClose(KeyHandle);


  /* Create '\Registry\Machine\System\Setup' key */
  RtlInitUnicodeStringFromLiteral(&KeyName,
				  L"\\Registry\\Machine\\SYSTEM\\Setup");
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
      DPRINT1("NtCreateKey() failed (Status %lx)\n", Status);
    }

  /* FIXME: Create value 'SetupType' */

  /* FIXME: Create value 'SystemSetupInProgress' */


  NtClose(KeyHandle);
#endif


  SetStatusText("   Importing hivesys.inf...");

  if (!ImportRegistryData (L"hivesys.inf"))
    {
      DPRINT1("ImportRegistryData (\"hivesys.inf\") failed\n");
    }

  SetStatusText("   Done...");

  return STATUS_SUCCESS;
}


BOOLEAN
SetInstallPathValue(PUNICODE_STRING InstallPath)
{
  OBJECT_ATTRIBUTES ObjectAttributes;
  UNICODE_STRING KeyName;
  UNICODE_STRING ValueName;
  HANDLE KeyHandle;
  NTSTATUS Status;

  /* Create the 'secret' InstallPath key */
  RtlInitUnicodeStringFromLiteral (&KeyName,
				   L"\\Registry\\Machine\\HARDWARE");
  InitializeObjectAttributes (&ObjectAttributes,
			      &KeyName,
			      OBJ_CASE_INSENSITIVE,
			      NULL,
			      NULL);
  Status =  NtOpenKey (&KeyHandle,
		       KEY_ALL_ACCESS,
		       &ObjectAttributes);
  if (!NT_SUCCESS(Status))
    {
      DPRINT1("NtOpenKey() failed (Status %lx)\n", Status);
      return FALSE;
    }

  RtlInitUnicodeStringFromLiteral (&ValueName,
				   L"InstallPath");
  Status = NtSetValueKey (KeyHandle,
			  &ValueName,
			  0,
			  REG_SZ,
			  (PVOID)InstallPath->Buffer,
			  InstallPath->Length);
  NtClose(KeyHandle);
  if (!NT_SUCCESS(Status))
    {
      DPRINT1("NtSetValueKey() failed (Status %lx)\n", Status);
      return FALSE;
    }

  return TRUE;
}

/* EOF */
