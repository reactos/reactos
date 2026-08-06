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
#include "wine/heap.h"
#include "cderr.h"
#include "cdlg.h"

WINE_DEFAULT_DEBUG_CHANNEL(commdlg);

typedef struct
{
  HWND hWnd1;
  HWND hWnd2;
  LPCHOOSEFONTW lpcf32w;
  int  added;
} CFn_ENUMSTRUCT, *LPCFn_ENUMSTRUCT;


static const WCHAR strWineFontData[] = L"__WINE_FONTDLGDATA";
static const WCHAR strWineFontData_a[] = L"__WINE_FONTDLGDATA_A";

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

static const WCHAR * const sample_lang_text[]={
    L"AaBbYyZz",                                /* Western and default */
    L"Symbol",                                  /* Symbol */
    L"Aa\x3042\x3041\x30a2\x30a1\x4e9c\x5b87",  /* Shift JIS */
    L"\xac00\xb098\xb2e4" "AaBYyZz",            /* Hangul */
    L"\x5fae\x8f6f\x4e2d\x6587\x8f6f\x4ef6",    /* GB2312 */
    L"\x4e2d\x6587\x5b57\x578b\x7bc4\x4f8b",    /* BIG5 */
    L"AaBb\x0391\x03b1\x0392\x03b2",            /* Greek */
    L"AaBb\x011e\x011f\x015e\x015f",            /* Turkish */
    L"AaBb\x05e0\x05e1\x05e9\x05ea",            /* Hebrew */
    L"AaBb\x0627\x0628\x062c\x062f\x0647\x0648\x0632", /* Arabic */
    L"AaBbYyZz",                                /* Baltic */
    L"AaBb\x01a0\x01a1\x01af\x01b0",            /* Vietnamese */
    L"AaBb\x0411\x0431\x0424\x0444",            /* Cyrillic */
    L"AaBb\x00c1\x00e1\x00d4\x00f4",            /* East European */
    L"AaBb\x0e2d\x0e31\x0e01\x0e29\x0e23\x0e44\x0e17\x0e22", /* Thai */
    L"\xac00\xb098\xb2e4" "AaBYyZz",            /* Johab */
    L"AaBbYyZz",                                /* Mac */
    L"AaBb\x00f8\x00f1\x00fd",                  /* OEM */
    /* the following character sets actually behave different (Win2K observation):
     * the sample string is 'sticky': it uses the sample string of the previous
     * selected character set. That behaviour looks like some default, which is
     * not (yet) implemented. */
    L"AaBb",                                    /* VISCII */
    L"AaBb",                                    /* TCVN */
    L"AaBb",                                    /* KOI-8 */
    L"AaBb",                                    /* ISO-8859-3 */
    L"AaBb",                                    /* ISO-8859-4 */
    L"AaBb",                                    /* ISO-8859-10 */
    L"AaBb"                                     /* Celtic */
};


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

static void _dump_cf_flags(DWORD cflags)
{
    unsigned int i;

    for (i = 0; i < ARRAY_SIZE(cfflags); i++)
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
        template=lpChFont->hInstance;
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
            if (!(hResInfo = FindResourceW(hDlginst, L"CHOOSE_FONT", (LPWSTR)RT_DIALOG)))
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

    if (lpChFont->Flags & CF_SELECTSCRIPT)
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
        template=lpChFont->hInstance;
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
            if (!(hResInfo = FindResourceW(hDlginst, L"CHOOSE_FONT", (LPWSTR)RT_DIALOG)))
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
    if (lpChFont->Flags & CF_SELECTSCRIPT)
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
static INT AddFontFamily(const ENUMLOGFONTEXW *lpElfex, const NEWTEXTMETRICEXW *lpNTM,
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
    if (lpcf->Flags & CF_NOVERTFONTS)
        if (lplf->lfFaceName[0] == '@')
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
static BOOL SetFontStylesToCombo2(HWND hwnd, HDC hdc, const LOGFONTW *lplf)
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
            if (j==CB_ERR) return TRUE;
            j=SendMessageW(hwnd, CB_SETITEMDATA, j,
                           MAKELONG(tm.tmWeight,fontstyles[i].italic));
            if (j==CB_ERR) return TRUE;
        }
    }
    return FALSE;
}

/*************************************************************************
 *              AddFontSizeToCombo3                           [internal]
 */
static BOOL AddFontSizeToCombo3(HWND hwnd, UINT h, const CHOOSEFONTW *lpcf)
{
    int j;
    WCHAR buffer[20];

    if (  (!(lpcf->Flags & CF_LIMITSIZE))  ||
            ((lpcf->Flags & CF_LIMITSIZE) && (h >= lpcf->nSizeMin) && (h <= lpcf->nSizeMax)))
    {
        swprintf(buffer, ARRAY_SIZE(buffer), L"%d", h);
        j=SendMessageW(hwnd, CB_FINDSTRINGEXACT, -1, (LPARAM)buffer);
        if (j==CB_ERR)
        {
            j=SendMessageW(hwnd, CB_INSERTSTRING, -1, (LPARAM)buffer);
            if (j!=CB_ERR) j = SendMessageW(hwnd, CB_SETITEMDATA, j, h);
            if (j==CB_ERR) return TRUE;
        }
    }
    return FALSE;
}

/*************************************************************************
 *              SetFontSizesToCombo3                           [internal]
 */
static BOOL SetFontSizesToCombo3(HWND hwnd, const CHOOSEFONTW *lpcf)
{
    static const BYTE sizes[]={6,7,8,9,10,11,12,14,16,18,20,22,24,26,28,36,48,72};
    unsigned int i;

    for (i = 0; i < ARRAY_SIZE(sizes); i++)
        if (AddFontSizeToCombo3(hwnd, sizes[i], lpcf)) return TRUE;
    return FALSE;
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
 *              GetScreenDPI                           [internal]
 */
static inline int GetScreenDPI(void)
{
    HDC hdc;
    int result;

    hdc = GetDC(0);
    result = GetDeviceCaps(hdc, LOGPIXELSY);
    ReleaseDC(0, hdc);

    return result;
}

/*************************************************************************
 *              CFn_ReleaseDC                           [internal]
 */
static inline void CFn_ReleaseDC(const CHOOSEFONTW *lpcf, HDC hdc)
{
        if(!((lpcf->Flags & CF_PRINTERFONTS) && lpcf->hDC))
            ReleaseDC(0, hdc);
}

/*************************************************************************
 *              select_combo_item                           [internal]
 */
static void select_combo_item( HWND dialog, int id, int sel )
{
    HWND combo = GetDlgItem( dialog, id );
    SendMessageW( combo, CB_SETCURSEL, sel, 0 );
    SendMessageW( dialog, WM_COMMAND, MAKEWPARAM( id, CBN_SELCHANGE ), (LPARAM)combo );
}

/***********************************************************************
 *                 AddFontStyle                          [internal]
 */
static INT AddFontStyle( const ENUMLOGFONTEXW *lpElfex, const NEWTEXTMETRICEXW *lpNTM,
                         UINT nFontType, const CHOOSEFONTW *lpcf, HWND hcmb2, HWND hcmb3, HWND hDlg)
{
    int i;
    const LOGFONTW *lplf = &(lpElfex->elfLogFont);
    HWND hcmb5;
    HDC hdc;

    TRACE("(nFontType=%d)\n",nFontType);
    TRACE("  %s h=%ld w=%ld e=%ld o=%ld wg=%ld i=%d u=%d s=%d"
            " ch=%d op=%d cp=%d q=%d pf=%xh\n",
            debugstr_w(lplf->lfFaceName),lplf->lfHeight,lplf->lfWidth,
            lplf->lfEscapement,lplf->lfOrientation,
            lplf->lfWeight,lplf->lfItalic,lplf->lfUnderline,
            lplf->lfStrikeOut,lplf->lfCharSet, lplf->lfOutPrecision,
            lplf->lfClipPrecision,lplf->lfQuality, lplf->lfPitchAndFamily);
    if (nFontType & RASTER_FONTTYPE)
    {
        INT points;
        points = MulDiv( lpNTM->ntmTm.tmHeight - lpNTM->ntmTm.tmInternalLeading,
                72, GetScreenDPI());
        if (AddFontSizeToCombo3(hcmb3, points, lpcf))
            return 0;
    } else if (SetFontSizesToCombo3(hcmb3, lpcf)) return 0;

    if (!SendMessageW(hcmb2, CB_GETCOUNT, 0, 0))
    {
        BOOL res;
        if(!(hdc = CFn_GetDC(lpcf))) return 0;
        res = SetFontStylesToCombo2(hcmb2,hdc,lplf);
        CFn_ReleaseDC(lpcf, hdc);
        if (res)
            return 0;
    }
    if (!( hcmb5 = GetDlgItem(hDlg, cmb5))) return 1;
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

static void CFn_FitFontSize( HWND hDlg, int points)
{
    int i,n;

    /* look for fitting font size in combobox3 */
    n=SendDlgItemMessageW(hDlg, cmb3, CB_GETCOUNT, 0, 0);
    for (i=0;i<n;i++)
    {
        if (points == (int)SendDlgItemMessageW
                (hDlg,cmb3, CB_GETITEMDATA,i,0))
        {
            select_combo_item( hDlg, cmb3, i );
            return;
        }
    }

    /* no default matching size, set text manually */
    SetDlgItemInt(hDlg, cmb3, points, TRUE);
}

static BOOL CFn_FitFontStyle( HWND hDlg, LONG packedstyle )
{
    LONG id;
    int i;
    /* look for fitting font style in combobox2 */
    for (i=0;i<TEXT_EXTRAS;i++)
    {
        id = SendDlgItemMessageW(hDlg, cmb2, CB_GETITEMDATA, i, 0);
        if (packedstyle == id)
        {
            select_combo_item( hDlg, cmb2, i );
            return TRUE;
        }
    }
    return FALSE;
}


static BOOL CFn_FitCharSet( HWND hDlg, int charset )
{
    int i,n,cs;
    /* look for fitting char set in combobox5 */
    n=SendDlgItemMessageW(hDlg, cmb5, CB_GETCOUNT, 0, 0);
    for (i=0;i<n;i++)
    {
        cs =SendDlgItemMessageW(hDlg, cmb5, CB_GETITEMDATA, i, 0);
        if (charset == cs)
        {
            select_combo_item( hDlg, cmb5, i );
            return TRUE;
        }
    }
    /* no charset fits: select the first one in the list */
    select_combo_item( hDlg, cmb5, 0 );
    return FALSE;
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
                         dwFontType, s->lpcf32w, hcmb2, hcmb3, hDlg);
}

/***********************************************************************
 *           CFn_WMInitDialog                            [internal]
 */
static LRESULT CFn_WMInitDialog(HWND hDlg, LPARAM lParam, LPCHOOSEFONTW lpcf)
{
    HDC hdc;
    int i,j;
    BOOL init = FALSE;
    long pstyle;
    CFn_ENUMSTRUCT s;
    LPLOGFONTW lpxx;
    HCURSOR hcursor=SetCursor(LoadCursorW(0,(LPWSTR)IDC_WAIT));

    SetPropW(hDlg, strWineFontData, lpcf);
    lpxx=lpcf->lpLogFont;
    TRACE("WM_INITDIALOG lParam=%08IX\n", lParam);

    if (lpcf->lStructSize != sizeof(CHOOSEFONTW))
    {
        ERR("structure size failure!!!\n");
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

            if (LoadStringW(COMDLG32_hInstance, IDS_COLOR_BLACK+i, name, ARRAY_SIZE(name)) == 0)
                lstrcpyW(name, L"[color name]");
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
            points = MulDiv( height, 72, GetScreenDPI());
            pstyle = MAKELONG(lpxx->lfWeight > FW_MEDIUM ? FW_BOLD:
                    FW_NORMAL,lpxx->lfItalic !=0);
            select_combo_item( hDlg, cmb1, j );
            init = TRUE;
            /* look for fitting font style in combobox2 */
            CFn_FitFontStyle(hDlg, pstyle);
            /* look for fitting font size in combobox3 */
            CFn_FitFontSize(hDlg, points);
            CFn_FitCharSet( hDlg, charset );
        }
    }
    if (!init)
    {
        select_combo_item( hDlg, cmb1, 0 );
        select_combo_item( hDlg, cmb2, 0 );
        select_combo_item( hDlg, cmb3, 0 );
        select_combo_item( hDlg, cmb5, 0 );
    }
    /* limit text length user can type in as font size */
    SendDlgItemMessageW(hDlg, cmb3, CB_LIMITTEXT, 5, 0);

    if ((lpcf->Flags & CF_USESTYLE) && lpcf->lpszStyle)
    {
        j=SendDlgItemMessageW(hDlg,cmb2,CB_FINDSTRING,-1,(LPARAM)lpcf->lpszStyle);
        if (j!=CB_ERR) select_combo_item( hDlg, cmb2, j );
    }
    CFn_ReleaseDC(lpcf, hdc);
    SetCursor(hcursor);
    return TRUE;
}


/***********************************************************************
 *           CFn_WMMeasureItem                           [internal]
 */
static LRESULT CFn_WMMeasureItem(HWND hDlg, LPARAM lParam)
{
    HDC hdc;
    HFONT hfontprev;
    TEXTMETRICW tm;
    LPMEASUREITEMSTRUCT lpmi=(LPMEASUREITEMSTRUCT)lParam;
    INT height = 0, cx;

    if (!himlTT)
        himlTT = ImageList_LoadImageW( COMDLG32_hInstance, MAKEINTRESOURCEW(38),
                TTBITMAP_XSIZE, 0, CLR_DEFAULT, IMAGE_BITMAP, 0);
    ImageList_GetIconSize( himlTT, &cx, &height);
    lpmi->itemHeight = height + 2;
    /* use MAX of bitmap height and tm.tmHeight .*/
    hdc=GetDC(hDlg);
    if(!hdc) return 0;
    hfontprev = SelectObject( hdc, (HFONT)SendMessageW( hDlg, WM_GETFONT, 0, 0 ));
    GetTextMetricsW(hdc, &tm);
    if( tm.tmHeight > lpmi->itemHeight) lpmi->itemHeight = tm.tmHeight;
    SelectObject(hdc, hfontprev);
    ReleaseDC(hDlg, hdc);
    return 0;
}


/***********************************************************************
 *           CFn_WMDrawItem                              [internal]
 */
static LRESULT CFn_WMDrawItem(LPARAM lParam)
{
    HBRUSH hBrush;
    WCHAR buffer[40];
    COLORREF cr, oldText=0, oldBk=0;
    RECT rect;
    int nFontType;
    int cx, cy, idx;
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
            ImageList_GetIconSize( himlTT, &cx, &cy);
            SendMessageW(lpdi->hwndItem, CB_GETLBTEXT, lpdi->itemID,
                         (LPARAM)buffer);
            TextOutW(lpdi->hDC, lpdi->rcItem.left + cx + 4,
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
                                (lpdi->rcItem.top + lpdi->rcItem.bottom - cy) / 2, ILD_TRANSPARENT);
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

static INT get_dialog_font_point_size(HWND hDlg, CHOOSEFONTW *cf)
{
    BOOL invalid_size = FALSE;
    INT i, size;

    i = SendDlgItemMessageW(hDlg, cmb3, CB_GETCURSEL, 0, 0);
    if (i != CB_ERR)
        size = LOWORD(SendDlgItemMessageW(hDlg, cmb3, CB_GETITEMDATA , i, 0));
    else
    {
        WCHAR buffW[8], *endptrW;

        GetDlgItemTextW(hDlg, cmb3, buffW, ARRAY_SIZE(buffW));
        size = wcstol(buffW, &endptrW, 10);
        invalid_size = size == 0 && *endptrW;

        if (size == 0)
            size = 10;
    }

    cf->iPointSize = 10 * size;
    cf->lpLogFont->lfHeight = -MulDiv(cf->iPointSize, GetScreenDPI(), 720);
    return invalid_size ? -1 : size;
}

/***********************************************************************
 *           CFn_WMCommand                               [internal]
 */
static LRESULT CFn_WMCommand(HWND hDlg, WPARAM wParam, LPARAM lParam, LPCHOOSEFONTW lpcf)
{
    int i;
    long l;
    HDC hdc;
    BOOL cmb_selected_by_edit = FALSE;

    if (!lpcf) return FALSE;

    if(HIWORD(wParam) == CBN_EDITCHANGE)
    {
        int idx;
        WCHAR str_edit[256], str_cmb[256];
        int cmb = LOWORD(wParam);

        GetDlgItemTextW(hDlg, cmb, str_edit, ARRAY_SIZE(str_edit));
        idx = SendDlgItemMessageW(hDlg, cmb, CB_FINDSTRING, -1, (LPARAM)str_edit);
        if(idx != -1)
        {
            SendDlgItemMessageW(hDlg, cmb, CB_GETLBTEXT, idx, (LPARAM)str_cmb);

            /* Select listbox entry only if we have an exact match */
            if(lstrcmpiW(str_edit, str_cmb) == 0)
            {
                 SendDlgItemMessageW(hDlg, cmb, CB_SETCURSEL, idx, 0);
                 SendDlgItemMessageW(hDlg, cmb, CB_SETEDITSEL, 0, -1); /* Remove edit field selection */
                 cmb_selected_by_edit = TRUE;
            }
        }
    }

    TRACE("WM_COMMAND wParam=%08IX lParam=%08IX\n", wParam, lParam);
    switch (LOWORD(wParam))
    {
    case cmb1:
        if (HIWORD(wParam) == CBN_SELCHANGE || cmb_selected_by_edit)
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
        break;
    case chx1:
    case chx2:
    case cmb2:
    case cmb3:
    case cmb5:
        if (HIWORD(wParam) == CBN_SELCHANGE || HIWORD(wParam) == BN_CLICKED || cmb_selected_by_edit)
        {
            WCHAR str[256];
            WINDOWINFO wininfo;
            LPLOGFONTW lpxx=lpcf->lpLogFont;

            TRACE("WM_COMMAND/cmb2,3 =%08IX\n", lParam);

            /* face name */
            i=SendDlgItemMessageW(hDlg,cmb1,CB_GETCURSEL,0,0);
            if (i==CB_ERR)
                GetDlgItemTextW( hDlg, cmb1, str, ARRAY_SIZE(str));
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
            lstrcpynW(lpxx->lfFaceName, str, ARRAY_SIZE(lpxx->lfFaceName));

            /* style */
            i=SendDlgItemMessageW(hDlg, cmb2, CB_GETCURSEL, 0, 0);
            if (i!=CB_ERR)
            {
                l=SendDlgItemMessageW(hDlg, cmb2, CB_GETITEMDATA, i, 0);
                if (0!=(lpxx->lfItalic=HIWORD(l)))
                    lpcf->nFontType |= ITALIC_FONTTYPE;
                if ((lpxx->lfWeight=LOWORD(l)) > FW_MEDIUM)
                    lpcf->nFontType |= BOLD_FONTTYPE;
            }

            /* size */
            get_dialog_font_point_size(hDlg, lpcf);

            /* charset */
            if (lpcf->Flags & CF_NOSCRIPTSEL)
                lpxx->lfCharSet = DEFAULT_CHARSET;
            else
            {
                i=SendDlgItemMessageW(hDlg, cmb5, CB_GETCURSEL, 0, 0);
                if (i!=CB_ERR)
                    lpxx->lfCharSet=SendDlgItemMessageW(hDlg, cmb5, CB_GETITEMDATA, i, 0);
                else
                    lpxx->lfCharSet = DEFAULT_CHARSET;
            }
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
        break;

    case IDOK:
    {
        WCHAR msgW[80];
        INT pointsize;

        pointsize = get_dialog_font_point_size(hDlg, lpcf);
        if (pointsize == -1)
        {
            LoadStringW(COMDLG32_hInstance, IDS_FONT_SIZE_INPUT, msgW, ARRAY_SIZE(msgW));
            MessageBoxW(hDlg, msgW, NULL, MB_OK | MB_ICONINFORMATION);
            return TRUE;
        }

        if (  (!(lpcf->Flags & CF_LIMITSIZE))  ||
              ( (lpcf->Flags & CF_LIMITSIZE) &&
                (lpcf->iPointSize >= 10 * lpcf->nSizeMin) &&
                (lpcf->iPointSize <= 10 * lpcf->nSizeMax)))
            EndDialog(hDlg, TRUE);
        else
        {
            WCHAR format[80];
            DWORD_PTR args[2];
            LoadStringW(COMDLG32_hInstance, IDS_FONT_SIZE, format, ARRAY_SIZE(format));
            args[0] = lpcf->nSizeMin;
            args[1] = lpcf->nSizeMax;
            FormatMessageW(FORMAT_MESSAGE_FROM_STRING|FORMAT_MESSAGE_ARGUMENT_ARRAY,
                           format, 0, 0, msgW, ARRAY_SIZE(msgW), (va_list *)args);
            MessageBoxW(hDlg, msgW, NULL, MB_OK);
        }
        return(TRUE);
    }
    case IDCANCEL:
        EndDialog(hDlg, FALSE);
        return(TRUE);
    }
    return(FALSE);
}

static LRESULT CFn_WMDestroy(HWND hwnd, LPCHOOSEFONTW lpcfw)
{
    LPCHOOSEFONTA lpcfa;
    LPSTR lpszStyle;
    LPLOGFONTA lpLogFonta;
    int len;

    if (!lpcfw) return FALSE;

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
        heap_free(lpcfw->lpszStyle);
    }

    heap_free(lpcfw->lpLogFont);
    heap_free(lpcfw);
    SetPropW(hwnd, strWineFontData, 0);

    return TRUE;
}

static LRESULT CFn_WMPaint(HWND hDlg, WPARAM wParam, LPARAM lParam, const CHOOSEFONTW *lpcf)
{
    WINDOWINFO info;

    if (!lpcf) return FALSE;

    info.cbSize=sizeof(info);
    if( GetWindowInfo( GetDlgItem( hDlg, stc5), &info ) )
    {
        PAINTSTRUCT ps;
        HDC hdc;
        HFONT hOrigFont;
        LOGFONTW lf = *(lpcf->lpLogFont);

        MapWindowPoints( 0, hDlg, (LPPOINT) &info.rcWindow, 2);
        hdc = BeginPaint( hDlg, &ps );

        TRACE("erase %d, rect=%s\n", ps.fErase, wine_dbgstr_rect(&ps.rcPaint));

        /* Paint frame */
        DrawEdge( hdc, &info.rcWindow, EDGE_SUNKEN, BF_RECT|BF_ADJUST );

        /* Draw the sample text itself */
        hOrigFont = SelectObject( hdc, CreateFontIndirectW( &lf ) );
        SetTextColor( hdc, lpcf->rgbColors );
        SetBkMode( hdc, TRANSPARENT );

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
static INT_PTR CALLBACK FormatCharDlgProcA(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    LPCHOOSEFONTW lpcfw;
    LPCHOOSEFONTA lpcfa;
    INT_PTR res = FALSE;
    int len;

    if (uMsg!=WM_INITDIALOG) {
        lpcfw = GetPropW(hDlg, strWineFontData);
        if (lpcfw && CFn_HookCallChk32(lpcfw))
            res=CallWindowProcA((WNDPROC)lpcfw->lpfnHook, hDlg, uMsg, wParam, lParam);
        if (res)
            return res;
    } else {
        lpcfa=(LPCHOOSEFONTA)lParam;
        SetPropW(hDlg, strWineFontData_a, (HANDLE)lParam);

        lpcfw = heap_alloc(sizeof(*lpcfw));
        memcpy(lpcfw, lpcfa, sizeof(CHOOSEFONTA));
        lpcfw->lpLogFont = heap_alloc(sizeof(*lpcfw->lpLogFont));
        memcpy(lpcfw->lpLogFont, lpcfa->lpLogFont, sizeof(LOGFONTA));
        MultiByteToWideChar(CP_ACP, 0, lpcfa->lpLogFont->lfFaceName,
                            LF_FACESIZE, lpcfw->lpLogFont->lfFaceName, LF_FACESIZE);

        if((lpcfa->Flags & CF_USESTYLE) && lpcfa->lpszStyle)  {
            len = MultiByteToWideChar(CP_ACP, 0, lpcfa->lpszStyle, -1, NULL, 0);
            lpcfw->lpszStyle = heap_alloc(len * sizeof(WCHAR));
            MultiByteToWideChar(CP_ACP, 0, lpcfa->lpszStyle, -1, lpcfw->lpszStyle, len);
        }

        if (!CFn_WMInitDialog(hDlg, lParam, lpcfw))
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
        return CFn_WMMeasureItem(hDlg,lParam);
    case WM_DRAWITEM:
        return CFn_WMDrawItem(lParam);
    case WM_COMMAND:
        return CFn_WMCommand(hDlg, wParam, lParam, lpcfw);
    case WM_DESTROY:
        return CFn_WMDestroy(hDlg, lpcfw);
    case WM_CHOOSEFONT_GETLOGFONT:
    {
        LOGFONTA *logfont = (LOGFONTA *)lParam;
        TRACE("WM_CHOOSEFONT_GETLOGFONT lParam=%08IX\n", lParam);
        memcpy( logfont, lpcfw->lpLogFont, FIELD_OFFSET( LOGFONTA, lfFaceName ));
        WideCharToMultiByte( CP_ACP, 0, lpcfw->lpLogFont->lfFaceName, LF_FACESIZE,
                             logfont->lfFaceName, LF_FACESIZE, NULL, NULL );
        break;
    }
    case WM_PAINT:
        return CFn_WMPaint(hDlg, wParam, lParam, lpcfw);
    }
    return res;
}

/***********************************************************************
 *           FormatCharDlgProcW   [internal]
 */
static INT_PTR CALLBACK FormatCharDlgProcW(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    LPCHOOSEFONTW lpcf;
    INT_PTR res = FALSE;

    if (uMsg!=WM_INITDIALOG)
    {
        lpcf= GetPropW(hDlg, strWineFontData);
        if (lpcf && CFn_HookCallChk32(lpcf))
            res=CallWindowProcW((WNDPROC)lpcf->lpfnHook, hDlg, uMsg, wParam, lParam);
        if (res)
            return res;
    }
    else
    {
        lpcf=(LPCHOOSEFONTW)lParam;
        if (!CFn_WMInitDialog(hDlg, lParam, lpcf))
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
        return CFn_WMMeasureItem(hDlg, lParam);
    case WM_DRAWITEM:
        return CFn_WMDrawItem(lParam);
    case WM_COMMAND:
        return CFn_WMCommand(hDlg, wParam, lParam, lpcf);
    case WM_DESTROY:
        return TRUE;
    case WM_CHOOSEFONT_GETLOGFONT:
        TRACE("WM_CHOOSEFONT_GETLOGFONT lParam=%08IX\n", lParam);
        memcpy( (LOGFONTW *)lParam, lpcf->lpLogFont, sizeof(LOGFONTW) );
        break;
    case WM_PAINT:
        return CFn_WMPaint(hDlg, wParam, lParam, lpcf);
    }
    return res;
}
