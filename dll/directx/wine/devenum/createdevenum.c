/*
 *	ICreateDevEnum implementation for DEVENUM.dll
 *
 * Copyright (C) 2002 Robert Shearman
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
 *
 * NOTES ON THIS FILE:
 * - Implements ICreateDevEnum interface which creates an IEnumMoniker
 *   implementation
 * - Also creates the special registry keys created at run-time
 */

#define NONAMELESSSTRUCT
#define NONAMELESSUNION

#include "devenum_private.h"
#include "vfw.h"
#include "aviriff.h"
#include "dsound.h"

#include "wine/debug.h"
#include "wine/unicode.h"
#include "wine/heap.h"
#include "mmddk.h"

#include "initguid.h"
#include "fil_data.h"

WINE_DEFAULT_DEBUG_CHANNEL(devenum);

extern HINSTANCE DEVENUM_hInstance;

static const WCHAR wszFilterKeyName[] = {'F','i','l','t','e','r',0};
static const WCHAR wszMeritName[] = {'M','e','r','i','t',0};
static const WCHAR wszPins[] = {'P','i','n','s',0};
static const WCHAR wszAllowedMany[] = {'A','l','l','o','w','e','d','M','a','n','y',0};
static const WCHAR wszAllowedZero[] = {'A','l','l','o','w','e','d','Z','e','r','o',0};
static const WCHAR wszDirection[] = {'D','i','r','e','c','t','i','o','n',0};
static const WCHAR wszIsRendered[] = {'I','s','R','e','n','d','e','r','e','d',0};
static const WCHAR wszTypes[] = {'T','y','p','e','s',0};
static const WCHAR wszFriendlyName[] = {'F','r','i','e','n','d','l','y','N','a','m','e',0};
static const WCHAR wszFilterData[] = {'F','i','l','t','e','r','D','a','t','a',0};

static ULONG WINAPI DEVENUM_ICreateDevEnum_AddRef(ICreateDevEnum * iface);
static HRESULT register_codecs(void);
static HRESULT DEVENUM_CreateAMCategoryKey(const CLSID * clsidCategory);

/**********************************************************************
 * DEVENUM_ICreateDevEnum_QueryInterface (also IUnknown)
 */
static HRESULT WINAPI DEVENUM_ICreateDevEnum_QueryInterface(ICreateDevEnum *iface, REFIID riid,
        void **ppv)
{
    TRACE("(%p)->(%s, %p)\n", iface, debugstr_guid(riid), ppv);

    if (!ppv)
        return E_POINTER;

    if (IsEqualGUID(riid, &IID_IUnknown) ||
	IsEqualGUID(riid, &IID_ICreateDevEnum))
    {
        *ppv = iface;
	DEVENUM_ICreateDevEnum_AddRef(iface);
	return S_OK;
    }

    FIXME("- no interface IID: %s\n", debugstr_guid(riid));
    *ppv = NULL;
    return E_NOINTERFACE;
}

/**********************************************************************
 * DEVENUM_ICreateDevEnum_AddRef (also IUnknown)
 */
static ULONG WINAPI DEVENUM_ICreateDevEnum_AddRef(ICreateDevEnum * iface)
{
    TRACE("\n");

    DEVENUM_LockModule();

    return 2; /* non-heap based object */
}

/**********************************************************************
 * DEVENUM_ICreateDevEnum_Release (also IUnknown)
 */
static ULONG WINAPI DEVENUM_ICreateDevEnum_Release(ICreateDevEnum * iface)
{
    TRACE("\n");

    DEVENUM_UnlockModule();

    return 1; /* non-heap based object */
}

static HRESULT register_codec(const CLSID *class, const WCHAR *name, IMoniker **ret)
{
    static const WCHAR deviceW[] = {'@','d','e','v','i','c','e',':','c','m',':',0};
    IParseDisplayName *parser;
    WCHAR *buffer;
    ULONG eaten;
    HRESULT hr;

    hr = CoCreateInstance(&CLSID_CDeviceMoniker, NULL, CLSCTX_INPROC, &IID_IParseDisplayName, (void **)&parser);
    if (FAILED(hr))
        return hr;

    buffer = heap_alloc((strlenW(deviceW) + CHARS_IN_GUID + strlenW(name) + 1) * sizeof(WCHAR));
    if (!buffer)
    {
        IParseDisplayName_Release(parser);
        return E_OUTOFMEMORY;
    }

    strcpyW(buffer, deviceW);
    StringFromGUID2(class, buffer + strlenW(buffer), CHARS_IN_GUID);
    strcatW(buffer, backslashW);
    strcatW(buffer, name);

    hr = IParseDisplayName_ParseDisplayName(parser, NULL, buffer, &eaten, ret);
    IParseDisplayName_Release(parser);
    heap_free(buffer);
    return hr;
}

static void DEVENUM_ReadPinTypes(HKEY hkeyPinKey, REGFILTERPINS2 *rgPin)
{
    HKEY hkeyTypes = NULL;
    DWORD dwMajorTypes, i;
    REGPINTYPES *lpMediaType = NULL;
    DWORD dwMediaTypeSize = 0;

    if (RegOpenKeyExW(hkeyPinKey, wszTypes, 0, KEY_READ, &hkeyTypes) != ERROR_SUCCESS)
        return ;

    if (RegQueryInfoKeyW(hkeyTypes, NULL, NULL, NULL, &dwMajorTypes, NULL, NULL, NULL, NULL, NULL, NULL, NULL)
                != ERROR_SUCCESS)
    {
        RegCloseKey(hkeyTypes);
        return ;
    }

    for (i = 0; i < dwMajorTypes; i++)
    {
        HKEY hkeyMajorType = NULL;
        WCHAR wszMajorTypeName[64];
        DWORD cName = sizeof(wszMajorTypeName) / sizeof(WCHAR);
        DWORD dwMinorTypes, i1;

        if (RegEnumKeyExW(hkeyTypes, i, wszMajorTypeName, &cName, NULL, NULL, NULL, NULL) != ERROR_SUCCESS) continue;

        if (RegOpenKeyExW(hkeyTypes, wszMajorTypeName, 0, KEY_READ, &hkeyMajorType) != ERROR_SUCCESS) continue;

        if (RegQueryInfoKeyW(hkeyMajorType, NULL, NULL, NULL, &dwMinorTypes, NULL, NULL, NULL, NULL, NULL, NULL, NULL)
                    != ERROR_SUCCESS)
        {
            RegCloseKey(hkeyMajorType);
            continue;
        }

        for (i1 = 0; i1 < dwMinorTypes; i1++)
        {
            WCHAR wszMinorTypeName[64];
            CLSID *clsMajorType = NULL, *clsMinorType = NULL;
            HRESULT hr;

            cName = sizeof(wszMinorTypeName) / sizeof(WCHAR);
            if (RegEnumKeyExW(hkeyMajorType, i1, wszMinorTypeName, &cName, NULL, NULL, NULL, NULL) != ERROR_SUCCESS) continue;

            clsMinorType = CoTaskMemAlloc(sizeof(CLSID));
            if (!clsMinorType) continue;

            clsMajorType = CoTaskMemAlloc(sizeof(CLSID));
            if (!clsMajorType) goto error_cleanup_types;

            hr = CLSIDFromString(wszMinorTypeName, clsMinorType);
            if (FAILED(hr)) goto error_cleanup_types;

            hr = CLSIDFromString(wszMajorTypeName, clsMajorType);
            if (FAILED(hr)) goto error_cleanup_types;

            if (rgPin->nMediaTypes == dwMediaTypeSize)
            {
                DWORD dwNewSize = dwMediaTypeSize + (dwMediaTypeSize < 2 ? 1 : dwMediaTypeSize / 2);
                REGPINTYPES *lpNewMediaType;

                lpNewMediaType = CoTaskMemRealloc(lpMediaType, sizeof(REGPINTYPES) * dwNewSize);
                if (!lpNewMediaType) goto error_cleanup_types;

                lpMediaType = lpNewMediaType;
                dwMediaTypeSize = dwNewSize;
             }

            lpMediaType[rgPin->nMediaTypes].clsMajorType = clsMajorType;
            lpMediaType[rgPin->nMediaTypes].clsMinorType = clsMinorType;
            rgPin->nMediaTypes++;
            continue;

            error_cleanup_types:

            CoTaskMemFree(clsMajorType);
            CoTaskMemFree(clsMinorType);
        }

        RegCloseKey(hkeyMajorType);
    }

    RegCloseKey(hkeyTypes);

    if (lpMediaType && !rgPin->nMediaTypes)
    {
        CoTaskMemFree(lpMediaType);
        lpMediaType = NULL;
    }

    rgPin->lpMediaType = lpMediaType;
}

static void DEVENUM_ReadPins(HKEY hkeyFilterClass, REGFILTER2 *rgf2)
{
    HKEY hkeyPins = NULL;
    DWORD dwPinsSubkeys, i;
    REGFILTERPINS2 *rgPins = NULL;

    rgf2->dwVersion = 2;
    rgf2->u.s2.cPins2 = 0;
    rgf2->u.s2.rgPins2 = NULL;

    if (RegOpenKeyExW(hkeyFilterClass, wszPins, 0, KEY_READ, &hkeyPins) != ERROR_SUCCESS)
        return ;

    if (RegQueryInfoKeyW(hkeyPins, NULL, NULL, NULL, &dwPinsSubkeys, NULL, NULL, NULL, NULL, NULL, NULL, NULL)
                != ERROR_SUCCESS)
    {
        RegCloseKey(hkeyPins);
        return ;
    }

    if (dwPinsSubkeys)
    {
        rgPins = CoTaskMemAlloc(sizeof(REGFILTERPINS2) * dwPinsSubkeys);
        if (!rgPins)
        {
            RegCloseKey(hkeyPins);
            return ;
        }
    }

    for (i = 0; i < dwPinsSubkeys; i++)
    {
        HKEY hkeyPinKey = NULL;
        WCHAR wszPinName[MAX_PATH];
        DWORD cName = sizeof(wszPinName) / sizeof(WCHAR);
        REGFILTERPINS2 *rgPin = &rgPins[rgf2->u.s2.cPins2];
        DWORD value, size, Type;
        LONG lRet;

        memset(rgPin, 0, sizeof(*rgPin));

        if (RegEnumKeyExW(hkeyPins, i, wszPinName, &cName, NULL, NULL, NULL, NULL) != ERROR_SUCCESS) continue;

        if (RegOpenKeyExW(hkeyPins, wszPinName, 0, KEY_READ, &hkeyPinKey) != ERROR_SUCCESS) continue;

        size = sizeof(DWORD);
        lRet = RegQueryValueExW(hkeyPinKey, wszAllowedMany, NULL, &Type, (BYTE *)&value, &size);
        if (lRet != ERROR_SUCCESS || Type != REG_DWORD)
            goto error_cleanup;
        if (value)
            rgPin->dwFlags |= REG_PINFLAG_B_MANY;

        size = sizeof(DWORD);
        lRet = RegQueryValueExW(hkeyPinKey, wszAllowedZero, NULL, &Type, (BYTE *)&value, &size);
        if (lRet != ERROR_SUCCESS || Type != REG_DWORD)
            goto error_cleanup;
        if (value)
            rgPin->dwFlags |= REG_PINFLAG_B_ZERO;

        size = sizeof(DWORD);
        lRet = RegQueryValueExW(hkeyPinKey, wszDirection, NULL, &Type, (BYTE *)&value, &size);
        if (lRet != ERROR_SUCCESS || Type != REG_DWORD)
            goto error_cleanup;
        if (value)
            rgPin->dwFlags |= REG_PINFLAG_B_OUTPUT;


        size = sizeof(DWORD);
        lRet = RegQueryValueExW(hkeyPinKey, wszIsRendered, NULL, &Type, (BYTE *)&value, &size);
        if (lRet != ERROR_SUCCESS || Type != REG_DWORD)
            goto error_cleanup;
        if (value)
            rgPin->dwFlags |= REG_PINFLAG_B_RENDERER;

        DEVENUM_ReadPinTypes(hkeyPinKey, rgPin);

        ++rgf2->u.s2.cPins2;
        continue;

        error_cleanup:

        RegCloseKey(hkeyPinKey);
    }

    RegCloseKey(hkeyPins);

    if (rgPins && !rgf2->u.s2.cPins2)
    {
        CoTaskMemFree(rgPins);
        rgPins = NULL;
    }

    rgf2->u.s2.rgPins2 = rgPins;
}

static void free_regfilter2(REGFILTER2 *rgf)
{
    if (rgf->u.s2.rgPins2)
    {
        UINT iPin;

        for (iPin = 0; iPin < rgf->u.s2.cPins2; iPin++)
        {
            if (rgf->u.s2.rgPins2[iPin].lpMediaType)
            {
                UINT iType;

                for (iType = 0; iType < rgf->u.s2.rgPins2[iPin].nMediaTypes; iType++)
                {
                    CoTaskMemFree((void *)rgf->u.s2.rgPins2[iPin].lpMediaType[iType].clsMajorType);
                    CoTaskMemFree((void *)rgf->u.s2.rgPins2[iPin].lpMediaType[iType].clsMinorType);
                }

                CoTaskMemFree((void *)rgf->u.s2.rgPins2[iPin].lpMediaType);
            }
        }

        CoTaskMemFree((void *)rgf->u.s2.rgPins2);
    }
}

static void write_filter_data(IPropertyBag *prop_bag, REGFILTER2 *rgf)
{
    IAMFilterData *fildata;
    SAFEARRAYBOUND sabound;
    BYTE *data, *array;
    VARIANT var;
    ULONG size;
    HRESULT hr;

    hr = CoCreateInstance(&CLSID_FilterMapper2, NULL, CLSCTX_INPROC, &IID_IAMFilterData, (void **)&fildata);
    if (FAILED(hr)) goto cleanup;

    hr = IAMFilterData_CreateFilterData(fildata, rgf, &data, &size);
    if (FAILED(hr)) goto cleanup;

    V_VT(&var) = VT_ARRAY | VT_UI1;
    sabound.lLbound = 0;
    sabound.cElements = size;
    if (!(V_ARRAY(&var) = SafeArrayCreate(VT_UI1, 1, &sabound)))
        goto cleanup;
    hr = SafeArrayAccessData(V_ARRAY(&var), (void *)&array);
    if (FAILED(hr)) goto cleanup;

    memcpy(array, data, size);
    hr = SafeArrayUnaccessData(V_ARRAY(&var));
    if (FAILED(hr)) goto cleanup;

    hr = IPropertyBag_Write(prop_bag, wszFilterData, &var);
    if (FAILED(hr)) goto cleanup;

cleanup:
    VariantClear(&var);
    CoTaskMemFree(data);
    IAMFilterData_Release(fildata);
}

static void register_legacy_filters(void)
{
    HKEY hkeyFilter = NULL;
    DWORD dwFilterSubkeys, i;
    LONG lRet;
    HRESULT hr;

    lRet = RegOpenKeyExW(HKEY_CLASSES_ROOT, wszFilterKeyName, 0, KEY_READ, &hkeyFilter);
    hr = HRESULT_FROM_WIN32(lRet);

    if (SUCCEEDED(hr))
    {
        lRet = RegQueryInfoKeyW(hkeyFilter, NULL, NULL, NULL, &dwFilterSubkeys, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
        hr = HRESULT_FROM_WIN32(lRet);
    }

    if (SUCCEEDED(hr))
        hr = DEVENUM_CreateAMCategoryKey(&CLSID_LegacyAmFilterCategory);

    if (SUCCEEDED(hr))
    {
        for (i = 0; i < dwFilterSubkeys; i++)
        {
            WCHAR wszFilterSubkeyName[64];
            DWORD cName = sizeof(wszFilterSubkeyName) / sizeof(WCHAR);
            IPropertyBag *prop_bag = NULL;
            WCHAR wszRegKey[MAX_PATH];
            HKEY classkey = NULL;
            IMoniker *mon = NULL;
            VARIANT var;
            REGFILTER2 rgf2;
            DWORD Type, len;

            if (RegEnumKeyExW(hkeyFilter, i, wszFilterSubkeyName, &cName, NULL, NULL, NULL, NULL) != ERROR_SUCCESS) continue;

            TRACE("Registering %s\n", debugstr_w(wszFilterSubkeyName));

            strcpyW(wszRegKey, clsidW);
            strcatW(wszRegKey, wszFilterSubkeyName);

            if (RegOpenKeyExW(HKEY_CLASSES_ROOT, wszRegKey, 0, KEY_READ, &classkey) != ERROR_SUCCESS)
                continue;

            hr = register_codec(&CLSID_LegacyAmFilterCategory, wszFilterSubkeyName, &mon);
            if (FAILED(hr)) goto cleanup;

            hr = IMoniker_BindToStorage(mon, NULL, NULL, &IID_IPropertyBag, (void **)&prop_bag);
            if (FAILED(hr)) goto cleanup;

            /* write friendly name */
            len = 0;
            V_VT(&var) = VT_BSTR;
            if (!RegQueryValueExW(classkey, NULL, NULL, &Type, NULL, &len))
            {
                WCHAR *friendlyname = heap_alloc(len);
                if (!friendlyname)
                    goto cleanup;
                RegQueryValueExW(classkey, NULL, NULL, &Type, (BYTE *)friendlyname, &len);
                V_BSTR(&var) = SysAllocStringLen(friendlyname, len/sizeof(WCHAR));
                heap_free(friendlyname);
            }
            else
                V_BSTR(&var) = SysAllocString(wszFilterSubkeyName);

            if (!V_BSTR(&var))
                goto cleanup;
            hr = IPropertyBag_Write(prop_bag, wszFriendlyName, &var);
            if (FAILED(hr)) goto cleanup;
            VariantClear(&var);

            /* write clsid */
            V_VT(&var) = VT_BSTR;
            if (!(V_BSTR(&var) = SysAllocString(wszFilterSubkeyName)))
                goto cleanup;
            hr = IPropertyBag_Write(prop_bag, clsid_keyname, &var);
            if (FAILED(hr)) goto cleanup;
            VariantClear(&var);

            /* write filter data */
            rgf2.dwMerit = MERIT_NORMAL;

            len = sizeof(rgf2.dwMerit);
            RegQueryValueExW(classkey, wszMeritName, NULL, &Type, (BYTE *)&rgf2.dwMerit, &len);

            DEVENUM_ReadPins(classkey, &rgf2);

            write_filter_data(prop_bag, &rgf2);

cleanup:
            if (prop_bag) IPropertyBag_Release(prop_bag);
            if (mon) IMoniker_Release(mon);
            RegCloseKey(classkey);
            VariantClear(&var);
            free_regfilter2(&rgf2);
        }
    }

    if (hkeyFilter) RegCloseKey(hkeyFilter);
}

static BOOL CALLBACK register_dsound_devices(GUID *guid, const WCHAR *desc, const WCHAR *module, void *context)
{
    static const WCHAR defaultW[] = {'D','e','f','a','u','l','t',' ','D','i','r','e','c','t','S','o','u','n','d',' ','D','e','v','i','c','e',0};
    static const WCHAR directsoundW[] = {'D','i','r','e','c','t','S','o','u','n','d',':',' ',0};
    static const WCHAR dsguidW[] = {'D','S','G','u','i','d',0};
    IPropertyBag *prop_bag = NULL;
    REGFILTERPINS2 rgpins = {0};
    REGPINTYPES rgtypes = {0};
    REGFILTER2 rgf = {0};
    WCHAR clsid[CHARS_IN_GUID];
    IMoniker *mon = NULL;
    VARIANT var;
    HRESULT hr;

    hr = DEVENUM_CreateAMCategoryKey(&CLSID_AudioRendererCategory);
    if (FAILED(hr)) goto cleanup;

    V_VT(&var) = VT_BSTR;
    if (guid)
    {
        WCHAR *name = heap_alloc(sizeof(defaultW) + strlenW(desc) * sizeof(WCHAR));
        if (!name)
            goto cleanup;
        strcpyW(name, directsoundW);
        strcatW(name, desc);

        V_BSTR(&var) = SysAllocString(name);
        heap_free(name);
    }
    else
        V_BSTR(&var) = SysAllocString(defaultW);

    if (!V_BSTR(&var))
        goto cleanup;

    hr = register_codec(&CLSID_AudioRendererCategory, V_BSTR(&var), &mon);
    if (FAILED(hr)) goto cleanup;

    hr = IMoniker_BindToStorage(mon, NULL, NULL, &IID_IPropertyBag, (void **)&prop_bag);
    if (FAILED(hr)) goto cleanup;

    /* write friendly name */
    hr = IPropertyBag_Write(prop_bag, wszFriendlyName, &var);
    if (FAILED(hr)) goto cleanup;
    VariantClear(&var);

    /* write clsid */
    V_VT(&var) = VT_BSTR;
    StringFromGUID2(&CLSID_DSoundRender, clsid, CHARS_IN_GUID);
    if (!(V_BSTR(&var) = SysAllocString(clsid)))
        goto cleanup;
    hr = IPropertyBag_Write(prop_bag, clsid_keyname, &var);
    if (FAILED(hr)) goto cleanup;
    VariantClear(&var);

    /* write filter data */
    rgf.dwVersion = 2;
    rgf.dwMerit = guid ? MERIT_DO_NOT_USE : MERIT_PREFERRED;
    rgf.u.s2.cPins2 = 1;
    rgf.u.s2.rgPins2 = &rgpins;
    rgpins.dwFlags = REG_PINFLAG_B_RENDERER;
    /* FIXME: native registers many more formats */
    rgpins.nMediaTypes = 1;
    rgpins.lpMediaType = &rgtypes;
    rgtypes.clsMajorType = &MEDIATYPE_Audio;
    rgtypes.clsMinorType = &MEDIASUBTYPE_PCM;

    write_filter_data(prop_bag, &rgf);

    /* write DSound guid */
    V_VT(&var) = VT_BSTR;
    StringFromGUID2(guid ? guid : &GUID_NULL, clsid, CHARS_IN_GUID);
    if (!(V_BSTR(&var) = SysAllocString(clsid)))
        goto cleanup;
    hr = IPropertyBag_Write(prop_bag, dsguidW, &var);
    if (FAILED(hr)) goto cleanup;

cleanup:
    VariantClear(&var);
    if (prop_bag) IPropertyBag_Release(prop_bag);
    if (mon) IMoniker_Release(mon);

    return TRUE;
}

static void register_waveout_devices(void)
{
    static const WCHAR defaultW[] = {'D','e','f','a','u','l','t',' ','W','a','v','e','O','u','t',' ','D','e','v','i','c','e',0};
    static const WCHAR waveoutidW[] = {'W','a','v','e','O','u','t','I','d',0};
    IPropertyBag *prop_bag = NULL;
    REGFILTERPINS2 rgpins = {0};
    REGPINTYPES rgtypes = {0};
    REGFILTER2 rgf = {0};
    WCHAR clsid[CHARS_IN_GUID];
    IMoniker *mon = NULL;
    WAVEOUTCAPSW caps;
    int i, count;
    VARIANT var;
    HRESULT hr;

    hr = DEVENUM_CreateAMCategoryKey(&CLSID_AudioRendererCategory);
    if (FAILED(hr)) return;

    count = waveOutGetNumDevs();

    for (i = -1; i < count; i++)
    {
        waveOutGetDevCapsW(i, &caps, sizeof(caps));

        V_VT(&var) = VT_BSTR;

        if (i == -1)    /* WAVE_MAPPER */
            V_BSTR(&var) = SysAllocString(defaultW);
        else
            V_BSTR(&var) = SysAllocString(caps.szPname);
        if (!(V_BSTR(&var)))
            goto cleanup;

        hr = register_codec(&CLSID_AudioRendererCategory, V_BSTR(&var), &mon);
        if (FAILED(hr)) goto cleanup;

        hr = IMoniker_BindToStorage(mon, NULL, NULL, &IID_IPropertyBag, (void **)&prop_bag);
        if (FAILED(hr)) goto cleanup;

        /* write friendly name */
        hr = IPropertyBag_Write(prop_bag, wszFriendlyName, &var);
        if (FAILED(hr)) goto cleanup;
        VariantClear(&var);

        /* write clsid */
        V_VT(&var) = VT_BSTR;
        StringFromGUID2(&CLSID_AudioRender, clsid, CHARS_IN_GUID);
        if (!(V_BSTR(&var) = SysAllocString(clsid)))
            goto cleanup;
        hr = IPropertyBag_Write(prop_bag, clsid_keyname, &var);
        if (FAILED(hr)) goto cleanup;
        VariantClear(&var);

        /* write filter data */
        rgf.dwVersion = 2;
        rgf.dwMerit = MERIT_DO_NOT_USE;
        rgf.u.s2.cPins2 = 1;
        rgf.u.s2.rgPins2 = &rgpins;
        rgpins.dwFlags = REG_PINFLAG_B_RENDERER;
        rgpins.nMediaTypes = 1;
        rgpins.lpMediaType = &rgtypes;
        rgtypes.clsMajorType = &MEDIATYPE_Audio;
        rgtypes.clsMinorType = &MEDIASUBTYPE_NULL;

        write_filter_data(prop_bag, &rgf);

        /* write WaveOutId */
        V_VT(&var) = VT_I4;
        V_I4(&var) = i;
        hr = IPropertyBag_Write(prop_bag, waveoutidW, &var);
        if (FAILED(hr)) goto cleanup;

cleanup:
        VariantClear(&var);
        if (prop_bag) IPropertyBag_Release(prop_bag);
        if (mon) IMoniker_Release(mon);
    }
}

static void register_wavein_devices(void)
{
    static const WCHAR waveinidW[] = {'W','a','v','e','I','n','I','d',0};
    IPropertyBag *prop_bag = NULL;
    REGFILTER2 rgf = {0};
    WCHAR clsid[CHARS_IN_GUID];
    IMoniker *mon = NULL;
    WAVEINCAPSW caps;
    int i, count;
    VARIANT var;
    HRESULT hr;

    hr = DEVENUM_CreateAMCategoryKey(&CLSID_AudioRendererCategory);
    if (FAILED(hr)) return;

    count = waveInGetNumDevs();

    for (i = 0; i < count; i++)
    {
        waveInGetDevCapsW(i, &caps, sizeof(caps));

        V_VT(&var) = VT_BSTR;

        V_BSTR(&var) = SysAllocString(caps.szPname);
        if (!(V_BSTR(&var)))
            goto cleanup;

        hr = register_codec(&CLSID_AudioInputDeviceCategory, V_BSTR(&var), &mon);
        if (FAILED(hr)) goto cleanup;

        hr = IMoniker_BindToStorage(mon, NULL, NULL, &IID_IPropertyBag, (void **)&prop_bag);
        if (FAILED(hr)) goto cleanup;

        /* write friendly name */
        hr = IPropertyBag_Write(prop_bag, wszFriendlyName, &var);
        if (FAILED(hr)) goto cleanup;
        VariantClear(&var);

        /* write clsid */
        V_VT(&var) = VT_BSTR;
        StringFromGUID2(&CLSID_AudioRecord, clsid, CHARS_IN_GUID);
        if (!(V_BSTR(&var) = SysAllocString(clsid)))
            goto cleanup;
        hr = IPropertyBag_Write(prop_bag, clsid_keyname, &var);
        if (FAILED(hr)) goto cleanup;
        VariantClear(&var);

        /* write filter data */
        rgf.dwVersion = 2;
        rgf.dwMerit = MERIT_DO_NOT_USE;

        write_filter_data(prop_bag, &rgf);

        /* write WaveInId */
        V_VT(&var) = VT_I4;
        V_I4(&var) = i;
        hr = IPropertyBag_Write(prop_bag, waveinidW, &var);
        if (FAILED(hr)) goto cleanup;

cleanup:
        VariantClear(&var);
        if (prop_bag) IPropertyBag_Release(prop_bag);
        if (mon) IMoniker_Release(mon);
    }
}

static void register_midiout_devices(void)
{
    static const WCHAR defaultW[] = {'D','e','f','a','u','l','t',' ','M','i','d','i','O','u','t',' ','D','e','v','i','c','e',0};
    static const WCHAR midioutidW[] = {'M','i','d','i','O','u','t','I','d',0};
    IPropertyBag *prop_bag = NULL;
    REGFILTERPINS2 rgpins = {0};
    REGPINTYPES rgtypes = {0};
    REGFILTER2 rgf = {0};
    WCHAR clsid[CHARS_IN_GUID];
    IMoniker *mon = NULL;
    MIDIOUTCAPSW caps;
    int i, count;
    VARIANT var;
    HRESULT hr;

    hr = DEVENUM_CreateAMCategoryKey(&CLSID_AudioRendererCategory);
    if (FAILED(hr)) return;

    count = midiOutGetNumDevs();

    for (i = -1; i < count; i++)
    {
        midiOutGetDevCapsW(i, &caps, sizeof(caps));

        V_VT(&var) = VT_BSTR;

        if (i == -1)    /* MIDI_MAPPER */
            V_BSTR(&var) = SysAllocString(defaultW);
        else
            V_BSTR(&var) = SysAllocString(caps.szPname);
        if (!(V_BSTR(&var)))
            goto cleanup;

        hr = register_codec(&CLSID_MidiRendererCategory, V_BSTR(&var), &mon);
        if (FAILED(hr)) goto cleanup;

        hr = IMoniker_BindToStorage(mon, NULL, NULL, &IID_IPropertyBag, (void **)&prop_bag);
        if (FAILED(hr)) goto cleanup;

        /* write friendly name */
        hr = IPropertyBag_Write(prop_bag, wszFriendlyName, &var);
        if (FAILED(hr)) goto cleanup;
        VariantClear(&var);

        /* write clsid */
        V_VT(&var) = VT_BSTR;
        StringFromGUID2(&CLSID_AVIMIDIRender, clsid, CHARS_IN_GUID);
        if (!(V_BSTR(&var) = SysAllocString(clsid)))
            goto cleanup;
        hr = IPropertyBag_Write(prop_bag, clsid_keyname, &var);
        if (FAILED(hr)) goto cleanup;
        VariantClear(&var);

        /* write filter data */
        rgf.dwVersion = 2;
        rgf.dwMerit = (i == -1) ? MERIT_PREFERRED : MERIT_DO_NOT_USE;
        rgf.u.s2.cPins2 = 1;
        rgf.u.s2.rgPins2 = &rgpins;
        rgpins.dwFlags = REG_PINFLAG_B_RENDERER;
        rgpins.nMediaTypes = 1;
        rgpins.lpMediaType = &rgtypes;
        rgtypes.clsMajorType = &MEDIATYPE_Midi;
        rgtypes.clsMinorType = &MEDIASUBTYPE_NULL;

        write_filter_data(prop_bag, &rgf);

        /* write MidiOutId */
        V_VT(&var) = VT_I4;
        V_I4(&var) = i;
        hr = IPropertyBag_Write(prop_bag, midioutidW, &var);
        if (FAILED(hr)) goto cleanup;

cleanup:
        VariantClear(&var);
        if (prop_bag) IPropertyBag_Release(prop_bag);
        if (mon) IMoniker_Release(mon);
    }
}

static void register_vfw_codecs(void)
{
    static const WCHAR fcchandlerW[] = {'F','c','c','H','a','n','d','l','e','r',0};
    REGFILTERPINS2 rgpins[2] = {{0}};
    IPropertyBag *prop_bag = NULL;
    REGPINTYPES rgtypes[2];
    REGFILTER2 rgf;
    WCHAR clsid[CHARS_IN_GUID];
    IMoniker *mon = NULL;
    GUID typeguid;
    ICINFO info;
    VARIANT var;
    HRESULT hr;
    int i = 0;
    HIC hic;

    hr = DEVENUM_CreateAMCategoryKey(&CLSID_AudioRendererCategory);
    if (FAILED(hr)) return;

    while (ICInfo(ICTYPE_VIDEO, i++, &info))
    {
        WCHAR name[5] = {LOBYTE(LOWORD(info.fccHandler)), HIBYTE(LOWORD(info.fccHandler)),
                         LOBYTE(HIWORD(info.fccHandler)), HIBYTE(HIWORD(info.fccHandler))};

        hic = ICOpen(ICTYPE_VIDEO, info.fccHandler, ICMODE_QUERY);
        ICGetInfo(hic, &info, sizeof(info));
        ICClose(hic);

        V_VT(&var) = VT_BSTR;

        V_BSTR(&var) = SysAllocString(name);
        if (!(V_BSTR(&var)))
            goto cleanup;

        hr = register_codec(&CLSID_VideoCompressorCategory, V_BSTR(&var), &mon);
        if (FAILED(hr)) goto cleanup;

        hr = IMoniker_BindToStorage(mon, NULL, NULL, &IID_IPropertyBag, (void **)&prop_bag);
        if (FAILED(hr)) goto cleanup;

        /* write WaveInId */
        hr = IPropertyBag_Write(prop_bag, fcchandlerW, &var);
        if (FAILED(hr)) goto cleanup;
        VariantClear(&var);

        /* write friendly name */
        V_VT(&var) = VT_BSTR;
        if (!(V_BSTR(&var) = SysAllocString(info.szDescription)))
            goto cleanup;

        hr = IPropertyBag_Write(prop_bag, wszFriendlyName, &var);
        if (FAILED(hr)) goto cleanup;
        VariantClear(&var);

        /* write clsid */
        V_VT(&var) = VT_BSTR;
        StringFromGUID2(&CLSID_AVICo, clsid, CHARS_IN_GUID);
        if (!(V_BSTR(&var) = SysAllocString(clsid)))
            goto cleanup;
        hr = IPropertyBag_Write(prop_bag, clsid_keyname, &var);
        if (FAILED(hr)) goto cleanup;
        VariantClear(&var);

        /* write filter data */
        rgf.dwVersion = 2;
        rgf.dwMerit = MERIT_DO_NOT_USE;
        rgf.u.s2.cPins2 = 2;
        rgf.u.s2.rgPins2 = rgpins;
        rgpins[0].dwFlags = 0;
        rgpins[0].nMediaTypes = 1;
        rgpins[0].lpMediaType = &rgtypes[0];
        rgtypes[0].clsMajorType = &MEDIATYPE_Video;
        typeguid = MEDIASUBTYPE_PCM;
        typeguid.Data1 = info.fccHandler;
        rgtypes[0].clsMinorType = &typeguid;
        rgpins[1].dwFlags = REG_PINFLAG_B_OUTPUT;
        rgpins[1].nMediaTypes = 1;
        rgpins[1].lpMediaType = &rgtypes[1];
        rgtypes[1].clsMajorType = &MEDIATYPE_Video;
        rgtypes[1].clsMinorType = &GUID_NULL;

        write_filter_data(prop_bag, &rgf);

cleanup:
        VariantClear(&var);
        if (prop_bag) IPropertyBag_Release(prop_bag);
        if (mon) IMoniker_Release(mon);
    }
}

/**********************************************************************
 * DEVENUM_ICreateDevEnum_CreateClassEnumerator
 */
static HRESULT WINAPI DEVENUM_ICreateDevEnum_CreateClassEnumerator(
    ICreateDevEnum * iface,
    REFCLSID clsidDeviceClass,
    IEnumMoniker **ppEnumMoniker,
    DWORD dwFlags)
{
    HRESULT hr;

    TRACE("(%p)->(%s, %p, %x)\n", iface, debugstr_guid(clsidDeviceClass), ppEnumMoniker, dwFlags);

    if (!ppEnumMoniker)
        return E_POINTER;

    *ppEnumMoniker = NULL;

    register_codecs();
    register_legacy_filters();
    hr = DirectSoundEnumerateW(&register_dsound_devices, NULL);
    if (FAILED(hr)) return hr;
    register_waveout_devices();
    register_wavein_devices();
    register_midiout_devices();
    register_vfw_codecs();

    return create_EnumMoniker(clsidDeviceClass, ppEnumMoniker);
}

/**********************************************************************
 * ICreateDevEnum_Vtbl
 */
static const ICreateDevEnumVtbl ICreateDevEnum_Vtbl =
{
    DEVENUM_ICreateDevEnum_QueryInterface,
    DEVENUM_ICreateDevEnum_AddRef,
    DEVENUM_ICreateDevEnum_Release,
    DEVENUM_ICreateDevEnum_CreateClassEnumerator,
};

/**********************************************************************
 * static CreateDevEnum instance
 */
ICreateDevEnum DEVENUM_CreateDevEnum = { &ICreateDevEnum_Vtbl };

/**********************************************************************
 * DEVENUM_CreateAMCategoryKey (INTERNAL)
 *
 * Creates a registry key for a category at HKEY_CURRENT_USER\Software\
 * Microsoft\ActiveMovie\devenum\{clsid}
 */
static HRESULT DEVENUM_CreateAMCategoryKey(const CLSID * clsidCategory)
{
    WCHAR wszRegKey[MAX_PATH];
    HRESULT res = S_OK;
    HKEY hkeyDummy = NULL;

    strcpyW(wszRegKey, wszActiveMovieKey);

    if (!StringFromGUID2(clsidCategory, wszRegKey + strlenW(wszRegKey), sizeof(wszRegKey)/sizeof(wszRegKey[0]) - strlenW(wszRegKey)))
        res = E_INVALIDARG;

    if (SUCCEEDED(res))
    {
        LONG lRes = RegCreateKeyW(HKEY_CURRENT_USER, wszRegKey, &hkeyDummy);
        res = HRESULT_FROM_WIN32(lRes);
    }

    if (hkeyDummy)
        RegCloseKey(hkeyDummy);

    if (FAILED(res))
        ERR("Failed to create key HKEY_CURRENT_USER\\%s\n", debugstr_w(wszRegKey));

    return res;
}

static HRESULT register_codecs(void)
{
    HRESULT res;
    WCHAR class[CHARS_IN_GUID];
    DWORD iDefaultDevice = -1;
    IFilterMapper2 * pMapper = NULL;
    REGFILTER2 rf2;
    REGFILTERPINS2 rfp2;
    HKEY basekey;

    /* Since devices can change between session, for example because you just plugged in a webcam
     * or switched from pulseaudio to alsa, delete all old devices first
     */
    RegOpenKeyW(HKEY_CURRENT_USER, wszActiveMovieKey, &basekey);
    StringFromGUID2(&CLSID_LegacyAmFilterCategory, class, CHARS_IN_GUID);
    RegDeleteTreeW(basekey, class);
    StringFromGUID2(&CLSID_AudioRendererCategory, class, CHARS_IN_GUID);
    RegDeleteTreeW(basekey, class);
    StringFromGUID2(&CLSID_AudioInputDeviceCategory, class, CHARS_IN_GUID);
    RegDeleteTreeW(basekey, class);
    StringFromGUID2(&CLSID_VideoInputDeviceCategory, class, CHARS_IN_GUID);
    RegDeleteTreeW(basekey, class);
    StringFromGUID2(&CLSID_MidiRendererCategory, class, CHARS_IN_GUID);
    RegDeleteTreeW(basekey, class);
    StringFromGUID2(&CLSID_VideoCompressorCategory, class, CHARS_IN_GUID);
    RegDeleteTreeW(basekey, class);
    RegCloseKey(basekey);

    rf2.dwVersion = 2;
    rf2.dwMerit = MERIT_PREFERRED;
    rf2.u.s2.cPins2 = 1;
    rf2.u.s2.rgPins2 = &rfp2;
    rfp2.cInstances = 1;
    rfp2.nMediums = 0;
    rfp2.lpMedium = NULL;
    rfp2.clsPinCategory = &IID_NULL;

    res = CoCreateInstance(&CLSID_FilterMapper2, NULL, CLSCTX_INPROC,
                           &IID_IFilterMapper2, (void **) &pMapper);
    /*
     * Fill in info for devices
     */
    if (SUCCEEDED(res))
    {
        UINT i;
        REGPINTYPES * pTypes;
        IPropertyBag * pPropBag = NULL;

        res = DEVENUM_CreateAMCategoryKey(&CLSID_VideoInputDeviceCategory);
        if (SUCCEEDED(res))
            for (i = 0; i < 10; i++)
            {
                WCHAR szDeviceName[32], szDeviceVersion[32], szDevicePath[10];

                if (capGetDriverDescriptionW ((WORD) i,
                                              szDeviceName, sizeof(szDeviceName)/sizeof(WCHAR),
                                              szDeviceVersion, sizeof(szDeviceVersion)/sizeof(WCHAR)))
                {
                    IMoniker * pMoniker = NULL;
                    WCHAR dprintf[] = { 'v','i','d','e','o','%','d',0 };
                    snprintfW(szDevicePath, sizeof(szDevicePath)/sizeof(WCHAR), dprintf, i);
                    /* The above code prevents 1 device with a different ID overwriting another */

                    rfp2.nMediaTypes = 1;
                    pTypes = CoTaskMemAlloc(rfp2.nMediaTypes * sizeof(REGPINTYPES));
                    if (!pTypes) {
                        IFilterMapper2_Release(pMapper);
                        return E_OUTOFMEMORY;
                    }

                    pTypes[0].clsMajorType = &MEDIATYPE_Video;
                    pTypes[0].clsMinorType = &MEDIASUBTYPE_None;

                    rfp2.lpMediaType = pTypes;

                    res = IFilterMapper2_RegisterFilter(pMapper,
                                                        &CLSID_VfwCapture,
                                                        szDeviceName,
                                                        &pMoniker,
                                                        &CLSID_VideoInputDeviceCategory,
                                                        szDevicePath,
                                                        &rf2);

                    if (pMoniker) {
                       OLECHAR wszVfwIndex[] = { 'V','F','W','I','n','d','e','x',0 };
                       VARIANT var;
                       V_VT(&var) = VT_I4;
                       V_I4(&var) = i;
                       res = IMoniker_BindToStorage(pMoniker, NULL, NULL, &IID_IPropertyBag, (LPVOID)&pPropBag);
                       if (SUCCEEDED(res)) {
                           res = IPropertyBag_Write(pPropBag, wszVfwIndex, &var);
                           IPropertyBag_Release(pPropBag);
                       }
                       IMoniker_Release(pMoniker);
                    }

                    if (i == iDefaultDevice) FIXME("Default device\n");
                    CoTaskMemFree(pTypes);
                }
            }
    }

    if (pMapper)
        IFilterMapper2_Release(pMapper);

    return res;
}
