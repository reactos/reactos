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
/* $Id: computername.c,v 1.1 2003/06/08 20:59:30 ekohl Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * PURPOSE:         Computer name functions
 * FILE:            lib/kernel32/misc/computername.c
 * PROGRAMER:       Eric Kohl (ekohl@rz-online.de)
 */

/* INCLUDES ******************************************************************/

#include <k32.h>

#define NDEBUG
#include <kernel32/kernel32.h>


/* FUNCTIONS *****************************************************************/

BOOL STDCALL
GetComputerNameA (LPSTR lpBuffer,
		  LPDWORD lpnSize)
{
  UNICODE_STRING UnicodeString;
  ANSI_STRING AnsiString;
  BOOL Result;

  AnsiString.MaximumLength = *lpnSize;
  AnsiString.Length = 0;
  AnsiString.Buffer = lpBuffer;

  UnicodeString.MaximumLength = *lpnSize * sizeof(WCHAR);
  UnicodeString.Length = 0;
  UnicodeString.Buffer = RtlAllocateHeap (RtlGetProcessHeap (),
					  0,
					  UnicodeString.MaximumLength);
  if (UnicodeString.Buffer == NULL)
    {
      SetLastError (ERROR_OUTOFMEMORY);
      return FALSE;
    }

  if (!GetComputerNameW (UnicodeString.Buffer, lpnSize))
    {
      RtlFreeUnicodeString (&UnicodeString);
      return FALSE;
    }

  UnicodeString.Length = *lpnSize * sizeof(WCHAR);

  RtlUnicodeStringToAnsiString (&AnsiString,
				&UnicodeString,
				FALSE);

  RtlFreeUnicodeString (&UnicodeString);

  return TRUE;
}


BOOL STDCALL
GetComputerNameW (LPWSTR lpBuffer,
		  LPDWORD lpnSize)
{
  PKEY_VALUE_PARTIAL_INFORMATION KeyInfo;
  OBJECT_ATTRIBUTES ObjectAttributes;
  UNICODE_STRING KeyName;
  UNICODE_STRING ValueName;
  HANDLE KeyHandle;
  ULONG KeyInfoSize;
  ULONG ReturnSize;
  NTSTATUS Status;

  RtlInitUnicodeString (&KeyName,
			L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\ComputerName\\ComputerName");
  InitializeObjectAttributes (&ObjectAttributes,
			      &KeyName,
			      OBJ_CASE_INSENSITIVE,
			      NULL,
			      NULL);
  Status = NtOpenKey (&KeyHandle,
		      KEY_READ,
		      &ObjectAttributes);
  if (!NT_SUCCESS(Status))
    {
      SetLastErrorByStatus (Status);
      return FALSE;
    }

  KeyInfoSize = sizeof(KEY_VALUE_PARTIAL_INFORMATION) +
		*lpnSize * sizeof(WCHAR);
  KeyInfo = RtlAllocateHeap (RtlGetProcessHeap (),
			     0,
			     KeyInfoSize);
  if (KeyInfo == NULL)
    {
      NtClose (KeyHandle);
      SetLastError (ERROR_OUTOFMEMORY);
      return FALSE;
    }

  RtlInitUnicodeString (&ValueName,
			L"ComputerName");

  Status = NtQueryValueKey (KeyHandle,
			    &ValueName,
			    KeyValuePartialInformation,
			    KeyInfo,
			    KeyInfoSize,
			    &ReturnSize);
  if (!NT_SUCCESS(Status))
    {
      RtlFreeHeap (RtlGetProcessHeap (),
		   0,
		   KeyInfo);
      NtClose (KeyHandle);
      SetLastErrorByStatus (Status);
      return FALSE;
    }

  *lpnSize =
    (KeyInfo->DataLength != 0) ? (KeyInfo->DataLength / sizeof(WCHAR)) - 1 : 0;

  RtlCopyMemory (lpBuffer,
		 KeyInfo->Data,
		 KeyInfo->DataLength);
  lpBuffer[*lpnSize] = 0;

  RtlFreeHeap (RtlGetProcessHeap (),
	       0,
	       KeyInfo);
  NtClose (KeyHandle);

  return TRUE;
}


BOOL STDCALL
SetComputerNameA (LPCSTR lpComputerName)
{
  UNICODE_STRING ComputerName;
  BOOL bResult;

  RtlCreateUnicodeStringFromAsciiz (&ComputerName,
				    (LPSTR)lpComputerName);

  bResult = SetComputerNameW (ComputerName.Buffer);

  RtlFreeUnicodeString (&ComputerName);

  return bResult;
}


static BOOL
IsValidComputerName (LPCWSTR lpComputerName)
{
  PWCHAR p;
  ULONG Length;

  Length = 0;
  p = (PWCHAR)lpComputerName;
  while (*p != 0)
    {
      if (!(iswctype (*p, _ALPHA || _DIGIT) ||
	    *p == L'!' ||
	    *p == L'@' ||
	    *p == L'#' ||
	    *p == L'$' ||
	    *p == L'%' ||
	    *p == L'^' ||
	    *p == L'&' ||
	    *p == L'\'' ||
	    *p == L')' ||
	    *p == L'(' ||
	    *p == L'.' ||
	    *p == L'-' ||
	    *p == L'_' ||
	    *p == L'{' ||
	    *p == L'}' ||
	    *p == L'~'))
	return FALSE;

      Length++;
      p++;
    }

  if (Length == 0 ||
      Length > MAX_COMPUTERNAME_LENGTH)
    return FALSE;

  return TRUE;
}


BOOL STDCALL
SetComputerNameW (LPCWSTR lpComputerName)
{
  OBJECT_ATTRIBUTES ObjectAttributes;
  UNICODE_STRING KeyName;
  UNICODE_STRING ValueName;
  HANDLE KeyHandle;
  NTSTATUS Status;

  if (!IsValidComputerName (lpComputerName))
    {
      SetLastError (ERROR_INVALID_PARAMETER);
      return FALSE;
    }

  RtlInitUnicodeString (&KeyName,
			L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\ComputerName\\ComputerName");
  InitializeObjectAttributes (&ObjectAttributes,
			      &KeyName,
			      OBJ_CASE_INSENSITIVE,
			      NULL,
			      NULL);
  Status = NtOpenKey (&KeyHandle,
		      KEY_WRITE,
		      &ObjectAttributes);
  if (!NT_SUCCESS(Status))
    {
      SetLastErrorByStatus (Status);
      return FALSE;
    }

  RtlInitUnicodeString (&ValueName,
			L"ComputerName");

  Status = NtSetValueKey (KeyHandle,
			  &ValueName,
			  0,
			  REG_SZ,
			  (PVOID)lpComputerName,
			  (wcslen (lpComputerName) + 1) * sizeof(WCHAR));
  if (!NT_SUCCESS(Status))
    {
      NtClose (KeyHandle);
      SetLastErrorByStatus (Status);
      return FALSE;
    }

  NtFlushKey (KeyHandle);
  NtClose (KeyHandle);

  return TRUE;
}

/* EOF */
