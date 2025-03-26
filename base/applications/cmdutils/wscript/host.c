/*
 * Copyright 2010 Jacek Caban for CodeWeavers
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

#define COBJMACROS
#define CONST_VTABLE

#include <windef.h>
#include <winbase.h>
#include <ole2.h>

#include "wscript.h"

#include <wine/debug.h>
#include <wine/heap.h>

WINE_DEFAULT_DEBUG_CHANNEL(wscript);

#define BUILDVERSION 16535

static const WCHAR wshNameW[] = {'W','i','n','d','o','w','s',' ','S','c','r','i','p','t',' ','H','o','s','t',0};
static const WCHAR wshVersionW[] = {'5','.','8'};

VARIANT_BOOL wshInteractive =
#ifndef CSCRIPT_BUILD
    VARIANT_TRUE;
#else
    VARIANT_FALSE;
#endif

static HRESULT to_string(VARIANT *src, BSTR *dst)
{
    VARIANT v;
    HRESULT hres;

    static const WCHAR nullW[] = {'n','u','l','l',0};

    if(V_VT(src) == VT_NULL) {
        *dst = SysAllocString(nullW);
        return *dst ? S_OK : E_OUTOFMEMORY;
    }

    V_VT(&v) = VT_EMPTY;
    hres = VariantChangeType(&v, src, 0, VT_BSTR);
    if(FAILED(hres)) {
        WARN("Could not convert argument %s to string\n", debugstr_variant(src));
        return hres;
    }

    *dst = V_BSTR(&v);
    return S_OK;
}

static void print_string(const WCHAR *string)
{
    DWORD count, ret, len, lena;
    char *buf;

    if(wshInteractive) {
        static const WCHAR windows_script_hostW[] =
            {'W','i','n','d','o','w','s',' ','S','c','r','i','p','t',' ','H','o','s','t',0};
        MessageBoxW(NULL, string, windows_script_hostW, MB_OK);
        return;
    }

    len = lstrlenW(string);
    ret = WriteConsoleW(GetStdHandle(STD_OUTPUT_HANDLE), string, len, &count, NULL);
    if(ret) {
        static const WCHAR crnlW[] = {'\r','\n'};
        WriteConsoleW(GetStdHandle(STD_OUTPUT_HANDLE), crnlW, ARRAY_SIZE(crnlW), &count, NULL);
        return;
    }

    lena = WideCharToMultiByte(GetConsoleOutputCP(), 0, string, len, NULL, 0, NULL, NULL);
    buf = heap_alloc(len);
    if(!buf)
        return;

    WideCharToMultiByte(GetConsoleOutputCP(), 0, string, len, buf, lena, NULL, NULL);
    WriteFile(GetStdHandle(STD_OUTPUT_HANDLE), buf, lena, &count, FALSE);
    heap_free(buf);
    WriteFile(GetStdHandle(STD_OUTPUT_HANDLE), "\r\n", 2, &count, FALSE);
}

static HRESULT WINAPI Host_QueryInterface(IHost *iface, REFIID riid, void **ppv)
{
    WINE_TRACE("(%s %p)\n", wine_dbgstr_guid(riid), ppv);

    if(IsEqualGUID(&IID_IUnknown, riid)
       || IsEqualGUID(&IID_IDispatch, riid)
       || IsEqualGUID(&IID_IHost, riid)) {
        *ppv = iface;
        return S_OK;
    }

    *ppv = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI Host_AddRef(IHost *iface)
{
    return 2;
}

static ULONG WINAPI Host_Release(IHost *iface)
{
    return 1;
}

static HRESULT WINAPI Host_GetTypeInfoCount(IHost *iface, UINT *pctinfo)
{
    WINE_TRACE("(%p)\n", pctinfo);
    *pctinfo = 1;
    return S_OK;
}

static HRESULT WINAPI Host_GetTypeInfo(IHost *iface, UINT iTInfo, LCID lcid,
        ITypeInfo **ppTInfo)
{
    WINE_TRACE("(%x %x %p\n", iTInfo, lcid, ppTInfo);

    ITypeInfo_AddRef(host_ti);
    *ppTInfo = host_ti;
    return S_OK;
}

static HRESULT WINAPI Host_GetIDsOfNames(IHost *iface, REFIID riid, LPOLESTR *rgszNames,
        UINT cNames, LCID lcid, DISPID *rgDispId)
{
    WINE_TRACE("(%s %p %d %x %p)\n", wine_dbgstr_guid(riid), rgszNames,
        cNames, lcid, rgDispId);

    return ITypeInfo_GetIDsOfNames(host_ti, rgszNames, cNames, rgDispId);
}

static HRESULT WINAPI Host_Invoke(IHost *iface, DISPID dispIdMember, REFIID riid,
        LCID lcid, WORD wFlags, DISPPARAMS *pDispParams, VARIANT *pVarResult,
        EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    WINE_TRACE("(%d %p %p)\n", dispIdMember, pDispParams, pVarResult);

    return ITypeInfo_Invoke(host_ti, iface, dispIdMember, wFlags, pDispParams,
            pVarResult, pExcepInfo, puArgErr);
}

static HRESULT WINAPI Host_get_Name(IHost *iface, BSTR *out_Name)
{
    WINE_TRACE("(%p)\n", out_Name);

    if(!(*out_Name = SysAllocString(wshNameW)))
	return E_OUTOFMEMORY;
    return S_OK;
}

static HRESULT WINAPI Host_get_Application(IHost *iface, IDispatch **out_Dispatch)
{
    WINE_TRACE("(%p)\n", out_Dispatch);

    *out_Dispatch = (IDispatch*)&host_obj;
    return S_OK;
}

static HRESULT WINAPI Host_get_FullName(IHost *iface, BSTR *out_Path)
{
    WCHAR fullPath[MAX_PATH];

    WINE_TRACE("(%p)\n", out_Path);

    if(GetModuleFileNameW(NULL, fullPath, ARRAY_SIZE(fullPath)) == 0)
        return E_FAIL;
    if(!(*out_Path = SysAllocString(fullPath)))
        return E_OUTOFMEMORY;
    return S_OK;
}

static HRESULT WINAPI Host_get_Path(IHost *iface, BSTR *out_Path)
{
    WCHAR path[MAX_PATH];
    int howMany;
    WCHAR *pos;

    WINE_TRACE("(%p)\n", out_Path);

    if(GetModuleFileNameW(NULL, path, ARRAY_SIZE(path)) == 0)
        return E_FAIL;
    pos = wcsrchr(path, '\\');
    howMany = pos - path;
    if(!(*out_Path = SysAllocStringLen(path, howMany)))
        return E_OUTOFMEMORY;
    return S_OK;
}

static HRESULT WINAPI Host_get_Interactive(IHost *iface, VARIANT_BOOL *out_Interactive)
{
    WINE_TRACE("(%p)\n", out_Interactive);

    *out_Interactive = wshInteractive;
    return S_OK;
}

static HRESULT WINAPI Host_put_Interactive(IHost *iface, VARIANT_BOOL v)
{
    WINE_TRACE("(%x)\n", v);

    wshInteractive = v;
    return S_OK;
}

static HRESULT WINAPI Host_Quit(IHost *iface, int ExitCode)
{
    FIXME("(%d) semi-stub: no script engine clean up\n", ExitCode);

    ExitProcess(ExitCode);
    return S_OK;
}

static HRESULT WINAPI Host_get_ScriptName(IHost *iface, BSTR *out_ScriptName)
{
    WCHAR *scriptName;

    WINE_TRACE("(%p)\n", out_ScriptName);

    scriptName = wcsrchr(scriptFullName, '\\');
    ++scriptName;
    if(!(*out_ScriptName = SysAllocString(scriptName)))
        return E_OUTOFMEMORY;
    return S_OK;
}

static HRESULT WINAPI Host_get_ScriptFullName(IHost *iface, BSTR *out_ScriptFullName)
{
    WINE_TRACE("(%p)\n", out_ScriptFullName);

    if(!(*out_ScriptFullName = SysAllocString(scriptFullName)))
        return E_OUTOFMEMORY;
    return S_OK;
}

static HRESULT WINAPI Host_get_Arguments(IHost *iface, IArguments2 **out_Arguments)
{
    WINE_TRACE("(%p)\n", out_Arguments);

    *out_Arguments = &arguments_obj;
    return S_OK;
}

static HRESULT WINAPI Host_get_Version(IHost *iface, BSTR *out_Version)
{
    WINE_TRACE("(%p)\n", out_Version);

    if(!(*out_Version = SysAllocString(wshVersionW)))
	return E_OUTOFMEMORY;
    return S_OK;
}

static HRESULT WINAPI Host_get_BuildVersion(IHost *iface, int *out_Build)
{
    WINE_TRACE("(%p)\n", out_Build);

    *out_Build = BUILDVERSION;
    return S_OK;
}

static HRESULT WINAPI Host_get_Timeout(IHost *iface, LONG *out_Timeout)
{
    WINE_FIXME("(%p)\n", out_Timeout);
    return E_NOTIMPL;
}

static HRESULT WINAPI Host_put_Timeout(IHost *iface, LONG v)
{
    WINE_FIXME("(%d)\n", v);
    return E_NOTIMPL;
}

static HRESULT WINAPI Host_CreateObject(IHost *iface, BSTR ProgID, BSTR Prefix,
        IDispatch **out_Dispatch)
{
    IUnknown *unk;
    GUID guid;
    HRESULT hres;

    TRACE("(%s %s %p)\n", wine_dbgstr_w(ProgID), wine_dbgstr_w(Prefix), out_Dispatch);

    if(Prefix && *Prefix) {
        FIXME("Prefix %s not supported\n", debugstr_w(Prefix));
        return E_NOTIMPL;
    }

    hres = CLSIDFromProgID(ProgID, &guid);
    if(FAILED(hres))
        return hres;

    hres = CoCreateInstance(&guid, NULL, CLSCTX_INPROC_SERVER|CLSCTX_LOCAL_SERVER|CLSCTX_REMOTE_SERVER,
            &IID_IUnknown, (void**)&unk);
    if(FAILED(hres))
        return hres;

    hres = IUnknown_QueryInterface(unk, &IID_IDispatch, (void**)out_Dispatch);
    IUnknown_Release(unk);
    return hres;
}

static HRESULT WINAPI Host_Echo(IHost *iface, SAFEARRAY *args)
{
    WCHAR *output = NULL, *ptr;
    unsigned argc, i, len;
#ifdef __REACTOS__
    LONG ubound, lbound;
#else
    int ubound, lbound;
#endif
    VARIANT *argv;
    BSTR *strs;
    HRESULT hres;

    TRACE("(%p)\n", args);

    if(SafeArrayGetDim(args) != 1) {
        FIXME("Unsupported args dim %d\n", SafeArrayGetDim(args));
        return E_NOTIMPL;
    }

    SafeArrayGetLBound(args, 1, &lbound);
    SafeArrayGetUBound(args, 1, &ubound);

    hres = SafeArrayAccessData(args, (void**)&argv);
    if(FAILED(hres))
        return hres;

    argc = ubound-lbound+1;
    strs = heap_alloc_zero(argc*sizeof(*strs));
    if(!strs) {
        SafeArrayUnaccessData(args);
        return E_OUTOFMEMORY;
    }

    /* Len of spaces between arguments. */
    len = argc-1;

    for(i=0; i < argc; i++) {
        hres = to_string(argv+i, strs+i);
        if(FAILED(hres))
            break;

        len += SysStringLen(strs[i]);
    }

    SafeArrayUnaccessData(args);
    if(SUCCEEDED(hres)) {
        ptr = output = heap_alloc((len+1)*sizeof(WCHAR));
        if(output) {
            for(i=0; i < argc; i++) {
                if(i)
                    *ptr++ = ' ';
                len = SysStringLen(strs[i]);
                memcpy(ptr, strs[i], len*sizeof(WCHAR));
                ptr += len;
            }
            *ptr = 0;
        }else {
            hres = E_OUTOFMEMORY;
        }
    }

    for(i=0; i < argc; i++)
        SysFreeString(strs[i]);
    heap_free(strs);
    if(FAILED(hres))
        return hres;

    print_string(output);

    heap_free(output);
    return S_OK;
}

static HRESULT WINAPI Host_GetObject(IHost *iface, BSTR Pathname, BSTR ProgID,
        BSTR Prefix, IDispatch **out_Dispatch)
{
    WINE_FIXME("(%s %s %s %p)\n", wine_dbgstr_w(Pathname), wine_dbgstr_w(ProgID),
        wine_dbgstr_w(Prefix), out_Dispatch);
    return E_NOTIMPL;
}

static HRESULT WINAPI Host_DisconnectObject(IHost *iface, IDispatch *Object)
{
    WINE_FIXME("(%p)\n", Object);
    return E_NOTIMPL;
}

static HRESULT WINAPI Host_Sleep(IHost *iface, LONG Time)
{
#ifdef __REACTOS__
    UNREFERENCED_PARAMETER(iface);
    if (Time < 0)
        return E_INVALIDARG;
    Sleep(Time);
    return S_OK;
#else
    WINE_FIXME("(%d)\n", Time);
    return E_NOTIMPL;
#endif
}

static HRESULT WINAPI Host_ConnectObject(IHost *iface, IDispatch *Object, BSTR Prefix)
{
    WINE_FIXME("(%p %s)\n", Object, wine_dbgstr_w(Prefix));
    return E_NOTIMPL;
}

static HRESULT WINAPI Host_get_StdIn(IHost *iface, ITextStream **ppts)
{
    WINE_FIXME("(%p)\n", ppts);
    return E_NOTIMPL;
}

static HRESULT WINAPI Host_get_StdOut(IHost *iface, ITextStream **ppts)
{
    WINE_FIXME("(%p)\n", ppts);
    return E_NOTIMPL;
}

static HRESULT WINAPI Host_get_StdErr(IHost *iface, ITextStream **ppts)
{
    WINE_FIXME("(%p)\n", ppts);
    return E_NOTIMPL;
}

static const IHostVtbl HostVtbl = {
    Host_QueryInterface,
    Host_AddRef,
    Host_Release,
    Host_GetTypeInfoCount,
    Host_GetTypeInfo,
    Host_GetIDsOfNames,
    Host_Invoke,
    Host_get_Name,
    Host_get_Application,
    Host_get_FullName,
    Host_get_Path,
    Host_get_Interactive,
    Host_put_Interactive,
    Host_Quit,
    Host_get_ScriptName,
    Host_get_ScriptFullName,
    Host_get_Arguments,
    Host_get_Version,
    Host_get_BuildVersion,
    Host_get_Timeout,
    Host_put_Timeout,
    Host_CreateObject,
    Host_Echo,
    Host_GetObject,
    Host_DisconnectObject,
    Host_Sleep,
    Host_ConnectObject,
    Host_get_StdIn,
    Host_get_StdOut,
    Host_get_StdErr
};

IHost host_obj = { &HostVtbl };
