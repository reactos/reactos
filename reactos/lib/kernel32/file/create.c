/* $Id: create.c,v 1.20 2000/05/13 13:50:56 dwelch Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/kernel32/file/create.c
 * PURPOSE:         Directory functions
 * PROGRAMMER:      Ariadne ( ariadne@xs4all.nl)
 *		    GetTempFileName is modified from WINE [ Alexandre Juiliard ]
 * UPDATE HISTORY:
 *                  Created 01/11/98
 *                  Removed use of SearchPath (not used by Windows)
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <ntdll/rtl.h>
#include <windows.h>

#define NDEBUG
#include <kernel32/kernel32.h>


/* FUNCTIONS ****************************************************************/

HANDLE STDCALL CreateFileA (LPCSTR			lpFileName,
			    DWORD			dwDesiredAccess,
			    DWORD			dwShareMode,
			    LPSECURITY_ATTRIBUTES	lpSecurityAttributes,
			    DWORD			dwCreationDisposition,
			    DWORD			dwFlagsAndAttributes,
			    HANDLE			hTemplateFile)
{
   UNICODE_STRING FileNameU;
   ANSI_STRING FileName;
   HANDLE FileHandle;
   
   DPRINT("CreateFileA(lpFileName %s)\n",lpFileName);
   
   RtlInitAnsiString (&FileName,
		      (LPSTR)lpFileName);
   
   /* convert ansi (or oem) string to unicode */
   if (bIsFileApiAnsi)
     RtlAnsiStringToUnicodeString (&FileNameU,
				   &FileName,
				   TRUE);
   else
     RtlOemStringToUnicodeString (&FileNameU,
				  &FileName,
				  TRUE);

   FileHandle = CreateFileW (FileNameU.Buffer,
			     dwDesiredAccess,
			     dwShareMode,
			     lpSecurityAttributes,
			     dwCreationDisposition,
			     dwFlagsAndAttributes,
			     hTemplateFile);
   
   RtlFreeHeap (RtlGetProcessHeap (),
		0,
		FileNameU.Buffer);
   
   return FileHandle;
}


HANDLE STDCALL CreateFileW (LPCWSTR			lpFileName,
			    DWORD			dwDesiredAccess,
			    DWORD			dwShareMode,
			    LPSECURITY_ATTRIBUTES	lpSecurityAttributes,
			    DWORD			dwCreationDisposition,
			    DWORD			dwFlagsAndAttributes,
			    HANDLE			hTemplateFile)
{
   OBJECT_ATTRIBUTES ObjectAttributes;
   IO_STATUS_BLOCK IoStatusBlock;
   UNICODE_STRING NtPathU;
   HANDLE FileHandle;
   NTSTATUS Status;
   ULONG Flags = 0;

   switch (dwCreationDisposition)
     {
      case CREATE_NEW:
	dwCreationDisposition = FILE_CREATE;
	break;
	
      case CREATE_ALWAYS:
	dwCreationDisposition = FILE_OVERWRITE_IF;
	break;
	
      case OPEN_EXISTING:
	dwCreationDisposition = FILE_OPEN;
	break;
	
      case OPEN_ALWAYS:
	dwCreationDisposition = OPEN_ALWAYS;
	break;

      case TRUNCATE_EXISTING:
	dwCreationDisposition = FILE_OVERWRITE;
     }
   
   DPRINT("CreateFileW(lpFileName %S)\n",lpFileName);
   
   if (dwDesiredAccess & GENERIC_READ)
     dwDesiredAccess |= FILE_GENERIC_READ;
   
   if (dwDesiredAccess & GENERIC_WRITE)
     dwDesiredAccess |= FILE_GENERIC_WRITE;
   
   if (!(dwFlagsAndAttributes & FILE_FLAG_OVERLAPPED))
     {
	Flags |= FILE_SYNCHRONOUS_IO_ALERT;
     }
   
   if (!RtlDosPathNameToNtPathName_U ((LPWSTR)lpFileName,
				      &NtPathU,
				      NULL,
				      NULL))
     return FALSE;
   
   DPRINT("NtPathU \'%S\'\n", NtPathU.Buffer);
   
   ObjectAttributes.Length = sizeof(OBJECT_ATTRIBUTES);
   ObjectAttributes.RootDirectory = NULL;
   ObjectAttributes.ObjectName = &NtPathU;
   ObjectAttributes.Attributes = OBJ_CASE_INSENSITIVE;
   ObjectAttributes.SecurityDescriptor = NULL;
   ObjectAttributes.SecurityQualityOfService = NULL;
   
   Status = NtCreateFile (&FileHandle,
			  dwDesiredAccess,
			  &ObjectAttributes,
			  &IoStatusBlock,
			  NULL,
			  dwFlagsAndAttributes,
			  dwShareMode,
			  dwCreationDisposition,
			  Flags,
			  NULL,
			  0);
   if (!NT_SUCCESS(Status))
     {
	SetLastError (RtlNtStatusToDosError (Status));
	return INVALID_HANDLE_VALUE;
     }
   
   return FileHandle;
}

/* EOF */
