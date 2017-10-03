/*
 * PROJECT:     ReactOS Print Spooler Service
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Xcv* functions
 * COPYRIGHT:   Copyright 2015 Colin Finck (colin@reactos.org)
 */

#include "precomp.h"

DWORD
_RpcXcvData(WINSPOOL_PRINTER_HANDLE hXcv, const WCHAR* pszDataName, BYTE* pInputData, DWORD cbInputData, BYTE* pOutputData, DWORD cbOutputData, DWORD* pcbOutputNeeded, DWORD* pdwStatus)
{
    UNIMPLEMENTED;
    return ERROR_INVALID_FUNCTION;
}
