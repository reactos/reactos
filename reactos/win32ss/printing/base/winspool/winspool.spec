@ stub AbortPrinter
@ stub AddFormA
@ stub AddFormW
@ stub AddJobA
@ stub AddJobW
@ stub AddMonitorA
@ stub AddMonitorW
@ stub AddPortA
@ stub AddPortExA
@ stub AddPortExW
@ stub AddPortW
@ stub AddPrinterA
@ stub AddPrinterConnectionA
@ stub AddPrinterConnectionW
@ stub AddPrinterDriverA
@ stub AddPrinterDriverExA
@ stub AddPrinterDriverExW
@ stub AddPrinterDriverW
@ stub AddPrinterW
@ stub AddPrintProcessorA
@ stub AddPrintProcessorW
@ stub AddPrintProvidorA
@ stub AddPrintProvidorW
@ stub AdvancedDocumentPropertiesA
@ stub AdvancedDocumentPropertiesW
@ stub ADVANCEDSETUPDIALOG
@ stub AdvancedSetupDialog
@ stdcall ClosePrinter(long)
@ stub CloseSpoolFileHandle
@ stub CommitSpoolData
@ stub ConfigurePortA
@ stub ConfigurePortW
@ stub ConnectToPrinterDlg
@ stub ConvertAnsiDevModeToUnicodeDevmode
@ stub ConvertUnicodeDevModeToAnsiDevmode
@ stub CreatePrinterIC
@ stub DeleteFormA
@ stub DeleteFormW
@ stub DeleteMonitorA
@ stub DeleteMonitorW
@ stub DeletePortA
@ stub DeletePortW
@ stub DeletePrinter
@ stub DeletePrinterConnectionA
@ stub DeletePrinterConnectionW
@ stub DeletePrinterDataA
@ stub DeletePrinterDataExA
@ stub DeletePrinterDataExW
@ stub DeletePrinterDataW
@ stub DeletePrinterDriverA
@ stub DeletePrinterDriverExA
@ stub DeletePrinterDriverExW
@ stub DeletePrinterDriverW
@ stub DeletePrinterIC
@ stub DeletePrinterKeyA
@ stub DeletePrinterKeyW
@ stub DeletePrintProcessorA
@ stub DeletePrintProcessorW
@ stub DeletePrintProvidorA
@ stub DeletePrintProvidorW
@ stub DEVICECAPABILITIES
@ stub DeviceCapabilities
@ stdcall DeviceCapabilitiesA(str str long ptr ptr)
@ stdcall DeviceCapabilitiesW(wstr wstr long ptr ptr)
@ stub DEVICEMODE
@ stub DeviceMode
@ stub DevicePropertySheets
@ stub DevQueryPrint
@ stub DevQueryPrintEx
@ stub DocumentEvent
@ stdcall DocumentPropertiesA(long long ptr ptr ptr long)
@ stdcall DocumentPropertiesW(long long ptr ptr ptr long)
@ stub DocumentPropertySheets
@ stdcall EndDocPrinter(long)
@ stdcall EndPagePrinter(long)
@ stub EnumFormsA
@ stub EnumFormsW
@ stub EnumJobsA
@ stub EnumJobsW
@ stub EnumMonitorsA
@ stub EnumMonitorsW
@ stub EnumPortsA
@ stub EnumPortsW
@ stub EnumPrinterDataA
@ stub EnumPrinterDataExA
@ stub EnumPrinterDataExW
@ stub EnumPrinterDataW
@ stub EnumPrinterDriversA
@ stub EnumPrinterDriversW
@ stub EnumPrinterKeyA
@ stub EnumPrinterKeyW
@ stdcall EnumPrintersA(long ptr long ptr long ptr ptr)
@ stdcall EnumPrintersW(long ptr long ptr long ptr ptr)
@ stdcall EnumPrintProcessorDatatypesA(ptr ptr long ptr long ptr ptr)
@ stdcall EnumPrintProcessorDatatypesW(ptr ptr long ptr long ptr ptr)
@ stub EnumPrintProcessorsA
@ stub EnumPrintProcessorsW
@ stub EXTDEVICEMODE
@ stub ExtDeviceMode
@ stub FindClosePrinterChangeNotification
@ stub FindFirstPrinterChangeNotification
@ stub FindNextPrinterChangeNotification
@ stub FlushPrinter
@ stub FreePrinterNotifyInfo
@ stdcall GetDefaultPrinterA(ptr ptr)
@ stdcall GetDefaultPrinterW(ptr ptr)
@ stub GetFormA
@ stub GetFormW
@ stub GetJobA
@ stub GetJobW
@ stdcall GetPrinterA(long long ptr long ptr)
@ stub GetPrinterDataA
@ stub GetPrinterDataExA
@ stub GetPrinterDataExW
@ stub GetPrinterDataW
@ stdcall GetPrinterDriverA(long str long ptr long ptr)
@ stub GetPrinterDriverDirectoryA
@ stub GetPrinterDriverDirectoryW
@ stdcall GetPrinterDriverW(long wstr long ptr long ptr)
@ stdcall GetPrinterW(long long ptr long ptr)
@ stub GetPrintProcessorDirectoryA
@ stdcall GetPrintProcessorDirectoryW(wstr wstr long ptr long ptr)
@ stub GetSpoolFileHandle
@ stub IsValidDevmodeA
@ stub IsValidDevmodeW
@ stdcall OpenPrinterA(str ptr ptr)
@ stdcall OpenPrinterW(wstr ptr ptr)
@ stub PerfClose
@ stub PerfCollect
@ stub PerfOpen
@ stub PlayGdiScriptOnPrinterIC
@ stub PrinterMessageBoxA
@ stub PrinterMessageBoxW
@ stub PrinterProperties
@ stub QueryColorProfile
@ stub QueryRemoteFonts
@ stub QuerySpoolMode
@ stub ReadPrinter
@ stub ResetPrinterA
@ stub ResetPrinterW
@ stub ScheduleJob
@ stub SeekPrinter
@ stub SetAllocFailCount
@ stub SetDefaultPrinterA
@ stub SetDefaultPrinterW
@ stub SetFormA
@ stub SetFormW
@ stub SetJobA
@ stub SetJobW
@ stub SetPortA
@ stub SetPortW
@ stub SetPrinterA
@ stub SetPrinterDataA
@ stub SetPrinterDataExA
@ stub SetPrinterDataExW
@ stub SetPrinterDataW
@ stub SetPrinterW
@ stub SplDriverUnloadComplete
@ stub SpoolerDevQueryPrintW
@ stdcall SpoolerInit()
@ stub SpoolerPrinterEvent
@ stub StartDocDlgA
@ stub StartDocDlgW
@ stub StartDocPrinterA
@ stdcall StartDocPrinterW(long long ptr)
@ stdcall StartPagePrinter(long)
@ stub WaitForPrinterChange
@ stdcall WritePrinter(long ptr long ptr)
@ stdcall XcvDataW(long wstr ptr long ptr long ptr ptr)
