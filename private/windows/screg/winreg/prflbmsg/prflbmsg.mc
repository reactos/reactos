;/*++ BUILD Version: 0001    // Increment this if a change has global effects
;
;Copyright (c) 1992-1994  Microsoft Corporation
;
;Module Name:
;
;    perflib.h
;       (generated from perflib.mc)
;
;Abstract:
;
;   Event message definitions used by routines in Perflib
;
;Created:
;
;    31-Oct-95 Bob Watson
;
;Revision History:
;
;--*/
;#ifndef _PRFLBMSG_H_
;#define _PRFLBMSG_H_
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

MessageId=1000
Severity=Error
Facility=Application
SymbolicName=PERFLIB_ACCESS_DENIED
Language=English
Access to performance data was denied to %1!s! as attempted from 
%2!s!
.
MessageId=+1
Severity=Error
Facility=Application
SymbolicName=PERFLIB_BUFFER_OVERFLOW
Language=English
The buffer size returned by a collect procedure in Extensible Counter 
DLL "%1!s!" for the "%2!s!" service was larger than the space 
available. Performance data returned by counter DLL will be not be 
returned in Perf Data Block. Overflow size is data DWORD 0.
.
MessageId=+1
Severity=Error
Facility=Application
SymbolicName=PERFLIB_GUARD_PAGE_VIOLATION
Language=English
A Guard Page was modified by a collect procedure in Extensible 
Counter DLL "%1!s!" for the "%2!s!" service. Performance data 
returned by counter DLL will be not be returned in Perf Data Block. 
.
MessageId=+1
Severity=Error
Facility=Application
SymbolicName=PERFLIB_INCORRECT_OBJECT_LENGTH
Language=English
The object length of an object returned by Extensible Counter DLL 
"%1!s!" for the "%2!s!" service was not correct. The sum of the 
object lengths returned did not match the size of the buffer 
returned.  Performance data returned by counter DLL will be not be 
returned in Perf Data Block. Count of objects returned is data 
DWORD 0.
.
MessageId=+1
Severity=Error
Facility=Application
SymbolicName=PERFLIB_INCORRECT_INSTANCE_LENGTH
Language=English
The instance length of an object returned by Extensible Counter 
DLL "%1!s!" for the "%2!s!" service was incorrect. The sum of the 
instance lengths plus the object definition structures did not match 
the size of the object. Performance data returned by counter DLL will 
be not be returned in Perf Data Block. The object title index of the 
bad object is data DWORD 0.
.
MessageId=+1
Severity=Error
Facility=Application
SymbolicName=PERFLIB_OPEN_PROC_NOT_FOUND
Language=English
Unable to locate the open procedure "%1!s!" in DLL "%2!s!" for 
the "%3!s!" service. Performance data for this service will not be
available. Error Status is data DWORD 0.
.
MessageId=+1
Severity=Error
Facility=Application
SymbolicName=PERFLIB_COLLECT_PROC_NOT_FOUND
Language=English
Unable to locate the collect procedure "%1!s!" in DLL "%2!s!" for the
"%3!s!" service. Performance data for this service will not be
available. Error Status is data DWORD 0.
.
MessageId=+1
Severity=Error
Facility=Application
SymbolicName=PERFLIB_CLOSE_PROC_NOT_FOUND
Language=English
Unable to locate the close procedure "%1!s!" in DLL "%2!s!" for the
"%3!s!" service. Performance data for this service will not be
available. Error Status is data DWORD 0.
.
MessageId=+1
Severity=Error
Facility=Application
SymbolicName=PERFLIB_OPEN_PROC_FAILURE
Language=English
The Open Procedure for service "%1!s!" in DLL "%2!s!" failed. 
Performance data for this service will not be available. Status code 
returned is data DWORD 0.
.
MessageId=+1
Severity=Error
Facility=Application
SymbolicName=PERFLIB_OPEN_PROC_EXCEPTION
Language=English
The Open Procedure for service "%1!s!" in DLL "%2!s!" generated an 
exception. Performance data for this service will not be available. 
Exception code returned is data DWORD 0.
.
MessageId=+1
Severity=Error
Facility=Application
SymbolicName=PERFLIB_COLLECT_PROC_EXCEPTION
Language=English
The Collect Procedure for the "%1!s!" service in DLL "%2!s!" generated an 
exception or returned an invalid status. Performance data returned by 
counter DLL will be not be returned in Perf Data Block. Exception or 
status code returned is data DWORD 0.
.
MessageId=+1
Severity=Error
Facility=Application
SymbolicName=PERFLIB_LIBRARY_NOT_FOUND
Language=English
The library file "%2!s!" specified for the "%1!s!" service could not 
be opened. Performance data for this service will not be available. 
Status code is data DWORD 0.
.
MessageId=+1
Severity=Error
Facility=Application
SymbolicName=PERFLIB_NEGATIVE_IDLE_TIME
Language=English
The system reported an idle process time that was less than the last
time reported. The data shows the current time and the last reported 
time for the system's idle process.
.
MessageId=+1
Severity=Error
Facility=Application
SymbolicName=PERFLIB_HEAP_ERROR
Language=English
The collect procedure in Extensible Counter DLL "%1!s!" for the "%2!s!" 
service returned a buffer that was larger than the space allocated and 
may have corrupted the application's heap. This DLL should be disabled 
or removed from the system until the problem has been corrected to 
prevent further corruption. The application accessing this performance 
data should be restarted.  The Performance data returned by counter 
DLL will be not be returned in Perf Data Block. Overflow size is 
data DWORD 0.
.
MessageId=+1
Severity=Error
Facility=Application
SymbolicName=PERFLIB_SERVER_OBJECT_ERROR
Language=English
An error occurred while trying to collect data from the Server Object.
The Error code returned by the function is DWORD 0. 
The Status returned in the IO Status Block is DWORD 1.
The Information field of the IO Status Block is DWORD 2.
.
MessageId=+1
Severity=Error
Facility=Application
SymbolicName=PERFLIB_COLLECTION_HUNG
Language=English
The timeout waiting for the performance data collection function "%1!s!"
in the "%2!s!" Library to finish has expired. There may be a problem with 
this extensible counter or the service it is collecting data from or the 
system may have been very busy when this call was attempted. 
.
MessageId=+1
Severity=Warning
Facility=Application
SymbolicName=PERFLIB_BUFFER_ALIGNMENT_ERROR
Language=English
The data buffer created for the "%1!s!" service in the "%2!s!" library is not
aligned on an 8-byte boundary. This may cause problems for applications that are 
trying to read the performance data buffer. Contact the manufacturer of this 
library or service to have this problem corrected or to get a newer version 
of this library.
.
MessageId=+1
Severity=Error
Facility=Application
SymbolicName=PERFLIB_LIBRARY_DISABLED
Language=English
Performance counter data collection from the "%1!s!" service has been disabled 
due to one or more errors generated by the performance counter library for that 
service. The error(s) that forced this action have been written to the application 
event log. The error(s) should be corrected before the performance counters
for this service are enabled again. 
.
MessageId=+1
Severity=Error
Facility=Application
SymbolicName=PERFLIB_LIBRARY_TEMP_DISABLED
Language=English
Performance counter data collection from the "%1!s!" service has been disabled 
for this session due to one or more errors generated by the performance counter 
library for that service. The error(s) that forced this action have been written 
to the application event log. 
.
MessageId=+1
Severity=Error
Facility=Application
SymbolicName=PERFLIB_INVALID_DEFINITION_BLOCK
Language=English
A definition field in an object returned by Extensible Counter 
DLL "%1!s!" for the "%2!s!" service was incorrect. The sum of the 
definitions block lengths in the object definition structures did not 
match the size specified in the object definition header. Performance data 
returned by this counter DLL will be not be returned in Perf Data Block. 
The object title index of the bad object is data DWORD 0.
.
MessageId=+1
Severity=Error
Facility=Application
SymbolicName=PERFLIB_INVALID_SIZE_RETURNED
Language=English
The size of the buffer used is greater than that passed to the collect
function of the "%1!s!" Extensible Counter DLL for the "%2!s!" service.
The size of the buffer passed in is data DWORD 0 and the size returned
is data DWORD 1.
.
MessageId=2000
Severity=Warning
Facility=Application
SymbolicName=PERFLIB_BUFFER_POINTER_MISMATCH
Language=English
The pointer returned did not match the buffer length returned by the
Collect procedure for the "%1!s!" service in Extensible Counter DLL 
"%2!s!". The Length will be adjusted to match and the performance 
data will appear in the Perf Data Block. The returned length is data 
DWORD 0, the new length is data DWORD 1.
.
MessageId=+1
Severity=Warning
Facility=Application
SymbolicName=PERFLIB_NO_PERFORMANCE_SUBKEY
Language=English
The "%1!s!" service does not have a Performance subkey or the key 
could not be opened. No performance counters will be collected for 
this service. The Win32 error code is returned in the data.
.
MessageId=+1
Severity=Warning
Facility=Application
SymbolicName=PERFLIB_OPEN_PROC_TIMEOUT
Language=English
The open procedure for service "%1!s!" in DLL "%2!s!" has taken longer than
the established wait time to complete. There may be a problem with 
this extensible counter or the service it is collecting data from or the 
system may have been very busy when this call was attempted.  
.
MessageId=+1
Severity=Warning
Facility=Application
SymbolicName=PERFLIB_NOT_TRUSTED_FILE
Language=English
The configuration information of the performance library "%1!s!" for the 
"%2!s!" service does not match the trusted performance library information 
stored in the registry. The functions in this library will not be treated 
as trusted.
.
;
MessageId=3000
Severity=Informational
Facility=Application
SymbolicName=PERFLIB_OPEN_PROC_SUCCESS
Language=English
Open procedure for service "%1!s!" in DLL "%2!s!" was called and 
returned successfully.
.
MessageId=+1
Severity=Informational
Facility=Application
SymbolicName=PERFLIB_COUNTERS_ENABLED
Language=English
An updated performance counter library file has been detected. The performance 
counters for the "%1!s!" have been enabled.
.
;
;#endif //_PRFLBMSG_H_

