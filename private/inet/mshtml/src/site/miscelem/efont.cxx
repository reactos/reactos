//+---------------------------------------------------------------------
//
//   File:      efont.cxx
//
//  Contents:   Font element class
//
//  Classes:    CFontElement, CBaseFontElement
//
//------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_EFONT_HXX_
#define X_EFONT_HXX_
#include "efont.hxx"
#endif

#ifndef X_FORMKRNL_HXX_
#define X_FORMKRNL_HXX_
#include "formkrnl.hxx"
#endif

#ifndef X_PROPS_HXX_
#define X_PROPS_HXX_
#include "props.hxx"
#endif

#define _cxx_
#include "font.hdl"

#define _cxx_
#include "basefont.hdl"

MtDefine(CFontElement, Elements, "CFontElement")
MtDefine(CBaseFontElement, Elements, "CBaseFontElement")

const CElement::CLASSDESC CFontElement::s_classdesc =
{
    {
        &CLSID_HTMLFontElement,             // _pclsid
        0,                                  // _idrBase
#ifndef NO_PROPERTY_PAGE
        s_apclsidPages,                     // _apClsidPages
#endif // NO_PROPERTY_PAGE
        s_acpi,                             // _pcpi
        0,                                  // _dwFlags
        &IID_IHTMLFontElement,              // _piidDispinterface
        &s_apHdlDescs,                      // _apHdlDesc
    },
    (void *)s_apfnpdIHTMLFontElement,       //_apfnTearOff
    NULL,                                   // _pAccelsDesign
    NULL                                    // _pAccelsRun
};

HRESULT
CFontElement::CreateElement(CHtmTag *pht,
                              CDoc *pDoc, CElement **ppElement)
{
    Assert(pht->Is(ETAG_FONT));
    Assert(ppElement);
    *ppElement = new CFontElement(pDoc);
    return *ppElement ? S_OK: E_OUTOFMEMORY;
}



//+-------------------------------------------------------------------
//
//  Member:     Add Attributes
//
//  Synopsis:   For all unspecified attributes in 'this', the attributes
//              of pElement are merged in.  The caller is expected to
//              to delete the pElement after this operation.
//
//--------------------------------------------------------------------

HRESULT
CFontElement::CombineAttributes( CFontElement * pFont )
{
    HRESULT hr = S_OK;
    LPCTSTR pchFace;

    Assert( ETAG_FONT == pFont->Tag() );

    if (GetFontSize().IsNull())
    {
        hr = THR( SetAAsize(pFont->GetFontSize()) );
        if (hr)
            goto Cleanup;
    }

    if (!GetAAcolor().IsDefined())
    {
        hr = THR( SetAAcolor(pFont->GetAAcolor()) );
        if (hr)
            goto Cleanup;
    }

    pchFace = GetAAface();
    if ( !(pchFace && pchFace[0]) )
    {
        hr = THR( SetAAface(pFont->GetAAface()) );
        if (hr)
            goto Cleanup;
    }

Cleanup:
    RRETURN(hr);
}

//+-------------------------------------------------------------------
//
//  Member:     RemoveAttributes
//
//  Synopsis:   Remove attributes specified in pFont for 'this.'
//              If all attributes were stripped, pfAttrBagEmpty
//              return true and the caller should free this object.
//
//--------------------------------------------------------------------

HRESULT
CFontElement::RemoveAttributes(
    CFontElement *pFont,
    BOOL * pfAttrBagEmpty )
{
    HRESULT hr=S_OK;
    BOOL fKeepThis = FALSE;
    
    Assert( ETAG_FONT == pFont->Tag() );

    if (!GetFontSize().IsNull())
    {
        if (!pFont->GetFontSize().IsNull())
        {
            CUnitValue cuv;
            cuv.SetNull();
            IGNORE_HR( SetAAsize(cuv) );
        }
        else
        {
            fKeepThis = TRUE;
        }
    }        

    if (GetAAcolor().IsDefined())
    {
        if (pFont->GetAAcolor().IsDefined())
        {
            IGNORE_HR( SetAAcolor(VALUE_UNDEF) );
        }
        else
        {
            fKeepThis = TRUE;
        }
    }

    LPCTSTR pchFace = GetAAface();
    if ( pchFace && pchFace[0] )
    {
        pchFace = pFont->GetAAface();
        if (pchFace && pchFace[0])
        {
            hr = THR( SetAAface(NULL) );
            if (hr)
                goto Cleanup;
        }
        else
        {
            fKeepThis = TRUE;
        }
    }

Cleanup:
    *pfAttrBagEmpty = !fKeepThis;

    RRETURN(hr);
}

//+----------------------------------------------------------------------------
//  Member:     CFontElement::ApplyDefaultFormat()
//
//  Synopsis:   Apply the default formats of the font element.
//-----------------------------------------------------------------------------
HRESULT
CFontElement::ApplyDefaultFormat(CFormatInfo * pCFI)
{
    HRESULT hr;

    // If this is an empty font tag, it should reset the font size
    if(!_pAA)
    {
        CUnitValue cuv;

        cuv.SetValue(0, CUnitValue::UNIT_RELATIVE);

        hr = THR(SetAAsize(cuv));
        if (hr)
            goto Cleanup;
    }

    hr = THR(super::ApplyDefaultFormat(pCFI));
Cleanup:
    RRETURN(hr);
}


//+-----------------------------------------------------
//
//  Member : CBaseFontElement::GetFontSize()
//
//  Synopsis : a wrapper function for the AttrArray access
//      method.
//
//+------------------------------------------------------
CUnitValue
CFontElement::GetFontSize( void )
{
    CUnitValue uvSize= GetAAsize();
    long lAASize = uvSize.GetUnitValue(); 

    //if it is valid
    lAASize = (lAASize<MIN_FONT_SIZE) ? MIN_FONT_SIZE : lAASize;
    lAASize = (lAASize>MAX_FONT_SIZE) ? MAX_FONT_SIZE : lAASize;

    uvSize.SetValue(lAASize, uvSize.GetUnitType());

    return uvSize;
}
//============================================================
//
//  CBaseFontElement Methods
//
//============================================================

const CElement::CLASSDESC CBaseFontElement::s_classdesc =
{
    {
        &CLSID_HTMLBaseFontElement,         // _pclsid
        0,                                  // _idrBase
#ifndef NO_PROPERTY_PAGE
        s_apclsidPages,                     // _apClsidPages
#endif // NO_PROPERTY_PAGE
        s_acpi,                             // _pcpi
        ELEMENTDESC_NOLAYOUT,               // _dwFlags
        &IID_IHTMLBaseFontElement,          // _piidDispinterface
        &s_apHdlDescs,                      // _apHdlDesc
    },
    (void *)s_apfnpdIHTMLBaseFontElement,   // _ApfnTearOff
    NULL,                                   // _pAccelsDesign
    NULL                                    // _pAccelsRun
};

HRESULT
CBaseFontElement::CreateElement(CHtmTag *pht,
                              CDoc *pDoc, CElement **ppElement)
{
    Assert(pht->Is(ETAG_BASEFONT));
    Assert(ppElement);
    *ppElement = new CBaseFontElement(pDoc);
    return *ppElement ? S_OK: E_OUTOFMEMORY;
}

//+-----------------------------------------------------
//
//  Member : CBaseFontElement::GetFontSize()
//
//  Synopsis : a wrapper function for the AttrArray access
//      method.
//
//+------------------------------------------------------
long
CBaseFontElement::GetFontSize( void )
{
    long lAASize = GetAAsize(); 

    //if it is valid
    lAASize = (lAASize<MIN_BASEFONT) ? MIN_BASEFONT : lAASize;
    lAASize = (lAASize>MAX_BASEFONT) ? MAX_BASEFONT : lAASize;


    return lAASize;
}
