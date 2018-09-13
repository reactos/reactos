/*++

Copyright (c) 1992-1997  Microsoft Corporation

Module Name:

    service.h

Abstract:

    Contains service definitions for SNMP master agent.

Environment:

    User Mode - Win32

Revision History:

    10-Feb-1997 DonRyan
        Rewrote to implement SNMPv2 support.

--*/
 
#ifndef _SERVICE_H_
#define _SERVICE_H_

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// SNMP service name                                                         //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#define SNMP_SERVICE                TEXT("SNMP")

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// SNMP log level limits (must be consistent with SNMP_LOG_ contants)        //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#define SNMP_LOGLEVEL_MINIMUM       0  
#define SNMP_LOGLEVEL_MAXIMUM       20 

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// SNMP log type limits (must be consistent with SNMP_OUTPUT_ contants)      //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#define SNMP_LOGTYPE_MINIMUM        0   
#define SNMP_LOGTYPE_MAXIMUM        10  

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// SNMP service parameter offsets (used in control handler)                  //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#define SNMP_SERVICE_CONTROL_BASE   128
#define SNMP_SERVICE_LOGLEVEL_BASE  SNMP_SERVICE_CONTROL_BASE
#define SNMP_SERVICE_LOGTYPE_BASE   \
    (SNMP_SERVICE_LOGLEVEL_BASE + SNMP_LOGLEVEL_MAXIMUM + 1)


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// SNMP service parameter macro definitions                                  //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#define IS_LOGLEVEL(x) \
    (((x) >= (SNMP_SERVICE_LOGLEVEL_BASE + SNMP_LOGLEVEL_MINIMUM)) && \
     ((x) <= (SNMP_SERVICE_LOGLEVEL_BASE + SNMP_LOGLEVEL_MAXIMUM)))

#define IS_LOGTYPE(x) \
    (((x) >= (SNMP_SERVICE_LOGTYPE_BASE + SNMP_LOGTYPE_MINIMUM)) && \
     ((x) <= (SNMP_SERVICE_LOGTYPE_BASE + SNMP_LOGTYPE_MAXIMUM)))

#define IS_LOGLEVEL_VALID(x) \
    (((x) >= SNMP_LOGLEVEL_MINIMUM) && ((x) <= SNMP_LOGLEVEL_MAXIMUM))

#define IS_LOGTYPE_VALID(x) \
    (((x) >= SNMP_LOGTYPE_MINIMUM) && ((x) <= SNMP_LOGTYPE_MAXIMUM))


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// SNMP service status definitions                                           //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#define NO_WAIT_HINT    0
#define SNMP_WAIT_HINT  30000


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// SNMP service debug string macro definitions                               //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#define SNMP_LOGLEVEL_STRING(x) \
    (((x) == SNMP_LOG_SILENT) \
        ? "SILENT" \
        : ((x) == SNMP_LOG_FATAL) \
            ? "FATAL" \
            : ((x) == SNMP_LOG_ERROR) \
                ? "ERROR" \
                : ((x) == SNMP_LOG_WARNING) \
                    ? "WARNING" \
                    : ((x) == SNMP_LOG_TRACE) \
                        ? "TRACE" \
                        : ((x) == SNMP_LOG_VERBOSE) \
                            ? "VERBOSE" \
                            : "UNKNOWN" \
                            )

#define SNMP_LOGTYPE_STRING(x) \
    (((x) == SNMP_OUTPUT_TO_CONSOLE) \
        ? "CONSOLE" \
        : ((x) == SNMP_OUTPUT_TO_LOGFILE) \
            ? "LOGFILE" \
            : ((x) == SNMP_OUTPUT_TO_EVENTLOG) \
                ? "EVENTLOG" \
                : ((x) == SNMP_OUTPUT_TO_DEBUGGER) \
                    ? "DEBUGGER" \
                    : "UNKNOWN" \
                    )

#define SERVICE_STATUS_STRING(x) \
    (((x) == SERVICE_STOPPED) \
        ? "STOPPED" \
        : ((x) == SERVICE_START_PENDING) \
              ? "START PENDING" \
              : ((x) == SERVICE_STOP_PENDING) \
                    ? "STOP PENDING" \
                    : ((x) == SERVICE_RUNNING) \
                          ? "RUNNING" \
                          : ((x) == SERVICE_CONTINUE_PENDING) \
                                ? "CONTINUE PENDING" \
                                : ((x) == SERVICE_PAUSE_PENDING) \
                                      ? "PAUSE PENDING" \
                                      : ((x) == SERVICE_PAUSED) \
                                            ? "PAUSED" \
                                            : "UNKNOWN" \
                                            )

#define SERVICE_CONTROL_STRING(x) \
    (((x) == SERVICE_CONTROL_STOP) \
        ? "STOP" \
        : ((x) == SERVICE_CONTROL_PAUSE) \
            ? "PAUSE" \
            : ((x) == SERVICE_CONTROL_CONTINUE) \
                ? "CONTINUE" \
                : ((x) == SERVICE_CONTROL_INTERROGATE) \
                    ? "INTERROGATE" \
                    : ((x) == SERVICE_CONTROL_SHUTDOWN) \
                        ? "SHUTDOWN" \
                        : "CONFIGURE" \
                        )

#endif // _SERVICE_H_
