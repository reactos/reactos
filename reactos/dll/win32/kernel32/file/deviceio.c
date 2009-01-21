/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/kernel32/file/deviceio.c
 * PURPOSE:         Device I/O and Overlapped Result functions
 * PROGRAMMER:      Ariadne (ariadne@xs4all.nl)
 * UPDATE HISTORY:
 *                  Created 01/11/98
 */

#include <k32.h>
#include <wine/debug.h>

WINE_DEFAULT_DEBUG_CHANNEL(kernel32file);

/*
 * @implemented
 */
BOOL
WINAPI
DeviceIoControl(IN HANDLE hDevice,
                IN DWORD dwIoControlCode,
                IN LPVOID lpInBuffer  OPTIONAL,
                IN DWORD nInBufferSize  OPTIONAL,
                OUT LPVOID lpOutBuffer  OPTIONAL,
                IN DWORD nOutBufferSize  OPTIONAL,
                OUT LPDWORD lpBytesReturned  OPTIONAL,
                IN LPOVERLAPPED lpOverlapped  OPTIONAL)
{
   BOOL FsIoCtl;
   NTSTATUS Status;

   FsIoCtl = ((dwIoControlCode >> 16) == FILE_DEVICE_FILE_SYSTEM);

   if (lpBytesReturned != NULL)
     {
        *lpBytesReturned = 0;
     }

   if (lpOverlapped != NULL)
     {
        PVOID ApcContext;

        lpOverlapped->Internal = STATUS_PENDING;
        ApcContext = (((ULONG_PTR)lpOverlapped->hEvent & 0x1) ? NULL : lpOverlapped);

        if (FsIoCtl)
          {
             Status = NtFsControlFile(hDevice,
                                      lpOverlapped->hEvent,
                                      NULL,
                                      ApcContext,
                                      (PIO_STATUS_BLOCK)lpOverlapped,
                                      dwIoControlCode,
                                      lpInBuffer,
                                      nInBufferSize,
                                      lpOutBuffer,
                                      nOutBufferSize);
          }
        else
          {
             Status = NtDeviceIoControlFile(hDevice,
                                            lpOverlapped->hEvent,
                                            NULL,
                                            ApcContext,
                                            (PIO_STATUS_BLOCK)lpOverlapped,
                                            dwIoControlCode,
                                            lpInBuffer,
                                            nInBufferSize,
                                            lpOutBuffer,
                                            nOutBufferSize);
          }

        /* return FALSE in case of failure and pending operations! */
        if (!NT_SUCCESS(Status) || Status == STATUS_PENDING)
          {
             SetLastErrorByStatus(Status);
             return FALSE;
          }

        if (lpBytesReturned != NULL)
          {
             *lpBytesReturned = lpOverlapped->InternalHigh;
          }
     }
   else
     {
        IO_STATUS_BLOCK Iosb;

        if (FsIoCtl)
          {
             Status = NtFsControlFile(hDevice,
                                      NULL,
                                      NULL,
                                      NULL,
                                      &Iosb,
                                      dwIoControlCode,
                                      lpInBuffer,
                                      nInBufferSize,
                                      lpOutBuffer,
                                      nOutBufferSize);
          }
        else
          {
             Status = NtDeviceIoControlFile(hDevice,
                                            NULL,
                                            NULL,
                                            NULL,
                                            &Iosb,
                                            dwIoControlCode,
                                            lpInBuffer,
                                            nInBufferSize,
                                            lpOutBuffer,
                                            nOutBufferSize);
          }

        /* wait in case operation is pending */
        if (Status == STATUS_PENDING)
          {
             Status = NtWaitForSingleObject(hDevice,
                                            FALSE,
                                            NULL);
             if (NT_SUCCESS(Status))
               {
                  Status = Iosb.Status;
               }
          }

        if (NT_SUCCESS(Status))
          {
             /* lpBytesReturned must not be NULL here, in fact Win doesn't
                check that case either and crashes (only after the operation
                completed) */
             *lpBytesReturned = Iosb.Information;
          }
        else
          {
             SetLastErrorByStatus(Status);
             return FALSE;
          }
     }

   return TRUE;
}


/*
 * @implemented
 */
BOOL
WINAPI
GetOverlappedResult (
  IN HANDLE   hFile,
	IN LPOVERLAPPED	lpOverlapped,
	OUT LPDWORD		lpNumberOfBytesTransferred,
	IN BOOL		bWait
	)
{
	DWORD WaitStatus;
  HANDLE hObject;

  if (lpOverlapped->Internal == STATUS_PENDING)
  {
    if (!bWait)
    {
      /* can't use SetLastErrorByStatus(STATUS_PENDING) here,
      since STATUS_PENDING translates to ERROR_IO_PENDING */
      SetLastError(ERROR_IO_INCOMPLETE);
      return FALSE;
    }

    hObject = lpOverlapped->hEvent ? lpOverlapped->hEvent : hFile;

    /* Wine delivers pending APC's while waiting, but Windows does
    not, nor do we... */
    WaitStatus = WaitForSingleObject(hObject, INFINITE);

    if (WaitStatus == WAIT_FAILED)
    {
      WARN("Wait failed!\n");
      /* WaitForSingleObjectEx sets the last error */
      return FALSE;
    }
  }

  *lpNumberOfBytesTransferred = lpOverlapped->InternalHigh;

  if (!NT_SUCCESS(lpOverlapped->Internal))
  {
    SetLastErrorByStatus(lpOverlapped->Internal);
    return FALSE;
  }

	return TRUE;
}

/* EOF */


