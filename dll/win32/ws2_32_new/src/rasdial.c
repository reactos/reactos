/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS WinSock 2 API
 * FILE:        dll/win32/ws2_32_new/src/rasdial.c
 * PURPOSE:     RAS Auto-Dial Support
 * PROGRAMMER:  Alex Ionescu (alex@relsoft.net)
 */

/* INCLUDES ******************************************************************/

#include <ws2_32.h>

#define NDEBUG
#include <debug.h>

/* DATA **********************************************************************/

typedef BOOL
(WSAAPI *PWS_ATTEMPT_AUTODIAL_ADDR)(
    IN CONST SOCKADDR FAR *Name,
    IN INT NameLength
);

typedef BOOL
(WSAAPI *PWS_ATTEMPT_AUTODIAL_NAME)(IN CONST LPWSAQUERYSETW lpqsRestrictions);

typedef VOID
(WSAAPI *PWS_NOTE_SUCCESSFUL_HOSTENT_LOOKUP)(
    IN CONST CHAR FAR *Name,
    IN CONST ULONG Address
);

BOOLEAN WsRasInitialized;
HINSTANCE WsRasDllHandle;
CRITICAL_SECTION WsRasHelperLock;
PWS_ATTEMPT_AUTODIAL_ADDR lpfnWSAttemptAutodialAddr;
PWS_ATTEMPT_AUTODIAL_NAME lpfnWSAttemptAutodialName;
PWS_NOTE_SUCCESSFUL_HOSTENT_LOOKUP lpfnWSNoteSuccessfulHostentLookup;

#define WsRasLock()          EnterCriticalSection(&WsRasHelperLock);
#define WsRasUnlock()        LeaveCriticalSection(&WsRasHelperLock);

/* FUNCTIONS *****************************************************************/

VOID
WSAAPI
WsRasInitializeAutodial(VOID)
{
    /* Initialize the autodial lock */
    InitializeCriticalSection(&WsRasHelperLock);
}

VOID
WSAAPI
WsRasUninitializeAutodial(VOID)
{
    /* Acquire lock */
    WsRasLock();

    /* Free the library if it's loaded */
    if (WsRasDllHandle) FreeLibrary(WsRasDllHandle);
    WsRasDllHandle = NULL;

    /* Release and delete lock */
    WsRasUnlock();
    DeleteCriticalSection(&WsRasHelperLock);
}

INT
WSAAPI
WsRasLoadHelperDll(VOID)
{
    CHAR HelperPath[MAX_PATH];
    HKEY WinsockKey;
    INT ErrorCode;
    DWORD RegType = REG_SZ;
    DWORD RegSize = MAX_PATH;

    /* Acquire the lock */
    WsRasLock();

    /* Check if we were already initialiazed */
    if (!WsRasInitialized)
    {
        /* Open the registry root key */
        WinsockKey = WsOpenRegistryRoot();
        if (WinsockKey)
        {
            /* Read the helper's location */
            ErrorCode = RegQueryValueEx(WinsockKey,
                                        "AutodialDLL",
                                        0,
                                        &RegType,
                                        (LPBYTE)&HelperPath,
                                        &RegSize);
            RegCloseKey(WinsockKey);

            /* Make sure we read the path */
            if (ErrorCode == ERROR_SUCCESS)
            {
                /* Now load it */
                WsRasDllHandle = LoadLibrary(HelperPath);
            }
        }

        /* Check if we weren't able to load it and load the default */
        if (!WsRasDllHandle) WsRasDllHandle = LoadLibrary("rasadhlp.dll");

        /* Check again if we loaded it */
        if (WsRasDllHandle)
        {
            /* Get function pointers */
            lpfnWSAttemptAutodialAddr = 
                (PVOID)GetProcAddress(WsRasDllHandle,
                                      "WSAttemptAutodialAddr");
            lpfnWSAttemptAutodialName = 
                (PVOID)GetProcAddress(WsRasDllHandle,
                                      "WSAttemptAutodialName");
            lpfnWSNoteSuccessfulHostentLookup = 
                (PVOID)GetProcAddress(WsRasDllHandle,
                                      "WSNoteSuccessfulHostentLookup");
        }

        /* Mark us as loaded */
        WsRasInitialized = TRUE;
    }

    /* Release lock */
    WsRasUnlock();

    /* Return status */
    return WsRasInitialized;
}

BOOL
WSAAPI
WSAttemptAutodialAddr(IN CONST SOCKADDR FAR *Name,
                      IN INT NameLength)
{
    /* Load the helper DLL and make sure it exports this routine */
    if (!(WsRasLoadHelperDll()) || !(lpfnWSAttemptAutodialAddr)) return FALSE;

    /* Call the function in the helper */
    return lpfnWSAttemptAutodialAddr(Name, NameLength);
}

BOOL
WSAAPI
WSAttemptAutodialName(IN CONST LPWSAQUERYSETW lpqsRestrictions)
{
    /* Load the helper DLL and make sure it exports this routine */
    if (!(WsRasLoadHelperDll()) || !(lpfnWSAttemptAutodialName)) return FALSE;

    /* Call the function in the helper */
    return lpfnWSAttemptAutodialName(lpqsRestrictions);
}

VOID
WSAAPI
WSNoteSuccessfulHostentLookup(IN CONST CHAR FAR *Name,
                              IN CONST ULONG Address)
{
    /* Load the helper DLL and make sure it exports this routine */
    if (!(WsRasLoadHelperDll()) || !(lpfnWSNoteSuccessfulHostentLookup)) return;

    /* Call the function in the helper */
    lpfnWSNoteSuccessfulHostentLookup(Name, Address);
}
