/*
 * PROJECT:     Filesystem Filter Manager library
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Implementing Filter Manager interfaces in user-mode
 * COPYRIGHT:   Ged Murphy <ged.murphy@reactos.org>
 *              Copyright 2023 Ratin Gao <ratin@knsoft.org>
 */

#include <stdarg.h>

#define WIN32_NO_STATUS

#include <windef.h>
#include <winbase.h>

#define NTOS_MODE_USER
#include <ndk/iofuncs.h>
#include <ndk/obfuncs.h>
#include <ndk/rtlfuncs.h>
#include <fltmgr_shared.h>

#include <pseh/pseh2.h>

/* DATA ****************************************************************************/

#define NT_FACILITY(Status) ((((ULONG)(Status)) >> 16) & 0xFFF)

static
HRESULT
FilterLoadUnload(_In_z_ LPCWSTR lpFilterName,
                 _In_ BOOL Load);

static
HRESULT
FilterpDeviceIoControl(
    _In_ HANDLE hFltMgr,
    _In_ DWORD dwControlCode,
    _In_reads_bytes_opt_(nInBufferSize) LPVOID lpInBuffer,
    _In_ DWORD nInBufferSize,
    _Out_writes_bytes_to_opt_(nOutBufferSize, *lpBytesReturned) LPVOID lpOutBuffer,
    _In_ DWORD nOutBufferSize,
    _Out_opt_ LPDWORD lpBytesReturned,
    _Inout_opt_ LPOVERLAPPED lpOverlapped);


/* PUBLIC FUNCTIONS ****************************************************************/

_Must_inspect_result_
HRESULT
WINAPI
FilterLoad(_In_ LPCWSTR lpFilterName)
{
    return FilterLoadUnload(lpFilterName, TRUE);
}

_Must_inspect_result_
HRESULT
WINAPI
FilterUnload(_In_ LPCWSTR lpFilterName)
{
    return FilterLoadUnload(lpFilterName, FALSE);
}


/* PRIVATE FUNCTIONS ****************************************************************/

HRESULT
FilterHResultFromNtStatus(_In_ NTSTATUS Status)
{
    HRESULT Result;
    ULONG DosError;

    /* Check facility code of the status */
    if (NT_FACILITY(Status))
    {
        if (NT_FACILITY(Status) == FACILITY_FILTER_MANAGER)
        {
            /* Translate FilterManager specified status (STATUS_FLT_XXX) case-by-case */
            if (Status == STATUS_FLT_BUFFER_TOO_SMALL)
            {
                Result = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
            }
            else
            {
                /* Translate other FilterManager error codes by FILTER_HRESULT_FROM_FLT_NTSTATUS macro */
                Result = FILTER_HRESULT_FROM_FLT_NTSTATUS(Status);
            }
        }
        else
        {
            /* Facility is not FilterManager, return E_FAIL */
            Result = E_FAIL;
        }
    }
    else if (Status == STATUS_TIMEOUT)
    {
        Result = HRESULT_FROM_WIN32(WAIT_TIMEOUT);
    }
    else
    {
        /* Translate status to Win32 error */
        DosError = RtlNtStatusToDosError(Status);

        /* If RtlNtStatusToDosError didn't translate the status, we return the return value as-is */
        if ((LONG)(DosError) <= 0)
        {
            return DosError;
        }

        /* Convert Win32 error to HRESULT */
        Result = HRESULT_FROM_WIN32(DosError);
    }

    return Result;
}

static
HRESULT
FilterLoadUnload(_In_z_ LPCWSTR lpFilterName,
                 _In_ BOOL Load)
{
    PFILTER_NAME FilterName;
    HANDLE hFltMgr;
    SIZE_T StringLength;
    SIZE_T BufferLength;
    DWORD dwBytesReturned;
    DWORD dwError;
    HRESULT hr;

    /* Get a handle to the filter manager */
    hFltMgr = CreateFileW(L"\\\\.\\fltmgr",
                          GENERIC_WRITE,
                          FILE_SHARE_WRITE,
                          NULL,
                          OPEN_EXISTING,
                          FILE_ATTRIBUTE_NORMAL,
                          &hFltMgr);
    if (hFltMgr == INVALID_HANDLE_VALUE)
    {
        dwError = GetLastError();
        return HRESULT_FROM_WIN32(dwError);
    }

    /* Calc and allocate a buffer to hold our filter name */
    StringLength = wcslen(lpFilterName) * sizeof(WCHAR);
    BufferLength = StringLength + sizeof(FILTER_NAME);
    FilterName = RtlAllocateHeap(GetProcessHeap(),
                                 0,
                                 BufferLength);
    if (FilterName == NULL)
    {
        CloseHandle(hFltMgr);
        return HRESULT_FROM_WIN32(ERROR_OUTOFMEMORY);
    }

    /* Build up the filter name */
    FilterName->Length = StringLength;
    CopyMemory(FilterName->FilterName, lpFilterName, StringLength);

    /* Tell the filter manager to load the filter for us */
    hr = FilterpDeviceIoControl(hFltMgr,
                                Load ? IOCTL_FILTER_LOAD : IOCTL_FILTER_UNLOAD,
                                FilterName,
                                BufferLength,
                                NULL,
                                0,
                                &dwBytesReturned,
                                NULL);

    /* Cleanup and bail */
    RtlFreeHeap(GetProcessHeap(), 0, FilterName);
    CloseHandle(hFltMgr);

    return hr;
}

static
HRESULT
FilterpDeviceIoControl(
    _In_ HANDLE hFltMgr,
    _In_ DWORD dwControlCode,
    _In_reads_bytes_opt_(nInBufferSize) LPVOID lpInBuffer,
    _In_ DWORD nInBufferSize,
    _Out_writes_bytes_to_opt_(nOutBufferSize, *lpBytesReturned) LPVOID lpOutBuffer,
    _In_ DWORD nOutBufferSize,
    _Out_opt_ LPDWORD lpBytesReturned,
    _Inout_opt_ LPOVERLAPPED lpOverlapped)
{
    NTSTATUS Status;
    DEVICE_TYPE DeviceType;
    PVOID ApcContext;
    IO_STATUS_BLOCK IoStatusBlock;

    /* Get device type of control code */
    DeviceType = DEVICE_TYPE_FROM_CTL_CODE(dwControlCode);

    /* Check for asynchronous operation */
    if (lpOverlapped)
    {
        /* Set pending status */
        lpOverlapped->Internal = STATUS_PENDING;

        /* No completion port notification if low-order bit of hEvent was set */
        ApcContext = (((ULONG_PTR)lpOverlapped->hEvent & 0x1) ? NULL : lpOverlapped);

        /* Send file system control or device control according to the device type */
        if (DeviceType == FILE_DEVICE_FILE_SYSTEM)
        {
            Status = NtFsControlFile(hFltMgr,
                                     lpOverlapped->hEvent,
                                     NULL,
                                     ApcContext,
                                     (PIO_STATUS_BLOCK)lpOverlapped,
                                     dwControlCode,
                                     lpInBuffer,
                                     nInBufferSize,
                                     lpOutBuffer,
                                     nOutBufferSize);
        }
        else
        {
            Status = NtDeviceIoControlFile(hFltMgr,
                                           lpOverlapped->hEvent,
                                           NULL,
                                           ApcContext,
                                           (PIO_STATUS_BLOCK)lpOverlapped,
                                           dwControlCode,
                                           lpInBuffer,
                                           nInBufferSize,
                                           lpOutBuffer,
                                           nOutBufferSize);
        }

        /* Return the number of bytes transferred if no error */
        if (!NT_ERROR(Status) && lpBytesReturned)
        {
            /* Protect with SEH */
            _SEH2_TRY
            {
                /* Return the bytes */
                *lpBytesReturned = (DWORD)lpOverlapped->InternalHigh;
            }
            _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
            {
                /* Return zero bytes */
                *lpBytesReturned = 0;
            }
            _SEH2_END;
        }
    }
    else
    {
        /* Set to 0 as default but seems Windows didn't set this */
        IoStatusBlock.Information = 0;

        /* Send file system control or device control according to the device type */
        if (DeviceType == FILE_DEVICE_FILE_SYSTEM)
        {
            Status = NtFsControlFile(hFltMgr,
                                     NULL,
                                     NULL,
                                     NULL,
                                     &IoStatusBlock,
                                     dwControlCode,
                                     lpInBuffer,
                                     nInBufferSize,
                                     lpOutBuffer,
                                     nOutBufferSize);
        }
        else
        {
            Status = NtDeviceIoControlFile(hFltMgr,
                                           NULL,
                                           NULL,
                                           NULL,
                                           &IoStatusBlock,
                                           dwControlCode,
                                           lpInBuffer,
                                           nInBufferSize,
                                           lpOutBuffer,
                                           nOutBufferSize);
        }

        /* Wait if operation still in progress */
        if (Status == STATUS_PENDING)
        {
            Status = NtWaitForSingleObject(hFltMgr, FALSE, NULL);
            if (NT_SUCCESS(Status))
            {
                Status = IoStatusBlock.Status;
            }
        }

        /* IoStatusBlock.Information is request-dependent and we already set it to 0 at the beginning */
        *lpBytesReturned = (DWORD)IoStatusBlock.Information;
    }

    return ((Status == STATUS_SUCCESS) ? S_OK : FilterHResultFromNtStatus(Status));
}

BOOL
WINAPI
DllMain(_In_ HINSTANCE hinstDLL,
        _In_ DWORD dwReason,
        _In_ LPVOID lpvReserved)
{
    switch (dwReason)
    {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hinstDLL);
        break;
    }

    return TRUE;
}
