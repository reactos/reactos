;/*++ BUILD Version: 0001    // Increment this if a change has global effects
;
;Copyright (c) 1993  Microsoft Corporation
;
;Module Name:
;
;    fsvgalog.mc
;
;Abstract:
;
;    Constant definitions for the I/O error code log values.
;
;Revision History:
;
;--*/
;
;#ifndef _FSVGALOG_
;#define _FSVGALOG_
;
;//
;//  Status values are 32 bit values layed out as follows:
;//
;//   3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1
;//   1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
;//  +---+-+-------------------------+-------------------------------+
;//  |Sev|C|       Facility          |               Code            |
;//  +---+-+-------------------------+-------------------------------+
;//
;//  where
;//
;//      Sev - is the severity code
;//
;//          00 - Success
;//          01 - Informational
;//          10 - Warning
;//          11 - Error
;//
;//      C - is the Customer code flag
;//
;//      Facility - is the facility code
;//
;//      Code - is the facility's status code
;//
;
MessageIdTypedef=NTSTATUS

SeverityNames=(Success=0x0:STATUS_SEVERITY_SUCCESS
               Informational=0x1:STATUS_SEVERITY_INFORMATIONAL
               Warning=0x2:STATUS_SEVERITY_WARNING
               Error=0x3:STATUS_SEVERITY_ERROR
              )

FacilityNames=(System=0x0
               RpcRuntime=0x2:FACILITY_RPC_RUNTIME
               RpcStubs=0x3:FACILITY_RPC_STUBS
               Io=0x4:FACILITY_IO_ERROR_CODE
               fsvga=0x5:FACILITY_FSVGA_ERROR_CODE
              )



MessageId=0x0001 Facility=fsvga Severity=Error SymbolicName=FSVGA_INSUFFICIENT_RESOURCES
Language=English
Not enough memory was available to allocate internal storage needed for the device %1.
.

MessageId=0x0003 Facility=fsvga Severity=Error SymbolicName=FSVGA_REGISTERS_NOT_MAPPED
Language=English
The hardware locations for %1 could not be translated to something the memory management system understands.
.

MessageId=0x0004 Facility=fsvga Severity=Error SymbolicName=FSVGA_RESOURCE_CONFLICT
Language=English
The hardware resources for %1 are already in use by another device.
.

MessageId=0x0007 Facility=fsvga Severity=Warning SymbolicName=FSVGA_NO_DEVICEMAP_CREATED
Language=English
Unable to create the device map entry for %1.
.

MessageId=0x0021 Facility=fsvga Severity=Warning SymbolicName=FSVGA_NO_SUCH_DEVICE
Language=English
The Full Screen Video devices do not exist or were not detected.
.

;#endif /* _FSVGALOG_ */
