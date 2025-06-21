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

#include "devenum_private.h"
#include "vfw.h"
#include "aviriff.h"
#include "dsound.h"

#include "wine/debug.h"
#include "mmddk.h"

#include "initguid.h"
#include "wine/fil_data.h"

WINE_DEFAULT_DEBUG_CHANNEL(devenum);

static HRESULT WINAPI devenum_factory_QueryInterface(ICreateDevEnum *iface, REFIID riid, void **ppv)
{
    TRACE("(%p)->(%s, %p)\n", iface, debugstr_guid(riid), ppv);

    if (!ppv)
        return E_POINTER;

    if (IsEqualGUID(riid, &IID_IUnknown) ||
	IsEqualGUID(riid, &IID_ICreateDevEnum))
    {
        *ppv = iface;
        ICreateDevEnum_AddRef(iface);
	return S_OK;
    }

    FIXME("- no interface IID: %s\n", debugstr_guid(riid));
    *ppv = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI devenum_factory_AddRef(ICreateDevEnum *iface)
{
    TRACE("\n");

    return 2; /* non-heap based object */
}

static ULONG WINAPI devenum_factory_Release(ICreateDevEnum *iface)
{
    TRACE("\n");

    return 1; /* non-heap based object */
}

static HRESULT register_codec(const GUID *class, const WCHAR *name,
        const GUID *clsid, const WCHAR *friendly_name, IPropertyBag **ret)
{
    WCHAR guidstr[CHARS_IN_GUID];
    IParseDisplayName *parser;
    IPropertyBag *propbag;
    IMoniker *mon;
    WCHAR *buffer;
    VARIANT var;
    ULONG eaten;
    HRESULT hr;

    hr = CoCreateInstance(&CLSID_CDeviceMoniker, NULL, CLSCTX_INPROC, &IID_IParseDisplayName, (void **)&parser);
    if (FAILED(hr))
        return hr;

    if (!(buffer = malloc((wcslen(L"@device:cm:") + CHARS_IN_GUID + wcslen(name) + 1) * sizeof(WCHAR))))
    {
        IParseDisplayName_Release(parser);
        return E_OUTOFMEMORY;
    }

    wcscpy(buffer, L"@device:cm:");
    StringFromGUID2(class, buffer + wcslen(buffer), CHARS_IN_GUID);
    wcscat(buffer, L"\\");
    wcscat(buffer, name);

    IParseDisplayName_ParseDisplayName(parser, NULL, buffer, &eaten, &mon);
    IParseDisplayName_Release(parser);
    free(buffer);

    IMoniker_BindToStorage(mon, NULL, NULL, &IID_IPropertyBag, (void **)&propbag);
    IMoniker_Release(mon);

    V_VT(&var) = VT_BSTR;
    V_BSTR(&var) = SysAllocString(friendly_name);
    hr = IPropertyBag_Write(propbag, L"FriendlyName", &var);
    VariantClear(&var);
    if (FAILED(hr))
    {
        IPropertyBag_Release(propbag);
        return hr;
    }

    V_VT(&var) = VT_BSTR;
    StringFromGUID2(clsid, guidstr, ARRAY_SIZE(guidstr));
    V_BSTR(&var) = SysAllocString(guidstr);
    hr = IPropertyBag_Write(propbag, L"CLSID", &var);
    VariantClear(&var);
    if (FAILED(hr))
    {
        IPropertyBag_Release(propbag);
        return hr;
    }

    *ret = propbag;
    return S_OK;
}

static void DEVENUM_ReadPinTypes(HKEY hkeyPinKey, REGFILTERPINS2 *rgPin)
{
    HKEY hkeyTypes = NULL;
    DWORD dwMajorTypes, i;
    REGPINTYPES *lpMediaType = NULL;
    DWORD dwMediaTypeSize = 0;

    if (RegOpenKeyExW(hkeyPinKey, L"Types", 0, KEY_READ, &hkeyTypes) != ERROR_SUCCESS)
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
        DWORD cName = ARRAY_SIZE(wszMajorTypeName);
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

            cName = ARRAY_SIZE(wszMinorTypeName);
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
    rgf2->cPins2 = 0;
    rgf2->rgPins2 = NULL;

    if (RegOpenKeyExW(hkeyFilterClass, L"Pins", 0, KEY_READ, &hkeyPins) != ERROR_SUCCESS)
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
        DWORD cName = ARRAY_SIZE(wszPinName);
        REGFILTERPINS2 *rgPin = &rgPins[rgf2->cPins2];
        DWORD value, size, Type;
        LONG lRet;

        memset(rgPin, 0, sizeof(*rgPin));

        if (RegEnumKeyExW(hkeyPins, i, wszPinName, &cName, NULL, NULL, NULL, NULL) != ERROR_SUCCESS) continue;

        if (RegOpenKeyExW(hkeyPins, wszPinName, 0, KEY_READ, &hkeyPinKey) != ERROR_SUCCESS) continue;

        size = sizeof(DWORD);
        lRet = RegQueryValueExW(hkeyPinKey, L"AllowedMany", NULL, &Type, (BYTE *)&value, &size);
        if (lRet != ERROR_SUCCESS || Type != REG_DWORD)
            goto error_cleanup;
        if (value)
            rgPin->dwFlags |= REG_PINFLAG_B_MANY;

        size = sizeof(DWORD);
        lRet = RegQueryValueExW(hkeyPinKey, L"AllowedZero", NULL, &Type, (BYTE *)&value, &size);
        if (lRet != ERROR_SUCCESS || Type != REG_DWORD)
            goto error_cleanup;
        if (value)
            rgPin->dwFlags |= REG_PINFLAG_B_ZERO;

        size = sizeof(DWORD);
        lRet = RegQueryValueExW(hkeyPinKey, L"Direction", NULL, &Type, (BYTE *)&value, &size);
        if (lRet != ERROR_SUCCESS || Type != REG_DWORD)
            goto error_cleanup;
        if (value)
            rgPin->dwFlags |= REG_PINFLAG_B_OUTPUT;


        size = sizeof(DWORD);
        lRet = RegQueryValueExW(hkeyPinKey, L"IsRendered", NULL, &Type, (BYTE *)&value, &size);
        if (lRet != ERROR_SUCCESS || Type != REG_DWORD)
            goto error_cleanup;
        if (value)
            rgPin->dwFlags |= REG_PINFLAG_B_RENDERER;

        DEVENUM_ReadPinTypes(hkeyPinKey, rgPin);

        ++rgf2->cPins2;
        continue;

        error_cleanup:

        RegCloseKey(hkeyPinKey);
    }

    RegCloseKey(hkeyPins);

    if (rgPins && !rgf2->cPins2)
    {
        CoTaskMemFree(rgPins);
        rgPins = NULL;
    }

    rgf2->rgPins2 = rgPins;
}

static void free_regfilter2(REGFILTER2 *rgf)
{
    if (rgf->rgPins2)
    {
        UINT iPin;

        for (iPin = 0; iPin < rgf->cPins2; iPin++)
        {
            if (rgf->rgPins2[iPin].lpMediaType)
            {
                UINT iType;

                for (iType = 0; iType < rgf->rgPins2[iPin].nMediaTypes; iType++)
                {
                    CoTaskMemFree((void *)rgf->rgPins2[iPin].lpMediaType[iType].clsMajorType);
                    CoTaskMemFree((void *)rgf->rgPins2[iPin].lpMediaType[iType].clsMinorType);
                }

                CoTaskMemFree((void *)rgf->rgPins2[iPin].lpMediaType);
            }
        }

        CoTaskMemFree((void *)rgf->rgPins2);
    }
}

HRESULT create_filter_data(VARIANT *var, REGFILTER2 *rgf)
{
    IAMFilterData *fildata;
    BYTE *data = NULL;
    ULONG size;
    HRESULT hr;

    hr = CoCreateInstance(&CLSID_FilterMapper2, NULL, CLSCTX_INPROC, &IID_IAMFilterData, (void **)&fildata);
    if (FAILED(hr))
        return hr;

    hr = IAMFilterData_CreateFilterData(fildata, rgf, &data, &size);
    IAMFilterData_Release(fildata);
    if (FAILED(hr))
        return hr;

    V_VT(var) = VT_ARRAY | VT_UI1;
    if (!(V_ARRAY(var) = SafeArrayCreateVector(VT_UI1, 1, size)))
    {
        VariantClear(var);
        CoTaskMemFree(data);
        return E_OUTOFMEMORY;
    }

    memcpy(V_ARRAY(var)->pvData, data, size);
    CoTaskMemFree(data);
    return S_OK;
}

static void write_filter_data(IPropertyBag *prop_bag, REGFILTER2 *rgf)
{
    VARIANT var;

    if (SUCCEEDED(create_filter_data(&var, rgf)))
    {
        IPropertyBag_Write(prop_bag, L"FilterData", &var);
        VariantClear(&var);
    }
}

static void register_legacy_filters(void)
{
    HKEY hkeyFilter = NULL;
    DWORD dwFilterSubkeys, i;
    LONG lRet;
    HRESULT hr;

    lRet = RegOpenKeyExW(HKEY_CLASSES_ROOT, L"Filter", 0, KEY_READ, &hkeyFilter);
    hr = HRESULT_FROM_WIN32(lRet);

    if (SUCCEEDED(hr))
    {
        lRet = RegQueryInfoKeyW(hkeyFilter, NULL, NULL, NULL, &dwFilterSubkeys, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
        hr = HRESULT_FROM_WIN32(lRet);
    }

    if (SUCCEEDED(hr))
    {
        for (i = 0; i < dwFilterSubkeys; i++)
        {
            WCHAR wszFilterSubkeyName[64];
            DWORD cName = ARRAY_SIZE(wszFilterSubkeyName);
            IPropertyBag *prop_bag = NULL;
            WCHAR wszRegKey[MAX_PATH];
            HKEY classkey = NULL;
            REGFILTER2 rgf2;
            DWORD Type, len;
            GUID clsid;

            if (RegEnumKeyExW(hkeyFilter, i, wszFilterSubkeyName, &cName, NULL, NULL, NULL, NULL) != ERROR_SUCCESS) continue;

            TRACE("Registering %s\n", debugstr_w(wszFilterSubkeyName));

            hr = CLSIDFromString(wszFilterSubkeyName, &clsid);
            if (FAILED(hr))
                continue;

            swprintf(wszRegKey, ARRAY_SIZE(wszRegKey), L"CLSID\\%s", wszFilterSubkeyName);
            if (RegOpenKeyExW(HKEY_CLASSES_ROOT, wszRegKey, 0, KEY_READ, &classkey) != ERROR_SUCCESS)
                continue;

            len = 0;
            if (!RegQueryValueExW(classkey, NULL, NULL, &Type, NULL, &len))
            {
                WCHAR *friendlyname = malloc(len);
                if (!friendlyname)
                {
                    RegCloseKey(classkey);
                    continue;
                }
                RegQueryValueExW(classkey, NULL, NULL, &Type, (BYTE *)friendlyname, &len);

                hr = register_codec(&CLSID_LegacyAmFilterCategory, wszFilterSubkeyName,
                        &clsid, friendlyname, &prop_bag);

                free(friendlyname);
            }
            else
                hr = register_codec(&CLSID_LegacyAmFilterCategory, wszFilterSubkeyName,
                        &clsid, wszFilterSubkeyName, &prop_bag);
            if (FAILED(hr))
            {
                RegCloseKey(classkey);
                continue;
            }

            /* write filter data */
            rgf2.dwMerit = MERIT_NORMAL;

            len = sizeof(rgf2.dwMerit);
            RegQueryValueExW(classkey, L"Merit", NULL, &Type, (BYTE *)&rgf2.dwMerit, &len);

            DEVENUM_ReadPins(classkey, &rgf2);

            write_filter_data(prop_bag, &rgf2);

            IPropertyBag_Release(prop_bag);
            RegCloseKey(classkey);
            free_regfilter2(&rgf2);
        }
    }

    if (hkeyFilter) RegCloseKey(hkeyFilter);
}

static BOOL CALLBACK register_dsound_devices(GUID *guid, const WCHAR *desc, const WCHAR *module, void *context)
{
    static const WCHAR defaultW[] = L"Default DirectSound Device";
    IPropertyBag *prop_bag = NULL;
    REGFILTERPINS2 rgpins = {0};
    REGPINTYPES rgtypes = {0};
    REGFILTER2 rgf = {0};
    WCHAR clsid[CHARS_IN_GUID];
    VARIANT var;
    HRESULT hr;

    if (guid)
    {
        WCHAR *name = malloc(sizeof(defaultW) + wcslen(desc) * sizeof(WCHAR));
        if (!name)
            return FALSE;
        wcscpy(name, L"DirectSound: ");
        wcscat(name, desc);

        hr = register_codec(&CLSID_AudioRendererCategory, name,
                &CLSID_DSoundRender, name, &prop_bag);
        free(name);
    }
    else
        hr = register_codec(&CLSID_AudioRendererCategory, defaultW,
                &CLSID_DSoundRender, defaultW, &prop_bag);
    if (FAILED(hr))
        return FALSE;

    /* write filter data */
    rgf.dwVersion = 2;
    rgf.dwMerit = guid ? MERIT_DO_NOT_USE : MERIT_PREFERRED;
    rgf.cPins2 = 1;
    rgf.rgPins2 = &rgpins;
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
    if ((V_BSTR(&var) = SysAllocString(clsid)))
        hr = IPropertyBag_Write(prop_bag, L"DSGuid", &var);

    VariantClear(&var);
    IPropertyBag_Release(prop_bag);
    return TRUE;
}

static void register_waveout_devices(void)
{
    IPropertyBag *prop_bag = NULL;
    REGFILTERPINS2 rgpins = {0};
    REGPINTYPES rgtypes = {0};
    REGFILTER2 rgf = {0};
    WAVEOUTCAPSW caps;
    const WCHAR *name;
    int i, count;
    VARIANT var;
    HRESULT hr;

    count = waveOutGetNumDevs();

    for (i = -1; i < count; i++)
    {
        waveOutGetDevCapsW(i, &caps, sizeof(caps));

        name = (i == -1) ? L"Default WaveOut Device" : caps.szPname;

        hr = register_codec(&CLSID_AudioRendererCategory, name,
                &CLSID_AudioRender, name, &prop_bag);
        if (FAILED(hr))
            continue;

        /* write filter data */
        rgf.dwVersion = 2;
        rgf.dwMerit = MERIT_DO_NOT_USE;
        rgf.cPins2 = 1;
        rgf.rgPins2 = &rgpins;
        rgpins.dwFlags = REG_PINFLAG_B_RENDERER;
        rgpins.nMediaTypes = 1;
        rgpins.lpMediaType = &rgtypes;
        rgtypes.clsMajorType = &MEDIATYPE_Audio;
        rgtypes.clsMinorType = &MEDIASUBTYPE_NULL;

        write_filter_data(prop_bag, &rgf);

        /* write WaveOutId */
        V_VT(&var) = VT_I4;
        V_I4(&var) = i;
        IPropertyBag_Write(prop_bag, L"WaveOutId", &var);

        VariantClear(&var);
        if (prop_bag) IPropertyBag_Release(prop_bag);
    }
}

static void register_wavein_devices(void)
{
    IPropertyBag *prop_bag = NULL;
    REGFILTER2 rgf = {0};
    WAVEINCAPSW caps;
    int i, count;
    VARIANT var;
    HRESULT hr;

    count = waveInGetNumDevs();

    for (i = 0; i < count; i++)
    {
        waveInGetDevCapsW(i, &caps, sizeof(caps));

        hr = register_codec(&CLSID_AudioInputDeviceCategory, caps.szPname,
                &CLSID_AudioRecord, caps.szPname, &prop_bag);
        if (FAILED(hr))
            continue;

        /* write filter data */
        rgf.dwVersion = 2;
        rgf.dwMerit = MERIT_DO_NOT_USE;

        write_filter_data(prop_bag, &rgf);

        /* write WaveInId */
        V_VT(&var) = VT_I4;
        V_I4(&var) = i;
        IPropertyBag_Write(prop_bag, L"WaveInId", &var);

        VariantClear(&var);
        IPropertyBag_Release(prop_bag);
    }
}

static void register_midiout_devices(void)
{
    IPropertyBag *prop_bag = NULL;
    REGFILTERPINS2 rgpins = {0};
    REGPINTYPES rgtypes = {0};
    REGFILTER2 rgf = {0};
    MIDIOUTCAPSW caps;
    const WCHAR *name;
    int i, count;
    VARIANT var;
    HRESULT hr;

    count = midiOutGetNumDevs();

    for (i = -1; i < count; i++)
    {
        midiOutGetDevCapsW(i, &caps, sizeof(caps));

        name = (i == -1) ? L"Default MidiOut Device" : caps.szPname;

        hr = register_codec(&CLSID_MidiRendererCategory, name,
                &CLSID_AVIMIDIRender, name, &prop_bag);
        if (FAILED(hr))
            continue;

        /* write filter data */
        rgf.dwVersion = 2;
        rgf.dwMerit = (i == -1) ? MERIT_PREFERRED : MERIT_DO_NOT_USE;
        rgf.cPins2 = 1;
        rgf.rgPins2 = &rgpins;
        rgpins.dwFlags = REG_PINFLAG_B_RENDERER;
        rgpins.nMediaTypes = 1;
        rgpins.lpMediaType = &rgtypes;
        rgtypes.clsMajorType = &MEDIATYPE_Midi;
        rgtypes.clsMinorType = &MEDIASUBTYPE_NULL;

        write_filter_data(prop_bag, &rgf);

        /* write MidiOutId */
        V_VT(&var) = VT_I4;
        V_I4(&var) = i;
        IPropertyBag_Write(prop_bag, L"MidiOutId", &var);

        VariantClear(&var);
        IPropertyBag_Release(prop_bag);
    }
}

static void register_vfw_codecs(void)
{
    REGFILTERPINS2 rgpins[2] = {};
    IPropertyBag *prop_bag = NULL;
    REGPINTYPES rgtypes[2];
    REGFILTER2 rgf;
    GUID typeguid;
    ICINFO info;
    VARIANT var;
    HRESULT hr;
    int i = 0;
    HIC hic;

    while (ICInfo(ICTYPE_VIDEO, i++, &info))
    {
        WCHAR name[5] = {LOBYTE(LOWORD(info.fccHandler)), HIBYTE(LOWORD(info.fccHandler)),
                         LOBYTE(HIWORD(info.fccHandler)), HIBYTE(HIWORD(info.fccHandler))};

        hic = ICOpen(ICTYPE_VIDEO, info.fccHandler, ICMODE_QUERY);
        ICGetInfo(hic, &info, sizeof(info));
        ICClose(hic);

        hr = register_codec(&CLSID_VideoCompressorCategory, name,
                &CLSID_AVICo, info.szDescription, &prop_bag);
        if (FAILED(hr))
            continue;

        /* write filter data */
        rgf.dwVersion = 2;
        rgf.dwMerit = MERIT_DO_NOT_USE;
        rgf.cPins2 = 2;
        rgf.rgPins2 = rgpins;
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

        /* write WaveInId */
        V_VT(&var) = VT_BSTR;
        V_BSTR(&var) = SysAllocString(name);
        IPropertyBag_Write(prop_bag, L"FccHandler", &var);

        VariantClear(&var);
        IPropertyBag_Release(prop_bag);
    }
}

static void register_avicap_devices(void)
{
    WCHAR friendlyname[32], version[32];
    IPropertyBag *prop_bag = NULL;
    REGFILTERPINS2 rgpins = {0};
    REGPINTYPES rgtypes;
    REGFILTER2 rgf;
    WCHAR name[7];
    VARIANT var;
    HRESULT hr;
    int i = 0;

    for (i = 0; i < 10; ++i)
    {
        if (!capGetDriverDescriptionW(i, friendlyname, ARRAY_SIZE(friendlyname),
                version, ARRAY_SIZE(version)))
            continue;

        swprintf(name, ARRAY_SIZE(name), L"video%d", i);

        hr = register_codec(&CLSID_VideoInputDeviceCategory, name,
                &CLSID_VfwCapture, friendlyname, &prop_bag);
        if (FAILED(hr))
            continue;

        rgf.dwVersion = 2;
        rgf.dwMerit = MERIT_DO_NOT_USE;
        rgf.cPins2 = 1;
        rgf.rgPins2 = &rgpins;
        rgpins.dwFlags = 0;
        rgpins.nMediaTypes = 1;
        rgpins.lpMediaType = &rgtypes;
        rgtypes.clsMajorType = &MEDIATYPE_Video;
        rgtypes.clsMinorType = &MEDIASUBTYPE_None;

        write_filter_data(prop_bag, &rgf);

        /* write VFWIndex */
        V_VT(&var) = VT_I4;
        V_I4(&var) = i;
        IPropertyBag_Write(prop_bag, L"VFWIndex", &var);

        VariantClear(&var);
        IPropertyBag_Release(prop_bag);
    }
}

static HRESULT WINAPI devenum_factory_CreateClassEnumerator(ICreateDevEnum *iface,
        REFCLSID class, IEnumMoniker **out, DWORD flags)
{
    WCHAR guidstr[CHARS_IN_GUID];
    HRESULT hr;
    HKEY key;

    TRACE("iface %p, class %s, out %p, flags %#lx.\n", iface, debugstr_guid(class), out, flags);

    if (!out)
        return E_POINTER;

    *out = NULL;

    if (!RegOpenKeyW(HKEY_CURRENT_USER, L"Software\\Microsoft\\ActiveMovie\\devenum", &key))
    {
        StringFromGUID2(class, guidstr, ARRAY_SIZE(guidstr));
        RegDeleteTreeW(key, guidstr);
    }

    if (IsEqualGUID(class, &CLSID_LegacyAmFilterCategory))
        register_legacy_filters();
    else if (IsEqualGUID(class, &CLSID_AudioRendererCategory))
    {
        hr = DirectSoundEnumerateW(&register_dsound_devices, NULL);
        if (FAILED(hr)) return hr;
        register_waveout_devices();
        register_midiout_devices();
    }
    else if (IsEqualGUID(class, &CLSID_AudioInputDeviceCategory))
        register_wavein_devices();
    else if (IsEqualGUID(class, &CLSID_VideoCompressorCategory))
        register_vfw_codecs();
    else if (IsEqualGUID(class, &CLSID_VideoInputDeviceCategory))
        register_avicap_devices();

    if (SUCCEEDED(hr = enum_moniker_create(class, out)))
    {
        IMoniker *mon;
        hr = IEnumMoniker_Next(*out, 1, &mon, NULL);
        if (hr == S_OK)
        {
            IMoniker_Release(mon);
            IEnumMoniker_Reset(*out);
        }
        else
        {
            IEnumMoniker_Release(*out);
            *out = NULL;
        }
    }

    return hr;
}

/**********************************************************************
 * ICreateDevEnum_Vtbl
 */
static const ICreateDevEnumVtbl ICreateDevEnum_Vtbl =
{
    devenum_factory_QueryInterface,
    devenum_factory_AddRef,
    devenum_factory_Release,
    devenum_factory_CreateClassEnumerator,
};

/**********************************************************************
 * static CreateDevEnum instance
 */
ICreateDevEnum devenum_factory = { &ICreateDevEnum_Vtbl };
