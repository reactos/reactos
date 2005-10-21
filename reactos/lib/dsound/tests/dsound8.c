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
#include <stdio.h>

#include "wine/test.h"
#include "dsound.h"
#include "dxerr8.h"
#include "dsconf.h"

#include "dsound_test.h"

static HRESULT (WINAPI *pDirectSoundCreate8)(LPCGUID,LPDIRECTSOUND8*,LPUNKNOWN)=NULL;

int align(int length, int align)
{
    return (length / align) * align;
}

static void IDirectSound8_test(LPDIRECTSOUND8 dso, BOOL initialized,
                               LPCGUID lpGuid)
{
    HRESULT rc;
    DSCAPS dscaps;
    int ref;
    IUnknown * unknown;
    IDirectSound * ds;
    IDirectSound8 * ds8;
    DWORD speaker_config, new_speaker_config;
    DWORD certified;

    /* Try to Query for objects */
    rc=IDirectSound8_QueryInterface(dso,&IID_IUnknown,(LPVOID*)&unknown);
    ok(rc==DS_OK,"IDirectSound8_QueryInterface(IID_IUnknown) failed: %s\n",
       DXGetErrorString8(rc));
    if (rc==DS_OK)
        IDirectSound8_Release(unknown);

    rc=IDirectSound8_QueryInterface(dso,&IID_IDirectSound,(LPVOID*)&ds);
    ok(rc==DS_OK,"IDirectSound8_QueryInterface(IID_IDirectSound) failed: %s\n",
       DXGetErrorString8(rc));
    if (rc==DS_OK)
        IDirectSound_Release(ds);

    rc=IDirectSound8_QueryInterface(dso,&IID_IDirectSound8,(LPVOID*)&ds8);
    ok(rc==DS_OK,"IDirectSound8_QueryInterface(IID_IDirectSound8) "
       "should have returned DSERR_INVALIDPARAM, returned: %s\n",
       DXGetErrorString8(rc));
    if (rc==DS_OK)
        IDirectSound8_Release(ds8);

    if (initialized == FALSE) {
        /* try unitialized object */
        rc=IDirectSound8_GetCaps(dso,0);
        ok(rc==DSERR_UNINITIALIZED,"IDirectSound8_GetCaps(NULL) "
           "should have returned DSERR_UNINITIALIZED, returned: %s\n",
           DXGetErrorString8(rc));

        rc=IDirectSound8_GetCaps(dso,&dscaps);
        ok(rc==DSERR_UNINITIALIZED,"IDirectSound8_GetCaps() "
           "should have returned DSERR_UNINITIALIZED, returned: %s\n",
           DXGetErrorString8(rc));

        rc=IDirectSound8_Compact(dso);
        ok(rc==DSERR_UNINITIALIZED,"IDirectSound8_Compact() "
           "should have returned DSERR_UNINITIALIZED, returned: %s\n",
           DXGetErrorString8(rc));

        rc=IDirectSound8_GetSpeakerConfig(dso,&speaker_config);
        ok(rc==DSERR_UNINITIALIZED,"IDirectSound8_GetSpeakerConfig() "
           "should have returned DSERR_UNINITIALIZED, returned: %s\n",
           DXGetErrorString8(rc));

        rc=IDirectSound8_VerifyCertification(dso, &certified);
        ok(rc==DSERR_UNINITIALIZED,"IDirectSound8_VerifyCertification() "
           "should have returned DSERR_UNINITIALIZED, returned: %s\n",
           DXGetErrorString8(rc));

        rc=IDirectSound8_Initialize(dso,lpGuid);
        ok(rc==DS_OK||rc==DSERR_NODRIVER||rc==DSERR_ALLOCATED||rc==E_FAIL,
           "IDirectSound8_Initialize() failed: %s\n",DXGetErrorString8(rc));
        if (rc==DSERR_NODRIVER) {
            trace("  No Driver\n");
            goto EXIT;
        } else if (rc==E_FAIL) {
            trace("  No Device\n");
            goto EXIT;
        } else if (rc==DSERR_ALLOCATED) {
            trace("  Already In Use\n");
            goto EXIT;
        }
    }

    rc=IDirectSound8_Initialize(dso,lpGuid);
    ok(rc==DSERR_ALREADYINITIALIZED, "IDirectSound8_Initialize() "
       "should have returned DSERR_ALREADYINITIALIZED: %s\n",
       DXGetErrorString8(rc));

    /* DSOUND: Error: Invalid caps buffer */
    rc=IDirectSound8_GetCaps(dso,0);
    ok(rc==DSERR_INVALIDPARAM,"IDirectSound8_GetCaps() "
       "should have returned DSERR_INVALIDPARAM, returned: %s\n",
       DXGetErrorString8(rc));

    ZeroMemory(&dscaps, sizeof(dscaps));

    /* DSOUND: Error: Invalid caps buffer */
    rc=IDirectSound8_GetCaps(dso,&dscaps);
    ok(rc==DSERR_INVALIDPARAM,"IDirectSound8_GetCaps() "
       "should have returned DSERR_INVALIDPARAM, returned: %s\n",
       DXGetErrorString8(rc));

    dscaps.dwSize=sizeof(dscaps);

    /* DSOUND: Running on a certified driver */
    rc=IDirectSound8_GetCaps(dso,&dscaps);
    ok(rc==DS_OK,"IDirectSound8_GetCaps() failed: %s\n",DXGetErrorString8(rc));

    rc=IDirectSound8_Compact(dso);
    ok(rc==DSERR_PRIOLEVELNEEDED,"IDirectSound8_Compact() failed: %s\n",
       DXGetErrorString8(rc));

    rc=IDirectSound8_SetCooperativeLevel(dso,get_hwnd(),DSSCL_PRIORITY);
    ok(rc==DS_OK,"IDirectSound8_SetCooperativeLevel() failed: %s\n",
       DXGetErrorString8(rc));

    rc=IDirectSound8_Compact(dso);
    ok(rc==DS_OK,"IDirectSound8_Compact() failed: %s\n",DXGetErrorString8(rc));

    rc=IDirectSound8_GetSpeakerConfig(dso,0);
    ok(rc==DSERR_INVALIDPARAM,"IDirectSound8_GetSpeakerConfig(NULL) "
       "should have returned DSERR_INVALIDPARAM, returned: %s\n",
       DXGetErrorString8(rc));

    rc=IDirectSound8_GetSpeakerConfig(dso,&speaker_config);
    ok(rc==DS_OK,"IDirectSound8_GetSpeakerConfig() failed: %s\n",
       DXGetErrorString8(rc));

    speaker_config = DSSPEAKER_COMBINED(DSSPEAKER_STEREO,
                                        DSSPEAKER_GEOMETRY_WIDE);
    rc=IDirectSound8_SetSpeakerConfig(dso,speaker_config);
    ok(rc==DS_OK,"IDirectSound8_SetSpeakerConfig() failed: %s\n",
       DXGetErrorString8(rc));
    if (rc==DS_OK) {
        rc=IDirectSound8_GetSpeakerConfig(dso,&new_speaker_config);
        ok(rc==DS_OK,"IDirectSound8_GetSpeakerConfig() failed: %s\n",
           DXGetErrorString8(rc));
        if (rc==DS_OK && speaker_config!=new_speaker_config)
               trace("IDirectSound8_GetSpeakerConfig() failed to set speaker "
               "config: expected 0x%08lx, got 0x%08lx\n",
               speaker_config,new_speaker_config);
    }

    rc=IDirectSound8_VerifyCertification(dso, &certified);
    ok(rc==DS_OK||rc==E_NOTIMPL,"IDirectSound8_VerifyCertification() failed: %s\n",
       DXGetErrorString8(rc));

EXIT:
    ref=IDirectSound8_Release(dso);
    ok(ref==0,"IDirectSound8_Release() has %d references, should have 0\n",ref);
}

static void IDirectSound8_tests(void)
{
    HRESULT rc;
    LPDIRECTSOUND8 dso=NULL;

    trace("Testing IDirectSound8\n");

    /* try the COM class factory method of creation with no device specified */
    rc=CoCreateInstance(&CLSID_DirectSound8, NULL, CLSCTX_INPROC_SERVER,
                        &IID_IDirectSound8, (void**)&dso);
    ok(rc==S_OK,"CoCreateInstance() failed: %s\n",DXGetErrorString8(rc));
    if (dso)
        IDirectSound8_test(dso, FALSE, NULL);

    /* try the COM class factory method of creation with default playback
     *  device specified */
    rc=CoCreateInstance(&CLSID_DirectSound8, NULL, CLSCTX_INPROC_SERVER,
                        &IID_IDirectSound8, (void**)&dso);
    ok(rc==S_OK,"CoCreateInstance(CLSID_DirectSound8) failed: %s\n",
       DXGetErrorString8(rc));
    if (dso)
        IDirectSound8_test(dso, FALSE, &DSDEVID_DefaultPlayback);

    /* try the COM class factory method of creation with default voice
     * playback device specified */
    rc=CoCreateInstance(&CLSID_DirectSound8, NULL, CLSCTX_INPROC_SERVER,
                        &IID_IDirectSound8, (void**)&dso);
    ok(rc==S_OK,"CoCreateInstance(CLSID_DirectSound8) failed: %s\n",
       DXGetErrorString8(rc));
    if (dso)
        IDirectSound8_test(dso, FALSE, &DSDEVID_DefaultVoicePlayback);

    /* try the COM class factory method of creation with a bad
     * IID specified */
    rc=CoCreateInstance(&CLSID_DirectSound8, NULL, CLSCTX_INPROC_SERVER,
                        &CLSID_DirectSoundPrivate, (void**)&dso);
    ok(rc==E_NOINTERFACE,
       "CoCreateInstance(CLSID_DirectSound8,CLSID_DirectSoundPrivate) "
       "should have failed: %s\n",DXGetErrorString8(rc));

    /* try the COM class factory method of creation with a bad
     * GUID and IID specified */
    rc=CoCreateInstance(&CLSID_DirectSoundPrivate, NULL, CLSCTX_INPROC_SERVER,
                        &IID_IDirectSound8, (void**)&dso);
    ok(rc==REGDB_E_CLASSNOTREG,
       "CoCreateInstance(CLSID_DirectSoundPrivate,IID_IDirectSound8) "
       "should have failed: %s\n",DXGetErrorString8(rc));

    /* try with no device specified */
    rc=pDirectSoundCreate8(NULL,&dso,NULL);
    ok(rc==S_OK||rc==DSERR_NODRIVER||rc==DSERR_ALLOCATED||rc==E_FAIL,
       "DirectSoundCreate8() failed: %s\n",DXGetErrorString8(rc));
    if (rc==DS_OK && dso)
        IDirectSound8_test(dso, TRUE, NULL);

    /* try with default playback device specified */
    rc=pDirectSoundCreate8(&DSDEVID_DefaultPlayback,&dso,NULL);
    ok(rc==S_OK||rc==DSERR_NODRIVER||rc==DSERR_ALLOCATED||rc==E_FAIL,
       "DirectSoundCreate8() failed: %s\n",DXGetErrorString8(rc));
    if (rc==DS_OK && dso)
        IDirectSound8_test(dso, TRUE, NULL);

    /* try with default voice playback device specified */
    rc=pDirectSoundCreate8(&DSDEVID_DefaultVoicePlayback,&dso,NULL);
    ok(rc==S_OK||rc==DSERR_NODRIVER||rc==DSERR_ALLOCATED||rc==E_FAIL,
       "DirectSoundCreate8() failed: %s\n",DXGetErrorString8(rc));
    if (rc==DS_OK && dso)
        IDirectSound8_test(dso, TRUE, NULL);

    /* try with a bad device specified */
    rc=pDirectSoundCreate8(&DSDEVID_DefaultVoiceCapture,&dso,NULL);
    ok(rc==DSERR_NODRIVER,"DirectSoundCreate8(DSDEVID_DefaultVoiceCapture) "
       "should have failed: %s\n",DXGetErrorString8(rc));
}

static HRESULT test_dsound8(LPGUID lpGuid)
{
    HRESULT rc;
    LPDIRECTSOUND8 dso=NULL;
    int ref;

    /* DSOUND: Error: Invalid interface buffer */
    rc=pDirectSoundCreate8(lpGuid,0,NULL);
    ok(rc==DSERR_INVALIDPARAM,"DirectSoundCreate8() should have returned "
       "DSERR_INVALIDPARAM, returned: %s\n",DXGetErrorString8(rc));

    /* Create the DirectSound8 object */
    rc=pDirectSoundCreate8(lpGuid,&dso,NULL);
    ok(rc==DS_OK||rc==DSERR_NODRIVER||rc==DSERR_ALLOCATED||rc==E_FAIL,
       "DirectSoundCreate8() failed: %s\n",DXGetErrorString8(rc));
    if (rc!=DS_OK)
        return rc;

    /* Try the enumerated device */
    IDirectSound8_test(dso, TRUE, lpGuid);

    /* Try the COM class factory method of creation with enumerated device */
    rc=CoCreateInstance(&CLSID_DirectSound8, NULL, CLSCTX_INPROC_SERVER,
                        &IID_IDirectSound8, (void**)&dso);
    ok(rc==S_OK,"CoCreateInstance(CLSID_DirectSound) failed: %s\n",
       DXGetErrorString8(rc));
    if (dso)
        IDirectSound8_test(dso, FALSE, lpGuid);

    /* Create a DirectSound8 object */
    rc=pDirectSoundCreate8(lpGuid,&dso,NULL);
    ok(rc==DS_OK,"DirectSoundCreate8() failed: %s\n",DXGetErrorString8(rc));
    if (rc==DS_OK) {
        LPDIRECTSOUND8 dso1=NULL;

        /* Create a second DirectSound8 object */
        rc=pDirectSoundCreate8(lpGuid,&dso1,NULL);
        ok(rc==DS_OK,"DirectSoundCreate8() failed: %s\n",DXGetErrorString8(rc));
        if (rc==DS_OK) {
            /* Release the second DirectSound8 object */
            ref=IDirectSound8_Release(dso1);
            ok(ref==0,"IDirectSound8_Release() has %d references, "
               "should have 0\n",ref);
            ok(dso!=dso1,"DirectSound8 objects should be unique: "
               "dso=%p,dso1=%p\n",dso,dso1);
        }

        /* Release the first DirectSound8 object */
        ref=IDirectSound8_Release(dso);
        ok(ref==0,"IDirectSound8_Release() has %d references, should have 0\n",
           ref);
        if (ref!=0)
            return DSERR_GENERIC;
    } else
        return rc;

    /* Create a DirectSound8 object */
    rc=pDirectSoundCreate8(lpGuid,&dso,NULL);
    ok(rc==DS_OK,"DirectSoundCreate8() failed: %s\n",DXGetErrorString8(rc));
    if (rc==DS_OK) {
        LPDIRECTSOUNDBUFFER secondary;
        DSBUFFERDESC bufdesc;
        WAVEFORMATEX wfx;

        init_format(&wfx,WAVE_FORMAT_PCM,11025,8,1);
        ZeroMemory(&bufdesc, sizeof(bufdesc));
        bufdesc.dwSize=sizeof(bufdesc);
        bufdesc.dwFlags=DSBCAPS_GETCURRENTPOSITION2 | DSBCAPS_CTRL3D;
        bufdesc.dwBufferBytes=align(wfx.nAvgBytesPerSec*BUFFER_LEN/1000,
                                    wfx.nBlockAlign);
        bufdesc.lpwfxFormat=&wfx;
        rc=IDirectSound8_CreateSoundBuffer(dso,&bufdesc,&secondary,NULL);
        ok(rc==DS_OK && secondary!=NULL,
           "IDirectSound8_CreateSoundBuffer() failed to create a secondary "
           "buffer: %s\n",DXGetErrorString8(rc));
        if (rc==DS_OK && secondary!=NULL) {
            LPDIRECTSOUND3DBUFFER buffer3d;
            LPDIRECTSOUNDBUFFER8 buffer8;
            rc=IDirectSound8_QueryInterface(secondary,
                                            &IID_IDirectSound3DBuffer,
                                            (void **)&buffer3d);
            ok(rc==DS_OK && buffer3d!=NULL,
               "IDirectSound8_QueryInterface() failed: %s\n",
               DXGetErrorString8(rc));
            if (rc==DS_OK && buffer3d!=NULL) {
                ref=IDirectSound3DBuffer_AddRef(buffer3d);
                ok(ref==2,"IDirectSound3DBuffer_AddRef() has %d references, "
                   "should have 2\n",ref);
            }
            rc=IDirectSound8_QueryInterface(secondary,
                                            &IID_IDirectSoundBuffer8,
                                            (void **)&buffer8);
            if (rc==DS_OK && buffer8!=NULL) {
                ref=IDirectSoundBuffer8_AddRef(buffer8);
                ok(ref==3,"IDirectSoundBuffer8_AddRef() has %d references, "
                   "should have 3\n",ref);
            }
            ref=IDirectSoundBuffer_AddRef(secondary);
            ok(ref==4,"IDirectSoundBuffer_AddRef() has %d references, "
               "should have 4\n",ref);
        }
        /* release with buffer */
        ref=IDirectSound8_Release(dso);
        ok(ref==0,"IDirectSound8_Release() has %d references, should have 0\n",
           ref);
        if (ref!=0)
            return DSERR_GENERIC;
    } else
        return rc;

    return DS_OK;
}

static HRESULT test_primary8(LPGUID lpGuid)
{
    HRESULT rc;
    LPDIRECTSOUND8 dso=NULL;
    LPDIRECTSOUNDBUFFER primary=NULL,second=NULL,third=NULL;
    DSBUFFERDESC bufdesc;
    DSCAPS dscaps;
    WAVEFORMATEX wfx;
    int ref;

    /* Create the DirectSound object */
    rc=pDirectSoundCreate8(lpGuid,&dso,NULL);
    ok(rc==DS_OK||rc==DSERR_NODRIVER||rc==DSERR_ALLOCATED,
       "DirectSoundCreate8() failed: %s\n",DXGetErrorString8(rc));
    if (rc!=DS_OK)
        return rc;

    /* Get the device capabilities */
    ZeroMemory(&dscaps, sizeof(dscaps));
    dscaps.dwSize=sizeof(dscaps);
    rc=IDirectSound8_GetCaps(dso,&dscaps);
    ok(rc==DS_OK,"IDirectSound8_GetCaps() failed: %s\n",DXGetErrorString8(rc));
    if (rc!=DS_OK)
        goto EXIT;

    /* DSOUND: Error: Invalid buffer description pointer */
    rc=IDirectSound8_CreateSoundBuffer(dso,0,0,NULL);
    ok(rc==DSERR_INVALIDPARAM,
       "IDirectSound8_CreateSoundBuffer should have returned "
       "DSERR_INVALIDPARAM, returned: %s\n",DXGetErrorString8(rc));

    /* DSOUND: Error: Invalid buffer description pointer */
    rc=IDirectSound8_CreateSoundBuffer(dso,0,&primary,NULL);
    ok(rc==DSERR_INVALIDPARAM && primary==0,
       "IDirectSound8_CreateSoundBuffer() should have returned "
       "DSERR_INVALIDPARAM, returned: rc=%s,dsbo=%p\n",
       DXGetErrorString8(rc),primary);

    /* DSOUND: Error: Invalid buffer description pointer */
    rc=IDirectSound8_CreateSoundBuffer(dso,&bufdesc,0,NULL);
    ok(rc==DSERR_INVALIDPARAM && primary==0,
       "IDirectSound8_CreateSoundBuffer() should have failed: rc=%s,"
       "dsbo=%p\n",DXGetErrorString8(rc),primary);

    ZeroMemory(&bufdesc, sizeof(bufdesc));

    /* DSOUND: Error: Invalid size */
    /* DSOUND: Error: Invalid buffer description */
    rc=IDirectSound8_CreateSoundBuffer(dso,&bufdesc,&primary,NULL);
    ok(rc==DSERR_INVALIDPARAM && primary==0,
       "IDirectSound8_CreateSoundBuffer() should have failed: rc=%s,"
       "primary=%p\n",DXGetErrorString8(rc),primary);

    /* We must call SetCooperativeLevel before calling CreateSoundBuffer */
    /* DSOUND: Setting DirectSound cooperative level to DSSCL_PRIORITY */
    rc=IDirectSound8_SetCooperativeLevel(dso,get_hwnd(),DSSCL_PRIORITY);
    ok(rc==DS_OK,"IDirectSound8_SetCooperativeLevel() failed: %s\n",
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
    rc=IDirectSound8_CreateSoundBuffer(dso,&bufdesc,&primary,NULL);
    ok(rc==DSERR_INVALIDPARAM,"IDirectSound8_CreateSoundBuffer() should have "
       "returned DSERR_INVALIDPARAM, returned: %s\n", DXGetErrorString8(rc));
    if (rc==DS_OK && primary!=NULL)
        IDirectSoundBuffer_Release(primary);

    primary=NULL;
    ZeroMemory(&bufdesc, sizeof(bufdesc));
    bufdesc.dwSize=sizeof(bufdesc);
    bufdesc.dwFlags=DSBCAPS_PRIMARYBUFFER|DSBCAPS_CTRLVOLUME;
    rc=IDirectSound8_CreateSoundBuffer(dso,&bufdesc,&primary,NULL);
    ok((rc==DS_OK && primary!=NULL) || (rc==DSERR_CONTROLUNAVAIL),
       "IDirectSound8_CreateSoundBuffer() failed to create a primary buffer: "
       "%s\n",DXGetErrorString8(rc));
    if (rc==DSERR_CONTROLUNAVAIL)
        trace("  No Primary\n");
    else if (rc==DS_OK && primary!=NULL) {
        LONG vol;

        /* Try to create a second primary buffer */
        /* DSOUND: Error: The primary buffer already exists.
         * Any changes made to the buffer description will be ignored. */
        rc=IDirectSound8_CreateSoundBuffer(dso,&bufdesc,&second,NULL);
        ok(rc==DS_OK && second==primary,
           "IDirectSound8_CreateSoundBuffer() should have returned original "
           "primary buffer: %s\n",DXGetErrorString8(rc));
        ref=IDirectSoundBuffer_Release(second);
        ok(ref==1,"IDirectSoundBuffer_Release() primary has %d references, "
           "should have 1\n",ref);

        /* Try to duplicate a primary buffer */
        /* DSOUND: Error: Can't duplicate primary buffers */
        rc=IDirectSound8_DuplicateSoundBuffer(dso,primary,&third);
        /* rc=0x88780032 */
        ok(rc!=DS_OK,"IDirectSound8_DuplicateSoundBuffer() primary buffer "
           "should have failed %s\n",DXGetErrorString8(rc));

        rc=IDirectSoundBuffer_GetVolume(primary,&vol);
        ok(rc==DS_OK,"IDirectSoundBuffer_GetVolume() failed: %s\n",
           DXGetErrorString8(rc));

        if (winetest_interactive) {
            trace("Playing a 5 seconds reference tone at the current volume.\n");
            if (rc==DS_OK)
                trace("(the current volume is %ld according to DirectSound)\n",
                      vol);
            trace("All subsequent tones should be identical to this one.\n");
            trace("Listen for stutter, changes in pitch, volume, etc.\n");
        }
        test_buffer8(dso,primary,1,FALSE,0,FALSE,0,winetest_interactive &&
                     !(dscaps.dwFlags & DSCAPS_EMULDRIVER),5.0,0,0,0,0);

        ref=IDirectSoundBuffer_Release(primary);
        ok(ref==0,"IDirectSoundBuffer_Release() primary has %d references, "
           "should have 0\n",ref);
    }

    /* Set the CooperativeLevel back to normal */
    /* DSOUND: Setting DirectSound cooperative level to DSSCL_NORMAL */
    rc=IDirectSound8_SetCooperativeLevel(dso,get_hwnd(),DSSCL_NORMAL);
    ok(rc==DS_OK,"IDirectSound8_SetCooperativeLevel() failed: %s\n",
       DXGetErrorString8(rc));

EXIT:
    ref=IDirectSound8_Release(dso);
    ok(ref==0,"IDirectSound8_Release() has %d references, should have 0\n",ref);
    if (ref!=0)
        return DSERR_GENERIC;

    return rc;
}

/*
 * Test the primary buffer at different formats while keeping the
 * secondary buffer at a constant format.
 */
static HRESULT test_primary_secondary8(LPGUID lpGuid)
{
    HRESULT rc;
    LPDIRECTSOUND8 dso=NULL;
    LPDIRECTSOUNDBUFFER primary=NULL,secondary=NULL;
    DSBUFFERDESC bufdesc;
    DSCAPS dscaps;
    WAVEFORMATEX wfx, wfx2;
    int ref;
    unsigned int f;

    /* Create the DirectSound object */
    rc=pDirectSoundCreate8(lpGuid,&dso,NULL);
    ok(rc==DS_OK||rc==DSERR_NODRIVER||rc==DSERR_ALLOCATED,
       "DirectSoundCreate8() failed: %s\n",DXGetErrorString8(rc));
    if (rc!=DS_OK)
        return rc;

    /* Get the device capabilities */
    ZeroMemory(&dscaps, sizeof(dscaps));
    dscaps.dwSize=sizeof(dscaps);
    rc=IDirectSound8_GetCaps(dso,&dscaps);
    ok(rc==DS_OK,"IDirectSound8_GetCaps() failed: %s\n",DXGetErrorString8(rc));
    if (rc!=DS_OK)
        goto EXIT;

    /* We must call SetCooperativeLevel before creating primary buffer */
    /* DSOUND: Setting DirectSound cooperative level to DSSCL_PRIORITY */
    rc=IDirectSound8_SetCooperativeLevel(dso,get_hwnd(),DSSCL_PRIORITY);
    ok(rc==DS_OK,"IDirectSound8_SetCooperativeLevel() failed: %s\n",
       DXGetErrorString8(rc));
    if (rc!=DS_OK)
        goto EXIT;

    ZeroMemory(&bufdesc, sizeof(bufdesc));
    bufdesc.dwSize=sizeof(bufdesc);
    bufdesc.dwFlags=DSBCAPS_PRIMARYBUFFER;
    rc=IDirectSound8_CreateSoundBuffer(dso,&bufdesc,&primary,NULL);
    ok(rc==DS_OK && primary!=NULL,
       "IDirectSound8_CreateSoundBuffer() failed to create a primary buffer "
       "%s\n",DXGetErrorString8(rc));

    if (rc==DS_OK && primary!=NULL) {
        for (f=0;f<NB_FORMATS;f++) {
            /* We must call SetCooperativeLevel to be allowed to call
             * SetFormat */
            /* DSOUND: Setting DirectSound cooperative level to
             * DSSCL_PRIORITY */
            rc=IDirectSound8_SetCooperativeLevel(dso,get_hwnd(),DSSCL_PRIORITY);
            ok(rc==DS_OK,"IDirectSound8_SetCooperativeLevel() failed: %s\n",
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
            rc=IDirectSound8_SetCooperativeLevel(dso,get_hwnd(),DSSCL_NORMAL);
            ok(rc==DS_OK,"IDirectSound8_SetCooperativeLevel() failed: %s\n",
               DXGetErrorString8(rc));

            init_format(&wfx2,WAVE_FORMAT_PCM,11025,16,2);

            secondary=NULL;
            ZeroMemory(&bufdesc, sizeof(bufdesc));
            bufdesc.dwSize=sizeof(bufdesc);
            bufdesc.dwFlags=DSBCAPS_GETCURRENTPOSITION2;
            bufdesc.dwBufferBytes=align(wfx.nAvgBytesPerSec*BUFFER_LEN/1000,
                                        wfx.nBlockAlign);
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
                test_buffer8(dso,secondary,0,FALSE,0,FALSE,0,
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
    rc=IDirectSound8_SetCooperativeLevel(dso,get_hwnd(),DSSCL_NORMAL);
    ok(rc==DS_OK,"IDirectSound8_SetCooperativeLevel() failed: %s\n",
       DXGetErrorString8(rc));

EXIT:
    ref=IDirectSound8_Release(dso);
    ok(ref==0,"IDirectSound8_Release() has %d references, should have 0\n",ref);
    if (ref!=0)
        return DSERR_GENERIC;

    return rc;
}

static HRESULT test_secondary8(LPGUID lpGuid)
{
    HRESULT rc;
    LPDIRECTSOUND8 dso=NULL;
    LPDIRECTSOUNDBUFFER primary=NULL,secondary=NULL;
    DSBUFFERDESC bufdesc;
    DSCAPS dscaps;
    WAVEFORMATEX wfx, wfx1;
    DWORD f;
    int ref;

    /* Create the DirectSound object */
    rc=pDirectSoundCreate8(lpGuid,&dso,NULL);
    ok(rc==DS_OK||rc==DSERR_NODRIVER||rc==DSERR_ALLOCATED,
       "DirectSoundCreate8() failed: %s\n",DXGetErrorString8(rc));
    if (rc!=DS_OK)
        return rc;

    /* Get the device capabilities */
    ZeroMemory(&dscaps, sizeof(dscaps));
    dscaps.dwSize=sizeof(dscaps);
    rc=IDirectSound8_GetCaps(dso,&dscaps);
    ok(rc==DS_OK,"IDirectSound8_GetCaps() failed: %s\n",DXGetErrorString8(rc));
    if (rc!=DS_OK)
        goto EXIT;

    /* We must call SetCooperativeLevel before creating primary buffer */
    /* DSOUND: Setting DirectSound cooperative level to DSSCL_PRIORITY */
    rc=IDirectSound8_SetCooperativeLevel(dso,get_hwnd(),DSSCL_PRIORITY);
    ok(rc==DS_OK,"IDirectSound8_SetCooperativeLevel() failed: %s\n",
       DXGetErrorString8(rc));
    if (rc!=DS_OK)
        goto EXIT;

    ZeroMemory(&bufdesc, sizeof(bufdesc));
    bufdesc.dwSize=sizeof(bufdesc);
    bufdesc.dwFlags=DSBCAPS_PRIMARYBUFFER;
    rc=IDirectSound8_CreateSoundBuffer(dso,&bufdesc,&primary,NULL);
    ok(rc==DS_OK && primary!=NULL,
       "IDirectSound8_CreateSoundBuffer() failed to create a primary buffer "
       "%s\n",DXGetErrorString8(rc));

    if (rc==DS_OK && primary!=NULL) {
        rc=IDirectSoundBuffer_GetFormat(primary,&wfx1,sizeof(wfx1),NULL);
        ok(rc==DS_OK,"IDirectSoundBuffer8_Getformat() failed: %s\n",
           DXGetErrorString8(rc));
        if (rc!=DS_OK)
            goto EXIT1;

        for (f=0;f<NB_FORMATS;f++) {
            init_format(&wfx,WAVE_FORMAT_PCM,formats[f][0],formats[f][1],
                        formats[f][2]);
            secondary=NULL;
            ZeroMemory(&bufdesc, sizeof(bufdesc));
            bufdesc.dwSize=sizeof(bufdesc);
            bufdesc.dwFlags=DSBCAPS_GETCURRENTPOSITION2;
            bufdesc.dwBufferBytes=align(wfx.nAvgBytesPerSec*BUFFER_LEN/1000,
                                        wfx.nBlockAlign);
            rc=IDirectSound8_CreateSoundBuffer(dso,&bufdesc,&secondary,NULL);
            ok(rc==DSERR_INVALIDPARAM,"IDirectSound8_CreateSoundBuffer() "
               "should have returned DSERR_INVALIDPARAM, returned: %s\n",
               DXGetErrorString8(rc));
            if (rc==DS_OK && secondary!=NULL)
                IDirectSoundBuffer_Release(secondary);

            secondary=NULL;
            ZeroMemory(&bufdesc, sizeof(bufdesc));
            bufdesc.dwSize=sizeof(bufdesc);
            bufdesc.dwFlags=DSBCAPS_GETCURRENTPOSITION2;
            bufdesc.dwBufferBytes=align(wfx.nAvgBytesPerSec*BUFFER_LEN/1000,
                                        wfx.nBlockAlign);
            bufdesc.lpwfxFormat=&wfx;
            if (winetest_interactive) {
                trace("  Testing a secondary buffer at %ldx%dx%d "
                      "with a primary buffer at %ldx%dx%d\n",
                      wfx.nSamplesPerSec,wfx.wBitsPerSample,wfx.nChannels,
                      wfx1.nSamplesPerSec,wfx1.wBitsPerSample,wfx1.nChannels);
            }
            rc=IDirectSound8_CreateSoundBuffer(dso,&bufdesc,&secondary,NULL);
            ok(rc==DS_OK && secondary!=NULL,
               "IDirectSound8_CreateSoundBuffer() failed to create a secondary "
               "buffer: %s\n",DXGetErrorString8(rc));

            if (rc==DS_OK && secondary!=NULL) {
                test_buffer8(dso,secondary,0,FALSE,0,FALSE,0,
                             winetest_interactive,1.0,0,NULL,0,0);

                ref=IDirectSoundBuffer_Release(secondary);
                ok(ref==0,"IDirectSoundBuffer_Release() has %d references, "
                   "should have 0\n",ref);
            }
        }
EXIT1:
        ref=IDirectSoundBuffer_Release(primary);
        ok(ref==0,"IDirectSoundBuffer_Release() primary has %d references, "
           "should have 0\n",ref);
    }

    /* Set the CooperativeLevel back to normal */
    /* DSOUND: Setting DirectSound cooperative level to DSSCL_NORMAL */
    rc=IDirectSound8_SetCooperativeLevel(dso,get_hwnd(),DSSCL_NORMAL);
    ok(rc==DS_OK,"IDirectSound8_SetCooperativeLevel() failed: %s\n",
       DXGetErrorString8(rc));

EXIT:
    ref=IDirectSound8_Release(dso);
    ok(ref==0,"IDirectSound8_Release() has %d references, should have 0\n",ref);
    if (ref!=0)
        return DSERR_GENERIC;

    return rc;
}

static BOOL WINAPI dsenum_callback(LPGUID lpGuid, LPCSTR lpcstrDescription,
                                   LPCSTR lpcstrModule, LPVOID lpContext)
{
    HRESULT rc;
    trace("*** Testing %s - %s ***\n",lpcstrDescription,lpcstrModule);
    rc = test_dsound8(lpGuid);
    if (rc == DSERR_NODRIVER)
        trace("  No Driver\n");
    else if (rc == DSERR_ALLOCATED)
        trace("  Already In Use\n");
    else if (rc == E_FAIL)
        trace("  No Device\n");
    else {
        test_primary8(lpGuid);
        test_primary_secondary8(lpGuid);
        test_secondary8(lpGuid);
    }

    return 1;
}

static void dsound8_tests(void)
{
    HRESULT rc;
    rc=DirectSoundEnumerateA(&dsenum_callback,NULL);
    ok(rc==DS_OK,"DirectSoundEnumerateA() failed: %s\n",DXGetErrorString8(rc));
}

const char * get_file_version(const char * file_name)
{
    static char version[32];
    DWORD size;
    DWORD handle;

    size = GetFileVersionInfoSizeA("dsound.dll", &handle);
    if (size) {
        char * data = HeapAlloc(GetProcessHeap(), 0, size);
        if (data) {
            if (GetFileVersionInfoA("dsound.dll", handle, size, data)) {
                VS_FIXEDFILEINFO *pFixedVersionInfo;
                UINT len;
                if (VerQueryValueA(data, "\\", (LPVOID *)&pFixedVersionInfo, &len)) {
                    sprintf(version, "%ld.%ld.%ld.%ld",
                            pFixedVersionInfo->dwFileVersionMS >> 16,
                            pFixedVersionInfo->dwFileVersionMS & 0xffff,
                            pFixedVersionInfo->dwFileVersionLS >> 16,
                            pFixedVersionInfo->dwFileVersionLS & 0xffff);
                } else
                    sprintf(version, "not available");
            } else
                sprintf(version, "failed");

            HeapFree(GetProcessHeap(), 0, data);
        } else
            sprintf(version, "failed");
    } else
        sprintf(version, "not available");

    return version;
}

START_TEST(dsound8)
{
    HMODULE hDsound;

    CoInitialize(NULL);

    hDsound = LoadLibraryA("dsound.dll");
    if (!hDsound) {
        trace("dsound.dll not found\n");
        return;
    }

    trace("DLL Version: %s\n", get_file_version("dsound.dll"));

    pDirectSoundCreate8 = (void*)GetProcAddress(hDsound, "DirectSoundCreate8");
    if (!pDirectSoundCreate8) {
        trace("dsound8 test skipped\n");
        return;
    }

    IDirectSound8_tests();
    dsound8_tests();

    CoUninitialize();
}
