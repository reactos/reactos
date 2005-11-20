/*
 * Header includes for shdocvw.dll
 *
 * Copyright 2001 John R. Sheets (for CodeWeavers)
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

#ifndef __WINE_SHDOCVW_H
#define __WINE_SHDOCVW_H

#define COM_NO_WINDOWS_H
#define COBJMACROS

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"

#include "ole2.h"
#include "olectl.h"
#include "shlobj.h"
#include "exdisp.h"

/**********************************************************************
 * IClassFactory declaration for SHDOCVW.DLL
 */
typedef struct
{
    /* IUnknown fields */
    const IClassFactoryVtbl *lpVtbl;
    LONG ref;
} IClassFactoryImpl;

extern IClassFactoryImpl SHDOCVW_ClassFactory;

/**********************************************************************
 * Shell Instance Objects
 */
extern HRESULT SHDOCVW_GetShellInstanceObjectClassObject(REFCLSID rclsid, 
    REFIID riid, LPVOID *ppvClassObj);

/**********************************************************************
 * WebBrowser declaration for SHDOCVW.DLL
 */
typedef struct {
    /* Interfaces available via WebBrowser object */

    const IWebBrowser2Vtbl              *lpWebBrowser2Vtbl;
    const IOleObjectVtbl                *lpOleObjectVtbl;
    const IOleInPlaceObjectVtbl         *lpOleInPlaceObjectVtbl;
    const IOleControlVtbl               *lpOleControlVtbl;
    const IPersistStorageVtbl           *lpPersistStorageVtbl;
    const IPersistStreamInitVtbl        *lpPersistStreamInitVtbl;
    const IProvideClassInfo2Vtbl        *lpProvideClassInfoVtbl;
    const IQuickActivateVtbl            *lpQuickActivateVtbl;
    const IConnectionPointContainerVtbl *lpConnectionPointContainerVtbl;
    const IViewObject2Vtbl              *lpViewObjectVtbl;

    /* Interfaces available for embeded document */

    const IOleClientSiteVtbl            *lpOleClientSiteVtbl;
    const IOleInPlaceSiteVtbl           *lpOleInPlaceSiteVtbl;

    LONG ref;

    IUnknown *document;

    IOleClientSite *client;
    IOleContainer *container;

    /* window context */

    HWND iphwnd;
    HWND frame_hwnd;
    IOleInPlaceFrame *frame;
    IOleInPlaceUIWindow *uiwindow;
    RECT pos_rect;
    RECT clip_rect;
    OLEINPLACEFRAMEINFO frameinfo;
} WebBrowser;

#define WEBBROWSER(x)   ((IWebBrowser*)                 &(x)->lpWebBrowser2Vtbl)
#define WEBBROWSER2(x)  ((IWebBrowser2*)                &(x)->lpWebBrowser2Vtbl)
#define OLEOBJ(x)       ((IOleObject*)                  &(x)->lpOleObjectVtbl)
#define INPLACEOBJ(x)   ((IOleInPlaceObject*)           &(x)->lpOleInPlaceObjectVtbl)
#define CONTROL(x)      ((IOleControl*)                 &(x)->lpOleControlVtbl)
#define PERSTORAGE(x)   ((IPersistStorage*)             &(x)->lpPersistStorageVtbl)
#define PERSTRINIT(x)   ((IPersistStreamInit*)          &(x)->lpPersistStreamInitVtbl)
#define CLASSINFO(x)    ((IProvideClassInfo2*)          &(x)->lpProvideClassInfoVtbl)
#define QUICKACT(x)     ((IQuickActivate*)              &(x)->lpQuickActivateVtbl)
#define CONPTCONT(x)    ((IConnectionPointContainer*)   &(x)->lpConnectionPointContainerVtbl)
#define VIEWOBJ(x)      ((IViewObject*)                 &(x)->lpViewObjectVtbl);
#define VIEWOBJ2(x)     ((IViewObject2*)                &(x)->lpViewObjectVtbl);

#define CLIENTSITE(x)   ((IOleClientSite*)              &(x)->lpOleClientSiteVtbl)
#define INPLACESITE(x)  ((IOleInPlaceSite*)             &(x)->lpOleInPlaceSiteVtbl)

void WebBrowser_OleObject_Init(WebBrowser*);
void WebBrowser_ViewObject_Init(WebBrowser*);
void WebBrowser_Persist_Init(WebBrowser*);
void WebBrowser_ClassInfo_Init(WebBrowser*);
void WebBrowser_Misc_Init(WebBrowser*);
void WebBrowser_Events_Init(WebBrowser*);

void WebBrowser_ClientSite_Init(WebBrowser*);

void WebBrowser_OleObject_Destroy(WebBrowser*);

HRESULT WebBrowser_Create(IUnknown*,REFIID,void**);

/**********************************************************************
 * IConnectionPoint declaration for SHDOCVW.DLL
 */
typedef struct
{
    /* IUnknown fields */
    const IConnectionPointVtbl *lpVtbl;
    LONG ref;
} IConnectionPointImpl;

#define DEFINE_THIS(cls,ifc,iface) ((cls*)((BYTE*)(iface)-offsetof(cls,lp ## ifc ## Vtbl)))

/**********************************************************************
 * Dll lifetime tracking declaration for shdocvw.dll
 */
extern LONG SHDOCVW_refCount;
static inline void SHDOCVW_LockModule(void) { InterlockedIncrement( &SHDOCVW_refCount ); }
static inline void SHDOCVW_UnlockModule(void) { InterlockedDecrement( &SHDOCVW_refCount ); }

#endif /* __WINE_SHDOCVW_H */
