// coming soon: new deskbar (old deskbar moved to browbar base class)

#include "priv.h"
#include "sccls.h"
#include "resource.h"
#include "deskbar.h"

#define SUPERCLASS  CDockingBar

//***   CDeskBar_CreateInstance --
//
STDAPI CDeskBar_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppunk, LPCOBJECTINFO poi)
{
    // aggregation checking is handled in class factory

    CDeskBar *pwbar = new CDeskBar();
    if (pwbar) {
        *ppunk = SAFECAST(pwbar, IDockingWindow*);
        return S_OK;
    }

    return E_OUTOFMEMORY;
}

//***
// NOTES
//  BUGBUG nuke this, fold it into CDeskBar_CreateInstance
HRESULT DeskBar_Create(IUnknown** ppunk, IUnknown** ppbs)
{
    HRESULT hres;

    *ppunk = NULL;
    if (ppbs)
        *ppbs = NULL;
    
    CDeskBar *pdb = new CDeskBar();
    if (!pdb)
        return E_OUTOFMEMORY;
    
    IDeskBarClient *pdbc;
    hres = CoCreateInstance(CLSID_RebarBandSite, NULL, CLSCTX_INPROC_SERVER, 
                            IID_IDeskBarClient, (LPVOID*)&pdbc);
    if (SUCCEEDED(hres))
    {
        hres = pdb->SetClient(pdbc);
        if (SUCCEEDED(hres))
        {
            if (ppbs) {
                *ppbs = pdbc;
                pdbc->AddRef();
            }
        
            *ppunk = SAFECAST(pdb, IDeskBar*);
        }
    
        pdbc->Release();
    }

    if (FAILED(hres))
    {
        pdb->Release();
    }

    return hres;
}


CDeskBar::CDeskBar() : SUPERCLASS()
{
    // We assume this object was ZERO-INITed on the heap.
    ASSERT(!_fRestrictionsInited);
}


//*** CDeskBar::IUnknown::* {

HRESULT CDeskBar::QueryInterface(REFIID riid, void **ppvObj)
{
    static const QITAB qit[] = {
        QITABENT(CDeskBar, IRestrict),
        { 0 },
    };

    HRESULT hres = QISearch(this, qit, riid, ppvObj);
    if (FAILED(hres))
        hres = SUPERCLASS::QueryInterface(riid, ppvObj);

    return hres;
}

// }


//*** CDeskBar::IPersistStream*::* {

HRESULT CDeskBar::GetClassID(CLSID *pClassID)
{
    *pClassID = CLSID_DeskBar;
    return S_OK;
}

// }



//*** CDeskBar::IRestrict::* {

HRESULT CDeskBar::IsRestricted(const GUID * pguidID, DWORD dwRestrictAction, VARIANT * pvarArgs, DWORD * pdwRestrictionResult)
{
    HRESULT hr = S_OK;

    if (!EVAL(pguidID) || !EVAL(pdwRestrictionResult))
        return E_INVALIDARG;

    *pdwRestrictionResult = RR_NOCHANGE;
    if (IsEqualGUID(RID_RDeskBars, *pguidID))
    {
        if (!_fRestrictionsInited)
        {
            _fRestrictionsInited = TRUE;
            if (SHRestricted(REST_NOCLOSE_DRAGDROPBAND))
                _fRestrictDDClose = TRUE;
            else
                _fRestrictDDClose = FALSE;

            if (SHRestricted(REST_NOMOVINGBAND))
                _fRestrictMove = TRUE;
            else
                _fRestrictMove = FALSE;
        }

        switch(dwRestrictAction)
        {
        case RA_DRAG:
        case RA_DROP:
        case RA_ADD:
        case RA_CLOSE:
            if (_fRestrictDDClose)
                *pdwRestrictionResult = RR_DISALLOW;
            break;
        case RA_MOVE:
            if (_fRestrictMove)
                *pdwRestrictionResult = RR_DISALLOW;
            break;
        }
    }

    if (RR_NOCHANGE == *pdwRestrictionResult)    // If we don't handle it, let our parents have a wack at it.
        hr = IUnknown_HandleIRestrict(_ptbSite, pguidID, dwRestrictAction, pvarArgs, pdwRestrictionResult);

    return hr;
}

// }



//*** CDeskBar::IServiceProvider::* {

HRESULT CDeskBar::QueryService(REFGUID guidService,
                                    REFIID riid, void **ppvObj)
{
    if (ppvObj)
        *ppvObj = NULL;

    if (IsEqualGUID(guidService, SID_SRestrictionHandler))
    {
        return QueryInterface(riid, ppvObj);
    }
    
    return SUPERCLASS::QueryService(guidService, riid, ppvObj);
}

// }
