/*
* PROJECT:         Filesystem Filter Manager library
* LICENSE:         GPL - See COPYING in the top level directory
* FILE:            dll/win32/fltlib/fltlib.c
* PURPOSE:         
* PROGRAMMERS:     Ged Murphy (ged.murphy@reactos.org)
*/

#include <stdarg.h>

#define WIN32_NO_STATUS

#include "windef.h"
#include "winbase.h"

#define NTOS_MODE_USER
#include <ndk/iofuncs.h>
#include <ndk/obfuncs.h>
#include <ndk/rtlfuncs.h>

#include "wine/debug.h"


/* DATA ****************************************************************************/

WINE_DEFAULT_DEBUG_CHANNEL(fltlib);

//MOVEME
#define IOCTL_LOAD_FILTER       CTL_CODE(FILE_DEVICE_DISK_FILE_SYSTEM, 0x01, 0, FILE_WRITE_DATA) //88004
#define IOCTL_UNLOAD_FILTER     CTL_CODE(FILE_DEVICE_DISK_FILE_SYSTEM, 0x02, 0, FILE_WRITE_DATA) //88008
#define IOCTL_CREATE_FILTER     CTL_CODE(FILE_DEVICE_DISK_FILE_SYSTEM, 0x03, 0, FILE_READ_DATA)  //8400c

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

static
HRESULT
FilterLoadUnload(_In_z_ LPCWSTR lpFilterName,
                 _In_ BOOL Load)
{
    PUNICODE_STRING FilterName;
    HANDLE hFltMgr;
    DWORD BufferLength;
    DWORD dwError;

    /* Get a handle to the filter manager */
    hFltMgr = CreateFileW(lpFilterName,
                          GENERIC_READ,
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
    BufferLength = wcslen(lpFilterName) * sizeof(WCHAR);
    FilterName = RtlAllocateHeap(GetProcessHeap(),
                                 0,
                                 BufferLength + sizeof(UNICODE_STRING));
    if (FilterName == NULL)
    {
        CloseHandle(hFltMgr);
        return HRESULT_FROM_WIN32(ERROR_OUTOFMEMORY);
    }

    /* Build up the filter name into a real life string */
    FilterName->Buffer = (PWCH)(FilterName + 1);
    FilterName->Length = BufferLength;
    FilterName->MaximumLength = BufferLength;
    RtlCopyMemory(FilterName->Buffer, lpFilterName, BufferLength);

    /* Tell the filter manager to load the filter for us */
    dwError = SendIoctl(hFltMgr,
                        Load ? IOCTL_LOAD_FILTER : IOCTL_UNLOAD_FILTER,
                        FilterName,
                        BufferLength + sizeof(UNICODE_STRING));

    /* Cleaup and bail*/
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