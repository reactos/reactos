/*
 * PROJECT:     ReactOS Print Spooler Service
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Functions related to Printers and printing
 * COPYRIGHT:   Copyright 2015-2018 Colin Finck (colin@reactos.org)
 */

#include "precomp.h"
#include <marshalling/printers.h>

DWORD
_RpcAbortPrinter(WINSPOOL_PRINTER_HANDLE hPrinter)
{
    UNIMPLEMENTED;
    return ERROR_INVALID_FUNCTION;
}

DWORD
_RpcAddPrinter(WINSPOOL_HANDLE pName, WINSPOOL_PRINTER_CONTAINER* pPrinterContainer, WINSPOOL_DEVMODE_CONTAINER* pDevModeContainer, WINSPOOL_SECURITY_CONTAINER* pSecurityContainer, WINSPOOL_PRINTER_HANDLE* pHandle)
{
    UNIMPLEMENTED;
    return ERROR_INVALID_FUNCTION;
}

DWORD
_RpcAddPrinterEx(WINSPOOL_HANDLE pName, WINSPOOL_PRINTER_CONTAINER* pPrinterContainer, WINSPOOL_DEVMODE_CONTAINER* pDevModeContainer, WINSPOOL_SECURITY_CONTAINER* pSecurityContainer, WINSPOOL_SPLCLIENT_CONTAINER* pClientInfo, WINSPOOL_PRINTER_HANDLE* pHandle)
{
    UNIMPLEMENTED;
    return ERROR_INVALID_FUNCTION;
}

DWORD
_RpcClosePrinter(WINSPOOL_PRINTER_HANDLE* phPrinter)
{
    DWORD dwErrorCode;

    dwErrorCode = RpcImpersonateClient(NULL);
    if (dwErrorCode != ERROR_SUCCESS)
    {
        ERR("RpcImpersonateClient failed with error %lu!\n", dwErrorCode);
        return dwErrorCode;
    }

    if (ClosePrinter(*phPrinter))
        *phPrinter = NULL;
    else
        dwErrorCode = GetLastError();

    RpcRevertToSelf();
    return dwErrorCode;
}

DWORD
_RpcDeletePrinter(WINSPOOL_PRINTER_HANDLE hPrinter)
{
    UNIMPLEMENTED;
    return ERROR_INVALID_FUNCTION;
}

DWORD
_RpcEndDocPrinter(WINSPOOL_PRINTER_HANDLE hPrinter)
{
    DWORD dwErrorCode;

    dwErrorCode = RpcImpersonateClient(NULL);
    if (dwErrorCode != ERROR_SUCCESS)
    {
        ERR("RpcImpersonateClient failed with error %lu!\n", dwErrorCode);
        return dwErrorCode;
    }

    if (!EndDocPrinter(hPrinter))
        dwErrorCode = GetLastError();

    RpcRevertToSelf();
    return dwErrorCode;
}

DWORD
_RpcEndPagePrinter(WINSPOOL_PRINTER_HANDLE hPrinter)
{
    DWORD dwErrorCode;

    dwErrorCode = RpcImpersonateClient(NULL);
    if (dwErrorCode != ERROR_SUCCESS)
    {
        ERR("RpcImpersonateClient failed with error %lu!\n", dwErrorCode);
        return dwErrorCode;
    }

    if (!EndPagePrinter(hPrinter))
        dwErrorCode = GetLastError();

    RpcRevertToSelf();
    return dwErrorCode;
}

DWORD
_RpcEnumPrinters(DWORD Flags, WINSPOOL_HANDLE Name, DWORD Level, BYTE* pPrinterEnum, DWORD cbBuf, DWORD* pcbNeeded, DWORD* pcReturned)
{
    DWORD dwErrorCode;
    PBYTE pPrinterEnumAligned;

    dwErrorCode = RpcImpersonateClient(NULL);
    if (dwErrorCode != ERROR_SUCCESS)
    {
        ERR("RpcImpersonateClient failed with error %lu!\n", dwErrorCode);
        return dwErrorCode;
    }

    pPrinterEnumAligned = AlignRpcPtr(pPrinterEnum, &cbBuf);

    if (EnumPrintersW(Flags, Name, Level, pPrinterEnumAligned, cbBuf, pcbNeeded, pcReturned))
    {
        // Replace absolute pointer addresses in the output by relative offsets.
        ASSERT(Level <= 9);
        MarshallDownStructuresArray(pPrinterEnumAligned, *pcReturned, pPrinterInfoMarshalling[Level]->pInfo, pPrinterInfoMarshalling[Level]->cbStructureSize, TRUE);
    }
    else
    {
        dwErrorCode = GetLastError();
    }

    RpcRevertToSelf();
    UndoAlignRpcPtr(pPrinterEnum, pPrinterEnumAligned, cbBuf, pcbNeeded);

    return dwErrorCode;
}

DWORD
_RpcFlushPrinter(WINSPOOL_PRINTER_HANDLE hPrinter, BYTE* pBuf, DWORD cbBuf, DWORD* pcWritten, DWORD cSleep)
{
    UNIMPLEMENTED;
    return ERROR_INVALID_FUNCTION;
}

DWORD
_RpcGetPrinter(WINSPOOL_PRINTER_HANDLE hPrinter, DWORD Level, BYTE* pPrinter, DWORD cbBuf, DWORD* pcbNeeded)
{
    DWORD dwErrorCode;
    PBYTE pPrinterAligned;

    dwErrorCode = RpcImpersonateClient(NULL);
    if (dwErrorCode != ERROR_SUCCESS)
    {
        ERR("RpcImpersonateClient failed with error %lu!\n", dwErrorCode);
        return dwErrorCode;
    }

    pPrinterAligned = AlignRpcPtr(pPrinter, &cbBuf);

    if (GetPrinterW(hPrinter, Level, pPrinterAligned, cbBuf, pcbNeeded))
    {
        // Replace absolute pointer addresses in the output by relative offsets.
        ASSERT(Level <= 9);
        MarshallDownStructure(pPrinterAligned, pPrinterInfoMarshalling[Level]->pInfo, pPrinterInfoMarshalling[Level]->cbStructureSize, TRUE);
    }
    else
    {
        dwErrorCode = GetLastError();
    }

    RpcRevertToSelf();
    UndoAlignRpcPtr(pPrinter, pPrinterAligned, cbBuf, pcbNeeded);

    return dwErrorCode;
}

DWORD
_RpcOpenPrinter(WINSPOOL_HANDLE pPrinterName, WINSPOOL_PRINTER_HANDLE* phPrinter, WCHAR* pDatatype, WINSPOOL_DEVMODE_CONTAINER* pDevModeContainer, DWORD AccessRequired)
{
    DWORD dwErrorCode;
    PRINTER_DEFAULTSW Default;

    dwErrorCode = RpcImpersonateClient(NULL);
    if (dwErrorCode != ERROR_SUCCESS)
    {
        ERR("RpcImpersonateClient failed with error %lu!\n", dwErrorCode);
        return dwErrorCode;
    }

    Default.DesiredAccess = AccessRequired;
    Default.pDatatype = pDatatype;
    Default.pDevMode = (PDEVMODEW)pDevModeContainer->pDevMode;

    if (!OpenPrinterW(pPrinterName, phPrinter, &Default))
        dwErrorCode = GetLastError();

    RpcRevertToSelf();
    return dwErrorCode;
}

DWORD
_RpcOpenPrinterEx(WINSPOOL_HANDLE pPrinterName, WINSPOOL_PRINTER_HANDLE* pHandle, WCHAR* pDatatype, WINSPOOL_DEVMODE_CONTAINER* pDevModeContainer, DWORD AccessRequired, WINSPOOL_SPLCLIENT_CONTAINER* pClientInfo)
{
    UNIMPLEMENTED;
    return ERROR_INVALID_FUNCTION;
}

DWORD
_RpcReadPrinter(WINSPOOL_PRINTER_HANDLE hPrinter, BYTE* pBuf, DWORD cbBuf, DWORD* pcNoBytesRead)
{
    DWORD dwErrorCode;

    dwErrorCode = RpcImpersonateClient(NULL);
    if (dwErrorCode != ERROR_SUCCESS)
    {
        ERR("RpcImpersonateClient failed with error %lu!\n", dwErrorCode);
        return dwErrorCode;
    }

    if (!ReadPrinter(hPrinter, pBuf, cbBuf, pcNoBytesRead))
        dwErrorCode = GetLastError();

    RpcRevertToSelf();
    return dwErrorCode;
}

DWORD
_RpcResetPrinter(WINSPOOL_PRINTER_HANDLE hPrinter, WCHAR* pDatatype, WINSPOOL_DEVMODE_CONTAINER* pDevModeContainer)
{
    UNIMPLEMENTED;
    return ERROR_INVALID_FUNCTION;
}

DWORD
_RpcResetPrinterEx(VOID)
{
    UNIMPLEMENTED;
    return ERROR_INVALID_FUNCTION;
}

DWORD
_RpcSeekPrinter(VOID)
{
    UNIMPLEMENTED;
    return ERROR_INVALID_FUNCTION;
}

DWORD
_RpcSetPrinter(WINSPOOL_PRINTER_HANDLE hPrinter, WINSPOOL_PRINTER_CONTAINER* pPrinterContainer, WINSPOOL_DEVMODE_CONTAINER* pDevModeContainer, WINSPOOL_SECURITY_CONTAINER* pSecurityContainer, DWORD Command)
{
    UNIMPLEMENTED;
    return ERROR_INVALID_FUNCTION;
}

DWORD
_RpcStartDocPrinter(WINSPOOL_PRINTER_HANDLE hPrinter, WINSPOOL_DOC_INFO_CONTAINER* pDocInfoContainer, DWORD* pJobId)
{
    DWORD dwErrorCode;

    dwErrorCode = RpcImpersonateClient(NULL);
    if (dwErrorCode != ERROR_SUCCESS)
    {
        ERR("RpcImpersonateClient failed with error %lu!\n", dwErrorCode);
        return dwErrorCode;
    }

    *pJobId = StartDocPrinterW(hPrinter, pDocInfoContainer->Level, (PBYTE)pDocInfoContainer->DocInfo.pDocInfo1);
    dwErrorCode = GetLastError();

    RpcRevertToSelf();
    return dwErrorCode;
}

DWORD
_RpcStartPagePrinter(WINSPOOL_PRINTER_HANDLE hPrinter)
{
    DWORD dwErrorCode;

    dwErrorCode = RpcImpersonateClient(NULL);
    if (dwErrorCode != ERROR_SUCCESS)
    {
        ERR("RpcImpersonateClient failed with error %lu!\n", dwErrorCode);
        return dwErrorCode;
    }

    if (!StartPagePrinter(hPrinter))
        dwErrorCode = GetLastError();

    RpcRevertToSelf();
    return dwErrorCode;
}

DWORD
_RpcWritePrinter(WINSPOOL_PRINTER_HANDLE hPrinter, BYTE* pBuf, DWORD cbBuf, DWORD* pcWritten)
{
    DWORD dwErrorCode;

    dwErrorCode = RpcImpersonateClient(NULL);
    if (dwErrorCode != ERROR_SUCCESS)
    {
        ERR("RpcImpersonateClient failed with error %lu!\n", dwErrorCode);
        return dwErrorCode;
    }

    if (!WritePrinter(hPrinter, pBuf, cbBuf, pcWritten))
        dwErrorCode = GetLastError();

    RpcRevertToSelf();
    return dwErrorCode;
}
