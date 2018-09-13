//---------------------------------------------------------------------------
//
// Copyright (c) Microsoft Corporation 
//
// File: appsize.cpp
//
// Compute the size of an application 
// 
// History:
//         2-17-98  by dli implemented CAppFolderSize
//------------------------------------------------------------------------
#include "priv.h"

// Do not build this file if on Win9X or NT4
#ifndef DOWNLEVEL_PLATFORM

#include "appsize.h"

// NOTE: CAppFolderSize and CAppFolderFinder are very similar to C*TreeWalkCB in Shell32.

CAppFolderSize::CAppFolderSize(ULONGLONG * puSize): _cRef(1), _puSize(puSize)
{
    CoInitialize(0);
}

CAppFolderSize::~CAppFolderSize()
{
    if (_pstw)
        _pstw->Release();
    CoUninitialize();
}

HRESULT CAppFolderSize::QueryInterface(REFIID riid, LPVOID * ppvOut)
{ 
    static const QITAB qit[] = {
        QITABENT(CAppFolderSize, IShellTreeWalkerCallBack),       // IID_IShellTreeWalkerCallBack
        { 0 },
    };

    return QISearch(this, qit, riid, ppvOut);
}


ULONG CAppFolderSize::AddRef()
{
    _cRef++;
    return _cRef;
}

ULONG CAppFolderSize::Release()
{
    _cRef--;
    if (_cRef > 0)
        return _cRef;

    delete this;
    return 0;
}

HRESULT CAppFolderSize::Initialize()
{
    return CoCreateInstance(CLSID_CShellTreeWalker, NULL, CLSCTX_INPROC_SERVER, IID_IShellTreeWalker, (LPVOID *)&_pstw);
}   

//
// IShellTreeWalkerCallBack::FoundFile 
//
HRESULT CAppFolderSize::FoundFile(LPCWSTR pwszFolder, LPTREEWALKERSTATS ptws, WIN32_FIND_DATAW * pwfd)
{
    HRESULT hres = S_OK;
    if (_puSize)
        *_puSize = ptws->ulActualSize;
    return hres;
}

HRESULT CAppFolderSize::EnterFolder(LPCWSTR pwszFolder, LPTREEWALKERSTATS ptws, WIN32_FIND_DATAW * pwfd)
{
    return E_NOTIMPL;
}

//
// IShellTreeWalkerCallBack::LeaveFolder
//
HRESULT CAppFolderSize::LeaveFolder(LPCWSTR pwszFolder, LPTREEWALKERSTATS ptws)
{
    return E_NOTIMPL;
}

//
// IShellTreeWalkerCallBack::HandleError 
//
HRESULT CAppFolderSize::HandleError(LPCWSTR pwszFolder, LPTREEWALKERSTATS ptws, HRESULT ErrorCode)
{
    return E_NOTIMPL;
}

#endif //DOWNLEVEL_PLATFORM
