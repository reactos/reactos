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
/* $Id: computername.c,v 1.6 2004/02/15 07:07:11 arty Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * PURPOSE:         Computer name functions
 * FILE:            lib/kernel32/misc/computername.c
 * PROGRAMER:       Eric Kohl (ekohl@rz-online.de)
 */

/* INCLUDES ******************************************************************/

#include <k32.h>
#include <rosrtl/string.h>
#include <rosrtl/registry.h>

#define NDEBUG
#include "../include/debug.h"


/* FUNCTIONS *****************************************************************/

static BOOL GetComputerNameFromRegistry( LPWSTR RegistryKey,
					 LPWSTR ValueNameStr,
					 LPWSTR lpBuffer, 
					 LPDWORD nSize ) {
    PKEY_VALUE_PARTIAL_INFORMATION KeyInfo;
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING KeyName;
    UNICODE_STRING ValueName;
    HANDLE KeyHandle;
    ULONG KeyInfoSize;
    ULONG ReturnSize;
    NTSTATUS Status;
    
    RtlInitUnicodeString (&KeyName,RegistryKey);
    InitializeObjectAttributes (&ObjectAttributes,
				&KeyName,
				OBJ_CASE_INSENSITIVE,
				NULL,
				NULL);
    Status = ZwOpenKey (&KeyHandle,
			KEY_READ,
			&ObjectAttributes);
    if (!NT_SUCCESS(Status))
    {
	SetLastErrorByStatus (Status);
	return FALSE;
    }
    
    KeyInfoSize = sizeof(KEY_VALUE_PARTIAL_INFORMATION) +
	*nSize * sizeof(WCHAR);
    KeyInfo = RtlAllocateHeap (RtlGetProcessHeap (),
			       0,
			       KeyInfoSize);
    if (KeyInfo == NULL)
    {
	ZwClose (KeyHandle);
	SetLastError (ERROR_OUTOFMEMORY);
	return FALSE;
    }
    
    RtlInitUnicodeString (&ValueName,ValueNameStr);
    
    Status = ZwQueryValueKey (KeyHandle,
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
	ZwClose (KeyHandle);
	SetLastErrorByStatus (Status);
	return FALSE;
    }
    
    if( *nSize > (KeyInfo->DataLength / sizeof(WCHAR)) ) {
	*nSize = KeyInfo->DataLength / sizeof(WCHAR);
	lpBuffer[*nSize] = 0;
    }

    RtlCopyMemory (lpBuffer,
		   KeyInfo->Data,
		   *nSize * sizeof(WCHAR));

    RtlFreeHeap (RtlGetProcessHeap (),
		 0,
		 KeyInfo)
;
    ZwClose (KeyHandle);
    
    return TRUE;
}

/*
 * @implemented
 */
BOOL STDCALL
GetComputerNameExW (
    COMPUTER_NAME_FORMAT NameType,
    LPWSTR lpBuffer,
    LPDWORD nSize
    )
{
    UNICODE_STRING ResultString;
    UNICODE_STRING DomainPart, Dot;
    UNICODE_STRING RegKey, RegValue;
    
    switch( NameType ) {
    case ComputerNameNetBIOS: 
	return GetComputerNameFromRegistry
	    ( L"\\Registry\\Machine\\System\\CurrentControlSet"
	      L"\\Control\\ComputerName\\ComputerName",
	      L"ComputerName",
	      lpBuffer, 
	      nSize );

    case ComputerNameDnsDomain:
	return GetComputerNameFromRegistry
	    ( L"\\Registry\\Machine\\System\\CurrentControlSet"
	      L"\\Services\\Tcpip\\Parameters",
	      L"Domain",
	      lpBuffer, 
	      nSize );

    case ComputerNameDnsFullyQualified:
	RtlInitUnicodeString(&Dot,L".");
	RosInitializeString(&ResultString,0,*nSize * sizeof(WCHAR),lpBuffer);
	RtlInitUnicodeString(&RegKey,
			     L"\\Registry\\Machine\\System"
			     L"\\CurrentControlSet\\Services\\Tcpip"
			     L"\\Parameters");
	RtlInitUnicodeString(&RegValue,L"HostName");
	RtlInitUnicodeString(&DomainPart,L"");
	if( NT_SUCCESS(RosReadRegistryValue(&RegKey,&RegValue,&DomainPart)) ) {
	    RtlAppendUnicodeStringToString(&ResultString,&DomainPart);
	    RtlAppendUnicodeStringToString(&ResultString,&Dot);
	    RtlFreeUnicodeString(&DomainPart);
	    RtlInitUnicodeString(&RegValue,L"Domain");
	    RtlInitUnicodeString(&DomainPart,L"");
	    if( NT_SUCCESS(RosReadRegistryValue
			   (&RegKey,&RegValue,&DomainPart)) ) {
		RtlAppendUnicodeStringToString(&ResultString,&DomainPart);
		RtlFreeUnicodeString(&DomainPart);
		*nSize = ResultString.Length / sizeof(WCHAR);
		return TRUE;
	    }
	}
	return FALSE;

    case ComputerNameDnsHostname:
	return GetComputerNameFromRegistry
	    ( L"\\Registry\\Machine\\System\\CurrentControlSet"
	      L"\\Services\\Tcpip\\Parameters",
	      L"Hostname",
	      lpBuffer, 
	      nSize );
	
    case ComputerNamePhysicalDnsDomain:
	return GetComputerNameFromRegistry
	    ( L"\\Registry\\Machine\\System\\CurrentControlSet"
	      L"\\Services\\Tcpip\\Parameters",
	      L"Domain",
	      lpBuffer, 
	      nSize );

	/* XXX Redo these */
    case ComputerNamePhysicalDnsFullyQualified:
	return GetComputerNameExW( ComputerNameDnsFullyQualified, 
				   lpBuffer, nSize );
    case ComputerNamePhysicalDnsHostname:
	return GetComputerNameExW( ComputerNameDnsHostname,
				   lpBuffer, nSize );
    case ComputerNamePhysicalNetBIOS:
	return GetComputerNameExW( ComputerNameNetBIOS,
				   lpBuffer, nSize );

    case ComputerNameMax:
	return FALSE;
    }

    return FALSE;
}

/*
 * @implemented
 */
BOOL
STDCALL
GetComputerNameExA (
    COMPUTER_NAME_FORMAT NameType,
    LPSTR lpBuffer,
    LPDWORD nSize
    )
{
    UNICODE_STRING UnicodeString;
    ANSI_STRING AnsiString;
    BOOL Result;
    PWCHAR TempBuffer = RtlAllocateHeap( GetProcessHeap(), 0, *nSize * sizeof(WCHAR) );
    
    if( !TempBuffer ) {
	return ERROR_OUTOFMEMORY;
    }

    AnsiString.MaximumLength = *nSize;
    AnsiString.Length = 0;
    AnsiString.Buffer = lpBuffer;

    Result = GetComputerNameExW( NameType, TempBuffer, nSize );

    if( Result ) {
	UnicodeString.MaximumLength = *nSize * sizeof(WCHAR);
	UnicodeString.Length = *nSize * sizeof(WCHAR);
	UnicodeString.Buffer = TempBuffer;
	
	RtlUnicodeStringToAnsiString (&AnsiString,
				      &UnicodeString,
				      FALSE);
    }

    HeapFree( GetProcessHeap(), 0, TempBuffer );
    
    return Result;
}

/*
 * @implemented
 */
BOOL STDCALL
GetComputerNameA (LPSTR lpBuffer,
		  LPDWORD lpnSize)
{
    return GetComputerNameExA( ComputerNameNetBIOS, lpBuffer, lpnSize );
}


/*
 * @implemented
 */
BOOL STDCALL
GetComputerNameW (LPWSTR lpBuffer,
		  LPDWORD lpnSize)
{
    return GetComputerNameExW( ComputerNameNetBIOS, lpBuffer, lpnSize );
}


/*
 * @implemented
 */
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


/*
 * @implemented
 */
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


/*
 * @implemented
 */
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
      ZwClose (KeyHandle);
      SetLastErrorByStatus (Status);
      return FALSE;
    }

  NtFlushKey (KeyHandle);
  ZwClose (KeyHandle);

  return TRUE;
}

/* EOF */
