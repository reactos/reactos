/*++
Copyright (c) 1992-1997  Microsoft Corporation

Module Name:
    args.c

Abstract:
    Contains routines for processing command line arguments.

Environment:
    User Mode - Win32

Revision History:

    10-Feb-1997 DonRyan
        Rewrote to implement SNMPv2 support.
--*/


// Include files                                                             //
#include "globals.h"
#include "args.h"
#include "stdlib.h"


// Public procedures                                                         //


BOOL ProcessArguments(DWORD  NumberOfArgs, LPSTR ArgumentPtrs[])
/*++
Routine Description:
    Processes command line arguments.
Arguments:
    NumberOfArgs - number of command line arguments.
    ArgumentPtrs - array of pointers to arguments.
Return Values:
    Returns true if successful.
--*/
{
    DWORD  dwArgument;
    LPSTR pCurrentArg;

    // initialize logging arguments
    g_CmdLineArguments.nLogLevel = INVALID_ARGUMENT;
    g_CmdLineArguments.nLogType  = INVALID_ARGUMENT;

    g_CmdLineArguments.fBypassCtrlDispatcher = FALSE;// initialize service controller argument

    // process arguments
    while (NumberOfArgs--) {
        pCurrentArg = ArgumentPtrs[NumberOfArgs];// retrieve argument pointer

        // make sure valid argument passed
        if (IS_ARGUMENT(pCurrentArg, LOGLEVEL)) {
            dwArgument = DWORD_ARGUMENT(pCurrentArg, LOGLEVEL);// convert string into dword argument
            g_CmdLineArguments.nLogLevel = dwArgument;// store in global argument structure
            SnmpSvcSetLogLevel(g_CmdLineArguments.nLogLevel);// modify the level at which logging occurs
        } else if (IS_ARGUMENT(pCurrentArg, LOGTYPE)) {
            dwArgument = DWORD_ARGUMENT(pCurrentArg, LOGTYPE);// convert string into dword argument
            g_CmdLineArguments.nLogType = dwArgument;// store in global argument structure
            SnmpSvcSetLogType(g_CmdLineArguments.nLogType);// modify the log type used during logging
        } else if (IS_ARGUMENT(pCurrentArg, DEBUG)) {
            g_CmdLineArguments.fBypassCtrlDispatcher = TRUE;// disable service controller when debugging
        } else if (NumberOfArgs) {
            SNMPDBG((SNMP_LOG_WARNING, "SNMP: SVC: Ignoring argument %s.\n", pCurrentArg));
        }
    }

    return TRUE;
}