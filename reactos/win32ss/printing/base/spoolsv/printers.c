/*
 * PROJECT:     ReactOS Print Spooler Service
 * LICENSE:     GNU GPLv2 or any later version as published by the Free Software Foundation
 * PURPOSE:     Functions related to Printers and printing
 * COPYRIGHT:   Copyright 2015-2017 Colin Finck <colin@reactos.org>
 */

#include "precomp.h"

static void
_MarshallDownPrinterInfo(PBYTE pPrinterInfo, DWORD Level)
{
    PPRINTER_INFO_1W pPrinterInfo1;
    PPRINTER_INFO_2W pPrinterInfo2;

    // Replace absolute pointer addresses in the output by relative offsets.
    if (Level == 1)
    {
        pPrinterInfo1 = (PPRINTER_INFO_1W)pPrinterInfo;

        pPrinterInfo1->pName = (PWSTR)((ULONG_PTR)pPrinterInfo1->pName - (ULONG_PTR)pPrinterInfo1);
        pPrinterInfo1->pDescription = (PWSTR)((ULONG_PTR)pPrinterInfo1->pDescription - (ULONG_PTR)pPrinterInfo1);
        pPrinterInfo1->pComment = (PWSTR)((ULONG_PTR)pPrinterInfo1->pComment - (ULONG_PTR)pPrinterInfo1);
    }
    else if (Level == 2)
    {
        pPrinterInfo2 = (PPRINTER_INFO_2W)pPrinterInfo;

        pPrinterInfo2->pPrinterName = (PWSTR)((ULONG_PTR)pPrinterInfo2->pPrinterName - (ULONG_PTR)pPrinterInfo2);
        pPrinterInfo2->pShareName = (PWSTR)((ULONG_PTR)pPrinterInfo2->pShareName - (ULONG_PTR)pPrinterInfo2);
        pPrinterInfo2->pPortName = (PWSTR)((ULONG_PTR)pPrinterInfo2->pPortName - (ULONG_PTR)pPrinterInfo2);
        pPrinterInfo2->pDriverName = (PWSTR)((ULONG_PTR)pPrinterInfo2->pDriverName - (ULONG_PTR)pPrinterInfo2);
        pPrinterInfo2->pComment = (PWSTR)((ULONG_PTR)pPrinterInfo2->pComment - (ULONG_PTR)pPrinterInfo2);
        pPrinterInfo2->pLocation = (PWSTR)((ULONG_PTR)pPrinterInfo2->pLocation - (ULONG_PTR)pPrinterInfo2);
        pPrinterInfo2->pDevMode = (PDEVMODEW)((ULONG_PTR)pPrinterInfo2->pDevMode - (ULONG_PTR)pPrinterInfo2);
        pPrinterInfo2->pSepFile = (PWSTR)((ULONG_PTR)pPrinterInfo2->pSepFile - (ULONG_PTR)pPrinterInfo2);
        pPrinterInfo2->pPrintProcessor = (PWSTR)((ULONG_PTR)pPrinterInfo2->pPrintProcessor - (ULONG_PTR)pPrinterInfo2);
        pPrinterInfo2->pDatatype = (PWSTR)((ULONG_PTR)pPrinterInfo2->pDatatype - (ULONG_PTR)pPrinterInfo2);
        pPrinterInfo2->pParameters = (PWSTR)((ULONG_PTR)pPrinterInfo2->pParameters - (ULONG_PTR)pPrinterInfo2);

        if (pPrinterInfo2->pServerName)
            pPrinterInfo2->pServerName = (PWSTR)((ULONG_PTR)pPrinterInfo2->pServerName - (ULONG_PTR)pPrinterInfo2);

        if (pPrinterInfo2->pSecurityDescriptor)
            pPrinterInfo2->pSecurityDescriptor = (PWSTR)((ULONG_PTR)pPrinterInfo2->pSecurityDescriptor - (ULONG_PTR)pPrinterInfo2);
    }
}

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
    PBYTE pPrinterEnumAligned;

    dwErrorCode = RpcImpersonateClient(NULL);
    if (dwErrorCode != ERROR_SUCCESS)
    {
        ERR("RpcImpersonateClient failed with error %lu!\n", dwErrorCode);
        return dwErrorCode;
    }

    pPrinterEnumAligned = AlignRpcPtr(pPrinterEnum, &cbBuf);
    EnumPrintersW(Flags, Name, Level, pPrinterEnumAligned, cbBuf, pcbNeeded, pcReturned);
    dwErrorCode = GetLastError();

    if (dwErrorCode == ERROR_SUCCESS)
    {
        DWORD i;
        PBYTE p = pPrinterEnumAligned;

        // Replace absolute pointer addresses in the output by relative offsets.
        for (i = 0; i < *pcReturned; i++)
        {
            _MarshallDownPrinterInfo(p, Level);

            if (Level == 1)
                p += sizeof(PRINTER_INFO_1W);
            else if (Level == 2)
                p += sizeof(PRINTER_INFO_2W);
        }
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
    UNIMPLEMENTED;
    return ERROR_INVALID_FUNCTION;
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

    ReadPrinter(hPrinter, pBuf, cbBuf, pcNoBytesRead);
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
_RpcResetPrinterEx()
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

    StartPagePrinter(hPrinter);
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

    WritePrinter(hPrinter, pBuf, cbBuf, pcWritten);
    dwErrorCode = GetLastError();

    RpcRevertToSelf();
    return dwErrorCode;
}
