;/*++
;
;Copyright (c) 1992-1996  Microsoft Corporation
;
;Module Name:
;
;    evtlog.h
;
;Abstract:
;
;    Event Logger Message definitions for the SNMP Service.
;
;Environment:
;
;    User Mode - Win32
;
;Revision History:
;
;    10-May-1996 DonRyan
;        Removed banner from Technology Dynamics, Inc.
;
;--*/

;
;#ifndef _EVTLOG_
;#define _EVTLOG_
;

MessageIdTypedef=DWORD

SeverityNames=(Success=0x0:STATUS_SEVERITY_SUCCESS
               Informational=0x1:STATUS_SEVERITY_INFORMATIONAL
               Warning=0x2:STATUS_SEVERITY_WARNING
               Error=0x3:STATUS_SEVERITY_ERROR
               )

;
;/////////////////////////////////////////////////////////////////////////
;//
;// SNMP events 1-1100 are informational
;//
;/////////////////////////////////////////////////////////////////////////
;

MessageId=1001 Severity=Informational Facility=Application SymbolicName=SNMP_EVENT_SERVICE_STARTED
Language=English
The SNMP Service has started successfully.
.

MessageId=1003 Severity=Informational Facility=Application SymbolicName=SNMP_EVENT_SERVICE_STOPPED
Language=English
The SNMP Service has stopped successfully.
.


;
;/////////////////////////////////////////////////////////////////////////
;//
;// SNMP events 1100-1998 are warnings and errors
;//
;/////////////////////////////////////////////////////////////////////////
;

MessageId=1101 Severity=Error Facility=Application SymbolicName=SNMP_EVENT_FATAL_ERROR
Language=English
The SNMP Service has encountered a fatal error.
.

MessageId=1102 Severity=Warning Facility=Application SymbolicName=SNMP_EVENT_INVALID_TRAP_DESTINATION
Language=English
The SNMP Service is ignoring trap destination %1 because it is invalid.
.

MessageId=1103 Severity=Error Facility=Application SymbolicName=SNMP_EVENT_INVALID_PLATFORM_ID
Language=English
The SNMP Service is not designed for this operating system.
.

MessageId=1104 Severity=Error Facility=Application SymbolicName=SNMP_EVENT_INVALID_REGISTRY_KEY
Language=English
The SNMP Service registry key %1 is missing or misconfigured.
.

MessageId=1105 Severity=Warning Facility=Application SymbolicName=SNMP_EVENT_INVALID_EXTENSION_AGENT_KEY
Language=English
The SNMP Service is ignoring extension agent key %1 because it is missing or misconfigured.
.

MessageId=1106 Severity=Warning Facility=Application SymbolicName=SNMP_EVENT_INVALID_EXTENSION_AGENT_DLL
Language=English
The SNMP Service is ignoring extension agent dll %1 because it is missing or misconfigured.
.

;
;/////////////////////////////////////////////////////////////////////////
;//
;// SNMP events 1999 is used to display debug messages
;//
;/////////////////////////////////////////////////////////////////////////
;

MessageId=1999 Severity=Informational Facility=Application SymbolicName=SNMP_EVENT_DEBUG_TRACE
Language=English
%1
.


;
;#endif // _EVTLOG_
;
