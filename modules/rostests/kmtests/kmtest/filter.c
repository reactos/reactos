/*
 * PROJECT:         ReactOS kernel-mode tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         File system filter implementation of the original service.c file
 * PROGRAMMER:      Thomas Faber <thomas.faber@reactos.org>
 *                  Ged Murphy <gedmurphy@reactos.org>
 */

//#include <fltuser.h>
#include <kmt_test.h>
#include "kmtest.h"

#include <assert.h>

#define SERVICE_ACCESS (SERVICE_START | SERVICE_STOP | DELETE)



/**
 * @name KmtFltLoad
 *
 * Start the specified filter driver by name
 *
 * @param ServiceName
 *        The name of the filter to start
 *
 * @return Win32 error code
 */
DWORD
KmtFltLoad(
    _In_z_ PCWSTR ServiceName)
{
    HRESULT hResult;
    DWORD Error = ERROR_SUCCESS;

    assert(ServiceName);

    hResult = FilterLoad(ServiceName);
    Error = SCODE_CODE(hResult);

    return Error;
}
#if 0
/**
 * @name KmtFltCreateAndStartService
 *
 * Create and load the specified filter driver and return a handle to it
 *
 * @param ServiceName
 *        Name of the service to create
 * @param ServicePath
 *        File name of the driver, relative to the current directory
 * @param DisplayName
 *        Service display name
 * @param ServiceHandle
 *        Pointer to a variable to receive the handle to the service
 * @param RestartIfRunning
 *        TRUE to stop and restart the service if it is already running
 *
 * @return Win32 error code
 */
DWORD
KmtFltCreateAndStartService(
    _In_z_ PCWSTR ServiceName,
    _In_z_ PCWSTR ServicePath,
    _In_z_ PCWSTR DisplayName OPTIONAL,
    _Out_ SC_HANDLE *ServiceHandle,
    _In_ BOOLEAN RestartIfRunning)
{
    DWORD Error = ERROR_SUCCESS;

    assert(ServiceHandle);

    Error = KmtFltCreateService(ServiceName, ServicePath, DisplayName, ServiceHandle);

    if (Error == ERROR_SERVICE_EXISTS)
        *ServiceHandle = OpenService(ScmHandle, ServiceName, SERVICE_ACCESS);

    if (Error && Error != ERROR_SERVICE_EXISTS)
        goto cleanup;

    Error = KmtFltLoad(ServiceName);

    if (Error != ERROR_SERVICE_ALREADY_RUNNING)
        goto cleanup;

    Error = ERROR_SUCCESS;

    if (!RestartIfRunning)
        goto cleanup;

    Error = KmtFltUnload(ServiceName);
    if (Error)
        goto cleanup;

    Error = KmtFltLoad(ServiceName);
    if (Error)
        goto cleanup;

cleanup:
    assert(Error);
    return Error;
}
#endif

/**
 * @name KmtFltConnect
 *
 * Create a comms connection to the specified filter
 *
 * @param ServiceName
 *        Name of the filter to connect to
 * @param hPort
 *        Handle to the filter's comms port
 *
 * @return Win32 error code
 */
DWORD
KmtFltConnect(
    _In_z_ PCWSTR ServiceName,
    _Out_ HANDLE *hPort)
{
    HRESULT hResult;
    DWORD Error;

    assert(ServiceName);
    assert(hPort);

    hResult = FilterConnectCommunicationPort(ServiceName,
                                             0,
                                             NULL,
                                             0,
                                             NULL,
                                             hPort);
    Error = SCODE_CODE(hResult);

    return Error;
}

/**
 * @name KmtFltDisconnect
 *
 * Disconenct from the comms port
 *
 * @param hPort
 *        Handle to the filter's comms port
 *
 * @return Win32 error code
 */
DWORD
KmtFltDisconnect(
    _In_ HANDLE hPort)
{
    DWORD Error = ERROR_SUCCESS;

    assert(hPort);

    if (!CloseHandle(hPort))
    {
        Error = GetLastError();
    }

    return Error;
}

/**
 * @name KmtFltSendMessage
 *
 * Sneds a message to a filter driver
 *
 * @param hPort
 *        Handle to the filter's comms port
 * @InBuffer
 *        Pointer to a buffer to send to the filter
 * @InBufferSize
 *        Size of the buffer pointed to by InBuffer
 * @OutBuffer
 *        Pointer to a buffer to receive reply data from the filter
 * @OutBufferSize
 *        Size of the buffer pointed to by OutBuffer
 * @BytesReturned
 *        Number of bytes written in the reply buffer
 *
 * @return Win32 error code
 */
DWORD
KmtFltSendMessage(
    _In_ HANDLE hPort,
    _In_reads_bytes_(dwInBufferSize) LPVOID InBuffer,
    _In_ DWORD InBufferSize,
    _Out_writes_bytes_to_opt_(dutBufferSize, *BytesReturned) LPVOID OutBuffer,
    _In_ DWORD OutBufferSize,
    _Out_opt_ LPDWORD BytesReturned)
{
    DWORD BytesRet;
    HRESULT hResult;
    DWORD Error;

    assert(hPort);
    assert(InBuffer);
    assert(InBufferSize);

    if (BytesReturned) *BytesReturned = 0;

    hResult = FilterSendMessage(hPort,
                                InBuffer,
                                InBufferSize,
                                OutBuffer,
                                OutBufferSize,
                                &BytesRet);

    Error = SCODE_CODE(hResult);
    if (Error == ERROR_SUCCESS)
    {
        if (BytesRet)
        {
            *BytesReturned = BytesRet;
        }
    }

    return Error;
}

/**
 * @name KmtFltGetMessage
 *
 * Gets a message from a filter driver
 *
 * @param hPort
 *        Handle to the filter's comms port
 * @MessageBuffer
 *        Pointer to a buffer to receive the data from the filter
 * @MessageBufferSize
 *        Size of the buffer pointed to by MessageBuffer
 * @Overlapped
 *        Pointer to an overlapped structure
 *
 * @return Win32 error code
 */
DWORD
KmtFltGetMessage(
    _In_ HANDLE hPort,
    _Out_writes_bytes_(MessageBufferSize) PFILTER_MESSAGE_HEADER MessageBuffer,
    _In_ DWORD MessageBufferSize,
    _In_opt_ LPOVERLAPPED Overlapped)
{
    HRESULT hResult;
    DWORD Error;

    assert(hPort);
    assert(MessageBuffer);

    hResult = FilterGetMessage(hPort,
                               MessageBuffer,
                               MessageBufferSize,
                               Overlapped);
    Error = SCODE_CODE(hResult);
    return Error;
}

/**
 * @name KmtFltReplyMessage
 *
 * Replies to a message from a filter driver
 *
 * @param hPort
 *        Handle to the filter's comms port
 * @ReplyBuffer
 *        Pointer to a buffer to return to the filter
 * @ReplyBufferSize
 *        Size of the buffer pointed to by ReplyBuffer
 *
 * @return Win32 error code
 */
DWORD
KmtFltReplyMessage(
    _In_ HANDLE hPort,
    _In_reads_bytes_(ReplyBufferSize) PFILTER_REPLY_HEADER ReplyBuffer,
    _In_ DWORD ReplyBufferSize)
{
    HRESULT hResult;
    DWORD Error;

    hResult = FilterReplyMessage(hPort,
                                 ReplyBuffer,
                                 ReplyBufferSize);
    Error = SCODE_CODE(hResult);
    return Error;
}

/**
* @name KmtFltGetMessageResult
*
* Gets the overlapped result from the IO
*
* @param hPort
*        Handle to the filter's comms port
* @Overlapped
*        Pointer to the overlapped structure usdd in the IO
* @BytesTransferred
*        Number of bytes transferred in the IO
*
* @return Win32 error code
*/
DWORD
KmtFltGetMessageResult(
    _In_ HANDLE hPort,
    _In_ LPOVERLAPPED Overlapped,
    _Out_ LPDWORD BytesTransferred)
{
    BOOL Success;
    DWORD Error = ERROR_SUCCESS;

    *BytesTransferred = 0;

    Success = GetOverlappedResult(hPort, Overlapped, BytesTransferred, TRUE);
    if (!Success)
    {
        Error = GetLastError();
    }

    return Error;
}

/**
 * @name KmtFltUnload
 *
 * Unload the specified filter driver
 *
 * @param ServiceName
 *        The name of the filter to unload
 *
 * @return Win32 error code
 */
DWORD
KmtFltUnload(
    _In_z_ PCWSTR ServiceName)
{
    HRESULT hResult;
    DWORD Error = ERROR_SUCCESS;

    assert(ServiceName);

    hResult = FilterUnload(ServiceName);
    Error = SCODE_CODE(hResult);

    return Error;
}
