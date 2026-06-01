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
#ifdef __REACTOS__
#define CONST_VTABLE
#endif

#include <windef.h>
#include <winbase.h>
#include <winreg.h>
#include <ole2.h>
#include <shellapi.h>
#include <activscp.h>
#include <initguid.h>

#include "wscript.h"
#ifdef __REACTOS__
#define IDS_USAGE              1
#define IDS_NO_SCRIPT_FILE     2
#define IDS_FILE_NOT_FOUND     3
#define IDS_SCRIPT_LOAD_ERROR  4
#define IDS_TIMEOUT_EXCEEDED   5
#else
#include "resource.h"
#endif

#include <wine/debug.h>

#ifdef _WIN64

#define IActiveScriptParse_Release IActiveScriptParse64_Release
#define IActiveScriptParse_InitNew IActiveScriptParse64_InitNew
#define IActiveScriptParse_ParseScriptText IActiveScriptParse64_ParseScriptText

#else

#define IActiveScriptParse_Release IActiveScriptParse32_Release
#define IActiveScriptParse_InitNew IActiveScriptParse32_InitNew
#define IActiveScriptParse_ParseScriptText IActiveScriptParse32_ParseScriptText

#endif

WINE_DEFAULT_DEBUG_CHANNEL(wscript);
WCHAR scriptFullName[MAX_PATH];

ITypeInfo *host_ti;
ITypeInfo *arguments_ti;

static BOOL nologo;

static HRESULT query_interface(REFIID,void**);

#ifdef __REACTOS__
#include <commctrl.h>

PCWSTR g_force_engine = NULL;

typedef struct {
    UINT itemsize, count;
    void *mem;
} SIMPLEVECTOR;

static void SVect_Free(SIMPLEVECTOR *pV)
{
    if (pV->mem)
        LocalFree(pV->mem);
    pV->mem = NULL;
}

static void* SVect_Add(SIMPLEVECTOR *pV)
{
    void *p = NULL;
    if (pV->mem)
    {
        p = LocalReAlloc(pV->mem, pV->itemsize * (pV->count + 1), LMEM_FIXED | LMEM_MOVEABLE);
        if (p)
        {
            pV->mem = p;
            p = (char*)p + (pV->count * pV->itemsize);
            pV->count++;
        }
    }
    else
    {
        p = pV->mem = LocalAlloc(LMEM_FIXED, pV->itemsize);
        if (p)
        {
            pV->count = 1;
        }
    }
    return p;
}

#define SVect_Delete(pV, pItem) ( (pV), (pItem) ) /* Should not be required for global items */

static void* SVect_Get(SIMPLEVECTOR *pV, UINT i)
{
    return pV->mem && i < pV->count ? (char*)pV->mem + (i * pV->itemsize) : NULL;
}

typedef struct {
    BSTR name;
    IUnknown *punk;
} GLOBAL_ITEM;

SIMPLEVECTOR g_global_items = { sizeof(GLOBAL_ITEM) };

static void free_globals(void)
{
    UINT i;
    for (i = 0;; ++i)
    {
        GLOBAL_ITEM *p = (GLOBAL_ITEM*)SVect_Get(&g_global_items, i);
        if (!p)
            break;
        IUnknown_Release(p->punk);
        SysFreeString(p->name);
    }
    SVect_Free(&g_global_items);
}

static HRESULT add_globalitem(IActiveScript *script, BSTR name, IUnknown *punk, DWORD siflags)
{
    GLOBAL_ITEM *item;
    HRESULT hr;

    name = SysAllocString(name);
    if (!name)
        return E_OUTOFMEMORY;

    item = SVect_Add(&g_global_items);
    if (item)
    {
        item->name = name;
        item->punk = punk;
        hr = IActiveScript_AddNamedItem(script, name, siflags);
        if (SUCCEEDED(hr))
        {
            IUnknown_AddRef(punk);
            return hr;
        }
        SVect_Delete(&g_global_items, item);
    }
    SysFreeString(name);
    return E_OUTOFMEMORY;
}

static HRESULT add_globalitem_from_clsid(IActiveScript *script, BSTR name, REFCLSID clsid, DWORD siflags)
{
    IUnknown *punk;
    HRESULT hr = CoCreateInstance(clsid, NULL, CLSCTX_INPROC_SERVER, &IID_IUnknown, (void**)&punk);
    if (SUCCEEDED(hr))
    {
        hr = add_globalitem(script, name, punk, siflags);
        IUnknown_Release(punk);
    }
    return hr;
}

static HRESULT get_globalitem_info(LPCOLESTR Name, DWORD Mask, IUnknown **ppunk, ITypeInfo **ppti, BOOL *pHandled)
{
    HRESULT hr = S_FALSE;
    UINT i;
    for (i = 0;; ++i)
    {
        GLOBAL_ITEM *p = (GLOBAL_ITEM*)SVect_Get(&g_global_items, i);
        if (!p)
            break;
        if (!lstrcmpiW(Name, p->name))
        {
            if (ppti)
                *ppti = NULL;
            if (Mask & SCRIPTINFO_IUNKNOWN)
            {
                *ppunk = p->punk;
                if (p->punk)
                {
                    IUnknown_AddRef(p->punk);
                    *pHandled = TRUE;
                }
                return S_OK;
            }
            break;
        }
    }
    return hr;
}
#endif

static HRESULT WINAPI ActiveScriptSite_QueryInterface(IActiveScriptSite *iface,
                                                      REFIID riid, void **ppv)
{
    return query_interface(riid, ppv);
}

static ULONG WINAPI ActiveScriptSite_AddRef(IActiveScriptSite *iface)
{
    return 2;
}

static ULONG WINAPI ActiveScriptSite_Release(IActiveScriptSite *iface)
{
    return 1;
}

static HRESULT WINAPI ActiveScriptSite_GetLCID(IActiveScriptSite *iface, LCID *plcid)
{
    WINE_TRACE("()\n");

    *plcid = GetUserDefaultLCID();
    return S_OK;
}

static HRESULT WINAPI ActiveScriptSite_GetItemInfo(IActiveScriptSite *iface,
        LPCOLESTR pstrName, DWORD dwReturnMask, IUnknown **ppunkItem, ITypeInfo **ppti)
{
    WINE_TRACE("(%s %lx %p %p)\n", wine_dbgstr_w(pstrName), dwReturnMask, ppunkItem, ppti);

#ifdef __REACTOS__
    {
        BOOL handled = FALSE;
        HRESULT hr = get_globalitem_info(pstrName, dwReturnMask, ppunkItem, ppti, &handled);
        if (handled)
            return hr;
    }
#endif

    if(lstrcmpW(pstrName, L"WSH") && lstrcmpW(pstrName, L"WScript"))
        return E_FAIL;

    if(dwReturnMask & SCRIPTINFO_ITYPEINFO) {
        ITypeInfo_AddRef(host_ti);
        *ppti = host_ti;
    }

    if(dwReturnMask & SCRIPTINFO_IUNKNOWN) {
        IHost_AddRef(&host_obj);
        *ppunkItem = (IUnknown*)&host_obj;
    }

    return S_OK;
}

static HRESULT WINAPI ActiveScriptSite_GetDocVersionString(IActiveScriptSite *iface,
        BSTR *pbstrVersion)
{
    WINE_FIXME("()\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ActiveScriptSite_OnScriptTerminate(IActiveScriptSite *iface,
        const VARIANT *pvarResult, const EXCEPINFO *pexcepinfo)
{
    WINE_FIXME("()\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ActiveScriptSite_OnStateChange(IActiveScriptSite *iface,
        SCRIPTSTATE ssScriptState)
{
    WINE_TRACE("(%x)\n", ssScriptState);
    return S_OK;
}

static void write_to_handle(HANDLE handle, const WCHAR *string)
{
    DWORD count, ret, len, lena;
    char *buf;

    len = lstrlenW(string);
    ret = WriteConsoleW(handle, string, len, &count, NULL);
    if(ret) {
        WriteConsoleW(handle, L"\r\n", 2, &count, NULL);
        return;
    }

    lena = WideCharToMultiByte(GetOEMCP(), 0, string, len, NULL, 0, NULL, NULL);
    buf = malloc(lena);
    if(!buf)
        return;

    WideCharToMultiByte(GetOEMCP(), 0, string, len, buf, lena, NULL, NULL);
    WriteFile(handle, buf, lena, &count, FALSE);
    free(buf);
    WriteFile(handle, "\r\n", 2, &count, FALSE);
}

static void print_error(const WCHAR *string)
{
    if(wshInteractive) {
        MessageBoxW(NULL, string, L"Windows Script Host", MB_OK);
        return;
    }

    write_to_handle(GetStdHandle(STD_ERROR_HANDLE), string);
}

static void print_string(const WCHAR *string)
{
    write_to_handle(GetStdHandle(STD_OUTPUT_HANDLE), string);
}

static void output_writeconsole(const WCHAR *str, DWORD wlen)
{
    DWORD count;

    if(!WriteConsoleW(GetStdHandle(STD_OUTPUT_HANDLE), str, wlen, &count, NULL)) {
        DWORD len;
        char *buf;

        /* On Windows WriteConsoleW() fails if the output is redirected. So fall
         * back to WriteFile() with OEM code page. */
        len = WideCharToMultiByte(GetOEMCP(), 0, str, wlen, NULL, 0, NULL, NULL);
        buf = malloc(len);

        WideCharToMultiByte(GetOEMCP(), 0, str, wlen, buf, len, NULL, NULL);
        WriteFile(GetStdHandle(STD_OUTPUT_HANDLE), buf, len, &count, FALSE);
        free(buf);
    }
}

static void output_formatstring(const WCHAR *fmt, va_list va_args)
{
    WCHAR *str;
    DWORD len;

    len = FormatMessageW(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ALLOCATE_BUFFER,
                         fmt, 0, 0, (WCHAR *)&str, 0, &va_args);
    if(len == 0 && GetLastError() != ERROR_NO_WORK_DONE) {
        WINE_FIXME("Could not format string: le=%lu, fmt=%s\n", GetLastError(), wine_dbgstr_w(fmt));
        return;
    }
    if(wshInteractive) {
        MessageBoxW(NULL, str, L"Windows Script Host", MB_OK);
    }else {
        output_writeconsole(str, len);
    }
    LocalFree(str);
}

static void WINAPIV print_resource(unsigned int id, ...)
{
    WCHAR *fmt = NULL;
    int len;
    va_list va_args;

#ifdef __REACTOS__
    #ifndef CSCRIPT_BUILD
    #define APPNAME L"WScript"
    #else
    #define APPNAME L"CScript"
    #endif
    const WCHAR *str = NULL;
    switch (id)
    {
        case IDS_USAGE: str = L"Usage: " APPNAME L" scriptname.extension [option...] [arguments...]\n"; break;
        case IDS_NO_SCRIPT_FILE: str = L"Input Error: There is no script file specified.\n"; break;
        case IDS_FILE_NOT_FOUND: str = L"Input Error: Can not find script file ""%1"".\n"; break;
        case IDS_SCRIPT_LOAD_ERROR: str = APPNAME L" Error: Loading script ""%1"" failed (%2).\n"; break;
        case IDS_TIMEOUT_EXCEEDED: str = L"Script execution time was exceeded on script ""%1"".\n\nScript execution was terminated.\n"; break;
    }
    if(!str)
    {
        WINE_FIXME("Missing string %u\n", id);
        return;
    }
    len = lstrlenW(str);
    fmt = malloc(++len * sizeof(WCHAR));
    if(!fmt)
        return;
    lstrcpyW(fmt, str);
#else
    if(!(len = LoadStringW(GetModuleHandleW(NULL), id, (WCHAR *)&fmt, 0))) {
        WINE_FIXME("LoadString failed with %ld\n", GetLastError());
        return;
    }

    len++;
    fmt = malloc(len * sizeof(WCHAR));
    if(!fmt)
        return;

    LoadStringW(GetModuleHandleW(NULL), id, fmt, len);
#endif

    va_start(va_args, id);
    output_formatstring(fmt, va_args);
    va_end(va_args);

    free(fmt);
}

static HANDLE timeout_cancel_event;
static HANDLE timeout_thread;

static DWORD WINAPI timeout_thread_proc(void *arg)
{
    DWORD ms = (DWORD)(DWORD_PTR)arg;
    if(WaitForSingleObject(timeout_cancel_event, ms) == WAIT_TIMEOUT) {
        print_resource(IDS_TIMEOUT_EXCEEDED, scriptFullName);
        ExitProcess(1);
    }
    return 0;
}

void schedule_timeout(LONG seconds)
{
    if(timeout_thread) {
        SetEvent(timeout_cancel_event);
        WaitForSingleObject(timeout_thread, INFINITE);
        CloseHandle(timeout_thread);
        timeout_thread = NULL;
    }
    if(seconds <= 0)
        return;
    if(!timeout_cancel_event)
        timeout_cancel_event = CreateEventW(NULL, TRUE, FALSE, NULL);
    else
        ResetEvent(timeout_cancel_event);
    timeout_thread = CreateThread(NULL, 0, timeout_thread_proc,
                                  (void *)(DWORD_PTR)(seconds * 1000), 0, NULL);
}

static void print_banner(void)
{
#ifdef __REACTOS__
    const WCHAR *header = L"ReactOS Script Host";
#else
    const char * (CDECL *wine_get_version)(void);
    WCHAR header[64];

    wine_get_version = (void *)GetProcAddress(GetModuleHandleW(L"ntdll.dll"), "wine_get_version");
    if(wine_get_version)
        swprintf(header, ARRAY_SIZE(header), L"Wine %S Script Host", wine_get_version());
    else
        swprintf(header, ARRAY_SIZE(header), L"Wine Script Host");
#endif
    print_string(header);
    print_string(L"");
}

static HRESULT WINAPI ActiveScriptSite_OnScriptError(IActiveScriptSite *iface,
        IActiveScriptError *pscripterror)
{
    EXCEPINFO excepinfo;
    ULONG line;
    LONG character;
    HRESULT hres;
    WCHAR buf[1024];

    WINE_TRACE("()\n");

    memset(&excepinfo, 0, sizeof(excepinfo));
    hres = IActiveScriptError_GetExceptionInfo(pscripterror, &excepinfo);
    if(SUCCEEDED(hres)) {
        line = 0;
        character = 0;
        IActiveScriptError_GetSourcePosition(pscripterror, NULL, &line, &character);

        swprintf(buf, ARRAY_SIZE(buf), L"%s(%lu, %ld) %s: %s",
                scriptFullName, line + 1, character + 1,
                excepinfo.bstrSource ? excepinfo.bstrSource : L"",
                excepinfo.bstrDescription ? excepinfo.bstrDescription : L"");

        print_error(buf);

        SysFreeString(excepinfo.bstrSource);
        SysFreeString(excepinfo.bstrDescription);
        SysFreeString(excepinfo.bstrHelpFile);
    }

    return S_OK;
}

static HRESULT WINAPI ActiveScriptSite_OnEnterScript(IActiveScriptSite *iface)
{
    WINE_TRACE("()\n");
    return S_OK;
}

static HRESULT WINAPI ActiveScriptSite_OnLeaveScript(IActiveScriptSite *iface)
{
    WINE_TRACE("()\n");
    return S_OK;
}

static IActiveScriptSiteVtbl ActiveScriptSiteVtbl = {
    ActiveScriptSite_QueryInterface,
    ActiveScriptSite_AddRef,
    ActiveScriptSite_Release,
    ActiveScriptSite_GetLCID,
    ActiveScriptSite_GetItemInfo,
    ActiveScriptSite_GetDocVersionString,
    ActiveScriptSite_OnScriptTerminate,
    ActiveScriptSite_OnStateChange,
    ActiveScriptSite_OnScriptError,
    ActiveScriptSite_OnEnterScript,
    ActiveScriptSite_OnLeaveScript
};

static IActiveScriptSite script_site = { &ActiveScriptSiteVtbl };

static HRESULT WINAPI ActiveScriptSiteWindow_QueryInterface(IActiveScriptSiteWindow *iface, REFIID riid, void **ppv)
{
    return query_interface(riid, ppv);
}

static ULONG WINAPI ActiveScriptSiteWindow_AddRef(IActiveScriptSiteWindow *iface)
{
    return 2;
}

static ULONG WINAPI ActiveScriptSiteWindow_Release(IActiveScriptSiteWindow *iface)
{
    return 1;
}

static HRESULT WINAPI ActiveScriptSiteWindow_GetWindow(IActiveScriptSiteWindow *iface, HWND *phwnd)
{
    TRACE("(%p)\n", phwnd);

    *phwnd = NULL;
    return S_OK;
}

static HRESULT WINAPI ActiveScriptSiteWindow_EnableModeless(IActiveScriptSiteWindow *iface, BOOL fEnable)
{
    TRACE("(%x)\n", fEnable);
    return S_OK;
}

static const IActiveScriptSiteWindowVtbl ActiveScriptSiteWindowVtbl = {
    ActiveScriptSiteWindow_QueryInterface,
    ActiveScriptSiteWindow_AddRef,
    ActiveScriptSiteWindow_Release,
    ActiveScriptSiteWindow_GetWindow,
    ActiveScriptSiteWindow_EnableModeless
};

static IActiveScriptSiteWindow script_site_window = { &ActiveScriptSiteWindowVtbl };

static HRESULT query_interface(REFIID riid, void **ppv)
{
    if(IsEqualGUID(riid, &IID_IUnknown)) {
        TRACE("(IID_IUnknown %p)\n", ppv);
        *ppv = &script_site;
    }else if(IsEqualGUID(riid, &IID_IActiveScriptSite)) {
        TRACE("(IID_IActiveScriptSite %p)\n", ppv);
        *ppv = &script_site;
    }else if(IsEqualGUID(riid, &IID_IActiveScriptSiteWindow)) {
        TRACE("(IID_IActiveScriptSiteWindow %p)\n", ppv);
        *ppv = &script_site_window;
    }else {
        *ppv = NULL;
        TRACE("(%s %p)\n", wine_dbgstr_guid(riid), ppv);
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static BOOL load_typelib(void)
{
    ITypeLib *typelib;
    HRESULT hres;

    hres = LoadTypeLib(L"wscript.exe", &typelib);
    if(FAILED(hres))
        return FALSE;

    hres = ITypeLib_GetTypeInfoOfGuid(typelib, &IID_IHost, &host_ti);
    if(SUCCEEDED(hres))
        hres = ITypeLib_GetTypeInfoOfGuid(typelib, &IID_IArguments2, &arguments_ti);

    ITypeLib_Release(typelib);
    return SUCCEEDED(hres);
}

static BOOL get_engine_clsid(const WCHAR *ext, CLSID *clsid)
{
    WCHAR fileid[64], progid[64];
    DWORD res;
    LONG size;
    HKEY hkey;
    HRESULT hres;

    res = RegOpenKeyW(HKEY_CLASSES_ROOT, ext, &hkey);
    if(res != ERROR_SUCCESS)
        return FALSE;

    size = ARRAY_SIZE(fileid);
    res = RegQueryValueW(hkey, NULL, fileid, &size);
    RegCloseKey(hkey);
    if(res != ERROR_SUCCESS)
        return FALSE;

    WINE_TRACE("fileid is %s\n", wine_dbgstr_w(fileid));

    lstrcatW(fileid, L"\\ScriptEngine");
    res = RegOpenKeyW(HKEY_CLASSES_ROOT, fileid, &hkey);
    if(res != ERROR_SUCCESS)
        return FALSE;

    size = ARRAY_SIZE(progid);
    res = RegQueryValueW(hkey, NULL, progid, &size);
    RegCloseKey(hkey);
    if(res != ERROR_SUCCESS)
        return FALSE;

    WINE_TRACE("ProgID is %s\n", wine_dbgstr_w(progid));

    hres = CLSIDFromProgID(progid, clsid);
    return SUCCEEDED(hres);
}

static BOOL create_engine(CLSID *clsid, IActiveScript **script_ret,
        IActiveScriptParse **parser)
{
    IActiveScript *script;
    IUnknown *unk;
    HRESULT hres;

    hres = CoCreateInstance(clsid, NULL, CLSCTX_INPROC_SERVER|CLSCTX_INPROC_HANDLER,
            &IID_IUnknown, (void**)&unk);
    if(FAILED(hres))
        return FALSE;

    hres = IUnknown_QueryInterface(unk, &IID_IActiveScript, (void**)&script);
    IUnknown_Release(unk);
    if(FAILED(hres))
        return FALSE;

    hres = IActiveScript_QueryInterface(script, &IID_IActiveScriptParse, (void**)parser);
    if(FAILED(hres)) {
        IActiveScript_Release(script);
        return FALSE;
    }

    *script_ret = script;
    return TRUE;
}

static BOOL init_engine(IActiveScript *script, IActiveScriptParse *parser)
{
    HRESULT hres;

    if(!load_typelib())
        return FALSE;

    hres = IActiveScript_SetScriptSite(script, &script_site);
    if(FAILED(hres))
        return FALSE;

    hres = IActiveScriptParse_InitNew(parser);
    if(FAILED(hres))
        return FALSE;

    hres = IActiveScript_AddNamedItem(script, L"WScript", SCRIPTITEM_ISVISIBLE);
    if(FAILED(hres))
        return FALSE;

    hres = IActiveScript_AddNamedItem(script, L"WSH", SCRIPTITEM_ISVISIBLE);
    if(FAILED(hres))
        return FALSE;

    hres = IActiveScript_SetScriptState(script, SCRIPTSTATE_INITIALIZED);
    return SUCCEEDED(hres);
}

static BSTR get_script_str(const WCHAR *filename)
{
    const char *file_map;
    HANDLE file, map;
    DWORD size, len;
    BSTR ret;

    file = CreateFileW(filename, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_READONLY, NULL);
    if(file == INVALID_HANDLE_VALUE)
        return NULL;

    size = GetFileSize(file, NULL);
    if(!size) {
        CloseHandle(file);
        return SysAllocStringLen(NULL, 0);
    }

    map = CreateFileMappingW(file, NULL, PAGE_READONLY, 0, 0, NULL);
    CloseHandle(file);
    if(map == INVALID_HANDLE_VALUE)
        return NULL;

    file_map = MapViewOfFile(map, FILE_MAP_READ, 0, 0, 0);
    CloseHandle(map);
    if(!file_map)
        return NULL;

#ifdef __REACTOS__
    if(size >= 2 && (BYTE)file_map[0] == 0xFF && (BYTE)file_map[1] == 0xFE) // UTF-16LE
    {
        ret = SysAllocStringLen(NULL, size - 2);
        if (ret)
            CopyMemory(ret, file_map + 2, size - 2);
        UnmapViewOfFile(file_map);
        return ret;
    }
#endif

    len = MultiByteToWideChar(CP_ACP, 0, file_map, size, NULL, 0);
    ret = SysAllocStringLen(NULL, len);
    MultiByteToWideChar(CP_ACP, 0, file_map, size, ret, len);

    UnmapViewOfFile(file_map);
    return ret;
}

static BOOL run_script(BSTR text, IActiveScript *script, IActiveScriptParse *parser)
{
    HRESULT hres;

    hres = IActiveScriptParse_ParseScriptText(parser, text, NULL, NULL, NULL, 1, 0,
            SCRIPTTEXT_HOSTMANAGESSOURCE|SCRIPTITEM_ISVISIBLE, NULL, NULL);
    if(FAILED(hres)) {
        if(hres != SCRIPT_E_REPORTED)
            WINE_WARN("ParseScriptText failed: %08lx\n", hres);
        return FALSE;
    }

    hres = IActiveScript_SetScriptState(script, SCRIPTSTATE_STARTED);
    if(FAILED(hres)) {
        if(hres != SCRIPT_E_REPORTED)
            WINE_WARN("SetScriptState failed: %08lx\n", hres);
    }

    return TRUE;
}

#ifdef __REACTOS__
#include <msxml2.h>
#include <shlwapi.h>

static HRESULT xmldomnode_getattributevalue(IXMLDOMNode *pnode, LPCWSTR name, BSTR *pout)
{
    IXMLDOMNamedNodeMap *pmap;
    HRESULT hr = E_OUTOFMEMORY;
    BSTR bsname = SysAllocString(name);
    *pout = NULL;
    if (bsname && SUCCEEDED(hr = IXMLDOMNode_get_attributes(pnode, &pmap)))
    {
        if (SUCCEEDED(hr = IXMLDOMNamedNodeMap_getNamedItem(pmap, bsname, &pnode)))
        {
            hr = HRESULT_FROM_WIN32(ERROR_NOT_FOUND);
            if (pnode)
            {
                hr = IXMLDOMNode_get_text(pnode, pout);
                if (SUCCEEDED(hr) && !*pout)
                    hr = HRESULT_FROM_WIN32(ERROR_NOT_FOUND);
                IXMLDOMNode_Release(pnode);
            }
        }
        IXMLDOMNamedNodeMap_Release(pmap);
    }
    SysFreeString(bsname);
    return hr;
}

static HRESULT xmldomelem_getelembytag(IXMLDOMElement *pelem, LPCWSTR name, long index, IXMLDOMNode**ppout)
{
    HRESULT hr = E_OUTOFMEMORY;
    IXMLDOMNodeList *pnl;
    BSTR bsname = SysAllocString(name);
    *ppout = NULL;
    if (bsname && SUCCEEDED(hr = IXMLDOMElement_getElementsByTagName(pelem, bsname, &pnl)))
    {
        hr = IXMLDOMNodeList_get_item(pnl, index, ppout);
        if (SUCCEEDED(hr) && !*ppout)
            hr = HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS);
        IUnknown_Release(pnl);
    }
    SysFreeString(bsname);
    return hr;
}

static HRESULT xmldomelem_getelembytagasdomelem(IXMLDOMElement *pelem, LPCWSTR name, long index, IXMLDOMElement**ppout)
{
    IXMLDOMNode *pnode;
    HRESULT hr = xmldomelem_getelembytag(pelem, name, index, &pnode);
    *ppout = NULL;
    if (SUCCEEDED(hr))
    {
        hr = IUnknown_QueryInterface(pnode, &IID_IXMLDOMElement, (void**)ppout);
        IUnknown_Release(pnode);
    }
    return hr;
}

static void wsf_addobjectfromnode(IActiveScript *script, IXMLDOMNode *obj)
{
    BSTR bsid, bsclsid = NULL;
    if (SUCCEEDED(xmldomnode_getattributevalue(obj, L"id", &bsid)))
    {
        CLSID clsid;
        HRESULT hr;
        hr = xmldomnode_getattributevalue(obj, L"clsid", &bsclsid);
        if (FAILED(hr) || FAILED(CLSIDFromString(bsclsid, &clsid)))
        {
            SysFreeString(bsclsid);
            if (SUCCEEDED(hr = xmldomnode_getattributevalue(obj, L"progid", &bsclsid)))
            {
                hr = CLSIDFromProgID(bsclsid, &clsid);
                SysFreeString(bsclsid);
            }
        }
        if (SUCCEEDED(hr))
        {
            hr = add_globalitem_from_clsid(script, bsid, &clsid, SCRIPTITEM_ISVISIBLE);
        }
        SysFreeString(bsid);
    }
}

static HRESULT run_wsfjob(IXMLDOMElement *jobtag)
{
    // FIXME: We are supposed to somehow handle multiple languages in the same IActiveScript.
    IActiveScript *script;
    LPCWSTR deflang = L"JScript";
    IXMLDOMNode *scripttag;
    HRESULT hr = S_OK;
    if (SUCCEEDED(xmldomelem_getelembytag(jobtag, L"script", 0, &scripttag)))
    {
        CLSID clsid;
        IActiveScriptParse *parser;
        BSTR lang, code;
        if (FAILED(xmldomnode_getattributevalue(scripttag, L"language", &lang)))
            lang = NULL;
        hr = CLSIDFromProgID(lang ? lang : deflang, &clsid);
        SysFreeString(lang);

        if (SUCCEEDED(hr))
        {
            hr = E_FAIL;
            if (create_engine(&clsid, &script, &parser))
            {
                if (init_engine(script, parser))
                {
                    long index;
                    for (index = 0; index < 0x7fffffff; ++index)
                    {
                        IXMLDOMNode *obj;
                        if (SUCCEEDED(xmldomelem_getelembytag(jobtag, L"object", index, &obj)))
                        {
                            wsf_addobjectfromnode(script, obj);
                            IUnknown_Release(obj);
                        }
                        else
                        {
                            break;
                        }
                    }

                    if (SUCCEEDED(hr = IXMLDOMNode_get_text(scripttag, &code)))
                    {
                        hr = IActiveScriptParse_ParseScriptText(parser, code, NULL, NULL, NULL, 1, 1,
                                                                SCRIPTTEXT_HOSTMANAGESSOURCE|SCRIPTITEM_ISVISIBLE,
                                                                NULL, NULL);
                        if (SUCCEEDED(hr))
                        {
                            hr = IActiveScript_SetScriptState(script, SCRIPTSTATE_STARTED);
                            IActiveScript_Close(script);
                        }
                        SysFreeString(code);
                    }
                    ITypeInfo_Release(host_ti);
                }
                IUnknown_Release(parser);
                IUnknown_Release(script);
            }
        }
        IUnknown_Release(scripttag);
    }
    return hr;
}

/*
.WSF files can contain a single job, or multiple jobs if contained in a package.
Jobs are identified by their id and if no id is specified, the first job is used.
Each job can contain multiple script tags and all scripts are merged into one.

<job><script language="JScript">WScript.Echo("JS");</script></job>
or
<package>
<job><script language="JScript">WScript.Echo("JS");</script></job>
</package>
or
<?xml version="1.0" ?>
<job>
<script language="JScript"><![CDATA[function JS(s) {WScript.Echo(s)}]]></script>
<script language="VBScript">JS "VB2JS"</script>
</job>
*/
static HRESULT run_wsf(LPCWSTR xmlpath)
{
    WCHAR url[ARRAY_SIZE("file://") + max(ARRAY_SIZE(scriptFullName), MAX_PATH)];
    DWORD cch = ARRAY_SIZE(url);
    IXMLDOMDocument *pdoc;
    HRESULT hr = UrlCreateFromPathW(xmlpath, url, &cch, 0), hrCom;
    if (FAILED(hr))
        return hr;

    hrCom = CoInitialize(NULL);
    hr = CoCreateInstance(&CLSID_DOMDocument30, NULL, CLSCTX_INPROC_SERVER,
                          &IID_IXMLDOMDocument, (void**)&pdoc);
    if (SUCCEEDED(hr))
    {
        VARIANT_BOOL succ = VARIANT_FALSE;
        IXMLDOMElement *pdocelm;
        BSTR bsurl = SysAllocString(url);
        VARIANT v;
        V_VT(&v) = VT_BSTR;
        V_BSTR(&v) = bsurl;
        if (!bsurl || (hr = IXMLDOMDocument_load(pdoc, v, &succ)) > 0 || (SUCCEEDED(hr) && !succ))
        {
            hr = E_FAIL;
        }
        if (SUCCEEDED(hr) && SUCCEEDED(hr = IXMLDOMDocument_get_documentElement(pdoc, &pdocelm)))
        {
            BSTR tagName = NULL;
            if (SUCCEEDED(hr = IXMLDOMElement_get_tagName(pdocelm, &tagName)))
            {
                if (lstrcmpiW(tagName, L"package") == 0)
                {
                    // FIXME: Accept job id as a function parameter and find the job here
                    IXMLDOMElement *p;
                    if (SUCCEEDED(hr = xmldomelem_getelembytagasdomelem(pdocelm, L"job", 0, &p)))
                    {
                        IUnknown_Release(pdocelm);
                        pdocelm = p;
                    }
                }
                else if (lstrcmpiW(tagName, L"job") != 0)
                {
                    hr = 0x800400C0ul;
                }
                SysFreeString(tagName);
            }
            if (SUCCEEDED(hr))
            {
                // FIXME: Only support CDATA blocks if the xml tag is present?
                hr = run_wsfjob(pdocelm);
            }
            IUnknown_Release(pdocelm);
        }
        VariantClear(&v);
        IUnknown_Release(pdoc);
    }
    free_globals();
    if (SUCCEEDED(hrCom))
        CoUninitialize();
    return hr;
}
#endif

static BOOL set_host_properties(const WCHAR *prop)
{
    if(*prop == '/') {
        ++prop;
        if(*prop == '/')
            ++prop;
    }
    else
        ++prop;

    if(wcsicmp(prop, L"i") == 0)
        wshInteractive = VARIANT_TRUE;
    else if(wcsicmp(prop, L"b") == 0)
        wshInteractive = VARIANT_FALSE;
    else if(wcsicmp(prop, L"nologo") == 0)
        nologo = TRUE;
    else if(wcsicmp(prop, L"logo") == 0)
        nologo = FALSE;
    else if(wcsicmp(prop, L"d") == 0)
        WINE_FIXME("ignoring //d\n");
    else if(wcsicmp(prop, L"x") == 0)
        WINE_FIXME("ignoring //x\n");
    else if(wcsicmp(prop, L"u") == 0)
        WINE_FIXME("ignoring //u\n");
    else if(wcsicmp(prop, L"s") == 0)
        WINE_FIXME("ignoring //s\n");
    else if(wcsnicmp(prop, L"e:", 2) == 0)
#ifdef __REACTOS__
        g_force_engine = &prop[2];
#else
        WINE_FIXME("ignoring //e:\n");
#endif
    else if(wcsnicmp(prop, L"h:", 2) == 0)
        WINE_FIXME("ignoring //h:\n");
    else if(wcsnicmp(prop, L"job:", 4) == 0)
        WINE_FIXME("ignoring //job:\n");
    else if(wcsnicmp(prop, L"t:", 2) == 0) {
        WCHAR *end;
        LONG t = wcstol(prop + 2, &end, 10);
        if(end == prop + 2 || *end || t < 0)
            return FALSE;
        wshTimeout = t;
    }
    else
        return FALSE;
    return TRUE;
}

int WINAPI wWinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPWSTR cmdline, int cmdshow)
{
    WCHAR *ext, *filepart, *filename = NULL;
    IActiveScriptParse *parser;
    IActiveScript *script;
    BSTR script_text;
    WCHAR **argv;
    CLSID clsid;
    int argc, i;
    int ret = 0;
    DWORD res;

    WINE_TRACE("(%p %p %s %x)\n", hInst, hPrevInst, wine_dbgstr_w(cmdline), cmdshow);

    argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    if(!argv)
        return 1;

    /* Pass 1: consume // host options from all positions. */
    for(i=1; i<argc; i++) {
        if(argv[i][0] == '/' && argv[i][1] == '/' && set_host_properties(argv[i]))
            argv[i] = NULL;
    }

    /* Pass 2: find filename, consuming single-/ and - options before it. */
    for(i=1; i<argc; i++) {
        if(!argv[i])
            continue;
        if(argv[i][0] == '-' && argv[i][1] == '-' && !argv[i][2])
            continue; /* -- separator */
        if((*argv[i] == '/' || *argv[i] == '-') && set_host_properties(argv[i]))
            continue;
        filename = argv[i];
        i++;
        break;
    }

    /* Pass 3: compact script args, skipping consumed // options. */
    if(filename) {
        int j = 0, first_arg = i;
        for(; i<argc; i++) {
            if(!argv[i])
                continue;
            argv[first_arg + j] = argv[i];
            j++;
        }
        argums = argv + first_arg;
        numOfArgs = j;
    }

    if(!filename && argc == 1) {
        if(wshInteractive) {
            WINE_FIXME("Settings dialog not implemented\n");
            return 0;
        }
        if(!nologo)
            print_banner();
        print_resource(IDS_USAGE);
        return 0;
    }

    if(!nologo)
        print_banner();

    if(!filename) {
        print_resource(IDS_NO_SCRIPT_FILE);
        return 1;
    }

    res = GetFullPathNameW(filename, ARRAY_SIZE(scriptFullName), scriptFullName, &filepart);
    if(!res || res > ARRAY_SIZE(scriptFullName))
        return 1;

#ifdef __REACTOS__
    ext = wcsrchr(filepart, '.');
    if(ext && !lstrcmpiW(ext, L".wsf")) {
        return run_wsf(scriptFullName);
    }
#endif

    script_text = get_script_str(scriptFullName);
    if(!script_text) {
        DWORD err = GetLastError();
        if(err == ERROR_FILE_NOT_FOUND || err == ERROR_PATH_NOT_FOUND) {
            print_resource(IDS_FILE_NOT_FOUND, scriptFullName);
        }else {
            WCHAR *syserr = NULL;
            FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                           NULL, err, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (WCHAR *)&syserr, 0, NULL);
            if(syserr) {
                /* Trim trailing whitespace from system error message. */
                int len = lstrlenW(syserr);
                while(len > 0 && (syserr[len - 1] == '\r' || syserr[len - 1] == '\n' || syserr[len - 1] == ' '))
                    syserr[--len] = 0;
                print_resource(IDS_SCRIPT_LOAD_ERROR, scriptFullName, syserr);
                LocalFree(syserr);
            }else {
                print_resource(IDS_SCRIPT_LOAD_ERROR, scriptFullName, L"");
            }
        }
        return 1;
    }

    ext = wcsrchr(filepart, '.');
#ifdef __REACTOS__
    if(g_force_engine) {
        CLSIDFromProgID(g_force_engine, &clsid);
    }
    else
#endif
    if(!ext || !get_engine_clsid(ext, &clsid)) {
        WINE_FIXME("Could not find engine for %s\n", wine_dbgstr_w(ext));
        return 1;
    }

    CoInitialize(NULL);

    if(!create_engine(&clsid, &script, &parser)) {
        WINE_FIXME("Could not create script engine\n");
        CoUninitialize();
        return 1;
    }

    if(init_engine(script, parser)) {
        schedule_timeout(wshTimeout);
        if(!run_script(script_text, script, parser))
            ret = 1;
        schedule_timeout(0);
        IActiveScript_Close(script);
        ITypeInfo_Release(host_ti);
    }else {
        WINE_FIXME("Script initialization failed\n");
    }

    SysFreeString(script_text);
    IActiveScript_Release(script);
    IActiveScriptParse_Release(parser);

#ifdef __REACTOS__
    free_globals();
#endif

    CoUninitialize();

    return ret;
}
