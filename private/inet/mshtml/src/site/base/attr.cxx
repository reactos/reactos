//+---------------------------------------------------------------------------
//
//  Microsoft Forms³
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       attr.cxx
//
//  Contents:   CAttrValue, CAttrArray
//
//----------------------------------------------------------------------------

#include "headers.hxx"

MtDefine(CAttrArray, Elements, "CAttrArray")
MtDefine(CAttrArray_pv, CAttrArray, "CAttrArray::_pv")
MtDefine(CAttrValue_pch, CAttrArray, "CAttrValue::_varValue::_pch")
MtDefine(CAttrValue_dbl, CAttrArray, "CAttrValue::_pdblVal")
MtDefine(CAttrValue_var, CAttrArray, "CAttrValue::_pvarVal")
MtDefine(CAttrArrayHeader, CAttrArray, "CAttrValue::_pAAHeader")

//$WIN64: Win64 doesn't need to allocate for doubles since u2 is already 8 bytes wide

static const DWORD dwUnsetOther=0;
static const DWORD dwUnsetColor = VALUE_UNDEF;

#define DESTROY_DEFAULT 1

#if DESTROY_DEFAULT
static BOOL
IsDefaultValue ( const PROPERTYDESC *pPropertyDesc, VARIANT *pvt )
{
    DWORD dwValue;
    if ( V_VT(pvt)!=VT_I4)
    {
        return FALSE;
    }
    dwValue = (DWORD)pPropertyDesc->ulTagNotPresentDefault;
    return dwValue == (DWORD)V_I4(pvt) ? TRUE : FALSE;
}
#endif

//+------------------------------------------------------------------------
//
//  Member:     CAttrValue::Free
//
//  Synopsis:   Free's memory used by this AV
//
//-------------------------------------------------------------------------
void
CAttrValue::Free()
{
    switch (GetAVType())
    {
    case VT_LPWSTR:
        delete[] GetLPWSTR();
        break;

    case VT_BSTR:
        FormsFreeString(GetBSTR());
        break;

    case VT_ATTRARRAY:
        delete GetAA();
        break;

    case VT_R8:
        delete GetDoublePtr();
        break;

    case VT_UNKNOWN:
    case VT_DISPATCH:
        ReleaseInterface(GetpUnkVal());
        break;

    case VT_VARIANT:
        VariantClear(GetVariant());
        delete GetVariant();
        break;

    case VT_AAHEADER:
        delete GetAAHeader();
        break;
    }

    SetAVType(VT_EMPTY);
}

void CAttrValue::SetAAType(AATYPE aaType) 
{
    WORD extraBits = _wFlags._aaExtraBits & AA_Extra_Propdesc;

    if (aaType == AA_StyleAttribute)
    {
         extraBits |= AA_Extra_Style;
         aaType = AA_Attribute;
    }

    _wFlags._aaType = aaType;
    _wFlags._aaExtraBits = extraBits;
}


//+------------------------------------------------------------------------
//
//  Member:     CAttrValue::CompareWith
//
//  Synopsis:   Is this the attr value we want?
//
//				0 = exact match
//				1 = greater than
//			   -1 = less than
//
//-------------------------------------------------------------------------
int
CAttrValue::CompareWith ( DISPID dispID, AATYPE aaType )
{
    DISPID dispIdTmp = GetDISPID();

    if ( dispID < dispIdTmp )
        return -1;
    else if ( dispID > dispIdTmp )
        return 1;
    else
    {
        if ( aaType == AAType() )
            return 0;
        else if ( aaType < AAType() )
            return -1;
        else 
            return 1;
    }
}


//+------------------------------------------------------------------------
//
//  Member:     CAttrValue::Copy
//
//  Synopsis:   Make this AV the same as input AV
//
//-------------------------------------------------------------------------
HRESULT
CAttrValue::Copy(const CAttrValue *pAV)
{
    HRESULT hr = S_OK;
    VARIANT varNew;
    Assert(pAV);

    // Tidy up existing
    Free();

    Assert(sizeof(u1._pPropertyDesc) == sizeof(u1));
    u1._pPropertyDesc = pAV->u1._pPropertyDesc;
    _wFlags = pAV->_wFlags;

    // Copy in new
    pAV->GetAsVariantNC(&varNew);
    hr = THR(InitVariant(&varNew, TRUE));

    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CAttrValue::CopyEmpty
//
//  Synopsis:   Copy everything about input AV except the value
//
//-------------------------------------------------------------------------
void
CAttrValue::CopyEmpty(const CAttrValue *pAV)
{
    Assert(pAV);

    // Tidy up existing
    Free();

    Assert(sizeof(u1._pPropertyDesc) == sizeof(u1));
    u1._pPropertyDesc = pAV->u1._pPropertyDesc;
    _wFlags = pAV->_wFlags;

    // Set as empty
    SetAVType(VT_EMPTY);
}

HRESULT
CAttrValue::InitVariant (const VARIANT *pvarNew, BOOL fIsCloning)
{
    LPTSTR lpNew;
    LPTSTR lpAlloc;
    BSTR bstrVal = NULL;
    HRESULT hr = S_OK;
    VARTYPE avType = V_VT(pvarNew);

    SetAVType(VT_EMPTY);
    switch (avType)
    {
    case VT_LPWSTR:
        {
            lpNew = (LPTSTR)(V_BYREF(pvarNew));
            if (lpNew)
            {
                lpAlloc = new(Mt(CAttrValue_pch)) TCHAR [ _tcslen ( lpNew ) + 1 ];
                if (!lpAlloc)
                    hr = E_OUTOFMEMORY;
                else
                {
                    SetLPWSTR(lpAlloc);
                    MemSetName((GetLPWSTR(), "CAttrValue string"));
                    _tcscpy (lpAlloc, lpNew);
                }
            }
            else
                SetLPWSTR(NULL);
        }
        break;

    case VT_ATTRARRAY:
        if (fIsCloning)
        {
            // Make a complete Copy
            CAttrArray *pCloneFromAttrArray = (CAttrArray *)V_BYREF(pvarNew);
            if (pCloneFromAttrArray)
            {
                CAttrArray *pNewAttrArray = NULL;
                hr = THR(pCloneFromAttrArray->Clone(&pNewAttrArray));
                if (!hr)
                    SetAA(pNewAttrArray);
            }
            else
                SetAA(NULL);
        }
        else
        {
            // Just initialising
            SetAA((CAttrArray *)V_BYREF(pvarNew));
        }
        break;

    case VT_UNKNOWN:
    case VT_DISPATCH:
        if (V_UNKNOWN(pvarNew))
            V_UNKNOWN(pvarNew)->AddRef();
        // fall through to VT_PTR

    case VT_PTR:
        SetPointer(V_BYREF(pvarNew), avType);
        break;

    case VT_R4:
    case VT_I4:
        SetLong(V_I4(pvarNew), avType);
        break;

    case VT_I2:
    case VT_BOOL:
        SetShort(V_I2(pvarNew), avType);
        break;

    case VT_R8:
        hr = THR(SetDouble(V_R8(pvarNew)));
        break;

    case VT_BSTR:
        // BUGBUG(sramani): realloc here?
        hr = THR(FormsAllocString(V_BSTR(pvarNew), &bstrVal));
        if (hr)
            goto Cleanup;

        SetBSTR(bstrVal);
        break;

    case VT_EMPTY:
    case VT_NULL:
        SetAVType(avType);
        break;

    default:
        hr = THR(SetVariant(pvarNew));
        break;
    }

Cleanup:
    RRETURN (hr);
}

HRESULT
CAttrValue::SetDouble (double dblVal)
{
    HRESULT hr = S_OK;
    VARTYPE avType = VT_EMPTY;

    u2._pdblVal = new(Mt(CAttrValue_dbl)) double;
    if (!u2._pdblVal)
        hr = E_OUTOFMEMORY;
    else
    {
        *(u2._pdblVal) = dblVal;
        avType = VT_R8;
    }

    SetAVType(avType);
    return hr;
}

HRESULT
CAttrValue::SetVariant (const VARIANT *pvar)
{
    HRESULT hr = S_OK;
    VARTYPE avType = VT_EMPTY;

    u2._pvarVal = new(Mt(CAttrValue_var)) VARIANT;
    if (!u2._pvarVal)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }
    V_VT(u2._pvarVal) = VT_EMPTY;
    hr = VariantCopy(u2._pvarVal, (VARIANT *)pvar);
    if (hr)
        goto Cleanup;

    avType = VT_VARIANT;

Cleanup:
    if (hr && u2._pvarVal)
        delete u2._pvarVal;

    SetAVType(avType);
    return hr;
}

void
CAttrValue::GetAsVariantNC(VARIANT *pvar) const
{
  	VARTYPE avType = GetAVType();
    V_VT(pvar) = avType;
    switch(avType)
    {
    case VT_EMPTY:
    case VT_NULL:
        break;

    case VT_I2:
    case VT_BOOL:
        V_BOOL(pvar) = GetiVal();
        break;

    case VT_R4:
    case VT_I4:
        V_I4(pvar) = GetlVal();
        break;

    case VT_I8:
    case VT_R8:
        V_R8(pvar) = GetDouble();
        break;

    case VT_VARIANT:
        memcpy(pvar, GetVariant(), sizeof(VARIANT));
        break;

    default:
        V_BYREF(pvar) = GetPointerVal();
        break;
    }
}

//+------------------------------------------------------------------------
//
//  Member:     CAttrValue::GetIntoVariant
//
//  Synopsis:   Initialize the variant and copy the attribute value into it
//
//-------------------------------------------------------------------------
HRESULT
CAttrValue::GetIntoVariant (VARIANT *pvar) const
{
    HRESULT hr = S_OK;

  	V_VT(pvar) = VT_EMPTY;
    switch (GetAVType())
    {
    case VT_LPWSTR:
    case VT_BSTR:
        hr = THR(FormsAllocString(GetString(), &V_BSTR(pvar)));
        if (hr)
            goto Cleanup;

        V_VT(pvar) = VT_BSTR;
        break;

    case VT_UNKNOWN:
    case VT_DISPATCH:
        if (GetpUnkVal())
            GetpUnkVal()->AddRef();
        
        V_UNKNOWN(pvar) = GetpUnkVal();
      	V_VT(pvar) = GetAVType();
        break;

    case VT_VARIANT:
        pvar->vt = VT_EMPTY;
        hr = VariantCopy(pvar, GetVariant());
        break;

    default:
        GetAsVariantNC(pvar);
        break;
    }

Cleanup:
    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CAttrValue::GetIntoString
//
//  Synopsis:   Massage the AttrValue into a string - if we need to alloc
//              a BSTR for this, so be it - the caller is responsible for
//              freeing it.
//
//-------------------------------------------------------------------------
HRESULT
CAttrValue::GetIntoString(BSTR *pbStr, LPCTSTR *ppStr) const
{
    HRESULT hr = S_OK;

    Assert(pbStr);

    *pbStr = NULL;

    switch (GetAVType())
    {
    case VT_LPWSTR:
        *ppStr = GetLPWSTR();
        break;

    case VT_BSTR:
        *ppStr = (LPTSTR)GetBSTR();
        break;

    default:
        {
            VARIANT varDest;
            VARIANT varSrc;
            varDest.vt = VT_EMPTY;
            GetAsVariantNC(&varSrc);
            hr = THR(VariantChangeTypeSpecial(&varDest, &varSrc, VT_BSTR));
            if (hr)
            {
                if (hr == DISP_E_TYPEMISMATCH)
                    hr = S_FALSE;
                goto Cleanup;
            }
            *ppStr = (LPTSTR)V_BSTR(&varDest);
            *pbStr = V_BSTR(&varDest);
        }
        break;
    }
Cleanup:
    return hr;
}

//+------------------------------------------------------------------------
//
//  Member:     CAttrValue::Compare
//
//  Synopsis:   Compare 2 AttributeValues.  TRUE means they are the same
//              FALSE means they are different
//
//-------------------------------------------------------------------------
BOOL
CAttrValue::Compare(const CAttrValue *pAV) const
{
    Assert(pAV);

    if ( pAV -> GetDISPID() != GetDISPID() || pAV->AAType() != AAType() )
        return FALSE;

    switch ( GetAVType() )
    {
    case VT_DISPATCH:
    case VT_UNKNOWN:
        switch ( pAV->GetAVType() )
        {
            case VT_DISPATCH:
            case VT_UNKNOWN:
                {
                    LPUNKNOWN lpUnk1 = NULL;
                    LPUNKNOWN lpUnk2 = NULL;
                    HRESULT hr;
                    LPUNKNOWN lpUnk = (IUnknown *)pAV->GetpUnkVal();
                    hr  = lpUnk -> QueryInterface ( IID_IUnknown, (LPVOID *)&lpUnk1 );
                    if ( !hr )
                    {
                        lpUnk = (IUnknown *)GetpUnkVal();
                        hr  = lpUnk -> QueryInterface ( IID_IUnknown, (LPVOID *)&lpUnk2 );
                        if ( !hr )
                        {
                            if ( lpUnk1 == lpUnk2 )
                            {
                                ReleaseInterface ( lpUnk1 );
                                ReleaseInterface ( lpUnk2 );
                                return TRUE;
                            }
                        }
                    }
                    ReleaseInterface ( lpUnk1 );
                    ReleaseInterface ( lpUnk2 );
                }
                break;

            default:
                break;
        }
        break;

    case VT_BSTR:
        break;

    case VT_LPWSTR:
        switch ( pAV->GetAVType() )
        {
        case VT_LPWSTR:
            if ( GetLPWSTR() == NULL || pAV->GetLPWSTR() == NULL )
            {
                if ( GetLPWSTR() == NULL && pAV->GetLPWSTR() == NULL )
                {
                    return TRUE;
                }
                else
                {
                    return FALSE;
                }
            }
            else
            {
                // N.B. Need to confirm case sensitivity here.
                return StrCmpC( GetLPWSTR(), pAV->GetLPWSTR() ) == 0 ? TRUE : FALSE;
            }
            break;

        default:
            break;
        }
        break;

    case VT_ATTRARRAY:
        return GetAA()->Compare ( pAV->GetAA(), NULL );

    default:
        switch ( pAV->GetAVType() )
        {
        case VT_LPWSTR:
        case VT_BSTR:
        case VT_DISPATCH:
        case VT_UNKNOWN:
            break;

        default:
            if ( pAV->GetAVType() != GetAVType() )
                return FALSE;
            switch ( pAV->GetAVType() )
            {
            case VT_I4:
                return pAV->GetLong() == GetLong();
            case VT_I2:
                return pAV->GetShort() == GetShort();
            case VT_R4:
                return pAV->GetFloat() == GetFloat();
            case VT_R8:
                return pAV->GetDouble() == GetDouble();
            case VT_PTR:
                return pAV->GetPointer() == GetPointer();
            default:
                break;
            }
            break;
        }
        break;
    }
    return FALSE;
}


//+------------------------------------------------------------------------
//
//  CAttrArray code follows:
//
//-------------------------------------------------------------------------

CAttrArray::CAttrArray() : CDataAry<CAttrValue>(Mt(CAttrArray_pv))
{
    _dwChecksum = 0;
}


//+------------------------------------------------------------------------
//
//  Member:     CAttrValue::Clone()
//
//  Synopsis:   Clone this CAttrArray
//
//-------------------------------------------------------------------------
HRESULT
CAttrArray::Clone(CAttrArray **ppAA) const
{
    HRESULT hr = S_OK;
    DISPID dispid;
    const PROPERTYDESC *pSrcDesc;
    BOOL fCopyNextInternal = FALSE;

    int z,nCount;
    CAttrValue *pAV, *pAV2;

    Assert(ppAA);

    *ppAA = new CAttrArray;
    if (!*ppAA)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    MemSetName((*ppAA, "cloned CAttrArray:%s", MemGetName((void *)this)));

    hr = THR( (*ppAA)->EnsureSize(Size()));
    if (hr)
        goto Cleanup;

    for (pAV2 = (CAttrValue*) *(*ppAA),
        pAV = (CAttrValue*) *(CAttrArray *)this,
        z=0, nCount = 0; z<Size(); ++z, ++pAV)
    {
        // Don't copy internals
        if ((pAV->AAType() != CAttrValue::AA_Internal) || fCopyNextInternal)
        {
            // don't copy uniqueID
            dispid = pAV->GetDISPID();
            if (DISPID_CElement_uniqueName == dispid)
                continue;

            pAV2->SetAVType(VT_EMPTY);
            hr = THR(pAV2->Copy(pAV));
            if (hr)
                goto Cleanup;
            ++pAV2; nCount++;

            // if onfoo event property the dispatch ptr to script code will be the next one in AA, copy it too
            pSrcDesc = pAV->GetPropDesc();
            fCopyNextInternal = pSrcDesc && (pSrcDesc->GetPPFlags() & PROPPARAM_SCRIPTLET);

            // Internals don't add to the checksum, so skip it if we are going to
            // copy the next internal.
            if (fCopyNextInternal)
            {
                if (    z + 1 < Size() 
                    &&  (pAV+1)->GetDISPID() == dispid 
                    &&  (pAV+1)->AAType() == CAttrValue::AA_Internal)
                   continue;
                else
                {
                   fCopyNextInternal = FALSE;
                }
            }
    
            (*ppAA)->_dwChecksum += ((DWORD)dispid)<<1;
        }

    }
    (*ppAA)->SetSize(nCount);

Cleanup:
    if (hr)
    {
        delete *ppAA;
    }
    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CAttrValue::Merge()
//
//  Synopsis:   Merge this AttrArray in to the target.
//
//  Arguments:  ppAATarget      The target attr array to merge into
//              pTarget
//              pAAUndo         AttrArray to save old values
//              fFromUndo       if TRUE, merge *everything*
//              fCopyID         merge the name/id also
//
//-------------------------------------------------------------------------
HRESULT
CAttrArray::Merge(CAttrArray **ppAATarget, CBase *pTarget, CAttrArray *pAAUndo, BOOL fFromUndo, BOOL fCopyID) const
{
    HRESULT hr = S_OK;
    int z, nSizeTarget = 0, nCompare;
    BOOL fFind, fAlloced = FALSE, fCopyNextInternal = FALSE;
    CAttrValue *pAVSource, *pAVTarget, *pAVTargetBase;
    AAINDEX aaIndex;
    const PROPERTYDESC *pTargetDesc;
    const PROPERTYDESC *pSrcDesc;
    const TCHAR *pchName = NULL;
    DISPID dispid;

    Assert(ppAATarget);

    if ( !(*ppAATarget) )
    {
        *ppAATarget = new CAttrArray;
        if (!*ppAATarget)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }
        fAlloced = TRUE;
    }

    nSizeTarget = (*ppAATarget)->Size();

    // Only look for existing attr values if there are any!
    fFind = nSizeTarget > 0;

    MemSetName((*ppAATarget, "merged CAttrArray:%s", MemGetName((void *)this)));

    hr = THR((*ppAATarget)->EnsureSize(Size()+nSizeTarget));
    if (hr)
        goto Cleanup;

    pAVTarget = pAVTargetBase = (CAttrValue*) *(*ppAATarget);

    for (pAVSource = (CAttrValue*) *(CAttrArray *)this,
         z=0 ; z<Size(); ++z, ++pAVSource)
    {
        dispid = pAVSource->GetDISPID();

        // don't copy id or name or uniqueID
        if (    !fFromUndo
            &&  (   !fCopyID 
                &&  (   (DISPID_CElement_id == dispid) 
                    ||  (STDPROPID_XOBJ_NAME == dispid)) 
                ||  (DISPID_CElement_uniqueName == dispid)))
            continue;

        // Don't copy internals, nested AAs, attached events or expressions
        if (    fFromUndo
            ||  (   pAVSource->AAType() != CAttrValue::AA_Internal
                &&  pAVSource->AAType() != CAttrValue::AA_AttrArray
                &&  pAVSource->AAType() != CAttrValue::AA_AttachEvent
                &&  pAVSource->AAType() != CAttrValue::AA_Expression ) )
        {
            Assert(fFromUndo ||
                   pAVSource->AAType() == CAttrValue::AA_Attribute ||
                   pAVSource->AAType() == CAttrValue::AA_Expando ||
                   pAVSource->IsStyleAttribute() || 
                   pAVSource->AAType() == CAttrValue::AA_UnknownAttr);

NEXT:
            // if at end of target AA, nothing left to compare, use common code path for nComape == -1
            fFind = fFind && ((pAVTarget - pAVTargetBase) < nSizeTarget);
            // if non-empty target, does src prop exist in target AA?
            nCompare = (fFind) ? pAVTarget->CompareWith(dispid, pAVSource->AAType()) : -1;
            switch (nCompare)  // yes
            {
            case  0: // yes, found one
            case -1: // no, src < target, or at end of target, time to insert appropriately into target
                    
                    pSrcDesc = pAVSource->GetPropDesc();

                    // if src and target elem are of same type OR src prop exists in target AA,
                    // just copy the new value, else ...
                    if (pTarget && nCompare && !fFromUndo)
                    {
                        // ... if not in common attribute range or not an expando\unknown, skip
                        if ((pAVSource->AAType() != CAttrValue::AA_Expando) &&
                            (pAVSource->AAType() != CAttrValue::AA_UnknownAttr) && 
                            (dispid >= 0))
                            break;

                        // ... does src prop have a propdesc ?
                        if (pSrcDesc)
                        {
                            // Yes, check to see if target supports prop name
                            pchName = pSrcDesc->pstrName;
                            pTargetDesc = pTarget->FindPropDescForName(pchName);
                    
                            // skip this src prop if only name is same in target, but not the whole property
                            if (!pTargetDesc ||
                                ((pSrcDesc != pTargetDesc) && 
                                 (pSrcDesc->GetDispid() != pTargetDesc->GetDispid())))
                                break;
                        }
                        // No, just copy (could be expando, unknwon attr or name\id)
                    }

COPYNEXTINTERNAL:                    
                    // destination element supports the property or it is an expando on the src,
                    // just insert new value if target is non-empty and src prop doesn't exist in it.
                    if (fFind && nCompare)
                    {
                        CAttrValue  tempAV;

                        // Setup CAttrValue to empty value so Copy won't free.
                        tempAV.SetAVType(VT_EMPTY);

                        // Need to allocate the value of the expando.
                        hr = THR(tempAV.Copy(pAVSource));

                        // get insertion point
                        aaIndex = pAVTarget - pAVTargetBase;
                        // if expando prop, it will already be in the doc's atom table, so don't bother adding again.
                        hr = THR((*ppAATarget)->InsertIndirect(aaIndex, &tempAV));

                        // Set back to empty, as owner is now ppAATarget, not tempAV.
                        tempAV.SetAVType(VT_EMPTY);
                    }
                    else
                    {
                        // empty target, or target at end of AA, or non-empty target with src prop value existing
                        // in it; just copy as src is already sorted 
                        
                        // if src has an expando, it will already be in the doc's atom table, so don't bother
                        // adding again.

                        // Need to set this here as pAVTarget always points to new, not necessarily zeroed out memory,
                        // so, need to protect from crashing in Free inside Copy.
                        if (nCompare)
                            pAVTarget->SetAVType(VT_EMPTY);

                        // For Undo, save the old value.
                        if (pAAUndo && !nCompare)
                        {
                            HRESULT     hrUndo;

                            hrUndo = THR( pAAUndo->AppendIndirect( pAVTarget ) );

                            // Undo AttrArray owns this now
                            pAVTarget->SetAVType(VT_EMPTY);
                        }

                        // The next CAV in target had better be the script dispatch code
                        Assert(nCompare ||
                               !fCopyNextInternal || 
                               ((pAVTarget->GetDISPID() == dispid) && (pAVTarget->AAType() == CAttrValue::AA_Internal)));

                        // If we are comming from undo and we have an exact match
                        // and our source has AVType == VT_EMPTY, remove the entry
                        // from the target array
                        if (fFromUndo && !nCompare && pAVSource->GetAVType() == VT_EMPTY)
                        {
                            // get insertion point
                            aaIndex = pAVTarget - pAVTargetBase;

                            // Fix up the checksum for the attr array
                            if (pAVTarget->AAType() != CAttrValue::AA_Internal)
                                (*ppAATarget)->_dwChecksum -= ((DWORD)pAVTarget->GetDISPID())<<1;

                            pAVTarget->Free();

                            // Delete will not realloc the array
                            (*ppAATarget)->Delete( aaIndex );

                            // The target array is one smaller
                            nSizeTarget--;

                            // Back off pAVTarget so we can move it forward below
                            pAVTarget--;
                        }
                        else
                        {
                            hr = THR(pAVTarget->Copy(pAVSource));
                        }
                    }

                    if (hr)
                        goto Cleanup;

                    if (nCompare)
                    {
                        if (pAVSource->AAType() != CAttrValue::AA_Internal)
                            (*ppAATarget)->_dwChecksum += ((DWORD)dispid)<<1;
                        nSizeTarget++;
                        (*ppAATarget)->SetSize(nSizeTarget);

                        // For Undo, store a VT_EMPTY to mark that we must
                        // remove this attr value
                        if (pAAUndo)
                        {
                            HRESULT     hrUndo;
                            CAttrValue *pAVUndo;

                            hrUndo = THR( pAAUndo->AppendIndirect( NULL, &pAVUndo ) );
                            if (!hrUndo)
                            {
                                pAVUndo->SetAVType( VT_EMPTY );
                                pAVUndo->CopyEmpty( pAVSource );
                            }
                        }
                    }
                    pAVTarget++;

                    // if onfoo event property the dispatch ptr to script code will be the next one in AA, copy it too
                    fCopyNextInternal = pSrcDesc && (pSrcDesc->GetPPFlags() & PROPPARAM_SCRIPTLET);
                    if (fCopyNextInternal)
                    {
                        pSrcDesc = NULL;
                        pAVSource++; z++;
                        if (    z < Size() 
                            &&  pAVSource->GetDISPID() == dispid 
                            &&  pAVSource->AAType() == CAttrValue::AA_Internal)
                           goto COPYNEXTINTERNAL;
                    }

                    break;

            case 1: // no, src > target, continue

                    pAVTarget++;
                    goto NEXT;
            }
        }
    }

Cleanup:

    if (hr && fAlloced)
    {
        delete *ppAATarget;
        *ppAATarget = NULL;
    }
    else if (*ppAATarget)
    {
        Assert((*ppAATarget)->Size() <= (nSizeTarget));
        hr = THR((*ppAATarget)->Grow(nSizeTarget));
    }

    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CAttrValue::CopyExpandos
//
//  Synopsis:   Add expandos of given attrarray to current attrarray
//
//-------------------------------------------------------------------------

HRESULT
CAttrArray::CopyExpandos(CAttrArray *pAA)
{
    HRESULT      hr = S_OK;
    CAttrValue * pAV2;

    Assert(pAA);

    pAV2 = (CAttrValue*) *pAA;

    for (int i = 0; i < pAA->Size(); i++, pAV2++)
    {
        // Copy all expandos
        if ( pAV2->AAType() == CAttrValue::AA_Expando)
        {
            VARIANT varVal;
            pAV2->GetAsVariantNC(&varVal);
            hr = THR(Set(pAV2->GetDISPID(), NULL, &varVal, CAttrValue::AA_Expando));
            if(hr)
                break;
        }
    }

    RRETURN(hr);
}



//+------------------------------------------------------------------------
//
//  Member:     CAttrArray::Compare
//
//  Synopsis:   We are already sorted, and have cache cookies!
//              TRUE means same, FALSE means different
//
//-------------------------------------------------------------------------

BOOL
CAttrArray::Compare ( CAttrArray * pAAThat, DISPID * pdispIDOneDifferent ) const
{
    int nThisSize = this ? Size() : 0;
    int nThatSize = pAAThat ? pAAThat->Size() : 0;
    const CAttrValue * pAVThis, * pAVThat;
    BOOL fFoundOneAttrValueDifferent = FALSE;

    //
    // If both arrays are empty, then they are the same.
    //

    if (nThatSize == 0 && nThisSize == 0)
        return TRUE;

    //
    // A quick check of the CRC's for each array will immediatly tell
    // us if thay are different (but no the same).  Note that if we also
    // want to know if one attr is differnt we can't do this quick check.
    //

    if (!pdispIDOneDifferent && pAAThat->GetChecksum() != this->GetChecksum())
        return FALSE;

    DISPID pdispIDDummy;
    DISPID & dispIDOneDifferent =
        pdispIDOneDifferent ? * pdispIDOneDifferent : pdispIDDummy;
    
    dispIDOneDifferent = 0;

    pAVThis = this    ? * (CAttrArray *)this    : (const CAttrValue *) NULL;
    pAVThat = pAAThat ? * pAAThat : (const CAttrValue *) NULL;

    for ( ; ; )
    {
        //
        // Skip all internal attrs
        //
        
        while (nThisSize && pAVThis->AAType() == CAttrValue::AA_Internal)
            nThisSize--, pAVThis++;

        while (nThatSize && pAVThat->AAType() == CAttrValue::AA_Internal)
            nThatSize--, pAVThat++;

        //
        // Have we reached the end of one of the arrays?
        //

        if (nThisSize == 0 || nThatSize == 0)
        {
            if (nThisSize != nThatSize)
                dispIDOneDifferent = 0;
            
            return nThisSize == nThatSize && !fFoundOneAttrValueDifferent;
        }

        //
        // We now have the next non-internal attr for each array, compare them
        //

        if (pAVThat->GetDISPID() != pAVThis->GetDISPID())
        {
            dispIDOneDifferent = 0;
            return FALSE;
        }

        //
        // See if the attr value is different
        //

        if (!pAVThis->Compare( pAVThat ))
        {
            if (fFoundOneAttrValueDifferent)
                dispIDOneDifferent = 0;
                
            if (!pdispIDOneDifferent || fFoundOneAttrValueDifferent)
                return FALSE;

            fFoundOneAttrValueDifferent = TRUE;
            dispIDOneDifferent = pAVThis->GetDISPID();
        }

        //
        // One to the next attr ...
        //

        pAVThis++, nThisSize--;
        pAVThat++, nThatSize--;
    }
}



//+------------------------------------------------------------------------
//
//  Member:     CAttrArray::HasAnyAttribute
//
//  Synopsis:   Returns TRUE when the AttrArray contains an attribute
//              or an expando (if the flag is set)
//
//-------------------------------------------------------------------------

BOOL 
CAttrArray::HasAnyAttribute(BOOL fCountExpandosToo)
{
    CAttrValue * pAV;
    CAttrValue::AATYPE aaType;

    for(int i = 0; i < Size(); i++)
    {
        pAV = ((CAttrValue *)*this) + i;
        aaType = pAV->AAType();
        if(  aaType == CAttrValue::AA_Attribute ||
            (fCountExpandosToo && aaType == CAttrValue::AA_Expando) )
            return TRUE;
    }

    return FALSE;
}


void
CAttrArray::Clear(CAttrArray * pAAUndo)
{
    long        i;
    CAttrValue *pAV;
    DISPID      dispid;
    HRESULT     hrUndo;

    _dwChecksum = _dwChecksum & 1;

    for (i = 0, pAV = (CAttrValue *)*this; i < Size(); pAV = ((CAttrValue *)*this) + i)
    {
        if (pAV->AAType() == CAttrValue::AA_Internal)
        {
            i++;
            continue;
        }

        // preserve identity.
        dispid = pAV->GetDISPID();
        if ((DISPID_CElement_id == dispid) ||
            (STDPROPID_XOBJ_NAME == dispid) ||
            (DISPID_CElement_uniqueName == dispid))
        {
            i++;
            _dwChecksum += ((DWORD)dispid)<<1;
            continue;
        }

        hrUndo = S_OK;

        if (pAAUndo)
            hrUndo = THR( pAAUndo->AppendIndirect( pAV ) );

        if (!pAAUndo || hrUndo)
            pAV->Free();

        Delete(i);
    }
}

//+------------------------------------------------------------------------
//
//  Member:     CAttrArray::Free
//
//  Synopsis:   Frees all members
//
//-------------------------------------------------------------------------

void
CAttrArray::Free()
{
    long        i;
    CAttrValue *pAV;

    _dwChecksum = 0;

    for (i = Size(), pAV = (CAttrValue *)*this; i > 0; i--, pAV++)
    {
        pAV->Free();
    }
    DeleteAll();
}


//+------------------------------------------------------------------------
//
//  Member:     CAttrArray::FreeSpecial
//
//  Synopsis:   Frees all members except so-called members with special 
//              dispids.  This exclusive list consists of the dispids
//              we reserve for special purposes (like event sinks and
//              prop notify sinks).
//
//  Notes:      This type of Free is called when the doc unloads.
//
//-------------------------------------------------------------------------

void
CAttrArray::FreeSpecial()
{
    long            i;
    CAttrValue *    pAV;

    _dwChecksum = 0;
    SetGINCache(DISPID_UNKNOWN, NULL, FALSE);

    //
    // This complex loop is needed because we're deleting out of the
    // array while iterating over it.
    //
    
    // BUGBUG rgardner - need to localize this hackery!!
    for (i = 0, pAV = (CAttrValue *)*this; 
         i < Size(); 
         pAV = ((CAttrValue *)*this) + i)
    {
        if (pAV->GetDISPID() != DISPID_A_PROPNOTIFYSINK &&
            pAV->GetDISPID() != DISPID_A_EVENTSINK &&
            pAV->GetDISPID() != DISPID_AAHEADER &&
            pAV->GetDISPID() != DISPID_INTERNAL_INVOKECONTEXT )
        {
            pAV->Free();
            Delete(i);
        }
        else
        {
            i++;

            if (pAV->AAType() != CAttrValue::AA_Internal)
                _dwChecksum += (pAV->GetDISPID())<<1;
        }
    }
}


//+------------------------------------------------------------------------
//
//  Member:     CAttrArray::Find
//
//  Synopsis:   Find CAttrValue given _dispID
//
//              The paaIdx is the index to start searching at if this value
//              is AA_IDX_UNKNOWN, then find first occurance.  If something
//              else then find the next occurance beyond AAINDEX which matches
//              thedispID.  The index of the CAttrValue returned is set in
//              paaIdx (this pointer can be NULL, if so then Find will always
//              return the first occurance of thedispID).
//
//-------------------------------------------------------------------------

CAttrValue*
CAttrArray::Find(
    DISPID              thedispID, 
    CAttrValue::AATYPE  aaType, 
    AAINDEX *           paaIdx,
    BOOL                fAllowMultiple)
{
    CAttrValue *    pAVBase;
    CAttrValue *    pAV;
    AAINDEX         aaIndx;
    
    if ( aaType == CAttrValue::AA_StyleAttribute )
    {
        aaType = CAttrValue::AA_Attribute;
    }

    if (paaIdx == NULL)
    {
        aaIndx = AA_IDX_UNKNOWN;
        paaIdx = &aaIndx;
    }

    // If thedispID is DISPID_UNKNOWN and *paaIdx is AA_IDX_UNKNOWN then
    // we're looking for the first occurance of the aaType.  Which we can
    // search linearly.

    // Yes, find first occurance.
    if (thedispID == DISPID_UNKNOWN)
    {
        int nCount;
        if ( !Size() )
            goto NotFound;

        // Linear search
        for ( nCount = *paaIdx + 1, pAV = ((CAttrValue*)*this) + nCount;
              nCount < Size();
              nCount++, pAV++ )
        {
            // We check with a bitwise & on the aatype so that the style and
            // attribute types are picked up
            if ( pAV->AAType() == aaType )
            {
                *paaIdx = nCount;
                thedispID = pAV->GetDISPID();
                goto UpdateCache;
            }
        }

NotFound:
        *paaIdx = AA_IDX_UNKNOWN;
    }
    else
    {
        int  nPos, nMin, nMax, nCompare;

        Assert(*paaIdx == AA_IDX_UNKNOWN);
        
        // Binary search
        pAVBase = (CAttrValue*)*this;

        //
        // Check cache first, but only if not accounting for multiple
        // entries of the same dispid.
        //
        
        for ( nPos = 0, nCompare = 0, nMin = 0, nMax = Size(); nMin < nMax ; )
        {
            nPos = (nMax + nMin) / 2;
            pAV = pAVBase + nPos;

            // Check sort order based on (DISPID,AATYPE) pair
            nCompare = pAV->CompareWith ( thedispID, aaType );

            if ( nCompare == 0 )
            {
                *paaIdx = pAV - pAVBase;
                goto UpdateCache;
            }
            else if ( nCompare < 0 )
            {
                nMax = nPos;
            }
            else
            {
                nMin = nPos + 1;
            }
        }
        // Calculate the insertion point for a new (DISPID,AATYPE)
        if ( nCompare == +1 )
        {
            *paaIdx = (AAINDEX)++nPos;        
        }
        else
        {
            *paaIdx = (AAINDEX)nPos;
        }
    }
    // Didn't find it
    return NULL;

UpdateCache:
    Assert(thedispID != DISPID_UNKNOWN && *paaIdx != AA_IDX_UNKNOWN);

    //
    // If multiple entries for the same dispid are allowed, then 
    // return the very first one.
    //
    
    if (fAllowMultiple)
    {

        ULONG   ulPos;
        
        for (ulPos = *paaIdx, pAV = ((CAttrValue*)*this) + ulPos;
             ;
             ulPos--, pAV--)
        {
            //
            // Catch the case when we're iterating down and we  
            // extend beyond the beginning or a change in dispid occurs  
            // AND we need to return the first instance of the dispid.
            //
            
            if (ulPos == AA_IDX_UNKNOWN || 
                pAV->GetDISPID() != thedispID ||
                pAV->AAType() != aaType ||
                ulPos >= (ULONG)Size())
            {
                ulPos++;
                pAV++;
                break;
            }
        }

        *paaIdx = (AAINDEX)ulPos;
    }
    
    return pAV;
}


BOOL
CAttrArray::FindString ( CAttrArray *pAA, const PROPERTYDESC *pPropertyDesc, LPCTSTR *ppStr )
{
    if ( pAA && pAA->FindString ( pPropertyDesc->GetDispid(), ppStr, CAttrValue::AA_Attribute ) )
        return TRUE;

    *ppStr = (LPTSTR)pPropertyDesc->ulTagNotPresentDefault;
    return FALSE;
}

BOOL
CAttrArray::FindString ( DISPID dispID, LPCTSTR *ppStr,
    CAttrValue::AATYPE aaType  ,
    const PROPERTYDESC **ppPropertyDesc )
{
    Assert(ppStr);

    CAttrValue *pAV = Find ( dispID, aaType );
    if ( pAV )
    {
        *ppStr = pAV->GetLPWSTR();
        if ( ppPropertyDesc )
            *ppPropertyDesc = pAV->GetPropDesc();
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}


BOOL
CAttrArray::FindSimple ( CAttrArray *pAA, const PROPERTYDESC *pPropertyDesc, DWORD *pdwValue )
{
    if ( pAA && pAA->FindSimple ( pPropertyDesc->GetDispid(), pdwValue, CAttrValue::AA_Attribute ) )
        return TRUE;

    *pdwValue = (DWORD)pPropertyDesc->ulTagNotPresentDefault;
    return FALSE;
}

BOOL
CAttrArray::FindSimple ( DISPID dispID, DWORD *pdwValue,
    CAttrValue::AATYPE aaType ,
    const PROPERTYDESC **ppPropertyDesc )
{
    Assert(pdwValue);

    CAttrValue *pAV = Find ( dispID, aaType );
    if ( pAV )
    {
        *pdwValue = (DWORD)pAV->GetLong();
        if ( ppPropertyDesc )
            *ppPropertyDesc = pAV->GetPropDesc();
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

BOOL
CAttrArray::FindSimpleInt4AndDelete( DISPID dispID, DWORD *pdwValue,
    CAttrValue::AATYPE aaType ,
    const PROPERTYDESC **ppPropertyDesc )
{
    AAINDEX aaIdx = AA_IDX_UNKNOWN;
    Assert(pdwValue);

    CAttrValue *pAV = Find ( dispID, aaType, &aaIdx );
    if ( pAV )
    {
        *pdwValue = (DWORD)pAV->GetLong();
        if ( ppPropertyDesc )
            *ppPropertyDesc = pAV->GetPropDesc();
        Destroy( aaIdx );
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

BOOL
CAttrArray::FindSimpleAndDelete( DISPID dispID,
    CAttrValue::AATYPE aaType ,
    const PROPERTYDESC **ppPropertyDesc )
{
    AAINDEX aaIdx = AA_IDX_UNKNOWN;

    CAttrValue *pAV = Find ( dispID, aaType, &aaIdx );
    if ( pAV )
    {
        Assert ( pAV->GetAVType() == VT_LPWSTR );
        if ( ppPropertyDesc )
            *ppPropertyDesc = pAV->GetPropDesc();
        Destroy( aaIdx );
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}


HRESULT
CAttrArray::SetAt (AAINDEX aaIdx, VARIANT *pvarNew)
{
    CAttrValue *pAV = *this;
    HRESULT hr = E_FAIL;

    if (aaIdx != AA_IDX_UNKNOWN && aaIdx < ULONG(Size()))
    {
        // Change an existing one
        // CRC doesn't change cos we didn't add a new dispid
        (pAV + aaIdx)->Free();

        // Copy in new VARIANT
        hr = THR((pAV + aaIdx)->InitVariant ( pvarNew ));
    }

    RRETURN(hr);
}



HRESULT
CAttrArray::Set (
    DISPID dispID,
    const PROPERTYDESC *pProp,
    VARIANT *pvarNew,
    CAttrValue::AATYPE aaType,
    WORD wFlags,
    BOOL fAllowMultiple)
{
    // For changing non-unique values the SetAt function must be used. This Set
    // routine allows for adding both unique and non-unique and changing unique
    // only values.
    HRESULT hr = S_OK;
    CAttrValue avNew;
    CAttrValue *pAV;
    AAINDEX aaIdx = AA_IDX_UNKNOWN;

    Assert ( pvarNew );
    Assert ( dispID != DISPID_UNKNOWN );
    Assert ((pProp && (pProp->GetDispid() == dispID)) || !pProp);

    // If we don't find the attribute, aaIdx will be set to the insertion point
    pAV = Find(dispID, aaType, &aaIdx ) ;

    if ((aaType == CAttrValue::AA_UnknownAttr) &&
        (wFlags & CAttrValue::AA_Extra_DefaultValue) &&
        !pAV)
    {
        Assert(aaIdx != AA_IDX_UNKNOWN);
        CAttrValue *pAVPrev = FindAt(aaIdx-1);
        // BUGBUG(sramani): when (& if) DESTROY_DEFAULT is turned off these if
        // conditions can be turned into asserts.
        if (pAVPrev &&
            pAVPrev->GetDISPID() == dispID &&
            pAVPrev->AAType() == CAttrValue::AA_Attribute)
            pAVPrev->SetDefault(TRUE);
    }

    // Add a new one if not found or if we don't want a unique one.  If a multiple
    // one needs to be change then use the CBase helpers ChangeAt functions.
    if (!pAV || fAllowMultiple)
    {
#if DESTROY_DEFAULT // explicitly store default values
        // Need a new one
        if ( pProp && IsDefaultValue ( pProp, pvarNew )  )
        {
            // Don't bother
            goto Cleanup;
        }
#endif
        avNew.SetAAType ( aaType );

        // if the AAType warrants a propdesc, but pProp == NULL,
        // as might be the case when storing a CFuncPtr *, store in _dispid
        if (pProp)
            avNew.SetPropDesc(pProp);
        else
            avNew.SetDISPID(dispID);

        hr = THR ( avNew.InitVariant ( pvarNew ) );
        if ( hr )
            goto Cleanup;

        avNew.SetImportant( wFlags & CAttrValue::AA_Extra_Important );
        avNew.SetImplied( wFlags & CAttrValue::AA_Extra_Implied );
        avNew.SetTridentSink( wFlags & CAttrValue::AA_Extra_TridentEvent );
//      ***TLL*** Always set oldEventStyle for VID compatibility.  If should enable below
//                and displace the SetOldEventStyle(TRUE) when we support new event style
//                with eventObject as a parameter to the event.
//      avNew.SetOldEventStyle( wFlags & CAttrValue::AA_Extra_OldEventStyle );
        avNew.SetOldEventStyle( TRUE );
        avNew.SetExpression( FALSE );

        hr = THR(InsertIndirect( aaIdx, &avNew ));
        if ( hr )
            goto Cleanup;

        if(aaType != CAttrValue::AA_Internal)
            _dwChecksum += ((DWORD)dispID)<<1;
        else if (dispID == DISPID_INTERNAL_CSTYLEPTRCACHE || dispID == DISPID_INTERNAL_CRUNTIMESTYLEPTRCACHE)
            SetStylePtrCachePossible();
    }
    else
    {
#if DESTROY_DEFAULT // explicitly store default values
        // Change an existing one
        if ( pProp && IsDefaultValue ( pProp, pvarNew )  )
        {
            // We're setting the attribute to its default value -
            // remove it from the attr array
            Destroy ( pAV-((CAttrValue*)*this) );
            goto Cleanup;
        }
#endif
        // Copy in new VARIANT
        pAV->Free();
        pAV->SetAAType ( aaType );
        pAV->SetImportant( wFlags & CAttrValue::AA_Extra_Important );
        pAV->SetImplied( wFlags & CAttrValue::AA_Extra_Implied );
        pAV->SetTridentSink( wFlags & CAttrValue::AA_Extra_TridentEvent );
//      ***TLL*** Always set oldEventStyle for VID compatibility.  If should enable below
//                and displace the SetOldEventStyle(TRUE) when we support new event style
//                with eventObject as a parameter to the event.
//      pAV->SetOldEventStyle( wFlags & CAttrValue::AA_Extra_OldEventStyle );
        pAV->SetOldEventStyle( TRUE );
        pAV->SetExpression(FALSE);
        hr = THR(pAV->InitVariant ( pvarNew ));
    }

Cleanup:
    RRETURN(hr);
}


/* static */
HRESULT
CAttrArray::Set(
    CAttrArray **ppAA,
    DISPID dispID,
    VARIANT *pvarNew,
    const PROPERTYDESC *pProp /* = NULL */,
    CAttrValue::AATYPE aaType /* = CAttrValue::AA_Attribute */,
    WORD wFlags,
    BOOL fAllowMultiple)
{
    HRESULT hr = S_OK;

    Assert(ppAA);
    if (!*ppAA)
    {
        *ppAA = new CAttrArray;
        if (!*ppAA)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }
    }
    hr = THR((*ppAA)->Set ( dispID, pProp, pvarNew, aaType, wFlags, fAllowMultiple));
Cleanup:
    RRETURN(hr);
}


//+------------------------------------------------------------------------
//
//  Member:     CAttrArray::Destroy
//
//  Synopsis:   Destroys attribute referenced by supplied index
//
//-------------------------------------------------------------------------
void
CAttrArray::Destroy(int iIndex)
{
    CAttrValue *pAV;

    Assert(iIndex>=0);
    Assert(iIndex<Size());

    pAV = ((CAttrValue*) *this) + iIndex;

    // Remove DISPID from checksum
    if (pAV->AAType() != CAttrValue::AA_Internal)
        _dwChecksum -= ((DWORD)pAV->GetDISPID())<<1;

    pAV->Free();
    Delete(iIndex);
}


//+------------------------------------------------------------------------
//
//  Member:     CAttrArray::SetSimple
//
//  Synopsis:   Static function that lets you store a simple types into
//              an AttrArray.  This creates both the AttrArray and AttrValue
//              if needed.  The caller still needs to call consolidate, as
//              the insertion may have messed up the sort order.
//
//-------------------------------------------------------------------------
HRESULT
CAttrArray::SetSimple(CAttrArray **ppAA, const PROPERTYDESC *pPropertyDesc, DWORD dwSimple, WORD wFlags )
{
    VARIANT varNew;
    DWORD dwPPFlags = pPropertyDesc->GetPPFlags();
    CAttrValue::AATYPE aaType;

    varNew.vt = VT_I4;
    varNew.lVal = (long)dwSimple;

    if ( dwPPFlags & PROPPARAM_STYLISTIC_PROPERTY )
    {
        aaType = CAttrValue::AA_StyleAttribute;
    }
    else
    {
        aaType = CAttrValue::AA_Attribute;
    }

    RRETURN ( CAttrArray::Set (ppAA, pPropertyDesc->GetDispid(), &varNew, pPropertyDesc, aaType, wFlags ) );
}

//+------------------------------------------------------------------------
//
//  Member:     CAttrArray::SetString
//
//  Synopsis:   Static function that lets you store a TCHAR* into
//              an AttrArray.  This creates both the AttrArray and AttrValue
//              if needed.  The caller still needs to call consolidate, as
//              the insertion may have messed up the sort order and has
//              not converted the input to a cookie
//
//-------------------------------------------------------------------------
HRESULT
CAttrArray::SetString(CAttrArray **ppAA, const PROPERTYDESC *pPropertyDesc, LPCTSTR pch,
    BOOL fIsUnknown /* = FALSE */, WORD wFlags /*=0*/ )
{
    VARIANT varNew;
    DWORD dwPPFlags = pPropertyDesc->GetPPFlags();
    CAttrValue::AATYPE aaType;

    varNew.vt = VT_LPWSTR;
    varNew.byref = (LPVOID)pch;

    if ( fIsUnknown )
    {
        aaType = CAttrValue::AA_UnknownAttr;
    }
    else
    {
        if ( dwPPFlags & PROPPARAM_STYLISTIC_PROPERTY )
        {
            aaType = CAttrValue::AA_StyleAttribute;
        }
        else
        {
            aaType = CAttrValue::AA_Attribute;
        }
    }
    RRETURN ( CAttrArray::Set (ppAA, pPropertyDesc->GetDispid(), &varNew, pPropertyDesc, aaType, wFlags ) );
}



HRESULT
CAttrArray::GetSimpleAt(AAINDEX aaIdx, DWORD *pdwValue)
{
    HRESULT             hr = DISP_E_MEMBERNOTFOUND;
    const CAttrValue   *pAV;

    Assert(pdwValue);
    Assert(this);

    // Any attr array?
    pAV = FindAt(aaIdx);
    // Found AttrValue?
    if (pAV)
    {
        *pdwValue = (DWORD)pAV->GetLong();
        hr = S_OK;
    }

    RRETURN(hr);
}

AAINDEX
CAttrArray::FindAAIndex(
    DISPID              dispID, 
    CAttrValue::AATYPE  aaType, 
    AAINDEX             aaLastOne,
    BOOL                fAllowMultiple)
{
    return (Find(dispID, aaType, &aaLastOne, fAllowMultiple)) ? 
        aaLastOne : 
        AA_IDX_UNKNOWN;
}




CAttrValue *
CAttrArray::EnsureHeader(BOOL fCreate)
{
    CAttrValue *pAV = (CAttrValue*)*this;

    if (Size() && (pAV->GetAVType() == CAttrValue::VT_AAHEADER))
        return pAV;

    if (fCreate)
    {
        if (SetHeader())
            goto Cleanup;

        return (CAttrValue*)*this;
    }

Cleanup:
    return NULL;
}

HRESULT
CAttrArray::SetHeader()
{
    HRESULT hr = S_OK;
    CAttrValue avNew;
    CAttrArrayHeader *pAAHeader = new CAttrArrayHeader;
    if (!pAAHeader)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    avNew.SetDISPID(DISPID_AAHEADER);
#ifdef _MAC // bugbug: can this be changed to work without an ifdef
    avNew.SetAAType(CAttrValue::AA_Internal);
#else
    avNew.SetAAType(CAttrValue::AATYPE::AA_Internal);
#endif
    avNew.SetAAHeader(pAAHeader);
    avNew.SetCachedDispid(DISPID_UNKNOWN);
    avNew.SetCachedVTblDesc(NULL);
    avNew.SetEventsToHook(NULL);

    hr = THR(InsertIndirect(0, &avNew));

Cleanup:
    if (hr && pAAHeader)
        delete pAAHeader;

    return hr;
}

DISPID CAttrArray::GetCachedDispidGIN()
{
    CAttrValue *pAV = EnsureHeader(FALSE);
    return pAV ? pAV->GetCachedDispid() : DISPID_UNKNOWN;
}

const VTABLEDESC *CAttrArray::GetCachedVTableDesc()
{
    CAttrValue *pAV = EnsureHeader(FALSE);
    return pAV ? pAV->GetCachedVTblDesc() : NULL;
}

void CAttrArray::SetGINCache(DISPID dispid, const VTABLEDESC *pVTblDesc, BOOL fCreate)
{
    CAttrValue *pAV = EnsureHeader(fCreate);
    if (!pAV)
        return;

    Assert(pAV->GetAAHeader());
    pAV->SetCachedDispid(dispid);
    pAV->SetCachedVTblDesc(pVTblDesc);
}
