/*
 * oledlg tests
 *
 * Copyright 2015 Nikolay Sivov for CodeWeavers
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

#include "wine/test.h"
#include <stdio.h>

#include "initguid.h"
#include "oledlg.h"

static HRESULT WINAPI enumverbs_QueryInterface(IEnumOLEVERB *iface, REFIID riid, void **ppv)
{
    if (IsEqualIID(riid, &IID_IEnumOLEVERB) || IsEqualIID(riid, &IID_IUnknown)) {
        *ppv = iface;
        IEnumOLEVERB_AddRef(iface);
        return S_OK;
    }

    *ppv = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI enumverbs_AddRef(IEnumOLEVERB *iface)
{
    return 2;
}

static ULONG WINAPI enumverbs_Release(IEnumOLEVERB *iface)
{
    return 1;
}

static int g_enumpos;
static HRESULT WINAPI enumverbs_Next(IEnumOLEVERB *iface, ULONG count, OLEVERB *verbs, ULONG *fetched)
{
    ok(count == 1, "got %lu\n", count);
    ok(fetched == NULL, "got %p\n", fetched);
    ok(g_enumpos == 0 || g_enumpos == 1, "got pos %d\n", g_enumpos);

    if (g_enumpos++ == 0) {
        verbs->lVerb = 123;
        verbs->lpszVerbName = CoTaskMemAlloc(sizeof(L"verb"));
        lstrcpyW(verbs->lpszVerbName, L"verb");
        verbs->fuFlags = MF_ENABLED;
        verbs->grfAttribs = OLEVERBATTRIB_ONCONTAINERMENU;
        if (fetched) *fetched = 1;
        return S_OK;
    }

    return S_FALSE;
}

static HRESULT WINAPI enumverbs_Skip(IEnumOLEVERB *iface, ULONG count)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI enumverbs_Reset(IEnumOLEVERB *iface)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI enumverbs_Clone(IEnumOLEVERB *iface, IEnumOLEVERB **ppenum)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static IEnumOLEVERBVtbl enumverbsvtbl = {
    enumverbs_QueryInterface,
    enumverbs_AddRef,
    enumverbs_Release,
    enumverbs_Next,
    enumverbs_Skip,
    enumverbs_Reset,
    enumverbs_Clone
};

static IEnumOLEVERB enumverbs = { &enumverbsvtbl };

static HRESULT WINAPI oleobject_QueryInterface(IOleObject *iface, REFIID riid, void **ppv)
{
    if (IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_IOleObject)) {
        *ppv = iface;
        IOleObject_AddRef(iface);
        return S_OK;
    }

    *ppv = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI oleobject_AddRef(IOleObject *iface)
{
    return 2;
}

static ULONG WINAPI oleobject_Release(IOleObject *iface)
{
    return 1;
}

static HRESULT WINAPI oleobject_SetClientSite(IOleObject *iface, IOleClientSite *site)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI oleobject_GetClientSite(IOleObject *iface, IOleClientSite **site)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI oleobject_SetHostNames(IOleObject *iface, LPCOLESTR containerapp,
    LPCOLESTR containerObj)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI oleobject_Close(IOleObject *iface, DWORD saveopt)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI oleobject_SetMoniker(IOleObject *iface, DWORD whichmoniker, IMoniker *mk)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI oleobject_GetMoniker(IOleObject *iface, DWORD assign, DWORD whichmoniker,
    IMoniker **mk)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI oleobject_InitFromData(IOleObject *iface, IDataObject *dataobject,
    BOOL creation, DWORD reserved)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI oleobject_GetClipboardData(IOleObject *iface, DWORD reserved, IDataObject **dataobject)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI oleobject_DoVerb(IOleObject *iface, LONG verb, MSG *msg, IOleClientSite *activesite,
    LONG index, HWND hwndParent, LPCRECT rect)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static BOOL g_enumverbsfail;
static HRESULT WINAPI oleobject_EnumVerbs(IOleObject *iface, IEnumOLEVERB **enumverb)
{
    if (g_enumverbsfail) {
        *enumverb = NULL;
        return E_FAIL;
    }
    *enumverb = &enumverbs;
    return S_OK;
}

static HRESULT WINAPI oleobject_Update(IOleObject *iface)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI oleobject_IsUpToDate(IOleObject *iface)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI oleobject_GetUserClassID(IOleObject *iface, CLSID *clsid)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}
static HRESULT WINAPI oleobject_GetUserType(IOleObject *iface, DWORD formoftype,
    LPOLESTR *usertype)
{
    ok(formoftype == USERCLASSTYPE_SHORT, "got %ld\n", formoftype);
    *usertype = CoTaskMemAlloc(sizeof(L"test"));
    lstrcpyW(*usertype, L"test");
    return S_OK;
}

static HRESULT WINAPI oleobject_SetExtent(IOleObject *iface, DWORD aspect, SIZEL *size)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI oleobject_GetExtent(IOleObject *iface, DWORD aspect, SIZEL *size)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI oleobject_Advise(IOleObject *iface, IAdviseSink *sink, DWORD *connection)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI oleobject_Unadvise(IOleObject *iface, DWORD connection)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI oleobject_EnumAdvise(IOleObject *iface, IEnumSTATDATA **enumadvise)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI oleobject_GetMiscStatus(IOleObject *iface, DWORD aspect, DWORD *status)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI oleobject_SetColorScheme(IOleObject *iface, LOGPALETTE *pal)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static IOleObjectVtbl oleobjectvtbl = {
    oleobject_QueryInterface,
    oleobject_AddRef,
    oleobject_Release,
    oleobject_SetClientSite,
    oleobject_GetClientSite,
    oleobject_SetHostNames,
    oleobject_Close,
    oleobject_SetMoniker,
    oleobject_GetMoniker,
    oleobject_InitFromData,
    oleobject_GetClipboardData,
    oleobject_DoVerb,
    oleobject_EnumVerbs,
    oleobject_Update,
    oleobject_IsUpToDate,
    oleobject_GetUserClassID,
    oleobject_GetUserType,
    oleobject_SetExtent,
    oleobject_GetExtent,
    oleobject_Advise,
    oleobject_Unadvise,
    oleobject_EnumAdvise,
    oleobject_GetMiscStatus,
    oleobject_SetColorScheme
};

static IOleObject oleobject = { &oleobjectvtbl };

static void test_OleUIAddVerbMenu(void)
{
    HMENU hMenu, verbmenu;
    MENUITEMINFOW info;
    WCHAR buffW[50];
    int count;
    BOOL ret;

    ret = OleUIAddVerbMenuW(NULL, NULL, NULL, 0, 0, 0, FALSE, 0, NULL);
    ok(!ret, "got %d\n", ret);

    verbmenu = (HMENU)0xdeadbeef;
    ret = OleUIAddVerbMenuW(NULL, NULL, NULL, 0, 0, 0, FALSE, 0, &verbmenu);
    ok(!ret, "got %d\n", ret);
    ok(verbmenu == NULL, "got %p\n", verbmenu);

    g_enumpos = 0;
    ret = OleUIAddVerbMenuW(&oleobject, NULL, NULL, 0, 0, 0, FALSE, 0, NULL);
    ok(!ret, "got %d\n", ret);

    hMenu = CreatePopupMenu();

    memset(&info, 0, sizeof(info));
    info.cbSize = sizeof(info);
    ret = InsertMenuItemW(hMenu, 0, TRUE, &info);
    ok(ret, "got %d\n", ret);

    count = GetMenuItemCount(hMenu);
    ok(count == 1, "got %d\n", count);

    g_enumpos = 0;
    ret = OleUIAddVerbMenuW(&oleobject, NULL, hMenu, 0, 0, 0, FALSE, 0, NULL);
    ok(!ret, "got %d\n", ret);

    count = GetMenuItemCount(hMenu);
    ok(count == 1, "got %d\n", count);

    ret = InsertMenuItemW(hMenu, 0, TRUE, &info);
    ok(ret, "got %d\n", ret);

    count = GetMenuItemCount(hMenu);
    ok(count == 2, "got %d\n", count);

    verbmenu = (HMENU)0xdeadbeef;
    g_enumpos = 0;
    ret = OleUIAddVerbMenuW(&oleobject, NULL, hMenu, 1, 0, 0, FALSE, 0, &verbmenu);
    ok(ret, "got %d\n", ret);
    ok(verbmenu == NULL, "got %p\n", verbmenu);

    count = GetMenuItemCount(hMenu);
    ok(count == 2, "got %d\n", count);

    /* object doesn't support EnumVerbs() */
    g_enumverbsfail = TRUE;
    g_enumpos = 0;
    verbmenu = (HMENU)0xdeadbeef;
    ret = OleUIAddVerbMenuW(&oleobject, NULL, hMenu, 2, 0, 0, FALSE, 0, &verbmenu);
    ok(!ret, "got %d\n", ret);
    ok(verbmenu == NULL, "got %p\n", verbmenu);
    g_enumverbsfail = FALSE;

    /* added disabled item */
    memset(&info, 0, sizeof(info));
    info.cbSize = sizeof(info);
    info.fMask = MIIM_STATE|MIIM_SUBMENU;
    ret = GetMenuItemInfoW(hMenu, 2, TRUE, &info);
    ok(ret, "got %d\n", ret);
    ok(info.fState & MFS_DISABLED, "got state 0x%08x\n", info.fState);
    ok(info.hSubMenu == NULL, "got submenu %p\n", info.hSubMenu);

    count = GetMenuItemCount(hMenu);
    ok(count == 3, "got %d\n", count);

    /* now without object */
    verbmenu = (HMENU)0xdeadbeef;
    ret = OleUIAddVerbMenuW(NULL, L"test", hMenu, 3, 42, 0, FALSE, 0, &verbmenu);
    ok(!ret, "got %d\n", ret);
    ok(verbmenu == NULL, "got %p\n", verbmenu);

    memset(&info, 0, sizeof(info));
    info.cbSize = sizeof(info);
    info.fMask = MIIM_STATE|MIIM_ID|MIIM_STRING|MIIM_SUBMENU;
    info.dwTypeData = buffW;
    info.cch = ARRAY_SIZE(buffW);
    ret = GetMenuItemInfoW(hMenu, 3, TRUE, &info);
    ok(ret, "got %d\n", ret);
    ok(info.fState == MF_GRAYED, "got state 0x%08x\n", info.fState);
    ok(info.wID == 42, "got id %d\n", info.wID);
    ok(info.hSubMenu == NULL, "got submenu %p\n", info.hSubMenu);

    count = GetMenuItemCount(hMenu);
    ok(count == 4, "got %d\n", count);

    verbmenu = (HMENU)0xdeadbeef;
    g_enumpos = 0;
    ret = OleUIAddVerbMenuW(&oleobject, NULL, hMenu, 4, 0, 0, FALSE, 0, &verbmenu);
    ok(ret, "got %d\n", ret);
    ok(verbmenu == NULL, "got %p\n", verbmenu);

    /* check newly added item */
    memset(&info, 0, sizeof(info));
    info.cbSize = sizeof(info);
    info.fMask = MIIM_STRING|MIIM_STATE|MIIM_SUBMENU;
    info.dwTypeData = buffW;
    info.cch = ARRAY_SIZE(buffW);
    ret = GetMenuItemInfoW(hMenu, 4, TRUE, &info);
    ok(ret, "got %d\n", ret);
    /* Item string contains verb, usertype and localized string for 'Object' word,
       exact format depends on localization. */
    ok(wcsstr(buffW, L"verb") != NULL, "str %s\n", wine_dbgstr_w(buffW));
    ok(info.fState == 0, "got state 0x%08x\n", info.fState);
    ok(info.hSubMenu == NULL, "got submenu %p\n", info.hSubMenu);

    count = GetMenuItemCount(hMenu);
    ok(count == 5, "got %d\n", count);

    DestroyMenu(hMenu);

    /* try to add verb menu repeatedly, with same id */
    hMenu = CreatePopupMenu();

    count = GetMenuItemCount(hMenu);
    ok(count == 0, "got %d\n", count);

    verbmenu = NULL;
    ret = OleUIAddVerbMenuW(NULL, NULL, hMenu, 0, 5, 10, TRUE, 3, &verbmenu);
    ok(!ret, "got %d\n", ret);
    ok(verbmenu == NULL, "got %p\n", verbmenu);

    count = GetMenuItemCount(hMenu);
    ok(count == 1, "got %d\n", count);

    verbmenu = NULL;
    ret = OleUIAddVerbMenuW(NULL, NULL, hMenu, 0, 5, 10, TRUE, 3, &verbmenu);
    ok(!ret, "got %d\n", ret);
    ok(verbmenu == NULL, "got %p\n", verbmenu);

    count = GetMenuItemCount(hMenu);
    ok(count == 1, "got %d\n", count);

    /* same position, different id */
    verbmenu = NULL;
    ret = OleUIAddVerbMenuW(NULL, NULL, hMenu, 0, 6, 10, TRUE, 3, &verbmenu);
    ok(!ret, "got %d\n", ret);
    ok(verbmenu == NULL, "got %p\n", verbmenu);

    count = GetMenuItemCount(hMenu);
    ok(count == 1, "got %d\n", count);

    /* change added item string and state */
    memset(&info, 0, sizeof(info));
    info.cbSize = sizeof(info);
    info.fMask = MIIM_STRING|MIIM_STATE;
    info.fState = MFS_ENABLED;
    info.dwTypeData = buffW;
    lstrcpyW(buffW, L"cadabra");
    ret = SetMenuItemInfoW(hMenu, 0, TRUE, &info);
    ok(ret, "got %d\n", ret);

    buffW[0] = 0;
    GetMenuStringW(hMenu, 0, buffW, ARRAY_SIZE(buffW), MF_BYPOSITION);
    ok(!lstrcmpW(buffW, L"cadabra"), "got %s\n", wine_dbgstr_w(buffW));

    verbmenu = NULL;
    ret = OleUIAddVerbMenuW(NULL, NULL, hMenu, 0, 5, 10, TRUE, 3, &verbmenu);
    ok(!ret, "got %d\n", ret);
    ok(verbmenu == NULL, "got %p\n", verbmenu);

    memset(&info, 0, sizeof(info));
    info.cbSize = sizeof(info);
    info.fMask = MIIM_STRING|MIIM_STATE;
    buffW[0] = 0;
    info.dwTypeData = buffW;
    info.cch = ARRAY_SIZE(buffW);
    ret = GetMenuItemInfoW(hMenu, 0, TRUE, &info);
    ok(ret, "got %d\n", ret);
    ok(lstrcmpW(buffW, L"cadabra"), "got %s\n", wine_dbgstr_w(buffW));
    ok(info.fState == MF_GRAYED, "got state 0x%08x\n", info.fState);

    count = GetMenuItemCount(hMenu);
    ok(count == 1, "got %d\n", count);

    DestroyMenu(hMenu);
}

START_TEST(main)
{
    test_OleUIAddVerbMenu();
}
