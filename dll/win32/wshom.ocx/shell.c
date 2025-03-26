/*
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

#ifdef __REACTOS__
#include <wchar.h>
#endif

#include "wshom_private.h"
#include "wshom.h"

#include "shellapi.h"
#include "shlobj.h"
#include "dispex.h"

#include "wine/debug.h"
#include "wine/heap.h"

WINE_DEFAULT_DEBUG_CHANNEL(wshom);

typedef struct
{
    struct provideclassinfo classinfo;
    IWshShell3 IWshShell3_iface;
} WshShellImpl;
static WshShellImpl WshShell3;

typedef struct
{
    struct provideclassinfo classinfo;
    IWshCollection IWshCollection_iface;
    LONG ref;
} WshCollection;

typedef struct
{
    struct provideclassinfo classinfo;
    IWshShortcut IWshShortcut_iface;
    LONG ref;

    IShellLinkW *link;
    BSTR path_link;
} WshShortcut;

typedef struct
{
    struct provideclassinfo classinfo;
    IWshEnvironment IWshEnvironment_iface;
    LONG ref;
} WshEnvironment;

typedef struct
{
    struct provideclassinfo classinfo;
    IWshExec IWshExec_iface;
    LONG ref;
    PROCESS_INFORMATION info;
} WshExecImpl;

static inline WshCollection *impl_from_IWshCollection( IWshCollection *iface )
{
    return CONTAINING_RECORD(iface, WshCollection, IWshCollection_iface);
}

static inline WshShortcut *impl_from_IWshShortcut( IWshShortcut *iface )
{
    return CONTAINING_RECORD(iface, WshShortcut, IWshShortcut_iface);
}

static inline WshEnvironment *impl_from_IWshEnvironment( IWshEnvironment *iface )
{
    return CONTAINING_RECORD(iface, WshEnvironment, IWshEnvironment_iface);
}

static inline WshExecImpl *impl_from_IWshExec( IWshExec *iface )
{
    return CONTAINING_RECORD(iface, WshExecImpl, IWshExec_iface);
}

static HRESULT WINAPI WshExec_QueryInterface(IWshExec *iface, REFIID riid, void **obj)
{
    WshExecImpl *This = impl_from_IWshExec(iface);

    TRACE("(%p)->(%s, %p)\n", This, debugstr_guid(riid), obj);

    if (IsEqualGUID(riid, &IID_IDispatch) ||
        IsEqualGUID(riid, &IID_IWshExec) ||
        IsEqualGUID(riid, &IID_IUnknown))
    {
        *obj = iface;
    }
    else if (IsEqualIID(riid, &IID_IProvideClassInfo))
    {
        *obj = &This->classinfo.IProvideClassInfo_iface;
    }
    else {
        FIXME("Unknown iface %s\n", debugstr_guid(riid));
        *obj = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown *)*obj);
    return S_OK;
}

static ULONG WINAPI WshExec_AddRef(IWshExec *iface)
{
    WshExecImpl *This = impl_from_IWshExec(iface);
    LONG ref = InterlockedIncrement(&This->ref);
    TRACE("(%p) ref = %d\n", This, ref);
    return ref;
}

static ULONG WINAPI WshExec_Release(IWshExec *iface)
{
    WshExecImpl *This = impl_from_IWshExec(iface);
    LONG ref = InterlockedDecrement(&This->ref);
    TRACE("(%p) ref = %d\n", This, ref);

    if (!ref) {
        CloseHandle(This->info.hThread);
        CloseHandle(This->info.hProcess);
        heap_free(This);
    }

    return ref;
}

static HRESULT WINAPI WshExec_GetTypeInfoCount(IWshExec *iface, UINT *pctinfo)
{
    WshExecImpl *This = impl_from_IWshExec(iface);
    TRACE("(%p)->(%p)\n", This, pctinfo);
    *pctinfo = 1;
    return S_OK;
}

static HRESULT WINAPI WshExec_GetTypeInfo(IWshExec *iface, UINT iTInfo, LCID lcid, ITypeInfo **ppTInfo)
{
    WshExecImpl *This = impl_from_IWshExec(iface);
    TRACE("(%p)->(%u %u %p)\n", This, iTInfo, lcid, ppTInfo);
    return get_typeinfo(IWshExec_tid, ppTInfo);
}

static HRESULT WINAPI WshExec_GetIDsOfNames(IWshExec *iface, REFIID riid, LPOLESTR *rgszNames,
        UINT cNames, LCID lcid, DISPID *rgDispId)
{
    WshExecImpl *This = impl_from_IWshExec(iface);
    ITypeInfo *typeinfo;
    HRESULT hr;

    TRACE("(%p)->(%s %p %u %u %p)\n", This, debugstr_guid(riid), rgszNames, cNames, lcid, rgDispId);

    hr = get_typeinfo(IWshExec_tid, &typeinfo);
    if(SUCCEEDED(hr))
    {
        hr = ITypeInfo_GetIDsOfNames(typeinfo, rgszNames, cNames, rgDispId);
        ITypeInfo_Release(typeinfo);
    }

    return hr;
}

static HRESULT WINAPI WshExec_Invoke(IWshExec *iface, DISPID dispIdMember, REFIID riid, LCID lcid,
        WORD wFlags, DISPPARAMS *pDispParams, VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    WshExecImpl *This = impl_from_IWshExec(iface);
    ITypeInfo *typeinfo;
    HRESULT hr;

    TRACE("(%p)->(%d %s %d %d %p %p %p %p)\n", This, dispIdMember, debugstr_guid(riid),
          lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);

    hr = get_typeinfo(IWshExec_tid, &typeinfo);
    if(SUCCEEDED(hr))
    {
        hr = ITypeInfo_Invoke(typeinfo, &This->IWshExec_iface, dispIdMember, wFlags,
                pDispParams, pVarResult, pExcepInfo, puArgErr);
        ITypeInfo_Release(typeinfo);
    }

    return hr;
}

static HRESULT WINAPI WshExec_get_Status(IWshExec *iface, WshExecStatus *status)
{
    WshExecImpl *This = impl_from_IWshExec(iface);
    DWORD code;

    TRACE("(%p)->(%p)\n", This, status);

    if (!status)
        return E_INVALIDARG;

    if (!GetExitCodeProcess(This->info.hProcess, &code))
        return HRESULT_FROM_WIN32(GetLastError());

    switch (code)
    {
    case 0:
        *status = WshFinished;
        break;
    case STILL_ACTIVE:
        *status = WshRunning;
        break;
    default:
        *status = WshFailed;
    }

    return S_OK;
}

static HRESULT WINAPI WshExec_get_StdIn(IWshExec *iface, ITextStream **stream)
{
    WshExecImpl *This = impl_from_IWshExec(iface);

    FIXME("(%p)->(%p): stub\n", This, stream);

    return E_NOTIMPL;
}

static HRESULT WINAPI WshExec_get_StdOut(IWshExec *iface, ITextStream **stream)
{
    WshExecImpl *This = impl_from_IWshExec(iface);

    FIXME("(%p)->(%p): stub\n", This, stream);

    return E_NOTIMPL;
}

static HRESULT WINAPI WshExec_get_StdErr(IWshExec *iface, ITextStream **stream)
{
    WshExecImpl *This = impl_from_IWshExec(iface);

    FIXME("(%p)->(%p): stub\n", This, stream);

    return E_NOTIMPL;
}

static HRESULT WINAPI WshExec_get_ProcessID(IWshExec *iface, DWORD *pid)
{
    WshExecImpl *This = impl_from_IWshExec(iface);

    TRACE("(%p)->(%p)\n", This, pid);

    if (!pid)
        return E_INVALIDARG;

    *pid = This->info.dwProcessId;
    return S_OK;
}

static HRESULT WINAPI WshExec_get_ExitCode(IWshExec *iface, DWORD *code)
{
    WshExecImpl *This = impl_from_IWshExec(iface);

    FIXME("(%p)->(%p): stub\n", This, code);

    return E_NOTIMPL;
}

static BOOL CALLBACK enum_thread_wnd_proc(HWND hwnd, LPARAM lParam)
{
    INT *count = (INT*)lParam;

    (*count)++;
    PostMessageW(hwnd, WM_CLOSE, 0, 0);
    /* try to send it to all windows, even if failed for some */
    return TRUE;
}

static HRESULT WINAPI WshExec_Terminate(IWshExec *iface)
{
    WshExecImpl *This = impl_from_IWshExec(iface);
    BOOL ret, kill = FALSE;
    INT count = 0;

    TRACE("(%p)\n", This);

    ret = EnumThreadWindows(This->info.dwThreadId, enum_thread_wnd_proc, (LPARAM)&count);
    if (ret && count) {
        /* manual testing shows that it waits 2 seconds before forcing termination */
        if (WaitForSingleObject(This->info.hProcess, 2000) != WAIT_OBJECT_0)
            kill = TRUE;
    }
    else
        kill = TRUE;

    if (kill)
        TerminateProcess(This->info.hProcess, 0);

    return S_OK;
}

static const IWshExecVtbl WshExecVtbl = {
    WshExec_QueryInterface,
    WshExec_AddRef,
    WshExec_Release,
    WshExec_GetTypeInfoCount,
    WshExec_GetTypeInfo,
    WshExec_GetIDsOfNames,
    WshExec_Invoke,
    WshExec_get_Status,
    WshExec_get_StdIn,
    WshExec_get_StdOut,
    WshExec_get_StdErr,
    WshExec_get_ProcessID,
    WshExec_get_ExitCode,
    WshExec_Terminate
};

static HRESULT WshExec_create(BSTR command, IWshExec **ret)
{
    STARTUPINFOW si = {0};
    WshExecImpl *This;

    *ret = NULL;

    This = heap_alloc(sizeof(*This));
    if (!This)
        return E_OUTOFMEMORY;

    This->IWshExec_iface.lpVtbl = &WshExecVtbl;
    This->ref = 1;

    if (!CreateProcessW(NULL, command, NULL, NULL, FALSE, 0, NULL, NULL, &si, &This->info)) {
        heap_free(This);
        return HRESULT_FROM_WIN32(GetLastError());
    }

    init_classinfo(&CLSID_WshExec, (IUnknown *)&This->IWshExec_iface, &This->classinfo);
    *ret = &This->IWshExec_iface;
    return S_OK;
}

static HRESULT WINAPI WshEnvironment_QueryInterface(IWshEnvironment *iface, REFIID riid, void **obj)
{
    WshEnvironment *This = impl_from_IWshEnvironment(iface);

    TRACE("(%p)->(%s, %p)\n", This, debugstr_guid(riid), obj);

    if (IsEqualGUID(riid, &IID_IUnknown)  ||
        IsEqualGUID(riid, &IID_IDispatch) ||
        IsEqualGUID(riid, &IID_IWshEnvironment))
    {
        *obj = iface;
    }
    else if (IsEqualIID(riid, &IID_IProvideClassInfo))
    {
        *obj = &This->classinfo.IProvideClassInfo_iface;
    }
    else {
        FIXME("Unknown iface %s\n", debugstr_guid(riid));
        *obj = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*obj);
    return S_OK;
}

static ULONG WINAPI WshEnvironment_AddRef(IWshEnvironment *iface)
{
    WshEnvironment *This = impl_from_IWshEnvironment(iface);
    LONG ref = InterlockedIncrement(&This->ref);
    TRACE("(%p) ref = %d\n", This, ref);
    return ref;
}

static ULONG WINAPI WshEnvironment_Release(IWshEnvironment *iface)
{
    WshEnvironment *This = impl_from_IWshEnvironment(iface);
    LONG ref = InterlockedDecrement(&This->ref);
    TRACE("(%p) ref = %d\n", This, ref);

    if (!ref)
        heap_free(This);

    return ref;
}

static HRESULT WINAPI WshEnvironment_GetTypeInfoCount(IWshEnvironment *iface, UINT *pctinfo)
{
    WshEnvironment *This = impl_from_IWshEnvironment(iface);
    TRACE("(%p)->(%p)\n", This, pctinfo);
    *pctinfo = 1;
    return S_OK;
}

static HRESULT WINAPI WshEnvironment_GetTypeInfo(IWshEnvironment *iface, UINT iTInfo, LCID lcid, ITypeInfo **ppTInfo)
{
    WshEnvironment *This = impl_from_IWshEnvironment(iface);
    TRACE("(%p)->(%u %u %p)\n", This, iTInfo, lcid, ppTInfo);
    return get_typeinfo(IWshEnvironment_tid, ppTInfo);
}

static HRESULT WINAPI WshEnvironment_GetIDsOfNames(IWshEnvironment *iface, REFIID riid, LPOLESTR *rgszNames,
        UINT cNames, LCID lcid, DISPID *rgDispId)
{
    WshEnvironment *This = impl_from_IWshEnvironment(iface);
    ITypeInfo *typeinfo;
    HRESULT hr;

    TRACE("(%p)->(%s %p %u %u %p)\n", This, debugstr_guid(riid), rgszNames, cNames, lcid, rgDispId);

    hr = get_typeinfo(IWshEnvironment_tid, &typeinfo);
    if(SUCCEEDED(hr))
    {
        hr = ITypeInfo_GetIDsOfNames(typeinfo, rgszNames, cNames, rgDispId);
        ITypeInfo_Release(typeinfo);
    }

    return hr;
}

static HRESULT WINAPI WshEnvironment_Invoke(IWshEnvironment *iface, DISPID dispIdMember, REFIID riid, LCID lcid,
        WORD wFlags, DISPPARAMS *pDispParams, VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    WshEnvironment *This = impl_from_IWshEnvironment(iface);
    ITypeInfo *typeinfo;
    HRESULT hr;

    TRACE("(%p)->(%d %s %d %d %p %p %p %p)\n", This, dispIdMember, debugstr_guid(riid),
          lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);

    hr = get_typeinfo(IWshEnvironment_tid, &typeinfo);
    if(SUCCEEDED(hr))
    {
        hr = ITypeInfo_Invoke(typeinfo, &This->IWshEnvironment_iface, dispIdMember, wFlags,
                pDispParams, pVarResult, pExcepInfo, puArgErr);
        ITypeInfo_Release(typeinfo);
    }

    return hr;
}

static HRESULT WINAPI WshEnvironment_get_Item(IWshEnvironment *iface, BSTR name, BSTR *value)
{
    WshEnvironment *This = impl_from_IWshEnvironment(iface);
    DWORD len;

    TRACE("(%p)->(%s %p)\n", This, debugstr_w(name), value);

    if (!value)
        return E_POINTER;

    len = GetEnvironmentVariableW(name, NULL, 0);
    if (len)
    {
        *value = SysAllocStringLen(NULL, len - 1);
        if (*value)
            GetEnvironmentVariableW(name, *value, len);
    }
    else
        *value = SysAllocStringLen(NULL, 0);

    return *value ? S_OK : E_OUTOFMEMORY;
}

static HRESULT WINAPI WshEnvironment_put_Item(IWshEnvironment *iface, BSTR name, BSTR value)
{
    WshEnvironment *This = impl_from_IWshEnvironment(iface);
    FIXME("(%p)->(%s %s): stub\n", This, debugstr_w(name), debugstr_w(value));
    return E_NOTIMPL;
}

static HRESULT WINAPI WshEnvironment_Count(IWshEnvironment *iface, LONG *count)
{
    WshEnvironment *This = impl_from_IWshEnvironment(iface);
    FIXME("(%p)->(%p): stub\n", This, count);
    return E_NOTIMPL;
}

static HRESULT WINAPI WshEnvironment_get_length(IWshEnvironment *iface, LONG *len)
{
    WshEnvironment *This = impl_from_IWshEnvironment(iface);
    FIXME("(%p)->(%p): stub\n", This, len);
    return E_NOTIMPL;
}

static HRESULT WINAPI WshEnvironment__NewEnum(IWshEnvironment *iface, IUnknown **penum)
{
    WshEnvironment *This = impl_from_IWshEnvironment(iface);
    FIXME("(%p)->(%p): stub\n", This, penum);
    return E_NOTIMPL;
}

static HRESULT WINAPI WshEnvironment_Remove(IWshEnvironment *iface, BSTR name)
{
    WshEnvironment *This = impl_from_IWshEnvironment(iface);
    FIXME("(%p)->(%s): stub\n", This, debugstr_w(name));
    return E_NOTIMPL;
}

static const IWshEnvironmentVtbl WshEnvironmentVtbl = {
    WshEnvironment_QueryInterface,
    WshEnvironment_AddRef,
    WshEnvironment_Release,
    WshEnvironment_GetTypeInfoCount,
    WshEnvironment_GetTypeInfo,
    WshEnvironment_GetIDsOfNames,
    WshEnvironment_Invoke,
    WshEnvironment_get_Item,
    WshEnvironment_put_Item,
    WshEnvironment_Count,
    WshEnvironment_get_length,
    WshEnvironment__NewEnum,
    WshEnvironment_Remove
};

static HRESULT WshEnvironment_Create(IWshEnvironment **env)
{
    WshEnvironment *This;

    This = heap_alloc(sizeof(*This));
    if (!This) return E_OUTOFMEMORY;

    This->IWshEnvironment_iface.lpVtbl = &WshEnvironmentVtbl;
    This->ref = 1;

    init_classinfo(&IID_IWshEnvironment, (IUnknown *)&This->IWshEnvironment_iface, &This->classinfo);
    *env = &This->IWshEnvironment_iface;

    return S_OK;
}

static HRESULT WINAPI WshCollection_QueryInterface(IWshCollection *iface, REFIID riid, void **ppv)
{
    WshCollection *This = impl_from_IWshCollection(iface);

    TRACE("(%p)->(%s, %p)\n", This, debugstr_guid(riid), ppv);

    if (IsEqualGUID(riid, &IID_IUnknown)  ||
        IsEqualGUID(riid, &IID_IDispatch) ||
        IsEqualGUID(riid, &IID_IWshCollection))
    {
        *ppv = iface;
    }
    else if (IsEqualIID(riid, &IID_IProvideClassInfo))
    {
        *ppv = &This->classinfo.IProvideClassInfo_iface;
    }
    else {
        FIXME("Unknown iface %s\n", debugstr_guid(riid));
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI WshCollection_AddRef(IWshCollection *iface)
{
    WshCollection *This = impl_from_IWshCollection(iface);
    LONG ref = InterlockedIncrement(&This->ref);
    TRACE("(%p) ref = %d\n", This, ref);
    return ref;
}

static ULONG WINAPI WshCollection_Release(IWshCollection *iface)
{
    WshCollection *This = impl_from_IWshCollection(iface);
    LONG ref = InterlockedDecrement(&This->ref);
    TRACE("(%p) ref = %d\n", This, ref);

    if (!ref)
        heap_free(This);

    return ref;
}

static HRESULT WINAPI WshCollection_GetTypeInfoCount(IWshCollection *iface, UINT *pctinfo)
{
    WshCollection *This = impl_from_IWshCollection(iface);
    TRACE("(%p)->(%p)\n", This, pctinfo);
    *pctinfo = 1;
    return S_OK;
}

static HRESULT WINAPI WshCollection_GetTypeInfo(IWshCollection *iface, UINT iTInfo, LCID lcid, ITypeInfo **ppTInfo)
{
    WshCollection *This = impl_from_IWshCollection(iface);
    TRACE("(%p)->(%u %u %p)\n", This, iTInfo, lcid, ppTInfo);
    return get_typeinfo(IWshCollection_tid, ppTInfo);
}

static HRESULT WINAPI WshCollection_GetIDsOfNames(IWshCollection *iface, REFIID riid, LPOLESTR *rgszNames,
        UINT cNames, LCID lcid, DISPID *rgDispId)
{
    WshCollection *This = impl_from_IWshCollection(iface);
    ITypeInfo *typeinfo;
    HRESULT hr;

    TRACE("(%p)->(%s %p %u %u %p)\n", This, debugstr_guid(riid), rgszNames, cNames, lcid, rgDispId);

    hr = get_typeinfo(IWshCollection_tid, &typeinfo);
    if(SUCCEEDED(hr))
    {
        hr = ITypeInfo_GetIDsOfNames(typeinfo, rgszNames, cNames, rgDispId);
        ITypeInfo_Release(typeinfo);
    }

    return hr;
}

static HRESULT WINAPI WshCollection_Invoke(IWshCollection *iface, DISPID dispIdMember, REFIID riid, LCID lcid,
        WORD wFlags, DISPPARAMS *pDispParams, VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    WshCollection *This = impl_from_IWshCollection(iface);
    ITypeInfo *typeinfo;
    HRESULT hr;

    TRACE("(%p)->(%d %s %d %d %p %p %p %p)\n", This, dispIdMember, debugstr_guid(riid),
          lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);

    hr = get_typeinfo(IWshCollection_tid, &typeinfo);
    if(SUCCEEDED(hr))
    {
        hr = ITypeInfo_Invoke(typeinfo, &This->IWshCollection_iface, dispIdMember, wFlags,
                pDispParams, pVarResult, pExcepInfo, puArgErr);
        ITypeInfo_Release(typeinfo);
    }

    return hr;
}

static HRESULT WINAPI WshCollection_Item(IWshCollection *iface, VARIANT *index, VARIANT *value)
{
    WshCollection *This = impl_from_IWshCollection(iface);
    static const WCHAR allusersdesktopW[] = {'A','l','l','U','s','e','r','s','D','e','s','k','t','o','p',0};
    static const WCHAR allusersprogramsW[] = {'A','l','l','U','s','e','r','s','P','r','o','g','r','a','m','s',0};
    static const WCHAR desktopW[] = {'D','e','s','k','t','o','p',0};
    PIDLIST_ABSOLUTE pidl;
    WCHAR pathW[MAX_PATH];
    int kind = 0;
    BSTR folder;
    HRESULT hr;

    TRACE("(%p)->(%s %p)\n", This, debugstr_variant(index), value);

    if (V_VT(index) != VT_BSTR)
    {
        FIXME("only BSTR index supported, got %d\n", V_VT(index));
        return E_NOTIMPL;
    }

    folder = V_BSTR(index);
    if (!wcsicmp(folder, desktopW))
        kind = CSIDL_DESKTOP;
    else if (!wcsicmp(folder, allusersdesktopW))
        kind = CSIDL_COMMON_DESKTOPDIRECTORY;
    else if (!wcsicmp(folder, allusersprogramsW))
        kind = CSIDL_COMMON_PROGRAMS;
    else
    {
        FIXME("folder kind %s not supported\n", debugstr_w(folder));
        return E_NOTIMPL;
    }

    hr = SHGetSpecialFolderLocation(NULL, kind, &pidl);
    if (hr != S_OK) return hr;

    if (SHGetPathFromIDListW(pidl, pathW))
    {
        V_VT(value) = VT_BSTR;
        V_BSTR(value) = SysAllocString(pathW);
        hr = V_BSTR(value) ? S_OK : E_OUTOFMEMORY;
    }
    else
        hr = E_FAIL;

    CoTaskMemFree(pidl);

    return hr;
}

static HRESULT WINAPI WshCollection_Count(IWshCollection *iface, LONG *count)
{
    WshCollection *This = impl_from_IWshCollection(iface);
    FIXME("(%p)->(%p): stub\n", This, count);
    return E_NOTIMPL;
}

static HRESULT WINAPI WshCollection_get_length(IWshCollection *iface, LONG *count)
{
    WshCollection *This = impl_from_IWshCollection(iface);
    FIXME("(%p)->(%p): stub\n", This, count);
    return E_NOTIMPL;
}

static HRESULT WINAPI WshCollection__NewEnum(IWshCollection *iface, IUnknown **Enum)
{
    WshCollection *This = impl_from_IWshCollection(iface);
    FIXME("(%p)->(%p): stub\n", This, Enum);
    return E_NOTIMPL;
}

static const IWshCollectionVtbl WshCollectionVtbl = {
    WshCollection_QueryInterface,
    WshCollection_AddRef,
    WshCollection_Release,
    WshCollection_GetTypeInfoCount,
    WshCollection_GetTypeInfo,
    WshCollection_GetIDsOfNames,
    WshCollection_Invoke,
    WshCollection_Item,
    WshCollection_Count,
    WshCollection_get_length,
    WshCollection__NewEnum
};

static HRESULT WshCollection_Create(IWshCollection **collection)
{
    WshCollection *This;

    This = heap_alloc(sizeof(*This));
    if (!This) return E_OUTOFMEMORY;

    This->IWshCollection_iface.lpVtbl = &WshCollectionVtbl;
    This->ref = 1;

    init_classinfo(&IID_IWshCollection, (IUnknown *)&This->IWshCollection_iface, &This->classinfo);
    *collection = &This->IWshCollection_iface;

    return S_OK;
}

/* IWshShortcut */
static HRESULT WINAPI WshShortcut_QueryInterface(IWshShortcut *iface, REFIID riid, void **ppv)
{
    WshShortcut *This = impl_from_IWshShortcut(iface);

    TRACE("(%p)->(%s, %p)\n", This, debugstr_guid(riid), ppv);

    if (IsEqualGUID(riid, &IID_IUnknown)  ||
        IsEqualGUID(riid, &IID_IDispatch) ||
        IsEqualGUID(riid, &IID_IWshShortcut))
    {
        *ppv = iface;
    }
    else if (IsEqualIID(riid, &IID_IProvideClassInfo))
    {
        *ppv = &This->classinfo.IProvideClassInfo_iface;
    }
    else {
        FIXME("Unknown iface %s\n", debugstr_guid(riid));
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI WshShortcut_AddRef(IWshShortcut *iface)
{
    WshShortcut *This = impl_from_IWshShortcut(iface);
    LONG ref = InterlockedIncrement(&This->ref);
    TRACE("(%p) ref = %d\n", This, ref);
    return ref;
}

static ULONG WINAPI WshShortcut_Release(IWshShortcut *iface)
{
    WshShortcut *This = impl_from_IWshShortcut(iface);
    LONG ref = InterlockedDecrement(&This->ref);
    TRACE("(%p) ref = %d\n", This, ref);

    if (!ref)
    {
        SysFreeString(This->path_link);
        IShellLinkW_Release(This->link);
        heap_free(This);
    }

    return ref;
}

static HRESULT WINAPI WshShortcut_GetTypeInfoCount(IWshShortcut *iface, UINT *pctinfo)
{
    WshShortcut *This = impl_from_IWshShortcut(iface);
    TRACE("(%p)->(%p)\n", This, pctinfo);
    *pctinfo = 1;
    return S_OK;
}

static HRESULT WINAPI WshShortcut_GetTypeInfo(IWshShortcut *iface, UINT iTInfo, LCID lcid, ITypeInfo **ppTInfo)
{
    WshShortcut *This = impl_from_IWshShortcut(iface);
    TRACE("(%p)->(%u %u %p)\n", This, iTInfo, lcid, ppTInfo);
    return get_typeinfo(IWshShortcut_tid, ppTInfo);
}

static HRESULT WINAPI WshShortcut_GetIDsOfNames(IWshShortcut *iface, REFIID riid, LPOLESTR *rgszNames,
        UINT cNames, LCID lcid, DISPID *rgDispId)
{
    WshShortcut *This = impl_from_IWshShortcut(iface);
    ITypeInfo *typeinfo;
    HRESULT hr;

    TRACE("(%p)->(%s %p %u %u %p)\n", This, debugstr_guid(riid), rgszNames, cNames, lcid, rgDispId);

    hr = get_typeinfo(IWshShortcut_tid, &typeinfo);
    if(SUCCEEDED(hr))
    {
        hr = ITypeInfo_GetIDsOfNames(typeinfo, rgszNames, cNames, rgDispId);
        ITypeInfo_Release(typeinfo);
    }

    return hr;
}

static HRESULT WINAPI WshShortcut_Invoke(IWshShortcut *iface, DISPID dispIdMember, REFIID riid, LCID lcid,
        WORD wFlags, DISPPARAMS *pDispParams, VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    WshShortcut *This = impl_from_IWshShortcut(iface);
    ITypeInfo *typeinfo;
    HRESULT hr;

    TRACE("(%p)->(%d %s %d %d %p %p %p %p)\n", This, dispIdMember, debugstr_guid(riid),
          lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);

    hr = get_typeinfo(IWshShortcut_tid, &typeinfo);
    if(SUCCEEDED(hr))
    {
        hr = ITypeInfo_Invoke(typeinfo, &This->IWshShortcut_iface, dispIdMember, wFlags,
                pDispParams, pVarResult, pExcepInfo, puArgErr);
        ITypeInfo_Release(typeinfo);
    }

    return hr;
}

static HRESULT WINAPI WshShortcut_get_FullName(IWshShortcut *iface, BSTR *name)
{
    WshShortcut *This = impl_from_IWshShortcut(iface);
    FIXME("(%p)->(%p): stub\n", This, name);
    return E_NOTIMPL;
}

static HRESULT WINAPI WshShortcut_get_Arguments(IWshShortcut *iface, BSTR *Arguments)
{
    WshShortcut *This = impl_from_IWshShortcut(iface);
    WCHAR buffW[INFOTIPSIZE];
    HRESULT hr;

    TRACE("(%p)->(%p)\n", This, Arguments);

    if (!Arguments)
        return E_POINTER;

    *Arguments = NULL;

    hr = IShellLinkW_GetArguments(This->link, buffW, ARRAY_SIZE(buffW));
    if (FAILED(hr))
        return hr;

    *Arguments = SysAllocString(buffW);
    return *Arguments ? S_OK : E_OUTOFMEMORY;
}

static HRESULT WINAPI WshShortcut_put_Arguments(IWshShortcut *iface, BSTR Arguments)
{
    WshShortcut *This = impl_from_IWshShortcut(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_w(Arguments));

    return IShellLinkW_SetArguments(This->link, Arguments);
}

static HRESULT WINAPI WshShortcut_get_Description(IWshShortcut *iface, BSTR *Description)
{
    WshShortcut *This = impl_from_IWshShortcut(iface);
    FIXME("(%p)->(%p): stub\n", This, Description);
    return E_NOTIMPL;
}

static HRESULT WINAPI WshShortcut_put_Description(IWshShortcut *iface, BSTR Description)
{
    WshShortcut *This = impl_from_IWshShortcut(iface);
    TRACE("(%p)->(%s)\n", This, debugstr_w(Description));
    return IShellLinkW_SetDescription(This->link, Description);
}

static HRESULT WINAPI WshShortcut_get_Hotkey(IWshShortcut *iface, BSTR *Hotkey)
{
    WshShortcut *This = impl_from_IWshShortcut(iface);
    FIXME("(%p)->(%p): stub\n", This, Hotkey);
    return E_NOTIMPL;
}

static HRESULT WINAPI WshShortcut_put_Hotkey(IWshShortcut *iface, BSTR Hotkey)
{
    WshShortcut *This = impl_from_IWshShortcut(iface);
    FIXME("(%p)->(%s): stub\n", This, debugstr_w(Hotkey));
    return E_NOTIMPL;
}

static HRESULT WINAPI WshShortcut_get_IconLocation(IWshShortcut *iface, BSTR *IconPath)
{
    static const WCHAR fmtW[] = {'%','s',',',' ','%','d',0};
    WshShortcut *This = impl_from_IWshShortcut(iface);
    WCHAR buffW[MAX_PATH], pathW[MAX_PATH];
    INT icon = 0;
    HRESULT hr;

    TRACE("(%p)->(%p)\n", This, IconPath);

    if (!IconPath)
        return E_POINTER;

    hr = IShellLinkW_GetIconLocation(This->link, buffW, ARRAY_SIZE(buffW), &icon);
    if (FAILED(hr)) return hr;

    swprintf(pathW, fmtW, buffW, icon);
    *IconPath = SysAllocString(pathW);
    if (!*IconPath) return E_OUTOFMEMORY;

    return S_OK;
}

static HRESULT WINAPI WshShortcut_put_IconLocation(IWshShortcut *iface, BSTR IconPath)
{
    WshShortcut *This = impl_from_IWshShortcut(iface);
    HRESULT hr;
    WCHAR *ptr;
    BSTR path;
    INT icon;

    TRACE("(%p)->(%s)\n", This, debugstr_w(IconPath));

    /* scan for icon id */
    ptr = wcsrchr(IconPath, ',');
    if (!ptr)
    {
        WARN("icon index not found\n");
        return E_FAIL;
    }

    path = SysAllocStringLen(IconPath, ptr-IconPath);

    /* skip spaces if any */
    while (iswspace(*++ptr))
        ;

    icon = wcstol(ptr, NULL, 10);

    hr = IShellLinkW_SetIconLocation(This->link, path, icon);
    SysFreeString(path);

    return hr;
}

static HRESULT WINAPI WshShortcut_put_RelativePath(IWshShortcut *iface, BSTR rhs)
{
    WshShortcut *This = impl_from_IWshShortcut(iface);
    FIXME("(%p)->(%s): stub\n", This, debugstr_w(rhs));
    return E_NOTIMPL;
}

static HRESULT WINAPI WshShortcut_get_TargetPath(IWshShortcut *iface, BSTR *Path)
{
    WshShortcut *This = impl_from_IWshShortcut(iface);
    FIXME("(%p)->(%p): stub\n", This, Path);
    return E_NOTIMPL;
}

static HRESULT WINAPI WshShortcut_put_TargetPath(IWshShortcut *iface, BSTR Path)
{
    WshShortcut *This = impl_from_IWshShortcut(iface);
    TRACE("(%p)->(%s)\n", This, debugstr_w(Path));
    return IShellLinkW_SetPath(This->link, Path);
}

static HRESULT WINAPI WshShortcut_get_WindowStyle(IWshShortcut *iface, int *ShowCmd)
{
    WshShortcut *This = impl_from_IWshShortcut(iface);
    TRACE("(%p)->(%p)\n", This, ShowCmd);
    return IShellLinkW_GetShowCmd(This->link, ShowCmd);
}

static HRESULT WINAPI WshShortcut_put_WindowStyle(IWshShortcut *iface, int ShowCmd)
{
    WshShortcut *This = impl_from_IWshShortcut(iface);
    TRACE("(%p)->(%d)\n", This, ShowCmd);
    return IShellLinkW_SetShowCmd(This->link, ShowCmd);
}

static HRESULT WINAPI WshShortcut_get_WorkingDirectory(IWshShortcut *iface, BSTR *WorkingDirectory)
{
    WshShortcut *This = impl_from_IWshShortcut(iface);
    WCHAR buffW[MAX_PATH];
    HRESULT hr;

    TRACE("(%p)->(%p)\n", This, WorkingDirectory);

    if (!WorkingDirectory)
        return E_POINTER;

    *WorkingDirectory = NULL;
    hr = IShellLinkW_GetWorkingDirectory(This->link, buffW, ARRAY_SIZE(buffW));
    if (FAILED(hr)) return hr;

    *WorkingDirectory = SysAllocString(buffW);
    return *WorkingDirectory ? S_OK : E_OUTOFMEMORY;
}

static HRESULT WINAPI WshShortcut_put_WorkingDirectory(IWshShortcut *iface, BSTR WorkingDirectory)
{
    WshShortcut *This = impl_from_IWshShortcut(iface);
    TRACE("(%p)->(%s)\n", This, debugstr_w(WorkingDirectory));
    return IShellLinkW_SetWorkingDirectory(This->link, WorkingDirectory);
}

static HRESULT WINAPI WshShortcut_Load(IWshShortcut *iface, BSTR PathLink)
{
    WshShortcut *This = impl_from_IWshShortcut(iface);
    FIXME("(%p)->(%s): stub\n", This, debugstr_w(PathLink));
    return E_NOTIMPL;
}

static HRESULT WINAPI WshShortcut_Save(IWshShortcut *iface)
{
    WshShortcut *This = impl_from_IWshShortcut(iface);
    IPersistFile *file;
    HRESULT hr;

    TRACE("(%p)\n", This);

    IShellLinkW_QueryInterface(This->link, &IID_IPersistFile, (void**)&file);
    hr = IPersistFile_Save(file, This->path_link, TRUE);
    IPersistFile_Release(file);

    return hr;
}

static const IWshShortcutVtbl WshShortcutVtbl = {
    WshShortcut_QueryInterface,
    WshShortcut_AddRef,
    WshShortcut_Release,
    WshShortcut_GetTypeInfoCount,
    WshShortcut_GetTypeInfo,
    WshShortcut_GetIDsOfNames,
    WshShortcut_Invoke,
    WshShortcut_get_FullName,
    WshShortcut_get_Arguments,
    WshShortcut_put_Arguments,
    WshShortcut_get_Description,
    WshShortcut_put_Description,
    WshShortcut_get_Hotkey,
    WshShortcut_put_Hotkey,
    WshShortcut_get_IconLocation,
    WshShortcut_put_IconLocation,
    WshShortcut_put_RelativePath,
    WshShortcut_get_TargetPath,
    WshShortcut_put_TargetPath,
    WshShortcut_get_WindowStyle,
    WshShortcut_put_WindowStyle,
    WshShortcut_get_WorkingDirectory,
    WshShortcut_put_WorkingDirectory,
    WshShortcut_Load,
    WshShortcut_Save
};

static HRESULT WshShortcut_Create(const WCHAR *path, IDispatch **shortcut)
{
    WshShortcut *This;
    HRESULT hr;

    *shortcut = NULL;

    This = heap_alloc(sizeof(*This));
    if (!This) return E_OUTOFMEMORY;

    This->IWshShortcut_iface.lpVtbl = &WshShortcutVtbl;
    This->ref = 1;

    hr = CoCreateInstance(&CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER,
            &IID_IShellLinkW, (void**)&This->link);
    if (FAILED(hr))
    {
        heap_free(This);
        return hr;
    }

    This->path_link = SysAllocString(path);
    if (!This->path_link)
    {
        IShellLinkW_Release(This->link);
        heap_free(This);
        return E_OUTOFMEMORY;
    }

    init_classinfo(&IID_IWshShortcut, (IUnknown *)&This->IWshShortcut_iface, &This->classinfo);
    *shortcut = (IDispatch*)&This->IWshShortcut_iface;

    return S_OK;
}

static HRESULT WINAPI WshShell3_QueryInterface(IWshShell3 *iface, REFIID riid, void **ppv)
{
    TRACE("(%s, %p)\n", debugstr_guid(riid), ppv);

    *ppv = NULL;

    if (IsEqualGUID(riid, &IID_IDispatch) ||
        IsEqualGUID(riid, &IID_IWshShell3) ||
        IsEqualGUID(riid, &IID_IWshShell2) ||
        IsEqualGUID(riid, &IID_IWshShell) ||
        IsEqualGUID(riid, &IID_IUnknown))
    {
        *ppv = iface;
    }
    else if (IsEqualGUID(riid, &IID_IDispatchEx))
    {
        return E_NOINTERFACE;
    }
    else if (IsEqualIID(riid, &IID_IProvideClassInfo))
    {
        *ppv = &WshShell3.classinfo.IProvideClassInfo_iface;
    }
    else
    {
        WARN("unknown iface %s\n", debugstr_guid(riid));
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown *)*ppv);
    return S_OK;
}

static ULONG WINAPI WshShell3_AddRef(IWshShell3 *iface)
{
    TRACE("()\n");
    return 2;
}

static ULONG WINAPI WshShell3_Release(IWshShell3 *iface)
{
    TRACE("()\n");
    return 2;
}

static HRESULT WINAPI WshShell3_GetTypeInfoCount(IWshShell3 *iface, UINT *pctinfo)
{
    TRACE("(%p)\n", pctinfo);
    *pctinfo = 1;
    return S_OK;
}

static HRESULT WINAPI WshShell3_GetTypeInfo(IWshShell3 *iface, UINT iTInfo, LCID lcid, ITypeInfo **ppTInfo)
{
    TRACE("(%u %u %p)\n", iTInfo, lcid, ppTInfo);
    return get_typeinfo(IWshShell3_tid, ppTInfo);
}

static HRESULT WINAPI WshShell3_GetIDsOfNames(IWshShell3 *iface, REFIID riid, LPOLESTR *rgszNames,
        UINT cNames, LCID lcid, DISPID *rgDispId)
{
    ITypeInfo *typeinfo;
    HRESULT hr;

    TRACE("(%s %p %u %u %p)\n", debugstr_guid(riid), rgszNames, cNames, lcid, rgDispId);

    hr = get_typeinfo(IWshShell3_tid, &typeinfo);
    if(SUCCEEDED(hr))
    {
        hr = ITypeInfo_GetIDsOfNames(typeinfo, rgszNames, cNames, rgDispId);
        ITypeInfo_Release(typeinfo);
    }

    return hr;
}

static HRESULT WINAPI WshShell3_Invoke(IWshShell3 *iface, DISPID dispIdMember, REFIID riid, LCID lcid,
        WORD wFlags, DISPPARAMS *pDispParams, VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    ITypeInfo *typeinfo;
    HRESULT hr;

    TRACE("(%d %s %d %d %p %p %p %p)\n", dispIdMember, debugstr_guid(riid),
          lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);

    hr = get_typeinfo(IWshShell3_tid, &typeinfo);
    if(SUCCEEDED(hr))
    {
        hr = ITypeInfo_Invoke(typeinfo, &WshShell3.IWshShell3_iface, dispIdMember, wFlags,
                pDispParams, pVarResult, pExcepInfo, puArgErr);
        ITypeInfo_Release(typeinfo);
    }

    return hr;
}

static HRESULT WINAPI WshShell3_get_SpecialFolders(IWshShell3 *iface, IWshCollection **folders)
{
    TRACE("(%p)\n", folders);
    return WshCollection_Create(folders);
}

static HRESULT WINAPI WshShell3_get_Environment(IWshShell3 *iface, VARIANT *type, IWshEnvironment **env)
{
    FIXME("(%s %p): semi-stub\n", debugstr_variant(type), env);
    return WshEnvironment_Create(env);
}

static inline BOOL is_optional_argument(const VARIANT *arg)
{
    return V_VT(arg) == VT_ERROR && V_ERROR(arg) == DISP_E_PARAMNOTFOUND;
}

static WCHAR *split_command( BSTR cmd, WCHAR **params )
{
    WCHAR *ret, *ptr;
    BOOL in_quotes = FALSE;

    if (!(ret = heap_alloc((lstrlenW(cmd) + 1) * sizeof(WCHAR)))) return NULL;
    lstrcpyW( ret, cmd );

    *params = NULL;
    for (ptr = ret; *ptr; ptr++)
    {
        if (*ptr == '"') in_quotes = !in_quotes;
        else if (*ptr == ' ' && !in_quotes)
        {
            *ptr = 0;
            *params = ptr + 1;
            break;
        }
    }

    return ret;
}

static HRESULT WINAPI WshShell3_Run(IWshShell3 *iface, BSTR cmd, VARIANT *style, VARIANT *wait, DWORD *exit_code)
{
    SHELLEXECUTEINFOW info;
    int waitforprocess;
    WCHAR *file, *params;
    VARIANT s;
    HRESULT hr;
    BOOL ret;

    TRACE("(%s %s %s %p)\n", debugstr_w(cmd), debugstr_variant(style), debugstr_variant(wait), exit_code);

    if (!style || !wait || !exit_code)
        return E_POINTER;

    VariantInit(&s);
#ifdef __REACTOS__
    if (is_optional_argument(style))
    {
        V_VT(&s) = VT_I4;
        V_I4(&s) = SW_SHOW;
        hr = S_OK;
    }
    else
#endif
    hr = VariantChangeType(&s, style, 0, VT_I4);
    if (FAILED(hr))
    {
        ERR("failed to convert style argument, 0x%08x\n", hr);
        return hr;
    }

    if (is_optional_argument(wait))
        waitforprocess = 0;
    else {
        VARIANT w;

        VariantInit(&w);
        hr = VariantChangeType(&w, wait, 0, VT_I4);
        if (FAILED(hr))
            return hr;

        waitforprocess = V_I4(&w);
    }

    if (!(file = split_command(cmd, &params))) return E_OUTOFMEMORY;

    memset(&info, 0, sizeof(info));
    info.cbSize = sizeof(info);
    info.fMask = waitforprocess ? SEE_MASK_NOASYNC | SEE_MASK_NOCLOSEPROCESS : SEE_MASK_DEFAULT;
    info.lpFile = file;
    info.lpParameters = params;
    info.nShow = V_I4(&s);

    ret = ShellExecuteExW(&info);
    heap_free( file );
    if (!ret)
    {
        TRACE("ShellExecute failed, %d\n", GetLastError());
        return HRESULT_FROM_WIN32(GetLastError());
    }
    else
    {
        if (waitforprocess)
        {
            WaitForSingleObject(info.hProcess, INFINITE);
            GetExitCodeProcess(info.hProcess, exit_code);
            CloseHandle(info.hProcess);
        }
        else
            *exit_code = 0;

        return S_OK;
    }
}

struct popup_thread_param
{
    WCHAR *text;
    VARIANT title;
    VARIANT type;
    INT button;
};

static DWORD WINAPI popup_thread_proc(void *arg)
{
    static const WCHAR defaulttitleW[] = {'W','i','n','d','o','w','s',' ','S','c','r','i','p','t',' ','H','o','s','t',0};
    struct popup_thread_param *param = (struct popup_thread_param *)arg;

    param->button = MessageBoxW(NULL, param->text, is_optional_argument(&param->title) ?
            defaulttitleW : V_BSTR(&param->title), V_I4(&param->type));
    return 0;
}

static HRESULT WINAPI WshShell3_Popup(IWshShell3 *iface, BSTR text, VARIANT *seconds_to_wait, VARIANT *title,
        VARIANT *type, int *button)
{
    struct popup_thread_param param;
    DWORD tid, status;
    VARIANT timeout;
    HANDLE hthread;
    HRESULT hr;

    TRACE("(%s %s %s %s %p)\n", debugstr_w(text), debugstr_variant(seconds_to_wait), debugstr_variant(title),
        debugstr_variant(type), button);

    if (!seconds_to_wait || !title || !type || !button)
        return E_POINTER;

    VariantInit(&timeout);
    if (!is_optional_argument(seconds_to_wait))
    {
        hr = VariantChangeType(&timeout, seconds_to_wait, 0, VT_I4);
        if (FAILED(hr))
            return hr;
    }
#ifdef __REACTOS__
    else
    {
        VariantChangeType(&timeout, &timeout, 0, VT_I4);
    }
#endif

    VariantInit(&param.type);
    if (!is_optional_argument(type))
    {
        hr = VariantChangeType(&param.type, type, 0, VT_I4);
        if (FAILED(hr))
            return hr;
    }
#ifdef __REACTOS__
    else
    {
        VariantChangeType(&param.type, &param.type, 0, VT_I4);
    }
#endif

    if (is_optional_argument(title))
        param.title = *title;
    else
    {
        VariantInit(&param.title);
        hr = VariantChangeType(&param.title, title, 0, VT_BSTR);
        if (FAILED(hr))
            return hr;
    }

    param.text = text;
    param.button = -1;
    hthread = CreateThread(NULL, 0, popup_thread_proc, &param, 0, &tid);
    status = MsgWaitForMultipleObjects(1, &hthread, FALSE, V_I4(&timeout) ? V_I4(&timeout) * 1000: INFINITE, 0);
    if (status == WAIT_TIMEOUT)
    {
        PostThreadMessageW(tid, WM_QUIT, 0, 0);
        MsgWaitForMultipleObjects(1, &hthread, FALSE, INFINITE, 0);
        param.button = -1;
    }
    *button = param.button;

    VariantClear(&param.title);
    CloseHandle(hthread);

    return S_OK;
}

static HRESULT WINAPI WshShell3_CreateShortcut(IWshShell3 *iface, BSTR PathLink, IDispatch** Shortcut)
{
    TRACE("(%s %p)\n", debugstr_w(PathLink), Shortcut);
    return WshShortcut_Create(PathLink, Shortcut);
}

static HRESULT WINAPI WshShell3_ExpandEnvironmentStrings(IWshShell3 *iface, BSTR Src, BSTR* Dst)
{
    DWORD ret;

    TRACE("(%s %p)\n", debugstr_w(Src), Dst);

    if (!Src || !Dst) return E_POINTER;

    ret = ExpandEnvironmentStringsW(Src, NULL, 0);
    *Dst = SysAllocStringLen(NULL, ret);
    if (!*Dst) return E_OUTOFMEMORY;

    if (ExpandEnvironmentStringsW(Src, *Dst, ret))
        return S_OK;
    else
    {
        SysFreeString(*Dst);
        *Dst = NULL;
        return HRESULT_FROM_WIN32(GetLastError());
    }
}

static HKEY get_root_key(const WCHAR *path)
{
    static const struct {
        const WCHAR full[20];
        const WCHAR abbrev[5];
        HKEY hkey;
    } rootkeys[] = {
        { {'H','K','E','Y','_','C','U','R','R','E','N','T','_','U','S','E','R',0},     {'H','K','C','U',0}, HKEY_CURRENT_USER },
        { {'H','K','E','Y','_','L','O','C','A','L','_','M','A','C','H','I','N','E',0}, {'H','K','L','M',0}, HKEY_LOCAL_MACHINE },
        { {'H','K','E','Y','_','C','L','A','S','S','E','S','_','R','O','O','T',0},     {'H','K','C','R',0}, HKEY_CLASSES_ROOT },
        { {'H','K','E','Y','_','U','S','E','R','S',0},                                                 {0}, HKEY_USERS },
        { {'H','K','E','Y','_','C','U','R','R','E','N','T','_','C','O','N','F','I','G',0},             {0}, HKEY_CURRENT_CONFIG }
    };
    int i;

    for (i = 0; i < ARRAY_SIZE(rootkeys); i++) {
        if (!wcsncmp(path, rootkeys[i].full, lstrlenW(rootkeys[i].full)))
            return rootkeys[i].hkey;
        if (rootkeys[i].abbrev[0] && !wcsncmp(path, rootkeys[i].abbrev, lstrlenW(rootkeys[i].abbrev)))
            return rootkeys[i].hkey;
    }

    return NULL;
}

/* Caller is responsible to free 'subkey' if 'value' is not NULL */
static HRESULT split_reg_path(const WCHAR *path, WCHAR **subkey, WCHAR **value)
{
    *value = NULL;

    /* at least one separator should be present */
    *subkey = wcschr(path, '\\');
    if (!*subkey)
        return HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND);

    /* default value or not */
    if ((*subkey)[lstrlenW(*subkey)-1] == '\\') {
        (*subkey)++;
        *value = NULL;
    }
    else {
        *value = wcsrchr(*subkey, '\\');
        if (*value - *subkey > 1) {
            unsigned int len = *value - *subkey - 1;
            WCHAR *ret;

            ret = heap_alloc((len + 1)*sizeof(WCHAR));
            if (!ret)
                return E_OUTOFMEMORY;

            memcpy(ret, *subkey + 1, len*sizeof(WCHAR));
            ret[len] = 0;
            *subkey = ret;
        }
        (*value)++;
    }

    return S_OK;
}

static HRESULT WINAPI WshShell3_RegRead(IWshShell3 *iface, BSTR name, VARIANT *value)
{
    DWORD type, datalen, ret;
    WCHAR *subkey, *val;
    HRESULT hr;
    HKEY root;

    TRACE("(%s %p)\n", debugstr_w(name), value);

    if (!name || !value)
        return E_POINTER;

    root = get_root_key(name);
    if (!root)
        return HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND);

    hr = split_reg_path(name, &subkey, &val);
    if (FAILED(hr))
        return hr;

    type = REG_NONE;
    datalen = 0;
    ret = RegGetValueW(root, subkey, val, RRF_RT_ANY, &type, NULL, &datalen);
    if (ret == ERROR_SUCCESS) {
        void *data;

        data = heap_alloc(datalen);
        if (!data) {
            hr = E_OUTOFMEMORY;
            goto fail;
        }

        ret = RegGetValueW(root, subkey, val, RRF_RT_ANY, &type, data, &datalen);
        if (ret) {
            heap_free(data);
            hr = HRESULT_FROM_WIN32(ret);
            goto fail;
        }

        switch (type) {
        case REG_SZ:
        case REG_EXPAND_SZ:
            V_VT(value) = VT_BSTR;
            V_BSTR(value) = SysAllocString((WCHAR*)data);
            if (!V_BSTR(value))
                hr = E_OUTOFMEMORY;
            break;
        case REG_DWORD:
            V_VT(value) = VT_I4;
            V_I4(value) = *(DWORD*)data;
            break;
        case REG_BINARY:
        {
            BYTE *ptr = (BYTE*)data;
            SAFEARRAYBOUND bound;
            unsigned int i;
            SAFEARRAY *sa;
            VARIANT *v;

            bound.lLbound = 0;
            bound.cElements = datalen;
            sa = SafeArrayCreate(VT_VARIANT, 1, &bound);
            if (!sa)
                break;

            hr = SafeArrayAccessData(sa, (void**)&v);
            if (FAILED(hr)) {
                SafeArrayDestroy(sa);
                break;
            }

            for (i = 0; i < datalen; i++) {
                V_VT(&v[i]) = VT_UI1;
                V_UI1(&v[i]) = ptr[i];
            }
            SafeArrayUnaccessData(sa);

            V_VT(value) = VT_ARRAY|VT_VARIANT;
            V_ARRAY(value) = sa;
            break;
        }
        case REG_MULTI_SZ:
        {
            WCHAR *ptr = (WCHAR*)data;
            SAFEARRAYBOUND bound;
            SAFEARRAY *sa;
            VARIANT *v;

            /* get element count first */
            bound.lLbound = 0;
            bound.cElements = 0;
            while (*ptr) {
                bound.cElements++;
                ptr += lstrlenW(ptr)+1;
            }

            sa = SafeArrayCreate(VT_VARIANT, 1, &bound);
            if (!sa)
                break;

            hr = SafeArrayAccessData(sa, (void**)&v);
            if (FAILED(hr)) {
                SafeArrayDestroy(sa);
                break;
            }

            ptr = (WCHAR*)data;
            while (*ptr) {
                V_VT(v) = VT_BSTR;
                V_BSTR(v) = SysAllocString(ptr);
                ptr += lstrlenW(ptr)+1;
                v++;
            }

            SafeArrayUnaccessData(sa);
            V_VT(value) = VT_ARRAY|VT_VARIANT;
            V_ARRAY(value) = sa;
            break;
        }
        default:
            FIXME("value type %d not supported\n", type);
            hr = E_FAIL;
        };

        heap_free(data);
        if (FAILED(hr))
            VariantInit(value);
    }
    else
        hr = HRESULT_FROM_WIN32(ret);

fail:
    if (val)
        heap_free(subkey);
    return hr;
}

static HRESULT WINAPI WshShell3_RegWrite(IWshShell3 *iface, BSTR name, VARIANT *value, VARIANT *type)
{
    static const WCHAR regexpandszW[] = {'R','E','G','_','E','X','P','A','N','D','_','S','Z',0};
    static const WCHAR regszW[] = {'R','E','G','_','S','Z',0};
    static const WCHAR regdwordW[] = {'R','E','G','_','D','W','O','R','D',0};
    static const WCHAR regbinaryW[] = {'R','E','G','_','B','I','N','A','R','Y',0};

    DWORD regtype, data_len;
    WCHAR *subkey, *val;
    const BYTE *data;
    HRESULT hr;
    HKEY root;
    VARIANT v;
    LONG ret;

    TRACE("(%s %s %s)\n", debugstr_w(name), debugstr_variant(value), debugstr_variant(type));

    if (!name || !value || !type)
        return E_POINTER;

    root = get_root_key(name);
    if (!root)
        return HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND);

    /* value type */
    if (is_optional_argument(type))
        regtype = REG_SZ;
    else {
        if (V_VT(type) != VT_BSTR)
            return E_INVALIDARG;

        if (!wcscmp(V_BSTR(type), regszW))
            regtype = REG_SZ;
        else if (!wcscmp(V_BSTR(type), regdwordW))
            regtype = REG_DWORD;
        else if (!wcscmp(V_BSTR(type), regexpandszW))
            regtype = REG_EXPAND_SZ;
        else if (!wcscmp(V_BSTR(type), regbinaryW))
            regtype = REG_BINARY;
        else {
            FIXME("unrecognized value type %s\n", debugstr_w(V_BSTR(type)));
            return E_FAIL;
        }
    }

    /* it's always a string or a DWORD */
    VariantInit(&v);
    switch (regtype)
    {
    case REG_SZ:
    case REG_EXPAND_SZ:
        hr = VariantChangeType(&v, value, 0, VT_BSTR);
        if (hr == S_OK) {
            data = (BYTE*)V_BSTR(&v);
            data_len = SysStringByteLen(V_BSTR(&v)) + sizeof(WCHAR);
        }
        break;
    case REG_DWORD:
    case REG_BINARY:
        hr = VariantChangeType(&v, value, 0, VT_I4);
        data = (BYTE*)&V_I4(&v);
        data_len = sizeof(DWORD);
        break;
    default:
        FIXME("unexpected regtype %d\n", regtype);
        return E_FAIL;
    };

    if (FAILED(hr)) {
        FIXME("failed to convert value, regtype %d, 0x%08x\n", regtype, hr);
        return hr;
    }

    hr = split_reg_path(name, &subkey, &val);
    if (FAILED(hr))
        goto fail;

    ret = RegSetKeyValueW(root, subkey, val, regtype, data, data_len);
    if (ret)
        hr = HRESULT_FROM_WIN32(ret);

fail:
    VariantClear(&v);
    if (val)
        heap_free(subkey);
    return hr;
}

static HRESULT WINAPI WshShell3_RegDelete(IWshShell3 *iface, BSTR Name)
{
    FIXME("(%s): stub\n", debugstr_w(Name));
    return E_NOTIMPL;
}

static HRESULT WINAPI WshShell3_LogEvent(IWshShell3 *iface, VARIANT *Type, BSTR Message, BSTR Target, VARIANT_BOOL *out_Success)
{
    FIXME("(%s %s %s %p): stub\n", debugstr_variant(Type), debugstr_w(Message), debugstr_w(Target), out_Success);
    return E_NOTIMPL;
}

static HRESULT WINAPI WshShell3_AppActivate(IWshShell3 *iface, VARIANT *App, VARIANT *Wait, VARIANT_BOOL *out_Success)
{
    FIXME("(%s %s %p): stub\n", debugstr_variant(App), debugstr_variant(Wait), out_Success);
    return E_NOTIMPL;
}

static HRESULT WINAPI WshShell3_SendKeys(IWshShell3 *iface, BSTR Keys, VARIANT *Wait)
{
    FIXME("(%s %p): stub\n", debugstr_w(Keys), Wait);
    return E_NOTIMPL;
}

static HRESULT WINAPI WshShell3_Exec(IWshShell3 *iface, BSTR command, IWshExec **ret)
{
    TRACE("(%s %p)\n", debugstr_w(command), ret);

    if (!ret)
        return E_POINTER;

    if (!command)
        return DISP_E_EXCEPTION;

    return WshExec_create(command, ret);
}

static HRESULT WINAPI WshShell3_get_CurrentDirectory(IWshShell3 *iface, BSTR *dir)
{
    DWORD ret;

    TRACE("(%p)\n", dir);

    ret = GetCurrentDirectoryW(0, NULL);
    if (!ret)
        return HRESULT_FROM_WIN32(GetLastError());

    *dir = SysAllocStringLen(NULL, ret-1);
    if (!*dir)
        return E_OUTOFMEMORY;

    ret = GetCurrentDirectoryW(ret, *dir);
    if (!ret) {
        SysFreeString(*dir);
        *dir = NULL;
        return HRESULT_FROM_WIN32(GetLastError());
    }

    return S_OK;
}

static HRESULT WINAPI WshShell3_put_CurrentDirectory(IWshShell3 *iface, BSTR dir)
{
    TRACE("(%s)\n", debugstr_w(dir));

    if (!dir)
        return E_INVALIDARG;

    if (!SetCurrentDirectoryW(dir))
        return HRESULT_FROM_WIN32(GetLastError());

    return S_OK;
}

static const IWshShell3Vtbl WshShell3Vtbl = {
    WshShell3_QueryInterface,
    WshShell3_AddRef,
    WshShell3_Release,
    WshShell3_GetTypeInfoCount,
    WshShell3_GetTypeInfo,
    WshShell3_GetIDsOfNames,
    WshShell3_Invoke,
    WshShell3_get_SpecialFolders,
    WshShell3_get_Environment,
    WshShell3_Run,
    WshShell3_Popup,
    WshShell3_CreateShortcut,
    WshShell3_ExpandEnvironmentStrings,
    WshShell3_RegRead,
    WshShell3_RegWrite,
    WshShell3_RegDelete,
    WshShell3_LogEvent,
    WshShell3_AppActivate,
    WshShell3_SendKeys,
    WshShell3_Exec,
    WshShell3_get_CurrentDirectory,
    WshShell3_put_CurrentDirectory
};

HRESULT WINAPI WshShellFactory_CreateInstance(IClassFactory *iface, IUnknown *outer, REFIID riid, void **ppv)
{
    TRACE("(%p %s %p)\n", outer, debugstr_guid(riid), ppv);

    WshShell3.IWshShell3_iface.lpVtbl = &WshShell3Vtbl;
    init_classinfo(&IID_IWshShell3, (IUnknown *)&WshShell3.IWshShell3_iface, &WshShell3.classinfo);
    return IWshShell3_QueryInterface(&WshShell3.IWshShell3_iface, riid, ppv);
}
