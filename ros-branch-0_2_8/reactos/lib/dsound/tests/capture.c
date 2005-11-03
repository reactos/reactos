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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#define NONAMELESSSTRUCT
#define NONAMELESSUNION
#include <windows.h>

#include <stdio.h>

#include "wine/test.h"
#include "dsound.h"
#include "mmreg.h"
#include "dxerr8.h"
#include "dsconf.h"

#include "dsound_test.h"

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

static char * format_string(WAVEFORMATEX* wfx)
{
    static char str[64];

    sprintf(str, "%5ldx%2dx%d %s",
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
       "failed: %s\n", DXGetErrorString8(rc));
    if (rc==DS_OK)
        IDirectSoundCapture_Release(unknown);

    rc=IDirectSoundCapture_QueryInterface(dsco, &IID_IDirectSoundCapture,
                                          (LPVOID*)&dsc);
    ok(rc==DS_OK, "IDirectSoundCapture_QueryInterface(IID_IDirectSoundCapture) "
       "failed: %s\n", DXGetErrorString8(rc));
    if (rc==DS_OK)
        IDirectSoundCapture_Release(dsc);

    if (initialized == FALSE) {
        /* try unitialized object */
        rc=IDirectSoundCapture_GetCaps(dsco,0);
        ok(rc==DSERR_UNINITIALIZED, "IDirectSoundCapture_GetCaps(NULL) "
           "should have returned DSERR_UNINITIALIZED, returned: %s\n",
           DXGetErrorString8(rc));

        rc=IDirectSoundCapture_GetCaps(dsco, &dsccaps);
        ok(rc==DSERR_UNINITIALIZED,"IDirectSoundCapture_GetCaps() "
           "should have returned DSERR_UNINITIALIZED, returned: %s\n",
           DXGetErrorString8(rc));

        rc=IDirectSoundCapture_Initialize(dsco, lpGuid);
        ok(rc==DS_OK||rc==DSERR_NODRIVER||rc==DSERR_ALLOCATED||rc==E_FAIL,
           "IDirectSoundCapture_Initialize() failed: %s\n",
           DXGetErrorString8(rc));
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

    rc=IDirectSoundCapture_Initialize(dsco, lpGuid);
    ok(rc==DSERR_ALREADYINITIALIZED, "IDirectSoundCapture_Initialize() "
       "should have returned DSERR_ALREADYINITIALIZED: %s\n",
       DXGetErrorString8(rc));

    /* DSOUND: Error: Invalid caps buffer */
    rc=IDirectSoundCapture_GetCaps(dsco, 0);
    ok(rc==DSERR_INVALIDPARAM, "IDirectSoundCapture_GetCaps(NULL) "
       "should have returned DSERR_INVALIDPARAM, returned: %s\n",
       DXGetErrorString8(rc));

    ZeroMemory(&dsccaps, sizeof(dsccaps));

    /* DSOUND: Error: Invalid caps buffer */
    rc=IDirectSound_GetCaps(dsco, &dsccaps);
    ok(rc==DSERR_INVALIDPARAM, "IDirectSound_GetCaps() "
       "should have returned DSERR_INVALIDPARAM, returned: %s\n",
       DXGetErrorString8(rc));

    dsccaps.dwSize=sizeof(dsccaps);

    /* DSOUND: Running on a certified driver */
    rc=IDirectSoundCapture_GetCaps(dsco, &dsccaps);
    ok(rc==DS_OK, "IDirectSoundCapture_GetCaps() failed: %s\n",
       DXGetErrorString8(rc));

EXIT:
    ref=IDirectSoundCapture_Release(dsco);
    ok(ref==0, "IDirectSoundCapture_Release() has %d references, "
       "should have 0\n", ref);
}

static void IDirectSoundCapture_tests(void)
{
    HRESULT rc;
    LPDIRECTSOUNDCAPTURE dsco=NULL;

    trace("Testing IDirectSoundCapture\n");

    /* try the COM class factory method of creation with no device specified */
    rc=CoCreateInstance(&CLSID_DirectSoundCapture, NULL, CLSCTX_INPROC_SERVER,
                        &IID_IDirectSoundCapture, (void**)&dsco);
    ok(rc==S_OK||rc==REGDB_E_CLASSNOTREG,"CoCreateInstance(CLSID_DirectSoundCapture) failed: %s\n",
       DXGetErrorString8(rc));
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
    ok(rc==S_OK,"CoCreateInstance(CLSID_DirectSoundCapture) failed: %s\n",
       DXGetErrorString8(rc));
    if (dsco)
        IDirectSoundCapture_test(dsco, FALSE, &DSDEVID_DefaultCapture);

    /* try the COM class factory method of creation with default voice
     * capture device specified */
    rc=CoCreateInstance(&CLSID_DirectSoundCapture, NULL, CLSCTX_INPROC_SERVER,
                        &IID_IDirectSoundCapture, (void**)&dsco);
    ok(rc==S_OK,"CoCreateInstance(CLSID_DirectSoundCapture) failed: %s\n",
       DXGetErrorString8(rc));
    if (dsco)
        IDirectSoundCapture_test(dsco, FALSE, &DSDEVID_DefaultVoiceCapture);

    /* try the COM class factory method of creation with a bad
     * IID specified */
    rc=CoCreateInstance(&CLSID_DirectSoundCapture, NULL, CLSCTX_INPROC_SERVER,
                        &CLSID_DirectSoundPrivate, (void**)&dsco);
    ok(rc==E_NOINTERFACE,
       "CoCreateInstance(CLSID_DirectSoundCapture,CLSID_DirectSoundPrivate) "
       "should have failed: %s\n",DXGetErrorString8(rc));

    /* try with no device specified */
    rc=pDirectSoundCaptureCreate(NULL,&dsco,NULL);
    ok(rc==DS_OK||rc==DSERR_NODRIVER||rc==DSERR_ALLOCATED||rc==E_FAIL,
       "DirectSoundCaptureCreate(NULL) failed: %s\n",DXGetErrorString8(rc));
    if (rc==S_OK && dsco)
        IDirectSoundCapture_test(dsco, TRUE, NULL);

    /* try with default capture device specified */
    rc=pDirectSoundCaptureCreate(&DSDEVID_DefaultCapture,&dsco,NULL);
    ok(rc==DS_OK||rc==DSERR_NODRIVER||rc==DSERR_ALLOCATED||rc==E_FAIL,
       "DirectSoundCaptureCreate(DSDEVID_DefaultCapture) failed: %s\n",
       DXGetErrorString8(rc));
    if (rc==DS_OK && dsco)
        IDirectSoundCapture_test(dsco, TRUE, NULL);

    /* try with default voice capture device specified */
    rc=pDirectSoundCaptureCreate(&DSDEVID_DefaultVoiceCapture,&dsco,NULL);
    ok(rc==DS_OK||rc==DSERR_NODRIVER||rc==DSERR_ALLOCATED||rc==E_FAIL,
       "DirectSoundCaptureCreate(DSDEVID_DefaultVoiceCapture) failed: %s\n",
       DXGetErrorString8(rc));
    if (rc==DS_OK && dsco)
        IDirectSoundCapture_test(dsco, TRUE, NULL);

    /* try with a bad device specified */
    rc=pDirectSoundCaptureCreate(&DSDEVID_DefaultVoicePlayback,&dsco,NULL);
    ok(rc==DSERR_NODRIVER,
       "DirectSoundCaptureCreate(DSDEVID_DefaultVoicePlatback) "
       "should have failed: %s\n",DXGetErrorString8(rc));
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
    ok(rc==DS_OK,"IDirectSoundCaptureBuffer_GetCurrentPosition() failed: %s\n",
       DXGetErrorString8(rc));
    if (rc!=DS_OK)
	return 0;

    rc=IDirectSoundCaptureBuffer_Lock(state->dscbo,state->offset,state->size,
                                      &ptr1,&len1,&ptr2,&len2,0);
    ok(rc==DS_OK,"IDirectSoundCaptureBuffer_Lock() failed: %s\n",
       DXGetErrorString8(rc));
    if (rc!=DS_OK)
	return 0;

    rc=IDirectSoundCaptureBuffer_Unlock(state->dscbo,ptr1,len1,ptr2,len2);
    ok(rc==DS_OK,"IDirectSoundCaptureBuffer_Unlock() failed: %s\n",
       DXGetErrorString8(rc));
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
       "have returned DSERR_INVALIDPARAM, returned: %s\n",
       DXGetErrorString8(rc));

    /* Private dsound.dll: Error: Invalid caps pointer */
    dscbcaps.dwSize=0;
    rc=IDirectSoundCaptureBuffer_GetCaps(dscbo,&dscbcaps);
    ok(rc==DSERR_INVALIDPARAM,"IDirectSoundCaptureBuffer_GetCaps() should "
       "have returned DSERR_INVALIDPARAM, returned: %s\n",
       DXGetErrorString8(rc));

    dscbcaps.dwSize=sizeof(dscbcaps);
    rc=IDirectSoundCaptureBuffer_GetCaps(dscbo,&dscbcaps);
    ok(rc==DS_OK,"IDirectSoundCaptureBuffer_GetCaps() failed: %s\n",
       DXGetErrorString8(rc));
    if (rc==DS_OK && winetest_debug > 1) {
	trace("    Caps: size = %ld flags=0x%08lx buffer size=%ld\n",
	    dscbcaps.dwSize,dscbcaps.dwFlags,dscbcaps.dwBufferBytes);
    }

    /* Query the format size. Note that it may not match sizeof(wfx) */
    /* Private dsound.dll: Error: Either pwfxFormat or pdwSizeWritten must
     * be non-NULL */
    rc=IDirectSoundCaptureBuffer_GetFormat(dscbo,NULL,0,NULL);
    ok(rc==DSERR_INVALIDPARAM,"IDirectSoundCaptureBuffer_GetFormat() should "
       "have returned DSERR_INVALIDPARAM, returned: %s\n",
       DXGetErrorString8(rc));

    size=0;
    rc=IDirectSoundCaptureBuffer_GetFormat(dscbo,NULL,0,&size);
    ok(rc==DS_OK && size!=0,"IDirectSoundCaptureBuffer_GetFormat() should "
       "have returned the needed size: rc=%s, size=%ld\n",
       DXGetErrorString8(rc),size);

    rc=IDirectSoundCaptureBuffer_GetFormat(dscbo,&wfx,sizeof(wfx),NULL);
    ok(rc==DS_OK,"IDirectSoundCaptureBuffer_GetFormat() failed: %s\n",
       DXGetErrorString8(rc));
    if (rc==DS_OK && winetest_debug > 1) {
	trace("    Format: tag=0x%04x %ldx%dx%d avg.B/s=%ld align=%d\n",
	      wfx.wFormatTag,wfx.nSamplesPerSec,wfx.wBitsPerSample,
	      wfx.nChannels,wfx.nAvgBytesPerSec,wfx.nBlockAlign);
    }

    /* Private dsound.dll: Error: Invalid status pointer */
    rc=IDirectSoundCaptureBuffer_GetStatus(dscbo,0);
    ok(rc==DSERR_INVALIDPARAM,"IDirectSoundCaptureBuffer_GetStatus() should "
       "have returned DSERR_INVALIDPARAM, returned: %s\n",
       DXGetErrorString8(rc));

    rc=IDirectSoundCaptureBuffer_GetStatus(dscbo,&status);
    ok(rc==DS_OK,"IDirectSoundCaptureBuffer_GetStatus() failed: %s\n",
       DXGetErrorString8(rc));
    if (rc==DS_OK && winetest_debug > 1) {
	trace("    Status=0x%04lx\n",status);
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
       "IDirectSoundCaptureBuffer_QueryInterface() failed: %s\n",
       DXGetErrorString8(rc));
    if (rc!=DS_OK)
	return;

    for (i = 0; i < NOTIFICATIONS; i++) {
	state.posnotify[i].dwOffset = (i * state.size) + state.size - 1;
	state.posnotify[i].hEventNotify = state.event[i];
    }

    rc=IDirectSoundNotify_SetNotificationPositions(state.notify,NOTIFICATIONS,
                                                   state.posnotify);
    ok(rc==DS_OK,"IDirectSoundNotify_SetNotificationPositions() failed: %s\n",
       DXGetErrorString8(rc));
    if (rc!=DS_OK)
	return;

    ref=IDirectSoundNotify_Release(state.notify);
    ok(ref==0,"IDirectSoundNotify_Release(): has %d references, should have "
       "0\n",ref);
    if (ref!=0)
	return;

    if (record) {
	rc=IDirectSoundCaptureBuffer_Start(dscbo,DSCBSTART_LOOPING);
	ok(rc==DS_OK,"IDirectSoundCaptureBuffer_Start() failed: %s\n",
           DXGetErrorString8(rc));
	if (rc!=DS_OK)
	    return;

	rc=IDirectSoundCaptureBuffer_GetStatus(dscbo,&status);
	ok(rc==DS_OK,"IDirectSoundCaptureBuffer_GetStatus() failed: %s\n",
           DXGetErrorString8(rc));
	ok(status==(DSCBSTATUS_CAPTURING|DSCBSTATUS_LOOPING),
	   "GetStatus: bad status: %lx\n",status);
	if (rc!=DS_OK)
	    return;

	/* wait for the notifications */
	for (i = 0; i < (NOTIFICATIONS * 2); i++) {
	    rc=WaitForMultipleObjects(NOTIFICATIONS,state.event,FALSE,3000);
	    ok(rc==(WAIT_OBJECT_0+(i%NOTIFICATIONS)),
               "WaitForMultipleObjects failed: 0x%lx\n",rc);
	    if (rc!=(WAIT_OBJECT_0+(i%NOTIFICATIONS))) {
		ok((rc==WAIT_TIMEOUT)||(rc==WAIT_FAILED),
                   "Wrong notification: should be %d, got %ld\n",
		    i%NOTIFICATIONS,rc-WAIT_OBJECT_0);
	    }
	    if (!capture_buffer_service(&state))
		break;
	}

	rc=IDirectSoundCaptureBuffer_Stop(dscbo);
	ok(rc==DS_OK,"IDirectSoundCaptureBuffer_Stop() failed: %s\n",
           DXGetErrorString8(rc));
	if (rc!=DS_OK)
	    return;
    }
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
       "returned DSERR_INVALIDPARAM, returned: %s\n",DXGetErrorString8(rc));
    if (rc==DS_OK) {
	ref=IDirectSoundCapture_Release(dsco);
	ok(ref==0,"IDirectSoundCapture_Release() has %d references, should "
           "have 0\n",ref);
    }

    rc=pDirectSoundCaptureCreate(lpGuid,&dsco,NULL);
    ok((rc==DS_OK)||(rc==DSERR_NODRIVER)||(rc==E_FAIL)||(rc==DSERR_ALLOCATED),
       "DirectSoundCaptureCreate() failed: %s\n",DXGetErrorString8(rc));
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
       "returned DSERR_INVALIDPARAM, returned: %s\n",DXGetErrorString8(rc));

    /* Private dsound.dll: Error: Invalid caps buffer */
    dsccaps.dwSize=0;
    rc=IDirectSoundCapture_GetCaps(dsco,&dsccaps);
    ok(rc==DSERR_INVALIDPARAM,"IDirectSoundCapture_GetCaps() should have "
       "returned DSERR_INVALIDPARAM, returned: %s\n",DXGetErrorString8(rc));

    dsccaps.dwSize=sizeof(dsccaps);
    rc=IDirectSoundCapture_GetCaps(dsco,&dsccaps);
    ok(rc==DS_OK,"IDirectSoundCapture_GetCaps() failed: %s\n",
       DXGetErrorString8(rc));
    if (rc==DS_OK && winetest_debug > 1) {
	trace("  Caps: size=%ld flags=0x%08lx formats=%05lx channels=%ld\n",
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
       "should have returned DSERR_INVALIDPARAM, returned: %s\n",
       DXGetErrorString8(rc));
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
       "should have returned DSERR_INVALIDPARAM, returned %s\n",
       DXGetErrorString8(rc));
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
       "should have returned DSERR_INVALIDPARAM, returned :%s\n",
       DXGetErrorString8(rc));
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
       "should have returned DSERR_INVALIDPARAM, returned: %s\n",
       DXGetErrorString8(rc));
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
	ok(((rc==DS_OK)&&(dscbo!=NULL))||(rc==DSERR_BADFORMAT)||
           ((rc==DSERR_NODRIVER))||(rc==DSERR_ALLOCATED)||(rc==E_INVALIDARG),
           "IDirectSoundCapture_CreateCaptureBuffer() failed to create a "
           "%s capture buffer: %s\n",format_string(&wfx),DXGetErrorString8(rc));
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
	    ok(rc==DS_OK,
               "IDirectSoundCapture_CreateCaptureBuffer() failed to create a "
               "%s capture buffer: %s\n",format_string(&wfx),
               DXGetErrorString8(rc));
            if (rc==DS_OK) {
	        test_capture_buffer(dsco, dscbo, winetest_interactive);
	        ref=IDirectSoundCaptureBuffer_Release(dscbo);
	        ok(ref==0,"IDirectSoundCaptureBuffer_Release() has %d "
                   "references, should have 0\n",ref);
            }
        }
    }

    /* try a non PCM format */
#if 0
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
       "failed to create a capture buffer: %s\n",DXGetErrorString8(rc));
    if ((rc==DS_OK)&&(dscbo!=NULL)) {
	test_capture_buffer(dsco, dscbo, winetest_interactive);
	ref=IDirectSoundCaptureBuffer_Release(dscbo);
	ok(ref==0,"IDirectSoundCaptureBuffer_Release() has %d references, "
           "should have 0\n",ref);
    }
#endif

    /* Try an invalid format to test error handling */
#if 0
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
       "at 2 MHz %s\n",DXGetErrorString8(rc));
#endif

EXIT:
    if (dsco!=NULL) {
	ref=IDirectSoundCapture_Release(dsco);
	ok(ref==0,"IDirectSoundCapture_Release() has %d references, should "
           "have 0\n",ref);
    }

    return TRUE;
}

static void capture_tests(void)
{
    HRESULT rc;
    rc=pDirectSoundCaptureEnumerateA(&dscenum_callback,NULL);
    ok(rc==DS_OK,"DirectSoundCaptureEnumerateA() failed: %s\n",
       DXGetErrorString8(rc));
}

START_TEST(capture)
{
    HMODULE hDsound;

    CoInitialize(NULL);

    hDsound = LoadLibraryA("dsound.dll");
    if (!hDsound) {
        trace("dsound.dll not found\n");
        return;
    }

    trace("DLL Version: %s\n", get_file_version("dsound.dll"));

    pDirectSoundCaptureCreate=(void*)GetProcAddress(hDsound,"DirectSoundCaptureCreate");
    pDirectSoundCaptureEnumerateA=(void*)GetProcAddress(hDsound,"DirectSoundCaptureEnumerateA");
    if (!pDirectSoundCaptureCreate || !pDirectSoundCaptureEnumerateA)
    {
        trace("capture test skipped\n");
        return;
    }

    IDirectSoundCapture_tests();
    capture_tests();

    CoUninitialize();
}
