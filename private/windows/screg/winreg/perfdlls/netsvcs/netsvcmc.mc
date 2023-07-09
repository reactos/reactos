;/*++ BUILD Version: 0001    // Increment this if a change has global effects
;
;Copyright (c) 1996  Microsoft Corporation
;
;Module Name:
;
;    netsvcmc.h
;       (generated from netsvcmc.mc)
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
;#ifndef _NETSVC_MC_H_
;#define _NETSVC_MC_H_
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
SymbolicName=PERFNET_UNABLE_OPEN
Language=English
Unable to open the Network Services performance object. 
Status code returned is data DWORD 0.
.
MessageId=+1
Severity=Error
Facility=Application
SymbolicName=PERFNET_NOT_OPEN
Language=English
The attempt to collect Network Services Performance data failed 
beause the DLL did not open successfully.
.
;//
;//     Module specific messages
;//
MessageId=2000
Severity=Error
Facility=Application
SymbolicName=PERFNET_UNABLE_OPEN_NETAPI32_DLL
Language=English
Unable to open the NETAPI32.DLL for the collection of Browser performance
data. Browser performance data will not be returned. Error code returned
is in data DWORD 0.
.
MessageId=+1
Severity=Error
Facility=Application
SymbolicName=PERFNET_UNABLE_LOCATE_BROWSER_PERF_FN
Language=English
Unable to locate the Query function for the Browser Performance data in the 
NETAPI32.DLL for the collection of Browser performance data. Browser 
performance data will not be returned. Error code returned is in 
data DWORD 0.
.
MessageId=+1
Severity=Error
Facility=Application
SymbolicName=PERFNET_UNABLE_OPEN_REDIR
Language=English
Unable to open the Redirector service. Redirector performance data 
will not be returned. Error code returned is in data DWORD 0.
.
MessageId=+1
Severity=Error
Facility=Application
SymbolicName=PERFNET_UNABLE_READ_REDIR
Language=English
Unable to read performance data from the Redirector service. 
No Redirector performance data will be returned in this sample. 
Error code returned is in data DWORD 0.
.
MessageId=+1
Severity=Error
Facility=Application
SymbolicName=PERFNET_UNABLE_OPEN_SERVER
Language=English
Unable to open the Server service. Server performance data 
will not be returned. Error code returned is in data DWORD 0.
.
MessageId=+1
Severity=Error
Facility=Application
SymbolicName=PERFNET_UNABLE_READ_SERVER
Language=English
Unable to read performance data from the Server service. 
No Server performance data will be returned in this sample. 
Error code returned is in data DWORD 0, IOSB.Status is DWORD 1 and
the IOSB.Information is DWORD 2.
.
MessageId=+1
Severity=Error
Facility=Application
SymbolicName=PERFNET_UNABLE_READ_SERVER_QUEUE
Language=English
Unable to read Server Queue performance data from the Server service. 
No Server Queue performance data will be returned in this sample. 
Error code returned is in data DWORD 0, IOSB.Status is DWORD 1 and
the IOSB.Information is DWORD 2.
.
;//
;#endif //_NETSVC_MC_H_
