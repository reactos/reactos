;/*++ BUILD Version: 0001    // Increment this if a change has global effects
;
;Copyright (c) 1996  Microsoft Corporation
;
;Module Name:
;
;    perfosmc.h
;       (generated from perfosmc.mc)
;
;Abstract:
;
;   Event message definititions used by routines in PerfOS Perf DLL
;
;Created:
;
;    23-Oct-96 Bob Watson
;
;Revision History:
;
;--*/
;#ifndef _PERFOS_MC_H_
;#define _PERFOS_MC_H_
;
MessageIdTypedef=DWORD
;//
;//     Perflib ERRORS
;//
SeverityNames=(Success=0x0:STATUS_SEVERITY_SUCCESS
               Informational=0x1:STATUS_SEVERITY_INFORMATIONAL
               Warning=0x2:STATUS_SEVERITY_WARNING
               Error=0x3:STATUS_SEVERITY_ERROR
              )

;//
;// Common messages
;//
MessageId=1000
Severity=Error
Facility=Application
SymbolicName=PERFOS_UNABLE_OPEN
Language=English
Unable to open the object. Status code returned is data DWORD 0.
.
MessageId=+1
Severity=Error
Facility=Application
SymbolicName=PERFOS_NOT_OPEN
Language=English
The attempt to collect OS Performance data failed beause the DLL did 
not open successfully.
.
;//
;//     Module specific messages
;//
MessageId=2000
Severity=Warning
Facility=Application
SymbolicName=PERFOS_UNABLE_QUERY_PROCSSOR_INFO
Language=English
Unable to get processor performance information from system. Status
code returned is data DWORD 0.
.
MessageId=+1
Severity=Warning
Facility=Application
SymbolicName=PERFOS_UNABLE_QUERY_INTERRUPT_INFO
Language=English
Unable to get interrupt performance information from system. Status
code returned is data DWORD 0.
.
MessageId=+1
Severity=Warning
Facility=Application
SymbolicName=PERFOS_UNABLE_QUERY_FILE_CACHE_INFO
Language=English
Unable to get file cache performance information from system. Status
code returned is data DWORD 0.
.
MessageId=+1
Severity=Warning
Facility=Application
SymbolicName=PERFOS_UNABLE_QUERY_PROCESS_OBJECT_INFO
Language=English
Unable to get process object information from system. Status
code returned is data DWORD 0.
.
MessageId=+1
Severity=Warning
Facility=Application
SymbolicName=PERFOS_UNABLE_QUERY_THREAD_OBJECT_INFO
Language=English
Unable to get thread object information from system. Status
code returned is data DWORD 0.
.
MessageId=+1
Severity=Warning
Facility=Application
SymbolicName=PERFOS_UNABLE_QUERY_EVENT_OBJECT_INFO
Language=English
Unable to get event object information from system. Status
code returned is data DWORD 0.
.
MessageId=+1
Severity=Warning
Facility=Application
SymbolicName=PERFOS_UNABLE_QUERY_SEMAPHORE_OBJECT_INFO
Language=English
Unable to get semaphore object information from system. Status
code returned is data DWORD 0.
.
MessageId=+1
Severity=Warning
Facility=Application
SymbolicName=PERFOS_UNABLE_QUERY_MUTEX_OBJECT_INFO
Language=English
Unable to get mutex object information from system. Status
code returned is data DWORD 0.
.
MessageId=+1
Severity=Warning
Facility=Application
SymbolicName=PERFOS_UNABLE_QUERY_SECTION_OBJECT_INFO
Language=English
Unable to get section object information from system. Status
code returned is data DWORD 0.
.
MessageId=+1
Severity=Error
Facility=Application
SymbolicName=PERFOS_UNABLE_QUERY_BASIC_INFO
Language=English
Unable to get basic system configuration information. Status
code returned is data DWORD 0.
.
MessageId=+1
Severity=Warning
Facility=Application
SymbolicName=PERFOS_UNABLE_QUERY_SYS_PERF_INFO
Language=English
Unable to get system performance information from system. Status
code returned is data DWORD 0.
.
MessageId=+1
Severity=Warning
Facility=Application
SymbolicName=PERFOS_UNABLE_QUERY_PAGEFILE_INFO
Language=English
Unable to get system pagefile information from system. Status
code returned is data DWORD 0.
.
MessageId=+1
Severity=Warning
Facility=Application
SymbolicName=PERFOS_UNABLE_QUERY_PROCESS_INFO
Language=English
Unable to get system process information from system. Status
code returned is data DWORD 0.
.
MessageId=+1
Severity=Warning
Facility=Application
SymbolicName=PERFOS_UNABLE_QUERY_EXCEPTION_INFO
Language=English
Unable to get system exception information from system. Status
code returned is data DWORD 0.
.
MessageId=+1
Severity=Warning
Facility=Application
SymbolicName=PERFOS_UNABLE_QUERY_REGISTRY_QUOTA_INFO
Language=English
Unable to get registry quota information from system. Status
code returned is data DWORD 0.
.
MessageId=+1
Severity=Warning
Facility=Application
SymbolicName=PERFOS_UNABLE_QUERY_SYSTEM_TIME_INFO
Language=English
Unable to get system process information from system. Status
code returned is data DWORD 0.
.
;
;#endif //_PERFOS_MC_H_
