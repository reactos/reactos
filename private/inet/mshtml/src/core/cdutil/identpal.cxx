//+------------------------------------------------------------------------
//
//  File:       identpal.cxx
//
//  Contents:   Debug routines for faster BitBlt to palette devices
//
//-------------------------------------------------------------------------

#include "headers.hxx"

DeclareTag(tagIdentity, "Performance", "Warn if non-identity palette, BitBlt");

BOOL 
IsSameAsPhysicalPalette(HPALETTE hpal)
{
    BOOL fIdentityPalette = TRUE;

    if ((GetDeviceCaps(TLS(hdcDesktop), RASTERCAPS) & RC_PALETTE) &&
        GetDeviceCaps(TLS(hdcDesktop), BITSPIXEL) * GetDeviceCaps(TLS(hdcDesktop), PLANES) == 8)
    {
        PALETTEENTRY apeSystem[256];
        PALETTEENTRY apePalette[256];

        GetSystemPaletteEntries(TLS(hdcDesktop), 0, ARRAY_SIZE(apeSystem), apeSystem);
        GetPaletteEntries(hpal, 0, ARRAY_SIZE(apePalette), apePalette);

        for (int i = 256; --i >= 0; )
        {
            if (apePalette[i].peRed != apeSystem[i].peRed ||
                apePalette[i].peGreen != apeSystem[i].peGreen ||
                apePalette[i].peBlue != apeSystem[i].peBlue)
            {
                fIdentityPalette = FALSE;
                break;
            }
        }
    }

    return fIdentityPalette;
}

/****************************************************************************

 IsIdentityPalette

    handy debug code to determine if a palette has a 1:1 logical to
    foreground translate (ie is a identity palette)

    Identity palettes are very important for windows apps using DIBs,
    DIBSections, WinGBitmaps in order to get the fastest possible
    speed out of BitBlt.

    it might be a good idea to put a Assert(IsIdentityPalette(hpal))
    in your app if you care about Blt speed.

    further reading....

        WinG help file (on MSDN) - great discusion of Identity palettes
        why you need them, how to get them etc...

        Win32 Animation techniques, Nigel Thompson

        MSDN search for identity

    03/02/95    ToddLa

 ****************************************************************************/


BOOL 
IsIdentityPalette(HPALETTE hpal)
{
    BOOL fIdentityPalette = TRUE;
    HDC hdcS;

    hdcS = GetDC(NULL);

    if ((GetDeviceCaps(hdcS, RASTERCAPS) & RC_PALETTE) &&
        GetDeviceCaps(hdcS, BITSPIXEL) * GetDeviceCaps(hdcS, PLANES) == 8)
    {
        int n=0;
        int i;
        BYTE xlat[256];
        HBITMAP hbm;
        HDC hdcM;

        GetObject(hpal, sizeof(n), &n);

        hdcM = CreateCompatibleDC(hdcS);
        hbm = CreateCompatibleBitmap(hdcS, 256, 1);
        SelectObject(hdcM, hbm);

        SelectPalette(hdcM, hpal, TRUE);
        RealizePalette(hdcM);
        for (i=0; i<n; i++)
        {
            SetPixel(hdcM, i, 0, PALETTEINDEX(i));
        }
        GetBitmapBits(hbm, sizeof(xlat), xlat);
        SelectPalette(hdcM, (HPALETTE)GetStockObject(DEFAULT_PALETTE), FALSE);

        DeleteDC(hdcM);
        DeleteObject(hbm);

        for (i=0; i<n; i++)
        {
            if (xlat[i] != i)
            {
                TraceTag((tagIdentity, "Using non-identity palette"));
                fIdentityPalette = FALSE;
                break;
            }
        }
    }

    ReleaseDC(NULL, hdcS);

    return fIdentityPalette;
}

/****************************************************************************

 IsIdentityBlt

    handy debug code to determine if a DibSection Blt is 1:1

  hdcS	- screen DC
  hdcD  - DIBSection DC
  xWid  - row width of hdcD
    
    03/02/95    ToddLa

 ****************************************************************************/

BOOL 
IsIdentityBlt(HDC hdcS, HDC hdcD, int xWid)
{
    BOOL fIdentityBlt = TRUE;

    if (    xWid > 0
        &&  (GetDeviceCaps(hdcS, RASTERCAPS) & RC_PALETTE)
        &&  GetDeviceCaps(hdcS, BITSPIXEL) * GetDeviceCaps(hdcS, PLANES) == 8)
    {
        int i, j, xSpan;
        BYTE xlat[256];
        HBITMAP hbm;
        HDC hdcM;
        COLORREF argb[256];
        HPALETTE hpal;

        hpal = (HPALETTE)GetCurrentObject(hdcS, OBJ_PAL);

        hdcM = CreateCompatibleDC(hdcS);
        hbm = CreateCompatibleBitmap(hdcS, 256, 1);

        SelectObject(hdcM, hbm);

        SelectPalette(hdcM, hpal, TRUE);
        RealizePalette(hdcM);

        if (xWid > 256)
            xWid = 256;

        for (i=0; i<xWid; i++)
        {
            argb[i] = GetPixel(hdcD, i, 0);
        }

        for (j=0; j<256; j += xWid)
        {
            xSpan = min(xWid, 256 - j);

            for (i = 0; i < xSpan; i++)
            {
                SetPixel(hdcD, i, 0, PALETTEINDEX(j + i));
            }

            BitBlt(hdcM, 0, 0, xSpan, 1, hdcD, 0, 0, SRCCOPY);

            GetBitmapBits(hbm, xSpan, xlat);

            for (i=0; i < xSpan; i++)
            {
                if (xlat[i] != j + i)
                {
                    TraceTag((tagIdentity, "Performing non-identity BitBlt"));
                    fIdentityBlt = FALSE;
                    break;
                }
            }

            if (!fIdentityBlt)
                break;
        }

        SelectPalette(hdcM, (HPALETTE)GetStockObject(DEFAULT_PALETTE), FALSE);
        DeleteDC(hdcM);
        DeleteObject(hbm);

        for (i=0; i<xWid; i++)
        {
            SetPixel(hdcD, i, 0, argb[i]);
        }
    }    

    return fIdentityBlt;
}
