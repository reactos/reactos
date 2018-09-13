;/*--
;Copyright (c) 1997  Microsoft Corporation
;
;Module Name:
;
;    userctrs.h
;       (derived from userctrs.mc by the message compiler  )
;
;Abstract:
;
;   Event message definitions used by routines in PERFUSER.DLL
;
;Revision History:
;
;--*/

;//
;#ifndef _USERCTRS_H_
;#define _USERCTRS_H_
;//
MessageIdTypedef=DWORD
;//
;//     Perfutil messages
;//
MessageId=1900
Severity=Informational
Facility=Application
SymbolicName=UTIL_LOG_OPEN
Language=English
An extensible counter has opened the Event Log for PERFUSER.DLL
.
;//
MessageId=1999
Severity=Informational
Facility=Application
SymbolicName=UTIL_CLOSING_LOG
Language=English
An extensible counter has closed the Event Log for PERFUSER.DLL
.
;//
MessageId=+1
Severity=Error
Facility=Application
SymbolicName=USERPERF_UNABLE_OPEN_DRIVER_KEY
Language=English
Unable to open "Performance" key in registry. Status code is returned in data.
.
;//
MessageId=+1
Severity=Error
Facility=Application
SymbolicName=USERPERF_UNABLE_READ_FIRST_COUNTER
Language=English
Unable to read the "First Counter" value under the PerfUser\Performance Key. Status codes returned in data.
.
;//
MessageId=+1
Severity=Error
Facility=Application
SymbolicName=USERPERF_UNABLE_READ_FIRST_HELP
Language=English
Unable to read the "First Help" value under the USER\Performance Key. Status codes returned in data.
.
;//
MessageId=+1
Severity=Error
Facility=Application
SymbolicName=USERPERF_CS_CANT_QUERY
Language=English
Can't query the USER Critical Section!  Make sure your win32k.sys was compiled with the USER_PERFORMANCE environment variable defined.
.
;//
;#endif // _USERCTRS_H_
