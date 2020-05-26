/*
 * PropVariant implementation
 *
 * Copyright 2008 James Hawkins for CodeWeavers
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
#include <stdio.h>
#include <stdlib.h>

#define NONAMELESSUNION

#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "winreg.h"
#include "winuser.h"
#include "shlobj.h"
#include "propvarutil.h"
#include "strsafe.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(propsys);

static HRESULT PROPVAR_ConvertFILETIME(const FILETIME *ft, PROPVARIANT *ppropvarDest, VARTYPE vt)
{
    SYSTEMTIME time;

    FileTimeToSystemTime(ft, &time);

    switch (vt)
    {
        case VT_LPSTR:
            ppropvarDest->u.pszVal = HeapAlloc(GetProcessHeap(), 0, 64);
            if (!ppropvarDest->u.pszVal)
                return E_OUTOFMEMORY;

            sprintf( ppropvarDest->u.pszVal, "%04d/%02d/%02d:%02d:%02d:%02d.%03d",
                      time.wYear, time.wMonth, time.wDay,
                      time.wHour, time.wMinute, time.wSecond,
                      time.wMilliseconds );

            return S_OK;

        default:
            FIXME("Unhandled target type: %d\n", vt);
    }

    return E_FAIL;
}

static HRESULT PROPVAR_ConvertNumber(REFPROPVARIANT pv, int dest_bits,
                                     BOOL dest_signed, LONGLONG *res)
{
    BOOL src_signed;

    switch (pv->vt)
    {
    case VT_I1:
        src_signed = TRUE;
        *res = pv->u.cVal;
        break;
    case VT_UI1:
        src_signed = FALSE;
        *res = pv->u.bVal;
        break;
    case VT_I2:
        src_signed = TRUE;
        *res = pv->u.iVal;
        break;
    case VT_UI2:
        src_signed = FALSE;
        *res = pv->u.uiVal;
        break;
    case VT_I4:
        src_signed = TRUE;
        *res = pv->u.lVal;
        break;
    case VT_UI4:
        src_signed = FALSE;
        *res = pv->u.ulVal;
        break;
    case VT_I8:
        src_signed = TRUE;
        *res = pv->u.hVal.QuadPart;
        break;
    case VT_UI8:
        src_signed = FALSE;
        *res = pv->u.uhVal.QuadPart;
        break;
    case VT_EMPTY:
        src_signed = FALSE;
        *res = 0;
        break;
    case VT_LPSTR:
    {
        char *end;
        *res = _strtoi64(pv->u.pszVal, &end, 0);
        if (pv->u.pszVal == end)
            return DISP_E_TYPEMISMATCH;
        src_signed = *res < 0;
        break;
    }
    case VT_LPWSTR:
    case VT_BSTR:
    {
        WCHAR *end;
        *res = wcstol(pv->u.pwszVal, &end, 0);
        if (pv->u.pwszVal == end)
            return DISP_E_TYPEMISMATCH;
        src_signed = *res < 0;
        break;
    }
    case VT_R8:
    {
        src_signed = TRUE;
        *res = pv->u.dblVal;
        break;
    }
    default:
        FIXME("unhandled vt %d\n", pv->vt);
        return E_NOTIMPL;
    }

    if (*res < 0 && src_signed != dest_signed)
        return HRESULT_FROM_WIN32(ERROR_ARITHMETIC_OVERFLOW);

    if (dest_bits < 64)
    {
        if (dest_signed)
        {
            if (*res >= ((LONGLONG)1 << (dest_bits-1)) ||
                *res < ((LONGLONG)-1 << (dest_bits-1)))
                return HRESULT_FROM_WIN32(ERROR_ARITHMETIC_OVERFLOW);
        }
        else
        {
            if ((ULONGLONG)(*res) >= ((ULONGLONG)1 << dest_bits))
                return HRESULT_FROM_WIN32(ERROR_ARITHMETIC_OVERFLOW);
        }
    }

    return S_OK;
}

HRESULT WINAPI PropVariantToDouble(REFPROPVARIANT propvarIn, double *ret)
{
    LONGLONG res;
    HRESULT hr;

    TRACE("(%p, %p)\n", propvarIn, ret);

    hr = PROPVAR_ConvertNumber(propvarIn, 64, TRUE, &res);
    if (SUCCEEDED(hr)) *ret = (double)res;
    return hr;
}

HRESULT WINAPI PropVariantToInt16(REFPROPVARIANT propvarIn, SHORT *ret)
{
    LONGLONG res;
    HRESULT hr;

    TRACE("%p,%p\n", propvarIn, ret);

    hr = PROPVAR_ConvertNumber(propvarIn, 16, TRUE, &res);
    if (SUCCEEDED(hr)) *ret = (SHORT)res;
    return hr;
}

HRESULT WINAPI PropVariantToInt32(REFPROPVARIANT propvarIn, LONG *ret)
{
    LONGLONG res;
    HRESULT hr;

    TRACE("%p,%p\n", propvarIn, ret);

    hr = PROPVAR_ConvertNumber(propvarIn, 32, TRUE, &res);
    if (SUCCEEDED(hr)) *ret = (LONG)res;
    return hr;
}

HRESULT WINAPI PropVariantToInt64(REFPROPVARIANT propvarIn, LONGLONG *ret)
{
    LONGLONG res;
    HRESULT hr;

    TRACE("%p,%p\n", propvarIn, ret);

    hr = PROPVAR_ConvertNumber(propvarIn, 64, TRUE, &res);
    if (SUCCEEDED(hr)) *ret = res;
    return hr;
}

HRESULT WINAPI PropVariantToUInt16(REFPROPVARIANT propvarIn, USHORT *ret)
{
    LONGLONG res;
    HRESULT hr;

    TRACE("%p,%p\n", propvarIn, ret);

    hr = PROPVAR_ConvertNumber(propvarIn, 16, FALSE, &res);
    if (SUCCEEDED(hr)) *ret = (USHORT)res;
    return hr;
}

HRESULT WINAPI PropVariantToUInt32(REFPROPVARIANT propvarIn, ULONG *ret)
{
    LONGLONG res;
    HRESULT hr;

    TRACE("%p,%p\n", propvarIn, ret);

    hr = PROPVAR_ConvertNumber(propvarIn, 32, FALSE, &res);
    if (SUCCEEDED(hr)) *ret = (ULONG)res;
    return hr;
}

HRESULT WINAPI PropVariantToUInt64(REFPROPVARIANT propvarIn, ULONGLONG *ret)
{
    LONGLONG res;
    HRESULT hr;

    TRACE("%p,%p\n", propvarIn, ret);

    hr = PROPVAR_ConvertNumber(propvarIn, 64, FALSE, &res);
    if (SUCCEEDED(hr)) *ret = (ULONGLONG)res;
    return hr;
}

HRESULT WINAPI PropVariantToBoolean(REFPROPVARIANT propvarIn, BOOL *ret)
{
    static const WCHAR trueW[] = {'t','r','u','e',0};
    static const WCHAR falseW[] = {'f','a','l','s','e',0};
    static const WCHAR true2W[] = {'#','T','R','U','E','#',0};
    static const WCHAR false2W[] = {'#','F','A','L','S','E','#',0};
    LONGLONG res;
    HRESULT hr;

    TRACE("%p,%p\n", propvarIn, ret);

    *ret = FALSE;

    switch (propvarIn->vt)
    {
        case VT_BOOL:
            *ret = propvarIn->u.boolVal == VARIANT_TRUE;
            return S_OK;

        case VT_LPWSTR:
        case VT_BSTR:
            if (!propvarIn->u.pwszVal)
                return DISP_E_TYPEMISMATCH;

            if (!lstrcmpiW(propvarIn->u.pwszVal, trueW) || !lstrcmpW(propvarIn->u.pwszVal, true2W))
            {
                *ret = TRUE;
                return S_OK;
            }

            if (!lstrcmpiW(propvarIn->u.pwszVal, falseW) || !lstrcmpW(propvarIn->u.pwszVal, false2W))
            {
                *ret = FALSE;
                return S_OK;
            }
            break;

         case VT_LPSTR:
            if (!propvarIn->u.pszVal)
                return DISP_E_TYPEMISMATCH;

            if (!lstrcmpiA(propvarIn->u.pszVal, "true") || !lstrcmpA(propvarIn->u.pszVal, "#TRUE#"))
            {
                *ret = TRUE;
                return S_OK;
            }

            if (!lstrcmpiA(propvarIn->u.pszVal, "false") || !lstrcmpA(propvarIn->u.pszVal, "#FALSE#"))
            {
                *ret = FALSE;
                return S_OK;
            }
            break;
    }

    hr = PROPVAR_ConvertNumber(propvarIn, 64, TRUE, &res);
    *ret = !!res;
    return hr;
}

HRESULT WINAPI PropVariantToBuffer(REFPROPVARIANT propvarIn, void *ret, UINT cb)
{
    HRESULT hr = S_OK;

    TRACE("(%p, %p, %d)\n", propvarIn, ret, cb);

    switch(propvarIn->vt)
    {
        case VT_VECTOR|VT_UI1:
            if(cb > propvarIn->u.caub.cElems)
                return E_FAIL;
            memcpy(ret, propvarIn->u.caub.pElems, cb);
            break;
        case VT_ARRAY|VT_UI1:
            FIXME("Unsupported type: VT_ARRAY|VT_UI1\n");
            hr = E_NOTIMPL;
            break;
        default:
            WARN("Unexpected type: %x\n", propvarIn->vt);
            hr = E_INVALIDARG;
    }

    return hr;
}


HRESULT WINAPI PropVariantToString(REFPROPVARIANT propvarIn, PWSTR ret, UINT cch)
{
    HRESULT hr;
    WCHAR *stringW = NULL;

    TRACE("(%p, %p, %d)\n", propvarIn, ret, cch);

    ret[0] = '\0';

    if(!cch)
        return E_INVALIDARG;

    hr = PropVariantToStringAlloc(propvarIn, &stringW);
    if(SUCCEEDED(hr))
    {
        if(lstrlenW(stringW) >= cch)
            hr = STRSAFE_E_INSUFFICIENT_BUFFER;
        lstrcpynW(ret, stringW, cch);
        CoTaskMemFree(stringW);
    }

    return hr;
}

HRESULT WINAPI PropVariantToStringAlloc(REFPROPVARIANT propvarIn, WCHAR **ret)
{
    WCHAR *res = NULL;
    HRESULT hr = S_OK;

    TRACE("%p,%p semi-stub\n", propvarIn, ret);

    switch(propvarIn->vt)
    {
        case VT_EMPTY:
        case VT_NULL:
            res = CoTaskMemAlloc(1*sizeof(WCHAR));
            res[0] = '\0';
            break;

        case VT_LPSTR:
            if(propvarIn->u.pszVal)
            {
                DWORD len;

                len = MultiByteToWideChar(CP_ACP, 0, propvarIn->u.pszVal, -1, NULL, 0);
                res = CoTaskMemAlloc(len*sizeof(WCHAR));
                if(!res)
                    return E_OUTOFMEMORY;

                MultiByteToWideChar(CP_ACP, 0, propvarIn->u.pszVal, -1, res, len);
            }
            break;

        case VT_LPWSTR:
        case VT_BSTR:
            if (propvarIn->u.pwszVal)
            {
                DWORD size = (lstrlenW(propvarIn->u.pwszVal) + 1) * sizeof(WCHAR);
                res = CoTaskMemAlloc(size);
                if(!res) return E_OUTOFMEMORY;
                memcpy(res, propvarIn->u.pwszVal, size);
            }
            break;

        default:
            FIXME("Unsupported conversion (%d)\n", propvarIn->vt);
            hr = E_FAIL;
            break;
    }

    *ret = res;

    return hr;
}

PCWSTR WINAPI PropVariantToStringWithDefault(REFPROPVARIANT propvarIn, LPCWSTR pszDefault)
{
    static const WCHAR str_empty[] = {0};
    if (propvarIn->vt == VT_BSTR)
    {
        if (propvarIn->u.bstrVal == NULL)
            return str_empty;

        return propvarIn->u.bstrVal;
    }

    if (propvarIn->vt == VT_LPWSTR && propvarIn->u.pwszVal != NULL)
        return propvarIn->u.pwszVal;

    return pszDefault;
}


/******************************************************************
 *  PropVariantChangeType   (PROPSYS.@)
 */
HRESULT WINAPI PropVariantChangeType(PROPVARIANT *ppropvarDest, REFPROPVARIANT propvarSrc,
                                     PROPVAR_CHANGE_FLAGS flags, VARTYPE vt)
{
    HRESULT hr;

    FIXME("(%p, %p, %d, %d, %d): semi-stub!\n", ppropvarDest, propvarSrc,
          propvarSrc->vt, flags, vt);

    if (vt == propvarSrc->vt)
        return PropVariantCopy(ppropvarDest, propvarSrc);

    if (propvarSrc->vt == VT_FILETIME)
        return PROPVAR_ConvertFILETIME(&propvarSrc->u.filetime, ppropvarDest, vt);

    switch (vt)
    {
    case VT_I1:
    {
        LONGLONG res;

        hr = PROPVAR_ConvertNumber(propvarSrc, 8, TRUE, &res);
        if (SUCCEEDED(hr))
        {
            ppropvarDest->vt = VT_I1;
            ppropvarDest->u.cVal = (char)res;
        }
        return hr;
    }

    case VT_UI1:
    {
        LONGLONG res;

        hr = PROPVAR_ConvertNumber(propvarSrc, 8, FALSE, &res);
        if (SUCCEEDED(hr))
        {
            ppropvarDest->vt = VT_UI1;
            ppropvarDest->u.bVal = (UCHAR)res;
        }
        return hr;
    }

    case VT_I2:
    {
        SHORT res;
        hr = PropVariantToInt16(propvarSrc, &res);
        if (SUCCEEDED(hr))
        {
            ppropvarDest->vt = VT_I2;
            ppropvarDest->u.iVal = res;
        }
        return hr;
    }
    case VT_UI2:
    {
        USHORT res;
        hr = PropVariantToUInt16(propvarSrc, &res);
        if (SUCCEEDED(hr))
        {
            ppropvarDest->vt = VT_UI2;
            ppropvarDest->u.uiVal = res;
        }
        return hr;
    }
    case VT_I4:
    {
        LONG res;
        hr = PropVariantToInt32(propvarSrc, &res);
        if (SUCCEEDED(hr))
        {
            ppropvarDest->vt = VT_I4;
            ppropvarDest->u.lVal = res;
        }
        return hr;
    }
    case VT_UI4:
    {
        ULONG res;
        hr = PropVariantToUInt32(propvarSrc, &res);
        if (SUCCEEDED(hr))
        {
            ppropvarDest->vt = VT_UI4;
            ppropvarDest->u.ulVal = res;
        }
        return hr;
    }
    case VT_I8:
    {
        LONGLONG res;
        hr = PropVariantToInt64(propvarSrc, &res);
        if (SUCCEEDED(hr))
        {
            ppropvarDest->vt = VT_I8;
            ppropvarDest->u.hVal.QuadPart = res;
        }
        return hr;
    }
    case VT_UI8:
    {
        ULONGLONG res;
        hr = PropVariantToUInt64(propvarSrc, &res);
        if (SUCCEEDED(hr))
        {
            ppropvarDest->vt = VT_UI8;
            ppropvarDest->u.uhVal.QuadPart = res;
        }
        return hr;
    }

    case VT_LPWSTR:
    case VT_BSTR:
    {
        WCHAR *res;
        hr = PropVariantToStringAlloc(propvarSrc, &res);
        if (SUCCEEDED(hr))
        {
            ppropvarDest->vt = VT_LPWSTR;
            ppropvarDest->u.pwszVal = res;
        }
        return hr;
    }

    case VT_LPSTR:
    {
        WCHAR *resW;
        hr = PropVariantToStringAlloc(propvarSrc, &resW);
        if (SUCCEEDED(hr))
        {
            char *res;
            DWORD len;

            len = WideCharToMultiByte(CP_ACP, 0, resW, -1, NULL, 0, NULL, NULL);
            res = CoTaskMemAlloc(len);
            if (res)
            {
                WideCharToMultiByte(CP_ACP, 0, resW, -1, res, len, NULL, NULL);
                ppropvarDest->vt = VT_LPSTR;
                ppropvarDest->u.pszVal = res;
            }
            else
                hr = E_OUTOFMEMORY;

            CoTaskMemFree(resW);
        }
        return hr;
    }

    default:
        FIXME("Unhandled dest type: %d\n", vt);
        return E_FAIL;
    }
}

static void PROPVAR_GUIDToWSTR(REFGUID guid, WCHAR *str)
{
    static const WCHAR format[] = {'{','%','0','8','X','-','%','0','4','X','-','%','0','4','X',
        '-','%','0','2','X','%','0','2','X','-','%','0','2','X','%','0','2','X','%','0','2','X',
        '%','0','2','X','%','0','2','X','%','0','2','X','}',0};

    swprintf(str, format, guid->Data1, guid->Data2, guid->Data3,
            guid->Data4[0], guid->Data4[1], guid->Data4[2], guid->Data4[3],
            guid->Data4[4], guid->Data4[5], guid->Data4[6], guid->Data4[7]);
}

HRESULT WINAPI InitPropVariantFromGUIDAsString(REFGUID guid, PROPVARIANT *ppropvar)
{
    TRACE("(%p %p)\n", guid, ppropvar);

    if(!guid)
        return E_FAIL;

    ppropvar->vt = VT_LPWSTR;
    ppropvar->u.pwszVal = CoTaskMemAlloc(39*sizeof(WCHAR));
    if(!ppropvar->u.pwszVal)
        return E_OUTOFMEMORY;

    PROPVAR_GUIDToWSTR(guid, ppropvar->u.pwszVal);
    return S_OK;
}

HRESULT WINAPI InitVariantFromGUIDAsString(REFGUID guid, VARIANT *pvar)
{
    TRACE("(%p %p)\n", guid, pvar);

    if(!guid) {
        FIXME("guid == NULL\n");
        return E_FAIL;
    }

    V_VT(pvar) = VT_BSTR;
    V_BSTR(pvar) = SysAllocStringLen(NULL, 38);
    if(!V_BSTR(pvar))
        return E_OUTOFMEMORY;

    PROPVAR_GUIDToWSTR(guid, V_BSTR(pvar));
    return S_OK;
}

HRESULT WINAPI InitPropVariantFromBuffer(const VOID *pv, UINT cb, PROPVARIANT *ppropvar)
{
    TRACE("(%p %u %p)\n", pv, cb, ppropvar);

    ppropvar->u.caub.pElems = CoTaskMemAlloc(cb);
    if(!ppropvar->u.caub.pElems)
        return E_OUTOFMEMORY;

    ppropvar->vt = VT_VECTOR|VT_UI1;
    ppropvar->u.caub.cElems = cb;
    memcpy(ppropvar->u.caub.pElems, pv, cb);
    return S_OK;
}

HRESULT WINAPI InitPropVariantFromCLSID(REFCLSID clsid, PROPVARIANT *ppropvar)
{
    TRACE("(%s %p)\n", debugstr_guid(clsid), ppropvar);

    ppropvar->u.puuid = CoTaskMemAlloc(sizeof(*ppropvar->u.puuid));
    if(!ppropvar->u.puuid)
        return E_OUTOFMEMORY;

    ppropvar->vt = VT_CLSID;
    memcpy(ppropvar->u.puuid, clsid, sizeof(*ppropvar->u.puuid));
    return S_OK;
}

HRESULT WINAPI InitVariantFromBuffer(const VOID *pv, UINT cb, VARIANT *pvar)
{
    SAFEARRAY *arr;
    void *data;
    HRESULT hres;

    TRACE("(%p %u %p)\n", pv, cb, pvar);

    arr = SafeArrayCreateVector(VT_UI1, 0, cb);
    if(!arr)
        return E_OUTOFMEMORY;

    hres = SafeArrayAccessData(arr, &data);
    if(FAILED(hres)) {
        SafeArrayDestroy(arr);
        return hres;
    }

    memcpy(data, pv, cb);

    hres = SafeArrayUnaccessData(arr);
    if(FAILED(hres)) {
        SafeArrayDestroy(arr);
        return hres;
    }

    V_VT(pvar) = VT_ARRAY|VT_UI1;
    V_ARRAY(pvar) = arr;
    return S_OK;
}

static inline DWORD PROPVAR_HexToNum(const WCHAR *hex)
{
    DWORD ret;

    if(hex[0]>='0' && hex[0]<='9')
        ret = hex[0]-'0';
    else if(hex[0]>='a' && hex[0]<='f')
        ret = hex[0]-'a'+10;
    else if(hex[0]>='A' && hex[0]<='F')
        ret = hex[0]-'A'+10;
    else
        return -1;

    ret <<= 4;
    if(hex[1]>='0' && hex[1]<='9')
        return ret + hex[1]-'0';
    else if(hex[1]>='a' && hex[1]<='f')
        return ret + hex[1]-'a'+10;
    else if(hex[1]>='A' && hex[1]<='F')
        return ret + hex[1]-'A'+10;
    else
        return -1;
}

static inline HRESULT PROPVAR_WCHARToGUID(const WCHAR *str, int len, GUID *guid)
{
    DWORD i, val=0;
    const WCHAR *p;

    memset(guid, 0, sizeof(GUID));

    if(len!=38 || str[0]!='{' || str[9]!='-' || str[14]!='-'
            || str[19]!='-' || str[24]!='-' || str[37]!='}') {
        WARN("Error parsing %s\n", debugstr_w(str));
        return E_INVALIDARG;
    }

    p = str+1;
    for(i=0; i<4 && val!=-1; i++) {
        val = PROPVAR_HexToNum(p);
        guid->Data1 = (guid->Data1<<8) + val;
        p += 2;
    }
    p++;
    for(i=0; i<2 && val!=-1; i++) {
        val = PROPVAR_HexToNum(p);
        guid->Data2 = (guid->Data2<<8) + val;
        p += 2;
    }
    p++;
    for(i=0; i<2 && val!=-1; i++) {
        val = PROPVAR_HexToNum(p);
        guid->Data3 = (guid->Data3<<8) + val;
        p += 2;
    }
    p++;
    for(i=0; i<8 && val!=-1; i++) {
        if(i == 2)
            p++;

        val = guid->Data4[i] = PROPVAR_HexToNum(p);
        p += 2;
    }

    if(val == -1) {
        WARN("Error parsing %s\n", debugstr_w(str));
        memset(guid, 0, sizeof(GUID));
        return E_INVALIDARG;
    }
    return S_OK;
}

HRESULT WINAPI PropVariantToGUID(const PROPVARIANT *ppropvar, GUID *guid)
{
    TRACE("%p %p)\n", ppropvar, guid);

    switch(ppropvar->vt) {
    case VT_BSTR:
        return PROPVAR_WCHARToGUID(ppropvar->u.bstrVal, SysStringLen(ppropvar->u.bstrVal), guid);
    case VT_LPWSTR:
        return PROPVAR_WCHARToGUID(ppropvar->u.pwszVal, lstrlenW(ppropvar->u.pwszVal), guid);
    case VT_CLSID:
        memcpy(guid, ppropvar->u.puuid, sizeof(*ppropvar->u.puuid));
        return S_OK;

    default:
        FIXME("unsupported vt: %d\n", ppropvar->vt);
        return E_NOTIMPL;
    }
}

HRESULT WINAPI VariantToGUID(const VARIANT *pvar, GUID *guid)
{
    TRACE("(%p %p)\n", pvar, guid);

    switch(V_VT(pvar)) {
    case VT_BSTR: {
        HRESULT hres = PROPVAR_WCHARToGUID(V_BSTR(pvar), SysStringLen(V_BSTR(pvar)), guid);
        if(hres == E_INVALIDARG)
            return E_FAIL;
        return hres;
    }

    default:
        FIXME("unsupported vt: %d\n", V_VT(pvar));
        return E_NOTIMPL;
    }
}

static BOOL isemptyornull(const PROPVARIANT *propvar)
{
    if (propvar->vt == VT_EMPTY || propvar->vt == VT_NULL)
        return TRUE;
    if ((propvar->vt & VT_ARRAY) == VT_ARRAY)
    {
        int i;
        for (i=0; i<propvar->u.parray->cDims; i++)
        {
            if (propvar->u.parray->rgsabound[i].cElements != 0)
                break;
        }
        return i == propvar->u.parray->cDims;
    }
    if (propvar->vt == VT_CLSID)
        return !propvar->u.puuid;

    /* FIXME: vectors, byrefs, errors? */
    return FALSE;
}

INT WINAPI PropVariantCompareEx(REFPROPVARIANT propvar1, REFPROPVARIANT propvar2,
    PROPVAR_COMPARE_UNIT unit, PROPVAR_COMPARE_FLAGS flags)
{
    const PROPVARIANT *propvar2_converted;
    PROPVARIANT propvar2_static;
    HRESULT hr;
    INT res=-1;

    TRACE("%p,%p,%x,%x\n", propvar1, propvar2, unit, flags);

    if (isemptyornull(propvar1))
    {
        if (isemptyornull(propvar2))
            return 0;
        return (flags & PVCF_TREATEMPTYASGREATERTHAN) ? 1 : -1;
    }

    if (isemptyornull(propvar2))
        return (flags & PVCF_TREATEMPTYASGREATERTHAN) ? -1 : 1;

    if (propvar1->vt != propvar2->vt)
    {
        hr = PropVariantChangeType(&propvar2_static, propvar2, 0, propvar1->vt);

        if (FAILED(hr))
            return -1;

        propvar2_converted = &propvar2_static;
    }
    else
        propvar2_converted = propvar2;

#define CMP_NUM_VALUE(var) do { \
    if (propvar1->u.var > propvar2_converted->u.var) \
        res = 1; \
    else if (propvar1->u.var < propvar2_converted->u.var) \
        res = -1; \
    else \
        res = 0; \
    } while (0)

    switch (propvar1->vt)
    {
    case VT_I1:
        CMP_NUM_VALUE(cVal);
        break;
    case VT_UI1:
        CMP_NUM_VALUE(bVal);
        break;
    case VT_I2:
        CMP_NUM_VALUE(iVal);
        break;
    case VT_UI2:
        CMP_NUM_VALUE(uiVal);
        break;
    case VT_I4:
        CMP_NUM_VALUE(lVal);
        break;
    case VT_UI4:
        CMP_NUM_VALUE(uiVal);
        break;
    case VT_I8:
        CMP_NUM_VALUE(hVal.QuadPart);
        break;
    case VT_UI8:
        CMP_NUM_VALUE(uhVal.QuadPart);
        break;
    case VT_R4:
        CMP_NUM_VALUE(fltVal);
        break;
    case VT_R8:
        CMP_NUM_VALUE(dblVal);
        break;
    case VT_BSTR:
    case VT_LPWSTR:
        /* FIXME: Use other string flags. */
        if (flags & (PVCF_USESTRCMPI | PVCF_USESTRCMPIC))
            res = lstrcmpiW(propvar1->u.bstrVal, propvar2_converted->u.bstrVal);
        else
            res = lstrcmpW(propvar1->u.bstrVal, propvar2_converted->u.bstrVal);
        break;
    case VT_LPSTR:
        /* FIXME: Use other string flags. */
        if (flags & (PVCF_USESTRCMPI | PVCF_USESTRCMPIC))
            res = lstrcmpiA(propvar1->u.pszVal, propvar2_converted->u.pszVal);
        else
            res = lstrcmpA(propvar1->u.pszVal, propvar2_converted->u.pszVal);
        break;
    case VT_CLSID:
        res = memcmp(propvar1->u.puuid, propvar2->u.puuid, sizeof(*propvar1->u.puuid));
        if (res) res = res > 0 ? 1 : -1;
        break;
    default:
        FIXME("vartype %#x not handled\n", propvar1->vt);
        res = -1;
        break;
    }

    if (propvar2_converted == &propvar2_static)
        PropVariantClear(&propvar2_static);

    return res;
}
