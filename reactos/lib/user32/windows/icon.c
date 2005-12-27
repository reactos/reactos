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
#define NDEBUG
#include <debug.h>


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

  if(NtUserGetCursorIconInfo((HANDLE)hIcon, &IconInfo))
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
  return CreateIconFromResourceEx(presbits, dwResSize, fIcon, dwVer, 0, 0, 0);
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

  DPRINT("dwVersion, cxDesired, cyDesired are all ignored in this implementation!\n");

  if (! fIcon)
    {
      wXHotspot = *(WORD*)pbIconBits;
      pbIconBits+=sizeof(WORD);
      wYHotspot = *(WORD*)pbIconBits;
      pbIconBits+=sizeof(WORD);
      cbIconBits-=2*sizeof(WORD);
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

  /* take into acount the origonal height was for both the AND and XOR images */
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
  if(IconInfo->hbmColor && !GetObjectW(IconInfo->hbmColor, sizeof(BITMAP), &ColorBitmap))
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
  return (BOOL)NtUserDestroyCursorIcon((HANDLE)hIcon, 0);
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
  /* FIXME - copy bitmaps */
  return (BOOL)NtUserGetCursorIconInfo((HANDLE)hIcon, IconInfo);
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
    return LookupIconIdFromDirectoryEx( presbits, fIcon,
	   fIcon ? GetSystemMetrics(SM_CXICON) : GetSystemMetrics(SM_CXCURSOR),
	   fIcon ? GetSystemMetrics(SM_CYICON) : GetSystemMetrics(SM_CYCURSOR), LR_DEFAULTCOLOR );
}

/* Ported from WINE20030408 */
GRPCURSORICONDIRENTRY*
CURSORICON_FindBestCursor( GRPCURSORICONDIR *dir, int width, int height, int colors)
{
    int i;
    GRPCURSORICONDIRENTRY *entry, *bestEntry = NULL;
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
		iTempXDiff = abs(width - entry->ResInfo.icon.bWidth);
		iTempYDiff = abs(height - entry->ResInfo.icon.bHeight);

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
        if(abs(width - entry->ResInfo.icon.bWidth) == (int) iXDiff &&
            abs(height - entry->ResInfo.icon.bHeight) == (int) iYDiff)
        {
            iTempColorDiff = abs(colors - entry->ResInfo.icon.bColorCount);

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
GRPCURSORICONDIRENTRY*
CURSORICON_FindBestIcon( GRPCURSORICONDIR *dir, int width, int height, int colorbits)
{
    int i;
    GRPCURSORICONDIRENTRY *entry, *bestEntry = NULL;
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
	iTempXDiff = abs(width - entry->ResInfo.icon.bWidth);

	iTempYDiff = abs(height - entry->ResInfo.icon.bHeight);

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
        if(abs(width - entry->ResInfo.icon.bWidth) == (int) iXDiff &&
           abs(height - entry->ResInfo.icon.bHeight) == (int) iYDiff)
        {
            iTempColorDiff = abs(colorbits - entry->wBitCount);
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
INT STDCALL
LookupIconIdFromDirectoryEx(
  PBYTE presbits,
  BOOL fIcon,
  int cxDesired,
  int cyDesired,
  UINT Flags)
{
   GRPCURSORICONDIR *dir = (GRPCURSORICONDIR*)presbits;
   UINT retVal = 0;

   if (dir && !dir->idReserved && (IMAGE_ICON == dir->idType || IMAGE_CURSOR == dir->idType))
   {
      GRPCURSORICONDIRENTRY *entry;
      HDC hdc;
      int ColorBits;

      hdc = CreateICW(NULL, NULL, NULL, NULL);
      if (Flags & LR_MONOCHROME)
      {
         ColorBits = 1;
      }
      else
      {
         ColorBits = GetDeviceCaps(hdc, BITSPIXEL);
         if (ColorBits > 8)
            ColorBits = 8;
      }
      DeleteDC(hdc);

      entry = CURSORICON_FindBestIcon( dir, cxDesired, cyDesired, ColorBits );

      if (entry)
         retVal = entry->nID;
   }
   else
   {
       DbgPrint("invalid resource directory\n");
   }
   return retVal;
}
