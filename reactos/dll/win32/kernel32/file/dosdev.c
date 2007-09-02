/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/kernel32/file/dosdev.c
 * PURPOSE:         Dos device functions
 * PROGRAMMER:      Ariadne ( ariadne@xs4all.nl)
 * UPDATE HISTORY:
 *                  Created 01/11/98
 */

/* INCLUDES ******************************************************************/

#include <k32.h>

#define NDEBUG
#include <debug.h>


/* FUNCTIONS *****************************************************************/

/*
 * @implemented
 */
BOOL
STDCALL
DefineDosDeviceA(
    DWORD dwFlags,
    LPCSTR lpDeviceName,
    LPCSTR lpTargetPath
    )
{
  UNICODE_STRING DeviceNameU;
  UNICODE_STRING TargetPathU;
  BOOL Result;

  if (!RtlCreateUnicodeStringFromAsciiz (&DeviceNameU,
					 (LPSTR)lpDeviceName))
  {
    SetLastError (ERROR_NOT_ENOUGH_MEMORY);
    return 0;
  }

  if (!RtlCreateUnicodeStringFromAsciiz (&TargetPathU,
					 (LPSTR)lpTargetPath))
  {
    RtlFreeHeap (RtlGetProcessHeap (),
		 0,
		 DeviceNameU.Buffer);
    SetLastError (ERROR_NOT_ENOUGH_MEMORY);
    return 0;
  }

  Result = DefineDosDeviceW (dwFlags,
			     DeviceNameU.Buffer,
			     TargetPathU.Buffer);

  RtlFreeHeap (RtlGetProcessHeap (),
	       0,
	       TargetPathU.Buffer);
  RtlFreeHeap (RtlGetProcessHeap (),
	       0,
	       DeviceNameU.Buffer);

  return Result;
}


/*
 * @unimplemented
 */
BOOL
STDCALL
DefineDosDeviceW(
    DWORD dwFlags,
    LPCWSTR lpDeviceName,
    LPCWSTR lpTargetPath
    )
{
	return FALSE;
}


/*
 * @implemented
 */
DWORD
STDCALL
QueryDosDeviceA(
    LPCSTR lpDeviceName,
    LPSTR lpTargetPath,
    DWORD ucchMax
    )
{
  UNICODE_STRING DeviceNameU;
  UNICODE_STRING TargetPathU;
  ANSI_STRING TargetPathA;
  DWORD Length;
  DWORD CurrentLength;
  PWCHAR Buffer;

  if (lpDeviceName)
  {
    if (!RtlCreateUnicodeStringFromAsciiz (&DeviceNameU,
					   (LPSTR)lpDeviceName))
    {
      SetLastError (ERROR_NOT_ENOUGH_MEMORY);
      return 0;
    }
  }
  Buffer = RtlAllocateHeap (RtlGetProcessHeap (),
			    0,
			    ucchMax * sizeof(WCHAR));
  if (Buffer == NULL)
  {
    if (lpDeviceName)
    {
      RtlFreeHeap (RtlGetProcessHeap (),
	           0,
	           DeviceNameU.Buffer);
    }
    SetLastError (ERROR_NOT_ENOUGH_MEMORY);
    return 0;
  }

  Length = QueryDosDeviceW (lpDeviceName ? DeviceNameU.Buffer : NULL,
			    Buffer,
			    ucchMax);
  if (Length != 0)
  {
    TargetPathA.Buffer = lpTargetPath;
    TargetPathU.Buffer = Buffer;
    ucchMax = Length;

    while (ucchMax)
    {
      CurrentLength = min (ucchMax, MAXUSHORT / 2);
      TargetPathU.MaximumLength = TargetPathU.Length = (USHORT)CurrentLength * sizeof(WCHAR);
     
      TargetPathA.Length = 0;
      TargetPathA.MaximumLength = (USHORT)CurrentLength;

      RtlUnicodeStringToAnsiString (&TargetPathA,
				    &TargetPathU,
				    FALSE);
      ucchMax -= CurrentLength;
      TargetPathA.Buffer += TargetPathA.Length;
      TargetPathU.Buffer += TargetPathU.Length / sizeof(WCHAR);
    }
  }

  RtlFreeHeap (RtlGetProcessHeap (),
	       0,
	       Buffer);
  if (lpDeviceName)
  {
    RtlFreeHeap (RtlGetProcessHeap (),
	         0,
	         DeviceNameU.Buffer);
  }
  return Length;
}


/*
 * @implemented
 */
DWORD
STDCALL
QueryDosDeviceW(
    LPCWSTR lpDeviceName,
    LPWSTR lpTargetPath,
    DWORD ucchMax
    )
{
  POBJECT_DIRECTORY_INFORMATION DirInfo;
  OBJECT_ATTRIBUTES ObjectAttributes;
  UNICODE_STRING UnicodeString;
  HANDLE DirectoryHandle;
  HANDLE DeviceHandle;
  ULONG ReturnLength;
  ULONG NameLength;
  ULONG Length;
  ULONG Context;
  BOOLEAN RestartScan;
  NTSTATUS Status;
  UCHAR Buffer[512];
  PWSTR Ptr;

  /* Open the '\??' directory */
  RtlInitUnicodeString (&UnicodeString,
			L"\\??");
  InitializeObjectAttributes (&ObjectAttributes,
			      &UnicodeString,
			      OBJ_CASE_INSENSITIVE,
			      NULL,
			      NULL);
  Status = NtOpenDirectoryObject (&DirectoryHandle,
				  DIRECTORY_QUERY,
				  &ObjectAttributes);
  if (!NT_SUCCESS (Status))
  {
    DPRINT ("NtOpenDirectoryObject() failed (Status %lx)\n", Status);
    SetLastErrorByStatus (Status);
    return 0;
  }

  Length = 0;

  if (lpDeviceName != NULL)
  {
    /* Open the lpDeviceName link object */
    RtlInitUnicodeString (&UnicodeString,
			  (PWSTR)lpDeviceName);
    InitializeObjectAttributes (&ObjectAttributes,
				&UnicodeString,
				OBJ_CASE_INSENSITIVE,
				DirectoryHandle,
				NULL);
    Status = NtOpenSymbolicLinkObject (&DeviceHandle,
				       SYMBOLIC_LINK_QUERY,
				       &ObjectAttributes);
    if (!NT_SUCCESS (Status))
    {
      DPRINT ("NtOpenSymbolicLinkObject() failed (Status %lx)\n", Status);
      NtClose (DirectoryHandle);
      SetLastErrorByStatus (Status);
      return 0;
    }

    /* Query link target */
    UnicodeString.Length = 0;
    UnicodeString.MaximumLength = (USHORT)ucchMax * sizeof(WCHAR);
    UnicodeString.Buffer = lpTargetPath;

    ReturnLength = 0;
    Status = NtQuerySymbolicLinkObject (DeviceHandle,
					&UnicodeString,
					&ReturnLength);
    NtClose (DeviceHandle);
    NtClose (DirectoryHandle);
    if (!NT_SUCCESS (Status))
    {
      DPRINT ("NtQuerySymbolicLinkObject() failed (Status %lx)\n", Status);
      SetLastErrorByStatus (Status);
      return 0;
    }

    DPRINT ("ReturnLength: %lu\n", ReturnLength);
    DPRINT ("TargetLength: %hu\n", UnicodeString.Length);
    DPRINT ("Target: '%wZ'\n", &UnicodeString);

    Length = ReturnLength / sizeof(WCHAR);
    if (Length < ucchMax)
    {
      /* Append null-charcter */
      lpTargetPath[Length] = UNICODE_NULL;
      Length++;
    }
    else
    {
      DPRINT ("Buffer is too small\n");
      SetLastErrorByStatus (STATUS_BUFFER_TOO_SMALL);
      return 0;
    }
  }
  else
  {
    RestartScan = TRUE;
    Context = 0;
    Ptr = lpTargetPath;
    DirInfo = (POBJECT_DIRECTORY_INFORMATION)Buffer;

    while (TRUE)
    {
      Status = NtQueryDirectoryObject (DirectoryHandle,
				       Buffer,
				       sizeof (Buffer),
				       TRUE,
				       RestartScan,
				       &Context,
				       &ReturnLength);
      if (!NT_SUCCESS(Status))
      {
	if (Status == STATUS_NO_MORE_ENTRIES)
	{
	  /* Terminate the buffer */
	  *Ptr = UNICODE_NULL;
	  Length++;

	  Status = STATUS_SUCCESS;
	}
	else
	{
	  Length = 0;
	}
	SetLastErrorByStatus (Status);
	break;
      }

      if (!wcscmp (DirInfo->TypeName.Buffer, L"SymbolicLink"))
      {
	DPRINT ("Name: '%wZ'\n", &DirInfo->Name);

	NameLength = DirInfo->Name.Length / sizeof(WCHAR);
	if (Length + NameLength + 1 >= ucchMax)
	{
	  Length = 0;
	  SetLastErrorByStatus (STATUS_BUFFER_TOO_SMALL);
	  break;
	}

	memcpy (Ptr,
		DirInfo->Name.Buffer,
		DirInfo->Name.Length);
	Ptr += NameLength;
	Length += NameLength;
	*Ptr = UNICODE_NULL;
	Ptr++;
	Length++;
      }

      RestartScan = FALSE;
    }

    NtClose (DirectoryHandle);
  }

  return Length;
}

/* EOF */
