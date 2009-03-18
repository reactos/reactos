/* $Id$
 *
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS winspool DRV
 * FILE:        stubs.c
 * PURPOSE:     Stub functions
 * PROGRAMMERS: Ge van Geldorp (ge@gse.nl)
 * REVISIONS:
 */

#include <windows.h>
#include <winerror.h>

/*
 * @unimplemented
 */
BOOL
WINAPI
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
WINAPI
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
WINAPI
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
WINAPI
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
WINAPI
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
WINAPI
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
WINAPI
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
WINAPI
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
WINAPI
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
WINAPI
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
WINAPI
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
WINAPI
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
WINAPI
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
WINAPI
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
WINAPI
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
WINAPI
AddPrintProcessorW(LPWSTR Name, LPWSTR Environment, LPWSTR PathName, LPWSTR PrintProcessorName)
{
  OutputDebugStringW(L"winspool AddPrintProcessorW stub called\n");
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

  return FALSE;
}

/*
 * @unimplemented
 */
LONG
WINAPI
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
WINAPI
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
WINAPI
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
WINAPI
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
WINAPI
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
WINAPI
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
WINAPI
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
WINAPI
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
WINAPI
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
WINAPI
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
WINAPI
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
WINAPI
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
WINAPI
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
WINAPI
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
WINAPI
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
WINAPI
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
WINAPI
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
WINAPI
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
WINAPI
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
WINAPI
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
WINAPI
DeletePrintProcessorW(LPWSTR Name, LPWSTR Environment, LPWSTR PrintProcessor)
{
  OutputDebugStringW(L"winspool DeletePrintProcessorW stub called\n");
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

  return FALSE;
}

/*
 * @unimplemented
 */
DWORD
WINAPI
DeviceCapabilitiesA(LPCSTR Device, LPCSTR Port, WORD Capability, LPSTR Buffer, CONST DEVMODEA *DevMode)
{
  OutputDebugStringW(L"winspool DeviceCapabilitiesA stub called\n");
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

  return -1;
}


/*
 * @unimplemented
 */
DWORD
WINAPI
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
WINAPI
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
WINAPI
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
WINAPI
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
WINAPI
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
WINAPI
EnumFormsA(HANDLE Printer, DWORD Level, PBYTE Buffer, DWORD BufSize, PDWORD Needed, PDWORD Returned)
{
  OutputDebugStringW(L"winspool EnumFormsA stub called\n");
  *Needed = 0;
  *Returned = 0;
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

  return FALSE;
}


/*
 * @unimplemented
 */
BOOL
WINAPI
EnumFormsW(HANDLE Printer, DWORD Level, PBYTE Buffer, DWORD BufSize, PDWORD Needed, PDWORD Returned)
{
  OutputDebugStringW(L"winspool EnumFormsW stub called\n");
  *Needed = 0;
  *Returned = 0;

  return FALSE;
}


/*
 * @unimplemented
 */
BOOL
WINAPI
EnumJobsA(HANDLE Printer, DWORD First, DWORD NoJobs, DWORD Level, PBYTE Buffer, DWORD BufSize, PDWORD Needed, PDWORD Returned)
{
  OutputDebugStringW(L"winspool EnumJobsA stub called\n");
  *Needed = 0;
  *Returned = 0;

  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

  return FALSE;
}


/*
 * @unimplemented
 */
BOOL
WINAPI
EnumJobsW(HANDLE Printer, DWORD First, DWORD NoJobs, DWORD Level, PBYTE Buffer, DWORD BufSize, PDWORD Needed, PDWORD Returned)
{
  OutputDebugStringW(L"winspool EnumJobsW stub called\n");
  *Needed = 0;
  *Returned = 0;

  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

  return FALSE;
}


/*
 * @unimplemented
 */
BOOL
WINAPI
EnumMonitorsA(LPSTR Name, DWORD Level, PBYTE Buffer, DWORD BufSize, PDWORD Needed, PDWORD Returned)
{
  OutputDebugStringW(L"winspool EnumMonitorsA stub called\n");
  *Needed = 0;
  *Returned = 0;

  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

  return FALSE;
}


/*
 * @unimplemented
 */
BOOL
WINAPI
EnumMonitorsW(LPWSTR Name, DWORD Level, PBYTE Buffer, DWORD BufSize, PDWORD Needed, PDWORD Returned)
{
  OutputDebugStringW(L"winspool EnumMonitorsW stub called\n");
  *Needed = 0;
  *Returned = 0;

  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

  return FALSE;
}


/*
 * @unimplemented
 */
BOOL
WINAPI
EnumPortsA(LPSTR Name, DWORD Level, PBYTE Buffer, DWORD BufSize, PDWORD Needed, PDWORD Returned)
{
  OutputDebugStringW(L"winspool EnumPortsA stub called\n");
  *Needed = 0;
  *Returned = 0;

  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

  return FALSE;
}


/*
 * @unimplemented
 */
BOOL
WINAPI
EnumPortsW(LPWSTR Name, DWORD Level, PBYTE Buffer, DWORD BufSize, PDWORD Needed, PDWORD Returned)
{
  OutputDebugStringW(L"winspool EnumPortsW stub called\n");
  *Needed = 0;
  *Returned = 0;

  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

  return FALSE;
}


/*
 * @unimplemented
 */
DWORD
WINAPI
EnumPrinterDataA(HANDLE Printer, DWORD Index, LPSTR Name, DWORD NameSize, PDWORD NameReturned, PDWORD Type, PBYTE Buffer, DWORD BufSize, PDWORD BufReturned)
{
  OutputDebugStringW(L"winspool EnumPrinterDataA stub called\n");

  return ERROR_CALL_NOT_IMPLEMENTED;
}


/*
 * @unimplemented
 */
DWORD
WINAPI
EnumPrinterDataW(HANDLE Printer, DWORD Index, LPWSTR Name, DWORD NameSize, PDWORD NameReturned, PDWORD Type, PBYTE Buffer, DWORD BufSize, PDWORD BufReturned)
{
  OutputDebugStringW(L"winspool EnumPrinterDataW stub called\n");

  return ERROR_CALL_NOT_IMPLEMENTED;
}


/*
 * @unimplemented
 */
BOOL
WINAPI
EnumPrinterDriversA(LPSTR Name, LPSTR Environment, DWORD Level, PBYTE Buffer, DWORD BufSize, PDWORD Needed, PDWORD Returned)
{
  OutputDebugStringW(L"winspool EnumPrinterDriversA stub called\n");
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  *Needed = 0;
  *Returned = 0;

  return FALSE;
}


/*
 * @unimplemented
 */
BOOL
WINAPI
EnumPrinterDriversW(LPWSTR Name, LPWSTR Environment, DWORD Level, PBYTE Buffer, DWORD BufSize, PDWORD Needed, PDWORD Returned)
{
  OutputDebugStringW(L"winspool EnumPrinterDriversW stub called\n");
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  *Needed = 0;
  *Returned = 0;

  return FALSE;
}


/*
 * @unimplemented
 */
BOOL
WINAPI
EnumPrintersA(DWORD Flags, LPSTR Name, DWORD Level, PBYTE Buffer, DWORD BufSize, PDWORD Needed, PDWORD Returned)
{
  OutputDebugStringW(L"winspool EnumPrintersA stub called\n");
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  *Needed = 0;
  *Returned = 0;

  return FALSE;
}


/*
 * @unimplemented
 */
BOOL
WINAPI
EnumPrintersW(DWORD Flags, LPWSTR Name, DWORD Level, PBYTE Buffer, DWORD BufSize, PDWORD Needed, PDWORD Returned)
{
  OutputDebugStringW(L"winspool EnumPrintersW stub called\n");
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  *Needed = 0;
  *Returned = 0;

  return FALSE;
}


/*
 * @unimplemented
 */
BOOL
WINAPI
EnumPrintProcessorDatatypesA(LPSTR Name, LPSTR PrintProcessor, DWORD Level, PBYTE Buffer, DWORD BufSize, PDWORD Needed, PDWORD Returned)
{
  OutputDebugStringW(L"winspool EnumPrintProcessorDatatypesA stub called\n");
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  *Needed = 0;
  *Returned = 0;

  return FALSE;
}


/*
 * @unimplemented
 */
BOOL
WINAPI
EnumPrintProcessorDatatypesW(LPWSTR Name, LPWSTR PrintProcessor, DWORD Level, PBYTE Buffer, DWORD BufSize, PDWORD Needed, PDWORD Returned)
{
  OutputDebugStringW(L"winspool EnumPrintProcessorDatatypesW stub called\n");
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  *Needed = 0;
  *Returned = 0;

  return FALSE;
}


/*
 * @unimplemented
 */
BOOL
WINAPI
EnumPrintProcessorsA(LPSTR Name, LPSTR Environment, DWORD Level, PBYTE Buffer, DWORD BufSize, PDWORD Needed, PDWORD Returned)
{
  OutputDebugStringW(L"winspool EnumPrintProcessorsA stub called\n");
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  *Needed = 0;
  *Returned = 0;

  return FALSE;
}


/*
 * @unimplemented
 */
BOOL
WINAPI
EnumPrintProcessorsW(LPWSTR Name, LPWSTR Environment, DWORD Level, PBYTE Buffer, DWORD BufSize, PDWORD Needed, PDWORD Returned)
{
  OutputDebugStringW(L"winspool EnumPrintProcessorsW stub called\n");
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  *Needed = 0;
  *Returned = 0;

  return FALSE;
}


/*
 * @unimplemented
 */
BOOL
WINAPI
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
WINAPI
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
WINAPI
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
WINAPI
FreePrinterNotifyInfo(PPRINTER_NOTIFY_INFO NotifyInfo)
{
  OutputDebugStringW(L"winspool FreePrinterNotifyInfo stub called\n");
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

  return FALSE;
}


/*
 * @unimplemented
 */
BOOL
WINAPI
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
WINAPI
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
WINAPI
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
WINAPI
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
WINAPI
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
WINAPI
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
WINAPI
GetPrinterDataA(HANDLE Printer, LPSTR Name, PDWORD Type, PBYTE Buffer, DWORD BufSize, PDWORD Needed)
{
  OutputDebugStringW(L"winspool GetPrinterDataA stub called\n");

  return ERROR_CALL_NOT_IMPLEMENTED;
}


/*
 * @unimplemented
 */
DWORD
WINAPI
GetPrinterDataW(HANDLE Printer, LPWSTR Name, PDWORD Type, PBYTE Buffer, DWORD BufSize, PDWORD Needed)
{
  OutputDebugStringW(L"winspool GetPrinterDataW stub called\n");

  return ERROR_CALL_NOT_IMPLEMENTED;
}


/*
 * @unimplemented
 */
BOOL
WINAPI
GetPrinterDriverA(HANDLE Printer, LPSTR Environment, DWORD Level, LPBYTE Buffer, DWORD BufSize, LPDWORD Needed)
{
  OutputDebugStringW(L"winspool GetPrinterDriverA stub called\n");
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

  return 0;
}


/*
 * @unimplemented
 */
BOOL
WINAPI
GetPrinterDriverW(HANDLE Printer, LPWSTR Environment, DWORD Level, LPBYTE Buffer, DWORD BufSize, LPDWORD Needed)
{
  OutputDebugStringW(L"winspool GetPrinterDriverW stub called\n");
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

  return 0;
}


/*
 * @unimplemented
 */
BOOL
WINAPI
GetPrinterDriverDirectoryA(LPSTR Name, LPSTR Environment, DWORD Level, LPBYTE Buffer, DWORD BufSize, LPDWORD Needed)
{
  OutputDebugStringW(L"winspool GetPrinterDriverDirectoryA stub called\n");
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

  return 0;
}


/*
 * @unimplemented
 */
BOOL
WINAPI
GetPrinterDriverDirectoryW(LPWSTR Name, LPWSTR Environment, DWORD Level, LPBYTE Buffer, DWORD BufSize, LPDWORD Needed)
{
  OutputDebugStringW(L"winspool GetPrinterDriverDirectoryW stub called\n");
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

  return 0;
}


/*
 * @unimplemented
 */
BOOL
WINAPI
GetPrintProcessorDirectoryA(LPSTR Name, LPSTR Environment, DWORD Level, LPBYTE Buffer, DWORD BufSize, LPDWORD Needed)
{
  OutputDebugStringW(L"winspool GetPrintProcessorDirectoryA stub called\n");
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

  return 0;
}


/*
 * @unimplemented
 */
BOOL
WINAPI
GetPrintProcessorDirectoryW(LPWSTR Name, LPWSTR Environment, DWORD Level, LPBYTE Buffer, DWORD BufSize, LPDWORD Needed)
{
  OutputDebugStringW(L"winspool GetPrintProcessorDirectoryW stub called\n");
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

  return 0;
}


/*
 * @unimplemented
 */
BOOL
WINAPI
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
WINAPI
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
WINAPI
PrinterMessageBoxA(HANDLE Printer, DWORD Error, HWND Wnd, LPSTR Text, LPSTR Caption, DWORD Type)
{
  OutputDebugStringW(L"winspool PrinterMessageBoxA stub called\n");

  return IDCANCEL;
}


/*
 * @unimplemented
 */
DWORD
WINAPI
PrinterMessageBoxW(HANDLE Printer, DWORD Error, HWND Wnd, LPWSTR Text, LPWSTR Caption, DWORD Type)
{
  OutputDebugStringW(L"winspool PrinterMessageBoxW stub called\n");

  return IDCANCEL;
}


/*
 * @unimplemented
 */
BOOL
WINAPI
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
WINAPI
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
WINAPI
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
WINAPI
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
WINAPI
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
WINAPI
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
WINAPI
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
WINAPI
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
WINAPI
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
WINAPI
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
WINAPI
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
WINAPI
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
WINAPI
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
WINAPI
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
WINAPI
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
WINAPI
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
WINAPI
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
WINAPI
WritePrinter(HANDLE Printer, PVOID Buffer, DWORD BufSize, PDWORD Written)
{
  OutputDebugStringW(L"winspool WritePrinter stub called\n");
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

  return FALSE;
}


/*
 * @unimplemented
 */
BOOL
WINAPI
XcvDataW(HANDLE hXcv,
         LPCWSTR pszDataName,
         PBYTE pInputData,
         DWORD cbInputData,
         PBYTE pOutputData,
         DWORD cbOutputData,
         PDWORD pcbOutputNeeded,
         PDWORD pdwStatus)
{
    OutputDebugStringW(L"winspool XcvDataW stub called\n");
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

/*
 * @unimplemented
 */
BOOL
WINAPI
SetDefaultPrinterA(LPCSTR pszPrinter)
{
    OutputDebugStringW(L"winspool SetDefaultPrinterA stub called\n");
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

/*
 * @unimplemented
 */
BOOL
WINAPI
SetDefaultPrinterW(LPCWSTR pszPrinter)
{
    OutputDebugStringW(L"winspool SetDefaultPrinterW stub called\n");
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

/*
 * @unimplemented
 */
BOOL
WINAPI
AddPortExA(LPSTR pName,
           DWORD dwLevel,
           LPBYTE pBuffer,
           LPSTR pMonitorName)
{
    OutputDebugStringW(L"winspool AddPortExA stub called\n");
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

/*
 * @unimplemented
 */
BOOL
WINAPI
AddPortExW(LPWSTR pName,
           DWORD dwLevel,
           LPBYTE pBuffer,
           LPWSTR pMonitorName)
{
    OutputDebugStringW(L"winspool AddPortExW stub called\n");
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

/*
 * @unimplemented
 */
BOOL
WINAPI
AddPrinterDriverExA(LPSTR pName,
                    DWORD dwLevel,
                    LPBYTE pDriverInfo,
                    DWORD dwFileCopyFlags)
{
    OutputDebugStringW(L"winspool AddPrinterDriverExA stub called\n");
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

/*
 * @unimplemented
 */
BOOL
WINAPI
AddPrinterDriverExW(LPWSTR pName,
                    DWORD dwLevel,
                    LPBYTE pDriverInfo,
                    DWORD dwFileCopyFlags)
{
    OutputDebugStringW(L"winspool AddPrinterDriverExW stub called\n");
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

/*
 * @unimplemented
 */
DWORD
WINAPI
DeletePrinterDataExA(HANDLE hPrinter,
                     LPCSTR pKeyName,
                     LPCSTR pValueName)
{
    OutputDebugStringW(L"winspool DeletePrinterDataExA stub called\n");
    return ERROR_CALL_NOT_IMPLEMENTED;
}

/*
 * @unimplemented
 */
DWORD
WINAPI
DeletePrinterDataExW(HANDLE hPrinter,
                     LPCWSTR pKeyName,
                     LPCWSTR pValueName)
{
    OutputDebugStringW(L"winspool DeletePrinterDataExW stub called\n");
    return ERROR_CALL_NOT_IMPLEMENTED;
}

/*
 * @unimplemented
 */
BOOL
WINAPI
DeletePrinterDriverExA(LPSTR pName,
                       LPSTR pEnvironment,
                       LPSTR pDriverName,
                       DWORD dwDeleteFlag,
                       DWORD dwVersionFlag)
{
    OutputDebugStringW(L"winspool DeletePrinterDriverExA stub called\n");
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

/*
 * @unimplemented
 */
BOOL
WINAPI
DeletePrinterDriverExW(LPWSTR pName,
                       LPWSTR pEnvironment,
                       LPWSTR pDriverName,
                       DWORD dwDeleteFlag,
                       DWORD dwVersionFlag)
{
    OutputDebugStringW(L"winspool DeletePrinterDriverExW stub called\n");
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

/*
 * @unimplemented
 */
DWORD
WINAPI
EnumPrinterDataExA(HANDLE hPrinter,
                   LPCSTR pKeyName,
                   LPBYTE pEnumValues,
                   DWORD cbEnumValues,
                   LPDWORD pcbEnumValues,
                   LPDWORD pnEnumValues)
{
    OutputDebugStringW(L"winspool EnumPrinterDataExA stub called\n");
    return ERROR_CALL_NOT_IMPLEMENTED;
}

/*
 * @unimplemented
 */
DWORD
WINAPI
EnumPrinterDataExW(HANDLE hPrinter,
                   LPCWSTR pKeyName,
                   LPBYTE pEnumValues,
                   DWORD cbEnumValues,
                   LPDWORD pcbEnumValues,
                   LPDWORD pnEnumValues)
{
    OutputDebugStringW(L"winspool EnumPrinterDataExW stub called\n");
    return ERROR_CALL_NOT_IMPLEMENTED;
}

/*
 * @unimplemented
 */
LONG
WINAPI
ExtDeviceMode(HWND hWnd,
              HANDLE hInst,
              LPDEVMODEA pDevModeOutput,
              LPSTR pDeviceName,
              LPSTR pPort,
              LPDEVMODEA pDevModeInput,
              LPSTR pProfile,
              DWORD fMode)
{
    OutputDebugStringW(L"winspool ExtDeviceMode stub called\n");
    return -1;
}

/*
 * @unimplemented
 */
DWORD
WINAPI
GetPrinterDataExA(HANDLE hPrinter,
                  LPCSTR pKeyName,
                  LPCSTR pValueName,
                  LPDWORD pType,
                  LPBYTE pData,
                  DWORD nSize,
                  LPDWORD pcbNeeded)
{
    OutputDebugStringW(L"winspool GetPrinterDataExA stub called\n");
    return ERROR_CALL_NOT_IMPLEMENTED;
}

/*
 * @unimplemented
 */
DWORD
WINAPI
GetPrinterDataExW(HANDLE hPrinter,
                  LPCWSTR pKeyName,
                  LPCWSTR pValueName,
                  LPDWORD pType,
                  LPBYTE pData,
                  DWORD nSize,
                  LPDWORD pcbNeeded)
{
    OutputDebugStringW(L"winspool GetPrinterDataExW stub called\n");
    return ERROR_CALL_NOT_IMPLEMENTED;
}

/*
 * @unimplemented
 */
DWORD
WINAPI
SetPrinterDataExA(HANDLE hPrinter,
                  LPCSTR pKeyName,
                  LPCSTR pValueName,
                  DWORD dwType,
                  LPBYTE pData,
                  DWORD cbData)
{
    OutputDebugStringW(L"winspool SetPrinterDataExA stub called\n");
    return ERROR_CALL_NOT_IMPLEMENTED;
}

/*
 * @unimplemented
 */
DWORD
WINAPI
SetPrinterDataExW(HANDLE hPrinter,
                  LPCWSTR pKeyName,
                  LPCWSTR pValueName,
                  DWORD dwType,
                  LPBYTE pData,
                  DWORD cbData)
{
    OutputDebugStringW(L"winspool SetPrinterDataExW stub called\n");
    return ERROR_CALL_NOT_IMPLEMENTED;
}

/*
 * @unimplemented
 */
BOOL
WINAPI
SpoolerInit(VOID)
{
    OutputDebugStringW(L"winspool SpoolerInit stub called\n");
    return FALSE;
}
