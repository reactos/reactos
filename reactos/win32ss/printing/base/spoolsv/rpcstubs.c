/*
 * PROJECT:     ReactOS Print Spooler Service
 * LICENSE:     GNU GPLv2 or any later version as published by the Free Software Foundation
 * PURPOSE:     Unimplemented RPC calls
 * COPYRIGHT:   Copyright 2015 Colin Finck <colin@reactos.org>
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
_RpcSetAllocFailCount()
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
_RpcSendRecvBidiData(WINSPOOL_PRINTER_HANDLE hPrinter, const WCHAR* pAction, WINSPOOL_BIDI_REQUEST_CONTAINER* pReqData, WINSPOOL_BIDI_RESPONSE_CONTAINER** ppRespData)
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
