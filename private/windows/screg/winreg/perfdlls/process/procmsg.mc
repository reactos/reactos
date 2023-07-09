;/*++ BUILD Version: 0001    // Increment this if a change has global effects
;
;Copyright (c) 1996  Microsoft Corporation
;
;Module Name:
;
;    PROCMSG.h
;       (generated from PROCMSG.mc)
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
;#ifndef _PROCMSG_MC_H_
;#define _PROCMSG_MC_H_
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
Severity=Warning
Facility=Application
SymbolicName=PERFPROC_UNABLE_OPEN
Language=English
Unable to open the Disk performance object. Status code returned is
data DWORD 0.
.
MessageId=+1
Severity=Error
Facility=Application
SymbolicName=PERFPROC_NOT_OPEN
Language=English
The attempt to collect Disk Performance data failed beause the DLL did 
not open successfully.
.
;//
;//     Module specific messages
;//
MessageId=2000
Severity=Error
Facility=Application
SymbolicName=PERFPROC_UNABLE_QUERY_PROCESS_INFO
Language=English
Unable to collect system process performance information. Status code
returned is data DWORD 0.
.
MessageId=+1
Severity=Error
Facility=Application
SymbolicName=PERFPROC_UNABLE_QUERY_VM_INFO
Language=English
Unable to collect process virtual memory information. Status code
returned is data DWORD 0.
.
;//
MessageId=+1
Severity=Warning
Facility=Application
SymbolicName=PERFPROC_UNABLE_OPEN_JOB
Language=English
Unable to open the job object %1 for query access. 
The calling process may not have permission to open this Job. 
The status returned is data DWORD 0.
.
;//
MessageId=+1
Severity=Warning
Facility=Application
SymbolicName=PERFPROC_UNABLE_QUERY_JOB_ACCT
Language=English
Unable to query the job object %1 for its accounting info. 
The calling process may not have permission to query this Job. 
The status returned is data DWORD 0.
.
;//
MessageId=+1
Severity=Warning
Facility=Application
SymbolicName=PERFPROC_UNABLE_QUERY_OBJECT_DIR
Language=English
Unable to query the %1 Object Directory to look for Job Objects.
The calling process may not have permission to perform this query.
The status returned is data DWORD 0.
.
;//
MessageId=+1
Severity=Warning
Facility=Application
SymbolicName=PERFPROC_UNABLE_QUERY_JOB_PIDS
Language=English
Unable to query the job object %1 for its Process IDs
The calling process may not have permission to query this Job. 
The status returned is data DWORD 0.
.
;//
;#endif //_PROCMSG_MC_H_
