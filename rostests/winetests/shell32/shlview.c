/*
 * Unit test of the IShellView
 *
 * Copyright 2010 Nikolay Sivov for CodeWeavers
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
#define CONST_VTABLE

#include "windef.h"
#include "winbase.h"
#include "wtypes.h"
#include "shellapi.h"

#include "shlguid.h"
#include "shlobj.h"
#include "shobjidl.h"
#include "shlwapi.h"
#include "ocidl.h"
#include "oleauto.h"

#include "initguid.h"

#include "wine/test.h"

#include "msg.h"

#define LISTVIEW_SEQ_INDEX  0
#define NUM_MSG_SEQUENCES   1

DEFINE_GUID(IID_IPersistHistory, 0x91a565c1, 0xe38f, 0x11d0, 0x94, 0xbf, 0x00, 0xa0, 0xc9, 0x05, 0x5c, 0xbf);

static struct msg_sequence *sequences[NUM_MSG_SEQUENCES];

static LRESULT WINAPI listview_subclass_proc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    WNDPROC oldproc = (WNDPROC)GetWindowLongPtrA(hwnd, GWLP_USERDATA);
    static LONG defwndproc_counter = 0;
    LRESULT ret;
    struct message msg;

    trace("listview: %p, %04x, %08lx, %08lx\n", hwnd, message, wParam, lParam);

    msg.message = message;
    msg.flags = sent|wparam|lparam;
    if (defwndproc_counter) msg.flags |= defwinproc;
    msg.wParam = wParam;
    msg.lParam = lParam;
    add_message(sequences, LISTVIEW_SEQ_INDEX, &msg);

    defwndproc_counter++;
    ret = CallWindowProcA(oldproc, hwnd, message, wParam, lParam);
    defwndproc_counter--;
    return ret;
}

static HWND subclass_listview(HWND hwnd)
{
    WNDPROC oldproc;
    HWND listview;

    /* listview is a first child */
    listview = FindWindowExA(hwnd, NULL, WC_LISTVIEWA, NULL);
    if(!listview)
    {
        /* .. except for some versions of Windows XP, where things
           are slightly more complicated. */
        HWND hwnd_tmp;
        hwnd_tmp = FindWindowExA(hwnd, NULL, "DUIViewWndClassName", NULL);
        hwnd_tmp = FindWindowExA(hwnd_tmp, NULL, "DirectUIHWND", NULL);
        hwnd_tmp = FindWindowExA(hwnd_tmp, NULL, "CtrlNotifySink", NULL);
        listview = FindWindowExA(hwnd_tmp, NULL, WC_LISTVIEWA, NULL);
    }

    oldproc = (WNDPROC)SetWindowLongPtrA(listview, GWLP_WNDPROC,
                                        (LONG_PTR)listview_subclass_proc);
    SetWindowLongPtrA(listview, GWLP_USERDATA, (LONG_PTR)oldproc);

    return listview;
}

static UINT get_msg_count(struct msg_sequence **seq, int sequence_index, UINT message)
{
    struct msg_sequence *msg_seq = seq[sequence_index];
    UINT i, count = 0;

    for(i = 0; i < msg_seq->count ; i++)
        if(msg_seq->sequence[i].message == message)
            count++;

    return count;
}

/* Checks that every message in the sequence seq is also present in
 * the UINT array msgs */
static void verify_msgs_in_(struct msg_sequence *seq, const UINT *msgs,
                           const char *file, int line)
{
    UINT i, j, msg, failcount = 0;
    for(i = 0; i < seq->count; i++)
    {
        BOOL found = FALSE;
        msg = seq->sequence[i].message;
        for(j = 0; msgs[j] != 0; j++)
            if(msgs[j] == msg) found = TRUE;

        if(!found)
        {
            failcount++;
            trace("Unexpected message %d\n", msg);
        }
    }
    ok_(file, line) (!failcount, "%d failures.\n", failcount);
    flush_sequences(sequences, NUM_MSG_SEQUENCES);
}

#define verify_msgs_in(seq, msgs)               \
    verify_msgs_in_(seq, msgs, __FILE__, __LINE__)

/* dummy IDataObject implementation */
typedef struct {
    IDataObject IDataObject_iface;
    LONG ref;
} IDataObjectImpl;

static const IDataObjectVtbl IDataObjectImpl_Vtbl;

static inline IDataObjectImpl *impl_from_IDataObject(IDataObject *iface)
{
    return CONTAINING_RECORD(iface, IDataObjectImpl, IDataObject_iface);
}

static IDataObject* IDataObjectImpl_Construct(void)
{
    IDataObjectImpl *obj;

    obj = HeapAlloc(GetProcessHeap(), 0, sizeof(*obj));
    obj->IDataObject_iface.lpVtbl = &IDataObjectImpl_Vtbl;
    obj->ref = 1;

    return &obj->IDataObject_iface;
}

static HRESULT WINAPI IDataObjectImpl_QueryInterface(IDataObject *iface, REFIID riid, void **ppvObj)
{
    IDataObjectImpl *This = impl_from_IDataObject(iface);

    if (IsEqualIID(riid, &IID_IUnknown) ||
        IsEqualIID(riid, &IID_IDataObject))
    {
        *ppvObj = This;
    }

    if(*ppvObj)
    {
        IDataObject_AddRef(iface);
        return S_OK;
    }

    return E_NOINTERFACE;
}

static ULONG WINAPI IDataObjectImpl_AddRef(IDataObject * iface)
{
    IDataObjectImpl *This = impl_from_IDataObject(iface);
    return InterlockedIncrement(&This->ref);
}

static ULONG WINAPI IDataObjectImpl_Release(IDataObject * iface)
{
    IDataObjectImpl *This = impl_from_IDataObject(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    if (!ref)
    {
        HeapFree(GetProcessHeap(), 0, This);
        return 0;
    }
    return ref;
}

static HRESULT WINAPI IDataObjectImpl_GetData(IDataObject *iface, FORMATETC *pformat, STGMEDIUM *pmedium)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI IDataObjectImpl_GetDataHere(IDataObject *iface, FORMATETC *pformat, STGMEDIUM *pmedium)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI IDataObjectImpl_QueryGetData(IDataObject *iface, FORMATETC *pformat)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI IDataObjectImpl_GetCanonicalFormatEtc(
    IDataObject *iface, FORMATETC *pformatIn, FORMATETC *pformatOut)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI IDataObjectImpl_SetData(
    IDataObject *iface, FORMATETC *pformat, STGMEDIUM *pmedium, BOOL release)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI IDataObjectImpl_EnumFormatEtc(
    IDataObject *iface, DWORD direction, IEnumFORMATETC **ppenumFormatEtc)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI IDataObjectImpl_DAdvise(
    IDataObject *iface, FORMATETC *pformatetc, DWORD advf, IAdviseSink *pSink, DWORD *pConnection)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI IDataObjectImpl_DUnadvise(IDataObject *iface, DWORD connection)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI IDataObjectImpl_EnumDAdvise(IDataObject *iface, IEnumSTATDATA **ppenumAdvise)
{
    return E_NOTIMPL;
}

static const IDataObjectVtbl IDataObjectImpl_Vtbl =
{
    IDataObjectImpl_QueryInterface,
    IDataObjectImpl_AddRef,
    IDataObjectImpl_Release,
    IDataObjectImpl_GetData,
    IDataObjectImpl_GetDataHere,
    IDataObjectImpl_QueryGetData,
    IDataObjectImpl_GetCanonicalFormatEtc,
    IDataObjectImpl_SetData,
    IDataObjectImpl_EnumFormatEtc,
    IDataObjectImpl_DAdvise,
    IDataObjectImpl_DUnadvise,
    IDataObjectImpl_EnumDAdvise
};

/* dummy IShellBrowser implementation */
typedef struct {
    IShellBrowser IShellBrowser_iface;
    LONG ref;
} IShellBrowserImpl;

static const IShellBrowserVtbl IShellBrowserImpl_Vtbl;

static inline IShellBrowserImpl *impl_from_IShellBrowser(IShellBrowser *iface)
{
    return CONTAINING_RECORD(iface, IShellBrowserImpl, IShellBrowser_iface);
}

static IShellBrowser* IShellBrowserImpl_Construct(void)
{
    IShellBrowserImpl *browser;

    browser = HeapAlloc(GetProcessHeap(), 0, sizeof(*browser));
    browser->IShellBrowser_iface.lpVtbl = &IShellBrowserImpl_Vtbl;
    browser->ref = 1;

    return &browser->IShellBrowser_iface;
}

static HRESULT WINAPI IShellBrowserImpl_QueryInterface(IShellBrowser *iface,
                                            REFIID riid,
                                            LPVOID *ppvObj)
{
    IShellBrowserImpl *This = impl_from_IShellBrowser(iface);

    *ppvObj = NULL;

    if(IsEqualIID(riid, &IID_IUnknown)   ||
       IsEqualIID(riid, &IID_IOleWindow) ||
       IsEqualIID(riid, &IID_IShellBrowser))
    {
        *ppvObj = This;
    }

    if(*ppvObj)
    {
        IShellBrowser_AddRef(iface);
        return S_OK;
    }

    return E_NOINTERFACE;
}

static ULONG WINAPI IShellBrowserImpl_AddRef(IShellBrowser * iface)
{
    IShellBrowserImpl *This = impl_from_IShellBrowser(iface);
    return InterlockedIncrement(&This->ref);
}

static ULONG WINAPI IShellBrowserImpl_Release(IShellBrowser * iface)
{
    IShellBrowserImpl *This = impl_from_IShellBrowser(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    if (!ref)
    {
        HeapFree(GetProcessHeap(), 0, This);
        return 0;
    }
    return ref;
}

static HRESULT WINAPI IShellBrowserImpl_GetWindow(IShellBrowser *iface,
                                           HWND *phwnd)
{
    if (phwnd) *phwnd = GetDesktopWindow();
    return S_OK;
}

static HRESULT WINAPI IShellBrowserImpl_ContextSensitiveHelp(IShellBrowser *iface,
                                                      BOOL fEnterMode)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI IShellBrowserImpl_BrowseObject(IShellBrowser *iface,
                                              LPCITEMIDLIST pidl,
                                              UINT wFlags)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI IShellBrowserImpl_EnableModelessSB(IShellBrowser *iface,
                                              BOOL fEnable)

{
    return E_NOTIMPL;
}

static HRESULT WINAPI IShellBrowserImpl_GetControlWindow(IShellBrowser *iface,
                                              UINT id,
                                              HWND *lphwnd)

{
    return E_NOTIMPL;
}

static HRESULT WINAPI IShellBrowserImpl_GetViewStateStream(IShellBrowser *iface,
                                                DWORD mode,
                                                LPSTREAM *pStrm)

{
    return E_NOTIMPL;
}

static HRESULT WINAPI IShellBrowserImpl_InsertMenusSB(IShellBrowser *iface,
                                           HMENU hmenuShared,
                                           LPOLEMENUGROUPWIDTHS lpMenuWidths)

{
    return E_NOTIMPL;
}

static HRESULT WINAPI IShellBrowserImpl_OnViewWindowActive(IShellBrowser *iface,
                                                IShellView *ppshv)

{
    return E_NOTIMPL;
}

static HRESULT WINAPI IShellBrowserImpl_QueryActiveShellView(IShellBrowser *iface,
                                                  IShellView **ppshv)

{
    return E_NOTIMPL;
}

static HRESULT WINAPI IShellBrowserImpl_RemoveMenusSB(IShellBrowser *iface,
                                           HMENU hmenuShared)

{
    return E_NOTIMPL;
}

static HRESULT WINAPI IShellBrowserImpl_SendControlMsg(IShellBrowser *iface,
                                            UINT id,
                                            UINT uMsg,
                                            WPARAM wParam,
                                            LPARAM lParam,
                                            LRESULT *pret)

{
    return E_NOTIMPL;
}

static HRESULT WINAPI IShellBrowserImpl_SetMenuSB(IShellBrowser *iface,
                                       HMENU hmenuShared,
                                       HOLEMENU holemenuReserved,
                                       HWND hwndActiveObject)

{
    return E_NOTIMPL;
}

static HRESULT WINAPI IShellBrowserImpl_SetStatusTextSB(IShellBrowser *iface,
                                             LPCOLESTR lpszStatusText)

{
    return E_NOTIMPL;
}

static HRESULT WINAPI IShellBrowserImpl_SetToolbarItems(IShellBrowser *iface,
                                             LPTBBUTTON lpButtons,
                                             UINT nButtons,
                                             UINT uFlags)

{
    return E_NOTIMPL;
}

static HRESULT WINAPI IShellBrowserImpl_TranslateAcceleratorSB(IShellBrowser *iface,
                                                    LPMSG lpmsg,
                                                    WORD wID)

{
    return E_NOTIMPL;
}

static const IShellBrowserVtbl IShellBrowserImpl_Vtbl =
{
    IShellBrowserImpl_QueryInterface,
    IShellBrowserImpl_AddRef,
    IShellBrowserImpl_Release,
    IShellBrowserImpl_GetWindow,
    IShellBrowserImpl_ContextSensitiveHelp,
    IShellBrowserImpl_InsertMenusSB,
    IShellBrowserImpl_SetMenuSB,
    IShellBrowserImpl_RemoveMenusSB,
    IShellBrowserImpl_SetStatusTextSB,
    IShellBrowserImpl_EnableModelessSB,
    IShellBrowserImpl_TranslateAcceleratorSB,
    IShellBrowserImpl_BrowseObject,
    IShellBrowserImpl_GetViewStateStream,
    IShellBrowserImpl_GetControlWindow,
    IShellBrowserImpl_SendControlMsg,
    IShellBrowserImpl_QueryActiveShellView,
    IShellBrowserImpl_OnViewWindowActive,
    IShellBrowserImpl_SetToolbarItems
};

static const struct message empty_seq[] = {
    { 0 }
};

static const struct message folderview_getspacing_seq[] = {
    { LVM_GETITEMSPACING, wparam|sent, FALSE },
    { 0 }
};

static const struct message folderview_getselectionmarked_seq[] = {
    { LVM_GETSELECTIONMARK, sent },
    { 0 }
};

static const struct message folderview_getfocused_seq[] = {
    { LVM_GETNEXTITEM, sent|wparam|lparam|optional, -1, LVNI_FOCUSED },
    { 0 }
};

static HRESULT WINAPI shellbrowser_QueryInterface(IShellBrowser *iface, REFIID riid, void **ppv)
{
    *ppv = NULL;

    if (IsEqualGUID(&IID_IShellBrowser, riid) ||
        IsEqualGUID(&IID_IOleWindow, riid) ||
        IsEqualGUID(&IID_IUnknown, riid))
    {
        *ppv = iface;
    }

    if (*ppv)
    {
        IUnknown_AddRef((IUnknown*)*ppv);
        return S_OK;
    }

    return E_NOINTERFACE;
}

static ULONG WINAPI shellbrowser_AddRef(IShellBrowser *iface)
{
    return 2;
}

static ULONG WINAPI shellbrowser_Release(IShellBrowser *iface)
{
    return 1;
}

static HRESULT WINAPI shellbrowser_GetWindow(IShellBrowser *iface, HWND *phwnd)
{
    *phwnd = GetDesktopWindow();
    return S_OK;
}

static HRESULT WINAPI shellbrowser_ContextSensitiveHelp(IShellBrowser *iface, BOOL mode)
{
    ok(0, "unexpected\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI shellbrowser_InsertMenusSB(IShellBrowser *iface, HMENU hmenuShared,
    OLEMENUGROUPWIDTHS *menuwidths)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI shellbrowser_SetMenuSB(IShellBrowser *iface, HMENU hmenuShared,
    HOLEMENU holemenuReserved, HWND hwndActiveObject)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI shellbrowser_RemoveMenusSB(IShellBrowser *iface, HMENU hmenuShared)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI shellbrowser_SetStatusTextSB(IShellBrowser *iface, LPCOLESTR text)
{
    ok(0, "unexpected\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI shellbrowser_EnableModelessSB(IShellBrowser *iface, BOOL enable)
{
    ok(0, "unexpected\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI shellbrowser_TranslateAcceleratorSB(IShellBrowser *iface, MSG *pmsg, WORD wID)
{
    ok(0, "unexpected\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI shellbrowser_BrowseObject(IShellBrowser *iface, LPCITEMIDLIST pidl, UINT flags)
{
    ok(0, "unexpected\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI shellbrowser_GetViewStateStream(IShellBrowser *iface, DWORD mode, IStream **stream)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI shellbrowser_GetControlWindow(IShellBrowser *iface, UINT id, HWND *phwnd)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI shellbrowser_SendControlMsg(IShellBrowser *iface, UINT id, UINT uMsg,
    WPARAM wParam, LPARAM lParam, LRESULT *pret)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI shellbrowser_QueryActiveShellView(IShellBrowser *iface, IShellView **view)
{
    ok(0, "unexpected\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI shellbrowser_OnViewWindowActive(IShellBrowser *iface, IShellView *view)
{
    ok(0, "unexpected\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI shellbrowser_SetToolbarItems(IShellBrowser *iface, LPTBBUTTONSB buttons,
    UINT count, UINT flags)
{
    return E_NOTIMPL;
}

static const IShellBrowserVtbl shellbrowservtbl = {
    shellbrowser_QueryInterface,
    shellbrowser_AddRef,
    shellbrowser_Release,
    shellbrowser_GetWindow,
    shellbrowser_ContextSensitiveHelp,
    shellbrowser_InsertMenusSB,
    shellbrowser_SetMenuSB,
    shellbrowser_RemoveMenusSB,
    shellbrowser_SetStatusTextSB,
    shellbrowser_EnableModelessSB,
    shellbrowser_TranslateAcceleratorSB,
    shellbrowser_BrowseObject,
    shellbrowser_GetViewStateStream,
    shellbrowser_GetControlWindow,
    shellbrowser_SendControlMsg,
    shellbrowser_QueryActiveShellView,
    shellbrowser_OnViewWindowActive,
    shellbrowser_SetToolbarItems
};

static IShellBrowser test_shellbrowser = { &shellbrowservtbl };

static void test_CreateViewWindow(void)
{
    IShellFolder *desktop;
    HWND hwnd_view, hwnd2;
    FOLDERSETTINGS settings;
    IShellView *view;
    IDropTarget *dt;
    HRESULT hr;
    RECT r = {0};
    ULONG ref1, ref2;

    hr = SHGetDesktopFolder(&desktop);
    ok(hr == S_OK, "got (0x%08x)\n", hr);

    hr = IShellFolder_CreateViewObject(desktop, NULL, &IID_IShellView, (void**)&view);
    ok(hr == S_OK, "got (0x%08x)\n", hr);

if (0)
{
    /* crashes on native */
    IShellView_CreateViewWindow(view, NULL, &settings, NULL, NULL, NULL);
}

    settings.ViewMode = FVM_ICON;
    settings.fFlags = 0;
    hwnd_view = (HWND)0xdeadbeef;
    hr = IShellView_CreateViewWindow(view, NULL, &settings, NULL, NULL, &hwnd_view);
    ok(hr == E_UNEXPECTED, "got (0x%08x)\n", hr);
    ok(hwnd_view == 0, "got %p\n", hwnd_view);

    hwnd_view = (HWND)0xdeadbeef;
    hr = IShellView_CreateViewWindow(view, NULL, &settings, NULL, &r, &hwnd_view);
    ok(hr == E_UNEXPECTED, "got (0x%08x)\n", hr);
    ok(hwnd_view == 0, "got %p\n", hwnd_view);

    hwnd_view = NULL;
    hr = IShellView_CreateViewWindow(view, NULL, &settings, &test_shellbrowser, &r, &hwnd_view);
    ok(hr == S_OK || broken(hr == S_FALSE), "got (0x%08x)\n", hr);
    ok(hwnd_view != 0, "got %p\n", hwnd_view);

    hwnd2 = (HWND)0xdeadbeef;
    hr = IShellView_CreateViewWindow(view, NULL, &settings, &test_shellbrowser, &r, &hwnd2);
    ok(hr == E_UNEXPECTED, "got (0x%08x)\n", hr);
    ok(hwnd2 == NULL, "got %p\n", hwnd2);

    /* ::DragLeave without drag operation */
    hr = IShellView_QueryInterface(view, &IID_IDropTarget, (void**)&dt);
    ok(hr == S_OK, "got (0x%08x)\n", hr);
    hr = IDropTarget_DragLeave(dt);
    ok(hr == S_OK, "got (0x%08x)\n", hr);
    IDropTarget_Release(dt);

    IShellView_AddRef(view);
    ref1 = IShellView_Release(view);
    hr = IShellView_DestroyViewWindow(view);
    ok(hr == S_OK, "got (0x%08x)\n", hr);
    ok(!IsWindow(hwnd_view), "hwnd %p still valid\n", hwnd_view);
    ref2 = IShellView_Release(view);
    ok(ref1 > ref2, "expected %u > %u\n", ref1, ref2);
    ref1 = ref2;

    /* Show that releasing the shell view does not destroy the window */
    hr = IShellFolder_CreateViewObject(desktop, NULL, &IID_IShellView, (void**)&view);
    ok(hr == S_OK, "got (0x%08x)\n", hr);
    hwnd_view = NULL;
    hr = IShellView_CreateViewWindow(view, NULL, &settings, &test_shellbrowser, &r, &hwnd_view);
    ok(hr == S_OK || broken(hr == S_FALSE), "got (0x%08x)\n", hr);
    ok(hwnd_view != NULL, "got %p\n", hwnd_view);
    ok(IsWindow(hwnd_view), "hwnd %p still valid\n", hwnd_view);
    ref2 = IShellView_Release(view);
    ok(ref2 != 0, "ref2 = %u\n", ref2);
    ok(ref2 > ref1, "expected %u > %u\n", ref2, ref1);
    ok(IsWindow(hwnd_view), "hwnd %p still valid\n", hwnd_view);
    DestroyWindow(hwnd_view);

    IShellFolder_Release(desktop);
}

static void test_IFolderView(void)
{
    IShellFolder *desktop, *folder;
    FOLDERSETTINGS settings;
    IShellView *view;
    IShellBrowser *browser;
    IFolderView2 *fv2;
    IFolderView *fv;
    IUnknown *unk;
    HWND hwnd_view, hwnd_list;
    PITEMID_CHILD pidl;
    HRESULT hr;
    INT ret, count;
    POINT pt;
    RECT r;

    hr = SHGetDesktopFolder(&desktop);
    ok(hr == S_OK, "got (0x%08x)\n", hr);

    hr = IShellFolder_CreateViewObject(desktop, NULL, &IID_IShellView, (void**)&view);
    ok(hr == S_OK, "got (0x%08x)\n", hr);

    hr = IShellView_QueryInterface(view, &IID_IFolderView, (void**)&fv);
    if (hr != S_OK)
    {
        win_skip("IFolderView not supported by desktop folder\n");
        IShellView_Release(view);
        IShellFolder_Release(desktop);
        return;
    }

    /* call methods before window creation */
    hr = IFolderView_GetSpacing(fv, NULL);
    ok(hr == S_FALSE || broken(hr == S_OK) /* win7 */, "got (0x%08x)\n", hr);

    pidl = (void*)0xdeadbeef;
    hr = IFolderView_Item(fv, 0, &pidl);
    ok(hr == E_INVALIDARG || broken(hr == E_FAIL) /* < Vista */, "got (0x%08x)\n", hr);
    ok(pidl == 0 || broken(pidl == (void*)0xdeadbeef) /* < Vista */, "got %p\n", pidl);

if (0)
{
    /* crashes on Vista and Win2k8 - List not created yet case */
    IFolderView_GetSpacing(fv, &pt);

    /* crashes on XP */
    IFolderView_GetSelectionMarkedItem(fv, NULL);
    IFolderView_GetFocusedItem(fv, NULL);

    /* crashes on Vista+ */
    IFolderView_Item(fv, 0, NULL);
}

    browser = IShellBrowserImpl_Construct();

    settings.ViewMode = FVM_ICON;
    settings.fFlags = 0;
    hwnd_view = (HWND)0xdeadbeef;
    r.left = r.top = 0;
    r.right = r.bottom = 100;
    hr = IShellView_CreateViewWindow(view, NULL, &settings, browser, &r, &hwnd_view);
    ok(hr == S_OK, "got (0x%08x)\n", hr);
    ok(IsWindow(hwnd_view), "got %p\n", hwnd_view);

    hwnd_list = subclass_listview(hwnd_view);
    if (!hwnd_list)
    {
        win_skip("Failed to subclass ListView control\n");
        IShellBrowser_Release(browser);
        IFolderView_Release(fv);
        IShellView_Release(view);
        IShellFolder_Release(desktop);
        return;
    }

    /* IFolderView::GetSpacing */
    flush_sequences(sequences, NUM_MSG_SEQUENCES);
    hr = IFolderView_GetSpacing(fv, NULL);
    ok(hr == S_OK, "got (0x%08x)\n", hr);
    ok_sequence(sequences, LISTVIEW_SEQ_INDEX, empty_seq, "IFolderView::GetSpacing, empty", FALSE);

    flush_sequences(sequences, NUM_MSG_SEQUENCES);
    hr = IFolderView_GetSpacing(fv, &pt);
    ok(hr == S_OK, "got (0x%08x)\n", hr);
    /* fails with empty sequence on win7 for unknown reason */
    if (sequences[LISTVIEW_SEQ_INDEX]->count)
    {
        ok_sequence(sequences, LISTVIEW_SEQ_INDEX, folderview_getspacing_seq, "IFolderView::GetSpacing", FALSE);
        ok(pt.x > 0, "got %d\n", pt.x);
        ok(pt.y > 0, "got %d\n", pt.y);
        ret = SendMessageA(hwnd_list, LVM_GETITEMSPACING, 0, 0);
        ok(pt.x == LOWORD(ret) && pt.y == HIWORD(ret), "got (%d, %d)\n", LOWORD(ret), HIWORD(ret));
    }

    /* IFolderView::ItemCount */
if (0)
{
    /* crashes on XP */
    IFolderView_ItemCount(fv, SVGIO_ALLVIEW, NULL);
}

    flush_sequences(sequences, NUM_MSG_SEQUENCES);
    IFolderView_ItemCount(fv, SVGIO_ALLVIEW, &count);

    /* IFolderView::GetSelectionMarkedItem */
if (0)
{
    /* crashes on XP */
    IFolderView_GetSelectionMarkedItem(fv, NULL);
}

    flush_sequences(sequences, NUM_MSG_SEQUENCES);
    hr = IFolderView_GetSelectionMarkedItem(fv, &ret);
    if (count)
        ok(hr == S_OK, "got (0x%08x)\n", hr);
    else
        ok(hr == S_FALSE, "got (0x%08x)\n", hr);
    ok_sequence(sequences, LISTVIEW_SEQ_INDEX, folderview_getselectionmarked_seq,
                "IFolderView::GetSelectionMarkedItem", FALSE);

    /* IFolderView::GetFocusedItem */
    flush_sequences(sequences, NUM_MSG_SEQUENCES);
    hr = IFolderView_GetFocusedItem(fv, &ret);
    if (count)
        ok(hr == S_OK, "got (0x%08x)\n", hr);
    else
        ok(hr == S_FALSE, "got (0x%08x)\n", hr);
    ok_sequence(sequences, LISTVIEW_SEQ_INDEX, folderview_getfocused_seq,
                "IFolderView::GetFocusedItem", FALSE);

    /* IFolderView::GetFolder, just return pointer */
if (0)
{
    /* crashes on XP */
    IFolderView_GetFolder(fv, NULL, (void**)&folder);
    IFolderView_GetFolder(fv, NULL, NULL);
}

    hr = IFolderView_GetFolder(fv, &IID_IShellFolder, NULL);
    ok(hr == E_POINTER, "got (0x%08x)\n", hr);

    hr = IFolderView_GetFolder(fv, &IID_IShellFolder, (void**)&folder);
    ok(hr == S_OK, "got (0x%08x)\n", hr);
    ok(desktop == folder, "\n");
    if (folder) IShellFolder_Release(folder);

    hr = IFolderView_GetFolder(fv, &IID_IUnknown, (void**)&unk);
    ok(hr == S_OK, "got (0x%08x)\n", hr);
    if (unk) IUnknown_Release(unk);

    hr = IFolderView_QueryInterface(fv, &IID_IFolderView2, (void**)&fv2);
    if (hr != S_OK)
        win_skip("IFolderView2 is not supported.\n");
    if (fv2) IFolderView2_Release(fv2);

    hr = IShellView_DestroyViewWindow(view);
    ok(hr == S_OK, "got (0x%08x)\n", hr);
    ok(!IsWindow(hwnd_view), "hwnd %p still valid\n", hwnd_view);

    IShellBrowser_Release(browser);
    IFolderView_Release(fv);
    IShellView_Release(view);
    IShellFolder_Release(desktop);
}

static void test_GetItemObject(void)
{
    IShellFolder *desktop;
    IShellView *view;
    IUnknown *unk;
    HRESULT hr;

    hr = SHGetDesktopFolder(&desktop);
    ok(hr == S_OK, "got (0x%08x)\n", hr);

    hr = IShellFolder_CreateViewObject(desktop, NULL, &IID_IShellView, (void**)&view);
    ok(hr == S_OK, "got (0x%08x)\n", hr);

    /* from documentation three interfaces are supported for SVGIO_BACKGROUND:
       IContextMenu, IDispatch, IPersistHistory */
    hr = IShellView_GetItemObject(view, SVGIO_BACKGROUND, &IID_IContextMenu, (void**)&unk);
    ok(hr == S_OK, "got (0x%08x)\n", hr);
    IUnknown_Release(unk);

    unk = NULL;
    hr = IShellView_GetItemObject(view, SVGIO_BACKGROUND, &IID_IDispatch, (void**)&unk);
    ok(hr == S_OK || broken(hr == E_NOTIMPL) /* NT4 */, "got (0x%08x)\n", hr);
    if (unk) IUnknown_Release(unk);

    unk = NULL;
    hr = IShellView_GetItemObject(view, SVGIO_BACKGROUND, &IID_IPersistHistory, (void**)&unk);
    todo_wine ok(hr == S_OK || broken(hr == E_NOTIMPL) /* W9x, NT4 */, "got (0x%08x)\n", hr);
    if (unk) IUnknown_Release(unk);

    /* example of unsupported interface, base for IPersistHistory */
    hr = IShellView_GetItemObject(view, SVGIO_BACKGROUND, &IID_IPersist, (void**)&unk);
    ok(hr == E_NOINTERFACE || broken(hr == E_NOTIMPL) /* W2K */, "got (0x%08x)\n", hr);

    IShellView_Release(view);
    IShellFolder_Release(desktop);
}

static void test_IShellFolderView(void)
{
    IShellFolderView *folderview;
    IShellFolder *desktop;
    IShellView *view;
    IDataObject *obj;
    UINT i;
    HRESULT hr;

    hr = SHGetDesktopFolder(&desktop);
    ok(hr == S_OK, "got (0x%08x)\n", hr);

    hr = IShellFolder_CreateViewObject(desktop, NULL, &IID_IShellView, (void**)&view);
    ok(hr == S_OK, "got (0x%08x)\n", hr);

    hr = IShellView_QueryInterface(view, &IID_IShellFolderView, (void**)&folderview);
    if (hr != S_OK)
    {
        win_skip("IShellView doesn't provide IShellFolderView on this platform\n");
        IShellView_Release(view);
        IShellFolder_Release(desktop);
        return;
    }

    /* ::MoveIcons */
    obj = IDataObjectImpl_Construct();
    hr = IShellFolderView_MoveIcons(folderview, obj);
    ok(hr == E_NOTIMPL || broken(hr == S_OK) /* W98 */, "got (0x%08x)\n", hr);
    IDataObject_Release(obj);

    /* ::SetRedraw without list created */
    hr = IShellFolderView_SetRedraw(folderview, TRUE);
    ok(hr == S_OK, "got (0x%08x)\n", hr);

    /* ::QuerySupport */
    hr = IShellFolderView_QuerySupport(folderview, NULL);
    ok(hr == S_OK, "got (0x%08x)\n", hr);
    i = 0xdeadbeef;
    hr = IShellFolderView_QuerySupport(folderview, &i);
    ok(hr == S_OK, "got (0x%08x)\n", hr);
    ok(i == 0xdeadbeef, "got %d\n", i);

    /* ::RemoveObject */
    i = 0xdeadbeef;
    hr = IShellFolderView_RemoveObject(folderview, NULL, &i);
    ok(hr == S_OK || hr == E_FAIL, "got (0x%08x)\n", hr);
    if (hr == S_OK) ok(i == 0 || broken(i == 0xdeadbeef) /* Vista, 2k8 */,
                       "got %d\n", i);

    IShellFolderView_Release(folderview);

    IShellView_Release(view);
    IShellFolder_Release(desktop);
}

static void test_IOleWindow(void)
{
    IShellFolder *desktop;
    IShellView *view;
    IOleWindow *wnd;
    HRESULT hr;

    hr = SHGetDesktopFolder(&desktop);
    ok(hr == S_OK, "got (0x%08x)\n", hr);

    hr = IShellFolder_CreateViewObject(desktop, NULL, &IID_IShellView, (void**)&view);
    ok(hr == S_OK, "got (0x%08x)\n", hr);

    hr = IShellView_QueryInterface(view, &IID_IOleWindow, (void**)&wnd);
    ok(hr == E_NOINTERFACE, "got (0x%08x)\n", hr);

    /* IShellView::ContextSensitiveHelp */
    hr = IShellView_ContextSensitiveHelp(view, TRUE);
    ok(hr == E_NOTIMPL, "got (0x%08x)\n", hr);
    hr = IShellView_ContextSensitiveHelp(view, FALSE);
    ok(hr == E_NOTIMPL, "got (0x%08x)\n", hr);

    IShellView_Release(view);
    IShellFolder_Release(desktop);
}

static const struct message folderview_setcurrentviewmode1_2_prevista[] = {
    { LVM_SETVIEW, sent|wparam, LV_VIEW_ICON},
    { LVM_SETIMAGELIST, sent|wparam, 0},
    { LVM_SETIMAGELIST, sent|wparam, 1},
    { 0x105a, sent},
    { LVM_SETBKIMAGEW, sent|optional},    /* w2k3 */
    { LVM_GETBKCOLOR, sent|optional},     /* w2k3 */
    { LVM_GETTEXTBKCOLOR, sent|optional}, /* w2k3 */
    { LVM_GETTEXTCOLOR, sent|optional},   /* w2k3 */
    { LVM_SETEXTENDEDLISTVIEWSTYLE, sent|optional|wparam, 0xc8}, /* w2k3 */
    { LVM_ARRANGE, sent },
    { LVM_ARRANGE, sent|optional },       /* WinXP */
    { 0 }
};

static const struct message folderview_setcurrentviewmode3_prevista[] = {
    { LVM_SETVIEW, sent|wparam, LV_VIEW_LIST},
    { LVM_SETIMAGELIST, sent|wparam, 0},
    { LVM_SETIMAGELIST, sent|wparam, 1},
    { 0x105a, sent},
    { LVM_SETBKIMAGEW, sent|optional},    /* w2k3 */
    { LVM_GETBKCOLOR, sent|optional},     /* w2k3 */
    { LVM_GETTEXTBKCOLOR, sent|optional}, /* w2k3 */
    { LVM_GETTEXTCOLOR, sent|optional},   /* w2k3 */
    { LVM_SETEXTENDEDLISTVIEWSTYLE, sent|optional|wparam, 0xc8}, /* w2k3 */
    { 0 }
};

static const struct message folderview_setcurrentviewmode4_prevista[] = {
    { LVM_GETHEADER, sent},
    { LVM_GETITEMCOUNT, sent|optional },
    { LVM_SETSELECTEDCOLUMN, sent},
    { WM_NOTIFY, sent },
    { WM_NOTIFY, sent },
    { WM_NOTIFY, sent },
    { WM_NOTIFY, sent },
    { LVM_SETVIEW, sent|wparam, LV_VIEW_DETAILS},
    { LVM_SETIMAGELIST, sent|wparam, 0},
    { LVM_SETIMAGELIST, sent|wparam, 1},
    { 0x105a, sent},
    { LVM_SETBKIMAGEW, sent|optional},    /* w2k3 */
    { LVM_GETBKCOLOR, sent|optional},     /* w2k3 */
    { LVM_GETTEXTBKCOLOR, sent|optional}, /* w2k3 */
    { LVM_GETTEXTCOLOR, sent|optional},   /* w2k3 */
    { LVM_SETEXTENDEDLISTVIEWSTYLE, sent|optional|wparam, 0xc8}, /* w2k3 */
    { 0 }
};

/* XP, SetCurrentViewMode(5)
   108e - LVM_SETVIEW (LV_VIEW_ICON);
   1036 - LVM_SETEXTEDEDLISTVIEWSTYLE (0x8000, 0)
   100c/104c repeated X times
   1003 - LVM_SETIMAGELIST
   1035 - LVM_SETICONSPACING
   1004 - LVM_GETITEMCOUNT
   105a - ?
   1016 - LVM_ARRANGE
   1016 - LVM_ARRANGE
*/

/* XP, SetCurrentViewMode(6)
   1036 - LVM_SETEXTENDEDLISTVIEWSTYLE (0x8000, 0)
   1035 - LVM_SETICONSPACING
   1003 - LVM_SETIMAGELIST
   1003 - LVM_SETIMAGELIST
   100c/104c repeated X times
   10a2 - LVM_SETTILEVIEWINFO
   108e - LVM_SETVIEW (LV_VIEW_TILE)
   1003 - LVM_SETIMAGELIST
   105a - ?
   1016 - LVM_ARRANGE
   1016 - LVM_ARRANGE
*/

/* XP, SetCurrentViewMode (7)
   10a2 - LVM_SETTILEVIEWINFO
   108e - LVM_SETVIEW (LV_VIEW_ICON)
   1004/10a4 (LVM_GETITEMCOUNT/LVM_SETTILEINFO) X times
   1016 - LVM_ARRANGE
   1016 - LVM_ARRANGE
   ...
   LVM_SETEXTENDEDLISTVIEWSTYLE (0x40000, 0x40000)
   ...
   LVM_SETEXTENDEDLISTVIEWSTYLE (0x8000, 0x8000)
*/

static void test_GetSetCurrentViewMode(void)
{
    IShellFolder *desktop;
    IShellView *sview;
    IFolderView *fview;
    IShellBrowser *browser;
    FOLDERSETTINGS fs;
    UINT viewmode;
    HWND hwnd;
    RECT rc = {0, 0, 10, 10};
    HRESULT hr;
    UINT i;
    static const int winxp_res[11] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    static const int win2k3_res[11] = {0, 1, 2, 3, 4, 5, 6, 5, 8, 0, 0};
    static const int vista_res[11] = {0, 1, 5, 3, 4, 5, 6, 7, 7, 0, 0};
    static const int win7_res[11] = {1, 1, 1, 3, 4, 1, 6, 1, 8, 8, 8};

    hr = SHGetDesktopFolder(&desktop);
    ok(hr == S_OK, "got (0x%08x)\n", hr);

    hr = IShellFolder_CreateViewObject(desktop, NULL, &IID_IShellView, (void**)&sview);
    ok(hr == S_OK, "got (0x%08x)\n", hr);

    fs.ViewMode = 1;
    fs.fFlags = 0;
    browser = IShellBrowserImpl_Construct();
    hr = IShellView_CreateViewWindow(sview, NULL, &fs, browser, &rc, &hwnd);
    ok(hr == S_OK || broken(hr == S_FALSE /*Win2k*/ ), "got (0x%08x)\n", hr);

    hr = IShellView_QueryInterface(sview, &IID_IFolderView, (void**)&fview);
    ok(hr == S_OK || broken(hr == E_NOINTERFACE), "got (0x%08x)\n", hr);
    if(SUCCEEDED(hr))
    {
        HWND hwnd_lv;
        UINT count;

        if (0)
        {
            /* Crashes under Win7/WinXP */
            IFolderView_GetCurrentViewMode(fview, NULL);
        }

        hr = IFolderView_GetCurrentViewMode(fview, &viewmode);
        ok(hr == S_OK, "got (0x%08x)\n", hr);
        ok(viewmode == 1, "ViewMode was %d\n", viewmode);

        hr = IFolderView_SetCurrentViewMode(fview, FVM_AUTO);
        ok(hr == S_OK, "got (0x%08x)\n", hr);

        hr = IFolderView_SetCurrentViewMode(fview, 0);
        ok(hr == E_INVALIDARG || broken(hr == S_OK),
           "got (0x%08x)\n", hr);

        hr = IFolderView_GetCurrentViewMode(fview, &viewmode);
        ok(hr == S_OK, "got (0x%08x)\n", hr);

        for(i = 1; i < 9; i++)
        {
            hr = IFolderView_SetCurrentViewMode(fview, i);
            ok(hr == S_OK || (i == 8 && hr == E_INVALIDARG /*Vista*/),
               "(%d) got (0x%08x)\n", i, hr);

            hr = IFolderView_GetCurrentViewMode(fview, &viewmode);
            ok(hr == S_OK, "(%d) got (0x%08x)\n", i, hr);

            /* Wine currently behaves like winxp here. */
            ok((viewmode == win7_res[i]) || (viewmode == vista_res[i]) ||
               (viewmode == win2k3_res[i]) || (viewmode == winxp_res[i]),
               "(%d) got %d\n",i , viewmode);
        }

        hr = IFolderView_SetCurrentViewMode(fview, 9);
        ok(hr == E_INVALIDARG || broken(hr == S_OK),
           "got (0x%08x)\n", hr);

        /* Test messages */
        hwnd_lv = subclass_listview(hwnd);
        ok(hwnd_lv != NULL, "Failed to subclass listview\n");
        if(hwnd_lv)
        {
            /* Vista seems to set the viewmode by other means than
               sending messages. At least no related messages are
               captured by subclassing.
            */
            BOOL vista_plus = FALSE;
            static const UINT vista_plus_msgs[] = {
                WM_SETREDRAW, WM_NOTIFY, WM_NOTIFYFORMAT, WM_QUERYUISTATE,
                WM_MENUCHAR, WM_WINDOWPOSCHANGING, WM_NCCALCSIZE, WM_WINDOWPOSCHANGED,
                WM_PARENTNOTIFY, LVM_GETHEADER, 0 };

            flush_sequences(sequences, NUM_MSG_SEQUENCES);
            hr = IFolderView_SetCurrentViewMode(fview, 1);
            ok(hr == S_OK, "got 0x%08x\n", hr);

            /* WM_SETREDRAW is not sent in versions before Vista. */
            vista_plus = get_msg_count(sequences, LISTVIEW_SEQ_INDEX, WM_SETREDRAW);
            if(vista_plus)
                verify_msgs_in(sequences[LISTVIEW_SEQ_INDEX], vista_plus_msgs);
            else
                ok_sequence(sequences, LISTVIEW_SEQ_INDEX, folderview_setcurrentviewmode1_2_prevista,
                            "IFolderView::SetCurrentViewMode(1)", TRUE);

            hr = IFolderView_SetCurrentViewMode(fview, 2);
            ok(hr == S_OK, "got 0x%08x\n", hr);
            if(vista_plus)
                verify_msgs_in(sequences[LISTVIEW_SEQ_INDEX], vista_plus_msgs);
            else
                ok_sequence(sequences, LISTVIEW_SEQ_INDEX, folderview_setcurrentviewmode1_2_prevista,
                            "IFolderView::SetCurrentViewMode(2)", TRUE);

            hr = IFolderView_SetCurrentViewMode(fview, 3);
            ok(hr == S_OK, "got 0x%08x\n", hr);
            if(vista_plus)
                verify_msgs_in(sequences[LISTVIEW_SEQ_INDEX], vista_plus_msgs);
            else
                ok_sequence(sequences, LISTVIEW_SEQ_INDEX, folderview_setcurrentviewmode3_prevista,
                            "IFolderView::SetCurrentViewMode(3)", TRUE);

            hr = IFolderView_SetCurrentViewMode(fview, 4);
            ok(hr == S_OK, "got 0x%08x\n", hr);
            if(vista_plus)
                verify_msgs_in(sequences[LISTVIEW_SEQ_INDEX], vista_plus_msgs);
            else
                ok_sequence(sequences, LISTVIEW_SEQ_INDEX, folderview_setcurrentviewmode4_prevista,
                            "IFolderView::SetCurrentViewMode(4)", TRUE);

            hr = IFolderView_SetCurrentViewMode(fview, 5);
            ok(hr == S_OK, "got 0x%08x\n", hr);
            todo_wine
            {
                if(vista_plus)
                {
                    verify_msgs_in(sequences[LISTVIEW_SEQ_INDEX], vista_plus_msgs);
                }
                else
                {
                    count = get_msg_count(sequences, LISTVIEW_SEQ_INDEX, LVM_SETVIEW);
                    ok(count == 1, "LVM_SETVIEW sent %d times.\n", count);
                    count = get_msg_count(sequences, LISTVIEW_SEQ_INDEX, LVM_SETEXTENDEDLISTVIEWSTYLE);
                    ok(count == 1 || count == 2, "LVM_SETEXTENDEDLISTVIEWSTYLE sent %d times.\n", count);
                    flush_sequences(sequences, NUM_MSG_SEQUENCES);
                }
            }

            hr = IFolderView_SetCurrentViewMode(fview, 6);
            ok(hr == S_OK, "got 0x%08x\n", hr);
            todo_wine
            {
                if(vista_plus)
                {
                    verify_msgs_in(sequences[LISTVIEW_SEQ_INDEX], vista_plus_msgs);
                }
                else
                {
                    count = get_msg_count(sequences, LISTVIEW_SEQ_INDEX, LVM_SETVIEW);
                    ok(count == 1, "LVM_SETVIEW sent %d times.\n", count);
                    count = get_msg_count(sequences, LISTVIEW_SEQ_INDEX, LVM_SETEXTENDEDLISTVIEWSTYLE);
                    ok(count == 1 || count == 2, "LVM_SETEXTENDEDLISTVIEWSTYLE sent %d times.\n", count);
                    flush_sequences(sequences, NUM_MSG_SEQUENCES);
                }
            }

            hr = IFolderView_SetCurrentViewMode(fview, 7);
            ok(hr == S_OK, "got 0x%08x\n", hr);
            todo_wine
            {
                if(vista_plus)
                {
                    verify_msgs_in(sequences[LISTVIEW_SEQ_INDEX], vista_plus_msgs);
                }
                else
                {
                    count = get_msg_count(sequences, LISTVIEW_SEQ_INDEX, LVM_SETVIEW);
                    ok(count == 1, "LVM_SETVIEW sent %d times.\n", count);
                    count = get_msg_count(sequences, LISTVIEW_SEQ_INDEX, LVM_SETEXTENDEDLISTVIEWSTYLE);
                    ok(count == 2, "LVM_SETEXTENDEDLISTVIEWSTYLE sent %d times.\n", count);
                    flush_sequences(sequences, NUM_MSG_SEQUENCES);
                }
            }

            hr = IFolderView_SetCurrentViewMode(fview, 8);
            ok(hr == S_OK || broken(hr == E_INVALIDARG /* Vista */), "got 0x%08x\n", hr);
            todo_wine
            {
                if(vista_plus)
                {
                    verify_msgs_in(sequences[LISTVIEW_SEQ_INDEX], vista_plus_msgs);
                }
                else
                {
                    count = get_msg_count(sequences, LISTVIEW_SEQ_INDEX, LVM_SETVIEW);
                    ok(count == 1, "LVM_SETVIEW sent %d times.\n", count);
                    count = get_msg_count(sequences, LISTVIEW_SEQ_INDEX, LVM_SETEXTENDEDLISTVIEWSTYLE);
                    ok(count == 2, "LVM_SETEXTENDEDLISTVIEWSTYLE sent %d times.\n", count);
                    flush_sequences(sequences, NUM_MSG_SEQUENCES);
                }
            }

            hr = IFolderView_GetCurrentViewMode(fview, &viewmode);
            ok(hr == S_OK, "Failed to get current viewmode.\n");
            ok_sequence(sequences, LISTVIEW_SEQ_INDEX, empty_seq,
                        "IFolderView::GetCurrentViewMode", FALSE);
        }

        IFolderView_Release(fview);
    }
    else
    {
        skip("No IFolderView for the desktop folder.\n");
    }

    IShellBrowser_Release(browser);
    IShellView_DestroyViewWindow(sview);
    IShellView_Release(sview);
    IShellFolder_Release(desktop);
}

static void test_IOleCommandTarget(void)
{
    IShellFolder *psf_desktop;
    IShellView *psv;
    IOleCommandTarget *poct;
    HRESULT hr;

    hr = SHGetDesktopFolder(&psf_desktop);
    ok(hr == S_OK, "got (0x%08x)\n", hr);

    hr = IShellFolder_CreateViewObject(psf_desktop, NULL, &IID_IShellView, (void**)&psv);
    ok(hr == S_OK, "got (0x%08x)\n", hr);
    if(SUCCEEDED(hr))
    {
        hr = IShellView_QueryInterface(psv, &IID_IOleCommandTarget, (void**)&poct);
        ok(hr == S_OK || broken(hr == E_NOINTERFACE) /* Win95/NT4 */, "Got 0x%08x\n", hr);
        if(SUCCEEDED(hr))
        {
            OLECMD oc;

            hr = IOleCommandTarget_QueryStatus(poct, NULL, 0, NULL, NULL);
            ok(hr == E_INVALIDARG, "Got 0x%08x\n", hr);

            oc.cmdID = 1;
            hr = IOleCommandTarget_QueryStatus(poct, NULL, 0, &oc, NULL);
            ok(hr == OLECMDERR_E_UNKNOWNGROUP, "Got 0x%08x\n", hr);

            oc.cmdID = 1;
            hr = IOleCommandTarget_QueryStatus(poct, NULL, 1, &oc, NULL);
            ok(hr == OLECMDERR_E_UNKNOWNGROUP, "Got 0x%08x\n", hr);

            hr = IOleCommandTarget_Exec(poct, NULL, 0, 0, NULL, NULL);
            ok(hr == OLECMDERR_E_UNKNOWNGROUP, "Got 0x%08x\n", hr);

            IOleCommandTarget_Release(poct);
        }

        IShellView_Release(psv);
    }

    IShellFolder_Release(psf_desktop);
}

START_TEST(shlview)
{
    OleInitialize(NULL);

    init_msg_sequences(sequences, NUM_MSG_SEQUENCES);

    test_CreateViewWindow();
    test_IFolderView();
    test_GetItemObject();
    test_IShellFolderView();
    test_IOleWindow();
    test_GetSetCurrentViewMode();
    test_IOleCommandTarget();

    OleUninitialize();
}
