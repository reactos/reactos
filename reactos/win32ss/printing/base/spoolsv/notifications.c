/*
 * PROJECT:     ReactOS Print Spooler Service
 * LICENSE:     GNU GPLv2 or any later version as published by the Free Software Foundation
 * PURPOSE:     Functions related to Printer Configuration Data
 * COPYRIGHT:   Copyright 2015 Colin Finck <colin@reactos.org>
 */

#include "precomp.h"

DWORD
_RpcClientFindFirstPrinterChangeNotification()
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
_RpcFindNextPrinterChangeNotification()
{
    UNIMPLEMENTED;
    return ERROR_INVALID_FUNCTION;
}

DWORD
_RpcRemoteFindFirstPrinterChangeNotification(WINSPOOL_PRINTER_HANDLE hPrinter, DWORD fdwFlags, DWORD fdwOptions, WCHAR* pszLocalMachine, DWORD dwPrinterLocal, DWORD cbBuffer, BYTE* pBuffer)
{
    UNIMPLEMENTED;
    return ERROR_INVALID_FUNCTION;
}

DWORD
_RpcRemoteFindFirstPrinterChangeNotificationEx(WINSPOOL_PRINTER_HANDLE hPrinter, DWORD fdwFlags, DWORD fdwOptions, WCHAR* pszLocalMachine, DWORD dwPrinterLocal, WINSPOOL_V2_NOTIFY_OPTIONS* pOptions)
{
    UNIMPLEMENTED;
    return ERROR_INVALID_FUNCTION;
}

DWORD
_RpcReplyClosePrinter(WINSPOOL_PRINTER_HANDLE* phNotify)
{
    UNIMPLEMENTED;
    return ERROR_INVALID_FUNCTION;
}

DWORD
_RpcReplyOpenPrinter(WINSPOOL_HANDLE pMachine, WINSPOOL_PRINTER_HANDLE* phPrinterNotify, DWORD dwPrinterRemote, DWORD dwType, DWORD cbBuffer, BYTE* pBuffer)
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
_RpcRouterRefreshPrinterChangeNotification(WINSPOOL_PRINTER_HANDLE hPrinter, DWORD dwColor, WINSPOOL_V2_NOTIFY_OPTIONS* pOptions, WINSPOOL_V2_NOTIFY_INFO** ppInfo)
{
    UNIMPLEMENTED;
    return ERROR_INVALID_FUNCTION;
}

DWORD
_RpcRouterReplyPrinter(WINSPOOL_PRINTER_HANDLE hNotify, DWORD fdwFlags, DWORD cbBuffer, BYTE* pBuffer)
{
    UNIMPLEMENTED;
    return ERROR_INVALID_FUNCTION;
}

DWORD
_RpcRouterReplyPrinterEx(WINSPOOL_PRINTER_HANDLE hNotify, DWORD dwColor, DWORD fdwFlags, DWORD* pdwResult, DWORD dwReplyType, WINSPOOL_V2_UREPLY_PRINTER Reply)
{
    UNIMPLEMENTED;
    return ERROR_INVALID_FUNCTION;
}

DWORD
_RpcWaitForPrinterChange(WINSPOOL_PRINTER_HANDLE hPrinter, DWORD Flags, DWORD* pFlags)
{
    UNIMPLEMENTED;
    return ERROR_INVALID_FUNCTION;
}
