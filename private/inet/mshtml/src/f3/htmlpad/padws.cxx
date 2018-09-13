//+------------------------------------------------------------------------
//  
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1996 - 1997.
//  
//  File:       Working set test infrastructure
//  
//-------------------------------------------------------------------------

#include "padhead.hxx"

HRESULT CPadDoc::WsClear()
{
    HANDLE hProcess;

    hProcess = GetCurrentProcess();
    if (!hProcess)
    {
        RRETURN(E_FAIL);
    }

    RRETURN(DbgExWsClear(hProcess));
}

HRESULT CPadDoc::WsTakeSnapshot()
{
    HANDLE hProcess;

    hProcess = GetCurrentProcess();
    if (!hProcess)
    {
        RRETURN(E_FAIL);
    }

	RRETURN(DbgExWsTakeSnapshot(hProcess));
}

HRESULT CPadDoc::get_WsModule(long row, BSTR *pbstrModule)
{
    *pbstrModule = SysAllocString(DbgExWsGetModule(row));
    return S_OK;
}

HRESULT CPadDoc::get_WsSection(long row, BSTR *pbstrSection)
{
    *pbstrSection = SysAllocString(DbgExWsGetSection(row));
    return S_OK;
}

HRESULT CPadDoc::get_WsSize(long row, long *plWsSize)
{
    *plWsSize = DbgExWsSize(row);
    return S_OK;
}

HRESULT CPadDoc::get_WsCount(long *plCount)
{
    *plCount = DbgExWsCount();
    return S_OK;
}

HRESULT CPadDoc::get_WsTotal(long *plTotal)
{
    *plTotal = DbgExWsTotal();
    return S_OK;
}

HRESULT CPadDoc::WsStartDelta()
{
    HANDLE hProcess;

    hProcess = GetCurrentProcess();
    if (!hProcess)
    {
        RRETURN(E_FAIL);
    }

    RRETURN(DbgExWsStartDelta(hProcess));
}

HRESULT CPadDoc::WsEndDelta(long *pnPageFaults)
{
    HANDLE hProcess;

    hProcess = GetCurrentProcess();
    if (!hProcess)
    {
        RRETURN(E_FAIL);
    }

    *pnPageFaults = DbgExWsEndDelta(hProcess);
    return S_OK;
}
