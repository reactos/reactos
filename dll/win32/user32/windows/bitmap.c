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
HANDLE STDCALL
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
   HANDLE hResource;
   HANDLE h2Resource;
   HANDLE hfRes;
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

      hResource = hfRes = FindResourceW(hinst, lpszName,
                                        Icon ? RT_GROUP_ICON : RT_GROUP_CURSOR);
      if (hResource == NULL)
         return NULL;

      if (fuLoad & LR_SHARED)
      {
         hIcon = NtUserFindExistingCursorIcon(hinst, (HRSRC)hfRes, width, height);
         if (hIcon)
            return hIcon;
      }

      hResource = LoadResource(hinst, hResource);
      if (hResource == NULL)
         return NULL;

      IconResDir = LockResource(hResource);
      if (IconResDir == NULL)
         return NULL;

      /*
       * Find the best fitting in the IconResDir for this resolution
       */

      id = LookupIconIdFromDirectoryEx((PBYTE)IconResDir, Icon, width, height,
                                       fuLoad & (LR_DEFAULTCOLOR | LR_MONOCHROME));

      h2Resource = FindResourceW(hinst, MAKEINTRESOURCEW(id),
                                 Icon ? MAKEINTRESOURCEW(RT_ICON) :
                                 MAKEINTRESOURCEW(RT_CURSOR));
      if (h2Resource == NULL)
         return NULL;

      hResource = LoadResource(hinst, h2Resource);
      if (hResource == NULL)
         return NULL;

      ResIcon = LockResource(hResource);
      if (ResIcon == NULL)
         return NULL;

      hIcon = CreateIconFromResourceEx((PBYTE)ResIcon,
                                       SizeofResource(hinst, h2Resource),
                                       Icon, 0x00030000, width, height,
                                       fuLoad & (LR_DEFAULTCOLOR | LR_MONOCHROME));
      if (hIcon && 0 != (fuLoad & LR_SHARED))
      {
         NtUserSetCursorIconData((HICON)hIcon, NULL, NULL, hinst, (HRSRC)hfRes,
                                 (HRSRC)NULL);
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
   memcpy(PrivateInfo, BitmapInfo, HeaderSize);

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

HANDLE STDCALL
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
HBITMAP STDCALL
LoadBitmapA(HINSTANCE hInstance, LPCSTR lpBitmapName)
{
   return LoadImageA(hInstance, lpBitmapName, IMAGE_BITMAP, 0, 0, 0);
}


/*
 * @implemented
 */
HBITMAP STDCALL
LoadBitmapW(HINSTANCE hInstance, LPCWSTR lpBitmapName)
{
   return LoadImageW(hInstance, lpBitmapName, IMAGE_BITMAP, 0, 0, 0);
}


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
   HBITMAP res;
   BITMAP bm;

   switch (type)
   {
      case IMAGE_BITMAP:
         {
            DbgPrint("WARNING:  Incomplete implementation of CopyImage!\n");
            /*
             * FIXME: Support flags LR_COPYDELETEORG, LR_COPYFROMRESOURCE,
             * LR_COPYRETURNORG, LR_CREATEDIBSECTION and LR_MONOCHROME.
             */
            if (!GetObjectW(hnd, sizeof(bm), &bm))
                return NULL;
            bm.bmBits = NULL;
            if ((res = CreateBitmapIndirect(&bm)))
            {
               char *buf = HeapAlloc(GetProcessHeap(), 0, bm.bmWidthBytes * bm.bmHeight);
               if (buf == NULL)
               {
                  DeleteObject(res);
                  return NULL;
               }
               GetBitmapBits(hnd, bm.bmWidthBytes * bm.bmHeight, buf);
               SetBitmapBits(res, bm.bmWidthBytes * bm.bmHeight, buf);
               HeapFree(GetProcessHeap(), 0, buf);
            }
            return res;
         }

      case IMAGE_ICON:
         {
            static BOOL IconMsgDisplayed = FALSE;
            /* FIXME: support loading the image as shared from an instance */
            if (!IconMsgDisplayed)
            {
               DbgPrint("FIXME: CopyImage doesn't support IMAGE_ICON correctly!\n");
               IconMsgDisplayed = TRUE;
            }
            return CopyIcon(hnd);
         }

      case IMAGE_CURSOR:
         {
            static BOOL IconMsgDisplayed = FALSE;
            /* FIXME: support loading the image as shared from an instance */
            if (!IconMsgDisplayed)
            {
               DbgPrint("FIXME: CopyImage doesn't support IMAGE_CURSOR correctly!\n");
               IconMsgDisplayed = TRUE;
            }
            return CopyCursor(hnd);
         }
   }

   return NULL;
}
