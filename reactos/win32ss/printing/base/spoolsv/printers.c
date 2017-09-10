/*
 * PROJECT:     ReactOS Print Spooler Service
 * LICENSE:     GNU GPLv2 or any later version as published by the Free Software Foundation
 * PURPOSE:     Functions related to Printers and printing
 * COPYRIGHT:   Copyright 2015-2017 Colin Finck <colin@reactos.org>
 */

#include "precomp.h"

static void
_MarshallDownPrinterInfo(PBYTE* ppPrinterInfo, DWORD Level)
{
    // Replace absolute pointer addresses in the output by relative offsets.
    if (Level == 0)
    {
        PPRINTER_INFO_STRESS pPrinterInfo0 = (PPRINTER_INFO_STRESS)(*ppPrinterInfo);

        pPrinterInfo0->pPrinterName = (PWSTR)((ULONG_PTR)pPrinterInfo0->pPrinterName - (ULONG_PTR)pPrinterInfo0);

        if (pPrinterInfo0->pServerName)
            pPrinterInfo0->pServerName = (PWSTR)((ULONG_PTR)pPrinterInfo0->pServerName - (ULONG_PTR)pPrinterInfo0);

        *ppPrinterInfo += sizeof(PRINTER_INFO_STRESS);
    }
    else if (Level == 1)
    {
        PPRINTER_INFO_1W pPrinterInfo1 = (PPRINTER_INFO_1W)(*ppPrinterInfo);

        pPrinterInfo1->pName = (PWSTR)((ULONG_PTR)pPrinterInfo1->pName - (ULONG_PTR)pPrinterInfo1);
        pPrinterInfo1->pDescription = (PWSTR)((ULONG_PTR)pPrinterInfo1->pDescription - (ULONG_PTR)pPrinterInfo1);
        pPrinterInfo1->pComment = (PWSTR)((ULONG_PTR)pPrinterInfo1->pComment - (ULONG_PTR)pPrinterInfo1);

        *ppPrinterInfo += sizeof(PRINTER_INFO_1W);
    }
    else if (Level == 2)
    {
        PPRINTER_INFO_2W pPrinterInfo2 = (PPRINTER_INFO_2W)(*ppPrinterInfo);

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
            pPrinterInfo2->pSecurityDescriptor = (PSECURITY_DESCRIPTOR)((ULONG_PTR)pPrinterInfo2->pSecurityDescriptor - (ULONG_PTR)pPrinterInfo2);

        *ppPrinterInfo += sizeof(PRINTER_INFO_2W);
    }
    else if (Level == 3)
    {
        PPRINTER_INFO_3 pPrinterInfo3 = (PPRINTER_INFO_3)(*ppPrinterInfo);

        pPrinterInfo3->pSecurityDescriptor = (PSECURITY_DESCRIPTOR)((ULONG_PTR)pPrinterInfo3->pSecurityDescriptor - (ULONG_PTR)pPrinterInfo3);

        *ppPrinterInfo += sizeof(PRINTER_INFO_3);
    }
    else if (Level == 4)
    {
        PPRINTER_INFO_4W pPrinterInfo4 = (PPRINTER_INFO_4W)(*ppPrinterInfo);

        pPrinterInfo4->pPrinterName = (PWSTR)((ULONG_PTR)pPrinterInfo4->pPrinterName - (ULONG_PTR)pPrinterInfo4);

        if (pPrinterInfo4->pServerName)
            pPrinterInfo4->pServerName = (PWSTR)((ULONG_PTR)pPrinterInfo4->pServerName - (ULONG_PTR)pPrinterInfo4);

        *ppPrinterInfo += sizeof(PRINTER_INFO_4W);
    }
    else if (Level == 5)
    {
        PPRINTER_INFO_5W pPrinterInfo5 = (PPRINTER_INFO_5W)(*ppPrinterInfo);

        pPrinterInfo5->pPrinterName = (PWSTR)((ULONG_PTR)pPrinterInfo5->pPrinterName - (ULONG_PTR)pPrinterInfo5);
        pPrinterInfo5->pPortName = (PWSTR)((ULONG_PTR)pPrinterInfo5->pPortName - (ULONG_PTR)pPrinterInfo5);

        *ppPrinterInfo += sizeof(PRINTER_INFO_5W);
    }
    else if (Level == 6)
    {
        *ppPrinterInfo += sizeof(PRINTER_INFO_6);
    }
    else if (Level == 7)
    {
        PPRINTER_INFO_7W pPrinterInfo7 = (PPRINTER_INFO_7W)(*ppPrinterInfo);

        if (pPrinterInfo7->pszObjectGUID)
            pPrinterInfo7->pszObjectGUID = (PWSTR)((ULONG_PTR)pPrinterInfo7->pszObjectGUID - (ULONG_PTR)pPrinterInfo7);

        *ppPrinterInfo += sizeof(PRINTER_INFO_7W);
    }
    else if (Level == 8)
    {
        PPRINTER_INFO_8W pPrinterInfo8 = (PPRINTER_INFO_8W)(*ppPrinterInfo);

        pPrinterInfo8->pDevMode = (PDEVMODEW)((ULONG_PTR)pPrinterInfo8->pDevMode - (ULONG_PTR)pPrinterInfo8);

        *ppPrinterInfo += sizeof(PRINTER_INFO_8W);
    }
    else if (Level == 9)
    {
        PPRINTER_INFO_9W pPrinterInfo9 = (PPRINTER_INFO_9W)(*ppPrinterInfo);

        pPrinterInfo9->pDevMode = (PDEVMODEW)((ULONG_PTR)pPrinterInfo9->pDevMode - (ULONG_PTR)pPrinterInfo9);

        *ppPrinterInfo += sizeof(PRINTER_INFO_9W);
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
        DWORD i;
        PBYTE p = pPrinterEnumAligned;

        for (i = 0; i < *pcReturned; i++)
            _MarshallDownPrinterInfo(&p, Level);
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
        PBYTE p = pPrinterAligned;
        _MarshallDownPrinterInfo(&p, Level);
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
