/*
 * Unit tests for capture functions
 *
 * Copyright (c) 2002 Francois Gouget
 * Copyright (c) 2003 Robert Reif
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

#include <stdio.h>

#define NOTIFICATIONS    5

static HRESULT (WINAPI *pDirectSoundCaptureCreate)(LPCGUID,LPDIRECTSOUNDCAPTURE*,LPUNKNOWN)=NULL;
static HRESULT (WINAPI *pDirectSoundCaptureEnumerateA)(LPDSENUMCALLBACKA,LPVOID)=NULL;

static const char * get_format_str(WORD format)
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

const char * format_string(const WAVEFORMATEX* wfx)
{
    static char str[64];

    sprintf(str, "%5dx%2dx%d %s",
	wfx->nSamplesPerSec, wfx->wBitsPerSample, wfx->nChannels,
        get_format_str(wfx->wFormatTag));

    return str;
}

static void IDirectSoundCapture_test(LPDIRECTSOUNDCAPTURE dsco,
                                     BOOL initialized, LPCGUID lpGuid)
{
    HRESULT rc;
    DSCCAPS dsccaps;
    int ref;
    IUnknown * unknown;
    IDirectSoundCapture * dsc;

    /* Try to Query for objects */
    rc=IDirectSoundCapture_QueryInterface(dsco, &IID_IUnknown,
                                          (LPVOID*)&unknown);
    ok(rc==DS_OK, "IDirectSoundCapture_QueryInterface(IID_IUnknown) "
       "failed: %08x\n", rc);
    if (rc==DS_OK)
        IDirectSoundCapture_Release(unknown);

    rc=IDirectSoundCapture_QueryInterface(dsco, &IID_IDirectSoundCapture,
                                          (LPVOID*)&dsc);
    ok(rc==DS_OK, "IDirectSoundCapture_QueryInterface(IID_IDirectSoundCapture) "
       "failed: %08x\n", rc);
    if (rc==DS_OK)
        IDirectSoundCapture_Release(dsc);

    if (initialized == FALSE) {
        /* try uninitialized object */
        rc=IDirectSoundCapture_GetCaps(dsco,0);
        ok(rc==DSERR_UNINITIALIZED||rc==E_INVALIDARG,
           "IDirectSoundCapture_GetCaps(NULL) should have returned "
           "DSERR_UNINITIALIZED or E_INVALIDARG, returned: %08x\n", rc);

        rc=IDirectSoundCapture_GetCaps(dsco, &dsccaps);
        ok(rc==DSERR_UNINITIALIZED,"IDirectSoundCapture_GetCaps() "
           "should have returned DSERR_UNINITIALIZED, returned: %08x\n", rc);

        rc=IDirectSoundCapture_Initialize(dsco, lpGuid);
        ok(rc==DS_OK||rc==DSERR_NODRIVER||rc==DSERR_ALLOCATED||
           rc==E_FAIL||rc==E_INVALIDARG,
           "IDirectSoundCapture_Initialize() failed: %08x\n", rc);
        if (rc==DSERR_NODRIVER||rc==E_INVALIDARG) {
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

    rc=IDirectSoundCapture_Initialize(dsco, lpGuid);
    ok(rc==DSERR_ALREADYINITIALIZED, "IDirectSoundCapture_Initialize() "
       "should have returned DSERR_ALREADYINITIALIZED: %08x\n", rc);

    /* DSOUND: Error: Invalid caps buffer */
    rc=IDirectSoundCapture_GetCaps(dsco, 0);
    ok(rc==DSERR_INVALIDPARAM, "IDirectSoundCapture_GetCaps(NULL) "
       "should have returned DSERR_INVALIDPARAM, returned: %08x\n", rc);

    ZeroMemory(&dsccaps, sizeof(dsccaps));

    /* DSOUND: Error: Invalid caps buffer */
    rc=IDirectSound_GetCaps(dsco, &dsccaps);
    ok(rc==DSERR_INVALIDPARAM, "IDirectSound_GetCaps() "
       "should have returned DSERR_INVALIDPARAM, returned: %08x\n", rc);

    dsccaps.dwSize=sizeof(dsccaps);

    /* DSOUND: Running on a certified driver */
    rc=IDirectSoundCapture_GetCaps(dsco, &dsccaps);
    ok(rc==DS_OK, "IDirectSoundCapture_GetCaps() failed: %08x\n", rc);

EXIT:
    ref=IDirectSoundCapture_Release(dsco);
    ok(ref==0, "IDirectSoundCapture_Release() has %d references, "
       "should have 0\n", ref);
}

static void test_capture(void)
{
    HRESULT rc;
    LPDIRECTSOUNDCAPTURE dsco=NULL;
    LPCLASSFACTORY cf=NULL;

    trace("Testing IDirectSoundCapture\n");

    rc=CoGetClassObject(&CLSID_DirectSoundCapture, CLSCTX_INPROC_SERVER, NULL,
                        &IID_IClassFactory, (void**)&cf);
    ok(rc==S_OK,"CoGetClassObject(CLSID_DirectSoundCapture, IID_IClassFactory) "
       "failed: %08x\n", rc);

    rc=CoGetClassObject(&CLSID_DirectSoundCapture, CLSCTX_INPROC_SERVER, NULL,
                        &IID_IUnknown, (void**)&cf);
    ok(rc==S_OK,"CoGetClassObject(CLSID_DirectSoundCapture, IID_IUnknown) "
       "failed: %08x\n", rc);

    /* try the COM class factory method of creation with no device specified */
    rc=CoCreateInstance(&CLSID_DirectSoundCapture, NULL, CLSCTX_INPROC_SERVER,
                        &IID_IDirectSoundCapture, (void**)&dsco);
    ok(rc==S_OK||rc==REGDB_E_CLASSNOTREG,"CoCreateInstance(CLSID_DirectSoundCapture) failed: %08x\n", rc);
    if (rc==REGDB_E_CLASSNOTREG) {
        trace("  Class Not Registered\n");
        return;
    }
    if (dsco)
        IDirectSoundCapture_test(dsco, FALSE, NULL);

    /* try the COM class factory method of creation with default capture
     * device specified */
    rc=CoCreateInstance(&CLSID_DirectSoundCapture, NULL, CLSCTX_INPROC_SERVER,
                        &IID_IDirectSoundCapture, (void**)&dsco);
    ok(rc==S_OK,"CoCreateInstance(CLSID_DirectSoundCapture) failed: %08x\n", rc);
    if (dsco)
        IDirectSoundCapture_test(dsco, FALSE, &DSDEVID_DefaultCapture);

    /* try the COM class factory method of creation with default voice
     * capture device specified */
    rc=CoCreateInstance(&CLSID_DirectSoundCapture, NULL, CLSCTX_INPROC_SERVER,
                        &IID_IDirectSoundCapture, (void**)&dsco);
    ok(rc==S_OK,"CoCreateInstance(CLSID_DirectSoundCapture) failed: %08x\n", rc);
    if (dsco)
        IDirectSoundCapture_test(dsco, FALSE, &DSDEVID_DefaultVoiceCapture);

    /* try the COM class factory method of creation with a bad
     * IID specified */
    rc=CoCreateInstance(&CLSID_DirectSoundCapture, NULL, CLSCTX_INPROC_SERVER,
                        &CLSID_DirectSoundPrivate, (void**)&dsco);
    ok(rc==E_NOINTERFACE,
       "CoCreateInstance(CLSID_DirectSoundCapture,CLSID_DirectSoundPrivate) "
       "should have failed: %08x\n",rc);

    /* try with no device specified */
    rc=pDirectSoundCaptureCreate(NULL,&dsco,NULL);
    ok(rc==DS_OK||rc==DSERR_NODRIVER||rc==DSERR_ALLOCATED||rc==E_FAIL,
       "DirectSoundCaptureCreate(NULL) failed: %08x\n",rc);
    if (rc==S_OK && dsco)
        IDirectSoundCapture_test(dsco, TRUE, NULL);

    /* try with default capture device specified */
    rc=pDirectSoundCaptureCreate(&DSDEVID_DefaultCapture,&dsco,NULL);
    ok(rc==DS_OK||rc==DSERR_NODRIVER||rc==DSERR_ALLOCATED||rc==E_FAIL,
       "DirectSoundCaptureCreate(DSDEVID_DefaultCapture) failed: %08x\n", rc);
    if (rc==DS_OK && dsco)
        IDirectSoundCapture_test(dsco, TRUE, NULL);

    /* try with default voice capture device specified */
    rc=pDirectSoundCaptureCreate(&DSDEVID_DefaultVoiceCapture,&dsco,NULL);
    ok(rc==DS_OK||rc==DSERR_NODRIVER||rc==DSERR_ALLOCATED||rc==E_FAIL,
       "DirectSoundCaptureCreate(DSDEVID_DefaultVoiceCapture) failed: %08x\n", rc);
    if (rc==DS_OK && dsco)
        IDirectSoundCapture_test(dsco, TRUE, NULL);

    /* try with a bad device specified */
    rc=pDirectSoundCaptureCreate(&DSDEVID_DefaultVoicePlayback,&dsco,NULL);
    ok(rc==DSERR_NODRIVER,
       "DirectSoundCaptureCreate(DSDEVID_DefaultVoicePlatback) "
       "should have failed: %08x\n",rc);
    if (rc==DS_OK && dsco)
        IDirectSoundCapture_Release(dsco);
}

typedef struct {
    char* wave;
    DWORD wave_len;

    LPDIRECTSOUNDCAPTUREBUFFER dscbo;
    LPWAVEFORMATEX wfx;
    DSBPOSITIONNOTIFY posnotify[NOTIFICATIONS];
    HANDLE event[NOTIFICATIONS];
    LPDIRECTSOUNDNOTIFY notify;

    DWORD buffer_size;
    DWORD read;
    DWORD offset;
    DWORD size;

    DWORD last_pos;
} capture_state_t;

static int capture_buffer_service(capture_state_t* state)
{
    HRESULT rc;
    LPVOID ptr1,ptr2;
    DWORD len1,len2;
    DWORD capture_pos,read_pos;

    rc=IDirectSoundCaptureBuffer_GetCurrentPosition(state->dscbo,&capture_pos,
                                                    &read_pos);
    ok(rc==DS_OK,"IDirectSoundCaptureBuffer_GetCurrentPosition() failed: %08x\n", rc);
    if (rc!=DS_OK)
	return 0;

    rc=IDirectSoundCaptureBuffer_Lock(state->dscbo,state->offset,state->size,
                                      &ptr1,&len1,&ptr2,&len2,0);
    ok(rc==DS_OK,"IDirectSoundCaptureBuffer_Lock() failed: %08x\n", rc);
    if (rc!=DS_OK)
	return 0;

    rc=IDirectSoundCaptureBuffer_Unlock(state->dscbo,ptr1,len1,ptr2,len2);
    ok(rc==DS_OK,"IDirectSoundCaptureBuffer_Unlock() failed: %08x\n", rc);
    if (rc!=DS_OK)
	return 0;

    state->offset = (state->offset + state->size) % state->buffer_size;

    return 1;
}

static void test_capture_buffer(LPDIRECTSOUNDCAPTURE dsco,
				LPDIRECTSOUNDCAPTUREBUFFER dscbo, int record)
{
    HRESULT rc;
    DSCBCAPS dscbcaps;
    WAVEFORMATEX wfx;
    DWORD size,status;
    capture_state_t state;
    int i, ref;

    /* Private dsound.dll: Error: Invalid caps pointer */
    rc=IDirectSoundCaptureBuffer_GetCaps(dscbo,0);
    ok(rc==DSERR_INVALIDPARAM,"IDirectSoundCaptureBuffer_GetCaps() should "
       "have returned DSERR_INVALIDPARAM, returned: %08x\n", rc);

    /* Private dsound.dll: Error: Invalid caps pointer */
    dscbcaps.dwSize=0;
    rc=IDirectSoundCaptureBuffer_GetCaps(dscbo,&dscbcaps);
    ok(rc==DSERR_INVALIDPARAM,"IDirectSoundCaptureBuffer_GetCaps() should "
       "have returned DSERR_INVALIDPARAM, returned: %08x\n", rc);

    dscbcaps.dwSize=sizeof(dscbcaps);
    rc=IDirectSoundCaptureBuffer_GetCaps(dscbo,&dscbcaps);
    ok(rc==DS_OK,"IDirectSoundCaptureBuffer_GetCaps() failed: %08x\n", rc);
    if (rc==DS_OK && winetest_debug > 1) {
	trace("    Caps: size = %d flags=0x%08x buffer size=%d\n",
	    dscbcaps.dwSize,dscbcaps.dwFlags,dscbcaps.dwBufferBytes);
    }

    /* Query the format size. Note that it may not match sizeof(wfx) */
    /* Private dsound.dll: Error: Either pwfxFormat or pdwSizeWritten must
     * be non-NULL */
    rc=IDirectSoundCaptureBuffer_GetFormat(dscbo,NULL,0,NULL);
    ok(rc==DSERR_INVALIDPARAM,"IDirectSoundCaptureBuffer_GetFormat() should "
       "have returned DSERR_INVALIDPARAM, returned: %08x\n", rc);

    size=0;
    rc=IDirectSoundCaptureBuffer_GetFormat(dscbo,NULL,0,&size);
    ok(rc==DS_OK && size!=0,"IDirectSoundCaptureBuffer_GetFormat() should "
       "have returned the needed size: rc=%08x, size=%d\n", rc,size);

    rc=IDirectSoundCaptureBuffer_GetFormat(dscbo,&wfx,sizeof(wfx),NULL);
    ok(rc==DS_OK,"IDirectSoundCaptureBuffer_GetFormat() failed: %08x\n", rc);
    if (rc==DS_OK && winetest_debug > 1) {
	trace("    Format: tag=0x%04x %dx%dx%d avg.B/s=%d align=%d\n",
	      wfx.wFormatTag,wfx.nSamplesPerSec,wfx.wBitsPerSample,
	      wfx.nChannels,wfx.nAvgBytesPerSec,wfx.nBlockAlign);
    }

    /* Private dsound.dll: Error: Invalid status pointer */
    rc=IDirectSoundCaptureBuffer_GetStatus(dscbo,0);
    ok(rc==DSERR_INVALIDPARAM,"IDirectSoundCaptureBuffer_GetStatus() should "
       "have returned DSERR_INVALIDPARAM, returned: %08x\n", rc);

    rc=IDirectSoundCaptureBuffer_GetStatus(dscbo,&status);
    ok(rc==DS_OK,"IDirectSoundCaptureBuffer_GetStatus() failed: %08x\n", rc);
    if (rc==DS_OK && winetest_debug > 1) {
	trace("    Status=0x%04x\n",status);
    }

    ZeroMemory(&state, sizeof(state));
    state.dscbo=dscbo;
    state.wfx=&wfx;
    state.buffer_size = dscbcaps.dwBufferBytes;
    for (i = 0; i < NOTIFICATIONS; i++)
	state.event[i] = CreateEvent( NULL, FALSE, FALSE, NULL );
    state.size = dscbcaps.dwBufferBytes / NOTIFICATIONS;

    rc=IDirectSoundCaptureBuffer_QueryInterface(dscbo,&IID_IDirectSoundNotify,
                                                (void **)&(state.notify));
    ok((rc==DS_OK)&&(state.notify!=NULL),
       "IDirectSoundCaptureBuffer_QueryInterface() failed: %08x\n", rc);

    for (i = 0; i < NOTIFICATIONS; i++) {
	state.posnotify[i].dwOffset = (i * state.size) + state.size - 1;
	state.posnotify[i].hEventNotify = state.event[i];
    }

    rc=IDirectSoundNotify_SetNotificationPositions(state.notify,NOTIFICATIONS,
                                                   state.posnotify);
    ok(rc==DS_OK,"IDirectSoundNotify_SetNotificationPositions() failed: %08x\n", rc);

    ref=IDirectSoundNotify_Release(state.notify);
    ok(ref==0,"IDirectSoundNotify_Release(): has %d references, should have "
       "0\n",ref);

    rc=IDirectSoundCaptureBuffer_Start(dscbo,DSCBSTART_LOOPING);
    ok(rc==DS_OK,"IDirectSoundCaptureBuffer_Start() failed: %08x\n", rc);

    rc=IDirectSoundCaptureBuffer_Start(dscbo,0);
    ok(rc==DS_OK,"IDirectSoundCaptureBuffer_Start() failed: %08x\n", rc);

    rc=IDirectSoundCaptureBuffer_GetStatus(dscbo,&status);
    ok(rc==DS_OK,"IDirectSoundCaptureBuffer_GetStatus() failed: %08x\n", rc);
    ok(status==(DSCBSTATUS_CAPTURING|DSCBSTATUS_LOOPING) || broken(status==DSCBSTATUS_CAPTURING),
       "GetStatus: bad status: %x\n",status);

    if (record) {
	/* wait for the notifications */
	for (i = 0; i < (NOTIFICATIONS * 2); i++) {
	    rc=WaitForMultipleObjects(NOTIFICATIONS,state.event,FALSE,3000);
	    ok(rc==(WAIT_OBJECT_0+(i%NOTIFICATIONS)),
               "WaitForMultipleObjects failed: 0x%x\n",rc);
	    if (rc!=(WAIT_OBJECT_0+(i%NOTIFICATIONS))) {
		ok((rc==WAIT_TIMEOUT)||(rc==WAIT_FAILED),
                   "Wrong notification: should be %d, got %d\n",
		    i%NOTIFICATIONS,rc-WAIT_OBJECT_0);
	    }
	    if (!capture_buffer_service(&state))
		break;
	}

    }
    rc=IDirectSoundCaptureBuffer_Stop(dscbo);
    ok(rc==DS_OK,"IDirectSoundCaptureBuffer_Stop() failed: %08x\n", rc);

    rc=IDirectSoundCaptureBuffer_Stop(dscbo);
    ok(rc==DS_OK,"IDirectSoundCaptureBuffer_Stop() failed: %08x\n", rc);
}

static BOOL WINAPI dscenum_callback(LPGUID lpGuid, LPCSTR lpcstrDescription,
				    LPCSTR lpcstrModule, LPVOID lpContext)
{
    HRESULT rc;
    LPDIRECTSOUNDCAPTURE dsco=NULL;
    LPDIRECTSOUNDCAPTUREBUFFER dscbo=NULL;
    DSCBUFFERDESC bufdesc;
    WAVEFORMATEX wfx;
    DSCCAPS dsccaps;
    DWORD f;
    int ref;

    /* Private dsound.dll: Error: Invalid interface buffer */
    trace("*** Testing %s - %s ***\n",lpcstrDescription,lpcstrModule);
    rc=pDirectSoundCaptureCreate(lpGuid,NULL,NULL);
    ok(rc==DSERR_INVALIDPARAM,"DirectSoundCaptureCreate() should have "
       "returned DSERR_INVALIDPARAM, returned: %08x\n",rc);

    rc=pDirectSoundCaptureCreate(lpGuid,&dsco,NULL);
    ok((rc==DS_OK)||(rc==DSERR_NODRIVER)||(rc==E_FAIL)||(rc==DSERR_ALLOCATED),
       "DirectSoundCaptureCreate() failed: %08x\n",rc);
    if (rc!=DS_OK) {
        if (rc==DSERR_NODRIVER)
            trace("  No Driver\n");
        else if (rc==E_FAIL)
            trace("  No Device\n");
        else if (rc==DSERR_ALLOCATED)
            trace("  Already In Use\n");
	goto EXIT;
    }

    /* Private dsound.dll: Error: Invalid caps buffer */
    rc=IDirectSoundCapture_GetCaps(dsco,NULL);
    ok(rc==DSERR_INVALIDPARAM,"IDirectSoundCapture_GetCaps() should have "
       "returned DSERR_INVALIDPARAM, returned: %08x\n",rc);

    /* Private dsound.dll: Error: Invalid caps buffer */
    dsccaps.dwSize=0;
    rc=IDirectSoundCapture_GetCaps(dsco,&dsccaps);
    ok(rc==DSERR_INVALIDPARAM,"IDirectSoundCapture_GetCaps() should have "
       "returned DSERR_INVALIDPARAM, returned: %08x\n",rc);

    dsccaps.dwSize=sizeof(dsccaps);
    rc=IDirectSoundCapture_GetCaps(dsco,&dsccaps);
    ok(rc==DS_OK,"IDirectSoundCapture_GetCaps() failed: %08x\n", rc);
    if (rc==DS_OK && winetest_debug > 1) {
	trace("  Caps: size=%d flags=0x%08x formats=%05x channels=%d\n",
	      dsccaps.dwSize,dsccaps.dwFlags,dsccaps.dwFormats,
              dsccaps.dwChannels);
    }

    /* Private dsound.dll: Error: Invalid size */
    /* Private dsound.dll: Error: Invalid capture buffer description */
    ZeroMemory(&bufdesc, sizeof(bufdesc));
    bufdesc.dwSize=0;
    bufdesc.dwFlags=0;
    bufdesc.dwBufferBytes=0;
    bufdesc.dwReserved=0;
    bufdesc.lpwfxFormat=NULL;
    rc=IDirectSoundCapture_CreateCaptureBuffer(dsco,&bufdesc,&dscbo,NULL);
    ok(rc==DSERR_INVALIDPARAM,"IDirectSoundCapture_CreateCaptureBuffer() "
       "should have returned DSERR_INVALIDPARAM, returned: %08x\n", rc);
    if (rc==DS_OK) {
	ref=IDirectSoundCaptureBuffer_Release(dscbo);
	ok(ref==0,"IDirectSoundCaptureBuffer_Release() has %d references, "
           "should have 0\n",ref);
    }

    /* Private dsound.dll: Error: Invalid buffer size */
    /* Private dsound.dll: Error: Invalid capture buffer description */
    ZeroMemory(&bufdesc, sizeof(bufdesc));
    bufdesc.dwSize=sizeof(bufdesc);
    bufdesc.dwFlags=0;
    bufdesc.dwBufferBytes=0;
    bufdesc.dwReserved=0;
    bufdesc.lpwfxFormat=NULL;
    rc=IDirectSoundCapture_CreateCaptureBuffer(dsco,&bufdesc,&dscbo,NULL);
    ok(rc==DSERR_INVALIDPARAM,"IDirectSoundCapture_CreateCaptureBuffer() "
       "should have returned DSERR_INVALIDPARAM, returned %08x\n", rc);
    if (rc==DS_OK) {
	ref=IDirectSoundCaptureBuffer_Release(dscbo);
	ok(ref==0,"IDirectSoundCaptureBuffer_Release() has %d references, "
           "should have 0\n",ref);
    }

    /* Private dsound.dll: Error: Invalid buffer size */
    /* Private dsound.dll: Error: Invalid capture buffer description */
    ZeroMemory(&bufdesc, sizeof(bufdesc));
    ZeroMemory(&wfx, sizeof(wfx));
    bufdesc.dwSize=sizeof(bufdesc);
    bufdesc.dwFlags=0;
    bufdesc.dwBufferBytes=0;
    bufdesc.dwReserved=0;
    bufdesc.lpwfxFormat=&wfx;
    rc=IDirectSoundCapture_CreateCaptureBuffer(dsco,&bufdesc,&dscbo,NULL);
    ok(rc==DSERR_INVALIDPARAM,"IDirectSoundCapture_CreateCaptureBuffer() "
       "should have returned DSERR_INVALIDPARAM, returned: %08x\n", rc);
    if (rc==DS_OK) {
	ref=IDirectSoundCaptureBuffer_Release(dscbo);
	ok(ref==0,"IDirectSoundCaptureBuffer_Release() has %d references, "
           "should have 0\n",ref);
    }

    /* Private dsound.dll: Error: Invalid buffer size */
    /* Private dsound.dll: Error: Invalid capture buffer description */
    init_format(&wfx,WAVE_FORMAT_PCM,11025,8,1);
    ZeroMemory(&bufdesc, sizeof(bufdesc));
    bufdesc.dwSize=sizeof(bufdesc);
    bufdesc.dwFlags=0;
    bufdesc.dwBufferBytes=0;
    bufdesc.dwReserved=0;
    bufdesc.lpwfxFormat=&wfx;
    rc=IDirectSoundCapture_CreateCaptureBuffer(dsco,&bufdesc,&dscbo,NULL);
    ok(rc==DSERR_INVALIDPARAM,"IDirectSoundCapture_CreateCaptureBuffer() "
       "should have returned DSERR_INVALIDPARAM, returned: %08x\n", rc);
    if (rc==DS_OK) {
	ref=IDirectSoundCaptureBuffer_Release(dscbo);
	ok(ref==0,"IDirectSoundCaptureBuffer_Release() has %d references, "
           "should have 0\n",ref);
    }

    for (f=0;f<NB_FORMATS;f++) {
	dscbo=NULL;
	init_format(&wfx,WAVE_FORMAT_PCM,formats[f][0],formats[f][1],
                    formats[f][2]);
	ZeroMemory(&bufdesc, sizeof(bufdesc));
	bufdesc.dwSize=sizeof(bufdesc);
	bufdesc.dwFlags=0;
	bufdesc.dwBufferBytes=wfx.nAvgBytesPerSec;
	bufdesc.dwReserved=0;
	bufdesc.lpwfxFormat=&wfx;
        if (winetest_interactive)
	    trace("  Testing the capture buffer at %s\n", format_string(&wfx));
	rc=IDirectSoundCapture_CreateCaptureBuffer(dsco,&bufdesc,&dscbo,NULL);
	ok(((rc==DS_OK)&&(dscbo!=NULL))
           || rc==DSERR_BADFORMAT || rc==DSERR_INVALIDCALL || rc==DSERR_NODRIVER
           || rc==DSERR_ALLOCATED || rc==E_INVALIDARG || rc==E_FAIL,
           "IDirectSoundCapture_CreateCaptureBuffer() failed to create a "
           "%s capture buffer: %08x\n",format_string(&wfx),rc);
	if (rc==DS_OK) {
	    test_capture_buffer(dsco, dscbo, winetest_interactive);
	    ref=IDirectSoundCaptureBuffer_Release(dscbo);
	    ok(ref==0,"IDirectSoundCaptureBuffer_Release() has %d references, "
               "should have 0\n",ref);
	} else if (rc==DSERR_BADFORMAT) {
            ok(!(dsccaps.dwFormats & formats[f][3]),
               "IDirectSoundCapture_CreateCaptureBuffer() failed to create a "
               "capture buffer: format listed as supported but using it failed\n");
            if (!(dsccaps.dwFormats & formats[f][3]))
                trace("  Format not supported: %s\n", format_string(&wfx));
        } else if (rc==DSERR_NODRIVER) {
            trace("  No Driver\n");
        } else if (rc==DSERR_ALLOCATED) {
            trace("  Already In Use\n");
        } else if (rc==E_INVALIDARG) { /* try the old version struct */
            DSCBUFFERDESC1 bufdesc1;
	    ZeroMemory(&bufdesc1, sizeof(bufdesc1));
	    bufdesc1.dwSize=sizeof(bufdesc1);
	    bufdesc1.dwFlags=0;
	    bufdesc1.dwBufferBytes=wfx.nAvgBytesPerSec;
	    bufdesc1.dwReserved=0;
	    bufdesc1.lpwfxFormat=&wfx;
	    rc=IDirectSoundCapture_CreateCaptureBuffer(dsco,
                (DSCBUFFERDESC*)&bufdesc1,&dscbo,NULL);
            ok(rc==DS_OK || broken(rc==DSERR_INVALIDPARAM),
               "IDirectSoundCapture_CreateCaptureBuffer() failed to create a "
               "%s capture buffer: %08x\n",format_string(&wfx), rc);
            if (rc==DSERR_INVALIDPARAM) {
                skip("broken driver\n");
                goto EXIT;
            }
            if (rc==DS_OK) {
	        test_capture_buffer(dsco, dscbo, winetest_interactive);
	        ref=IDirectSoundCaptureBuffer_Release(dscbo);
	        ok(ref==0,"IDirectSoundCaptureBuffer_Release() has %d "
                   "references, should have 0\n",ref);
            }
        } else if (rc==E_FAIL) {
            /* WAVE_FORMAT_PCM only allows 8 and 16 bits per sample, so only
             * report a failure if the bits per sample is 8 or 16
             */
            if (wfx.wBitsPerSample == 8 || wfx.wBitsPerSample == 16)
                ok(FALSE,"Should not fail for 8 or 16 bits per sample\n");
        }
    }

    /* try a non PCM format */
    if (0)
    {
    /* FIXME: Why is this commented out? */
    init_format(&wfx,WAVE_FORMAT_MULAW,8000,8,1);
    ZeroMemory(&bufdesc, sizeof(bufdesc));
    bufdesc.dwSize=sizeof(bufdesc);
    bufdesc.dwFlags=DSCBCAPS_WAVEMAPPED;
    bufdesc.dwBufferBytes=wfx.nAvgBytesPerSec;
    bufdesc.dwReserved=0;
    bufdesc.lpwfxFormat=&wfx;
    if (winetest_interactive)
        trace("  Testing the capture buffer at %s\n", format_string(&wfx));
    rc=IDirectSoundCapture_CreateCaptureBuffer(dsco,&bufdesc,&dscbo,NULL);
    ok((rc==DS_OK)&&(dscbo!=NULL),"IDirectSoundCapture_CreateCaptureBuffer() "
       "failed to create a capture buffer: %08x\n",rc);
    if ((rc==DS_OK)&&(dscbo!=NULL)) {
	test_capture_buffer(dsco, dscbo, winetest_interactive);
	ref=IDirectSoundCaptureBuffer_Release(dscbo);
	ok(ref==0,"IDirectSoundCaptureBuffer_Release() has %d references, "
           "should have 0\n",ref);
    }
    }

    /* Try an invalid format to test error handling */
    if (0)
    {
    /* FIXME: Remove this test altogether? */
    init_format(&wfx,WAVE_FORMAT_PCM,2000000,16,2);
    ZeroMemory(&bufdesc, sizeof(bufdesc));
    bufdesc.dwSize=sizeof(bufdesc);
    bufdesc.dwFlags=DSCBCAPS_WAVEMAPPED;
    bufdesc.dwBufferBytes=wfx.nAvgBytesPerSec;
    bufdesc.dwReserved=0;
    bufdesc.lpwfxFormat=&wfx;
    if (winetest_interactive)
        trace("  Testing the capture buffer at %s\n", format_string(&wfx));
    rc=IDirectSoundCapture_CreateCaptureBuffer(dsco,&bufdesc,&dscbo,NULL);
    ok(rc!=DS_OK,"IDirectSoundCapture_CreateCaptureBuffer() should have failed "
       "at 2 MHz %08x\n",rc);
    }

EXIT:
    if (dsco!=NULL) {
	ref=IDirectSoundCapture_Release(dsco);
	ok(ref==0,"IDirectSoundCapture_Release() has %d references, should "
           "have 0\n",ref);
    }

    return TRUE;
}

static void test_enumerate(void)
{
    HRESULT rc;
    rc=pDirectSoundCaptureEnumerateA(&dscenum_callback,NULL);
    ok(rc==DS_OK,"DirectSoundCaptureEnumerateA() failed: %08x\n", rc);
}

static void test_COM(void)
{
    IDirectSoundCapture *dsc = (IDirectSoundCapture*)0xdeadbeef;
    IDirectSoundCaptureBuffer *buffer = (IDirectSoundCaptureBuffer*)0xdeadbeef;
    IDirectSoundNotify *notify;
    IUnknown *unk;
    DSCBUFFERDESC bufdesc;
    WAVEFORMATEX wfx;
    HRESULT hr;
    ULONG refcount;

    hr = pDirectSoundCaptureCreate(NULL, &dsc, (IUnknown*)0xdeadbeef);
    ok(hr == DSERR_NOAGGREGATION,
       "DirectSoundCaptureCreate failed: %08x, expected DSERR_NOAGGREGATION\n", hr);
    ok(dsc == (IDirectSoundCapture*)0xdeadbeef, "dsc = %p\n", dsc);

    hr = pDirectSoundCaptureCreate(NULL, &dsc, NULL);
    if (hr == DSERR_NODRIVER) {
        skip("No driver\n");
        return;
    }
    ok(hr == DS_OK, "DirectSoundCaptureCreate failed: %08x, expected DS_OK\n", hr);

    /* Different refcount for IDirectSoundCapture and for IUnknown */
    refcount = IDirectSoundCapture_AddRef(dsc);
    ok(refcount == 2, "refcount == %u, expected 2\n", refcount);
    hr = IDirectSoundCapture_QueryInterface(dsc, &IID_IUnknown, (void**)&unk);
    ok(hr == S_OK, "QueryInterface for IID_IUnknown failed: %08x\n", hr);
    refcount = IUnknown_AddRef(unk);
    ok(refcount == 2, "refcount == %u, expected 2\n", refcount);
    IUnknown_Release(unk);
    IUnknown_Release(unk);
    IDirectSoundCapture_Release(dsc);

    init_format(&wfx, WAVE_FORMAT_PCM, 44100, 16, 1);
    ZeroMemory(&bufdesc, sizeof(bufdesc));
    bufdesc.dwSize = sizeof(bufdesc);
    bufdesc.dwBufferBytes = wfx.nAvgBytesPerSec;
    bufdesc.lpwfxFormat = &wfx;

    hr = IDirectSoundCapture_CreateCaptureBuffer(dsc, &bufdesc, &buffer, (IUnknown*)0xdeadbeef);
    if (hr == E_INVALIDARG) {
        /* Old DirectX has only the 1st version of the DSCBUFFERDESC struct */
        bufdesc.dwSize = sizeof(DSCBUFFERDESC1);
        hr = IDirectSoundCapture_CreateCaptureBuffer(dsc, &bufdesc, &buffer, (IUnknown*)0xdeadbeef);
    }
    ok(hr == DSERR_NOAGGREGATION,
       "IDirectSoundCapture_CreateCaptureBuffer failed: %08x, expected DSERR_NOAGGREGATION\n", hr);
    ok(buffer == (IDirectSoundCaptureBuffer*)0xdeadbeef || !buffer /* Win2k without DirectX9 */,
       "buffer = %p\n", buffer);

    hr = IDirectSoundCapture_CreateCaptureBuffer(dsc, &bufdesc, &buffer, NULL);
    ok(hr == DS_OK, "IDirectSoundCapture_CreateCaptureBuffer failed: %08x, expected DS_OK\n", hr);

    /* IDirectSoundCaptureBuffer and IDirectSoundNotify have separate refcounts */
    IDirectSoundCaptureBuffer_AddRef(buffer);
    refcount = IDirectSoundCaptureBuffer_AddRef(buffer);
    ok(refcount == 3, "IDirectSoundCaptureBuffer refcount is %u, expected 3\n", refcount);
    hr = IDirectSoundCaptureBuffer_QueryInterface(buffer, &IID_IDirectSoundNotify, (void**)&notify);
    ok(hr == DS_OK, "IDirectSoundCapture_QueryInterface failed: %08x, expected DS_OK\n", hr);
    refcount = IDirectSoundNotify_AddRef(notify);
    ok(refcount == 2, "IDirectSoundNotify refcount is %u, expected 2\n", refcount);
    IDirectSoundCaptureBuffer_AddRef(buffer);
    refcount = IDirectSoundCaptureBuffer_Release(buffer);
    ok(refcount == 3, "IDirectSoundCaptureBuffer refcount is %u, expected 3\n", refcount);

    /* Release IDirectSoundCaptureBuffer while keeping IDirectSoundNotify alive */
    while (IDirectSoundCaptureBuffer_Release(buffer) > 0);
    refcount = IDirectSoundNotify_AddRef(notify);
    ok(refcount == 3, "IDirectSoundNotify refcount is %u, expected 3\n", refcount);
    refcount = IDirectSoundCaptureBuffer_AddRef(buffer);
    ok(refcount == 1, "IDirectSoundCaptureBuffer refcount is %u, expected 1\n", refcount);

    while (IDirectSoundNotify_Release(notify) > 0);
    refcount = IDirectSoundCaptureBuffer_Release(buffer);
    ok(refcount == 0, "IDirectSoundCaptureBuffer refcount is %u, expected 0\n", refcount);
    refcount = IDirectSoundCapture_Release(dsc);
    ok(refcount == 0, "IDirectSoundCapture refcount is %u, expected 0\n", refcount);
}

START_TEST(capture)
{
    HMODULE hDsound;

    CoInitialize(NULL);

    hDsound = LoadLibrary("dsound.dll");
    if (!hDsound) {
        skip("dsound.dll not found - skipping all tests\n");
        return;
    }

    pDirectSoundCaptureCreate = (void*)GetProcAddress(hDsound, "DirectSoundCaptureCreate");
    pDirectSoundCaptureEnumerateA = (void*)GetProcAddress(hDsound, "DirectSoundCaptureEnumerateA");
    if (!pDirectSoundCaptureCreate || !pDirectSoundCaptureEnumerateA) {
        skip("DirectSoundCapture{Create,Enumerate} missing - skipping all tests\n");
        return;
    }

    test_COM();
    test_capture();
    test_enumerate();

    FreeLibrary(hDsound);
    CoUninitialize();
}
