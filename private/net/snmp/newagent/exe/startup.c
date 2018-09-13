/*++

Copyright (c) 1992-1997  Microsoft Corporation

Module Name:

    startup.c

Abstract:

    Contains routines for starting SNMP master agent.

Environment:

    User Mode - Win32

Revision History:

    10-Feb-1997 DonRyan
        Rewrote to implement SNMPv2 support.

--*/

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Include files                                                             //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include "globals.h"
#include "startup.h"
#include "network.h"
#include "registry.h"
#include "snmpthrd.h"
#include "regthrd.h"
#include "trapthrd.h"
#include "args.h"
#include "mem.h"
#include "snmpmgmt.h"


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Global variables                                                          //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

HANDLE g_hAgentThread = NULL;
HANDLE g_hRegistryThread = NULL; // Used to track registry changes
CRITICAL_SECTION g_RegCriticalSectionA;
CRITICAL_SECTION g_RegCriticalSectionB;


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Private procedures                                                        //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

BOOL
LoadWinsock(
    )

/*++

Routine Description:

    Startup winsock.

Arguments:

    None.

Return Values:

    Returns true if successful.

--*/

{
    WSADATA WsaData;
    WORD wVersionRequested = MAKEWORD(2,0);
    INT nStatus;
    
    // attempt to startup winsock    
    nStatus = WSAStartup(wVersionRequested, &WsaData);

    // validate return code
    if (nStatus == SOCKET_ERROR) {
        
        SNMPDBG((
            SNMP_LOG_ERROR,
            "SNMP: SVC: error %d starting winsock.\n",
            WSAGetLastError()
            ));

        // failure
        return FALSE;
    }

    // success
    return TRUE;
}

BOOL
UnloadWinsock(
    )

/*++

Routine Description:

    Shutdown winsock.

Arguments:

    None.

Return Values:

    Returns true if successful.

--*/

{
    INT nStatus;

    // cleanup
    nStatus = WSACleanup();

    // validate return code
    if (nStatus == SOCKET_ERROR) {
            
        SNMPDBG((
            SNMP_LOG_ERROR,
            "SNMP: SVC: error %d stopping winsock.\n",
            WSAGetLastError()
            ));

        // failure
        return FALSE;
    }

    // success
    return TRUE;
}


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Public procedures                                                         //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

BOOL
StartupAgent(
    )

/*++

Routine Description:

    Performs essential initialization of master agent.

Arguments:

    None.

Return Values:

    Returns true if successful.

--*/

{
    BOOL fOk = TRUE;
    DWORD dwThreadId = 0;
	DWORD regThreadId = 0;
    INT nCSOk = 0;          // counts the number of CS that were successfully initialized

	// initialize management variables
    mgmtInit();

    // initialize list heads
    InitializeListHead(&g_Subagents);
    InitializeListHead(&g_SupportedRegions);
    InitializeListHead(&g_ValidCommunities);
    InitializeListHead(&g_TrapDestinations);
    InitializeListHead(&g_PermittedManagers);
    InitializeListHead(&g_IncomingTransports);
    InitializeListHead(&g_OutgoingTransports);

    __try
    {
        InitializeCriticalSection(&g_RegCriticalSectionA); nCSOk++;
        InitializeCriticalSection(&g_RegCriticalSectionB); nCSOk++;
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        if (nCSOk == 1)
            DeleteCriticalSection(&g_RegCriticalSectionA);
        // nCSOk can't be 2 as far as we are here

        fOk = FALSE;
    }
    SNMPDBG((
        SNMP_LOG_TRACE,
        "SNMP: SVC: Initialize critical sections...%s\n", fOk? "Ok" : "Failed"));


	fOk = fOk &&
          (g_hRegistryEvent = CreateEvent(NULL, FALSE, TRUE, NULL)) != NULL;
      
    g_dwUpTimeReference = SnmpSvcInitUptime();
    // retreive system uptime reference
    SNMPDBG((
        SNMP_LOG_TRACE,
        "SNMP: SVC: Getting system uptime...%d\n", g_dwUpTimeReference));

    // allocate essentials
    fOk = fOk && AgentHeapCreate();
    SNMPDBG((
        SNMP_LOG_TRACE,
        "SNMP: SVC: Creating agent heap...%s\n", fOk? "Ok" : "Failed"));

    fOk = fOk && LoadWinsock();
    SNMPDBG((
        SNMP_LOG_TRACE,
        "SNMP: SVC: Loading Winsock stack...%s\n", fOk? "Ok" : "Failed"));

    fOk = fOk && LoadIncomingTransports();
    SNMPDBG((
        SNMP_LOG_TRACE,
        "SNMP: SVC: Loading Incoming transports...%s\n", fOk? "Ok" : "Failed"));

    fOk = fOk && LoadOutgoingTransports();
    SNMPDBG((
        SNMP_LOG_TRACE,
        "SNMP: SVC: Loading Outgoing transports...%s\n", fOk? "Ok" : "Failed"));

    fOk = fOk &&
            // attempt to start main thread
          (g_hAgentThread = CreateThread(
                               NULL,               // lpThreadAttributes
                               0,                  // dwStackSize
                               ProcessSnmpMessages,
                               NULL,               // lpParameter
                               CREATE_SUSPENDED,   // dwCreationFlags
                               &dwThreadId
                               )) != NULL;
    SNMPDBG((
        SNMP_LOG_TRACE,
        "SNMP: SVC: Starting ProcessSnmpMessages thread...%s\n", fOk? "Ok" : "Failed"));

    fOk = fOk &&
           // attempt to start registry listener thread
          (g_hRegistryThread = CreateThread(
                               NULL,
                               0,
                               ProcessRegistryMessage,
                               NULL,
                               CREATE_SUSPENDED,
                               &regThreadId)) != NULL;
    SNMPDBG((
        SNMP_LOG_TRACE,
        "SNMP: SVC: Starting ProcessRegistryMessages thread...%s\n", fOk? "Ok" : "Failed"));

    return fOk;        
}


BOOL
ShutdownAgent(
    )

/*++

Routine Description:

    Performs final cleanup of master agent.

Arguments:

    None.

Return Values:

    Returns true if successful.

--*/

{
    BOOL fOk;
    DWORD dwStatus;

    // make sure shutdown signalled
    fOk = SetEvent(g_hTerminationEvent);

    if (!fOk) {
                    
        SNMPDBG((
            SNMP_LOG_ERROR,
            "SNMP: SVC: error %d signalling termination.\n",
            GetLastError()
            ));
    }

    // check if thread created
    if ((g_hAgentThread != NULL) && (g_hRegistryThread != NULL)) {
        HANDLE hEvntArray[2];

        hEvntArray[0] = g_hAgentThread;
        hEvntArray[1] = g_hRegistryThread;

        dwStatus = WaitForMultipleObjects(2, hEvntArray, TRUE, SHUTDOWN_WAIT_HINT);

        // validate return status
        if ((dwStatus != WAIT_OBJECT_0) &&
            (dwStatus != WAIT_OBJECT_0 + 1) &&
            (dwStatus != WAIT_TIMEOUT)) {
            
            SNMPDBG((
                SNMP_LOG_ERROR,
                "SNMP: SVC: error %d waiting for thread(s) termination.\n",
                GetLastError()
                ));
        }
    } else if (g_hAgentThread != NULL) {

        // wait for pdu processing thread to terminate
        dwStatus = WaitForSingleObject(g_hAgentThread, SHUTDOWN_WAIT_HINT);

        // validate return status
        if ((dwStatus != WAIT_OBJECT_0) &&
            (dwStatus != WAIT_TIMEOUT)) {
            
            SNMPDBG((
                SNMP_LOG_ERROR,
                "SNMP: SVC: error %d waiting for main thread termination.\n",
                GetLastError()
                ));
        }
    } else if (g_hRegistryThread != NULL) {

        // wait for registry processing thread to terminate
        dwStatus = WaitForSingleObject(g_hRegistryThread, SHUTDOWN_WAIT_HINT);

        // validate return status
        if ((dwStatus != WAIT_OBJECT_0) &&
            (dwStatus != WAIT_TIMEOUT)) {
            
            SNMPDBG((
                SNMP_LOG_ERROR,
                "SNMP: SVC: error %d waiting for registry thread termination.\n",
                GetLastError()
                ));
        }
    }

    // unload incoming transports
    UnloadIncomingTransports();

    // unload outgoing transports
    UnloadOutgoingTransports();

    // unload registry info
    UnloadRegistryParameters();

    // unload the winsock stack
    UnloadWinsock();

    // cleanup the internal management buffers
    mgmtCleanup();

    // nuke private heap
    AgentHeapDestroy();

    ReportSnmpEvent(
        SNMP_EVENT_SERVICE_STOPPED,
        0,
        NULL,
        0);

    return TRUE;
}
