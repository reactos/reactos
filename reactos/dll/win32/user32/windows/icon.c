/*
 *  ReactOS kernel
 *  Copyright (C) 1998, 1999, 2000, 2001 ReactOS Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/* $Id$
 *
 * PROJECT:         ReactOS user32.dll
 * FILE:            lib/user32/windows/icon.c
 * PURPOSE:         Icon
 * PROGRAMMER:      Casper S. Hornstrup (chorns@users.sourceforge.net)
 * UPDATE HISTORY:
 *      09-05-2001  CSH  Created
 */

/* INCLUDES ******************************************************************/

#include <user32.h>

#include <wine/debug.h>
WINE_DEFAULT_DEBUG_CHANNEL(user32);

/* FUNCTIONS *****************************************************************/

HICON
ICON_CreateIconFromData(HDC hDC, PVOID ImageData, ICONIMAGE* IconImage, int cxDesired, int cyDesired, int xHotspot, int yHotspot)
{
   BYTE BitmapInfoBuffer[sizeof(BITMAPINFOHEADER) + 2 * sizeof(RGBQUAD)];
   BITMAPINFO *bwBIH = (BITMAPINFO *)BitmapInfoBuffer;
   ICONINFO IconInfo;

   IconInfo.fIcon = TRUE;
   IconInfo.xHotspot = xHotspot;
   IconInfo.yHotspot = yHotspot;

   /* Load the XOR bitmap */
   IconInfo.hbmColor = CreateDIBitmap(hDC, &IconImage->icHeader, CBM_INIT,
                                      ImageData, (BITMAPINFO*)IconImage,
                                      DIB_RGB_COLORS);

   /* Make ImageData point to the start of the AND image data. */
   ImageData = ((PBYTE)ImageData) + (((IconImage->icHeader.biWidth *
                                      IconImage->icHeader.biBitCount + 31) & ~31) >> 3) *
                                      (IconImage->icHeader.biHeight );

   /* Create a BITMAPINFO header for the monocrome part of the icon. */
   bwBIH->bmiHeader.biBitCount = 1;
   bwBIH->bmiHeader.biWidth = IconImage->icHeader.biWidth;
   bwBIH->bmiHeader.biHeight = IconImage->icHeader.biHeight;
   bwBIH->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
   bwBIH->bmiHeader.biPlanes = 1;
   bwBIH->bmiHeader.biSizeImage = 0;
   bwBIH->bmiHeader.biCompression = BI_RGB;
   bwBIH->bmiHeader.biClrImportant = 0;
   bwBIH->bmiHeader.biClrUsed = 0;
   bwBIH->bmiHeader.biXPelsPerMeter = 0;
   bwBIH->bmiHeader.biYPelsPerMeter = 0;

   bwBIH->bmiColors[0].rgbBlue = 0;
   bwBIH->bmiColors[0].rgbGreen = 0;
   bwBIH->bmiColors[0].rgbRed = 0;
   bwBIH->bmiColors[0].rgbReserved = 0;

   bwBIH->bmiColors[1].rgbBlue = 0xff;
   bwBIH->bmiColors[1].rgbGreen = 0xff;
   bwBIH->bmiColors[1].rgbRed = 0xff;
   bwBIH->bmiColors[1].rgbReserved = 0;

   /* Load the AND bitmap. */
   IconInfo.hbmMask = CreateDIBitmap(hDC, &bwBIH->bmiHeader, 0,
                                     ImageData, bwBIH, DIB_RGB_COLORS);

   SetDIBits(hDC, IconInfo.hbmMask, 0, IconImage->icHeader.biHeight,
             ImageData, bwBIH, DIB_RGB_COLORS);

   /* Create the icon based on everything we have so far */
   return NtUserCreateCursorIconHandle(&IconInfo, FALSE);
}

HICON
ICON_CreateCursorFromData(HDC hDC, PVOID ImageData, ICONIMAGE* IconImage, int cxDesired, int cyDesired, int xHotspot, int yHotspot)
{
   /* FIXME - color cursors */
   BYTE BitmapInfoBuffer[sizeof(BITMAPINFOHEADER) + 2 * sizeof(RGBQUAD)];
   BITMAPINFO *bwBIH = (BITMAPINFO *)BitmapInfoBuffer;
   ICONINFO IconInfo;
   PVOID XORImageData = ImageData;

   IconInfo.fIcon = FALSE;
   IconInfo.xHotspot = xHotspot;
   IconInfo.yHotspot = yHotspot;

   /* Create a BITMAPINFO header for the monocrome part of the icon */
   bwBIH->bmiHeader.biBitCount = 1;
   bwBIH->bmiHeader.biWidth = IconImage->icHeader.biWidth;
   bwBIH->bmiHeader.biHeight = IconImage->icHeader.biHeight;
   bwBIH->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
   bwBIH->bmiHeader.biPlanes = 1;
   bwBIH->bmiHeader.biSizeImage = 0;
   bwBIH->bmiHeader.biCompression = BI_RGB;
   bwBIH->bmiHeader.biClrImportant = 0;
   bwBIH->bmiHeader.biClrUsed = 0;
   bwBIH->bmiHeader.biXPelsPerMeter = 0;
   bwBIH->bmiHeader.biYPelsPerMeter = 0;

   bwBIH->bmiColors[0].rgbBlue = 0;
   bwBIH->bmiColors[0].rgbGreen = 0;
   bwBIH->bmiColors[0].rgbRed = 0;
   bwBIH->bmiColors[0].rgbReserved = 0;

   bwBIH->bmiColors[1].rgbBlue = 0xff;
   bwBIH->bmiColors[1].rgbGreen = 0xff;
   bwBIH->bmiColors[1].rgbRed = 0xff;
   bwBIH->bmiColors[1].rgbReserved = 0;

   /* Load the AND bitmap */
   IconInfo.hbmMask = CreateDIBitmap(hDC, &bwBIH->bmiHeader, 0,
                                     XORImageData, bwBIH, DIB_RGB_COLORS);
   if (IconInfo.hbmMask)
   {
      SetDIBits(hDC, IconInfo.hbmMask, 0, IconImage->icHeader.biHeight,
                XORImageData, bwBIH, DIB_RGB_COLORS);
   }

   IconInfo.hbmColor = (HBITMAP)0;

   /* Create the icon based on everything we have so far */
   return NtUserCreateCursorIconHandle(&IconInfo, FALSE);
}


/*
 * @implemented
 */
HICON
STDCALL
CopyIcon(
  HICON hIcon)
{
  ICONINFO IconInfo;

  if(GetIconInfo((HANDLE)hIcon, &IconInfo))
  {
    return NtUserCreateCursorIconHandle(&IconInfo, FALSE);
  }
  return (HICON)0;
}


/*
 * @implemented
 */
HICON
STDCALL
CreateIcon(
  HINSTANCE hInstance,
  int nWidth,
  int nHeight,
  BYTE cPlanes,
  BYTE cBitsPixel,
  CONST BYTE *ANDbits,
  CONST BYTE *XORbits)
{
  ICONINFO IconInfo;

  IconInfo.fIcon = TRUE;
  IconInfo.xHotspot = nWidth / 2;
  IconInfo.yHotspot = nHeight / 2;
  IconInfo.hbmMask = CreateBitmap(nWidth, nHeight, 1, 1, ANDbits);
  if(!IconInfo.hbmMask)
  {
    return (HICON)0;
  }
  IconInfo.hbmColor = CreateBitmap(nWidth, nHeight, cPlanes, cBitsPixel, XORbits);
  if(!IconInfo.hbmColor)
  {
    DeleteObject(IconInfo.hbmMask);
    return (HICON)0;
  }

  return NtUserCreateCursorIconHandle(&IconInfo, FALSE);
}


/*
 * @implemented
 */
HICON
STDCALL
CreateIconFromResource(
  PBYTE presbits,
  DWORD dwResSize,
  BOOL fIcon,
  DWORD dwVer)
{
  return CreateIconFromResourceEx(presbits, dwResSize, fIcon, dwVer, 0, 0, LR_DEFAULTSIZE|LR_SHARED );
}


/*
 * @implemented
 */
HICON
STDCALL
CreateIconFromResourceEx(
  PBYTE pbIconBits,
  DWORD cbIconBits,
  BOOL fIcon,
  DWORD dwVersion,
  int cxDesired,
  int cyDesired,
  UINT uFlags)
{
  ICONIMAGE* SafeIconImage;
  HICON hIcon;
  ULONG HeaderSize;
  ULONG ColourCount;
  PVOID Data;
  HDC hScreenDc;
  WORD wXHotspot;
  WORD wYHotspot;

  /*
    FIXME - does win support LR_SHARED? According to msdn it does but we don't
            have useful information to identify the icon
  if (uFlags & LR_SHARED)
  {
    DbgPrint("FIXME: need LR_SHARED support in CreateIconFromResourceEx()\n");
  }
  */

  TRACE("dwVersion, cxDesired, cyDesired are all ignored in this implementation!\n");

  if (! fIcon)
    {
      wXHotspot = *(WORD*)pbIconBits;
      pbIconBits += sizeof(WORD);
      wYHotspot = *(WORD*)pbIconBits;
      pbIconBits += sizeof(WORD);
      cbIconBits -= 2 * sizeof(WORD);
    }
  else
    {
      wXHotspot = cxDesired / 2;
      wYHotspot = cyDesired / 2;
    }

  /* get an safe copy of the icon data */
  SafeIconImage = RtlAllocateHeap(GetProcessHeap(), 0, cbIconBits);
  if (SafeIconImage == NULL)
    {
      return NULL;
    }
  memcpy(SafeIconImage, pbIconBits, cbIconBits);

  /* take into acount the original height was for both the AND and XOR images */
  if(fIcon)
    SafeIconImage->icHeader.biHeight /= 2;

  if (SafeIconImage->icHeader.biSize == sizeof(BITMAPCOREHEADER))
    {
      BITMAPCOREHEADER* Core = (BITMAPCOREHEADER*)SafeIconImage;
      ColourCount = (Core->bcBitCount <= 8) ? (1 << Core->bcBitCount) : 0;
      HeaderSize = sizeof(BITMAPCOREHEADER) + ColourCount * sizeof(RGBTRIPLE);
    }
  else
    {
      ColourCount = (SafeIconImage->icHeader.biBitCount <= 8) ?
                       (1 << SafeIconImage->icHeader.biBitCount) : 0;
      HeaderSize = sizeof(BITMAPINFOHEADER) + ColourCount * sizeof(RGBQUAD);
    }

  /* make data point to the start of the XOR image data */
  Data = (PBYTE)SafeIconImage + HeaderSize;

  /* get a handle to the screen dc, the icon we create is going to be compatable with this */
  hScreenDc = CreateCompatibleDC(NULL);
  if (hScreenDc == NULL)
    {
      RtlFreeHeap(GetProcessHeap(), 0, SafeIconImage);
      return(NULL);
    }

  if(fIcon)
    hIcon = ICON_CreateIconFromData(hScreenDc, Data, SafeIconImage, cxDesired, cyDesired, wXHotspot, wYHotspot);
  else
    hIcon = ICON_CreateCursorFromData(hScreenDc, Data, SafeIconImage, cxDesired, cyDesired, wXHotspot, wYHotspot);
  RtlFreeHeap(GetProcessHeap(), 0, SafeIconImage);
  DeleteDC(hScreenDc);

  return hIcon;
}


/*
 * @implemented
 */
HICON
STDCALL
CreateIconIndirect(PICONINFO IconInfo)
{
  BITMAP ColorBitmap;
  BITMAP MaskBitmap;

  if(!IconInfo)
  {
    SetLastError(ERROR_INVALID_PARAMETER);
    return (HICON)0;
  }

  if(!GetObjectW(IconInfo->hbmMask, sizeof(BITMAP), &MaskBitmap))
  {
    return (HICON)0;
  }
  /* FIXME - does there really *have* to be a color bitmap? monochrome cursors don't have one */
  if(/*IconInfo->hbmColor &&*/ !GetObjectW(IconInfo->hbmColor, sizeof(BITMAP), &ColorBitmap))
  {
    return (HICON)0;
  }

  /* FIXME - i doubt this is right (monochrome cursors */
  /*if(ColorBitmap.bmWidth != MaskBitmap.bmWidth ||
     ColorBitmap.bmHeight != MaskBitmap.bmWidth)
  {
    SetLastError(ERROR_INVALID_PARAMETER);
    return (HICON)0;
  }*/

  return (HICON)NtUserCreateCursorIconHandle(IconInfo, TRUE);
}


/*
 * @implemented
 */
BOOL
STDCALL
DestroyIcon(
  HICON hIcon)
{
  return (BOOL)NtUserDestroyCursor((HANDLE)hIcon, 0);
}


/*
 * @implemented
 */
BOOL
STDCALL
DrawIcon(
  HDC hDC,
  int X,
  int Y,
  HICON hIcon)
{
  return DrawIconEx(hDC, X, Y, hIcon, 0, 0, 0, NULL, DI_NORMAL | DI_DEFAULTSIZE);
}

/*
 * @implemented
 */
BOOL
STDCALL
DrawIconEx(
  HDC hdc,
  int xLeft,
  int yTop,
  HICON hIcon,
  int cxWidth,
  int cyWidth,
  UINT istepIfAniCur,
  HBRUSH hbrFlickerFreeDraw,
  UINT diFlags)
{
  return (BOOL)NtUserDrawIconEx(hdc, xLeft, yTop, hIcon, cxWidth, cyWidth,
                                   istepIfAniCur, hbrFlickerFreeDraw, diFlags,
                                   0, 0);
}


/*
 * @implemented
 */
BOOL
STDCALL
GetIconInfo(
  HICON hIcon,
  PICONINFO IconInfo)
{
  return NtUserGetIconInfo((HANDLE)hIcon, IconInfo, 0, 0, 0, 0);
}


/*
 * @implemented
 */
HICON
STDCALL
LoadIconA(
  HINSTANCE hInstance,
  LPCSTR lpIconName)
{
  return(LoadImageA(hInstance, lpIconName, IMAGE_ICON, 0, 0, LR_SHARED | LR_DEFAULTSIZE));
}


/*
 * @implemented
 */
HICON
STDCALL
LoadIconW(
  HINSTANCE hInstance,
  LPCWSTR lpIconName)
{
  return(LoadImageW(hInstance, lpIconName, IMAGE_ICON, 0, 0, LR_SHARED | LR_DEFAULTSIZE));
}


/*
 * @implemented
 */
int
STDCALL
LookupIconIdFromDirectory(
  PBYTE presbits,
  BOOL fIcon)
{
    return LookupIconIdFromDirectoryEx(presbits,
                                       fIcon,
                                       fIcon ? GetSystemMetrics(SM_CXICON) : GetSystemMetrics(SM_CXCURSOR),
                                       fIcon ? GetSystemMetrics(SM_CYICON) : GetSystemMetrics(SM_CYCURSOR),
                                       LR_DEFAULTCOLOR);
}





/*
 *  The following macro function accounts for the irregularities of
 *   accessing cursor and icon resources in files and resource entries.
 */
typedef BOOL
(*fnGetCIEntry)(LPVOID dir, int n, int *width, int *height, int *bits );

/**********************************************************************
 *	    CURSORICON_FindBestIcon
 *
 * Find the icon closest to the requested size and number of colors.
 */
static int
CURSORICON_FindBestIcon(LPVOID dir,
                        fnGetCIEntry get_entry,
                        int Width,
                        int Height,
                        int ColorBits)
{
    int i, cx, cy, Bits, BestBits = 0, BestEntry = -1;
    UINT iTotalDiff, iXDiff=0, iYDiff=0, iColorDiff;
    UINT iTempXDiff, iTempYDiff, iTempColorDiff;

    /* Find Best Fit */
    iTotalDiff = 0xFFFFFFFF;
    iColorDiff = 0xFFFFFFFF;
    for (i = 0; get_entry(dir, i, &cx, &cy, &Bits); i++ )
    {
        iTempXDiff = abs(Width - cx);
        iTempYDiff = abs(Height - cy);

        if(iTotalDiff > (iTempXDiff + iTempYDiff))
        {
            iXDiff = iTempXDiff;
            iYDiff = iTempYDiff;
            iTotalDiff = iXDiff + iYDiff;
        }
    }

    /* Find Best Colors for Best Fit */
    for (i = 0; get_entry(dir, i, &cx, &cy, &Bits); i++ )
    {
        if(abs(Width - cx) == iXDiff && abs(Height - cy) == iYDiff)
        {
            iTempColorDiff = abs(ColorBits - Bits);
            if(iColorDiff > iTempColorDiff)
            {
                BestEntry = i;
                BestBits = Bits;
                iColorDiff = iTempColorDiff;
            }
        }
    }

    TRACE("Best Icon: ResId: %d, bits : %d\n", BestEntry, BestBits);

    return BestEntry;
}



/**********************************************************************
 *	    CURSORICON_FindBestCursor
 *
 * Find the cursor closest to the requested size.
 * FIXME: parameter 'color' ignored and entries with more than 1 bpp
 *        ignored too
 */
static int
CURSORICON_FindBestCursor(LPVOID dir,
                          fnGetCIEntry get_entry,
                          int Width,
                          int Height,
                          int ColorBits)
{
    int i, cx, cy, Bits, BestBits = 0, BestEntry = -1;
    UINT iTotalDiff, iXDiff=0, iYDiff=0, iColorDiff;
    UINT iTempXDiff, iTempYDiff, iTempColorDiff;

    /* Find Best Fit */
    iTotalDiff = 0xFFFFFFFF;
    iColorDiff = 0xFFFFFFFF;
    for (i = 0; get_entry(dir, i, &cx, &cy, &Bits); i++ )
    {
        iTempXDiff = abs(Width - cx);
        iTempYDiff = abs(Height - cy);

        if(iTotalDiff > (iTempXDiff + iTempYDiff))
        {
            iXDiff = iTempXDiff;
            iYDiff = iTempYDiff;
            iTotalDiff = iXDiff + iYDiff;
        }
    }

    /* Find Best Colors for Best Fit */
    for (i = 0; get_entry(dir, i, &cx, &cy, &Bits); i++ )
    {
        if(abs(Width - cx) == iXDiff && abs(Height - cy) == iYDiff)
        {
            iTempColorDiff = abs(ColorBits - Bits);
            if(iColorDiff > iTempColorDiff)
            {
                BestEntry = i;
                BestBits = Bits;
                iColorDiff = iTempColorDiff;
            }
        }
    }

    TRACE("Best Cursor: ResId: %d, bits : %d\n", BestEntry, BestBits);

    return BestEntry;
}


static BOOL
CURSORICON_GetResIconEntry(LPVOID dir,
                           int n,
                           int *Width,
                           int *Height,
                           int *Bits)
{
    GRPCURSORICONDIR *ResDir = dir;
    ICONRESDIR *Icon;

    if (ResDir->idCount <= n)
        return FALSE;

    Icon = &ResDir->idEntries[n].ResInfo.icon;
    *Width = Icon->bWidth;
    *Height = Icon->bHeight;
    *Bits = ResDir->idEntries[n].wBitCount;
    return TRUE;
}

static BOOL
CURSORICON_GetResCursorEntry(LPVOID dir,
                             int n,
                             int *Width,
                             int *Height,
                             int *Bits)
{
    GRPCURSORICONDIR *ResDir = dir;
    CURSORRESDIR *Cursor;

    if (ResDir->idCount <= n)
        return FALSE;

    Cursor = &ResDir->idEntries[n].ResInfo.cursor;
    *Width = Cursor->wWidth;
    *Height = Cursor->wHeight;
    *Bits = ResDir->idEntries[n].wBitCount;
    return TRUE;
}

static GRPCURSORICONDIRENTRY *
CURSORICON_FindBestIconRes(GRPCURSORICONDIR * dir,
                           int Width,
                           int Height,
                           int ColorBits)
{
    int n;
    n = CURSORICON_FindBestIcon(dir,
                                CURSORICON_GetResIconEntry,
                                Width,
                                Height,
                                ColorBits);
    if (n < 0)
        return NULL;

    return &dir->idEntries[n];
}

static GRPCURSORICONDIRENTRY *
CURSORICON_FindBestCursorRes(GRPCURSORICONDIR *dir,
                             int Width,
                             int Height,
                             int ColorBits)
{
    int n;
    n = CURSORICON_FindBestCursor(dir,
                                  CURSORICON_GetResCursorEntry,
                                  Width,
                                  Height,
                                  ColorBits);
    if (n < 0)
        return NULL;

    return &dir->idEntries[n];
}


INT WINAPI
LookupIconIdFromDirectoryEx(PBYTE xdir,
                            BOOL bIcon,
                            INT width,
                            INT height,
                            UINT cFlag)
{
    GRPCURSORICONDIR *dir = (GRPCURSORICONDIR*)xdir;
    UINT retVal = 0;
    if(dir && !dir->idReserved && (IMAGE_ICON == dir->idType || IMAGE_CURSOR == dir->idType))
    {
        GRPCURSORICONDIRENTRY *entry = NULL;
        int ColorBits;

        if (cFlag & LR_MONOCHROME)
        {
            ColorBits = 1;
        }
        else
        {
            HDC hdc = CreateICW(NULL, NULL, NULL, NULL);
            ColorBits = GetDeviceCaps(hdc, BITSPIXEL);
            DeleteDC(hdc);
        }

        if(bIcon)
            entry = CURSORICON_FindBestIconRes(dir, width, height, ColorBits);
        else
            entry = CURSORICON_FindBestCursorRes(dir, width, height, 1);

        if (entry)
            retVal = entry->nID;
    }
    else
        WARN("%s() : Invalid resource directory\n", __FUNCTION__);

    return retVal;
}
