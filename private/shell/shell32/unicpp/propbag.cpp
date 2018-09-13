#include "stdafx.h"
#include "stddef.h"
#pragma hdrstop


//
// property bag implementation used to pass information to dsfolder::ParseDisplayName
//

typedef struct
{
    LPWSTR pszPropName;
    VARIANT variant;
} PBAGENTRY, * LPPBAGENTRY;

class CPropertyBag : public IPropertyBag
{
    private:
        LONG _cRef;
        HDSA _hdsaProperties;
        HRESULT _Find(LPCOLESTR pszPropName, PBAGENTRY **pppbe, BOOL fCreate);

    public:
        CPropertyBag();
        ~CPropertyBag();

        // IUnknown
        STDMETHOD(QueryInterface)(REFIID riid, void **ppv);
        STDMETHOD_(ULONG, AddRef)();
        STDMETHOD_(ULONG, Release)();

        // IPropertyBag
        STDMETHOD(Read)(LPCOLESTR pszPropName, VARIANT *pVar, IErrorLog *pErrorLog);
        STDMETHOD(Write)(LPCOLESTR pszPropName, VARIANT *pVar);
};


//
// construction / destruction / IUnknown
// 

CPropertyBag::CPropertyBag() :
    _cRef(1),
    _hdsaProperties(NULL)
{
}

INT _FreePropBagCB(LPVOID pData, LPVOID lParam)
{
    LPPBAGENTRY ppbe = (LPPBAGENTRY)pData;
    Str_SetPtrW(&ppbe->pszPropName, NULL);
    VariantClear(&ppbe->variant);
    return 1;
}

CPropertyBag::~CPropertyBag()
{
    if ( _hdsaProperties )
        DSA_DestroyCallback(_hdsaProperties, _FreePropBagCB, NULL);
}


//
// IUnknown goop
//

STDMETHODIMP CPropertyBag::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] = {
        QITABENT(CPropertyBag, IPropertyBag),       // IID_IPropertyBag
        { 0 },
    };
    return QISearch(this, qit, riid, ppv);
}

STDMETHODIMP_(ULONG) CPropertyBag::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

STDMETHODIMP_(ULONG) CPropertyBag::Release()
{
    if (InterlockedDecrement(&_cRef))
        return _cRef;

    delete this;
    return 0;
}


//
// manange the list of propeties in the property bag
//

HRESULT CPropertyBag::_Find(LPCOLESTR pszPropName, PBAGENTRY **pppbe, BOOL fCreate)
{   
    int i;
    PBAGENTRY pbe = { 0 };

    *pppbe = NULL;

    // look up the property in the DSA
    // BUGBUG: change to a DPA and sort accordingly for better perf (daviddv 110798)

    for ( i = 0 ; _hdsaProperties && (i < DSA_GetItemCount(_hdsaProperties)) ; i++ )
    {
        LPPBAGENTRY ppbe = (LPPBAGENTRY)DSA_GetItemPtr(_hdsaProperties, i);

        if ( !StrCmpIW(pszPropName, ppbe->pszPropName) )
        {
            *pppbe = ppbe;
            return S_OK;
        }
    }

    // no entry found, should we create one?

    if ( !fCreate )
        return E_INVALIDARG;

    // yes, so lets check to see if we have a DSA yet.

    if ( !_hdsaProperties )
        _hdsaProperties = DSA_Create(SIZEOF(PBAGENTRY), 4);
    if ( !_hdsaProperties )
        return E_OUTOFMEMORY;

    // we have the DSA so lets fill the record we want to put into it

    if ( !Str_SetPtrW(&pbe.pszPropName, pszPropName) )
        return E_OUTOFMEMORY;

    VariantInit(&pbe.variant);

    // append it to the DSA we are using

    i = DSA_AppendItem(_hdsaProperties, &pbe);
    if ( -1 == i )
    {
        Str_SetPtrW(&pbe.pszPropName, NULL);
        return E_OUTOFMEMORY;
    }
    
    *pppbe = (LPPBAGENTRY)DSA_GetItemPtr(_hdsaProperties, i);

    return S_OK;
}


//
// IPropertyBag methods
//

STDMETHODIMP CPropertyBag::Read(LPCOLESTR pszPropName, VARIANT *pv, IErrorLog *pErrorLog)
{
    LPPBAGENTRY ppbe;

    if ( !pszPropName || !pv )
        return E_INVALIDARG;

    HRESULT hres  = _Find(pszPropName, &ppbe, FALSE);
    if ( SUCCEEDED(hres) )
        hres = VariantCopy(pv, &ppbe->variant);

    return hres;
}

STDMETHODIMP CPropertyBag::Write(LPCOLESTR pszPropName, VARIANT *pv)
{
    LPPBAGENTRY ppbe;

    if ( !pszPropName || !pv )
        return E_INVALIDARG;

    HRESULT hres  = _Find(pszPropName, &ppbe, TRUE);
    if ( SUCCEEDED(hres) )
    {
        VariantClear(&ppbe->variant);
        hres = VariantCopy(&ppbe->variant, pv);
    }

    return hres;
}


//
// Exported function for creating a IPropertyBag (or variant of) object.
//

STDAPI SHCreatePropertyBag(REFIID riid, void **ppv)
{
    CPropertyBag *ppb = new CPropertyBag;
    if ( !ppb )
        return E_OUTOFMEMORY;
    
    HRESULT hres = ppb->QueryInterface(riid, ppv);
    ppb->Release();

    return hres;
}

