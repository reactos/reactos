//---------------------------------------------------------------------------
//
// Copyright (c) Microsoft Corporation 
//
// File: pubenum.cpp
//
// The current order of enumeration is Legacy --> Darwin --> SMS
//
// History:
//         1-18-97  by dli
//------------------------------------------------------------------------
#include "priv.h"

// Do not build this file if on Win9X or NT4
#ifndef DOWNLEVEL_PLATFORM

#include "pubenum.h"


void _DestroyHdpaEnum(HDPA hdpaEnum)
{
    ASSERT(IsValidHDPA(hdpaEnum));
    IEnumPublishedApps * pepa;
    int idpa;
    for (idpa = 0; idpa < DPA_GetPtrCount(hdpaEnum); idpa++)
    {
        pepa = (IEnumPublishedApps *)DPA_GetPtr(hdpaEnum, idpa);
        if (EVAL(pepa))
            pepa->Release();
    }

    DPA_Destroy(hdpaEnum);
}

CShellEnumPublishedApps::CShellEnumPublishedApps(HDPA hdpaEnum) : _cRef(1), _hdpaEnum(hdpaEnum)
{
}

CShellEnumPublishedApps::~CShellEnumPublishedApps()
{
    if (_hdpaEnum)
        _DestroyHdpaEnum(_hdpaEnum);
}

// IEnumPublishedApps::QueryInterface
HRESULT CShellEnumPublishedApps::QueryInterface(REFIID riid, LPVOID * ppvOut)
{ 
    static const QITAB qit[] = {
        QITABENT(CShellEnumPublishedApps, IEnumPublishedApps),                  // IID_IEnumPublishedApps
        { 0 },
    };

    return QISearch(this, qit, riid, ppvOut);
}

// IEnumPublishedApps::AddRef
ULONG CShellEnumPublishedApps::AddRef()
{
    _cRef++;
    TraceMsg(TF_OBJLIFE, "CShellEnumPublishedApps()::AddRef called, new _cRef=%lX", _cRef);
    return _cRef;
}

// IEnumPublishedApps::Release
ULONG CShellEnumPublishedApps::Release()
{
    _cRef--;
    TraceMsg(TF_OBJLIFE, "CShellEnumPublishedApps()::Release called, new _cRef=%lX", _cRef);
    if (_cRef > 0)
        return _cRef;

    delete this;
    return 0;
}


// IEnumPublishedApps::Next
HRESULT CShellEnumPublishedApps::Next(IPublishedApp ** ppia)
{
    HRESULT hres = E_FAIL;
    if (_hdpaEnum)
    {
        IEnumPublishedApps * pepa = (IEnumPublishedApps *)DPA_GetPtr(_hdpaEnum, _iEnum);

        // If pepa is not valid or pepa->Next failed, we skip this Enumerator, and go on to the next
        // one until we hit the limit
        
        while ((!pepa || FAILED(hres = pepa->Next(ppia)))  && (_iEnum < DPA_GetPtrCount(_hdpaEnum)))
        {
            _iEnum++;
            pepa = (IEnumPublishedApps *)DPA_GetPtr(_hdpaEnum, _iEnum);
        }
    }    
    return hres;
}


// IEnumPublishedApps::Reset
HRESULT CShellEnumPublishedApps::Reset(void)
{
    // Call reset on everyone in the list and set our index iEnum to 0;
    if (_hdpaEnum)
    {
        IEnumPublishedApps * pepa;
        int idpa;
        for (idpa = 0; idpa < DPA_GetPtrCount(_hdpaEnum); idpa++)
        {
            pepa = (IEnumPublishedApps *)DPA_GetPtr(_hdpaEnum, idpa);
            if (pepa)
                pepa->Reset();
        }

        _iEnum = 0;

        return S_OK;
    }
    
    return E_FAIL;
}

#endif //DOWNLEVEL_PLATFORM
