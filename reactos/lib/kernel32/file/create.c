/* $Id: create.c,v 1.18 2000/01/21 23:27:47 phreak Exp $
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
#include <windows.h>
#include <wchar.h>
#include <string.h>

#define NDEBUG
#include <kernel32/kernel32.h>


/* FUNCTIONS ****************************************************************/

HANDLE STDCALL CreateFileA(LPCSTR lpFileName,
			   DWORD dwDesiredAccess,
			   DWORD dwShareMode,
			   LPSECURITY_ATTRIBUTES lpSecurityAttributes,
			   DWORD dwCreationDisposition,
			   DWORD dwFlagsAndAttributes,
			   HANDLE hTemplateFile)
{
   WCHAR FileNameW[MAX_PATH];
   ULONG i = 0;

   DPRINT("CreateFileA(lpFileName %s)\n",lpFileName);

   while ((*lpFileName)!=0 && i < MAX_PATH)
     {
	FileNameW[i] = *lpFileName;
	lpFileName++;
	i++;
     }
   FileNameW[i] = 0;

   return CreateFileW(FileNameW,dwDesiredAccess,
		      dwShareMode,
		      lpSecurityAttributes,
		      dwCreationDisposition,
		      dwFlagsAndAttributes, 
		      hTemplateFile);
}


HANDLE STDCALL CreateFileW(LPCWSTR lpFileName,
			   DWORD dwDesiredAccess,
			   DWORD dwShareMode,
			   LPSECURITY_ATTRIBUTES lpSecurityAttributes,
			   DWORD dwCreationDisposition,
			   DWORD dwFlagsAndAttributes,
			   HANDLE hTemplateFile)
{
   HANDLE FileHandle;
   NTSTATUS Status;  
   OBJECT_ATTRIBUTES ObjectAttributes;
   IO_STATUS_BLOCK IoStatusBlock;
   UNICODE_STRING FileNameString;
   ULONG Flags = 0;
   WCHAR PathNameW[MAX_PATH];
   WCHAR FileNameW[MAX_PATH];
   UINT Len = 0;

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

   if (lpFileName[1] == (WCHAR)':')
     {
	wcscpy(PathNameW, lpFileName);
     }
   else if (wcslen(lpFileName) > 4 &&
	    lpFileName[0] == (WCHAR)'\\' &&
	    lpFileName[1] == (WCHAR)'\\' &&
	    lpFileName[2] == (WCHAR)'.' &&
	    lpFileName[3] == (WCHAR)'\\')
     {
	wcscpy(PathNameW, lpFileName+4);
     }
   else if (lpFileName[0] == (WCHAR)'\\')
     {
	GetCurrentDirectoryW(MAX_PATH,PathNameW);
	PathNameW[3] = 0;
	wcscat(PathNameW, lpFileName);
     }
   else
     {
	Len =  GetCurrentDirectoryW(MAX_PATH,PathNameW);
	if ( Len == 0 )
	  return NULL;
	if ( PathNameW[Len-1] != L'\\' ) {
	   PathNameW[Len] = L'\\';
	   PathNameW[Len+1] = 0;
	}
	wcscat(PathNameW,lpFileName); 
     }

   FileNameW[0] = '\\';
   FileNameW[1] = '?';
   FileNameW[2] = '?';
   FileNameW[3] = '\\';
   FileNameW[4] = 0;
   wcscat(FileNameW,PathNameW);

   FileNameString.Length = wcslen( FileNameW)*sizeof(WCHAR);

   if ( FileNameString.Length == 0 )
	return NULL;

   if ( FileNameString.Length > MAX_PATH*sizeof(WCHAR) )
	return NULL;

   FileNameString.Buffer = (WCHAR *)FileNameW;
   FileNameString.MaximumLength = FileNameString.Length + sizeof(WCHAR);

   ObjectAttributes.Length = sizeof(OBJECT_ATTRIBUTES);
   ObjectAttributes.RootDirectory = NULL;
   ObjectAttributes.ObjectName = &FileNameString;
   ObjectAttributes.Attributes = OBJ_CASE_INSENSITIVE;
   ObjectAttributes.SecurityDescriptor = NULL;
   ObjectAttributes.SecurityQualityOfService = NULL;

   DPRINT("File Name %S\n",FileNameW);

   Status = ZwCreateFile(&FileHandle,
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
	SetLastError(RtlNtStatusToDosError(Status));
	return INVALID_HANDLE_VALUE;
   }
   return(FileHandle);
}

/* EOF */
