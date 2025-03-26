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
