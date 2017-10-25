/*
 * PROJECT:         ReactOS kernel-mode tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         File system mini-filter support routines
 * PROGRAMMER:      Ged Murphy <gedmurphy@reactos.org>
 */

#include <kmt_test.h>

#include "kmtest.h"
#include <kmt_public.h>

#include <assert.h>
#include <debug.h>

// move to a shared location
typedef struct _KMTFLT_MESSAGE_HEADER
{
    ULONG Message;
    PVOID Buffer;
    ULONG BufferSize;

} KMTFLT_MESSAGE_HEADER, *PKMTFLT_MESSAGE_HEADER;

extern HANDLE KmtestHandle;
static WCHAR TestServiceName[MAX_PATH];


/**
 * @name KmtFltLoadDriver
 *
 * Load the specified filter driver
 * This routine will create the service entry if it doesn't already exist
 *
 * @param ServiceName
 *        Name of the driver service (Kmtest- prefix will be added automatically)
 * @param RestartIfRunning
 *        TRUE to stop and restart the service if it is already running
 * @param ConnectComms
 *        TRUE to create a comms connection to the specified filter
 * @param hPort
 *        Handle to the filter's comms port
 *
 * @return Win32 error code
 */
DWORD
KmtFltLoadDriver(
    _In_z_ PCWSTR ServiceName,
    _In_ BOOLEAN RestartIfRunning,
    _In_ BOOLEAN ConnectComms,
    _Out_ HANDLE *hPort)
{
    DWORD Error = ERROR_SUCCESS;
    WCHAR ServicePath[MAX_PATH];
    SC_HANDLE TestServiceHandle;

    StringCbCopy(ServicePath, sizeof ServicePath, ServiceName);
    StringCbCat(ServicePath, sizeof ServicePath, L"_drv.sys");

    StringCbCopy(TestServiceName, sizeof TestServiceName, L"Kmtest-");
    StringCbCat(TestServiceName, sizeof TestServiceName, ServiceName);

    Error = KmtFltCreateAndStartService(TestServiceName, ServicePath, NULL, &TestServiceHandle, TRUE);

    if (Error == ERROR_SUCCESS && ConnectComms)
    {
        Error = KmtFltConnect(ServiceName, hPort);
    }

    return Error;
}

/**
 * @name KmtFltUnloadDriver
 *
 * Unload the specified filter driver
 *
 * @param hPort
 *        Handle to the filter's comms port
 * @param ConnectComms
 *        TRUE to disconnect the comms connection before unloading
 *
 * @return Win32 error code
 */
DWORD
KmtFltUnloadDriver(
    _In_ HANDLE *hPort,
    _In_ BOOLEAN DisonnectComms)
{
    DWORD Error = ERROR_SUCCESS;

    if (DisonnectComms)
    {
        Error = KmtFltDisconnect(hPort);

        if (Error)
        {
            return Error;
        }
    }

    Error = KmtFltUnload(TestServiceName);

    if (Error)
    {
        // TODO
        __debugbreak();
    }

    return Error;
}


/**
* @name KmtFltRunKernelTest
*
* Run the specified filter test part
*
* @param hPort
*        Handle to the filter's comms port
* @param TestName
*        Name of the test to run
*
* @return Win32 error code
*/
DWORD
KmtFltRunKernelTest(
    _In_ HANDLE hPort,
    _In_z_ PCSTR TestName)
{
    return KmtFltSendStringToDriver(hPort, KMTFLT_RUN_TEST, TestName);
}

/**
* @name KmtFltSendToDriver
*
* Send an I/O control message with no arguments to the driver opened with KmtOpenDriver
*
* @param hPort
*        Handle to the filter's comms port
* @param Message
*        The message to send to the filter
*
* @return Win32 error code as returned by DeviceIoControl
*/
DWORD
KmtFltSendToDriver(
    _In_ HANDLE hPort,
    _In_ DWORD Message)
{
    assert(hPort);
    return KmtFltSendBufferToDriver(hPort, Message, NULL, 0, NULL, 0, NULL);
}

/**
 * @name KmtFltSendStringToDriver
 *
 * Send an I/O control message with a string argument to the driver opened with KmtOpenDriver
 *
 *
 * @param hPort
 *        Handle to the filter's comms port
 * @param Message
 *        The message associated with the string
 * @param String
 *        An ANSI string to send to the filter
 *
 * @return Win32 error code as returned by DeviceIoControl
 */
DWORD
KmtFltSendStringToDriver(
    _In_ HANDLE hPort,
    _In_ DWORD Message,
    _In_ PCSTR String)
{
    assert(hPort);
    assert(String);
    return KmtFltSendBufferToDriver(hPort, Message, (PVOID)String, (DWORD)strlen(String), NULL, 0, NULL);
}

/**
 * @name KmtFltSendWStringToDriver
 *
 * Send an I/O control message with a wide string argument to the driver opened with KmtOpenDriver
 *
 * @param hPort
 *        Handle to the filter's comms port
 * @param Message
 *        The message associated with the string
 * @param String
 *        An wide string to send to the filter
 *
 * @return Win32 error code as returned by DeviceIoControl
 */
DWORD
KmtFltSendWStringToDriver(
    _In_ HANDLE hPort,
    _In_ DWORD Message,
    _In_ PCWSTR String)
{
    return KmtFltSendBufferToDriver(hPort, Message, (PVOID)String, (DWORD)wcslen(String) * sizeof(WCHAR), NULL, 0, NULL);
}

/**
 * @name KmtFltSendUlongToDriver
 *
 * Send an I/O control message with an integer argument to the driver opened with KmtOpenDriver
 *
 * @param hPort
 *        Handle to the filter's comms port
 * @param Message
 *        The message associated with the value
 * @param Value
 *        An 32bit valueng to send to the filter
 *
 * @return Win32 error code as returned by DeviceIoControl
 */
DWORD
KmtFltSendUlongToDriver(
    _In_ HANDLE hPort,
    _In_ DWORD Message,
    _In_ DWORD Value)
{
    return KmtFltSendBufferToDriver(hPort, Message, &Value, sizeof(Value), NULL, 0, NULL);
}

/**
 * @name KmtSendBufferToDriver
 *
 * Send an I/O control message with the specified arguments to the driver opened with KmtOpenDriver
 *
 * @param hPort
 *        Handle to the filter's comms port
 * @param Message
 *        The message associated with the value
 * @param InBuffer
 *        Pointer to a buffer to send to the filter
 * @param BufferSize
 *        Size of the buffer pointed to by InBuffer
 * @param OutBuffer
 *        Pointer to a buffer to receive a response from the filter
 * @param OutBufferSize
 *        Size of the buffer pointed to by OutBuffer
 * @param BytesReturned
 *        Number of bytes written in the reply buffer
 *
 * @return Win32 error code as returned by DeviceIoControl
 */
DWORD
KmtFltSendBufferToDriver(
    _In_ HANDLE hPort,
    _In_ DWORD Message,
    _In_reads_bytes_(BufferSize) LPVOID InBuffer,
    _In_ DWORD BufferSize,
    _Out_writes_bytes_to_opt_(OutBufferSize, *BytesReturned) LPVOID OutBuffer,
    _In_ DWORD OutBufferSize,
    _Out_opt_ LPDWORD BytesReturned)
{
    PKMTFLT_MESSAGE_HEADER Ptr;
    KMTFLT_MESSAGE_HEADER Header;
    BOOLEAN FreeMemory = FALSE;
    DWORD InBufferSize;
    DWORD Error;

    assert(hPort);

    if (BufferSize)
    {
        assert(InBuffer);

        InBufferSize = sizeof(KMTFLT_MESSAGE_HEADER) + BufferSize;
        Ptr = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, InBufferSize);
        if (!Ptr)
        {
            return ERROR_NOT_ENOUGH_MEMORY;
        }
        FreeMemory = TRUE;
    }
    else
    {
        InBufferSize = sizeof(KMTFLT_MESSAGE_HEADER);
        Ptr = &Header;
    }

    Ptr->Message = Message;
    if (BufferSize)
    {
        Ptr->Buffer = (Ptr + 1);
        StringCbCopy(Ptr->Buffer, BufferSize, InBuffer);
        Ptr->BufferSize = BufferSize;
    }

    Error = KmtFltSendMessage(hPort, Ptr, InBufferSize, OutBuffer, OutBufferSize, BytesReturned);

    if (FreeMemory)
    {
        HeapFree(GetProcessHeap(), 0, Ptr);
    }

    return Error;
}
