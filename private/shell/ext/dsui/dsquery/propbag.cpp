#include "pch.h"
#include "stddef.h"
#pragma hdrstop


/*----------------------------------------------------------------------------
/ Very simple IPropertyBag implementation 
/----------------------------------------------------------------------------*/

//
// property bag implementation used to pass information to dsfolder::ParseDisplayName
//

typedef struct
{
    LPWSTR pszPropName;
    VARIANT variant;
} PBAGENTRY, * LPPBAGENTRY;

class CPropertyBag : public IPropertyBag, CUnknown
{
    private:
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
    _hdsaProperties(NULL)
{
}

INT _FreePropBagCB(LPVOID pData, LPVOID lParam)
{
    LPPBAGENTRY ppbe = (LPPBAGENTRY)pData;
    LocalFreeStringW(&ppbe->pszPropName);
    VariantClear(&ppbe->variant);
    return 1;
}

CPropertyBag::~CPropertyBag()
{
    if ( _hdsaProperties )
        DSA_DestroyCallback(_hdsaProperties, _FreePropBagCB, NULL);
}

// IUnknown handlers

#undef CLASS_NAME
#define CLASS_NAME CPropertyBag
#include "unknown.inc"

STDMETHODIMP CPropertyBag::QueryInterface( REFIID riid, void **ppv)
{
    INTERFACES iface[] =
    {
        &IID_IPropertyBag, (IPropertyBag*)this,
    };

   return HandleQueryInterface(riid, ppv, iface, ARRAYSIZE(iface));
}

// Create instance function

STDAPI CPropertyBag_CreateInstance(REFIID riid, void **ppv)
{
    CPropertyBag *ppb = new CPropertyBag;
    if ( !ppb )
        return E_OUTOFMEMORY;
    
    HRESULT hres = ppb->QueryInterface(riid, ppv);
    ppb->Release();

    return hres;
}


//
// manange the list of propeties in the property bag
//

HRESULT CPropertyBag::_Find(LPCOLESTR pszPropName, PBAGENTRY **pppbe, BOOL fCreate)
{   
    int i;
    HRESULT hres;
    PBAGENTRY pbe = { 0 };
    USES_CONVERSION;

    TraceEnter(TRACE_PBAG, "_Find");
    Trace(TEXT("Looking for property: %s"), W2CT(pszPropName));

    *pppbe = NULL;

    // look up the property in the DSA
    // BUGBUG: change to a DPA and sort accordingly for better perf (daviddv 110798)

    for ( i = 0 ; _hdsaProperties && (i < DSA_GetItemCount(_hdsaProperties)) ; i++ )
    {
        LPPBAGENTRY ppbe = (LPPBAGENTRY)DSA_GetItemPtr(_hdsaProperties, i);
        TraceAssert(ppbe);

        if ( !StrCmpIW(pszPropName, ppbe->pszPropName) )
        {
            *pppbe = ppbe;
            ExitGracefully(hres, S_OK, "Found an entry");
        }
    }

    // no entry found, should we create one?

    if ( !fCreate )
        ExitGracefully(hres, E_INVALIDARG, "Property not found");

    // yes, so lets check to see if we have a DSA yet.

    if ( !_hdsaProperties )
        _hdsaProperties = DSA_Create(SIZEOF(PBAGENTRY), 4);
    if ( !_hdsaProperties )
        ExitGracefully(hres, E_OUTOFMEMORY, "Failed to allocate the DSA");

    // we have the DSA so lets fill the record we want to put into it

    hres = LocalAllocStringW(&pbe.pszPropName, pszPropName);
    FailGracefully(hres, "Failed to copy the property name");

    VariantInit(&pbe.variant);

    // append it to the DSA we are using

    i = DSA_AppendItem(_hdsaProperties, &pbe);
    Trace(TEXT("Property added to bag at index %d"), i);

    if ( -1 == i )
    {
        LocalFreeStringW(&pbe.pszPropName);
        ExitGracefully(hres, E_OUTOFMEMORY, "Failed to append property to the property bag");
    }
    
    *pppbe = (LPPBAGENTRY)DSA_GetItemPtr(_hdsaProperties, i);
    TraceAssert(&pppbe);

    hres = S_OK;

exit_gracefully:

    TraceLeaveResult(hres);
}


/*----------------------------------------------------------------------------
/ IPropertyBag
/----------------------------------------------------------------------------*/

STDMETHODIMP CPropertyBag::Read(LPCOLESTR pszPropName, VARIANT *pv, IErrorLog *pErrorLog)
{
    HRESULT hres;
    LPPBAGENTRY ppbe;
    USES_CONVERSION;

    TraceEnter(TRACE_PBAG, "CPropertyBag::Read");

    if ( !pszPropName || !pv )
        ExitGracefully(hres, E_INVALIDARG, "Bad propname/result buffer");

    hres  = _Find(pszPropName, &ppbe, FALSE);
    FailGracefully(hres, "Failed to find the property bag entry");

    hres = VariantCopy(pv, &ppbe->variant);
    FailGracefully(hres, "Failed when copying the variant");

exit_gracefully:

    TraceLeaveResult(hres);
}

/*---------------------------------------------------------------------------*/

STDMETHODIMP CPropertyBag::Write(LPCOLESTR pszPropName, VARIANT *pv)
{
    HRESULT hres;
    LPPBAGENTRY ppbe;
    USES_CONVERSION;

    TraceEnter(TRACE_PBAG, "CPropertyBag::Write");

    if ( !pszPropName || !pv )
        ExitGracefully(hres, E_INVALIDARG, "Bad propname/result buffer");

    hres  = _Find(pszPropName, &ppbe, TRUE);
    FailGracefully(hres, "Failed to find the existing / create new property");

    VariantClear(&ppbe->variant);

    hres = VariantCopy(&ppbe->variant, pv);
    FailGracefully(hres, "Failed when copying the variant");

exit_gracefully:

    TraceLeaveResult(hres);

    TraceLeaveResult(E_NOTIMPL);
}
