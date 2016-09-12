/*
 * Copyright 2005 Jacek Caban
 * Copyright 2011 Thomas Mullaly for CodeWeavers
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

static const WCHAR feature_control_keyW[] =
    {'S','o','f','t','w','a','r','e','\\',
     'M','i','c','r','o','s','o','f','t','\\',
     'I','n','t','e','r','n','e','t',' ','E','x','p','l','o','r','e','r','\\',
     'M','a','i','n','\\',
     'F','e','a','t','u','r','e','C','o','n','t','r','o','l',0};

static const WCHAR feature_object_cachingW[] =
    {'F','E','A','T','U','R','E','_','O','B','J','E','C','T','_','C','A','C','H','I','N','G',0};
static const WCHAR feature_zone_elevationW[] =
    {'F','E','A','T','U','R','E','_','Z','O','N','E','_','E','L','E','V','A','T','I','O','N',0};
static const WCHAR feature_mime_handlingW[] =
    {'F','E','A','T','U','R','E','_','M','I','M','E','_','H','A','N','D','L','I','N','G',0};
static const WCHAR feature_mime_sniffingW[] =
    {'F','E','A','T','U','R','E','_','M','I','M','E','_','S','N','I','F','F','I','N','G',0};
static const WCHAR feature_window_restrictionsW[] =
    {'F','E','A','T','U','R','E','_','W','I','N','D','O','W','_','R','E','S','T','R','I','C','T','I','O','N','S',0};
static const WCHAR feature_weboc_popupmanagementW[] =
    {'F','E','A','T','U','R','E','_','W','E','B','O','C','_','P','O','P','U','P','M','A','N','A','G','E','M','E','N','T',0};
static const WCHAR feature_behaviorsW[] =
    {'F','E','A','T','U','R','E','_','B','E','H','A','V','I','O','R','S',0};
static const WCHAR feature_disable_mk_protocolW[] =
    {'F','E','A','T','U','R','E','_','D','I','S','A','B','L','E','_','M','K','_','P','R','O','T','O','C','O','L',0};
static const WCHAR feature_localmachine_lockdownW[] =
    {'F','E','A','T','U','R','E','_','L','O','C','A','L','M','A','C','H','I','N','E','_','L','O','C','K','D','O','W','N',0};
static const WCHAR feature_securitybandW[] =
    {'F','E','A','T','U','R','E','_','S','E','C','U','R','I','T','Y','B','A','N','D',0};
static const WCHAR feature_restrict_activexinstallW[] =
    {'F','E','A','T','U','R','E','_','R','E','S','T','R','I','C','T','_','A','C','T','I','V','E','X','I','N','S','T','A','L','L',0};
static const WCHAR feature_validate_navigate_urlW[] =
    {'F','E','A','T','U','R','E','_','V','A','L','I','D','A','T','E','_','N','A','V','I','G','A','T','E','_','U','R','L',0};
static const WCHAR feature_restrict_filedownloadW[] =
    {'F','E','A','T','U','R','E','_','R','E','S','T','R','I','C','T','_','F','I','L','E','D','O','W','N','L','O','A','D',0};
static const WCHAR feature_addon_managementW[] =
    {'F','E','A','T','U','R','E','_','A','D','D','O','N','_','M','A','N','A','G','E','M','E','N','T',0};
static const WCHAR feature_protocol_lockdownW[] =
    {'F','E','A','T','U','R','E','_','P','R','O','T','O','C','O','L','_','L','O','C','K','D','O','W','N',0};
static const WCHAR feature_http_username_password_disableW[] =
    {'F','E','A','T','U','R','E','_','H','T','T','P','_','U','S','E','R','N','A','M','E','_',
     'P','A','S','S','W','O','R','D','_','D','I','S','A','B','L','E',0};
static const WCHAR feature_safe_bindtoobjectW[] =
    {'F','E','A','T','U','R','E','_','S','A','F','E','_','B','I','N','D','T','O','O','B','J','E','C','T',0};
static const WCHAR feature_unc_savedfilecheckW[] =
    {'F','E','A','T','U','R','E','_','U','N','C','_','S','A','V','E','D','F','I','L','E','C','H','E','C','K',0};
static const WCHAR feature_get_url_dom_filepath_unencodedW[] =
    {'F','E','A','T','U','R','E','_','G','E','T','_','U','R','L','_','D','O','M','_',
     'F','I','L','E','P','A','T','H','_','U','N','E','N','C','O','D','E','D',0};
static const WCHAR feature_tabbed_browsingW[] =
    {'F','E','A','T','U','R','E','_','T','A','B','B','E','D','_','B','R','O','W','S','I','N','G',0};
static const WCHAR feature_ssluxW[] =
    {'F','E','A','T','U','R','E','_','S','S','L','U','X',0};
static const WCHAR feature_disable_navigation_soundsW[] =
    {'F','E','A','T','U','R','E','_','D','I','S','A','B','L','E','_','N','A','V','I','G','A','T','I','O','N','_',
     'S','O','U','N','D','S',0};
static const WCHAR feature_disable_legacy_compressionW[] =
    {'F','E','A','T','U','R','E','_','D','I','S','A','B','L','E','_','L','E','G','A','C','Y','_',
     'C','O','M','P','R','E','S','S','I','O','N',0};
static const WCHAR feature_force_addr_and_statusW[] =
    {'F','E','A','T','U','R','E','_','F','O','R','C','E','_','A','D','D','R','_','A','N','D','_',
     'S','T','A','T','U','S',0};
static const WCHAR feature_xmlhttpW[] =
    {'F','E','A','T','U','R','E','_','X','M','L','H','T','T','P',0};
static const WCHAR feature_disable_telnet_protocolW[] =
    {'F','E','A','T','U','R','E','_','D','I','S','A','B','L','E','_','T','E','L','N','E','T','_',
     'P','R','O','T','O','C','O','L',0};
static const WCHAR feature_feedsW[] =
    {'F','E','A','T','U','R','E','_','F','E','E','D','S',0};
static const WCHAR feature_block_input_promptsW[] =
    {'F','E','A','T','U','R','E','_','B','L','O','C','K','_','I','N','P','U','T','_','P','R','O','M','P','T','S',0};

static CRITICAL_SECTION process_features_cs;
static CRITICAL_SECTION_DEBUG process_features_cs_dbg =
{
    0, 0, &process_features_cs,
    { &process_features_cs_dbg.ProcessLocksList, &process_features_cs_dbg.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": process features") }
};
static CRITICAL_SECTION process_features_cs = { &process_features_cs_dbg, -1, 0, 0, 0, 0 };

typedef struct feature_control {
    LPCWSTR feature_name;
    BOOL    enabled;
    BOOL    check_registry;
} feature_control;

/* IMPORTANT!!!
 *
 * This array is indexed using INTERNETFEATURELIST values, so everything must
 * appear in the same order as it does in INTERNETFEATURELIST.
 */
static feature_control process_feature_controls[FEATURE_ENTRY_COUNT] = {
    {feature_object_cachingW,                   TRUE ,TRUE},
    {feature_zone_elevationW,                   FALSE,TRUE},
    {feature_mime_handlingW,                    FALSE,TRUE},
    {feature_mime_sniffingW,                    FALSE,TRUE},
    {feature_window_restrictionsW,              FALSE,TRUE},
    {feature_weboc_popupmanagementW,            FALSE,TRUE},
    {feature_behaviorsW,                        TRUE ,TRUE},
    {feature_disable_mk_protocolW,              TRUE ,TRUE},
    {feature_localmachine_lockdownW,            FALSE,TRUE},
    {feature_securitybandW,                     FALSE,TRUE},
    {feature_restrict_activexinstallW,          FALSE,TRUE},
    {feature_validate_navigate_urlW,            FALSE,TRUE},
    {feature_restrict_filedownloadW,            FALSE,TRUE},
    {feature_addon_managementW,                 FALSE,TRUE},
    {feature_protocol_lockdownW,                FALSE,TRUE},
    {feature_http_username_password_disableW,   FALSE,TRUE},
    {feature_safe_bindtoobjectW,                FALSE,TRUE},
    {feature_unc_savedfilecheckW,               FALSE,TRUE},
    {feature_get_url_dom_filepath_unencodedW,   TRUE ,TRUE},
    {feature_tabbed_browsingW,                  FALSE,TRUE},
    {feature_ssluxW,                            FALSE,TRUE},
    {feature_disable_navigation_soundsW,        FALSE,TRUE},
    {feature_disable_legacy_compressionW,       TRUE ,TRUE},
    {feature_force_addr_and_statusW,            FALSE,TRUE},
    {feature_xmlhttpW,                          TRUE ,TRUE},
    {feature_disable_telnet_protocolW,          FALSE,TRUE},
    {feature_feedsW,                            FALSE,TRUE},
    {feature_block_input_promptsW,              FALSE,TRUE}
};

static HRESULT parse_schema(LPCWSTR url, DWORD flags, LPWSTR result, DWORD size, DWORD *rsize)
{
    WCHAR *ptr;
    DWORD len = 0;

    TRACE("(%s %08x %p %d %p)\n", debugstr_w(url), flags, result, size, rsize);

    if(flags)
        ERR("wrong flags\n");
    
    ptr = strchrW(url, ':');
    if(ptr)
        len = ptr-url;

    if(rsize)
        *rsize = len;

    if(len >= size)
        return E_POINTER;

    if(len)
        memcpy(result, url, len*sizeof(WCHAR));
    result[len] = 0;

    return S_OK;
}

static HRESULT parse_canonicalize_url(LPCWSTR url, DWORD flags, LPWSTR result,
        DWORD size, DWORD *rsize)
{
    IInternetProtocolInfo *protocol_info;
    DWORD prsize = size;
    HRESULT hres;

    TRACE("(%s %08x %p %d %p)\n", debugstr_w(url), flags, result, size, rsize);

    protocol_info = get_protocol_info(url);

    if(protocol_info) {
        hres = IInternetProtocolInfo_ParseUrl(protocol_info, url, PARSE_CANONICALIZE,
                flags, result, size, rsize, 0);
        IInternetProtocolInfo_Release(protocol_info);
        if(SUCCEEDED(hres))
            return hres;
    }

    hres = UrlCanonicalizeW(url, result, &prsize, flags);

    if(rsize)
        *rsize = prsize;
    return hres;
}

static HRESULT parse_security_url(LPCWSTR url, DWORD flags, LPWSTR result, DWORD size, DWORD *rsize)
{
    IInternetProtocolInfo *protocol_info;
    HRESULT hres;

    TRACE("(%s %08x %p %d %p)\n", debugstr_w(url), flags, result, size, rsize);

    protocol_info = get_protocol_info(url);

    if(protocol_info) {
        hres = IInternetProtocolInfo_ParseUrl(protocol_info, url, PARSE_SECURITY_URL,
                flags, result, size, rsize, 0);
        IInternetProtocolInfo_Release(protocol_info);
        return hres;
    }

    return E_FAIL;
}

static HRESULT parse_encode(LPCWSTR url, DWORD flags, LPWSTR result, DWORD size, DWORD *rsize)
{
    IInternetProtocolInfo *protocol_info;
    DWORD prsize;
    HRESULT hres;

    TRACE("(%s %08x %p %d %p)\n", debugstr_w(url), flags, result, size, rsize);

    protocol_info = get_protocol_info(url);

    if(protocol_info) {
        hres = IInternetProtocolInfo_ParseUrl(protocol_info, url, PARSE_ENCODE,
                flags, result, size, rsize, 0);
        IInternetProtocolInfo_Release(protocol_info);
        if(SUCCEEDED(hres))
            return hres;
    }

    prsize = size;
    hres = UrlUnescapeW((LPWSTR)url, result, &prsize, flags);

    if(rsize)
        *rsize = prsize;

    return hres;
}

static HRESULT parse_path_from_url(LPCWSTR url, DWORD flags, LPWSTR result, DWORD size, DWORD *rsize)
{
    IInternetProtocolInfo *protocol_info;
    DWORD prsize;
    HRESULT hres;

    TRACE("(%s %08x %p %d %p)\n", debugstr_w(url), flags, result, size, rsize);

    protocol_info = get_protocol_info(url);

    if(protocol_info) {
        hres = IInternetProtocolInfo_ParseUrl(protocol_info, url, PARSE_PATH_FROM_URL,
                flags, result, size, rsize, 0);
        IInternetProtocolInfo_Release(protocol_info);
        if(SUCCEEDED(hres))
            return hres;
    }

    prsize = size;
    hres = PathCreateFromUrlW(url, result, &prsize, 0);

    if(rsize)
        *rsize = prsize;
    return hres;
}

static HRESULT parse_security_domain(LPCWSTR url, DWORD flags, LPWSTR result,
        DWORD size, DWORD *rsize)
{
    IInternetProtocolInfo *protocol_info;
    HRESULT hres;

    TRACE("(%s %08x %p %d %p)\n", debugstr_w(url), flags, result, size, rsize);

    protocol_info = get_protocol_info(url);

    if(protocol_info) {
        hres = IInternetProtocolInfo_ParseUrl(protocol_info, url, PARSE_SECURITY_DOMAIN,
                flags, result, size, rsize, 0);
        IInternetProtocolInfo_Release(protocol_info);
        if(SUCCEEDED(hres))
            return hres;
    }

    return E_FAIL;
}

static HRESULT parse_domain(LPCWSTR url, DWORD flags, LPWSTR result,
        DWORD size, DWORD *rsize)
{
    IInternetProtocolInfo *protocol_info;
    HRESULT hres;

    TRACE("(%s %08x %p %d %p)\n", debugstr_w(url), flags, result, size, rsize);

    protocol_info = get_protocol_info(url);

    if(protocol_info) {
        hres = IInternetProtocolInfo_ParseUrl(protocol_info, url, PARSE_DOMAIN,
                flags, result, size, rsize, 0);
        IInternetProtocolInfo_Release(protocol_info);
        if(SUCCEEDED(hres))
            return hres;
    }

    hres = UrlGetPartW(url, result, &size, URL_PART_HOSTNAME, flags);
    if(rsize)
        *rsize = size;

    if(hres == E_POINTER)
        return S_FALSE;

    if(FAILED(hres))
        return E_FAIL;
    return S_OK;
}

static HRESULT parse_rootdocument(LPCWSTR url, DWORD flags, LPWSTR result,
        DWORD size, DWORD *rsize)
{
    IInternetProtocolInfo *protocol_info;
    PARSEDURLW url_info;
    HRESULT hres;

    TRACE("(%s %08x %p %d %p)\n", debugstr_w(url), flags, result, size, rsize);

    protocol_info = get_protocol_info(url);

    if(protocol_info) {
        hres = IInternetProtocolInfo_ParseUrl(protocol_info, url, PARSE_ROOTDOCUMENT,
                flags, result, size, rsize, 0);
        IInternetProtocolInfo_Release(protocol_info);
        if(SUCCEEDED(hres))
            return hres;
    }

    url_info.cbSize = sizeof(url_info);
    if(FAILED(ParseURLW(url, &url_info)))
        return E_FAIL;

    switch(url_info.nScheme) {
        case URL_SCHEME_FTP:
        case URL_SCHEME_HTTP:
        case URL_SCHEME_HTTPS:
            if(url_info.cchSuffix<3 || *(url_info.pszSuffix)!='/'
                    || *(url_info.pszSuffix+1)!='/')
                return E_FAIL;

            if(size < url_info.cchProtocol+3) {
                size = 0;
                hres = UrlGetPartW(url, result, &size, URL_PART_HOSTNAME, flags);

                if(rsize)
                    *rsize = size+url_info.cchProtocol+3;

                if(hres == E_POINTER)
                    return S_FALSE;

                return hres;
            }

            size -= url_info.cchProtocol+3;
            hres = UrlGetPartW(url, result+url_info.cchProtocol+3,
                    &size, URL_PART_HOSTNAME, flags);

            if(hres == E_POINTER)
                return S_FALSE;

            if(FAILED(hres))
                return E_FAIL;

            if(rsize)
                *rsize = size+url_info.cchProtocol+3;

            memcpy(result, url, (url_info.cchProtocol+3)*sizeof(WCHAR));
            return hres;
        default:
            return E_FAIL;
    }
}

/**************************************************************************
 *          CoInternetParseUrl    (URLMON.@)
 */
HRESULT WINAPI CoInternetParseUrl(LPCWSTR pwzUrl, PARSEACTION ParseAction, DWORD dwFlags,
        LPWSTR pszResult, DWORD cchResult, DWORD *pcchResult, DWORD dwReserved)
{
    if(dwReserved)
        WARN("dwReserved = %d\n", dwReserved);

    switch(ParseAction) {
    case PARSE_CANONICALIZE:
        return parse_canonicalize_url(pwzUrl, dwFlags, pszResult, cchResult, pcchResult);
    case PARSE_SECURITY_URL:
        return parse_security_url(pwzUrl, dwFlags, pszResult, cchResult, pcchResult);
    case PARSE_ENCODE:
        return parse_encode(pwzUrl, dwFlags, pszResult, cchResult, pcchResult);
    case PARSE_PATH_FROM_URL:
        return parse_path_from_url(pwzUrl, dwFlags, pszResult, cchResult, pcchResult);
    case PARSE_SCHEMA:
        return parse_schema(pwzUrl, dwFlags, pszResult, cchResult, pcchResult);
    case PARSE_SECURITY_DOMAIN:
        return parse_security_domain(pwzUrl, dwFlags, pszResult, cchResult, pcchResult);
    case PARSE_DOMAIN:
        return parse_domain(pwzUrl, dwFlags, pszResult, cchResult, pcchResult);
    case PARSE_ROOTDOCUMENT:
        return parse_rootdocument(pwzUrl, dwFlags, pszResult, cchResult, pcchResult);
    default:
        FIXME("not supported action %d\n", ParseAction);
    }

    return E_NOTIMPL;
}

/**************************************************************************
 *          CoInternetCombineUrl    (URLMON.@)
 */
HRESULT WINAPI CoInternetCombineUrl(LPCWSTR pwzBaseUrl, LPCWSTR pwzRelativeUrl,
        DWORD dwCombineFlags, LPWSTR pwzResult, DWORD cchResult, DWORD *pcchResult,
        DWORD dwReserved)
{
    IInternetProtocolInfo *protocol_info;
    DWORD size = cchResult;
    HRESULT hres;
    
    TRACE("(%s,%s,0x%08x,%p,%d,%p,%d)\n", debugstr_w(pwzBaseUrl),
          debugstr_w(pwzRelativeUrl), dwCombineFlags, pwzResult, cchResult, pcchResult,
          dwReserved);

    protocol_info = get_protocol_info(pwzBaseUrl);

    if(protocol_info) {
        hres = IInternetProtocolInfo_CombineUrl(protocol_info, pwzBaseUrl, pwzRelativeUrl,
                dwCombineFlags, pwzResult, cchResult, pcchResult, dwReserved);
        IInternetProtocolInfo_Release(protocol_info);
        if(SUCCEEDED(hres))
            return hres;
    }


    hres = UrlCombineW(pwzBaseUrl, pwzRelativeUrl, pwzResult, &size, dwCombineFlags);

    if(pcchResult)
        *pcchResult = size;

    return hres;
}

/**************************************************************************
 *          CoInternetCompareUrl    (URLMON.@)
 */
HRESULT WINAPI CoInternetCompareUrl(LPCWSTR pwzUrl1, LPCWSTR pwzUrl2, DWORD dwCompareFlags)
{
    IInternetProtocolInfo *protocol_info;
    HRESULT hres;

    TRACE("(%s,%s,%08x)\n", debugstr_w(pwzUrl1), debugstr_w(pwzUrl2), dwCompareFlags);

    protocol_info = get_protocol_info(pwzUrl1);

    if(protocol_info) {
        hres = IInternetProtocolInfo_CompareUrl(protocol_info, pwzUrl1, pwzUrl2, dwCompareFlags);
        IInternetProtocolInfo_Release(protocol_info);
        if(SUCCEEDED(hres))
            return hres;
    }

    return UrlCompareW(pwzUrl1, pwzUrl2, dwCompareFlags) ? S_FALSE : S_OK;
}

/***********************************************************************
 *           CoInternetQueryInfo (URLMON.@)
 *
 * Retrieves information relevant to a specified URL
 *
 */
HRESULT WINAPI CoInternetQueryInfo(LPCWSTR pwzUrl, QUERYOPTION QueryOption,
        DWORD dwQueryFlags, LPVOID pvBuffer, DWORD cbBuffer, DWORD *pcbBuffer,
        DWORD dwReserved)
{
    IInternetProtocolInfo *protocol_info;
    HRESULT hres;

    TRACE("(%s, %x, %x, %p, %x, %p, %x): stub\n", debugstr_w(pwzUrl),
          QueryOption, dwQueryFlags, pvBuffer, cbBuffer, pcbBuffer, dwReserved);

    protocol_info = get_protocol_info(pwzUrl);

    if(protocol_info) {
        hres = IInternetProtocolInfo_QueryInfo(protocol_info, pwzUrl, QueryOption, dwQueryFlags,
                pvBuffer, cbBuffer, pcbBuffer, dwReserved);
        IInternetProtocolInfo_Release(protocol_info);

        return SUCCEEDED(hres) ? hres : E_FAIL;
    }

    switch(QueryOption) {
    case QUERY_USES_NETWORK:
        if(!pvBuffer || cbBuffer < sizeof(DWORD))
            return E_FAIL;

        *(DWORD*)pvBuffer = 0;
        if(pcbBuffer)
            *pcbBuffer = sizeof(DWORD);
        break;

    default:
        FIXME("Not supported option %d\n", QueryOption);
        return E_NOTIMPL;
    }

    return S_OK;
}

static void set_feature_on_process(INTERNETFEATURELIST feature, BOOL enable)
{
    EnterCriticalSection(&process_features_cs);

    process_feature_controls[feature].enabled = enable;
    process_feature_controls[feature].check_registry = FALSE;

    LeaveCriticalSection(&process_features_cs);
}

static HRESULT set_internet_feature(INTERNETFEATURELIST feature, DWORD flags, BOOL enable)
{
    const DWORD supported_flags = SET_FEATURE_ON_PROCESS;

    if(feature >= FEATURE_ENTRY_COUNT)
        return E_FAIL;

    if(flags & ~supported_flags)
        FIXME("Unsupported flags: %08x\n", flags & ~supported_flags);

    if(flags & SET_FEATURE_ON_PROCESS)
        set_feature_on_process(feature, enable);

    return S_OK;
}

static BOOL get_feature_from_reg(HKEY feature_control, LPCWSTR feature_name, LPCWSTR process_name, BOOL *enabled)
{
    DWORD type, value, size;
    HKEY feature;
    DWORD res;

    static const WCHAR wildcardW[] = {'*',0};

    res = RegOpenKeyW(feature_control, feature_name, &feature);
    if(res != ERROR_SUCCESS)
        return FALSE;

    size = sizeof(DWORD);
    res = RegQueryValueExW(feature, process_name, NULL, &type, (BYTE*)&value, &size);
    if(res != ERROR_SUCCESS || type != REG_DWORD) {
        size = sizeof(DWORD);
        res = RegQueryValueExW(feature, wildcardW, NULL, &type, (BYTE*)&value, &size);
    }

    RegCloseKey(feature);
    if(res != ERROR_SUCCESS)
        return FALSE;

    if(type != REG_DWORD) {
        WARN("Unexpected registry value type %d (expected REG_DWORD) for %s\n", type, debugstr_w(wildcardW));
        return FALSE;
    }

    *enabled = value == 1;
    return TRUE;
}

/* Assumes 'process_features_cs' is held. */
static HRESULT load_process_feature(INTERNETFEATURELIST feature)
{
    DWORD res;
    HKEY feature_control;
    WCHAR module_name[MAX_PATH];
    LPCWSTR process_name, feature_name;
    HRESULT hres = S_FALSE;
    BOOL check_hklm = FALSE;
    BOOL enabled;

    if (!GetModuleFileNameW(NULL, module_name, sizeof(module_name)/sizeof(WCHAR))) {
        ERR("Failed to get module file name: %u\n", GetLastError());
        return E_UNEXPECTED;
    }

    process_name = strrchrW(module_name, '\\');
    if(!process_name) {
        ERR("Invalid module file name: %s\n", debugstr_w(module_name));
        return E_UNEXPECTED;
    }

    /* Skip past the '\\' in front of the filename. */
    ++process_name;

    feature_name = process_feature_controls[feature].feature_name;

    res = RegOpenKeyW(HKEY_CURRENT_USER, feature_control_keyW, &feature_control);
    if(res == ERROR_SUCCESS) {
        if(get_feature_from_reg(feature_control, feature_name, process_name, &enabled)) {
            hres = enabled ? S_OK : S_FALSE;
            process_feature_controls[feature].enabled = enabled;
        } else
            /* We didn't find anything in HKCU, so check HKLM. */
            check_hklm = TRUE;

        RegCloseKey(feature_control);
    }

    if(check_hklm) {
        res = RegOpenKeyW(HKEY_LOCAL_MACHINE, feature_control_keyW, &feature_control);
        if(res == ERROR_SUCCESS) {
            if(get_feature_from_reg(feature_control, feature_name, process_name, &enabled)) {
                hres = enabled ? S_OK : S_FALSE;
                process_feature_controls[feature].enabled = enabled;
            }
            RegCloseKey(feature_control);
        }
    }

    /* Don't bother checking the registry again for this feature. */
    process_feature_controls[feature].check_registry = FALSE;

    return hres;
}

static HRESULT get_feature_from_process(INTERNETFEATURELIST feature)
{
    HRESULT hres = S_OK;

    EnterCriticalSection(&process_features_cs);

    /* Try loading the feature from the registry, if it hasn't already
     * been done.
     */
    if(process_feature_controls[feature].check_registry)
        hres = load_process_feature(feature);
    if(SUCCEEDED(hres))
        hres = process_feature_controls[feature].enabled ? S_OK : S_FALSE;

    LeaveCriticalSection(&process_features_cs);

    return hres;
}

static HRESULT get_internet_feature(INTERNETFEATURELIST feature, DWORD flags)
{
    HRESULT hres;

    if(feature >= FEATURE_ENTRY_COUNT)
        return E_FAIL;

    if(flags == GET_FEATURE_FROM_PROCESS)
        hres = get_feature_from_process(feature);
    else {
        FIXME("Unsupported flags: %08x\n", flags);
        hres = E_NOTIMPL;
    }

    return hres;
}

/***********************************************************************
 *             CoInternetSetFeatureEnabled (URLMON.@)
 */
HRESULT WINAPI CoInternetSetFeatureEnabled(INTERNETFEATURELIST FeatureEntry, DWORD dwFlags, BOOL fEnable)
{
    TRACE("(%d, %08x, %x)\n", FeatureEntry, dwFlags, fEnable);
    return set_internet_feature(FeatureEntry, dwFlags, fEnable);
}

/***********************************************************************
 *             CoInternetIsFeatureEnabled (URLMON.@)
 */
HRESULT WINAPI CoInternetIsFeatureEnabled(INTERNETFEATURELIST FeatureEntry, DWORD dwFlags)
{
    TRACE("(%d, %08x)\n", FeatureEntry, dwFlags);
    return get_internet_feature(FeatureEntry, dwFlags);
}

/***********************************************************************
 *             CoInternetIsFeatureEnabledForUrl (URLMON.@)
 */
HRESULT WINAPI CoInternetIsFeatureEnabledForUrl(INTERNETFEATURELIST FeatureEntry, DWORD dwFlags, LPCWSTR szURL,
        IInternetSecurityManager *pSecMgr)
{
    DWORD urlaction = 0;
    HRESULT hres;

    TRACE("(%d %08x %s %p)\n", FeatureEntry, dwFlags, debugstr_w(szURL), pSecMgr);

    if(FeatureEntry == FEATURE_MIME_SNIFFING)
        urlaction = URLACTION_FEATURE_MIME_SNIFFING;
    else if(FeatureEntry == FEATURE_WINDOW_RESTRICTIONS)
        urlaction = URLACTION_FEATURE_WINDOW_RESTRICTIONS;
    else if(FeatureEntry == FEATURE_ZONE_ELEVATION)
        urlaction = URLACTION_FEATURE_ZONE_ELEVATION;

    if(!szURL || !urlaction || !pSecMgr)
        return CoInternetIsFeatureEnabled(FeatureEntry, dwFlags);

    switch(dwFlags) {
    case GET_FEATURE_FROM_THREAD:
    case GET_FEATURE_FROM_THREAD_LOCALMACHINE:
    case GET_FEATURE_FROM_THREAD_INTRANET:
    case GET_FEATURE_FROM_THREAD_TRUSTED:
    case GET_FEATURE_FROM_THREAD_INTERNET:
    case GET_FEATURE_FROM_THREAD_RESTRICTED:
        FIXME("unsupported flags %x\n", dwFlags);
        return E_NOTIMPL;

    case GET_FEATURE_FROM_PROCESS:
        hres = CoInternetIsFeatureEnabled(FeatureEntry, dwFlags);
        if(hres != S_OK)
            return hres;
        /* fall through */

    default: {
        DWORD policy = URLPOLICY_DISALLOW;

        hres = IInternetSecurityManager_ProcessUrlAction(pSecMgr, szURL, urlaction,
                (BYTE*)&policy, sizeof(DWORD), NULL, 0, PUAF_NOUI, 0);
        if(hres!=S_OK || policy!=URLPOLICY_ALLOW)
            return S_OK;
        return S_FALSE;
    }
    }
}

/***********************************************************************
 *             CoInternetIsFeatureZoneElevationEnabled (URLMON.@)
 */
HRESULT WINAPI CoInternetIsFeatureZoneElevationEnabled(LPCWSTR szFromURL, LPCWSTR szToURL,
        IInternetSecurityManager *pSecMgr, DWORD dwFlags)
{
    HRESULT hres;

    TRACE("(%s %s %p %x)\n", debugstr_w(szFromURL), debugstr_w(szToURL), pSecMgr, dwFlags);

    if(!pSecMgr || !szToURL)
        return CoInternetIsFeatureEnabled(FEATURE_ZONE_ELEVATION, dwFlags);

    switch(dwFlags) {
    case GET_FEATURE_FROM_THREAD:
    case GET_FEATURE_FROM_THREAD_LOCALMACHINE:
    case GET_FEATURE_FROM_THREAD_INTRANET:
    case GET_FEATURE_FROM_THREAD_TRUSTED:
    case GET_FEATURE_FROM_THREAD_INTERNET:
    case GET_FEATURE_FROM_THREAD_RESTRICTED:
        FIXME("unsupported flags %x\n", dwFlags);
        return E_NOTIMPL;

    case GET_FEATURE_FROM_PROCESS:
        hres = CoInternetIsFeatureEnabled(FEATURE_ZONE_ELEVATION, dwFlags);
        if(hres != S_OK)
            return hres;
        /* fall through */

    default: {
        DWORD policy = URLPOLICY_DISALLOW;

        hres = IInternetSecurityManager_ProcessUrlAction(pSecMgr, szToURL,
                URLACTION_FEATURE_ZONE_ELEVATION, (BYTE*)&policy, sizeof(DWORD),
                NULL, 0, PUAF_NOUI, 0);
        if(FAILED(hres))
            return S_OK;

        switch(policy) {
        case URLPOLICY_ALLOW:
            return S_FALSE;
        case URLPOLICY_QUERY:
            FIXME("Ask user dialog not implemented\n");
        default:
            return S_OK;
        }
    }
    }
}
