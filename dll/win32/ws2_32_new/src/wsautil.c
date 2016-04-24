/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS WinSock 2 API
 * FILE:        dll/win32/ws2_32_new/src/wsautil.c
 * PURPOSE:     Winsock Utility Functions
 * PROGRAMMER:  Alex Ionescu (alex@relsoft.net)
 */

/* INCLUDES ******************************************************************/

#include <ws2_32.h>

/* FUNCTIONS *****************************************************************/

HKEY
WSAAPI
WsOpenRegistryRoot(VOID)
{
    HKEY WinsockRootKey;
    INT ErrorCode;
    ULONG CreateDisposition;

    /* Open Registry Key */
    ErrorCode = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                             WINSOCK_ROOT,
                             0,
                             MAXIMUM_ALLOWED,
                             &WinsockRootKey);
    
    /* Check if it wasn't found */
    if (ErrorCode == ERROR_FILE_NOT_FOUND)
    {
        /* Create it */
        RegCreateKeyEx(HKEY_LOCAL_MACHINE,
                       WINSOCK_ROOT,
                       0,
                       NULL,
                       REG_OPTION_NON_VOLATILE,
                       KEY_ALL_ACCESS,
                       NULL,
                       &WinsockRootKey,
                       &CreateDisposition);
    }
    else if (ErrorCode == ERROR_SUCCESS)
    {
        /* It already exists */
        CreateDisposition = REG_OPENED_EXISTING_KEY;
    }

    /* Check for failure */
    if (ErrorCode != ERROR_SUCCESS) return NULL;

    /* Check if we had to create a new key */
    if (CreateDisposition == REG_CREATED_NEW_KEY)
    {
        /* Write the Winsock Version */
        RegSetValueEx(WinsockRootKey,
                      "WinSock_Registry_Version",
                      0,
                      REG_SZ,
                      (BYTE*)"2.2",
                      4);
    }
    else
    {
        /* Read the Winsock Version */
    }

    /* Return the key */
    return WinsockRootKey;
}

BOOL
WSAAPI
WsCheckCatalogState(IN HANDLE Event)
{
    DWORD Return;

    /* Wait for the object */
    Return = WaitForSingleObject(Event, 0);

    /* Check for the value */
    if (Return == WAIT_OBJECT_0) return TRUE;

    /* If it timedout or anything else, return false */
    return FALSE;
}

INT
WSAAPI
WsApiProlog(OUT PWSPROCESS *Process,
            OUT PWSTHREAD *Thread)
{
    INT ErrorCode = WSANOTINITIALISED;

    /* Try to get the current process */
    if ((*Process = WsGetProcess()))
    {
        /* And the current thread */
        ErrorCode = WsThreadGetCurrentThread(*Process, Thread);
    }

    /* Return init status */
    return ErrorCode;
}

INT
WSAAPI
WsSlowProlog(VOID)
{
    PWSPROCESS Process;
    PWSTHREAD Thread;

    /* Call the prolog */
    return WsApiProlog(&Process, &Thread);
}

INT
WSAAPI
WsSlowPrologTid(OUT LPWSATHREADID *ThreadId)
{
    PWSPROCESS Process;
    PWSTHREAD Thread;
    INT ErrorCode;

    /* Call the prolog */
    ErrorCode = WsApiProlog(&Process, &Thread);

    /* Check for success */
    if (ErrorCode == ERROR_SUCCESS)
    {
        /* Return the Thread ID */
        *ThreadId = &Thread->WahThreadId;
    }

    /* Return status */
    return ErrorCode;
}

INT
WSAAPI
WsSetupCatalogProtection(IN HKEY CatalogKey,
                         IN HANDLE CatalogEvent,
                         OUT LPDWORD UniqueId)
{
    INT ErrorCode;
    HKEY RegistryKey;
    DWORD NewUniqueId;
    CHAR KeyBuffer[32];
    DWORD RegType = REG_DWORD;
    DWORD RegSize = sizeof(DWORD);

    /* Start loop */
    do
    {
#if 0
        /* Ask for notifications */
        ErrorCode = RegNotifyChangeKeyValue(CatalogKey,
                                            FALSE,
                                            REG_NOTIFY_CHANGE_NAME,
                                            CatalogEvent,
                                            TRUE);
        if (ErrorCode != ERROR_SUCCESS)
        {
            /* Normalize error code */
            ErrorCode = WSASYSCALLFAILURE;
            break;
        }
#endif

        /* Read the current ID */
        ErrorCode = RegQueryValueEx(CatalogKey,
                                    "Serial_Access_Num",
                                    0,
                                    &RegType,
                                    (LPBYTE)&NewUniqueId,
                                    &RegSize);
        if (ErrorCode != ERROR_SUCCESS)
        {
            /* Critical failure */
            ErrorCode = WSASYSCALLFAILURE;
            break;
        }

        /* Try to open it for writing */
        sprintf(KeyBuffer, "%8.8lX", NewUniqueId);
        ErrorCode = RegOpenKeyEx(CatalogKey,
                                 KeyBuffer,
                                 0,
                                 MAXIMUM_ALLOWED,
                                 &RegistryKey);

        /* If the key doesn't exist or is being delete, that's ok for us */
        if ((ErrorCode == ERROR_FILE_NOT_FOUND) ||
            (ErrorCode == ERROR_KEY_DELETED))
        {
            /* Set success and return the new ID */
            ErrorCode = ERROR_SUCCESS;
            *UniqueId = NewUniqueId;
            break;
        }
        else if (ErrorCode != ERROR_SUCCESS)
        {
            /* Any other failure is bad */
            ErrorCode = WSASYSCALLFAILURE;
            break;
        }

        /* If we could actually open the key, someone is using it :/ */
        ErrorCode = RegCloseKey(RegistryKey);

        /* In case we break out prematurely */
        ErrorCode = WSANO_RECOVERY;

        /* Keep looping until they let go of the registry writing */
    } while (!WaitForSingleObject(CatalogEvent, 180 * 1000));

    /* Return error code */
    return ErrorCode;
}

INT
WSAAPI
MapUnicodeProtocolInfoToAnsi(IN LPWSAPROTOCOL_INFOW UnicodeInfo,
                             OUT LPWSAPROTOCOL_INFOA AnsiInfo)
{
    INT ReturnValue;

    /* Copy all the data that doesn't need converting */
    RtlCopyMemory(AnsiInfo,
                  UnicodeInfo,
                  FIELD_OFFSET(WSAPROTOCOL_INFOA, szProtocol));

    /* Now convert the protocol string */
    ReturnValue = WideCharToMultiByte(CP_ACP,
                                      0,
                                      UnicodeInfo->szProtocol,
                                      -1,
                                      AnsiInfo->szProtocol,
                                      sizeof(AnsiInfo->szProtocol),
                                      NULL,
                                      NULL);
    if (!ReturnValue) return WSASYSCALLFAILURE;

    /* Return success */
    return ERROR_SUCCESS;
}

/*
 * @implemented
 */
VOID
WSAAPI
WEP(VOID)
{
    return;
}
