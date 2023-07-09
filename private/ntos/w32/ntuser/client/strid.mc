;
;// Message definition for Error Message Instrument 
;

SeverityNames=(Success=0x0:STATUS_SEVERITY_SUCCESS
               Informational=0x1:STATUS_SEVERITY_INFORMATIONAL
               Warning=0x2:STATUS_SEVERITY_WARNING
               Error=0x3:STATUS_SEVERITY_ERROR
              )

FacilityNames=(System=0x0
               RpcRuntime=0x2:FACILITY_RPC_RUNTIME
               RpcStubs=0x3:FACILITY_RPC_STUBS
               Io=0x4:FACILITY_IO_ERROR_CODE
               TerminalServer=0xA:FACILITY_TERMINAL_SERVER
               USB=0x10:FACILITY_USB_ERROR_CODE
               HID=0x11:FACILITY_HID_ERROR_CODE
               FIREWIRE=0x12:FACILITY_FIREWIRE_ERROR_CODE
               Cluster=0x13:FACILITY_CLUSTER_ERROR_CODE
               ACPI=0x14:FACILITY_ACPI_ERROR_CODE
              )


MessageId=0x0430 Facility=System Severity=Informational SymbolicName=STATUS_LOG_ERROR_MSG
Language=English
Error Instrument: ProcessName: %1  WindowTitle: %2  MsgCaption: %3  MsgText: %4  CallerModuleName: %5  BaseAddr: %6  ImageSize: %7   ReturnAddr: %8
.


