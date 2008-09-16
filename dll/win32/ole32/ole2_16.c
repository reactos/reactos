
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
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
#include "ole2.h"
#include "winerror.h"

#include "wine/winbase16.h"
#include "wine/wingdi16.h"
#include "wine/winuser16.h"
#include "ifs.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(ole);

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
    METAFILEPICT16 *mf16;
    HGLOBAL16 hmf16;
    HMETAFILE hmf;
    INT mfSize;
    HDC hdc;

    if (!hIcon) {
        if (lpszSourceFile) {
	    HINSTANCE16 hInstance = LoadLibrary16(lpszSourceFile);

	    /* load the icon at index from lpszSourceFile */
	    hIcon = HICON_16(LoadIconA(HINSTANCE_32(hInstance), (LPCSTR)(DWORD)iIconIndex));
	    FreeLibrary16(hInstance);
	} else
	    return 0;
    }

    FIXME("(%04x, '%s', '%s', %d): incorrect metrics, please try to correct them !\n", 
          hIcon, lpszLabel, lpszSourceFile, iIconIndex);

    hdc = CreateMetaFileW(NULL);
    DrawIcon(hdc, 0, 0, HICON_32(hIcon)); /* FIXME */
    TextOutA(hdc, 0, 0, lpszLabel, 1); /* FIXME */
    hmf = CloseMetaFile(hdc);

    hmf16 = GlobalAlloc16(0, sizeof(METAFILEPICT16));
    mf16 = (METAFILEPICT16 *)GlobalLock16(hmf16);
    mf16->mm = MM_ANISOTROPIC;
    mf16->xExt = 20; /* FIXME: bogus */
    mf16->yExt = 20; /* ditto */
    mfSize = GetMetaFileBitsEx(hmf, 0, 0);
    mf16->hMF = GlobalAlloc16(GMEM_MOVEABLE, mfSize);
    if(mf16->hMF)
    {
        GetMetaFileBitsEx(hmf, mfSize, GlobalLock16(mf16->hMF));
        GlobalUnlock16(mf16->hMF);
    }
    return hmf16;
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

/******************************************************************************
 *        OleSetMenuDescriptor (OLE2.41)
 *
 * PARAMS
 *  hOleMenu  FIXME: Should probably be an HOLEMENU16.
 */
HRESULT WINAPI OleSetMenuDescriptor16(
    HOLEMENU               hOleMenu,
    HWND16                 hwndFrame,
    HWND16                 hwndActiveObject,
    LPOLEINPLACEFRAME        lpFrame,
    LPOLEINPLACEACTIVEOBJECT lpActiveObject)
{
    FIXME("(%p, %x, %x, %p, %p), stub!\n", hOleMenu, hwndFrame, hwndActiveObject, lpFrame, lpActiveObject);
    return E_NOTIMPL;
}

/******************************************************************************
 *              IsValidInterface        [COMPOBJ.23]
 *
 * Determines whether a pointer is a valid interface.
 *
 * PARAMS
 *  punk [I] Interface to be tested.
 *
 * RETURNS
 *  TRUE, if the passed pointer is a valid interface, or FALSE otherwise.
 */
BOOL WINAPI IsValidInterface16(SEGPTR punk)
{
	DWORD **ptr;

	if (IsBadReadPtr16(punk,4))
		return FALSE;
	ptr = MapSL(punk);
	if (IsBadReadPtr16((SEGPTR)ptr[0],4))	/* check vtable ptr */
		return FALSE;
	ptr = MapSL((SEGPTR)ptr[0]);		/* ptr to first method */
	if (IsBadReadPtr16((SEGPTR)ptr[0],2))
		return FALSE;
	return TRUE;
}

/******************************************************************************
 *              OleLoad        [OLE2.12]
 *
 * PARAMS
 *  pStg Segmented LPSTORAGE pointer.
 *  pClientSite Segmented LPOLECLIENTSITE pointer.
 */
HRESULT WINAPI OleLoad16(
    SEGPTR		pStg,
    REFIID            	riid,
    SEGPTR		pClientSite,
    LPVOID*		ppvObj)
{
  FIXME("(%x,%s,%x,%p), stub!\n", pStg, debugstr_guid(riid), pClientSite, ppvObj);
  return E_NOTIMPL;
}

/******************************************************************************
 *              OleDoAutoConvert        [OLE2.79]
 */
HRESULT WINAPI OleDoAutoConvert16(LPSTORAGE pStg, LPCLSID pClsidNew)
{
    FIXME("(%p,%p) : stub\n",pStg,pClsidNew);
    return E_NOTIMPL;
}

/***********************************************************************
 *           OleSetClipboard                            [OLE2.49]
 */
HRESULT WINAPI OleSetClipboard16(IDataObject* pDataObj)
{
  FIXME("(%p): stub\n", pDataObj);
  return S_OK;
}

/***********************************************************************
 *           OleGetClipboard                            [OLE2.50]
 */
HRESULT WINAPI OleGetClipboard16(IDataObject** ppDataObj)
{
  FIXME("(%p): stub\n", ppDataObj);
  return E_NOTIMPL;
}

/***********************************************************************
 *           OleFlushClipboard   [OLE2.76]
 */

HRESULT WINAPI OleFlushClipboard16(void)
{
  return OleFlushClipboard();
}
