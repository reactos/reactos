/*
* PROJECT:         Filesystem Filter Manager library
* LICENSE:         GPL - See COPYING in the top level directory
* FILE:            dll/win32/fltlib/stubs.c
* PURPOSE:         
* PROGRAMMERS:     Ged Murphy (ged.murphy@reactos.org)
*/

// Hack - our SDK reports NTDDI_VERSION as 0x05020100 (from _WIN32_WINNT 0x502)
// which doesn't pass the FLT_MGR_BASELINE check in fltkernel.h
#define NTDDI_VERSION NTDDI_WS03SP1

#include <stdarg.h>
#include "windef.h"
#include "winbase.h"

#include <fltuser.h>

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(fltlib);


_Must_inspect_result_
HRESULT
WINAPI
FilterCreate(_In_ LPCWSTR lpFilterName,
             _Outptr_ HFILTER *hFilter)
{
    UNREFERENCED_PARAMETER(lpFilterName);
    UNREFERENCED_PARAMETER(hFilter);
    return E_NOTIMPL;
}

HRESULT
WINAPI
FilterClose(_In_ HFILTER hFilter)
{
    UNREFERENCED_PARAMETER(hFilter);
    return E_NOTIMPL;
}

_Must_inspect_result_
HRESULT
WINAPI
FilterInstanceCreate(_In_ LPCWSTR lpFilterName,
                     _In_ LPCWSTR lpVolumeName,
                     _In_opt_ LPCWSTR lpInstanceName,
                     _Outptr_ HFILTER_INSTANCE *hInstance)
{
    UNREFERENCED_PARAMETER(lpFilterName);
    UNREFERENCED_PARAMETER(lpVolumeName);
    UNREFERENCED_PARAMETER(lpInstanceName);
    UNREFERENCED_PARAMETER(hInstance);
    return E_NOTIMPL;
}

HRESULT
WINAPI
FilterInstanceClose(_In_ HFILTER_INSTANCE hInstance)
{
    UNREFERENCED_PARAMETER(hInstance);
    return E_NOTIMPL;
}

_Must_inspect_result_
HRESULT
WINAPI
FilterAttach(_In_ LPCWSTR lpFilterName,
             _In_ LPCWSTR lpVolumeName,
             _In_opt_ LPCWSTR lpInstanceName,
             _In_opt_ DWORD dwCreatedInstanceNameLength,
             _Out_writes_bytes_opt_(dwCreatedInstanceNameLength) LPWSTR lpCreatedInstanceName)
{
    UNREFERENCED_PARAMETER(lpFilterName);
    UNREFERENCED_PARAMETER(lpVolumeName);
    UNREFERENCED_PARAMETER(lpInstanceName);
    UNREFERENCED_PARAMETER(dwCreatedInstanceNameLength);
    UNREFERENCED_PARAMETER(lpCreatedInstanceName);
    return E_NOTIMPL;
}

_Must_inspect_result_
HRESULT
WINAPI
FilterAttachAtAltitude(_In_ LPCWSTR lpFilterName,
                       _In_ LPCWSTR lpVolumeName,
                       _In_ LPCWSTR lpAltitude,
                       _In_opt_ LPCWSTR lpInstanceName,
                       _In_opt_ DWORD dwCreatedInstanceNameLength,
                       _Out_writes_bytes_opt_(dwCreatedInstanceNameLength) LPWSTR lpCreatedInstanceName)
{
    UNREFERENCED_PARAMETER(lpFilterName);
    UNREFERENCED_PARAMETER(lpVolumeName);
    UNREFERENCED_PARAMETER(lpAltitude);
    UNREFERENCED_PARAMETER(lpInstanceName);
    UNREFERENCED_PARAMETER(dwCreatedInstanceNameLength);
    UNREFERENCED_PARAMETER(lpCreatedInstanceName);
    return E_NOTIMPL;
}

_Must_inspect_result_
HRESULT
WINAPI
FilterDetach(_In_ LPCWSTR lpFilterName,
             _In_ LPCWSTR lpVolumeName,
             _In_opt_ LPCWSTR lpInstanceName
)
{
    UNREFERENCED_PARAMETER(lpFilterName);
    UNREFERENCED_PARAMETER(lpVolumeName);
    UNREFERENCED_PARAMETER(lpInstanceName);
    return E_NOTIMPL;
}

_Must_inspect_result_
HRESULT
WINAPI
FilterFindFirst(_In_ FILTER_INFORMATION_CLASS dwInformationClass,
                _Out_writes_bytes_to_(dwBufferSize, *lpBytesReturned) LPVOID lpBuffer,
                _In_ DWORD dwBufferSize,
                _Out_ LPDWORD lpBytesReturned,
                _Out_ LPHANDLE lpFilterFind)
{
    UNREFERENCED_PARAMETER(dwInformationClass);
    UNREFERENCED_PARAMETER(lpBuffer);
    UNREFERENCED_PARAMETER(dwBufferSize);
    UNREFERENCED_PARAMETER(lpBytesReturned);
    UNREFERENCED_PARAMETER(lpFilterFind);
    return E_NOTIMPL;
}

_Must_inspect_result_
HRESULT
WINAPI
FilterFindNext(_In_ HANDLE hFilterFind,
               _In_ FILTER_INFORMATION_CLASS dwInformationClass,
               _Out_writes_bytes_to_(dwBufferSize, *lpBytesReturned) LPVOID lpBuffer,
               _In_ DWORD dwBufferSize,
               _Out_ LPDWORD lpBytesReturned)
{
    UNREFERENCED_PARAMETER(hFilterFind);
    UNREFERENCED_PARAMETER(dwInformationClass);
    UNREFERENCED_PARAMETER(lpBuffer);
    UNREFERENCED_PARAMETER(dwBufferSize);
    UNREFERENCED_PARAMETER(lpBytesReturned);
    return E_NOTIMPL;
}

_Must_inspect_result_
HRESULT
WINAPI
FilterFindClose(_In_ HANDLE hFilterFind)
{
    UNREFERENCED_PARAMETER(hFilterFind);
    return E_NOTIMPL;
}

_Must_inspect_result_
HRESULT
WINAPI
FilterVolumeFindFirst(_In_ FILTER_VOLUME_INFORMATION_CLASS dwInformationClass,
                      _Out_writes_bytes_to_(dwBufferSize, *lpBytesReturned) LPVOID lpBuffer,
                      _In_ DWORD dwBufferSize,
                      _Out_ LPDWORD lpBytesReturned,
                      _Out_ PHANDLE lpVolumeFind)
{
    UNREFERENCED_PARAMETER(dwInformationClass);
    UNREFERENCED_PARAMETER(lpBuffer);
    UNREFERENCED_PARAMETER(dwBufferSize);
    UNREFERENCED_PARAMETER(lpBytesReturned);
    UNREFERENCED_PARAMETER(lpVolumeFind);
    return E_NOTIMPL;
}

_Must_inspect_result_
HRESULT
WINAPI
FilterVolumeFindNext(_In_ HANDLE hVolumeFind,
                     _In_ FILTER_VOLUME_INFORMATION_CLASS dwInformationClass,
                     _Out_writes_bytes_to_(dwBufferSize, *lpBytesReturned) LPVOID lpBuffer,
                     _In_ DWORD dwBufferSize,
                     _Out_ LPDWORD lpBytesReturned)
{
    UNREFERENCED_PARAMETER(hVolumeFind);
    UNREFERENCED_PARAMETER(dwInformationClass);
    UNREFERENCED_PARAMETER(lpBuffer);
    UNREFERENCED_PARAMETER(dwBufferSize);
    UNREFERENCED_PARAMETER(lpBytesReturned);
    return E_NOTIMPL;
}

HRESULT
WINAPI
FilterVolumeFindClose(_In_ HANDLE hVolumeFind)
{
    UNREFERENCED_PARAMETER(hVolumeFind);
    return E_NOTIMPL;
}

_Must_inspect_result_
HRESULT
WINAPI
FilterInstanceFindFirst(_In_ LPCWSTR lpFilterName,
                        _In_ INSTANCE_INFORMATION_CLASS dwInformationClass,
                        _Out_writes_bytes_to_(dwBufferSize, *lpBytesReturned) LPVOID lpBuffer,
                        _In_ DWORD dwBufferSize,
                        _Out_ LPDWORD lpBytesReturned,
                        _Out_ LPHANDLE lpFilterInstanceFind)
{
    UNREFERENCED_PARAMETER(lpFilterName);
    UNREFERENCED_PARAMETER(dwInformationClass);
    UNREFERENCED_PARAMETER(lpBuffer);
    UNREFERENCED_PARAMETER(dwBufferSize);
    UNREFERENCED_PARAMETER(lpBytesReturned);
    UNREFERENCED_PARAMETER(lpFilterInstanceFind);
    return E_NOTIMPL;
}

_Must_inspect_result_
HRESULT
WINAPI
FilterInstanceFindNext(_In_ HANDLE hFilterInstanceFind,
                       _In_ INSTANCE_INFORMATION_CLASS dwInformationClass,
                       _Out_writes_bytes_to_(dwBufferSize, *lpBytesReturned) LPVOID lpBuffer,
                       _In_ DWORD dwBufferSize,
                       _Out_ LPDWORD lpBytesReturned)
{

    UNREFERENCED_PARAMETER(hFilterInstanceFind);
    UNREFERENCED_PARAMETER(dwInformationClass);
    UNREFERENCED_PARAMETER(lpBuffer);
    UNREFERENCED_PARAMETER(dwBufferSize);
    UNREFERENCED_PARAMETER(lpBytesReturned);
    return E_NOTIMPL;
}

_Must_inspect_result_
HRESULT
WINAPI
FilterInstanceFindClose(_In_ HANDLE hFilterInstanceFind)
{
    UNREFERENCED_PARAMETER(hFilterInstanceFind);
    return E_NOTIMPL;
}

_Must_inspect_result_
HRESULT
WINAPI
FilterVolumeInstanceFindFirst(_In_ LPCWSTR lpVolumeName,
                              _In_ INSTANCE_INFORMATION_CLASS dwInformationClass,
                              _Out_writes_bytes_to_(dwBufferSize, *lpBytesReturned) LPVOID lpBuffer,
                              _In_ DWORD dwBufferSize,
                              _Out_ LPDWORD lpBytesReturned,
                              _Out_ LPHANDLE lpVolumeInstanceFind)
{
    UNREFERENCED_PARAMETER(lpVolumeName);
    UNREFERENCED_PARAMETER(dwInformationClass);
    UNREFERENCED_PARAMETER(lpBuffer);
    UNREFERENCED_PARAMETER(dwBufferSize);
    UNREFERENCED_PARAMETER(lpBytesReturned);
    UNREFERENCED_PARAMETER(lpVolumeInstanceFind);
    return E_NOTIMPL;
}

_Must_inspect_result_
HRESULT
WINAPI
FilterVolumeInstanceFindNext(_In_ HANDLE hVolumeInstanceFind,
                             _In_ INSTANCE_INFORMATION_CLASS dwInformationClass,
                             _Out_writes_bytes_to_(dwBufferSize, *lpBytesReturned) LPVOID lpBuffer,
                             _In_ DWORD dwBufferSize,
                             _Out_ LPDWORD lpBytesReturned)
{
    UNREFERENCED_PARAMETER(hVolumeInstanceFind);
    UNREFERENCED_PARAMETER(dwInformationClass);
    UNREFERENCED_PARAMETER(lpBuffer);
    UNREFERENCED_PARAMETER(dwBufferSize);
    UNREFERENCED_PARAMETER(lpBytesReturned);
    return E_NOTIMPL;
}

HRESULT
WINAPI
FilterVolumeInstanceFindClose(_In_ HANDLE hVolumeInstanceFind)
{
    UNREFERENCED_PARAMETER(hVolumeInstanceFind);
    return E_NOTIMPL;
}

_Must_inspect_result_
HRESULT
WINAPI
FilterGetInformation(_In_ HFILTER hFilter,
                     _In_ FILTER_INFORMATION_CLASS dwInformationClass,
                     _Out_writes_bytes_to_(dwBufferSize, *lpBytesReturned) LPVOID lpBuffer,
                     _In_ DWORD dwBufferSize,
                     _Out_ LPDWORD lpBytesReturned)
{
    UNREFERENCED_PARAMETER(hFilter);
    UNREFERENCED_PARAMETER(dwInformationClass);
    UNREFERENCED_PARAMETER(lpBuffer);
    UNREFERENCED_PARAMETER(dwBufferSize);
    UNREFERENCED_PARAMETER(lpBytesReturned);
    return E_NOTIMPL;
}

_Must_inspect_result_
HRESULT
WINAPI
FilterInstanceGetInformation(_In_ HFILTER_INSTANCE hInstance,
                             _In_ INSTANCE_INFORMATION_CLASS dwInformationClass,
                             _Out_writes_bytes_to_(dwBufferSize, *lpBytesReturned) LPVOID lpBuffer,
                             _In_ DWORD dwBufferSize,
                             _Out_ LPDWORD lpBytesReturned)
{
    UNREFERENCED_PARAMETER(hInstance);
    UNREFERENCED_PARAMETER(dwInformationClass);
    UNREFERENCED_PARAMETER(lpBuffer);
    UNREFERENCED_PARAMETER(dwBufferSize);
    UNREFERENCED_PARAMETER(lpBytesReturned);
    return E_NOTIMPL;
}


_Must_inspect_result_
HRESULT
WINAPI
FilterConnectCommunicationPort(_In_ LPCWSTR lpPortName,
                               _In_ DWORD dwOptions,
                               _In_reads_bytes_opt_(wSizeOfContext) LPCVOID lpContext,
                               _In_ WORD wSizeOfContext,
                               _In_opt_ LPSECURITY_ATTRIBUTES lpSecurityAttributes,
                               _Outptr_ HANDLE *hPort)
{
    UNREFERENCED_PARAMETER(lpPortName);
    UNREFERENCED_PARAMETER(dwOptions);
    UNREFERENCED_PARAMETER(lpContext);
    UNREFERENCED_PARAMETER(wSizeOfContext);
    UNREFERENCED_PARAMETER(lpSecurityAttributes);
    UNREFERENCED_PARAMETER(hPort);
    return E_NOTIMPL;
}

_Must_inspect_result_
HRESULT
WINAPI
FilterSendMessage(_In_ HANDLE hPort,
                  _In_reads_bytes_(dwInBufferSize) LPVOID lpInBuffer,
                  _In_ DWORD dwInBufferSize,
                  _Out_writes_bytes_to_opt_(dwOutBufferSize, *lpBytesReturned) LPVOID lpOutBuffer,
                  _In_ DWORD dwOutBufferSize,
                  _Out_ LPDWORD lpBytesReturned)
{
    UNREFERENCED_PARAMETER(hPort);
    UNREFERENCED_PARAMETER(lpInBuffer);
    UNREFERENCED_PARAMETER(dwInBufferSize);
    UNREFERENCED_PARAMETER(lpOutBuffer);
    UNREFERENCED_PARAMETER(dwOutBufferSize);
    UNREFERENCED_PARAMETER(lpBytesReturned);
    return E_NOTIMPL;
}

_Must_inspect_result_
HRESULT
WINAPI
FilterGetMessage(_In_ HANDLE hPort,
                 _Out_writes_bytes_(dwMessageBufferSize) PFILTER_MESSAGE_HEADER lpMessageBuffer,
                 _In_ DWORD dwMessageBufferSize,
                 _Inout_opt_ LPOVERLAPPED lpOverlapped)
{
    UNREFERENCED_PARAMETER(hPort);
    UNREFERENCED_PARAMETER(lpMessageBuffer);
    UNREFERENCED_PARAMETER(dwMessageBufferSize);
    UNREFERENCED_PARAMETER(lpOverlapped);
    return E_NOTIMPL;
}

_Must_inspect_result_
HRESULT
WINAPI
FilterReplyMessage(_In_ HANDLE hPort,
                   _In_reads_bytes_(dwReplyBufferSize) PFILTER_REPLY_HEADER lpReplyBuffer,
                   _In_ DWORD dwReplyBufferSize)
{
    UNREFERENCED_PARAMETER(hPort);
    UNREFERENCED_PARAMETER(lpReplyBuffer);
    UNREFERENCED_PARAMETER(dwReplyBufferSize);
    return E_NOTIMPL;
}

_Must_inspect_result_
HRESULT
WINAPI
FilterGetDosName(_In_ LPCWSTR lpVolumeName,
                 _Out_writes_(dwDosNameBufferSize) LPWSTR lpDosName,
                 _In_ DWORD dwDosNameBufferSize)
{
    UNREFERENCED_PARAMETER(lpVolumeName);
    UNREFERENCED_PARAMETER(lpDosName);
    UNREFERENCED_PARAMETER(dwDosNameBufferSize);
    return E_NOTIMPL;
}
