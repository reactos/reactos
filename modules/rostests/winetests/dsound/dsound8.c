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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "dsound_test.h"

#include <initguid.h>
#include <mmdeviceapi.h>

static HRESULT (WINAPI *pDirectSoundEnumerateA)(LPDSENUMCALLBACKA,LPVOID)=NULL;
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
    DWORD speaker_config, new_speaker_config, ref_speaker_config;
    DWORD certified;

    /* Try to Query for objects */
    rc=IDirectSound8_QueryInterface(dso,&IID_IUnknown,(LPVOID*)&unknown);
    ok(rc==DS_OK,"IDirectSound8_QueryInterface(IID_IUnknown) failed: %08x\n", rc);
    if (rc==DS_OK)
        IDirectSound8_Release(unknown);

    rc=IDirectSound8_QueryInterface(dso,&IID_IDirectSound,(LPVOID*)&ds);
    ok(rc==DS_OK,"IDirectSound8_QueryInterface(IID_IDirectSound) failed: %08x\n", rc);
    if (rc==DS_OK)
        IDirectSound_Release(ds);

    rc=IDirectSound8_QueryInterface(dso,&IID_IDirectSound8,(LPVOID*)&ds8);
    ok(rc==DS_OK,"IDirectSound8_QueryInterface(IID_IDirectSound8) "
       "should have returned DSERR_INVALIDPARAM, returned: %08x\n", rc);
    if (rc==DS_OK)
        IDirectSound8_Release(ds8);

    if (initialized == FALSE) {
        /* try uninitialized object */
        rc=IDirectSound8_GetCaps(dso,0);
        ok(rc==DSERR_UNINITIALIZED,"IDirectSound8_GetCaps(NULL) "
           "should have returned DSERR_UNINITIALIZED, returned: %08x\n", rc);

        rc=IDirectSound8_GetCaps(dso,&dscaps);
        ok(rc==DSERR_UNINITIALIZED,"IDirectSound8_GetCaps() "
           "should have returned DSERR_UNINITIALIZED, returned: %08x\n", rc);

        rc=IDirectSound8_Compact(dso);
        ok(rc==DSERR_UNINITIALIZED,"IDirectSound8_Compact() "
           "should have returned DSERR_UNINITIALIZED, returned: %08x\n", rc);

        rc=IDirectSound8_GetSpeakerConfig(dso,&speaker_config);
        ok(rc==DSERR_UNINITIALIZED,"IDirectSound8_GetSpeakerConfig() "
           "should have returned DSERR_UNINITIALIZED, returned: %08x\n", rc);

        rc=IDirectSound8_VerifyCertification(dso, &certified);
        ok(rc==DSERR_UNINITIALIZED,"IDirectSound8_VerifyCertification() "
           "should have returned DSERR_UNINITIALIZED, returned: %08x\n", rc);

        rc=IDirectSound8_Initialize(dso,lpGuid);
        ok(rc==DS_OK||rc==DSERR_NODRIVER||rc==DSERR_ALLOCATED||rc==E_FAIL,
           "IDirectSound8_Initialize() failed: %08x\n",rc);
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
       "should have returned DSERR_ALREADYINITIALIZED: %08x\n", rc);

    /* DSOUND: Error: Invalid caps buffer */
    rc=IDirectSound8_GetCaps(dso,0);
    ok(rc==DSERR_INVALIDPARAM,"IDirectSound8_GetCaps() "
       "should have returned DSERR_INVALIDPARAM, returned: %08x\n", rc);

    ZeroMemory(&dscaps, sizeof(dscaps));

    /* DSOUND: Error: Invalid caps buffer */
    rc=IDirectSound8_GetCaps(dso,&dscaps);
    ok(rc==DSERR_INVALIDPARAM,"IDirectSound8_GetCaps() "
       "should have returned DSERR_INVALIDPARAM, returned: %08x\n", rc);

    dscaps.dwSize=sizeof(dscaps);

    /* DSOUND: Running on a certified driver */
    rc=IDirectSound8_GetCaps(dso,&dscaps);
    ok(rc==DS_OK,"IDirectSound8_GetCaps() failed: %08x\n",rc);

    rc=IDirectSound8_Compact(dso);
    ok(rc==DSERR_PRIOLEVELNEEDED,"IDirectSound8_Compact() failed: %08x\n", rc);

    rc=IDirectSound8_SetCooperativeLevel(dso,get_hwnd(),DSSCL_PRIORITY);
    ok(rc==DS_OK,"IDirectSound8_SetCooperativeLevel() failed: %08x\n", rc);

    rc=IDirectSound8_Compact(dso);
    ok(rc==DS_OK,"IDirectSound8_Compact() failed: %08x\n",rc);

    rc=IDirectSound8_GetSpeakerConfig(dso,0);
    ok(rc==DSERR_INVALIDPARAM,"IDirectSound8_GetSpeakerConfig(NULL) "
       "should have returned DSERR_INVALIDPARAM, returned: %08x\n", rc);

    rc=IDirectSound8_GetSpeakerConfig(dso,&speaker_config);
    ok(rc==DS_OK,"IDirectSound8_GetSpeakerConfig() failed: %08x\n", rc);
    ref_speaker_config = speaker_config;

    speaker_config = DSSPEAKER_COMBINED(DSSPEAKER_STEREO,
                                        DSSPEAKER_GEOMETRY_WIDE);
    if (speaker_config == ref_speaker_config)
        speaker_config = DSSPEAKER_COMBINED(DSSPEAKER_STEREO,
                                            DSSPEAKER_GEOMETRY_NARROW);
    if(rc==DS_OK) {
        rc=IDirectSound8_SetSpeakerConfig(dso,speaker_config);
        ok(rc==DS_OK,"IDirectSound8_SetSpeakerConfig() failed: %08x\n", rc);
    }
    if (rc==DS_OK) {
        rc=IDirectSound8_GetSpeakerConfig(dso,&new_speaker_config);
        ok(rc==DS_OK,"IDirectSound8_GetSpeakerConfig() failed: %08x\n", rc);
        if (rc==DS_OK && speaker_config!=new_speaker_config)
               trace("IDirectSound8_GetSpeakerConfig() failed to set speaker "
               "config: expected 0x%08x, got 0x%08x\n",
               speaker_config,new_speaker_config);
        IDirectSound8_SetSpeakerConfig(dso,ref_speaker_config);
    }

    rc=IDirectSound8_VerifyCertification(dso, &certified);
    ok(rc==DS_OK||rc==E_NOTIMPL,"IDirectSound8_VerifyCertification() failed: %08x\n", rc);

EXIT:
    ref=IDirectSound8_Release(dso);
    ok(ref==0,"IDirectSound8_Release() has %d references, should have 0\n",ref);
}

static void IDirectSound8_tests(void)
{
    HRESULT rc;
    LPDIRECTSOUND8 dso=NULL;
    LPCLASSFACTORY cf=NULL;

    trace("Testing IDirectSound8\n");

    rc=CoGetClassObject(&CLSID_DirectSound8, CLSCTX_INPROC_SERVER, NULL,
                        &IID_IClassFactory, (void**)&cf);
    ok(rc==S_OK,"CoGetClassObject(CLSID_DirectSound8, IID_IClassFactory) "
       "failed: %08x\n", rc);

    rc=CoGetClassObject(&CLSID_DirectSound8, CLSCTX_INPROC_SERVER, NULL,
                        &IID_IUnknown, (void**)&cf);
    ok(rc==S_OK,"CoGetClassObject(CLSID_DirectSound8, IID_IUnknown) "
       "failed: %08x\n", rc);

    /* try the COM class factory method of creation with no device specified */
    rc=CoCreateInstance(&CLSID_DirectSound8, NULL, CLSCTX_INPROC_SERVER,
                        &IID_IDirectSound8, (void**)&dso);
    ok(rc==S_OK||rc==REGDB_E_CLASSNOTREG,"CoCreateInstance() failed: %08x\n", rc);
    if (rc==REGDB_E_CLASSNOTREG) {
        trace("  Class Not Registered\n");
        return;
    }
    if (dso)
        IDirectSound8_test(dso, FALSE, NULL);

    /* try the COM class factory method of creation with default playback
     *  device specified */
    rc=CoCreateInstance(&CLSID_DirectSound8, NULL, CLSCTX_INPROC_SERVER,
                        &IID_IDirectSound8, (void**)&dso);
    ok(rc==S_OK,"CoCreateInstance(CLSID_DirectSound8) failed: %08x\n", rc);
    if (dso)
        IDirectSound8_test(dso, FALSE, &DSDEVID_DefaultPlayback);

    /* try the COM class factory method of creation with default voice
     * playback device specified */
    rc=CoCreateInstance(&CLSID_DirectSound8, NULL, CLSCTX_INPROC_SERVER,
                        &IID_IDirectSound8, (void**)&dso);
    ok(rc==S_OK,"CoCreateInstance(CLSID_DirectSound8) failed: %08x\n", rc);
    if (dso)
        IDirectSound8_test(dso, FALSE, &DSDEVID_DefaultVoicePlayback);

    /* try the COM class factory method of creation with a bad
     * IID specified */
    rc=CoCreateInstance(&CLSID_DirectSound8, NULL, CLSCTX_INPROC_SERVER,
                        &CLSID_DirectSoundPrivate, (void**)&dso);
    ok(rc==E_NOINTERFACE,
       "CoCreateInstance(CLSID_DirectSound8,CLSID_DirectSoundPrivate) "
       "should have failed: %08x\n",rc);

    /* try the COM class factory method of creation with a bad
     * GUID and IID specified */
    rc=CoCreateInstance(&CLSID_DirectSoundPrivate, NULL, CLSCTX_INPROC_SERVER,
                        &IID_IDirectSound8, (void**)&dso);
    ok(rc==REGDB_E_CLASSNOTREG,
       "CoCreateInstance(CLSID_DirectSoundPrivate,IID_IDirectSound8) "
       "should have failed: %08x\n",rc);

    /* try with no device specified */
    rc=pDirectSoundCreate8(NULL,&dso,NULL);
    ok(rc==S_OK||rc==DSERR_NODRIVER||rc==DSERR_ALLOCATED||rc==E_FAIL,
       "DirectSoundCreate8() failed: %08x\n",rc);
    if (rc==DS_OK && dso)
        IDirectSound8_test(dso, TRUE, NULL);

    /* try with default playback device specified */
    rc=pDirectSoundCreate8(&DSDEVID_DefaultPlayback,&dso,NULL);
    ok(rc==S_OK||rc==DSERR_NODRIVER||rc==DSERR_ALLOCATED||rc==E_FAIL,
       "DirectSoundCreate8() failed: %08x\n",rc);
    if (rc==DS_OK && dso)
        IDirectSound8_test(dso, TRUE, NULL);

    /* try with default voice playback device specified */
    rc=pDirectSoundCreate8(&DSDEVID_DefaultVoicePlayback,&dso,NULL);
    ok(rc==S_OK||rc==DSERR_NODRIVER||rc==DSERR_ALLOCATED||rc==E_FAIL,
       "DirectSoundCreate8() failed: %08x\n",rc);
    if (rc==DS_OK && dso)
        IDirectSound8_test(dso, TRUE, NULL);

    /* try with a bad device specified */
    rc=pDirectSoundCreate8(&DSDEVID_DefaultVoiceCapture,&dso,NULL);
    ok(rc==DSERR_NODRIVER,"DirectSoundCreate8(DSDEVID_DefaultVoiceCapture) "
       "should have failed: %08x\n",rc);
}

static HRESULT test_dsound8(LPGUID lpGuid)
{
    HRESULT rc;
    LPDIRECTSOUND8 dso=NULL;
    int ref;

    /* DSOUND: Error: Invalid interface buffer */
    rc=pDirectSoundCreate8(lpGuid,0,NULL);
    ok(rc==DSERR_INVALIDPARAM,"DirectSoundCreate8() should have returned "
       "DSERR_INVALIDPARAM, returned: %08x\n",rc);

    /* Create the DirectSound8 object */
    rc=pDirectSoundCreate8(lpGuid,&dso,NULL);
    ok(rc==DS_OK||rc==DSERR_NODRIVER||rc==DSERR_ALLOCATED||rc==E_FAIL,
       "DirectSoundCreate8() failed: %08x\n",rc);
    if (rc!=DS_OK)
        return rc;

    /* Try the enumerated device */
    IDirectSound8_test(dso, TRUE, lpGuid);

    /* Try the COM class factory method of creation with enumerated device */
    rc=CoCreateInstance(&CLSID_DirectSound8, NULL, CLSCTX_INPROC_SERVER,
                        &IID_IDirectSound8, (void**)&dso);
    ok(rc==S_OK,"CoCreateInstance(CLSID_DirectSound) failed: %08x\n", rc);
    if (dso)
        IDirectSound8_test(dso, FALSE, lpGuid);

    /* Create a DirectSound8 object */
    rc=pDirectSoundCreate8(lpGuid,&dso,NULL);
    ok(rc==DS_OK,"DirectSoundCreate8() failed: %08x\n",rc);
    if (rc==DS_OK) {
        LPDIRECTSOUND8 dso1=NULL;

        /* Create a second DirectSound8 object */
        rc=pDirectSoundCreate8(lpGuid,&dso1,NULL);
        ok(rc==DS_OK,"DirectSoundCreate8() failed: %08x\n",rc);
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
    ok(rc==DS_OK,"DirectSoundCreate8() failed: %08x\n",rc);
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
           "buffer: %08x\n",rc);
        if (rc==DS_OK && secondary!=NULL) {
            LPDIRECTSOUND3DBUFFER buffer3d;
            LPDIRECTSOUNDBUFFER8 buffer8;
            rc=IDirectSound8_QueryInterface(secondary,
                                            &IID_IDirectSound3DBuffer,
                                            (void **)&buffer3d);
            ok(rc==DS_OK && buffer3d!=NULL,
               "IDirectSound8_QueryInterface() failed: %08x\n", rc);
            if (rc==DS_OK && buffer3d!=NULL) {
                ref=IDirectSound3DBuffer_AddRef(buffer3d);
                ok(ref==2,"IDirectSound3DBuffer_AddRef() has %d references, "
                   "should have 2\n",ref);
            }
            rc=IDirectSound8_QueryInterface(secondary,
                                            &IID_IDirectSoundBuffer8,
                                            (void **)&buffer8);
            if (rc==DS_OK && buffer8!=NULL) {
                ok(buffer8==(IDirectSoundBuffer8*)secondary,
                   "IDirectSoundBuffer8 iface different from IDirectSoundBuffer.\n");
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
    LPDIRECTSOUNDBUFFER8 pb8 = NULL;
    DSBUFFERDESC bufdesc;
    DSCAPS dscaps;
    WAVEFORMATEX wfx;
    int ref;

    /* Create the DirectSound object */
    rc=pDirectSoundCreate8(lpGuid,&dso,NULL);
    ok(rc==DS_OK||rc==DSERR_NODRIVER||rc==DSERR_ALLOCATED,
       "DirectSoundCreate8() failed: %08x\n",rc);
    if (rc!=DS_OK)
        return rc;

    /* Get the device capabilities */
    ZeroMemory(&dscaps, sizeof(dscaps));
    dscaps.dwSize=sizeof(dscaps);
    rc=IDirectSound8_GetCaps(dso,&dscaps);
    ok(rc==DS_OK,"IDirectSound8_GetCaps() failed: %08x\n",rc);
    if (rc!=DS_OK)
        goto EXIT;

    /* DSOUND: Error: Invalid buffer description pointer */
    rc=IDirectSound8_CreateSoundBuffer(dso,0,0,NULL);
    ok(rc==DSERR_INVALIDPARAM,
       "IDirectSound8_CreateSoundBuffer should have returned "
       "DSERR_INVALIDPARAM, returned: %08x\n",rc);

    /* DSOUND: Error: Invalid buffer description pointer */
    rc=IDirectSound8_CreateSoundBuffer(dso,0,&primary,NULL);
    ok(rc==DSERR_INVALIDPARAM && primary==0,
       "IDirectSound8_CreateSoundBuffer() should have returned "
       "DSERR_INVALIDPARAM, returned: rc=%08x,dsbo=%p\n",
       rc,primary);

    ZeroMemory(&bufdesc, sizeof(bufdesc));
    bufdesc.dwSize = sizeof(DSBUFFERDESC);

    /* DSOUND: Error: Invalid dsound buffer interface pointer */
    rc=IDirectSound8_CreateSoundBuffer(dso,&bufdesc,0,NULL);
    ok(rc==DSERR_INVALIDPARAM && primary==0,
       "IDirectSound8_CreateSoundBuffer() should have failed: rc=%08x,"
       "dsbo=%p\n",rc,primary);

    ZeroMemory(&bufdesc, sizeof(bufdesc));

    /* DSOUND: Error: Invalid size */
    /* DSOUND: Error: Invalid buffer description */
    rc=IDirectSound8_CreateSoundBuffer(dso,&bufdesc,&primary,NULL);
    ok(rc==DSERR_INVALIDPARAM && primary==0,
       "IDirectSound8_CreateSoundBuffer() should have failed: rc=%08x,"
       "primary=%p\n",rc,primary);

    /* We must call SetCooperativeLevel before calling CreateSoundBuffer */
    /* DSOUND: Setting DirectSound cooperative level to DSSCL_PRIORITY */
    rc=IDirectSound8_SetCooperativeLevel(dso,get_hwnd(),DSSCL_PRIORITY);
    ok(rc==DS_OK,"IDirectSound8_SetCooperativeLevel() failed: %08x\n", rc);
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
       "returned DSERR_INVALIDPARAM, returned: %08x\n", rc);
    if (rc==DS_OK && primary!=NULL)
        IDirectSoundBuffer_Release(primary);

    primary=NULL;
    ZeroMemory(&bufdesc, sizeof(bufdesc));
    bufdesc.dwSize=sizeof(bufdesc);
    bufdesc.dwFlags=DSBCAPS_PRIMARYBUFFER|DSBCAPS_CTRLVOLUME;
    rc=IDirectSound8_CreateSoundBuffer(dso,&bufdesc,&primary,NULL);
    ok((rc==DS_OK && primary!=NULL) || (rc==DSERR_CONTROLUNAVAIL),
       "IDirectSound8_CreateSoundBuffer() failed to create a primary buffer: "
       "%08x\n",rc);
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
           "primary buffer: %08x\n",rc);
        ref=IDirectSoundBuffer_Release(second);
        ok(ref==1,"IDirectSoundBuffer_Release() primary has %d references, "
           "should have 1\n",ref);

        /* Try to duplicate a primary buffer */
        /* DSOUND: Error: Can't duplicate primary buffers */
        rc=IDirectSound8_DuplicateSoundBuffer(dso,primary,&third);
        /* rc=0x88780032 */
        ok(rc!=DS_OK,"IDirectSound8_DuplicateSoundBuffer() primary buffer "
           "should have failed %08x\n",rc);

        /* Primary buffers don't have an IDirectSoundBuffer8 */
        rc = IDirectSoundBuffer_QueryInterface(primary, &IID_IDirectSoundBuffer8, (LPVOID*)&pb8);
        ok(FAILED(rc), "Primary buffer does have an IDirectSoundBuffer8: %08x\n", rc);

        rc=IDirectSoundBuffer_GetVolume(primary,&vol);
        ok(rc==DS_OK,"IDirectSoundBuffer_GetVolume() failed: %08x\n", rc);

        if (winetest_interactive) {
            trace("Playing a 5 seconds reference tone at the current volume.\n");
            if (rc==DS_OK)
                trace("(the current volume is %d according to DirectSound)\n",
                      vol);
            trace("All subsequent tones should be identical to this one.\n");
            trace("Listen for stutter, changes in pitch, volume, etc.\n");
        }
        test_buffer8(dso,&primary,1,FALSE,0,FALSE,0,winetest_interactive &&
                     !(dscaps.dwFlags & DSCAPS_EMULDRIVER),5.0,0,0,0,0);

        ref=IDirectSoundBuffer_Release(primary);
        ok(ref==0,"IDirectSoundBuffer_Release() primary has %d references, "
           "should have 0\n",ref);
    }

    /* Set the CooperativeLevel back to normal */
    /* DSOUND: Setting DirectSound cooperative level to DSSCL_NORMAL */
    rc=IDirectSound8_SetCooperativeLevel(dso,get_hwnd(),DSSCL_NORMAL);
    ok(rc==DS_OK,"IDirectSound8_SetCooperativeLevel() failed: %08x\n", rc);

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
    unsigned int f, tag;

    /* Create the DirectSound object */
    rc=pDirectSoundCreate8(lpGuid,&dso,NULL);
    ok(rc==DS_OK||rc==DSERR_NODRIVER||rc==DSERR_ALLOCATED,
       "DirectSoundCreate8() failed: %08x\n",rc);
    if (rc!=DS_OK)
        return rc;

    /* Get the device capabilities */
    ZeroMemory(&dscaps, sizeof(dscaps));
    dscaps.dwSize=sizeof(dscaps);
    rc=IDirectSound8_GetCaps(dso,&dscaps);
    ok(rc==DS_OK,"IDirectSound8_GetCaps() failed: %08x\n",rc);
    if (rc!=DS_OK)
        goto EXIT;

    /* We must call SetCooperativeLevel before creating primary buffer */
    /* DSOUND: Setting DirectSound cooperative level to DSSCL_PRIORITY */
    rc=IDirectSound8_SetCooperativeLevel(dso,get_hwnd(),DSSCL_PRIORITY);
    ok(rc==DS_OK,"IDirectSound8_SetCooperativeLevel() failed: %08x\n", rc);
    if (rc!=DS_OK)
        goto EXIT;

    ZeroMemory(&bufdesc, sizeof(bufdesc));
    bufdesc.dwSize=sizeof(bufdesc);
    bufdesc.dwFlags=DSBCAPS_PRIMARYBUFFER;
    rc=IDirectSound8_CreateSoundBuffer(dso,&bufdesc,&primary,NULL);
    ok(rc==DS_OK && primary!=NULL,
       "IDirectSound8_CreateSoundBuffer() failed to create a primary buffer "
       "%08x\n",rc);

    if (rc==DS_OK && primary!=NULL) {
        for (f=0;f<NB_FORMATS;f++) {
          for (tag=0;tag<NB_TAGS;tag++) {
            /* if float, we only want to test 32-bit */
            if ((format_tags[tag] == WAVE_FORMAT_IEEE_FLOAT) && (formats[f][1] != 32))
                continue;

            /* We must call SetCooperativeLevel to be allowed to call
             * SetFormat */
            /* DSOUND: Setting DirectSound cooperative level to
             * DSSCL_PRIORITY */
            rc=IDirectSound8_SetCooperativeLevel(dso,get_hwnd(),DSSCL_PRIORITY);
            ok(rc==DS_OK,"IDirectSound8_SetCooperativeLevel() failed: %08x\n", rc);
            if (rc!=DS_OK)
                goto EXIT;

            init_format(&wfx,format_tags[tag],formats[f][0],formats[f][1],
                        formats[f][2]);
            wfx2=wfx;
            rc=IDirectSoundBuffer_SetFormat(primary,&wfx);
            ok(rc==DS_OK
               || rc==DSERR_INVALIDPARAM, /* 2003 */
               "IDirectSoundBuffer_SetFormat(%s) failed: %08x\n",
               format_string(&wfx), rc);

            /* There is no guarantee that SetFormat will actually change the
             * format to what we asked for. It depends on what the soundcard
             * supports. So we must re-query the format.
             */
            rc=IDirectSoundBuffer_GetFormat(primary,&wfx,sizeof(wfx),NULL);
            ok(rc==DS_OK,"IDirectSoundBuffer_GetFormat() failed: %08x\n", rc);
            if (rc==DS_OK &&
                (wfx.wFormatTag!=wfx2.wFormatTag ||
                 wfx.nSamplesPerSec!=wfx2.nSamplesPerSec ||
                 wfx.wBitsPerSample!=wfx2.wBitsPerSample ||
                 wfx.nChannels!=wfx2.nChannels)) {
                trace("Requested primary format tag=0x%04x %dx%dx%d "
                      "avg.B/s=%d align=%d\n",
                      wfx2.wFormatTag,wfx2.nSamplesPerSec,wfx2.wBitsPerSample,
                      wfx2.nChannels,wfx2.nAvgBytesPerSec,wfx2.nBlockAlign);
                trace("Got tag=0x%04x %dx%dx%d avg.B/s=%d align=%d\n",
                      wfx.wFormatTag,wfx.nSamplesPerSec,wfx.wBitsPerSample,
                      wfx.nChannels,wfx.nAvgBytesPerSec,wfx.nBlockAlign);
            }

            /* Set the CooperativeLevel back to normal */
            /* DSOUND: Setting DirectSound cooperative level to DSSCL_NORMAL */
            rc=IDirectSound8_SetCooperativeLevel(dso,get_hwnd(),DSSCL_NORMAL);
            ok(rc==DS_OK,"IDirectSound8_SetCooperativeLevel() failed: %08x\n", rc);

            init_format(&wfx2,WAVE_FORMAT_PCM,11025,16,2);

            secondary=NULL;
            ZeroMemory(&bufdesc, sizeof(bufdesc));
            bufdesc.dwSize=sizeof(bufdesc);
            bufdesc.dwFlags=DSBCAPS_GETCURRENTPOSITION2;
            bufdesc.dwBufferBytes=align(wfx.nAvgBytesPerSec*BUFFER_LEN/1000,
                                        wfx.nBlockAlign);
            bufdesc.lpwfxFormat=&wfx2;
            if (winetest_interactive) {
                trace("  Testing a primary buffer at %dx%dx%d (fmt=%d) with a "
                      "secondary buffer at %dx%dx%d\n",
                      wfx.nSamplesPerSec,wfx.wBitsPerSample,wfx.nChannels,format_tags[tag],
                      wfx2.nSamplesPerSec,wfx2.wBitsPerSample,wfx2.nChannels);
            }
            rc=IDirectSound_CreateSoundBuffer(dso,&bufdesc,&secondary,NULL);
            ok(rc==DS_OK && secondary!=NULL,
               "IDirectSound_CreateSoundBuffer() failed to create a secondary "
               "buffer %08x\n",rc);

            if (rc==DS_OK && secondary!=NULL) {
                test_buffer8(dso,&secondary,0,FALSE,0,FALSE,0,
                             winetest_interactive,1.0,0,NULL,0,0);

                ref=IDirectSoundBuffer_Release(secondary);
                ok(ref==0,"IDirectSoundBuffer_Release() has %d references, "
                   "should have 0\n",ref);
            }
          }
        }

        ref=IDirectSoundBuffer_Release(primary);
        ok(ref==0,"IDirectSoundBuffer_Release() primary has %d references, "
           "should have 0\n",ref);
    }

    /* Set the CooperativeLevel back to normal */
    /* DSOUND: Setting DirectSound cooperative level to DSSCL_NORMAL */
    rc=IDirectSound8_SetCooperativeLevel(dso,get_hwnd(),DSSCL_NORMAL);
    ok(rc==DS_OK,"IDirectSound8_SetCooperativeLevel() failed: %08x\n", rc);

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
    DWORD f, tag;
    int ref;

    /* Create the DirectSound object */
    rc=pDirectSoundCreate8(lpGuid,&dso,NULL);
    ok(rc==DS_OK||rc==DSERR_NODRIVER||rc==DSERR_ALLOCATED,
       "DirectSoundCreate8() failed: %08x\n",rc);
    if (rc!=DS_OK)
        return rc;

    /* Get the device capabilities */
    ZeroMemory(&dscaps, sizeof(dscaps));
    dscaps.dwSize=sizeof(dscaps);
    rc=IDirectSound8_GetCaps(dso,&dscaps);
    ok(rc==DS_OK,"IDirectSound8_GetCaps() failed: %08x\n",rc);
    if (rc!=DS_OK)
        goto EXIT;

    /* We must call SetCooperativeLevel before creating primary buffer */
    /* DSOUND: Setting DirectSound cooperative level to DSSCL_PRIORITY */
    rc=IDirectSound8_SetCooperativeLevel(dso,get_hwnd(),DSSCL_PRIORITY);
    ok(rc==DS_OK,"IDirectSound8_SetCooperativeLevel() failed: %08x\n", rc);
    if (rc!=DS_OK)
        goto EXIT;

    ZeroMemory(&bufdesc, sizeof(bufdesc));
    bufdesc.dwSize=sizeof(bufdesc);
    bufdesc.dwFlags=DSBCAPS_PRIMARYBUFFER;
    rc=IDirectSound8_CreateSoundBuffer(dso,&bufdesc,&primary,NULL);
    ok(rc==DS_OK && primary!=NULL,
       "IDirectSound8_CreateSoundBuffer() failed to create a primary buffer "
       "%08x\n",rc);

    if (rc==DS_OK && primary!=NULL) {
        rc=IDirectSoundBuffer_GetFormat(primary,&wfx1,sizeof(wfx1),NULL);
        ok(rc==DS_OK,"IDirectSoundBuffer8_Getformat() failed: %08x\n", rc);
        if (rc!=DS_OK)
            goto EXIT1;

        for (f=0;f<NB_FORMATS;f++) {
          for (tag=0;tag<NB_TAGS;tag++) {
            WAVEFORMATEXTENSIBLE wfxe;

            /* if float, we only want to test 32-bit */
            if ((format_tags[tag] == WAVE_FORMAT_IEEE_FLOAT) && (formats[f][1] != 32))
                continue;

            init_format(&wfx,format_tags[tag],formats[f][0],formats[f][1],
                        formats[f][2]);
            secondary=NULL;
            ZeroMemory(&bufdesc, sizeof(bufdesc));
            bufdesc.dwSize=sizeof(bufdesc);
            bufdesc.dwFlags=DSBCAPS_GETCURRENTPOSITION2;
            bufdesc.dwBufferBytes=align(wfx.nAvgBytesPerSec*BUFFER_LEN/1000,
                                        wfx.nBlockAlign);
            rc=IDirectSound8_CreateSoundBuffer(dso,&bufdesc,&secondary,NULL);
            ok(rc==DSERR_INVALIDPARAM,"IDirectSound8_CreateSoundBuffer() "
               "should have returned DSERR_INVALIDPARAM, returned: %08x\n", rc);
            if (rc==DS_OK && secondary!=NULL)
                IDirectSoundBuffer_Release(secondary);

            secondary=NULL;
            ZeroMemory(&bufdesc, sizeof(bufdesc));
            bufdesc.dwSize=sizeof(bufdesc);
            bufdesc.dwFlags=DSBCAPS_GETCURRENTPOSITION2;
            bufdesc.dwBufferBytes=align(wfx.nAvgBytesPerSec*BUFFER_LEN/1000,
                                        wfx.nBlockAlign);
            bufdesc.lpwfxFormat=&wfx;
            rc=IDirectSound_CreateSoundBuffer(dso,&bufdesc,&secondary,NULL);
            if (wfx.wBitsPerSample != 8 && wfx.wBitsPerSample != 16)
                ok(((rc == DSERR_CONTROLUNAVAIL || rc == DSERR_INVALIDCALL || rc == DSERR_INVALIDPARAM /* 2003 */) && !secondary)
                    || rc == DS_OK, /* driver dependent? */
                    "IDirectSound_CreateSoundBuffer() "
                    "should have returned (DSERR_CONTROLUNAVAIL or DSERR_INVALIDCALL) "
                    "and NULL, returned: %08x %p\n", rc, secondary);
            else
                ok(rc==DS_OK && secondary!=NULL,
                    "IDirectSound_CreateSoundBuffer() failed to create a secondary "
                    "buffer %08x\n",rc);
            if (secondary)
                IDirectSoundBuffer_Release(secondary);
            secondary = NULL;

            bufdesc.lpwfxFormat=(WAVEFORMATEX*)&wfxe;
            wfxe.Format = wfx;
            wfxe.Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
            wfxe.SubFormat = (format_tags[tag] == WAVE_FORMAT_PCM ? KSDATAFORMAT_SUBTYPE_PCM : KSDATAFORMAT_SUBTYPE_IEEE_FLOAT);
            wfxe.Format.cbSize = 1;
            wfxe.Samples.wValidBitsPerSample = wfx.wBitsPerSample;
            wfxe.dwChannelMask = (wfx.nChannels == 1 ? KSAUDIO_SPEAKER_MONO : KSAUDIO_SPEAKER_STEREO);

            rc=IDirectSound_CreateSoundBuffer(dso,&bufdesc,&secondary,NULL);
            ok(rc==DSERR_INVALIDPARAM && !secondary,
                "IDirectSound_CreateSoundBuffer() returned: %08x %p\n",
                rc, secondary);
            if (secondary)
            {
                IDirectSoundBuffer_Release(secondary);
                secondary=NULL;
            }

            wfxe.Format.cbSize = sizeof(wfxe) - sizeof(wfx) + 1;

            rc=IDirectSound_CreateSoundBuffer(dso,&bufdesc,&secondary,NULL);
            ok(((rc==DSERR_CONTROLUNAVAIL || rc==DSERR_INVALIDCALL /* 2003 */) && !secondary)
                || rc==DS_OK /* driver dependent? */,
                "IDirectSound_CreateSoundBuffer() returned: %08x %p\n",
                rc, secondary);
            if (secondary)
            {
                IDirectSoundBuffer_Release(secondary);
                secondary=NULL;
            }

            wfxe.Format.cbSize = sizeof(wfxe) - sizeof(wfx);
            wfxe.SubFormat = GUID_NULL;
            rc=IDirectSound_CreateSoundBuffer(dso,&bufdesc,&secondary,NULL);
            ok((rc==DSERR_INVALIDPARAM || rc==DSERR_INVALIDCALL) && !secondary,
                "IDirectSound_CreateSoundBuffer() returned: %08x %p\n",
                rc, secondary);
            if (secondary)
            {
                IDirectSoundBuffer_Release(secondary);
                secondary=NULL;
            }

            wfxe.Format.cbSize = sizeof(wfxe);
            rc=IDirectSound_CreateSoundBuffer(dso,&bufdesc,&secondary,NULL);
            ok((rc==DSERR_CONTROLUNAVAIL || rc==DSERR_INVALIDCALL || rc==E_INVALIDARG) && !secondary,
                "IDirectSound_CreateSoundBuffer() returned: %08x %p\n",
                rc, secondary);
            if (secondary)
            {
                IDirectSoundBuffer_Release(secondary);
                secondary=NULL;
            }

            wfxe.SubFormat = (format_tags[tag] == WAVE_FORMAT_PCM ? KSDATAFORMAT_SUBTYPE_PCM : KSDATAFORMAT_SUBTYPE_IEEE_FLOAT);
            rc=IDirectSound_CreateSoundBuffer(dso,&bufdesc,&secondary,NULL);
            ok(rc==DS_OK && secondary,
                "IDirectSound_CreateSoundBuffer() returned: %08x %p\n",
                rc, secondary);
            if (secondary)
            {
                IDirectSoundBuffer_Release(secondary);
                secondary=NULL;
            }

            wfxe.Format.cbSize = sizeof(wfxe) + 1;
            rc=IDirectSound_CreateSoundBuffer(dso,&bufdesc,&secondary,NULL);
            ok(((rc==DSERR_CONTROLUNAVAIL || rc==DSERR_INVALIDCALL /* 2003 */) && !secondary)
                || rc==DS_OK /* driver dependent? */,
                "IDirectSound_CreateSoundBuffer() returned: %08x %p\n",
                rc, secondary);
            if (secondary)
            {
                IDirectSoundBuffer_Release(secondary);
                secondary=NULL;
            }

            wfxe.Format.cbSize = sizeof(wfxe) - sizeof(wfx);
            ++wfxe.Samples.wValidBitsPerSample;
            rc=IDirectSound_CreateSoundBuffer(dso,&bufdesc,&secondary,NULL);
            ok(rc==DSERR_INVALIDPARAM && !secondary,
                "IDirectSound_CreateSoundBuffer() returned: %08x %p\n",
                rc, secondary);
            if (secondary)
            {
                IDirectSoundBuffer_Release(secondary);
                secondary=NULL;
            }
            --wfxe.Samples.wValidBitsPerSample;

            wfxe.Samples.wValidBitsPerSample = 0;
            rc=IDirectSound_CreateSoundBuffer(dso,&bufdesc,&secondary,NULL);
            ok(rc==DS_OK && secondary,
                "IDirectSound_CreateSoundBuffer() returned: %08x %p\n",
                rc, secondary);
            if (secondary)
            {
                IDirectSoundBuffer_Release(secondary);
                secondary=NULL;
            }
            wfxe.Samples.wValidBitsPerSample = wfxe.Format.wBitsPerSample;

            rc=IDirectSound_CreateSoundBuffer(dso,&bufdesc,&secondary,NULL);
            ok(rc==DS_OK && secondary!=NULL,
                "IDirectSound_CreateSoundBuffer() failed to create a secondary "
                "buffer %08x\n",rc);

            if (rc==DS_OK && secondary!=NULL) {
                if (winetest_interactive) {
                    trace("  Testing a secondary buffer at %dx%dx%d (fmt=%d) "
                        "with a primary buffer at %dx%dx%d\n",
                        wfx.nSamplesPerSec,wfx.wBitsPerSample,wfx.nChannels,format_tags[tag],
                        wfx1.nSamplesPerSec,wfx1.wBitsPerSample,wfx1.nChannels);
                }
                test_buffer8(dso,&secondary,0,FALSE,0,FALSE,0,
                             winetest_interactive,1.0,0,NULL,0,0);

                ref=IDirectSoundBuffer_Release(secondary);
                ok(ref==0,"IDirectSoundBuffer_Release() has %d references, "
                   "should have 0\n",ref);
            }
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
    ok(rc==DS_OK,"IDirectSound8_SetCooperativeLevel() failed: %08x\n", rc);

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
    rc=pDirectSoundEnumerateA(&dsenum_callback,NULL);
    ok(rc==DS_OK,"DirectSoundEnumerateA() failed: %08x\n",rc);
}

static void test_hw_buffers(void)
{
    IDirectSound8 *ds;
    IDirectSoundBuffer *primary, *primary2, **secondaries, *secondary;
    IDirectSoundBuffer8 *buf8;
    DSCAPS caps;
    DSBCAPS bufcaps;
    DSBUFFERDESC bufdesc;
    WAVEFORMATEX fmt;
    UINT i;
    HRESULT hr;

    hr = pDirectSoundCreate8(NULL, &ds, NULL);
    ok(hr == S_OK || hr == DSERR_NODRIVER || hr == DSERR_ALLOCATED || hr == E_FAIL,
            "DirectSoundCreate8 failed: %08x\n", hr);
    if(hr != S_OK)
        return;

    caps.dwSize = sizeof(caps);

    hr = IDirectSound8_GetCaps(ds, &caps);
    ok(hr == S_OK, "GetCaps failed: %08x\n", hr);

    ok(caps.dwPrimaryBuffers == 1, "Got wrong number of primary buffers: %u\n",
            caps.dwPrimaryBuffers);

    /* DSBCAPS_LOC* is ignored for primary buffers */
    bufdesc.dwSize = sizeof(bufdesc);
    bufdesc.dwFlags = DSBCAPS_GETCURRENTPOSITION2 | DSBCAPS_LOCHARDWARE |
        DSBCAPS_PRIMARYBUFFER;
    bufdesc.dwBufferBytes = 0;
    bufdesc.dwReserved = 0;
    bufdesc.lpwfxFormat = NULL;
    bufdesc.guid3DAlgorithm = GUID_NULL;

    hr = IDirectSound8_CreateSoundBuffer(ds, &bufdesc, &primary, NULL);
    ok(hr == S_OK, "CreateSoundBuffer failed: %08x\n", hr);
    if(hr != S_OK){
        IDirectSound8_Release(ds);
        return;
    }

    bufdesc.dwFlags = DSBCAPS_GETCURRENTPOSITION2 | DSBCAPS_LOCSOFTWARE |
        DSBCAPS_PRIMARYBUFFER;

    hr = IDirectSound8_CreateSoundBuffer(ds, &bufdesc, &primary2, NULL);
    ok(hr == S_OK, "CreateSoundBuffer failed: %08x\n", hr);
    ok(primary == primary2, "Got different primary buffers: %p, %p\n", primary, primary2);
    if(hr == S_OK)
        IDirectSoundBuffer_Release(primary2);

    buf8 = (IDirectSoundBuffer8 *)0xDEADBEEF;
    hr = IDirectSoundBuffer_QueryInterface(primary, &IID_IDirectSoundBuffer8,
            (void**)&buf8);
    ok(hr == E_NOINTERFACE, "QueryInterface gave wrong failure: %08x\n", hr);
    ok(buf8 == NULL, "Pointer didn't get set to NULL\n");

    fmt.wFormatTag = WAVE_FORMAT_PCM;
    fmt.nChannels = 2;
    fmt.nSamplesPerSec = 48000;
    fmt.wBitsPerSample = 16;
    fmt.nBlockAlign = fmt.nChannels * fmt.wBitsPerSample / 8;
    fmt.nAvgBytesPerSec = fmt.nBlockAlign * fmt.nSamplesPerSec;
    fmt.cbSize = 0;

    bufdesc.lpwfxFormat = &fmt;
    bufdesc.dwBufferBytes = fmt.nSamplesPerSec * fmt.nBlockAlign / 10;
    bufdesc.dwFlags = DSBCAPS_GETCURRENTPOSITION2 | DSBCAPS_LOCHARDWARE |
        DSBCAPS_CTRLVOLUME;

    secondaries = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
            sizeof(IDirectSoundBuffer *) * caps.dwMaxHwMixingAllBuffers);

    /* try to fill all of the hw buffers */
    trace("dwMaxHwMixingAllBuffers: %u\n", caps.dwMaxHwMixingAllBuffers);
    trace("dwMaxHwMixingStaticBuffers: %u\n", caps.dwMaxHwMixingStaticBuffers);
    trace("dwMaxHwMixingStreamingBuffers: %u\n", caps.dwMaxHwMixingStreamingBuffers);
    for(i = 0; i < caps.dwMaxHwMixingAllBuffers; ++i){
        hr = IDirectSound8_CreateSoundBuffer(ds, &bufdesc, &secondaries[i], NULL);
        ok(hr == S_OK || hr == E_NOTIMPL || broken(hr == DSERR_CONTROLUNAVAIL) || broken(hr == E_FAIL),
                "CreateSoundBuffer(%u) failed: %08x\n", i, hr);
        if(hr != S_OK)
            break;

        bufcaps.dwSize = sizeof(bufcaps);
        hr = IDirectSoundBuffer_GetCaps(secondaries[i], &bufcaps);
        ok(hr == S_OK, "GetCaps failed: %08x\n", hr);
        ok((bufcaps.dwFlags & DSBCAPS_LOCHARDWARE) != 0,
                "Buffer wasn't allocated in hardware, dwFlags: %x\n", bufcaps.dwFlags);
    }

    /* see if we can create one more */
    hr = IDirectSound8_CreateSoundBuffer(ds, &bufdesc, &secondary, NULL);
    ok((i == caps.dwMaxHwMixingAllBuffers && hr == DSERR_ALLOCATED) || /* out of hw buffers */
            (caps.dwMaxHwMixingAllBuffers == 0 && hr == DSERR_INVALIDCALL) || /* no hw buffers at all */
            hr == E_NOTIMPL || /* don't support hw buffers */
            broken(hr == DSERR_CONTROLUNAVAIL) || /* vmware winxp, others? */
            broken(hr == E_FAIL) || /* broken AC97 driver */
            broken(hr == S_OK) /* broken driver allows more hw bufs than dscaps claims */,
            "CreateSoundBuffer(%u) gave wrong error: %08x\n", i, hr);
    if(hr == S_OK)
        IDirectSoundBuffer_Release(secondary);

    for(i = 0; i < caps.dwMaxHwMixingAllBuffers; ++i)
        if(secondaries[i])
            IDirectSoundBuffer_Release(secondaries[i]);

    HeapFree(GetProcessHeap(), 0, secondaries);

    IDirectSoundBuffer_Release(primary);
    IDirectSound8_Release(ds);
}

static struct {
    UINT dev_count;
    GUID guid;
} default_info = { 0 };

static BOOL WINAPI default_device_cb(GUID *guid, const char *desc,
        const char *module, void *user)
{
    trace("guid: %p, desc: %s\n", guid, desc);
    if(!guid)
        ok(default_info.dev_count == 0, "Got NULL GUID not in first position\n");
    else{
        if(default_info.dev_count == 0){
            ok(IsEqualGUID(guid, &default_info.guid), "Expected default device GUID\n");
        }else{
            ok(!IsEqualGUID(guid, &default_info.guid), "Got default GUID at unexpected location: %u\n",
                    default_info.dev_count);
        }

        /* only count real devices */
        ++default_info.dev_count;
    }

    return TRUE;
}

static void test_first_device(void)
{
    IMMDeviceEnumerator *devenum;
    IMMDevice *defdev;
    IPropertyStore *ps;
    PROPVARIANT pv;
    HRESULT hr;

    hr = CoCreateInstance(&CLSID_MMDeviceEnumerator, NULL,
            CLSCTX_INPROC_SERVER, &IID_IMMDeviceEnumerator, (void**)&devenum);
    if(FAILED(hr)){
        win_skip("MMDevAPI is not available, skipping default device test\n");
        return;
    }

    hr = IMMDeviceEnumerator_GetDefaultAudioEndpoint(devenum, eRender,
            eMultimedia, &defdev);
    if (hr == E_NOTFOUND) {
        win_skip("No default device found\n");
        return;
    }
    ok(hr == S_OK, "GetDefaultAudioEndpoint failed: %08x\n", hr);

    hr = IMMDevice_OpenPropertyStore(defdev, STGM_READ, &ps);
    ok(hr == S_OK, "OpenPropertyStore failed: %08x\n", hr);

    PropVariantInit(&pv);

    hr = IPropertyStore_GetValue(ps, &PKEY_AudioEndpoint_GUID, &pv);
    ok(hr == S_OK, "GetValue failed: %08x\n", hr);

    CLSIDFromString(pv.pwszVal, &default_info.guid);

    PropVariantClear(&pv);
    IPropertyStore_Release(ps);
    IMMDevice_Release(defdev);
    IMMDeviceEnumerator_Release(devenum);

    hr = pDirectSoundEnumerateA(&default_device_cb, NULL);
    ok(hr == S_OK, "DirectSoundEnumerateA failed: %08x\n", hr);
}

static void test_COM(void)
{
    IDirectSound *ds;
    IDirectSound8 *ds8 = (IDirectSound8*)0xdeadbeef;
    IUnknown *unk, *unk8;
    ULONG refcount;
    HRESULT hr;

    /* COM aggregation */
    hr = CoCreateInstance(&CLSID_DirectSound8, (IUnknown*)&ds, CLSCTX_INPROC_SERVER,
            &IID_IUnknown, (void**)&ds8);
    ok(hr == CLASS_E_NOAGGREGATION,
            "DirectSound create failed: %08x, expected CLASS_E_NOAGGREGATION\n", hr);
    ok(!ds8, "ds8 = %p\n", ds8);

    /* Invalid RIID */
    hr = CoCreateInstance(&CLSID_DirectSound8, NULL, CLSCTX_INPROC_SERVER,
            &IID_IDirectSound3DBuffer, (void**)&ds8);
    ok(hr == E_NOINTERFACE,
            "DirectSound create failed: %08x, expected E_NOINTERFACE\n", hr);

    /* Same refcount for IDirectSound and IDirectSound8 */
    hr = CoCreateInstance(&CLSID_DirectSound8, NULL, CLSCTX_INPROC_SERVER, &IID_IDirectSound8,
            (void**)&ds8);
    ok(hr == S_OK, "DirectSound create failed: %08x, expected S_OK\n", hr);
    refcount = IDirectSound8_AddRef(ds8);
    ok(refcount == 2, "refcount == %u, expected 2\n", refcount);
    hr = IDirectSound8_QueryInterface(ds8, &IID_IDirectSound, (void**)&ds);
    ok(hr == S_OK, "QueryInterface for IID_IDirectSound failed: %08x\n", hr);
    refcount = IDirectSound8_AddRef(ds8);
    ok(refcount == 4, "refcount == %u, expected 4\n", refcount);
    refcount = IDirectSound_AddRef(ds);
    ok(refcount == 5, "refcount == %u, expected 5\n", refcount);

    /* Separate refcount for IUnknown */
    hr = IDirectSound_QueryInterface(ds, &IID_IUnknown, (void**)&unk);
    ok(hr == S_OK, "QueryInterface for IID_IUnknown failed: %08x\n", hr);
    refcount = IUnknown_AddRef(unk);
    ok(refcount == 2, "refcount == %u, expected 2\n", refcount);
    hr = IDirectSound_QueryInterface(ds8, &IID_IUnknown, (void**)&unk8);
    ok(hr == S_OK, "QueryInterface for IID_IUnknown failed: %08x\n", hr);
    refcount = IUnknown_AddRef(unk8);
    ok(refcount == 4, "refcount == %u, expected 4\n", refcount);
    refcount = IDirectSound_AddRef(ds);
    ok(refcount == 6, "refcount == %u, expected 6\n", refcount);

    while (IDirectSound_Release(ds));
    while (IUnknown_Release(unk));
}

START_TEST(dsound8)
{
    HMODULE hDsound;

    CoInitialize(NULL);

    hDsound = LoadLibrary("dsound.dll");
    if (hDsound)
    {

        pDirectSoundEnumerateA = (void*)GetProcAddress(hDsound,
            "DirectSoundEnumerateA");
        pDirectSoundCreate8 = (void*)GetProcAddress(hDsound,
            "DirectSoundCreate8");
        if (pDirectSoundCreate8)
        {
            test_COM();
            IDirectSound8_tests();
            dsound8_tests();
            test_hw_buffers();
            test_first_device();
        }
        else
            skip("DirectSoundCreate8 missing - skipping all tests\n");

        FreeLibrary(hDsound);
    }
    else
        skip("dsound.dll not found - skipping all tests\n");

    CoUninitialize();
}
