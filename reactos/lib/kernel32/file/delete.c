/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/kernel32/file/file.c
 * PURPOSE:         Directory functions
 * PROGRAMMER:      Ariadne ( ariadne@xs4all.nl)
 *		    GetTempFileName is modified from WINE [ Alexandre Juiliard ]
 * UPDATE HISTORY:
 *                  Created 01/11/98
 */

/* FIXME: the large integer manipulations in this file dont handle overflow  */

/* INCLUDES ****************************************************************/

#include <windows.h>
#include <ddk/ntddk.h>
#include <wchar.h>
#include <string.h>

//#define NDEBUG
#include <kernel32/kernel32.h>

/* FUNCTIONS ****************************************************************/

WINBOOL STDCALL DeleteFileA(LPCSTR lpFileName)
{
   ULONG i;
   WCHAR FileNameW[MAX_PATH];
   
   i = 0;
   while ((*lpFileName)!=0 && i < MAX_PATH)
     {
	FileNameW[i] = *lpFileName;
	lpFileName++;
	i++;
     }
   FileNameW[i] = 0;
   return DeleteFileW(FileNameW);
}

WINBOOL STDCALL DeleteFileW(LPCWSTR lpFileName)
{
   OBJECT_ATTRIBUTES ObjectAttributes;
   UNICODE_STRING FileNameString;
   NTSTATUS errCode;
   WCHAR PathNameW[MAX_PATH];
   UINT Len;
	
   if (lpFileName[1] != ':') 
     {
	Len =  GetCurrentDirectoryW(MAX_PATH,PathNameW);
	if ( Len == 0 )
	  return FALSE;
	if ( PathNameW[Len-1] != L'\\' ) 
	  {
	     PathNameW[Len] = L'\\';
	     PathNameW[Len+1] = 0;
	  }
     }
   else
     PathNameW[0] = 0;
   lstrcatW(PathNameW,lpFileName); 
   FileNameString.Length = lstrlenW( PathNameW)*sizeof(WCHAR);
   if ( FileNameString.Length == 0 )
     return FALSE;
   
   if (FileNameString.Length > MAX_PATH*sizeof(WCHAR))
     return FALSE;
   
   FileNameString.Buffer = (WCHAR *)PathNameW;
   FileNameString.MaximumLength = FileNameString.Length+sizeof(WCHAR);
  
   ObjectAttributes.Length = sizeof(OBJECT_ATTRIBUTES);
   ObjectAttributes.RootDirectory = NULL;
   ObjectAttributes.ObjectName = &FileNameString;
   ObjectAttributes.Attributes = OBJ_CASE_INSENSITIVE| OBJ_INHERIT;
   ObjectAttributes.SecurityDescriptor = NULL;
   ObjectAttributes.SecurityQualityOfService = NULL;
   
   errCode = NtDeleteFile(&ObjectAttributes);
   if (!NT_SUCCESS(errCode)) 
     {
	SetLastError(RtlNtStatusToDosError(errCode));
	return FALSE;
     }
   return TRUE;
}

