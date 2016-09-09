/*
 * Wordpad implementation - Richedit OLE callback implementation
 *
 * Copyright 2010 by Dylan Smith <dylan.ah.smith@gmail.com>
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

#define COBJMACROS

#define WIN32_NO_STATUS
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <richedit.h>
#include <richole.h>
#include <wine/debug.h>

#include "wordpad.h"

WINE_DEFAULT_DEBUG_CHANNEL(wordpad);

struct IRichEditOleCallbackImpl {
    const IRichEditOleCallbackVtbl *vtbl;
    IStorage *stg;
    int item_num;
};

struct IRichEditOleCallbackImpl olecallback;

static HRESULT STDMETHODCALLTYPE RichEditOleCallback_QueryInterface(
    IRichEditOleCallback* This,
    REFIID riid,
    void **ppvObject)
{
    WINE_TRACE("(%p, %s, %p)\n", This, wine_dbgstr_guid(riid), ppvObject);
    if (IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_IRichEditOleCallback))
    {
        *ppvObject = This;
        return S_OK;
    }
    WINE_FIXME("Unknown interface: %s\n", wine_dbgstr_guid(riid));
    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE RichEditOleCallback_AddRef(
    IRichEditOleCallback* This)
{
    WINE_TRACE("(%p)\n", This);
    /* singleton */
    return 1;
}

static ULONG STDMETHODCALLTYPE RichEditOleCallback_Release(
    IRichEditOleCallback* This)
{
    WINE_TRACE("(%p)\n", This);
    return 1;
}

/*** IRichEditOleCallback methods ***/
static HRESULT STDMETHODCALLTYPE RichEditOleCallback_GetNewStorage(
    IRichEditOleCallback* This,
    LPSTORAGE *lplpstg)
{
    WCHAR name[32];
    static const WCHAR template[] = {'R','E','O','L','E','_','%','u','\0'};

    WINE_TRACE("(%p, %p)\n", This, lplpstg);
    wsprintfW(name, template, olecallback.item_num++);
    return IStorage_CreateStorage(olecallback.stg, name,
                      STGM_READWRITE | STGM_SHARE_EXCLUSIVE | STGM_CREATE,
                      0, 0, lplpstg);
}

static HRESULT STDMETHODCALLTYPE RichEditOleCallback_GetInPlaceContext(
    IRichEditOleCallback* This,
    LPOLEINPLACEFRAME *lplpFrame,
    LPOLEINPLACEUIWINDOW *lplpDoc,
    LPOLEINPLACEFRAMEINFO lpFrameInfo)
{
    WINE_FIXME("(%p, %p, %p, %p) stub\n", This, lplpFrame, lplpDoc, lpFrameInfo);
    return E_INVALIDARG;
}

static HRESULT STDMETHODCALLTYPE RichEditOleCallback_ShowContainerUI(
    IRichEditOleCallback* This,
    BOOL fShow)
{
    WINE_TRACE("(%p, %d)\n", This, fShow);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE RichEditOleCallback_QueryInsertObject(
    IRichEditOleCallback* This,
    LPCLSID lpclsid,
    LPSTORAGE lpstg,
    LONG cp)
{
    WINE_TRACE("(%p, %p, %p, %d)\n", This, lpclsid, lpstg, cp);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE RichEditOleCallback_DeleteObject(
    IRichEditOleCallback* This,
    LPOLEOBJECT lpoleobj)
{
    WINE_TRACE("(%p, %p)\n", This, lpoleobj);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE RichEditOleCallback_QueryAcceptData(
    IRichEditOleCallback* This,
    LPDATAOBJECT lpdataobj,
    CLIPFORMAT *lpcfFormat,
    DWORD reco,
    BOOL fReally,
    HGLOBAL hMetaPict)
{
    WINE_TRACE("(%p, %p, %p, %x, %d, %p)\n",
               This, lpdataobj, lpcfFormat, reco, fReally, hMetaPict);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE RichEditOleCallback_ContextSensitiveHelp(
    IRichEditOleCallback* This,
    BOOL fEnterMode)
{
    WINE_TRACE("(%p, %d)\n", This, fEnterMode);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE RichEditOleCallback_GetClipboardData(
    IRichEditOleCallback* This,
    CHARRANGE *lpchrg,
    DWORD reco,
    LPDATAOBJECT *lplpdataobj)
{
    WINE_TRACE("(%p, %p, %x, %p)\n", This, lpchrg, reco, lplpdataobj);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE RichEditOleCallback_GetDragDropEffect(
    IRichEditOleCallback* This,
    BOOL fDrag,
    DWORD grfKeyState,
    LPDWORD pdwEffect)
{
    WINE_TRACE("(%p, %d, %x, %p)\n", This, fDrag, grfKeyState, pdwEffect);
    if (pdwEffect)
          *pdwEffect = DROPEFFECT_COPY;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE RichEditOleCallback_GetContextMenu(
    IRichEditOleCallback* This,
    WORD seltype,
    LPOLEOBJECT lpoleobj,
    CHARRANGE *lpchrg,
    HMENU *lphmenu)
{
    HINSTANCE hInstance = GetModuleHandleW(0);
    HMENU hPopupMenu = LoadMenuW(hInstance, MAKEINTRESOURCEW(IDM_POPUP));

    WINE_TRACE("(%p, %x, %p, %p, %p)\n",
               This, seltype, lpoleobj, lpchrg, lphmenu);

    *lphmenu = GetSubMenu(hPopupMenu, 0);
    return S_OK;
}

struct IRichEditOleCallbackVtbl olecallbackVtbl = {
    RichEditOleCallback_QueryInterface,
    RichEditOleCallback_AddRef,
    RichEditOleCallback_Release,
    RichEditOleCallback_GetNewStorage,
    RichEditOleCallback_GetInPlaceContext,
    RichEditOleCallback_ShowContainerUI,
    RichEditOleCallback_QueryInsertObject,
    RichEditOleCallback_DeleteObject,
    RichEditOleCallback_QueryAcceptData,
    RichEditOleCallback_ContextSensitiveHelp,
    RichEditOleCallback_GetClipboardData,
    RichEditOleCallback_GetDragDropEffect,
    RichEditOleCallback_GetContextMenu
};

struct IRichEditOleCallbackImpl olecallback = {
    &olecallbackVtbl, NULL, 0
};

HRESULT setup_richedit_olecallback(HWND hEditorWnd)
{
    HRESULT hr = StgCreateDocfile(NULL,
          STGM_READWRITE | STGM_SHARE_EXCLUSIVE | STGM_DELETEONRELEASE,
          0, &olecallback.stg);

    SendMessageW(hEditorWnd, EM_SETOLECALLBACK, 0, (LPARAM)&olecallback);
    return hr;
}
