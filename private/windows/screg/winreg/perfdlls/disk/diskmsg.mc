;/*++ BUILD Version: 0001    // Increment this if a change has global effects
;
;Copyright (c) 1996  Microsoft Corporation
;
;Module Name:
;
;    diskmsg.h
;       (generated from diskmsg.mc)
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
;#ifndef _DISKMSG_MC_H_
;#define _DISKMSG_MC_H_
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
SymbolicName=PERFDISK_UNABLE_OPEN
Language=English
Unable to open the Disk performance object. Status code returned is
data DWORD 0.
.
MessageId=+1
Severity=Error
Facility=Application
SymbolicName=PERFDISK_NOT_OPEN
Language=English
The attempt to collect Disk Performance data failed beause the DLL did 
not open successfully.
.
MessageId=+1
Severity=Error
Facility=Application
SymbolicName=PERFDISK_UNABLE_ALLOC_BUFFER
Language=English
Unable to allocate a dynamic memory buffer
.
MessageId=+1
Severity=Error
Facility=Application
SymbolicName=PERFDISK_BUSY
Language=English
The Disk Performance data collection function was called by one
thread while in use by another. No will be returned to the 2nd caller.
.
;//
;//     Module specific messages
;//
MessageId=2000
Severity=Warning
Facility=Application
SymbolicName=PERFDISK_UNABLE_QUERY_VOLUME_INFO
Language=English
Unable to read the Logical Volume information from the system.
Status code returned is data DWORD 0.
.
MessageId=+1
Severity=Warning
Facility=Application
SymbolicName=PERFDISK_UNABLE_QUERY_DISKPERF_INFO
Language=English
Unable to read the disk performance information from the system. 
Disk performance counters must be enabled for at least one 
physical disk or logical volume in order for these counters to appear.
Disk performance counters can be enabled by using the Hardware Device Manager property pages.
Status code returned is data DWORD 0.
.
;//
;#endif //_DISKMSG_MC_H_
