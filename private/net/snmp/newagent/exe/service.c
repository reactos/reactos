/*++

Copyright (c) 1992-1997  Microsoft Corporation

Module Name:

    service.c

Abstract:

    Contains service controller code for SNMP service.

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
#include "service.h"
#include "startup.h"
#include "trapthrd.h"
#include "registry.h"


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Global variables                                                          //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

SERVICE_STATUS_HANDLE g_SnmpSvcHandle = 0;
SERVICE_STATUS g_SnmpSvcStatus = {
    SERVICE_WIN32,      // dwServiceType                
    SERVICE_STOPPED,    // dwCurrentState    
    0,                  // dwControlsAccepted    
    NO_ERROR,           // dwWin32ExitCode    
    0,                  // dwServiceSpecificExitCode    
    0,                  // dwCheckPoint    
    0                   // dwWaitHint    
    };     


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Private procedures                                                        //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

BOOL
TerminateService(
    )

/*++

Routine Description:

    Shutdown SNMP service.

Arguments:

    None.

Return Values:

    Returns true if successful.

--*/

{
    // signal io thread to terminate
    BOOL fOk = SetEvent(g_hTerminationEvent);
    
    if (!fOk) {
                
        SNMPDBG((
            SNMP_LOG_ERROR, 
            "SNMP: SVC: error 0x%08lx setting termination event.\n",
            GetLastError()
            ));
    }

    return fOk;
}


BOOL
UpdateController(
    DWORD dwCurrentState,
    DWORD dwWaitHint
    )

/*++

Routine Description:

    Notify service controller of SNMP service status.

Arguments:

    dwCurrentState - state of the service.

    dwWaitHint - worst case estimate to next checkpoint.

Return Values:

    Returns true if successful.

--*/

{
    BOOL fOk = FALSE;

    // validate handle
    if (g_SnmpSvcHandle != 0) {

        static DWORD dwCheckPoint = 1;

        // check to see if the service is starting    
        if (dwCurrentState == SERVICE_START_PENDING) {

            // do not accept controls during startup
            g_SnmpSvcStatus.dwControlsAccepted = 0;

        } else {

            // only accept stop command during operation
            g_SnmpSvcStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP;
        }

        // if checkpoint needs incrementing
        if ((dwCurrentState == SERVICE_RUNNING) ||
            (dwCurrentState == SERVICE_STOPPED)) {

            // re-initialize checkpint    
            g_SnmpSvcStatus.dwCheckPoint = 0;
    
        } else {
            
            // increment checkpoint to denote processing
            g_SnmpSvcStatus.dwCheckPoint = dwCheckPoint++;
        }

        // update global status structure
        g_SnmpSvcStatus.dwCurrentState = dwCurrentState;
        g_SnmpSvcStatus.dwWaitHint     = dwWaitHint;
            
        SNMPDBG((
            SNMP_LOG_TRACE, 
            "SNMP: SVC: setting service status to %s (0x%08lx).\n",
            SERVICE_STATUS_STRING(g_SnmpSvcStatus.dwCurrentState),
            g_SnmpSvcStatus.dwCheckPoint
            ));
    
        // register current state with service controller
        fOk = SetServiceStatus(g_SnmpSvcHandle, &g_SnmpSvcStatus);

        if (!fOk) {
            
            SNMPDBG((
                SNMP_LOG_WARNING, 
                "SNMP: SVC: error 0x%08lx setting service status.\n",
                GetLastError()
                ));
        }
    }

    return fOk;
}


VOID
ProcessControllerRequests(
    DWORD dwOpCode
    )

/*++

Routine Description:

    Control handling function of SNMP service.

Arguments:

    dwOpCode - requested control code.

Return Values:

    None.

--*/

{
    DWORD dwCurrentState = SERVICE_RUNNING;
    DWORD dwWaitHint     = 0;
        
    SNMPDBG((
        SNMP_LOG_VERBOSE, 
        "SNMP: SVC: processing request to %s service.\n",
        SERVICE_CONTROL_STRING(dwOpCode)
        ));

    // handle command
    switch (dwOpCode) {

    case SERVICE_CONTROL_STOP:

        // change service status to stopping
        dwCurrentState = SERVICE_STOP_PENDING;    
        dwWaitHint     = SNMP_WAIT_HINT;

        break; 

    case SERVICE_CONTROL_INTERROGATE:

        //
        // update controller below...
        //

        break;

    default:

        // check for parameters 
        if (IS_LOGLEVEL(dwOpCode)) {

            UINT nLogLevel;

            // derive the new log level from the opcode            
            nLogLevel = dwOpCode - SNMP_SERVICE_LOGLEVEL_BASE;
            
            SNMPDBG((
                SNMP_LOG_TRACE, 
                "SNMP: SVC: changing log level to %s.\n",
                SNMP_LOGLEVEL_STRING(nLogLevel)
                ));

            // store the new log level                     
            SnmpSvcSetLogLevel(nLogLevel);

        } else if (IS_LOGTYPE(dwOpCode)) {

            UINT nLogType;

            // derive the new log type from opcode
            nLogType = dwOpCode - SNMP_SERVICE_LOGTYPE_BASE;
            
            SNMPDBG((
                SNMP_LOG_TRACE, 
                "SNMP: SVC: changing log type to %s.\n",
                SNMP_LOGTYPE_STRING(nLogType)
                ));

            // store the new log type
            SnmpSvcSetLogType(nLogType);

        } else {
                                           
            SNMPDBG((
                SNMP_LOG_WARNING, 
                "SNMP: SVC: unhandled control code %d.\n",
                dwOpCode
                ));
        }

        break;        
    }

    // report status to controller
    UpdateController(dwCurrentState, dwWaitHint);

    // make sure to set shutdown event    
    if (dwCurrentState == SERVICE_STOP_PENDING) {

        // terminate
        TerminateService();
    }
}


BOOL 
WINAPI
ProcessConsoleRequests(
    DWORD dwOpCode
    )

/*++

Routine Description:

    Handle console control events.

Arguments:

    dwOpCode - requested control code.

Return Values:

    Returns true if request processed.

--*/

{
    BOOL fOk = FALSE;

    // check if user wants to exit
    if ((dwOpCode == CTRL_C_EVENT) ||
        (dwOpCode == CTRL_BREAK_EVENT)) {
                
        SNMPDBG((
            SNMP_LOG_TRACE, 
            "SNMP: SVC: processing ctrl-c request.\n"
            ));

        // stop service
        fOk = TerminateService();
    }
    
    return fOk;
} 


VOID
ServiceMain(
    IN DWORD  NumberOfArgs,
    IN LPTSTR  ArgumentPtrs[]
    )

/*++

Routine Description:

    Entry point of SNMP service.

Arguments:

    NumberOfArgs - number of command line arguments.
    ArgumentPtrs - array of pointers to arguments.

Return Values:

    None.

--*/

{
    // check if we need to bypass dispatcher
    if (!g_CmdLineArguments.fBypassCtrlDispatcher) {

        // register snmp with service controller
        g_SnmpSvcHandle = RegisterServiceCtrlHandler(
                                SNMP_SERVICE,
                                ProcessControllerRequests
                                );

        // validate handle
        if (g_SnmpSvcHandle == 0) { 

            // save error code in service status structure
            g_SnmpSvcStatus.dwWin32ExitCode = GetLastError();    
            
            SNMPDBG((
                SNMP_LOG_ERROR, 
                "SNMP: SVC: error 0x%08lx registering service.\n",
                g_SnmpSvcStatus.dwWin32ExitCode
                ));

            return; // bail...
        }
    }
    
    // report status to service controller
    UpdateController(SERVICE_START_PENDING, SNMP_WAIT_HINT);

    // startup agent
    if (StartupAgent()) {

        // report status to service controller
        UpdateController(SERVICE_RUNNING, NO_WAIT_HINT);

        // load registry
        // this is done after notifying the service controller that SNMP is up and running
        // because of the potential delay taken to load each subagent apart.
        // it is done here and not in the thread resumed below, because this call has to complete
        // before ProcessSubagentEvents() (data structures used in ProcessSubagentEvents() are initialized in
        // LoadRegistryParameters())
        // bugs: #259509 & #274055.
        LoadRegistryParameters();

        if (ResumeThread(g_hAgentThread) != 0xFFFFFFFF)
        {
            if (ResumeThread(g_hRegistryThread) == 0xFFFFFFFF) 
            {
                DWORD errCode = GetLastError();

                SNMPDBG((
                    SNMP_LOG_ERROR, 
                    "SNMP: SVC: error 0x%08lx starting the ProcessRegistryMessages thread.\n",
                    errCode
                    ));
                // log an event to system log file - SNMP service is going on but will not update on registry changes
                ReportSnmpEvent(
                    SNMP_EVENT_REGNOTIFY_THREAD_FAILED, 
                    0, 
                    NULL, 
                    errCode
                    );
            }
            // service subagents 
            ProcessSubagentEvents(); 
        }
        else
        {
            SNMPDBG((
                SNMP_LOG_ERROR, 
                "SNMP: SVC: error 0x%08lx starting the ProcessMessages thread.\n",
                GetLastError()
                ));
        }
    }

    // report status to service controller
    UpdateController(SERVICE_STOP_PENDING, SNMP_WAIT_HINT);

    // stop agent
    ShutdownAgent();
    
    // report status to service controller
    UpdateController(SERVICE_STOPPED, NO_WAIT_HINT);
}


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Public procedures                                                         //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

INT 
__cdecl 
main(
    DWORD  NumberOfArgs,
    LPSTR ArgumentPtrs[]
    )

/*++

Routine Description:

    Entry point of program.

Arguments:

    NumberOfArgs - number of command line arguments.
    ArgumentPtrs - array of pointers to arguments.

Return Values:

    None.

--*/

{
    BOOL fOk;
    DWORD dwLastError;

    static SERVICE_TABLE_ENTRY SnmpServiceTable[] =
        {{SNMP_SERVICE, ServiceMain}, {NULL, NULL}};

    // process command line arguments before starting
    if (ProcessArguments(NumberOfArgs, ArgumentPtrs)) {

        // create manual reset termination event for service
        g_hTerminationEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

        // check if we need to bypass dispatcher
        if (g_CmdLineArguments.fBypassCtrlDispatcher) {
            
            SNMPDBG((    
                SNMP_LOG_TRACE,     
                "SNMP: SVC: bypassing service controller...\n"
                ));    
             
        
            // install console command handler
            SetConsoleCtrlHandler(ProcessConsoleRequests, TRUE);

            // dispatch snmp service manually
            ServiceMain(NumberOfArgs, (LPTSTR*)ArgumentPtrs);

        } else {
                            
            SNMPDBG((    
                SNMP_LOG_TRACE,     
                "SNMP: SVC: connecting to service controller...\n"
                ));    
             

            // attempt to connect to service controller
            fOk = StartServiceCtrlDispatcher(SnmpServiceTable);

            if (!fOk) {
        
                // retrieve controller failure
                dwLastError = GetLastError();

                // check to see whether or not the error was unexpected
                if (dwLastError == ERROR_FAILED_SERVICE_CONTROLLER_CONNECT) {
                        
                    SNMPDBG((    
                        SNMP_LOG_TRACE,     
                        "SNMP: SVC: unable to connect so manually starting...\n"
                        ));    
                     

                    // make note that service is not connected
                    g_CmdLineArguments.fBypassCtrlDispatcher = TRUE;
                    
                    // install console command handler
                    SetConsoleCtrlHandler(ProcessConsoleRequests, TRUE);

                    // attempt to dispatch service manually
                    ServiceMain(NumberOfArgs, (LPTSTR*)ArgumentPtrs);

                } else {
                    
                    SNMPDBG((    
                        SNMP_LOG_ERROR,     
                        "SNMP: SVC: error 0x%08lx connecting to controller.\n", 
                        dwLastError
                        ));    
                }
            }
        }

        // close termination event
        CloseHandle(g_hTerminationEvent);
    }
    
    SNMPDBG((    
        SNMP_LOG_TRACE,     
        "SNMP: SVC: service exiting 0x%08lx.\n",    
        g_SnmpSvcStatus.dwWin32ExitCode
        ));    

    // return service status code
    return g_SnmpSvcStatus.dwWin32ExitCode;
}
