
/*
 *	OLE2 library - 16 bit only interfaces
 *
 *	Copyright 1995	Martin von Loewis
 *      Copyright 1999  Francis Beaudet
 *      Copyright 1999  Noel Borthwick
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"

#include <assert.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#define NONAMELESSUNION
#define NONAMELESSSTRUCT
#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winnls.h"
#include "commctrl.h"
#include "ole2.h"
#include "ole2ver.h"
#include "winerror.h"
#include "wownt32.h"

#include "wine/winbase16.h"
#include "wine/wingdi16.h"
#include "wine/winuser16.h"
#include "ole32_main.h"
#include "ifs.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(ole);
WINE_DECLARE_DEBUG_CHANNEL(accel);

#define HICON_16(h32)		(LOWORD(h32))
#define HICON_32(h16)		((HICON)(ULONG_PTR)(h16))
#define HINSTANCE_32(h16)	((HINSTANCE)(ULONG_PTR)(h16))

/***********************************************************************
 *           RegisterDragDrop (OLE2.35)
 */
HRESULT WINAPI RegisterDragDrop16(
	HWND16 hwnd,
	LPDROPTARGET pDropTarget
) {
	FIXME("(0x%04x,%p),stub!\n",hwnd,pDropTarget);
	return S_OK;
}

/***********************************************************************
 *           RevokeDragDrop (OLE2.36)
 */
HRESULT WINAPI RevokeDragDrop16(
	HWND16 hwnd
) {
	FIXME("(0x%04x),stub!\n",hwnd);
	return S_OK;
}

/******************************************************************************
 * OleMetaFilePictFromIconAndLabel (OLE2.56)
 *
 * Returns a global memory handle to a metafile which contains the icon and
 * label given.
 * I guess the result of that should look somehow like desktop icons.
 * If no hIcon is given, we load the icon via lpszSourceFile and iIconIndex.
 * This code might be wrong at some places.
 */
HGLOBAL16 WINAPI OleMetaFilePictFromIconAndLabel16(
	HICON16 hIcon,
	LPCOLESTR16 lpszLabel,
	LPCOLESTR16 lpszSourceFile,
	UINT16 iIconIndex
) {
    METAFILEPICT16 *mf;
    HGLOBAL16 hmf;
    HDC hdc;

    FIXME("(%04x, '%s', '%s', %d): incorrect metrics, please try to correct them !\n\n\n", hIcon, lpszLabel, lpszSourceFile, iIconIndex);

    if (!hIcon) {
        if (lpszSourceFile) {
	    HINSTANCE16 hInstance = LoadLibrary16(lpszSourceFile);

	    /* load the icon at index from lpszSourceFile */
	    hIcon = HICON_16(LoadIconA(HINSTANCE_32(hInstance), (LPCSTR)(DWORD)iIconIndex));
	    FreeLibrary16(hInstance);
	} else
	    return 0;
    }

    hdc = CreateMetaFileA(NULL);
    DrawIcon(hdc, 0, 0, HICON_32(hIcon)); /* FIXME */
    TextOutA(hdc, 0, 0, lpszLabel, 1); /* FIXME */
    hmf = GlobalAlloc16(0, sizeof(METAFILEPICT16));
    mf = (METAFILEPICT16 *)GlobalLock16(hmf);
    mf->mm = MM_ANISOTROPIC;
    mf->xExt = 20; /* FIXME: bogus */
    mf->yExt = 20; /* dito */
    mf->hMF = CloseMetaFile16(HDC_16(hdc));
    return hmf;
}


/******************************************************************************
 *        CreateItemMoniker	(OLE2.27)
 */
HRESULT WINAPI CreateItemMoniker16(LPCOLESTR16 lpszDelim,LPCOLESTR16 lpszItem,LPMONIKER* ppmk)
{
    FIXME("(%s,%p),stub!\n",lpszDelim,ppmk);
    *ppmk = NULL;
    return E_NOTIMPL;
}


/******************************************************************************
 *        CreateFileMoniker (OLE2.28)
 */
HRESULT WINAPI CreateFileMoniker16(LPCOLESTR16 lpszPathName,LPMONIKER* ppmk)
{
    FIXME("(%s,%p),stub!\n",lpszPathName,ppmk);
    return E_NOTIMPL;
}
