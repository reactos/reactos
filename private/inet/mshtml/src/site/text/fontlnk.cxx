//+---------------------------------------------------------------------
//
//   File:      fontlnk.cxx
//
//  Contents:   Code for fontlinking.
//
//------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X__FONTLNK_H_
#define X__FONTLNK_H_
#include "_fontlnk.h"
#endif

#ifndef X_INTL_HXX_
#define X_INTL_HXX_
#include "intl.hxx"
#endif

#ifndef X__FONT_H_
#define X__FONT_H_
#include "_font.h"
#endif

#ifndef X_FPRINT_HXX_
#define X_FPRINT_HXX_
#include "fprint.hxx"
#endif

#ifndef X_WCHDEFS_H_
#define X_WCHDEFS_H_
#include "wchdefs.h"
#endif

#ifndef X_ROOTELEM_HXX_
#define X_ROOTELEM_HXX_
#include "rootelem.hxx"
#endif

#ifndef X_TXTDEFS_H_
#define X_TXTDEFS_H_
#include "txtdefs.h"
#endif

#ifndef X_UNISID_H_
#define X_UNISID_H_
#include <unisid.h>
#endif

#ifndef NO_UTF16
#ifndef X_UNIPART_H_
#define X_UNIPART_H_
#include <unipart.h>
#endif
#endif

MtDefine(FontLinkTextOut_aryLinkFonts_pv, Locals, "FontLinkTextOut aryLinkFonts::_pv")
MtDefine(FontLinkTextOut_aryStringCopy_pv, Locals, "FontLinkTextOut aryStringCopy::_pv")

#pragma warning(disable:4706) /* assignment within conditional expression */

// Need to get at MLANG.
extern IMultiLanguage *g_pMultiLanguage;

// number of font pointers in FontLinkTextOut stack cache
#define FLTO_CACHE_SIZE 32 // max possible, only 32 distinct script bits

DWORD
GetLangBits(
    IMLangFontLink * pMLangFontLink,
    WCHAR wc)
{
    DWORD dwCodePages = 0;

    if (wc < 128 || wc == WCH_NBSP)
    {
        dwCodePages = SBITS_ALLLANGS;
    }
    else if (pMLangFontLink)
    {
#ifndef NO_UTF16
        if (!IsSurrogateChar(wc))
        {
            IGNORE_HR(pMLangFontLink->GetCharCodePages(wc, &dwCodePages));
        }
        else
        {
            if (IsLowSurrogateChar(wc))
            {
                dwCodePages = SBITS_SURROGATE_A | SBITS_SURROGATE_B;
            }
            else
            {
                const CHAR_CLASS cc = CharClassFromCh(wc);

                Assert( cc == NHS_ || cc == WHT_ );
                
                dwCodePages = (cc == NHS_) ? SBITS_SURROGATE_A : SBITS_SURROGATE_B;
            }
        }
#else
        IGNORE_HR(pMLangFontLink->GetCharCodePages(wc, &dwCodePages));
#endif
    }

    return dwCodePages;
}

DWORD GetFontScriptBits(HDC hDC, const TCHAR *szFaceName, LOGFONT *plf)
{
#ifdef WIN16
    return 0x0040;
#else
#ifndef WINCE
    HRESULT hr;
    IMLangFontLink* pMLangFontLink;
    DWORD dwCodePages = 0;

    if ( 0 == StrCmpC( szFaceName, _T("MS Sans Serif") ) &&
         PRIMARYLANGID(LANGIDFROMLCID(g_lcidUserDefault)) == LANG_ENGLISH)
    {
        // NB (cthrash) MS Sans Serif is a really evil font - in spite of the
        // fact that it contains virtually none of the Latin-1 characters,
        // it claims it supports Latin-1.  Unfortunately, MS Sans Serif is
        // an extremely common font for use as DEFUAULT_GUI_FONT.  This means
        // that intrinsic controls will often use MS Sans Serif.  By setting
        // our lang bits to SBITS_ASCII, we make certain we always font link
        // for non-ASCII characters.  We should never fontlink for ASCII chars.
        // See comment in _fontlnk.h.

        dwCodePages = SBITS_ASCII;
        goto Cleanup;
    }
    else if (SUCCEEDED(hr = EnsureMultiLanguage()) &&
             SUCCEEDED(hr = g_pMultiLanguage->QueryInterface(IID_IMLangFontLink,
                                                             (void**)&pMLangFontLink)))
    {
        LOGFONT lf;
        HFONT hFont;

        lf = *plf;
        ::_tcsncpy(lf.lfFaceName, szFaceName, ARRAY_SIZE(lf.lfFaceName));

        hFont = ::CreateFontIndirect(&lf);
        if (hFont)
        {
            hr = pMLangFontLink->GetFontCodePages(hDC, hFont, &dwCodePages);
            Verify(::DeleteObject(hFont));
        }
        else
        {
            hr = E_FAIL; // Out of GDI resource
        }

        pMLangFontLink->Release();
    }

    if (FAILED(hr) || !dwCodePages)
    {
        CHARSETINFO csi;

        if (::TranslateCharsetInfo((LPDWORD)(LONG_PTR)MAKELONG(plf->lfCharSet, 0), &csi, TCI_SRCCHARSET))
        {
            dwCodePages = csi.fs.fsCsb[0];
            hr = S_OK;
        }
        else
        {
            hr = E_FAIL; // Invalid plf->lfCharSet
        }
    }

    // If dwLangBits is zero, this means we would ALWAYS fontlink.  If
    // it is in fact zero, we can't really know what the font supports.
    // Here we set all the bits on, so as to NEVER fontlink.

    if (dwCodePages == 0 || FAILED(hr))
    {
        dwCodePages = SBITS_ALLLANGS;
    }

    // NB (cthrash) We've decouple the ASCII portion of Latin-1 so that
    // we may more efficiently handle bad fonts which claim to support
    // Latin-1 when only really supporting ASCII.

    dwCodePages |= SBITS_ASCII;

Cleanup:

    return dwCodePages;
#else
    return 0;
#endif // WINCE
#endif // ndef WIN16
}

void DeinitFontLinking(THREADSTATE *)
{
    IMLangFontLink* pMLangFontLink;

    if (g_pMultiLanguage &&
        SUCCEEDED(g_pMultiLanguage->QueryInterface(IID_IMLangFontLink, (void**)&pMLangFontLink)))
    {
        pMLangFontLink->ResetFontMapping();
        pMLangFontLink->Release();
    }
}

//+---------------------------------------------------------------
//
//  CharSetFromLangBits
//
//  Synopsis : Compute a valid GDI charset from the language bits
//
//+---------------------------------------------------------------

static const BYTE s_abCharSetFromLangBit[32] =
{
    SYMBOL_CHARSET,      // FS_SYMBOL               0x80000000L
    DEFAULT_CHARSET,     //                         0x40000000L
    DEFAULT_CHARSET,     //                         0x20000000L
    DEFAULT_CHARSET,     //                         0x10000000L
    DEFAULT_CHARSET,     //                         0x08000000L
    DEFAULT_CHARSET,     //                         0x04000000L
    DEFAULT_CHARSET,     //                         0x02000000L
    DEFAULT_CHARSET,     //                         0x01000000L
    DEFAULT_CHARSET,     //                         0x00800000L
    DEFAULT_CHARSET,     //                         0x00400000L
    JOHAB_CHARSET,       // FS_JOHAB                0x00200000L
    CHINESEBIG5_CHARSET, // FS_CHINESETRAD          0x00100000L
    HANGEUL_CHARSET,     // FS_WANSUNG              0x00080000L
    GB2312_CHARSET,      // FS_CHINESESIMP          0x00040000L
    SHIFTJIS_CHARSET,    // FS_JISJAPAN             0x00020000L
    THAI_CHARSET,        // FS_THAI                 0x00010000L
    DEFAULT_CHARSET,     //                         0x00008000L
    DEFAULT_CHARSET,     //                         0x00004000L
    DEFAULT_CHARSET,     //                         0x00002000L
    DEFAULT_CHARSET,     //                         0x00001000L
    DEFAULT_CHARSET,     //                         0x00000800L
    DEFAULT_CHARSET,     //                         0x00000400L
    DEFAULT_CHARSET,     //                         0x00000200L
    VIETNAMESE_CHARSET,  // FS_VIETNAMESE           0x00000100L
    BALTIC_CHARSET,      // FS_BALTIC               0x00000080L
    ARABIC_CHARSET,      // FS_ARABIC               0x00000040L
    HEBREW_CHARSET,      // FS_HEBREW               0x00000020L
    TURKISH_CHARSET,     // FS_TURKISH              0x00000010L
    GREEK_CHARSET,       // FS_GREEK                0x00000008L
    RUSSIAN_CHARSET,     // FS_CYRILLIC             0x00000004L
    EASTEUROPE_CHARSET,  // FS_LATIN2               0x00000002L
    ANSI_CHARSET         // FS_LATIN1               0x00000001L
};


BYTE
CharSetFromLangBits( DWORD dwLangBits )
{
    for (int i=32; i--; dwLangBits >>= 1)
    {
        if (dwLangBits & 1)
        {
            return s_abCharSetFromLangBit[i];
        }
    }

    return DEFAULT_CHARSET;
}

//-----------------------------------------------------------------------------
//
//  function:   GetFontLinkFontName
//
//  returns:    An appropriate facename for the script/lang input set in pcf
//
//-----------------------------------------------------------------------------

BOOL
GetFontLinkFontName(
    IMLangFontLink *pMLangFontLink, // IN
    HDC             hdc,            // IN
    CDoc *          pDoc,           // IN                    
    DWORD           dwScriptBits,   // IN
    LCID            lcidLang,       // IN
    const LOGFONT * lplf,           // IN
    CCharFormat *   pcf )           // OUT
{
    HFONT hSrcFont = ::CreateFontIndirect(lplf);
    HFONT hDestFont = NULL;
    LOGFONT lfDestFont;
    BOOL fNewlyFetchedFromRegistry = FALSE; // for surrogates

#ifndef NO_UTF16
    if (dwScriptBits & (SBITS_SURROGATE_A | SBITS_SURROGATE_B))
    {
        fNewlyFetchedFromRegistry = SelectScriptAppropriateFont( (dwScriptBits & SBITS_SURROGATE_A)
                                                                 ? sidSurrogateA
                                                                 : sidSurrogateB,
                                                                 DEFAULT_CHARSET,
                                                                 pDoc,
                                                                 pcf );

        goto Cleanup;
    }
    else
#endif
    if (hSrcFont && pMLangFontLink)
    {
        TCHAR pszCodePage[4+1];
        DWORD dwCodePages;

        if (::GetLocaleInfo(lcidLang, LOCALE_IDEFAULTANSICODEPAGE, pszCodePage, ARRAY_SIZE(pszCodePage)) &&
            SUCCEEDED(pMLangFontLink->CodePageToCodePages(StrToInt(pszCodePage), &dwCodePages)))
        {
            dwCodePages &= dwScriptBits;
            if (dwCodePages &&
                dwCodePages != dwScriptBits &&
                FAILED(pMLangFontLink->MapFont(hdc, dwCodePages, hSrcFont, &hDestFont)))
            {
                hDestFont = NULL;
            }
        }

        if (!hDestFont &&
            FAILED(pMLangFontLink->MapFont(hdc, dwScriptBits, hSrcFont, &hDestFont)))
        {
            hDestFont = NULL;
        }

        if (hSrcFont)
            ::DeleteObject(hSrcFont);

        if (hDestFont)
        {
            ::GetObject(hDestFont, sizeof(lfDestFont), &lfDestFont);
            pMLangFontLink->ReleaseFont(hDestFont);

            lplf = &lfDestFont;
        }
    }

    pcf->SetFaceName(lplf->lfFaceName);

    // BUGBUG (cthrash) MLANG suffers from the same GDI bug that Trident does, namely
    // it calls EnumFontFamilies instead of EnumFontFamiliesEx and thus gets
    // inaccurate GDI charset information.  For now, we will assume all charset values
    // other than 0 (ANSI_CHARSET) to be valid.  This is so we don't pick the wrong
    // charset in the unified-Han scenario.  Note that WinNT has no issue with
    // the the GDI charset being DEFAULT_CHARSET

    if (g_dwPlatformID == VER_PLATFORM_WIN32_NT)
    {
        pcf->_bCharSet = DEFAULT_CHARSET;
    }
    else if (lplf->lfCharSet != ANSI_CHARSET)
    {
        pcf->_bCharSet = lplf->lfCharSet;
    }
    else
    {
        pcf->_bCharSet = CharSetFromLangBits( dwScriptBits );
    }

Cleanup:

    return fNewlyFetchedFromRegistry;
}

//+---------------------------------------------------------------
//
//  GetFont
//
//  Synopsis : Returns a CCcs * with langbits matching ch.
//             Helper function for FontLinkTextOut.
//
//+---------------------------------------------------------------

CCcs *
GetFont(
    IMLangFontLink * pMLangFontLink,
    TCHAR ch,
    HDC hDC,
    CDocInfo *pdci,
    const CCharFormat *pCF,
    CCcs *pccsDefault,
    CStackDataAry<CCcs *,FLTO_CACHE_SIZE> *paryLinkFonts )
{
    CCharFormat cf;
    DWORD dwLangBits;
    CCcs *pccs;
    CCcs **ppccs;
    int i;
    CBaseCcs *pBaseCcsDefault = pccsDefault->GetBaseCcs();
    BOOL fNewlyFetchedFromRegistry; // for surrogates
    
    // BUGBUG (cthrash) Temporary hack.

    pccsDefault->EnsureLangBits();

    // check the default font first
    if (ch <= 0x7f || // perf hack: all fonts have glyphs for unicode 0 - 0x7f
        (dwLangBits = GetLangBits(pMLangFontLink, ch)) == 0 ||
        (dwLangBits & pBaseCcsDefault->_dwLangBits))
    {
        return pccsDefault;
    }

    // Search for a match in the cache we build up.
    // There's some perf improvement here because we avoid extra calls
    // to GetFontLinkFontName, but the real reason we do this is to
    // ensure the same character is always rendered with a consistent
    // glyph.
    for (i=0; i < paryLinkFonts->Size(); i++)
    {
        if (dwLangBits & (*paryLinkFonts)[i]->GetBaseCcs()->_dwLangBits)
        {
            break;
        }
    }

    if (i < paryLinkFonts->Size())
    {
        return (*paryLinkFonts)[i];
    }

    // no match, grab a new font from the global cache

    cf = *pCF;
    fNewlyFetchedFromRegistry = GetFontLinkFontName(pMLangFontLink, hDC, pdci->_pDoc, dwLangBits, cf._lcid, &pBaseCcsDefault->_lf, &cf);
    cf._bCrcFont = cf.ComputeFontCrc();
    pccs = fc().GetFontLinkCcs(hDC, pdci, pccsDefault, &cf);
    if (pccs == NULL)
    {
        return pccsDefault;
    }
    
    pccs->EnsureLangBits();

#ifndef NO_UTF16
    if (dwLangBits & (SBITS_SURROGATE_A | SBITS_SURROGATE_B))
    {
        // If we just fetched the face from the registry, be sure to
        // set the SCRIPT_IDS straight in the CFontCache.

        if (fNewlyFetchedFromRegistry)
        {
            ForceScriptIdOnUserSpecifiedFont( pdci->_pDoc->_pOptionSettings,
                                              (dwLangBits & SBITS_SURROGATE_A)
                                              ? sidSurrogateA
                                              : sidSurrogateB );
        }

        // EnsureLangBits isn't going to set the SBITS_SURROGATE_A or _B flags.
        // Force the bits set, so that the next char can be rendered in the same font.

        pccs->GetBaseCcs()->_dwLangBits |= dwLangBits;
    }
#endif

    if  ((pccs->GetBaseCcs()->_dwLangBits & dwLangBits) == 0 ||
         (ppccs = paryLinkFonts->Append()) == NULL)
    {
        pccs->Release();
        return pccsDefault;
    }

    *ppccs = pccs;

    return pccs;
}

//+---------------------------------------------------------------
//
//  FontLinkTextOut
//
//  Synopsis : ExtTextOutW functionality with font linking thrown
//             in.
//
//  uMode is one of
//
//      FLTO_BOTH           render the string and return its text extent
//      FLTO_TEXTOUTONLY    render the string, return value is pos/neg only
//      FLTO_TEXTEXTONLY    just return the text textent, render nothing
//
//  Return value is negative on error, otherwise the text extent
//  or in any case a non-negative number.
//
//+---------------------------------------------------------------

int
FontLinkTextOut (
    HDC hDC,
    int x, int y,
    UINT fuOptions,
    const GDIRECT *prc,
    LPCTSTR pString,
    UINT cch,
    CDocInfo *pdci,
    const CCharFormat *pCF,
    UINT uMode )
{
    CStackDataAry<CCcs *, FLTO_CACHE_SIZE> aryLinkFonts(Mt(FontLinkTextOut_aryLinkFonts_pv));
    CCcs *pccsRun;
    CCcs *pccsNextRun;
    CCcs *pccsDefault;
    LPCTSTR pPastRun;
    long lCharWidth = 0;
    HFONT hOldFont;
    int iDefBaseLinePlusY;
    int xin;
    int iDx;
    int i;
    CStackDataAry <TCHAR, CHUNKSIZE> aryStringCopy(Mt(FontLinkTextOut_aryStringCopy_pv));
    IMLangFontLink *pMLangFontLink = NULL;
    CODEPAGE codepage = pdci->_pDoc->GetFamilyCodePage();
    BOOL fBidiCodepage = (codepage == CP_1256 || codepage == CP_1255);

    if (pString == NULL || cch <= 0)
    {
        return (cch == 0) ? 0 : -1;
    }

    pccsDefault = fc().GetCcs(hDC, pdci, pCF);

    pccsDefault->EnsureLangBits();

    if ((pccsDefault == NULL) ||
        (pccsDefault->GetBaseCcs()->_dwLangBits == 0))
    {
        if (pccsDefault != NULL)
        {
            pccsDefault->Release();
        }

        return -1;
    }

    xin = x;
    iDefBaseLinePlusY = pccsDefault->GetBaseCcs()->_yHeight -
                        pccsDefault->GetBaseCcs()->_yDescent + y;

    hOldFont = (HFONT)GetCurrentObject(hDC, OBJ_FONT);

    //
    // Get IMultiLanguage::IMLangFontLink
    //
    
    if (g_pMultiLanguage)
    {
        IGNORE_HR( g_pMultiLanguage->QueryInterface(IID_IMLangFontLink,(void**)&pMLangFontLink) );
    }

    // init for loop
    pPastRun = pString;
    pccsNextRun = GetFont(pMLangFontLink, *pString, hDC, pdci, pCF, pccsDefault, &aryLinkFonts);

    do
    {
        // set the font for this run
        SelectFontEx(hDC, pccsNextRun->GetBaseCcs()->_hfont);

        // skip first char in run -- already checked its font
        pPastRun++;
        cch--;
        pccsRun = pccsNextRun;

        // get the length of the run and next font
        while (cch)
        {
            // May need to do the *ugly* NBSP hack.
            if (*pPastRun == WCH_NBSP)
            {
                if (pccsDefault->GetBaseCcs()->ConvertNBSPs(hDC, pdci->_pDoc))
                {
                    LONG cchDelta = pPastRun - pString;
                    LONG cchNew = cch + cchDelta;

                    if (aryStringCopy.Grow( cchNew ) == S_OK)
                    {
                        TCHAR * pNewString = aryStringCopy;
                        const TCHAR * pSrc = pString;
                        TCHAR * pDst = pNewString;

                        while (cchNew--)
                        {
                            TCHAR ch = *pSrc++;
                            *pDst++ = (ch == WCH_NBSP) ? _T(' ') : ch;
                        }

                        pString = pNewString;
                        pPastRun = pString + cchDelta;
                    }
                }
            }

            if(!fBidiCodepage)
            {
                pccsNextRun = GetFont(pMLangFontLink, *pPastRun, hDC, pdci, pCF, pccsDefault, &aryLinkFonts);
            }
            else
            {
                // If we have a bidi codepage (Arabic or Hebrew) we want to try the existing font for the character first.
                // This will help us not to break in cases where we have RTL text in a LTR control - Uniscribe
                // correctly handles shaping for us in LSUniscribeTextOut if an entire run of mixed text
                // is fed in.
                pccsNextRun = GetFont(pMLangFontLink, *pPastRun, hDC, pdci, pCF, pccsRun, &aryLinkFonts);
            }

            if (pccsRun->GetBaseCcs() != pccsNextRun->GetBaseCcs())
                break;

            cch--;
            pPastRun++;
        }

        // output this run
        iDx = 0;
        if (uMode != FLTO_TEXTEXTONLY)
        {
            int yRun = iDefBaseLinePlusY - (pccsRun->GetBaseCcs()->_yHeight - pccsRun->GetBaseCcs()->_yDescent);

            if (*pString != WCH_NBSP)
            {
                VanillaTextOut(pccsRun, hDC, x, yRun, fuOptions, prc, pString, pPastRun - pString, codepage, &iDx);
            }
            else
            {
                int cch = pPastRun - pString - 1;

                VanillaTextOut(pccsRun, hDC, x, yRun, fuOptions, prc, L" ", 1, codepage, &iDx);

                if (cch)
                {
                    int xT;
                    
                    pccsRun->Include(L' ', lCharWidth);
                    if (fuOptions & ETO_RTLREADING)
                        xT = x - lCharWidth;
                    else
                        xT = x + lCharWidth;

                    VanillaTextOut(pccsRun, hDC, xT, yRun, fuOptions, prc, pString + 1, cch, codepage, &iDx);
                }                    
            }
        }

        // Handle char widths for the final run only if caller wants text extent
        // or we need the text extent to handle underline/strikeout.
        if(iDx == 0)
        {
            if (cch || uMode != FLTO_TEXTOUTONLY || pCF->_fUnderline || pCF->_fStrikeOut)
            {
                int dx = 0;

                while (pString < pPastRun)
                {
                    pccsRun->Include(*pString, lCharWidth);
                    dx += lCharWidth;
                    pString++;
                }

                x += (fuOptions & ETO_RTLREADING) ? -dx : dx;
            }
        }
        else
        {
            if(fuOptions & ETO_RTLREADING)
                x -= iDx;
            else
                x += iDx;

            pString = pPastRun;
        }

    } while (cch);

    if (uMode != FLTO_TEXTEXTONLY && (pCF->_fUnderline || pCF->_fStrikeOut))
    {
        HFONT hFont;
        LOGFONT lf;

        // Fonts from the global cache never have underline/strikeout attributes
        // because CRenderer draws its own underlines/strikeouts.  So allocate
        // an appropriate font here.
        // We do the underline/strikeout in one go to ensure consistency across
        // multiple runs and to work around a win95 gdi bug.
        // perf: could have the caller pass in the new HFONT, but the tradeoff
        // is generality and the risk of the font not matching.
        lf = pccsDefault->GetBaseCcs()->_lf;
        lf.lfUnderline = pCF->_fUnderline;
        lf.lfStrikeOut = pCF->_fStrikeOut;

        if ((hFont = CreateFontIndirect(&lf)) != 0)
        {
            DrawUnderlineStrikeOut(xin, y, x - xin, hDC, hFont, prc);
            Verify(DeleteObject(hFont));
        }
    }

    if (hOldFont != pccsRun->GetBaseCcs()->_hfont) // often the same because of the global font cache
    {
        SelectFontEx(hDC, hOldFont);
    }

    pccsDefault->Release();

    for (i=0; i < aryLinkFonts.Size(); i++)
    {
        aryLinkFonts[i]->Release();
    }

    ReleaseInterface( pMLangFontLink );

    return (fuOptions & ETO_RTLREADING ? xin - x : x - xin);
}


//+---------------------------------------------------------------
//
//  VanillaTextOut
//
//  Synopsis : A wrapped ExtTextOutW, accounting for a win95 china
//  bug.
//
//+---------------------------------------------------------------
void VanillaTextOut(
    CCcs * pccs,                    
    HDC hdc,
    int x, int y,
    UINT fuOptions,
    const GDIRECT *prc,
    LPCTSTR pString,
    UINT cch,
    UINT uCodePage,
    int *piDx)
{
    // COMPLEX TEXT.  ExtTextOutW does not work properly on foreign language versions
    //                of Win95. Additionally, we want to send all glyphable text through
    //                Uniscribe for shaping and output.

    BOOL fGlyph = FALSE;
    BOOL fRTL = FALSE;

    // There is an assumption that *piDx is 0 coming in. Really, we should move
    // the measurement code from FontLinkTextOut() down here and use it
    // whenever we don't go through LSUniscribeTextOut(). Then this wouldn't
    // matter.
    Assert(piDx == NULL || *piDx == 0);

    fRTL = !!(fuOptions & ETO_RTLREADING);

    if(!fRTL)
    {
        for(UINT i = 0; i < cch; i++)
        {
            WCHAR ch = pString[i];
            if(ch >= 0x300 && IsGlyphableChar(ch))
            {
                fGlyph = TRUE;
                break;
            }
        }
    }


    // send complex text or text layed out right-to-left
    // to be drawn through Uniscribe
    if((fGlyph || fRTL) && !g_fExtTextOutGlyphCrash)
    {
        HRESULT hr;

        extern HRESULT LSUniscribeTextOut(HDC hdc, 
                                       int iX, 
                                       int iY, 
                                       UINT uOptions, 
                                       CONST RECT *prc, 
                                       LPCTSTR pString, 
                                       UINT cch,
                                       int *piDx); 
 
        hr = LSUniscribeTextOut(hdc,
                                x, 
                                y,
                                fuOptions,
                                prc,
                                pString,
                                cch,
                                piDx);

        // if we failed, let the normal path be our fallback
        if(!hr)
            return;
    }

#ifndef WIN16
    if (   IsExtTextOutWBuggy(uCodePage))
    {
        extern BOOL LSReExtTextOut( CCcs *, HDC, int, int, UINT,
                                    CONST RECT *, const WCHAR *,
                                    long cbCount, CONST INT *lpDx,
                                    CONVERTMODE cm );

        LSReExtTextOut( pccs,
                        hdc,
                        x, y,
                        fuOptions,
                        prc,
                        pString,
                        cch,
                        NULL,
                        CM_MULTIBYTE );
    }
    else
#endif
    {
        ExtTextOutW(hdc,
                    x, y,
                    fuOptions,
                    prc,
                    pString,
                    cch,
                    NULL);
    }

    return;
}


//+---------------------------------------------------------------
//
//  DrawUnderlineStrikeOut
//
//  Synopsis : Draws an underline/strikeout.  This is a workaround
//  for a win95 ExtTextOutW problems with underlines.
//
//+---------------------------------------------------------------
void DrawUnderlineStrikeOut(int x, int y, int iLength, HDC hDC, HFONT hFont, const GDIRECT *prc)
{
    int iOldBkMode;
    HFONT hOldFont;

    if ((iOldBkMode = SetBkMode(hDC, TRANSPARENT)) == 0)
    {
        return;
    }
    hOldFont = SelectFontEx(hDC, hFont);

    ExtTextOutA(hDC, x, y, 0, prc, " ", 1, &iLength);

    SelectFontEx(hDC, hOldFont);
    SetBkMode(hDC, iOldBkMode);

    return;
}


//+---------------------------------------------------------------
//
//  NeedsFontLinking
//
//  Synopsis : Returns TRUE iff the supplied font does not contain
//  glyphs for the entire string.
//
//----------------------------------------------------------------

BOOL NeedsFontLinking(HDC hdc, CCcs *pccs, LPCTSTR pString, int cch, CDoc *pDoc)
{
    CBaseCcs *pBaseCcs;
    IMLangFontLink *pMLangFontLink = NULL;
    
    if (pccs == NULL ||
        pccs->GetBaseCcs() == NULL ||
        pString == NULL ||
        cch <= 0)
    {
        return FALSE;
    }

    
    pBaseCcs = pccs->GetBaseCcs();
    pccs->EnsureLangBits();

    if (pBaseCcs->_dwLangBits == 0)
    {
        return FALSE;
    }

    const DWORD dwDefLangBits = pBaseCcs->_dwLangBits;

    for (; cch; cch--)
    {
        const TCHAR ch = *pString++;

        if (ch < 0x80)
        {
            continue; // perf hack: all fonts have glyphs for unicode 0 - 0x7f
        }
        else if (ch == WCH_NBSP)
        {
            if (pBaseCcs->ConvertNBSPs(hdc, pDoc))
            {
                break;
            }
        }
        else
        {
            if (!pMLangFontLink)
            {

                if (g_pMultiLanguage)
                {
                    THR( g_pMultiLanguage->QueryInterface(IID_IMLangFontLink, (void**)&pMLangFontLink) );
                }
            }
                
            if (!(GetLangBits(pMLangFontLink, ch) & dwDefLangBits))
                break;
        }
    }

    ReleaseInterface( pMLangFontLink );

    return cch != 0;
}

//+----------------------------------------------------------------------------
//
//  Function:   SelectScriptAppropriateFont
//
//  Synopsis:   Populates a CCharFormat element on the fly for fontlinking
//              with script-appropriate information.
//
//  Notes:      The incomoming SCRIPT_ID (sid) should already by Han ununified,
//              This means if any CJK disamguation is necessary, it should
//              have been done before calling this function.
//
//  Returns:    Nothing.
//
//-----------------------------------------------------------------------------

const WCHAR * AlternateFontNameIfAvailable( const WCHAR * );

BOOL
SelectScriptAppropriateFont(
    SCRIPT_ID sid,          // IN
    BYTE bCharSet,          // IN
    CDoc * pDoc,            // IN
    CCharFormat * pcf )     // IN/OUT
{
    HRESULT hr;
    BOOL fNewlyFetched = FALSE;
    LONG latmFontFace;

    hr = THR( pDoc->EnsureOptionSettings() );
    if (hr)
        goto Cleanup;

    sid = RegistryAppropriateSidFromSid( sid );

    fNewlyFetched = ScriptAppropriateFaceNameAtom(sid, pDoc, pcf->_bPitchAndFamily & FIXED_PITCH, &latmFontFace );

    pcf->_latmFaceName = latmFontFace;
    pcf->_bCharSet = bCharSet;
    pcf->_fNarrow  = IsNarrowCharSet( bCharSet );
    pcf->_bCrcFont = pcf->ComputeFontCrc();

Cleanup:

    return fNewlyFetched;
}

BOOL
ScriptAppropriateFaceNameAtom(
    SCRIPT_ID sid,          // IN
    CDoc * pDoc,            // IN
    BOOL fFixed,            // IN
    LONG *platmFontFace )   // OUT                              
{
    LONG latmFontFace = fFixed
                        ? pDoc->_pOptionSettings->alatmFixedPitchFonts[sid]
                        : pDoc->_pOptionSettings->alatmProporitionalFonts[sid];
    const BOOL fFetch = latmFontFace == -1;

    if (fFetch)
    {
        CODEPAGESETTINGS CS;

        CS.SetDefaults(pDoc->GetFamilyCodePage(), pDoc->_pOptionSettings->sBaselineFontDefault );

        pDoc->_pOptionSettings->ReadCodepageSettingsFromRegistry( &CS, 0, sid );

        pDoc->_pOptionSettings->alatmFixedPitchFonts[sid] = CS.latmFixedFontFace;
        pDoc->_pOptionSettings->alatmProporitionalFonts[sid] = CS.latmPropFontFace;

        latmFontFace = fFixed
                       ? CS.latmFixedFontFace
                       : CS.latmPropFontFace;
    }

    *platmFontFace = latmFontFace;
    
    return fFetch;
}

void
ForceScriptIdOnUserSpecifiedFont( 
    OPTIONSETTINGS * pOS,
    SCRIPT_ID sid )
{
    const SCRIPT_ID sidReg = RegistryAppropriateSidFromSid( sid );
    const LONG latmFixedFont = pOS->alatmFixedPitchFonts[sidReg];
    const LONG latmProportionalFont = pOS->alatmProporitionalFonts[sidReg];

    if (latmFixedFont > 0)
    {
        fc().AddScriptIdToAtom(latmFixedFont, sid);
    }

    if (latmProportionalFont && latmProportionalFont != latmFixedFont)
    {
        fc().AddScriptIdToAtom(latmProportionalFont, sid);
    }
}
