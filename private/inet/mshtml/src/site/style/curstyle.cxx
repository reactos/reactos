//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1994-1997
//
//  File:       curstyle.cxx
//
//  Contents:   Support for CElement::CurrentStyle property
//
//----------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_INTSHCUT_H_
#define X_INTSHCUT_H_
#include "intshcut.h"
#endif

#ifndef X_FORMKRNL_HXX_
#define X_FORMKRNL_HXX_
#include "formkrnl.hxx"
#endif

#ifndef X_DOWNLOAD_HXX_
#define X_DOWNLOAD_HXX_
#include "download.hxx"
#endif

#ifndef X_CSITE_HXX_
#define X_CSITE_HXX_
#include "csite.hxx"
#endif

#ifndef X_STYLE_HXX_
#define X_STYLE_HXX_
#include "style.hxx"
#endif

#ifndef X_CURSTYLE_HXX_
#define X_CURSTYLE_HXX_
#include "curstyle.hxx"
#endif

#ifndef X_CBUFSTR_HXX_
#define X_CBUFSTR_HXX_
#include "cbufstr.hxx"
#endif

#ifndef X_PEER_HXX_
#define X_PEER_HXX_
#include "peer.hxx"
#endif

#ifndef X_PEERFACT_HXX_
#define X_PEERFACT_HXX_
#include "peerfact.hxx"
#endif

#ifndef X_QI_IMPL_H_
#define X_QI_IMPL_H_
#include "qi_impl.h"
#endif

#ifndef X_MISCPROT_H_
#define X_MISCPROT_H_
#include "miscprot.h"
#endif

#ifndef X_MSHTMDID_H_
#define X_MSHTMDID_H_
#include "mshtmdid.h"
#endif

#define _cxx_
#include "curstyle.hdl"

MtDefine(CCurrentStyle, StyleSheets, "CCurrentStyle")

#define GetUnitValueWithEnumDef(p, Uv, pDesc, enumname, nDefEnumValue) \
    GetUnitValueHelper(p, Uv, pDesc, &s_enumdesc##enumname, nDefEnumValue)
#define GetUnitValueWithEnum(p, Uv, pDesc, enumname) \
    GetUnitValueHelper(p, Uv, pDesc, &s_enumdesc##enumname)

#define NULLPTRCHECK(ptr, err)  if (!ptr) \
    {   \
        hr = err; \
        goto Cleanup;   \
    }

//
// marka temporary fix for SupeHot 33205. We may have deleted the elemnet
// after we get the current style the first time.
// becuase of a ref-counting problem - we can't now replace the node
// talk to jbeda and rgardner about replacing this.
//
#define NODE_IN_TREE_CHECK() CTreeNode* pNodeToUse = _pNode; \
    if( ! _pNode->IsInMarkup() ) \
    { \
        pNodeToUse = _pNode->Element()->GetFirstBranch(); \
        NULLPTRCHECK(pNodeToUse, E_POINTER) \
    }

BEGIN_TEAROFF_TABLE(CCurrentStyle, IRecalcProperty)
    TEAROFF_METHOD(CCurrentStyle, GetCanonicalProperty, getcanonicalproperty, (DISPID dispid, IUnknown **ppUnk, DISPID *pdispid))
END_TEAROFF_TABLE()


//+------------------------------------------------------------------------
//
//  Member:     CCurrentStyle::CCurrentStyle
//
//-------------------------------------------------------------------------
CCurrentStyle::CCurrentStyle(CTreeNode *pNode)
{
    WHEN_DBG(_dwCookie=eCookie;)

    Assert ( pNode );

    _pNode = pNode;

    _pNode->NodeAddRef();
}

//+------------------------------------------------------------------------
//
//  Member:     CCurrentStyle::Passivate
//
//-------------------------------------------------------------------------
void
CCurrentStyle::Passivate()
{
    super::Passivate();

    // Remove myself from the node's lookaside
    if( _pNode->HasCurrentStyle() && this == _pNode->GetCurrentStyle() )
    {
        Verify( this == _pNode->DelCurrentStyle() );
    }

    _pNode->NodeRelease();
}

//+------------------------------------------------------------------------
//
//  Member:     ClassDesc
//
//-------------------------------------------------------------------------
const CCurrentStyle::CLASSDESC CCurrentStyle::s_classdesc =
{
    &CLSID_HTMLCurrentStyle,       // _pclsid
    0,                             // _idrBase
#ifndef NO_PROPERTY_PAGE
    NULL,                          // _apClsidPages
#endif // NO_PROPERTY_PAGE
    NULL,                          // _pcpi
    0,                             // _dwFlags
    &IID_IHTMLCurrentStyle,        // _piidDispinterface
    &s_apHdlDescs,                 // _apHdlDesc
};



//+------------------------------------------------------------------------
//
//  Member:     CCurrentStyle::PrivateQueryInterface, IUnknown
//
//  Synopsis:   Private unknown QI.
//
//-------------------------------------------------------------------------

HRESULT
CCurrentStyle::PrivateQueryInterface(REFIID iid, void ** ppv)
{
    HRESULT hr = S_OK;

    AssertSz(eCookie==_dwCookie, "NOT A CCurrentSTYLE");

    *ppv = NULL;

    switch (iid.Data1)
    {
    QI_INHERITS((IPrivateUnknown *)this, IUnknown)
    QI_TEAROFF_DISPEX(this, NULL)
    QI_TEAROFF((CBase *)this, IPerPropertyBrowsing, NULL)
    QI_TEAROFF((CBase *)this, IRecalcProperty, NULL)
    default:
        if (iid == IID_IHTMLCurrentStyle)
        {
            hr = THR(CreateTearOffThunk(
                    this,
                    s_apfnIHTMLCurrentStyle,
                    NULL,
                    ppv));
        }
    }

    if (*ppv)
    {
        ((IUnknown*)*ppv)->AddRef();
        return S_OK;
    }

    RRETURN(E_NOINTERFACE);
}


//+----------------------------------------------------------------
//
//  member : get_position
//
//  Synopsis : IHTMLCurrentStyle property. returns the position enum
//
//+----------------------------------------------------------------

HRESULT
CCurrentStyle::get_position( BSTR *pbstr )
{
    HRESULT         hr;
    stylePosition   stPos;
    NODE_IN_TREE_CHECK()
    NULLPTRCHECK(pbstr, E_POINTER)

    stPos = pNodeToUse->GetCascadedposition();
    if(stPos == stylePositionNotSet)
        stPos = stylePositionstatic;

   // get the positioning, and then its string
    hr = THR(STRINGFROMENUM(stylePosition, stPos, pbstr));

Cleanup:
    RRETURN(SetErrorInfo( hr ));
}

//+----------------------------------------------------------------
//
//  member : get_styleFloat
//
//  Synopsis : IHTMLCurrentStyle property. returns the styleDloatenum
//
//+----------------------------------------------------------------

HRESULT
CCurrentStyle::get_styleFloat( BSTR *pbstr )
{
    HRESULT         hr;
    styleStyleFloat stFloat;
    NODE_IN_TREE_CHECK()
    NULLPTRCHECK(pbstr, E_POINTER)

    stFloat = pNodeToUse->GetCascadedfloat();

    if(stFloat == styleStyleFloatNotSet)
        stFloat = styleStyleFloatNone;

    hr = THR(STRINGFROMENUM(styleStyleFloat, stFloat, pbstr));

Cleanup:
    RRETURN(SetErrorInfo( hr ));
}

//+----------------------------------------------------------------
//
//  member : get_color
//
//  Synopsis : IHTMLCurrentStyle property. returns the current color
//
//+----------------------------------------------------------------

HRESULT
CCurrentStyle::get_color(VARIANT * p)
{
    HRESULT hr;
    NODE_IN_TREE_CHECK()

    hr = THR(GetColorHelper(p, pNodeToUse->GetCascadedcolor()));

Cleanup:
    RRETURN(SetErrorInfo( hr ));
}

//+----------------------------------------------------------------
//
//  member : get_backgroundColor
//
//  Synopsis : IHTMLCurrentStyle property. returns the current backgroundColor
//
//+----------------------------------------------------------------

HRESULT
CCurrentStyle::get_backgroundColor(VARIANT * p)
{
    HRESULT      hr;
    CColorValue  colVal;
    NODE_IN_TREE_CHECK()
    NULLPTRCHECK(p, E_POINTER)
    TCHAR        szBuffer[pdlColor];


    V_VT(p) = VT_EMPTY;
    colVal = pNodeToUse->GetCascadedbackgroundColor();
    if (!colVal.IsDefined())
    {
        hr = THR(colVal.SetValue(0, FALSE, CColorValue::TYPE_TRANSPARENT));
        if (hr)
            goto Cleanup;
    }
    
    hr = THR(colVal.FormatBuffer(szBuffer, ARRAY_SIZE(szBuffer), NULL));
    if(hr)
        goto Cleanup;

    V_VT(p) = VT_BSTR;
    hr = THR(FormsAllocString(szBuffer, &(V_BSTR(p))));
    if (hr)
        goto Cleanup;
        
Cleanup:
    RRETURN(SetErrorInfo( hr ));
}


//+----------------------------------------------------------------
//
//  member : get_fontFamily
//
//  Synopsis : IHTMLCurrentStyle property
//
//+----------------------------------------------------------------

HRESULT
CCurrentStyle::get_fontFamily(BSTR * p)
{
    HRESULT         hr;
    LPCTSTR         pchName;
    VARIANT         varValue;
    CElement *      pElement;

    NODE_IN_TREE_CHECK()
    pElement = pNodeToUse->SafeElement();

    NULLPTRCHECK(p, E_POINTER)
    NULLPTRCHECK(pElement, E_FAIL)

    hr = THR(pElement->ComputeExtraFormat(DISPID_A_FONTFACE, TRUE, pNodeToUse, &varValue));
    if(hr)
        goto Cleanup;

    if(((CVariant *)&varValue)->IsEmpty())
    {
        // Return the rendered values

        // Try to get the font name
        pchName = pNodeToUse->GetCascadedfontFaceName();
        if(!pchName  || !(*pchName))
        {
            // Get the generic family name
            pchName = pNodeToUse->GetCascadedfontFamilyName();
            if(!pchName  || !(*pchName))
            {
                *p = NULL;
                hr = S_OK;
                goto Cleanup;
            }
        }
    }
    else
    {
        Assert(V_VT(&varValue) == VT_BSTR || V_VT(&varValue) == VT_LPWSTR);
        pchName = V_BSTR(&varValue);
    }

    hr = THR(FormsAllocString(pchName, p));

Cleanup:
    RRETURN(SetErrorInfo( hr ));
}


//+----------------------------------------------------------------
//
//  member : get_fontStyle
//
//  Synopsis : IHTMLCurrentStyle property
//
//+----------------------------------------------------------------

HRESULT
CCurrentStyle::get_fontStyle(BSTR * p)
{
    HRESULT         hr;
    styleFontStyle  sty;
    VARIANT         varValue;
    CElement *      pElement;
    NODE_IN_TREE_CHECK()
    pElement = pNodeToUse->SafeElement();

    NULLPTRCHECK(pElement, E_FAIL)
    NULLPTRCHECK(p, E_POINTER)

    hr = THR(pElement->ComputeExtraFormat(DISPID_A_FONTSTYLE, TRUE, pNodeToUse, &varValue));
    if(hr)
        goto Cleanup;

    if( !((CVariant *)&varValue)->IsEmpty() )
        sty = (styleFontStyle) V_I4(&varValue);
    else
        sty = (pNodeToUse->GetCascadedfontItalic()) ? styleFontStyleItalic : styleFontStyleNormal;

    Assert((pNodeToUse->GetCascadedfontItalic() &&  (sty == styleFontStyleOblique || sty == styleFontStyleItalic) )
            || (!pNodeToUse->GetCascadedfontItalic() && sty != styleFontStyleOblique && sty != styleFontStyleItalic) );

    hr = THR(STRINGFROMENUM(styleFontStyle, sty, p));

Cleanup:
    RRETURN(SetErrorInfo( hr ));
}


//+----------------------------------------------------------------
//
//  member : get_fontVariant
//
//  Synopsis : IHTMLCurrentStyle property
//
//+----------------------------------------------------------------

HRESULT
CCurrentStyle::get_fontVariant(BSTR * p)
{
    HRESULT           hr;
    styleFontVariant  sty;
    VARIANT           varValue;
    CElement *        pElement;
    NODE_IN_TREE_CHECK()
    pElement = pNodeToUse->SafeElement();

    NULLPTRCHECK(pElement, E_FAIL)
    NULLPTRCHECK(p, E_POINTER)

    hr = THR(pElement->ComputeExtraFormat(DISPID_A_FONTVARIANT, TRUE, pNodeToUse, &varValue));
    if(hr)
        goto Cleanup;

    sty = (((CVariant *)&varValue)->IsEmpty())
                        ? styleFontVariantNormal
                        : (styleFontVariant) V_I4(&varValue);

    hr = THR(STRINGFROMENUM(styleFontVariant, sty, p));

Cleanup:
    RRETURN(SetErrorInfo( hr ));
}


//+----------------------------------------------------------------
//
//  member : get_fontWeight
//
//  Synopsis : IHTMLCurrentStyle property
//
//+----------------------------------------------------------------

HRESULT
CCurrentStyle::get_fontWeight(VARIANT * p)
{
    HRESULT      hr = S_OK;
    WORD         wWeight;
    NODE_IN_TREE_CHECK()
    NULLPTRCHECK(p, E_POINTER)

    wWeight = pNodeToUse->GetCascadedfontWeight();
    V_VT(p) = VT_I4;
    V_I4(p) = wWeight;

Cleanup:
    RRETURN(SetErrorInfo( hr ));
}


//+----------------------------------------------------------------
//
//  member : get_fontSize
//
//  Synopsis : IHTMLCurrentStyle property
//
//+----------------------------------------------------------------

HRESULT
CCurrentStyle::get_fontSize(VARIANT * p)
{
    HRESULT      hr = S_OK;
    LONG         lSize;
    CVariant     varValue;
    CElement   * pElement;
    CUnitValue * pVal;
    CUnitValue   cuv;
    NODE_IN_TREE_CHECK()
    pElement = pNodeToUse->SafeElement();

    NULLPTRCHECK(pElement, E_FAIL)
    NULLPTRCHECK(p, E_POINTER)

    // Full font size information is not stored in CF so we need to walk
    //  the element tree and apply the formats
    hr = THR(pElement->ComputeExtraFormat(DISPID_A_FONTSIZE, TRUE, pNodeToUse, &varValue));
    if(hr)
        goto Cleanup;

    if(varValue.IsEmpty())
    {
        CDoc *pDoc = pNodeToUse->Doc();

        Assert(pDoc);

        lSize = pNodeToUse->GetCharFormat()->GetHeightInTwips(pDoc);

        lSize = MulDivQuick(lSize, 1000, TWIPS_PER_POINT);
        cuv.SetValue(lSize, CUnitValue::UNIT_POINT);

        pVal = &cuv;
    }
   else
    {
        pVal = (CUnitValue*) (void*) &V_I4(&varValue);
    }

   hr = THR(GetUnitValueWithEnum(p, *pVal,
                        &(s_propdescCCurrentStylefontSize.a), styleFontSize));

Cleanup:
    RRETURN(SetErrorInfo( hr ));
}


//+----------------------------------------------------------------
//
//  member : get_layoutGridChar
//
//  Synopsis : IHTMLCurrentStyle property
//
//+----------------------------------------------------------------

HRESULT
CCurrentStyle::get_layoutGridChar(VARIANT * p)
{
    HRESULT      hr = S_OK;
    CUnitValue   cuv;
    NODE_IN_TREE_CHECK()

    NULLPTRCHECK(p, E_POINTER)

    cuv = pNodeToUse->GetCascadedlayoutGridChar();
    hr = THR(GetUnitValueWithEnumDef(p, cuv, 
                                     &(s_propdescCCurrentStylelayoutGridChar.a), styleLayoutGridChar,
                                     styleLayoutGridCharNone));
Cleanup:
    RRETURN(SetErrorInfo( hr ));
}


//+----------------------------------------------------------------
//
//  member : get_layoutGridLine
//
//  Synopsis : IHTMLCurrentStyle property
//
//+----------------------------------------------------------------

HRESULT
CCurrentStyle::get_layoutGridLine(VARIANT * p)
{
    HRESULT      hr = S_OK;
    CUnitValue   cuv;
    NODE_IN_TREE_CHECK()

    NULLPTRCHECK(p, E_POINTER)

    cuv = pNodeToUse->GetCascadedlayoutGridLine();
    hr = THR(GetUnitValueWithEnumDef(p, cuv, 
                                     &(s_propdescCCurrentStylelayoutGridLine.a), styleLayoutGridLine,
                                     styleLayoutGridLineNone));
Cleanup:
    RRETURN(SetErrorInfo( hr ));
}


//+----------------------------------------------------------------
//
//  member : get_layoutGridMode
//
//  Synopsis : IHTMLCurrentStyle property
//
//+----------------------------------------------------------------

HRESULT
CCurrentStyle::get_layoutGridMode(BSTR * pbstr)
{
    HRESULT         hr;
    styleLayoutGridMode  sty;
    NODE_IN_TREE_CHECK()

    NULLPTRCHECK(pbstr, E_POINTER)

    sty = pNodeToUse->GetCascadedlayoutGridMode();
        
    hr = THR(STRINGFROMENUM(styleLayoutGridMode, sty, pbstr));

Cleanup:
    RRETURN(SetErrorInfo( hr ));

}


//+----------------------------------------------------------------
//
//  member : get_layoutGridType
//
//  Synopsis : IHTMLCurrentStyle property
//
//+----------------------------------------------------------------

HRESULT
CCurrentStyle::get_layoutGridType(BSTR * pbstr)
{
    HRESULT         hr;
    styleLayoutGridType  sty;
    NODE_IN_TREE_CHECK()

    NULLPTRCHECK(pbstr, E_POINTER)

    sty = pNodeToUse->GetCascadedlayoutGridType();
        
    hr = THR(STRINGFROMENUM(styleLayoutGridType, sty, pbstr));

Cleanup:
    RRETURN(SetErrorInfo( hr ));

}


//+----------------------------------------------------------------
//
//  member : get_backgroundImage
//
//  Synopsis : IHTMLCurrentStyle property
//
//+----------------------------------------------------------------

HRESULT
CCurrentStyle::get_backgroundImage(BSTR * p)
{
    HRESULT      hr;
    long         lCookie;
    NODE_IN_TREE_CHECK()
    NULLPTRCHECK(p, E_POINTER)

    lCookie = pNodeToUse->GetCascadedbackgroundImageCookie();

    if(lCookie != 0)
    {
        hr = THR(GetImageNameFromCookie(lCookie, p));
    }
    else
    {
        hr = THR(FormsAllocString(_T("none"), p));
    }

Cleanup:
    RRETURN(SetErrorInfo( hr ));
}


//+----------------------------------------------------------------
//
//  member : get_backgroundPositionX
//
//  Synopsis : IHTMLCurrentStyle property
//
//+----------------------------------------------------------------

HRESULT
CCurrentStyle::get_backgroundPositionX(VARIANT * p)
{
    HRESULT         hr;
    VARIANT         varValue;
    CUnitValue      uvBkgnd;
    CElement *      pElement;
    NODE_IN_TREE_CHECK()
    pElement = pNodeToUse->SafeElement();

    NULLPTRCHECK(pElement, E_FAIL)
    NULLPTRCHECK(p, E_POINTER)

    // Information is modified before being stored in CF/PF/FF so we need to walk
    //  the element tree and apply the formats in a special mode to get the original value
    hr = THR(pElement->ComputeExtraFormat(DISPID_A_BACKGROUNDPOSX, FALSE, pNodeToUse, &varValue));
    if(hr)
        goto Cleanup;

    if(((CVariant *)&varValue)->IsEmpty())
        uvBkgnd.SetRawValue(MAKEUNITVALUE(0, UNIT_PERCENT));
    else
        uvBkgnd = *(CUnitValue *)&V_I4(&varValue);

    hr = GetUnitValueWithEnum(p, uvBkgnd,
        &(s_propdescCCurrentStylebackgroundPositionX.a), styleBackgroundPositionX);

Cleanup:
    RRETURN(SetErrorInfo(hr));
}


//+----------------------------------------------------------------
//
//  member : get_backgroundPositionY
//
//  Synopsis : IHTMLCurrentStyle property
//
//+----------------------------------------------------------------

HRESULT
CCurrentStyle::get_backgroundPositionY(VARIANT * p)
{
    HRESULT         hr;
    VARIANT         varValue;
    CUnitValue      uvBkgnd;
    CElement *      pElement;
    NODE_IN_TREE_CHECK()
    pElement = pNodeToUse->SafeElement();

    NULLPTRCHECK(pElement, E_FAIL)
    NULLPTRCHECK(p, E_POINTER)

    // Information is modified before being stored in CF/PF/FF so we need to walk
    //  the element tree and apply the formats in a special mode to get the original value
    hr = THR(pElement->ComputeExtraFormat(DISPID_A_BACKGROUNDPOSY, FALSE, pNodeToUse, &varValue));
    if(hr)
        goto Cleanup;

    if(((CVariant *)&varValue)->IsEmpty())
        uvBkgnd.SetRawValue(MAKEUNITVALUE(0, UNIT_PERCENT));
    else
        uvBkgnd = *(CUnitValue *)&V_I4(&varValue);

    hr = GetUnitValueWithEnum(p, uvBkgnd,
        &(s_propdescCCurrentStylebackgroundPositionY.a), styleBackgroundPositionY);

Cleanup:
    RRETURN(SetErrorInfo(hr));
}


//+----------------------------------------------------------------
//
//  member : get_backgroundRepeat
//
//  Synopsis : IHTMLCurrentStyle property
//
//+----------------------------------------------------------------

HRESULT
CCurrentStyle::get_backgroundRepeat(BSTR * p)
{
    HRESULT                 hr;
    BOOL                    fRepeatX, fRepeatY;
    styleBackgroundRepeat   eBr;
    NODE_IN_TREE_CHECK()
    NULLPTRCHECK(p, E_POINTER)

    fRepeatX = pNodeToUse->GetCascadedbackgroundRepeatX();
    fRepeatY = pNodeToUse->GetCascadedbackgroundRepeatY();

    if(fRepeatX)
        if(fRepeatY)
            eBr = styleBackgroundRepeatRepeat;
        else
            eBr = styleBackgroundRepeatRepeatX;
    else
        if(fRepeatY)
            eBr = styleBackgroundRepeatRepeatY;
        else
            eBr = styleBackgroundRepeatNoRepeat;

    // Convert to string
    hr = THR(STRINGFROMENUM( styleBackgroundRepeat, eBr, p));

Cleanup:
    RRETURN(SetErrorInfo( hr ));
}


//+----------------------------------------------------------------
//
//  member : get_borderLeftColor
//
//  Synopsis : IHTMLCurrentStyle property
//
//+----------------------------------------------------------------

HRESULT
CCurrentStyle::get_borderLeftColor(VARIANT * p)
{
    HRESULT hr;
    NODE_IN_TREE_CHECK()
    {
        CColorValue cv = pNodeToUse->GetCascadedborderLeftColor();
        if(!cv.IsDefined())
            cv = pNodeToUse->GetCascadedcolor();

        hr = THR(GetColorHelper(p, cv));
    }

Cleanup:
    RRETURN(SetErrorInfo( hr ));
}



//+----------------------------------------------------------------
//
//  member : get_borderTopColor
//
//  Synopsis : IHTMLCurrentStyle property
//
//+----------------------------------------------------------------

HRESULT
CCurrentStyle::get_borderTopColor(VARIANT * p)
{
    HRESULT hr;
    NODE_IN_TREE_CHECK()
    hr = THR(GetColorHelper(p, pNodeToUse->GetCascadedborderTopColor()));
Cleanup:
    RRETURN(SetErrorInfo( hr ));
}


//+----------------------------------------------------------------
//
//  member : get_borderRightColor
//
//  Synopsis : IHTMLCurrentStyle property
//
//+----------------------------------------------------------------

HRESULT
CCurrentStyle::get_borderRightColor(VARIANT * p)
{
    HRESULT hr;
    NODE_IN_TREE_CHECK()
    hr = THR(GetColorHelper(p, pNodeToUse->GetCascadedborderRightColor()));
Cleanup:
    RRETURN(SetErrorInfo( hr ));
}


//+----------------------------------------------------------------
//
//  member : get_borderBottomColor
//
//  Synopsis : IHTMLCurrentStyle property
//
//+----------------------------------------------------------------

HRESULT
CCurrentStyle::get_borderBottomColor(VARIANT * p)
{
    HRESULT hr;
    NODE_IN_TREE_CHECK()
    
    hr = THR(GetColorHelper(p, pNodeToUse->GetCascadedborderBottomColor()));

Cleanup:
    RRETURN(SetErrorInfo( hr ));
}



//+----------------------------------------------------------------
//
//  member : get_borderTopStyle
//
//  Synopsis : IHTMLCurrentStyle property
//
//+----------------------------------------------------------------

HRESULT
CCurrentStyle::get_borderTopStyle(BSTR * p)
{
    HRESULT             hr;
    styleBorderStyle    bdrStyle;

    NODE_IN_TREE_CHECK()
    NULLPTRCHECK(p, E_POINTER)

    bdrStyle = GetBorderStyle(BORDER_TOP);

    hr = THR(STRINGFROMENUM(styleBorderStyle, bdrStyle, p));

Cleanup:
    RRETURN(SetErrorInfo( hr ));
}


//+----------------------------------------------------------------
//
//  member : get_borderRightStyle
//
//  Synopsis : IHTMLCurrentStyle property
//
//+----------------------------------------------------------------

HRESULT
CCurrentStyle::get_borderRightStyle(BSTR * p)
{
    HRESULT             hr;
    styleBorderStyle    bdrStyle;

    NODE_IN_TREE_CHECK()
    NULLPTRCHECK(p, E_POINTER)

    bdrStyle = GetBorderStyle(BORDER_RIGHT);

    hr = THR(STRINGFROMENUM(styleBorderStyle, bdrStyle, p));

Cleanup:
    RRETURN(SetErrorInfo( hr ));
}


//+----------------------------------------------------------------
//
//  member : get_borderBottomStyle
//
//  Synopsis : IHTMLCurrentStyle property
//
//+----------------------------------------------------------------

HRESULT
CCurrentStyle::get_borderBottomStyle(BSTR * p)
{
    HRESULT             hr;
    styleBorderStyle    bdrStyle;

    NODE_IN_TREE_CHECK()
    NULLPTRCHECK(p, E_POINTER)

    bdrStyle = GetBorderStyle(BORDER_BOTTOM);

    hr = THR(STRINGFROMENUM(styleBorderStyle, bdrStyle, p));

Cleanup:
    RRETURN(SetErrorInfo( hr ));
}


//+----------------------------------------------------------------
//
//  member : get_borderLeftStyle
//
//  Synopsis : IHTMLCurrentStyle property
//
//+----------------------------------------------------------------

HRESULT
CCurrentStyle::get_borderLeftStyle(BSTR * p)
{
    HRESULT             hr;
    styleBorderStyle    bdrStyle;

    NODE_IN_TREE_CHECK()
    NULLPTRCHECK(p, E_POINTER)

    bdrStyle = GetBorderStyle(BORDER_LEFT);

    hr = THR(STRINGFROMENUM(styleBorderStyle, bdrStyle, p));

Cleanup:
    RRETURN(SetErrorInfo( hr ));
}


//+----------------------------------------------------------------
//
//  member : get_borderTopWidth
//
//  Synopsis : IHTMLCurrentStyle property
//
//+----------------------------------------------------------------

HRESULT
CCurrentStyle::get_borderTopWidth(VARIANT * p)
{
    HRESULT hr;
    NODE_IN_TREE_CHECK()
    {
        CUnitValue uvWidth = pNodeToUse->GetCascadedborderTopWidth();

        hr = THR(GetUnitValueWithEnumDef(p, uvWidth,
                            &(s_propdescCCurrentStyleborderTopWidth.a),
                            styleBorderWidth, styleBorderWidthMedium));
    }
Cleanup:
    RRETURN(SetErrorInfo( hr ));
}


//+----------------------------------------------------------------
//
//  member : get_borderRightWidth
//
//  Synopsis : IHTMLCurrentStyle property
//
//+----------------------------------------------------------------

HRESULT
CCurrentStyle::get_borderRightWidth(VARIANT * p)
{
    HRESULT hr;
    NODE_IN_TREE_CHECK()
    {
        CUnitValue uvWidth = pNodeToUse->GetCascadedborderRightWidth();

        hr = THR(GetUnitValueWithEnumDef(p, uvWidth,
                            &(s_propdescCCurrentStyleborderRightWidth.a),
                            styleBorderWidth, styleBorderWidthMedium));
    }
Cleanup:
    RRETURN(SetErrorInfo( hr ));
}


//+----------------------------------------------------------------
//
//  member : get_borderBottomWidth
//
//  Synopsis : IHTMLCurrentStyle property
//
//+----------------------------------------------------------------

HRESULT
CCurrentStyle::get_borderBottomWidth(VARIANT * p)
{
    HRESULT hr;
    NODE_IN_TREE_CHECK()
    {
        CUnitValue uvWidth = pNodeToUse->GetCascadedborderBottomWidth();

        hr = THR(GetUnitValueWithEnumDef(p, uvWidth,
            &(s_propdescCCurrentStyleborderBottomWidth.a),
                            styleBorderWidth, styleBorderWidthMedium));
    }
Cleanup:
    RRETURN(SetErrorInfo( hr ));
}


//+----------------------------------------------------------------
//
//  member : get_borderLeftWidth
//
//  Synopsis : IHTMLCurrentStyle property
//
//+----------------------------------------------------------------

HRESULT
CCurrentStyle::get_borderLeftWidth(VARIANT * p)
{
    HRESULT hr;
    NODE_IN_TREE_CHECK()
    {
        CUnitValue uvWidth = pNodeToUse->GetCascadedborderLeftWidth();

        hr = THR(GetUnitValueWithEnumDef(p, uvWidth,
                        &(s_propdescCCurrentStyleborderLeftWidth.a),
                            styleBorderWidth, styleBorderWidthMedium));
    }
Cleanup:
    RRETURN(SetErrorInfo( hr ));
}


//+----------------------------------------------------------------
//
//  member : get_left
//
//  Synopsis : IHTMLCurrentStyle property
//
//+----------------------------------------------------------------

HRESULT
CCurrentStyle::get_left(VARIANT * p)
{
    HRESULT hr;
    NODE_IN_TREE_CHECK()
    hr = THR(GetUnitValueWithEnumDef(p, pNodeToUse->GetCascadedleft(),
        &(s_propdescCCurrentStyleleft.a), styleAuto, styleAutoAuto));

Cleanup:
    RRETURN(SetErrorInfo( hr ));
}

//+----------------------------------------------------------------
//
//  member : get_right
//
//  Synopsis : IHTMLCurrentStyle property
//
//+----------------------------------------------------------------

HRESULT
CCurrentStyle::get_right(VARIANT * p)
{
    HRESULT hr;
    NODE_IN_TREE_CHECK()
    hr = THR(GetUnitValueWithEnumDef(p, pNodeToUse->GetCascadedright(), 
                &(s_propdescCCurrentStyleright.a), styleAuto, styleAutoAuto));

Cleanup:
    RRETURN(SetErrorInfo( hr ));
}


//+----------------------------------------------------------------
//
//  member : get_top
//
//  Synopsis : IHTMLCurrentStyle property
//
//+----------------------------------------------------------------

HRESULT
CCurrentStyle::get_top(VARIANT * p)
{
    HRESULT hr;
    NODE_IN_TREE_CHECK()
    hr = THR(GetUnitValueWithEnumDef(p, pNodeToUse->GetCascadedtop(),
        &(s_propdescCCurrentStyletop.a), styleAuto, styleAutoAuto));

Cleanup:
    RRETURN(SetErrorInfo( hr ));
}


//+----------------------------------------------------------------
//
//  member : get_bottom
//
//  Synopsis : IHTMLCurrentStyle property
//
//+----------------------------------------------------------------

HRESULT
CCurrentStyle::get_bottom(VARIANT * p)
{
    HRESULT hr;
    NODE_IN_TREE_CHECK()
    hr = THR(GetUnitValueWithEnumDef(p, pNodeToUse->GetCascadedbottom(), 
         &(s_propdescCCurrentStylebottom.a), styleAuto, styleAutoAuto));

Cleanup:
    RRETURN(SetErrorInfo( hr ));
}


//+----------------------------------------------------------------
//
//  member : get_width
//
//  Synopsis : IHTMLCurrentStyle property
//
//+----------------------------------------------------------------

HRESULT
CCurrentStyle::get_width(VARIANT * p)
{
    HRESULT hr;
    NODE_IN_TREE_CHECK()
    hr = THR(GetUnitValueWithEnumDef(p, pNodeToUse->GetCascadedwidth(),
        &(s_propdescCCurrentStylewidth.a), styleAuto, styleAutoAuto));

Cleanup:
    RRETURN(SetErrorInfo( hr ));
}


//+----------------------------------------------------------------
//
//  member : get_height
//
//  Synopsis : IHTMLCurrentStyle property
//
//+----------------------------------------------------------------

HRESULT
CCurrentStyle::get_height(VARIANT * p)
{
    HRESULT hr;
    NODE_IN_TREE_CHECK()
    hr = THR(GetUnitValueWithEnumDef(p, pNodeToUse->GetCascadedheight(),
        &(s_propdescCCurrentStyleheight.a), styleAuto, styleAutoAuto));

Cleanup:
    RRETURN(SetErrorInfo( hr ));
}


//+----------------------------------------------------------------
//
//  member : get_paddingLeft
//
//  Synopsis : IHTMLCurrentStyle property
//
//+----------------------------------------------------------------

HRESULT
CCurrentStyle::get_paddingLeft(VARIANT * p)
{
    HRESULT hr;
    NODE_IN_TREE_CHECK()
    {
        CUnitValue uvPad = pNodeToUse->GetCascadedpaddingLeft();

        if(uvPad.IsNull())
            uvPad.SetRawValue(MAKEUNITVALUE(0, UNIT_PIXELS));

        hr = THR(GetUnitValueHelper(p, uvPad,
                        &(s_propdescCCurrentStylepaddingLeft.a)));
    }
Cleanup:
    RRETURN(SetErrorInfo( hr ));
}


//+----------------------------------------------------------------
//
//  member : get_paddingTop
//
//  Synopsis : IHTMLCurrentStyle property
//
//+----------------------------------------------------------------

HRESULT
CCurrentStyle::get_paddingTop(VARIANT * p)
{
    HRESULT hr;
    NODE_IN_TREE_CHECK()
    {
        CUnitValue uvPad = pNodeToUse->GetCascadedpaddingTop();

        if(uvPad.IsNull())
            uvPad.SetRawValue(MAKEUNITVALUE(0, UNIT_PIXELS));

        hr = THR(GetUnitValueHelper(p, uvPad,
                        &(s_propdescCCurrentStylepaddingTop.a)));
    }
Cleanup:
    RRETURN(SetErrorInfo( hr ));
}


//+----------------------------------------------------------------
//
//  member : get_paddingRight
//
//  Synopsis : IHTMLCurrentStyle property
//
//+----------------------------------------------------------------

HRESULT
CCurrentStyle::get_paddingRight(VARIANT * p)
{
    HRESULT hr;
    NODE_IN_TREE_CHECK()
    {
        CUnitValue uvPad = pNodeToUse->GetCascadedpaddingRight();

        if(uvPad.IsNull())
            uvPad.SetRawValue(MAKEUNITVALUE(0, UNIT_PIXELS));

        hr = THR(GetUnitValueHelper(p, uvPad,
                        &(s_propdescCCurrentStylepaddingRight.a)));
    }
Cleanup:
    RRETURN(SetErrorInfo( hr ));
}


//+----------------------------------------------------------------
//
//  member : get_paddingBottom
//
//  Synopsis : IHTMLCurrentStyle property
//
//+----------------------------------------------------------------

HRESULT
CCurrentStyle::get_paddingBottom(VARIANT * p)
{
    HRESULT hr;
    NODE_IN_TREE_CHECK()
    {
        CUnitValue uvPad = pNodeToUse->GetCascadedpaddingBottom();

        if(uvPad.IsNull())
            uvPad.SetRawValue(MAKEUNITVALUE(0, UNIT_PIXELS));

        hr = THR(GetUnitValueHelper(p, uvPad,
                        &(s_propdescCCurrentStylepaddingBottom.a)));
    }
Cleanup:
    RRETURN(SetErrorInfo( hr ));
}


//+----------------------------------------------------------------
//
//  member : get_textAlign
//
//  Synopsis : IHTMLCurrentStyle property
//
//+----------------------------------------------------------------

HRESULT
CCurrentStyle::get_textAlign(BSTR * p)
{
    HRESULT         hr;
    htmlBlockAlign  align;
    styleDir        dir;
    NODE_IN_TREE_CHECK()

    NULLPTRCHECK(p, E_POINTER)

    align = pNodeToUse->GetCascadedblockAlign();
    dir   = pNodeToUse->GetCascadedBlockDirection();

    // The default value is left align if LTR and right align if RTL
    if(align == htmlBlockAlignNotSet)
    {
        if(dir == styleDirLeftToRight)
           align = htmlBlockAlignLeft;
        else
           align = htmlBlockAlignRight;
    }

    hr = THR(STRINGFROMENUM(htmlBlockAlign, align, p));

Cleanup:
    RRETURN(SetErrorInfo( hr ));
}


//+----------------------------------------------------------------
//
//  member : get_textDecoration
//
//  Synopsis : IHTMLCurrentStyle property
//
//+----------------------------------------------------------------

HRESULT
CCurrentStyle::get_textDecoration(BSTR * p)
{
    HRESULT         hr;
    textDecoration  td;
    NODE_IN_TREE_CHECK()
    NULLPTRCHECK(p, E_POINTER)

    if(pNodeToUse->GetCascadedunderline())
        td = textDecorationUnderline;
    else if(pNodeToUse->GetCascadedoverline())
        td = textDecorationOverline;
    else if(pNodeToUse->GetCascadedstrikeOut())
        td = textDecorationLineThrough;
    else
        td = textDecorationNone;

    hr = THR(STRINGFROMENUM(textDecoration, td, p));

Cleanup:
    RRETURN(SetErrorInfo( hr ));
}

//+----------------------------------------------------------------
//
//  member : get_accelerator
//
//  Synopsis : IHTMLCurrentStyle property
//
//+----------------------------------------------------------------

HRESULT
CCurrentStyle::get_accelerator(BSTR * p)
{
    HRESULT         hr;
    styleAccelerator sa;
    NODE_IN_TREE_CHECK()
    NULLPTRCHECK(p, E_POINTER)

    sa = pNodeToUse->GetCascadedaccelerator();

    hr = THR(STRINGFROMENUM(styleAccelerator, sa, p));

Cleanup:
    RRETURN(SetErrorInfo( hr ));
}


//+----------------------------------------------------------------
//
//  member : get_display
//
//  Synopsis : IHTMLCurrentStyle property
//
//+----------------------------------------------------------------

HRESULT
CCurrentStyle::get_display(BSTR * p)
{
    HRESULT      hr;
    styleDisplay sd;
    NODE_IN_TREE_CHECK()
    NULLPTRCHECK(p, E_POINTER)

    sd = pNodeToUse->GetCascadeddisplay();
    if(sd == styleDisplayNotSet)
        sd = styleDisplayInline;

    hr = THR(STRINGFROMENUM(styleDisplay, sd, p));

Cleanup:
    RRETURN(SetErrorInfo( hr ));
}


//+----------------------------------------------------------------
//
//  member : get_visibility
//
//  Synopsis : IHTMLCurrentStyle property
//
//+----------------------------------------------------------------

HRESULT
CCurrentStyle::get_visibility(BSTR * p)
{
    HRESULT         hr;
    styleVisibility sv;
    NODE_IN_TREE_CHECK()
    NULLPTRCHECK(p, E_POINTER)

    sv = pNodeToUse->GetCascadedvisibility();
    if (sv == styleVisibilityNotSet)
        sv = styleVisibilityInherit;

    hr = THR(STRINGFROMENUM(styleVisibility, sv, p));

Cleanup:
    RRETURN(SetErrorInfo( hr ));
}


//+----------------------------------------------------------------
//
//  member : get_zIndex
//
//  Synopsis : IHTMLCurrentStyle property
//
//+----------------------------------------------------------------

HRESULT
CCurrentStyle::get_zIndex(VARIANT * p)
{
    HRESULT      hr = S_OK;
    long         lVal;
    NODE_IN_TREE_CHECK()
    NULLPTRCHECK(p, E_POINTER)

    lVal = pNodeToUse->GetCascadedzIndex();
    if(lVal != 0)
    {
        V_I4(p) = lVal;
        V_VT(p) = VT_I4;
    }
    else
    {
        hr = THR(FormsAllocString(_T("auto"), &(V_BSTR(p))));
        if(hr)
            goto Cleanup;
        V_VT(p) = VT_BSTR;
    }

Cleanup:
    RRETURN(SetErrorInfo( hr ));
}


//+----------------------------------------------------------------
//
//  member : get_letterSpacing
//
//  Synopsis : IHTMLCurrentStyle property
//
//+----------------------------------------------------------------

HRESULT
CCurrentStyle::get_letterSpacing(VARIANT * p)
{
    HRESULT hr;
    NODE_IN_TREE_CHECK()
    hr = THR(GetUnitValueWithEnumDef(p, pNodeToUse->GetCascadedletterSpacing(),
                    &(s_propdescCCurrentStyleletterSpacing.a), styleNormal,
                    styleNormalNormal));
Cleanup:
    RRETURN(SetErrorInfo( hr ));
}


//+----------------------------------------------------------------
//
//  member : get_lineHeight
//
//  Synopsis : IHTMLCurrentStyle property
//
//+----------------------------------------------------------------

HRESULT
CCurrentStyle::get_lineHeight(VARIANT * p)
{
    HRESULT hr;
    NODE_IN_TREE_CHECK()
    {
        CUnitValue valHeight = pNodeToUse->GetCascadedlineHeight();
        hr = THR(GetUnitValueWithEnumDef(p, valHeight,
                    &(s_propdescCCurrentStylelineHeight.a), styleNormal,
                    styleNormalNormal));
    }
Cleanup:
    RRETURN(SetErrorInfo( hr ));
}


//+----------------------------------------------------------------
//
//  member : get_textIndent
//
//  Synopsis : IHTMLCurrentStyle property
//
//+----------------------------------------------------------------

HRESULT
CCurrentStyle::get_textIndent(VARIANT * p)
{
    HRESULT hr;
    NODE_IN_TREE_CHECK()
    {
        CUnitValue uvIndent = pNodeToUse->GetCascadedtextIndent();

        if(uvIndent.IsNull())
            uvIndent.SetRawValue(MAKEUNITVALUE(0, UNIT_POINT));

        hr = THR(GetUnitValueHelper(p, uvIndent,
                        &(s_propdescCCurrentStyletextIndent.a)));
    }
Cleanup:
    RRETURN(SetErrorInfo( hr ));
}

//+----------------------------------------------------------------
//
//  member : get_verticalAlign
//
//  Synopsis : IHTMLCurrentStyle property
//
//+----------------------------------------------------------------

HRESULT
CCurrentStyle::get_verticalAlign(VARIANT * p)
{
    HRESULT                     hr;
    styleVerticalAlign          eVA;
    NODE_IN_TREE_CHECK()
    NULLPTRCHECK(p, E_POINTER)

    V_VT(p) = VT_EMPTY;

    if(pNodeToUse->GetCascadedsubscript())
        eVA = styleVerticalAlignSub;
    else if(pNodeToUse->GetCascadedsuperscript())
        eVA = styleVerticalAlignSuper;
    else
        eVA = styleVerticalAlignNotSet;

    // Convert to string
    hr = THR(STRINGFROMENUM(styleVerticalAlign, eVA, &(V_BSTR(p))));
    if(hr)
        goto Cleanup;

    V_VT(p) = VT_BSTR;

Cleanup:
    RRETURN(SetErrorInfo( hr ));
}


//+----------------------------------------------------------------
//
//  member : get_backgroundAttachment
//
//  Synopsis : IHTMLCurrentStyle property
//
//+----------------------------------------------------------------

HRESULT
CCurrentStyle::get_backgroundAttachment(BSTR * p)
{
    HRESULT                     hr;
    styleBackgroundAttachment   eBA;
    NODE_IN_TREE_CHECK()
    NULLPTRCHECK(p, E_POINTER)

    eBA = (pNodeToUse->GetCascadedbackgroundAttachmentFixed()) ?
            styleBackgroundAttachmentFixed : styleBackgroundAttachmentScroll;

    // Convert to string
    hr = THR(STRINGFROMENUM(styleBackgroundAttachment, eBA, p));

Cleanup:
    RRETURN(SetErrorInfo( hr ));
}


//+----------------------------------------------------------------
//
 //  member : get_marginTop
//
//  Synopsis : IHTMLCurrentStyle property
//
//+----------------------------------------------------------------

HRESULT
CCurrentStyle::get_marginTop(VARIANT * p)
{
    HRESULT hr;
    NODE_IN_TREE_CHECK()
    {
        CUnitValue uvMargin = pNodeToUse->GetCascadedmarginTop();

        hr = THR(GetUnitValueWithEnumDef(p, uvMargin,
            &(s_propdescCCurrentStylemarginTop.a), styleAuto, styleAutoAuto));
    }
Cleanup:
    RRETURN(SetErrorInfo( hr ));
}


//+----------------------------------------------------------------
//
//  member : get_marginRight
//
//  Synopsis : IHTMLCurrentStyle property
//
//+----------------------------------------------------------------

HRESULT
CCurrentStyle::get_marginRight(VARIANT * p)
{
    HRESULT hr;
    NODE_IN_TREE_CHECK()
    {
        CUnitValue uvMargin = pNodeToUse->GetCascadedmarginRight();

        hr = THR(GetUnitValueWithEnumDef(p, uvMargin,
            &(s_propdescCCurrentStylemarginRight.a), styleAuto, styleAutoAuto));
    }

Cleanup:
    RRETURN(SetErrorInfo( hr ));
}


//+----------------------------------------------------------------
//
//  member : get_marginBottom
//
//  Synopsis : IHTMLCurrentStyle property
//
//+----------------------------------------------------------------

HRESULT
CCurrentStyle::get_marginBottom(VARIANT * p)
{
    HRESULT hr;
    NODE_IN_TREE_CHECK()
    {
        CUnitValue uvMargin = pNodeToUse->GetCascadedmarginBottom();

        hr = THR(GetUnitValueWithEnumDef(p, uvMargin,
            &(s_propdescCCurrentStylemarginBottom.a), styleAuto, styleAutoAuto));
    }

Cleanup:
    RRETURN(SetErrorInfo( hr ));
}


//+----------------------------------------------------------------
//
//  member : get_marginLeft
//
//  Synopsis : IHTMLCurrentStyle property
//
//+----------------------------------------------------------------

HRESULT
CCurrentStyle::get_marginLeft(VARIANT * p)
{
    HRESULT hr;
    NODE_IN_TREE_CHECK()
    {
        CUnitValue uvMargin = pNodeToUse->GetCascadedmarginLeft();

        hr = THR(GetUnitValueWithEnumDef(p, uvMargin,
            &(s_propdescCCurrentStylemarginLeft.a), styleAuto, styleAutoAuto));
    }

Cleanup:
    RRETURN(SetErrorInfo( hr ));
}


//+----------------------------------------------------------------
//
//  member : get_clear
//
//  Synopsis : IHTMLCurrentStyle property
//
//+----------------------------------------------------------------

HRESULT
CCurrentStyle::get_clear(BSTR * p)
{
    HRESULT       hr;
    BOOL          fClearLeft, fClearRight;
    htmlClear     eClr;
    NODE_IN_TREE_CHECK()
    NULLPTRCHECK(p, E_POINTER)

    fClearLeft = pNodeToUse->GetCascadedclearLeft();
    fClearRight = pNodeToUse->GetCascadedclearRight();

    if(fClearLeft)
        if(fClearRight)
            eClr = htmlClearBoth;
        else
            eClr = htmlClearLeft;
    else
        if(fClearRight)
            eClr = htmlClearRight;
        else
            eClr = htmlClearNone;

    // Convert to string
    hr = THR(STRINGFROMENUM(htmlClear, eClr, p));

Cleanup:
    RRETURN(SetErrorInfo( hr ));
}


//+----------------------------------------------------------------
//
//  member : get_listStyleType
//
//  Synopsis : IHTMLCurrentStyle property
//
//+----------------------------------------------------------------

HRESULT
CCurrentStyle::get_listStyleType(BSTR * pbstr)
{
    HRESULT             hr;
    styleListStyleType  slt;
    NODE_IN_TREE_CHECK()
    NULLPTRCHECK(pbstr, E_POINTER)


    slt = pNodeToUse->GetCascadedlistStyleType();

    if (slt == styleListStyleTypeNotSet)
       slt = styleListStyleTypeDisc;

    hr = THR(STRINGFROMENUM(styleListStyleType, slt, pbstr));

Cleanup:
    RRETURN(SetErrorInfo( hr ));
}


//+----------------------------------------------------------------
//
//  member : get_listStylePosition
//
//  Synopsis : IHTMLCurrentStyle property
//
//+----------------------------------------------------------------

HRESULT
CCurrentStyle::get_listStylePosition(BSTR * pbstr)
{
    HRESULT hr;
    styleListStylePosition slsp;

    NODE_IN_TREE_CHECK()
    NULLPTRCHECK(pbstr, E_POINTER)


    slsp = pNodeToUse->GetCascadedlistStylePosition();
    if(slsp == styleListStylePositionNotSet)
        slsp = styleListStylePositionOutSide;

    hr = THR(STRINGFROMENUM(styleListStylePosition, slsp, pbstr));

Cleanup:
    RRETURN(SetErrorInfo( hr ));
}


//+----------------------------------------------------------------
//
//  member : get_listStyleImage
//
//  Synopsis : IHTMLCurrentStyle property
//
//+----------------------------------------------------------------

HRESULT
CCurrentStyle::get_listStyleImage(BSTR * p)
{
    HRESULT      hr;
    long         lCookie;
    NODE_IN_TREE_CHECK()
    NULLPTRCHECK(p, E_POINTER)

    lCookie = pNodeToUse->GetCascadedlistImageCookie();

    if(lCookie == 0)
    {
        hr = THR(FormsAllocString(_T("none"), p));
    }
    else
    {
        hr = THR(GetImageNameFromCookie(lCookie, p));
    }

Cleanup:
    RRETURN(SetErrorInfo( hr ));
}


//+----------------------------------------------------------------
//
//  member : get_clipTop
//
//  Synopsis : IHTMLCurrentStyle property
//
//+----------------------------------------------------------------

HRESULT
CCurrentStyle::get_clipTop(VARIANT * p)
{
    HRESULT hr;
    NODE_IN_TREE_CHECK()
    hr = THR(GetUnitValueWithEnum(p, pNodeToUse->GetCascadedclipTop(),
                    &(s_propdescCCurrentStyleclipTop.a), styleAuto));
Cleanup:
    RRETURN(SetErrorInfo( hr ));
}


//+----------------------------------------------------------------
//
//  member : get_clipRight
//
//  Synopsis : IHTMLCurrentStyle property
//
//+----------------------------------------------------------------

HRESULT
CCurrentStyle::get_clipRight(VARIANT * p)
{
    HRESULT hr;
    NODE_IN_TREE_CHECK()
    hr = THR(GetUnitValueWithEnum(p, pNodeToUse->GetCascadedclipRight(),
                    &(s_propdescCCurrentStyleclipRight.a), styleAuto));
Cleanup:
    RRETURN(SetErrorInfo( hr ));
}


//+----------------------------------------------------------------
//
//  member : get_clipBottom
//
//  Synopsis : IHTMLCurrentStyle property
//
//+----------------------------------------------------------------

HRESULT
CCurrentStyle::get_clipBottom(VARIANT * p)
{
    HRESULT hr;
    NODE_IN_TREE_CHECK()
    hr = THR(GetUnitValueWithEnum(p, pNodeToUse->GetCascadedclipBottom(),
                    &(s_propdescCCurrentStyleclipBottom.a), styleAuto));
Cleanup:
    RRETURN(SetErrorInfo( hr ));
}


//+----------------------------------------------------------------
//
//  member : get_clipLeft
//
//  Synopsis : IHTMLCurrentStyle property
//
//+----------------------------------------------------------------

HRESULT
CCurrentStyle::get_clipLeft(VARIANT * p)
{
    HRESULT hr;
    NODE_IN_TREE_CHECK()
    hr = (GetUnitValueWithEnum(p, pNodeToUse->GetCascadedclipLeft(),
                    &(s_propdescCCurrentStyleclipLeft.a), styleAuto));
Cleanup:
    RRETURN(SetErrorInfo( hr ));
}


//+----------------------------------------------------------------
//
//  member : get_clipLeft
//
//  Synopsis : IHTMLCurrentStyle property
//
//+----------------------------------------------------------------

HRESULT
CCurrentStyle::get_overflow(BSTR * pbstr)
{
    HRESULT         hr;
    styleOverflow   stO;
    NODE_IN_TREE_CHECK()
    NULLPTRCHECK(pbstr, E_POINTER)


    stO = pNodeToUse->GetCascadedoverflow();

    if(stO == styleOverflowNotSet)
        stO = styleOverflowVisible;

    hr = THR(STRINGFROMENUM(styleOverflow, stO, pbstr));

Cleanup:
    RRETURN(SetErrorInfo( hr ));
}


//+----------------------------------------------------------------
//
//  member : get_pageBreakBefore
//
//  Synopsis : IHTMLCurrentStyle property
//
//+----------------------------------------------------------------

HRESULT
CCurrentStyle::get_pageBreakBefore(BSTR * pbstr)
{
    HRESULT         hr;
    stylePageBreak  stpb;
    NODE_IN_TREE_CHECK()
    NULLPTRCHECK(pbstr, E_POINTER)


    stpb = pNodeToUse->GetCascadedpageBreakBefore();

    // When not set the default value is auto
    if(stpb == stylePageBreakNotSet)
        stpb = stylePageBreakAuto;

    hr = THR(STRINGFROMENUM(stylePageBreak, stpb, pbstr));

Cleanup:
    RRETURN(SetErrorInfo( hr ));
}


//+----------------------------------------------------------------
//
//  member : get_pageBreakAfter
//
//  Synopsis : IHTMLCurrentStyle property
//
//+----------------------------------------------------------------

HRESULT
CCurrentStyle::get_pageBreakAfter(BSTR * pbstr)
{
    HRESULT hr;
    stylePageBreak  stpb;

    NODE_IN_TREE_CHECK()
    NULLPTRCHECK(pbstr, E_POINTER)


    stpb = pNodeToUse->GetCascadedpageBreakAfter();

    // When not set the default value is auto
    if(stpb == stylePageBreakNotSet)
        stpb = stylePageBreakAuto;

    hr = THR(STRINGFROMENUM(stylePageBreak, stpb, pbstr));

Cleanup:
    RRETURN(SetErrorInfo( hr ));
}


//+----------------------------------------------------------------
//
//  member : get_cursor
//
//  Synopsis : IHTMLCurrentStyle property
//
//+----------------------------------------------------------------

HRESULT
CCurrentStyle::get_cursor(BSTR * pbstr)
{
    HRESULT hr;
    NODE_IN_TREE_CHECK()
    NULLPTRCHECK(pbstr, E_POINTER)


    hr = THR(STRINGFROMENUM(styleCursor, pNodeToUse->GetCascadedcursor(), pbstr));

Cleanup:
    RRETURN(SetErrorInfo( hr ));
}


//+----------------------------------------------------------------
//
//  member : get_tableLayout
//
//  Synopsis : IHTMLCurrentStyle property
//
//+----------------------------------------------------------------

HRESULT
CCurrentStyle::get_tableLayout(BSTR * pbstr)
{
    HRESULT hr;
    NODE_IN_TREE_CHECK()
    NULLPTRCHECK(pbstr, E_POINTER)


    hr = THR(STRINGFROMENUM(styleTableLayout, pNodeToUse->GetCascadedtableLayoutEnum(), pbstr));

Cleanup:
    RRETURN(SetErrorInfo( hr ));
}


//+----------------------------------------------------------------
//
//  member : get_borderCollapse
//
//  Synopsis : IHTMLCurrentStyle property
//
//+----------------------------------------------------------------

HRESULT
CCurrentStyle::get_borderCollapse (BSTR * pbstr)
{
    HRESULT hr;

    NODE_IN_TREE_CHECK()
    NULLPTRCHECK(pbstr, E_POINTER)

    hr = THR(STRINGFROMENUM(styleBorderCollapse, pNodeToUse->GetCascadedborderCollapseEnum(), pbstr));

Cleanup:
    RRETURN(SetErrorInfo( hr ));
}


//+----------------------------------------------------------------
//
//  member : get_blockDirection
//
//  Synopsis : IHTMLCurrentStyle property
//
//+----------------------------------------------------------------

HRESULT
CCurrentStyle::get_blockDirection(BSTR * pbstr)
{
    HRESULT hr;
    
    NODE_IN_TREE_CHECK()
    NULLPTRCHECK(pbstr, E_POINTER)

    
    hr = THR(STRINGFROMENUM(styleDir, pNodeToUse->GetCascadedBlockDirection(), pbstr));

Cleanup:
    RRETURN(SetErrorInfo( hr ));
}

//+----------------------------------------------------------------
//
//  member : get_direction
//
//  Synopsis : IHTMLCurrentStyle property
//
//+----------------------------------------------------------------

HRESULT
CCurrentStyle::get_direction(BSTR * pbstr)
{
    HRESULT hr;

    NODE_IN_TREE_CHECK()
    NULLPTRCHECK(pbstr, E_POINTER)


    hr = THR(STRINGFROMENUM(styleDir, pNodeToUse->GetCascadeddirection(), pbstr));

Cleanup:
    RRETURN(SetErrorInfo( hr ));
}

//+----------------------------------------------------------------
//
//  member : get_unicodeBidi
//
//  Synopsis : IHTMLCurrentStyle property
//
//+----------------------------------------------------------------

HRESULT
CCurrentStyle::get_unicodeBidi(BSTR * pbstr)
{
    HRESULT hr;

    NODE_IN_TREE_CHECK()
    NULLPTRCHECK(pbstr, E_POINTER)

    hr = THR(STRINGFROMENUM(styleBidi,pNodeToUse->GetCascadedunicodeBidi(), pbstr));

Cleanup:
    RRETURN(SetErrorInfo( hr ));
}


//+----------------------------------------------------------------
//
//  member : get_imeMode
//
//  Synopsis : IHTMLCurrentStyle property
//
//+----------------------------------------------------------------

HRESULT
CCurrentStyle::get_imeMode(BSTR * pbstr)
{
    HRESULT         hr;
    styleImeMode    sty;

    NULLPTRCHECK(pbstr, E_POINTER)

    hr = THR(ComputeLongExtraFormat(DISPID_A_IMEMODE, TRUE, (long)styleImeModeNotSet, (long *)&sty));
    if(hr)
        goto Cleanup;
    hr = THR(STRINGFROMENUM(styleImeMode, sty, pbstr));

Cleanup:
    RRETURN(SetErrorInfo( hr ));

}


//+----------------------------------------------------------------
//
//  member : get_rubyAlign
//
//  Synopsis : IHTMLCurrentStyle property
//
//+----------------------------------------------------------------

HRESULT
CCurrentStyle::get_rubyAlign(BSTR * pbstr)
{
    HRESULT         hr;
    styleRubyAlign  sty;

    NULLPTRCHECK(pbstr, E_POINTER)

    hr = THR(ComputeLongExtraFormat(DISPID_A_RUBYALIGN, TRUE, (long)styleRubyAlignAuto, (long *)&sty));
    if(hr)
        goto Cleanup;
    hr = THR(STRINGFROMENUM(styleRubyAlign, sty, pbstr));

Cleanup:
    RRETURN(SetErrorInfo( hr ));

}


//+----------------------------------------------------------------
//
//  member : get_rubyPosition
//
//  Synopsis : IHTMLCurrentStyle property
//
//+----------------------------------------------------------------

HRESULT
CCurrentStyle::get_rubyPosition(BSTR * pbstr)
{
    HRESULT         hr;
    styleRubyPosition  sty;

    NULLPTRCHECK(pbstr, E_POINTER)

    hr = THR(ComputeLongExtraFormat(DISPID_A_RUBYPOSITION, TRUE, (long)styleRubyPositionAbove, (long *)&sty));
    if(hr)
        goto Cleanup;
    hr = THR(STRINGFROMENUM(styleRubyPosition, sty, pbstr));

Cleanup:
    RRETURN(SetErrorInfo( hr ));
}


//+----------------------------------------------------------------
//
//  member : get_rubyOverhang
//
//  Synopsis : IHTMLCurrentStyle property
//
//+----------------------------------------------------------------

HRESULT
CCurrentStyle::get_rubyOverhang(BSTR * pbstr)
{
    HRESULT            hr;
    styleRubyOverhang  sty;

    NULLPTRCHECK(pbstr, E_POINTER)

    hr = THR(ComputeLongExtraFormat(DISPID_A_RUBYOVERHANG, TRUE, (long)styleRubyOverhangAuto, (long *)&sty));
    if(hr)
        goto Cleanup;
    hr = THR(STRINGFROMENUM(styleRubyOverhang, sty, pbstr));

Cleanup:
    RRETURN(SetErrorInfo( hr ));
}


//+----------------------------------------------------------------
//
//  member : get_textAutospace
//
//  Synopsis : IHTMLCurrentStyle property
//
//+----------------------------------------------------------------

HRESULT
CCurrentStyle::get_textAutospace(BSTR * pbstr)
{
    HRESULT hr = S_OK;
    NODE_IN_TREE_CHECK()

    NULLPTRCHECK(pbstr, E_POINTER)
    WriteTextAutospaceFromLongToBSTR(pNodeToUse->GetCascadedtextAutospace(), pbstr, TRUE);

Cleanup:
    RRETURN(SetErrorInfo( hr ));
}


//+----------------------------------------------------------------
//
//  member : get_wordBreak
//
//  Synopsis : IHTMLCurrentStyle property
//
//+----------------------------------------------------------------

HRESULT
CCurrentStyle::get_wordBreak(BSTR * pbstr)
{
    HRESULT hr;

    NODE_IN_TREE_CHECK()
    NULLPTRCHECK(pbstr, E_POINTER)

    hr = THR(STRINGFROMENUM(styleWordBreak, pNodeToUse->GetCascadedwordBreak(), pbstr));

Cleanup:
    RRETURN(SetErrorInfo( hr ));
}



//+----------------------------------------------------------------
//
//  member : get_lineBreak
//
//  Synopsis : IHTMLCurrentStyle property
//
//+----------------------------------------------------------------

HRESULT
CCurrentStyle::get_lineBreak(BSTR * pbstr)
{
    HRESULT            hr;
    styleLineBreak     sty;
    NODE_IN_TREE_CHECK()

    NULLPTRCHECK(pbstr, E_POINTER)

    sty = pNodeToUse->GetCascadedlineBreak();
    hr = THR(STRINGFROMENUM(styleLineBreak, sty, pbstr));

Cleanup:
    RRETURN(SetErrorInfo( hr ));
}


//+----------------------------------------------------------------
//
//  member : get_textJustify
//
//  Synopsis : IHTMLCurrentStyle property
//
//+----------------------------------------------------------------

HRESULT
CCurrentStyle::get_textJustify(BSTR * pbstr)
{
    HRESULT          hr;
    styleTextJustify just;
    htmlBlockAlign   align;

    NODE_IN_TREE_CHECK()
    NULLPTRCHECK(pbstr, E_POINTER);

    align = pNodeToUse->GetCascadedblockAlign();

    if (align == htmlBlockAlignJustify)
    {
        just = pNodeToUse->GetCascadedtextJustify();
    }
    else
    {
        just = styleTextJustifyAuto;
    }
            
    hr = THR(STRINGFROMENUM(styleTextJustify, just, pbstr));

Cleanup:
    RRETURN(SetErrorInfo( hr ));
}



//+----------------------------------------------------------------
//
//  member : get_textJustifyTrim
//
//  Synopsis : IHTMLCurrentStyle property
//
//+----------------------------------------------------------------

HRESULT
CCurrentStyle::get_textJustifyTrim(BSTR * pbstr)
{
    HRESULT              hr;
    styleTextJustifyTrim trim;
    htmlBlockAlign       align;

    NODE_IN_TREE_CHECK()
    NULLPTRCHECK(pbstr, E_POINTER)

    align = pNodeToUse->GetCascadedblockAlign();

    if (align == htmlBlockAlignJustify)
    {
        trim = pNodeToUse->GetCascadedtextJustifyTrim();
    }
    else
    {
        trim = styleTextJustifyTrimNotSet;
    }
    
    hr = THR(STRINGFROMENUM(styleTextJustifyTrim, trim, pbstr));

Cleanup:
    RRETURN(SetErrorInfo( hr ));
}



//+----------------------------------------------------------------
//
//  member : get_textKashida
//
//  Synopsis : IHTMLCurrentStyle property
//
//+----------------------------------------------------------------

HRESULT
CCurrentStyle::get_textKashida(VARIANT * p)
{
    HRESULT hr;
    
    NULLPTRCHECK(p, E_POINTER);

    hr = S_OK;
    
    VariantInit(p);
    
Cleanup:
    RRETURN(SetErrorInfo(hr));
}



//+----------------------------------------------------------------
//
//  member   : get_borderColor
//
//  Synopsis : IHTMLCurrentStyle property
//
//+----------------------------------------------------------------

HRESULT 
CCurrentStyle::get_borderColor(BSTR * p)
{
    HRESULT             hr;
    CVariant            varTop, varRight, varBottom, varLeft;

    NULLPTRCHECK(p, E_POINTER)
    *p = NULL;

    hr = THR(get_borderTopColor(&varTop));
    if(hr)
        goto Cleanup;

    hr = THR(get_borderRightColor(&varRight));
    if(hr)
        goto Cleanup;

    hr = THR(get_borderBottomColor(&varBottom));
    if(hr)
        goto Cleanup;

    hr = THR(get_borderLeftColor(&varLeft));
    if(hr)
        goto Cleanup;

    hr = THR(GetCompositBSTR(&varTop, &varRight, &varBottom, &varLeft, p));

Cleanup:
    RRETURN(SetErrorInfo(hr));
}


//+----------------------------------------------------------------
//
//  member   : get_borderWidth
//
//  Synopsis : IHTMLCurrentStyle property
//
//+----------------------------------------------------------------

HRESULT 
CCurrentStyle::get_borderWidth(BSTR * p)
{
    HRESULT             hr;
    CVariant            varTop, varRight, varBottom, varLeft;

    NULLPTRCHECK(p, E_POINTER)
    *p = NULL;

    hr = THR(get_borderTopWidth(&varTop));
    if(hr)
        goto Cleanup;

    hr = THR(get_borderRightWidth(&varRight));
    if(hr)
        goto Cleanup;

    hr = THR(get_borderBottomWidth(&varBottom));
    if(hr)
        goto Cleanup;

    hr = THR(get_borderLeftWidth(&varLeft));
    if(hr)
        goto Cleanup;

    hr = THR(GetCompositBSTR(&varTop, &varRight, &varBottom, &varLeft, p));

Cleanup:
    RRETURN(SetErrorInfo(hr));
}


//+----------------------------------------------------------------
//
//  member : get_padding
//
//  Synopsis : IHTMLCurrentStyle property
//
//+----------------------------------------------------------------

HRESULT 
CCurrentStyle::get_padding(BSTR * p)
{
    HRESULT             hr;
    CVariant            varTop, varRight, varBottom, varLeft;

    NULLPTRCHECK(p, E_POINTER)
    *p = NULL;

    hr = THR(get_paddingTop(&varTop));
    if(hr)
        goto Cleanup;

    hr = THR(get_paddingRight(&varRight));
    if(hr)
        goto Cleanup;

    hr = THR(get_paddingBottom(&varBottom));
    if(hr)
        goto Cleanup;

    hr = THR(get_paddingLeft(&varLeft));
    if(hr)
        goto Cleanup;

    hr = THR(GetCompositBSTR(&varTop, &varRight, &varBottom, &varLeft, p));

Cleanup:
    RRETURN(SetErrorInfo(hr));
}


//+----------------------------------------------------------------
//
//  member : get_padding
//
//  Synopsis : IHTMLCurrentStyle property
//
//+----------------------------------------------------------------

HRESULT 
CCurrentStyle::get_margin(BSTR * p)
{
    HRESULT             hr;
    CVariant            varTop, varRight, varBottom, varLeft;

    NULLPTRCHECK(p, E_POINTER)
    *p = NULL;

    hr = THR(get_marginTop(&varTop));
    if(hr)
        goto Cleanup;

    hr = THR(get_marginRight(&varRight));
    if(hr)
        goto Cleanup;

    hr = THR(get_marginBottom(&varBottom));
    if(hr)
        goto Cleanup;

    hr = THR(get_marginLeft(&varLeft));
    if(hr)
        goto Cleanup;

    hr = THR(GetCompositBSTR(&varTop, &varRight, &varBottom, &varLeft, p));

Cleanup:
    RRETURN(SetErrorInfo(hr));
}


//+----------------------------------------------------------------
//
//  member   : get_borderStyle
//
//  Synopsis : IHTMLCurrentStyle property
//
//+----------------------------------------------------------------

HRESULT 
CCurrentStyle::get_borderStyle(BSTR * p)
{
    HRESULT             hr;
    styleBorderStyle    bdrStyle;
    LPCTSTR             strTop, strRight, strBottom, strLeft;

    NODE_IN_TREE_CHECK()

    NULLPTRCHECK(p, E_POINTER)
    *p = NULL;
    
    bdrStyle = GetBorderStyle(BORDER_TOP);
    strTop = STRINGPTRFROMENUM(styleBorderStyle, bdrStyle);
    if(!strTop)
        goto Error;

    bdrStyle = GetBorderStyle(BORDER_RIGHT);
    strRight = STRINGPTRFROMENUM(styleBorderStyle, bdrStyle);
    if(!strRight)
        goto Error;

    bdrStyle = GetBorderStyle(BORDER_BOTTOM);
    strBottom = STRINGPTRFROMENUM(styleBorderStyle, bdrStyle);
    if(!strBottom)
        goto Error;

    bdrStyle = GetBorderStyle(BORDER_LEFT);
    strLeft = STRINGPTRFROMENUM(styleBorderStyle, bdrStyle);
    if(!strLeft)
        goto Error;

    hr = THR(GetCompositBSTR(strTop, strRight, strBottom, strLeft, p));

Cleanup:
    RRETURN(SetErrorInfo(hr));

Error:
    hr = E_INVALIDARG;
    goto Cleanup;
}


//+----------------------------------------------------------------
//
//  member : get_overflowX
//
//  Synopsis : IHTMLCurrentStyle property
//
//+----------------------------------------------------------------

HRESULT
CCurrentStyle::get_overflowX(BSTR * pbstr)
{
    HRESULT            hr;
    styleOverflow      sty;
    NODE_IN_TREE_CHECK()

    NULLPTRCHECK(pbstr, E_POINTER)

    sty = pNodeToUse->GetCascadedoverflowX();
    if(sty == styleOverflowNotSet)
        sty = styleOverflowVisible;
    hr = THR(STRINGFROMENUM(styleOverflow, sty, pbstr));

Cleanup:
    RRETURN(SetErrorInfo( hr ));
}



//+----------------------------------------------------------------
//
//  member : get_overflowY
//
//  Synopsis : IHTMLCurrentStyle property
//
//+----------------------------------------------------------------

HRESULT
CCurrentStyle::get_overflowY(BSTR * pbstr)
{
    HRESULT            hr;
    styleOverflow      sty;
    NODE_IN_TREE_CHECK()

    NULLPTRCHECK(pbstr, E_POINTER)

    sty = pNodeToUse->GetCascadedoverflowY();
    if(sty == styleOverflowNotSet)
        sty = styleOverflowVisible;
    hr = THR(STRINGFROMENUM(styleOverflow, sty, pbstr));

Cleanup:
    RRETURN(SetErrorInfo( hr ));
}



//+----------------------------------------------------------------
//
//  member : get_textTransform
//
//  Synopsis : IHTMLCurrentStyle property
//
//+----------------------------------------------------------------

HRESULT
CCurrentStyle::get_textTransform(BSTR * pbstr)
{
    HRESULT            hr;
    styleTextTransform sty;
    NODE_IN_TREE_CHECK()

    NULLPTRCHECK(pbstr, E_POINTER)

    sty = pNodeToUse->GetCascadedtextTransform();
    if(sty == styleTextTransformNotSet)
        sty = styleTextTransformNone;
    hr = THR(STRINGFROMENUM(styleTextTransform, sty, pbstr));

Cleanup:
    RRETURN(SetErrorInfo( hr ));
}


//+----------------------------------------------------------------
//
//  member : GetCompositBSTR
//
//  Synopsis : Returns the string that represents the value of a 
//               border property
//              Here are the rules:
//
//            If there is only one value, it applies to all sides. 
//            If there are two values, the top and bottom are set to the 
//              first value and the right and left are set to the second. 
//            If there are three values, the top is set to the first value, 
//              the left and right are set to the second, and the bottom is 
//              set to the third. 
//            If there are four values, they apply to the top, right, bottom,
//               and left, respectively. 
//+----------------------------------------------------------------

HRESULT
CCurrentStyle::GetCompositBSTR(LPCTSTR szTop, LPCTSTR szRight, 
                               LPCTSTR szBottom, LPCTSTR szLeft, BSTR *pbstrRet)
{
    CBufferedStr    strRet; 
    BOOL            fWriteRightLeft = FALSE, fWriteBottom = FALSE, fWriteLeft = FALSE;

    Assert(pbstrRet);
    Assert(szTop && szRight && szBottom && szLeft);

    *pbstrRet = NULL;

    strRet.QuickAppend(szTop);    // We always have the top string
    if ( _tcsicmp( szRight, szLeft ) )
    {   // Right and left don't match - write out everything.
        fWriteRightLeft = TRUE;
        fWriteBottom = TRUE;
        fWriteLeft = TRUE;
    }
    else
    {
        if ( _tcsicmp( szTop, szBottom ) )
        {
            fWriteBottom = TRUE;     // Top and bottom don't match
            fWriteRightLeft = TRUE;
        }
        else if ( _tcsicmp( szTop, szRight ) )
            fWriteRightLeft = TRUE;
    }

    if ( fWriteRightLeft )
    {
        strRet.QuickAppend(_T(" "));
        strRet.QuickAppend(szRight);    // Write out the right string (may be left also)
    }
    if ( fWriteBottom )
    {
        strRet.QuickAppend(_T(" "));
        strRet.QuickAppend(szBottom);    // Write out the bottom string
    }
    if ( fWriteLeft )
    {
        strRet.QuickAppend(_T(" "));
        strRet.QuickAppend(szLeft);    // Write out the left string
    }

    RRETURN(FormsAllocString(strRet, pbstrRet));
}


//+----------------------------------------------------------------
//
//  member : GetCompositBSTR
//
//  Synopsis : Returns the string that represents the value of a 
//               border property.
//
//              Here are the rules:
//
//            If there is only one value, it applies to all sides. 
//            If there are two values, the top and bottom are set to the 
//              first value and the right and left are set to the second. 
//            If there are three values, the top is set to the first value, 
//              the left and right are set to the second, and the bottom is 
//              set to the third. 
//            If there are four values, they apply to the top, right, bottom,
//               and left, respectively. 
//+----------------------------------------------------------------

HRESULT
CCurrentStyle::GetCompositBSTR(CVariant *pvarTop, CVariant *pvarRight, 
                               CVariant *pvarBottom, CVariant *pvarLeft, BSTR *bstrStr)
{
    HRESULT     hr;

    hr = THR(pvarTop->CoerceVariantArg(VT_BSTR));
    if(hr)
        goto Cleanup;

    hr = THR(pvarRight->CoerceVariantArg(VT_BSTR));
    if(hr)
        goto Cleanup;

    hr = THR(pvarBottom->CoerceVariantArg(VT_BSTR));
    if(hr)
        goto Cleanup;

    hr = THR(pvarLeft->CoerceVariantArg(VT_BSTR));
    if(hr)
        goto Cleanup;
    
    hr = THR(GetCompositBSTR(V_BSTR(pvarTop), V_BSTR(pvarRight), 
        V_BSTR(pvarBottom), V_BSTR(pvarLeft), bstrStr));

Cleanup:
    RRETURN(SetErrorInfo(hr));
}


//+----------------------------------------------------------------
//
//  member : ComputeLongExtraFormat
//
//  Synopsis : Looks up the variant value of a format attribute
//             and returns it as a long
//
//+----------------------------------------------------------------

HRESULT
CCurrentStyle::ComputeLongExtraFormat(DISPID dispID,   // IN
                                      BOOL fInherits,  // IN
                                      long defaultVal, // IN
                                      long *plReturn)  // OUT
{
    HRESULT         hr;
    VARIANT         varValue;
    CElement *      pElement;
    NODE_IN_TREE_CHECK()
    pElement = pNodeToUse->SafeElement();

    NULLPTRCHECK(pElement, E_FAIL)

    hr = THR(pElement->ComputeExtraFormat(dispID, fInherits, pNodeToUse, &varValue));
    if(hr)
        goto Cleanup;

    *plReturn = (((CVariant *)&varValue)->IsEmpty())
                                             ? defaultVal
                                             : V_I4(&varValue);

Cleanup:
    RRETURN(hr);
}


//+----------------------------------------------------------------
//
//  member : get_behavior
//
//  Synopsis : IHTMLCurrentStyle property
//
//+----------------------------------------------------------------

HRESULT
CCurrentStyle::get_behavior(BSTR * pbstr)
{
    HRESULT hr = S_OK;
    NODE_IN_TREE_CHECK()
    NULLPTRCHECK(pbstr, E_POINTER);


    *pbstr = NULL;
/*
    if(_pNode->Element())
    {
        // (alexz) this is wrong because there are multiple behaviors on the element
        // also, the logic itself should be in peer.cxx, not here
        CPeerHolder * pPeerPtr = _pNode->Element()->GetPeerHolderPtr();
        if(pPeerPtr)
        {
            CPeerFactoryUrl * pFactoryURL = pPeerPtr->_pPeerFactoryUrl;
            if(pFactoryURL)
            {
                FormsAllocString(pFactoryURL->_cstrUrl, pbstr);
            }

        }
    }
*/
Cleanup:
    RRETURN(SetErrorInfo( hr ));
}


//*********************************************************************
// CCurrentStyle::GetDispID, IDispatch
//    Is used to do sepecial processing for the expandos. CurrentStyle
//      expandos are not stored in object's attr array
//
//*********************************************************************

STDMETHODIMP
CCurrentStyle::GetDispID(BSTR bstrName, DWORD grfdex, DISPID *pid)
{
    HRESULT         hr;

    // We do not allow adding expandos on currentStyle
    grfdex = grfdex & (~fdexNameEnsure);

    // Now let CBase to search for the dispid
    hr = THR_NOTRACE(super::GetDispID(bstrName, grfdex, pid));

    if(hr == DISP_E_UNKNOWNNAME)
    {
        CAttrArray          * pAA;

        // Not found, search cached propagated expandos of the node
        pAA = GetCachedExpandoAA();
        if(pAA)
        {
            hr = GetExpandoDispID(bstrName, pid, grfdex, pAA);
            if (hr == S_FALSE)
            {
                Assert(*pid == DISPID_UNKNOWN);
                hr = S_OK;
            }
        }
    }

    RRETURN1(hr, DISP_E_UNKNOWNNAME);
}


//*********************************************************************
// CCurrentStyle::GetDispID, InvokeEx
//    Is used to do sepecial processing for the expandos. CurrentStyle
//      expandos are not stored in object's attr array
//
//*********************************************************************

HRESULT
CCurrentStyle::InvokeEx(DISPID dispid, LCID lcid, WORD wFlags,  DISPPARAMS *pdispparams,
               VARIANT *pvarResult, EXCEPINFO *pexcepinfo, IServiceProvider *pSrvProvider)
{
    HRESULT hr = S_OK;

    if (!pvarResult)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    // Fail if it is a property put
    if( wFlags & (DISPATCH_PROPERTYPUT | DISPATCH_PROPERTYPUTREF) )
    {
        hr = DISP_E_MEMBERNOTFOUND;
        goto Cleanup;
    }

    if (IsExpandoDISPID(dispid))
    {
        CAttrArray          * pAA;

        if(!(wFlags & DISPATCH_PROPERTYGET))
        {
            hr = DISP_E_MEMBERNOTFOUND;
            goto Cleanup;
        }

        // Not found, search cached propagated expandos of the node
        pAA = GetCachedExpandoAA();

        if(!pAA)
        {
            hr = DISP_E_MEMBERNOTFOUND;
            goto Cleanup;
        }

        const CAttrValue * pAV = pAA->Find(dispid, CAttrValue::AA_Expando);

        if(pAV != NULL)
            hr = pAV->GetIntoVariant(pvarResult);
        else
            V_VT(pvarResult) = VT_NULL;
    }
    else
    {
        hr = THR(super::InvokeEx(dispid, lcid, wFlags, pdispparams, pvarResult, pexcepinfo, pSrvProvider));
    }


Cleanup:
    RRETURN1(hr, DISP_E_MEMBERNOTFOUND);
}


//+----------------------------------------------------------------
//
//  member : GetColorHelper
//
//  Synopsis : helper function used by functions gettting from
//                  CColorValue
//
//+----------------------------------------------------------------

HRESULT
CCurrentStyle::GetColorHelper(VARIANT * p, const CColorValue &cvCol)
{
    HRESULT      hr = S_OK;
    TCHAR        szBuffer[pdlColor];

    NODE_IN_TREE_CHECK()
    NULLPTRCHECK(p, E_POINTER)

    V_VT(p) = VT_BSTR;
    V_BSTR(p) = NULL;

    if(cvCol.IsDefined())
    {
        hr = THR(cvCol.FormatBuffer(szBuffer, ARRAY_SIZE(szBuffer), NULL));
        if(hr)
            goto Cleanup;
        hr = THR(FormsAllocString(szBuffer, &(V_BSTR(p))));
    }
    else
    {
        CColorValue cvColDef = pNodeToUse->GetCascadedcolor();

        if(cvColDef.IsDefined())
        {
            hr = THR(cvColDef.FormatBuffer(szBuffer, ARRAY_SIZE(szBuffer), NULL));
            if(hr)
                goto Cleanup;
            hr = THR(FormsAllocString(szBuffer, &(V_BSTR(p))));
        }
    }


Cleanup:
    RRETURN(hr);
}


//+----------------------------------------------------------------
//
//  member : GetUnitValueHelper
//
//  Synopsis : helper function used by functions gettting from
//                  CUnitValue
//
//+----------------------------------------------------------------

HRESULT
CCurrentStyle::GetUnitValueHelper(
    VARIANT *p,
    CUnitValue uvVal,
    const PROPERTYDESC *pDesc,
    const ENUMDESC *pEnDesc /* = NULL */,
    const int nDefEnumValue /* = 0 */)
{
    HRESULT      hr = S_OK;

    NULLPTRCHECK(p, E_POINTER)

    V_VT(p) = VT_EMPTY;

    if(pEnDesc && uvVal.IsNull())
        uvVal.SetRawValue(MAKEUNITVALUE(nDefEnumValue, UNIT_ENUM));

    if(uvVal.GetUnitType() == CUnitValue::UNIT_ENUM)
    {
        if(pEnDesc == NULL)
        {
            Assert(0 && "Unexpeted Unit value of enum type");
            hr = E_FAIL;
            goto Cleanup;
        }

        hr = THR(pEnDesc->StringFromEnum(uvVal.GetUnitValue(), &(V_BSTR(p))));
    }
    else
    {
        TCHAR        szBuffer[pdlLength];

        // We aways want the unit values to be appended, so we pass in true
        hr = THR(uvVal.FormatBuffer(szBuffer, ARRAY_SIZE(szBuffer), pDesc, TRUE)) ;
        if(hr)
            goto Cleanup;
        hr = THR(FormsAllocString(szBuffer, &(V_BSTR(p))));
    }
    if(hr)
        goto Cleanup;
    V_VT(p) = VT_BSTR;

Cleanup:
    RRETURN(hr);
}



//+----------------------------------------------------------------
//
//  member : GetImageNameFromCookie
//
//  Synopsis : helper function that allocates and return the url corresponging
//               to given cookie enclosion the url in url("...")
//
//+----------------------------------------------------------------

HRESULT
CCurrentStyle::GetImageNameFromCookie(long lCookie, BSTR *p)
{
    HRESULT hr = S_OK;
    NODE_IN_TREE_CHECK()
    if(lCookie)
    {
        CImgCtx *pCtx = pNodeToUse->Doc()->GetUrlImgCtx(lCookie);
        if (!pCtx)
        {
            *p = NULL;
        }
        else
        {
            CBufferedStr szBufSt;
            szBufSt.Set(_T("url(\""));
            szBufSt.QuickAppend(pCtx->GetUrl());
            szBufSt.QuickAppend(_T("\")"));
            hr = THR(FormsAllocString(szBufSt, p));
        }
    }
    else
    {
        *p = NULL;
    }

Cleanup:
    RRETURN(hr);
}


//+----------------------------------------------------------------
//
//  member : GetBorderStyle
//
//  Synopsis : Helper function
//
//+----------------------------------------------------------------

styleBorderStyle
CCurrentStyle::GetBorderStyle(int nBorder)
{
    CDocInfo            di;
    CBorderInfo         bi;
    DISPNODEBORDER      brdrType;
    HRESULT             hr;

    NODE_IN_TREE_CHECK()

    brdrType = (DISPNODEBORDER)pNodeToUse->Element()->GetBorderInfo(&di, &bi, FALSE);
    if(brdrType == DISPNODEBORDER_NONE)
        return styleBorderStyleNone;
    else
        return ConvertFmToCSSBorderStyle(bi.abStyles[nBorder]);
Cleanup:    
    // It should never come here because calles should have called NODE_IN_TREE_CHECK()
    AssertSz(0, "Caller should have called NODE_IN_TREE_CHECK()");
    return styleBorderStyleNone;
}



//+----------------------------------------------------------------
//
//  member : GetCachedExpandoAA
//
//  Synopsis : Returns the attribute array that contains the expandos
//               propagated from the style.
//+----------------------------------------------------------------

CAttrArray *
CCurrentStyle::GetCachedExpandoAA()
{
    HRESULT hr;
    CAttrArray          * pAA = NULL;
    const CFancyFormat  * pFF;

    NODE_IN_TREE_CHECK()

    if(pNodeToUse->_iFF == -1)
    {
         // This makes sure ComputeFormat is called and style expandos are propagated
        pNodeToUse->GetFancyFormatIndex();
        Assert(pNodeToUse->_iFF != -1);
    }

    pFF = GetFancyFormatEx(pNodeToUse->_iFF);
    if(pFF->_iExpandos != -1)
        pAA = GetExpandosAttrArrayFromCacheEx(pFF->_iExpandos);

Cleanup:
    return pAA;
}

//+----------------------------------------------------------------------------
//
//  Function:   CCurrentStyle::GetCanonicalProperty
//
//  Synopsis:   Returns the canonical pUnk/dispid pair for a particular dispid
//              Used by the recalc engine to catch aliased properties.
//
//  Parameters: ppUnk will contain the canonical object
//              pdispid will contain the canonical dispid
//
//  Returns:    S_OK if successful
//              S_FALSE if property has no alias
//
//-----------------------------------------------------------------------------

HRESULT
CCurrentStyle::GetCanonicalProperty(DISPID dispid, IUnknown **ppUnk, DISPID *pdispid)
{
    HRESULT hr;

    NODE_IN_TREE_CHECK()

    switch (dispid)
    {
    case DISPID_IHTMLSTYLE_LEFT:
        *pdispid = DISPID_IHTMLELEMENT_OFFSETLEFT;
        hr = THR(pNodeToUse->Element()->PrivateQueryInterface(IID_IUnknown, (LPVOID *) ppUnk));
        break;
    case DISPID_IHTMLSTYLE_TOP:
        *pdispid = DISPID_IHTMLELEMENT_OFFSETTOP;
        hr = THR(pNodeToUse->Element()->PrivateQueryInterface(IID_IUnknown, (LPVOID *) ppUnk));
        break;

    case DISPID_IHTMLSTYLE_WIDTH:
        *pdispid = DISPID_IHTMLELEMENT_OFFSETWIDTH;
        hr = THR(pNodeToUse->Element()->PrivateQueryInterface(IID_IUnknown, (LPVOID *) ppUnk));
        break;

    case DISPID_IHTMLSTYLE_HEIGHT:
        *pdispid = DISPID_IHTMLELEMENT_OFFSETHEIGHT;
        hr = THR(pNodeToUse->Element()->PrivateQueryInterface(IID_IUnknown, (LPVOID *) ppUnk));
        break;
    default:
        *ppUnk = 0;
        *pdispid = 0;
        hr = S_FALSE;
    }

Cleanup:
    RRETURN1(hr, S_FALSE);
}

