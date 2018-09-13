//+------------------------------------------------------------------------
//
//  File:       color.cxx
//
//  Contents:   FormsTranslateColor
//
//-------------------------------------------------------------------------

#include "headers.hxx"

MtDefine(CColorInfo, Utilities, "CColorInfo")

COLORREF g_acrSysColor[25];
BOOL     g_fSysColorInit = FALSE;
COLORREF g_crPaletteRelative = 0;
HPALETTE g_hpalHalftone = NULL;
HDC      g_hdcMem1 = NULL;
HDC      g_hdcMem2 = NULL;
HDC      g_hdcMem3 = NULL;
BOOL     g_fNoDisplayChange = TRUE;

#if DBG==1
int g_cGetMemDc = 0;
DeclareTag(tagDCTrace, "Color", "Trace DCs")
#endif

#ifndef _MAC 		
// Inverse color table
BYTE * g_pInvCMAP = NULL;
#endif

RGBQUAD g_rgbHalftone[256];

#if !defined(WIN16) && !defined(_MAC)
LOGPAL256 g_lpHalftone;
#else
//Win16 only
LOGPAL256 g_lpHalftone =
{
    0x0300, 256,
    {
        { 0x00, 0x00, 0x00, 0x00 },
        { 0x80, 0x00, 0x00, 0x00 },
        { 0x00, 0x80, 0x00, 0x00 },
        { 0x80, 0x80, 0x00, 0x00 },
        { 0x00, 0x00, 0x80, 0x00 },
        { 0x80, 0x00, 0x80, 0x00 },
        { 0x00, 0x80, 0x80, 0x00 },
        { 0xC0, 0xC0, 0xC0, 0x00 },
        { 0xC0, 0xDC, 0xC0, PC_NOCOLLAPSE },
        { 0xA6, 0xCA, 0xF0, PC_NOCOLLAPSE },
        { 0xCC, 0x33, 0x00, PC_NOCOLLAPSE },
        { 0xFF, 0x33, 0x00, PC_NOCOLLAPSE },
        { 0x00, 0x66, 0x00, PC_NOCOLLAPSE },
        { 0x33, 0x66, 0x00, PC_NOCOLLAPSE },
        { 0x66, 0x66, 0x00, PC_NOCOLLAPSE },
        { 0x99, 0x66, 0x00, PC_NOCOLLAPSE },
        { 0xCC, 0x66, 0x00, PC_NOCOLLAPSE },
        { 0xFF, 0x66, 0x00, PC_NOCOLLAPSE },
        { 0x00, 0x99, 0x00, PC_NOCOLLAPSE },
        { 0x33, 0x99, 0x00, PC_NOCOLLAPSE },
        { 0x66, 0x99, 0x00, PC_NOCOLLAPSE },
        { 0x99, 0x99, 0x00, PC_NOCOLLAPSE },
        { 0xCC, 0x99, 0x00, PC_NOCOLLAPSE },
        { 0xFF, 0x99, 0x00, PC_NOCOLLAPSE },
        { 0x00, 0xCC, 0x00, PC_NOCOLLAPSE },
        { 0x33, 0xCC, 0x00, PC_NOCOLLAPSE },
        { 0x66, 0xCC, 0x00, PC_NOCOLLAPSE },
        { 0x99, 0xCC, 0x00, PC_NOCOLLAPSE },
        { 0xCC, 0xCC, 0x00, PC_NOCOLLAPSE },
        { 0xFF, 0xCC, 0x00, PC_NOCOLLAPSE },
        { 0xEF, 0xD6, 0xC6, PC_NOCOLLAPSE },
        { 0xAD, 0xA9, 0x90, PC_NOCOLLAPSE },
        { 0x66, 0xFF, 0x00, PC_NOCOLLAPSE },
        { 0x99, 0xFF, 0x00, PC_NOCOLLAPSE },
        { 0xCC, 0xFF, 0x00, PC_NOCOLLAPSE },
        { 0x66, 0x33, 0x00, PC_NOCOLLAPSE },
        { 0x00, 0x00, 0x33, PC_NOCOLLAPSE },
        { 0x33, 0x00, 0x33, PC_NOCOLLAPSE },
        { 0x66, 0x00, 0x33, PC_NOCOLLAPSE },
        { 0x99, 0x00, 0x33, PC_NOCOLLAPSE },
        { 0xCC, 0x00, 0x33, PC_NOCOLLAPSE },
        { 0xFF, 0x00, 0x33, PC_NOCOLLAPSE },
        { 0x00, 0x33, 0x33, PC_NOCOLLAPSE },
        { 0x33, 0x33, 0x33, PC_NOCOLLAPSE },
        { 0x66, 0x33, 0x33, PC_NOCOLLAPSE },
        { 0x99, 0x33, 0x33, PC_NOCOLLAPSE },
        { 0xCC, 0x33, 0x33, PC_NOCOLLAPSE },
        { 0xFF, 0x33, 0x33, PC_NOCOLLAPSE },
        { 0x00, 0x66, 0x33, PC_NOCOLLAPSE },
        { 0x33, 0x66, 0x33, PC_NOCOLLAPSE },
        { 0x66, 0x66, 0x33, PC_NOCOLLAPSE },
        { 0x99, 0x66, 0x33, PC_NOCOLLAPSE },
        { 0xCC, 0x66, 0x33, PC_NOCOLLAPSE },
        { 0xFF, 0x66, 0x33, PC_NOCOLLAPSE },
        { 0x00, 0x99, 0x33, PC_NOCOLLAPSE },
        { 0x33, 0x99, 0x33, PC_NOCOLLAPSE },
        { 0x66, 0x99, 0x33, PC_NOCOLLAPSE },
        { 0x99, 0x99, 0x33, PC_NOCOLLAPSE },
        { 0xCC, 0x99, 0x33, PC_NOCOLLAPSE },
        { 0xFF, 0x99, 0x33, PC_NOCOLLAPSE },
        { 0x00, 0xCC, 0x33, PC_NOCOLLAPSE },
        { 0x33, 0xCC, 0x33, PC_NOCOLLAPSE },
        { 0x66, 0xCC, 0x33, PC_NOCOLLAPSE },
        { 0x99, 0xCC, 0x33, PC_NOCOLLAPSE },
        { 0xCC, 0xCC, 0x33, PC_NOCOLLAPSE },
        { 0xFF, 0xCC, 0x33, PC_NOCOLLAPSE },
        { 0x00, 0x33, 0x00, PC_NOCOLLAPSE },
        { 0x33, 0xFF, 0x33, PC_NOCOLLAPSE },
        { 0x66, 0xFF, 0x33, PC_NOCOLLAPSE },
        { 0x99, 0xFF, 0x33, PC_NOCOLLAPSE },
        { 0xCC, 0xFF, 0x33, PC_NOCOLLAPSE },
        { 0xFF, 0xFF, 0x33, PC_NOCOLLAPSE },
        { 0x00, 0x00, 0x66, PC_NOCOLLAPSE },
        { 0x33, 0x00, 0x66, PC_NOCOLLAPSE },
        { 0x66, 0x00, 0x66, PC_NOCOLLAPSE },
        { 0x99, 0x00, 0x66, PC_NOCOLLAPSE },
        { 0xCC, 0x00, 0x66, PC_NOCOLLAPSE },
        { 0xFF, 0x00, 0x66, PC_NOCOLLAPSE },
        { 0x00, 0x33, 0x66, PC_NOCOLLAPSE },
        { 0x33, 0x33, 0x66, PC_NOCOLLAPSE },
        { 0x66, 0x33, 0x66, PC_NOCOLLAPSE },
        { 0x99, 0x33, 0x66, PC_NOCOLLAPSE },
        { 0xCC, 0x33, 0x66, PC_NOCOLLAPSE },
        { 0xFF, 0x33, 0x66, PC_NOCOLLAPSE },
        { 0x00, 0x66, 0x66, PC_NOCOLLAPSE },
        { 0x33, 0x66, 0x66, PC_NOCOLLAPSE },
        { 0x66, 0x66, 0x66, PC_NOCOLLAPSE },
        { 0x99, 0x66, 0x66, PC_NOCOLLAPSE },
        { 0xCC, 0x66, 0x66, PC_NOCOLLAPSE },
        { 0xFF, 0x66, 0x66, PC_NOCOLLAPSE },
        { 0x00, 0x99, 0x66, PC_NOCOLLAPSE },
        { 0x33, 0x99, 0x66, PC_NOCOLLAPSE },
        { 0x66, 0x99, 0x66, PC_NOCOLLAPSE },
        { 0x99, 0x99, 0x66, PC_NOCOLLAPSE },
        { 0xCC, 0x99, 0x66, PC_NOCOLLAPSE },
        { 0xFF, 0x99, 0x66, PC_NOCOLLAPSE },
        { 0x00, 0xCC, 0x66, PC_NOCOLLAPSE },
        { 0x33, 0xCC, 0x66, PC_NOCOLLAPSE },
        { 0x66, 0xCC, 0x66, PC_NOCOLLAPSE },
        { 0x99, 0xCC, 0x66, PC_NOCOLLAPSE },
        { 0xCC, 0xCC, 0x66, PC_NOCOLLAPSE },
        { 0xFF, 0xCC, 0x66, PC_NOCOLLAPSE },
        { 0x00, 0xFF, 0x66, PC_NOCOLLAPSE },
        { 0x33, 0xFF, 0x66, PC_NOCOLLAPSE },
        { 0x66, 0xFF, 0x66, PC_NOCOLLAPSE },
        { 0x99, 0xFF, 0x66, PC_NOCOLLAPSE },
        { 0xCC, 0xFF, 0x66, PC_NOCOLLAPSE },
        { 0xFF, 0xFF, 0x66, PC_NOCOLLAPSE },
        { 0x00, 0x00, 0x99, PC_NOCOLLAPSE },
        { 0x33, 0x00, 0x99, PC_NOCOLLAPSE },
        { 0x66, 0x00, 0x99, PC_NOCOLLAPSE },
        { 0x99, 0x00, 0x99, PC_NOCOLLAPSE },
        { 0xCC, 0x00, 0x99, PC_NOCOLLAPSE },
        { 0xFF, 0x00, 0x99, PC_NOCOLLAPSE },
        { 0x00, 0x33, 0x99, PC_NOCOLLAPSE },
        { 0x33, 0x33, 0x99, PC_NOCOLLAPSE },
        { 0x66, 0x33, 0x99, PC_NOCOLLAPSE },
        { 0x99, 0x33, 0x99, PC_NOCOLLAPSE },
        { 0xCC, 0x33, 0x99, PC_NOCOLLAPSE },
        { 0xFF, 0x33, 0x99, PC_NOCOLLAPSE },
        { 0x00, 0x66, 0x99, PC_NOCOLLAPSE },
        { 0x33, 0x66, 0x99, PC_NOCOLLAPSE },
        { 0x66, 0x66, 0x99, PC_NOCOLLAPSE },
        { 0x99, 0x66, 0x99, PC_NOCOLLAPSE },
        { 0xCC, 0x66, 0x99, PC_NOCOLLAPSE },
        { 0xFF, 0x66, 0x99, PC_NOCOLLAPSE },
        { 0x00, 0x99, 0x99, PC_NOCOLLAPSE },
        { 0x33, 0x99, 0x99, PC_NOCOLLAPSE },
        { 0x66, 0x99, 0x99, PC_NOCOLLAPSE },
        { 0x99, 0x99, 0x99, PC_NOCOLLAPSE },
        { 0xCC, 0x99, 0x99, PC_NOCOLLAPSE },
        { 0xFF, 0x99, 0x99, PC_NOCOLLAPSE },
        { 0x00, 0xCC, 0x99, PC_NOCOLLAPSE },
        { 0x33, 0xCC, 0x99, PC_NOCOLLAPSE },
        { 0x66, 0xCC, 0x99, PC_NOCOLLAPSE },
        { 0x99, 0xCC, 0x99, PC_NOCOLLAPSE },
        { 0xCC, 0xCC, 0x99, PC_NOCOLLAPSE },
        { 0xFF, 0xCC, 0x99, PC_NOCOLLAPSE },
        { 0x00, 0xFF, 0x99, PC_NOCOLLAPSE },
        { 0x33, 0xFF, 0x99, PC_NOCOLLAPSE },
        { 0x66, 0xFF, 0x99, PC_NOCOLLAPSE },
        { 0x99, 0xFF, 0x99, PC_NOCOLLAPSE },
        { 0xCC, 0xFF, 0x99, PC_NOCOLLAPSE },
        { 0xFF, 0xFF, 0x99, PC_NOCOLLAPSE },
        { 0x00, 0x00, 0xCC, PC_NOCOLLAPSE },
        { 0x33, 0x00, 0xCC, PC_NOCOLLAPSE },
        { 0x66, 0x00, 0xCC, PC_NOCOLLAPSE },
        { 0x99, 0x00, 0xCC, PC_NOCOLLAPSE },
        { 0xCC, 0x00, 0xCC, PC_NOCOLLAPSE },
        { 0xFF, 0x00, 0xCC, PC_NOCOLLAPSE },
        { 0x00, 0x33, 0xCC, PC_NOCOLLAPSE },
        { 0x33, 0x33, 0xCC, PC_NOCOLLAPSE },
        { 0x66, 0x33, 0xCC, PC_NOCOLLAPSE },
        { 0x99, 0x33, 0xCC, PC_NOCOLLAPSE },
        { 0xCC, 0x33, 0xCC, PC_NOCOLLAPSE },
        { 0xFF, 0x33, 0xCC, PC_NOCOLLAPSE },
        { 0x00, 0x66, 0xCC, PC_NOCOLLAPSE },
        { 0x33, 0x66, 0xCC, PC_NOCOLLAPSE },
        { 0x66, 0x66, 0xCC, PC_NOCOLLAPSE },
        { 0x99, 0x66, 0xCC, PC_NOCOLLAPSE },
        { 0xCC, 0x66, 0xCC, PC_NOCOLLAPSE },
        { 0xFF, 0x66, 0xCC, PC_NOCOLLAPSE },
        { 0x00, 0x99, 0xCC, PC_NOCOLLAPSE },
        { 0x33, 0x99, 0xCC, PC_NOCOLLAPSE },
        { 0x66, 0x99, 0xCC, PC_NOCOLLAPSE },
        { 0x99, 0x99, 0xCC, PC_NOCOLLAPSE },
        { 0xCC, 0x99, 0xCC, PC_NOCOLLAPSE },
        { 0xFF, 0x99, 0xCC, PC_NOCOLLAPSE },
        { 0x00, 0xCC, 0xCC, PC_NOCOLLAPSE },
        { 0x33, 0xCC, 0xCC, PC_NOCOLLAPSE },
        { 0x66, 0xCC, 0xCC, PC_NOCOLLAPSE },
        { 0x99, 0xCC, 0xCC, PC_NOCOLLAPSE },
        { 0xCC, 0xCC, 0xCC, PC_NOCOLLAPSE },
        { 0xFF, 0xCC, 0xCC, PC_NOCOLLAPSE },
        { 0x00, 0xFF, 0xCC, PC_NOCOLLAPSE },
        { 0x33, 0xFF, 0xCC, PC_NOCOLLAPSE },
        { 0x66, 0xFF, 0xCC, PC_NOCOLLAPSE },
        { 0x99, 0xFF, 0xCC, PC_NOCOLLAPSE },
        { 0xCC, 0xFF, 0xCC, PC_NOCOLLAPSE },
        { 0xFF, 0xFF, 0xCC, PC_NOCOLLAPSE },
        { 0xA5, 0x00, 0x21, PC_NOCOLLAPSE },
        { 0xD6, 0x00, 0x93, PC_NOCOLLAPSE },
        { 0x66, 0x00, 0xFF, PC_NOCOLLAPSE },
        { 0x99, 0x00, 0xFF, PC_NOCOLLAPSE },
        { 0xCC, 0x00, 0xFF, PC_NOCOLLAPSE },
        { 0xFF, 0x50, 0x50, PC_NOCOLLAPSE },
        { 0x33, 0x33, 0x00, PC_NOCOLLAPSE },
        { 0x33, 0x33, 0xFF, PC_NOCOLLAPSE },
        { 0x66, 0x33, 0xFF, PC_NOCOLLAPSE },
        { 0x99, 0x33, 0xFF, PC_NOCOLLAPSE },
        { 0xCC, 0x33, 0xFF, PC_NOCOLLAPSE },
        { 0xFF, 0x33, 0xFF, PC_NOCOLLAPSE },
        { 0x00, 0x66, 0xFF, PC_NOCOLLAPSE },
        { 0x33, 0x66, 0xFF, PC_NOCOLLAPSE },
        { 0x66, 0x66, 0xFF, PC_NOCOLLAPSE },
        { 0x99, 0x66, 0xFF, PC_NOCOLLAPSE },
        { 0xCC, 0x66, 0xFF, PC_NOCOLLAPSE },
        { 0xFF, 0x66, 0xFF, PC_NOCOLLAPSE },
        { 0x00, 0x99, 0xFF, PC_NOCOLLAPSE },
        { 0x33, 0x99, 0xFF, PC_NOCOLLAPSE },
        { 0x66, 0x99, 0xFF, PC_NOCOLLAPSE },
        { 0x99, 0x99, 0xFF, PC_NOCOLLAPSE },
        { 0xCC, 0x99, 0xFF, PC_NOCOLLAPSE },
        { 0xFF, 0x99, 0xFF, PC_NOCOLLAPSE },
        { 0x00, 0xCC, 0xFF, PC_NOCOLLAPSE },
        { 0x33, 0xCC, 0xFF, PC_NOCOLLAPSE },
        { 0x66, 0xCC, 0xFF, PC_NOCOLLAPSE },
        { 0x99, 0xCC, 0xFF, PC_NOCOLLAPSE },
        { 0xCC, 0xCC, 0xFF, PC_NOCOLLAPSE },
        { 0xFF, 0xCC, 0xFF, PC_NOCOLLAPSE },
        { 0x99, 0x33, 0x00, PC_NOCOLLAPSE },
        { 0x33, 0xFF, 0xFF, PC_NOCOLLAPSE },
        { 0x66, 0xFF, 0xFF, PC_NOCOLLAPSE },
        { 0x99, 0xFF, 0xFF, PC_NOCOLLAPSE },
        { 0xCC, 0xFF, 0xFF, PC_NOCOLLAPSE },
        { 0xFF, 0x7C, 0x80, PC_NOCOLLAPSE },
        { 0x04, 0x04, 0x04, PC_NOCOLLAPSE },
        { 0x08, 0x08, 0x08, PC_NOCOLLAPSE },
        { 0x0C, 0x0C, 0x0C, PC_NOCOLLAPSE },
        { 0x11, 0x11, 0x11, PC_NOCOLLAPSE },
        { 0x16, 0x16, 0x16, PC_NOCOLLAPSE },
        { 0x1C, 0x1C, 0x1C, PC_NOCOLLAPSE },
        { 0x22, 0x22, 0x22, PC_NOCOLLAPSE },
        { 0x29, 0x29, 0x29, PC_NOCOLLAPSE },
        { 0x39, 0x39, 0x39, PC_NOCOLLAPSE },
        { 0x42, 0x42, 0x42, PC_NOCOLLAPSE },
        { 0x4D, 0x4D, 0x4D, PC_NOCOLLAPSE },
        { 0x55, 0x55, 0x55, PC_NOCOLLAPSE },
        { 0x5F, 0x5F, 0x5F, PC_NOCOLLAPSE },
        { 0x77, 0x77, 0x77, PC_NOCOLLAPSE },
        { 0x86, 0x86, 0x86, PC_NOCOLLAPSE },
        { 0x96, 0x96, 0x96, PC_NOCOLLAPSE },
        { 0xB2, 0xB2, 0xB2, PC_NOCOLLAPSE },
        { 0xCB, 0xCB, 0xCB, PC_NOCOLLAPSE },
        { 0xD7, 0xD7, 0xD7, PC_NOCOLLAPSE },
        { 0xDD, 0xDD, 0xDD, PC_NOCOLLAPSE },
        { 0xE3, 0xE3, 0xE3, PC_NOCOLLAPSE },
        { 0xEA, 0xEA, 0xEA, PC_NOCOLLAPSE },
        { 0xF1, 0xF1, 0xF1, PC_NOCOLLAPSE },
        { 0xF8, 0xF8, 0xF8, PC_NOCOLLAPSE },
        { 0xE7, 0xE7, 0xD6, PC_NOCOLLAPSE },
        { 0xCC, 0xEC, 0xFF, PC_NOCOLLAPSE },
        { 0x33, 0x00, 0x00, PC_NOCOLLAPSE },
        { 0x66, 0x00, 0x00, PC_NOCOLLAPSE },
        { 0x99, 0x00, 0x00, PC_NOCOLLAPSE },
        { 0xCC, 0x00, 0x00, PC_NOCOLLAPSE },
        { 0xFF, 0xFB, 0xF0, PC_NOCOLLAPSE },
        { 0xA0, 0xA0, 0xA4, PC_NOCOLLAPSE },
        { 0x80, 0x80, 0x80, 0x00 },
        { 0xFF, 0x00, 0x00, 0x00 },
        { 0x00, 0xFF, 0x00, 0x00 },
        { 0xFF, 0xFF, 0x00, 0x00 },
        { 0x00, 0x00, 0xFF, 0x00 },
        { 0xFF, 0x00, 0xFF, 0x00 },
        { 0x00, 0xFF, 0xFF, 0x00 },
        { 0xFF, 0xFF, 0xFF, 0x00 },
    }
};
#endif

void CopyColorsFromPaletteEntries(RGBQUAD *prgb, const PALETTEENTRY *ppe,
    UINT uCount)
{
    while (uCount--)
    {
        prgb->rgbRed   = ppe->peRed;
        prgb->rgbGreen = ppe->peGreen;
        prgb->rgbBlue  = ppe->peBlue;
        prgb->rgbReserved = 0;

        prgb++;
        ppe++;
    }
}

void CopyPaletteEntriesFromColors(PALETTEENTRY *ppe, const RGBQUAD *prgb,
    UINT uCount)
{
    while (uCount--)
    {
        ppe->peRed   = prgb->rgbRed;
        ppe->peGreen = prgb->rgbGreen;
        ppe->peBlue  = prgb->rgbBlue;
        ppe->peFlags = 0;

        prgb++;
        ppe++;
    }
}

HRESULT
InitPalette()
{
    HDC hdc = GetDC(NULL);
    HRESULT hr = S_OK;

#if !defined(WIN16) && !defined(_MAC) 		
    g_hpalHalftone = SHCreateShellPalette(NULL);
    if (g_hpalHalftone)
    {
        g_lpHalftone.wCnt = (WORD)GetPaletteEntries(g_hpalHalftone, 0, 256, g_lpHalftone.ape);
        g_lpHalftone.wVer = 0x0300;
        CopyColorsFromPaletteEntries(g_rgbHalftone, g_lpHalftone.ape, g_lpHalftone.wCnt);
    }
    else
        return GetLastWin32Error();
#else
// Win16 only
    g_hpalHalftone = CreatePalette((LOGPALETTE *)&g_lpHalftone);
        // BUGBUG This will leak hdc
    if (g_hpalHalftone == NULL)
        return GetLastWin32Error();
    CopyColorsFromPaletteEntries(g_rgbHalftone, g_lpHalftone.ape, g_lpHalftone.wCnt);
#endif

#if !defined(WIN16) && !defined(_MAC)
        // Get the dithering table
        SHGetInverseCMAP((BYTE *)&g_pInvCMAP, sizeof(BYTE *));
        if (g_pInvCMAP == NULL)
            return E_OUTOFMEMORY;
#endif

    if (GetDeviceCaps(hdc, RASTERCAPS) & RC_PALETTE)
    {
#if DBG==1
        PALETTEENTRY ape[20], * pe = g_lpHalftone.ape;

        GetSystemPaletteEntries(hdc, 0, 10, ape);
        GetSystemPaletteEntries(hdc, 246, 10, ape + 10);

        if (    memcmp(ape, pe, 10 * sizeof(PALETTEENTRY)) != 0
            ||  memcmp(ape + 10, pe + 246, 10 * sizeof(PALETTEENTRY)) != 0)
        {
            TraceTag((tagPerf, "InitPalette: Unexpected system colors detected"));
        }
#endif

#ifndef NO_PERFDBG
        // Make sure we are using an identity palette
// Disable for IDW build
//        Assert(IsIdentityPalette(g_hpalHalftone));
#endif

        // Generate the inverse mapping table

        g_crPaletteRelative = 0x02000000;
    }
    else
        g_crPaletteRelative = 0;

    ReleaseDC(NULL, hdc);

    RRETURN(hr);
}

void
DeinitPalette()
{
    if (g_hdcMem1)
    {
        TraceTag((tagDCTrace, "GetMemoryDC, about to delete dc, count=%d", --++g_cGetMemDc));
        DeleteDC(g_hdcMem1);
    }
    if (g_hdcMem2)
    {
        TraceTag((tagDCTrace, "GetMemoryDC, about to delete dc, count=%d", --++g_cGetMemDc));
        DeleteDC(g_hdcMem2);
    }
    if (g_hdcMem3)
    {
        TraceTag((tagDCTrace, "GetMemoryDC, about to delete dc, count=%d", --++g_cGetMemDc));
        DeleteDC(g_hdcMem3);
    }

    if (g_hpalHalftone)
    {
        DeleteObject(g_hpalHalftone);
        g_hpalHalftone = NULL;
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   GetDefaultPalette
//
//  Synopsis:   Returns a generic (halftone) palette, optionally selecting it
//              into the DC.
//
//              Where possible, use CDoc::GetPalette instead.
//
//----------------------------------------------------------------------------

HPALETTE
GetDefaultPalette(HDC hdc)
{
    HPALETTE hpal = NULL;

    if (g_crPaletteRelative)
    {
        hpal = g_hpalHalftone;
        if (hdc)
        {
            SelectPalette(hdc, hpal, TRUE);
            RealizePalette(hdc);
        }
    }
    return hpal;
}

//+---------------------------------------------------------------------------
//
//  Function:   IsHalftonePalette
//
//  Synopsis:   Returns TRUE if the passed palette exactly matches the
//              halftone palette.
//
//----------------------------------------------------------------------------

BOOL
IsHalftonePalette(HPALETTE hpal)
{
    int cColors;
    PALETTEENTRY ape[256];

    if (!hpal)
        return FALSE;

    if (hpal == g_hpalHalftone)
        return TRUE;

    cColors = GetPaletteEntries(hpal, 0, 0, NULL);

    // Right number of colors?
    if (cColors != ARRAY_SIZE(ape))
        return FALSE;
    
    GetPaletteEntries(hpal, 0, cColors, ape);

    return (!memcmp(ape, g_lpHalftone.ape, sizeof(PALETTEENTRY) * ARRAY_SIZE(ape)));
}

//+---------------------------------------------------------------------------
//
//  Function:   InitColorTranslation
//
//  Synopsis:   Initialize system color translation table.
//
//----------------------------------------------------------------------------

void
InitColorTranslation()
{
    for (int i = 0; i < ARRAY_SIZE(g_acrSysColor); i++)
    {
                // WinCE redefines all the GetSysColor defines
#if defined(WINCE) && !defined(WINCE_NT)
        g_acrSysColor[i] = GetSysColor(i | COLOR_INDEX_MASK);
#else
        g_acrSysColor[i] = GetSysColor(i);
#endif
    }

    g_fSysColorInit = TRUE;
}

//+---------------------------------------------------------------------------
//
//  Function:   GetSysColorQuick
//
//  Synopsis:   Looks up system colors from a cache.
//
//----------------------------------------------------------------------------

COLORREF GetSysColorQuick(int i)
{
    if (!g_fSysColorInit)
        InitColorTranslation();

    return g_acrSysColor[i];
}

//+---------------------------------------------------------------------------
//
//  Function:   FormsTranslateColor
//
//  Synopsis:   Map OLE_COLORs to COLORREFs.
//              This API is identical to OleTranslateColor.
//
//----------------------------------------------------------------------------

STDAPI
FormsTranslateColor(OLE_COLOR clr, HPALETTE hpal, COLORREF * pcr)
{
    int syscolor;

    switch (clr & 0xff000000)
    {
    case 0x80000000:

        syscolor = clr & 0x00ffffff;

        if (syscolor >= ARRAY_SIZE(g_acrSysColor))
            RRETURN(E_INVALIDARG);

        clr = GetSysColorQuick(syscolor);

        break;

    case 0x01000000 :
        // check validity of index
        if (hpal)
        {
            PALETTEENTRY pe;
            // try to get palette entry, if it fails we assume index is invalid
            if (!GetPaletteEntries(hpal, (UINT)(clr & 0xffff), 1, &pe))
                RRETURN(E_INVALIDARG);        // BUGBUG? : CDK uses CTL_E_OVERFLOW
        }
        break;

#ifdef UNIX
    case 0x04000000 :   // Motif System Color
#endif
    case 0x02000000 :
        break;

    case 0:
        if (hpal != NULL)
        {
            clr |= 0x02000000;
        }
        break;

    default :
        RRETURN(E_INVALIDARG);
    }

    if (pcr)
        *pcr = clr;

    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Function:   IsOleColorValid
//
//  Synopsis:   Return true if the given OLE_COLOR is valid.
//
//----------------------------------------------------------------------------

BOOL
IsOleColorValid(OLE_COLOR clr)
{
    //
    // BUGBUG -- Why aren't we calling OleTranslateColor?
    //
    return OK(FormsTranslateColor(clr, NULL, NULL));
}


//+---------------------------------------------------------------------------
//
//  Function:   ColorRefFromOleColor
//
//  Synopsis:   Map OLE_COLORs to COLORREFs.  This function does not contain
//              any error checking.   Callers should validate the color with
//              IsOleColorValid() before calling this function.
//
//----------------------------------------------------------------------------

COLORREF
ColorRefFromOleColor(OLE_COLOR clr)
{
// Under WinCE all the colors come in as invalid--starting with 0xC... instead of 0x8
#if defined(WINCE) && !defined(WINCE_NT)
        clr &= 0x80ffffff;
#endif // WINCE
    Assert(IsOleColorValid(clr));
    Assert((clr & 0x02000000) == 0);

    if ((long)clr < 0)
    {
        return GetSysColorQuick(clr & 0xff);
    }
    else
    {
#ifdef UNIX
        if ( CColorValue(clr).IsMotifColor() ) // Motif System Color
            return clr;
#endif

        return clr | g_crPaletteRelative;
    }
}

HDC GetMemoryDC()
{
    HDC hdcMem;

    if (g_fNoDisplayChange)
    {
        hdcMem = (HDC)InterlockedExchangePointer((void **)&g_hdcMem1, NULL);
        if (hdcMem)
        {
            TraceTag((tagDCTrace, "GetMemoryDC, about to return dc #1, count still %d", g_cGetMemDc));
            return(hdcMem);
        }

        hdcMem = (HDC)InterlockedExchangePointer((void **)&g_hdcMem2, NULL);
        if (hdcMem)
        {
            TraceTag((tagDCTrace, "GetMemoryDC, about to return dc #2, count still %d", g_cGetMemDc));
            return(hdcMem);
        }

        hdcMem = (HDC)InterlockedExchangePointer((void **)&g_hdcMem3, NULL);
        if (hdcMem)
        {
            TraceTag((tagDCTrace, "GetMemoryDC, about to return dc #3, count still %d", g_cGetMemDc));
            return(hdcMem);
        }

        TraceTag((tagDCTrace, "GetMemoryDC, about to return new dc, count=%d", ++g_cGetMemDc));
    }

    hdcMem = CreateCompatibleDC(NULL);

    if (hdcMem)
    {
        SetStretchBltMode(hdcMem, COLORONCOLOR);
        GetDefaultPalette(hdcMem);
    }

    return(hdcMem);
}

void ReleaseMemoryDC(HDC hdcIn)
{
    HDC hdc = hdcIn;

    if (g_fNoDisplayChange)
    {
        hdc = (HDC)InterlockedExchangePointer((void **)&g_hdcMem1, hdc);
        if (hdc == NULL)
        {
            TraceTag((tagDCTrace, "GetMemoryDC, stored a dc in #1, count still %d", g_cGetMemDc));
            return;
        }

        hdc = (HDC)InterlockedExchangePointer((void **)&g_hdcMem2, hdc);
        if (hdc == NULL)
        {
            TraceTag((tagDCTrace, "GetMemoryDC, stored a dc in #2, count still %d", g_cGetMemDc));
            return;
        }

        hdc = (HDC)InterlockedExchangePointer((void **)&g_hdcMem3, hdc);
        if (hdc == NULL)
        {
            TraceTag((tagDCTrace, "GetMemoryDC, stored a dc in #3, count still %d", g_cGetMemDc));
            return;
        }

        TraceTag((tagDCTrace, "GetMemoryDC, about to delete dc, count=%d", --++g_cGetMemDc));
    }

    DeleteDC(hdc);
}

CColorInfo::CColorInfo() : _dwDrawAspect(DVASPECT_CONTENT) , _lindex(-1), _pvAspect(NULL), _ptd(NULL), _hicTargetDev(NULL), _cColors(0) , _cColorsMax(256)
{
}

CColorInfo::CColorInfo(DWORD dwDrawAspect, LONG lindex, void FAR *pvAspect, DVTARGETDEVICE FAR *ptd, HDC hicTargetDev, unsigned cColorsMax) :
    _dwDrawAspect(dwDrawAspect), _lindex(lindex) , _pvAspect(pvAspect) , _ptd(ptd) , _hicTargetDev(hicTargetDev) , _cColors(0)
{
    _cColorsMax = max((unsigned)256, cColorsMax);
}

HRESULT
CColorInfo::AddColors(HPALETTE hpal)
{
    unsigned cColors = GetPaletteEntries(hpal, 0, 0, NULL);
    cColors = min(_cColorsMax - _cColors, cColors);

    if (cColors)
    {
        GetPaletteEntries(hpal, 0, cColors, &_aColors[_cColors]);
        _cColors += cColors;
    }
    RRETURN(S_OK);
}

HRESULT
CColorInfo::AddColors(LOGPALETTE *pLogPal)
{
    RRETURN(AddColors(pLogPal->palNumEntries, pLogPal->palPalEntry));
}

HRESULT
CColorInfo::AddColors(unsigned cColors, PALETTEENTRY *pColors)
{
    // The check for system colors assumes they occur in the same order as
    // the real system colors.  This is the simplest possible fix for now.

    // Remove any system colors at the end (do this first so that cColors makes sense)

    Assert(g_lpHalftone.wCnt == 256);
    Assert(sizeof(PALETTEENTRY) == sizeof(DWORD));

    DWORD *pSystem = (DWORD *)&g_lpHalftone.ape[g_lpHalftone.wCnt - 1];
    DWORD *pInput = (DWORD *)&pColors[cColors - 1];
    while (cColors && (*pSystem == *pInput))
    {
        pSystem--;
        pInput--;
        cColors--;
    }

    // Remove any system colors at the beginning
    pSystem = (DWORD *)(g_lpHalftone.ape);
    pInput = (DWORD *)pColors;
    while (cColors && (*pSystem == *pInput))
    {
        pSystem++;
        pInput++;
        cColors--;
    }
    pColors = (PALETTEENTRY *)pInput;

    cColors = min(_cColorsMax - _cColors, cColors);
    if (cColors)
    {
        memcpy(&_aColors[_cColors], pColors, cColors * sizeof(PALETTEENTRY));
        _cColors += cColors;
    }

    RRETURN(S_OK);
}

HRESULT
CColorInfo::AddColors(unsigned cColors, RGBQUAD *pColors)
{
    cColors = min(_cColorsMax - _cColors, cColors);
    if (cColors)
    {
        CopyPaletteEntriesFromColors(&_aColors[_cColors], pColors, cColors);
        _cColors += cColors;
    }

    RRETURN(S_OK);
}

HRESULT
CColorInfo::AddColors(unsigned cColors, COLORREF *pColors)
{
    cColors = min(_cColorsMax - _cColors, cColors);
    while (cColors)
    {
        _aColors[_cColors].peRed = GetRValue(*pColors);
        _aColors[_cColors].peGreen = GetGValue(*pColors);
        _aColors[_cColors].peBlue = GetBValue(*pColors);
        _aColors[_cColors].peFlags = 0;
        pColors++;
    }

    RRETURN(S_OK);
}

HRESULT
CColorInfo::AddColors(IViewObject *pVO)
{
    LPLOGPALETTE pColors = NULL;
    HRESULT hr = pVO->GetColorSet(_dwDrawAspect, _lindex, _pvAspect, _ptd, _hicTargetDev, &pColors);
    if (FAILED(hr))
    {
        hr = E_NOTIMPL;
    }
    else if (hr == S_OK && pColors)
    {
        AddColors(pColors);
        CoTaskMemFree(pColors);

    }

    RRETURN1(hr, S_FALSE);
}

BOOL
CColorInfo::IsFull()
{
    Assert(_cColors <= _cColorsMax);
    return _cColors >= _cColorsMax;
}

HRESULT
CColorInfo::GetColorSet(LPLOGPALETTE FAR *ppColors)
{
    *ppColors = 0;
    if (_cColors == 0)
        RRETURN1(S_FALSE, S_FALSE);

    LOGPALETTE *pColors;

    //
    // It's just easier to allocate 256 colors instead of messing about.
    //
    *ppColors = pColors = (LOGPALETTE *)CoTaskMemAlloc(sizeof(LOGPALETTE) + 255 * sizeof(PALETTEENTRY));

    if (!pColors)
        RRETURN(E_OUTOFMEMORY);

    // This will ensure that we have a reasonable set of colors, including
    // the system colors to start
    memcpy(pColors, &g_lpHalftone, sizeof(g_lpHalftone));

    unsigned cColors = min((unsigned)236, _cColors);

    // Notice that we avoid overwriting the beginning of the _aColors array.
    // The assumption is that the colors are in some kind of order.
    memcpy(pColors->palPalEntry + 10, _aColors, cColors * sizeof(PALETTEENTRY));

    for (unsigned i = 10 ; i < (cColors + 10); i++)
        pColors->palPalEntry[i].peFlags = PC_NOCOLLAPSE;

    RRETURN(S_OK);
}
#if DBG == 1

ExternTag(tagPalette);

void DumpPalette(CHAR *szName, unsigned cColors, PALETTEENTRY *pColors)
{
    if (pColors)
    {
        TraceTagEx((tagPalette, TAG_NONAME | TAG_NONEWLINE | TAG_USECONSOLE, "DumpPalette: %s\n", szName));
        TraceTagEx((tagPalette, TAG_NONAME | TAG_NONEWLINE | TAG_USECONSOLE, "idx r  g  b  flags"));
        for (unsigned i = 0 ; i < cColors ; i++)
        {
            TraceTagEx((tagPalette, TAG_NONAME | TAG_NONEWLINE | TAG_USECONSOLE, "%02x: %02x %02x %02x %02x", i, pColors[i].peRed, pColors[i].peGreen, pColors[i].peBlue, pColors[i].peFlags));
            if (i % 8 == 0)
                TraceTagEx((tagPalette, TAG_NONAME | TAG_NONEWLINE | TAG_USECONSOLE, "\n"));
        }
    }
    else
        TraceTag((tagPalette, "%s is NULL", szName));
}

void DumpPalette(CHAR *szName, LOGPALETTE *lp)
{
    if (lp)
        DumpPalette(szName, lp->palNumEntries, lp->palPalEntry);
    else
        TraceTag((tagPalette, "%s is NULL", szName));
}

void DumpPalette(CHAR *szName, HPALETTE hpal)
{
    if (hpal)
    {
        PALETTEENTRY ape[256];
        DumpPalette(szName, GetPaletteEntries(hpal, 0, 256, ape), ape);
    }
    else
        TraceTag((tagPalette, "%s is NULL", szName));
}

#endif

