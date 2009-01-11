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
 * FILE:            lib/user32/windows/input.c
 * PURPOSE:         Input
 * PROGRAMMER:      Casper S. Hornstrup (chorns@users.sourceforge.net)
 * UPDATE HISTORY:
 *      09-05-2001  CSH  Created
 */

/* INCLUDES ******************************************************************/

#include <user32.h>

#include <wine/debug.h>
WINE_DEFAULT_DEBUG_CHANNEL(user32);

#include "pshpack1.h"

typedef struct {
    BYTE bWidth;
    BYTE bHeight;
    BYTE bColorCount;
    BYTE bReserved;
    WORD xHotspot;
    WORD yHotspot;
    DWORD dwDIBSize;
    DWORD dwDIBOffset;
} CURSORICONFILEDIRENTRY;

typedef struct
{
    WORD                idReserved;
    WORD                idType;
    WORD                idCount;
    CURSORICONFILEDIRENTRY  idEntries[1];
} CURSORICONFILEDIR;

#include "poppack.h"

/*forward declerations... actualy in user32\windows\icon.c but usful here****/
HICON ICON_CreateCursorFromData(HDC hDC, PVOID ImageData, ICONIMAGE* IconImage, int cxDesired, int cyDesired, int xHotspot, int yHotspot);
HICON ICON_CreateIconFromData(HDC hDC, PVOID ImageData, ICONIMAGE* IconImage, int cxDesired, int cyDesired, int xHotspot, int yHotspot);
CURSORICONDIRENTRY *CURSORICON_FindBestIcon( CURSORICONDIR *dir, int width, int height, int colors);
CURSORICONDIRENTRY *CURSORICON_FindBestCursor( CURSORICONDIR *dir, int width, int height, int colors);

/* FUNCTIONS *****************************************************************/

/*
 * @implemented
 */
HANDLE WINAPI
LoadImageA(HINSTANCE hinst,
	   LPCSTR lpszName,
	   UINT uType,
	   int cxDesired,
	   int cyDesired,
	   UINT fuLoad)
{
   LPWSTR lpszWName;
   HANDLE Handle;
   UNICODE_STRING NameString;

   if (HIWORD(lpszName))
   {
      RtlCreateUnicodeStringFromAsciiz(&NameString, (LPSTR)lpszName);
      lpszWName = NameString.Buffer;
      Handle = LoadImageW(hinst, lpszWName, uType, cxDesired,
			  cyDesired, fuLoad);
      RtlFreeUnicodeString(&NameString);
   }
   else
   {
      Handle = LoadImageW(hinst, (LPCWSTR)lpszName, uType, cxDesired,
			  cyDesired, fuLoad);
   }

   return Handle;
}


/*
 *  The following macro functions account for the irregularities of
 *   accessing cursor and icon resources in files and resource entries.
 */
typedef BOOL (*fnGetCIEntry)( LPVOID dir, int n,
                              int *width, int *height, int *bits );

/**********************************************************************
 *	    CURSORICON_FindBestCursor2
 *
 * Find the cursor closest to the requested size.
 * FIXME: parameter 'color' ignored and entries with more than 1 bpp
 *        ignored too
 */
static int CURSORICON_FindBestCursor2( LPVOID dir, fnGetCIEntry get_entry,
                                      int width, int height, int color )
{
    int i, maxwidth, maxheight, cx, cy, bits, bestEntry = -1;

    /* Double height to account for AND and XOR masks */

    height *= 2;

    /* First find the largest one smaller than or equal to the requested size*/

    maxwidth = maxheight = 0;
    for ( i = 0; get_entry( dir, i, &cx, &cy, &bits ); i++ )
    {
        if ((cx <= width) && (cy <= height) &&
            (cx > maxwidth) && (cy > maxheight) &&
            (bits == 1))
        {
            bestEntry = i;
            maxwidth  = cx;
            maxheight = cy;
        }
    }
    if (bestEntry != -1) return bestEntry;

    /* Now find the smallest one larger than the requested size */

    maxwidth = maxheight = 255;
    for ( i = 0; get_entry( dir, i, &cx, &cy, &bits ); i++ )
    {
        if (((cx < maxwidth) && (cy < maxheight) && (bits == 1)) ||
            (bestEntry==-1))
        {
            bestEntry = i;
            maxwidth  = cx;
            maxheight = cy;
        }
    }

    return bestEntry;
}

static BOOL CURSORICON_GetFileEntry( LPVOID dir, int n,
                                     int *width, int *height, int *bits )
{
    CURSORICONFILEDIR *filedir = dir;
    CURSORICONFILEDIRENTRY *entry;

    if ( filedir->idCount <= n )
        return FALSE;
    entry = &filedir->idEntries[n];
    *width = entry->bWidth;
    *height = entry->bHeight;
    *bits = entry->bColorCount;
    return TRUE;
}

static CURSORICONFILEDIRENTRY *CURSORICON_FindBestCursorFile( CURSORICONFILEDIR *dir,
                                      int width, int height, int color )
{
    int n = CURSORICON_FindBestCursor2( dir, CURSORICON_GetFileEntry,
                                       width, height, color );
    if ( n < 0 )
        return NULL;
    return &dir->idEntries[n];
}

static HANDLE
LoadCursorIconImage(
   HINSTANCE hinst,
   LPCWSTR lpszName,
   INT width,
   INT height,
   UINT fuLoad,
   ULONG uType)
{
   HRSRC hResInfo;
   HANDLE hResource;
   HANDLE hFile;
   HANDLE hSection;
   CURSORICONFILEDIR *IconDIR;
   HDC hScreenDc;
   HICON hIcon;
   ULONG HeaderSize;
   ULONG ColorCount;
   ULONG ColorBits;
   PVOID Data;
   CURSORICONFILEDIRENTRY* dirEntry;
   ICONIMAGE* SafeIconImage = NULL;
   GRPCURSORICONDIR* IconResDir;
   INT id;
   ICONIMAGE *ResIcon;
   BOOL Icon = (uType == IMAGE_ICON);
   DWORD filesize = 0;

   if (!(fuLoad & LR_LOADFROMFILE))
   {
      if (hinst == NULL)
         hinst = User32Instance;

      hResInfo = FindResourceW(hinst, lpszName,
                                        Icon ? RT_GROUP_ICON : RT_GROUP_CURSOR);
      if (hResInfo == NULL)
         return NULL;

      hResource = LoadResource(hinst, hResInfo);
      if (hResource == NULL)
         return NULL;

      IconResDir = LockResource(hResource);
      if (IconResDir == NULL)
      {
         return NULL;
      }

      /* Find the best fitting in the IconResDir for this resolution */
      id = LookupIconIdFromDirectoryEx((PBYTE)IconResDir, Icon, width, height,
                                       fuLoad & (LR_DEFAULTCOLOR | LR_MONOCHROME));

      hResInfo = FindResourceW(hinst, MAKEINTRESOURCEW(id),
                                 Icon ? (LPCWSTR) RT_ICON :
                                 (LPCWSTR) RT_CURSOR);
      if (hResInfo == NULL)
      {
         return NULL;
      }

      /* Now we have found the icon we want to load.
       * Let's see if we already loaded it */
      if (fuLoad & LR_SHARED)
      {
         hIcon = NtUserFindExistingCursorIcon(hinst, hResInfo, 0, 0);
         if (hIcon)
         {
            return hIcon;
         }
         else
             TRACE("Didn't find the shared icon!!\n");
      }

      hResource = LoadResource(hinst, hResInfo);
      if (hResource == NULL)
      {
         return NULL;
      }

      ResIcon = LockResource(hResource);
      if (ResIcon == NULL)
      {
         return NULL;
      }

      hIcon = CreateIconFromResourceEx((PBYTE)ResIcon,
                                       SizeofResource(hinst, hResInfo),
                                       Icon, 0x00030000, width, height,
                                       fuLoad & (LR_DEFAULTCOLOR | LR_MONOCHROME));

      if (hIcon && 0 != (fuLoad & LR_SHARED))
      {
#if 1
         NtUserSetCursorIconData((HICON)hIcon, NULL, NULL, hinst, hResInfo,
                                 (HRSRC)NULL);
#else
         ICONINFO iconInfo;

         if(NtUserGetIconInfo(ResIcon, &iconInfo, NULL, NULL, NULL, FALSE))
            NtUserSetCursorIconData((HICON)hIcon, hinst, NULL, &iconInfo);
#endif
      }

      return hIcon;
   }

   if (fuLoad & LR_SHARED)
   {
      DbgPrint("FIXME: need LR_SHARED support for loading icon images from files\n");
   }

   hFile = CreateFileW(lpszName, GENERIC_READ, FILE_SHARE_READ, NULL,
                       OPEN_EXISTING, 0, NULL);
   if (hFile == INVALID_HANDLE_VALUE)
      return NULL;

   hSection = CreateFileMappingW(hFile, NULL, PAGE_READONLY, 0, 0, NULL);
   filesize = GetFileSize( hFile, NULL );
   CloseHandle(hFile);
   if (hSection == NULL)
      return NULL;

   IconDIR = MapViewOfFile(hSection, FILE_MAP_READ, 0, 0, 0);
   CloseHandle(hSection);
   if (IconDIR == NULL)
      return NULL;

   if (0 != IconDIR->idReserved ||
       (IMAGE_ICON != IconDIR->idType && IMAGE_CURSOR != IconDIR->idType))
   {
      UnmapViewOfFile(IconDIR);
      return NULL;
   }

   /* Get a handle to the screen dc, the icon we create is going to be
    * compatable with this. */
   hScreenDc = CreateDCW(NULL, NULL, NULL, NULL);
   if (hScreenDc == NULL)
   {
      UnmapViewOfFile(IconDIR);
      RtlFreeHeap(GetProcessHeap(), 0, SafeIconImage);
      return NULL;
   }

   if (fuLoad & LR_MONOCHROME)
   {
      ColorBits = 1;
   }
   else
   {
      ColorBits = GetDeviceCaps(hScreenDc, BITSPIXEL);
      /*
       * FIXME:
       * Remove this after proper support for alpha icons will be finished.
       */
      if (ColorBits > 8)
         ColorBits = 8;
   }

   /* Pick the best size. */
   dirEntry = CURSORICON_FindBestCursorFile( IconDIR, width, height, ColorBits );
   if (!dirEntry)
   {
      DeleteDC(hScreenDc);
      UnmapViewOfFile(IconDIR);
      return NULL;
   }

   if ( dirEntry->dwDIBOffset > filesize )
   {
      DeleteDC(hScreenDc);
      UnmapViewOfFile(IconDIR);
      return NULL;
   }

   if ( dirEntry->dwDIBOffset + dirEntry->dwDIBSize > filesize ){
      DeleteDC(hScreenDc);
      UnmapViewOfFile(IconDIR);
      return NULL;
   }

   SafeIconImage = RtlAllocateHeap(GetProcessHeap(), 0, dirEntry->dwDIBSize);
   if (SafeIconImage == NULL)
   {
      DeleteDC(hScreenDc);
      UnmapViewOfFile(IconDIR);
      return NULL;
   }

   memcpy(SafeIconImage, ((PBYTE)IconDIR) + dirEntry->dwDIBOffset, dirEntry->dwDIBSize);
   UnmapViewOfFile(IconDIR);

   /* At this point we have a copy of the icon image to play with. */

   SafeIconImage->icHeader.biHeight = SafeIconImage->icHeader.biHeight /2;

   if (SafeIconImage->icHeader.biSize == sizeof(BITMAPCOREHEADER))
   {
      BITMAPCOREHEADER* Core = (BITMAPCOREHEADER*)SafeIconImage;
      ColorCount = (Core->bcBitCount <= 8) ? (1 << Core->bcBitCount) : 0;
      HeaderSize = sizeof(BITMAPCOREHEADER) + ColorCount * sizeof(RGBTRIPLE);
   }
   else
   {
      ColorCount = SafeIconImage->icHeader.biClrUsed;
      if (ColorCount == 0 && SafeIconImage->icHeader.biBitCount <= 8)
         ColorCount = 1 << SafeIconImage->icHeader.biBitCount;
      HeaderSize = sizeof(BITMAPINFOHEADER) + ColorCount * sizeof(RGBQUAD);
   }

   /* Make data point to the start of the XOR image data. */
   Data = (PBYTE)SafeIconImage + HeaderSize;

   hIcon = ICON_CreateIconFromData(hScreenDc, Data, SafeIconImage, width, height, width/2, height/2);
   RtlFreeHeap(GetProcessHeap(), 0, SafeIconImage);
   DeleteDC(hScreenDc);

   return hIcon;
}


static HANDLE
LoadBitmapImage(HINSTANCE hInstance, LPCWSTR lpszName, UINT fuLoad)
{
   HANDLE hResource;
   HANDLE hFile;
   HANDLE hSection;
   LPBITMAPINFO BitmapInfo;
   LPBITMAPINFO PrivateInfo;
   HDC hScreenDc;
   HANDLE hBitmap;
   ULONG HeaderSize;
   ULONG ColorCount;
   PVOID Data;
   BOOL Hit = FALSE;

   if (!(fuLoad & LR_LOADFROMFILE))
   {
      if (hInstance == NULL)
         hInstance = User32Instance;

      hResource = FindResourceW(hInstance, lpszName, RT_BITMAP);
      if (hResource == NULL)
         return NULL;
      hResource = LoadResource(hInstance, hResource);
      if (hResource == NULL)
         return NULL;
      BitmapInfo = LockResource(hResource);
      if (BitmapInfo == NULL)
         return NULL;
   }
   else
   {
      hFile = CreateFileW(lpszName, GENERIC_READ, FILE_SHARE_READ, NULL,
                          OPEN_EXISTING, 0, NULL);
      if (hFile == INVALID_HANDLE_VALUE)
         return NULL;

      hSection = CreateFileMappingW(hFile, NULL, PAGE_READONLY, 0, 0, NULL);
      CloseHandle(hFile);
      if (hSection == NULL)
         return NULL;

      BitmapInfo = MapViewOfFile(hSection, FILE_MAP_READ, 0, 0, 0);
      CloseHandle(hSection);
      if (BitmapInfo == NULL)
         return NULL;

      BitmapInfo = (LPBITMAPINFO)((ULONG_PTR)BitmapInfo + sizeof(BITMAPFILEHEADER));
   }

   if (BitmapInfo->bmiHeader.biSize == sizeof(BITMAPCOREHEADER))
   {
      BITMAPCOREHEADER* Core = (BITMAPCOREHEADER*)BitmapInfo;
      ColorCount = (Core->bcBitCount <= 8) ? (1 << Core->bcBitCount) : 0;
      HeaderSize = sizeof(BITMAPCOREHEADER) + ColorCount * sizeof(RGBTRIPLE);
   }
   else
   {
      ColorCount = BitmapInfo->bmiHeader.biClrUsed;
      if (ColorCount == 0 && BitmapInfo->bmiHeader.biBitCount <= 8)
         ColorCount = 1 << BitmapInfo->bmiHeader.biBitCount;
      HeaderSize = sizeof(BITMAPINFOHEADER) + ColorCount * sizeof(RGBQUAD);
   }
   Data = (PVOID)((ULONG_PTR)BitmapInfo + HeaderSize);

   PrivateInfo = RtlAllocateHeap(GetProcessHeap(), 0, HeaderSize);
   if (PrivateInfo == NULL)
   {
      if (fuLoad & LR_LOADFROMFILE)
         UnmapViewOfFile(BitmapInfo);
      return NULL;
   }

   _SEH2_TRY
   {
   memcpy(PrivateInfo, BitmapInfo, HeaderSize);
   }
   _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
   {
      Hit = TRUE;
   }
   _SEH2_END;

   if (Hit)
   {
      DbgPrint("We have a thread overrun, these are already freed! pi -> %d bi -> %d\n", PrivateInfo, BitmapInfo);
      RtlFreeHeap(GetProcessHeap(), 0, PrivateInfo);
      if (fuLoad & LR_LOADFROMFILE)
         UnmapViewOfFile(BitmapInfo);
      return NULL;
   }
   
   /* FIXME: Handle color conversion and transparency. */

   hScreenDc = CreateCompatibleDC(NULL);
   if (hScreenDc == NULL)
   {
      RtlFreeHeap(GetProcessHeap(), 0, PrivateInfo);
      if (fuLoad & LR_LOADFROMFILE)
         UnmapViewOfFile(BitmapInfo);
      return NULL;
   }

   if (fuLoad & LR_CREATEDIBSECTION)
   {
      DIBSECTION Dib;

      hBitmap = CreateDIBSection(hScreenDc, PrivateInfo, DIB_RGB_COLORS, NULL,
                                 0, 0);
      GetObjectA(hBitmap, sizeof(DIBSECTION), &Dib);
      SetDIBits(hScreenDc, hBitmap, 0, Dib.dsBm.bmHeight, Data, BitmapInfo,
                DIB_RGB_COLORS);
   }
   else
   {
      hBitmap = CreateDIBitmap(hScreenDc, &PrivateInfo->bmiHeader, CBM_INIT,
                               Data, PrivateInfo, DIB_RGB_COLORS);
   }

   RtlFreeHeap(GetProcessHeap(), 0, PrivateInfo);
   DeleteDC(hScreenDc);
   if (fuLoad & LR_LOADFROMFILE)
      UnmapViewOfFile(BitmapInfo);

   return hBitmap;
}

HANDLE WINAPI
LoadImageW(
   IN HINSTANCE hinst,
   IN LPCWSTR lpszName,
   IN UINT uType,
   IN INT cxDesired,
   IN INT cyDesired,
   IN UINT fuLoad)
{
   if (fuLoad & LR_DEFAULTSIZE)
   {
      if (uType == IMAGE_ICON)
      {
         if (cxDesired == 0)
            cxDesired = GetSystemMetrics(SM_CXICON);
         if (cyDesired == 0)
            cyDesired = GetSystemMetrics(SM_CYICON);
      }
      else if (uType == IMAGE_CURSOR)
      {
         if (cxDesired == 0)
            cxDesired = GetSystemMetrics(SM_CXCURSOR);
         if (cyDesired == 0)
            cyDesired = GetSystemMetrics(SM_CYCURSOR);
      }
   }

   switch (uType)
   {
      case IMAGE_BITMAP:
         return LoadBitmapImage(hinst, lpszName, fuLoad);
      case IMAGE_CURSOR:
      case IMAGE_ICON:
         return LoadCursorIconImage(hinst, lpszName, cxDesired, cyDesired,
                                    fuLoad, uType);
      default:
         break;
   }

   return NULL;
}


/*
 * @implemented
 */
HBITMAP WINAPI
LoadBitmapA(HINSTANCE hInstance, LPCSTR lpBitmapName)
{
   return LoadImageA(hInstance, lpBitmapName, IMAGE_BITMAP, 0, 0, 0);
}


/*
 * @implemented
 */
HBITMAP WINAPI
LoadBitmapW(HINSTANCE hInstance, LPCWSTR lpBitmapName)
{
   return LoadImageW(hInstance, lpBitmapName, IMAGE_BITMAP, 0, 0, 0);
}


static HANDLE
CopyBmp(HANDLE hnd,
        UINT type,
        INT desiredx,
        INT desiredy,
        UINT flags)
{
    HBITMAP res = NULL;
    DIBSECTION ds;
    int objSize;
    BITMAPINFO * bi;

    objSize = GetObjectW( hnd, sizeof(ds), &ds );
    if (!objSize) return 0;
    if ((desiredx < 0) || (desiredy < 0)) return 0;

    if (flags & LR_COPYFROMRESOURCE)
    {
        FIXME("FIXME: The flag LR_COPYFROMRESOURCE is not implemented for bitmaps\n");
    }

    if (desiredx == 0) desiredx = ds.dsBm.bmWidth;
    if (desiredy == 0) desiredy = ds.dsBm.bmHeight;

    /* Allocate memory for a BITMAPINFOHEADER structure and a
       color table. The maximum number of colors in a color table
       is 256 which corresponds to a bitmap with depth 8.
       Bitmaps with higher depths don't have color tables. */
    bi = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(BITMAPINFOHEADER) + 256 * sizeof(RGBQUAD));
    if (!bi) return 0;

    bi->bmiHeader.biSize        = sizeof(bi->bmiHeader);
    bi->bmiHeader.biPlanes      = ds.dsBm.bmPlanes;
    bi->bmiHeader.biBitCount    = ds.dsBm.bmBitsPixel;
    bi->bmiHeader.biCompression = BI_RGB;

    if (flags & LR_CREATEDIBSECTION)
    {
        /* Create a DIB section. LR_MONOCHROME is ignored */
        void * bits;
        HDC dc = CreateCompatibleDC(NULL);

        if (objSize == sizeof(DIBSECTION))
        {
            /* The source bitmap is a DIB.
               Get its attributes to create an exact copy */
            memcpy(bi, &ds.dsBmih, sizeof(BITMAPINFOHEADER));
        }

        /* Get the color table or the color masks */
        GetDIBits(dc, hnd, 0, ds.dsBm.bmHeight, NULL, bi, DIB_RGB_COLORS);

        bi->bmiHeader.biWidth  = desiredx;
        bi->bmiHeader.biHeight = desiredy;
        bi->bmiHeader.biSizeImage = 0;

        res = CreateDIBSection(dc, bi, DIB_RGB_COLORS, &bits, NULL, 0);
        DeleteDC(dc);
    }
    else
    {
        /* Create a device-dependent bitmap */

        BOOL monochrome = (flags & LR_MONOCHROME);

        if (objSize == sizeof(DIBSECTION))
        {
            /* The source bitmap is a DIB section.
               Get its attributes */
            HDC dc = CreateCompatibleDC(NULL);
            bi->bmiHeader.biSize = sizeof(bi->bmiHeader);
            bi->bmiHeader.biBitCount = ds.dsBm.bmBitsPixel;
            GetDIBits(dc, hnd, 0, ds.dsBm.bmHeight, NULL, bi, DIB_RGB_COLORS);
            DeleteDC(dc);

            if (!monochrome && ds.dsBm.bmBitsPixel == 1)
            {
                /* Look if the colors of the DIB are black and white */

                monochrome =
                      (bi->bmiColors[0].rgbRed == 0xff
                    && bi->bmiColors[0].rgbGreen == 0xff
                    && bi->bmiColors[0].rgbBlue == 0xff
                    && bi->bmiColors[0].rgbReserved == 0
                    && bi->bmiColors[1].rgbRed == 0
                    && bi->bmiColors[1].rgbGreen == 0
                    && bi->bmiColors[1].rgbBlue == 0
                    && bi->bmiColors[1].rgbReserved == 0)
                    ||
                      (bi->bmiColors[0].rgbRed == 0
                    && bi->bmiColors[0].rgbGreen == 0
                    && bi->bmiColors[0].rgbBlue == 0
                    && bi->bmiColors[0].rgbReserved == 0
                    && bi->bmiColors[1].rgbRed == 0xff
                    && bi->bmiColors[1].rgbGreen == 0xff
                    && bi->bmiColors[1].rgbBlue == 0xff
                    && bi->bmiColors[1].rgbReserved == 0);
            }
        }
        else if (!monochrome)
        {
            monochrome = ds.dsBm.bmBitsPixel == 1;
        }

        if (monochrome)
        {
            res = CreateBitmap(desiredx, desiredy, 1, 1, NULL);
        }
        else
        {
            HDC screenDC = GetDC(NULL);
            res = CreateCompatibleBitmap(screenDC, desiredx, desiredy);
            ReleaseDC(NULL, screenDC);
        }
    }

    if (res)
    {
        /* Only copy the bitmap if it's a DIB section or if it's
           compatible to the screen */
        BOOL copyContents;

        if (objSize == sizeof(DIBSECTION))
        {
            copyContents = TRUE;
        }
        else
        {
            HDC screenDC = GetDC(NULL);
            int screen_depth = GetDeviceCaps(screenDC, BITSPIXEL);
            ReleaseDC(NULL, screenDC);

            copyContents = (ds.dsBm.bmBitsPixel == 1 || ds.dsBm.bmBitsPixel == screen_depth);
        }

        if (copyContents)
        {
            /* The source bitmap may already be selected in a device context,
               use GetDIBits/StretchDIBits and not StretchBlt  */

            HDC dc;
            void * bits;

            dc = CreateCompatibleDC(NULL);

            bi->bmiHeader.biWidth = ds.dsBm.bmWidth;
            bi->bmiHeader.biHeight = ds.dsBm.bmHeight;
            bi->bmiHeader.biSizeImage = 0;
            bi->bmiHeader.biClrUsed = 0;
            bi->bmiHeader.biClrImportant = 0;

            /* Fill in biSizeImage */
            GetDIBits(dc, hnd, 0, ds.dsBm.bmHeight, NULL, bi, DIB_RGB_COLORS);
            bits = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, bi->bmiHeader.biSizeImage);

            if (bits)
            {
                HBITMAP oldBmp;

                /* Get the image bits of the source bitmap */
                GetDIBits(dc, hnd, 0, ds.dsBm.bmHeight, bits, bi, DIB_RGB_COLORS);

                /* Copy it to the destination bitmap */
                oldBmp = SelectObject(dc, res);
                StretchDIBits(dc, 0, 0, desiredx, desiredy,
                              0, 0, ds.dsBm.bmWidth, ds.dsBm.bmHeight,
                              bits, bi, DIB_RGB_COLORS, SRCCOPY);
                SelectObject(dc, oldBmp);

                HeapFree(GetProcessHeap(), 0, bits);
            }

            DeleteDC(dc);
        }

        if (flags & LR_COPYDELETEORG)
        {
            DeleteObject(hnd);
        }
    }
    HeapFree(GetProcessHeap(), 0, bi);
    return res;
}


INT
GetIconCurBpp(PICONINFO pIconInfo)
{
    PBITMAPINFO pbi;

    pbi = (PBITMAPINFO)pIconInfo->hbmColor;
    return pbi->bmiHeader.biBitCount;
}

#if 0
static BOOL
SetCursorIconData(
  HANDLE Handle,
  HINSTANCE hMod,
  LPWSTR lpResName,
  PICONINFO pIconInfo)
{

    UNICODE_STRING Res;

    if (!Handle || !pIconInfo)
        return FALSE;

    RtlInitUnicodeString(&Res, lpResName);

    return NtUserSetCursorIconData(Handle, hMod, &Res, pIconInfo);

}


/* bare bones icon copy implementation */
static HANDLE
CopyIcoCur(HANDLE hIconCur,
           UINT type,
           INT desiredx,
           INT desiredy,
           UINT flags)
{
    HANDLE hNewIcon = NULL;
    ICONINFO origIconInfo, newIconInfo;
    SIZE origSize;
    DWORD origBpp;

    if (!hIconCur)
        return NULL;

    if (flags & LR_COPYFROMRESOURCE)
    {
        TRACE("FIXME: LR_COPYFROMRESOURCE is yet not implemented for icons\n");
    }

    if (NtUserGetIconSize(hIconCur, 0, &origSize.cx, &origSize.cy))
    {
        if (desiredx == 0) desiredx = origSize.cx;
        if (desiredx == 0) desiredy = origSize.cy;

        if (NtUserGetIconInfo(hIconCur, &origIconInfo, NULL, NULL, &origBpp, TRUE))
        {
            hNewIcon = (HANDLE)NtUserCallOneParam(0, ONEPARAM_ROUTINE_CREATECURICONHANDLE);

            if (hNewIcon)
            {
                /* the bitmaps returned from the NtUserGetIconInfo are copies of the original,
                 * so we can use these directly to build up our icon/cursor copy */
                RtlCopyMemory(&newIconInfo, &origIconInfo, sizeof(ICONINFO));

                if (!SetCursorIconData(hNewIcon, NULL, NULL, &newIconInfo))
                {
                    if (newIconInfo.fIcon)
                        DestroyIcon(hNewIcon);
                    else
                        DestroyCursor(hNewIcon);

                    hNewIcon = NULL;
                }
            }

            DeleteObject(origIconInfo.hbmMask);
            DeleteObject(origIconInfo.hbmColor);
        }
    }

    if (hNewIcon && (flags & LR_COPYDELETEORG))
    {
        DestroyCursor((HCURSOR)hIconCur);
    }

    return hNewIcon;
}
#endif

/*
 * @unimplemented
 */
HANDLE WINAPI
CopyImage(
   IN HANDLE hnd,
   IN UINT type,
   IN INT desiredx,
   IN INT desiredy,
   IN UINT flags)
{
/*
 * BUGS
 *    Only Windows NT 4.0 supports the LR_COPYRETURNORG flag for bitmaps,
 *    all other versions (95/2000/XP have been tested) ignore it.
 *
 * NOTES
 *    If LR_CREATEDIBSECTION is absent, the copy will be monochrome for
 *    a monochrome source bitmap or if LR_MONOCHROME is present, otherwise
 *    the copy will have the same depth as the screen.
 *    The content of the image will only be copied if the bit depth of the
 *    original image is compatible with the bit depth of the screen, or
 *    if the source is a DIB section.
 *    The LR_MONOCHROME flag is ignored if LR_CREATEDIBSECTION is present.
 */
   switch (type)
   {
      case IMAGE_BITMAP:
        return CopyBmp(hnd, type, desiredx, desiredy, flags);

      case IMAGE_ICON:
        //return CopyIcoCur(hnd, type, desiredx, desiredy, flags);
          return CopyIcon(hnd);

      case IMAGE_CURSOR:
         {
            static BOOL IconMsgDisplayed = FALSE;
            /* FIXME: support loading the image as shared from an instance */
            if (!IconMsgDisplayed)
            {
               FIXME("FIXME: CopyImage doesn't support IMAGE_CURSOR correctly!\n");
               IconMsgDisplayed = TRUE;
            }
            /* Should call CURSORICON_ExtCopy but more testing
             * needs to be done before we change this
             */
            if (flags) FIXME("FIXME: Flags are ignored\n");
            return CopyCursor(hnd);
         }
   }

   return NULL;
}
