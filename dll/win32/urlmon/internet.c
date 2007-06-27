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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <stdarg.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "winreg.h"
#include "shlwapi.h"
#include "ole2.h"
#include "urlmon.h"
#include "urlmon_main.h"

#include "wine/debug.h"
#include "wine/unicode.h"

WINE_DEFAULT_DEBUG_CHANNEL(urlmon);

static HRESULT parse_schema(LPCWSTR url, DWORD flags, LPWSTR result, DWORD size, DWORD *rsize)
{
    WCHAR *ptr;
    DWORD len = 0;

    TRACE("(%s %08lx %p %ld %p)\n", debugstr_w(url), flags, result, size, rsize);

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

static IInternetProtocolInfo *get_protocol_info(LPCWSTR url)
{
    IInternetProtocolInfo *ret = NULL;
    IUnknown *unk;
    HRESULT hres;

    hres = get_protocol_iface(url, &unk);
    if(FAILED(hres))
        return NULL;

    IUnknown_QueryInterface(unk, &IID_IInternetProtocolInfo, (void**)&ret);
    IUnknown_Release(unk);

    return ret;
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
        return hres;
    }

    return E_FAIL;
}

static HRESULT parse_encode(LPCWSTR url, DWORD flags, LPWSTR result, DWORD size, DWORD *rsize)
{
    IInternetProtocolInfo *protocol_info;
    DWORD prsize;
    HRESULT hres;

    TRACE("(%s %08lx %p %ld %p)\n", debugstr_w(url), flags, result, size, rsize);

    protocol_info = get_protocol_info(url);

    if(protocol_info) {
        hres = IInternetProtocolInfo_ParseUrl(protocol_info, url, PARSE_ENCODE,
                flags, result, size, rsize, 0);
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
        WARN("dwReserved = %ld\n", dwReserved);

    switch(ParseAction) {
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
    
    TRACE("(%s,%s,0x%08lx,%p,%ld,%p,%ld)\n", debugstr_w(pwzBaseUrl),
          debugstr_w(pwzRelativeUrl), dwCombineFlags, pwzResult, cchResult, pcchResult,
          dwReserved);

    protocol_info = get_protocol_info(pwzBaseUrl);

    if(protocol_info) {
        hres = IInternetProtocolInfo_CombineUrl(protocol_info, pwzBaseUrl, pwzRelativeUrl,
                dwCombineFlags, pwzResult, cchResult, pcchResult, dwReserved);
        if(SUCCEEDED(hres))
            return hres;
    }


    hres = UrlCombineW(pwzBaseUrl, pwzRelativeUrl, pwzResult, &size, dwCombineFlags);

    if(pcchResult)
        *pcchResult = size;

    return hres;
}
