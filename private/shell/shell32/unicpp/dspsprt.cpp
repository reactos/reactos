#include "stdafx.h"
#pragma hdrstop

#define TF_IDISPATCH 0

CImpIDispatch::CImpIDispatch(const IID *plibid, USHORT wVerMajor, USHORT wVerMinor, const IID * piid)
{
    m_piid = piid;
    m_plibid = plibid;
    m_wVerMajor = wVerMajor;
    m_wVerMinor = wVerMinor;

    ASSERT(NULL==m_pITINeutral);
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
HRESULT GetTypeInfoFromLibId(LCID lcid, UUID libid, USHORT wVerMajor, USHORT wVerMinor, 
                             UUID uuid, ITypeInfo **ppITypeInfo)
{
    *ppITypeInfo = NULL;        // assume failure

    /* The type libraries are registered under 0 (neutral),
     * 7 (German), and 9 (English) with no specific sub-
     * language, which would make them 407 or 409 and such.
     * If you are sensitive to sub-languages, then use the
     * full LCID instead of just the LANGID as done here. */

    ITypeLib *pITypeLib;
    HRESULT hr = LoadRegTypeLib(libid, wVerMajor, wVerMinor, PRIMARYLANGID(lcid), &pITypeLib);
    if (SUCCEEDED(hr))
    {
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

    //Load a type lib if we don't have the information already.
    if (NULL == *ppITI)
    {
        ITypeInfo *pITIDisp;
        HRESULT hr = GetTypeInfoFromLibId(lcid, *m_plibid, m_wVerMajor, m_wVerMinor, *m_piid, &pITIDisp);
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

STDMETHODIMP CImpIDispatch::GetIDsOfNames(REFIID riid, OLECHAR **rgszNames, UINT cNames, LCID lcid, DISPID *rgDispID)
{
    if (IID_NULL != riid)
        return DISP_E_UNKNOWNINTERFACE;

    //Get the right ITypeInfo for lcid.
    ITypeInfo  *pTI;
    HRESULT hr = GetTypeInfo(0, lcid, &pTI);
    if (SUCCEEDED(hr))
    {
        hr = pTI->GetIDsOfNames(rgszNames, cNames, rgDispID);
        pTI->Release();
    }

#ifdef DEBUG
    TCHAR szParam[MAX_PATH] = TEXT("");
    if (cNames >= 1)
        SHUnicodeToTChar(*rgszNames, szParam, ARRAYSIZE(szParam));

    TraceMsg(TF_IDISPATCH, "CImpIDispatch::GetIDsOfNames(%s = %x) called hres(%x)",
            szParam, *rgDispID, hr);
#endif
    return hr;
}

STDMETHODIMP CImpIDispatch::Invoke(DISPID dispID, REFIID riid, 
                                   LCID lcid, unsigned short wFlags, DISPPARAMS *pDispParams, 
                                   VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    if (IID_NULL != riid)
        return DISP_E_UNKNOWNINTERFACE; // riid is supposed to be IID_NULL always

    IDispatch *pdisp;
    HRESULT hr = QueryInterface(*m_piid, (void **)&pdisp);
    if (SUCCEEDED(hr))
    {
        //Get the ITypeInfo for lcid
        ITypeInfo *pTI;
        hr = GetTypeInfo(0, lcid, &pTI);
        if (SUCCEEDED(hr))
        {
            SetErrorInfo(0, NULL);  //Clear exceptions
    
            // This is exactly what DispInvoke does--so skip the overhead.
            hr = pTI->Invoke(pdisp, dispID, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
            pTI->Release();
        }
        pdisp->Release();
    }
    return hr;
}

void CImpIDispatch::Exception(WORD wException)
{
    ASSERT(FALSE); // No one should call this yet
}

