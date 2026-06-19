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
#include "winreg.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(urlmon);

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

static name_space *find_name_space(LPCWSTR protocol)
{
    name_space *iter;

    LIST_FOR_EACH_ENTRY(iter, &name_space_list, name_space, entry) {
        if(!wcsicmp(iter->protocol, protocol))
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

    wszKey = malloc(sizeof(wszProtocolsKey) + (schema_len + 1) * sizeof(WCHAR));
    memcpy(wszKey, wszProtocolsKey, sizeof(wszProtocolsKey));
    memcpy(wszKey + ARRAY_SIZE(wszProtocolsKey), schema, (schema_len+1)*sizeof(WCHAR));

    res = RegOpenKeyW(HKEY_CLASSES_ROOT, wszKey, &hkey);
    free(wszKey);
    if(res != ERROR_SUCCESS) {
        TRACE("Could not open protocol handler key\n");
        return MK_E_SYNTAX;
    }
    
    size = sizeof(str_clsid);
    res = RegQueryValueExW(hkey, L"CLSID", NULL, &type, (BYTE*)str_clsid, &size);
    RegCloseKey(hkey);
    if(res != ERROR_SUCCESS || type != REG_SZ) {
        WARN("Could not get protocol CLSID res=%ld\n", res);
        return MK_E_SYNTAX;
    }

    hres = CLSIDFromString(str_clsid, &clsid);
    if(FAILED(hres)) {
        WARN("CLSIDFromString failed: %08lx\n", hres);
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

    new_name_space = malloc(sizeof(name_space));

    if(!urlmon_protocol)
        IClassFactory_AddRef(cf);
    new_name_space->cf = cf;
    new_name_space->clsid = *clsid;
    new_name_space->urlmon = urlmon_protocol;
    new_name_space->protocol = wcsdup(protocol);

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
        if(iter->cf == cf && !wcsicmp(iter->protocol, protocol)) {
            list_remove(&iter->entry);

            LeaveCriticalSection(&session_cs);

            if(!iter->urlmon)
                IClassFactory_Release(iter->cf);
            free(iter->protocol);
            free(iter);
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

    hres = CoInternetParseUrl(url, PARSE_SCHEMA, 0, schema, ARRAY_SIZE(schema), &schema_len, 0);
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

    hres = CoInternetParseUrl(url, PARSE_SCHEMA, 0, schema, ARRAY_SIZE(schema), &schema_len, 0);
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

HRESULT get_protocol_handler(IUri *uri, CLSID *clsid, IClassFactory **ret)
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
    }

    LeaveCriticalSection(&session_cs);

    hres = *ret ? S_OK : get_protocol_cf(scheme, SysStringLen(scheme), clsid, ret);
    SysFreeString(scheme);
    return hres;
}

IInternetProtocol *get_mime_filter(LPCWSTR mime)
{
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
        if(!wcscmp(iter->mime, mime)) {
            cf = iter->cf;
            break;
        }
    }

    LeaveCriticalSection(&session_cs);

    if(cf) {
        hres = IClassFactory_CreateInstance(cf, NULL, &IID_IInternetProtocol, (void**)&ret);
        if(FAILED(hres)) {
            WARN("CreateInstance failed: %08lx\n", hres);
            return NULL;
        }

        return ret;
    }

    res = RegOpenKeyW(HKEY_CLASSES_ROOT, L"Protocols\\Filter", &hlist);
    if(res != ERROR_SUCCESS) {
        TRACE("Could not open MIME filters key\n");
        return NULL;
    }

    res = RegOpenKeyW(hlist, mime, &hfilter);
    CloseHandle(hlist);
    if(res != ERROR_SUCCESS)
        return NULL;

    size = sizeof(clsidw);
    res = RegQueryValueExW(hfilter, L"CLSID", NULL, &type, (BYTE*)clsidw, &size);
    CloseHandle(hfilter);
    if(res!=ERROR_SUCCESS || type!=REG_SZ) {
        WARN("Could not get filter CLSID for %s\n", debugstr_w(mime));
        return NULL;
    }

    hres = CLSIDFromString(clsidw, &clsid);
    if(FAILED(hres)) {
        WARN("CLSIDFromString failed for %s (%lx)\n", debugstr_w(mime), hres);
        return NULL;
    }

    hres = CoCreateInstance(&clsid, NULL, CLSCTX_INPROC_SERVER, &IID_IInternetProtocol, (void**)&ret);
    if(FAILED(hres)) {
        WARN("CoCreateInstance failed: %08lx\n", hres);
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
    TRACE("(%p %s %s %ld %p %ld)\n", pCF, debugstr_guid(rclsid), debugstr_w(pwzProtocol),
          cPatterns, ppwzPatterns, dwReserved);

    if(cPatterns || ppwzPatterns)
        FIXME("patterns not supported\n");
    if(dwReserved)
        WARN("dwReserved = %ld\n", dwReserved);

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

    filter = malloc(sizeof(mime_filter));

    IClassFactory_AddRef(pCF);
    filter->cf = pCF;
    filter->clsid = *rclsid;
    filter->mime = wcsdup(pwzType);

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
        if(iter->cf == pCF && !wcscmp(iter->mime, pwzType)) {
            list_remove(&iter->entry);

            LeaveCriticalSection(&session_cs);

            IClassFactory_Release(iter->cf);
            free(iter->mime);
            free(iter);
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

    TRACE("(%p %s %p %p %p %08lx)\n", pBC, debugstr_w(szUrl), pUnkOuter, ppUnk,
            ppOInetProt, dwOption);

    if(pBC || pUnkOuter || ppUnk || dwOption)
        FIXME("Unsupported arguments\n");

    hres = create_binding_protocol(&protocol);
    if(FAILED(hres))
        return hres;

    *ppOInetProt = (IInternetProtocol*)&protocol->IInternetProtocolEx_iface;
    return S_OK;
}

static HRESULT WINAPI InternetSession_SetSessionOption(IInternetSession *iface,
        DWORD dwOption, LPVOID pBuffer, DWORD dwBufferLength, DWORD dwReserved)
{
    FIXME("(%08lx %p %ld %ld)\n", dwOption, pBuffer, dwBufferLength, dwReserved);
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
    TRACE("(%ld %p %ld)\n", dwSessionMode, ppIInternetSession, dwReserved);

    if(dwSessionMode)
        ERR("dwSessionMode=%ld\n", dwSessionMode);
    if(dwReserved)
        ERR("dwReserved=%ld\n", dwReserved);

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

    res = RegOpenKeyW(root, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Internet Settings", &hkey);
    if(res != ERROR_SUCCESS)
        return FALSE;

    res = RegQueryValueExW(hkey, L"UrlEncoding", NULL, &type, (BYTE*)encoding, &size);
    RegCloseKey(hkey);

    return res == ERROR_SUCCESS;
}

static LPWSTR user_agent;
static BOOL user_agent_set;

static size_t obtain_user_agent(unsigned int version, WCHAR *ret, size_t size)
{
    BOOL is_wow, quirks = FALSE, use_current = FALSE;
    OSVERSIONINFOW info = {sizeof(info)};
    const WCHAR *os_type, *is_nt;
    DWORD res;
    size_t len = 0;
    HKEY key;

    if(version & UAS_EXACTLEGACY) {
        version &= ~UAS_EXACTLEGACY;
        if(version == 7)
            quirks = TRUE;
        else {
            use_current = TRUE;
            version = 7;
        }
    }

    if(version > 11) {
        FIXME("Unsupported version %u\n", version);
        version = 11;
    }

    if(version < 7 || use_current) {
        EnterCriticalSection(&session_cs);
        if(user_agent) {
            len = wcslen(user_agent) + 1;
            memcpy(ret, user_agent, min(size, len) * sizeof(WCHAR));
        }
        LeaveCriticalSection(&session_cs);
        if(len) return len;
    }

    if(version < 7)
        version = 7;

    swprintf(ret, size, L"Mozilla/%s (", version < 9 ? L"4.0" : L"5.0");
    len = lstrlenW(ret);
    if(version < 11) {
        swprintf(ret + len, size - len, L"compatible; MSIE %u.0; ", version);
        len += wcslen(ret + len);
    }

    GetVersionExW(&info);
    is_nt = info.dwPlatformId == VER_PLATFORM_WIN32_NT ? L"NT " : L"";

    if(sizeof(void*) == 8)
#ifdef __x86_64__
        os_type = L"; Win64; x64";
#else
        os_type = L"; Win64";
#endif
    else if(IsWow64Process(GetCurrentProcess(), &is_wow) && is_wow)
        os_type = L"; WOW64";
    else
        os_type = L"";

    swprintf(ret + len, size - len, L"Windows %s%d.%d%s", is_nt, info.dwMajorVersion,
             info.dwMinorVersion, os_type);
    len = lstrlenW(ret);

    if(!quirks) {
        wcscpy(ret + len, L"; Trident/7.0");
        len += ARRAY_SIZE(L"; Trident/7.0") - 1;
    }

    if(version < 9) {
        res = RegOpenKeyW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\"
                          "Internet Settings\\5.0\\User Agent\\Post Platform", &key);
        if(res == ERROR_SUCCESS) {
            DWORD value_len, idx;

            for(idx = 0;; idx++) {
                ret[len++] = ';';
                ret[len++] = ' ';

                value_len = size - len - 2;
                res = RegEnumValueW(key, idx, ret + len, &value_len, NULL, NULL, NULL, NULL);
                if(res != ERROR_SUCCESS)
                    break;

                len += value_len;
            }

            RegCloseKey(key);
            if(idx) len -= 2;
        }
    }
    wcscpy(ret + len, version >= 11 ? L"; rv:11.0) like Gecko" : L")");
    len += wcslen(ret + len) + 1;

    TRACE("Using user agent %s\n", debugstr_w(ret));
    return len;
}

static void ensure_user_agent(void)
{
    EnterCriticalSection(&session_cs);

    if(!user_agent) {
        WCHAR buf[1024];
        obtain_user_agent(0, buf, ARRAY_SIZE(buf));
        user_agent = wcsdup(buf);
    }

    LeaveCriticalSection(&session_cs);
}

LPWSTR get_useragent(void)
{
    LPWSTR ret;

    ensure_user_agent();

    EnterCriticalSection(&session_cs);
    ret = wcsdup(user_agent);
    LeaveCriticalSection(&session_cs);

    return ret;
}

HRESULT WINAPI UrlMkGetSessionOption(DWORD dwOption, LPVOID pBuffer, DWORD dwBufferLength,
                                     DWORD* pdwBufferLength, DWORD dwReserved)
{
    TRACE("(%lx, %p, %ld, %p)\n", dwOption, pBuffer, dwBufferLength, pdwBufferLength);

    if(dwReserved)
        WARN("dwReserved = %ld\n", dwReserved);

    switch(dwOption) {
    case URLMON_OPTION_USERAGENT: {
        HRESULT hres = E_OUTOFMEMORY;
        DWORD size;

        if(!pdwBufferLength)
            return E_INVALIDARG;

        EnterCriticalSection(&session_cs);

        ensure_user_agent();
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
        FIXME("unsupported option %lx\n", dwOption);
    }

    return E_INVALIDARG;
}

/**************************************************************************
 *                 UrlMkSetSessionOption (URLMON.@)
 */
HRESULT WINAPI UrlMkSetSessionOption(DWORD dwOption, LPVOID pBuffer, DWORD dwBufferLength,
        DWORD Reserved)
{
    TRACE("(%lx %p %lx)\n", dwOption, pBuffer, dwBufferLength);

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
        new_user_agent = malloc((size + 1) * sizeof(WCHAR));
        if(!new_user_agent)
            return E_OUTOFMEMORY;
        MultiByteToWideChar(CP_ACP, 0, buf, len, new_user_agent, size);
        new_user_agent[size] = 0;

        EnterCriticalSection(&session_cs);

        free(user_agent);
        user_agent = new_user_agent;
        user_agent_set = TRUE;
        update_user_agent(user_agent);

        LeaveCriticalSection(&session_cs);
        break;
    }
    default:
        FIXME("Unknown option %lx\n", dwOption);
        return E_INVALIDARG;
    }

    return S_OK;
}

/**************************************************************************
 *                 ObtainUserAgentString (URLMON.@)
 */
HRESULT WINAPI ObtainUserAgentString(DWORD option, char *ret, DWORD *ret_size)
{
    DWORD size, len;
    WCHAR buf[1024];
    HRESULT hres = S_OK;

    TRACE("(%ld %p %p)\n", option, ret, ret_size);

    if(!ret || !ret_size)
        return E_INVALIDARG;

    len = obtain_user_agent(option, buf, ARRAY_SIZE(buf));
    size = WideCharToMultiByte(CP_ACP, 0, buf, len, NULL, 0, NULL, NULL);
    if(size <= *ret_size)
        WideCharToMultiByte(CP_ACP, 0, buf, len, ret, *ret_size+1, NULL, NULL);
    else
        hres = E_OUTOFMEMORY;

    *ret_size = size;
    return hres;
}

/***********************************************************************
 *                 MapBrowserEmulationModeToUserAgent (URLMON.445)
 *    Undocumented, added in IE8
 */
HRESULT WINAPI MapBrowserEmulationModeToUserAgent(const void *arg, WCHAR **ret)
{
    DWORD size, version;
    const WCHAR *ua;
    WCHAR buf[1024];

    TRACE("%p %p: semi-stub\n", arg, ret);

    if(user_agent_set) {
        /* Native ignores first arg if custom user agent has been set, doesn't crash even if NULL */
        size = (wcslen(user_agent) + 1) * sizeof(WCHAR);
        ua = user_agent;
    }else {
        *ret = NULL;

        /* First arg seems to be a pointer to a structure of unknown size, and crashes
           if it's too small (or filled with arbitrary values from the stack). For our
           purposes, we only check first field which seems to be the requested version. */
        version = *(DWORD*)arg;
        if(version == 5)
            version = 7;
        if(version < 7 || version > 11)
            return E_FAIL;

        size = obtain_user_agent(version, buf, ARRAY_SIZE(buf)) * sizeof(WCHAR);
        ua = buf;
    }

    if(!(*ret = CoTaskMemAlloc(size)))
        return E_OUTOFMEMORY;
    memcpy(*ret, ua, size);
    return S_OK;
}

void free_session(void)
{
    name_space *ns_iter, *ns_last;
    mime_filter *mf_iter, *mf_last;

    LIST_FOR_EACH_ENTRY_SAFE(ns_iter, ns_last, &name_space_list, name_space, entry) {
            if(!ns_iter->urlmon)
                IClassFactory_Release(ns_iter->cf);
            free(ns_iter->protocol);
            free(ns_iter);
    }

    LIST_FOR_EACH_ENTRY_SAFE(mf_iter, mf_last, &mime_filter_list, mime_filter, entry) {
            IClassFactory_Release(mf_iter->cf);
            free(mf_iter->mime);
            free(mf_iter);
    }

    free(user_agent);
}
