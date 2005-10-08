/*
 * Tests basic sound playback in DirectSound.
 * In particular we test each standard Windows sound format to make sure
 * we handle the sound card/driver quirks correctly.
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
#include <stdlib.h>

#include "wine/test.h"
#include "windef.h"
#include "wingdi.h"
#include "dsound.h"
#include "dxerr8.h"
#include "dsconf.h"

#include "dsound_test.h"

static void IDirectSound_test(LPDIRECTSOUND dso, BOOL initialized,
                              LPCGUID lpGuid)
{
    HRESULT rc;
    DSCAPS dscaps;
    int ref;
    IUnknown * unknown;
    IDirectSound * ds;
    IDirectSound8 * ds8;
    DWORD speaker_config, new_speaker_config;

    /* Try to Query for objects */
    rc=IDirectSound_QueryInterface(dso,&IID_IUnknown,(LPVOID*)&unknown);
    ok(rc==DS_OK,"IDirectSound_QueryInterface(IID_IUnknown) failed: %s\n",
       DXGetErrorString8(rc));
    if (rc==DS_OK)
        IDirectSound_Release(unknown);

    rc=IDirectSound_QueryInterface(dso,&IID_IDirectSound,(LPVOID*)&ds);
    ok(rc==DS_OK,"IDirectSound_QueryInterface(IID_IDirectSound) failed: %s\n",
       DXGetErrorString8(rc));
    if (rc==DS_OK)
        IDirectSound_Release(ds);

    rc=IDirectSound_QueryInterface(dso,&IID_IDirectSound8,(LPVOID*)&ds8);
    ok(rc==E_NOINTERFACE,"IDirectSound_QueryInterface(IID_IDirectSound8) "
       "should have failed: %s\n",DXGetErrorString8(rc));
    if (rc==DS_OK)
        IDirectSound8_Release(ds8);

    if (initialized == FALSE) {
        /* try unitialized object */
        rc=IDirectSound_GetCaps(dso,0);
        ok(rc==DSERR_UNINITIALIZED,"IDirectSound_GetCaps(NULL) "
           "should have returned DSERR_UNINITIALIZED, returned: %s\n",
           DXGetErrorString8(rc));

        rc=IDirectSound_GetCaps(dso,&dscaps);
        ok(rc==DSERR_UNINITIALIZED,"IDirectSound_GetCaps() "
           "should have returned DSERR_UNINITIALIZED, returned: %s\n",
           DXGetErrorString8(rc));

        rc=IDirectSound_Compact(dso);
        ok(rc==DSERR_UNINITIALIZED,"IDirectSound_Compact() "
           "should have returned DSERR_UNINITIALIZED, returned: %s\n",
           DXGetErrorString8(rc));

        rc=IDirectSound_GetSpeakerConfig(dso,&speaker_config);
        ok(rc==DSERR_UNINITIALIZED,"IDirectSound_GetSpeakerConfig() "
           "should have returned DSERR_UNINITIALIZED, returned: %s\n",
           DXGetErrorString8(rc));

        rc=IDirectSound_Initialize(dso,lpGuid);
        ok(rc==DS_OK||rc==DSERR_NODRIVER||rc==DSERR_ALLOCATED,
           "IDirectSound_Initialize() failed: %s\n",DXGetErrorString8(rc));
        if (rc==DSERR_NODRIVER) {
            trace("  No Driver\n");
            return;
        } else if (rc==DSERR_ALLOCATED) {
            trace("  Already Allocated\n");
            return;
        }
    }

    /* DSOUND: Error: Invalid caps buffer */
    rc=IDirectSound_GetCaps(dso,0);
    ok(rc==DSERR_INVALIDPARAM,"IDirectSound_GetCaps(NULL) "
       "should have returned DSERR_INVALIDPARAM, returned: %s\n",
       DXGetErrorString8(rc));

    ZeroMemory(&dscaps, sizeof(dscaps));

    /* DSOUND: Error: Invalid caps buffer */
    rc=IDirectSound_GetCaps(dso,&dscaps);
    ok(rc==DSERR_INVALIDPARAM,"IDirectSound_GetCaps() "
       "should have returned DSERR_INVALIDPARAM, returned: %s\n",
       DXGetErrorString8(rc));

    dscaps.dwSize=sizeof(dscaps);

    /* DSOUND: Running on a certified driver */
    rc=IDirectSound_GetCaps(dso,&dscaps);
    ok(rc==DS_OK,"IDirectSound_GetCaps() failed: %s\n",DXGetErrorString8(rc));

    rc=IDirectSound_Compact(dso);
    ok(rc==DSERR_PRIOLEVELNEEDED,"IDirectSound_Compact() failed: %s\n",
       DXGetErrorString8(rc));

    rc=IDirectSound_SetCooperativeLevel(dso,get_hwnd(),DSSCL_PRIORITY);
    ok(rc==DS_OK,"IDirectSound_SetCooperativeLevel() failed: %s\n",
       DXGetErrorString8(rc));

    rc=IDirectSound_Compact(dso);
    ok(rc==DS_OK,"IDirectSound_Compact() failed: %s\n",DXGetErrorString8(rc));

    rc=IDirectSound_GetSpeakerConfig(dso,0);
    ok(rc==DSERR_INVALIDPARAM,"IDirectSound_GetSpeakerConfig(NULL) "
       "should have returned DSERR_INVALIDPARAM, returned: %s\n",
       DXGetErrorString8(rc));

    rc=IDirectSound_GetSpeakerConfig(dso,&speaker_config);
    ok(rc==DS_OK,"IDirectSound_GetSpeakerConfig() failed: %s\n",
       DXGetErrorString8(rc));

    speaker_config = DSSPEAKER_COMBINED(DSSPEAKER_STEREO,
                                        DSSPEAKER_GEOMETRY_WIDE);
    rc=IDirectSound_SetSpeakerConfig(dso,speaker_config);
    ok(rc==DS_OK,"IDirectSound_SetSpeakerConfig() failed: %s\n",
       DXGetErrorString8(rc));
    if (rc==DS_OK) {
        rc=IDirectSound_GetSpeakerConfig(dso,&new_speaker_config);
        ok(rc==DS_OK,"IDirectSound_GetSpeakerConfig() failed: %s\n",
           DXGetErrorString8(rc));
        if (rc==DS_OK && speaker_config!=new_speaker_config)
               trace("IDirectSound_GetSpeakerConfig() failed to set speaker "
               "config: expected 0x%08lx, got 0x%08lx\n",
               speaker_config,new_speaker_config);
    }

    ref=IDirectSound_Release(dso);
    ok(ref==0,"IDirectSound_Release() has %d references, should have 0\n",ref);
}

static void IDirectSound_tests()
{
    HRESULT rc;
    LPDIRECTSOUND dso=NULL;

    trace("Testing IDirectSound\n");

    /* try the COM class factory method of creation with no device specified */
    rc=CoCreateInstance(&CLSID_DirectSound, NULL, CLSCTX_INPROC_SERVER,
                        &IID_IDirectSound, (void**)&dso);
    ok(rc==S_OK,"CoCreateInstance(CLSID_DirectSound) failed: %s\n",
       DXGetErrorString8(rc));
    if (dso)
        IDirectSound_test(dso, FALSE, NULL);

    /* try the COM class factory method of creation with default playback
     * device specified */
    rc=CoCreateInstance(&CLSID_DirectSound, NULL, CLSCTX_INPROC_SERVER,
                        &IID_IDirectSound, (void**)&dso);
    ok(rc==S_OK,"CoCreateInstance(CLSID_DirectSound) failed: %s\n",
       DXGetErrorString8(rc));
    if (dso)
        IDirectSound_test(dso, FALSE, &DSDEVID_DefaultPlayback);

    /* try the COM class factory method of creation with default voice
     * playback device specified */
    rc=CoCreateInstance(&CLSID_DirectSound, NULL, CLSCTX_INPROC_SERVER,
                        &IID_IDirectSound, (void**)&dso);
    ok(rc==S_OK,"CoCreateInstance(CLSID_DirectSound) failed: %s\n",
       DXGetErrorString8(rc));
    if (dso)
        IDirectSound_test(dso, FALSE, &DSDEVID_DefaultVoicePlayback);

    /* try the COM class factory method of creation with a bad
     * IID specified */
    rc=CoCreateInstance(&CLSID_DirectSound, NULL, CLSCTX_INPROC_SERVER,
                        &CLSID_DirectSoundPrivate, (void**)&dso);
    ok(rc==E_NOINTERFACE,
       "CoCreateInstance(CLSID_DirectSound,CLSID_DirectSoundPrivate) "
       "should have failed: %s\n",DXGetErrorString8(rc));

    /* try the COM class factory method of creation with a bad
     * GUID and IID specified */
    rc=CoCreateInstance(&CLSID_DirectSoundPrivate, NULL, CLSCTX_INPROC_SERVER,
                        &IID_IDirectSound, (void**)&dso);
    ok(rc==REGDB_E_CLASSNOTREG,
       "CoCreateInstance(CLSID_DirectSoundPrivate,IID_IDirectSound) "
       "should have failed: %s\n",DXGetErrorString8(rc));

    /* try with no device specified */
    rc=DirectSoundCreate(NULL,&dso,NULL);
    ok(rc==S_OK||rc==DSERR_NODRIVER||rc==DSERR_ALLOCATED,
       "DirectSoundCreate(NULL) failed: %s\n",DXGetErrorString8(rc));
    if (rc==S_OK && dso)
        IDirectSound_test(dso, TRUE, NULL);

    /* try with default playback device specified */
    rc=DirectSoundCreate(&DSDEVID_DefaultPlayback,&dso,NULL);
    ok(rc==S_OK||rc==DSERR_NODRIVER||rc==DSERR_ALLOCATED,
       "DirectSoundCreate(DSDEVID_DefaultPlayback) failed: %s\n",
       DXGetErrorString8(rc));
    if (rc==DS_OK && dso)
        IDirectSound_test(dso, TRUE, NULL);

    /* try with default voice playback device specified */
    rc=DirectSoundCreate(&DSDEVID_DefaultVoicePlayback,&dso,NULL);
    ok(rc==S_OK||rc==DSERR_NODRIVER||rc==DSERR_ALLOCATED,
       "DirectSoundCreate(DSDEVID_DefaultVoicePlayback) failed: %s\n",
       DXGetErrorString8(rc));
    if (rc==DS_OK && dso)
        IDirectSound_test(dso, TRUE, NULL);

    /* try with a bad device specified */
    rc=DirectSoundCreate(&DSDEVID_DefaultVoiceCapture,&dso,NULL);
    ok(rc==DSERR_NODRIVER,"DirectSoundCreate(DSDEVID_DefaultVoiceCapture) "
       "should have failed: %s\n",DXGetErrorString8(rc));
}

static HRESULT test_dsound(LPGUID lpGuid)
{
    HRESULT rc;
    LPDIRECTSOUND dso=NULL;
    int ref;

    /* DSOUND: Error: Invalid interface buffer */
    rc=DirectSoundCreate(lpGuid,0,NULL);
    ok(rc==DSERR_INVALIDPARAM,"DirectSoundCreate() should have returned "
       "DSERR_INVALIDPARAM, returned: %s\n",DXGetErrorString8(rc));

    /* Create the DirectSound object */
    rc=DirectSoundCreate(lpGuid,&dso,NULL);
    ok(rc==DS_OK||rc==DSERR_NODRIVER||rc==DSERR_ALLOCATED,
       "DirectSoundCreate() failed: %s\n",DXGetErrorString8(rc));
    if (rc!=DS_OK)
        return rc;

    /* Try the enumerated device */
    IDirectSound_test(dso, TRUE, lpGuid);

    /* Try the COM class factory method of creation with enumerated device */
    rc=CoCreateInstance(&CLSID_DirectSound, NULL, CLSCTX_INPROC_SERVER,
                        &IID_IDirectSound, (void**)&dso);
    ok(rc==S_OK,"CoCreateInstance(CLSID_DirectSound) failed: %s\n",
       DXGetErrorString8(rc));
    if (dso)
        IDirectSound_test(dso, FALSE, lpGuid);

    /* Create a DirectSound object */
    rc=DirectSoundCreate(lpGuid,&dso,NULL);
    ok(rc==DS_OK,"DirectSoundCreate() failed: %s\n",DXGetErrorString8(rc));
    if (rc==DS_OK) {
        LPDIRECTSOUND dso1=NULL;

        /* Create a second DirectSound object */
        rc=DirectSoundCreate(lpGuid,&dso1,NULL);
        ok(rc==DS_OK,"DirectSoundCreate() failed: %s\n",DXGetErrorString8(rc));
        if (rc==DS_OK) {
            /* Release the second DirectSound object */
            ref=IDirectSound_Release(dso1);
            ok(ref==0,"IDirectSound_Release() has %d references, should have "
               "0\n",ref);
            ok(dso!=dso1,"DirectSound objects should be unique: "
               "dso=0x%08lx,dso1=0x%08lx\n",(DWORD)dso,(DWORD)dso1);
        }

        /* Release the first DirectSound object */
        ref=IDirectSound_Release(dso);
        ok(ref==0,"IDirectSound_Release() has %d references, should have 0\n",
           ref);
        if (ref!=0)
            return DSERR_GENERIC;
    } else
        return rc;

    /* Create a DirectSound object */
    rc=DirectSoundCreate(lpGuid,&dso,NULL);
    ok(rc==DS_OK,"DirectSoundCreate() failed: %s\n",DXGetErrorString8(rc));
    if (rc==DS_OK) {
        LPDIRECTSOUNDBUFFER secondary;
        DSBUFFERDESC bufdesc;
        WAVEFORMATEX wfx;

        init_format(&wfx,WAVE_FORMAT_PCM,11025,8,1);
        ZeroMemory(&bufdesc, sizeof(bufdesc));
        bufdesc.dwSize=sizeof(bufdesc);
        bufdesc.dwFlags=DSBCAPS_GETCURRENTPOSITION2 | DSBCAPS_CTRL3D;
        bufdesc.dwBufferBytes=wfx.nAvgBytesPerSec*BUFFER_LEN/1000;
        bufdesc.lpwfxFormat=&wfx;
        rc=IDirectSound_CreateSoundBuffer(dso,&bufdesc,&secondary,NULL);
        ok(rc==DS_OK && secondary!=NULL,
           "IDirectSound_CreateSoundBuffer() failed to create a secondary "
           "buffer %s\n",DXGetErrorString8(rc));
        if (rc==DS_OK && secondary!=NULL) {
            LPDIRECTSOUND3DBUFFER buffer3d;
            rc=IDirectSound_QueryInterface(secondary, &IID_IDirectSound3DBuffer,
                                           (void **)&buffer3d);
            ok(rc==DS_OK && buffer3d!=NULL,"IDirectSound_QueryInterface() "
               "failed:  %s\n",DXGetErrorString8(rc));
            if (rc==DS_OK && buffer3d!=NULL) {
                ref=IDirectSound3DBuffer_AddRef(buffer3d);
                ok(ref==2,"IDirectSound3DBuffer_AddRef() has %d references, "
                   "should have 2\n",ref);
            }
            ref=IDirectSoundBuffer_AddRef(secondary);
            ok(ref==2,"IDirectSoundBuffer_AddRef() has %d references, "
               "should have 2\n",ref);
        }
        /* release with buffer */
        ref=IDirectSound_Release(dso);
        ok(ref==0,"IDirectSound_Release() has %d references, should have 0\n",
           ref);
        if (ref!=0)
            return DSERR_GENERIC;
    } else
        return rc;

    return DS_OK;
}

static HRESULT test_primary(LPGUID lpGuid)
{
    HRESULT rc;
    LPDIRECTSOUND dso=NULL;
    LPDIRECTSOUNDBUFFER primary=NULL,second=NULL,third=NULL;
    DSBUFFERDESC bufdesc;
    DSCAPS dscaps;
    WAVEFORMATEX wfx;
    int ref;

    /* Create the DirectSound object */
    rc=DirectSoundCreate(lpGuid,&dso,NULL);
    ok(rc==DS_OK||rc==DSERR_NODRIVER||rc==DSERR_ALLOCATED,
       "DirectSoundCreate() failed: %s\n",DXGetErrorString8(rc));
    if (rc!=DS_OK)
        return rc;

    /* Get the device capabilities */
    ZeroMemory(&dscaps, sizeof(dscaps));
    dscaps.dwSize=sizeof(dscaps);
    rc=IDirectSound_GetCaps(dso,&dscaps);
    ok(rc==DS_OK,"IDirectSound_GetCaps() failed: %s\n",DXGetErrorString8(rc));
    if (rc!=DS_OK)
        goto EXIT;

    /* DSOUND: Error: Invalid buffer description pointer */
    rc=IDirectSound_CreateSoundBuffer(dso,0,0,NULL);
    ok(rc==DSERR_INVALIDPARAM,
       "IDirectSound_CreateSoundBuffer() should have failed: %s\n",
       DXGetErrorString8(rc));

    /* DSOUND: Error: Invalid buffer description pointer */
    rc=IDirectSound_CreateSoundBuffer(dso,0,&primary,NULL);
    ok(rc==DSERR_INVALIDPARAM && primary==0,
       "IDirectSound_CreateSoundBuffer() should have failed: rc=%s,"
       "dsbo=0x%lx\n",DXGetErrorString8(rc),(DWORD)primary);

    /* DSOUND: Error: Invalid buffer description pointer */
    rc=IDirectSound_CreateSoundBuffer(dso,&bufdesc,0,NULL);
    ok(rc==DSERR_INVALIDPARAM && primary==0,
       "IDirectSound_CreateSoundBuffer() should have failed: rc=%s,"
       "dsbo=0x%lx\n",DXGetErrorString8(rc),(DWORD)primary);

    ZeroMemory(&bufdesc, sizeof(bufdesc));

    /* DSOUND: Error: Invalid size */
    /* DSOUND: Error: Invalid buffer description */
    rc=IDirectSound_CreateSoundBuffer(dso,&bufdesc,&primary,NULL);
    ok(rc==DSERR_INVALIDPARAM && primary==0,
       "IDirectSound_CreateSoundBuffer() should have failed: rc=%s,"
       "primary=0x%lx\n",DXGetErrorString8(rc),(DWORD)primary);

    /* We must call SetCooperativeLevel before calling CreateSoundBuffer */
    /* DSOUND: Setting DirectSound cooperative level to DSSCL_PRIORITY */
    rc=IDirectSound_SetCooperativeLevel(dso,get_hwnd(),DSSCL_PRIORITY);
    ok(rc==DS_OK,"IDirectSound_SetCooperativeLevel() failed: %s\n",
       DXGetErrorString8(rc));
    if (rc!=DS_OK)
        goto EXIT;

    /* Testing the primary buffer */
    primary=NULL;
    ZeroMemory(&bufdesc, sizeof(bufdesc));
    bufdesc.dwSize=sizeof(bufdesc);
    bufdesc.dwFlags=DSBCAPS_PRIMARYBUFFER|DSBCAPS_CTRLVOLUME;
    bufdesc.lpwfxFormat = &wfx;
    init_format(&wfx,WAVE_FORMAT_PCM,11025,8,2);
    rc=IDirectSound_CreateSoundBuffer(dso,&bufdesc,&primary,NULL);
    ok(rc==DSERR_INVALIDPARAM,"IDirectSound_CreateSoundBuffer() should have "
       "returned DSERR_INVALIDPARAM, returned: %s\n", DXGetErrorString8(rc));
    if (rc==DS_OK && primary!=NULL)
        IDirectSoundBuffer_Release(primary);

    primary=NULL;
    ZeroMemory(&bufdesc, sizeof(bufdesc));
    bufdesc.dwSize=sizeof(bufdesc);
    bufdesc.dwFlags=DSBCAPS_PRIMARYBUFFER|DSBCAPS_CTRLVOLUME;
    rc=IDirectSound_CreateSoundBuffer(dso,&bufdesc,&primary,NULL);
    ok(rc==DS_OK && primary!=NULL,
       "IDirectSound_CreateSoundBuffer() failed to create a primary buffer: "
       "%s\n",DXGetErrorString8(rc));
    if (rc==DS_OK && primary!=NULL) {
        LONG vol;

        /* Try to create a second primary buffer */
        /* DSOUND: Error: The primary buffer already exists.
         * Any changes made to the buffer description will be ignored. */
        rc=IDirectSound_CreateSoundBuffer(dso,&bufdesc,&second,NULL);
        ok(rc==DS_OK && second==primary,
           "IDirectSound_CreateSoundBuffer() should have returned original "
           "primary buffer: %s\n",DXGetErrorString8(rc));
        ref=IDirectSoundBuffer_Release(second);
        ok(ref==1,"IDirectSoundBuffer_Release() primary has %d references, "
           "should have 1\n",ref);

        /* Try to duplicate a primary buffer */
        /* DSOUND: Error: Can't duplicate primary buffers */
        rc=IDirectSound_DuplicateSoundBuffer(dso,primary,&third);
        /* rc=0x88780032 */
        ok(rc!=DS_OK,"IDirectSound_DuplicateSoundBuffer() primary buffer "
           "should have failed %s\n",DXGetErrorString8(rc));

        rc=IDirectSoundBuffer_GetVolume(primary,&vol);
        ok(rc==DS_OK,"IDirectSoundBuffer_GetVolume() failed: %s\n",
           DXGetErrorString8(rc));

        if (winetest_interactive) {
            trace("Playing a 5 seconds reference tone at the current "
                  "volume.\n");
            if (rc==DS_OK)
                trace("(the current volume is %ld according to DirectSound)\n",
                      vol);
            trace("All subsequent tones should be identical to this one.\n");
            trace("Listen for stutter, changes in pitch, volume, etc.\n");
        }
        test_buffer(dso,primary,1,FALSE,0,FALSE,0,winetest_interactive &&
                    !(dscaps.dwFlags & DSCAPS_EMULDRIVER),5.0,0,0,0,0);

        ref=IDirectSoundBuffer_Release(primary);
        ok(ref==0,"IDirectSoundBuffer_Release() primary has %d references, "
           "should have 0\n",ref);
    }

    /* Set the CooperativeLevel back to normal */
    /* DSOUND: Setting DirectSound cooperative level to DSSCL_NORMAL */
    rc=IDirectSound_SetCooperativeLevel(dso,get_hwnd(),DSSCL_NORMAL);
    ok(rc==DS_OK,"IDirectSound_SetCooperativeLevel() failed: %s\n",
       DXGetErrorString8(rc));

EXIT:
    ref=IDirectSound_Release(dso);
    ok(ref==0,"IDirectSound_Release() has %d references, should have 0\n",ref);
    if (ref!=0)
        return DSERR_GENERIC;

    return rc;
}

/*
 * Test the primary buffer at different formats while keeping the
 * secondary buffer at a constant format.
 */
static HRESULT test_primary_secondary(LPGUID lpGuid)
{
    HRESULT rc;
    LPDIRECTSOUND dso=NULL;
    LPDIRECTSOUNDBUFFER primary=NULL,secondary=NULL;
    DSBUFFERDESC bufdesc;
    DSCAPS dscaps;
    WAVEFORMATEX wfx, wfx2;
    int f,ref;

    /* Create the DirectSound object */
    rc=DirectSoundCreate(lpGuid,&dso,NULL);
    ok(rc==DS_OK||rc==DSERR_NODRIVER||rc==DSERR_ALLOCATED,
       "DirectSoundCreate() failed: %s\n",DXGetErrorString8(rc));
    if (rc!=DS_OK)
        return rc;

    /* Get the device capabilities */
    ZeroMemory(&dscaps, sizeof(dscaps));
    dscaps.dwSize=sizeof(dscaps);
    rc=IDirectSound_GetCaps(dso,&dscaps);
    ok(rc==DS_OK,"IDirectSound_GetCaps() failed: %s\n",DXGetErrorString8(rc));
    if (rc!=DS_OK)
        goto EXIT;

    /* We must call SetCooperativeLevel before creating primary buffer */
    /* DSOUND: Setting DirectSound cooperative level to DSSCL_PRIORITY */
    rc=IDirectSound_SetCooperativeLevel(dso,get_hwnd(),DSSCL_PRIORITY);
    ok(rc==DS_OK,"IDirectSound_SetCooperativeLevel() failed: %s\n",
       DXGetErrorString8(rc));
    if (rc!=DS_OK)
        goto EXIT;

    ZeroMemory(&bufdesc, sizeof(bufdesc));
    bufdesc.dwSize=sizeof(bufdesc);
    bufdesc.dwFlags=DSBCAPS_PRIMARYBUFFER;
    rc=IDirectSound_CreateSoundBuffer(dso,&bufdesc,&primary,NULL);
    ok(rc==DS_OK && primary!=NULL,
       "IDirectSound_CreateSoundBuffer() failed to create a primary buffer "
       "%s\n",DXGetErrorString8(rc));

    if (rc==DS_OK && primary!=NULL) {
        for (f=0;f<NB_FORMATS;f++) {
            /* We must call SetCooperativeLevel to be allowed to call
             * SetFormat */
            /* DSOUND: Setting DirectSound cooperative level to
             * DSSCL_PRIORITY */
            rc=IDirectSound_SetCooperativeLevel(dso,get_hwnd(),DSSCL_PRIORITY);
            ok(rc==DS_OK,"IDirectSound_SetCooperativeLevel() failed: %s\n",
               DXGetErrorString8(rc));
            if (rc!=DS_OK)
                goto EXIT;

            init_format(&wfx,WAVE_FORMAT_PCM,formats[f][0],formats[f][1],
                        formats[f][2]);
            wfx2=wfx;
            rc=IDirectSoundBuffer_SetFormat(primary,&wfx);
            ok(rc==DS_OK,"IDirectSoundBuffer_SetFormat() failed: %s\n",
               DXGetErrorString8(rc));

            /* There is no garantee that SetFormat will actually change the
             * format to what we asked for. It depends on what the soundcard
             * supports. So we must re-query the format.
             */
            rc=IDirectSoundBuffer_GetFormat(primary,&wfx,sizeof(wfx),NULL);
            ok(rc==DS_OK,"IDirectSoundBuffer_GetFormat() failed: %s\n",
               DXGetErrorString8(rc));
            if (rc==DS_OK &&
                (wfx.wFormatTag!=wfx2.wFormatTag ||
                 wfx.nSamplesPerSec!=wfx2.nSamplesPerSec ||
                 wfx.wBitsPerSample!=wfx2.wBitsPerSample ||
                 wfx.nChannels!=wfx2.nChannels)) {
                trace("Requested primary format tag=0x%04x %ldx%dx%d "
                      "avg.B/s=%ld align=%d\n",
                      wfx2.wFormatTag,wfx2.nSamplesPerSec,wfx2.wBitsPerSample,
                      wfx2.nChannels,wfx2.nAvgBytesPerSec,wfx2.nBlockAlign);
                trace("Got tag=0x%04x %ldx%dx%d avg.B/s=%ld align=%d\n",
                      wfx.wFormatTag,wfx.nSamplesPerSec,wfx.wBitsPerSample,
                      wfx.nChannels,wfx.nAvgBytesPerSec,wfx.nBlockAlign);
            }

            /* Set the CooperativeLevel back to normal */
            /* DSOUND: Setting DirectSound cooperative level to DSSCL_NORMAL */
            rc=IDirectSound_SetCooperativeLevel(dso,get_hwnd(),DSSCL_NORMAL);
            ok(rc==DS_OK,"IDirectSound_SetCooperativeLevel() failed: %s\n",
               DXGetErrorString8(rc));

            init_format(&wfx2,WAVE_FORMAT_PCM,11025,16,2);

            secondary=NULL;
            ZeroMemory(&bufdesc, sizeof(bufdesc));
            bufdesc.dwSize=sizeof(bufdesc);
            bufdesc.dwFlags=DSBCAPS_GETCURRENTPOSITION2;
            bufdesc.dwBufferBytes=wfx.nAvgBytesPerSec*BUFFER_LEN/1000;
            bufdesc.lpwfxFormat=&wfx2;
            if (winetest_interactive) {
                trace("  Testing a primary buffer at %ldx%dx%d with a "
                      "secondary buffer at %ldx%dx%d\n",
                      wfx.nSamplesPerSec,wfx.wBitsPerSample,wfx.nChannels,
                      wfx2.nSamplesPerSec,wfx2.wBitsPerSample,wfx2.nChannels);
            }
            rc=IDirectSound_CreateSoundBuffer(dso,&bufdesc,&secondary,NULL);
            ok(rc==DS_OK && secondary!=NULL,
               "IDirectSound_CreateSoundBuffer() failed to create a secondary "
               "buffer %s\n",DXGetErrorString8(rc));

            if (rc==DS_OK && secondary!=NULL) {
                test_buffer(dso,secondary,0,FALSE,0,FALSE,0,
                            winetest_interactive,1.0,0,NULL,0,0);

                ref=IDirectSoundBuffer_Release(secondary);
                ok(ref==0,"IDirectSoundBuffer_Release() has %d references, "
                   "should have 0\n",ref);
            }
        }

        ref=IDirectSoundBuffer_Release(primary);
        ok(ref==0,"IDirectSoundBuffer_Release() primary has %d references, "
           "should have 0\n",ref);
    }

    /* Set the CooperativeLevel back to normal */
    /* DSOUND: Setting DirectSound cooperative level to DSSCL_NORMAL */
    rc=IDirectSound_SetCooperativeLevel(dso,get_hwnd(),DSSCL_NORMAL);
    ok(rc==DS_OK,"IDirectSound_SetCooperativeLevel() failed: %s\n",
       DXGetErrorString8(rc));

EXIT:
    ref=IDirectSound_Release(dso);
    ok(ref==0,"IDirectSound_Release() has %d references, should have 0\n",ref);
    if (ref!=0)
        return DSERR_GENERIC;

    return rc;
}

static HRESULT test_secondary(LPGUID lpGuid)
{
    HRESULT rc;
    LPDIRECTSOUND dso=NULL;
    LPDIRECTSOUNDBUFFER primary=NULL,secondary=NULL;
    DSBUFFERDESC bufdesc;
    DSCAPS dscaps;
    WAVEFORMATEX wfx;
    DWORD f;
    int ref;

    /* Create the DirectSound object */
    rc=DirectSoundCreate(lpGuid,&dso,NULL);
    ok(rc==DS_OK||rc==DSERR_NODRIVER||rc==DSERR_ALLOCATED,
       "DirectSoundCreate() failed: %s\n",DXGetErrorString8(rc));
    if (rc!=DS_OK)
        return rc;

    /* Get the device capabilities */
    ZeroMemory(&dscaps, sizeof(dscaps));
    dscaps.dwSize=sizeof(dscaps);
    rc=IDirectSound_GetCaps(dso,&dscaps);
    ok(rc==DS_OK,"IDirectSound_GetCaps() failed: %s\n",DXGetErrorString8(rc));
    if (rc!=DS_OK)
        goto EXIT;

    /* We must call SetCooperativeLevel before creating primary buffer */
    /* DSOUND: Setting DirectSound cooperative level to DSSCL_PRIORITY */
    rc=IDirectSound_SetCooperativeLevel(dso,get_hwnd(),DSSCL_PRIORITY);
    ok(rc==DS_OK,"IDirectSound_SetCooperativeLevel() failed: %s\n",
       DXGetErrorString8(rc));
    if (rc!=DS_OK)
        goto EXIT;

    ZeroMemory(&bufdesc, sizeof(bufdesc));
    bufdesc.dwSize=sizeof(bufdesc);
    bufdesc.dwFlags=DSBCAPS_PRIMARYBUFFER;
    rc=IDirectSound_CreateSoundBuffer(dso,&bufdesc,&primary,NULL);
    ok(rc==DS_OK && primary!=NULL,
       "IDirectSound_CreateSoundBuffer() failed to create a primary buffer "
       "%s\n",DXGetErrorString8(rc));

    if (rc==DS_OK && primary!=NULL) {
        for (f=0;f<NB_FORMATS;f++) {
            init_format(&wfx,WAVE_FORMAT_PCM,formats[f][0],formats[f][1],
                        formats[f][2]);
            secondary=NULL;
            ZeroMemory(&bufdesc, sizeof(bufdesc));
            bufdesc.dwSize=sizeof(bufdesc);
            bufdesc.dwFlags=DSBCAPS_GETCURRENTPOSITION2;
            bufdesc.dwBufferBytes=wfx.nAvgBytesPerSec*BUFFER_LEN/1000;
            rc=IDirectSound_CreateSoundBuffer(dso,&bufdesc,&secondary,NULL);
            ok(rc==DSERR_INVALIDPARAM,"IDirectSound_CreateSoundBuffer() "
               "should have returned DSERR_INVALIDPARAM, returned: %s\n",
               DXGetErrorString8(rc));
            if (rc==DS_OK && secondary!=NULL)
                IDirectSoundBuffer_Release(secondary);

            secondary=NULL;
            ZeroMemory(&bufdesc, sizeof(bufdesc));
            bufdesc.dwSize=sizeof(bufdesc);
            bufdesc.dwFlags=DSBCAPS_GETCURRENTPOSITION2;
            bufdesc.dwBufferBytes=wfx.nAvgBytesPerSec*BUFFER_LEN/1000;
            bufdesc.lpwfxFormat=&wfx;
            if (winetest_interactive) {
                trace("  Testing a secondary buffer at %ldx%dx%d\n",
                      wfx.nSamplesPerSec,wfx.wBitsPerSample,wfx.nChannels);
            }
            rc=IDirectSound_CreateSoundBuffer(dso,&bufdesc,&secondary,NULL);
            ok(rc==DS_OK && secondary!=NULL,
               "IDirectSound_CreateSoundBuffer() failed to create a secondary "
               "buffer %s\n",DXGetErrorString8(rc));

            if (rc==DS_OK && secondary!=NULL) {
                test_buffer(dso,secondary,0,FALSE,0,FALSE,0,
                            winetest_interactive,1.0,0,NULL,0,0);

                ref=IDirectSoundBuffer_Release(secondary);
                ok(ref==0,"IDirectSoundBuffer_Release() has %d references, "
                   "should have 0\n",ref);
            }
        }

        ref=IDirectSoundBuffer_Release(primary);
        ok(ref==0,"IDirectSoundBuffer_Release() primary has %d references, "
           "should have 0\n",ref);
    }

    /* Set the CooperativeLevel back to normal */
    /* DSOUND: Setting DirectSound cooperative level to DSSCL_NORMAL */
    rc=IDirectSound_SetCooperativeLevel(dso,get_hwnd(),DSSCL_NORMAL);
    ok(rc==DS_OK,"IDirectSound_SetCooperativeLevel() failed: %s\n",
       DXGetErrorString8(rc));

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
    rc = test_dsound(lpGuid);
    if (rc == DSERR_NODRIVER)
        trace("  No Driver\n");
    else if (rc == DSERR_ALLOCATED)
        trace("  Already In Use\n");
    else {
        test_primary(lpGuid);
        test_primary_secondary(lpGuid);
        test_secondary(lpGuid);
    }

    return 1;
}

static void dsound_tests()
{
    HRESULT rc;
    rc=DirectSoundEnumerateA(&dsenum_callback,NULL);
    ok(rc==DS_OK,"DirectSoundEnumerateA() failed: %s\n",DXGetErrorString8(rc));
}

START_TEST(dsound)
{
    CoInitialize(NULL);

    IDirectSound_tests();
    dsound_tests();
}
