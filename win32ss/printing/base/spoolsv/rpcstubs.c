/*
 * PROJECT:     ReactOS Print Spooler Service
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Unimplemented RPC calls
 * COPYRIGHT:   Copyright 2015 Colin Finck (colin@reactos.org)
 */

#include "precomp.h"

DWORD
_RpcCreatePrinterIC(WINSPOOL_PRINTER_HANDLE hPrinter, WINSPOOL_GDI_HANDLE* pHandle, WINSPOOL_DEVMODE_CONTAINER* pDevModeContainer)
{
    UNIMPLEMENTED;
    return ERROR_INVALID_FUNCTION;
}

DWORD
_RpcPlayGdiScriptOnPrinterIC(WINSPOOL_GDI_HANDLE hPrinterIC, BYTE* pIn, DWORD cIn, BYTE* pOut, DWORD cOut, DWORD ul)
{
    UNIMPLEMENTED;
    return ERROR_INVALID_FUNCTION;
}

DWORD
_RpcDeletePrinterIC(WINSPOOL_GDI_HANDLE* phPrinterIC)
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
_RpcPrinterMessageBox(WINSPOOL_PRINTER_HANDLE hPrinter, DWORD Error, ULONG_PTR hWnd, WCHAR* pText, WCHAR* pCaption, DWORD dwType)
{
    UNIMPLEMENTED;
    return ERROR_INVALID_FUNCTION;
}

DWORD
_RpcSetAllocFailCount(VOID)
{
    UNIMPLEMENTED;
    return ERROR_INVALID_FUNCTION;
}

DWORD
_RpcClusterSplOpen(VOID)
{
    UNIMPLEMENTED;
    return ERROR_INVALID_FUNCTION;
}

DWORD
_RpcClusterSplClose(VOID)
{
    UNIMPLEMENTED;
    return ERROR_INVALID_FUNCTION;
}

DWORD
_RpcClusterSplIsAlive(VOID)
{
    UNIMPLEMENTED;
    return ERROR_INVALID_FUNCTION;
}

DWORD
_RpcAddPerMachineConnection(WINSPOOL_HANDLE pServer, const WCHAR* pPrinterName, const WCHAR* pPrintServer, const WCHAR* pProvider)
{
    UNIMPLEMENTED;
    return ERROR_INVALID_FUNCTION;
}

DWORD
_RpcDeletePerMachineConnection(WINSPOOL_HANDLE pServer, const WCHAR* pPrinterName)
{
    UNIMPLEMENTED;
    return ERROR_INVALID_FUNCTION;
}

DWORD
_RpcEnumPerMachineConnections(WINSPOOL_HANDLE pServer, BYTE* pPrinterEnum, DWORD cbBuf, DWORD* pcbNeeded, DWORD* pcReturned)
{
    UNIMPLEMENTED;
    return ERROR_INVALID_FUNCTION;
}

DWORD
_RpcSplOpenPrinter(VOID)
{
    UNIMPLEMENTED;
    return ERROR_INVALID_FUNCTION;
}

DWORD
_RpcGetSpoolFileInfo(VOID)
{
    UNIMPLEMENTED;
    return ERROR_INVALID_FUNCTION;
}

DWORD
_RpcCommitSpoolData(VOID)
{
    UNIMPLEMENTED;
    return ERROR_INVALID_FUNCTION;
}

DWORD
_RpcCloseSpoolFileHandle(VOID)
{
    UNIMPLEMENTED;
    return ERROR_INVALID_FUNCTION;
}

DWORD
_RpcSendRecvBidiData(WINSPOOL_PRINTER_HANDLE hPrinter, const WCHAR* pAction, WINSPOOL_BIDI_REQUEST_CONTAINER* pReqData, WINSPOOL_BIDI_RESPONSE_CONTAINER** ppRespData)
{
    UNIMPLEMENTED;
    return ERROR_INVALID_FUNCTION;
}

DWORD
_RpcAddDriverCatalog(VOID)
{
    UNIMPLEMENTED;
    return ERROR_INVALID_FUNCTION;
}
