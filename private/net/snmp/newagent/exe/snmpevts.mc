;/*++
;
;Copyright (c) 1992-1997  Microsoft Corporation
;
;Module Name:
;
;    snmpevts.h
;
;Abstract:
;
;    Eventlog message definitions for the SNMP Service.
;
;Environment:
;
;    User Mode - Win32
;
;Revision History:
;
;
;--*/
;
;#ifndef _SNMPEVTS_
;#define _SNMPEVTS_
;
;/////////////////////////////////////////////////////////////////////////
;//                                                                     //
;// Public procedures                                                   //
;//                                                                     //
;/////////////////////////////////////////////////////////////////////////
;
;VOID
;SNMP_FUNC_TYPE
;ReportSnmpEvent(
;    DWORD   nMsgId, 
;    DWORD   cSubStrings, 
;    LPTSTR *ppSubStrings,
;    DWORD   nErrorCode
;    );

MessageIdTypedef=DWORD

SeverityNames=(Success=0x0:STATUS_SEVERITY_SUCCESS
               Informational=0x1:STATUS_SEVERITY_INFORMATIONAL
               Warning=0x2:STATUS_SEVERITY_WARNING
               Error=0x3:STATUS_SEVERITY_ERROR
               )

FacilityNames=( System=0x0FF
                Appication=0xFFF )

;
;/////////////////////////////////////////////////////////////////////////
;//                                                                     //
;// SNMP events 1-1100 are informational                                //
;//                                                                     //
;/////////////////////////////////////////////////////////////////////////
;

MessageId=1001 Severity=Informational Facility=System SymbolicName=SNMP_EVENT_SERVICE_STARTED
Language=English
The SNMP Service has started successfully.
.

MessageId=1003 Severity=Informational Facility=System SymbolicName=SNMP_EVENT_SERVICE_STOPPED
Language=English
The SNMP Service has stopped successfully.
.

MessageId=1004 Severity=Informational Facility=System SymbolicName=SNMP_EVENT_CONFIGURATION_UPDATED
Language=English
The SNMP Service configuration has been updated successfully.
.

;
;/////////////////////////////////////////////////////////////////////////
;//                                                                     //
;// SNMP events 1100-1499 are warnings                                  //
;//                                                                     //
;/////////////////////////////////////////////////////////////////////////
;

MessageId=1100 Severity=Warning Facility=System SymbolicName=SNMP_EVENT_NAME_RESOLUTION_FAILURE
Language=English
The SNMP Service is ignoring the manager %1 because its name could not be resolved.
.

MessageId= Severity=Warning Facility=System SymbolicName=SNMP_EVENT_INVALID_EXTENSION_AGENT_KEY
Language=English
The SNMP Service is ignoring extension agent key %1 because it is missing or misconfigured.
.

MessageId= Severity=Warning Facility=System SymbolicName=SNMP_EVENT_INVALID_EXTENSION_AGENT_DLL
Language=English
The SNMP Service is ignoring extension agent dll %1 because it is missing or misconfigured.
.

MessageId= Severity=Warning Facility=System SymbolicName=SNMP_EVENT_INVALID_ENTERPRISEOID
Language=English
The SNMP Service has reset the registry parameter sysObjectID to a default value.
This is caused either by an invalid type or by an invalid string format of the registry value.
.

;/////////////////////////////////////////////////////////////////////////
;//                                                                     //
;// SNMP events 1500-1998 are warnings                                  //
;//                                                                     //
;/////////////////////////////////////////////////////////////////////////
;
MessageId=1500 Severity=Error Facility=System SymbolicName=SNMP_EVENT_INVALID_REGISTRY_KEY
Language=English
The SNMP Service encountered an error while accessing the registry key %1.
.

MessageId= Severity=Error Facility=System SymbolicName=SNMP_EVNT_INCOMING_TRANSPORT_CLOSED
Language=English
The SNMP Service encountered an error while setting up the incoming transports.\n
The %1 transport has been dropped out.
.

MessageId= Severity=Error Facility=System SymbolicName=SNMP_EVENT_REGNOTIFY_THREAD_FAILED
Language=English
The SNMP Service encountered an error while registering for registry notifications.\n
Changes in the service's configuration will not be considered.
.

;
;/////////////////////////////////////////////////////////////////////////
;//                                                                     //
;// SNMP events 1999 is used to display debug messages (obsolete)       //
;//                                                                     //
;/////////////////////////////////////////////////////////////////////////
;

MessageId=1999 Severity=Informational Facility=System SymbolicName=SNMP_EVENT_DEBUG_TRACE
Language=English
%1
.


;
;#endif // _SNMPEVTS_
;
