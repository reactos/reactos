/*
* PROJECT:         Filesystem Filter Manager library
* LICENSE:         GPL - See COPYING in the top level directory
* FILE:            dll/win32/fltlib/message.c
* PURPOSE:         Handles messaging to and from the filter manager
* PROGRAMMERS:     Ged Murphy (ged.murphy@reactos.org)
*/

#define WIN32_NO_STATUS
#include <windef.h>
#include <winbase.h>

#define NTOS_MODE_USER
#include <ndk/iofuncs.h>
#include <ndk/obfuncs.h>
#include <ndk/rtlfuncs.h>
#include <fltuser.h>
#include <fltmgr_shared.h>

#include "fltlib.h"

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
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatusBlock;
    PFILTER_PORT_DATA PortData;
    UNICODE_STRING DeviceName;
    UNICODE_STRING PortName;
    HANDLE FileHandle;
    SIZE_T PortNameSize;
    SIZE_T BufferSize;
    PCHAR Ptr;
    NTSTATUS Status;
    HRESULT hr;

    *hPort = INVALID_HANDLE_VALUE;

    /* Sanity check */
    if (lpContext && wSizeOfContext == 0)
    {
        return E_INVALIDARG;
    }

    /* Get the length of the port name */
    PortNameSize = wcslen(lpPortName) * sizeof(WCHAR);

    /* Calculate and allocate the size of the required buffer */
    BufferSize = sizeof(FILTER_PORT_DATA) + PortNameSize + wSizeOfContext;
    PortData = RtlAllocateHeap(GetProcessHeap(), 0, BufferSize);
    if (PortData == NULL) return E_OUTOFMEMORY;

    /* Clear out the buffer and find the end of the fixed struct */
    RtlZeroMemory(PortData, BufferSize);
    Ptr = (PCHAR)(PortData + 1);

    PortData->Size = BufferSize;
    PortData->Options = dwOptions;

    /* Setup the port name */
    RtlInitUnicodeString(&PortName, lpPortName);
    PortData->PortName.Buffer = (PWCH)Ptr;
    PortData->PortName.MaximumLength = PortNameSize;
    RtlCopyUnicodeString(&PortData->PortName, &PortName);
    Ptr += PortData->PortName.Length;

    /* Check if we were given a context */
    if (lpContext)
    {
        /* Add that into the buffer too */
        PortData->Context = Ptr;
        RtlCopyMemory(PortData->Context, lpContext, wSizeOfContext);
    }

    /* Initialize the object attributes */
    RtlInitUnicodeString(&DeviceName, L"\\Global??\\FltMgrMsg");
    InitializeObjectAttributes(&ObjectAttributes,
                               &DeviceName,
                               OBJ_EXCLUSIVE | OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    /* Check if we were passed any security attributes */
    if (lpSecurityAttributes)
    {
        /* Add these manually and update the flags if we were asked to make it inheritable */
        ObjectAttributes.SecurityDescriptor = lpSecurityAttributes->lpSecurityDescriptor;
        if (lpSecurityAttributes->bInheritHandle)
        {
            ObjectAttributes.Attributes |= OBJ_INHERIT;
        }
    }

    /* Now get a handle to the device */
    Status = NtCreateFile(&FileHandle,
                          SYNCHRONIZE | FILE_READ_DATA | FILE_WRITE_DATA,
                          &ObjectAttributes,
                          &IoStatusBlock,
                          0,
                          0,
                          0,
                          FILE_OPEN_IF,
                          0,
                          PortData,
                          BufferSize);
    if (NT_SUCCESS(Status))
    {
        *hPort = FileHandle;
        hr = S_OK;
    }
    else
    {
        hr = NtStatusToHResult(Status);
    }

    /* Cleanup and return */
    RtlFreeHeap(GetProcessHeap(), 0, PortData);
    return hr;
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
