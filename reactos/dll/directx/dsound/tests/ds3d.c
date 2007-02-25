/*
 * Tests the panning and 3D functions of DirectSound
 *
 * Part of this test involves playing test tones. But this only makes
 * sense if someone is going to carefully listen to it, and would only
 * bother everyone else.
 * So this is only done if the test is being run in interactive mode.
 *
 * Copyright (c) 2002-2004 Francois Gouget
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#define NONAMELESSSTRUCT
#define NONAMELESSUNION
#include <windows.h>

#include <math.h>

#include "wine/test.h"
#include "dsound.h"
#include "dxerr8.h"

#include "dsound_test.h"

#define PI 3.14159265358979323846
char* wave_generate_la(WAVEFORMATEX* wfx, double duration, DWORD* size)
{
    int i;
    int nb_samples;
    char* buf;
    char* b;

    nb_samples=(int)(duration*wfx->nSamplesPerSec);
    *size=nb_samples*wfx->nBlockAlign;
    b=buf=malloc(*size);
    for (i=0;i<nb_samples;i++) {
        double y=sin(440.0*2*PI*i/wfx->nSamplesPerSec);
        if (wfx->wBitsPerSample==8) {
            unsigned char sample=(unsigned char)((double)127.5*(y+1.0));
            *b++=sample;
            if (wfx->nChannels==2)
                *b++=sample;
        } else {
            signed short sample=(signed short)((double)32767.5*y-0.5);
            b[0]=sample & 0xff;
            b[1]=sample >> 8;
            b+=2;
            if (wfx->nChannels==2) {
                b[0]=sample & 0xff;
                b[1]=sample >> 8;
                b+=2;
            }
        }
    }
    return buf;
}

const char * getDSBCAPS(DWORD xmask) {
    static struct {
        DWORD   mask;
        const char    *name;
    } flags[] = {
#define FE(x) { x, #x },
        FE(DSBCAPS_PRIMARYBUFFER)
        FE(DSBCAPS_STATIC)
        FE(DSBCAPS_LOCHARDWARE)
        FE(DSBCAPS_LOCSOFTWARE)
        FE(DSBCAPS_CTRL3D)
        FE(DSBCAPS_CTRLFREQUENCY)
        FE(DSBCAPS_CTRLPAN)
        FE(DSBCAPS_CTRLVOLUME)
        FE(DSBCAPS_CTRLPOSITIONNOTIFY)
        FE(DSBCAPS_STICKYFOCUS)
        FE(DSBCAPS_GLOBALFOCUS)
        FE(DSBCAPS_GETCURRENTPOSITION2)
        FE(DSBCAPS_MUTE3DATMAXDISTANCE)
#undef FE
    };
    static char buffer[512];
    unsigned int i;
    BOOL first = TRUE;

    buffer[0] = 0;

    for (i=0;i<sizeof(flags)/sizeof(flags[0]);i++) {
        if ((flags[i].mask & xmask) == flags[i].mask) {
            if (first)
                first = FALSE;
            else
                strcat(buffer, "|");
            strcat(buffer, flags[i].name);
        }
    }

    return buffer;
}

HWND get_hwnd()
{
    HWND hwnd=GetForegroundWindow();
    if (!hwnd)
        hwnd=GetDesktopWindow();
    return hwnd;
}

void init_format(WAVEFORMATEX* wfx, int format, int rate, int depth,
                 int channels)
{
    wfx->wFormatTag=format;
    wfx->nChannels=channels;
    wfx->wBitsPerSample=depth;
    wfx->nSamplesPerSec=rate;
    wfx->nBlockAlign=wfx->nChannels*wfx->wBitsPerSample/8;
    /* FIXME: Shouldn't this test be if (format!=WAVE_FORMAT_PCM) */
    if (wfx->nBlockAlign==0)
    {
        /* align compressed formats to byte boundary */
        wfx->nBlockAlign=1;
    }
    wfx->nAvgBytesPerSec=wfx->nSamplesPerSec*wfx->nBlockAlign;
    wfx->cbSize=0;
}

typedef struct {
    char* wave;
    DWORD wave_len;

    LPDIRECTSOUNDBUFFER dsbo;
    LPWAVEFORMATEX wfx;
    DWORD buffer_size;
    DWORD written;
    DWORD played;
    DWORD offset;
} play_state_t;

static int buffer_refill(play_state_t* state, DWORD size)
{
    LPVOID ptr1,ptr2;
    DWORD len1,len2;
    HRESULT rc;

    if (size>state->wave_len-state->written)
        size=state->wave_len-state->written;

    rc=IDirectSoundBuffer_Lock(state->dsbo,state->offset,size,
                               &ptr1,&len1,&ptr2,&len2,0);
    ok(rc==DS_OK,"IDirectSoundBuffer_Lock() failed: %s\n",
       DXGetErrorString8(rc));
    if (rc!=DS_OK)
        return -1;

    memcpy(ptr1,state->wave+state->written,len1);
    state->written+=len1;
    if (ptr2!=NULL) {
        memcpy(ptr2,state->wave+state->written,len2);
        state->written+=len2;
    }
    state->offset=state->written % state->buffer_size;
    rc=IDirectSoundBuffer_Unlock(state->dsbo,ptr1,len1,ptr2,len2);
    ok(rc==DS_OK,"IDirectSoundBuffer_Unlock() failed: %s\n",
       DXGetErrorString8(rc));
    if (rc!=DS_OK)
        return -1;
    return size;
}

static int buffer_silence(play_state_t* state, DWORD size)
{
    LPVOID ptr1,ptr2;
    DWORD len1,len2;
    HRESULT rc;
    BYTE s;

    rc=IDirectSoundBuffer_Lock(state->dsbo,state->offset,size,
                               &ptr1,&len1,&ptr2,&len2,0);
    ok(rc==DS_OK,"IDirectSoundBuffer_Lock() failed: %s\n",
       DXGetErrorString8(rc));
    if (rc!=DS_OK)
        return -1;

    s=(state->wfx->wBitsPerSample==8?0x80:0);
    memset(ptr1,s,len1);
    if (ptr2!=NULL) {
        memset(ptr2,s,len2);
    }
    state->offset=(state->offset+size) % state->buffer_size;
    rc=IDirectSoundBuffer_Unlock(state->dsbo,ptr1,len1,ptr2,len2);
    ok(rc==DS_OK,"IDirectSoundBuffer_Unlock() failed: %s\n",
       DXGetErrorString8(rc));
    if (rc!=DS_OK)
        return -1;
    return size;
}

static int buffer_service(play_state_t* state)
{
    DWORD last_play_pos,play_pos,buf_free;
    HRESULT rc;

    rc=IDirectSoundBuffer_GetCurrentPosition(state->dsbo,&play_pos,NULL);
    ok(rc==DS_OK,"IDirectSoundBuffer_GetCurrentPosition() failed: %s\n",
       DXGetErrorString8(rc));
    if (rc!=DS_OK) {
        goto STOP;
    }

    /* Update the amount played */
    last_play_pos=state->played % state->buffer_size;
    if (play_pos<last_play_pos)
        state->played+=state->buffer_size-last_play_pos+play_pos;
    else
        state->played+=play_pos-last_play_pos;

    if (winetest_debug > 1)
        trace("buf size=%ld last_play_pos=%ld play_pos=%ld played=%ld / %ld\n",
              state->buffer_size,last_play_pos,play_pos,state->played,
              state->wave_len);

    if (state->played>state->wave_len)
    {
        /* Everything has been played */
        goto STOP;
    }

    /* Refill the buffer */
    if (state->offset<=play_pos)
        buf_free=play_pos-state->offset;
    else
        buf_free=state->buffer_size-state->offset+play_pos;

    if (winetest_debug > 1)
        trace("offset=%ld free=%ld written=%ld / %ld\n",
              state->offset,buf_free,state->written,state->wave_len);
    if (buf_free==0)
        return 1;

    if (state->written<state->wave_len)
    {
        int w=buffer_refill(state,buf_free);
        if (w==-1)
            goto STOP;
        buf_free-=w;
        if (state->written==state->wave_len && winetest_debug > 1)
            trace("last sound byte at %ld\n",
                  (state->written % state->buffer_size));
    }

    if (buf_free>0) {
        /* Fill with silence */
        if (winetest_debug > 1)
            trace("writing %ld bytes of silence\n",buf_free);
        if (buffer_silence(state,buf_free)==-1)
            goto STOP;
    }
    return 1;

STOP:
    if (winetest_debug > 1)
        trace("stopping playback\n");
    rc=IDirectSoundBuffer_Stop(state->dsbo);
    ok(rc==DS_OK,"IDirectSoundBuffer_Stop() failed: %s\n",
       DXGetErrorString8(rc));
    return 0;
}

void test_buffer(LPDIRECTSOUND dso, LPDIRECTSOUNDBUFFER dsbo,
                 BOOL is_primary, BOOL set_volume, LONG volume,
                 BOOL set_pan, LONG pan, BOOL play, double duration,
                 BOOL buffer3d, LPDIRECTSOUND3DLISTENER listener,
                 BOOL move_listener, BOOL move_sound,
                 BOOL set_frequency, DWORD frequency)
{
    HRESULT rc;
    DSBCAPS dsbcaps;
    WAVEFORMATEX wfx,wfx2;
    DWORD size,status,freq;
    int ref;

    if (set_frequency) {
        rc=IDirectSoundBuffer_SetFrequency(dsbo,frequency);
        ok(rc==DS_OK||rc==DSERR_CONTROLUNAVAIL,
           "IDirectSoundBuffer_SetFrequency() failed to set frequency "
           "%s\n",DXGetErrorString8(rc));
        if (rc!=DS_OK)
            return;
    }

    /* DSOUND: Error: Invalid caps pointer */
    rc=IDirectSoundBuffer_GetCaps(dsbo,0);
    ok(rc==DSERR_INVALIDPARAM,"IDirectSoundBuffer_GetCaps() should have "
       "returned DSERR_INVALIDPARAM, returned: %s\n",DXGetErrorString8(rc));

    ZeroMemory(&dsbcaps, sizeof(dsbcaps));

    /* DSOUND: Error: Invalid caps pointer */
    rc=IDirectSoundBuffer_GetCaps(dsbo,&dsbcaps);
    ok(rc==DSERR_INVALIDPARAM,"IDirectSoundBuffer_GetCaps() should have "
       "returned DSERR_INVALIDPARAM, returned: %s\n",DXGetErrorString8(rc));

    dsbcaps.dwSize=sizeof(dsbcaps);
    rc=IDirectSoundBuffer_GetCaps(dsbo,&dsbcaps);
    ok(rc==DS_OK,"IDirectSoundBuffer_GetCaps() failed: %s\n",
       DXGetErrorString8(rc));
    if (rc==DS_OK && winetest_debug > 1) {
        trace("    Caps: flags=0x%08lx size=%ld\n",dsbcaps.dwFlags,
              dsbcaps.dwBufferBytes);
    }

    /* Query the format size. Note that it may not match sizeof(wfx) */
    size=0;
    rc=IDirectSoundBuffer_GetFormat(dsbo,NULL,0,&size);
    ok(rc==DS_OK && size!=0,"IDirectSoundBuffer_GetFormat() should have "
       "returned the needed size: rc=%s size=%ld\n",DXGetErrorString8(rc),size);

    rc=IDirectSoundBuffer_GetFormat(dsbo,&wfx,sizeof(wfx),NULL);
    ok(rc==DS_OK,"IDirectSoundBuffer_GetFormat() failed: %s\n",
       DXGetErrorString8(rc));
    if (rc==DS_OK && winetest_debug > 1) {
        trace("    Format: %s tag=0x%04x %ldx%dx%d avg.B/s=%ld align=%d\n",
              is_primary ? "Primary" : "Secondary",
              wfx.wFormatTag,wfx.nSamplesPerSec,wfx.wBitsPerSample,
              wfx.nChannels,wfx.nAvgBytesPerSec,wfx.nBlockAlign);
    }

    /* DSOUND: Error: Invalid frequency buffer */
    rc=IDirectSoundBuffer_GetFrequency(dsbo,0);
    ok(rc==DSERR_INVALIDPARAM,"IDirectSoundBuffer_GetFrequency() should have "
       "returned DSERR_INVALIDPARAM, returned: %s\n",DXGetErrorString8(rc));

    /* DSOUND: Error: Primary buffers don't support CTRLFREQUENCY */
    rc=IDirectSoundBuffer_GetFrequency(dsbo,&freq);
    ok((rc==DS_OK && !is_primary) || (rc==DSERR_CONTROLUNAVAIL&&is_primary) ||
       (rc==DSERR_CONTROLUNAVAIL&&!(dsbcaps.dwFlags&DSBCAPS_CTRLFREQUENCY)),
       "IDirectSoundBuffer_GetFrequency() failed: %s\n",DXGetErrorString8(rc));
    if (rc==DS_OK) {
        DWORD f = set_frequency?frequency:wfx.nSamplesPerSec;
        ok(freq==f,"The frequency returned by GetFrequency "
           "%ld does not match the format %ld\n",freq,f);
    }

    /* DSOUND: Error: Invalid status pointer */
    rc=IDirectSoundBuffer_GetStatus(dsbo,0);
    ok(rc==DSERR_INVALIDPARAM,"IDirectSoundBuffer_GetStatus() should have "
       "returned DSERR_INVALIDPARAM, returned: %s\n",DXGetErrorString8(rc));

    rc=IDirectSoundBuffer_GetStatus(dsbo,&status);
    ok(rc==DS_OK,"IDirectSoundBuffer_GetStatus() failed: %s\n",
       DXGetErrorString8(rc));
    ok(status==0,"status=0x%lx instead of 0\n",status);

    if (is_primary) {
        /* We must call SetCooperativeLevel to be allowed to call SetFormat */
        /* DSOUND: Setting DirectSound cooperative level to DSSCL_PRIORITY */
        rc=IDirectSound_SetCooperativeLevel(dso,get_hwnd(),DSSCL_PRIORITY);
        ok(rc==DS_OK,"IDirectSound_SetCooperativeLevel(DSSCL_PRIORITY) failed: "
           "%s\n",DXGetErrorString8(rc));
        if (rc!=DS_OK)
            return;

        /* DSOUND: Error: Invalid format pointer */
        rc=IDirectSoundBuffer_SetFormat(dsbo,0);
        ok(rc==DSERR_INVALIDPARAM,"IDirectSoundBuffer_SetFormat() should have "
           "returned DSERR_INVALIDPARAM, returned: %s\n",DXGetErrorString8(rc));

        init_format(&wfx2,WAVE_FORMAT_PCM,11025,16,2);
        rc=IDirectSoundBuffer_SetFormat(dsbo,&wfx2);
        ok(rc==DS_OK,"IDirectSoundBuffer_SetFormat() failed: %s\n",
           DXGetErrorString8(rc));

        /* There is no garantee that SetFormat will actually change the
	 * format to what we asked for. It depends on what the soundcard
	 * supports. So we must re-query the format.
	 */
        rc=IDirectSoundBuffer_GetFormat(dsbo,&wfx,sizeof(wfx),NULL);
        ok(rc==DS_OK,"IDirectSoundBuffer_GetFormat() failed: %s\n",
           DXGetErrorString8(rc));
        if (rc==DS_OK &&
            (wfx.wFormatTag!=wfx2.wFormatTag ||
             wfx.nSamplesPerSec!=wfx2.nSamplesPerSec ||
             wfx.wBitsPerSample!=wfx2.wBitsPerSample ||
             wfx.nChannels!=wfx2.nChannels)) {
            trace("Requested format tag=0x%04x %ldx%dx%d avg.B/s=%ld align=%d\n",
                  wfx2.wFormatTag,wfx2.nSamplesPerSec,wfx2.wBitsPerSample,
                  wfx2.nChannels,wfx2.nAvgBytesPerSec,wfx2.nBlockAlign);
            trace("Got tag=0x%04x %ldx%dx%d avg.B/s=%ld align=%d\n",
                  wfx.wFormatTag,wfx.nSamplesPerSec,wfx.wBitsPerSample,
                  wfx.nChannels,wfx.nAvgBytesPerSec,wfx.nBlockAlign);
        }

        /* Set the CooperativeLevel back to normal */
        /* DSOUND: Setting DirectSound cooperative level to DSSCL_NORMAL */
        rc=IDirectSound_SetCooperativeLevel(dso,get_hwnd(),DSSCL_NORMAL);
        ok(rc==DS_OK,"IDirectSound_SetCooperativeLevel(DSSCL_NORMAL) failed: "
           "%s\n",DXGetErrorString8(rc));
    }

    if (play) {
        play_state_t state;
        DS3DLISTENER listener_param;
        LPDIRECTSOUND3DBUFFER buffer=NULL;
        DS3DBUFFER buffer_param;
        DWORD start_time,now;

        if (winetest_interactive) {
            if (set_frequency)
                trace("    Playing %g second 440Hz tone at %ldx%dx%d with a "
                      "frequency of %ld (%ldHz)\n", duration,
                      wfx.nSamplesPerSec, wfx.wBitsPerSample, wfx.nChannels,
                      frequency, (440 * frequency) / wfx.nSamplesPerSec);
            else
                trace("    Playing %g second 440Hz tone at %ldx%dx%d\n", duration,
                      wfx.nSamplesPerSec, wfx.wBitsPerSample, wfx.nChannels);
        }

        if (is_primary) {
            /* We must call SetCooperativeLevel to be allowed to call Lock */
            /* DSOUND: Setting DirectSound cooperative level to
             * DSSCL_WRITEPRIMARY */
            rc=IDirectSound_SetCooperativeLevel(dso,get_hwnd(),
                                                DSSCL_WRITEPRIMARY);
            ok(rc==DS_OK,"IDirectSound_SetCooperativeLevel(DSSCL_WRITEPRIMARY) "
               "failed: %s\n",DXGetErrorString8(rc));
            if (rc!=DS_OK)
                return;
        }
        if (buffer3d) {
            LPDIRECTSOUNDBUFFER temp_buffer;

            rc=IDirectSoundBuffer_QueryInterface(dsbo,&IID_IDirectSound3DBuffer,
                                                 (LPVOID *)&buffer);
            ok(rc==DS_OK,"IDirectSoundBuffer_QueryInterface() failed: %s\n",
               DXGetErrorString8(rc));
            if (rc!=DS_OK)
                return;

            /* check the COM interface */
            rc=IDirectSoundBuffer_QueryInterface(dsbo, &IID_IDirectSoundBuffer,
                                                 (LPVOID *)&temp_buffer);
            ok(rc==DS_OK && temp_buffer!=NULL,
               "IDirectSoundBuffer_QueryInterface() failed: %s\n",
               DXGetErrorString8(rc));
            ok(temp_buffer==dsbo,"COM interface broken: %p != %p\n",
               temp_buffer,dsbo);
            ref=IDirectSoundBuffer_Release(temp_buffer);
            ok(ref==1,"IDirectSoundBuffer_Release() has %d references, "
               "should have 1\n",ref);

            temp_buffer=NULL;
            rc=IDirectSound3DBuffer_QueryInterface(dsbo,
                                                   &IID_IDirectSoundBuffer,
                                                   (LPVOID *)&temp_buffer);
            ok(rc==DS_OK && temp_buffer!=NULL,
               "IDirectSound3DBuffer_QueryInterface() failed: %s\n",
               DXGetErrorString8(rc));
            ok(temp_buffer==dsbo,"COM interface broken: %p != %p\n",
               temp_buffer,dsbo);
            ref=IDirectSoundBuffer_Release(temp_buffer);
            ok(ref==1,"IDirectSoundBuffer_Release() has %d references, "
               "should have 1\n",ref);

#if 0
            /* FIXME: this works on windows */
            ref=IDirectSoundBuffer_Release(dsbo);
            ok(ref==0,"IDirectSoundBuffer_Release() has %d references, "
               "should have 0\n",ref);

            rc=IDirectSound3DBuffer_QueryInterface(buffer,
                                                   &IID_IDirectSoundBuffer,
                                                   (LPVOID *)&dsbo);
            ok(rc==DS_OK && dsbo!=NULL,"IDirectSound3DBuffer_QueryInterface() "
               "failed: %s\n",DXGetErrorString8(rc));
#endif

            /* DSOUND: Error: Invalid buffer */
            rc=IDirectSound3DBuffer_GetAllParameters(buffer,0);
            ok(rc==DSERR_INVALIDPARAM,"IDirectSound3DBuffer_GetAllParameters() "
               "failed: %s\n",DXGetErrorString8(rc));

            ZeroMemory(&buffer_param, sizeof(buffer_param));

            /* DSOUND: Error: Invalid buffer */
            rc=IDirectSound3DBuffer_GetAllParameters(buffer,&buffer_param);
            ok(rc==DSERR_INVALIDPARAM,"IDirectSound3DBuffer_GetAllParameters() "
               "failed: %s\n",DXGetErrorString8(rc));

            buffer_param.dwSize=sizeof(buffer_param);
            rc=IDirectSound3DBuffer_GetAllParameters(buffer,&buffer_param);
            ok(rc==DS_OK,"IDirectSound3DBuffer_GetAllParameters() failed: %s\n",
               DXGetErrorString8(rc));
        }
        if (set_volume) {
            if (dsbcaps.dwFlags & DSBCAPS_CTRLVOLUME) {
                LONG val;
                rc=IDirectSoundBuffer_GetVolume(dsbo,&val);
                ok(rc==DS_OK,"IDirectSoundBuffer_GetVolume() failed: %s\n",
                   DXGetErrorString8(rc));

                rc=IDirectSoundBuffer_SetVolume(dsbo,volume);
                ok(rc==DS_OK,"IDirectSoundBuffer_SetVolume() failed: %s\n",
                   DXGetErrorString8(rc));
            } else {
                /* DSOUND: Error: Buffer does not have CTRLVOLUME */
                rc=IDirectSoundBuffer_GetVolume(dsbo,&volume);
                ok(rc==DSERR_CONTROLUNAVAIL,"IDirectSoundBuffer_GetVolume() "
                   "should have returned DSERR_CONTROLUNAVAIL, returned: %s\n",
                   DXGetErrorString8(rc));
            }
        }

        if (set_pan) {
            if (dsbcaps.dwFlags & DSBCAPS_CTRLPAN) {
                LONG val;
                rc=IDirectSoundBuffer_GetPan(dsbo,&val);
                ok(rc==DS_OK,"IDirectSoundBuffer_GetPan() failed: %s\n",
                   DXGetErrorString8(rc));

                rc=IDirectSoundBuffer_SetPan(dsbo,pan);
                ok(rc==DS_OK,"IDirectSoundBuffer_SetPan() failed: %s\n",
                   DXGetErrorString8(rc));
            } else {
                /* DSOUND: Error: Buffer does not have CTRLPAN */
                rc=IDirectSoundBuffer_GetPan(dsbo,&pan);
                ok(rc==DSERR_CONTROLUNAVAIL,"IDirectSoundBuffer_GetPan() "
                   "should have returned DSERR_CONTROLUNAVAIL, returned: %s\n",
                   DXGetErrorString8(rc));
            }
        }

        if (set_frequency)
            state.wave=wave_generate_la(&wfx,(duration*frequency)/wfx.nSamplesPerSec,&state.wave_len);
        else
            state.wave=wave_generate_la(&wfx,duration,&state.wave_len);

        state.dsbo=dsbo;
        state.wfx=&wfx;
        state.buffer_size=dsbcaps.dwBufferBytes;
        state.played=state.written=state.offset=0;
        buffer_refill(&state,state.buffer_size);

        rc=IDirectSoundBuffer_Play(dsbo,0,0,DSBPLAY_LOOPING);
        ok(rc==DS_OK,"IDirectSoundBuffer_Play() failed: %s\n",
           DXGetErrorString8(rc));

        rc=IDirectSoundBuffer_GetStatus(dsbo,&status);
        ok(rc==DS_OK,"IDirectSoundBuffer_GetStatus() failed: %s\n",
           DXGetErrorString8(rc));
        ok(status==(DSBSTATUS_PLAYING|DSBSTATUS_LOOPING),
           "GetStatus: bad status: %lx\n",status);

        if (listener) {
            ZeroMemory(&listener_param,sizeof(listener_param));
            listener_param.dwSize=sizeof(listener_param);
            rc=IDirectSound3DListener_GetAllParameters(listener,
                                                       &listener_param);
            ok(rc==DS_OK,"IDirectSound3dListener_GetAllParameters() "
               "failed: %s\n",DXGetErrorString8(rc));
            if (move_listener) {
                listener_param.vPosition.x = -5.0;
                listener_param.vVelocity.x = 10.0/duration;
            }
            rc=IDirectSound3DListener_SetAllParameters(listener,
                                                       &listener_param,
                                                       DS3D_IMMEDIATE);
            ok(rc==DS_OK,"IDirectSound3dListener_SetPosition() failed: %s\n",
               DXGetErrorString8(rc));
        }
        if (buffer3d) {
            if (move_sound) {
                buffer_param.vPosition.x = 100.0;
                buffer_param.vVelocity.x = -200.0/duration;
            }
            buffer_param.flMinDistance = 10;
            rc=IDirectSound3DBuffer_SetAllParameters(buffer,&buffer_param,
                                                     DS3D_IMMEDIATE);
            ok(rc==DS_OK,"IDirectSound3dBuffer_SetPosition() failed: %s\n",
               DXGetErrorString8(rc));
        }

        start_time=GetTickCount();
        while (buffer_service(&state)) {
            WaitForSingleObject(GetCurrentProcess(),TIME_SLICE);
            now=GetTickCount();
            if (listener && move_listener) {
                listener_param.vPosition.x = -5.0+10.0*(now-start_time)/
                    1000/duration;
                if (winetest_debug>2)
                    trace("listener position=%g\n",listener_param.vPosition.x);
                rc=IDirectSound3DListener_SetPosition(listener,
                    listener_param.vPosition.x,listener_param.vPosition.y,
                    listener_param.vPosition.z,DS3D_IMMEDIATE);
                ok(rc==DS_OK,"IDirectSound3dListener_SetPosition() failed: "
                   "%s\n",DXGetErrorString8(rc));
            }
            if (buffer3d && move_sound) {
                buffer_param.vPosition.x = 100-200.0*(now-start_time)/
                    1000/duration;
                if (winetest_debug>2)
                    trace("sound position=%g\n",buffer_param.vPosition.x);
                rc=IDirectSound3DBuffer_SetPosition(buffer,
                    buffer_param.vPosition.x,buffer_param.vPosition.y,
                    buffer_param.vPosition.z,DS3D_IMMEDIATE);
                ok(rc==DS_OK,"IDirectSound3dBuffer_SetPosition() failed: %s\n",
                   DXGetErrorString8(rc));
            }
        }
        /* Check the sound duration was within 10% of the expected value */
        now=GetTickCount();
        ok(fabs(1000*duration-now+start_time)<=100*duration,
           "The sound played for %ld ms instead of %g ms\n",
           now-start_time,1000*duration);

        free(state.wave);
        if (is_primary) {
            /* Set the CooperativeLevel back to normal */
            /* DSOUND: Setting DirectSound cooperative level to DSSCL_NORMAL */
            rc=IDirectSound_SetCooperativeLevel(dso,get_hwnd(),DSSCL_NORMAL);
            ok(rc==DS_OK,"IDirectSound_SetCooperativeLevel(DSSCL_NORMAL) "
               "failed: %s\n",DXGetErrorString8(rc));
        }
        if (buffer3d) {
            ref=IDirectSound3DBuffer_Release(buffer);
            ok(ref==0,"IDirectSound3DBuffer_Release() has %d references, "
               "should have 0\n",ref);
        }
    }
}

static HRESULT test_secondary(LPGUID lpGuid, int play,
                              int has_3d, int has_3dbuffer,
                              int has_listener, int has_duplicate,
                              int move_listener, int move_sound)
{
    HRESULT rc;
    LPDIRECTSOUND dso=NULL;
    LPDIRECTSOUNDBUFFER primary=NULL,secondary=NULL;
    LPDIRECTSOUND3DLISTENER listener=NULL;
    DSBUFFERDESC bufdesc;
    WAVEFORMATEX wfx, wfx1;
    int ref;

    /* Create the DirectSound object */
    rc=DirectSoundCreate(lpGuid,&dso,NULL);
    ok(rc==DS_OK||rc==DSERR_NODRIVER,"DirectSoundCreate() failed: %s\n",
       DXGetErrorString8(rc));
    if (rc!=DS_OK)
        return rc;

    /* We must call SetCooperativeLevel before creating primary buffer */
    /* DSOUND: Setting DirectSound cooperative level to DSSCL_PRIORITY */
    rc=IDirectSound_SetCooperativeLevel(dso,get_hwnd(),DSSCL_PRIORITY);
    ok(rc==DS_OK,"IDirectSound_SetCooperativeLevel(DSSCL_PRIORITY) failed: "
       "%s\n",DXGetErrorString8(rc));
    if (rc!=DS_OK)
        goto EXIT;

    ZeroMemory(&bufdesc, sizeof(bufdesc));
    bufdesc.dwSize=sizeof(bufdesc);
    bufdesc.dwFlags=DSBCAPS_PRIMARYBUFFER;
    if (has_3d)
        bufdesc.dwFlags|=DSBCAPS_CTRL3D;
    else
        bufdesc.dwFlags|=(DSBCAPS_CTRLVOLUME|DSBCAPS_CTRLPAN);
    rc=IDirectSound_CreateSoundBuffer(dso,&bufdesc,&primary,NULL);
    ok((rc==DS_OK && primary!=NULL) || (rc==DSERR_CONTROLUNAVAIL),
       "IDirectSound_CreateSoundBuffer() failed to create a %sprimary buffer: "
       "%s\n",has_3d?"3D ":"", DXGetErrorString8(rc));
    if (rc==DSERR_CONTROLUNAVAIL)
        trace("  No Primary\n");
    else if (rc==DS_OK && primary!=NULL) {
        rc=IDirectSoundBuffer_GetFormat(primary,&wfx1,sizeof(wfx1),NULL);
        ok(rc==DS_OK,"IDirectSoundBuffer8_Getformat() failed: %s\n",
           DXGetErrorString8(rc));
        if (rc!=DS_OK)
            goto EXIT1;

        if (has_listener) {
            rc=IDirectSoundBuffer_QueryInterface(primary,
                                                 &IID_IDirectSound3DListener,
                                                 (void **)&listener);
            ok(rc==DS_OK && listener!=NULL,
               "IDirectSoundBuffer_QueryInterface() failed to get a 3D "
               "listener: %s\n",DXGetErrorString8(rc));
            ref=IDirectSoundBuffer_Release(primary);
            ok(ref==0,"IDirectSoundBuffer_Release() primary has %d references, "
               "should have 0\n",ref);
            if (rc==DS_OK && listener!=NULL) {
                DS3DLISTENER listener_param;
                ZeroMemory(&listener_param,sizeof(listener_param));
                /* DSOUND: Error: Invalid buffer */
                rc=IDirectSound3DListener_GetAllParameters(listener,0);
                ok(rc==DSERR_INVALIDPARAM,
                   "IDirectSound3dListener_GetAllParameters() should have "
                   "returned DSERR_INVALIDPARAM, returned: %s\n",
                   DXGetErrorString8(rc));

                /* DSOUND: Error: Invalid buffer */
                rc=IDirectSound3DListener_GetAllParameters(listener,
                                                           &listener_param);
                ok(rc==DSERR_INVALIDPARAM,
                   "IDirectSound3dListener_GetAllParameters() should have "
                   "returned DSERR_INVALIDPARAM, returned: %s\n",
                   DXGetErrorString8(rc));

                listener_param.dwSize=sizeof(listener_param);
                rc=IDirectSound3DListener_GetAllParameters(listener,
                                                           &listener_param);
                ok(rc==DS_OK,"IDirectSound3dListener_GetAllParameters() "
                   "failed: %s\n",DXGetErrorString8(rc));
            }
            else
                goto EXIT;
        }

        init_format(&wfx,WAVE_FORMAT_PCM,22050,16,2);
        secondary=NULL;
        ZeroMemory(&bufdesc, sizeof(bufdesc));
        bufdesc.dwSize=sizeof(bufdesc);
        bufdesc.dwFlags=DSBCAPS_GETCURRENTPOSITION2;
        if (has_3d)
            bufdesc.dwFlags|=DSBCAPS_CTRL3D;
        else
            bufdesc.dwFlags|=
                (DSBCAPS_CTRLFREQUENCY|DSBCAPS_CTRLVOLUME|DSBCAPS_CTRLPAN);
        bufdesc.dwBufferBytes=align(wfx.nAvgBytesPerSec*BUFFER_LEN/1000,
                                    wfx.nBlockAlign);
        bufdesc.lpwfxFormat=&wfx;
        if (winetest_interactive) {
            trace("  Testing a %s%ssecondary buffer %s%s%s%sat %ldx%dx%d "
                  "with a primary buffer at %ldx%dx%d\n",
                  has_3dbuffer?"3D ":"",
                  has_duplicate?"duplicated ":"",
                  listener!=NULL||move_sound?"with ":"",
                  move_listener?"moving ":"",
                  listener!=NULL?"listener ":"",
                  listener&&move_sound?"and moving sound ":move_sound?
                  "moving sound ":"",
                  wfx.nSamplesPerSec,wfx.wBitsPerSample,wfx.nChannels,
                  wfx1.nSamplesPerSec,wfx1.wBitsPerSample,wfx1.nChannels);
        }
        rc=IDirectSound_CreateSoundBuffer(dso,&bufdesc,&secondary,NULL);
        ok(rc==DS_OK && secondary!=NULL,"IDirectSound_CreateSoundBuffer() "
           "failed to create a %s%ssecondary buffer %s%s%s%sat %ldx%dx%d (%s): %s\n",
           has_3dbuffer?"3D ":"", has_duplicate?"duplicated ":"",
           listener!=NULL||move_sound?"with ":"", move_listener?"moving ":"",
           listener!=NULL?"listener ":"",
           listener&&move_sound?"and moving sound ":move_sound?
           "moving sound ":"",
           wfx.nSamplesPerSec,wfx.wBitsPerSample,wfx.nChannels,
           getDSBCAPS(bufdesc.dwFlags),DXGetErrorString8(rc));
        if (rc==DS_OK && secondary!=NULL) {
            if (!has_3d) {
                LONG refvol,vol,refpan,pan;

                /* Check the initial secondary buffer's volume and pan */
                rc=IDirectSoundBuffer_GetVolume(secondary,&vol);
                ok(rc==DS_OK,"IDirectSoundBuffer_GetVolume(secondary) failed: "
                   "%s\n",DXGetErrorString8(rc));
                ok(vol==0,"wrong volume for a new secondary buffer: %ld\n",vol);
                rc=IDirectSoundBuffer_GetPan(secondary,&pan);
                ok(rc==DS_OK,"IDirectSoundBuffer_GetPan(secondary) failed: "
                   "%s\n",DXGetErrorString8(rc));
                ok(pan==0,"wrong pan for a new secondary buffer: %ld\n",pan);

                /* Check that changing the secondary buffer's volume and pan
                 * does not impact the primary buffer's volume and pan
                 */
                rc=IDirectSoundBuffer_GetVolume(primary,&refvol);
                ok(rc==DS_OK,"IDirectSoundBuffer_GetVolume(primary) failed: "
                   "%s\n",DXGetErrorString8(rc));
                rc=IDirectSoundBuffer_GetPan(primary,&refpan);
                ok(rc==DS_OK,"IDirectSoundBuffer_GetPan(primary) failed: %s\n",
                   DXGetErrorString8(rc));

                rc=IDirectSoundBuffer_SetVolume(secondary,-1000);
                ok(rc==DS_OK,"IDirectSoundBuffer_SetVolume(secondary) failed: "
                   "%s\n",DXGetErrorString8(rc));
                rc=IDirectSoundBuffer_GetVolume(secondary,&vol);
                ok(rc==DS_OK,"IDirectSoundBuffer_SetVolume(secondary) failed: "
                   "%s\n",DXGetErrorString8(rc));
                ok(vol==-1000,"secondary: wrong volume %ld instead of -1000\n",
                   vol);
                rc=IDirectSoundBuffer_SetPan(secondary,-1000);
                ok(rc==DS_OK,"IDirectSoundBuffer_SetPan(secondary) failed: "
                   "%s\n",DXGetErrorString8(rc));
                rc=IDirectSoundBuffer_GetPan(secondary,&pan);
                ok(rc==DS_OK,"IDirectSoundBuffer_SetPan(secondary) failed: "
                   "%s\n",DXGetErrorString8(rc));
                ok(pan==-1000,"secondary: wrong pan %ld instead of -1000\n",
                   pan);

                rc=IDirectSoundBuffer_GetVolume(primary,&vol);
                ok(rc==DS_OK,"IDirectSoundBuffer_GetVolume(primary) failed: "
                   "%s\n",DXGetErrorString8(rc));
                ok(vol==refvol,"The primary volume changed from %ld to %ld\n",
                   refvol,vol);
                rc=IDirectSoundBuffer_GetPan(primary,&pan);
                ok(rc==DS_OK,"IDirectSoundBuffer_GetPan(primary) failed: %s\n",
                   DXGetErrorString8(rc));
                ok(pan==refpan,"The primary pan changed from %ld to %ld\n",
                   refpan,pan);

                rc=IDirectSoundBuffer_SetVolume(secondary,0);
                ok(rc==DS_OK,"IDirectSoundBuffer_SetVolume(secondary) failed: "
                   "%s\n",DXGetErrorString8(rc));
                rc=IDirectSoundBuffer_SetPan(secondary,0);
                ok(rc==DS_OK,"IDirectSoundBuffer_SetPan(secondary) failed: "
                   "%s\n",DXGetErrorString8(rc));
            }
            if (has_duplicate) {
                LPDIRECTSOUNDBUFFER duplicated=NULL;

                /* DSOUND: Error: Invalid source buffer */
                rc=IDirectSound_DuplicateSoundBuffer(dso,0,0);
                ok(rc==DSERR_INVALIDPARAM,
                   "IDirectSound_DuplicateSoundBuffer() should have returned "
                   "DSERR_INVALIDPARAM, returned: %s\n",DXGetErrorString8(rc));

                /* DSOUND: Error: Invalid dest buffer */
                rc=IDirectSound_DuplicateSoundBuffer(dso,secondary,0);
                ok(rc==DSERR_INVALIDPARAM,
                   "IDirectSound_DuplicateSoundBuffer() should have returned "
                   "DSERR_INVALIDPARAM, returned: %s\n",DXGetErrorString8(rc));

                /* DSOUND: Error: Invalid source buffer */
                rc=IDirectSound_DuplicateSoundBuffer(dso,0,&duplicated);
                ok(rc==DSERR_INVALIDPARAM,
                  "IDirectSound_DuplicateSoundBuffer() should have returned "
                  "DSERR_INVALIDPARAM, returned: %s\n",DXGetErrorString8(rc));

                duplicated=NULL;
                rc=IDirectSound_DuplicateSoundBuffer(dso,secondary,
                                                     &duplicated);
                ok(rc==DS_OK && duplicated!=NULL,
                   "IDirectSound_DuplicateSoundBuffer() failed to duplicate "
                   "a secondary buffer: %s\n",DXGetErrorString8(rc));

                if (rc==DS_OK && duplicated!=NULL) {
                    ref=IDirectSoundBuffer_Release(secondary);
                    ok(ref==0,"IDirectSoundBuffer_Release() secondary has %d "
                      "references, should have 0\n",ref);
                    secondary=duplicated;
                }
            }

            if (rc==DS_OK && secondary!=NULL) {
                double duration;
                duration=(move_listener || move_sound?4.0:1.0);
                test_buffer(dso,secondary,0,FALSE,0,FALSE,0,
                            winetest_interactive,duration,has_3dbuffer,
                            listener,move_listener,move_sound,FALSE,0);
                ref=IDirectSoundBuffer_Release(secondary);
                ok(ref==0,"IDirectSoundBuffer_Release() %s has %d references, "
                   "should have 0\n",has_duplicate?"duplicated":"secondary",
                   ref);
            }
        }
    }
EXIT1:
    if (has_listener) {
        ref=IDirectSound3DListener_Release(listener);
        ok(ref==0,"IDirectSound3dListener_Release() listener has %d "
           "references, should have 0\n",ref);
    } else {
        ref=IDirectSoundBuffer_Release(primary);
        ok(ref==0,"IDirectSoundBuffer_Release() primary has %d references, "
           "should have 0\n",ref);
    }

    /* Set the CooperativeLevel back to normal */
    /* DSOUND: Setting DirectSound cooperative level to DSSCL_NORMAL */
    rc=IDirectSound_SetCooperativeLevel(dso,get_hwnd(),DSSCL_NORMAL);
    ok(rc==DS_OK,"IDirectSound_SetCooperativeLevel(DSSCL_NORMAL) failed: %s\n",
       DXGetErrorString8(rc));

EXIT:
    ref=IDirectSound_Release(dso);
    ok(ref==0,"IDirectSound_Release() has %d references, should have 0\n",ref);
    if (ref!=0)
        return DSERR_GENERIC;

    return rc;
}

static HRESULT test_for_driver(LPGUID lpGuid)
{
    HRESULT rc;
    LPDIRECTSOUND dso=NULL;
    int ref;

    /* Create the DirectSound object */
    rc=DirectSoundCreate(lpGuid,&dso,NULL);
    ok(rc==DS_OK||rc==DSERR_NODRIVER||rc==DSERR_ALLOCATED||rc==E_FAIL,
       "DirectSoundCreate() failed: %s\n",DXGetErrorString8(rc));
    if (rc!=DS_OK)
        return rc;

    ref=IDirectSound_Release(dso);
    ok(ref==0,"IDirectSound_Release() has %d references, should have 0\n",ref);
    if (ref!=0)
        return DSERR_GENERIC;

    return rc;
}

static HRESULT test_primary(LPGUID lpGuid)
{
    HRESULT rc;
    LPDIRECTSOUND dso=NULL;
    LPDIRECTSOUNDBUFFER primary=NULL;
    DSBUFFERDESC bufdesc;
    DSCAPS dscaps;
    int ref, i;

    /* Create the DirectSound object */
    rc=DirectSoundCreate(lpGuid,&dso,NULL);
    ok(rc==DS_OK||rc==DSERR_NODRIVER,"DirectSoundCreate() failed: %s\n",
       DXGetErrorString8(rc));
    if (rc!=DS_OK)
        return rc;

    /* Get the device capabilities */
    ZeroMemory(&dscaps, sizeof(dscaps));
    dscaps.dwSize=sizeof(dscaps);
    rc=IDirectSound_GetCaps(dso,&dscaps);
    ok(rc==DS_OK,"IDirectSound_GetCaps() failed: %s\n",DXGetErrorString8(rc));
    if (rc!=DS_OK)
        goto EXIT;

    /* We must call SetCooperativeLevel before calling CreateSoundBuffer */
    /* DSOUND: Setting DirectSound cooperative level to DSSCL_PRIORITY */
    rc=IDirectSound_SetCooperativeLevel(dso,get_hwnd(),DSSCL_PRIORITY);
    ok(rc==DS_OK,"IDirectSound_SetCooperativeLevel(DSSCL_PRIORITY) failed: "
       "%s\n",DXGetErrorString8(rc));
    if (rc!=DS_OK)
        goto EXIT;

    /* Testing the primary buffer */
    primary=NULL;
    ZeroMemory(&bufdesc, sizeof(bufdesc));
    bufdesc.dwSize=sizeof(bufdesc);
    bufdesc.dwFlags=DSBCAPS_PRIMARYBUFFER|DSBCAPS_CTRLVOLUME|DSBCAPS_CTRLPAN;
    rc=IDirectSound_CreateSoundBuffer(dso,&bufdesc,&primary,NULL);
    ok((rc==DS_OK && primary!=NULL) || (rc==DSERR_CONTROLUNAVAIL),
       "IDirectSound_CreateSoundBuffer() failed to create a primary buffer: "
       "%s\n",DXGetErrorString8(rc));
    if (rc==DSERR_CONTROLUNAVAIL)
        trace("  No Primary\n");
    else if (rc==DS_OK && primary!=NULL) {
        test_buffer(dso,primary,1,TRUE,0,TRUE,0,winetest_interactive &&
                    !(dscaps.dwFlags & DSCAPS_EMULDRIVER),1.0,0,NULL,0,0,
                    FALSE,0);
        if (winetest_interactive) {
            LONG volume,pan;

            volume = DSBVOLUME_MAX;
            for (i = 0; i < 6; i++) {
                test_buffer(dso,primary,1,TRUE,volume,TRUE,0,
                            winetest_interactive &&
                            !(dscaps.dwFlags & DSCAPS_EMULDRIVER),
                            1.0,0,NULL,0,0,FALSE,0);
                volume -= ((DSBVOLUME_MAX-DSBVOLUME_MIN) / 40);
            }

            pan = DSBPAN_LEFT;
            for (i = 0; i < 7; i++) {
                test_buffer(dso,primary,1,TRUE,0,TRUE,pan,
                            winetest_interactive &&
                            !(dscaps.dwFlags & DSCAPS_EMULDRIVER),1.0,0,0,0,0,FALSE,0);
                pan += ((DSBPAN_RIGHT-DSBPAN_LEFT) / 6);
            }
        }
        ref=IDirectSoundBuffer_Release(primary);
        ok(ref==0,"IDirectSoundBuffer_Release() primary has %d references, "
           "should have 0\n",ref);
    }

    /* Set the CooperativeLevel back to normal */
    /* DSOUND: Setting DirectSound cooperative level to DSSCL_NORMAL */
    rc=IDirectSound_SetCooperativeLevel(dso,get_hwnd(),DSSCL_NORMAL);
    ok(rc==DS_OK,"IDirectSound_SetCooperativeLevel(DSSCL_NORMAL) failed: %s\n",
       DXGetErrorString8(rc));

EXIT:
    ref=IDirectSound_Release(dso);
    ok(ref==0,"IDirectSound_Release() has %d references, should have 0\n",ref);
    if (ref!=0)
        return DSERR_GENERIC;

    return rc;
}

static HRESULT test_primary_3d(LPGUID lpGuid)
{
    HRESULT rc;
    LPDIRECTSOUND dso=NULL;
    LPDIRECTSOUNDBUFFER primary=NULL;
    DSBUFFERDESC bufdesc;
    DSCAPS dscaps;
    int ref;

    /* Create the DirectSound object */
    rc=DirectSoundCreate(lpGuid,&dso,NULL);
    ok(rc==DS_OK||rc==DSERR_NODRIVER,"DirectSoundCreate() failed: %s\n",
       DXGetErrorString8(rc));
    if (rc!=DS_OK)
        return rc;

    /* Get the device capabilities */
    ZeroMemory(&dscaps, sizeof(dscaps));
    dscaps.dwSize=sizeof(dscaps);
    rc=IDirectSound_GetCaps(dso,&dscaps);
    ok(rc==DS_OK,"IDirectSound_GetCaps() failed: %s\n",DXGetErrorString8(rc));
    if (rc!=DS_OK)
        goto EXIT;

    /* We must call SetCooperativeLevel before calling CreateSoundBuffer */
    /* DSOUND: Setting DirectSound cooperative level to DSSCL_PRIORITY */
    rc=IDirectSound_SetCooperativeLevel(dso,get_hwnd(),DSSCL_PRIORITY);
    ok(rc==DS_OK,"IDirectSound_SetCooperativeLevel(DSSCL_PRIORITY) failed: "
       "%s\n",DXGetErrorString8(rc));
    if (rc!=DS_OK)
        goto EXIT;

    primary=NULL;
    ZeroMemory(&bufdesc, sizeof(bufdesc));
    bufdesc.dwSize=sizeof(bufdesc);
    bufdesc.dwFlags=DSBCAPS_PRIMARYBUFFER;
    rc=IDirectSound_CreateSoundBuffer(dso,&bufdesc,&primary,NULL);
    ok(rc==DS_OK && primary!=NULL,"IDirectSound_CreateSoundBuffer() failed "
       "to create a primary buffer: %s\n",DXGetErrorString8(rc));
    if (rc==DS_OK && primary!=NULL) {
        ref=IDirectSoundBuffer_Release(primary);
        ok(ref==0,"IDirectSoundBuffer_Release() primary has %d references, "
           "should have 0\n",ref);
        primary=NULL;
        ZeroMemory(&bufdesc, sizeof(bufdesc));
        bufdesc.dwSize=sizeof(bufdesc);
        bufdesc.dwFlags=DSBCAPS_PRIMARYBUFFER|DSBCAPS_CTRL3D;
        rc=IDirectSound_CreateSoundBuffer(dso,&bufdesc,&primary,NULL);
        ok(rc==DS_OK && primary!=NULL,"IDirectSound_CreateSoundBuffer() "
           "failed to create a 3D primary buffer: %s\n",DXGetErrorString8(rc));
        if (rc==DS_OK && primary!=NULL) {
            test_buffer(dso,primary,1,FALSE,0,FALSE,0,winetest_interactive &&
                        !(dscaps.dwFlags & DSCAPS_EMULDRIVER),1.0,0,0,0,0,
                        FALSE,0);
            ref=IDirectSoundBuffer_Release(primary);
            ok(ref==0,"IDirectSoundBuffer_Release() primary has %d references, "
               "should have 0\n",ref);
        }
    }
    /* Set the CooperativeLevel back to normal */
    /* DSOUND: Setting DirectSound cooperative level to DSSCL_NORMAL */
    rc=IDirectSound_SetCooperativeLevel(dso,get_hwnd(),DSSCL_NORMAL);
    ok(rc==DS_OK,"IDirectSound_SetCooperativeLevel(DSSCL_NORMAL) failed: %s\n",
       DXGetErrorString8(rc));

EXIT:
    ref=IDirectSound_Release(dso);
    ok(ref==0,"IDirectSound_Release() has %d references, should have 0\n",ref);
    if (ref!=0)
        return DSERR_GENERIC;

    return rc;
}

static HRESULT test_primary_3d_with_listener(LPGUID lpGuid)
{
    HRESULT rc;
    LPDIRECTSOUND dso=NULL;
    LPDIRECTSOUNDBUFFER primary=NULL;
    DSBUFFERDESC bufdesc;
    DSCAPS dscaps;
    int ref;

    /* Create the DirectSound object */
    rc=DirectSoundCreate(lpGuid,&dso,NULL);
    ok(rc==DS_OK||rc==DSERR_NODRIVER,"DirectSoundCreate() failed: %s\n",
       DXGetErrorString8(rc));
    if (rc!=DS_OK)
        return rc;

    /* Get the device capabilities */
    ZeroMemory(&dscaps, sizeof(dscaps));
    dscaps.dwSize=sizeof(dscaps);
    rc=IDirectSound_GetCaps(dso,&dscaps);
    ok(rc==DS_OK,"IDirectSound_GetCaps() failed: %s\n",DXGetErrorString8(rc));
    if (rc!=DS_OK)
        goto EXIT;

    /* We must call SetCooperativeLevel before calling CreateSoundBuffer */
    /* DSOUND: Setting DirectSound cooperative level to DSSCL_PRIORITY */
    rc=IDirectSound_SetCooperativeLevel(dso,get_hwnd(),DSSCL_PRIORITY);
    ok(rc==DS_OK,"IDirectSound_SetCooperativeLevel(DSSCL_PRIORITY) failed: "
       "%s\n",DXGetErrorString8(rc));
    if (rc!=DS_OK)
        goto EXIT;
    primary=NULL;
    ZeroMemory(&bufdesc, sizeof(bufdesc));
    bufdesc.dwSize=sizeof(bufdesc);
    bufdesc.dwFlags=DSBCAPS_PRIMARYBUFFER|DSBCAPS_CTRL3D;
    rc=IDirectSound_CreateSoundBuffer(dso,&bufdesc,&primary,NULL);
    ok(rc==DS_OK && primary!=NULL,"IDirectSound_CreateSoundBuffer() failed "
       "to create a 3D primary buffer: %s\n",DXGetErrorString8(rc));
    if (rc==DS_OK && primary!=NULL) {
        LPDIRECTSOUND3DLISTENER listener=NULL;
        rc=IDirectSoundBuffer_QueryInterface(primary,
            &IID_IDirectSound3DListener,(void **)&listener);
        ok(rc==DS_OK && listener!=NULL,"IDirectSoundBuffer_QueryInterface() "
           "failed to get a 3D listener: %s\n",DXGetErrorString8(rc));
        if (rc==DS_OK && listener!=NULL) {
            LPDIRECTSOUNDBUFFER temp_buffer=NULL;

            /* Checking the COM interface */
            rc=IDirectSoundBuffer_QueryInterface(primary,
                &IID_IDirectSoundBuffer,(LPVOID *)&temp_buffer);
            ok(rc==DS_OK && temp_buffer!=NULL,
               "IDirectSoundBuffer_QueryInterface() failed: %s\n",
               DXGetErrorString8(rc));
            ok(temp_buffer==primary,
               "COM interface broken: %p != %p\n",
               temp_buffer,primary);
            if (rc==DS_OK && temp_buffer!=NULL) {
                ref=IDirectSoundBuffer_Release(temp_buffer);
                ok(ref==1,"IDirectSoundBuffer_Release() has %d references, "
                   "should have 1\n",ref);

                temp_buffer=NULL;
                rc=IDirectSound3DListener_QueryInterface(listener,
                    &IID_IDirectSoundBuffer,(LPVOID *)&temp_buffer);
                ok(rc==DS_OK && temp_buffer!=NULL,
                   "IDirectSoundBuffer_QueryInterface() failed: %s\n",
                   DXGetErrorString8(rc));
                ok(temp_buffer==primary,
                   "COM interface broken: %p != %p\n",
                   temp_buffer,primary);
                ref=IDirectSoundBuffer_Release(temp_buffer);
                ok(ref==1,"IDirectSoundBuffer_Release() has %d references, "
                   "should have 1\n",ref);

                /* Testing the buffer */
                test_buffer(dso,primary,1,FALSE,0,FALSE,0,
                            winetest_interactive &&
                            !(dscaps.dwFlags & DSCAPS_EMULDRIVER),1.0,0,
                            listener,0,0,FALSE,0);
            }

            /* Testing the reference counting */
            ref=IDirectSound3DListener_Release(listener);
            ok(ref==0,"IDirectSound3DListener_Release() listener has %d "
               "references, should have 0\n",ref);
        }

        /* Testing the reference counting */
        ref=IDirectSoundBuffer_Release(primary);
        ok(ref==0,"IDirectSoundBuffer_Release() primary has %d references, "
           "should have 0\n",ref);
    }

EXIT:
    ref=IDirectSound_Release(dso);
    ok(ref==0,"IDirectSound_Release() has %d references, should have 0\n",ref);
    if (ref!=0)
return DSERR_GENERIC;

    return rc;
}

static BOOL WINAPI dsenum_callback(LPGUID lpGuid, LPCSTR lpcstrDescription,
                                   LPCSTR lpcstrModule, LPVOID lpContext)
{
    HRESULT rc;
    trace("*** Testing %s - %s ***\n",lpcstrDescription,lpcstrModule);

    rc = test_for_driver(lpGuid);
    if (rc == DSERR_NODRIVER) {
        trace("  No Driver\n");
        return 1;
    } else if (rc == DSERR_ALLOCATED) {
        trace("  Already In Use\n");
        return 1;
    } else if (rc == E_FAIL) {
        trace("  No Device\n");
        return 1;
    }

    trace("  Testing the primary buffer\n");
    test_primary(lpGuid);

    trace("  Testing 3D primary buffer\n");
    test_primary_3d(lpGuid);

    trace("  Testing 3D primary buffer with listener\n");
    test_primary_3d_with_listener(lpGuid);

    /* Testing secondary buffers */
    test_secondary(lpGuid,winetest_interactive,0,0,0,0,0,0);
    test_secondary(lpGuid,winetest_interactive,0,0,0,1,0,0);

    /* Testing 3D secondary buffers */
    test_secondary(lpGuid,winetest_interactive,1,0,0,0,0,0);
    test_secondary(lpGuid,winetest_interactive,1,1,0,0,0,0);
    test_secondary(lpGuid,winetest_interactive,1,1,0,1,0,0);
    test_secondary(lpGuid,winetest_interactive,1,0,1,0,0,0);
    test_secondary(lpGuid,winetest_interactive,1,0,1,1,0,0);
    test_secondary(lpGuid,winetest_interactive,1,1,1,0,0,0);
    test_secondary(lpGuid,winetest_interactive,1,1,1,1,0,0);
    test_secondary(lpGuid,winetest_interactive,1,1,1,0,1,0);
    test_secondary(lpGuid,winetest_interactive,1,1,1,0,0,1);
    test_secondary(lpGuid,winetest_interactive,1,1,1,0,1,1);

    return 1;
}

static void ds3d_tests(void)
{
    HRESULT rc;
    rc=DirectSoundEnumerateA(&dsenum_callback,NULL);
    ok(rc==DS_OK,"DirectSoundEnumerateA() failed: %s\n",DXGetErrorString8(rc));
}

START_TEST(ds3d)
{
    CoInitialize(NULL);

    trace("DLL Version: %s\n", get_file_version("dsound.dll"));

    ds3d_tests();

    CoUninitialize();
}
