/*  LOOKDLG.C
**
**  Copyright (C) Microsoft, 1993, All Rights Reserved.
**
**
**  History:
**
*/
#include "pch.hxx" // PCH
#pragma hdrstop

#include "AccWiz.h"

#include "desk.h"
#include "deskid.h"
#include "look.h"
#include <commdlg.h>
#include <commctrl.h>
#include <regstr.h>
#include "help.h"



#define RGB_PALETTE 0x02000000

void FAR SetMagicColors(HDC, DWORD, WORD);

#define CENTRIES_3D 3

HPALETTE g_hpal3D = NULL;               // only exist if palette device
HPALETTE g_hpalVGA = NULL;              // only exist if palette device
BOOL g_bPalette = FALSE;                // is this a palette device?
int cyBorder;
int cxBorder;
int cxEdge;
int cyEdge;

HWND g_hwndTooltip;

LOOK_FONT g_fonts[NUM_FONTS];
HBRUSH g_brushes[NT40_COLOR_MAX];

BOOL g_bInit = TRUE;

BOOL g_fProprtySheetExiting = FALSE;

#define METRIC_CHANGE 0x0001
#define COLOR_CHANGE  0x0002
#define SCHEME_CHANGE 0x8000
UINT g_fChanged;

LOOK_SIZE g_sizes[NUM_SIZES] = {
/* SIZE_FRAME */        {0, 0, 50},
/* SIZE_SCROLL */       {0, 8, 100},
/* SIZE_CAPTION */      {0, 8, 100},
/* SIZE_SMCAPTION */    {0, 4, 100},
/* SIZE_MENU */         {0, 8, 100},
/* SIZE_DXICON */       {0, 0, 150},    // x spacing
/* SIZE_DYICON */       {0, 0, 150},    // y spacing
/* SIZE_ICON */         {0, 16, 72},    // shell icon size
/* SIZE_SMICON */       {0, 8, 36},     // shell small icon size
};

LOOK_SIZE g_elCurSize;

#define COLORFLAG_SOLID 0x0001

UINT g_colorFlags[NT40_COLOR_MAX] = {
/* COLOR_SCROLLBAR           */ 0,
/* COLOR_DESKTOP             */ 0,
/* COLOR_ACTIVECAPTION       */ COLORFLAG_SOLID,
/* COLOR_INACTIVECAPTION     */ COLORFLAG_SOLID,
/* COLOR_MENU                */ COLORFLAG_SOLID,
/* COLOR_WINDOW              */ COLORFLAG_SOLID,
/* COLOR_WINDOWFRAME         */ COLORFLAG_SOLID,
/* COLOR_MENUTEXT            */ COLORFLAG_SOLID,
/* COLOR_WINDOWTEXT          */ COLORFLAG_SOLID,
/* COLOR_CAPTIONTEXT         */ COLORFLAG_SOLID,
/* COLOR_ACTIVEBORDER        */ 0,
/* COLOR_INACTIVEBORDER      */ 0,
/* COLOR_APPWORKSPACE        */ 0,
/* COLOR_HIGHLIGHT           */ COLORFLAG_SOLID,
/* COLOR_HIGHLIGHTTEXT       */ COLORFLAG_SOLID,
/* COLOR_3DFACE              */ COLORFLAG_SOLID,
/* COLOR_3DSHADOW            */ COLORFLAG_SOLID,
/* COLOR_GRAYTEXT            */ COLORFLAG_SOLID,
/* COLOR_BTNTEXT             */ COLORFLAG_SOLID,
/* COLOR_INACTIVECAPTIONTEXT */ COLORFLAG_SOLID,
/* COLOR_3DHILIGHT           */ COLORFLAG_SOLID,
/* COLOR_3DDKSHADOW          */ COLORFLAG_SOLID,
/* COLOR_3DLIGHT             */ COLORFLAG_SOLID,
/* COLOR_INFOTEXT            */ COLORFLAG_SOLID,
/* COLOR_INFOBK              */ 0,
/* COLOR_3DALTFACE           */ COLORFLAG_SOLID,
/* COLOR_HOTLIGHT            */ COLORFLAG_SOLID,
/* COLOR_GRADIENTACTIVECAPTION */ COLORFLAG_SOLID,
/* COLOR_GRADIENTINACTIVECAPTION */ COLORFLAG_SOLID
};

// strings for color names.
PTSTR s_pszColorNames[NT40_COLOR_MAX] = {
/* COLOR_SCROLLBAR           */ TEXT("Scrollbar"),
/* COLOR_DESKTOP             */ TEXT("Background"),
/* COLOR_ACTIVECAPTION       */ TEXT("ActiveTitle"),
/* COLOR_INACTIVECAPTION     */ TEXT("InactiveTitle"),
/* COLOR_MENU                */ TEXT("Menu"),
/* COLOR_WINDOW              */ TEXT("Window"),
/* COLOR_WINDOWFRAME         */ TEXT("WindowFrame"),
/* COLOR_MENUTEXT            */ TEXT("MenuText"),
/* COLOR_WINDOWTEXT          */ TEXT("WindowText"),
/* COLOR_CAPTIONTEXT         */ TEXT("TitleText"),
/* COLOR_ACTIVEBORDER        */ TEXT("ActiveBorder"),
/* COLOR_INACTIVEBORDER      */ TEXT("InactiveBorder"),
/* COLOR_APPWORKSPACE        */ TEXT("AppWorkspace"),
/* COLOR_HIGHLIGHT           */ TEXT("Hilight"),
/* COLOR_HIGHLIGHTTEXT       */ TEXT("HilightText"),
/* COLOR_3DFACE              */ TEXT("ButtonFace"),
/* COLOR_3DSHADOW            */ TEXT("ButtonShadow"),
/* COLOR_GRAYTEXT            */ TEXT("GrayText"),
/* COLOR_BTNTEXT             */ TEXT("ButtonText"),
/* COLOR_INACTIVECAPTIONTEXT */ TEXT("InactiveTitleText"),
/* COLOR_3DHILIGHT           */ TEXT("ButtonHilight"),
/* COLOR_3DDKSHADOW          */ TEXT("ButtonDkShadow"),
/* COLOR_3DLIGHT             */ TEXT("ButtonLight"),
/* COLOR_INFOTEXT            */ TEXT("InfoText"),
/* COLOR_INFOBK              */ TEXT("InfoWindow"),
/* COLOR_3DALTFACE           */ TEXT("ButtonAlternateFace"),
/* COLOR_HOTLIGHT            */ TEXT("HotTracking"),
/* COLOR_GRADIENTACTIVECAPTION */ TEXT("GradientActiveTitle"),
/* COLOR_GRADIENTINACTIVECAPTION */ TEXT("GradientInactiveTitle")
};
TCHAR g_szColors[] = TEXT("colors");           // colors section name

// Location of the Colors subkey in Registry; Defined in RegStr.h
TCHAR szRegStr_Colors[] = REGSTR_PATH_COLORS;

//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//this order has to match the enum order in look.h
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
LOOK_ELEMENT g_elements[] = {
/* ELEMENT_APPSPACE        */   {COLOR_APPWORKSPACE,    SIZE_NONE,      FALSE,   COLOR_NONE,             FONT_NONE,     ELNAME_APPSPACE, -1,       {-1,-1,-1,-1}, COLOR_NONE},
/* ELEMENT_DESKTOP         */   {COLOR_BACKGROUND,      SIZE_NONE,      FALSE,   COLOR_NONE,             FONT_NONE,     ELNAME_DESKTOP, -1,        {-1,-1,-1,-1}, COLOR_NONE},
/* ELEMENT_INACTIVEBORDER  */   {COLOR_INACTIVEBORDER,  SIZE_FRAME,     FALSE,   COLOR_NONE,             FONT_NONE,     ELNAME_INACTIVEBORDER, -1, {-1,-1,-1,-1}, COLOR_NONE},
/* ELEMENT_ACTIVEBORDER    */   {COLOR_ACTIVEBORDER,    SIZE_FRAME,     FALSE,   COLOR_NONE,             FONT_NONE,     ELNAME_ACTIVEBORDER, -1,   {-1,-1,-1,-1}, COLOR_NONE},
/* ELEMENT_INACTIVECAPTION */   {COLOR_INACTIVECAPTION, SIZE_CAPTION,   TRUE,    COLOR_INACTIVECAPTIONTEXT,FONT_CAPTION,ELNAME_INACTIVECAPTION, -1,{-1,-1,-1,-1}, COLOR_GRADIENTINACTIVECAPTION},
/* ELEMENT_INACTIVESYSBUT1 */   {COLOR_NONE,            SIZE_CAPTION,   FALSE,   COLOR_NONE,             FONT_NONE,     -1, ELEMENT_ACTIVESYSBUT1, {-1,-1,-1,-1}, COLOR_NONE},
/* ELEMENT_INACTIVESYSBUT2 */   {COLOR_NONE,            SIZE_CAPTION,   FALSE,   COLOR_NONE,             FONT_NONE,     -1, ELEMENT_ACTIVESYSBUT1, {-1,-1,-1,-1}, COLOR_NONE},
/* ELEMENT_ACTIVECAPTION   */   {COLOR_ACTIVECAPTION,   SIZE_CAPTION,   TRUE,    COLOR_CAPTIONTEXT,      FONT_CAPTION,  ELNAME_ACTIVECAPTION, -1,  {-1,-1,-1,-1}, COLOR_GRADIENTACTIVECAPTION},
/* ELEMENT_ACTIVESYSBUT1   */   {COLOR_NONE,            SIZE_CAPTION,   FALSE,   COLOR_NONE,             FONT_NONE,     ELNAME_CAPTIONBUTTON, -1,  {-1,-1,-1,-1}, COLOR_NONE},
/* ELEMENT_ACTIVESYSBUT2   */   {COLOR_NONE,            SIZE_CAPTION,   FALSE,   COLOR_NONE,             FONT_NONE,     -1, ELEMENT_ACTIVESYSBUT1, {-1,-1,-1,-1}, COLOR_NONE},
/* ELEMENT_MENUNORMAL      */   {COLOR_MENU,            SIZE_MENU,      TRUE,    COLOR_MENUTEXT,         FONT_MENU,     ELNAME_MENU, -1,           {-1,-1,-1,-1}, COLOR_NONE},
/* ELEMENT_MENUSELECTED    */   {COLOR_HIGHLIGHT,       SIZE_MENU,      TRUE,    COLOR_HIGHLIGHTTEXT,    FONT_MENU,     ELNAME_MENUSELECTED, -1,   {-1,-1,-1,-1}, COLOR_NONE},
/* ELEMENT_MENUDISABLED    */   {COLOR_MENU,            SIZE_MENU,      TRUE,    COLOR_NONE,             FONT_MENU,     -1, ELEMENT_MENUNORMAL,    {-1,-1,-1,-1}, COLOR_NONE},
/* ELEMENT_WINDOW          */   {COLOR_WINDOW,          SIZE_NONE,      FALSE,   COLOR_WINDOWTEXT,       FONT_NONE,     ELNAME_WINDOW, -1,         {-1,-1,-1,-1}, COLOR_NONE},
/* ELEMENT_MSGBOX          */   {COLOR_NONE,            SIZE_NONE,      TRUE,    COLOR_WINDOWTEXT,       FONT_MSGBOX,   ELNAME_MSGBOX, -1,         {-1,-1,-1,-1}, COLOR_NONE},
/* ELEMENT_MSGBOXCAPTION   */   {COLOR_ACTIVECAPTION,   SIZE_CAPTION,   TRUE,    COLOR_CAPTIONTEXT,      FONT_CAPTION,  -1, ELEMENT_ACTIVECAPTION, {-1,-1,-1,-1}, COLOR_NONE},
/* ELEMENT_MSGBOXSYSBUT    */   {COLOR_3DFACE,          SIZE_CAPTION,   TRUE,    COLOR_BTNTEXT,          FONT_CAPTION,  -1, ELEMENT_ACTIVESYSBUT1, {-1,-1,-1,-1}, COLOR_NONE},
// do not even try to set a scrollbar color the system will ignore you
/* ELEMENT_SCROLLBAR       */   {COLOR_NONE,            SIZE_SCROLL,    FALSE,   COLOR_NONE,             FONT_NONE,     ELNAME_SCROLLBAR, -1,      {-1,-1,-1,-1}, COLOR_NONE},
/* ELEMENT_SCROLLUP        */   {COLOR_NONE,            SIZE_SCROLL,    FALSE,   COLOR_NONE,             FONT_NONE,     -1, ELEMENT_SCROLLBAR,     {-1,-1,-1,-1}, COLOR_NONE},
/* ELEMENT_SCROLLDOWN      */   {COLOR_NONE,            SIZE_SCROLL,    FALSE,   COLOR_NONE,             FONT_NONE,     -1, ELEMENT_SCROLLBAR,     {-1,-1,-1,-1}, COLOR_NONE},
/* ELEMENT_BUTTON          */   {COLOR_3DFACE,          SIZE_NONE,      FALSE,   COLOR_BTNTEXT,          FONT_NONE,     ELNAME_BUTTON, -1,         {-1,-1,-1,-1}, COLOR_NONE},
/* ELEMENT_SMCAPTION       */   {COLOR_NONE,            SIZE_SMCAPTION, TRUE,    COLOR_NONE,             FONT_SMCAPTION,ELNAME_SMALLCAPTION, -1,   {-1,-1,-1,-1}, COLOR_NONE},
/* ELEMENT_ICON            */   {COLOR_NONE,            SIZE_ICON,      FALSE,   COLOR_NONE,             FONT_ICONTITLE,ELNAME_ICON, -1,           {-1,-1,-1,-1}, COLOR_NONE},
/* ELEMENT_ICONHORZSPACING */   {COLOR_NONE,            SIZE_DXICON,    FALSE,   COLOR_NONE,             FONT_NONE,     ELNAME_DXICON, -1,         {-1,-1,-1,-1}, COLOR_NONE},
/* ELEMENT_ICONVERTSPACING */   {COLOR_NONE,            SIZE_DYICON,    FALSE,   COLOR_NONE,             FONT_NONE,     ELNAME_DYICON, -1,         {-1,-1,-1,-1}, COLOR_NONE},
/* ELEMENT_INFO            */   {COLOR_INFOBK,          SIZE_NONE,      TRUE,    COLOR_INFOTEXT,         FONT_STATUS,   ELNAME_INFO, -1,           {-1,-1,-1,-1}, COLOR_NONE},
};
#if 0
// go fix look.h if you decide to add this back in
/* ELEMENT_SMICON          */   {COLOR_NONE,            SIZE_SMICON,    FALSE,   COLOR_NONE,             FONT_NONE,     ELNAME_SMICON, -1,         {-1,-1,-1,-1}},
#endif

// used by ChooseColor dialog
COLORREF g_CustomColors[16];

// structure used to store a scheme in the registry
#ifdef UNICODE
#   define SCHEME_VERSION 2        // Ver 2 == Unicode
#else
#   define SCHEME_VERSION 1        // Ver 1 == Win95 ANSI
#endif

/*
 * Note -- this must match the High Contrast accessibility code
 *  in windows\gina\winlogon.
 */

typedef struct {
    SHORT version;
    WORD  wDummy;               // for alignment
    NONCLIENTMETRICS ncm;
    LOGFONT lfIconTitle;
    COLORREF rgb[NT40_COLOR_MAX];
} SCHEMEDATA;


HWND g_hDlg = NULL;             // nonzero if page is up
int g_iCurElement = -2;         // start off as not even "not set"
int g_LogDPI = 96;              // logical resolution of display
#define ELCUR           (g_elements[g_iCurElement])
#define ELCURFONT       (g_fonts[ELCUR.iFont])
// this one kept separately for range purposes
#define ELCURSIZE       g_elCurSize
int g_iPrevSize = SIZE_NONE;

#define MAXSCHEMENAME 100
TCHAR g_szCurScheme[MAXSCHEMENAME];      // current scheme name
TCHAR g_szLastScheme[MAXSCHEMENAME];     // last scheme they had

HBRUSH g_hbrMainColor = NULL;
HBRUSH g_hbrTextColor = NULL;
HBRUSH g_hbrGradColor = NULL;

const TCHAR c_szRegPathUserMetrics[] = TEXT("Control Panel\\Desktop\\WindowMetrics");
const TCHAR c_szRegValIconSize[] = TEXT("Shell Icon Size");
const TCHAR c_szRegValSmallIconSize[] = TEXT("Shell Small Icon Size");

void NEAR PASCAL Look_Repaint(HWND hDlg, BOOL bRecalc);
BOOL NEAR PASCAL Look_ChangeColor(HWND hDlg, int iColor, COLORREF rgb);
BOOL CALLBACK SaveSchemeDlgProc(HWND, UINT, WPARAM, LPARAM);
void NEAR PASCAL Look_UpdateSizeBasedOnFont(HWND hDlg, BOOL fComputeIdeal);
void NEAR PASCAL Look_SetCurSizeAndRange(HWND hDlg);
void NEAR PASCAL Look_SyncSize(HWND hDlg);
void NEAR PASCAL Look_DoSizeStuff(HWND hDlg, BOOL fCanComputeIdeal);

//
// bugbug, needs to be in inc16\windows.h
//
//int WINAPI EnumFontFamiliesEx(HDC, LPLOGFONT, FONTENUMPROC, LPARAM, DWORD);


COLORREF GetNearestPaletteColor(HPALETTE hpal, COLORREF rgb)
{
    PALETTEENTRY pe;
    GetPaletteEntries(hpal, GetNearestPaletteIndex(hpal, rgb & 0x00FFFFFF), 1, &pe);
    return RGB(pe.peRed, pe.peGreen, pe.peBlue);
}

BOOL IsPaletteColor(HPALETTE hpal, COLORREF rgb)
{
    return GetNearestPaletteColor(hpal, rgb) == (rgb & 0xFFFFFF);
}

COLORREF NearestColor(int iColor, COLORREF rgb)
{
    rgb &= 0x00FFFFFF;

    //
    // if we are on a palette device, we need to do special stuff...
    //
    if (g_bPalette)
    {
        if (g_colorFlags[iColor] & COLORFLAG_SOLID)
        {
            if (IsPaletteColor(g_hpal3D, rgb))
                rgb |= RGB_PALETTE;
            else
                rgb = GetNearestPaletteColor(g_hpalVGA, rgb);
        }
        else
        {
            if (IsPaletteColor(g_hpal3D, rgb))
                rgb |= RGB_PALETTE;

            else if (IsPaletteColor((HPALETTE)GetStockObject(DEFAULT_PALETTE), rgb))
                rgb ^= 0x000001;    // force a dither
        }
    }
    else
    {
        // map color to nearest color if we need to for this UI element.
        if (g_colorFlags[iColor] & COLORFLAG_SOLID)
        {
            HDC hdc = GetDC(NULL);
            rgb = GetNearestColor(hdc, rgb);
            ReleaseDC(NULL, hdc);
        }
    }

    return rgb;
}



void NEAR PASCAL Set3DPaletteColor(COLORREF rgb, int iColor)
{
    int iPalette;
    PALETTEENTRY pe;

    if (!g_hpal3D)
        return;

    switch (iColor)
    {
        case COLOR_3DFACE:
            iPalette = 16;
            break;
        case COLOR_3DSHADOW:
            iPalette = 17;
            break;
        case COLOR_3DHILIGHT:
            iPalette = 18;
            break;
        default:
            return;
    }

    pe.peRed    = GetRValue(rgb);
    pe.peGreen  = GetGValue(rgb);
    pe.peBlue   = GetBValue(rgb);
    pe.peFlags  = 0;
    SetPaletteEntries(g_hpal3D, iPalette, 1, (LPPALETTEENTRY)&pe);
}


void NEAR PASCAL Look_RebuildSysStuff(BOOL fInit)
{
    int i;
    PALETTEENTRY pal[4];
    HPALETTE hpal = (HPALETTE)GetStockObject(DEFAULT_PALETTE);

    SelectObject(g_hdcMem, GetStockObject(BLACK_BRUSH));
    SelectObject(g_hdcMem, GetStockObject(SYSTEM_FONT));

    for (i = 0; i < NUM_FONTS; i++)
    {
        if (g_fonts[i].hfont)
            DeleteObject(g_fonts[i].hfont);
        g_fonts[i].hfont = CreateFontIndirect(&g_fonts[i].lf);
    }

    if (fInit)
    {
        // get current magic colors
        GetPaletteEntries(hpal, 8,  4, pal);
        SetPaletteEntries(g_hpal3D, 16,  4, pal);

        // set up magic colors in the 3d palette
        if (!IsPaletteColor(hpal, g_Options.m_schemePreview.m_rgb[COLOR_3DFACE]))
        {
            Set3DPaletteColor(g_Options.m_schemePreview.m_rgb[COLOR_3DFACE], COLOR_3DFACE);
            Set3DPaletteColor(g_Options.m_schemePreview.m_rgb[COLOR_3DSHADOW], COLOR_3DSHADOW);
            Set3DPaletteColor(g_Options.m_schemePreview.m_rgb[COLOR_3DHILIGHT], COLOR_3DHILIGHT);
        }
    }

    for (i = 0; i < NT40_COLOR_MAX; i++)
    {
        if (g_brushes[i])
            DeleteObject(g_brushes[i]);

        g_brushes[i] = CreateSolidBrush(NearestColor(i, g_Options.m_schemePreview.m_rgb[i]));
    }
}

#ifndef LF32toLF

void NEAR LF32toLF(LPLOGFONT_32 lplf32, LPLOGFONT lplf)
{
    lplf->lfHeight       = (int) lplf32->lfHeight;
    lplf->lfWidth        = (int) lplf32->lfWidth;
    lplf->lfEscapement   = (int) lplf32->lfEscapement;
    lplf->lfOrientation  = (int) lplf32->lfOrientation;
    lplf->lfWeight       = (int) lplf32->lfWeight;
    *((LPCOMMONFONT) &lplf->lfItalic) = lplf32->lfCommon;
}
#endif


void NEAR SetMyNonClientMetrics(LPNONCLIENTMETRICS lpncm)
{
    g_sizes[SIZE_FRAME].CurSize = (int)lpncm->iBorderWidth;
    g_sizes[SIZE_SCROLL].CurSize = (int)lpncm->iScrollWidth;
    g_sizes[SIZE_CAPTION].CurSize = (int)lpncm->iCaptionHeight;
    g_sizes[SIZE_SMCAPTION].CurSize = (int)lpncm->iSmCaptionHeight;
    g_sizes[SIZE_MENU].CurSize = (int)lpncm->iMenuHeight;

    LF32toLF(&(lpncm->lfCaptionFont), &(g_fonts[FONT_CAPTION].lf));
    LF32toLF(&(lpncm->lfSmCaptionFont), &(g_fonts[FONT_SMCAPTION].lf));
    LF32toLF(&(lpncm->lfMenuFont), &(g_fonts[FONT_MENU].lf));
    LF32toLF(&(lpncm->lfStatusFont), &(g_fonts[FONT_STATUS].lf));
    LF32toLF(&(lpncm->lfMessageFont), &(g_fonts[FONT_MSGBOX].lf));
}

/*
** Fill in a NONCLIENTMETRICS structure with latest preview stuff
*/
void NEAR GetMyNonClientMetrics(LPNONCLIENTMETRICS lpncm)
{
    lpncm->iBorderWidth = (LONG)g_sizes[SIZE_FRAME].CurSize;
    lpncm->iScrollWidth = lpncm->iScrollHeight = (LONG)g_sizes[SIZE_SCROLL].CurSize;
    lpncm->iCaptionWidth = lpncm->iCaptionHeight = (LONG)g_sizes[SIZE_CAPTION].CurSize;
    lpncm->iSmCaptionWidth = lpncm->iSmCaptionHeight = (LONG)g_sizes[SIZE_SMCAPTION].CurSize;
    lpncm->iMenuWidth = lpncm->iMenuHeight = (LONG)g_sizes[SIZE_MENU].CurSize;
    LFtoLF32(&(g_fonts[FONT_CAPTION].lf), &(lpncm->lfCaptionFont));
    LFtoLF32(&(g_fonts[FONT_SMCAPTION].lf), &(lpncm->lfSmCaptionFont));
    LFtoLF32(&(g_fonts[FONT_MENU].lf), &(lpncm->lfMenuFont));
    LFtoLF32(&(g_fonts[FONT_STATUS].lf), &(lpncm->lfStatusFont));
    LFtoLF32(&(g_fonts[FONT_MSGBOX].lf), &(lpncm->lfMessageFont));
}




/*
** clean up any mess made in maintaining system information
** also, write out any global changes in our setup.
*/
void NEAR PASCAL Look_DestroySysStuff(void)
{
    int i;
    HKEY hkAppear;

    SelectObject(g_hdcMem, GetStockObject(BLACK_BRUSH));
    SelectObject(g_hdcMem, GetStockObject(SYSTEM_FONT));

    for (i = 0; i < NUM_FONTS; i++)
    {
        if (g_fonts[i].hfont)
            DeleteObject(g_fonts[i].hfont);
    }
    for (i = 0; i < NT40_COLOR_MAX; i++)
    {
        if (g_brushes[i])
            DeleteObject(g_brushes[i]);
    }

    if (g_hpal3D)
        DeleteObject(g_hpal3D);

    if (g_hpalVGA)
        DeleteObject(g_hpalVGA);

    // save out possible changes to custom color table
    if (RegOpenKey(HKEY_CURRENT_USER, REGSTR_PATH_APPEARANCE, &hkAppear) == ERROR_SUCCESS)
    {
        RegSetValueEx(hkAppear, REGSTR_VAL_CUSTOMCOLORS, 0L, REG_BINARY,
                                (LPBYTE)g_CustomColors, sizeof(g_CustomColors));
    }
}

#if 0
void NEAR PASCAL Look_SetSysStuff(UINT fChanged)
{
    // COMPATIBILITY:
    //   Do metrics first since the color stuff might cause USER to generate a
    // WM_SYSCOLORCHANGE message and we don't want to send two of them...

    DPRINT("desk.cpl: Look_SetSysStuff");
    
    if( fChanged & METRIC_CHANGE )
    {
        NONCLIENTMETRICS ncm;
        HKEY hkey;

        DPRINT("desk.cpl: Metrics Changed");

        GetMyNonClientMetrics(&ncm);

        ncm.cbSize = sizeof(ncm);
        DPRINT("desk.cpl: Calling SPI_SETNONCLIENTMETRICS");
        SystemParametersInfo(SPI_SETNONCLIENTMETRICS, sizeof(ncm),
            (void far *)(LPNONCLIENTMETRICS)&ncm,
            SPIF_UPDATEINIFILE );

        DPRINT("desk.cpl: Calling SPI_SETICONTITLELOGFONT");
        SystemParametersInfo(SPI_SETICONTITLELOGFONT, sizeof(LOGFONT),
            (void far *)(LPLOGFONT)&(g_fonts[FONT_ICONTITLE].lf),
            SPIF_UPDATEINIFILE );

        DPRINT("desk.cpl: Calling SPI_ICONHORIZONTALSPACING");
        SystemParametersInfo(SPI_ICONHORIZONTALSPACING,
            g_sizes[ SIZE_DXICON ].CurSize + g_sizes[ SIZE_ICON ].CurSize,
            NULL, SPIF_UPDATEINIFILE );

        DPRINT("desk.cpl: Calling SPI_ICONVERTICALSPACING");
        SystemParametersInfo(SPI_ICONVERTICALSPACING,
            g_sizes[ SIZE_DYICON ].CurSize + g_sizes[ SIZE_ICON ].CurSize,
            NULL, SPIF_UPDATEINIFILE );

        DPRINT("desk.cpl: Done calling SPI's");

        if( RegCreateKey( HKEY_CURRENT_USER, c_szRegPathUserMetrics, &hkey )
            == ERROR_SUCCESS )
        {
            TCHAR val[ 8 ];

            wsprintf( val, TEXT("%d"), g_sizes[ SIZE_ICON ].CurSize );
            RegSetValueEx( hkey, c_szRegValIconSize, 0, REG_SZ,
                (LPBYTE)&val, SIZEOF(TCHAR) * (lstrlen( val ) + 1) );

#ifdef THE_SHELL_CAN_HANDLE_CUSTOM_SMALL_ICON_SIZES_YET
            wsprintf( val, TEXT("%d"), g_sizes[ SIZE_SMICON ].CurSize );
            RegSetValueEx( hkey, c_szRegValSmallIconSize, 0, REG_SZ,
                (LPBYTE)&val, SIZEOF(TCHAR) * (lstrlen( val ) + 1) );
#else
            RegDeleteValue( hkey, c_szRegValSmallIconSize );
#endif

            RegCloseKey( hkey );
        }

        // WM_SETTINGCHANGE is sent at the end of the function
    }

    if( fChanged & COLOR_CHANGE )
    {
        int i;
        int iColors[NT40_COLOR_MAX];
        COLORREF rgbColors[NT40_COLOR_MAX];
        TCHAR szRGB[32];
        COLORREF rgb;
        HKEY     hk;
        HDC      hdc;
        BOOL     bGradient = FALSE;

        if ((g_Options.m_schemePreview.m_rgb[COLOR_ACTIVECAPTION] != g_Options.m_schemePreview.m_rgb[COLOR_GRADIENTACTIVECAPTION]) ||
                (g_Options.m_schemePreview.m_rgb[COLOR_INACTIVECAPTION] != g_Options.m_schemePreview.m_rgb[COLOR_GRADIENTINACTIVECAPTION])) {
            bGradient = TRUE;
        }

        DPRINT("desk.cpl: Calling SPI_SETGRADIENTCAPTIONS");
        SystemParametersInfo(SPI_SETGRADIENTCAPTIONS, 0, (PVOID)bGradient, SPIF_UPDATEINIFILE);
        
        //
        // restore magic colors back to Win31 defaults.
        //
        hdc = GetDC(NULL);
        SetMagicColors(hdc, 0x00c0dcc0, 8);         // money green
        SetMagicColors(hdc, 0x00f0caa6, 9);         // IBM blue
        SetMagicColors(hdc, 0x00f0fbff, 246);       // off white
        ReleaseDC(NULL, hdc);

        // -------------------------------------------------
        // This call causes user to send a WM_SYSCOLORCHANGE
        // -------------------------------------------------
        for (i=0; i<NT40_COLOR_MAX; i++)
        {
            iColors[i] = i;
            rgbColors[i] = g_Options.m_schemePreview.m_rgb[i] & 0x00FFFFFF;
        }

        SetSysColors(NT40_COLOR_MAX, iColors, rgbColors);

        if(RegCreateKey(HKEY_CURRENT_USER, szRegStr_Colors, &hk) == ERROR_SUCCESS)
        {
            // write out the color information to win.ini
            for (i = 0; i < NT40_COLOR_MAX; i++)
            {
                rgb = g_Options.m_schemePreview.m_rgb[i];
                wsprintf(szRGB, TEXT("%d %d %d"), GetRValue(rgb), GetGValue(rgb), GetBValue(rgb));

                // For the time being we will update the INI file also.
                WriteProfileString(g_szColors, s_pszColorNames[i], szRGB);
                // Update the registry (Be sure to include the terminating zero in the byte count!)
                RegSetValueEx(hk, s_pszColorNames[i], 0L, REG_SZ, (LPBYTE)szRGB, SIZEOF(TCHAR) * (lstrlen(szRGB)+1));
#ifdef DEBUG
                {
                TCHAR Buff[100];
                wsprintf(Buff, TEXT("CPL:Write Color: %s=%s\n\r"),s_pszColorNames[i], szRGB);
                OutputDebugString(Buff);
                }
#endif  // DEBUG
            }
            RegCloseKey(hk);
        }
    }
    else if( fChanged & METRIC_CHANGE )
    {
        // COMPATIBILITY HACK:
        // no colors were changed, but metrics were
        // EXCEL 5.0 BOZOS tied metrics changes to WM_SYSCOLORCHANGE
        // and ignore the WM_WININICHANGE (now called WM_SETTINGCHANGE)

        // send a bogus WM_SYSCOLORCHANGE
        DPRINT("desk.cpl: Sending WM_SYSCOLORCHANGE");
        SendNotifyMessage(HWND_BROADCAST, WM_SYSCOLORCHANGE, 0, 0);
        DPRINT("desk.cpl: Back from sending WM_SYSCOLORCHANGE");
    }

    // if metrics changed at all send a WM_SETTINGCHANGE
    if( fChanged & METRIC_CHANGE ) {
        DPRINT("desk.cpl: Sending WM_SETTINGCHANGE");
        SendNotifyMessage(HWND_BROADCAST, WM_SETTINGCHANGE, SPI_SETNONCLIENTMETRICS, (LPARAM)TEXT("WindowMetrics"));
        DPRINT("desk.cpl: Back sending WM_SETTINGCHANGE");
    }
}

#endif

//------------------------ mini font picker controls --------------------------
/*
** initialize the constant dialog components
**
** initialize the list of element names.  this stays constant with the
** possible exception that some items might be added/removed depending
** on some special case conditions.
*/

void NEAR PASCAL Look_DestroyDialog(HWND hDlg)
{
    HFONT hfont, hfontOther;

    hfontOther = (HFONT)SendDlgItemMessage(hDlg, IDC_MAINSIZE, WM_GETFONT, 0, 0L);
    hfont = (HFONT)SendDlgItemMessage(hDlg, IDC_FONTBOLD, WM_GETFONT, 0, 0L);
    if (hfont && (hfont != hfontOther))
        DeleteObject(hfont);
    hfont = (HFONT)SendDlgItemMessage(hDlg, IDC_FONTITAL, WM_GETFONT, 0, 0L);
    if (hfont && (hfont != hfontOther))
        DeleteObject(hfont);

    DestroyWindow(g_hwndTooltip);
}


#define LSE_SETCUR 0x0001
#define LSE_ALWAYS 0x0002

#if 0
void FAR PASCAL Look_SelectElement(HWND hDlg, int iElement, DWORD dwFlags)
{
    BOOL bEnable;
    int i;

    if ((iElement == g_iCurElement) && !(dwFlags & LSE_ALWAYS))
        return;

    g_iCurElement = iElement;

    // if needed, find this element in the combobox and select it
    if (dwFlags & LSE_SETCUR)
    {
        i = (int)SendDlgItemMessage(hDlg, IDC_ELEMENTS, CB_GETCOUNT,0,0L);
        for (i--; i >=0 ; i--)
        {
            // if this is the one that references our element, stop
            if (iElement == (int)LOWORD(SendDlgItemMessage(hDlg, IDC_ELEMENTS, CB_GETITEMDATA, (WPARAM)i, 0L)))
                break;
        }
        SendDlgItemMessage(hDlg, IDC_ELEMENTS, CB_SETCURSEL, (WPARAM)i,0L);
    }

    bEnable = (ELCUR.iMainColor != COLOR_NONE);
    if (bEnable)
        Color_SetColor(hDlg, IDC_MAINCOLOR, g_brushes[ELCUR.iMainColor]);
    EnableWindow(GetDlgItem(hDlg, IDC_MAINCOLOR), bEnable);
    EnableWindow(GetDlgItem(hDlg, IDC_COLORLABEL), bEnable);

    bEnable = (ELCUR.iGradColor != COLOR_NONE);
    EnableGradientControl(hDlg, bEnable);
    if (bEnable)
        Color_SetColor(hDlg, IDC_GRADCOLOR, g_brushes[ELCUR.iGradColor]);

    bEnable = (ELCUR.iFont != FONT_NONE);
    if (bEnable)
    {
        Font_NewFont(hDlg, ELCUR.iFont);
    }
    Font_EnableControls(hDlg, bEnable);

    // size may be based on font
    Look_DoSizeStuff(hDlg, FALSE);

    bEnable = (ELCUR.iSize != SIZE_NONE);
    EnableWindow(GetDlgItem(hDlg, IDC_MAINSIZE), bEnable);
    EnableWindow(GetDlgItem(hDlg, IDC_SIZEARROWS), bEnable);
    EnableWindow(GetDlgItem(hDlg, IDC_SIZELABEL), bEnable);

    bEnable = (ELCUR.iTextColor != COLOR_NONE);
    if (bEnable)
        Color_SetColor(hDlg, IDC_TEXTCOLOR, g_brushes[ELCUR.iTextColor]);
    EnableWindow(GetDlgItem(hDlg, IDC_TEXTCOLOR), bEnable);
    EnableWindow(GetDlgItem(hDlg, IDC_FNCOLORLABEL), bEnable);
}
#endif

const static DWORD FAR aAppearanceHelpIds[] = {
        IDC_SCHEMES,        IDH_APPEAR_SCHEME,
        IDC_SAVESCHEME,     IDH_APPEAR_SAVEAS,
        IDC_DELSCHEME,      IDH_APPEAR_DELETE,
        IDC_ELEMENTS,       IDH_APPEAR_ITEM,
        IDC_MAINCOLOR,      IDH_APPEAR_BACKGRNDCOLOR,
        IDC_SIZELABEL,      IDH_APPEAR_ITEMSIZE,
        IDC_MAINSIZE,       IDH_APPEAR_ITEMSIZE,
        IDC_SIZEARROWS,     IDH_APPEAR_ITEMSIZE,
        IDC_FONTLABEL,      IDH_APPEAR_FONT,
        IDC_FONTNAME,       IDH_APPEAR_FONT,
        IDC_FONTSIZE,       IDH_APPEAR_FONTSIZE,
        IDC_FONTBOLD,       IDH_APPEAR_FONTBOLD,
        IDC_FONTITAL,       IDH_APPEAR_FONTITALIC,
        IDC_LOOKPREV,       IDH_APPEAR_GRAPHIC,
        IDC_FONTSIZELABEL,  IDH_APPEAR_FONTSIZE,
        IDC_COLORLABEL,     IDH_APPEAR_BACKGRNDCOLOR,
        IDC_TEXTCOLOR,      IDH_APPEAR_FONTCOLOR,
        IDC_FNCOLORLABEL,   IDH_APPEAR_FONTCOLOR,

        0, 0
};


LONG WINAPI MyStrToLong(LPCTSTR sz)
{
    long l=0;

    while (*sz >= TEXT('0') && *sz <= TEXT('9'))
        l = l*10 + (*sz++ - TEXT('0'));

    return l;
}

