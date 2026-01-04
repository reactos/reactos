/*
 *  OLE32 Initialization
 *
 * Copyright 2000 Huw D M Davies for CodeWeavers
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <stdarg.h>
#include <stdio.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winnls.h"
#include "objbase.h"
#include "ole2.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(ole);

#define HIMETRIC_INCHES 2540

/***********************************************************************
 *		OleMetafilePictFromIconAndLabel (OLE32.@)
 */
HGLOBAL WINAPI OleMetafilePictFromIconAndLabel(HICON hIcon, LPOLESTR lpszLabel,
                                               LPOLESTR lpszSourceFile, UINT iIconIndex)
{
	METAFILEPICT mfp;
	HDC hdc;
	HGLOBAL hmem = NULL;
	LPVOID mfdata;
	static const char szIconOnly[] = "IconOnly";
	SIZE text_size = { 0, 0 };
	INT width;
	INT icon_width;
	INT icon_height;
	INT label_offset;
	HDC hdcScreen;
	LOGFONTW lf;
	HFONT font;

	TRACE("%p %p %s %d\n", hIcon, lpszLabel, debugstr_w(lpszSourceFile), iIconIndex);

	if( !hIcon )
		return NULL;

	if (!SystemParametersInfoW(SPI_GETICONTITLELOGFONT, sizeof(lf), &lf, 0))
		return NULL;

	font = CreateFontIndirectW(&lf);
	if (!font)
		return NULL;

	hdc = CreateMetaFileW(NULL);
	if( !hdc )
	{
		DeleteObject(font);
		return NULL;
	}

	SelectObject(hdc, font);

	ExtEscape(hdc, MFCOMMENT, sizeof(szIconOnly), szIconOnly, 0, NULL);

	icon_width = GetSystemMetrics(SM_CXICON);
	icon_height = GetSystemMetrics(SM_CYICON);
	/* FIXME: should we give the label a bit of padding here? */
	label_offset = icon_height;
	if (lpszLabel)
	{
		HFONT screen_old_font;
		/* metafile DCs don't support GetTextExtentPoint32, so size the font
		 * using the desktop window DC */
		hdcScreen = GetDC(NULL);
		screen_old_font = SelectObject(hdcScreen, font);
		GetTextExtentPoint32W(hdcScreen, lpszLabel, lstrlenW(lpszLabel), &text_size);
		SelectObject(hdcScreen, screen_old_font);
		ReleaseDC(NULL, hdcScreen);

		width = 3 * icon_width;
	}
	else
		width = icon_width;

	SetMapMode(hdc, MM_ANISOTROPIC);
	SetWindowOrgEx(hdc, 0, 0, NULL);
	SetWindowExtEx(hdc, width, label_offset + text_size.cy, NULL);

	/* draw the icon centered */
	DrawIcon(hdc, (width-icon_width) / 2, 0, hIcon);
	if(lpszLabel)
		/* draw the label centered too, if provided */
		TextOutW(hdc, (width-text_size.cx) / 2, label_offset, lpszLabel, lstrlenW(lpszLabel));

	if (lpszSourceFile)
	{
		char szIconIndex[10];
		int path_length = WideCharToMultiByte(CP_ACP,0,lpszSourceFile,-1,NULL,0,NULL,NULL);
		if (path_length > 1)
		{
			char * szPath = CoTaskMemAlloc(path_length * sizeof(CHAR));
			if (szPath)
			{
				WideCharToMultiByte(CP_ACP,0,lpszSourceFile,-1,szPath,path_length,NULL,NULL);
				ExtEscape(hdc, MFCOMMENT, path_length, szPath, 0, NULL);
				CoTaskMemFree(szPath);
			}
		}
		snprintf(szIconIndex, 10, "%u", iIconIndex);
		ExtEscape(hdc, MFCOMMENT, strlen(szIconIndex)+1, szIconIndex, 0, NULL);
	}

	mfp.mm = MM_ANISOTROPIC;
	hdcScreen = GetDC(NULL);
	mfp.xExt = MulDiv(width, HIMETRIC_INCHES, GetDeviceCaps(hdcScreen, LOGPIXELSX));
	mfp.yExt = MulDiv(label_offset + text_size.cy, HIMETRIC_INCHES, GetDeviceCaps(hdcScreen, LOGPIXELSY));
	ReleaseDC(NULL, hdcScreen);
	mfp.hMF = CloseMetaFile(hdc);
	DeleteObject(font);
	if( !mfp.hMF )
		return NULL;

	hmem = GlobalAlloc( GMEM_MOVEABLE, sizeof(mfp) );
	if( !hmem )
	{
		DeleteMetaFile(mfp.hMF);
		return NULL;
	}

	mfdata = GlobalLock( hmem );
	if( !mfdata )
	{
		GlobalFree( hmem );
		DeleteMetaFile(mfp.hMF);
		return NULL;
	}

	memcpy(mfdata,&mfp,sizeof(mfp));
	GlobalUnlock( hmem );

	TRACE("returning %p\n",hmem);

	return hmem;
}

/******************************************************************************
 *		IsValidInterface	[OLE32.@]
 *
 * Determines whether a pointer is a valid interface.
 *
 * PARAMS
 *  punk [I] Interface to be tested.
 *
 * RETURNS
 *  TRUE, if the passed pointer is a valid interface, or FALSE otherwise.
 */
BOOL WINAPI IsValidInterface(LPUNKNOWN punk)
{
	return !(IsBadReadPtr(punk,4) ||
                 IsBadReadPtr(punk->lpVtbl,4) ||
                 IsBadReadPtr(punk->lpVtbl->QueryInterface,9) ||
                 IsBadCodePtr((FARPROC)punk->lpVtbl->QueryInterface));
}
