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

#include "wshom_private.h"

#include <shellapi.h>
#include <shlobj.h>
#include <dispex.h>
#include <winreg.h>

#include <wine/unicode.h>

/* FIXME: Vista+*/
LONG WINAPI RegSetKeyValueW( HKEY hkey, LPCWSTR subkey, LPCWSTR name, DWORD type, const void *data, DWORD len )
{
    HKEY hsubkey = NULL;
    DWORD ret;

    TRACE("(%p,%s,%s,%d,%p,%d)\n", hkey, debugstr_w(subkey), debugstr_w(name), type, data, len );

    if (subkey && subkey[0])  /* need to create the subkey */
    {
        if ((ret = RegCreateKeyW( hkey, subkey, &hsubkey )) != ERROR_SUCCESS) return ret;
        hkey = hsubkey;
    }

    ret = RegSetValueExW( hkey, name, 0, type, (const BYTE*)data, len );
    if (hsubkey) RegCloseKey( hsubkey );
    return ret;
}

static IWshShell3 WshShell3;

typedef struct
{
    IWshCollection IWshCollection_iface;
    LONG ref;
} WshCollection;

typedef struct
{
    IWshShortcut IWshShortcut_iface;
    LONG ref;

    IShellLinkW *link;
    BSTR path_link;
} WshShortcut;

typedef struct
{
    IWshEnvironment IWshEnvironment_iface;
    LONG ref;
} WshEnvironment;

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

static HRESULT WINAPI WshEnvironment_QueryInterface(IWshEnvironment *iface, REFIID riid, void **obj)
{
    WshEnvironment *This = impl_from_IWshEnvironment(iface);

    TRACE("(%p)->(%s, %p)\n", This, debugstr_guid(riid), obj);

    if (IsEqualGUID(riid, &IID_IUnknown)  ||
        IsEqualGUID(riid, &IID_IDispatch) ||
        IsEqualGUID(riid, &IID_IWshEnvironment))
    {
        *obj = iface;
    }else {
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
        HeapFree(GetProcessHeap(), 0, This);

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
    *value = SysAllocStringLen(NULL, len);
    if (!*value)
        return E_OUTOFMEMORY;

    if (len)
        GetEnvironmentVariableW(name, *value, len+1);

    return S_OK;
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

    This = HeapAlloc(GetProcessHeap(), 0, sizeof(*This));
    if (!This) return E_OUTOFMEMORY;

    This->IWshEnvironment_iface.lpVtbl = &WshEnvironmentVtbl;
    This->ref = 1;

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
    }else {
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
        HeapFree(GetProcessHeap(), 0, This);

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
    if (!strcmpiW(folder, desktopW))
        kind = CSIDL_DESKTOP;
    else if (!strcmpiW(folder, allusersdesktopW))
        kind = CSIDL_COMMON_DESKTOPDIRECTORY;
    else if (!strcmpiW(folder, allusersprogramsW))
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

static HRESULT WINAPI WshCollection__NewEnum(IWshCollection *iface, IUnknown *Enum)
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

    This = HeapAlloc(GetProcessHeap(), 0, sizeof(*This));
    if (!This) return E_OUTOFMEMORY;

    This->IWshCollection_iface.lpVtbl = &WshCollectionVtbl;
    This->ref = 1;

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
    }else {
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
        HeapFree(GetProcessHeap(), 0, This);
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

    hr = IShellLinkW_GetArguments(This->link, buffW, sizeof(buffW)/sizeof(WCHAR));
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

    hr = IShellLinkW_GetIconLocation(This->link, buffW, sizeof(buffW)/sizeof(WCHAR), &icon);
    if (FAILED(hr)) return hr;

    sprintfW(pathW, fmtW, buffW, icon);
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
    ptr = strrchrW(IconPath, ',');
    if (!ptr)
    {
        WARN("icon index not found\n");
        return E_FAIL;
    }

    path = SysAllocStringLen(IconPath, ptr-IconPath);

    /* skip spaces if any */
    while (isspaceW(*++ptr))
        ;

    icon = atoiW(ptr);

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
    hr = IShellLinkW_GetWorkingDirectory(This->link, buffW, sizeof(buffW)/sizeof(WCHAR));
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

    This = HeapAlloc(GetProcessHeap(), 0, sizeof(*This));
    if (!This) return E_OUTOFMEMORY;

    This->IWshShortcut_iface.lpVtbl = &WshShortcutVtbl;
    This->ref = 1;

    hr = CoCreateInstance(&CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER,
            &IID_IShellLinkW, (void**)&This->link);
    if (FAILED(hr))
    {
        HeapFree(GetProcessHeap(), 0, This);
        return hr;
    }

    This->path_link = SysAllocString(path);
    if (!This->path_link)
    {
        IShellLinkW_Release(This->link);
        HeapFree(GetProcessHeap(), 0, This);
        return E_OUTOFMEMORY;
    }

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
    else
    {
        WARN("unknown iface %s\n", debugstr_guid(riid));
        return E_NOINTERFACE;
    }

    IWshShell3_AddRef(iface);
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
        hr = ITypeInfo_Invoke(typeinfo, &WshShell3, dispIdMember, wFlags,
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

static HRESULT WINAPI WshShell3_Run(IWshShell3 *iface, BSTR cmd, VARIANT *style, VARIANT *wait, DWORD *exit_code)
{
    SHELLEXECUTEINFOW info;
    int waitforprocess;
    VARIANT s;
    HRESULT hr;

    TRACE("(%s %s %s %p)\n", debugstr_w(cmd), debugstr_variant(style), debugstr_variant(wait), exit_code);

    if (!style || !wait || !exit_code)
        return E_POINTER;

    VariantInit(&s);
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

    memset(&info, 0, sizeof(info));
    info.cbSize = sizeof(info);
    info.fMask = waitforprocess ? SEE_MASK_NOASYNC | SEE_MASK_NOCLOSEPROCESS : SEE_MASK_DEFAULT;
    info.lpFile = cmd;
    info.nShow = V_I4(&s);

    if (!ShellExecuteExW(&info))
    {
        TRACE("ShellExecute failed, %d\n", GetLastError());
        return HRESULT_FROM_WIN32(GetLastError());
    }
    else
    {
        if (waitforprocess)
        {
            GetExitCodeProcess(info.hProcess, exit_code);
            CloseHandle(info.hProcess);
        }
        else
            *exit_code = 0;

        return S_OK;
    }
}

static HRESULT WINAPI WshShell3_Popup(IWshShell3 *iface, BSTR Text, VARIANT* SecondsToWait, VARIANT *Title, VARIANT *Type, int *button)
{
    FIXME("(%s %s %s %s %p): stub\n", debugstr_w(Text), debugstr_variant(SecondsToWait),
        debugstr_variant(Title), debugstr_variant(Type), button);
    return E_NOTIMPL;
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

    for (i = 0; i < sizeof(rootkeys)/sizeof(rootkeys[0]); i++) {
        if (!strncmpW(path, rootkeys[i].full, strlenW(rootkeys[i].full)))
            return rootkeys[i].hkey;
        if (rootkeys[i].abbrev[0] && !strncmpW(path, rootkeys[i].abbrev, strlenW(rootkeys[i].abbrev)))
            return rootkeys[i].hkey;
    }

    return NULL;
}

/* Caller is responsible to free 'subkey' if 'value' is not NULL */
static HRESULT split_reg_path(const WCHAR *path, WCHAR **subkey, WCHAR **value)
{
    *value = NULL;

    /* at least one separator should be present */
    *subkey = strchrW(path, '\\');
    if (!*subkey)
        return HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND);

    /* default value or not */
    if ((*subkey)[strlenW(*subkey)-1] == '\\') {
        (*subkey)++;
        *value = NULL;
    }
    else {
        *value = strrchrW(*subkey, '\\');
        if (*value - *subkey > 1) {
            unsigned int len = *value - *subkey - 1;
            WCHAR *ret;

            ret = HeapAlloc(GetProcessHeap(), 0, (len+1)*sizeof(WCHAR));
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

        data = HeapAlloc(GetProcessHeap(), 0, datalen);
        if (!data) {
            hr = E_OUTOFMEMORY;
            goto fail;
        }

        ret = RegGetValueW(root, subkey, val, RRF_RT_ANY, &type, data, &datalen);
        if (ret) {
            HeapFree(GetProcessHeap(), 0, data);
            hr = HRESULT_FROM_WIN32(ret);
            goto fail;
        }

        switch (type) {
        case REG_SZ:
        case REG_EXPAND_SZ:
            V_VT(value) = VT_BSTR;
            V_BSTR(value) = SysAllocStringLen((WCHAR*)data, datalen - sizeof(WCHAR));
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
                ptr += strlenW(ptr)+1;
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
                ptr += strlenW(ptr)+1;
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

        HeapFree(GetProcessHeap(), 0, data);
        if (FAILED(hr))
            VariantInit(value);
    }
    else
        hr = HRESULT_FROM_WIN32(ret);

fail:
    if (val)
        HeapFree(GetProcessHeap(), 0, subkey);
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

        if (!strcmpW(V_BSTR(type), regszW))
            regtype = REG_SZ;
        else if (!strcmpW(V_BSTR(type), regdwordW))
            regtype = REG_DWORD;
        else if (!strcmpW(V_BSTR(type), regexpandszW))
            regtype = REG_EXPAND_SZ;
        else if (!strcmpW(V_BSTR(type), regbinaryW))
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
        HeapFree(GetProcessHeap(), 0, subkey);
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
    WshShell3_SendKeys
};

static IWshShell3 WshShell3 = { &WshShell3Vtbl };

HRESULT WINAPI WshShellFactory_CreateInstance(IClassFactory *iface, IUnknown *outer, REFIID riid, void **ppv)
{
    TRACE("(%p %s %p)\n", outer, debugstr_guid(riid), ppv);

    return IWshShell3_QueryInterface(&WshShell3, riid, ppv);
}
