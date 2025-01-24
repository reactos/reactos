/*
 * Active Template Library ActiveX functions (atl.dll)
 *
 * Copyright 2006 Andrey Turkin
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

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "winuser.h"
#include "wine/debug.h"
#include "wine/heap.h"
#include "objbase.h"
#include "objidl.h"
#include "ole2.h"
#include "exdisp.h"
#include "wine/atlbase.h"
#include "atliface.h"
#include "wine/atlwin.h"
#include "shlwapi.h"


WINE_DEFAULT_DEBUG_CHANNEL(atl);

typedef struct IOCS {
    IOleClientSite            IOleClientSite_iface;
    IOleContainer             IOleContainer_iface;
    IOleInPlaceSiteWindowless IOleInPlaceSiteWindowless_iface;
    IOleInPlaceFrame          IOleInPlaceFrame_iface;
    IOleControlSite           IOleControlSite_iface;

    LONG ref;
    HWND hWnd;
    IOleObject *control;
    RECT size;
    WNDPROC OrigWndProc;
    BOOL fActive, fInPlace, fWindowless;
} IOCS;

static const WCHAR wine_atl_iocsW[] = {'_','_','W','I','N','E','_','A','T','L','_','I','O','C','S','\0'};

/**********************************************************************
 * AtlAxWin class window procedure
 */
static LRESULT CALLBACK AtlAxWin_wndproc( HWND hWnd, UINT wMsg, WPARAM wParam, LPARAM lParam )
{
    if ( wMsg == WM_CREATE )
    {
            DWORD len = GetWindowTextLengthW( hWnd ) + 1;
            WCHAR *ptr = HeapAlloc( GetProcessHeap(), 0, len*sizeof(WCHAR) );
            if (!ptr)
                return 1;
            GetWindowTextW( hWnd, ptr, len );
            AtlAxCreateControlEx( ptr, hWnd, NULL, NULL, NULL, NULL, NULL );
            HeapFree( GetProcessHeap(), 0, ptr );
            return 0;
    }
    return DefWindowProcW( hWnd, wMsg, wParam, lParam );
}

/***********************************************************************
 *           AtlAxWinInit          [atl100.@]
 * Initializes the control-hosting code: registering the AtlAxWin,
 * AtlAxWin7 and AtlAxWinLic7 window classes and some messages.
 *
 * RETURNS
 *  TRUE or FALSE
 */

BOOL WINAPI AtlAxWinInit(void)
{
    WNDCLASSEXW wcex;

#if _ATL_VER <= _ATL_VER_30
#define ATL_NAME_SUFFIX 0
#elif _ATL_VER == _ATL_VER_80
#define ATL_NAME_SUFFIX '8','0',0
#elif _ATL_VER == _ATL_VER_90
#define ATL_NAME_SUFFIX '9','0',0
#elif _ATL_VER == _ATL_VER_100
#define ATL_NAME_SUFFIX '1','0','0',0
#elif _ATL_VER == _ATL_VER_110
#define ATL_NAME_SUFFIX '1','1','0',0
#else
#error Unsupported version
#endif

    static const WCHAR AtlAxWinW[] = {'A','t','l','A','x','W','i','n',ATL_NAME_SUFFIX};

    FIXME("version %04x semi-stub\n", _ATL_VER);

    if ( FAILED( OleInitialize(NULL) ) )
        return FALSE;

    wcex.cbSize        = sizeof(wcex);
    wcex.style         = CS_GLOBALCLASS | (_ATL_VER > _ATL_VER_30 ? CS_DBLCLKS : 0);
    wcex.cbClsExtra    = 0;
    wcex.cbWndExtra    = 0;
    wcex.hInstance     = GetModuleHandleW( NULL );
    wcex.hIcon         = NULL;
    wcex.hCursor       = NULL;
    wcex.hbrBackground = NULL;
    wcex.lpszMenuName  = NULL;
    wcex.hIconSm       = 0;

    wcex.lpfnWndProc   = AtlAxWin_wndproc;
    wcex.lpszClassName = AtlAxWinW;
    if ( !RegisterClassExW( &wcex ) )
        return FALSE;

    if(_ATL_VER > _ATL_VER_30) {
        static const WCHAR AtlAxWinLicW[] = {'A','t','l','A','x','W','i','n','L','i','c',ATL_NAME_SUFFIX};

        wcex.lpszClassName = AtlAxWinLicW;
        if ( !RegisterClassExW( &wcex ) )
            return FALSE;
    }

    return TRUE;
}

/***********************************************************************
 *  Atl container component implementation
 */

/******      IOleClientSite    *****/
static inline IOCS *impl_from_IOleClientSite(IOleClientSite *iface)
{
    return CONTAINING_RECORD(iface, IOCS, IOleClientSite_iface);
}

static HRESULT IOCS_Detach( IOCS *This ) /* remove subclassing */
{
    if ( This->hWnd )
    {
        SetWindowLongPtrW( This->hWnd, GWLP_WNDPROC, (ULONG_PTR) This->OrigWndProc );
        RemovePropW( This->hWnd, wine_atl_iocsW);
        This->hWnd = NULL;
    }
    if ( This->control )
    {
        IOleObject *control = This->control;

        This->control = NULL;
        IOleObject_Close( control, OLECLOSE_NOSAVE );
        IOleObject_SetClientSite( control, NULL );
        IOleObject_Release( control );
    }
    return S_OK;
}

static HRESULT WINAPI OleClientSite_QueryInterface(IOleClientSite *iface, REFIID riid, void **ppv)
{
    IOCS *This = impl_from_IOleClientSite(iface);

    TRACE("(%p)->(%s %p)\n", This, debugstr_guid(riid), ppv);

    *ppv = NULL;

    if (IsEqualIID(&IID_IUnknown, riid) ||
        IsEqualIID(&IID_IOleClientSite, riid))
    {
        *ppv = iface;
    }
    else if (IsEqualIID(&IID_IOleContainer, riid))
    {
        *ppv = &This->IOleContainer_iface;
    }
    else if (IsEqualIID(&IID_IOleInPlaceSite, riid) ||
             IsEqualIID(&IID_IOleInPlaceSiteEx, riid) ||
             IsEqualIID(&IID_IOleInPlaceSiteWindowless, riid))
    {
        *ppv = &This->IOleInPlaceSiteWindowless_iface;
    }
    else if (IsEqualIID(&IID_IOleInPlaceFrame, riid))
    {
        *ppv = &This->IOleInPlaceFrame_iface;
    }
    else if (IsEqualIID(&IID_IOleControlSite, riid))
    {
        *ppv = &This->IOleControlSite_iface;
    }

    if (*ppv)
    {
        IOleClientSite_AddRef(iface);
        return S_OK;
    }

    WARN("unsupported interface %s\n", debugstr_guid(riid));
    return E_NOINTERFACE;
}

static ULONG WINAPI OleClientSite_AddRef(IOleClientSite *iface)
{
    IOCS *This = impl_from_IOleClientSite(iface);
    ULONG ref = InterlockedIncrement(&This->ref);
    TRACE("(%p)->(%d)\n", This, ref);
    return ref;
}

static ULONG WINAPI OleClientSite_Release(IOleClientSite *iface)
{
    IOCS *This = impl_from_IOleClientSite(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p)->(%d)\n", This, ref);

    if (!ref)
    {
        IOCS_Detach( This );
        HeapFree( GetProcessHeap(), 0, This );
    }

    return ref;
}

static HRESULT WINAPI OleClientSite_SaveObject(IOleClientSite *iface)
{
    IOCS *This = impl_from_IOleClientSite(iface);
    FIXME( "(%p) - stub\n", This );
    return E_NOTIMPL;
}

static HRESULT WINAPI OleClientSite_GetMoniker(IOleClientSite *iface, DWORD dwAssign, DWORD dwWhichMoniker, IMoniker **ppmk)
{
    IOCS *This = impl_from_IOleClientSite(iface);

    FIXME( "(%p, 0x%x, 0x%x, %p)\n", This, dwAssign, dwWhichMoniker, ppmk );
    return E_NOTIMPL;
}

static HRESULT WINAPI OleClientSite_GetContainer(IOleClientSite *iface, IOleContainer **container)
{
    IOCS *This = impl_from_IOleClientSite(iface);
    TRACE("(%p, %p)\n", This, container);
    return IOleClientSite_QueryInterface(iface, &IID_IOleContainer, (void**)container);
}

static HRESULT WINAPI OleClientSite_ShowObject(IOleClientSite *iface)
{
    IOCS *This = impl_from_IOleClientSite(iface);
    FIXME( "(%p) - stub\n", This );
    return S_OK;
}

static HRESULT WINAPI OleClientSite_OnShowWindow(IOleClientSite *iface, BOOL fShow)
{
    IOCS *This = impl_from_IOleClientSite(iface);
    FIXME( "(%p, %s) - stub\n", This, fShow ? "TRUE" : "FALSE" );
    return E_NOTIMPL;
}

static HRESULT WINAPI OleClientSite_RequestNewObjectLayout(IOleClientSite *iface)
{
    IOCS *This = impl_from_IOleClientSite(iface);
    FIXME( "(%p) - stub\n", This );
    return E_NOTIMPL;
}


/******      IOleContainer     *****/
static inline IOCS *impl_from_IOleContainer(IOleContainer *iface)
{
    return CONTAINING_RECORD(iface, IOCS, IOleContainer_iface);
}

static HRESULT WINAPI OleContainer_QueryInterface( IOleContainer* iface, REFIID riid, void** ppv)
{
    IOCS *This = impl_from_IOleContainer(iface);
    return IOleClientSite_QueryInterface(&This->IOleClientSite_iface, riid, ppv);
}

static ULONG WINAPI OleContainer_AddRef(IOleContainer* iface)
{
    IOCS *This = impl_from_IOleContainer(iface);
    return IOleClientSite_AddRef(&This->IOleClientSite_iface);
}

static ULONG WINAPI OleContainer_Release(IOleContainer* iface)
{
    IOCS *This = impl_from_IOleContainer(iface);
    return IOleClientSite_Release(&This->IOleClientSite_iface);
}

static HRESULT WINAPI OleContainer_ParseDisplayName(IOleContainer* iface, IBindCtx* pbc,
        LPOLESTR pszDisplayName, ULONG* pchEaten, IMoniker** ppmkOut)
{
    IOCS *This = impl_from_IOleContainer(iface);
    FIXME( "(%p,%p,%s,%p,%p) - stub\n", This, pbc, debugstr_w(pszDisplayName), pchEaten, ppmkOut );
    return E_NOTIMPL;
}

static HRESULT WINAPI OleContainer_EnumObjects(IOleContainer* iface, DWORD grfFlags, IEnumUnknown** ppenum)
{
    IOCS *This = impl_from_IOleContainer(iface);
    FIXME( "(%p, %u, %p) - stub\n", This, grfFlags, ppenum );
    return E_NOTIMPL;
}

static HRESULT WINAPI OleContainer_LockContainer(IOleContainer* iface, BOOL fLock)
{
    IOCS *This = impl_from_IOleContainer(iface);
    FIXME( "(%p, %s) - stub\n", This, fLock?"TRUE":"FALSE" );
    return E_NOTIMPL;
}


/******    IOleInPlaceSiteWindowless   *******/
static inline IOCS *impl_from_IOleInPlaceSiteWindowless(IOleInPlaceSiteWindowless *iface)
{
    return CONTAINING_RECORD(iface, IOCS, IOleInPlaceSiteWindowless_iface);
}

static HRESULT WINAPI OleInPlaceSiteWindowless_QueryInterface(IOleInPlaceSiteWindowless *iface, REFIID riid, void **ppv)
{
    IOCS *This = impl_from_IOleInPlaceSiteWindowless(iface);
    return IOleClientSite_QueryInterface(&This->IOleClientSite_iface, riid, ppv);
}

static ULONG WINAPI OleInPlaceSiteWindowless_AddRef(IOleInPlaceSiteWindowless *iface)
{
    IOCS *This = impl_from_IOleInPlaceSiteWindowless(iface);
    return IOleClientSite_AddRef(&This->IOleClientSite_iface);
}

static ULONG WINAPI OleInPlaceSiteWindowless_Release(IOleInPlaceSiteWindowless *iface)
{
    IOCS *This = impl_from_IOleInPlaceSiteWindowless(iface);
    return IOleClientSite_Release(&This->IOleClientSite_iface);
}

static HRESULT WINAPI OleInPlaceSiteWindowless_GetWindow(IOleInPlaceSiteWindowless* iface, HWND* phwnd)
{
    IOCS *This = impl_from_IOleInPlaceSiteWindowless(iface);

    TRACE("(%p,%p)\n", This, phwnd);
    *phwnd = This->hWnd;
    return S_OK;
}

static HRESULT WINAPI OleInPlaceSiteWindowless_ContextSensitiveHelp(IOleInPlaceSiteWindowless* iface, BOOL fEnterMode)
{
    IOCS *This = impl_from_IOleInPlaceSiteWindowless(iface);
    FIXME("(%p,%d) - stub\n", This, fEnterMode);
    return E_NOTIMPL;
}

static HRESULT WINAPI OleInPlaceSiteWindowless_CanInPlaceActivate(IOleInPlaceSiteWindowless *iface)
{
    IOCS *This = impl_from_IOleInPlaceSiteWindowless(iface);
    TRACE("(%p)\n", This);
    return S_OK;
}

static HRESULT WINAPI OleInPlaceSiteWindowless_OnInPlaceActivate(IOleInPlaceSiteWindowless *iface)
{
    IOCS *This = impl_from_IOleInPlaceSiteWindowless(iface);

    TRACE("(%p)\n", This);

    This->fInPlace = TRUE;
    return S_OK;
}

static HRESULT WINAPI OleInPlaceSiteWindowless_OnUIActivate(IOleInPlaceSiteWindowless *iface)
{
    IOCS *This = impl_from_IOleInPlaceSiteWindowless(iface);

    TRACE("(%p)\n", This);

    return S_OK;
}
static HRESULT WINAPI OleInPlaceSiteWindowless_GetWindowContext(IOleInPlaceSiteWindowless *iface,
        IOleInPlaceFrame **frame, IOleInPlaceUIWindow **ppDoc, LPRECT lprcPosRect,
        LPRECT lprcClipRect, LPOLEINPLACEFRAMEINFO lpFrameInfo)
{
    IOCS *This = impl_from_IOleInPlaceSiteWindowless(iface);

    TRACE("(%p,%p,%p,%p,%p,%p)\n", This, frame, ppDoc, lprcPosRect, lprcClipRect, lpFrameInfo);

    if ( lprcClipRect )
        *lprcClipRect = This->size;
    if ( lprcPosRect )
        *lprcPosRect = This->size;

    if ( frame )
    {
        *frame = &This->IOleInPlaceFrame_iface;
        IOleInPlaceFrame_AddRef(*frame);
    }

    if ( ppDoc )
        *ppDoc = NULL;

    if ( lpFrameInfo )
    {
        lpFrameInfo->fMDIApp = FALSE;
        lpFrameInfo->hwndFrame = This->hWnd;
        lpFrameInfo->haccel = NULL;
        lpFrameInfo->cAccelEntries = 0;
    }

    return S_OK;
}

static HRESULT WINAPI OleInPlaceSiteWindowless_Scroll(IOleInPlaceSiteWindowless *iface, SIZE scrollExtent)
{
    IOCS *This = impl_from_IOleInPlaceSiteWindowless(iface);
    FIXME("(%p) - stub\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI OleInPlaceSiteWindowless_OnUIDeactivate(IOleInPlaceSiteWindowless *iface, BOOL fUndoable)
{
    IOCS *This = impl_from_IOleInPlaceSiteWindowless(iface);
    FIXME("(%p,%d) - stub\n", This, fUndoable);
    return E_NOTIMPL;
}

static HRESULT WINAPI OleInPlaceSiteWindowless_OnInPlaceDeactivate(IOleInPlaceSiteWindowless *iface)
{
    IOCS *This = impl_from_IOleInPlaceSiteWindowless(iface);

    TRACE("(%p)\n", This);

    This->fInPlace = This->fWindowless = FALSE;
    return S_OK;
}

static HRESULT WINAPI OleInPlaceSiteWindowless_DiscardUndoState(IOleInPlaceSiteWindowless *iface)
{
    IOCS *This = impl_from_IOleInPlaceSiteWindowless(iface);
    FIXME("(%p) - stub\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI OleInPlaceSiteWindowless_DeactivateAndUndo(IOleInPlaceSiteWindowless *iface)
{
    IOCS *This = impl_from_IOleInPlaceSiteWindowless(iface);
    FIXME("(%p) - stub\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI OleInPlaceSiteWindowless_OnPosRectChange(IOleInPlaceSiteWindowless *iface, LPCRECT lprcPosRect)
{
    IOCS *This = impl_from_IOleInPlaceSiteWindowless(iface);
    FIXME("(%p,%p) - stub\n", This, lprcPosRect);
    return E_NOTIMPL;
}

static HRESULT WINAPI OleInPlaceSiteWindowless_OnInPlaceActivateEx( IOleInPlaceSiteWindowless *iface, BOOL* pfNoRedraw, DWORD dwFlags)
{
    IOCS *This = impl_from_IOleInPlaceSiteWindowless(iface);

    TRACE("\n");

    This->fActive = This->fInPlace = TRUE;
    if ( dwFlags & ACTIVATE_WINDOWLESS )
        This->fWindowless = TRUE;
    return S_OK;
}

static HRESULT WINAPI OleInPlaceSiteWindowless_OnInPlaceDeactivateEx( IOleInPlaceSiteWindowless *iface, BOOL fNoRedraw)
{
    IOCS *This = impl_from_IOleInPlaceSiteWindowless(iface);

    TRACE("\n");

    This->fActive = This->fInPlace = This->fWindowless = FALSE;
    return S_OK;
}
static HRESULT WINAPI OleInPlaceSiteWindowless_RequestUIActivate( IOleInPlaceSiteWindowless *iface)
{
    FIXME("\n");
    return E_NOTIMPL;
}
static HRESULT WINAPI OleInPlaceSiteWindowless_CanWindowlessActivate( IOleInPlaceSiteWindowless *iface)
{
    FIXME("\n");
    return S_OK;
}
static HRESULT WINAPI OleInPlaceSiteWindowless_GetCapture( IOleInPlaceSiteWindowless *iface)
{
    FIXME("\n");
    return E_NOTIMPL;
}
static HRESULT WINAPI OleInPlaceSiteWindowless_SetCapture( IOleInPlaceSiteWindowless *iface, BOOL fCapture)
{
    FIXME("\n");
    return E_NOTIMPL;
}
static HRESULT WINAPI OleInPlaceSiteWindowless_GetFocus( IOleInPlaceSiteWindowless *iface)
{
    FIXME("\n");
    return E_NOTIMPL;
}
static HRESULT WINAPI OleInPlaceSiteWindowless_SetFocus( IOleInPlaceSiteWindowless *iface, BOOL fFocus)
{
    FIXME("\n");
    return E_NOTIMPL;
}
static HRESULT WINAPI OleInPlaceSiteWindowless_GetDC( IOleInPlaceSiteWindowless *iface, LPCRECT pRect, DWORD grfFlags, HDC* phDC)
{
    FIXME("\n");
    return E_NOTIMPL;
}
static HRESULT WINAPI OleInPlaceSiteWindowless_ReleaseDC( IOleInPlaceSiteWindowless *iface, HDC hDC)
{
    FIXME("\n");
    return E_NOTIMPL;
}
static HRESULT WINAPI OleInPlaceSiteWindowless_InvalidateRect( IOleInPlaceSiteWindowless *iface, LPCRECT pRect, BOOL fErase)
{
    FIXME("\n");
    return E_NOTIMPL;
}
static HRESULT WINAPI OleInPlaceSiteWindowless_InvalidateRgn( IOleInPlaceSiteWindowless *iface, HRGN hRGN, BOOL fErase)
{
    FIXME("\n");
    return E_NOTIMPL;
}
static HRESULT WINAPI OleInPlaceSiteWindowless_ScrollRect( IOleInPlaceSiteWindowless *iface, INT dx, INT dy, LPCRECT pRectScroll, LPCRECT pRectClip)
{
    FIXME("\n");
    return E_NOTIMPL;
}
static HRESULT WINAPI OleInPlaceSiteWindowless_AdjustRect( IOleInPlaceSiteWindowless *iface, LPRECT prc)
{
    FIXME("\n");
    return E_NOTIMPL;
}
static HRESULT WINAPI OleInPlaceSiteWindowless_OnDefWindowMessage( IOleInPlaceSiteWindowless *iface, UINT msg, WPARAM wParam, LPARAM lParam, LRESULT* plResult)
{
    FIXME("\n");
    return E_NOTIMPL;
}


/******    IOleInPlaceFrame   *******/
static inline IOCS *impl_from_IOleInPlaceFrame(IOleInPlaceFrame *iface)
{
    return CONTAINING_RECORD(iface, IOCS, IOleInPlaceFrame_iface);
}

static HRESULT WINAPI OleInPlaceFrame_QueryInterface(IOleInPlaceFrame *iface, REFIID riid, void **ppv)
{
    IOCS *This = impl_from_IOleInPlaceFrame(iface);
    return IOleClientSite_QueryInterface(&This->IOleClientSite_iface, riid, ppv);
}

static ULONG WINAPI OleInPlaceFrame_AddRef(IOleInPlaceFrame *iface)
{
    IOCS *This = impl_from_IOleInPlaceFrame(iface);
    return IOleClientSite_AddRef(&This->IOleClientSite_iface);
}

static ULONG WINAPI OleInPlaceFrame_Release(IOleInPlaceFrame *iface)
{
    IOCS *This = impl_from_IOleInPlaceFrame(iface);
    return IOleClientSite_Release(&This->IOleClientSite_iface);
}

static HRESULT WINAPI OleInPlaceFrame_GetWindow(IOleInPlaceFrame *iface, HWND *phWnd)
{
    IOCS *This = impl_from_IOleInPlaceFrame(iface);

    TRACE( "(%p,%p)\n", This, phWnd );

    *phWnd = This->hWnd;
    return S_OK;
}

static HRESULT WINAPI OleInPlaceFrame_ContextSensitiveHelp(IOleInPlaceFrame *iface, BOOL fEnterMode)
{
    IOCS *This = impl_from_IOleInPlaceFrame(iface);

    FIXME( "(%p,%d) - stub\n", This, fEnterMode );
    return E_NOTIMPL;
}

static HRESULT WINAPI OleInPlaceFrame_GetBorder(IOleInPlaceFrame *iface, LPRECT lprectBorder)
{
    IOCS *This = impl_from_IOleInPlaceFrame(iface);

    FIXME( "(%p,%p) - stub\n", This, lprectBorder );
    return E_NOTIMPL;
}

static HRESULT WINAPI OleInPlaceFrame_RequestBorderSpace(IOleInPlaceFrame *iface, LPCBORDERWIDTHS pborderwidths)
{
    IOCS *This = impl_from_IOleInPlaceFrame(iface);

    FIXME( "(%p,%p) - stub\n", This, pborderwidths );
    return E_NOTIMPL;
}

static HRESULT WINAPI OleInPlaceFrame_SetBorderSpace(IOleInPlaceFrame *iface, LPCBORDERWIDTHS pborderwidths)
{
    IOCS *This = impl_from_IOleInPlaceFrame(iface);

    FIXME( "(%p,%p) - stub\n", This, pborderwidths );
    return E_NOTIMPL;
}

static HRESULT WINAPI OleInPlaceFrame_SetActiveObject(IOleInPlaceFrame *iface, IOleInPlaceActiveObject *pActiveObject, LPCOLESTR pszObjName)
{
    IOCS *This = impl_from_IOleInPlaceFrame(iface);

    FIXME( "(%p,%p,%s) - stub\n", This, pActiveObject, debugstr_w(pszObjName) );
    return S_OK;
}

static HRESULT WINAPI OleInPlaceFrame_InsertMenus(IOleInPlaceFrame *iface, HMENU hmenuShared, LPOLEMENUGROUPWIDTHS lpMenuWidths)
{
    IOCS *This = impl_from_IOleInPlaceFrame(iface);

    FIXME( "(%p,%p,%p) - stub\n", This, hmenuShared, lpMenuWidths );
    return E_NOTIMPL;
}

static HRESULT WINAPI OleInPlaceFrame_SetMenu(IOleInPlaceFrame *iface, HMENU hmenuShared, HOLEMENU holemenu, HWND hwndActiveObject)
{
    IOCS *This = impl_from_IOleInPlaceFrame(iface);

    FIXME( "(%p,%p,%p,%p) - stub\n", This, hmenuShared, holemenu, hwndActiveObject );
    return E_NOTIMPL;
}

static HRESULT WINAPI OleInPlaceFrame_RemoveMenus(IOleInPlaceFrame *iface, HMENU hmenuShared)
{
    IOCS *This = impl_from_IOleInPlaceFrame(iface);

    FIXME( "(%p, %p) - stub\n", This, hmenuShared );
    return E_NOTIMPL;
}

static HRESULT WINAPI OleInPlaceFrame_SetStatusText(IOleInPlaceFrame *iface, LPCOLESTR pszStatusText)
{
    IOCS *This = impl_from_IOleInPlaceFrame(iface);

    FIXME( "(%p, %s) - stub\n", This, debugstr_w( pszStatusText ) );
    return E_NOTIMPL;
}

static HRESULT WINAPI OleInPlaceFrame_EnableModeless(IOleInPlaceFrame *iface, BOOL fEnable)
{
    IOCS *This = impl_from_IOleInPlaceFrame(iface);

    FIXME( "(%p, %d) - stub\n", This, fEnable );
    return E_NOTIMPL;
}

static HRESULT WINAPI OleInPlaceFrame_TranslateAccelerator(IOleInPlaceFrame *iface, LPMSG lpmsg, WORD wID)
{
    IOCS *This = impl_from_IOleInPlaceFrame(iface);

    FIXME( "(%p, %p, %x) - stub\n", This, lpmsg, wID );
    return E_NOTIMPL;
}


/******    IOleControlSite    *******/
static inline IOCS *impl_from_IOleControlSite(IOleControlSite *iface)
{
    return CONTAINING_RECORD(iface, IOCS, IOleControlSite_iface);
}

static HRESULT WINAPI OleControlSite_QueryInterface(IOleControlSite *iface, REFIID riid, void **ppv)
{
    IOCS *This = impl_from_IOleControlSite(iface);
    return IOleClientSite_QueryInterface(&This->IOleClientSite_iface, riid, ppv);
}

static ULONG WINAPI OleControlSite_AddRef(IOleControlSite *iface)
{
    IOCS *This = impl_from_IOleControlSite(iface);
    return IOleClientSite_AddRef(&This->IOleClientSite_iface);
}

static ULONG WINAPI OleControlSite_Release(IOleControlSite *iface)
{
    IOCS *This = impl_from_IOleControlSite(iface);
    return IOleClientSite_Release(&This->IOleClientSite_iface);
}

static HRESULT WINAPI OleControlSite_OnControlInfoChanged( IOleControlSite* This)
{
    FIXME( "\n" );
    return E_NOTIMPL;
}
static HRESULT WINAPI OleControlSite_LockInPlaceActive( IOleControlSite* This, BOOL fLock)
{
    FIXME( "\n" );
    return E_NOTIMPL;
}
static HRESULT WINAPI OleControlSite_GetExtendedControl( IOleControlSite* This, IDispatch** ppDisp)
{
    FIXME( "\n" );
    return E_NOTIMPL;
}
static HRESULT WINAPI OleControlSite_TransformCoords( IOleControlSite* This, POINTL* pPtlHimetric, POINTF* pPtfContainer, DWORD dwFlags)
{
    FIXME( "\n" );
    return E_NOTIMPL;
}
static HRESULT WINAPI OleControlSite_TranslateAccelerator( IOleControlSite* This, MSG* pMsg, DWORD grfModifiers)
{
    FIXME( "\n" );
    return E_NOTIMPL;
}
static HRESULT WINAPI OleControlSite_OnFocus( IOleControlSite* This, BOOL fGotFocus)
{
    FIXME( "\n" );
    return E_NOTIMPL;
}
static HRESULT WINAPI OleControlSite_ShowPropertyFrame( IOleControlSite* This)
{
    FIXME( "\n" );
    return E_NOTIMPL;
}


static const IOleClientSiteVtbl OleClientSite_vtbl = {
    OleClientSite_QueryInterface,
    OleClientSite_AddRef,
    OleClientSite_Release,
    OleClientSite_SaveObject,
    OleClientSite_GetMoniker,
    OleClientSite_GetContainer,
    OleClientSite_ShowObject,
    OleClientSite_OnShowWindow,
    OleClientSite_RequestNewObjectLayout
};
static const IOleContainerVtbl OleContainer_vtbl = {
    OleContainer_QueryInterface,
    OleContainer_AddRef,
    OleContainer_Release,
    OleContainer_ParseDisplayName,
    OleContainer_EnumObjects,
    OleContainer_LockContainer
};
static const IOleInPlaceSiteWindowlessVtbl OleInPlaceSiteWindowless_vtbl = {
    OleInPlaceSiteWindowless_QueryInterface,
    OleInPlaceSiteWindowless_AddRef,
    OleInPlaceSiteWindowless_Release,
    OleInPlaceSiteWindowless_GetWindow,
    OleInPlaceSiteWindowless_ContextSensitiveHelp,
    OleInPlaceSiteWindowless_CanInPlaceActivate,
    OleInPlaceSiteWindowless_OnInPlaceActivate,
    OleInPlaceSiteWindowless_OnUIActivate,
    OleInPlaceSiteWindowless_GetWindowContext,
    OleInPlaceSiteWindowless_Scroll,
    OleInPlaceSiteWindowless_OnUIDeactivate,
    OleInPlaceSiteWindowless_OnInPlaceDeactivate,
    OleInPlaceSiteWindowless_DiscardUndoState,
    OleInPlaceSiteWindowless_DeactivateAndUndo,
    OleInPlaceSiteWindowless_OnPosRectChange,
    OleInPlaceSiteWindowless_OnInPlaceActivateEx,
    OleInPlaceSiteWindowless_OnInPlaceDeactivateEx,
    OleInPlaceSiteWindowless_RequestUIActivate,
    OleInPlaceSiteWindowless_CanWindowlessActivate,
    OleInPlaceSiteWindowless_GetCapture,
    OleInPlaceSiteWindowless_SetCapture,
    OleInPlaceSiteWindowless_GetFocus,
    OleInPlaceSiteWindowless_SetFocus,
    OleInPlaceSiteWindowless_GetDC,
    OleInPlaceSiteWindowless_ReleaseDC,
    OleInPlaceSiteWindowless_InvalidateRect,
    OleInPlaceSiteWindowless_InvalidateRgn,
    OleInPlaceSiteWindowless_ScrollRect,
    OleInPlaceSiteWindowless_AdjustRect,
    OleInPlaceSiteWindowless_OnDefWindowMessage
};
static const IOleInPlaceFrameVtbl OleInPlaceFrame_vtbl =
{
    OleInPlaceFrame_QueryInterface,
    OleInPlaceFrame_AddRef,
    OleInPlaceFrame_Release,
    OleInPlaceFrame_GetWindow,
    OleInPlaceFrame_ContextSensitiveHelp,
    OleInPlaceFrame_GetBorder,
    OleInPlaceFrame_RequestBorderSpace,
    OleInPlaceFrame_SetBorderSpace,
    OleInPlaceFrame_SetActiveObject,
    OleInPlaceFrame_InsertMenus,
    OleInPlaceFrame_SetMenu,
    OleInPlaceFrame_RemoveMenus,
    OleInPlaceFrame_SetStatusText,
    OleInPlaceFrame_EnableModeless,
    OleInPlaceFrame_TranslateAccelerator
};
static const IOleControlSiteVtbl OleControlSite_vtbl =
{
    OleControlSite_QueryInterface,
    OleControlSite_AddRef,
    OleControlSite_Release,
    OleControlSite_OnControlInfoChanged,
    OleControlSite_LockInPlaceActive,
    OleControlSite_GetExtendedControl,
    OleControlSite_TransformCoords,
    OleControlSite_TranslateAccelerator,
    OleControlSite_OnFocus,
    OleControlSite_ShowPropertyFrame
};

static void IOCS_OnSize( IOCS* This, LPCRECT rect )
{
    SIZEL inPix, inHi;

    This->size = *rect;

    if ( !This->control )
        return;

    inPix.cx = rect->right - rect->left;
    inPix.cy = rect->bottom - rect->top;
    AtlPixelToHiMetric( &inPix, &inHi );
    IOleObject_SetExtent( This->control, DVASPECT_CONTENT, &inHi );

    if ( This->fInPlace )
    {
        IOleInPlaceObject *wl;

        if ( SUCCEEDED( IOleObject_QueryInterface( This->control, &IID_IOleInPlaceObject, (void**)&wl ) ) )
        {
            IOleInPlaceObject_SetObjectRects( wl, rect, rect );
            IOleInPlaceObject_Release( wl );
        }
    }
}

static void IOCS_OnShow( IOCS *This, BOOL fShow )
{
    if (!This->control || This->fActive || !fShow )
        return;

    This->fActive = TRUE;
}

static void IOCS_OnDraw( IOCS *This )
{
    IViewObject *view;

    if ( !This->control || !This->fWindowless )
        return;

    if ( SUCCEEDED( IOleObject_QueryInterface( This->control, &IID_IViewObject, (void**)&view ) ) )
    {
        HDC dc = GetDC( This->hWnd );
        RECTL rect;

        rect.left = This->size.left; rect.top = This->size.top;
        rect.bottom = This->size.bottom; rect.right = This->size.right;

        IViewObject_Draw( view, DVASPECT_CONTENT, ~0, NULL, NULL, 0, dc, &rect, &rect, NULL, 0 );
        IViewObject_Release( view );
        ReleaseDC( This->hWnd, dc );
    }
}

static LRESULT IOCS_OnWndProc( IOCS *This, HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
    WNDPROC OrigWndProc = This->OrigWndProc;

    switch( uMsg )
    {
        case WM_DESTROY:
            IOCS_Detach( This );
            break;
        case WM_SIZE:
            {
                RECT r;
                SetRect(&r, 0, 0, LOWORD(lParam), HIWORD(lParam));
                IOCS_OnSize( This, &r );
            }
            break;
        case WM_SHOWWINDOW:
            IOCS_OnShow( This, (BOOL) wParam );
            break;
        case WM_PAINT:
            IOCS_OnDraw( This );
            break;
    }

    return CallWindowProcW( OrigWndProc, hWnd, uMsg, wParam, lParam );
}

static LRESULT CALLBACK AtlHost_wndproc( HWND hWnd, UINT wMsg, WPARAM wParam, LPARAM lParam )
{
    IOCS *This = (IOCS*) GetPropW( hWnd, wine_atl_iocsW );
    return IOCS_OnWndProc( This, hWnd, wMsg, wParam, lParam );
}

static HRESULT IOCS_Attach( IOCS *This, HWND hWnd, IUnknown *pUnkControl ) /* subclass hWnd */
{
    This->hWnd = hWnd;
    IUnknown_QueryInterface( pUnkControl, &IID_IOleObject, (void**)&This->control );
    IOleObject_SetClientSite( This->control, &This->IOleClientSite_iface );
    SetPropW( hWnd, wine_atl_iocsW, This );
    This->OrigWndProc = (WNDPROC)SetWindowLongPtrW( hWnd, GWLP_WNDPROC, (ULONG_PTR) AtlHost_wndproc );

    return S_OK;
}

static HRESULT IOCS_Init( IOCS *This )
{
    RECT rect;
    static const WCHAR AXWIN[] = {'A','X','W','I','N',0};

    IOleObject_SetHostNames( This->control, AXWIN, AXWIN );

    GetClientRect( This->hWnd, &rect );
    IOCS_OnSize( This, &rect );
    IOleObject_DoVerb( This->control, OLEIVERB_INPLACEACTIVATE, NULL, &This->IOleClientSite_iface,
                       0, This->hWnd, &rect );

    return S_OK;
}

/**********************************************************************
 * Create new instance of Atl host component and attach it to window  *
 */
static HRESULT IOCS_Create( HWND hWnd, IUnknown *pUnkControl, IUnknown **container )
{
    HRESULT hr;
    IOCS *This;

    if (!container)
        return S_OK;

    *container = NULL;
    This = HeapAlloc(GetProcessHeap(), 0, sizeof(IOCS));

    if (!This)
        return E_OUTOFMEMORY;

    This->IOleClientSite_iface.lpVtbl = &OleClientSite_vtbl;
    This->IOleContainer_iface.lpVtbl = &OleContainer_vtbl;
    This->IOleInPlaceSiteWindowless_iface.lpVtbl = &OleInPlaceSiteWindowless_vtbl;
    This->IOleInPlaceFrame_iface.lpVtbl = &OleInPlaceFrame_vtbl;
    This->IOleControlSite_iface.lpVtbl = &OleControlSite_vtbl;
    This->ref = 1;

    This->OrigWndProc = NULL;
    This->hWnd = NULL;
    This->fWindowless = This->fActive = This->fInPlace = FALSE;

    hr = IOCS_Attach( This, hWnd, pUnkControl );
    if ( SUCCEEDED( hr ) )
        hr = IOCS_Init( This );
    if ( SUCCEEDED( hr ) )
        *container = (IUnknown*)&This->IOleClientSite_iface;
    else
    {
        IOCS_Detach( This );
        HeapFree(GetProcessHeap(), 0, This);
    }

    return hr;
}


/***********************************************************************
 *           AtlAxCreateControl           [atl100.@]
 */
HRESULT WINAPI AtlAxCreateControl(LPCOLESTR lpszName, HWND hWnd,
        IStream *pStream, IUnknown **ppUnkContainer)
{
    return AtlAxCreateControlEx( lpszName, hWnd, pStream, ppUnkContainer,
            NULL, NULL, NULL );
}

enum content
{
    IsEmpty = 0,
    IsGUID = 1,
    IsHTML = 2,
    IsURL = 3,
    IsUnknown = 4
};

static enum content get_content_type(LPCOLESTR name, CLSID *control_id)
{
    static const WCHAR mshtml_prefixW[] = {'m','s','h','t','m','l',':',0};
    WCHAR new_urlW[MAX_PATH];
    DWORD size = MAX_PATH;

    if (!name || !name[0])
    {
        WARN("name %s\n", wine_dbgstr_w(name));
        return IsEmpty;
    }

    if (CLSIDFromString(name, control_id) == S_OK ||
        CLSIDFromProgID(name, control_id) == S_OK)
        return IsGUID;

    if (PathIsURLW (name) ||
        UrlApplySchemeW(name, new_urlW, &size, URL_APPLY_GUESSSCHEME|URL_APPLY_GUESSFILE) == S_OK)
    {
        *control_id = CLSID_WebBrowser;
        return IsURL;
    }

    if (!_wcsnicmp(name, mshtml_prefixW, 7))
    {
        FIXME("mshtml prefix not implemented\n");
        *control_id = CLSID_WebBrowser;
        return IsHTML;
    }

    return IsUnknown;
}

/***********************************************************************
 *           AtlAxCreateControlLicEx         [atl100.@]
 *
 * REMARKS
 *   See https://www.codeproject.com/KB/com/cwebpage.aspx for some background
 *
 */
HRESULT WINAPI AtlAxCreateControlLicEx(LPCOLESTR lpszName, HWND hWnd,
        IStream *pStream, IUnknown **ppUnkContainer, IUnknown **ppUnkControl,
        REFIID iidSink, IUnknown *punkSink, BSTR lic)
{
    CLSID controlId;
    HRESULT hRes;
    IOleObject *pControl;
    IUnknown *pUnkControl = NULL;
    IPersistStreamInit *pPSInit;
    IUnknown *pContainer = NULL;
    enum content content;

    TRACE("(%s %p %p %p %p %p %p %s)\n", debugstr_w(lpszName), hWnd, pStream,
            ppUnkContainer, ppUnkControl, iidSink, punkSink, debugstr_w(lic));

    if (lic)
        FIXME("semi stub\n");

    if (ppUnkContainer) *ppUnkContainer = NULL;
    if (ppUnkControl) *ppUnkControl = NULL;

    content = get_content_type(lpszName, &controlId);

    if (content == IsEmpty)
        return S_OK;

    if (content == IsUnknown)
        return CO_E_CLASSSTRING;

    hRes = CoCreateInstance( &controlId, 0, CLSCTX_ALL, &IID_IOleObject,
            (void**) &pControl );
    if ( FAILED( hRes ) )
    {
        WARN( "cannot create ActiveX control %s instance - error 0x%08x\n",
                debugstr_guid( &controlId ), hRes );
        return hRes;
    }

    hRes = IOleObject_QueryInterface( pControl, &IID_IPersistStreamInit, (void**) &pPSInit );
    if ( SUCCEEDED( hRes ) )
    {
        if (!pStream)
            IPersistStreamInit_InitNew( pPSInit );
        else
            IPersistStreamInit_Load( pPSInit, pStream );
        IPersistStreamInit_Release( pPSInit );
    } else
        WARN("cannot get IID_IPersistStreamInit out of control\n");

    IOleObject_QueryInterface( pControl, &IID_IUnknown, (void**) &pUnkControl );
    IOleObject_Release( pControl );


    hRes = AtlAxAttachControl( pUnkControl, hWnd, &pContainer );
    if ( FAILED( hRes ) )
        WARN("cannot attach control to window\n");

    if ( content == IsURL )
    {
        IWebBrowser2 *browser;

        hRes = IOleObject_QueryInterface( pControl, &IID_IWebBrowser2, (void**) &browser );
        if ( !browser )
            WARN( "Cannot query IWebBrowser2 interface: %08x\n", hRes );
        else {
            VARIANT url;

            IWebBrowser2_put_Visible( browser, VARIANT_TRUE ); /* it seems that native does this on URL (but do not on MSHTML:! why? */

            V_VT(&url) = VT_BSTR;
            V_BSTR(&url) = SysAllocString( lpszName );

            hRes = IWebBrowser2_Navigate2( browser, &url, NULL, NULL, NULL, NULL );
            if ( FAILED( hRes ) )
                WARN( "IWebBrowser2::Navigate2 failed: %08x\n", hRes );
            SysFreeString( V_BSTR(&url) );

            IWebBrowser2_Release( browser );
        }
    }

    if (ppUnkContainer)
    {
        *ppUnkContainer = pContainer;
        if ( pContainer )
            IUnknown_AddRef( pContainer );
    }
    if (ppUnkControl)
    {
        *ppUnkControl = pUnkControl;
        if ( pUnkControl )
            IUnknown_AddRef( pUnkControl );
    }

    if ( pUnkControl )
        IUnknown_Release( pUnkControl );
    if ( pContainer )
        IUnknown_Release( pContainer );

    return S_OK;
}

/***********************************************************************
 *           AtlAxAttachControl           [atl100.@]
 */
HRESULT WINAPI AtlAxAttachControl(IUnknown *control, HWND hWnd, IUnknown **container)
{
    HRESULT hr;

    TRACE("(%p %p %p)\n", control, hWnd, container);

    if (!control)
        return E_INVALIDARG;

    hr = IOCS_Create( hWnd, control, container );
    return hWnd ? hr : S_FALSE;
}

/**********************************************************************
 * Helper function for AX_ConvertDialogTemplate
 */
static inline BOOL advance_array(WORD **pptr, DWORD *palloc, DWORD *pfilled, const WORD *data, DWORD size)
{
    if ( (*pfilled + size) > *palloc )
    {
        *palloc = ((*pfilled+size) + 0xFF) & ~0xFF;
        *pptr = HeapReAlloc( GetProcessHeap(), 0, *pptr, *palloc * sizeof(WORD) );
        if (!*pptr)
            return FALSE;
    }
    RtlMoveMemory( *pptr+*pfilled, data, size * sizeof(WORD) );
    *pfilled += size;
    return TRUE;
}

/**********************************************************************
 * Convert ActiveX control templates to AtlAxWin class instances
 */
static LPDLGTEMPLATEW AX_ConvertDialogTemplate(LPCDLGTEMPLATEW src_tmpl)
{
#define GET_WORD(x)  (*(const  WORD *)(x))
#define GET_DWORD(x) (*(const DWORD *)(x))
#define PUT_BLOCK(x,y) do {if (!advance_array(&output, &allocated, &filled, (x), (y))) return NULL;} while (0)
#define PUT_WORD(x)  do {WORD w = (x);PUT_BLOCK(&w, 1);} while(0)
    const WORD *tmp, *src = (const WORD *)src_tmpl;
    WORD *output;
    DWORD allocated, filled; /* in WORDs */
    BOOL ext;
    WORD signature, dlgver, rescount;
    DWORD style;

    filled = 0; allocated = 256;
    output = HeapAlloc( GetProcessHeap(), 0, allocated * sizeof(WORD) );
    if (!output)
        return NULL;

    /* header */
    tmp = src;
    signature = GET_WORD(src);
    dlgver = GET_WORD(src + 1);
    if (signature == 1 && dlgver == 0xFFFF)
    {
        ext = TRUE;
        src += 6;
        style = GET_DWORD(src);
        src += 2;
        rescount = GET_WORD(src++);
        src += 4;
        if ( GET_WORD(src) == 0xFFFF ) /* menu */
            src += 2;
        else
            src += lstrlenW(src) + 1;
        if ( GET_WORD(src) == 0xFFFF ) /* class */
            src += 2;
        else
            src += lstrlenW(src) + 1;
        src += lstrlenW(src) + 1; /* title */
        if ( style & (DS_SETFONT | DS_SHELLFONT) )
        {
            src += 3;
            src += lstrlenW(src) + 1;
        }
    } else {
        ext = FALSE;
        style = GET_DWORD(src);
        src += 4;
        rescount = GET_WORD(src++);
        src += 4;
        if ( GET_WORD(src) == 0xFFFF ) /* menu */
            src += 2;
        else
            src += lstrlenW(src) + 1;
        if ( GET_WORD(src) == 0xFFFF ) /* class */
            src += 2;
        else
            src += lstrlenW(src) + 1;
        src += lstrlenW(src) + 1; /* title */
        if ( style & DS_SETFONT )
        {
            src++;
            src += lstrlenW(src) + 1;
        }
    }
    PUT_BLOCK(tmp, src-tmp);

    while(rescount--)
    {
        src = (const WORD *)( ( ((ULONG_PTR)src) + 3) & ~3); /* align on DWORD boundary */
        filled = (filled + 1) & ~1; /* depends on DWORD-aligned allocation unit */

        tmp = src;
        if (ext)
            src += 12;
        else
            src += 9;
        PUT_BLOCK(tmp, src-tmp);

        tmp = src;
        if ( GET_WORD(src) == 0xFFFF ) /* class */
        {
            src += 2;
        } else
        {
            src += lstrlenW(src) + 1;
        }
        src += lstrlenW(src) + 1; /* title */
        if ( GET_WORD(tmp) == '{' ) /* all this mess created because of this line */
        {
            static const WCHAR AtlAxWin[] = {'A','t','l','A','x','W','i','n', 0};
            PUT_BLOCK(AtlAxWin, ARRAY_SIZE(AtlAxWin));
            PUT_BLOCK(tmp, lstrlenW(tmp)+1);
        } else
            PUT_BLOCK(tmp, src-tmp);

        if ( GET_WORD(src) )
        {
            WORD size = (GET_WORD(src)+sizeof(WORD)-1) / sizeof(WORD); /* quite ugly :( Maybe use BYTE* instead of WORD* everywhere ? */
            PUT_BLOCK(src, size);
            src+=size;
        }
        else
        {
            PUT_WORD(0);
            src++;
        }
    }
    return (LPDLGTEMPLATEW) output;
}

/***********************************************************************
 *           AtlAxCreateDialogA           [atl100.@]
 *
 * Creates a dialog window
 *
 * PARAMS
 *  hInst   [I] Application instance
 *  name    [I] Dialog box template name
 *  owner   [I] Dialog box parent HWND
 *  dlgProc [I] Dialog box procedure
 *  param   [I] This value will be passed to dlgProc as WM_INITDIALOG's message lParam
 *
 * RETURNS
 *  Window handle of dialog window.
 */
HWND WINAPI AtlAxCreateDialogA(HINSTANCE hInst, LPCSTR name, HWND owner, DLGPROC dlgProc ,LPARAM param)
{
    HWND res = NULL;
    int length;
    WCHAR *nameW;

    if (IS_INTRESOURCE(name))
        return AtlAxCreateDialogW( hInst, (LPCWSTR) name, owner, dlgProc, param );

    length = MultiByteToWideChar( CP_ACP, 0, name, -1, NULL, 0 );
    nameW = HeapAlloc( GetProcessHeap(), 0, length * sizeof(WCHAR) );
    if (nameW)
    {
        MultiByteToWideChar( CP_ACP, 0, name, -1, nameW, length );
        res = AtlAxCreateDialogW( hInst, nameW, owner, dlgProc, param );
        HeapFree( GetProcessHeap(), 0, nameW );
    }
    return res;
}

/***********************************************************************
 *           AtlAxCreateDialogW           [atl100.@]
 *
 * See AtlAxCreateDialogA
 *
 */
HWND WINAPI AtlAxCreateDialogW(HINSTANCE hInst, LPCWSTR name, HWND owner, DLGPROC dlgProc ,LPARAM param)
{
    HRSRC hrsrc;
    HGLOBAL hgl;
    LPCDLGTEMPLATEW ptr;
    LPDLGTEMPLATEW newptr;
    HWND res;

    TRACE("(%p %s %p %p %lx)\n", hInst, debugstr_w(name), owner, dlgProc, param);

    hrsrc = FindResourceW( hInst, name, (LPWSTR)RT_DIALOG );
    if ( !hrsrc )
        return NULL;
    hgl = LoadResource (hInst, hrsrc);
    if ( !hgl )
        return NULL;
    ptr = LockResource ( hgl );
    if (!ptr)
    {
        FreeResource( hgl );
        return NULL;
    }
    newptr = AX_ConvertDialogTemplate( ptr );
    if ( newptr )
    {
            res = CreateDialogIndirectParamW( hInst, newptr, owner, dlgProc, param );
            HeapFree( GetProcessHeap(), 0, newptr );
    } else
        res = NULL;
    FreeResource ( hrsrc );
    return res;
}

/***********************************************************************
 *           AtlAxGetHost                 [atl100.@]
 *
 */
HRESULT WINAPI AtlAxGetHost(HWND hWnd, IUnknown **host)
{
    IOCS *This;

    TRACE("(%p, %p)\n", hWnd, host);

    *host = NULL;

    This = (IOCS*) GetPropW( hWnd, wine_atl_iocsW );
    if ( !This )
    {
        WARN("No container attached to %p\n", hWnd );
        return E_FAIL;
    }

    return IOleClientSite_QueryInterface(&This->IOleClientSite_iface, &IID_IUnknown, (void**)host);
}

/***********************************************************************
 *           AtlAxGetControl              [atl100.@]
 *
 */
HRESULT WINAPI AtlAxGetControl(HWND hWnd, IUnknown **pUnk)
{
    IOCS *This;

    TRACE( "(%p, %p)\n", hWnd, pUnk );

    *pUnk = NULL;

    This = (IOCS*) GetPropW( hWnd, wine_atl_iocsW );
    if ( !This || !This->control )
    {
        WARN("No control attached to %p\n", hWnd );
        return E_FAIL;
    }

    return IOleObject_QueryInterface( This->control, &IID_IUnknown, (void**) pUnk );
}

/***********************************************************************
 *           AtlAxDialogBoxA              [atl100.@]
 *
 */
INT_PTR WINAPI AtlAxDialogBoxA(HINSTANCE hInst, LPCSTR name, HWND owner, DLGPROC dlgProc, LPARAM param)
{
    INT_PTR res = 0;
    int length;
    WCHAR *nameW;

    if (IS_INTRESOURCE(name))
        return AtlAxDialogBoxW( hInst, (LPCWSTR) name, owner, dlgProc, param );

    length = MultiByteToWideChar( CP_ACP, 0, name, -1, NULL, 0 );
    nameW = heap_alloc( length * sizeof(WCHAR) );
    if (nameW)
    {
        MultiByteToWideChar( CP_ACP, 0, name, -1, nameW, length );
        res = AtlAxDialogBoxW( hInst, nameW, owner, dlgProc, param );
        heap_free( nameW );
    }
    return res;
}

/***********************************************************************
 *           AtlAxDialogBoxW              [atl100.@]
 *
 */
INT_PTR WINAPI AtlAxDialogBoxW(HINSTANCE hInst, LPCWSTR name, HWND owner, DLGPROC dlgProc, LPARAM param)
{
    HRSRC hrsrc;
    HGLOBAL hgl;
    LPCDLGTEMPLATEW ptr;
    LPDLGTEMPLATEW newptr;
    INT_PTR res;

    TRACE("(%p %s %p %p %lx)\n", hInst, debugstr_w(name), owner, dlgProc, param);

    hrsrc = FindResourceW( hInst, name, (LPWSTR)RT_DIALOG );
    if ( !hrsrc )
        return 0;
    hgl = LoadResource (hInst, hrsrc);
    if ( !hgl )
        return 0;
    ptr = LockResource ( hgl );
    if (!ptr)
    {
        FreeResource( hgl );
        return 0;
    }
    newptr = AX_ConvertDialogTemplate( ptr );
    if ( newptr )
    {
        res = DialogBoxIndirectParamW( hInst, newptr, owner, dlgProc, param );
        heap_free( newptr );
    } else
        res = 0;
    FreeResource ( hrsrc );
    return res;
}

/***********************************************************************
 *           AtlAxCreateControlLic        [atl100.59]
 *
 */
HRESULT WINAPI AtlAxCreateControlLic(const WCHAR *lpTricsData, HWND hwnd, IStream *stream, IUnknown **container, BSTR lic)
{
    return AtlAxCreateControlLicEx(lpTricsData, hwnd, stream, container, NULL, NULL, NULL, lic);
}

/***********************************************************************
 *           AtlAxCreateControlEx         [atl100.@]
 *
 */
HRESULT WINAPI AtlAxCreateControlEx(const WCHAR *lpTricsData, HWND hwnd, IStream *stream,
        IUnknown **container, IUnknown **control, REFIID iidSink, IUnknown *punkSink)
{
    return AtlAxCreateControlLicEx(lpTricsData, hwnd, stream, container, control, iidSink, punkSink, NULL);
}
