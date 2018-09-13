;/*++ BUILD Version: 0001    // Increment this if a change has global effects
;
;Copyright (c) 1992  Microsoft Corporation
;
;Module Name:
;
;    snmpelmg.h
;
;Abstract:
;
;    Definitions for SNMP Event Log messages.
;
;Author:
;
;    Randy G. Braze  16-October-1994
;
;Revision History:
;
;Notes:
;
;
;
;--*/
;
;#ifndef SNMPELMSG_H
;#define SNMPELMSG_H
;

SeverityNames=(Success=0x0:SNMPELEA_SUCCESS
               Informational=0x1:SNMPELEA_INFORMATIONAL
               Warning=0x2:SNMPELEA_WARNING
               Error=0x3:SNMPELEA_ERROR
              )

;
;/////////////////////////////////////////////////////////////////////////
;//
;// Initialization and Registry Messages (1000-1999)
;//
;/////////////////////////////////////////////////////////////////////////
;

MessageId=1000 Severity=Warning SymbolicName=SNMPELEA_NO_REGISTRY_PARAMETERS
Language=English
Error opening registry for Parameter information; defaults used for all parameters.
Return code from RegOpenKeyEx is %1.
.

MessageId=+1 Severity=Error SymbolicName=SNMPELEA_NO_REGISTRY_LOG_NAME
Language=English
Error opening registry for EventLogFiles information. Extension agent cannot run.
Return code from RegOpenKeyEx is %1.
.

MessageId=+1 Severity=Error SymbolicName=SNMPELEA_NO_REGISTRY_EVENT_LOGS
Language=English
No Event Logs were specified for event scanning.
Extension agent is terminating.
.

MessageId=+1 Severity=Warning SymbolicName=SNMPELEA_NO_REGISTRY_TRACE_FILE_PARAMETER
Language=English
TraceFileName parameter not located in registry;
Default trace file used is %1.
.

MessageId=+1 Severity=Warning SymbolicName=SNMPELEA_NO_REGISTRY_TRIM_FLAG_PARAMETER
Language=English
TrimFlag parameter not located in registry;
Default value used is %1.
.

MessageId=+1 Severity=Warning SymbolicName=SNMPELEA_NO_REGISTRY_TRIM_MESSAGE_PARAMETER
Language=English
TrimMessage parameter not located in registry;
Default value used is %1.
.

MessageId=+1 Severity=Warning SymbolicName=SNMPELEA_NO_REGISTRY_TRAP_SIZE_PARAMETER
Language=English
MaxTrapSize parameter not located in registry;
Default value used is %1.
.

MessageId=+1 Severity=Warning SymbolicName=SNMPELEA_NO_REGISTRY_THRESHOLDENABLED_PARAMETER
Language=English
ThresholdEnabled parameter not located in registry;
Default value used is %1.
.

MessageId=+1 Severity=Warning SymbolicName=SNMPELEA_NO_REGISTRY_THRESHOLD_PARAMETER
Language=English
Threshold parameter not located in registry;
Default value used is %1.
.

MessageId=+1 Severity=Warning SymbolicName=SNMPELEA_NO_REGISTRY_THRESHOLDCOUNT_PARAMETER
Language=English
ThresholdCount parameter not located in registry;
Default value used is %1.
.

MessageId=+1 Severity=Warning SymbolicName=SNMPELEA_REGISTRY_LOW_THRESHOLDCOUNT_PARAMETER
Language=English
ThresholdCount parameter is an invalid value in the registry;
Default value used is %1.
.

MessageId=+1 Severity=Warning SymbolicName=SNMPELEA_NO_REGISTRY_THRESHOLDTIME_PARAMETER
Language=English
ThresholdTime parameter not located in registry;
Default value used is %1.
.

MessageId=+1 Severity=Warning SymbolicName=SNMPELEA_REGISTRY_LOW_THRESHOLDTIME_PARAMETER
Language=English
ThresholdTime parameter is an invalid value in the registry;
Default value used is %1.
.

MessageId=+1 Severity=Warning SymbolicName=SNMPELEA_REGISTRY_TRACE_FILE_PARAMETER_TYPE
Language=English
TraceFileName parameter is of the wrong type in the registry;
Default trace file used is %1.
.

MessageId=+1 Severity=Warning SymbolicName=SNMPELEA_REGISTRY_TRACE_LEVEL_PARAMETER_TYPE
Language=English
TraceLevel parameter is of the wrong type in the registry;
Default trace level used is %1.
.

MessageId=+1 Severity=Warning SymbolicName=SNMPELEA_NO_REGISTRY_TRACE_LEVEL_PARAMETER
Language=English
TraceLevel parameter not located in registry;
Default trace level used is %1.
.

MessageId=+1 Severity=Error SymbolicName=SNMPELEA_ERROR_REGISTRY_PARAMETER_ENUMERATE
Language=English
Error reading Parameter key information from the registry.
Return code from RegEnumValue is %1. Index value is %2.
Extension agent terminating.
.

MessageId=+1 Severity=Error SymbolicName=SNMPELEA_ERROR_REGISTRY_LOG_NAME_ENUMERATE
Language=English
Error reading EventLogFiles key information from the registry.
Return code from RegEnumValue is %1. Index value is %2.
Extension agent terminating.
.

MessageId=+1 Severity=Error SymbolicName=SNMPELEA_NO_REGISTRY_BASEOID_PARAMETER
Language=English
No BaseEnterpriseOID parameter found in registry.
Extension agent terminating.
.

MessageId=+1 Severity=Error SymbolicName=SNMPELEA_NO_REGISTRY_SUPVIEW_PARAMETER
Language=English
No SupportedView parameter found in registry.
Extension agent terminating.
.

MessageId=+1 Severity=Error SymbolicName=SNMPELEA_REGISTRY_INIT_ERROR
Language=English
Error processing registry parameters.
Extension agent terminating.
.

MessageId=+1 Severity=Error SymbolicName=SNMPELEA_BASEOID_CONVERT_ERROR
Language=English
Unable to convert BaseEnterpriseOID from string to ASN.1 OID.
Extension agent terminating.
.

MessageId=+1 Severity=Error SymbolicName=SNMPELEA_CANT_CONVERT_ENTERPRISE_OID
Language=English
Unable to convert EnterpriseOID from string to ASN.1 OID.
Trap cannot be sent.
.

MessageId=+1 Severity=Error SymbolicName=SNMPELEA_SUPVIEW_CONVERT_ERROR
Language=English
Unable to convert SupportedView from string to ASN.1 OID.
Extension agent terminating.
.

MessageId=+1 Severity=Informational SymbolicName=SNMPELEA_NO_REGISTRY_PRIM_DLL
Language=English
Unable to locate ParameterMessageFile for %1. RegOpenKeyEx returned %2.
.

MessageId=+1 Severity=Error SymbolicName=SNMPELEA_CANT_LOAD_PRIM_DLL
Language=English
Unable to load PrimaryModule %1. LoadLibraryEx returned %2.
.

MessageId=+1 Severity=Warning SymbolicName=SNMPELEA_SET_VALUE_FAILED
Language=English
Unable to set Threshold key in registry. Processing will continue.
Return code from RegSetValueEx is %1.
.

;
;/////////////////////////////////////////////////////////////////////////
;//
;// Control Messages (2000-2999)
;//
;/////////////////////////////////////////////////////////////////////////
;

MessageId=2000 Severity=Error SymbolicName=SNMPELEA_ERROR_CREATING_STOP_AGENT_EVENT
Language=English
Error creating the stop extension agent event.
Return code from CreateEvent is %1.
.

MessageId=+1 Severity=Error SymbolicName=SNMPELEA_ERROR_CREATING_STOP_LOG_EVENT
Language=English
Error creating the stop log processing routine termination event.
Return code from CreateEvent is %1.
.

MessageId=+1 Severity=Error SymbolicName=SNMPELEA_ERROR_CREATING_EVENT_NOTIFY_EVENT
Language=English
Error creating the log event notification event.
Return code from CreateEvent is %1.
.

MessageId=+1 Severity=Warning SymbolicName=SNMPELEA_ERROR_CREATING_REG_CHANGE_EVENT
Language=English
Error creating the registry key change notify event.
Return code from CreateEvent is %1.
Initialization will continue, but registry changes will not be updated until SNMP is stopped and restarted.
.

MessageId=+1 Severity=Informational SymbolicName=SNMPELEA_REG_NOTIFY_CHANGE_FAILED
Language=English
RegNotifyChangeKeyValue failed with a return code of %1.
Initialization will continue, but registry changes will not be updated until SNMP is stopped and restarted.
.

MessageId=+1 Severity=Error SymbolicName=SNMPELEA_ERROR_CREATING_LOG_THREAD
Language=English
Error creating the log processing routine service thread.
Return code from CreateThread is %1.
.

MessageId=+1 Severity=Warning SymbolicName=SNMPELEA_ERROR_CLOSING_STOP_AGENT_HANDLE
Language=English
Error closing the stop agent event handle %1.
Return code from CloseHandle is %2.
.

MessageId=+1 Severity=Warning SymbolicName=SNMPELEA_ERROR_CLOSING_STOP_LOG_EVENT_HANDLE
Language=English
Error closing the stop log processing routine event handle %1.
Return code from CloseHandle is %2.
.

MessageId=+1 Severity=Warning SymbolicName=SNMPELEA_ERROR_CLOSING_REG_CHANGED_EVENT_HANDLE
Language=English
Error closing the registry key changed event handle %1.
Return code from CloseHandle is %2.
.

MessageId=+1 Severity=Warning SymbolicName=SNMPELEA_ERROR_CLOSING_REG_PARM_KEY
Language=English
Error closing the registry Parameter key handle %1.
Return code from RegCloseKey is %2.
.

MessageId=+1 Severity=Warning SymbolicName=SNMPELEA_ERROR_CLOSING_STOP_LOG_THREAD_HANDLE
Language=English
Error closing the stop log processing routine thread handle %1.
Return code from CloseHandle is %2.
.

MessageId=+1 Severity=Error SymbolicName=SNMPELEA_ERROR_WAIT_AGENT_STOP_EVENT
Language=English
Error waiting for extension agent shutdown request event %1.
Return code from WaitForSingleObject is %2.
Extension agent did not initialize.
.

MessageId=+1 Severity=Error SymbolicName=SNMPELEA_ERROR_SET_AGENT_STOP_EVENT
Language=English
Error setting the extension agent shutdown event %1.
Return code from SetEvent is %2.
.

MessageId=+1 Severity=Error SymbolicName=SNMPELEA_ERROR_SET_LOG_STOP_EVENT
Language=English
Error setting the log processing routine shutdown event %1.
Return code from SetEvent is %2.
.

MessageId=+1 Severity=Error SymbolicName=SNMPELEA_ERROR_TERMINATE_LOG_THREAD
Language=English
Error terminating the log processing routine %1.
Return code from TerminateThread is %2.
.

MessageId=+1 Severity=Warning SymbolicName=SNMPELEA_ERROR_WAIT_LOG_THREAD_STOP
Language=English
Error waiting for the log processing routine thread %1 to terminate.
Return code from WaitForSingleObject is %2.
.

MessageId=+1 Severity=Warning SymbolicName=SNMPELEA_LOG_THREAD_STOP_WAIT_30
Language=English
The log processing routine thread %1 failed to terminate within 30 seconds.
Thread will be terminated.
.

MessageId=+1 Severity=Warning SymbolicName=SNMPELEA_WAIT_LOG_STOP_UNKNOWN_RETURN
Language=English
An unknown value %2 was returned while waiting for log processing routine thread %1 termination.
Thread state is unknown.
.

MessageId=+1 Severity=Informational SymbolicName=SNMPELEA_STARTED
Language=English
SNMP Event Log Extension Agent is starting.
.

MessageId=+1 Severity=Error SymbolicName=SNMPELEA_ABNORMAL_INITIALIZATION
Language=English
SNMP Event Log Extension Agent did not initialize correctly.
.

MessageId=+1 Severity=Informational SymbolicName=SNMPELEA_STOPPED
Language=English
SNMP Event Log Extension Agent has terminated.
.

;
;/////////////////////////////////////////////////////////////////////////
;//
;// Main Routine Messages (3000-3999)
;//
;/////////////////////////////////////////////////////////////////////////
;

MessageId=3000 Severity=Error SymbolicName=SNMPELEA_LOG_HANDLE_INVALID
Language=English
Error positioning to end of log file -- handle is invalid.
Handle specified is %1.
.

MessageId=+1 Severity=Warning SymbolicName=SNMPELEA_ERROR_LOG_END
Language=English
Log file not positioned at end.
.

MessageId=+1 Severity=Error SymbolicName=SNMPELEA_ERROR_LOG_BUFFER_ALLOCATE
Language=English
Error positioning to end of log file -- log buffer allocation failed.
Handle specified is %1.
.

MessageId=+1 Severity=Error SymbolicName=SNMPELEA_ERROR_LOG_GET_OLDEST_RECORD
Language=English
Error positioning to end of log file -- can't get oldest log record.
Handle specified is %1. Return code from GetOldestEventLogRecord is %2.
.

MessageId=+1 Severity=Error SymbolicName=SNMPELEA_ERROR_LOG_GET_NUMBER_RECORD
Language=English
Error positioning to end of log file -- can't get number of log records.
Handle specified is %1. Return code from GetNumberOfEventLogRecords is %2.
.

MessageId=+1 Severity=Error SymbolicName=SNMPELEA_ERROR_LOG_SEEK
Language=English
Error positioning to end of log file -- seek to end of log failed.
Handle specified is %1. Return code from ReadEventLog is %2.
.

MessageId=+1 Severity=Warning SymbolicName=SNMPELEA_ERROR_READ_LOG_EVENT
Language=English
Error reading log event record.
Handle specified is %1. Return code from ReadEventLog is %2.
.

MessageId=+1 Severity=Warning SymbolicName=SNMPELEA_ERROR_OPEN_EVENT_LOG
Language=English
Error opening event log file %1. Log will not be processed.
Return code from OpenEventLog is %2.
.

MessageId=+1 Severity=Warning SymbolicName=SNMPELEA_ERROR_PULSE_STOP_LOG_EVENT
Language=English
Error pulsing event for log processing routine shutdown event %1.
Return code from PulseEvent is %2.
.

MessageId=+1 Severity=Error SymbolicName=SNMPELEA_CANT_ALLOCATE_WAIT_EVENT_ARRAY
Language=English
Insufficient memory available to allocate the wait event array.
.

MessageId=+1 Severity=Error SymbolicName=SNMPELEA_REALLOC_LOG_EVENT_ARRAY
Language=English
Insufficient memory available to reallocate the log event array.
.

MessageId=+1 Severity=Error SymbolicName=SNMPELEA_ALLOC_EVENT
Language=English
Insufficient memory available to allocate dynamic variable.
.

MessageId=+1 Severity=Error SymbolicName=SNMPELEA_REALLOC_LOG_NAME_ARRAY
Language=English
Insufficient memory available to reallocate the log name array.
.

MessageId=+1 Severity=Error SymbolicName=SNMPELEA_REALLOC_PRIM_HANDLE_ARRAY
Language=English
Insufficient memory available to reallocate the PrimaryModule handle array.
.

MessageId=+1 Severity=Error SymbolicName=SNMPELEA_REALLOC_INSERTION_STRINGS_FAILED
Language=English
Insufficient memory available to reallocate the insertion strings for secondary parameters.
Further secondary substitution has been terminated. Message will be formatted with strings as is.
.

MessageId=+1 Severity=Informational SymbolicName=SNMPELEA_THRESHOLD_REACHED
Language=English
Performance threshold values have been reached.
Trap processing is being quiesed. Further traps will not be sent without operator intervention.
.

MessageId=+1 Severity=Informational SymbolicName=SNMPELEA_THRESHOLD_RESUMED
Language=English
Performance threshold values have been restored by operator intervention.
Trap processing has been resumed.
.

MessageId=+1 Severity=Informational SymbolicName=SNMPELEA_THRESHOLD_SET
Language=English
Performance threshold values have been set by operator intervention.
Current settings indicate performance thresholds have been reached.
Trap processing is being quiesed. Further traps will not be sent without operator intervention.
.

;
;/////////////////////////////////////////////////////////////////////////
;//
;// Log Processing Routine Messages (4000-4999)
;//
;/////////////////////////////////////////////////////////////////////////
;

MessageId=4000 Severity=Warning SymbolicName=SNMPELEA_TRAP_TOO_BIG
Language=English
The trap size of the requested event log exceeds the maximum length of an SNMP trap.
.

MessageId=+1 Severity=Error SymbolicName=SNMPELEA_ERROR_CREATE_LOG_NOTIFY_EVENT
Language=English
Error creating event for log event notification.
Return code from CreateEvent is %1.
.

MessageId=+1 Severity=Error SymbolicName=SNMPELEA_ERROR_LOG_NOTIFY
Language=English
Error requesting notification of change in log event %1.
Return code from ElfChangeNotify is %2.
.

MessageId=+1 Severity=Error SymbolicName=SNMPELEA_ERROR_WAIT_ARRAY
Language=English
Error waiting on event array in log processing event routine.
Return code from WaitForMultipleObjects is %1.
Extension agent is terminating.
.

MessageId=+1 Severity=Error SymbolicName=SNMPELEA_ERROR_CLOSE_WAIT_EVENT_HANDLE
Language=English
Error closing an event log wait event handle.
Event handle is %1. Return code from CloseHandle is %2.
.

MessageId=+1 Severity=Warning SymbolicName=SNMPELEA_ERROR_LOG_BUFFER_ALLOCATE_BAD
Language=English
Error allocating memory for event log buffer.
Trap will not be sent.
.

;
;/////////////////////////////////////////////////////////////////////////
;//
;// Format Routine Messages (5000-5999)
;//
;/////////////////////////////////////////////////////////////////////////
;

MessageId=5000 Severity=Warning SymbolicName=SNMPELEA_CANT_ALLOCATE_TRAP_BUFFER
Language=English
No memory was available to allocate the buffer for the trap to be processed.
Trap will not be sent.
.

MessageId=+1 Severity=Warning SymbolicName=SNMPELEA_CANT_ALLOCATE_EVENT_TYPE_STORAGE
Language=English
No memory was available to allocate the storage for the event type string.
Trap will not be sent.
.

MessageId=+1 Severity=Warning SymbolicName=SNMPELEA_CANT_ALLOCATE_EVENT_CATEGORY_STORAGE
Language=English
No memory was available to allocate the storage for the event category string.
Trap will not be sent.
.

MessageId=+1 Severity=Warning SymbolicName=SNMPELEA_CANT_ALLOCATE_COMPUTER_NAME_STORAGE
Language=English
No memory was available to allocate the storage for the computer name string.
Trap will not be sent.
.

MessageId=+1 Severity=Warning SymbolicName=SNMPELEA_CANT_ALLOCATE_VARBIND_ENTRY_STORAGE
Language=English
No memory was available to allocate the buffer for the varbind queue entry structure.
Trap will not be sent.
.

MessageId=+1 Severity=Warning SymbolicName=SNMPELEA_CANT_ALLOC_VARBIND_LIST_STORAGE
Language=English
No memory was available to allocate the buffer for the varbind list.
Trap will not be sent.
.

MessageId=+1 Severity=Warning SymbolicName=SNMPELEA_CANT_ALLOC_ENTERPRISE_OID
Language=English
No memory was available to allocate the buffer for the enterprise OID.
Trap will not be sent.
.

MessageId=+1 Severity=Warning SymbolicName=SNMPELEA_CANT_ALLOCATE_OID_ARRAY
Language=English
No memory was available to allocate the integer array for the enterprise OID.
Trap will not be sent.
.

MessageId=+1 Severity=Warning SymbolicName=SNMPELEA_NON_NUMERIC_OID
Language=English
An attempt was made to convert a string to an OID, but the string contained non-numeric values.
The OID cannot be created. Trap will not be sent.
.

MessageId=+1 Severity=Informational SymbolicName=SNMPELEA_TIME_CONVERT_FAILED
Language=English
The event log time could not be converted from UCT to local time.
GetTimeZoneInformation returned a value of %1. UCT time will be used in this trap.
.

MessageId=+1 Severity=Warning SymbolicName=SNMPELEA_CANT_OPEN_REGISTRY_MSG_DLL
Language=English
Unable to open the registry key for event source for %1. RegOpenKeyEx returned %2.
Trap will not be sent.
.

MessageId=+1 Severity=Warning SymbolicName=SNMPELEA_CANT_OPEN_REGISTRY_PARM_DLL
Language=English
Unable to open the registry key for event source for %1. RegOpenKeyEx returned %2.
Trap will not be sent.
.

MessageId=+1 Severity=Warning SymbolicName=SNMPELEA_NO_REGISTRY_MSG_DLL
Language=English
Unable to locate EventMessageFile for event source for %1. RegOpenKeyEx returned %2.
Trap will not be sent.
.

MessageId=+1 Severity=Informational SymbolicName=SNMPELEA_NO_REGISTRY_PARM_DLL
Language=English
Unable to locate ParameterMessageFile for event source for %1. RegOpenKeyEx returned %2.
.

MessageId=+1 Severity=Warning SymbolicName=SNMPELEA_CANT_EXPAND_MSG_DLL
Language=English
Unable to expand file name in EventMessageFile for %1. Size needed is %2.
Trap will not be sent.
.

MessageId=+1 Severity=Informational SymbolicName=SNMPELEA_CANT_EXPAND_PARM_DLL
Language=English
Unable to expand file name in ParameterMessageFile for %1. Size needed is %2.
.

MessageId=+1 Severity=Warning SymbolicName=SNMPELEA_CANT_LOAD_MSG_DLL
Language=English
Unable to load file name in EventMessageFile for %1. LoadLibraryEx returned %2.
Trap will not be sent.
.

MessageId=+1 Severity=Informational SymbolicName=SNMPELEA_CANT_LOAD_PARM_DLL
Language=English
Unable to load file name in ParameterMessageFile for %1. LoadLibraryEx returned %2.
.

MessageId=+1 Severity=Warning SymbolicName=SNMPELEA_CANT_FORMAT_MSG
Language=English
Unable to format message %1. FormatMessage returned %2.
Trap will not be sent.
.

MessageId=+1 Severity=Informational SymbolicName=SNMPELEA_ERROR_FREEING_MSG_DLL
Language=English
An error occurred freeing message DLL. FreeLibrary returned %1.
.

MessageId=+1 Severity=Informational SymbolicName=SNMPELEA_SID_UNKNOWN
Language=English
Account name could not be located for this event. Unknown will be used.
LookupAccountSid returned %1.
.

MessageId=+1 Severity=Informational SymbolicName=SNMPELEA_FREE_LOCAL_FAILED
Language=English
Local storage for buffer could not be freed. LocalFree returned %1. Potential memory leak.
.

MessageId=+1 Severity=Warning SymbolicName=SNMPELEA_CANT_POST_NOTIFY_EVENT
Language=English
Unable to post event completion to handle %1. SetEvent returned %2.
Trap may not be sent.
.

MessageId=+1 Severity=Informational SymbolicName=SNMPELEA_PARM_NOT_FOUND
Language=English
ParameterMessageFile did not contain a substitution string for %1. Error code was %2.
.

MessageId=+1 Severity=Informational SymbolicName=SNMPELEA_PRIM_NOT_FOUND
Language=English
PrimaryModule did not contain a substitution string for %1. Error code was %2.
This secondary parameter will be removed.
.

;
;/////////////////////////////////////////////////////////////////////////
;//
;// Format Routine Messages (6000-6999)
;//
;/////////////////////////////////////////////////////////////////////////
;

MessageId=6000 Severity=Warning SymbolicName=SNMPELEA_CREATE_MUTEX_ERROR
Language=English
OpenMutex failed for object %1, reason code %2.
Trap will not be sent.
.

MessageId=+1 Severity=Warning SymbolicName=SNMPELEA_MUTEX_ABANDONED
Language=English
Mutex object has been abandoned.
Trap will not be sent.
.

MessageId=+1 Severity=Warning SymbolicName=SNMPELEA_RELEASE_MUTEX_ERROR
Language=English
Mutex object could not be released. Reason code is %1.
Trap may not be sent.
.

MessageId=+1 Severity=Warning SymbolicName=SNMPELEA_ERROR_WAIT_UNKNOWN
Language=English
WaitForMultipleObjects returned an unknown error condition.
Trap will not be sent.
.

MessageId=+1 Severity=Warning SymbolicName=SNMPELEA_GET_EXIT_CODE_THREAD_FAILED
Language=English
GetExitCodeThread returned FALSE, reason code is %1.
.

MessageId=+1 Severity=Warning SymbolicName=SNMPELEA_COUNT_TABLE_ALLOC_ERROR
Language=English
Error allocating storage for Count/Time table entry.
Trap will not be sent.
.

MessageId=+1 Severity=Warning SymbolicName=SNMPELEA_ERROR_ALLOC_VAR_BIND
Language=English
Error allocating storage for variable bindings.
Trap will not be sent.
.

MessageId=+1 Severity=Warning SymbolicName=SNMPELEA_INSERTION_STRING_ARRAY_ALLOC_FAILED
Language=English
Error allocating storage for message DLL insertion string array.
Trap will not be sent.
.

MessageId=+1 Severity=Warning SymbolicName=SNMPELEA_INSERTION_STRING_LENGTH_ARRAY_ALLOC_FAILED
Language=English
Error allocating storage for message DLL insertion string length array.
Trap will not be sent.
.

MessageId=+1 Severity=Warning SymbolicName=SNMPELEA_INSERTION_STRING_ALLOC_FAILED
Language=English
Error allocating storage for message DLL insertion strings.
Trap will not be sent.
.

MessageId=+1 Severity=Informational SymbolicName=SNMPELEA_TOO_MANY_INSERTION_STRINGS
Language=English
Too many insertion strings. First %1 used.
.

MessageId=+1 Severity=Warning SymbolicName=SNMPELEA_LOG_EVENT_IGNORED
Language=English
Cannot specify SNMP Event Log Extension Agent DLL events as trap generators.
This trap is ignored.
.

MessageId=+1 Severity=Warning SymbolicName=SNMPELEA_CANT_COPY_VARBIND
Language=English
Unable to copy varbind entry during trap buffer trimming.
Trap will not be sent.
.

MessageId=+1 Severity=Warning SymbolicName=SNMPELEA_TRIM_TRAP_FAILURE
Language=English
Attempt to reduce trap buffer size failed.
Trap will not be sent.
.

MessageId=+1 Severity=Warning SymbolicName=SNMPELEA_TRIM_FAILED
Language=English
The amount of data required to be trimmed is larger than the entire trap.
Trap will not be sent.
.

;
;#endif // SNMPELMSG
;
