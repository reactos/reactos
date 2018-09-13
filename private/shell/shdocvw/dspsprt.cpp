#include "priv.h"
#include "dspsprt.h"

#define TF_IDISPATCH 0

CImpIDispatch::CImpIDispatch(const IID * piid)
{
    m_piid = piid;
    ASSERT(NULL==m_pITINeutral);
    ASSERT(NULL==m_pdisp);
}

CImpIDispatch::~CImpIDispatch(void)
{
    ATOMICRELEASE(m_pITINeutral);
}

STDMETHODIMP CImpIDispatch::GetTypeInfoCount(UINT *pctInfo)
{
    *pctInfo = 1;
    return NOERROR;
}

// helper function for pulling ITypeInfo out of our typelib
HRESULT SHDOCVWGetTypeInfo(LCID lcid, UUID uuid, ITypeInfo **ppITypeInfo)
{
    ITypeLib  *pITypeLib;

    // Just in case we can't find the type library anywhere
    *ppITypeInfo = NULL;

    /*
     * The type libraries are registered under 0 (neutral),
     * 7 (German), and 9 (English) with no specific sub-
     * language, which would make them 407 or 409 and such.
     * If you are sensitive to sub-languages, then use the
     * full LCID instead of just the LANGID as done here.
     */
    HRESULT hr = LoadRegTypeLib(LIBID_SHDocVw, 1, 1, PRIMARYLANGID(lcid), &pITypeLib);

    /*
     * If LoadRegTypeLib fails, try loading directly with
     * LoadTypeLib, which will register the library for us.
     * Note that there's no default case here because the
     * prior switch will have filtered lcid already.
     *
     * NOTE:  You should prepend your DIR registry key to the
     * .TLB name so you don't depend on it being it the PATH.
     * This sample will be updated later to reflect this.
     */
    if (FAILED(hr))
    {
        OLECHAR wszPath[MAX_PATH];
        GetModuleFileName(HINST_THISDLL, wszPath, ARRAYSIZE(wszPath));

        switch (PRIMARYLANGID(lcid))
        {
        case LANG_NEUTRAL:
        case LANG_ENGLISH:
            hr = LoadTypeLib(wszPath, &pITypeLib);
            break;
        }
    }

    if (SUCCEEDED(hr))
    {
        //Got the type lib, get type info for the interface we want
        hr = pITypeLib->GetTypeInfoOfGuid(uuid, ppITypeInfo);
        pITypeLib->Release();
    }
    return hr;
}

STDMETHODIMP CImpIDispatch::GetTypeInfo(UINT itInfo, LCID lcid, ITypeInfo **ppITypeInfo)
{
    ITypeInfo **ppITI;

    *ppITypeInfo = NULL;

    if (0 != itInfo)
        return TYPE_E_ELEMENTNOTFOUND;

    // docs say we can ignore lcid if we support only one LCID
    // we don't have to return DISP_E_UNKNOWNLCID if we're *ignoring* it
    ppITI = &m_pITINeutral;

    // Load a type lib if we don't have the information already.
    if (NULL == *ppITI)
    {
        ITypeInfo *pITIDisp;
        HRESULT hr = SHDOCVWGetTypeInfo(lcid, *m_piid, &pITIDisp);
        if (SUCCEEDED(hr))
        {
            // All our IDispatch implementations are DUAL. GetTypeInfoOfGuid
            // returns the ITypeInfo of the IDispatch-part only. We need to
            // find the ITypeInfo for the dual interface-part.
            //
            HREFTYPE hrefType;
            HRESULT hrT = pITIDisp->GetRefTypeOfImplType(0xffffffff, &hrefType);
            if (SUCCEEDED(hrT))
            {
                hrT = pITIDisp->GetRefTypeInfo(hrefType, ppITI);
            }

            ASSERT(SUCCEEDED(hrT));
            if (FAILED(hrT))
            {
                // I suspect GetRefTypeOfImplType may fail if someone uses
                // CImpIDispatch on a non-dual interface. In this case the
                // ITypeInfo we got above is just fine to use.
                *ppITI = pITIDisp;
            }
            else
            {
                pITIDisp->Release();
            }
        }

        if (FAILED(hr))
            return hr;
    }

    (*ppITI)->AddRef();
    *ppITypeInfo = *ppITI;
    return NOERROR;
}

STDMETHODIMP CImpIDispatch::GetIDsOfNames(REFIID riid, OLECHAR **rgszNames, 
                                          UINT cNames, LCID lcid, DISPID *rgDispID)
{
    if (IID_NULL != riid)
        return DISP_E_UNKNOWNINTERFACE;

    ITypeInfo *pTI;
    HRESULT hr = GetTypeInfo(0, lcid, &pTI);
    if (SUCCEEDED(hr))
    {
        hr = pTI->GetIDsOfNames(rgszNames, cNames, rgDispID);
        pTI->Release();
    }

    TraceMsg(TF_IDISPATCH, "CImpIDispatch::GetIDsOfNames(%s = %x) called hres(%x)",
        *rgszNames ? *rgszNames : L"", *rgDispID, hr);

    return hr;
}

STDMETHODIMP CImpIDispatch::Invoke(DISPID dispID, REFIID riid, 
                                   LCID lcid, unsigned short wFlags, 
                                   DISPPARAMS *pDispParams, VARIANT *pVarResult, 
                                   EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    HRESULT hr;

    if (IID_NULL != riid)
        return DISP_E_UNKNOWNINTERFACE; // riid is supposed to be IID_NULL always

    // make sure we have an interface to hand off to Invoke
    if (NULL == m_pdisp)
    {
        hr = QueryInterface(*m_piid, (void **)&m_pdisp);
        m_pdisp->Release();
    }

    ITypeInfo *pTI;
    hr = GetTypeInfo(0, lcid, &pTI);
    if (SUCCEEDED(hr))
    {
        SetErrorInfo(0, NULL);  // Clear exceptions

        hr = pTI->Invoke(m_pdisp, dispID, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
        pTI->Release();
    }
    return hr;
}

void CImpIDispatch::Exception(WORD wException)
{
}

