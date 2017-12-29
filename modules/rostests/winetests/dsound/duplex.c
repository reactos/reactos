/*
 * Unit tests for duplex functions
 *
 * Copyright (c) 2006 Robert Reif
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

#include "dsound_test.h"

static HRESULT (WINAPI *pDirectSoundFullDuplexCreate)(LPCGUID, LPCGUID,
    LPCDSCBUFFERDESC, LPCDSBUFFERDESC, HWND, DWORD, LPDIRECTSOUNDFULLDUPLEX *,
    LPDIRECTSOUNDCAPTUREBUFFER8*, LPDIRECTSOUNDBUFFER8*, LPUNKNOWN)=NULL;

static void IDirectSoundFullDuplex_test(LPDIRECTSOUNDFULLDUPLEX dsfdo,
                                        BOOL initialized, LPCGUID lpGuidCapture,
                                        LPCGUID lpGuidRender)
{
    HRESULT rc;
    int ref;
    IUnknown * unknown;
    IDirectSound * ds;
    IDirectSound8 * ds8;
    IDirectSoundCapture * dsc;
    IDirectSoundFullDuplex * dsfd;

    /* Try to Query for objects */
    rc=IDirectSoundFullDuplex_QueryInterface(dsfdo,&IID_IUnknown,(LPVOID*)&unknown);
    ok(rc==DS_OK,"IDirectSoundFullDuplex_QueryInterface(IID_IUnknown) failed: %08x\n", rc);
    if (rc==DS_OK) {
        ref=IDirectSoundFullDuplex_Release(unknown);
        ok(ref==0, "IDirectSoundFullDuplex_Release() has %d references, "
           "should have 0\n", ref);
    }

    rc=IDirectSoundFullDuplex_QueryInterface(dsfdo,&IID_IDirectSound,(LPVOID*)&ds);
    ok(rc==(initialized?DS_OK:E_NOINTERFACE),"IDirectSoundFullDuplex_QueryInterface(IID_IDirectSound) failed: %08x\n", rc);
    if (rc==DS_OK) {
        ref=IDirectSound_Release(ds);
        ok(ref==0, "IDirectSound_Release() has %d references, "
           "should have 0\n", ref);
    }

    rc=IDirectSoundFullDuplex_QueryInterface(dsfdo,&IID_IDirectSound8,(LPVOID*)&ds8);
    ok(rc==(initialized?DS_OK:E_NOINTERFACE),"IDirectSoundFullDuplex_QueryInterface(IID_IDirectSound8) "
       "failed: %08x\n",rc);
    if (rc==DS_OK) {
        IDirectSoundFullDuplex * dsfd1;
        rc=IDirectSound8_QueryInterface(ds8,&IID_IDirectSoundFullDuplex,(LPVOID*)&dsfd1);
        ok(rc==DS_OK,"IDirectSound8_QueryInterface(IID_IDirectSoundFullDuplex) "
           "failed: %08x\n",rc);
        if (rc==DS_OK) {
            ref=IDirectSoundFullDuplex_Release(dsfd1);
            ok(ref==1, "IDirectSoundFullDuplex_Release() has %d references, "
               "should have 1\n", ref);
        }
        ref=IDirectSound8_Release(ds8);
        ok(ref==0, "IDirectSound8_Release() has %d references, "
           "should have 0\n", ref);
    }

    rc=IDirectSoundFullDuplex_QueryInterface(dsfdo,&IID_IDirectSoundCapture,(LPVOID*)&dsc);
    ok(rc==(initialized?DS_OK:E_NOINTERFACE),"IDirectSoundFullDuplex_QueryInterface(IID_IDirectSoundCapture) "
       "failed: %08x\n",rc);
    if (rc==DS_OK) {
        ref=IDirectSoundCapture_Release(dsc);
        ok(ref==0, "IDirectSoundCapture_Release() has %d references, "
           "should have 0\n", ref);
    }

    rc=IDirectSoundFullDuplex_QueryInterface(dsfdo,&IID_IDirectSoundFullDuplex,(LPVOID*)&dsfd);
    ok(rc==DS_OK,"IDirectSoundFullDuplex_QueryInterface(IID_IDirectSoundFullDuplex) "
       "failed: %08x\n",rc);
    if (rc==DS_OK) {
        ok(dsfdo==dsfd, "different interfaces\n");
        ref=IDirectSound8_Release(dsfd);
        ok(ref==1, "IDirectSoundFullDuplex_Release() has %d references, should have 1\n", ref);
    }

    ref=IDirectSoundFullDuplex_Release(dsfdo);
    ok(ref==0, "IDirectSoundFullDuplex_Release() has %d references, "
       "should have 0\n", ref);
}

static void IDirectSoundFullDuplex_tests(void)
{
    HRESULT rc;
    LPDIRECTSOUNDFULLDUPLEX dsfdo = NULL;
    DSCBUFFERDESC DSCBufferDesc;
    DSBUFFERDESC DSBufferDesc;
    LPDIRECTSOUNDCAPTUREBUFFER8 pDSCBuffer8;
    LPDIRECTSOUNDBUFFER8 pDSBuffer8;
    WAVEFORMATEX wfex;

    trace("Testing IDirectSoundFullDuplex\n");

    /* try the COM class factory method of creation with no devices specified */
    rc=CoCreateInstance(&CLSID_DirectSoundFullDuplex, NULL,
                        CLSCTX_INPROC_SERVER, &IID_IDirectSoundFullDuplex,
                        (void**)&dsfdo);
    ok(rc==S_OK||rc==REGDB_E_CLASSNOTREG||rc==CLASS_E_CLASSNOTAVAILABLE,
       "CoCreateInstance(CLSID_DirectSoundFullDuplex) failed: 0x%08x\n", rc);
    if (rc==REGDB_E_CLASSNOTREG) {
        trace("  Class Not Registered\n");
        return;
    } else if (rc==CLASS_E_CLASSNOTAVAILABLE) {
        trace("  Class Not Available\n");
        return;
    }
    if (dsfdo)
        IDirectSoundFullDuplex_test(dsfdo, FALSE, NULL, NULL);

    /* try the COM class factory method of creation with default devices
     * specified */
    rc=CoCreateInstance(&CLSID_DirectSoundFullDuplex, NULL,
                        CLSCTX_INPROC_SERVER, &IID_IDirectSoundFullDuplex,
                        (void**)&dsfdo);
    ok(rc==S_OK,"CoCreateInstance(CLSID_DirectSoundFullDuplex) failed: 0x%08x\n", rc);
    if (dsfdo)
        IDirectSoundFullDuplex_test(dsfdo, FALSE, &DSDEVID_DefaultCapture,
                                    &DSDEVID_DefaultPlayback);

    /* try the COM class factory method of creation with default voice
     * devices specified */
    rc=CoCreateInstance(&CLSID_DirectSoundFullDuplex, NULL,
                        CLSCTX_INPROC_SERVER, &IID_IDirectSoundFullDuplex,
                        (void**)&dsfdo);
    ok(rc==S_OK,"CoCreateInstance(CLSID_DirectSoundFullDuplex) failed: 0x%08x\n", rc);
    if (dsfdo)
        IDirectSoundFullDuplex_test(dsfdo, FALSE, &DSDEVID_DefaultVoiceCapture,
                                    &DSDEVID_DefaultVoicePlayback);

    /* try the COM class factory method of creation with a bad
     * IID specified */
    rc=CoCreateInstance(&CLSID_DirectSoundFullDuplex, NULL,
                        CLSCTX_INPROC_SERVER, &CLSID_DirectSoundPrivate,
                        (void**)&dsfdo);
    ok(rc==E_NOINTERFACE,
       "CoCreateInstance(CLSID_DirectSoundFullDuplex,CLSID_DirectSoundPrivate) "
       "should have failed: 0x%08x\n", rc);

    ZeroMemory(&wfex, sizeof(wfex));
    wfex.wFormatTag = WAVE_FORMAT_PCM;
    wfex.nChannels = 1;
    wfex.nSamplesPerSec = 8000;
    wfex.wBitsPerSample = 16;
    wfex.nBlockAlign = (wfex.wBitsPerSample * wfex.nChannels) / 8;
    wfex.nAvgBytesPerSec = wfex.nSamplesPerSec * wfex.nBlockAlign;

    ZeroMemory(&DSCBufferDesc, sizeof(DSCBufferDesc));
    DSCBufferDesc.dwSize = sizeof(DSCBufferDesc);
    DSCBufferDesc.dwFlags = DSCBCAPS_WAVEMAPPED;
    DSCBufferDesc.dwBufferBytes = 8192;
    DSCBufferDesc.lpwfxFormat = &wfex;

    ZeroMemory(&DSBufferDesc, sizeof(DSBufferDesc));
    DSBufferDesc.dwSize = sizeof(DSBufferDesc);
    DSBufferDesc.dwFlags = DSBCAPS_GLOBALFOCUS;
    DSBufferDesc.dwBufferBytes = 8192;
    DSBufferDesc.lpwfxFormat = &wfex;

    /* try with no device specified */
    rc=pDirectSoundFullDuplexCreate(NULL,NULL,&DSCBufferDesc,&DSBufferDesc,
                                    get_hwnd(),DSSCL_EXCLUSIVE ,&dsfdo,&pDSCBuffer8,
                                    &pDSBuffer8,NULL);
    ok(rc==DS_OK||rc==DSERR_NODRIVER||rc==DSERR_ALLOCATED||rc==E_FAIL||rc==DSERR_INVALIDCALL,
       "DirectSoundFullDuplexCreate(NULL,NULL) failed: %08x\n",rc);
    if (rc==S_OK && dsfdo)
        IDirectSoundFullDuplex_test(dsfdo, TRUE, NULL, NULL);

    /* try with default devices specified */
    rc=pDirectSoundFullDuplexCreate(&DSDEVID_DefaultCapture,
                                    &DSDEVID_DefaultPlayback,&DSCBufferDesc,
                                    &DSBufferDesc,get_hwnd(),DSSCL_EXCLUSIVE,&dsfdo,
                                    &pDSCBuffer8,&pDSBuffer8,NULL);
    ok(rc==DS_OK||rc==DSERR_NODRIVER||rc==DSERR_ALLOCATED||rc==E_FAIL||rc==DSERR_INVALIDCALL,
       "DirectSoundFullDuplexCreate(DSDEVID_DefaultCapture,"
       "DSDEVID_DefaultPlayback) failed: %08x\n", rc);
    if (rc==DS_OK && dsfdo)
        IDirectSoundFullDuplex_test(dsfdo, TRUE, NULL, NULL);

    /* try with default voice devices specified */
    rc=pDirectSoundFullDuplexCreate(&DSDEVID_DefaultVoiceCapture,
                                    &DSDEVID_DefaultVoicePlayback,
                                    &DSCBufferDesc,&DSBufferDesc,get_hwnd(),DSSCL_EXCLUSIVE,
                                    &dsfdo,&pDSCBuffer8,&pDSBuffer8,NULL);
    ok(rc==DS_OK||rc==DSERR_NODRIVER||rc==DSERR_ALLOCATED||rc==E_FAIL||rc==DSERR_INVALIDCALL,
       "DirectSoundFullDuplexCreate(DSDEVID_DefaultVoiceCapture,"
       "DSDEVID_DefaultVoicePlayback) failed: %08x\n", rc);
    if (rc==DS_OK && dsfdo)
        IDirectSoundFullDuplex_test(dsfdo, TRUE, NULL, NULL);

    /* try with bad devices specified */
    rc=pDirectSoundFullDuplexCreate(&DSDEVID_DefaultVoicePlayback,
                                    &DSDEVID_DefaultVoiceCapture,
                                    &DSCBufferDesc,&DSBufferDesc,get_hwnd(),DSSCL_EXCLUSIVE,
                                    &dsfdo,&pDSCBuffer8,&pDSBuffer8,NULL);
    ok(rc==DSERR_NODRIVER||rc==DSERR_INVALIDCALL,
       "DirectSoundFullDuplexCreate(DSDEVID_DefaultVoicePlayback,"
       "DSDEVID_DefaultVoiceCapture) should have failed: %08x\n", rc);
    if (rc==DS_OK && dsfdo)
        IDirectSoundFullDuplex_Release(dsfdo);
}

static void test_COM(void)
{
    IDirectSoundFullDuplex *dsfd = (IDirectSoundFullDuplex*)0xdeadbeef;
    IDirectSound *ds;
    IDirectSound8 *ds8;
    IDirectSoundCapture *dsc;
    IUnknown *unk, *unk8;
    IDirectSoundBuffer8 *dsb8;
    IDirectSoundCaptureBuffer8 *dscb8;
    DSBUFFERDESC bufdesc;
    DSCBUFFERDESC cbufdesc;
    WAVEFORMATEX wfx;
    ULONG refcount;
    HRESULT hr;

    /* COM aggregation */
    hr = CoCreateInstance(&CLSID_DirectSoundFullDuplex, (IUnknown*)&dsfd, CLSCTX_INPROC_SERVER,
            &IID_IUnknown, (void**)&dsfd);
    ok(hr == CLASS_E_NOAGGREGATION,
            "DirectSoundFullDuplex create failed: %08x, expected CLASS_E_NOAGGREGATION\n", hr);
    ok(!dsfd, "dsfd = %p\n", dsfd);

    /* Invalid RIID */
    hr = CoCreateInstance(&CLSID_DirectSoundFullDuplex, NULL, CLSCTX_INPROC_SERVER,
            &IID_IDirectSound3DBuffer, (void**)&dsfd);
    ok(hr == E_NOINTERFACE,
            "DirectSoundFullDuplex create failed: %08x, expected E_NOINTERFACE\n", hr);

    /* Different refcount for IDirectSoundFullDuplex and for IUnknown */
    hr = CoCreateInstance(&CLSID_DirectSoundFullDuplex, NULL, CLSCTX_INPROC_SERVER,
            &IID_IDirectSoundFullDuplex, (void**)&dsfd);
    ok(hr == S_OK, "DirectSoundFullDuplex create failed: %08x, expected S_OK\n", hr);
    refcount = IDirectSoundFullDuplex_AddRef(dsfd);
    ok(refcount == 2, "refcount == %u, expected 2\n", refcount);
    hr = IDirectSoundFullDuplex_QueryInterface(dsfd, &IID_IUnknown, (void**)&unk);
    ok(hr == S_OK, "QueryInterface for IID_IUnknown failed: %08x\n", hr);
    refcount = IUnknown_AddRef(unk);
    ok(refcount == 2, "refcount == %u, expected 2\n", refcount);

    /* Not initialized */
    hr = IDirectSoundFullDuplex_QueryInterface(dsfd, &IID_IDirectSound8, (void**)&ds8);
    ok(hr == E_NOINTERFACE,
            "QueryInterface for IID_IDirectSound8 failed: %08x, expected E_NOINTERFACE\n", hr);
    hr = IDirectSoundFullDuplex_QueryInterface(dsfd, &IID_IDirectSoundCapture, (void**)&dsc);
    ok(hr == E_NOINTERFACE,
            "QueryInterface for IID_IDirectSoundCapture failed: %08x, expected E_NOINTERFACE\n", hr);

    init_format(&wfx, WAVE_FORMAT_PCM, 44100, 16, 1);
    ZeroMemory(&bufdesc, sizeof(bufdesc));
    bufdesc.dwSize = sizeof(bufdesc);
    bufdesc.dwBufferBytes = wfx.nAvgBytesPerSec;
    bufdesc.lpwfxFormat = &wfx;
    ZeroMemory(&cbufdesc, sizeof(cbufdesc));
    cbufdesc.dwSize = sizeof(cbufdesc);
    cbufdesc.dwBufferBytes = wfx.nAvgBytesPerSec;
    cbufdesc.lpwfxFormat = &wfx;
    hr = IDirectSoundFullDuplex_Initialize(dsfd, NULL, NULL, &cbufdesc, &bufdesc, get_hwnd(),
            DSSCL_EXCLUSIVE, NULL, NULL);
    ok(hr == E_INVALIDARG,
            "IDirectSoundFullDuplex_Initialize failed: %08x, expected E_INVALIDARG\n", hr);
    hr = IDirectSoundFullDuplex_Initialize(dsfd, NULL, NULL, &cbufdesc, &bufdesc, get_hwnd(),
            DSSCL_EXCLUSIVE, &dscb8, &dsb8);
    if (hr == DSERR_NODRIVER || hr == DSERR_INVALIDCALL) {
        skip("No driver\n");
        return;
    }
    ok(hr == S_OK, "IDirectSoundFullDuplex_Initialize failed: %08x\n", hr);

    /* IDirectSound and IDirectSound8 */
    hr = IDirectSoundFullDuplex_QueryInterface(dsfd, &IID_IDirectSound8, (void**)&ds8);
    ok(hr == S_OK, "QueryInterface for IID_IDirectSound8 failed: %08x\n", hr);
    refcount = IDirectSound8_AddRef(ds8);
    ok(refcount == 2, "refcount == %u, expected 2\n", refcount);
    hr = IDirectSoundFullDuplex_QueryInterface(dsfd, &IID_IDirectSound, (void**)&ds);
    ok(hr == S_OK, "QueryInterface for IID_IDirectSound failed: %08x\n", hr);
    refcount = IDirectSound8_AddRef(ds8);
    ok(refcount == 4, "refcount == %u, expected 4\n", refcount);
    refcount = IDirectSound_AddRef(ds);
    ok(refcount == 5, "refcount == %u, expected 5\n", refcount);
    hr = IDirectSound8_QueryInterface(ds8, &IID_IUnknown, (void**)&unk8);
    ok(hr == S_OK, "QueryInterface for IID_IUnknown failed: %08x\n", hr);
    ok(unk == unk8, "Got different IUnknown when QI'ing IDirectSoundFullDuplex and IDirectSound\n");
    refcount = IUnknown_AddRef(unk8);
    ok(refcount == 4, "refcount == %u, expected 4\n", refcount);
    refcount = IDirectSound_AddRef(ds);
    ok(refcount == 6, "refcount == %u, expected 6\n", refcount);
    refcount = IDirectSoundFullDuplex_AddRef(dsfd);
    ok(refcount == 3, "refcount == %u, expected 3\n", refcount);

    /* IDirectSoundCapture */
    hr = IDirectSoundFullDuplex_QueryInterface(dsfd, &IID_IDirectSoundCapture, (void**)&dsc);
    ok(hr == S_OK, "QueryInterface for IID_IDirectSoundCapture failed: %08x\n", hr);
    refcount = IDirectSoundCapture_AddRef(dsc);
    ok(refcount == 2, "refcount == %u, expected 2\n", refcount);
    refcount = IDirectSoundFullDuplex_AddRef(dsfd);
    ok(refcount == 4, "refcount == %u, expected 4\n", refcount);
    hr = IDirectSoundCapture_QueryInterface(ds8, &IID_IUnknown, (void**)&unk8);
    ok(hr == S_OK, "QueryInterface for IID_IUnknown failed: %08x\n", hr);
    ok(unk == unk8,
            "Got different IUnknown when QI'ing IDirectSoundFullDuplex and IDirectSoundCapture\n");
    refcount = IUnknown_AddRef(unk8);
    ok(refcount == 6, "refcount == %u, expected 6\n", refcount);

    IDirectSoundBuffer8_Release(dsb8);
    IDirectSoundCaptureBuffer8_Release(dscb8);
    while (IDirectSound8_Release(ds8));
    while (IDirectSoundCapture_Release(dsc));
    while (IDirectSoundFullDuplex_Release(dsfd));
    while (IUnknown_Release(unk));
}

START_TEST(duplex)
{
    HMODULE hDsound;

    CoInitialize(NULL);

    hDsound = LoadLibrary("dsound.dll");
    if (hDsound)
    {

        pDirectSoundFullDuplexCreate=(void*)GetProcAddress(hDsound,
            "DirectSoundFullDuplexCreate");
        if (pDirectSoundFullDuplexCreate) {
            test_COM();
            IDirectSoundFullDuplex_tests();
        } else
            skip("DirectSoundFullDuplexCreate missing - skipping all tests\n");

        FreeLibrary(hDsound);
    }
    else
        skip("dsound.dll not found - skipping all tests\n");

    CoUninitialize();
}
