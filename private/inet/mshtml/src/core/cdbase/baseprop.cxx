//+---------------------------------------------------------------------------
//
//  Microsoft Forms³
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       baseprop.cxx
//
//  Contents:   CBase property setting utilities.
//
//----------------------------------------------------------------------------


#include "headers.hxx"

#ifndef X_STRBUF_HXX_
#define X_STRBUF_HXX_
#include "strbuf.hxx"
#endif

#ifndef X_TEAROFF_HXX_
#define X_TEAROFF_HXX_
#include "tearoff.hxx"
#endif

#ifndef X_QI_IMPL_H_
#define X_QI_IMPL_H_
#include "qi_impl.h"
#endif

#ifndef X_MSHTMLRC_H_
#define X_MSHTMLRC_H_
#include "mshtmlrc.h"
#endif

#ifndef X_FUNCSIG_HXX_
#define X_FUNCSIG_HXX_
#include "funcsig.hxx"
#endif

#ifndef X_OLECTL_H_
#define X_OLECTL_H_
#include <olectl.h>
#endif

#ifndef X_MSHTMDID_H_
#define X_MSHTMDID_H_
#include "mshtmdid.h"
#endif

MtDefine(StripCRLF, Utilities, "StripCRLF *ppDest")

// forward reference decl's: local helper function
static HRESULT StripCRLF(TCHAR *pSrc, TCHAR **pCstrDest);

/* flag values */
#define FL_UNSIGNED   1       /* wcstoul called */
#define FL_NEG        2       /* negative sign found */
#define FL_OVERFLOW   4       /* overflow occured */
#define FL_READDIGIT  8       /* we've read at least one correct digit */

static HRESULT RTCCONV PropertyStringToLong (
        LPCTSTR nptr,
        TCHAR **endptr,
        int ibase,
        int flags,
        unsigned long *plNumber );

HRESULT
BASICPROPPARAMS::GetAvString (void * pObject, const void * pvParams, CStr *pstr, BOOL * pfValuePresent ) const
{
    HRESULT hr = S_OK;
    LPCTSTR lpStr;
    BOOL  fDummy;

    if (!pfValuePresent)
        pfValuePresent = &fDummy;

    Assert(pstr);

    if (dwPPFlags & PROPPARAM_ATTRARRAY)
    {
        *pfValuePresent = CAttrArray::FindString ( *(CAttrArray**)pObject, ((PROPERTYDESC*)this) -1, &lpStr );
        // String pointer will be set to a default if not present
        pstr->Set ( lpStr );
    }
    else
    {
        //Stored as offset from a struct
        CStr *pstoredstr = (CStr *)(  (BYTE *)pObject + *(DWORD *)pvParams  );
        pstr -> Set ( (LPTSTR)*pstoredstr ); 
        *pfValuePresent = TRUE;
    }
    RRETURN(hr);
}

DWORD
BASICPROPPARAMS::GetAvNumber (void * pObject, const void * pvParams,
    UINT uNumBytes, BOOL * pfValuePresent  ) const
{
    DWORD dwValue;
    BOOL  fDummy;

    if (!pfValuePresent)
        pfValuePresent = &fDummy;

    if (dwPPFlags & PROPPARAM_ATTRARRAY)
    {
        *pfValuePresent = CAttrArray::FindSimple ( *(CAttrArray**)pObject, ((PROPERTYDESC*)this) -1, &dwValue  );
    }
    else
    {
        //Stored as offset from a struct
        BYTE *pbValue = (BYTE *)pObject + *(DWORD *)pvParams;
        dwValue = (DWORD)GetNumberOfSize( (void*) pbValue, uNumBytes);
        *pfValuePresent = TRUE;
    }
    return dwValue;
}


HRESULT
BASICPROPPARAMS::SetAvNumber ( void *pObject, DWORD dwNumber, const void *pvParams,
    UINT uNumberBytes, WORD wFlags /*=0*/ ) const
{
    HRESULT hr = S_OK;

    if (dwPPFlags & PROPPARAM_ATTRARRAY)
    {
        hr = CAttrArray::SetSimple ( (CAttrArray**)pObject,
            ((PROPERTYDESC*)this) -1, dwNumber, wFlags );
    }
    else
    {
        BYTE *pbData = (BYTE *)pObject + *(DWORD *)pvParams;
        SetNumberOfSize ( (void *)pbData, uNumberBytes, dwNumber );
    }
    RRETURN(hr);
}


//+---------------------------------------------------------------
//
//  Function:   GetGetSetMf
//
//  Synopsis:   Helper for getting get/set pointer to member functions
//
//----------------------------------------------------------------
#ifdef WIN16
typedef HRESULT (BUGCALL * PFN_ARBITRARYPROPGET) (CVoid *);
typedef HRESULT (BUGCALL * PFN_ARBITRARYPROPSET) (CVoid *);
#else
typedef HRESULT (BUGCALL CVoid::* PFN_ARBITRARYPROPGET) ( ... );
typedef HRESULT (BUGCALL CVoid::* PFN_ARBITRARYPROPSET) ( ... );
#endif

// ===========================  New generic handlers and helpers  =============================


inline HRESULT StringToLong( LPTSTR pch, long * pl)
{
    // Skip a leading '+' that would otherwise cause a
    // convert failure
//    RRETURN(VarI4FromStr(pch, LOCALE_SYSTEM_DEFAULT, 0, pl));
    *pl = _tcstol(pch, 0, 0);
    return S_OK;
}

inline HRESULT WriteText (IStream * pStm, TCHAR * pch)
{
    if (pch)
        return pStm->Write(pch, _tcslen(pch)*sizeof(TCHAR), 0);
    else
        return S_OK;
}

inline HRESULT WriteTextLen (IStream * pStm, TCHAR * pch, int nLen)
{
    return pStm->Write(pch, nLen*sizeof(TCHAR), 0);
}

inline HRESULT WriteTextCStr(CStreamWriteBuff * pStmWrBuff, CStr * pstr, BOOL fAlwaysQuote, BOOL fNeverQuote )
{
    UINT u;

    if ((u = pstr->Length()) != 0)
    {
        if ( fNeverQuote )
            RRETURN(pStmWrBuff->Write( *pstr ));
        else
            RRETURN(pStmWrBuff->WriteQuotedText(*pstr, fAlwaysQuote));
    }
    else
    {
        if ( !fNeverQuote )
            RRETURN(WriteText(pStmWrBuff, _T("\"\"")));
    }
    return S_OK;
}

inline HRESULT WriteTextLong(IStream * pStm, long l)
{
    TCHAR ach[20];

    HRESULT hr = Format(0, ach, ARRAY_SIZE(ach), _T("<0d>"), l);
    if (hr)
        goto Cleanup;

    hr = pStm->Write(ach, _tcslen(ach)*sizeof(TCHAR), 0);
    if (hr)
        goto Cleanup;

Cleanup:

    RRETURN(hr);
}


//+---------------------------------------------------------------
//
//  Function:   GetGetMethodPtr
//
//  Synopsis:   Helper for getting get/set pointer to member functions
//
//----------------------------------------------------------------

void
BASICPROPPARAMS::GetGetMethodP (const void * pvParams, void * pfn) const
{
    Assert(!(dwPPFlags & PROPPARAM_MEMBER));
    Assert(dwPPFlags & PROPPARAM_GETMFHandler);
    Assert(dwPPFlags & PROPPARAM_SETMFHandler);

    memcpy(pfn, pvParams, sizeof(PFN_ARBITRARYPROPGET));
}


//+---------------------------------------------------------------
//
//  Function:   GetSetMethodptr
//
//  Synopsis:   Helper for getting get/set pointer to member functions
//
//----------------------------------------------------------------

void
BASICPROPPARAMS::GetSetMethodP (const void * pvParams, void * pfn) const
{
    Assert(!(dwPPFlags & PROPPARAM_MEMBER));
    Assert(dwPPFlags & PROPPARAM_GETMFHandler);
    Assert(dwPPFlags & PROPPARAM_SETMFHandler);

    memcpy(pfn, (BYTE *)pvParams + sizeof(PFN_ARBITRARYPROPGET), sizeof(PFN_ARBITRARYPROPSET));
}


//+---------------------------------------------------------------
//
//  Member:     BASICPROPPARAMS::GetColorProperty, public
//
//  Synopsis:   Helper for setting color valued properties
//
//----------------------------------------------------------------



HRESULT
BASICPROPPARAMS::GetColorProperty(VARIANT * pVar, CBase * pObject, CVoid * pSubObject) const
{
    HRESULT hr;

    if (!pSubObject)
    {
        pSubObject = pObject;
    }

    if(!pVar)
    {
        hr = E_INVALIDARG;
    }
    else
    {
        VariantInit(pVar);
        V_VT(pVar) = VT_BSTR;
        hr = THR( GetColor(pSubObject,  &(pVar->bstrVal)) );
    }

    RRETURN(pObject->SetErrorInfoPGet(hr, dispid));
}


//+---------------------------------------------------------------
//
//  Member:     BASICPROPPARAMS::SetColorProperty, public
//
//  Synopsis:   Helper for setting color valued properties
//
//----------------------------------------------------------------

HRESULT
BASICPROPPARAMS::SetColorProperty(VARIANT var, CBase * pObject, CVoid * pSubObject, WORD wFlags) const
{
    CBase::CLock    Lock(pObject);
    HRESULT         hr=S_OK;
    DWORD           dwOldValue;
    CColorValue     cvValue;
    CVariant        *v = (CVariant *)&var;

    // check for shorts to keep vbscript happy and other VT_U* and VT_UI* types to keep C programmer's happy.
    // vbscript interprets values between 0x8000 and 0xFFFF in hex as a short!, jscript doesn't
    if (v->CoerceNumericToI4())
    {
        DWORD dwRGB = V_I4(v);

        // if -ve value or highbyte!=0x00, ignore (NS compat)
        if (dwRGB & CColorValue::MASK_FLAG)
            goto Cleanup;
        
        // flip RRGGBB to BBGGRR to be in CColorValue format
        cvValue.SetFromRGB(dwRGB);
    }
    else if (V_VT(&var) == VT_BSTR)
    {
// Removed 4/24/97 because "" clearing a color is a useful feature.  -CWilso
// if NULL or empty string, ignore (NS compat)
//        if (!(V_BSTR(&var)) || !*(V_BSTR(&var)))
//            goto Cleanup;

        hr = cvValue.FromString((LPTSTR)(V_BSTR(&var)), ( dwPPFlags & PROPPARAM_STYLESHEET_PROPERTY ) );
    }
    else
        goto Cleanup;   // if invalid type, ignore

    if (hr)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }


    if (!pSubObject)
    {
        pSubObject = pObject;
    }

    hr = GetColor( pSubObject, & dwOldValue );
    if (hr)
        goto Cleanup;

    if ( dwOldValue == (DWORD)cvValue.GetRawValue() )
    {
        // No change - ignore it
        hr = S_OK;
        goto Cleanup;
    }

#ifndef NO_EDIT
    if ( pObject->QueryCreateUndo(TRUE, FALSE) )
    {
        CStr    strOldColor;
        CVariant varOldColor;
        BOOL    fOldPresent;

        hr = THR( GetColor( pSubObject, &strOldColor, FALSE, &fOldPresent ) );

        if (hr)
            goto Cleanup;

        if (fOldPresent)
        {
            V_VT(&varOldColor) = VT_BSTR;
            hr = THR( strOldColor.AllocBSTR( &V_BSTR(&varOldColor) ) );
            if (hr)
                goto Cleanup;
        }
        else
        {
            V_VT(&varOldColor) = VT_NULL;
        }
        
        hr = THR(pObject->CreatePropChangeUndo(dispid, &varOldColor, NULL));
        
        if (hr)
            goto Cleanup;
    }
#endif // NO_EDIT

    hr = THR( SetColor( pSubObject, cvValue.GetRawValue(), wFlags ) );
    if (hr)
        goto Cleanup;

    hr = THR(pObject->OnPropertyChange(dispid, dwFlags));

    if (hr)
    {
        IGNORE_HR(SetColor(pSubObject, dwOldValue, wFlags));
        IGNORE_HR(pObject->OnPropertyChange(dispid, dwFlags));
    }

Cleanup:

    RRETURN1(pObject->SetErrorInfoPSet(hr, dispid), E_INVALIDARG);
}

//+---------------------------------------------------------------
//
//  Function:   SetString
//
//  Synopsis:   Helper for setting string value
//
//----------------------------------------------------------------

HRESULT
BASICPROPPARAMS::SetString(CVoid * pObject, TCHAR * pch, WORD wFlags) const
{
    HRESULT hr;

    if (dwPPFlags & PROPPARAM_SETMFHandler)
    {
        PFN_CSTRPROPSET pmfSet;
        CStr szString;
        szString.Set ( pch );

        GetSetMethodP(this + 1, &pmfSet );

#ifdef WIN16
        hr = (*pmfSet)( pObject, &szString);
#else
        hr = CALL_METHOD(pObject,pmfSet,( &szString));
#endif
    }
    else
    {

        if (dwPPFlags & PROPPARAM_ATTRARRAY)
        {            
            hr = THR( CAttrArray::SetString ( (CAttrArray**) (void*) pObject,
                    (PROPERTYDESC*)this - 1, pch, FALSE, wFlags ) );
            if (hr)
                goto Cleanup;
        }
        else
        {
            //Stored as offset from a struct
            CStr *pcstr;
            pcstr = (CStr *) (  (BYTE *)pObject + *(DWORD *)(this + 1)  );
            hr = THR( pcstr->Set(pch) );
            if (hr)
                goto Cleanup;
        }
    }

Cleanup:
    RRETURN( hr );
}

//+---------------------------------------------------------------
//
//  Function:   GetString
//
//  Synopsis:   Helper for getting string value
//
//----------------------------------------------------------------

HRESULT
BASICPROPPARAMS::GetString(CVoid * pObject, CStr * pcstr, BOOL *pfValuePresent) const
{
    HRESULT hr = S_OK;

    if (dwPPFlags & PROPPARAM_GETMFHandler)
    {
        PFN_CSTRPROPGET pmfGet;

        GetGetMethodP(this + 1, &pmfGet);

        // Get Method fn prototype takes a BSTR ptr
        //
#ifdef WIN16
        hr = (*pmfGet)( pObject, pcstr );
#else
        hr = CALL_METHOD(pObject,pmfGet,( pcstr ));
#endif
        if (pfValuePresent)
            *pfValuePresent = TRUE;
    }
    else
    {
        hr = GetAvString (pObject,this + 1,  pcstr, pfValuePresent);
    }

    RRETURN( hr );
}


//+---------------------------------------------------------------
//
//  Member:     SetStringProperty, public
//
//  Synopsis:   Helper for setting string values properties
//
//----------------------------------------------------------------

HRESULT
BASICPROPPARAMS::SetStringProperty(BSTR bstrNew, CBase * pObject, CVoid * pSubObject, WORD wFlags) const
{
    CBase::CLock    Lock(pObject);
    HRESULT         hr;
    CStr            cstrOld;
    BOOL            fOldPresent;

    if (!pSubObject)
    {
        pSubObject = pObject;
    }

    hr = GetString(pSubObject, &cstrOld, &fOldPresent);
    if (hr)
        goto Cleanup;

    // HACK! HACK! HACK! (MohanB) In order to fix #64710 at this very late
    // stage in IE5, I am putting in this hack to specifically check for
    // DISPID_CElement_id. For IE6, we should not fire onpropertychange for any
    // property (INCLUDING non-string type properties!) if the value of the
    // property has not been modified.

    // Quit if the value has not been modified
    if (    fOldPresent
        &&  DISPID_IHTMLELEMENT_ID == dispid
        &&  0 == _tcsicmp(cstrOld, bstrNew)
       )
    {
        goto Cleanup;
    }

#ifndef NO_EDIT
    if ( pObject->QueryCreateUndo(TRUE, FALSE) )
    {
        CVariant varOld;

        if (fOldPresent)
        {
            V_VT(&varOld) = VT_BSTR;
            hr = THR( cstrOld.AllocBSTR( &V_BSTR(&varOld) ) );
            if (hr)
                goto Cleanup;
        }
        else
        {
            V_VT(&varOld) = VT_NULL;
        }

        hr = THR(pObject->CreatePropChangeUndo(dispid, &varOld, NULL));
        if (hr)
            goto Cleanup;
    }
#endif // NO_EDIT

    hr = THR(SetString(pSubObject, (TCHAR *)bstrNew, wFlags));
    if (hr)
        goto Cleanup;

    hr = THR(pObject->OnPropertyChange(dispid, dwFlags));

    if (hr)
    {
        IGNORE_HR(SetString(pSubObject, (TCHAR *)cstrOld, wFlags));
        IGNORE_HR(pObject->OnPropertyChange(dispid, dwFlags));
    }

Cleanup:
    RRETURN(pObject->SetErrorInfoPSet(hr, dispid));
}

const PROPERTYDESC *HDLDESC::FindPropDescForName ( LPCTSTR szName, BOOL fCaseSensitive, long *pidx ) const
{
    const PROPERTYDESC * found = NULL;
    STRINGCOMPAREFN pfnCompareString = fCaseSensitive ? StrCmpC : StrCmpIC;
    if (pidx)
        *pidx = -1;

    if ( uNumPropDescs )
    {
        int r;
        const PROPERTYDESC * const *low = ppPropDescs;
        const PROPERTYDESC * const *high;
        const PROPERTYDESC * const *mid;

        high = low + uNumPropDescs - 1;

        // Binary search for property name
        while ( high >= low )
        {
            if ( high != low )
            {
                mid = low + ( ( ( high - low ) + 1 ) >> 1 );
                r = pfnCompareString( szName, (*mid)->pstrName );
                if ( r < 0 )
                {
                    high = mid - 1;
                }
                else if ( r > 0 )
                {
                    low = mid + 1;
                }
                else
                {
                    found = *mid;
                    if (pidx)
                        *pidx = mid - ppPropDescs;
                    break;
                }
            }
            else 
            {
                found = pfnCompareString( szName, (*low)->pstrName ) ? NULL : *low;
                if (pidx && found)
                    *pidx = low - ppPropDescs;
                break;
            }
        }
    }
    return found;
}

HRESULT ENUMDESC::EnumFromString ( LPCTSTR pStr, long *plValue, BOOL fCaseSensitive ) const
{   
    int i;
    HRESULT hr = S_OK;              
    STRINGCOMPAREFN pfnCompareString = fCaseSensitive ? StrCmpC : StrCmpIC;

    if (!pStr)
        pStr = _T("");

    for (i = cEnums - 1; i >= 0; i--)
    {
        if (!( pfnCompareString ( pStr, aenumpairs[i].pszName) ) )
        {
            *plValue = aenumpairs[i].iVal;
            break;
        }
    }

    if (i < 0)
    {
        hr = E_INVALIDARG;
    }

    RRETURN1(hr, E_INVALIDARG);
}

HRESULT ENUMDESC::StringFromEnum ( long lValue, BSTR *pbstr ) const
{
    int     i;
    HRESULT hr = E_INVALIDARG;

    for (i = 0; i < cEnums; i++)
    {
        if ( aenumpairs[i].iVal == lValue )
        {
            hr = THR(FormsAllocString( aenumpairs[i].pszName, pbstr ));
            break;
        }
    }
    RRETURN1(hr,E_INVALIDARG);
}


LPCTSTR ENUMDESC::StringPtrFromEnum ( long lValue ) const
{
    int     i;

    for (i = 0; i < cEnums; i++)
    {
        if ( aenumpairs[i].iVal == lValue )
        {
            return( aenumpairs[i].pszName );
        }
    }
    return( NULL );
}


HRESULT
NUMPROPPARAMS::SetEnumStringProperty(BSTR bstrNew, CBase * pObject, CVoid * pSubObject, WORD wFlags) const
{
    HRESULT hr = E_INVALIDARG;
    CBase::CLock    Lock(pObject);
    long lNewValue,lOldValue;
    BOOL fOldPresent;

    hr = GetNumber(pObject, pSubObject, &lOldValue, &fOldPresent);
    if ( hr )
        goto Cleanup;

    hr = LookupEnumString ( this, (LPTSTR)bstrNew, &lNewValue );
    if ( hr )
        goto Cleanup;
    
    if ( lNewValue == lOldValue )
        goto Cleanup;

    hr = ValidateNumberProperty( &lNewValue, pObject);
    if (hr)
        goto Cleanup;

#ifndef NO_EDIT
    if ( pObject->QueryCreateUndo( TRUE, FALSE ) )
    {
        CVariant      varOld;

        if (fOldPresent)
        {
            ENUMDESC *    pEnumDesc;
            const TCHAR * pchOldValue;

            if ( bpp.dwPPFlags & PROPPARAM_ANUMBER )
            {
                pEnumDesc = *(ENUMDESC **)((BYTE *)(this+1)+ sizeof(DWORD_PTR));
            }
            else
            {
                pEnumDesc = (ENUMDESC *) lMax;
            }

            pchOldValue = pEnumDesc->StringPtrFromEnum( lOldValue );

            V_VT(&varOld) = VT_BSTR;
            hr = FormsAllocString(pchOldValue, &V_BSTR(&varOld));
            if (hr)
                goto Cleanup;
        }
        else
        {
            V_VT(&varOld) = VT_NULL;
        }
        
        hr = THR( pObject->CreatePropChangeUndo( bpp.dispid, &varOld, NULL ) );
        if (hr)
            goto Cleanup;
    }
#endif // NO_EDIT
    
    hr = SetNumber(pObject, pSubObject, lNewValue, wFlags );
    if (hr)
        goto Cleanup;

    hr = THR(pObject->OnPropertyChange(bpp.dispid, bpp.dwFlags));

    CHECK_HEAP();

Cleanup:
    RRETURN(pObject->SetErrorInfoPGet(hr, bpp.dispid));
}

HRESULT
NUMPROPPARAMS::GetEnumStringProperty(BSTR *pbstr, CBase * pObject, CVoid * pSubObject) const
{
    HRESULT hr;
    VARIANT varValue;
    PROPERTYDESC *ppdPropDesc = ((PROPERTYDESC *)this)-1;

    if(!pbstr)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    if (!pSubObject)
    {
        pSubObject = pObject;
    }


	hr = THR(ppdPropDesc->HandleNumProperty( HANDLEPROP_AUTOMATION | (PROPTYPE_VARIANT<<16), 
        (void *)&varValue, pObject, pSubObject ));
    if ( !hr )
        *pbstr = V_BSTR(&varValue);

Cleanup:
    RRETURN(pObject->SetErrorInfoPGet(hr, bpp.dispid));
}

//+---------------------------------------------------------------
//
//  Member:     BASICPROPPARAMS::GetStringProperty, public
//
//  Synopsis:   Helper for setting string valued properties
//
//----------------------------------------------------------------

HRESULT
BASICPROPPARAMS::GetStringProperty(BSTR *pbstr, CBase * pObject, CVoid * pSubObject) const
{
    HRESULT hr;

    if (!pSubObject)
    {
        pSubObject = pObject;
    }

    if(!pbstr)
    {
        hr = E_INVALIDARG;
    }
    else
    {
        CStr    cstr;

        // BUGBUG: (anandra) Possibly have a helper here that returns
        // a bstr to avoid two allocations.
        hr = THR(GetString(pSubObject, &cstr));
        if (hr)
            goto Cleanup;
        hr = THR(cstr.AllocBSTR(pbstr));
    }

Cleanup:
    RRETURN(pObject->SetErrorInfoPGet(hr, dispid));
}


//+---------------------------------------------------------------
//
//  Function:   SetUrl
//
//  Synopsis:   Helper for setting url string value
//
//----------------------------------------------------------------

HRESULT
BASICPROPPARAMS::SetUrl(CVoid * pObject, TCHAR * pch, WORD wFlags) const
{
    HRESULT hr = S_OK;
    TCHAR   *pstrNoCRLF=NULL;

    hr = THR(StripCRLF(pch, &pstrNoCRLF));
    if (hr)
        goto Cleanup;

    hr =THR(SetString(pObject, pstrNoCRLF, wFlags));

Cleanup:
    if (pstrNoCRLF)
        MemFree(pstrNoCRLF);

    RRETURN( hr );
}

//+---------------------------------------------------------------
//
//  Member:     SetUrlProperty, public
//
//  Synopsis:   Helper for setting url-string values properties
//     strip off CR/LF and call setString
//
//----------------------------------------------------------------

HRESULT
BASICPROPPARAMS::SetUrlProperty(BSTR bstrNew, CBase * pObject, CVoid * pSubObject, WORD wFlags) const
{
    HRESULT   hr=S_OK;
    TCHAR    *pstrNoCRLF=NULL;

    hr = THR(StripCRLF((TCHAR*)bstrNew, &pstrNoCRLF));
    if (hr)
        goto Cleanup;

    // SetStringProperty calls Set ErrorInfoPSet
    hr = THR(SetStringProperty(pstrNoCRLF, pObject, pSubObject, wFlags));

Cleanup:
    if (pstrNoCRLF)
        MemFree(pstrNoCRLF);

    RRETURN(pObject->SetErrorInfoPSet(hr, dispid));
}

//+---------------------------------------------------------------
//
//  Function:   GetUrl
//
//  Synopsis:   Helper for getting url-string value
//
//----------------------------------------------------------------

HRESULT
BASICPROPPARAMS::GetUrl(CVoid * pObject, CStr * pcstr) const
{
    RRETURN( GetString(pObject, pcstr) );
}


//+---------------------------------------------------------------
//
//  Member:     BASICPROPPARAMS::GetUrlProperty, public
//
//  Synopsis:   Helper for setting url string valued properties
//
//----------------------------------------------------------------

HRESULT
BASICPROPPARAMS::GetUrlProperty(BSTR *pbstr, CBase * pObject, CVoid * pSubObject) const
{
    // SetString does the SetErrorInfoPGet
    RRETURN( GetStringProperty(pbstr, pObject, pSubObject));
}

//+---------------------------------------------------------------
//
//  Function:   GetNumber
//
//  Synopsis:   Helper for getting number values properties
//
//----------------------------------------------------------------

HRESULT
NUMPROPPARAMS::GetNumber (CBase * pObject, CVoid * pSubObject, long * plNum, BOOL *pfValuePresent) const
{
    HRESULT hr = S_OK;

    if (bpp.dwPPFlags & PROPPARAM_GETMFHandler)
    {
        PFN_NUMPROPGET pmfGet;

        bpp.GetGetMethodP(this + 1, &pmfGet);

#ifdef WIN16
        hr = (*pmfGet)((CVoid*)(void*)pObject, plNum);
#else
        hr = CALL_METHOD((CVoid*)(void*)pObject,pmfGet,(plNum));
#endif
        if (pfValuePresent)
            *pfValuePresent = TRUE;
    }
    else
    {
        *plNum = bpp.GetAvNumber ( pSubObject, this + 1, cbMember, pfValuePresent );
    }

    RRETURN(hr);
}


//+---------------------------------------------------------------
//
//  Function:   SetNumber
//
//  Synopsis:   Helper for setting number values properties
//
//----------------------------------------------------------------

HRESULT
NUMPROPPARAMS::SetNumber (CBase * pObject, CVoid * pSubObject, long lNum, WORD wFlags) const
{
    HRESULT hr = S_OK;

    if (bpp.dwPPFlags & PROPPARAM_SETMFHandler)
    {
        PFN_NUMPROPSET pmfSet;

        bpp.GetSetMethodP(this + 1, &pmfSet);

#ifdef WIN16
        hr = (*pmfSet)((CVoid*)(void*)pObject, lNum);
#else
        hr = CALL_METHOD((CVoid*)(void*)pObject,pmfSet,(lNum));
#endif
    }
    else
    {
        hr = THR( bpp.SetAvNumber (pSubObject, (DWORD)lNum, this + 1, cbMember, wFlags ) );
    }

    RRETURN(hr);
}


//+---------------------------------------------------------------
//
//  Member:     ValidateNumberProperty, public
//
//  Synopsis:   Helper for testing the validity of numeric properties
//
//----------------------------------------------------------------

HRESULT
NUMPROPPARAMS::ValidateNumberProperty(long *lArg, CBase * pObject ) const
{
    long lMinValue = 0, lMaxValue = LONG_MAX;
    HRESULT hr = S_OK;
    int ids = 0;
    const PROPERTYDESC *pPropDesc = (const PROPERTYDESC *)this -1;

    if (vt == VT_BOOL || vt == VT_BOOL4)
        return S_OK;

    // Check validity of input argument
    if ( bpp.dwPPFlags & PROPPARAM_UNITVALUE )
    {
        CUnitValue uv;
        uv.SetRawValue ( *lArg );
        hr = uv.IsValid( pPropDesc );
        if ( hr )
        {
            if (bpp.dwPPFlags & PROPPARAM_MINOUT)
            {
                // set value to min;
                *lArg = lMin;
                hr = S_OK;
            }
            else
            {
                    //otherwise return error, i.e. set to default
                ids = IDS_ES_ENTER_PROPER_VALUE;
            }
        }
        goto Cleanup;
    }

    if ( ( ( bpp.dwPPFlags & PROPPARAM_ENUM )  && ( bpp.dwPPFlags & PROPPARAM_ANUMBER) )
        || !(bpp.dwPPFlags & PROPPARAM_ENUM) )
    {
        lMinValue = lMin; lMaxValue = lMax;
        if( (*lArg < lMinValue || *lArg > lMaxValue) && 
            *lArg != (long)pPropDesc->GetNotPresentDefault() &&
            *lArg != (long)pPropDesc->GetInvalidDefault() )
        {
            if (lMaxValue != LONG_MAX)
            {
                ids = IDS_ES_ENTER_VALUE_IN_RANGE;
            }
            else if (lMinValue == 0)
            {
                ids = IDS_ES_ENTER_VALUE_GE_ZERO;
            }
            else if (lMinValue == 1)
            {
                ids = IDS_ES_ENTER_VALUE_GT_ZERO;
            }
            else
            {
                ids = IDS_ES_ENTER_VALUE_IN_RANGE;
            }
            hr = E_INVALIDARG;
        }
        else
        {
            // inside of range
            goto Cleanup;
        }
    }

    // We have 3 scenarios to check for :-
    // 1) Just a number validate w min & max
    // 2) Just an emum validated w pEnumDesc
    // 3) A number w. min & max & one or more enum values

    // If we've got a number OR enum type, first check the
    if ( bpp.dwPPFlags & PROPPARAM_ENUM )
    {
        ENUMDESC *pEnumDesc = pPropDesc->GetEnumDescriptor();

        // dwMask represent a mask allowing or rejecting values 0 to 31
        if(*lArg >= 0 && *lArg < 32 )
        {
            if (!((pEnumDesc -> dwMask >> (DWORD)*lArg) & 1))
            {
                if ( !hr )
                {
                    ids = IDS_ES_ENTER_PROPER_VALUE;
                    hr = E_INVALIDARG;
                }
            }
            else
            {
                hr = S_OK;
            }
        }
        else
        {
            // If it's not in the mask, look for the value in the array
            WORD i;

            for ( i = 0 ; i < pEnumDesc -> cEnums ; i++ )
            {
                if ( pEnumDesc -> aenumpairs [ i ].iVal == *lArg )
                {
                    break;
                }
            }
            if ( i == pEnumDesc -> cEnums )
            {
                if ( !hr )
                {
                    ids = IDS_ES_ENTER_PROPER_VALUE;
                    hr = E_INVALIDARG;
                }
            }
            else
            {
                hr = S_OK;
            }
        }
    }

Cleanup:
    if ( hr && ids == IDS_ES_ENTER_PROPER_VALUE )
    {
        RRETURN1 ( pObject->SetErrorInfoPBadValue ( bpp.dispid, ids ), E_INVALIDARG );
    }
    else if ( hr )
    {
        RRETURN1 (pObject->SetErrorInfoPBadValue(
                    bpp.dispid,
                    ids,
                    lMinValue,
                    lMaxValue ),E_INVALIDARG );
    }
    else
        return S_OK;
}


//+---------------------------------------------------------------
//
//  Member:     CBase::GetNumberProperty, public
//
//  Synopsis:   Helper for getting number values properties
//
//----------------------------------------------------------------

HRESULT
NUMPROPPARAMS::GetNumberProperty(void * pv, CBase * pObject, CVoid * pSubObject) const
{
    HRESULT hr;
    long num;

    if(!pv)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    if (!pSubObject)
    {
        pSubObject = pObject;
    }

    hr = GetNumber(pObject, pSubObject, &num);
    if (hr)
        goto Cleanup;

    SetNumberOfType(pv, VARENUM(vt), num);

Cleanup:
    RRETURN(pObject->SetErrorInfoPGet(hr, bpp.dispid));
}


//+---------------------------------------------------------------
//
//  Member:     SetNumberProperty, public
//
//  Synopsis:   Helper for setting number values properties
//
//----------------------------------------------------------------

HRESULT
NUMPROPPARAMS::SetNumberProperty(long lValueNew, CBase * pObject, CVoid * pSubObject, BOOL fValidate /* = TRUE */, WORD wFlags ) const
{
    CBase::CLock    Lock(pObject);
    HRESULT     hr;
    long        lValueOld;
    BOOL        fOldPresent;

        //
        // Get the current value of the property
        //
    if (!pSubObject)
    {
        pSubObject = pObject;
    }

    hr = GetNumber(pObject, pSubObject, &lValueOld, &fOldPresent);
    if (hr)
        goto Cleanup;

        //
        // Validate the old value, just for kicks...
        //

        //
        // If the old and new values are the same, then git out of here
        //

    if (lValueNew == lValueOld)
        goto Cleanup;

        //
        // Make sure the new value is OK
        //

    if ( fValidate )
    {
        // Validate using propdesc encoded parser rules 
        hr = THR(ValidateNumberProperty(&lValueNew, pObject ));
        if (hr)
            return hr;  // Error info set in validate
    }
        //
        // Create the undo thing-a-ma-jig
        //

#ifndef NO_EDIT
    if (pObject->QueryCreateUndo( TRUE, FALSE ))
    {
        CVariant  varOld;
        TCHAR     achValueOld[30];

        // Handle CUnitValues as BSTRs, everything else as plain numbers.

        if ( !fOldPresent )
        {
            V_VT(&varOld) = VT_NULL;
        }

#ifdef WIN16
        else if ( GetPropertyDesc()->pfnHandleProperty ==
             PROPERTYDESC::handleUnitValueproperty )
#elif defined(_MAC)
        else if ( GetPropertyDesc()->pfnHandleProperty ==
             &PROPERTYDESC::HandleUnitValueProperty )
#else
        else if ( GetPropertyDesc()->pfnHandleProperty ==
             PROPERTYDESC::HandleUnitValueProperty )
#endif
        {
            CUnitValue uv;

            uv.SetRawValue( lValueOld );

            if (uv.IsNull())
            {
                V_VT(&varOld) = VT_BSTR;
            }
            else
            {
                hr = uv.FormatBuffer( achValueOld, ARRAY_SIZE( achValueOld ),
                                      GetPropertyDesc() );
                if ( hr )
                    goto Cleanup;

                V_VT(&varOld) = VT_BSTR;
                hr = FormsAllocString(achValueOld, &V_BSTR(&varOld));
                if (hr)
                    goto Cleanup;
            }
        }
        else
        {
            AssertSz( vt == VT_I4 || vt == VT_I2 || vt == VT_BOOL || vt == VT_BOOL4, "Bad vt" );

            V_VT(&varOld) = vt;
            V_I4(&varOld) = lValueOld;
        }

        hr = THR(pObject->CreatePropChangeUndo(
                bpp.dispid,&varOld, NULL));
        if (hr)
            goto Cleanup;
    }
#endif // NO_EDIT

        //
        // Stuff the new value in
        //

    hr = THR(SetNumber(pObject, pSubObject, lValueNew, wFlags));
    if (hr)
        goto Cleanup;

        //
        // Tell everybody about the new value
        //

    hr = THR(pObject->OnPropertyChange(bpp.dispid, bpp.dwFlags));

        //
        // If anybody complained, then revert
        //

// BUGBUG: should we delte the undo thing-a-ma-jig if the OnPropChange fails?

    if (hr)
    {
        IGNORE_HR(SetNumber(pObject, pSubObject, lValueOld, wFlags));
        IGNORE_HR(pObject->OnPropertyChange(bpp.dispid, bpp.dwFlags));
    }

Cleanup:
    RRETURN(pObject->SetErrorInfoPSet(hr, bpp.dispid));
}




HRESULT BUGCALL
PROPERTYDESC::HandleUnitValueProperty (DWORD dwOpCode, void * pv, CBase * pObject, CVoid * pSubObject) const
{
    HRESULT hr = S_OK,hr2;
    const NUMPROPPARAMS *ppp = (NUMPROPPARAMS *)(this + 1);
    const BASICPROPPARAMS *bpp = (BASICPROPPARAMS *)ppp;
    CVariant varDest;
    LONG lNewValue;
    long lTemp;
    BOOL fValidated = FALSE;

    // The code in this fn assumes that the address of the object
    // is the address of the long value
    if (ISSET(dwOpCode))
    {
        CUnitValue uvValue;
        uvValue.SetDefaultValue();
        LPCTSTR pStr = (TCHAR *)pv;

        Assert(!(ISSTREAM(dwOpCode))); // we can't do this yet...

        switch(PROPTYPE(dwOpCode))
        {
        case PROPTYPE_LPWSTR:
FromString:
            if ( pStr == NULL || !pStr[0] )
            {
                lNewValue = GetNotPresentDefault();
            }
            else
            {
                hr = uvValue.FromString ( pStr, this );
                if ( !hr )
                {
                    lNewValue = uvValue.GetRawValue ();
                    if ( !(dwOpCode & HANDLEPROP_DONTVALIDATE ) )
                        hr = uvValue.IsValid( this );
                }

                if ( hr == E_INVALIDARG )
                {
                    // Ignore invalid values when parsing a stylesheet
                    if(IsStyleSheetProperty())
                        goto Cleanup;

                    lNewValue = GetInvalidDefault();
                }
                else if ( hr )
                {
                    goto Cleanup;
                }
            }
            // If we're just sniffing for a good parse - don't set up a default
            if ( hr && ISSAMPLING( dwOpCode ) )
                goto Cleanup;
            fValidated = TRUE;
            pv = &lNewValue;
            break;
                                             
        case PROPTYPE_VARIANT:
            {
                VARIANT *pVar = (VARIANT *)pv;

                switch ( pVar -> vt )
                {
                default:
                    hr = VariantChangeTypeSpecial(&varDest, (VARIANT *)pVar, VT_BSTR);
                    if (hr)
                        goto Cleanup;

                    pVar = &varDest;

                    // Fall thru to VT_BSTR code below

                case VT_BSTR:
                    pStr = pVar -> bstrVal;
                    goto FromString;

                case VT_NULL:
                    lNewValue = 0;
                    break;

                }
                pv = &lNewValue;
            }
            break;

        default:
            Assert(PROPTYPE(dwOpCode) == 0);    // assumed native long
        }

        WORD wFlags = 0;
        if ( dwOpCode & HANDLEPROP_IMPORTANT )
            wFlags |= CAttrValue::AA_Extra_Important;
        if ( dwOpCode & HANDLEPROP_IMPLIED )
            wFlags |= CAttrValue::AA_Extra_Implied;
        switch(OPCODE(dwOpCode))
        {
        case HANDLEPROP_DEFAULT:
            // Set default Value from propdesc
            hr = ppp->SetNumber(pObject, pSubObject,
                (long)ulTagNotPresentDefault, wFlags );
            goto Cleanup;


        case HANDLEPROP_AUTOMATION:
            if ( hr )
                goto Cleanup;

			// The unit value handler can be called directly through automation
			// This is pretty unique - we usualy have a helper fn
            hr = HandleNumProperty(SETPROPTYPE(dwOpCode, PROPTYPE_EMPTY),
				pv, pObject, pSubObject);
            if (hr)
                goto Cleanup;
			break;

        case HANDLEPROP_VALUE:
            if ( !fValidated )
            {
                // We need to preserve an existing error code if there is one
                hr2 = ppp->ValidateNumberProperty((long *)pv, pObject);
                if (hr2)
                {
                    hr = hr2;
                    goto Cleanup;
                }
            }
            hr2 = ppp->SetNumber(pObject, pSubObject, *(long *)pv, wFlags);
            if (hr2)
            {
                hr = hr2;
                goto Cleanup;
            }

            if (dwOpCode & HANDLEPROP_MERGE)
            {
                hr2 = THR(pObject->OnPropertyChange(bpp->dispid, bpp->dwFlags));
                if (hr2)
                {
                    hr = hr2;
                    goto Cleanup;
                }
            }

            break;
        }
        RRETURN1(hr, E_INVALIDARG);
    }
    else
    {
        switch(OPCODE(dwOpCode))
        {
        case HANDLEPROP_AUTOMATION:
            hr = ppp->GetNumberProperty(&lTemp, pObject, pSubObject);
            if ( hr )
                goto Cleanup;
            // Coerce to type
            {
                BSTR *pbstr = (BSTR *)pv;

                switch(PROPTYPE(dwOpCode))
                {
                case PROPTYPE_VARIANT:
                    // pv points to a VARIANT
                    VariantInit ( (VARIANT *)pv );
                    V_VT ( (VARIANT *)pv ) = VT_BSTR;
                    pbstr = &V_BSTR((VARIANT *)pv);
                    // intentional fall-through
                case PROPTYPE_BSTR:
                    {
                        TCHAR cValue [ 30 ];


                        CUnitValue uvThis;
                        uvThis.SetRawValue ( lTemp );

                        if ( uvThis.IsNull() )
                        {
                            // Return an empty BSTR
                            hr = FormsAllocString( g_Zero.ach, pbstr );
                        }
                        else
                        {
                            hr = uvThis.FormatBuffer ( (LPTSTR)cValue, (UINT)ARRAY_SIZE ( cValue ), this );
                            if ( hr )
                                goto Cleanup;

                            hr = THR(FormsAllocString( cValue, pbstr ));
                        }
                    }
                    break;

                    default:
                        *(long *)pv = lTemp;
                        break;
                }
            }
            goto Cleanup;
            break;

        case HANDLEPROP_VALUE:
            {
                CUnitValue uvValue;

                // get raw value
                hr = ppp->GetNumber(pObject, pSubObject, (long *)&uvValue);

                if (PROPTYPE(dwOpCode) == VT_VARIANT)
                {
                    ((VARIANT *)pv)->vt = VT_I4;
                    ((VARIANT *)pv)->lVal = uvValue.GetUnitValue();
                }
                else if (PROPTYPE(dwOpCode) == PROPTYPE_BSTR)
                {
                    TCHAR cValue [ 30 ];

                    hr = uvValue.FormatBuffer ( (LPTSTR)cValue, (UINT)ARRAY_SIZE ( cValue ), this );
                    if ( hr )
                        goto Cleanup;

                    hr = THR( FormsAllocString( cValue, (BSTR *)pv ) );
                }
                else
                {
                    Assert(PROPTYPE(dwOpCode) == 0);
                    *(long *)pv = uvValue.GetUnitValue();
                }
            }
            goto Cleanup;

        case HANDLEPROP_COMPARE:
            hr = ppp->GetNumber(pObject, pSubObject, &lTemp);
            if (hr)
                goto Cleanup;
            // if inherited and not set, return S_OK
            hr = ( lTemp == *(long *)pv ) ? S_OK : S_FALSE;
            RRETURN1 (hr, S_FALSE);
            break;

        case HANDLEPROP_STREAM:
            {
            // Get value into an IStream
            // until the binary persistance we assume text save
            Assert(PROPTYPE(dwOpCode) == PROPTYPE_LPWSTR);

            hr = ppp->GetNumber(pObject, pSubObject, &lNewValue);
            if (hr)
                goto Cleanup;

            CUnitValue uvValue;
            uvValue.SetRawValue ( lNewValue );

            hr = uvValue.Persist ( (IStream *)pv, this );

            goto Cleanup;
            }
            break;

        }
    }
    // Let the basic numproperty handler handle all other cases
    hr = HandleNumProperty ( dwOpCode, pv, pObject, pSubObject );
Cleanup:
    RRETURN1(hr, E_INVALIDARG);
}


HRESULT LookupEnumString ( const NUMPROPPARAMS *ppp, LPCTSTR pStr, long *plNewValue )
{
    HRESULT hr = S_OK;
    PROPERTYDESC *ppdPropDesc = ((PROPERTYDESC *)ppp)-1;
    ENUMDESC *pEnumDesc = ppdPropDesc->GetEnumDescriptor();
        
    // ### v-gsrir - Modified the 3rd parameter to result in a 
    // Bool instead of a DWORD - which was resulting in the parameter
    // being taken as 0 always.
    hr = pEnumDesc->EnumFromString ( pStr, plNewValue,( 
        ppp->bpp.dwPPFlags & PROPPARAM_CASESENSITIVE ) ? TRUE:FALSE);

    if ( hr == E_INVALIDARG )
    {
        if ( ppp->bpp.dwPPFlags & PROPPARAM_ANUMBER )
        {
            // Not one of the enums, is it a number??
            hr = ttol_with_error( pStr, plNewValue);
        }
    }
    RRETURN(hr);
}

// Don't call this function outside of automation - loads oleaut!
HRESULT LookupStringFromEnum( const NUMPROPPARAMS *ppp, BSTR *pbstr, long lValue )
{
    ENUMDESC *pEnumDesc;

    Assert( (ppp->bpp.dwPPFlags&PROPPARAM_ENUM) && "Can't convert a non-enum to an enum string!" );

    if ( ppp->bpp.dwPPFlags & PROPPARAM_ANUMBER )
    {
        pEnumDesc = *(ENUMDESC **)((BYTE *)(ppp+1)+ sizeof(DWORD_PTR));
    }
    else
    {
        pEnumDesc = (ENUMDESC *) ppp->lMax;
    }

    RRETURN1( pEnumDesc->StringFromEnum( lValue, pbstr ), S_FALSE );
}

BOOL 
PROPERTYDESC::IsBOOLProperty ( void ) const
{                        
#ifdef WIN16
    return  ( pfnHandleProperty == &PROPERTYDESC::handleNumproperty &&
        ((NUMPROPPARAMS *)(this + 1))-> vt == VT_BOOL  ) ? TRUE : FALSE;
#else
    return  ( pfnHandleProperty == &PROPERTYDESC::HandleNumProperty &&
        ((NUMPROPPARAMS *)(this + 1))-> vt == VT_BOOL  ) ? TRUE : FALSE;
#endif
}

//+---------------------------------------------------------------
//
//  Member:     PROPERTYDESC::HandleNumProperty, public
//
//  Synopsis:   Helper for getting/setting number value properties
//
//  Arguments:  dwOpCode        -- encodes the incoming type (PROPTYPE_FLAG) in the upper WORD and
//                                 the opcode in the lower WORD (HANDLERPROP_FLAGS)
//                                 PROPTYPE_EMPTY means the 'native' type (long in this case)
//              pv              -- points to the 'media' the value is stored for the get and set
//              pObject         -- object owns the property
//              pSubObject      -- subobject storing the property (could be the main object)
//
//----------------------------------------------------------------

HRESULT
PROPERTYDESC::HandleNumProperty (DWORD dwOpCode, void * pv, CBase * pObject, CVoid * pSubObject) const
{
    HRESULT             hr = S_OK, hr2;
    const NUMPROPPARAMS *ppp = (NUMPROPPARAMS *)(this + 1);
    const BASICPROPPARAMS *bpp = (BASICPROPPARAMS *)ppp;
    VARIANT             varDest;
    LONG                lNewValue, lTemp;
    CStreamWriteBuff    *pStmWrBuff;
    VARIANT             *pVariant;
    VARIANT             varTemp;

    varDest.vt = VT_EMPTY;

    if (ISSET(dwOpCode))
    {
        Assert(!(ISSTREAM(dwOpCode))); // we can't do this yet...
        switch(PROPTYPE(dwOpCode))
        {
        case PROPTYPE_LPWSTR:
            // If the parameter is a BOOL, the presence of the value sets the value
            if ( ppp -> vt == VT_BOOL )
            {
                // For an OLE BOOL -1 == True
                lNewValue = -1;
            }
            else
            {
SetFromString:
                LPTSTR pStr = (TCHAR *)pv;
                hr = S_OK;
                if ( pStr == NULL || !pStr[0] )
                {
                    // Just have Tag=, set to not assigned value
                    lNewValue = GetNotPresentDefault();
                }
                else
                {
                    if ( !(ppp->bpp.dwPPFlags & PROPPARAM_ENUM) )
                    {
                        hr = ttol_with_error( pStr, &lNewValue);
                    }
                    else
                    {
                        // enum string, look it up
                        hr = LookupEnumString ( ppp, pStr, &lNewValue );
                    }
                    if ( hr )
                    {
                        lNewValue = GetInvalidDefault();
                    }
                }
            }
            // If we're just sniffing for a good parse - don't set up a default
            if ( hr && ISSAMPLING( dwOpCode ) )
                goto Cleanup;
            pv = &lNewValue;
            break;

        case PROPTYPE_VARIANT:
            if ( pv == NULL )
            {
                // Just have Tag=, ignore it
                return S_OK;
            }
            VariantInit ( &varDest );
            if ( ppp->bpp.dwPPFlags & PROPPARAM_ENUM )
            {
                hr = VariantChangeTypeSpecial(&varDest, (VARIANT *)pv, VT_BSTR);
                if (hr)
                    goto Cleanup;
                pv = V_BSTR(&varDest);
                // Send it through the String handler
                goto SetFromString;
            }
            else
            {
                hr = VariantChangeTypeSpecial(&varDest, (VARIANT *)pv, VT_I4);
                if (hr)
                    goto Cleanup;
                pv = &V_I4(&varDest);
            }
            break;
        default:
            Assert(PROPTYPE(dwOpCode) == 0);    // assumed native long
        }
        WORD wFlags = 0;
        if ( dwOpCode & HANDLEPROP_IMPORTANT )
            wFlags |= CAttrValue::AA_Extra_Important;
        if ( dwOpCode & HANDLEPROP_IMPLIED )
            wFlags |= CAttrValue::AA_Extra_Implied;
        switch(OPCODE(dwOpCode))
        {
        case HANDLEPROP_VALUE:
            // We need to preserve an existing hr
            hr2 = ppp->ValidateNumberProperty((long *)pv, pObject);
            if (hr2)
            {
                hr = hr2;
                goto Cleanup;
            }
            hr2 = ppp->SetNumber(pObject, pSubObject, *(long *)pv, wFlags);
            if ( hr2 )
            {
                hr = hr2;
                goto Cleanup;
            }

            if (dwOpCode & HANDLEPROP_MERGE)
            {
                hr2 = THR(pObject->OnPropertyChange(bpp->dispid, bpp->dwFlags));
                if (hr2)
                {
                    hr = hr2;
                    goto Cleanup;
                }
            }
            break;
            
        case HANDLEPROP_AUTOMATION:
            if ( hr )
                goto Cleanup;
            hr = ppp->SetNumberProperty(*(long *)pv, pObject, pSubObject,
                dwOpCode & HANDLEPROP_DONTVALIDATE ? FALSE : TRUE, wFlags );
            break;

        case HANDLEPROP_DEFAULT:
            Assert(pv == NULL);
            hr = ppp->SetNumber(pObject, pSubObject,
                (long)ulTagNotPresentDefault, wFlags);
            break;

        default:
            hr = E_FAIL;
            Assert(FALSE && "Invalid operation");
        }
    }
    else
    {
        switch(OPCODE(dwOpCode))
        {
        case HANDLEPROP_AUTOMATION:
            hr = ppp->GetNumberProperty( &lTemp, pObject, pSubObject);
            if ( hr )
                goto Cleanup;
            switch(PROPTYPE(dwOpCode))
            {
                case PROPTYPE_VARIANT:
                    pVariant = (VARIANT *)pv;
                    VariantInit ( pVariant );
                    if ( ppp->bpp.dwPPFlags & PROPPARAM_ENUM )
                    {
                        hr = LookupStringFromEnum( ppp, &V_BSTR(pVariant), lTemp );
                        if ( !hr )
                        {
                            V_VT( pVariant ) = VT_BSTR;
                        }
                        else
                        {
                            if ( hr != E_INVALIDARG)
                                goto Cleanup;

                            hr = S_OK;
                        }
                    }
                    if ( V_VT ( pVariant ) == VT_EMPTY )
                    {
                        V_VT ( pVariant ) = VT_I4;
                        V_I4 ( pVariant ) = lTemp;
                    }
                    break;

                case PROPTYPE_BSTR:
                    *((BSTR *)pv) = NULL;
                    if ( ppp->bpp.dwPPFlags & PROPPARAM_ENUM )
                    {
                        hr = LookupStringFromEnum( ppp, (BSTR *)pv, lTemp );
                    }
                    else
                    {
                        TCHAR szNumber[33];
                        hr = THR( Format(0, szNumber, 32, _T("<0d>"), lTemp ) );
                        if ( hr == S_OK )
                            hr = FormsAllocString( szNumber, (BSTR *)pv );
                    }
                    break;

                default:
                    *(long *)pv = lTemp;
                    break;
            }
            break;

        case HANDLEPROP_STREAM:
            // until the binary persistance we assume text save
            Assert(PROPTYPE(dwOpCode) == PROPTYPE_LPWSTR);
            hr = ppp->GetNumber(pObject, pSubObject, &lNewValue);
            if (hr)
                goto Cleanup;
            pStmWrBuff = (CStreamWriteBuff *)pv;
            // If it's one of the enums, save out the enum string
            if ( ppp->bpp.dwPPFlags & PROPPARAM_ENUM )
            {
                INT i;
                ENUMDESC *pEnumDesc = GetEnumDescriptor();
                // until the binary persistance we assume text save
                for (i = pEnumDesc->cEnums - 1; i >= 0; i--)
                {
                    if (lNewValue == pEnumDesc->aenumpairs[i].iVal)
                    {
                        hr = pStmWrBuff->WriteQuotedText(pEnumDesc->aenumpairs[i].pszName, FALSE);
                        goto Cleanup;
                    }
                }
            }
            // Either don't have an enum array or wasn't one of the enum values
            hr = WriteTextLong(pStmWrBuff, lNewValue);
            break;

        case HANDLEPROP_VALUE:
            hr = ppp->GetNumber(pObject, pSubObject, &lNewValue);
            if ( hr )
                goto Cleanup;
            switch (PROPTYPE(dwOpCode))
            {
            case VT_VARIANT:
                {
                ENUMDESC *pEnumDesc = GetEnumDescriptor();
                if ( pEnumDesc )
                {
                    hr = pEnumDesc->StringFromEnum ( lNewValue, &V_BSTR((VARIANT *)pv) );
                    if ( !hr )
                    {
                        ((VARIANT *)pv)->vt = VT_BSTR;
                        goto Cleanup;
                    }
                }

                // if the Numeric prop is boolean...
                if (GetNumPropParams()->vt == VT_BOOL)
                {
                    ((VARIANT *)pv)->boolVal = (VARIANT_BOOL)lNewValue;
                    ((VARIANT *)pv)->vt = VT_BOOL;
                    break;
                }

                // Either mixed enum/integer or plain integer return I4
                ((VARIANT *)pv)->lVal = lNewValue;
                ((VARIANT *)pv)->vt = VT_I4;
                }
                break;

            case VT_BSTR:
                {
                    ENUMDESC *pEnumDesc = GetEnumDescriptor();
                    if ( pEnumDesc )
                    {
                        hr = pEnumDesc->StringFromEnum ( lNewValue, (BSTR *)pv );
                        if ( !hr )
                        {
                            goto Cleanup;
                        }
                    }
                    // Either mixed enum/integer or plain integer return BSTR
                    varTemp.lVal = lNewValue;
                    varTemp.vt = VT_I4;   
                    hr = VariantChangeTypeSpecial ( &varTemp, &varTemp, VT_BSTR );
                    *(BSTR *)pv = V_BSTR(&varTemp);
                }
                break;

            default:
                Assert(PROPTYPE(dwOpCode) == 0);
                *(long *)pv = lNewValue;
                break;
            }
            break;

        case HANDLEPROP_COMPARE:
            hr = ppp->GetNumber(pObject, pSubObject, &lNewValue);
            if (hr)
                goto Cleanup;
            hr = ( lNewValue == *(long *)pv ) ? S_OK : S_FALSE;
            RRETURN1(hr,S_FALSE);
            break;

        default:
            hr = E_FAIL;
            Assert(FALSE && "Invalid operation");
        }
    }

Cleanup:

    if (varDest.vt != VT_EMPTY)
    {
        VariantClear(&varDest);
    }

    RRETURN1(hr, E_INVALIDARG);
}


//+---------------------------------------------------------------
//
//  Member:     PROPERTYDESC::HandleStringProperty, public
//
//  Synopsis:   Helper for getting/setting string value properties
//
//  Arguments:  dwOpCode        -- encodes the incoming type (PROPTYPE_FLAG) in the upper WORD and
//                                 the opcode in the lower WORD (HANDLERPROP_FLAGS)
//                                 PROPTYPE_EMPTY means the 'native' type (CStr in this case)
//              pv              -- points to the 'media' the value is stored for the get and set
//              pObject         -- object owns the property
//              pSubObject      -- subobject storing the property (could be the main object)
//
//----------------------------------------------------------------


HRESULT
PROPERTYDESC::HandleStringProperty(DWORD dwOpCode, void * pv, CBase * pObject, CVoid * pSubObject) const
{
    HRESULT     hr = S_OK;
    BASICPROPPARAMS * ppp = (BASICPROPPARAMS *)(this + 1);
    VARIANT     varDest;

    varDest.vt = VT_EMPTY;
    if (ISSET(dwOpCode))
    {
        Assert(!(ISSTREAM(dwOpCode))); // we can't do this yet...
        switch(PROPTYPE(dwOpCode))
        {
        case PROPTYPE_LPWSTR:
            break;
        case PROPTYPE_VARIANT:
            if (V_VT((VARIANT *)pv) == VT_BSTR)
            {
                pv = (void *)V_BSTR((VARIANT *)pv);
            }
            else
            {
                hr = VariantChangeTypeSpecial(&varDest, (VARIANT *)pv,  VT_BSTR);
                if (hr)
                    goto Cleanup;
                pv = V_BSTR(&varDest);
            }
        default:
            Assert(PROPTYPE(dwOpCode) == 0);    // assumed native long
        }
        WORD wFlags = 0;
        if ( dwOpCode & HANDLEPROP_IMPORTANT )
            wFlags |= CAttrValue::AA_Extra_Important;
        if ( dwOpCode & HANDLEPROP_IMPLIED )
            wFlags |= CAttrValue::AA_Extra_Implied;
        switch(OPCODE(dwOpCode))
        {
        case HANDLEPROP_DEFAULT:
            Assert(pv == NULL);
            pv = (void *)ulTagNotPresentDefault;

            if (!pv)
                goto Cleanup;       // zero string

            // fall thru
        case HANDLEPROP_VALUE:
            hr = ppp->SetString(pSubObject, (TCHAR *)pv, wFlags);
            if (dwOpCode & HANDLEPROP_MERGE)
            {
                hr = THR(pObject->OnPropertyChange(ppp->dispid, ppp->dwFlags));
                if (hr)
                    goto Cleanup;
            }
            break;
        case HANDLEPROP_AUTOMATION:
            hr = ppp->SetStringProperty( (TCHAR *)pv, pObject, pSubObject, wFlags );
            break;
        default:
            hr = E_FAIL;
            Assert(FALSE && "Invalid operation");
        }
    }
    else
    {
        switch(OPCODE(dwOpCode))
        {
        case HANDLEPROP_AUTOMATION:
            {
                BSTR bstr;
                hr = ppp->GetStringProperty( &bstr, pObject, pSubObject );
                if(hr)
                    goto Cleanup;
                switch(PROPTYPE(dwOpCode))
                {
                case PROPTYPE_VARIANT:
                    V_VT((VARIANTARG *)pv) = VT_BSTR;
                    V_BSTR((VARIANTARG *)pv) = bstr;
                    break;
                case PROPTYPE_BSTR:
                    *(BSTR *)pv = bstr;
                    break;
                default:
                    Assert( "Wrong type for property!" );
                    hr = E_INVALIDARG;
                    goto Cleanup;
                }

            }
            break;

        case HANDLEPROP_STREAM:
            {
                CStr cstr;

                // until the binary persistance we assume text save
                Assert(PROPTYPE(dwOpCode) == PROPTYPE_LPWSTR);
                hr = ppp->GetString(pSubObject, &cstr);
                if (hr)
                    goto Cleanup;

                hr = WriteTextCStr((CStreamWriteBuff *)pv, &cstr, FALSE, ( ppp->dwPPFlags & PROPPARAM_STYLESHEET_PROPERTY ) );
            }
            break;
        case HANDLEPROP_VALUE:
            if (PROPTYPE(dwOpCode) == PROPTYPE_VARIANT)
            {
                CStr cstr;

                // BUGBUG istvanc this is bad, we allocate the string twice
                // we need a new API which allocates the BSTR directly
                hr = ppp->GetString(pSubObject, &cstr);
                if (hr)
                    goto Cleanup;

                hr = cstr.AllocBSTR(&((VARIANT *)pv)->bstrVal);
                if (hr)
                    goto Cleanup;

                ((VARIANT *)pv)->vt = VT_BSTR;
            }
            else if (PROPTYPE(dwOpCode) == PROPTYPE_BSTR)
            {
                CStr cstr;

                // BUGBUG istvanc this is bad, we allocate the string twice
                // we need a new API which allocates the BSTR directly
                hr = ppp->GetString(pSubObject, &cstr);
                if (hr)
                    goto Cleanup;

                hr = cstr.AllocBSTR((BSTR *)pv);
            }
            else
            {
                Assert(PROPTYPE(dwOpCode) == 0);
                hr = ppp->GetString(pSubObject, (CStr *)pv);
            }
            break;
        case HANDLEPROP_COMPARE:
            {
                CStr cstr;
                hr = ppp->GetString ( pSubObject, &cstr );
                if ( hr )
                    goto Cleanup;
                LPTSTR lpThisString = (LPTSTR) cstr;
                if (lpThisString == NULL || *(TCHAR**)pv == NULL)
                {
                    hr = ( lpThisString == NULL &&  *(TCHAR**)pv == NULL ) ? S_OK : S_FALSE;
                }
                else
                {
                    hr = _tcsicmp(lpThisString, *(TCHAR**)pv) ? S_FALSE : S_OK;
                }
            }
            RRETURN1(hr,S_FALSE);
            break;

        default:
            hr = E_FAIL;
            Assert(FALSE && "Invalid operation");
        }
    }

Cleanup:

    if (varDest.vt != VT_EMPTY)
    {
        VariantClear(&varDest);
    }
    RRETURN1(hr, E_INVALIDARG);
}


//+---------------------------------------------------------------
//
//  Member:     PROPERTYDESC::HandleEnumProperty, public
//
//  Synopsis:   Helper for getting/setting enum value properties
//
//  Arguments:  dwOpCode        -- encodes the incoming type (PROPTYPE_FLAG) in the upper WORD and
//                                 the opcode in the lower WORD (HANDLERPROP_FLAGS)
//                                 PROPTYPE_EMPTY means the 'native' type (long in this case)
//              pv              -- points to the 'media' the value is stored for the get and set
//              pObject         -- object owns the property
//              pSubObject      -- subobject storing the property (could be the main object)
//
//----------------------------------------------------------------

HRESULT
PROPERTYDESC::HandleEnumProperty (DWORD dwOpCode, void * pv, CBase * pObject, CVoid * pSubObject) const
{
    HRESULT     hr;
    NUMPROPPARAMS * ppp = (NUMPROPPARAMS *)(this + 1);
    long lNewValue;

    if (ISSET(dwOpCode))
    {
        Assert(!(ISSTREAM(dwOpCode))); // we can't do this yet...
        Assert(ppp->bpp.dwPPFlags & PROPPARAM_ENUM);
    }
    else if (OPCODE(dwOpCode) == HANDLEPROP_STREAM)
    {
        RRETURN1 ( HandleNumProperty ( dwOpCode,
            pv, pObject, pSubObject ), E_INVALIDARG );
    }
    else if (OPCODE(dwOpCode) == HANDLEPROP_COMPARE)
    {
        hr = ppp->GetNumber(pObject, pSubObject, &lNewValue);
        if (hr)
            goto Cleanup;
        // if inherited and not set, return S_OK
        hr = ( lNewValue == *(long *)pv ) ? S_OK : S_FALSE;
        RRETURN1(hr, S_FALSE);
    }

    hr = HandleNumProperty(dwOpCode, pv, pObject, pSubObject);

Cleanup:

    RRETURN1(hr, E_INVALIDARG);
}


//+---------------------------------------------------------------
//
//  Member:     PROPERTYDESC::HandleColorProperty, public
//
//  Synopsis:   Helper for getting/setting enum value properties
//
//  Arguments:  dwOpCode        -- encodes the incoming type (PROPTYPE_FLAG) in the upper WORD and
//                                 the opcode in the lower WORD (HANDLERPROP_FLAGS)
//                                 PROPTYPE_EMPTY means the 'native' type (OLE_COLOR in this case)
//              pv              -- points to the 'media' the value is stored for the get and set
//              pObject         -- object owns the property
//              pSubObject      -- subobject storing the property (could be the main object)
//
//----------------------------------------------------------------

HRESULT
PROPERTYDESC::HandleColorProperty (DWORD dwOpCode, void * pv, CBase * pObject, CVoid * pSubObject) const
{
    HRESULT     hr = S_OK,hr2;
    BASICPROPPARAMS * ppp = (BASICPROPPARAMS *)(this + 1);
    VARIANT     varDest;
    DWORD       dwNewValue;
    CColorValue cvValue;
    LPTSTR pStr;

    // just to be a little paranoid
    Assert(sizeof(OLE_COLOR) == 4);
    if (ISSET(dwOpCode))
    {
        Assert(!(ISSTREAM(dwOpCode))); // we can't do this yet...
        VariantInit(&varDest);
        switch(PROPTYPE(dwOpCode))
        {
        case PROPTYPE_LPWSTR:
            V_VT(&varDest) = VT_BSTR;
            V_BSTR(&varDest) = (BSTR)pv;
            break;
        case PROPTYPE_VARIANT:
            if ( pv == NULL )
            {
                // Just have Tag=, ignore it
                return S_OK;
            }
            break;
        default:
            Assert(PROPTYPE(dwOpCode) == 0);    // assumed native long
        }
        WORD wFlags = 0;
        if ( dwOpCode & HANDLEPROP_IMPORTANT )
            wFlags |= CAttrValue::AA_Extra_Important;
        if ( dwOpCode & HANDLEPROP_IMPLIED )
            wFlags |= CAttrValue::AA_Extra_Implied;
        switch(OPCODE(dwOpCode))
        {
        case HANDLEPROP_VALUE:
            pStr = (LPTSTR)pv;
            if ( pStr == NULL || !pStr[0] )
            {
                if ( IsNotPresentAsDefaultSet() || !UseNotAssignedValue() )
                    cvValue = *(CColorValue *)&ulTagNotPresentDefault;
                else
                    cvValue = *(CColorValue *)&ulTagNotAssignedDefault;
            }
            else
            {
                hr = cvValue.FromString( pStr, ( ppp->dwPPFlags & PROPPARAM_STYLESHEET_PROPERTY ) );
                if (hr || S_OK != cvValue.IsValid())
                {
                    if ( IsInvalidAsNoAssignSet() && UseNotAssignedValue() )
                        cvValue = *(CColorValue *)&ulTagNotAssignedDefault;
                    else
                        cvValue = *(CColorValue *)&ulTagNotPresentDefault;
                }
           }
            // If we're just sniffing for a good parse - don't set up a default
            if ( hr && ISSAMPLING( dwOpCode ) )
                goto Cleanup;

            // if asp string, we need to store the original string itself as an unknown attr
            // skip leading white space.
            while (pStr && *pStr && _istspace(*pStr))
                pStr++;

            if (pStr && (*pStr == _T('<')) && (*(pStr+1) == _T('%')))
                hr = E_INVALIDARG;
 
            // We need to preserve the hr if there is one
            hr2 = ppp->SetColor(pSubObject, cvValue.GetRawValue(), wFlags );
            if (hr2)
            {
                hr = hr2;
                goto Cleanup;
            }
            
            if (dwOpCode & HANDLEPROP_MERGE)
            {
                hr2 = THR(pObject->OnPropertyChange(ppp->dispid, ppp->dwFlags));
                if (hr2)
                {
                    hr = hr2;
                    goto Cleanup;
                }
            }
            break;

        case HANDLEPROP_AUTOMATION:
            if (PROPTYPE(dwOpCode)==PROPTYPE_LPWSTR)
                hr = ppp->SetColorProperty( varDest, pObject, pSubObject, wFlags);
            else
                hr = ppp->SetColorProperty( *(VARIANT *)pv, pObject, pSubObject, wFlags);
            break;

        case HANDLEPROP_DEFAULT:
            Assert(pv == NULL);
            // CColorValue is initialized to VALUE_UNDEF.
            hr = ppp->SetColor(pSubObject, *(DWORD *)&ulTagNotPresentDefault, wFlags);
            break;
        default:
            hr = E_FAIL;
            Assert(FALSE && "Invalid operation");
        }
    }
    else
    {
        switch(OPCODE(dwOpCode))
        {
        case HANDLEPROP_AUTOMATION:
            {
                // this code path results from OM access. OM color's are 
                // always returned in Pound6 format.
                // make sure there is a subObject

                if (!pSubObject)
                    pSubObject = pObject;

                // get the dw Color value
                V_VT(&varDest) = VT_BSTR;
		V_BSTR(&varDest) = NULL;
                ppp->GetColor(pSubObject, &(V_BSTR(&varDest)), !(ppp->dwPPFlags & PROPPARAM_STYLESHEET_PROPERTY) );

                // Coerce to type to return
                switch(PROPTYPE(dwOpCode))
                {
                    case PROPTYPE_VARIANT:
                        VariantInit ( (VARIANT *)pv );
                        V_VT ( (VARIANT *)pv ) = VT_BSTR;
                        V_BSTR((VARIANT *)pv) = V_BSTR(&varDest);
                        break;
                    default:
                        *(BSTR *)pv = V_BSTR(&varDest);
                }
                hr = S_OK;
            }
            break;
        case HANDLEPROP_STREAM:
            // until the binary persistance we assume text save
            Assert(PROPTYPE(dwOpCode) == PROPTYPE_LPWSTR);
            hr = ppp->GetColor(pSubObject, &dwNewValue);
            if (hr)
                goto Cleanup;
            cvValue = dwNewValue;

            hr = cvValue.Persist( (IStream *)pv, this );
            break;
        case HANDLEPROP_VALUE:
            if (PROPTYPE(dwOpCode) == PROPTYPE_VARIANT)
            {
                ((VARIANT *)pv)->vt = VT_UI4;
                hr = ppp->GetColor(pSubObject, &((VARIANT *)pv)->ulVal);
            }
            else
            if (PROPTYPE(dwOpCode) == PROPTYPE_BSTR)
            {
                hr = ppp->GetColorProperty(&varDest, pObject, pSubObject);
                if (hr)
                    goto Cleanup;
                *(BSTR *)pv = V_BSTR(&varDest);
            }
            else
            {
                Assert(PROPTYPE(dwOpCode) == 0);
                hr = ppp->GetColor(pSubObject, (DWORD *)pv);
            }
            break;
        case HANDLEPROP_COMPARE:
            hr = ppp->GetColor(pSubObject, &dwNewValue);
            if (hr)
                goto Cleanup;
            // if inherited and not set, return S_OK
            hr = dwNewValue == *(DWORD *)pv ? S_OK : S_FALSE;
            RRETURN1(hr,S_FALSE);
            break;

        default:
            hr = E_FAIL;
            Assert(FALSE && "Invalid operation");
            break;
        }
    }

Cleanup:

    RRETURN1(hr, E_INVALIDARG);
}

//+---------------------------------------------------------------------------
//  Member: HandleUrlProperty
//
//  Synopsis : Handler for Getting/setting Url typed properties
//              pretty much, the only thing this needs to do is strip out CR/LF;s and 
//                  call the HandleStringProperty
//
//  Arguments:  dwOpCode        -- encodes the incoming type (PROPTYPE_FLAG) in the upper WORD and
//                                 the opcode in the lower WORD (HANDLERPROP_FLAGS)
//                                 PROPTYPE_EMPTY means the 'native' type (OLE_COLOR in this case)
//              pv              -- points to the 'media' the value is stored for the get and set
//              pObject         -- object owns the property
//              pSubObject      -- subobject storing the property (could be the main object)
//
//+---------------------------------------------------------------------------
HRESULT
PROPERTYDESC::HandleUrlProperty (DWORD dwOpCode, void * pv, CBase * pObject, CVoid * pSubObject) const
{
    HRESULT hr = S_OK;
    TCHAR   *pstrNoCRLF=NULL;
    VARIANT     varDest;

    varDest.vt = VT_EMPTY;
    if ISSET(dwOpCode)
    {
        // First handle the storage type
        Assert(!(ISSTREAM(dwOpCode))); // we can't do this yet...
        switch(PROPTYPE(dwOpCode))
        {
        case PROPTYPE_LPWSTR:
            break;
        case PROPTYPE_VARIANT:
            if (V_VT((VARIANT *)pv) == VT_BSTR)
            {
                pv = (void *)V_BSTR((VARIANT *)pv);
            }
            else
            {
                hr = VariantChangeTypeSpecial(&varDest, (VARIANT *)pv,  VT_BSTR);
                if (hr)
                    goto Cleanup;
                pv = V_BSTR(&varDest);
            }
        default:
            Assert(PROPTYPE(dwOpCode) == 0);    // assumed native long
        }

        //then handle the operation
        switch(OPCODE(dwOpCode))
        {
            case HANDLEPROP_VALUE:
            case HANDLEPROP_DEFAULT:
                hr =THR(StripCRLF((TCHAR *)pv, &pstrNoCRLF));
                if (hr)
                    goto Cleanup;

                hr = THR(HandleStringProperty( dwOpCode, (void *)pstrNoCRLF, pObject, pSubObject));
    
                if (pstrNoCRLF)
                    MemFree(pstrNoCRLF);
                goto Cleanup;
                break;
        }
    }
    else
    {   // BUGBUG: This code should not be necessary - it forces us to always quote URLs when
        // persisting.  It is here to circumvent an Athena bug (IEv4.1 RAID #20953) - CWilso
        if(ISSTREAM(dwOpCode))
        {   // Have to make sure we quote URLs when persisting.
            CStr cstr;
            BASICPROPPARAMS * ppp = (BASICPROPPARAMS *)(this + 1);

            // until the binary persistance we assume text save
            Assert(PROPTYPE(dwOpCode) == PROPTYPE_LPWSTR);
            hr = ppp->GetString(pSubObject, &cstr);
            if (hr)
                goto Cleanup;

            hr = WriteTextCStr((CStreamWriteBuff *)pv, &cstr, TRUE, FALSE );
            goto Cleanup;
        }
    }

    hr = THR(HandleStringProperty( dwOpCode, pv, pObject, pSubObject));

Cleanup:
    if (varDest.vt != VT_EMPTY)
    {
        VariantClear(&varDest);
    }
    if ( OPCODE(dwOpCode)== HANDLEPROP_COMPARE )
        RRETURN1( hr, S_FALSE );
    else
        RRETURN1( hr, E_INVALIDARG );
}


CUnitValue::TypeDesc CUnitValue::TypeNames[] =
{
    { _T("null"), UNIT_NULLVALUE, 0,0 }, // Never used
    { _T("pt"), UNIT_POINT, +3, 1000 },
    { _T("pc"), UNIT_PICA, +2, 100 },
    { _T("in"), UNIT_INCH, +3, 1000 },
    { _T("cm"), UNIT_CM, +3, 1000 },
    { _T("mm"), UNIT_MM, +2, 100 }, // Chosen to match HIMETRIC
    { _T("em"), UNIT_EM, +2, 100 },
    { _T("en"), UNIT_EN, +2, 100 },
    { _T("ex"), UNIT_EX, +2, 100 },
    { _T("px"), UNIT_PIXELS, 0, 1 },
    { _T("%"), UNIT_PERCENT, +2, 100 },
    { _T("*"), UNIT_TIMESRELATIVE, +2, 100 },
    { _T("float"), UNIT_FLOAT, +4, 10000 },
};

#define LOCAL_BUF_COUNT   (pdlLength + 1)
HRESULT
CUnitValue::NumberFromString ( LPCTSTR pStr, const PROPERTYDESC *pPropDesc )
{
    BOOL fIsSigned = FALSE;
    long lNewValue = 0;
    LPCTSTR pStartPoint = NULL;
    UNITVALUETYPE uvt = UNIT_INTEGER;
    WORD i,j,k;
    HRESULT hr = S_OK;
    TCHAR tcValue [ LOCAL_BUF_COUNT ];  
    WORD wShift;
    NUMPROPPARAMS *ppp = (NUMPROPPARAMS *)(pPropDesc + 1);
    long lExponent = 0;
    BOOL fNegativeExponent = FALSE;

    enum ParseState
    {
        Starting,
        AfterPoint,
        AfterE,
        InExponent,
        InTypeSpecifier,
	ParseState_Last_enum
    } State = Starting;

    UINT uNumChars =0;
    UINT uPrefixDigits = 0;
    BOOL fSeenDigit = FALSE;

    // Find the unit specifier
    for ( i = 0 ; uNumChars < LOCAL_BUF_COUNT ; i++ )
    {
        switch ( State )
        {
        case Starting:
            if ( i == 0 && ( pStr [ i ] == _T('-') || pStr [ i ] == _T('+')  ) )
            {
                tcValue [ uNumChars++ ] = pStr [ i ]; uPrefixDigits++;
                fIsSigned = TRUE;
            }
            else if ( !_istdigit ( pStr [ i ] ) )
            {
                if ( pStr [ i ] == _T('.') )
                {
                    State = AfterPoint;
                }
                else if ( pStr[ i ] == _T('\0') || pStr[ i ] == _T('\"')  ||pStr[ i ] == _T('\'') )
                {
                    goto Convert;
                }
                else
                {
                    // such up white space, and treat whatevers left as the type
                    while ( _istspace ( pStr [ i ] ) )
                        i++;

                    pStartPoint = pStr + i;
                    State = ( pStr [ i ] == _T('e') || pStr [ i ] == _T('E') ) ? AfterE : InTypeSpecifier;
                }
            }
            else
            {
                fSeenDigit = TRUE;
                tcValue [ uNumChars++ ] = pStr [ i ]; uPrefixDigits++;
            }
            break;


        case AfterPoint:
            if ( !_istdigit ( pStr [ i ] ) )
            {
                if ( pStr [ i ] == _T('\0') )
                {
                    goto Convert;
                }
                else if ( !_istspace ( pStr [ i ] ) )
                {
                    pStartPoint = pStr + i;
                    State = ( pStr [ i ] == _T('e') || pStr [ i ] == _T('E') ) ? AfterE : InTypeSpecifier;
                }
            }
            else
            {
                fSeenDigit = TRUE;
                tcValue [ uNumChars++ ] = pStr [ i ];
            }
            break;

        case AfterE:
            if ( pStr [ i ] == _T('-') || pStr [ i ] == _T('+')  ||
                 _istdigit ( pStr [ i ] ) )
            {   // Looks like scientific notation to me.
                if ( _istdigit ( pStr [ i ] ) )
                    lExponent = pStr [ i ] - _T('0');
                else if ( pStr [ i ] == _T('-') )
                    fNegativeExponent = TRUE;
                State = InExponent;
                break;
            }
            // Otherwise, this must just be a regular old "em" or "en" type specifier-
            // Set the state and let's drop back into this switch with the same char...
            State = InTypeSpecifier;
            i--;
            break;

        case InExponent:
            if ( _istdigit ( pStr [ i ] ) )
            {
                lExponent *= 10;
                lExponent += pStr [ i ] - _T('0');
                break;
            }
            while ( _istspace ( pStr [ i ] ) )
                i++;
            if ( pStr [ i ] == _T('\0') )
                goto Convert;
            State = InTypeSpecifier;    // otherwise, fall through to type handler
            pStartPoint = pStr + i;

        case InTypeSpecifier:
            if ( _istspace ( pStr [ i ] ) )
            {
                if ( pPropDesc->IsStyleSheetProperty() )
                {
                    while ( _istspace ( pStr [ i ] ) )
                        i++;
                    if ( pStr [ i ] != _T('\0') )
                    {
                        hr = E_INVALIDARG;
                        goto Cleanup;
                    }
                }
                goto CompareTypeNames;
            }
            if ( pStr [ i ] == _T('\0') )
            {
CompareTypeNames:
                for ( j = 0 ; j < ARRAY_SIZE ( TypeNames ) ; j++ )
                {
                    if ( TypeNames[ j ].pszName )
                    {
                        int iLen = _tcslen( TypeNames[ j ].pszName );

                        if ( _tcsnipre( TypeNames [ j ].pszName, iLen, pStartPoint, -1 ) )
                        {
                            if ( pPropDesc->IsStyleSheetProperty() )
                            {
                                if ( pStartPoint[ iLen ] && !_istspace( pStartPoint[ iLen ] ) )
                                    continue;
                            }
                            break;
                        }
                    }
                }
                if ( j == ARRAY_SIZE ( TypeNames ) )
                {
                    if ( ( ppp -> bpp.dwPPFlags & PP_UV_UNITFONT ) == PP_UV_UNITFONT )
                    {
                        hr = E_INVALIDARG;
                        goto Cleanup;
                    }
                    // Ignore invalid unit specifier
                    goto Convert;
                }
                uvt = TypeNames [ j ].uvt;
                goto Convert;
            }
            break;
        }
    }

Convert:

    if ( !fSeenDigit && uvt != UNIT_TIMESRELATIVE )
    {
        if ( ppp -> bpp.dwPPFlags & PROPPARAM_ENUM )
        {
            long lEnum;
            hr = LookupEnumString ( ppp, (LPTSTR)pStr, &lEnum );
            if ( hr == S_OK )
            {
                SetValue ( lEnum, UNIT_ENUM );
                goto Cleanup;
            }
        }

        hr = E_INVALIDARG;
        goto Cleanup;
    }

    if ( fIsSigned && uvt == UNIT_INTEGER && ( ( ppp->bpp.dwPPFlags & PP_UV_UNITFONT ) != PP_UV_UNITFONT ) )
    {
        uvt = UNIT_RELATIVE;
    }


    // If the validation for this attribute does not require that we distinguish
    // between integer and relative, just store integer

    // Currently the only legal relative type is a Font size, if this changes either
    // add a separte PROPPARAM_ or do something else
    if ( uvt == UNIT_RELATIVE && !(ppp -> bpp.dwPPFlags & PROPPARAM_FONTSIZE ))
    {
        uvt = UNIT_INTEGER;
    }

    // If a unit was supplied that is not valid for this propdesc,
    // drop back to the default (pixels if no notpresent and notassigned defaults)
    if ( ( IsScalerUnit ( uvt ) && !(ppp -> bpp.dwPPFlags & PROPPARAM_LENGTH ) ) ||
        ( uvt == UNIT_PERCENT && !(ppp -> bpp.dwPPFlags & PROPPARAM_PERCENTAGE ) ) ||
        ( uvt == UNIT_TIMESRELATIVE && !(ppp -> bpp.dwPPFlags & PROPPARAM_TIMESRELATIVE ) ) ||
        ( ( uvt == UNIT_INTEGER ) && !(ppp -> bpp.dwPPFlags & PROPPARAM_FONTSIZE ) ) )
    {   // If no units where specified and a default unit is specified, use it
        if ( ( uvt != UNIT_INTEGER ) && pPropDesc->IsStyleSheetProperty() ) // Stylesheet unit values only:
        {   // We'll default unadorned numbers to "px", but we won't change "100%" to "100px" if percent is not allowed.
            hr = E_INVALIDARG;
            goto Cleanup;
        }
        
        CUnitValue uv;
        uv.SetRawValue ( (long)pPropDesc -> ulTagNotPresentDefault );
        uvt = uv.GetUnitType();
        if ( uvt == UNIT_NULLVALUE )
        {
            uv.SetRawValue ( (long)pPropDesc -> ulTagNotAssignedDefault );
            uvt = uv.GetUnitType();
            if ( uvt == UNIT_NULLVALUE )
                uvt = UNIT_PIXELS;
        }
    }

    // Check the various types, don't do the shift for the various types
    switch ( uvt )
    {
    case UNIT_INTEGER:
    case UNIT_RELATIVE:
    case UNIT_ENUM:
        wShift = 0;
        break;

    default:
        wShift = TypeNames [ uvt ].wShiftOffset;
        break;
    }

    if ( lExponent && !fNegativeExponent && ( ( uPrefixDigits + wShift ) < uNumChars ) )
    {
        long lAdditionalShift = uNumChars - ( uPrefixDigits + wShift );
        if ( lAdditionalShift > lExponent )
            lAdditionalShift = lExponent;
        wShift += (WORD)lAdditionalShift;
        lExponent -= lAdditionalShift;
    }
    // uPrefixDigits tells us how may characters there are before the point;
    // Assume we're always shifting to the right of the point
    k = (uPrefixDigits + wShift < LOCAL_BUF_COUNT) ? uPrefixDigits + wShift : LOCAL_BUF_COUNT - 1;
    tcValue [ k ] = _T('\0');
    for ( j = (WORD)uNumChars; j < k ; j++ )
    {
        tcValue [ j ] = _T('0');
    }

    // Skip leading zeros, they confuse StringToLong
    for ( pStartPoint = tcValue ; *pStartPoint == _T('0') && *(pStartPoint+1) != _T('\0') ; pStartPoint++ );

    if (*pStartPoint)
    {
        hr = ttol_with_error( pStartPoint, &lNewValue);
        // returns LONG_MIN or LONG_MIN on overflow
        if (hr)
        {
            hr = E_INVALIDARG;
            goto Cleanup;
        }
    }
    else if (uvt != UNIT_TIMESRELATIVE)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    if ((uvt == UNIT_TIMESRELATIVE) && (lNewValue == 0))
    {
        lNewValue = 1 * TypeNames[UNIT_TIMESRELATIVE].wScaleMult;
    }

    if ( fNegativeExponent )
    {
        while ( lNewValue && lExponent-- > 0 )
            lNewValue /= 10;
    }
    else
    {
        while ( lExponent-- > 0 )
        {
            if ( lNewValue > LONG_MAX/10 )
            {
                lNewValue = LONG_MAX;
                break;
            }
            lNewValue *= 10;
        }
    }

    SetValue ( lNewValue, uvt );

Cleanup:
    RRETURN1 ( hr, E_INVALIDARG );
}

long 
CUnitValue::SetValue ( long lVal, UNITVALUETYPE uvt ) 
{
    if ( lVal > (LONG_MAX>>NUM_TYPEBITS ))
        lVal = LONG_MAX>>NUM_TYPEBITS;
    else if ( lVal < (LONG_MIN>>NUM_TYPEBITS) )
        lVal = LONG_MIN>>NUM_TYPEBITS;
    return _lValue = (lVal << NUM_TYPEBITS) | uvt; 
}


HRESULT
CUnitValue::Persist (
    IStream *pStream,
    const PROPERTYDESC *pPropDesc ) const
{
    TCHAR   cBuffer [ 30 ];
    int     iBufSize = ARRAY_SIZE ( cBuffer );
    LPTSTR  pBuffer = (LPTSTR)&cBuffer;
    HRESULT hr;
    UNITVALUETYPE   uvt = GetUnitType();

    if (uvt==UNIT_PERCENT )
    {
        *pBuffer++ = _T('\"');
        iBufSize--;
    }

    hr = FormatBuffer ( pBuffer, iBufSize, pPropDesc );
    if ( hr )
        goto Cleanup;

    // and finally the close quotes
    if (uvt==UNIT_PERCENT )
        _tcscat(cBuffer, _T("\""));

    hr = WriteText ( pStream, cBuffer );

Cleanup:
    RRETURN ( hr );
}


// We usually do not append default unit types to the string we return. Setting fAlwaysAppendUnit
//   to true forces the unit to be always appended to the number
HRESULT
CUnitValue::FormatBuffer ( LPTSTR szBuffer, UINT uMaxLen, 
                        const PROPERTYDESC *pPropDesc, BOOL fAlwaysAppendUnit /*= FALSE */) const
{
    HRESULT         hr = S_OK;
    UNITVALUETYPE   uvt = GetUnitType();
    long            lUnitValue = GetUnitValue();
    TCHAR           ach[20 ];
    int             nLen,i,nOutLen = 0;
    BOOL            fAppend = TRUE;

    switch ( uvt )
    {
    case UNIT_ENUM:
        {
            ENUMDESC *pEnumDesc;
            LPCTSTR pcszEnum;

            Assert( ( ((NUMPROPPARAMS *)(pPropDesc + 1))->bpp.dwPPFlags & PROPPARAM_ENUM ) && "Not an enum property!" );

            hr = E_INVALIDARG;
            pEnumDesc = pPropDesc->GetEnumDescriptor();
            if ( pEnumDesc )
            {
                pcszEnum = pEnumDesc->StringPtrFromEnum( lUnitValue );
                if ( pcszEnum )
                {
                    _tcsncpy( szBuffer, pcszEnum, uMaxLen );
                    szBuffer[ uMaxLen - 1 ] = _T('\0');
                    hr = S_OK;
                }
            }
        }
        break;

    case UNIT_INTEGER:
        hr = Format(0, szBuffer, uMaxLen, _T("<0d>"), lUnitValue);
        break;

    case UNIT_RELATIVE:
        if ( lUnitValue >= 0 )
        {
            szBuffer [ nOutLen++ ] = _T('+');
        }
        hr = Format(0, szBuffer+nOutLen, uMaxLen-nOutLen, _T("<0d>"), lUnitValue);
        break;

    case UNIT_NULLVALUE:
        // BUGBUG Need to chage save code to NOT comapre the lDefault directly
        // But to go through the handler
        szBuffer[0] = 0;
        hr = S_OK;
        break;

    case UNIT_TIMESRELATIVE:
        _tcscpy ( szBuffer, TypeNames [ uvt ].pszName );
        hr = Format(0, szBuffer+_tcslen ( szBuffer ), uMaxLen-_tcslen ( szBuffer ),
            _T("<0d>"), lUnitValue);
        break;

    default:
        hr = Format(0, ach, ARRAY_SIZE(ach), _T("<0d>"), lUnitValue);
        if ( hr )
            goto Cleanup;

        nLen = _tcslen ( ach );

        LPTSTR pStr = ach;

        if ( ach [ 0 ] == _T('-') )
        {
            szBuffer [ nOutLen++ ] = _T('-');
            pStr++; nLen--;
        }

        if ( nLen > TypeNames [ uvt ].wShiftOffset )
        {
            for ( i = 0 ; i < ( nLen - TypeNames [ uvt ].wShiftOffset ) ; i++ )
            {
                szBuffer [ nOutLen++ ] = *pStr++;
            }
            szBuffer [ nOutLen++ ] = _T('.');
            for ( i = 0 ; i < TypeNames [ uvt ].wShiftOffset ; i++ )
            {
                szBuffer [ nOutLen++ ] = *pStr++;
            }
        }
        else
        {
            szBuffer [ nOutLen++ ] = _T('0');
            szBuffer [ nOutLen++ ] = _T('.');
            for ( i = 0 ; i < ( TypeNames [ uvt ].wShiftOffset - nLen ) ; i++ )
            {
                szBuffer [ nOutLen++ ] = _T('0');
            }
            for ( i = 0 ; i < nLen ; i++ )
            {
                szBuffer [ nOutLen++ ] = *pStr++;
            }
        }

        // Strip trailing 0 digits. If there's at least one trailing digit put a point
        for ( i = nOutLen-1 ; ; i-- )
        {
            if ( szBuffer [ i ] == _T('.') )
            {
                nOutLen--;
                break;
            }
            else if ( szBuffer [ i ] == _T('0') )
            {
                nOutLen--;
            }
            else
            {
                break;
            }
        }

        // Append the type prefix, unless it's the default and not forced
        if(uvt == UNIT_FLOAT)
        {
            fAppend = FALSE;
        }
        else if(!fAlwaysAppendUnit && !( pPropDesc->GetPPFlags() & PROPPARAM_LENGTH ) )
        {
            CUnitValue      uvDefault;
            uvDefault.SetRawValue ( pPropDesc -> ulTagNotPresentDefault );
            UNITVALUETYPE   uvtDefault = uvDefault.GetUnitType();

            if(uvt == uvtDefault || (uvtDefault == UNIT_NULLVALUE && uvt == UNIT_PIXELS))
            {
                fAppend = FALSE;
            }
        }

        if(fAppend)
            _tcscpy ( szBuffer + nOutLen, TypeNames [ uvt ].pszName );
        else
            szBuffer [ nOutLen ] = _T('\0');
                
        break;
    }
Cleanup:
    RRETURN ( hr );
}

HRESULT
CUnitValue::IsValid ( const PROPERTYDESC *pPropDesc ) const
{
    HRESULT hr = S_OK;
    BOOL    fTestAnyHow=FALSE;
    UNITVALUETYPE uvt = GetUnitType();
    long lUnitValue = GetUnitValue();
    NUMPROPPARAMS *ppp = (NUMPROPPARAMS *)(pPropDesc + 1);

    if ( ( ppp->lMin == 0 ) && ( lUnitValue < 0 ) )
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    switch ( uvt )
    {
    case UNIT_TIMESRELATIVE:
        if ( ! ( ppp ->bpp.dwPPFlags & PROPPARAM_TIMESRELATIVE ) )
        {
            hr = E_INVALIDARG;
            goto Cleanup;
        }
        break;

    case UNIT_ENUM:
        if ( !(ppp->bpp.dwPPFlags & PROPPARAM_ENUM) &&
             !(ppp->bpp.dwPPFlags & PROPPARAM_ANUMBER) )
        {
            hr = E_INVALIDARG;
            goto Cleanup;
        }
        break;

    case UNIT_INTEGER:
        if ( !(ppp->bpp.dwPPFlags & PROPPARAM_FONTSIZE) && 
             !(ppp->bpp.dwPPFlags & PROPPARAM_ANUMBER) )
        {
            hr = E_INVALIDARG;
            goto Cleanup;
        }
        break;

    case UNIT_RELATIVE:
        if ( ppp ->bpp.dwPPFlags & PROPPARAM_FONTSIZE )
        {
            // Netscape treats any FONTSIZE as valid . A value greater than
            // 6 gets treated as seven, less than -6 gets treated as -6.
            // 8 and 9 get treated as "larger" and "smaller".
            goto Cleanup;
        }
        else if ( !( ppp ->bpp.dwPPFlags & PROPPARAM_ANUMBER ) )
        {
            hr = E_INVALIDARG;
            goto Cleanup;
        }
        break;

    case UNIT_PERCENT:
        if ( ! ( ppp ->bpp.dwPPFlags & PROPPARAM_PERCENTAGE ) )
        {
            hr = E_INVALIDARG;
            goto Cleanup;
        }
        else
        {
            long lMin = ((CUnitValue*)&(ppp->lMin))->GetUnitValue();
            long lMax = ((CUnitValue*)&(ppp->lMax))->GetUnitValue();

            lUnitValue = lUnitValue /  TypeNames [ UNIT_PERCENT ].wScaleMult;

            if ( lUnitValue < lMin || lUnitValue > lMax )
            {
                hr = E_INVALIDARG;
                goto Cleanup;
            }
        }
        break;

    case UNIT_NULLVALUE: // Always valid
        break;

    case UNIT_PIXELS:  // pixels are HTML and CSS valid
        fTestAnyHow = TRUE;
        // fall through

    case UNIT_FLOAT:
    default:  // other Units of measurement are only CSS valid
        if ( ! ( ppp ->bpp.dwPPFlags & PROPPARAM_LENGTH ) && !fTestAnyHow)
        {
            hr = E_INVALIDARG;
            goto Cleanup;
        }
        else
        {
            // it is a length but sometimes we do not want negatives
            long lMin = ((CUnitValue*)&(ppp->lMin))->GetUnitValue();
            long lMax = ((CUnitValue*)&(ppp->lMax))->GetUnitValue();
            if ( lUnitValue < lMin || lUnitValue > lMax )
            {
                hr = E_INVALIDARG;
                goto Cleanup;
            }
        }

        break;
    }
Cleanup:
    RRETURN1 ( hr, E_INVALIDARG );
}

// The following conversion table converts whole units of basic types and assume
// 1 twip = 1/20 point
// 1 pica = 12 points
// 1 point = 1/72 inch
// 1 in = 2.54 cm

#define CVIX(off) ( off - UNIT_POINT )

CUnitValue::ConvertTable CUnitValue::BasicConversions[6][6] =
{
    {  // From UNIT_POINT
        { 1,1 },                // To UNIT_POINT
        { 1,12 },               // To UNIT_PICA
        { 1, 72 },              // To UNIT_INCH
        { 1*254, 72*100 },      // To UNIT_CM
        { 1*2540, 72*100 },     // To UNIT_MM
        { 20,1 },               // To UNIT_EM 
    },
    { // From UNIT_PICA
        { 12,1 },               // To UNIT_POINT
        { 1,1 },                // To UNIT_PICS
        { 12, 72 },             // To UNIT_INCH
        { 12*254, 72*100 },     // To UNIT_CM
        { 12*2540, 72*100 },    // To UNIT_MM
        { 20*12, 1 },           // To UNIT_EM 
    },
    { // From UNIT_INCH
        { 72,1 },               // To UNIT_POINT
        { 72,12 },              // To UNIT_PICA
        { 1, 1 },               // To UNIT_INCH
        { 254, 100 },           // To UNIT_CM
        { 2540, 100 },          // To UNIT_MM
        { 1440,1 },             // To UNIT_EM 
    },
    { // From UNIT_CM
        { 72*100,1*254 },        // To UNIT_POINT
        { 72*100,12*254 },       // To UNIT_PICA
        { 100, 254 },            // To UNIT_INCH
        { 1, 1 },                // To UNIT_CM
        { 10, 1 },               // To UNIT_MM
        { 20*72*100, 254 },      // To UNIT_EM 
    },
    { // From UNIT_MM
        { 72*100,1*2540 },       // To UNIT_POINT
        { 72*100,12*2540 },      // To UNIT_PICA
        { 100, 2540 },           // To UNIT_INCH
        { 10, 1 },               // To UNIT_CM
        { 1,1 },                 // To UNIT_MM
        { 20*72*100, 2540 },     // To UNIT_EM 
    },
    { // From UNIT_EM
        { 1,20 },                // To UNIT_POINT
        { 1, 20*12 },            // To UNIT_PICA
        { 1, 20*72 },            // To UNIT_INCH
        { 254, 20*72*100 },      // To UNIT_CM
        { 2540, 20*72*100 },     // To UNIT_MM
        { 1,1 },                 // To UNIT_EM 
    },
};

// EM's, EN's and EX's get converted to pixels

// Whever we convert to-from pixels we use the screen logpixs to ensure that the screen size
// matches the printer size


/* static */
long CUnitValue::ConvertTo ( long lValue, 
                            UNITVALUETYPE uvtFromUnits, 
                            UNITVALUETYPE uvtTo, 
                            DIRECTION direction,
                            long  lFontHeight /* = 1 */)
{
    UNITVALUETYPE uvtToUnits;
    long lFontMul = 1,lFontDiv = 1;

    if ( uvtFromUnits == uvtTo )
    {
        return lValue;
    }

    if ( uvtFromUnits == UNIT_PIXELS )
    {
        int logPixels = ( direction == DIRECTION_CX ) ? g_sizePixelsPerInch.cx :
            g_sizePixelsPerInch.cy;

        // Convert to inches ( stored precision )
        lValue = MulDivQuick ( lValue, TypeNames [ UNIT_INCH ].wScaleMult,
            logPixels );

        uvtFromUnits = UNIT_INCH;
        uvtToUnits = uvtTo;
    }
    else if ( uvtTo == UNIT_PIXELS )
    {
        uvtToUnits = UNIT_INCH;
    }
    else
    {
        uvtToUnits = uvtTo;
    }

    if ( uvtToUnits == UNIT_EM )
        lFontDiv = lFontHeight;
    if ( uvtFromUnits == UNIT_EM )
        lFontMul = lFontHeight;

    if ( uvtToUnits == UNIT_EX )
    {
        lFontDiv = lFontHeight*2;
        uvtToUnits = UNIT_EM;
    }
    if ( uvtFromUnits == UNIT_EX )
    {
        lFontMul = lFontHeight/2;
        uvtFromUnits = UNIT_EM;
    }

    // Note that we perform two conversions in one here to avoid losing
    // intermediate accuracy for conversions to pixels, i.e. we're converting
    // units to inches and from inches to pixels.
    if ( uvtTo == UNIT_PIXELS )
    {
        int logPixels = ( direction == DIRECTION_CX ) ? g_sizePixelsPerInch.cx :
            g_sizePixelsPerInch.cy;

        lValue = MulDivQuick ( lValue,
            lFontMul * BasicConversions [CVIX(uvtFromUnits)][CVIX(uvtToUnits)].lMul * 
                               TypeNames [ uvtToUnits ].wScaleMult * logPixels,
            lFontDiv * BasicConversions [CVIX(uvtFromUnits)][CVIX(uvtToUnits)].lDiv * 
                               TypeNames [ uvtFromUnits ].wScaleMult *
                               TypeNames [ UNIT_INCH ].wScaleMult );
    }
    else
    {
        lValue = MulDivQuick ( lValue,
            lFontMul * BasicConversions [CVIX(uvtFromUnits)][CVIX(uvtToUnits)].lMul * 
                               TypeNames [ uvtToUnits ].wScaleMult ,
            lFontDiv * BasicConversions [CVIX(uvtFromUnits)][CVIX(uvtToUnits)].lDiv * 
                               TypeNames [ uvtFromUnits ].wScaleMult);
    }        


    return lValue;
}

// Set the Percent Value - Don't need a transform because we assume lNewValue is in the
// same transform as lPercentOfValue
BOOL CUnitValue::SetPercentValue ( long lNewValue, DIRECTION direction, long lPercentOfValue )
{
    UNITVALUETYPE uvtToUnits = GetUnitType() ;
    long lUnitValue = GetUnitValue();

    // Set the internal percentage to reflect what percentage of lMaxValue
    if ( lPercentOfValue == 0 )
    {
        lUnitValue = 0;
    }
    else if ( uvtToUnits == UNIT_PERCENT )
    {
        lUnitValue = MulDivQuick ( lNewValue, 100*TypeNames [ UNIT_PERCENT ].wScaleMult, lPercentOfValue  );
    }
    else
    {
        lUnitValue = MulDivQuick ( lNewValue, TypeNames [ UNIT_TIMESRELATIVE ].wScaleMult, lPercentOfValue  );
    }
    SetValue ( lUnitValue, uvtToUnits );
    return TRUE;
}


BOOL CUnitValue::SetFromHimetricValue ( long lNewValue, 
                                       DIRECTION direction, 
                                       long lPercentOfValue,
                                       long lFontHeight)
{
    UNITVALUETYPE uvtToUnits = GetUnitType() ;

    if ( uvtToUnits == UNIT_PERCENT || uvtToUnits == UNIT_TIMESRELATIVE )
    {
        return SetPercentValue ( lNewValue, direction, lPercentOfValue );
    }
    else
    {
        return SetHimetricMeasureValue ( lNewValue, direction, lFontHeight );
    }
}


// Set the Value from the current system-wide measurement units - currently HiMetric for
// measurement type, but retains the previous storage unit type. If you pass in
// a transform, I'll convert to Window units ( assume stored value in Document units )
BOOL CUnitValue::SetHimetricMeasureValue ( long lNewValue, 
                                           DIRECTION direction, 
                                           long lFontHeight )
{
    UNITVALUETYPE uvtToUnits = GetUnitType() ;
    long lToValue;

    // Check illegal conversions
    if ( uvtToUnits ==  UNIT_TIMESRELATIVE || uvtToUnits ==  UNIT_PERCENT )
    {
        return FALSE;
    }

    if ( uvtToUnits ==  UNIT_NULLVALUE )
    {
        // Not had a unit type yet
        uvtToUnits = UNIT_PIXELS;
    }

    lToValue = ConvertTo ( lNewValue, UNIT_MM, uvtToUnits, direction, lFontHeight );

    SetValue ( lToValue, uvtToUnits );

    return TRUE;
}

long CUnitValue::GetPixelValueCore ( CTransform *pTransform, 
                                     DIRECTION direction,
                                     long lPercentOfValue,
                                     long lFontHeight /* =1 */) const
{
    UNITVALUETYPE uvtFromUnits = GetUnitType() ;
    long lRetValue;

    switch ( uvtFromUnits )
    {
    case UNIT_TIMESRELATIVE:
        lRetValue = MulDivQuick(GetUnitValue(), lPercentOfValue,
                                TypeNames[UNIT_TIMESRELATIVE].wScaleMult);
        break;

    case UNIT_NULLVALUE:
        return 0L;

    case UNIT_PERCENT:
        lRetValue = GetPercentValue ( direction, lPercentOfValue );
        break;

    case UNIT_EX:
    case UNIT_EM:
        // turn the twips into pixels
        lRetValue = ConvertTo(GetUnitValue(), 
                              uvtFromUnits, 
                              UNIT_PIXELS, 
                              direction, 
                              lFontHeight);

        // Still need to scale since zooming has been taken out and thus ConvertTo() no longer
        // scales (zooms) for printing (IE5 8543, IE4 11376).  Formerly:  pTransform = NULL;
        break;

    case UNIT_EN:
        lRetValue = MulDivQuick(GetUnitValue(), lFontHeight,
                                TypeNames[UNIT_EN].wScaleMult);
        break;

    case UNIT_ENUM:
        // BUGBUG We need to put proper handling for enum in the Apply() so that
        // no one calls GetPixelValue on (for example) the margin CUV objects if
        // they're set to "auto".
        Assert( "Shouldn't call GetPixelValue() on enumerated unit values!" );
        return 0;

    case UNIT_INTEGER:
    case UNIT_RELATIVE:
        // BUGBUG this doesn't seem right; nothing's being done to convert the
        // retrieved value to anything resembling pixels.
        return GetUnitValue();

    case UNIT_FLOAT:
        lRetValue = MulDivQuick(GetUnitValue(), lFontHeight,
                                TypeNames[UNIT_FLOAT].wScaleMult);
        break;

    default:
        lRetValue = GetMeasureValue ( direction, UNIT_PIXELS );
        break;
    }
    // Finally convert from Window to Doc units, assuming return value is a length
    // For conversions to HIMETRIC, involve the transform
    if ( pTransform  )
    {
        BOOL fRelative = UNIT_PERCENT == uvtFromUnits ||
                         UNIT_TIMESRELATIVE == uvtFromUnits;

        if ( direction == DIRECTION_CX )
        {
            lRetValue = pTransform->DocPixelsFromWindowX(lRetValue, fRelative);
        }
        else
        {
            lRetValue = pTransform->DocPixelsFromWindowY(lRetValue, fRelative);
        }
    }
    return lRetValue;
}


// Get the Value in the cuurent system-wide measurement units - currently HiMetric for
// measurement type, just the value for UNIT_INTEGER or UNIT_RELATIVE. If you pass in
// a trannsform, I'll convert to Document units ( assume stored value in Window units )
long CUnitValue::GetMeasureValue ( DIRECTION direction, 
                                  UNITVALUETYPE uvtConvertToUnits, 
                                  long lFontHeight ) const
{
    UNITVALUETYPE uvtFromUnits = GetUnitType() ;
    long lValue = GetUnitValue();

    return ConvertTo ( lValue, uvtFromUnits, uvtConvertToUnits, direction, lFontHeight );
}

long CUnitValue::GetPercentValue ( DIRECTION direction, long lPercentOfValue ) const
{
    //  lPercentOfValue is the 100%value, expressed in Window Himmetric coords
    UNITVALUETYPE uvtFromUnits = GetUnitType() ;
    long lUnitValue = GetUnitValue();

    if ( uvtFromUnits == UNIT_PERCENT )
    {
        return  MulDivQuick ( lPercentOfValue, lUnitValue, 100*TypeNames [ UNIT_PERCENT ].wScaleMult );
    }
    else
    {
        return MulDivQuick ( lPercentOfValue, lUnitValue, TypeNames [ UNIT_TIMESRELATIVE ].wScaleMult );
    }
}

HRESULT
CUnitValue::FromString ( LPCTSTR pStr, const PROPERTYDESC *pPropDesc )
{
    NUMPROPPARAMS *ppp = (NUMPROPPARAMS *)(pPropDesc + 1);

    // if string is empty and boolean is an allowed type make it so...

    // Note the special handling of the table relative "*" here

    while ( _istspace ( *pStr ) )
        pStr++;

    if ( _istdigit ( pStr [ 0 ] ) || pStr [ 0 ] == _T('+') || pStr [ 0 ] == _T('-') ||
        pStr [ 0 ] == _T('*') || pStr [ 0 ] == _T('.') )
    {
        // Assume some numerical value followed by an optional unit
        // specifier
        RRETURN1 ( NumberFromString ( pStr, pPropDesc ), E_INVALIDARG );
    }
    else
    {
        // Assume an enum
        if ( ppp -> bpp.dwPPFlags & PROPPARAM_ENUM )
        {
            long lNewValue;

            if ( S_OK == LookupEnumString( ppp, (LPTSTR)pStr, &lNewValue ) )
            {
                SetValue( lNewValue, UNIT_ENUM );
                return S_OK;
            }
        }
        return E_INVALIDARG;
    }
}

float CUnitValue::GetFloatValueInUnits ( UNITVALUETYPE uvtTo, 
                                        DIRECTION dir, 
                                        long lFontHeight /*=1*/ )
{
    // Convert the unitvalue into a floating point number in units uvt
    long lEncodedValue = ConvertTo ( GetUnitValue(), GetUnitType(), uvtTo,  dir, lFontHeight );

    return (float)lEncodedValue / (float)TypeNames [ uvtTo ].wScaleMult;
}


HRESULT CUnitValue::ConvertToUnitType ( UNITVALUETYPE uvtConvertTo, 
                                       long           lCurrentPixelSize, 
                                       DIRECTION      dir,
                                       long           lFontHeight /*=1*/)
{
    AssertSz(GetUnitType() != UNIT_PERCENT, "Cannot handle percent units! Contact RGardner!");
    
    // Convert the unit value to the new type but keep the
    // absolute value of the units the same
    // e.e. 20.34 pels -> 20.34 in

    if ( GetUnitType() == uvtConvertTo )
        return S_OK;

    if ( GetUnitType() == UNIT_NULLVALUE )
    {
        // The current value is Null, treat it as a pixel unit with lCurrentPixelSize value
        SetValue ( lCurrentPixelSize, UNIT_PIXELS );
        // fall into one of the cases below
    }

    if ( uvtConvertTo == UNIT_NULLVALUE )
    {
        SetValue ( 0, UNIT_NULLVALUE );
    }
    else if ( IsScalerUnit ( GetUnitType() ) && IsScalerUnit ( uvtConvertTo )  )
    {
        // Simply converting between scaler units e.g. 20mm => 2px
        SetValue ( ConvertTo ( GetUnitValue(), GetUnitType(), uvtConvertTo, dir, lFontHeight ), uvtConvertTo );
    }
    else if ( IsScalerUnit ( GetUnitType() ) )
    {
        // Convert from a scaler type to a non-scaler type e.g. 20.3in => %
        // Have no reference for conversion, max it out
        switch ( uvtConvertTo )
        {
        case UNIT_PERCENT:
            SetValue ( 100 * TypeNames [ UNIT_PERCENT ].wScaleMult, UNIT_PERCENT );
            break;

        case UNIT_TIMESRELATIVE:
            SetValue ( 1 * TypeNames [ UNIT_PERCENT ].wScaleMult, UNIT_TIMESRELATIVE );
            break;

        default:
            AssertSz ( FALSE, "Invalid type passed to ConvertToUnitType()" );
            return E_INVALIDARG;
        }
    }
    else if ( IsScalerUnit ( uvtConvertTo) )
    {
        // Convert from a non-scaler to a scaler type use the current pixel size
        // e.g. We know that 20% is equivalent to 152 pixels. So we convert
        // the current pixel value to the new metric unit
        SetValue ( ConvertTo ( lCurrentPixelSize, UNIT_PIXELS, uvtConvertTo, dir, lFontHeight ), uvtConvertTo );
    }
    else
    {
        // Convert between non-scaler types e,g, 20% => *
        // Since we have only two non-sclaer types right now:-
        switch ( uvtConvertTo )
        {
        case UNIT_PERCENT:  // From UNIT_TIMESRELATIVE
            // 1* == 100%
            SetValue ( MulDivQuick ( GetUnitValue(),
                100 * TypeNames [ UNIT_PERCENT ].wScaleMult,
                TypeNames [ UNIT_TIMESRELATIVE ].wScaleMult ) ,
                uvtConvertTo );
            break;

        case UNIT_TIMESRELATIVE: // From UNIT_PERCENT
            // 100% == 1*
            SetValue ( MulDivQuick ( GetUnitValue(),
                TypeNames [ UNIT_TIMESRELATIVE ].wScaleMult,
                TypeNames [ UNIT_PERCENT ].wScaleMult * 100 ) ,
                uvtConvertTo );
            break;

        default:
            AssertSz ( FALSE, "Invalid type passed to ConvertToUnitType()" );
            return E_INVALIDARG;
        }
    }
    return S_OK;
}


/* static */
BOOL CUnitValue::IsScalerUnit ( UNITVALUETYPE uvt ) 
{
    switch ( uvt )
    {
    case UNIT_POINT:
    case UNIT_PICA:
    case UNIT_INCH:
    case UNIT_CM:
    case UNIT_MM:
    case UNIT_PIXELS:
    case UNIT_EM:
    case UNIT_EN:
    case UNIT_EX:
    case UNIT_FLOAT:
        return TRUE;

    default:
        return FALSE;
    }
}

HRESULT CUnitValue::SetFloatValueKeepUnits ( float fValue, 
                                             UNITVALUETYPE uvt, 
                                             long lCurrentPixelValue,
                                             DIRECTION dir,
                                             long lFontHeight)
{
    long lNewValue;

    // Set the new value to the equivalent of : fValue in uvt units
    // If the current value is a percent use the lCurrentPixelValue to
    // work out the new size

    // There are a number of restrictions on the uvt coming in. Since it's either
    // Document Units when called from CUnitMeasurement::SetValue or
    // PIXELS when called from CUnitMeasurement::SetPixelValue we restrict our
    // handling to the possible doc unit types - which are all the "metric" types

    lNewValue = (long) ( fValue * (float)TypeNames [ uvt ].wScaleMult );
    
    if ( uvt == GetUnitType() )
    {
        SetValue ( lNewValue, uvt );
    }
    else if ( uvt == UNIT_NULLVALUE || !IsScalerUnit ( uvt ) )
    {
        return E_INVALIDARG;
    }
    else if ( GetUnitType() == UNIT_NULLVALUE )
    {
        // If we're current NUll, just set to the new value
        SetValue ( lNewValue, uvt );
    }
    else if ( IsScalerUnit ( GetUnitType() ) )
    {
        // If the conversion is to/from metric units, just convert units
        SetValue ( ConvertTo ( lNewValue, uvt, GetUnitType(), dir, lFontHeight ), GetUnitType() );
    }
    else
    {
        // unit value holds a relative unit,
        // Convert the fValue,uvt to pixels
        lNewValue = ConvertTo ( lNewValue, uvt, UNIT_PIXELS, dir, lFontHeight );

        if ( GetUnitType() == UNIT_PERCENT )
        {
            SetValue ( MulDivQuick ( lNewValue, 
                                     100*TypeNames [ UNIT_PERCENT ].wScaleMult, 
                                     (!! lCurrentPixelValue) ?  lCurrentPixelValue : 1), // don't pass in 0 for divisor
                UNIT_PERCENT );
        }
        else
        {
            // Cannot keep units without loss of precision, over-ride
            SetValue ( lNewValue, UNIT_PIXELS );
        }
    }
    return S_OK;
}

HRESULT CUnitValue::SetFloatUnitValue ( float fValue )
{
    // Set the value from fValue
    // but keep the internaly stored units intact
    UNITVALUETYPE uvType;

    if ( ( uvType = GetUnitType()) == UNIT_NULLVALUE )
    {
        // This case can only happen if SetUnitValue is called when the current value is NULL
        // so force a unit type
        uvType = UNIT_PIXELS;
    }

    // Convert the fValue into a CUnitValue with current unit
    SetValue ( (long)(fValue * (float)TypeNames [ uvType ].wScaleMult),
        uvType );
    return S_OK;
}

#ifdef WIN16
#pragma code_seg( "baseprop2_TEXT" )
#endif // ndef WIN16

//
// Support for CColorValue follows.
//
// IEUNIX: color value starts from 0x10.

#define nColorNamesStartOffset 0x11
#define COLOR_INDEX(i) (((DWORD)nColorNamesStartOffset + (DWORD)(i)) << 24)

const struct COLORVALUE_PAIR aColorNames[] =
{
    { _T("aliceblue"),             COLOR_INDEX(0x00) | 0xfff8f0 },
    { _T("antiquewhite"),          COLOR_INDEX(0x01) | 0xd7ebfa },
    { _T("aqua"),                  COLOR_INDEX(0x02) | 0xffff00 },
    { _T("aquamarine"),            COLOR_INDEX(0x03) | 0xd4ff7f },
    { _T("azure"),                 COLOR_INDEX(0x04) | 0xfffff0 },
    { _T("beige"),                 COLOR_INDEX(0x05) | 0xdcf5f5 },
    { _T("bisque"),                COLOR_INDEX(0x06) | 0xc4e4ff },
    { _T("black"),                 COLOR_INDEX(0x07) | 0x000000 },
    { _T("blanchedalmond"),        COLOR_INDEX(0x08) | 0xcdebff },
    { _T("blue"),                  COLOR_INDEX(0x09) | 0xff0000 },
    { _T("blueviolet"),            COLOR_INDEX(0x0a) | 0xe22b8a },
    { _T("brown"),                 COLOR_INDEX(0x0b) | 0x2a2aa5 },
    { _T("burlywood"),             COLOR_INDEX(0x0c) | 0x87b8de },
    { _T("cadetblue"),             COLOR_INDEX(0x0d) | 0xa09e5f },
    { _T("chartreuse"),            COLOR_INDEX(0x0e) | 0x00ff7f },
    { _T("chocolate"),             COLOR_INDEX(0x0f) | 0x1e69d2 },
    { _T("coral"),                 COLOR_INDEX(0x10) | 0x507fff },
    { _T("cornflowerblue"),        COLOR_INDEX(0x11) | 0xed9564 },
    { _T("cornsilk"),              COLOR_INDEX(0x12) | 0xdcf8ff },
    { _T("crimson"),               COLOR_INDEX(0x13) | 0x3c14dc },
    { _T("cyan"),                  COLOR_INDEX(0x14) | 0xffff00 },
    { _T("darkblue"),              COLOR_INDEX(0x15) | 0x8b0000 },
    { _T("darkcyan"),              COLOR_INDEX(0x16) | 0x8b8b00 },
    { _T("darkgoldenrod"),         COLOR_INDEX(0x17) | 0x0b86b8 },
    { _T("darkgray"),              COLOR_INDEX(0x18) | 0xa9a9a9 },
    { _T("darkgreen"),             COLOR_INDEX(0x19) | 0x006400 },
    { _T("darkkhaki"),             COLOR_INDEX(0x1a) | 0x6bb7bd },
    { _T("darkmagenta"),           COLOR_INDEX(0x1b) | 0x8b008b },
    { _T("darkolivegreen"),        COLOR_INDEX(0x1c) | 0x2f6b55 },
    { _T("darkorange"),            COLOR_INDEX(0x1d) | 0x008cff },
    { _T("darkorchid"),            COLOR_INDEX(0x1e) | 0xcc3299 },
    { _T("darkred"),               COLOR_INDEX(0x1f) | 0x00008b },
    { _T("darksalmon"),            COLOR_INDEX(0x20) | 0x7a96e9 },
    { _T("darkseagreen"),          COLOR_INDEX(0x21) | 0x8fbc8f },
    { _T("darkslateblue"),         COLOR_INDEX(0x22) | 0x8b3d48 },
    { _T("darkslategray"),         COLOR_INDEX(0x23) | 0x4f4f2f },
    { _T("darkturquoise"),         COLOR_INDEX(0x24) | 0xd1ce00 },
    { _T("darkviolet"),            COLOR_INDEX(0x25) | 0xd30094 },
    { _T("deeppink"),              COLOR_INDEX(0x26) | 0x9314ff },
    { _T("deepskyblue"),           COLOR_INDEX(0x27) | 0xffbf00 },
    { _T("dimgray"),               COLOR_INDEX(0x28) | 0x696969 },
    { _T("dodgerblue"),            COLOR_INDEX(0x29) | 0xff901e },
    { _T("firebrick"),             COLOR_INDEX(0x2a) | 0x2222b2 },
    { _T("floralwhite"),           COLOR_INDEX(0x2b) | 0xf0faff },
    { _T("forestgreen"),           COLOR_INDEX(0x2c) | 0x228b22 },
    { _T("fuchsia"),               COLOR_INDEX(0x2d) | 0xff00ff },
    { _T("gainsboro"),             COLOR_INDEX(0x2e) | 0xdcdcdc },
    { _T("ghostwhite"),            COLOR_INDEX(0x2f) | 0xfff8f8 },
    { _T("gold"),                  COLOR_INDEX(0x30) | 0x00d7ff },
    { _T("goldenrod"),             COLOR_INDEX(0x31) | 0x20a5da },
    { _T("gray"),                  COLOR_INDEX(0x32) | 0x808080 },
    { _T("green"),                 COLOR_INDEX(0x33) | 0x008000 },
    { _T("greenyellow"),           COLOR_INDEX(0x34) | 0x2fffad },
    { _T("honeydew"),              COLOR_INDEX(0x35) | 0xf0fff0 },
    { _T("hotpink"),               COLOR_INDEX(0x36) | 0xb469ff },
    { _T("indianred"),             COLOR_INDEX(0x37) | 0x5c5ccd },
    { _T("indigo"),                COLOR_INDEX(0x38) | 0x82004b },
    { _T("ivory"),                 COLOR_INDEX(0x39) | 0xf0ffff },
    { _T("khaki"),                 COLOR_INDEX(0x3a) | 0x8ce6f0 },
    { _T("lavender"),              COLOR_INDEX(0x3b) | 0xfae6e6 },
    { _T("lavenderblush"),         COLOR_INDEX(0x3c) | 0xf5f0ff },
    { _T("lawngreen"),             COLOR_INDEX(0x3d) | 0x00fc7c },
    { _T("lemonchiffon"),          COLOR_INDEX(0x3e) | 0xcdfaff },
    { _T("lightblue"),             COLOR_INDEX(0x3f) | 0xe6d8ad },
    { _T("lightcoral"),            COLOR_INDEX(0x40) | 0x8080f0 },
    { _T("lightcyan"),             COLOR_INDEX(0x41) | 0xffffe0 },
    { _T("lightgoldenrodyellow"),  COLOR_INDEX(0x42) | 0xd2fafa },
    { _T("lightgreen"),            COLOR_INDEX(0x43) | 0x90ee90 },
    { _T("lightgrey"),             COLOR_INDEX(0x44) | 0xd3d3d3 },
    { _T("lightpink"),             COLOR_INDEX(0x45) | 0xc1b6ff },
    { _T("lightsalmon"),           COLOR_INDEX(0x46) | 0x7aa0ff },
    { _T("lightseagreen"),         COLOR_INDEX(0x47) | 0xaab220 },
    { _T("lightskyblue"),          COLOR_INDEX(0x48) | 0xface87 },
    { _T("lightslategray"),        COLOR_INDEX(0x49) | 0x998877 },
    { _T("lightsteelblue"),        COLOR_INDEX(0x4a) | 0xdec4b0 },
    { _T("lightyellow"),           COLOR_INDEX(0x4b) | 0xe0ffff },
    { _T("lime"),                  COLOR_INDEX(0x4c) | 0x00ff00 },
    { _T("limegreen"),             COLOR_INDEX(0x4d) | 0x32cd32 },
    { _T("linen"),                 COLOR_INDEX(0x4e) | 0xe6f0fa },
    { _T("magenta"),               COLOR_INDEX(0x4f) | 0xff00ff },
    { _T("maroon"),                COLOR_INDEX(0x50) | 0x000080 },
    { _T("mediumaquamarine"),      COLOR_INDEX(0x51) | 0xaacd66 },
    { _T("mediumblue"),            COLOR_INDEX(0x52) | 0xcd0000 },
    { _T("mediumorchid"),          COLOR_INDEX(0x53) | 0xd355ba },
    { _T("mediumpurple"),          COLOR_INDEX(0x54) | 0xdb7093 },
    { _T("mediumseagreen"),        COLOR_INDEX(0x55) | 0x71b33c },
    { _T("mediumslateblue"),       COLOR_INDEX(0x56) | 0xee687b },
    { _T("mediumspringgreen"),     COLOR_INDEX(0x57) | 0x9afa00 },
    { _T("mediumturquoise"),       COLOR_INDEX(0x58) | 0xccd148 },
    { _T("mediumvioletred"),       COLOR_INDEX(0x59) | 0x8515c7 },
    { _T("midnightblue"),          COLOR_INDEX(0x5a) | 0x701919 },
    { _T("mintcream"),             COLOR_INDEX(0x5b) | 0xfafff5 },
    { _T("mistyrose"),             COLOR_INDEX(0x5c) | 0xe1e4ff },
    { _T("moccasin"),              COLOR_INDEX(0x5d) | 0xb5e4ff },
    { _T("navajowhite"),           COLOR_INDEX(0x5e) | 0xaddeff },
    { _T("navy"),                  COLOR_INDEX(0x5f) | 0x800000 },
    { _T("oldlace"),               COLOR_INDEX(0x60) | 0xe6f5fd },
    { _T("olive"),                 COLOR_INDEX(0x61) | 0x008080 },
    { _T("olivedrab"),             COLOR_INDEX(0x62) | 0x238e6b },
    { _T("orange"),                COLOR_INDEX(0x63) | 0x00a5ff },
    { _T("orangered"),             COLOR_INDEX(0x64) | 0x0045ff },
    { _T("orchid"),                COLOR_INDEX(0x65) | 0xd670da },
    { _T("palegoldenrod"),         COLOR_INDEX(0x66) | 0xaae8ee },
    { _T("palegreen"),             COLOR_INDEX(0x67) | 0x98fb98 },
    { _T("paleturquoise"),         COLOR_INDEX(0x68) | 0xeeeeaf },
    { _T("palevioletred"),         COLOR_INDEX(0x69) | 0x9370db },
    { _T("papayawhip"),            COLOR_INDEX(0x6a) | 0xd5efff },
    { _T("peachpuff"),             COLOR_INDEX(0x6b) | 0xb9daff },
    { _T("peru"),                  COLOR_INDEX(0x6c) | 0x3f85cd },
    { _T("pink"),                  COLOR_INDEX(0x6d) | 0xcbc0ff },
    { _T("plum"),                  COLOR_INDEX(0x6e) | 0xdda0dd },
    { _T("powderblue"),            COLOR_INDEX(0x6f) | 0xe6e0b0 },
    { _T("purple"),                COLOR_INDEX(0x70) | 0x800080 },
    { _T("red"),                   COLOR_INDEX(0x71) | 0x0000ff },
    { _T("rosybrown"),             COLOR_INDEX(0x72) | 0x8f8fbc },
    { _T("royalblue"),             COLOR_INDEX(0x73) | 0xe16941 },
    { _T("saddlebrown"),           COLOR_INDEX(0x74) | 0x13458b },
    { _T("salmon"),                COLOR_INDEX(0x75) | 0x7280fa },
    { _T("sandybrown"),            COLOR_INDEX(0x76) | 0x60a4f4 },
    { _T("seagreen"),              COLOR_INDEX(0x77) | 0x578b2e },
    { _T("seashell"),              COLOR_INDEX(0x78) | 0xeef5ff },
    { _T("sienna"),                COLOR_INDEX(0x79) | 0x2d52a0 },
    { _T("silver"),                COLOR_INDEX(0x7a) | 0xc0c0c0 },
    { _T("skyblue"),               COLOR_INDEX(0x7b) | 0xebce87 },
    { _T("slateblue"),             COLOR_INDEX(0x7c) | 0xcd5a6a },
    { _T("slategray"),             COLOR_INDEX(0x7d) | 0x908070 },
    { _T("snow"),                  COLOR_INDEX(0x7e) | 0xfafaff },
    { _T("springgreen"),           COLOR_INDEX(0x7f) | 0x7fff00 },
    { _T("steelblue"),             COLOR_INDEX(0x80) | 0xb48246 },
    { _T("tan"),                   COLOR_INDEX(0x81) | 0x8cb4d2 },
    { _T("teal"),                  COLOR_INDEX(0x82) | 0x808000 },
    { _T("thistle"),               COLOR_INDEX(0x83) | 0xd8bfd8 },
    { _T("tomato"),                COLOR_INDEX(0x84) | 0x4763ff },
    { _T("turquoise"),             COLOR_INDEX(0x85) | 0xd0e040 },
    { _T("violet"),                COLOR_INDEX(0x86) | 0xee82ee },
    { _T("wheat"),                 COLOR_INDEX(0x87) | 0xb3def5 },
    { _T("white"),                 COLOR_INDEX(0x88) | 0xffffff },
    { _T("whitesmoke"),            COLOR_INDEX(0x89) | 0xf5f5f5 },
    { _T("yellow"),                COLOR_INDEX(0x8a) | 0x00ffff },
    { _T("yellowgreen"),           COLOR_INDEX(0x8b) | 0x32cd9a }
};

static struct COLORVALUE_PAIR aSystemColors[] =
{
    { _T("activeborder"),       COLOR_ACTIVEBORDER},    // Active window border.
    { _T("activecaption"),      COLOR_ACTIVECAPTION},   // Active window caption.
    { _T("appworkspace"),       COLOR_APPWORKSPACE},    // Background color of multiple document interface (MDI) applications.
    { _T("background"),         COLOR_BACKGROUND},      // Desktop background.
    { _T("buttonface"),         COLOR_BTNFACE},         // Face color for three-dimensional display elements.
    { _T("buttonhighlight"),    COLOR_BTNHIGHLIGHT},    // Dark shadow for three-dimensional display elements.
    { _T("buttonshadow"),       COLOR_BTNSHADOW},       // Shadow color for three-dimensional display elements (for edges facing away from the light source).
    { _T("buttontext"),         COLOR_BTNTEXT},         // Text on push buttons.
    { _T("captiontext"),        COLOR_CAPTIONTEXT},     // Text in caption, size box, and scroll bar arrow box.
    { _T("graytext"),           COLOR_GRAYTEXT},        // Grayed (disabled) text. This color is set to 0 if the current display driver does not support a solid gray color.
    { _T("highlight"),          COLOR_HIGHLIGHT},       // Item(s) selected in a control.
    { _T("highlighttext"),      COLOR_HIGHLIGHTTEXT},   // Text of item(s) selected in a control.
    { _T("inactiveborder"),     COLOR_INACTIVEBORDER},  // Inactive window border.
    { _T("inactivecaption"),    COLOR_INACTIVECAPTION}, // Inactive window caption.
    { _T("inactivecaptiontext"),COLOR_INACTIVECAPTIONTEXT}, // Color of text in an inactive caption.
    { _T("infobackground"),     COLOR_INFOBK},          // Background color for tooltip controls.
    { _T("infotext"),           COLOR_INFOTEXT},        // Text color for tooltip controls.
    { _T("menu"),               COLOR_MENU},            // Menu background.
    { _T("menutext"),           COLOR_MENUTEXT},        // Text in menus.
    { _T("scrollbar"),          COLOR_SCROLLBAR},       // Scroll bar gray area.
    { _T("threeddarkshadow"),   COLOR_3DDKSHADOW },     // Dark shadow for three-dimensional display elements.
    { _T("threedface"),         COLOR_3DFACE},
    { _T("threedhighlight"),    COLOR_3DHIGHLIGHT},     // Highlight color for three-dimensional display elements (for edges facing the light source.)
    { _T("threedlightshadow"),  COLOR_3DLIGHT},         // Light color for three-dimensional display elements (for edges facing the light source.)
    { _T("threedshadow"),       COLOR_3DSHADOW},        // Dark shadow for three-dimensional display elements.
    { _T("window"),             COLOR_WINDOW},          // Window background.
    { _T("windowframe"),        COLOR_WINDOWFRAME},     // Window frame.
    { _T("windowtext"),         COLOR_WINDOWTEXT},      // Text in windows.
};

int RTCCONV
CompareColorValuePairsByName( const void * pv1, const void * pv2 )
{
    return StrCmpIC( ((struct COLORVALUE_PAIR *)pv1)->szName,
                     ((struct COLORVALUE_PAIR *)pv2)->szName );
}

const struct COLORVALUE_PAIR *
FindColorByName( const TCHAR * szString )
{
    struct COLORVALUE_PAIR ColorName;

    ColorName.szName = szString;

    return (const struct COLORVALUE_PAIR *)bsearch( &ColorName,
                                              aColorNames,
                                              ARRAY_SIZE(aColorNames),
                                              sizeof(struct COLORVALUE_PAIR),
                                              CompareColorValuePairsByName );
}

const struct COLORVALUE_PAIR *
FindColorBySystemName( const TCHAR * szString )
{
    struct COLORVALUE_PAIR ColorName;

    ColorName.szName = szString;

    return (const struct COLORVALUE_PAIR *)bsearch( &ColorName,
                                                    aSystemColors,
                                                    ARRAY_SIZE(aSystemColors),
                                                    sizeof(struct COLORVALUE_PAIR),
                                                    CompareColorValuePairsByName );
}

const struct COLORVALUE_PAIR *
FindColorByColor( DWORD lColor )
{
    int nIndex;
    const struct COLORVALUE_PAIR * pColorName = NULL;

    // Unfortunately, this is a linear search.
    // Fortunately, we will need to lookup the name rarely.

    // The mask (high) byte should be clear.
    if (!(lColor & CColorValue::MASK_FLAG)) 
    {
        // Can't possibly be one of our colors
        return NULL;
    }

    for ( nIndex = ARRAY_SIZE( aColorNames ); nIndex-- ; )
    {
        if (lColor == (aColorNames[nIndex].dwValue & CColorValue::MASK_COLOR))
        {
            pColorName = aColorNames + nIndex;
            break;
        }
    }

    return pColorName;
}

const struct COLORVALUE_PAIR *
FindColorByValue( DWORD dwValue )
{
    CColorValue cvColor = dwValue;
    Assert(ARRAY_SIZE( aColorNames ) > cvColor.Index( dwValue ));
    return aColorNames + cvColor.Index( dwValue );
}

CColorValue::CColorValue ( VARIANT * pvar )
{
    if (V_VT(pvar) == VT_I4)
    {
        SetValue( V_I4( pvar ), TRUE );
    }
    else if (V_VT(pvar) == VT_BSTR)
    {
        FromString( V_BSTR( pvar ) );
    }
    else
    {
        _dwValue = VALUE_UNDEF;
    }
}

int
CColorValue::Index(const DWORD dwValue) const
{
    // The index, as stored in dwValue, should be 1-based.  This
    // is because we want to retain 0 as 'default' flag (undefined).

    // For the return value, however, we generally want an index
    // into aColorNames, so convert it to a 0-based index.

    // IEUNIX: color value starts form nColorNamesStartOffset.

    int nOneBasedIndex = (int)(dwValue >> 24);
    Assert( ARRAY_SIZE( aColorNames ) >= nOneBasedIndex - nColorNamesStartOffset );
    return nOneBasedIndex - nColorNamesStartOffset;
}

long
CColorValue::SetValue( long lColor, BOOL fLookupName, TYPE type)
{
    const struct COLORVALUE_PAIR * pColorName = NULL;

    if (fLookupName)
    {
        pColorName = FindColorByColor( lColor );
    }

    if (pColorName)
    {
        SetValue( pColorName );
    }
    else
    {
#ifdef UNIX
        if ( CColorValue(lColor).IsUnixSysColor()) {
            _dwValue = lColor;
            return _dwValue;
        }
#endif

        _dwValue = (lColor & MASK_COLOR) | type;
    }

    return _dwValue & MASK_COLOR;
}

long
CColorValue::SetValue( const struct COLORVALUE_PAIR * pColorName )
{
    Assert( pColorName );
    Assert( ARRAY_SIZE( aColorNames ) > Index( pColorName->dwValue ) );

    _dwValue = pColorName->dwValue;
    return _dwValue & MASK_COLOR;
}

long
CColorValue::SetRawValue( DWORD dwValue )
{
    _dwValue = dwValue;
    AssertSz( S_OK == IsValid(), "CColorValue::SetRawValue invalid value.");

#ifdef UNIX
    if ( IsUnixSysColor()) {
        return _dwValue;
    }
#endif

    return _dwValue & MASK_COLOR;
}

long
CColorValue::SetSysColor(int nIndex)
{
    _dwValue = TYPE_SYSINDEX + (nIndex << 24);

    return _dwValue & MASK_COLOR;
}


//+-----------------------------------------------------------------
//
//  Member : FromString
//
//  Synopsis : '#' tells us to force a Hex interpretation, w/0 it
//      we try to do a name look up, and if that fails, then we
//      fall back on hex interpretation anyhow.
//
//+-----------------------------------------------------------------
HRESULT
CColorValue::FromString( LPCTSTR pch, BOOL fValidOnly /*=FALSE*/ , int iStrLen /* =-1 */)
{
    HRESULT hr = E_INVALIDARG;

    if (!pch || !*pch)
    {
        Undefine();
        hr = S_OK;
        goto Cleanup;
    }

    if (iStrLen == -1)
        iStrLen = _tcslen(pch);
    else
        iStrLen = min(iStrLen, (int)_tcslen(pch));

    // Leading '#' means it's a hex color, not a named color.
    if ( *pch != _T('#') )
    {
        // it can only be a name if there it is all alphanumeric
        int   i;
        BOOL  bNotAName = FALSE;
        BOOL  bFoundNonHex = FALSE;

        for (i=0; i<iStrLen; i++)
        {
            if (!_istalpha(pch[i]))
            {
                bNotAName = TRUE;
                break;
            }
            if (!bFoundNonHex && !_istxdigit(pch[i]))
                bFoundNonHex = TRUE;
        }

        // if it still COULD be a name, try it
        if (!bNotAName && bFoundNonHex)
        {
            hr = THR_NOTRACE(NameColor( pch ));
            //S_OK means we got it
            if (!hr)
                goto Cleanup;
        }

        // Try it as an rgb(r,g,b) functional notation
        hr = RgbColor( pch, iStrLen );
        // S_OK means it was.
        if ( !hr )
            goto Cleanup;
    }
    else
    {
         // Skip the '#' character
         pch++;
         iStrLen--;
    }

    // either its NOT a known name or it is a hex value so
    //   convert if necessary
	hr = THR_NOTRACE(HexColor(pch, iStrLen, fValidOnly));

Cleanup:
    RRETURN1( hr, E_INVALIDARG );
}

HRESULT GetRgbParam( LPCTSTR &pch, int &iStrLen, BYTE *pResult )
{
    HRESULT hr = S_OK;
    long lValue = 0;
    long lFractionDecimal = 1;
    BOOL fIsNegative = FALSE;

    Assert( "GetRgbParam requires a place to store its result!" && pResult );

    while ( iStrLen && _istspace( *pch ) )
    {
        pch++;
        iStrLen--;
    }

    if ( iStrLen && ( *pch == _T('-') ) )
    {
        fIsNegative = TRUE;
        pch++;
        iStrLen--;
    }

    if ( !(iStrLen && _istdigit( *pch ) ) )
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    while ( iStrLen && _istdigit( *pch ) )
    {
        lValue *= 10;
        lValue += (long)(*pch) - '0';
        pch++;
        iStrLen--;
        if ( lValue > 255 )
        {   // We're over the maximum, might as well stop paying attention.
            while ( _istdigit( *pch ) )
            {
                pch++;
                iStrLen--;
            }
        }
    }

    if ( iStrLen && ( *pch == _T('.') ) )
    {
        pch++;
        iStrLen--;
        while ( iStrLen && _istdigit( *pch ) )
        {
            lValue *= 10;
            lValue += (long)(*pch) - _T('0');
            lFractionDecimal *= 10;
            pch++;
            iStrLen--;

            if ( lValue > ( LONG_MAX / 5100 ) ) // 5100 = 255 (multiplier in algorithm below) * 10 (multiplier in loop above) * 2 (slop)
            {   // Safety valve so we don't overrun a long.
                while ( _istdigit( *pch ) )
                {
                    pch++;
                    iStrLen--;
                }
            }
        }

    }

    if ( iStrLen && ( *pch == _T('%') ) )
    {
        if ( ( lValue / lFractionDecimal ) >= 100 )
            *pResult = 255;
        else
            *pResult = (BYTE) ( ( lValue * 255 ) / ( lFractionDecimal * 100 ) );
        pch++;
        iStrLen--;
    }
    else
    {
        if ( ( lValue / lFractionDecimal ) > 255 )
            *pResult = 255;
        else
            *pResult = (BYTE)(lValue/lFractionDecimal);
    }

    while ( iStrLen && _istspace( *pch ) )
    {
        pch++;
        iStrLen--;
    }

    if ( iStrLen && ( *pch == _T(',') ) )
    {
        pch++;
        iStrLen--;
    }

    if ( fIsNegative )
        *pResult = 0;
Cleanup:
    RRETURN1( hr, E_INVALIDARG );
}

//+------------------------------------------------------------------------
//
//  Member   : CColorValue::RgbColor
//
//  Synopsis : Convert an RGB functional notation to RGB DWORD
//
//  On entry : pch: "rgb( r, g, b )"
//
//  Returns  : 
//+-----------------------------------------------------------------------
HRESULT CColorValue::RgbColor( LPCTSTR pch, int iStrLen )
{
    HRESULT hr = E_INVALIDARG;
    DWORD   rgbColor = 0;

    if ( iStrLen > 4 && !_tcsnicmp( pch, 4, _T("rgb("), 4 ) )
    {
        // Looks like a functional notation to me.
        pch += 4;
        iStrLen -= 4;

#ifndef unix
        hr = GetRgbParam( pch, iStrLen, &((BYTE *)&rgbColor)[0] );
#else
        hr = GetRgbParam( pch, iStrLen, &((BYTE *)&rgbColor)[3] );
#endif
        if ( hr != S_OK )
            goto Cleanup;
#ifndef unix
        hr = GetRgbParam( pch, iStrLen, &((BYTE *)&rgbColor)[1] );
#else
        hr = GetRgbParam( pch, iStrLen, &((BYTE *)&rgbColor)[2] );
#endif
        if ( hr != S_OK )
            goto Cleanup;
#ifndef unix
        hr = GetRgbParam( pch, iStrLen, &((BYTE *)&rgbColor)[2] );
#else
        hr = GetRgbParam( pch, iStrLen, &((BYTE *)&rgbColor)[1] );
#endif
        if ( hr != S_OK )
            goto Cleanup;

        if ( ( iStrLen == 1 ) && ( *pch == _T(')') ) )
            SetValue( rgbColor, FALSE, TYPE_RGB );
        else
            hr = E_INVALIDARG;
    }

Cleanup:
    RRETURN1( hr, E_INVALIDARG );
}

//+-----------------------------------------------------------------------
//
//  Member   : GetHexDigit
//
//  Synopsis : Convert an ASCII hex digit character to binary
//
//  On entry : ch: ASCII character '0'..'9','A'..'F', or 'a'..'f'
//
//  Returns  : binary equivalent of ch interpretted at hex digit
//             0 if ch isn't a hex digit
//+-----------------------------------------------------------------------
static inline BYTE GetHexDigit( TCHAR ch, BOOL &fIsValid )
{
    if ( ch >= _T('0') && ch <= _T('9') ) 
    {
        return (BYTE) (ch - _T('0'));
    } 
    else 
    {
        if (ch >= _T('a') && ch <=('f'))
            return (BYTE) (ch - _T('a') + 10);            
        if ( ch >= _T('A') && ch <= _T('F') )
            return (BYTE) (ch - _T('A') + 10);
    }
    fIsValid = FALSE;
    return 0;
}

//+------------------------------------------------------------------------
//  Member: CColorValue::HexColor
//
//  Synopsis: Modifed from IE3 code base, this function takes a string which
//      needs to be converted into a hex rep for the rrggbb color.  The 
//      converstion is annoyingly lenient.
//
//  Note: The leniency of this routine is designed to emulate a popular Web browser
//        Fundamentally, the idea is to divide the given hex string into thirds, with 
//        each third being one of the colors (R,G,B).  If the string length isn't a multiple
//        three, it is (logically) extended with 0's.  Any character that isn't a hex digit
//        is treated as if it were a 0.  When each individual color spec is greater than
//        8 bits, the largest supplied color is used to determine how the given color
//        values should be interpretted (either as is, or scaled down to 8 bits).
//
//+------------------------------------------------------------------------
#define NUM_PRIMARIES   3
#define SHIFT_NUM_BASE  4
#define NUMBER_BASE     (1<<SHIFT_NUM_BASE)
#define MAX_COLORLENGTH 255

HRESULT
CColorValue::HexColor( LPCTSTR pch, int iStrLen, BOOL fValidOnly )
{
    HRESULT hr   = E_INVALIDARG;
    LPCTSTR pchTemp = pch;
    int     vlen = (iStrLen+NUM_PRIMARIES-1)/NUM_PRIMARIES;   // how many digits per section
    int     i, j;
    unsigned int rgb_vals[NUM_PRIMARIES];
    unsigned int max_seen = 0;
    DWORD   rgbColor = 0;
    TYPE    ColorType;
    BOOL    fIsValid = TRUE;

    if (!pch)
        goto Cleanup;

    if ( fValidOnly && (iStrLen != 3) && (iStrLen != 6) && (iStrLen != 9) )
        return E_INVALIDARG;

    // convert string to three color digits ala IE3, and others
    for ( i = 0; i < NUM_PRIMARIES; i++ ) 
    {                               
        // for each tri-section of the string
        for ( j = 0, rgb_vals[i] = 0; j < vlen; j++ ) 
        {                        
            rgb_vals[i] = (rgb_vals[i]<<SHIFT_NUM_BASE) + GetHexDigit(*pchTemp, fIsValid);

            if ( fValidOnly && !fIsValid )
                return E_INVALIDARG;

            if ( *pchTemp )                                         
                pchTemp++; 
        }
        if ( rgb_vals[i] > max_seen ) 
            max_seen = rgb_vals[i]; 
    }

    // rgb_values now has the triad in decimal
    // If any individual color component uses more than 8 bits, 
    //      scale all color values down.
    for ( i = 0 ; max_seen > MAX_COLORLENGTH ; i++ ) 
    {
        max_seen >>= SHIFT_NUM_BASE; 
    }

    if ( i>0 )
    {
        for ( j = 0; j < NUM_PRIMARIES; j++ ) 
            rgb_vals[j] >>= i*SHIFT_NUM_BASE;
    }

    // we used to do arena compatible handling of pound[1,2,3] colors always,
    // however we want NS compatibility and so #ab is turned to #0a0b00 rather
    // than #aabb00, but ONLY for non-stylesheet properties.
    if ( fValidOnly )
    {
        // This code makes #RGB expand to #RRGGBB, instead of #0R0G0B, which apparently those ... people at NS think is correct.
        if ( vlen == 1 )	// only 4 bits/color - scale up by 4 bits (& add to self, to get full range).
        {
            for ( i = 0; i < NUM_PRIMARIES; i++ ) 
            {
                rgb_vals[ i ] += rgb_vals[ i ]<<SHIFT_NUM_BASE;
            }
        }
    }

    // now put the rgb_vals together into a format our code understands
    // mnopqr => qropmn  or in colorese:  rrggbb => bbggrr
#ifdef BIG_ENDIAN
    ((BYTE *)&rgbColor)[0] = 0; 
    ((BYTE *)&rgbColor)[1] = (BYTE)rgb_vals[2];
    ((BYTE *)&rgbColor)[2] = (BYTE)rgb_vals[1];
    ((BYTE *)&rgbColor)[3] = (BYTE)rgb_vals[0];
#else
    ((BYTE *)&rgbColor)[0] = (BYTE)rgb_vals[0];
    ((BYTE *)&rgbColor)[1] = (BYTE)rgb_vals[1];
    ((BYTE *)&rgbColor)[2] = (BYTE)rgb_vals[2];
    ((BYTE *)&rgbColor)[3] = 0;
#endif
    switch (iStrLen) 
    {
    case 0:
        ColorType = TYPE_UNDEF;
        break;
    case 1:
        ColorType = TYPE_POUND1;
        break;
    case 2:
        ColorType = TYPE_POUND2;
        break;
    case 3:
        ColorType = TYPE_POUND3;
        break;
    case 4:
        ColorType = TYPE_POUND4;
        break;
    case 5:
        ColorType = TYPE_POUND5;
        break;
    case 6:
    default:
        ColorType = TYPE_POUND6;
        break;
    }

    // and finally set the color
    SetValue( rgbColor, FALSE, ColorType);
    hr = S_OK;

Cleanup:
    RRETURN1( hr, E_INVALIDARG );
}

//+--------------------------------------------------------------------
//
//  member : Name Color
//
//  Sysnopsis : trying to parse a color string.. it could be a name
//      so lets look for it. if we find it, set the value and leave
//      otherwite return invalidarg (i.e. membernotfound)
//
//+--------------------------------------------------------------------
HRESULT
CColorValue::NameColor( LPCTSTR pch )
{
    const struct COLORVALUE_PAIR * pColorName = FindColorByName( pch );
    HRESULT hr = S_OK;

    if (pColorName)
        SetValue( pColorName );
    else
    {
        pColorName = FindColorBySystemName( pch );
        if ( pColorName )
            _dwValue = TYPE_SYSNAME + (pColorName->dwValue<<24);
        else
        {
            if ( !_wcsicmp( pch, _T("transparent") ) )
                _dwValue = (DWORD)TYPE_TRANSPARENT;
            else
                hr = E_INVALIDARG;
        }
    }
    RRETURN1( hr, E_INVALIDARG );
}

HRESULT
CColorValue::Persist (
    IStream *pStream,
    const PROPERTYDESC * pPropDesc ) const
{
    TCHAR cBuffer[ 64 ];
    HRESULT hr;

    Assert( S_OK == IsValid() );

    hr = FormatBuffer( cBuffer, ARRAY_SIZE( cBuffer ), pPropDesc );
    if (hr)
        goto Cleanup;

    hr = WriteText( pStream, cBuffer );

Cleanup:
    RRETURN ( hr );
}

HRESULT
CColorValue::FormatBuffer (
    TCHAR * szBuffer,
    UINT uMaxLen,
    const PROPERTYDESC *pPropDesc,
    BOOL fReturnAsHex6 /*==FALSE */) const
{
    HRESULT hr = S_OK;
    const struct COLORVALUE_PAIR * pColorPair;
    LONG lValue;
    DWORD type = GetType();
    INT idx;
    DWORD dwSysColor;
    TCHAR achFmt[5] = _T("<0C>");

    switch (type)
    {
        default:
        case TYPE_UNDEF:
            *szBuffer = 0;
            break;

        case TYPE_NAME:
            // requests from the OM set fReturnAsHex to true, so instead
            // of returning "red; we want to return "#ff0000"
            if (!fReturnAsHex6)
            {
                pColorPair = FindColorByValue( GetRawValue() );
                Assert(pColorPair);
                _tcsncpy( szBuffer, pColorPair->szName, uMaxLen );
                break;
            }
            // else fall through !!!

        case TYPE_RGB:
            // requests from the OM set fReturnAsHex to true, so instead
            // of returning "rgb(255,0,0)" we want to return "#ff0000"
            if (!fReturnAsHex6)
            {
                hr = Format(0, szBuffer, uMaxLen, _T("rgb(<0d>,<1d>,<2d>)"), (_dwValue & 0x0000FF), (_dwValue & 0x00FF00)>>8, (_dwValue & 0xFF0000)>>16);
                break;
            }
            // else fall through !!!

        case TYPE_SYSINDEX:
        case TYPE_POUND6:
        case TYPE_POUND5:
        case TYPE_POUND4:
            achFmt[2] = _T('c');

        case TYPE_POUND3:
        case TYPE_POUND2:
        case TYPE_POUND1:
            lValue = (LONG)GetColorRef();
            if (fReturnAsHex6)
            {
                achFmt[2] = _T('c');
            }
            hr = Format(0, szBuffer, uMaxLen, achFmt, lValue);
            if (!fReturnAsHex6 && (type != TYPE_POUND6))
            {
                szBuffer[((TYPE_POUND1 - type)>>24) + 2] = _T('\0');
            }
            break;

#ifdef UNIX
        case TYPE_UNIXSYSCOL:
            hr = E_INVALIDARG;
            if (!g_fSysColorInit)
                InitColorTranslation();

            for (int idx = 0; idx < ARRAY_SIZE(g_acrSysColor); idx++)
			{
                if ( _dwValue == g_acrSysColor[idx] )
                {
                    _tcsncpy( szBuffer, aSystemColors[idx].szName, uMaxLen );
                    hr = S_OK;
                    break;
                }
			}
            if ( hr )
                AssertSz ( FALSE, "Invalid Color Stored" );
            break;
#endif

        case TYPE_SYSNAME:
            dwSysColor = (_dwValue & MASK_SYSCOLOR)>>24;
            hr = E_INVALIDARG;
            for ( idx = 0; idx < ARRAY_SIZE(aSystemColors); idx++ )
            {
                if ( dwSysColor == aSystemColors[idx].dwValue )
                {
                    _tcsncpy( szBuffer, aSystemColors[idx].szName, uMaxLen );
                    hr = S_OK;
                    break;
                }
            }
            AssertSz ( !hr, "Invalid Color Stored" );
            break;
        case TYPE_TRANSPARENT:
            _tcsncpy( szBuffer, _T("transparent"), uMaxLen );
            break;
    }
    RRETURN ( hr );
}

HRESULT
CColorValue::IsValid() const
{
    return ((TYPE_NAME != GetType()) ||
            (ARRAY_SIZE(aColorNames) > Index(GetRawValue()))) ? S_OK : E_INVALIDARG;
}

OLE_COLOR
CColorValue::GetOleColor() const
{
    if ( IsSysColor()) 
        return OLECOLOR_FROM_SYSCOLOR((_dwValue & MASK_SYSCOLOR)>>24);

#ifdef UNIX       
    if ( IsUnixSysColor()) {
        return (OLE_COLOR)_dwValue;
    }
#endif

    return (OLE_COLOR)(_dwValue & MASK_COLOR);
}

DWORD 
CColorValue::GetIntoRGB(void) const
{
    DWORD dwRet = GetColorRef();
    return ((dwRet & 0xFF) << 16) | (dwRet & 0xFF00) | ((dwRet & 0xFF0000) >> 16);
}

COLORREF
CColorValue::GetColorRef() const
{
    if ( IsSysColor() )
        return (COLORREF)GetSysColorQuick((_dwValue & MASK_SYSCOLOR)>>24);

#ifdef UNIX
        if ( IsUnixSysColor()) 
            return (COLORREF)_dwValue;
#endif

    return (COLORREF) ((_dwValue & MASK_COLOR) | 0x02000000);
}

CColorValue::TYPE
CColorValue::GetType() const
{
    DWORD dwFlag = _dwValue & MASK_FLAG;

    // What a royal mess.  See the comment in cdbase.hxx for more info
    if ((dwFlag < TYPE_TRANSPARENT) && (dwFlag >= TYPE_NAME))
    {
        if (dwFlag >= TYPE_SYSNAME)
        {
            // A fancy way of avoiding yet another comparison (against TYPE_SYSINDEX)

            dwFlag &= ~MASK_SYSCOLOR;
        }
        else
        {
            dwFlag = TYPE_NAME;
        }
    }

    return (CColorValue::TYPE) dwFlag;
}

// Allocates and returns a string reperesenting the color value as #RRGGBB
HRESULT 
CColorValue::FormatAsPound6Str(BSTR *pszColor, DWORD dwColor)
{
    HRESULT     hr;
    OLECHAR     szBuf[8];

    hr = THR(Format(0, szBuf, ARRAY_SIZE(szBuf), _T("<0c>"), dwColor));
    if(hr)
        goto Cleanup;

    hr = FormsAllocString(szBuf, pszColor);

Cleanup:
    RRETURN(hr);
}

HRESULT
BASICPROPPARAMS::GetColor(CVoid * pObject, BSTR * pbstrColor, 
                          BOOL fReturnAsHex /* == FALSE */) const
{
    HRESULT hr;
    TCHAR szBuffer[64];
    CColorValue cvColor;

    Assert(!(dwPPFlags & PROPPARAM_GETMFHandler));

    cvColor = (CColorValue)GetAvNumber (pObject, this + 1, sizeof(CColorValue));
    hr = cvColor.FormatBuffer(szBuffer, ARRAY_SIZE(szBuffer), NULL, fReturnAsHex );
    if ( hr )
        goto Cleanup;

    hr = FormsAllocString(szBuffer, pbstrColor);
Cleanup:
    RRETURN( hr );
}


HRESULT
BASICPROPPARAMS::GetColor(CVoid * pObject, CStr * pcstr, 
                          BOOL fReturnAsHex/* =FALSE*/, BOOL *pfValuePresent /*= NULL*/) const
{
    HRESULT hr;
    TCHAR szBuffer[64];
    CColorValue cvColor;

    Assert(!(dwPPFlags & PROPPARAM_GETMFHandler));

    cvColor = (CColorValue)GetAvNumber (pObject, this + 1, sizeof(CColorValue), pfValuePresent);
    hr = cvColor.FormatBuffer(szBuffer, ARRAY_SIZE(szBuffer), NULL, fReturnAsHex );
    if ( hr )
        goto Cleanup;
    hr = pcstr->Set( szBuffer );
Cleanup:
    RRETURN( hr );
}

HRESULT
BASICPROPPARAMS::GetColor(CVoid * pObject, DWORD * pdwValue) const
{
    CColorValue cvColor;

    Assert(!(dwPPFlags & PROPPARAM_GETMFHandler));

    cvColor = (CColorValue)GetAvNumber (pObject, this + 1, sizeof(CColorValue));
    *pdwValue = cvColor.GetRawValue();

    return S_OK;
}

HRESULT
BASICPROPPARAMS::SetColor(CVoid * pObject, TCHAR * pch, WORD wFlags) const
{
    HRESULT hr;
    CColorValue cvColor;

    hr = cvColor.FromString(pch);
    if (hr)
        goto Cleanup;

    Assert(!(dwPPFlags & PROPPARAM_SETMFHandler));

    hr = THR( SetAvNumber ( pObject, (DWORD)cvColor.GetRawValue(), this+1, sizeof ( CColorValue ), wFlags ) );

Cleanup:
    RRETURN1( hr, E_INVALIDARG );
}

HRESULT
BASICPROPPARAMS::SetColor(CVoid * pObject, DWORD dwValue, WORD wFlags) const
{
    HRESULT hr;
    CColorValue cvColor;

    Assert(!(dwPPFlags & PROPPARAM_SETMFHandler));

    cvColor.SetRawValue(dwValue);

    hr = THR( SetAvNumber ( pObject, (DWORD)cvColor.GetRawValue(), this+1, sizeof ( CColorValue ), wFlags ) );

    RRETURN1(hr,E_INVALIDARG);
}



//+-----------------------------------------------------
//
//  Member : StripCRLF
//
//  Synopsis : strips CR and LF from a provided string
//      the returned string has been allocated and needs 
//      to be freed by the caller.
//
//+-----------------------------------------------------
static HRESULT
StripCRLF(TCHAR *pSrc, TCHAR **ppDest)
{
    HRESULT hr = S_OK;
    long    lLength;
    TCHAR  *pTarget = NULL;


    if (!ppDest)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    if ( !pSrc )
        goto Cleanup;

    lLength = _tcslen(pSrc);
    *ppDest= (TCHAR*)MemAlloc(Mt(StripCRLF), sizeof(TCHAR)*(lLength+1));
    if (!*ppDest)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    pTarget = *ppDest;

    for (; lLength>0; lLength--)
    {
        if ((*pSrc != _T('\r')) && (*pSrc != _T('\n'))) 
        {
            // we want it.
            *pTarget = *pSrc;
            pTarget++;
        }
        pSrc++;
    }

    *pTarget = _T('\0');

Cleanup:
    RRETURN(hr);
}


HRESULT ttol_with_error ( LPCTSTR pStr, long *plValue )
{
    // Always do base 10 regardless of contents of 
    RRETURN1 ( PropertyStringToLong ( pStr, NULL, 10, 0, (unsigned long *)plValue ), E_INVALIDARG );
}


static HRESULT RTCCONV PropertyStringToLong (
        LPCTSTR nptr,
        TCHAR **endptr,
        int ibase,
        int flags,
        unsigned long *plNumber )
{
        const TCHAR *p;
        TCHAR c;
        unsigned long number;
        unsigned digval;
        unsigned long maxval;

        *plNumber = 0;                  /* on error result is 0 */

        p = nptr;                       /* p is our scanning pointer */
        number = 0;                     /* start with zero */

        c = *p++;                       /* read char */
        while (_istspace(c))
            c = *p++;                   /* skip whitespace */

        if (c == '-') 
        {
            flags |= FL_NEG;        /* remember minus sign */
            c = *p++;
        }
        else if (c == '+')
            c = *p++;               /* skip sign */

        if (ibase < 0 || ibase == 1 || ibase > 36) 
        {
            /* bad base! */
            if (endptr)
                    /* store beginning of string in endptr */
                    *endptr = (TCHAR *)nptr;
            return E_INVALIDARG;              /* return 0 */
        }
        else if (ibase == 0) 
        {
            /* determine base free-lance, based on first two chars of
               string */
            if (c != L'0')
                    ibase = 10;
            else if (*p == L'x' || *p == L'X')
                    ibase = 16;
            else
                    ibase = 8;
        }

        if (ibase == 16) 
        {
            /* we might have 0x in front of number; remove if there */
            if (c == L'0' && (*p == L'x' || *p == L'X')) 
            {
                    ++p;
                    c = *p++;       /* advance past prefix */
            }
        }

        /* if our number exceeds this, we will overflow on multiply */
        maxval = ULONG_MAX / ibase;


        for (;;) 
        {      /* exit in middle of loop */
            /* convert c to value */
            if (_istdigit(c))
                    digval = c - L'0';
            else if (_istalpha(c))
            {
                if ( ibase > 10 )
                {
                    if (c >= 'a' && c <= 'z')
                        digval = (unsigned)c - 'a' + 10;
                    else
                        digval = (unsigned)c - 'A' + 10;
                }
                else
                {
                    return E_INVALIDARG;              /* return 0 */
                }
            }
            else
                break;

            if (digval >= (unsigned)ibase)
                break;          /* exit loop if bad digit found */

            /* record the fact we have read one digit */
            flags |= FL_READDIGIT;

            /* we now need to compute number = number * base + digval,
               but we need to know if overflow occured.  This requires
               a tricky pre-check. */

            if (number < maxval || (number == maxval &&
                (unsigned long)digval <= ULONG_MAX % ibase)) 
            {
                    /* we won't overflow, go ahead and multiply */
                    number = number * ibase + digval;
            }
            else 
            {
                    /* we would have overflowed -- set the overflow flag */
                    flags |= FL_OVERFLOW;
            }

            c = *p++;               /* read next digit */
        }

        --p;                            /* point to place that stopped scan */

        if (!(flags & FL_READDIGIT)) 
        {
            number = 0L;                        /* return 0 */

            /* no number there; return 0 and point to beginning of
               string */
            if (endptr)
                /* store beginning of string in endptr later on */
                p = nptr;

            return E_INVALIDARG;            // Return error not a number
        }
        else if ( (flags & FL_OVERFLOW) ||
                  ( !(flags & FL_UNSIGNED) &&
                    ( ( (flags & FL_NEG) && (number > -LONG_MIN) ) ||
                      ( !(flags & FL_NEG) && (number > LONG_MAX) ) ) ) )
        {
            /* overflow or signed overflow occurred */
            //errno = ERANGE;
            if ( flags & FL_UNSIGNED )
                    number = ULONG_MAX;
            else if ( flags & FL_NEG )
                    number = (unsigned long)(-LONG_MIN);
            else
                    number = LONG_MAX;
        }

        if (endptr != NULL)
            /* store pointer to char that stopped the scan */
            *endptr = (TCHAR *)p;

        if (flags & FL_NEG)
                /* negate result if there was a neg sign */
                number = (unsigned long)(-(long)number);

        *plNumber = number; 
        return S_OK;                  /* done. */
}

//+---------------------------------------------------------------------------
//
//  Member: PROPERTYDESC::HandleCodeProperty
//
//+---------------------------------------------------------------------------

HRESULT
PROPERTYDESC::HandleCodeProperty (DWORD dwOpCode, void * pv, CBase * pObject, CVoid * pSubObject) const
{
    HRESULT             hr = S_OK;
    BASICPROPPARAMS *   ppp = (BASICPROPPARAMS *)(this + 1);

    if (ISSET(dwOpCode))
    {
        Assert(!(ISSTREAM(dwOpCode))); // we can't do this yet...

        //
        // Assert ( actualTypeOf(pv) == (VARIANT*) );
        //

        switch(OPCODE(dwOpCode))
        {
        case HANDLEPROP_DEFAULT:
        case HANDLEPROP_VALUE:
            hr = THR(HandleStringProperty(dwOpCode, pv, pObject, pSubObject));
            break;

        case HANDLEPROP_AUTOMATION:

            // Assert ( actualTypeOf(pv) == (VARIANT*) );
            Assert (PROPTYPE_VARIANT == PROPTYPE(dwOpCode) && "Only this type supported for this case");

            hr = THR(ppp->SetCodeProperty( (VARIANT*)pv, pObject, pSubObject));
            break;

        default:
            hr = E_FAIL;
            Assert(FALSE && "Invalid operation");
            break;
        }
    }
    else
    {
        switch(OPCODE(dwOpCode))
        {
        case HANDLEPROP_VALUE:
        case HANDLEPROP_STREAM:
            hr = THR(HandleStringProperty(dwOpCode, pv, pObject, pSubObject));
            break;

        case HANDLEPROP_COMPARE:
            hr = THR(HandleStringProperty(dwOpCode, pv, pObject, pSubObject));
            RRETURN1(hr,S_FALSE);
            break;

        case HANDLEPROP_AUTOMATION:
            if (!pv)
            {
                hr = E_POINTER;
                goto Cleanup;
            }

            Assert (PROPTYPE_VARIANT == PROPTYPE(dwOpCode) && "Only V_DISPATCH supported for this case");

            hr = THR(ppp->GetCodeProperty ((VARIANT*)pv, pObject, pSubObject));
            if (hr)
                goto Cleanup;
            break;

        default:
            hr = E_FAIL;
            Assert(FALSE && "Invalid operation");
        }
    }

Cleanup:

    RRETURN1(hr, E_INVALIDARG);
}


//+---------------------------------------------------------------------------
//
//  Member:     CBase::SetCodeProperty
//
//----------------------------------------------------------------------------

HRESULT
CBase::SetCodeProperty (DISPID dispidCodeProp, IDispatch *pDispCode, BOOL *pfAnyDeleted /* = NULL */)
{
    HRESULT hr = S_OK;
    BOOL    fAnyDelete;

    fAnyDelete = DidFindAAIndexAndDelete (dispidCodeProp, CAttrValue::AA_Attribute);
    fAnyDelete |= DidFindAAIndexAndDelete (dispidCodeProp, CAttrValue::AA_Internal);

    if (pDispCode)
    {
        hr = THR(AddDispatchObject(
            dispidCodeProp,
            pDispCode,
            CAttrValue::AA_Internal));
    }

    // if this is an element, let it know that events can now fire
    IGNORE_HR(OnPropertyChange(DISPID_EVPROP_ONATTACHEVENT, 0));

    if (pfAnyDeleted)
        *pfAnyDeleted = fAnyDelete;

    RRETURN (hr);
}

#ifdef USE_STACK_SPEW
#pragma check_stack(off)  
#endif 
STDMETHODIMP CBase::put_StyleComponent(BSTR v)
{
    GET_THUNK_PROPDESC
    return put_StyleComponentHelper(v, pPropDesc);
}
#ifdef USE_STACK_SPEW
#pragma check_stack(on)  
#endif 

STDMETHODIMP CBase::put_StyleComponentHelper(BSTR v, const PROPERTYDESC *pPropDesc, CAttrArray ** ppAttr)
{
    Assert(pPropDesc);

    RECALC_PUT_HELPER(pPropDesc->GetDispid())

    return pPropDesc->GetBasicPropParams()->SetStyleComponentProperty(v, this, ppAttr ? CVOID_CAST(ppAttr) : CVOID_CAST(GetAttrArray()));
}

#ifdef USE_STACK_SPEW
#pragma check_stack(off)  
#endif 
STDMETHODIMP CBase::put_Url(BSTR v)
{
    GET_THUNK_PROPDESC
    return put_UrlHelper(v, pPropDesc);
}
#ifdef USE_STACK_SPEW
#pragma check_stack(on)  
#endif 

STDMETHODIMP CBase::put_UrlHelper(BSTR v, const PROPERTYDESC *pPropDesc, CAttrArray ** ppAttr)
{
    Assert(pPropDesc);

    RECALC_PUT_HELPER(pPropDesc->GetDispid())

    return pPropDesc->GetBasicPropParams()->SetUrlProperty(v, this, ppAttr ? CVOID_CAST(ppAttr) : CVOID_CAST(GetAttrArray()));
}

#ifdef USE_STACK_SPEW
#pragma check_stack(off)  
#endif 
STDMETHODIMP CBase::put_String(BSTR v)
{
    GET_THUNK_PROPDESC
    return put_StringHelper(v, pPropDesc);
}
#ifdef USE_STACK_SPEW
#pragma check_stack(on)  
#endif 

STDMETHODIMP CBase::put_StringHelper(BSTR v, const PROPERTYDESC *pPropDesc, CAttrArray ** ppAttr)
{
    Assert(pPropDesc);

    RECALC_PUT_HELPER(pPropDesc->GetDispid())

    CVoid *pSubObj = (pPropDesc->GetBasicPropParams()->dwPPFlags & PROPPARAM_ATTRARRAY) ?
                         (ppAttr ? CVOID_CAST(ppAttr) : CVOID_CAST(GetAttrArray())) : CVOID_CAST(this);

    switch (pPropDesc->GetBasicPropParams()->wInvFunc)
    {
    case IDX_GS_BSTR:
        return pPropDesc->GetBasicPropParams()->SetStringProperty(v, this, pSubObj);
    case IDX_GS_PropEnum:
        return pPropDesc->GetNumPropParams()->SetEnumStringProperty(v, this, pSubObj);
    }
    return S_OK;
}

#ifdef USE_STACK_SPEW
#pragma check_stack(off)  
#endif 
STDMETHODIMP CBase::put_Short(short v)
{
    GET_THUNK_PROPDESC
    return put_ShortHelper(v, pPropDesc);
}
#ifdef USE_STACK_SPEW
#pragma check_stack(on)  
#endif 

STDMETHODIMP CBase::put_ShortHelper(short v, const PROPERTYDESC *pPropDesc, CAttrArray ** ppAttr)
{
    Assert(pPropDesc);

    RECALC_PUT_HELPER(pPropDesc->GetDispid())

    return pPropDesc->GetNumPropParams()->SetNumberProperty(v, this, ppAttr ? CVOID_CAST(ppAttr) : CVOID_CAST(GetAttrArray()));
}

#ifdef USE_STACK_SPEW
#pragma check_stack(off)  
#endif 
STDMETHODIMP CBase::put_Long(long v)
{
    GET_THUNK_PROPDESC
    return put_LongHelper(v, pPropDesc);
}
#ifdef USE_STACK_SPEW
#pragma check_stack(on)  
#endif 

STDMETHODIMP CBase::put_LongHelper(long v, const PROPERTYDESC *pPropDesc, CAttrArray ** ppAttr)
{
    Assert(pPropDesc);

    RECALC_PUT_HELPER(pPropDesc->GetDispid())

    CVoid *pSubObj = (pPropDesc->GetBasicPropParams()->dwPPFlags & PROPPARAM_ATTRARRAY) ?
                        (ppAttr ? CVOID_CAST(ppAttr) : CVOID_CAST(GetAttrArray())) : CVOID_CAST(this);

    return pPropDesc->GetNumPropParams()->SetNumberProperty(v, this, pSubObj);
}

#ifdef USE_STACK_SPEW
#pragma check_stack(off)  
#endif 
STDMETHODIMP CBase::put_Bool(VARIANT_BOOL v)
{
    GET_THUNK_PROPDESC
    return put_BoolHelper(v, pPropDesc);
}
#ifdef USE_STACK_SPEW
#pragma check_stack(on)  
#endif 

STDMETHODIMP CBase::put_BoolHelper(VARIANT_BOOL v, const PROPERTYDESC *pPropDesc, CAttrArray ** ppAttr)
{
    Assert(pPropDesc);

    RECALC_PUT_HELPER(pPropDesc->GetDispid())

    CVoid *pSubObj = (pPropDesc->GetBasicPropParams()->dwPPFlags & PROPPARAM_ATTRARRAY) ?
                        (ppAttr ? CVOID_CAST(ppAttr) : CVOID_CAST(GetAttrArray())) : CVOID_CAST(this);

    return pPropDesc->GetNumPropParams()->SetNumberProperty(v, this, pSubObj);
}

#ifdef USE_STACK_SPEW
#pragma check_stack(off)  
#endif 
STDMETHODIMP CBase::put_Variant(VARIANT v)
{
    GET_THUNK_PROPDESC
    return put_VariantHelper(v, pPropDesc);
}
#ifdef USE_STACK_SPEW
#pragma check_stack(on)  
#endif 

STDMETHODIMP CBase::put_VariantHelper(VARIANT v, const PROPERTYDESC *pPropDesc, CAttrArray ** ppAttr)
{
    Assert(pPropDesc);

    RECALC_PUT_HELPER(pPropDesc->GetDispid())

    return SetErrorInfo((pPropDesc->*pPropDesc->pfnHandleProperty)(HANDLEPROP_SET | HANDLEPROP_AUTOMATION | (PROPTYPE_VARIANT << 16),
                                                                   &v,
                                                                   this,
                                                                   ppAttr ? CVOID_CAST(ppAttr) : CVOID_CAST(GetAttrArray())));
}

#ifdef USE_STACK_SPEW
#pragma check_stack(off)  
#endif 
STDMETHODIMP CBase::get_Url(BSTR *p)
{
    GET_THUNK_PROPDESC
    return get_UrlHelper(p, pPropDesc);
}
#ifdef USE_STACK_SPEW
#pragma check_stack(on)  
#endif 

STDMETHODIMP CBase::get_UrlHelper(BSTR *p, const PROPERTYDESC *pPropDesc, CAttrArray ** ppAttr)
{
    Assert(pPropDesc);

    RECALC_GET_HELPER(pPropDesc->GetDispid())

    return pPropDesc->GetBasicPropParams()->GetUrlProperty(p, this, ppAttr ? CVOID_CAST(ppAttr) : CVOID_CAST(GetAttrArray()));
}

#ifdef USE_STACK_SPEW
#pragma check_stack(off)  
#endif 
STDMETHODIMP CBase::get_StyleComponent(BSTR *p)
{
    GET_THUNK_PROPDESC
    return get_StyleComponentHelper(p, pPropDesc);
}
#ifdef USE_STACK_SPEW
#pragma check_stack(on)  
#endif 

STDMETHODIMP CBase::get_StyleComponentHelper(BSTR *p, const PROPERTYDESC *pPropDesc, CAttrArray ** ppAttr)
{
    Assert(pPropDesc);

    RECALC_GET_HELPER(pPropDesc->GetDispid())

    return pPropDesc->GetBasicPropParams()->GetStyleComponentProperty(p, this, ppAttr ? CVOID_CAST(ppAttr) : CVOID_CAST(GetAttrArray()));
}

#ifdef USE_STACK_SPEW
#pragma check_stack(off)  
#endif 
STDMETHODIMP CBase::get_Property(void *p)
{
    GET_THUNK_PROPDESC
    return get_PropertyHelper(p, pPropDesc);
}
#ifdef USE_STACK_SPEW
#pragma check_stack(on)  
#endif 

STDMETHODIMP CBase::get_PropertyHelper(void *p, const PROPERTYDESC *pPropDesc, CAttrArray ** ppAttr)
{
    Assert(pPropDesc);

    RECALC_GET_HELPER(pPropDesc->GetDispid())

    CVoid *pSubObj = (pPropDesc->GetBasicPropParams()->dwPPFlags & PROPPARAM_ATTRARRAY) ?
                        (ppAttr ? CVOID_CAST(ppAttr) : CVOID_CAST(GetAttrArray())) : CVOID_CAST(this);

    switch (pPropDesc->GetBasicPropParams()->wInvFunc)
    {
    case IDX_G_VARIANT:
    case IDX_GS_VARIANT:
    	return SetErrorInfo((pPropDesc->*pPropDesc->pfnHandleProperty)(HANDLEPROP_AUTOMATION | (PROPTYPE_VARIANT << 16), p, this, pSubObj));
    case IDX_G_VARIANTBOOL:
    case IDX_GS_VARIANTBOOL:
    case IDX_G_long:
    case IDX_GS_long:
    case IDX_G_short:
    case IDX_GS_short:
	    return pPropDesc->GetNumPropParams()->GetNumberProperty(p, this, pSubObj);
    case IDX_G_BSTR:
    case IDX_GS_BSTR:
        return pPropDesc->GetBasicPropParams()->GetStringProperty((BSTR *)p, this, pSubObj);
    case IDX_G_PropEnum:
    case IDX_GS_PropEnum:
        return pPropDesc->GetNumPropParams()->GetEnumStringProperty((BSTR *)p, this, pSubObj);
    }
    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:     BASICPROPPARAMS::SetCodeProperty
//
//----------------------------------------------------------------------------

HRESULT
BASICPROPPARAMS::SetCodeProperty(VARIANT *pvarCode, CBase * pObject, CVoid *) const
{
    HRESULT hr = S_OK;

    Assert ((dwPPFlags & PROPPARAM_ATTRARRAY) && "only attr array support implemented for 'type:code'");

    if ( V_VT(pvarCode) == VT_NULL )
    {
        hr = pObject->SetCodeProperty(dispid, NULL);
    }
    else if ( V_VT(pvarCode) == VT_DISPATCH )
    {
        hr = pObject->SetCodeProperty(dispid, V_DISPATCH(pvarCode) );
    }
    else if ( V_VT(pvarCode) == VT_BSTR )
    {
        pObject->FindAAIndexAndDelete (dispid, CAttrValue::AA_Internal);
        hr = THR(pObject->AddString(dispid, V_BSTR(pvarCode),CAttrValue::AA_Attribute ));
    }
    else
    {
        hr = E_NOTIMPL;
    }

    if (!hr)
        hr = THR(pObject->OnPropertyChange(dispid, dwFlags));


    RRETURN (hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     BASICPROPPARAMS::GetCodeProperty
//
//----------------------------------------------------------------------------

HRESULT
BASICPROPPARAMS::GetCodeProperty (VARIANT *pvarCode, CBase * pObject, CVoid * pSubObject) const
{
    HRESULT     hr = S_OK;
    AAINDEX     aaidx;

    V_VT(pvarCode) = VT_NULL;

    Assert ((dwPPFlags & PROPPARAM_ATTRARRAY) && "only attr array support implemented for 'type:code'");

    aaidx = pObject->FindAAIndex(dispid, CAttrValue::AA_Internal);
    if ((AA_IDX_UNKNOWN != aaidx) && (pObject->GetVariantTypeAt(aaidx) == VT_DISPATCH))
    {
        hr = THR(pObject->GetDispatchObjectAt(aaidx, &V_DISPATCH(pvarCode)) );
        if ( hr )
            goto Cleanup;
        V_VT(pvarCode) = VT_DISPATCH;
    }
    else
    {
        LPCTSTR szCodeText;
        aaidx = pObject->FindAAIndex(dispid, CAttrValue::AA_Attribute);
        if (AA_IDX_UNKNOWN != aaidx)
        {
            hr = THR(pObject->GetStringAt(aaidx, &szCodeText) );
            if ( hr )
                goto Cleanup;
            hr = FormsAllocString ( szCodeText, &V_BSTR(pvarCode) );
            if ( hr )
                goto Cleanup;            
            V_VT(pvarCode) = VT_BSTR;
        }
    }
Cleanup:
    RRETURN (hr);
}


//+------------------------------------------------------------------------
//
//  Helper Function:    NextParenthesizedToken
//
//  Synopsis:
//      This function tokenizes a word or functional notation.
//  (BUGBUG: does not handle quoted strings!)  Expects string to have no
//  leading whitespace.
//
//  Return Values:
//      "" if the end of the string was reached.
//      pointer to following characters if success.
//-------------------------------------------------------------------------
TCHAR *NextParenthesizedToken( TCHAR *pszToken )
{
    int iParenDepth = 0;
    BOOL fInQuotes = FALSE;
    TCHAR quotechar = _T('"');

    while ( *pszToken && ( iParenDepth || !_istspace( *pszToken ) ) )
    {
        if ( !fInQuotes )
        {
            if ( ( *pszToken == _T('\'') ) || ( *pszToken == _T('"') ) )
            {
                fInQuotes = TRUE;
                quotechar = *pszToken;
            }
            else if ( iParenDepth && ( *pszToken == _T(')') ) )
                iParenDepth--;
            else if ( *pszToken == _T('(') )
                iParenDepth++;
        }
        else if ( quotechar == *pszToken )
        {
            fInQuotes = FALSE;
        }
        pszToken++;
    }
    return pszToken;
}

#ifdef WIN16
#define DEFINE_PROPERTYDESC_METHOD(fn)\
    HRESULT BUGCALL PROPERTYDESC::handle ## fn ## property(PROPERTYDESC *pObj, DWORD dwOpCode, void *pValue, CBase *pObject, CVoid *pSubObject)\
        { return pObj->Handle ## fn ## Property(dwOpCode, pValue, pObject, pSubObject); }

DEFINE_PROPERTYDESC_METHOD(Num)      //  (DWORD dwOpCode, void * pValue, CBase * pObject, CVoid * pSubObject) const;
DEFINE_PROPERTYDESC_METHOD(String);  //  (DWORD dwOpCode, void * pValue, CBase * pObject, CVoid * pSubObject) const;
DEFINE_PROPERTYDESC_METHOD(Enum);    //  (DWORD dwOpCode, void * pValue, CBase * pObject, CVoid * pSubObject) const;
DEFINE_PROPERTYDESC_METHOD(Color);   //  (DWORD dwOpCode, void * pValue, CBase * pObject, CVoid * pSubObject) const;
//DEFINE_PROPERTYDESC_METHOD(Variant); //  (DWORD dwOpCode, void * pValue, CBase * pObject, CVoid * pSubObject) const;
DEFINE_PROPERTYDESC_METHOD(UnitValue);// (DWORD dwOpCode, void * pValue, CBase * pObject, CVoid * pSubObject) const;
DEFINE_PROPERTYDESC_METHOD(Style)    //  (DWORD dwOpCode, void * pValue, CBase * pObject, CVoid * pSubObject) const;
DEFINE_PROPERTYDESC_METHOD(StyleComponent) //  (DWORD dwOpCode, void * pValue, CBase * pObject, CVoid * pSubObject) const;
DEFINE_PROPERTYDESC_METHOD(Url)      //  (DWORD dwOpCode, void * pValue, CBase * pObject, CVoid * pSubObject) const;
DEFINE_PROPERTYDESC_METHOD(Code)     //  (DWORD dwOpCode, void * pValue, CBase * pObject, CVoid * pSubObject) const;

#endif
