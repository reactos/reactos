/*
 * Some unit tests for devenum
 *
 * Copyright (C) 2012 Christian Costa
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

#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H

#define COBJMACROS

//#include <stdio.h>

#include <wine/test.h>
#include <wingdi.h>
#include <initguid.h>
#include <ole2.h>
#include <strmif.h>
#include <uuids.h>

static const WCHAR friendly_name[] = {'F','r','i','e','n','d','l','y','N','a','m','e',0};
static const WCHAR fcc_handlerW[] = {'F','c','c','H','a','n','d','l','e','r',0};
static const WCHAR mrleW[] = {'m','r','l','e',0};

struct category
{
    const char * name;
    const GUID * clsid;
};

static struct category am_categories[] =
{
    { "Legacy AM Filter category", &CLSID_LegacyAmFilterCategory },
    { "Audio renderer category", &CLSID_AudioRendererCategory },
    { "Midi renderer category", &CLSID_MidiRendererCategory },
    { "Audio input device category", &CLSID_AudioInputDeviceCategory },
    { "Video input device category", &CLSID_VideoInputDeviceCategory },
    { "Audio compressor category", &CLSID_AudioCompressorCategory },
    { "Video compressor category", &CLSID_VideoCompressorCategory }
};

static void test_devenum(IBindCtx *bind_ctx)
{
    HRESULT res;
    ICreateDevEnum* create_devenum;
    IEnumMoniker* enum_moniker = NULL;
    BOOL have_mrle = FALSE;
    int i;

    res = CoCreateInstance(&CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC,
                           &IID_ICreateDevEnum, (LPVOID*)&create_devenum);
    if (res != S_OK) {
        skip("Cannot create SystemDeviceEnum object (%x)\n", res);
        return;
    }

    for (i = 0; i < (sizeof(am_categories) / sizeof(struct category)); i++)
    {
        if (winetest_debug > 1)
            trace("%s:\n", am_categories[i].name);

        res = ICreateDevEnum_CreateClassEnumerator(create_devenum, am_categories[i].clsid, &enum_moniker, 0);
        ok(SUCCEEDED(res), "Cannot create enum moniker (res = %x)\n", res);
        if (res == S_OK)
        {
            IMoniker* moniker;
            while (IEnumMoniker_Next(enum_moniker, 1, &moniker, NULL) == S_OK)
            {
                IPropertyBag* prop_bag = NULL;
                VARIANT var;
                HRESULT hr;

                VariantInit(&var);
                hr = IMoniker_BindToStorage(moniker, bind_ctx, NULL, &IID_IPropertyBag, (LPVOID*)&prop_bag);
                ok(hr == S_OK, "IMoniker_BindToStorage failed with error %x\n", hr);

                if (SUCCEEDED(hr))
                {
                    hr = IPropertyBag_Read(prop_bag, friendly_name, &var, NULL);
                    ok((hr == S_OK) || broken(hr == 0x80070002), "IPropertyBag_Read failed with error %x\n", hr);

                    if (SUCCEEDED(hr))
                    {
                        if (winetest_debug > 1)
                            trace("  %s\n", wine_dbgstr_w(V_BSTR(&var)));
                        VariantClear(&var);
                    }
                    else
                    {
                        trace("  ???\n");
                    }

                    if (IsEqualGUID(&CLSID_VideoCompressorCategory, am_categories[i].clsid)) {
                        /* Test well known compressor to ensure that we really enumerate codecs */
                        hr = IPropertyBag_Read(prop_bag, fcc_handlerW, &var, NULL);
                        if (SUCCEEDED(hr)) {
                            ok(V_VT(&var) == VT_BSTR, "V_VT(var) = %d\n", V_VT(&var));
                            if(!lstrcmpW(V_BSTR(&var), mrleW))
                                have_mrle = TRUE;
                            VariantClear(&var);
                        }
                    }
                }

                if (prop_bag)
                    IPropertyBag_Release(prop_bag);
                IMoniker_Release(moniker);
            }
            IEnumMoniker_Release(enum_moniker);
        }
    }

    ICreateDevEnum_Release(create_devenum);

    /* 64-bit windows are missing mrle codec */
    if(sizeof(void*) == 4)
        ok(have_mrle, "mrle codec not found\n");
}

/* CLSID_CDeviceMoniker */

START_TEST(devenum)
{
    IBindCtx *bind_ctx = NULL;
    HRESULT hr;

    CoInitialize(NULL);

    test_devenum(NULL);

    /* IBindCtx is allowed in IMoniker_BindToStorage (IMediaCatMoniker_BindToStorage) */
    hr = CreateBindCtx(0, &bind_ctx);
    ok(hr == S_OK, "Cannot create BindCtx: (res = 0x%x)\n", hr);
    if (bind_ctx) {
        test_devenum(bind_ctx);
        IBindCtx_Release(bind_ctx);
    }

    CoUninitialize();
}
