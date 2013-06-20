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

;
; eventlog events 6000-6099
;

MessageId=6000
Severity=Warning
Facility=System
SymbolicName=EVENT_LOG_FULL
Language=English
The %1 log file is full.
.

MessageId=6001
Severity=Warning
Facility=System
SymbolicName=EVENT_LogFileNotOpened
Language=English
The %1 log file cannot be opened.
.

MessageId=6002
Severity=Warning
Facility=System
SymbolicName=EVENT_LogFileCorrupt
Language=English
The %1 log file is corrupted and will be cleared.
.

MessageId=6003
Severity=Warning
Facility=System
SymbolicName=EVENT_DefaultLogCorrupt
Language=English
The Application log file could not be opened.  %1 will be used as the default log file.
.

MessageId=6004
Severity=Warning
Facility=System
SymbolicName=EVENT_BadDriverPacket
Language=English
A driver packet received from the I/O subsystem was invalid.  The data is the packet.
.

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

MessageId=6007
Severity=Warning
Facility=System
SymbolicName=TITLE_EventlogMessageBox
Language=English
Eventlog Service %0
.

MessageId=6008
Severity=Warning
Facility=System
SymbolicName=EVENT_EventlogAbnormalShutdown
Language=English
The previous system shutdown at %1 on %2 was unexpected.
.

MessageId=6009
Severity=Warning
Facility=System
SymbolicName=EVENT_EventLogProductInfo
Language=English
ReactOS %1 %2 %3 %4.
.

MessageId=6010
Severity=Error
Facility=System
SymbolicName=EVENT_ServiceNoEventLog
Language=English
The %1 service was unable to set up an event source.
.

MessageId=6011
Severity=Error
Facility=System
SymbolicName=EVENT_ComputerNameChange
Language=English
The NetBIOS name and DNS host name of this machine have been changed from %1 to %2.
.

MessageId=6012
Severity=Error
Facility=System
SymbolicName=EVENT_DNSDomainNameChange
Language=English
The DNS domain assigned to this computer has been changed from %1 to %2.
.


;
; system events 6100 - 6199
;

MessageId=6100
Severity=Error
Facility=System
SymbolicName=EVENT_UP_DRIVER_ON_MP
Language=English
A uniprocessor-specific driver was loaded on a multiprocessor system.  The driver could not load.
.


;
; service controller events 7000-7899
;

MessageId=7000
Severity=Error
Facility=System
SymbolicName=EVENT_SERVICE_START_FAILED
Language=English
The %1 service failed to start due to the following error: %n%2
.

MessageId=7001
Severity=Error
Facility=System
SymbolicName=EVENT_SERVICE_START_FAILED_II
Language=English
The %1 service depends on the %2 service which failed to start because of the following error: %n%3
.

MessageId=7002
Severity=Error
Facility=System
SymbolicName=EVENT_SERVICE_START_FAILED_GROUP
Language=English
The %1 service depends on the %2 group and no member of this group started.
.

MessageId=7003
Severity=Error
Facility=System
SymbolicName=EVENT_SERVICE_START_FAILED_NONE
Language=English
The %1 service depends on the following nonexistent service: %2
.

MessageId=7004
Severity=Error
Facility=System
SymbolicName=EVENT_CALL_TO_FUNCTION_FAILED
Language=English
The %1 call failed with the following error: %n%2
.

MessageId=7005
Severity=Error
Facility=System
SymbolicName=EVENT_CALL_TO_FUNCTION_FAILED_II
Language=English
The %1 call failed for %2 with the following error: %n%3
.

MessageId=7006
Severity=Error
Facility=System
SymbolicName=EVENT_REVERTED_TO_LASTKNOWNGOOD
Language=English
The system reverted to its last known good configuration.  The system is restarting....
.

MessageId=7007
Severity=Error
Facility=System
SymbolicName=EVENT_BAD_ACCOUNT_NAME
Language=English
No backslash is in the account name.
.

MessageId=7008
Severity=Error
Facility=System
SymbolicName=EVENT_CONNECTION_TIMEOUT
Language=English
Timeout (%1 milliseconds) waiting for the %2 service to connect.
.

MessageId=7009
Severity=Error
Facility=System
SymbolicName=EVENT_READFILE_TIMEOUT
Language=English
Timeout (%1 milliseconds) waiting for ReadFile.
.

MessageId=7010
Severity=Error
Facility=System
SymbolicName=EVENT_TRANSACT_TIMEOUT
Language=English
Timeout (%1 milliseconds) waiting for a transaction response from the %2 service.
.

MessageId=7011
Severity=Error
Facility=System
SymbolicName=EVENT_TRANSACT_INVALID
Language=English
Message returned in transaction has incorrect size.
.

MessageId=7012
Severity=Error
Facility=System
SymbolicName=EVENT_FIRST_LOGON_FAILED
Language=English
Logon attempt with current password failed with the following error: %n%1
.

MessageId=7013
Severity=Error
Facility=System
SymbolicName=EVENT_SECOND_LOGON_FAILED
Language=English
Second logon attempt with old password also failed with the following error: %n%1
.

MessageId=7014
Severity=Error
Facility=System
SymbolicName=EVENT_INVALID_DRIVER_DEPENDENCY
Language=English
Boot-start or system-start driver (%1) must not depend on a service.
.

MessageId=7015
Severity=Error
Facility=System
SymbolicName=EVENT_BAD_SERVICE_STATE
Language=English
The %1 service has reported an invalid current state %2.
.

MessageId=7016
Severity=Error
Facility=System
SymbolicName=EVENT_CIRCULAR_DEPENDENCY_DEMAND
Language=English
Detected circular dependencies demand starting %1.
.

MessageId=7017
Severity=Error
Facility=System
SymbolicName=EVENT_CIRCULAR_DEPENDENCY_AUTO
Language=English
Detected circular dependencies auto-starting services.
.

MessageId=7018
Severity=Error
Facility=System
SymbolicName=EVENT_DEPEND_ON_LATER_SERVICE
Language=English
Circular dependency: The %1 service depends on a service in a group which starts later.
.

MessageId=7019
Severity=Error
Facility=System
SymbolicName=EVENT_DEPEND_ON_LATER_GROUP
Language=English
Circular dependency: The %1 service depends on a group which starts later.
.

MessageId=7020
Severity=Error
Facility=System
SymbolicName=EVENT_SEVERE_SERVICE_FAILED
Language=English
About to revert to the last known good configuration because the %1 service failed to start.
.

MessageId=7021
Severity=Error
Facility=System
SymbolicName=EVENT_SERVICE_START_HUNG
Language=English
The %1 service hung on starting.
.

MessageId=7022
Severity=Error
Facility=System
SymbolicName=EVENT_SERVICE_EXIT_FAILED
Language=English
The %1 service terminated with the following error: %n%2
.

MessageId=7023
Severity=Error
Facility=System
SymbolicName=EVENT_SERVICE_EXIT_FAILED_SPECIFIC
Language=English
The %1 service terminated with service-specific error %2.
.

MessageId=7024
Severity=Error
Facility=System
SymbolicName=EVENT_SERVICE_START_AT_BOOT_FAILED
Language=English
At least one service or driver failed during system startup.  Use Event Viewer to examine the event log for details.
.

MessageId=7025
Severity=Error
Facility=System
SymbolicName=EVENT_BOOT_SYSTEM_DRIVERS_FAILED
Language=English
The following boot-start or system-start driver(s) failed to load: %1
.

MessageId=7026
Severity=Error
Facility=System
SymbolicName=EVENT_RUNNING_LASTKNOWNGOOD
Language=English
ReactOS could not be started as configured.  A previous working configuration was used instead.
.

MessageId=7027
Severity=Error
Facility=System
SymbolicName=EVENT_TAKE_OWNERSHIP
Language=English
The %1 Registry key denied access to SYSTEM account programs so the Service Control Manager took ownership of the Registry key.
.

MessageId=7028
Severity=Error
Facility=System
SymbolicName=TITLE_SC_MESSAGE_BOX
Language=English
Service Control Manager %0
.

MessageId=7029
Severity=Error
Facility=System
SymbolicName=EVENT_SERVICE_NOT_INTERACTIVE
Language=English
The %1 service is marked as an interactive service.  However, the system is configured to not allow interactive services.  This service may not function properly.
.

MessageId=7030
Severity=Error
Facility=System
SymbolicName=EVENT_SERVICE_CRASH
Language=English
The %1 service terminated unexpectedly.  It has done this %2 time(s).  The following corrective action will be taken in %3 milliseconds: %5.
.

MessageId=7031
Severity=Error
Facility=System
SymbolicName=EVENT_SERVICE_RECOVERY_FAILED
Language=English
The Service Control Manager tried to take a corrective action (%2) after the unexpected termination of the %3 service, but this action failed with the following error: %n%4
.

MessageId=7032
Severity=Error
Facility=System
SymbolicName=EVENT_SERVICE_SCESRV_FAILED
Language=English
The Service Control Manager did not initialize successfully. The security
configuration server (scesrv.dll) failed to initialize with error %1.  The
system is restarting...
.

MessageId=7033
Severity=Error
Facility=System
SymbolicName=EVENT_SERVICE_CRASH_NO_ACTION
Language=English
The %1 service terminated unexpectedly.  It has done this %2 time(s).
.

MessageId=7034
Severity=Informational
Facility=System
SymbolicName=EVENT_SERVICE_CONTROL_SUCCESS
Language=English
The %1 service was successfully sent a %2 control.
.

MessageId=7035
Severity=Informational
Facility=System
SymbolicName=EVENT_SERVICE_STATUS_SUCCESS
Language=English
Service Control Manager %0
.

MessageId=7036
Severity=Error
Facility=System
SymbolicName=EVENT_SERVICE_CONFIG_BACKOUT_FAILED
Language=English
The Service Control Manager encountered an error undoing a configuration change
to the %1 service.  The service's %2 is currently in an unpredictable state.

If you do not correct this configuration, you may not be able to restart the %1
service or may encounter other errors.  To ensure that the service is configured
properly, use the Services snap-in in Microsoft Management Console (MMC).
.

MessageId=7037
Severity=Error
Facility=System
SymbolicName=EVENT_FIRST_LOGON_FAILED_II
Language=English
The %1 service was unable to log on as %2 with the currently configured
password due to the following error: %n%3%n%nTo ensure that the service is
configured properly, use the Services snap-in in Microsoft Management
Console (MMC).
.


;
; transport events 9000-9499
;

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
