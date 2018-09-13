//---------------------------------------------------------------------------
//
// Copyright (c) Microsoft Corporation 
//
// File: darenum.cpp
//
// The current order of enumeration is Legacy --> Darwin --> SMS
//
// History:
//         2-03-97  by dli
//------------------------------------------------------------------------
#include "priv.h"

// Do not build this file if on Win9X or NT4
#ifndef DOWNLEVEL_PLATFORM

#include "darenum.h"
#include "darapp.h"
#include "util.h"

CDarwinEnumPublishedApps::CDarwinEnumPublishedApps(GUID * pAppCategoryId) : _cRef(1)
{
    ASSERT(_bGuidUsed == FALSE);

    // Do we have a Catogory GUID?
    if (pAppCategoryId)
    {
        // Yes
        _CategoryGUID = *pAppCategoryId;
        _bGuidUsed = TRUE;
    }

    GetManagedApplications(_bGuidUsed ? &_CategoryGUID : NULL, _bGuidUsed ? MANAGED_APPS_FROMCATEGORY : MANAGED_APPS_USERAPPLICATIONS,
                           MANAGED_APPS_INFOLEVEL_DEFAULT, &_dwNumApps, &_prgApps);
}

CDarwinEnumPublishedApps::~CDarwinEnumPublishedApps() 
{
    if (_prgApps && (_dwNumApps > 0))
    {
        LocalFree(_prgApps);
    }
}

// IEnumPublishedApps::QueryInterface
HRESULT CDarwinEnumPublishedApps::QueryInterface(REFIID riid, LPVOID * ppvOut)
{ 
    static const QITAB qit[] = {
        QITABENT(CDarwinEnumPublishedApps, IEnumPublishedApps),                  // IID_IEnumPublishedApps
        { 0 },
    };

    return QISearch(this, qit, riid, ppvOut);
}

// IEnumPublishedApps::AddRef
ULONG CDarwinEnumPublishedApps::AddRef()
{
    _cRef++;
    TraceMsg(TF_OBJLIFE, "CDarwinEnumPublishedApps()::AddRef called, new _cRef=%lX", _cRef);
    return _cRef;
}

// IEnumPublishedApps::Release
ULONG CDarwinEnumPublishedApps::Release()
{
    _cRef--;
    TraceMsg(TF_OBJLIFE, "CDarwinEnumPublishedApps()::Release called, new _cRef=%lX", _cRef);
    if (_cRef > 0)
        return _cRef;

    delete this;
    return 0;
}


// IEnumPublishedApps::Next
// BUGBUG: we should do some optimization instead of enumerating these apps
// one by one.
// S_FALSE means end of enumeration
HRESULT CDarwinEnumPublishedApps::Next(IPublishedApp ** ppia)
{
    HRESULT hres = S_FALSE;
    *ppia = NULL;
    if (_prgApps && (_dwNumApps > 0) && (_dwIndex < _dwNumApps))
    {
        BOOL bContinue = FALSE;
        do {
            PMANAGEDAPPLICATION pma = &_prgApps[_dwIndex];

            // NOTE: no Hydra machines (_bTSSession == TRUE) we filter out all the
            // Darwin apps. 
            if (pma->pszPackageName && pma->pszPackageName[0])
            {
                CDarwinPublishedApp *pdpa = new CDarwinPublishedApp(pma);
                if (pdpa)
                {
                    *ppia = SAFECAST(pdpa, IPublishedApp *);
                    hres = S_OK;
                }
                else
                    hres = E_OUTOFMEMORY;
                bContinue = FALSE;
            }   
            else
            {
                ClearManagedApplication(pma);
                bContinue = TRUE;
            }
            
            _dwIndex++;
        } while (bContinue && (_dwIndex < _dwNumApps));
    }
    
    return hres;
}


// IEnumPublishedApps::Reset
HRESULT CDarwinEnumPublishedApps::Reset(void)
{
    if (_prgApps && (_dwNumApps > 0))
    {
        LocalFree(_prgApps);
    }

    GetManagedApplications(_bGuidUsed ? &_CategoryGUID : NULL, MANAGED_APPS_USERAPPLICATIONS,
                           _bGuidUsed ? MANAGED_APPS_FROMCATEGORY : MANAGED_APPS_INFOLEVEL_DEFAULT, &_dwNumApps, &_prgApps);

    _dwIndex = 0;
    return S_OK;
}

#endif //DOWNLEVEL_PLATFORM
