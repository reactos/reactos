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

#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "winreg.h"
#include "winuser.h"
#include "shlobj.h"
#include "shlwapi.h"
#include "propvarutil.h"
#include "strsafe.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(propsys);

#define GUID_STR_LEN 38
static HRESULT VARIANT_ValidateType(VARTYPE vt)
{
    VARTYPE vtExtra = vt & (VT_VECTOR | VT_ARRAY | VT_BYREF | VT_RESERVED);

    vt &= VT_TYPEMASK;

    if (!(vtExtra & (VT_VECTOR | VT_RESERVED)))
    {
        if (vt < VT_VOID || vt == VT_RECORD || vt == VT_CLSID)
        {
            if ((vtExtra & (VT_BYREF | VT_ARRAY)) && vt <= VT_NULL)
                return DISP_E_BADVARTYPE;
            if (vt != (VARTYPE)15)
                return S_OK;
        }
    }

    return DISP_E_BADVARTYPE;
}

static HRESULT PROPVAR_ConvertFILETIME(const FILETIME *ft, PROPVARIANT *ppropvarDest, VARTYPE vt)
{
    SYSTEMTIME time;

    FileTimeToSystemTime(ft, &time);

    switch (vt)
    {
        case VT_LPSTR:
            ppropvarDest->pszVal = HeapAlloc(GetProcessHeap(), 0, 64);
            if (!ppropvarDest->pszVal)
                return E_OUTOFMEMORY;

            sprintf( ppropvarDest->pszVal, "%04d/%02d/%02d:%02d:%02d:%02d.%03d",
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
        *res = pv->cVal;
        break;
    case VT_UI1:
        src_signed = FALSE;
        *res = pv->bVal;
        break;
    case VT_I2:
        src_signed = TRUE;
        *res = pv->iVal;
        break;
    case VT_UI2:
        src_signed = FALSE;
        *res = pv->uiVal;
        break;
    case VT_I4:
        src_signed = TRUE;
        *res = pv->lVal;
        break;
    case VT_UI4:
        src_signed = FALSE;
        *res = pv->ulVal;
        break;
    case VT_I8:
        src_signed = TRUE;
        *res = pv->hVal.QuadPart;
        break;
    case VT_UI8:
        src_signed = FALSE;
        *res = pv->uhVal.QuadPart;
        break;
    case VT_EMPTY:
        src_signed = FALSE;
        *res = 0;
        break;
    case VT_LPSTR:
    {
        char *end;
#ifdef __REACTOS__
        *res = _strtoi64(pv->pszVal, &end, 0);
#else
        *res = strtoll(pv->pszVal, &end, 0);
#endif
        if (pv->pszVal == end)
            return DISP_E_TYPEMISMATCH;
        src_signed = *res < 0;
        break;
    }
    case VT_LPWSTR:
    case VT_BSTR:
    {
        WCHAR *end;
        *res = wcstol(pv->pwszVal, &end, 0);
        if (pv->pwszVal == end)
            return DISP_E_TYPEMISMATCH;
        src_signed = *res < 0;
        break;
    }
    case VT_R8:
    {
        src_signed = TRUE;
        *res = pv->dblVal;
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

ULONG WINAPI PropVariantToUInt32WithDefault(REFPROPVARIANT propvarIn, ULONG ulDefault)
{
    LONGLONG res;
    HRESULT hr;

    TRACE("%p,%lu\n", propvarIn, ulDefault);

    hr = PROPVAR_ConvertNumber(propvarIn, 32, FALSE, &res);
    if (SUCCEEDED(hr))
        return (ULONG)res;

    return ulDefault;
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
    LONGLONG res;
    HRESULT hr;

    TRACE("%p,%p\n", propvarIn, ret);

    *ret = FALSE;

    switch (propvarIn->vt)
    {
        case VT_BOOL:
            *ret = propvarIn->boolVal == VARIANT_TRUE;
            return S_OK;

        case VT_LPWSTR:
        case VT_BSTR:
            if (!propvarIn->pwszVal)
                return DISP_E_TYPEMISMATCH;

            if (!lstrcmpiW(propvarIn->pwszVal, L"true") || !lstrcmpW(propvarIn->pwszVal, L"#TRUE#"))
            {
                *ret = TRUE;
                return S_OK;
            }

            if (!lstrcmpiW(propvarIn->pwszVal, L"false") || !lstrcmpW(propvarIn->pwszVal, L"#FALSE#"))
            {
                *ret = FALSE;
                return S_OK;
            }
            break;

         case VT_LPSTR:
            if (!propvarIn->pszVal)
                return DISP_E_TYPEMISMATCH;

            if (!lstrcmpiA(propvarIn->pszVal, "true") || !lstrcmpA(propvarIn->pszVal, "#TRUE#"))
            {
                *ret = TRUE;
                return S_OK;
            }

            if (!lstrcmpiA(propvarIn->pszVal, "false") || !lstrcmpA(propvarIn->pszVal, "#FALSE#"))
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

HRESULT WINAPI PropVariantToBSTR(REFPROPVARIANT propvar, BSTR *bstr)
{
    WCHAR *str;
    HRESULT hr;

    TRACE("propvar %p, propvar->vt %#x, bstr %p.\n",
            propvar, propvar ? propvar->vt : 0, bstr);

    if (FAILED(hr = PropVariantToStringAlloc(propvar, &str)))
        return hr;

    *bstr = SysAllocString(str);
    CoTaskMemFree(str);

    if (!*bstr)
        return E_OUTOFMEMORY;

    return S_OK;
}

HRESULT WINAPI PropVariantToBuffer(REFPROPVARIANT propvarIn, void *ret, UINT cb)
{
    HRESULT hr = S_OK;

    TRACE("(%p, %p, %d)\n", propvarIn, ret, cb);

    switch(propvarIn->vt)
    {
        case VT_VECTOR|VT_UI1:
            if(cb > propvarIn->caub.cElems)
                return E_FAIL;
            memcpy(ret, propvarIn->caub.pElems, cb);
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
            if(propvarIn->pszVal)
            {
                DWORD len;

                len = MultiByteToWideChar(CP_ACP, 0, propvarIn->pszVal, -1, NULL, 0);
                res = CoTaskMemAlloc(len*sizeof(WCHAR));
                if(!res)
                    return E_OUTOFMEMORY;

                MultiByteToWideChar(CP_ACP, 0, propvarIn->pszVal, -1, res, len);
            }
            break;

        case VT_LPWSTR:
        case VT_BSTR:
            if (propvarIn->pwszVal)
            {
                DWORD size = (lstrlenW(propvarIn->pwszVal) + 1) * sizeof(WCHAR);
                res = CoTaskMemAlloc(size);
                if(!res) return E_OUTOFMEMORY;
                memcpy(res, propvarIn->pwszVal, size);
            }
            break;

        case VT_CLSID:
            if (propvarIn->puuid)
            {
                if (!(res = CoTaskMemAlloc((GUID_STR_LEN + 1) * sizeof(WCHAR))))
                    return E_OUTOFMEMORY;
                StringFromGUID2(propvarIn->puuid, res, GUID_STR_LEN + 1);
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
    if (propvarIn->vt == VT_BSTR)
    {
        if (propvarIn->bstrVal == NULL)
            return L"";

        return propvarIn->bstrVal;
    }

    if (propvarIn->vt == VT_LPWSTR && propvarIn->pwszVal != NULL)
        return propvarIn->pwszVal;

    return pszDefault;
}

/******************************************************************
 *  VariantToStringWithDefault   (PROPSYS.@)
 */
PCWSTR WINAPI VariantToStringWithDefault(const VARIANT *pvar, const WCHAR *default_value)
{
    TRACE("%s, %s.\n", debugstr_variant(pvar), debugstr_w(default_value));

    if (V_VT(pvar) == (VT_BYREF | VT_VARIANT)) pvar = V_VARIANTREF(pvar);
    if (V_VT(pvar) == (VT_BYREF | VT_BSTR) || V_VT(pvar) == VT_BSTR)
    {
        BSTR ret = V_ISBYREF(pvar) ? *V_BSTRREF(pvar) : V_BSTR(pvar);
        return ret ? ret : L"";
    }

    return default_value;
}

/******************************************************************
 *  VariantToString   (PROPSYS.@)
 */
HRESULT WINAPI VariantToString(REFVARIANT var, PWSTR ret, UINT cch)
{
    WCHAR buffer[64], *str = buffer;

    TRACE("%p, %p, %u.\n", var, ret, cch);

    *ret = 0;

    if (!cch)
        return E_INVALIDARG;

    switch (V_VT(var))
    {
        case VT_BSTR:
            str = V_BSTR(var);
            break;
        case VT_I4:
            swprintf(buffer, ARRAY_SIZE(buffer), L"%d", V_I4(var));
            break;
        default:
            FIXME("Unsupported type %d.\n", V_VT(var));
            return E_NOTIMPL;
    }

    if (wcslen(str) > cch - 1)
        return STRSAFE_E_INSUFFICIENT_BUFFER;
    wcscpy(ret, str);

    return S_OK;
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
        return PROPVAR_ConvertFILETIME(&propvarSrc->filetime, ppropvarDest, vt);

    switch (vt)
    {
    case VT_I1:
    {
        LONGLONG res;

        hr = PROPVAR_ConvertNumber(propvarSrc, 8, TRUE, &res);
        if (SUCCEEDED(hr))
        {
            ppropvarDest->vt = VT_I1;
            ppropvarDest->cVal = (char)res;
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
            ppropvarDest->bVal = (UCHAR)res;
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
            ppropvarDest->iVal = res;
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
            ppropvarDest->uiVal = res;
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
            ppropvarDest->lVal = res;
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
            ppropvarDest->ulVal = res;
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
            ppropvarDest->hVal.QuadPart = res;
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
            ppropvarDest->uhVal.QuadPart = res;
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
            ppropvarDest->pwszVal = res;
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
                ppropvarDest->pszVal = res;
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
HRESULT WINAPI InitPropVariantFromGUIDAsString(REFGUID guid, PROPVARIANT *ppropvar)
{
    TRACE("(%p %p)\n", guid, ppropvar);

    if(!guid)
        return E_FAIL;

    ppropvar->vt = VT_LPWSTR;
    ppropvar->pwszVal = CoTaskMemAlloc((GUID_STR_LEN + 1) * sizeof(WCHAR));
    if(!ppropvar->pwszVal)
        return E_OUTOFMEMORY;

    StringFromGUID2(guid, ppropvar->pwszVal, GUID_STR_LEN + 1);
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
    V_BSTR(pvar) = SysAllocStringLen(NULL, GUID_STR_LEN);
    if(!V_BSTR(pvar))
        return E_OUTOFMEMORY;

    StringFromGUID2(guid, V_BSTR(pvar), GUID_STR_LEN + 1);
    return S_OK;
}

HRESULT WINAPI InitPropVariantFromBuffer(const VOID *pv, UINT cb, PROPVARIANT *ppropvar)
{
    TRACE("(%p %u %p)\n", pv, cb, ppropvar);

    ppropvar->caub.pElems = CoTaskMemAlloc(cb);
    if(!ppropvar->caub.pElems)
        return E_OUTOFMEMORY;

    ppropvar->vt = VT_VECTOR|VT_UI1;
    ppropvar->caub.cElems = cb;
    memcpy(ppropvar->caub.pElems, pv, cb);
    return S_OK;
}

HRESULT WINAPI InitPropVariantFromCLSID(REFCLSID clsid, PROPVARIANT *ppropvar)
{
    TRACE("(%s %p)\n", debugstr_guid(clsid), ppropvar);

    ppropvar->puuid = CoTaskMemAlloc(sizeof(*ppropvar->puuid));
    if(!ppropvar->puuid)
        return E_OUTOFMEMORY;

    ppropvar->vt = VT_CLSID;
    memcpy(ppropvar->puuid, clsid, sizeof(*ppropvar->puuid));
    return S_OK;
}

HRESULT WINAPI InitPropVariantFromStringVector(PCWSTR *strs, ULONG count, PROPVARIANT *ppropvar)
{
    unsigned int i;

    TRACE("(%p %lu %p)\n", strs, count, ppropvar);

    ppropvar->calpwstr.pElems = CoTaskMemAlloc(count * sizeof(*ppropvar->calpwstr.pElems));
    if(!ppropvar->calpwstr.pElems)
        return E_OUTOFMEMORY;

    ppropvar->vt = VT_LPWSTR | VT_VECTOR;
    ppropvar->calpwstr.cElems = 0;
    if (count)
        memset(ppropvar->calpwstr.pElems, 0, count * sizeof(*ppropvar->calpwstr.pElems));

    for (i = 0; i < count; ++i)
    {
        if (strs[i])
        {
            if (!(ppropvar->calpwstr.pElems[i] = CoTaskMemAlloc((wcslen(strs[i]) + 1)*sizeof(**strs))))
            {
                PropVariantClear(ppropvar);
                return E_OUTOFMEMORY;
            }
        }
        wcscpy(ppropvar->calpwstr.pElems[i], strs[i]);
        ppropvar->calpwstr.cElems++;
    }

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

HRESULT WINAPI InitVariantFromFileTime(const FILETIME *ft, VARIANT *var)
{
    SYSTEMTIME st;

    TRACE("%p, %p\n", ft, var);

    VariantInit(var);
    if (!FileTimeToSystemTime(ft, &st))
        return E_INVALIDARG;
    if (!SystemTimeToVariantTime(&st, &V_DATE(var)))
        return E_INVALIDARG;
    V_VT(var) = VT_DATE;
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
        return PROPVAR_WCHARToGUID(ppropvar->bstrVal, SysStringLen(ppropvar->bstrVal), guid);
    case VT_LPWSTR:
        return PROPVAR_WCHARToGUID(ppropvar->pwszVal, lstrlenW(ppropvar->pwszVal), guid);
    case VT_CLSID:
        memcpy(guid, ppropvar->puuid, sizeof(*ppropvar->puuid));
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
        for (i=0; i<propvar->parray->cDims; i++)
        {
            if (propvar->parray->rgsabound[i].cElements != 0)
                break;
        }
        return i == propvar->parray->cDims;
    }
    if (propvar->vt == VT_CLSID)
        return !propvar->puuid;

    if (propvar->vt & VT_VECTOR)
        return !propvar->caub.cElems;

    /* FIXME: byrefs, errors? */
    return FALSE;
}

INT WINAPI PropVariantCompareEx(REFPROPVARIANT propvar1, REFPROPVARIANT propvar2,
    PROPVAR_COMPARE_UNIT unit, PROPVAR_COMPARE_FLAGS flags)
{
    const PROPVARIANT *propvar2_converted;
    PROPVARIANT propvar2_static;
    unsigned int count;
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
    if (propvar1->var > propvar2_converted->var) \
        res = 1; \
    else if (propvar1->var < propvar2_converted->var) \
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
        CMP_NUM_VALUE(ulVal);
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
            res = lstrcmpiW(propvar1->bstrVal, propvar2_converted->bstrVal);
        else
            res = lstrcmpW(propvar1->bstrVal, propvar2_converted->bstrVal);
        break;
    case VT_LPSTR:
        /* FIXME: Use other string flags. */
        if (flags & (PVCF_USESTRCMPI | PVCF_USESTRCMPIC))
            res = lstrcmpiA(propvar1->pszVal, propvar2_converted->pszVal);
        else
            res = lstrcmpA(propvar1->pszVal, propvar2_converted->pszVal);
        break;
    case VT_CLSID:
        res = memcmp(propvar1->puuid, propvar2->puuid, sizeof(*propvar1->puuid));
        if (res) res = res > 0 ? 1 : -1;
        break;
    case VT_VECTOR | VT_UI1:
        count = min(propvar1->caub.cElems, propvar2->caub.cElems);
        res = count ? memcmp(propvar1->caub.pElems, propvar2->caub.pElems, sizeof(*propvar1->caub.pElems) * count) : 0;
        if (res) res = res > 0 ? 1 : -1;
        if (!res && propvar1->caub.cElems != propvar2->caub.cElems)
            res = propvar1->caub.cElems > propvar2->caub.cElems ? 1 : -1;
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

HRESULT WINAPI PropVariantToVariant(const PROPVARIANT *propvar, VARIANT *var)
{
    HRESULT hr = S_OK;

    TRACE("propvar %p, var %p, propvar->vt %#x.\n", propvar, var, propvar ? propvar->vt : 0);

    if (!var || !propvar)
        return E_INVALIDARG;

    VariantInit(var);
#ifdef __REACTOS__
    V_VT(var) = propvar->vt;
#else
    var->vt = propvar->vt;
#endif

    switch (propvar->vt)
    {
        case VT_EMPTY:
        case VT_NULL:
            break;
        case VT_I1:
            V_I1(var) = propvar->cVal;
            break;
        case VT_I2:
            V_I2(var) = propvar->iVal;
            break;
        case VT_I4:
            V_I4(var) = propvar->lVal;
            break;
        case VT_I8:
            V_I8(var) = propvar->hVal.QuadPart;
            break;
        case VT_UI1:
            V_UI1(var) = propvar->bVal;
            break;
        case VT_UI2:
            V_UI2(var) = propvar->uiVal;
            break;
        case VT_UI4:
            V_UI4(var) = propvar->ulVal;
            break;
        case VT_UI8:
            V_UI8(var) = propvar->uhVal.QuadPart;
            break;
        case VT_BOOL:
            V_BOOL(var) = propvar->boolVal;
            break;
        case VT_R4:
            V_R4(var) = propvar->fltVal;
            break;
        case VT_R8:
            V_R8(var) = propvar->dblVal;
            break;
        case VT_LPSTR:
        case VT_LPWSTR:
        case VT_BSTR:
        case VT_CLSID:
#ifdef __REACTOS__
            V_VT(var) = VT_BSTR;
#else
            var->vt = VT_BSTR;
#endif
            hr = PropVariantToBSTR(propvar, &V_BSTR(var));
            break;
        default:
            FIXME("Unsupported type %d.\n", propvar->vt);
            return E_INVALIDARG;
    }

    return hr;
}

HRESULT WINAPI InitPropVariantFromFileTime(const FILETIME *pftIn, PROPVARIANT *ppropvar)
{
    TRACE("(%p, %p)\n", pftIn, ppropvar);
    if (!pftIn || !ppropvar) return E_INVALIDARG;
    ppropvar->vt = VT_FILETIME;
    ppropvar->filetime = *pftIn;
    return S_OK;
}

HRESULT WINAPI PropVariantToFileTime(REFPROPVARIANT propvarIn, PSTIME_FLAGS pstfOut, FILETIME *pftOut)
{
    TRACE("(%p, %d, %p)\n", propvarIn, pstfOut, pftOut);
    if (!pftOut) return E_INVALIDARG;
    if (propvarIn->vt != VT_FILETIME)
        return DISP_E_TYPEMISMATCH;
    if (pstfOut == PSTF_LOCAL)
        FileTimeToLocalFileTime(&propvarIn->filetime, pftOut);
    else
        *pftOut = propvarIn->filetime;
    return S_OK;
}

ULONG WINAPI PropVariantGetElementCount(REFPROPVARIANT propvar)
{
    TRACE("(%p)\n", propvar);
    if (!propvar || propvar->vt == VT_EMPTY || propvar->vt == VT_NULL)
        return 0;
    if (propvar->vt & VT_VECTOR)
        return propvar->caub.cElems;
    if (propvar->vt & VT_ARRAY)
    {
        ULONG count = 1;
        UINT i;
        if (!propvar->parray) return 0;
        for (i = 0; i < propvar->parray->cDims; i++)
            count *= propvar->parray->rgsabound[i].cElements;
        return count;
    }
    return 1;
}

HRESULT WINAPI InitPropVariantFromBooleanVector(const BOOL *prgf, ULONG cElems, PROPVARIANT *ppropvar)
{
    ULONG i;
    TRACE("(%p, %lu, %p)\n", prgf, cElems, ppropvar);
    if (!ppropvar) return E_INVALIDARG;
    PropVariantInit(ppropvar);
    ppropvar->vt = VT_VECTOR | VT_BOOL;
    if (!cElems) { ppropvar->cabool.cElems = 0; ppropvar->cabool.pElems = NULL; return S_OK; }
    if (!prgf) return E_INVALIDARG;
    ppropvar->cabool.pElems = CoTaskMemAlloc(cElems * sizeof(VARIANT_BOOL));
    if (!ppropvar->cabool.pElems) return E_OUTOFMEMORY;
    ppropvar->cabool.cElems = cElems;
    for (i = 0; i < cElems; i++)
        ppropvar->cabool.pElems[i] = prgf[i] ? VARIANT_TRUE : VARIANT_FALSE;
    return S_OK;
}

HRESULT WINAPI InitPropVariantFromInt16Vector(const SHORT *prgn, ULONG cElems, PROPVARIANT *ppropvar)
{
    TRACE("(%p, %lu, %p)\n", prgn, cElems, ppropvar);
    if (!ppropvar) return E_INVALIDARG;
    PropVariantInit(ppropvar);
    ppropvar->vt = VT_VECTOR | VT_I2;
    if (!cElems) { ppropvar->cai.cElems = 0; ppropvar->cai.pElems = NULL; return S_OK; }
    if (!prgn) return E_INVALIDARG;
    ppropvar->cai.pElems = CoTaskMemAlloc(cElems * sizeof(SHORT));
    if (!ppropvar->cai.pElems) return E_OUTOFMEMORY;
    ppropvar->cai.cElems = cElems;
    memcpy(ppropvar->cai.pElems, prgn, cElems * sizeof(SHORT));
    return S_OK;
}

HRESULT WINAPI InitPropVariantFromUInt16Vector(const USHORT *prgn, ULONG cElems, PROPVARIANT *ppropvar)
{
    TRACE("(%p, %lu, %p)\n", prgn, cElems, ppropvar);
    if (!ppropvar) return E_INVALIDARG;
    PropVariantInit(ppropvar);
    ppropvar->vt = VT_VECTOR | VT_UI2;
    if (!cElems) { ppropvar->caui.cElems = 0; ppropvar->caui.pElems = NULL; return S_OK; }
    if (!prgn) return E_INVALIDARG;
    ppropvar->caui.pElems = CoTaskMemAlloc(cElems * sizeof(USHORT));
    if (!ppropvar->caui.pElems) return E_OUTOFMEMORY;
    ppropvar->caui.cElems = cElems;
    memcpy(ppropvar->caui.pElems, prgn, cElems * sizeof(USHORT));
    return S_OK;
}

HRESULT WINAPI InitPropVariantFromInt32Vector(const LONG *prgn, ULONG cElems, PROPVARIANT *ppropvar)
{
    TRACE("(%p, %lu, %p)\n", prgn, cElems, ppropvar);
    if (!ppropvar) return E_INVALIDARG;
    PropVariantInit(ppropvar);
    ppropvar->vt = VT_VECTOR | VT_I4;
    if (!cElems) { ppropvar->cal.cElems = 0; ppropvar->cal.pElems = NULL; return S_OK; }
    if (!prgn) return E_INVALIDARG;
    ppropvar->cal.pElems = CoTaskMemAlloc(cElems * sizeof(LONG));
    if (!ppropvar->cal.pElems) return E_OUTOFMEMORY;
    ppropvar->cal.cElems = cElems;
    memcpy(ppropvar->cal.pElems, prgn, cElems * sizeof(LONG));
    return S_OK;
}

HRESULT WINAPI InitPropVariantFromUInt32Vector(const ULONG *prgn, ULONG cElems, PROPVARIANT *ppropvar)
{
    TRACE("(%p, %lu, %p)\n", prgn, cElems, ppropvar);
    if (!ppropvar) return E_INVALIDARG;
    PropVariantInit(ppropvar);
    ppropvar->vt = VT_VECTOR | VT_UI4;
    if (!cElems) { ppropvar->caul.cElems = 0; ppropvar->caul.pElems = NULL; return S_OK; }
    if (!prgn) return E_INVALIDARG;
    ppropvar->caul.pElems = CoTaskMemAlloc(cElems * sizeof(ULONG));
    if (!ppropvar->caul.pElems) return E_OUTOFMEMORY;
    ppropvar->caul.cElems = cElems;
    memcpy(ppropvar->caul.pElems, prgn, cElems * sizeof(ULONG));
    return S_OK;
}

HRESULT WINAPI InitPropVariantFromInt64Vector(const LONGLONG *prgn, ULONG cElems, PROPVARIANT *ppropvar)
{
    TRACE("(%p, %lu, %p)\n", prgn, cElems, ppropvar);
    if (!ppropvar) return E_INVALIDARG;
    PropVariantInit(ppropvar);
    ppropvar->vt = VT_VECTOR | VT_I8;
    if (!cElems) { ppropvar->cah.cElems = 0; ppropvar->cah.pElems = NULL; return S_OK; }
    if (!prgn) return E_INVALIDARG;
    ppropvar->cah.pElems = CoTaskMemAlloc(cElems * sizeof(LARGE_INTEGER));
    if (!ppropvar->cah.pElems) return E_OUTOFMEMORY;
    ppropvar->cah.cElems = cElems;
    memcpy(ppropvar->cah.pElems, prgn, cElems * sizeof(LONGLONG));
    return S_OK;
}

HRESULT WINAPI InitPropVariantFromUInt64Vector(const ULONGLONG *prgn, ULONG cElems, PROPVARIANT *ppropvar)
{
    TRACE("(%p, %lu, %p)\n", prgn, cElems, ppropvar);
    if (!ppropvar) return E_INVALIDARG;
    PropVariantInit(ppropvar);
    ppropvar->vt = VT_VECTOR | VT_UI8;
    if (!cElems) { ppropvar->cauh.cElems = 0; ppropvar->cauh.pElems = NULL; return S_OK; }
    if (!prgn) return E_INVALIDARG;
    ppropvar->cauh.pElems = CoTaskMemAlloc(cElems * sizeof(ULARGE_INTEGER));
    if (!ppropvar->cauh.pElems) return E_OUTOFMEMORY;
    ppropvar->cauh.cElems = cElems;
    memcpy(ppropvar->cauh.pElems, prgn, cElems * sizeof(ULONGLONG));
    return S_OK;
}

HRESULT WINAPI InitPropVariantFromDoubleVector(const DOUBLE *prgn, ULONG cElems, PROPVARIANT *ppropvar)
{
    TRACE("(%p, %lu, %p)\n", prgn, cElems, ppropvar);
    if (!ppropvar) return E_INVALIDARG;
    PropVariantInit(ppropvar);
    ppropvar->vt = VT_VECTOR | VT_R8;
    if (!cElems) { ppropvar->cadbl.cElems = 0; ppropvar->cadbl.pElems = NULL; return S_OK; }
    if (!prgn) return E_INVALIDARG;
    ppropvar->cadbl.pElems = CoTaskMemAlloc(cElems * sizeof(DOUBLE));
    if (!ppropvar->cadbl.pElems) return E_OUTOFMEMORY;
    ppropvar->cadbl.cElems = cElems;
    memcpy(ppropvar->cadbl.pElems, prgn, cElems * sizeof(DOUBLE));
    return S_OK;
}

HRESULT WINAPI InitPropVariantFromPropVariantVectorElem(REFPROPVARIANT propvarIn, ULONG iElem, PROPVARIANT *ppropvar)
{
    TRACE("(%p, %lu, %p)\n", propvarIn, iElem, ppropvar);
    if (!ppropvar) return E_INVALIDARG;
    PropVariantInit(ppropvar);
    if (iElem >= PropVariantGetElementCount(propvarIn)) return E_INVALIDARG;

    switch (propvarIn->vt & VT_TYPEMASK)
    {
    case VT_I2:   ppropvar->vt = VT_I2;   ppropvar->iVal    = (propvarIn->vt & VT_VECTOR) ? propvarIn->cai.pElems[iElem]    : propvarIn->iVal; break;
    case VT_UI2:  ppropvar->vt = VT_UI2;  ppropvar->uiVal   = (propvarIn->vt & VT_VECTOR) ? propvarIn->caui.pElems[iElem]   : propvarIn->uiVal; break;
    case VT_I4:   ppropvar->vt = VT_I4;   ppropvar->lVal    = (propvarIn->vt & VT_VECTOR) ? propvarIn->cal.pElems[iElem]    : propvarIn->lVal; break;
    case VT_UI4:  ppropvar->vt = VT_UI4;  ppropvar->ulVal   = (propvarIn->vt & VT_VECTOR) ? propvarIn->caul.pElems[iElem]   : propvarIn->ulVal; break;
    case VT_I8:   ppropvar->vt = VT_I8;   ppropvar->hVal    = (propvarIn->vt & VT_VECTOR) ? propvarIn->cah.pElems[iElem]    : propvarIn->hVal; break;
    case VT_UI8:  ppropvar->vt = VT_UI8;  ppropvar->uhVal   = (propvarIn->vt & VT_VECTOR) ? propvarIn->cauh.pElems[iElem]   : propvarIn->uhVal; break;
    case VT_R8:   ppropvar->vt = VT_R8;   ppropvar->dblVal  = (propvarIn->vt & VT_VECTOR) ? propvarIn->cadbl.pElems[iElem]  : propvarIn->dblVal; break;
    case VT_BOOL: ppropvar->vt = VT_BOOL; ppropvar->boolVal = (propvarIn->vt & VT_VECTOR) ? propvarIn->cabool.pElems[iElem] : propvarIn->boolVal; break;
    case VT_LPWSTR:
    {
        PCWSTR src = (propvarIn->vt & VT_VECTOR) ? propvarIn->calpwstr.pElems[iElem] : propvarIn->pwszVal;
        if (src)
        {
            DWORD len = (lstrlenW(src) + 1) * sizeof(WCHAR);
            ppropvar->pwszVal = CoTaskMemAlloc(len);
            if (!ppropvar->pwszVal) return E_OUTOFMEMORY;
            memcpy(ppropvar->pwszVal, src, len);
        }
        ppropvar->vt = VT_LPWSTR;
        break;
    }
    default:
        FIXME("unhandled vt %d\n", propvarIn->vt);
        return E_INVALIDARG;
    }
    return S_OK;
}

HRESULT WINAPI InitPropVariantVectorFromPropVariant(REFPROPVARIANT propvarSingle, PROPVARIANT *ppropvar)
{
    TRACE("(%p, %p)\n", propvarSingle, ppropvar);
    if (!ppropvar) return E_INVALIDARG;
    if (propvarSingle->vt & (VT_VECTOR | VT_ARRAY))
        return E_INVALIDARG;
    switch (propvarSingle->vt)
    {
    case VT_I2:   return InitPropVariantFromInt16Vector(&propvarSingle->iVal, 1, ppropvar);
    case VT_UI2:  return InitPropVariantFromUInt16Vector(&propvarSingle->uiVal, 1, ppropvar);
    case VT_I4:   return InitPropVariantFromInt32Vector(&propvarSingle->lVal, 1, ppropvar);
    case VT_UI4:  return InitPropVariantFromUInt32Vector(&propvarSingle->ulVal, 1, ppropvar);
    case VT_R8:   return InitPropVariantFromDoubleVector(&propvarSingle->dblVal, 1, ppropvar);
    case VT_I8:   return InitPropVariantFromInt64Vector(&propvarSingle->hVal.QuadPart, 1, ppropvar);
    case VT_UI8:  return InitPropVariantFromUInt64Vector(&propvarSingle->uhVal.QuadPart, 1, ppropvar);
    case VT_LPWSTR:
    {
        PCWSTR p = propvarSingle->pwszVal;
        return InitPropVariantFromStringVector(&p, 1, ppropvar);
    }
    case VT_BOOL:
    {
        BOOL b = propvarSingle->boolVal != VARIANT_FALSE;
        return InitPropVariantFromBooleanVector(&b, 1, ppropvar);
    }
    default:
        FIXME("unhandled vt %d\n", propvarSingle->vt);
        return E_INVALIDARG;
    }
}

HRESULT WINAPI InitPropVariantFromResource(HMODULE hinst, UINT id, PROPVARIANT *ppropvar)
{
    WCHAR buf[1024];
    int len;
    TRACE("(%p, %u, %p)\n", hinst, id, ppropvar);
    if (!ppropvar) return E_INVALIDARG;
    PropVariantInit(ppropvar);
    len = LoadStringW(hinst, id, buf, ARRAY_SIZE(buf));
    if (!len) return HRESULT_FROM_WIN32(GetLastError());
    ppropvar->pwszVal = CoTaskMemAlloc((len + 1) * sizeof(WCHAR));
    if (!ppropvar->pwszVal) return E_OUTOFMEMORY;
    memcpy(ppropvar->pwszVal, buf, (len + 1) * sizeof(WCHAR));
    ppropvar->vt = VT_LPWSTR;
    return S_OK;
}

BOOL WINAPI PropVariantToBooleanWithDefault(REFPROPVARIANT propvarIn, BOOL fDefault)
{
    BOOL result;
    TRACE("(%p, %d)\n", propvarIn, fDefault);
    if (SUCCEEDED(PropVariantToBoolean(propvarIn, &result))) return result;
    return fDefault;
}

LONG WINAPI PropVariantToInt32WithDefault(REFPROPVARIANT propvarIn, LONG lDefault)
{
    LONG result;
    TRACE("(%p, %ld)\n", propvarIn, lDefault);
    if (SUCCEEDED(PropVariantToInt32(propvarIn, &result))) return result;
    return lDefault;
}

LONGLONG WINAPI PropVariantToInt64WithDefault(REFPROPVARIANT propvarIn, LONGLONG llDefault)
{
    LONGLONG result;
    TRACE("(%p, %I64d)\n", propvarIn, llDefault);
    if (SUCCEEDED(PropVariantToInt64(propvarIn, &result))) return result;
    return llDefault;
}

USHORT WINAPI PropVariantToUInt16WithDefault(REFPROPVARIANT propvarIn, USHORT uiDefault)
{
    USHORT result;
    TRACE("(%p, %u)\n", propvarIn, uiDefault);
    if (SUCCEEDED(PropVariantToUInt16(propvarIn, &result))) return result;
    return uiDefault;
}

ULONGLONG WINAPI PropVariantToUInt64WithDefault(REFPROPVARIANT propvarIn, ULONGLONG ullDefault)
{
    ULONGLONG result;
    TRACE("(%p, %I64u)\n", propvarIn, ullDefault);
    if (SUCCEEDED(PropVariantToUInt64(propvarIn, &result))) return result;
    return ullDefault;
}

DOUBLE WINAPI PropVariantToDoubleWithDefault(REFPROPVARIANT propvarIn, DOUBLE dblDefault)
{
    DOUBLE result;
    TRACE("(%p, %g)\n", propvarIn, dblDefault);
    if (SUCCEEDED(PropVariantToDouble(propvarIn, &result))) return result;
    return dblDefault;
}

HRESULT WINAPI PropVariantGetStringElem(REFPROPVARIANT propvar, ULONG iElem, PWSTR *ppszVal)
{
    PCWSTR src;
    DWORD len;
    TRACE("(%p, %lu, %p)\n", propvar, iElem, ppszVal);
    if (!ppszVal) return E_INVALIDARG;
    *ppszVal = NULL;
    if (propvar->vt == (VT_VECTOR | VT_LPWSTR))
    {
        if (iElem >= propvar->calpwstr.cElems) return E_INVALIDARG;
        src = propvar->calpwstr.pElems[iElem];
    }
    else if (propvar->vt == (VT_VECTOR | VT_BSTR))
    {
        if (iElem >= propvar->cabstr.cElems) return E_INVALIDARG;
        src = propvar->cabstr.pElems[iElem];
    }
    else if ((propvar->vt == VT_LPWSTR || propvar->vt == VT_BSTR) && iElem == 0)
        src = propvar->pwszVal;
    else
        return DISP_E_TYPEMISMATCH;
    if (!src) return S_OK;
    len = (lstrlenW(src) + 1) * sizeof(WCHAR);
    *ppszVal = CoTaskMemAlloc(len);
    if (!*ppszVal) return E_OUTOFMEMORY;
    memcpy(*ppszVal, src, len);
    return S_OK;
}

HRESULT WINAPI PropVariantGetBooleanElem(REFPROPVARIANT propvar, ULONG iElem, BOOL *pfVal)
{
    TRACE("(%p, %lu, %p)\n", propvar, iElem, pfVal);
    if (!pfVal) return E_INVALIDARG;
    if (propvar->vt == (VT_VECTOR | VT_BOOL))
    {
        if (iElem >= propvar->cabool.cElems) return E_INVALIDARG;
        *pfVal = propvar->cabool.pElems[iElem] != VARIANT_FALSE;
        return S_OK;
    }
    if (propvar->vt == VT_BOOL && iElem == 0)
    {
        *pfVal = propvar->boolVal != VARIANT_FALSE;
        return S_OK;
    }
    return DISP_E_TYPEMISMATCH;
}

HRESULT WINAPI PropVariantGetInt16Elem(REFPROPVARIANT propvar, ULONG iElem, SHORT *pnVal)
{
    TRACE("(%p, %lu, %p)\n", propvar, iElem, pnVal);
    if (!pnVal) return E_INVALIDARG;
    if (propvar->vt == (VT_VECTOR | VT_I2)) { if (iElem >= propvar->cai.cElems) return E_INVALIDARG; *pnVal = propvar->cai.pElems[iElem]; return S_OK; }
    if (propvar->vt == VT_I2 && iElem == 0) { *pnVal = propvar->iVal; return S_OK; }
    return DISP_E_TYPEMISMATCH;
}

HRESULT WINAPI PropVariantGetUInt16Elem(REFPROPVARIANT propvar, ULONG iElem, USHORT *pnVal)
{
    TRACE("(%p, %lu, %p)\n", propvar, iElem, pnVal);
    if (!pnVal) return E_INVALIDARG;
    if (propvar->vt == (VT_VECTOR | VT_UI2)) { if (iElem >= propvar->caui.cElems) return E_INVALIDARG; *pnVal = propvar->caui.pElems[iElem]; return S_OK; }
    if (propvar->vt == VT_UI2 && iElem == 0) { *pnVal = propvar->uiVal; return S_OK; }
    return DISP_E_TYPEMISMATCH;
}

HRESULT WINAPI PropVariantGetInt32Elem(REFPROPVARIANT propvar, ULONG iElem, LONG *pnVal)
{
    TRACE("(%p, %lu, %p)\n", propvar, iElem, pnVal);
    if (!pnVal) return E_INVALIDARG;
    if (propvar->vt == (VT_VECTOR | VT_I4)) { if (iElem >= propvar->cal.cElems) return E_INVALIDARG; *pnVal = propvar->cal.pElems[iElem]; return S_OK; }
    if (propvar->vt == VT_I4 && iElem == 0) { *pnVal = propvar->lVal; return S_OK; }
    return DISP_E_TYPEMISMATCH;
}

HRESULT WINAPI PropVariantGetUInt32Elem(REFPROPVARIANT propvar, ULONG iElem, ULONG *pnVal)
{
    TRACE("(%p, %lu, %p)\n", propvar, iElem, pnVal);
    if (!pnVal) return E_INVALIDARG;
    if (propvar->vt == (VT_VECTOR | VT_UI4)) { if (iElem >= propvar->caul.cElems) return E_INVALIDARG; *pnVal = propvar->caul.pElems[iElem]; return S_OK; }
    if (propvar->vt == VT_UI4 && iElem == 0) { *pnVal = propvar->ulVal; return S_OK; }
    return DISP_E_TYPEMISMATCH;
}

HRESULT WINAPI PropVariantGetInt64Elem(REFPROPVARIANT propvar, ULONG iElem, LONGLONG *pnVal)
{
    TRACE("(%p, %lu, %p)\n", propvar, iElem, pnVal);
    if (!pnVal) return E_INVALIDARG;
    if (propvar->vt == (VT_VECTOR | VT_I8)) { if (iElem >= propvar->cah.cElems) return E_INVALIDARG; *pnVal = propvar->cah.pElems[iElem].QuadPart; return S_OK; }
    if (propvar->vt == VT_I8 && iElem == 0) { *pnVal = propvar->hVal.QuadPart; return S_OK; }
    return DISP_E_TYPEMISMATCH;
}

HRESULT WINAPI PropVariantGetUInt64Elem(REFPROPVARIANT propvar, ULONG iElem, ULONGLONG *pnVal)
{
    TRACE("(%p, %lu, %p)\n", propvar, iElem, pnVal);
    if (!pnVal) return E_INVALIDARG;
    if (propvar->vt == (VT_VECTOR | VT_UI8)) { if (iElem >= propvar->cauh.cElems) return E_INVALIDARG; *pnVal = propvar->cauh.pElems[iElem].QuadPart; return S_OK; }
    if (propvar->vt == VT_UI8 && iElem == 0) { *pnVal = propvar->uhVal.QuadPart; return S_OK; }
    return DISP_E_TYPEMISMATCH;
}

HRESULT WINAPI PropVariantGetDoubleElem(REFPROPVARIANT propvar, ULONG iElem, DOUBLE *pnVal)
{
    TRACE("(%p, %lu, %p)\n", propvar, iElem, pnVal);
    if (!pnVal) return E_INVALIDARG;
    if (propvar->vt == (VT_VECTOR | VT_R8)) { if (iElem >= propvar->cadbl.cElems) return E_INVALIDARG; *pnVal = propvar->cadbl.pElems[iElem]; return S_OK; }
    if (propvar->vt == VT_R8 && iElem == 0) { *pnVal = propvar->dblVal; return S_OK; }
    return DISP_E_TYPEMISMATCH;
}

HRESULT WINAPI PropVariantGetFileTimeElem(REFPROPVARIANT propvar, ULONG iElem, FILETIME *pftVal)
{
    TRACE("(%p, %lu, %p)\n", propvar, iElem, pftVal);
    if (!pftVal) return E_INVALIDARG;
    if (propvar->vt == (VT_VECTOR | VT_FILETIME)) { if (iElem >= propvar->cafiletime.cElems) return E_INVALIDARG; *pftVal = propvar->cafiletime.pElems[iElem]; return S_OK; }
    if (propvar->vt == VT_FILETIME && iElem == 0) { *pftVal = propvar->filetime; return S_OK; }
    return DISP_E_TYPEMISMATCH;
}

HRESULT WINAPI PropVariantToStringVectorAlloc(REFPROPVARIANT propvar, PWSTR **pprgsz, ULONG *pcElem)
{
    ULONG i, count;
    PWSTR *arr;
    TRACE("(%p, %p, %p)\n", propvar, pprgsz, pcElem);
    if (!pprgsz || !pcElem) return E_INVALIDARG;
    *pprgsz = NULL;
    *pcElem = 0;
    if (propvar->vt == (VT_VECTOR | VT_LPWSTR))
    {
        count = propvar->calpwstr.cElems;
        if (!count) return S_OK;
        arr = CoTaskMemAlloc(count * sizeof(*arr));
        if (!arr) return E_OUTOFMEMORY;
        for (i = 0; i < count; i++)
        {
            DWORD len = (lstrlenW(propvar->calpwstr.pElems[i]) + 1) * sizeof(WCHAR);
            arr[i] = CoTaskMemAlloc(len);
            if (!arr[i])
            {
                ULONG j;
                for (j = 0; j < i; j++) CoTaskMemFree(arr[j]);
                CoTaskMemFree(arr);
                return E_OUTOFMEMORY;
            }
            memcpy(arr[i], propvar->calpwstr.pElems[i], len);
        }
        *pprgsz = arr;
        *pcElem = count;
        return S_OK;
    }
    if (propvar->vt == VT_LPWSTR || propvar->vt == VT_BSTR)
    {
        PCWSTR src = (propvar->vt == VT_BSTR) ? propvar->bstrVal : propvar->pwszVal;
        DWORD len;
        if (!src) return S_OK;
        arr = CoTaskMemAlloc(sizeof(*arr));
        if (!arr) return E_OUTOFMEMORY;
        len = (lstrlenW(src) + 1) * sizeof(WCHAR);
        arr[0] = CoTaskMemAlloc(len);
        if (!arr[0]) { CoTaskMemFree(arr); return E_OUTOFMEMORY; }
        memcpy(arr[0], src, len);
        *pprgsz = arr;
        *pcElem = 1;
        return S_OK;
    }
    return DISP_E_TYPEMISMATCH;
}

void WINAPI ClearPropVariantArray(PROPVARIANT *rgPropVar, UINT cVars)
{
    UINT i;
    TRACE("(%p, %u)\n", rgPropVar, cVars);
    if (!rgPropVar) return;
    for (i = 0; i < cVars; i++)
        PropVariantClear(&rgPropVar[i]);
}

ULONG WINAPI VariantGetElementCount(REFVARIANT var)
{
    TRACE("(%p)\n", var);
    if (!var || V_VT(var) == VT_EMPTY || V_VT(var) == VT_NULL) return 0;
    if (V_VT(var) & VT_ARRAY)
    {
        ULONG count = 1;
        UINT i;
        if (!V_ARRAY(var)) return 0;
        for (i = 0; i < V_ARRAY(var)->cDims; i++)
            count *= V_ARRAY(var)->rgsabound[i].cElements;
        return count;
    }
    return 1;
}

HRESULT WINAPI VariantToPropVariant(const VARIANT *var, PROPVARIANT *propvar)
{
    HRESULT hr;

    TRACE("var %p, propvar %p.\n", debugstr_variant(var), propvar);

    if (!var || !propvar)
        return E_INVALIDARG;

#ifdef __REACTOS__
    if (FAILED(hr = VARIANT_ValidateType(V_VT(var))))
#else
    if (FAILED(hr = VARIANT_ValidateType(var->vt)))
#endif
        return hr;

    PropVariantInit(propvar);


#ifdef __REACTOS__
    propvar->vt = V_VT(var);
#else
    propvar->vt = var->vt;
#endif

#ifdef __REACTOS__
    switch (V_VT(var))
#else
    switch (var->vt)
#endif
    {
        case VT_EMPTY:
        case VT_NULL:
            break;
        case VT_I1:
            propvar->cVal = V_I1(var);
            break;
        case VT_I2:
            propvar->iVal = V_I2(var);
            break;
        case VT_I4:
            propvar->lVal = V_I4(var);
            break;
        case VT_I8:
            propvar->hVal.QuadPart = V_I8(var);
            break;
        case VT_UI1:
            propvar->bVal = V_UI1(var);
            break;
        case VT_UI2:
            propvar->uiVal = V_UI2(var);
            break;
        case VT_UI4:
            propvar->ulVal = V_UI4(var);
            break;
        case VT_UI8:
            propvar->uhVal.QuadPart = V_UI8(var);
            break;
        case VT_BOOL:
            propvar->boolVal = V_BOOL(var);
            break;
        case VT_R4:
            propvar->fltVal = V_R4(var);
            break;
        case VT_R8:
            propvar->dblVal = V_R8(var);
            break;
        case VT_BSTR:
            propvar->bstrVal = SysAllocString(V_BSTR(var));
            break;
        default:
#ifdef __REACTOS__
            FIXME("Unsupported type %d.\n", V_VT(var));
#else
            FIXME("Unsupported type %d.\n", var->vt);
#endif
            return E_INVALIDARG;
    }

    return S_OK;
}

HRESULT WINAPI InitPropVariantFromStrRet(STRRET *pstrret, PCUITEMID_CHILD pidl, PROPVARIANT *ppropvar)
{
    LPWSTR pszW;
    HRESULT hr;

    TRACE("(%p, %p, %p)\n", pstrret, pidl, ppropvar);

    if (!pstrret || !ppropvar)
        return E_INVALIDARG;

    PropVariantInit(ppropvar);

    hr = StrRetToStrW(pstrret, pidl, &pszW);
    if (FAILED(hr))
        return hr;

    ppropvar->vt = VT_LPWSTR;
    ppropvar->pwszVal = pszW;
    return S_OK;
}

HRESULT WINAPI PropVariantToStrRet(REFPROPVARIANT propvarIn, STRRET *pstrret)
{
    TRACE("(%p, %p)\n", propvarIn, pstrret);

    if (!pstrret)
        return E_INVALIDARG;

    switch (propvarIn->vt)
    {
        case VT_LPWSTR:
            pstrret->uType = STRRET_WSTR;
            return SHStrDupW(propvarIn->pwszVal, &pstrret->pOleStr);

        case VT_BSTR:
            pstrret->uType = STRRET_WSTR;
            return SHStrDupW(propvarIn->bstrVal, &pstrret->pOleStr);

        case VT_EMPTY:
        case VT_NULL:
            pstrret->uType = STRRET_WSTR;
            return SHStrDupW(L"", &pstrret->pOleStr);

        default:
            FIXME("Unsupported type %d\n", propvarIn->vt);
            return E_FAIL;
    }
}

HRESULT WINAPI InitVariantFromStrRet(STRRET *pstrret, PCUITEMID_CHILD pidl, VARIANT *pvar)
{
    LPWSTR pszW;
    HRESULT hr;

    TRACE("(%p, %p, %p)\n", pstrret, pidl, pvar);

    if (!pstrret || !pvar)
        return E_INVALIDARG;

    VariantInit(pvar);

    hr = StrRetToStrW(pstrret, pidl, &pszW);
    if (FAILED(hr))
        return hr;

    V_VT(pvar) = VT_BSTR;
    V_BSTR(pvar) = SysAllocString(pszW);
    CoTaskMemFree(pszW);
    if (!V_BSTR(pvar))
        return E_OUTOFMEMORY;

    return S_OK;
}

HRESULT WINAPI VariantToStrRet(REFVARIANT pvar, STRRET *pstrret)
{
    TRACE("(%p, %p)\n", pvar, pstrret);

    if (!pstrret)
        return E_INVALIDARG;

    switch (V_VT(pvar))
    {
        case VT_BSTR:
            pstrret->uType = STRRET_WSTR;
            return SHStrDupW(V_BSTR(pvar) ? V_BSTR(pvar) : L"", &pstrret->pOleStr);

        case VT_LPWSTR:
            pstrret->uType = STRRET_WSTR;
            return SHStrDupW(V_BSTR(pvar) ? V_BSTR(pvar) : L"", &pstrret->pOleStr);

        case VT_EMPTY:
        case VT_NULL:
            pstrret->uType = STRRET_WSTR;
            return SHStrDupW(L"", &pstrret->pOleStr);

        default:
            FIXME("Unsupported type %d\n", V_VT(pvar));
            return E_FAIL;
    }
}

void WINAPI ClearVariantArray(VARIANT *rgVar, UINT cVars)
{
    UINT i;
    TRACE("(%p, %u)\n", rgVar, cVars);
    if (!rgVar) return;
    for (i = 0; i < cVars; i++)
        VariantClear(&rgVar[i]);
}

HRESULT WINAPI VariantToBoolean(REFVARIANT pvar, BOOL *pfRet)
{
    VARIANT dest;
    HRESULT hr;

    TRACE("(%p, %p)\n", pvar, pfRet);

    if (!pfRet) return E_INVALIDARG;
    *pfRet = FALSE;

    if (V_VT(pvar) == VT_BOOL)
    {
        *pfRet = V_BOOL(pvar) != VARIANT_FALSE;
        return S_OK;
    }

    VariantInit(&dest);
    hr = VariantChangeType(&dest, (VARIANT *)pvar, 0, VT_BOOL);
    if (SUCCEEDED(hr))
        *pfRet = V_BOOL(&dest) != VARIANT_FALSE;
    return hr;
}

BOOL WINAPI VariantToBooleanWithDefault(REFVARIANT pvar, BOOL fDefault)
{
    BOOL result;
    TRACE("(%p, %d)\n", pvar, fDefault);
    if (SUCCEEDED(VariantToBoolean(pvar, &result))) return result;
    return fDefault;
}

HRESULT WINAPI VariantToDouble(REFVARIANT pvar, double *pdblRet)
{
    VARIANT dest;
    HRESULT hr;

    TRACE("(%p, %p)\n", pvar, pdblRet);

    if (!pdblRet) return E_INVALIDARG;
    *pdblRet = 0;

    if (V_VT(pvar) == VT_R8)
    {
        *pdblRet = V_R8(pvar);
        return S_OK;
    }

    VariantInit(&dest);
    hr = VariantChangeType(&dest, (VARIANT *)pvar, 0, VT_R8);
    if (SUCCEEDED(hr))
        *pdblRet = V_R8(&dest);
    return hr;
}

DOUBLE WINAPI VariantToDoubleWithDefault(REFVARIANT pvar, DOUBLE dblDefault)
{
    DOUBLE result;
    TRACE("(%p, %g)\n", pvar, dblDefault);
    if (SUCCEEDED(VariantToDouble(pvar, &result))) return result;
    return dblDefault;
}

HRESULT WINAPI VariantToInt16(REFVARIANT pvar, SHORT *piRet)
{
    VARIANT dest;
    HRESULT hr;

    TRACE("(%p, %p)\n", pvar, piRet);

    if (!piRet) return E_INVALIDARG;
    *piRet = 0;

    if (V_VT(pvar) == VT_I2)
    {
        *piRet = V_I2(pvar);
        return S_OK;
    }

    VariantInit(&dest);
    hr = VariantChangeType(&dest, (VARIANT *)pvar, 0, VT_I2);
    if (SUCCEEDED(hr))
        *piRet = V_I2(&dest);
    return hr;
}

SHORT WINAPI VariantToInt16WithDefault(REFVARIANT pvar, SHORT iDefault)
{
    SHORT result;
    TRACE("(%p, %d)\n", pvar, iDefault);
    if (SUCCEEDED(VariantToInt16(pvar, &result))) return result;
    return iDefault;
}

HRESULT WINAPI VariantToInt32(REFVARIANT pvar, LONG *plRet)
{
    VARIANT dest;
    HRESULT hr;

    TRACE("(%p, %p)\n", pvar, plRet);

    if (!plRet) return E_INVALIDARG;
    *plRet = 0;

    if (V_VT(pvar) == VT_I4)
    {
        *plRet = V_I4(pvar);
        return S_OK;
    }

    VariantInit(&dest);
    hr = VariantChangeType(&dest, (VARIANT *)pvar, 0, VT_I4);
    if (SUCCEEDED(hr))
        *plRet = V_I4(&dest);
    return hr;
}

LONG WINAPI VariantToInt32WithDefault(REFVARIANT pvar, LONG lDefault)
{
    LONG result;
    TRACE("(%p, %ld)\n", pvar, lDefault);
    if (SUCCEEDED(VariantToInt32(pvar, &result))) return result;
    return lDefault;
}

HRESULT WINAPI VariantToInt64(REFVARIANT pvar, LONGLONG *pllRet)
{
    VARIANT dest;
    HRESULT hr;

    TRACE("(%p, %p)\n", pvar, pllRet);

    if (!pllRet) return E_INVALIDARG;
    *pllRet = 0;

    if (V_VT(pvar) == VT_I8)
    {
        *pllRet = V_I8(pvar);
        return S_OK;
    }

    VariantInit(&dest);
    hr = VariantChangeType(&dest, (VARIANT *)pvar, 0, VT_I8);
    if (SUCCEEDED(hr))
        *pllRet = V_I8(&dest);
    return hr;
}

LONGLONG WINAPI VariantToInt64WithDefault(REFVARIANT pvar, LONGLONG llDefault)
{
    LONGLONG result;
    TRACE("(%p, %I64d)\n", pvar, llDefault);
    if (SUCCEEDED(VariantToInt64(pvar, &result))) return result;
    return llDefault;
}

HRESULT WINAPI VariantToUInt16(REFVARIANT pvar, USHORT *puiRet)
{
    VARIANT dest;
    HRESULT hr;

    TRACE("(%p, %p)\n", pvar, puiRet);

    if (!puiRet) return E_INVALIDARG;
    *puiRet = 0;

    if (V_VT(pvar) == VT_UI2)
    {
        *puiRet = V_UI2(pvar);
        return S_OK;
    }

    VariantInit(&dest);
    hr = VariantChangeType(&dest, (VARIANT *)pvar, 0, VT_UI2);
    if (SUCCEEDED(hr))
        *puiRet = V_UI2(&dest);
    return hr;
}

USHORT WINAPI VariantToUInt16WithDefault(REFVARIANT pvar, USHORT uiDefault)
{
    USHORT result;
    TRACE("(%p, %u)\n", pvar, uiDefault);
    if (SUCCEEDED(VariantToUInt16(pvar, &result))) return result;
    return uiDefault;
}

HRESULT WINAPI VariantToUInt32(REFVARIANT pvar, ULONG *pulRet)
{
    VARIANT dest;
    HRESULT hr;

    TRACE("(%p, %p)\n", pvar, pulRet);

    if (!pulRet) return E_INVALIDARG;
    *pulRet = 0;

    if (V_VT(pvar) == VT_UI4)
    {
        *pulRet = V_UI4(pvar);
        return S_OK;
    }

    VariantInit(&dest);
    hr = VariantChangeType(&dest, (VARIANT *)pvar, 0, VT_UI4);
    if (SUCCEEDED(hr))
        *pulRet = V_UI4(&dest);
    return hr;
}

ULONG WINAPI VariantToUInt32WithDefault(REFVARIANT pvar, ULONG ulDefault)
{
    ULONG result;
    TRACE("(%p, %lu)\n", pvar, ulDefault);
    if (SUCCEEDED(VariantToUInt32(pvar, &result))) return result;
    return ulDefault;
}

HRESULT WINAPI VariantToUInt64(REFVARIANT pvar, ULONGLONG *pullRet)
{
    VARIANT dest;
    HRESULT hr;

    TRACE("(%p, %p)\n", pvar, pullRet);

    if (!pullRet) return E_INVALIDARG;
    *pullRet = 0;

    if (V_VT(pvar) == VT_UI8)
    {
        *pullRet = V_UI8(pvar);
        return S_OK;
    }

    VariantInit(&dest);
    hr = VariantChangeType(&dest, (VARIANT *)pvar, 0, VT_UI8);
    if (SUCCEEDED(hr))
        *pullRet = V_UI8(&dest);
    return hr;
}

ULONGLONG WINAPI VariantToUInt64WithDefault(REFVARIANT pvar, ULONGLONG ullDefault)
{
    ULONGLONG result;
    TRACE("(%p, %I64u)\n", pvar, ullDefault);
    if (SUCCEEDED(VariantToUInt64(pvar, &result))) return result;
    return ullDefault;
}

HRESULT WINAPI VariantToStringAlloc(REFVARIANT pvar, WCHAR **ppszBuf)
{
    VARIANT dest;
    HRESULT hr;
    const WCHAR *src;
    DWORD len;

    TRACE("(%p, %p)\n", pvar, ppszBuf);

    if (!ppszBuf) return E_INVALIDARG;
    *ppszBuf = NULL;

    if (V_VT(pvar) == VT_BSTR)
    {
        src = V_BSTR(pvar) ? V_BSTR(pvar) : L"";
        len = (lstrlenW(src) + 1) * sizeof(WCHAR);
        *ppszBuf = CoTaskMemAlloc(len);
        if (!*ppszBuf) return E_OUTOFMEMORY;
        memcpy(*ppszBuf, src, len);
        return S_OK;
    }

    VariantInit(&dest);
    hr = VariantChangeType(&dest, (VARIANT *)pvar, 0, VT_BSTR);
    if (FAILED(hr)) return hr;

    src = V_BSTR(&dest) ? V_BSTR(&dest) : L"";
    len = (lstrlenW(src) + 1) * sizeof(WCHAR);
    *ppszBuf = CoTaskMemAlloc(len);
    if (*ppszBuf)
        memcpy(*ppszBuf, src, len);
    else
        hr = E_OUTOFMEMORY;

    VariantClear(&dest);
    return hr;
}

HRESULT WINAPI VariantToFileTime(REFVARIANT pvar, PSTIME_FLAGS stfOut, FILETIME *pftOut)
{
    TRACE("(%p, %d, %p)\n", pvar, stfOut, pftOut);

    if (!pftOut) return E_INVALIDARG;

    if (V_VT(pvar) == VT_DATE)
    {
        SYSTEMTIME st;
        FILETIME ft;
        if (!VariantTimeToSystemTime(V_DATE(pvar), &st))
            return E_FAIL;
        if (!SystemTimeToFileTime(&st, &ft))
            return E_FAIL;
        if (stfOut == PSTF_LOCAL)
            LocalFileTimeToFileTime(&ft, pftOut);
        else
            *pftOut = ft;
        return S_OK;
    }

    FIXME("Unsupported type %d\n", V_VT(pvar));
    return DISP_E_TYPEMISMATCH;
}

HRESULT WINAPI PropVariantToStringVector(REFPROPVARIANT propvar, PWSTR *prgsz, ULONG crgsz)
{
    PWSTR *arr;
    ULONG i, count;
    HRESULT hr;

    TRACE("(%p, %p, %lu)\n", propvar, prgsz, crgsz);

    if (!prgsz || !crgsz) return E_INVALIDARG;

    hr = PropVariantToStringVectorAlloc(propvar, &arr, &count);
    if (FAILED(hr)) return hr;

    count = min(count, crgsz);
    for (i = 0; i < count; i++)
    {
        prgsz[i] = arr[i];
        arr[i] = NULL;
    }
    for (i = count; i < crgsz; i++)
        prgsz[i] = NULL;

    CoTaskMemFree(arr);
    return S_OK;
}

HRESULT WINAPI PSFormatForDisplayAlloc(REFPROPERTYKEY key, REFPROPVARIANT propvar,
    PROPDESC_FORMAT_FLAGS flags, WCHAR **ppszDisplay)
{
    WCHAR buf[256];
    HRESULT hr;

    TRACE("(%p, %p, %#x, %p)\n", key, propvar, flags, ppszDisplay);

    if (!ppszDisplay) return E_INVALIDARG;
    *ppszDisplay = NULL;

    hr = PSFormatForDisplay(key, propvar, flags, buf, ARRAY_SIZE(buf));
    if (FAILED(hr)) return hr;

    return SHStrDupW(buf, ppszDisplay);
}

HRESULT WINAPI PSFormatForDisplay(REFPROPERTYKEY key, REFPROPVARIANT propvar,
    PROPDESC_FORMAT_FLAGS flags, WCHAR *pszDisplay, DWORD cchDisplay)
{
    WCHAR buf[256];
    const WCHAR *psz = buf;

    TRACE("(%p, %p, %#x, %p, %lu)\n", key, propvar, flags, pszDisplay, cchDisplay);

    if (!pszDisplay || !cchDisplay) return E_INVALIDARG;
    pszDisplay[0] = 0;

    switch (propvar->vt)
    {
        case VT_EMPTY:
        case VT_NULL:
            buf[0] = 0;
            break;
        case VT_BOOL:
            psz = (propvar->boolVal != VARIANT_FALSE) ? L"Yes" : L"No";
            break;
        case VT_I2:
            swprintf(buf, ARRAY_SIZE(buf), L"%d", propvar->iVal);
            break;
        case VT_UI2:
            swprintf(buf, ARRAY_SIZE(buf), L"%u", propvar->uiVal);
            break;
        case VT_I4:
            swprintf(buf, ARRAY_SIZE(buf), L"%d", propvar->lVal);
            break;
        case VT_UI4:
            swprintf(buf, ARRAY_SIZE(buf), L"%u", propvar->ulVal);
            break;
        case VT_I8:
            swprintf(buf, ARRAY_SIZE(buf), L"%I64d", propvar->hVal.QuadPart);
            break;
        case VT_UI8:
            swprintf(buf, ARRAY_SIZE(buf), L"%I64u", propvar->uhVal.QuadPart);
            break;
        case VT_R4:
            swprintf(buf, ARRAY_SIZE(buf), L"%g", (double)propvar->fltVal);
            break;
        case VT_R8:
            swprintf(buf, ARRAY_SIZE(buf), L"%g", propvar->dblVal);
            break;
        case VT_LPWSTR:
        case VT_BSTR:
            psz = propvar->pwszVal ? propvar->pwszVal : L"";
            break;
        case VT_LPSTR:
        {
            if (propvar->pszVal)
                MultiByteToWideChar(CP_ACP, 0, propvar->pszVal, -1, buf, ARRAY_SIZE(buf));
            else
                buf[0] = 0;
            break;
        }
        case VT_FILETIME:
        {
            SYSTEMTIME st;
            FILETIME local;
            FileTimeToLocalFileTime(&propvar->filetime, &local);
            FileTimeToSystemTime(&local, &st);
            swprintf(buf, ARRAY_SIZE(buf), L"%04d-%02d-%02d %02d:%02d:%02d",
                     st.wYear, st.wMonth, st.wDay,
                     st.wHour, st.wMinute, st.wSecond);
            break;
        }
        case VT_CLSID:
            if (propvar->puuid)
                StringFromGUID2(propvar->puuid, buf, ARRAY_SIZE(buf));
            else
                buf[0] = 0;
            break;
        default:
            FIXME("Unhandled type %d for key %s\n", propvar->vt, debugstr_guid(&key->fmtid));
            return E_NOTIMPL;
    }

    if (wcslen(psz) >= cchDisplay)
        return HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);

    wcscpy(pszDisplay, psz);
    return S_OK;
}
