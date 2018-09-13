/*
 *  @doc    INTERNAL
 *
 *  @module LSMISC.CXX -- line services misc support
 *
 *
 *  Owner: <nl>
 *      Chris Thrasher <nl>
 *      Sujal Parikh <nl>
 *
 *  History: <nl>
 *      01/05/98     cthrash created
 *
 *  Copyright (c) 1997-1998 Microsoft Corporation. All rights reserved.
 */

#include "headers.hxx"

#ifndef X__FONT_H_
#define X__FONT_H_
#include "_font.h"
#endif

#ifndef X_CDUTIL_HXX_
#define X_CDUTIL_HXX_
#include "cdutil.hxx"
#endif

#ifndef X_TREEPOS_HXX_
#define X_TREEPOS_HXX_
#include "treepos.hxx"
#endif

#ifndef X_LINESRV_HXX_
#define X_LINESRV_HXX_
#include "linesrv.hxx"
#endif

#ifndef X_TXTDEFS_H_
#define X_TXTDEFS_H_
#include "txtdefs.h"
#endif

#ifndef X_LSPAP_H_
#define X_LSPAP_H_
#include <lspap.h>
#endif

#ifndef X_LSCHP_H_
#define X_LSCHP_H_
#include <lschp.h>
#endif

#ifndef X_LSFFI_H_
#define X_LSFFI_H_
#include <lsffi.h>
#endif

#ifndef X_OBJDIM_H_
#define X_OBJDIM_H_
#include <objdim.h>
#endif

#ifndef X_LIMITS_H_
#define X_LIMITS_H_
#include <limits.h>
#endif

#ifndef X_LSKTAB_H_
#define X_LSKTAB_H_
#include <lsktab.h>
#endif

#ifndef X_LSENUM_H_
#define X_LSENUM_H_
#include <lsenum.h>
#endif

#ifndef X_POSICHNK_H_
#define X_POSICHNK_H_
#include <posichnk.h>
#endif

#ifndef X_LSRENDER_HXX_
#define X_LSRENDER_HXX_
#include "lsrender.hxx"
#endif

#ifndef X_FLOWLYT_HXX_
#define X_FLOWLYT_HXX_
#include <flowlyt.hxx>
#endif

#ifndef X_LTCELL_HXX_
#define X_LTCELL_HXX_
#include <ltcell.hxx>
#endif

#ifndef X_ALTFONT_H_
#define X_ALTFONT_H_
#include <altfont.h>
#endif

#ifndef X__FONTLINK_H_
#define X__FONTLINK_H_
#include "_fontlnk.h"
#endif

#ifndef X_STYLE_HXX_
#define X_STYLE_HXX_
#include "style.hxx"
#endif

#ifndef X_INTL_HXX_
#define X_INTL_HXX_
#include <intl.hxx>
#endif

#ifndef X_MLANG_H_
#define X_MLANG_H_
#include <mlang.h>
#endif

#define MSLS_MIN_VERSION 323
#define MSLS_MAX_VERSION INT_MAX
#define MSLS_BUILD_LOCATION "\\\\word2\\lineserv\\rel0336"

DeclareLSTag(tagLSAllowEmergenyBreaks, "Allow emergency breaks");
DeclareLSTag(tagLSTraceLines, "Trace plsline setup/discardal");

DeclareTag(tagCCcsCacheHits, "LineServices", "Trace Ccs cache hit %");
DeclareTag(tagFontLinkFonts, "Font", "Trace fontlinking on selected text");

MtDefine(QueryLinePointPcp_aryLsqsubinfo_pv, Locals, "CLineServices::QueryLinePointPcp::aryLsqsubinfo_pv");
MtDefine(QueryLineCpPpoint_aryLsqsubinfo_pv, Locals, "CLineServices::QueryLineCpPpoint::aryLsqsubinfo_pv");
MtDefine(LSVerCheck, Locals, "LS Version check")

#if DBG==1
void CLineServices::InitTimeSanityCheck()
{
    //
    // First verify we're looking at the right version of msls.
    //

    static BOOL fCheckedVersion = FALSE;

    if (!fCheckedVersion)
    {
        BOOL fAOK = FALSE;
        extern DYNLIB g_dynlibMSLS;

        AssertSz( g_dynlibMSLS.hinst, "Line Services (msls31.dll) was not loaded.  This is bad.");

        if (g_dynlibMSLS.hinst)
        {
            char achPath[MAX_PATH];

            if (GetModuleFileNameA( g_dynlibMSLS.hinst, achPath, sizeof(achPath) ))
            {
                DWORD dwHandle;
                DWORD dwVerInfoSize = GetFileVersionInfoSizeA(achPath, &dwHandle);

                if (dwVerInfoSize)
                {
                    void * lpBuffer = MemAlloc( Mt(LSVerCheck), dwVerInfoSize );

                    if (lpBuffer)
                    {
                        if (GetFileVersionInfoA(achPath, dwHandle, dwVerInfoSize, lpBuffer))
                        {
                            char * pchVersion;
                            UINT uiLen;
                                                    
                            if (VerQueryValueA(lpBuffer, "\\StringFileInfo\\040904E4\\FileVersion", (void **)&pchVersion, &uiLen) && uiLen)
                            {
                                char * pchDot = StrChrA( pchVersion, '.' );

                                if (pchDot)
                                {
                                    pchDot = StrChrA( pchDot + 1, '.' );

                                    if (pchDot)
                                    {
                                        int iVersion = atoi(pchDot + 1);

                                        fAOK = iVersion >= MSLS_MIN_VERSION && iVersion <= MSLS_MAX_VERSION;
                                    }
                                }
                            }
                        }

                        MemFree(lpBuffer);
                    }
                }
            }
        }

        fCheckedVersion = TRUE;

        AssertSz(fAOK, "MSLS31.DLL version mismatch.  You should get a new version from " MSLS_BUILD_LOCATION );
    }

    
    // lskt values should be indentical tomAlign values
    

    AssertSz( lsktLeft == tomAlignLeft &&
              lsktCenter == tomAlignCenter &&
              lsktRight == tomAlignRight &&
              lsktDecimal == tomAlignDecimal,
              "enum values have changed!" );

    AssertSz( tomSpaces == 0 &&
              tomDots == 1 &&
              tomDashes == 2 &&
              tomLines == 3,
              "enum values have changed!" );

    // Checks for synthetic characters.
    
    AssertSz( (SYNTHTYPE_REVERSE & 1) == 0 &&
              (SYNTHTYPE_ENDREVERSE & 1) == 1,
              "synthtypes order has been broken" );

    // The breaking rules rely on this to be true.

    Assert( long(ichnkOutside) < 0 );

    // Check some basic classifications.

    Assert( !IsGlyphableChar(WCH_NBSP) &&
            !IsGlyphableChar(TEXT('\r')) &&
            !IsGlyphableChar(TEXT('\n')) &&
            !IsRTLChar(WCH_NBSP) &&
            !IsRTLChar(TEXT('\r')) &&
            !IsRTLChar(TEXT('\n')) );

    // Check the range for justification (uses table lookup)

    AssertSz(    0 == styleTextJustifyNotSet
              && 1 == styleTextJustifyInterWord
              && 2 == styleTextJustifyNewspaper
              && 3 == styleTextJustifyDistribute
              && 4 == styleTextJustifyDistributeAllLines
              && 5 == styleTextJustifyInterIdeograph
              && 6 == styleTextJustifyAuto,
              "CSS text-justify values have changed.");
    //
    // Test alternate font functionality.
    // BUGBUG (cthrash) This test doesn't really belong here.

    {
        const WCHAR * pchMSGothic;

        pchMSGothic = AlternateFontName( L"\xFF2D\xFF33\x0020\x30B4\x30B7\x30C3\x30AF" );

        Assert( pchMSGothic && StrCmpC( L"MS Gothic", pchMSGothic ) == 0 );
        
        pchMSGothic = AlternateFontName( L"ms GOthic" );

        Assert( pchMSGothic && StrCmpC( L"\xFF2D\xFF33\x0020\x30B4\x30B7\x30C3\x30AF", pchMSGothic ) == 0 );
    }
}
#endif

//+----------------------------------------------------------------------------
//
// Member:      CLineServices::SetRenderer
//
// Synopsis:    Sets up the line services object to indicate that we will be
//              using for rendering.
//
//+----------------------------------------------------------------------------
void
CLineServices::SetRenderer(CLSRenderer *pRenderer, BOOL fWrapLongLines, const CCharFormat *pCFLi)
{
    _pMeasurer = pRenderer;
    _pCFLi     = pCFLi;
    _lsMode    = LSMODE_RENDERER;
    _fWrapLongLines = fWrapLongLines;
}

//+----------------------------------------------------------------------------
//
// Member:      CLineServices::SetMeasurer
//
// Synopsis:    Sets up the line services object to indicate that we will be
//              using for measuring / hittesting.
//
//+----------------------------------------------------------------------------
void
CLineServices::SetMeasurer(CLSMeasurer *pMeasurer, LSMODE lsMode, BOOL fWrapLongLines)
{
    _pMeasurer = pMeasurer;
    Assert(lsMode != LSMODE_NONE && lsMode != LSMODE_RENDERER);
    _lsMode    = lsMode;
    _fWrapLongLines = fWrapLongLines;
}

//+----------------------------------------------------------------------------
//
// Member:      CLineServices::GetRenderer
//
//+----------------------------------------------------------------------------
CLSRenderer *
CLineServices::GetRenderer()
{
    return (_pMeasurer && _lsMode == LSMODE_RENDERER) ?
            DYNCAST(CLSRenderer, _pMeasurer) : NULL;
}

//+----------------------------------------------------------------------------
//
// Member:      CLineServices::GetMeasurer
//
//+----------------------------------------------------------------------------
CLSMeasurer *
CLineServices::GetMeasurer()
{
    return (_pMeasurer && _lsMode == LSMODE_MEASURER) ? _pMeasurer : NULL;
}

//+----------------------------------------------------------------------------
//
//  Member:     CLineServices::PAPFromPF
//
//  Synopsis:   Construct a PAP from PF
//
//-----------------------------------------------------------------------------

static const BYTE s_ablskjustMap[] =
{
    lskjFullInterWord,          // NotSet (default)
    lskjFullInterWord,          // InterWord
    lskjFullInterLetterAligned, // Newspaper
    lskjFullInterLetterAligned, // Distribute
    lskjFullInterLetterAligned, // DistributeAllLines
    lskjFullScaled,             // InterIdeograph
    lskjFullInterWord           // Auto (?)
};

void
CLineServices::PAPFromPF(PLSPAP pap, const CParaFormat *pPF, BOOL fInnerPF, CComplexRun *pcr)
{
    _fExpectCRLF = pPF->HasPre(fInnerPF) || _fIsTextLess;
    const BOOL fJustified = pPF->GetBlockAlign(fInnerPF) == htmlBlockAlignJustify
                            && !pPF->HasInclEOLWhite(fInnerPF)
                            && !_fMinMaxPass;
        
    // line services format flags (lsffi.h)

    pap->grpf =   fFmiApplyBreakingRules     // Use our breaking tables
                | fFmiSpacesInfluenceHeight; // Whitespace contributes to extent

    if (_fWrapLongLines)
        pap->grpf |= fFmiWrapAllSpaces;
    else
        pap->grpf |= fFmiForceBreakAsNext;   // No emergency breaks

#if DBG==1
    // To test possible bugs with overriding emergency breaks in LS, we
    // provide an option here to turn it off.  Degenerate lines will break
    // at the wrapping width (default LS behavior.)

    if (IsTagEnabled(tagLSAllowEmergenyBreaks))
    {
        pap->grpf &= ~fFmiForceBreakAsNext;
    }
#endif // DBG

    pap->uaLeft = 0;
    pap->uaRightBreak = 0;
    pap->uaRightJustify = 0;
    pap->duaIndent = 0;
    pap->duaHyphenationZone = 0;
 
    // Justification type

    pap->lsbrj = lsbrjBreakJustify;

    // Justification

    if (fJustified)
    {
        _li._fJustified = JUSTIFY_FULL;
        
        // A. If we are a complex script set lskj to do glyphing
        if(pcr != NULL || _pMarkup->Doc()->GetCodePage() == CP_THAI)
        {
            pap->lskj = lskjFullGlyphs;


            if(!pPF->_cuvTextKashida.IsNull())
            {
                Assert(_xWidthMaxAvail > 0);
                pap->lsbrj = lsbrjBreakThenExpand;

                // set the amount of the right break
                long xKashidaPercent = pPF->_cuvTextKashida.GetPercentValue(CUnitValue::DIRECTION_CX, _xWidthMaxAvail);
                _xWrappingWidth = _xWidthMaxAvail - xKashidaPercent;
                
                // we need to set this amount as twips
                Assert(_pci);
                pap->uaRightBreak = _pci->TwipsFromDeviceCX(xKashidaPercent);

                Assert(_xWrappingWidth >= 0);
            }
        }
        else
        {
            pap->lskj = LSKJUST(s_ablskjustMap[ pPF->_uTextJustify ]);
        }
        _fExpansionOrCompression = pPF->_uTextJustify > styleTextJustifyInterWord;
    }
    else
    {
        _fExpansionOrCompression = FALSE;
        pap->lskj = lskjNone;
    }

    // Alignment

    pap->lskal = lskalLeft;

    // Autonumbering

    pap->duaAutoDecimalTab = 0;

    // kind of paragraph ending

    pap->lskeop = _fExpectCRLF ? lskeopEndPara1 : lskeopEndParaAlt;

    // Main text flow direction

    Assert(pPF->HasRTL(fInnerPF) == (BOOL) _li._fRTL);
    if (!_li._fRTL)
    {
        pap->lstflow = lstflowES;
    }
    else
    {
        if (_pBidiLine == NULL)
        {
            _pBidiLine = new CBidiLine(_treeInfo, _cpStart, _li._fRTL, _pli);
        }
#if DBG==1
        else
        {
            Assert(_pBidiLine->IsEqual(_treeInfo, _cpStart, _li._fRTL, _pli));
        }
#endif
        pap->lstflow = lstflowWS;
    }
}

//+----------------------------------------------------------------------------
//
//  Member:     CLineServices::CHPFromCF
//
//  Synopsis:   Construct a CHP from CF
//
//-----------------------------------------------------------------------------
void
CLineServices::CHPFromCF(
    COneRun * por,
    const CCharFormat * pCF )
{
    PLSCHP pchp = &por->_lsCharProps;

    // The structure has already been zero'd out in fetch run, which sets almost
    // everything we care about to the correct value (0).

    if (pCF->_fTextAutospace)
    {
        _lineFlags.AddLineFlag(por->Cp(), FLAG_HAS_NOBLAST);

        if (pCF->_fTextAutospace & TEXTAUTOSPACE_ALPHA)
        {
            pchp->fModWidthOnRun = TRUE;
            por->_csco |= cscoAutospacingAlpha;
        }
        if (pCF->_fTextAutospace & TEXTAUTOSPACE_NUMERIC)
        {
            pchp->fModWidthOnRun = TRUE;
            por->_csco |= cscoAutospacingDigit;
        }
        if (pCF->_fTextAutospace & TEXTAUTOSPACE_SPACE)
        {
            pchp->fModWidthSpace = TRUE;
            por->_csco |= cscoAutospacingAlpha;
        }
        if (pCF->_fTextAutospace & TEXTAUTOSPACE_PARENTHESIS)
        {
            pchp->fModWidthOnRun = TRUE;
            por->_csco |= cscoAutospacingParen;
        }
    }

    if (_fExpansionOrCompression)
    {
        pchp->fCompressOnRun = TRUE;
        pchp->fCompressSpace = TRUE; 
        pchp->fCompressTable = TRUE;
        pchp->fExpandOnRun = 0 == (pCF->_bPitchAndFamily & FF_SCRIPT);
        pchp->fExpandSpace = TRUE;
        pchp->fExpandTable = TRUE;
    }

    pchp->idObj = LSOBJID_TEXT;
}

//+----------------------------------------------------------------------------
//
//  Member:     CLineServices::SetPOLS
//
//  Synopsis:   We call this function when we assign a CLineServices
//              to a CLSMeasurer.
//
//-----------------------------------------------------------------------------
void
CLineServices::SetPOLS(CFlowLayout *pFlowLayout, CCalcInfo * pci)
{
    CTreePos * ptpStart, * ptpLast;
    CElement * pElementOwner = pFlowLayout->ElementOwner();

    _pFlowLayout = pFlowLayout;
    _fIsEditable = _pFlowLayout->IsEditable();
    _fIsTextLess = pElementOwner->HasFlag(TAGDESC_TEXTLESS);
    _fIsTD = pElementOwner->Tag() == ETAG_TD;
    _pMarkup = _pFlowLayout->GetContentMarkup();
    _fHasSites = FALSE;
    _pci = pci;
    _plsline = NULL;
    _chPassword = _pFlowLayout->GetPasswordCh();

    //
    // We have special wrapping rules inside TDs with width specified.
    // Make note so the ILS breaking routines can break correctly.
    //
    
    _xTDWidth = MAX_MEASURE_WIDTH;
    if (_fIsTD)
    {
        const LONG iUserWidth = DYNCAST(CTableCellLayout, pFlowLayout)->GetSpecifiedPixelWidth(pci);

        if (iUserWidth)
        {
            _xTDWidth = iUserWidth;
        }
    }

    ClearLinePropertyFlags();
    
    _pFlowLayout->GetContentTreeExtent(&ptpStart, &ptpLast);
    _treeInfo._cpLayoutFirst = ptpStart->GetCp() + 1;
    _treeInfo._cpLayoutLast  = ptpLast->GetCp();
    _treeInfo._ptpLayoutLast = ptpLast;
    _treeInfo._tpFrontier.BindToCp(0);
    
    InitChunkInfo(_treeInfo._cpLayoutFirst);

    _pPFFirst = NULL;
    WHEN_DBG( _cpStart = -1 );
}

//+----------------------------------------------------------------------------
//
//  Member:     CLineServices::ClearPOLS
//
//  Synopsis:   We call this function when we have finished using the measurer.
//
//-----------------------------------------------------------------------------
void
CLineServices::ClearPOLS()
{
    // This assert will fire if we are leaking lsline's.  This happens
    // if somebody calls LSDoCreateLine without ever calling DiscardLine.
    Assert(_plsline == NULL);
    _pMarginInfo = NULL;
    if (_plcFirstChunk)
        DeleteChunks();
    DiscardOneRuns();

    if (_pccsCache)
    {
        _pccsCache->Release();
        _pccsCache = NULL;
        _pCFCache = NULL;
    }
    if (_pccsAltCache)
    {
        _pccsAltCache->Release();
        _pccsAltCache = NULL;
        _pCFAltCache = NULL;
    }
}

static CCharFormat s_cfBullet;

//+----------------------------------------------------------------------------
//
//  Member:     CLineServices::GetCFSymbol
//
//  Synopsis:   Get the a CF for the symbol passed in and put it in the COneRun.
//
//-----------------------------------------------------------------------------
void
CLineServices::GetCFSymbol(COneRun *por, TCHAR chSymbol, const CCharFormat *pcfIn)
{
    static BOOL s_fBullet = FALSE;

    CCharFormat *pcfOut = por->GetOtherCF();

    Assert(pcfIn && pcfOut);
    if (pcfIn == NULL || pcfOut == NULL)
        goto Cleanup;

    if (!s_fBullet)
    {
        // N.B. (johnv) For some reason, Win95 does not render the Windings font properly
        //  for certain characters at less than 7 points.  Do not go below that size!
        s_cfBullet.SetHeightInTwips( TWIPS_FROM_POINTS ( 7 ) );
        s_cfBullet._bCharSet = SYMBOL_CHARSET;
        s_cfBullet._fNarrow = FALSE;
        s_cfBullet._bPitchAndFamily = (BYTE) FF_DONTCARE;
        s_cfBullet._latmFaceName= fc().GetAtomWingdings();
        s_cfBullet._bCrcFont = s_cfBullet.ComputeFontCrc();

        s_fBullet = TRUE;
    }

    // Use bullet char format
    *pcfOut = s_cfBullet;

    pcfOut->_ccvTextColor = pcfIn->_ccvTextColor;

    // Important - CM_SYMBOL is a special mode where out WC chars are actually
    // zero-extended MB chars.  This allows us to have a codepage-independent
    // call to ExTextOutA. (cthrash)
    por->SetConvertMode(CM_SYMBOL);

Cleanup:
    return;
}

//+----------------------------------------------------------------------------
//
//  Member:     CLineServices::GetCFNumber
//
//  Synopsis:   Get the a CF for the number passed in and put it in the COneRun.
//
//-----------------------------------------------------------------------------
void
CLineServices::GetCFNumber(COneRun *por, const CCharFormat * pcfIn)
{
    CCharFormat *pcfOut = por->GetOtherCF();

    *pcfOut = *pcfIn;
    pcfOut->_fSubscript = pcfOut->_fSuperscript = FALSE;
    pcfOut->_bCrcFont   = pcfOut->ComputeFontCrc();
}

LONG
CLineServices::GetDirLevel(LSCP lscp)
{
    LONG nLevel;
    COneRun *pHead;

    nLevel = _li._fRTL;
    for (pHead = _listCurrent._pHead; pHead; pHead = pHead->_pNext)
    {
        if (lscp >= pHead->_lscpBase)
        {
            if (pHead->IsSyntheticRun())
            {
                SYNTHTYPE synthtype = pHead->_synthType;
                // Since SYNTHTYPE_REVERSE preceeds SYNTHTYPE_ENDREVERSE and only
                // differs in the last bit, we can compute nLevel with bit magic. We
                // have to be sure this condition really exists of course, so we
                // Assert() it above.
                if (IN_RANGE(SYNTHTYPE_DIRECTION_FIRST, synthtype, SYNTHTYPE_DIRECTION_LAST))
                {
                    nLevel -= (((synthtype & 1) << 1) - 1);
                }
            }
        }
        else
            break;
    }

    return nLevel;
}

//-----------------------------------------------------------------------------
//
//  Member:     CLineFlags::AddLineFlag
//
//  Synopsis:   Set flags for a given cp.
//
//-----------------------------------------------------------------------------

LSERR
CLineFlags::AddLineFlag(LONG cp, DWORD dwlf)
{
#if 0
    CFlagEntry fe(cp, dwlf);

    return S_OK == _aryLineFlags.AppendIndirect(&fe)
            ? lserrNone
            : lserrOutOfMemory;
#else    
    int c = _aryLineFlags.Size();

    if (!c || cp >= _aryLineFlags[c-1]._cp)
    {
        CFlagEntry fe(cp, dwlf);

        return S_OK == _aryLineFlags.AppendIndirect(&fe)
                ? lserrNone
                : lserrOutOfMemory;
    }

  #if 0
    else
    {
        // LS will occasionally go back in remeasure the line.  Logically,
        // any flag it might set on the second iteration should be
        // identical to the those set in the first, provided that the
        // range was covered in the first iteration.   Nevertheless, here
        // we make sure that if LS chooses to rollback, we computed the
        // flags in the same manner.
        
        int i;

        for (i=0;i<c;i++)
        {
            LONG cpT = _aryLineFlags[i]._cp;

            if (cpT > cp)
            {
                break;
            }
            else if (cpT == cp && _aryLineFlags[i]._dwlf == dwlf)
            {
                return lserrNone;
            }
        }

        AssertSz( 0, "_aryLineFlags has been computed incorrectly" );
    }    
  #endif

    return lserrNone;
#endif
}

LSERR
CLineFlags::AddLineFlagForce(LONG cp, DWORD dwlf)
{
    CFlagEntry fe(cp, dwlf);

    _fForced = TRUE;
    return S_OK == _aryLineFlags.AppendIndirect(&fe)
            ? lserrNone
            : lserrOutOfMemory;
}

//+----------------------------------------------------------------------------
//
//  Member:     CLineServices::GetLineFlags
//
//  Synopsis:   Given a cp, it computes all the flags which have been turned
//              on till that cp.
//
//-----------------------------------------------------------------------------

DWORD
CLineFlags::GetLineFlags(LONG cpMax)
{
    DWORD dwlf;
    LONG i;
    
    dwlf = FLAG_NONE;
    
    for (i = 0; i < _aryLineFlags.Size(); i++)
    {
        if (_aryLineFlags[i]._cp >= cpMax)
        {
            if (_fForced)
                continue;
            else
                break;
        }
        else
            dwlf |= _aryLineFlags[i]._dwlf;
    }

#if DBG==1
    if (!_fForced)
    {
        //
        // This verifies that LS does indeed ask for runs in a monotonically
        // increasing manner as far as cp's are concerned
        //
        for (; i < _aryLineFlags.Size(); i++)
        {
            Assert(_aryLineFlags[i]._cp >= cpMax);
        }
    }
#endif

    return dwlf;
}

//+----------------------------------------------------------------------------
//
//  Member:     CLineServices::AddLineCount
//
//  Synopsis:   Adds a particular line count at a given cp. It also checks if the
//              count has already been added at that cp. This is needed to solve
//              2 problems with maintaining line counts:
//              1) A run can be fetched multiple times. In this case we want to
//                 increment the counts just once.
//              2) LS can over fetch runs, in which case we want to disregard
//                 the counts of those runs which did not end up on the line.
//
//-----------------------------------------------------------------------------
LSERR
CLineCounts::AddLineCount(LONG cp, LC_TYPE lcType, LONG count)
{
    CLineCount lc(cp, lcType, count);
    int i = _aryLineCounts.Size();

    while (i--)
    {
        if (_aryLineCounts[i]._cp != cp)
            break;

        if (_aryLineCounts[i]._lcType == lcType)
            return lserrNone;
    }
    
    return S_OK == _aryLineCounts.AppendIndirect(&lc)
            ? lserrNone
            : lserrOutOfMemory;
}

//+----------------------------------------------------------------------------
//
//  Member:     CLineServices::GetLineCount
//
//  Synopsis:   Finds a particular line count at a given cp.
//
//-----------------------------------------------------------------------------
LONG
CLineCounts::GetLineCount(LONG cp, LC_TYPE lcType)
{
    LONG count = 0;

    for (LONG i = 0; i < _aryLineCounts.Size(); i++)
    {
        if (   _aryLineCounts[i]._lcType == lcType
            && _aryLineCounts[i]._cp < cp
           )
        {
            count += _aryLineCounts[i]._count;
        }
    }
    return count;
}

//+----------------------------------------------------------------------------
//
//  Member:     CLineServices::GetMaxLineValue
//
//  Synopsis:   Finds a particular line value uptil a given cp.
//
//-----------------------------------------------------------------------------
LONG
CLineCounts::GetMaxLineValue(LONG cp, LC_TYPE lcType)
{
    LONG value = LONG_MIN;

    for (LONG i = 0; i < _aryLineCounts.Size(); i++)
    {
        if (   _aryLineCounts[i]._lcType == lcType
            && _aryLineCounts[i]._cp < cp
           )
        {
            value = max(value, _aryLineCounts[i]._count);
        }
    }
    return value;
}

#define MIN_FOR_LS 1

HRESULT
CLineServices::Setup(
    LONG xWidthMaxAvail,
    LONG cp,
    CTreePos *ptp,
    const CMarginInfo *pMarginInfo,
    const CLine * pli,
    BOOL fMinMaxPass )
{
    const CParaFormat *pPF;
    BOOL fWrapLongLines = _fWrapLongLines;
    HRESULT hr = S_OK;
    
#ifndef QUILL    
    Assert(_pMeasurer);

    if (!_treeInfo._fInited || cp != long(_pMeasurer->GetCp()))
#else
    if (!_treeInfo._fInited)
#endif  // QUILL
    {
        DiscardOneRuns();
        hr = THR(_treeInfo.InitializeTreeInfo(_pFlowLayout, _fIsEditable, cp, ptp));
        if (hr != S_OK)
            goto Cleanup;
    }

    _lineFlags.InitLineFlags();
    _cpStart     = _cpAccountedTill = cp;
    _pMarginInfo = pMarginInfo;
    _cWhiteAtBOL = 0;
    _cAlignedSitesAtBOL = 0;
    _cInlinedSites = 0;
    _cAbsoluteSites = 0;
    _cAlignedSites = 0;
    _lCharGridSize = 0;
    _lLineGridSize = 0;
    _pNodeForAfterSpace = NULL;
    if (_lsMode == LSMODE_MEASURER)
    {
        _pli = NULL;
    } 
    else
    {
        Assert(_lsMode == LSMODE_HITTEST || _lsMode == LSMODE_RENDERER);
        _pli = pli;
        _li._fLookaheadForGlyphing = (_pli ? _pli->_fLookaheadForGlyphing : FALSE);
    }
    
    ClearLinePropertyFlags(); // zero out all flags
    _fWrapLongLines = fWrapLongLines; // preserve this flag when we 0 _dwProps
    
    // We're getting max, so start really small.
    _lMaxLineHeight = LONG_MIN;
    _fFoundLineHeight = FALSE;

    _xWrappingWidth = -1;

    pPF = _treeInfo._pPF;
    _fInnerPFFirst = _treeInfo._fInnerPF;

    // BUGBUG (a-pauln) some elements are getting assigned a PF from the
    //        wrong branch in InitializeTreeInfo() above. This hack is a
    //        temporary correction of the problem's manifestation until
    //        we determine how to correct it.
    if(!_treeInfo._fHasNestedElement || !ptp)
        _li._fRTL = pPF->HasRTL(_fInnerPFFirst);
    else
    {
        pPF = ptp->Branch()->GetParaFormat();
        _li._fRTL = pPF->HasRTL(_fInnerPFFirst);
    }
    
    if (    !_pFlowLayout->GetDisplay()->GetWordWrap()
        ||  (pPF && pPF->HasPre(_fInnerPFFirst) && !_fWrapLongLines))
    {
        _xWrappingWidth = xWidthMaxAvail;
        xWidthMaxAvail = MAX_MEASURE_WIDTH;
    }
    else if (xWidthMaxAvail <= MIN_FOR_LS)
    {
        //BUGBUG(SujalP): Remove hack when LS gets their in-efficient calc bug fixed.

        xWidthMaxAvail = 0;
    }

    _xWidthMaxAvail = xWidthMaxAvail;
    if (_xWrappingWidth == -1)
        _xWrappingWidth = _xWidthMaxAvail;

    _fMinMaxPass = fMinMaxPass;
    _pPFFirst = pPF;
    DeleteChunks();
#ifndef QUILL
    InitChunkInfo(cp - (_pMeasurer->_fMeasureFromTheStart ? 0 : _pMeasurer->_cchPreChars));
#else
    if (!_pFlowLayout->FExternalLayout())
        InitChunkInfo(cp - (_pMeasurer->_fMeasureFromTheStart
                                ? 0 
                                : _pMeasurer->_cchPreChars));
    else
        InitChunkInfo(cp);
#endif  // QUILL

Cleanup:
    RRETURN(hr);
}

//-----------------------------------------------------------------------------
//
//  Function:   CLineServices::GetMinDurBreaks (member)
//
//  Synopsis:   Determine the minimum width of the line.  Also compute any
//              adjustments to the maximum width of the line.
//
//-----------------------------------------------------------------------------

LSERR
CLineServices::GetMinDurBreaks( 
    LONG * pdvMin,
    LONG * pdvMaxDelta )
{
    LSERR lserr;

    Assert(_plsline);
    Assert(_fMinMaxPass);
    Assert(pdvMin);
    Assert(pdvMaxDelta);

    //
    // First we call LsGetMinDurBreaks.  This call does the right thing only
    // for text runs, not ILS objects.
    //

    if (!_fScanForCR)
    {
        LONG  dvDummy;
        
        lserr = LsGetMinDurBreaks( GetContext(), _plsline, &dvDummy, pdvMin );
        if (lserr)
            goto Cleanup;
    }
    
    //
    // Now we need to go and compute the true max width.  The current max
    // width is incorrect by the difference in the min and max widths of
    // dobjs for which these values are not the same (e.g. tables).  We've
    // cached the difference in the dobj's, so we need to enumerate these
    // and add them up.  The enumeration callback adjusts the value in
    // CLineServices::dvMaxDelta;
    //

    _dvMaxDelta = 0;

    lserr = LsEnumLine( _plsline,
                        FALSE,        // fReverseOrder
                        FALSE,        // fGeometryNeeded
                        &g_Zero.pt ); // pptOrg

    *pdvMaxDelta = _dvMaxDelta;
    
    if (_fScanForCR)
    {
        *pdvMin = _li._xWidth + _dvMaxDelta;
    }

Cleanup:
    return lserr;
}

void COneRun::Deinit()
{
    if (_pCF)
    {
        if (_fMustDeletePcf)
        {
            delete _pCF;
        }
        _pCF = NULL;
    }

    _bConvertMode = CM_UNINITED;
    
    if (_pComplexRun)
    {
        delete _pComplexRun;
        _pComplexRun = NULL;
    }

    _cstrRunChars.Free();

    // Finally, clear out all the flags
    _dwProps = 0;
}


#ifdef FASTER_GAOR

//
// This is for the presently unused tweaked GetAvailableOneRuns
//

// Like ReSetup, but just scoots along from where it is.
void 
COneRun::RelativeSetup(CTreePosList *ptpl, 
                       CElement *pelRestricting, 
                       LONG cp, 
                       CFlowLayout *pRunOwner, 
                       BOOL fRight)
{
    Deinit();

    // The cp-threshhold number here is a performance-tuned number.  Any number will work.
    // (It will never do the wrong thing, it just might take a while to do it.)
    // It represents the largest cp-distance that can be covered with Advance that will
    // take the same amount of time as setting one up from scratch.
    if( ptpl != _ptpl || (abs(cp - Cp()) > 100) )
        super::ReSetUp(ptpl,pelRestricting,cp,pRunOwner,fRight);
    {
#if DBG == 1
        COneRun orDebug(this);
        orDebug.ReSetUp(ptpl,pelRestricting,cp,pRunOwner,fRight);
#endif // DBG == 1
        SetRestrictingElement(pelRestricting);
        Advance( cp - Cp() );
        Assert( !memcmp( this, &orDebug, sizeof(this) ) );
    }
}

#endif // FASTER_GAOR

//-----------------------------------------------------------------------------
//
//  Function:   CLineServices destructor
//
//  Synopsis:   Free the COneRun cache
//
//-----------------------------------------------------------------------------

CLineServices::~CLineServices()
{
    if (_pccsCache)
        _pccsCache->Release();

    if (_pccsAltCache)
        _pccsAltCache->Release();
}

//-----------------------------------------------------------------------------
//
//  Function:   CLineServices::DiscardLine (member)
//
//  Synopsis:   The lifetime of a CLineServices object is that of it's
//              containing CDoc.  This function is used to clear state
//              after each use of the object, as opposed to the destructor.
//              This function needs to be called after each measured/rendered
//              line (~CLine.)
//
//-----------------------------------------------------------------------------

void
CLineServices::DiscardLine()
{
    WHEN_DBG( static int nCall = 0 );
    
    TraceTag((tagLSTraceLines,
              "CLineServices::DiscardLine[%d] sn=%d null=%s ",
              nCall++, _nSerial, _plsline ? "true" : "false"));

    if (_plsline)
    {
        LsDestroyLine( _plsc, _plsline );
        _lineFlags.DeleteAll();
        _lineCounts.DeleteAll();
        _plsline = NULL;
    }

    // For now just do this simple thing here. Eventually we will do more
    // complex things like holding onto tree state.
    _treeInfo._fInited = FALSE;

    if (_pBidiLine != NULL)
    {
        delete _pBidiLine;
        _pBidiLine = NULL;
    }

    _aryRubyInfo.DeleteAll();
}


//+----------------------------------------------------------------------------
//
// Member:      InitChunkInfo
//
// Synopsis:    Initializest the chunk info store.
//
//-----------------------------------------------------------------------------
void
CLineServices::InitChunkInfo(LONG cp)
{
    _cpLastChunk = cp;
    _xPosLastChunk = 0;
    _plcFirstChunk = _plcLastChunk = NULL;
    _pElementLastRelative = NULL;
    _fLastChunkSingleSite = FALSE;
}

//+----------------------------------------------------------------------------
//
// Member:      DeleteChunks
//
// Synopsis:    Delete chunk related information in the line
//
//-----------------------------------------------------------------------------
void
CLineServices::DeleteChunks()
{
    while(_plcFirstChunk)
    {
        CLSLineChunk * plc = _plcFirstChunk;

        _plcFirstChunk = _plcFirstChunk->_plcNext;
        delete plc;
    }
    _plcLastChunk = NULL;
}

//-----------------------------------------------------------------------------
//
//  Function:   QueryLinePointPcp (member)
//
//  Synopsis:   Wrapper for LsQueryLinePointPcp
//
//  Returns:    S_OK    - Success
//              S_FALSE - depth was zero
//              E_FAIL  - error
//
//-----------------------------------------------------------------------------

HRESULT
CLineServices::QueryLinePointPcp(
    LONG u,                     // IN
    LONG v,                     // IN
    LSTFLOW *pktFlow,           // OUT
    PLSTEXTCELL plstextcell)    // OUT
{
    POINTUV uvPoint;
    CStackDataAry < LSQSUBINFO, 4 > aryLsqsubinfo( Mt(QueryLinePointPcp_aryLsqsubinfo_pv) );
    HRESULT hr;
    DWORD nDepthIn = 4;

    uvPoint.u = u;
    uvPoint.v = v;

    Assert(_plsline);

    #define NDEPTH_MAX 32

    for (;;)
    {
        DWORD nDepth;
        LSERR lserr = LsQueryLinePointPcp( _plsline,
                                           &uvPoint,
                                           nDepthIn,
                                           aryLsqsubinfo,
                                           &nDepth,
                                           plstextcell);

        if (lserr == lserrNone)
        {
            hr = S_OK;
            // get the flow direction for proper x+/- manipulation
            if(nDepth > 0)
            {
                if (   aryLsqsubinfo[nDepth - 1].idobj != LSOBJID_TEXT
                    && aryLsqsubinfo[nDepth - 1].idobj != LSOBJID_GLYPH
                    && aryLsqsubinfo[nDepth - 1].idobj != LSOBJID_EMBEDDED
                   )
                {
                    LSQSUBINFO &qsubinfo = aryLsqsubinfo[nDepth-1];
                    plstextcell->dupCell = qsubinfo.dupObj;
                    plstextcell->pointUvStartCell = qsubinfo.pointUvStartSubline;
                    plstextcell->cCharsInCell = 0;
                    plstextcell->cpStartCell = qsubinfo.cpFirstSubline;
                    plstextcell->cpEndCell = qsubinfo.cpFirstSubline + qsubinfo.dcpSubline;
                }
                else
                    plstextcell->cCharsInCell = plstextcell->cpEndCell - plstextcell->cpStartCell + 1;
                *pktFlow = aryLsqsubinfo[nDepth - 1].lstflowSubline;

            }
            else if (nDepth == 0)
            {
                // HACK ALERT(MikeJoch):
                // See hack alert by SujalP below. We can run into this case
                // when the line is terminated by a [section break] type
                // character. We should take it upon ourselves to fill in
                // plstextcell and pktFlow when this happens.
                LONG duIgnore;

                plstextcell->cpStartCell = _lscpLim - 1;
                plstextcell->cpEndCell = _lscpLim;
                plstextcell->dupCell = 0;
                plstextcell->cCharsInCell = 1;

                hr = THR(GetLineWidth( &plstextcell->pointUvStartCell.u, &duIgnore) );

                // If we don't have a level, assume that the flow is in the line direction.
                if (pktFlow)
                {
                    *pktFlow = _li._fRTL ? fUDirection : 0;
                }

            }
            else
            {
                hr = E_FAIL;
            }
            break;
        }
        else if (lserr == lserrInsufficientQueryDepth ) 
        {
            if (nDepthIn > NDEPTH_MAX)
            {
                hr = E_FAIL;
                break;
            }
            
            nDepthIn *= 2;
            Assert( nDepthIn <= NDEPTH_MAX );  // That would be rediculous

            hr = THR(aryLsqsubinfo.Grow(nDepthIn));
            if (hr)
                break;

            // Loop back.
        }
        else
        {
            hr = E_FAIL;
            break;
        }
    }

    RRETURN1(hr, S_FALSE);
}

//-----------------------------------------------------------------------------
//
//  Function:   QueryLineCpPpoint (member)
//
//  Synopsis:   Wrapper for LsQueryLineCpPpoint
//
//  Returns:    S_OK    - Success
//              S_FALSE - depth was zero
//              E_FAIL  - error
//
//-----------------------------------------------------------------------------

HRESULT
CLineServices::QueryLineCpPpoint(
    LSCP lscp,                              // IN
    BOOL fFromGetLineWidth,                 // IN
    CDataAry<LSQSUBINFO> * paryLsqsubinfo,  // IN/OUT
    PLSTEXTCELL plstextcell,                // OUT
    BOOL *pfRTLFlow)                        // OUT
{
    CStackDataAry < LSQSUBINFO, 4 > aryLsqsubinfo( Mt(QueryLineCpPpoint_aryLsqsubinfo_pv) );
    HRESULT hr;
    DWORD nDepthIn;
    LSTFLOW ktFlow;
    DWORD nDepth;

    Assert(_plsline);

    if (paryLsqsubinfo == NULL)
    {
        aryLsqsubinfo.Grow(4); // Guaranteed to succeed since we're working from the stack.
        paryLsqsubinfo = &aryLsqsubinfo;
    }
    nDepthIn = paryLsqsubinfo->Size();

    #define NDEPTH_MAX 32

    for (;;)
    {
        LSERR lserr = LsQueryLineCpPpoint( _plsline,
                                           lscp,
                                           nDepthIn,
                                           *paryLsqsubinfo,
                                           &nDepth,
                                           plstextcell);

        if (lserr == lserrNone)
        {
            // HACK ALERT(SujalP):
            // Consider the case where the line contains just 3 characters:
            // A[space][blockbreak] at cp 0, 1 and 2 respectively. If we query
            // LS at cp 2, if would expect lsTextCell to point to the zero
            // width cell containing [blockbreak], meaning that
            // lsTextCell.pointUvStartCell.u would be the width of the line
            // (including whitespaces). However, upon being queried at cp=2
            // LS returns a nDepth of ***0*** because it thinks this is some
            // splat crap. This problem breaks a lot of our callers, hence
            // we attemp to hack around this problem.
            // NOTE: In case LS fixes their problem, be sure that hittest.htm
            // renders all its text properly for it exhibits a problem because
            // of this problem.
            // ORIGINAL CODE: hr = nDepth ? S_OK : S_FALSE;
            if (nDepth == 0)
            {
                LONG duIgnore;
                
                if (!fFromGetLineWidth && lscp >= _lscpLim - 1)
                {
                    plstextcell->cpStartCell = _lscpLim - 1;
                    plstextcell->cpEndCell = _lscpLim;
                    plstextcell->dupCell = 0;

                    hr = THR(GetLineWidth( &plstextcell->pointUvStartCell.u, &duIgnore) );

                }
                else
                    hr = S_FALSE;

                // If we don't have a level, assume that the flow is in the line direction.
                if(pfRTLFlow)
                    *pfRTLFlow = _li._fRTL;

            }
            else
            {
                hr = S_OK;
                LSQSUBINFO &qsubinfo = (*paryLsqsubinfo)[nDepth-1];

                if (   qsubinfo.idobj != LSOBJID_TEXT
                    && qsubinfo.idobj != LSOBJID_GLYPH
                    && qsubinfo.idobj != LSOBJID_EMBEDDED
                   )
                {
                    plstextcell->dupCell = qsubinfo.dupObj;
                    plstextcell->pointUvStartCell = qsubinfo.pointUvStartObj;
                    plstextcell->cCharsInCell = 0;
                    plstextcell->cpStartCell = qsubinfo.cpFirstSubline;
                    plstextcell->cpEndCell = qsubinfo.cpFirstSubline + qsubinfo.dcpSubline;
                }

                // if we are going in the opposite direction of the line we
                // will need to compensate for proper xPos

                ktFlow = qsubinfo.lstflowSubline;
                if(pfRTLFlow)
                    *pfRTLFlow = (ktFlow == lstflowWS);
            }
            break;
        }
        else if (lserr == lserrInsufficientQueryDepth ) 
        {
            if (nDepthIn > NDEPTH_MAX)
            {
                hr = E_FAIL;
                break;
            }
            
            nDepthIn *= 2;
            Assert( nDepthIn <= NDEPTH_MAX );  // That would be ridiculous

            hr = THR(paryLsqsubinfo->Grow(nDepthIn));
            if (hr)
                break;

            // Loop back.
        }
        else
        {
            hr = E_FAIL;
            break;
        }
    }

    if (hr == S_OK)
    {
        Assert((LONG) nDepth <= paryLsqsubinfo->Size());
        paryLsqsubinfo->SetSize(nDepth);
    }

    RRETURN1(hr, S_FALSE);
}

HRESULT
CLineServices::GetLineWidth(LONG *pdurWithTrailing, LONG *pdurWithoutTrailing)
{
    LSERR lserr;
    LONG  duIgnore;
    
    lserr = LsQueryLineDup( _plsline, &duIgnore, &duIgnore, &duIgnore,
                            pdurWithoutTrailing, pdurWithTrailing);
    if (lserr != lserrNone)
        goto Cleanup;

#if DBG==1
    {
        DWORD dwFlags = _lineFlags.GetLineFlags(_cpLim);
        BOOL fHasLetterSpacing = (dwFlags & FLAG_HAS_LETTERSPACING) ? TRUE : FALSE;
        Assert(fHasLetterSpacing || (*pdurWithoutTrailing <= *pdurWithTrailing));
    }
#endif
    
Cleanup:
    return HRFromLSERR(lserr);
}

BOOL
CLineServices::IsOwnLineSite(COneRun *por)
{
    return por->_fCharsForNestedLayout ?
            por->_ptp->Branch()->Element()->IsOwnLineElement(_pFlowLayout):
            FALSE;
}

//+----------------------------------------------------------------------------
//
//  Function:   CLineServices::UnUnifyHan
//
//  Synopsis:   Pick of one the Far East script id's for an sidHan range.
//
//              Most ideographic characters fall in sidHan (~34k out of all
//              of ucs-2), but not all fonts support every Han character.
//              This function attempts to pick one of the four FE sids.
//
//  Returns:    sidKana     for Japanese
//              sidHangul   for Korean
//              sidBopomofo for Traditional Chinese
//              sidHan      for Simplified Chinese
//
//-----------------------------------------------------------------------------

SCRIPT_ID
CLineServices::UnUnifyHan(
    HDC hdc,
    UINT uiFamilyCodePage,
    LCID lcid,
    COneRun * por )
{
    SCRIPT_ID sid = sidTridentLim;

	if ( !lcid )
	{
		switch (uiFamilyCodePage)
		{
			case CP_CHN_GB:     sid = sidHan;       break;
			case CP_KOR_5601:   sid = sidHangul;    break;
			case CP_TWN:        sid = sidBopomofo;  break;
			case CP_JPN_SJ:     sid = sidKana;      break;
		}
	}
	else
	{
		LANGID lid = LANGIDFROMLCID(lcid);
		WORD plid = PRIMARYLANGID(lid);

		if (plid == LANG_CHINESE)
		{
			if (SUBLANGID(lid) == SUBLANG_CHINESE_TRADITIONAL)
			{
				sid = sidBopomofo;
			}
			else
			{
				sid = sidHan;
			}
		}
		else if (plid == LANG_KOREAN)
		{
			sid = sidHangul;
		}
		else if (plid == LANG_JAPANESE)
		{
			sid = sidKana;
		}
	}

    if (sid == sidTridentLim)
    {
        long cp = por->Cp();
        long cch = GetScriptTextBlock( por->_ptp, &cp );
        DWORD dwPriorityCodePages = fc().GetSupportedCodePageInfo(hdc) & (FS_JOHAB | FS_CHINESETRAD | FS_WANSUNG | FS_CHINESESIMP | FS_JISJAPAN);
        DWORD dwCodePages = GetTextCodePages( dwPriorityCodePages, cp, cch );

        dwCodePages &= dwPriorityCodePages;

        if (dwCodePages & FS_JISJAPAN)
        {
            sid = sidKana;
        }
        else if (!dwCodePages || dwCodePages & FS_CHINESETRAD)
        {
            sid = sidBopomofo;
        }
        else if (dwCodePages & FS_WANSUNG)
        {
            sid = sidHangul;
        }
        else
        {
            sid = sidHan;
        }
    }

    return sid;
}

//+----------------------------------------------------------------------------
//
//  Function:   CLineServices::DisambiguateScriptId
//
//  Synopsis:   Some characters have sporadic coverage in Far East fonts.  To
//              make matters worse, these characters often have good glyphs in
//              Latin fonts.  We call IMLangCodePages to check the coverage
//              of codepages for the codepoints of interest, and then pick
//              an appropriate script id.
//
//              The first priority is to not switch the font.  If it appears
//              that the prevailing font has coverage, we will preferentially
//              pick this font.  Otherwise, we go through a somewhat arbitrary
//              priority list to pick a better guess.
//
//  Returns:    TRUE if the default font should be used.
//
//              Script ID, iff the default font isn't used.
//
//-----------------------------------------------------------------------------

const DWORD s_adwCodePagesMap[] =
{
    FS_JOHAB,           // 0
    FS_CHINESETRAD,     // 1
    FS_WANSUNG,         // 2
    FS_CHINESESIMP,     // 3
    FS_JISJAPAN,        // 4
    FS_VIETNAMESE,      // 5
    FS_BALTIC,          // 6
    FS_ARABIC,          // 7
    FS_HEBREW,          // 8
    FS_TURKISH,         // 9
    FS_GREEK,           // 10
    FS_CYRILLIC,        // 11
    FS_LATIN2           // 12
};

const SCRIPT_ID s_asidCodePagesMap[] =
{
    sidHangul,          // 0  FS_JOHAB
    sidBopomofo,        // 1  FS_CHINESETRAD
    sidHangul,          // 2  FS_WANSUNG
    sidHan,             // 3  FS_CHINESESIMP
    sidKana,            // 4  FS_JISJAPAN
    sidLatin,           // 5  FS_VIETNAMESE
    sidLatin,           // 6  FS_BALTIC
    sidArabic,          // 7  FS_ARABIC
    sidHebrew,          // 8  FS_HEBREW
    sidLatin,           // 9  FS_TURKISH
    sidGreek,           // 10 FS_GREEK
    sidCyrillic,        // 11 FS_CYRILLIC
    sidLatin            // 12 FS_LATIN2
};

const BYTE s_abCodePagesMap[] =
{
    JOHAB_CHARSET,      // 0  FS_JOHAB
    CHINESEBIG5_CHARSET,// 1  FS_CHINESETRAD
    HANGEUL_CHARSET,    // 2  FS_WANSUNG
    GB2312_CHARSET,     // 3  FS_CHINESESIMP
    SHIFTJIS_CHARSET,   // 4  FS_JISJAPAN
    VIETNAMESE_CHARSET, // 5  FS_VIETNAMESE
    BALTIC_CHARSET,     // 6  FS_BALTIC
    ARABIC_CHARSET,     // 7  FS_ARABIC
    HEBREW_CHARSET,     // 8  FS_HEBREW
    TURKISH_CHARSET,    // 9  FS_TURKISH
    GREEK_CHARSET,      // 10 FS_GREEK
    RUSSIAN_CHARSET,    // 11 FS_CYRILLIC
    EASTEUROPE_CHARSET  // 12 FS_LATIN2
};

BOOL
CLineServices::DisambiguateScriptId(
    HDC hdc,                    // IN
    CBaseCcs *pBaseCcs,         // IN
    COneRun * por,              // IN
    SCRIPT_ID *psidAlt,         // OUT
    BYTE *pbCharSetAlt )        // OUT                                    
{
    long cp = por->Cp();
    long cch = GetScriptTextBlock( por->_ptp, &cp );
    DWORD dwCodePages = GetTextCodePages( 0, cp ,cch );
    BOOL fCheckAltFont;
    SCRIPT_ID sid = sidDefault;
    BYTE bCharSetCurrent = pBaseCcs->_bCharSet;
    BOOL fFECharSet;

    if (dwCodePages)
    {
        //
        // (1) Check if the current font will cover the glyphs
        //
        
        if (bCharSetCurrent == ANSI_CHARSET)
        {
            fCheckAltFont =   0 == (dwCodePages & FS_LATIN1)
                           || 0 == (pBaseCcs->_sids & ScriptBit(sidLatin));
            fFECharSet = FALSE;
        }
        else
        {
            int i = ARRAY_SIZE(s_adwCodePagesMap);

            fCheckAltFont = TRUE;
            fFECharSet = IsFECharset(bCharSetCurrent);

            while (i--)
            {
                if (bCharSetCurrent == s_abCodePagesMap[i])
                {
                    fCheckAltFont = 0 == (dwCodePages & s_adwCodePagesMap[i]);
                    break;
                }
            }
        }

        if (fCheckAltFont)
        {
            SCRIPT_IDS sidsFace = fc().EnsureScriptIDsForFont( hdc, pBaseCcs, FALSE );

            if (fFECharSet)
            {
                sidsFace &= (ScriptBit(sidKana)|ScriptBit(sidHangul)|ScriptBit(sidBopomofo)|ScriptBit(sidHan));
            }            

            //
            // (2) Check if the current fontface will cover the glyphs
            // (3) Pick an appropriate sid for the glyphs
            //

            if (dwCodePages & FS_LATIN1)
            {
                if (sidsFace & ScriptBit(sidLatin))
                {
                    sid = sidAmbiguous;
                    fCheckAltFont = 0 == (pBaseCcs->_sids & ScriptBit(sidLatin));
                }
                else
                {
                    sid = sidLatin;
                }

                *pbCharSetAlt = ANSI_CHARSET;
            }
            else
            {
                int i = ARRAY_SIZE(s_adwCodePagesMap);
                int iAlt = -1;

                dwCodePages &= fc().GetSupportedCodePageInfo(hdc);

                if (fFECharSet)
                {
                    // NOTE (cthrash) If are native font doesn't support it, don't switch to
                    // another FE charset because the font creation will simply fail and give
                    // us an undesirable font.

                    dwCodePages &= ~(FS_JOHAB | FS_CHINESETRAD | FS_WANSUNG | FS_CHINESESIMP | FS_JISJAPAN);
                }
                
                if (dwCodePages)
                {
                    while (i--)
                    {
                        if (dwCodePages & s_adwCodePagesMap[i])
                        {
                            const SCRIPT_IDS sids = ScriptBit(s_asidCodePagesMap[i]);

                            if (sidsFace & sids)
                            {
                                fCheckAltFont = 0 == (pBaseCcs->_sids & sids);
                                break;          // (2)
                            }
                            else if (-1 == iAlt)
                            {
                                iAlt = i;       // Candidate for (3), provided nothing meet requirement for (2)
                            }
                        }
                    }

                    if (-1 != i)
                    {
                        sid = sidAmbiguous;
                        *pbCharSetAlt = s_abCodePagesMap[i];
                    }
                    else if (-1 != iAlt)
                    {
                        sid = s_asidCodePagesMap[iAlt];
                        *pbCharSetAlt = s_abCodePagesMap[iAlt];
                    }
                    else
                    {
                        fCheckAltFont = FALSE;
                    }
                }
            }
        }
    }
    else
    {
        fCheckAltFont = FALSE;
    }

    *psidAlt = sid;
    
    return fCheckAltFont;
}

//+----------------------------------------------------------------------------
//
//  Function:   CLineServices::GetScriptTextBlock
//
//  Synopsis:   Get the cp and cch of the current text block, which may be
//              artificially chopped up due to the presence of pointer nodes.
//
//  Returns:    cch and cp.
//
//-----------------------------------------------------------------------------

#define FONTLINK_SCAN_MAX 32L

long
CLineServices::GetScriptTextBlock(
    CTreePos * ptp, // IN
    long * pcp )    // IN/OUT
{
    const long ich = *pcp - ptp->GetCp(); // chars to the left of the cp in the ptp
    const SCRIPT_ID sid = ptp->Sid();
    long cchBack;
    long cchForward;
    CTreePos * ptpStart = ptp;
    
    //
    // Find the 'true' start, backing up over pointers.
    // Look for the beginning of the sid block.
    //

    for (cchBack = ich; cchBack <= FONTLINK_SCAN_MAX;)
    {
        CTreePos * ptpPrev = ptpStart->PreviousTreePos();

        if (   !ptpPrev
            || ptpPrev->IsNode()
            || (  ptpPrev->IsText()
                && ptpPrev->Sid() != sid))
        {
            break;
        }

        ptpStart = ptpPrev;

        cchBack += ptpPrev->GetCch();
    }

    //
    // Find the 'true' end.
    //

    for (cchForward = ptp->GetCch() - ich; cchForward <= FONTLINK_SCAN_MAX;)
    {
        CTreePos * ptpNext = ptp->NextTreePos();

        if (   !ptpNext
            || ptpNext->IsNode()
            || (  ptpNext->IsText()
                && ptpNext->Sid() != sid))
        {
            break;
        }

        ptp = ptpNext;

        cchForward += ptp->GetCch();
    }

    //
    // Determine the cp at which to start, and the cch to scan.
    //

    if (cchBack <= FONTLINK_SCAN_MAX)
    {
        *pcp = ptpStart->GetCp();
    }
    else
    {
        *pcp = ptpStart->GetCp() + cchBack - FONTLINK_SCAN_MAX;
    }

    return min(cchBack, FONTLINK_SCAN_MAX) + min( cchForward, FONTLINK_SCAN_MAX );
}

//+----------------------------------------------------------------------------
//
//  Function:   CLineServices::GetTextCodePages
//
//  Synopsis:   Determine the codepage coverage of cch chars of text starting
//              at cp using IMLangCodePages.
//
//  Returns:    Codepage coverage as a bitfield (FS_*, see wingdi.h).
//
//-----------------------------------------------------------------------------

#define GETSTRCODEPAGES_CHUNK 10L

DWORD
CLineServices::GetTextCodePages(
    DWORD dwPriorityCodePages,
    long  cp,
    long  cch )
{
    HRESULT hr;
    extern IMultiLanguage *g_pMultiLanguage;
    IMLangCodePages * pMLangCodePages = NULL;
    DWORD dwCodePages;
    LONG cchCodePages;
    long cchValid;
    CTxtPtr tp(_pMarkup, cp);
    const TCHAR * pch = tp.GetPch(cchValid);
    long cchInTp = cchValid;
    DWORD dwMask;

    //
    // PERF (cthrash) In the following HTML, we will have a sidAmbiguous
    // run with a single space char in in (e.g. amazon.com): <B>&middot;</B> X
    // The middot is sidAmbiguous, the space is sidMerge, and thus they merge.
    // The X of course is sidAsciiLatin.  When called for a single space char,
    // we don't want to bother calling MLANG.  Just give the caller a stock
    // answer so as to avoid the unnecessary busy work.
    //    
   
    if (cch == 1 && *pch == WCH_SPACE)
        return dwPriorityCodePages ? dwPriorityCodePages : DWORD(-1);
    
    dwCodePages = dwPriorityCodePages;
    dwMask = DWORD(-1);

    hr = THR( EnsureMultiLanguage() );
    if (hr)
        goto Cleanup;

    hr = THR( g_pMultiLanguage->QueryInterface( IID_IMLangCodePages, (void**)&pMLangCodePages ) );
    if (hr)
        goto Cleanup;

    // Strip out the leading WCH_NBSP chars

    while (cch)
    {
        long cchToCheck = min( GETSTRCODEPAGES_CHUNK, min( cch, cchValid ));
        const TCHAR * pchStart = pch;
        const TCHAR * pchNext;
        DWORD dwRet = 0;

        while (*pch == WCH_NBSP)
        {
            pch++;
            cchToCheck--;
            if (!cchToCheck)
                break;
        }

        pchNext = pch + cchToCheck;

        if (cchToCheck)
        {
            hr = THR( pMLangCodePages->GetStrCodePages( pch, cchToCheck, dwCodePages, &dwRet, &cchCodePages ) );
            if (hr)
                goto Cleanup;

            dwCodePages = dwMask & dwRet;

            //
            // No match, not much we can do.
            //

            if (!dwCodePages)
                break;

            //
            // This is an error condition of sorts.  The whole run couldn't be
            // covered by a single codepage.  Just bail and give the user what
            // we have so far.
            //

            if (cchToCheck > cchCodePages)
            {
                if (pch[cchCodePages] == WCH_NBSP)
                {
                    pchNext = pch + cchCodePages + 1; // resume after NBSP char
                }
                else
                {
                    break;
                }
            }
        }

        //
        // Decrement our counts, advance our pointers.
        //

        cchValid -= (long)(pchNext - pchStart);
        cch -= (long)(pchNext - pchStart);

        if (cch)
        {
            DWORD dw = dwCodePages;
            int i, j;

            //
            // First check if we have only one codepage represented
            // If we do, we're done.  Note the largest bit we care
            // about is FS_JOHAB (0x00200000) so we look at 22 bits.
            //

            for (i=22, j=0;i-- && j<2; dw>>=1)
            {
                j += dw & 1;
            }

            if (j == 1)
                break;

            if (cchValid)
            {
                pch = pchNext;
            }
            else
            {
                tp.AdvanceCp(cchInTp);
                pch = tp.GetPch(cchInTp);
                cchValid = cchInTp;
            }

            dwMask = dwCodePages;
        }
    }

Cleanup:

    ReleaseInterface( pMLangCodePages );

    return dwCodePages;
}

//-----------------------------------------------------------------------------
//
//  Function:   CLineServices::ShouldSwitchFontsForPUA
//
//  Synopsis:   Decide if we should switch fonts for Unicode Private Use Area
//              characters.  This is a heuristic approach.
//
//  Returns:    BOOL    - true if we should switch fonts
//              *psid   - the SCRIPT_ID to which we should switch
//
//-----------------------------------------------------------------------------

BOOL
CLineServices::ShouldSwitchFontsForPUA(
    HDC hdc,
    UINT uiFamilyCodePage,
    CBaseCcs * pBaseCcs,
    const CCharFormat * pcf,
    SCRIPT_ID * psid )
{
    BOOL fShouldSwitch;
    
    if (pcf->_fExplicitFace)
    {
        // Author specified a face name -- don't switch fonts on them.
        
        fShouldSwitch = FALSE;
    }
    else
    {
        SCRIPT_IDS sidsFace = fc().EnsureScriptIDsForFont( hdc, pBaseCcs, FALSE );
        SCRIPT_ID sid = sidDefault;

        if (pcf->_lcid)
        {
            // Author specified a LANG attribute -- see if the current font
            // covers this script

            const LANGID langid = LANGIDFROMLCID(pcf->_lcid);

            if (langid == LANG_CHINESE)
            {
                sid = SUBLANGID(langid) == SUBLANG_CHINESE_TRADITIONAL
                      ? sidBopomofo
                      : sidHan;
            }
            else
            {
                sid = DefaultScriptIDFromLang(langid);
            }
        }
        else
        {
            // Check the document codepage, then the system codepage

            switch (uiFamilyCodePage)
            {
                case CP_CHN_GB:     sid = sidHan;       break;
                case CP_KOR_5601:   sid = sidHangul;    break;
                case CP_TWN:        sid = sidBopomofo;  break;
                case CP_JPN_SJ:     sid = sidKana;      break;
                default:
                {
                    switch (g_cpDefault)
                    {
                        case CP_CHN_GB:     sid = sidHan;       break;
                        case CP_KOR_5601:   sid = sidHangul;    break;
                        case CP_TWN:        sid = sidBopomofo;  break;
                        case CP_JPN_SJ:     sid = sidKana;      break;
                        default:            sid = sidDefault;   break;                        
                    }
                }
            }
        }

        if (sid != sidDefault)
        {
            fShouldSwitch = (sidsFace & ScriptBit(sid)) == sidsNotSet;
            *psid = sid;
        }
        else
        {
            fShouldSwitch = FALSE;
        }
    }

    return fShouldSwitch;            
}

//-----------------------------------------------------------------------------
//
//  Function:   CLineServices::GetCcs
//
//  Synopsis:   Gets the suitable font (CCcs) for the given COneRun.
//
//  Returns:    CCcs
//
//-----------------------------------------------------------------------------

CCcs *
CLineServices::GetCcs(COneRun *por, HDC hdc, CDocInfo *pdi)
{
    CCcs *pccs;
    const CCharFormat *pCF = por->GetCF();
    const BOOL fDontFontLink =    !por->_ptp
                               || !por->_ptp->IsText()
                               || _chPassword
                               || pCF->_bCharSet == SYMBOL_CHARSET
                               || pCF->_fDownloadedFont
                               || pdi->_pDoc->GetCodePage() == 50000;
    BOOL fCheckAltFont;   // TRUE if _pccsCache does not have glyphs needed for sidText
    BOOL fPickNewAltFont; // TRUE if _pccsAltCache needs to be created anew
    SCRIPT_ID sidAlt = 0;
    BYTE bCharSetAlt = 0;
    SCRIPT_ID sidText;

    //
    // NB (cthrash) Although generally it will, CCharFormat::_latmFaceName need
    // not necessarily match CBaseCcs::_latmLFFaceName.  This is particularly true
    // when a generic CSS font-family is used (Serif, Fantasy, etc.)  We won't
    // know the actual font properties until we call CCcs::MakeFont.  For this
    // reason, we always compute the _pccsCache first, even though it's
    // possible (not likely, but possible) that we'll never use the font.
    //

           // If we have a different pCF then what _pccsCache is based on,
    if (   pCF != _pCFCache
           // *or* If we dont have a cached _pccsCache
        || !_pccsCache
       )
    {
        pccs = fc().GetCcs(hdc, pdi, pCF);
        if (pccs)
        {
            if (CM_UNINITED != por->_bConvertMode)
            {
                pccs->GetBaseCcs()->_bConvertMode = por->_bConvertMode; // BUGBUG (cthrash) Is this right?
            }

            _pCFCache = (!por->_fMustDeletePcf ? pCF : NULL);

            if (_pccsCache)
            {
                _pccsCache->Release();
            }
            
            _pccsCache = pccs;
        }
        else
        {
            AssertSz(0, "CCcs failed to be created.");
            goto Cleanup;
        }
    }

    Assert(pCF == _pCFCache || por->_fMustDeletePcf);
    pccs = _pccsCache;
    
    if (fDontFontLink)
        goto Cleanup;

    //
    // Check if the _pccsCache covers the sid of the text run
    //

    Assert(por->_lsCharProps.fGlyphBased ||
           por->_sid == (!_pCFLi ? DWORD(por->_ptp->Sid()) : sidAsciiLatin));

    sidText = por->_sid;

    AssertSz( sidText != sidMerge, "Script IDs should have been merged." );

    {
        CBaseCcs * pBaseCcs = _pccsCache->GetBaseCcs();

        if (sidText < sidLim)
        {
            if (sidText == sidDefault)
            {
                fCheckAltFont = FALSE; // Assume the author picked a font containing the glyph.  Don't fontlink.
            }
            else
            {
                fCheckAltFont = (pBaseCcs->_sids & ScriptBit(sidText)) == sidsNotSet;
            }
        }
        else
        {
            Assert(   sidText == sidSurrogateA
                   || sidText == sidSurrogateB
                   || sidText == sidAmbiguous
                   || sidText == sidEUDC
                   || sidText == sidHalfWidthKana );

            if (sidText == sidAmbiguous)
            {
                fCheckAltFont = DisambiguateScriptId( hdc, pBaseCcs, por, &sidAlt, &bCharSetAlt );
            }
            else if (sidText == sidEUDC)
            {
                const UINT uiFamilyCodePage = pdi->_pDoc->GetFamilyCodePage();
                SCRIPT_ID sidForPUA;
                
                fCheckAltFont = ShouldSwitchFontsForPUA( hdc, uiFamilyCodePage, pBaseCcs, pCF, &sidForPUA );
                if (fCheckAltFont)
                {
                    sidText = sidAlt = sidAmbiguous; // to prevent call to UnUnifyHan
                    bCharSetAlt = DefaultCharSetFromScriptAndCodePage( sidForPUA, uiFamilyCodePage );
                }
            }
            else
            {
                fCheckAltFont = (pBaseCcs->_sids & ScriptBit(sidText)) == sidsNotSet;
            }
        }
    }

    if (!fCheckAltFont)
        goto Cleanup;

    //
    // Check to see if the _pccsAltCache covers the sid of the text run
    //
    
    {
        const UINT uiFamilyCodePage = pdi->_pDoc->GetFamilyCodePage();

        if (sidText == sidHan)
        {
            sidAlt = UnUnifyHan( hdc, uiFamilyCodePage, pCF->_lcid, por );
            bCharSetAlt = DefaultCharSetFromScriptAndCodePage( sidAlt, uiFamilyCodePage );
        }
        else if (sidText != sidAmbiguous)
        {
            SCRIPT_IDS sidsFace = fc().EnsureScriptIDsForFont( hdc, _pccsCache->GetBaseCcs(), FALSE );

            bCharSetAlt = DefaultCharSetFromScriptAndCodePage( sidText, uiFamilyCodePage );

            if ((sidsFace & ScriptBit(sidText)) == sidsNotSet)
            {
                sidAlt = sidText;           // current face does not cover
            }
            else
            {
                sidAlt = sidAmbiguous;      // current face does cover
            }
        }

        fPickNewAltFont =   !_pccsAltCache
                         || _pCFAltCache != pCF
                         || _pccsAltCache->GetBaseCcs()->_bCharSet != bCharSetAlt;
    }

    //
    // Looks like we need to pick a new alternate font
    //
    
    if (!fPickNewAltFont)
    {
        Assert(_pccsAltCache);
        pccs = _pccsAltCache;
    }
    else
    {
        CCharFormat cfAlt = *pCF;
        BOOL fNewlyFetchedFromRegistry;

        // sidAlt of sidAmbiguous at this point implies we have the right facename,
        // but the wrong GDI charset.  Otherwise, lookup in the registry/mlang to
        // determine an appropriate font for the given script id.

        if (sidAlt != sidAmbiguous)
        {
            fNewlyFetchedFromRegistry = SelectScriptAppropriateFont( sidAlt, bCharSetAlt, pdi->_pDoc, &cfAlt );
        }
        else
        {
            fNewlyFetchedFromRegistry = FALSE;
            cfAlt._bCharSet = bCharSetAlt;
            cfAlt._bCrcFont = cfAlt.ComputeFontCrc();
        }

        // NB (cthrash) sidAltText may differ from sidText in the case where
        // sidText is sidHan.  SelectScriptAppropriateFont makes a reasonable
        // effort to pick the optimal script amongst the unified Han scripts.

        pccs = fc().GetFontLinkCcs(hdc, pdi, pccs, &cfAlt);
        if (pccs)
        {
            // Remember the pCF from which the pccs was derived.
            _pCFAltCache = pCF;

            if (_pccsAltCache)
            {
                _pccsAltCache->Release();
            }

            _pccsAltCache = pccs;

            if (fNewlyFetchedFromRegistry)
            {
                // It's possible that the font signature for the given font would indicate
                // that it does not have the necessary glyph coverage for the sid in question.
                // Nevertheless, the user has specifically requested this face for the sid,
                // so we honor his/her choice and assume that the glyphs are covered.

                ForceScriptIdOnUserSpecifiedFont( pdi->_pDoc->_pOptionSettings, sidAlt );
            }

            pccs->GetBaseCcs()->_sids |= ScriptBit(sidAlt);
        }
    }

Cleanup:
    Assert(!pccs || pccs->GetHDC() == hdc);
    return pccs;
}

//-----------------------------------------------------------------------------
//
//  Function:   LineHasNoText
//
//  Synopsis:   Utility function which returns TRUE if there is no text (only nested
//              layout or antisynth'd goop) on the line till the specified cp.
//
//  Returns:    BOOL
//
//-----------------------------------------------------------------------------
BOOL
CLineServices::LineHasNoText(LONG cp)
{
    LONG fRet = TRUE;
    for (COneRun *por = _listCurrent._pHead; por; por = por->_pNext)
    {
        if (por->Cp() >= cp)
            break;

        if (!por->IsNormalRun())
            continue;
        
        if (por->_lsCharProps.idObj == LSOBJID_TEXT)
        {
            fRet = FALSE;
            break;
        }
    }
    return fRet;
}

//-----------------------------------------------------------------------------
//
//  Function:   CheckSetTables (member)
//
//  Synopsis:   Set appropriate tables for Line Services
//
//  Returns:    HRESULT
//
//-----------------------------------------------------------------------------

HRESULT
CLineServices::CheckSetTables()
{
    LSERR lserr;

    lserr = CheckSetBreaking();

    if (   lserr == lserrNone
        && _pPFFirst->_uTextJustify > styleTextJustifyInterWord)
    {
        lserr = CheckSetExpansion();

        if (lserr == lserrNone)
        {
            lserr = CheckSetCompression();
        }
    }

    RRETURN(HRFromLSERR(lserr));
}

//-----------------------------------------------------------------------------
//
//  Function:   HRFromLSERR (global)
//
//  Synopsis:   Utility function to converts a LineServices return value
//              (LSERR) into an appropriate HRESULT.
//
//  Returns:    HRESULT
//
//-----------------------------------------------------------------------------

HRESULT
HRFromLSERR( LSERR lserr )
{
    HRESULT hr;

#if DBG==1
    if (lserr != lserrNone)
    {
        char ach[64];

        StrCpyA( ach, "Line Services returned an error: " );
        StrCatA( ach, LSERRName(lserr) );

        AssertSz( FALSE, ach );
    }
#endif
    
    switch (lserr)
    {
        case lserrNone:         hr = S_OK;          break;
        case lserrOutOfMemory:  hr = E_OUTOFMEMORY; break;
        default:                hr = E_FAIL;        break;
    }

    return hr;
}

//-----------------------------------------------------------------------------
//
//  Function:   LSERRFromHR (global)
//
//  Synopsis:   Utility function to converts a HRESULT into an appropriate
//              LineServices return value (LSERR).
//
//  Returns:    LSERR
//
//-----------------------------------------------------------------------------

LSERR
LSERRFromHR( HRESULT hr )
{
    LSERR lserr;

    if (SUCCEEDED(hr))
    {
        lserr = lserrNone;
    }
    else
    {
        switch (hr)
        {
            default:
                AssertSz(FALSE, "Unmatched error code; returning lserrOutOfMemory");
            case E_OUTOFMEMORY:     lserr = lserrOutOfMemory;   break;
        }
    }

    return lserr;
}

#if DBG==1
//-----------------------------------------------------------------------------
//
//  Function:   LSERRName (global)
//
//  Synopsis:   Return lserr in a string form.
//
//-----------------------------------------------------------------------------

static const char * rgachLSERRName[] =
{
    "None",
    "InvalidParameter",//           (-1L)
    "OutOfMemory",//                (-2L)
    "NullOutputParameter",//        (-3L)
    "InvalidContext",//             (-4L)
    "InvalidLine",//                (-5L)
    "InvalidDnode",//               (-6L)
    "InvalidDeviceResolution",//    (-7L)
    "InvalidRun",//                 (-8L)
    "MismatchLineContext",//        (-9L)
    "ContextInUse",//               (-10L)
    "DuplicateSpecialCharacter",//  (-11L)
    "InvalidAutonumRun",//          (-12L)
    "FormattingFunctionDisabled",// (-13L)
    "UnfinishedDnode",//            (-14L)
    "InvalidDnodeType",//           (-15L)
    "InvalidPenDnode",//            (-16L)
    "InvalidNonPenDnode",//         (-17L)
    "InvalidBaselinePenDnode",//    (-18L)
    "InvalidFormatterResult",//     (-19L)
    "InvalidObjectIdFetched",//     (-20L)
    "InvalidDcpFetched",//          (-21L)
    "InvalidCpContentFetched",//    (-22L)
    "InvalidBookmarkType",//        (-23L)
    "SetDocDisabled",//             (-24L)
    "FiniFunctionDisabled",//       (-25L)
    "CurrentDnodeIsNotTab",//       (-26L)
    "PendingTabIsNotResolved",//    (-27L)
    "WrongFiniFunction",//          (-28L)
    "InvalidBreakingClass",//       (-29L)
    "BreakingTableNotSet",//        (-30L)
    "InvalidModWidthClass",//       (-31L)
    "ModWidthPairsNotSet",//        (-32L)
    "WrongTruncationPoint",//       (-33L)
    "WrongBreak",//                 (-34L)
    "DupInvalid",//                 (-35L)
    "RubyInvalidVersion",//         (-36L)
    "TatenakayokoInvalidVersion",// (-37L)
    "WarichuInvalidVersion",//      (-38L)
    "WarichuInvalidData",//         (-39L)
    "CreateSublineDisabled",//      (-40L)
    "CurrentSublineDoesNotExist",// (-41L)
    "CpOutsideSubline",//           (-42L)
    "HihInvalidVersion",//          (-43L)
    "InsufficientQueryDepth",//     (-44L)
    "InsufficientBreakRecBuffer",// (-45L)
    "InvalidBreakRecord",//         (-46L)
    "InvalidPap",//                 (-47L)
    "ContradictoryQueryInput",//    (-48L)
    "LineIsNotActive",//            (-49L)
    "Unknown"
};

const char *
LSERRName( LSERR lserr )
{
    int err = -(int)lserr;

    if (err < 0 || err >= ARRAY_SIZE(rgachLSERRName))
    {
        err = ARRAY_SIZE(rgachLSERRName) - 1;
    }

    return rgachLSERRName[err];
}

#endif // DBG==1

#if DBG == 1 || defined(DUMPTREE)
void
CLineServices::DumpTree()
{
    GetMarkup()->DumpTree();
}

void
CLineServices::DumpUnicodeInfo(TCHAR ch)
{
    CHAR_CLASS CharClassFromChSlow(WCHAR wch);
    char ab[180];
    UINT u;

    HANDLE hf = CreateFile(
                _T("c:\\x"),
                GENERIC_WRITE | GENERIC_READ,
                FILE_SHARE_WRITE | FILE_SHARE_READ,
                NULL,
                OPEN_ALWAYS,
                FILE_ATTRIBUTE_NORMAL,
                NULL);
    
    for (u=0;u<0x10000;u++)
    {
        ch = TCHAR(u);
        
        CHAR_CLASS cc = CharClassFromChSlow(ch);
        SCRIPT_ID sid = ScriptIDFromCharClass(cc);

        if (sid == sidDefault)
        {
            DWORD nbw;
            int i = wsprintfA(ab, "{ 0x%04X, %d },\r", ch, cc );
            WriteFile( hf, ab, i, &nbw, NULL);
        }
    }
}

#ifndef X_TXTDEFS_H_
#define X_TXTDEFS_H_
#include "txtdefs.h"
#endif

DWORD TestAmb()
{
    int i;

    for (i=0;i<65536;i++)
    {
        SCRIPT_ID sid = ScriptIDFromCh(WCHAR(i));

        if (sid == sidDefault)
        {
            char abBuf[32];
            
            wsprintfA(abBuf, "0x%04x\r\n", i);
            OutputDebugStringA(abBuf);
        }
    }

    return 0;
}
#endif
