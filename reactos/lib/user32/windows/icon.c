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
/* $Id: icon.c,v 1.12 2003/10/06 16:25:53 gvg Exp $
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

STATIC WINBOOL FASTCALL
ICON_CopyBitmaps(HBITMAP *MaskTo, HBITMAP MaskFrom,
                 HBITMAP *ColorTo, HBITMAP *ColorFrom,
                 DWORD Width, DWORD Height)
{
  HDC DCFrom, DCTo;
  HBITMAP InitialFrom, InitialTo;

  DCFrom = CreateCompatibleDC(NULL);
  if (NULL == DCFrom)
    {
      return FALSE;
    }
  DCTo = CreateCompatibleDC(DCFrom);
  if (NULL == DCTo)
    {
      DeleteDC(DCFrom);
      return FALSE;
    }

  *MaskTo = CreateCompatibleBitmap(DCTo, Width, Height);
  if (NULL == *MaskTo)
    {
      DeleteDC(DCTo);
      DeleteDC(DCFrom);
      return FALSE;
    }

  InitialFrom = SelectObject(DCFrom, MaskFrom);
  if (NULL == InitialFrom)
    {
      DeleteObject(*MaskTo);
      *MaskTo = NULL;
      DeleteDC(DCTo);
      DeleteDC(DCFrom);
      return FALSE;
    }
      
  InitialTo = SelectObject(DCTo, *MaskTo);
  if (NULL == InitialTo)
    {
      DeleteObject(*MaskTo);
      *MaskTo = NULL;
      DeleteDC(DCTo);
      SelectObject(DCFrom, InitialFrom);
      DeleteDC(DCFrom);
      return FALSE;
    }

  if (! BitBlt(DCTo, 0, 0, Width, Height, DCFrom, 0, 0, SRCCOPY))
    {
      SelectObject(DCTo, InitialTo);
      DeleteObject(*MaskTo);
      *MaskTo = NULL;
      DeleteDC(DCTo);
      SelectObject(DCFrom, InitialFrom);
      DeleteDC(DCFrom);
      return FALSE;
    }

  *ColorTo = CreateCompatibleBitmap(DCTo, Width, Height);
  if (NULL == *ColorTo)
    {
      SelectObject(DCTo, InitialTo);
      DeleteObject(*MaskTo);
      *MaskTo = NULL;
      DeleteDC(DCTo);
      SelectObject(DCFrom, InitialFrom);
      DeleteDC(DCFrom);
      return FALSE;
    }

  if (NULL == SelectObject(DCFrom, ColorFrom))
    {
      DeleteObject(*ColorTo);
      *ColorTo = NULL;
      SelectObject(DCTo, InitialTo);
      DeleteObject(*MaskTo);
      *MaskTo = NULL;
      DeleteDC(DCTo);
      SelectObject(DCFrom, InitialFrom);
      DeleteDC(DCFrom);
      return FALSE;
    }

  if (NULL == SelectObject(DCTo, *ColorTo))
    {
      DeleteObject(*ColorTo);
      *ColorTo = NULL;
      SelectObject(DCTo, InitialTo);
      DeleteObject(*MaskTo);
      *MaskTo = NULL;
      DeleteDC(DCTo);
      SelectObject(DCFrom, InitialFrom);
      DeleteDC(DCFrom);
      return FALSE;
    }


  if (! BitBlt(DCTo, 0, 0, Width, Height, DCFrom, 0, 0, SRCCOPY))
    {
      SelectObject(DCTo, InitialTo);
      DeleteObject(*ColorTo);
      *ColorTo = NULL;
      DeleteObject(*MaskTo);
      *MaskTo = NULL;
      DeleteDC(DCTo);
      SelectObject(DCFrom, InitialFrom);
      DeleteDC(DCFrom);
      return FALSE;
    }

  SelectObject(DCTo, InitialTo);
  DeleteDC(DCTo);
  SelectObject(DCFrom, InitialFrom);
  DeleteDC(DCFrom);

  return TRUE;
}

STATIC
HICON 
FASTCALL 
ICON_CreateIconIndirect(PICONINFO IconInfo, WINBOOL CopyBitmaps,
                        DWORD Width, DWORD Height)
{
  PICONINFO NewIcon;

  if (NULL == IconInfo)
    {
      DPRINT("Invalid parameter passed\n");
      SetLastError(ERROR_INVALID_PARAMETER);
      return NULL;
    }

  NewIcon = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(ICONINFO));
  if (NULL == NewIcon)
    {
      SetLastError(ERROR_NOT_ENOUGH_MEMORY);
      return NULL;
    }

  /* Set up the basic icon stuff */
  NewIcon->fIcon = IconInfo->fIcon;
  NewIcon->xHotspot = IconInfo->xHotspot;
  NewIcon->yHotspot = IconInfo->yHotspot;

  if (CopyBitmaps)
    {
      /* Store a copy the bitmaps */
      if (! ICON_CopyBitmaps(&(NewIcon->hbmMask), IconInfo->hbmMask,
                             &(NewIcon->hbmColor), IconInfo->hbmColor,
                             Width, Height))
        {
          HeapFree(GetProcessHeap(), 0, NewIcon);
          return NULL;
        }
    }
  else
    {
      /* We take ownership of the bitmaps */
      NewIcon->hbmMask = IconInfo->hbmMask;
      NewIcon->hbmColor = IconInfo->hbmColor;
    }

  return (HICON) NewIcon;
}

HICON
ICON_CreateIconFromData(HDC hDC, PVOID ImageData, ICONIMAGE* IconImage, int cxDesired, int cyDesired, int xHotspot, int yHotspot)
{
  BITMAPINFO* bwBIH;
  ICONINFO IconInfo;

  IconInfo.fIcon = TRUE;
  IconInfo.xHotspot = xHotspot;
  IconInfo.yHotspot = yHotspot;

  /* Load the XOR bitmap */
  IconInfo.hbmColor = CreateDIBitmap(hDC, &IconImage->icHeader, CBM_INIT,
			             ImageData, (BITMAPINFO*)IconImage, DIB_RGB_COLORS);

  /* make ImageData point to the start of the AND image data */
  ImageData = ((PBYTE)ImageData) + (((IconImage->icHeader.biWidth * 
                                      IconImage->icHeader.biBitCount + 31) & ~31) >> 3) * 
                                      (IconImage->icHeader.biHeight );

  /* create a BITMAPINFO header for the monocrome part of the icon */
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

  /* load the AND bitmap */
  IconInfo.hbmMask = CreateDIBitmap(hDC, &bwBIH->bmiHeader, CBM_INIT,
                                    ImageData, bwBIH, DIB_RGB_COLORS);

  RtlFreeHeap(RtlGetProcessHeap(), 0, bwBIH);

  /* Create the icon based on everything we have so far */
  return ICON_CreateIconIndirect(&IconInfo, FALSE, IconImage->icHeader.biWidth,
                                 IconImage->icHeader.biHeight);
}


/*
 * @implemented
 */
HICON
STDCALL
CopyIcon(
  HICON hIcon)
{
  PICONINFO IconInfo = (PICONINFO) hIcon;
  BITMAP BitmapInfo;

  if (NULL == IconInfo)
    {
      SetLastError(ERROR_INVALID_HANDLE);
      return NULL;
    }

  if (0 == GetObjectW(IconInfo->hbmColor, sizeof(BITMAP), &BitmapInfo))
    {
      return NULL;
    }

  return ICON_CreateIconIndirect(IconInfo, TRUE, BitmapInfo.bmWidth, BitmapInfo.bmHeight);
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
  IconInfo.hbmMask = CreateBitmap(nWidth, nHeight, cPlanes, cBitsPixel, ANDbits);
  if (NULL == IconInfo.hbmMask)
    {
      return NULL;
    }
  IconInfo.hbmColor = CreateBitmap(nWidth, nHeight, cPlanes, cBitsPixel, XORbits);
  if (NULL == IconInfo.hbmColor)
    {
      DeleteObject(IconInfo.hbmMask);
      return NULL;
    }

  return ICON_CreateIconIndirect(&IconInfo, FALSE, nWidth, nHeight);
}


/*
 * @implemented
 */
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


/*
 * @implemented
 */
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
  WORD wXHotspot;
  WORD wYHotspot;

  DPRINT("dwVersion, cxDesired, cyDesired are all ignored in this implementation!\n");

  if (! fIcon)
    {
      wXHotspot = (WORD)*pbIconBits;
      pbIconBits+=2;
      wYHotspot = (WORD)*pbIconBits;
      pbIconBits+=2;
      cbIconBits-=4;
    }
  else
    {
      wXHotspot = cxDesired / 2;
      wYHotspot = cyDesired / 2;
    }

  /* get an safe copy of the icon data */
  SafeIconImage = RtlAllocateHeap(RtlGetProcessHeap(), 0, cbIconBits);
  memcpy(SafeIconImage, pbIconBits, cbIconBits);

  /* take into acount the origonal hight was for both the AND and XOR images */
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
      RtlFreeHeap(RtlGetProcessHeap(), 0, SafeIconImage);
      return(NULL);
    }

  hIcon = ICON_CreateIconFromData(hScreenDc, Data, SafeIconImage, cxDesired, cyDesired, wXHotspot, wYHotspot);
  RtlFreeHeap(RtlGetProcessHeap(), 0, SafeIconImage);

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

  if (NULL == IconInfo)
    {
      SetLastError(ERROR_INVALID_PARAMETER);
      return NULL;
    }

  if (0 == GetObjectW(IconInfo->hbmColor, sizeof(BITMAP), &ColorBitmap))
    {
      return NULL;
    }
  if (0 == GetObjectW(IconInfo->hbmMask, sizeof(BITMAP), &MaskBitmap))
    {
      return NULL;
    }
  if (ColorBitmap.bmWidth != MaskBitmap.bmWidth ||
      ColorBitmap.bmHeight != MaskBitmap.bmWidth)
    {
      SetLastError(ERROR_INVALID_PARAMETER);
      return NULL;
    }

  return ICON_CreateIconIndirect(IconInfo, TRUE, ColorBitmap.bmWidth, ColorBitmap.bmHeight);
}


/*
 * @implemented
 */
WINBOOL
STDCALL
DestroyIcon(
  HICON hIcon)
{
  PICONINFO IconInfo = (PICONINFO) hIcon;

  if (NULL == IconInfo)
    {
      SetLastError(ERROR_INVALID_HANDLE);
      return FALSE;
    }

  if (NULL != IconInfo->hbmMask)
    {
      DeleteObject(IconInfo->hbmMask);
    }
  if (NULL != IconInfo->hbmColor)
    {
      DeleteObject(IconInfo->hbmColor);
    }

  HeapFree(GetProcessHeap(), 0, IconInfo);

  return TRUE;
}


/*
 * @implemented
 */
WINBOOL
STDCALL
DrawIcon(
  HDC hDC,
  int X,
  int Y,
  HICON hIcon)
{
  return DrawIconEx(hDC, X, Y, hIcon, 0, 0, 0, NULL, DI_NORMAL | DI_COMPAT | DI_DEFAULTSIZE);
}

/* Ported from WINE20030408 */
/*
 * @implemented
 */
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
  PICONINFO IconInfo = (PICONINFO) hIcon;
  BITMAP Bitmap;
  HDC hDC_off = 0, hMemDC;
  BOOL result = FALSE, DoOffscreen;
  HBITMAP hB_off = 0, hOld = 0;

  if (NULL == IconInfo)
    {
      SetLastError(ERROR_INVALID_HANDLE);
      return FALSE;
    }

  GetObjectW(IconInfo->hbmColor, sizeof(BITMAP), &Bitmap);

  DPRINT("(hdc=%p,pos=%d.%d,hicon=%p,extend=%d.%d,istep=%d,br=%p,flags=0x%08x)\n",
               hdc,xLeft,yTop,hIcon,cxWidth,cyWidth,istepIfAniCur,hbrFlickerFreeDraw,diFlags );

  hMemDC = CreateCompatibleDC(hdc);
  if (diFlags & DI_COMPAT)
    {
      DPRINT("Ignoring flag DI_COMPAT\n");
    }

  if (!diFlags)
    {
      diFlags = DI_NORMAL;
    }

  /* Calculate the size of the destination image. */
  if (cxWidth == 0)
    {
      cxWidth = (diFlags & DI_DEFAULTSIZE ? GetSystemMetrics (SM_CXICON)
                                          : Bitmap.bmWidth);
    }
  if (cyWidth == 0)
    {
      cyWidth = (diFlags & DI_DEFAULTSIZE ? GetSystemMetrics (SM_CYICON)
                                          : Bitmap.bmHeight);
    }

  DoOffscreen = (NULL != hbrFlickerFreeDraw
                 && OBJ_BRUSH == GetObjectType(hbrFlickerFreeDraw));

  if (DoOffscreen)
    {
      RECT r;

      r.left = 0;
      r.top = 0;
      r.right = cxWidth;
      r.bottom = cxWidth;

      DPRINT("in DrawIconEx calling: CreateCompatibleDC\n");
      hDC_off = CreateCompatibleDC(hdc);

      DPRINT("in DrawIconEx calling: CreateCompatibleBitmap\n");
      hB_off = CreateCompatibleBitmap(hdc, cxWidth, cyWidth);
      if (hDC_off && hB_off)
        {
          DPRINT("in DrawIconEx calling: SelectObject\n");
          hOld = SelectObject(hDC_off, hB_off);

          DPRINT("in DrawIconEx calling: FillRect\n");
          FillRect(hDC_off, &r, hbrFlickerFreeDraw);
        }
    }

  if (hMemDC && (! DoOffscreen || (hDC_off && hB_off)))
    {
      COLORREF oldFg, oldBg;
      INT nStretchMode;

      nStretchMode = SetStretchBltMode(hdc, STRETCH_DELETESCANS);
      oldFg = SetTextColor(hdc, RGB(0, 0, 0));
      oldBg = SetBkColor(hdc, RGB(255, 255, 255));

      if (IconInfo->hbmColor && IconInfo->hbmMask)
        {
          HBITMAP hBitTemp = SelectObject(hMemDC, IconInfo->hbmMask);
          if (diFlags & DI_MASK)
            {
              if (DoOffscreen)
                {
                  StretchBlt (hDC_off, 0, 0, cxWidth, cyWidth,
                              hMemDC, 0, 0, Bitmap.bmWidth, Bitmap.bmHeight,
                              SRCAND);
                }
              else
                {
                  StretchBlt (hdc, xLeft, yTop, cxWidth, cyWidth,
                              hMemDC, 0, 0, Bitmap.bmWidth, Bitmap.bmHeight,
                              SRCAND);
                }
            }
          SelectObject(hMemDC, IconInfo->hbmColor);
          if (diFlags & DI_IMAGE)
            {
              if (DoOffscreen)
                {
                  StretchBlt (hDC_off, 0, 0, cxWidth, cyWidth,
                              hMemDC, 0, 0, Bitmap.bmWidth, Bitmap.bmHeight,
                              SRCINVERT);
                }
              else
                {
                  StretchBlt (hdc, xLeft, yTop, cxWidth, cyWidth,
                              hMemDC, 0, 0, Bitmap.bmWidth, Bitmap.bmHeight,
                              SRCINVERT);
                }
            }
          SelectObject(hMemDC, hBitTemp);
          result = TRUE;
        }

      SetTextColor( hdc, oldFg );
      SetBkColor( hdc, oldBg );
      SetStretchBltMode (hdc, nStretchMode);

      if (DoOffscreen)
        {
          BitBlt(hdc, xLeft, yTop, cxWidth, cyWidth, hDC_off, 0, 0, SRCCOPY);
          SelectObject(hDC_off, hOld);
        }
    }

  if (hMemDC)
    {
      DeleteDC( hMemDC );
    }
  if (hDC_off)
    {
      DeleteDC(hDC_off);
    }
  if (hB_off)
    {
      DeleteObject(hB_off);
    }

  return result;
}


/*
 * @implemented
 */
WINBOOL
STDCALL
GetIconInfo(
  HICON hIcon,
  PICONINFO IconInfo)
{
  PICONINFO IconData = (PICONINFO) hIcon;
  BITMAP BitmapInfo;

  if (NULL == IconData)
    {
      SetLastError(ERROR_INVALID_HANDLE);
      return FALSE;
    }
  if (NULL == IconInfo)
    {
      SetLastError(ERROR_INVALID_PARAMETER);
      return FALSE;
    }

  /* Copy basic info */      
  IconInfo->fIcon = IconData->fIcon;
  IconInfo->xHotspot = IconData->xHotspot;
  IconInfo->yHotspot = IconData->yHotspot;

  /* Copy the bitmaps */
  if (0 == GetObjectW(IconData->hbmColor, sizeof(BITMAP), &BitmapInfo))
    {
      return FALSE;
    }
  if (! ICON_CopyBitmaps(&(IconInfo->hbmMask), IconData->hbmMask,
                         &(IconInfo->hbmColor), IconData->hbmColor,
                         BitmapInfo.bmWidth, BitmapInfo.bmHeight))
    {
      return FALSE;
    }

  return TRUE;
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
		iTempXDiff = abs(width - entry->bWidth);
		iTempYDiff = abs(height - entry->bHeight);

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
        if(abs(width - entry->bWidth) == (int) iXDiff &&
            abs(height - entry->bHeight) == (int) iYDiff)
        {
            iTempColorDiff = abs(colors - entry->bColorCount);

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
	iTempXDiff = abs(width - entry->bWidth);

	iTempYDiff = abs(height - entry->bHeight);

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
        if(abs(width - entry->bWidth) == (int) iXDiff &&
           abs(height - entry->bHeight) == (int) iYDiff)
        {
            iTempColorDiff = abs(colors - entry->bColorCount);
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
/*
 * @implemented
 */
int
STDCALL
LookupIconIdFromDirectoryEx(
  PBYTE presbits,
  WINBOOL fIcon,
  int cxDesired,
  int cyDesired,
  UINT Flags)
{
    GRPCURSORICONDIR *dir = (GRPCURSORICONDIR*)presbits;
    UINT retVal = 0;

    if( dir && !dir->idReserved && (dir->idType & 3) )
    {
	GRPCURSORICONDIRENTRY* entry;
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

	entry = (GRPCURSORICONDIRENTRY*)CURSORICON_FindBestIcon( (CURSORICONDIR*)dir,
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
