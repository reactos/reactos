/*
* PROJECT:         Filesystem Filter Manager library
* LICENSE:         GPL - See COPYING in the top level directory
* FILE:            dll/win32/fltlib/fltlib.c
* PURPOSE:
* PROGRAMMERS:     Ged Murphy (ged.murphy@reactos.org)
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

/* DATA ****************************************************************************/

static
HRESULT
FilterLoadUnload(_In_z_ LPCWSTR lpFilterName,
                 _In_ BOOL Load);

static
DWORD
SendIoctl(_In_ HANDLE Handle,
          _In_ ULONG IoControlCode,
          _In_reads_bytes_opt_(BufferSize) LPVOID lpInBuffer,
          _In_ DWORD BufferSize);


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
NtStatusToHResult(_In_ NTSTATUS Status)
{
    HRESULT hr;
    hr = RtlNtStatusToDosError(Status);
    if (hr != ERROR_SUCCESS)
    {
        hr = (ULONG)hr | 0x80070000;
    }
    return hr;
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
    DWORD dwError;

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
    dwError = SendIoctl(hFltMgr,
                        Load ? IOCTL_FILTER_LOAD : IOCTL_FILTER_UNLOAD,
                        FilterName,
                        BufferLength);

    /* Cleanup and bail*/
    RtlFreeHeap(GetProcessHeap(), 0, FilterName);
    CloseHandle(hFltMgr);

    return HRESULT_FROM_WIN32(dwError);
}

static
DWORD
SendIoctl(_In_ HANDLE Handle,
          _In_ ULONG IoControlCode,
          _In_reads_bytes_opt_(BufferSize) LPVOID lpInBuffer,
          _In_ DWORD BufferSize)
{
    IO_STATUS_BLOCK IoStatusBlock;
    NTSTATUS Status;

    Status = NtDeviceIoControlFile(Handle,
                                   NULL,
                                   NULL,
                                   NULL,
                                   &IoStatusBlock,
                                   IoControlCode,
                                   lpInBuffer,
                                   BufferSize,
                                   NULL,
                                   0);
    if (Status == STATUS_PENDING)
    {
        Status = NtWaitForSingleObject(Handle, FALSE, NULL);
        if (NT_SUCCESS(Status))
        {
            Status = IoStatusBlock.Status;
        }
    }

    return RtlNtStatusToDosError(Status);
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