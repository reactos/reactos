/*
 * Copyright 2010 Maarten Lankhorst for CodeWeavers
 *      2011-2012 Jörg Höhle
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

/* This test is for audio playback specific mechanisms
 * Tests:
 * - IAudioClient with eRender and IAudioRenderClient
 */

#include <math.h>
#include <stdio.h>

#include "wine/test.h"

#define COBJMACROS

#ifdef STANDALONE
#include "initguid.h"
#endif

#include "unknwn.h"
#include "uuids.h"
#include "mmdeviceapi.h"
#include "mmsystem.h"
#include "audioclient.h"
#include "audiopolicy.h"

static const unsigned int win_formats[][4] = {
    { 8000,  8, 1},   { 8000,  8, 2},   { 8000, 16, 1},   { 8000, 16, 2},
    {11025,  8, 1},   {11025,  8, 2},   {11025, 16, 1},   {11025, 16, 2},
    {12000,  8, 1},   {12000,  8, 2},   {12000, 16, 1},   {12000, 16, 2},
    {16000,  8, 1},   {16000,  8, 2},   {16000, 16, 1},   {16000, 16, 2},
    {22050,  8, 1},   {22050,  8, 2},   {22050, 16, 1},   {22050, 16, 2},
    {44100,  8, 1},   {44100,  8, 2},   {44100, 16, 1},   {44100, 16, 2},
    {48000,  8, 1},   {48000,  8, 2},   {48000, 16, 1},   {48000, 16, 2},
    {96000,  8, 1},   {96000,  8, 2},   {96000, 16, 1},   {96000, 16, 2}
};
#define NB_WIN_FORMATS (sizeof(win_formats)/sizeof(*win_formats))

#define NULL_PTR_ERR MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, RPC_X_NULL_REF_POINTER)

static IMMDeviceEnumerator *mme = NULL;
static IMMDevice *dev = NULL;
static HRESULT hexcl = S_OK; /* or AUDCLNT_E_EXCLUSIVE_MODE_NOT_ALLOWED */

static const LARGE_INTEGER ullZero;

#define PI 3.14159265358979323846L
static DWORD wave_generate_tone(PWAVEFORMATEX pwfx, BYTE* data, UINT32 frames)
{
    static double phase = 0.; /* normalized to unit, not 2*PI */
    PWAVEFORMATEXTENSIBLE wfxe = (PWAVEFORMATEXTENSIBLE)pwfx;
    DWORD cn, i;
    double delta, y;

    if(!winetest_interactive)
        return AUDCLNT_BUFFERFLAGS_SILENT;
    if(wfxe->Format.wBitsPerSample != ((wfxe->Format.wFormatTag == WAVE_FORMAT_EXTENSIBLE &&
       IsEqualGUID(&wfxe->SubFormat, &KSDATAFORMAT_SUBTYPE_IEEE_FLOAT)) ? 8 * sizeof(float) : 16))
        return AUDCLNT_BUFFERFLAGS_SILENT;

    for(delta = phase, cn = 0; cn < wfxe->Format.nChannels;
        delta += .5/wfxe->Format.nChannels, cn++){
        for(i = 0; i < frames; i++){
            y = sin(2*PI*(440.* i / wfxe->Format.nSamplesPerSec + delta));
            /* assume alignment is granted */
            if(wfxe->Format.wBitsPerSample == 16)
                ((short*)data)[cn+i*wfxe->Format.nChannels] = y * 32767.9;
            else
                ((float*)data)[cn+i*wfxe->Format.nChannels] = y;
        }
    }
    phase += 440.* frames / wfxe->Format.nSamplesPerSec;
    phase -= floor(phase);
    return 0;
}

static void test_uninitialized(IAudioClient *ac)
{
    HRESULT hr;
    UINT32 num;
    REFERENCE_TIME t1;

    HANDLE handle = CreateEventW(NULL, FALSE, FALSE, NULL);
    IUnknown *unk;

    hr = IAudioClient_GetBufferSize(ac, &num);
    ok(hr == AUDCLNT_E_NOT_INITIALIZED, "Uninitialized GetBufferSize call returns %08x\n", hr);

    hr = IAudioClient_GetStreamLatency(ac, &t1);
    ok(hr == AUDCLNT_E_NOT_INITIALIZED, "Uninitialized GetStreamLatency call returns %08x\n", hr);

    hr = IAudioClient_GetCurrentPadding(ac, &num);
    ok(hr == AUDCLNT_E_NOT_INITIALIZED, "Uninitialized GetCurrentPadding call returns %08x\n", hr);

    hr = IAudioClient_Start(ac);
    ok(hr == AUDCLNT_E_NOT_INITIALIZED, "Uninitialized Start call returns %08x\n", hr);

    hr = IAudioClient_Stop(ac);
    ok(hr == AUDCLNT_E_NOT_INITIALIZED, "Uninitialized Stop call returns %08x\n", hr);

    hr = IAudioClient_Reset(ac);
    ok(hr == AUDCLNT_E_NOT_INITIALIZED, "Uninitialized Reset call returns %08x\n", hr);

    hr = IAudioClient_SetEventHandle(ac, handle);
    ok(hr == AUDCLNT_E_NOT_INITIALIZED, "Uninitialized SetEventHandle call returns %08x\n", hr);

    hr = IAudioClient_GetService(ac, &IID_IAudioStreamVolume, (void**)&unk);
    ok(hr == AUDCLNT_E_NOT_INITIALIZED, "Uninitialized GetService call returns %08x\n", hr);

    CloseHandle(handle);
}

static void test_audioclient(void)
{
    IAudioClient *ac;
    IUnknown *unk;
    HRESULT hr;
    ULONG ref;
    WAVEFORMATEX *pwfx, *pwfx2;
    REFERENCE_TIME t1, t2;
    HANDLE handle;

    hr = IMMDevice_Activate(dev, &IID_IAudioClient, CLSCTX_INPROC_SERVER,
            NULL, (void**)&ac);
    ok(hr == S_OK, "Activation failed with %08x\n", hr);
    if(hr != S_OK)
        return;

    handle = CreateEventW(NULL, FALSE, FALSE, NULL);

    hr = IAudioClient_QueryInterface(ac, &IID_IUnknown, NULL);
    ok(hr == E_POINTER, "QueryInterface(NULL) returned %08x\n", hr);

    unk = (void*)(LONG_PTR)0x12345678;
    hr = IAudioClient_QueryInterface(ac, &IID_NULL, (void**)&unk);
    ok(hr == E_NOINTERFACE, "QueryInterface(IID_NULL) returned %08x\n", hr);
    ok(!unk, "QueryInterface(IID_NULL) returned non-null pointer %p\n", unk);

    hr = IAudioClient_QueryInterface(ac, &IID_IUnknown, (void**)&unk);
    ok(hr == S_OK, "QueryInterface(IID_IUnknown) returned %08x\n", hr);
    if (unk)
    {
        ref = IUnknown_Release(unk);
        ok(ref == 1, "Released count is %u\n", ref);
    }

    hr = IAudioClient_QueryInterface(ac, &IID_IAudioClient, (void**)&unk);
    ok(hr == S_OK, "QueryInterface(IID_IAudioClient) returned %08x\n", hr);
    if (unk)
    {
        ref = IUnknown_Release(unk);
        ok(ref == 1, "Released count is %u\n", ref);
    }

    hr = IAudioClient_GetDevicePeriod(ac, NULL, NULL);
    ok(hr == E_POINTER, "Invalid GetDevicePeriod call returns %08x\n", hr);

    hr = IAudioClient_GetDevicePeriod(ac, &t1, NULL);
    ok(hr == S_OK, "Valid GetDevicePeriod call returns %08x\n", hr);

    hr = IAudioClient_GetDevicePeriod(ac, NULL, &t2);
    ok(hr == S_OK, "Valid GetDevicePeriod call returns %08x\n", hr);

    hr = IAudioClient_GetDevicePeriod(ac, &t1, &t2);
    ok(hr == S_OK, "Valid GetDevicePeriod call returns %08x\n", hr);
    trace("Returned periods: %u.%04u ms %u.%04u ms\n",
          (UINT)(t1/10000), (UINT)(t1 % 10000),
          (UINT)(t2/10000), (UINT)(t2 % 10000));

    hr = IAudioClient_GetMixFormat(ac, NULL);
    ok(hr == E_POINTER, "GetMixFormat returns %08x\n", hr);

    hr = IAudioClient_GetMixFormat(ac, &pwfx);
    ok(hr == S_OK, "Valid GetMixFormat returns %08x\n", hr);

    if (hr == S_OK)
    {
        trace("pwfx: %p\n", pwfx);
        trace("Tag: %04x\n", pwfx->wFormatTag);
        trace("bits: %u\n", pwfx->wBitsPerSample);
        trace("chan: %u\n", pwfx->nChannels);
        trace("rate: %u\n", pwfx->nSamplesPerSec);
        trace("align: %u\n", pwfx->nBlockAlign);
        trace("extra: %u\n", pwfx->cbSize);
        ok(pwfx->wFormatTag == WAVE_FORMAT_EXTENSIBLE, "wFormatTag is %x\n", pwfx->wFormatTag);
        if (pwfx->wFormatTag == WAVE_FORMAT_EXTENSIBLE)
        {
            WAVEFORMATEXTENSIBLE *pwfxe = (void*)pwfx;
            trace("Res: %u\n", pwfxe->Samples.wReserved);
            trace("Mask: %x\n", pwfxe->dwChannelMask);
            trace("Alg: %s\n",
                  IsEqualGUID(&pwfxe->SubFormat, &KSDATAFORMAT_SUBTYPE_PCM)?"PCM":
                  (IsEqualGUID(&pwfxe->SubFormat,
                               &KSDATAFORMAT_SUBTYPE_IEEE_FLOAT)?"FLOAT":"Other"));
        }

        hr = IAudioClient_IsFormatSupported(ac, AUDCLNT_SHAREMODE_SHARED, pwfx, &pwfx2);
        ok(hr == S_OK, "Valid IsFormatSupported(Shared) call returns %08x\n", hr);
        ok(pwfx2 == NULL, "pwfx2 is non-null\n");
        CoTaskMemFree(pwfx2);

        hr = IAudioClient_IsFormatSupported(ac, AUDCLNT_SHAREMODE_SHARED, NULL, NULL);
        ok(hr == E_POINTER, "IsFormatSupported(NULL) call returns %08x\n", hr);

        hr = IAudioClient_IsFormatSupported(ac, AUDCLNT_SHAREMODE_SHARED, pwfx, NULL);
        ok(hr == E_POINTER, "IsFormatSupported(Shared,NULL) call returns %08x\n", hr);

        hr = IAudioClient_IsFormatSupported(ac, AUDCLNT_SHAREMODE_EXCLUSIVE, pwfx, NULL);
        ok(hr == S_OK || hr == AUDCLNT_E_UNSUPPORTED_FORMAT || hr == AUDCLNT_E_EXCLUSIVE_MODE_NOT_ALLOWED,
           "IsFormatSupported(Exclusive) call returns %08x\n", hr);
        hexcl = hr;

        pwfx2 = (WAVEFORMATEX*)0xDEADF00D;
        hr = IAudioClient_IsFormatSupported(ac, AUDCLNT_SHAREMODE_EXCLUSIVE, pwfx, &pwfx2);
        ok(hr == hexcl, "IsFormatSupported(Exclusive) call returns %08x\n", hr);
        ok(pwfx2 == NULL, "pwfx2 non-null on exclusive IsFormatSupported\n");

        if (hexcl != AUDCLNT_E_EXCLUSIVE_MODE_NOT_ALLOWED)
            hexcl = S_OK;

        hr = IAudioClient_IsFormatSupported(ac, 0xffffffff, pwfx, NULL);
        ok(hr == E_INVALIDARG/*w32*/ ||
           broken(hr == AUDCLNT_E_UNSUPPORTED_FORMAT/*w64 response from exclusive mode driver */),
           "IsFormatSupported(0xffffffff) call returns %08x\n", hr);
    }

    test_uninitialized(ac);

    hr = IAudioClient_Initialize(ac, 3, 0, 5000000, 0, pwfx, NULL);
    ok(hr == AUDCLNT_E_NOT_INITIALIZED, "Initialize with invalid sharemode returns %08x\n", hr);

    hr = IAudioClient_Initialize(ac, AUDCLNT_SHAREMODE_SHARED, 0xffffffff, 5000000, 0, pwfx, NULL);
    ok(hr == E_INVALIDARG ||
            hr == AUDCLNT_E_INVALID_STREAM_FLAG, "Initialize with invalid flags returns %08x\n", hr);

    /* A period != 0 is ignored and the call succeeds.
     * Since we can only initialize successfully once, skip those tests.
     */
    hr = IAudioClient_Initialize(ac, AUDCLNT_SHAREMODE_SHARED, 0, 5000000, 0, NULL, NULL);
    ok(hr == E_POINTER, "Initialize with null format returns %08x\n", hr);

    hr = IAudioClient_Initialize(ac, AUDCLNT_SHAREMODE_SHARED, 0, 0, 0, pwfx, NULL);
    ok(hr == S_OK, "Initialize with 0 buffer size returns %08x\n", hr);
    if(hr == S_OK){
        UINT32 num;

        hr = IAudioClient_GetBufferSize(ac, &num);
        ok(hr == S_OK, "GetBufferSize from duration 0 returns %08x\n", hr);
        if(hr == S_OK)
            trace("Initialize(duration=0) GetBufferSize is %u\n", num);
    }

    IAudioClient_Release(ac);

    hr = IMMDevice_Activate(dev, &IID_IAudioClient, CLSCTX_INPROC_SERVER,
            NULL, (void**)&ac);
    ok(hr == S_OK, "Activation failed with %08x\n", hr);

    if(pwfx->wFormatTag == WAVE_FORMAT_EXTENSIBLE){
        WAVEFORMATEXTENSIBLE *fmtex = (WAVEFORMATEXTENSIBLE*)pwfx;
        WAVEFORMATEX *fmt2 = NULL;

        ok(fmtex->dwChannelMask != 0, "Got empty dwChannelMask\n");

        fmtex->dwChannelMask = 0xffff;

        hr = IAudioClient_Initialize(ac, AUDCLNT_SHAREMODE_SHARED, 0, 5000000, 0, pwfx, NULL);
        ok(hr == S_OK, "Initialize(dwChannelMask = 0xffff) returns %08x\n", hr);

        IAudioClient_Release(ac);

        hr = IMMDevice_Activate(dev, &IID_IAudioClient, CLSCTX_INPROC_SERVER,
                NULL, (void**)&ac);
        ok(hr == S_OK, "Activation failed with %08x\n", hr);

        fmtex->dwChannelMask = 0;

        hr = IAudioClient_IsFormatSupported(ac, AUDCLNT_SHAREMODE_SHARED, pwfx, &fmt2);
        ok(hr == S_OK || broken(hr == S_FALSE /* w7 Realtek HDA */),
           "IsFormatSupported(dwChannelMask = 0) call returns %08x\n", hr);
        ok(fmtex->dwChannelMask == 0, "Passed format was modified\n");

        CoTaskMemFree(fmt2);

        hr = IAudioClient_Initialize(ac, AUDCLNT_SHAREMODE_SHARED, 0, 5000000, 0, pwfx, NULL);
        ok(hr == S_OK, "Initialize(dwChannelMask = 0) returns %08x\n", hr);

        IAudioClient_Release(ac);

        hr = IMMDevice_Activate(dev, &IID_IAudioClient, CLSCTX_INPROC_SERVER,
                NULL, (void**)&ac);
        ok(hr == S_OK, "Activation failed with %08x\n", hr);

        CoTaskMemFree(pwfx);

        hr = IAudioClient_GetMixFormat(ac, &pwfx);
        ok(hr == S_OK, "Valid GetMixFormat returns %08x\n", hr);
    }else
        skip("Skipping dwChannelMask tests\n");

    hr = IAudioClient_Initialize(ac, AUDCLNT_SHAREMODE_SHARED, 0, 5000000, 0, pwfx, NULL);
    ok(hr == S_OK, "Valid Initialize returns %08x\n", hr);
    if (hr != S_OK)
    {
        IAudioClient_Release(ac);
        CoTaskMemFree(pwfx);
        return;
    }

    hr = IAudioClient_GetStreamLatency(ac, NULL);
    ok(hr == E_POINTER, "GetStreamLatency(NULL) call returns %08x\n", hr);

    hr = IAudioClient_GetStreamLatency(ac, &t2);
    ok(hr == S_OK, "Valid GetStreamLatency call returns %08x\n", hr);
    trace("Returned latency: %u.%04u ms\n",
          (UINT)(t2/10000), (UINT)(t2 % 10000));
    ok(t2 >= t1 || broken(t2 >= t1/2 && pwfx->nSamplesPerSec > 48000),
       "Latency < default period, delta %ldus\n", (long)((t2-t1)/10));
    /* Native appears to add the engine period to the HW latency in shared mode */

    hr = IAudioClient_Initialize(ac, AUDCLNT_SHAREMODE_SHARED, 0, 5000000, 0, pwfx, NULL);
    ok(hr == AUDCLNT_E_ALREADY_INITIALIZED, "Calling Initialize twice returns %08x\n", hr);

    hr = IAudioClient_SetEventHandle(ac, NULL);
    ok(hr == E_INVALIDARG, "SetEventHandle(NULL) returns %08x\n", hr);

    hr = IAudioClient_SetEventHandle(ac, handle);
    ok(hr == AUDCLNT_E_EVENTHANDLE_NOT_EXPECTED ||
       broken(hr == HRESULT_FROM_WIN32(ERROR_INVALID_NAME)) ||
       broken(hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND)) /* Some 2k8 */ ||
       broken(hr == HRESULT_FROM_WIN32(ERROR_BAD_PATHNAME)) /* Some Vista */
       , "SetEventHandle returns %08x\n", hr);

    hr = IAudioClient_Reset(ac);
    ok(hr == S_OK, "Reset on an initialized stream returns %08x\n", hr);

    hr = IAudioClient_Reset(ac);
    ok(hr == S_OK, "Reset on a resetted stream returns %08x\n", hr);

    hr = IAudioClient_Stop(ac);
    ok(hr == S_FALSE, "Stop on a stopped stream returns %08x\n", hr);

    hr = IAudioClient_Start(ac);
    ok(hr == S_OK, "Start on a stopped stream returns %08x\n", hr);

    hr = IAudioClient_Start(ac);
    ok(hr == AUDCLNT_E_NOT_STOPPED, "Start twice returns %08x\n", hr);

    IAudioClient_Release(ac);

    CloseHandle(handle);
    CoTaskMemFree(pwfx);
}

static void test_formats(AUDCLNT_SHAREMODE mode)
{
    IAudioClient *ac;
    HRESULT hr, hrs;
    WAVEFORMATEX fmt, *pwfx, *pwfx2;
    int i;

    fmt.wFormatTag = WAVE_FORMAT_PCM;
    fmt.cbSize = 0;

    for(i = 0; i < NB_WIN_FORMATS; i++) {
        hr = IMMDevice_Activate(dev, &IID_IAudioClient, CLSCTX_INPROC_SERVER,
                NULL, (void**)&ac);
        ok(hr == S_OK, "Activation failed with %08x\n", hr);
        if(hr != S_OK)
            continue;

        hr = IAudioClient_GetMixFormat(ac, &pwfx);
        ok(hr == S_OK, "GetMixFormat failed: %08x\n", hr);

        fmt.nSamplesPerSec = win_formats[i][0];
        fmt.wBitsPerSample = win_formats[i][1];
        fmt.nChannels      = win_formats[i][2];
        fmt.nBlockAlign    = fmt.nChannels * fmt.wBitsPerSample / 8;
        fmt.nAvgBytesPerSec= fmt.nBlockAlign * fmt.nSamplesPerSec;

        pwfx2 = (WAVEFORMATEX*)0xDEADF00D;
        hr = IAudioClient_IsFormatSupported(ac, mode, &fmt, &pwfx2);
        hrs = hr;
        /* Only shared mode suggests something ... GetMixFormat! */
        ok(hr == S_OK || (mode == AUDCLNT_SHAREMODE_SHARED
           ? hr == S_FALSE || broken(hr == AUDCLNT_E_UNSUPPORTED_FORMAT &&
               /* 5:1 card exception when asked for 1 channel at mixer rate */
               pwfx->nChannels > 2 && fmt.nSamplesPerSec == pwfx->nSamplesPerSec)
           : (hr == AUDCLNT_E_UNSUPPORTED_FORMAT || hr == hexcl)),
           "IsFormatSupported(%d, %ux%2ux%u) returns %08x\n", mode,
           fmt.nSamplesPerSec, fmt.wBitsPerSample, fmt.nChannels, hr);
        if (hr == S_OK)
            trace("IsSupported(%s, %ux%2ux%u)\n",
                  mode == AUDCLNT_SHAREMODE_SHARED ? "shared " : "exclus.",
                  fmt.nSamplesPerSec, fmt.wBitsPerSample, fmt.nChannels);

        /* Change GetMixFormat wBitsPerSample only => S_OK */
        if (mode == AUDCLNT_SHAREMODE_SHARED
            && fmt.nSamplesPerSec == pwfx->nSamplesPerSec
            && fmt.nChannels == pwfx->nChannels)
            ok(hr == S_OK, "Varying BitsPerSample %u\n", fmt.wBitsPerSample);

        ok((hr == S_FALSE)^(pwfx2 == NULL), "hr %x<->suggest %p\n", hr, pwfx2);
        if (pwfx2 == (WAVEFORMATEX*)0xDEADF00D)
            pwfx2 = NULL; /* broken in Wine < 1.3.28 */
        if (pwfx2) {
            ok(pwfx2->nSamplesPerSec == pwfx->nSamplesPerSec &&
               pwfx2->nChannels      == pwfx->nChannels &&
               pwfx2->wBitsPerSample == pwfx->wBitsPerSample,
               "Suggestion %ux%2ux%u differs from GetMixFormat\n",
               pwfx2->nSamplesPerSec, pwfx2->wBitsPerSample, pwfx2->nChannels);
        }

        /* Vista returns E_INVALIDARG upon AUDCLNT_STREAMFLAGS_RATEADJUST */
        hr = IAudioClient_Initialize(ac, mode, 0, 5000000, 0, &fmt, NULL);
        if ((hrs == S_OK) ^ (hr == S_OK))
            trace("Initialize (%s, %ux%2ux%u) returns %08x unlike IsFormatSupported\n",
                  mode == AUDCLNT_SHAREMODE_SHARED ? "shared " : "exclus.",
                  fmt.nSamplesPerSec, fmt.wBitsPerSample, fmt.nChannels, hr);
        if (mode == AUDCLNT_SHAREMODE_SHARED)
            ok(hrs == S_OK ? hr == S_OK : hr == AUDCLNT_E_UNSUPPORTED_FORMAT,
               "Initialize(shared,  %ux%2ux%u) returns %08x\n",
               fmt.nSamplesPerSec, fmt.wBitsPerSample, fmt.nChannels, hr);
        else if (hrs == AUDCLNT_E_EXCLUSIVE_MODE_NOT_ALLOWED)
            /* Unsupported format implies "create failed" and shadows "not allowed" */
            ok(hrs == hexcl && (hr == AUDCLNT_E_ENDPOINT_CREATE_FAILED || hr == hrs),
               "Initialize(noexcl., %ux%2ux%u) returns %08x(%08x)\n",
               fmt.nSamplesPerSec, fmt.wBitsPerSample, fmt.nChannels, hr, hrs);
        else
            /* On testbot 48000x16x1 claims support, but does not Initialize.
             * Some cards Initialize 44100|48000x16x1 yet claim no support;
             * F. Gouget's w7 bots do that for 12000|96000x8|16x1|2 */
            ok(hrs == S_OK ? hr == S_OK || broken(hr == AUDCLNT_E_ENDPOINT_CREATE_FAILED)
               : hr == AUDCLNT_E_ENDPOINT_CREATE_FAILED || hr == AUDCLNT_E_UNSUPPORTED_FORMAT ||
                 broken(hr == S_OK &&
                     ((fmt.nChannels == 1 && fmt.wBitsPerSample == 16) ||
                      (fmt.nSamplesPerSec == 12000 || fmt.nSamplesPerSec == 96000))),
               "Initialize(exclus., %ux%2ux%u) returns %08x\n",
               fmt.nSamplesPerSec, fmt.wBitsPerSample, fmt.nChannels, hr);

        /* Bug in native (Vista/w2k8/w7): after Initialize failed, better
         * Release this ac and Activate a new one.
         * A second call (with a known working format) would yield
         * ALREADY_INITIALIZED in shared mode yet be unusable, and in exclusive
         * mode some entity keeps a lock on the device, causing DEVICE_IN_USE to
         * all subsequent calls until the audio engine service is restarted. */

        CoTaskMemFree(pwfx2);
        CoTaskMemFree(pwfx);
        IAudioClient_Release(ac);
    }
}

static void test_formats2(void)
{
    IAudioClient *ac;
    HRESULT hr;
    WAVEFORMATEX *pwfx, *pwfx2;
    WAVEFORMATEXTENSIBLE *pwfe, wfe, *pwfe2;

    hr = IMMDevice_Activate(dev, &IID_IAudioClient, CLSCTX_INPROC_SERVER,
                            NULL, (void**)&ac);

    ok(hr == S_OK, "Activation failed with %08x\n", hr);
    if (hr != S_OK)
        return;

    hr = IAudioClient_GetMixFormat(ac, &pwfx);
    ok(hr == S_OK, "GetMixFormat failed: %08x\n", hr);
    if (hr != S_OK)
        return;

    ok(pwfx->wFormatTag == WAVE_FORMAT_EXTENSIBLE, "Invalid wFormatTag\n");
    if (pwfx->wFormatTag != WAVE_FORMAT_EXTENSIBLE) {
        CoTaskMemFree(pwfx);
        return;
    }

    pwfe = (WAVEFORMATEXTENSIBLE*)pwfx;
    ok(pwfe->Samples.wValidBitsPerSample, "wValidBitsPerSample should be non-zero\n");

    if (pwfx->nChannels > 2) {
        trace("Limiting channels to 2\n");
        pwfx->nChannels = 2;
        pwfx->nBlockAlign = pwfx->wBitsPerSample / 8 * pwfx->nChannels;
        pwfx->nAvgBytesPerSec = pwfx->nBlockAlign * pwfx->nSamplesPerSec;
        pwfe->dwChannelMask = SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT;
    }

    wfe = *pwfe;
    pwfx->nAvgBytesPerSec = pwfx->nBlockAlign = 0;

    hr = IAudioClient_IsFormatSupported(ac, AUDCLNT_SHAREMODE_EXCLUSIVE, pwfx, NULL);
    ok(hr == AUDCLNT_E_UNSUPPORTED_FORMAT || hr == AUDCLNT_E_EXCLUSIVE_MODE_NOT_ALLOWED,
       "Exclusive IsFormatSupported with nAvgBytesPerSec=0 and nBlockAlign=0 returned %08x\n", hr);

    pwfx2 = NULL;
    hr = IAudioClient_IsFormatSupported(ac, AUDCLNT_SHAREMODE_SHARED, pwfx, &pwfx2);
    ok((hr == E_INVALIDARG || hr == AUDCLNT_E_UNSUPPORTED_FORMAT) && !pwfx2,
       "Shared IsFormatSupported with nAvgBytesPerSec=0 and nBlockAlign=0 returned %08x %p\n", hr, pwfx2);
    CoTaskMemFree(pwfx2);

    pwfx->wFormatTag = WAVE_FORMAT_PCM;
    pwfx2 = NULL;
    hr = IAudioClient_IsFormatSupported(ac, AUDCLNT_SHAREMODE_SHARED, pwfx, &pwfx2);
    ok((hr == S_OK || hr == AUDCLNT_E_UNSUPPORTED_FORMAT) && !pwfx2,
       "Shared IsFormatSupported with nAvgBytesPerSec=0 and nBlockAlign=0 returned %08x %p\n", hr, pwfx2);
    CoTaskMemFree(pwfx2);

    *pwfe = wfe;
    pwfe->dwChannelMask = 0;
    hr = IAudioClient_IsFormatSupported(ac, AUDCLNT_SHAREMODE_EXCLUSIVE, pwfx, NULL);
    ok(hr == AUDCLNT_E_UNSUPPORTED_FORMAT || hr == AUDCLNT_E_EXCLUSIVE_MODE_NOT_ALLOWED,
       "Exclusive IsFormatSupported with dwChannelMask=0 returned %08x\n", hr);

    pwfx2 = NULL;
    hr = IAudioClient_IsFormatSupported(ac, AUDCLNT_SHAREMODE_SHARED, pwfx, &pwfx2);
    ok(hr == S_OK,
       "Shared IsFormatSupported with dwChannelMask=0 returned %08x\n", hr);
    CoTaskMemFree(pwfx2);


    pwfe->dwChannelMask = 0x3ffff;
    hr = IAudioClient_IsFormatSupported(ac, AUDCLNT_SHAREMODE_EXCLUSIVE, pwfx, NULL);
    ok(hr == AUDCLNT_E_UNSUPPORTED_FORMAT || hr == AUDCLNT_E_EXCLUSIVE_MODE_NOT_ALLOWED,
       "Exclusive IsFormatSupported with dwChannelMask=0x3ffff returned %08x\n", hr);

    pwfx2 = NULL;
    hr = IAudioClient_IsFormatSupported(ac, AUDCLNT_SHAREMODE_SHARED, pwfx, &pwfx2);
    ok(hr == S_OK && !pwfx2,
       "Shared IsFormatSupported with dwChannelMask=0x3ffff returned %08x %p\n", hr, pwfx2);
    CoTaskMemFree(pwfx2);


    pwfe->dwChannelMask = 0x40000000;
    hr = IAudioClient_IsFormatSupported(ac, AUDCLNT_SHAREMODE_EXCLUSIVE, pwfx, NULL);
    ok(hr == AUDCLNT_E_UNSUPPORTED_FORMAT || hr == AUDCLNT_E_EXCLUSIVE_MODE_NOT_ALLOWED,
       "Exclusive IsFormatSupported with dwChannelMask=0x40000000 returned %08x\n", hr);

    pwfx2 = NULL;
    hr = IAudioClient_IsFormatSupported(ac, AUDCLNT_SHAREMODE_SHARED, pwfx, &pwfx2);
    ok(hr == S_OK && !pwfx2,
       "Shared IsFormatSupported with dwChannelMask=0x40000000 returned %08x %p\n", hr, pwfx2);
    CoTaskMemFree(pwfx2);

    pwfe->dwChannelMask = SPEAKER_ALL | SPEAKER_RESERVED;
    hr = IAudioClient_IsFormatSupported(ac, AUDCLNT_SHAREMODE_EXCLUSIVE, pwfx, NULL);
    ok(hr == AUDCLNT_E_UNSUPPORTED_FORMAT || hr == AUDCLNT_E_EXCLUSIVE_MODE_NOT_ALLOWED,
       "Exclusive IsFormatSupported with dwChannelMask=SPEAKER_ALL | SPEAKER_RESERVED returned %08x\n", hr);

    pwfx2 = NULL;
    hr = IAudioClient_IsFormatSupported(ac, AUDCLNT_SHAREMODE_SHARED, pwfx, &pwfx2);
    ok(hr == S_OK && !pwfx2,
       "Shared IsFormatSupported with dwChannelMask=SPEAKER_ALL | SPEAKER_RESERVED returned %08x %p\n", hr, pwfx2);
    CoTaskMemFree(pwfx2);

    *pwfe = wfe;
    pwfe->Samples.wValidBitsPerSample = 0;
    hr = IAudioClient_IsFormatSupported(ac, AUDCLNT_SHAREMODE_EXCLUSIVE, pwfx, NULL);
    ok(hr == AUDCLNT_E_UNSUPPORTED_FORMAT || hr == AUDCLNT_E_EXCLUSIVE_MODE_NOT_ALLOWED,
       "Exclusive IsFormatSupported with wValidBitsPerSample=0 returned %08x\n", hr);

    pwfx2 = NULL;
    hr = IAudioClient_IsFormatSupported(ac, AUDCLNT_SHAREMODE_SHARED, pwfx, &pwfx2);
    ok((hr == S_FALSE || hr == AUDCLNT_E_UNSUPPORTED_FORMAT) && pwfx2,
       "Shared IsFormatSupported with wValidBitsPerSample=0 returned %08x %p\n", hr, pwfx2);
    if (pwfx2) {
        pwfe2 = (WAVEFORMATEXTENSIBLE*)pwfx2;
        ok(pwfe2->Samples.wValidBitsPerSample == pwfx->wBitsPerSample,
           "Shared IsFormatSupported had wValidBitsPerSample set to %u, not %u\n",
           pwfe2->Samples.wValidBitsPerSample, pwfx->wBitsPerSample);
        CoTaskMemFree(pwfx2);
    }

    pwfx2 = NULL;
    pwfe->Samples.wValidBitsPerSample = pwfx->wBitsPerSample + 1;
    hr = IAudioClient_IsFormatSupported(ac, AUDCLNT_SHAREMODE_SHARED, pwfx, &pwfx2);
    ok((hr == E_INVALIDARG || hr == AUDCLNT_E_UNSUPPORTED_FORMAT) && !pwfx2,
       "Shared IsFormatSupported with wValidBitsPerSample += 1 returned %08x %p\n", hr, pwfx2);

    *pwfe = wfe;
    memset(&pwfe->SubFormat, 0xff, 16);
    pwfx2 = NULL;
    hr = IAudioClient_IsFormatSupported(ac, AUDCLNT_SHAREMODE_SHARED, pwfx, &pwfx2);
    ok(hr == AUDCLNT_E_UNSUPPORTED_FORMAT && !pwfx2,
       "Shared IsFormatSupported with SubFormat=-1 returned %08x %p\n", hr, pwfx2);
    CoTaskMemFree(pwfx2);

    *pwfe = wfe;
    pwfx2 = NULL;
    pwfe->Samples.wValidBitsPerSample = pwfx->wBitsPerSample = 256;
    pwfx->nBlockAlign = pwfx->wBitsPerSample / 8 * pwfx->nChannels;
    pwfx->nAvgBytesPerSec = pwfx->nBlockAlign * pwfx->nSamplesPerSec;
    hr = IAudioClient_IsFormatSupported(ac, AUDCLNT_SHAREMODE_SHARED, pwfx, &pwfx2);
    ok((hr == E_INVALIDARG || hr == AUDCLNT_E_UNSUPPORTED_FORMAT) && !pwfx2,
       "Shared IsFormatSupported with wBitsPerSample=256 returned %08x %p\n", hr, pwfx2);
    CoTaskMemFree(pwfx2);

    *pwfe = wfe;
    pwfx2 = NULL;
    pwfe->Samples.wValidBitsPerSample = pwfx->wBitsPerSample - 1;
    hr = IAudioClient_IsFormatSupported(ac, AUDCLNT_SHAREMODE_SHARED, pwfx, &pwfx2);
    ok(hr == S_FALSE && pwfx2,
       "Shared IsFormatSupported with wValidBitsPerSample-=1 returned %08x %p\n", hr, pwfx2);
    if (pwfx2) {
        pwfe2 = (WAVEFORMATEXTENSIBLE*)pwfx2;
        ok(pwfe2->Samples.wValidBitsPerSample == pwfx->wBitsPerSample,
           "Shared IsFormatSupported had wValidBitsPerSample set to %u, not %u\n",
           pwfe2->Samples.wValidBitsPerSample, pwfx->wBitsPerSample);
        CoTaskMemFree(pwfx2);
    }

    CoTaskMemFree(pwfx);
    IAudioClient_Release(ac);
}

static void test_references(void)
{
    IAudioClient *ac;
    IAudioRenderClient *rc;
    ISimpleAudioVolume *sav;
    IAudioStreamVolume *asv;
    IAudioClock *acl;
    WAVEFORMATEX *pwfx;
    HRESULT hr;
    ULONG ref;

    /* IAudioRenderClient */
    hr = IMMDevice_Activate(dev, &IID_IAudioClient, CLSCTX_INPROC_SERVER,
            NULL, (void**)&ac);
    ok(hr == S_OK, "Activation failed with %08x\n", hr);
    if(hr != S_OK)
        return;

    hr = IAudioClient_GetMixFormat(ac, &pwfx);
    ok(hr == S_OK, "GetMixFormat failed: %08x\n", hr);

    hr = IAudioClient_Initialize(ac, AUDCLNT_SHAREMODE_SHARED, 0, 5000000,
            0, pwfx, NULL);
    ok(hr == S_OK, "Initialize failed: %08x\n", hr);

    CoTaskMemFree(pwfx);

    hr = IAudioClient_GetService(ac, &IID_IAudioRenderClient, (void**)&rc);
    ok(hr == S_OK, "GetService failed: %08x\n", hr);
    if(hr != S_OK) {
        IAudioClient_Release(ac);
        return;
    }

    IAudioRenderClient_AddRef(rc);
    ref = IAudioRenderClient_Release(rc);
    ok(ref != 0, "RenderClient_Release gave wrong refcount: %u\n", ref);

    ref = IAudioClient_Release(ac);
    ok(ref != 0, "Client_Release gave wrong refcount: %u\n", ref);

    ref = IAudioRenderClient_Release(rc);
    ok(ref == 0, "RenderClient_Release gave wrong refcount: %u\n", ref);

    /* ISimpleAudioVolume */
    hr = IMMDevice_Activate(dev, &IID_IAudioClient, CLSCTX_INPROC_SERVER,
            NULL, (void**)&ac);
    ok(hr == S_OK, "Activation failed with %08x\n", hr);
    if(hr != S_OK)
        return;

    hr = IAudioClient_GetMixFormat(ac, &pwfx);
    ok(hr == S_OK, "GetMixFormat failed: %08x\n", hr);

    hr = IAudioClient_Initialize(ac, AUDCLNT_SHAREMODE_SHARED, 0, 5000000,
            0, pwfx, NULL);
    ok(hr == S_OK, "Initialize failed: %08x\n", hr);

    CoTaskMemFree(pwfx);

    hr = IAudioClient_GetService(ac, &IID_ISimpleAudioVolume, (void**)&sav);
    ok(hr == S_OK, "GetService failed: %08x\n", hr);

    ISimpleAudioVolume_AddRef(sav);
    ref = ISimpleAudioVolume_Release(sav);
    ok(ref != 0, "SimpleAudioVolume_Release gave wrong refcount: %u\n", ref);

    ref = IAudioClient_Release(ac);
    ok(ref != 0, "Client_Release gave wrong refcount: %u\n", ref);

    ref = ISimpleAudioVolume_Release(sav);
    ok(ref == 0, "SimpleAudioVolume_Release gave wrong refcount: %u\n", ref);

    /* IAudioClock */
    hr = IMMDevice_Activate(dev, &IID_IAudioClient, CLSCTX_INPROC_SERVER,
            NULL, (void**)&ac);
    ok(hr == S_OK, "Activation failed with %08x\n", hr);
    if(hr != S_OK)
        return;

    hr = IAudioClient_GetMixFormat(ac, &pwfx);
    ok(hr == S_OK, "GetMixFormat failed: %08x\n", hr);

    hr = IAudioClient_Initialize(ac, AUDCLNT_SHAREMODE_SHARED, 0, 5000000,
            0, pwfx, NULL);
    ok(hr == S_OK, "Initialize failed: %08x\n", hr);

    CoTaskMemFree(pwfx);

    hr = IAudioClient_GetService(ac, &IID_IAudioClock, (void**)&acl);
    ok(hr == S_OK, "GetService failed: %08x\n", hr);

    IAudioClock_AddRef(acl);
    ref = IAudioClock_Release(acl);
    ok(ref != 0, "AudioClock_Release gave wrong refcount: %u\n", ref);

    ref = IAudioClient_Release(ac);
    ok(ref != 0, "Client_Release gave wrong refcount: %u\n", ref);

    ref = IAudioClock_Release(acl);
    ok(ref == 0, "AudioClock_Release gave wrong refcount: %u\n", ref);

    /* IAudioStreamVolume */
    hr = IMMDevice_Activate(dev, &IID_IAudioClient, CLSCTX_INPROC_SERVER,
            NULL, (void**)&ac);
    ok(hr == S_OK, "Activation failed with %08x\n", hr);
    if(hr != S_OK)
        return;

    hr = IAudioClient_GetMixFormat(ac, &pwfx);
    ok(hr == S_OK, "GetMixFormat failed: %08x\n", hr);

    hr = IAudioClient_Initialize(ac, AUDCLNT_SHAREMODE_SHARED, 0, 5000000,
            0, pwfx, NULL);
    ok(hr == S_OK, "Initialize failed: %08x\n", hr);

    CoTaskMemFree(pwfx);

    hr = IAudioClient_GetService(ac, &IID_IAudioStreamVolume, (void**)&asv);
    ok(hr == S_OK, "GetService failed: %08x\n", hr);

    IAudioStreamVolume_AddRef(asv);
    ref = IAudioStreamVolume_Release(asv);
    ok(ref != 0, "AudioStreamVolume_Release gave wrong refcount: %u\n", ref);

    ref = IAudioClient_Release(ac);
    ok(ref != 0, "Client_Release gave wrong refcount: %u\n", ref);

    ref = IAudioStreamVolume_Release(asv);
    ok(ref == 0, "AudioStreamVolume_Release gave wrong refcount: %u\n", ref);
}

static void test_event(void)
{
    HANDLE event;
    HRESULT hr;
    DWORD r;
    IAudioClient *ac;
    WAVEFORMATEX *pwfx;

    hr = IMMDevice_Activate(dev, &IID_IAudioClient, CLSCTX_INPROC_SERVER,
            NULL, (void**)&ac);
    ok(hr == S_OK, "Activation failed with %08x\n", hr);
    if(hr != S_OK)
        return;

    hr = IAudioClient_GetMixFormat(ac, &pwfx);
    ok(hr == S_OK, "GetMixFormat failed: %08x\n", hr);

    hr = IAudioClient_Initialize(ac, AUDCLNT_SHAREMODE_SHARED,
            AUDCLNT_STREAMFLAGS_EVENTCALLBACK, 5000000,
            0, pwfx, NULL);
    ok(hr == S_OK, "Initialize failed: %08x\n", hr);

    CoTaskMemFree(pwfx);

    event = CreateEventW(NULL, FALSE, FALSE, NULL);
    ok(event != NULL, "CreateEvent failed\n");

    hr = IAudioClient_Start(ac);
    ok(hr == AUDCLNT_E_EVENTHANDLE_NOT_SET, "Start failed: %08x\n", hr);

    hr = IAudioClient_SetEventHandle(ac, event);
    ok(hr == S_OK, "SetEventHandle failed: %08x\n", hr);

    hr = IAudioClient_SetEventHandle(ac, event);
    ok(hr == HRESULT_FROM_WIN32(ERROR_INVALID_NAME), "SetEventHandle returns %08x\n", hr);

    r = WaitForSingleObject(event, 40);
    ok(r == WAIT_TIMEOUT, "Wait(event) before Start gave %x\n", r);

    hr = IAudioClient_Start(ac);
    ok(hr == S_OK, "Start failed: %08x\n", hr);

    r = WaitForSingleObject(event, 20);
    ok(r == WAIT_OBJECT_0, "Wait(event) after Start gave %x\n", r);

    hr = IAudioClient_Stop(ac);
    ok(hr == S_OK, "Stop failed: %08x\n", hr);

    ok(ResetEvent(event), "ResetEvent\n");

    /* Still receiving events! */
    r = WaitForSingleObject(event, 20);
    ok(r == WAIT_OBJECT_0, "Wait(event) after Stop gave %x\n", r);

    hr = IAudioClient_Reset(ac);
    ok(hr == S_OK, "Reset failed: %08x\n", hr);

    ok(ResetEvent(event), "ResetEvent\n");

    r = WaitForSingleObject(event, 120);
    ok(r == WAIT_OBJECT_0, "Wait(event) after Reset gave %x\n", r);

    hr = IAudioClient_SetEventHandle(ac, NULL);
    ok(hr == E_INVALIDARG, "SetEventHandle(NULL) returns %08x\n", hr);

    r = WaitForSingleObject(event, 70);
    ok(r == WAIT_OBJECT_0, "Wait(NULL event) gave %x\n", r);

    /* test releasing a playing stream */
    hr = IAudioClient_Start(ac);
    ok(hr == S_OK, "Start failed: %08x\n", hr);
    IAudioClient_Release(ac);

    CloseHandle(event);
}

static void test_padding(void)
{
    HRESULT hr;
    IAudioClient *ac;
    IAudioRenderClient *arc;
    WAVEFORMATEX *pwfx;
    REFERENCE_TIME minp, defp;
    BYTE *buf, silence;
    UINT32 psize, pad, written, i;

    hr = IMMDevice_Activate(dev, &IID_IAudioClient, CLSCTX_INPROC_SERVER,
            NULL, (void**)&ac);
    ok(hr == S_OK, "Activation failed with %08x\n", hr);
    if(hr != S_OK)
        return;

    hr = IAudioClient_GetMixFormat(ac, &pwfx);
    ok(hr == S_OK, "GetMixFormat failed: %08x\n", hr);
    if(hr != S_OK)
        return;

    hr = IAudioClient_Initialize(ac, AUDCLNT_SHAREMODE_SHARED,
            0, 5000000, 0, pwfx, NULL);
    ok(hr == S_OK, "Initialize failed: %08x\n", hr);
    if(hr != S_OK)
        return;

    if(pwfx->wBitsPerSample == 8)
        silence = 128;
    else
        silence = 0;

    /** GetDevicePeriod
     * Default (= shared) device period is 10ms (e.g. 441 frames at 44100),
     * except when the HW/OS forces a particular alignment,
     * e.g. 10.1587ms is 28 * 16 = 448 frames at 44100 with HDA.
     * 441 observed with Vista, 448 with w7 on the same HW! */
    hr = IAudioClient_GetDevicePeriod(ac, &defp, &minp);
    ok(hr == S_OK, "GetDevicePeriod failed: %08x\n", hr);
    /* some wineXYZ.drv use 20ms, not seen on native */
    ok(defp == 100000 || broken(defp == 101587) || defp == 200000,
       "Expected 10ms default period: %u\n", (ULONG)defp);
    ok(minp != 0, "Minimum period is 0\n");
    ok(minp <= defp, "Mininum period is greater than default period\n");

    hr = IAudioClient_GetService(ac, &IID_IAudioRenderClient, (void**)&arc);
    ok(hr == S_OK, "GetService failed: %08x\n", hr);

    psize = MulDiv(defp, pwfx->nSamplesPerSec, 10000000) * 10;

    written = 0;
    hr = IAudioClient_GetCurrentPadding(ac, &pad);
    ok(hr == S_OK, "GetCurrentPadding failed: %08x\n", hr);
    ok(pad == written, "GetCurrentPadding returned %u, should be %u\n", pad, written);

    hr = IAudioRenderClient_GetBuffer(arc, psize, &buf);
    ok(hr == S_OK, "GetBuffer failed: %08x\n", hr);
    ok(buf != NULL, "NULL buffer returned\n");
    for(i = 0; i < psize * pwfx->nBlockAlign; ++i){
        if(buf[i] != silence){
            ok(0, "buffer has data in it already\n");
            break;
        }
    }

    hr = IAudioRenderClient_GetBuffer(arc, 0, &buf);
    ok(hr == AUDCLNT_E_OUT_OF_ORDER, "GetBuffer 0 size failed: %08x\n", hr);
    ok(buf == NULL, "GetBuffer 0 gave %p\n", buf);
    /* MSDN instead documents buf remains untouched */

    hr = IAudioClient_Reset(ac);
    ok(hr == AUDCLNT_E_BUFFER_OPERATION_PENDING, "Reset failed: %08x\n", hr);

    hr = IAudioRenderClient_ReleaseBuffer(arc, psize,
            AUDCLNT_BUFFERFLAGS_SILENT);
    ok(hr == S_OK, "ReleaseBuffer failed: %08x\n", hr);
    if(hr == S_OK) written += psize;

    hr = IAudioClient_GetCurrentPadding(ac, &pad);
    ok(hr == S_OK, "GetCurrentPadding failed: %08x\n", hr);
    ok(pad == written, "GetCurrentPadding returned %u, should be %u\n", pad, written);

    psize = MulDiv(minp, pwfx->nSamplesPerSec, 10000000) * 10;

    hr = IAudioRenderClient_GetBuffer(arc, psize, &buf);
    ok(hr == S_OK, "GetBuffer failed: %08x\n", hr);
    ok(buf != NULL, "NULL buffer returned\n");

    hr = IAudioRenderClient_ReleaseBuffer(arc, psize,
            AUDCLNT_BUFFERFLAGS_SILENT);
    ok(hr == S_OK, "ReleaseBuffer failed: %08x\n", hr);
    written += psize;

    hr = IAudioClient_GetCurrentPadding(ac, &pad);
    ok(hr == S_OK, "GetCurrentPadding failed: %08x\n", hr);
    ok(pad == written, "GetCurrentPadding returned %u, should be %u\n", pad, written);

    /* overfull buffer. requested 1/2s buffer size, so try
     * to get a 1/2s buffer, which should fail */
    psize = pwfx->nSamplesPerSec / 2;
    buf = (void*)0xDEADF00D;
    hr = IAudioRenderClient_GetBuffer(arc, psize, &buf);
    ok(hr == AUDCLNT_E_BUFFER_TOO_LARGE, "GetBuffer gave wrong error: %08x\n", hr);
    ok(buf == NULL, "NULL expected %p\n", buf);

    hr = IAudioRenderClient_ReleaseBuffer(arc, psize, 0);
    ok(hr == AUDCLNT_E_OUT_OF_ORDER, "ReleaseBuffer gave wrong error: %08x\n", hr);

    psize = MulDiv(minp, pwfx->nSamplesPerSec, 10000000) * 2;

    hr = IAudioRenderClient_GetBuffer(arc, psize, &buf);
    ok(hr == S_OK, "GetBuffer failed: %08x\n", hr);
    ok(buf != NULL, "NULL buffer returned\n");

    hr = IAudioRenderClient_ReleaseBuffer(arc, 0, 0);
    ok(hr == S_OK, "ReleaseBuffer 0 gave wrong error: %08x\n", hr);

    buf = (void*)0xDEADF00D;
    hr = IAudioRenderClient_GetBuffer(arc, 0, &buf);
    ok(hr == S_OK, "GetBuffer 0 size failed: %08x\n", hr);
    ok(buf == NULL, "GetBuffer 0 gave %p\n", buf);
    /* MSDN instead documents buf remains untouched */

    buf = (void*)0xDEADF00D;
    hr = IAudioRenderClient_GetBuffer(arc, 0, &buf);
    ok(hr == S_OK, "GetBuffer 0 size #2 failed: %08x\n", hr);
    ok(buf == NULL, "GetBuffer 0 #2 gave %p\n", buf);

    hr = IAudioRenderClient_ReleaseBuffer(arc, psize, 0);
    ok(hr == AUDCLNT_E_OUT_OF_ORDER, "ReleaseBuffer not size 0 gave %08x\n", hr);

    hr = IAudioRenderClient_GetBuffer(arc, psize, &buf);
    ok(hr == S_OK, "GetBuffer failed: %08x\n", hr);
    ok(buf != NULL, "NULL buffer returned\n");

    hr = IAudioRenderClient_ReleaseBuffer(arc, 0, 0);
    ok(hr == S_OK, "ReleaseBuffer 0 gave wrong error: %08x\n", hr);

    hr = IAudioClient_GetCurrentPadding(ac, &pad);
    ok(hr == S_OK, "GetCurrentPadding failed: %08x\n", hr);
    ok(pad == written, "GetCurrentPadding returned %u, should be %u\n", pad, written);

    hr = IAudioRenderClient_GetBuffer(arc, psize, &buf);
    ok(hr == S_OK, "GetBuffer failed: %08x\n", hr);
    ok(buf != NULL, "NULL buffer returned\n");

    hr = IAudioRenderClient_ReleaseBuffer(arc, psize+1, AUDCLNT_BUFFERFLAGS_SILENT);
    ok(hr == AUDCLNT_E_INVALID_SIZE, "ReleaseBuffer too large error: %08x\n", hr);
    /* todo_wine means Wine may overwrite memory */
    if(hr == S_OK) written += psize+1;

    /* Buffer still hold */
    hr = IAudioRenderClient_ReleaseBuffer(arc, psize/2, AUDCLNT_BUFFERFLAGS_SILENT);
    ok(hr == S_OK, "ReleaseBuffer after error: %08x\n", hr);
    if(hr == S_OK) written += psize/2;

    hr = IAudioRenderClient_ReleaseBuffer(arc, 0, 0);
    ok(hr == S_OK, "ReleaseBuffer 0 gave wrong error: %08x\n", hr);

    hr = IAudioClient_GetCurrentPadding(ac, &pad);
    ok(hr == S_OK, "GetCurrentPadding failed: %08x\n", hr);
    ok(pad == written, "GetCurrentPadding returned %u, should be %u\n", pad, written);

    CoTaskMemFree(pwfx);

    IAudioRenderClient_Release(arc);
    IAudioClient_Release(ac);
}

static void test_clock(int share)
{
    HRESULT hr;
    IAudioClient *ac;
    IAudioClock *acl;
    IAudioRenderClient *arc;
    UINT64 freq, pos, pcpos0, pcpos, last;
    UINT32 pad, gbsize, bufsize, fragment, parts, avail, slept = 0, sum = 0;
    BYTE *data;
    WAVEFORMATEX *pwfx;
    LARGE_INTEGER hpctime, hpctime0, hpcfreq;
    REFERENCE_TIME minp, defp, t1, t2;
    REFERENCE_TIME duration = 5000000, period = 150000;
    int i;

    ok(QueryPerformanceFrequency(&hpcfreq), "PerfFrequency failed\n");

    hr = IMMDevice_Activate(dev, &IID_IAudioClient, CLSCTX_INPROC_SERVER,
            NULL, (void**)&ac);
    ok(hr == S_OK, "Activation failed with %08x\n", hr);
    if(hr != S_OK)
        return;

    hr = IAudioClient_GetMixFormat(ac, &pwfx);
    ok(hr == S_OK, "GetMixFormat failed: %08x\n", hr);
    if(hr != S_OK)
        return;

    hr = IAudioClient_GetDevicePeriod(ac, &defp, &minp);
    ok(hr == S_OK, "GetDevicePeriod failed: %08x\n", hr);
    ok(minp <= period, "desired period %u too small for %u\n", (ULONG)period, (ULONG)minp);

    if (share) {
        trace("Testing shared mode\n");
        /* period is ignored */
        hr = IAudioClient_Initialize(ac, AUDCLNT_SHAREMODE_SHARED,
                0, duration, period, pwfx, NULL);
        period = defp;
    } else {
        pwfx->wFormatTag = WAVE_FORMAT_PCM;
        pwfx->nChannels = 2;
        pwfx->cbSize = 0;
        pwfx->wBitsPerSample = 16; /* no floating point */
        pwfx->nBlockAlign = pwfx->nChannels * pwfx->wBitsPerSample / 8;
        pwfx->nAvgBytesPerSec = pwfx->nSamplesPerSec * pwfx->nBlockAlign;
        trace("Testing exclusive mode at %u\n", pwfx->nSamplesPerSec);

        hr = IAudioClient_Initialize(ac, AUDCLNT_SHAREMODE_EXCLUSIVE,
                0, duration, period, pwfx, NULL);
    }
    ok(share ? hr == S_OK : hr == hexcl || hr == AUDCLNT_E_DEVICE_IN_USE, "Initialize failed: %08x\n", hr);
    if (hr != S_OK) {
        CoTaskMemFree(pwfx);
        IAudioClient_Release(ac);
        if(hr == AUDCLNT_E_DEVICE_IN_USE)
            skip("Device in use, no %s access\n", share ? "shared" : "exclusive");
        return;
    }

    /** GetStreamLatency
     * Shared mode: 1x period + a little, but some 192000 devices return 5.3334ms.
     * Exclusive mode: testbot returns 2x period + a little, but
     * some HDA drivers return 1x period, some + a little. */
    hr = IAudioClient_GetStreamLatency(ac, &t2);
    ok(hr == S_OK, "GetStreamLatency failed: %08x\n", hr);
    trace("Latency: %u.%04u ms\n", (UINT)(t2/10000), (UINT)(t2 % 10000));
    ok(t2 >= period || broken(t2 >= period/2 && share && pwfx->nSamplesPerSec > 48000),
       "Latency < default period, delta %ldus\n", (long)((t2-period)/10));

    /** GetBufferSize
     * BufferSize must be rounded up, maximum 2s says MSDN.
     * Both is wrong.  Rounding may lead to size a little smaller than duration;
     * duration > 2s is accepted in shared mode.
     * Shared mode: round solely w.r.t. mixer rate,
     *              duration is no multiple of period.
     * Exclusive mode: size appears as a multiple of some fragment that
     * is either the rounded period or a fixed constant like 1024,
     * whatever the driver implements. */
    hr = IAudioClient_GetBufferSize(ac, &gbsize);
    ok(hr == S_OK, "GetBufferSize failed: %08x\n", hr);

    bufsize   =  MulDiv(duration, pwfx->nSamplesPerSec, 10000000);
    fragment  =  MulDiv(period,   pwfx->nSamplesPerSec, 10000000);
    parts     =  MulDiv(bufsize, 1, fragment); /* instead of (duration, 1, period) */
    trace("BufferSize %u estimated fragment %u x %u = %u\n", gbsize, fragment, parts, fragment * parts);
    /* fragment size (= period in frames) is rounded up.
     * BufferSize must be rounded up, maximum 2s says MSDN
     * but it is rounded down modulo fragment ! */
    if (share)
    ok(gbsize == bufsize,
       "BufferSize %u at rate %u\n", gbsize, pwfx->nSamplesPerSec);
    else
    ok(gbsize == parts * fragment || gbsize == MulDiv(bufsize, 1, 1024) * 1024,
       "BufferSize %u misfits fragment size %u at rate %u\n", gbsize, fragment, pwfx->nSamplesPerSec);

    /* In shared mode, GetCurrentPadding decreases in multiples of
     * fragment size (i.e. updated only at period ticks), whereas
     * GetPosition appears to be reporting continuous positions.
     * In exclusive mode, testbot behaves likewise, but native's Intel
     * HDA driver shows no such deltas, GetCurrentPadding closely
     * matches GetPosition, as in
     * GetCurrentPadding = GetPosition - frames held in mmdevapi */

    hr = IAudioClient_GetService(ac, &IID_IAudioClock, (void**)&acl);
    ok(hr == S_OK, "GetService(IAudioClock) failed: %08x\n", hr);

    hr = IAudioClock_GetFrequency(acl, &freq);
    ok(hr == S_OK, "GetFrequency failed: %08x\n", hr);
    trace("Clock Frequency %u\n", (UINT)freq);

    /* MSDN says it's arbitrary units, but shared mode is unlikely to change */
    if (share)
        ok(freq == pwfx->nSamplesPerSec * pwfx->nBlockAlign,
           "Clock Frequency %u\n", (UINT)freq);
    else
        ok(freq == pwfx->nSamplesPerSec,
           "Clock Frequency %u\n", (UINT)freq);

    hr = IAudioClock_GetPosition(acl, NULL, NULL);
    ok(hr == E_POINTER, "GetPosition wrong error: %08x\n", hr);

    pcpos0 = 0;
    hr = IAudioClock_GetPosition(acl, &pos, &pcpos0);
    ok(hr == S_OK, "GetPosition failed: %08x\n", hr);
    ok(pos == 0, "GetPosition returned non-zero pos before being started\n");
    ok(pcpos0 != 0, "GetPosition returned zero pcpos\n");

    hr = IAudioClient_GetService(ac, &IID_IAudioRenderClient, (void**)&arc);
    ok(hr == S_OK, "GetService(IAudioRenderClient) failed: %08x\n", hr);

    hr = IAudioRenderClient_GetBuffer(arc, gbsize+1, &data);
    ok(hr == AUDCLNT_E_BUFFER_TOO_LARGE, "GetBuffer too large failed: %08x\n", hr);

    avail = gbsize;
    data = NULL;
    hr = IAudioRenderClient_GetBuffer(arc, avail, &data);
    ok(hr == S_OK, "GetBuffer failed: %08x\n", hr);
    trace("data at %p\n", data);

    hr = IAudioRenderClient_ReleaseBuffer(arc, avail, winetest_debug>2 ?
        wave_generate_tone(pwfx, data, avail) : AUDCLNT_BUFFERFLAGS_SILENT);
    ok(hr == S_OK, "ReleaseBuffer failed: %08x\n", hr);
    if(hr == S_OK) sum += avail;

    hr = IAudioClient_GetCurrentPadding(ac, &pad);
    ok(hr == S_OK, "GetCurrentPadding failed: %08x\n", hr);
    ok(pad == sum, "padding %u prior to start\n", pad);

    hr = IAudioClock_GetPosition(acl, &pos, NULL);
    ok(hr == S_OK, "GetPosition failed: %08x\n", hr);
    ok(pos == 0, "GetPosition returned non-zero pos before being started\n");

    hr = IAudioClient_Start(ac); /* #1 */
    ok(hr == S_OK, "Start failed: %08x\n", hr);

    Sleep(100);
    slept += 100;

    hr = IAudioClient_GetStreamLatency(ac, &t1);
    ok(hr == S_OK, "GetStreamLatency failed: %08x\n", hr);
    ok(t1 == t2, "Latency not constant, delta %ld\n", (long)(t1-t2));

    hr = IAudioClock_GetPosition(acl, &pos, NULL);
    ok(hr == S_OK, "GetPosition failed: %08x\n", hr);
    ok(pos > 0, "Position %u vs. last %u\n", (UINT)pos,0);
    /* in rare cases is slept*1.1 not enough with dmix */
    ok(pos*1000/freq <= slept*1.4, "Position %u too far after playing %ums\n", (UINT)pos, slept);
    last = pos;

    hr = IAudioClient_Stop(ac);
    ok(hr == S_OK, "Stop failed: %08x\n", hr);

    hr = IAudioClock_GetPosition(acl, &pos, NULL);
    ok(hr == S_OK, "GetPosition failed: %08x\n", hr);
    ok(pos >= last, "Position %u vs. last %u\n", (UINT)pos,(UINT)last);
    last = pos;
    if(/*share &&*/ winetest_debug>1)
        ok(pos*1000/freq <= slept*1.1, "Position %u too far after stop %ums\n", (UINT)pos, slept);

    hr = IAudioClient_Start(ac); /* #2 */
    ok(hr == S_OK, "Start failed: %08x\n", hr);

    Sleep(100);
    slept += 100;

    hr = IAudioClient_GetCurrentPadding(ac, &pad);
    ok(hr == S_OK, "GetCurrentPadding failed: %08x\n", hr);
    trace("padding %u past sleep #2\n", pad);

    /** IAudioClient_Stop
     * Exclusive mode: the audio engine appears to drop frames,
     * bumping GetPosition to a higher value than time allows, even
     * allowing GetPosition > sum Released - GetCurrentPadding (testbot)
     * Shared mode: no drop observed (or too small to be visible).
     * GetPosition = sum Released - GetCurrentPadding
     * Bugs: Some USB headset system drained the whole buffer, leaving
     *       padding 0 and bumping pos to sum minus 17 frames! */

    hr = IAudioClient_Stop(ac);
    ok(hr == S_OK, "Stop failed: %08x\n", hr);

    hr = IAudioClient_GetCurrentPadding(ac, &pad);
    ok(hr == S_OK, "GetCurrentPadding failed: %08x\n", hr);

    hr = IAudioClock_GetPosition(acl, &pos, NULL);
    ok(hr == S_OK, "GetPosition failed: %08x\n", hr);
    trace("padding %u position %u past stop #2\n", pad, (UINT)pos);
    ok(pos * pwfx->nSamplesPerSec <= sum * freq, "Position %u > written %u\n", (UINT)pos, sum);
    /* Prove that Stop must not drop frames (in shared mode). */
    ok(pad ? pos > last : pos >= last, "Position %u vs. last %u\n", (UINT)pos,(UINT)last);
    if (share && pad > 0 && winetest_debug>1)
        ok(pos*1000/freq <= slept*1.1, "Position %u too far after playing %ums\n", (UINT)pos, slept);
    /* in exclusive mode, testbot's w7 machines yield pos > sum-pad */
    if(/*share &&*/ winetest_debug>1)
        ok(pos * pwfx->nSamplesPerSec == (sum-pad) * freq,
           "Position %u after stop vs. %u padding\n", (UINT)pos, pad);
    last = pos;

    Sleep(100);

    hr = IAudioClock_GetPosition(acl, &pos, NULL);
    ok(hr == S_OK, "GetPosition failed: %08x\n", hr);
    ok(pos == last, "Position %u should stop.\n", (UINT)pos);

    /* Restart from 0 */
    hr = IAudioClient_Reset(ac);
    ok(hr == S_OK, "Reset failed: %08x\n", hr);
    slept = sum = 0;

    hr = IAudioClient_Reset(ac);
    ok(hr == S_OK, "Reset on a resetted stream returns %08x\n", hr);

    hr = IAudioClock_GetPosition(acl, &pos, &pcpos);
    ok(hr == S_OK, "GetPosition failed: %08x\n", hr);
    ok(pos == 0, "GetPosition returned non-zero pos after Reset\n");
    ok(pcpos > pcpos0, "pcpos should increase\n");

    avail = gbsize; /* implies GetCurrentPadding == 0 */
    hr = IAudioRenderClient_GetBuffer(arc, avail, &data);
    ok(hr == S_OK, "GetBuffer failed: %08x\n", hr);
    trace("data at %p\n", data);

    hr = IAudioRenderClient_ReleaseBuffer(arc, avail, winetest_debug>2 ?
        wave_generate_tone(pwfx, data, avail) : AUDCLNT_BUFFERFLAGS_SILENT);
    ok(hr == S_OK, "ReleaseBuffer failed: %08x\n", hr);
    if(hr == S_OK) sum += avail;

    hr = IAudioClient_GetCurrentPadding(ac, &pad);
    ok(hr == S_OK, "GetCurrentPadding failed: %08x\n", hr);
    ok(pad == sum, "padding %u prior to start\n", pad);

    hr = IAudioClock_GetPosition(acl, &pos, NULL);
    ok(hr == S_OK, "GetPosition failed: %08x\n", hr);
    ok(pos == 0, "GetPosition returned non-zero pos after Reset\n");
    last = pos;

    hr = IAudioClient_Start(ac); /* #3 */
    ok(hr == S_OK, "Start failed: %08x\n", hr);

    Sleep(100);
    slept += 100;

    hr = IAudioClock_GetPosition(acl, &pos, NULL);
    ok(hr == S_OK, "GetPosition failed: %08x\n", hr);
    trace("position %u past %ums sleep #3\n", (UINT)pos, slept);
    ok(pos > last, "Position %u vs. last %u\n", (UINT)pos,(UINT)last);
    ok(pos * pwfx->nSamplesPerSec <= sum * freq, "Position %u > written %u\n", (UINT)pos, sum);
    if (winetest_debug>1)
        ok(pos*1000/freq <= slept*1.1, "Position %u too far after playing %ums\n", (UINT)pos, slept);
    else
        skip("Rerun with WINETEST_DEBUG=2 for GetPosition tests.\n");
    last = pos;

    hr = IAudioClient_Reset(ac);
    ok(hr == AUDCLNT_E_NOT_STOPPED, "Reset while playing: %08x\n", hr);

    hr = IAudioClient_Stop(ac);
    ok(hr == S_OK, "Stop failed: %08x\n", hr);

    hr = IAudioClient_GetCurrentPadding(ac, &pad);
    ok(hr == S_OK, "GetCurrentPadding failed: %08x\n", hr);

    hr = IAudioClock_GetPosition(acl, &pos, &pcpos);
    ok(hr == S_OK, "GetPosition failed: %08x\n", hr);
    trace("padding %u position %u past stop #3\n", pad, (UINT)pos);
    ok(pos >= last, "Position %u vs. last %u\n", (UINT)pos,(UINT)last);
    ok(pcpos > pcpos0, "pcpos should increase\n");
    ok(pos * pwfx->nSamplesPerSec <= sum * freq, "Position %u > written %u\n", (UINT)pos, sum);
    if (pad > 0 && winetest_debug>1)
        ok(pos*1000/freq <= slept*1.1, "Position %u too far after stop %ums\n", (UINT)pos, slept);
    if(winetest_debug>1)
        ok(pos * pwfx->nSamplesPerSec == (sum-pad) * freq,
           "Position %u after stop vs. %u padding\n", (UINT)pos, pad);
    last = pos;

    /* Begin the big loop */
    hr = IAudioClient_Reset(ac);
    ok(hr == S_OK, "Reset failed: %08x\n", hr);
    slept = last = sum = 0;
    pcpos0 = pcpos;

    ok(QueryPerformanceCounter(&hpctime0), "PerfCounter unavailable\n");

    hr = IAudioClient_Reset(ac);
    ok(hr == S_OK, "Reset on a resetted stream returns %08x\n", hr);

    hr = IAudioClient_Start(ac);
    ok(hr == S_OK, "Start failed: %08x\n", hr);

    avail = pwfx->nSamplesPerSec * 15 / 16 / 2;
    data = NULL;
    hr = IAudioRenderClient_GetBuffer(arc, avail, &data);
    ok(hr == S_OK, "GetBuffer failed: %08x\n", hr);
    trace("data at %p for prefill %u\n", data, avail);

    if (winetest_debug>2) {
        hr = IAudioClient_Stop(ac);
        ok(hr == S_OK, "Stop failed: %08x\n", hr);

        Sleep(20);
        slept += 20;

        hr = IAudioClient_Reset(ac);
        ok(hr == AUDCLNT_E_BUFFER_OPERATION_PENDING, "Reset failed: %08x\n", hr);

        hr = IAudioClient_Start(ac);
        ok(hr == S_OK, "Start failed: %08x\n", hr);
    }

    /* Despite passed time, data must still point to valid memory... */
    hr = IAudioRenderClient_ReleaseBuffer(arc, avail,
        wave_generate_tone(pwfx, data, avail));
    ok(hr == S_OK, "ReleaseBuffer after stop+start failed: %08x\n", hr);
    if(hr == S_OK) sum += avail;

    /* GetCurrentPadding(GCP) == 0 does not mean an underrun happened, as the
     * mixer may still have a little data.  We believe an underrun will occur
     * when the mixer finds GCP smaller than a period size at the *end* of a
     * period cycle, i.e. shortly before calling SetEvent to signal the app
     * that it has ~10ms to supply data for the next cycle.  IOW, a zero GCP
     * with no data written for over a period causes an underrun. */

    Sleep(350);
    slept += 350;
    ok(QueryPerformanceCounter(&hpctime), "PerfCounter failed\n");
    trace("hpctime %u after %ums\n",
        (ULONG)((hpctime.QuadPart-hpctime0.QuadPart)*1000/hpcfreq.QuadPart), slept);

    hr = IAudioClock_GetPosition(acl, &pos, &pcpos);
    ok(hr == S_OK, "GetPosition failed: %08x\n", hr);
    ok(pos > last, "Position %u vs. last %u\n", (UINT)pos,(UINT)last);
    last = pos;

    for(i=0; i < 9; i++) {
        Sleep(100);
        slept += 100;

        hr = IAudioClock_GetPosition(acl, &pos, &pcpos);
        ok(hr == S_OK, "GetPosition failed: %08x\n", hr);

        hr = IAudioClient_GetCurrentPadding(ac, &pad);
        ok(hr == S_OK, "GetCurrentPadding failed: %08x\n", hr);

        ok(QueryPerformanceCounter(&hpctime), "PerfCounter failed\n");
        trace("hpctime %u pcpos %u\n",
              (ULONG)((hpctime.QuadPart-hpctime0.QuadPart)*1000/hpcfreq.QuadPart),
              (ULONG)((pcpos-pcpos0)/10000));

        /* Use sum-pad to see whether position is ahead padding or not. */
        trace("padding %u position %u/%u slept %ums iteration %d\n", pad, (UINT)pos, sum-pad, slept, i);
        ok(pad ? pos > last : pos >= last, "No position increase at iteration %d\n", i);
        ok(pos * pwfx->nSamplesPerSec <= sum * freq, "Position %u > written %u\n", (UINT)pos, sum);
        if (winetest_debug>1) {
            /* Padding does not lag behind by much */
            ok(pos * pwfx->nSamplesPerSec <= (sum-pad+fragment) * freq, "Position %u > written %u\n", (UINT)pos, sum);
            ok(pos*1000/freq <= slept*1.1, "Position %u too far after %ums\n", (UINT)pos, slept);
            if (pad) /* not in case of underrun */
                ok((pos-last)*1000/freq >= 90 && 110 >= (pos-last)*1000/freq,
                   "Position delta %ld not regular: %ld ms\n", (long)(pos-last), (long)((pos-last)*1000/freq));
        }
        last = pos;

        hr = IAudioClient_GetStreamLatency(ac, &t1);
        ok(hr == S_OK, "GetStreamLatency failed: %08x\n", hr);
        ok(t1 == t2, "Latency not constant, delta %ld\n", (long)(t1-t2));

        avail = pwfx->nSamplesPerSec * 15 / 16 / 2;
        data = NULL;
        hr = IAudioRenderClient_GetBuffer(arc, avail, &data);
        /* ok(hr == AUDCLNT_E_BUFFER_TOO_LARGE || (hr == S_OK && i==0) without todo_wine */
        ok(hr == S_OK || hr == AUDCLNT_E_BUFFER_TOO_LARGE,
           "GetBuffer large (%u) failed: %08x\n", avail, hr);
        if(hr == S_OK && i) ok(FALSE, "GetBuffer large (%u) at iteration %d\n", avail, i);
        /* Only the first iteration should allow that large a buffer
         * as prefill was drained during the first 350+100ms sleep.
         * Afterwards, only 100ms of data should find room per iteration. */

        if(hr == S_OK) {
            trace("data at %p\n", data);
        } else {
            avail = gbsize - pad;
            hr = IAudioRenderClient_GetBuffer(arc, avail, &data);
            ok(hr == S_OK, "GetBuffer small %u failed: %08x\n", avail, hr);
            trace("data at %p (small %u)\n", data, avail);
        }
        ok(data != NULL, "NULL buffer returned\n");
        if(i % 3 && !winetest_interactive) {
            memset(data, 0, avail * pwfx->nBlockAlign);
            hr = IAudioRenderClient_ReleaseBuffer(arc, avail, 0);
        } else {
            hr = IAudioRenderClient_ReleaseBuffer(arc, avail,
                wave_generate_tone(pwfx, data, avail));
        }
        ok(hr == S_OK, "ReleaseBuffer failed: %08x\n", hr);
        if(hr == S_OK) sum += avail;
    }

    hr = IAudioClock_GetPosition(acl, &pos, NULL);
    ok(hr == S_OK, "GetPosition failed: %08x\n", hr);
    trace("position %u\n", (UINT)pos);

    Sleep(1000); /* 500ms buffer underrun past full buffer */

    hr = IAudioClient_GetCurrentPadding(ac, &pad);
    ok(hr == S_OK, "GetCurrentPadding failed: %08x\n", hr);

    hr = IAudioClock_GetPosition(acl, &pos, NULL);
    ok(hr == S_OK, "GetPosition failed: %08x\n", hr);
    trace("position %u past underrun, %u padding left, %u frames written\n", (UINT)pos, pad, sum);

    if (share) {
        /* Following underrun, all samples were played */
        ok(pad == 0, "GetCurrentPadding returned %u, should be 0\n", pad);
        ok(pos * pwfx->nSamplesPerSec == sum * freq,
           "Position %u at end vs. %u submitted frames\n", (UINT)pos, sum);
    } else {
        /* Vista and w2k8 leave partial fragments behind */
        ok(pad == 0 /* w7, w2k8R2 */||
           pos * pwfx->nSamplesPerSec == (sum-pad) * freq, "GetCurrentPadding returned %u, should be 0\n", pad);
        /* expect at most 5 fragments (75ms) away */
        ok(pos * pwfx->nSamplesPerSec <= sum * freq &&
           pos * pwfx->nSamplesPerSec + 5 * fragment * freq >= sum * freq,
           "Position %u at end vs. %u submitted frames\n", (UINT)pos, sum);
    }

    hr = IAudioClient_GetStreamLatency(ac, &t1);
    ok(hr == S_OK, "GetStreamLatency failed: %08x\n", hr);
    ok(t1 == t2, "Latency not constant, delta %ld\n", (long)(t1-t2));

    ok(QueryPerformanceCounter(&hpctime), "PerfCounter failed\n");
    trace("hpctime %u after underrun\n", (ULONG)((hpctime.QuadPart-hpctime0.QuadPart)*1000/hpcfreq.QuadPart));

    hr = IAudioClient_Stop(ac);
    ok(hr == S_OK, "Stop failed: %08x\n", hr);

    CoTaskMemFree(pwfx);

    IAudioClock_Release(acl);
    IAudioRenderClient_Release(arc);
    IAudioClient_Release(ac);
}

static void test_session(void)
{
    IAudioClient *ses1_ac1, *ses1_ac2, *cap_ac;
    IAudioSessionControl2 *ses1_ctl, *ses1_ctl2, *cap_ctl = NULL;
    IMMDevice *cap_dev;
    GUID ses1_guid;
    AudioSessionState state;
    WAVEFORMATEX *pwfx;
    ULONG ref;
    HRESULT hr;

    hr = CoCreateGuid(&ses1_guid);
    ok(hr == S_OK, "CoCreateGuid failed: %08x\n", hr);

    hr = IMMDevice_Activate(dev, &IID_IAudioClient, CLSCTX_INPROC_SERVER,
            NULL, (void**)&ses1_ac1);
    ok(hr == S_OK, "Activation failed with %08x\n", hr);
    if (FAILED(hr)) return;

    hr = IAudioClient_GetMixFormat(ses1_ac1, &pwfx);
    ok(hr == S_OK, "GetMixFormat failed: %08x\n", hr);

    hr = IAudioClient_Initialize(ses1_ac1, AUDCLNT_SHAREMODE_SHARED,
            0, 5000000, 0, pwfx, &ses1_guid);
    ok(hr == S_OK, "Initialize failed: %08x\n", hr);

    if(hr == S_OK){
        hr = IMMDevice_Activate(dev, &IID_IAudioClient, CLSCTX_INPROC_SERVER,
                NULL, (void**)&ses1_ac2);
        ok(hr == S_OK, "Activation failed with %08x\n", hr);
    }
    if(hr != S_OK){
        skip("Unable to open the same device twice. Skipping session tests\n");

        ref = IAudioClient_Release(ses1_ac1);
        ok(ref == 0, "AudioClient wasn't released: %u\n", ref);
        CoTaskMemFree(pwfx);
        return;
    }

    hr = IAudioClient_Initialize(ses1_ac2, AUDCLNT_SHAREMODE_SHARED,
            0, 5000000, 0, pwfx, &ses1_guid);
    ok(hr == S_OK, "Initialize failed: %08x\n", hr);

    hr = IMMDeviceEnumerator_GetDefaultAudioEndpoint(mme, eCapture,
            eMultimedia, &cap_dev);
    if(hr == S_OK){
        hr = IMMDevice_Activate(cap_dev, &IID_IAudioClient, CLSCTX_INPROC_SERVER,
                NULL, (void**)&cap_ac);
        ok((hr == S_OK)^(cap_ac == NULL), "Activate %08x &out pointer\n", hr);
        ok(hr == S_OK, "Activate failed: %08x\n", hr);
        IMMDevice_Release(cap_dev);
    }
    if(hr == S_OK){
        WAVEFORMATEX *cap_pwfx;

        hr = IAudioClient_GetMixFormat(cap_ac, &cap_pwfx);
        ok(hr == S_OK, "GetMixFormat failed: %08x\n", hr);

        hr = IAudioClient_Initialize(cap_ac, AUDCLNT_SHAREMODE_SHARED,
                0, 5000000, 0, cap_pwfx, &ses1_guid);
        ok(hr == S_OK, "Initialize failed for capture in rendering session: %08x\n", hr);
        CoTaskMemFree(cap_pwfx);
    }
    if(hr == S_OK){
        hr = IAudioClient_GetService(cap_ac, &IID_IAudioSessionControl, (void**)&cap_ctl);
        ok(hr == S_OK, "GetService failed: %08x\n", hr);
        if(FAILED(hr))
            cap_ctl = NULL;
    }else
        skip("No capture session: %08x; skipping capture device in render session tests\n", hr);

    hr = IAudioClient_GetService(ses1_ac1, &IID_IAudioSessionControl2, (void**)&ses1_ctl);
    ok(hr == E_NOINTERFACE, "GetService gave wrong error: %08x\n", hr);

    hr = IAudioClient_GetService(ses1_ac1, &IID_IAudioSessionControl, (void**)&ses1_ctl);
    ok(hr == S_OK, "GetService failed: %08x\n", hr);

    hr = IAudioClient_GetService(ses1_ac1, &IID_IAudioSessionControl, (void**)&ses1_ctl2);
    ok(hr == S_OK, "GetService failed: %08x\n", hr);
    ok(ses1_ctl == ses1_ctl2, "Got different controls: %p %p\n", ses1_ctl, ses1_ctl2);
    ref = IAudioSessionControl2_Release(ses1_ctl2);
    ok(ref != 0, "AudioSessionControl was destroyed\n");

    hr = IAudioClient_GetService(ses1_ac2, &IID_IAudioSessionControl, (void**)&ses1_ctl2);
    ok(hr == S_OK, "GetService failed: %08x\n", hr);

    hr = IAudioSessionControl2_GetState(ses1_ctl, NULL);
    ok(hr == NULL_PTR_ERR, "GetState gave wrong error: %08x\n", hr);

    hr = IAudioSessionControl2_GetState(ses1_ctl, &state);
    ok(hr == S_OK, "GetState failed: %08x\n", hr);
    ok(state == AudioSessionStateInactive, "Got wrong state: %d\n", state);

    hr = IAudioSessionControl2_GetState(ses1_ctl2, &state);
    ok(hr == S_OK, "GetState failed: %08x\n", hr);
    ok(state == AudioSessionStateInactive, "Got wrong state: %d\n", state);

    if(cap_ctl){
        hr = IAudioSessionControl2_GetState(cap_ctl, &state);
        ok(hr == S_OK, "GetState failed: %08x\n", hr);
        ok(state == AudioSessionStateInactive, "Got wrong state: %d\n", state);
    }

    hr = IAudioClient_Start(ses1_ac1);
    ok(hr == S_OK, "Start failed: %08x\n", hr);

    hr = IAudioSessionControl2_GetState(ses1_ctl, &state);
    ok(hr == S_OK, "GetState failed: %08x\n", hr);
    ok(state == AudioSessionStateActive, "Got wrong state: %d\n", state);

    hr = IAudioSessionControl2_GetState(ses1_ctl2, &state);
    ok(hr == S_OK, "GetState failed: %08x\n", hr);
    ok(state == AudioSessionStateActive, "Got wrong state: %d\n", state);

    if(cap_ctl){
        hr = IAudioSessionControl2_GetState(cap_ctl, &state);
        ok(hr == S_OK, "GetState failed: %08x\n", hr);
        ok(state == AudioSessionStateInactive, "Got wrong state: %d\n", state);
    }

    hr = IAudioClient_Stop(ses1_ac1);
    ok(hr == S_OK, "Start failed: %08x\n", hr);

    hr = IAudioSessionControl2_GetState(ses1_ctl, &state);
    ok(hr == S_OK, "GetState failed: %08x\n", hr);
    ok(state == AudioSessionStateInactive, "Got wrong state: %d\n", state);

    hr = IAudioSessionControl2_GetState(ses1_ctl2, &state);
    ok(hr == S_OK, "GetState failed: %08x\n", hr);
    ok(state == AudioSessionStateInactive, "Got wrong state: %d\n", state);

    if(cap_ctl){
        hr = IAudioSessionControl2_GetState(cap_ctl, &state);
        ok(hr == S_OK, "GetState failed: %08x\n", hr);
        ok(state == AudioSessionStateInactive, "Got wrong state: %d\n", state);

        hr = IAudioClient_Start(cap_ac);
        ok(hr == S_OK, "Start failed: %08x\n", hr);

        hr = IAudioSessionControl2_GetState(ses1_ctl, &state);
        ok(hr == S_OK, "GetState failed: %08x\n", hr);
        ok(state == AudioSessionStateInactive, "Got wrong state: %d\n", state);

        hr = IAudioSessionControl2_GetState(ses1_ctl2, &state);
        ok(hr == S_OK, "GetState failed: %08x\n", hr);
        ok(state == AudioSessionStateInactive, "Got wrong state: %d\n", state);

        hr = IAudioSessionControl2_GetState(cap_ctl, &state);
        ok(hr == S_OK, "GetState failed: %08x\n", hr);
        ok(state == AudioSessionStateActive, "Got wrong state: %d\n", state);

        hr = IAudioClient_Stop(cap_ac);
        ok(hr == S_OK, "Stop failed: %08x\n", hr);

        hr = IAudioSessionControl2_GetState(ses1_ctl, &state);
        ok(hr == S_OK, "GetState failed: %08x\n", hr);
        ok(state == AudioSessionStateInactive, "Got wrong state: %d\n", state);

        hr = IAudioSessionControl2_GetState(ses1_ctl2, &state);
        ok(hr == S_OK, "GetState failed: %08x\n", hr);
        ok(state == AudioSessionStateInactive, "Got wrong state: %d\n", state);

        hr = IAudioSessionControl2_GetState(cap_ctl, &state);
        ok(hr == S_OK, "GetState failed: %08x\n", hr);
        ok(state == AudioSessionStateInactive, "Got wrong state: %d\n", state);

        ref = IAudioSessionControl2_Release(cap_ctl);
        ok(ref == 0, "AudioSessionControl wasn't released: %u\n", ref);

        ref = IAudioClient_Release(cap_ac);
        ok(ref == 0, "AudioClient wasn't released: %u\n", ref);
    }

    ref = IAudioSessionControl2_Release(ses1_ctl);
    ok(ref == 0, "AudioSessionControl wasn't released: %u\n", ref);

    ref = IAudioClient_Release(ses1_ac1);
    ok(ref == 0, "AudioClient wasn't released: %u\n", ref);

    ref = IAudioClient_Release(ses1_ac2);
    ok(ref == 1, "AudioClient had wrong refcount: %u\n", ref);

    /* we've released all of our IAudioClient references, so check GetState */
    hr = IAudioSessionControl2_GetState(ses1_ctl2, &state);
    ok(hr == S_OK, "GetState failed: %08x\n", hr);
    ok(state == AudioSessionStateInactive, "Got wrong state: %d\n", state);

    ref = IAudioSessionControl2_Release(ses1_ctl2);
    ok(ref == 0, "AudioSessionControl wasn't released: %u\n", ref);

    CoTaskMemFree(pwfx);
}

static void test_streamvolume(void)
{
    IAudioClient *ac;
    IAudioStreamVolume *asv;
    WAVEFORMATEX *fmt;
    UINT32 chans, i;
    HRESULT hr;
    float vol, *vols;

    hr = IMMDevice_Activate(dev, &IID_IAudioClient, CLSCTX_INPROC_SERVER,
            NULL, (void**)&ac);
    ok(hr == S_OK, "Activation failed with %08x\n", hr);
    if(hr != S_OK)
        return;

    hr = IAudioClient_GetMixFormat(ac, &fmt);
    ok(hr == S_OK, "GetMixFormat failed: %08x\n", hr);

    hr = IAudioClient_Initialize(ac, AUDCLNT_SHAREMODE_SHARED, 0, 5000000,
            0, fmt, NULL);
    ok(hr == S_OK, "Initialize failed: %08x\n", hr);

    if(hr == S_OK){
        hr = IAudioClient_GetService(ac, &IID_IAudioStreamVolume, (void**)&asv);
        ok(hr == S_OK, "GetService failed: %08x\n", hr);
    }
    if(hr != S_OK){
        IAudioClient_Release(ac);
        CoTaskMemFree(fmt);
        return;
    }

    hr = IAudioStreamVolume_GetChannelCount(asv, NULL);
    ok(hr == E_POINTER, "GetChannelCount gave wrong error: %08x\n", hr);

    hr = IAudioStreamVolume_GetChannelCount(asv, &chans);
    ok(hr == S_OK, "GetChannelCount failed: %08x\n", hr);
    ok(chans == fmt->nChannels, "GetChannelCount gave wrong number of channels: %d\n", chans);

    hr = IAudioStreamVolume_GetChannelVolume(asv, fmt->nChannels, NULL);
    ok(hr == E_POINTER, "GetChannelCount gave wrong error: %08x\n", hr);

    hr = IAudioStreamVolume_GetChannelVolume(asv, fmt->nChannels, &vol);
    ok(hr == E_INVALIDARG, "GetChannelCount gave wrong error: %08x\n", hr);

    hr = IAudioStreamVolume_GetChannelVolume(asv, 0, NULL);
    ok(hr == E_POINTER, "GetChannelCount gave wrong error: %08x\n", hr);

    hr = IAudioStreamVolume_GetChannelVolume(asv, 0, &vol);
    ok(hr == S_OK, "GetChannelCount failed: %08x\n", hr);
    ok(vol == 1.f, "Channel volume was not 1: %f\n", vol);

    hr = IAudioStreamVolume_SetChannelVolume(asv, fmt->nChannels, -1.f);
    ok(hr == E_INVALIDARG, "SetChannelVolume gave wrong error: %08x\n", hr);

    hr = IAudioStreamVolume_SetChannelVolume(asv, 0, -1.f);
    ok(hr == E_INVALIDARG, "SetChannelVolume gave wrong error: %08x\n", hr);

    hr = IAudioStreamVolume_SetChannelVolume(asv, 0, 2.f);
    ok(hr == E_INVALIDARG, "SetChannelVolume gave wrong error: %08x\n", hr);

    hr = IAudioStreamVolume_SetChannelVolume(asv, 0, 0.2f);
    ok(hr == S_OK, "SetChannelVolume failed: %08x\n", hr);

    hr = IAudioStreamVolume_GetChannelVolume(asv, 0, &vol);
    ok(hr == S_OK, "GetChannelCount failed: %08x\n", hr);
    ok(fabsf(vol - 0.2f) < 0.05f, "Channel volume wasn't 0.2: %f\n", vol);

    hr = IAudioStreamVolume_GetAllVolumes(asv, 0, NULL);
    ok(hr == E_POINTER, "GetAllVolumes gave wrong error: %08x\n", hr);

    hr = IAudioStreamVolume_GetAllVolumes(asv, fmt->nChannels, NULL);
    ok(hr == E_POINTER, "GetAllVolumes gave wrong error: %08x\n", hr);

    vols = HeapAlloc(GetProcessHeap(), 0, fmt->nChannels * sizeof(float));
    ok(vols != NULL, "HeapAlloc failed\n");

    hr = IAudioStreamVolume_GetAllVolumes(asv, fmt->nChannels - 1, vols);
    ok(hr == E_INVALIDARG, "GetAllVolumes gave wrong error: %08x\n", hr);

    hr = IAudioStreamVolume_GetAllVolumes(asv, fmt->nChannels, vols);
    ok(hr == S_OK, "GetAllVolumes failed: %08x\n", hr);
    ok(fabsf(vols[0] - 0.2f) < 0.05f, "Channel 0 volume wasn't 0.2: %f\n", vol);
    for(i = 1; i < fmt->nChannels; ++i)
        ok(vols[i] == 1.f, "Channel %d volume is not 1: %f\n", i, vols[i]);

    hr = IAudioStreamVolume_SetAllVolumes(asv, 0, NULL);
    ok(hr == E_POINTER, "SetAllVolumes gave wrong error: %08x\n", hr);

    hr = IAudioStreamVolume_SetAllVolumes(asv, fmt->nChannels, NULL);
    ok(hr == E_POINTER, "SetAllVolumes gave wrong error: %08x\n", hr);

    hr = IAudioStreamVolume_SetAllVolumes(asv, fmt->nChannels - 1, vols);
    ok(hr == E_INVALIDARG, "SetAllVolumes gave wrong error: %08x\n", hr);

    hr = IAudioStreamVolume_SetAllVolumes(asv, fmt->nChannels, vols);
    ok(hr == S_OK, "SetAllVolumes failed: %08x\n", hr);

    HeapFree(GetProcessHeap(), 0, vols);
    IAudioStreamVolume_Release(asv);
    IAudioClient_Release(ac);
    CoTaskMemFree(fmt);
}

static void test_channelvolume(void)
{
    IAudioClient *ac;
    IChannelAudioVolume *acv;
    WAVEFORMATEX *fmt;
    UINT32 chans, i;
    HRESULT hr;
    float vol, *vols;

    hr = IMMDevice_Activate(dev, &IID_IAudioClient, CLSCTX_INPROC_SERVER,
            NULL, (void**)&ac);
    ok(hr == S_OK, "Activation failed with %08x\n", hr);
    if(hr != S_OK)
        return;

    hr = IAudioClient_GetMixFormat(ac, &fmt);
    ok(hr == S_OK, "GetMixFormat failed: %08x\n", hr);

    hr = IAudioClient_Initialize(ac, AUDCLNT_SHAREMODE_SHARED,
            AUDCLNT_STREAMFLAGS_NOPERSIST, 5000000, 0, fmt, NULL);
    ok(hr == S_OK, "Initialize failed: %08x\n", hr);

    if(hr == S_OK){
        hr = IAudioClient_GetService(ac, &IID_IChannelAudioVolume, (void**)&acv);
        ok(hr == S_OK, "GetService failed: %08x\n", hr);
    }
    if(hr != S_OK){
        IAudioClient_Release(ac);
        CoTaskMemFree(fmt);
        return;
    }

    hr = IChannelAudioVolume_GetChannelCount(acv, NULL);
    ok(hr == NULL_PTR_ERR, "GetChannelCount gave wrong error: %08x\n", hr);

    hr = IChannelAudioVolume_GetChannelCount(acv, &chans);
    ok(hr == S_OK, "GetChannelCount failed: %08x\n", hr);
    ok(chans == fmt->nChannels, "GetChannelCount gave wrong number of channels: %d\n", chans);

    hr = IChannelAudioVolume_GetChannelVolume(acv, fmt->nChannels, NULL);
    ok(hr == NULL_PTR_ERR, "GetChannelCount gave wrong error: %08x\n", hr);

    hr = IChannelAudioVolume_GetChannelVolume(acv, fmt->nChannels, &vol);
    ok(hr == E_INVALIDARG, "GetChannelCount gave wrong error: %08x\n", hr);

    hr = IChannelAudioVolume_GetChannelVolume(acv, 0, NULL);
    ok(hr == NULL_PTR_ERR, "GetChannelCount gave wrong error: %08x\n", hr);

    hr = IChannelAudioVolume_GetChannelVolume(acv, 0, &vol);
    ok(hr == S_OK, "GetChannelCount failed: %08x\n", hr);
    ok(vol == 1.f, "Channel volume was not 1: %f\n", vol);

    hr = IChannelAudioVolume_SetChannelVolume(acv, fmt->nChannels, -1.f, NULL);
    ok(hr == E_INVALIDARG, "SetChannelVolume gave wrong error: %08x\n", hr);

    hr = IChannelAudioVolume_SetChannelVolume(acv, 0, -1.f, NULL);
    ok(hr == E_INVALIDARG, "SetChannelVolume gave wrong error: %08x\n", hr);

    hr = IChannelAudioVolume_SetChannelVolume(acv, 0, 2.f, NULL);
    ok(hr == E_INVALIDARG, "SetChannelVolume gave wrong error: %08x\n", hr);

    hr = IChannelAudioVolume_SetChannelVolume(acv, 0, 0.2f, NULL);
    ok(hr == S_OK, "SetChannelVolume failed: %08x\n", hr);

    hr = IChannelAudioVolume_GetChannelVolume(acv, 0, &vol);
    ok(hr == S_OK, "GetChannelCount failed: %08x\n", hr);
    ok(fabsf(vol - 0.2f) < 0.05f, "Channel volume wasn't 0.2: %f\n", vol);

    hr = IChannelAudioVolume_GetAllVolumes(acv, 0, NULL);
    ok(hr == NULL_PTR_ERR, "GetAllVolumes gave wrong error: %08x\n", hr);

    hr = IChannelAudioVolume_GetAllVolumes(acv, fmt->nChannels, NULL);
    ok(hr == NULL_PTR_ERR, "GetAllVolumes gave wrong error: %08x\n", hr);

    vols = HeapAlloc(GetProcessHeap(), 0, fmt->nChannels * sizeof(float));
    ok(vols != NULL, "HeapAlloc failed\n");

    hr = IChannelAudioVolume_GetAllVolumes(acv, fmt->nChannels - 1, vols);
    ok(hr == E_INVALIDARG, "GetAllVolumes gave wrong error: %08x\n", hr);

    hr = IChannelAudioVolume_GetAllVolumes(acv, fmt->nChannels, vols);
    ok(hr == S_OK, "GetAllVolumes failed: %08x\n", hr);
    ok(fabsf(vols[0] - 0.2f) < 0.05f, "Channel 0 volume wasn't 0.2: %f\n", vol);
    for(i = 1; i < fmt->nChannels; ++i)
        ok(vols[i] == 1.f, "Channel %d volume is not 1: %f\n", i, vols[i]);

    hr = IChannelAudioVolume_SetAllVolumes(acv, 0, NULL, NULL);
    ok(hr == NULL_PTR_ERR, "SetAllVolumes gave wrong error: %08x\n", hr);

    hr = IChannelAudioVolume_SetAllVolumes(acv, fmt->nChannels, NULL, NULL);
    ok(hr == NULL_PTR_ERR, "SetAllVolumes gave wrong error: %08x\n", hr);

    hr = IChannelAudioVolume_SetAllVolumes(acv, fmt->nChannels - 1, vols, NULL);
    ok(hr == E_INVALIDARG, "SetAllVolumes gave wrong error: %08x\n", hr);

    hr = IChannelAudioVolume_SetAllVolumes(acv, fmt->nChannels, vols, NULL);
    ok(hr == S_OK, "SetAllVolumes failed: %08x\n", hr);

    hr = IChannelAudioVolume_SetChannelVolume(acv, 0, 1.0f, NULL);
    ok(hr == S_OK, "SetChannelVolume failed: %08x\n", hr);

    HeapFree(GetProcessHeap(), 0, vols);
    IChannelAudioVolume_Release(acv);
    IAudioClient_Release(ac);
    CoTaskMemFree(fmt);
}

static void test_simplevolume(void)
{
    IAudioClient *ac;
    ISimpleAudioVolume *sav;
    WAVEFORMATEX *fmt;
    HRESULT hr;
    float vol;
    BOOL mute;

    hr = IMMDevice_Activate(dev, &IID_IAudioClient, CLSCTX_INPROC_SERVER,
            NULL, (void**)&ac);
    ok(hr == S_OK, "Activation failed with %08x\n", hr);
    if(hr != S_OK)
        return;

    hr = IAudioClient_GetMixFormat(ac, &fmt);
    ok(hr == S_OK, "GetMixFormat failed: %08x\n", hr);

    hr = IAudioClient_Initialize(ac, AUDCLNT_SHAREMODE_SHARED,
            AUDCLNT_STREAMFLAGS_NOPERSIST, 5000000, 0, fmt, NULL);
    ok(hr == S_OK, "Initialize failed: %08x\n", hr);

    if(hr == S_OK){
        hr = IAudioClient_GetService(ac, &IID_ISimpleAudioVolume, (void**)&sav);
        ok(hr == S_OK, "GetService failed: %08x\n", hr);
    }
    if(hr != S_OK){
        IAudioClient_Release(ac);
        CoTaskMemFree(fmt);
        return;
    }

    hr = ISimpleAudioVolume_GetMasterVolume(sav, NULL);
    ok(hr == NULL_PTR_ERR, "GetMasterVolume gave wrong error: %08x\n", hr);

    hr = ISimpleAudioVolume_GetMasterVolume(sav, &vol);
    ok(hr == S_OK, "GetMasterVolume failed: %08x\n", hr);
    ok(vol == 1.f, "Master volume wasn't 1: %f\n", vol);

    hr = ISimpleAudioVolume_SetMasterVolume(sav, -1.f, NULL);
    ok(hr == E_INVALIDARG, "SetMasterVolume gave wrong error: %08x\n", hr);

    hr = ISimpleAudioVolume_SetMasterVolume(sav, 2.f, NULL);
    ok(hr == E_INVALIDARG, "SetMasterVolume gave wrong error: %08x\n", hr);

    hr = ISimpleAudioVolume_SetMasterVolume(sav, 0.2f, NULL);
    ok(hr == S_OK, "SetMasterVolume failed: %08x\n", hr);

    hr = ISimpleAudioVolume_GetMasterVolume(sav, &vol);
    ok(hr == S_OK, "GetMasterVolume failed: %08x\n", hr);
    ok(fabsf(vol - 0.2f) < 0.05f, "Master volume wasn't 0.2: %f\n", vol);

    hr = ISimpleAudioVolume_GetMute(sav, NULL);
    ok(hr == NULL_PTR_ERR, "GetMute gave wrong error: %08x\n", hr);

    mute = TRUE;
    hr = ISimpleAudioVolume_GetMute(sav, &mute);
    ok(hr == S_OK, "GetMute failed: %08x\n", hr);
    ok(mute == FALSE, "Session is already muted\n");

    hr = ISimpleAudioVolume_SetMute(sav, TRUE, NULL);
    ok(hr == S_OK, "SetMute failed: %08x\n", hr);

    mute = FALSE;
    hr = ISimpleAudioVolume_GetMute(sav, &mute);
    ok(hr == S_OK, "GetMute failed: %08x\n", hr);
    ok(mute == TRUE, "Session should have been muted\n");

    hr = ISimpleAudioVolume_GetMasterVolume(sav, &vol);
    ok(hr == S_OK, "GetMasterVolume failed: %08x\n", hr);
    ok(fabsf(vol - 0.2f) < 0.05f, "Master volume wasn't 0.2: %f\n", vol);

    hr = ISimpleAudioVolume_SetMasterVolume(sav, 1.f, NULL);
    ok(hr == S_OK, "SetMasterVolume failed: %08x\n", hr);

    mute = FALSE;
    hr = ISimpleAudioVolume_GetMute(sav, &mute);
    ok(hr == S_OK, "GetMute failed: %08x\n", hr);
    ok(mute == TRUE, "Session should have been muted\n");

    hr = ISimpleAudioVolume_SetMute(sav, FALSE, NULL);
    ok(hr == S_OK, "SetMute failed: %08x\n", hr);

    ISimpleAudioVolume_Release(sav);
    IAudioClient_Release(ac);
    CoTaskMemFree(fmt);
}

static void test_volume_dependence(void)
{
    IAudioClient *ac, *ac2;
    ISimpleAudioVolume *sav;
    IChannelAudioVolume *cav;
    IAudioStreamVolume *asv;
    WAVEFORMATEX *fmt;
    HRESULT hr;
    float vol;
    GUID session;
    UINT32 nch;

    hr = CoCreateGuid(&session);
    ok(hr == S_OK, "CoCreateGuid failed: %08x\n", hr);

    hr = IMMDevice_Activate(dev, &IID_IAudioClient, CLSCTX_INPROC_SERVER,
            NULL, (void**)&ac);
    ok(hr == S_OK, "Activation failed with %08x\n", hr);
    if(hr != S_OK)
        return;

    hr = IAudioClient_GetMixFormat(ac, &fmt);
    ok(hr == S_OK, "GetMixFormat failed: %08x\n", hr);

    hr = IAudioClient_Initialize(ac, AUDCLNT_SHAREMODE_SHARED,
            AUDCLNT_STREAMFLAGS_NOPERSIST, 5000000, 0, fmt, &session);
    ok(hr == S_OK, "Initialize failed: %08x\n", hr);

    if(hr == S_OK){
        hr = IAudioClient_GetService(ac, &IID_ISimpleAudioVolume, (void**)&sav);
        ok(hr == S_OK, "GetService (SimpleAudioVolume) failed: %08x\n", hr);
    }
    if(hr != S_OK){
        IAudioClient_Release(ac);
        CoTaskMemFree(fmt);
        return;
    }

    hr = IAudioClient_GetService(ac, &IID_IChannelAudioVolume, (void**)&cav);
    ok(hr == S_OK, "GetService (ChannelAudioVolme) failed: %08x\n", hr);

    hr = IAudioClient_GetService(ac, &IID_IAudioStreamVolume, (void**)&asv);
    ok(hr == S_OK, "GetService (AudioStreamVolume) failed: %08x\n", hr);

    hr = IAudioStreamVolume_SetChannelVolume(asv, 0, 0.2f);
    ok(hr == S_OK, "ASV_SetChannelVolume failed: %08x\n", hr);

    hr = IChannelAudioVolume_SetChannelVolume(cav, 0, 0.4f, NULL);
    ok(hr == S_OK, "CAV_SetChannelVolume failed: %08x\n", hr);

    hr = ISimpleAudioVolume_SetMasterVolume(sav, 0.6f, NULL);
    ok(hr == S_OK, "SAV_SetMasterVolume failed: %08x\n", hr);

    hr = IAudioStreamVolume_GetChannelVolume(asv, 0, &vol);
    ok(hr == S_OK, "ASV_GetChannelVolume failed: %08x\n", hr);
    ok(fabsf(vol - 0.2f) < 0.05f, "ASV_GetChannelVolume gave wrong volume: %f\n", vol);

    hr = IChannelAudioVolume_GetChannelVolume(cav, 0, &vol);
    ok(hr == S_OK, "CAV_GetChannelVolume failed: %08x\n", hr);
    ok(fabsf(vol - 0.4f) < 0.05f, "CAV_GetChannelVolume gave wrong volume: %f\n", vol);

    hr = ISimpleAudioVolume_GetMasterVolume(sav, &vol);
    ok(hr == S_OK, "SAV_GetMasterVolume failed: %08x\n", hr);
    ok(fabsf(vol - 0.6f) < 0.05f, "SAV_GetMasterVolume gave wrong volume: %f\n", vol);

    hr = IMMDevice_Activate(dev, &IID_IAudioClient, CLSCTX_INPROC_SERVER,
            NULL, (void**)&ac2);
    ok(hr == S_OK, "Activation failed with %08x\n", hr);

    if(hr == S_OK){
        hr = IAudioClient_Initialize(ac2, AUDCLNT_SHAREMODE_SHARED,
                AUDCLNT_STREAMFLAGS_NOPERSIST, 5000000, 0, fmt, &session);
        ok(hr == S_OK, "Initialize failed: %08x\n", hr);
        if(hr != S_OK)
            IAudioClient_Release(ac2);
    }

    if(hr == S_OK){
        IChannelAudioVolume *cav2;
        IAudioStreamVolume *asv2;

        hr = IAudioClient_GetService(ac2, &IID_IChannelAudioVolume, (void**)&cav2);
        ok(hr == S_OK, "GetService failed: %08x\n", hr);

        hr = IAudioClient_GetService(ac2, &IID_IAudioStreamVolume, (void**)&asv2);
        ok(hr == S_OK, "GetService failed: %08x\n", hr);

        hr = IChannelAudioVolume_GetChannelVolume(cav2, 0, &vol);
        ok(hr == S_OK, "CAV_GetChannelVolume failed: %08x\n", hr);
        ok(fabsf(vol - 0.4f) < 0.05f, "CAV_GetChannelVolume gave wrong volume: %f\n", vol);

        hr = IAudioStreamVolume_GetChannelVolume(asv2, 0, &vol);
        ok(hr == S_OK, "ASV_GetChannelVolume failed: %08x\n", hr);
        ok(vol == 1.f, "ASV_GetChannelVolume gave wrong volume: %f\n", vol);

        hr = IChannelAudioVolume_GetChannelCount(cav2, &nch);
        ok(hr == S_OK, "GetChannelCount failed: %08x\n", hr);
        ok(nch == fmt->nChannels, "Got wrong channel count, expected %u: %u\n", fmt->nChannels, nch);

        hr = IAudioStreamVolume_GetChannelCount(asv2, &nch);
        ok(hr == S_OK, "GetChannelCount failed: %08x\n", hr);
        ok(nch == fmt->nChannels, "Got wrong channel count, expected %u: %u\n", fmt->nChannels, nch);

        IAudioStreamVolume_Release(asv2);
        IChannelAudioVolume_Release(cav2);
        IAudioClient_Release(ac2);
    }else
        skip("Unable to open the same device twice. Skipping session volume control tests\n");

    hr = IChannelAudioVolume_SetChannelVolume(cav, 0, 1.f, NULL);
    ok(hr == S_OK, "CAV_SetChannelVolume failed: %08x\n", hr);

    hr = ISimpleAudioVolume_SetMasterVolume(sav, 1.f, NULL);
    ok(hr == S_OK, "SAV_SetMasterVolume failed: %08x\n", hr);

    CoTaskMemFree(fmt);
    ISimpleAudioVolume_Release(sav);
    IChannelAudioVolume_Release(cav);
    IAudioStreamVolume_Release(asv);
    IAudioClient_Release(ac);
}

static void test_session_creation(void)
{
    IMMDevice *cap_dev;
    IAudioClient *ac;
    IAudioSessionManager *sesm;
    ISimpleAudioVolume *sav;
    GUID session_guid;
    float vol;
    HRESULT hr;
    WAVEFORMATEX *fmt;

    CoCreateGuid(&session_guid);

    hr = IMMDevice_Activate(dev, &IID_IAudioSessionManager,
            CLSCTX_INPROC_SERVER, NULL, (void**)&sesm);
    ok((hr == S_OK)^(sesm == NULL), "Activate %08x &out pointer\n", hr);
    ok(hr == S_OK, "Activate failed: %08x\n", hr);

    hr = IAudioSessionManager_GetSimpleAudioVolume(sesm, &session_guid,
            FALSE, &sav);
    ok(hr == S_OK, "GetSimpleAudioVolume failed: %08x\n", hr);

    hr = ISimpleAudioVolume_SetMasterVolume(sav, 0.6f, NULL);
    ok(hr == S_OK, "SetMasterVolume failed: %08x\n", hr);

    /* Release completely to show session persistence */
    ISimpleAudioVolume_Release(sav);
    IAudioSessionManager_Release(sesm);

    /* test if we can create a capture audioclient in the session we just
     * created from a SessionManager derived from a render device */
    hr = IMMDeviceEnumerator_GetDefaultAudioEndpoint(mme, eCapture,
            eMultimedia, &cap_dev);
    if(hr == S_OK){
        WAVEFORMATEX *cap_pwfx;
        IAudioClient *cap_ac;
        ISimpleAudioVolume *cap_sav;
        IAudioSessionManager *cap_sesm;

        hr = IMMDevice_Activate(cap_dev, &IID_IAudioSessionManager,
                CLSCTX_INPROC_SERVER, NULL, (void**)&cap_sesm);
        ok((hr == S_OK)^(cap_sesm == NULL), "Activate %08x &out pointer\n", hr);
        ok(hr == S_OK, "Activate failed: %08x\n", hr);

        hr = IAudioSessionManager_GetSimpleAudioVolume(cap_sesm, &session_guid,
                FALSE, &cap_sav);
        ok(hr == S_OK, "GetSimpleAudioVolume failed: %08x\n", hr);

        vol = 0.5f;
        hr = ISimpleAudioVolume_GetMasterVolume(cap_sav, &vol);
        ok(hr == S_OK, "GetMasterVolume failed: %08x\n", hr);
        ok(vol == 1.f, "Got wrong volume: %f\n", vol);

        ISimpleAudioVolume_Release(cap_sav);
        IAudioSessionManager_Release(cap_sesm);

        hr = IMMDevice_Activate(cap_dev, &IID_IAudioClient,
                CLSCTX_INPROC_SERVER, NULL, (void**)&cap_ac);
        ok(hr == S_OK, "Activate failed: %08x\n", hr);

        IMMDevice_Release(cap_dev);

        hr = IAudioClient_GetMixFormat(cap_ac, &cap_pwfx);
        ok(hr == S_OK, "GetMixFormat failed: %08x\n", hr);

        hr = IAudioClient_Initialize(cap_ac, AUDCLNT_SHAREMODE_SHARED,
                0, 5000000, 0, cap_pwfx, &session_guid);
        ok(hr == S_OK, "Initialize failed: %08x\n", hr);

        CoTaskMemFree(cap_pwfx);

        if(hr == S_OK){
            hr = IAudioClient_GetService(cap_ac, &IID_ISimpleAudioVolume,
                    (void**)&cap_sav);
            ok(hr == S_OK, "GetService failed: %08x\n", hr);
        }
        if(hr == S_OK){
            vol = 0.5f;
            hr = ISimpleAudioVolume_GetMasterVolume(cap_sav, &vol);
            ok(hr == S_OK, "GetMasterVolume failed: %08x\n", hr);
            ok(vol == 1.f, "Got wrong volume: %f\n", vol);

            ISimpleAudioVolume_Release(cap_sav);
        }

        IAudioClient_Release(cap_ac);
    }

    hr = IMMDevice_Activate(dev, &IID_IAudioClient, CLSCTX_INPROC_SERVER,
            NULL, (void**)&ac);
    ok((hr == S_OK)^(ac == NULL), "Activate %08x &out pointer\n", hr);
    ok(hr == S_OK, "Activation failed with %08x\n", hr);
    if(hr != S_OK)
        return;

    hr = IAudioClient_GetMixFormat(ac, &fmt);
    ok(hr == S_OK, "GetMixFormat failed: %08x\n", hr);

    hr = IAudioClient_Initialize(ac, AUDCLNT_SHAREMODE_SHARED,
            AUDCLNT_STREAMFLAGS_NOPERSIST, 5000000, 0, fmt, &session_guid);
    ok(hr == S_OK, "Initialize failed: %08x\n", hr);

    hr = IAudioClient_GetService(ac, &IID_ISimpleAudioVolume, (void**)&sav);
    ok(hr == S_OK, "GetService failed: %08x\n", hr);
    if(hr == S_OK){
        vol = 0.5f;
        hr = ISimpleAudioVolume_GetMasterVolume(sav, &vol);
        ok(hr == S_OK, "GetMasterVolume failed: %08x\n", hr);
        ok(fabs(vol - 0.6f) < 0.05f, "Got wrong volume: %f\n", vol);

        ISimpleAudioVolume_Release(sav);
    }

    CoTaskMemFree(fmt);
    IAudioClient_Release(ac);
}

static void test_worst_case(void)
{
    HANDLE event;
    HRESULT hr;
    IAudioClient *ac;
    IAudioRenderClient *arc;
    IAudioClock *acl;
    WAVEFORMATEX *pwfx;
    REFERENCE_TIME defp;
    UINT64 freq, pos, pcpos0, pcpos;
    BYTE *data;
    DWORD r;
    UINT32 pad, fragment, sum;
    int i,j;

    hr = IMMDevice_Activate(dev, &IID_IAudioClient, CLSCTX_INPROC_SERVER,
            NULL, (void**)&ac);
    ok(hr == S_OK, "Activation failed with %08x\n", hr);
    if(hr != S_OK)
        return;

    hr = IAudioClient_GetMixFormat(ac, &pwfx);
    ok(hr == S_OK, "GetMixFormat failed: %08x\n", hr);

    hr = IAudioClient_Initialize(ac, AUDCLNT_SHAREMODE_SHARED,
            AUDCLNT_STREAMFLAGS_EVENTCALLBACK, 500000, 0, pwfx, NULL);
    ok(hr == S_OK, "Initialize failed: %08x\n", hr);
    if(hr != S_OK)
        return;

    hr = IAudioClient_GetDevicePeriod(ac, &defp, NULL);
    ok(hr == S_OK, "GetDevicePeriod failed: %08x\n", hr);

    fragment  =  MulDiv(defp,   pwfx->nSamplesPerSec, 10000000);

    event = CreateEventW(NULL, FALSE, FALSE, NULL);
    ok(event != NULL, "CreateEvent failed\n");

    hr = IAudioClient_SetEventHandle(ac, event);
    ok(hr == S_OK, "SetEventHandle failed: %08x\n", hr);

    hr = IAudioClient_GetService(ac, &IID_IAudioRenderClient, (void**)&arc);
    ok(hr == S_OK, "GetService(IAudioRenderClient) failed: %08x\n", hr);

    hr = IAudioClient_GetService(ac, &IID_IAudioClock, (void**)&acl);
    ok(hr == S_OK, "GetService(IAudioClock) failed: %08x\n", hr);

    hr = IAudioClock_GetFrequency(acl, &freq);
    ok(hr == S_OK, "GetFrequency failed: %08x\n", hr);

    for(j = 0; j <= (winetest_interactive ? 9 : 2); j++){
        sum = 0;
        trace("Should play %ums continuous tone with fragment size %u.\n",
              (ULONG)(defp/100), fragment);

        hr = IAudioClock_GetPosition(acl, &pos, &pcpos0);
        ok(hr == S_OK, "GetPosition failed: %08x\n", hr);

        /* XAudio2 prefills one period, play without it */
        if(winetest_debug>2){
            hr = IAudioRenderClient_GetBuffer(arc, fragment, &data);
            ok(hr == S_OK, "GetBuffer failed: %08x\n", hr);

            hr = IAudioRenderClient_ReleaseBuffer(arc, fragment, AUDCLNT_BUFFERFLAGS_SILENT);
            ok(hr == S_OK, "ReleaseBuffer failed: %08x\n", hr);
            if(hr == S_OK)
                sum += fragment;
        }

        hr = IAudioClient_Start(ac);
        ok(hr == S_OK, "Start failed: %08x\n", hr);

        for(i = 0; i <= 99; i++){ /* 100 x 10ms = 1 second */
            r = WaitForSingleObject(event, 60 + defp / 10000);
            ok(r == WAIT_OBJECT_0, "Wait iteration %d gave %x\n", i, r);

            /* the app has nearly one period time to feed data */
            Sleep((i % 10) * defp / 120000);

            hr = IAudioClient_GetCurrentPadding(ac, &pad);
            ok(hr == S_OK, "GetCurrentPadding failed: %08x\n", hr);

            /* XAudio2 writes only when there's little data left */
            if(pad <= fragment){
                hr = IAudioRenderClient_GetBuffer(arc, fragment, &data);
                ok(hr == S_OK, "GetBuffer failed: %08x\n", hr);

                hr = IAudioRenderClient_ReleaseBuffer(arc, fragment,
                       wave_generate_tone(pwfx, data, fragment));
                ok(hr == S_OK, "ReleaseBuffer failed: %08x\n", hr);
                if(hr == S_OK)
                    sum += fragment;
            }
        }

        hr = IAudioClient_Stop(ac);
        ok(hr == S_OK, "Stop failed: %08x\n", hr);

        hr = IAudioClient_GetCurrentPadding(ac, &pad);
        ok(hr == S_OK, "GetCurrentPadding failed: %08x\n", hr);

        hr = IAudioClock_GetPosition(acl, &pos, &pcpos);
        ok(hr == S_OK, "GetPosition failed: %08x\n", hr);

        Sleep(100);

        trace("Released %u=%ux%u -%u frames at %u worth %ums in %ums\n",
              sum, sum/fragment, fragment, pad,
              pwfx->nSamplesPerSec, MulDiv(sum-pad, 1000, pwfx->nSamplesPerSec),
              (ULONG)((pcpos-pcpos0)/10000));

        ok(pos * pwfx->nSamplesPerSec == (sum-pad) * freq,
           "Position %u at end vs. %u-%u submitted frames\n", (UINT)pos, sum, pad);

        hr = IAudioClient_Reset(ac);
        ok(hr == S_OK, "Reset failed: %08x\n", hr);

        Sleep(250);
    }

    CoTaskMemFree(pwfx);
    IAudioClient_Release(ac);
    IAudioClock_Release(acl);
    IAudioRenderClient_Release(arc);
}

static void test_marshal(void)
{
    IStream *pStream;
    IAudioClient *ac, *acDest;
    IAudioRenderClient *rc, *rcDest;
    WAVEFORMATEX *pwfx;
    HRESULT hr;

    /* IAudioRenderClient */
    hr = IMMDevice_Activate(dev, &IID_IAudioClient, CLSCTX_INPROC_SERVER,
            NULL, (void**)&ac);
    ok(hr == S_OK, "Activation failed with %08x\n", hr);
    if(hr != S_OK)
        return;

    hr = IAudioClient_GetMixFormat(ac, &pwfx);
    ok(hr == S_OK, "GetMixFormat failed: %08x\n", hr);

    hr = IAudioClient_Initialize(ac, AUDCLNT_SHAREMODE_SHARED, 0, 5000000,
            0, pwfx, NULL);
    ok(hr == S_OK, "Initialize failed: %08x\n", hr);

    CoTaskMemFree(pwfx);

    hr = IAudioClient_GetService(ac, &IID_IAudioRenderClient, (void**)&rc);
    ok(hr == S_OK, "GetService failed: %08x\n", hr);
    if(hr != S_OK) {
        IAudioClient_Release(ac);
        return;
    }

    hr = CreateStreamOnHGlobal(NULL, TRUE, &pStream);
    ok(hr == S_OK, "CreateStreamOnHGlobal failed 0x%08x\n", hr);

    /* marshal IAudioClient */

    hr = CoMarshalInterface(pStream, &IID_IAudioClient, (IUnknown*)ac, MSHCTX_INPROC, NULL, MSHLFLAGS_NORMAL);
    ok(hr == S_OK, "CoMarshalInterface IAudioClient failed 0x%08x\n", hr);

    IStream_Seek(pStream, ullZero, STREAM_SEEK_SET, NULL);
    hr = CoUnmarshalInterface(pStream, &IID_IAudioClient, (void **)&acDest);
    ok(hr == S_OK, "CoUnmarshalInterface IAudioClient failed 0x%08x\n", hr);
    if (hr == S_OK)
        IAudioClient_Release(acDest);

    IStream_Seek(pStream, ullZero, STREAM_SEEK_SET, NULL);
    /* marshal IAudioRenderClient */

    hr = CoMarshalInterface(pStream, &IID_IAudioRenderClient, (IUnknown*)rc, MSHCTX_INPROC, NULL, MSHLFLAGS_NORMAL);
    ok(hr == S_OK, "CoMarshalInterface IAudioRenderClient failed 0x%08x\n", hr);

    IStream_Seek(pStream, ullZero, STREAM_SEEK_SET, NULL);
    hr = CoUnmarshalInterface(pStream, &IID_IAudioRenderClient, (void **)&rcDest);
    ok(hr == S_OK, "CoUnmarshalInterface IAudioRenderClient failed 0x%08x\n", hr);
    if (hr == S_OK)
        IAudioRenderClient_Release(rcDest);


    IStream_Release(pStream);

    IAudioClient_Release(ac);
    IAudioRenderClient_Release(rc);

}

START_TEST(render)
{
    HRESULT hr;

    CoInitializeEx(NULL, COINIT_MULTITHREADED);
    hr = CoCreateInstance(&CLSID_MMDeviceEnumerator, NULL, CLSCTX_INPROC_SERVER, &IID_IMMDeviceEnumerator, (void**)&mme);
    if (FAILED(hr))
    {
        skip("mmdevapi not available: 0x%08x\n", hr);
        goto cleanup;
    }

    hr = IMMDeviceEnumerator_GetDefaultAudioEndpoint(mme, eRender, eMultimedia, &dev);
    ok(hr == S_OK || hr == E_NOTFOUND, "GetDefaultAudioEndpoint failed: 0x%08x\n", hr);
    if (hr != S_OK || !dev)
    {
        if (hr == E_NOTFOUND)
            skip("No sound card available\n");
        else
            skip("GetDefaultAudioEndpoint returns 0x%08x\n", hr);
        goto cleanup;
    }

    test_audioclient();
    test_formats(AUDCLNT_SHAREMODE_EXCLUSIVE);
    test_formats(AUDCLNT_SHAREMODE_SHARED);
    test_formats2();
    test_references();
    test_marshal();
    trace("Output to a MS-DOS console is particularly slow and disturbs timing.\n");
    trace("Please redirect output to a file.\n");
    test_event();
    test_padding();
    test_clock(1);
    test_clock(0);
    test_session();
    test_streamvolume();
    test_channelvolume();
    test_simplevolume();
    test_volume_dependence();
    test_session_creation();
    test_worst_case();

    IMMDevice_Release(dev);

cleanup:
    if (mme)
        IMMDeviceEnumerator_Release(mme);
    CoUninitialize();
}
