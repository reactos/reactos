//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1994-1996
//
//  File:       propbag.cxx
//
//  Contents:   CPropertyBag
//
//----------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_PROPBAG_HXX_
#define X_PROPBAG_HXX_
#include "propbag.hxx"
#endif

#ifndef X_STRBUF_HXX_
#define X_STRBUF_HXX_
#include "strbuf.hxx"
#endif

MtDefine(CPropertyBag, ObjectModel, "CPropertyBag")
MtDefine(CPropertyBag_aryProps_pv, CPropertyBag, "CPropertyBag::_aryProps::_pv")
MtDefine(CPropertyBagGetPropertyBagContents_pVar, Locals, "CPropertyBag::GetPropertyBagContents pVar->pbVal")
MtDefine(CPropertyBagGetPropertyBagContents_pch, Locals, "CPropertyBag::GetPropertyBagContents pch")

DeclareTag(tagPropBag, "Property Bag", "Property Bag methods")

//+---------------------------------------------------------------------------
//
//  Member:     PROPNAMEVALUE::Set
//
//  Synopsis:   Initialize the struct with a name value pair.
//
//----------------------------------------------------------------------------

HRESULT
PROPNAMEVALUE::Set(TCHAR *pchPropName, VARIANT *pVar)
{
    HRESULT hr = _cstrName.Set(pchPropName);
    if (hr)
        goto Cleanup;
        
    hr = THR(VariantCopy(&_varValue, pVar));
    if (hr)
        goto Cleanup;

Cleanup:
    RRETURN(hr);
}


//+---------------------------------------------------------------------------
//
//  Member:     PROPNAMEVALUE::Free
//
//  Synopsis:   Free memory associated with this name value pair.
//
//----------------------------------------------------------------------------

void
PROPNAMEVALUE::Free()
{
    _cstrName.Free();
    VariantClear(&_varValue);
}


//+---------------------------------------------------------------------------
//
//  Member:     CPropertyBag::CPropertyBag
//
//  Synopsis:   ctor.
//
//----------------------------------------------------------------------------

CPropertyBag::CPropertyBag()
{
    _ulRefs = 1;
    _aryProps.SetSize(0);
    _pElementExpandos = NULL;
}


//+---------------------------------------------------------------------------
//
//  Member:     CPropertyBag::~CPropertyBag
//
//  Synopsis:   dtor.
//
//----------------------------------------------------------------------------

CPropertyBag::~CPropertyBag()
{
    Clear();
}

//+---------------------------------------------------------------------------
//
//  Member:     CPropertyBag::Clear
//
//----------------------------------------------------------------------------

void
CPropertyBag::Clear()
{
    long            c;
    PROPNAMEVALUE * pprop;
    
    for (c = _aryProps.Size(), pprop = _aryProps; c > 0; c--, pprop++)
    {
        pprop->Free();
    }
    _aryProps.SetSize(0);
}

//+---------------------------------------------------------------------------
//
//  Member:     CPropertyBag::QueryInterface
//
//  Synopsis:   per IUnknown.
//
//----------------------------------------------------------------------------

STDMETHODIMP
CPropertyBag::QueryInterface(REFIID riid, LPVOID * ppv)
{
    if (riid == IID_IPropertyBag ||
        riid == IID_IUnknown)
    {
        *ppv = (IPropertyBag *) this;
    }
    else if (riid == IID_IPropertyBag2)
    {
        *ppv = (IPropertyBag2 *)this;
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    ((IUnknown *)*ppv)->AddRef();
    return S_OK;
}


//+---------------------------------------------------------------------------
//
//  Member:     CPropertyBag::Read
//
//  Synopsis:   per IPropertyBag.
//
//----------------------------------------------------------------------------

STDMETHODIMP
CPropertyBag::Read(LPCOLESTR pchPropName, VARIANT *pVar, IErrorLog *pErrorLog)
{
    RRETURN (Read(pchPropName, pVar, pErrorLog, -1));
}

//+---------------------------------------------------------------------------
//
//  Member:     CPropertyBag::Read
//
//  Synopsis:   helper
//
//----------------------------------------------------------------------------

HRESULT
CPropertyBag::Read(LPCOLESTR pchPropName, VARIANT *pVar, IErrorLog *pErrorLog, long iLikelyIndex)
{
    HRESULT         hr = S_OK;
    PROPNAMEVALUE * pprop = NULL;
    VARTYPE         vt;
    
    // BUGBUG istvanc error log is not supported yet
    if (!pchPropName || !pVar)
    {
        hr = E_POINTER;
        goto Cleanup;
    }
    
    TraceTag((tagPropBag, "IPropertyBag::Read name=%ls", pchPropName));

    vt = V_VT(pVar);

    // BUGBUG istvanc cannot deal with objects yet
    if (vt == VT_UNKNOWN)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    //
    // The Java VM (the one which does not know about IPropertyBag2)
    // calls Read with this hacked up string.  Call GetPropertyBagContents
    // in response
    //
    
    if (_tcsequal(pchPropName, _T("%%pbcontents%%")))
    {
        V_VT(pVar) = vt;
        hr = THR(GetPropertyBagContents(pVar, pErrorLog));
    }
    else
    {
        pprop = Find((TCHAR *)pchPropName, iLikelyIndex);
        if (!pprop)
        {
            hr = E_INVALIDARG;
            goto Cleanup;
        }

        if (vt == VT_EMPTY)
        {
            vt = VT_BSTR;

            //
            // !!! (alexz) (anandra) (lylec) (cfranks)
            // We have to return variant of VT_BSTR for compatibility with IE3.01 and IE3.02.
            // Note that changing this to VT_I4 as IE3.00 did can have complications because
            // non-numeric string parameters can not be converted to VT_I4.
            //
        }

        //
        // Set the vt of the variant to VT_EMPTY.
        // There are controls that call in here with garbage values in the
        // the variant and VariantChangeType will choke.  Look at ie4 
        // bug 38350.  (anandra)
        //
    
        V_VT(pVar) = VT_EMPTY;

        //BUGWIN16: No support for VARIANT_NOUSEROVERRIDE
#ifndef WIN16
        // Do not change this to use VariantChangeTypeSpecial, due to IE3 compatability concerns
        hr = THR(VariantChangeTypeEx(pVar,
                                     &pprop->_varValue,
                                     LCID_SCRIPTING,
                                     VARIANT_NOUSEROVERRIDE,
                                     vt));
        if (hr)
        {
            //
            // some controls rely on vt untouched after call to Read method of propbag.
            // If VariantChangeTypeEx succeeds in this code, then the variant remains untouched.
            // However, if VariantChangeTypeEx fails, then vt will be set to VT_EMPTY because we
            // cleared the variant. To preserve those controls who rely on untouched variant,
            // we restore here the type.
            // Control: marquee. IE4 Bug: 54109.
            // (alexz)

            V_VT(pVar) = vt;
            goto Cleanup;
        }
#endif // ndef WIN16
    }

    //
    // if we are in expandos loading mode, then, according the protocol, after reading each expando,
    // the expando is supposed to be removed from the element.
    //

    if (_pElementExpandos)
    {
        BSTR    bstrName;

        hr = THR(FormsAllocString(pchPropName, &bstrName));
        if (hr)
            goto Cleanup;

        hr = THR(_pElementExpandos->removeAttribute(bstrName, 0, NULL));

        FormsFreeString(bstrName);
    }

Cleanup:
    RRETURN(hr);
}


//+---------------------------------------------------------------------------
//
//  Member:     CPropertyBag::Write
//
//  Synopsis:   per IPropertyBag.
//
//----------------------------------------------------------------------------

STDMETHODIMP
CPropertyBag::Write(LPCOLESTR pchPropName, VARIANT *pVar)
{
    HRESULT         hr = S_OK;
    PROPNAMEVALUE * pprop = NULL;
    PROPNAMEVALUE   propNew;
    
    if (!pchPropName || !pVar)
        return E_POINTER;

    TraceTag((tagPropBag, "IPropertyBag::Write name=%ls", pchPropName));

    pprop = Find((TCHAR *)pchPropName);

    //
    // Add only newly encountered properties.  Ignore new values of
    // properties we've seen before (compat with IE 3.0).
    //
    
    if (pprop)
        goto Cleanup;
        
    hr = THR(_aryProps.AppendIndirect(&propNew, &pprop));
    if (hr)
        goto Cleanup;

    hr = THR(pprop->Set((TCHAR *)pchPropName, pVar));
    if (hr)
        goto Cleanup;
        
Cleanup:
    RRETURN(hr);
}


//+---------------------------------------------------------------------------
//
//  Member:     CPropertyBag::Read
//
//  Synopsis:   Per IPropertyBag2
//
//----------------------------------------------------------------------------

HRESULT
CPropertyBag::Read(
    ULONG cProperties, 
    PROPBAG2 *pPB2, 
    IErrorLog *pErrorLog,
    VARIANT *pVar,
    HRESULT *phrError)
{
    long        i;
    HRESULT     hr;
    BOOL        fFail = FALSE;
    
    if (phrError)
    {
        //
        // Assume no errors
        //
        
        memset(phrError, S_OK, sizeof(HRESULT) * cProperties);
    }
        
    //
    // Temp implementation.  Delegate to IPropertyBag::Read
    //

    for (i = cProperties; i > 0; i--, pPB2++, pVar++)
    {
        VariantClear(pVar);
        V_VT(pVar) = pPB2->vt;
        hr = THR(Read(pPB2->pstrName, pVar, pErrorLog, pPB2->dwHint));
        if (hr)
        {
            fFail = TRUE;
            if (phrError)
            {
                *(phrError + cProperties - i) = hr;
            }
        }
    }

    RRETURN((fFail) ? E_FAIL : S_OK);
}


//+---------------------------------------------------------------------------
//
//  Member:     CPropertyBag::Write
//
//  Synopsis:   Per IPropertyBag2
//
//----------------------------------------------------------------------------

HRESULT
CPropertyBag::Write(ULONG cProperties, PROPBAG2 *pPB2, VARIANT *pVar)
{
    long        i;
    BOOL        fFail = FALSE;
    HRESULT     hr;
    
    //
    // Temp implementation.  Delegate to IPropertyBag::Write
    //

    for (i = cProperties; i > 0; i--, pPB2++, pVar++)
    {
        hr = THR(Write(pPB2->pstrName, pVar));
        if (hr)
        {
            fFail = TRUE;
        }
    }

    RRETURN((fFail) ? E_FAIL : S_OK);
}


//+---------------------------------------------------------------------------
//
//  Member:     CPropertyBag::CountProperties
//
//  Synopsis:   Per IPropertyBag2
//
//----------------------------------------------------------------------------

HRESULT
CPropertyBag::CountProperties(ULONG *pcProp)
{
    if (!pcProp)
        RRETURN(E_POINTER);

    *pcProp = _aryProps.Size();
    return S_OK;
}


//+---------------------------------------------------------------------------
//
//  Member:     CPropertyBag::GetPropertyInfo
//
//  Synopsis:   Per IPropertyBag2
//
//----------------------------------------------------------------------------

HRESULT
CPropertyBag::GetPropertyInfo(
    ULONG iProperty,
    ULONG cProperties,
    PROPBAG2 *pPropBag,
    ULONG *pcProperties)
{
    PROPNAMEVALUE * pprop = NULL;
    HRESULT         hr = S_OK;
    long            i, iStop;
    
    //
    // Perform initializations
    //
    
    *pcProperties = 0;
    memset(pPropBag, 0, sizeof(PROPBAG2) * cProperties);
    
    if (iProperty >= (ULONG)_aryProps.Size())
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    //
    // Figure out the number of PROPBAG2 structs to fill.  This
    // is the min of the number requested and the size of the array.
    //

    *pcProperties = min(cProperties, _aryProps.Size() - iProperty);
    
    //
    // For now only reply with PROPBAG2_TYPE_DATA
    //
    
    for (pprop = (PROPNAMEVALUE *)_aryProps + iProperty, i = iProperty, iStop = iProperty + *pcProperties; 
         i < iStop; 
         pprop++, i++, pPropBag++)
    {
        pPropBag->dwType = PROPBAG2_TYPE_DATA;
        pPropBag->vt = V_VT(&pprop->_varValue);
        pPropBag->cfType = CF_TEXT;
        pPropBag->pstrName = (TCHAR *)
            CoTaskMemAlloc((pprop->_cstrName.Length() +1) * sizeof(TCHAR));
        pPropBag->dwHint = i;
        if (!pPropBag->pstrName)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }
        _tcscpy(pPropBag->pstrName, pprop->_cstrName);
    }
    
Cleanup:
    RRETURN(hr);
}


//+---------------------------------------------------------------------------
//
//  Member:     CPropertyBag::LoadObject
//
//  Synopsis:   Per IPropertyBag2
//
//----------------------------------------------------------------------------

HRESULT
CPropertyBag::LoadObject(
    LPCOLESTR pstrName,
    DWORD dwHint,
    IUnknown *pUnkObject,
    IErrorLog *pErrorLog)
{
    RRETURN(E_NOTIMPL);
}


//======================================================================
//
// BUGBUG: istvanc from OHARE -> HACKHACK davidna 5/7/96 Beta1 Hack
//
// Called when PropertyBag::read() is called with the Property name set
// to "%%pbcontents%%"
//
// Put the contents in a block of memory as name & value pairs.
// Stored as char FAR * 's
// Return this in the callers Variant
//
// The storage goes like this:
// We have pointers to the name/value pairs following the
// the count (ULONG) of items (or pairs) in _pbcontent
//
//

//$ WIN64: CPropertyBag::GetPropertyBagContents constructs a buffer of counts and
//$ WIN64:   pointers to strings.  It assumes that sizeof(void *) == sizeof(ULONG).
//$ WIN64:   Appears that Java VM understands this format as well (%%pbcontents%%).

HRESULT 
CPropertyBag::GetPropertyBagContents(VARIANT *pVar, IErrorLog *pErrorLog)
{
    HRESULT         hr = S_OK;
    BYTE *          pbcontent = NULL;
    PROPNAMEVALUE * pprop;
    long            c;
    CVariant        Var;
    ULONG           cb;
    ULONG *         pul;
    
    DbgMemoryTrackDisable(TRUE);
    
    if (!pVar)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    // Must be this type for this HACK!
    if (V_VT(pVar) != (VT_BYREF|VT_UI1))
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    if (!_aryProps.Size())
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    // allocate storage for the count (ULONG/number of items)
    // plus each of the items or name/value pairs
    cb = sizeof(ULONG) + (sizeof(char *) * _aryProps.Size()) * 2;

    Assert(sizeof(ULONG) == sizeof(char *));

    pbcontent = new(Mt(CPropertyBagGetPropertyBagContents_pVar)) BYTE [cb];
    if (!pbcontent)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }
    memset(pbcontent, 0, cb);

    pul = (ULONG *)pbcontent;

    // first ULONG is count of items
    c = _aryProps.Size();
    *pul++ = c;

    // Fill in the blanks
    for (pprop = _aryProps;
         c > 0;
         c--, pprop++)
    {
        VARIANT * pVar;
        char * pch;

        //  Convert to ansi 'cause Java Applets probably won't like BSTR's
        //   and they're our only users of this

        //
        // store a pointer to the name
        //
        cb = WideCharToMultiByte(CP_ACP, 0, pprop->_cstrName, -1, NULL, 0,
              NULL, NULL);

        pch = new(Mt(CPropertyBagGetPropertyBagContents_pch)) char[cb + 1];
        if (!pch)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }

        WideCharToMultiByte(CP_ACP, 0, pprop->_cstrName, -1, pch, cb,
            NULL, NULL);
        
        pch[cb] = 0;

        *pul++ = (ULONG)(ULONG_PTR)pch;

        //BUGWIN16: No support for VARIANT_NOUSEROVERRIDE
#ifndef WIN16
        // Now store a pointer to the value
        //
        if (pprop->_varValue.vt != VT_BSTR)
        {
            // Do not change this to use VariantChangeTypeSpecial, due to IE3 compatability concerns
            hr = VariantChangeTypeEx(&Var,
                                     &pprop->_varValue,
                                     LCID_SCRIPTING,
                                     VARIANT_NOUSEROVERRIDE,
                                     VT_BSTR);
            if (hr)
                goto Cleanup;
            pVar = &Var;
        }
        else
#endif // ndef WIN16
        {
            pVar = &pprop->_varValue;
        }

        //
        // store a pointer to the value
        //
        
        cb = WideCharToMultiByte(CP_ACP, 0, pVar->bstrVal,
          FormsStringLen(pVar->bstrVal), NULL, 0, NULL, NULL);
        
        pch = new(Mt(CPropertyBagGetPropertyBagContents_pch)) char[cb + 1];
        if (!pch)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }

        WideCharToMultiByte(CP_ACP, 0, pVar->bstrVal,
            FormsStringLen(pVar->bstrVal), pch, cb, NULL, NULL);

        pch[cb] = 0;

        *pul++ = (ULONG)(ULONG_PTR)pch;
    }

    // put in variant
    pVar->pbVal = pbcontent;

Cleanup:
    DbgMemoryTrackDisable(FALSE);
    RRETURN(hr);
}


//+---------------------------------------------------------------------------
//
//  Member:     CPropertyBag::AddProp
//
//  Synopsis:   Helper to add a given property and name value pair.
//              with count of chars in name & value.
//
//----------------------------------------------------------------------------

HRESULT
CPropertyBag::AddProp(
    TCHAR *pchName, 
    int cchName, 
    TCHAR *pchValue, 
    int cchValue)
{
    HRESULT     hr;
    CStr        cstrName;
    CVariant    Var;

    Assert (pchName && (0 < cchName) && (0 <= cchValue));

    hr = THR(cstrName.Set (pchName, cchName));
    if (hr)
        goto Cleanup;

    if (cchValue > 0 && pchValue)
    {
        V_VT(&Var) = VT_BSTR;
        hr = THR(FormsAllocStringLen(pchValue, cchValue, &V_BSTR(&Var)));
        if (hr)
            goto Cleanup;
    }
    else
    {   // Zero length or NULL values are treated as VT_EMPTY:
        V_VT(&Var) = VT_EMPTY;
    }

    hr = THR(Write(cstrName, &Var));

Cleanup:
    RRETURN(hr);
}


//+---------------------------------------------------------------------------
//
//  Member:     CPropertyBag::AddProp
//
//  Synopsis:   Helper to add a given property and name value pair.
//              These are null terminated strings.
//
//----------------------------------------------------------------------------

HRESULT
CPropertyBag::AddProp(TCHAR *pchName, TCHAR *pchValue)
{
    RRETURN(AddProp(
        pchName, _tcslen(pchName),
        pchValue, _tcslen(pchValue)));
}

//----------------------------------------------------------------------------
//  Member:     CPropertyBag::FindAndSetProp
//
//  Synopsis:   Helper to change a property value after the property bag 
//              has been prepared
//----------------------------------------------------------------------------
HRESULT
CPropertyBag::FindAndSetProp( TCHAR *pchName, TCHAR *pchValue)
{
    HRESULT     hr;
    CStr        cstrName;
    CVariant    Var;

    Assert (pchName);

    PROPNAMEVALUE * pprop = NULL;
    int             cchName = _tcslen(pchName);
    int             cchValue= 0;
    
    if (pchValue)
        cchValue = _tcslen(pchValue);
    
    Assert (0 < cchName);

    hr = THR(cstrName.Set (pchName, cchName));
    if (hr)
        goto Cleanup;

    if (cchValue > 0)   //implies that pchValue!=NULL
    {
        V_VT(&Var) = VT_BSTR;
        hr = THR(FormsAllocStringLen(pchValue, cchValue, &V_BSTR(&Var)));
        if (hr)
            goto Cleanup;
    }
    else
    {   // Zero length or NULL values are treated as VT_EMPTY:
        V_VT(&Var) = VT_EMPTY;
    }

//
// BUGBUG:(ferhane) Move this before the if statement above on the 5.x tree
//                  for perf.
//
    pprop = Find( pchName, 0);

    if (!pprop)
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    // free the old variant that was stored in the propertybag
    pprop->Free();

    hr = THR(pprop->Set(cstrName, &Var));

Cleanup:
    RRETURN(hr);
}


//+---------------------------------------------------------------------------
//
//  Member:     CPropertyBag::Save
//
//  Synopsis:   saves to pStreamWrBuff html-formatted param bag
//
//----------------------------------------------------------------------------

HRESULT
CPropertyBag::Save(CStreamWriteBuff * pStreamWrBuff)
{
    HRESULT         hr = S_OK;
    long            c;
    PROPNAMEVALUE * pprop;
    DWORD           dwOldBuffFlags;
    
    dwOldBuffFlags = pStreamWrBuff->ClearFlags(WBF_ENTITYREF);
    pStreamWrBuff->BeginPre();
    pStreamWrBuff->BeginIndent();

    for (c = _aryProps.Size(), pprop = _aryProps;
         c > 0; 
         c--, pprop++)
    {
        //
        // First try to convert this param into a string.
        //
        
        if (VT_BSTR != V_VT(&pprop->_varValue))
        {
            //BUGWIN16: No support for VARIANT_NOUSEROVERRIDE
#ifdef WIN16
            hr = S_OK;
            continue;
#else
            // Do not change this to use VariantChangeTypeSpecial, due to IE3 compatability concerns
            hr = VariantChangeTypeEx(&pprop->_varValue,
                                     &pprop->_varValue,
                                     LCID_SCRIPTING,
                                     VARIANT_NOUSEROVERRIDE,
                                     VT_BSTR);
            if (hr)
            {
                // a property in param bag could not be converted to string;
                // we still try to save other properties

                // BUGBUG: need to report this error to user somehow;
                // e.g., by putting a comment to html about failure to save
                // the property.   Also handle catastrophic failure such as out-of-mem
                // appropriately.

                hr = S_OK;  // so if this was the last property everyone continues merrily along.
                continue;
            }
#endif
        }

        hr = THR(pStreamWrBuff->NewLine());
        if (hr)
            break;

        hr = THR(pStreamWrBuff->Write(
                     pStreamWrBuff->TestFlag(WBF_SAVE_FOR_PRINTDOC) && pStreamWrBuff->TestFlag(WBF_SAVE_FOR_XML)
                        ? _T("<HTML:PARAM NAME=") : _T("<PARAM NAME=") )
                );
        if (hr)
            break;

        hr = THR(pStreamWrBuff->WriteQuotedText(pprop->_cstrName, TRUE));
        if (hr)
            break;

        hr = THR(pStreamWrBuff->Write(_T(" VALUE=")));
        if (hr)
            break;

        Assert(V_VT(&pprop->_varValue) == VT_BSTR);
        hr = THR(pStreamWrBuff->WriteQuotedText(
                V_BSTR(&pprop->_varValue), 
                TRUE));
        if (hr)
            break;

        hr = THR(pStreamWrBuff->Write(_T(">")));
        if (hr)
            break;
    }

    pStreamWrBuff->EndIndent();
    pStreamWrBuff->EndPre();
    pStreamWrBuff->RestoreFlags(dwOldBuffFlags);

    RRETURN(hr);
}


//+---------------------------------------------------------------------------
//
//  Member:     CPropertyBag::Find
//
//  Synopsis:   Return struct of PROPNAMEVALUE of given name
//
//----------------------------------------------------------------------------

PROPNAMEVALUE *
CPropertyBag::Find(TCHAR *pchName, long iLikelyIndex)
{
    PROPNAMEVALUE * pprop;
    long            c;

    Assert(pchName);

    if (0 <= iLikelyIndex && iLikelyIndex < _aryProps.Size())
    {
        pprop = &_aryProps[iLikelyIndex];
        if (0 == _tcsicmp(pchName, pprop->_cstrName))
        {
            return pprop;
        }
    }

    for (c = _aryProps.Size(), pprop = _aryProps; c > 0; c--, pprop++)
    {
        if (0 == _tcsicmp(pchName, pprop->_cstrName))
        {
            // Found, so return it.
            return pprop;
        }
    }

    return NULL;
}
