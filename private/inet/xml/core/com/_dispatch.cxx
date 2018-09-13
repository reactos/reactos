/*
 *  Copyright (C) 1998,1999 Microsoft Corporation. All rights reserved. * 
 */

#include "core.hxx"
#pragma hdrstop

#include "core/com/bstr.hxx"
#include "core/util/datatype.hxx"
#include <xmldomdid.h>

DeclareTag(tagDispatch, "IDispatchEx", "IDispatchEx");

HRESULT 
_dispatchImpl::GetTypeInfoCount( DISPATCHINFO * pdispatchinfo,
    /* [out] */ UINT __RPC_FAR *pctinfo)
{
    *pctinfo = 1;
    return S_OK;
}

HRESULT 
_dispatchImpl::GetTypeInfo( DISPATCHINFO * pdispatchinfo,
    /* [in] */ UINT iTInfo,
    /* [in] */ LCID lcid,
    /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo)
{
    Assert(GetComponentCount());
    if (!pdispatchinfo->_pTypeInfoCache)
    {
        HRESULT hr = ensureTypeInfo(pdispatchinfo, lcid);
        if (hr)
            return hr;
    }
    *ppTInfo = pdispatchinfo->_pTypeInfoCache;
    (*ppTInfo)->AddRef();
    return S_OK;
}

HRESULT 
_dispatchImpl::GetIDsOfNames( DISPATCHINFO * pdispatchinfo,
    /* [in] */ REFIID riid,
    /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
    /* [in] */ UINT cNames,
    /* [in] */ LCID lcid,
    /* [size_is][out] */ DISPID __RPC_FAR *rgDispId)
{
    Assert(GetComponentCount());
    HRESULT hr;

    if (pdispatchinfo->pMethods)
    {
        Assert(pdispatchinfo->pIndex && pdispatchinfo->pInvokeFunc);
        hr = FindIdsOfNames( rgszNames, cNames, pdispatchinfo->pMethods,
                             pdispatchinfo->cMethods, lcid, rgDispId, false);
    }
    else
    {
        if (!pdispatchinfo->_pTypeInfoCache)
        {
            hr = ensureTypeInfo(pdispatchinfo, lcid);
            if (hr)
                return hr;
        }
        hr = pdispatchinfo->_pTypeInfoCache->GetIDsOfNames(rgszNames, cNames, rgDispId);
    }
    return hr;
}

HRESULT 
_dispatchImpl::Invoke( DISPATCHINFO * pdispatchinfo, VOID FAR* pObj,
    /* [in] */ DISPID dispIdMember,
    /* [in] */ REFIID riid,
    /* [in] */ LCID lcid,
    /* [in] */ WORD wFlags,
    /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
    /* [out] */ VARIANT __RPC_FAR *pVarResult,
    /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
    /* [out] */ UINT __RPC_FAR *puArgErr)
{
    Assert(GetComponentCount());
    HRESULT hr;

    if (pdispatchinfo->pMethods && 0 == pDispParams->cNamedArgs)
    {
        Assert(pdispatchinfo->pIndex && pdispatchinfo->pInvokeFunc);
        hr = InvokeHelper(pObj, pdispatchinfo, dispIdMember, wFlags, 
                          pDispParams, pVarResult, pExcepInfo, puArgErr);
    }
    else
    {
        if (!pdispatchinfo->_pTypeInfoCache)
        {
            hr = ensureTypeInfo(pdispatchinfo, lcid);
            if (hr)
                return hr;
        }
        hr = pdispatchinfo->_pTypeInfoCache->Invoke(pObj, dispIdMember, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
    }
    return hr;
}


HRESULT 
_dispatchImpl::GetDispID(
    DISPATCHINFO * pdispatchinfo, 
    bool fCollection,
    /* [in] */ BSTR bstrName,
    /* [in] */ DWORD grfdex,
    /* [out] */ DISPID __RPC_FAR *pid)
{
    HRESULT hr;
    Assert(GetComponentCount());

    if (pdispatchinfo->pMethods)
    {    
        Assert(pdispatchinfo->pIndex && pdispatchinfo->pInvokeFunc);
        hr = FindIdsOfNames( &bstrName, 1, pdispatchinfo->pMethods,
                             pdispatchinfo->cMethods, 0x409, pid, grfdex & fdexNameCaseSensitive);
    }
    else
    {
        // We don't do expandos, so the following two flags are not supported
    //    if ((grfdex & fdexNameEnsure) || (grfdex & fdexNameImplicit))
    //    {
    //        hr = E_FAIL;
    //        goto cleanup;
    //    }

        if (!(pdispatchinfo->_pTypeInfoCache))
        {
            // Our typelibs should never be localized, so it's 
            // safe to assume a LCID of 0x409
            hr = ensureTypeInfo(pdispatchinfo, 0x409);
            if (hr)
                goto cleanup;
        }

        // Get the DISPID for this one name
        hr = pdispatchinfo->_pTypeInfoCache->GetIDsOfNames(&bstrName, 1, pid);

        // If it's found, do a case sensitive lookup based on the DISPID
        if (S_OK == hr && (grfdex & fdexNameCaseSensitive))
        {
            BSTR bstrAbsoluteName;
            UINT cNames = 0;

            // Get the appropriate string from the typelib, and do a case-sensitive compare
            if (SUCCEEDED(hr = pdispatchinfo->_pTypeInfoCache->GetNames(*pid, &bstrAbsoluteName, 1, &cNames)))
            {
                // Are the strings the same ?
                if (StrCmpW(bstrName, bstrAbsoluteName))
                {
                    hr = DISP_E_UNKNOWNNAME;
                }

                SysFreeString(bstrAbsoluteName);
            }
    
        }
    }

    if (S_OK != hr && fCollection)
    {
        VARIANT vInt;
        ::VariantInit(&vInt);

        if (S_OK == ParseNumeric((const WCHAR *)bstrName, _tcslen(bstrName), 
                                 DT_I4, &vInt, 
                                 null))
        {
            int nIndex = V_I4(&vInt);
            if ((VT_I4 == vInt.vt)
                && (0 <= nIndex)
                && (nIndex <= (DISPID_DOM_COLLECTION_MAX - DISPID_DOM_COLLECTION_BASE)) )
            {
                *pid = DISPID_DOM_COLLECTION_BASE + nIndex;
                hr = S_OK;
            }
        }
        hr = ::VariantClear(&vInt);
    }

cleanup:

    return hr;
}

 
#define DISPID_THIS (-613)

HRESULT 
_dispatchImpl::InvokeEx( 
    IDispatch* pObj,
    DISPATCHINFO * pdispatchinfo,
    bool fCollection,
    /* [in] */ DISPID id,
    /* [in] */ LCID lcid,
    /* [in] */ WORD wFlags,
    /* [in] */ DISPPARAMS __RPC_FAR *pdp,
    /* [out] */ VARIANT __RPC_FAR *pvarRes,
    /* [out] */ EXCEPINFO __RPC_FAR *pei,
    /* [unique][in] */ IServiceProvider __RPC_FAR *pspCaller)
{
    HRESULT hr;

    if (wFlags & DISPATCH_CONSTRUCT)
    {
#ifdef _DEBUG
        OutputDebugStringA("MSXML: IDispatchEx doesn't support the DISPATCH_CONSTRUCT flag\n");
#endif
        hr = E_FAIL;
    }
    else
    {
        DISPPARAMS dp;
        VARIANT vArg;

        if (fCollection
            && DISPID_DOM_COLLECTION_BASE <= id
            && id <= DISPID_DOM_COLLECTION_MAX
            && NULL != (wFlags & (DISPATCH_METHOD|DISPATCH_PROPERTYGET))
            && 0 == pdp->cArgs
            && 0 == pdp->cNamedArgs)
        {
            // pretend we are calling item()
            ::VariantInit(&vArg); // doesn't need a VariantClear because it is only VT_I4
            dp.rgvarg = &vArg;
            dp.cArgs = 1;
            dp.rgdispidNamedArgs = NULL;
            dp.cNamedArgs = 0;
            vArg.vt = VT_I4;
            V_I4(&vArg) = id - DISPID_DOM_COLLECTION_BASE;
            pdp = &dp;
            id = DISPID_VALUE;
        }
        else if (0 != pdp->cNamedArgs && DISPID_THIS == pdp->rgdispidNamedArgs[0])
        {
            // Setup a DISPPARAM that doesn't have a this pointer.
	        // This parameter is only allowed for call.
	        if (!(wFlags & DISPATCH_METHOD))
		        return E_INVALIDARG;
	        dp.cArgs = pdp->cArgs - 1;
	        dp.cNamedArgs = pdp->cNamedArgs - 1;
	        dp.rgvarg = pdp->rgvarg + 1;
	        dp.rgdispidNamedArgs = pdp->rgdispidNamedArgs + 1;
	        pdp = &dp;
	    }

        UINT uArgErr;
        hr = pObj->Invoke(id, IID_NULL, lcid, wFlags, pdp, pvarRes, pei, &uArgErr);
//        hr = pdispatchinfo->_pTypeInfoCache->Invoke(pObj, id, wFlags, pdp, pvarRes, pei, &uArgErr);
    }

    return hr;
}


extern ShareMutex * g_pMutexSR;

HRESULT 
_dispatchImpl::ensureTypeInfo( DISPATCHINFO * pdispatchinfo,
    /* [in] */ LCID lcid)
{
    if (!pdispatchinfo->_pTypeInfoCache)
    {
        MutexLock lock(g_pMutexSR);
        // check again
        if (!pdispatchinfo->_pTypeInfoCache)
        {
            ITypeInfo * pITypeInfo;
            HRESULT hr = ::GetTypeInfo(*pdispatchinfo->_plibid, pdispatchinfo->_ord, lcid, *pdispatchinfo->_puuid, &pITypeInfo);            
            if (hr)
            {
                // In an non-us OS, a lcid passed in may not match with the typeLib of the US msxml.dll
                // in this case, we try a general lcid, 0
                // We want to try the passed in lcid first because we might have localized typelib for 
                // different OS-s in the future.
                hr = ::GetTypeInfo(*pdispatchinfo->_plibid, pdispatchinfo->_ord, 0,
                       *pdispatchinfo->_puuid, &pITypeInfo);
                if (hr)
                    return hr;
            }
            // note that we are registering it with a NULL ptr.  This is to avoid RegisterStaticUnknown() doing an extra AddRef()
            hr = RegisterStaticUnknown( (IUnknown **)&pdispatchinfo->_pTypeInfoCache);
            if (hr)
            {
                pITypeInfo->Release();
                return hr;
            }
            pdispatchinfo->_pTypeInfoCache = pITypeInfo;
        }
    }
    return S_OK;
}


Exception *
_dispatchImpl::setErrorInfo(Exception * e)
{
    bstr b;

    TRY
    {
        b = e->toString();
        setErrorInfo(b);
    }
    CATCH
    {
        // If an exception occurs here then no rich error info is available.
        // Note: The HRESULT is still returned to the caller.
    }
    ENDTRY

    return e;
}


void
_dispatchImpl::setErrorInfo(const WCHAR * szDescription)
{
    _reference<ICreateErrorInfo>    cerrinfo;
    _reference<IErrorInfo>          errinfo;

    if (SUCCEEDED(::CreateErrorInfo(&cerrinfo)))
    {
        if (SUCCEEDED(cerrinfo->QueryInterface(IID_IErrorInfo, (void **) &errinfo)))
        {
            cerrinfo->SetDescription((WCHAR*)szDescription);
            ::SetErrorInfo(0, errinfo);
        }
    }
}


void
_dispatchImpl::setErrorInfo(ResourceID resId)
{
    String * pDesc = Resources::FormatMessage(resId, null);
    pDesc->AddRef(); // this is needed for GC safety
    // note: setErrorInfo must NEVER throw an exception
    setErrorInfo(pDesc->getData());
    pDesc->Release();
}


//////////////////////////////////////////////////////////////////////////////
//
// InvokeHelper
// 
// @mfunc	A helper function for implementing IDispatch::Invoke() 
//			
// 
// @side	None
//
// @rdesc	None
//
// Notes by nzeng 7/30/99:
//          puArgErr is not set when argument is wrong. may revisit this later
//////////////////////////////////////////////////////////////////////////////

HRESULT
_dispatchImpl::InvokeHelper(
    /* [in] */ void * pTarget,
    /* [in] */ DISPATCHINFO * pdispatchinfo,
    /* [in] */ DISPID dispid,
    /* [in] */ WORD wFlags,
    /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
    /* [out] */ VARIANT __RPC_FAR *pVarResult,
    /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
    /* [out] */ UINT __RPC_FAR *puArgErr)
{
    HRESULT hr = S_OK;
    bool fExcepInfo = NULL != pExcepInfo;
    INVOKE_ARG rgArgs[10];
    UINT cArgs = 0;
    WORD wInvokeType;
    int index;

    Assert(GetComponentCount());

    // This debug only code is here so that we can repro NT bug 410339 & IE bug 89098
#if DBG == 1
    VariantInit(&rgArgs[0].vArg);
    rgArgs[0].vArg.vt = VT_INT | VT_BYREF;
    rgArgs[0].vArg.plVal = NULL;
#endif

    hr = FindIndex(dispid, pdispatchinfo->pIndex, pdispatchinfo->cIndex, index);

    if (S_OK == hr)
    {
        hr = PrepareInvokeArgsAndResult(pDispParams, wFlags,  
                                    &(pdispatchinfo->pMethods[index]),
                                    pVarResult, rgArgs, cArgs, wInvokeType);
    }

    if (S_OK == hr)
    {
        hr = pdispatchinfo->pInvokeFunc(pTarget, dispid, rgArgs, wInvokeType, pVarResult, pDispParams->cArgs);
    }

    // Clear args
    for( UINT iArg = 0; iArg < cArgs; iArg++ )
    {
        if( rgArgs[iArg].fClear )
        {
            HRESULT hr2 = VariantClear( &rgArgs[iArg].vArg );
            if (S_OK == hr)
                hr = hr2;
        }
    }

    // Handles exception
    if (fExcepInfo && FAILED(hr))
    {
        hr = FailedInvoke(hr, pExcepInfo);
    }

    return hr;
}


//////////////////////////////////////////////////////////////////////////////
//
// FindIdsOfNames
// 
// @mfunc	Functions similar to IDispatch::GetIdsOfNames, however it works 
//			off of a sorted string table instead of a type library.
// 
// @side	None
//
// @rdesc	None
// Notes by nzeng 7/30/99:
//          This binary search algorithm assumes that the name table rgKnownNames is sorted by 
//          pwszName in case-sensitive way, but is used for both case-sensitive and case-insensitive searchs.
//          The algorithm could be broken when two names share the same prefix when case-insensitive
//          but the prefixes are in different case. e.g. ANameDNode and anamedDOMElement would be in
//          different order when sorted case-sensitively or case-insensitively 
//////////////////////////////////////////////////////////////////////////////

HRESULT 
_dispatchImpl::FindIdsOfNames( 
    WCHAR **rgNames, 
    UINT cNames, 
    INVOKE_METHOD *rgKnownNames, 
    int cKnownNames, 
    LCID lcid, 
    DISPID *rgdispid, 
    bool fCaseSensitive )
{
    if( !rgNames || !cNames || !rgKnownNames || !cKnownNames || !rgdispid )
        return E_INVALIDARG;

    // cannot handle the case were parameter also need translating
    if( cNames != 1 )
        return DISP_E_UNKNOWNNAME;

    int iBot = 0;
    int iTop = cKnownNames - 1;

    while( true )
    {
        int iMid = (iTop + iBot) / 2;
        int iCmp;
        
        if (fCaseSensitive)
            iCmp = StrCmpW( *rgNames, rgKnownNames[iMid].pwszName );
        else
            iCmp = StrCmpIW( *rgNames, rgKnownNames[iMid].pwszName );

        if( iCmp < 0 )
        {
            iTop = iMid - 1;
        }
        else if( iCmp > 0 )
        {
            iBot = iMid + 1;
        }
        else
        {
            *rgdispid = rgKnownNames[iMid].dispid;
            break;
        }

        if( iBot > iTop )
        {
            return DISP_E_UNKNOWNNAME;
        }
    }

    return S_OK;
}




HRESULT 
_dispatchImpl::FindIndex(
    DISPID dispid, 
    DISPIDTOINDEX * pIndexTable, 
    int cMax,
    int &index)
{
    int iBot = 0;
    int iTop = cMax - 1;

    while( true )
    {
        int iMid = (iTop + iBot) / 2;
        int iCmp = dispid - pIndexTable[iMid].dispid;

        if( iCmp < 0 )
        {
            iTop = iMid - 1;
        }
        else if( iCmp > 0 )
        {
            iBot = iMid + 1;
        }
        else
        {
            index = pIndexTable[iMid].index;
            return S_OK;
        }

        if( iBot > iTop )
        {
            return DISP_E_MEMBERNOTFOUND;
        }
    }
}


//////////////////////////////////////////////////////////////////////////////
//
// PrepareInvokeArgs
// 
// @mfunc   Prepares the arguements passed in via a dispparams structure to be
//          passed directly to the intended method call.  Variants are coerced
//          to the appropriate types if possible.
// 
// @side    None
//
// @rdesc   None
//
//////////////////////////////////////////////////////////////////////////////

HRESULT 
_dispatchImpl::PrepareInvokeArgs( 
    DISPPARAMS *pdispparams, 
    INVOKE_ARG *rgArgs, 
    VARTYPE *rgTypes, 
    const GUID **rgTypeIds,
    UINT cArgs )
{
    HRESULT hr = S_OK;
    GUID *pguid = (GUID*)&IID_IDispatch;

    Assert( pdispparams );
    Assert( rgArgs );
    Assert( rgTypes );
    Assert( cArgs > 0 );

    memset( &rgArgs[0], 0, sizeof(*rgArgs)*cArgs );

    for( UINT iArg = 0; iArg < cArgs; iArg++ )
    {
        INVOKE_ARG *pArg = &rgArgs[iArg];
        VARTYPE vt = rgTypes[iArg] & (~VT_OPTIONAL);
        bool fOptional = (rgTypes[iArg] & VT_OPTIONAL) != 0;

        if( iArg >= pdispparams->cArgs || FAILED(hr) )
        {
            pArg->vArg.vt = VT_ERROR;
            pArg->vArg.scode = DISP_E_PARAMNOTFOUND;
            if (SUCCEEDED(hr))
                pArg->fMissing = true;
        }
        else
        {
            // parameters are recorded in right to left order
            VARIANT *pParam = &pdispparams->rgvarg[pdispparams->cArgs - iArg - 1];

            // dereference indirect variants
            while (V_ISBYREF(pParam) && ((V_VT(pParam) & VT_TYPEMASK) == VT_VARIANT) && V_VARIANTREF(pParam))
            {
                pParam = pParam->pvarVal;
            }

            if( vt & VT_BYREF ) // special case for output args
            {
                if( vt == (VT_VARIANT|VT_BYREF) )
                {
                    // in/out paremeter, free the passed argument
                    if( !(pParam->vt & VT_BYREF) )
                        hr = VariantClear(pParam);

                    pArg->vArg.vt = VT_VARIANT|VT_BYREF;
                    pArg->vArg.pvarVal = pParam;
                }
                else if( VARTYPEMAP(vt) == VARTYPEMAP(pParam->vt) )
                {
                    pArg->vArg = *pParam;
                }
                else
                {
                    hr = DISP_E_TYPEMISMATCH;
                }
            }
            else
            {
                // if this is the type we are expecting, or missing use it w/o coercing
                if( VARTYPEMAP( pParam->vt ) == VARTYPEMAP( vt ) || VT_VARIANT == vt ||
                    (pParam->vt == VT_ERROR && pParam->scode == DISP_E_PARAMNOTFOUND) )
                {
                    // For object parms, QI to the desired interface/dispinterface
                    if ( (VT_DISPATCH == vt) && (NULL != VARMEMBER(pParam, pdispVal)) )
                    {
                        if (pParam->vt == VT_DISPATCH)
                        {
                            hr = V_DISPATCH(pParam)->QueryInterface(*rgTypeIds[iArg],
                                          (LPVOID *)&V_DISPATCH(&pArg->vArg));
                        }
                        else // byref
                        {
                            hr = (*V_DISPATCHREF(pParam))->QueryInterface(*rgTypeIds[iArg],
                                          (LPVOID *)&V_DISPATCH(&pArg->vArg));
                        }

                        if (SUCCEEDED(hr))
                        {
                            pArg->fClear = true;
                            V_VT(&pArg->vArg) = vt;
                        }
                        else
                        {
                            hr = DISP_E_TYPEMISMATCH;
                        }
                    }
                    else
                    {
                        pArg->vArg = *pParam;
                    }
                }
                else // otherwise, coerce this into the correct type if possible
                {
                    pArg->fClear = true;
                    hr = VariantChangeType( &pArg->vArg, pParam, 0, vt );
                }
            }

            if( SUCCEEDED(hr) && pArg->vArg.vt == VT_ERROR )
            {
                pArg->fMissing = true;
            }
        }

        if( pArg->fMissing && !fOptional )
        {
            hr = DISP_E_BADPARAMCOUNT;
        }
    }

    return hr;
}


//////////////////////////////////////////////////////////////////////////////
//
// PrepareInvokeArgsAndResult
// 
// @mfunc   Calls PrepareInvokeArgs if cArgs > 0, also may use part of rgArgs
//          if pvarResult is NULL.  In this case pvarResult is redirected to
//          point at one of variants in rgArgs, and its clear flag is set so
//          ClearInvokeArgs will free it.
// 
// @side    None
//
// @rdesc   None
//
//////////////////////////////////////////////////////////////////////////////


HRESULT 
_dispatchImpl::PrepareInvokeArgsAndResult( 
     DISPPARAMS *pdispParams, 
     WORD wFlags, 
     INVOKE_METHOD *pInvokeInfo,
     VARIANT *&pvarResult, 
     INVOKE_ARG *rgArgs, 
     UINT &cArgs,
     WORD &wInvokeType)
{
    HRESULT hr = S_OK;
    bool fResult = true;

    if ( (pdispParams->cArgs == 0) && (wFlags & DISPATCH_PROPERTYGET) )
    {
        wInvokeType = DISPATCH_PROPERTYGET;
        cArgs = 0;
    }
    else if ( (pdispParams->cArgs == 1) && (wFlags & (DISPATCH_PROPERTYPUTREF | DISPATCH_PROPERTYPUT)) )
    {
        if (wFlags & DISPATCH_PROPERTYPUTREF)
            wInvokeType = DISPATCH_PROPERTYPUTREF;
        else
            wInvokeType = DISPATCH_PROPERTYPUT;
        fResult = false;
        cArgs = 1;
    }
    else
    {
        cArgs = pInvokeInfo->cArgs;
        wInvokeType = DISPATCH_METHOD;
        fResult = VT_ERROR != pInvokeInfo->vtResult;
    }

    if ( !(wInvokeType & pInvokeInfo->wInvokeType) )
    {
        // Set cArgs to 0 to signal that do not need to call ClearInvokeArgs() 
        cArgs = 0;
        hr = DISP_E_MEMBERNOTFOUND;
        goto Cleanup;  
    }
    else if ( pdispParams->cArgs > cArgs )
    {
        cArgs = 0;
        hr = DISP_E_BADPARAMCOUNT;
        goto Cleanup;
    }

   
    if ( cArgs > 0 )
    {
        hr = PrepareInvokeArgs( pdispParams, rgArgs, pInvokeInfo->rgTypes, pInvokeInfo->rgTypeIds, cArgs );
        if (S_OK != hr)
            goto Cleanup;
    }

    if ( fResult )
    {
        // if no result is supplied, point to an invoke_arg
        if ( !pvarResult )
        {
            pvarResult = &rgArgs[cArgs].vArg;
            rgArgs[cArgs].fClear = true;
            cArgs++;
        }

        memset( pvarResult, 0, sizeof(*pvarResult) );
        pvarResult->vt = pInvokeInfo->vtResult;
    }

Cleanup:

//    if (S_OK != hr)
//    {
//        SetErrorInfo(0, null);
//    }

    return hr;
}



//////////////////////////////////////////////////////////////////////////////
//
// FailedInvoke
// 
// @mfunc	Fill exception information to pexcepinfo 
// 
// @side	None
//
// @rdesc	None
//
//////////////////////////////////////////////////////////////////////////////

HRESULT 
_dispatchImpl::FailedInvoke(
    const HRESULT &hr,
    EXCEPINFO * pexcepinfo)
{
    // clear out exception info
    IErrorInfo * pei = NULL;
    HRESULT res = hr;

    // Hack: don't create errorinfo for these errors to mimic oleaut behavior.
    if( hr == DISP_E_BADPARAMCOUNT ||
        hr == DISP_E_NONAMEDARGS ||
        hr == DISP_E_MEMBERNOTFOUND )
        return hr;

    // clear out exception info
    pexcepinfo->wCode = 0;
    pexcepinfo->scode = hr;

    // if error info exists, use it
    GetErrorInfo(0, &pei);
   
    if (pei)
    {
        // give back to OLE
        SetErrorInfo(0, pei);

        pei->GetHelpContext(&pexcepinfo->dwHelpContext);
        pei->GetSource(&pexcepinfo->bstrSource);
        pei->GetDescription(&pexcepinfo->bstrDescription);
        pei->GetHelpFile(&pexcepinfo->bstrHelpFile);

        // give complete ownership to OLE
        pei->Release();
        res = DISP_E_EXCEPTION;
    }

    return res;
}



#if DBG == 1

void 
_dispatchImpl::TestMethodTable(INVOKE_METHOD* table, int size)
{
    HRESULT hr;
    DISPID  id;
    for (int i = 0; i < size; i++)
    {
        hr = FindIdsOfNames(&(table[i].pwszName), 1, table, size, 0x409, &id, true);
        if (S_OK != hr)
        {
            Assert(0 && "Failed to find the dispid for name in case-sensitive case");
        }
        hr = FindIdsOfNames(&(table[i].pwszName), 1, table, size, 0x409, &id, false); 
        if (S_OK != hr)
        {
            Assert(0 && "Failed to find the dispid for name in case-insensitive case");
        }

        if (i < size - 1)
        {
            int iCmp;
            iCmp = StrCmpW( table[i].pwszName, table[i+1].pwszName );
            if (iCmp >= 0)
            {
                Assert(0 && "Wrong name order in method table: case sensitive");
            }

            iCmp = StrCmpIW( table[i].pwszName, table[i+1].pwszName );
            if (iCmp >= 0)
            {
                Assert(0 && "Wrong name order in method table: case insensitive");
            }
        }
    }
}


void 
_dispatchImpl::TestIndexMap(INVOKE_METHOD* table, DISPIDTOINDEX* indexTable, int size)
{
    for (int i = 0; i < size; i++)
    {
        if (table[indexTable[i].index].dispid != indexTable[i].dispid)
        {
            Assert(0 && "invalid index for dispid");
        }

        if (i < size - 1 && indexTable[i].dispid >= indexTable[i+1].dispid)
        {
            Assert(0 && "Wrong order in dispid index table"); 
        }
    }
}

#endif



HRESULT 
__dispatch::QueryInterface(IUnknown * punk, REFIID riid, void ** ppvObject)
{
    if (!ppvObject)
        return E_POINTER;

    if (riid == IID_IDispatch || riid == *_pdispatchinfo->_puuid)
    {
        punk->AddRef();
        *ppvObject = punk;
        return S_OK;
    }
    else if (riid == IID_ISupportErrorInfo)
    {
        punk->AddRef();
        *ppvObject = static_cast<ISupportErrorInfo *>(this);
        return S_OK;
    }
    else
    {
        *ppvObject = NULL;
        return E_NOINTERFACE;
    }
}


HRESULT 
__dispatch::GetTypeInfoCount(
    /* [out] */ UINT __RPC_FAR *pctinfo)
{
    *pctinfo = 1;
    return S_OK;
}

HRESULT 
__dispatch::GetTypeInfo(
    /* [in] */ UINT iTInfo,
    /* [in] */ LCID lcid,
    /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo)
{
    return _dispatchImpl::GetTypeInfo(_pdispatchinfo, iTInfo, lcid, ppTInfo);
}

HRESULT 
__dispatch::GetIDsOfNames(
    /* [in] */ REFIID riid,
    /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
    /* [in] */ UINT cNames,
    /* [in] */ LCID lcid,
    /* [size_is][out] */ DISPID __RPC_FAR *rgDispId)
{
    return _dispatchImpl::GetIDsOfNames(_pdispatchinfo, riid, rgszNames, cNames, lcid, rgDispId);
}

HRESULT 
__dispatch::Invoke(VOID FAR* pObj,
    /* [in] */ DISPID dispIdMember,
    /* [in] */ REFIID riid,
    /* [in] */ LCID lcid,
    /* [in] */ WORD wFlags,
    /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
    /* [out] */ VARIANT __RPC_FAR *pVarResult,
    /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
    /* [out] */ UINT __RPC_FAR *puArgErr)
{

    return _dispatchImpl::Invoke(_pdispatchinfo, pObj, dispIdMember, riid, lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
}


HRESULT 
__dispatch::InterfaceSupportsErrorInfo(REFIID riid)
{
    return (riid == *_pdispatchinfo->_puuid) ? S_OK : S_FALSE;
}

#if DBG == 1
LONG _dispatchCount;
#endif
