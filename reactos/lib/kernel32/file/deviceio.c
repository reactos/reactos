/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/kernel32/file/deviceio.c
 * PURPOSE:         Device I/O and Overlapped Result functions
 * PROGRAMMER:      Ariadne (ariadne@xs4all.nl)
 * UPDATE HISTORY:
 *                  Created 01/11/98
 */

#include <windows.h>
#include <ddk/ntddk.h>

#define NDEBUG
#include <kernel32/kernel32.h>


WINBOOL
STDCALL
DeviceIoControl(
		HANDLE hDevice,
		DWORD dwIoControlCode,
		LPVOID lpInBuffer,
		DWORD nInBufferSize,
		LPVOID lpOutBuffer,
		DWORD nOutBufferSize,
		LPDWORD lpBytesReturned,
		LPOVERLAPPED lpOverlapped
		)
{
	NTSTATUS errCode = 0;
	HANDLE hEvent = NULL;
	PIO_STATUS_BLOCK IoStatusBlock;
	IO_STATUS_BLOCK IIosb;

	WINBOOL bFsIoControlCode = FALSE;

	if ( lpBytesReturned == NULL ) {
		SetLastError(RtlNtStatusToDosError(STATUS_INVALID_PARAMETER));
		return FALSE;;
	}

    	if( ( ( dwIoControlCode >> 16 ) & FILE_DEVICE_FILE_SYSTEM ) == FILE_DEVICE_FILE_SYSTEM ) 
		bFsIoControlCode = TRUE;	
	else
		bFsIoControlCode = FALSE;
// CHECKPOINT
    	if(lpOverlapped  != NULL) {
		hEvent = lpOverlapped->hEvent;
		lpOverlapped->Internal = STATUS_PENDING;
		IoStatusBlock = (PIO_STATUS_BLOCK)lpOverlapped;
	}
	else {
		IoStatusBlock = &IIosb;
	}

// CHECKPOINT
        if(bFsIoControlCode == TRUE) {
            	errCode = NtFsControlFile(hDevice,hEvent,NULL,NULL,IoStatusBlock,dwIoControlCode,lpInBuffer, nInBufferSize, lpOutBuffer, nOutBufferSize );
        } else {   
            	errCode = NtDeviceIoControlFile(hDevice,hEvent,NULL,NULL,IoStatusBlock,dwIoControlCode, lpInBuffer, nInBufferSize, lpOutBuffer, nOutBufferSize);
        }
// CHECKPOINT
	if(errCode == STATUS_PENDING ) {
           
            	if(NtWaitForSingleObject(hDevice,FALSE,NULL) < 0) {
			*lpBytesReturned = IoStatusBlock->Information;
                	SetLastError(RtlNtStatusToDosError(errCode));
			return FALSE;;
            	}
            	
        } else if ( !NT_SUCCESS(errCode) ) {
		SetLastError(RtlNtStatusToDosError(errCode));
		return FALSE;
	}
// CHECKPOINT
        if (lpOverlapped)
                *lpBytesReturned = lpOverlapped->InternalHigh;
        else
                *lpBytesReturned = IoStatusBlock->Information;
// CHECKPOINT
        return TRUE;
}



WINBOOL 
STDCALL 
GetOverlappedResult( 
    HANDLE hFile, 
    LPOVERLAPPED lpOverlapped, 
    LPDWORD lpNumberOfBytesTransferred, 
    WINBOOL bWait 
    )
{
	DWORD WaitStatus;

	if ( lpOverlapped == NULL ) {
		SetLastError(RtlNtStatusToDosError(STATUS_INVALID_PARAMETER));
		return FALSE;
	}
        if ( lpOverlapped ->Internal == STATUS_PENDING) {
                if ( lpNumberOfBytesTransferred == 0 ) {
                        SetLastError(RtlNtStatusToDosError(STATUS_PENDING));
			return FALSE;
                }
		else if ( bWait == TRUE ) {
                	if ( lpOverlapped->hEvent != NULL ) {
                        	WaitStatus = WaitForSingleObject(lpOverlapped->hEvent,-1);
    				if ( WaitStatus ==  STATUS_TIMEOUT ) {
                        		SetLastError(ERROR_IO_INCOMPLETE);
                        		return FALSE;
                		}
				else
					return GetOverlappedResult(hFile,lpOverlapped,lpNumberOfBytesTransferred,FALSE);	
      
        		}
		}
	}
        *lpNumberOfBytesTransferred = lpOverlapped->InternalHigh;
        if ( lpOverlapped->Internal < 0 ) {
		SetLastError(RtlNtStatusToDosError(lpOverlapped->Internal));
                return FALSE;
        }
        return TRUE;
        


}



