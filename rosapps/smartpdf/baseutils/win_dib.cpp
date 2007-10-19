// CDIBSectionLite.cpp : implementation file
//
// General purpose DIBsection wrapper class for Win9x, NT 4.0, W2K and WinCE.
//
// Author      : Chris Maunder (cmaunder@mail.com)
// Date        : 17 May 1999
//
// Copyright © Dundas Software Ltd. 1999, All Rights Reserved
//
// This code may be used in compiled form in any way you desire. This
// file may be redistributed unmodified by any means PROVIDING it is
// not sold for profit without the authors written consent, and
// providing that this notice and the authors name is included. If
// the source code in this file is used in any commercial application
// then a simple email would be nice.
//
// This file is provided "as is" with no expressed or implied warranty.
// The author accepts no liability for any damage, in any form, caused
// by this code. Use it at your own risk and as with all code expect bugs!
// It's been tested but I'm not perfect.
//
// Please use and enjoy. Please let me know of any bugs/mods/improvements
// that you have found/implemented and I will fix/incorporate them into this
// file.
//
// History :  25 May 1999 - First release
//             4 Jun 1999 - Fixed SetBitmap bug
//             4 May 2000 - 16 or 32 bit compression bug fix (Jim Miller <jam@3dfx.com>)
//                          Bug fix in Save() (saving 4 bytes too many - Tadeusz Dracz)
//			  12 Dec 2000 - KennyG@magenic.com - Massive MFC extraxtion, remove that bloat!
//
#include "stdafx.h"
#include "CDIB.h"

// Standard colors
RGBQUAD CDIB::ms_StdColors[] = {
    { 0x00, 0x00, 0x00, 0 },  // System palette - first 10 colors
    { 0x80, 0x00, 0x00, 0 },
    { 0x00, 0x80, 0x00, 0 },
    { 0x80, 0x80, 0x00, 0 },
    { 0x00, 0x00, 0x80, 0 },
    { 0x80, 0x00, 0x80, 0 },
    { 0x00, 0x80, 0x80, 0 },
    { 0xC0, 0xC0, 0xC0, 0 },
    { 0xC0, 0xDC, 0xC0, 0 },
    { 0xA6, 0xCA, 0xF0, 0 },

    { 0x2C, 0x00, 0x00, 0 },
    { 0x56, 0x00, 0x00, 0 },
    { 0x87, 0x00, 0x00, 0 },
    { 0xC0, 0x00, 0x00, 0 },
    { 0xFF, 0x00, 0x00, 0 },
    { 0x00, 0x2C, 0x00, 0 },
    { 0x2C, 0x2C, 0x00, 0 },
    { 0x56, 0x2C, 0x00, 0 },
    { 0x87, 0x2C, 0x00, 0 },
    { 0xC0, 0x2C, 0x00, 0 },
    { 0xFF, 0x2C, 0x00, 0 },
    { 0x00, 0x56, 0x00, 0 },
    { 0x2C, 0x56, 0x00, 0 },
    { 0x56, 0x56, 0x00, 0 },
    { 0x87, 0x56, 0x00, 0 },
    { 0xC0, 0x56, 0x00, 0 },
    { 0xFF, 0x56, 0x00, 0 },
    { 0x00, 0x87, 0x00, 0 },
    { 0x2C, 0x87, 0x00, 0 },
    { 0x56, 0x87, 0x00, 0 },
    { 0x87, 0x87, 0x00, 0 },
    { 0xC0, 0x87, 0x00, 0 },
    { 0xFF, 0x87, 0x00, 0 },
    { 0x00, 0xC0, 0x00, 0 },
    { 0x2C, 0xC0, 0x00, 0 },
    { 0x56, 0xC0, 0x00, 0 },
    { 0x87, 0xC0, 0x00, 0 },
    { 0xC0, 0xC0, 0x00, 0 },
    { 0xFF, 0xC0, 0x00, 0 },
    { 0x00, 0xFF, 0x00, 0 },
    { 0x2C, 0xFF, 0x00, 0 },
    { 0x56, 0xFF, 0x00, 0 },
    { 0x87, 0xFF, 0x00, 0 },
    { 0xC0, 0xFF, 0x00, 0 },
    { 0xFF, 0xFF, 0x00, 0 },
    { 0x00, 0x00, 0x2C, 0 },
    { 0x2C, 0x00, 0x2C, 0 },
    { 0x56, 0x00, 0x2C, 0 },
    { 0x87, 0x00, 0x2C, 0 },
    { 0xC0, 0x00, 0x2C, 0 },
    { 0xFF, 0x00, 0x2C, 0 },
    { 0x00, 0x2C, 0x2C, 0 },
    { 0x2C, 0x2C, 0x2C, 0 },
    { 0x56, 0x2C, 0x2C, 0 },
    { 0x87, 0x2C, 0x2C, 0 },
    { 0xC0, 0x2C, 0x2C, 0 },
    { 0xFF, 0x2C, 0x2C, 0 },
    { 0x00, 0x56, 0x2C, 0 },
    { 0x2C, 0x56, 0x2C, 0 },
    { 0x56, 0x56, 0x2C, 0 },
    { 0x87, 0x56, 0x2C, 0 },
    { 0xC0, 0x56, 0x2C, 0 },
    { 0xFF, 0x56, 0x2C, 0 },
    { 0x00, 0x87, 0x2C, 0 },
    { 0x2C, 0x87, 0x2C, 0 },
    { 0x56, 0x87, 0x2C, 0 },
    { 0x87, 0x87, 0x2C, 0 },
    { 0xC0, 0x87, 0x2C, 0 },
    { 0xFF, 0x87, 0x2C, 0 },
    { 0x00, 0xC0, 0x2C, 0 },
    { 0x2C, 0xC0, 0x2C, 0 },
    { 0x56, 0xC0, 0x2C, 0 },
    { 0x87, 0xC0, 0x2C, 0 },
    { 0xC0, 0xC0, 0x2C, 0 },
    { 0xFF, 0xC0, 0x2C, 0 },
    { 0x00, 0xFF, 0x2C, 0 },
    { 0x2C, 0xFF, 0x2C, 0 },
    { 0x56, 0xFF, 0x2C, 0 },
    { 0x87, 0xFF, 0x2C, 0 },
    { 0xC0, 0xFF, 0x2C, 0 },
    { 0xFF, 0xFF, 0x2C, 0 },
    { 0x00, 0x00, 0x56, 0 },
    { 0x2C, 0x00, 0x56, 0 },
    { 0x56, 0x00, 0x56, 0 },
    { 0x87, 0x00, 0x56, 0 },
    { 0xC0, 0x00, 0x56, 0 },
    { 0xFF, 0x00, 0x56, 0 },
    { 0x00, 0x2C, 0x56, 0 },
    { 0x2C, 0x2C, 0x56, 0 },
    { 0x56, 0x2C, 0x56, 0 },
    { 0x87, 0x2C, 0x56, 0 },
    { 0xC0, 0x2C, 0x56, 0 },
    { 0xFF, 0x2C, 0x56, 0 },
    { 0x00, 0x56, 0x56, 0 },
    { 0x2C, 0x56, 0x56, 0 },
    { 0x56, 0x56, 0x56, 0 },
    { 0x87, 0x56, 0x56, 0 },
    { 0xC0, 0x56, 0x56, 0 },
    { 0xFF, 0x56, 0x56, 0 },
    { 0x00, 0x87, 0x56, 0 },
    { 0x2C, 0x87, 0x56, 0 },
    { 0x56, 0x87, 0x56, 0 },
    { 0x87, 0x87, 0x56, 0 },
    { 0xC0, 0x87, 0x56, 0 },
    { 0xFF, 0x87, 0x56, 0 },
    { 0x00, 0xC0, 0x56, 0 },
    { 0x2C, 0xC0, 0x56, 0 },
    { 0x56, 0xC0, 0x56, 0 },
    { 0x87, 0xC0, 0x56, 0 },
    { 0xC0, 0xC0, 0x56, 0 },
    { 0xFF, 0xC0, 0x56, 0 },
    { 0x00, 0xFF, 0x56, 0 },
    { 0x2C, 0xFF, 0x56, 0 },
    { 0x56, 0xFF, 0x56, 0 },
    { 0x87, 0xFF, 0x56, 0 },
    { 0xC0, 0xFF, 0x56, 0 },
    { 0xFF, 0xFF, 0x56, 0 },
    { 0x00, 0x00, 0x87, 0 },
    { 0x2C, 0x00, 0x87, 0 },
    { 0x56, 0x00, 0x87, 0 },
    { 0x87, 0x00, 0x87, 0 },
    { 0xC0, 0x00, 0x87, 0 },
    { 0xFF, 0x00, 0x87, 0 },
    { 0x00, 0x2C, 0x87, 0 },
    { 0x2C, 0x2C, 0x87, 0 },
    { 0x56, 0x2C, 0x87, 0 },
    { 0x87, 0x2C, 0x87, 0 },
    { 0xC0, 0x2C, 0x87, 0 },
    { 0xFF, 0x2C, 0x87, 0 },
    { 0x00, 0x56, 0x87, 0 },
    { 0x2C, 0x56, 0x87, 0 },
    { 0x56, 0x56, 0x87, 0 },
    { 0x87, 0x56, 0x87, 0 },
    { 0xC0, 0x56, 0x87, 0 },
    { 0xFF, 0x56, 0x87, 0 },
    { 0x00, 0x87, 0x87, 0 },
    { 0x2C, 0x87, 0x87, 0 },
    { 0x56, 0x87, 0x87, 0 },
    { 0x87, 0x87, 0x87, 0 },
    { 0xC0, 0x87, 0x87, 0 },
    { 0xFF, 0x87, 0x87, 0 },
    { 0x00, 0xC0, 0x87, 0 },
    { 0x2C, 0xC0, 0x87, 0 },
    { 0x56, 0xC0, 0x87, 0 },
    { 0x87, 0xC0, 0x87, 0 },
    { 0xC0, 0xC0, 0x87, 0 },
    { 0xFF, 0xC0, 0x87, 0 },
    { 0x00, 0xFF, 0x87, 0 },
    { 0x2C, 0xFF, 0x87, 0 },
    { 0x56, 0xFF, 0x87, 0 },
    { 0x87, 0xFF, 0x87, 0 },
    { 0xC0, 0xFF, 0x87, 0 },
    { 0xFF, 0xFF, 0x87, 0 },
    { 0x00, 0x00, 0xC0, 0 },
    { 0x2C, 0x00, 0xC0, 0 },
    { 0x56, 0x00, 0xC0, 0 },
    { 0x87, 0x00, 0xC0, 0 },
    { 0xC0, 0x00, 0xC0, 0 },
    { 0xFF, 0x00, 0xC0, 0 },
    { 0x00, 0x2C, 0xC0, 0 },
    { 0x2C, 0x2C, 0xC0, 0 },
    { 0x56, 0x2C, 0xC0, 0 },
    { 0x87, 0x2C, 0xC0, 0 },
    { 0xC0, 0x2C, 0xC0, 0 },
    { 0xFF, 0x2C, 0xC0, 0 },
    { 0x00, 0x56, 0xC0, 0 },
    { 0x2C, 0x56, 0xC0, 0 },
    { 0x56, 0x56, 0xC0, 0 },
    { 0x87, 0x56, 0xC0, 0 },
    { 0xC0, 0x56, 0xC0, 0 },
    { 0xFF, 0x56, 0xC0, 0 },
    { 0x00, 0x87, 0xC0, 0 },
    { 0x2C, 0x87, 0xC0, 0 },
    { 0x56, 0x87, 0xC0, 0 },
    { 0x87, 0x87, 0xC0, 0 },
    { 0xC0, 0x87, 0xC0, 0 },
    { 0xFF, 0x87, 0xC0, 0 },
    { 0x00, 0xC0, 0xC0, 0 },
    { 0x2C, 0xC0, 0xC0, 0 },
    { 0x56, 0xC0, 0xC0, 0 },
    { 0x87, 0xC0, 0xC0, 0 },
    { 0xFF, 0xC0, 0xC0, 0 },
    { 0x00, 0xFF, 0xC0, 0 },
    { 0x2C, 0xFF, 0xC0, 0 },
    { 0x56, 0xFF, 0xC0, 0 },
    { 0x87, 0xFF, 0xC0, 0 },
    { 0xC0, 0xFF, 0xC0, 0 },
    { 0xFF, 0xFF, 0xC0, 0 },
    { 0x00, 0x00, 0xFF, 0 },
    { 0x2C, 0x00, 0xFF, 0 },
    { 0x56, 0x00, 0xFF, 0 },
    { 0x87, 0x00, 0xFF, 0 },
    { 0xC0, 0x00, 0xFF, 0 },
    { 0xFF, 0x00, 0xFF, 0 },
    { 0x00, 0x2C, 0xFF, 0 },
    { 0x2C, 0x2C, 0xFF, 0 },
    { 0x56, 0x2C, 0xFF, 0 },
    { 0x87, 0x2C, 0xFF, 0 },
    { 0xC0, 0x2C, 0xFF, 0 },
    { 0xFF, 0x2C, 0xFF, 0 },
    { 0x00, 0x56, 0xFF, 0 },
    { 0x2C, 0x56, 0xFF, 0 },
    { 0x56, 0x56, 0xFF, 0 },
    { 0x87, 0x56, 0xFF, 0 },
    { 0xC0, 0x56, 0xFF, 0 },
    { 0xFF, 0x56, 0xFF, 0 },
    { 0x00, 0x87, 0xFF, 0 },
    { 0x2C, 0x87, 0xFF, 0 },
    { 0x56, 0x87, 0xFF, 0 },
    { 0x87, 0x87, 0xFF, 0 },
    { 0xC0, 0x87, 0xFF, 0 },
    { 0xFF, 0x87, 0xFF, 0 },
    { 0x00, 0xC0, 0xFF, 0 },
    { 0x2C, 0xC0, 0xFF, 0 },
    { 0x56, 0xC0, 0xFF, 0 },
    { 0x87, 0xC0, 0xFF, 0 },
    { 0xC0, 0xC0, 0xFF, 0 },
    { 0xFF, 0xC0, 0xFF, 0 },
    { 0x2C, 0xFF, 0xFF, 0 },
    { 0x56, 0xFF, 0xFF, 0 },
    { 0x87, 0xFF, 0xFF, 0 },
    { 0xC0, 0xFF, 0xFF, 0 },
    { 0xFF, 0xFF, 0xFF, 0 },
    { 0x11, 0x11, 0x11, 0 },
    { 0x18, 0x18, 0x18, 0 },
    { 0x1E, 0x1E, 0x1E, 0 },
    { 0x25, 0x25, 0x25, 0 },
    { 0x2C, 0x2C, 0x2C, 0 },
    { 0x34, 0x34, 0x34, 0 },
    { 0x3C, 0x3C, 0x3C, 0 },
    { 0x44, 0x44, 0x44, 0 },
    { 0x4D, 0x4D, 0x4D, 0 },
    { 0x56, 0x56, 0x56, 0 },
    { 0x5F, 0x5F, 0x5F, 0 },
    { 0x69, 0x69, 0x69, 0 },
    { 0x72, 0x72, 0x72, 0 },
    { 0x7D, 0x7D, 0x7D, 0 },
    { 0x92, 0x92, 0x92, 0 },
    { 0x9D, 0x9D, 0x9D, 0 },
    { 0xA8, 0xA8, 0xA8, 0 },
    { 0xB4, 0xB4, 0xB4, 0 },
    { 0xCC, 0xCC, 0xCC, 0 },
    { 0xD8, 0xD8, 0xD8, 0 },
    { 0xE5, 0xE5, 0xE5, 0 },
    { 0xF2, 0xF2, 0xF2, 0 },
    { 0xFF, 0xFF, 0xFF, 0 },

    { 0xFF, 0xFB, 0xF0, 0 },  // System palette - last 10 colors
    { 0xA0, 0xA0, 0xA4, 0 },
    { 0x80, 0x80, 0x80, 0 },
    { 0xFF, 0x00, 0x00, 0 },
    { 0x00, 0xFF, 0x00, 0 },
    { 0xFF, 0xFF, 0x00, 0 },
    { 0x00, 0x00, 0xFF, 0 },
    { 0xFF, 0x00, 0xFF, 0 },
    { 0x00, 0xFF, 0xFF, 0 },
    { 0xFF, 0xFF, 0xFF, 0 },
};

/////////////////////////////////////////////////////////////////////////////
// CE DIBSection global functions

#ifdef _WIN32_WCE
UINT CEGetDIBColorTable(HDC hdc, UINT uStartIndex, UINT cEntries, RGBQUAD *pColors);
#endif

/////////////////////////////////////////////////////////////////////////////
// CDIB static functions

//
// --- In  : nBitsPerPixel - bits per pixel
//           nCompression  - type of compression
// --- Out :
// --- Returns :The number of colors for this color depth
// --- Effect : Returns the number of color table entries given the number
//              of bits per pixel of a bitmap
/*static*/ int CDIB::NumColorEntries(int nBitsPerPixel, int nCompression)
{
    int nColors = 0;

    switch (nBitsPerPixel)
    {
	    case 1:
            nColors = 2;
            break;
#ifdef _WIN32_WCE
        case 2:
            nColors = 4;
            break;   // winCE only
#endif
        case 4:
            nColors = 16;
            break;
        case 8:
            nColors = 256;
            break;
        case 24:
            nColors = 0;
            break;
        case 16:
        case 32:
            if (nCompression == BI_BITFIELDS)
				nColors = 3; // 16 or 32 bpp have 3 colors(masks) in the color table if bitfield compression
			else
				nColors = 0; // 16 or 32 bpp have no color table if no bitfield compression
            break;

        default:
           ASSERT(FALSE);
    }

    return nColors;
}

//
// --- In  : nWidth - image width in pixels
//           nBitsPerPixel - bits per pixel
// --- Out :
// --- Returns : Returns the number of storage bytes needed for each scanline
//               in the bitmap
// --- Effect :
/*static*/ int CDIB::BytesPerLine(int nWidth, int nBitsPerPixel)
{
    return ( (nWidth * nBitsPerPixel + 31) & (~31) ) / 8;
}

#ifndef DIBSECTION_NO_PALETTE

// --- In  : palette - reference to a palette object which will be filled
//           nNumColors - number of color entries to fill
// --- Out :
// --- Returns : TRUE on success, false otherwise
// --- Effect : Creates a halftone color palette independant of screen color depth.
//              palette will be filled with the colors, and nNumColors is the No.
//              of colors to file. If nNumColorsis 0 or > 256, then 256 colors are used.
/*static*/
BOOL CDIB::CreateHalftonePalette(HPALETTE hPal, int nNumColors)
{
    ::DeleteObject(hPal);

    // Sanity check on requested number of colours.
    if (nNumColors <= 0 || nNumColors > 256)
        nNumColors = 256;
    else if (nNumColors <= 2)
        nNumColors = 2;
    else if (nNumColors <= 16)
        nNumColors = 16;
    else if  (nNumColors <= 256)
        nNumColors = 256;

    PALETTEINFO pi;
    pi.palNumEntries = (WORD) nNumColors;

    if (nNumColors == 2)
    {
        // According to the MS article "The Palette Manager: How and Why"
        // monochrome palettes not really needed (will use B&W)
        pi.palPalEntry[0].peRed   = ms_StdColors[0].rgbRed;
        pi.palPalEntry[0].peGreen = ms_StdColors[0].rgbGreen;
        pi.palPalEntry[0].peBlue  = ms_StdColors[0].rgbBlue;
        pi.palPalEntry[0].peFlags = 0;
        pi.palPalEntry[1].peRed   = ms_StdColors[255].rgbRed;
        pi.palPalEntry[1].peGreen = ms_StdColors[255].rgbGreen;
        pi.palPalEntry[1].peBlue  = ms_StdColors[255].rgbBlue;
        pi.palPalEntry[1].peFlags = 0;
   }
   else if (nNumColors == 16)
   {
        // According to the MS article "The Palette Manager: How and Why"
        // 4-bit palettes not really needed (will use VGA palette)
       for (int i = 0; i < 8; i++)
       {
           pi.palPalEntry[i].peRed   = ms_StdColors[i].rgbRed;
           pi.palPalEntry[i].peGreen = ms_StdColors[i].rgbGreen;
           pi.palPalEntry[i].peBlue  = ms_StdColors[i].rgbBlue;
           pi.palPalEntry[i].peFlags = 0;
       }
       for (i = 8; i < 16; i++)
       {
           pi.palPalEntry[i].peRed   = ms_StdColors[248+i].rgbRed;
           pi.palPalEntry[i].peGreen = ms_StdColors[248+i].rgbGreen;
           pi.palPalEntry[i].peBlue  = ms_StdColors[248+i].rgbBlue;
           pi.palPalEntry[i].peFlags = 0;
       }
   }
   else // if (nNumColors == 256)
   {
       // Fill palette with full halftone palette
       for (int i = 0; i < 256; i++)
       {
           pi.palPalEntry[i].peRed   = ms_StdColors[i].rgbRed;
           pi.palPalEntry[i].peGreen = ms_StdColors[i].rgbGreen;
           pi.palPalEntry[i].peBlue  = ms_StdColors[i].rgbBlue;
           pi.palPalEntry[i].peFlags = 0;
       }
   }

   hPal = ::CreatePalette((LPLOGPALETTE) pi);
   return (hPal != NULL);
}
#endif // DIBSECTION_NO_PALETTE


/////////////////////////////////////////////////////////////////////////////
// CDIB

CDIB::CDIB()
{
    // Just in case...
    ASSERT(sizeof(ms_StdColors) / sizeof(ms_StdColors[0]) == 256);

    m_hBitmap     = NULL;
    m_hOldBitmap  = NULL;

#ifndef DIBSECTION_NO_MEMDC_REUSE
    m_bReuseMemDC = FALSE;
#endif

#ifndef DIBSECTION_NO_PALETTE
    m_hOldPal = NULL;
#endif
}


CDIB::~CDIB()
{
    // Unselect the bitmap out of the memory DC before deleting bitmap
    ReleaseMemoryDC(TRUE);

    if (m_hBitmap)
        ::DeleteObject(m_hBitmap);
    m_hBitmap = NULL;
    m_ppvBits = NULL;

#ifndef DIBSECTION_NO_PALETTE
	DeleteObject((HGDIOBJ) m_hPal);
#endif

    memset(&m_DIBinfo, 0, sizeof(m_DIBinfo));

    m_iColorDataType = DIB_RGB_COLORS;
    m_iColorTableSize = 0;
}


/////////////////////////////////////////////////////////////////////////////
//
// CDIB operations
//
// --- In  : pDC - Pointer to a device context
//           ptDest - point at which the topleft corner of the image is drawn
// --- Out :
// --- Returns : TRUE on success
// --- Effect : Draws the image 1:1 on the device context
//
///////////////////////////////////////////////////////////////////////////////

BOOL CDIB::Draw(HDC hDC, POINT& ptDest, BOOL bForceBackground /*=FALSE*/)
{
    if (!m_hBitmap)
        return FALSE;

    SIZE size; // = GetSize();
    POINT ptOrigin = {0,0};
    BOOL bResult = FALSE;

    // Create a memory DC compatible with the destination DC
    HDC hMemDC = GetMemoryDC(hDC, FALSE);
    if (!hMemDC)
        return FALSE;

#ifndef DIBSECTION_NO_PALETTE
    // Select and realize the palette
    HPALETTE hOldPalette = NULL;
    if (m_hPal && UsesPalette(hDC))
    {
		hOldPalette = SelectPalette(hDC, m_hPal, bForceBackground );
		RealizePalette(hDC);
    }
#endif // DIBSECTION_NO_PALETTE

	bResult = BitBlt(hDC, ptDest.x, ptDest.y, size.cx, size.cy, hMemDC,
		ptOrigin.x, ptOrigin.y, SRCCOPY);

#ifndef DIBSECTION_NO_PALETTE
        if (hOldPalette)
            SelectPalette(hDC, hOldPalette, FALSE);
#endif // DIBSECTION_NO_PALETTE

    ReleaseMemoryDC();

    return bResult;
}


///////////////////////////////////////////////////////////////////////////////
//
// --- In  : pDC - Pointer to a device context
//           ptDest - point at which the topleft corner of the image is drawn
//           size - size to stretch the image
// --- Out :
// --- Returns : TRUE on success
// --- Effect : Stretch draws the image to the desired size on the device context
//
///////////////////////////////////////////////////////////////////////////////

BOOL CDIB::Stretch(HDC hDC, POINT& ptDest, SIZE& size, BOOL bForceBackground /*=FALSE*/)
{
    if (!m_hBitmap)
        return FALSE;

	BOOL bResult = FALSE;

    POINT ptOrigin = {0,0};
	SIZE imagesize;
	GetSize(imagesize);

#ifndef _WIN32_WCE
    //pDC->SetStretchBltMode(COLORONCOLOR);
#endif

    // Create a memory DC compatible with the destination DC
    HDC hMemDC = GetMemoryDC(hDC, FALSE);
    if (!hMemDC)
        return FALSE;

#ifndef DIBSECTION_NO_PALETTE
    // Select and realize the palette
    HPALETTE hOldPalette = NULL;
    if (m_hPal && UsesPalette(hDC))
    {
        hOldPalette = SelectPalette(hDC, m_hPal, bForceBackground);
        RealizePalette(hDC);
    }
#endif // DIBSECTION_NO_PALETTE

    bResult = StretchBlt(hDC, ptDest.x, ptDest.y, size.cx, size.cy,
		hMemDC, ptOrigin.x, ptOrigin.y, imagesize.cx, imagesize.cy, SRCCOPY);

#ifndef DIBSECTION_NO_PALETTE
        if (hOldPalette)
			SelectPalette(hDC, hOldPalette, FALSE);
#endif // DIBSECTION_NO_PALETTE

    ReleaseMemoryDC();

    return bResult;
}

//////////////////////////////////////////////////////////////////////////////
// Setting the bitmap...
//
// --- In  : nIDResource - resource ID
// --- Out :
// --- Returns : Returns TRUE on success, FALSE otherwise
// --- Effect : Initialises the bitmap from a resource. If failure, then object is
//              initialised back to an empty bitmap.
//
///////////////////////////////////////////////////////////////////////////////

BOOL CDIB::SetBitmap(UINT nIDResource, HINSTANCE hInst /*= NULL*/ )
{
    return SetBitmap(MAKEINTRESOURCE(nIDResource), hInst);
}


///////////////////////////////////////////////////////////////////////////////
//
// --- In  : lpszResourceName - resource name
// --- Out :
// --- Returns : Returns TRUE on success, FALSE otherwise
// --- Effect : Initialises the bitmap from a resource. If failure, then object is
//              initialised back to an empty bitmap.
//
///////////////////////////////////////////////////////////////////////////////

BOOL CDIB::SetBitmap(LPCTSTR lpszRes, HINSTANCE hInst /*= NULL*/ )
{
    HBITMAP hBmp = (HBITMAP)::LoadImage(hInst, lpszRes, IMAGE_BITMAP, 0,0,0);
	if (!hBmp)
	{
		TRACE0("Unable to LoadImage");
		return FALSE;
	}

	BOOL bResult = SetBitmap(hBmp);
	::DeleteObject(hBmp);
	return bResult;
}

///////////////////////////////////////////////////////////////////////////////
//
// --- In  : lpBitmapInfo - pointer to a BITMAPINFO structure
//           lpBits - pointer to image bits
// --- Out :
// --- Returns : Returns TRUE on success, FALSE otherwise
// --- Effect : Initialises the bitmap using the information in lpBitmapInfo to determine
//              the dimensions and colors, and the then sets the bits from the bits in
//              lpBits. If failure, then object is initialised back to an empty bitmap.
//
///////////////////////////////////////////////////////////////////////////////

BOOL CDIB::SetBitmap(LPBITMAPINFO lpBitmapInfo, LPVOID lpBits)
{
//	DeleteObject();

    if (!lpBitmapInfo || !lpBits)
        return FALSE;

    HDC hDC = NULL;
    BITMAPINFOHEADER& bmih = lpBitmapInfo->bmiHeader;

    // Compute the number of colors in the color table
    m_iColorTableSize = NumColorEntries(bmih.biBitCount, bmih.biCompression);

    DWORD dwBitmapInfoSize = sizeof(BITMAPINFO) + m_iColorTableSize*sizeof(RGBQUAD);

    // Copy over BITMAPINFO contents
    memcpy(&m_DIBinfo, lpBitmapInfo, dwBitmapInfoSize);

    // Should now have all the info we need to create the sucker.
    //TRACE(_T("Width %d, Height %d, Bits/pixel %d, Image Size %d\n"),
    //      bmih.biWidth, bmih.biHeight, bmih.biBitCount, bmih.biSizeImage);

    // Create a DC which will be used to get DIB, then create DIBsection
    hDC = ::GetDC(NULL);
    if (!hDC)
    {
        TRACE0("Unable to get DC\n");
		return FALSE;
    }

    m_hBitmap = CreateDIBSection(hDC, (const BITMAPINFO*) m_DIBinfo, m_iColorDataType, &m_ppvBits, NULL, 0);
    ::ReleaseDC(NULL, hDC);
    if (!m_hBitmap)
    {
        TRACE0("CreateDIBSection failed\n");
        return FALSE;
    }

    if (m_DIBinfo.bmiHeader.biSizeImage == 0)
    {
        int nBytesPerLine = BytesPerLine(lpBitmapInfo->bmiHeader.biWidth,
                                         lpBitmapInfo->bmiHeader.biBitCount);
        m_DIBinfo.bmiHeader.biSizeImage = nBytesPerLine * lpBitmapInfo->bmiHeader.biHeight;
    }

	memcpy(m_ppvBits, lpBits, m_DIBinfo.bmiHeader.biSizeImage);

#ifndef DIBSECTION_NO_PALETTE
    if (!CreatePalette())
    {
        TRACE0("Unable to create palette\n");
        return FALSE;
    }
#endif // DIBSECTION_NO_PALETTE

    return TRUE;
}


//////////////////////////////////////////////////////////////////////////////////
//
// --- In  : hBitmap - handle to image
//           pPalette - optional palette to use when setting image
// --- Out :
// --- Returns : Returns TRUE on success, FALSE otherwise
// --- Effect : Initialises the bitmap from the HBITMAP supplied. If failure, then
//              object is initialised back to an empty bitmap.
//
///////////////////////////////////////////////////////////////////////////////

BOOL CDIB::SetBitmap(HBITMAP hBitmap, HPALETTE hPal /*= NULL*/)
{
//    DeleteObject();

    if (!hBitmap)
        return FALSE;

    // Get dimensions of bitmap
    BITMAP bm;
    if (!::GetObject(hBitmap, sizeof(bm),(LPVOID)&bm))
        return FALSE;
    bm.bmHeight = abs(bm.bmHeight);

	HDC hDC = GetWindowDC(NULL);
    HPALETTE hOldPal = NULL;

    m_iColorTableSize = NumColorEntries(bm.bmBitsPixel, BI_RGB);

    // Initialize the BITMAPINFOHEADER in m_DIBinfo
    BITMAPINFOHEADER& bih = m_DIBinfo.bmiHeader;
    bih.biSize          = sizeof(BITMAPINFOHEADER);
    bih.biWidth         = bm.bmWidth;
    bih.biHeight        = bm.bmHeight;
    bih.biPlanes        = 1;                // Must always be 1 according to docs
    bih.biBitCount      = bm.bmBitsPixel;
    bih.biCompression   = BI_RGB;
    bih.biSizeImage     = BytesPerLine(bm.bmWidth, bm.bmBitsPixel) * bm.bmHeight;
    bih.biXPelsPerMeter = 0;
    bih.biYPelsPerMeter = 0;
    bih.biClrUsed       = 0;
    bih.biClrImportant  = 0;

    GetColorTableEntries(hDC, hBitmap);

    // If we have a palette supplied, then set the palette (and hance DIB color
    // table) using this palette
    if (hPal)
        SetPalette(hPal);

    if (hPal)
    {
        hOldPal = SelectPalette(hDC, m_hPal, FALSE);
        RealizePalette(hDC);
    }

    // Create it!
    m_hBitmap = CreateDIBSection(hDC, (const BITMAPINFO*) m_DIBinfo, m_iColorDataType, &m_ppvBits, NULL, 0);
    if (hOldPal)
        SelectPalette(hDC, hOldPal, FALSE);
    hOldPal = NULL;

    if (! m_hBitmap)
    {
        TRACE0("Unable to CreateDIBSection\n");
        return FALSE;
    }

    // If palette was supplied then create a palette using the entries in the DIB
    // color table.
    if (! hPal)
        CreatePalette();

    // Need to copy the supplied bitmap onto the newly created DIBsection
    HDC hMemDC = CreateCompatibleDC(hDC);
	HDC hCopyDC = CreateCompatibleDC(hDC);

    if (! hMemDC || ! hCopyDC)
    {
        TRACE0("Unable to create compatible DC's\n");
        //AfxThrowResourceException();
    }

    if (m_hPal)
    {
        SelectPalette(hMemDC, m_hPal, FALSE);   RealizePalette(hMemDC);
        SelectPalette(hCopyDC, m_hPal, FALSE);  RealizePalette(hCopyDC);
    }

    HBITMAP hOldMemBitmap  = (HBITMAP) SelectObject(hMemDC,  hBitmap);
    HBITMAP hOldCopyBitmap = (HBITMAP) SelectObject(hCopyDC, m_hBitmap);

    BitBlt(hCopyDC, 0, 0, bm.bmWidth, bm.bmHeight, hMemDC, 0, 0, SRCCOPY);

    SelectObject(hMemDC,  hOldMemBitmap);
    SelectObject(hCopyDC, hOldCopyBitmap);

    if (m_hPal)
    {
		HGDIOBJ hObj = ::GetStockObject(DEFAULT_PALETTE);
        SelectObject(hMemDC, hObj);
        SelectObject(hCopyDC, hObj);
    }

	ReleaseDC(NULL, hDC);
    return TRUE;
}


///////////////////////////////////////////////////////////////////////////////
//
// CDIB palette stuff
//
// --- In  : nNumColors - number of colors to set
//           pColors - array of RGBQUAD's containing colors to set
// --- Out :
// --- Returns : Returns TRUE on success, FALSE otherwise
// --- Effect : Sets the colors used by the image. Only works if # colors <= 256
//
///////////////////////////////////////////////////////////////////////////////

BOOL CDIB::SetColorTable(UINT nNumColors, RGBQUAD *pColors)
{
    if (!m_hBitmap ||!pColors || !nNumColors || m_iColorTableSize == 0 || nNumColors > 256)
        return FALSE;

    LPRGBQUAD pColorTable = GetColorTable();
    ASSERT(pColorTable);

    int nCount = min(m_iColorTableSize, nNumColors);
    ::memset(pColorTable, 0, m_iColorTableSize*sizeof(RGBQUAD));
    ::memcpy(pColorTable, pColors, nCount*sizeof(RGBQUAD));

    return TRUE;
}


// --- In  :
// --- Out :
// --- Returns : TRUE on success
// --- Effect : Creates the palette from the DIBSection's color table. Assumes
//              m_iColorTableSize has been set and the DIBsection m_hBitmap created
//
///////////////////////////////////////////////////////////////////////////////

BOOL CDIB::CreatePalette()
{
    //m_hPal DeleteObject();

    if (!m_hBitmap)
        return FALSE;

    // Create a 256 color halftone palette if there is no color table in the DIBSection
    if (m_iColorTableSize == 0)
        return CreateHalftonePalette(m_hPal, 256);

    // Get space for the color entries
    RGBQUAD *pRGB = new RGBQUAD[m_iColorTableSize];
    if (!pRGB)
        return CreateHalftonePalette(m_hPal, m_iColorTableSize);

    HDC hDC = ::GetDC(NULL);
    if (!hDC)
    {
        delete [] pRGB;
        return FALSE;
    }

    // Create a memory DC compatible with the current DC
    HDC hMemDC = CreateCompatibleDC(hDC);
    if (!hMemDC)
    {
        delete [] pRGB;
		ReleaseDC(NULL, hDC);
        return CreateHalftonePalette(m_hPal, m_iColorTableSize);
    }
    ReleaseDC(NULL, hDC);

    HBITMAP hOldBitmap = (HBITMAP) SelectObject(hMemDC, m_hBitmap);
    if (!hOldBitmap)
    {
        delete [] pRGB;
        return CreateHalftonePalette(m_hPal, m_iColorTableSize);
    }

    // Get the colors used. WinCE does not support GetDIBColorTable so if you
    // are using this on a CE device with palettes, then you need to replace
    // the call with code that manually gets the color table from the m_DIBinfo structure.
    int nColors = CEGetDIBColorTable(hDC, 0, m_iColorTableSize, pRGB);

    // Clean up
    SelectObject(hMemDC, hOldBitmap);

    if (!nColors)   // No colors retrieved => the bitmap in the DC is not a DIB section
    {
        delete [] pRGB;
        return CreateHalftonePalette(m_hPal, m_iColorTableSize);
    }

    // Create and fill a LOGPALETTE structure with the colors used.
    PALETTEINFO PaletteInfo;
    PaletteInfo.palNumEntries = m_iColorTableSize;

    for (int ii = 0; ii < nColors; ii++)
    {
        PaletteInfo.palPalEntry[ii].peRed   = pRGB[ii].rgbRed;
        PaletteInfo.palPalEntry[ii].peGreen = pRGB[ii].rgbGreen;
        PaletteInfo.palPalEntry[ii].peBlue  = pRGB[ii].rgbBlue;
        PaletteInfo.palPalEntry[ii].peFlags = 0;
    }

    delete [] pRGB;

    // Create Palette!
	m_hPal = ::CreatePalette( &PaletteInfo );
    return (NULL != m_hPal);
}

// --- In  : pPalette - new palette to use
// --- Out :
// --- Returns : TRUE on success
// --- Effect : Sets the current palette used by the image from the supplied CPalette,
//              and sets the color table in the DIBSection
//
///////////////////////////////////////////////////////////////////////////////

BOOL CDIB::SetPalette(HPALETTE hPal)
{
    // m_Palette.DeleteObject();

    if (!hPal)
        return FALSE;

	WORD nColors;
	::GetObject(hPal, sizeof(WORD), &nColors);

    if (nColors <= 0 || nColors > 256)
        return FALSE;

    // Get palette entries
    PALETTEINFO pi;
    pi.palNumEntries = (WORD) ::GetPaletteEntries(hPal, 0, nColors, (LPPALETTEENTRY) pi);
    return SetLogPalette(&pi);
}

// --- In  : pLogPalette - new palette to use
// --- Out :
// --- Returns : TRUE on success
// --- Effect : Sets the current palette used by the image from the supplied LOGPALETTE
//
///////////////////////////////////////////////////////////////////////////////

BOOL CDIB::SetLogPalette(LOGPALETTE* pLogPalette)
{
    if (!pLogPalette)
    {
        CreatePalette();
        return FALSE;
    }

    ASSERT(pLogPalette->palVersion == (WORD) 0x300);

    UINT nColors = pLogPalette->palNumEntries;
    if (nColors <= 0 || nColors > 256)
    {
        CreatePalette();
        return FALSE;
    }

    // Create new palette
	DeleteObject( m_hPal );
	m_hPal = ::CreatePalette(pLogPalette);
	if (!m_hPal)
    {
        CreatePalette();
        return FALSE;
    }

    if (m_iColorTableSize == 0)
        return TRUE;

    // Set the DIB colors
    RGBQUAD RGBquads[256];
    for (UINT i = 0; i < nColors; i++)
    {
        RGBquads[i].rgbRed   = pLogPalette->palPalEntry[i].peRed;
        RGBquads[i].rgbGreen = pLogPalette->palPalEntry[i].peGreen;
        RGBquads[i].rgbBlue  = pLogPalette->palPalEntry[i].peBlue;
        RGBquads[i].rgbReserved = 0;
    }

    return FillDIBColorTable(nColors, RGBquads);
}

// --- In  : nNumColors - number of colors to set
//           pRGB - colors to fill
// --- Out :
// --- Returns : Returns TRUE on success
// --- Effect : Sets the colors used by the image. Only works if # colors <= 256
//
///////////////////////////////////////////////////////////////////////////////

BOOL CDIB::FillDIBColorTable(UINT nNumColors, RGBQUAD *pRGB)
{
    if (!pRGB || !nNumColors || !m_iColorTableSize || nNumColors > 256)
        return FALSE;

    // get the number of colors to return per BITMAPINFOHEADER docs
    UINT nColors;
    LPBITMAPINFOHEADER pBmih = GetBitmapInfoHeader();
    if (pBmih->biClrUsed)
        nColors = pBmih->biClrUsed;
    else
        nColors = 1 << (pBmih->biBitCount*pBmih->biPlanes);

    // Initialize the loop variables
    nColors = min(nNumColors, nColors);

    LPRGBQUAD pColorTable = GetColorTable();
    for (UINT iColor = 0; iColor < nColors; iColor++)
    {
        pColorTable[iColor].rgbReserved = 0;
        pColorTable[iColor].rgbBlue     = pRGB[iColor].rgbBlue;
        pColorTable[iColor].rgbRed      = pRGB[iColor].rgbRed;
        pColorTable[iColor].rgbGreen    = pRGB[iColor].rgbGreen;
    }

    return TRUE;
}

//#endif // DIBSECTION_NO_PALETTE


// --- In  : hdc     - the Device Context in which the DIBSection is selected
//           hBitmap - the bitmap whose solor entries are to be queried
//           lpbi    - a pointer to a BITMAPINFO structure that will have it's
//                     color table filled.
// --- Out :
// --- Returns : the number of colors placed in the color table
// --- Effect : This function is a replacement for GetDIBits, in that it retrieves
//              (or synthesizes) the color table from the given bitmap, and stores
//              the values in the BITMAPINFO structure supplied.
//
///////////////////////////////////////////////////////////////////////////////

UINT CDIB::GetColorTableEntries(HDC hdc, HBITMAP hBitmap)
{
    if (!m_iColorTableSize)
        return 0;

    // Fill the color table with the colors from the bitmap's color table
    LPRGBQUAD pColorTable = GetColorTable();

    // Get the color table from the HBITMAP and copy them over.
    UINT nCount;
    RGBQUAD* pRGB = new RGBQUAD[m_iColorTableSize];
    if (pRGB)
    {
        HBITMAP hOldBitmap = (HBITMAP) SelectObject(hdc, hBitmap);
        nCount = CEGetDIBColorTable(hdc, 0, m_iColorTableSize, pRGB);
        SelectObject(hdc, hOldBitmap);
        if (nCount)
        {
            // m_iColorTableSize = nCount;
            memcpy(pColorTable, pRGB, nCount*sizeof(RGBQUAD));
        }
    }
    delete [] pRGB;

    // Didn't work - so synthesize one.
    if (!nCount)
    {
        nCount = min( m_iColorTableSize, sizeof(ms_StdColors) / sizeof(ms_StdColors[0]) );
        memcpy(pColorTable, ms_StdColors, nCount*sizeof(RGBQUAD));
    }

    return nCount;
}


///////////////////////////////////////////////////////////////////////////////
//
// This function is from the MS KB article "HOWTO: Get the Color Table of
//  DIBSection in Windows CE".
//
// PARAMETERS:
// HDC - the Device Context in which the DIBSection is selected
/// UINT - the index of the first color table entry to retrieve
// UINT - the number of color table entries to retrieve
// RGBQUAD - a buffer large enough to hold the number of RGBQUAD
// entries requested
//
// RETURNS:
// UINT - the number of colors placed in the buffer
//
//
///////////////////////////////////////////////////////////////////////////////

UINT CEGetDIBColorTable(HDC hdc, UINT uStartIndex, UINT cEntries, RGBQUAD *pColors)
{
    if (pColors == NULL)
        return 0;                       // No place to put them, fail

    // Get a description of the DIB Section
    HBITMAP hDIBSection = (HBITMAP) GetCurrentObject( hdc, OBJ_BITMAP );

    DIBSECTION ds;
    DWORD dwSize = GetObject( hDIBSection, sizeof(DIBSECTION), &ds );

    if (dwSize != sizeof(DIBSECTION))
        return 0;                      // Must not be a DIBSection, fail

    if (ds.dsBmih.biBitCount > 8)
        return 0;                      // Not Palettized, fail

    // get the number of colors to return per BITMAPINFOHEADER docs
    UINT cColors;
    if (ds.dsBmih.biClrUsed)
        cColors = ds.dsBmih.biClrUsed;
    else
        cColors = 1 << (ds.dsBmih.biBitCount * ds.dsBmih.biPlanes);

    // Create a mask for the palette index bits for 1, 2, 4, and 8 bpp
    WORD wIndexMask = (0xFF << (8 - ds.dsBmih.biBitCount)) & 0x00FF;

    // Get the pointer to the image bits
    LPBYTE pBits = (LPBYTE) ds.dsBm.bmBits;

    // Initialize the loop variables
    cColors = min( cColors, cEntries );
    BYTE OldPalIndex = *pBits;

    UINT TestPixelY;
    if (ds.dsBmih.biHeight > 0 )
        // If button up DIB, pBits points to last row
        TestPixelY = ds.dsBm.bmHeight-1;
    else
        // If top down DIB, pBits points to first row
        TestPixelY = 0;

    for (UINT iColor = uStartIndex; iColor < cColors; iColor++)
    {
        COLORREF    rgbColor;

        // Set the palette index for the test pixel,
        // modifying only the bits for one pixel
        *pBits = (iColor << (8 - ds.dsBmih.biBitCount)) | (*pBits & ~wIndexMask);

        // now get the resulting color
        rgbColor = GetPixel( hdc, 0, TestPixelY );

        pColors[iColor - uStartIndex].rgbReserved = 0;
        pColors[iColor - uStartIndex].rgbBlue = GetBValue(rgbColor);
        pColors[iColor - uStartIndex].rgbRed = GetRValue(rgbColor);
        pColors[iColor - uStartIndex].rgbGreen = GetGValue(rgbColor);
    }

    // Restore the test pixel
    *pBits = OldPalIndex;

    return cColors;
}


///////////////////////////////////////////////////////////////////////////////
//
// --- In  : pDC - device context to use when calling CreateCompatibleDC
//           bSelectPalette - if TRUE, the current palette will be preselected
// --- Out :
// --- Returns : A pointer to a memory DC
// --- Effect : Creates a memory DC and selects in the current bitmap so it can be
//              modified using the GDI functions. Only one memDC can be created for
//              a given CDIB object. If you have a memDC but wish to recreate it
//              as compatible with a different DC, then call ReleaseMemoryDC first.
//              If the memory DC has already been created then it will be recycled.
//              Note that if using this in an environment where the color depth of
//              the screen may change, then you will need to set "m_bReuseMemDC" to FALSE
//
///////////////////////////////////////////////////////////////////////////////

HDC CDIB::GetMemoryDC(HDC hDC /*=NULL*/, BOOL bSelectPalette /*=TRUE*/)
{
#ifdef DIBSECTION_NO_MEMDC_REUSE
    ReleaseMemoryDC(TRUE);
#else
    if (!m_bReuseMemDC)
	{
        ReleaseMemoryDC(TRUE);
	}
    else if (m_hMemDC)   // Already created?
    {
        return m_hMemDC;
    }
#endif // DIBSECTION_NO_MEMDC_REUSE

    // Create a memory DC compatible with the given DC
	m_hMemDC = CreateCompatibleDC(hDC);
    if (!m_hMemDC)
        return NULL;

    // Select in the bitmap
    m_hOldBitmap = (HBITMAP) ::SelectObject(m_hMemDC, m_hBitmap);

    // Select in the palette
    if (bSelectPalette && UsesPalette(m_hMemDC))
    {
        // Palette should already have been created - but just in case...
        if (!m_hPal)
            CreatePalette();

        m_hOldPal = SelectPalette( m_hMemDC, m_hPal, FALSE );
        RealizePalette( m_hMemDC );
    }
    else
        m_hOldPal = NULL;

    return m_hMemDC;
}


///////////////////////////////////////////////////////////////////////////////
//
// --- In  : bForceRelease - if TRUE, then the memory DC is forcibly released
// --- Out :
// --- Returns : TRUE on success
// --- Effect : Selects out the current bitmap and deletes the mem dc. If bForceRelease
//              is FALSE, then the DC release will not actually occur. This is provided
//              so you can have
//
//                 GetMemoryDC(...)
//                 ... do something
//                 ReleaseMemoryDC()
//
//               bracketed calls. If m_bReuseMemDC is subsequently set to FALSE, then
//               the same code fragment will still work.
//
///////////////////////////////////////////////////////////////////////////////

BOOL CDIB::ReleaseMemoryDC(BOOL bForceRelease /*=FALSE*/)
{
    if ( !m_hMemDC
#ifndef DIBSECTION_NO_MEMDC_REUSE
        || (m_bReuseMemDC && !bForceRelease)
#endif // DIBSECTION_NO_MEMDC_REUSE
        )
        return TRUE; // Nothing to do

    // Select out the current bitmap
    if (m_hOldBitmap)
        ::SelectObject(m_hMemDC, m_hOldBitmap);

    m_hOldBitmap = NULL;

#ifndef DIBSECTION_NO_PALETTE
    // Select out the current palette
    if (m_hOldPal)
        SelectPalette(m_hMemDC, m_hOldPal, FALSE);

    m_hOldPal = NULL;
#endif // DIBSECTION_NO_PALETTE

    // Delete the memory DC
    return DeleteDC(m_hMemDC);
}

