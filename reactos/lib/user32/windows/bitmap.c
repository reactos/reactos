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
/* $Id: bitmap.c,v 1.6 2002/09/30 21:21:38 chorns Exp $
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
      Handle = LoadImageW(hinst, lpszWName, uType, cxDesired,
			  cyDesired, fuLoad);
    }
  return(Handle);
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
	DbgPrint("FIXME: Need support for loading cursors.\n");
	return(NULL);
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
