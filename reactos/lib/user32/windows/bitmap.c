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
/* $Id: bitmap.c,v 1.29 2004/04/13 00:06:50 weiden Exp $
 *
 * PROJECT:         ReactOS user32.dll
 * FILE:            lib/user32/windows/input.c
 * PURPOSE:         Input
 * PROGRAMMER:      Casper S. Hornstrup (chorns@users.sourceforge.net)
 * UPDATE HISTORY:
 *      09-05-2001  CSH  Created
 */

/* INCLUDES ******************************************************************/

#include <string.h>
#include <windows.h>
#include <user32.h>
#include <debug.h>
#include <stdlib.h>
#define NTOS_MODE_USER
#include <ntos.h>

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
  return(Handle);
}


HANDLE STATIC
LoadCursorImage(HINSTANCE hinst, LPCWSTR lpszName, UINT fuLoad)
{
   HANDLE hResource;
   HANDLE h2Resource;
   HANDLE hfRes;
   HANDLE hFile;
   HANDLE hSection;
   CURSORICONDIR *IconDIR;
   HDC hScreenDc;
   HANDLE hIcon;
   ULONG HeaderSize;
   ULONG ColorCount;
   PVOID Data;
   CURSORICONDIRENTRY* dirEntry;
   ICONIMAGE* SafeIconImage;
   GRPCURSORICONDIR* IconResDir;
   INT id;
   ICONIMAGE *ResIcon;
   UINT ColorBits;
  
   if (!(fuLoad & LR_LOADFROMFILE))
   {
      if (hinst == NULL)
      {
         hinst = GetModuleHandleW(L"USER32");
      }
      hResource = hfRes = FindResourceW(hinst, lpszName, RT_GROUP_CURSOR);
      if (hResource == NULL)
      {
         return NULL;
      }
	  
      if (fuLoad & LR_SHARED)
      {
         /* FIXME - pass size! */
         hIcon = (HANDLE)NtUserFindExistingCursorIcon(hinst, (HRSRC)hfRes, 0, 0);
         if (hIcon)
         {
            return hIcon;
         }
      }

      hResource = LoadResource(hinst, hResource);
      if (hResource == NULL)
      {
         return NULL;
      }
      IconResDir = LockResource(hResource);
      if (IconResDir == NULL)
      {
         return NULL;
      }

      /* Find the best fitting in the IconResDir for this resolution. */
      id = LookupIconIdFromDirectoryEx((PBYTE)IconResDir, TRUE,
         32, 32, fuLoad & (LR_DEFAULTCOLOR | LR_MONOCHROME));

      h2Resource = FindResourceW(hinst, MAKEINTRESOURCEW(id),
         MAKEINTRESOURCEW(RT_CURSOR));

      hResource = LoadResource(hinst, h2Resource);
      if (hResource == NULL)
      {
         return NULL;
      }

      ResIcon = LockResource(hResource);
      if (ResIcon == NULL)
      {
         return NULL;
      }

      hIcon = (HANDLE)CreateIconFromResourceEx((PBYTE)ResIcon,
         SizeofResource(hinst, h2Resource), FALSE, 0x00030000,
         32, 32, fuLoad & (LR_DEFAULTCOLOR | LR_MONOCHROME));
      if (hIcon)
      {
         NtUserSetCursorIconData((HICON)hIcon, NULL, NULL, hinst, (HRSRC)hfRes, 
                                 (HRSRC)NULL);
      }

      return hIcon;
   }
   else
   {
      if (fuLoad & LR_SHARED)
      {
         DbgPrint("FIXME: need LR_SHARED support loading cursor images from files\n");
      }
      
      hFile = CreateFileW(lpszName, GENERIC_READ, FILE_SHARE_READ, NULL,
         OPEN_EXISTING, 0, NULL);
      if (hFile == NULL)
      {
         return NULL;
      }

      hSection = CreateFileMappingW(hFile, NULL, PAGE_READONLY, 0, 0, NULL);
      CloseHandle(hFile);
      if (hSection == NULL)
      {
         return NULL;
      }

      IconDIR = MapViewOfFile(hSection, FILE_MAP_READ, 0, 0, 0);
      CloseHandle(hSection);
      if (IconDIR == NULL)
      {
         return NULL;
      }

      /* 
       * Get a handle to the screen dc, the icon we create is going to be
       * compatable with it.
       */
      hScreenDc = CreateCompatibleDC(0);
      if (hScreenDc == NULL)
      {
         UnmapViewOfFile(IconDIR);
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
      dirEntry = (CURSORICONDIRENTRY *)CURSORICON_FindBestIcon(IconDIR, 32, 32, ColorBits);
      if (!dirEntry)
      {
         UnmapViewOfFile(IconDIR);
         return(NULL);
      }

      SafeIconImage = RtlAllocateHeap(GetProcessHeap(), 0, dirEntry->dwBytesInRes); 
      memcpy(SafeIconImage, ((PBYTE)IconDIR) + dirEntry->dwImageOffset, dirEntry->dwBytesInRes);
   }

  //at this point we have a copy of the icon image to play with

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
	{
	  ColorCount = 1 << SafeIconImage->icHeader.biBitCount;
	}
      HeaderSize = sizeof(BITMAPINFOHEADER) + ColorCount * sizeof(RGBQUAD);
    }
  
  //make data point to the start of the XOR image data
  Data = (PBYTE)SafeIconImage + HeaderSize;

  hIcon = ICON_CreateCursorFromData(hScreenDc, Data, SafeIconImage, 32, 32, dirEntry->Info.cursor.wXHotspot, dirEntry->Info.cursor.wYHotspot);
  DeleteDC(hScreenDc);
  RtlFreeHeap(GetProcessHeap(), 0, SafeIconImage);
  return hIcon;
}


HANDLE STATIC
LoadIconImage(HINSTANCE hinst, LPCWSTR lpszName, INT width, INT height, UINT fuLoad)
{
  HANDLE hResource;
  HANDLE h2Resource;
  HANDLE hfRes;
  HANDLE hFile;
  HANDLE hSection;
  CURSORICONDIR* IconDIR;
  HDC hScreenDc;
  HANDLE hIcon;
  ULONG HeaderSize;
  ULONG ColorCount;
  PVOID Data;
  CURSORICONDIRENTRY* dirEntry;
  ICONIMAGE* SafeIconImage;
  GRPCURSORICONDIR* IconResDir;
  INT id;
  ICONIMAGE *ResIcon;
  
  if (!(fuLoad & LR_LOADFROMFILE))
  {
      if (hinst == NULL)
	  {
	    hinst = GetModuleHandleW(L"USER32");
	  }
      hResource = hfRes = FindResourceW(hinst, lpszName, RT_GROUP_ICON);
      if (hResource == NULL)
	  {
	    return(NULL);
	  }
	  
      if (fuLoad & LR_SHARED)
          {
            hIcon = NtUserFindExistingCursorIcon(hinst, (HRSRC)hfRes, width, height);
            if(hIcon)
              return hIcon;
          }

      hResource = LoadResource(hinst, hResource);
      if (hResource == NULL)
	  {
	    return(NULL);
	  }
      IconResDir = LockResource(hResource);
      if (IconResDir == NULL)
	  {
	    return(NULL);
	  }

      //find the best fitting in the IconResDir for this resolution
      id = LookupIconIdFromDirectoryEx((PBYTE) IconResDir, TRUE,
                width, height, fuLoad & (LR_DEFAULTCOLOR | LR_MONOCHROME));

	  h2Resource = FindResourceW(hinst,
                     MAKEINTRESOURCEW(id),
                     MAKEINTRESOURCEW(RT_ICON));

      hResource = LoadResource(hinst, h2Resource);
      if (hResource == NULL)
	  {
	    return(NULL);
	  }

      ResIcon = LockResource(hResource);
      if (ResIcon == NULL)
	  {
	    return(NULL);
	  }
      hIcon = (HANDLE)CreateIconFromResourceEx((PBYTE) ResIcon,
                        SizeofResource(hinst, h2Resource), TRUE, 0x00030000,
                        width, height, fuLoad & (LR_DEFAULTCOLOR | LR_MONOCHROME));
      if(hIcon)
      {
        NtUserSetCursorIconData((HICON)hIcon, NULL, NULL, hinst, (HRSRC)hfRes, 
                                (HRSRC)NULL);
      }
      return hIcon;
  }
  else
  {
      if (fuLoad & LR_SHARED)
      {
        DbgPrint("FIXME: need LR_SHARED support for loading icon images from files\n");
      }
      
      hFile = CreateFileW(lpszName,
			 GENERIC_READ,
			 FILE_SHARE_READ,
			 NULL,
			 OPEN_EXISTING,
			 0,
			 NULL);
      if (hFile == NULL)
	  {
	    return(NULL);
	  }

      hSection = CreateFileMappingW(hFile,
				   NULL,
				   PAGE_READONLY,
				   0,
				   0,
				   NULL);

      if (hSection == NULL)
	  {
	    CloseHandle(hFile);
	    return(NULL);
	  }
      IconDIR = MapViewOfFile(hSection,
				 FILE_MAP_READ,
				 0,
				 0,
				 0);

      if (IconDIR == NULL)
	  {
	    CloseHandle(hFile);
	    CloseHandle(hSection);
	    return(NULL);
	  }

      //pick the best size.
      dirEntry = (CURSORICONDIRENTRY *)  CURSORICON_FindBestIcon( IconDIR, width, height, 1);


      if (!dirEntry)
	  {
	       CloseHandle(hFile);
	       CloseHandle(hSection);
	       UnmapViewOfFile(IconDIR);
	       return(NULL);
	  }

      SafeIconImage = RtlAllocateHeap(GetProcessHeap(), 0, dirEntry->dwBytesInRes); 

      memcpy(SafeIconImage, ((PBYTE)IconDIR) + dirEntry->dwImageOffset, dirEntry->dwBytesInRes);

      CloseHandle(hFile);
      CloseHandle(hSection);
  }

  //at this point we have a copy of the icon image to play with

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
	{
	  ColorCount = 1 << SafeIconImage->icHeader.biBitCount;
	}
      HeaderSize = sizeof(BITMAPINFOHEADER) + ColorCount * sizeof(RGBQUAD);
    }
  
  //make data point to the start of the XOR image data
  Data = (PBYTE)SafeIconImage + HeaderSize;


  //get a handle to the screen dc, the icon we create is going to be compatable with this
  hScreenDc = CreateDCW(L"DISPLAY", NULL, NULL, NULL);
  if (hScreenDc == NULL)
  {
      if (fuLoad & LR_LOADFROMFILE)
	  {
	  	RtlFreeHeap(GetProcessHeap(), 0, SafeIconImage);
        UnmapViewOfFile(IconDIR);
	  }
      return(NULL);
  }

  hIcon = ICON_CreateIconFromData(hScreenDc, Data, SafeIconImage, width, height, width/2, height/2);
  RtlFreeHeap(GetProcessHeap(), 0, SafeIconImage);
  return hIcon;
}


HANDLE STATIC
LoadBitmapImage(HINSTANCE hInstance, LPCWSTR lpszName, UINT fuLoad)
{
  HANDLE hResource;
  HANDLE hFile;
  HANDLE hSection;
  BITMAPINFO* BitmapInfo;
  BITMAPINFO* PrivateInfo;
  HDC hScreenDc;
  HANDLE hBitmap;
  ULONG HeaderSize;
  ULONG ColorCount;
  PVOID Data;

  if (!(fuLoad & LR_LOADFROMFILE))
    {
      if (hInstance == NULL)
	{
	  hInstance = GetModuleHandleW(L"USER32");
	}
      hResource = FindResourceW(hInstance, lpszName, RT_BITMAP);
      if (hResource == NULL)
	{
	  return(NULL);
	}
      hResource = LoadResource(hInstance, hResource);
      if (hResource == NULL)
	{
	  return(NULL);
	}
      BitmapInfo = LockResource(hResource);
      if (BitmapInfo == NULL)
	{
	  return(NULL);
	}
    }
  else
    {
      hFile = CreateFileW(lpszName,
			 GENERIC_READ,
			 FILE_SHARE_READ,
			 NULL,
			 OPEN_EXISTING,
			 0,
			 NULL);
      if (hFile == NULL)
	{
	  return(NULL);
	}
      hSection = CreateFileMappingW(hFile,
				   NULL,
				   PAGE_READONLY,
				   0,
				   0,
				   NULL);
      CloseHandle(hFile);
      if (hSection == NULL)
	{		
	  return(NULL);
	}
      BitmapInfo = MapViewOfFile(hSection,
				 FILE_MAP_READ,
				 0,
				 0,
				 0);
      CloseHandle(hSection);
      if (BitmapInfo == NULL)
	{
	  return(NULL);
	}
	/* offset BitmapInfo by 14 bytes to acount for the size of BITMAPFILEHEADER
	   unfortunatly sizeof(BITMAPFILEHEADER) = 16, but the acutal size should be 14!
	*/
	BitmapInfo = (BITMAPINFO*)(((PBYTE)BitmapInfo) + 14);
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
	{
	  ColorCount = 1 << BitmapInfo->bmiHeader.biBitCount;
	}
      HeaderSize = sizeof(BITMAPINFOHEADER) + ColorCount * sizeof(RGBQUAD);
    }
  Data = (PVOID)BitmapInfo + HeaderSize;

  PrivateInfo = RtlAllocateHeap(GetProcessHeap(), 0, HeaderSize);
  if (PrivateInfo == NULL)
    {
      if (fuLoad & LR_LOADFROMFILE)
	{
	  UnmapViewOfFile(BitmapInfo);
	}
      return(NULL);
    }
  memcpy(PrivateInfo, BitmapInfo, HeaderSize);

  /* FIXME: Handle color conversion and transparency. */

  hScreenDc = CreateDCW(L"DISPLAY", NULL, NULL, NULL);
  if (hScreenDc == NULL)
    {
      RtlFreeHeap(GetProcessHeap(), 0, PrivateInfo);
      if (fuLoad & LR_LOADFROMFILE)
	{
	  UnmapViewOfFile(BitmapInfo);
	}
      return(NULL);
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
  /*DeleteDC(hScreenDc);*/
  if (fuLoad & LR_LOADFROMFILE)
    {
      UnmapViewOfFile(BitmapInfo);
    }
  return(hBitmap);
}

HANDLE STDCALL
LoadImageW(HINSTANCE hinst,
	   LPCWSTR lpszName,
	   UINT uType,
	   int cxDesired,
	   int cyDesired,
	   UINT fuLoad)
{  
  if (fuLoad & LR_DEFAULTSIZE)
    {
      if (uType == IMAGE_ICON)
	{
	  if (cxDesired == 0)
	    {
	      cxDesired = GetSystemMetrics(SM_CXICON);
	    }
	  if (cyDesired == 0)
	    {
	      cyDesired = GetSystemMetrics(SM_CYICON);
	    }
	}
      else if (uType == IMAGE_CURSOR)
	{
	  if (cxDesired == 0)
	    {
	      cxDesired = GetSystemMetrics(SM_CXCURSOR);
	    }
	  if (cyDesired == 0)
	    {
	      cyDesired = GetSystemMetrics(SM_CYCURSOR);
	    }
	}
    }

  switch (uType)
    {
    case IMAGE_BITMAP:
      {
	return(LoadBitmapImage(hinst, lpszName, fuLoad));
      }
    case IMAGE_CURSOR:
      {
	return(LoadCursorImage(hinst, lpszName, fuLoad));
      }
    case IMAGE_ICON:
      {
	return(LoadIconImage(hinst, lpszName, cxDesired, cyDesired, fuLoad));
      }
    default:
      DbgBreakPoint();
      break;
    }
  return(NULL);
}


/*
 * @implemented
 */
HBITMAP STDCALL
LoadBitmapA(HINSTANCE hInstance, LPCSTR lpBitmapName)
{
  return(LoadImageA(hInstance, lpBitmapName, IMAGE_BITMAP, 0, 0, 0));
}


/*
 * @implemented
 */
HBITMAP STDCALL
LoadBitmapW(HINSTANCE hInstance, LPCWSTR lpBitmapName)
{
  return(LoadImageW(hInstance, lpBitmapName, IMAGE_BITMAP, 0, 0, 0));
}


/*
 * @implemented
 */
HANDLE WINAPI
CopyImage(HANDLE hnd, UINT type, INT desiredx, INT desiredy, UINT flags)
{
   switch (type)
   {
      case IMAGE_BITMAP:
         {
         	DbgPrint("WARNING:  Incomplete implementation of CopyImage!\n");
         	/* FIXME:  support flags LR_COPYDELETEORG, LR_COPYFROMRESOURCE,
         	   						 LR_COPYRETURNORG, LR_CREATEDIBSECTION,
         	   						 and LR_MONOCHROME; */
            HBITMAP res;
            BITMAP bm;

            if (!GetObjectW(hnd, sizeof(bm), &bm)) return 0;
            bm.bmBits = NULL;
            if ((res = CreateBitmapIndirect(&bm)))
            {
               char *buf = HeapAlloc(GetProcessHeap(), 0, bm.bmWidthBytes * bm.bmHeight);
               GetBitmapBits(hnd, bm.bmWidthBytes * bm.bmHeight, buf);
               SetBitmapBits(res, bm.bmWidthBytes * bm.bmHeight, buf);
               HeapFree(GetProcessHeap(), 0, buf);
            }
            return res;
        }
     case IMAGE_ICON:
        DbgPrint("FIXME: CopyImage doesn't support IMAGE_ICON correctly!\n");
        return CopyIcon(hnd);
     case IMAGE_CURSOR:
        DbgPrint("FIXME: CopyImage doesn't support IMAGE_CURSOR correctly!\n");
        return CopyCursor(hnd);
    }
    return 0;
}
