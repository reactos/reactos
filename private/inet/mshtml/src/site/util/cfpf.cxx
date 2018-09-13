/*
 *  @doc    INTERNAL
 *
 *  @module CFPF.C -- -- RichEdit CCharFormat and CParaFormat Classes |
 *
 *  Created: <nl>
 *      9/1995 -- Murray Sargent <nl>
 *
 *  @devnote
 *      The this ptr for all methods points to an internal format class, i.e.,
 *      either a CCharFormat or a CParaFormat, which uses the cbSize field as
 *      a reference count.  The pCF or pPF argument points at an external
 *      CCharFormat or CParaFormat class, that is, pCF->cbSize and pPF->cbSize
 *      give the size of their structure.  The code still assumes that both
 *      internal and external forms are derived from the CHARFORMAT(2) and
 *      PARAFORMAT(2) API structures, so some redesign would be necessary to
 *      obtain a more space-efficient internal form.
 *
 *  Copyright (c) 1995-1996, Microsoft Corporation. All rights reserved.
 */

#include "headers.hxx"

#ifndef X_CFPF_HXX_
#define X_CFPF_HXX_
#include "cfpf.hxx"
#endif

#ifndef X_MSHTMLRC_H_
#define X_MSHTMLRC_H_
#include "mshtmlrc.h"
#endif

#ifndef X_DOCGLBS_HXX_
#define X_DOCGLBS_HXX_
#include "docglbs.hxx"
#endif

#ifndef X_ELEMENT_HXX_
#define X_ELEMENT_HXX_
#include "element.hxx"
#endif

#ifndef X_FCACHE_HXX_
#define X_FCACHE_HXX_
#include "fcache.hxx"
#endif

#ifndef X_EFONT_HXX_
#define X_EFONT_HXX_
#include "efont.hxx"
#endif

#ifndef X_TABLE_H_
#define X_TABLE_H_
#include "table.h"
#endif

#ifndef X_CAPTION_H_
#define X_CAPTION_H_
#include "caption.h"
#endif

#ifndef X_TXTSITE_HXX_
#define X_TXTSITE_HXX_
#include "txtsite.hxx"
#endif

#ifndef X_INTL_HXX_
#define X_INTL_HXX_
#include "intl.hxx"
#endif

#ifndef X__FONTLNK_H_
#define X__FONTLNK_H_
#include "_fontlnk.h"
#endif

#ifndef X_FPRINT_HXX_
#define X_FPRINT_HXX_
#include "fprint.hxx"
#endif

#ifndef X_TOMCONST_H_
#define X_TOMCONST_H_
#include "tomconst.h"
#endif

#ifndef X_PROPS_HXX_
#define X_PROPS_HXX_
#include "props.hxx"
#endif

#ifndef X_STYLE_HXX_
#define X_STYLE_HXX_
#include "style.hxx"
#endif

#ifndef X_SHEETS_HXX_
#define X_SHEETS_HXX_
#include "sheets.hxx"
#endif

#ifndef X_FONTFACE_HXX_
#define X_FONTFACE_HXX_
#include "fontface.hxx"
#endif

#ifndef X_TXTDEFS_H_
#define X_TXTDEFS_H_
#include "txtdefs.h"
#endif

#ifndef X_MSHTMHST_H_
#define X_MSHTMHST_H_
#include <mshtmhst.h>
#endif

#ifndef X_ALTFONT_H_
#define X_ALTFONT_H_
#include <altfont.h>
#endif

#ifndef X_INPUTTXT_HXX_
#define X_INPUTTXT_HXX_
#include "inputtxt.hxx"
#endif

DeclareTag(tagRecalcStyle, "Recalc Style", "Recalc Style trace")

extern DWORD g_dwPlatformID;
extern BOOL g_fInWin98Discover;

struct
{
    const TCHAR* szGenericFamily;
    DWORD        dwWindowsFamily;
}
const s_fontFamilyMap[] =
{
    { _T("sans-serif"), FF_SWISS },
    { _T("serif"),      FF_ROMAN },
    { _T("monospace"),  FF_MODERN },
    { _T("cursive"),    FF_SCRIPT },
    { _T("fantasy"),    FF_DECORATIVE }
};

/*
 *  CCharFormat::Compare(pCF)
 *
 *  @mfunc
 *      Compare this CCharFormat to *<p pCF>
 *
 *  @rdesc
 *      TRUE if they are the same
 *
 *  @devnote
 *      Compare simple types in memcmp.  If equal, compare complex ones
 */
BOOL CCharFormat::Compare (const CCharFormat *pCF) const
{
    BOOL fRet;

    Assert( _bCrcFont      == ComputeFontCrc() );
    Assert( pCF->_bCrcFont == pCF->ComputeFontCrc() );

    fRet = memcmp(this, pCF, offsetof(CCharFormat, _bCrcFont));

    // If the return value is TRUE then the CRC's should be the same.
    // That is, either the return value is FALSE, or the CRC's are the same.
    Assert( (!!fRet) || (_bCrcFont == pCF->_bCrcFont) );

    return (!fRet);
}


/*
 *  CCharFormat::CompareForLayout(pCF)
 *
 *  @mfunc
 *      Compare this CCharFormat to *<p pCF> and return FALSE if any
 *      attribute generally requiring a re-layout is different.
 *
 *  @rdesc
 *      TRUE if charformats are closed enough to not require relayout
 */
BOOL CCharFormat::CompareForLayout (const CCharFormat *pCF) const
{
    BYTE * pb1, * pb2;

    Assert( _bCrcFont      == ComputeFontCrc() );
    Assert( pCF->_bCrcFont == pCF->ComputeFontCrc() );

    if (_fNoBreak          != pCF->_fNoBreak          ||
        _fNoBreakInner     != pCF->_fNoBreakInner     ||
        _fVisibilityHidden != pCF->_fVisibilityHidden ||
        _fRelative         != pCF->_fRelative)
        return FALSE;

    pb1 = (BYTE*) &((CCharFormat*)this)->_wFontSpecificFlags();
    pb2 = (BYTE*) &((CCharFormat*)pCF)->_wFontSpecificFlags();

    if(memcmp(pb1, pb2, offsetof(CCharFormat, _ccvTextColor) -
                        offsetof(CCharFormat,
                                 _wFontSpecificFlags())))
        return FALSE;

    if( _latmFaceName != pCF->_latmFaceName )
        return FALSE;

    return TRUE;
}


/*
 *  CCharFormat::CompareForLikeFormat(pCF)
 *
 *  @mfunc
 *      Compare this CCharFormat to *<p pCF> and return FALSE if any
 *      attribute generally requiring a different Cccs is found.
 *
 *  @rdesc
 *      TRUE if charformats are close enough to not require a different Cccs.
 *      This is generally used in scripts like Arabic that have connecting
 *      characters
 */
BOOL CCharFormat::CompareForLikeFormat(const CCharFormat *pCF) const
{
    Assert(pCF != NULL);
    Assert(_bCrcFont == ComputeFontCrc());
    Assert(pCF->_bCrcFont == pCF->ComputeFontCrc());

    // We can actually just use the CRC; this is used in the font cache to
    // avoid caching redundant fonts.
    return (_bCrcFont == pCF->_bCrcFont);
}


/*
 *  CCharFormat::InitDefault(hfont)
 *
 *  @mfunc
 *      Returns the font family name
 *
 *
 *  @rdesc
 *      HRESULT = (if success) ? string : NULL
 */

const TCHAR *  CCharFormat::GetFamilyName() const
{
    int     n;

    for( n = 0; n < ARRAY_SIZE( s_fontFamilyMap ); ++n )
    {
        if(_bPitchAndFamily == s_fontFamilyMap[ n ].dwWindowsFamily)
            return s_fontFamilyMap[ n ].szGenericFamily;
    }

    return NULL;
}


/*
 *  CCharFormat::InitDefault(hfont)
 *
 *  @mfunc
 *      Initialize this CCharFormat with information coming from the font
 *      <p hfont>
 *
 *  @rdesc
 *      HRESULT = (if success) ? NOERROR : E_FAIL
 */
HRESULT CCharFormat::InitDefault (
    HFONT hfont)        //@parm Handle to font info to use
{
    LONG twips;
    LOGFONT lf;

    memset((LPBYTE)this, 0, sizeof(CCharFormat));

    // 0 enum value means normal
    _cuvLetterSpacing.SetRawValue(MAKEUNITVALUE(0,UNIT_ENUM));

    // If hfont isn't defined, get LOGFONT for default font
    if (!hfont)
        hfont = (HFONT)GetStockObject(SYSTEM_FONT);

    // Get LOGFONT for passed hfont
    if (!GetObject(hfont, sizeof(LOGFONT), &lf))
        return E_FAIL;

    /* COMPATIBILITY ISSUE:
     * RichEdit 1.0 selects hfont into a screen DC, gets the TEXTMETRIC,
     * and uses tm.tmHeight - tm.tmInternalLeading instead of lf.lfHeight
     * in the following. The following is simpler and since we have broken
     * backward compatibility on line/page breaks, I've left it (murrays).
     */

    // BUGBUG (cthrash) g_sizePixelsPerInch is only valid for the screen.

    twips = MulDivQuick( lf.lfHeight, TWIPS_PER_INCH, g_sizePixelsPerInch.cy );

    if(twips < 0)
        twips = - twips;

    SetHeightInTwips( twips );

    _fBold = lf.lfWeight >= FW_BOLD;
    _fItalic = lf.lfItalic;
    _fUnderline = lf.lfUnderline;
    _fStrikeOut = lf.lfStrikeOut;

    _wWeight = (WORD)lf.lfWeight;

    _bCharSet = lf.lfCharSet;
    _fNarrow = IsNarrowCharSet(lf.lfCharSet);
    _bPitchAndFamily = lf.lfPitchAndFamily;

    SetFaceName(lf.lfFaceName);
    return NOERROR;
}

TCHAR g_achFaceName[LF_FACESIZE];
LONG g_latmFaceName = 0;

/*
 *  CCharFormat::InitDefault( OPTIONSETTINGS *pOS, BOOL fKeepFaceIntact )
 *
 *  @mfunc
 *      Initialize this CCharFormat with given typeface and size
 *
 *  @rdesc
 *      HRESULT = (if success) ? NOERROR : E_FAIL
 */
HRESULT
CCharFormat::InitDefault (
    OPTIONSETTINGS * pOS,
    CODEPAGESETTINGS * pCS,
    BOOL fKeepFaceIntact )
{
    if (fKeepFaceIntact)
    {
        LONG latmOldFaceName= _latmFaceName;
        BYTE bOldCharSet = _bCharSet;
        BYTE bPitchAndFamily = _bPitchAndFamily;
        BOOL fNarrow = _fNarrow;
        BOOL fExplicitFace = _fExplicitFace;

        // Zero out structure

        memset((LPBYTE)this, 0, sizeof(CCharFormat));

        // restore cached values
        
        _latmFaceName= latmOldFaceName;
        _bCharSet = bOldCharSet;
        _bPitchAndFamily = bPitchAndFamily;
        _fNarrow = fNarrow;
        _fExplicitFace = fExplicitFace;
    }
    else
    {
        // Zero out structure

        memset((LPBYTE)this, 0, sizeof(CCharFormat));

        if (pCS)
        {
            _latmFaceName = pCS->latmPropFontFace;
            _bCharSet = pCS->bCharSet;
            _fNarrow = IsNarrowCharSet(_bCharSet);
        }
        else
        {
            if (g_latmFaceName == 0)
            {
                Verify(LoadString(GetResourceHInst(), IDS_HTMLDEFAULTFONT, g_achFaceName, LF_FACESIZE));
                g_latmFaceName = fc().GetAtomFromFaceName(g_achFaceName);
            }

            _latmFaceName = g_latmFaceName;
            _bCharSet = DEFAULT_CHARSET;
        }

        // These are values are all zero.
        // _bPitchAndFamily = DEFAULT_PITCH;
        // _fExplicitFace = FALSE;
    }

    _cuvLetterSpacing.SetRawValue(MAKEUNITVALUE(0,UNIT_ENUM));

    SetHeightInTwips( ConvertHtmlSizeToTwips(3) );

    _ccvTextColor.SetValue( pOS ? pOS->crText()
                                : GetSysColorQuick(COLOR_BTNTEXT),
                           FALSE);

    _wWeight = 400;

    return NOERROR;
}

/*
 *  CCharFormat::ComputeCrc()
 *
 *  @mfunc
 *      For fast font cache lookup, calculate CRC.
 *
 *  @devnote
 *      The font cache stores anything that has to
 *      do with measurement of metrics. Any font attribute
 *      that does not affect this should NOT be counted
 *      in the CRC; things like underline and color are not counted.
 */

WORD
CCharFormat::ComputeCrc() const
{
    BYTE    bCrc = 0;
    BYTE*   pb;
    BYTE *  pend = (BYTE*)&_bCrcFont;

    for (pb = (BYTE*)&((CCharFormat*)this)->_wFlags(); pb < pend; pb++)
        bCrc ^= *pb;

    return (WORD)bCrc;
}


//+----------------------------------------------------------------------------
// CCharFormat::ComuteFontCrc
//
// Synopsis:    Compute the font specific crc, assumes all the members that
//              effect the font are grouped together and are between
//              _wFontSpecificFlags and _bCursorIdx.
//              It is important the we do not have non specific
//              members in between to avoid creating fonts unnecessarily.
//
//-----------------------------------------------------------------------------

BYTE
CCharFormat::ComputeFontCrc() const
{
    BYTE bCrc, * pb, * pbend;

    Assert( & ((CCharFormat*)this)->_wFlags() < & ((CCharFormat*)this)->_wFontSpecificFlags() );

    bCrc = 0;

    pb    = (BYTE*) & ((CCharFormat*)this)->_wFontSpecificFlags();
    pbend = (BYTE*) & _bCursorIdx;

    for ( ; pb < pbend ; pb++ )
        bCrc ^= *pb;

    return bCrc;
}

void
CCharFormat::SetHeightInTwips(LONG twips)
{
    _fSizeDontScale = FALSE;
    _yHeight = twips;
}

void
CCharFormat::SetHeightInNonscalingTwips(LONG twips)
{
    _fSizeDontScale = TRUE;
    _yHeight = twips;
}

void
CCharFormat::ChangeHeightRelative(int diff)
{
    // This is a crude approximation.

    _yHeight = ConvertHtmlSizeToTwips( ConvertTwipsToHtmlSize( _yHeight ) + diff );
}

static TwipsFromHtmlSize[9][7] =
{
// scale fonts up for TV
#ifndef NTSC
  { 120, 160, 180, 200, 240, 320, 480 },
#endif
  { 140, 180, 200, 240, 320, 400, 600 },
  { 151, 200, 240, 271, 360, 480, 720 },
  { 200, 240, 280, 320, 400, 560, 840 },
  { 240, 280, 320, 360, 480, 640, 960 }
// scale fonts up for TV
#ifdef NTSC
  ,
  { 280, 320, 360, 400, 560, 720, 1080 }
#endif
  ,
  { 320, 360, 400, 440, 600, 760, 1120 }
  ,
  { 360, 400, 440, 480, 640, 800, 1160 }
  ,
  { 400, 440, 480, 520, 680, 840, 1200 }
  ,
  { 440, 480, 520, 560, 720, 880, 1240 }
};

static
LONG ScaleTwips( LONG iTwips, LONG iBaseLine, LONG iBumpDown )
{
    // iTwips:    the initial (unscaled) size in twips
    // iBaseLine: the baseline to use to scale (should be between 0-4)
    // iBumpDown: how many units to bump down (should be betweem 0-2)

    // If we are in the default baseline font and do not want to bump down, we do not
    //  need to scale.
    if( iBaseLine == BASELINEFONTDEFAULT && !iBumpDown )
        return iTwips;

    LONG lHtmlSize = ConvertTwipsToHtmlSize( iTwips );

    if( lHtmlSize > 7 || lHtmlSize <= iBumpDown ||
            TwipsFromHtmlSize[2][lHtmlSize - 1] != iTwips )
        // If we are out of range or do not match a table entry, scale manually
        return MulDivQuick( iTwips, iBaseLine + 4 - iBumpDown, 6 );
    else
    {
        // Scale according to IE table above.
        // the ratio is roughly as follows (from the IE table):
        //
        //   smallest small medium large largest
        //       1     5/4   6/4    7/4    8/4
        //
        // so if we scale that to medium, we have
        //
        //   smallest small medium large largest
        //      4/6    5/6     1    7/6    8/6

        lHtmlSize = max( 1L, lHtmlSize - iBumpDown );
        return TwipsFromHtmlSize[iBaseLine][lHtmlSize-1];
    }
}

LONG
CCharFormat::GetHeightInTwips( CDoc * pDoc) const
{
    LONG twips =0;
    LONG iBumpDown = 0;
    LONG iScaleFactor = 1;

    if (!pDoc)
        return twips;

    // If we want super or subscript, we want to bump the font size down one notch
    if (_fSubSuperSized)
        ++iBumpDown;

    if (g_fHighContrastMode)
    {
        iScaleFactor = 2;
    }

    if (_fSizeDontScale)
    {
        // For intrinsics, we don't want to change the pitch size
        // regardless of the z1oom factor (lBaseline).  We may want
        // to bump down if we get super or subscript, however.
        twips = ScaleTwips( _yHeight, BASELINEFONTDEFAULT*iScaleFactor, iBumpDown );
    }
    else
    {
        SHORT sBaselineFont = pDoc->GetBaselineFont();

        Assert( sBaselineFont >= 0 && sBaselineFont <= 4 );

        if( _fBumpSizeDown )
            ++iBumpDown;

        // If CDoc is a HTML dialog, use default font size.
        if (pDoc->_dwFlagsHostInfo & DOCHOSTUIFLAG_DIALOG)
        {
            twips = ScaleTwips( _yHeight, BASELINEFONTDEFAULT*iScaleFactor, iBumpDown );
        }
        else
        {
            twips = ScaleTwips( _yHeight, sBaselineFont*iScaleFactor, iBumpDown );
        }
    }

    Assert(twips >=0 && "twips height is negative");
    return twips;
}

LONG
CCharFormat::GetHeightInPixels(CDocInfo * pdci)
{
    // [twips] * [pix/inch]
    // -------------------- * zoom factor = zoomed pix
    //     [twips/inch]

    // Note that we've divided both 2540 (0.01 mm/inch) and
    // 1440 (twips/inch) by twenty to avoid integer overflow.

#ifdef UNIX
    // On UNIX, if we do as NT does, we get a swing font height.
    // BUGBUG: UNIX calculation does not take zoom into account
    HDC hdc = pdci->_pDoc->GetHDC();

    if ( hdc != NULL )
    {
        long lPixels = GetDeviceCaps(hdc, LOGPIXELSY);
        int nMultiplicand = GetHeightInTwips(pdci->_pDoc);
        double dblRes = (double)nMultiplicand * (double)lPixels / 1440.0;
        return (int)dblRes;
    }
#endif

#ifdef  IE5_ZOOM

    long dyt = pdci->DytFromTwp(GetHeightInTwips(pdci->_pDoc));
    long lRetValueNew = pdci->DyzFromDyt(dyt);

#if DBG==1
    long lRetValueOld = MulDivQuick( GetHeightInTwips(pdci->_pDoc),
                        2540/20 * pdci->_sizeDst.cy,
                        pdci->_sizeSrc.cy * 1440/20);
    Assert(IsWithinN(lRetValueNew, lRetValueOld, 2) || pdci->IsZoomed());
#endif  // DBG==1

    return lRetValueNew;

#else   // !IE5_ZOOM

    return MulDivQuick( GetHeightInTwips(pdci->_pDoc),
                        2540/20 * pdci->_sizeDst.cy,
                        pdci->_sizeSrc.cy * 1440/20);

#endif  // IE5_ZOOM
}

//------------------------- CParaFormat Class -----------------------------------

/*
 *  CParaFormat::AddTab(tbPos, tbAln, tbLdr)
 *
 *  @mfunc
 *      Add tabstop at position <p tbPos>, alignment type <p tbAln>, and
 *      leader style <p tbLdr>
 *
 *  @rdesc
 *      (success) ? NOERROR : S_FALSE
 *
 *  @devnote
 *      Tab struct that overlays LONG in internal rgxTabs is
 *
 *          DWORD   tabPos : 24;
 *          DWORD   tabType : 4;
 *          DWORD   tabLeader : 4;
 */
HRESULT CParaFormat::AddTab (
    LONG    tbPos,      //@parm New tab position
    LONG    tbAln,      //@parm New tab alignment type
    LONG    tbLdr)      //@parm New tab leader style
{
    LONG    Count   = _cTabCount;
    LONG    iTab;
    LONG    tbPosCurrent;

    if ((DWORD)tbAln > tomAlignBar ||               // Validate arguments
        (DWORD)tbLdr > tomLines ||                  // Comparing DWORDs causes
        (DWORD)tbPos > 0xffffff || !tbPos)          //  negative values to be
    {                                               //  treated as invalid
        return E_INVALIDARG;
    }

    LONG tbValue = tbPos + (tbAln << 24) + (tbLdr << 28);

    for(iTab = 0; iTab < Count &&                   // Determine where to insert
        tbPos > GetTabPos(_rgxTabs[iTab]);           //  insert new tabstop
        iTab++) ;

    if(iTab < MAX_TAB_STOPS)
    {
        tbPosCurrent = GetTabPos(_rgxTabs[iTab]);
        if(iTab == Count || tbPosCurrent != tbPos)
        {
            MoveMemory(&_rgxTabs[iTab + 1],          // Shift array up
                &_rgxTabs[iTab],                     //  (unless iTab = Count)
                (Count - iTab)*sizeof(LONG));

            if(Count < MAX_TAB_STOPS)               // If there's room,
            {
                _rgxTabs[iTab] = tbValue;            //  store new tab stop,
                _cTabCount++;                        //  increment tab count,
                return NOERROR;                     //  signal no error
            }
        }
        else if(tbPos == tbPosCurrent)              // Update tab since leader
        {                                           //  style or alignment may
            _rgxTabs[iTab] = tbValue;                //  have changed
            return NOERROR;
        }
    }
    return S_FALSE;
}

/*
 *  CParaFormat::Compare(pPF)
 *
 *  @mfunc
 *      Compare this CParaFormat to *<p pPF>
 *
 *  @rdesc
 *      TRUE if they are the same
 *
 *  @devnote
 *      First compare all of CParaFormat except rgxTabs
 *      If they are identical, compare the _cTabCount elemets of rgxTabs.
 *      If still identical, compare _cstrBkUrl
 *      Return TRUE only if all comparisons succeed.
 */
BOOL CParaFormat::Compare (const CParaFormat *pPF) const
{
    BOOL fRet;
    Assert(pPF);

    fRet = memcmp(this, pPF, offsetof(CParaFormat, _rgxTabs));
    if (!fRet)
    {
        fRet = memcmp(&_rgxTabs, &pPF->_rgxTabs, _cTabCount*sizeof(LONG));
    }
    return (!fRet);
}

/*
 *  CParaFormat::DeleteTab(tbPos)
 *
 *  @mfunc
 *      Delete tabstop at position <p tbPos>
 *
 *  @rdesc
 *      (success) ? NOERROR : S_FALSE
 */
HRESULT CParaFormat::DeleteTab (
    LONG     tbPos)         //@parm Tab position to delete
{
    LONG    Count   = _cTabCount;
    LONG    iTab;

    if(tbPos <= 0)
        return E_INVALIDARG;

    for(iTab = 0; iTab < Count; iTab++)         // Find tabstop for position
    {
        if (GetTabPos(_rgxTabs[iTab]) == tbPos)
        {
            MoveMemory(&_rgxTabs[iTab],          // Shift array down
                &_rgxTabs[iTab + 1],             //  (unless iTab is last tab)
                (Count - iTab - 1)*sizeof(LONG));
            _cTabCount--;                        // Decrement tab count and
            return NOERROR;                     //  signal no error
        }
    }
    return S_FALSE;
}

/*
 *  CParaFormat::GetTab (iTab, ptbPos, ptbAln, ptbLdr)
 *
 *  @mfunc
 *      Get tab parameters for the <p iTab> th tab, that is, set *<p ptbPos>,
 *      *<p ptbAln>, and *<p ptbLdr> equal to the <p iTab> th tab's
 *      displacement, alignment type, and leader style, respectively.  The
 *      displacement is given in twips.
 *
 *  @rdesc
 *      HRESULT = (no <p iTab> tab) ? E_INVALIDARG : NOERROR
 */
HRESULT CParaFormat::GetTab (
    long    iTab,           //@parm Index of tab to retrieve info for
    long *  ptbPos,         //@parm Out parm to receive tab displacement
    long *  ptbAln,         //@parm Out parm to receive tab alignment type
    long *  ptbLdr) const   //@parm Out parm to receive tab leader style
{
    AssertSz(ptbPos && ptbAln && ptbLdr,
        "CParaFormat::GetTab: illegal arguments");

    if(iTab < 0)                                    // Get tab previous to, at,
    {                                               //  or subsequent to the
        if(iTab < tomTabBack)                       //  position *ptbPos
            return E_INVALIDARG;

        LONG i;
        LONG tbPos = *ptbPos;
        LONG tbPosi;

        *ptbPos = 0;                                // Default tab not found
        for(i = 0; i < _cTabCount &&                 // Find *ptbPos
            tbPos > GetTabPos(_rgxTabs[i]);
            i++) ;

        tbPosi = GetTabPos(_rgxTabs[i]);             // tbPos <= tbPosi
        if(iTab == tomTabBack)                      // Get tab info for tab
            i--;                                    //  previous to tbPos
        else if(iTab == tomTabNext)                 // Get tab info for tab
        {                                           //  following tbPos
            if(tbPos == tbPosi)
                i++;
        }
        else if(tbPos != tbPosi)                    // tomTabHere
            return S_FALSE;

        iTab = i;
    }
    if((DWORD)iTab >= (DWORD)_cTabCount)             // DWORD cast also
        return E_INVALIDARG;                        //  catches values < 0

    iTab = _rgxTabs[iTab];
    *ptbPos = iTab & 0xffffff;
    *ptbAln = (iTab >> 24) & 0xf;
    *ptbLdr = iTab >> 28;
    return NOERROR;
}

/*
 *  CParaFormat::InitDefault()
 *
 *  @mfunc
 *      Initialize this CParaFormat with default paragraph formatting
 *
 *  @rdesc
 *      HRESULT = (if success) ? NOERROR : E_FAIL
 */
HRESULT CParaFormat::InitDefault()
{
    memset((LPBYTE)this, 0, sizeof(CParaFormat));

    // BUGBUG Arye: Is this left over from when we used masking?
    // Why do we default to all of these things set to on?

    _fTabStops = TRUE;

    _bBlockAlign   = htmlBlockAlignNotSet;

#if lDefaultTab <= 0
#error "default tab (lDefaultTab) must be > 0"
#endif

    _cTabCount = 1;
    _rgxTabs[0] = lDefaultTab;

    // Note that we don't use the inline method here because we want to
    // allow anyone to override.
    _cuvLeftIndentPoints.SetValue(0, CUnitValue::UNIT_POINT);
    _cuvRightIndentPoints.SetValue(0, CUnitValue::UNIT_POINT);
    _cuvNonBulletIndentPoints.SetValue(0, CUnitValue::UNIT_POINT);
    _cuvOffsetPoints.SetValue(0, CUnitValue::UNIT_POINT);

    return NOERROR;
}


/*
 *  CParaFormat::ComputeCrc()
 *
 *  @mfunc
 *      For fast font cache lookup, calculate CRC.
 *
 *  @devnote
 *      Compute items that deal with measurement of the element.
 *      Items which are purely stylistic should not be counted.
 */
WORD
CParaFormat::ComputeCrc() const
{
    DWORD dwCrc = 0, *pdw;

    for (pdw = (DWORD*)this; pdw < (DWORD*)(this+1); pdw++)
        dwCrc ^= *pdw;

    return HIWORD(dwCrc)^LOWORD(dwCrc);
}

// Font height conversion data.  Valid HTML font sizes ares [1..7]
// NB (cthrash) These are in twips, and are in the 'smallest' font
// size.  The scaling takes place in CFontCache::GetCcs().

// BUGBUG (cthrash) We will need to get these values from the registry
// when we internationalize this product, so as to get sizing appropriate
// for the target locale.

//BUGBUG johnv: Where did these old numbers come from?  The new ones now correspond to
//TwipsFromHtmlSize[2] defined above.
//static const int aiSizesInTwips[7] = { 100, 130, 160, 200, 240, 320, 480 };

// scale fonts up for TV
#ifdef NTSC
static const int aiSizesInTwips[7] = { 200, 240, 280, 320, 400, 560, 840 };
#elif defined(UNIX) // Default font size could be 13 (13*20=260)
static const int aiSizesInTwips[7] = { 151, 200, 260, 271, 360, 480, 720 };
#else
static const int aiSizesInTwips[7] = { 151, 200, 240, 271, 360, 480, 720 };
#endif

int
ConvertHtmlSizeToTwips(int nHtmlSize)
{
    // If the size is out of our conversion range do correction
    // Valid HTML font sizes ares [1..7]
    nHtmlSize = max( 1, min( 7, nHtmlSize ) );

    return aiSizesInTwips[ nHtmlSize - 1 ];
}

int
ConvertTwipsToHtmlSize(int nFontSize)
{
    int nNumElem = ARRAY_SIZE(aiSizesInTwips);

    // Now convert the point size to size units used by HTML
    // Valid HTML font sizes ares [1..7]
    int i;
    for(i = 0; i < nNumElem; i++)
    {
        if(nFontSize <= aiSizesInTwips[i])
            break;
    }

    return i + 1;
}


//+------------------------------------------------------------------------
//
//  CFancyFormat::CFancyFormat
//
//-------------------------------------------------------------------------
CFancyFormat::CFancyFormat()
{
    _pszFilters = NULL;
    _iExpandos = -1;
}

CFancyFormat::CFancyFormat(const CFancyFormat &ff)
{
    _iExpandos = -1;
    *this = ff;
}

void CFancyFormat::InitDefault ( void )
{
    int i, iOrigiExpandos;

    if ( _pszFilters )
        MemFree ( _pszFilters );

    // We want expandos to be inherited
    iOrigiExpandos = _iExpandos;

    memset((LPBYTE)this, 0, sizeof(CFancyFormat));
    _ccvBorderColorLight.Undefine();
    _ccvBorderColorDark.Undefine();
    _ccvBorderColorHilight.Undefine();
    _ccvBorderColorShadow.Undefine();
    _cuvSpaceBefore.SetValue(0, CUnitValue::UNIT_POINT);
    _cuvSpaceAfter.SetValue(0, CUnitValue::UNIT_POINT);

    for ( i = BORDER_TOP; i <= BORDER_LEFT; i++ )
    {
        _ccvBorderColors[i].Undefine();
        _cuvBorderWidths[i].SetNull();
        _bBorderStyles[i] = (BYTE)-1;
    }

    _ccvBackColor.Undefine();
    _cuvBgPosX.SetNull();
    _cuvBgPosY.SetNull();
    _fBgRepeatX = 1;
    _fBgRepeatY = 1;

    // Restore the orignial value
    _iExpandos = (SHORT)iOrigiExpandos;
}

//+------------------------------------------------------------------------
//
//  CFancyFormat::~CFancyFormat
//
//-------------------------------------------------------------------------
CFancyFormat::~CFancyFormat()
{
    if ( _pszFilters )
    {
        MemFree( _pszFilters );
        _pszFilters = NULL;
    }
    if(_iExpandos >= 0)
    {
        TLS( _pStyleExpandoCache )->ReleaseData(_iExpandos);
        _iExpandos = -1;
    }
}

//+------------------------------------------------------------------------
//
//  Member:     CFancyFormat::operator=
//
//  Synopsis:   Copy members of this struct to another
//
//-------------------------------------------------------------------------
CFancyFormat&
CFancyFormat::operator=(const CFancyFormat &ff)
{
    if(_iExpandos >= 0)
        TLS( _pStyleExpandoCache )->ReleaseData(_iExpandos);
    memcpy(this, &ff, sizeof(*this));
    if ( _pszFilters )
       _pszFilters = _tcsdup ( _pszFilters );
    // Addref the new expando table
    if(_iExpandos >= 0)
        TLS( _pStyleExpandoCache )->AddRefData(_iExpandos);
   return *this;
}

CFancyFormat&
CFancyFormat::Copy ( const CFancyFormat &ff )
{
    if ( _pszFilters )
       MemFree( _pszFilters );
    *this = ff;
    return *this;
}


//+------------------------------------------------------------------------
//
//  Member:     CFancyFormat::Compare
//
//  Synopsis:   Compare 2 structs
//              return TRUE iff equal, else FALSE
//
//-------------------------------------------------------------------------
BOOL
CFancyFormat::Compare(const CFancyFormat *pFF) const
{
    Assert(pFF);

    BOOL fRet = memcmp(this, pFF, offsetof(CFancyFormat, _pszFilters));
    if (!fRet)
    {
        fRet = (_pszFilters && pFF->_pszFilters) ?
                    _tcsicmp( _pszFilters, pFF->_pszFilters ) :
        ( (!_pszFilters && !pFF->_pszFilters) ? FALSE : TRUE );
    }
    return (!fRet);
}

//+------------------------------------------------------------------------
//
//  Member:     CFancyFormat::ComputeCrc
//
//  Synopsis:   Compute Hash
//
//-------------------------------------------------------------------------
WORD
CFancyFormat::ComputeCrc() const
{
    DWORD dwCrc=0, z;

    for (z=0;z<offsetof(CFancyFormat, _pszFilters)/sizeof(DWORD);z++)
    {
        dwCrc ^= ((DWORD*) this)[z];
    }
    if ( _pszFilters )
    {
        TCHAR *psz = _pszFilters;

        while ( *psz )
        {
            dwCrc ^= (*psz++);
        }
    }
    return (LOWORD(dwCrc) ^ HIWORD(dwCrc));
}




//+----------------------------------------------------------------------------
//
//  Function:   CopyAttrVal
//
//  Synopsis:   Copies wSize bytes of dwSrc onto pDest
//
//-------------------------------------------------------------------------
inline void CopyAttrVal(BYTE *pDest, DWORD dwSrc, WORD wSize)
{
    switch (wSize)
    {
    case 1:
        *(BYTE*) pDest  = (BYTE) dwSrc;
        break;
    case 2:
        *(WORD*) pDest  = (WORD) dwSrc;
        break;
    case 4:
        *(DWORD*) pDest  = (DWORD) dwSrc;
        break;
    default:
        Assert(0);
    }
}

//+----------------------------------------------------------------------------
//
//  Function:   ApplyClear
//
//  Synopsis:   Helper function called exactly once from ApplyAttrArrayOnXF.
//              Apply a Clear Type
//
//-----------------------------------------------------------------------------
void ApplyClear(CElement * pElem, htmlClear hc, CFormatInfo *pCFI)
{
    BOOL fClearLeft = FALSE;
    BOOL fClearRight = FALSE;

    switch (hc)
    {
    case htmlClearBoth:
    case htmlClearAll:
        fClearLeft  = fClearRight = TRUE;
        break;
    case htmlClearLeft:
        fClearLeft  = TRUE;
        fClearRight = FALSE;
        break;
    case htmlClearRight:
        fClearRight = TRUE;
        fClearLeft  = FALSE;
        break;
    case htmlClearNone:
        fClearLeft  = fClearRight = FALSE;
        break;
    case htmlClearNotSet:
        AssertSz(FALSE, "Invalid Clear value.");
        break;
    }

    pCFI->PrepareFancyFormat();
    pCFI->_ff()._fClearLeft  = fClearLeft;
    pCFI->_ff()._fClearRight = fClearRight;
    pCFI->UnprepareForDebug();
}

//+----------------------------------------------------------------------------
//
//  Function:   ApplyFontFace
//
//  Synopsis:   Helper function called exactly once from ApplyAttrArrayOnXF.
//              Apply a Font Face
//
//-----------------------------------------------------------------------------

struct CFBAG
{
    BOOL          fMatchedOnce;
    CCharFormat * pcf;
    BYTE          bCharSet;

    CFBAG(CCharFormat * pcfArg, BYTE bCharSetArg)
    {
        fMatchedOnce = FALSE;
        pcf = pcfArg;
        bCharSet = bCharSetArg;
    };
};

#ifdef WIN16
int FAR PASCAL CALLBACK
__ApplyFontFace_Compare( const ENUMLOGFONT FAR * lplf, const NEWTEXTMETRIC FAR * lptm,
                            int FontType, LPARAM lParam )
{
    //
    // Here's the algorithm -- If we find a charset match, we'll take it.
    // The next best thing is a facename match, even if it means getting
    // the wrong charset.
    //

    BOOL fCharsetMatched = ((struct CFBAG *)lParam)->bCharSet ==
                           lplf->elfLogFont.lfCharSet;

    if ( fCharsetMatched || !((struct CFBAG *)lParam)->fMatchedOnce )
    {
        ((struct CFBAG *)lParam)->fMatchedOnce = TRUE;
        ((struct CFBAG *)lParam)->pcf->_bCharSet  = lplf->elfLogFont.lfCharSet;
        ((struct CFBAG *)lParam)->pcf->_bPitchAndFamily = lplf->elfLogFont.lfPitchAndFamily;
        ((struct CFBAG *)lParam)->pcf->SetFaceName( lplf->elfLogFont.lfFaceName );
    }

    return !fCharsetMatched;
}
#else
static int CALLBACK
__ApplyFontFace_Compare( const LOGFONT FAR * lplf, const TEXTMETRIC FAR * lptm,
                            DWORD FontType, LPARAM lParam )
{
    //
    // Here's the algorithm -- If we find a charset match, we'll take it.
    // The next best thing is a facename match, even if it means getting
    // the wrong charset.
    //

    BOOL fCharsetMatched = ((struct CFBAG *)lParam)->bCharSet ==
                           lplf->lfCharSet;

    if ( fCharsetMatched || !((struct CFBAG *)lParam)->fMatchedOnce )
    {
        ((struct CFBAG *)lParam)->fMatchedOnce = TRUE;
        ((struct CFBAG *)lParam)->pcf->_bCharSet  = lplf->lfCharSet;
        ((struct CFBAG *)lParam)->pcf->_bPitchAndFamily = lplf->lfPitchAndFamily;
        ((struct CFBAG *)lParam)->pcf->SetFaceName( lplf->lfFaceName );
    }

    return !fCharsetMatched;
}
#endif //!WIN16

// If the supplied face name maps to a generic font family, fill in the
// appropriate pCF members and return TRUE.  Otherwise, leave pCF untouched
// and return FALSE.


static BOOL
__ApplyFontFace_MatchGenericFamily(
    TCHAR* szFaceName,
    CCharFormat* pCF )
{
    int n;

    // BUGBUG johnv: If the array were a bit bigger we should probably hash.
    for( n = 0; n < ARRAY_SIZE( s_fontFamilyMap ); ++n )
    {
        if( StrCmpIC( szFaceName, s_fontFamilyMap[ n ].szGenericFamily ) == 0 )
        {
            pCF->SetFaceName( _T("") );     // we found a family, not a face name
            pCF->_bPitchAndFamily = s_fontFamilyMap[ n ].dwWindowsFamily;
            pCF->_bCharSet = DEFAULT_CHARSET;
            return TRUE;
        }
    }
    return FALSE;
}

// If the supplied face name maps to a successfully downloaded (embedded) font, fill in the
// appropriate pCF members and return TRUE.  Otherwise, leave pCF untouched
// and return FALSE.

static BOOL
__ApplyFontFace_MatchDownloadedFont( TCHAR* szFaceName, CCharFormat* pCF, CDoc *pDoc )
{
    CStyleSheetArray * pStyleSheets = pDoc->_pPrimaryMarkup->GetStyleSheetArray();
    if ( !pStyleSheets )
        return FALSE;

    int n = pStyleSheets->_apFontFaces.Size();
    CFontFace **ppFace = (CFontFace **)pStyleSheets->_apFontFaces;

    for( ; n > 0; --n, ppFace++ )
    {
        if( ( (*ppFace)->IsInstalled() ) && ( _tcsicmp( szFaceName, (*ppFace)->GetFriendlyName() ) == 0 ) )
        {
            pCF->SetFaceName( (*ppFace)->GetInstalledName() );
            pCF->_bPitchAndFamily = DEFAULT_PITCH | FF_DONTCARE;
            pCF->_bCharSet = DEFAULT_CHARSET;
            pCF->_fDownloadedFont = TRUE;
            return TRUE;
        }
    }
    return FALSE;
}

//+----------------------------------------------------------------------------
//
//  Function:   __ApplyFontFace_ExtractFamilyName, static
//
//  Synopsis:   From the comma-delimited font-family property, extract the
//              next facename.
//
//              As of IE5, we also decode hex-encoded unicode codepoints.
//
//  Returns:    Pointer to scan the next iteration.
//
//-----------------------------------------------------------------------------

static TCHAR *
__ApplyFontFace_ExtractFamilyName(
    TCHAR * pchSrc,
    TCHAR achFaceName[LF_FACESIZE] )
{
    enum PSTATE
    {
        PSTATE_START,
        PSTATE_COPY,
        PSTATE_HEX
    } state = PSTATE_START;

    TCHAR * pchDst = achFaceName;
    TCHAR * pchDstStop = achFaceName + LF_FACESIZE - 1;
    TCHAR chHex = 0;
    TCHAR chStop = _T(',');

    achFaceName[0] = 0;

    while (pchDst < pchDstStop)
    {
        const TCHAR ch = *pchSrc++;

        if (!ch)
        {
            if (chHex)
            {
                *pchDst++ = chHex;
            }

            --pchSrc; // backup for outer while loop.
            break;
        }

        switch (state)
        {
            case PSTATE_START:
                if (ch == _T(' ') || ch == _T(',')) // skip leading junk
                {
                    break;
                }

                state = PSTATE_COPY;

                if (ch == _T('\"') || ch == _T('\'')) // consume quote.
                {
                    chStop = ch;
                    break;
                }

                // fall through if non of the above

            case PSTATE_COPY:
                if (ch == chStop)
                {
                    pchDstStop = pchDst; // force exit
                }
                else if (ch == _T('\\'))
                {
                    chHex = 0;
                    state = PSTATE_HEX;
                }
                else
                {
                    *pchDst++ = ch;
                }
                break;

            case PSTATE_HEX:
                if (InRange( ch, _T('a'), _T('f')))
                {
                    chHex = (chHex << 4) + (ch - _T('a') + 10);
                }
                else if (InRange( ch, _T('A'), _T('F')))
                {
                    chHex = (chHex << 4) + (ch - _T('A') + 10);
                }
                else if (InRange( ch, _T('0'), _T('9')))
                {
                    chHex = (chHex << 4) + (ch - _T('0'));
                }
                else if (chHex)
                {
                    *pchDst++ = chHex;
                    --pchSrc; // backup
                    state = PSTATE_COPY;
                    chHex = 0;
                }
                else
                {
                    *pchDst++ = ch;
                }
        }
    }

    // Trim off trailing white space.

    while (pchDst > achFaceName && pchDst[-1] == _T(' '))
    {
        --pchDst;
    }
    *pchDst = 0;

    return pchSrc;
}

/* inline */
void
ApplyFontFace( CCharFormat *pCF, LPTSTR p, CDoc *pDoc )
{
    // Often fonts are specified as a comma-separated list, so we need to
    // just pick the first one in the list which is installed on the user's
    // system.
    // Note: If no fonts in the list are available pCF->szFaceName will not
    //       be touched.
    if ( p && p[0] )
    {
        HDC    hDC;
        TCHAR *pStr;
        TCHAR  szFaceName[ LF_FACESIZE ];
        BYTE   bCharSet = pDoc->_pCodepageSettings->bCharSet;
        const BOOL fIsPrintDoc = pDoc && pDoc->IsPrintDoc();
        BOOL fCacheHit=FALSE;
        CFontCache *pfc = &fc();

        if (fIsPrintDoc)
        {
            // if we're currently printing, we should enumerate Printer dc fonts
            hDC = DYNCAST(CPrintDoc, pDoc->GetRootDoc())->_hdc;
            Assert(hDC && "looks like we have a desktop DC instead of a printer DC");
        }
        else
        {
            hDC = TLS(hdcDesktop);
        }

        // Because we have a cache for most recently used face names
        // that is used for all instances, we enclose the whole thing
        // in a critical section.
        if (!fIsPrintDoc)
            EnterCriticalSection(&pfc->_csFaceCache);

        pStr = (LPTSTR)p;
        while( *pStr )
        {
            struct CFBAG cfbag( pCF, bCharSet );
            BOOL fFound;

            pStr = __ApplyFontFace_ExtractFamilyName( pStr, szFaceName );

            if ( !szFaceName[0] )
                break;

            // When we're on the screen, we use a cache to speed things up.
            // We don't use if for the printer because we share the cache
            // across all DCs, and we don't want to have matching problems
            // across DCs.
            if (!fIsPrintDoc)
            {
                // The atom name for the given face name.
                LONG lAtom = pfc->FindAtomFromFaceName(szFaceName);

                // Check to see if it's the same as a recent find.
                if (lAtom)
                {
                    int i;
                    for (i=0; i<pfc->_iCacheLen; i++)
                    {
                        // Check the facename and charset.
                        if (lAtom == pfc->_fcFaceCache[i]._latmFaceName &&
                            bCharSet == pfc->_fcFaceCache[i]._bCharSet)
                        {
                            pCF->_bCharSet = pfc->_fcFaceCache[i]._bCharSet;
                            pCF->_bPitchAndFamily = pfc->_fcFaceCache[i]._bPitchAndFamily;
                            pCF->_latmFaceName = pfc->_fcFaceCache[i]._latmFaceName;
                            pCF->_fExplicitFace = pfc->_fcFaceCache[i]._fExplicitFace;
                            pCF->_fNarrow = pfc->_fcFaceCache[i]._fNarrow;
#if DBG==1
                            pfc->_iCacheHitCount++;
#endif
                            fCacheHit = TRUE;
                            break;
                        }
                    }
                }
                if (fCacheHit)
                    break;
            }

            // BUGBUG (cthrash) Perf - we can bypass the calls to GDI and
            // simply look up our font-info cache (fc()._atFontInfo) to see
            // if we've already a match.  The issue we need to iron out is
            // getting other info (charset/pitch&family) cached correctly.

            // (cthrash) The if clause represents a WinNT PCL bug workaround.
            // Use Ex enumeration function, which seems better implemented
            // under NT.  We should consider using this always (not just for
            // printing) in 5.0.  We're late in the game for 4.01 so do this
            // only for NT PrintDoc's for now.

            if (g_dwPlatformID == VER_PLATFORM_WIN32_NT && fIsPrintDoc)
            {
                LOGFONT lf;

                // First attempt a charset *and* facename match.

                lf.lfCharSet = bCharSet;
                lf.lfPitchAndFamily = 0;
                StrCpyN( lf.lfFaceName, szFaceName, LF_FACESIZE );

                if( __ApplyFontFace_MatchDownloadedFont( szFaceName, pCF, pDoc ) ||
                    EnumFontFamiliesEx( hDC, &lf, __ApplyFontFace_Compare, (LPARAM)&cfbag, 0L) == 0 )
                {
                    pCF->_fExplicitFace = TRUE;
                    break;
                }

                // Now try a facename match (ignore charset).

                lf.lfCharSet = DEFAULT_CHARSET;

                if (EnumFontFamiliesEx( hDC, &lf, __ApplyFontFace_Compare, (LPARAM)&cfbag, 0L) == 0 ||
                    cfbag.fMatchedOnce ||
                    __ApplyFontFace_MatchGenericFamily( szFaceName, pCF ) == TRUE)
                {
                    pCF->_fExplicitFace = TRUE;
                    break;
                }
            }

            //
            // Check to see if the font exists.  If so, break and return
            //

            // (1) Check our download font list to see if we have a match

            fFound = __ApplyFontFace_MatchDownloadedFont( szFaceName, pCF, pDoc );

            // (2) Check the system if we have a facename match

            if (!fFound)
            {
                fFound =    EnumFontFamilies( hDC, szFaceName, __ApplyFontFace_Compare, (LPARAM)&cfbag) == 0
                         || cfbag.fMatchedOnce;

                if (fFound)
                {
                    // BUGBUG (cthrash) Terribly hack for Mangal.  When we
                    // enumerate for Mangal, which as Indic font, GDI says we
                    // have font with a GDI charset of ANSI.  This is simply
                    // false, and furthermore will not create the proper font
                    // when we do a CreateFontIndirect.  Hack this so we have
                    // some hope of handling Indic correctly.

#if defined(UNIX) || defined(_MAC)
                    if (StrCmpIC( szFaceName, TEXT("Mangal")) == 0)
                    {
                        pCF->_bCharSet = DEFAULT_CHARSET;
                    }
#else
                    if (   (((*(DWORD *)szFaceName) & 0xffdfffdf) == 0x41004d)
                        && StrCmpIC( szFaceName, TEXT("Mangal")) == 0)
                    {
                        pCF->_bCharSet = DEFAULT_CHARSET;
                    }
#endif
                }
            }

            // (3) Check if we have an known alternate name, and see if the
            //     system has a match

            if (!fFound)
            {
                const TCHAR * pAltFace = AlternateFontName( szFaceName );

                fFound =    pAltFace
                         && (   EnumFontFamilies( hDC, pAltFace, __ApplyFontFace_Compare, (LPARAM)&cfbag) == 0
                             || cfbag.fMatchedOnce);
            }

            // (4) Check for generic family names (Serif, etc.)

            if (!fFound)
            {
                // BUGBUG (cthrash) If we have generic CSS family name, we
                // should Consider picking a facename here, rather than
                // forcing CCcs::MakeFont run around and pick one.

                fFound = __ApplyFontFace_MatchGenericFamily( szFaceName, pCF );
            }

            if (fFound)
            {
                //
                // HACK (cthrash) Font enumeration doens't work on certain
                // drivers.  Insist on the charset rather than the facename.
                //

                if (   VER_PLATFORM_WIN32_WINDOWS == g_dwPlatformID
                    && cfbag.fMatchedOnce
                    && pCF->_bCharSet != bCharSet
                    && pCF->_bCharSet != SYMBOL_CHARSET
                    && pCF->_bCharSet != OEM_CHARSET
                    && fIsPrintDoc)
                {
                    pCF->_bCharSet = bCharSet;
                }

                pCF->_fExplicitFace = TRUE;
                pCF->_fNarrow = IsNarrowCharSet(pCF->_bCharSet);
                break;
            }
        }

        // Update the cache and leave the critical section.
        if (!fIsPrintDoc)
        {
            if (!fCacheHit)
            {
                int iCacheNext = pfc->_iCacheNext;
                pfc->_fcFaceCache[iCacheNext]._bCharSet = pCF->_bCharSet;
                pfc->_fcFaceCache[iCacheNext]._bPitchAndFamily = pCF->_bPitchAndFamily;
                pfc->_fcFaceCache[iCacheNext]._latmFaceName = pCF->_latmFaceName;
                pfc->_fcFaceCache[iCacheNext]._fExplicitFace = pCF->_fExplicitFace;
                pfc->_fcFaceCache[iCacheNext]._fNarrow = pCF->_fNarrow;

                pfc->_iCacheNext++;
                if (pfc->_iCacheLen < CACHEMAX)
                    pfc->_iCacheLen++;
                if (pfc->_iCacheNext >= CACHEMAX)
                    pfc->_iCacheNext = 0;
            }
            LeaveCriticalSection(&pfc->_csFaceCache);
        }

    }
}

//+----------------------------------------------------------------------------
//
//  Function:   HandleLargerSmaller
//
//  Synopsis:   Helper function called from ApplyFontSize to handle "larger"
//              and "smaller" font sizes.  Returns TRUE if we handled a "larger"
//              or "smaller" case, returns FALSE if we didn't do anything.
//
//-----------------------------------------------------------------------------

static BOOL HandleLargerSmaller(long *plSize, long lUnitValue )
{
    long lSize = *plSize;
    const int nNumElem = ARRAY_SIZE(aiSizesInTwips);
    int i;

    switch (lUnitValue)
    {
    case styleFontSizeLarger:
        // Look for a larger size in the abs. size array
        for (i=0 ; i < nNumElem ; ++i)
        {
            if (aiSizesInTwips[i] > lSize)
            {
                *plSize = aiSizesInTwips[i];
                return TRUE;
            }
        }
        // We are bigger than "xx-large"; use factor of 1.5x
        *plSize = lSize * 3 / 2;
        return TRUE;
    case styleFontSizeSmaller:
        // If we are more than 1.5x bigger than the largest size in the
        // abs. size array, then only shrink down by 1.5x.
        if ( (lSize * 2 / 3) > aiSizesInTwips[nNumElem-1] )
        {
            *plSize = lSize * 2 / 3;
            return TRUE;
        }
        // Look for a smaller size in the abs. size array
        for (i=nNumElem-1 ; i >= 0 ; --i)
        {
            if (aiSizesInTwips[i] < lSize)
            {
                *plSize = aiSizesInTwips[i];
                return TRUE;
            }
        }
        // We are smaller than "xx-small"; use factor of 1/1.5x
        *plSize = lSize * 2 / 3;
        return TRUE;
    default:
        return FALSE;
    }
}

//+----------------------------------------------------------------------------
//
//  Function:   ApplyFontSize
//
//  Synopsis:   Helper function called exactly once from ApplyAttrArrayOnXF.
//              Apply a Font Size
//
//-----------------------------------------------------------------------------
void ApplyFontSize(CCharFormat *pCF, const CUnitValue *pCUV, CTreeNode * pNode, BOOL fAlwaysUseMyFontSize)
{
    LONG lSize;
    BOOL fScale = FALSE;
    CUnitValue::UNITVALUETYPE uvt;

    Assert( pNode );
    Assert( pCF );

    if ( pCUV->IsNull() )
        return;

    // See what value we inherited in the CF.  lSize is in twips.
    lSize = pCF->GetHeightInTwips( pNode->Doc() );

    // But use our parent element's value if we can get it, as per CSS1 spec for font-size.
    CTreeNode * pNodeParent = pNode->Parent();
    if (pNodeParent)
    {
        const CCharFormat *pParentCF = pNodeParent->GetCharFormat();
        if (pParentCF)
        {
            lSize = pParentCF->GetHeightInTwips( pNodeParent->Doc() );
            fScale = !pParentCF->_fSizeDontScale;
        }
    }

    if (pCUV->IsNull() )
        return;

    uvt = pCUV->GetUnitType();

    switch(uvt)
    {
    case CUnitValue::UNIT_RELATIVE:     // Relative to the base font size
        {
            LONG lBaseFontHtmlSize = 3;

            // Base fonts don't inherit through table cells or other
            // text sites.
            pNode = pNode->SearchBranchToFlowLayoutForTag( ETAG_BASEFONT );

            if (pNode)
                lBaseFontHtmlSize = DYNCAST(CBaseFontElement, pNode->Element())->GetFontSize();

            lSize = ConvertHtmlSizeToTwips( lBaseFontHtmlSize + pCUV->GetUnitValue() );
        }
        fScale = TRUE;
        break;
    case CUnitValue::UNIT_PERCENT:
        lSize = MulDivQuick( lSize, pCUV->GetPercent(), 100 );
        fScale = FALSE;
        break;
    case CUnitValue::UNIT_EM:
        // BUGBUG: assumes 1 EM == lSize (font height).  This may be suspicious.
        lSize = MulDivQuick(lSize, pCUV->GetUnitValue(), CUnitValue::TypeNames[CUnitValue::UNIT_EM].wScaleMult);
        break;
//        UNIT_EN,            // en's ( relative to width of an "n" character in current-font )
    case CUnitValue::UNIT_EX:
        // BUGBUG: assumes 1 EN == lSize/2 (font height)/2.  This may be suspicious.
        lSize = MulDivQuick( lSize, pCUV->GetUnitValue(), CUnitValue::TypeNames[CUnitValue::UNIT_EX].wScaleMult * 2 );
        break;
    case CUnitValue::UNIT_POINT:  // Convert from points to twips
        lSize = MulDivQuick( pCUV->GetUnitValue(), 20, CUnitValue::TypeNames[CUnitValue::UNIT_POINT].wScaleMult );
        fScale = FALSE;
        break;
    case CUnitValue::UNIT_ENUM:
        if ( !HandleLargerSmaller( &lSize, pCUV->GetUnitValue() ) ) // lSize modified if ret=true
            lSize = ConvertHtmlSizeToTwips( pCUV->GetUnitValue() + 1 ); // Must be "xx-small", etc.
        fScale = TRUE;
        break;
    case CUnitValue::UNIT_INTEGER:
        lSize = ConvertHtmlSizeToTwips( pCUV->GetUnitValue() );
        fScale = TRUE;
        break;
    case CUnitValue::UNIT_PICA:
        lSize = MulDivQuick( pCUV->GetUnitValue(), 240, CUnitValue::TypeNames[CUnitValue::UNIT_PICA].wScaleMult );
        fScale = FALSE;
        break;
    case CUnitValue::UNIT_INCH:
        lSize = MulDivQuick( pCUV->GetUnitValue(), 1440, CUnitValue::TypeNames[CUnitValue::UNIT_INCH].wScaleMult );
        fScale = FALSE;
        break;
    case CUnitValue::UNIT_CM:
        lSize = MulDivQuick( pCUV->GetUnitValue(), 144000, CUnitValue::TypeNames[CUnitValue::UNIT_CM].wScaleMult * 254 );
        fScale = FALSE;
        break;
    case CUnitValue::UNIT_MM:
        lSize = MulDivQuick( pCUV->GetUnitValue(), 144000, CUnitValue::TypeNames[CUnitValue::UNIT_MM].wScaleMult * 2540 );
        fScale = FALSE;
        break;
    case CUnitValue::UNIT_PIXELS:
        {
            CUnitValue cuv;

            cuv.SetRawValue( pCUV->GetRawValue() );
            THR( cuv.ConvertToUnitType( CUnitValue::UNIT_POINT, 1, CUnitValue::DIRECTION_CY, lSize ) );
            lSize = MulDivQuick( cuv.GetUnitValue(), 20, CUnitValue::TypeNames[CUnitValue::UNIT_POINT].wScaleMult );
            fScale = FALSE;
        }
        break;
    default:
        //AssertSz(FALSE,"Suspicious CUnitValue in ApplyFontSize.");
        break;
    }

    if ( lSize < 0 )
        lSize = -lSize;

    if (pCF->_fBumpSizeDown &&
        uvt != CUnitValue::UNIT_RELATIVE &&
        uvt != CUnitValue::UNIT_PERCENT &&
        uvt != CUnitValue::UNIT_TIMESRELATIVE)
    {
        // For absolute font size specifications, element which normally
        // may have set fBumpSize down must cancel it (nav compatibility).
        pCF->_fBumpSizeDown = FALSE;
    }


    // Make sure that size is within the abs. size array range if
    // 'Always Use My Font Size' is chosen from the option settings
    if (fAlwaysUseMyFontSize)
    {
        long nSizes = ARRAY_SIZE(aiSizesInTwips);
        lSize = (lSize < aiSizesInTwips[0]) ? aiSizesInTwips[0] : ((lSize > aiSizesInTwips[nSizes-1]) ? aiSizesInTwips[nSizes-1] : lSize);
    }

    if ( fScale )
        pCF->SetHeightInTwips( lSize );
    else
        pCF->SetHeightInNonscalingTwips( lSize );
}

//+----------------------------------------------------------------------------
//
//  Function:   ApplyBaseFont
//
//  Synopsis:   Helper function called exactly once from ApplyAttrArrayOnXF.
//              Apply a Base Font Size
//
//-----------------------------------------------------------------------------
inline void ApplyBaseFont(CCharFormat *pCF, long lSize)
{
    pCF->SetHeightInTwips( ConvertHtmlSizeToTwips(lSize) );
}

//+----------------------------------------------------------------------------
//
//  Function:   ApplyFontStyle
//
//  Synopsis:   Helper function called exactly once from ApplyAttrArrayOnXF.
//              Apply a Font Style
//
//-----------------------------------------------------------------------------
inline void ApplyFontStyle(CCharFormat *pCF, styleFontStyle sfs)
{
    if ( sfs == styleFontStyleNotSet )
        return;

    if ( ( sfs == styleFontStyleItalic ) || ( sfs == styleFontStyleOblique ) )
        pCF->_fItalic = TRUE;
    else
        pCF->_fItalic = FALSE;
}


//+----------------------------------------------------------------------------
//
//  Function:   ApplyFontWeight
//
//  Synopsis:   Helper function called exactly once from ApplyAttrArrayOnXF.
//              Apply a Font Weight
//
//-----------------------------------------------------------------------------
inline void ApplyFontWeight(CCharFormat *pCF, styleFontWeight sfw )
{
    if ( sfw == styleFontWeightNotSet  )
        return;

    if ( sfw == styleFontWeightBolder )
    {
        pCF->_wWeight = (WORD)min( 900, pCF->_wWeight+300 );
    }
    else if ( sfw == styleFontWeightLighter )
    {
        pCF->_wWeight = (WORD)max( 100, pCF->_wWeight-300 );
    }
    else
    {
        //See wingdi.h and our enum table
        //we currently do not handle relative boldness
        Assert(1 == styleFontWeight100);

        if ( sfw == styleFontWeightNormal )
            sfw = styleFontWeight400;
        if ( sfw == styleFontWeightBold )
            sfw = styleFontWeight700;

        pCF->_wWeight = 100 * (DWORD) sfw;
    }

    if (FW_NORMAL < pCF->_wWeight)
        pCF->_fBold = TRUE;
    else
        pCF->_fBold = FALSE;
}

BOOL g_fSystemFontsNeedRefreshing = TRUE;
#ifndef WIN16
#define NUM_SYS_FONTS 6
static LOGFONTW alfSystemFonts[ NUM_SYS_FONTS ]; // sysfont_caption, sysfont_icon, sysfont_menu, sysfont_messagebox, sysfont_smallcaption, sysfont_statusbar

void RefreshSystemFontCache( void )
{
#ifdef WINCE
    GetObject(GetStockObject(SYSTEM_FONT), sizeof(LOGFONT), &alfSystemFonts[ sysfont_icon ]);

    memcpy( &alfSystemFonts[ sysfont_caption ], &alfSystemFonts[ sysfont_icon ], sizeof(LOGFONT) );
    memcpy( &alfSystemFonts[ sysfont_menu ], &alfSystemFonts[ sysfont_icon ], sizeof(LOGFONT) );
    memcpy( &alfSystemFonts[ sysfont_messagebox ], &alfSystemFonts[ sysfont_icon ], sizeof(LOGFONT) );
    memcpy( &alfSystemFonts[ sysfont_smallcaption ], &alfSystemFonts[ sysfont_icon ], sizeof(LOGFONT) );
    memcpy( &alfSystemFonts[ sysfont_statusbar ], &alfSystemFonts[ sysfont_icon ], sizeof(LOGFONT) );
#else
    NONCLIENTMETRICS ncm;
    ncm.cbSize  = sizeof(NONCLIENTMETRICS);

    SystemParametersInfo( SPI_GETICONTITLELOGFONT, sizeof(LOGFONT), &alfSystemFonts[ sysfont_icon ], 0 );

    if ( SystemParametersInfo( SPI_GETNONCLIENTMETRICS, 0, &ncm, 0 ) )
    {   // Copy fonts into place
        memcpy( &alfSystemFonts[ sysfont_caption ], &ncm.lfCaptionFont, sizeof(LOGFONT) );
        memcpy( &alfSystemFonts[ sysfont_menu ], &ncm.lfMenuFont, sizeof(LOGFONT) );
        memcpy( &alfSystemFonts[ sysfont_messagebox ], &ncm.lfMessageFont, sizeof(LOGFONT) );
        memcpy( &alfSystemFonts[ sysfont_smallcaption ], &ncm.lfSmCaptionFont, sizeof(LOGFONT) );
        memcpy( &alfSystemFonts[ sysfont_statusbar ], &ncm.lfStatusFont, sizeof(LOGFONT) );
    }
    else
    {   // Probably failed due to Unicodeness, try again with short char version of NCM.
        NONCLIENTMETRICSA ncma;

        ncma.cbSize = sizeof(NONCLIENTMETRICSA);
        if ( SystemParametersInfo( SPI_GETNONCLIENTMETRICS, 0, &ncma, 0 ) )
        {   // Copy the fonts and do the Unicode conversion.
            memcpy( &alfSystemFonts[ sysfont_caption ], &ncma.lfCaptionFont, sizeof(LOGFONTA) );
            UnicodeFromMbcs( alfSystemFonts[ sysfont_caption ].lfFaceName, ARRAY_SIZE(alfSystemFonts[ sysfont_caption ].lfFaceName),
                             ncma.lfCaptionFont.lfFaceName );
            memcpy( &alfSystemFonts[ sysfont_menu ], &ncma.lfMenuFont, sizeof(LOGFONTA) );
            UnicodeFromMbcs( alfSystemFonts[ sysfont_menu ].lfFaceName, ARRAY_SIZE(alfSystemFonts[ sysfont_menu ].lfFaceName),
                             ncma.lfMenuFont.lfFaceName );
            memcpy( &alfSystemFonts[ sysfont_messagebox ], &ncma.lfMessageFont, sizeof(LOGFONTA) );
            UnicodeFromMbcs( alfSystemFonts[ sysfont_messagebox ].lfFaceName, ARRAY_SIZE(alfSystemFonts[ sysfont_messagebox ].lfFaceName),
                             ncma.lfMessageFont.lfFaceName );
            memcpy( &alfSystemFonts[ sysfont_smallcaption ], &ncma.lfSmCaptionFont, sizeof(LOGFONTA) );
            UnicodeFromMbcs( alfSystemFonts[ sysfont_smallcaption ].lfFaceName, ARRAY_SIZE(alfSystemFonts[ sysfont_smallcaption ].lfFaceName),
                             ncma.lfSmCaptionFont.lfFaceName );
            memcpy( &alfSystemFonts[ sysfont_statusbar ], &ncma.lfStatusFont, sizeof(LOGFONTA) );
            UnicodeFromMbcs( alfSystemFonts[ sysfont_statusbar ].lfFaceName, ARRAY_SIZE(alfSystemFonts[ sysfont_statusbar ].lfFaceName),
                             ncma.lfStatusFont.lfFaceName );
        }
    }
#endif // WINCE
    g_fSystemFontsNeedRefreshing = FALSE;
}

void ApplySystemFont( CCharFormat *pCF, Esysfont eFontType )
{
    LOGFONT *lplf = NULL;
    long lSize = 1;

    if ( g_fSystemFontsNeedRefreshing )
        RefreshSystemFontCache();

    if ( ( eFontType < sysfont_caption ) || ( eFontType > sysfont_statusbar ) )
        return;

    lplf = &alfSystemFonts[ eFontType ];

    pCF->SetFaceName( lplf->lfFaceName );
    pCF->_bCharSet = lplf->lfCharSet;
    pCF->_fNarrow = IsNarrowCharSet(lplf->lfCharSet);
    pCF->_bPitchAndFamily = lplf->lfPitchAndFamily;
    pCF->_wWeight = lplf->lfWeight;
    pCF->_fBold = (pCF->_wWeight > 400);
    pCF->_fItalic = lplf->lfItalic;
    pCF->_fUnderline = lplf->lfUnderline;
    pCF->_fStrikeOut = lplf->lfStrikeOut;

    lSize = MulDivQuick( lplf->lfHeight,  TWIPS_PER_INCH, g_sizePixelsPerInch.cy );
    if ( lSize < 0 )
        lSize = -lSize;
    pCF->SetHeightInTwips( lSize );
}
#endif // !WIN16

void ApplySiteAlignment (CFormatInfo *pCFI, htmlControlAlign at, CElement * pElem)
{
    if ( at == htmlControlAlignNotSet )
        return;

    if(at == htmlControlAlignCenter)
        pCFI->_bCtrlBlockAlign = htmlBlockAlignCenter;

    if(pElem->Tag() == ETAG_LEGEND)
    {
        if(at == htmlControlAlignLeft)
            pCFI->_bCtrlBlockAlign = htmlBlockAlignLeft;

        if(at == htmlControlAlignRight)
            pCFI->_bCtrlBlockAlign = htmlBlockAlignRight;
    }

    pCFI->_bControlAlign = at;
}

void ApplyParagraphAlignment ( CFormatInfo *pCFI, htmlBlockAlign at, CElement *pElem )
{
    if ( at == htmlBlockAlignNotSet )
        return;

    switch(at)
    {
    case htmlBlockAlignRight:
    case htmlBlockAlignLeft:
    case htmlBlockAlignCenter:
        pCFI->_bBlockAlign = at;
        break;
    case htmlBlockAlignJustify:
        if (pElem->Tag() != ETAG_CAPTION)   // ignore Justify for captions,
                                            // we use Justify enum in cpation.pdl just to be consistent
                                            // with the rest of the block alignment enums.
        {
            pCFI->_bBlockAlign = at;
        }
        break;
    default:
        if (pElem->Tag() == ETAG_CAPTION)
        {
            // There's some trickyness going on here.  The caption.pdl
            // specifies the align as DISPID_A_BLOCKALIGN - which means it
            // gets written into the lrcAlign - but caption has two extra
            // ( vertical ) enum values that we need to map onto the equivalent
            // vertical alignment values.

            if ((BYTE)at == htmlCaptionAlignTop || (BYTE)at == htmlCaptionAlignBottom)
            {
                at = (htmlBlockAlign)((BYTE)at == htmlCaptionAlignTop? htmlCaptionVAlignTop: htmlCaptionVAlignBottom);
                if (pCFI->_ppf->_bTableVAlignment != (BYTE)at)
                {
                    pCFI->PrepareParaFormat();
                    pCFI->_pf()._bTableVAlignment = (BYTE)at;
                    pCFI->UnprepareForDebug();
                }
                break;
            }
        }
        pCFI->_bBlockAlign = htmlBlockAlignNotSet;
        break;
    }
}

void ApplyVerticalAlignment(CFormatInfo *pCFI, CUnitValue *puv, CElement *pElem)
{
    if ( puv->GetUnitType() == CUnitValue::UNIT_ENUM )
    {
        styleVerticalAlign vt = (styleVerticalAlign) puv->GetUnitValue();

        switch (pElem->Tag())
        {
        case ETAG_TD:
        case ETAG_TH:
        case ETAG_COL:
        case ETAG_COLGROUP:
        case ETAG_TBODY:
        case ETAG_TFOOT:
        case ETAG_THEAD:
        case ETAG_TR:
        case ETAG_CAPTION:
            {
                htmlCellVAlign cvt = htmlCellVAlignNotSet;

                switch ( vt )
                {
                case styleVerticalAlignTop:
                case styleVerticalAlignTextTop:
                    cvt = htmlCellVAlignTop;
                    break;
                case styleVerticalAlignTextBottom:
                case styleVerticalAlignBottom:
                    cvt = htmlCellVAlignBottom;
                    break;
                case styleVerticalAlignBaseline:
                    cvt = htmlCellVAlignBaseline;
                    break;
                case styleVerticalAlignMiddle:
                default:
                    cvt = pElem->Tag() != ETAG_CAPTION? htmlCellVAlignMiddle : htmlCellVAlignTop;
                    break;
                }
                if (pCFI->_ppf->_bTableVAlignment != cvt)
                {
                    pCFI->PrepareParaFormat();
                    pCFI->_pf()._bTableVAlignment = cvt;
                    pCFI->UnprepareForDebug();
                }
            }
            break;
        default:
            {
                htmlControlAlign at       = htmlControlAlignNotSet;
                BOOL             fSetCtrl = TRUE;

                switch(vt)
                {
                case styleVerticalAlignSub:
                    pCFI->PrepareCharFormat();
                    pCFI->_cf()._fSubscript = TRUE;
                    pCFI->UnprepareForDebug();
                    fSetCtrl = FALSE;
                    break;
                case styleVerticalAlignSuper:
                    pCFI->PrepareCharFormat();
                    pCFI->_cf()._fSuperscript = TRUE;
                    pCFI->UnprepareForDebug();
                    fSetCtrl = FALSE;
                    break;

                case styleVerticalAlignTextBottom:
                case styleVerticalAlignBaseline:
                    at = htmlControlAlignNotSet;
                    break;
                case styleVerticalAlignTop:
                    at = htmlControlAlignTop;
                    break;
                case styleVerticalAlignTextTop:
                    at = htmlControlAlignTextTop;
                    break;
                case styleVerticalAlignMiddle:
                    at = htmlControlAlignMiddle;
                    break;
                case styleVerticalAlignBottom:
                    at = htmlControlAlignAbsBottom;
                    break;
                }

                if (fSetCtrl)
                {
                    pCFI->_bControlAlign = at;
                }
            }
        }
    }
    else
    {
        // BUGBUG percents NYI
    }
}

void ApplyTableVAlignment ( CParaFormat *pPF, htmlCellVAlign at)
{
    if ( at == htmlCellVAlignNotSet )
        return;
    pPF ->_bTableVAlignment = at;

}

void ApplyLineHeight( CCharFormat *pCF, CUnitValue *cuv )
{
    Assert( pCF && cuv );

    if ( ( cuv->GetUnitType() == CUnitValue::UNIT_ENUM ) && ( cuv->GetUnitValue() == styleNormalNormal ) )
        cuv->SetNull();

    pCF->_cuvLineHeight = *cuv;

}

BOOL ConvertCSSToFmBorderStyle( long lCSSBorderStyle, BYTE *pbFmBorderStyle )
{
    switch ( lCSSBorderStyle )
    {
    case styleBorderStyleNotSet: // No change
        return FALSE;
    case styleBorderStyleDotted:
        *pbFmBorderStyle = fmBorderStyleDotted;
        break;
    case styleBorderStyleDashed:
        *pbFmBorderStyle = fmBorderStyleDashed;
        break;
    case styleBorderStyleDouble:
        *pbFmBorderStyle = fmBorderStyleDouble;
        break;
    case styleBorderStyleSolid:
        *pbFmBorderStyle = fmBorderStyleSingle;
        break;
    case styleBorderStyleGroove:
        *pbFmBorderStyle = fmBorderStyleEtched;
        break;
    case styleBorderStyleRidge:
        *pbFmBorderStyle = fmBorderStyleBump;
        break;
    case styleBorderStyleInset:
        *pbFmBorderStyle = fmBorderStyleSunken;
        break;
    case styleBorderStyleOutset:
        *pbFmBorderStyle = fmBorderStyleRaised;
        break;
    case styleBorderStyleNone:
        *pbFmBorderStyle = fmBorderStyleNone;
        return FALSE;
    default:
        Assert( FALSE && "Unknown Border Style!" );
        return FALSE;
    }
    return TRUE;
}

void ApplyLang(CCharFormat *pCF, LPTSTR achLang)
{
    if (!pCF)
        return;

    pCF->_lcid = LCIDFromString(achLang);
}

//+------------------------------------------------------------------------
//
//  Method:     ::ApplyAttrArrayValues
//
//  Synopsis:   Apply all the CAttrValues in a CAttrArray to a given set of formats.
//
//      If passType is APPLY_All, work normally.  If passType is APPLY_ImportantOnly,
//  then only apply attrvals with the "!important" bit set.  If passType is
//  APPLY_NoImportant, then only apply attrvals that do not have the "!important"
//  bit set - but if pfContainsImportant is non-NULL, set it to TRUE if you see
//  an attrval with the "!important" bit set.
//
//+------------------------------------------------------------------------
HRESULT ApplyAttrArrayValues (
    CStyleInfo *pStyleInfo,
    CAttrArray **ppAA,
    CachedStyleSheet *pStyleCache /* = NULL */,
    ApplyPassType passType /*=APPLY_All*/,
    BOOL *pfContainsImportant /*=NULL*/,
    BOOL fApplyExpandos /*= TRUE */,
    DISPID dispidSetValue /* = 0 */)
{
    // Apply all Attr values
    HRESULT hr = S_OK;

    if ( !ppAA || !*ppAA || (*ppAA) -> Size() == 0 )
        return S_OK;

    if (passType == APPLY_Behavior)
    {
        hr = THR(ApplyBehaviorProperty(
            *ppAA,
            (CBehaviorInfo*) pStyleInfo,
            pStyleCache));
    }
    else // if (passType != APPLY_Init)
    {
        CFormatInfo *pCFI;
        CAttrValue *pAV;  //The current AttrValue.  Only valid when nAA>0
        INT i;
        VARIANT varVal;

        pAV = (CAttrValue *)*(CAttrArray *)(*ppAA);

        pCFI = (CFormatInfo*)pStyleInfo; //..todo DYNCAST(CFormatInfo, pStyleInfo);

        // Apply regular attributes
        for ( i = 0 ; i < (*ppAA) -> Size() ; i++, pAV++ )
        {
            // When applying extra values apply only the value that was requested
            if ((pCFI->_eExtraValues & ComputeFormatsType_GetValue) && 
                pAV->GetDISPID() != pCFI->_dispIDExtra)
                continue;

            pAV->GetAsVariantNC(&varVal);

            // Apply these only if they're set & actual properties
            if ( pAV->IsStyleAttribute() )
            {
                if ( ( passType == APPLY_All ) ||   // Normal pass
                     ( passType == APPLY_NoImportant && !pAV->IsImportant() ) ||    // Only non-important properties
                     ( passType == APPLY_ImportantOnly && pAV->IsImportant() ) )    // Important properties only
                {
                    hr = THR(ApplyFormatInfoProperty ( pAV->GetPropDesc(), pAV->GetDISPID(),
                        varVal, pCFI, pStyleCache ));
                    if ( hr )
                        break;

                    // If there's an expression with the same DISPID, now is the time to delete it.
                    //
                    // BUGBUG (michaelw) this work should go away when CAttrValueExpression comes into being.
                    //
                    // If dispidSetValue is set then we do not overwrite expressions with that dispid
                    // dispidSetValue == 0 means that we overwrite any and all expressions
                    //
                    // Expression values never overwrite expressions (hence the !pAV->IsExpression())
                    //
                    if (!pAV->IsExpression() && pCFI->_pff->_fHasExpressions && ((dispidSetValue == 0) || (dispidSetValue != pAV->GetDISPID())))
                    {
                        CAttrArray *pAA = pCFI->GetAAExpando();

                        if (pAA)
                        {
#if DBG == 1
                            {
                                CAttrValue *pAVExpr = pAA->Find(pAV->GetDISPID(), CAttrValue::AA_Expression, NULL, FALSE);
                                if (pAVExpr)
                                {
                                    TraceTag((tagRecalcStyle, "ApplyAttrArrayValues: overwriting expression: dispid: %08x expr: %ls with value dispid: %08x", pAVExpr->GetDISPID(), pAVExpr->GetLPWSTR(), pAV->GetDISPID()));
                                }
                            }
#endif
                            pAA->FindSimpleAndDelete(pAV->GetDISPID(), CAttrValue::AA_Expression);
                        }

                    }
                }
                if ( pfContainsImportant && pAV->IsImportant())
                    *pfContainsImportant = TRUE;

            }
            else if(fApplyExpandos && passType != APPLY_ImportantOnly && (pAV->AAType() == CAttrValue::AA_Expando))
            {
                CAttrArray * pAA = pCFI->GetAAExpando();
                hr = THR(CAttrArray::Set(&pAA, pAV->GetDISPID(), &varVal, NULL, CAttrValue::AA_Expando));
                if(hr)
                    break;
            }
            else if (fApplyExpandos && passType != APPLY_ImportantOnly && (pAV->AAType() == CAttrValue::AA_Expression))
            {

                TraceTag((tagRecalcStyle, "ApplyAttrArrayValues: cascading expression %08x '%ls'", pAV->GetDISPID(), pAV->GetLPWSTR()));
                CAttrArray *pAA = pCFI->GetAAExpando();
                hr = THR(CAttrArray::Set(&pAA, pAV->GetDISPID(), &varVal, NULL, CAttrValue::AA_Expression));
                if (hr)
                    break;
                pCFI->PrepareFancyFormat();
                pCFI->_ff()._fHasExpressions = TRUE;
                pCFI->UnprepareForDebug();
            }
        } // eo for ( i = 0 ; i < (*ppAA) -> Size() ; i++, pAV++ )
    } // eo if (passType != APPLY_Init)


    RRETURN(hr);
}

// Convert attribute to style list types.
static styleListStyleType ListTypeToStyle (htmlListType type)
{
    styleListStyleType retType;
    switch (type)
    {
        default:
        case htmlListTypeNotSet:
            retType = styleListStyleTypeNotSet;
            break;
        case htmlListTypeLargeAlpha:
            retType = styleListStyleTypeUpperAlpha;
            break;
        case htmlListTypeSmallAlpha:
            retType = styleListStyleTypeLowerAlpha;
            break;
        case htmlListTypeLargeRoman:
            retType = styleListStyleTypeUpperRoman;
            break;
        case htmlListTypeSmallRoman:
            retType = styleListStyleTypeLowerRoman;
            break;
        case htmlListTypeNumbers:
            retType = styleListStyleTypeDecimal;
            break;
        case htmlListTypeDisc:
            retType = styleListStyleTypeDisc;
            break;
        case htmlListTypeCircle:
            retType = styleListStyleTypeCircle;
            break;
        case htmlListTypeSquare:
            retType = styleListStyleTypeSquare;
            break;
    }
    return retType;
}

//+---------------------------------------------------------------------------
//
// Helper class: CUrlStringIterator
//
//----------------------------------------------------------------------------

class CUrlStringIterator
{
public:

    CUrlStringIterator();
    ~CUrlStringIterator();
    void            Init(LPTSTR pch);
    void            Unroll();
    void            Step();
    inline BOOL     IsEnd()   { return 0 == _pchStart[0]; };
    inline BOOL     IsError() { return S_OK != _hr; }
    inline LPTSTR   Current() { return _pchStart; };

    HRESULT _hr;
    LPTSTR  _pch;
    LPTSTR  _pchStart;
    LPTSTR  _pchEnd;
    TCHAR   _chEnd;
};

CUrlStringIterator::CUrlStringIterator()
{
    memset (this, 0, sizeof(*this));
}

CUrlStringIterator::~CUrlStringIterator()
{
    Unroll();
}

void
CUrlStringIterator::Init(LPTSTR pch)
{
    _hr = S_OK;
    _pch = pch;

    Assert (_pch);

    Step();
}

void
CUrlStringIterator::Unroll()
{
    if (_pchEnd)
    {
        *_pchEnd = _chEnd;
        _pchEnd =  NULL;
    }
}

void
CUrlStringIterator::Step()
{
    Unroll();

    //
    // CONSIDER (alexz) using a state machine instead
    //

    _pchStart = _pch;

    // skip spaces or commas
    while (_istspace(*_pchStart) || _T(',') == *_pchStart)
        _pchStart++;

    if (0 == *_pchStart)
        return;

    // verify presence of "URL" prefix
    if (0 != StrCmpNIC(_pchStart, _T("URL"), 3))
        goto Error;

    _pchStart += 3; // step past "URL"

    // skip spaces between "URL" and "("
    while (_istspace(*_pchStart))
        _pchStart++;

    // verify presence of "("
    if (_T('(') != *_pchStart)
        goto Error;

    _pchStart++; // step past "("

    // skip spaces following "("
    while (_istspace(*_pchStart))
        _pchStart++;

    // verify that not end yet
    if (0 == *_pchStart)
        goto Error;

    // if quoted string...
    if (_T('\'') == *_pchStart || _T('"') == *_pchStart)
    {
        TCHAR       chClosing;

        chClosing = *_pchStart;
        _pchStart++;

        _pch = _pchStart;

        while (0 != *_pch && chClosing != *_pch)
            _pch++;

        // verify that not end yet
        if (0 == *_pch)
            goto Error;

        _pchEnd = _pch;

        _pch++; // step past quote
    }
    else // if not quoted
    {
        _pch = _pchStart;

        // scan to end, ")" or space
        while (0 != *_pch && _T(')') != *_pch && !_istspace(*_pch))
            _pch++;

        // verify that not end yet
        if (0 == *_pch)
            goto Error;

        _pchEnd = _pch;
    }

    // skip spaces
    while (_istspace(*_pch))
        _pch++;

    if (_T(')') != *_pch)
        goto Error;

    _pch++;

    // null-terminate current url (Unroll restores it)
    _chEnd = *_pchEnd;
    *_pchEnd = 0;

    return;

Error:
    _pchEnd = NULL;
    _hr = E_INVALIDARG;
    return;
}

//+---------------------------------------------------------------------------
//
// Helper:      ApplyBehaviorProperty
//
//----------------------------------------------------------------------------

HRESULT
ApplyBehaviorProperty (
    CAttrArray *        pAA,
    CBehaviorInfo *     pInfo,
    CachedStyleSheet *  pSheetCache)
{
    HRESULT             hr = S_OK;
    LPTSTR              pchUrl;
    CAttrValue *        pAV;
    AAINDEX             aaIdx = AA_IDX_UNKNOWN;
    CUrlStringIterator  iterator;

    pAV = pAA->Find(DISPID_A_BEHAVIOR, CAttrValue::AA_Attribute, &aaIdx);
    if (!pAV)
        goto Cleanup;

    pInfo->_acstrBehaviorUrls.Free();

    Assert (VT_LPWSTR == pAV->GetAVType());

    pchUrl = pAV->GetString();
    if (!pchUrl || !pchUrl[0])
        goto Cleanup;

    iterator.Init(pchUrl);
    while (!iterator.IsEnd() && !iterator.IsError())
    {
        hr = THR(pInfo->_acstrBehaviorUrls.AddNameToAtomTable(iterator.Current(), NULL));
        if (hr)
            goto Cleanup;

        iterator.Step();
    }

Cleanup:
    RRETURN (hr);
}

//+---------------------------------------------------------------------------
//
// Helper:      ApplyFormatInfoProperty
//
//----------------------------------------------------------------------------

HRESULT
ApplyFormatInfoProperty (
    const PROPERTYDESC * pPropertyDesc,
    DISPID dispID,
    VARIANT varValue,
    CFormatInfo *pCFI,
    CachedStyleSheet *pSheetCache )
{
    HRESULT hr = S_OK;

    Assert(pCFI && pCFI->_pNodeContext);
    CElement * pElem = pCFI->_pNodeContext->Element();

    BOOL fDesignMode        = pElem->Doc()->_fDesignMode;

    switch ( dispID )
    {
    case STDPROPID_XOBJ_HEIGHT:
        {
            CUnitValue *pcuv = (CUnitValue *)&V_I4(&varValue);

            if (!pcuv->IsNull())
            {
                pCFI->PrepareFancyFormat();
                pCFI->_ff()._cuvHeight = *pcuv;
                pCFI->_ff()._fHeightPercent |= (pcuv->GetUnitType() == CUnitValue::UNIT_PERCENT);
                pCFI->UnprepareForDebug();
            }
        }
        break;
    case STDPROPID_XOBJ_WIDTH:
        {
            CUnitValue *pcuv = (CUnitValue *)&V_I4(&varValue);
            if ( !pcuv->IsNull())
            {
                pCFI->PrepareFancyFormat();
                pCFI->_ff()._cuvWidth = *pcuv;
                pCFI->_ff()._fWidthPercent  = (pcuv->GetUnitType() == CUnitValue::UNIT_PERCENT);
                pCFI->UnprepareForDebug();
            }
        }
        break;
    case STDPROPID_XOBJ_TOP:
        {
            CUnitValue *pcuv = (CUnitValue *)&V_I4(&varValue);
            if (!pcuv->IsNull())
            {
                pCFI->PrepareFancyFormat();
                pCFI->_ff()._cuvTop = *pcuv;
                pCFI->_ff()._fHeightPercent |= (pcuv->GetUnitType() == CUnitValue::UNIT_PERCENT);
                pCFI->UnprepareForDebug();
            }
        }
        break;
    case STDPROPID_XOBJ_BOTTOM:
        {
            CUnitValue *pcuv = (CUnitValue *)&V_I4(&varValue);
            if (!pcuv->IsNull())
            {
                pCFI->PrepareFancyFormat();
                pCFI->_ff()._cuvBottom = *pcuv;
                pCFI->_ff()._fHeightPercent = TRUE;
                pCFI->UnprepareForDebug();
            }
        }
        break;
    case STDPROPID_XOBJ_LEFT:
        {
            CUnitValue *pcuv = (CUnitValue *)&V_I4(&varValue);
            if (!pcuv->IsNull())
            {
                pCFI->PrepareFancyFormat();
                pCFI->_ff()._cuvLeft = *pcuv;
                pCFI->UnprepareForDebug();
            }
        }
        break;
    case STDPROPID_XOBJ_RIGHT:
        {
            CUnitValue *pcuv = (CUnitValue *)&V_I4(&varValue);
            if (!pcuv->IsNull())
            {
                pCFI->PrepareFancyFormat();
                pCFI->_ff()._cuvRight = *pcuv;
                pCFI->UnprepareForDebug();
            }
        }
        break;

    case DISPID_A_VERTICALALIGN:
        ApplyVerticalAlignment( pCFI, (CUnitValue *) &V_I4(&varValue), pElem );
        break;

    case STDPROPID_XOBJ_CONTROLALIGN:
        {
            // BUGBUG (yinxie) with input morphing, all inputs has align attr
            // we need to find a better way to block this
            BOOL fIsInputNotImage   = (pElem->Tag() == ETAG_INPUT 
                                    && DYNCAST(CInput, pElem)->GetType() != htmlInputImage);
            if (fIsInputNotImage)
               break;
            pCFI->_fCtrlAlignLast = TRUE;
            ApplySiteAlignment(pCFI, (htmlControlAlign) V_I4(&varValue), pElem);
        }
        break;

    case DISPID_A_LISTTYPE:
        {   // This code treads a careful line with Nav3/Nav4/IE3 compat.  Before changing it,
            // please consult CWilso and/or AryeG.
            pCFI->PrepareFancyFormat();
            pCFI->_ff()._ListType = ListTypeToStyle ((htmlListType)V_I4(&varValue));
            if ( pElem->Tag() == ETAG_LI )
            {   // LIs inside OLs can't be converted to ULs via attribute, or vice versa.
                switch ( pCFI->_ff()._ListType )
                {
                case styleListStyleTypeSquare:
                case styleListStyleTypeCircle:
                case styleListStyleTypeDisc:
                    if (pCFI->_ppf->_cListing.GetType() == CListing::NUMBERING)
                        pCFI->_ff()._ListType = styleListStyleTypeDecimal;
                    break;
                case styleListStyleTypeLowerRoman:
                case styleListStyleTypeUpperRoman:
                case styleListStyleTypeLowerAlpha:
                case styleListStyleTypeUpperAlpha:
                case styleListStyleTypeDecimal:
                    if (pCFI->_ppf->_cListing.GetType() == CListing::BULLET)
                        pCFI->_ff()._ListType = styleListStyleTypeDisc;
                    break;
                }
            }
            pCFI->UnprepareForDebug();
        }
        break;

    case DISPID_A_LISTSTYLETYPE:
        pCFI->PrepareFancyFormat();
        pCFI->_ff()._ListType = (styleListStyleType)V_I4(&varValue);
        pCFI->UnprepareForDebug();
        break;

    case DISPID_A_LISTSTYLEPOSITION:
        pCFI->PrepareParaFormat();
        pCFI->_pf()._bListPosition = BYTE(V_I4(&varValue));
        pCFI->UnprepareForDebug();
        break;

    case DISPID_A_BACKGROUNDIMAGE:
        if (pCFI->_fAlwaysUseMyColors)
            break;
        // fall through!
    case DISPID_A_LISTSTYLEIMAGE:
        {
            TCHAR * pchImageURL = NULL;
            TCHAR * pchImageURLToSet;

            Assert ( varValue.vt == VT_LPWSTR );

            // If the sheet this attribute is in has an absolute URL then use this
            // url as the base url to construct the relative URL passed in.
            if ( varValue.byref && ((LPTSTR)varValue.byref) [ 0 ] )
            {
                TCHAR *pchAbsURL = pSheetCache
                                   ? pSheetCache->GetBaseURL()
                                   : NULL;

                if (pchAbsURL)
                {
                    hr = THR( ExpandUrlWithBaseUrl(pchAbsURL,
                                                   (LPTSTR)varValue.byref,
                                                   &pchImageURL) );

                    // E_POINTER implies that there was a problem with the URL.
                    // We don't want to set the format, but we also don't want
                    // to propagate the HRESULT to ApplyFormat, as this would
                    // cause the format caches not to be created. (cthrash)

                    if (hr)
                    {
                        hr = (hr == E_POINTER) ? S_OK : hr;
                        break;
                    }
                }
            }

            pchImageURLToSet = pchImageURL
                               ? pchImageURL
                               : (TCHAR *)varValue.byref;

            if (DISPID_A_BACKGROUNDIMAGE == dispID)
            {
                pCFI->_cstrBgImgUrl.Set(pchImageURLToSet);
            }
            else
            {
                pCFI->_cstrLiImgUrl.Set(pchImageURLToSet);
            }

            MemFreeString(pchImageURL);
        }
        break;

    case DISPID_A_BORDERTOPSTYLE:
        pCFI->PrepareFancyFormat();
        if ( ConvertCSSToFmBorderStyle( V_I4(&varValue), &pCFI->_ff()._bBorderStyles[BORDER_TOP] ))
        {
            pCFI->_ff()._fOverrideTablewideBorderDefault = TRUE;
            pCFI->_fPadBord = TRUE;
            // BUGBUG (donmarsh) -- no double borders on BODY, because the transparent area
            // is difficult to render correctly without flashing
            if (pCFI->_ff()._bBorderStyles[BORDER_TOP] == fmBorderStyleDouble &&
                pElem->Tag() == ETAG_BODY)
            {
                pCFI->_ff()._bBorderStyles[BORDER_TOP] = fmBorderStyleSingle;
            }
        }
        pCFI->UnprepareForDebug();
        break;
    case DISPID_A_BORDERRIGHTSTYLE:
        pCFI->PrepareFancyFormat();
        if ( ConvertCSSToFmBorderStyle( V_I4(&varValue), &pCFI->_ff()._bBorderStyles[BORDER_RIGHT] ))
        {
            pCFI->_ff()._fOverrideTablewideBorderDefault = TRUE;
            pCFI->_fPadBord = TRUE;
            // BUGBUG (donmarsh) -- no double borders on BODY, because the transparent area
            // is difficult to render correctly without flashing
            if (pCFI->_ff()._bBorderStyles[BORDER_RIGHT] == fmBorderStyleDouble &&
                pElem->Tag() == ETAG_BODY)
            {
                pCFI->_ff()._bBorderStyles[BORDER_RIGHT] = fmBorderStyleSingle;
            }
        }
        pCFI->UnprepareForDebug();
        break;
    case DISPID_A_BORDERBOTTOMSTYLE:
        pCFI->PrepareFancyFormat();
        if ( ConvertCSSToFmBorderStyle( V_I4(&varValue), &pCFI->_ff()._bBorderStyles[BORDER_BOTTOM] ))
        {
            pCFI->_ff()._fOverrideTablewideBorderDefault = TRUE;
            pCFI->_fPadBord = TRUE;
            // BUGBUG (donmarsh) -- no double borders on BODY, because the transparent area
            // is difficult to render correctly without flashing
            if (pCFI->_ff()._bBorderStyles[BORDER_BOTTOM] == fmBorderStyleDouble &&
                pElem->Tag() == ETAG_BODY)
            {
                pCFI->_ff()._bBorderStyles[BORDER_BOTTOM] = fmBorderStyleSingle;
            }
        }
        pCFI->UnprepareForDebug();
        break;
    case DISPID_A_BORDERLEFTSTYLE:
        pCFI->PrepareFancyFormat();
        if ( ConvertCSSToFmBorderStyle( V_I4(&varValue), &pCFI->_ff()._bBorderStyles[BORDER_LEFT] ))
        {
            pCFI->_ff()._fOverrideTablewideBorderDefault = TRUE;
            pCFI->_fPadBord = TRUE;
            // BUGBUG (donmarsh) -- no double borders on BODY, because the transparent area
            // is difficult to render correctly without flashing
            if (pCFI->_ff()._bBorderStyles[BORDER_LEFT] == fmBorderStyleDouble &&
                pElem->Tag() == ETAG_BODY)
            {
                pCFI->_ff()._bBorderStyles[BORDER_LEFT] = fmBorderStyleSingle;
            }
        }
        pCFI->UnprepareForDebug();
        break;

    case DISPID_A_MARGINTOP:
        {
            CUnitValue *pcuv = (CUnitValue *)&V_I4(&varValue);
            if (!pcuv->IsNull())
            {
                pCFI->PrepareFancyFormat();
                pCFI->_ff()._cuvMarginTop = *pcuv;
                pCFI->_ff()._fHasMargins = TRUE;
                pCFI->_ff()._fExplicitTopMargin = TRUE;
                pCFI->UnprepareForDebug();
            }
        }
        break;
    case DISPID_A_MARGINBOTTOM:
        {
            CUnitValue *pcuv = (CUnitValue *)&V_I4(&varValue);
            if (!pcuv->IsNull())
            {
                pCFI->PrepareFancyFormat();
                pCFI->_ff()._cuvMarginBottom = *pcuv;
                pCFI->_ff()._fHasMargins = TRUE;
                pCFI->_ff()._fExplicitBottomMargin = TRUE;
                pCFI->UnprepareForDebug();
            }
        }
        break;

    case DISPID_A_MARGINLEFT:
    {
        CUnitValue *pcuv = (CUnitValue *)&V_I4(&varValue);
        if (!pcuv->IsNull())
        {
            pCFI->PrepareFancyFormat();
            pCFI->_ff()._cuvMarginLeft = *pcuv;
            pCFI->_ff()._fHasMargins = TRUE;
            pCFI->UnprepareForDebug();
        }
    }
    break;
    case DISPID_A_MARGINRIGHT:
    {
        CUnitValue *pcuv = (CUnitValue *)&V_I4(&varValue);
        if (!pcuv->IsNull())
        {
            pCFI->PrepareFancyFormat();
            pCFI->_ff()._cuvMarginRight = *pcuv;
            pCFI->_ff()._fHasMargins = TRUE;
            pCFI->UnprepareForDebug();
        }
    }
    break;

    case DISPID_A_CLEAR:
        ApplyClear(pElem, (htmlClear) V_I4(&varValue), pCFI);
        break;

    case DISPID_A_PAGEBREAKBEFORE:
        if (V_I4(&varValue))
        {
            pCFI->PrepareFancyFormat();
            SET_PGBRK_BEFORE(pCFI->_ff()._bPageBreaks,V_I4(&varValue));

            if (!!GET_PGBRK_BEFORE(pCFI->_ff()._bPageBreaks) && pElem->Doc()->IsPrintDoc())
            {
                DYNCAST(CPrintDoc, pElem->Doc()->GetRootDoc())->_fHasPageBreaks = TRUE;
            }

            pCFI->UnprepareForDebug();
        }
        break;
    case DISPID_A_PAGEBREAKAFTER:
        if (V_I4(&varValue))
        {
            pCFI->PrepareFancyFormat();
            SET_PGBRK_AFTER(pCFI->_ff()._bPageBreaks,V_I4(&varValue));

            if (!!GET_PGBRK_AFTER(pCFI->_ff()._bPageBreaks) && pElem->Doc()->IsPrintDoc())
            {
                DYNCAST(CPrintDoc, pElem->Doc()->GetRootDoc())->_fHasPageBreaks = TRUE;
            }

            pCFI->UnprepareForDebug();
        }
        break;

    case DISPID_A_COLOR:
        if (!pCFI->_fAlwaysUseMyColors)
        {
            CColorValue *pccv = (CColorValue *)&V_I4(&varValue);
            if ( !pccv->IsNull() )
            {
                pCFI->PrepareCharFormat();
                pCFI->_cf()._ccvTextColor = *pccv;
                pCFI->UnprepareForDebug();
            }
        }
        break;

    case DISPID_A_DISPLAY:
        // We can only display this element if the parent is not display:none.
        pCFI->PrepareFancyFormat();
        pCFI->_ff()._bDisplay = V_I4(&varValue);
        if (!fDesignMode)
            pCFI->_fDisplayNone = !!(pCFI->_ff()._bDisplay == styleDisplayNone);
        pCFI->UnprepareForDebug();
        break;

    case DISPID_A_VISIBILITY:
        if (!g_fInWin98Discover ||
            !pCFI->_pcfSrc->_fVisibilityHidden)
        {
            pCFI->PrepareFancyFormat();
            pCFI->_ff()._bVisibility = V_I4(&varValue);

            if (!fDesignMode)
            {
                if (pCFI->_pff->_bVisibility == styleVisibilityHidden)
                    pCFI->_fVisibilityHidden = TRUE;
                else if (pCFI->_pff->_bVisibility == styleVisibilityVisible)
                    pCFI->_fVisibilityHidden = FALSE;
                else if (pCFI->_pff->_bVisibility == styleVisibilityInherit)
                    pCFI->_fVisibilityHidden = pCFI->_pcfSrc->_fVisibilityHidden;
            }
            pCFI->UnprepareForDebug();
        }

        break;

    case DISPID_A_IMEMODE:
    case DISPID_A_RUBYOVERHANG:
    case DISPID_A_RUBYALIGN:
        if(pCFI->_eExtraValues & ComputeFormatsType_GetValue)
        {
            *pCFI->_pvarExtraValue = varValue;
            break;
        }

        break;

    case DISPID_A_RUBYPOSITION:
        if(pCFI->_eExtraValues & ComputeFormatsType_GetValue)
        {
            *pCFI->_pvarExtraValue = varValue;
        }
        if(pElem->Tag() == ETAG_RUBY)
        {
            pCFI->PrepareCharFormat();
            pCFI->_cf()._fIsRuby = (styleRubyPositionInline != V_I4(&varValue));
            pCFI->UnprepareForDebug();
        }
        break;

    case DISPID_A_TEXTTRANSFORM:
        {
            if ( styleTextTransformNotSet != V_I4(&varValue) )
            {
                pCFI->PrepareCharFormat();
                pCFI->_cf()._bTextTransform = (BYTE)V_I4(&varValue);
                pCFI->UnprepareForDebug();
            }
        }
        break;


    case DISPID_A_LETTERSPACING:
        {
            CUnitValue *puv = (CUnitValue *)&V_I4(&varValue);
            if ( !puv->IsNull() )
            {
                pCFI->PrepareCharFormat();
                pCFI->_cf()._cuvLetterSpacing = *puv;
                pCFI->UnprepareForDebug();
            }
        }
        break;

    case DISPID_A_OVERFLOWX:
        if ( V_I4(&varValue) != styleOverflowNotSet )
        {
            pCFI->PrepareFancyFormat();
            pCFI->_ff()._bOverflowX = (BYTE)V_I4(&varValue);
            pCFI->UnprepareForDebug();
        }
        break;

    case DISPID_A_OVERFLOWY:
        if ( V_I4(&varValue) != styleOverflowNotSet )
        {
            pCFI->PrepareFancyFormat();
            pCFI->_ff()._bOverflowY = (BYTE)V_I4(&varValue);
            pCFI->UnprepareForDebug();
        }
        break;

    case DISPID_A_OVERFLOW:
        if ( V_I4(&varValue) != styleOverflowNotSet )
        {
            pCFI->PrepareFancyFormat();
            pCFI->_ff()._bOverflowX = (BYTE)V_I4(&varValue);
            pCFI->_ff()._bOverflowY = (BYTE)V_I4(&varValue);
            pCFI->UnprepareForDebug();
        }
        break;

    case DISPID_A_PADDINGTOP:
        {
            CUnitValue *puv = (CUnitValue *)&V_I4(&varValue);
            if (!puv->IsNull())
            {
                pCFI->PrepareFancyFormat();
                pCFI->_ff()._cuvPaddingTop = *puv;

                if (puv->IsPercent())
                {
                    pCFI->_ff()._fPercentVertPadding = TRUE;
                }

                pCFI->PrepareParaFormat();
                pCFI->_pf()._fPadBord = TRUE;   // Apply directly to PF & skip CFI
                pCFI->UnprepareForDebug();
            }
        }
        break;

    case DISPID_A_PADDINGRIGHT:
        {
            CUnitValue *puv = (CUnitValue *)&V_I4(&varValue);
            if (!puv->IsNull())
            {
                pCFI->PrepareFancyFormat();
                pCFI->_ff()._cuvPaddingRight = *puv;

                if (puv->IsPercent())
                {
                    pCFI->_ff()._fPercentHorzPadding = TRUE;
                }

                pCFI->PrepareParaFormat();
                pCFI->_pf()._fPadBord = TRUE;   // Apply directly to PF & skip CFI
                pCFI->UnprepareForDebug();
            }
        }
        break;

    case DISPID_A_PADDINGBOTTOM:
        {
            CUnitValue *puv = (CUnitValue *)&V_I4(&varValue);
            if (!puv->IsNull())
            {
                pCFI->PrepareFancyFormat();
                pCFI->_ff()._cuvPaddingBottom = *puv;

                if (puv->IsPercent())
                {
                    pCFI->_ff()._fPercentVertPadding = TRUE;
                }

                pCFI->PrepareParaFormat();
                pCFI->_pf()._fPadBord = TRUE;   // Apply directly to PF & skip CFI
                pCFI->UnprepareForDebug();
            }
        }
        break;

    case DISPID_A_PADDINGLEFT:
        {
            CUnitValue *puv = (CUnitValue *)&V_I4(&varValue);
            if (!puv->IsNull())
            {
                pCFI->PrepareFancyFormat();
                pCFI->_ff()._cuvPaddingLeft = *puv;

                if (puv->IsPercent())
                {
                    pCFI->_ff()._fPercentHorzPadding = TRUE;
                }

                pCFI->PrepareParaFormat();
                pCFI->_pf()._fPadBord = TRUE;   // Apply directly to PF & skip CFI
                pCFI->UnprepareForDebug();
            }
        }
        break;

    case DISPID_A_TABLEBORDERCOLOR:
        if (!pCFI->_fAlwaysUseMyColors)
        {
            CColorValue *pccv = (CColorValue *)&V_I4(&varValue);
            if ( !pccv->IsNull() )
            {
                pCFI->PrepareFancyFormat();
                pCFI->_ff()._fOverrideTablewideBorderDefault = TRUE;
                pCFI->_ff()._ccvBorderColors[BORDER_TOP] =
                    pCFI->_ff()._ccvBorderColors[BORDER_RIGHT] =
                    pCFI->_ff()._ccvBorderColors[BORDER_BOTTOM] =
                    pCFI->_ff()._ccvBorderColors[BORDER_LEFT] = *pccv;
                pCFI->UnprepareForDebug();
            }
        }
        break;

    case DISPID_A_TABLEBORDERCOLORLIGHT:
        if (!pCFI->_fAlwaysUseMyColors)
        {
            CColorValue *pccv = (CColorValue *)&V_I4(&varValue);
            if ( !pccv->IsNull() )
            {
                pCFI->PrepareFancyFormat();
                pCFI->_ff()._fOverrideTablewideBorderDefault = TRUE;
                pCFI->_ff()._ccvBorderColorLight = *pccv;
                pCFI->UnprepareForDebug();
            }
        }
        break;

    case DISPID_A_TABLEBORDERCOLORDARK:
        if (!pCFI->_fAlwaysUseMyColors)
        {
            CColorValue *pccv = (CColorValue *)&V_I4(&varValue);
            if ( !pccv->IsNull() )
            {
                pCFI->PrepareFancyFormat();
                pCFI->_ff()._fOverrideTablewideBorderDefault = TRUE;
                pCFI->_ff()._ccvBorderColorDark = *pccv;
                pCFI->UnprepareForDebug();
            }
        }
        break;

    case DISPID_A_BORDERTOPCOLOR:
        if (!pCFI->_fAlwaysUseMyColors)
        {
            CColorValue *pccv = (CColorValue *)&V_I4(&varValue);
            if ( !pccv->IsNull() )
            {
                pCFI->PrepareFancyFormat();
                pCFI->_ff()._fOverrideTablewideBorderDefault = TRUE;
                pCFI->_ff()._ccvBorderColors[BORDER_TOP] = *pccv;
                SETBORDERSIDECLRUNIQUE( (&pCFI->_ff()), BORDER_TOP );
                pCFI->_fPadBord = TRUE;
                pCFI->UnprepareForDebug();
            }
        }
        break;


    case DISPID_A_BORDERRIGHTCOLOR:
        if (!pCFI->_fAlwaysUseMyColors)
        {
            CColorValue *pccv = (CColorValue *)&V_I4(&varValue);
            if ( !pccv->IsNull() )
            {
                pCFI->PrepareFancyFormat();
                pCFI->_ff()._fOverrideTablewideBorderDefault = TRUE;
                pCFI->_ff()._ccvBorderColors[BORDER_RIGHT] = *pccv;
                SETBORDERSIDECLRUNIQUE( (&pCFI->_ff()), BORDER_RIGHT );
                pCFI->_fPadBord = TRUE;
                pCFI->UnprepareForDebug();
            }
        }
        break;


    case DISPID_A_BORDERBOTTOMCOLOR:
        if (!pCFI->_fAlwaysUseMyColors)
        {
            CColorValue *pccv = (CColorValue *)&V_I4(&varValue);
            if ( !pccv->IsNull() )
            {
                pCFI->PrepareFancyFormat();
                pCFI->_ff()._fOverrideTablewideBorderDefault = TRUE;
                pCFI->_ff()._ccvBorderColors[BORDER_BOTTOM] = *pccv;
                SETBORDERSIDECLRUNIQUE( (&pCFI->_ff()), BORDER_BOTTOM );
                pCFI->_fPadBord = TRUE;
                pCFI->UnprepareForDebug();
            }
        }
        break;

    case DISPID_A_BORDERLEFTCOLOR:
        if (!pCFI->_fAlwaysUseMyColors)
        {
            CColorValue *pccv = (CColorValue *)&V_I4(&varValue);
            if ( !pccv->IsNull() )
            {
                pCFI->PrepareFancyFormat();
                pCFI->_ff()._fOverrideTablewideBorderDefault = TRUE;
                pCFI->_ff()._ccvBorderColors[BORDER_LEFT] = *pccv;
                SETBORDERSIDECLRUNIQUE( (&pCFI->_ff()), BORDER_LEFT );
                pCFI->_fPadBord = TRUE;
                pCFI->UnprepareForDebug();
            }
        }
        break;


    case DISPID_A_BORDERTOPWIDTH:
        {
            CUnitValue *puv = (CUnitValue *)&V_I4(&varValue);
            if ( !puv->IsNull() )
            {
                pCFI->PrepareFancyFormat();
                pCFI->_ff()._cuvBorderWidths[BORDER_TOP] = *puv;
                pCFI->_ff()._fOverrideTablewideBorderDefault = TRUE;
                pCFI->_fPadBord = TRUE;
                pCFI->UnprepareForDebug();
            }
        }
        break;

    case DISPID_A_BORDERRIGHTWIDTH:
        {
            CUnitValue *puv = (CUnitValue *)&V_I4(&varValue);
            if ( !puv->IsNull() )
            {
                pCFI->PrepareFancyFormat();
                pCFI->_ff()._cuvBorderWidths[BORDER_RIGHT] = *puv;
                pCFI->_ff()._fOverrideTablewideBorderDefault = TRUE;
                pCFI->_fPadBord = TRUE;
                pCFI->UnprepareForDebug();
            }
        }
        break;

    case DISPID_A_BORDERBOTTOMWIDTH:
        {
            CUnitValue *puv = (CUnitValue *)&V_I4(&varValue);
            if ( !puv->IsNull() )
            {
                pCFI->PrepareFancyFormat();
                pCFI->_ff()._cuvBorderWidths[BORDER_BOTTOM] = *puv;
                pCFI->_ff()._fOverrideTablewideBorderDefault = TRUE;
                pCFI->_fPadBord = TRUE;
                pCFI->UnprepareForDebug();
            }
        }
        break;

    case DISPID_A_BORDERLEFTWIDTH:
        {
            CUnitValue *puv = (CUnitValue *)&V_I4(&varValue);
            if ( !puv->IsNull() )
            {
                pCFI->PrepareFancyFormat();
                pCFI->_ff()._cuvBorderWidths[BORDER_LEFT] = *puv;
                pCFI->_ff()._fOverrideTablewideBorderDefault = TRUE;
                pCFI->_fPadBord = TRUE;
                pCFI->UnprepareForDebug();
            }
        }
        break;

    case DISPID_A_POSITION:
        if (V_I4(&varValue) != stylePositionNotSet )
        {
            pCFI->PrepareFancyFormat();
            pCFI->_ff()._bPositionType = (BYTE)V_I4(&varValue);
            pCFI->_ff()._fRelative = (pCFI->_ff()._bPositionType == stylePositionrelative);
            pCFI->_fRelative = pCFI->_ff()._fRelative;
            pCFI->UnprepareForDebug();

            #if DBG==1
            if(pCFI->_pff->_bPositionType != stylePositionstatic)
            {
                Assert(pElem->Doc()->_fRegionCollection &&
                        "Inconsistent _fRegionCollection flag, user style sheet sets the position?");
            }
            #endif
        }
        break;

    case DISPID_A_ZINDEX:
        pCFI->PrepareFancyFormat();
        pCFI->_ff()._lZIndex = V_I4(&varValue);
        pCFI->UnprepareForDebug();
        break;

    case DISPID_BACKCOLOR:
        if (!pCFI->_fAlwaysUseMyColors)
        {
            CColorValue *pccv = (CColorValue *)&V_I4(&varValue);
            if ( !pccv->IsNull())
            {
                BOOL fTransparent = (pccv->GetType()
                                    == CColorValue::TYPE_TRANSPARENT);

                pCFI->PrepareFancyFormat();
                if (fTransparent)
                {
                    //
                    // The assumption that ancestor's draw background color
                    // is not true for body. Body already inherits background
                    // color from HTML or option settings, so leave it
                    // alone if a transparent value is set.
                    //
                    if (pElem->Tag() != ETAG_BODY)
                        pCFI->_ff()._ccvBackColor.Undefine();
                }
                else
                {
                    pCFI->_ff()._ccvBackColor = *pccv;
                }
                pCFI->UnprepareForDebug();

                // site draw their own background, so we dont have
                // to inherit their background info
                pCFI->_fHasBgColor = !fTransparent;
            }
        }
        break;

    case DISPID_A_BACKGROUNDPOSX:
        {
            // Return the value if extra info is requested
            if(pCFI->_eExtraValues & ComputeFormatsType_GetValue)
            {
                *pCFI->_pvarExtraValue = varValue;
                break;
            }
            CUnitValue *cuv = (CUnitValue *)&V_I4(&varValue);
            if ( !cuv->IsNull() )
            {
                pCFI->PrepareFancyFormat();
                if ( cuv->GetUnitType() == CUnitValue::UNIT_ENUM )
                {
                    long lVal = 0;
                    switch ( cuv->GetUnitValue() )
                    {
                    //  styleBackgroundPositionXLeft - do nothing.
                    case styleBackgroundPositionXCenter:
                        lVal = 50;
                        break;
                    case styleBackgroundPositionXRight:
                        lVal = 100;
                        break;
                    }
                    pCFI->_ff()._cuvBgPosX.SetValue ( lVal * CUnitValue::TypeNames[CUnitValue::UNIT_PERCENT].wScaleMult, CUnitValue::UNIT_PERCENT );
                }
                else
                {
                    pCFI->_ff()._cuvBgPosX = *cuv;
                }
                pCFI->UnprepareForDebug();
            }
        }
        break;

    case DISPID_A_BACKGROUNDPOSY:
        {
            // Return the value if extra info is requested
            if(pCFI->_eExtraValues & ComputeFormatsType_GetValue)
            {
                *pCFI->_pvarExtraValue = varValue;
                break;
            }
            CUnitValue *pcuv = (CUnitValue *)&V_I4(&varValue);
            if ( !pcuv->IsNull() )
            {
                pCFI->PrepareFancyFormat();
                if ( pcuv->GetUnitType() == CUnitValue::UNIT_ENUM )
                {
                    long lVal = 0;
                    switch ( pcuv->GetUnitValue() )
                    {
                    //  styleBackgroundPositionXLeft - do nothing.
                    case styleBackgroundPositionYCenter:
                        lVal = 50;
                        break;
                    case styleBackgroundPositionYBottom:
                        lVal = 100;
                        break;
                    }
                    pCFI->_ff()._cuvBgPosY.SetValue ( lVal * CUnitValue::TypeNames[CUnitValue::UNIT_PERCENT].wScaleMult, CUnitValue::UNIT_PERCENT );
                }
                else
                {
                    pCFI->_ff()._cuvBgPosY = *pcuv;
                }
                pCFI->UnprepareForDebug();
            }
        }
        break;

    case DISPID_A_BACKGROUNDREPEAT:
        {
            const LONG lRepeat = V_I4(&varValue);

            pCFI->PrepareFancyFormat();
            pCFI->_ff()._fBgRepeatX = lRepeat == styleBackgroundRepeatRepeatX ||
                                      lRepeat == styleBackgroundRepeatRepeat;
            pCFI->_ff()._fBgRepeatY = lRepeat == styleBackgroundRepeatRepeatY ||
                                      lRepeat == styleBackgroundRepeatRepeat;
            pCFI->UnprepareForDebug();
        }
        break;

    case DISPID_A_BACKGROUNDATTACHMENT:
        pCFI->PrepareFancyFormat();
        pCFI->_ff()._fBgFixed = V_I4(&varValue) == styleBackgroundAttachmentFixed;
        pCFI->UnprepareForDebug();
        break;

    case DISPID_A_LANG:
        Assert ( varValue.vt == VT_LPWSTR );
        pCFI->PrepareCharFormat();
        ApplyLang(&pCFI->_cf(), (LPTSTR) varValue.byref);
        pCFI->UnprepareForDebug();
        break;

    case DISPID_A_FLOAT:
        if (V_I4(&varValue) != styleStyleFloatNotSet)
        {
            pCFI->PrepareFancyFormat();
            pCFI->_ff()._bStyleFloat = (BYTE)V_I4(&varValue);
            pCFI->UnprepareForDebug();
        }
        break;

    case DISPID_A_CLIPRECTTOP:
        {
           CUnitValue *puv = (CUnitValue *)&V_I4(&varValue);
           if ( !puv->IsNull() )
           {
               pCFI->PrepareFancyFormat();
               pCFI->_ff()._cuvClipTop = *puv;
               pCFI->UnprepareForDebug();
           }
        }
        break;

    case DISPID_A_CLIPRECTLEFT:
        {
           CUnitValue *puv = (CUnitValue *)&V_I4(&varValue);
           if ( !puv->IsNull() )
           {
               pCFI->PrepareFancyFormat();
               pCFI->_ff()._cuvClipLeft = *puv;
               pCFI->UnprepareForDebug();
           }
        }
        break;

    case DISPID_A_CLIPRECTRIGHT:
        {
           CUnitValue *puv = (CUnitValue *)&V_I4(&varValue);
           if ( !puv->IsNull() )
           {
               pCFI->PrepareFancyFormat();
               pCFI->_ff()._cuvClipRight = *puv;
               pCFI->UnprepareForDebug();
           }
        }
        break;

    case DISPID_A_CLIPRECTBOTTOM:
        {
           CUnitValue *puv = (CUnitValue *)&V_I4(&varValue);
           if ( !puv->IsNull() )
           {
               pCFI->PrepareFancyFormat();
               pCFI->_ff()._cuvClipBottom = *puv;
               pCFI->UnprepareForDebug();
           }
        }
        break;

    case DISPID_A_FILTER:
        {
            if (pCFI->_cstrFilters.Set(varValue.byref ? (LPTSTR)varValue.byref : NULL) == S_OK)
            {
                pCFI->PrepareFancyFormat();
                // BUGBUG: We really intend to cascade these together.
                pCFI->_ff()._pszFilters = pCFI->_cstrFilters;
                pCFI->UnprepareForDebug();
                pCFI->_fHasFilters = TRUE;
            }
        }
        break;

    case DISPID_A_TEXTINDENT:
        {
            CUnitValue *cuv = (CUnitValue *)&V_I4(&varValue);
            if ( !cuv->IsNull() )
            {
                pCFI->_cuvTextIndent.SetRawValue(V_I4(&varValue));
            }
        }
        break;

    case DISPID_A_TABLELAYOUT:
        // table-layout CSS2 attribute
        pCFI->PrepareFancyFormat();
        pCFI->_ff()._bTableLayout = V_I4(&varValue) == styleTableLayoutFixed;
        pCFI->UnprepareForDebug();
        break;

    case DISPID_A_BORDERCOLLAPSE:
        // border-collapse CSS2 attribute
        pCFI->PrepareFancyFormat();
        pCFI->_ff()._bBorderCollapse = V_I4(&varValue) == styleBorderCollapseCollapse;
        pCFI->UnprepareForDebug();
        break;

    case DISPID_A_DIR:
        {
            BOOL fRTL = FALSE;

            switch (V_I4(&varValue))
            {
            case htmlDirNotSet:
            case htmlDirLeftToRight:
                fRTL = FALSE;
                break;
            case htmlDirRightToLeft:
                fRTL = TRUE;
                break;
            default:
                Assert("Fix the .PDL");
                break;
            }

            pCFI->_fBidiEmbed = TRUE;

            pCFI->PrepareCharFormat();
            pCFI->_cf()._fRTL = fRTL;
            pCFI->UnprepareForDebug();
        }
        break;

    case DISPID_A_DIRECTION:
        {
            BOOL fRTL = FALSE;

            switch (V_I4(&varValue))
            {
            case styleDirLeftToRight:
                fRTL = FALSE;
                break;
            case styleDirRightToLeft:
                fRTL = TRUE;
                break;
            case styleDirNotSet:
            case styleDirInherit:
                // Use our parent element's value.
                fRTL = pCFI->_pcfSrc->_fRTL;
                break;
            default:
                Assert("Fix the .PDL");
                break;
            }

            pCFI->PrepareCharFormat();
            pCFI->_cf()._fRTL = fRTL;
            pCFI->UnprepareForDebug();
        }
        break;

    case DISPID_A_UNICODEBIDI:
        switch (V_I4(&varValue))
        {
        case styleBidiEmbed:
            pCFI->_fBidiEmbed = TRUE;
            pCFI->_fBidiOverride = FALSE;
            break;
        case styleBidiOverride:
            pCFI->_fBidiEmbed = TRUE;
            pCFI->_fBidiOverride = TRUE;
            break;
        case styleBidiNotSet:
        case styleBidiNormal:
            pCFI->_fBidiEmbed = FALSE;
            pCFI->_fBidiOverride = FALSE;
            break;
        case styleBidiInherit:
            pCFI->_fBidiEmbed = pCFI->_pcfSrc->_fBidiEmbed;
            pCFI->_fBidiOverride = pCFI->_pcfSrc->_fBidiOverride;
            break;
        }
        break;

    case DISPID_A_TEXTAUTOSPACE:
        pCFI->PrepareCharFormat();
        pCFI->_cf()._fTextAutospace = varValue.lVal;
        pCFI->UnprepareForDebug();
        break;

    case DISPID_A_LINEBREAK:
        if (V_I4(&varValue) != styleLineBreakNotSet)
        {
            pCFI->PrepareCharFormat();
            pCFI->_cf()._fLineBreakStrict = V_I4(&varValue) == styleLineBreakStrict ? TRUE : FALSE;
            pCFI->UnprepareForDebug();
        }
        break;

    case DISPID_A_WORDBREAK:
        if (V_I4(&varValue) != styleWordBreakNotSet)
        {
            Assert( V_I4(&varValue) >= 1 && V_I4(&varValue) <= 3 );
            pCFI->PrepareParaFormat();
            pCFI->_pf()._fWordBreak = V_I4(&varValue);
            pCFI->UnprepareForDebug();
        }
        break;
       
    case DISPID_A_TEXTJUSTIFY:
        pCFI->_uTextJustify = V_I4(&varValue);
        break;
       
    case DISPID_A_TEXTJUSTIFYTRIM:
        pCFI->_uTextJustifyTrim = V_I4(&varValue);
        break;

    case DISPID_A_TEXTKASHIDA:        
        pCFI->_cuvTextKashida.SetRawValue(V_I4(&varValue));
        break;

    case DISPID_A_NOWRAP:
        if ( V_I4(&varValue) )
        {
            pCFI->PrepareFancyFormat();
            pCFI->_ff()._fHasNoWrap = TRUE;
            pCFI->UnprepareForDebug();
            pCFI->_fNoBreak = TRUE;
        }
        break;


    case DISPID_A_TEXTDECORATION:
        {
            long lTDBits = V_I4(&varValue);

            pCFI->PrepareCharFormat();

            if ( lTDBits & TD_NONE )
            {
                pCFI->_cf()._fUnderline = pCFI->_cf()._fOverline =
                pCFI->_cf()._fStrikeOut = 0;
            }
            if ( lTDBits & TD_UNDERLINE)
            {
                if (!pCFI->_cf()._fAccelerator || !(pElem->Doc()->_wUIState & UISF_HIDEACCEL))
                    pCFI->_cf()._fUnderline = 1;
            }
            if ( lTDBits & TD_OVERLINE )
                pCFI->_cf()._fOverline = 1;
            if ( lTDBits & TD_LINETHROUGH )
                pCFI->_cf()._fStrikeOut = 1;

            pCFI->UnprepareForDebug();
        }
        break;

    case DISPID_A_ACCELERATOR:
        {
            BOOL fAccelerator = (V_I4(&varValue) == styleAcceleratorTrue);

            pCFI->PrepareCharFormat();

            pCFI->_cf()._fAccelerator = fAccelerator;
            if (fAccelerator)
            {
                if (pElem->Doc()->_wUIState & UISF_HIDEACCEL)
                    pCFI->_cf()._fUnderline = FALSE;
                else
                    pCFI->_cf()._fUnderline = TRUE;
            }

            pCFI->UnprepareForDebug();
        }
        break;

    case DISPID_A_FONT:
        {
            Esysfont eFontType = FindSystemFontByName( (LPTSTR)varValue.byref );
#ifdef WIN16
            // BUGWIN16: Need to implement this or make this a limitation.
            Assert(0);
#else
            if ( eFontType != sysfont_non_system )
            {
                pCFI->PrepareCharFormat();
                ApplySystemFont( &pCFI->_cf(), eFontType );
                pCFI->UnprepareForDebug();
            }
#endif //!WIN16
        }
        break;
    case DISPID_A_FONTSIZE:
        // Return the value if extra info is requested by currentStyle
        if(pCFI->_eExtraValues & ComputeFormatsType_GetValue)
        {
            *pCFI->_pvarExtraValue = varValue;
            break;
        }
        if (!pCFI->_fAlwaysUseMyFontSize)
        {
            pCFI->PrepareCharFormat();
            ApplyFontSize(&pCFI->_cf(), (CUnitValue*) (void*) &V_I4(&varValue), pCFI->_pNodeContext, pElem->Doc()->_pOptionSettings->fAlwaysUseMyFontSize);
            pCFI->UnprepareForDebug();
        }
        break;
        
    case DISPID_A_FONTSTYLE:
        if (pCFI->_eExtraValues & ComputeFormatsType_GetValue)
        {
            *pCFI->_pvarExtraValue = varValue;
            break;
        }
        pCFI->PrepareCharFormat();
        ApplyFontStyle(&pCFI->_cf(), (styleFontStyle) V_I4(&varValue));
        pCFI->UnprepareForDebug();
        // Save the value if extra info is requested
        break;
    case DISPID_A_FONTVARIANT:
        if (pCFI->_eExtraValues & ComputeFormatsType_GetValue)
        {
            *pCFI->_pvarExtraValue = varValue;
            break;
        }
        pCFI->PrepareCharFormat();
        if (V_I4(&varValue) == styleFontVariantSmallCaps)
        {
            pCFI->_cf()._bTextTransform = styleTextTransformUppercase;
            pCFI->_cf()._fBumpSizeDown = TRUE;
        }
        else    // "normal"
        {
            pCFI->_cf()._bTextTransform = styleTextTransformNone;
            pCFI->_cf()._fBumpSizeDown = FALSE;
        }
        pCFI->UnprepareForDebug();
        break;
    case DISPID_A_FONTFACE:
        // Return the unmodified value if requested for currentStyle
        if (pCFI->_eExtraValues & ComputeFormatsType_GetValue)
        {
            *pCFI->_pvarExtraValue = varValue;
            break;
        }
        if (!pCFI->_fAlwaysUseMyFontFace)
        {
            pCFI->PrepareCharFormat();
            ApplyFontFace(&pCFI->_cf(), (LPTSTR)V_BSTR(&varValue), pElem->Doc());
            pCFI->UnprepareForDebug();
        }
        break;
    case DISPID_A_BASEFONT:
        pCFI->PrepareCharFormat();
        ApplyBaseFont(&pCFI->_cf(), (long) V_I4(&varValue));
        pCFI->UnprepareForDebug();
        break;
    case DISPID_A_FONTWEIGHT:
        pCFI->PrepareCharFormat();
        ApplyFontWeight(&pCFI->_cf(), (styleFontWeight) V_I4(&varValue));
        pCFI->UnprepareForDebug();
        break;
    case DISPID_A_LINEHEIGHT:
        pCFI->PrepareCharFormat();
        ApplyLineHeight(&pCFI->_cf(), (CUnitValue*) &V_I4(&varValue));
        pCFI->UnprepareForDebug();
        break;
    case DISPID_A_TABLEVALIGN:
        pCFI->PrepareParaFormat();
        ApplyTableVAlignment(&pCFI->_pf(), (htmlCellVAlign) V_I4(&varValue) );
        pCFI->UnprepareForDebug();
        break;

    case STDPROPID_XOBJ_BLOCKALIGN:
        pCFI->_fCtrlAlignLast = FALSE;
        ApplyParagraphAlignment(pCFI, (htmlBlockAlign) V_I4(&varValue), pElem);
        break;

    case DISPID_A_CURSOR:
        pCFI->PrepareCharFormat();
        pCFI->_cf()._bCursorIdx = V_I4(&varValue);
        pCFI->UnprepareForDebug();
        break;
    case STDPROPID_XOBJ_DISABLED:
        if (V_BOOL(&varValue))
        {
            pCFI->PrepareCharFormat();
            pCFI->_cf()._fDisabled = V_BOOL(&varValue);
            pCFI->UnprepareForDebug();
        }
        break;
    case DISPID_A_LAYOUTGRIDCHAR:
        pCFI->PrepareParaFormat();
        pCFI->PrepareCharFormat();
        pCFI->_cf()._fHasGridValues = TRUE;
        pCFI->_pf()._cuvCharGridSizeInner = (CUnitValue)(V_I4(&varValue));

        // In case of change of layout-grid-char, layout-grid-mode must be updated 
        // if its value is not set. This helps handle default values.
        if (    pCFI->_cf()._uLayoutGridModeInner == styleLayoutGridModeNotSet
            ||  (   pCFI->_cf()._uLayoutGridModeInner & styleLayoutGridModeNone
                &&  pCFI->_cf()._uLayoutGridModeInner & styleLayoutGridModeBoth))
        {
            // Now _uLayoutGridModeInner can be one of { 000, 101, 110, 111 }
            // it means that layout-grid-mode is not set

            if (pCFI->_pf()._cuvCharGridSizeInner.IsNull())
            {   // clear deduced char mode
                pCFI->_cf()._uLayoutGridModeInner &= (styleLayoutGridModeNone | styleLayoutGridModeLine);
                if (pCFI->_cf()._uLayoutGridModeInner == styleLayoutGridModeNone)
                    pCFI->_cf()._uLayoutGridModeInner = styleLayoutGridModeNotSet;
            }
            else
            {   // set deduced char mode
                pCFI->_cf()._uLayoutGridModeInner |= (styleLayoutGridModeNone | styleLayoutGridModeChar);
            }
        }
        pCFI->UnprepareForDebug();
        break;

    case DISPID_A_LAYOUTGRIDLINE:
        pCFI->PrepareParaFormat();
        pCFI->PrepareCharFormat();
        pCFI->_cf()._fHasGridValues = TRUE;
        pCFI->_pf()._cuvLineGridSizeInner = (CUnitValue)(V_I4(&varValue));

        // In case of change of layout-grid-line, layout-grid-mode must be updated 
        // if its value is not set. This helps handle default values.
        if (    pCFI->_cf()._uLayoutGridModeInner == styleLayoutGridModeNotSet
            ||  (   pCFI->_cf()._uLayoutGridModeInner & styleLayoutGridModeNone
                &&  pCFI->_cf()._uLayoutGridModeInner & styleLayoutGridModeBoth))
        {
            // Now _uLayoutGridModeInner can be one of { 000, 101, 110, 111 }
            // it means that layout-grid-mode is not set

            if (pCFI->_pf()._cuvLineGridSizeInner.IsNull())
            {   // clear deduced line mode
                pCFI->_cf()._uLayoutGridModeInner &= (styleLayoutGridModeNone | styleLayoutGridModeChar);
                if (pCFI->_cf()._uLayoutGridModeInner == styleLayoutGridModeNone)
                    pCFI->_cf()._uLayoutGridModeInner = styleLayoutGridModeNotSet;
            }
            else
            {   // set deduced char mode
                pCFI->_cf()._uLayoutGridModeInner |= (styleLayoutGridModeNone | styleLayoutGridModeLine);
            }
        }
        pCFI->UnprepareForDebug();
        break;

    case DISPID_A_LAYOUTGRIDMODE:
        if (pCFI->_eExtraValues & ComputeFormatsType_GetValue)
        {
            *pCFI->_pvarExtraValue = varValue;
        }
        else
        {
            pCFI->PrepareParaFormat();
            pCFI->PrepareCharFormat();
            pCFI->_cf()._fHasGridValues = TRUE;
            pCFI->_cf()._uLayoutGridModeInner = V_I4(&varValue);

            // Handle default values of layout-grid-mode.
            if (pCFI->_cf()._uLayoutGridModeInner == styleLayoutGridModeNotSet)
            {
                if (!pCFI->_pf()._cuvCharGridSizeInner.IsNull())
                {   // set deduced char mode
                    pCFI->_cf()._uLayoutGridModeInner |= (styleLayoutGridModeNone | styleLayoutGridModeChar);
                }
                if (!pCFI->_pf()._cuvLineGridSizeInner.IsNull())
                {   // set deduced char mode
                    pCFI->_cf()._uLayoutGridModeInner |= (styleLayoutGridModeNone | styleLayoutGridModeLine);
                }
            }
            pCFI->UnprepareForDebug();
        }
        break;

    case DISPID_A_LAYOUTGRIDTYPE:
        if (pCFI->_eExtraValues & ComputeFormatsType_GetValue)
        {
            *pCFI->_pvarExtraValue = varValue;
        }
        else
        {
            pCFI->PrepareCharFormat();
            pCFI->_cf()._fHasGridValues = TRUE;
            pCFI->_cf()._uLayoutGridTypeInner = V_I4(&varValue);
            pCFI->UnprepareForDebug();
        }
        break;

    default:
        break;
    }

    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
// Member:      SwapSelectionColors
//
// Synopsis:    Decide if we need to swap the windows selection colors
//              based on the current text color.
//
// Returns:     TRUE: If swap, FALSE otherwise
//
//----------------------------------------------------------------------------
BOOL
CCharFormat::SwapSelectionColors() const
{
    BOOL fSwapColor;
    COLORREF crTextColor, crNewTextColor, crNewBkColor;

    if(_ccvTextColor.IsDefined())
    {
        crTextColor = _ccvTextColor.GetColorRef();

        crNewTextColor = GetSysColor (COLOR_HIGHLIGHTTEXT);
        crNewBkColor   = GetSysColor (COLOR_HIGHLIGHT);
        fSwapColor = ColorDiff (crTextColor, crNewTextColor) <
                     ColorDiff (crTextColor, crNewBkColor);
    }
    else
    {
        fSwapColor = FALSE;
    }

    return fSwapColor;
}

BOOL
CCharFormat::AreInnerFormatsDirty()
{
    return(_fHasBgImage || _fHasBgColor || _fRelative || _fBidiEmbed || _fBidiOverride);
}

void
CCharFormat::ClearInnerFormats()
{
    _fHasBgImage = FALSE;
    _fHasBgColor = FALSE;
    _fRelative   = FALSE;
    _fBidiEmbed  = FALSE;
    _fBidiOverride = FALSE;
    _fHasDirtyInnerFormats = FALSE;
    Assert(!AreInnerFormatsDirty());
}

BOOL
CParaFormat::AreInnerFormatsDirty()
{
    CUnitValue cuvZeroPoints, cuvZeroPercent;
    LONG lZeroPoints, lZeroPercent;
    cuvZeroPoints.SetPoints(0);
    cuvZeroPercent.SetValue(0, CUnitValue::UNIT_PERCENT);
    lZeroPoints = cuvZeroPoints.GetRawValue();
    lZeroPercent = cuvZeroPercent.GetRawValue();

    return( _fTabStops
        ||  _fCompactDL
        ||  _fResetDLLevel
        ||  _fPadBord
        ||  _cuvOffsetPoints.GetRawValue() != lZeroPoints
        ||  _cuvOffsetPercent.GetRawValue() != lZeroPercent
        ||  _cuvLeftIndentPoints.GetRawValue() != lZeroPoints
        ||  _cuvLeftIndentPercent.GetRawValue() != lZeroPercent
        ||  _cuvRightIndentPoints.GetRawValue() != lZeroPoints
        ||  _cuvRightIndentPercent.GetRawValue() != lZeroPercent
        ||  _cuvNonBulletIndentPoints.GetRawValue() != lZeroPoints
        ||  _cuvNonBulletIndentPercent.GetRawValue() != lZeroPercent
        ||  !_cListing.IsReset()
        ||  _lNumberingStart != 0
        ||  _cTabCount != 0);
}

void
CParaFormat::ClearInnerFormats()
{
    CUnitValue cuvZeroPoints, cuvZeroPercent;
    LONG lZeroPoints, lZeroPercent;
    cuvZeroPoints.SetPoints(0);
    cuvZeroPercent.SetValue(0, CUnitValue::UNIT_PERCENT);
    lZeroPoints = cuvZeroPoints.GetRawValue();
    lZeroPercent = cuvZeroPercent.GetRawValue();

    _fTabStops = FALSE;
    _fCompactDL = FALSE;
    _fResetDLLevel = FALSE;
    _fPadBord = FALSE;
    _cuvOffsetPoints.SetRawValue(lZeroPoints);
    _cuvOffsetPercent.SetRawValue(lZeroPercent);
    _cuvLeftIndentPoints.SetRawValue(lZeroPoints);
    _cuvLeftIndentPercent.SetRawValue(lZeroPercent);
    _cuvRightIndentPoints.SetRawValue(lZeroPoints);
    _cuvRightIndentPercent.SetRawValue(lZeroPercent);
    _cuvNonBulletIndentPoints.SetRawValue(lZeroPoints);
    _cuvNonBulletIndentPercent.SetRawValue(lZeroPercent);
    _cListing.Reset();
    _lNumberingStart = 0;
    _cTabCount = 0;
    _fHasDirtyInnerFormats = FALSE;
    Assert(!AreInnerFormatsDirty());
}

BOOL
CFancyFormat::ElementNeedsFlowLayout() const
{
    return (    !_cuvWidth.IsNullOrEnum()
            ||  !_cuvHeight.IsNullOrEnum()
            ||   _bPositionType == stylePositionabsolute
            ||   _bStyleFloat == styleStyleFloatLeft
            ||   _bStyleFloat == styleStyleFloatRight);
}
