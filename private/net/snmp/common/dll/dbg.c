/*++

Copyright (c) 1992-1997  Microsoft Corporation

Module Name:

    dbg.c

Abstract:

    Contains common SNMP debugging routines.

        SnmpSvcSetLogLevel
        SnmpSvcSetLogType
        SnmpUtilDbgPrint

Environment:

    User Mode - Win32

Revision History:

--*/

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Include files                                                             //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include <snmp.h>
#include <snmputil.h>
#include <stdio.h>
#include <time.h>


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Private Definitions                                                       //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#define MAX_LOG_ENTRY_LEN   512


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Global Variables                                                          //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

INT g_nLogType  = SNMP_OUTPUT_TO_DEBUGGER;  
INT g_nLogLevel = SNMP_LOG_SILENT;    


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Private Procedures                                                        //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

VOID 
OutputLogEntry(
    LPSTR pLogEntry
    )

/*++

Routine Description:

    Writes log entry to log types specified.

Arguments:

    pLogEntry - zero-terminated string containing log entry text.

Return Values:

    None. 

--*/

{
    // initialize descriptor
    static FILE * fd = NULL;

    // check if console output specified
    if (g_nLogType & SNMP_OUTPUT_TO_CONSOLE) {

        // output entry to stream
        fprintf(stdout, "%s", pLogEntry);

        // flush stream
        fflush(stdout);
    }

    // check if logfile output specified
    if (g_nLogType & SNMP_OUTPUT_TO_LOGFILE) {

        // validate    
        if (fd == NULL) {

            // attempt to open log file 
            fd = fopen("snmpdbg.log", "w");
        }

        // validate    
        if (fd != NULL) {

            // output entry to stream
            fprintf(fd, "%s", pLogEntry);

            // flush stream
            fflush(fd);
        }
    }

    // check if debugger output specified
    if (g_nLogType & SNMP_OUTPUT_TO_DEBUGGER) {

        // output entry to debugger
        OutputDebugStringA(pLogEntry);
    }

} 


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Public Procedures                                                         //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////


VOID
SNMP_FUNC_TYPE
SnmpSvcSetLogLevel(
    INT nLogLevel
    )

/*++

Routine Description:

    Modifies the logging level of the SNMP process.

Arguments:

    nLogLevel - new logging level.

Return Values:

    None. 

--*/

{
    // update log level
    g_nLogLevel = nLogLevel; 
}


VOID 
SNMP_FUNC_TYPE
SnmpSvcSetLogType(
    INT nLogType
    )

/*++

Routine Description:

    Modifies the type of log used by the SNMP process.

Arguments:

    nLogType - type of log.

Return Values:

    None. 

--*/

{
    // update log type
    g_nLogType = nLogType;
}


VOID 
SNMP_FUNC_TYPE 
SnmpUtilDbgPrint(
    INT   nLogLevel, 
    LPSTR szFormat, 
    ...
    )

/*++

Routine Description:

    Prints debug message to current log types.

Arguments:

    nLogLevel - log level of message.

    szFormat - formatting string (see printf).

Return Values:

    None. 

--*/

{
    va_list arglist;

	// 640 octets should be enough to encode oid's of 128 sub-ids.
	// (one subid can be encoded on at most 5 octets; there can be at
	// 128 sub-ids per oid. MAX_LOG_ENTRY_LEN = 512
    char szLogEntry[4*MAX_LOG_ENTRY_LEN];

    // validate entry's level
    if (nLogLevel <= g_nLogLevel) {

        time_t now;

        // initialize variable args
        va_start(arglist, szFormat);

        time(&now);
        strftime(szLogEntry, MAX_LOG_ENTRY_LEN, "%H:%M:%S :", localtime(&now));

        // transfer variable args to buffer
        vsprintf(szLogEntry + strlen(szLogEntry), szFormat, arglist);

        // actually output entry
        OutputLogEntry(szLogEntry);
    }
}
