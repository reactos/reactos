//=================================================
//
//  File : CVariant.cxx
//
//  purpose : implementation of a very usefull VARIANT wrapper class
//
//=================================================


#include "headers.h"
#include "utils.hxx"

#ifndef X_DISPEX_H_
#define X_DISPEX_H_
#include <dispex.h>
#endif



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
VariantChangeTypeSpecial(VARIANT *pVArgDest, 
                         VARIANT *pvarg, 
                         VARTYPE vt,
                         IServiceProvider *pSrvProvider, 
                         WORD wFlags)
{
    HRESULT             hr;
    IVariantChangeType *pVarChangeType = NULL;

    if (pSrvProvider)
    {
        hr = pSrvProvider->QueryService(SID_VariantConversion,
                                            IID_IVariantChangeType,
                                            (void **)&pVarChangeType);
        if (hr)
            goto OldWay;

        // Use script engine conversion routine.
    	hr = pVarChangeType->ChangeType(pVArgDest, pvarg, 0, vt);
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
        V_BSTR(pVArgDest) = SysAllocString( L"null");
        if (! V_BSTR(pVArgDest) )
            hr = E_OUTOFMEMORY;

        goto Cleanup;
    }
    else if (vt == VT_BSTR && V_VT(pvarg) == VT_EMPTY)
    {
        // Converting "undefined" to BSTR
        V_VT(pVArgDest) = VT_BSTR;
        V_BSTR(pVArgDest) = SysAllocString( L"undefined");
        if (! V_BSTR(pVArgDest) )
            hr = E_OUTOFMEMORY;

        goto Cleanup;
    }
    else if (vt == VT_BOOL && V_VT(pvarg) == VT_BSTR)
    {
        // Converting from BSTR to BOOL
        // To match Navigator compatibility empty strings implies false when
        // assigned to a boolean type any other string implies true.
        V_VT(pVArgDest) = VT_BOOL;
        V_BOOL(pVArgDest) = SysStringLen(V_BSTR(pvarg)) == 0 ? VB_FALSE : VB_TRUE;
        goto Cleanup;
    }
    else if (  V_VT(pvarg) == VT_BOOL && vt == VT_BSTR )
    {
        // Converting from BOOL to BSTR
        // To match Nav we either get "true" or "false"
        V_VT(pVArgDest) = VT_BSTR;
        V_BSTR(pVArgDest) = SysAllocString( V_BOOL(pvarg) == VB_TRUE ? L"true" : L"false");
        if (! V_BSTR(pVArgDest) )
            hr = E_OUTOFMEMORY;

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
                V_BSTR(pVArgDest) = SysAllocString(L"NaN");
                if (! V_BSTR(pVArgDest)) 
                    hr = E_OUTOFMEMORY;
            }
            else
            {
                // Infinity
                V_BSTR(pVArgDest) = SysAllocString((dblValue < 0) ? L"-Infinity" : L"Infinity" );
                if (! V_BSTR(pVArgDest)) 
                    hr = E_OUTOFMEMORY;
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
    hr = VariantChangeTypeEx(pVArgDest, 
                             pvarg, 
                             LCID_SCRIPTING, 
                             wFlags|VARIANT_NOUSEROVERRIDE, 
                             vt);

    
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
            V_BSTR(pVArgDest) = SysAllocString ( (V_DISPATCH(pvarg)) ? L"[object]" : L"null");
            if (! V_BSTR(pVArgDest) )
                hr = E_OUTOFMEMORY;

        }
        else if ( V_VT(pvarg) == VT_BSTR && V_BSTRREF(pvarg)  &&
            ( (V_BSTR(pvarg))[0] == _T('\0')) && (  vt == VT_I4 || vt == VT_I2 || vt == VT_UI2 || vt == VT_UI4 || vt == VT_I8 ||
                vt == VT_UI8 || vt == VT_INT || vt == VT_UINT ) )
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

Cleanup:
    ReleaseInterface(pVarChangeType);

    return hr ;
}



//+--------------------------------------------------------
//
//   Method : CVariant::CoerceVariantArg 
//
//  Synopsis : Coerce current variant into itself or 
//      Coerce pArgFrom into this instance from anyvariant 
//      to a given type
//
//+--------------------------------------------------------

HRESULT CVariant::CoerceVariantArg ( VARIANT *pArgFrom, WORD wCoerceToType)
{
    HRESULT hr = S_OK;
    VARIANT *pvar;

    if( V_VT(pArgFrom) == (VT_BYREF | VT_VARIANT) )
        pvar = V_VARIANTREF(pArgFrom);
    else
        pvar = pArgFrom;

    if ( !(pvar->vt == VT_EMPTY || pvar->vt == VT_ERROR ) )
    {
        hr = VariantChangeTypeSpecial ( (VARIANT *)this, 
                                    pvar,  
                                    wCoerceToType );
    }
    else
    {
        return S_FALSE;
    }

    return hr;
}

HRESULT CVariant::CoerceVariantArg (WORD wCoerceToType)
{
    HRESULT hr = S_OK;

    if ( !(vt == VT_EMPTY || vt == VT_ERROR ) )
    {
        hr = VariantChangeTypeSpecial ( (VARIANT *)this, 
                    (VARIANT *)this, wCoerceToType );
    }
    else
    {
        return S_FALSE;
    }

    return hr;
}


//+--------------------------------------------------------
//
//  Method : CVariant::CoerceNumericToI4 
//
//  Synopsis : Coerce any numeric (VT_I* or  VT_UI*) into a 
//      VT_I4 in this instance
//
//+--------------------------------------------------------
BOOL CVariant::CoerceNumericToI4 ()
{
    switch (vt)
    {
    case VT_I1:
    case VT_UI1:
        lVal = 0x000000FF & (DWORD)bVal;
        break;
    
    case VT_UI2:
    case VT_I2:
        lVal = 0x0000FFFF & (DWORD)iVal;
        break;
    
    case VT_UI4:
    case VT_I4:
    case VT_INT: 
    case VT_UINT:
        break;

    case VT_R8:
        lVal = (LONG)dblVal;
        break;

    case VT_R4:
        lVal = (LONG)fltVal;
        break;

    default:
        return FALSE;
    }

    vt = VT_I4;
    return TRUE;
}
