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
/* $Id: icon.c,v 1.5 2003/06/03 22:25:37 ekohl Exp $
 *
 * PROJECT:         ReactOS user32.dll
 * FILE:            lib/user32/windows/icon.c
 * PURPOSE:         Icon
 * PROGRAMMER:      Casper S. Hornstrup (chorns@users.sourceforge.net)
 * UPDATE HISTORY:
 *      09-05-2001  CSH  Created
 */

/* INCLUDES ******************************************************************/

#include <windows.h>
#include <user32.h>
#include <string.h>
#include <stdlib.h>
#include <debug.h>

/* FUNCTIONS *****************************************************************/

HICON
ICON_CreateIconFromData(HDC hDC, PVOID ImageData, ICONIMAGE* IconImage, int cxDesired, int cyDesired)
{
  HANDLE hXORBitmap;
  HANDLE hANDBitmap;
  BITMAPINFO* bwBIH;
  ICONINFO IconInfo;
  HICON hIcon;

  //load the XOR bitmap
  hXORBitmap = CreateDIBitmap(hDC, &IconImage->icHeader, CBM_INIT,
			       ImageData, (BITMAPINFO*)IconImage, DIB_RGB_COLORS);

  //make ImageData point to the start of the AND image data
  ImageData = ((PBYTE)ImageData) + (((IconImage->icHeader.biWidth * 
                                      IconImage->icHeader.biBitCount + 31) & ~31) >> 3) * 
                                      (IconImage->icHeader.biHeight );

  //create a BITMAPINFO header for the monocrome part of the icon
  bwBIH = RtlAllocateHeap(RtlGetProcessHeap(), 0, sizeof (BITMAPINFOHEADER)+2*sizeof(RGBQUAD));

  bwBIH->bmiHeader.biBitCount = 1;
  bwBIH->bmiHeader.biWidth = IconImage->icHeader.biWidth;
  bwBIH->bmiHeader.biHeight = IconImage->icHeader.biHeight;
  bwBIH->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
  bwBIH->bmiHeader.biPlanes = 1;
  bwBIH->bmiHeader.biSizeImage = (((IconImage->icHeader.biWidth * 1 + 31) & ~31) >> 3) * 
                                    (IconImage->icHeader.biHeight );
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

  //load the AND bitmap
  hANDBitmap = CreateDIBitmap(hDC, &bwBIH->bmiHeader, CBM_INIT,
			       ImageData, bwBIH, DIB_RGB_COLORS);

  RtlFreeHeap(RtlGetProcessHeap(), 0, bwBIH);

  IconInfo.fIcon = TRUE;
  IconInfo.xHotspot = cxDesired/2;
  IconInfo.yHotspot = cyDesired/2;
  IconInfo.hbmColor = hXORBitmap;
  IconInfo.hbmMask = hANDBitmap;

  //Create the icon based on everything we have so far
  hIcon = CreateIconIndirect(&IconInfo);

  //clean up
  DeleteObject(hXORBitmap);
  DeleteObject(hANDBitmap);

  return hIcon;
}

HICON
STDCALL
CopyIcon(
  HICON hIcon)
{
  ICONINFO IconInfo;
  GetIconInfo(hIcon, &IconInfo);
  return CreateIconIndirect(&IconInfo);
}

HICON
STDCALL
CreateIcon(
  HINSTANCE hInstance,
  int nWidth,
  int nHeight,
  BYTE cPlanes,
  BYTE cBitsPixel,
  CONST BYTE *lpbANDbits,
  CONST BYTE *lpbXORbits)
{
  DPRINT("hInstance not used in this implementation\n");
  return W32kCreateIcon(TRUE,
                        nWidth,
                        nHeight,
                        cPlanes,
                        cBitsPixel,
                        nWidth/2,
                        nHeight/2,
                        lpbANDbits,
                        lpbXORbits);
}

HICON
STDCALL
CreateIconFromResource(
  PBYTE presbits,
  DWORD dwResSize,
  WINBOOL fIcon,
  DWORD dwVer)
{
  return CreateIconFromResourceEx( presbits, dwResSize, fIcon, dwVer, 0,0,0);
}

HICON
STDCALL
CreateIconFromResourceEx(
  PBYTE pbIconBits,
  DWORD cbIconBits,
  WINBOOL fIcon,
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

  DPRINT("fIcon, dwVersion, cxDesired, cyDesired are all ignored in this implementation!\n");

  //get an safe copy of the icon data
  SafeIconImage = RtlAllocateHeap(RtlGetProcessHeap(), 0, cbIconBits);
  memcpy(SafeIconImage, pbIconBits, cbIconBits);

  //take into acount the origonal hight was for both the AND and XOR images
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

  //make data point to the start of the XOR image data
  Data = (PBYTE)SafeIconImage + HeaderSize;

  //get a handle to the screen dc, the icon we create is going to be compatable with this
  hScreenDc = CreateDCW(L"DISPLAY", NULL, NULL, NULL);
  if (hScreenDc == NULL)
  {
     RtlFreeHeap(RtlGetProcessHeap(), 0, SafeIconImage);
     return(NULL);
  }

  hIcon = ICON_CreateIconFromData(hScreenDc, Data, SafeIconImage, cxDesired, cyDesired);
  RtlFreeHeap(RtlGetProcessHeap(), 0, SafeIconImage);
  return hIcon;
}

HICON
STDCALL
CreateIconIndirect(
  PICONINFO piconinfo)
{
  BITMAP bmMask;
  BITMAP bmColor;

  W32kGetObject( piconinfo->hbmMask, sizeof(BITMAP), &bmMask );
  W32kGetObject( piconinfo->hbmColor, sizeof(BITMAP), &bmColor );

  return W32kCreateIcon(piconinfo->fIcon,
			bmColor.bmWidth,
			bmColor.bmHeight,
			bmColor.bmPlanes,
			bmColor.bmBitsPixel,
			piconinfo->xHotspot,
			piconinfo->yHotspot,
			bmMask.bmBits,
			bmColor.bmBits);
}

WINBOOL
STDCALL
DestroyIcon(
  HICON hIcon)
{
  return W32kDeleteObject(hIcon);
}

WINBOOL
STDCALL
DrawIcon(
  HDC hDC,
  int X,
  int Y,
  HICON hIcon)
{
  return DrawIconEx (hDC, X, Y, hIcon, 0, 0, 0, NULL, DI_NORMAL | DI_COMPAT | DI_DEFAULTSIZE);
}

/* Ported from WINE20030408 */
WINBOOL
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
    ICONINFO IconInfo;
    BITMAP XORBitmap;
    HDC hDC_off = 0, hMemDC;
    BOOL result = FALSE, DoOffscreen;
    HBITMAP hB_off = 0, hOld = 0;

    if (!GetIconInfo(hIcon, &IconInfo))
      return FALSE;

    W32kGetObject(IconInfo.hbmColor, sizeof(BITMAP), &XORBitmap);

    DPRINT("(hdc=%p,pos=%d.%d,hicon=%p,extend=%d.%d,istep=%d,br=%p,flags=0x%08x)\n",
                 hdc,xLeft,yTop,hIcon,cxWidth,cyWidth,istepIfAniCur,hbrFlickerFreeDraw,diFlags );

    hMemDC = CreateCompatibleDC (hdc);
    if (diFlags & DI_COMPAT)
        DPRINT("Ignoring flag DI_COMPAT\n");

    if (!diFlags)
    {
	  diFlags = DI_NORMAL;
    }

    // Calculate the size of the destination image.
    if (cxWidth == 0)
    {
      if (diFlags & DI_DEFAULTSIZE)
	     cxWidth = GetSystemMetrics (SM_CXICON);
      else
	     cxWidth = XORBitmap.bmWidth;
    }
    if (cyWidth == 0)
    {
      if (diFlags & DI_DEFAULTSIZE)
        cyWidth = GetSystemMetrics (SM_CYICON);
      else
	    cyWidth = XORBitmap.bmHeight;
    }

    DoOffscreen = (GetObjectType( hbrFlickerFreeDraw ) == OBJ_BRUSH);

    if (DoOffscreen)
    {
      RECT r;

      r.left = 0;
      r.top = 0;
      r.right = cxWidth;
      r.bottom = cxWidth;

      DbgPrint("in DrawIconEx calling: CreateCompatibleDC\n");
      hDC_off = CreateCompatibleDC(hdc);

      DbgPrint("in DrawIconEx calling: CreateCompatibleBitmap\n");
      hB_off = CreateCompatibleBitmap(hdc, cxWidth, cyWidth);
      if (hDC_off && hB_off)
      {
        DbgPrint("in DrawIconEx calling: SelectObject\n");
        hOld = SelectObject(hDC_off, hB_off);

        DbgPrint("in DrawIconEx calling: FillRect\n");
        FillRect(hDC_off, &r, hbrFlickerFreeDraw);
      }
    }

    if (hMemDC && (!DoOffscreen || (hDC_off && hB_off)))
    {
      COLORREF  oldFg, oldBg;
      INT     nStretchMode;

      nStretchMode = SetStretchBltMode (hdc, STRETCH_DELETESCANS);

      oldFg = SetTextColor( hdc, RGB(0,0,0) );

      oldBg = SetBkColor( hdc, RGB(255,255,255) );

      if (IconInfo.hbmColor && IconInfo.hbmMask)
      {
        HBITMAP hBitTemp = SelectObject( hMemDC, IconInfo.hbmMask );
        if (diFlags & DI_MASK)
        {
          if (DoOffscreen)
            StretchBlt (hDC_off, 0, 0, cxWidth, cyWidth,
               hMemDC, 0, 0, XORBitmap.bmWidth, XORBitmap.bmHeight, SRCAND);
          else
            StretchBlt (hdc, xLeft, yTop, cxWidth, cyWidth,
                 hMemDC, 0, 0, XORBitmap.bmWidth, XORBitmap.bmHeight, SRCAND);
        }
        SelectObject( hMemDC, IconInfo.hbmColor );
        if (diFlags & DI_IMAGE)
        {
          if (DoOffscreen)
            StretchBlt (hDC_off, 0, 0, cxWidth, cyWidth,
               hMemDC, 0, 0, XORBitmap.bmWidth, XORBitmap.bmHeight, SRCPAINT);
          else
            StretchBlt (hdc, xLeft, yTop, cxWidth, cyWidth,
              hMemDC, 0, 0, XORBitmap.bmWidth, XORBitmap.bmHeight, SRCPAINT);
        }
        SelectObject( hMemDC, hBitTemp );
        result = TRUE;
      }

      SetTextColor( hdc, oldFg );
      SetBkColor( hdc, oldBg );
      if (IconInfo.hbmColor)
        DeleteObject( IconInfo.hbmColor );

      if (IconInfo.hbmMask) 
        DeleteObject( IconInfo.hbmMask );

      SetStretchBltMode (hdc, nStretchMode);

      if (DoOffscreen)
      {
        BitBlt(hdc, xLeft, yTop, cxWidth, cyWidth, hDC_off, 0, 0, SRCCOPY);
        SelectObject(hDC_off, hOld);
      }
    }

    if (hMemDC)
      DeleteDC( hMemDC );
    if (hDC_off)
      DeleteDC(hDC_off);
    if (hB_off)
      DeleteObject(hB_off);
    return result;
}

WINBOOL
STDCALL
GetIconInfo(
  HICON hIcon,
  PICONINFO piconinfo)
{
  return NtUserGetIconInfo(hIcon,
                           &piconinfo->fIcon,
                           &piconinfo->xHotspot,
                           &piconinfo->yHotspot,
                           &piconinfo->hbmMask,
                           &piconinfo->hbmColor);
}

HICON
STDCALL
LoadIconA(
  HINSTANCE hInstance,
  LPCSTR lpIconName)
{
  return(LoadImageA(hInstance, lpIconName, IMAGE_ICON, 0, 0, LR_SHARED | LR_DEFAULTSIZE));
}

HICON
STDCALL
LoadIconW(
  HINSTANCE hInstance,
  LPCWSTR lpIconName)
{
  return(LoadImageW(hInstance, lpIconName, IMAGE_ICON, 0, 0, LR_SHARED | LR_DEFAULTSIZE));
}

int
STDCALL
LookupIconIdFromDirectory(
  PBYTE presbits,
  WINBOOL fIcon)
{
    return LookupIconIdFromDirectoryEx( presbits, fIcon,
	   fIcon ? GetSystemMetrics(SM_CXICON) : GetSystemMetrics(SM_CXCURSOR),
	   fIcon ? GetSystemMetrics(SM_CYICON) : GetSystemMetrics(SM_CYCURSOR), LR_DEFAULTCOLOR );
}

/* Ported from WINE20030408 */
CURSORICONDIRENTRY*
CURSORICON_FindBestCursor( CURSORICONDIR *dir, int width, int height, int colors)
{
    int i;
    CURSORICONDIRENTRY *entry, *bestEntry = NULL;
    UINT iTotalDiff, iXDiff=0, iYDiff=0, iColorDiff;
    UINT iTempXDiff, iTempYDiff, iTempColorDiff;

    if (dir->idCount < 1)
    {
        DPRINT("Empty directory!\n");
        return NULL;
    }
    if (dir->idCount == 1) 
      return &dir->idEntries[0];  /* No choice... */

    /* Find Best Fit */
    iTotalDiff = 0xFFFFFFFF;
    iColorDiff = 0xFFFFFFFF;
    for (i = 0, entry = &dir->idEntries[0]; i < dir->idCount; i++,entry++)
    {
		iTempXDiff = abs(width - entry->Info.icon.bWidth);
		iTempYDiff = abs(height - entry->Info.icon.bHeight);

        if(iTotalDiff > (iTempXDiff + iTempYDiff))
        {
            iXDiff = iTempXDiff;
            iYDiff = iTempYDiff;
            iTotalDiff = iXDiff + iYDiff;
        }
    }

    /* Find Best Colors for Best Fit */
    for (i = 0, entry = &dir->idEntries[0]; i < dir->idCount; i++,entry++)
    {
        if(abs(width - entry->Info.icon.bWidth) == iXDiff &&
            abs(height - entry->Info.icon.bHeight) == iYDiff)
        {
            iTempColorDiff = abs(colors - entry->Info.icon.bColorCount);

            if(iColorDiff > iTempColorDiff)
        	{
            	bestEntry = entry;
                iColorDiff = iTempColorDiff;
        	}
        }
    }

    return bestEntry;
}

/* Ported from WINE20030408 */
CURSORICONDIRENTRY*
CURSORICON_FindBestIcon( CURSORICONDIR *dir, int width, int height, int colors)
{
    int i;
    CURSORICONDIRENTRY *entry, *bestEntry = NULL;
    UINT iTotalDiff, iXDiff=0, iYDiff=0, iColorDiff;
    UINT iTempXDiff, iTempYDiff, iTempColorDiff;

    if (dir->idCount < 1)
    {
        DPRINT("Empty directory!\n");
        return NULL;
    }
    if (dir->idCount == 1)
      return &dir->idEntries[0];  /* No choice... */

    /* Find Best Fit */
    iTotalDiff = 0xFFFFFFFF;
    iColorDiff = 0xFFFFFFFF;
    for (i = 0, entry = &dir->idEntries[0]; i < dir->idCount; i++,entry++)
      {
	iTempXDiff = abs(width - entry->Info.icon.bWidth);

	iTempYDiff = abs(height - entry->Info.icon.bHeight);

        if(iTotalDiff > (iTempXDiff + iTempYDiff))
        {
            iXDiff = iTempXDiff;
            iYDiff = iTempYDiff;
            iTotalDiff = iXDiff + iYDiff;
        }
      }

    /* Find Best Colors for Best Fit */
    for (i = 0, entry = &dir->idEntries[0]; i < dir->idCount; i++,entry++)
      {
        if(abs(width - entry->Info.icon.bWidth) == iXDiff &&
           abs(height - entry->Info.icon.bHeight) == iYDiff)
        {
            iTempColorDiff = abs(colors - entry->Info.icon.bColorCount);
            if(iColorDiff > iTempColorDiff)
            {
                bestEntry = entry;
                iColorDiff = iTempColorDiff;
            }
        }
    }

    return bestEntry;
}

/* Ported from WINE20030408 */
int
STDCALL
LookupIconIdFromDirectoryEx(
  PBYTE presbits,
  WINBOOL fIcon,
  int cxDesired,
  int cyDesired,
  UINT Flags)
{
    GRPICONDIR *dir = (GRPICONDIR*)presbits;
    UINT retVal = 0;

    if( dir && !dir->idReserved && (dir->idType & 3) )
    {
	GRPICONDIRENTRY* entry;
	HDC hdc;
	UINT palEnts;
	int colors;
	hdc = GetDC(0);
#if 0
	palEnts = GetSystemPaletteEntries(hdc, 0, 0, NULL);
	if (palEnts == 0)
	palEnts = 256;
#endif
	palEnts = 16;  //use this until GetSystemPaletteEntries works
	colors = (Flags & LR_MONOCHROME) ? 2 : palEnts;

	ReleaseDC(0, hdc);

	entry = (GRPICONDIRENTRY*)CURSORICON_FindBestIcon( (CURSORICONDIR*)dir,
	                                                   cxDesired,
	                                                   cyDesired,
	                                                   colors );

	if( entry )
	    retVal = entry->nID;
    }
    else
    {
       DbgPrint("invalid resource directory\n");
    }
    return retVal;
}
