//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       disputil.cxx
//
//  Contents:   Dispatch Utilities used internally by Forms3
//              and by C++ language integration clients.
//
//----------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_COREGUID_H_
#define X_COREGUID_H_
#include "coreguid.h"
#endif

#ifndef X_DISPEX_H_
#define X_DISPEX_H_
#include <dispex.h>
#endif

#ifndef X_OLECTL_H_
#define X_OLECTL_H_
#include <olectl.h>
#endif

//+---------------------------------------------------------------------------
//
//  Function:   FreeEXCEPINFO
//
//  Synopsis:   Frees resources in an excepinfo.  Does not reinitialize
//              these fields.
//
//----------------------------------------------------------------------------

void
FreeEXCEPINFO(EXCEPINFO * pEI)
{
    if (pEI)
    {
        FormsFreeString(pEI->bstrSource);
        FormsFreeString(pEI->bstrDescription);
        FormsFreeString(pEI->bstrHelpFile);
    }
}



//+---------------------------------------------------------------------------
//
//  Function:   SetErrorInfoFromEXCEPINFO
//
//  Synopsis:   Set per thread error info using data from an EXCEPINFO
//
//----------------------------------------------------------------------------

HRESULT
SetErrorInfoFromEXCEPINFO(EXCEPINFO *pexcepinfo)
{
    ICreateErrorInfo *  pCEI = NULL;
    IErrorInfo *        pEI = NULL;
    HRESULT             hr;

    if (pexcepinfo->pfnDeferredFillIn)
    {
        hr = THR((*pexcepinfo->pfnDeferredFillIn)(pexcepinfo));
        if (hr)
            goto Cleanup;

        pexcepinfo->pfnDeferredFillIn = NULL;
    }

    hr = THR(CreateErrorInfo(&pCEI));
    if (hr)
        goto Cleanup;

    hr = THR(pCEI->SetSource(pexcepinfo->bstrSource));
    if (hr)
        goto Cleanup;

    hr = THR(pCEI->SetDescription(pexcepinfo->bstrDescription));
    if (hr)
        goto Cleanup;

    hr = THR(pCEI->SetHelpFile(pexcepinfo->bstrHelpFile));
    if (hr)
        goto Cleanup;

    hr = THR(pCEI->SetHelpContext(pexcepinfo->dwHelpContext));
    if (hr)
        goto Cleanup;

    hr = THR(pCEI->QueryInterface(IID_IErrorInfo, (void **)&pEI));
    if (hr)
        goto Cleanup;

    hr = SetErrorInfo(NULL, pEI);

Cleanup:
    ReleaseInterface(pEI);
    ReleaseInterface(pCEI);
    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Function:   VARTYPEFromBYTE
//
//  Synopsis:   Converts a byte specification of the type of a variant to
//              a VARTYPE.
//
//----------------------------------------------------------------------------

VARTYPE
VARTYPEFromBYTE(BYTE b)
{
    VARTYPE vt;

    Assert(!(b & 0xB0));
    if (b & 0x40)
    {
        vt = (VARTYPE) ((UINT) b ^ (0x40 | VT_BYREF));
    }
    else
    {
        vt = b;
    }

    return vt;
}


//+---------------------------------------------------------------------------
//
//  Function:   IsVariantOmitted
//
//  Synopsis:   Checks to see if variant was omitted in vb call
//
//  Arguments:  [pvarg] -- Variant tpo check
//
//  Returns:    BOOL: TRUE is omitted
//
//----------------------------------------------------------------------------

BOOL
IsVariantOmitted ( const VARIANT * pvarg )
{
    if (!pvarg)
        return TRUE;
    
    return pvarg &&
           V_VT(pvarg) == VT_ERROR && 
           V_ERROR(pvarg) == DISP_E_PARAMNOTFOUND;
}


// We don't want to include the CRuntime so we've built the routine here.

// IEEE format specifies these...
// +Infinity: 7FF00000 00000000
// -Infinity: FFF00000 00000000
//       NAN: 7FF***** ********
//       NAN: FFF***** ********

// We also test for these, because the MSVC 1.52 CRT produces them for things
// like log(0)...
// +Infinity: 7FEFFFFF FFFFFFFF
// -Infinity: FFEFFFFF FFFFFFFF


// returns true for non-infinite nans.
int isNAN(double dbl)
{
    union
    {
        USHORT rgw[4];
        ULONG  rglu[2];
        double dbl;
    } v;

    v.dbl = dbl;

#ifdef BIG_ENDIAN
    return 0 == (~v.rgw[0] & 0x7FF0) &&
        ((v.rgw[0] & 0x000F) || v.rgw[1] || v.rglu[1]);
#else
    return 0 == (~v.rgw[3] & 0x7FF0) &&
        ((v.rgw[3] & 0x000F) || v.rgw[2] || v.rglu[0]);
#endif
}


// returns false for infinities and nans.
int isFinite(double dbl)
{
    union
    {
        USHORT rgw[4];
        ULONG rglu[2];
        double dbl;
    } v;

    v.dbl = dbl;

#ifdef BIG_ENDIAN
    return (~v.rgw[0] & 0x7FE0) ||
        0 == (v.rgw[0] & 0x0010) &&
        (~v.rglu[1] || ~v.rgw[1] || (~v.rgw[0] & 0x000F));
#else
    return (~v.rgw[3] & 0x7FE0) ||
        0 == (v.rgw[3] & 0x0010) &&
        (~v.rglu[0] || ~v.rgw[2] || (~v.rgw[3] & 0x000F));
#endif
}


//+---------------------------------------------------------------------------
//
//  Function:   VARIANTARGChangeTypeSpecial
//
//  Synopsis:   Helper.
//              Converts a VARIANT of arbitrary type to a VARIANT of type VT,
//              using browswer specific conversion rules, which may differ from
//              standard OLE Automation conversion rules (usually because
//              Netscape does something wierd).
//
//              This was pulled out of VARIANTARGToCVar because its also called
//              from CheckBox databinding.
//  
//  Arguments:  [pVArgDest]     -- Destination VARIANT (should already be init'd).
//              [vt]            -- Type to convert to.
//              [pvarg]         -- Variant to convert.
//              [pv]            -- Location to place C-language variable.
//
//  Modifies:   [pv].
//
//  Returns:    HRESULT.
//
//  History:    1-7-96  cfranks pulled out from VARIANTARGToCVar.
//
//----------------------------------------------------------------------------

HRESULT
VariantChangeTypeSpecial(VARIANT *pVArgDest, VARIANT *pvarg, VARTYPE vt,IServiceProvider *pSrvProvider, DWORD dwFlags)
{
    HRESULT             hr;
    IVariantChangeType *pVarChangeType = NULL;

    if (pSrvProvider)
    {
        hr = THR(pSrvProvider->QueryService(SID_VariantConversion,
                                            IID_IVariantChangeType,
                                            (void **)&pVarChangeType));
        if (hr)
            goto OldWay;

        // Use script engine conversion routine.
    	hr = pVarChangeType->ChangeType(pVArgDest, pvarg, 0, vt);

        //Assert(!hr && "IVariantChangeType::ChangeType failure");
        if (!hr)
            goto Cleanup;   // ChangeType suceeded we're done...
    }

    // Fall back to our tried & trusted type coercions
OldWay:

    hr = S_OK;

    if (vt == VT_BSTR && V_VT(pvarg) == VT_NULL)
    {
        // Converting a NULL to BSTR
        V_VT(pVArgDest) = VT_BSTR;
        hr = THR(FormsAllocString( _T("null"),
            &V_BSTR(pVArgDest) ) );
        goto Cleanup;
    }
    else if (vt == VT_BSTR && V_VT(pvarg) == VT_EMPTY)
    {
        // Converting "undefined" to BSTR
        V_VT(pVArgDest) = VT_BSTR;
        hr = THR(FormsAllocString( _T("undefined"),
            &V_BSTR(pVArgDest) ) );
        goto Cleanup;
    }
    else if (vt == VT_BOOL && V_VT(pvarg) == VT_BSTR)
    {
        // Converting from BSTR to BOOL
        // To match Navigator compatibility empty strings implies false when
        // assigned to a boolean type any other string implies true.
        V_VT(pVArgDest) = VT_BOOL;
        V_BOOL(pVArgDest) = FormsStringLen(V_BSTR(pvarg)) == 0 ? VB_FALSE : VB_TRUE;
        goto Cleanup;
    }
    else if (  V_VT(pvarg) == VT_BOOL && vt == VT_BSTR )
    {
        // Converting from BOOL to BSTR
        // To match Nav we either get "true" or "false"
        V_VT(pVArgDest) = VT_BSTR;
        hr = THR(FormsAllocString( V_BOOL(pvarg) == VB_TRUE ? _T("true") : _T("false"),
            &V_BSTR(pVArgDest) ) );
        goto Cleanup;
    }
    // If we're converting R4 or R8 to a string then we need special handling to
    // map Nan and +/-Inf.
    else if (vt == VT_BSTR && (V_VT(pvarg) == VT_R8 || V_VT(pvarg) == VT_R4))
    {
        double  dblValue = V_VT(pvarg) == VT_R8 ? V_R8(pvarg) : (double)(V_R4(pvarg));

        // Infinity or NAN?
        if (!isFinite(dblValue))
        {
            if (isNAN(dblValue))
            {
                // NAN
                hr = FormsAllocStringW(_T("NaN"), &(V_BSTR(pVArgDest)));
            }
            else
            {
                // Infinity
                hr = FormsAllocStringW((dblValue < 0) ? _T("-Infinity") : _T("Infinity"), &(V_BSTR(pVArgDest)));
            }
        }
        else
            goto DefaultConvert;


        // Any error from allocating string?
        if (hr)
           goto Cleanup;

        V_VT(pVArgDest) = vt;
        goto Cleanup;
    }


DefaultConvert:
    //
    // Default VariantChangeTypeEx.
    //

    // VARIANT_NOUSEROVERRIDE flag is undocumented flag that tells OLEAUT to convert to the lcid
    // given. Without it the conversion is done to user localeid
    hr = THR_NOTRACE(VariantChangeTypeEx(pVArgDest, pvarg, LCID_SCRIPTING, dwFlags|VARIANT_NOUSEROVERRIDE, vt));

    
    if (hr == DISP_E_TYPEMISMATCH  )
    {
        if ( V_VT(pvarg) == VT_NULL )
        {
            hr = S_OK;
            switch ( vt )
            {
            case VT_BOOL:
                V_BOOL(pVArgDest) = VB_FALSE;
                V_VT(pVArgDest) = VT_BOOL;
                break;

            // For NS compatability - NS treats NULL args as 0
            default:
                V_I4(pVArgDest)=0;
                break;
            }
        }
        else if (V_VT(pvarg) == VT_DISPATCH )
        {
            // Nav compatability - return the string [object] or null 
            V_VT(pVArgDest) = VT_BSTR;
            hr = THR(FormsAllocString ( (V_DISPATCH(pvarg)) ? _T("[object]") : _T("null"), &V_BSTR(pVArgDest) ) );
        }
        else if (   V_VT(pvarg) == VT_BSTR 
                &&  (   V_BSTR(pvarg)  
                    &&  ((V_BSTR(pvarg))[0] == _T('\0')) 
                    ||  ! V_BSTR(pvarg) )
                &&  (  vt == VT_I4 || vt == VT_I2 
                    || vt == VT_UI2 || vt == VT_UI4 
                    || vt == VT_I8 || vt == VT_UI8 
                    || vt == VT_INT || vt == VT_UINT ) )
        {
            // Converting empty string to integer => Zero
            hr = S_OK;
            V_VT(pVArgDest) = vt;
            V_I4(pVArgDest) = 0;
            goto Cleanup;
        }    
    }
    else if (hr == DISP_E_OVERFLOW && vt == VT_I4 && (V_VT(pvarg) == VT_R8 || V_VT(pvarg) == VT_R4))
    {
        // Nav compatability - return MAXLONG on overflow
        V_VT(pVArgDest) = VT_I4;
        V_I4(pVArgDest) = MAXLONG;
        hr = S_OK;
        goto Cleanup;
    }

    // To match Navigator change any scientific notation E to e.
    if (!hr && (vt == VT_BSTR && (V_VT(pvarg) == VT_R8 || V_VT(pvarg) == VT_R4)))
    {
        TCHAR *pENotation;

        pENotation = _tcschr(V_BSTR(pVArgDest), _T('E'));
        if (pENotation)
            *pENotation = _T('e');
    }

Cleanup:
    ReleaseInterface(pVarChangeType);

    RRETURN(hr);
}

HRESULT ClipVarString(VARIANT *pvarSrc, VARIANT *pvarDest, BOOL *pfAlloc, WORD wMaxstrlen)
{
    HRESULT hr = S_OK;
    if (wMaxstrlen && (V_VT(pvarSrc) == VT_BSTR) && FormsStringLen(V_BSTR(pvarSrc)) > wMaxstrlen)
    {
        hr = FormsAllocStringLen(V_BSTR(pvarSrc), wMaxstrlen, &V_BSTR(pvarDest));
        if (hr)
            goto Cleanup;

        *pfAlloc = TRUE;
        V_VT(pvarDest) = VT_BSTR;
    }
    else
        hr = S_FALSE;

Cleanup:
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   VARIANTARGToCVar
//
//  Synopsis:   Converts a VARIANT to a C-language variable.
//
//  Arguments:  [pvarg]         -- Variant to convert.
//              [pfAlloc]       -- BSTR allocated during conversion caller is
//                                 now owner of this BSTR or IUnknown or IDispatch
//                                 object allocated needs to be released.
//              [vt]            -- Type to convert to.
//              [pv]            -- Location to place C-language variable.
//
//  Modifies:   [pv].
//
//  Returns:    HRESULT.
//
//  History:    2-23-94   adams   Created
//
//  Notes:      Supports all variant pointer types, VT_I2, VT_I4, VT_R4,
//              VT_R8, VT_ERROR.
//----------------------------------------------------------------------------

HRESULT
VARIANTARGToCVar(VARIANT * pvarg, BOOL *pfAlloc, VARTYPE vt, void * pv, IServiceProvider *pSrvProvider, WORD wMaxstrlen)
{
    HRESULT         hr = S_OK;
    VARIANTARG     *pVArgCopy = pvarg;
    VARIANTARG      vargNew;                    // variant of new type
    BOOL            fAlloc;

    Assert(pvarg);
    Assert(pv);

    if (!pfAlloc)
        pfAlloc = &fAlloc;

    Assert((vt & ~VT_TYPEMASK) == 0 || (vt & ~VT_TYPEMASK) == VT_BYREF);

    // Assume no allocations yet.
    *pfAlloc = FALSE;

    if (vt & VT_BYREF)
    {
        // If the parameter is a variant pointer then everything is acceptable.

        if ((vt & VT_TYPEMASK) == VT_VARIANT)
        {
            switch (V_VT(pvarg))
            {
            case VT_VARIANT | VT_BYREF :
                hr = ClipVarString(pvarg->pvarVal, *(VARIANT **)pv, pfAlloc, wMaxstrlen);
                break;
            default:
                hr = ClipVarString(pvarg, *(VARIANT **)pv, pfAlloc, wMaxstrlen);
                break;
            }
            if (hr == S_FALSE)
            {
                hr = S_OK;
                *(PVOID *)pv = (PVOID)pvarg;
            }

            goto Cleanup;
        }

        if ((V_VT(pvarg) & VT_TYPEMASK) != (vt & VT_TYPEMASK))
        {
            hr = DISP_E_TYPEMISMATCH;
            goto Cleanup;
        }

        // Type of both original and destination or same type (however, original
        // may not be a byref only the original.

        if (V_ISBYREF(pvarg))
        {
            // Destination and original are byref and same type just copy pointer.
            *(PVOID *)pv = V_BYREF(pvarg);
        }
        else
        {
            // Convert original to byref.
            switch (vt & VT_TYPEMASK)
            {
            case VT_BOOL:
                *(PVOID *)pv = (PVOID)&V_BOOL(pvarg);
                break;

            case VT_I2:
                *(PVOID *)pv = (PVOID)&V_I2(pvarg);
                break;

            case VT_ERROR:
            case VT_I4:
                *(PVOID *)pv = (PVOID)&V_I4(pvarg);
                break;

            case VT_R4:
                *(PVOID *)pv = (PVOID)&V_R4(pvarg);
                break;

            case VT_R8:
                *(PVOID *)pv = (PVOID)&V_R8(pvarg);
                break;

            case VT_CY:
                *(PVOID *)pv = (PVOID)&V_CY(pvarg);
                break;

            // All pointer types.
            case VT_PTR:
            case VT_BSTR:
            case VT_LPSTR:
            case VT_LPWSTR:
            case VT_DISPATCH:
            case VT_UNKNOWN:
                *(PVOID *)pv = (PVOID)&V_UNKNOWN(pvarg);
                break;

            case VT_VARIANT:
                Assert("Dead code: shudn't have gotten here!");
                *(PVOID *)pv = (PVOID)pvarg;
                break;

            default:
                Assert(!"Unknown type in BYREF VARIANTARGToCVar().\n");
                hr = DISP_E_TYPEMISMATCH;
                goto Cleanup;
            }
        }

        goto Cleanup;
    }
    // If the c style parameter is the same type as the VARIANT then we'll just
    // move the data.  Also if the c style type is a VARIANT then there's
    // nothing to convert just copy the variant to the C parameter.
    else if ((V_VT(pvarg) & (VT_TYPEMASK | VT_BYREF)) != vt && (vt != VT_VARIANT))
    {
        // If the request type isn't the same as the variant passed in then we
        // need to convert.
        VariantInit(&vargNew);
        pVArgCopy = &vargNew;

        hr = VariantChangeTypeSpecial(pVArgCopy, pvarg, vt,pSrvProvider);

        if (hr)
            goto Cleanup;

        *pfAlloc = (vt == VT_BSTR) || (vt == VT_UNKNOWN) || (vt == VT_DISPATCH);
    }

    // Move the variant data to C style data.
    switch (vt)
    {
    case VT_BOOL:
        #if DBG==1
            if (V_BOOL(pVArgCopy) != VB_FALSE && V_BOOL(pVArgCopy) != VB_TRUE)
            {
                TraceTag((tagWarning, "Illegal VT_BOOL in VARIANTARGToCVar"));
            }
        #endif

        // convert VT_TRUE and any other non-zero values to TRUE
        *(VARIANT_BOOL *)pv = V_BOOL(pVArgCopy);
        break;

    case VT_I2:
        *(short *)pv = V_I2(pVArgCopy);
        break;

    case VT_ERROR:
    case VT_I4:
        *(long *)pv = V_I4(pVArgCopy);
        break;

    case VT_R4:
        *(float *)pv = V_R4(pVArgCopy);
        break;

    case VT_R8:
        *(double *)pv = V_R8(pVArgCopy);
        break;

    case VT_CY:
        *(CY *)pv = V_CY(pVArgCopy);
        break;

    //
    // All Pointer types.
    //
    case VT_BSTR:
        if (wMaxstrlen && FormsStringLen(V_BSTR(pVArgCopy)) > wMaxstrlen)
        {
            hr = FormsAllocStringLen(V_BSTR(pVArgCopy), wMaxstrlen, (BSTR *)pv);
            if (hr)
                goto Cleanup;

            if (*pfAlloc)
                VariantClear(&vargNew);
            else
                *pfAlloc = TRUE;

            goto Cleanup;
        }
    case VT_PTR:
    case VT_LPSTR:
    case VT_LPWSTR:
    case VT_DISPATCH:
    case VT_UNKNOWN:
        *(void **)pv = V_BYREF(pVArgCopy);
        break;

    case VT_VARIANT:
        hr = ClipVarString(pVArgCopy, (VARIANT *)pv, pfAlloc, wMaxstrlen);
        if (hr == S_FALSE)
        {
            hr = S_OK;
            // Copy entire variant to output parameter.
            *(VARIANT *)pv = *pVArgCopy;
        }

        break;

    default:
        Assert(FALSE && "Unknown type in VARIANTARGToCVar().\n");
        hr = DISP_E_TYPEMISMATCH;
        break;
    }

Cleanup:
    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Function:   VARIANTARGToIndex
//
//  Synopsis:   Converts a VARIANT to an index of type long. Sets the index to
//              -1 if the VARIANT type is bad or empty.
//
//  Arguments:  [pvarg]         -- Variant to convert.
//              [plIndex        -- Location to place index.
//
//  Notes:      Useful special case of VARIANTARGToCVar for reading array
//              indices.
//----------------------------------------------------------------------------

HRESULT
VARIANTARGToIndex(VARIANT * pvarg, long * plIndex)
{
    HRESULT         hr = S_OK;
        
    Assert(pvarg);
    *plIndex = -1;

    // Quick return for the common case
    if(V_VT(pvarg) == VT_I4 || V_VT(pvarg) == (VT_I4 | VT_BYREF))
    {
        *plIndex = (V_VT(pvarg) == VT_I4) ? V_I4(pvarg) : *V_I4REF(pvarg);
        return S_OK;
    }

    if (V_VT(pvarg) == VT_ERROR || V_VT(pvarg) == VT_EMPTY)
    {
        return S_OK;
    }

    // Must perform type corecion
    CVariant varNum;
    hr = THR(VariantChangeTypeEx(&varNum, pvarg, LCID_SCRIPTING, 0, VT_I4));
    if (hr)
        goto Cleanup;

    Assert(V_VT(&varNum) == VT_I4 || V_VT(&varNum) == (VT_I4 | VT_BYREF));
    *plIndex = (V_VT(&varNum) == VT_I4) ? V_I4(&varNum) : *V_I4REF(&varNum);

Cleanup:
    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Function:   CVarToVARIANTARG
//
//  Synopsis:   Converts a C-language variable to a VARIANT.
//
//  Arguments:  [pv]    -- Pointer to C-language variable.
//              [vt]    -- Type of C-language variable.
//              [pvarg] -- Resulting VARIANT.  Must be initialized by caller.
//                         Any contents will be freed.
//
//  Modifies:   [pvarg]
//
//  History:    2-23-94   adams   Created
//
//  Notes:      Supports all variant pointer types, VT_UI2, VT_I2, VT_UI4,
//              VT_I4, VT_R4, VT_R8, VT_ERROR.
//
//----------------------------------------------------------------------------

void
CVarToVARIANTARG(void* pv, VARTYPE vt, VARIANTARG * pvarg)
{
    Assert(pv);
    Assert(pvarg);

    VariantClear(pvarg);

    V_VT(pvarg) = vt;
    if (V_ISBYREF(pvarg))
    {
        // Use a supported pointer type for derefencing.
        vt = VT_UNKNOWN;
    }

    switch (vt)
    {
    case VT_BOOL:
        // convert TRUE to VT_TRUE
        Assert(*(BOOL *) pv == 1 || *(BOOL *) pv == 0);
        V_BOOL(pvarg) = VARIANT_BOOL(-*(BOOL *) pv);
        break;

    case VT_I2:
        V_I2(pvarg) = *(short *) pv;
        break;

    case VT_ERROR:
    case VT_I4:
        V_I4(pvarg) = *(long *) pv;
        break;

    case VT_R4:
         V_R4(pvarg) = *(float *) pv;
        break;

    case VT_R8:
        V_R8(pvarg) = *(double *) pv;
        break;

    case VT_CY:
        V_CY(pvarg) = *(CY *) pv;
        break;

    //
    // All Pointer types.
    //
    case VT_PTR:
    case VT_BSTR:
    case VT_LPSTR:
    case VT_LPWSTR:
    case VT_DISPATCH:
    case VT_UNKNOWN:
        V_BYREF(pvarg) = *(void **)pv;
        break;

    default:
        Assert(FALSE && "Unknown type.");
        break;
    }
}



//+---------------------------------------------------------------------------
//
//  Function:   CParamsToDispParams
//
//  Synopsis:   Converts a C parameter list to a dispatch parameter list.
//
//  Arguments:  [pDispParams] -- Resulting dispatch parameter list.
//                               Note that the rgvarg member of pDispParams
//                               must be initialized with an array of
//                               EVENTPARAMS_MAX VARIANTs.
//
//              [pb]         -- List of C parameter types.  May be NULL.
//                              Construct using EVENT_PARAM macro.
//
//              [va]          -- List of C arguments.
//
//  Modifies:   [pDispParams]
//
//  History:    05-Jan-94   adams   Created
//              23-Feb-94   adams   Reversed order of disp arguments, added
//                                  support for VT_R4, VT_R8, and pointer
//                                  types.
//
//----------------------------------------------------------------------------

void
CParamsToDispParams(
        DISPPARAMS *    pDispParams,
        BYTE *          pb,
        va_list         va)
{
    Assert(pDispParams);
    Assert(pDispParams->rgvarg);

    VARIANTARG *    pvargCur;           // current variant
    BYTE *          pbCur;              // current vartype

    // Assign vals to dispatch param list.
    pDispParams->cNamedArgs         = 0;
    pDispParams->rgdispidNamedArgs  = NULL;

    pDispParams->cArgs = strlen((char *) pb);
    Assert(pDispParams->cArgs < EVENTPARAMS_MAX);

    //
    // Convert each C-param to a dispparam.  Note that the order of dispatch
    // parameters is the reverse of the order of c-params.
    //

    Assert(pDispParams->rgvarg);
    pvargCur = pDispParams->rgvarg + pDispParams->cArgs;
    for (pbCur = pb; *pbCur; pbCur++)
    {
        pvargCur--;
        Assert(pvargCur >= pDispParams->rgvarg);

        V_VT(pvargCur) = VARTYPEFromBYTE(*pbCur);
        if (V_VT(pvargCur) & VT_BYREF)
        {
            V_BYREF(pvargCur) = va_arg(va, long *);
        }
        else
        {
            switch (V_VT(pvargCur))
            {
            case VT_BOOL:
                // convert TRUE to VT_TRUE
                V_BOOL(pvargCur) = VARIANT_BOOL(-va_arg(va, BOOL));
                Assert(V_BOOL(pvargCur) == VB_FALSE ||
                        V_BOOL(pvargCur) == VB_TRUE);
                break;

            case VT_I2:
                V_I2(pvargCur) = va_arg(va, short);
                break;

            case VT_ERROR:
            case VT_I4:
                V_I4(pvargCur) = va_arg(va, long);
                break;

            case VT_R4:
                V_R4(pvargCur) = (float) va_arg(va, double);
                // casting & change to double inserted to fix BUG 5005
                break;

            case VT_R8:
                V_R8(pvargCur) = va_arg(va, double);
                break;

            //
            // All Pointer types.
            //
            case VT_PTR:
            case VT_BSTR:
            case VT_LPSTR:
            case VT_LPWSTR:
            case VT_DISPATCH:
            case VT_UNKNOWN:
                V_BYREF(pvargCur) = va_arg(va, void **);
                break;

            case VT_VARIANT:
                *pvargCur = va_arg(va, VARIANT);
                break;

            default:
                Assert(FALSE && "Unknown type.\n");
            }
        }
    }
}



//+---------------------------------------------------------------------------
//
//  Function:   DispParamsToCParams
//
//  Synopsis:   Converts Dispatch::Invoke method params to C-language params.
//
//  Arguments:  [pDP] -- Dispatch params to be converted.
//              [pb]  -- Array of types of C-params.  May be NULL.
//                       Construct using EVENT_PARAM macro.
//              [...] -- List of pointers to c-params to be converted to.
//              -1    -- Last parameter to signal end of parameter list.
//
//  Returns:    HRESULT.
//
//  History:    2-23-94   adams   Created
//
//  Notes:      Supports types listed in VARIANTToCParam.
//
//----------------------------------------------------------------------------

HRESULT
DispParamsToCParams(
        IServiceProvider   *pSrvProvider,
        DISPPARAMS         *pDP,
        ULONG              *pAlloc,
        WORD                wMaxstrlen,
        VARTYPE            *pVT,
        ...)
{
    HRESULT         hr;
    va_list         va;                // list of pointers to c-params.
    VARIANTARG *    pvargCur;          // current VARIANT being converted.
    void *          pv;                // current c-param being converted.
    UINT            cArgs;             // count of arguments.

    Assert(pDP);

    hr = S_OK;
    va_start(va, pVT);
    if (!pVT)
    {
        if (pDP->cArgs > 0)
            goto BadParamCountError;

        goto Cleanup;
    }

    pvargCur = pDP->rgvarg + pDP->cArgs - 1;
    for (cArgs = 0; cArgs < pDP->cArgs; cArgs++)
    {
        BOOL    fAlloc;

        // If the DISPID_THIS named argument is passed in skip it.
        if (pDP->cNamedArgs && (pDP->cArgs - cArgs <= pDP->cNamedArgs))
        {
            if (pDP->rgdispidNamedArgs[(pDP->cArgs - cArgs) - 1] == DISPID_THIS)
            {
                pvargCur--;
                continue;
            }
        }

        pv = va_arg(va, void *);

        // Done processing arguments?
        if (pv == (void *)-1)
            goto Cleanup;

        // Skip all byvalue variants custom invoke doesn't pass them.
        if (!((*pVT == VT_VARIANT) && (pv == NULL)))
        {
            hr = THR(VARIANTARGToCVar(pvargCur, &fAlloc, *pVT, pv, pSrvProvider, ((wMaxstrlen == pdlNoLimit) ? 0 : wMaxstrlen)));
            if (hr)
                goto Cleanup;

            // Any BSTRs or objects (IUnknow, IDispatch) allocated during
            // conversion to CVar then remember which param this occurred to so
            // we can de-allocate it when we're finished.
            if (pAlloc && fAlloc)
            {
                *pAlloc |= (1 << cArgs);
            }
        }

        pvargCur--;
        pVT++;
    }

Cleanup:
    va_end(va);
    RRETURN(hr);

BadParamCountError:
    hr = DISP_E_BADPARAMCOUNT;
    goto Cleanup;
}

//----------------------------------------------------------------------
//
// Function: GetNamedProp
//
// Description: Gets a property by name.  Does all the work for you
//
//----------------------------------------------------------------------
HRESULT
GetNamedProp(IDispatch *pDispatch, BSTR bstrPropName, LCID lcid, VARIANT *pv, DISPID *pDispid, EXCEPINFO *pexecpinfo, BOOL fMethodCall, BOOL fCaseSensitive)
{
    HRESULT hr;
    DISPID dispid;
    IDispatchEx *pDEX = 0;
    DWORD flags = fMethodCall ? DISPATCH_METHOD : DISPATCH_PROPERTYGET;
    DISPPARAMS dp;
    UINT uiErr;

    if (pDispid == 0)
        pDispid = &dispid;
    dp.rgvarg = NULL;
    dp.rgdispidNamedArgs = NULL;
    dp.cArgs = 0;
    dp.cNamedArgs = 0;

    VariantInit(pv);

    if (fCaseSensitive)
    {
        hr = THR(pDispatch->QueryInterface(IID_IDispatchEx, (LPVOID *) &pDEX));
        if (hr)
            goto Cleanup;

        hr = THR(pDEX->GetDispID(bstrPropName, fdexNameCaseSensitive , pDispid));
        if (hr)
            goto Cleanup;

        hr = THR(pDEX->InvokeEx(*pDispid, lcid, flags, &dp, pv, pexecpinfo, NULL));
        if (hr)
            goto Cleanup;
    }
    else
    {
        hr = THR(pDispatch->GetIDsOfNames(IID_NULL, &bstrPropName, 1, lcid, pDispid));
        if (hr)
            goto Cleanup;

        hr = THR(pDispatch->Invoke(*pDispid, IID_NULL, lcid, flags, &dp, pv, pexecpinfo, &uiErr));
        if (hr)
            goto Cleanup;
    }

Cleanup:
    ReleaseInterface(pDEX);
    RRETURN(hr);
}


//+---------------------------------------------------------------------------
//
//  Function:   GetDispProp
//
//  Synopsis:   Gets a property of an object.
//
//  Arguments:  [pDisp]  -- The object containing the property.
//              [dispid] -- The ID of the property.
//              [lcid]   -- The locale of the object.
//              [pvar]   -- The resulting property.  Must be initialized.
//              [pexcepinfo] -- where caller wants exception info
//              [fMethodCall] -- a straight method call should be used,
//                               rather than property-specific mechanisms
//
//  Returns:    HRESULT.
//
//  Modifies:   [pvarg].
//
//  History:    23-Feb-94   adams   Created
//
//----------------------------------------------------------------------------

HRESULT
GetDispProp(
        IDispatch * pDisp,
        DISPID      dispid,
        LCID        lcid,
        VARIANT *   pvar,
        EXCEPINFO * pexcepinfo,
        DWORD       dwFlags)
{
    HRESULT     hr;
    DISPPARAMS  dp;                    // Params for IDispatch::Invoke.
    UINT        uiErr;                 // Argument error.

    Assert(pDisp);
    Assert(pvar);

    dwFlags = (dwFlags & DISPATCH_METHOD) ? DISPATCH_METHOD : DISPATCH_PROPERTYGET;
    
    dp.rgvarg = NULL;
    dp.rgdispidNamedArgs = NULL;
    dp.cArgs = 0;
    dp.cNamedArgs = 0;

    hr = THR_NOTRACE(pDisp->Invoke(
            dispid,
            IID_NULL,
            lcid,
            dwFlags,
            &dp,
            pvar,
            pexcepinfo,
            &uiErr));


    RRETURN1(hr, DISP_E_MEMBERNOTFOUND);
}



//+---------------------------------------------------------------------------
//
//  Function:   SetDispProp
//
//  Synopsis:   Sets a property on an object.
//
//  Arguments:  [pDisp]  -- The object to set the property on.
//              [dispid] -- The ID of the property.
//              [lcid]   -- The locale of the property.
//              [pvarg]  -- The value to set.
//              [pexcepinfo] -- where caller wants exception info
//              [fMethodCall] -- a straight method call should be used,
//                               rather than property-specific mechanisms
//
//  Returns:    HRESULT.
//
//  History:    23-Feb-94   adams   Created
//
//----------------------------------------------------------------------------

HRESULT
SetDispProp(
        IDispatch *     pDisp,
        DISPID          dispid,
        LCID            lcid,
        VARIANTARG *    pvarg,
        EXCEPINFO *     pexcepinfo,
        DWORD           dwFlags)
{
    HRESULT     hr;
    DISPID      dispidPut = DISPID_PROPERTYPUT; // Dispid of prop arg.
    DISPPARAMS  dp;                    // Params for Invoke
    UINT        uiErr;                 // Invoke error param.

    Assert(pDisp);
    Assert(pvarg);

    dp.rgvarg = pvarg;
    dp.cArgs = 1;
    
    if (dwFlags & DISPATCH_METHOD)
    {
        dwFlags = DISPATCH_METHOD;
        dp.cNamedArgs = 0;
        dp.rgdispidNamedArgs = NULL;
    }
    else
    {
        dwFlags = (dispid == DISPID_FONT || (dwFlags & DISPATCH_PROPERTYPUTREF))
                    ? DISPATCH_PROPERTYPUTREF : DISPATCH_PROPERTYPUT;
        dp.cNamedArgs = 1;
        dp.rgdispidNamedArgs = &dispidPut;
    }
    hr = THR_NOTRACE(pDisp->Invoke(
            dispid,
            IID_NULL,
            lcid,
            dwFlags,
            &dp,
            NULL,
            pexcepinfo,
            &uiErr));

    RRETURN1(hr, DISP_E_MEMBERNOTFOUND);
}



//+---------------------------------------------------------------------------
//
//  Function:   CallDispMethod
//
//  Synopsis:   Calls a late-bound method on a object via IDispatch::Invoke.
//
//  Arguments:  [pDisp]     -- Object to call method on.
//              [dispid]    -- Method ID.
//              [lcid]      -- Locale of method.
//              [vtReturn]  -- Type of return value.  If no return value,
//                             must be VT_VOID.
//              [pvReturn]  -- Location of return value.  If no return value,
//                             must be NULL.
//              [pbParams]  -- List of param types.  May be NULL.
//                             Construct using EVENT_PARAM macro.
//              [...]       -- List of params.
//
//  Returns:    HRESULT.
//
//  History:    2-23-94   adams   Created
//
//----------------------------------------------------------------------------

HRESULT
CallDispMethod(
        IServiceProvider *pSrvProvider,
        IDispatch * pDisp,
        DISPID      dispid,
        LCID        lcid,
        VARTYPE     vtReturn,
        void *      pvReturn,
        BYTE *      pbParams,
        ...)
{
    HRESULT     hr;
    VARIANTARG  av[EVENTPARAMS_MAX];   // List of args for Invoke.
    DISPPARAMS  dp;                    // Params for Invoke.
    VARIANT     varReturn;             // Return value.
    va_list     va;                    // List of C-params.
    EXCEPINFO   excepinfo;             // macnote: this is a required param to IDispatch->Invoke
    BOOL        fVariantClear = FALSE;
    IDispatchEx *pDispEx = NULL;

    Assert(pDisp);
    Assert((vtReturn != VT_VOID) == (pvReturn != NULL));

    va_start(va, pbParams);
    dp.rgvarg = av;
    CParamsToDispParams(&dp, pbParams, va);
    va_end(va);
    memset(&excepinfo,0,sizeof(excepinfo));

    if (pvReturn)
        VariantInit(&varReturn);

    // use IDispatchEx, if available
    hr = pDisp->QueryInterface(IID_IDispatchEx, (void**)&pDispEx);

    if (!hr && pDispEx)
    {
        hr = THR_NOTRACE(pDispEx->InvokeEx(
                dispid,
                lcid,
                DISPATCH_METHOD,
                &dp,
                pvReturn ? &varReturn : NULL,
                &excepinfo,
                pSrvProvider));
    }
    else
    {
        hr = THR_NOTRACE(pDisp->Invoke(
                dispid,
                IID_NULL,
                lcid,
                DISPATCH_METHOD,
                &dp,
                pvReturn ? &varReturn : NULL,
                &excepinfo,
                NULL));
    }
    if (hr)
        goto Cleanup;

    if (pvReturn)
        hr = THR(VARIANTARGToCVar(&varReturn, &fVariantClear, vtReturn, pvReturn, pSrvProvider));
    if (fVariantClear)
        VariantClear(&varReturn);

Cleanup:
    FreeEXCEPINFO(&excepinfo);
    ReleaseInterface(pDispEx);
    RRETURN1(hr, DISP_E_MEMBERNOTFOUND);
}



//+------------------------------------------------------------------------
//
//  Function:   IsVariantEqual, public API
//
//  Synopsis:   Compares the values of two VARIANTARGs.
//
//  Arguments:  [pvar1], [pvar2] -- VARIANTARGs to compare.
//
//  Returns:    TRUE if equal, FALSE if not.
//
//  History:    18-Mar-93   SumitC      Created.
//              11-May-94   SumitC      don't assert for VT_UNKNOWN
//
//  Notes:      Variant type unequal returns FALSE, even if actual values
//              are the same.
//              Currently does I2, I4, R4, R8, CY, BSTR, BOOL
//              Returns FALSE for all other VariantTypes.
//
//-------------------------------------------------------------------------

BOOL
IsVariantEqual( VARIANTARG FAR* pvar1, VARIANTARG FAR* pvar2 )
{
    if( V_VT(pvar1) != V_VT(pvar2) )
        return FALSE;

    switch (V_VT(pvar1))
    {
    case VT_EMPTY :
    case VT_NULL:
        return TRUE;    // just the types being equal is good enough

    case VT_I2 :
        return (V_I2(pvar1) == V_I2(pvar2));

    case VT_I4 :
        return (V_I4(pvar1) == V_I4(pvar2));

    case VT_R4 :
        return (V_R4(pvar1) == V_R4(pvar2));

    case VT_R8 :
        return (V_R8(pvar1) == V_R8(pvar2));

    case VT_CY :
        return !memcmp(&V_CY(pvar1), &V_CY(pvar2), sizeof(CY));

    case VT_BSTR :
        return !FormsStringCmp(V_BSTR(pvar1), V_BSTR(pvar2));

    case VT_BOOL :
        return (V_BOOL(pvar1) == V_BOOL(pvar2));

    case VT_PTR:
    case VT_DISPATCH:
    case VT_UNKNOWN:
        // returns FALSE unless the objects are the same
        return (V_UNKNOWN(pvar1) == V_UNKNOWN(pvar2));

    default:
        Assert(0 && "Type not handled");
        break;
    };

    return(FALSE);
}


//+------------------------------------------------------------------------
//  Function:   DispParamsToSAFEARRAY, public API
//
//  Synopsis:   Converts all arguments in dispparams to a SAFEARRAY
//              If the DISPPARAMS contains no arguments we should create
//              an empty SAFEARRAY.
//
//  Arguments:  [pdispparams] -- VARIANTARGs to add to safearray.
//
//  Returns:    If the DISPPARAMS contains no arguments we should create an
//              empty SAFEARRAY.  It is the responsibility of the caller to
//              call SafeArrayDestroy.
//-------------------------------------------------------------------------

SAFEARRAY *
DispParamsToSAFEARRAY (DISPPARAMS *pdispparams)
{
    SAFEARRAY  *psa = NULL;
    HRESULT     hr = S_OK;

    LONG saElemIdx;
    SAFEARRAYBOUND  sabounds;
    const LONG cArgsToArray = pdispparams->cArgs;
    const LONG cArgsNamed = pdispparams->cNamedArgs;

    sabounds.cElements = cArgsToArray;

    // If first named arg is DISPID_THIS then this parameter won't be part of
    // the safearray.
    if (cArgsNamed)
    {
        if (pdispparams->rgdispidNamedArgs[0] == DISPID_THIS)
            sabounds.cElements--;
    }

    sabounds.lLbound = 0;
    psa = SafeArrayCreate(VT_VARIANT, 1, &sabounds);
    if (psa == NULL)
        goto Cleanup;

    // dispparams are in right to left order.
    for( saElemIdx = 0; saElemIdx < cArgsToArray; saElemIdx++ )
    {
        // Don't process any DISPID_THIS named arguments.
        if (cArgsNamed && (cArgsToArray - saElemIdx <= cArgsNamed))
        {
            if (pdispparams->rgdispidNamedArgs[(cArgsToArray - saElemIdx) - 1] == DISPID_THIS)
                continue;
        }

        hr = SafeArrayPutElement(psa, &saElemIdx, pdispparams->rgvarg + (cArgsToArray - 1 - saElemIdx) );
        if (hr)
            goto Cleanup;
    }

Cleanup:
    if (hr && psa)
    {
        hr = SafeArrayDestroy(psa);
        psa = NULL;
    }

    return psa;
}

//+------------------------------------------------------------------------
//
//  Class:      CInvoke
//
//-------------------------------------------------------------------------

CInvoke::CInvoke()
{
    memset (this, 0, sizeof(*this));
}

CInvoke::CInvoke (IDispatchEx * pdispex)
{
    memset (this, 0, sizeof(*this));
    IGNORE_HR(Init(pdispex));
}

CInvoke::CInvoke (IDispatch * pdisp)
{
    memset (this, 0, sizeof(*this));
    IGNORE_HR(Init(pdisp));
}

CInvoke::CInvoke (IUnknown * punk)
{
    memset (this, 0, sizeof(*this));
    IGNORE_HR(Init(punk));
}

CInvoke::CInvoke (CBase * pBase)
{
    memset (this, 0, sizeof(*this));
    IGNORE_HR(Init(pBase));
}

CInvoke::~CInvoke()
{
    Clear();
}

HRESULT
CInvoke::Init(IDispatchEx * pdispex)
{
    ReplaceInterface (&_pdispex, pdispex);

    return S_OK;
};

HRESULT
CInvoke::Init(IDispatch * pdisp)
{
    ReplaceInterface (&_pdisp, pdisp);

    return S_OK;
};

HRESULT
CInvoke::Init(IUnknown * punk)
{
    HRESULT hr;

    hr = THR(punk->QueryInterface(IID_IDispatchEx, (void**)&_pdispex));
    if (hr)
    {
        _pdispex = NULL;

        hr = THR(punk->QueryInterface(IID_IDispatch, (void**)&_pdisp));
    }

    RRETURN (hr);
}

HRESULT
CInvoke::Init(CBase * pBase)
{
    HRESULT hr;

    hr = THR(pBase->PrivateQueryInterface(IID_IDispatchEx, (void**)&_pdispex));

    RRETURN (hr);
}

void
CInvoke::Clear()
{
    ClearInterface(&_pdispex);
    ClearInterface(&_pdisp);
    ClearArgs();
    ClearRes();
}

void
CInvoke::ClearArgs()
{
    UINT i;

    for (i = 0; i < _dispParams.cArgs; i++)
    {
        VariantClear(&_aryvarArg[i]);
    }
    _dispParams.cArgs  = 0;
    _dispParams.rgvarg = NULL;
}

void
CInvoke::ClearRes()
{
    VariantClear(&_varRes);
}

HRESULT
CInvoke::Invoke (DISPID dispid, WORD wFlags)
{
    HRESULT     hr;

    Assert (_pdispex || _pdisp);

    if (_pdispex)
    {
        hr = THR_NOTRACE(_pdispex->InvokeEx(
            dispid, g_lcidUserDefault, wFlags, &_dispParams, &_varRes, &_excepInfo, NULL));
    }
    else if (_pdisp)
    {
        UINT    nArgErr;

        hr = THR_NOTRACE(_pdisp->Invoke(
            dispid, IID_NULL, g_lcidUserDefault, wFlags, &_dispParams, &_varRes, &_excepInfo, &nArgErr));
    }
    else
    {
        hr = E_FAIL;
    }

    RRETURN (hr);
}

HRESULT
CInvoke::AddArg()
{
    HRESULT     hr = S_OK;

    if (ARRAY_SIZE(_aryvarArg) <= _dispParams.cArgs)
        RRETURN (E_NOTIMPL);

    if (0 == _dispParams.cArgs)
    {
        _dispParams.rgvarg = _aryvarArg;
    }

    _dispParams.cArgs++;

    RRETURN (hr);
}

HRESULT
CInvoke::AddArg(VARIANT * pvarArg)
{
    HRESULT     hr;

    hr = THR(AddArg());
    if (hr)
        goto Cleanup;

    VariantCopy(&_aryvarArg[_dispParams.cArgs - 1], pvarArg);

Cleanup:

    RRETURN (hr);
}

HRESULT
CInvoke::AddNamedArg(DISPID dispid)
{
    HRESULT     hr = S_OK;

    if (ARRAY_SIZE(_arydispidNamedArg) <= _dispParams.cNamedArgs)
        RRETURN (E_NOTIMPL);

    if (0 == _dispParams.cNamedArgs)
    {
        _dispParams.rgdispidNamedArgs = _arydispidNamedArg;
    }

    _dispParams.cNamedArgs++;

    _arydispidNamedArg[_dispParams.cNamedArgs - 1] = dispid;

    RRETURN (hr);
}
