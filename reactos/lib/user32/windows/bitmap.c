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
/* $Id: bitmap.c,v 1.10 2003/06/03 22:25:37 ekohl Exp $
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

/*forward declerations... actualy in user32\windows\icon.c but usful here****/
HICON ICON_CreateIconFromData(HDC hDC, PVOID ImageData, ICONIMAGE* IconImage, int cxDesired, int cyDesired);
CURSORICONDIRENTRY *CURSORICON_FindBestIcon( CURSORICONDIR *dir, int width, int height, int colors);


/* FUNCTIONS *****************************************************************/

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
  DbgPrint("FIXME: Need support for loading cursor images.\n");
  return(NULL);
}

HANDLE STATIC
LoadIconImage(HINSTANCE hinst, LPCWSTR lpszName, INT width, INT height, UINT fuLoad)
{
  HANDLE hResource;
  HANDLE h2Resource;
  HANDLE hFile;
  HANDLE hSection;
  CURSORICONDIR* IconDIR;
  HDC hScreenDc;
  HANDLE hIcon;
  ULONG HeaderSize;
  ULONG ColourCount;
  PVOID Data;
  CURSORICONDIRENTRY* dirEntry;
  ICONIMAGE* SafeIconImage;
  GRPICONDIR* IconResDir;
  INT id;
  ICONIMAGE *ResIcon;

  if (fuLoad & LR_SHARED)
    DbgPrint("FIXME: need LR_SHARED support Loading icon images\n");

  if (!(fuLoad & LR_LOADFROMFILE))
  {
      if (hinst == NULL)
	  {
	    hinst = GetModuleHandle(L"USER32");		
	  }
      hResource = FindResourceW(hinst, lpszName, RT_GROUP_ICON);
      if (hResource == NULL)
	  {
	    return(NULL);
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

	  h2Resource = FindResource(hinst,
                     MAKEINTRESOURCE(id),
                     MAKEINTRESOURCE(RT_ICON));

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
      return CreateIconFromResourceEx((PBYTE) ResIcon,
                  SizeofResource(hinst, h2Resource), TRUE, 0x00030000,
                  width, height, fuLoad & (LR_DEFAULTCOLOR | LR_MONOCHROME));
  }
  else
  {
      hFile = CreateFile(lpszName,
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

      hSection = CreateFileMapping(hFile,
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
      IconDIR = MapViewOfFile(hSection,
				 FILE_MAP_READ,
				 0,
				 0,
				 0);

      CloseHandle(hSection);
      if (IconDIR == NULL)
	  {
	    return(NULL);
	  }

      //pick the best size.
      dirEntry = (CURSORICONDIRENTRY *)  CURSORICON_FindBestIcon( IconDIR, width, height, 1);


      if (!dirEntry)
	  {
         if (fuLoad & LR_LOADFROMFILE)
		 {
	       UnmapViewOfFile(IconDIR);
		 }
         return(NULL);
	  }

      SafeIconImage = RtlAllocateHeap(RtlGetProcessHeap(), 0, dirEntry->dwBytesInRes); 

      memcpy(SafeIconImage, ((PBYTE)IconDIR) + dirEntry->dwImageOffset, dirEntry->dwBytesInRes);
  }

  //at this point we have a copy of the icon image to play with

  SafeIconImage->icHeader.biHeight = SafeIconImage->icHeader.biHeight /2;

  if (SafeIconImage->icHeader.biSize == sizeof(BITMAPCOREHEADER))
    {
      BITMAPCOREHEADER* Core = (BITMAPCOREHEADER*)SafeIconImage;
      ColourCount = (Core->bcBitCount <= 8) ? (1 << Core->bcBitCount) : 0;
      HeaderSize = sizeof(BITMAPCOREHEADER) + ColourCount * sizeof(RGBTRIPLE);
    }
  else
    {
      ColourCount = SafeIconImage->icHeader.biClrUsed;
      if (ColourCount == 0 && SafeIconImage->icHeader.biBitCount <= 8)
	{
	  ColourCount = 1 << SafeIconImage->icHeader.biBitCount;
	}
      HeaderSize = sizeof(BITMAPINFOHEADER) + ColourCount * sizeof(RGBQUAD);
    }
  
  //make data point to the start of the XOR image data
  Data = (PBYTE)SafeIconImage + HeaderSize;


  //get a handle to the screen dc, the icon we create is going to be compatable with this
  hScreenDc = CreateDCW(L"DISPLAY", NULL, NULL, NULL);
  if (hScreenDc == NULL)
  {
      if (fuLoad & LR_LOADFROMFILE)
	  {
	  	RtlFreeHeap(RtlGetProcessHeap(), 0, SafeIconImage);
        UnmapViewOfFile(IconDIR);
	  }
      return(NULL);
  }

  hIcon = ICON_CreateIconFromData(hScreenDc, Data, SafeIconImage, width, height);
  RtlFreeHeap(RtlGetProcessHeap(), 0, SafeIconImage);
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
  ULONG ColourCount;
  PVOID Data;

  if (!(fuLoad & LR_LOADFROMFILE))
    {
      if (hInstance == NULL)
	{
	  hInstance = GetModuleHandle(L"USER32");		
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
      hFile = CreateFile(lpszName,
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
      hSection = CreateFileMapping(hFile,
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
      ColourCount = (Core->bcBitCount <= 8) ? (1 << Core->bcBitCount) : 0;
      HeaderSize = sizeof(BITMAPCOREHEADER) + ColourCount * sizeof(RGBTRIPLE);
    }
  else
    {
      ColourCount = BitmapInfo->bmiHeader.biClrUsed;
      if (ColourCount == 0 && BitmapInfo->bmiHeader.biBitCount <= 8)
	{
	  ColourCount = 1 << BitmapInfo->bmiHeader.biBitCount;
	}
      HeaderSize = sizeof(BITMAPINFOHEADER) + ColourCount * sizeof(RGBQUAD);
    }
  Data = (PVOID)BitmapInfo + HeaderSize;

  PrivateInfo = RtlAllocateHeap(RtlGetProcessHeap(), 0, HeaderSize);
  if (PrivateInfo == NULL)
    {
      if (fuLoad & LR_LOADFROMFILE)
	{
	  UnmapViewOfFile(BitmapInfo);
	}
      return(NULL);
    }
  memcpy(PrivateInfo, BitmapInfo, HeaderSize);

  /* FIXME: Handle colour conversion and transparency. */

  hScreenDc = CreateDCW(L"DISPLAY", NULL, NULL, NULL);
  if (hScreenDc == NULL)
    {
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

  RtlFreeHeap(RtlGetProcessHeap(), 0, PrivateInfo);
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


HBITMAP STDCALL
LoadBitmapA(HINSTANCE hInstance, LPCSTR lpBitmapName)
{
  return(LoadImageA(hInstance, lpBitmapName, IMAGE_BITMAP, 0, 0, 0));
}

HBITMAP STDCALL
LoadBitmapW(HINSTANCE hInstance, LPCWSTR lpBitmapName)
{
  return(LoadImageW(hInstance, lpBitmapName, IMAGE_BITMAP, 0, 0, 0));
}
