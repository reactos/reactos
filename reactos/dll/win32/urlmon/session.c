/*
 * Copyright 2005-2006 Jacek Caban for CodeWeavers
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

#include "urlmon_main.h"

typedef struct {
    LPWSTR protocol;
    IClassFactory *cf;
    CLSID clsid;
    BOOL urlmon;

    struct list entry;
} name_space;

typedef struct {
    IClassFactory *cf;
    CLSID clsid;
    LPWSTR mime;

    struct list entry;
} mime_filter;

static struct list name_space_list = LIST_INIT(name_space_list);
static struct list mime_filter_list = LIST_INIT(mime_filter_list);

static CRITICAL_SECTION session_cs;
static CRITICAL_SECTION_DEBUG session_cs_dbg =
{
    0, 0, &session_cs,
    { &session_cs_dbg.ProcessLocksList, &session_cs_dbg.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": session") }
};
static CRITICAL_SECTION session_cs = { &session_cs_dbg, -1, 0, 0, 0, 0 };

static const WCHAR internet_settings_keyW[] =
    {'S','O','F','T','W','A','R','E',
     '\\','M','i','c','r','o','s','o','f','t',
     '\\','W','i','n','d','o','w','s',
     '\\','C','u','r','r','e','n','t','V','e','r','s','i','o','n',
     '\\','I','n','t','e','r','n','e','t',' ','S','e','t','t','i','n','g','s',0};

static name_space *find_name_space(LPCWSTR protocol)
{
    name_space *iter;

    LIST_FOR_EACH_ENTRY(iter, &name_space_list, name_space, entry) {
        if(!strcmpiW(iter->protocol, protocol))
            return iter;
    }

    return NULL;
}

static HRESULT get_protocol_cf(LPCWSTR schema, DWORD schema_len, CLSID *pclsid, IClassFactory **ret)
{
    WCHAR str_clsid[64];
    HKEY hkey = NULL;
    DWORD res, type, size;
    CLSID clsid;
    LPWSTR wszKey;
    HRESULT hres;

    static const WCHAR wszProtocolsKey[] =
        {'P','R','O','T','O','C','O','L','S','\\','H','a','n','d','l','e','r','\\'};
    static const WCHAR wszCLSID[] = {'C','L','S','I','D',0};

    wszKey = heap_alloc(sizeof(wszProtocolsKey)+(schema_len+1)*sizeof(WCHAR));
    memcpy(wszKey, wszProtocolsKey, sizeof(wszProtocolsKey));
    memcpy(wszKey + sizeof(wszProtocolsKey)/sizeof(WCHAR), schema, (schema_len+1)*sizeof(WCHAR));

    res = RegOpenKeyW(HKEY_CLASSES_ROOT, wszKey, &hkey);
    heap_free(wszKey);
    if(res != ERROR_SUCCESS) {
        TRACE("Could not open protocol handler key\n");
        return MK_E_SYNTAX;
    }
    
    size = sizeof(str_clsid);
    res = RegQueryValueExW(hkey, wszCLSID, NULL, &type, (LPBYTE)str_clsid, &size);
    RegCloseKey(hkey);
    if(res != ERROR_SUCCESS || type != REG_SZ) {
        WARN("Could not get protocol CLSID res=%d\n", res);
        return MK_E_SYNTAX;
    }

    hres = CLSIDFromString(str_clsid, &clsid);
    if(FAILED(hres)) {
        WARN("CLSIDFromString failed: %08x\n", hres);
        return hres;
    }

    if(pclsid)
        *pclsid = clsid;

    if(!ret)
        return S_OK;

    hres = CoGetClassObject(&clsid, CLSCTX_INPROC_SERVER, NULL, &IID_IClassFactory, (void**)ret);
    return SUCCEEDED(hres) ? S_OK : MK_E_SYNTAX;
}

HRESULT register_namespace(IClassFactory *cf, REFIID clsid, LPCWSTR protocol, BOOL urlmon_protocol)
{
    name_space *new_name_space;

    new_name_space = heap_alloc(sizeof(name_space));

    if(!urlmon_protocol)
        IClassFactory_AddRef(cf);
    new_name_space->cf = cf;
    new_name_space->clsid = *clsid;
    new_name_space->urlmon = urlmon_protocol;
    new_name_space->protocol = heap_strdupW(protocol);

    EnterCriticalSection(&session_cs);

    list_add_head(&name_space_list, &new_name_space->entry);

    LeaveCriticalSection(&session_cs);

    return S_OK;
}

static HRESULT unregister_namespace(IClassFactory *cf, LPCWSTR protocol)
{
    name_space *iter;

    EnterCriticalSection(&session_cs);

    LIST_FOR_EACH_ENTRY(iter, &name_space_list, name_space, entry) {
        if(iter->cf == cf && !strcmpiW(iter->protocol, protocol)) {
            list_remove(&iter->entry);

            LeaveCriticalSection(&session_cs);

            if(!iter->urlmon)
                IClassFactory_Release(iter->cf);
            heap_free(iter->protocol);
            heap_free(iter);
            return S_OK;
        }
    }

    LeaveCriticalSection(&session_cs);
    return S_OK;
}

BOOL is_registered_protocol(LPCWSTR url)
{
    DWORD schema_len;
    WCHAR schema[64];
    HRESULT hres;

    hres = CoInternetParseUrl(url, PARSE_SCHEMA, 0, schema, sizeof(schema)/sizeof(schema[0]),
            &schema_len, 0);
    if(FAILED(hres))
        return FALSE;

    return get_protocol_cf(schema, schema_len, NULL, NULL) == S_OK;
}

IInternetProtocolInfo *get_protocol_info(LPCWSTR url)
{
    IInternetProtocolInfo *ret = NULL;
    IClassFactory *cf;
    name_space *ns;
    WCHAR schema[64];
    DWORD schema_len;
    HRESULT hres;

    hres = CoInternetParseUrl(url, PARSE_SCHEMA, 0, schema, sizeof(schema)/sizeof(schema[0]),
            &schema_len, 0);
    if(FAILED(hres) || !schema_len)
        return NULL;

    EnterCriticalSection(&session_cs);

    ns = find_name_space(schema);
    if(ns && !ns->urlmon) {
        hres = IClassFactory_QueryInterface(ns->cf, &IID_IInternetProtocolInfo, (void**)&ret);
        if(FAILED(hres))
            hres = IClassFactory_CreateInstance(ns->cf, NULL, &IID_IInternetProtocolInfo, (void**)&ret);
    }

    LeaveCriticalSection(&session_cs);

    if(ns && SUCCEEDED(hres))
        return ret;

    hres = get_protocol_cf(schema, schema_len, NULL, &cf);
    if(FAILED(hres))
        return NULL;

    hres = IClassFactory_QueryInterface(cf, &IID_IInternetProtocolInfo, (void**)&ret);
    if(FAILED(hres))
        IClassFactory_CreateInstance(cf, NULL, &IID_IInternetProtocolInfo, (void**)&ret);
    IClassFactory_Release(cf);

    return ret;
}

HRESULT get_protocol_handler(IUri *uri, CLSID *clsid, BOOL *urlmon_protocol, IClassFactory **ret)
{
    name_space *ns;
    BSTR scheme;
    HRESULT hres;

    *ret = NULL;

    /* FIXME: Avoid GetSchemeName call for known schemes */
    hres = IUri_GetSchemeName(uri, &scheme);
    if(FAILED(hres))
        return hres;

    EnterCriticalSection(&session_cs);

    ns = find_name_space(scheme);
    if(ns) {
        *ret = ns->cf;
        IClassFactory_AddRef(*ret);
        if(clsid)
            *clsid = ns->clsid;
        if(urlmon_protocol)
            *urlmon_protocol = ns->urlmon;
    }

    LeaveCriticalSection(&session_cs);

    if(*ret) {
        hres = S_OK;
    }else {
        if(urlmon_protocol)
            *urlmon_protocol = FALSE;
        hres = get_protocol_cf(scheme, SysStringLen(scheme), clsid, ret);
    }

    SysFreeString(scheme);
    return hres;
}

IInternetProtocol *get_mime_filter(LPCWSTR mime)
{
    static const WCHAR filtersW[] = {'P','r','o','t','o','c','o','l','s',
        '\\','F','i','l','t','e','r',0 };
    static const WCHAR CLSIDW[] = {'C','L','S','I','D',0};

    IClassFactory *cf = NULL;
    IInternetProtocol *ret;
    mime_filter *iter;
    HKEY hlist, hfilter;
    WCHAR clsidw[64];
    CLSID clsid;
    DWORD res, type, size;
    HRESULT hres;

    EnterCriticalSection(&session_cs);

    LIST_FOR_EACH_ENTRY(iter, &mime_filter_list, mime_filter, entry) {
        if(!strcmpW(iter->mime, mime)) {
            cf = iter->cf;
            break;
        }
    }

    LeaveCriticalSection(&session_cs);

    if(cf) {
        hres = IClassFactory_CreateInstance(cf, NULL, &IID_IInternetProtocol, (void**)&ret);
        if(FAILED(hres)) {
            WARN("CreateInstance failed: %08x\n", hres);
            return NULL;
        }

        return ret;
    }

    res = RegOpenKeyW(HKEY_CLASSES_ROOT, filtersW, &hlist);
    if(res != ERROR_SUCCESS) {
        TRACE("Could not open MIME filters key\n");
        return NULL;
    }

    res = RegOpenKeyW(hlist, mime, &hfilter);
    CloseHandle(hlist);
    if(res != ERROR_SUCCESS)
        return NULL;

    size = sizeof(clsidw);
    res = RegQueryValueExW(hfilter, CLSIDW, NULL, &type, (LPBYTE)clsidw, &size);
    CloseHandle(hfilter);
    if(res!=ERROR_SUCCESS || type!=REG_SZ) {
        WARN("Could not get filter CLSID for %s\n", debugstr_w(mime));
        return NULL;
    }

    hres = CLSIDFromString(clsidw, &clsid);
    if(FAILED(hres)) {
        WARN("CLSIDFromString failed for %s (%x)\n", debugstr_w(mime), hres);
        return NULL;
    }

    hres = CoCreateInstance(&clsid, NULL, CLSCTX_INPROC_SERVER, &IID_IInternetProtocol, (void**)&ret);
    if(FAILED(hres)) {
        WARN("CoCreateInstance failed: %08x\n", hres);
        return NULL;
    }

    return ret;
}

static HRESULT WINAPI InternetSession_QueryInterface(IInternetSession *iface,
        REFIID riid, void **ppv)
{
    TRACE("(%s %p)\n", debugstr_guid(riid), ppv);

    if(IsEqualGUID(&IID_IUnknown, riid) || IsEqualGUID(&IID_IInternetSession, riid)) {
        *ppv = iface;
        IInternetSession_AddRef(iface);
        return S_OK;
    }

    *ppv = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI InternetSession_AddRef(IInternetSession *iface)
{
    TRACE("()\n");
    URLMON_LockModule();
    return 2;
}

static ULONG WINAPI InternetSession_Release(IInternetSession *iface)
{
    TRACE("()\n");
    URLMON_UnlockModule();
    return 1;
}

static HRESULT WINAPI InternetSession_RegisterNameSpace(IInternetSession *iface,
        IClassFactory *pCF, REFCLSID rclsid, LPCWSTR pwzProtocol, ULONG cPatterns,
        const LPCWSTR *ppwzPatterns, DWORD dwReserved)
{
    TRACE("(%p %s %s %d %p %d)\n", pCF, debugstr_guid(rclsid), debugstr_w(pwzProtocol),
          cPatterns, ppwzPatterns, dwReserved);

    if(cPatterns || ppwzPatterns)
        FIXME("patterns not supported\n");
    if(dwReserved)
        WARN("dwReserved = %d\n", dwReserved);

    if(!pCF || !pwzProtocol)
        return E_INVALIDARG;

    return register_namespace(pCF, rclsid, pwzProtocol, FALSE);
}

static HRESULT WINAPI InternetSession_UnregisterNameSpace(IInternetSession *iface,
        IClassFactory *pCF, LPCWSTR pszProtocol)
{
    TRACE("(%p %s)\n", pCF, debugstr_w(pszProtocol));

    if(!pCF || !pszProtocol)
        return E_INVALIDARG;

    return unregister_namespace(pCF, pszProtocol);
}

static HRESULT WINAPI InternetSession_RegisterMimeFilter(IInternetSession *iface,
        IClassFactory *pCF, REFCLSID rclsid, LPCWSTR pwzType)
{
    mime_filter *filter;

    TRACE("(%p %s %s)\n", pCF, debugstr_guid(rclsid), debugstr_w(pwzType));

    filter = heap_alloc(sizeof(mime_filter));

    IClassFactory_AddRef(pCF);
    filter->cf = pCF;
    filter->clsid = *rclsid;
    filter->mime = heap_strdupW(pwzType);

    EnterCriticalSection(&session_cs);

    list_add_head(&mime_filter_list, &filter->entry);

    LeaveCriticalSection(&session_cs);

    return S_OK;
}

static HRESULT WINAPI InternetSession_UnregisterMimeFilter(IInternetSession *iface,
        IClassFactory *pCF, LPCWSTR pwzType)
{
    mime_filter *iter;

    TRACE("(%p %s)\n", pCF, debugstr_w(pwzType));

    EnterCriticalSection(&session_cs);

    LIST_FOR_EACH_ENTRY(iter, &mime_filter_list, mime_filter, entry) {
        if(iter->cf == pCF && !strcmpW(iter->mime, pwzType)) {
            list_remove(&iter->entry);

            LeaveCriticalSection(&session_cs);

            IClassFactory_Release(iter->cf);
            heap_free(iter->mime);
            heap_free(iter);
            return S_OK;
        }
    }

    LeaveCriticalSection(&session_cs);
    return S_OK;
}

static HRESULT WINAPI InternetSession_CreateBinding(IInternetSession *iface,
        LPBC pBC, LPCWSTR szUrl, IUnknown *pUnkOuter, IUnknown **ppUnk,
        IInternetProtocol **ppOInetProt, DWORD dwOption)
{
    BindProtocol *protocol;
    HRESULT hres;

    TRACE("(%p %s %p %p %p %08x)\n", pBC, debugstr_w(szUrl), pUnkOuter, ppUnk,
            ppOInetProt, dwOption);

    if(pBC || pUnkOuter || ppUnk || dwOption)
        FIXME("Unsupported arguments\n");

    hres = create_binding_protocol(FALSE, &protocol);
    if(FAILED(hres))
        return hres;

    *ppOInetProt = (IInternetProtocol*)&protocol->IInternetProtocolEx_iface;
    return S_OK;
}

static HRESULT WINAPI InternetSession_SetSessionOption(IInternetSession *iface,
        DWORD dwOption, LPVOID pBuffer, DWORD dwBufferLength, DWORD dwReserved)
{
    FIXME("(%08x %p %d %d)\n", dwOption, pBuffer, dwBufferLength, dwReserved);
    return E_NOTIMPL;
}

static const IInternetSessionVtbl InternetSessionVtbl = {
    InternetSession_QueryInterface,
    InternetSession_AddRef,
    InternetSession_Release,
    InternetSession_RegisterNameSpace,
    InternetSession_UnregisterNameSpace,
    InternetSession_RegisterMimeFilter,
    InternetSession_UnregisterMimeFilter,
    InternetSession_CreateBinding,
    InternetSession_SetSessionOption
};

static IInternetSession InternetSession = { &InternetSessionVtbl };

/***********************************************************************
 *           CoInternetGetSession (URLMON.@)
 *
 * Create a new internet session and return an IInternetSession interface
 * representing it.
 *
 * PARAMS
 *    dwSessionMode      [I] Mode for the internet session
 *    ppIInternetSession [O] Destination for creates IInternetSession object
 *    dwReserved         [I] Reserved, must be 0.
 *
 * RETURNS
 *    Success: S_OK. ppIInternetSession contains the IInternetSession interface.
 *    Failure: E_INVALIDARG, if any argument is invalid, or
 *             E_OUTOFMEMORY if memory allocation fails.
 */
HRESULT WINAPI CoInternetGetSession(DWORD dwSessionMode, IInternetSession **ppIInternetSession,
        DWORD dwReserved)
{
    TRACE("(%d %p %d)\n", dwSessionMode, ppIInternetSession, dwReserved);

    if(dwSessionMode)
        ERR("dwSessionMode=%d\n", dwSessionMode);
    if(dwReserved)
        ERR("dwReserved=%d\n", dwReserved);

    IInternetSession_AddRef(&InternetSession);
    *ppIInternetSession = &InternetSession;
    return S_OK;
}

/**************************************************************************
 *                 UrlMkGetSessionOption (URLMON.@)
 */
static BOOL get_url_encoding(HKEY root, DWORD *encoding)
{
    DWORD size = sizeof(DWORD), res, type;
    HKEY hkey;

    static const WCHAR wszUrlEncoding[] = {'U','r','l','E','n','c','o','d','i','n','g',0};

    res = RegOpenKeyW(root, internet_settings_keyW, &hkey);
    if(res != ERROR_SUCCESS)
        return FALSE;

    res = RegQueryValueExW(hkey, wszUrlEncoding, NULL, &type, (LPBYTE)encoding, &size);
    RegCloseKey(hkey);

    return res == ERROR_SUCCESS;
}

static LPWSTR user_agent;

static void ensure_useragent(void)
{
    OSVERSIONINFOW info = {sizeof(info)};
    const WCHAR *os_type, *is_nt;
    WCHAR buf[512], *ret, *tmp;
    DWORD res, idx=0;
    size_t len, size;
    BOOL is_wow;
    HKEY key;

    static const WCHAR formatW[] =
        {'M','o','z','i','l','l','a','/','4','.','0',
         ' ','(','c','o','m','p','a','t','i','b','l','e',';',
         ' ','M','S','I','E',' ','8','.','0',';',
         ' ','W','i','n','d','o','w','s',' ','%','s','%','d','.','%','d',';',
         ' ','%','s',';',' ','T','r','i','d','e','n','t','/','5','.','0',0};
    static const WCHAR post_platform_keyW[] =
        {'S','O','F','T','W','A','R','E',
         '\\','M','i','c','r','o','s','o','f','t',
         '\\','W','i','n','d','o','w','s',
         '\\','C','u','r','r','e','n','t','V','e','r','s','i','o','n',
         '\\','I','n','t','e','r','n','e','t',' ','S','e','t','t','i','n','g','s',
         '\\','5','.','0','\\','U','s','e','r',' ','A','g','e','n','t',
         '\\','P','o','s','t',' ','P','l','a','t','f','o','r','m',0};
    static const WCHAR ntW[] = {'N','T',' ',0};
    static const WCHAR win32W[] = {'W','i','n','3','2',0};
    static const WCHAR win64W[] = {'W','i','n','6','4',0};
    static const WCHAR wow64W[] = {'W','O','W','6','4',0};
    static const WCHAR emptyW[] = {0};

    if(user_agent)
        return;

    GetVersionExW(&info);
    is_nt = info.dwPlatformId == VER_PLATFORM_WIN32_NT ? ntW : emptyW;

    if(sizeof(void*) == 8)
        os_type = win64W;
    else if(IsWow64Process(GetCurrentProcess(), &is_wow) && is_wow)
        os_type = wow64W;
    else
        os_type = win32W;

    sprintfW(buf, formatW, is_nt, info.dwMajorVersion, info.dwMinorVersion, os_type);
    len = strlenW(buf);

    size = len+40;
    ret = heap_alloc(size * sizeof(WCHAR));
    if(!ret)
        return;

    memcpy(ret, buf, len*sizeof(WCHAR));

    res = RegOpenKeyW(HKEY_LOCAL_MACHINE, post_platform_keyW, &key);
    if(res == ERROR_SUCCESS) {
        DWORD value_len;

        while(1) {
            value_len = sizeof(buf)/sizeof(WCHAR);
            res = RegEnumValueW(key, idx, buf, &value_len, NULL, NULL, NULL, NULL);
            if(res != ERROR_SUCCESS)
                break;
            idx++;

            if(len + value_len + 2 /* strlen("; ") */ + 1 /* trailing ')' */ >= size) {
                tmp = heap_realloc(ret, (size*2+value_len)*sizeof(WCHAR));
                if(!tmp)
                    break;
                ret = tmp;
                size = size*2+value_len;
            }

            ret[len++] = ';';
            ret[len++] = ' ';
            memcpy(ret+len, buf, value_len*sizeof(WCHAR));
            len += value_len;
        }

        RegCloseKey(key);
    }

    ret[len++] = ')';
    ret[len++] = 0;

    user_agent = ret;
    TRACE("Using user agent %s\n", debugstr_w(user_agent));
}

LPWSTR get_useragent(void)
{
    LPWSTR ret;

    ensure_useragent();

    EnterCriticalSection(&session_cs);
    ret = heap_strdupW(user_agent);
    LeaveCriticalSection(&session_cs);

    return ret;
}

HRESULT WINAPI UrlMkGetSessionOption(DWORD dwOption, LPVOID pBuffer, DWORD dwBufferLength,
                                     DWORD* pdwBufferLength, DWORD dwReserved)
{
    TRACE("(%x, %p, %d, %p)\n", dwOption, pBuffer, dwBufferLength, pdwBufferLength);

    if(dwReserved)
        WARN("dwReserved = %d\n", dwReserved);

    switch(dwOption) {
    case URLMON_OPTION_USERAGENT: {
        HRESULT hres = E_OUTOFMEMORY;
        DWORD size;

        if(!pdwBufferLength)
            return E_INVALIDARG;

        EnterCriticalSection(&session_cs);

        ensure_useragent();
        if(user_agent) {
            size = WideCharToMultiByte(CP_ACP, 0, user_agent, -1, NULL, 0, NULL, NULL);
            *pdwBufferLength = size;
            if(size <= dwBufferLength) {
                if(pBuffer)
                    WideCharToMultiByte(CP_ACP, 0, user_agent, -1, pBuffer, size, NULL, NULL);
                else
                    hres = E_INVALIDARG;
            }
        }

        LeaveCriticalSection(&session_cs);

        /* Tests prove that we have to return E_OUTOFMEMORY on success. */
        return hres;
    }
    case URLMON_OPTION_URL_ENCODING: {
        DWORD encoding = 0;

        if(!pBuffer || dwBufferLength < sizeof(DWORD) || !pdwBufferLength)
            return E_INVALIDARG;

        if(!get_url_encoding(HKEY_CURRENT_USER, &encoding))
            get_url_encoding(HKEY_LOCAL_MACHINE, &encoding);

        *pdwBufferLength = sizeof(DWORD);
        *(DWORD*)pBuffer = encoding ? URL_ENCODING_DISABLE_UTF8 : URL_ENCODING_ENABLE_UTF8;
        return S_OK;
    }
    default:
        FIXME("unsupported option %x\n", dwOption);
    }

    return E_INVALIDARG;
}

/**************************************************************************
 *                 UrlMkSetSessionOption (URLMON.@)
 */
HRESULT WINAPI UrlMkSetSessionOption(DWORD dwOption, LPVOID pBuffer, DWORD dwBufferLength,
        DWORD Reserved)
{
    TRACE("(%x %p %x)\n", dwOption, pBuffer, dwBufferLength);

    switch(dwOption) {
    case URLMON_OPTION_USERAGENT: {
        LPWSTR new_user_agent;
        char *buf = pBuffer;
        DWORD len, size;

        if(!pBuffer || !dwBufferLength)
            return E_INVALIDARG;

        for(len=0; len<dwBufferLength && buf[len]; len++);

        TRACE("Setting user agent %s\n", debugstr_an(buf, len));

        size = MultiByteToWideChar(CP_ACP, 0, buf, len, NULL, 0);
        new_user_agent = heap_alloc((size+1)*sizeof(WCHAR));
        if(!new_user_agent)
            return E_OUTOFMEMORY;
        MultiByteToWideChar(CP_ACP, 0, buf, len, new_user_agent, size);
        new_user_agent[size] = 0;

        EnterCriticalSection(&session_cs);

        heap_free(user_agent);
        user_agent = new_user_agent;
        update_user_agent(user_agent);

        LeaveCriticalSection(&session_cs);
        break;
    }
    default:
        FIXME("Unknown option %x\n", dwOption);
        return E_INVALIDARG;
    }

    return S_OK;
}

/**************************************************************************
 *                 ObtainUserAgentString (URLMON.@)
 */
HRESULT WINAPI ObtainUserAgentString(DWORD dwOption, LPSTR pcszUAOut, DWORD *cbSize)
{
    DWORD size;
    HRESULT hres = E_FAIL;

    TRACE("(%d %p %p)\n", dwOption, pcszUAOut, cbSize);

    if(!pcszUAOut || !cbSize)
        return E_INVALIDARG;

    EnterCriticalSection(&session_cs);

    ensure_useragent();
    if(user_agent) {
        size = WideCharToMultiByte(CP_ACP, 0, user_agent, -1, NULL, 0, NULL, NULL);

        if(size <= *cbSize) {
            WideCharToMultiByte(CP_ACP, 0, user_agent, -1, pcszUAOut, *cbSize, NULL, NULL);
            hres = S_OK;
        }else {
            hres = E_OUTOFMEMORY;
        }

        *cbSize = size;
    }

    LeaveCriticalSection(&session_cs);
    return hres;
}

void free_session(void)
{
    name_space *ns_iter, *ns_last;
    mime_filter *mf_iter, *mf_last;

    LIST_FOR_EACH_ENTRY_SAFE(ns_iter, ns_last, &name_space_list, name_space, entry) {
            if(!ns_iter->urlmon)
                IClassFactory_Release(ns_iter->cf);
            heap_free(ns_iter->protocol);
            heap_free(ns_iter);
    }

    LIST_FOR_EACH_ENTRY_SAFE(mf_iter, mf_last, &mime_filter_list, mime_filter, entry) {
            IClassFactory_Release(mf_iter->cf);
            heap_free(mf_iter->mime);
            heap_free(mf_iter);
    }

    heap_free(user_agent);
}
