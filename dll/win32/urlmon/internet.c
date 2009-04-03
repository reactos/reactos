/*
 * Copyright 2005 Jacek Caban
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

    if(len >= size)
        return E_POINTER;

    if(len)
        memcpy(result, url, len*sizeof(WCHAR));
    result[len] = 0;

    if(rsize)
        *rsize = len;

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

/***********************************************************************
 *             CoInternetSetFeatureEnabled (URLMON.@)
 */
HRESULT WINAPI CoInternetSetFeatureEnabled(INTERNETFEATURELIST feature, DWORD flags, BOOL enable)
{
    FIXME("%d, 0x%08x, %x, stub\n", feature, flags, enable);
    return E_NOTIMPL;
}
