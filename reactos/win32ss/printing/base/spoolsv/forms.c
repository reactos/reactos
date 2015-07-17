/*
 * PROJECT:     ReactOS Print Spooler Service
 * LICENSE:     GNU GPLv2 or any later version as published by the Free Software Foundation
 * PURPOSE:     Functions related to Forms
 * COPYRIGHT:   Copyright 2015 Colin Finck <colin@reactos.org>
 */

#include "precomp.h"

DWORD
_RpcAddForm(WINSPOOL_PRINTER_HANDLE hPrinter, WINSPOOL_FORM_CONTAINER* pFormInfoContainer)
{
    UNIMPLEMENTED;
    return ERROR_INVALID_FUNCTION;
}

DWORD
_RpcDeleteForm(WINSPOOL_PRINTER_HANDLE hPrinter, WCHAR* pFormName)
{
    UNIMPLEMENTED;
    return ERROR_INVALID_FUNCTION;
}

DWORD
_RpcEnumForms(WINSPOOL_PRINTER_HANDLE hPrinter, DWORD Level, BYTE* pForm, DWORD cbBuf, DWORD* pcbNeeded, DWORD* pcReturned)
{
    UNIMPLEMENTED;
    return ERROR_INVALID_FUNCTION;
}

DWORD
_RpcGetForm(WINSPOOL_PRINTER_HANDLE hPrinter, WCHAR* pFormName, DWORD Level, BYTE* pForm, DWORD cbBuf, DWORD* pcbNeeded)
{
    UNIMPLEMENTED;
    return ERROR_INVALID_FUNCTION;
}

DWORD
_RpcSetForm(WINSPOOL_PRINTER_HANDLE hPrinter, WCHAR* pFormName, WINSPOOL_FORM_CONTAINER* pFormInfoContainer)
{
    UNIMPLEMENTED;
    return ERROR_INVALID_FUNCTION;
}
