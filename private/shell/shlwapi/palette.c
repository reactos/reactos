/***************************************************************************
 *  palette.c
 *
 *  Common halftone palette used by shell.   
 *
 ***************************************************************************/

#include "priv.h"
#include "palette.h"

#ifdef UNIX
#include <mainwin.h>
#endif /* UNIX */

BYTE * g_pbWinNTCMAP = NULL;

// This function behaves the same as CreateHalftone palette:
//      hdc == NULL         always return full palette
//      hdc palettized      return full palette
//      hdc not palettized  return default palette (VGA colors)

HPALETTE SHCreateShellPalette(HDC hdc)
{
    HPALETTE        hpalHalftone;
    LOGPAL256       lp;
    BOOL            fGetSystemColors = FALSE;
    
    // We would like to use CreateHalftonePalette() always but they
    // differ significantly between NT and Win95.  Win95's is very
    // close to Netscape's; NT's is significantly different (color
    // cube cut differently) and it is not an identity palette.
    //
    // So, we will use CreateHalftonePalette() on Win95 and on NT
    // we will use a custom palette containing the same colors in
    // Win95's halftone palette ordered so that the color flash
    // will be minimized when switching between NT's halftone palette
    // and our palette.
    //
    // On NT 5 and later the halftone palette matches Win95's so the
    // custom palette will only be used on machines running NT 4 or below.
    // However, we still need to patch in the system colors on NT 5.

#ifdef UNIX
    if (MwIsInitLite())
       return NULL;
#endif /* UNIX */

    if (!g_bRunningOnNT || g_bRunningOnNT5OrHigher)
    {
        hpalHalftone = CreateHalftonePalette(hdc);

        if (g_bRunningOnNT5OrHigher && hpalHalftone)
        {
            lp.wCnt = (WORD)GetPaletteEntries(hpalHalftone, 0, 256, lp.ape);
            lp.wVer = 0x0300;
            fGetSystemColors = TRUE;
            DeleteObject(hpalHalftone);
        }
    }
    else if (hdc == NULL || (GetDeviceCaps(hdc, RASTERCAPS) & RC_PALETTE))
    {
        memcpy(&lp, &g_lpHalftone, sizeof(g_lpHalftone));
        fGetSystemColors = TRUE;
    }
    else
        hpalHalftone = GetStockObject(DEFAULT_PALETTE);

    if (fGetSystemColors)
    {
        HDC hdcScreen = hdc;

        if (hdcScreen == NULL)
            hdcScreen = CreateCompatibleDC(NULL);
#ifndef UNIX
        if (hdcScreen)
        {
            GetSystemPaletteEntries(hdcScreen, 0, 10, lp.ape);
            GetSystemPaletteEntries(hdcScreen, 246, 10, lp.ape + 246);
        }
#endif
        hpalHalftone = CreatePalette((LOGPALETTE *)&lp);
        
        if (hdc == NULL && hdcScreen)
            DeleteDC(hdcScreen);
    }

    return hpalHalftone;
}

static const BYTE *GetInverseCMAP()
{
    const BYTE * abWin95ToNT;

    if (!g_bRunningOnNT)
    {
        return g_abWin95CMAP;
    }

    abWin95ToNT = (g_bRunningOnNT5OrHigher ? g_abWin95ToNT5 : g_abWin95ToNT4);

    if (g_pbWinNTCMAP == NULL)
    {
        BYTE * pbMap = LocalAlloc(LPTR, 32768);
        if (pbMap)
        {
            int i;
            BYTE * pbDst = pbMap;
            const BYTE * pbSrc = g_abWin95CMAP;
            for (i = 0; i < 32768; ++i)
            {
                *pbDst++ = abWin95ToNT[*pbSrc++];
            }
            if (SHInterlockedCompareExchange((void **)&g_pbWinNTCMAP, pbMap, NULL))
            {
                LocalFree(pbMap);   // race, get rid of dupe copy
            }
        }
    }
    return g_pbWinNTCMAP;
}

HRESULT SHGetInverseCMAP(BYTE *pbMap, ULONG cbMap)
{
    const BYTE *pbSrc;

    if (pbMap == NULL)
        return E_POINTER;
        
    if (cbMap != 32768 && cbMap != sizeof(BYTE *))
        return E_INVALIDARG;

    pbSrc = GetInverseCMAP();

    if (pbSrc == NULL)
        return E_OUTOFMEMORY;

    if (cbMap == sizeof(BYTE *))
    {
        *(const BYTE **)pbMap = pbSrc;
    }
    else
    {
        memcpy(pbMap, pbSrc, 32768);
    }

    return(S_OK);
}


void TermPalette()
{
    if (g_pbWinNTCMAP)
        LocalFree(g_pbWinNTCMAP);
}
