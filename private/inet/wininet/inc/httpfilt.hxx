/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    httpfilt.hxx

Abstract:

    This file contains headers for the WININET HTTP filters.

    Contents:
        HttpFiltOpen
        HttpFiltClose
        HttpFiltOnRequest
        HttpFiltOnResponse
        HttpFiltOnTransactionComplete
        HttpFiltOnBlockingOps

Author:

    Rajeev Dujari (RajeevD) 01-Jul-1996

Revision History:

    01-Jul-1996  rajeevd
        Created

--*/

BOOL HttpFiltOpen (void);
BOOL HttpFiltClose (void);
BOOL HttpFiltOnRequest (LPVOID pRequest);
BOOL HttpFiltOnResponse (LPVOID pRequest);
BOOL HttpFiltOnTransactionComplete (HINTERNET hRequest);
BOOL HttpFiltOnBlockingOps(LPVOID pRequest, HINTERNET hRequest, HWND hwnd);
