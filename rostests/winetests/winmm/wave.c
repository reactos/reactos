/*
 * Test winmm sound playback in each sound format
 *
 * Copyright (c) 2002 Francois Gouget
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
#include <math.h>

#include "wine/test.h"
#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "winnls.h"
#include "mmsystem.h"
#define NOBITMAP
#include "mmddk.h"
#include "mmreg.h"
//#include "ks.h"
//#include "ksguid.h"
//#include "ksmedia.h"

#include "winmm_test.h"

/* FIXME */
DEFINE_GUID(KSDATAFORMAT_SUBTYPE_PCM, 0x00000001, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71);
DEFINE_GUID(KSDATAFORMAT_SUBTYPE_IEEE_FLOAT, 0x00000003, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71);

static DWORD g_tid;

static void test_multiple_waveopens(void)
{
    HWAVEOUT handle1, handle2;
    MMRESULT ret;
    WAVEFORMATEX wfx;

    wfx.wFormatTag = WAVE_FORMAT_PCM;
    wfx.nChannels = 1;
    wfx.nSamplesPerSec = 11025;
    wfx.nBlockAlign = 1;
    wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nBlockAlign;
    wfx.wBitsPerSample = 8;
    wfx.cbSize = 0;

    ret = waveOutOpen(&handle1, 0, &wfx, 0, 0, 0);
    if (ret != MMSYSERR_NOERROR)
    {
        skip("Could not do the duplicate waveopen test\n");
        return;
    }

    ret = waveOutOpen(&handle2, 0, &wfx, 0, 0, 0);
    /* Modern Windows allows for wave-out devices to be opened multiple times.
     * Some Wine audio drivers allow that and some don't.  To avoid false alarms
     * for those that do, don't "todo_wine ok(...)" on success.
     */
    if (ret != MMSYSERR_NOERROR)
    {
        todo_wine ok(ret == MMSYSERR_NOERROR || broken(ret == MMSYSERR_ALLOCATED), /* winME */
                     "second waveOutOpen returns: %x\n", ret);
    }
    else
        waveOutClose(handle2);

    waveOutClose(handle1);
}

/*
 * Note that in most of this test we may get MMSYSERR_BADDEVICEID errors
 * at about any time if the user starts another application that uses the
 * sound device. So we should not report these as test failures.
 *
 * This test can play a test tone. But this only makes sense if someone
 * is going to carefully listen to it, and would only bother everyone else.
 * So this is only done if the test is being run in interactive mode.
 */

#define PI 3.14159265358979323846
static char* wave_generate_la(WAVEFORMATEX* wfx, double duration, DWORD* size)
{
    int i,j;
    int nb_samples;
    char* buf;
    char* b;
    WAVEFORMATEXTENSIBLE *wfex = (WAVEFORMATEXTENSIBLE*)wfx;

    nb_samples=(int)(duration*wfx->nSamplesPerSec);
    *size=nb_samples*wfx->nBlockAlign;
    b=buf=HeapAlloc(GetProcessHeap(), 0, *size);
    for (i=0;i<nb_samples;i++) {
        double y=sin(440.0*2*PI*i/wfx->nSamplesPerSec);
        if (wfx->wBitsPerSample==8) {
            unsigned char sample=(unsigned char)((double)127.5*(y+1.0));
            for (j = 0; j < wfx->nChannels; j++)
                *b++=sample;
        } else if (wfx->wBitsPerSample==16) {
            signed short sample=(signed short)((double)32767.5*y-0.5);
            for (j = 0; j < wfx->nChannels; j++) {
                b[0]=sample & 0xff;
                b[1]=sample >> 8;
                b+=2;
            }
        } else if (wfx->wBitsPerSample==24) {
            signed int sample=(signed int)(((double)0x7fffff+0.5)*y-0.5);
            for (j = 0; j < wfx->nChannels; j++) {
                b[0]=sample & 0xff;
                b[1]=(sample >> 8) & 0xff;
                b[2]=(sample >> 16) & 0xff;
                b+=3;
            }
        } else if ((wfx->wBitsPerSample==32) && ((wfx->wFormatTag == WAVE_FORMAT_PCM) ||
            ((wfx->wFormatTag == WAVE_FORMAT_EXTENSIBLE) &&
            IsEqualGUID(&wfex->SubFormat, &KSDATAFORMAT_SUBTYPE_PCM)))) {
            signed int sample=(signed int)(((double)0x7fffffff+0.5)*y-0.5);
            for (j = 0; j < wfx->nChannels; j++) {
                b[0]=sample & 0xff;
                b[1]=(sample >> 8) & 0xff;
                b[2]=(sample >> 16) & 0xff;
                b[3]=(sample >> 24) & 0xff;
                b+=4;
            }
        } else if ((wfx->wBitsPerSample==32) && (wfx->wFormatTag == WAVE_FORMAT_EXTENSIBLE) &&
            IsEqualGUID(&wfex->SubFormat, &KSDATAFORMAT_SUBTYPE_IEEE_FLOAT)) {
            union { float f; char c[4]; } sample;
            sample.f=(float)y;
            for (j = 0; j < wfx->nChannels; j++) {
                b[0]=sample.c[0];
                b[1]=sample.c[1];
                b[2]=sample.c[2];
                b[3]=sample.c[3];
                b+=4;
            }
        }
    }
    return buf;
}

static char* wave_generate_silence(WAVEFORMATEX* wfx, double duration, DWORD* size)
{
    int i,j;
    int nb_samples;
    char* buf;
    char* b;
    WAVEFORMATEXTENSIBLE *wfex = (WAVEFORMATEXTENSIBLE*)wfx;

    nb_samples=(int)(duration*wfx->nSamplesPerSec);
    *size=nb_samples*wfx->nBlockAlign;
    b=buf=HeapAlloc(GetProcessHeap(), 0, *size);
    for (i=0;i<nb_samples;i++) {
        if (wfx->wBitsPerSample==8) {
            for (j = 0; j < wfx->nChannels; j++)
                *b++=128;
        } else if (wfx->wBitsPerSample==16) {
            for (j = 0; j < wfx->nChannels; j++) {
                b[0]=0;
                b[1]=0;
                b+=2;
            }
        } else if (wfx->wBitsPerSample==24) {
            for (j = 0; j < wfx->nChannels; j++) {
                b[0]=0;
                b[1]=0;
                b[2]=0;
                b+=3;
            }
        } else if ((wfx->wBitsPerSample==32) && ((wfx->wFormatTag == WAVE_FORMAT_PCM) ||
            ((wfx->wFormatTag == WAVE_FORMAT_EXTENSIBLE) &&
            IsEqualGUID(&wfex->SubFormat, &KSDATAFORMAT_SUBTYPE_PCM)))) {
            for (j = 0; j < wfx->nChannels; j++) {
                b[0]=0;
                b[1]=0;
                b[2]=0;
                b[3]=0;
                b+=4;
            }
        } else if ((wfx->wBitsPerSample==32) && (wfx->wFormatTag == WAVE_FORMAT_EXTENSIBLE) &&
            IsEqualGUID(&wfex->SubFormat, &KSDATAFORMAT_SUBTYPE_IEEE_FLOAT)) {
            union { float f; char c[4]; } sample;
            sample.f=0;
            for (j = 0; j < wfx->nChannels; j++) {
                b[0]=sample.c[0];
                b[1]=sample.c[1];
                b[2]=sample.c[2];
                b[3]=sample.c[3];
                b+=4;
            }
        }
    }
    return buf;
}

const char * dev_name(int device)
{
    static char name[16];
    if (device == WAVE_MAPPER)
        return "WAVE_MAPPER";
    sprintf(name, "%d", device);
    return name;
}

const char* mmsys_error(MMRESULT error)
{
#define ERR_TO_STR(dev) case dev: return #dev
    static char	unknown[32];
    switch (error) {
    ERR_TO_STR(MMSYSERR_NOERROR);
    ERR_TO_STR(MMSYSERR_ERROR);
    ERR_TO_STR(MMSYSERR_BADDEVICEID);
    ERR_TO_STR(MMSYSERR_NOTENABLED);
    ERR_TO_STR(MMSYSERR_ALLOCATED);
    ERR_TO_STR(MMSYSERR_INVALHANDLE);
    ERR_TO_STR(MMSYSERR_NODRIVER);
    ERR_TO_STR(MMSYSERR_NOMEM);
    ERR_TO_STR(MMSYSERR_NOTSUPPORTED);
    ERR_TO_STR(MMSYSERR_BADERRNUM);
    ERR_TO_STR(MMSYSERR_INVALFLAG);
    ERR_TO_STR(MMSYSERR_INVALPARAM);
    ERR_TO_STR(WAVERR_BADFORMAT);
    ERR_TO_STR(WAVERR_STILLPLAYING);
    ERR_TO_STR(WAVERR_UNPREPARED);
    ERR_TO_STR(WAVERR_SYNC);
    ERR_TO_STR(MIDIERR_UNPREPARED);
    ERR_TO_STR(MIDIERR_STILLPLAYING);
    ERR_TO_STR(MIDIERR_NOTREADY);
    ERR_TO_STR(MIDIERR_NODEVICE);
    ERR_TO_STR(MIDIERR_INVALIDSETUP);
    ERR_TO_STR(TIMERR_NOCANDO);
    ERR_TO_STR(TIMERR_STRUCT);
    ERR_TO_STR(JOYERR_PARMS);
    ERR_TO_STR(JOYERR_NOCANDO);
    ERR_TO_STR(JOYERR_UNPLUGGED);
    ERR_TO_STR(MIXERR_INVALLINE);
    ERR_TO_STR(MIXERR_INVALCONTROL);
    ERR_TO_STR(MIXERR_INVALVALUE);
    ERR_TO_STR(MMIOERR_FILENOTFOUND);
    ERR_TO_STR(MMIOERR_OUTOFMEMORY);
    ERR_TO_STR(MMIOERR_CANNOTOPEN);
    ERR_TO_STR(MMIOERR_CANNOTCLOSE);
    ERR_TO_STR(MMIOERR_CANNOTREAD);
    ERR_TO_STR(MMIOERR_CANNOTWRITE);
    ERR_TO_STR(MMIOERR_CANNOTSEEK);
    ERR_TO_STR(MMIOERR_CANNOTEXPAND);
    ERR_TO_STR(MMIOERR_CHUNKNOTFOUND);
    ERR_TO_STR(MMIOERR_UNBUFFERED);
    }
    sprintf(unknown, "Unknown(0x%08x)", error);
    return unknown;
#undef ERR_TO_STR
}

const char* wave_out_error(MMRESULT error)
{
    static char msg[1024];
    static char long_msg[1100];
    MMRESULT rc;

    rc = waveOutGetErrorTextA(error, msg, sizeof(msg));
    if (rc != MMSYSERR_NOERROR)
        sprintf(long_msg, "waveOutGetErrorTextA(%x) failed with error %x", error, rc);
    else
        sprintf(long_msg, "%s(%s)", mmsys_error(error), msg);
    return long_msg;
}

const char * wave_open_flags(DWORD flags)
{
    static char msg[1024];
    BOOL first = TRUE;
    msg[0] = 0;
    if ((flags & CALLBACK_TYPEMASK) == CALLBACK_EVENT) {
        strcat(msg, "CALLBACK_EVENT");
        first = FALSE;
    }
    if ((flags & CALLBACK_TYPEMASK) == CALLBACK_FUNCTION) {
        if (!first) strcat(msg, "|");
        strcat(msg, "CALLBACK_FUNCTION");
        first = FALSE;
    }
    if ((flags & CALLBACK_TYPEMASK) == CALLBACK_NULL) {
        if (!first) strcat(msg, "|");
        strcat(msg, "CALLBACK_NULL");
        first = FALSE;
    }
    if ((flags & CALLBACK_TYPEMASK) == CALLBACK_THREAD) {
        if (!first) strcat(msg, "|");
        strcat(msg, "CALLBACK_THREAD");
        first = FALSE;
    }
    if ((flags & CALLBACK_TYPEMASK) == CALLBACK_WINDOW) {
        if (!first) strcat(msg, "|");
        strcat(msg, "CALLBACK_WINDOW");
        first = FALSE;
    }
    if ((flags & WAVE_ALLOWSYNC) == WAVE_ALLOWSYNC) {
        if (!first) strcat(msg, "|");
        strcat(msg, "WAVE_ALLOWSYNC");
        first = FALSE;
    }
    if ((flags & WAVE_FORMAT_DIRECT) == WAVE_FORMAT_DIRECT) {
        if (!first) strcat(msg, "|");
        strcat(msg, "WAVE_FORMAT_DIRECT");
        first = FALSE;
    }
    if ((flags & WAVE_FORMAT_QUERY) == WAVE_FORMAT_QUERY) {
        if (!first) strcat(msg, "|");
        strcat(msg, "WAVE_FORMAT_QUERY");
        first = FALSE;
    }
    if ((flags & WAVE_MAPPED) == WAVE_MAPPED) {
        if (!first) strcat(msg, "|");
        strcat(msg, "WAVE_MAPPED");
    }
    return msg;
}

static const char * wave_header_flags(DWORD flags)
{
#define WHDR_MASK (WHDR_BEGINLOOP|WHDR_DONE|WHDR_ENDLOOP|WHDR_INQUEUE|WHDR_PREPARED)
    static char msg[1024];
    BOOL first = TRUE;
    msg[0] = 0;
    if (flags & WHDR_BEGINLOOP) {
        strcat(msg, "WHDR_BEGINLOOP");
        first = FALSE;
    }
    if (flags & WHDR_DONE) {
        if (!first) strcat(msg, " ");
        strcat(msg, "WHDR_DONE");
        first = FALSE;
    }
    if (flags & WHDR_ENDLOOP) {
        if (!first) strcat(msg, " ");
        strcat(msg, "WHDR_ENDLOOP");
        first = FALSE;
    }
    if (flags & WHDR_INQUEUE) {
        if (!first) strcat(msg, " ");
        strcat(msg, "WHDR_INQUEUE");
        first = FALSE;
    }
    if (flags & WHDR_PREPARED) {
        if (!first) strcat(msg, " ");
        strcat(msg, "WHDR_PREPARED");
        first = FALSE;
    }
    if (flags & ~WHDR_MASK) {
        char temp[32];
        sprintf(temp, "UNKNOWN(0x%08x)", flags & ~WHDR_MASK);
        if (!first) strcat(msg, " ");
        strcat(msg, temp);
    }
    return msg;
}

static const char * wave_out_caps(DWORD dwSupport)
{
#define ADD_FLAG(f) if (dwSupport & f) strcat(msg, " " #f)
    static char msg[256];
    msg[0] = 0;

    ADD_FLAG(WAVECAPS_PITCH);
    ADD_FLAG(WAVECAPS_PLAYBACKRATE);
    ADD_FLAG(WAVECAPS_VOLUME);
    ADD_FLAG(WAVECAPS_LRVOLUME);
    ADD_FLAG(WAVECAPS_SYNC);
    ADD_FLAG(WAVECAPS_SAMPLEACCURATE);

    return msg[0] ? msg + 1 : "";
#undef ADD_FLAG
}

const char * wave_time_format(UINT type)
{
    static char msg[32];
#define TIME_FORMAT(f) case f: return #f
    switch (type) {
    TIME_FORMAT(TIME_MS);
    TIME_FORMAT(TIME_SAMPLES);
    TIME_FORMAT(TIME_BYTES);
    TIME_FORMAT(TIME_SMPTE);
    TIME_FORMAT(TIME_MIDI);
    TIME_FORMAT(TIME_TICKS);
    }
#undef TIME_FORMAT
    sprintf(msg, "Unknown(0x%04x)", type);
    return msg;
}

const char * get_format_str(WORD format)
{
    static char msg[32];
#define WAVE_FORMAT(f) case f: return #f
    switch (format) {
    WAVE_FORMAT(WAVE_FORMAT_PCM);
    WAVE_FORMAT(WAVE_FORMAT_ADPCM);
    WAVE_FORMAT(WAVE_FORMAT_IBM_CVSD);
    WAVE_FORMAT(WAVE_FORMAT_ALAW);
    WAVE_FORMAT(WAVE_FORMAT_MULAW);
    WAVE_FORMAT(WAVE_FORMAT_OKI_ADPCM);
    WAVE_FORMAT(WAVE_FORMAT_IMA_ADPCM);
    WAVE_FORMAT(WAVE_FORMAT_MEDIASPACE_ADPCM);
    WAVE_FORMAT(WAVE_FORMAT_SIERRA_ADPCM);
    WAVE_FORMAT(WAVE_FORMAT_G723_ADPCM);
    WAVE_FORMAT(WAVE_FORMAT_DIGISTD);
    WAVE_FORMAT(WAVE_FORMAT_DIGIFIX);
    WAVE_FORMAT(WAVE_FORMAT_DIALOGIC_OKI_ADPCM);
    WAVE_FORMAT(WAVE_FORMAT_YAMAHA_ADPCM);
    WAVE_FORMAT(WAVE_FORMAT_SONARC);
    WAVE_FORMAT(WAVE_FORMAT_DSPGROUP_TRUESPEECH);
    WAVE_FORMAT(WAVE_FORMAT_ECHOSC1);
    WAVE_FORMAT(WAVE_FORMAT_AUDIOFILE_AF36);
    WAVE_FORMAT(WAVE_FORMAT_APTX);
    WAVE_FORMAT(WAVE_FORMAT_AUDIOFILE_AF10);
    WAVE_FORMAT(WAVE_FORMAT_DOLBY_AC2);
    WAVE_FORMAT(WAVE_FORMAT_GSM610);
    WAVE_FORMAT(WAVE_FORMAT_ANTEX_ADPCME);
    WAVE_FORMAT(WAVE_FORMAT_CONTROL_RES_VQLPC);
    WAVE_FORMAT(WAVE_FORMAT_DIGIREAL);
    WAVE_FORMAT(WAVE_FORMAT_DIGIADPCM);
    WAVE_FORMAT(WAVE_FORMAT_CONTROL_RES_CR10);
    WAVE_FORMAT(WAVE_FORMAT_NMS_VBXADPCM);
    WAVE_FORMAT(WAVE_FORMAT_G721_ADPCM);
    WAVE_FORMAT(WAVE_FORMAT_MPEG);
    WAVE_FORMAT(WAVE_FORMAT_MPEGLAYER3);
    WAVE_FORMAT(WAVE_FORMAT_CREATIVE_ADPCM);
    WAVE_FORMAT(WAVE_FORMAT_CREATIVE_FASTSPEECH8);
    WAVE_FORMAT(WAVE_FORMAT_CREATIVE_FASTSPEECH10);
    WAVE_FORMAT(WAVE_FORMAT_FM_TOWNS_SND);
    WAVE_FORMAT(WAVE_FORMAT_OLIGSM);
    WAVE_FORMAT(WAVE_FORMAT_OLIADPCM);
    WAVE_FORMAT(WAVE_FORMAT_OLICELP);
    WAVE_FORMAT(WAVE_FORMAT_OLISBC);
    WAVE_FORMAT(WAVE_FORMAT_OLIOPR);
    WAVE_FORMAT(WAVE_FORMAT_DEVELOPMENT);
    WAVE_FORMAT(WAVE_FORMAT_EXTENSIBLE);
    }
#undef WAVE_FORMAT
    sprintf(msg, "Unknown(0x%04x)", format);
    return msg;
}

DWORD bytes_to_samples(DWORD bytes, LPWAVEFORMATEX pwfx)
{
    return bytes / pwfx->nBlockAlign;
}

DWORD bytes_to_ms(DWORD bytes, LPWAVEFORMATEX pwfx)
{
    return bytes_to_samples(bytes, pwfx) * 1000 / pwfx->nSamplesPerSec;
}

DWORD time_to_bytes(LPMMTIME mmtime, LPWAVEFORMATEX pwfx)
{
    if (mmtime->wType == TIME_BYTES)
        return mmtime->u.cb;
    else if (mmtime->wType == TIME_SAMPLES)
        return mmtime->u.sample * pwfx->nBlockAlign;
    else if (mmtime->wType == TIME_MS)
        return mmtime->u.ms * pwfx->nAvgBytesPerSec / 1000;
    else if (mmtime->wType == TIME_SMPTE)
        return ((mmtime->u.smpte.hour * 60 * 60) +
                (mmtime->u.smpte.min * 60) +
                (mmtime->u.smpte.sec)) * pwfx->nAvgBytesPerSec +
                mmtime->u.smpte.frame  * pwfx->nAvgBytesPerSec / 30;

    trace("FIXME: time_to_bytes() type not supported\n");
    return -1;
}

static void check_position(int device, HWAVEOUT wout, DWORD bytes,
                           LPWAVEFORMATEX pwfx )
{
    MMTIME mmtime;
    MMRESULT rc;
    DWORD returned;

    mmtime.wType = TIME_BYTES;
    rc=waveOutGetPosition(wout, &mmtime, sizeof(mmtime) - 1);
    ok(rc==MMSYSERR_ERROR,
       "waveOutGetPosition(%s): rc=%s\n",dev_name(device),wave_out_error(rc));

    mmtime.wType = TIME_BYTES;
    rc=waveOutGetPosition(wout, &mmtime, sizeof(mmtime) + 1);
    ok(rc==MMSYSERR_NOERROR,
       "waveOutGetPosition(%s): rc=%s\n",dev_name(device),wave_out_error(rc));
    if (mmtime.wType != TIME_BYTES && winetest_debug > 1)
        trace("waveOutGetPosition(%s): TIME_BYTES not supported, returned %s\n",
              dev_name(device),wave_time_format(mmtime.wType));
    returned = time_to_bytes(&mmtime, pwfx);
    ok(returned == bytes, "waveOutGetPosition(%s): returned %d bytes, "
       "should be %d\n", dev_name(device), returned, bytes);

    mmtime.wType = TIME_SAMPLES;
    rc=waveOutGetPosition(wout, &mmtime, sizeof(mmtime));
    ok(rc==MMSYSERR_NOERROR,
       "waveOutGetPosition(%s): rc=%s\n",dev_name(device),wave_out_error(rc));
    if (mmtime.wType != TIME_SAMPLES && winetest_debug > 1)
        trace("waveOutGetPosition(%s): TIME_SAMPLES not supported, "
              "returned %s\n",dev_name(device),wave_time_format(mmtime.wType));
    returned = time_to_bytes(&mmtime, pwfx);
    ok(returned == bytes, "waveOutGetPosition(%s): returned %d samples "
       "(%d bytes), should be %d (%d bytes)\n", dev_name(device),
       bytes_to_samples(returned, pwfx), returned,
       bytes_to_samples(bytes, pwfx), bytes);

    mmtime.wType = TIME_MS;
    rc=waveOutGetPosition(wout, &mmtime, sizeof(mmtime));
    ok(rc==MMSYSERR_NOERROR,
       "waveOutGetPosition(%s): rc=%s\n",dev_name(device),wave_out_error(rc));
    if (mmtime.wType != TIME_MS && winetest_debug > 1)
        trace("waveOutGetPosition(%s): TIME_MS not supported, returned %s\n",
              dev_name(device), wave_time_format(mmtime.wType));
    returned = time_to_bytes(&mmtime, pwfx);
    ok(returned == bytes, "waveOutGetPosition(%s): returned %d ms, "
       "(%d bytes), should be %d (%d bytes)\n", dev_name(device),
       bytes_to_ms(returned, pwfx), returned,
       bytes_to_ms(bytes, pwfx), bytes);

    mmtime.wType = TIME_SMPTE;
    rc=waveOutGetPosition(wout, &mmtime, sizeof(mmtime));
    ok(rc==MMSYSERR_NOERROR,
       "waveOutGetPosition(%s): rc=%s\n",dev_name(device),wave_out_error(rc));
    if (mmtime.wType != TIME_SMPTE && winetest_debug > 1)
        trace("waveOutGetPosition(%s): TIME_SMPTE not supported, returned %s\n",
              dev_name(device),wave_time_format(mmtime.wType));
    returned = time_to_bytes(&mmtime, pwfx);
    ok(returned == bytes, "waveOutGetPosition(%s): SMPTE test failed\n",
       dev_name(device));

    mmtime.wType = TIME_MIDI;
    rc=waveOutGetPosition(wout, &mmtime, sizeof(mmtime));
    ok(rc==MMSYSERR_NOERROR,
       "waveOutGetPosition(%s): rc=%s\n",dev_name(device),wave_out_error(rc));
    if (mmtime.wType != TIME_MIDI && winetest_debug > 1)
        trace("waveOutGetPosition(%s): TIME_MIDI not supported, returned %s\n",
              dev_name(device),wave_time_format(mmtime.wType));
    returned = time_to_bytes(&mmtime, pwfx);
    ok(returned == bytes, "waveOutGetPosition(%s): MIDI test failed\n",
       dev_name(device));

    mmtime.wType = TIME_TICKS;
    rc=waveOutGetPosition(wout, &mmtime, sizeof(mmtime));
    ok(rc==MMSYSERR_NOERROR,
       "waveOutGetPosition(%s): rc=%s\n",dev_name(device),wave_out_error(rc));
    if (mmtime.wType != TIME_TICKS && winetest_debug > 1)
        trace("waveOutGetPosition(%s): TIME_TICKS not supported, returned %s\n",
              dev_name(device),wave_time_format(mmtime.wType));
    returned = time_to_bytes(&mmtime, pwfx);
    ok(returned == bytes, "waveOutGetPosition(%s): TICKS test failed\n",
       dev_name(device));
}

static void CALLBACK callback_func(HWAVEOUT hwo, UINT uMsg,
                                   DWORD_PTR dwInstance,
                                   DWORD dwParam1, DWORD dwParam2)
{
    if(uMsg == WOM_OPEN || uMsg == WOM_CLOSE)
        ok(GetCurrentThreadId() == g_tid, "Got different thread ID\n");
    SetEvent((HANDLE)dwInstance);
}

static DWORD WINAPI callback_thread(LPVOID lpParameter)
{
    MSG msg;

    PeekMessageW( &msg, 0, 0, 0, PM_NOREMOVE );  /* make sure the thread has a message queue */
    SetEvent(lpParameter);

    while (GetMessageA(&msg, 0, 0, 0)) {
        UINT message = msg.message;
        /* for some reason XP sends a WM_USER message before WOM_OPEN */
        ok (message == WOM_OPEN || message == WOM_DONE ||
            message == WOM_CLOSE || message == WM_USER || message == WM_APP,
            "GetMessageA returned unexpected message: %u\n", message);
        if (message == WOM_OPEN || message == WOM_DONE || message == WOM_CLOSE)
            SetEvent(lpParameter);
        else if (message == WM_APP) {
            SetEvent(lpParameter);
            return 0;
        }
    }

    return 0;
}

static void wave_out_test_deviceOut(int device, double duration, int headers, int loops,
        WAVEFORMATEX *pwfx, DWORD format, DWORD flags, WAVEOUTCAPSA *pcaps, BOOL interactive,
        BOOL sine, BOOL pause)
{
    HWAVEOUT wout;
    HANDLE hevent = CreateEventW(NULL, FALSE, FALSE, NULL);
    WAVEHDR *frags = 0;
    MMRESULT rc;
    DWORD volume;
    WORD nChannels = pwfx->nChannels;
    WORD wBitsPerSample = pwfx->wBitsPerSample;
    DWORD nSamplesPerSec = pwfx->nSamplesPerSec;
    BOOL has_volume = (pcaps->dwSupport & WAVECAPS_VOLUME) != 0;
    double paused = 0.0;
    DWORD_PTR callback = 0;
    DWORD_PTR callback_instance = 0;
    HANDLE thread = 0;
    DWORD thread_id;
    char * buffer;
    DWORD length;
    DWORD frag_length;
    int i, j;

    if ((flags & CALLBACK_TYPEMASK) == CALLBACK_EVENT) {
        callback = (DWORD_PTR)hevent;
        callback_instance = 0;
    } else if ((flags & CALLBACK_TYPEMASK) == CALLBACK_FUNCTION) {
        callback = (DWORD_PTR)callback_func;
        callback_instance = (DWORD_PTR)hevent;
    } else if ((flags & CALLBACK_TYPEMASK) == CALLBACK_THREAD) {
        thread = CreateThread(NULL, 0, callback_thread, hevent, 0, &thread_id);
        if (thread) {
            /* make sure thread is running */
            WaitForSingleObject(hevent,10000);
            callback = thread_id;
            callback_instance = 0;
        } else {
            trace("CreateThread() failed\n");
            CloseHandle(hevent);
            return;
        }
    } else if ((flags & CALLBACK_TYPEMASK) == CALLBACK_WINDOW) {
        trace("CALLBACK_THREAD not implemented\n");
        CloseHandle(hevent);
        return;
    } else if (flags & CALLBACK_TYPEMASK) {
        trace("Undefined callback type!\n");
        CloseHandle(hevent);
        return;
    } else {
        trace("CALLBACK_NULL not implemented\n");
        CloseHandle(hevent);
        return;
    }
    wout=NULL;
    g_tid = GetCurrentThreadId();
    rc=waveOutOpen(&wout,device,pwfx,callback,callback_instance,flags);
    /* Note: Win9x doesn't know WAVE_FORMAT_DIRECT */
    /* It is acceptable to fail on formats that are not specified to work */
    ok(rc==MMSYSERR_NOERROR || rc==MMSYSERR_BADDEVICEID ||
       rc==MMSYSERR_NOTENABLED || rc==MMSYSERR_NODRIVER ||
       rc==MMSYSERR_ALLOCATED ||
       ((rc==WAVERR_BADFORMAT || rc==MMSYSERR_NOTSUPPORTED) &&
       (flags & WAVE_FORMAT_DIRECT) && !(pcaps->dwFormats & format)) ||
       ((rc==WAVERR_BADFORMAT || rc==MMSYSERR_NOTSUPPORTED) &&
       (!(flags & WAVE_FORMAT_DIRECT) || (flags & WAVE_MAPPED)) &&
       !(pcaps->dwFormats & format)) ||
       (rc==MMSYSERR_INVALFLAG && (flags & WAVE_FORMAT_DIRECT)),
       "waveOutOpen(%s): format=%dx%2dx%d flags=%x(%s) rc=%s\n",
       dev_name(device),pwfx->nSamplesPerSec,pwfx->wBitsPerSample,
       pwfx->nChannels,CALLBACK_EVENT|flags,
       wave_open_flags(CALLBACK_EVENT|flags),wave_out_error(rc));
    if ((rc==WAVERR_BADFORMAT || rc==MMSYSERR_NOTSUPPORTED) &&
       (flags & WAVE_FORMAT_DIRECT) && (pcaps->dwFormats & format))
        trace(" Reason: The device lists this format as supported in its "
              "capabilities but opening it failed.\n");
    if ((rc==WAVERR_BADFORMAT || rc==MMSYSERR_NOTSUPPORTED) &&
       !(pcaps->dwFormats & format))
        trace("waveOutOpen(%s): format=%dx%2dx%d %s rc=%s failed but format "
              "not supported so OK.\n", dev_name(device), pwfx->nSamplesPerSec,
              pwfx->wBitsPerSample,pwfx->nChannels,
              flags & WAVE_FORMAT_DIRECT ? "flags=WAVE_FORMAT_DIRECT" :
              flags & WAVE_MAPPED ? "flags=WAVE_MAPPED" : "", mmsys_error(rc));
    if (rc!=MMSYSERR_NOERROR)
        goto EXIT;

    rc=WaitForSingleObject(hevent,9000);
    ok(rc==WAIT_OBJECT_0, "missing WOM_OPEN notification\n");

    ok(pwfx->nChannels==nChannels &&
       pwfx->wBitsPerSample==wBitsPerSample &&
       pwfx->nSamplesPerSec==nSamplesPerSec,
       "got the wrong format: %dx%2dx%d instead of %dx%2dx%d\n",
       pwfx->nSamplesPerSec, pwfx->wBitsPerSample,
       pwfx->nChannels, nSamplesPerSec, wBitsPerSample, nChannels);

    frags = HeapAlloc(GetProcessHeap(), 0, headers * sizeof(WAVEHDR));

    if (sine)
        buffer=wave_generate_la(pwfx,duration / (loops + 1),&length);
    else
        buffer=wave_generate_silence(pwfx,duration / (loops + 1),&length);

    rc=waveOutGetVolume(wout,0);
    ok(rc==MMSYSERR_INVALPARAM,"waveOutGetVolume(%s,0) expected "
       "MMSYSERR_INVALPARAM, got %s\n", dev_name(device),wave_out_error(rc));
    rc=waveOutGetVolume(wout,&volume);
    if (rc == MMSYSERR_NOTSUPPORTED) has_volume = FALSE;
    ok(has_volume ? rc==MMSYSERR_NOERROR : rc==MMSYSERR_NOTSUPPORTED,
       "waveOutGetVolume(%s): rc=%s\n",dev_name(device),wave_out_error(rc));

    /* make sure fragment length is a multiple of block size */
    frag_length = ((length / headers) / pwfx->nBlockAlign) * pwfx->nBlockAlign;

    for (i = 0; i < headers; i++) {
        frags[i].lpData=buffer + (i * frag_length);
        if (i != (headers-1))
            frags[i].dwBufferLength=frag_length;
        else {
            /* use remainder of buffer for last fragment */
            frags[i].dwBufferLength=length - (i * frag_length);
        }
        frags[i].dwFlags=0;
        frags[i].dwLoops=0;
        rc=waveOutPrepareHeader(wout, &frags[i], sizeof(frags[0]));
        ok(rc==MMSYSERR_NOERROR,
           "waveOutPrepareHeader(%s): rc=%s\n",dev_name(device),wave_out_error(rc));
    }

    if (interactive && rc==MMSYSERR_NOERROR) {
        trace("Playing %g second %s at %5dx%2dx%d %2d header%s %d loop%s %d bytes %s %s\n",duration,
              sine ? "440 Hz tone" : "silence", pwfx->nSamplesPerSec,
              pwfx->wBitsPerSample,pwfx->nChannels, headers, headers > 1 ? "s": " ",
              loops, loops == 1 ? " " : "s", length * (loops + 1),
              get_format_str(pwfx->wFormatTag),
              wave_open_flags(flags));
        if (sine && has_volume && volume == 0)
            trace("*** Warning the sound is muted, you will not hear the test\n");

        /* Check that the position is 0 at start */
        check_position(device, wout, 0, pwfx);

        rc=waveOutSetVolume(wout,0x20002000);
        ok(has_volume ? rc==MMSYSERR_NOERROR : rc==MMSYSERR_NOTSUPPORTED,
           "waveOutSetVolume(%s): rc=%s\n",dev_name(device),wave_out_error(rc));

        rc=waveOutSetVolume(wout,volume);
        ok(has_volume ? rc==MMSYSERR_NOERROR : rc==MMSYSERR_NOTSUPPORTED,
           "waveOutSetVolume(%s): rc=%s\n",dev_name(device),wave_out_error(rc));

        rc=waveOutWrite(wout, &frags[0], sizeof(frags[0]));
        ok(rc==MMSYSERR_NOERROR,"waveOutWrite(%s): rc=%s\n",
           dev_name(device),wave_out_error(rc));

        ok(frags[0].dwFlags==(WHDR_PREPARED|WHDR_INQUEUE),
           "WHDR_INQUEUE WHDR_PREPARED expected, got= %s\n",
           wave_header_flags(frags[0].dwFlags));

        rc=waveOutWrite(wout, &frags[0], sizeof(frags[0]));
        ok(rc==WAVERR_STILLPLAYING,
           "waveOutWrite(%s): WAVE_STILLPLAYING expected, got %s\n",
           dev_name(device),wave_out_error(rc));

        ok(frags[0].dwFlags==(WHDR_PREPARED|WHDR_INQUEUE),
           "WHDR_INQUEUE WHDR_PREPARED expected, got %s\n",
           wave_header_flags(frags[0].dwFlags));

        if (headers == 1 && loops == 0 && pause) {
            paused = duration / 2;
            Sleep(paused * 1000);
            rc=waveOutPause(wout);
            ok(rc==MMSYSERR_NOERROR,"waveOutPause(%s): rc=%s\n",
               dev_name(device),wave_out_error(rc));
            trace("pausing for %g seconds\n", paused);
            Sleep(paused * 1000);
            rc=waveOutRestart(wout);
            ok(rc==MMSYSERR_NOERROR,"waveOutRestart(%s): rc=%s\n",
               dev_name(device),wave_out_error(rc));
        }

        for (j = 0; j <= loops; j++) {
            for (i = 0; i < headers; i++) {
                /* don't do last one */
                if (!((j == loops) && (i == (headers - 1)))) {
                    if (j > 0)
                        frags[(i+1) % headers].dwFlags = WHDR_PREPARED;
                    rc=waveOutWrite(wout, &frags[(i+1) % headers], sizeof(frags[0]));
                    ok(rc==MMSYSERR_NOERROR,"waveOutWrite(%s, header[%d]): rc=%s\n",
                       dev_name(device),(i+1)%headers,wave_out_error(rc));
                }
                rc=WaitForSingleObject(hevent,8000);
                ok(rc==WAIT_OBJECT_0, "missing WOM_DONE notification\n");
            }
        }

        for (i = 0; i < headers; i++) {
            ok(frags[i].dwFlags==(WHDR_DONE|WHDR_PREPARED) ||
               broken((flags & CALLBACK_TYPEMASK)==CALLBACK_EVENT &&
                       frags[i].dwFlags==(WHDR_DONE|WHDR_PREPARED|0x1000)), /* < NT4 */
               "(%02d) WHDR_DONE WHDR_PREPARED expected, got %s\n",
               i, wave_header_flags(frags[i].dwFlags));
        }
        check_position(device, wout, length * (loops + 1), pwfx);
    }

    for (i = 0; i < headers; i++) {
        rc=waveOutUnprepareHeader(wout, &frags[i], sizeof(frags[0]));
        ok(rc==MMSYSERR_NOERROR,
           "waveOutUnprepareHeader(%s): rc=%s\n",dev_name(device),
           wave_out_error(rc));
    }

    ok(frags[0].dwFlags==(interactive ? WHDR_DONE : 0), "dwFlags(%d)=%x\n",device,frags[0].dwFlags);

    frags[0].dwFlags |= WHDR_DONE;
    rc=waveOutUnprepareHeader(wout, &frags[0], sizeof(frags[0]));
    ok(rc==MMSYSERR_NOERROR, "waveOutUnprepareHeader(%d): rc=%s\n",device,wave_out_error(rc));
    ok(frags[0].dwFlags==WHDR_DONE, "dwFlags(%d)=%x\n",device,frags[0].dwFlags);

    frags[0].dwFlags |= WHDR_INQUEUE;
    rc=waveOutPrepareHeader(wout, &frags[0], sizeof(frags[0]));
    ok(rc==MMSYSERR_NOERROR, "waveOutPrepareHeader(%d): rc=%s\n",device,wave_out_error(rc));
    ok(frags[0].dwFlags==WHDR_PREPARED, "dwFlags(%d)=%x\n",device,frags[0].dwFlags);

    frags[0].dwFlags |= WHDR_INQUEUE;
    rc=waveOutPrepareHeader(wout, &frags[0], sizeof(frags[0]));
    ok(rc==MMSYSERR_NOERROR, "waveOutPrepareHeader(%d): rc=%s\n",device,wave_out_error(rc));
    ok(frags[0].dwFlags==(WHDR_PREPARED|WHDR_INQUEUE), "dwFlags(%d)=%x\n",device,frags[0].dwFlags);

    frags[0].dwFlags &= ~(WHDR_INQUEUE|WHDR_DONE);
    rc=waveOutUnprepareHeader(wout, &frags[0], sizeof(frags[0]));
    ok(rc==MMSYSERR_NOERROR, "waveOutUnprepareHeader(%d): rc=%s\n",device,wave_out_error(rc));
    ok(frags[0].dwFlags==0, "dwFlags(%d)=%x\n",device,frags[0].dwFlags);

    rc=waveOutClose(wout);
    ok(rc==MMSYSERR_NOERROR,"waveOutClose(%s): rc=%s\n",dev_name(device),
       wave_out_error(rc));
    if (rc==WAVERR_STILLPLAYING) {
        /* waveOutReset ought to return all buffers s.t. waveOutClose succeeds */
        rc=waveOutReset(wout);
        ok(rc==MMSYSERR_NOERROR,"waveOutReset(%s): rc=%s\n",dev_name(device),
           wave_out_error(rc));

        for (i = 0; i < headers; i++) {
            rc=waveOutUnprepareHeader(wout, &frags[i], sizeof(frags[0]));
            ok(rc==MMSYSERR_NOERROR,
               "waveOutUnprepareHeader(%s): rc=%s\n",dev_name(device),
               wave_out_error(rc));
        }
        rc=waveOutClose(wout);
        ok(rc==MMSYSERR_NOERROR,"waveOutClose(%s): rc=%s\n",dev_name(device),
           wave_out_error(rc));
    }
    rc=WaitForSingleObject(hevent,1500);
    ok(rc==WAIT_OBJECT_0, "missing WOM_CLOSE notification\n");

    wout = (HWAVEOUT)0xdeadf00d;
    rc=waveOutOpen(&wout,device,pwfx,callback,callback_instance,flags|WAVE_FORMAT_QUERY);
    ok(rc==MMSYSERR_NOERROR, "WAVE_FORMAT_QUERY(%s): rc=%s\n",dev_name(device),
       wave_out_error(rc));
    ok(wout==(HWAVEOUT)0xdeadf00d, "WAVE_FORMAT_QUERY handle %p\n", wout);

    rc=WaitForSingleObject(hevent,20);
    ok(rc==WAIT_TIMEOUT, "Notification from %s rc=%x\n",
       wave_open_flags(flags|WAVE_FORMAT_QUERY),rc);

    HeapFree(GetProcessHeap(), 0, buffer);
EXIT:
    if ((flags & CALLBACK_TYPEMASK) == CALLBACK_THREAD) {
        PostThreadMessageW(thread_id, WM_APP, 0, 0);
        WaitForSingleObject(hevent,10000);
    }
    CloseHandle(hevent);
    HeapFree(GetProcessHeap(), 0, frags);
}

static void wave_out_test_device(UINT_PTR device)
{
    WAVEOUTCAPSA capsA;
    WAVEOUTCAPSW capsW;
    WAVEFORMATEX format;
    WAVEFORMATEXTENSIBLE wfex;
    IMAADPCMWAVEFORMAT wfa;
    HWAVEOUT wout;
    MMRESULT rc;
    UINT f;
    WCHAR * nameW;
    CHAR * nameA;
    DWORD size;
    DWORD dwPageSize;
    BYTE * twoPages;
    SYSTEM_INFO sSysInfo;
    DWORD flOldProtect;
    BOOL res;

    GetSystemInfo(&sSysInfo);
    dwPageSize = sSysInfo.dwPageSize;

    rc=waveOutGetDevCapsA(device,&capsA,sizeof(capsA));
    ok(rc==MMSYSERR_NOERROR || rc==MMSYSERR_BADDEVICEID ||
       rc==MMSYSERR_NODRIVER,
       "waveOutGetDevCapsA(%s): failed to get capabilities: rc=%s\n",
       dev_name(device),wave_out_error(rc));
    if (rc==MMSYSERR_BADDEVICEID || rc==MMSYSERR_NODRIVER)
        return;

    rc=waveOutGetDevCapsW(device,&capsW,sizeof(capsW));
    ok(rc==MMSYSERR_NOERROR || rc==MMSYSERR_NOTSUPPORTED,
       "waveOutGetDevCapsW(%s): MMSYSERR_NOERROR or MMSYSERR_NOTSUPPORTED "
       "expected, got %s\n",dev_name(device),wave_out_error(rc));

    rc=waveOutGetDevCapsA(device,0,sizeof(capsA));
    ok(rc==MMSYSERR_INVALPARAM,
       "waveOutGetDevCapsA(%s): MMSYSERR_INVALPARAM expected, "
       "got %s\n",dev_name(device),wave_out_error(rc));

    rc=waveOutGetDevCapsW(device,0,sizeof(capsW));
    ok(rc==MMSYSERR_INVALPARAM || rc==MMSYSERR_NOTSUPPORTED,
       "waveOutGetDevCapsW(%s): MMSYSERR_INVALPARAM or MMSYSERR_NOTSUPPORTED "
       "expected, got %s\n",dev_name(device),wave_out_error(rc));

    if (0)
    {
    /* FIXME: this works on windows but crashes wine */
    rc=waveOutGetDevCapsA(device,(LPWAVEOUTCAPSA)1,sizeof(capsA));
    ok(rc==MMSYSERR_INVALPARAM,
       "waveOutGetDevCapsA(%s): MMSYSERR_INVALPARAM expected, got %s\n",
       dev_name(device),wave_out_error(rc));

    rc=waveOutGetDevCapsW(device,(LPWAVEOUTCAPSW)1,sizeof(capsW));
    ok(rc==MMSYSERR_INVALPARAM || rc==MMSYSERR_NOTSUPPORTED,
       "waveOutGetDevCapsW(%s): MMSYSERR_INVALPARAM or MMSYSERR_NOTSUPPORTED "
       "expected, got %s\n",dev_name(device),wave_out_error(rc));
    }

    rc=waveOutGetDevCapsA(device,&capsA,4);
    ok(rc==MMSYSERR_NOERROR || rc==MMSYSERR_INVALPARAM,
       "waveOutGetDevCapsA(%s): MMSYSERR_NOERROR or MMSYSERR_INVALPARAM "
       "expected, got %s\n", dev_name(device),wave_out_error(rc));

    rc=waveOutGetDevCapsW(device,&capsW,4);
    ok(rc==MMSYSERR_NOERROR || rc==MMSYSERR_NOTSUPPORTED ||
       rc==MMSYSERR_INVALPARAM, /* Vista, W2K8 */
       "waveOutGetDevCapsW(%s): unexpected return value %s\n",
       dev_name(device),wave_out_error(rc));

    rc=waveOutMessage((HWAVEOUT)device, DRV_QUERYMAPPABLE, 0, 0);
    ok(rc==MMSYSERR_NOERROR || rc==MMSYSERR_NOTSUPPORTED,
            "DRV_QUERYMAPPABLE(%s): unexpected return value %s\n",
            dev_name(device),wave_out_error(rc));

    nameA=NULL;
    rc=waveOutMessage((HWAVEOUT)device, DRV_QUERYDEVICEINTERFACESIZE,
                      (DWORD_PTR)&size, 0);
    ok(rc==MMSYSERR_NOERROR || broken(rc==MMSYSERR_INVALPARAM ||
       rc==MMSYSERR_NOTSUPPORTED),
       "waveOutMessage(%s): failed to get interface size, rc=%s\n",
       dev_name(device),wave_out_error(rc));
    if (rc==MMSYSERR_NOERROR) {
        nameW = HeapAlloc(GetProcessHeap(), 0, size);
        rc=waveOutMessage((HWAVEOUT)device, DRV_QUERYDEVICEINTERFACE,
                          (DWORD_PTR)nameW, size);
        ok(rc==MMSYSERR_NOERROR,"waveOutMessage(%s): failed to get interface "
           "name, rc=%s\n",dev_name(device),wave_out_error(rc));
        ok(lstrlenW(nameW)+1==size/sizeof(WCHAR),"got an incorrect size %d\n",size);
        if (rc==MMSYSERR_NOERROR) {
            nameA = HeapAlloc(GetProcessHeap(), 0, size/sizeof(WCHAR));
            WideCharToMultiByte(CP_ACP, 0, nameW, size/sizeof(WCHAR), nameA,
                                size/sizeof(WCHAR), NULL, NULL);
        }
        HeapFree(GetProcessHeap(), 0, nameW);
    }
    else if (rc==MMSYSERR_NOTSUPPORTED) {
        nameA=HeapAlloc(GetProcessHeap(), 0, sizeof("not supported"));
        strcpy(nameA, "not supported");
    }

    rc=waveOutGetDevCapsA(device,&capsA,sizeof(capsA));
    ok(rc==MMSYSERR_NOERROR,
       "waveOutGetDevCapsA(%s): MMSYSERR_NOERROR expected, got %s\n",
       dev_name(device),wave_out_error(rc));
    if (rc!=MMSYSERR_NOERROR)
        return;

    trace("  %s: \"%s\" (%s) %d.%d (%d:%d)\n",dev_name(device),capsA.szPname,
          (nameA?nameA:"failed"),capsA.vDriverVersion >> 8,
          capsA.vDriverVersion & 0xff, capsA.wMid,capsA.wPid);
    trace("     channels=%d formats=%05x support=%04x\n",
          capsA.wChannels,capsA.dwFormats,capsA.dwSupport);
    trace("     %s\n",wave_out_caps(capsA.dwSupport));
    HeapFree(GetProcessHeap(), 0, nameA);

    if (winetest_interactive && (device != WAVE_MAPPER))
    {
        trace("Playing a 5 seconds reference tone.\n");
        trace("All subsequent tones should be identical to this one.\n");
        trace("Listen for stutter, changes in pitch, volume, etc.\n");
        format.wFormatTag=WAVE_FORMAT_PCM;
        format.nChannels=1;
        format.wBitsPerSample=8;
        format.nSamplesPerSec=22050;
        format.nBlockAlign=format.nChannels*format.wBitsPerSample/8;
        format.nAvgBytesPerSec=format.nSamplesPerSec*format.nBlockAlign;
        format.cbSize=0;

        wave_out_test_deviceOut(device,5.0,1,0,&format,WAVE_FORMAT_2M08,
                                CALLBACK_EVENT,&capsA,TRUE,TRUE,FALSE);
        wave_out_test_deviceOut(device,5.0,1,0,&format,WAVE_FORMAT_2M08,
                                CALLBACK_FUNCTION,&capsA,TRUE,TRUE,FALSE);
        wave_out_test_deviceOut(device,5.0,1,0,&format,WAVE_FORMAT_2M08,
                                CALLBACK_THREAD,&capsA,TRUE,TRUE,FALSE);

        wave_out_test_deviceOut(device,5.0,10,0,&format,WAVE_FORMAT_2M08,
                                CALLBACK_EVENT,&capsA,TRUE,TRUE,FALSE);
        wave_out_test_deviceOut(device,5.0,5,1,&format,WAVE_FORMAT_2M08,
                                CALLBACK_EVENT,&capsA,TRUE,TRUE,FALSE);
    } else {
        format.wFormatTag=WAVE_FORMAT_PCM;
        format.nChannels=1;
        format.wBitsPerSample=8;
        format.nSamplesPerSec=22050;
        format.nBlockAlign=format.nChannels*format.wBitsPerSample/8;
        format.nAvgBytesPerSec=format.nSamplesPerSec*format.nBlockAlign;
        format.cbSize=0;
        wave_out_test_deviceOut(device,0.6,1,0,&format,WAVE_FORMAT_2M08,
                                CALLBACK_EVENT,&capsA,TRUE,FALSE,FALSE);
        wave_out_test_deviceOut(device,0.6,1,0,&format,WAVE_FORMAT_2M08,
                                CALLBACK_EVENT,&capsA,TRUE,FALSE,TRUE);
        wave_out_test_deviceOut(device,0.6,1,0,&format,WAVE_FORMAT_2M08,
                                CALLBACK_FUNCTION,&capsA,TRUE,FALSE,FALSE);
        wave_out_test_deviceOut(device,0.6,1,0,&format,WAVE_FORMAT_2M08,
                                CALLBACK_FUNCTION,&capsA,TRUE,FALSE,TRUE);
        wave_out_test_deviceOut(device,0.6,1,0,&format,WAVE_FORMAT_2M08,
                                CALLBACK_THREAD,&capsA,TRUE,FALSE,FALSE);
        wave_out_test_deviceOut(device,0.6,1,0,&format,WAVE_FORMAT_2M08,
                                CALLBACK_THREAD,&capsA,TRUE,FALSE,TRUE);

        wave_out_test_deviceOut(device,0.8,10,0,&format,WAVE_FORMAT_2M08,
                                CALLBACK_EVENT,&capsA,TRUE,FALSE,FALSE);
        wave_out_test_deviceOut(device,1.0,5,1,&format,WAVE_FORMAT_2M08,
                                CALLBACK_EVENT,&capsA,TRUE,FALSE,FALSE);
    }

    for (f=0;f<NB_WIN_FORMATS;f++) {
        format.wFormatTag=WAVE_FORMAT_PCM;
        format.nChannels=win_formats[f][3];
        format.wBitsPerSample=win_formats[f][2];
        format.nSamplesPerSec=win_formats[f][1];
        format.nBlockAlign=format.nChannels*format.wBitsPerSample/8;
        format.nAvgBytesPerSec=format.nSamplesPerSec*format.nBlockAlign;
        format.cbSize=0;
        wave_out_test_deviceOut(device,1.0,1,0,&format,win_formats[f][0],
                                CALLBACK_EVENT,&capsA,winetest_interactive,
                                TRUE,FALSE);
        wave_out_test_deviceOut(device,1.0,1,0,&format,win_formats[f][0],
                                CALLBACK_FUNCTION,&capsA,winetest_interactive,
                                TRUE,FALSE);
        wave_out_test_deviceOut(device,1.0,1,0,&format,win_formats[f][0],
                                CALLBACK_THREAD,&capsA,winetest_interactive,
                                TRUE,FALSE);

        wave_out_test_deviceOut(device,1.0,10,0,&format,win_formats[f][0],
                                CALLBACK_EVENT,&capsA,winetest_interactive,
                                TRUE,FALSE);
        wave_out_test_deviceOut(device,1.0,5,1,&format,win_formats[f][0],
                                CALLBACK_EVENT,&capsA,winetest_interactive,
                                TRUE,FALSE);

        if (winetest_interactive) {
            wave_out_test_deviceOut(device,1.0,1,0,&format,win_formats[f][0],
                                    CALLBACK_EVENT,&capsA,winetest_interactive,
                                    TRUE,TRUE);
            wave_out_test_deviceOut(device,1.0,1,0,&format,win_formats[f][0],
                                    CALLBACK_FUNCTION,&capsA,winetest_interactive,
                                    TRUE,TRUE);
            wave_out_test_deviceOut(device,1.0,1,0,&format,win_formats[f][0],
                                    CALLBACK_THREAD,&capsA,winetest_interactive,
                                    TRUE,TRUE);

            wave_out_test_deviceOut(device,1.0,10,0,&format,win_formats[f][0],
                                    CALLBACK_EVENT,&capsA,winetest_interactive,
                                    TRUE,TRUE);
            wave_out_test_deviceOut(device,1.0,5,1,&format,win_formats[f][0],
                                    CALLBACK_EVENT,&capsA,winetest_interactive,
                                    TRUE,TRUE);
        }
        if (device != WAVE_MAPPER)
        {
            wave_out_test_deviceOut(device,1.0,1,0,&format,win_formats[f][0],
                                    CALLBACK_EVENT|WAVE_FORMAT_DIRECT,&capsA,
                                    winetest_interactive,TRUE,FALSE);
            wave_out_test_deviceOut(device,1.0,1,0,&format,win_formats[f][0],
                                    CALLBACK_EVENT|WAVE_MAPPED,&capsA,
                                    winetest_interactive,TRUE,FALSE);
            wave_out_test_deviceOut(device,1.0,1,0,&format,win_formats[f][0],
                                    CALLBACK_FUNCTION|WAVE_FORMAT_DIRECT,&capsA,
                                    winetest_interactive,TRUE,FALSE);
            wave_out_test_deviceOut(device,1.0,1,0,&format,win_formats[f][0],
                                    CALLBACK_FUNCTION|WAVE_MAPPED,&capsA,
                                    winetest_interactive,TRUE,FALSE);
            wave_out_test_deviceOut(device,1.0,1,0,&format,win_formats[f][0],
                                    CALLBACK_THREAD|WAVE_FORMAT_DIRECT,&capsA,
                                    winetest_interactive,TRUE,FALSE);
            wave_out_test_deviceOut(device,1.0,1,0,&format,win_formats[f][0],
                                    CALLBACK_THREAD|WAVE_MAPPED,&capsA,
                                    winetest_interactive,TRUE,FALSE);

            wave_out_test_deviceOut(device,1.0,10,0,&format,win_formats[f][0],
                                    CALLBACK_EVENT|WAVE_FORMAT_DIRECT,&capsA,
                                    winetest_interactive,TRUE,FALSE);
            wave_out_test_deviceOut(device,1.0,5,1,&format,win_formats[f][0],
                                    CALLBACK_EVENT|WAVE_FORMAT_DIRECT,&capsA,
                                    winetest_interactive,TRUE,FALSE);
        }
    }

    /* Try a PCMWAVEFORMAT aligned next to an unaccessible page for bounds
     * checking */
    twoPages = VirtualAlloc(NULL, 2 * dwPageSize, MEM_RESERVE | MEM_COMMIT,
                            PAGE_READWRITE);
    ok(twoPages!=NULL,"Failed to allocate 2 pages of memory\n");
    if (twoPages) {
        res = VirtualProtect(twoPages + dwPageSize, dwPageSize, PAGE_NOACCESS,
                             &flOldProtect);
        ok(res, "Failed to set memory access on second page\n");
        if (res) {
            LPWAVEFORMATEX pwfx = (LPWAVEFORMATEX)(twoPages + dwPageSize -
                sizeof(PCMWAVEFORMAT));
            pwfx->wFormatTag=WAVE_FORMAT_PCM;
            pwfx->nChannels=1;
            pwfx->wBitsPerSample=8;
            pwfx->nSamplesPerSec=22050;
            pwfx->nBlockAlign=pwfx->nChannels*pwfx->wBitsPerSample/8;
            pwfx->nAvgBytesPerSec=pwfx->nSamplesPerSec*pwfx->nBlockAlign;
            wave_out_test_deviceOut(device,1.0,1,0,pwfx,WAVE_FORMAT_2M08,
                                    CALLBACK_EVENT,&capsA,winetest_interactive,
                                    TRUE,FALSE);
            wave_out_test_deviceOut(device,1.0,10,0,pwfx,WAVE_FORMAT_2M08,
                                    CALLBACK_EVENT,&capsA,winetest_interactive,
                                    TRUE,FALSE);
            wave_out_test_deviceOut(device,1.0,5,1,pwfx,WAVE_FORMAT_2M08,
                                    CALLBACK_EVENT,&capsA,winetest_interactive,
                                    TRUE,FALSE);
            if (device != WAVE_MAPPER)
            {
                wave_out_test_deviceOut(device,1.0,1,0,pwfx,WAVE_FORMAT_2M08,
                                        CALLBACK_EVENT|WAVE_FORMAT_DIRECT,
                                        &capsA,winetest_interactive,TRUE,FALSE);
                wave_out_test_deviceOut(device,1.0,1,0,pwfx,WAVE_FORMAT_2M08,
                                        CALLBACK_EVENT|WAVE_MAPPED,&capsA,
                                        winetest_interactive,TRUE,FALSE);
                wave_out_test_deviceOut(device,1.0,10,0,pwfx,WAVE_FORMAT_2M08,
                                        CALLBACK_EVENT|WAVE_FORMAT_DIRECT,
                                        &capsA,winetest_interactive,TRUE,FALSE);
                wave_out_test_deviceOut(device,1.0,10,0,pwfx,WAVE_FORMAT_2M08,
                                        CALLBACK_EVENT|WAVE_MAPPED,&capsA,
                                        winetest_interactive,TRUE,FALSE);
                wave_out_test_deviceOut(device,1.0,5,1,pwfx,WAVE_FORMAT_2M08,
                                        CALLBACK_EVENT|WAVE_FORMAT_DIRECT,
                                        &capsA,winetest_interactive,TRUE,FALSE);
                wave_out_test_deviceOut(device,1.0,5,1,pwfx,WAVE_FORMAT_2M08,
                                        CALLBACK_EVENT|WAVE_MAPPED,&capsA,
                                        winetest_interactive,TRUE,FALSE);
            }
        }
        VirtualFree(twoPages, 0, MEM_RELEASE);
    }

    /* try some non PCM formats */
    format.wFormatTag=WAVE_FORMAT_MULAW;
    format.nChannels=1;
    format.wBitsPerSample=8;
    format.nSamplesPerSec=8000;
    format.nBlockAlign=format.nChannels*format.wBitsPerSample/8;
    format.nAvgBytesPerSec=format.nSamplesPerSec*format.nBlockAlign;
    format.cbSize=0;
    rc=waveOutOpen(&wout,device,&format,0,0,CALLBACK_NULL|WAVE_FORMAT_DIRECT);
    ok(rc==MMSYSERR_NOERROR ||rc==WAVERR_BADFORMAT ||
       rc==MMSYSERR_INVALFLAG || rc==MMSYSERR_INVALPARAM,
       "waveOutOpen(%s): returned %s\n",dev_name(device),wave_out_error(rc));
    if (rc==MMSYSERR_NOERROR) {
        waveOutClose(wout);
        wave_out_test_deviceOut(device,1.0,1,0,&format,0,CALLBACK_EVENT,
                                &capsA,winetest_interactive,TRUE,FALSE);
        wave_out_test_deviceOut(device,1.0,10,0,&format,0,CALLBACK_EVENT,
                                &capsA,winetest_interactive,TRUE,FALSE);
        wave_out_test_deviceOut(device,1.0,5,1,&format,0,CALLBACK_EVENT,
                                &capsA,winetest_interactive,TRUE,FALSE);
    } else {
        MMRESULT query_rc;

        trace("waveOutOpen(%s): WAVE_FORMAT_MULAW not supported\n",
              dev_name(device));

        query_rc = waveOutOpen(NULL, device, &format, 0, 0, CALLBACK_NULL | WAVE_FORMAT_QUERY);
        ok(query_rc==MMSYSERR_NOERROR || query_rc==WAVERR_BADFORMAT || query_rc==MMSYSERR_INVALPARAM,
           "waveOutOpen(%s): returned %s\n",dev_name(device),wave_out_error(rc));

        rc = waveOutOpen(&wout, device, &format, 0, 0, CALLBACK_NULL);
        ok(rc == query_rc,
           "waveOutOpen(%s): returned different from query: %s\n",dev_name(device),wave_out_error(rc));
        if(rc == MMSYSERR_NOERROR)
            waveOutClose(wout);
    }

    wfa.wfx.wFormatTag=WAVE_FORMAT_IMA_ADPCM;
    wfa.wfx.nChannels=1;
    wfa.wfx.nSamplesPerSec=11025;
    wfa.wfx.nAvgBytesPerSec=5588;
    wfa.wfx.nBlockAlign=256;
    wfa.wfx.wBitsPerSample=4; /* see imaadp32.c */
    wfa.wfx.cbSize=2;
    wfa.wSamplesPerBlock=505;
    rc=waveOutOpen(&wout,device,&wfa.wfx,0,0,CALLBACK_NULL|WAVE_FORMAT_DIRECT);
    ok(rc==MMSYSERR_NOERROR ||rc==WAVERR_BADFORMAT ||
       rc==MMSYSERR_INVALFLAG || rc==MMSYSERR_INVALPARAM,
       "waveOutOpen(%s): returned %s\n",dev_name(device),wave_out_error(rc));
    if (rc==MMSYSERR_NOERROR) {
        waveOutClose(wout);
        /* TODO: teach wave_generate_* ADPCM
        wave_out_test_deviceOut(device,1.0,1,0,&wfa.wfx,0,CALLBACK_EVENT,
                                &capsA,winetest_interactive,TRUE,FALSE);
        wave_out_test_deviceOut(device,1.0,10,0,&wfa.wfx,0,CALLBACK_EVENT,
                                &capsA,winetest_interactive,TRUE,FALSE);
        wave_out_test_deviceOut(device,1.0,5,1,&wfa.wfx,0,CALLBACK_EVENT,
                                &capsA,winetest_interactive,TRUE,FALSE);
	*/
    } else
        trace("waveOutOpen(%s): WAVE_FORMAT_IMA_ADPCM not supported\n",
              dev_name(device));

    /* test if WAVEFORMATEXTENSIBLE supported */
    wfex.Format.wFormatTag=WAVE_FORMAT_EXTENSIBLE;
    wfex.Format.nChannels=2;
    wfex.Format.wBitsPerSample=16;
    wfex.Format.nSamplesPerSec=22050;
    wfex.Format.nBlockAlign=wfex.Format.nChannels*wfex.Format.wBitsPerSample/8;
    wfex.Format.nAvgBytesPerSec=wfex.Format.nSamplesPerSec*
        wfex.Format.nBlockAlign;
    wfex.Format.cbSize=22;
    wfex.Samples.wValidBitsPerSample=wfex.Format.wBitsPerSample;
    wfex.dwChannelMask=SPEAKER_ALL;
    wfex.SubFormat=KSDATAFORMAT_SUBTYPE_PCM;
    rc=waveOutOpen(&wout,device,&wfex.Format,0,0,
                   CALLBACK_NULL|WAVE_FORMAT_DIRECT);
    ok(rc==MMSYSERR_NOERROR || rc==WAVERR_BADFORMAT ||
       rc==MMSYSERR_INVALFLAG || rc==MMSYSERR_INVALPARAM,
       "waveOutOpen(%s): returned %s\n",dev_name(device),wave_out_error(rc));
    if (rc==MMSYSERR_NOERROR) {
        waveOutClose(wout);
        wave_out_test_deviceOut(device,1.0,1,0,&wfex.Format,WAVE_FORMAT_2M16,
                                CALLBACK_EVENT,&capsA,winetest_interactive,
                                TRUE,FALSE);
        wave_out_test_deviceOut(device,1.0,10,0,&wfex.Format,WAVE_FORMAT_2M16,
                                CALLBACK_EVENT,&capsA,winetest_interactive,
                                TRUE,FALSE);
        wave_out_test_deviceOut(device,1.0,5,1,&wfex.Format,WAVE_FORMAT_2M16,
                                CALLBACK_EVENT,&capsA,winetest_interactive,
                                TRUE,FALSE);
    } else
        trace("waveOutOpen(%s): WAVE_FORMAT_EXTENSIBLE not supported\n",
              dev_name(device));

    /* test if 4 channels supported */
    wfex.Format.wFormatTag=WAVE_FORMAT_EXTENSIBLE;
    wfex.Format.nChannels=4;
    wfex.Format.wBitsPerSample=16;
    wfex.Format.nSamplesPerSec=22050;
    wfex.Format.nBlockAlign=wfex.Format.nChannels*wfex.Format.wBitsPerSample/8;
    wfex.Format.nAvgBytesPerSec=wfex.Format.nSamplesPerSec*
        wfex.Format.nBlockAlign;
    wfex.Format.cbSize=22;
    wfex.Samples.wValidBitsPerSample=wfex.Format.wBitsPerSample;
    wfex.dwChannelMask=SPEAKER_ALL;
    wfex.SubFormat=KSDATAFORMAT_SUBTYPE_PCM;
    rc=waveOutOpen(&wout,device,&wfex.Format,0,0,
                   CALLBACK_NULL|WAVE_FORMAT_DIRECT);
    ok(rc==MMSYSERR_NOERROR || rc==WAVERR_BADFORMAT ||
       rc==MMSYSERR_INVALFLAG || rc==MMSYSERR_INVALPARAM,
       "waveOutOpen(%s): returned %s\n",dev_name(device),wave_out_error(rc));
    if (rc==MMSYSERR_NOERROR) {
        waveOutClose(wout);
        wave_out_test_deviceOut(device,1.0,1,0,&wfex.Format,0,CALLBACK_EVENT,
                                &capsA,winetest_interactive,TRUE,FALSE);
        wave_out_test_deviceOut(device,1.0,10,0,&wfex.Format,0,CALLBACK_EVENT,
                                &capsA,winetest_interactive,TRUE,FALSE);
        wave_out_test_deviceOut(device,1.0,5,1,&wfex.Format,0,CALLBACK_EVENT,
                                &capsA,winetest_interactive,TRUE,FALSE);
    } else
        trace("waveOutOpen(%s): 4 channels not supported\n",
              dev_name(device));

    /* test if 6 channels supported */
    wfex.Format.wFormatTag=WAVE_FORMAT_EXTENSIBLE;
    wfex.Format.nChannels=6;
    wfex.Format.wBitsPerSample=16;
    wfex.Format.nSamplesPerSec=22050;
    wfex.Format.nBlockAlign=wfex.Format.nChannels*wfex.Format.wBitsPerSample/8;
    wfex.Format.nAvgBytesPerSec=wfex.Format.nSamplesPerSec*
        wfex.Format.nBlockAlign;
    wfex.Format.cbSize=22;
    wfex.Samples.wValidBitsPerSample=wfex.Format.wBitsPerSample;
    wfex.dwChannelMask=SPEAKER_ALL;
    wfex.SubFormat=KSDATAFORMAT_SUBTYPE_PCM;
    rc=waveOutOpen(&wout,device,&wfex.Format,0,0,
                   CALLBACK_NULL|WAVE_FORMAT_DIRECT);
    ok(rc==MMSYSERR_NOERROR || rc==WAVERR_BADFORMAT ||
       rc==MMSYSERR_INVALFLAG || rc==MMSYSERR_INVALPARAM,
       "waveOutOpen(%s): returned %s\n",dev_name(device),wave_out_error(rc));
    if (rc==MMSYSERR_NOERROR) {
        waveOutClose(wout);
        wave_out_test_deviceOut(device,1.0,1,0,&wfex.Format,WAVE_FORMAT_2M16,
                                CALLBACK_EVENT,&capsA,winetest_interactive,
                                TRUE,FALSE);
        wave_out_test_deviceOut(device,1.0,10,0,&wfex.Format,WAVE_FORMAT_2M16,
                                CALLBACK_EVENT,&capsA,winetest_interactive,
                                TRUE,FALSE);
        wave_out_test_deviceOut(device,1.0,5,1,&wfex.Format,WAVE_FORMAT_2M16,
                                CALLBACK_EVENT,&capsA,winetest_interactive,
                                TRUE,FALSE);
    } else
        trace("waveOutOpen(%s): 6 channels not supported\n",
              dev_name(device));

    if (0)
    {
    /* FIXME: ALSA doesn't like this format */
    /* test if 24 bit samples supported */
    wfex.Format.wFormatTag=WAVE_FORMAT_EXTENSIBLE;
    wfex.Format.nChannels=2;
    wfex.Format.wBitsPerSample=24;
    wfex.Format.nSamplesPerSec=22050;
    wfex.Format.nBlockAlign=wfex.Format.nChannels*wfex.Format.wBitsPerSample/8;
    wfex.Format.nAvgBytesPerSec=wfex.Format.nSamplesPerSec*
        wfex.Format.nBlockAlign;
    wfex.Format.cbSize=22;
    wfex.Samples.wValidBitsPerSample=wfex.Format.wBitsPerSample;
    wfex.dwChannelMask=SPEAKER_ALL;
    wfex.SubFormat=KSDATAFORMAT_SUBTYPE_PCM;
    rc=waveOutOpen(&wout,device,&wfex.Format,0,0,
                   CALLBACK_NULL|WAVE_FORMAT_DIRECT);
    ok(rc==MMSYSERR_NOERROR || rc==WAVERR_BADFORMAT ||
       rc==MMSYSERR_INVALFLAG || rc==MMSYSERR_INVALPARAM,
       "waveOutOpen(%s): returned %s\n",dev_name(device),wave_out_error(rc));
    if (rc==MMSYSERR_NOERROR) {
        waveOutClose(wout);
        wave_out_test_deviceOut(device,1.0,1,0,&wfex.Format,WAVE_FORMAT_2M16,
                                CALLBACK_EVENT,&capsA,winetest_interactive,
                                TRUE,FALSE);
    } else
        trace("waveOutOpen(%s): 24 bit samples not supported\n",
              dev_name(device));
    }

    /* test if 32 bit samples supported */
    wfex.Format.wFormatTag=WAVE_FORMAT_EXTENSIBLE;
    wfex.Format.nChannels=2;
    wfex.Format.wBitsPerSample=32;
    wfex.Format.nSamplesPerSec=22050;
    wfex.Format.nBlockAlign=wfex.Format.nChannels*wfex.Format.wBitsPerSample/8;
    wfex.Format.nAvgBytesPerSec=wfex.Format.nSamplesPerSec*
        wfex.Format.nBlockAlign;
    wfex.Format.cbSize=22;
    wfex.Samples.wValidBitsPerSample=wfex.Format.wBitsPerSample;
    wfex.dwChannelMask=SPEAKER_ALL;
    wfex.SubFormat=KSDATAFORMAT_SUBTYPE_PCM;
    rc=waveOutOpen(&wout,device,&wfex.Format,0,0,
                   CALLBACK_NULL|WAVE_FORMAT_DIRECT);
    ok(rc==MMSYSERR_NOERROR || rc==WAVERR_BADFORMAT ||
       rc==MMSYSERR_INVALFLAG || rc==MMSYSERR_INVALPARAM,
       "waveOutOpen(%s): returned %s\n",dev_name(device),wave_out_error(rc));
    if (rc==MMSYSERR_NOERROR) {
        waveOutClose(wout);
        wave_out_test_deviceOut(device,1.0,1,0,&wfex.Format,WAVE_FORMAT_2M16,
                                CALLBACK_EVENT,&capsA,winetest_interactive,
                                TRUE,FALSE);
        wave_out_test_deviceOut(device,1.0,10,0,&wfex.Format,WAVE_FORMAT_2M16,
                                CALLBACK_EVENT,&capsA,winetest_interactive,
                                TRUE,FALSE);
        wave_out_test_deviceOut(device,1.0,5,1,&wfex.Format,WAVE_FORMAT_2M16,
                                CALLBACK_EVENT,&capsA,winetest_interactive,
                                TRUE,FALSE);
    } else
        trace("waveOutOpen(%s): 32 bit samples not supported\n",
              dev_name(device));

    /* test if 32 bit float samples supported */
    wfex.Format.wFormatTag=WAVE_FORMAT_EXTENSIBLE;
    wfex.Format.nChannels=2;
    wfex.Format.wBitsPerSample=32;
    wfex.Format.nSamplesPerSec=22050;
    wfex.Format.nBlockAlign=wfex.Format.nChannels*wfex.Format.wBitsPerSample/8;
    wfex.Format.nAvgBytesPerSec=wfex.Format.nSamplesPerSec*
        wfex.Format.nBlockAlign;
    wfex.Format.cbSize=22;
    wfex.Samples.wValidBitsPerSample=wfex.Format.wBitsPerSample;
    wfex.dwChannelMask=SPEAKER_ALL;
    wfex.SubFormat=KSDATAFORMAT_SUBTYPE_IEEE_FLOAT;
    rc=waveOutOpen(&wout,device,&wfex.Format,0,0,
                   CALLBACK_NULL|WAVE_FORMAT_DIRECT);
    ok(rc==MMSYSERR_NOERROR || rc==WAVERR_BADFORMAT ||
       rc==MMSYSERR_INVALFLAG || rc==MMSYSERR_INVALPARAM,
       "waveOutOpen(%s): returned %s\n",dev_name(device),wave_out_error(rc));
    if (rc==MMSYSERR_NOERROR) {
        waveOutClose(wout);
        wave_out_test_deviceOut(device,1.0,1,0,&wfex.Format,WAVE_FORMAT_2M16,
                                CALLBACK_EVENT,&capsA,winetest_interactive,
                                TRUE,FALSE);
        wave_out_test_deviceOut(device,1.0,10,0,&wfex.Format,WAVE_FORMAT_2M16,
                                CALLBACK_EVENT,&capsA,winetest_interactive,
                                TRUE,FALSE);
        wave_out_test_deviceOut(device,1.0,5,1,&wfex.Format,WAVE_FORMAT_2M16,
                                CALLBACK_EVENT,&capsA,winetest_interactive,
                                TRUE,FALSE);
    } else
        trace("waveOutOpen(%s): 32 bit float samples not supported\n",
              dev_name(device));
}

static void wave_out_tests(void)
{
    WAVEOUTCAPSA capsA;
    WAVEOUTCAPSW capsW;
    WAVEFORMATEX format;
    HWAVEOUT wout;
    MMRESULT rc;
    DWORD preferred, status;
    UINT ndev,d;

    ndev=waveOutGetNumDevs();
    trace("found %d WaveOut devices\n",ndev);

    rc = waveOutMessage((HWAVEOUT)WAVE_MAPPER, DRVM_MAPPER_PREFERRED_GET,
            (DWORD_PTR)&preferred, (DWORD_PTR)&status);
    ok((ndev == 0 && (rc == MMSYSERR_NODRIVER || rc == MMSYSERR_BADDEVICEID)) ||
            rc == MMSYSERR_NOTSUPPORTED ||
            rc == MMSYSERR_NOERROR, "waveOutMessage(DRVM_MAPPER_PREFERRED_GET) failed: %u\n", rc);

    if(rc != MMSYSERR_NOTSUPPORTED)
        ok((ndev == 0 && (preferred == -1 || broken(preferred != -1))) ||
                preferred < ndev, "Got invalid preferred device: 0x%x\n", preferred);

    rc=waveOutGetDevCapsA(ndev+1,&capsA,sizeof(capsA));
    ok(rc==MMSYSERR_BADDEVICEID,
       "waveOutGetDevCapsA(%s): MMSYSERR_BADDEVICEID expected, got %s\n",
       dev_name(ndev+1),mmsys_error(rc));

    rc=waveOutGetDevCapsW(ndev+1,&capsW,sizeof(capsW));
    ok(rc==MMSYSERR_BADDEVICEID || rc==MMSYSERR_NOTSUPPORTED,
       "waveOutGetDevCapsW(%s): MMSYSERR_BADDEVICEID or MMSYSERR_NOTSUPPORTED "
       "expected, got %s\n",dev_name(ndev+1),mmsys_error(rc));

    rc=waveOutGetDevCapsA(WAVE_MAPPER,&capsA,sizeof(capsA));
    if (ndev>0)
        ok(rc==MMSYSERR_NOERROR,
           "waveOutGetDevCapsA(%s): MMSYSERR_NOERROR expected, got %s\n",
           dev_name(WAVE_MAPPER),mmsys_error(rc));
    else
        ok(rc==MMSYSERR_BADDEVICEID || rc==MMSYSERR_NODRIVER,
           "waveOutGetDevCapsA(%s): MMSYSERR_BADDEVICEID or MMSYSERR_NODRIVER "
           "expected, got %s\n",dev_name(WAVE_MAPPER),mmsys_error(rc));

    rc=waveOutGetDevCapsW(WAVE_MAPPER,&capsW,sizeof(capsW));
    if (ndev>0)
        ok(rc==MMSYSERR_NOERROR || rc==MMSYSERR_NOTSUPPORTED,
           "waveOutGetDevCapsW(%s): MMSYSERR_NOERROR or MMSYSERR_NOTSUPPORTED "
           "expected, got %s\n",dev_name(WAVE_MAPPER),mmsys_error(rc));
    else
        ok(rc==MMSYSERR_BADDEVICEID || rc==MMSYSERR_NODRIVER ||
           rc==MMSYSERR_NOTSUPPORTED,
           "waveOutGetDevCapsW(%s): MMSYSERR_BADDEVICEID or MMSYSERR_NODRIVER "
           " or MMSYSERR_NOTSUPPORTED expected, got %s\n",
           dev_name(WAVE_MAPPER),mmsys_error(rc));

    format.wFormatTag=WAVE_FORMAT_PCM;
    format.nChannels=2;
    format.wBitsPerSample=16;
    format.nSamplesPerSec=44100;
    format.nBlockAlign=format.nChannels*format.wBitsPerSample/8;
    format.nAvgBytesPerSec=format.nSamplesPerSec*format.nBlockAlign;
    format.cbSize=0;
    rc=waveOutOpen(&wout,ndev+1,&format,0,0,CALLBACK_NULL);
    ok(rc==MMSYSERR_BADDEVICEID,
       "waveOutOpen(%s): MMSYSERR_BADDEVICEID expected, got %s\n",
       dev_name(ndev+1),mmsys_error(rc));

    if(winetest_interactive)
        for (d=0;d<ndev;d++)
            wave_out_test_device(d);

    if (ndev>0)
        wave_out_test_device(WAVE_MAPPER);
}

static void test_sndPlaySound(void)
{
    BOOL br;

    static const WCHAR not_existW[] = {'C',':','\\','n','o','t','_','e','x','i','s','t','.','w','a','v',0};
    static const WCHAR SystemAsteriskW[] = {'S','y','s','t','e','m','A','s','t','e','r','i','s','k',0};

    br = sndPlaySoundA((LPCSTR)SND_ALIAS_SYSTEMASTERISK, SND_ALIAS_ID|SND_SYNC);
    ok(br == TRUE || br == FALSE, "sndPlaySound gave strange return: %u\n", br);

    br = sndPlaySoundW((LPCWSTR)SND_ALIAS_SYSTEMASTERISK, SND_ALIAS_ID|SND_SYNC);
    ok(br == TRUE || br == FALSE, "sndPlaySound gave strange return: %u\n", br);

    br = sndPlaySoundA((LPCSTR)sndAlias('X','Y'), SND_ALIAS_ID|SND_SYNC);
    ok(br == TRUE || br == FALSE, "sndPlaySound gave strange return: %u\n", br);

    br = sndPlaySoundW((LPCWSTR)sndAlias('X','Y'), SND_ALIAS_ID|SND_SYNC);
    ok(br == TRUE || br == FALSE, "sndPlaySound gave strange return: %u\n", br);

    br = sndPlaySoundA("SystemAsterisk", SND_ALIAS|SND_SYNC);
    ok(br == TRUE || br == FALSE, "sndPlaySound gave strange return: %u\n", br);

    br = sndPlaySoundW(SystemAsteriskW, SND_ALIAS|SND_SYNC);
    ok(br == TRUE || br == FALSE, "sndPlaySound gave strange return: %u\n", br);

    br = sndPlaySoundA("C:\not_exist.wav", SND_FILENAME|SND_SYNC);
    ok(br == TRUE || br == FALSE, "sndPlaySound gave strange return: %u\n", br);

    br = sndPlaySoundW(not_existW, SND_FILENAME|SND_SYNC);
    ok(br == TRUE || br == FALSE, "sndPlaySound gave strange return: %u\n", br);
}

static void test_fragmentsize(void)
{
    MMRESULT rc;
    WAVEHDR hdr[2];
    HWAVEOUT wout;
    WAVEFORMATEX fmt;
    MMTIME mmtime;
    DWORD wait;
    HANDLE hevent;

    if(waveOutGetNumDevs() == 0)
        return;

    fmt.wFormatTag = WAVE_FORMAT_PCM;
    fmt.nChannels = 2;
    fmt.nSamplesPerSec = 44100;
    fmt.wBitsPerSample = 16;
    fmt.nBlockAlign = fmt.nChannels * fmt.wBitsPerSample / 8;
    fmt.nAvgBytesPerSec = fmt.nBlockAlign * fmt.nSamplesPerSec;
    fmt.cbSize = sizeof(WAVEFORMATEX);

    hevent = CreateEventW(NULL, FALSE, FALSE, NULL);
    g_tid = GetCurrentThreadId();

    rc = waveOutOpen(&wout, WAVE_MAPPER, &fmt, (DWORD_PTR)callback_func,
            (DWORD_PTR)hevent, CALLBACK_FUNCTION);
    ok(rc == MMSYSERR_NOERROR || rc == WAVERR_BADFORMAT ||
           rc == MMSYSERR_INVALFLAG || rc == MMSYSERR_INVALPARAM,
           "waveOutOpen(%s) failed: %s\n", dev_name(WAVE_MAPPER), wave_out_error(rc));
    if(rc != MMSYSERR_NOERROR){
        CloseHandle(hevent);
        return;
    }

    wait = WaitForSingleObject(hevent, 1000);
    ok(wait == WAIT_OBJECT_0, "wave open callback missed\n");

    memset(hdr, 0, sizeof(hdr));
    hdr[0].dwBufferLength = (fmt.nSamplesPerSec * fmt.nBlockAlign / 4) + 1;
    hdr[1].dwBufferLength = hdr[0].dwBufferLength - 2;
    hdr[1].lpData = hdr[0].lpData =
        HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, hdr[0].dwBufferLength);

    rc = waveOutPrepareHeader(wout, &hdr[0], sizeof(hdr[0]));
    ok(rc == MMSYSERR_NOERROR, "waveOutPrepareHeader failed: %s\n", wave_out_error(rc));

    rc = waveOutPrepareHeader(wout, &hdr[1], sizeof(hdr[1]));
    ok(rc == MMSYSERR_NOERROR, "waveOutPrepareHeader failed: %s\n", wave_out_error(rc));

    trace("writing %u bytes then %u bytes\n", hdr[0].dwBufferLength, hdr[1].dwBufferLength);
    rc = waveOutWrite(wout, &hdr[0], sizeof(hdr[0]));
    ok(rc == MMSYSERR_NOERROR, "waveOutWrite failed: %s\n", wave_out_error(rc));

    rc = waveOutWrite(wout, &hdr[1], sizeof(hdr[1]));
    ok(rc == MMSYSERR_NOERROR, "waveOutWrite failed: %s\n", wave_out_error(rc));

    wait = WaitForSingleObject(hevent, 1000);
    ok(wait == WAIT_OBJECT_0, "header 1 callback missed\n");

    wait = WaitForSingleObject(hevent, 1000);
    ok(wait == WAIT_OBJECT_0, "header 2 callback missed\n");

    memset(&mmtime, 0, sizeof(mmtime));
    mmtime.wType = TIME_BYTES;

    rc = waveOutGetPosition(wout, &mmtime, sizeof(mmtime));
    ok(rc == MMSYSERR_NOERROR, "waveOutGetPosition failed: %s\n", wave_out_error(rc));

    /* windows behavior is inconsistent */
    ok(mmtime.u.cb == 88200 ||
            mmtime.u.cb == 88196, "after position: %u\n", mmtime.u.cb);

    rc = waveOutClose(wout);
    ok(rc == MMSYSERR_NOERROR, "waveOutClose failed: %s\n", wave_out_error(rc));

    CloseHandle(hevent);
}

static void create_wav_file(char *temp_file)
{
    WAVEFORMATEX format;
    HMMIO h;
    MMCKINFO riff_chunk, chunk;
    MMRESULT rc;
    LONG written;
    DWORD length;
    char *buffer;

    format.wFormatTag=WAVE_FORMAT_PCM;
    format.cbSize = 0;
    format.nChannels=1;
    format.wBitsPerSample=8;
    format.nSamplesPerSec=8000;
    format.nBlockAlign=format.nChannels*format.wBitsPerSample/8;
    format.nAvgBytesPerSec=format.nSamplesPerSec*format.nBlockAlign;

    h = mmioOpenA(temp_file, NULL, MMIO_ALLOCBUF | MMIO_WRITE | MMIO_CREATE);
    ok(h != NULL, "Can't open temp_file\n");

    riff_chunk.fccType = mmioFOURCC('W','A','V','E');
    riff_chunk.cksize = 0;
    rc = mmioCreateChunk(h, &riff_chunk, MMIO_CREATERIFF);
    ok(rc == MMSYSERR_NOERROR, "mmioCreateChunk failed, got %u\n", rc);

    chunk.ckid = mmioFOURCC('f','m','t',' ');
    chunk.cksize = 0;
    rc = mmioCreateChunk(h, &chunk, 0);
    ok(rc == MMSYSERR_NOERROR, "mmioCreateChunk failed, got %u\n", rc);
    written = mmioWrite(h, (char*)&format, sizeof(format));
    ok(written == sizeof(format), "mmioWrite failed, got %d\n", written);
    rc = mmioAscend(h, &chunk, 0);
    ok(rc == MMSYSERR_NOERROR, "mmioAscend failed, got %d\n", rc);

    chunk.ckid = mmioFOURCC('d','a','t','a');
    rc = mmioCreateChunk(h, &chunk, 0);
    ok(rc == MMSYSERR_NOERROR, "mmioCreateChunk failed, got %u\n", rc);
    buffer = wave_generate_silence(&format, .1, &length);
    written = mmioWrite(h, buffer, length);
    ok(written == length, "mmioWrite failed, got %d\n", written);
    rc = mmioAscend(h, &chunk, 0);
    ok(rc == MMSYSERR_NOERROR, "mmioAscend failed, got %d\n", rc);
    HeapFree(GetProcessHeap(), 0, buffer);

    rc = mmioAscend(h, &riff_chunk, 0);
    ok(rc == MMSYSERR_NOERROR, "mmioAscend failed, got %d\n", rc);

    rc = mmioClose(h, 0);
    ok(rc == MMSYSERR_NOERROR, "mmioClose failed, got %u\n", rc);
}

static void test_PlaySound(void)
{
    BOOL br;
    char test_file[MAX_PATH], temp[MAX_PATH], *exts;

    if(waveOutGetNumDevs() == 0) {
        skip("No output devices available\n");
        return;
    }

    GetTempPathA(sizeof(test_file), test_file);
    strcat(test_file, "mysound.wav");
    create_wav_file(test_file);

    br = PlaySoundA(test_file, NULL, SND_FILENAME | SND_NODEFAULT);
    ok(br, "PlaySound failed, got %d\n", br);

    /* SND_ALIAS fallbacks to SND_FILENAME */
    br = PlaySoundA(test_file, NULL, SND_ALIAS | SND_NODEFAULT);
    ok(br, "PlaySound failed, got %d\n", br);

    strcpy(temp, test_file);
    exts = strrchr(temp, '.');

    /* no extensions */
    *exts = '\0';
    br = PlaySoundA(temp, NULL, SND_FILENAME | SND_NODEFAULT);
    ok(br, "PlaySound failed, got %d\n", br);

    /* ends with a dot */
    strcpy(exts, ".");
    br = PlaySoundA(temp, NULL, SND_FILENAME | SND_NODEFAULT);
    ok(!br || broken(br), "PlaySound succeeded, got %d\n", br);

    DeleteFileA(test_file);
}

START_TEST(wave)
{
    test_multiple_waveopens();
    wave_out_tests();
    test_sndPlaySound();
    test_fragmentsize();
    test_PlaySound();
}
