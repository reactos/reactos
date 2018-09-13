//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1995.
//
//  File:       typenav.cxx
//
//  Contents:   Navigates an IDispatch's ITypeInfo.
//
//  Classes:    Priv
//
//----------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_TYPENAV_HXX_
#define X_TYPENAV_HXX_
#include "typenav.hxx"
#endif

DeclareTag(tagCTypeInfoNav, "CTypeInfoNav", "CTypeInfoNav methods");


CTypeInfoNav::CTypeInfoNav () :
      _pTI(0),
      _wVarCount(0),
      _wFuncCount(0),
      _uIndex(~0U),                      // Signal to Next() need to preload 1
      _wVarFlagsFilter(0),
      _fFuncDesc(FALSE),
      _pVD(0),
      _dispid(DISPID_UNKNOWN)
{
    TraceTag((tagCTypeInfoNav, "CTypeInfoNav::CTypeInfoNav() -> %p", this));
}


CTypeInfoNav::~CTypeInfoNav ()
{
    TraceTag((tagCTypeInfoNav, "CTypeInfoNav::~CTypeInfoNav() -> %p", this));

    //
    // Release the ITypeInfo and VARDESC or FUNCDESC pointers.
    //
    if (_pVD)
    {
        Assert(_pTI);
        if (_fFuncDesc)
            _pTI->ReleaseFuncDesc(_pFD);
        else
            _pTI->ReleaseVarDesc(_pVD);
    }

    ReleaseInterface(_pTI);
}


//+---------------------------------------------------------------------------
//
//  Function:   InitIDispatch
//
//  Synopsis:   Queries for an IDispatch to digout the ITypeInfo interface
//              and remember the actual number of entries in ITypeInfo for
//              iterating later.
//
//  Arguments:  [pUnk]      -- The interface which supports IDispatch.
//              [pITypeInfo]-- Returns an AddRef'd ITypeInfo if the pointer
//                             is not 0.
//              [wVFFilter] -- Variable or function filter to match (if zero
//                             then all properties are used).
//              [dispid]    -- dispid to match (if DISPID_UNKNOWN then look at
//                             each entry).
//
//  Returns:    HRESULT
//                  E_NOINTERFACE   - if pUnk doesn't support IID_Dispatch and
//                                    ITypeInfo.
//                  E_nnnn          - any HRESULT from:
//                                          QueryInterface,
//                                          IDispatch::GetTypeInfo
//                                          ITypeInfo::GetTypeAttr
//                  S_OK            - success.
//
//  Modifies:   Nothing
//
//----------------------------------------------------------------------------
HRESULT
CTypeInfoNav::InitIDispatch (IUnknown * pUnk,
                             ITypeInfo ** pITypeInfo,
                             WORD wVFFilter,
                             DISPID dispid)
{
    TraceTag((tagCTypeInfoNav,
              "CTypeInfoNav::InitIDispatch(%p, %p, %l, %l) -> %p",
              pUnk, pITypeInfo, wVFFilter, dispid, this));

    HRESULT     hr;
    IDispatch * pDisp = 0;

    Assert(pUnk);

    // Get typeinfo of control.
    hr = pUnk->QueryInterface(IID_IDispatch, (void **)&pDisp);
    if (hr)
        goto Cleanup;

    hr = InitIDispatch(pDisp, pITypeInfo, wVFFilter, dispid);

Cleanup:
    ReleaseInterface(pDisp);

    RRETURN(hr);
}


//+---------------------------------------------------------------------------
//
//  Function:   InitIDispatch
//
//  Synopsis:   Queries for an IDispatch to digout the ITypeInfo interface
//              and remember the actual number of entries in ITypeInfo for
//              iterating later.
//
//  Arguments:  [pDispatch] -- The IDispatch interface typeinfo to traverse.
//              [pITypeInfo]-- Returns an AddRef'd ITypeInfo if the pointer
//                             is not 0.
//              [wVFFilter] -- Variable or function filter to match (if zero
//                             then all properties are used).
//              [dispid]    -- dispid to match (if DISPID_UNKNOWN then look at
//                             each entry).
//
//  Returns:    HRESULT
//                  E_NOINTERFACE   - if pUnk doesn't support IID_Dispatch and
//                                    ITypeInfo.
//                  E_nnnn          - any HRESULT from:
//                                          QueryInterface,
//                                          IDispatch::GetTypeInfo
//                                          ITypeInfo::GetTypeAttr
//                  S_OK            - success.
//
//  Modifies:   Nothing
//
//----------------------------------------------------------------------------
HRESULT
CTypeInfoNav::InitIDispatch (IDispatch * pDisp,
                             ITypeInfo ** ppTypeInfo,
                             WORD wVFFilter,
                             DISPID dispid)
{
    TraceTag((tagCTypeInfoNav,
              "CTypeInfoNav::InitIDispatch(%p, %p, %l, %l) -> %p",
              pDisp, ppTypeInfo, wVFFilter, dispid, this));

    HRESULT     hr;

    Assert(pDisp);

    // Get typeinfo of control.
    hr = pDisp->GetTypeInfo(0, g_lcidUserDefault, &_pTI);
    if (hr)
        goto Cleanup;

    hr = InitITypeInfo(ppTypeInfo ? _pTI : 0, wVFFilter, dispid);

Cleanup:
    // We need to addref the ITypeInfo we're returning, InitITypeInfo
    // will do the addref for the copy we're returning.  GetTypeInfo
    // did the addref for the _pTI we're holding on to.
    if (ppTypeInfo)
    {
        *ppTypeInfo = hr ? NULL : _pTI;
    }

    RRETURN(hr);
}


//+---------------------------------------------------------------------------
//
//  Function:   InitITypeInfo
//
//  Synopsis:   Given an ITypeInfo interface remember this ITypeInfo interface
//              and the actual number of entries in ITypeInfo for iteration.
//
//  Arguments:  [pTypeInfo] -- ITypeInfo.
//              [wVFFilter] -- Variable or function filter to match (if zero
//                             then all properties are used).
//              [dispid]    -- dispid to match (if DISPID_UNKNOWN then look at
//                             each entry).
//
//  Returns:    HRESULT
//                  E_POINTER   - if pTypeInfo is 0.
//                  E_nnnn      - any HRESULT from ITypeInfo::GetTypeAttr
//                  S_OK        - success.
//
//  Modifies:   Nothing
//
//----------------------------------------------------------------------------
HRESULT
CTypeInfoNav::InitITypeInfo (ITypeInfo * pTypeInfo,
                             WORD wVFFilter,
                             DISPID dispid)
{
    TraceTag((tagCTypeInfoNav,
              "CTypeInfoNav::InitITypeInfo(%p, %l, %l) -> %p",
              pTypeInfo, wVFFilter, dispid, this));

    HRESULT     hr;
    TYPEATTR *  pTA;

    Assert((pTypeInfo == 0 && _pTI) || pTypeInfo);

    _dispid = dispid;

    if (pTypeInfo)
    {
        _pTI = pTypeInfo;
        _pTI->AddRef();
    }

    hr = _pTI->GetTypeAttr(&pTA);
    Assert(!hr);
    if (!hr)
    {
        _wVarCount = pTA->cVars;
        _wFuncCount = pTA->cFuncs;
        
        _fIsDual = ((pTA->wTypeFlags & TYPEFLAG_FDUAL) != 0);

        _pTI->ReleaseTypeAttr(pTA);

        _wVarFlagsFilter = wVFFilter;
    }

    RRETURN(hr);
}



#if DBG == 1
//+---------------------------------------------------------------------------
//
//  Function:   DbgUserDefined
//
//  Synopsis:   Internal debugging code, for a property in the typelib traverse
//              the property type looking for a user defined type.
//
//  Arguments:  None
//
//  Returns:    None
//
//  Modifies:   Nothing
//
//----------------------------------------------------------------------------
void
CTypeInfoNav::DbgUserDefined ()
{
    HRESULT hr;

    // If the tag is on then output additional information.
    if (IsTagEnabled(tagCTypeInfoNav))
    {
        OutputDebugString(_T("Property Name = "));

        BSTR bstr = NULL;
        UINT cNames;
        _pTI->GetNames(_fFuncDesc ? _pFD->memid : _pVD->memid,
                       &bstr, 1, &cNames);
#ifndef _MACUNICODE
        OutputDebugString(bstr);
#else
        {
            CStr str;
            str.Set(bstr);
            OutputDebugString(str);
        }
#endif
        OutputDebugString(_T("\r\n"));

        FormsFreeString(bstr);

        if (_fFuncDesc)
        {
            if (_pFD->lprgelemdescParam)
            {
                if (_pFD->lprgelemdescParam->tdesc.vt == VT_USERDEFINED)
                {
                    HREFTYPE hreftype = 
                               _pFD->lprgelemdescParam->tdesc.hreftype;

                    ITypeInfo * pUserTI;
                    hr = _pTI->GetRefTypeInfo(hreftype, &pUserTI);
                    if (!hr)
                    {
                        TYPEATTR *pTypeAttr;
                        hr = pUserTI->GetTypeAttr(&pTypeAttr);
                        if (!hr)
                        {
                            OutputDebugString(
                                           _T("User define type\r\n") );
                            pUserTI->ReleaseTypeAttr(pTypeAttr);
                        }
                        ReleaseInterface(pUserTI);
                    }
                }
            }
        }
    }
}
#endif



//+---------------------------------------------------------------------------
//
//  Function:   Next
//
//  Synopsis:   Iterates through an ITypeInfo libraries looking only at
//              VAR_DISPATCH entries which match the _wVarFlagsFilter.
//
//  Arguments:  None
//
//  Returns:    HRESULT
//                  E_POINTER           - if _pTINav or pTI are undefined (NULL)
//                  E_nnnn              - any HRESULT from ITypeInfo::GetVarDesc
//                  S_OK                - success
//                  S_FALSE             - No more items to iterate over
//
//  Modifies:   Nothing
//
//----------------------------------------------------------------------------
HRESULT
CTypeInfoNav::Next ()
{
    TraceTag((tagCTypeInfoNav, "CTypeInfoNav::Next() -> %p", this));

    HRESULT     hr = S_OK;
    WORD        wCount = _fIsDual ? _wFuncCount : _wFuncCount + _wVarCount;

    Assert(_pTI);

    while (++_uIndex < wCount)
    {
        // No, get the next one.

        UINT uIndex;        // index within current type (funcs or vars)

        // release the previous result
        if (_pVD)
        {
            if (_fFuncDesc)
                _pTI->ReleaseFuncDesc(_pFD);
            else
                _pTI->ReleaseVarDesc(_pVD);
            _pVD = 0;
        }

        // Get the description of the IDispatched variable.
        _fFuncDesc = (_uIndex < _wFuncCount);
        uIndex = _fFuncDesc ? _uIndex : (_uIndex - _wFuncCount);

        hr = _fFuncDesc ? _pTI->GetFuncDesc(uIndex, &_pFD) :
                          _pTI->GetVarDesc(uIndex, &_pVD);
        if (!hr)
        {
            TraceTag((tagCTypeInfoNav,
                     "  Next: dispid = %p; wVarFlags = %p -> %p",
                     _fFuncDesc ? _pFD->memid : _pVD->memid,
                     _fFuncDesc ? _pFD->wFuncFlags : _pVD->wVarFlags,
                     this));

#if DBG == 1
            DbgUserDefined();
#endif

            //
            // Can this variable only be accessed via IDispatch::Invoke?
            //
            if (_fFuncDesc ?
                ((_pFD->funckind == FUNC_DISPATCH) || (_pFD->funckind == FUNC_PUREVIRTUAL)) :
                (_pVD->varkind == VAR_DISPATCH))
            {
                DISPID dispid = _fFuncDesc ? _pFD->memid : _pVD->memid;

                // Are we trying to match to a particular dispid or if we found
                // the particular dispid then look at the filter.
                if ((_dispid == DISPID_UNKNOWN) || (_dispid == dispid))
                {
                    //
                    // If we have filters to check, then make sure the
                    // currentwVarFlags matches the filter before we say
                    // it's a match.
                    //
                    WORD wFlags = _fFuncDesc ? _pFD->wFuncFlags :
                                               _pVD->wVarFlags;
                    if (!_wVarFlagsFilter ||
                        (_wVarFlagsFilter & wFlags) == _wVarFlagsFilter)
                        break;
                }
            }            
        } else
            break;
    }      // end while loop

    //
    // If the internal index is larger than Count() then return
    // S_FALSE to signal there are no more entries to iterate over
    // or if we had an error then just the current error value in hr otherwise
    // we succeeded so return S_OK.
    //
    if (!hr)
        hr = (_uIndex >= wCount) ? S_FALSE : S_OK;

    RRETURN1(hr, S_FALSE);
}
