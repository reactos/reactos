/* $Id: deviceio.c,v 1.10 2002/10/03 19:09:04 robd Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/kernel32/file/deviceio.c
 * PURPOSE:         Device I/O and Overlapped Result functions
 * PROGRAMMER:      Ariadne (ariadne@xs4all.nl)
 * UPDATE HISTORY:
 *                  Created 01/11/98
 */

#include <ddk/ntddk.h>
#include <windows.h>

#define NDEBUG
//#define DBG
#include <kernel32/kernel32.h>
#include <kernel32/error.h>


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

    DPRINT("DeviceIoControl(hDevice %x dwIoControlCode %d lpInBuffer %x "
          "nInBufferSize %d lpOutBuffer %x nOutBufferSize %d "
          "lpBytesReturned %x lpOverlapped %x)\n",
          hDevice,dwIoControlCode,lpInBuffer,nInBufferSize,lpOutBuffer,
          nOutBufferSize,lpBytesReturned,lpOverlapped);

	if (lpBytesReturned == NULL)
	{
        DPRINT("DeviceIoControl() - returning STATUS_INVALID_PARAMETER\n");
		SetLastErrorByStatus (STATUS_INVALID_PARAMETER);
		return FALSE;
	}
	//
	// TODO: Review and approve this change by RobD. IoCtrls for Serial.sys were 
	//       going to NtFsControlFile instead of NtDeviceIoControlFile.
	//		 Don't know at this point if anything else is affected by this change.
	//
	// if (((dwIoControlCode >> 16) & FILE_DEVICE_FILE_SYSTEM) == FILE_DEVICE_FILE_SYSTEM) {
	//

	if ((dwIoControlCode >> 16) == FILE_DEVICE_FILE_SYSTEM) {

		bFsIoControlCode = TRUE;
        DPRINT("DeviceIoControl() - FILE_DEVICE_FILE_SYSTEM == TRUE %x %x\n", dwIoControlCode, dwIoControlCode >> 16);
	} else {
		bFsIoControlCode = FALSE;
        DPRINT("DeviceIoControl() - FILE_DEVICE_FILE_SYSTEM == FALSE %x %x\n", dwIoControlCode, dwIoControlCode >> 16);
	}

	if(lpOverlapped  != NULL)
	{
		hEvent = lpOverlapped->hEvent;
		lpOverlapped->Internal = STATUS_PENDING;
		IoStatusBlock = (PIO_STATUS_BLOCK)lpOverlapped;
	}
	else
	{
		IoStatusBlock = &IIosb;
	}

	if (bFsIoControlCode == TRUE)
	{
		errCode = NtFsControlFile (hDevice,
		                           hEvent,
		                           NULL,
		                           NULL,
		                           IoStatusBlock,
		                           dwIoControlCode,
		                           lpInBuffer,
		                           nInBufferSize,
		                           lpOutBuffer,
		                           nOutBufferSize);
	}
	else
	{
		errCode = NtDeviceIoControlFile (hDevice,
		                                 hEvent,
		                                 NULL,
		                                 NULL,
		                                 IoStatusBlock,
		                                 dwIoControlCode,
		                                 lpInBuffer,
		                                 nInBufferSize,
		                                 lpOutBuffer,
		                                 nOutBufferSize);
	}

	if (errCode == STATUS_PENDING)
	{
        DPRINT("DeviceIoControl() - STATUS_PENDING\n");
		if (NtWaitForSingleObject(hDevice,FALSE,NULL) < 0)
		{
			*lpBytesReturned = IoStatusBlock->Information;
			SetLastErrorByStatus (errCode);
            DPRINT("DeviceIoControl() - STATUS_PENDING wait failed.\n");
			return FALSE;
		}
	}
	else if (!NT_SUCCESS(errCode))
	{
		SetLastErrorByStatus (errCode);
        DPRINT("DeviceIoControl() - ERROR: %x\n", errCode);
		return FALSE;
	}

	if (lpOverlapped)
		*lpBytesReturned = lpOverlapped->InternalHigh;
	else
		*lpBytesReturned = IoStatusBlock->Information;

	return TRUE;
}


WINBOOL
STDCALL
GetOverlappedResult (
	HANDLE		hFile,
	LPOVERLAPPED	lpOverlapped,
	LPDWORD		lpNumberOfBytesTransferred,
	WINBOOL		bWait
	)
{
	DWORD WaitStatus;

	if (lpOverlapped == NULL)
	{
		SetLastErrorByStatus(STATUS_INVALID_PARAMETER);
		return FALSE;
	}

	if (lpOverlapped ->Internal == STATUS_PENDING)
	{
		if (lpNumberOfBytesTransferred == 0)
		{
			SetLastErrorByStatus (STATUS_PENDING);
			return FALSE;
		}
		else if (bWait == TRUE)
		{
			if (lpOverlapped->hEvent != NULL)
			{
				WaitStatus = WaitForSingleObject (lpOverlapped->hEvent,
				                                  -1);
				if (WaitStatus ==  STATUS_TIMEOUT)
				{
					SetLastError (ERROR_IO_INCOMPLETE);
					return FALSE;
				}
				else
					return GetOverlappedResult (hFile,
					                            lpOverlapped,
					                            lpNumberOfBytesTransferred,
					                            FALSE);
			}
		}
	}

	*lpNumberOfBytesTransferred = lpOverlapped->InternalHigh;

	if (lpOverlapped->Internal < 0)
	{
		SetLastErrorByStatus (lpOverlapped->Internal);
		return FALSE;
	}

	return TRUE;
}

/* EOF */


