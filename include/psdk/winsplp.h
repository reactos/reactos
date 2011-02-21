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
  BOOL (WINAPI *fpOpenPrinter)(PWSTR lpPrinterName, HANDLE *phPrinter,
                               PPRINTER_DEFAULTSW pDefault);
  BOOL (WINAPI *fpSetJob)(HANDLE hPrinter, DWORD JobID, DWORD Level,
                          LPBYTE pJob, DWORD Command);
  BOOL (WINAPI *fpGetJob)(HANDLE hPrinter, DWORD JobID, DWORD Level,
                          LPBYTE pJob, DWORD cbBuf, LPDWORD pcbNeeded);
  BOOL (WINAPI *fpEnumJobs)(HANDLE hPrinter, DWORD FirstJob, DWORD NoJobs,
                            DWORD Level, LPBYTE pJob, DWORD cbBuf, LPDWORD pcbNeeded,
                            LPDWORD pcReturned);
  HANDLE (WINAPI *fpAddPrinter)(LPWSTR pName, DWORD Level, LPBYTE pPrinter);
  BOOL (WINAPI *fpDeletePrinter)(HANDLE hPrinter);
  BOOL (WINAPI *fpSetPrinter)(HANDLE hPrinter, DWORD Level, LPBYTE pPrinter,
                              DWORD Command);
  BOOL (WINAPI *fpGetPrinter)(HANDLE hPrinter, DWORD Level, LPBYTE pPrinter,
                              DWORD cbBuf, LPDWORD pcbNeeded);
  BOOL (WINAPI *fpEnumPrinters)(DWORD dwType, LPWSTR lpszName, DWORD dwLevel,
                                LPBYTE lpbPrinters, DWORD cbBuf, LPDWORD lpdwNeeded,
                                LPDWORD lpdwReturned);
  BOOL (WINAPI *fpAddPrinterDriver)(LPWSTR pName, DWORD Level, LPBYTE pDriverInfo);
  BOOL (WINAPI *fpEnumPrinterDrivers)(LPWSTR pName, LPWSTR pEnvironment,
                                      DWORD Level, LPBYTE pDriverInfo, DWORD cbBuf,
                                      LPDWORD pcbNeeded, LPDWORD pcbReturned);
  BOOL (WINAPI *fpGetPrinterDriver)(HANDLE hPrinter, LPWSTR pEnvironment,
                                    DWORD Level, LPBYTE pDriverInfo, DWORD cbBuf,
                                    LPDWORD pcbNeeded);
  BOOL (WINAPI *fpGetPrinterDriverDirectory)(LPWSTR pName, LPWSTR pEnvironment,
                                             DWORD Level, LPBYTE pDriverDirectory,
                                             DWORD cbBuf, LPDWORD pcbNeeded);
  BOOL (WINAPI *fpDeletePrinterDriver)(LPWSTR pName, LPWSTR pEnvironment,
                                       LPWSTR pDriverName);
  BOOL (WINAPI *fpAddPrintProcessor)(LPWSTR pName, LPWSTR pEnvironment,
                                     LPWSTR pPathName, LPWSTR pPrintProcessorName);
  BOOL (WINAPI *fpEnumPrintProcessors)(LPWSTR pName, LPWSTR pEnvironment,
                                       DWORD Level, LPBYTE pPrintProcessorInfo,
                                       DWORD cbBuf, LPDWORD pcbNeeded,
                                       LPDWORD pcbReturned);
  BOOL (WINAPI *fpGetPrintProcessorDirectory)(LPWSTR pName, LPWSTR pEnvironment,
                                              DWORD Level, LPBYTE pPrintProcessorInfo,
                                              DWORD cbBuf, LPDWORD pcbNeeded);
  BOOL (WINAPI *fpDeletePrintProcessor)(LPWSTR pName, LPWSTR pEnvironment,
                 LPWSTR pPrintProcessorName);
  BOOL (WINAPI *fpEnumPrintProcessorDatatypes)(LPWSTR pName,
                                               LPWSTR pPrintProcessorName,
                                               DWORD Level, LPBYTE pDatatypes,
                                               DWORD cbBuf, LPDWORD pcbNeeded,
                                               LPDWORD pcbReturned);
  DWORD (WINAPI *fpStartDocPrinter)(HANDLE hPrinter, DWORD Level, LPBYTE pDocInfo);
  BOOL (WINAPI *fpStartPagePrinter)(HANDLE hPrinter);
  BOOL (WINAPI *fpWritePrinter)(HANDLE hPrinter, LPVOID pBuf, DWORD cbBuf,
                                LPDWORD pcWritten);
  BOOL (WINAPI *fpEndPagePrinter)(HANDLE hPrinter);
  BOOL (WINAPI *fpAbortPrinter)(HANDLE hPrinter);
  BOOL (WINAPI *fpReadPrinter)(HANDLE hPrinter, LPVOID pBuf, DWORD cbBuf,
                               LPDWORD pNoBytesRead);
  BOOL (WINAPI *fpEndDocPrinter)(HANDLE hPrinter);
  BOOL (WINAPI *fpAddJob)(HANDLE hPrinter, DWORD Level, LPBYTE pData,
                          DWORD cbBuf, LPDWORD pcbNeeded);
  BOOL (WINAPI *fpScheduleJob)(HANDLE hPrinter, DWORD JobID);
  DWORD (WINAPI *fpGetPrinterData)(HANDLE hPrinter, LPWSTR pValueName,
                                   LPDWORD pType, LPBYTE pData, DWORD nSize,
                                   LPDWORD pcbNeeded);
  DWORD (WINAPI *fpSetPrinterData)(HANDLE hPrinter, LPWSTR pValueName,
                                   DWORD Type, LPBYTE pData, DWORD cbData);
  DWORD (WINAPI *fpWaitForPrinterChange)(HANDLE hPrinter, DWORD Flags);
  BOOL (WINAPI *fpClosePrinter)(HANDLE phPrinter);
  BOOL (WINAPI *fpAddForm)(HANDLE hPrinter, DWORD Level, LPBYTE pForm);
  BOOL (WINAPI *fpDeleteForm)(HANDLE hPrinter, LPWSTR pFormName);
  BOOL (WINAPI *fpGetForm)(HANDLE hPrinter, LPWSTR pFormName, DWORD Level,
                           LPBYTE pForm, DWORD cbBuf, LPDWORD pcbNeeded);
  BOOL (WINAPI *fpSetForm)(HANDLE hPrinter, LPWSTR pFormName, DWORD Level,
                           LPBYTE pForm);
  BOOL (WINAPI *fpEnumForms)(HANDLE hPrinter, DWORD Level, LPBYTE pForm,
                             DWORD cbBuf, LPDWORD pcbNeeded, LPDWORD pcReturned);
  BOOL (WINAPI *fpEnumMonitors)(LPWSTR pName, DWORD Level, LPBYTE pMonitors,
                                DWORD cbBuf, LPDWORD pcbNeeded,
                                LPDWORD pcReturned);
  BOOL (WINAPI *fpEnumPorts)(LPWSTR pName, DWORD Level, LPBYTE pPorts,
                             DWORD cbBuf, LPDWORD pcbNeeded, LPDWORD pcReturned);
  BOOL (WINAPI *fpAddPort)(LPWSTR pName, HWND hWnd, LPWSTR pMonitorName);
  BOOL (WINAPI *fpConfigurePort)(LPWSTR pName, HWND hWnd, LPWSTR pPortName);
  BOOL (WINAPI *fpDeletePort)(LPWSTR pName, HWND hWnd, LPWSTR pPortName);
  HANDLE (WINAPI *fpCreatePrinterIC)(HANDLE hPrinter, LPDEVMODEW pDevMode);
  BOOL (WINAPI *fpPlayGdiScriptOnPrinterIC)(HANDLE hPrinterIC, LPBYTE pIn,
                                            DWORD cIn, LPBYTE pOut, DWORD cOut,
                                            DWORD ul);
  BOOL (WINAPI *fpDeletePrinterIC)(HANDLE hPrinterIC);
  BOOL (WINAPI *fpAddPrinterConnection)(LPWSTR pName);
  BOOL (WINAPI *fpDeletePrinterConnection)(LPWSTR pName);
  DWORD (WINAPI *fpPrinterMessageBox)(HANDLE hPrinter, DWORD Error, HWND hWnd,
                                      LPWSTR pText, LPWSTR pCaption,
                                      DWORD dwType);
  BOOL (WINAPI *fpAddMonitor)(LPWSTR pName, DWORD Level, LPBYTE pMonitors);
  BOOL (WINAPI *fpDeleteMonitor)(LPWSTR pName, LPWSTR pEnvironment,
                                 LPWSTR pMonitorName);
  BOOL (WINAPI *fpResetPrinter)(HANDLE hPrinter, LPPRINTER_DEFAULTSW pDefault);
  BOOL (WINAPI *fpGetPrinterDriverEx)(HANDLE hPrinter, LPWSTR pEnvironment,
                                      DWORD Level, LPBYTE pDriverInfo,
                                      DWORD cbBuf, LPDWORD pcbNeeded,
                                      DWORD dwClientMajorVersion,
                                      DWORD dwClientMinorVersion,
                                      PDWORD pdwServerMajorVersion,
                                      PDWORD pdwServerMinorVersion);
  HANDLE (WINAPI *fpFindFirstPrinterChangeNotification)(HANDLE hPrinter,
                                                        DWORD fdwFlags,
                                                        DWORD fdwOptions,
                                                        LPVOID pPrinterNotifyOptions);
  BOOL (WINAPI *fpFindClosePrinterChangeNotification)(HANDLE hChange);
  BOOL (WINAPI *fpAddPortEx)(HANDLE hMonitor, LPWSTR pName, DWORD Level,
                             LPBYTE lpBuffer, LPWSTR lpMonitorName);
  BOOL (WINAPI *fpShutDown)(LPVOID pvReserved);
  BOOL (WINAPI *fpRefreshPrinterChangeNotification)(HANDLE hPrinter,
                                                    DWORD Reserved,
                                                    PVOID pvReserved,
                                                    PVOID pPrinterNotifyInfo);
  BOOL (WINAPI *fpOpenPrinterEx)(LPWSTR pPrinterName, LPHANDLE phPrinter,
                                 LPPRINTER_DEFAULTSW pDefault, LPBYTE pClientInfo,
                                 DWORD Level);
  HANDLE (WINAPI *fpAddPrinterEx)(LPWSTR pName, DWORD Level, LPBYTE pPrinter,
                                  LPBYTE pClientInfo, DWORD ClientInfoLevel);
  BOOL (WINAPI *fpSetPort)(LPWSTR pName, LPWSTR pPortName, DWORD dwLevel,
                           LPBYTE pPortInfo);
  DWORD (WINAPI *fpEnumPrinterData)(HANDLE hPrinter, DWORD dwIndex,
                                    LPWSTR pValueName, DWORD cbValueName,
                                    LPDWORD pcbValueName, LPDWORD pType,
                                    LPBYTE pData, DWORD cbData, LPDWORD pcbData);
  DWORD (WINAPI *fpDeletePrinterData)(HANDLE hPrinter, LPWSTR pValueName);
  DWORD (WINAPI *fpClusterSplOpen)(LPCWSTR pszServer, LPCWSTR pszResource,
                                   PHANDLE phSpooler, LPCWSTR pszName,
                                   LPCWSTR pszAddress);
  DWORD (WINAPI *fpClusterSplClose)(HANDLE hSpooler);
  DWORD (WINAPI *fpClusterSplIsAlive)(HANDLE hSpooler);
  DWORD (WINAPI *fpSetPrinterDataEx)(HANDLE hPrinter, LPCWSTR pKeyName,
                                     LPCWSTR pValueName, DWORD Type,
                                     LPBYTE pData, DWORD cbData);
  DWORD (WINAPI *fpGetPrinterDataEx)(HANDLE hPrinter, LPCWSTR pKeyName,
                                     LPCWSTR pValueName, LPDWORD pType,
                                     LPBYTE pData, DWORD nSize, LPDWORD pcbNeeded);
  DWORD (WINAPI *fpEnumPrinterDataEx)(HANDLE hPrinter, LPCWSTR pKeyName,
                                      LPBYTE pEnumValues, DWORD cbEnumValues,
                                      LPDWORD pcbEnumValues, LPDWORD pnEnumValues);
  DWORD (WINAPI *fpEnumPrinterKey)(HANDLE hPrinter, LPCWSTR pKeyName,
                                   LPWSTR pSubkey, DWORD cbSubkey, LPDWORD pcbSubkey);
  DWORD (WINAPI *fpDeletePrinterDataEx)(HANDLE hPrinter, LPCWSTR pKeyName,
                                        LPCWSTR pValueName);
  DWORD (WINAPI *fpDeletePrinterKey)(HANDLE hPrinter, LPCWSTR pKeyName);
  BOOL (WINAPI *fpSeekPrinter)(HANDLE hPrinter, LARGE_INTEGER liDistanceToMove,
                               PLARGE_INTEGER pliNewPointer, DWORD dwMoveMethod,
                               BOOL bWrite);
  BOOL (WINAPI *fpDeletePrinterDriverEx)(LPWSTR pName, LPWSTR pEnvironment,
                                         LPWSTR pDriverName, DWORD dwDeleteFlag,
                                         DWORD dwVersionNum);
  BOOL (WINAPI *fpAddPerMachineConnection)(LPCWSTR pServer,
                                           LPCWSTR pPrinterName, LPCWSTR pPrintServer,
                                           LPCWSTR pProvider);
  BOOL (WINAPI *fpDeletePerMachineConnection)(LPCWSTR pServer,
                                              LPCWSTR pPrinterName);
  BOOL (WINAPI *fpEnumPerMachineConnections)(LPCWSTR pServer,
                                             LPBYTE pPrinterEnum, DWORD cbBuf,
                                             LPDWORD pcbNeeded,
                 LPDWORD pcReturned);
  BOOL (WINAPI *fpXcvData)(HANDLE hXcv, LPCWSTR pszDataName, PBYTE pInputData,
                           DWORD cbInputData, PBYTE pOutputData, DWORD cbOutputData,
                           PDWORD pcbOutputNeeded, PDWORD pdwStatus);
  BOOL (WINAPI *fpAddPrinterDriverEx)(LPWSTR pName, DWORD Level,
                                      LPBYTE pDriverInfo, DWORD dwFileCopyFlags);
  BOOL (WINAPI *fpSplReadPrinter)(HANDLE hPrinter, LPBYTE *pBuf, DWORD cbBuf);
  BOOL (WINAPI *fpDriverUnloadComplete)(LPWSTR pDriverFile);
  BOOL (WINAPI *fpGetSpoolFileInfo)(HANDLE hPrinter, LPWSTR *pSpoolDir,
                                    LPHANDLE phFile, HANDLE hSpoolerProcess,
                                    HANDLE hAppProcess);
  BOOL (WINAPI *fpCommitSpoolData)(HANDLE hPrinter, DWORD cbCommit);
  BOOL (WINAPI *fpCloseSpoolFileHandle)(HANDLE hPrinter);
  BOOL (WINAPI *fpFlushPrinter)(HANDLE hPrinter, LPBYTE pBuf, DWORD cbBuf,
                                LPDWORD pcWritten, DWORD cSleep);
  DWORD (WINAPI *fpSendRecvBidiData)(HANDLE hPort, LPCWSTR pAction,
                                     LPBIDI_REQUEST_CONTAINER pReqData,
                                     LPBIDI_RESPONSE_CONTAINER *ppResData);
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
  LONG (WINAPI *fpCreateKey)(HANDLE hcKey, LPCWSTR pszSubKey, DWORD dwOptions,
                             REGSAM samDesired,
                             PSECURITY_ATTRIBUTES pSecurityAttributes,
                             PHANDLE phckResult, PDWORD pdwDisposition,
                             HANDLE hSpooler);
  LONG (WINAPI *fpOpenKey)(HANDLE hcKey, LPCWSTR pszSubKey, REGSAM samDesired,
                           PHANDLE phkResult, HANDLE hSpooler);
  LONG (WINAPI *fpCloseKey)(HANDLE hcKey, HANDLE hSpooler);
  LONG (WINAPI *fpDeleteKey)(HANDLE hcKey, LPCWSTR pszSubKey, HANDLE hSpooler);
  LONG (WINAPI *fpEnumKey)(HANDLE hcKey, DWORD dwIndex, LPWSTR pszName,
                           PDWORD pcchName, PFILETIME pftLastWriteTime,
                           HANDLE hSpooler);
  LONG (WINAPI *fpQueryInfoKey)(HANDLE hcKey, PDWORD pcSubKeys, PDWORD pcbKey,
                                PDWORD pcValues, PDWORD pcbValue, PDWORD pcbData,
                                PDWORD pcbSecurityDescriptor,
                                PFILETIME pftLastWriteTime,
                                HANDLE hSpooler);
  LONG (WINAPI *fpSetValue)(HANDLE hcKey, LPCWSTR pszValue, DWORD dwType,
                const BYTE* pData, DWORD cbData, HANDLE hSpooler);
  LONG (WINAPI *fpDeleteValue)(HANDLE hcKey, LPCWSTR pszValue, HANDLE hSpooler);
  LONG (WINAPI *fpEnumValue)(HANDLE hcKey, DWORD dwIndex, LPWSTR pszValue,
                             PDWORD pcbValue, PDWORD pType, PBYTE pData, PDWORD pcbData,
                             HANDLE hSpooler);
  LONG (WINAPI *fpQueryValue)(HANDLE hcKey, LPCWSTR pszValue, PDWORD pType,
                              PBYTE pData, PDWORD pcbData, HANDLE hSpooler);
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
  BOOL (WINAPI *pfnEnumPorts)(LPWSTR pName, DWORD Level, LPBYTE pPorts,
                              DWORD cbBuf, LPDWORD pcbNeeded, LPDWORD pcReturned);
  BOOL (WINAPI *pfnOpenPort)(LPWSTR pName, PHANDLE pHandle);
  BOOL (WINAPI *pfnOpenPortEx)(LPWSTR pPortName, LPWSTR pPrinterName,
                               PHANDLE pHandle, struct _MONITOR *pMonitor);
  BOOL (WINAPI *pfnStartDocPort)(HANDLE hPort, LPWSTR pPrinterName,
                                 DWORD JobId, DWORD Level, LPBYTE pDocInfo);
  BOOL (WINAPI *pfnWritePort)(HANDLE hPort, LPBYTE pBuffer, DWORD cbBuf,
                              LPDWORD pcbWritten);
  BOOL (WINAPI *pfnReadPort)(HANDLE hPort, LPBYTE pBuffer, DWORD cbBuffer,
                             LPDWORD pcbRead);
  BOOL (WINAPI *pfnEndDocPort)(HANDLE hPort);
  BOOL (WINAPI *pfnClosePort)(HANDLE hPort);
  BOOL (WINAPI *pfnAddPort)(LPWSTR pName, HWND hWnd, LPWSTR pMonitorName);
  BOOL (WINAPI *pfnAddPortEx)(LPWSTR pName, DWORD Level, LPBYTE lpBuffer,
                              LPWSTR lpMonitorName);
  BOOL (WINAPI *pfnConfigurePort)(LPWSTR pName, HWND hWnd, LPWSTR pPortName);
  BOOL (WINAPI *pfnDeletePort)(LPWSTR pName, HWND hWnd, LPWSTR pPortName);
  BOOL (WINAPI *pfnGetPrinterDataFromPort)(HANDLE hPort, DWORD ControlID,
                                           LPWSTR pValueName, LPWSTR lpInBuffer,
                                           DWORD cbInBuffer, LPWSTR lpOutBuffer,
                                           DWORD cbOutBuffer, LPDWORD lpcbReturned);
  BOOL (WINAPI *pfnSetPortTimeOuts)(HANDLE hPort, LPCOMMTIMEOUTS lpCTO,
                                    DWORD reserved);
  BOOL (WINAPI *pfnXcvOpenPort)(LPCWSTR pszObject, ACCESS_MASK GrantedAccess, PHANDLE phXcv);
  DWORD (WINAPI *pfnXcvDataPort)(HANDLE hXcv, LPCWSTR pszDataName,
                                 PBYTE pInputData, DWORD cbInputData,
                                 PBYTE pOutputData, DWORD cbOutputData,
                                 PDWORD pcbOutputNeeded);
  BOOL (WINAPI *pfnXcvClosePort)(HANDLE hXcv);
} MONITOR, *LPMONITOR;

typedef struct _MONITOREX {
  DWORD dwMonitorSize;
  MONITOR Monitor;
} MONITOREX, *LPMONITOREX;

typedef struct _MONITOR2 {
  DWORD cbSize;
  BOOL (WINAPI *pfnEnumPorts)(LPWSTR pName, DWORD Level, LPBYTE pPorts,
                              DWORD cbBuf, LPDWORD pcbNeeded,
                              LPDWORD pcReturned);
  BOOL (WINAPI *pfnOpenPort)(LPWSTR pName, PHANDLE pHandle);
  BOOL (WINAPI *pfnOpenPortEx)(LPWSTR pPortName, LPWSTR pPrinterName,
                               PHANDLE pHandle, struct _MONITOR2 *pMonitor2);
  BOOL (WINAPI *pfnStartDocPort)(HANDLE hPort, LPWSTR pPrinterName,
                                 DWORD JobId, DWORD Level, LPBYTE pDocInfo);
  BOOL (WINAPI *pfnWritePort)(HANDLE hPort, LPBYTE pBuffer, DWORD cbBuf,
                              LPDWORD pcbWritten);
  BOOL (WINAPI *pfnReadPort)(HANDLE hPort, LPBYTE pBuffer, DWORD cbBuffer,
                             LPDWORD pcbRead);
  BOOL (WINAPI *pfnEndDocPort)(HANDLE hPort);
  BOOL (WINAPI *pfnClosePort)(HANDLE hPort);
  BOOL (WINAPI *pfnAddPort)(LPWSTR pName, HWND hWnd, LPWSTR pMonitorName);
  BOOL (WINAPI *pfnAddPortEx)(LPWSTR pName, DWORD Level, LPBYTE lpBuffer,
                              LPWSTR lpMonitorName);
  BOOL (WINAPI *pfnConfigurePort)(LPWSTR pName, HWND hWnd, LPWSTR pPortName);
  BOOL (WINAPI *pfnDeletePort)(LPWSTR pName, HWND hWnd, LPWSTR pPortName);
  BOOL (WINAPI *pfnGetPrinterDataFromPort)(HANDLE hPort, DWORD ControlID,
                                           LPWSTR pValueName, LPWSTR lpInBuffer,
                                           DWORD cbInBuffer, LPWSTR lpOutBuffer,
                                           DWORD cbOutBuffer, LPDWORD lpcbReturned);
  BOOL (WINAPI *pfnSetPortTimeOuts)(HANDLE hPort, LPCOMMTIMEOUTS lpCTO,
                                    DWORD reserved);
  BOOL (WINAPI *pfnXcvOpenPort)(HANDLE hMonitor, LPCWSTR pszObject,
                                ACCESS_MASK GrantedAccess, PHANDLE phXcv);
  DWORD (WINAPI *pfnXcvDataPort)(HANDLE hXcv, LPCWSTR pszDataName,
                                 PBYTE pInputData, DWORD cbInputData,
                                 PBYTE pOutputData, DWORD cbOutputData,
                                 PDWORD pcbOutputNeeded);
  BOOL (WINAPI *pfnXcvClosePort)(HANDLE hXcv);
  VOID (WINAPI *pfnShutdown)(HANDLE hMonitor);
#if (NTDDI_VERSION >= NTDDI_WINXP)
 DWORD (WINAPI *pfnSendRecvBidiDataFromPort)(HANDLE hPort, DWORD dwAccessBit,
                                             LPCWSTR pAction,
                                             PBIDI_REQUEST_CONTAINER pReqData,
                                             PBIDI_RESPONSE_CONTAINER *ppResData);
#endif
#if (NTDDI_VERSION >= NTDDI_WIN7)
  DWORD (WINAPI *pfnNotifyUsedPorts)(HANDLE hMonitor, DWORD cPorts,
                                   PCWSTR *ppszPorts);

  DWORD (WINAPI *pfnNotifyUnusedPorts)(HANDLE hMonitor, DWORD cPorts,
                                       PCWSTR *ppszPorts);
#endif
} MONITOR2, *LPMONITOR2, *PMONITOR2;

typedef struct _MONITORUI {
  DWORD dwMonitorUISize;
  BOOL (WINAPI *pfnAddPortUI)(PCWSTR pszServer, HWND hWnd,
                              PCWSTR pszPortNameIn, PWSTR *ppszPortNameOut);
  BOOL (WINAPI *pfnConfigurePortUI)(PCWSTR pName, HWND hWnd, PCWSTR pPortName);
  BOOL (WINAPI *pfnDeletePortUI)(PCWSTR pszServer, HWND hWnd, PCWSTR pszPortName);
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
  IN DWORD dwCommand,
  IN PVOID pContext,
  IN DWORD dwColor,
  IN PPRINTER_NOTIFY_INFO pNofityInfo,
  IN DWORD fdwFlags,
  OUT PDWORD pdwResult);

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
  OUT LPPRINTPROVIDOR pPrintProvidor,
  IN DWORD cbPrintProvidor,
  IN LPWSTR pFullRegistryPath OPTIONAL);

HANDLE
WINAPI
OpenPrintProcessor(
  IN LPWSTR pPrinterName,
  IN PPRINTPROCESSOROPENDATA pPrintProcessorOpenData);

BOOL
WINAPI
PrintDocumentOnPrintProcessor(
  IN HANDLE hPrintProcessor,
  IN LPWSTR pDocumentName);

BOOL
WINAPI
ClosePrintProcessor(
  IN OUT HANDLE hPrintProcessor);

BOOL
WINAPI
ControlPrintProcessor(
  IN HANDLE hPrintProcessor,
  IN DWORD Command);

DWORD
WINAPI
GetPrintProcessorCapabilities(
  IN LPTSTR pValueName,
  IN DWORD dwAttributes,
  OUT LPBYTE pData,
  IN DWORD nSize,
  OUT LPDWORD pcbNeeded);

BOOL
WINAPI
InitializeMonitor(
  IN LPWSTR pRegistryRoot);

BOOL
WINAPI
OpenPort(
  IN LPWSTR pName,
  OUT PHANDLE pHandle);

BOOL
WINAPI
WritePort(
  HANDLE hPort,
  LPBYTE pBuffer,
  DWORD cbBuf,
  LPDWORD pcbWritten);

BOOL
WINAPI
ReadPort(
  HANDLE hPort,
  LPBYTE pBuffer,
  DWORD cbBuffer,
  LPDWORD pcbRead);

BOOL
WINAPI
ClosePort(
  IN HANDLE hPort);

BOOL
WINAPI
XcvOpenPort(
  LPCWSTR pszObject,
  ACCESS_MASK GrantedAccess,
  PHANDLE phXcv);

DWORD
WINAPI
XcvDataPort(
  HANDLE hXcv,
  LPCWSTR pszDataName,
  PBYTE pInputData,
  DWORD cbInputData,
  PBYTE pOutputData,
  DWORD cbOutputData,
  PDWORD pcbOutputNeeded);

BOOL
WINAPI
XcvClosePort(
  IN HANDLE hXcv);

BOOL
WINAPI
AddPortUI(
  IN PCWSTR pszServer OPTIONAL,
  IN HWND hWnd,
  IN PCWSTR pszMonitorNameIn,
  OUT PWSTR *ppszPortNameOut OPTIONAL);

BOOL
WINAPI
ConfigurePortUI(
  IN PCWSTR pszServer,
  IN HWND hWnd,
  IN PCWSTR pszPortName);

BOOL
WINAPI
DeletePortUI(
  IN PCWSTR pszServer,
  IN HWND hWnd,
  IN PCWSTR pszPortName);

BOOL
WINAPI
SplDeleteSpoolerPortStart(
  IN PCWSTR pPortName);

BOOL
WINAPI
SplDeleteSpoolerPortEnd(
  IN PCWSTR pName,
  IN BOOL bDeletePort);

BOOL
WINAPI
SpoolerCopyFileEvent(
  IN LPWSTR pszPrinterName,
  IN LPWSTR pszKey,
  IN DWORD dwCopyFileEvent);

DWORD
WINAPI
GenerateCopyFilePaths(
  IN LPCWSTR pszPrinterName,
  IN LPCWSTR pszDirectory,
  IN LPBYTE pSplClientInfo,
  IN DWORD dwLevel,
  IN OUT LPWSTR pszSourceDir,
  IN OUT LPDWORD pcchSourceDirSize,
  IN OUT LPWSTR pszTargetDir,
  IN OUT LPDWORD pcchTargetDirSize,
  IN DWORD dwFlags);

HANDLE WINAPI CreatePrinterIC(HANDLE hPrinter, LPDEVMODEW pDevMode);
BOOL WINAPI PlayGdiScriptOnPrinterIC(HANDLE hPrinterIC, LPBYTE pIn,
                                     DWORD cIn, LPBYTE pOut, DWORD cOut, DWORD ul);
BOOL WINAPI DeletePrinterIC(HANDLE hPrinterIC);
BOOL WINAPI DevQueryPrint(HANDLE hPrinter, LPDEVMODEW pDevMode, DWORD *pResID);
HANDLE WINAPI RevertToPrinterSelf(VOID);
BOOL WINAPI ImpersonatePrinterClient(HANDLE hToken);
BOOL WINAPI ReplyPrinterChangeNotification(HANDLE hNotify, DWORD fdwFlags,
                                           PDWORD pdwResult, PVOID pPrinterNotifyInfo);
BOOL WINAPI ReplyPrinterChangeNotificationEx(HANDLE hNotify, DWORD dwColor,
                                             DWORD fdwFlags, PDWORD pdwResult,
                                             PVOID pPrinterNotifyInfo);
BOOL WINAPI PartialReplyPrinterChangeNotification(HANDLE hNotify,
                                                  PPRINTER_NOTIFY_INFO_DATA pInfoDataSrc);
PPRINTER_NOTIFY_INFO WINAPI RouterAllocPrinterNotifyInfo(DWORD cPrinterNotifyInfoData);
BOOL WINAPI RouterFreePrinterNotifyInfo(PPRINTER_NOTIFY_INFO pInfo);

BOOL WINAPI AppendPrinterNotifyInfoData(PPRINTER_NOTIFY_INFO pInfoDest,
                                        PPRINTER_NOTIFY_INFO_DATA pInfoDataSrc,
                                        DWORD fdwFlags);
DWORD WINAPI CallRouterFindFirstPrinterChangeNotification(HANDLE hPrinter,
                                                          DWORD fdwFlags,
                                                          DWORD fdwOptions,
                                                          HANDLE hNotify,
                                                          PPRINTER_NOTIFY_OPTIONS pPrinterNotifyOptions);
BOOL WINAPI ProvidorFindFirstPrinterChangeNotification(HANDLE hPrinter,
                                                       DWORD fdwFlags,
                                                       DWORD fdwOptions,
                                                       HANDLE hNotify,
                                                       PVOID pvReserved0,
                                                       PVOID pvReserved1);
BOOL WINAPI ProvidorFindClosePrinterChangeNotification(HANDLE hPrinter);

/* Spooler */
BOOL WINAPI SpoolerFindFirstPrinterChangeNotification(HANDLE hPrinter,
                                                      DWORD fdwFlags,
                                                      DWORD fdwOptions,
                                                      PHANDLE phEvent,
                                                      PVOID pPrinterNotifyOptions,
                                                      PVOID pvReserved);
BOOL WINAPI SpoolerFindNextPrinterChangeNotification(HANDLE hPrinter,
                                                     LPDWORD pfdwChange,
                                                     PVOID pvReserved0,
                                                     PVOID ppPrinterNotifyInfo);
VOID WINAPI SpoolerFreePrinterNotifyInfo(PPRINTER_NOTIFY_INFO pInfo);
BOOL WINAPI SpoolerFindClosePrinterChangeNotification(HANDLE hPrinter);

/* Port monitor / Language monitor / Print monitor */
LPMONITOR2 WINAPI InitializePrintMonitor2(PMONITORINIT pMonitorInit,
                                          PHANDLE phMonitor);
PMONITORUI WINAPI InitializePrintMonitorUI(VOID);
LPMONITOREX WINAPI InitializePrintMonitor(LPWSTR pRegistryRoot);
BOOL WINAPI InitializeMonitorEx(LPWSTR pRegistryRoot, LPMONITOR pMonitor);

#if (NTDDI_VERSION >= NTDDI_WINXP)

PBIDI_RESPONSE_CONTAINER WINAPI RouterAllocBidiResponseContainer(DWORD Count);
PVOID WINAPI RouterAllocBidiMem(size_t NumBytes);
DWORD WINAPI RouterFreeBidiResponseContainer(PBIDI_RESPONSE_CONTAINER pData);
VOID WINAPI RouterFreeBidiMem(PVOID pMemPointer);

BOOL
WINAPI
SplPromptUIInUsersSession(
  IN HANDLE hPrinter,
  IN DWORD JobId,
  IN PSHOWUIPARAMS pUIParams,
  OUT DWORD *pResponse);

DWORD
WINAPI
SplIsSessionZero(
  IN HANDLE hPrinter,
  IN DWORD JobId,
  OUT BOOL *pIsSessionZero);

#endif /* (NTDDI_VERSION >= NTDDI_WINXP) */

#if (NTDDI_VERSION >= NTDDI_WS03)
BOOL
WINAPI
GetJobAttributes(
  IN LPWSTR pPrinterName,
  IN LPDEVMODEW pDevmode,
  OUT PATTRIBUTE_INFO_3 pAttributeInfo);
#endif

#if (NTDDI_VERSION >= NTDDI_VISTA)

#define FILL_WITH_DEFAULTS   0x1

BOOL
WINAPI
GetJobAttributesEx(
  IN LPWSTR pPrinterName,
  IN LPDEVMODEW pDevmode,
  IN DWORD dwLevel,
  OUT LPBYTE pAttributeInfo,
  IN DWORD nSize,
  IN DWORD dwFlags);

BOOL WINAPI SpoolerRefreshPrinterChangeNotification(HANDLE hPrinter,
                                                    DWORD dwColor,
                                                    PPRINTER_NOTIFY_OPTIONS pOptions,
                                                    PPRINTER_NOTIFY_INFO *ppInfo);

#endif /* (NTDDI_VERSION >= NTDDI_VISTA) */

/* FIXME : The following declarations are not present in the official header */

BOOL WINAPI OpenPrinterToken(PHANDLE phToken);
BOOL WINAPI SetPrinterToken(HANDLE hToken);
BOOL WINAPI ClosePrinterToken(HANDLE hToken);
BOOL WINAPI InstallPrintProcessor(HWND hWnd);

#ifdef __cplusplus
}
#endif
