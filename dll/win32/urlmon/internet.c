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
#include "winreg.h"
#include "shlwapi.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(urlmon);

static const WCHAR feature_control_keyW[] =
    L"Software\\Microsoft\\Internet Explorer\\Main\\FeatureControl";

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
    {L"FEATURE_OBJECT_CACHING",                   TRUE ,TRUE},
    {L"FEATURE_ZONE_ELEVATION",                   FALSE,TRUE},
    {L"FEATURE_MIME_HANDLING",                    FALSE,TRUE},
    {L"FEATURE_MIME_SNIFFING",                    FALSE,TRUE},
    {L"FEATURE_WINDOW_RESTRICTIONS",              FALSE,TRUE},
    {L"FEATURE_WEBOC_POPUPMANAGEMENT",            FALSE,TRUE},
    {L"FEATURE_BEHAVIORS",                        TRUE ,TRUE},
    {L"FEATURE_DISABLE_MK_PROTOCOL",              TRUE ,TRUE},
    {L"FEATURE_LOCALMACHINE_LOCKDOWN",            FALSE,TRUE},
    {L"FEATURE_SECURITYBAND",                     FALSE,TRUE},
    {L"FEATURE_RESTRICT_ACTIVEXINSTALL",          FALSE,TRUE},
    {L"FEATURE_VALIDATE_NAVIGATE_URL",            FALSE,TRUE},
    {L"FEATURE_RESTRICT_FILEDOWNLOAD",            FALSE,TRUE},
    {L"FEATURE_ADDON_MANAGEMENT",                 FALSE,TRUE},
    {L"FEATURE_PROTOCOL_LOCKDOWN",                FALSE,TRUE},
    {L"FEATURE_HTTP_USERNAME_PASSWORD_DISABLE",   FALSE,TRUE},
    {L"FEATURE_SAFE_BINDTOOBJECT",                FALSE,TRUE},
    {L"FEATURE_UNC_SAVEDFILECHECK",               FALSE,TRUE},
    {L"FEATURE_GET_URL_DOM_FILEPATH_UNENCODED",   TRUE ,TRUE},
    {L"FEATURE_TABBED_BROWSING",                  FALSE,TRUE},
    {L"FEATURE_SSLUX",                            FALSE,TRUE},
    {L"FEATURE_DISABLE_NAVIGATION_SOUNDS",        FALSE,TRUE},
    {L"FEATURE_DISABLE_LEGACY_COMPRESSION",       TRUE ,TRUE},
    {L"FEATURE_FORCE_ADDR_AND_STATUS",            FALSE,TRUE},
    {L"FEATURE_XMLHTTP",                          TRUE ,TRUE},
    {L"FEATURE_DISABLE_TELNET_PROTOCOL",          FALSE,TRUE},
    {L"FEATURE_FEEDS",                            FALSE,TRUE},
    {L"FEATURE_BLOCK_INPUT_PROMPTS",              FALSE,TRUE}
};

static HRESULT parse_schema(LPCWSTR url, DWORD flags, LPWSTR result, DWORD size, DWORD *rsize)
{
    WCHAR *ptr;
    DWORD len = 0;

    TRACE("(%s %08lx %p %ld %p)\n", debugstr_w(url), flags, result, size, rsize);

    if(flags)
        ERR("wrong flags\n");
    
    ptr = wcschr(url, ':');
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

    TRACE("(%s %08lx %p %ld %p)\n", debugstr_w(url), flags, result, size, rsize);

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

    TRACE("(%s %08lx %p %ld %p)\n", debugstr_w(url), flags, result, size, rsize);

    protocol_info = get_protocol_info(url);

    if(protocol_info) {
        hres = IInternetProtocolInfo_ParseUrl(protocol_info, url, PARSE_SECURITY_URL,
                flags, result, size, rsize, 0);
        IInternetProtocolInfo_Release(protocol_info);
        return hres;
    }

    return E_FAIL;
}

static HRESULT parse_encode(LPCWSTR url, PARSEACTION action, DWORD flags, LPWSTR result, DWORD size, DWORD *rsize)
{
    IInternetProtocolInfo *protocol_info;
    DWORD prsize;
    HRESULT hres;

    TRACE("(%s %08lx %p %ld %p)\n", debugstr_w(url), flags, result, size, rsize);

    protocol_info = get_protocol_info(url);

    if(protocol_info) {
        hres = IInternetProtocolInfo_ParseUrl(protocol_info, url, action,
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

    TRACE("(%s %08lx %p %ld %p)\n", debugstr_w(url), flags, result, size, rsize);

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

    TRACE("(%s %08lx %p %ld %p)\n", debugstr_w(url), flags, result, size, rsize);

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

    TRACE("(%s %08lx %p %ld %p)\n", debugstr_w(url), flags, result, size, rsize);

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

    TRACE("(%s %08lx %p %ld %p)\n", debugstr_w(url), flags, result, size, rsize);

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
        WARN("dwReserved = %ld\n", dwReserved);

    switch(ParseAction) {
    case PARSE_CANONICALIZE:
        return parse_canonicalize_url(pwzUrl, dwFlags, pszResult, cchResult, pcchResult);
    case PARSE_SECURITY_URL:
        return parse_security_url(pwzUrl, dwFlags, pszResult, cchResult, pcchResult);
    case PARSE_ENCODE:
    case PARSE_UNESCAPE:
        return parse_encode(pwzUrl, ParseAction, dwFlags, pszResult, cchResult, pcchResult);
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
    
    TRACE("(%s,%s,0x%08lx,%p,%ld,%p,%ld)\n", debugstr_w(pwzBaseUrl),
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

    TRACE("(%s,%s,%08lx)\n", debugstr_w(pwzUrl1), debugstr_w(pwzUrl2), dwCompareFlags);

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

    TRACE("(%s, %x, %lx, %p, %lx, %p, %lx)\n", debugstr_w(pwzUrl),
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
        FIXME("Unsupported flags: %08lx\n", flags & ~supported_flags);

    if(flags & SET_FEATURE_ON_PROCESS)
        set_feature_on_process(feature, enable);

    return S_OK;
}

static BOOL get_feature_from_reg(HKEY feature_control, LPCWSTR feature_name, LPCWSTR process_name, BOOL *enabled)
{
    DWORD type, value, size;
    HKEY feature;
    DWORD res;

    res = RegOpenKeyW(feature_control, feature_name, &feature);
    if(res != ERROR_SUCCESS)
        return FALSE;

    size = sizeof(DWORD);
    res = RegQueryValueExW(feature, process_name, NULL, &type, (BYTE*)&value, &size);
    if(res != ERROR_SUCCESS || type != REG_DWORD) {
        size = sizeof(DWORD);
        res = RegQueryValueExW(feature, L"*", NULL, &type, (BYTE*)&value, &size);
    }

    RegCloseKey(feature);
    if(res != ERROR_SUCCESS)
        return FALSE;

    if(type != REG_DWORD) {
        WARN("Unexpected registry value type %ld (expected REG_DWORD) for %s\n", type, debugstr_w(L"*"));
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

    if (!GetModuleFileNameW(NULL, module_name, ARRAY_SIZE(module_name))) {
        ERR("Failed to get module file name: %lu\n", GetLastError());
        return E_UNEXPECTED;
    }

    process_name = wcsrchr(module_name, '\\');
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
        FIXME("Unsupported flags: %08lx\n", flags);
        hres = E_NOTIMPL;
    }

    return hres;
}

/***********************************************************************
 *             CoInternetSetFeatureEnabled (URLMON.@)
 */
HRESULT WINAPI CoInternetSetFeatureEnabled(INTERNETFEATURELIST FeatureEntry, DWORD dwFlags, BOOL fEnable)
{
    TRACE("(%d, %08lx, %x)\n", FeatureEntry, dwFlags, fEnable);
    return set_internet_feature(FeatureEntry, dwFlags, fEnable);
}

/***********************************************************************
 *             CoInternetIsFeatureEnabled (URLMON.@)
 */
HRESULT WINAPI CoInternetIsFeatureEnabled(INTERNETFEATURELIST FeatureEntry, DWORD dwFlags)
{
    TRACE("(%d, %08lx)\n", FeatureEntry, dwFlags);
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

    TRACE("(%d %08lx %s %p)\n", FeatureEntry, dwFlags, debugstr_w(szURL), pSecMgr);

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
        FIXME("unsupported flags %lx\n", dwFlags);
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

    TRACE("(%s %s %p %lx)\n", debugstr_w(szFromURL), debugstr_w(szToURL), pSecMgr, dwFlags);

    if(!pSecMgr || !szToURL)
        return CoInternetIsFeatureEnabled(FEATURE_ZONE_ELEVATION, dwFlags);

    switch(dwFlags) {
    case GET_FEATURE_FROM_THREAD:
    case GET_FEATURE_FROM_THREAD_LOCALMACHINE:
    case GET_FEATURE_FROM_THREAD_INTRANET:
    case GET_FEATURE_FROM_THREAD_TRUSTED:
    case GET_FEATURE_FROM_THREAD_INTERNET:
    case GET_FEATURE_FROM_THREAD_RESTRICTED:
        FIXME("unsupported flags %lx\n", dwFlags);
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
