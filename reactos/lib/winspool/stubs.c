/* $Id: stubs.c,v 1.2 2003/07/10 21:48:16 chorns Exp $
 *
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS winspool DRV
 * FILE:        stubs.c
 * PURPOSE:     Stub functions
 * PROGRAMMERS: Ge van Geldorp (ge@gse.nl)
 * REVISIONS:
 */

#include <windows.h>

/*
 * @unimplemented
 */
BOOL
STDCALL
DllMain(HINSTANCE InstDLL,
        DWORD Reason,
        LPVOID Reserved)
{
  return TRUE;
}


/*
 * @unimplemented
 */
BOOL
STDCALL
AbortPrinter(HANDLE Printer)
{
  OutputDebugStringW(L"winspool AbortPrinter stub called\n");
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

  return FALSE;
}


/*
 * @unimplemented
 */
BOOL
STDCALL
AddFormA(HANDLE Printer, DWORD Level, PBYTE Form)
{
  OutputDebugStringW(L"winspool AddFormA stub called\n");
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

  return FALSE;
}


/*
 * @unimplemented
 */
BOOL
STDCALL
AddFormW(HANDLE Printer, DWORD Level, PBYTE Form)
{
  OutputDebugStringW(L"winspool AddFormW stub called\n");
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

  return FALSE;
}


/*
 * @unimplemented
 */
BOOL
STDCALL
AddJobA(HANDLE Printer, DWORD Level, PBYTE Data, DWORD BufSize, PDWORD Needed)
{
  OutputDebugStringW(L"winspool AddJobA stub called\n");
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

  return FALSE;
}


/*
 * @unimplemented
 */
BOOL
STDCALL
AddJobW(HANDLE Printer, DWORD Level, PBYTE Data, DWORD BufSize, PDWORD Needed)
{
  OutputDebugStringW(L"winspool AddJobW stub called\n");
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

  return FALSE;
}


/*
 * @unimplemented
 */
BOOL
STDCALL
AddMonitorA(LPSTR Name, DWORD Level, PBYTE Monitors)
{
  OutputDebugStringW(L"winspool AddMonitorA stub called\n");
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

  return FALSE;
}


/*
 * @unimplemented
 */
BOOL
STDCALL
AddMonitorW(LPWSTR Name, DWORD Level, PBYTE Monitors)
{
  OutputDebugStringW(L"winspool AddMonitorW stub called\n");
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

  return FALSE;
}


/*
 * @unimplemented
 */
BOOL
STDCALL
AddPortA(LPSTR Name, HWND Wnd, LPSTR MonitorName)
{
  OutputDebugStringW(L"winspool  stub called\n");
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

  return FALSE;
}


/*
 * @unimplemented
 */
BOOL
STDCALL
AddPortW(LPWSTR Name, HWND Wnd, LPWSTR MonitorName)
{
  OutputDebugStringW(L"winspool AddPortW stub called\n");
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

  return FALSE;
}


/*
 * @unimplemented
 */
HANDLE
STDCALL
AddPrinterA(LPSTR Name, DWORD Level, PBYTE Buffer)
{
  OutputDebugStringW(L"winspool AddPrinterA stub called\n");
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

  return NULL;
}


/*
 * @unimplemented
 */
HANDLE
STDCALL
AddPrinterW(LPWSTR Name, DWORD Level, PBYTE Buffer)
{
  OutputDebugStringW(L"winspool AddPrinterW stub called\n");
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

  return NULL;
}


/*
 * @unimplemented
 */
BOOL
STDCALL
AddPrinterConnectionA(LPSTR Name)
{
  OutputDebugStringW(L"winspool AddPrinterConnectionA stub called\n");
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

  return FALSE;
}


/*
 * @unimplemented
 */
BOOL
STDCALL
AddPrinterConnectionW(LPWSTR Name)
{
  OutputDebugStringW(L"winspool AddPrinterConnectionW stub called\n");
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

  return FALSE;
}


/*
 * @unimplemented
 */
BOOL
STDCALL
AddPrinterDriverA(LPSTR Name, DWORD Level, PBYTE Buffer)
{
  OutputDebugStringW(L"winspool AddPrinterDriverA stub called\n");
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

  return FALSE;
}


/*
 * @unimplemented
 */
BOOL
STDCALL
AddPrinterDriverW(LPWSTR Name, DWORD Level, PBYTE Buffer)
{
  OutputDebugStringW(L"winspool AddPrinterDriverW stub called\n");
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

  return FALSE;
}


/*
 * @unimplemented
 */
BOOL
STDCALL
AddPrintProcessorA(LPSTR Name, LPSTR Environment, LPSTR PathName, LPSTR PrintProcessorName)
{
  OutputDebugStringW(L"winspool AddPrintProcessorA stub called\n");
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

  return FALSE;
}


/*
 * @unimplemented
 */
BOOL
STDCALL
AddPrintProcessorW(LPWSTR Name, LPWSTR Environment, LPWSTR PathName, LPWSTR PrintProcessorName)
{
  OutputDebugStringW(L"winspool AddPrintProcessorW stub called\n");
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

  return FALSE;
}


/*
 * @unimplemented
 */
BOOL
STDCALL
AddPrintProvidorA(LPSTR Name, DWORD Level, PBYTE Buffer)
{
  OutputDebugStringW(L"winspool AddPrintProvidorA stub called\n");
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

  return FALSE;
}


/*
 * @unimplemented
 */
BOOL
STDCALL
AddPrintProvidorW(LPWSTR Name, DWORD Level, PBYTE Buffer)
{
  OutputDebugStringW(L"winspool AddPrintProvidorW stub called\n");
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

  return FALSE;
}


/*
 * @unimplemented
 */
LONG
STDCALL
AdvancedDocumentPropertiesA(HWND Wnd, HANDLE Printer, LPSTR DeviceName, PDEVMODEA DevModeOut, PDEVMODEA DevModeIn)
{
  OutputDebugStringW(L"winspool AdvancedDocumentPropertiesA stub called\n");
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

  return 0;
}


/*
 * @unimplemented
 */
LONG
STDCALL
AdvancedDocumentPropertiesW(HWND Wnd, HANDLE Printer, LPWSTR DeviceName, PDEVMODEW DevModeOut, PDEVMODEW DevModeIn)
{
  OutputDebugStringW(L"winspool AdvancedDocumentPropertiesW stub called\n");
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

  return 0;
}


/*
 * @unimplemented
 */
BOOL
STDCALL
ClosePrinter(HANDLE Printer)
{
  OutputDebugStringW(L"winspool ClosePrinter stub called\n");
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

  return FALSE;
}


/*
 * @unimplemented
 */
BOOL
STDCALL
ConfigurePortA(LPSTR Name, HWND Wnd, LPSTR PortName)
{
  OutputDebugStringW(L"winspool ConfigurePortA stub called\n");
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

  return FALSE;
}


/*
 * @unimplemented
 */
BOOL
STDCALL
ConfigurePortW(LPWSTR Name, HWND Wnd, LPWSTR PortName)
{
  OutputDebugStringW(L"winspool ConfigurePortW stub called\n");
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

  return FALSE;
}


/*
 * @unimplemented
 */
HANDLE
STDCALL
ConnectToPrinterDlg(HWND Wnd, DWORD Flags)
{
  OutputDebugStringW(L"winspool ConnectToPrinterDlg stub called\n");
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

  return NULL;
}


/*
 * @unimplemented
 */
BOOL
STDCALL
DeleteFormA(HANDLE Printer, LPSTR Name)
{
  OutputDebugStringW(L"winspool DeleteFormA stub called\n");
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

  return FALSE;
}


/*
 * @unimplemented
 */
BOOL
STDCALL
DeleteFormW(HANDLE Printer, LPWSTR Name)
{
  OutputDebugStringW(L"winspool DeleteFormW stub called\n");
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

  return FALSE;
}


/*
 * @unimplemented
 */
BOOL
STDCALL
DeleteMonitorA(LPSTR Name, LPSTR Environment, LPSTR MonitorName)
{
  OutputDebugStringW(L"winspool DeleteMonitorA stub called\n");
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

  return FALSE;
}


/*
 * @unimplemented
 */
BOOL
STDCALL
DeleteMonitorW(LPWSTR Name, LPWSTR Environment, LPWSTR MonitorName)
{
  OutputDebugStringW(L"winspool DeleteMonitorW stub called\n");
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

  return FALSE;
}


/*
 * @unimplemented
 */
BOOL
STDCALL
DeletePortA(LPSTR Name, HWND Wnd, LPSTR PortName)
{
  OutputDebugStringW(L"winspool DeletePortA stub called\n");
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

  return FALSE;
}


/*
 * @unimplemented
 */
BOOL
STDCALL
DeletePortW(LPWSTR Name, HWND Wnd, LPWSTR PortName)
{
  OutputDebugStringW(L"winspool DeletePortW stub called\n");
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

  return FALSE;
}


/*
 * @unimplemented
 */
BOOL
STDCALL
DeletePrinter(HANDLE Printer)
{
  OutputDebugStringW(L"winspool DeletePrinter stub called\n");
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

  return FALSE;
}


/*
 * @unimplemented
 */
BOOL
STDCALL
DeletePrinterConnectionA(LPSTR Name)
{
  OutputDebugStringW(L"winspool DeletePrinterConnectionA stub called\n");
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

  return FALSE;
}


/*
 * @unimplemented
 */
BOOL
STDCALL
DeletePrinterConnectionW(LPWSTR Name)
{
  OutputDebugStringW(L"winspool DeletePrinterConnectionW stub called\n");
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

  return FALSE;
}


/*
 * @unimplemented
 */
DWORD
STDCALL
DeletePrinterDataA(HANDLE Printer, LPSTR Name)
{
  OutputDebugStringW(L"winspool DeletePrinterDataA stub called\n");
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

  return FALSE;
}


/*
 * @unimplemented
 */
DWORD
STDCALL
DeletePrinterDataW(HANDLE Printer, LPWSTR Name)
{
  OutputDebugStringW(L"winspool DeletePrinterDataW stub called\n");
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

  return FALSE;
}


/*
 * @unimplemented
 */
BOOL
STDCALL
DeletePrinterDriverA(LPSTR Name, LPSTR Environment, LPSTR Driver)
{
  OutputDebugStringW(L"winspool DeletePrinterDriverA stub called\n");
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

  return FALSE;
}


/*
 * @unimplemented
 */
BOOL
STDCALL
DeletePrinterDriverW(LPWSTR Name, LPWSTR Environment, LPWSTR Driver)
{
  OutputDebugStringW(L"winspool DeletePrinterDriverW stub called\n");
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

  return FALSE;
}


/*
 * @unimplemented
 */
BOOL
STDCALL
DeletePrintProcessorA(LPSTR Name, LPSTR Environment, LPSTR PrintProcessor)
{
  OutputDebugStringW(L"winspool DeletePrintProcessorA stub called\n");
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

  return FALSE;
}


/*
 * @unimplemented
 */
BOOL
STDCALL
DeletePrintProcessorW(LPWSTR Name, LPWSTR Environment, LPWSTR PrintProcessor)
{
  OutputDebugStringW(L"winspool DeletePrintProcessorW stub called\n");
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

  return FALSE;
}


/*
 * @unimplemented
 */
BOOL
STDCALL
DeletePrintProvidorA(LPSTR Name, LPSTR Environment, LPSTR PrintProvidor)
{
  OutputDebugStringW(L"winspool DeletePrintProvidorA stub called\n");
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

  return FALSE;
}


/*
 * @unimplemented
 */
BOOL
STDCALL
DeletePrintProvidorW(LPWSTR Name, LPWSTR Environment, LPWSTR PrintProvidor)
{
  OutputDebugStringW(L"winspool DeletePrintProvidorW stub called\n");
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

  return FALSE;
}


/*
 * @unimplemented
 */
int
STDCALL
DeviceCapabilitiesA(LPCSTR Device, LPCSTR Port, WORD Capability, LPSTR Buffer, CONST DEVMODEA *DevMode)
{
  OutputDebugStringW(L"winspool DeviceCapabilitiesA stub called\n");
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

  return -1;
}


/*
 * @unimplemented
 */
int
STDCALL
DeviceCapabilitiesW(LPCWSTR Device, LPCWSTR Port, WORD Capability, LPWSTR Buffer, CONST DEVMODEW *DevMode)
{
  OutputDebugStringW(L"winspool DeviceCapabilitiesW stub called\n");
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

  return -1;
}


/*
 * @unimplemented
 */
LONG
STDCALL
DocumentPropertiesA(HWND Wnd, HANDLE Printer, LPSTR Device, PDEVMODEA DevModeOut, PDEVMODEA DevModeIn, DWORD Mode)
{
  OutputDebugStringW(L"winspool DocumentPropertiesA stub called\n");
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

  return -1L;
}


/*
 * @unimplemented
 */
LONG
STDCALL
DocumentPropertiesW(HWND Wnd, HANDLE Printer, LPWSTR Device, PDEVMODEW DevModeOut, PDEVMODEW DevModeIn, DWORD Mode)
{
  OutputDebugStringW(L"winspool DocumentPropertiesW stub called\n");
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

  return FALSE;
}


/*
 * @unimplemented
 */
BOOL
STDCALL
EndDocPrinter(HANDLE Printer)
{
  OutputDebugStringW(L"winspool EndDocPrinter stub called\n");
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

  return FALSE;
}


/*
 * @unimplemented
 */
BOOL
STDCALL
EndPagePrinter(HANDLE Printer)
{
  OutputDebugStringW(L"winspool EndPagePrinter stub called\n");
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

  return FALSE;
}


/*
 * @unimplemented
 */
BOOL
STDCALL
EnumFormsA(HANDLE Printer, DWORD Level, PBYTE Buffer, DWORD BufSize, PDWORD Needed, PDWORD Returned)
{
  OutputDebugStringW(L"winspool EnumFormsA stub called\n");
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

  return FALSE;
}


/*
 * @unimplemented
 */
BOOL
STDCALL
EnumFormsW(HANDLE Printer, DWORD Level, PBYTE Buffer, DWORD BufSize, PDWORD Needed, PDWORD Returned)
{
  OutputDebugStringW(L"winspool EnumFormsW stub called\n");

  return FALSE;
}


/*
 * @unimplemented
 */
BOOL
STDCALL
EnumJobsA(HANDLE Printer, DWORD First, DWORD NoJobs, DWORD Level, PBYTE Buffer, DWORD BufSize, PDWORD Needed, PDWORD Returned)
{
  OutputDebugStringW(L"winspool EnumJobsA stub called\n");
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

  return FALSE;
}


/*
 * @unimplemented
 */
BOOL
STDCALL
EnumJobsW(HANDLE Printer, DWORD First, DWORD NoJobs, DWORD Level, PBYTE Buffer, DWORD BufSize, PDWORD Needed, PDWORD Returned)
{
  OutputDebugStringW(L"winspool EnumJobsW stub called\n");
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

  return FALSE;
}


/*
 * @unimplemented
 */
BOOL
STDCALL
EnumMonitorsA(LPSTR Name, DWORD Level, PBYTE Buffer, DWORD BufSize, PDWORD Needed, PDWORD Returned)
{
  OutputDebugStringW(L"winspool EnumMonitorsA stub called\n");
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

  return FALSE;
}


/*
 * @unimplemented
 */
BOOL
STDCALL
EnumMonitorsW(LPWSTR Name, DWORD Level, PBYTE Buffer, DWORD BufSize, PDWORD Needed, PDWORD Returned)
{
  OutputDebugStringW(L"winspool EnumMonitorsW stub called\n");
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

  return FALSE;
}


/*
 * @unimplemented
 */
BOOL
STDCALL
EnumPortsA(LPSTR Name, DWORD Level, PBYTE Buffer, DWORD BufSize, PDWORD Needed, PDWORD Returned)
{
  OutputDebugStringW(L"winspool EnumPortsA stub called\n");
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

  return FALSE;
}


/*
 * @unimplemented
 */
BOOL
STDCALL
EnumPortsW(LPWSTR Name, DWORD Level, PBYTE Buffer, DWORD BufSize, PDWORD Needed, PDWORD Returned)
{
  OutputDebugStringW(L"winspool EnumPortsW stub called\n");
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

  return FALSE;
}


/*
 * @unimplemented
 */
DWORD
STDCALL
EnumPrinterDataA(HANDLE Printer, DWORD Index, LPSTR Name, DWORD NameSize, PDWORD NameReturned, PDWORD Type, PBYTE Buffer, DWORD BufSize, PDWORD BufReturned)
{
  OutputDebugStringW(L"winspool EnumPrinterDataA stub called\n");

  return ERROR_CALL_NOT_IMPLEMENTED;
}


/*
 * @unimplemented
 */
DWORD
STDCALL
EnumPrinterDataW(HANDLE Printer, DWORD Index, LPWSTR Name, DWORD NameSize, PDWORD NameReturned, PDWORD Type, PBYTE Buffer, DWORD BufSize, PDWORD BufReturned)
{
  OutputDebugStringW(L"winspool EnumPrinterDataW stub called\n");

  return ERROR_CALL_NOT_IMPLEMENTED;
}


/*
 * @unimplemented
 */
BOOL
STDCALL
EnumPrinterDriversA(LPSTR Name, LPSTR Environment, DWORD Level, PBYTE Buffer, DWORD BufSize, PDWORD Needed, PDWORD Returned)
{
  OutputDebugStringW(L"winspool EnumPrinterDriversA stub called\n");
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

  return FALSE;
}


/*
 * @unimplemented
 */
BOOL
STDCALL
EnumPrinterDriversW(LPWSTR Name, LPWSTR Environment, DWORD Level, PBYTE Buffer, DWORD BufSize, PDWORD Needed, PDWORD Returned)
{
  OutputDebugStringW(L"winspool EnumPrinterDriversW stub called\n");
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

  return FALSE;
}


/*
 * @unimplemented
 */
BOOL
STDCALL
EnumPrintersA(DWORD Flags, LPSTR Name, DWORD Level, PBYTE Buffer, DWORD BufSize, PDWORD Needed, PDWORD Returned)
{
  OutputDebugStringW(L"winspool EnumPrintersA stub called\n");
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

  return FALSE;
}


/*
 * @unimplemented
 */
BOOL
STDCALL
EnumPrintersW(DWORD Flags, LPWSTR Name, DWORD Level, PBYTE Buffer, DWORD BufSize, PDWORD Needed, PDWORD Returned)
{
  OutputDebugStringW(L"winspool EnumPrintersW stub called\n");
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

  return FALSE;
}


/*
 * @unimplemented
 */
BOOL
STDCALL
EnumPrintProcessorDatatypesA(LPSTR Name, LPSTR PrintProcessor, DWORD Level, PBYTE Buffer, DWORD BufSize, PDWORD Needed, PDWORD Returned)
{
  OutputDebugStringW(L"winspool EnumPrintProcessorDatatypesA stub called\n");
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

  return FALSE;
}


/*
 * @unimplemented
 */
BOOL
STDCALL
EnumPrintProcessorDatatypesW(LPWSTR Name, LPWSTR PrintProcessor, DWORD Level, PBYTE Buffer, DWORD BufSize, PDWORD Needed, PDWORD Returned)
{
  OutputDebugStringW(L"winspool EnumPrintProcessorDatatypesW stub called\n");
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

  return FALSE;
}


/*
 * @unimplemented
 */
BOOL
STDCALL
EnumPrintProcessorsA(LPSTR Name, LPSTR Environment, DWORD Level, PBYTE Buffer, DWORD BufSize, PDWORD Needed, PDWORD Returned)
{
  OutputDebugStringW(L"winspool EnumPrintProcessorsA stub called\n");
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

  return FALSE;
}


/*
 * @unimplemented
 */
BOOL
STDCALL
EnumPrintProcessorsW(LPWSTR Name, LPWSTR Environment, DWORD Level, PBYTE Buffer, DWORD BufSize, PDWORD Needed, PDWORD Returned)
{
  OutputDebugStringW(L"winspool EnumPrintProcessorsW stub called\n");
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

  return FALSE;
}


/*
 * @unimplemented
 */
BOOL
STDCALL
FindClosePrinterChangeNotification(HANDLE Printer)
{
  OutputDebugStringW(L"winspool FindClosePrinterChangeNotification stub called\n");
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

  return FALSE;
}


/*
 * @unimplemented
 */
HANDLE
STDCALL
FindFirstPrinterChangeNotification(HANDLE Printer, DWORD Flags, DWORD Options, PVOID NotifyOptions)
{
  OutputDebugStringW(L"winspool FindFirstPrinterChangeNotification stub called\n");
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

  return INVALID_HANDLE_VALUE;
}


/*
 * @unimplemented
 */
HANDLE
STDCALL
FindNextPrinterChangeNotification(HANDLE Printer, PDWORD Change, PVOID NotifyOptions, PVOID* NotifyInfo)
{
  OutputDebugStringW(L"winspool FindNextPrinterChangeNotification stub called\n");
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

  return INVALID_HANDLE_VALUE;
}


/*
 * @unimplemented
 */
BOOL
STDCALL
FreePrinterNotifyInfo(PVOID /* Really PPRINTER_NOTIFY_INFO */ NotifyInfo)
{
  OutputDebugStringW(L"winspool FreePrinterNotifyInfo stub called\n");
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

  return FALSE;
}


/*
 * @unimplemented
 */
BOOL
STDCALL
GetFormA(HANDLE Printer, LPSTR Name, DWORD Level, PBYTE Buffer, DWORD BufSize, PDWORD Needed)
{
  OutputDebugStringW(L"winspool GetFormA stub called\n");
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

  return FALSE;
}


/*
 * @unimplemented
 */
BOOL
STDCALL
GetFormW(HANDLE Printer, LPWSTR Name, DWORD Level, PBYTE Buffer, DWORD BufSize, PDWORD Needed)
{
  OutputDebugStringW(L"winspool GetFormW stub called\n");
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

  return FALSE;
}


/*
 * @unimplemented
 */
BOOL
STDCALL
GetJobA(HANDLE Printer, DWORD Job, DWORD Level, PBYTE Buffer, DWORD BufSize, PDWORD Needed)
{
  OutputDebugStringW(L"winspool GetJobA stub called\n");
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

  return FALSE;
}


/*
 * @unimplemented
 */
BOOL
STDCALL
GetJobW(HANDLE Printer, DWORD Job, DWORD Level, PBYTE Buffer, DWORD BufSize, PDWORD Needed)
{
  OutputDebugStringW(L"winspool GetJobW stub called\n");
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

  return FALSE;
}


/*
 * @unimplemented
 */
BOOL
STDCALL
GetPrinterA(HANDLE Printer, DWORD Level, PBYTE Buffer, DWORD BufSize, PDWORD Needed)
{
  OutputDebugStringW(L"winspool GetPrinterA stub called\n");
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

  return FALSE;
}


/*
 * @unimplemented
 */
BOOL
STDCALL
GetPrinterW(HANDLE Printer, DWORD Level, PBYTE Buffer, DWORD BufSize, PDWORD Needed)
{
  OutputDebugStringW(L"winspool GetPrinterW stub called\n");
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

  return FALSE;
}


/*
 * @unimplemented
 */
DWORD
STDCALL
GetPrinterDataA(HANDLE Printer, LPSTR Name, PDWORD Type, PBYTE Buffer, DWORD BufSize, PDWORD Needed)
{
  OutputDebugStringW(L"winspool GetPrinterDataA stub called\n");

  return ERROR_CALL_NOT_IMPLEMENTED;
}


/*
 * @unimplemented
 */
DWORD
STDCALL
GetPrinterDataW(HANDLE Printer, LPWSTR Name, PDWORD Type, PBYTE Buffer, DWORD BufSize, PDWORD Needed)
{
  OutputDebugStringW(L"winspool GetPrinterDataW stub called\n");

  return ERROR_CALL_NOT_IMPLEMENTED;
}


/*
 * @unimplemented
 */
DWORD
STDCALL
GetPrinterDriverA(HANDLE Printer, LPSTR Environment, DWORD Level, PBYTE Buffer, DWORD BufSize, PDWORD Needed)
{
  OutputDebugStringW(L"winspool GetPrinterDriverA stub called\n");
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

  return 0;
}


/*
 * @unimplemented
 */
DWORD
STDCALL
GetPrinterDriverW(HANDLE Printer, LPWSTR Environment, DWORD Level, PBYTE Buffer, DWORD BufSize, PDWORD Needed)
{
  OutputDebugStringW(L"winspool GetPrinterDriverW stub called\n");
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

  return 0;
}


/*
 * @unimplemented
 */
DWORD
STDCALL
GetPrinterDriverDirectoryA(LPSTR Name, LPSTR Environment, DWORD Level, PBYTE Buffer, DWORD BufSize, PDWORD Needed)
{
  OutputDebugStringW(L"winspool GetPrinterDriverDirectoryA stub called\n");
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

  return 0;
}


/*
 * @unimplemented
 */
DWORD
STDCALL
GetPrinterDriverDirectoryW(LPWSTR Name, LPWSTR Environment, DWORD Level, PBYTE Buffer, DWORD BufSize, PDWORD Needed)
{
  OutputDebugStringW(L"winspool GetPrinterDriverDirectoryW stub called\n");
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

  return 0;
}


/*
 * @unimplemented
 */
DWORD
STDCALL
GetPrintProcessorDirectoryA(LPSTR Name, LPSTR Environment, DWORD Level, PBYTE Buffer, DWORD BufSize, PDWORD Needed)
{
  OutputDebugStringW(L"winspool GetPrintProcessorDirectoryA stub called\n");
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

  return 0;
}


/*
 * @unimplemented
 */
DWORD
STDCALL
GetPrintProcessorDirectoryW(LPWSTR Name, LPWSTR Environment, DWORD Level, PBYTE Buffer, DWORD BufSize, PDWORD Needed)
{
  OutputDebugStringW(L"winspool GetPrintProcessorDirectoryW stub called\n");
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

  return 0;
}


/*
 * @unimplemented
 */
BOOL
STDCALL
OpenPrinterA(LPSTR Name, PHANDLE Printer, LPPRINTER_DEFAULTSA Defaults)
{
  OutputDebugStringW(L"winspool OpenPrinterA stub called\n");
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

  return FALSE;
}


/*
 * @unimplemented
 */
BOOL
STDCALL
OpenPrinterW(LPWSTR Name, PHANDLE Printer, LPPRINTER_DEFAULTSW Defaults)
{
  OutputDebugStringW(L"winspool OpenPrinterW stub called\n");
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

  return FALSE;
}


/*
 * @unimplemented
 */
DWORD
STDCALL
PrinterMessageBoxA(HANDLE Printer, DWORD Error, HWND Wnd, LPSTR Text, LPSTR Caption, DWORD Type)
{
  OutputDebugStringW(L"winspool PrinterMessageBoxA stub called\n");

  return IDCANCEL;
}


/*
 * @unimplemented
 */
DWORD
STDCALL
PrinterMessageBoxW(HANDLE Printer, DWORD Error, HWND Wnd, LPWSTR Text, LPWSTR Caption, DWORD Type)
{
  OutputDebugStringW(L"winspool PrinterMessageBoxW stub called\n");

  return IDCANCEL;
}


/*
 * @unimplemented
 */
BOOL
STDCALL
PrinterProperties(HWND Wnd, HANDLE Printer)
{
  OutputDebugStringW(L"winspool PrinterProperties stub called\n");
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

  return FALSE;
}


/*
 * @unimplemented
 */
BOOL
STDCALL
ReadPrinter(HANDLE Printer, PVOID Buffer, DWORD BufSize, PDWORD Received)
{
  OutputDebugStringW(L"winspool ReadPrinter stub called\n");
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

  return FALSE;
}


/*
 * @unimplemented
 */
BOOL
STDCALL
ResetPrinterA(HANDLE Printer, LPPRINTER_DEFAULTSA Defaults)
{
  OutputDebugStringW(L"winspool ResetPrinterA stub called\n");
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

  return FALSE;
}


/*
 * @unimplemented
 */
BOOL
STDCALL
ResetPrinterW(HANDLE Printer, LPPRINTER_DEFAULTSW Defaults)
{
  OutputDebugStringW(L"winspool ResetPrinterW stub called\n");
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

  return FALSE;
}


/*
 * @unimplemented
 */
BOOL
STDCALL
ScheduleJob(HANDLE Printer, DWORD Job)
{
  OutputDebugStringW(L"winspool ScheduleJob stub called\n");
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

  return FALSE;
}


/*
 * @unimplemented
 */
BOOL
STDCALL
SetFormA(HANDLE Printer, LPSTR Form, DWORD Level, PBYTE Buffer)
{
  OutputDebugStringW(L"winspool SetFormA stub called\n");
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

  return FALSE;
}


/*
 * @unimplemented
 */
BOOL
STDCALL
SetFormW(HANDLE Printer, LPWSTR Form, DWORD Level, PBYTE Buffer)
{
  OutputDebugStringW(L"winspool SetFormW stub called\n");
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

  return FALSE;
}


/*
 * @unimplemented
 */
BOOL
STDCALL
SetJobA(HANDLE Printer, DWORD Job, DWORD Level, PBYTE Buffer, DWORD Command)
{
  OutputDebugStringW(L"winspool SetJobA stub called\n");
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

  return FALSE;
}


/*
 * @unimplemented
 */
BOOL
STDCALL
SetJobW(HANDLE Printer, DWORD Job, DWORD Level, PBYTE Buffer, DWORD Command)
{
  OutputDebugStringW(L"winspool SetJobW stub called\n");
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

  return FALSE;
}


/*
 * @unimplemented
 */
BOOL
STDCALL
SetPrinterA(HANDLE Printer, DWORD Level, PBYTE Buffer, DWORD Command)
{
  OutputDebugStringW(L"winspool SetPrinterA stub called\n");
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

  return FALSE;
}


/*
 * @unimplemented
 */
BOOL
STDCALL
SetPrinterW(HANDLE Printer, DWORD Level, PBYTE Buffer, DWORD Command)
{
  OutputDebugStringW(L"winspool SetPrinterW stub called\n");
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

  return FALSE;
}


/*
 * @unimplemented
 */
BOOL
STDCALL
SetPrinterDataA(HANDLE Printer, LPSTR Name, DWORD Type, PBYTE Buffer, DWORD BufSize)
{
  OutputDebugStringW(L"winspool SetPrinterDataA stub called\n");
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

  return FALSE;
}


/*
 * @unimplemented
 */
BOOL
STDCALL
SetPrinterDataW(HANDLE Printer, LPWSTR Name, DWORD Type, PBYTE Buffer, DWORD BufSize)
{
  OutputDebugStringW(L"winspool SetPrinterDataW stub called\n");
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

  return FALSE;
}


/*
 * @unimplemented
 */
DWORD
STDCALL
StartDocPrinterA(HANDLE Printer, DWORD Level, PBYTE Buffer)
{
  OutputDebugStringW(L"winspool StartDocPrinterA stub called\n");
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

  return 0;
}


/*
 * @unimplemented
 */
DWORD
STDCALL
StartDocPrinterW(HANDLE Printer, DWORD Level, PBYTE Buffer)
{
  OutputDebugStringW(L"winspool StartDocPrinterW stub called\n");
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

  return 0;
}


/*
 * @unimplemented
 */
BOOL
STDCALL
StartPagePrinter(HANDLE Printer)
{
  OutputDebugStringW(L"winspool StartPagePrinter stub called\n");
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

  return FALSE;
}


/*
 * @unimplemented
 */
DWORD
STDCALL
WaitForPrinterChange(HANDLE Printer, DWORD Flags)
{
  OutputDebugStringW(L"winspool WaitForPrinterChange stub called\n");
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

  return 0;
}


/*
 * @unimplemented
 */
BOOL
STDCALL
WritePrinter(HANDLE Printer, PVOID Buffer, DWORD BufSize, PDWORD Written)
{
  OutputDebugStringW(L"winspool WritePrinter stub called\n");
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

  return FALSE;
}
