/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/kernel32/file/copy.c
 * PURPOSE:         Copying files
 * PROGRAMMER:      Ariadne ( ariadne@xs4all.nl)
 * UPDATE HISTORY:
 *                  01/11/98 Created
 *                  07/02/99 Moved to seperate file 
 */

/* INCLUDES ****************************************************************/

#include <ddk/ntddk.h>
#include <windows.h>
#include <wchar.h>
#include <string.h>

#define NDEBUG
#include <kernel32/kernel32.h>

#define LPPROGRESS_ROUTINE void*

/* FUNCTIONS ****************************************************************/

WINBOOL STDCALL CopyFileExW(LPCWSTR lpExistingFileName, 
			    LPCWSTR lpNewFileName, 
			    LPPROGRESS_ROUTINE lpProgressRoutine, 
			    LPVOID lpData, 
			    WINBOOL * pbCancel, 
			    DWORD dwCopyFlags)
{
   NTSTATUS errCode = 0;
   HANDLE FileHandleSource, FileHandleDest;
   IO_STATUS_BLOCK IoStatusBlock;
   FILE_STANDARD_INFORMATION FileStandard;
   FILE_BASIC_INFORMATION FileBasic;
   FILE_POSITION_INFORMATION FilePosition;
   UCHAR *lpBuffer = NULL;
   ULONG RegionSize = 0x1000000;
   BOOL bCancel = FALSE;

   FileHandleSource = CreateFileW(lpExistingFileName,	
				  GENERIC_READ,	
				  FILE_SHARE_READ,	
				  NULL,	
				  OPEN_EXISTING,	
				  FILE_ATTRIBUTE_NORMAL|FILE_FLAG_NO_BUFFERING,
				  NULL);
   if (FileHandleSource == NULL)
     {
	return(FALSE);
     }
  
   errCode = NtQueryInformationFile(FileHandleSource,
				    &IoStatusBlock,
				    &FileStandard, 
				    sizeof(FILE_STANDARD_INFORMATION),
				    FileStandardInformation);
   if (!NT_SUCCESS(errCode)) 
     {
	NtClose(FileHandleSource);
	SetLastError(RtlNtStatusToDosError(errCode));
	return FALSE;
     }

   errCode = NtQueryInformationFile(FileHandleSource,
				    &IoStatusBlock,&FileBasic, 
				    sizeof(FILE_BASIC_INFORMATION),
				    FileBasicInformation);
   if (!NT_SUCCESS(errCode)) 
     {
	NtClose(FileHandleSource);
	SetLastError(RtlNtStatusToDosError(errCode));
	return FALSE;
     }
  
   FileHandleDest = CreateFileW(lpNewFileName,	
				GENERIC_WRITE,	
				FILE_SHARE_WRITE,	
				NULL,	
				dwCopyFlags  ?  CREATE_NEW : CREATE_ALWAYS ,
				FileBasic.FileAttributes|FILE_FLAG_NO_BUFFERING,
				NULL);
   if (FileHandleDest == NULL)
     {
	return(FALSE);
     }

   FilePosition.CurrentByteOffset.QuadPart = 0;
   
   errCode = NtSetInformationFile(FileHandleSource,
				  &IoStatusBlock,
				  &FilePosition, 
				  sizeof(FILE_POSITION_INFORMATION),
				  FilePositionInformation);
   if (!NT_SUCCESS(errCode)) 
     {
	NtClose(FileHandleSource);
	NtClose(FileHandleDest);
	SetLastError(RtlNtStatusToDosError(errCode));
	return FALSE;
     } 

   errCode = NtSetInformationFile(FileHandleDest,
				  &IoStatusBlock,
				  &FilePosition, 
				  sizeof(FILE_POSITION_INFORMATION),
				  FilePositionInformation);
   if (!NT_SUCCESS(errCode))
     {
	NtClose(FileHandleSource);
	NtClose(FileHandleDest);
	SetLastError(RtlNtStatusToDosError(errCode));
	return FALSE;
     } 

   errCode = NtAllocateVirtualMemory(NtCurrentProcess(),
				     (PVOID *)&lpBuffer,
				     2,
				     &RegionSize,
				     MEM_COMMIT, 
				     PAGE_READWRITE);
 
   if (!NT_SUCCESS(errCode)) 
     {
	NtClose(FileHandleSource);
	NtClose(FileHandleDest);
	SetLastError(RtlNtStatusToDosError(errCode));
	return FALSE;
     } 
   
   do {
      errCode = NtReadFile(FileHandleSource,
			   NULL,
			   NULL,
			   NULL,
			   (PIO_STATUS_BLOCK)&IoStatusBlock,
			   lpBuffer,
			   RegionSize,
			   NULL,
			   NULL);
      if (pbCancel != NULL)
	bCancel = *pbCancel;
	
      if (!NT_SUCCESS(errCode) || bCancel)
	{
	   NtFreeVirtualMemory(NtCurrentProcess(),
			       (PVOID *)&lpBuffer, &RegionSize,MEM_RELEASE);
	   NtClose(FileHandleSource);
	   NtClose(FileHandleDest);
	   if ( errCode == STATUS_END_OF_FILE )
	     break;
	   else
	     return FALSE;   
	}

      errCode = NtWriteFile(FileHandleDest,
			    NULL,
			    lpProgressRoutine,
			    lpData,
			    (PIO_STATUS_BLOCK)&IoStatusBlock,
			    lpBuffer,
			    RegionSize,
			    NULL,
			    NULL);
      	      
      if (!NT_SUCCESS(errCode)) 
	{
	   NtFreeVirtualMemory(NtCurrentProcess(),
			       (PVOID *)&lpBuffer, 
			       &RegionSize,
			       MEM_RELEASE);
	   NtClose(FileHandleSource);
	   NtClose(FileHandleDest);
	   return FALSE;
	}
      
   } while ( TRUE );
   return TRUE;
}

WINBOOL STDCALL CopyFileExA(LPCSTR lpExistingFileName, 
			    LPCSTR lpNewFileName, 
			    LPPROGRESS_ROUTINE lpProgressRoutine, 
			    LPVOID lpData, 
			    WINBOOL* pbCancel, 
			    DWORD dwCopyFlags)
{
   WCHAR ExistingFileNameW[MAX_PATH];
   WCHAR NewFileNameW[MAX_PATH];
   
   if (!KERNEL32_AnsiToUnicode(ExistingFileNameW,
			       lpExistingFileName,
			       MAX_PATH))
     {
	return(FALSE);
     }
   if (!KERNEL32_AnsiToUnicode(NewFileNameW,
			       lpNewFileName,
			       MAX_PATH))
     {
	return(FALSE);
     }
   return(CopyFileExW(ExistingFileNameW,
		      NewFileNameW,
		      lpProgressRoutine,
		      lpData,
		      pbCancel,
		      dwCopyFlags));
}


WINBOOL STDCALL CopyFileA(LPCSTR lpExistingFileName,
			  LPCSTR lpNewFileName,
			  WINBOOL bFailIfExists)
{
	return CopyFileExA(lpExistingFileName,
			   lpNewFileName,
			   NULL,
			   NULL,
			   FALSE,
			   bFailIfExists);
}
WINBOOL STDCALL CopyFileW(LPCWSTR lpExistingFileName,
			  LPCWSTR lpNewFileName,
			  WINBOOL bFailIfExists)
{
   return CopyFileExW(lpExistingFileName,
		      lpNewFileName,
		      NULL,
		      NULL,
		      NULL,
		      bFailIfExists);
}
