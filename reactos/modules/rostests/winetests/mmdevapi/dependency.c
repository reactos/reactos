/*
 * Copyright 2009 Maarten Lankhorst
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

#include "wine/test.h"

#define COBJMACROS

#ifdef STANDALONE
#include "initguid.h"
#endif

#include "unknwn.h"
#include "uuids.h"
#include "mmdeviceapi.h"
#include "dshow.h"
#include "dsound.h"

START_TEST(dependency)
{
    HRESULT hr;
    IMMDeviceEnumerator *mme = NULL;
    IMMDevice *dev = NULL;
    IDirectSound8 *ds8 = NULL;
    IBaseFilter *bf = NULL;

    CoInitializeEx(NULL, COINIT_MULTITHREADED);
    hr = CoCreateInstance(&CLSID_MMDeviceEnumerator, NULL, CLSCTX_INPROC_SERVER, &IID_IMMDeviceEnumerator, (void**)&mme);
    if (FAILED(hr))
    {
        skip("mmdevapi not available: 0x%08x\n", hr);
        goto cleanup;
    }

    hr = IMMDeviceEnumerator_GetDefaultAudioEndpoint(mme, eRender, eMultimedia, &dev);
    ok(hr == S_OK || hr == E_NOTFOUND, "GetDefaultAudioEndpoint failed: 0x%08x\n", hr);
    if (hr != S_OK)
    {
        if (hr == E_NOTFOUND)
            skip("No sound card available\n");
        else
            skip("GetDefaultAudioEndpoint returns 0x%08x\n", hr);
        goto cleanup;
    }

    ok(!GetModuleHandleA("dsound.dll"), "dsound.dll was already loaded!\n");

    hr = IMMDevice_Activate(dev, &IID_IDirectSound8, CLSCTX_INPROC_SERVER, NULL, (void**)&ds8);
    ok(hr == S_OK, "Activating ds8 interface failed: 0x%08x\n", hr);
    if (hr == S_OK)
    {
        ok(GetModuleHandleA("dsound.dll") != NULL, "dsound.dll not loaded!\n");
        ok(ds8 != NULL, "ds8 pointer is null\n");
    }
    if (ds8)
        IDirectSound8_Release(ds8);

    ok(!GetModuleHandleA("quartz.dll"), "quartz.dll was already loaded!\n");
    hr = IMMDevice_Activate(dev, &IID_IBaseFilter, CLSCTX_INPROC_SERVER, NULL, (void**)&bf);
    ok(hr == S_OK, "Activating bf failed: 0x%08x\n", hr);
    if (hr == S_OK)
    {
        ok(GetModuleHandleA("quartz.dll") != NULL, "quartz.dll not loaded!\n");
        ok(bf != NULL, "bf pointer is null\n");
        if (bf)
        {
            CLSID clsid;
            hr = IBaseFilter_GetClassID(bf, &clsid);
            ok(hr == S_OK, "GetClassId failed with 0x%08x\n", hr);
            if (hr == S_OK)
                ok(IsEqualCLSID(&clsid, &CLSID_DSoundRender), "Wrong class id %s\n", wine_dbgstr_guid(&clsid));
        }
    }

cleanup:
    if (bf)
        IBaseFilter_Release(bf);
    if (dev)
        IMMDevice_Release(dev);
    if (mme)
        IMMDeviceEnumerator_Release(mme);

    CoUninitialize();
}
