//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1994-1996
//
//  File:       sprop.cxx
//
//  Contents:   These are the Style Handlers and Helpers
//
//----------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_STYLE_HXX_
#define X_STYLE_HXX_
#include "style.hxx"
#endif

///+---------------------------------------------------------------
//
//  Member:     PROPERTYDESC::HandleStyleProperty, public
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
PROPERTYDESC::HandleStyleProperty(DWORD dwOpCode, void * pv, CBase * pObject, CVoid * pSubObject) const
{
    HRESULT     hr = S_OK;
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
                hr = THR(VariantChangeTypeSpecial(&varDest, (VARIANT *)pv,  VT_BSTR));
                if (hr)
                    goto Cleanup;
                pv = V_BSTR(&varDest);
            }
        default:
            Assert(PROPTYPE(dwOpCode) == 0);    // assumed native long
            break;
        }

        switch(OPCODE(dwOpCode))
        {
        case HANDLEPROP_DEFAULT:
            Assert(pv == NULL);
            pv = (void *)ulTagNotPresentDefault;

            if (!pv)
                goto Cleanup;       // zero string

            // fall thru
        case HANDLEPROP_VALUE:
        case HANDLEPROP_AUTOMATION:
            if ( pv && *(TCHAR *)pv )
            {
                CElement *pElem = (CElement *)pObject;
                LPTSTR lpszStyleText = (TCHAR *)pv;
                CAttrArray **ppAA = pElem->CreateStyleAttrArray(DISPID_INTERNAL_INLINESTYLEAA);
#ifdef XMV_PARSE
                BOOL fXML = pElem->IsInMarkup() && pElem->GetMarkupPtr()->IsXML();
                CCSSParser ps( NULL, ppAA, fXML, eSingleStyle, &CStyle::s_apHdlDescs, pObject, OPERATION(dwOpCode) );
#else
                CCSSParser ps( NULL, ppAA, eSingleStyle, &CStyle::s_apHdlDescs, pObject, OPERATION(dwOpCode) );
#endif

                ps.Open();
                ps.Write( lpszStyleText, _tcslen( lpszStyleText ) );
                ps.Close();
            }
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
            if (PROPTYPE(dwOpCode) == PROPTYPE_VARIANT)
            {
                hr = THR(WriteStyleToBSTR(
                        pObject, 
                        ((CElement *)pObject)->GetInLineStyleAttrArray(), 
                        &((VARIANT *)pv)->bstrVal, 
                        TRUE));
                ((VARIANT *)pv)->vt = VT_BSTR;
            }
            else if (PROPTYPE(dwOpCode) == PROPTYPE_BSTR)
            {
                hr = THR(WriteStyleToBSTR(
                        pObject, 
                        ((CElement *)pObject)->GetInLineStyleAttrArray(), 
                        (BSTR *)pv, 
                        TRUE));
            }
            else
            {
                BSTR    bstr = NULL;
                
                Assert(PROPTYPE(dwOpCode) == 0);

                hr = THR(WriteStyleToBSTR(
                        pObject, 
                        ((CElement *)pObject)->GetInLineStyleAttrArray(), 
                        &bstr, 
                        TRUE));
                if (!hr)
                {
                    hr = ((CStr *)pv)->Set(bstr);
                    FormsFreeString(bstr);
                }
            }
            if (hr)
                goto Cleanup;
                
            break;
            
        case HANDLEPROP_STREAM:
            Assert(PROPTYPE(dwOpCode) == PROPTYPE_LPWSTR);
            {
                BSTR bstrTemp;
                hr = THR( WriteStyleToBSTR( pObject, ((CElement *)pObject)->GetInLineStyleAttrArray(), &bstrTemp, TRUE ) );
                if ( !hr )
                {
                    if ( *bstrTemp )
                        hr = THR(((IStream*) (void*) pv)->Write( bstrTemp, _tcslen(bstrTemp) * sizeof(TCHAR), NULL));
                    FormsFreeString( bstrTemp );
                }
            }

            if (hr)
                goto Cleanup;
            break;

        case HANDLEPROP_COMPARE:
            {
                CElement *pElem = (CElement *)pObject;
                CAttrArray *pAA = pElem->GetInLineStyleAttrArray();
                // Check for presence of Attributs and Expandos
                hr = pAA && pAA->HasAnyAttribute(TRUE) ? S_FALSE : S_OK;
            }
            break;

        default:
            hr = E_FAIL;
            Assert(FALSE && "Invalid operation");
            break;
        }
    }

Cleanup:
    if (varDest.vt != VT_EMPTY)
    {
        VariantClear(&varDest);
    }
    RRETURN1(hr,S_FALSE);
}

HRESULT BASICPROPPARAMS::GetStyleComponentProperty(BSTR *pbstr, CBase *pObject, CVoid *pSubObject) const
{
    VARIANT varValue;
    HRESULT hr;
    PROPERTYDESC *ppdPropDesc = ((PROPERTYDESC *)this)-1;

    hr = THR(ppdPropDesc->HandleStyleComponentProperty( HANDLEPROP_VALUE | (PROPTYPE_VARIANT<<16), 
        (void *)&varValue, pObject, pSubObject ));
    if ( !hr )
        *pbstr = V_BSTR(&varValue);

    RRETURN(pObject ? pObject->SetErrorInfo(hr) : hr);
}

HRESULT BASICPROPPARAMS::SetStyleComponentProperty( BSTR bstr, CBase *pObject, CVoid *pSubObject, WORD wFlags ) const
{
    HRESULT hr;
    CBase::CLock    Lock(pObject);
    DWORD dwOpCode = HANDLEPROP_SET|HANDLEPROP_AUTOMATION;
    PROPERTYDESC *ppdPropDesc = ((PROPERTYDESC *)this)-1;

    if (wFlags & CAttrValue::AA_Extra_Important)
        dwOpCode |= HANDLEPROP_IMPORTANT;
    if (wFlags & CAttrValue::AA_Extra_Implied)
        dwOpCode |= HANDLEPROP_IMPLIED;

    SETPROPTYPE( dwOpCode, PROPTYPE_LPWSTR );

    hr = THR(ppdPropDesc->HandleStyleComponentProperty( dwOpCode, (void *) bstr, pObject, pSubObject ));
    
    RRETURN1(pObject ? pObject->SetErrorInfo(hr) : hr, E_INVALIDARG);
}

HRESULT BASICPROPPARAMS::GetStyleComponentBooleanProperty( VARIANT_BOOL * p, CBase *pObject, CVoid *pSubObject) const
{
    VARIANT varValue;
    HRESULT hr;
    PROPERTYDESC *ppdPropDesc = ((PROPERTYDESC *)this)-1;

    hr = THR(ppdPropDesc->HandleStyleComponentProperty( HANDLEPROP_VALUE | (PROPTYPE_VARIANT<<16), 
        (void *)&varValue, pObject, pSubObject ));
    if ( !hr )
    {
        Assert( varValue.vt == VT_BOOL );
        *p = varValue.boolVal;
    }
    RRETURN(pObject->SetErrorInfo(hr));
}

HRESULT BASICPROPPARAMS::SetStyleComponentBooleanProperty( VARIANT_BOOL v, CBase *pObject, CVoid *pSubObject ) const
{
    HRESULT hr;
    CBase::CLock    Lock(pObject);
    DWORD dwOpCode = HANDLEPROP_SET|HANDLEPROP_AUTOMATION;
    PROPERTYDESC *ppdPropDesc = ((PROPERTYDESC *)this)-1;
    VARIANT var;

    var.vt = VT_BOOL;
    var.boolVal = v;

    SETPROPTYPE( dwOpCode, PROPTYPE_VARIANT );

    hr = THR(ppdPropDesc->HandleStyleComponentProperty( dwOpCode, (void *) &var, pObject, pSubObject ));
    RRETURN1(pObject->SetErrorInfo(hr), E_INVALIDARG);
}

STDMETHODIMP CStyle::put_textDecorationNone(VARIANT_BOOL v)
{
	return s_propdescCStyletextDecorationNone.b.SetStyleComponentBooleanProperty(v, this, (CVoid *)(void *)(GetAttrArray()));
}
STDMETHODIMP CStyle::get_textDecorationNone(VARIANT_BOOL * p)
{
	return s_propdescCStyletextDecorationNone.b.GetStyleComponentBooleanProperty(p, this, (CVoid *)(void *)(GetAttrArray()));
}
STDMETHODIMP CStyle::put_textDecorationUnderline(VARIANT_BOOL v)
{
	return s_propdescCStyletextDecorationUnderline.b.SetStyleComponentBooleanProperty(v, this, (CVoid *)(void *)(GetAttrArray()));
}
STDMETHODIMP CStyle::get_textDecorationUnderline(VARIANT_BOOL * p)
{
	return s_propdescCStyletextDecorationUnderline.b.GetStyleComponentBooleanProperty(p, this, (CVoid *)(void *)(GetAttrArray()));
}
STDMETHODIMP CStyle::put_textDecorationOverline(VARIANT_BOOL v)
{
	return s_propdescCStyletextDecorationOverline.b.SetStyleComponentBooleanProperty(v, this, (CVoid *)(void *)(GetAttrArray()));
}
STDMETHODIMP CStyle::get_textDecorationOverline(VARIANT_BOOL * p)
{
	return s_propdescCStyletextDecorationOverline.b.GetStyleComponentBooleanProperty(p, this, (CVoid *)(void *)(GetAttrArray()));
}
STDMETHODIMP CStyle::put_textDecorationLineThrough(VARIANT_BOOL v)
{
	return s_propdescCStyletextDecorationLineThrough.b.SetStyleComponentBooleanProperty(v, this, (CVoid *)(void *)(GetAttrArray()));
}
STDMETHODIMP CStyle::get_textDecorationLineThrough(VARIANT_BOOL * p)
{
	return s_propdescCStyletextDecorationLineThrough.b.GetStyleComponentBooleanProperty(p, this, (CVoid *)(void *)(GetAttrArray()));
}
STDMETHODIMP CStyle::put_textDecorationBlink(VARIANT_BOOL v)
{
	return s_propdescCStyletextDecorationBlink.b.SetStyleComponentBooleanProperty(v, this, (CVoid *)(void *)(GetAttrArray()));
}
STDMETHODIMP CStyle::get_textDecorationBlink(VARIANT_BOOL * p)
{
	return s_propdescCStyletextDecorationBlink.b.GetStyleComponentBooleanProperty(p, this, (CVoid *)(void *)(GetAttrArray()));
}


HRESULT
CStyle::toString(BSTR* String)
{
    RRETURN(super::toString(String));
};



//+---------------------------------------------------------------
//
//  Member:     PROPERTYDESC::HandleStyleComponentProperty, public
//
//  Synopsis:   Helper for getting/setting url style sheet properties...
//              url(string)
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
PROPERTYDESC::HandleStyleComponentProperty(DWORD dwOpCode, void * pv, CBase * pObject, CVoid * pSubObject) const
{
    HRESULT hr = S_OK;
    VARIANT varDest;
    size_t  nLenIn = (size_t) -1;
	DWORD dispid = GetBasicPropParams()->dispid;
    BSTR bstrTemp;  // Used by some of the stream writers
    BOOL fTDPropertyValue=FALSE;  // If this is a SET of a text-decoration sub-property, this is the value
    WORD wFlags = 0;

    if (ISSET(dwOpCode))
    {
        Assert(!(ISSTREAM(dwOpCode))); // we can't do this yet...
        switch ( dispid )
        {
        case DISPID_A_TEXTDECORATIONNONE:
        case DISPID_A_TEXTDECORATIONUNDERLINE:
        case DISPID_A_TEXTDECORATIONOVERLINE:
        case DISPID_A_TEXTDECORATIONLINETHROUGH:
        case DISPID_A_TEXTDECORATIONBLINK:
            Assert( PROPTYPE(dwOpCode) == PROPTYPE_VARIANT && "Text-decoration subproperties must take variants!" );
            Assert( V_VT((VARIANT *)pv) == VT_BOOL && "Text-decoration subproperties must take BOOLEANs!" );
            fTDPropertyValue = !!((VARIANT *)pv)->boolVal;
            break;

        default:
            switch(PROPTYPE(dwOpCode))
            {
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

                //intentional fallthrough
            case PROPTYPE_LPWSTR:
                switch (dispid)
                {
                case DISPID_A_BACKGROUNDIMAGE:
                case DISPID_A_LISTSTYLEIMAGE:
                case DISPID_A_FONTFACESRC:
                    nLenIn = ValidStyleUrl((TCHAR*) pv);
                    if ( OPCODE(dwOpCode) == HANDLEPROP_VALUE )
                    {
                        if (!nLenIn && _tcsicmp( (TCHAR*)pv, _T("none") ) )
                        {
                            hr = E_INVALIDARG;
                            goto Cleanup;
                        }
                    }
                    break;

                case DISPID_A_BEHAVIOR:
                    nLenIn = pv ? _tcslen((TCHAR*) pv) : 0;
                    break;
                }
                break;

		    default:
			    Assert( FALSE );	// We shouldn't get here.
            }
            break;
        }

        switch ( dispid )
        {
        case DISPID_A_LISTSTYLEIMAGE:
        case DISPID_A_BACKGROUNDIMAGE:
        case DISPID_A_FONTFACESRC:
        case DISPID_A_BEHAVIOR:

            if (nLenIn && (nLenIn != (size_t) -1))
            {
                if (DISPID_A_BEHAVIOR == dispid)
                {
                    hr = THR(HandleStringProperty(dwOpCode, (TCHAR*) pv, pObject, pSubObject));
                }
                else
                {
                    TCHAR *pch = (TCHAR*) pv;
                    TCHAR *psz = pch+4;
                    TCHAR *quote = NULL;
                    TCHAR *pszEnd;
                    TCHAR terminator;

                    while ( _istspace( *psz ) )
                        psz++;
                    if ( *psz == _T('\'') || *psz == _T('"') )
                    {
                        quote = psz++;
                    }
                    nLenIn--;   // Skip back over the ')' character - we know there is one, because ValidStyleUrl passed this string.
                    pszEnd = pch + nLenIn - 1;
                    while ( _istspace( *pszEnd ) && ( pszEnd > psz ) )
                        pszEnd--;
                    if ( quote && ( *pszEnd == *quote ) )
                        pszEnd--;
                    terminator = *(pszEnd+1);
                    *(pszEnd+1) = _T('\0');
                    hr = THR(HandleStringProperty(dwOpCode, psz, pObject, pSubObject));
                    *(pszEnd+1) = terminator;
                }
            }
            else
            {
                if ( !pv || !*(TCHAR*)pv )
                {   // Empty string - delete the entry.
                    CAttrArray **ppAA = (CAttrArray **)pSubObject;

                    if ( *ppAA )
                        (*ppAA)->FindSimpleAndDelete( dispid, CAttrValue::AA_StyleAttribute, NULL );
                }
                else if ( !_tcsicmp( (TCHAR*)pv, _T("none") ) )
                {
                    hr = THR(HandleStringProperty(dwOpCode, (void *)_T(""), pObject, pSubObject));
                }
                else
                    hr = E_INVALIDARG;
            }
            break;

        case DISPID_A_BACKGROUND:
            hr = ParseBackgroundProperty( (CAttrArray **)pSubObject, pObject, OPERATION(dwOpCode), (TCHAR*) pv,
                            ( OPCODE(dwOpCode) == HANDLEPROP_VALUE ) );
            break;
        case DISPID_A_FONT:
            if ( pv && FindSystemFontByName( (TCHAR*) pv ) != sysfont_non_system )
            {
                hr = THR(HandleStringProperty(dwOpCode, pv, pObject, pSubObject));
            }
            else
                hr = ParseFontProperty( (CAttrArray **)pSubObject, pObject, OPERATION(dwOpCode), (TCHAR*) pv );
            break;
        case DISPID_A_LAYOUTGRID:
            hr = ParseLayoutGridProperty( (CAttrArray **)pSubObject, pObject, OPERATION(dwOpCode), (TCHAR*) pv );
            break;
        case DISPID_A_TEXTAUTOSPACE:
            if ( dwOpCode & HANDLEPROP_IMPORTANT )
                wFlags |= CAttrValue::AA_Extra_Important;
            if ( dwOpCode & HANDLEPROP_IMPLIED )
                wFlags |= CAttrValue::AA_Extra_Implied;

            hr = ParseTextAutospaceProperty( (CAttrArray **)pSubObject, (LPCTSTR)pv, wFlags );
            break;
        case DISPID_A_TEXTDECORATION:
            if ( dwOpCode & HANDLEPROP_IMPORTANT )
                wFlags |= CAttrValue::AA_Extra_Important;
            if ( dwOpCode & HANDLEPROP_IMPLIED )
                wFlags |= CAttrValue::AA_Extra_Implied;

            hr = ParseTextDecorationProperty( (CAttrArray **)pSubObject, (LPCTSTR)pv, wFlags );
            break;
        case DISPID_A_TEXTDECORATIONNONE:
        case DISPID_A_TEXTDECORATIONUNDERLINE:
        case DISPID_A_TEXTDECORATIONOVERLINE:
        case DISPID_A_TEXTDECORATIONLINETHROUGH:
        case DISPID_A_TEXTDECORATIONBLINK:
            {
                VARIANT v;

                v.vt = VT_I4;
                v.lVal = 0;
                if ( *((CAttrArray **)pSubObject) )
                {   // See if we already have a text-decoration value
                    CAttrValue *pAV = (*((CAttrArray **)pSubObject))->Find( DISPID_A_TEXTDECORATION, CAttrValue::AA_Attribute );
                    if ( pAV )
                    {   // We do!  Copy its value into our working variant
                        v.lVal = pAV->GetLong();
                    }
                }
                switch ( dispid )
                {
                case DISPID_A_TEXTDECORATIONNONE:
                    if ( fTDPropertyValue )
                        v.lVal = TD_NONE;   // "none" clears all the other properties (unlike the other properties)
                    else
                        v.lVal &= ~TD_NONE;
                    break;
                case DISPID_A_TEXTDECORATIONUNDERLINE:
                    if ( fTDPropertyValue )
                        v.lVal |= TD_UNDERLINE;
                    else
                        v.lVal &= ~TD_UNDERLINE;
                    break;
                case DISPID_A_TEXTDECORATIONOVERLINE:
                    if ( fTDPropertyValue )
                        v.lVal |= TD_OVERLINE;
                    else
                        v.lVal &= ~TD_OVERLINE;
                    break;
                case DISPID_A_TEXTDECORATIONLINETHROUGH:
                    if ( fTDPropertyValue )
                        v.lVal |= TD_LINETHROUGH;
                    else
                        v.lVal &= ~TD_LINETHROUGH;
                    break;
                case DISPID_A_TEXTDECORATIONBLINK:
                    if ( fTDPropertyValue )
                        v.lVal |= TD_BLINK;
                    else
                        v.lVal &= ~TD_BLINK;
                    break;
                }
                if ( dwOpCode & HANDLEPROP_IMPORTANT )
                    wFlags |= CAttrValue::AA_Extra_Important;
                if ( dwOpCode & HANDLEPROP_IMPLIED )
                    wFlags |= CAttrValue::AA_Extra_Implied;
                hr = CAttrArray::Set( (CAttrArray **)pSubObject, DISPID_A_TEXTDECORATION, &v,
                                (PROPERTYDESC *)&s_propdescCStyletextDecoration, CAttrValue::AA_StyleAttribute, wFlags );
            }
            dispid = DISPID_A_TEXTDECORATION;   // This is so we call OnPropertyChange for the right property below.
            break;

        case DISPID_A_MARGIN:
        case DISPID_A_PADDING:
            hr = THR( ParseExpandProperty( (CAttrArray **)pSubObject, pObject, OPERATION(dwOpCode), (TCHAR*) pv, dispid, TRUE ) );
            break;

        case DISPID_A_BORDERCOLOR:
        case DISPID_A_BORDERWIDTH:
        case DISPID_A_BORDERSTYLE:
            hr = THR( ParseExpandProperty( (CAttrArray **)pSubObject, pObject, OPERATION(dwOpCode), (TCHAR*) pv, dispid, FALSE ) );
            break;

        case DISPID_A_STYLETEXT:
            {
                LPTSTR lpszStyleText = (TCHAR *)pv;
                CAttrArray **ppAA = (CAttrArray **)pSubObject;

                if ( *ppAA )
                    (*ppAA)->Free();

                if ( lpszStyleText && *lpszStyleText )
                {
                    CStyle *pStyle = DYNCAST(CStyle, pObject);

                    Assert( pStyle );
                    pStyle->MaskPropertyChanges( TRUE );
#ifdef XMV_PARSE
                    CElement *pElem = pStyle->GetElementPtr();
                    BOOL fXML = pElem && pElem->IsInMarkup() && pElem->GetMarkupPtr()->IsXML();
                    CCSSParser ps( NULL, ppAA, fXML, eSingleStyle, &CStyle::s_apHdlDescs, 
                                   pObject, OPERATION(dwOpCode) );
#else
                    CCSSParser ps( NULL, ppAA, eSingleStyle, &CStyle::s_apHdlDescs, 
                                   pObject, OPERATION(dwOpCode) );
#endif

                    ps.Open();
	                ps.Write( lpszStyleText, lstrlen( lpszStyleText ) );
	                ps.Close();
                    pStyle->MaskPropertyChanges( FALSE );
                }
            }
            break;

        case DISPID_A_BORDERTOP:
        case DISPID_A_BORDERRIGHT:
        case DISPID_A_BORDERBOTTOM:
        case DISPID_A_BORDERLEFT:
            hr = ParseAndExpandBorderSideProperty( (CAttrArray **)pSubObject, pObject, OPERATION(dwOpCode), (TCHAR*) pv, dispid );
            break;

        case DISPID_A_BORDER:
            hr = ParseBorderProperty( (CAttrArray **)pSubObject, pObject, OPERATION(dwOpCode), (TCHAR*) pv );
            break;

        case DISPID_A_LISTSTYLE:
            hr = ParseListStyleProperty( (CAttrArray **)pSubObject, pObject, OPERATION(dwOpCode), (TCHAR*) pv );
            break;

        case DISPID_A_BACKGROUNDPOSITION:
            hr = ParseBackgroundPositionProperty( (CAttrArray **)pSubObject, pObject, OPERATION(dwOpCode), (TCHAR *)pv );
            break;

        case DISPID_A_CLIP:
            hr = ParseClipProperty( (CAttrArray **)pSubObject, pObject, OPERATION(dwOpCode), (TCHAR *)pv );
            break;

        default:
            Assert( "Attempting to set an unknown type of CStyleComponent!" );
        }

        if (hr)
            goto Cleanup;
        else
        {
            // Note that dispid reflects the property that changed, not what was set -
            // e.g., textDecorationUnderline has been changed to textDecoration.
            if ( dwOpCode & HANDLEPROP_AUTOMATION )
            {
                CBase::CLock Lock( pObject );
                hr = THR(pObject->OnPropertyChange(dispid, GetdwFlags()));
            }
        }
    }
    else
    {	// GET value from data
        switch(OPCODE(dwOpCode))
        {
        case HANDLEPROP_STREAM:
            {
                IStream *pis = (IStream *) pv;

                switch ( dispid )
                {
                case DISPID_A_LISTSTYLEIMAGE:
                case DISPID_A_BACKGROUNDIMAGE:
                case DISPID_A_BEHAVIOR:
                case DISPID_A_FONTFACESRC:
                    if ( (*(CAttrArray **)pSubObject)->Find( dispid, CAttrValue::AA_Attribute ) )
                    {
                        BSTR bstrSub;

                        hr = HandleStringProperty( HANDLEPROP_AUTOMATION | (PROPTYPE_BSTR << 16), 
                                                   &bstrSub, pObject, pSubObject );
                        if ( hr == S_OK )
                        {
                            if ( bstrSub && *bstrSub )
                            {   // This is a normal url.
                                hr = pis->Write(_T("url("), 4*sizeof(TCHAR), NULL);
                                if (!hr)
                                {
                                    hr = pis->Write( bstrSub, FormsStringLen( bstrSub ) * sizeof(TCHAR), NULL);
                                    if (!hr)
                                    {
                                        hr = pis->Write(_T(")"), 1*sizeof(TCHAR), NULL);
                                    }
                                }
                            }
                            else
                            {   // We only get here if a NULL string was stored in the array; i.e., the value is NONE.
                                hr = pis->Write(_T("none"), 4*sizeof(TCHAR), NULL);
                            }
                            FormsFreeString( bstrSub );
                        }
                    }
                    break;

                case DISPID_A_BACKGROUND:
                    hr = WriteBackgroundStyleToBSTR( *(CAttrArray **)pSubObject, &bstrTemp );
                    if ( hr == S_OK )
                    {
                        hr = THR(pis->Write( bstrTemp, FormsStringLen(bstrTemp) * sizeof(TCHAR), NULL));
                        FormsFreeString( bstrTemp );
                    }
                    break;

                case DISPID_A_TEXTAUTOSPACE:
                    // We need to cook up this property.
                    hr = WriteTextAutospaceToBSTR( *(CAttrArray **)pSubObject, &bstrTemp );
                    if ( hr == S_OK )
                    {
                        hr = THR(pis->Write( bstrTemp, FormsStringLen(bstrTemp) * sizeof(TCHAR), NULL));
                        FormsFreeString( bstrTemp );
                    }
                    break;

                case DISPID_A_TEXTDECORATION:
                    // We need to cook up this property.
                    hr = WriteTextDecorationToBSTR( *(CAttrArray **)pSubObject, &bstrTemp );
                    if ( hr == S_OK )
                    {
                        hr = THR(pis->Write( bstrTemp, FormsStringLen(bstrTemp) * sizeof(TCHAR), NULL));
                        FormsFreeString( bstrTemp );
                    }
                    break;

                case DISPID_A_BORDERTOP:
                case DISPID_A_BORDERRIGHT:
                case DISPID_A_BORDERBOTTOM:
                case DISPID_A_BORDERLEFT:
                    hr = WriteBorderSidePropertyToBSTR( dispid, *(CAttrArray **)pSubObject, &bstrTemp );
                    if ( hr == S_OK )
                    {
                        hr = THR(pis->Write( bstrTemp, FormsStringLen(bstrTemp) * sizeof(TCHAR), NULL));
                        FormsFreeString( bstrTemp );
                    }
                    break;

                case DISPID_A_BORDER:
                    hr = WriteBorderToBSTR( *(CAttrArray **)pSubObject, &bstrTemp );
                    if ( hr == S_OK )
                    {
                        hr = THR(pis->Write( bstrTemp, FormsStringLen(bstrTemp) * sizeof(TCHAR), NULL));
                        FormsFreeString( bstrTemp );
                    }
                    break;

                case DISPID_A_MARGIN:
                case DISPID_A_PADDING:
                case DISPID_A_BORDERCOLOR:
                case DISPID_A_BORDERWIDTH:
                case DISPID_A_BORDERSTYLE:
                    hr = WriteExpandedPropertyToBSTR( dispid, *(CAttrArray **)pSubObject, &bstrTemp );
                    if ( hr == S_OK )
                    {
                        hr = THR(pis->Write( bstrTemp, FormsStringLen(bstrTemp) * sizeof(TCHAR), NULL));
                        FormsFreeString( bstrTemp );
                    }
                    break;

                case DISPID_A_LISTSTYLE:
                    hr = WriteListStyleToBSTR( *(CAttrArray **)pSubObject, &bstrTemp );
                    if ( hr == S_OK )
                    {
                        hr = THR(pis->Write( bstrTemp, FormsStringLen(bstrTemp) * sizeof(TCHAR), NULL));
                        FormsFreeString( bstrTemp );
                    }
                    break;
            
                case DISPID_A_BACKGROUNDPOSITION:
                    hr = WriteBackgroundPositionToBSTR( *(CAttrArray **)pSubObject, &bstrTemp );
                    if ( hr == S_OK )
                    {
                        hr = THR(pis->Write( bstrTemp, FormsStringLen(bstrTemp) * sizeof(TCHAR), NULL));
                        FormsFreeString( bstrTemp );
                    }
                    break;

                case DISPID_A_FONT:
                    if ( (*(CAttrArray **)pSubObject)->Find(DISPID_A_FONT, CAttrValue::AA_Attribute ) )
                        hr = HandleStringProperty(dwOpCode, pv, pObject, pSubObject);
                    else
                    {
                        // We need to cook up a "font" property.
                        hr = WriteFontToBSTR( *(CAttrArray **)pSubObject, &bstrTemp );
                        if ( !hr )
                        {
                            if ( *bstrTemp )
                                hr = THR(pis->Write( bstrTemp, FormsStringLen(bstrTemp) * sizeof(TCHAR), NULL));
                            FormsFreeString( bstrTemp );
                        }
                    }
                    break;
                case DISPID_A_LAYOUTGRID:
                    // We need to cook up a "layout grid" property.
                    hr = WriteLayoutGridToBSTR( *(CAttrArray **)pSubObject, &bstrTemp );
                    if ( !hr )
                    {
                        if ( *bstrTemp )
                            hr = THR(pis->Write( bstrTemp, FormsStringLen(bstrTemp) * sizeof(TCHAR), NULL));
                        FormsFreeString( bstrTemp );
                    }
                    break;
                case DISPID_A_CLIP:     // We need to cook up a "clip" property with the "rect" shape.
                    hr = WriteClipToBSTR( *(CAttrArray **)pSubObject, &bstrTemp );
                    if ( !hr )
                    {
                        if ( *bstrTemp )
                            hr = THR(pis->Write( bstrTemp, FormsStringLen(bstrTemp) * sizeof(TCHAR), NULL));
                        FormsFreeString( bstrTemp );
                    }
                    break;
                case DISPID_A_STYLETEXT:
                    hr = WriteStyleToBSTR( pObject, *(CAttrArray **)pSubObject, &bstrTemp, FALSE );
                    if ( !hr )
                    {
                        if ( *bstrTemp )
                            hr = THR(pis->Write( bstrTemp, _tcslen(bstrTemp) * sizeof(TCHAR), NULL));
                        FormsFreeString( bstrTemp );
                    }
                    break;
				}
            }
            break;
        default:
            {
                BSTR *pbstr;
                switch(PROPTYPE(dwOpCode))
                {
                case PROPTYPE_VARIANT:
                    V_VT((VARIANT *)pv) = VT_BSTR;
                    pbstr = &(((VARIANT *)pv)->bstrVal);
                    break;
                case PROPTYPE_BSTR:
                    pbstr = (BSTR *)pv;
                    break;
                default:
                    Assert( "Can't get anything but a VARIANT or BSTR for style component properties!" );
                    hr = S_FALSE;
                    goto Cleanup;
                }
                switch ( dispid )
                {
                case DISPID_A_BACKGROUND:
                    hr = WriteBackgroundStyleToBSTR( *(CAttrArray **)pSubObject, pbstr );
                    break;
                case DISPID_A_LISTSTYLEIMAGE:
                case DISPID_A_BACKGROUNDIMAGE:
                case DISPID_A_FONTFACESRC:
                case DISPID_A_BEHAVIOR:
                    {
                        CStr cstr;
                        if ( (*(CAttrArray **)pSubObject)->Find( dispid, CAttrValue::AA_Attribute ) )
                        {
                            BSTR bstrSub;
                            hr = HandleStringProperty( HANDLEPROP_AUTOMATION | (PROPTYPE_BSTR << 16), 
                                                       &bstrSub, pObject, pSubObject );
                            if ( hr == S_OK )
                            {
                                if ( bstrSub && *bstrSub )
                                {
                                    // CONSIDER (alexz) using Format, to remove the memallocs here

                                    if (dispid != DISPID_A_BEHAVIOR)
                                        cstr.Set( _T("url(") );

                                    cstr.Append( bstrSub );

                                    if (dispid != DISPID_A_BEHAVIOR)
                                        cstr.Append( _T(")") );
                                }
                                else
                                {   // We only get here if a NULL string was stored in the array; i.e., the value is NONE.
                                    cstr.Set( _T("none") );
                                }
                                FormsFreeString( bstrSub );
                            }
                        }
                        hr = cstr.AllocBSTR( pbstr );
                    }
                    break;

                case DISPID_A_TEXTAUTOSPACE:
                    hr = WriteTextAutospaceToBSTR( *(CAttrArray **)pSubObject, pbstr );
                    break;
                case DISPID_A_TEXTDECORATION:
                    hr = WriteTextDecorationToBSTR( *(CAttrArray **)pSubObject, pbstr );
                    break;

                case DISPID_A_TEXTDECORATIONNONE:
                case DISPID_A_TEXTDECORATIONUNDERLINE:
                case DISPID_A_TEXTDECORATIONOVERLINE:
                case DISPID_A_TEXTDECORATIONLINETHROUGH:
                case DISPID_A_TEXTDECORATIONBLINK:
                    if ( PROPTYPE( dwOpCode ) != PROPTYPE_VARIANT )
                    {
                        Assert( "Can't get/set text-decoration subproperties as anything but VARIANTs!" );
                        hr = S_FALSE;
                        goto Cleanup;
                    }

                    V_VT((VARIANT *)pv) = VT_BOOL;
                    ((VARIANT *)pv)->boolVal = 0;

                    if ( *((CAttrArray **)pSubObject) )
                    {   // See if we already have a text-decoration value
                        CAttrValue *pAV = (*((CAttrArray **)pSubObject))->Find( DISPID_A_TEXTDECORATION, CAttrValue::AA_Attribute );
                        if ( pAV )
                        {   // We do!  Copy its value into our working variant
                            long lVal = pAV->GetLong();

                            switch ( dispid )
                            {
                            case DISPID_A_TEXTDECORATIONNONE:
                                lVal &= TD_NONE;
                                break;
                            case DISPID_A_TEXTDECORATIONUNDERLINE:
                                lVal &= TD_UNDERLINE;
                                break;
                            case DISPID_A_TEXTDECORATIONOVERLINE:
                                lVal &= TD_OVERLINE;
                                break;
                            case DISPID_A_TEXTDECORATIONLINETHROUGH:
                                lVal &= TD_LINETHROUGH;
                                break;
                            case DISPID_A_TEXTDECORATIONBLINK:
                                lVal &= TD_BLINK;
                                break;
                            }
                            if ( lVal )
                                ((VARIANT *)pv)->boolVal = -1;
                        }
                    }
                    break;

                case DISPID_A_BORDERTOP:
                case DISPID_A_BORDERRIGHT:
                case DISPID_A_BORDERBOTTOM:
                case DISPID_A_BORDERLEFT:
                    hr = WriteBorderSidePropertyToBSTR( dispid, *(CAttrArray **)pSubObject, pbstr );
                    break;

                case DISPID_A_BORDER:
                    hr = WriteBorderToBSTR( *(CAttrArray **)pSubObject, pbstr );
                    break;

                case DISPID_A_MARGIN:
                case DISPID_A_PADDING:
                case DISPID_A_BORDERCOLOR:
                case DISPID_A_BORDERWIDTH:
                case DISPID_A_BORDERSTYLE:
                    hr = WriteExpandedPropertyToBSTR( dispid, *(CAttrArray **)pSubObject, pbstr );
                    break;

                case DISPID_A_LISTSTYLE:
                    hr = WriteListStyleToBSTR( *(CAttrArray **)pSubObject, pbstr );
                    break;
            
                case DISPID_A_BACKGROUNDPOSITION:
                    hr = WriteBackgroundPositionToBSTR( *(CAttrArray **)pSubObject, pbstr );
                    break;

                case DISPID_A_FONT:
                    if ( (*(CAttrArray **)pSubObject)->Find(DISPID_A_FONT, CAttrValue::AA_Attribute ) )
                        hr = HandleStringProperty(dwOpCode, pv, pObject, pSubObject);
                    else
                    {
                        // We need to cook up a "font" property.
                        hr = WriteFontToBSTR( *(CAttrArray **)pSubObject, pbstr );
                    }
                    break;

                case DISPID_A_LAYOUTGRID:
                    hr = WriteLayoutGridToBSTR( *(CAttrArray **)pSubObject, pbstr );
                    break;

                case DISPID_A_STYLETEXT:
                    hr = WriteStyleToBSTR( pObject, *(CAttrArray **)pSubObject, pbstr, FALSE );
                    break;

                case DISPID_A_CLIP:     // We need to cook up a "clip" property with the "rect" shape.
                    hr = WriteClipToBSTR( *(CAttrArray **)pSubObject, pbstr );
                    break;

                default:
                    Assert( "Unrecognized type being handled by CStyleUrl handler!" && FALSE );
                    break;
                }
                if ( hr == S_FALSE )
                    hr = FormsAllocString( _T(""), pbstr );
            }
            break;
        }
    }

Cleanup:
    RRETURN1(hr, S_FALSE);
}

