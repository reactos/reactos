100 stub -noname EnumPrinterPropertySheets
101 stub -noname ClusterSplOpen
102 stub -noname ClusterSplClose
103 stub -noname ClusterSplIsAlive
104 stub PerfClose
105 stub PerfCollect
106 stub PerfOpen
201 stdcall GetDefaultPrinterA(ptr ptr)
202 stdcall SetDefaultPrinterA(str)
203 stdcall GetDefaultPrinterW(ptr ptr)
204 stdcall SetDefaultPrinterW(wstr)
205 stub -noname SplReadPrinter
206 stub -noname AddPerMachineConnectionA
207 stub -noname AddPerMachineConnectionW
208 stub -noname DeletePerMachineConnectionA
209 stub -noname DeletePerMachineConnectionW
210 stub -noname EnumPerMachineConnectionsA
211 stub -noname EnumPerMachineConnectionsW
212 stub -noname LoadPrinterDriver
213 stub -noname RefCntLoadDriver
214 stub -noname RefCntUnloadDriver
215 stub -noname ForceUnloadDriver
216 stub -noname PublishPrinterA
217 stub -noname PublishPrinterW
218 stub -noname CallCommonPropertySheetUI
219 stub -noname PrintUIQueueCreate
220 stub -noname PrintUIPrinterPropPages
221 stub -noname PrintUIDocumentDefaults
222 stub -noname SendRecvBidiData
223 stub -noname RouterFreeBidiResponseContainer
224 stub -noname ExternalConnectToLd64In32Server
226 stub -noname PrintUIWebPnpEntry
227 stub -noname PrintUIWebPnpPostEntry
228 stub -noname PrintUICreateInstance
229 stub -noname PrintUIDocumentPropertiesWrap
230 stub -noname PrintUIPrinterSetup
231 stub -noname PrintUIServerPropPages
232 stub -noname AddDriverCatalog

@ stub ADVANCEDSETUPDIALOG
@ stdcall AbortPrinter(long)
@ stdcall AddFormA(long long ptr)
@ stdcall AddFormW(long long ptr)
@ stdcall AddJobA(long long ptr long ptr)
@ stdcall AddJobW(long long ptr long ptr)
@ stdcall AddMonitorA(str long ptr)
@ stdcall AddMonitorW(wstr long ptr)
@ stdcall AddPortA(str ptr str)
@ stdcall AddPortExA(str long ptr str)
@ stdcall AddPortExW(wstr long ptr wstr)
@ stdcall AddPortW(wstr long wstr)
@ stdcall AddPrinterA(str long ptr)
@ stdcall AddPrinterConnectionA(str)
@ stdcall AddPrinterConnectionW(wstr)
@ stdcall AddPrinterDriverA(str long ptr)
@ stdcall AddPrinterDriverExA(str long ptr long)
@ stdcall AddPrinterDriverExW(wstr long ptr long)
@ stdcall AddPrinterDriverW(wstr long ptr)
@ stdcall AddPrinterW(wstr long ptr)
@ stdcall AddPrintProcessorA(str str str str)
@ stdcall AddPrintProcessorW(wstr wstr wstr wstr)
@ stdcall AddPrintProvidorA(str long ptr)
@ stdcall AddPrintProvidorW(wstr long ptr)
@ stdcall AdvancedDocumentPropertiesA(long long str ptr ptr)
@ stdcall AdvancedDocumentPropertiesW(long long wstr ptr ptr)
@ stub AdvancedSetupDialog
@ stdcall ClosePrinter(long)
@ stub CloseSpoolFileHandle
@ stdcall ConfigurePortA(str long str)
@ stdcall ConfigurePortW(wstr long wstr)
@ stdcall ConnectToPrinterDlg(long long)
@ stub ConvertAnsiDevModeToUnicodeDevMode
@ stub ConvertUnicodeDevModeToAnsiDevMode
@ stub CommitSpoolData
@ stub CreatePrinterIC
@ stub DEVICECAPABILITIES
@ stub DEVICEMODE
@ stdcall DeleteFormA(long str)
@ stdcall DeleteFormW(long wstr)
@ stdcall DeleteMonitorA(str str str)
@ stdcall DeleteMonitorW(wstr wstr wstr)
@ stdcall DeletePortA(str long str)
@ stdcall DeletePortW(wstr long wstr)
@ stdcall DeletePrinter(long)
@ stdcall DeletePrinterConnectionA(str)
@ stdcall DeletePrinterConnectionW(wstr)
@ stdcall DeletePrinterDataExA(long str str)
@ stdcall DeletePrinterDataExW(long wstr wstr)
@ stdcall DeletePrinterDataA(ptr str)
@ stdcall DeletePrinterDataW(ptr wstr)
@ stdcall DeletePrinterDriverA(str str str)
@ stdcall DeletePrinterDriverExA(str str str long long)
@ stdcall DeletePrinterDriverExW(wstr wstr wstr long long)
@ stdcall DeletePrinterDriverW(wstr wstr wstr)
@ stub DeletePrinterIC
@ stub DevQueryPrint
@ stdcall DeletePrintProcessorA(str str str)
@ stdcall DeletePrintProcessorW(wstr wstr wstr)
@ stdcall DeletePrintProvidorA(str str str)
@ stdcall DeletePrintProvidorW(wstr wstr wstr)
@ stdcall DeviceCapabilitiesA(str str long ptr ptr)
@ stdcall DeviceCapabilitiesW(wstr wstr long ptr ptr)
@ stub DeviceMode
@ stub DocumentEvent
@ stdcall DllMain(ptr long ptr)
@ stdcall DocumentPropertiesA(long long ptr ptr ptr long)
@ stdcall DocumentPropertiesW(long long ptr ptr ptr long)
@ stub EXTDEVICEMODE
@ stdcall EndDocPrinter(long)
@ stdcall EndPagePrinter(long)
@ stdcall EnumFormsA(long long ptr long ptr ptr)
@ stdcall EnumFormsW(long long ptr long ptr ptr)
@ stdcall EnumJobsA(long long long long ptr long ptr ptr)
@ stdcall EnumJobsW(long long long long ptr long ptr ptr)
@ stdcall EnumMonitorsA(str long ptr long long long)
@ stdcall EnumMonitorsW(wstr long ptr long long long)
@ stdcall EnumPortsA(str long ptr ptr ptr ptr)
@ stdcall EnumPortsW(wstr long ptr ptr ptr ptr)
@ stdcall EnumPrinterDataA(long long ptr long ptr ptr ptr long ptr)
@ stdcall EnumPrinterDataExA(long str ptr long ptr ptr)
@ stdcall EnumPrinterDataExW(long wstr ptr long ptr ptr)
@ stdcall EnumPrinterDataW(long long ptr long ptr ptr ptr long ptr)
@ stdcall EnumPrinterDriversA(str str long ptr long ptr ptr)
@ stdcall EnumPrinterDriversW(wstr wstr long ptr long ptr ptr)
@ stdcall EnumPrintersA(long ptr long ptr long ptr ptr)
@ stdcall EnumPrintersW(long ptr long ptr long ptr ptr)
@ stdcall EnumPrinterKeyA(long str str long ptr)
@ stdcall EnumPrinterKeyW(long wstr wstr long ptr)
@ stdcall ExtDeviceMode(long long ptr str str ptr str long)
@ stdcall EnumPrintProcessorDatatypesA(str str long ptr long ptr ptr)
@ stdcall EnumPrintProcessorDatatypesW(wstr wstr long ptr long ptr ptr)
@ stdcall EnumPrintProcessorsA(str str long ptr long ptr ptr)
@ stdcall EnumPrintProcessorsW(wstr wstr long ptr long ptr ptr)
@ stdcall FindClosePrinterChangeNotification(long)
@ stdcall FindFirstPrinterChangeNotification(long long long ptr)
@ stdcall FindNextPrinterChangeNotification(long ptr ptr ptr)
@ stdcall FreePrinterNotifyInfo(ptr)
@ stdcall GetFormA(long str long ptr long ptr)
@ stdcall GetFormW(long wstr long ptr long ptr)
@ stdcall GetJobA(long long long ptr long ptr)
@ stdcall GetJobW(long long long ptr long ptr)
@ stdcall GetPrinterA(long long ptr long ptr)
@ stdcall GetPrinterDataA(long str ptr ptr long ptr)
@ stdcall GetPrinterDataExA(long str str ptr ptr long ptr)
@ stdcall GetPrinterDataExW(long wstr wstr ptr ptr long ptr)
@ stdcall GetPrinterDataW(long wstr ptr ptr long ptr)
@ stdcall GetPrinterDriverA(long str long ptr long ptr)
@ stdcall GetPrinterDriverDirectoryA(str str long ptr long ptr)
@ stdcall GetPrinterDriverDirectoryW(wstr wstr long ptr long ptr)
@ stdcall GetPrinterDriverW(long str long ptr long ptr)
@ stdcall GetPrinterW(long long ptr long ptr)
@ stdcall GetPrintProcessorDirectoryA(str str long ptr long ptr)
@ stdcall GetPrintProcessorDirectoryW(wstr wstr long ptr long ptr)
@ stub GetSpoolFileHandle
@ stub IsValidDevmodeA
@ stdcall -stub IsValidDevmodeW(ptr long)
@ stdcall OpenPrinterA(str ptr ptr)
@ stdcall OpenPrinterW(wstr ptr ptr)
@ stub PlayGdiScriptOnPrinterIC
@ stdcall PrinterMessageBoxA(ptr long ptr str str long)
@ stdcall PrinterMessageBoxW(ptr long ptr wstr wstr long)
@ stdcall PrinterProperties(long long)
@ stub QueryColorProfile
@ stub QuerySpoolMode
@ stub QueryRemoteFonts
@ stdcall ReadPrinter(long ptr long ptr)
@ stdcall ResetPrinterA(long ptr)
@ stdcall ResetPrinterW(long ptr)
@ stdcall ScheduleJob(long long)
@ stub SeekPrinter
@ stub SetAllocFailCount
@ stdcall SetFormA(long str long ptr)
@ stdcall SetFormW(long wstr long ptr)
@ stdcall SetJobA(long long long ptr long)
@ stdcall SetJobW(long long long ptr long)
@ stdcall SetPrinterA(long long ptr long)
@ stdcall SetPrinterDataA(long str long ptr long)
@ stdcall SetPrinterDataExA(long str str long ptr long)
@ stdcall SetPrinterDataExW(long wstr wstr long ptr long)
@ stdcall SetPrinterDataW(long wstr long ptr long)
@ stdcall SetPrinterW(long long ptr long)
@ stub SplDriverUnloadComplete
@ stub SpoolerDevQueryPrintW
@ stdcall SpoolerInit()
@ stub SpoolerPrinterEvent
@ stub StartDocDlgA
@ stub StartDocDlgW
@ stdcall StartDocPrinterA(long long ptr)
@ stdcall StartDocPrinterW(long long ptr)
@ stdcall StartPagePrinter(long)
@ stdcall WaitForPrinterChange(ptr long)
@ stdcall WritePrinter(long ptr long ptr)
@ stdcall XcvDataW(long wstr ptr long ptr long ptr ptr)
