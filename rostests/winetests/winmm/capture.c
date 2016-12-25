/*
 * Test winmm sound capture in each sound format
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
#include "winnls.h"
#include "mmddk.h"
#include "mmsystem.h"
#define NOBITMAP
#include "mmreg.h"

extern GUID KSDATAFORMAT_SUBTYPE_PCM;
extern GUID KSDATAFORMAT_SUBTYPE_IEEE_FLOAT;

#include "winmm_test.h"

static const char * wave_in_error(MMRESULT error)
{
    static char msg[1024];
    static char long_msg[1100];
    MMRESULT rc;

    rc = waveInGetErrorTextA(error, msg, sizeof(msg));
    if (rc != MMSYSERR_NOERROR)
        sprintf(long_msg, "waveInGetErrorTextA(%x) failed with error %x", error, rc);
    else
        sprintf(long_msg, "%s(%s)", mmsys_error(error), msg);
    return long_msg;
}

static void check_position(int device, HWAVEIN win, DWORD bytes,
                           LPWAVEFORMATEX pwfx )
{
    MMTIME mmtime;
    MMRESULT rc;
    DWORD returned;

    mmtime.wType = TIME_BYTES;
    rc=waveInGetPosition(win, &mmtime, sizeof(mmtime));
    ok(rc==MMSYSERR_NOERROR,
       "waveInGetPosition(%s): rc=%s\n",dev_name(device),wave_in_error(rc));
    if (mmtime.wType != TIME_BYTES && winetest_debug > 1)
        trace("waveInGetPosition(%s): TIME_BYTES not supported, returned %s\n",
              dev_name(device),wave_time_format(mmtime.wType));
    returned = time_to_bytes(&mmtime, pwfx);
    ok(returned == bytes, "waveInGetPosition(%s): returned %d bytes, "
       "should be %d\n", dev_name(device), returned, bytes);

    mmtime.wType = TIME_SAMPLES;
    rc=waveInGetPosition(win, &mmtime, sizeof(mmtime));
    ok(rc==MMSYSERR_NOERROR,
       "waveInGetPosition(%s): rc=%s\n",dev_name(device),wave_in_error(rc));
    if (mmtime.wType != TIME_SAMPLES && winetest_debug > 1)
        trace("waveInGetPosition(%s): TIME_SAMPLES not supported, "
              "returned %s\n",dev_name(device),wave_time_format(mmtime.wType));
    returned = time_to_bytes(&mmtime, pwfx);
    ok(returned == bytes, "waveInGetPosition(%s): returned %d samples, "
       "should be %d\n", dev_name(device), bytes_to_samples(returned, pwfx),
       bytes_to_samples(bytes, pwfx));

    mmtime.wType = TIME_MS;
    rc=waveInGetPosition(win, &mmtime, sizeof(mmtime));
    ok(rc==MMSYSERR_NOERROR,
       "waveInGetPosition(%s): rc=%s\n",dev_name(device),wave_in_error(rc));
    if (mmtime.wType != TIME_MS && winetest_debug > 1)
        trace("waveInGetPosition(%s): TIME_MS not supported, returned %s\n",
              dev_name(device), wave_time_format(mmtime.wType));
    returned = time_to_bytes(&mmtime, pwfx);
    ok(returned == bytes, "waveInGetPosition(%s): returned %d ms, "
       "should be %d\n", dev_name(device), bytes_to_ms(returned, pwfx),
       bytes_to_ms(bytes, pwfx));

    mmtime.wType = TIME_SMPTE;
    rc=waveInGetPosition(win, &mmtime, sizeof(mmtime));
    ok(rc==MMSYSERR_NOERROR,
       "waveInGetPosition(%s): rc=%s\n",dev_name(device),wave_in_error(rc));
    if (mmtime.wType != TIME_SMPTE && winetest_debug > 1)
        trace("waveInGetPosition(%s): TIME_SMPTE not supported, returned %s\n",
              dev_name(device),wave_time_format(mmtime.wType));
    returned = time_to_bytes(&mmtime, pwfx);
    ok(returned == bytes, "waveInGetPosition(%s): SMPTE test failed\n",
       dev_name(device));

    mmtime.wType = TIME_MIDI;
    rc=waveInGetPosition(win, &mmtime, sizeof(mmtime));
    ok(rc==MMSYSERR_NOERROR,
       "waveInGetPosition(%s): rc=%s\n",dev_name(device),wave_in_error(rc));
    if (mmtime.wType != TIME_MIDI && winetest_debug > 1)
        trace("waveInGetPosition(%s): TIME_MIDI not supported, returned %s\n",
              dev_name(device),wave_time_format(mmtime.wType));
    returned = time_to_bytes(&mmtime, pwfx);
    ok(returned == bytes, "waveInGetPosition(%s): MIDI test failed\n",
       dev_name(device));

    mmtime.wType = TIME_TICKS;
    rc=waveInGetPosition(win, &mmtime, sizeof(mmtime));
    ok(rc==MMSYSERR_NOERROR,
       "waveInGetPosition(%s): rc=%s\n",dev_name(device),wave_in_error(rc));
    if (mmtime.wType != TIME_TICKS && winetest_debug > 1)
        trace("waveInGetPosition(%s): TIME_TICKS not supported, returned %s\n",
              dev_name(device),wave_time_format(mmtime.wType));
    returned = time_to_bytes(&mmtime, pwfx);
    ok(returned == bytes, "waveInGetPosition(%s): TICKS test failed\n",
       dev_name(device));
}

static void wave_in_test_deviceIn(int device, WAVEFORMATEX *pwfx, DWORD format, DWORD flags,
        WAVEINCAPSA *pcaps)
{
    HWAVEIN win;
    HANDLE hevent = CreateEventW(NULL, FALSE, FALSE, NULL);
    WAVEHDR frag;
    MMRESULT rc;
    DWORD res;
    MMTIME mmt;
    WORD nChannels = pwfx->nChannels;
    WORD wBitsPerSample = pwfx->wBitsPerSample;
    DWORD nSamplesPerSec = pwfx->nSamplesPerSec;

    win=NULL;
    rc=waveInOpen(&win,device,pwfx,(DWORD_PTR)hevent,0,CALLBACK_EVENT|flags);
    /* Note: Win9x doesn't know WAVE_FORMAT_DIRECT */
    ok(rc==MMSYSERR_NOERROR || rc==MMSYSERR_BADDEVICEID ||
       rc==MMSYSERR_NOTENABLED || rc==MMSYSERR_NODRIVER ||
       rc==MMSYSERR_ALLOCATED ||
       ((rc==WAVERR_BADFORMAT || rc==MMSYSERR_NOTSUPPORTED) &&
       (flags & WAVE_FORMAT_DIRECT) && !(pcaps->dwFormats & format)) ||
       ((rc==WAVERR_BADFORMAT || rc==MMSYSERR_NOTSUPPORTED) &&
       (!(flags & WAVE_FORMAT_DIRECT) || (flags & WAVE_MAPPED)) &&
       !(pcaps->dwFormats & format)) ||
       (rc==MMSYSERR_INVALFLAG && (flags & WAVE_FORMAT_DIRECT)),
       "waveInOpen(%s): format=%dx%2dx%d flags=%x(%s) rc=%s\n",
       dev_name(device),pwfx->nSamplesPerSec,pwfx->wBitsPerSample,
       pwfx->nChannels,CALLBACK_EVENT|flags,
       wave_open_flags(CALLBACK_EVENT|flags),wave_in_error(rc));
    if ((rc==WAVERR_BADFORMAT || rc==MMSYSERR_NOTSUPPORTED) &&
       (flags & WAVE_FORMAT_DIRECT) && (pcaps->dwFormats & format))
        trace(" Reason: The device lists this format as supported in its "
              "capabilities but opening it failed.\n");
    if ((rc==WAVERR_BADFORMAT || rc==MMSYSERR_NOTSUPPORTED) &&
       !(pcaps->dwFormats & format))
        trace("waveInOpen(%s): format=%dx%2dx%d %s rc=%s failed but format "
              "not supported so OK.\n",dev_name(device),pwfx->nSamplesPerSec,
              pwfx->wBitsPerSample,pwfx->nChannels,
              flags & WAVE_FORMAT_DIRECT ? "flags=WAVE_FORMAT_DIRECT" :
              flags & WAVE_MAPPED ? "flags=WAVE_MAPPED" : "", mmsys_error(rc));
    if (rc!=MMSYSERR_NOERROR) {
        CloseHandle(hevent);
        return;
    }
    res=WaitForSingleObject(hevent,1000);
    ok(res==WAIT_OBJECT_0,"WaitForSingleObject failed for open\n");

    ok(pwfx->nChannels==nChannels &&
       pwfx->wBitsPerSample==wBitsPerSample &&
       pwfx->nSamplesPerSec==nSamplesPerSec,
       "got the wrong format: %dx%2dx%d instead of %dx%2dx%d\n",
       pwfx->nSamplesPerSec, pwfx->wBitsPerSample,
       pwfx->nChannels, nSamplesPerSec, wBitsPerSample, nChannels);

    /* Check that the position is 0 at start */
    check_position(device, win, 0, pwfx);

    frag.lpData=HeapAlloc(GetProcessHeap(), 0, pwfx->nAvgBytesPerSec);
    frag.dwBufferLength=pwfx->nAvgBytesPerSec;
    frag.dwBytesRecorded=0;
    frag.dwUser=0;
    frag.dwFlags=0;
    frag.dwLoops=0;
    frag.lpNext=0;

    rc=waveInPrepareHeader(win, &frag, sizeof(frag));
    ok(rc==MMSYSERR_NOERROR, "waveInPrepareHeader(%s): rc=%s\n",
       dev_name(device),wave_in_error(rc));
    ok(frag.dwFlags&WHDR_PREPARED,"waveInPrepareHeader(%s): prepared flag "
       "not set\n",dev_name(device));

    if (winetest_interactive && rc==MMSYSERR_NOERROR) {
        trace("Recording for 1 second at %5dx%2dx%d %s %s\n",
              pwfx->nSamplesPerSec, pwfx->wBitsPerSample,pwfx->nChannels,
              get_format_str(pwfx->wFormatTag),
              flags & WAVE_FORMAT_DIRECT ? "WAVE_FORMAT_DIRECT" :
              flags & WAVE_MAPPED ? "WAVE_MAPPED" : "");
        rc=waveInAddBuffer(win, &frag, sizeof(frag));
        ok(rc==MMSYSERR_NOERROR,"waveInAddBuffer(%s): rc=%s\n",
           dev_name(device),wave_in_error(rc));

        /* Check that the position is 0 at start */
        check_position(device, win, 0, pwfx);

        rc=waveInStart(win);
        ok(rc==MMSYSERR_NOERROR,"waveInStart(%s): rc=%s\n",
           dev_name(device),wave_in_error(rc));

        res = WaitForSingleObject(hevent,1200);
        ok(res==WAIT_OBJECT_0,"WaitForSingleObject failed for header\n");
        ok(frag.dwFlags&WHDR_DONE,"WHDR_DONE not set in frag.dwFlags\n");
        ok(frag.dwBytesRecorded==pwfx->nAvgBytesPerSec,
           "frag.dwBytesRecorded=%d, should=%d\n",
           frag.dwBytesRecorded,pwfx->nAvgBytesPerSec);

        mmt.wType = TIME_BYTES;
        rc=waveInGetPosition(win, &mmt, sizeof(mmt));
        ok(rc==MMSYSERR_NOERROR,"waveInGetPosition(%s): rc=%s\n",
           dev_name(device),wave_in_error(rc));
        ok(mmt.wType == TIME_BYTES, "doesn't support TIME_BYTES: %u\n", mmt.wType);
        ok(mmt.u.cb == frag.dwBytesRecorded, "Got wrong position: %u\n", mmt.u.cb);

        /* stop playing on error */
        if (res!=WAIT_OBJECT_0) {
            rc=waveInStop(win);
            ok(rc==MMSYSERR_NOERROR,
               "waveInStop(%s): rc=%s\n",dev_name(device),wave_in_error(rc));
        }
    }

    rc=waveInUnprepareHeader(win, &frag, sizeof(frag));
    ok(rc==MMSYSERR_NOERROR,"waveInUnprepareHeader(%s): rc=%s\n",
       dev_name(device),wave_in_error(rc));

    rc=waveInClose(win);
    ok(rc==MMSYSERR_NOERROR,
       "waveInClose(%s): rc=%s\n",dev_name(device),wave_in_error(rc));
    res=WaitForSingleObject(hevent,1000);
    ok(res==WAIT_OBJECT_0,"WaitForSingleObject failed for close\n");

    if (winetest_interactive)
    {
        /*
         * Now play back what we recorded
         */
        HWAVEOUT wout;

        trace("Playing back recorded sound\n");
        rc=waveOutOpen(&wout,WAVE_MAPPER,pwfx,(DWORD_PTR)hevent,0,CALLBACK_EVENT);
        ok(rc==MMSYSERR_NOERROR || rc==MMSYSERR_BADDEVICEID ||
           rc==MMSYSERR_NOTENABLED || rc==MMSYSERR_NODRIVER ||
           rc==MMSYSERR_ALLOCATED ||
           ((rc==WAVERR_BADFORMAT || rc==MMSYSERR_NOTSUPPORTED) &&
            !(pcaps->dwFormats & format)),
           "waveOutOpen(%s) format=%dx%2dx%d flags=%x(%s) rc=%s\n",
           dev_name(device),pwfx->nSamplesPerSec,pwfx->wBitsPerSample,
           pwfx->nChannels,CALLBACK_EVENT|flags,
           wave_open_flags(CALLBACK_EVENT),wave_out_error(rc));
        if (rc==MMSYSERR_NOERROR)
        {
            rc=waveOutPrepareHeader(wout, &frag, sizeof(frag));
            ok(rc==MMSYSERR_NOERROR,"waveOutPrepareHeader(%s): rc=%s\n",
               dev_name(device),wave_out_error(rc));

            if (rc==MMSYSERR_NOERROR)
            {
                WaitForSingleObject(hevent,INFINITE);
                rc=waveOutWrite(wout, &frag, sizeof(frag));
                ok(rc==MMSYSERR_NOERROR,"waveOutWrite(%s): rc=%s\n",
                   dev_name(device),wave_out_error(rc));
                WaitForSingleObject(hevent,INFINITE);

                rc=waveOutUnprepareHeader(wout, &frag, sizeof(frag));
                ok(rc==MMSYSERR_NOERROR,"waveOutUnprepareHeader(%s): rc=%s\n",
                   dev_name(device),wave_out_error(rc));
            }
            rc=waveOutClose(wout);
            ok(rc==MMSYSERR_NOERROR,"waveOutClose(%s): rc=%s\n",
               dev_name(device),wave_out_error(rc));
        }
        else
            trace("Unable to play back the recorded sound\n");
    }

    HeapFree(GetProcessHeap(), 0, frag.lpData);
    CloseHandle(hevent);
}

static void wave_in_test_device(UINT_PTR device)
{
    WAVEINCAPSA capsA;
    WAVEINCAPSW capsW;
    WAVEFORMATEX format;
    WAVEFORMATEXTENSIBLE wfex;
    HWAVEIN win;
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

    rc=waveInGetDevCapsA(device,&capsA,sizeof(capsA));
    ok(rc==MMSYSERR_NOERROR || rc==MMSYSERR_BADDEVICEID ||
       rc==MMSYSERR_NODRIVER,
       "waveInGetDevCapsA(%s): failed to get capabilities: rc=%s\n",
       dev_name(device),wave_in_error(rc));
    if (rc==MMSYSERR_BADDEVICEID || rc==MMSYSERR_NODRIVER)
        return;

    rc=waveInGetDevCapsW(device,&capsW,sizeof(capsW));
    ok(rc==MMSYSERR_NOERROR || rc==MMSYSERR_NOTSUPPORTED,
       "waveInGetDevCapsW(%s): MMSYSERR_NOERROR or MMSYSERR_NOTSUPPORTED "
       "expected, got %s\n",dev_name(device),wave_in_error(rc));

    rc=waveInGetDevCapsA(device,NULL,sizeof(capsA));
    ok(rc==MMSYSERR_INVALPARAM,
       "waveInGetDevCapsA(%s): MMSYSERR_INVALPARAM expected, got %s\n",
       dev_name(device),wave_in_error(rc));

    rc=waveInGetDevCapsW(device,NULL,sizeof(capsW));
    ok(rc==MMSYSERR_INVALPARAM || rc==MMSYSERR_NOTSUPPORTED,
       "waveInGetDevCapsW(%s): MMSYSERR_INVALPARAM or MMSYSERR_NOTSUPPORTED "
       "expected, got %s\n",dev_name(device),wave_in_error(rc));

    if (0)
    {
    /* FIXME: this works on windows but crashes wine */
    rc=waveInGetDevCapsA(device,(LPWAVEINCAPSA)1,sizeof(capsA));
    ok(rc==MMSYSERR_INVALPARAM,
       "waveInGetDevCapsA(%s): MMSYSERR_INVALPARAM expected, got %s\n",
       dev_name(device),wave_in_error(rc));

    rc=waveInGetDevCapsW(device,(LPWAVEINCAPSW)1,sizeof(capsW));
    ok(rc==MMSYSERR_INVALPARAM ||  rc==MMSYSERR_NOTSUPPORTED,
       "waveInGetDevCapsW(%s): MMSYSERR_INVALPARAM or MMSYSERR_NOTSUPPORTED "
       "expected, got %s\n",dev_name(device),wave_in_error(rc));
    }

    rc=waveInGetDevCapsA(device,&capsA,4);
    ok(rc==MMSYSERR_NOERROR,
       "waveInGetDevCapsA(%s): MMSYSERR_NOERROR expected, got %s\n",
       dev_name(device),wave_in_error(rc));

    rc=waveInGetDevCapsW(device,&capsW,4);
    ok(rc==MMSYSERR_NOERROR || rc==MMSYSERR_NOTSUPPORTED ||
       rc==MMSYSERR_INVALPARAM, /* Vista, W2K8 */
       "waveInGetDevCapsW(%s): unexpected return value %s\n",
       dev_name(device),wave_in_error(rc));

    nameA=NULL;
    rc=waveInMessage((HWAVEIN)device, DRV_QUERYDEVICEINTERFACESIZE,
                     (DWORD_PTR)&size, 0);
    ok(rc==MMSYSERR_NOERROR || rc==MMSYSERR_INVALPARAM ||
       rc==MMSYSERR_NOTSUPPORTED,
       "waveInMessage(%s): failed to get interface size: rc=%s\n",
       dev_name(device),wave_in_error(rc));
    if (rc==MMSYSERR_NOERROR) {
        nameW = HeapAlloc(GetProcessHeap(), 0, size);
        rc=waveInMessage((HWAVEIN)device, DRV_QUERYDEVICEINTERFACE,
                         (DWORD_PTR)nameW, size);
        ok(rc==MMSYSERR_NOERROR,"waveInMessage(%s): failed to get interface "
           "name: rc=%s\n",dev_name(device),wave_in_error(rc));
        ok(lstrlenW(nameW)+1==size/sizeof(WCHAR),
           "got an incorrect size %d\n", size);
        if (rc==MMSYSERR_NOERROR) {
            nameA = HeapAlloc(GetProcessHeap(), 0, size/sizeof(WCHAR));
            WideCharToMultiByte(CP_ACP, 0, nameW, size/sizeof(WCHAR),
                                nameA, size/sizeof(WCHAR), NULL, NULL);
        }
        HeapFree(GetProcessHeap(), 0, nameW);
    } else if (rc==MMSYSERR_NOTSUPPORTED) {
        nameA=HeapAlloc(GetProcessHeap(), 0, sizeof("not supported"));
        strcpy(nameA, "not supported");
    }

    trace("  %s: \"%s\" (%s) %d.%d (%d:%d)\n",dev_name(device),capsA.szPname,
          (nameA?nameA:"failed"),capsA.vDriverVersion >> 8,
          capsA.vDriverVersion & 0xff,capsA.wMid,capsA.wPid);
    trace("     channels=%d formats=%05x\n",
          capsA.wChannels,capsA.dwFormats);

    HeapFree(GetProcessHeap(), 0, nameA);

    for (f=0;f<NB_WIN_FORMATS;f++) {
        format.wFormatTag=WAVE_FORMAT_PCM;
        format.nChannels=win_formats[f][3];
        format.wBitsPerSample=win_formats[f][2];
        format.nSamplesPerSec=win_formats[f][1];
        format.nBlockAlign=format.nChannels*format.wBitsPerSample/8;
        format.nAvgBytesPerSec=format.nSamplesPerSec*format.nBlockAlign;
        format.cbSize=0;
        wave_in_test_deviceIn(device,&format,win_formats[f][0],0, &capsA);
        if (device != WAVE_MAPPER) {
            wave_in_test_deviceIn(device,&format,win_formats[f][0],
                                  WAVE_FORMAT_DIRECT, &capsA);
            wave_in_test_deviceIn(device,&format,win_formats[f][0],
                                  WAVE_MAPPED, &capsA);
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
            wave_in_test_deviceIn(device,pwfx,WAVE_FORMAT_2M08,0, &capsA);
            if (device != WAVE_MAPPER) {
                wave_in_test_deviceIn(device,pwfx,WAVE_FORMAT_2M08,
                    WAVE_FORMAT_DIRECT, &capsA);
                wave_in_test_deviceIn(device,pwfx,WAVE_FORMAT_2M08,
                                      WAVE_MAPPED, &capsA);
            }
        }
        VirtualFree(twoPages, 2 * dwPageSize, MEM_RELEASE);
    }

    /* test non PCM formats */
    format.wFormatTag=WAVE_FORMAT_MULAW;
    format.nChannels=1;
    format.wBitsPerSample=8;
    format.nSamplesPerSec=8000;
    format.nBlockAlign=format.nChannels*format.wBitsPerSample/8;
    format.nAvgBytesPerSec=format.nSamplesPerSec*format.nBlockAlign;
    format.cbSize=0;
    rc=waveInOpen(&win,device,&format,0,0,CALLBACK_NULL|WAVE_FORMAT_DIRECT);
    ok(rc==MMSYSERR_NOERROR || rc==WAVERR_BADFORMAT ||
       rc==MMSYSERR_INVALFLAG || rc==MMSYSERR_INVALPARAM ||
       rc==MMSYSERR_ALLOCATED,
       "waveInOpen(%s): returned: %s\n",dev_name(device),wave_in_error(rc));
    if (rc==MMSYSERR_NOERROR) {
        waveInClose(win);
        wave_in_test_deviceIn(device,&format,0,0,&capsA);
    } else
        trace("waveInOpen(%s): WAVE_FORMAT_MULAW not supported\n",
              dev_name(device));

    format.wFormatTag=WAVE_FORMAT_ADPCM;
    format.nChannels=2;
    format.wBitsPerSample=4;
    format.nSamplesPerSec=22050;
    format.nBlockAlign=format.nChannels*format.wBitsPerSample/8;
    format.nAvgBytesPerSec=format.nSamplesPerSec*format.nBlockAlign;
    format.cbSize=0;
    rc=waveInOpen(&win,device,&format,0,0,CALLBACK_NULL|WAVE_FORMAT_DIRECT);
    ok(rc==MMSYSERR_NOERROR || rc==WAVERR_BADFORMAT ||
       rc==MMSYSERR_INVALFLAG || rc==MMSYSERR_INVALPARAM ||
       rc==MMSYSERR_ALLOCATED,
       "waveInOpen(%s): returned: %s\n",dev_name(device),wave_in_error(rc));
    if (rc==MMSYSERR_NOERROR) {
        waveInClose(win);
        wave_in_test_deviceIn(device,&format,0,0,&capsA);
    } else
        trace("waveInOpen(%s): WAVE_FORMAT_ADPCM not supported\n",
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
    rc=waveInOpen(&win,device,&wfex.Format,0,0,
                  CALLBACK_NULL|WAVE_FORMAT_DIRECT);
    ok(rc==MMSYSERR_NOERROR || rc==WAVERR_BADFORMAT ||
       rc==MMSYSERR_INVALFLAG || rc==MMSYSERR_INVALPARAM ||
       rc==MMSYSERR_ALLOCATED,
       "waveInOpen(%s): returned: %s\n",dev_name(device),wave_in_error(rc));
    if (rc==MMSYSERR_NOERROR) {
        waveInClose(win);
        wave_in_test_deviceIn(device,&wfex.Format,0,0,&capsA);
    } else
        trace("waveInOpen(%s): WAVE_FORMAT_EXTENSIBLE not supported\n",
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
    rc=waveInOpen(&win,device,&wfex.Format,0,0,
                  CALLBACK_NULL|WAVE_FORMAT_DIRECT);
    ok(rc==MMSYSERR_NOERROR || rc==WAVERR_BADFORMAT ||
       rc==MMSYSERR_INVALFLAG || rc==MMSYSERR_INVALPARAM ||
       rc==MMSYSERR_ALLOCATED,
       "waveInOpen(%s): returned: %s\n",dev_name(device),wave_in_error(rc));
    if (rc==MMSYSERR_NOERROR) {
        waveInClose(win);
        wave_in_test_deviceIn(device,&wfex.Format,0,0,&capsA);
    } else
        trace("waveInOpen(%s): 4 channels not supported\n",
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
    rc=waveInOpen(&win,device,&wfex.Format,0,0,
                  CALLBACK_NULL|WAVE_FORMAT_DIRECT);
    ok(rc==MMSYSERR_NOERROR || rc==WAVERR_BADFORMAT ||
       rc==MMSYSERR_INVALFLAG || rc==MMSYSERR_INVALPARAM ||
       rc==MMSYSERR_ALLOCATED,
       "waveInOpen(%s): returned: %s\n",dev_name(device),wave_in_error(rc));
    if (rc==MMSYSERR_NOERROR) {
        waveInClose(win);
        wave_in_test_deviceIn(device,&wfex.Format,0,0,&capsA);
    } else
        trace("waveInOpen(%s): 6 channels not supported\n",
              dev_name(device));

    if (0)
    {
    /* FIXME: ALSA doesn't like this */
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
    rc=waveInOpen(&win,device,&wfex.Format,0,0,
                  CALLBACK_NULL|WAVE_FORMAT_DIRECT);
    ok(rc==MMSYSERR_NOERROR || rc==WAVERR_BADFORMAT ||
       rc==MMSYSERR_INVALFLAG || rc==MMSYSERR_INVALPARAM ||
       rc==MMSYSERR_ALLOCATED,
       "waveInOpen(%s): returned: %s\n",dev_name(device),wave_in_error(rc));
    if (rc==MMSYSERR_NOERROR) {
        waveInClose(win);
        wave_in_test_deviceIn(device,&wfex.Format,0,0,&capsA);
    } else
        trace("waveInOpen(%s): 24 bit samples not supported\n",
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
    rc=waveInOpen(&win,device,&wfex.Format,0,0,
                  CALLBACK_NULL|WAVE_FORMAT_DIRECT);
    ok(rc==MMSYSERR_NOERROR || rc==WAVERR_BADFORMAT ||
       rc==MMSYSERR_INVALFLAG || rc==MMSYSERR_INVALPARAM ||
       rc==MMSYSERR_ALLOCATED,
       "waveInOpen(%s): returned: %s\n",dev_name(device),wave_in_error(rc));
    if (rc==MMSYSERR_NOERROR) {
        waveInClose(win);
        wave_in_test_deviceIn(device,&wfex.Format,0,0,&capsA);
    } else
        trace("waveInOpen(%s): 32 bit samples not supported\n",
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
    rc=waveInOpen(&win,device,&wfex.Format,0,0,
                  CALLBACK_NULL|WAVE_FORMAT_DIRECT);
    ok(rc==MMSYSERR_NOERROR || rc==WAVERR_BADFORMAT ||
       rc==MMSYSERR_INVALFLAG || rc==MMSYSERR_INVALPARAM ||
       rc==MMSYSERR_ALLOCATED,
       "waveInOpen(%s): returned: %s\n",dev_name(device),wave_in_error(rc));
    if (rc==MMSYSERR_NOERROR) {
        waveInClose(win);
        wave_in_test_deviceIn(device,&wfex.Format,0,0,&capsA);
    } else
        trace("waveInOpen(%s): 32 bit float samples not supported\n",
              dev_name(device));
}

static void wave_in_tests(void)
{
    WAVEINCAPSA capsA;
    WAVEINCAPSW capsW;
    WAVEFORMATEX format;
    HWAVEIN win;
    MMRESULT rc;
    DWORD preferred, status;
    UINT ndev,d;

    ndev=waveInGetNumDevs();
    trace("found %d WaveIn devices\n",ndev);

    rc = waveInMessage((HWAVEIN)WAVE_MAPPER, DRVM_MAPPER_PREFERRED_GET,
            (DWORD_PTR)&preferred, (DWORD_PTR)&status);
    ok((ndev == 0 && (rc == MMSYSERR_NODRIVER || rc == MMSYSERR_BADDEVICEID)) ||
            rc == MMSYSERR_NOTSUPPORTED ||
            rc == MMSYSERR_NOERROR, "waveInMessage(DRVM_MAPPER_PREFERRED_GET) failed: %u\n", rc);

    if(rc != MMSYSERR_NOTSUPPORTED)
        ok((ndev == 0 && (preferred == -1 || broken(preferred != -1))) ||
                preferred < ndev, "Got invalid preferred device: 0x%x\n", preferred);

    rc=waveInGetDevCapsA(ndev+1,&capsA,sizeof(capsA));
    ok(rc==MMSYSERR_BADDEVICEID,
       "waveInGetDevCapsA(%s): MMSYSERR_BADDEVICEID expected, got %s\n",
       dev_name(ndev+1),wave_in_error(rc));

    rc=waveInGetDevCapsA(WAVE_MAPPER,&capsA,sizeof(capsA));
    ok(rc==MMSYSERR_NOERROR || rc==MMSYSERR_NODRIVER || (!ndev && (rc==MMSYSERR_BADDEVICEID)),
       "waveInGetDevCapsA(%s): got %s\n",dev_name(WAVE_MAPPER),wave_in_error(rc));

    rc=waveInGetDevCapsW(ndev+1,&capsW,sizeof(capsW));
    ok(rc==MMSYSERR_BADDEVICEID || rc==MMSYSERR_NOTSUPPORTED,
       "waveInGetDevCapsW(%s): MMSYSERR_BADDEVICEID or MMSYSERR_NOTSUPPORTED "
       "expected, got %s\n",dev_name(ndev+1),wave_in_error(rc));

    rc=waveInGetDevCapsW(WAVE_MAPPER,&capsW,sizeof(capsW));
    ok(rc==MMSYSERR_NOERROR || rc==MMSYSERR_NODRIVER ||
       rc==MMSYSERR_NOTSUPPORTED || (!ndev && (rc==MMSYSERR_BADDEVICEID)),
       "waveInGetDevCapsW(%s): got %s\n", dev_name(ndev+1),wave_in_error(rc));

    format.wFormatTag=WAVE_FORMAT_PCM;
    format.nChannels=2;
    format.wBitsPerSample=16;
    format.nSamplesPerSec=44100;
    format.nBlockAlign=format.nChannels*format.wBitsPerSample/8;
    format.nAvgBytesPerSec=format.nSamplesPerSec*format.nBlockAlign;
    format.cbSize=0;
    rc=waveInOpen(&win,ndev+1,&format,0,0,CALLBACK_NULL);
    ok(rc==MMSYSERR_BADDEVICEID,
       "waveInOpen(%s): MMSYSERR_BADDEVICEID expected, got %s\n",
       dev_name(ndev+1),wave_in_error(rc));

    for (d=0;d<ndev;d++)
        wave_in_test_device(d);

    if (ndev>0)
        wave_in_test_device(WAVE_MAPPER);
}

START_TEST(capture)
{
    wave_in_tests();
}
