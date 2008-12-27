/*
 * COMMDLG - Font Dialog
 *
 * Copyright 1994 Martin Ayotte
 * Copyright 1996 Albrecht Kleine
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <ctype.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include "windef.h"
#include "winbase.h"
#include "winnls.h"
#include "wingdi.h"
#include "winuser.h"
#include "commdlg.h"
#include "dlgs.h"
#include "wine/debug.h"
#include "cderr.h"

WINE_DEFAULT_DEBUG_CHANNEL(commdlg);

static const WCHAR strWineFontData[] = {'_','_','W','I','N','E','_','F','O','N','T','D','L','G','D','A','T','A',0};
static const WCHAR strWineFontData_a[] =
                               {'_','_','W','I','N','E','_','F','O','N','T','D','L','G','D','A','T','A','_','A',0};
static const WCHAR chooseFontW[] = {'C','H','O','O','S','E','_','F','O','N','T',0};

#include "cdlg.h"

/* image list with TrueType bitmaps and more */
static HIMAGELIST himlTT = 0;
#define TTBITMAP_XSIZE 20 /* x-size of the bitmaps */

static INT_PTR CALLBACK FormatCharDlgProcA(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
static INT_PTR CALLBACK FormatCharDlgProcW(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

/* There is a table here of all charsets, and the sample text for each.
 * There is a second table that translates a charset into an index into
 * the first table.
 */

#define CI(cs) ((IDS_CHARSET_##cs)-IDS_CHARSET_ANSI)


static const WCHAR stWestern[]={'A','a','B','b','Y','y','Z','z',0}; /* Western and default */
static const WCHAR stSymbol[]={'S','y','m','b','o','l',0}; /* Symbol */
static const WCHAR stShiftJis[]={'A','a',0x3042,0x3041,0x30a2,0x30a1,0x4e9c,0x5b87,0}; /* Shift JIS */
static const WCHAR stHangul[]={0xac00,0xb098,0xb2e4,'A','a','B','Y','y','Z','z',0}; /* Hangul */
static const WCHAR stGB2312[]={0x5fae,0x8f6f,0x4e2d,0x6587,0x8f6f,0x4ef6,0}; /* GB2312 */
static const WCHAR stBIG5[]={0x4e2d,0x6587,0x5b57,0x578b,0x7bc4,0x4f8b,0}; /* BIG5 */
static const WCHAR stGreek[]={'A','a','B','b',0x0391,0x03b1,0x0392,0x03b2,0}; /* Greek */
static const WCHAR stTurkish[]={'A','a','B','b',0x011e,0x011f,0x015e,0x015f,0}; /* Turkish */
static const WCHAR stHebrew[]={'A','a','B','b',0x05e0,0x05e1,0x05e9,0x05ea,0}; /* Hebrew */
static const WCHAR stArabic[]={'A','a','B','b',0x0627,0x0628,0x062c,0x062f,0x0647,0x0648,0x0632,0};/* Arabic */
static const WCHAR stBaltic[]={'A','a','B','b','Y','y','Z','z',0}; /* Baltic */
static const WCHAR stVietname[]={'A','a','B','b',0x01a0,0x01a1,0x01af,0x01b0,0}; /* Vietnamese */
static const WCHAR stCyrillic[]={'A','a','B','b',0x0411,0x0431,0x0424,0x0444,0}; /* Cyrillic */
static const WCHAR stEastEur[]={'A','a','B','b',0xc1,0xe1,0xd4,0xf4,0}; /* East European */
static const WCHAR stThai[]={'A','a','B','b',0x0e2d,0x0e31,0x0e01,0x0e29,0x0e23,0x0e44,0x0e17,0x0e22,0}; /* Thai */
static const WCHAR stJohab[]={0xac00,0xb098,0xb2e4,'A','a','B','Y','y','Z','z',0}; /* Johab */
static const WCHAR stMac[]={'A','a','B','b','Y','y','Z','z',0}; /* Mac */
static const WCHAR stOEM[]={'A','a','B','b',0xf8,0xf1,0xfd,0}; /* OEM */
/* the following character sets actually behave different (Win2K observation):
 * the sample string is 'sticky': it uses the sample string of the previous
 * selected character set. That behaviour looks like some default, which is
 * not (yet) implemented. */
static const WCHAR stVISCII[]={'A','a','B','b',0}; /* VISCII */
static const WCHAR stTCVN[]={'A','a','B','b',0}; /* TCVN */
static const WCHAR stKOI8[]={'A','a','B','b',0}; /* KOI-8 */
static const WCHAR stIso88593[]={'A','a','B','b',0}; /* ISO-8859-3 */
static const WCHAR stIso88594[]={'A','a','B','b',0}; /* ISO-8859-4 */
static const WCHAR stIso885910[]={'A','a','B','b',0}; /* ISO-8859-10 */
static const WCHAR stCeltic[]={'A','a','B','b',0};/* Celtic */

static const WCHAR * const sample_lang_text[]={
    stWestern,stSymbol,stShiftJis,stHangul,stGB2312,
    stBIG5,stGreek,stTurkish,stHebrew,stArabic,
    stBaltic,stVietname,stCyrillic,stEastEur,stThai,
    stJohab,stMac,stOEM,stVISCII,stTCVN,
    stKOI8,stIso88593,stIso88594,stIso885910,stCeltic};


static const BYTE CHARSET_ORDER[256]={
    CI(ANSI), 0, CI(SYMBOL), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, CI(MAC), 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    CI(JIS), CI(HANGUL), CI(JOHAB), 0, 0, 0, CI(GB2312), 0, CI(BIG5), 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, CI(GREEK), CI(TURKISH), CI(VIETNAMESE), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, CI(HEBREW), CI(ARABIC), 0, 0, 0, 0, 0, 0, 0, CI(BALTIC), 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, CI(RUSSIAN), 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, CI(THAI), 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, CI(EE), 0,
    CI(VISCII), CI(TCVN), CI(KOI8), CI(ISO3), CI(ISO4), CI(ISO10), CI(CELTIC), 0, 0, 0, 0, 0, 0, 0, 0, CI(OEM),
};

static const struct {
    DWORD       mask;
    const char *name;
} cfflags[] = {
#define XX(x) { x, #x },
    XX(CF_SCREENFONTS)
    XX(CF_PRINTERFONTS)
    XX(CF_SHOWHELP)
    XX(CF_ENABLEHOOK)
    XX(CF_ENABLETEMPLATE)
    XX(CF_ENABLETEMPLATEHANDLE)
    XX(CF_INITTOLOGFONTSTRUCT)
    XX(CF_USESTYLE)
    XX(CF_EFFECTS)
    XX(CF_APPLY)
    XX(CF_ANSIONLY)
    XX(CF_NOVECTORFONTS)
    XX(CF_NOSIMULATIONS)
    XX(CF_LIMITSIZE)
    XX(CF_FIXEDPITCHONLY)
    XX(CF_WYSIWYG)
    XX(CF_FORCEFONTEXIST)
    XX(CF_SCALABLEONLY)
    XX(CF_TTONLY)
    XX(CF_NOFACESEL)
    XX(CF_NOSTYLESEL)
    XX(CF_NOSIZESEL)
    XX(CF_SELECTSCRIPT)
    XX(CF_NOSCRIPTSEL)
    XX(CF_NOVERTFONTS)
#undef XX
};

void _dump_cf_flags(DWORD cflags)
{
    unsigned int i;

    for (i = 0; i < sizeof(cfflags)/sizeof(cfflags[0]); i++)
        if (cfflags[i].mask & cflags)
            TRACE("%s|",cfflags[i].name);
    TRACE("\n");
}

/***********************************************************************
 *           ChooseFontW   (COMDLG32.@)
 *
 * Create a font dialog box.
 *
 * PARAMS
 *  lpChFont [I/O] in:  information to initialize the dialog box.
 *                 out: User's color selection
 *
 * RETURNS
 *  TRUE:  Ok button clicked.
 *  FALSE: Cancel button clicked, or error.
 */
BOOL WINAPI ChooseFontW(LPCHOOSEFONTW lpChFont)
{
    LPCVOID template;
    HRSRC hResInfo;
    HINSTANCE hDlginst;
    HGLOBAL hDlgTmpl;

    TRACE("(%p)\n", lpChFont);

    if ( (lpChFont->Flags&CF_ENABLETEMPLATEHANDLE)!=0 )
    {
        template=(LPCVOID)lpChFont->hInstance;
    } else
    {
        if ( (lpChFont->Flags&CF_ENABLETEMPLATE)!=0 )
        {
            hDlginst=lpChFont->hInstance;
            if( !(hResInfo = FindResourceW(hDlginst, lpChFont->lpTemplateName,
                            (LPWSTR)RT_DIALOG)))
            {
                COMDLG32_SetCommDlgExtendedError(CDERR_FINDRESFAILURE);
                return FALSE;
            }
        } else
        {
            hDlginst=COMDLG32_hInstance;
            if (!(hResInfo = FindResourceW(hDlginst, chooseFontW, (LPWSTR)RT_DIALOG)))
            {
                COMDLG32_SetCommDlgExtendedError(CDERR_FINDRESFAILURE);
                return FALSE;
            }
        }
        if (!(hDlgTmpl = LoadResource(hDlginst, hResInfo )) ||
                !(template = LockResource( hDlgTmpl )))
        {
            COMDLG32_SetCommDlgExtendedError(CDERR_LOADRESFAILURE);
            return FALSE;
        }
    }
    if (TRACE_ON(commdlg))
        _dump_cf_flags(lpChFont->Flags);

    if (lpChFont->Flags & (CF_SELECTSCRIPT | CF_NOVERTFONTS ))
        FIXME(": unimplemented flag (ignored)\n");

    return DialogBoxIndirectParamW(COMDLG32_hInstance, template,
            lpChFont->hwndOwner, FormatCharDlgProcW, (LPARAM)lpChFont );
}

/***********************************************************************
 *           ChooseFontA   (COMDLG32.@)
 *
 * See ChooseFontW.
 */
BOOL WINAPI ChooseFontA(LPCHOOSEFONTA lpChFont)
{
    LPCVOID template;
    HRSRC hResInfo;
    HINSTANCE hDlginst;
    HGLOBAL hDlgTmpl;

    TRACE("(%p)\n", lpChFont);

    if ( (lpChFont->Flags&CF_ENABLETEMPLATEHANDLE)!=0 )
    {
        template=(LPCVOID)lpChFont->hInstance;
    } else
    {
        if ( (lpChFont->Flags&CF_ENABLETEMPLATE)!=0 )
        {
            hDlginst=lpChFont->hInstance;
            if( !(hResInfo = FindResourceA(hDlginst, lpChFont->lpTemplateName,
                            (LPSTR)RT_DIALOG)))
            {
                COMDLG32_SetCommDlgExtendedError(CDERR_FINDRESFAILURE);
                return FALSE;
            }
        } else
        {
            hDlginst=COMDLG32_hInstance;
            if (!(hResInfo = FindResourceW(hDlginst, chooseFontW, (LPWSTR)RT_DIALOG)))
            {
                COMDLG32_SetCommDlgExtendedError(CDERR_FINDRESFAILURE);
                return FALSE;
            }
        }
        if (!(hDlgTmpl = LoadResource(hDlginst, hResInfo )) ||
                !(template = LockResource( hDlgTmpl )))
        {
            COMDLG32_SetCommDlgExtendedError(CDERR_LOADRESFAILURE);
            return FALSE;
        }
    }
    if (TRACE_ON(commdlg))
        _dump_cf_flags(lpChFont->Flags);
    if (lpChFont->Flags & (CF_SELECTSCRIPT | CF_NOVERTFONTS ))
        FIXME(": unimplemented flag (ignored)\n");

    return DialogBoxIndirectParamA(COMDLG32_hInstance, template,
            lpChFont->hwndOwner, FormatCharDlgProcA, (LPARAM)lpChFont );
}

#define TEXT_EXTRAS 4
#define TEXT_COLORS 16

static const COLORREF textcolors[TEXT_COLORS]=
{
    0x00000000L,0x00000080L,0x00008000L,0x00008080L,
    0x00800000L,0x00800080L,0x00808000L,0x00808080L,
    0x00c0c0c0L,0x000000ffL,0x0000ff00L,0x0000ffffL,
    0x00ff0000L,0x00ff00ffL,0x00ffff00L,0x00FFFFFFL
};

/***********************************************************************
 *                          CFn_HookCallChk32                 [internal]
 */
static BOOL CFn_HookCallChk32(const CHOOSEFONTW *lpcf)
{
    if (lpcf)
        if(lpcf->Flags & CF_ENABLEHOOK)
            if (lpcf->lpfnHook)
                return TRUE;
    return FALSE;
}

/*************************************************************************
 *              AddFontFamily                               [internal]
 */
INT AddFontFamily(const ENUMLOGFONTEXW *lpElfex, const NEWTEXTMETRICEXW *lpNTM,
        UINT nFontType, const CHOOSEFONTW *lpcf, HWND hwnd, LPCFn_ENUMSTRUCT e)
{
    int i;
    WORD w;
    const LOGFONTW *lplf = &(lpElfex->elfLogFont);

    TRACE("font=%s (nFontType=%d)\n", debugstr_w(lplf->lfFaceName), nFontType);

    if (lpcf->Flags & CF_FIXEDPITCHONLY)
        if (!(lplf->lfPitchAndFamily & FIXED_PITCH))
            return 1;
    if (lpcf->Flags & CF_ANSIONLY)
        if (lplf->lfCharSet != ANSI_CHARSET)
            return 1;
    if (lpcf->Flags & CF_TTONLY)
        if (!(nFontType & TRUETYPE_FONTTYPE))
            return 1;

    if (e) e->added++;

    i=SendMessageW(hwnd, CB_FINDSTRINGEXACT, 0, (LPARAM)lplf->lfFaceName);
    if (i == CB_ERR) {
        i = SendMessageW(hwnd, CB_ADDSTRING, 0, (LPARAM)lplf->lfFaceName);
        if( i != CB_ERR) {
            /* store some important font information */
            w = (lplf->lfPitchAndFamily) << 8 |
                (HIWORD(lpNTM->ntmTm.ntmFlags) & 0xff);
            SendMessageW(hwnd, CB_SETITEMDATA, i, MAKELONG(nFontType,w));
        }
    }
    return 1;
}

/*************************************************************************
 *              FontFamilyEnumProc32                           [internal]
 */
static INT WINAPI FontFamilyEnumProc(const ENUMLOGFONTEXW *lpElfex,
        const TEXTMETRICW *metrics, DWORD dwFontType, LPARAM lParam)
{
    LPCFn_ENUMSTRUCT e;
    e=(LPCFn_ENUMSTRUCT)lParam;
    return AddFontFamily( lpElfex, (const NEWTEXTMETRICEXW *) metrics,
            dwFontType, e->lpcf32w, e->hWnd1, e);
}

/*************************************************************************
 *              SetFontStylesToCombo2                           [internal]
 *
 * Fill font style information into combobox  (without using font.c directly)
 */
static int SetFontStylesToCombo2(HWND hwnd, HDC hdc, const LOGFONTW *lplf)
{
#define FSTYLES 4
    struct FONTSTYLE
    {
        int italic;
        int weight;
        UINT resId;
    };
    static const struct FONTSTYLE fontstyles[FSTYLES]={
        { 0, FW_NORMAL, IDS_FONT_REGULAR },
        { 1, FW_NORMAL, IDS_FONT_ITALIC },
        { 0, FW_BOLD,   IDS_FONT_BOLD },
        { 1, FW_BOLD,   IDS_FONT_BOLD_ITALIC }
    };
    HFONT hf;
    TEXTMETRICW tm;
    int i,j;
    LOGFONTW lf;

    lf = *lplf;

    for (i=0;i<FSTYLES;i++)
    {
        lf.lfItalic=fontstyles[i].italic;
        lf.lfWeight=fontstyles[i].weight;
        hf=CreateFontIndirectW(&lf);
        hf=SelectObject(hdc,hf);
        GetTextMetricsW(hdc,&tm);
        hf=SelectObject(hdc,hf);
        DeleteObject(hf);
                /* font successful created ? */
        if (((fontstyles[i].weight == FW_NORMAL && tm.tmWeight <= FW_MEDIUM) ||
             (fontstyles[i].weight == FW_BOLD && tm.tmWeight > FW_MEDIUM)) &&
            ((tm.tmItalic != 0)==fontstyles[i].italic))
        {
            WCHAR name[64];
            LoadStringW(COMDLG32_hInstance, fontstyles[i].resId, name, 64);
            j=SendMessageW(hwnd,CB_ADDSTRING,0,(LPARAM)name );
            if (j==CB_ERR) return 1;
            j=SendMessageW(hwnd, CB_SETITEMDATA, j,
                           MAKELONG(tm.tmWeight,fontstyles[i].italic));
            if (j==CB_ERR) return 1;
        }
    }
    return 0;
}

/*************************************************************************
 *              AddFontSizeToCombo3                           [internal]
 */
static int AddFontSizeToCombo3(HWND hwnd, UINT h, const CHOOSEFONTW *lpcf)
{
    int j;
    WCHAR buffer[20];
    static const WCHAR strFormat[] = {'%','2','d',0};

    if (  (!(lpcf->Flags & CF_LIMITSIZE))  ||
            ((lpcf->Flags & CF_LIMITSIZE) && (h >= lpcf->nSizeMin) && (h <= lpcf->nSizeMax)))
    {
        wsprintfW(buffer, strFormat, h);
        j=SendMessageW(hwnd, CB_FINDSTRINGEXACT, -1, (LPARAM)buffer);
        if (j==CB_ERR)
        {
            j=SendMessageW(hwnd, CB_ADDSTRING, 0, (LPARAM)buffer);
            if (j!=CB_ERR) j = SendMessageW(hwnd, CB_SETITEMDATA, j, h);
            if (j==CB_ERR) return 1;
        }
    }
    return 0;
}

/*************************************************************************
 *              SetFontSizesToCombo3                           [internal]
 */
static int SetFontSizesToCombo3(HWND hwnd, const CHOOSEFONTW *lpcf)
{
    static const BYTE sizes[]={6,7,8,9,10,11,12,14,16,18,20,22,24,26,28,36,48,72};
    unsigned int i;

    for (i = 0; i < sizeof(sizes)/sizeof(sizes[0]); i++)
        if (AddFontSizeToCombo3(hwnd, sizes[i], lpcf)) return 1;
    return 0;
}

/*************************************************************************
 *              CFn_GetDC                           [internal]
 */
static inline HDC CFn_GetDC(const CHOOSEFONTW *lpcf)
{
    HDC ret = ((lpcf->Flags & CF_PRINTERFONTS) && lpcf->hDC) ?
        lpcf->hDC :
        GetDC(0);
    if(!ret) ERR("HDC failure!!!\n");
    return ret;
}

/*************************************************************************
 *              CFn_ReleaseDC                           [internal]
 */
static inline void CFn_ReleaseDC(const CHOOSEFONTW *lpcf, HDC hdc)
{
        if(!((lpcf->Flags & CF_PRINTERFONTS) && lpcf->hDC))
            ReleaseDC(0, hdc);
}

/***********************************************************************
 *                 AddFontStyle                          [internal]
 */
INT AddFontStyle( const ENUMLOGFONTEXW *lpElfex, const NEWTEXTMETRICEXW *lpNTM,
                UINT nFontType, const CHOOSEFONTW *lpcf, HWND hcmb2, HWND hcmb3,
                HWND hDlg, BOOL iswin16)
{
    int i;
    const LOGFONTW *lplf = &(lpElfex->elfLogFont);
    HWND hcmb5;
    HDC hdc;

    TRACE("(nFontType=%d)\n",nFontType);
    TRACE("  %s h=%d w=%d e=%d o=%d wg=%d i=%d u=%d s=%d"
            " ch=%d op=%d cp=%d q=%d pf=%xh\n",
            debugstr_w(lplf->lfFaceName),lplf->lfHeight,lplf->lfWidth,
            lplf->lfEscapement,lplf->lfOrientation,
            lplf->lfWeight,lplf->lfItalic,lplf->lfUnderline,
            lplf->lfStrikeOut,lplf->lfCharSet, lplf->lfOutPrecision,
            lplf->lfClipPrecision,lplf->lfQuality, lplf->lfPitchAndFamily);
    if (nFontType & RASTER_FONTTYPE)
    {
        INT points;
        if(!(hdc = CFn_GetDC(lpcf))) return 0;
        points = MulDiv( lpNTM->ntmTm.tmHeight - lpNTM->ntmTm.tmInternalLeading,
                72, GetDeviceCaps(hdc, LOGPIXELSY));
        CFn_ReleaseDC(lpcf, hdc);
        i = AddFontSizeToCombo3(hcmb3, points, lpcf);
        if(i) return 0;
    } else if (SetFontSizesToCombo3(hcmb3, lpcf)) return 0;

    if (!SendMessageW(hcmb2, CB_GETCOUNT, 0, 0))
    {
        if(!(hdc = CFn_GetDC(lpcf))) return 0;
        i=SetFontStylesToCombo2(hcmb2,hdc,lplf);
        CFn_ReleaseDC(lpcf, hdc);
        if (i)
            return 0;
    }
    if( iswin16 || !( hcmb5 = GetDlgItem(hDlg, cmb5))) return 1;
    i = SendMessageW( hcmb5, CB_FINDSTRINGEXACT, 0,
                (LPARAM)lpElfex->elfScript);
    if( i == CB_ERR) {
        i = SendMessageW( hcmb5, CB_ADDSTRING, 0,
                (LPARAM)lpElfex->elfScript);
        if( i != CB_ERR)
            SendMessageW( hcmb5, CB_SETITEMDATA, i, lplf->lfCharSet);
    }
    return 1 ;
}

static INT CFn_FitFontSize( HWND hDlg, int points)
{
    int i,n;
    int ret = 0;
    /* look for fitting font size in combobox3 */
    n=SendDlgItemMessageW(hDlg, cmb3, CB_GETCOUNT, 0, 0);
    for (i=0;i<n;i++)
    {
        if (points == (int)SendDlgItemMessageW
                (hDlg,cmb3, CB_GETITEMDATA,i,0))
        {
            SendDlgItemMessageW(hDlg,cmb3,CB_SETCURSEL,i,0);
            SendMessageW(hDlg, WM_COMMAND,
                    MAKEWPARAM(cmb3, CBN_SELCHANGE),
                    (LPARAM)GetDlgItem(hDlg,cmb3));
            ret = 1;
            break;
        }
    }
    return ret;
}

static INT CFn_FitFontStyle( HWND hDlg, LONG packedstyle )
{
    LONG id;
    int i, ret = 0;
    /* look for fitting font style in combobox2 */
    for (i=0;i<TEXT_EXTRAS;i++)
    {
        id = SendDlgItemMessageW(hDlg, cmb2, CB_GETITEMDATA, i, 0);
        if (packedstyle == id)
        {
            SendDlgItemMessageW(hDlg, cmb2, CB_SETCURSEL, i, 0);
            SendMessageW(hDlg, WM_COMMAND, MAKEWPARAM(cmb2, CBN_SELCHANGE),
                    (LPARAM)GetDlgItem(hDlg,cmb2));
            ret = 1;
            break;
        }
    }
    return ret;
}


static INT CFn_FitCharSet( HWND hDlg, int charset )
{
    int i,n,cs;
    /* look for fitting char set in combobox5 */
    n=SendDlgItemMessageW(hDlg, cmb5, CB_GETCOUNT, 0, 0);
    for (i=0;i<n;i++)
    {
        cs =SendDlgItemMessageW(hDlg, cmb5, CB_GETITEMDATA, i, 0);
        if (charset == cs)
        {
            SendDlgItemMessageW(hDlg, cmb5, CB_SETCURSEL, i, 0);
            SendMessageW(hDlg, WM_COMMAND, MAKEWPARAM(cmb5, CBN_SELCHANGE),
                    (LPARAM)GetDlgItem(hDlg,cmb2));
            return 1;
        }
    }
    /* no charset fits: select the first one in the list */
    SendDlgItemMessageW(hDlg, cmb5, CB_SETCURSEL, 0, 0);
    SendMessageW(hDlg, WM_COMMAND, MAKEWPARAM(cmb5, CBN_SELCHANGE),
            (LPARAM)GetDlgItem(hDlg,cmb2));
    return 0;
}

/***********************************************************************
 *                 FontStyleEnumProc32                     [internal]
 */
static INT WINAPI FontStyleEnumProc( const ENUMLOGFONTEXW *lpElfex,
        const TEXTMETRICW *metrics, DWORD dwFontType, LPARAM lParam )
{
    LPCFn_ENUMSTRUCT s=(LPCFn_ENUMSTRUCT)lParam;
    HWND hcmb2=s->hWnd1;
    HWND hcmb3=s->hWnd2;
    HWND hDlg=GetParent(hcmb3);
    return AddFontStyle( lpElfex, (const NEWTEXTMETRICEXW *) metrics,
            dwFontType, s->lpcf32w, hcmb2, hcmb3, hDlg, FALSE);
}

/***********************************************************************
 *           CFn_WMInitDialog                            [internal]
 */
LRESULT CFn_WMInitDialog(HWND hDlg, WPARAM wParam, LPARAM lParam,
                         LPCHOOSEFONTW lpcf)
{
    HDC hdc;
    int i,j,init=0;
    long pstyle;
    CFn_ENUMSTRUCT s;
    LPLOGFONTW lpxx;
    HCURSOR hcursor=SetCursor(LoadCursorW(0,(LPWSTR)IDC_WAIT));
    static const WCHAR strColorName[] = {'[','c','o','l','o','r',' ','n','a','m','e',']',0};

    SetPropW(hDlg, strWineFontData, (HANDLE)lpcf);
    lpxx=lpcf->lpLogFont;
    TRACE("WM_INITDIALOG lParam=%08lX\n", lParam);

    if (lpcf->lStructSize != sizeof(CHOOSEFONTW))
    {
        ERR("structure size failure !!!\n");
        EndDialog (hDlg, 0);
        return FALSE;
    }
    if (!himlTT)
        himlTT = ImageList_LoadImageW( COMDLG32_hInstance, MAKEINTRESOURCEW(38),
                TTBITMAP_XSIZE, 0, CLR_DEFAULT, IMAGE_BITMAP, 0);

    /* Set effect flags */
    if((lpcf->Flags & CF_EFFECTS) && (lpcf->Flags & CF_INITTOLOGFONTSTRUCT))
    {
        if(lpxx->lfUnderline)
            CheckDlgButton(hDlg, chx2, TRUE);
        if(lpxx->lfStrikeOut)
            CheckDlgButton(hDlg, chx1, TRUE);
    }

    if (!(lpcf->Flags & CF_SHOWHELP) || !IsWindow(lpcf->hwndOwner))
        ShowWindow(GetDlgItem(hDlg,pshHelp),SW_HIDE);
    if (!(lpcf->Flags & CF_APPLY))
        ShowWindow(GetDlgItem(hDlg,psh3),SW_HIDE);
    if (lpcf->Flags & CF_NOSCRIPTSEL)
        EnableWindow(GetDlgItem(hDlg,cmb5),FALSE);
    if (lpcf->Flags & CF_EFFECTS)
    {
        for (i=0;i<TEXT_COLORS;i++)
        {
            WCHAR name[30];

            if( LoadStringW(COMDLG32_hInstance, IDS_COLOR_BLACK+i, name,
                        sizeof(name)/sizeof(*name) )==0 )
            {
                memcpy(name, strColorName, sizeof(strColorName));
            }
            j=SendDlgItemMessageW(hDlg, cmb4, CB_ADDSTRING, 0, (LPARAM)name);
            SendDlgItemMessageW(hDlg, cmb4, CB_SETITEMDATA, j, textcolors[i]);
            /* look for a fitting value in color combobox */
            if (textcolors[i]==lpcf->rgbColors)
                SendDlgItemMessageW(hDlg,cmb4, CB_SETCURSEL,j,0);
        }
    }
    else
    {
        ShowWindow(GetDlgItem(hDlg,cmb4),SW_HIDE);
        ShowWindow(GetDlgItem(hDlg,chx1),SW_HIDE);
        ShowWindow(GetDlgItem(hDlg,chx2),SW_HIDE);
        ShowWindow(GetDlgItem(hDlg,grp1),SW_HIDE);
        ShowWindow(GetDlgItem(hDlg,stc4),SW_HIDE);
    }
    if(!(hdc = CFn_GetDC(lpcf)))
    {
        EndDialog (hDlg, 0);
        return FALSE;
    }
    s.hWnd1=GetDlgItem(hDlg,cmb1);
    s.lpcf32w=lpcf;
    do {
        LOGFONTW elf;
        s.added = 0;
        elf.lfCharSet = DEFAULT_CHARSET; /* enum all charsets */
        elf.lfPitchAndFamily = 0;
        elf.lfFaceName[0] = '\0'; /* enum all fonts */
        if (!EnumFontFamiliesExW(hdc, &elf, (FONTENUMPROCW)FontFamilyEnumProc, (LPARAM)&s, 0))
        {
            TRACE("EnumFontFamiliesEx returns 0\n");
            break;
        }
        if (s.added) break;
        if (lpcf->Flags & CF_FIXEDPITCHONLY) {
            FIXME("No font found with fixed pitch only, dropping flag.\n");
            lpcf->Flags &= ~CF_FIXEDPITCHONLY;
            continue;
        }
        if (lpcf->Flags & CF_TTONLY) {
            FIXME("No font found with truetype only, dropping flag.\n");
            lpcf->Flags &= ~CF_TTONLY;
            continue;
        }
        break;
    } while (1);


    if (lpcf->Flags & CF_INITTOLOGFONTSTRUCT)
    {
        /* look for fitting font name in combobox1 */
        j=SendDlgItemMessageW(hDlg,cmb1,CB_FINDSTRING,-1,(LPARAM)lpxx->lfFaceName);
        if (j!=CB_ERR)
        {
            INT height = lpxx->lfHeight < 0 ? -lpxx->lfHeight :
                lpxx->lfHeight;
            INT points;
            int charset = lpxx->lfCharSet;
            points = MulDiv( height, 72, GetDeviceCaps(hdc, LOGPIXELSY));
            pstyle = MAKELONG(lpxx->lfWeight > FW_MEDIUM ? FW_BOLD:
                    FW_NORMAL,lpxx->lfItalic !=0);
            SendDlgItemMessageW(hDlg, cmb1, CB_SETCURSEL, j, 0);
            SendMessageW(hDlg, WM_COMMAND, MAKEWPARAM(cmb1, CBN_SELCHANGE),
                    (LPARAM)GetDlgItem(hDlg,cmb1));
            init=1;
            /* look for fitting font style in combobox2 */
            CFn_FitFontStyle(hDlg, pstyle);
            /* look for fitting font size in combobox3 */
            CFn_FitFontSize(hDlg, points);
            CFn_FitCharSet( hDlg, charset );
        }
    }
    if (!init)
    {
        SendDlgItemMessageW(hDlg,cmb1,CB_SETCURSEL,0,0);
        SendMessageW(hDlg, WM_COMMAND, MAKEWPARAM(cmb1, CBN_SELCHANGE),
                (LPARAM)GetDlgItem(hDlg,cmb1));
        SendDlgItemMessageW(hDlg,cmb2,CB_SETCURSEL,0,0);
        SendMessageW(hDlg, WM_COMMAND, MAKEWPARAM(cmb2, CBN_SELCHANGE),
                (LPARAM)GetDlgItem(hDlg,cmb1));
        SendDlgItemMessageW(hDlg,cmb3,CB_SETCURSEL,0,0);
        SendMessageW(hDlg, WM_COMMAND, MAKEWPARAM(cmb3, CBN_SELCHANGE),
                (LPARAM)GetDlgItem(hDlg,cmb3));
        SendDlgItemMessageW(hDlg,cmb5,CB_SETCURSEL,0,0);
        SendMessageW(hDlg, WM_COMMAND, MAKEWPARAM(cmb5, CBN_SELCHANGE),
                (LPARAM)GetDlgItem(hDlg,cmb5));
    }
    if ((lpcf->Flags & CF_USESTYLE) && lpcf->lpszStyle)
    {
        j=SendDlgItemMessageW(hDlg,cmb2,CB_FINDSTRING,-1,(LPARAM)lpcf->lpszStyle);
        if (j!=CB_ERR)
        {
            j=SendDlgItemMessageW(hDlg,cmb2,CB_SETCURSEL,j,0);
            SendMessageW(hDlg,WM_COMMAND,cmb2,
                    MAKELONG(HWND_16(GetDlgItem(hDlg,cmb2)),CBN_SELCHANGE));
        }
    }
    CFn_ReleaseDC(lpcf, hdc);
    SetCursor(hcursor);
    return TRUE;
}


/***********************************************************************
 *           CFn_WMMeasureItem                           [internal]
 */
LRESULT CFn_WMMeasureItem(HWND hDlg, WPARAM wParam, LPARAM lParam)
{
    HDC hdc;
    HFONT hfontprev;
    TEXTMETRICW tm;
    LPMEASUREITEMSTRUCT lpmi=(LPMEASUREITEMSTRUCT)lParam;
    INT height = 0;

    if (!himlTT)
        himlTT = ImageList_LoadImageW( COMDLG32_hInstance, MAKEINTRESOURCEW(38),
                TTBITMAP_XSIZE, 0, CLR_DEFAULT, IMAGE_BITMAP, 0);
    ImageList_GetIconSize( himlTT, 0, &height);
    lpmi->itemHeight = height + 2;
    /* use MAX of bitmap height and tm.tmHeight .*/
    hdc=GetDC(hDlg);
    if(!hdc) return 0;
    hfontprev = SelectObject( hdc, GetStockObject( SYSTEM_FONT));
    GetTextMetricsW(hdc, &tm);
    if( tm.tmHeight > lpmi->itemHeight) lpmi->itemHeight = tm.tmHeight;
    SelectObject(hdc, hfontprev);
    ReleaseDC(hDlg, hdc);
    return 0;
}


/***********************************************************************
 *           CFn_WMDrawItem                              [internal]
 */
LRESULT CFn_WMDrawItem(HWND hDlg, WPARAM wParam, LPARAM lParam)
{
    HBRUSH hBrush;
    WCHAR buffer[40];
    COLORREF cr, oldText=0, oldBk=0;
    RECT rect;
    int nFontType;
    int idx;
    LPDRAWITEMSTRUCT lpdi = (LPDRAWITEMSTRUCT)lParam;

    if (lpdi->itemID == (UINT)-1)  /* got no items */
        DrawFocusRect(lpdi->hDC, &lpdi->rcItem);
    else
    {
        if (lpdi->CtlType == ODT_COMBOBOX)
        {
            if (lpdi->itemState & ODS_SELECTED)
            {
                hBrush=GetSysColorBrush(COLOR_HIGHLIGHT);
                oldText=SetTextColor(lpdi->hDC, GetSysColor(COLOR_HIGHLIGHTTEXT));
                oldBk=SetBkColor(lpdi->hDC, GetSysColor(COLOR_HIGHLIGHT));
            }  else
            {
                hBrush = SelectObject(lpdi->hDC, GetStockObject(LTGRAY_BRUSH));
                SelectObject(lpdi->hDC, hBrush);
            }
            FillRect(lpdi->hDC, &lpdi->rcItem, hBrush);
        }
        else
            return TRUE;        /* this should never happen */

        rect=lpdi->rcItem;
        switch (lpdi->CtlID)
        {
        case cmb1:
            /* TRACE(commdlg,"WM_Drawitem cmb1\n"); */
            SendMessageW(lpdi->hwndItem, CB_GETLBTEXT, lpdi->itemID,
                         (LPARAM)buffer);
            TextOutW(lpdi->hDC, lpdi->rcItem.left + TTBITMAP_XSIZE + 10,
                     lpdi->rcItem.top, buffer, lstrlenW(buffer));
            nFontType = SendMessageW(lpdi->hwndItem, CB_GETITEMDATA, lpdi->itemID,0L);
            idx = -1;
            if (nFontType & TRUETYPE_FONTTYPE) {
                idx = 0;  /* picture: TT */
                if( nFontType & NTM_TT_OPENTYPE)
                    idx = 2; /* picture: O */
            } else if( nFontType & NTM_PS_OPENTYPE)
                idx = 3; /* picture: O+ps */
            else if( nFontType & NTM_TYPE1)
                idx = 4; /* picture: a */
            else if( nFontType & DEVICE_FONTTYPE)
                idx = 1; /* picture: printer */
            if( idx >= 0)
                ImageList_Draw( himlTT, idx, lpdi->hDC, lpdi->rcItem.left,
                        lpdi->rcItem.top, ILD_TRANSPARENT);
            break;
        case cmb2:
        case cmb3:
            /* TRACE(commdlg,"WM_DRAWITEN cmb2,cmb3\n"); */
        case cmb5:
            SendMessageW(lpdi->hwndItem, CB_GETLBTEXT, lpdi->itemID,
                         (LPARAM)buffer);
            TextOutW(lpdi->hDC, lpdi->rcItem.left,
                     lpdi->rcItem.top, buffer, lstrlenW(buffer));
            break;

        case cmb4:
            /* TRACE(commdlg,"WM_DRAWITEM cmb4 (=COLOR)\n"); */
            SendMessageW(lpdi->hwndItem, CB_GETLBTEXT, lpdi->itemID,
                     (LPARAM)buffer);
            TextOutW(lpdi->hDC, lpdi->rcItem.left +  25+5,
                     lpdi->rcItem.top, buffer, lstrlenW(buffer));
            cr = SendMessageW(lpdi->hwndItem, CB_GETITEMDATA, lpdi->itemID,0L);
            hBrush = CreateSolidBrush(cr);
            if (hBrush)
            {
                hBrush = SelectObject (lpdi->hDC, hBrush) ;
                rect.right=rect.left+25;
                rect.top++;
                rect.left+=5;
                rect.bottom--;
                Rectangle( lpdi->hDC, rect.left, rect.top,
                           rect.right, rect.bottom );
                DeleteObject( SelectObject (lpdi->hDC, hBrush)) ;
            }
            rect=lpdi->rcItem;
            rect.left+=25+5;
            break;

        default:
            return TRUE;  /* this should never happen */
        }
        if (lpdi->itemState & ODS_SELECTED)
        {
            SetTextColor(lpdi->hDC, oldText);
            SetBkColor(lpdi->hDC, oldBk);
        }
    }
    return TRUE;
}

/***********************************************************************
 *           CFn_WMCommand                               [internal]
 */
LRESULT CFn_WMCommand(HWND hDlg, WPARAM wParam, LPARAM lParam,
        LPCHOOSEFONTW lpcf)
{
    int i;
    long l;
    HDC hdc;
    LPLOGFONTW lpxx=lpcf->lpLogFont;

    TRACE("WM_COMMAND wParam=%08X lParam=%08lX\n", (LONG)wParam, lParam);
    switch (LOWORD(wParam))
    {
    case cmb1:
        if (HIWORD(wParam)==CBN_SELCHANGE)
        {
            INT pointsize; /* save current pointsize */
            LONG pstyle;  /* save current style */
            int charset;
            int idx;
            if(!(hdc = CFn_GetDC(lpcf)))
            {
                EndDialog (hDlg, 0);
                return TRUE;
            }
            idx = SendDlgItemMessageW(hDlg, cmb3, CB_GETCURSEL, 0, 0);
            pointsize = (int)SendDlgItemMessageW( hDlg, cmb3, CB_GETITEMDATA,
                    idx, 0);
            idx = SendDlgItemMessageW(hDlg, cmb2, CB_GETCURSEL, 0, 0);
            pstyle = SendDlgItemMessageW(hDlg, cmb2, CB_GETITEMDATA, idx, 0);
            idx = SendDlgItemMessageW(hDlg, cmb5, CB_GETCURSEL, 0, 0);
            charset = SendDlgItemMessageW(hDlg, cmb5, CB_GETITEMDATA, idx, 0);

            SendDlgItemMessageW(hDlg, cmb2, CB_RESETCONTENT, 0, 0);
            SendDlgItemMessageW(hDlg, cmb3, CB_RESETCONTENT, 0, 0);
            SendDlgItemMessageW(hDlg, cmb5, CB_RESETCONTENT, 0, 0);
            i=SendDlgItemMessageW(hDlg, cmb1, CB_GETCURSEL, 0, 0);
            if (i!=CB_ERR)
            {
                HCURSOR hcursor=SetCursor(LoadCursorW(0,(LPWSTR)IDC_WAIT));
                CFn_ENUMSTRUCT s;
                LOGFONTW enumlf;
                SendDlgItemMessageW(hDlg, cmb1, CB_GETLBTEXT, i,
                                    (LPARAM)enumlf.lfFaceName);
                TRACE("WM_COMMAND/cmb1 =>%s\n", debugstr_w(enumlf.lfFaceName));
                s.hWnd1=GetDlgItem(hDlg, cmb2);
                s.hWnd2=GetDlgItem(hDlg, cmb3);
                s.lpcf32w=lpcf;
                enumlf.lfCharSet = DEFAULT_CHARSET; /* enum all charsets */
                enumlf.lfPitchAndFamily = 0;
                EnumFontFamiliesExW(hdc, &enumlf,
                        (FONTENUMPROCW)FontStyleEnumProc, (LPARAM)&s, 0);
                CFn_FitFontStyle(hDlg, pstyle);
                if( pointsize != CB_ERR) CFn_FitFontSize(hDlg, pointsize);
                if( charset != CB_ERR) CFn_FitCharSet( hDlg, charset );
                SetCursor(hcursor);
            }
            CFn_ReleaseDC(lpcf, hdc);
        }
    case chx1:
    case chx2:
    case cmb2:
    case cmb3:
    case cmb5:
        if (HIWORD(wParam)==CBN_SELCHANGE || HIWORD(wParam)== BN_CLICKED )
        {
            WCHAR str[256];
            WINDOWINFO wininfo;

            TRACE("WM_COMMAND/cmb2,3 =%08lX\n", lParam);
            i=SendDlgItemMessageW(hDlg,cmb1,CB_GETCURSEL,0,0);
            if (i==CB_ERR)
                i=GetDlgItemTextW( hDlg, cmb1, str, 256 );
            else
            {
                SendDlgItemMessageW(hDlg,cmb1,CB_GETLBTEXT,i,
                                    (LPARAM)str);
                l=SendDlgItemMessageW(hDlg,cmb1,CB_GETITEMDATA,i,0);
                lpcf->nFontType = LOWORD(l);
                /* FIXME:   lpcf->nFontType |= ....  SIMULATED_FONTTYPE and so */
                /* same value reported to the EnumFonts
                   call back with the extra FONTTYPE_...  bits added */
                lpxx->lfPitchAndFamily = HIWORD(l) >> 8;
            }
            lstrcpynW(lpxx->lfFaceName, str, sizeof(lpxx->lfFaceName)/sizeof(lpxx->lfFaceName[0]));
            i=SendDlgItemMessageW(hDlg, cmb2, CB_GETCURSEL, 0, 0);
            if (i!=CB_ERR)
            {
                l=SendDlgItemMessageW(hDlg, cmb2, CB_GETITEMDATA, i, 0);
                if (0!=(lpxx->lfItalic=HIWORD(l)))
                    lpcf->nFontType |= ITALIC_FONTTYPE;
                if ((lpxx->lfWeight=LOWORD(l)) > FW_MEDIUM)
                    lpcf->nFontType |= BOLD_FONTTYPE;
            }
            i=SendDlgItemMessageW(hDlg, cmb3, CB_GETCURSEL, 0, 0);
            if( i != CB_ERR)
                lpcf->iPointSize = 10 * LOWORD(SendDlgItemMessageW(hDlg, cmb3,
                            CB_GETITEMDATA , i, 0));
            else
                lpcf->iPointSize = 100;
            hdc = CFn_GetDC(lpcf);
            if( hdc)
            {
                lpxx->lfHeight = - MulDiv( lpcf->iPointSize ,
                        GetDeviceCaps(hdc, LOGPIXELSY), 720);
                CFn_ReleaseDC(lpcf, hdc);
            } else
                lpxx->lfHeight = -lpcf->iPointSize / 10;
            i=SendDlgItemMessageW(hDlg, cmb5, CB_GETCURSEL, 0, 0);
            if (i!=CB_ERR)
                lpxx->lfCharSet=SendDlgItemMessageW(hDlg, cmb5, CB_GETITEMDATA, i, 0);
            else
                lpxx->lfCharSet = DEFAULT_CHARSET;
            lpxx->lfStrikeOut=IsDlgButtonChecked(hDlg,chx1);
            lpxx->lfUnderline=IsDlgButtonChecked(hDlg,chx2);
            lpxx->lfWidth=lpxx->lfOrientation=lpxx->lfEscapement=0;
            lpxx->lfOutPrecision=OUT_DEFAULT_PRECIS;
            lpxx->lfClipPrecision=CLIP_DEFAULT_PRECIS;
            lpxx->lfQuality=DEFAULT_QUALITY;

            wininfo.cbSize=sizeof(wininfo);

            if( GetWindowInfo( GetDlgItem( hDlg, stc5), &wininfo ) )
            {
                MapWindowPoints( 0, hDlg, (LPPOINT) &wininfo.rcWindow, 2);
                InvalidateRect( hDlg, &wininfo.rcWindow, TRUE );
            }
        }
        break;

    case cmb4:
        i=SendDlgItemMessageW(hDlg, cmb4, CB_GETCURSEL, 0, 0);
        if (i!=CB_ERR)
        {
            WINDOWINFO wininfo;

            lpcf->rgbColors = SendDlgItemMessageW(hDlg, cmb4, CB_GETITEMDATA, i, 0);
            wininfo.cbSize=sizeof(wininfo);

            if( GetWindowInfo( GetDlgItem( hDlg, stc5), &wininfo ) )
            {
                MapWindowPoints( 0, hDlg, (LPPOINT) &wininfo.rcWindow, 2);
                InvalidateRect( hDlg, &wininfo.rcWindow, TRUE );
            }
        }
        break;

    case psh15:
        i=RegisterWindowMessageW( HELPMSGSTRINGW );
        if (lpcf->hwndOwner)
            SendMessageW(lpcf->hwndOwner, i, 0, (LPARAM)GetPropW(hDlg, strWineFontData));
        /* if (CFn_HookCallChk(lpcf))
           CallWindowProc16(lpcf->lpfnHook,hDlg,WM_COMMAND,psh15,(LPARAM)lpcf);*/
        break;

    case IDOK:
        if (  (!(lpcf->Flags & CF_LIMITSIZE))  ||
              ( (lpcf->Flags & CF_LIMITSIZE) &&
                (lpcf->iPointSize >= 10 * lpcf->nSizeMin) &&
                (lpcf->iPointSize <= 10 * lpcf->nSizeMax)))
            EndDialog(hDlg, TRUE);
        else
        {
            WCHAR buffer[80];
            WCHAR format[80];
            LoadStringW(COMDLG32_hInstance, IDS_FONT_SIZE, format, sizeof(format)/sizeof(WCHAR));
            wsprintfW(buffer, format, lpcf->nSizeMin,lpcf->nSizeMax);
            MessageBoxW(hDlg, buffer, NULL, MB_OK);
        }
        return(TRUE);
    case IDCANCEL:
        EndDialog(hDlg, FALSE);
        return(TRUE);
    }
    return(FALSE);
}

static LRESULT CFn_WMDestroy(HWND hwnd, WPARAM wParam, LPARAM lParam, LPCHOOSEFONTW lpcfw)
{
    LPCHOOSEFONTA lpcfa;
    LPSTR lpszStyle;
    LPLOGFONTA lpLogFonta;
    int len;

    lpcfa = GetPropW(hwnd, strWineFontData_a);
    lpLogFonta = lpcfa->lpLogFont;
    lpszStyle = lpcfa->lpszStyle;
    memcpy(lpcfa, lpcfw, sizeof(CHOOSEFONTA));
    lpcfa->lpLogFont = lpLogFonta;
    lpcfa->lpszStyle = lpszStyle;
    memcpy(lpcfa->lpLogFont, lpcfw->lpLogFont, sizeof(LOGFONTA));
    WideCharToMultiByte(CP_ACP, 0, lpcfw->lpLogFont->lfFaceName,
                        LF_FACESIZE, lpcfa->lpLogFont->lfFaceName, LF_FACESIZE, 0, 0);

    if((lpcfw->Flags & CF_USESTYLE) && lpcfw->lpszStyle) {
        len = WideCharToMultiByte(CP_ACP, 0, lpcfw->lpszStyle, -1, NULL, 0, 0, 0);
        WideCharToMultiByte(CP_ACP, 0, lpcfw->lpszStyle, -1, lpcfa->lpszStyle, len, 0, 0);
        HeapFree(GetProcessHeap(), 0, lpcfw->lpszStyle);
    }

    HeapFree(GetProcessHeap(), 0, lpcfw->lpLogFont);
    HeapFree(GetProcessHeap(), 0, lpcfw);
    SetPropW(hwnd, strWineFontData, 0);

    return TRUE;
}

LRESULT CFn_WMPaint(HWND hDlg, WPARAM wParam, LPARAM lParam, const CHOOSEFONTW *lpcf)
{
    WINDOWINFO info;

    info.cbSize=sizeof(info);
    if( GetWindowInfo( GetDlgItem( hDlg, stc5), &info ) )
    {
        PAINTSTRUCT ps;
        HDC hdc;
        HPEN hOrigPen;
        HFONT hOrigFont;
        LOGFONTW lf = *(lpcf->lpLogFont);

        MapWindowPoints( 0, hDlg, (LPPOINT) &info.rcWindow, 2);
        hdc = BeginPaint( hDlg, &ps );

        TRACE("erase %d, rect=(%d,%d)-(%d,%d)\n", ps.fErase,
              ps.rcPaint.left, ps.rcPaint.top,
              ps.rcPaint.right, ps.rcPaint.bottom);

        /* Paint frame */
        MoveToEx( hdc, info.rcWindow.left, info.rcWindow.bottom, NULL );
        hOrigPen=SelectObject( hdc, CreatePen( PS_SOLID, 2,
                                               GetSysColor( COLOR_3DSHADOW ) ));
        LineTo( hdc, info.rcWindow.left, info.rcWindow.top );
        LineTo( hdc, info.rcWindow.right, info.rcWindow.top );
        DeleteObject(SelectObject( hdc, CreatePen( PS_SOLID, 2,
                                                   GetSysColor( COLOR_3DLIGHT ) )));
        LineTo( hdc, info.rcWindow.right, info.rcWindow.bottom );
        LineTo( hdc, info.rcWindow.left, info.rcWindow.bottom );
        DeleteObject(SelectObject( hdc, hOrigPen ));

        /* Draw the sample text itself */
        info.rcWindow.right--;
        info.rcWindow.bottom--;
        info.rcWindow.top++;
        info.rcWindow.left++;
        hOrigFont = SelectObject( hdc, CreateFontIndirectW( &lf ) );
        SetTextColor( hdc, lpcf->rgbColors );

        DrawTextW( hdc,
                sample_lang_text[CHARSET_ORDER[lpcf->lpLogFont->lfCharSet]],
                -1, &info.rcWindow, DT_CENTER|DT_VCENTER|DT_SINGLELINE );

        DeleteObject(SelectObject( hdc, hOrigFont ));
        EndPaint( hDlg, &ps );
    }
    return FALSE;
}

/***********************************************************************
 *           FormatCharDlgProcA   [internal]
 */
INT_PTR CALLBACK FormatCharDlgProcA(HWND hDlg, UINT uMsg, WPARAM wParam,
        LPARAM lParam)
{
    LPCHOOSEFONTW lpcfw;
    LPCHOOSEFONTA lpcfa;
    INT_PTR res = FALSE;
    int len;

    if (uMsg!=WM_INITDIALOG) {
        lpcfw = GetPropW(hDlg, strWineFontData);
        if (!lpcfw)
            return FALSE;
        if (CFn_HookCallChk32(lpcfw))
            res=CallWindowProcA((WNDPROC)lpcfw->lpfnHook, hDlg, uMsg, wParam, lParam);
        if (res)
            return res;
    } else {
        lpcfa=(LPCHOOSEFONTA)lParam;
        SetPropW(hDlg, strWineFontData_a, (HANDLE)lParam);

        lpcfw = HeapAlloc(GetProcessHeap(), 0, sizeof(CHOOSEFONTW));
        memcpy(lpcfw, lpcfa, sizeof(CHOOSEFONTA));
        lpcfw->lpLogFont = HeapAlloc(GetProcessHeap(), 0, sizeof(LOGFONTW));
        memcpy(lpcfw->lpLogFont, lpcfa->lpLogFont, sizeof(LOGFONTA));
        MultiByteToWideChar(CP_ACP, 0, lpcfa->lpLogFont->lfFaceName,
                            LF_FACESIZE, lpcfw->lpLogFont->lfFaceName, LF_FACESIZE);

        if((lpcfa->Flags & CF_USESTYLE) && lpcfa->lpszStyle)  {
            len = MultiByteToWideChar(CP_ACP, 0, lpcfa->lpszStyle, -1, NULL, 0);
            lpcfw->lpszStyle = HeapAlloc(GetProcessHeap(), 0, len*sizeof(WCHAR));
            MultiByteToWideChar(CP_ACP, 0, lpcfa->lpszStyle, -1, lpcfw->lpszStyle, len);
        }

        if (!CFn_WMInitDialog(hDlg, wParam, lParam, lpcfw))
        {
            TRACE("CFn_WMInitDialog returned FALSE\n");
            return FALSE;
        }
        if (CFn_HookCallChk32(lpcfw))
            return CallWindowProcA((WNDPROC)lpcfa->lpfnHook,hDlg,WM_INITDIALOG,wParam,lParam);
    }
    switch (uMsg)
    {
    case WM_MEASUREITEM:
        return CFn_WMMeasureItem(hDlg, wParam, lParam);
    case WM_DRAWITEM:
        return CFn_WMDrawItem(hDlg, wParam, lParam);
    case WM_COMMAND:
        return CFn_WMCommand(hDlg, wParam, lParam, lpcfw);
    case WM_DESTROY:
        return CFn_WMDestroy(hDlg, wParam, lParam, lpcfw);
    case WM_CHOOSEFONT_GETLOGFONT:
        TRACE("WM_CHOOSEFONT_GETLOGFONT lParam=%08lX\n", lParam);
        FIXME("current logfont back to caller\n");
        break;
    case WM_PAINT:
        return CFn_WMPaint(hDlg, wParam, lParam, lpcfw);
    }
    return res;
}

/***********************************************************************
 *           FormatCharDlgProcW   [internal]
 */
INT_PTR CALLBACK FormatCharDlgProcW(HWND hDlg, UINT uMsg, WPARAM wParam,
        LPARAM lParam)
{
    LPCHOOSEFONTW lpcf;
    INT_PTR res = FALSE;

    if (uMsg!=WM_INITDIALOG)
    {
        lpcf= GetPropW(hDlg, strWineFontData);
        if (!lpcf)
            return FALSE;
        if (CFn_HookCallChk32(lpcf))
            res=CallWindowProcW((WNDPROC)lpcf->lpfnHook, hDlg, uMsg, wParam, lParam);
        if (res)
            return res;
    }
    else
    {
        lpcf=(LPCHOOSEFONTW)lParam;
        if (!CFn_WMInitDialog(hDlg, wParam, lParam, lpcf))
        {
            TRACE("CFn_WMInitDialog returned FALSE\n");
            return FALSE;
        }
        if (CFn_HookCallChk32(lpcf))
            return CallWindowProcW((WNDPROC)lpcf->lpfnHook,hDlg,WM_INITDIALOG,wParam,lParam);
    }
    switch (uMsg)
    {
    case WM_MEASUREITEM:
        return CFn_WMMeasureItem(hDlg, wParam, lParam);
    case WM_DRAWITEM:
        return CFn_WMDrawItem(hDlg, wParam, lParam);
    case WM_COMMAND:
        return CFn_WMCommand(hDlg, wParam, lParam, lpcf);
    case WM_DESTROY:
        return TRUE;
    case WM_CHOOSEFONT_GETLOGFONT:
        TRACE("WM_CHOOSEFONT_GETLOGFONT lParam=%08lX\n", lParam);
        FIXME("current logfont back to caller\n");
        break;
    case WM_PAINT:
        return CFn_WMPaint(hDlg, wParam, lParam, lpcf);
    }
    return res;
}
