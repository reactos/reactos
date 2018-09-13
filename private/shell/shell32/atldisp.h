#ifndef __ATL_IDISPATCH_H__
#define __ATL_IDISPATCH_H__

#include "unicpp/stdafx.h"

extern LCID g_lcidLocale;


#pragma pack(push, _ATL_PACKING)

#ifndef ATL_NO_NAMESPACE
namespace ATL
{
#endif // ATL_NO_NAMESPACE



template <class T, const GUID* pclsid, const IID* piid, const GUID* plibid, WORD wMajor = 1, WORD wMinor = 0, class tihclass = CComTypeInfoHolder>
class ATL_NO_VTABLE CShell32AtlIDispatch
                    : public IOleObjectImpl<T>
{
public:
    CShell32AtlIDispatch();
    ~CShell32AtlIDispatch();

    // *** IDispatch ***
    virtual STDMETHODIMP Invoke(DISPID dispidMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS * pdispparams, VARIANT * pvarResult, EXCEPINFO * pexcepinfo, UINT * puArgErr);
    virtual STDMETHODIMP GetTypeInfo(UINT itinfo, LCID lcid, ITypeInfo** pptinfo);
    virtual STDMETHODIMP GetIDsOfNames(REFIID riid, OLECHAR **rgszNames, UINT cNames, LCID lcid, DISPID * rgdispid);

    // *** IProvideClassInfo ***
    virtual STDMETHODIMP GetClassInfo(ITypeInfo** pptinfo);

    // *** IOleObject ***
    virtual STDMETHODIMP GetMiscStatus(DWORD dwAspect, DWORD *pdwStatus);

    virtual STDMETHODIMP DoVerbUIActivate(LPCRECT prcPosRect, HWND hwndParent, HWND hwnd);
    virtual STDMETHODIMP TranslateAcceleratorPriv(T * pThis, MSG *pMsg, IOleClientSite * pocs);

    virtual STDMETHODIMP PrivateQI(REFIID iid, void ** ppvObject) = 0;

protected:
    // Helper functions;
    ITypeInfo *         _pClassTypeInfo;             // ITypeInfo of class
    IFileSearchBand *   _pdisp;                      // This will not contain a ref because it's equal to 'this'.
};


template <class T, const GUID* pclsid, const IID* piid, const GUID* plibid, WORD wMajor, WORD wMinor, class tihclass>
CShell32AtlIDispatch<T, pclsid, piid, plibid, 1, 0, tihclass>::CShell32AtlIDispatch()
{
    // This allocator should have zero inited the memory, so assert the member variables are empty.
    ASSERT(!_pClassTypeInfo);
}

template <class T, const GUID* pclsid, const IID* piid, const GUID* plibid, WORD wMajor, WORD wMinor, class tihclass>
CShell32AtlIDispatch<T, pclsid, piid, plibid, 1, 0, tihclass>::~CShell32AtlIDispatch()
{
    if (_pClassTypeInfo)
        _pClassTypeInfo->Release();

    // _pdisp doesn't have a ref so it's OK if it's not NULL.
}


// *** IProvideClassInfo ***
template <class T, const GUID* pclsid, const IID* piid, const GUID* plibid, WORD wMajor, WORD wMinor, class tihclass>
HRESULT CShell32AtlIDispatch<T, pclsid, piid, plibid, 1, 0, tihclass>::GetClassInfo(ITypeInfo ** ppTI)
{
    if (!_pClassTypeInfo) 
        Shell32GetTypeInfo(LANGIDFROMLCID(g_lcidLocale), *pclsid, &_pClassTypeInfo);

    if (EVAL(_pClassTypeInfo))
    {
        _pClassTypeInfo->AddRef();
        *ppTI = _pClassTypeInfo;
        return S_OK;
    }

    *ppTI = NULL;
    return E_FAIL;
}


// *** IDispatch ***
template <class T, const GUID* pclsid, const IID* piid, const GUID* plibid, WORD wMajor, WORD wMinor, class tihclass>
HRESULT CShell32AtlIDispatch<T, pclsid, piid, plibid, 1, 0, tihclass>::GetTypeInfo(UINT itinfo, LCID lcid, ITypeInfo** ppITypeInfo)
{
    HRESULT hr = S_OK;

    *ppITypeInfo = NULL;

    if (0 != itinfo)
        return(TYPE_E_ELEMENTNOTFOUND);

    //Load a type lib if we don't have the information already.
    if (NULL == *ppITypeInfo)
    {
        ITypeInfo * pITIDisp;

        hr = Shell32GetTypeInfo(lcid, *piid, &pITIDisp);

        if (SUCCEEDED(hr))
        {
            HRESULT hrT;
            HREFTYPE hrefType;

            // All our IDispatch implementations are DUAL. GetTypeInfoOfGuid
            // returns the ITypeInfo of the IDispatch-part only. We need to
            // find the ITypeInfo for the dual interface-part.
            //
            hrT = pITIDisp->GetRefTypeOfImplType(0xffffffff, &hrefType);
            if (SUCCEEDED(hrT))
                hrT = pITIDisp->GetRefTypeInfo(hrefType, ppITypeInfo);

            ASSERT(SUCCEEDED(hrT));
            pITIDisp->Release();
        }
    }

    return hr;
}


template <class T, const GUID* pclsid, const IID* piid, const GUID* plibid, WORD wMajor, WORD wMinor, class tihclass>
HRESULT CShell32AtlIDispatch<T, pclsid, piid, plibid, 1, 0, tihclass>::GetIDsOfNames(REFIID // riid
    , LPOLESTR* rgszNames, UINT cNames, LCID lcid, DISPID* rgdispid)
{
    ITypeInfo* pInfo;
    HRESULT hr = GetTypeInfo(0, lcid, &pInfo);

    if (pInfo != NULL)
    {
        hr = pInfo->GetIDsOfNames(rgszNames, cNames, rgdispid);
        pInfo->Release();
    }

    return hr;
}

template <class T, const GUID* pclsid, const IID* piid, const GUID* plibid, WORD wMajor, WORD wMinor, class tihclass>
HRESULT CShell32AtlIDispatch<T, pclsid, piid, plibid, 1, 0, tihclass>::Invoke(DISPID dispidMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS * pdispparams, VARIANT * pvarResult, EXCEPINFO * pexcepinfo, UINT * puArgErr)
{
    HRESULT hr = E_FAIL;
    DISPPARAMS dispparams = {0};

    if (!pdispparams)
        pdispparams = &dispparams;  // otherwise OLE Fails when passed NULL.

    // make sure we have an interface to hand off to Invoke
    if (NULL == _pdisp)
    {
        hr = PrivateQI(*piid, (LPVOID*)&_pdisp);
        ASSERT(SUCCEEDED(hr));

        // don't hold a refcount on ourself
        _pdisp->Release();
    }

    ITypeInfo * pITypeInfo;
    hr = GetTypeInfo(0, lcid, &pITypeInfo);
    if (EVAL(SUCCEEDED(hr)))
    {
        //Clear exceptions
        SetErrorInfo(0L, NULL);

        hr = pITypeInfo->Invoke(_pdisp, dispidMember, wFlags, pdispparams, pvarResult, pexcepinfo, puArgErr);
        pITypeInfo->Release();
    }

    return hr;
}


#define DW_MISC_STATUS (OLEMISC_SETCLIENTSITEFIRST | OLEMISC_ACTIVATEWHENVISIBLE | OLEMISC_RECOMPOSEONRESIZE | \
                        OLEMISC_CANTLINKINSIDE | OLEMISC_INSIDEOUT)

template <class T, const GUID* pclsid, const IID* piid, const GUID* plibid, WORD wMajor, WORD wMinor, class tihclass>
HRESULT CShell32AtlIDispatch<T, pclsid, piid, plibid, 1, 0, tihclass>::GetMiscStatus(DWORD dwAspect, DWORD *pdwStatus)
{
    *pdwStatus = DW_MISC_STATUS;
    return S_OK;
}


template <class T, const GUID* pclsid, const IID* piid, const GUID* plibid, WORD wMajor, WORD wMinor, class tihclass>
HRESULT CShell32AtlIDispatch<T, pclsid, piid, plibid, 1, 0, tihclass>::DoVerbUIActivate(LPCRECT prcPosRect, HWND hwndParent, HWND hwnd)
{
    HRESULT hr = IOleObjectImpl<T>::DoVerbUIActivate(prcPosRect, hwndParent);
    
    if (hwnd)
        ::SetFocus(hwnd);

    return hr;
}


template <class T, const GUID* pclsid, const IID* piid, const GUID* plibid, WORD wMajor, WORD wMinor, class tihclass>
HRESULT CShell32AtlIDispatch<T, pclsid, piid, plibid, 1, 0, tihclass>::TranslateAcceleratorPriv(T * pThis, MSG *pMsg, IOleClientSite * pocs)
{
    HRESULT hr = S_FALSE;

    if (!EVAL(pMsg))
        return E_INVALIDARG;

    hr = pThis->T::TranslateAcceleratorInternal(pMsg, pocs);
    if (FAILED(hr))
    {
        if (WM_KEYDOWN == pMsg->message)
        {
            switch (pMsg->wParam)
            {
            // We can't handle RETURN because the script wants that
            // for post and other reasons
            //case VK_RETURN:
            case VK_ESCAPE:
            case VK_END:
            case VK_HOME:
            case VK_LEFT:
            case VK_UP:
            case VK_RIGHT:
            case VK_DOWN:
            case VK_DELETE:
                if (TranslateMessage(pMsg))
                {
                    DispatchMessage(pMsg);
                    hr = S_OK;
                }
                break;
            case VK_TAB:
                {
                    CComQIPtr <IOleControlSite, &IID_IOleControlSite> spSite(pocs);
                    if (EVAL(spSite))
                        hr = spSite->TranslateAccelerator(pMsg, 0);
                }
                break;
            default:
                break;
            }
        }

        if (S_OK != hr)
        {
            // We didn't handle it so give our base class a chances.
            hr = pThis->IOleInPlaceActiveObjectImpl<T>::TranslateAccelerator(pMsg);
        }
    }

    return hr;
}


#ifndef ATL_NO_NAMESPACE
}; //namespace ATL
#endif // ATL_NO_NAMESPACE


#endif // __ATL_IDISPATCH_H__
