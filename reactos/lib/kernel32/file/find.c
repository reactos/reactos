
/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/kernel32/file/find.c
 * PURPOSE:         Find functions
 * PROGRAMMER:      Ariadne ( ariadne@xs4all.nl)
 * UPDATE HISTORY:
 *                  Created 01/11/98
 */

/* INCLUDES *****************************************************************/

#include <windows.h>
#include <wchar.h>
#include <ddk/ntddk.h>

#define NDEBUG
#include <kernel32/kernel32.h>

/* TYPES ********************************************************************/

typedef struct _KERNEL32_FIND_FILE_DATA
{
   HANDLE DirectoryHandle;
   FILE_BOTH_DIRECTORY_INFORMATION FileInfo;
   WCHAR FileNameExtra[MAX_PATH];
   UNICODE_STRING PatternStr;
} KERNEL32_FIND_FILE_DATA, *PKERNEL32_FIND_FILE_DATA;

typedef struct _WIN32_FIND_DATA_UNICODE {
  DWORD dwFileAttributes; 
  FILETIME ftCreationTime; 
  FILETIME ftLastAccessTime; 
  FILETIME ftLastWriteTime; 
  DWORD    nFileSizeHigh; 
  DWORD    nFileSizeLow; 
  DWORD    dwReserved0; 
  DWORD    dwReserved1; 
  WCHAR    cFileName[ MAX_PATH ]; 
  WCHAR    cAlternateFileName[ 14 ]; 
} WIN32_FIND_DATA_UNICODE, *PWIN32_FIND_DATA_UNICODE; 

typedef struct _WIN32_FIND_DATA_ASCII { 
  DWORD dwFileAttributes; 
  FILETIME ftCreationTime; 
  FILETIME ftLastAccessTime; 
  FILETIME ftLastWriteTime; 
  DWORD    nFileSizeHigh; 
  DWORD    nFileSizeLow; 
  DWORD    dwReserved0; 
  DWORD    dwReserved1; 
  CHAR    cFileName[ MAX_PATH ]; 
  CHAR    cAlternateFileName[ 14 ]; 
} WIN32_FIND_DATA_ASCII, *PWIN32_FIND_DATA_ASCII; 


/* FUNCTIONS *****************************************************************/

WINBOOL STDCALL InternalFindNextFile(HANDLE hFindFile,
                                     LPWIN32_FIND_DATA lpFindFileData)
{
   IO_STATUS_BLOCK IoStatusBlock;
   NTSTATUS Status;
   PKERNEL32_FIND_FILE_DATA IData;
   
   IData = (PKERNEL32_FIND_FILE_DATA)hFindFile;
   
   Status = NtQueryDirectoryFile(IData->DirectoryHandle,
                                 NULL,
                                 NULL,
                                 NULL,
                                 &IoStatusBlock,
                                 (PVOID)&IData->FileInfo,
                                 sizeof(IData->FileInfo) +
                                 sizeof(IData->FileNameExtra),
                                 FileBothDirectoryInformation,
                                 TRUE,
                                 &(IData->PatternStr),
                                 FALSE);
   DPRINT("Found %w\n",IData->FileInfo.FileName);
   if (Status != STATUS_SUCCESS)
   {
        return(FALSE);
   }

   FileDataToWin32Data(lpFindFileData, IData);

   return(TRUE);
}

static FileDataToWin32Data(LPWIN32_FIND_DATA lpFindFileData, PKERNEL32_FIND_FILE_DATA IData)
{
 int i;
   lpFindFileData->dwFileAttributes = IData->FileInfo.FileAttributes;
//   memcpy(&lpFindFileData->ftCreationTime,&IData->FileInfo.CreationTime,sizeof(FILETIME));
//   memcpy(&lpFindFileData->ftLastAccessTime,&IData->FileInfo.LastAccessTime,sizeof(FILETIME));
//   memcpy(&lpFindFileData->ftLastWriteTime,&IData->FileInfo.LastWriteTime,sizeof(FILETIME));
   lpFindFileData->nFileSizeHigh = IData->FileInfo.EndOfFile.HighPart;
   lpFindFileData->nFileSizeLow = IData->FileInfo.EndOfFile.LowPart;

}

HANDLE STDCALL InternalFindFirstFile(LPCWSTR lpFileName, 
                                      LPWIN32_FIND_DATA lpFindFileData)
{
   WCHAR CurrentDirectory[MAX_PATH];
   WCHAR Pattern[MAX_PATH];
   WCHAR Directory[MAX_PATH];
   PWSTR End;
   PKERNEL32_FIND_FILE_DATA IData;
   OBJECT_ATTRIBUTES ObjectAttributes;
   UNICODE_STRING DirectoryNameStr;
   IO_STATUS_BLOCK IoStatusBlock;

   DPRINT("FindFirstFileW(lpFileName %w, lpFindFileData %x)\n",
       lpFileName, lpFindFileData);

   GetFullPathNameW(lpFileName, MAX_PATH, CurrentDirectory, NULL);
   Directory[0] = '\\';
   Directory[1] = '?';
   Directory[2] = '?';
   Directory[3] = '\\';
   Directory[4] = 0;
   DPRINT("Directory %w\n",Directory);
   wcscat(Directory, CurrentDirectory);
   DPRINT("Directory %w\n",Directory);
   End = wcsrchr(Directory, '\\');
   *End = 0;
   
   wcscpy(Pattern, End+1);
   *(End+1) = 0;
   *End = '\\';

   /* change pattern: "*.*" --> "*" */
   if (!wcscmp(Pattern, L"*.*"))
        Pattern[1] = 0;

   DPRINT("Directory %w Pattern %w\n",Directory,Pattern);
   
   IData = HeapAlloc(GetProcessHeap(), 
		     HEAP_ZERO_MEMORY, 
		     sizeof(KERNEL32_FIND_FILE_DATA));
   
   RtlInitUnicodeString(&DirectoryNameStr, Directory);
   InitializeObjectAttributes(&ObjectAttributes,
			      &DirectoryNameStr,
			      0,
			      NULL,
			      NULL);

   if (ZwOpenFile(&IData->DirectoryHandle,
		  FILE_LIST_DIRECTORY,
		  &ObjectAttributes,
		  &IoStatusBlock,
          FILE_OPEN_IF,
		  OPEN_EXISTING)!=STATUS_SUCCESS)
     {
	return(NULL);
     }

   RtlInitUnicodeString(&(IData->PatternStr), Pattern);
   
   NtQueryDirectoryFile(IData->DirectoryHandle,
			NULL,
			NULL,
			NULL,
			&IoStatusBlock,
			(PVOID)&IData->FileInfo,
			sizeof(IData->FileInfo) +
			sizeof(IData->FileNameExtra),
			FileBothDirectoryInformation,
			TRUE,
                        &(IData->PatternStr),
			FALSE);
   DPRINT("Found %w\n",IData->FileInfo.FileName);
   
   FileDataToWin32Data(lpFindFileData, IData);

   return(IData);
}

HANDLE FindFirstFileA(LPCTSTR lpFileName, LPWIN32_FIND_DATA lpFindFileData)
{
   WCHAR lpFileNameW[MAX_PATH];
   ULONG i;
   PKERNEL32_FIND_FILE_DATA IData;
   PWIN32_FIND_DATA_ASCII Ret;

   i = 0;
   while (lpFileName[i]!=0)
     {
        lpFileNameW[i] = lpFileName[i];
	i++;
     }
   lpFileNameW[i] = 0;
   
   IData = InternalFindFirstFile(lpFileNameW,lpFindFileData);
   if (IData == NULL)
     {
	DPRINT("Failing request\n");
	return(INVALID_HANDLE_VALUE);
     }


   Ret = (PWIN32_FIND_DATA_ASCII)lpFindFileData;

   DPRINT("IData->FileInfo.FileNameLength %d\n",
	  IData->FileInfo.FileNameLength);
   for (i=0; i<IData->FileInfo.FileNameLength; i++)
   {
        Ret->cFileName[i] = IData->FileInfo.FileName[i];
   }
   Ret->cFileName[i] = 0;

   DPRINT("IData->FileInfo.ShortNameLength %d\n",
	  IData->FileInfo.ShortNameLength);
   if (IData->FileInfo.ShortNameLength > 13)
     {
	IData->FileInfo.ShortNameLength = 13;
     }
   for (i=0; i<IData->FileInfo.ShortNameLength; i++)
     {
	Ret->cAlternateFileName[i] = IData->FileInfo.ShortName[i];
     }
   Ret->cAlternateFileName[i] = 0;


   return(IData);
}

WINBOOL FindNextFileA(HANDLE hFindFile, LPWIN32_FIND_DATA lpFindFileData)
{
   PWIN32_FIND_DATA_ASCII Ret;
   PKERNEL32_FIND_FILE_DATA IData;
   ULONG i;

   IData = (PKERNEL32_FIND_FILE_DATA)hFindFile;   
   if (IData == NULL)
     {
	return(FALSE);
     }
   if (!InternalFindNextFile(hFindFile, lpFindFileData))
   {
      DPRINT("InternalFindNextFile() failed\n");
      return(FALSE);
   }

   Ret = (PWIN32_FIND_DATA_ASCII)lpFindFileData;

   DPRINT("IData->FileInfo.FileNameLength %d\n",
	  IData->FileInfo.FileNameLength);
   for (i=0; i<IData->FileInfo.FileNameLength; i++)
   {
        Ret->cFileName[i] = IData->FileInfo.FileName[i];
   }
   Ret->cFileName[i] = 0;
   
   DPRINT("IData->FileInfo.ShortNameLength %d\n",
	  IData->FileInfo.ShortNameLength);
   for (i=0; i<IData->FileInfo.ShortNameLength; i++)
     {
	Ret->cAlternateFileName[i] = IData->FileInfo.ShortName[i];
     }
   Ret->cAlternateFileName[i] = 0;

   return(TRUE);
}

BOOL FindClose(HANDLE hFindFile)
{
   PKERNEL32_FIND_FILE_DATA IData;
   
   DPRINT("FindClose(hFindFile %x)\n",hFindFile);

   if (hFindFile || hFindFile == INVALID_HANDLE_VALUE)
     {
       SetLastError (ERROR_INVALID_HANDLE);
       return(FALSE);
     }
   IData = (PKERNEL32_FIND_FILE_DATA)hFindFile;
   CloseHandle(IData->DirectoryHandle);
   HeapFree(GetProcessHeap(), 0, IData);
   return(TRUE);
}

HANDLE STDCALL FindFirstFileW(LPCWSTR lpFileName,
			      LPWIN32_FIND_DATA lpFindFileData)
{
   PWIN32_FIND_DATA_UNICODE Ret;
   PKERNEL32_FIND_FILE_DATA IData;
   int i;

   IData = InternalFindFirstFile(lpFileName,lpFindFileData);
   Ret = (PWIN32_FIND_DATA_UNICODE)lpFindFileData;

   memcpy(Ret->cFileName, IData->FileInfo.FileName, 
	  IData->FileInfo.FileNameLength);
   memset(Ret->cAlternateFileName, IData->FileInfo.ShortName, 
	  IData->FileInfo.ShortNameLength);


   return(IData);
}

WINBOOL STDCALL FindNextFileW(HANDLE hFindFile,
			      LPWIN32_FIND_DATA lpFindFileData)
{
   PWIN32_FIND_DATA_UNICODE Ret;
   PKERNEL32_FIND_FILE_DATA IData;
   int i;

   IData = (PKERNEL32_FIND_FILE_DATA)hFindFile;   
   if (!InternalFindNextFile(hFindFile, lpFindFileData))
   {
        return(FALSE);
   }

   Ret = (PWIN32_FIND_DATA_UNICODE)lpFindFileData;

   memcpy(Ret->cFileName, IData->FileInfo.FileName, 
	  IData->FileInfo.FileNameLength);
   memcpy(Ret->cAlternateFileName, IData->FileInfo.ShortName, 
	  IData->FileInfo.ShortNameLength);

   return(TRUE);
}
