//=================================================================
//
//   File:      collbase.cxx
//
//  Contents:   CCollectionBase class
//
//  Classes:    CCollectionBase
//
//=================================================================

#include "headers.hxx"

#ifndef X_COLLBASE_HXX_
#define X_COLLBASE_HXX_
#include "collbase.hxx"
#endif



//+---------------------------------------------------------------
// CCollectionBase::InvokeEx, IDispatch
// Provides access to properties and members of the object
//
// Arguments:   [dispidMember] - Member id to invoke
//              [riid]         - Interface ID being accessed
//              [wFlags]       - Flags describing context of call
//              [pdispparams]  - Structure containing arguments
//              [pvarResult]   - Place to put result
//              [pexcepinfo]   - Pointer to exception information struct
//              [puArgErr]     - Indicates which argument is incorrect
//
// We override this to support ordinal and named member access to the
// elements of the collection.
//----------------------------------------------------------------

STDMETHODIMP
CCollectionBase::InvokeEx(DISPID    dispidMember,
               LCID                 lcid,
               WORD                 wFlags,
               DISPPARAMS          *pdispparams,
               VARIANT             *pvarResult,
               EXCEPINFO           *pexcepinfo,
               IServiceProvider    *pSrvProvider)
{
    HRESULT hr = DISP_E_MEMBERNOTFOUND;

    // Is the dispid an ordinal index? (array access)
    if (IsCollectionDispID(dispidMember))
    {
        if (wFlags & DISPATCH_PROPERTYPUT) 
        {
            goto Cleanup;       // collection is RO return DISP_E_MEMBERNOTFOUND
        }
        else if (wFlags & DISPATCH_PROPERTYGET)
        {
            if (pvarResult)
            {
                hr = GetItem(dispidMember - DISPID_COLLECTION_MIN, pvarResult);
            }
        }
    }
    else
    {
        // CBase knows how to handle expando
        hr = THR_NOTRACE(super::InvokeEx(dispidMember,
                         lcid,
                         wFlags,
                         pdispparams,
                         pvarResult,
                         pexcepinfo,
                         pSrvProvider));
    }

Cleanup:
    RRETURN(hr);
}


//+---------------------------------------------------------------
//  CCollectionBase::GetDispID, IDispatchEx
//
//  Overridden to output a particular dispid range collection accessing.
//----------------------------------------------------------------

STDMETHODIMP
CCollectionBase::GetDispID(BSTR bstrName, DWORD grfdex, DISPID *pid)
{
    HRESULT     hr;
    long        lIdx;

    if (!pid)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    // Could be an ordinal access
    hr = ttol_with_error(bstrName, &lIdx);
    if (hr)
    {
    // Not ordinal access; could be named 
        lIdx = FindByName(bstrName, !!(grfdex & fdexNameCaseSensitive));
    }

    *pid = DISPID_COLLECTION_MIN + lIdx;

    hr =  S_OK;

    if (lIdx == -1 || !IsLegalIndex(lIdx) || !IsCollectionDispID(*pid))
    {
        // Otherwise delegate to CBase impl for expando support etc.
        hr = THR_NOTRACE(super::GetDispID(bstrName, grfdex, pid));
    }


Cleanup:
    RRETURN(hr);
}



//+---------------------------------------------------------------
//  CCollectionBase::GetNextDispID, IDispatchEx
//
//  Supports enumerating our collection indices in addition to the
//  collection's own properties.
//----------------------------------------------------------------

STDMETHODIMP
CCollectionBase::GetNextDispID(DWORD grfdex, DISPID id, DISPID *prgid)
{
    HRESULT     hr = S_OK;
    long        lIdx;

    if (!prgid)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    // Are we enumerating?
    if (!IsCollectionDispID(id))
    {
        // No, so delegate to CBase for normal properties
        hr = super::GetNextDispID(grfdex, id, prgid);
        if (hr)
        {
            // Start drilling into the collection.
            if (IsLegalIndex(0))
            {
                *prgid = DISPID_COLLECTION_MIN;
                hr = S_OK;
            }
        }
    }
    else
    {
        // Drill into the collection
        lIdx = id - DISPID_COLLECTION_MIN + 1;

        // Yes we're enumerating indices, so return string of current DISPID, and DISPID for next index,
        // or DISPID_UNKNOWN if we're out of bounds.
        if (!IsCollectionDispID(id+1) || !IsLegalIndex(lIdx))
        {
            *prgid = DISPID_UNKNOWN;
            hr = S_FALSE;
            goto Cleanup;
        }

        ++id;
        *prgid = id;
    }

Cleanup:
    RRETURN1(hr, S_FALSE);
}

STDMETHODIMP
CCollectionBase::GetMemberName(DISPID id, BSTR *pbstrName)
{
    HRESULT hr;

    if (!pbstrName)
        return E_INVALIDARG;
    
    *pbstrName = NULL;

    // Are we enumerating?
    if (!IsCollectionDispID(id))
    {
        // No, so delegate to CBase for normal properties
        super::GetMemberName(id, pbstrName);
    }
    else
    {
        // Drill into the collection
        long lIdx;
        LPCTSTR szName;
        TCHAR ach[20];

        lIdx = id - DISPID_COLLECTION_MIN;

        // Yes we're enumerating indices, so return string of current DISPID, and DISPID for next index,
        // or DISPID_UNKNOWN if we're out of bounds.
        if (!IsCollectionDispID(id) || !IsLegalIndex(lIdx))
            goto Cleanup;

        szName = GetName(lIdx);
        if ( !szName )
        {
            // No name return index
            hr = Format(0, ach, ARRAY_SIZE(ach), _T("<0d>"), lIdx);
            if (hr)
                return hr;
            szName = ach;
        }
        hr = THR(FormsAllocString(szName, pbstrName));
        if ( hr )
            return hr;
    }

Cleanup:
    return *pbstrName ? S_OK : DISP_E_MEMBERNOTFOUND;
}

//+---------------------------------------------------------------
//  CCollectionBase::IsLegalIndex
//
//  Returns TRUE if given number is in the range of collection
//  indexes.
//----------------------------------------------------------------

BOOL 
CCollectionBase::IsLegalIndex(long lIdx)
{
    HRESULT    hr;
    
    // GetItem with NULL pointer means we want to cheeck the 
    // index only
    hr = THR(GetItem(lIdx, NULL));
    if(!hr)
        return TRUE;

    return FALSE;
}


//+---------------------------------------------------------------
//  CCollectionBase::GetIDsOfNames, IDispatch
//
// We need our IDispatch methods because if we don't provide them CBase
//  implementaion will be called (they are not virtual)
//----------------------------------------------------------------

HRESULT STDMETHODCALLTYPE 
CCollectionBase::GetIDsOfNames(REFIID riid, LPTSTR *rgszNames, UINT cNames,
         LCID lcid, DISPID *rgdispid)
{
    if (!IsEqualIID(riid, IID_NULL)) 
        RRETURN(E_INVALIDARG);
    return GetDispID(rgszNames[0], fdexFromGetIdsOfNames, rgdispid);
}


//+---------------------------------------------------------------
//  CCollectionBase::Invoke, IDispatch
//
// We need our IDispatch methods because if we don't provide them CBase
//  implementaion will be called (they are not virtual)
//----------------------------------------------------------------

HRESULT STDMETHODCALLTYPE 
CCollectionBase::Invoke(DISPID dispidMember, REFIID riid, LCID lcid, WORD wFlags,
         DISPPARAMS FAR* pdispparams, VARIANT FAR* pvarResult, EXCEPINFO FAR* pexcepinfo,
         UINT FAR* puArgErr)
{
    if (!IsEqualIID(riid, IID_NULL)) 
        RRETURN(E_INVALIDARG);
    return InvokeEx(dispidMember, lcid, wFlags, pdispparams, pvarResult, pexcepinfo, NULL);
}

