/* $Id: delete.c,v 1.5 2000/01/11 17:30:16 ekohl Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/kernel32/file/delete.c
 * PURPOSE:         Deleting files
 * PROGRAMMER:      Ariadne ( ariadne@xs4all.nl)
 * UPDATE HISTORY:
 *                  Created 01/11/98
 */

/* INCLUDES ****************************************************************/

#include <ddk/ntddk.h>
#include <windows.h>
#include <wchar.h>
#include <string.h>

#define NDEBUG
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
   WCHAR FileNameW[MAX_PATH];
   HANDLE FileHandle;
   FILE_DISPOSITION_INFORMATION FileDispInfo;
   IO_STATUS_BLOCK IoStatusBlock;
   UINT Len;

   DPRINT("DeleteFileW (lpFileName %S)\n",lpFileName);

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
	wcscpy(PathNameW, lpFileName);
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
	  return FALSE;
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
	return FALSE;

   if ( FileNameString.Length > MAX_PATH*sizeof(WCHAR) )
	return FALSE;

   FileNameString.Buffer = (WCHAR *)FileNameW;
   FileNameString.MaximumLength = FileNameString.Length + sizeof(WCHAR);

   ObjectAttributes.Length = sizeof(OBJECT_ATTRIBUTES);
   ObjectAttributes.RootDirectory = NULL;
   ObjectAttributes.ObjectName = &FileNameString;
   ObjectAttributes.Attributes = OBJ_CASE_INSENSITIVE| OBJ_INHERIT;
   ObjectAttributes.SecurityDescriptor = NULL;
   ObjectAttributes.SecurityQualityOfService = NULL;

   DPRINT("FileName %S\n",FileNameW);

   errCode = ZwCreateFile(&FileHandle,
                          FILE_WRITE_ATTRIBUTES,
			  &ObjectAttributes,
			  &IoStatusBlock,
			  NULL,
                          FILE_ATTRIBUTE_NORMAL,
			  0,
                          FILE_OPEN,
                          FILE_DIRECTORY_FILE,
			  NULL,
			  0);

   if (!NT_SUCCESS(errCode))
     {
        CHECKPOINT;
	SetLastError(RtlNtStatusToDosError(errCode));
	return FALSE;
     }

   FileDispInfo.DeleteFile = TRUE;

   errCode = NtSetInformationFile(FileHandle,
                                  &IoStatusBlock,
                                  &FileDispInfo,
                                  sizeof(FILE_DISPOSITION_INFORMATION),
                                  FileDispositionInformation);

   if (!NT_SUCCESS(errCode))
     {
        CHECKPOINT;
        NtClose(FileHandle);
	SetLastError(RtlNtStatusToDosError(errCode));
	return FALSE;
     }

   errCode = NtClose(FileHandle);

   if (!NT_SUCCESS(errCode))
     {
        CHECKPOINT;
	SetLastError(RtlNtStatusToDosError(errCode));
	return FALSE;
     }

   return TRUE;
}

/* EOF */
