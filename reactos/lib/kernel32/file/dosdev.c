/* $Id: dosdev.c,v 1.8 2003/09/03 16:16:04 ekohl Exp $
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
#include <kernel32/kernel32.h>


/* FUNCTIONS *****************************************************************/

/*
 * @implemented
 */
WINBOOL
STDCALL
DefineDosDeviceA(
    DWORD dwFlags,
    LPCSTR lpDeviceName,
    LPCSTR lpTargetPath
    )
{
	ULONG i;

	WCHAR DeviceNameW[MAX_PATH];
	WCHAR TargetPathW[MAX_PATH];

	i = 0;
	while ((*lpDeviceName)!=0 && i < MAX_PATH)
	{
		DeviceNameW[i] = *lpDeviceName;
		lpDeviceName++;
		i++;
	}
	DeviceNameW[i] = 0;

	i = 0;
	while ((*lpTargetPath)!=0 && i < MAX_PATH)
	{
		TargetPathW[i] = *lpTargetPath;
		lpTargetPath++;
		i++;
	}
	TargetPathW[i] = 0;
	return DefineDosDeviceW(dwFlags,DeviceNameW,TargetPathW);
}


/*
 * @unimplemented
 */
WINBOOL
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

  if (!RtlCreateUnicodeStringFromAsciiz (&DeviceNameU,
					 (LPSTR)lpDeviceName))
  {
    SetLastError (ERROR_NOT_ENOUGH_MEMORY);
    return 0;
  }

  TargetPathU.Length = 0;
  TargetPathU.MaximumLength = (USHORT)ucchMax * sizeof(WCHAR);
  TargetPathU.Buffer = RtlAllocateHeap (RtlGetProcessHeap (),
					0,
					TargetPathU.MaximumLength);
  if (TargetPathU.Buffer == NULL)
  {
    SetLastError (ERROR_NOT_ENOUGH_MEMORY);
    return 0;
  }

  Length = QueryDosDeviceW (DeviceNameU.Buffer,
			    TargetPathU.Buffer,
			    ucchMax);
  if (Length != 0)
  {
    TargetPathU.Length = (Length - 1) * sizeof(WCHAR);

    TargetPathA.Length = 0;
    TargetPathA.MaximumLength = (USHORT)ucchMax;
    TargetPathA.Buffer = lpTargetPath;

    RtlUnicodeStringToAnsiString (&TargetPathA,
				  &TargetPathU,
				  FALSE);

    DPRINT ("TargetPathU: '%wZ'\n", &TargetPathU);
    DPRINT ("TargetPathA: '%Z'\n", &TargetPathA);
  }

  RtlFreeHeap (RtlGetProcessHeap (),
	       0,
	       TargetPathU.Buffer);
  RtlFreeHeap (RtlGetProcessHeap (),
	       0,
	       DeviceNameU.Buffer);

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
  OBJECT_ATTRIBUTES ObjectAttributes;
  UNICODE_STRING UnicodeString;
  HANDLE DirectoryHandle;
  HANDLE DeviceHandle;
  ULONG ReturnedLength;
  ULONG Length;
  NTSTATUS Status;

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

    ReturnedLength = 0;
    Status = NtQuerySymbolicLinkObject (DeviceHandle,
					&UnicodeString,
					&ReturnedLength);
    NtClose (DeviceHandle);
    if (!NT_SUCCESS (Status))
    {
      DPRINT ("NtQuerySymbolicLinkObject() failed (Status %lx)\n", Status);
      NtClose (DirectoryHandle);
      SetLastErrorByStatus (Status);
      return 0;
    }

    DPRINT ("ReturnedLength: %lu\n", ReturnedLength);
    DPRINT ("TargetLength: %hu\n", UnicodeString.Length);
    DPRINT ("Target: '%wZ'\n", &UnicodeString);

    Length = ReturnedLength / sizeof(WCHAR);
    if (Length <= ucchMax)
    {
      lpTargetPath[Length] = 0;
    }
    else
    {
      DPRINT ("Buffer is too small\n");
      NtClose (DirectoryHandle);
      SetLastErrorByStatus (STATUS_BUFFER_TOO_SMALL);
      return 0;
    }
  }
  else
  {
    /* FIXME */
    DPRINT1 ("Not implemented yet\n");
    NtClose (DirectoryHandle);
    SetLastErrorByStatus (STATUS_NOT_IMPLEMENTED);
    return 0;
  }

  NtClose (DirectoryHandle);

  return Length;
}

/* EOF */
