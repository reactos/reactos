/* $Id: deviceio.c,v 1.8 2002/09/07 15:12:26 chorns Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/kernel32/file/deviceio.c
 * PURPOSE:         Device I/O and Overlapped Result functions
 * PROGRAMMER:      Ariadne (ariadne@xs4all.nl)
 * UPDATE HISTORY:
 *                  Created 01/11/98
 */

#include <windows.h>
#define NTOS_USER_MODE
#include <ntos.h>

#define NDEBUG
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

	if (lpBytesReturned == NULL)
	{
		SetLastErrorByStatus (STATUS_INVALID_PARAMETER);
		return FALSE;
	}

	if (((dwIoControlCode >> 16) & FILE_DEVICE_FILE_SYSTEM) == FILE_DEVICE_FILE_SYSTEM)
		bFsIoControlCode = TRUE;
	else
		bFsIoControlCode = FALSE;

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
		if (NtWaitForSingleObject(hDevice,FALSE,NULL) < 0)
		{
			*lpBytesReturned = IoStatusBlock->Information;
			SetLastErrorByStatus (errCode);
			return FALSE;
		}
	}
	else if (!NT_SUCCESS(errCode))
	{
		SetLastErrorByStatus (errCode);
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


