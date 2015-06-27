/*
 * PROJECT:     ReactOS Print Spooler Service
 * LICENSE:     GNU GPLv2 or any later version as published by the Free Software Foundation
 * PURPOSE:     Unimplemented RPC calls
 * COPYRIGHT:   Copyright 2015 Colin Finck <colin@reactos.org>
 */

#include "precomp.h"

DWORD
_RpcAddPrinter(WINSPOOL_HANDLE pName, WINSPOOL_PRINTER_CONTAINER *pPrinterContainer, WINSPOOL_DEVMODE_CONTAINER *pDevModeContainer, WINSPOOL_SECURITY_CONTAINER *pSecurityContainer, WINSPOOL_PRINTER_HANDLE *pHandle)
{
    UNIMPLEMENTED;
    return ERROR_INVALID_FUNCTION;
}

DWORD
_RpcDeletePrinter(WINSPOOL_PRINTER_HANDLE hPrinter)
{
    UNIMPLEMENTED;
    return ERROR_INVALID_FUNCTION;
}

DWORD
_RpcSetPrinter(WINSPOOL_PRINTER_HANDLE hPrinter, WINSPOOL_PRINTER_CONTAINER *pPrinterContainer, WINSPOOL_DEVMODE_CONTAINER *pDevModeContainer, WINSPOOL_SECURITY_CONTAINER *pSecurityContainer, DWORD Command)
{
    UNIMPLEMENTED;
    return ERROR_INVALID_FUNCTION;
}

DWORD
_RpcGetPrinter(WINSPOOL_PRINTER_HANDLE hPrinter, DWORD Level, BYTE *pPrinter, DWORD cbBuf, DWORD *pcbNeeded)
{
    UNIMPLEMENTED;
    return ERROR_INVALID_FUNCTION;
}

DWORD
_RpcAddPrinterDriver(WINSPOOL_HANDLE pName, WINSPOOL_DRIVER_CONTAINER *pDriverContainer)
{
    UNIMPLEMENTED;
    return ERROR_INVALID_FUNCTION;
}

DWORD
_RpcEnumPrinterDrivers(WINSPOOL_HANDLE pName, WCHAR *pEnvironment, DWORD Level, BYTE *pDrivers, DWORD cbBuf, DWORD *pcbNeeded, DWORD *pcReturned)
{
    UNIMPLEMENTED;
    return ERROR_INVALID_FUNCTION;
}

DWORD
_RpcGetPrinterDriver(WINSPOOL_PRINTER_HANDLE hPrinter, WCHAR *pEnvironment, DWORD Level, BYTE *pDriver, DWORD cbBuf, DWORD *pcbNeeded)
{
    UNIMPLEMENTED;
    return ERROR_INVALID_FUNCTION;
}

DWORD
_RpcGetPrinterDriverDirectory(WINSPOOL_HANDLE pName, WCHAR *pEnvironment, DWORD Level, BYTE *pDriverDirectory, DWORD cbBuf, DWORD *pcbNeeded)
{
    UNIMPLEMENTED;
    return ERROR_INVALID_FUNCTION;
}

DWORD
_RpcDeletePrinterDriver(WINSPOOL_HANDLE pName, WCHAR *pEnvironment, WCHAR *pDriverName)
{
    UNIMPLEMENTED;
    return ERROR_INVALID_FUNCTION;
}

DWORD
_RpcStartDocPrinter(WINSPOOL_PRINTER_HANDLE hPrinter, WINSPOOL_DOC_INFO_CONTAINER *pDocInfoContainer, DWORD *pJobId)
{
    UNIMPLEMENTED;
    return ERROR_INVALID_FUNCTION;
}

DWORD
_RpcStartPagePrinter(WINSPOOL_PRINTER_HANDLE hPrinter)
{
    UNIMPLEMENTED;
    return ERROR_INVALID_FUNCTION;
}

DWORD
_RpcWritePrinter(WINSPOOL_PRINTER_HANDLE hPrinter, BYTE *pBuf, DWORD cbBuf, DWORD *pcWritten)
{
    UNIMPLEMENTED;
    return ERROR_INVALID_FUNCTION;
}

DWORD
_RpcEndPagePrinter(WINSPOOL_PRINTER_HANDLE hPrinter)
{
    UNIMPLEMENTED;
    return ERROR_INVALID_FUNCTION;
}

DWORD
_RpcAbortPrinter(WINSPOOL_PRINTER_HANDLE hPrinter)
{
    UNIMPLEMENTED;
    return ERROR_INVALID_FUNCTION;
}

DWORD
_RpcReadPrinter(WINSPOOL_PRINTER_HANDLE hPrinter, BYTE *pBuf, DWORD cbBuf, DWORD *pcNoBytesRead)
{
    UNIMPLEMENTED;
    return ERROR_INVALID_FUNCTION;
}

DWORD
_RpcEndDocPrinter(WINSPOOL_PRINTER_HANDLE hPrinter)
{
    UNIMPLEMENTED;
    return ERROR_INVALID_FUNCTION;
}

DWORD
_RpcGetPrinterData(WINSPOOL_PRINTER_HANDLE hPrinter, WCHAR *pValueName, DWORD *pType, BYTE *pData, DWORD nSize, DWORD *pcbNeeded)
{
    UNIMPLEMENTED;
    return ERROR_INVALID_FUNCTION;
}

DWORD
_RpcSetPrinterData(WINSPOOL_PRINTER_HANDLE hPrinter, WCHAR *pValueName, DWORD Type, BYTE *pData, DWORD cbData)
{
    UNIMPLEMENTED;
    return ERROR_INVALID_FUNCTION;
}

DWORD
_RpcWaitForPrinterChange(WINSPOOL_PRINTER_HANDLE hPrinter, DWORD Flags, DWORD *pFlags)
{
    UNIMPLEMENTED;
    return ERROR_INVALID_FUNCTION;
}

DWORD
_RpcClosePrinter(WINSPOOL_PRINTER_HANDLE *phPrinter)
{
    UNIMPLEMENTED;
    return ERROR_INVALID_FUNCTION;
}

DWORD
_RpcAddForm(WINSPOOL_PRINTER_HANDLE hPrinter, WINSPOOL_FORM_CONTAINER *pFormInfoContainer)
{
    UNIMPLEMENTED;
    return ERROR_INVALID_FUNCTION;
}

DWORD
_RpcDeleteForm(WINSPOOL_PRINTER_HANDLE hPrinter, WCHAR *pFormName)
{
    UNIMPLEMENTED;
    return ERROR_INVALID_FUNCTION;
}

DWORD
_RpcGetForm(WINSPOOL_PRINTER_HANDLE hPrinter, WCHAR *pFormName, DWORD Level, BYTE *pForm, DWORD cbBuf, DWORD *pcbNeeded)
{
    UNIMPLEMENTED;
    return ERROR_INVALID_FUNCTION;
}

DWORD
_RpcSetForm(WINSPOOL_PRINTER_HANDLE hPrinter, WCHAR *pFormName, WINSPOOL_FORM_CONTAINER *pFormInfoContainer)
{
    UNIMPLEMENTED;
    return ERROR_INVALID_FUNCTION;
}

DWORD
_RpcEnumForms(WINSPOOL_PRINTER_HANDLE hPrinter, DWORD Level, BYTE *pForm, DWORD cbBuf, DWORD *pcbNeeded, DWORD *pcReturned)
{
    UNIMPLEMENTED;
    return ERROR_INVALID_FUNCTION;
}

DWORD
_RpcEnumPorts(WINSPOOL_HANDLE pName, DWORD Level, BYTE *pPort, DWORD cbBuf, DWORD *pcbNeeded, DWORD *pcReturned)
{
    UNIMPLEMENTED;
    return ERROR_INVALID_FUNCTION;
}

DWORD
_RpcEnumMonitors(WINSPOOL_HANDLE pName, DWORD Level, BYTE *pMonitor, DWORD cbBuf, DWORD *pcbNeeded, DWORD *pcReturned)
{
    UNIMPLEMENTED;
    return ERROR_INVALID_FUNCTION;
}

DWORD
_RpcAddPort(WINSPOOL_HANDLE pName, ULONG_PTR hWnd, WCHAR *pMonitorName)
{
    UNIMPLEMENTED;
    return ERROR_INVALID_FUNCTION;
}

DWORD
_RpcConfigurePort(WINSPOOL_HANDLE pName, ULONG_PTR hWnd, WCHAR *pPortName)
{
    UNIMPLEMENTED;
    return ERROR_INVALID_FUNCTION;
}

DWORD
_RpcDeletePort(WINSPOOL_HANDLE pName, ULONG_PTR hWnd, WCHAR *pPortName)
{
    UNIMPLEMENTED;
    return ERROR_INVALID_FUNCTION;
}

DWORD
_RpcCreatePrinterIC(WINSPOOL_PRINTER_HANDLE hPrinter, WINSPOOL_GDI_HANDLE *pHandle, WINSPOOL_DEVMODE_CONTAINER *pDevModeContainer)
{
    UNIMPLEMENTED;
    return ERROR_INVALID_FUNCTION;
}

DWORD
_RpcPlayGdiScriptOnPrinterIC(WINSPOOL_GDI_HANDLE hPrinterIC, BYTE *pIn, DWORD cIn, BYTE *pOut, DWORD cOut, DWORD ul)
{
    UNIMPLEMENTED;
    return ERROR_INVALID_FUNCTION;
}

DWORD
_RpcDeletePrinterIC(WINSPOOL_GDI_HANDLE *phPrinterIC)
{
    UNIMPLEMENTED;
    return ERROR_INVALID_FUNCTION;
}

DWORD
_RpcAddPrinterConnection(WINSPOOL_HANDLE pName)
{
    UNIMPLEMENTED;
    return ERROR_INVALID_FUNCTION;
}

DWORD
_RpcDeletePrinterConnection(WINSPOOL_HANDLE pName)
{
    UNIMPLEMENTED;
    return ERROR_INVALID_FUNCTION;
}

DWORD
_RpcPrinterMessageBox(WINSPOOL_PRINTER_HANDLE hPrinter, DWORD Error, ULONG_PTR hWnd, WCHAR *pText, WCHAR *pCaption, DWORD dwType)
{
    UNIMPLEMENTED;
    return ERROR_INVALID_FUNCTION;
}

DWORD
_RpcAddMonitor(WINSPOOL_HANDLE pName, WINSPOOL_MONITOR_CONTAINER *pMonitorContainer)
{
    UNIMPLEMENTED;
    return ERROR_INVALID_FUNCTION;
}

DWORD
_RpcDeleteMonitor(WINSPOOL_HANDLE pName, WCHAR *pEnvironment, WCHAR *pMonitorName)
{
    UNIMPLEMENTED;
    return ERROR_INVALID_FUNCTION;
}

DWORD
_RpcDeletePrintProcessor(WINSPOOL_HANDLE pName, WCHAR *pEnvironment, WCHAR *pPrintProcessorName)
{
    UNIMPLEMENTED;
    return ERROR_INVALID_FUNCTION;
}

DWORD
_RpcAddPrintProvidor(WINSPOOL_HANDLE pName, WINSPOOL_PROVIDOR_CONTAINER *pProvidorContainer)
{
    UNIMPLEMENTED;
    return ERROR_INVALID_FUNCTION;
}

DWORD
_RpcDeletePrintProvidor(WINSPOOL_HANDLE pName, WCHAR *pEnvironment, WCHAR *pPrintProviderName)
{
    UNIMPLEMENTED;
    return ERROR_INVALID_FUNCTION;
}

DWORD
_RpcResetPrinter(WINSPOOL_PRINTER_HANDLE hPrinter, WCHAR *pDatatype, WINSPOOL_DEVMODE_CONTAINER *pDevModeContainer)
{
    UNIMPLEMENTED;
    return ERROR_INVALID_FUNCTION;
}

DWORD
_RpcGetPrinterDriver2(WINSPOOL_PRINTER_HANDLE hPrinter, WCHAR *pEnvironment, DWORD Level, BYTE *pDriver, DWORD cbBuf, DWORD *pcbNeeded, DWORD dwClientMajorVersion, DWORD dwClientMinorVersion, DWORD *pdwServerMaxVersion, DWORD *pdwServerMinVersion)
{
    UNIMPLEMENTED;
    return ERROR_INVALID_FUNCTION;
}

DWORD
_RpcClientFindFirstPrinterChangeNotification()
{
    UNIMPLEMENTED;
    return ERROR_INVALID_FUNCTION;
}

DWORD
_RpcFindNextPrinterChangeNotification()
{
    UNIMPLEMENTED;
    return ERROR_INVALID_FUNCTION;
}

DWORD
_RpcFindClosePrinterChangeNotification()
{
    UNIMPLEMENTED;
    return ERROR_INVALID_FUNCTION;
}

DWORD
_RpcRouterFindFirstPrinterChangeNotificationOld()
{
    UNIMPLEMENTED;
    return ERROR_INVALID_FUNCTION;
}

DWORD
_RpcReplyOpenPrinter(WINSPOOL_HANDLE pMachine, WINSPOOL_PRINTER_HANDLE *phPrinterNotify, DWORD dwPrinterRemote, DWORD dwType, DWORD cbBuffer, BYTE *pBuffer)
{
    UNIMPLEMENTED;
    return ERROR_INVALID_FUNCTION;
}

DWORD
_RpcRouterReplyPrinter(WINSPOOL_PRINTER_HANDLE hNotify, DWORD fdwFlags, DWORD cbBuffer, BYTE *pBuffer)
{
    UNIMPLEMENTED;
    return ERROR_INVALID_FUNCTION;
}

DWORD
_RpcReplyClosePrinter(WINSPOOL_PRINTER_HANDLE *phNotify)
{
    UNIMPLEMENTED;
    return ERROR_INVALID_FUNCTION;
}

DWORD
_RpcAddPortEx(WINSPOOL_HANDLE pName, WINSPOOL_PORT_CONTAINER *pPortContainer, WINSPOOL_PORT_VAR_CONTAINER *pPortVarContainer, WCHAR *pMonitorName)
{
    UNIMPLEMENTED;
    return ERROR_INVALID_FUNCTION;
}

DWORD
_RpcRemoteFindFirstPrinterChangeNotification(WINSPOOL_PRINTER_HANDLE hPrinter, DWORD fdwFlags, DWORD fdwOptions, WCHAR *pszLocalMachine, DWORD dwPrinterLocal, DWORD cbBuffer, BYTE *pBuffer)
{
    UNIMPLEMENTED;
    return ERROR_INVALID_FUNCTION;
}

DWORD
_RpcResetPrinterEx()
{
    UNIMPLEMENTED;
    return ERROR_INVALID_FUNCTION;
}

DWORD
_RpcRemoteFindFirstPrinterChangeNotificationEx(WINSPOOL_PRINTER_HANDLE hPrinter, DWORD fdwFlags, DWORD fdwOptions, WCHAR *pszLocalMachine, DWORD dwPrinterLocal, WINSPOOL_V2_NOTIFY_OPTIONS *pOptions)
{
    UNIMPLEMENTED;
    return ERROR_INVALID_FUNCTION;
}

DWORD
_RpcRouterReplyPrinterEx(WINSPOOL_PRINTER_HANDLE hNotify, DWORD dwColor, DWORD fdwFlags, DWORD *pdwResult, DWORD dwReplyType, WINSPOOL_V2_UREPLY_PRINTER Reply)
{
    UNIMPLEMENTED;
    return ERROR_INVALID_FUNCTION;
}

DWORD
_RpcRouterRefreshPrinterChangeNotification(WINSPOOL_PRINTER_HANDLE hPrinter, DWORD dwColor, WINSPOOL_V2_NOTIFY_OPTIONS *pOptions, WINSPOOL_V2_NOTIFY_INFO **ppInfo)
{
    UNIMPLEMENTED;
    return ERROR_INVALID_FUNCTION;
}

DWORD
_RpcSetAllocFailCount()
{
    UNIMPLEMENTED;
    return ERROR_INVALID_FUNCTION;
}

DWORD
_RpcOpenPrinterEx(WINSPOOL_HANDLE pPrinterName, WINSPOOL_PRINTER_HANDLE *pHandle, WCHAR *pDatatype, WINSPOOL_DEVMODE_CONTAINER *pDevModeContainer, DWORD AccessRequired, WINSPOOL_SPLCLIENT_CONTAINER *pClientInfo)
{
    UNIMPLEMENTED;
    return ERROR_INVALID_FUNCTION;
}

DWORD
_RpcAddPrinterEx(WINSPOOL_HANDLE pName, WINSPOOL_PRINTER_CONTAINER *pPrinterContainer, WINSPOOL_DEVMODE_CONTAINER *pDevModeContainer, WINSPOOL_SECURITY_CONTAINER *pSecurityContainer, WINSPOOL_SPLCLIENT_CONTAINER *pClientInfo, WINSPOOL_PRINTER_HANDLE *pHandle)
{
    UNIMPLEMENTED;
    return ERROR_INVALID_FUNCTION;
}

DWORD
_RpcSetPort(WINSPOOL_HANDLE pName, WCHAR *pPortName, WINSPOOL_PORT_CONTAINER *pPortContainer)
{
    UNIMPLEMENTED;
    return ERROR_INVALID_FUNCTION;
}

DWORD
_RpcEnumPrinterData(WINSPOOL_PRINTER_HANDLE hPrinter, DWORD dwIndex, WCHAR *pValueName, DWORD cbValueName, DWORD *pcbValueName, DWORD *pType, BYTE *pData, DWORD cbData, DWORD *pcbData)
{
    UNIMPLEMENTED;
    return ERROR_INVALID_FUNCTION;
}

DWORD
_RpcDeletePrinterData(WINSPOOL_PRINTER_HANDLE hPrinter, WCHAR *pValueName)
{
    UNIMPLEMENTED;
    return ERROR_INVALID_FUNCTION;
}

DWORD
_RpcClusterSplOpen()
{
    UNIMPLEMENTED;
    return ERROR_INVALID_FUNCTION;
}

DWORD
_RpcClusterSplClose()
{
    UNIMPLEMENTED;
    return ERROR_INVALID_FUNCTION;
}

DWORD
_RpcClusterSplIsAlive()
{
    UNIMPLEMENTED;
    return ERROR_INVALID_FUNCTION;
}

DWORD
_RpcSetPrinterDataEx(WINSPOOL_PRINTER_HANDLE hPrinter, const WCHAR *pKeyName, const WCHAR *pValueName, DWORD Type, BYTE *pData, DWORD cbData)
{
    UNIMPLEMENTED;
    return ERROR_INVALID_FUNCTION;
}

DWORD
_RpcGetPrinterDataEx(WINSPOOL_PRINTER_HANDLE hPrinter, const WCHAR *pKeyName, const WCHAR *pValueName, DWORD *pType, BYTE *pData, DWORD nSize, DWORD *pcbNeeded)
{
    UNIMPLEMENTED;
    return ERROR_INVALID_FUNCTION;
}

DWORD
_RpcEnumPrinterDataEx(WINSPOOL_PRINTER_HANDLE hPrinter, const WCHAR *pKeyName, BYTE *pEnumValues, DWORD cbEnumValues, DWORD *pcbEnumValues, DWORD *pnEnumValues)
{
    UNIMPLEMENTED;
    return ERROR_INVALID_FUNCTION;
}

DWORD
_RpcEnumPrinterKey(WINSPOOL_PRINTER_HANDLE hPrinter, const WCHAR *pKeyName, WCHAR *pSubkey, DWORD cbSubkey, DWORD *pcbSubkey)
{
    UNIMPLEMENTED;
    return ERROR_INVALID_FUNCTION;
}

DWORD
_RpcDeletePrinterDataEx(WINSPOOL_PRINTER_HANDLE hPrinter, const WCHAR *pKeyName, const WCHAR *pValueName)
{
    UNIMPLEMENTED;
    return ERROR_INVALID_FUNCTION;
}

DWORD
_RpcDeletePrinterKey(WINSPOOL_PRINTER_HANDLE hPrinter, const WCHAR *pKeyName)
{
    UNIMPLEMENTED;
    return ERROR_INVALID_FUNCTION;
}

DWORD
_RpcSeekPrinter()
{
    UNIMPLEMENTED;
    return ERROR_INVALID_FUNCTION;
}

DWORD
_RpcDeletePrinterDriverEx(WINSPOOL_HANDLE pName, WCHAR *pEnvironment, WCHAR *pDriverName, DWORD dwDeleteFlag, DWORD dwVersionNum)
{
    UNIMPLEMENTED;
    return ERROR_INVALID_FUNCTION;
}

DWORD
_RpcAddPerMachineConnection(WINSPOOL_HANDLE pServer, const WCHAR *pPrinterName, const WCHAR *pPrintServer, const WCHAR *pProvider)
{
    UNIMPLEMENTED;
    return ERROR_INVALID_FUNCTION;
}

DWORD
_RpcDeletePerMachineConnection(WINSPOOL_HANDLE pServer, const WCHAR *pPrinterName)
{
    UNIMPLEMENTED;
    return ERROR_INVALID_FUNCTION;
}

DWORD
_RpcEnumPerMachineConnections(WINSPOOL_HANDLE pServer, BYTE *pPrinterEnum, DWORD cbBuf, DWORD *pcbNeeded, DWORD *pcReturned)
{
    UNIMPLEMENTED;
    return ERROR_INVALID_FUNCTION;
}

DWORD
_RpcXcvData(WINSPOOL_PRINTER_HANDLE hXcv, const WCHAR *pszDataName, BYTE *pInputData, DWORD cbInputData, BYTE *pOutputData, DWORD cbOutputData, DWORD *pcbOutputNeeded, DWORD *pdwStatus)
{
    UNIMPLEMENTED;
    return ERROR_INVALID_FUNCTION;
}

DWORD
_RpcAddPrinterDriverEx(WINSPOOL_HANDLE pName, WINSPOOL_DRIVER_CONTAINER *pDriverContainer, DWORD dwFileCopyFlags)
{
    UNIMPLEMENTED;
    return ERROR_INVALID_FUNCTION;
}

DWORD
_RpcSplOpenPrinter()
{
    UNIMPLEMENTED;
    return ERROR_INVALID_FUNCTION;
}

DWORD
_RpcGetSpoolFileInfo()
{
    UNIMPLEMENTED;
    return ERROR_INVALID_FUNCTION;
}

DWORD
_RpcCommitSpoolData()
{
    UNIMPLEMENTED;
    return ERROR_INVALID_FUNCTION;
}

DWORD
_RpcCloseSpoolFileHandle()
{
    UNIMPLEMENTED;
    return ERROR_INVALID_FUNCTION;
}

DWORD
_RpcFlushPrinter(WINSPOOL_PRINTER_HANDLE hPrinter, BYTE *pBuf, DWORD cbBuf, DWORD *pcWritten, DWORD cSleep)
{
    UNIMPLEMENTED;
    return ERROR_INVALID_FUNCTION;
}

DWORD
_RpcSendRecvBidiData(WINSPOOL_PRINTER_HANDLE hPrinter, const WCHAR *pAction, WINSPOOL_BIDI_REQUEST_CONTAINER *pReqData, WINSPOOL_BIDI_RESPONSE_CONTAINER **ppRespData)
{
    UNIMPLEMENTED;
    return ERROR_INVALID_FUNCTION;
}

DWORD
_RpcAddDriverCatalog()
{
    UNIMPLEMENTED;
    return ERROR_INVALID_FUNCTION;
}
