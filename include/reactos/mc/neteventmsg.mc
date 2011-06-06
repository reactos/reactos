;
; netevent.mc MESSAGE resources for netevent.dll
;

MessageIdTypedef=DWORD

SeverityNames=(Success=0x0:STATUS_SEVERITY_SUCCESS
               Informational=0x1:STATUS_SEVERITY_INFORMATIONAL
               Warning=0x2:STATUS_SEVERITY_WARNING
               Error=0x3:STATUS_SEVERITY_ERROR
              )

FacilityNames=(System=0x0:FACILITY_SYSTEM
              )

LanguageNames=(English=0x409:MSG00409)


;
; message definitions
;

; Facility=System

; Eventlog messages 6xxx-

MessageId=6005
Severity=Warning
Facility=System
SymbolicName=EVENT_EventlogStarted
Language=English
The Event log service was started.
.

MessageId=6006
Severity=Warning
Facility=System
SymbolicName=EVENT_EventlogStopped
Language=English
The Event log service was stopped.
.

MessageId=6009
Severity=Warning
Facility=System
SymbolicName=EVENT_EventLogProductInfo
Language=English
ReactOS %1 %2 %3 %4.
.


; transport messages

MessageId=9004
Severity=Error
Facility=System
SymbolicName=EVENT_TRANSPORT_REGISTER_FAILED
Language=English
%2 failed to register itself with the NDIS wrapper.
.

MessageId=9006
Severity=Error
Facility=System
SymbolicName=EVENT_TRANSPORT_ADAPTER_NOT_FOUND
Language=English
%2 could not find adapter %3.
.
