/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/kernel32/file/file.c
 * PURPOSE:         Directory functions
 * PROGRAMMER:      Ariadne ( ariadne@xs4all.nl)
		    GetTempFileName is modified from WINE [ Alexandre Juiliard ]
 * UPDATE HISTORY:
 *                  Created 01/11/98
 */

/* FIXME: the large integer manipulations in this file dont handle overflow  */

#undef WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <ddk/ntddk.h>
#include <wstring.h>
#include <string.h>
#include <ddk/li.h>
#include <ddk/rtl.h>

#define LPPROGRESS_ROUTINE void*




WINBOOL 
CopyFileExW( 
    LPCWSTR lpExistingFileName, 
    LPCWSTR lpNewFileName, 
    LPPROGRESS_ROUTINE lpProgressRoutine, 
    LPVOID lpData, 
    WINBOOL * pbCancel, 
    DWORD dwCopyFlags 
    );

WINBOOL 
CopyFileExA( 
    LPCSTR lpExistingFileName, 
    LPCSTR lpNewFileName, 
    LPPROGRESS_ROUTINE lpProgressRoutine, 
    LPVOID lpData, 
    WINBOOL * pbCancel, 
    DWORD dwCopyFlags 
    ); 

DWORD
GetCurrentTime(VOID);

BOOLEAN  bIsFileApiAnsi; // set the file api to ansi or oem



VOID
STDCALL
SetFileApisToOEM(VOID)
{
	bIsFileApiAnsi = FALSE;
	return;	
}

WINBASEAPI
VOID
WINAPI
SetFileApisToANSI(VOID)
{
	bIsFileApiAnsi = TRUE;
	return;	
}



WINBOOL
STDCALL
AreFileApisANSI(VOID)
{
	return  bIsFileApiAnsi;
	
}




WINBOOL STDCALL WriteFile(HANDLE  hFile,	
		       LPCVOID lpBuffer,	
		       DWORD nNumberOfBytesToWrite,
		       LPDWORD lpNumberOfBytesWritten,	
		       LPOVERLAPPED lpOverLapped)
{

   LARGE_INTEGER Offset;
   HANDLE hEvent = NULL;
   NTSTATUS errCode;

   WCHAR Buffer[1000];

  //printk("%.*s",nNumberOfBytesToWrite,lpBuffer);
	
   if (lpOverLapped != NULL ) 
     {
	SET_LARGE_INTEGER_LOW_PART(Offset, lpOverLapped->Offset);
	SET_LARGE_INTEGER_HIGH_PART(Offset, lpOverLapped->OffsetHigh);
	lpOverLapped->Internal = STATUS_PENDING;
	hEvent= lpOverLapped->hEvent;
     }
   errCode = NtWriteFile(hFile,hEvent,NULL,NULL,
			 (PIO_STATUS_BLOCK)lpOverLapped,
			 (PVOID)lpBuffer, 
			 nNumberOfBytesToWrite,
			 &Offset,
			 NULL);
   if (!NT_SUCCESS(errCode))
     {
	SetLastError(RtlNtStatusToDosError(errCode));
	return FALSE;
     }
   
   return(TRUE);
}

WINBOOL STDCALL ReadFile(HANDLE hFile,
			 LPVOID lpBuffer,
			 DWORD nNumberOfBytesToRead,
			 LPDWORD lpNumberOfBytesRead,
			 LPOVERLAPPED lpOverLapped)
{

   HANDLE hEvent = NULL;
   PLARGE_INTEGER Offset;
   LARGE_INTEGER ByteOffset;
   NTSTATUS errCode;
   PIO_STATUS_BLOCK IoStatusBlock;
   IO_STATUS_BLOCK IIosb;

  
   if ( lpOverLapped != NULL )
     {
	SET_LARGE_INTEGER_LOW_PART(ByteOffset, lpOverLapped->Offset);
	SET_LARGE_INTEGER_HIGH_PART(ByteOffset, lpOverLapped->OffsetHigh);
	Offset = &ByteOffset;
	lpOverLapped->Internal = STATUS_PENDING;
	hEvent = lpOverLapped->hEvent;
	IoStatusBlock = (PIO_STATUS_BLOCK)lpOverLapped;
     }
   else
     {
	IoStatusBlock = &IIosb;
	Offset = NULL;
     }
				 	     
   errCode = NtReadFile(hFile,
			hEvent,
			NULL,
			NULL,
			IoStatusBlock,
			lpBuffer,
			nNumberOfBytesToRead,
			Offset,			
			NULL);   
   if ( !NT_SUCCESS(errCode) )  
     {      
	SetLastError(RtlNtStatusToDosError(errCode));
	return FALSE;
     }

 
   return TRUE;  
}

WINBOOL
STDCALL
ReadFileEx(
	   HANDLE hFile,
	   LPVOID lpBuffer,
	   DWORD nNumberOfBytesToRead,
	   LPOVERLAPPED lpOverLapped,
	   LPOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine
	   )
{
	HANDLE hEvent = NULL;
	LARGE_INTEGER Offset;
	NTSTATUS errCode;
	IO_STATUS_BLOCK IIosb;
	PIO_STATUS_BLOCK IoStatusBlock;
	
		
	if ( lpOverLapped != NULL ) {
		SET_LARGE_INTEGER_LOW_PART(Offset, lpOverLapped->Offset);
		SET_LARGE_INTEGER_HIGH_PART(Offset, lpOverLapped->OffsetHigh);
		lpOverLapped->Internal = STATUS_PENDING;
		hEvent = lpOverLapped->hEvent;
		IoStatusBlock = (PIO_STATUS_BLOCK)lpOverLapped;
	}
	else {
		SET_LARGE_INTEGER_LOW_PART(Offset, 0);
		SET_LARGE_INTEGER_HIGH_PART(Offset, 0);
		IoStatusBlock = &IIosb;
	}
	
				 
		

	errCode = NtReadFile(hFile,
			     hEvent,
			     (PIO_APC_ROUTINE)lpCompletionRoutine,
			     NULL,
			     IoStatusBlock,
			     lpBuffer,
			     nNumberOfBytesToRead,
			     &Offset,
			     NULL);
	if ( !NT_SUCCESS(errCode) )  {
		SetLastError(RtlNtStatusToDosError(errCode));
		return FALSE;
	}
	return TRUE;  
}


WINBOOL
STDCALL
LockFile(
	 HANDLE hFile,
	 DWORD dwFileOffsetLow,
	 DWORD dwFileOffsetHigh,
	 DWORD nNumberOfBytesToLockLow,
	 DWORD nNumberOfBytesToLockHigh
	 )
{	
	DWORD dwReserved;
	OVERLAPPED Overlapped;
   
	Overlapped.Offset = dwFileOffsetLow;
	Overlapped.OffsetHigh = dwFileOffsetHigh;
	dwReserved = 0;

  	return LockFileEx(hFile, LOCKFILE_FAIL_IMMEDIATELY|LOCKFILE_EXCLUSIVE_LOCK,dwReserved,nNumberOfBytesToLockLow, nNumberOfBytesToLockHigh, &Overlapped ) ;
 
}

WINBOOL
STDCALL
LockFileEx(
	   HANDLE hFile,
	   DWORD dwFlags,
	   DWORD dwReserved,
	   DWORD nNumberOfBytesToLockLow,
	   DWORD nNumberOfBytesToLockHigh,
	   LPOVERLAPPED lpOverlapped
	   )
{
   LARGE_INTEGER BytesToLock;	
   BOOL LockImmediate;
   BOOL LockExclusive;
   NTSTATUS errCode;
   LARGE_INTEGER Offset;
   
   if(dwReserved != 0) 
     {      
	SetLastError(ERROR_INVALID_PARAMETER);
	return FALSE;
     }
   
   lpOverlapped->Internal = STATUS_PENDING;  
   
   SET_LARGE_INTEGER_LOW_PART(Offset, lpOverlapped->Offset);
   SET_LARGE_INTEGER_HIGH_PART(Offset, lpOverlapped->OffsetHigh);
   
   if ( (dwFlags & LOCKFILE_FAIL_IMMEDIATELY) == LOCKFILE_FAIL_IMMEDIATELY )
     LockImmediate = TRUE;
   else
     LockImmediate = FALSE;
   
   if ( (dwFlags & LOCKFILE_EXCLUSIVE_LOCK) == LOCKFILE_EXCLUSIVE_LOCK )
     LockExclusive = TRUE;
   else
     LockExclusive = FALSE;
   
   SET_LARGE_INTEGER_LOW_PART(BytesToLock, nNumberOfBytesToLockLow);
   SET_LARGE_INTEGER_HIGH_PART(BytesToLock, nNumberOfBytesToLockHigh);
   
   errCode = NtLockFile(hFile,
			NULL,
			NULL,
			NULL,
			(PIO_STATUS_BLOCK)lpOverlapped,
			&Offset,
			&BytesToLock,
			NULL,
			LockImmediate,
			LockExclusive);
   if ( !NT_SUCCESS(errCode) ) 
     {
      SetLastError(RtlNtStatusToDosError(errCode));
      return FALSE;
     }
   
   return TRUE;
  	         
}

WINBOOL
STDCALL
UnlockFile(
	   HANDLE hFile,
	   DWORD dwFileOffsetLow,
	   DWORD dwFileOffsetHigh,
	   DWORD nNumberOfBytesToUnlockLow,
	   DWORD nNumberOfBytesToUnlockHigh
	   )
{
	DWORD dwReserved;
	OVERLAPPED Overlapped;
	Overlapped.Offset = dwFileOffsetLow;
	Overlapped.OffsetHigh = dwFileOffsetHigh;
	dwReserved = 0;
	return UnlockFileEx(hFile, dwReserved, nNumberOfBytesToUnlockLow, nNumberOfBytesToUnlockHigh, &Overlapped);

}



WINBOOL 
STDCALL 
UnlockFileEx(
	HANDLE hFile,
	DWORD dwReserved,
	DWORD nNumberOfBytesToUnLockLow,
	DWORD nNumberOfBytesToUnLockHigh,
	LPOVERLAPPED lpOverlapped
	)
{
   LARGE_INTEGER BytesToUnLock;
   LARGE_INTEGER StartAddress;
   NTSTATUS errCode;
   
   if(dwReserved != 0) 
     {
	SetLastError(ERROR_INVALID_PARAMETER);
	return FALSE;
     }
   if ( lpOverlapped == NULL ) 
     {
	SetLastError(ERROR_INVALID_PARAMETER);
	return FALSE;
     }
   
   SET_LARGE_INTEGER_LOW_PART(BytesToUnLock, nNumberOfBytesToUnLockLow);
   SET_LARGE_INTEGER_HIGH_PART(BytesToUnLock, nNumberOfBytesToUnLockHigh);
   
   SET_LARGE_INTEGER_LOW_PART(StartAddress, lpOverlapped->Offset);
   SET_LARGE_INTEGER_HIGH_PART(StartAddress, lpOverlapped->OffsetHigh);
   
   errCode = NtUnlockFile(hFile,
			  (PIO_STATUS_BLOCK)lpOverlapped,
			  &StartAddress,
			  &BytesToUnLock,
			  NULL);
   if ( !NT_SUCCESS(errCode) ) {
      SetLastError(RtlNtStatusToDosError(errCode));
      return FALSE;
   }
   
   return TRUE;
}







WINBOOL
STDCALL
CopyFileA(
    LPCSTR lpExistingFileName,
    LPCSTR lpNewFileName,
    WINBOOL bFailIfExists
    )
{
	return CopyFileExA(lpExistingFileName,lpNewFileName,NULL,NULL,FALSE,bFailIfExists);
}

WINBOOL 
STDCALL 
CopyFileExA( 
    LPCSTR lpExistingFileName, 
    LPCSTR lpNewFileName, 
    LPPROGRESS_ROUTINE lpProgressRoutine, 
    LPVOID lpData, 
    WINBOOL * pbCancel, 
    DWORD dwCopyFlags 
    )
{
	ULONG i;
	WCHAR ExistingFileNameW[MAX_PATH];
	WCHAR NewFileNameW[MAX_PATH];

	

    	i = 0;
   	while ((*lpExistingFileName)!=0 && i < MAX_PATH)
     	{
		ExistingFileNameW[i] = *lpExistingFileName;
		lpExistingFileName++;
		i++;
     	}
   	ExistingFileNameW[i] = 0;

	i = 0;
   	while ((*lpNewFileName)!=0 && i < MAX_PATH)
     	{
		NewFileNameW[i] = *lpNewFileName;
		lpNewFileName++;
		i++;
     	}
   	NewFileNameW[i] = 0;

	return CopyFileExW(ExistingFileNameW,NewFileNameW,lpProgressRoutine,lpData,pbCancel,dwCopyFlags);
}


WINBOOL
STDCALL
CopyFileW(
    LPCWSTR lpExistingFileName,
    LPCWSTR lpNewFileName,
    WINBOOL bFailIfExists
    )
{
	return CopyFileExW(lpExistingFileName,lpNewFileName,NULL,NULL,NULL,bFailIfExists);
}





WINBOOL 
STDCALL 
CopyFileExW( 
    LPCWSTR lpExistingFileName, 
    LPCWSTR lpNewFileName, 
    LPPROGRESS_ROUTINE lpProgressRoutine, 
    LPVOID lpData, 
    WINBOOL * pbCancel, 
    DWORD dwCopyFlags 
    )
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

	

	FileHandleSource = CreateFileW(
  		lpExistingFileName,	
    		GENERIC_READ,	
    		FILE_SHARE_READ,	
    		NULL,	
    		OPEN_EXISTING,	
    		FILE_ATTRIBUTE_NORMAL|FILE_FLAG_NO_BUFFERING,	
    		NULL 
   	);



	if ( !NT_SUCCESS(errCode) ) {
		SetLastError(RtlNtStatusToDosError(errCode));
		return FALSE;
	}
  

	errCode = NtQueryInformationFile(FileHandleSource,
		&IoStatusBlock,&FileStandard, sizeof(FILE_STANDARD_INFORMATION),
		FileStandardInformation);
	if ( !NT_SUCCESS(errCode) ) {
		NtClose(FileHandleSource);
		SetLastError(RtlNtStatusToDosError(errCode));
		return FALSE;
	}

	errCode = NtQueryInformationFile(FileHandleSource,
		&IoStatusBlock,&FileBasic, sizeof(FILE_BASIC_INFORMATION),
		FileBasicInformation);
	if ( !NT_SUCCESS(errCode) ) {
		NtClose(FileHandleSource);
		SetLastError(RtlNtStatusToDosError(errCode));
		return FALSE;
	}
		

	

   
  
		FileHandleDest = CreateFileW(
  			lpNewFileName,	
    		GENERIC_WRITE,	
    		FILE_SHARE_WRITE,	
    		NULL,	
		dwCopyFlags  ?  CREATE_NEW : CREATE_ALWAYS ,	
    		FileBasic.FileAttributes|FILE_FLAG_NO_BUFFERING,	
    		NULL 
		);
 
	if ( !NT_SUCCESS(errCode) ) {
		NtClose(FileHandleSource);
		SetLastError(RtlNtStatusToDosError(errCode));
		return FALSE;
	} 




	SET_LARGE_INTEGER_LOW_PART(FilePosition.CurrentByteOffset, 0);
	SET_LARGE_INTEGER_HIGH_PART(FilePosition.CurrentByteOffset, 0);

	errCode = NtSetInformationFile(FileHandleSource,
		&IoStatusBlock,&FilePosition, sizeof(FILE_POSITION_INFORMATION),
		FilePositionInformation);
	if ( !NT_SUCCESS(errCode) ) {
		NtClose(FileHandleSource);
		NtClose(FileHandleDest);
		SetLastError(RtlNtStatusToDosError(errCode));
		return FALSE;
	} 

	errCode = NtSetInformationFile(FileHandleDest,
		&IoStatusBlock,&FilePosition, sizeof(FILE_POSITION_INFORMATION),
		FilePositionInformation);
  
	if ( !NT_SUCCESS(errCode) ) {
		NtClose(FileHandleSource);
		NtClose(FileHandleDest);
		SetLastError(RtlNtStatusToDosError(errCode));
		return FALSE;
	} 



	errCode = NtAllocateVirtualMemory( 
		NtCurrentProcess(),
		(PVOID *)&lpBuffer,
		2,
		&RegionSize,
		MEM_COMMIT, 
		PAGE_READWRITE
	);
  
	if ( !NT_SUCCESS(errCode) ) {
		NtClose(FileHandleSource);
		NtClose(FileHandleDest);
		SetLastError(RtlNtStatusToDosError(errCode));
		return FALSE;
	} 

	

	

	
	do {

		errCode = NtReadFile(
			     FileHandleSource,
			     NULL,
			     NULL,
			     NULL,
			     (PIO_STATUS_BLOCK)&IoStatusBlock,
			     lpBuffer,
			     RegionSize,
			     NULL,
			     NULL);
		if ( pbCancel != NULL )
			bCancel = *pbCancel;

	
		if ( !NT_SUCCESS(errCode) || bCancel )  {
			NtFreeVirtualMemory(NtCurrentProcess(),(PVOID *)&lpBuffer, &RegionSize,MEM_RELEASE);
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

		

 		if ( !NT_SUCCESS(errCode) ) {
			NtFreeVirtualMemory(NtCurrentProcess(),(PVOID *)&lpBuffer, &RegionSize,MEM_RELEASE);
			NtClose(FileHandleSource);
			NtClose(FileHandleDest);
			return FALSE;
		}

	} while ( TRUE );

	return TRUE;


}


HFILE
STDCALL
OpenFile(
	 LPCSTR lpFileName,
	 LPOFSTRUCT lpReOpenBuff,
	 UINT uStyle
	 )
{
	NTSTATUS errCode;
	HANDLE FileHandle = NULL;
	UNICODE_STRING FileNameString;
	WCHAR FileNameW[MAX_PATH];
	WCHAR PathNameW[MAX_PATH];
	ULONG i;
	OBJECT_ATTRIBUTES ObjectAttributes;
	IO_STATUS_BLOCK IoStatusBlock;
	WCHAR *FilePart;	
	ULONG Len;

	if ( lpReOpenBuff == NULL ) {
		return FALSE;
	}

	

	
    	i = 0;
   	while ((*lpFileName)!=0 && i < MAX_PATH)
     	{
		FileNameW[i] = *lpFileName;
		lpFileName++;
		i++;
     	}
   	FileNameW[i] = 0;


	Len = SearchPathW(NULL,FileNameW,NULL,MAX_PATH,PathNameW,&FilePart);
   	if ( Len == 0 )
		return (HFILE)NULL;


   	if ( Len > MAX_PATH )
		return (HFILE)NULL;
   
   	FileNameString.Length = lstrlenW(PathNameW)*sizeof(WCHAR);
   	FileNameString.Buffer = PathNameW;
   	FileNameString.MaximumLength = FileNameString.Length+sizeof(WCHAR);
   

    	

  	ObjectAttributes.Length = sizeof(OBJECT_ATTRIBUTES);
	ObjectAttributes.RootDirectory = NULL;
   	ObjectAttributes.ObjectName = &FileNameString;
	ObjectAttributes.Attributes = OBJ_CASE_INSENSITIVE| OBJ_INHERIT;
	ObjectAttributes.SecurityDescriptor = NULL;
	ObjectAttributes.SecurityQualityOfService = NULL;

	// FILE_SHARE_READ
	// FILE_NO_INTERMEDIATE_BUFFERING



	if ((uStyle & OF_PARSE) == OF_PARSE ) 
		return (HFILE)NULL;


	errCode = NtOpenFile(
		&FileHandle,
		GENERIC_READ|SYNCHRONIZE,
		&ObjectAttributes,
		&IoStatusBlock,   
		FILE_SHARE_READ,         
   		FILE_NON_DIRECTORY_FILE                                                                 
	);

	lpReOpenBuff->nErrCode = RtlNtStatusToDosError(errCode);

	if ( !NT_SUCCESS(errCode) ) {
		SetLastError(RtlNtStatusToDosError(errCode));
		return (HFILE)INVALID_HANDLE_VALUE;
	}

	return (HFILE)FileHandle;

}


WINBOOL
STDCALL
MoveFileA(
    LPCSTR lpExistingFileName,
    LPCSTR lpNewFileName
    )
{
	return MoveFileExA(lpExistingFileName,lpNewFileName,MOVEFILE_COPY_ALLOWED);
}

WINBOOL
STDCALL
MoveFileExA(
    LPCSTR lpExistingFileName,
    LPCSTR lpNewFileName,
    DWORD dwFlags
    )
{
	ULONG i;
	WCHAR ExistingFileNameW[MAX_PATH];
	WCHAR NewFileNameW[MAX_PATH];

	

    	i = 0;
   	while ((*lpExistingFileName)!=0 && i < MAX_PATH)
     	{
		ExistingFileNameW[i] = *lpExistingFileName;
		lpExistingFileName++;
		i++;
     	}
   	ExistingFileNameW[i] = 0;

	i = 0;
   	while ((*lpNewFileName)!=0 && i < MAX_PATH)
     	{
		NewFileNameW[i] = *lpNewFileName;
		lpNewFileName++;
		i++;
     	}
   	NewFileNameW[i] = 0;

	return MoveFileExW(ExistingFileNameW,NewFileNameW,dwFlags);
	
}



WINBOOL
STDCALL
MoveFileW(
    LPCWSTR lpExistingFileName,
    LPCWSTR lpNewFileName
    )
{
	return MoveFileExW(lpExistingFileName,lpNewFileName,MOVEFILE_COPY_ALLOWED);
}

#define FILE_RENAME_SIZE  MAX_PATH +sizeof(FILE_RENAME_INFORMATION)

WINBOOL
STDCALL
MoveFileExW(
    LPCWSTR lpExistingFileName,
    LPCWSTR lpNewFileName,
    DWORD dwFlags
    )
{
	HANDLE hFile = NULL;
	IO_STATUS_BLOCK IoStatusBlock;
	FILE_RENAME_INFORMATION *FileRename;
	USHORT Buffer[FILE_RENAME_SIZE];
	NTSTATUS errCode;	

	hFile = CreateFileW(
  		lpExistingFileName,	
    		GENERIC_ALL,	
    		FILE_SHARE_WRITE|FILE_SHARE_READ,	
    		NULL,	
    		OPEN_EXISTING,	
    		FILE_ATTRIBUTE_NORMAL,	
    		NULL 
   	);



	FileRename = (FILE_RENAME_INFORMATION *)Buffer;
	if ( ( dwFlags & MOVEFILE_REPLACE_EXISTING ) == MOVEFILE_REPLACE_EXISTING )
		FileRename->Replace = TRUE;
	else
		FileRename->Replace = FALSE;

	FileRename->FileNameLength = lstrlenW(lpNewFileName);
	memcpy(FileRename->FileName,lpNewFileName,min(FileRename->FileNameLength,MAX_PATH));
	
	errCode = NtSetInformationFile(hFile,&IoStatusBlock,FileRename, FILE_RENAME_SIZE, FileRenameInformation);
	if ( !NT_SUCCESS(errCode) ) {
		if ( CopyFileW(lpExistingFileName,lpNewFileName,FileRename->Replace) )
			DeleteFileW(lpExistingFileName);
	}

	CloseHandle(hFile);
	return TRUE;
}

WINBOOL
STDCALL
DeleteFileA(
    LPCSTR lpFileName
    )
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

WINBOOL
STDCALL
DeleteFileW(
    LPCWSTR lpFileName
    )
{
	OBJECT_ATTRIBUTES ObjectAttributes;
	UNICODE_STRING FileNameString;
	NTSTATUS errCode;
	WCHAR PathNameW[MAX_PATH];
	UINT Len;
	if ( lpFileName[1] != ':' ) {
		Len =  GetCurrentDirectoryW(MAX_PATH,PathNameW);
		if ( Len == 0 )
			return FALSE;
		if ( PathNameW[Len-1] != L'\\' ) {
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

   	if ( FileNameString.Length > MAX_PATH*sizeof(WCHAR) )
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
	if ( !NT_SUCCESS(errCode) ) {
		SetLastError(RtlNtStatusToDosError(errCode));
		return FALSE;
	}
	return TRUE;
}

WINBOOL 
STDCALL
FlushFileBuffers(
	HANDLE hFile 	
	)
{
	NTSTATUS errCode;
	IO_STATUS_BLOCK IoStatusBlock;

	errCode = NtFlushBuffersFile(
		hFile,
		&IoStatusBlock
	);
	if ( !NT_SUCCESS(errCode) ) {
		SetLastError(RtlNtStatusToDosError(errCode));
		return FALSE;
	}
	return TRUE;
}


DWORD
STDCALL
SetFilePointer(
	       HANDLE hFile,
	       LONG lDistanceToMove,
	       PLONG lpDistanceToMoveHigh,
	       DWORD dwMoveMethod
	       )
{
	FILE_POSITION_INFORMATION FilePosition;
	FILE_END_OF_FILE_INFORMATION FileEndOfFile;
	NTSTATUS errCode;
	IO_STATUS_BLOCK IoStatusBlock;
	if ( dwMoveMethod == FILE_CURRENT ) {
		NtQueryInformationFile(hFile,&IoStatusBlock,&FilePosition, sizeof(FILE_POSITION_INFORMATION),FilePositionInformation);
		SET_LARGE_INTEGER_LOW_PART(FilePosition.CurrentByteOffset,
                  GET_LARGE_INTEGER_LOW_PART(FilePosition.CurrentByteOffset) + 
                  lDistanceToMove);
		if ( lpDistanceToMoveHigh != NULL ) {
			SET_LARGE_INTEGER_HIGH_PART(FilePosition.CurrentByteOffset,
                          GET_LARGE_INTEGER_HIGH_PART(FilePosition.CurrentByteOffset) +
                          *lpDistanceToMoveHigh);
                }
	}
	else if ( dwMoveMethod == FILE_END ) {
		NtQueryInformationFile(hFile,&IoStatusBlock,&FileEndOfFile, sizeof(FILE_END_OF_FILE_INFORMATION),FileEndOfFileInformation);
		SET_LARGE_INTEGER_LOW_PART(FilePosition.CurrentByteOffset,
                  GET_LARGE_INTEGER_LOW_PART(FileEndOfFile.EndOfFile) - 
                  lDistanceToMove);
		if ( lpDistanceToMoveHigh != NULL ) {
			SET_LARGE_INTEGER_HIGH_PART(FilePosition.CurrentByteOffset,
                          GET_LARGE_INTEGER_HIGH_PART(FileEndOfFile.EndOfFile) - 
                          *lpDistanceToMoveHigh);
		} else {
			SET_LARGE_INTEGER_HIGH_PART(FilePosition.CurrentByteOffset,
                          GET_LARGE_INTEGER_HIGH_PART(FileEndOfFile.EndOfFile));
                }
	}
	else if ( dwMoveMethod == FILE_CURRENT ) {
		SET_LARGE_INTEGER_LOW_PART(FilePosition.CurrentByteOffset,
                  lDistanceToMove);
		if ( lpDistanceToMoveHigh != NULL ) {
			SET_LARGE_INTEGER_HIGH_PART(FilePosition.CurrentByteOffset,
                          *lpDistanceToMoveHigh);
		} else {
			SET_LARGE_INTEGER_HIGH_PART(FilePosition.CurrentByteOffset,
                          0);
                }
	}

	errCode = NtSetInformationFile(hFile,&IoStatusBlock,&FilePosition, sizeof(FILE_POSITION_INFORMATION),FilePositionInformation);
	if ( !NT_SUCCESS(errCode) ) {
      		SetLastError(RtlNtStatusToDosError(errCode));
      		return -1;
   	}
	
	if ( lpDistanceToMoveHigh != NULL ) {
		*lpDistanceToMoveHigh = GET_LARGE_INTEGER_HIGH_PART(FilePosition.CurrentByteOffset);
        }
	return GET_LARGE_INTEGER_LOW_PART(FilePosition.CurrentByteOffset);
}

DWORD 
STDCALL
GetFileType(
	    HANDLE hFile 	
   )
{
	return FILE_TYPE_UNKNOWN;
}


DWORD 
STDCALL
GetFileSize(
    	HANDLE hFile,	
    	LPDWORD lpFileSizeHigh 	
   )
{
	NTSTATUS errCode;
	FILE_STANDARD_INFORMATION FileStandard;
	IO_STATUS_BLOCK IoStatusBlock;


	errCode = NtQueryInformationFile(hFile,
		&IoStatusBlock,&FileStandard, sizeof(FILE_STANDARD_INFORMATION),
		FileStandardInformation);
	if ( !NT_SUCCESS(errCode) ) {
		CloseHandle(hFile);
		SetLastError(RtlNtStatusToDosError(errCode));
		if ( lpFileSizeHigh == NULL ) {
			return -1;
		}
		else
			return 0;
	}
	if ( lpFileSizeHigh != NULL )
		*lpFileSizeHigh = GET_LARGE_INTEGER_HIGH_PART(FileStandard.AllocationSize);

	CloseHandle(hFile);
	return GET_LARGE_INTEGER_LOW_PART(FileStandard.AllocationSize);	
}

DWORD
STDCALL
GetCompressedFileSizeA(
    LPCSTR lpFileName,
    LPDWORD lpFileSizeHigh
    )
{
	WCHAR FileNameW[MAX_PATH];
	ULONG i;
	i = 0;
   	while ((*lpFileName)!=0 && i < MAX_PATH)
     	{
		FileNameW[i] = *lpFileName;
		lpFileName++;
		i++;
     	}
   	FileNameW[i] = 0;


	return GetCompressedFileSizeW(FileNameW,lpFileSizeHigh);
	
}

DWORD
STDCALL
GetCompressedFileSizeW(
    LPCWSTR lpFileName,
    LPDWORD lpFileSizeHigh
    )
{
	FILE_COMPRESSION_INFORMATION FileCompression;
	NTSTATUS errCode;
	IO_STATUS_BLOCK IoStatusBlock;
	HANDLE hFile;

	hFile = CreateFileW(
  		lpFileName,	
    		GENERIC_READ,	
    		FILE_SHARE_READ,	
    		NULL,	
    		OPEN_EXISTING,	
    		FILE_ATTRIBUTE_NORMAL,	
    		NULL 
   	);

	errCode = NtQueryInformationFile(hFile,
		&IoStatusBlock,&FileCompression, sizeof(FILE_COMPRESSION_INFORMATION),
		FileCompressionInformation);
	if ( !NT_SUCCESS(errCode) ) {
		CloseHandle(hFile);
		SetLastError(RtlNtStatusToDosError(errCode));
		return 0;
	}
	CloseHandle(hFile);
	return 0;

}



WINBOOL
STDCALL
GetFileInformationByHandle(
			   HANDLE hFile,
			   LPBY_HANDLE_FILE_INFORMATION lpFileInformation
			   )
{
	FILE_DIRECTORY_INFORMATION FileDirectory;
	FILE_INTERNAL_INFORMATION FileInternal;
	FILE_FS_VOLUME_INFORMATION FileFsVolume;
	FILE_STANDARD_INFORMATION FileStandard;

	NTSTATUS errCode;
	IO_STATUS_BLOCK IoStatusBlock;
	errCode = NtQueryInformationFile(hFile,&IoStatusBlock,&FileDirectory, sizeof(FILE_DIRECTORY_INFORMATION),FileDirectoryInformation);
	if ( !NT_SUCCESS(errCode) ) {
		SetLastError(RtlNtStatusToDosError(errCode));
		return FALSE;
	}
	lpFileInformation->dwFileAttributes = (DWORD)FileDirectory.FileAttributes;
	memcpy(&lpFileInformation->ftCreationTime,&FileDirectory.CreationTime,sizeof(LARGE_INTEGER));
	memcpy(&lpFileInformation->ftLastAccessTime,&FileDirectory.LastAccessTime,sizeof(LARGE_INTEGER));
	memcpy(&lpFileInformation->ftLastWriteTime, &FileDirectory.LastWriteTime,sizeof(LARGE_INTEGER)); 
	lpFileInformation->nFileSizeHigh = GET_LARGE_INTEGER_HIGH_PART(FileDirectory.AllocationSize); 
    	lpFileInformation->nFileSizeLow = GET_LARGE_INTEGER_LOW_PART(FileDirectory.AllocationSize); 
    	 
    

	errCode = NtQueryInformationFile(hFile,&IoStatusBlock,&FileInternal, sizeof(FILE_INTERNAL_INFORMATION),FileInternalInformation);
	if ( !NT_SUCCESS(errCode) ) {
		SetLastError(RtlNtStatusToDosError(errCode));
		return FALSE;
	}
	lpFileInformation->nFileIndexHigh = GET_LARGE_INTEGER_HIGH_PART(FileInternal.IndexNumber);
	lpFileInformation->nFileIndexLow = GET_LARGE_INTEGER_LOW_PART(FileInternal.IndexNumber);


	errCode = NtQueryVolumeInformationFile(hFile,&IoStatusBlock,&FileFsVolume, sizeof(FILE_FS_VOLUME_INFORMATION),FileFsVolumeInformation);
	if ( !NT_SUCCESS(errCode) ) {
		SetLastError(RtlNtStatusToDosError(errCode));
		return FALSE;
	}
	lpFileInformation->dwVolumeSerialNumber = FileFsVolume.VolumeSerialNumber;


	errCode = NtQueryInformationFile(hFile,&IoStatusBlock,&FileStandard, sizeof(FILE_STANDARD_INFORMATION),FileStandardInformation);
	if ( !NT_SUCCESS(errCode) ) {
		CloseHandle(hFile);
		SetLastError(RtlNtStatusToDosError(errCode));
		return FALSE;
	}
	lpFileInformation->nNumberOfLinks = FileStandard.NumberOfLinks;
	CloseHandle(hFile);
	return TRUE;
	
}






DWORD
STDCALL
GetFileAttributesA(
    LPCSTR lpFileName
    )
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
	return GetFileAttributesW(FileNameW);
}


DWORD
STDCALL
GetFileAttributesW(
    LPCWSTR lpFileName
    )
{
	IO_STATUS_BLOCK IoStatusBlock;
	FILE_BASIC_INFORMATION FileBasic;
	HANDLE hFile;
	NTSTATUS errCode;


	hFile = CreateFileW(
  		lpFileName,	
    		GENERIC_READ,	
    		FILE_SHARE_READ,	
    		NULL,	
    		OPEN_EXISTING,	
    		FILE_ATTRIBUTE_NORMAL,	
    		NULL 
   	);

	
	errCode = NtQueryInformationFile(hFile,&IoStatusBlock,&FileBasic, sizeof(FILE_BASIC_INFORMATION),FileBasicInformation);
	if ( !NT_SUCCESS(errCode) ) {
		CloseHandle(hFile);
		SetLastError(RtlNtStatusToDosError(errCode));
		return 0;
	}
	CloseHandle(hFile);
	return (DWORD)FileBasic.FileAttributes;
		
}

WINBOOL
STDCALL
SetFileAttributesA(
    LPCSTR lpFileName,
    DWORD dwFileAttributes
    )
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
	return SetFileAttributesW(FileNameW, dwFileAttributes);
}


WINBOOL
STDCALL
SetFileAttributesW(
    LPCWSTR lpFileName,
    DWORD dwFileAttributes
    )
{
	IO_STATUS_BLOCK IoStatusBlock;
	FILE_BASIC_INFORMATION FileBasic;
	HANDLE hFile;
	NTSTATUS errCode;


	hFile = CreateFileW(
  		lpFileName,	
    		GENERIC_READ,	
    		FILE_SHARE_READ,	
    		NULL,	
    		OPEN_EXISTING,	
    		FILE_ATTRIBUTE_NORMAL,	
    		NULL 
   	);

	
	errCode = NtQueryInformationFile(hFile,&IoStatusBlock,&FileBasic, sizeof(FILE_BASIC_INFORMATION),FileBasicInformation);
	if ( !NT_SUCCESS(errCode) ) {
		CloseHandle(hFile);
		SetLastError(RtlNtStatusToDosError(errCode));
		return FALSE;
	}
	FileBasic.FileAttributes = dwFileAttributes;
	errCode = NtSetInformationFile(hFile,&IoStatusBlock,&FileBasic, sizeof(FILE_BASIC_INFORMATION),FileBasicInformation);
	if ( !NT_SUCCESS(errCode) ) {
		CloseHandle(hFile);
		SetLastError(RtlNtStatusToDosError(errCode));
		return FALSE;
	}
	CloseHandle(hFile);
	return TRUE;
		
}



DWORD
GetCurrentTime(VOID)
{
	NTSTATUS errCode;
	FILETIME CurrentTime;
memset(&CurrentTime,sizeof(FILETIME),0);
//	errCode = NtQuerySystemTime (
//		(TIME *)&CurrentTime
//	);
	return CurrentTime.dwLowDateTime;
}

UINT
STDCALL
GetTempFileNameA(
    LPCSTR lpPathName,
    LPCSTR lpPrefixString,
    UINT uUnique,
    LPSTR lpTempFileName
    )
{
	HANDLE hFile;
	UINT unique = uUnique;
  
	if (lpPathName == NULL)
		return 0;

  	if (uUnique == 0)
    		uUnique = GetCurrentTime();
  /*
  	sprintf(lpTempFileName,"%s\\%c%.3s%4.4x%s",
	  lpPathName,'~',lpPrefixString,uUnique,".tmp");
  */
  	if (unique)
		return uUnique;
  
  	while ((hFile = CreateFileA(lpTempFileName, GENERIC_WRITE, 0, NULL,
			     CREATE_NEW, FILE_ATTRIBUTE_TEMPORARY,
			     0)) == INVALID_HANDLE_VALUE)
    	{
  //  		wsprintfA(lpTempFileName,"%s\\%c%.3s%4.4x%s",
//	  	lpPathName,'~',lpPrefixString,++uUnique,".tmp");
    	}

  	CloseHandle((HANDLE)hFile);
  	return uUnique;
}


UINT
STDCALL
GetTempFileNameW(
    LPCWSTR lpPathName,
    LPCWSTR lpPrefixString,
    UINT uUnique,
    LPWSTR lpTempFileName
    )
{
	HANDLE hFile;
	UINT unique = uUnique;
  
	if (lpPathName == NULL)
		return 0;

  	if (uUnique == 0)
    		uUnique = GetCurrentTime();
  
  //	swprintf(lpTempFileName,L"%s\\%c%.3s%4.4x%s",
//	  lpPathName,'~',lpPrefixString,uUnique,L".tmp");
  
  	if (unique)
		return uUnique;
  
  	while ((hFile = CreateFileW(lpTempFileName, GENERIC_WRITE, 0, NULL,
			     CREATE_NEW, FILE_ATTRIBUTE_TEMPORARY,
			     0)) == INVALID_HANDLE_VALUE)
    	{
//    		wsprintfW(lpTempFileName,L"%s\\%c%.3s%4.4x%s",
//	  	lpPathName,'~',lpPrefixString,++uUnique,L".tmp");
    	}

  	CloseHandle((HANDLE)hFile);
  	return uUnique;
}

WINBOOL
STDCALL
GetFileTime(
	    HANDLE hFile,
	    LPFILETIME lpCreationTime,
	    LPFILETIME lpLastAccessTime,
	    LPFILETIME lpLastWriteTime
	    )
{
	IO_STATUS_BLOCK IoStatusBlock;
	FILE_BASIC_INFORMATION FileBasic;
	NTSTATUS errCode;
	

	
	errCode = NtQueryInformationFile(hFile,&IoStatusBlock,&FileBasic, sizeof(FILE_BASIC_INFORMATION),FileBasicInformation);
	if ( !NT_SUCCESS(errCode) ) {
		SetLastError(RtlNtStatusToDosError(errCode));
		return FALSE;
	}
	memcpy(lpCreationTime,&FileBasic.CreationTime,sizeof(FILETIME));
	memcpy(lpLastAccessTime,&FileBasic.LastAccessTime,sizeof(FILETIME));
	memcpy(lpLastWriteTime,&FileBasic.LastWriteTime,sizeof(FILETIME));
	return TRUE;
}

WINBOOL
STDCALL
SetFileTime(
	    HANDLE hFile,
	    CONST FILETIME *lpCreationTime,
	    CONST FILETIME *lpLastAccessTime,
	    CONST FILETIME *lpLastWriteTime
	    )
{
	FILE_BASIC_INFORMATION FileBasic;
	IO_STATUS_BLOCK IoStatusBlock;
	NTSTATUS errCode;



	memcpy(&FileBasic.CreationTime,lpCreationTime,sizeof(FILETIME));
	memcpy(&FileBasic.LastAccessTime,lpLastAccessTime,sizeof(FILETIME));
	memcpy(&FileBasic.LastWriteTime,lpLastWriteTime,sizeof(FILETIME));

	// shoud i initialize changetime ???
	
	errCode = NtSetInformationFile(hFile,&IoStatusBlock,&FileBasic, sizeof(FILE_BASIC_INFORMATION),FileBasicInformation);
	if ( !NT_SUCCESS(errCode) ) {
		SetLastError(RtlNtStatusToDosError(errCode));
		return FALSE;
	}
	
	return TRUE;
}

WINBOOL
STDCALL
SetEndOfFile(
	     HANDLE hFile
	     )
{
	int x = -1;
	DWORD Num;
	return WriteFile(hFile,&x,1,&Num,NULL);
}
