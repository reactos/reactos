/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/kernel32/misc/handle.c
 * PURPOSE:         Object  functions
 * PROGRAMMER:      Ariadne ( ariadne@xs4all.nl)
 * UPDATE HISTORY:
 *                  Created 01/11/98
 */
#include <windows.h>
#include <ddk/ntddk.h>



 
WINBOOL 
WINAPI 
GetHandleInformation( 
    HANDLE hObject, 
    LPDWORD lpdwFlags 
    )
{
	OBJECT_DATA_INFORMATION HandleInfo;
	ULONG BytesWritten;
	NTSTATUS errCode;

	errCode = NtQueryObject(hObject,ObjectDataInformation, &HandleInfo, sizeof(OBJECT_DATA_INFORMATION),&BytesWritten);
	if (!NT_SUCCESS(errCode) ) {
		SetLastError(RtlNtStatusToDosError(errCode));
		return FALSE;
	}
	if ( HandleInfo.bInheritHandle )
		*lpdwFlags &= HANDLE_FLAG_INHERIT;
	if ( HandleInfo.bProtectFromClose )
		*lpdwFlags &= HANDLE_FLAG_PROTECT_FROM_CLOSE;
	return TRUE;
	
	
} 
 
WINBOOL 
STDCALL 
SetHandleInformation( 
    HANDLE hObject, 
    DWORD dwMask, 
    DWORD dwFlags 
    )
{	
	OBJECT_DATA_INFORMATION HandleInfo;
	NTSTATUS errCode;
	ULONG BytesWritten;

	errCode = NtQueryObject(hObject,ObjectDataInformation,&HandleInfo,sizeof(OBJECT_DATA_INFORMATION),&BytesWritten);
	if (!NT_SUCCESS(errCode) ) {
		SetLastError(RtlNtStatusToDosError(errCode));
		return FALSE;
	}
	if ( (dwMask & HANDLE_FLAG_INHERIT)== HANDLE_FLAG_INHERIT) {
		HandleInfo.bInheritHandle = (BOOLEAN)((dwFlags & HANDLE_FLAG_INHERIT) ==  HANDLE_FLAG_INHERIT);      
	}	
	if ( (dwMask & HANDLE_FLAG_PROTECT_FROM_CLOSE)  == HANDLE_FLAG_PROTECT_FROM_CLOSE ) {
		HandleInfo.bProtectFromClose = (BOOLEAN)((dwFlags & HANDLE_FLAG_PROTECT_FROM_CLOSE) == HANDLE_FLAG_PROTECT_FROM_CLOSE ) ; 
	}

	errCode = NtSetInformationObject(hObject,ObjectDataInformation,&HandleInfo,sizeof(OBJECT_DATA_INFORMATION));
	if (!NT_SUCCESS(errCode) ) {
		SetLastError(RtlNtStatusToDosError(errCode));
		return FALSE;
	}

	return TRUE;
}
 


WINBOOL 
STDCALL
CloseHandle(    HANDLE  hObject )
{

	NTSTATUS errCode;

        errCode = NtClose(hObject);
        if(!NT_SUCCESS(errCode)) {
          	SetLastError(RtlNtStatusToDosError(errCode));
          	return FALSE;
	}
      	
	return TRUE;
}



WINBOOL
STDCALL
DuplicateHandle(
    HANDLE hSourceProcessHandle,	
    HANDLE hSourceHandle,	
    HANDLE hTargetProcessHandle,	
    LPHANDLE lpTargetHandle,	
    DWORD dwDesiredAccess,	 
    BOOL bInheritHandle,	
    DWORD dwOptions 	
   )
{

	
	NTSTATUS errCode;
	
	errCode = NtDuplicateObject(hSourceProcessHandle,hSourceHandle,hTargetProcessHandle,lpTargetHandle, dwDesiredAccess, (BOOLEAN)bInheritHandle,dwOptions);
	if ( !NT_SUCCESS(errCode) ) {
		SetLastError(RtlNtStatusToDosError(errCode));
		return FALSE;
	}
		
			
	
	return TRUE;
		
}

UINT STDCALL
SetHandleCount(UINT nCount)
{
	return nCount;
}




 


