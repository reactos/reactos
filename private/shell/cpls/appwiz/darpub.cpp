#include "priv.h"

// Do not build this file if on Win9X or NT4
#ifndef DOWNLEVEL_PLATFORM

#include "darpub.h"
#include "darenum.h"
#include "sccls.h"
#include "util.h"

/////////////////////////////////////////////////////////////////////////////
// CDarwinAppPublisher
// Very thin layer around the darwin CoGet* API's


// constructor
CDarwinAppPublisher::CDarwinAppPublisher() : _cRef(1)
{
    DllAddRef();

    TraceAddRef(CDarwinAppPub, _cRef);
}


// destructor
CDarwinAppPublisher::~CDarwinAppPublisher()
{
    DllRelease();
}


// IAppPublisher::QueryInterface
HRESULT CDarwinAppPublisher::QueryInterface(REFIID riid, LPVOID * ppvOut)
{ 
     static const QITAB qit[] = {
        QITABENT(CDarwinAppPublisher, IAppPublisher),                  // IID_IAppPublisher
        { 0 },
    };

    return QISearch(this, qit, riid, ppvOut);
}

// IAppPublisher::AddRef
ULONG CDarwinAppPublisher::AddRef()
{
    _cRef++;
    TraceAddRef(CDarwinAppPub, _cRef);
    return _cRef;
}

// IAppPublisher::Release
ULONG CDarwinAppPublisher::Release()
{
    _cRef--;
    TraceRelease(CDarwinAppPub, _cRef);
    if (_cRef > 0)
        return _cRef;

    delete this;
    return 0;
}

// IAppPublisher::GetNumberOfCategories
STDMETHODIMP CDarwinAppPublisher::GetNumberOfCategories(DWORD * pdwCat)
{
    return E_NOTIMPL;
}

// IAppPublisher::GetCategories
STDMETHODIMP CDarwinAppPublisher::GetCategories(APPCATEGORYINFOLIST * pAppCategoryList)
{
    HRESULT hres = E_FAIL;
    RIP(pAppCategoryList);

    ZeroMemory(pAppCategoryList, SIZEOF(APPCATEGORYINFOLIST));
    APPCATEGORYINFOLIST acil = {0};
    hres = CsGetAppCategories(&acil);
    if (SUCCEEDED(hres) && (acil.cCategory > 0))
    {
        hres = _DuplicateCategoryList(&acil, pAppCategoryList);
        ReleaseAppCategoryInfoList(&acil);
    }
    
    return hres;
}

// IAppPublisher::GetNumberOfApps
STDMETHODIMP CDarwinAppPublisher::GetNumberOfApps(DWORD * pdwApps)
{
    return E_NOTIMPL;
}

// IAppPublisher::EnumApps
STDMETHODIMP CDarwinAppPublisher::EnumApps(GUID * pAppCategoryId, IEnumPublishedApps ** ppepa)
{
    HRESULT hres = E_FAIL;
    CDarwinEnumPublishedApps * pdepa = new CDarwinEnumPublishedApps(pAppCategoryId);
    if (pdepa)
    {
        *ppepa = SAFECAST(pdepa, IEnumPublishedApps *);
        hres = S_OK;
    }
    else
        hres = E_OUTOFMEMORY;

    return hres;
    
}

/*----------------------------------------------------------
Purpose: Create-instance function for class factory

*/
STDAPI CDarwinAppPublisher_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppunk, LPCOBJECTINFO poi)
{
    // aggregation checking is handled in class factory

    HRESULT hres = E_OUTOFMEMORY;
    CDarwinAppPublisher* pObj = new CDarwinAppPublisher();
    if (pObj)
    {
        *ppunk = SAFECAST(pObj, IAppPublisher *);
        hres = S_OK;
    }

    return hres;
}

#endif //DOWNLEVEL_PLATFORM
