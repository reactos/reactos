/*
 * PROJECT:         ReactOS kernel-mode tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         File system mini-filter support routines
 * PROGRAMMER:      Ged Murphy <gedmurphy@reactos.org>
 */

#include <kmt_test.h>

#define KMT_FLT_USER_MODE
#include "kmtest.h"
#include <kmt_public.h>

#include <ndk/setypes.h>
#include <assert.h>
#include <debug.h>

/*
 * We need to call the internal function in the service.c file
 */
DWORD
KmtpCreateService(
    IN PCWSTR ServiceName,
    IN PCWSTR ServicePath,
    IN PCWSTR DisplayName OPTIONAL,
    IN DWORD ServiceType,
    OUT SC_HANDLE *ServiceHandle);

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
 * @name KmtFltCreateService
 *
 * Create the specified driver service and return a handle to it
 *
 * @param ServiceName
 *        Name of the service to create
 * @param ServicePath
 *        File name of the driver, relative to the current directory
 * @param DisplayName
 *        Service display name
 * @param ServiceHandle
 *        Pointer to a variable to receive the handle to the service
 *
 * @return Win32 error code
 */
DWORD
KmtFltCreateService(
    _In_z_ PCWSTR ServiceName,
    _In_z_ PCWSTR DisplayName,
    _Out_ SC_HANDLE *ServiceHandle)
{
    WCHAR ServicePath[MAX_PATH];

    StringCbCopyW(ServicePath, sizeof(ServicePath), ServiceName);
    StringCbCatW(ServicePath, sizeof(ServicePath), L"_drv.sys");

    StringCbCopyW(TestServiceName, sizeof(TestServiceName), L"Kmtest-");
    StringCbCatW(TestServiceName, sizeof(TestServiceName), ServiceName);

    return KmtpCreateService(TestServiceName,
                             ServicePath,
                             DisplayName,
                             SERVICE_FILE_SYSTEM_DRIVER,
                             ServiceHandle);
}

/**
 * @name KmtFltDeleteService
 *
 * Delete the specified filter driver
 *
 * @param ServiceName
 *        If *ServiceHandle is NULL, name of the service to delete
 * @param ServiceHandle
 *        Pointer to a variable containing the service handle.
 *        Will be set to NULL on success
 *
 * @return Win32 error code
 */
DWORD
KmtFltDeleteService(
    _In_opt_z_ PCWSTR ServiceName,
    _Inout_ SC_HANDLE *ServiceHandle)
{
    return KmtDeleteService(ServiceName, ServiceHandle);
}

/**
 * @name KmtFltLoadDriver
 *
 * Delete the specified filter driver
 *
 * @return Win32 error code
 */
DWORD
KmtFltLoadDriver(
    _In_ BOOLEAN EnableDriverLoadPrivilege,
    _In_ BOOLEAN RestartIfRunning,
    _In_ BOOLEAN ConnectComms,
    _Out_ HANDLE *hPort
)
{
    DWORD Error;

    if (EnableDriverLoadPrivilege)
    {
        BOOLEAN WasEnabled;
        Error = RtlNtStatusToDosError(RtlAdjustPrivilege(
                    SE_LOAD_DRIVER_PRIVILEGE,
                    TRUE,
                    FALSE, // Enable in current process.
                    &WasEnabled));
        if (Error)
            return Error;
    }

    Error = KmtFltLoad(TestServiceName);
    if ((Error == ERROR_SERVICE_ALREADY_RUNNING) && RestartIfRunning)
    {
        Error = KmtFltUnload(TestServiceName);
        if (Error)
        {
            // TODO
            __debugbreak();
        }

        Error = KmtFltLoad(TestServiceName);
    }

    if (Error)
        return Error;

    if (ConnectComms)
        Error = KmtFltConnectComms(hPort);

    return Error;
}

/**
 * @name KmtFltUnloadDriver
 *
 * Unload the specified filter driver
 *
 * @param hPort
 *        Handle to the filter's comms port
 * @param DisonnectComms
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
 * @name KmtFltConnectComms
 *
 * Create a comms connection to the specified filter
 *
 * @param hPort
 *        Handle to the filter's comms port
 *
 * @return Win32 error code
 */
DWORD
KmtFltConnectComms(
    _Out_ HANDLE *hPort)
{
    return KmtFltConnect(TestServiceName, hPort);
}

/**
 * @name KmtFltDisconnectComms
 *
 * Disconenct from the comms port
 *
 * @param hPort
 *        Handle to the filter's comms port
 *
 * @return Win32 error code
 */
DWORD
KmtFltDisconnectComms(
    _In_ HANDLE hPort)
{
    return KmtFltDisconnect(hPort);
}


/**
* @name KmtFltCloseService
*
* Close the specified driver service handle
*
* @param ServiceHandle
*        Pointer to a variable containing the service handle.
*        Will be set to NULL on success
*
* @return Win32 error code
*/
DWORD KmtFltCloseService(
    _Inout_ SC_HANDLE *ServiceHandle)
{
    return KmtCloseService(ServiceHandle);
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
* @return Win32 error code
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
 * @return Win32 error code
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
 * @return Win32 error code
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
 * @return Win32 error code
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
 * @return Win32 error code
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

/**
* @name KmtFltAddAltitude
*
* Sets up the mini-filter altitude data in the registry
*
* @param hPort
*        The altitude string to set
*
* @return Win32 error code
*/
DWORD
KmtFltAddAltitude(
    _In_z_ LPWSTR Altitude)
{
    WCHAR DefaultInstance[128];
    WCHAR KeyPath[256];
    HKEY hKey = NULL;
    HKEY hSubKey = NULL;
    DWORD Zero = 0;
    LONG Error;

    StringCbCopyW(KeyPath, sizeof(KeyPath), L"SYSTEM\\CurrentControlSet\\Services\\");
    StringCbCatW(KeyPath, sizeof(KeyPath), TestServiceName);
    StringCbCatW(KeyPath, sizeof(KeyPath), L"\\Instances\\");

    Error = RegCreateKeyEx(HKEY_LOCAL_MACHINE,
                           KeyPath,
                           0,
                           NULL,
                           REG_OPTION_NON_VOLATILE,
                           KEY_CREATE_SUB_KEY | KEY_SET_VALUE,
                           NULL,
                           &hKey,
                           NULL);
    if (Error != ERROR_SUCCESS)
    {
        return Error;
    }

    StringCbCopyW(DefaultInstance, sizeof(DefaultInstance), TestServiceName);
    StringCbCatW(DefaultInstance, sizeof(DefaultInstance), L" Instance");

    Error = RegSetValueExW(hKey,
                           L"DefaultInstance",
                           0,
                           REG_SZ,
                           (LPBYTE)DefaultInstance,
                           (lstrlenW(DefaultInstance) + 1) * sizeof(WCHAR));
    if (Error != ERROR_SUCCESS)
    {
        goto Quit;
    }

    Error = RegCreateKeyW(hKey, DefaultInstance, &hSubKey);
    if (Error != ERROR_SUCCESS)
    {
        goto Quit;
    }

    Error = RegSetValueExW(hSubKey,
                           L"Altitude",
                           0,
                           REG_SZ,
                           (LPBYTE)Altitude,
                           (lstrlenW(Altitude) + 1) * sizeof(WCHAR));
    if (Error != ERROR_SUCCESS)
    {
        goto Quit;
    }

    Error = RegSetValueExW(hSubKey,
                           L"Flags",
                           0,
                           REG_DWORD,
                           (LPBYTE)&Zero,
                           sizeof(DWORD));

Quit:
    if (hSubKey)
    {
        RegCloseKey(hSubKey);
    }
    if (hKey)
    {
        RegCloseKey(hKey);
    }

    return Error;

}
