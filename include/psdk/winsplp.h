/*
 * winsplp.h
 *
 * This file is part of the ReactOS PSDK package.
 *
 * Contributors:
 *   Created by Amine Khaldi.
 *
 * THIS SOFTWARE IS NOT COPYRIGHTED
 *
 * This source code is offered for use in the public domain. You may
 * use, modify or distribute it freely.
 *
 * This code is distributed in the hope that it will be useful but
 * WITHOUT ANY WARRANTY. ALL WARRANTIES, EXPRESS OR IMPLIED ARE HEREBY
 * DISCLAIMED. This includes but is not limited to warranties of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 */

#pragma once

#ifndef _WINSPLP_
#define _WINSPLP_

#ifdef __cplusplus
extern "C" {
#endif

#if (STRICT && (NTDDI_VERSION >= NTDDI_VISTA))
#define HKEYMONITOR HKEY
#else
#define HKEYMONITOR HANDLE
#endif

#define PRINTER_NOTIFY_STATUS_ENDPOINT  1
#define PRINTER_NOTIFY_STATUS_POLL      2
#define PRINTER_NOTIFY_STATUS_INFO      4

#define ROUTER_UNKNOWN      0
#define ROUTER_SUCCESS      1
#define ROUTER_STOP_ROUTING 2

#if (NTDDI_VERSION >= NTDDI_WINXP)
#define MONITOR2_SIZE_WIN2K (sizeof(DWORD) + (sizeof(PVOID)*18))
#endif

#define COPYFILE_EVENT_SET_PRINTER_DATAEX           1
#define COPYFILE_EVENT_DELETE_PRINTER               2
#define COPYFILE_EVENT_ADD_PRINTER_CONNECTION       3
#define COPYFILE_EVENT_DELETE_PRINTER_CONNECTION    4
#define COPYFILE_EVENT_FILES_CHANGED                5

#define COPYFILE_FLAG_CLIENT_SPOOLER             0x00000001
#define COPYFILE_FLAG_SERVER_SPOOLER             0x00000002

#define PRINTER_NOTIFY_INFO_DATA_COMPACT         1

typedef struct _PRINTER_NOTIFY_INIT {
  DWORD Size;
  DWORD Reserved;
  DWORD PollTime;
} PRINTER_NOTIFY_INIT, *LPPRINTER_NOTIFY_INIT, *PPRINTER_NOTIFY_INIT;

typedef struct _SPLCLIENT_INFO_1 {
  DWORD dwSize;
  LPWSTR pMachineName;
  LPWSTR pUserName;
  DWORD dwBuildNum;
  DWORD dwMajorVersion;
  DWORD dwMinorVersion;
  WORD wProcessorArchitecture;
} SPLCLIENT_INFO_1, *LPSPLCLIENT_INFO_1, *PSPLCLIENT_INFO_1;

typedef struct _SPLCLIENT_INFO_2_V1{
  ULONG_PTR hSplPrinter;
} SPLCLIENT_INFO_2_W2K;

typedef struct _SPLCLIENT_INFO_2_V2{
#ifdef _WIN64
  DWORD64 hSplPrinter;
#else
  DWORD32 hSplPrinter;
#endif
} SPLCLIENT_INFO_2_WINXP;

typedef struct _SPLCLIENT_INFO_2_V3{
  UINT64 hSplPrinter;
} SPLCLIENT_INFO_2_LONGHORN;

typedef struct _PRINTPROVIDOR {

  BOOL
  (WINAPI *fpOpenPrinter)(
    _In_opt_ PWSTR lpPrinterName,
    _Out_ HANDLE *phPrinter,
    _In_opt_ PPRINTER_DEFAULTSW pDefault);

  BOOL
  (WINAPI *fpSetJob)(
    _In_ HANDLE hPrinter,
    _In_ DWORD JobID,
    _In_ DWORD Level,
    _In_reads_opt_(_Inexpressible_(0)) LPBYTE pJob,
    _In_ DWORD Command);

  BOOL
  (WINAPI *fpGetJob)(
    _In_ HANDLE hPrinter,
    _In_ DWORD JobID,
    _In_ DWORD Level,
    _Out_writes_bytes_to_opt_(cbBuf, *pcbNeeded) LPBYTE pJob,
    _In_ DWORD cbBuf,
    _Out_ LPDWORD pcbNeeded);

  BOOL
  (WINAPI *fpEnumJobs)(
    _In_ HANDLE hPrinter,
    _In_ DWORD FirstJob,
    _In_ DWORD NoJobs,
    _In_ DWORD Level,
    _Out_writes_bytes_to_opt_(cbBuf, *pcbNeeded) LPBYTE pJob,
    _In_ DWORD cbBuf,
    _Out_ LPDWORD pcbNeeded,
    _Out_ LPDWORD pcReturned);

  HANDLE
  (WINAPI *fpAddPrinter)(
    _In_opt_ LPWSTR pName,
    _In_ DWORD Level,
    _In_reads_(_Inexpressible_(0)) LPBYTE pPrinter);

  BOOL (WINAPI *fpDeletePrinter)(_In_ HANDLE hPrinter);

  BOOL
  (WINAPI *fpSetPrinter)(
    _In_ HANDLE hPrinter,
    _In_ DWORD Level,
    _In_reads_(_Inexpressible_(0)) LPBYTE pPrinter,
    _In_ DWORD Command);

  BOOL
  (WINAPI *fpGetPrinter)(
    _In_ HANDLE hPrinter,
    _In_ DWORD Level,
    _Out_writes_bytes_to_opt_(cbBuf, *pcbNeeded) LPBYTE pPrinter,
    _In_ DWORD cbBuf,
    _Out_ LPDWORD pcbNeeded);

  BOOL
  (WINAPI *fpEnumPrinters)(
    _In_ DWORD dwType,
    _In_opt_ LPWSTR lpszName,
    _In_ DWORD dwLevel,
    _Out_writes_bytes_to_opt_(cbBuf, *pcbNeeded) LPBYTE lpbPrinters,
    _In_ DWORD cbBuf,
    _Out_ LPDWORD lpdwNeeded,
    _Out_ LPDWORD lpdwReturned);

  BOOL
  (WINAPI *fpAddPrinterDriver)(
    _In_opt_ LPWSTR pName,
    _In_ DWORD Level,
    _In_reads_(_Inexpressible_(0)) LPBYTE pDriverInfo);

  BOOL
  (WINAPI *fpEnumPrinterDrivers)(
    _In_opt_ LPWSTR pName,
    _In_opt_ LPWSTR pEnvironment,
    _In_ DWORD Level,
    _Out_writes_bytes_to_opt_(cbBuf, *pcbNeeded) LPBYTE pDriverInfo,
    _In_ DWORD cbBuf,
    _Out_ LPDWORD pcbNeeded,
    _Out_ LPDWORD pcbReturned);

  BOOL
  (WINAPI *fpGetPrinterDriver)(
    _In_ HANDLE hPrinter,
    _In_opt_ LPWSTR pEnvironment,
    _In_ DWORD Level,
    _Out_writes_bytes_to_opt_(cbBuf, *pcbNeeded) LPBYTE pDriverInfo,
    _In_ DWORD cbBuf,
    _Out_ LPDWORD pcbNeeded);

  BOOL
  (WINAPI *fpGetPrinterDriverDirectory)(
    _In_opt_ LPWSTR pName,
    _In_opt_ LPWSTR pEnvironment,
    _In_ DWORD Level,
    _Out_writes_bytes_to_opt_(cbBuf, *pcbNeeded) LPBYTE pDriverDirectory,
    _In_ DWORD cbBuf,
    _Out_ LPDWORD pcbNeeded);

  BOOL
  (WINAPI *fpDeletePrinterDriver)(
    _In_opt_ LPWSTR pName,
    _In_opt_ LPWSTR pEnvironment,
    _In_ LPWSTR pDriverName);

  BOOL
  (WINAPI *fpAddPrintProcessor)(
    _In_opt_ LPWSTR pName,
    _In_opt_ LPWSTR pEnvironment,
    _In_ LPWSTR pPathName,
    _In_ LPWSTR pPrintProcessorName);

  BOOL
  (WINAPI *fpEnumPrintProcessors)(
    _In_opt_ LPWSTR pName,
    _In_opt_ LPWSTR pEnvironment,
    _In_ DWORD Level,
    _Out_writes_bytes_to_opt_(cbBuf, *pcbNeeded) LPBYTE pPrintProcessorInfo,
    _In_ DWORD cbBuf,
    _Out_ LPDWORD pcbNeeded,
    _Out_ LPDWORD pcbReturned);

  BOOL
  (WINAPI *fpGetPrintProcessorDirectory)(
    _In_opt_ LPWSTR pName,
    _In_opt_ LPWSTR pEnvironment,
    _In_ DWORD Level,
    _Out_writes_bytes_to_opt_(cbBuf, *pcbNeeded) LPBYTE pPrintProcessorInfo,
    _In_ DWORD cbBuf,
    _In_ LPDWORD pcbNeeded);

  BOOL
  (WINAPI *fpDeletePrintProcessor)(
    _In_opt_ LPWSTR pName,
    _In_opt_ LPWSTR pEnvironment,
    _In_ LPWSTR pPrintProcessorName);

  BOOL
  (WINAPI *fpEnumPrintProcessorDatatypes)(
    _In_opt_ LPWSTR pName,
    _In_ LPWSTR pPrintProcessorName,
    _In_ DWORD Level,
    _Out_writes_bytes_to_opt_(cbBuf, *pcbNeeded) LPBYTE pDatatypes,
    _In_ DWORD cbBuf,
    _Out_ LPDWORD pcbNeeded,
    _Out_ LPDWORD pcbReturned);

  DWORD
  (WINAPI *fpStartDocPrinter)(
    _In_ HANDLE hPrinter,
    _In_ DWORD Level,
    _In_reads_(_Inexpressible_(0)) LPBYTE pDocInfo);

  BOOL (WINAPI *fpStartPagePrinter)(_In_ HANDLE hPrinter);

  BOOL
  (WINAPI *fpWritePrinter)(
    _In_ HANDLE hPrinter,
    _In_reads_bytes_(cbBuf) LPVOID pBuf,
    _In_ DWORD cbBuf,
    _Out_ LPDWORD pcWritten);

  BOOL (WINAPI *fpEndPagePrinter)(_In_ HANDLE hPrinter);

  BOOL (WINAPI *fpAbortPrinter)(_In_ HANDLE hPrinter);

  BOOL
  (WINAPI *fpReadPrinter)(
    _In_ HANDLE hPrinter,
    _Out_writes_bytes_to_opt_(cbBuf, *pNoBytesRead) LPVOID pBuf,
    _In_ DWORD cbBuf,
    _Out_ LPDWORD pNoBytesRead);

  BOOL (WINAPI *fpEndDocPrinter)(_In_ HANDLE hPrinter);

  BOOL
  (WINAPI *fpAddJob)(
    _In_ HANDLE hPrinter,
    _In_ DWORD Level,
    _Out_writes_bytes_to_opt_(cbBuf, *pcbNeeded) LPBYTE pData,
    _In_ DWORD cbBuf,
    _Out_ LPDWORD pcbNeeded);

  BOOL (WINAPI *fpScheduleJob)(_In_ HANDLE hPrinter, _In_ DWORD JobID);

  DWORD
  (WINAPI *fpGetPrinterData)(
    _In_ HANDLE hPrinter,
    _In_ LPWSTR pValueName,
    _Out_opt_ LPDWORD pType,
    _Out_writes_bytes_to_opt_(nSize, *pcbNeeded) LPBYTE pData,
    _In_ DWORD nSize,
    _Out_ LPDWORD pcbNeeded);

  DWORD
  (WINAPI *fpSetPrinterData)(
    _In_ HANDLE hPrinter,
    _In_ LPWSTR pValueName,
    _In_ DWORD Type,
    _In_reads_bytes_(cbData) LPBYTE pData,
    _In_ DWORD cbData);

  DWORD
  (WINAPI *fpWaitForPrinterChange)(
    _In_ HANDLE hPrinter,
    _In_ DWORD Flags);

  BOOL (WINAPI *fpClosePrinter)(_In_ HANDLE phPrinter);

  BOOL
  (WINAPI *fpAddForm)(
    _In_ HANDLE hPrinter,
    _In_ DWORD Level,
    _In_reads_(_Inexpressible_(0)) LPBYTE pForm);

  BOOL (WINAPI *fpDeleteForm)(_In_ HANDLE hPrinter, _In_ LPWSTR pFormName);

  BOOL
  (WINAPI *fpGetForm)(
    _In_ HANDLE hPrinter,
    _In_ LPWSTR pFormName,
    _In_ DWORD Level,
    _Out_writes_bytes_to_opt_(cbBuf, *pcbNeeded) LPBYTE pForm,
    _In_ DWORD cbBuf,
    _Out_ LPDWORD pcbNeeded);

  BOOL
  (WINAPI *fpSetForm)(
    _In_ HANDLE hPrinter,
    _In_ LPWSTR pFormName,
    _In_ DWORD Level,
    _In_reads_(_Inexpressible_(0)) LPBYTE pForm);

  BOOL
  (WINAPI *fpEnumForms)(
    _In_ HANDLE hPrinter,
    _In_ DWORD Level,
    _Out_writes_bytes_to_opt_(cbBuf, *pcbNeeded) LPBYTE pForm,
    _In_ DWORD cbBuf,
    _Out_ LPDWORD pcbNeeded,
    _Out_ LPDWORD pcReturned);

  BOOL
  (WINAPI *fpEnumMonitors)(
    _In_opt_ LPWSTR pName,
    _In_ DWORD Level,
    _Out_writes_bytes_to_opt_(cbBuf, *pcbNeeded) LPBYTE pMonitors,
    _In_ DWORD cbBuf,
    _Out_ LPDWORD pcbNeeded,
    _Out_ LPDWORD pcReturned);

  BOOL
  (WINAPI *fpEnumPorts)(
    _In_opt_ LPWSTR pName,
    _In_ DWORD Level,
    _Out_writes_bytes_to_opt_(cbBuf, *pcbNeeded) LPBYTE pPorts,
    _In_ DWORD cbBuf,
    _Out_ LPDWORD pcbNeeded,
    _Out_ LPDWORD pcReturned);

  BOOL
  (WINAPI *fpAddPort)(
    _In_opt_ LPWSTR pName,
    _In_ HWND hWnd,
    _In_ LPWSTR pMonitorName);

  BOOL
  (WINAPI *fpConfigurePort)(
    _In_opt_ LPWSTR pName,
    _In_ HWND hWnd,
    _In_ LPWSTR pPortName);

  BOOL
  (WINAPI *fpDeletePort)(
    _In_opt_ LPWSTR pName,
    _In_ HWND hWnd,
    _In_ LPWSTR pPortName);

  HANDLE
  (WINAPI *fpCreatePrinterIC)(
    _In_ HANDLE hPrinter,
    _In_opt_ LPDEVMODEW pDevMode);

  BOOL
  (WINAPI *fpPlayGdiScriptOnPrinterIC)(
    _In_ HANDLE hPrinterIC,
    _In_reads_bytes_(cIn) LPBYTE pIn,
    _In_ DWORD cIn,
    _Out_writes_bytes_(cOut) LPBYTE pOut,
    _In_ DWORD cOut,
    _In_ DWORD ul);

  BOOL (WINAPI *fpDeletePrinterIC)(_In_ HANDLE hPrinterIC);

  BOOL (WINAPI *fpAddPrinterConnection)(_In_ LPWSTR pName);

  BOOL (WINAPI *fpDeletePrinterConnection)(_In_ LPWSTR pName);

  DWORD
  (WINAPI *fpPrinterMessageBox)(
    _In_ HANDLE hPrinter,
    _In_ DWORD Error,
    _In_ HWND hWnd,
    _In_ LPWSTR pText,
    _In_ LPWSTR pCaption,
    _In_ DWORD dwType);

  BOOL
  (WINAPI *fpAddMonitor)(
    _In_opt_ LPWSTR pName,
    _In_ DWORD Level,
    _In_reads_(_Inexpressible_(0)) LPBYTE pMonitors);

  BOOL
  (WINAPI *fpDeleteMonitor)(
    _In_ LPWSTR pName,
    _In_opt_ LPWSTR pEnvironment,
    _In_ LPWSTR pMonitorName);

  BOOL
  (WINAPI *fpResetPrinter)(
    _In_ HANDLE hPrinter,
    _In_ LPPRINTER_DEFAULTSW pDefault);

  BOOL
  (WINAPI *fpGetPrinterDriverEx)(
    _In_ HANDLE hPrinter,
    _In_opt_ LPWSTR pEnvironment,
    _In_ DWORD Level,
    _Out_writes_bytes_to_opt_(cbBuf, *pcbNeeded) LPBYTE pDriverInfo,
    _In_ DWORD cbBuf,
    _Out_ LPDWORD pcbNeeded,
    _In_ DWORD dwClientMajorVersion,
    _In_ DWORD dwClientMinorVersion,
    _Out_ PDWORD pdwServerMajorVersion,
    _Out_ PDWORD pdwServerMinorVersion);

  HANDLE (WINAPI *fpFindFirstPrinterChangeNotification)(HANDLE hPrinter,
                                                        DWORD fdwFlags,
                                                        DWORD fdwOptions,
                                                        LPVOID pPrinterNotifyOptions);

  BOOL (WINAPI *fpFindClosePrinterChangeNotification)(_In_ HANDLE hChange);

  BOOL
  (WINAPI *fpAddPortEx)(
    _In_opt_ LPWSTR pName,
    _In_ DWORD Level,
    _In_reads_(_Inexpressible_(0)) LPBYTE lpBuffer,
    _In_ LPWSTR lpMonitorName);

  BOOL (WINAPI *fpShutDown)(_In_opt_ LPVOID pvReserved);

  BOOL
  (WINAPI *fpRefreshPrinterChangeNotification)(
    _In_ HANDLE hPrinter,
    _In_ DWORD Reserved,
    _In_opt_ PVOID pvReserved,
    _In_ PVOID pPrinterNotifyInfo);

  BOOL
  (WINAPI *fpOpenPrinterEx)(
    _In_opt_ LPWSTR pPrinterName,
    _Out_ LPHANDLE phPrinter,
    _In_opt_ LPPRINTER_DEFAULTSW pDefault,
    _In_reads_opt_(_Inexpressible_(0)) LPBYTE pClientInfo,
    _In_ DWORD Level);

  HANDLE
  (WINAPI *fpAddPrinterEx)(
    _In_opt_ LPWSTR pName,
    _In_ DWORD Level,
    _In_reads_(_Inexpressible_(0)) LPBYTE pPrinter,
    _In_reads_opt_(_Inexpressible_(0)) LPBYTE pClientInfo,
    _In_ DWORD ClientInfoLevel);

  BOOL
  (WINAPI *fpSetPort)(
    _In_opt_ LPWSTR pName,
    _In_ LPWSTR pPortName,
    _In_ DWORD dwLevel,
    _In_reads_(_Inexpressible_(0)) LPBYTE pPortInfo);

  DWORD
  (WINAPI *fpEnumPrinterData)(
    _In_ HANDLE hPrinter,
    _In_ DWORD dwIndex,
    _Out_writes_bytes_to_opt_(cbValueName, *pcbValueName) LPWSTR pValueName,
    _In_ DWORD cbValueName,
    _Out_ LPDWORD pcbValueName,
    _Out_opt_ LPDWORD pType,
    _Out_writes_bytes_to_opt_(cbData, *pcbData) LPBYTE pData,
    _In_ DWORD cbData,
    _Out_ LPDWORD pcbData);

  DWORD
  (WINAPI *fpDeletePrinterData)(
    _In_ HANDLE hPrinter,
    _In_ LPWSTR pValueName);

  DWORD
  (WINAPI *fpClusterSplOpen)(
    _In_ LPCWSTR pszServer,
    _In_ LPCWSTR pszResource,
    _Out_ PHANDLE phSpooler,
    _In_ LPCWSTR pszName,
    _In_ LPCWSTR pszAddress);

  DWORD (WINAPI *fpClusterSplClose)(_In_ HANDLE hSpooler);

  DWORD (WINAPI *fpClusterSplIsAlive)(_In_ HANDLE hSpooler);

  DWORD
  (WINAPI *fpSetPrinterDataEx)(
    _In_ HANDLE hPrinter,
    _In_ LPCWSTR pKeyName,
    _In_ LPCWSTR pValueName,
    _In_ DWORD Type,
    _In_reads_bytes_(cbData) LPBYTE pData,
    _In_ DWORD cbData);

  DWORD
  (WINAPI *fpGetPrinterDataEx)(
    _In_ HANDLE hPrinter,
    _In_ LPCWSTR pKeyName,
    _In_ LPCWSTR pValueName,
    _Out_opt_ LPDWORD pType,
    _Out_writes_bytes_to_opt_(nSize, *pcbNeeded) LPBYTE pData,
    _In_ DWORD nSize,
    _Out_ LPDWORD pcbNeeded);

  DWORD
  (WINAPI *fpEnumPrinterDataEx)(
    _In_ HANDLE hPrinter,
    _In_ LPCWSTR pKeyName,
    _Out_writes_bytes_to_opt_(cbEnumValues, *pcbEnumValues) LPBYTE pEnumValues,
    _In_ DWORD cbEnumValues,
    _Out_ LPDWORD pcbEnumValues,
    _Out_ LPDWORD pnEnumValues);

  DWORD
  (WINAPI *fpEnumPrinterKey)(
    _In_ HANDLE hPrinter,
    _In_ LPCWSTR pKeyName,
    _Out_writes_bytes_to_opt_(cbSubkey, *pcbSubkey) LPWSTR pSubkey,
    _In_ DWORD cbSubkey,
    _Out_ LPDWORD pcbSubkey);

  DWORD
  (WINAPI *fpDeletePrinterDataEx)(
    _In_ HANDLE hPrinter,
    _In_ LPCWSTR pKeyName,
    _In_ LPCWSTR pValueName);

  DWORD
  (WINAPI *fpDeletePrinterKey)(
    _In_ HANDLE hPrinter,
    _In_ LPCWSTR pKeyName);

  BOOL
  (WINAPI *fpSeekPrinter)(
    _In_ HANDLE hPrinter,
    _In_ LARGE_INTEGER liDistanceToMove,
    _Out_ PLARGE_INTEGER pliNewPointer,
    _In_ DWORD dwMoveMethod,
    _In_ BOOL bWrite);

  BOOL
  (WINAPI *fpDeletePrinterDriverEx)(
    _In_opt_ LPWSTR pName,
    _In_opt_ LPWSTR pEnvironment,
    _In_ LPWSTR pDriverName,
    _In_ DWORD dwDeleteFlag,
    _In_ DWORD dwVersionNum);

  BOOL
  (WINAPI *fpAddPerMachineConnection)(
    _In_opt_ LPCWSTR pServer,
    _In_ LPCWSTR pPrinterName,
    _In_ LPCWSTR pPrintServer,
    _In_ LPCWSTR pProvider);

  BOOL
  (WINAPI *fpDeletePerMachineConnection)(
    _In_opt_ LPCWSTR pServer,
    _In_ LPCWSTR pPrinterName);

  BOOL
  (WINAPI *fpEnumPerMachineConnections)(
    _In_opt_ LPCWSTR pServer,
    _Out_writes_bytes_to_opt_(cbBuf, *pcbNeeded) LPBYTE pPrinterEnum,
    _In_ DWORD cbBuf,
    _Out_ LPDWORD pcbNeeded,
    _Out_ LPDWORD pcReturned);

  BOOL
  (WINAPI *fpXcvData)(
    _In_ HANDLE hXcv,
    _In_ LPCWSTR pszDataName,
    _In_reads_bytes_(cbInputData) PBYTE pInputData,
    _In_ DWORD cbInputData,
    _Out_writes_bytes_to_opt_(cbOutputData, *pcbOutputNeeded) PBYTE pOutputData,
    _In_ DWORD cbOutputData,
    _Out_ PDWORD pcbOutputNeeded,
    _Out_ PDWORD pdwStatus);

  BOOL
  (WINAPI *fpAddPrinterDriverEx)(
    _In_opt_ LPWSTR pName,
    _In_ DWORD Level,
    _In_reads_(_Inexpressible_(0)) LPBYTE pDriverInfo,
    _In_ DWORD dwFileCopyFlags);

  BOOL
  (WINAPI *fpSplReadPrinter)(
    _In_ HANDLE hPrinter,
    _Out_writes_bytes_(cbBuf) LPBYTE *pBuf,
    _In_ DWORD cbBuf);

  BOOL (WINAPI *fpDriverUnloadComplete)(_In_ LPWSTR pDriverFile);

  BOOL
  (WINAPI *fpGetSpoolFileInfo)(
    _In_ HANDLE hPrinter,
    _Outptr_result_maybenull_ LPWSTR *pSpoolDir,
    _Out_ LPHANDLE phFile,
    _In_ HANDLE hSpoolerProcess,
    _In_ HANDLE hAppProcess);

  BOOL (WINAPI *fpCommitSpoolData)(_In_ HANDLE hPrinter, _In_ DWORD cbCommit);

  BOOL (WINAPI *fpCloseSpoolFileHandle)(_In_ HANDLE hPrinter);

  BOOL
  (WINAPI *fpFlushPrinter)(
    _In_ HANDLE hPrinter,
    _In_reads_bytes_(cbBuf) LPBYTE pBuf,
    _In_ DWORD cbBuf,
    _Out_ LPDWORD pcWritten,
    _In_ DWORD cSleep);

  DWORD
  (WINAPI *fpSendRecvBidiData)(
    _In_ HANDLE hPort,
    _In_ LPCWSTR pAction,
    _In_ LPBIDI_REQUEST_CONTAINER pReqData,
    _Outptr_ LPBIDI_RESPONSE_CONTAINER *ppResData);

  BOOL (WINAPI *fpAddDriverCatalog)(HANDLE hPrinter, DWORD dwLevel,
                                    VOID *pvDriverInfCatInfo, DWORD dwCatalogCopyFlags);
} PRINTPROVIDOR, *LPPRINTPROVIDOR;

typedef struct _PRINTPROCESSOROPENDATA {
  PDEVMODEW pDevMode;
  LPWSTR pDatatype;
  LPWSTR pParameters;
  LPWSTR pDocumentName;
  DWORD JobId;
  LPWSTR pOutputFile;
  LPWSTR pPrinterName;
} PRINTPROCESSOROPENDATA, *LPPRINTPROCESSOROPENDATA, *PPRINTPROCESSOROPENDATA;

typedef struct _MONITORREG {
  DWORD cbSize;

  LONG
  (WINAPI *fpCreateKey)(
    _In_ HANDLE hcKey,
    _In_ LPCWSTR pszSubKey,
    _In_ DWORD dwOptions,
    _In_ REGSAM samDesired,
    _In_opt_ PSECURITY_ATTRIBUTES pSecurityAttributes,
    _Out_ PHANDLE phckResult,
    _Out_opt_ PDWORD pdwDisposition,
    _In_ HANDLE hSpooler);

  LONG
  (WINAPI *fpOpenKey)(
    _In_ HANDLE hcKey,
    _In_ LPCWSTR pszSubKey,
    _In_ REGSAM samDesired,
    _Out_ PHANDLE phkResult,
    _In_ HANDLE hSpooler);

  LONG (WINAPI *fpCloseKey)(_In_ HANDLE hcKey, _In_ HANDLE hSpooler);

  LONG
  (WINAPI *fpDeleteKey)(
    _In_ HANDLE hcKey,
    _In_ LPCWSTR pszSubKey,
    _In_ HANDLE hSpooler);

  LONG
  (WINAPI *fpEnumKey)(
    _In_ HANDLE hcKey,
    _In_ DWORD dwIndex,
    _Inout_updates_to_(*pcchName, *pcchName) LPWSTR pszName,
    _Inout_ PDWORD pcchName,
    _Out_opt_ PFILETIME pftLastWriteTime,
    _In_ HANDLE hSpooler);

  LONG
  (WINAPI *fpQueryInfoKey)(
    _In_ HANDLE hcKey,
    _Out_opt_ PDWORD pcSubKeys,
    _Out_opt_ PDWORD pcbKey,
    _Out_opt_ PDWORD pcValues,
    _Out_opt_ PDWORD pcbValue,
    _Out_opt_ PDWORD pcbData,
    _Out_opt_ PDWORD pcbSecurityDescriptor,
    _Out_opt_ PFILETIME pftLastWriteTime,
    _In_ HANDLE hSpooler);

  LONG
  (WINAPI *fpSetValue)(
    _In_ HANDLE hcKey,
    _In_ LPCWSTR pszValue,
    _In_ DWORD dwType,
    _In_reads_bytes_(cbData) const BYTE *pData,
    _In_ DWORD cbData,
    _In_ HANDLE hSpooler);

  LONG
  (WINAPI *fpDeleteValue)(
    _In_ HANDLE hcKey,
    _In_ LPCWSTR pszValue,
    _In_ HANDLE hSpooler);

  LONG
  (WINAPI *fpEnumValue)(
    _In_ HANDLE hcKey,
    _In_ DWORD dwIndex,
    _Inout_updates_to_(*pcbValue, *pcbValue) LPWSTR pszValue,
    _Inout_ PDWORD pcbValue,
    _Out_opt_ PDWORD pType,
    _Out_writes_bytes_to_opt_(*pcbData, *pcbData) PBYTE pData,
    _Inout_ PDWORD pcbData,
    _In_ HANDLE hSpooler);

  LONG
  (WINAPI *fpQueryValue)(
    _In_ HANDLE hcKey,
    _In_ LPCWSTR pszValue,
    _Out_opt_ PDWORD pType,
    _Out_writes_bytes_to_opt_(*pcbData, *pcbData) PBYTE pData,
    _Inout_ PDWORD pcbData,
    _In_ HANDLE hSpooler);

} MONITORREG, *PMONITORREG;

typedef struct _MONITORINIT {
  DWORD cbSize;
  HANDLE hSpooler;
  HKEYMONITOR hckRegistryRoot;
  PMONITORREG pMonitorReg;
  BOOL bLocal;
  LPCWSTR pszServerName;
} MONITORINIT, *PMONITORINIT;

typedef struct _MONITOR {

  BOOL
  (WINAPI *pfnEnumPorts)(
    _In_opt_ LPWSTR pName,
    _In_ DWORD Level,
    _Out_writes_bytes_to_opt_(cbBuf, *pcbNeeded) LPBYTE pPorts,
    _In_ DWORD cbBuf,
    _Out_ LPDWORD pcbNeeded,
    _Out_ LPDWORD pcReturned);

  BOOL (WINAPI *pfnOpenPort)(_In_ LPWSTR pName, _Out_ PHANDLE pHandle);

  BOOL
  (WINAPI *pfnOpenPortEx)(
    _In_ LPWSTR pPortName,
    _In_ LPWSTR pPrinterName,
    _Out_ PHANDLE pHandle,
    _In_ struct _MONITOR *pMonitor);

  BOOL
  (WINAPI *pfnStartDocPort)(
    _In_ HANDLE hPort,
    _In_ LPWSTR pPrinterName,
    _In_ DWORD JobId,
    _In_ DWORD Level,
    _In_reads_(_Inexpressible_(0)) LPBYTE pDocInfo);

  BOOL
  (WINAPI *pfnWritePort)(
    _In_ HANDLE hPort,
    _In_reads_bytes_(cbBuf) LPBYTE pBuffer,
    _In_ DWORD cbBuf,
    _Out_ LPDWORD pcbWritten);

  BOOL
  (WINAPI *pfnReadPort)(
    _In_ HANDLE hPort,
    _Out_writes_bytes_to_(cbBuffer, *pcbRead) LPBYTE pBuffer,
    _In_ DWORD cbBuffer,
    _Out_ LPDWORD pcbRead);

  BOOL (WINAPI *pfnEndDocPort)(_In_ HANDLE hPort);

  BOOL (WINAPI *pfnClosePort)(_In_ HANDLE hPort);

  BOOL
  (WINAPI *pfnAddPort)(
    _In_ LPWSTR pName,
    _In_ HWND hWnd,
    _In_ LPWSTR pMonitorName);

  BOOL
  (WINAPI *pfnAddPortEx)(
    _In_ LPWSTR pName,
    _In_ DWORD Level,
    _In_reads_(_Inexpressible_(0)) LPBYTE lpBuffer,
    _In_ LPWSTR lpMonitorName);

  BOOL
  (WINAPI *pfnConfigurePort)(
    _In_ LPWSTR pName,
    _In_ HWND hWnd,
    _In_ LPWSTR pPortName);

  BOOL
  (WINAPI *pfnDeletePort)(
    _In_ LPWSTR pName,
    _In_ HWND hWnd,
    _In_ LPWSTR pPortName);

  BOOL
  (WINAPI *pfnGetPrinterDataFromPort)(
    _In_ HANDLE hPort,
    _In_ DWORD ControlID,
    _In_ LPWSTR pValueName,
    _In_reads_bytes_(cbInBuffer) LPWSTR lpInBuffer,
    _In_ DWORD cbInBuffer,
    _Out_writes_bytes_to_opt_(cbOutBuffer, *lpcbReturned) LPWSTR lpOutBuffer,
    _In_ DWORD cbOutBuffer,
    _Out_ LPDWORD lpcbReturned);

  BOOL
  (WINAPI *pfnSetPortTimeOuts)(
    _In_ HANDLE hPort,
    _In_ LPCOMMTIMEOUTS lpCTO,
    _In_ DWORD reserved);

  BOOL
  (WINAPI *pfnXcvOpenPort)(
    _In_ LPCWSTR pszObject,
    _In_ ACCESS_MASK GrantedAccess,
    _Out_ PHANDLE phXcv);

  DWORD
  (WINAPI *pfnXcvDataPort)(
    _In_ HANDLE hXcv,
    _In_ LPCWSTR pszDataName,
    _In_reads_bytes_(cbInputData) PBYTE pInputData,
    _In_ DWORD cbInputData,
    _Out_writes_bytes_to_opt_(cbOutputData, *pcbOutputNeeded) PBYTE pOutputData,
    _In_ DWORD cbOutputData,
    _Out_ PDWORD pcbOutputNeeded);

  BOOL (WINAPI *pfnXcvClosePort)(_In_ HANDLE hXcv);

} MONITOR, *LPMONITOR;

typedef struct _MONITOREX {
  DWORD dwMonitorSize;
  MONITOR Monitor;
} MONITOREX, *LPMONITOREX;

typedef struct _MONITOR2 {
  DWORD cbSize;

  BOOL
  (WINAPI *pfnEnumPorts)(
    _In_ HANDLE hMonitor,
    _In_opt_ LPWSTR pName,
    _In_ DWORD Level,
    _Out_writes_bytes_to_opt_(cbBuf, *pcbNeeded) LPBYTE pPorts,
    _In_ DWORD cbBuf,
    _Out_ LPDWORD pcbNeeded,
    _Out_ LPDWORD pcReturned);

  BOOL
  (WINAPI *pfnOpenPort)(
    _In_ HANDLE hMonitor,
    _In_ LPWSTR pName,
    _Out_ PHANDLE pHandle);

  BOOL
  (WINAPI *pfnOpenPortEx)(
    _In_ HANDLE hMonitor,
    _In_ HANDLE hMonitorPort,
    _In_ LPWSTR pPortName,
    _In_ LPWSTR pPrinterName,
    _Out_ PHANDLE pHandle,
    _In_ struct _MONITOR2 *pMonitor2);

  BOOL
  (WINAPI *pfnStartDocPort)(
    _In_ HANDLE hPort,
    _In_ LPWSTR pPrinterName,
    _In_ DWORD JobId,
    _In_ DWORD Level,
    _In_reads_(_Inexpressible_(0)) LPBYTE pDocInfo);

  BOOL
  (WINAPI *pfnWritePort)(
    _In_ HANDLE hPort,
    _In_reads_bytes_(cbBuf) LPBYTE pBuffer,
    _In_ DWORD cbBuf,
    _Out_ LPDWORD pcbWritten);

  BOOL
  (WINAPI *pfnReadPort)(
    _In_ HANDLE hPort,
    _Out_writes_bytes_to_opt_(cbBuffer, *pcbRead) LPBYTE pBuffer,
    _In_ DWORD cbBuffer,
    _Out_ LPDWORD pcbRead);

  BOOL (WINAPI *pfnEndDocPort)(_In_ HANDLE hPort);

  BOOL (WINAPI *pfnClosePort)(_In_ HANDLE hPort);

  BOOL
  (WINAPI *pfnAddPort)(
    _In_ HANDLE hMonitor,
    _In_ LPWSTR pName,
    _In_ HWND hWnd,
    _In_ LPWSTR pMonitorName);

  BOOL
  (WINAPI *pfnAddPortEx)(
    _In_ HANDLE hMonitor,
    _In_ LPWSTR pName,
    _In_ DWORD Level,
    _In_reads_(_Inexpressible_(0)) LPBYTE lpBuffer,
    _In_ LPWSTR lpMonitorName);

  BOOL
  (WINAPI *pfnConfigurePort)(
    _In_ HANDLE hMonitor,
    _In_ LPWSTR pName,
    _In_ HWND hWnd,
    _In_ LPWSTR pPortName);

  BOOL
  (WINAPI *pfnDeletePort)(
    _In_ HANDLE hMonitor,
    _In_ LPWSTR pName,
    _In_ HWND hWnd,
    _In_ LPWSTR pPortName);

  BOOL
  (WINAPI *pfnGetPrinterDataFromPort)(
    _In_ HANDLE hPort,
    _In_ DWORD ControlID,
    _In_ LPWSTR pValueName,
    _In_reads_bytes_(cbInBuffer) LPWSTR lpInBuffer,
    _In_ DWORD cbInBuffer,
    _Out_writes_bytes_to_opt_(cbOutBuffer, *lpcbReturned) LPWSTR lpOutBuffer,
    _In_ DWORD cbOutBuffer,
    _Out_ LPDWORD lpcbReturned);

  BOOL
  (WINAPI *pfnSetPortTimeOuts)(
    _In_ HANDLE hPort,
    _In_ LPCOMMTIMEOUTS lpCTO,
    _In_ DWORD reserved);

  BOOL
  (WINAPI *pfnXcvOpenPort)(
    _In_ HANDLE hMonitor,
    _In_ LPCWSTR pszObject,
    _In_ ACCESS_MASK GrantedAccess,
    _Out_ PHANDLE phXcv);

  DWORD
  (WINAPI *pfnXcvDataPort)(
    _In_ HANDLE hXcv,
    _In_ LPCWSTR pszDataName,
    _In_reads_bytes_(cbInputData) PBYTE pInputData,
    _In_ DWORD cbInputData,
    _Out_writes_bytes_to_opt_(cbOutputData, *pcbOutputNeeded) PBYTE pOutputData,
    _In_ DWORD cbOutputData,
    _Out_ PDWORD pcbOutputNeeded);

  BOOL (WINAPI *pfnXcvClosePort)(_In_ HANDLE hXcv);

  VOID (WINAPI *pfnShutdown)(_In_ HANDLE hMonitor);

#if (NTDDI_VERSION >= NTDDI_WINXP)
  DWORD
  (WINAPI *pfnSendRecvBidiDataFromPort)(
    _In_ HANDLE hPort,
    _In_ DWORD dwAccessBit,
    _In_ LPCWSTR pAction,
    _In_ PBIDI_REQUEST_CONTAINER pReqData,
    _Outptr_ PBIDI_RESPONSE_CONTAINER *ppResData);
#endif

#if (NTDDI_VERSION >= NTDDI_WIN7)

  DWORD
  (WINAPI *pfnNotifyUsedPorts)(
    _In_ HANDLE hMonitor,
    _In_ DWORD cPorts,
    _In_reads_(cPorts) PCWSTR *ppszPorts);

  DWORD
  (WINAPI *pfnNotifyUnusedPorts)(
    _In_ HANDLE hMonitor,
    _In_ DWORD cPorts,
    _In_reads_(cPorts) PCWSTR *ppszPorts);

#endif

} MONITOR2, *LPMONITOR2, *PMONITOR2;

typedef struct _MONITORUI {
  DWORD dwMonitorUISize;

  BOOL
  (WINAPI *pfnAddPortUI)(
    _At_(return, _Success_(return != 0)) _In_opt_ PCWSTR pszServer,
    _In_ HWND hWnd,
    _In_ PCWSTR pszPortNameIn,
    _Out_opt_ PWSTR *ppszPortNameOut);

  BOOL
  (WINAPI *pfnConfigurePortUI)(
    _In_opt_ PCWSTR pName,
    _In_ HWND hWnd,
    _In_ PCWSTR pPortName);

  BOOL
  (WINAPI *pfnDeletePortUI)(
    _In_opt_ PCWSTR pszServer,
    _In_ HWND hWnd,
    _In_ PCWSTR pszPortName);

} MONITORUI, *PMONITORUI;

#if (NTDDI_VERSION >= NTDDI_WINXP)

typedef enum {
  kMessageBox = 0
} UI_TYPE;

typedef struct {
  DWORD cbSize;
  LPWSTR pTitle;
  LPWSTR pMessage;
  DWORD Style;
  DWORD dwTimeout;
  BOOL bWait;
} MESSAGEBOX_PARAMS, *PMESSAGEBOX_PARAMS;

typedef struct {
  UI_TYPE UIType;
  MESSAGEBOX_PARAMS MessageBoxParams;
} SHOWUIPARAMS, *PSHOWUIPARAMS;

#endif /* (NTDDI_VERSION >= NTDDI_WINXP) */

#if (NTDDI_VERSION >= NTDDI_WS03)
#ifndef __ATTRIBUTE_INFO_3__
#define __ATTRIBUTE_INFO_3__
typedef struct _ATTRIBUTE_INFO_3 {
  DWORD dwJobNumberOfPagesPerSide;
  DWORD dwDrvNumberOfPagesPerSide;
  DWORD dwNupBorderFlags;
  DWORD dwJobPageOrderFlags;
  DWORD dwDrvPageOrderFlags;
  DWORD dwJobNumberOfCopies;
  DWORD dwDrvNumberOfCopies;
  DWORD dwColorOptimization;
  short dmPrintQuality;
  short dmYResolution;
} ATTRIBUTE_INFO_3, *PATTRIBUTE_INFO_3;
#endif /* __ATTRIBUTE_INFO_3__ */
#endif /* (NTDDI_VERSION >= NTDDI_WS03) */

#if (NTDDI_VERSION >= NTDDI_VISTA)

typedef BOOL
(CALLBACK *ROUTER_NOTIFY_CALLBACK)(
  _In_ DWORD dwCommand,
  _In_ PVOID pContext,
  _In_ DWORD dwColor,
  _In_ PPRINTER_NOTIFY_INFO pNofityInfo,
  _In_ DWORD fdwFlags,
  _Out_ PDWORD pdwResult);

typedef enum _NOTIFICATION_CALLBACK_COMMANDS {
  NOTIFICATION_COMMAND_NOTIFY,
  NOTIFICATION_COMMAND_CONTEXT_ACQUIRE,
  NOTIFICATION_COMMAND_CONTEXT_RELEASE
} NOTIFICATION_CALLBACK_COMMANDS;

typedef struct _NOTIFICATION_CONFIG_1 {
  UINT cbSize;
  DWORD fdwFlags;
  ROUTER_NOTIFY_CALLBACK pfnNotifyCallback;
  PVOID pContext;
} NOTIFICATION_CONFIG_1, *PNOTIFICATION_CONFIG_1;

typedef enum _NOTIFICATION_CONFIG_FLAGS {
  NOTIFICATION_CONFIG_CREATE_EVENT = 1 << 0,
  NOTIFICATION_CONFIG_REGISTER_CALLBACK = 1 << 1,
  NOTIFICATION_CONFIG_EVENT_TRIGGER = 1 << 2,
  NOTIFICATION_CONFIG_ASYNC_CHANNEL = 1 << 3
} NOTIFICATION_CONFIG_FLAGS;

typedef struct _SPLCLIENT_INFO_3 {
  UINT cbSize;
  DWORD dwFlags;
  DWORD dwSize;
  PWSTR pMachineName;
  PWSTR pUserName;
  DWORD dwBuildNum;
  DWORD dwMajorVersion;
  DWORD dwMinorVersion;
  WORD wProcessorArchitecture;
  UINT64 hSplPrinter;
} SPLCLIENT_INFO_3, *PSPLCLIENT_INFO_3, *LPSPLCLIENT_INFO_3;

#ifndef __ATTRIBUTE_INFO_4__
#define __ATTRIBUTE_INFO_4__

typedef struct _ATTRIBUTE_INFO_4 {
  DWORD dwJobNumberOfPagesPerSide;
  DWORD dwDrvNumberOfPagesPerSide;
  DWORD dwNupBorderFlags;
  DWORD dwJobPageOrderFlags;
  DWORD dwDrvPageOrderFlags;
  DWORD dwJobNumberOfCopies;
  DWORD dwDrvNumberOfCopies;
  DWORD dwColorOptimization;
  short dmPrintQuality;
  short dmYResolution;
  DWORD dwDuplexFlags;
  DWORD dwNupDirection;
  DWORD dwBookletFlags;
  DWORD dwScalingPercentX;
  DWORD dwScalingPercentY;
} ATTRIBUTE_INFO_4, *PATTRIBUTE_INFO_4;

#define REVERSE_PAGES_FOR_REVERSE_DUPLEX (0x00000001)
#define DONT_SEND_EXTRA_PAGES_FOR_DUPLEX (0x00000001 << 1)

#define RIGHT_THEN_DOWN                  (0x00000001)
#define DOWN_THEN_RIGHT                  (0x00000001 << 1)
#define LEFT_THEN_DOWN                   (0x00000001 << 2)
#define DOWN_THEN_LEFT                   (0x00000001 << 3)

#define BOOKLET_EDGE_LEFT                0x00000000
#define BOOKLET_EDGE_RIGHT               0x00000001

#endif /* __ATTRIBUTE_INFO_4__ */

#endif /* (NTDDI_VERSION >= NTDDI_VISTA) */

#if (OSVER(NTDDI_VERSION) == NTDDI_W2K)
typedef SPLCLIENT_INFO_2_W2K SPLCLIENT_INFO_2, *PSPLCLIENT_INFO_2, *LPSPLCLIENT_INFO_2;
#elif ((OSVER(NTDDI_VERSION) == NTDDI_WINXP) || (OSVER(NTDDI_VERSION) == NTDDI_WS03))
typedef SPLCLIENT_INFO_2_WINXP SPLCLIENT_INFO_2, *PSPLCLIENT_INFO_2, *LPSPLCLIENT_INFO_2;
#else
typedef SPLCLIENT_INFO_2_LONGHORN SPLCLIENT_INFO_2, *PSPLCLIENT_INFO_2, *LPSPLCLIENT_INFO_2;
#endif /* (OSVER(NTDDI_VERSION) == NTDDI_W2K) */

BOOL
WINAPI
InitializePrintProvidor(
  _Out_writes_bytes_(cbPrintProvidor) LPPRINTPROVIDOR pPrintProvidor,
  _In_ DWORD cbPrintProvidor,
  _In_opt_ LPWSTR pFullRegistryPath);

HANDLE
WINAPI
OpenPrintProcessor(
  _In_ LPWSTR pPrinterName,
  _In_ PPRINTPROCESSOROPENDATA pPrintProcessorOpenData);

BOOL
WINAPI
PrintDocumentOnPrintProcessor(
  _In_ HANDLE hPrintProcessor,
  _In_ LPWSTR pDocumentName);

BOOL
WINAPI
ClosePrintProcessor(
  _Inout_ HANDLE hPrintProcessor);

BOOL
WINAPI
ControlPrintProcessor(
  _In_ HANDLE hPrintProcessor,
  _In_ DWORD Command);

DWORD
WINAPI
GetPrintProcessorCapabilities(
  _In_ LPTSTR pValueName,
  _In_ DWORD dwAttributes,
  _Out_writes_bytes_(nSize) LPBYTE pData,
  _In_ DWORD nSize,
  _Out_ LPDWORD pcbNeeded);

BOOL
WINAPI
InitializeMonitor(
  _In_ LPWSTR pRegistryRoot);

BOOL
WINAPI
OpenPort(
  _In_ LPWSTR pName,
  _Out_ PHANDLE pHandle);

BOOL
WINAPI
WritePort(
  _In_ HANDLE hPort,
  _In_reads_bytes_(cbBuf) LPBYTE pBuffer,
  _In_ DWORD cbBuf,
  _Out_ LPDWORD pcbWritten);

BOOL
WINAPI
ReadPort(
  _In_ HANDLE hPort,
  _Out_writes_bytes_(cbBuffer) LPBYTE pBuffer,
  _In_ DWORD cbBuffer,
  _Out_ LPDWORD pcbRead);

BOOL
WINAPI
ClosePort(
  _In_ HANDLE hPort);

BOOL
WINAPI
XcvOpenPort(
  _In_ LPCWSTR pszObject,
  _In_ ACCESS_MASK GrantedAccess,
  _Out_ PHANDLE phXcv);

DWORD
WINAPI
XcvDataPort(
  _In_ HANDLE hXcv,
  _In_ LPCWSTR pszDataName,
  _In_reads_bytes_(cbInputData) PBYTE pInputData,
  _In_ DWORD cbInputData,
  _Out_writes_bytes_(cbOutputData) PBYTE pOutputData,
  _In_ DWORD cbOutputData,
  _Out_ PDWORD pcbOutputNeeded);

BOOL
WINAPI
XcvClosePort(
  _In_ HANDLE hXcv);

_Success_(return != 0)
BOOL
WINAPI
AddPortUI(
  _In_opt_ PCWSTR pszServer,
  _In_ HWND hWnd,
  _In_ PCWSTR pszMonitorNameIn,
  _Out_opt_ PWSTR *ppszPortNameOut);

BOOL
WINAPI
ConfigurePortUI(
  _In_ PCWSTR pszServer,
  _In_ HWND hWnd,
  _In_ PCWSTR pszPortName);

BOOL
WINAPI
DeletePortUI(
  _In_ PCWSTR pszServer,
  _In_ HWND hWnd,
  _In_ PCWSTR pszPortName);

BOOL
WINAPI
SplDeleteSpoolerPortStart(
  _In_ PCWSTR pPortName);

BOOL
WINAPI
SplDeleteSpoolerPortEnd(
  _In_ PCWSTR pName,
  _In_ BOOL bDeletePort);

BOOL
WINAPI
SpoolerCopyFileEvent(
  _In_ LPWSTR pszPrinterName,
  _In_ LPWSTR pszKey,
  _In_ DWORD dwCopyFileEvent);

DWORD
WINAPI
GenerateCopyFilePaths(
  _In_ LPCWSTR pszPrinterName,
  _In_ LPCWSTR pszDirectory,
  _In_ LPBYTE pSplClientInfo,
  _In_ DWORD dwLevel,
  _Inout_updates_(*pcchSourceDirSize) LPWSTR pszSourceDir,
  _Inout_ LPDWORD pcchSourceDirSize,
  _Inout_updates_(*pcchTargetDirSize) LPWSTR pszTargetDir,
  _Inout_ LPDWORD pcchTargetDirSize,
  _In_ DWORD dwFlags);

HANDLE
WINAPI
CreatePrinterIC(
  _In_ HANDLE hPrinter,
  _In_opt_ LPDEVMODEW pDevMode);

BOOL
WINAPI
PlayGdiScriptOnPrinterIC(
  _In_ HANDLE hPrinterIC,
  _In_reads_bytes_(cIn) LPBYTE pIn,
  _In_ DWORD cIn,
  _Out_writes_bytes_(cOut) LPBYTE pOut,
  _In_ DWORD cOut,
  _In_ DWORD ul);

BOOL WINAPI DeletePrinterIC(_In_ HANDLE hPrinterIC);

BOOL
WINAPI
DevQueryPrint(
  _In_ HANDLE hPrinter,
  _In_ LPDEVMODEW pDevMode,
  _Out_ DWORD *pResID);

HANDLE WINAPI RevertToPrinterSelf(VOID);
BOOL WINAPI ImpersonatePrinterClient(_In_ HANDLE hToken);

BOOL
WINAPI
ReplyPrinterChangeNotification(
  _In_ HANDLE hNotify,
  _In_ DWORD fdwFlags,
  _Out_opt_ PDWORD pdwResult,
  _In_opt_ PVOID pPrinterNotifyInfo);

BOOL
WINAPI
ReplyPrinterChangeNotificationEx(
  _In_ HANDLE hNotify,
  _In_ DWORD dwColor,
  _In_ DWORD fdwFlags,
  _Out_ PDWORD pdwResult,
  _In_ PVOID pPrinterNotifyInfo);

BOOL
WINAPI
PartialReplyPrinterChangeNotification(
  _In_ HANDLE hNotify,
  _In_opt_ PPRINTER_NOTIFY_INFO_DATA pInfoDataSrc);

PPRINTER_NOTIFY_INFO
WINAPI
RouterAllocPrinterNotifyInfo(
  _In_ DWORD cPrinterNotifyInfoData);

BOOL WINAPI RouterFreePrinterNotifyInfo(_In_opt_ PPRINTER_NOTIFY_INFO pInfo);

BOOL
WINAPI
AppendPrinterNotifyInfoData(
  _In_ PPRINTER_NOTIFY_INFO pInfoDest,
  _In_opt_ PPRINTER_NOTIFY_INFO_DATA pInfoDataSrc,
  _In_ DWORD fdwFlags);

DWORD
WINAPI
CallRouterFindFirstPrinterChangeNotification(
  _In_ HANDLE hPrinter,
  _In_ DWORD fdwFlags,
  _In_ DWORD fdwOptions,
  _In_ HANDLE hNotify,
  _In_ PPRINTER_NOTIFY_OPTIONS pPrinterNotifyOptions);

BOOL
WINAPI
ProvidorFindFirstPrinterChangeNotification(
  _In_ HANDLE hPrinter,
  _In_ DWORD fdwFlags,
  _In_ DWORD fdwOptions,
  _In_ HANDLE hNotify,
  _In_opt_ PVOID pvReserved0,
  _Out_opt_ PVOID pvReserved1);

BOOL WINAPI ProvidorFindClosePrinterChangeNotification(_In_ HANDLE hPrinter);

/* Spooler */
BOOL WINAPI SpoolerFindFirstPrinterChangeNotification(HANDLE hPrinter,
                                                      DWORD fdwFlags,
                                                      DWORD fdwOptions,
                                                      PHANDLE phEvent,
                                                      PVOID pPrinterNotifyOptions,
                                                      PVOID pvReserved);

BOOL
WINAPI
SpoolerFindNextPrinterChangeNotification(
  _In_ HANDLE hPrinter,
  _Out_ LPDWORD pfdwChange,
  _In_opt_ LPVOID pPrinterNotifyOptions,
  _Inout_opt_ LPVOID *ppPrinterNotifyInfo);

VOID WINAPI SpoolerFreePrinterNotifyInfo(_In_ PPRINTER_NOTIFY_INFO pInfo);
BOOL WINAPI SpoolerFindClosePrinterChangeNotification(_In_ HANDLE hPrinter);

/* Port monitor / Language monitor / Print monitor */

LPMONITOR2
WINAPI
InitializePrintMonitor2(
  _In_ PMONITORINIT pMonitorInit,
  _Out_ PHANDLE phMonitor);

PMONITORUI WINAPI InitializePrintMonitorUI(VOID);
LPMONITOREX WINAPI InitializePrintMonitor(_In_ LPWSTR pRegistryRoot);

BOOL
WINAPI
InitializeMonitorEx(
  _In_ LPWSTR pRegistryRoot,
  _Out_ LPMONITOR pMonitor);

#if (NTDDI_VERSION >= NTDDI_WINXP)

PBIDI_RESPONSE_CONTAINER
WINAPI
RouterAllocBidiResponseContainer(
  _In_ DWORD Count);

PVOID WINAPI RouterAllocBidiMem(_In_ size_t NumBytes);

DWORD
WINAPI
RouterFreeBidiResponseContainer(
  _In_ PBIDI_RESPONSE_CONTAINER pData);

VOID WINAPI RouterFreeBidiMem(_In_ PVOID pMemPointer);

BOOL
WINAPI
SplPromptUIInUsersSession(
  _In_ HANDLE hPrinter,
  _In_ DWORD JobId,
  _In_ PSHOWUIPARAMS pUIParams,
  _Out_ DWORD *pResponse);

DWORD
WINAPI
SplIsSessionZero(
  _In_ HANDLE hPrinter,
  _In_ DWORD JobId,
  _Out_ BOOL *pIsSessionZero);

#endif /* (NTDDI_VERSION >= NTDDI_WINXP) */

#if (NTDDI_VERSION >= NTDDI_WS03)
BOOL
WINAPI
GetJobAttributes(
  _In_ LPWSTR pPrinterName,
  _In_ LPDEVMODEW pDevmode,
  _Out_ PATTRIBUTE_INFO_3 pAttributeInfo);
#endif

#if (NTDDI_VERSION >= NTDDI_VISTA)

#define FILL_WITH_DEFAULTS   0x1

BOOL
WINAPI
GetJobAttributesEx(
  _In_ LPWSTR pPrinterName,
  _In_ LPDEVMODEW pDevmode,
  _In_ DWORD dwLevel,
  _Out_writes_bytes_(nSize) LPBYTE pAttributeInfo,
  _In_ DWORD nSize,
  _In_ DWORD dwFlags);

BOOL
WINAPI
SpoolerRefreshPrinterChangeNotification(
  _In_ HANDLE hPrinter,
  _In_ DWORD dwColor,
  _In_ PPRINTER_NOTIFY_OPTIONS pOptions,
  _Inout_opt_ PPRINTER_NOTIFY_INFO *ppInfo);

#endif /* (NTDDI_VERSION >= NTDDI_VISTA) */

/* FIXME : The following declarations are not present in the official header */

BOOL WINAPI OpenPrinterToken(PHANDLE phToken);
BOOL WINAPI SetPrinterToken(HANDLE hToken);
BOOL WINAPI ClosePrinterToken(HANDLE hToken);
BOOL WINAPI InstallPrintProcessor(HWND hWnd);

#ifdef __cplusplus
}
#endif

#endif /* _WINSPLP_ */
