/*
** shocx.c
**
** builds an ocx out of the embedding in shembed.c
*/

#include "priv.h"
#include "sccls.h"
#include "olectl.h"
#include "stdenum.h"
#include "shocx.h"
#include "resource.h"

/*
** some globals
*/

LCID g_lcidLocale = MAKELCID(LANG_USER_DEFAULT, SORT_DEFAULT);

#define SUPERCLASS CShellEmbedding

/*
** *** CShellOcx ***
*/

CShellOcx::CShellOcx(IUnknown* punkOuter, LPCOBJECTINFO poi, const OLEVERB* pverbs, const OLEVERB* pdesignverbs) :
                        CShellEmbedding(punkOuter, poi, pverbs),
                        _pDesignVerbs(pdesignverbs),
                        CImpIDispatch(poi->piid)
{
    // CShellEmbedding class handles the DllAddRef / DllRelease

    m_cpEvents.SetOwner(_GetInner(), poi->piidEvents);
    m_cpPropNotify.SetOwner(_GetInner(), &IID_IPropertyNotifySink);

    _nDesignMode = MODE_UNKNOWN;
}

CShellOcx::~CShellOcx()
{
    // Should have been released when cllient site was set to NULL.... Don't release
    // it here as this will cause some applications like VC5 to fault...
    ASSERT(_pDispAmbient==NULL);

    if (_pClassTypeInfo)
        _pClassTypeInfo->Release();
}

/*
** *** Override CShellEmbedding stuff ***
*/

//
// We have a different set of verbs in design mode
//
HRESULT CShellOcx::EnumVerbs(
    IEnumOLEVERB **ppEnumOleVerb)
{
    TraceMsg(TF_SHDCONTROL, "sho: EnumVerbs");

    if (_IsDesignMode())
    {
        *ppEnumOleVerb = new CSVVerb(_pDesignVerbs);
        if (*ppEnumOleVerb)
            return S_OK;
    }

    return SUPERCLASS::EnumVerbs(ppEnumOleVerb);
}


//
// For the interfaces we support here
//
HRESULT CShellOcx::v_InternalQueryInterface(REFIID riid, LPVOID * ppvObj)
{
    HRESULT hres;

    static const QITAB qit[] = {
        QITABENT(CShellOcx, IDispatch),
        QITABENT(CShellOcx, IOleControl),
        QITABENT(CShellOcx, IConnectionPointContainer),
        QITABENT(CShellOcx, IPersistStreamInit),
        QITABENTMULTI(CShellOcx, IPersistStream, IPersistStreamInit),
        QITABENT(CShellOcx, IPersistPropertyBag),
        QITABENT(CShellOcx, IProvideClassInfo2),
        QITABENTMULTI(CShellOcx, IProvideClassInfo, IProvideClassInfo2),
        { 0 },
    };

    hres = QISearch(this, qit, riid, ppvObj);

    if (FAILED(hres))
        hres = SUPERCLASS::v_InternalQueryInterface(riid, ppvObj);

    return hres;
}

//
// On a SetClientSite, we need to discard everything created from _pcli
// because shembed frees _pcli
//
HRESULT CShellOcx::SetClientSite(IOleClientSite *pClientSite)
{
    if (_pDispAmbient)
    {
        _pDispAmbient->Release();
        _pDispAmbient = NULL;
    }

    return SUPERCLASS::SetClientSite(pClientSite);
}


HRESULT CShellOcx::Draw(
    DWORD dwDrawAspect,
    LONG lindex,
    void *pvAspect,
    DVTARGETDEVICE *ptd,
    HDC hdcTargetDev,
    HDC hdcDraw,
    LPCRECTL lprcBounds,
    LPCRECTL lprcWBounds,
    BOOL ( __stdcall *pfnContinue )(ULONG_PTR dwContinue),
    ULONG_PTR dwContinue)
{
    if (_IsDesignMode())
    {
        HBRUSH hbrOld = (HBRUSH)SelectObject(hdcDraw, (HBRUSH)GetStockObject(WHITE_BRUSH));
        HPEN hpenOld = (HPEN)SelectObject(hdcDraw, (HPEN)GetStockObject(BLACK_PEN));
        Rectangle(hdcDraw, lprcBounds->left, lprcBounds->top, lprcBounds->right, lprcBounds->bottom);
        MoveToEx(hdcDraw, lprcBounds->left, lprcBounds->top, NULL);
        LineTo(hdcDraw, lprcBounds->right, lprcBounds->bottom);
        MoveToEx(hdcDraw, lprcBounds->left, lprcBounds->bottom, NULL);
        LineTo(hdcDraw, lprcBounds->right, lprcBounds->top);
        SelectObject(hdcDraw, hbrOld);
        SelectObject(hdcDraw, hpenOld);
        return S_OK;
    }

    return SUPERCLASS::Draw(dwDrawAspect, lindex, pvAspect, ptd, hdcTargetDev, hdcDraw,
                            lprcBounds, lprcWBounds, pfnContinue, dwContinue);
}


/*
** *** CShellOcx stuff ***
*/

// *** IPersistStream ***

HRESULT CShellOcx::GetSizeMax(ULARGE_INTEGER *pcbSize)
{
    // BUGBUG REVIEW: this is overly large, I believe E_NOTIMPL is a valid
    // return from this and it tells the container that we don't know how big we are.

    ULARGE_INTEGER cbMax = { 1028 * 8, 0 }; // isn't this overly large?
    *pcbSize = cbMax;
    return S_OK;
}

// *** IOleControl ***
STDMETHODIMP CShellOcx::GetControlInfo(LPCONTROLINFO pCI)
{
    return(E_NOTIMPL); // for mnemonics
}
STDMETHODIMP CShellOcx::OnMnemonic(LPMSG pMsg)
{
    return(E_NOTIMPL); // for mnemonics
}
STDMETHODIMP CShellOcx::OnAmbientPropertyChange(DISPID dispid)
{
    switch (dispid)
    {
    case DISPID_AMBIENT_USERMODE:           // design mode  vs  run mode
    case DISPID_UNKNOWN:
        _nDesignMode = MODE_UNKNOWN;
        break;
    }

    return(S_OK);
}
STDMETHODIMP CShellOcx::FreezeEvents(BOOL bFreeze)
{
    _fEventsFrozen = bFreeze;
    return(S_OK);
}


HRESULT CShellOcx::GetIDsOfNames(REFIID riid, OLECHAR * * rgszNames, UINT cNames, LCID lcid, DISPID * rgdispid)
{
    // This is gross, for some reason from VBScript in a page can not get "Document" through so try "Doc" and map
    HRESULT hres = CImpIDispatch::GetIDsOfNames(riid, rgszNames, cNames, lcid, rgdispid);
    if (FAILED(hres) && (cNames == 1) && rgszNames)
    {
        OLECHAR const *c_pwszDocument = L"Document";

        if (StrCmpIW(*rgszNames, L"Doc") == 0)
            hres = CImpIDispatch::GetIDsOfNames(riid, (OLECHAR**)&c_pwszDocument, cNames, lcid, rgdispid);
    }
    return hres;
}


// *** IConnectionPointContainer ***
CConnectionPoint* CShellOcx::_FindCConnectionPointNoRef(BOOL fdisp, REFIID iid)
{
    CConnectionPoint* pccp;

    if (IsEqualIID(iid, EVENTIIDOFCONTROL(this)) ||
        (fdisp && IsEqualIID(iid, IID_IDispatch)))
    {
        pccp = &m_cpEvents;
    }
    else if (IsEqualIID(iid, IID_IPropertyNotifySink))
    {
        pccp = &m_cpPropNotify;
    }
    else
    {
        pccp = NULL;
    }

    return pccp;
}

STDMETHODIMP CShellOcx::EnumConnectionPoints(LPENUMCONNECTIONPOINTS * ppEnum)
{
    return CreateInstance_IEnumConnectionPoints(ppEnum, 2,
            m_cpEvents.CastToIConnectionPoint(),
            m_cpPropNotify.CastToIConnectionPoint());
}

// *** IProvideClassInfo2 ***
STDMETHODIMP CShellOcx::GetClassInfo(LPTYPEINFO * ppTI)
{
    if (!_pClassTypeInfo) 
        SHDOCVWGetTypeInfo(LANGIDFROMLCID(g_lcidLocale), CLSIDOFOBJECT(this), &_pClassTypeInfo);

    if (_pClassTypeInfo) {
        _pClassTypeInfo->AddRef();
        *ppTI = _pClassTypeInfo;
        return S_OK;
    }

    ppTI = NULL;
    return E_FAIL;
}

// *** IProvideClassInfo2 ***

STDMETHODIMP CShellOcx::GetGUID(DWORD dwGuidKind, GUID *pGUID)
{
	if (pGUID == NULL)
		return E_POINTER;

	if ( (dwGuidKind == GUIDKIND_DEFAULT_SOURCE_DISP_IID)
        && _pObjectInfo->piidEvents)
	{
		*pGUID = EVENTIIDOFCONTROL(this);
		return S_OK;
	}
	*pGUID = GUID_NULL;
	return E_FAIL;
}
 

/*
** *** Random general support functions ***
*/

// this table is used for copying data around, and persisting properties.
// basically, it contains the size of a given data type
//
const BYTE g_rgcbDataTypeSize[] = {
    0,                      // VT_EMPTY = 0,
    0,                      // VT_NULL = 1,
    sizeof(short),          // VT_I2 = 2,
    sizeof(long),           // VT_I4 = 3,
    sizeof(float),          // VT_R4 = 4,
    sizeof(double),         // VT_R8= 5,
    sizeof(CURRENCY),       // VT_CY= 6,
    sizeof(DATE),           // VT_DATE = 7,
    sizeof(BSTR),           // VT_BSTR = 8,
    sizeof(IDispatch *),    // VT_DISPATCH = 9,
    sizeof(SCODE),          // VT_ERROR = 10,
    sizeof(VARIANT_BOOL),   // VT_BOOL = 11,
    sizeof(VARIANT),        // VT_VARIANT = 12,
    sizeof(IUnknown *),     // VT_UNKNOWN = 13,
};

//=--------------------------------------------------------------------------=
// CShellOcx::_IsDesignMode    [callable]
//=--------------------------------------------------------------------------=
// returns TRUE iff MODE_DESIGN
//
BOOL CShellOcx::_IsDesignMode(void)
{
    if (_nDesignMode == MODE_UNKNOWN)
    {
        VARIANT_BOOL fBool;

        if (_GetAmbientProperty(DISPID_AMBIENT_USERMODE, VT_BOOL, &fBool))
        {
            _nDesignMode = fBool ? MODE_FALSE : MODE_TRUE;
        }
        else
            _nDesignMode = MODE_FALSE;
    }

    return(_nDesignMode == MODE_TRUE);
}

//=--------------------------------------------------------------------------=
// CShellOcx::_GetAmbientProperty    [callable]
//=--------------------------------------------------------------------------=
// returns the value of an ambient property
//
// Parameters:
//    DISPID        - [in]  property to get
//    VARTYPE       - [in]  type of desired data
//    void *        - [out] where to put the data
//
// Output:
//    BOOL          - FALSE means didn't work.
//
// Notes:
//
BOOL CShellOcx::_GetAmbientProperty(DISPID dispid, VARTYPE vt, void *pData)
{
    DISPPARAMS dispparams;
    VARIANT v;
    HRESULT hr;

    // IE30's WebBrowser OC never requested ambient properties.
    // IE40's does and we're finding that apps implemented some of
    // the properties we care about incorrectly. Assume old classid
    // means this is an old app and fail. The code that calls this
    // is smart enough to deal with failure.
    //
    if (_pObjectInfo->pclsid == &CLSID_WebBrowser_V1)
        return FALSE;

    VariantInit(&v);

    // get a pointer to the source of ambient properties.
    //
    if (!_pDispAmbient)
    {
        if (_pcli)
            _pcli->QueryInterface(IID_IDispatch, (void **)&_pDispAmbient);

        if (!_pDispAmbient)
            return FALSE;
    }

    // now go and get the property into a variant.
    //
    ZeroMemory(&dispparams, sizeof(dispparams));
    hr = _pDispAmbient->Invoke(dispid, IID_NULL, 0, DISPATCH_PROPERTYGET,
                               &dispparams, &v, NULL, NULL);
    if (SUCCEEDED(hr))
    {
        VARIANT vDest;
        VariantInit(&vDest);
        // we've got the variant, so now go an coerce it to the type
        // that the user wants.
        //
        hr = VariantChangeType(&vDest, &v, 0, vt);
        if (SUCCEEDED(hr))
        {
            // copy the data to where the user wants it
            //
            CopyMemory(pData, &(vDest.lVal), g_rgcbDataTypeSize[vt]);
            VariantClear(&vDest);
        }
        VariantClear(&v);
    }

    return SUCCEEDED(hr);
}
