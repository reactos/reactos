#include "stdafx.h"
#pragma hdrstop

//#include "dspsprt.h"
//#include "exdisp.h"

#define TF_IDISPATCH 0


/*
 * CImpIDispatch::CImpIDispatch
 * CImpIDispatch::~CImpIDispatch
 *
 * Parameters (Constructor):
 *  piid    guid this IDispatch implementation is for
 *          we call QueryInterface to get the interface
 */

CImpIDispatch::CImpIDispatch(const IID *plibid, USHORT wVerMajor, USHORT wVerMinor, const IID * piid)
{
    m_piid = piid;
    m_plibid = plibid;
    m_wVerMajor = wVerMajor;
    m_wVerMinor = wVerMinor;

    ASSERT(NULL==m_pITINeutral);

    return;
}

CImpIDispatch::~CImpIDispatch(void)
{
    if (m_pITINeutral)
    {
        m_pITINeutral->Release();
        m_pITINeutral = NULL;
    }
    return;
}


/*
 * CImpIDispatch::GetTypeInfoCount
 *
 * Purpose:
 *  Returns the number of type information (ITypeInfo) interfaces
 *  that the object provides (0 or 1).
 *
 * Parameters:
 *  pctInfo         UINT * to the location to receive
 *                  the count of interfaces.
 *
 * Return Value:
 *  HRESULT         NOERROR or a general error code.
 */

STDMETHODIMP CImpIDispatch::GetTypeInfoCount(UINT *pctInfo)
{
    //We implement GetTypeInfo so return 1
    *pctInfo=1;
    return NOERROR;
}


//
// helper function for pulling ITypeInfo out of our typelib
//
HRESULT GetTypeInfoFromLibId(LCID lcid, UUID libid, USHORT wVerMajor, USHORT wVerMinor, 
                             UUID uuid, ITypeInfo **ppITypeInfo)
{
    HRESULT    hr;
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

    hr=LoadRegTypeLib(libid, wVerMajor, wVerMinor, PRIMARYLANGID(lcid), &pITypeLib);

    if (SUCCEEDED(hr))
    {
        //Got the type lib, get type info for the interface we want
        hr=pITypeLib->GetTypeInfoOfGuid(uuid, ppITypeInfo);
        pITypeLib->Release();
    }

    return(hr);
}


/*
 * CImpIDispatch::GetTypeInfo
 *
 * Purpose:
 *  Retrieves type information for the automation interface.  This
 *  is used anywhere that the right ITypeInfo interface is needed
 *  for whatever LCID is applicable.  Specifically, this is used
 *  from within GetIDsOfNames and Invoke.
 *
 * Parameters:
 *  itInfo          UINT reserved.  Must be zero.
 *  lcid            LCID providing the locale for the type
 *                  information.  If the object does not support
 *                  localization, this is ignored.
 *  ppITypeInfo     ITypeInfo ** in which to store the ITypeInfo
 *                  interface for the object.
 *
 * Return Value:
 *  HRESULT         NOERROR or a general error code.
 */

STDMETHODIMP CImpIDispatch::GetTypeInfo(UINT itInfo, LCID lcid
    , ITypeInfo **ppITypeInfo)
{
    ITypeInfo **ppITI;

    *ppITypeInfo=NULL;

    if (0!=itInfo)
        return(TYPE_E_ELEMENTNOTFOUND);

    // docs say we can ignore lcid if we support only one LCID
    // we don't have to return DISP_E_UNKNOWNLCID if we're *ignoring* it
    ppITI = &m_pITINeutral;

    //Load a type lib if we don't have the information already.
    if (NULL==*ppITI)
    {
        HRESULT    hr;
        ITypeInfo *pITIDisp;

        hr = GetTypeInfoFromLibId(lcid, *m_plibid, m_wVerMajor, m_wVerMinor, *m_piid, &pITIDisp);

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
            {
                hrT = pITIDisp->GetRefTypeInfo(hrefType, ppITI);
            }

            ASSERT(SUCCEEDED(hrT));
            if (FAILED(hrT))
            {
                // I suspect GetRefTypeOfImplType may fail if someone uses
                // CImpIDispatch on a non-dual interface. In this case the
                // ITypeInfo we got above is just fine to use.
                //
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

    /*
     * Note:  the type library is still loaded since we have
     * an ITypeInfo from it.
     */

    (*ppITI)->AddRef();
    *ppITypeInfo=*ppITI;
    return NOERROR;
}


/*
 * CImpIDispatch::GetIDsOfNames
 *
 * Purpose:
 *  Converts text names into DISPIDs to pass to Invoke
 *
 * Parameters:
 *  riid            REFIID reserved.  Must be IID_NULL.
 *  rgszNames       OLECHAR ** pointing to the array of names to be
 *                  mapped.
 *  cNames          UINT number of names to be mapped.
 *  lcid            LCID of the locale.
 *  rgDispID        DISPID * caller allocated array containing IDs
 *                  corresponging to those names in rgszNames.
 *
 * Return Value:
 *  HRESULT         NOERROR or a general error code.
 */

STDMETHODIMP CImpIDispatch::GetIDsOfNames(REFIID riid
    , OLECHAR **rgszNames, UINT cNames, LCID lcid, DISPID *rgDispID)
{
    HRESULT     hr;
    ITypeInfo  *pTI;

    if (IID_NULL!=riid)
        return(DISP_E_UNKNOWNINTERFACE);

    //Get the right ITypeInfo for lcid.
    hr=GetTypeInfo(0, lcid, &pTI);

    if (SUCCEEDED(hr))
    {
        hr=pTI->GetIDsOfNames(rgszNames, cNames, rgDispID);

        pTI->Release();
    }

#ifdef DEBUG
    char szParam[MAX_PATH] = "";
    if (cNames >= 1)
    {
        WideCharToMultiByte(CP_ACP, 0,
            *rgszNames, -1,
            szParam, ARRAYSIZE(szParam), NULL, NULL);
    }

    TraceMsg(TF_IDISPATCH, "CImpIDispatch::GetIDsOfNames(%s = %x) called hres(%x)",
            szParam, *rgDispID, hr);
#endif

    return hr;
}



/*
 * CImpIDispatch::Invoke
 *
 * Purpose:
 *  Calls a method in the dispatch interface or manipulates a
 *  property.
 *
 * Parameters:
 *  dispID          DISPID of the method or property of interest.
 *  riid            REFIID reserved, must be IID_NULL.
 *  lcid            LCID of the locale.
 *  wFlags          USHORT describing the context of the invocation.
 *  pDispParams     DISPPARAMS * to the array of arguments.
 *  pVarResult      VARIANT * in which to store the result.  Is
 *                  NULL if the caller is not interested.
 *  pExcepInfo      EXCEPINFO * to exception information.
 *  puArgErr        UINT * in which to store the index of an
 *                  invalid parameter if DISP_E_TYPEMISMATCH
 *                  is returned.
 *
 * Return Value:
 *  HRESULT         NOERROR or a general error code.
 */

STDMETHODIMP CImpIDispatch::Invoke(DISPID dispID, REFIID riid
    , LCID lcid, unsigned short wFlags, DISPPARAMS *pDispParams
    , VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    ITypeInfo  *pTI;
    HRESULT hr;

    //riid is supposed to be IID_NULL always
    if (IID_NULL!=riid)
        return(DISP_E_UNKNOWNINTERFACE);

    IDispatch *pdisp;
    hr=QueryInterface(*m_piid, (LPVOID*)&pdisp);
    if (SUCCEEDED(hr))
    {
        //Get the ITypeInfo for lcid
        hr=GetTypeInfo(0, lcid, &pTI);
    
        if (SUCCEEDED(hr))
        {
            //Clear exceptions
            SetErrorInfo(0L, NULL);
    
            //This is exactly what DispInvoke does--so skip the overhead.
            hr=pTI->Invoke(pdisp, dispID, wFlags
                , pDispParams, pVarResult, pExcepInfo, puArgErr);
    
            pTI->Release();
        }
        pdisp->Release();
    }
    return hr;
}




/*
 * CImpIDispatch::Exception
 *
 * Purpose:
 *  Raises an exception for CImpIDispatch::Invoke from within
 *  ITypeInfo::Invoke using the CreateErrorInfo API and the
 *  ICreateErrorInfo interface.
 *
 *  Note that this method doesn't allow for deferred filling
 *  of an EXCEPINFO structure.
 *
 * Parameters:
 *  wException      WORD exception code.
 */

void CImpIDispatch::Exception(WORD wException)
{
    ASSERT(FALSE); // No one should call this yet
}

