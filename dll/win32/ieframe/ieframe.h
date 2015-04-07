/*
 * Header includes for ieframe.dll
 *
 * Copyright 2011 Jacek Caban for CodeWeavers
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

#ifndef _IEFRAME_H_
#define _IEFRAME_H_

#include <stdio.h>

#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H

#define COBJMACROS
#define NONAMELESSUNION
#define NONAMELESSSTRUCT

#include <windef.h>
#include <winbase.h>
#include <wingdi.h>
#include <winreg.h>
#include <wincon.h>
#include <shlobj.h>
#include <mshtmhst.h>
#include <mshtmdid.h>
#include <exdispid.h>
#include <htiface.h>
#include <idispids.h>
#include <intshcut.h>
#include <perhist.h>
#include <shellapi.h>
#include <shlwapi.h>
#include <shdeprecated.h>
#include <docobjectservice.h>

#include <wine/unicode.h>
#include <wine/list.h>

#include <wine/debug.h>
WINE_DEFAULT_DEBUG_CHANNEL(ieframe);

#include "resource.h"

typedef struct ConnectionPoint ConnectionPoint;
typedef struct DocHost DocHost;

typedef struct {
    IConnectionPointContainer IConnectionPointContainer_iface;

    ConnectionPoint *wbe2;
    ConnectionPoint *wbe;
    ConnectionPoint *pns;

    IUnknown *impl;
} ConnectionPointContainer;

typedef struct {
    IHlinkFrame    IHlinkFrame_iface;
    ITargetFrame2  ITargetFrame2_iface;
    ITargetFramePriv2 ITargetFramePriv2_iface;
    IWebBrowserPriv2IE9 IWebBrowserPriv2IE9_iface;

    IUnknown *outer;
    DocHost *doc_host;
} HlinkFrame;

struct _task_header_t;

typedef void (*task_proc_t)(DocHost*, struct _task_header_t*);
typedef void (*task_destr_t)(struct _task_header_t*);

typedef struct _task_header_t {
    struct list entry;
    task_proc_t proc;
    task_destr_t destr;
} task_header_t;

typedef struct {
    IShellBrowser IShellBrowser_iface;
    IBrowserService IBrowserService_iface;
    IDocObjectService IDocObjectService_iface;

    LONG ref;

    DocHost *doc_host;
}  ShellBrowser;

typedef struct {
    IHTMLWindow2 IHTMLWindow2_iface;
    DocHost *doc_host;
} IEHTMLWindow;

typedef struct {
    INewWindowManager INewWindowManager_iface;
    DocHost *doc_host;
} NewWindowManager;

typedef struct {
    WCHAR *url;
    IStream *stream;
} travellog_entry_t;

typedef struct _IDocHostContainerVtbl
{
    ULONG (*addref)(DocHost*);
    ULONG (*release)(DocHost*);
    void (WINAPI* GetDocObjRect)(DocHost*,RECT*);
    HRESULT (WINAPI* SetStatusText)(DocHost*,LPCWSTR);
    void (WINAPI* SetURL)(DocHost*,LPCWSTR);
    HRESULT (*exec)(DocHost*,const GUID*,DWORD,DWORD,VARIANT*,VARIANT*);
} IDocHostContainerVtbl;

struct DocHost {
    IOleClientSite      IOleClientSite_iface;
    IOleInPlaceSiteEx   IOleInPlaceSiteEx_iface;
    IDocHostUIHandler2  IDocHostUIHandler2_iface;
    IOleDocumentSite    IOleDocumentSite_iface;
    IOleControlSite     IOleControlSite_iface;
    IOleCommandTarget   IOleCommandTarget_iface;
    IDispatch           IDispatch_iface;
    IPropertyNotifySink IPropertyNotifySink_iface;
    IServiceProvider    IServiceProvider_iface;

    /* Interfaces of InPlaceFrame object */
    IOleInPlaceFrame  IOleInPlaceFrame_iface;

    IWebBrowser2 *wb;

    IDispatch *client_disp;
    IDocHostUIHandler *hostui;
    IOleInPlaceFrame *frame;

    IUnknown *document;
    IOleDocumentView *view;
    IUnknown *doc_navigate;

    const IDocHostContainerVtbl *container_vtbl;

    HWND hwnd;
    HWND frame_hwnd;

    struct list task_queue;

    LPOLESTR url;

    VARIANT_BOOL silent;
    VARIANT_BOOL offline;
    VARIANT_BOOL busy;

    READYSTATE ready_state;
    READYSTATE doc_state;
    DWORD prop_notif_cookie;
    BOOL is_prop_notif;

    ShellBrowser *browser_service;
    IShellUIHelper2 *shell_ui_helper;

    struct {
        travellog_entry_t *log;
        unsigned size;
        unsigned length;
        unsigned position;
        int loading_pos;
    } travellog;

    ConnectionPointContainer cps;
    IEHTMLWindow html_window;
    NewWindowManager nwm;
};

struct WebBrowser {
    IWebBrowser2             IWebBrowser2_iface;
    IOleObject               IOleObject_iface;
    IOleInPlaceObject        IOleInPlaceObject_iface;
    IOleControl              IOleControl_iface;
    IPersistStorage          IPersistStorage_iface;
    IPersistMemory           IPersistMemory_iface;
    IPersistStreamInit       IPersistStreamInit_iface;
    IProvideClassInfo2       IProvideClassInfo2_iface;
    IViewObject2             IViewObject2_iface;
    IOleInPlaceActiveObject  IOleInPlaceActiveObject_iface;
    IOleCommandTarget        IOleCommandTarget_iface;
    IServiceProvider         IServiceProvider_iface;
    IDataObject              IDataObject_iface;
    HlinkFrame hlink_frame;

    LONG ref;

    INT version;

    IOleClientSite *client;
    IOleContainer *container;
    IOleInPlaceSiteEx *inplace;

    /* window context */

    HWND frame_hwnd;
    IOleInPlaceUIWindow *uiwindow;
    RECT pos_rect;
    RECT clip_rect;
    OLEINPLACEFRAMEINFO frameinfo;
    SIZEL extent;

    HWND shell_embedding_hwnd;

    VARIANT_BOOL register_browser;
    VARIANT_BOOL visible;
    VARIANT_BOOL menu_bar;
    VARIANT_BOOL address_bar;
    VARIANT_BOOL status_bar;
    VARIANT_BOOL tool_bar;
    VARIANT_BOOL full_screen;
    VARIANT_BOOL theater_mode;

    DocHost doc_host;
};

struct InternetExplorer {
    DocHost doc_host;
    IWebBrowser2 IWebBrowser2_iface;
    IExternalConnection IExternalConnection_iface;
    IServiceProvider IServiceProvider_iface;
    HlinkFrame hlink_frame;

    LONG ref;
    LONG extern_ref;

    HWND frame_hwnd;
    HWND status_hwnd;
    HMENU menu;
    BOOL nohome;

    struct list entry;
};

void WebBrowser_OleObject_Init(WebBrowser*) DECLSPEC_HIDDEN;
void WebBrowser_ViewObject_Init(WebBrowser*) DECLSPEC_HIDDEN;
void WebBrowser_Persist_Init(WebBrowser*) DECLSPEC_HIDDEN;
void WebBrowser_ClassInfo_Init(WebBrowser*) DECLSPEC_HIDDEN;

void WebBrowser_OleObject_Destroy(WebBrowser*) DECLSPEC_HIDDEN;

void DocHost_Init(DocHost*,IWebBrowser2*,const IDocHostContainerVtbl*) DECLSPEC_HIDDEN;
void DocHost_Release(DocHost*) DECLSPEC_HIDDEN;
void DocHost_ClientSite_Init(DocHost*) DECLSPEC_HIDDEN;
void DocHost_ClientSite_Release(DocHost*) DECLSPEC_HIDDEN;
void DocHost_Frame_Init(DocHost*) DECLSPEC_HIDDEN;
void release_dochost_client(DocHost*) DECLSPEC_HIDDEN;

void IEHTMLWindow_Init(DocHost*) DECLSPEC_HIDDEN;
void NewWindowManager_Init(DocHost*) DECLSPEC_HIDDEN;

void HlinkFrame_Init(HlinkFrame*,IUnknown*,DocHost*) DECLSPEC_HIDDEN;
BOOL HlinkFrame_QI(HlinkFrame*,REFIID,void**) DECLSPEC_HIDDEN;

HRESULT create_browser_service(DocHost*,ShellBrowser**) DECLSPEC_HIDDEN;
void detach_browser_service(ShellBrowser*) DECLSPEC_HIDDEN;
HRESULT create_shell_ui_helper(IShellUIHelper2**) DECLSPEC_HIDDEN;

void ConnectionPointContainer_Init(ConnectionPointContainer*,IUnknown*) DECLSPEC_HIDDEN;
void ConnectionPointContainer_Destroy(ConnectionPointContainer*) DECLSPEC_HIDDEN;

void call_sink(ConnectionPoint*,DISPID,DISPPARAMS*) DECLSPEC_HIDDEN;
HRESULT navigate_url(DocHost*,LPCWSTR,const VARIANT*,const VARIANT*,VARIANT*,VARIANT*) DECLSPEC_HIDDEN;
HRESULT go_home(DocHost*) DECLSPEC_HIDDEN;
HRESULT go_back(DocHost*) DECLSPEC_HIDDEN;
HRESULT go_forward(DocHost*) DECLSPEC_HIDDEN;
HRESULT refresh_document(DocHost*,const VARIANT*) DECLSPEC_HIDDEN;
HRESULT get_location_url(DocHost*,BSTR*) DECLSPEC_HIDDEN;
HRESULT set_dochost_url(DocHost*,const WCHAR*) DECLSPEC_HIDDEN;
void handle_navigation_error(DocHost*,HRESULT,BSTR,IHTMLWindow2*) DECLSPEC_HIDDEN;
HRESULT dochost_object_available(DocHost*,IUnknown*) DECLSPEC_HIDDEN;
void set_doc_state(DocHost*,READYSTATE) DECLSPEC_HIDDEN;
void deactivate_document(DocHost*) DECLSPEC_HIDDEN;
void create_doc_view_hwnd(DocHost*) DECLSPEC_HIDDEN;
void on_commandstate_change(DocHost*,LONG,VARIANT_BOOL) DECLSPEC_HIDDEN;

#define WM_DOCHOSTTASK (WM_USER+0x300)
void push_dochost_task(DocHost*,task_header_t*,task_proc_t,task_destr_t,BOOL) DECLSPEC_HIDDEN;
void abort_dochost_tasks(DocHost*,task_proc_t) DECLSPEC_HIDDEN;
LRESULT process_dochost_tasks(DocHost*) DECLSPEC_HIDDEN;

void InternetExplorer_WebBrowser_Init(InternetExplorer*) DECLSPEC_HIDDEN;
HRESULT update_ie_statustext(InternetExplorer*, LPCWSTR) DECLSPEC_HIDDEN;
void released_obj(void) DECLSPEC_HIDDEN;
DWORD release_extern_ref(InternetExplorer*,BOOL) DECLSPEC_HIDDEN;

void register_iewindow_class(void) DECLSPEC_HIDDEN;
void unregister_iewindow_class(void) DECLSPEC_HIDDEN;

#define TID_LIST \
    XCLSID(WebBrowser) \
    XCLSID(WebBrowser_V1) \
    XIID(IWebBrowser2)

typedef enum {
#define XIID(iface) iface ## _tid,
#define XCLSID(class) class ## _tid,
TID_LIST
#undef XIID
#undef XCLSID
    LAST_tid
} tid_t;

HRESULT get_typeinfo(tid_t,ITypeInfo**) DECLSPEC_HIDDEN;
HRESULT register_class_object(BOOL) DECLSPEC_HIDDEN;

HRESULT WINAPI CUrlHistory_Create(IClassFactory*,IUnknown*,REFIID,void**) DECLSPEC_HIDDEN;
HRESULT WINAPI InternetExplorer_Create(IClassFactory*,IUnknown*,REFIID,void**) DECLSPEC_HIDDEN;
HRESULT WINAPI InternetShortcut_Create(IClassFactory*,IUnknown*,REFIID,void**) DECLSPEC_HIDDEN;
HRESULT WINAPI WebBrowser_Create(IClassFactory*,IUnknown*,REFIID,void**) DECLSPEC_HIDDEN;
HRESULT WINAPI WebBrowserV1_Create(IClassFactory*,IUnknown*,REFIID,void**) DECLSPEC_HIDDEN;

extern LONG module_ref DECLSPEC_HIDDEN;
extern HINSTANCE ieframe_instance DECLSPEC_HIDDEN;

static inline void lock_module(void) {
    InterlockedIncrement(&module_ref);
}

static inline void unlock_module(void) {
    InterlockedDecrement(&module_ref);
}

static inline void *heap_alloc(size_t len)
{
    return HeapAlloc(GetProcessHeap(), 0, len);
}

static inline void *heap_alloc_zero(size_t len)
{
    return HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, len);
}

static inline void *heap_realloc(void *mem, size_t len)
{
    return HeapReAlloc(GetProcessHeap(), 0, mem, len);
}

static inline BOOL heap_free(void *mem)
{
    return HeapFree(GetProcessHeap(), 0, mem);
}

static inline LPWSTR heap_strdupW(LPCWSTR str)
{
    LPWSTR ret = NULL;

    if(str) {
        DWORD size;

        size = (strlenW(str)+1)*sizeof(WCHAR);
        ret = heap_alloc(size);
        if(ret)
            memcpy(ret, str, size);
    }

    return ret;
}

static inline LPWSTR co_strdupW(LPCWSTR str)
{
    WCHAR *ret = CoTaskMemAlloc((strlenW(str) + 1)*sizeof(WCHAR));
    if (ret)
        lstrcpyW(ret, str);
    return ret;
}

static inline LPWSTR co_strdupAtoW(LPCSTR str)
{
    INT len;
    WCHAR *ret;
    len = MultiByteToWideChar(CP_ACP, 0, str, -1, NULL, 0);
    ret = CoTaskMemAlloc(len*sizeof(WCHAR));
    if (ret)
        MultiByteToWideChar(CP_ACP, 0, str, -1, ret, len);
    return ret;
}

static inline LPSTR co_strdupWtoA(LPCWSTR str)
{
    INT len;
    CHAR *ret;
    len = WideCharToMultiByte(CP_ACP, 0, str, -1, NULL, 0, 0, 0);
    ret = CoTaskMemAlloc(len);
    if (ret)
        WideCharToMultiByte(CP_ACP, 0, str, -1, ret, len, 0, 0);
    return ret;
}

#endif /* _IEFRAME_H_ */
