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
/* $Id: reginf.c,v 1.3 2003/05/18 12:12:07 ekohl Exp $
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS hive maker
 * FILE:            tools/mkhive/reginf.h
 * PURPOSE:         Inf file import code
 * PROGRAMMER:      Eric Kohl
 */

/* INCLUDES *****************************************************************/

#include <string.h>
#include <stdlib.h>

#include "mkhive.h"
#include "registry.h"
#include "infcache.h"



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


/* FUNCTIONS ****************************************************************/

static BOOL
GetRootKey (PCHAR Name)
{
  if (!strcasecmp (Name, "HKCR"))
    {
      strcpy (Name, "\\Registry\\Machine\\SOFTWARE\\Classes\\");
      return TRUE;
    }

  if (!strcasecmp (Name, "HKCU"))
    {
      strcpy (Name, "\\Registry\\User\\.DEFAULT\\");
      return TRUE;
    }

  if (!strcasecmp (Name, "HKLM"))
    {
      strcpy (Name, "\\Registry\\Machine\\");
      return TRUE;
    }

  if (!strcasecmp (Name, "HKU"))
    {
      strcpy (Name, "\\Registry\\User\\");
      return TRUE;
    }

#if 0
  if (!strcasecmp (Name, "HKR"))
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
static BOOL
do_reg_operation(HKEY KeyHandle,
		 PCHAR ValueName,
		 PINFCONTEXT Context,
		 ULONG Flags)
{
  CHAR EmptyStr = (CHAR)0;
  ULONG Type;
  ULONG Size;
//  NTSTATUS Status;

  if (Flags & FLG_ADDREG_DELVAL)  /* deletion */
    {
#if 0
      if (ValueName)
	{
	  RegDeleteValueW( hkey, value );
	}
      else
	{
	  RegDeleteKeyW( hkey, NULL );
	}
#endif
      return TRUE;
    }

  if (Flags & FLG_ADDREG_KEYONLY)
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
      PCHAR Str = NULL;

      if (Type == REG_MULTI_SZ)
	{
	  if (!InfGetMultiSzField (Context, 5, NULL, 0, &Size))
	    Size = 0;

	  if (Size)
	    {
	      Str = malloc (Size);
	      if (Str == NULL)
		return FALSE;

	      InfGetMultiSzField (Context, 5, Str, Size, NULL);
	    }

	  if (Flags & FLG_ADDREG_APPEND)
	    {
	      if (Str == NULL)
		return TRUE;

//	      append_multi_sz_value( hkey, value, str, size );

	      free (Str);
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
	      Str = malloc (Size);
	      if (Str == NULL)
		return FALSE;

	      InfGetStringField (Context, 5, Str, Size, NULL);
	    }
	}

      if (Type == REG_DWORD)
	{
	  ULONG dw = Str ? strtol (Str, NULL, 0) : 0;

	  DPRINT("setting dword %s to %lx\n", ValueName, dw);

	  RegSetValue (KeyHandle,
		       ValueName,
		       Type,
		       (PVOID)&dw,
		       sizeof(ULONG));
	}
      else
	{
	  DPRINT ("setting value %wZ to %S\n", ValueName, Str);

	  if (Str)
	    {
	      RegSetValue (KeyHandle,
			   ValueName,
			   Type,
			   (PVOID)Str,
			   Size);
	    }
	  else
	    {
	      RegSetValue (KeyHandle,
			   ValueName,
			   Type,
			   (PVOID)&EmptyStr,
			   sizeof(CHAR));
	    }
	}
      free (Str);
    }
  else  /* get the binary data */
    {
      PUCHAR Data = NULL;

      if (!InfGetBinaryField (Context, 5, NULL, 0, &Size))
	Size = 0;

      if (Size)
	{
	  Data = malloc (Size);
	  if (Data == NULL)
	    return FALSE;

	  DPRINT("setting binary data %s len %lu\n", ValueName, Size);
	  InfGetBinaryField (Context, 5, Data, Size, NULL);
	}

      RegSetValue (KeyHandle,
		   ValueName,
		   Type,
		   (PVOID)Data,
		   Size);

      free (Data);
    }

  return TRUE;
}


/***********************************************************************
 *            registry_callback
 *
 * Called once for each AddReg and DelReg entry in a given section.
 */
static BOOL
registry_callback (HINF hInf, PCHAR Section, BOOL Delete)
{
  CHAR Buffer[MAX_INF_STRING_LENGTH];
  PCHAR ValuePtr;
  ULONG Flags;
  ULONG Length;

  INFCONTEXT Context;
  HKEY KeyHandle;
  BOOL Ok;


  Ok = InfFindFirstLine (hInf, Section, NULL, &Context);
  if (!Ok)
    return FALSE;

  for (;Ok; Ok = InfFindNextLine (&Context, &Context))
    {
      /* get root */
      if (!InfGetStringField (&Context, 1, Buffer, MAX_INF_STRING_LENGTH, NULL))
	continue;
      if (!GetRootKey (Buffer))
	continue;

      /* get key */
      Length = strlen (Buffer);
      if (!InfGetStringField (&Context, 2, Buffer + Length, MAX_INF_STRING_LENGTH - Length, NULL))
	*Buffer = 0;

      DPRINT("KeyName: <%s>\n", Buffer);

      /* get flags */
      if (!InfGetIntField (&Context, 4, (PLONG)&Flags))
	Flags = 0;

      DPRINT("Flags: %lx\n", Flags);

      if (Delete || (Flags & FLG_ADDREG_OVERWRITEONLY))
	{
	  if (RegOpenKey (NULL, Buffer, &KeyHandle) != ERROR_SUCCESS)
	    {
	      DPRINT("RegOpenKey(%s) failed\n", Buffer);
	      continue;  /* ignore if it doesn't exist */
	    }
	}
      else
	{
	  if (RegCreateKey (NULL, Buffer, &KeyHandle) != ERROR_SUCCESS)
	    {
	      DPRINT("RegCreateKey(%s) failed\n", Buffer);
	      continue;
	    }
	}

      /* get value name */
      if (InfGetStringField (&Context, 3, Buffer, MAX_INF_STRING_LENGTH, NULL))
	{
	  ValuePtr = Buffer;
	}
      else
	{
	  ValuePtr = NULL;
	}

      /* and now do it */
      if (!do_reg_operation (KeyHandle, ValuePtr, &Context, Flags))
	{
	  return FALSE;
	}
    }

  return TRUE;
}


BOOL
ImportRegistryFile(PCHAR FileName,
		   PCHAR Section,
		   BOOL Delete)
{
  HINF hInf;
  ULONG ErrorLine;

  /* Load inf file from install media. */
  if (!InfOpenFile(&hInf, FileName, &ErrorLine))
    {
      DPRINT1 ("InfOpenFile() failed\n");
      return FALSE;
    }

  if (!registry_callback (hInf, "AddReg", FALSE))
    {
      DPRINT1 ("registry_callback() failed\n");
    }

  InfCloseFile (hInf);

  return TRUE;
}

/* EOF */
