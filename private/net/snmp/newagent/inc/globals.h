/*++

Copyright (c) 1992-1997  Microsoft Corporation

Module Name:

    globals.h

Abstract:

    Contains global definitions for SNMP master agent.

Environment:

    User Mode - Win32

Revision History:

    10-Feb-1997 DonRyan
        Rewrote to implement SNMPv2 support.

--*/
 
#ifndef _GLOBALS_H_
#define _GLOBALS_H_

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Include files                                                             //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <tchar.h>
#include <windef.h>
#include <winsvc.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <wsipx.h>
#include <snmputil.h>
#include "snmpevts.h"
#include "args.h"
#include "mem.h"


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Private definitions                                                       //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////
#define SHUTDOWN_WAIT_HINT 5000


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Global variables                                                          //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

extern DWORD  g_dwUpTimeReference;
extern HANDLE g_hTerminationEvent;
extern HANDLE g_hRegistryEvent;

extern HANDLE g_hDefaultRegNotifier;
extern HKEY   g_hDefaultKey;
extern HANDLE g_hPolicyRegNotifier;
extern HKEY   g_hPolicyKey;

extern LIST_ENTRY g_Subagents;
extern LIST_ENTRY g_SupportedRegions;
extern LIST_ENTRY g_ValidCommunities;
extern LIST_ENTRY g_TrapDestinations;
extern LIST_ENTRY g_PermittedManagers;
extern LIST_ENTRY g_IncomingTransports;
extern LIST_ENTRY g_OutgoingTransports;

extern CRITICAL_SECTION g_RegCriticalSectionA;
extern CRITICAL_SECTION g_RegCriticalSectionB;

extern CMD_LINE_ARGUMENTS g_CmdLineArguments;


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Registry definitions                                                      //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////
#define REG_POLICY_ROOT               \
    TEXT("SOFTWARE\\Policies")

#define REG_POLICY_PARAMETERS         \
    TEXT("SOFTWARE\\Policies\\SNMP\\Parameters")

#define REG_POLICY_VALID_COMMUNITIES  \
    REG_POLICY_PARAMETERS TEXT("\\ValidCommunities")

#define REG_POLICY_TRAP_DESTINATIONS  \
    REG_POLICY_PARAMETERS TEXT("\\TrapConfiguration")

#define REG_POLICY_PERMITTED_MANAGERS \
    REG_POLICY_PARAMETERS TEXT("\\PermittedManagers")

#define REG_KEY_SNMP_PARAMETERS     \
    TEXT("SYSTEM\\CurrentControlSet\\Services\\SNMP\\Parameters")

#define REG_KEY_EXTENSION_AGENTS    \
    REG_KEY_SNMP_PARAMETERS TEXT("\\ExtensionAgents")

#define REG_KEY_VALID_COMMUNITIES   \
    REG_KEY_SNMP_PARAMETERS TEXT("\\ValidCommunities")

#define REG_KEY_TRAP_DESTINATIONS   \
    REG_KEY_SNMP_PARAMETERS TEXT("\\TrapConfiguration")

#define REG_KEY_PERMITTED_MANAGERS  \
    REG_KEY_SNMP_PARAMETERS TEXT("\\PermittedManagers")

#define REG_KEY_MIB2 \
    REG_KEY_SNMP_PARAMETERS TEXT("\\RFC1156Agent")


#define REG_VALUE_SUBAGENT_PATH     "Pathname"
#define REG_VALUE_AUTH_TRAPS        TEXT("EnableAuthenticationTraps")
#define REG_VALUE_MGRRES_COUNTER    TEXT("NameResolutionRetries")
#define REG_VALUE_SYS_OBJECTID      TEXT("sysObjectID")

#define MAX_VALUE_NAME_LEN          256
#define MAX_VALUE_DATA_LEN          256


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Miscellaneous definitions                                                 //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#define PDUTYPESTRING(nPduType) \
            ((nPduType == SNMP_PDU_GETNEXT) \
                ? "getnext" \
                : (nPduType == SNMP_PDU_GETBULK) \
                    ? "getbulk" \
                    : (nPduType == SNMP_PDU_GET) \
                        ? "get" \
                        : (nPduType == SNMP_PDU_SET) \
                            ? "set" \
                            : "unknown")

#define SNMPERRORSTRING(nErr) \
            ((nErr == SNMP_ERRORSTATUS_NOERROR) \
              ? "NOERROR" \
              : (nErr == SNMP_ERRORSTATUS_GENERR) \
               ? "GENERR" \
               : (nErr == SNMP_ERRORSTATUS_NOSUCHNAME) \
                ? "NOSUCHNAME" \
                : (nErr == SNMP_ERRORSTATUS_NOTWRITABLE) \
                 ? "NOTWRITABLE" \
                 : (nErr == SNMP_ERRORSTATUS_TOOBIG) \
                  ? "TOOBIG" \
                  : (nErr == SNMP_ERRORSTATUS_BADVALUE) \
                   ? "BADVALUE" \
                   : (nErr == SNMP_ERRORSTATUS_READONLY) \
                    ? "READONLY" \
                    : (nErr == SNMP_ERRORSTATUS_WRONGTYPE) \
                     ? "WRONGTYPE" \
                     : (nErr == SNMP_ERRORSTATUS_WRONGLENGTH) \
                      ? "WRONGLENGTH" \
                      : (nErr == SNMP_ERRORSTATUS_WRONGENCODING) \
                       ? "WRONGENCODING" \
                       : (nErr == SNMP_ERRORSTATUS_WRONGVALUE) \
                        ? "WRONGVALUE" \
                        : (nErr == SNMP_ERRORSTATUS_NOCREATION) \
                         ? "NOCREATION" \
                         : (nErr == SNMP_ERRORSTATUS_INCONSISTENTVALUE) \
                          ? "INCONSISTENTVALUE" \
                          : (nErr == SNMP_ERRORSTATUS_RESOURCEUNAVAILABLE) \
                           ? "RESOURCEUNAVAILABLE" \
                           : (nErr == SNMP_ERRORSTATUS_COMMITFAILED) \
                            ? "COMMITFAILED" \
                            : (nErr == SNMP_ERRORSTATUS_UNDOFAILED) \
                             ? "UNDOFAILED" \
                             : (nErr == SNMP_ERRORSTATUS_AUTHORIZATIONERROR) \
                              ? "AUTHORIZATIONERROR" \
                              : (nErr == SNMP_ERRORSTATUS_NOACCESS) \
                               ? "NOACCESS" \
                               : (nErr == SNMP_ERRORSTATUS_INCONSISTENTNAME) \
                                ? "INCONSISTENTNAME" \
                                : "unknown")
 
 



























































































#endif // _GLOBALS_H_
