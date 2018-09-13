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
 
///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Include files                                                             //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include "globals.h"
#include "args.h"
#include "stdlib.h"


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Public procedures                                                         //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

BOOL
ProcessArguments(
    DWORD  NumberOfArgs,
    LPSTR ArgumentPtrs[]
    )

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

    // initialize service controller argument
    g_CmdLineArguments.fBypassCtrlDispatcher = FALSE;

    // process arguments
    while (NumberOfArgs--) {

        // retrieve argument pointer
        pCurrentArg = ArgumentPtrs[NumberOfArgs];

        // make sure valid argument passed
        if (IS_ARGUMENT(pCurrentArg, LOGLEVEL)) {

            // convert string into dword argument
            dwArgument = DWORD_ARGUMENT(pCurrentArg, LOGLEVEL);

            // store in global argument structure
            g_CmdLineArguments.nLogLevel = dwArgument;

            // modify the level at which logging occurs
            SnmpSvcSetLogLevel(g_CmdLineArguments.nLogLevel);

        } else if (IS_ARGUMENT(pCurrentArg, LOGTYPE)) {

            // convert string into dword argument
            dwArgument = DWORD_ARGUMENT(pCurrentArg, LOGTYPE);

            // store in global argument structure
            g_CmdLineArguments.nLogType = dwArgument;

            // modify the log type used during logging 
            SnmpSvcSetLogType(g_CmdLineArguments.nLogType);

        } else if (IS_ARGUMENT(pCurrentArg, DEBUG)) {

            // disable service controller when debugging
            g_CmdLineArguments.fBypassCtrlDispatcher = TRUE;

        } else if (NumberOfArgs) {
            
            SNMPDBG((
                SNMP_LOG_WARNING,
                "SNMP: SVC: Ignoring argument %s.\n",
                pCurrentArg
                ));
        }
    }

    return TRUE;
}
