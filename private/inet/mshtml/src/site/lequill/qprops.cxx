//+----------------------------------------------------------------------------
//
// File:        qprops.CXX
//
// Contents:    Implementation of ITextProperties for Trident
//
// Copyright (c) 1998 Microsoft Corporation. All rights reserved.
//
//-----------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_CFPF_HXX_
#define X_CFPF_HXX_
#include "cfpf.hxx"
#endif


#include "qprops.hxx"

// define offset() macro
// NOTE: don't define it in the header file, because of conflict with
// Trident's variable name.
#ifndef offset
#define offset(type, field) ((int) (&((type *) 0)->field))
#endif // offset

#define ptsInch		72		// number of points per inch
#define PtsFromDxp(dxp) (g_sizePixelsPerInch.cx ? ((float) (dxp) * ptsInch / g_sizePixelsPerInch.cx) : 1)
#define PtsFromDyp(dyp) (g_sizePixelsPerInch.cy ? ((float) (dyp) * ptsInch / g_sizePixelsPerInch.cy) : 1)

enum
	{
	xDirection,
	yDirection,
	};

//
// CF Property Tables
//
TextPropertyDescription rgpropCF[] =
{
	{ tptagCharFirstTag               , tptReserved },
	{ tptagBold                   	  , tptBit, tpuBool, sizeof(WORD),	offset(CCharFormat, _wFontSpecificFlagsVar), 0x01 }, /*_fBold*/
	{ tptagItalic                     , tptBit, tpuBool, sizeof(WORD),	offset(CCharFormat, _wFontSpecificFlagsVar), 0x02 }, /*_fItalic*/
	{ tptagUnderline                  , tptCustom, tpuBool, sizeof(WORD), offset(CCharFormat, _wFlagsVar), 0x01, tpfCustomFetch },
	{ tptagOutline                    , tptReserved },
	{ tptagShadow                     , tptReserved },
	{ tptagStrikethrough              , tptReserved },
	{ tptagSmallCaps                  , tptReserved },
	{ tptagCaps                       , tptReserved },
	{ tptagVanish                     , tptCustom, tpuBool, sizeof(WORD),	offset(CCharFormat, _wFlagsVar), 0x0010, tpfCustomFetch}, /*_fVisibilityHidden*/
	{ tptagProtected                  , tptReserved },
	{ tptagEmboss                     , tptReserved },
	{ tptagEngrave                    , tptReserved },
	{ tptagIss                        , tptReserved },
	{ tptagSfx                        , tptReserved },
	{ tptagCsk                        , tptReserved },
	{ tptagCrFore                     , tptCustom, tpuNative, sizeof(DWORD), offset(CCharFormat, _ccvTextColor), 0, tpfCustomFetch },
	{ tptagCrBack                     , tptCustom, tpuNative, sizeof(DWORD), 0/*offset(CFancyFormat, _ccvBackColor)*/, 0, tpfCustomFetch },
	{ tptagCrShadow                   , tptReserved },
	{ tptagCrEffectFill               , tptReserved },
	{ tptagFont                       , tptI4, tpuNative, sizeof(LONG), offset(CCharFormat, _latmFaceName) },
	{ tptagFps                        , tptI4, tpuUnknown, sizeof(LONG), offset(CCharFormat, _yHeight) },
	{ tptagFpsKern                    , tptReserved },
	{ tptagDylPos                     , tptReserved },
	{ tptagDxlSpace                   , tptCustom, tpuPoint, sizeof(DWORD), offset(CCharFormat, _cuvLetterSpacing), 0, tpfCustomFetch },
	{ tptagIsty                       , tptReserved },
	{ tptagLcid                       , tptUnknown, tpuNative, sizeof(LCID),offset(CCharFormat, _lcid) },
	{ tptagCharClientLong             , tptReserved },
	{ tptagCtk                        , tptReserved },
	{ tptagCtkLong                    , tptReserved },
	{ tptagYsr                        , tptReserved },
	{ tptagFss                        , tptReserved },
	{ tptagFcs                        , tptReserved },
	{ tptagPctShadowHorzOffset        , tptReserved },
	{ tptagPctShadowVertOffset        , tptReserved },
	{ tptagQem                        , tptReserved },
	{ tptagNoProofing                 , tptReserved },
	{ tptagPassword                   , tptBit, tpuBool, sizeof(WORD),	offset(CCharFormat, _wFontSpecificFlagsVar), 0x0020 }, /*_fPassword*/
	{ tptagFontName                   , tptCustom, tpuNative, sizeof(TCHAR*), 0, 0, tpfCustomFetch },
	{ tptagFfc                        , tptReserved },
};

TextPropertyDictionary dictCF = 
{ 
	sizeof(rgpropCF) / sizeof(TextPropertyDescription), 
    tptagCharFirstTag,
	rgpropCF
};

//
// custom property access for CF
//
void *CCharFormatPropertyAccess::get_This()
{
    return m_pThis;
}

HRESULT CCharFormatPropertyAccess::FetchProperty(int tag, int nUnit, const void *pvSrc, void *pvDst, int cbDst)
{
	Assert(pvSrc && pvDst);
	 
	switch (tag)
		{
	case tptagCrFore:
	case tptagCrBack:
		{
		Assert(cbDst == sizeof(COLORREF));
        CColorValue *pCColorValue = NULL;
        COLORREF cr;

        if (tag == tptagCrFore)
            pCColorValue = (CColorValue*)pvSrc;
        else if (get_FF() != NULL)
            pCColorValue = &(get_FF()->_ccvBackColor);

        if (!pCColorValue || pCColorValue->GetType() == CColorValue::TYPE_UNDEF)
            cr = tpMaskColorUndefined;
        else if (pCColorValue->GetType() == CColorValue::TYPE_TRANSPARENT)
            cr = tpMaskColorTransparent;
        else
            cr = pCColorValue->GetColorRef() & ~tpMaskColor;

		memcpy(pvDst, &cr, cbDst);
		}
		break;

	case tptagUnderline:
		{
    	int underlineType;

        if (*((int*)pvSrc))
            {
		    switch (((CCharFormat*)get_This())->_bUnderlineType)
			    {
		    case CFU_UNDERLINE:
			    underlineType = tpvalUnderlineSingle;
			    break;
		    case CFU_UNDERLINEWORD:
			    underlineType = tpvalUnderlineWord;
			    break;
		    case CFU_UNDERLINEDOUBLE:
			    underlineType = tpvalUnderlineDouble;
			    break;
		    case CFU_UNDERLINEDOTTED:
			    underlineType = tpvalUnderlineDot;
			    break;
		    default:
			    underlineType = tpvalUnderlineNone;
			    }
            }
        else
		    underlineType = tpvalUnderlineNone;

		memcpy(pvDst, &underlineType, cbDst);
		}
		break;

	case tptagFontName:
		{
		Assert(sizeof(TCHAR) == sizeof(WCHAR)); // Make sure we are working in Unicode
		Assert(cbDst == sizeof(WCHAR*));

        CCharFormat *pCCharFormat = (CCharFormat*)get_This();
        *((WCHAR**)pvDst) = pCCharFormat->GetFaceName();
		}
		break;

	case tptagDxlSpace:
		{
		Assert(cbDst == sizeof(float));
        CUnitValue *pCUnitValue = (CUnitValue*)pvSrc;
		float value;

   	    if (!pCUnitValue->IsNull())
            {
            if (pCUnitValue->GetUnitType() >= (CUnitValue::UNIT_PERCENT))
        	    value = PtsFromDxp(XResolvePercentageValue(pCUnitValue, ((CCharFormat*)get_This())->_yHeight));
            else
                value = pCUnitValue->XGetFloatValueInUnits(CUnitValue::UNIT_POINT);
            }
        else
            value = 0;
       
		memcpy(pvDst, &value, cbDst);
		}
		break;

	case tptagVanish:
        *((int*)pvDst) = ((CCharFormat*)get_This())->IsDisplayNone();
		break;

	default:
		AssertSz(0, "Unsupported custom fetch paragraph property");
	    return E_NOTIMPL;
		}

    return S_OK;
}

HRESULT CCharFormatPropertyAccess::ApplyProperty(int tag, int nUnit, const void *pvSrc, void *pvDst, int cb)
{
    return E_NOTIMPL;
}

HRESULT CCharFormatPropertyAccess::CompareProperty(int tag, int nUnit, const void *pvSrc, const void *pvDst)
{
    return E_NOTIMPL;
}


//
// PF Property Tables
//
TextPropertyDescription rgpropPF[] =
{
	{ tptagParaFirstTag               , tptReserved },
	{ tptagHorizontalAlignment     	  , tptCustom, tpuNative, sizeof(char), offset(CParaFormat, _bBlockAlign), 0, tpfCustomFetch },
	{ tptagVerticalAlignment          , tptReserved },
	{ tptagClnDropCap                 , tptReserved },
	{ tptagClnDropCapRaised           , tptReserved },
	{ tptagCchDropCap                 , tptReserved },
	{ tptagPageBreakBefore            , tptReserved },
	{ tptagLineSpacing     	          , tptCustom, tpuPoint, sizeof(float), 0/*offset(CFancyFormat, _cuvTextIndent)*/, 0, tpfCustomFetch },
	{ tptagIndentFirst     	          , tptCustom, tpuPoint, sizeof(float), 0/*offset(CFancyFormat, _cuvTextIndent)*/, 0, tpfCustomFetch },
	{ tptagIndentLeft     	          , tptCustom, tpuPoint, sizeof(float), offset(CParaFormat, _cuvLeftIndentPoints), 0, tpfCustomFetch },
	{ tptagIndentRight           	  , tptCustom, tpuPoint, sizeof(float), offset(CParaFormat, _cuvRightIndentPoints), 0, tpfCustomFetch },
	{ tptagSpaceBefore                , tptCustom, tpuPoint, sizeof(float), 0/*offset(CFancyFormat, _cuvSpaceBefore)*/, 0, tpfCustomFetch },
	{ tptagSpaceAfter                 , tptCustom, tpuPoint, sizeof(float), 0/*offset(CFancyFormat, _cuvSpaceAfter)*/, 0, tpfCustomFetch },
};

TextPropertyDictionary dictPF = 
{
	sizeof(rgpropPF) / sizeof(TextPropertyDescription), 
    tptagParaFirstTag,
	rgpropPF
};

//
// custom property access for PF
//
void *CParaFormatPropertyAccess::get_This()
{
    return m_pThis;
}

HRESULT CParaFormatPropertyAccess::FetchProperty(int tag, int nUnit, const void *pvSrc, void *pvDst, int cbDst)
{
	Assert(pvSrc && pvDst);

    CUnitValue *pCUnitValue = NULL;
	int direction = xDirection;

	switch (tag)
		{
	case tptagHorizontalAlignment:
		{
		Assert(nUnit == tpuNative && cbDst <= sizeof(int));
		int propDst;

		switch (*(int*)pvSrc)
			{
		case htmlBlockAlignNotSet:
		case htmlBlockAlignLeft:
			propDst = tpvalAlignmentLeft;
			break;
		case htmlBlockAlignCenter:
			propDst = tpvalAlignmentCenter;
			break;
		case htmlBlockAlignRight:
			propDst = tpvalAlignmentRight;
			break;
		case htmlBlockAlignJustify:
			propDst = tpvalAlignmentJustified;
			break;
		default:
			AssertSz(0, "Unsupported horizontal alignment mode");
		    return E_NOTIMPL;
			}
		memcpy(pvDst, &propDst, cbDst);
		}
		break;

	case tptagIndentLeft:
		{
		Assert(cbDst == sizeof(float));
        CParaFormat *pCParaFormat = (CParaFormat*)get_This();

        // NOTE(azmyh): it may be easier to use pCParaFormat->GetLeftIndent(..), but it returns indent
        // in pixel which we have to convert back to points (unnecessay if we don't have percentage values)
        float value = pCParaFormat->_cuvLeftIndentPoints.XGetFloatValueInUnits(CUnitValue::UNIT_POINT);

	    if (!pCParaFormat->_cuvLeftIndentPercent.IsNull())
            {
            LONG lValue = pCParaFormat->_cuvLeftIndentPercent.XGetPixelValue(get_ParentInfo(),
                                        get_ParentInfo()->_sizeParent.cx, pCParaFormat->_lFontHeightTwips);
            value += PtsFromDxp(lValue);
            }

		memcpy(pvDst, &value, cbDst);
		}
		break;

	case tptagIndentRight:
		{
		Assert(cbDst == sizeof(float));
        CParaFormat *pCParaFormat = (CParaFormat*)get_This();

        float value = pCParaFormat->_cuvRightIndentPoints.XGetFloatValueInUnits(CUnitValue::UNIT_POINT);

	    if (!pCParaFormat->_cuvRightIndentPercent.IsNull())
            {
            LONG lValue = pCParaFormat->_cuvRightIndentPercent.XGetPixelValue(get_ParentInfo(),
                                        get_ParentInfo()->_sizeParent.cx, pCParaFormat->_lFontHeightTwips);
            value += PtsFromDxp(lValue);
            }

		memcpy(pvDst, &value, cbDst);
		}
		break;

	case tptagIndentFirst:
        pCUnitValue = &((CParaFormat*)get_This())->_cuvTextIndent;
        break;

	case tptagSpaceBefore:
        pCUnitValue = &get_FF()->_cuvSpaceBefore;
		direction = yDirection;
        break;

	case tptagSpaceAfter:
        pCUnitValue = &get_FF()->_cuvSpaceAfter;
		direction = yDirection;
        break;

	case tptagLineSpacing:
        pCUnitValue = &get_CF()->_cuvLineHeight;
		direction = yDirection;
        break;

	default:
		AssertSz(0, "Unsupported custom fetch paragraph property");
	    return E_NOTIMPL;
		}


    // Do unit conversion for properties that need it.
    if (pCUnitValue)
		{
		Assert(cbDst == sizeof(float));
		float value;

   	    if (!pCUnitValue->IsNull())
            {
            if (pCUnitValue->GetUnitType() >= (CUnitValue::UNIT_PERCENT))
				{
				if (direction == xDirection)
        			value = PtsFromDxp(pCUnitValue->XGetPixelValue(get_ParentInfo(), get_ParentInfo()->_sizeParent.cx, ((CParaFormat*)get_This())->_lFontHeightTwips));
				else
        			value = PtsFromDyp(pCUnitValue->YGetPixelValue(get_ParentInfo(), get_ParentInfo()->_sizeParent.cy, ((CParaFormat*)get_This())->_lFontHeightTwips));
				}
            else
                value = pCUnitValue->XGetFloatValueInUnits(CUnitValue::UNIT_POINT, ((CParaFormat*)get_This())->_lFontHeightTwips);
            }
        else
            value = 0;
       
		memcpy(pvDst, &value, cbDst);
		}

    return S_OK;
}

HRESULT CParaFormatPropertyAccess::ApplyProperty(int tag, int nUnit, const void *pvSrc, void *pvDst, int cb)
{
    return E_NOTIMPL;
}

HRESULT CParaFormatPropertyAccess::CompareProperty(int tag, int nUnit, const void *pvSrc, const void *pvDst)
{
    return E_NOTIMPL;
}

