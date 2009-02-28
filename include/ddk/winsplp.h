/*
 * Definitions for print provider, monitor, processor and spooler
 *
 * Copyright 2005 Detlef Riekenberg
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 *
 *
 * "providor" is not a spelling error in this file. It's the real name.
 *
 */

#ifndef _WINSPLP_
#define _WINSPLP_

#ifdef __cplusplus
extern "C" {
#endif

/* DEFINES */

#define PRINTER_NOTIFY_STATUS_ENDPOINT  1
#define PRINTER_NOTIFY_STATUS_POLL      2
#define PRINTER_NOTIFY_STATUS_INFO      4


#define ROUTER_UNKNOWN      0
#define ROUTER_SUCCESS      1
#define ROUTER_STOP_ROUTING 2

/*
 * WARNING: Many Functions are declared as "BOOL", but return ROUTER_*
 */


/* TYPES */

typedef struct _MONITOR {
 BOOL  (WINAPI *pfnEnumPorts)(LPWSTR pName, DWORD Level, LPBYTE pPorts,
                DWORD cbBuf, LPDWORD pcbNeeded, LPDWORD pcReturned);
 BOOL  (WINAPI *pfnOpenPort)(LPWSTR pName, PHANDLE pHandle);
 BOOL  (WINAPI *pfnOpenPortEx)(LPWSTR pPortName, LPWSTR pPrinterName,
                PHANDLE pHandle, struct _MONITOR *pMonitor);
 BOOL  (WINAPI *pfnStartDocPort)(HANDLE hPort, LPWSTR pPrinterName,
                DWORD JobId, DWORD Level, LPBYTE pDocInfo);
 BOOL  (WINAPI *pfnWritePort)(HANDLE hPort, LPBYTE pBuffer, DWORD cbBuf,
                LPDWORD pcbWritten);
 BOOL  (WINAPI *pfnReadPort)(HANDLE hPort, LPBYTE pBuffer, DWORD cbBuffer,
                LPDWORD pcbRead);
 BOOL  (WINAPI *pfnEndDocPort)(HANDLE hPort);
 BOOL  (WINAPI *pfnClosePort)(HANDLE hPort);
 BOOL  (WINAPI *pfnAddPort)(LPWSTR pName, HWND hWnd, LPWSTR pMonitorName);
 BOOL  (WINAPI *pfnAddPortEx)(LPWSTR pName, DWORD Level, LPBYTE lpBuffer,
                LPWSTR lpMonitorName);
 BOOL  (WINAPI *pfnConfigurePort)(LPWSTR pName, HWND hWnd, LPWSTR pPortName);
 BOOL  (WINAPI *pfnDeletePort)(LPWSTR pName, HWND hWnd, LPWSTR pPortName);
 BOOL  (WINAPI *pfnGetPrinterDataFromPort)(HANDLE hPort, DWORD ControlID,
                LPWSTR pValueName, LPWSTR lpInBuffer, DWORD cbInBuffer,
                LPWSTR lpOutBuffer, DWORD cbOutBuffer, LPDWORD lpcbReturned);
 BOOL  (WINAPI *pfnSetPortTimeOuts)(HANDLE hPort, LPCOMMTIMEOUTS lpCTO,
                DWORD reserved);
 BOOL  (WINAPI *pfnXcvOpenPort)(LPCWSTR pszObject, ACCESS_MASK GrantedAccess, PHANDLE phXcv);
 DWORD (WINAPI *pfnXcvDataPort)(HANDLE hXcv, LPCWSTR pszDataName,
                PBYTE pInputData, DWORD cbInputData,
                PBYTE pOutputData, DWORD cbOutputData, PDWORD pcbOutputNeeded);
 BOOL  (WINAPI *pfnXcvClosePort)(HANDLE hXcv);
} MONITOR, *LPMONITOR;

typedef struct _MONITOR2 {
 DWORD cbSize;
 BOOL  (WINAPI *pfnEnumPorts)(LPWSTR pName, DWORD Level, LPBYTE pPorts,
                DWORD cbBuf, LPDWORD pcbNeeded, LPDWORD pcReturned);
 BOOL  (WINAPI *pfnOpenPort)(LPWSTR pName, PHANDLE pHandle);
 BOOL  (WINAPI *pfnOpenPortEx)(LPWSTR pPortName, LPWSTR pPrinterName,
                PHANDLE pHandle, struct _MONITOR2 *pMonitor2);
 BOOL  (WINAPI *pfnStartDocPort)(HANDLE hPort, LPWSTR pPrinterName,
                DWORD JobId, DWORD Level, LPBYTE pDocInfo);
 BOOL  (WINAPI *pfnWritePort)(HANDLE hPort, LPBYTE pBuffer, DWORD cbBuf,
                LPDWORD pcbWritten);
 BOOL  (WINAPI *pfnReadPort)(HANDLE hPort, LPBYTE pBuffer, DWORD cbBuffer,
                LPDWORD pcbRead);
 BOOL  (WINAPI *pfnEndDocPort)(HANDLE hPort);
 BOOL  (WINAPI *pfnClosePort)(HANDLE hPort);
 BOOL  (WINAPI *pfnAddPort)(LPWSTR pName, HWND hWnd, LPWSTR pMonitorName);
 BOOL  (WINAPI *pfnAddPortEx)(LPWSTR pName, DWORD Level, LPBYTE lpBuffer,
                LPWSTR lpMonitorName);
 BOOL  (WINAPI *pfnConfigurePort)(LPWSTR pName, HWND hWnd, LPWSTR pPortName);
 BOOL  (WINAPI *pfnDeletePort)(LPWSTR pName, HWND hWnd, LPWSTR pPortName);
 BOOL  (WINAPI *pfnGetPrinterDataFromPort)(HANDLE hPort, DWORD ControlID,
                LPWSTR pValueName, LPWSTR lpInBuffer, DWORD cbInBuffer,
                LPWSTR lpOutBuffer, DWORD cbOutBuffer, LPDWORD lpcbReturned);
 BOOL  (WINAPI *pfnSetPortTimeOuts)(HANDLE hPort, LPCOMMTIMEOUTS lpCTO,
                DWORD reserved);
 BOOL  (WINAPI *pfnXcvOpenPort)(HANDLE hMonitor, LPCWSTR pszObject,
                ACCESS_MASK GrantedAccess, PHANDLE phXcv);
 DWORD (WINAPI *pfnXcvDataPort)(HANDLE hXcv, LPCWSTR pszDataName,
                PBYTE pInputData, DWORD cbInputData,
                PBYTE pOutputData, DWORD cbOutputData, PDWORD pcbOutputNeeded);
 BOOL  (WINAPI *pfnXcvClosePort)(HANDLE hXcv);
 /* Additions from MONITOR to MONITOR2 */
 VOID  (WINAPI *pfnShutdown)(HANDLE hMonitor);
 DWORD (WINAPI *pfnSendRecvBidiDataFromPort)(HANDLE hPort, DWORD dwAccessBit,
                LPCWSTR pAction, PBIDI_REQUEST_CONTAINER pReqData,
                PBIDI_RESPONSE_CONTAINER *ppResData);
} MONITOR2, *LPMONITOR2, *PMONITOR2;

typedef struct _MONITOREX {
 DWORD      dwMonitorSize;
 MONITOR    Monitor;
} MONITOREX, *LPMONITOREX;

typedef struct _MONITORREG {
 DWORD cbSize;
 LONG  (WINAPI *fpCreateKey)(HANDLE hcKey, LPCWSTR pszSubKey, DWORD dwOptions,
                REGSAM samDesired, PSECURITY_ATTRIBUTES pSecurityAttributes,
                PHANDLE phckResult, PDWORD pdwDisposition, HANDLE hSpooler);
 LONG  (WINAPI *fpOpenKey)(HANDLE hcKey, LPCWSTR pszSubKey, REGSAM samDesired,
                PHANDLE phkResult, HANDLE hSpooler);
 LONG  (WINAPI *fpCloseKey)(HANDLE hcKey, HANDLE hSpooler);
 LONG  (WINAPI *fpDeleteKey)(HANDLE hcKey, LPCWSTR pszSubKey, HANDLE hSpooler);
 LONG  (WINAPI *fpEnumKey)(HANDLE hcKey, DWORD dwIndex, LPWSTR pszName,
                PDWORD pcchName, PFILETIME pftLastWriteTime, HANDLE hSpooler);
 LONG  (WINAPI *fpQueryInfoKey)(HANDLE hcKey, PDWORD pcSubKeys, PDWORD pcbKey,
                PDWORD pcValues, PDWORD pcbValue, PDWORD pcbData,
                PDWORD pcbSecurityDescriptor, PFILETIME pftLastWriteTime,
                HANDLE hSpooler);
 LONG  (WINAPI *fpSetValue)(HANDLE hcKey, LPCWSTR pszValue, DWORD dwType,
                const BYTE* pData, DWORD cbData, HANDLE hSpooler);
 LONG  (WINAPI *fpDeleteValue)(HANDLE hcKey, LPCWSTR pszValue, HANDLE hSpooler);
 LONG  (WINAPI *fpEnumValue)(HANDLE hcKey, DWORD dwIndex, LPWSTR pszValue,
                PDWORD pcbValue, PDWORD pType, PBYTE pData, PDWORD pcbData,
                HANDLE hSpooler);
 LONG  (WINAPI *fpQueryValue)(HANDLE hcKey, LPCWSTR pszValue, PDWORD pType,
                PBYTE pData, PDWORD pcbData, HANDLE hSpooler);
} MONITORREG, *PMONITORREG;

typedef struct _MONITORINIT {
 DWORD       cbSize;
 HANDLE      hSpooler;
 HANDLE      hckRegistryRoot;
 PMONITORREG pMonitorReg;
 BOOL        bLocal;
} MONITORINIT, *PMONITORINIT;

typedef struct _MONITORUI {
 DWORD dwMonitorUISize;
 BOOL  (WINAPI *pfnAddPortUI)(PCWSTR pszServer, HWND hWnd,
                PCWSTR pszPortNameIn, PWSTR *ppszPortNameOut);
 BOOL  (WINAPI *pfnConfigurePortUI)(PCWSTR pName, HWND hWnd, PCWSTR pPortName);
 BOOL  (WINAPI *pfnDeletePortUI)(PCWSTR pszServer, HWND hWnd, PCWSTR pszPortName);
}MONITORUI, *PMONITORUI;

typedef struct _PRINTER_NOTIFY_INIT {
 DWORD  Size;
 DWORD  Reserved;
 DWORD  PollTime;
} PRINTER_NOTIFY_INIT, *LPPRINTER_NOTIFY_INIT, *PPRINTER_NOTIFY_INIT;

typedef struct _PRINTPROCESSOROPENDATA {
 PDEVMODEW pDevMode;
 LPWSTR    pDatatype;
 LPWSTR    pParameters;
 LPWSTR    pDocumentName;
 DWORD     JobId;
 LPWSTR    pOutputFile;
 LPWSTR    pPrinterName;
} PRINTPROCESSOROPENDATA, *LPPRINTPROCESSOROPENDATA, *PPRINTPROCESSOROPENDATA;


/*
 * WARNING: Many Functions are declared as "BOOL", but return ROUTER_*
 */

typedef struct _PRINTPROVIDOR {
 BOOL   (WINAPI *fpOpenPrinter)(LPWSTR lpPrinterName, HANDLE *phPrinter,
                 LPPRINTER_DEFAULTSW pDefault);
 BOOL   (WINAPI *fpSetJob)(HANDLE hPrinter, DWORD JobID, DWORD Level,
                 LPBYTE pJob, DWORD Command);
 BOOL   (WINAPI *fpGetJob)(HANDLE hPrinter, DWORD JobID, DWORD Level,
                 LPBYTE pJob, DWORD cbBuf, LPDWORD pcbNeeded);
 BOOL   (WINAPI *fpEnumJobs)(HANDLE hPrinter, DWORD FirstJob, DWORD NoJobs,
                 DWORD Level, LPBYTE pJob, DWORD cbBuf, LPDWORD pcbNeeded,
                 LPDWORD pcReturned);
 HANDLE (WINAPI *fpAddPrinter)(LPWSTR pName, DWORD Level, LPBYTE pPrinter);
 BOOL   (WINAPI *fpDeletePrinter)(HANDLE hPrinter);
 BOOL   (WINAPI *fpSetPrinter)(HANDLE hPrinter, DWORD Level, LPBYTE pPrinter,
                 DWORD Command);
 BOOL   (WINAPI *fpGetPrinter)(HANDLE hPrinter, DWORD Level, LPBYTE pPrinter,
                 DWORD cbBuf, LPDWORD pcbNeeded);
 BOOL   (WINAPI *fpEnumPrinters)(DWORD dwType, LPWSTR lpszName, DWORD dwLevel,
                 LPBYTE lpbPrinters, DWORD cbBuf, LPDWORD lpdwNeeded,
                 LPDWORD lpdwReturned);
 BOOL   (WINAPI *fpAddPrinterDriver)(LPWSTR pName, DWORD Level,
                 LPBYTE pDriverInfo);
 BOOL   (WINAPI *fpEnumPrinterDrivers)(LPWSTR pName, LPWSTR pEnvironment,
                 DWORD Level, LPBYTE pDriverInfo, DWORD cbBuf,
                 LPDWORD pcbNeeded, LPDWORD pcbReturned);
 BOOL   (WINAPI *fpGetPrinterDriver)(HANDLE hPrinter, LPWSTR pEnvironment,
                 DWORD Level, LPBYTE pDriverInfo, DWORD cbBuf,
                 LPDWORD pcbNeeded);
 BOOL   (WINAPI *fpGetPrinterDriverDirectory)(LPWSTR pName, LPWSTR pEnvironment,
                 DWORD Level, LPBYTE pDriverDirectory, DWORD cbBuf,
                 LPDWORD pcbNeeded);
 BOOL   (WINAPI *fpDeletePrinterDriver)(LPWSTR pName, LPWSTR pEnvironment,
                 LPWSTR pDriverName);
 BOOL   (WINAPI *fpAddPrintProcessor)(LPWSTR pName, LPWSTR pEnvironment,
                 LPWSTR pPathName, LPWSTR pPrintProcessorName);
 BOOL   (WINAPI *fpEnumPrintProcessors)(LPWSTR pName, LPWSTR pEnvironment,
                 DWORD Level, LPBYTE pPrintProcessorInfo, DWORD cbBuf,
                 LPDWORD pcbNeeded, LPDWORD pcbReturned);
 BOOL   (WINAPI *fpGetPrintProcessorDirectory)(LPWSTR pName, LPWSTR pEnvironment,
                 DWORD Level, LPBYTE pPrintProcessorInfo, DWORD cbBuf,
                 LPDWORD pcbNeeded);
 BOOL   (WINAPI *fpDeletePrintProcessor)(LPWSTR pName, LPWSTR pEnvironment,
                 LPWSTR pPrintProcessorName);
 BOOL   (WINAPI *fpEnumPrintProcessorDatatypes)(LPWSTR pName,
                 LPWSTR pPrintProcessorName, DWORD Level, LPBYTE pDatatypes,
                 DWORD cbBuf, LPDWORD pcbNeeded, LPDWORD pcbReturned);
 DWORD  (WINAPI *fpStartDocPrinter)(HANDLE hPrinter, DWORD Level, LPBYTE pDocInfo);
 BOOL   (WINAPI *fpStartPagePrinter)(HANDLE hPrinter);
 BOOL   (WINAPI *fpWritePrinter)(HANDLE hPrinter, LPVOID pBuf, DWORD cbBuf,
                 LPDWORD pcWritten);
 BOOL   (WINAPI *fpEndPagePrinter)(HANDLE hPrinter);
 BOOL   (WINAPI *fpAbortPrinter)(HANDLE hPrinter);
 BOOL   (WINAPI *fpReadPrinter)(HANDLE hPrinter, LPVOID pBuf, DWORD cbBuf,
                 LPDWORD pNoBytesRead);
 BOOL   (WINAPI *fpEndDocPrinter)(HANDLE hPrinter);
 BOOL   (WINAPI *fpAddJob)(HANDLE hPrinter, DWORD Level, LPBYTE pData,
                 DWORD cbBuf, LPDWORD pcbNeeded);
 BOOL   (WINAPI *fpScheduleJob)(HANDLE hPrinter, DWORD JobID);
 DWORD  (WINAPI *fpGetPrinterData)(HANDLE hPrinter, LPWSTR pValueName,
                 LPDWORD pType, LPBYTE pData, DWORD nSize, LPDWORD pcbNeeded);
 DWORD  (WINAPI *fpSetPrinterData)(HANDLE hPrinter, LPWSTR pValueName,
                 DWORD Type, LPBYTE pData, DWORD cbData);
 DWORD  (WINAPI *fpWaitForPrinterChange)(HANDLE hPrinter, DWORD Flags);
 BOOL   (WINAPI *fpClosePrinter)(HANDLE phPrinter);
 BOOL   (WINAPI *fpAddForm)(HANDLE hPrinter, DWORD Level, LPBYTE pForm);
 BOOL   (WINAPI *fpDeleteForm)(HANDLE hPrinter, LPWSTR pFormName);
 BOOL   (WINAPI *fpGetForm)(HANDLE hPrinter, LPWSTR pFormName, DWORD Level,
                 LPBYTE pForm, DWORD cbBuf, LPDWORD pcbNeeded);
 BOOL   (WINAPI *fpSetForm)(HANDLE hPrinter, LPWSTR pFormName, DWORD Level,
                 LPBYTE pForm);
 BOOL   (WINAPI *fpEnumForms)(HANDLE hPrinter, DWORD Level, LPBYTE pForm,
                 DWORD cbBuf, LPDWORD pcbNeeded, LPDWORD pcReturned);
 BOOL   (WINAPI *fpEnumMonitors)(LPWSTR pName, DWORD Level, LPBYTE pMonitors,
                 DWORD cbBuf, LPDWORD pcbNeeded, LPDWORD pcReturned);
 BOOL   (WINAPI *fpEnumPorts)(LPWSTR pName, DWORD Level, LPBYTE pPorts,
                 DWORD cbBuf, LPDWORD pcbNeeded, LPDWORD pcReturned);
 BOOL   (WINAPI *fpAddPort)(LPWSTR pName, HWND hWnd, LPWSTR pMonitorName);
 BOOL   (WINAPI *fpConfigurePort)(LPWSTR pName, HWND hWnd, LPWSTR pPortName);
 BOOL   (WINAPI *fpDeletePort)(LPWSTR pName, HWND hWnd, LPWSTR pPortName);
 HANDLE (WINAPI *fpCreatePrinterIC)(HANDLE  hPrinter, LPDEVMODEW  pDevMode);
 BOOL   (WINAPI *fpPlayGdiScriptOnPrinterIC)(HANDLE  hPrinterIC, LPBYTE pIn,
                 DWORD cIn, LPBYTE pOut, DWORD cOut, DWORD  ul);
 BOOL   (WINAPI *fpDeletePrinterIC)(HANDLE hPrinterIC);
 BOOL   (WINAPI *fpAddPrinterConnection)(LPWSTR pName);
 BOOL   (WINAPI *fpDeletePrinterConnection)(LPWSTR pName);
 DWORD  (WINAPI *fpPrinterMessageBox)(HANDLE hPrinter, DWORD Error, HWND hWnd,
                 LPWSTR pText, LPWSTR pCaption, DWORD dwType);
 BOOL   (WINAPI *fpAddMonitor)(LPWSTR pName, DWORD Level, LPBYTE pMonitors);
 BOOL   (WINAPI *fpDeleteMonitor)(LPWSTR pName, LPWSTR pEnvironment,
                 LPWSTR pMonitorName);
 BOOL   (WINAPI *fpResetPrinter)(HANDLE hPrinter, LPPRINTER_DEFAULTSW pDefault);
 BOOL   (WINAPI *fpGetPrinterDriverEx)(HANDLE hPrinter, LPWSTR pEnvironment,
                 DWORD Level, LPBYTE pDriverInfo, DWORD cbBuf, LPDWORD pcbNeeded,
                 DWORD dwClientMajorVersion, DWORD dwClientMinorVersion,
                 PDWORD pdwServerMajorVersion, PDWORD pdwServerMinorVersion);
 HANDLE (WINAPI *fpFindFirstPrinterChangeNotification)(HANDLE hPrinter,
                 DWORD fdwFlags, DWORD fdwOptions, LPVOID pPrinterNotifyOptions);
 BOOL   (WINAPI *fpFindClosePrinterChangeNotification)(HANDLE hChange);
 BOOL   (WINAPI *fpAddPortEx)(HANDLE hMonitor, LPWSTR pName, DWORD Level,
                 LPBYTE lpBuffer, LPWSTR lpMonitorName);
 BOOL   (WINAPI *fpShutDown)(LPVOID pvReserved);
 BOOL   (WINAPI *fpRefreshPrinterChangeNotification)(HANDLE hPrinter,
                 DWORD Reserved, PVOID pvReserved, PVOID pPrinterNotifyInfo);
 BOOL   (WINAPI *fpOpenPrinterEx)(LPWSTR pPrinterName, LPHANDLE phPrinter,
                 LPPRINTER_DEFAULTSW pDefault, LPBYTE pClientInfo, DWORD Level);
 HANDLE (WINAPI *fpAddPrinterEx)(LPWSTR pName, DWORD Level, LPBYTE pPrinter,
                 LPBYTE pClientInfo, DWORD ClientInfoLevel);
 BOOL   (WINAPI *fpSetPort)(LPWSTR pName, LPWSTR pPortName, DWORD dwLevel,
                 LPBYTE pPortInfo);
 DWORD  (WINAPI *fpEnumPrinterData)( HANDLE hPrinter, DWORD dwIndex,
                 LPWSTR pValueName, DWORD cbValueName, LPDWORD pcbValueName,
                 LPDWORD pType, LPBYTE pData, DWORD cbData, LPDWORD pcbData);
 DWORD  (WINAPI *fpDeletePrinterData)(HANDLE hPrinter, LPWSTR  pValueName);
 DWORD  (WINAPI *fpClusterSplOpen)(LPCWSTR pszServer, LPCWSTR pszResource,
                 PHANDLE phSpooler, LPCWSTR pszName, LPCWSTR pszAddress);
 DWORD  (WINAPI *fpClusterSplClose)(HANDLE hSpooler);
 DWORD  (WINAPI *fpClusterSplIsAlive)(HANDLE  hSpooler);
 DWORD  (WINAPI *fpSetPrinterDataEx)(HANDLE hPrinter, LPCWSTR pKeyName,
                 LPCWSTR pValueName, DWORD Type, LPBYTE pData, DWORD cbData);
 DWORD  (WINAPI *fpGetPrinterDataEx)(HANDLE hPrinter, LPCWSTR pKeyName,
                 LPCWSTR pValueName, LPDWORD pType, LPBYTE pData, DWORD nSize,
                 LPDWORD pcbNeeded);
 DWORD  (WINAPI *fpEnumPrinterDataEx)(HANDLE hPrinter, LPCWSTR pKeyName,
                 LPBYTE pEnumValues, DWORD cbEnumValues, LPDWORD pcbEnumValues,
                 LPDWORD pnEnumValues);
 DWORD  (WINAPI *fpEnumPrinterKey)(HANDLE hPrinter, LPCWSTR pKeyName,
                 LPWSTR pSubkey, DWORD cbSubkey, LPDWORD  pcbSubkey);
 DWORD  (WINAPI *fpDeletePrinterDataEx)(HANDLE hPrinter, LPCWSTR pKeyName,
                 LPCWSTR pValueName);
 DWORD  (WINAPI *fpDeletePrinterKey)(HANDLE hPrinter, LPCWSTR pKeyName);
 BOOL   (WINAPI *fpSeekPrinter)(HANDLE hPrinter, LARGE_INTEGER liDistanceToMove,
                 PLARGE_INTEGER pliNewPointer, DWORD dwMoveMethod, BOOL bWrite);
 BOOL   (WINAPI *fpDeletePrinterDriverEx)(LPWSTR pName, LPWSTR pEnvironment,
                 LPWSTR pDriverName, DWORD dwDeleteFlag, DWORD dwVersionNum);
 BOOL   (WINAPI *fpAddPerMachineConnection)(LPCWSTR pServer,
                 LPCWSTR pPrinterName, LPCWSTR pPrintServer, LPCWSTR pProvider);
 BOOL   (WINAPI *fpDeletePerMachineConnection)(LPCWSTR pServer,
                 LPCWSTR pPrinterName);
 BOOL   (WINAPI *fpEnumPerMachineConnections)(LPCWSTR pServer,
                 LPBYTE pPrinterEnum, DWORD cbBuf, LPDWORD pcbNeeded,
                 LPDWORD pcReturned);
 BOOL   (WINAPI *fpXcvData)(HANDLE hXcv, LPCWSTR pszDataName, PBYTE pInputData,
                 DWORD cbInputData, PBYTE pOutputData, DWORD cbOutputData,
                 PDWORD pcbOutputNeeded, PDWORD pdwStatus);
 BOOL   (WINAPI *fpAddPrinterDriverEx)(LPWSTR pName, DWORD Level,
                 LPBYTE pDriverInfo, DWORD  dwFileCopyFlags);
 BOOL   (WINAPI *fpSplReadPrinter)(HANDLE hPrinter, LPBYTE *pBuf, DWORD cbBuf);
 BOOL   (WINAPI *fpDriverUnloadComplete)(LPWSTR pDriverFile);
 BOOL   (WINAPI *fpGetSpoolFileInfo)(HANDLE hPrinter, LPWSTR *pSpoolDir,
                 LPHANDLE phFile, HANDLE hSpoolerProcess, HANDLE hAppProcess);
 BOOL   (WINAPI *fpCommitSpoolData)(HANDLE hPrinter, DWORD cbCommit);
 BOOL   (WINAPI *fpCloseSpoolFileHandle)(HANDLE hPrinter);
 BOOL   (WINAPI *fpFlushPrinter)(HANDLE hPrinter, LPBYTE pBuf, DWORD cbBuf,
                 LPDWORD pcWritten, DWORD cSleep);
 DWORD  (WINAPI *fpSendRecvBidiData)(HANDLE hPort, LPCWSTR pAction,
                 LPBIDI_REQUEST_CONTAINER pReqData,
                 LPBIDI_RESPONSE_CONTAINER *ppResData);
 BOOL   (WINAPI *fpAddDriverCatalog)(HANDLE hPrinter, DWORD dwLevel,
                 VOID *pvDriverInfCatInfo, DWORD dwCatalogCopyFlags);
} PRINTPROVIDOR, *LPPRINTPROVIDOR;

typedef struct _SPLCLIENT_INFO_1 {
 DWORD  dwSize;
 LPWSTR pMachineName;
 LPWSTR pUserName;
 DWORD  dwBuildNum;
 DWORD  dwMajorVersion;
 DWORD  dwMinorVersion;
 WORD   wProcessorArchitecture;
} SPLCLIENT_INFO_1, *LPSPLCLIENT_INFO_1, *PSPLCLIENT_INFO_1;

/* DECLARATIONS */

HANDLE WINAPI CreatePrinterIC(HANDLE hPrinter, LPDEVMODEW pDevMode);
BOOL   WINAPI PlayGdiScriptOnPrinterIC(HANDLE hPrinterIC, LPBYTE pIn,
              DWORD cIn, LPBYTE pOut, DWORD cOut, DWORD ul);
BOOL   WINAPI DeletePrinterIC(HANDLE hPrinterIC);
BOOL   WINAPI DevQueryPrint(HANDLE hPrinter, LPDEVMODEW pDevMode, DWORD *pResID);

/* Security and Access */
HANDLE WINAPI RevertToPrinterSelf(VOID);
BOOL   WINAPI ImpersonatePrinterClient(HANDLE  hToken);
BOOL   WINAPI OpenPrinterToken(PHANDLE phToken);
BOOL   WINAPI SetPrinterToken(HANDLE hToken);
BOOL   WINAPI ClosePrinterToken(HANDLE hToken);

/* Notification */
BOOL   WINAPI ReplyPrinterChangeNotification(HANDLE hNotify, DWORD fdwFlags,
              PDWORD pdwResult, PVOID pPrinterNotifyInfo);
BOOL   WINAPI PartialReplyPrinterChangeNotification(HANDLE hNotify,
              PPRINTER_NOTIFY_INFO_DATA pInfoDataSrc);
PPRINTER_NOTIFY_INFO RouterAllocPrinterNotifyInfo(DWORD cPrinterNotifyInfoData);
BOOL   WINAPI RouterFreePrinterNotifyInfo(PPRINTER_NOTIFY_INFO pInfo);
BOOL   WINAPI AppendPrinterNotifyInfoData(PPRINTER_NOTIFY_INFO pInfoDest,
              PPRINTER_NOTIFY_INFO_DATA pInfoDataSrc, DWORD fdwFlags);
DWORD  WINAPI CallRouterFindFirstPrinterChangeNotification(HANDLE hPrinter,
              DWORD fdwFlags, DWORD fdwOptions, HANDLE hNotify, PVOID pvReserved);

/* Port monitor / Language monitor / Print monitor */
LPMONITOR2  WINAPI InitializePrintMonitor2(PMONITORINIT pMonitorInit,
                   PHANDLE phMonitor);
PMONITORUI  WINAPI InitializePrintMonitorUI(VOID);
LPMONITOREX WINAPI InitializePrintMonitor(LPWSTR pRegistryRoot);
BOOL        WINAPI InitializeMonitorEx(LPWSTR pRegistryRoot, LPMONITOR pMonitor);
BOOL        WINAPI InitializeMonitor(LPWSTR pRegistryRoot);

BOOL  WINAPI OpenPort(LPWSTR pName, PHANDLE pHandle);
BOOL  WINAPI WritePort(HANDLE hPort, LPBYTE pBuffer, DWORD cbBuf,
             LPDWORD pcbWritten);
BOOL  WINAPI ReadPort(HANDLE hPort, LPBYTE pBuffer, DWORD cbBuffer,
             LPDWORD pcbRead);
BOOL  WINAPI ClosePort(HANDLE hPort);

/* Print processor */
HANDLE WINAPI OpenPrintProcessor(LPWSTR pPrinterName,
              PPRINTPROCESSOROPENDATA pPrintProcessorOpenData);
BOOL   WINAPI PrintDocumentOnPrintProcessor(HANDLE hPrintProcessor,
              LPWSTR pDocumentName);
BOOL   WINAPI ClosePrintProcessor(HANDLE hPrintProcessor);
BOOL   WINAPI ControlPrintProcessor(HANDLE hPrintProcessor, DWORD Command);
BOOL   WINAPI InstallPrintProcessor(HWND hWnd);

/* Print provider */
BOOL   WINAPI InitializePrintProvidor(LPPRINTPROVIDOR pPrintProvidor,
              DWORD cbPrintProvidor, LPWSTR pFullRegistryPath);
BOOL   WINAPI ProvidorFindFirstPrinterChangeNotification(HANDLE hPrinter,
              DWORD fdwFlags, DWORD fdwOptions, HANDLE hNotify,
              PVOID pvReserved0, PVOID pvReserved1);
BOOL   WINAPI ProvidorFindClosePrinterChangeNotification(HANDLE hPrinter);

/* Spooler */
BOOL   WINAPI SpoolerFindFirstPrinterChangeNotification(HANDLE hPrinter,
              DWORD fdwFlags, DWORD fdwOptions, PHANDLE phEvent,
              PVOID pPrinterNotifyOptions, PVOID pvReserved);
BOOL   WINAPI SpoolerFindNextPrinterChangeNotification(HANDLE hPrinter,
              LPDWORD pfdwChange, PVOID pvReserved0, PVOID ppPrinterNotifyInfo);
VOID   WINAPI SpoolerFreePrinterNotifyInfo(PPRINTER_NOTIFY_INFO pInfo);
BOOL   WINAPI SpoolerFindClosePrinterChangeNotification(HANDLE hPrinter);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif  /* _WINSPLP_ */
