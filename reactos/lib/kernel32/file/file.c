/*
 * Win32 File Api functions
 * Author: Boudewijn Dekker
 */

#undef WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <ddk/ntddk.h>


BOOLEAN  bIsFileApiAnsi; // set the file api to ansi or oem


WINBASEAPI
VOID
WINAPI
SetFileApisToOEM(VOID)
{
	bIsFileApiAnsi = FALSE;
	return;	
}

HANDLE STDCALL CreateFileA(LPCSTR lpFileName,
			   DWORD dwDesiredAccess,
			   DWORD dwShareMode,
			   LPSECURITY_ATTRIBUTES lpSecurityAttributes,
			   DWORD dwCreationDisposition,
			   DWORD dwFlagsAndAttributes,
			   HANDLE hTemplateFile)
{
   HANDLE FileHandle;
   NTSTATUS Status;
   WCHAR FileNameW[255];
   OBJECT_ATTRIBUTES ObjectAttributes;
   IO_STATUS_BLOCK IoStatusBlock;
   ULONG i = 0;
   UNICODE_STRING FileNameString;
   ULONG Flags = 0;
   
   if (!(dwFlagsAndAttributes & FILE_FLAG_OVERLAPPED))
     {
	Flags = Flags | FILE_SYNCHRONOUS_IO_ALERT;
     }
   
   FileNameString.Length = 0;
   
   while ((*lpFileName)!=0)
     {
	FileNameW[i] = *lpFileName;
	lpFileName++;
	i++;
	FileNameString.Length++;
     }
   FileNameW[i] = 0;
   FileNameString.Length++;
   
   FileNameString.Buffer = &FileNameW;
   FileNameString.MaximumLength = FileNameString.Length;
   
   ObjectAttributes.RootDirectory = NULL;
   ObjectAttributes.ObjectName = &FileNameString;
   
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

WINBASEAPI
VOID
WINAPI
SetFileApisToANSI(VOID)
{
	bIsFileApiAnsi = TRUE;
	return;	
}


WINBASEAPI
WINBOOL
STDCALL
AreFileApisANSI(VOID)
{
	return  bIsFileApiAnsi;
	
}




BOOL STDCALL WriteFile(HANDLE  hFile,	
		       LPCVOID lpBuffer,	
		       DWORD nNumberOfBytesToWrite,
		       LPDWORD lpNumberOfBytesWritten,	
		       LPOVERLAPPED lpOverLapped)
{
   //FIXME: WriteFile should write to a console if appropriate
   LARGE_INTEGER Offset;
   HANDLE hEvent = NULL;
   NTSTATUS errCode;
	
   if (lpOverLapped != NULL ) 
     {
	Offset.LowPart = lpOverLapped->Offset;
	Offset.HighPart = lpOverLapped->OffsetHigh;
	lpOverLapped->Internal = STATUS_PENDING;
	hEvent= lpOverLapped->hEvent;
     }
   errCode = NtWriteFile(hFile,hEvent,NULL,NULL,
			 (PIO_STATUS_BLOCK)lpOverLapped,
			 lpBuffer, 
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
//FIXME ReadFile should write to a console if appropriate
   HANDLE hEvent = NULL;
   LARGE_INTEGER Offset;
   NTSTATUS errCode;
   PIO_STATUS_BLOCK IoStatusBlock;
   IO_STATUS_BLOCK IIosb;
   OVERLAPPED IOverlapped;
   
   if ( lpOverLapped != NULL ) 
     {
	Offset.LowPart = lpOverLapped->Offset;
	Offset.HighPart = lpOverLapped->OffsetHigh;
	lpOverLapped->Internal = STATUS_PENDING;
	hEvent = lpOverLapped->hEvent;
	IoStatusBlock = (PIO_STATUS_BLOCK)lpOverLapped;
     }
   else
     {
	IoStatusBlock = &IIosb;
     }
				 	     
   errCode = NtReadFile(hFile,
			hEvent,
			NULL,
			NULL,
			IoStatusBlock,
			lpBuffer,
			nNumberOfBytesToRead,
			&Offset,
			NULL);
   if ( errCode < 0 )  
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
		
	if ( lpOverLapped != NULL ) {
		Offset.LowPart = lpOverLapped->Offset;
		Offset.HighPart = lpOverLapped->OffsetHigh;
		lpOverLapped->Internal = STATUS_PENDING;
		hEvent = lpOverLapped->hEvent;
	}
				 
		

	errCode = NtReadFile(hFile,
			     hEvent,
			     (PIO_APC_ROUTINE)lpCompletionRoutine,
			     NULL,
			     (PIO_STATUS_BLOCK)lpOverLapped,
			     lpBuffer,
			     nNumberOfBytesToRead,
			     &Offset,
			     NULL);
	if ( errCode < 0 )  {
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
   
   Offset.LowPart = lpOverlapped->Offset;
   Offset.HighPart = lpOverlapped->OffsetHigh;
   
   if ( (dwFlags & LOCKFILE_FAIL_IMMEDIATELY) == LOCKFILE_FAIL_IMMEDIATELY )
     LockImmediate = TRUE;
   else
     LockImmediate = FALSE;
   
   if ( (dwFlags & LOCKFILE_EXCLUSIVE_LOCK) == LOCKFILE_EXCLUSIVE_LOCK )
     LockExclusive = TRUE;
   else
     LockExclusive = FALSE;
   
   BytesToLock.LowPart = nNumberOfBytesToLockLow;
   BytesToLock.HighPart = nNumberOfBytesToLockHigh;
   
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
   if ( errCode < 0 ) 
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



WINBOOL STDCALL UnlockFileEx(HANDLE hFile,
			     DWORD dwReserved,
			     DWORD nNumberOfBytesToUnLockLow,
			     DWORD nNumberOfBytesToUnLockHigh,
			     LPOVERLAPPED lpOverlapped)
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
   
   BytesToUnLock.LowPart = nNumberOfBytesToUnLockLow;
   BytesToUnLock.HighPart = nNumberOfBytesToUnLockHigh;
   
   StartAddress.LowPart = lpOverlapped->Offset;
   StartAddress.HighPart = lpOverlapped->OffsetHigh;
   
   errCode = NtUnlockFile(hFile,
			  (PIO_STATUS_BLOCK)lpOverlapped,
			  StartAddress,
			  BytesToUnLock,
			  NULL);
   if ( errCode < 0 ) {
      SetLastError(RtlNtStatusToDosError(errCode));
      return FALSE;
   }
   
   return TRUE;
}


