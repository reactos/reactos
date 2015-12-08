/*
 * Copyright 2010 Maarten Lankhorst for CodeWeavers
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

/* This test is for audio capture specific mechanisms
 * Tests:
 * - IAudioClient with eCapture and IAudioCaptureClient
 */

#include <math.h>

#include "wine/test.h"

#define COBJMACROS

#ifdef STANDALONE
#include "initguid.h"
#endif

#include "unknwn.h"
#include "uuids.h"
#include "mmdeviceapi.h"
#include "audioclient.h"

#define NULL_PTR_ERR MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, RPC_X_NULL_REF_POINTER)

/* undocumented error code */
#define D3D11_ERROR_4E MAKE_HRESULT(SEVERITY_ERROR, FACILITY_DIRECT3D11, 0x4e)

static IMMDevice *dev = NULL;
static const LARGE_INTEGER ullZero;

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

static void test_capture(IAudioClient *ac, HANDLE handle, WAVEFORMATEX *wfx)
{
    IAudioCaptureClient *acc;
    HRESULT hr;
    UINT32 frames, next, pad, sum = 0;
    BYTE *data;
    DWORD flags;
    UINT64 pos, qpc;
    REFERENCE_TIME period;

    hr = IAudioClient_GetService(ac, &IID_IAudioCaptureClient, (void**)&acc);
    ok(hr == S_OK, "IAudioClient_GetService(IID_IAudioCaptureClient) returns %08x\n", hr);
    if (hr != S_OK)
        return;

    frames = 0xabadcafe;
    data = (void*)0xdeadf00d;
    flags = 0xabadcafe;
    pos = qpc = 0xdeadbeef;
    hr = IAudioCaptureClient_GetBuffer(acc, &data, &frames, &flags, &pos, &qpc);
    ok(hr == AUDCLNT_S_BUFFER_EMPTY, "Initial IAudioCaptureClient_GetBuffer returns %08x\n", hr);

    /* should be empty right after start. Otherwise consume one packet */
    if(hr == S_OK){
        hr = IAudioCaptureClient_ReleaseBuffer(acc, frames);
        ok(hr == S_OK, "Releasing buffer returns %08x\n", hr);
        sum += frames;

        frames = 0xabadcafe;
        data = (void*)0xdeadf00d;
        flags = 0xabadcafe;
        pos = qpc = 0xdeadbeef;
        hr = IAudioCaptureClient_GetBuffer(acc, &data, &frames, &flags, &pos, &qpc);
        ok(hr == AUDCLNT_S_BUFFER_EMPTY, "Initial IAudioCaptureClient_GetBuffer returns %08x\n", hr);
    }

    if(hr == AUDCLNT_S_BUFFER_EMPTY){
        ok(!frames, "frames changed to %u\n", frames);
        ok(data == (void*)0xdeadf00d, "data changed to %p\n", data);
        ok(flags == 0xabadcafe, "flags changed to %x\n", flags);
        ok(pos == 0xdeadbeef, "position changed to %u\n", (UINT)pos);
        ok(qpc == 0xdeadbeef, "timer changed to %u\n", (UINT)qpc);

        /* GetNextPacketSize yields 0 if no data is yet available
         * it is not constantly period_size * SamplesPerSec */
        hr = IAudioCaptureClient_GetNextPacketSize(acc, &next);
        ok(hr == S_OK, "IAudioCaptureClient_GetNextPacketSize returns %08x\n", hr);
        ok(!next, "GetNextPacketSize %u\n", next);
    }

    hr = IAudioCaptureClient_ReleaseBuffer(acc, frames);
    ok(hr == S_OK, "Releasing buffer returns %08x\n", hr);
    sum += frames;

    ok(ResetEvent(handle), "ResetEvent\n");

    hr = IAudioCaptureClient_GetNextPacketSize(acc, &next);
    ok(hr == S_OK, "IAudioCaptureClient_GetNextPacketSize returns %08x\n", hr);

    hr = IAudioClient_GetCurrentPadding(ac, &pad);
    ok(hr == S_OK, "GetCurrentPadding call returns %08x\n", hr);
    ok(next == pad, "GetNextPacketSize %u vs. GCP %u\n", next, pad);
    /* later GCP will grow, while GNPS is 0 or period size */

    hr = IAudioCaptureClient_GetNextPacketSize(acc, NULL);
    ok(hr == E_POINTER, "IAudioCaptureClient_GetNextPacketSize(NULL) returns %08x\n", hr);

    data = (void*)0xdeadf00d;
    frames = 0xdeadbeef;
    flags = 0xabadcafe;
    hr = IAudioCaptureClient_GetBuffer(acc, &data, NULL, NULL, NULL, NULL);
    ok(hr == E_POINTER, "IAudioCaptureClient_GetBuffer(data, NULL, NULL) returns %08x\n", hr);

    hr = IAudioCaptureClient_GetBuffer(acc, NULL, &frames, NULL, NULL, NULL);
    ok(hr == E_POINTER, "IAudioCaptureClient_GetBuffer(NULL, &frames, NULL) returns %08x\n", hr);

    hr = IAudioCaptureClient_GetBuffer(acc, NULL, NULL, &flags, NULL, NULL);
    ok(hr == E_POINTER, "IAudioCaptureClient_GetBuffer(NULL, NULL, &flags) returns %08x\n", hr);

    hr = IAudioCaptureClient_GetBuffer(acc, &data, &frames, NULL, NULL, NULL);
    ok(hr == E_POINTER, "IAudioCaptureClient_GetBuffer(&ata, &frames, NULL) returns %08x\n", hr);
    ok((DWORD_PTR)data == 0xdeadf00d, "data is reset to %p\n", data);
    ok(frames == 0xdeadbeef, "frames is reset to %08x\n", frames);
    ok(flags == 0xabadcafe, "flags is reset to %08x\n", flags);

    hr = IAudioClient_GetDevicePeriod(ac, &period, NULL);
    ok(hr == S_OK, "GetDevicePeriod failed: %08x\n", hr);
    period = MulDiv(period, wfx->nSamplesPerSec, 10000000); /* as in render.c */

    ok(WaitForSingleObject(handle, 1000) == WAIT_OBJECT_0, "Waiting on event handle failed!\n");

    data = (void*)0xdeadf00d;
    hr = IAudioCaptureClient_GetBuffer(acc, &data, &frames, &flags, &pos, &qpc);
    ok(hr == S_OK || hr == AUDCLNT_S_BUFFER_EMPTY, "Valid IAudioCaptureClient_GetBuffer returns %08x\n", hr);
    if (hr == S_OK){
        ok(frames, "Amount of frames locked is 0!\n");
        /* broken: some w7 machines return pad == 0 and DATA_DISCONTINUITY here,
         * AUDCLNT_S_BUFFER_EMPTY above, yet pos == 1-2 * period rather than 0 */
        ok(pos == sum || broken(pos == period || pos == 2*period),
           "Position %u expected %u\n", (UINT)pos, sum);
        sum = pos;
    }else if (hr == AUDCLNT_S_BUFFER_EMPTY){
        ok(!frames, "Amount of frames locked with empty buffer is %u!\n", frames);
        ok(data == (void*)0xdeadf00d, "No data changed to %p\n", data);
    }

    trace("Wait'ed position %d pad %u flags %x, amount of frames locked: %u\n",
          hr==S_OK ? (UINT)pos : -1, pad, flags, frames);

    hr = IAudioCaptureClient_GetNextPacketSize(acc, &next);
    ok(hr == S_OK, "IAudioCaptureClient_GetNextPacketSize returns %08x\n", hr);
    ok(next == frames, "GetNextPacketSize %u vs. GetBuffer %u\n", next, frames);

    hr = IAudioCaptureClient_ReleaseBuffer(acc, frames);
    ok(hr == S_OK, "Releasing buffer returns %08x\n", hr);

    hr = IAudioCaptureClient_ReleaseBuffer(acc, 0);
    ok(hr == S_OK, "Releasing 0 returns %08x\n", hr);

    hr = IAudioCaptureClient_GetNextPacketSize(acc, &next);
    ok(hr == S_OK, "IAudioCaptureClient_GetNextPacketSize returns %08x\n", hr);

    if (frames) {
        hr = IAudioCaptureClient_ReleaseBuffer(acc, frames);
        ok(hr == AUDCLNT_E_OUT_OF_ORDER, "Releasing buffer twice returns %08x\n", hr);
        sum += frames;
    }

    Sleep(350); /* for sure there's data now */

    hr = IAudioClient_GetCurrentPadding(ac, &pad);
    ok(hr == S_OK, "GetCurrentPadding call returns %08x\n", hr);

    /** GetNextPacketSize
     * returns either 0 or one period worth of frames
     * whereas GetCurrentPadding grows when input is not consumed. */
    hr = IAudioCaptureClient_GetNextPacketSize(acc, &next);
    ok(hr == S_OK, "IAudioCaptureClient_GetNextPacketSize returns %08x\n", hr);
    ok(next <  pad, "GetNextPacketSize %u vs. GCP %u\n", next, pad);

    hr = IAudioCaptureClient_GetBuffer(acc, &data, &frames, &flags, &pos, &qpc);
    ok(hr == S_OK, "Valid IAudioCaptureClient_GetBuffer returns %08x\n", hr);
    ok(next == frames, "GetNextPacketSize %u vs. GetBuffer %u\n", next, frames);

    if(hr == S_OK){
        UINT32 frames2 = frames;
        UINT64 pos2, qpc2;
        ok(frames, "Amount of frames locked is 0!\n");
        ok(pos == sum, "Position %u expected %u\n", (UINT)pos, sum);

        hr = IAudioCaptureClient_ReleaseBuffer(acc, 0);
        ok(hr == S_OK, "Releasing 0 returns %08x\n", hr);

        /* GCP did not decrement, no data consumed */
        hr = IAudioClient_GetCurrentPadding(ac, &frames);
        ok(hr == S_OK, "GetCurrentPadding call returns %08x\n", hr);
        ok(frames == pad || frames == pad + next /* concurrent feeder */,
           "GCP %u past ReleaseBuffer(0) initially %u\n", frames, pad);

        /* should re-get the same data */
        hr = IAudioCaptureClient_GetBuffer(acc, &data, &frames, &flags, &pos2, &qpc2);
        ok(hr == S_OK, "Valid IAudioCaptureClient_GetBuffer returns %08x\n", hr);
        ok(frames2 == frames, "GetBuffer after ReleaseBuffer(0) %u/%u\n", frames2, frames);
        ok(pos2 == pos, "Position after ReleaseBuffer(0) %u/%u\n", (UINT)pos2, (UINT)pos);
        if(qpc2 != qpc)
            /* FIXME: Some drivers fail */
            todo_wine ok(qpc2 == qpc, "HPC after ReleaseBuffer(0) %u vs. %u\n", (UINT)qpc2, (UINT)qpc);
        else
            ok(qpc2 == qpc, "HPC after ReleaseBuffer(0) %u vs. %u\n", (UINT)qpc2, (UINT)qpc);
    }

    /* trace after the GCP test because log output to MS-DOS console disturbs timing */
    trace("Sleep.1 position %d pad %u flags %x, amount of frames locked: %u\n",
          hr==S_OK ? (UINT)pos : -1, pad, flags, frames);

    if(hr == S_OK){
        UINT32 frames2 = 0xabadcafe;
        BYTE *data2 = (void*)0xdeadf00d;
        flags = 0xabadcafe;

        ok(pos == sum, "Position %u expected %u\n", (UINT)pos, sum);

        pos = qpc = 0xdeadbeef;
        hr = IAudioCaptureClient_GetBuffer(acc, &data2, &frames2, &flags, &pos, &qpc);
        ok(hr == AUDCLNT_E_OUT_OF_ORDER, "Out of order IAudioCaptureClient_GetBuffer returns %08x\n", hr);
        ok(frames2 == 0xabadcafe, "Out of order frames changed to %x\n", frames2);
        ok(data2 == (void*)0xdeadf00d, "Out of order data changed to %p\n", data2);
        ok(flags == 0xabadcafe, "Out of order flags changed to %x\n", flags);
        ok(pos == 0xdeadbeef, "Out of order position changed to %x\n", (UINT)pos);
        ok(qpc == 0xdeadbeef, "Out of order timer changed to %x\n", (UINT)qpc);

        hr = IAudioCaptureClient_ReleaseBuffer(acc, frames+1);
        ok(hr == AUDCLNT_E_INVALID_SIZE, "Releasing buffer+1 returns %08x\n", hr);

        hr = IAudioCaptureClient_ReleaseBuffer(acc, 1);
        ok(hr == AUDCLNT_E_INVALID_SIZE, "Releasing 1 returns %08x\n", hr);

        hr = IAudioClient_Reset(ac);
        ok(hr == AUDCLNT_E_NOT_STOPPED, "Reset failed: %08x\n", hr);
    }

    hr = IAudioCaptureClient_ReleaseBuffer(acc, frames);
    ok(hr == S_OK, "Releasing buffer returns %08x\n", hr);

    if (frames) {
        sum += frames;
        hr = IAudioCaptureClient_ReleaseBuffer(acc, frames);
        ok(hr == AUDCLNT_E_OUT_OF_ORDER, "Releasing buffer twice returns %08x\n", hr);
    }

    frames = period;
    ok(next == frames, "GetNextPacketSize %u vs. GetDevicePeriod %u\n", next, frames);

    /* GetBufferSize is not a multiple of the period size! */
    hr = IAudioClient_GetBufferSize(ac, &next);
    ok(hr == S_OK, "GetBufferSize failed: %08x\n", hr);
    trace("GetBufferSize %u period size %u\n", next, frames);

    Sleep(400); /* overrun */

    hr = IAudioClient_GetCurrentPadding(ac, &pad);
    ok(hr == S_OK, "GetCurrentPadding call returns %08x\n", hr);

    hr = IAudioCaptureClient_GetBuffer(acc, &data, &frames, &flags, &pos, &qpc);
    ok(hr == S_OK, "Valid IAudioCaptureClient_GetBuffer returns %08x\n", hr);

    trace("Overrun position %d pad %u flags %x, amount of frames locked: %u\n",
          hr==S_OK ? (UINT)pos : -1, pad, flags, frames);

    if(hr == S_OK){
        /* The discontinuity is reported here, but is this an old or new packet? */
        if(!(flags & AUDCLNT_BUFFERFLAGS_DATA_DISCONTINUITY)){
            /* FIXME: Some drivers fail */
            todo_wine ok(flags & AUDCLNT_BUFFERFLAGS_DATA_DISCONTINUITY, "expect DISCONTINUITY %x\n", flags);
            todo_wine ok(pos == sum + frames, "Position %u gap %d\n",
                         (UINT)pos, (UINT)pos - sum);
        }else{
            ok(flags & AUDCLNT_BUFFERFLAGS_DATA_DISCONTINUITY, "expect DISCONTINUITY %x\n", flags);

            /* Native's position is one period further than what we read.
             * Perhaps that's precisely the meaning of DATA_DISCONTINUITY:
             * signal when the position jump left a gap. */
            ok(pos == sum + frames, "Position %u gap %d\n",
                         (UINT)pos, (UINT)pos - sum);
        }

        ok(pad == next, "GCP %u vs. BufferSize %u\n", (UINT32)pad, next);

        if(flags & AUDCLNT_BUFFERFLAGS_DATA_DISCONTINUITY)
            sum = pos;
    }

    hr = IAudioCaptureClient_ReleaseBuffer(acc, frames);
    ok(hr == S_OK, "Releasing buffer returns %08x\n", hr);
    sum += frames;

    hr = IAudioClient_GetCurrentPadding(ac, &pad);
    ok(hr == S_OK, "GetCurrentPadding call returns %08x\n", hr);

    hr = IAudioCaptureClient_GetBuffer(acc, &data, &frames, &flags, &pos, &qpc);
    ok(hr == S_OK, "Valid IAudioCaptureClient_GetBuffer returns %08x\n", hr);

    trace("Cont'ed position %d pad %u flags %x, amount of frames locked: %u\n",
          hr==S_OK ? (UINT)pos : -1, pad, flags, frames);

    if(hr == S_OK){
        ok(pos == sum, "Position %u expected %u\n", (UINT)pos, sum);
        ok(!flags, "flags %u\n", flags);

        hr = IAudioCaptureClient_ReleaseBuffer(acc, frames);
        ok(hr == S_OK, "Releasing buffer returns %08x\n", hr);
        sum += frames;
    }

    hr = IAudioClient_Stop(ac);
    ok(hr == S_OK, "Stop on a started stream returns %08x\n", hr);

    hr = IAudioClient_Start(ac);
    ok(hr == S_OK, "Start on a stopped stream returns %08x\n", hr);

    hr = IAudioCaptureClient_GetBuffer(acc, &data, &frames, &flags, &pos, &qpc);
    ok(hr == S_OK, "Valid IAudioCaptureClient_GetBuffer returns %08x\n", hr);

    hr = IAudioClient_GetCurrentPadding(ac, &pad);
    ok(hr == S_OK, "GetCurrentPadding call returns %08x\n", hr);

    trace("Restart position %d pad %u flags %x, amount of frames locked: %u\n",
          hr==S_OK ? (UINT)pos : -1, pad, flags, frames);
    ok(pad > sum, "restarted GCP %u\n", pad); /* GCP is still near buffer size */

    if(frames){
        ok(pos == sum, "Position %u expected %u\n", (UINT)pos, sum);
        ok(!flags, "flags %u\n", flags);

        hr = IAudioCaptureClient_ReleaseBuffer(acc, frames);
        ok(hr == S_OK, "Releasing buffer returns %08x\n", hr);
        sum += frames;
    }

    hr = IAudioClient_Stop(ac);
    ok(hr == S_OK, "Stop on a started stream returns %08x\n", hr);

    hr = IAudioClient_Reset(ac);
    ok(hr == S_OK, "Reset on a stopped stream returns %08x\n", hr);
    sum += pad - frames;

    hr = IAudioClient_Start(ac);
    ok(hr == S_OK, "Start on a stopped stream returns %08x\n", hr);

    hr = IAudioClient_GetCurrentPadding(ac, &pad);
    ok(hr == S_OK, "GetCurrentPadding call returns %08x\n", hr);

    flags = 0xabadcafe;
    hr = IAudioCaptureClient_GetBuffer(acc, &data, &frames, &flags, &pos, &qpc);
    ok(hr == AUDCLNT_S_BUFFER_EMPTY || /*PulseAudio*/hr == S_OK,
       "Initial IAudioCaptureClient_GetBuffer returns %08x\n", hr);

    trace("Reset   position %d pad %u flags %x, amount of frames locked: %u\n",
          hr==S_OK ? (UINT)pos : -1, pad, flags, frames);

    if(hr == S_OK){
        /* Only PulseAudio goes here; despite snd_pcm_drop it manages
         * to fill GetBufferSize with a single snd_pcm_read */
        trace("Test marked todo: only PulseAudio gets here\n");
        todo_wine ok(flags & AUDCLNT_BUFFERFLAGS_DATA_DISCONTINUITY, "expect DISCONTINUITY %x\n", flags);
        /* Reset zeroes padding, not the position */
        ok(pos >= sum, "Position %u last %u\n", (UINT)pos, sum);
        /*sum = pos; check after next GetBuffer */

        hr = IAudioCaptureClient_ReleaseBuffer(acc, frames);
        ok(hr == S_OK, "Releasing buffer returns %08x\n", hr);
        sum += frames;
    }
    else if(hr == AUDCLNT_S_BUFFER_EMPTY){
        ok(!pad, "resetted GCP %u\n", pad);
        Sleep(180);
    }

    hr = IAudioClient_GetCurrentPadding(ac, &pad);
    ok(hr == S_OK, "GetCurrentPadding call returns %08x\n", hr);

    hr = IAudioCaptureClient_GetBuffer(acc, &data, &frames, &flags, &pos, &qpc);
    ok(hr == S_OK, "Valid IAudioCaptureClient_GetBuffer returns %08x\n", hr);
    trace("Running position %d pad %u flags %x, amount of frames locked: %u\n",
          hr==S_OK ? (UINT)pos : -1, pad, flags, frames);

    if(hr == S_OK){
        /* Some w7 machines signal DATA_DISCONTINUITY here following the
         * previous AUDCLNT_S_BUFFER_EMPTY, others not.  What logic? */
        ok(pos >= sum, "Position %u gap %d\n", (UINT)pos, (UINT)pos - sum);
        IAudioCaptureClient_ReleaseBuffer(acc, frames);
    }

    IAudioCaptureClient_Release(acc);
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
        ok(hr == S_OK || hr == AUDCLNT_E_UNSUPPORTED_FORMAT, "IsFormatSupported(Exclusive) call returns %08x\n", hr);

        hr = IAudioClient_IsFormatSupported(ac, AUDCLNT_SHAREMODE_EXCLUSIVE, pwfx, &pwfx2);
        ok(hr == S_OK || hr == AUDCLNT_E_UNSUPPORTED_FORMAT, "IsFormatSupported(Exclusive) call returns %08x\n", hr);
        ok(pwfx2 == NULL, "pwfx2 non-null on exclusive IsFormatSupported\n");

        hr = IAudioClient_IsFormatSupported(ac, 0xffffffff, pwfx, NULL);
        ok(hr == E_INVALIDARG/*w32*/ ||
           broken(hr == AUDCLNT_E_UNSUPPORTED_FORMAT/*w64 response from exclusive mode driver */),
           "IsFormatSupported(0xffffffff) call returns %08x\n", hr);
    }

    test_uninitialized(ac);

    hr = IAudioClient_Initialize(ac, 3, 0, 5000000, 0, pwfx, NULL);
    ok(hr == AUDCLNT_E_NOT_INITIALIZED, "Initialize with invalid sharemode returns %08x\n", hr);

    hr = IAudioClient_Initialize(ac, AUDCLNT_SHAREMODE_SHARED, 0xffffffff, 5000000, 0, pwfx, NULL);
    ok(hr == E_INVALIDARG || hr == AUDCLNT_E_INVALID_STREAM_FLAG, "Initialize with invalid flags returns %08x\n", hr);

    /* A period != 0 is ignored and the call succeeds.
     * Since we can only initialize successfully once, skip those tests.
     */
    hr = IAudioClient_Initialize(ac, AUDCLNT_SHAREMODE_SHARED, 0, 5000000, 0, NULL, NULL);
    ok(hr == E_POINTER, "Initialize with null format returns %08x\n", hr);

    hr = IAudioClient_Initialize(ac, AUDCLNT_SHAREMODE_SHARED, AUDCLNT_STREAMFLAGS_EVENTCALLBACK, 4987654, 0, pwfx, NULL);
    ok(hr == S_OK, "Valid Initialize returns %08x\n", hr);

    if (hr != S_OK)
    {
        skip("Cannot initialize %08x, remainder of tests is useless\n", hr);
        CoTaskMemFree(pwfx);
        return;
    }

    hr = IAudioClient_GetStreamLatency(ac, NULL);
    ok(hr == E_POINTER, "GetStreamLatency(NULL) call returns %08x\n", hr);

    hr = IAudioClient_GetStreamLatency(ac, &t1);
    ok(hr == S_OK, "Valid GetStreamLatency call returns %08x\n", hr);
    trace("Returned latency: %u.%04u ms\n",
          (UINT)(t1/10000), (UINT)(t1 % 10000));

    hr = IAudioClient_Initialize(ac, AUDCLNT_SHAREMODE_SHARED, 0, 5000000, 0, pwfx, NULL);
    ok(hr == AUDCLNT_E_ALREADY_INITIALIZED, "Calling Initialize twice returns %08x\n", hr);

    hr = IAudioClient_SetEventHandle(ac, NULL);
    ok(hr == E_INVALIDARG, "SetEventHandle(NULL) returns %08x\n", hr);

    hr = IAudioClient_Start(ac);
    ok(hr == AUDCLNT_E_EVENTHANDLE_NOT_SET ||
            hr == D3D11_ERROR_4E /* win10 */, "Start before SetEventHandle returns %08x\n", hr);

    hr = IAudioClient_SetEventHandle(ac, handle);
    ok(hr == S_OK, "SetEventHandle returns %08x\n", hr);

    hr = IAudioClient_Reset(ac);
    ok(hr == S_OK, "Reset on a resetted stream returns %08x\n", hr);

    hr = IAudioClient_Stop(ac);
    ok(hr == S_FALSE, "Stop on a stopped stream returns %08x\n", hr);

    hr = IAudioClient_Start(ac);
    ok(hr == S_OK, "Start on a stopped stream returns %08x\n", hr);

    test_capture(ac, handle, pwfx);

    IAudioClient_Release(ac);
    CloseHandle(handle);
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
    if(hr != S_OK)
        return;

    hr = IAudioClient_GetService(ac, &IID_IAudioStreamVolume, (void**)&asv);
    ok(hr == S_OK, "GetService failed: %08x\n", hr);

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

    hr = IAudioClient_GetService(ac, &IID_IChannelAudioVolume, (void**)&acv);
    ok(hr == S_OK, "GetService failed: %08x\n", hr);
    if(hr != S_OK)
        return;

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

    hr = IAudioClient_GetService(ac, &IID_ISimpleAudioVolume, (void**)&sav);
    ok(hr == S_OK, "GetService failed: %08x\n", hr);
    if(hr != S_OK)
        return;

    hr = ISimpleAudioVolume_GetMasterVolume(sav, NULL);
    ok(hr == NULL_PTR_ERR, "GetMasterVolume gave wrong error: %08x\n", hr);

    hr = ISimpleAudioVolume_GetMasterVolume(sav, &vol);
    ok(hr == S_OK, "GetMasterVolume failed: %08x\n", hr);

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

    hr = IAudioClient_GetService(ac, &IID_ISimpleAudioVolume, (void**)&sav);
    ok(hr == S_OK, "GetService (SimpleAudioVolume) failed: %08x\n", hr);

    hr = IAudioClient_GetService(ac, &IID_IChannelAudioVolume, (void**)&cav);
    ok(hr == S_OK, "GetService (ChannelAudioVolme) failed: %08x\n", hr);

    hr = IAudioClient_GetService(ac, &IID_IAudioStreamVolume, (void**)&asv);
    ok(hr == S_OK, "GetService (AudioStreamVolume) failed: %08x\n", hr);
    if(hr != S_OK)
        return;

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
    if(SUCCEEDED(hr)){
        IChannelAudioVolume *cav2;
        IAudioStreamVolume *asv2;

        hr = IAudioClient_Initialize(ac2, AUDCLNT_SHAREMODE_SHARED,
                AUDCLNT_STREAMFLAGS_NOPERSIST, 5000000, 0, fmt, &session);
        ok(hr == S_OK, "Initialize failed: %08x\n", hr);

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

static void test_marshal(void)
{
    IStream *pStream;
    IAudioClient *ac, *acDest;
    IAudioCaptureClient *cc, *ccDest;
    WAVEFORMATEX *pwfx;
    HRESULT hr;

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

    hr = IAudioClient_GetService(ac, &IID_IAudioCaptureClient, (void**)&cc);
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
    /* marshal IAudioCaptureClient */

    hr = CoMarshalInterface(pStream, &IID_IAudioCaptureClient, (IUnknown*)cc, MSHCTX_INPROC, NULL, MSHLFLAGS_NORMAL);
    ok(hr == S_OK, "CoMarshalInterface IAudioCaptureClient failed 0x%08x\n", hr);

    IStream_Seek(pStream, ullZero, STREAM_SEEK_SET, NULL);
    hr = CoUnmarshalInterface(pStream, &IID_IAudioCaptureClient, (void **)&ccDest);
    ok(hr == S_OK, "CoUnmarshalInterface IAudioCaptureClient failed 0x%08x\n", hr);
    if (hr == S_OK)
        IAudioCaptureClient_Release(ccDest);

    IStream_Release(pStream);

    IAudioClient_Release(ac);
    IAudioCaptureClient_Release(cc);

}

START_TEST(capture)
{
    HRESULT hr;
    IMMDeviceEnumerator *mme = NULL;

    CoInitializeEx(NULL, COINIT_MULTITHREADED);
    hr = CoCreateInstance(&CLSID_MMDeviceEnumerator, NULL, CLSCTX_INPROC_SERVER, &IID_IMMDeviceEnumerator, (void**)&mme);
    if (FAILED(hr))
    {
        skip("mmdevapi not available: 0x%08x\n", hr);
        goto cleanup;
    }

    hr = IMMDeviceEnumerator_GetDefaultAudioEndpoint(mme, eCapture, eMultimedia, &dev);
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
    test_streamvolume();
    test_channelvolume();
    test_simplevolume();
    test_volume_dependence();
    test_marshal();

    IMMDevice_Release(dev);

cleanup:
    if (mme)
        IMMDeviceEnumerator_Release(mme);
    CoUninitialize();
}
