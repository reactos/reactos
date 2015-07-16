/*
 * PROJECT:     ReactOS Print Spooler Service
 * LICENSE:     GNU GPLv2 or any later version as published by the Free Software Foundation
 * PURPOSE:     Functions related to Printers and printing
 * COPYRIGHT:   Copyright 2015 Colin Finck <colin@reactos.org>
 */

#include "precomp.h"

DWORD
_RpcClosePrinter(WINSPOOL_PRINTER_HANDLE *phPrinter)
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

    dwErrorCode = GetLastError();

    RpcRevertToSelf();
    return dwErrorCode;
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

    EndDocPrinter(hPrinter);
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

    EndPagePrinter(hPrinter);
    dwErrorCode = GetLastError();

    RpcRevertToSelf();
    return dwErrorCode;
}

DWORD
_RpcEnumPrinters(DWORD Flags, WINSPOOL_HANDLE Name, DWORD Level, BYTE* pPrinterEnum, DWORD cbBuf, DWORD* pcbNeeded, DWORD* pcReturned)
{
    DWORD dwErrorCode;

    dwErrorCode = RpcImpersonateClient(NULL);
    if (dwErrorCode != ERROR_SUCCESS)
    {
        ERR("RpcImpersonateClient failed with error %lu!\n", dwErrorCode);
        return dwErrorCode;
    }

    EnumPrintersW(Flags, Name, Level, pPrinterEnum, cbBuf, pcbNeeded, pcReturned);
    dwErrorCode = GetLastError();

    RpcRevertToSelf();
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

    OpenPrinterW(pPrinterName, phPrinter, &Default);
    dwErrorCode = GetLastError();

    RpcRevertToSelf();
    return dwErrorCode;
}

DWORD
_RpcReadPrinter(WINSPOOL_PRINTER_HANDLE hPrinter, BYTE *pBuf, DWORD cbBuf, DWORD *pcNoBytesRead)
{
    DWORD dwErrorCode;

    dwErrorCode = RpcImpersonateClient(NULL);
    if (dwErrorCode != ERROR_SUCCESS)
    {
        ERR("RpcImpersonateClient failed with error %lu!\n", dwErrorCode);
        return dwErrorCode;
    }

    ReadPrinter(hPrinter, pBuf, cbBuf, pcNoBytesRead);
    dwErrorCode = GetLastError();

    RpcRevertToSelf();
    return dwErrorCode;
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

    StartPagePrinter(hPrinter);
    dwErrorCode = GetLastError();

    RpcRevertToSelf();
    return dwErrorCode;
}

DWORD
_RpcWritePrinter(WINSPOOL_PRINTER_HANDLE hPrinter, BYTE *pBuf, DWORD cbBuf, DWORD *pcWritten)
{
    DWORD dwErrorCode;

    dwErrorCode = RpcImpersonateClient(NULL);
    if (dwErrorCode != ERROR_SUCCESS)
    {
        ERR("RpcImpersonateClient failed with error %lu!\n", dwErrorCode);
        return dwErrorCode;
    }

    WritePrinter(hPrinter, pBuf, cbBuf, pcWritten);
    dwErrorCode = GetLastError();

    RpcRevertToSelf();
    return dwErrorCode;
}
