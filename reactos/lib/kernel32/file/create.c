/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/kernel32/file/create.c
 * PURPOSE:         File create/open functions
 * PROGRAMMER:      Ariadne ( ariadne@xs4all.nl)
 *		    GetTempFileName is modified from WINE [ Alexandre Juiliard ]
 * UPDATE HISTORY:
 *                  Created 01/11/98
 */

/* INCLUDES *****************************************************************/

#include <windows.h>
#include <ddk/ntddk.h>
#include <wstring.h>
#include <string.h>
#include <kernel32/li.h>
#include <ddk/rtl.h>

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
   
   DPRINT("CreateFileA\n");
   
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
   WCHAR *FilePart;
   UINT Len = 0;
   WCHAR CurrentDir[MAX_PATH];
   
   DPRINT("CreateFileW\n");
   
   if (!(dwFlagsAndAttributes & FILE_FLAG_OVERLAPPED))
     {
	Flags |= FILE_SYNCHRONOUS_IO_ALERT;
     }
   
//   lstrcpyW(PathNameW,L"\\??\\");
   PathNameW[0] = '\\';
   PathNameW[1] = '?';
   PathNameW[2] = '?';
   PathNameW[3] = '\\';
   PathNameW[4] = 0;
   
   DPRINT("Name %w\n",PathNameW);
   if (lpFileName[0] != L'\\' && lpFileName[1] != L':')     
     {
	Len =  GetCurrentDirectoryW(MAX_PATH,CurrentDir);
	DPRINT("CurrentDir %w\n",CurrentDir);
	lstrcatW(PathNameW,CurrentDir);
	DPRINT("Name %w\n",PathNameW);
     }
   lstrcatW(PathNameW,lpFileName);
   DPRINT("Name %w\n",PathNameW);
     
   FileNameString.Length = lstrlenW( PathNameW)*sizeof(WCHAR);
	 
   if ( FileNameString.Length == 0 )
	return NULL;

   if ( FileNameString.Length > MAX_PATH )
	return NULL;
   
   FileNameString.Buffer = (WCHAR *)PathNameW;
   FileNameString.MaximumLength = FileNameString.Length;
     
   ObjectAttributes.Length = sizeof(OBJECT_ATTRIBUTES);
   ObjectAttributes.RootDirectory = NULL;
   ObjectAttributes.ObjectName = &FileNameString;
   ObjectAttributes.Attributes = OBJ_CASE_INSENSITIVE;
   ObjectAttributes.SecurityDescriptor = NULL;
   ObjectAttributes.SecurityQualityOfService = NULL;
   
   Status = NtCreateFile(&FileHandle,
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
   return(FileHandle);			 
}

