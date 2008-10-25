/*              DirectSoundCapture
 *
 * Copyright 1998 Marcus Meissner
 * Copyright 1998 Rob Riggs
 * Copyright 2000-2001 TransGaming Technologies, Inc.
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
/*
 * TODO:
 *	Implement FX support.
 *	Implement both IDirectSoundCaptureBuffer and IDirectSoundCaptureBuffer8
 *	Make DirectSoundCaptureCreate and DirectSoundCaptureCreate8 behave differently
 */

#include <stdarg.h>

#define NONAMELESSSTRUCT
#define NONAMELESSUNION
#include "windef.h"
#include "winbase.h"
#include "mmsystem.h"
#include "mmddk.h"
#include "winreg.h"
#include "winternl.h"
#include "winnls.h"
#include "wine/debug.h"
#include "dsound.h"
#include "dsdriver.h"
#include "dsound_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(dsound);

static HRESULT WINAPI IDirectSoundCaptureImpl_Initialize(
    LPDIRECTSOUNDCAPTURE iface,
    LPCGUID lpcGUID );
static ULONG WINAPI IDirectSoundCaptureImpl_Release(
    LPDIRECTSOUNDCAPTURE iface );
static ULONG WINAPI IDirectSoundCaptureBufferImpl_Release(
    LPDIRECTSOUNDCAPTUREBUFFER8 iface );
static HRESULT DSOUND_CreateDirectSoundCaptureBuffer(
    IDirectSoundCaptureImpl *ipDSC,
    LPCDSCBUFFERDESC lpcDSCBufferDesc,
    LPVOID* ppobj );

static const IDirectSoundCaptureVtbl dscvt;
static const IDirectSoundCaptureBuffer8Vtbl dscbvt;

DirectSoundCaptureDevice * DSOUND_capture[MAXWAVEDRIVERS];

static const char * captureStateString[] = {
    "STATE_STOPPED",
    "STATE_STARTING",
    "STATE_CAPTURING",
    "STATE_STOPPING"
};

HRESULT WINAPI IDirectSoundCaptureImpl_Create(
    LPDIRECTSOUNDCAPTURE8 * ppDSC)
{
    IDirectSoundCaptureImpl *pDSC;
    TRACE("(%p)\n", ppDSC);

    /* Allocate memory */
    pDSC = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(IDirectSoundCaptureImpl));
    if (pDSC == NULL) {
        WARN("out of memory\n");
        *ppDSC = NULL;
        return DSERR_OUTOFMEMORY;
    }

    pDSC->lpVtbl = &dscvt;
    pDSC->ref    = 0;
    pDSC->device = NULL;

    *ppDSC = (LPDIRECTSOUNDCAPTURE8)pDSC;

    return DS_OK;
}

HRESULT WINAPI DSOUND_CaptureCreate(
    LPDIRECTSOUNDCAPTURE *ppDSC,
    IUnknown *pUnkOuter)
{
    LPDIRECTSOUNDCAPTURE pDSC;
    HRESULT hr;
    TRACE("(%p,%p)\n",ppDSC,pUnkOuter);

    /* Get dsound configuration */
    setup_dsound_options();

    hr = IDirectSoundCaptureImpl_Create(&pDSC);
    if (hr == DS_OK) {
        IDirectSoundCapture_AddRef(pDSC);
        *ppDSC = pDSC;
    } else {
        WARN("IDirectSoundCaptureImpl_Create failed\n");
        *ppDSC = 0;
    }

    return hr;
}

HRESULT WINAPI DSOUND_CaptureCreate8(
    LPDIRECTSOUNDCAPTURE8 *ppDSC8,
    IUnknown *pUnkOuter)
{
    LPDIRECTSOUNDCAPTURE8 pDSC8;
    HRESULT hr;
    TRACE("(%p,%p)\n",ppDSC8,pUnkOuter);

    /* Get dsound configuration */
    setup_dsound_options();

    hr = IDirectSoundCaptureImpl_Create(&pDSC8);
    if (hr == DS_OK) {
        IDirectSoundCapture_AddRef(pDSC8);
        *ppDSC8 = pDSC8;
    } else {
        WARN("IDirectSoundCaptureImpl_Create failed\n");
        *ppDSC8 = 0;
    }

    return hr;
}

/***************************************************************************
 * DirectSoundCaptureCreate [DSOUND.6]
 *
 * Create and initialize a DirectSoundCapture interface.
 *
 * PARAMS
 *    lpcGUID   [I] Address of the GUID that identifies the sound capture device.
 *    lplpDSC   [O] Address of a variable to receive the interface pointer.
 *    pUnkOuter [I] Must be NULL.
 *
 * RETURNS
 *    Success: DS_OK
 *    Failure: DSERR_NOAGGREGATION, DSERR_ALLOCATED, DSERR_INVALIDPARAM,
 *             DSERR_OUTOFMEMORY
 *
 * NOTES
 *    lpcGUID must be one of the values returned from DirectSoundCaptureEnumerate
 *    or NULL for the default device or DSDEVID_DefaultCapture or
 *    DSDEVID_DefaultVoiceCapture.
 *
 *    DSERR_ALLOCATED is returned for sound devices that do not support full duplex.
 */
HRESULT WINAPI DirectSoundCaptureCreate(
    LPCGUID lpcGUID,
    LPDIRECTSOUNDCAPTURE *ppDSC,
    LPUNKNOWN pUnkOuter)
{
    HRESULT hr;
    LPDIRECTSOUNDCAPTURE pDSC;
    TRACE("(%s,%p,%p)\n", debugstr_guid(lpcGUID), ppDSC, pUnkOuter);

    if (ppDSC == NULL) {
	WARN("invalid parameter: ppDSC == NULL\n");
        return DSERR_INVALIDPARAM;
    }

    if (pUnkOuter) {
	WARN("invalid parameter: pUnkOuter != NULL\n");
        *ppDSC = NULL;
        return DSERR_NOAGGREGATION;
    }

    hr = DSOUND_CaptureCreate(&pDSC, (IUnknown *)pUnkOuter);
    if (hr == DS_OK) {
        hr = IDirectSoundCapture_Initialize(pDSC, lpcGUID);
        if (hr != DS_OK) {
            IDirectSoundCapture_Release(pDSC);
            pDSC = 0;
        }
    }

    *ppDSC = pDSC;

    return hr;
}

/***************************************************************************
 * DirectSoundCaptureCreate8 [DSOUND.12]
 *
 * Create and initialize a DirectSoundCapture interface.
 *
 * PARAMS
 *    lpcGUID   [I] Address of the GUID that identifies the sound capture device.
 *    lplpDSC   [O] Address of a variable to receive the interface pointer.
 *    pUnkOuter [I] Must be NULL.
 *
 * RETURNS
 *    Success: DS_OK
 *    Failure: DSERR_NOAGGREGATION, DSERR_ALLOCATED, DSERR_INVALIDPARAM,
 *             DSERR_OUTOFMEMORY
 *
 * NOTES
 *    lpcGUID must be one of the values returned from DirectSoundCaptureEnumerate
 *    or NULL for the default device or DSDEVID_DefaultCapture or
 *    DSDEVID_DefaultVoiceCapture.
 *
 *    DSERR_ALLOCATED is returned for sound devices that do not support full duplex.
 */
HRESULT WINAPI DirectSoundCaptureCreate8(
    LPCGUID lpcGUID,
    LPDIRECTSOUNDCAPTURE8 *ppDSC8,
    LPUNKNOWN pUnkOuter)
{
    HRESULT hr;
    LPDIRECTSOUNDCAPTURE8 pDSC8;
    TRACE("(%s,%p,%p)\n", debugstr_guid(lpcGUID), ppDSC8, pUnkOuter);

    if (ppDSC8 == NULL) {
	WARN("invalid parameter: ppDSC8 == NULL\n");
        return DSERR_INVALIDPARAM;
    }

    if (pUnkOuter) {
	WARN("invalid parameter: pUnkOuter != NULL\n");
        *ppDSC8 = NULL;
        return DSERR_NOAGGREGATION;
    }

    hr = DSOUND_CaptureCreate8(&pDSC8, (IUnknown *)pUnkOuter);
    if (hr == DS_OK) {
        hr = IDirectSoundCapture_Initialize(pDSC8, lpcGUID);
        if (hr != DS_OK) {
            IDirectSoundCapture_Release(pDSC8);
            pDSC8 = 0;
        }
    }

    *ppDSC8 = pDSC8;

    return hr;
}

static HRESULT DirectSoundCaptureDevice_Create(
    DirectSoundCaptureDevice ** ppDevice)
{
    DirectSoundCaptureDevice * device;
    TRACE("(%p)\n", ppDevice);

    /* Allocate memory */
    device = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(DirectSoundCaptureDevice));

    if (device == NULL) {
	WARN("out of memory\n");
        return DSERR_OUTOFMEMORY;
    }

    device->ref = 1;
    device->state = STATE_STOPPED;

    InitializeCriticalSection( &(device->lock) );
    device->lock.DebugInfo->Spare[0] = (DWORD_PTR)"DSCAPTURE_lock";

    *ppDevice = device;

    return DS_OK;
}

/***************************************************************************
 * DirectSoundCaptureEnumerateA [DSOUND.7]
 *
 * Enumerate all DirectSound drivers installed in the system.
 *
 * PARAMS
 *    lpDSEnumCallback  [I] Address of callback function.
 *    lpContext         [I] Address of user defined context passed to callback function.
 *
 * RETURNS
 *    Success: DS_OK
 *    Failure: DSERR_INVALIDPARAM
 */
HRESULT WINAPI
DirectSoundCaptureEnumerateA(
    LPDSENUMCALLBACKA lpDSEnumCallback,
    LPVOID lpContext)
{
    unsigned devs, wid;
    DSDRIVERDESC desc;
    GUID guid;
    int err;

    TRACE("(%p,%p)\n", lpDSEnumCallback, lpContext );

    if (lpDSEnumCallback == NULL) {
	WARN("invalid parameter: lpDSEnumCallback == NULL\n");
        return DSERR_INVALIDPARAM;
    }

    devs = waveInGetNumDevs();
    if (devs > 0) {
	if (GetDeviceID(&DSDEVID_DefaultCapture, &guid) == DS_OK) {
    	    for (wid = 0; wid < devs; ++wid) {
                if (IsEqualGUID( &guid, &DSOUND_capture_guids[wid] ) ) {
                    err = mmErr(WineWaveInMessage((HWAVEIN)wid,DRV_QUERYDSOUNDDESC,(DWORD_PTR)&desc,0));
                    if (err == DS_OK) {
                        TRACE("calling lpDSEnumCallback(NULL,\"%s\",\"%s\",%p)\n",
                              "Primary Sound Capture Driver",desc.szDrvname,lpContext);
                        if (lpDSEnumCallback(NULL, "Primary Sound Capture Driver", desc.szDrvname, lpContext) == FALSE)
                            return DS_OK;
                    }
                }
	    }
	}
    }

    for (wid = 0; wid < devs; ++wid) {
	err = mmErr(WineWaveInMessage((HWAVEIN)wid,DRV_QUERYDSOUNDDESC,(DWORD_PTR)&desc,0));
	if (err == DS_OK) {
            TRACE("calling lpDSEnumCallback(%s,\"%s\",\"%s\",%p)\n",
                  debugstr_guid(&DSOUND_capture_guids[wid]),desc.szDesc,desc.szDrvname,lpContext);
            if (lpDSEnumCallback(&DSOUND_capture_guids[wid], desc.szDesc, desc.szDrvname, lpContext) == FALSE)
                return DS_OK;
	}
    }

    return DS_OK;
}

/***************************************************************************
 * DirectSoundCaptureEnumerateW [DSOUND.8]
 *
 * Enumerate all DirectSound drivers installed in the system.
 *
 * PARAMS
 *    lpDSEnumCallback  [I] Address of callback function.
 *    lpContext         [I] Address of user defined context passed to callback function.
 *
 * RETURNS
 *    Success: DS_OK
 *    Failure: DSERR_INVALIDPARAM
 */
HRESULT WINAPI
DirectSoundCaptureEnumerateW(
    LPDSENUMCALLBACKW lpDSEnumCallback,
    LPVOID lpContext)
{
    unsigned devs, wid;
    DSDRIVERDESC desc;
    GUID guid;
    int err;
    WCHAR wDesc[MAXPNAMELEN];
    WCHAR wName[MAXPNAMELEN];

    TRACE("(%p,%p)\n", lpDSEnumCallback, lpContext );

    if (lpDSEnumCallback == NULL) {
	WARN("invalid parameter: lpDSEnumCallback == NULL\n");
        return DSERR_INVALIDPARAM;
    }

    devs = waveInGetNumDevs();
    if (devs > 0) {
	if (GetDeviceID(&DSDEVID_DefaultCapture, &guid) == DS_OK) {
    	    for (wid = 0; wid < devs; ++wid) {
                if (IsEqualGUID( &guid, &DSOUND_capture_guids[wid] ) ) {
                    err = mmErr(WineWaveInMessage((HWAVEIN)wid,DRV_QUERYDSOUNDDESC,(DWORD_PTR)&desc,0));
                    if (err == DS_OK) {
                        TRACE("calling lpDSEnumCallback(NULL,\"%s\",\"%s\",%p)\n",
                              "Primary Sound Capture Driver",desc.szDrvname,lpContext);
                        MultiByteToWideChar( CP_ACP, 0, "Primary Sound Capture Driver", -1,
                                             wDesc, sizeof(wDesc)/sizeof(WCHAR) );
                        MultiByteToWideChar( CP_ACP, 0, desc.szDrvname, -1,
                                             wName, sizeof(wName)/sizeof(WCHAR) );
                        if (lpDSEnumCallback(NULL, wDesc, wName, lpContext) == FALSE)
                            return DS_OK;
                    }
                }
	    }
	}
    }

    for (wid = 0; wid < devs; ++wid) {
	err = mmErr(WineWaveInMessage((HWAVEIN)wid,DRV_QUERYDSOUNDDESC,(DWORD_PTR)&desc,0));
	if (err == DS_OK) {
            TRACE("calling lpDSEnumCallback(%s,\"%s\",\"%s\",%p)\n",
                  debugstr_guid(&DSOUND_capture_guids[wid]),desc.szDesc,desc.szDrvname,lpContext);
            MultiByteToWideChar( CP_ACP, 0, desc.szDesc, -1,
                                 wDesc, sizeof(wDesc)/sizeof(WCHAR) );
            MultiByteToWideChar( CP_ACP, 0, desc.szDrvname, -1,
                                 wName, sizeof(wName)/sizeof(WCHAR) );
            if (lpDSEnumCallback((LPGUID)&DSOUND_capture_guids[wid], wDesc, wName, lpContext) == FALSE)
                return DS_OK;
	}
    }

    return DS_OK;
}

static void CALLBACK
DSOUND_capture_callback(
    HWAVEIN hwi,
    UINT msg,
    DWORD dwUser,
    DWORD dw1,
    DWORD dw2 )
{
    DirectSoundCaptureDevice * This = (DirectSoundCaptureDevice*)dwUser;
    TRACE("(%p,%08x(%s),%08lx,%08lx,%08lx) entering at %ld\n",hwi,msg,
	msg == MM_WIM_OPEN ? "MM_WIM_OPEN" : msg == MM_WIM_CLOSE ? "MM_WIM_CLOSE" :
	msg == MM_WIM_DATA ? "MM_WIM_DATA" : "UNKNOWN",dwUser,dw1,dw2,GetTickCount());

    if (msg == MM_WIM_DATA) {
        LPWAVEHDR pHdr = (LPWAVEHDR)dw1;
    	EnterCriticalSection( &(This->lock) );
	TRACE("DirectSoundCapture msg=MM_WIM_DATA, old This->state=%s, old This->index=%d\n",
	    captureStateString[This->state],This->index);
	if (This->state != STATE_STOPPED) {
	    int index = This->index;
	    if (This->state == STATE_STARTING) {
                This->read_position = pHdr->dwBytesRecorded;
		This->state = STATE_CAPTURING;
	    }
	    if (This->capture_buffer->nrofnotifies)
		SetEvent(This->capture_buffer->notifies[This->index].hEventNotify);
	    This->index = (This->index + 1) % This->nrofpwaves;
	    if ( (This->index == 0) && !(This->capture_buffer->flags & DSCBSTART_LOOPING) ) {
		TRACE("end of buffer\n");
		This->state = STATE_STOPPED;
	    } else {
		if (This->state == STATE_CAPTURING) {
		    waveInAddBuffer(hwi, &(This->pwave[index]), sizeof(WAVEHDR));
	        } else if (This->state == STATE_STOPPING) {
		    TRACE("stopping\n");
		    This->state = STATE_STOPPED;
		}
	    }
	}
	TRACE("DirectSoundCapture new This->state=%s, new This->index=%d\n",
	    captureStateString[This->state],This->index);
    	LeaveCriticalSection( &(This->lock) );
    }

    TRACE("completed\n");
}

static HRESULT WINAPI
IDirectSoundCaptureImpl_QueryInterface(
    LPDIRECTSOUNDCAPTURE iface,
    REFIID riid,
    LPVOID* ppobj )
{
    IDirectSoundCaptureImpl *This = (IDirectSoundCaptureImpl *)iface;
    TRACE( "(%p,%s,%p)\n", This, debugstr_guid(riid), ppobj );

    if (ppobj == NULL) {
	WARN("invalid parameter\n");
	return E_INVALIDARG;
    }

    *ppobj = NULL;

    if (IsEqualIID(riid, &IID_IUnknown)) {
        IDirectSoundCapture_AddRef((LPDIRECTSOUNDCAPTURE)This);
        *ppobj = This;
        return DS_OK;
    } else if (IsEqualIID(riid, &IID_IDirectSoundCapture)) {
        IDirectSoundCapture_AddRef((LPDIRECTSOUNDCAPTURE)This);
        *ppobj = This;
        return DS_OK;
    }

    WARN("unsupported riid: %s\n", debugstr_guid(riid));
    return E_NOINTERFACE;
}

static ULONG WINAPI
IDirectSoundCaptureImpl_AddRef( LPDIRECTSOUNDCAPTURE iface )
{
    IDirectSoundCaptureImpl *This = (IDirectSoundCaptureImpl *)iface;
    ULONG ref = InterlockedIncrement(&(This->ref));
    TRACE("(%p) ref was %ld\n", This, ref - 1);
    return ref;
}

static ULONG DirectSoundCaptureDevice_Release(
    DirectSoundCaptureDevice * device)
{
    ULONG ref;
    TRACE("(%p) ref was %lu\n", device, device->ref);

    device->ref--;
    ref=device->ref;
    if (device->ref == 0) {
        TRACE("deleting object\n");
        if (device->capture_buffer)
            IDirectSoundCaptureBufferImpl_Release(
		(LPDIRECTSOUNDCAPTUREBUFFER8) device->capture_buffer);

        if (device->driver) {
            IDsCaptureDriver_Close(device->driver);
            IDsCaptureDriver_Release(device->driver);
        }

        HeapFree(GetProcessHeap(), 0, device->pwfx);
        device->lock.DebugInfo->Spare[0] = 0;
        DeleteCriticalSection( &(device->lock) );
        DSOUND_capture[device->drvdesc.dnDevNode] = NULL;
        HeapFree(GetProcessHeap(), 0, device);
	TRACE("(%p) released\n", device);
    }
    return ref;
}

static ULONG WINAPI
IDirectSoundCaptureImpl_Release( LPDIRECTSOUNDCAPTURE iface )
{
    IDirectSoundCaptureImpl *This = (IDirectSoundCaptureImpl *)iface;
    ULONG ref = InterlockedDecrement(&(This->ref));
    TRACE("(%p) ref was %ld\n", This, ref + 1);

    if (!ref) {
        if (This->device)
            DirectSoundCaptureDevice_Release(This->device);

        HeapFree( GetProcessHeap(), 0, This );
	TRACE("(%p) released\n", This);
    }
    return ref;
}

static HRESULT WINAPI
IDirectSoundCaptureImpl_CreateCaptureBuffer(
    LPDIRECTSOUNDCAPTURE iface,
    LPCDSCBUFFERDESC lpcDSCBufferDesc,
    LPDIRECTSOUNDCAPTUREBUFFER* lplpDSCaptureBuffer,
    LPUNKNOWN pUnk )
{
    HRESULT hr;
    IDirectSoundCaptureImpl *This = (IDirectSoundCaptureImpl *)iface;

    TRACE( "(%p,%p,%p,%p)\n",iface,lpcDSCBufferDesc,lplpDSCaptureBuffer,pUnk);

    if (This == NULL) {
	WARN("invalid parameter: This == NULL\n");
	return DSERR_INVALIDPARAM;
    }

    if (lpcDSCBufferDesc == NULL) {
	WARN("invalid parameter: lpcDSCBufferDesc == NULL)\n");
	return DSERR_INVALIDPARAM;
    }

    if (lplpDSCaptureBuffer == NULL) {
	WARN("invalid parameter: lplpDSCaptureBuffer == NULL\n");
	return DSERR_INVALIDPARAM;
    }

    if (pUnk != NULL) {
	WARN("invalid parameter: pUnk != NULL\n");
	return DSERR_INVALIDPARAM;
    }

    /* FIXME: We can only have one buffer so what do we do here? */
    if (This->device->capture_buffer) {
	WARN("lnvalid parameter: already has buffer\n");
	return DSERR_INVALIDPARAM;    /* DSERR_GENERIC ? */
    }

    hr = DSOUND_CreateDirectSoundCaptureBuffer(This, lpcDSCBufferDesc,
        (LPVOID*)lplpDSCaptureBuffer );

    if (hr != DS_OK)
	WARN("DSOUND_CreateDirectSoundCaptureBuffer failed\n");

    return hr;
}

static HRESULT WINAPI
IDirectSoundCaptureImpl_GetCaps(
    LPDIRECTSOUNDCAPTURE iface,
    LPDSCCAPS lpDSCCaps )
{
    IDirectSoundCaptureImpl *This = (IDirectSoundCaptureImpl *)iface;
    TRACE("(%p,%p)\n",This,lpDSCCaps);

    if (This->device == NULL) {
	WARN("not initialized\n");
	return DSERR_UNINITIALIZED;
    }

    if (lpDSCCaps== NULL) {
	WARN("invalid parameter: lpDSCCaps== NULL\n");
	return DSERR_INVALIDPARAM;
    }

    if (lpDSCCaps->dwSize < sizeof(*lpDSCCaps)) {
	WARN("invalid parameter: lpDSCCaps->dwSize = %ld < %d\n",
	    lpDSCCaps->dwSize, sizeof(*lpDSCCaps));
	return DSERR_INVALIDPARAM;
    }

    lpDSCCaps->dwFlags = This->device->drvcaps.dwFlags;
    lpDSCCaps->dwFormats = This->device->drvcaps.dwFormats;
    lpDSCCaps->dwChannels = This->device->drvcaps.dwChannels;

    TRACE("(flags=0x%08lx,format=0x%08lx,channels=%ld)\n",lpDSCCaps->dwFlags,
        lpDSCCaps->dwFormats, lpDSCCaps->dwChannels);

    return DS_OK;
}

static HRESULT WINAPI
IDirectSoundCaptureImpl_Initialize(
    LPDIRECTSOUNDCAPTURE iface,
    LPCGUID lpcGUID )
{
    HRESULT err = DSERR_INVALIDPARAM;
    unsigned wid, widn;
    BOOLEAN found = FALSE;
    GUID devGUID;
    IDirectSoundCaptureImpl *This = (IDirectSoundCaptureImpl *)iface;
    DirectSoundCaptureDevice *device = This->device;
    TRACE("(%p)\n", This);

    if (!This) {
	WARN("invalid parameter: This == NULL\n");
	return DSERR_INVALIDPARAM;
    }

    if (device != NULL) {
	WARN("already initialized\n");
	return DSERR_ALREADYINITIALIZED;
    }

    /* Default device? */
    if ( !lpcGUID || IsEqualGUID(lpcGUID, &GUID_NULL) )
	lpcGUID = &DSDEVID_DefaultCapture;

    if (GetDeviceID(lpcGUID, &devGUID) != DS_OK) {
        WARN("invalid parameter: lpcGUID\n");
        return DSERR_INVALIDPARAM;
    }

    widn = waveInGetNumDevs();
    if (!widn) {
	WARN("no audio devices found\n");
	return DSERR_NODRIVER;
    }

    /* enumerate WINMM audio devices and find the one we want */
    for (wid=0; wid<widn; wid++) {
    	if (IsEqualGUID( &devGUID, &DSOUND_capture_guids[wid]) ) {
	    found = TRUE;
	    break;
	}
    }

    if (found == FALSE) {
	WARN("No device found matching given ID!\n");
	return DSERR_NODRIVER;
    }

    if (DSOUND_capture[wid]) {
        WARN("already in use\n");
        return DSERR_ALLOCATED;
    }

    err = DirectSoundCaptureDevice_Create(&(device));
    if (err != DS_OK) {
        WARN("DirectSoundCaptureDevice_Create failed\n");
        return err;
    }

    This->device = device;
    device->guid = devGUID;

    err = mmErr(WineWaveInMessage((HWAVEIN)wid,DRV_QUERYDSOUNDIFACE,(DWORD_PTR)&(This->device->driver),0));
    if ( (err != DS_OK) && (err != DSERR_UNSUPPORTED) ) {
	WARN("WineWaveInMessage failed; err=%lx\n",err);
	return err;
    }
    err = DS_OK;

    /* Disable the direct sound driver to force emulation if requested. */
    if (ds_hw_accel == DS_HW_ACCEL_EMULATION)
	This->device->driver = NULL;

    /* Get driver description */
    if (This->device->driver) {
        TRACE("using DirectSound driver\n");
        err = IDsCaptureDriver_GetDriverDesc(This->device->driver, &(This->device->drvdesc));
	if (err != DS_OK) {
	    WARN("IDsCaptureDriver_GetDriverDesc failed\n");
	    return err;
	}
    } else {
        TRACE("using WINMM\n");
        /* if no DirectSound interface available, use WINMM API instead */
        This->device->drvdesc.dwFlags = DSDDESC_DOMMSYSTEMOPEN |
            DSDDESC_DOMMSYSTEMSETFORMAT;
    }

    This->device->drvdesc.dnDevNode = wid;

    /* open the DirectSound driver if available */
    if (This->device->driver && (err == DS_OK))
        err = IDsCaptureDriver_Open(This->device->driver);

    if (err == DS_OK) {
        This->device = device;

        /* the driver is now open, so it's now allowed to call GetCaps */
        if (This->device->driver) {
	    This->device->drvcaps.dwSize = sizeof(This->device->drvcaps);
            err = IDsCaptureDriver_GetCaps(This->device->driver,&(This->device->drvcaps));
	    if (err != DS_OK) {
		WARN("IDsCaptureDriver_GetCaps failed\n");
		return err;
	    }
        } else /*if (This->hwi)*/ {
            WAVEINCAPSA    wic;
            err = mmErr(waveInGetDevCapsA((UINT)This->device->drvdesc.dnDevNode, &wic, sizeof(wic)));

            if (err == DS_OK) {
                This->device->drvcaps.dwFlags = 0;
                lstrcpynA(This->device->drvdesc.szDrvname, wic.szPname,
                          sizeof(This->device->drvdesc.szDrvname));

                This->device->drvcaps.dwFlags |= DSCCAPS_EMULDRIVER;
                This->device->drvcaps.dwFormats = wic.dwFormats;
                This->device->drvcaps.dwChannels = wic.wChannels;
            }
        }
    }

    return err;
}

static const IDirectSoundCaptureVtbl dscvt =
{
    /* IUnknown methods */
    IDirectSoundCaptureImpl_QueryInterface,
    IDirectSoundCaptureImpl_AddRef,
    IDirectSoundCaptureImpl_Release,

    /* IDirectSoundCapture methods */
    IDirectSoundCaptureImpl_CreateCaptureBuffer,
    IDirectSoundCaptureImpl_GetCaps,
    IDirectSoundCaptureImpl_Initialize
};

static HRESULT
DSOUND_CreateDirectSoundCaptureBuffer(
    IDirectSoundCaptureImpl *ipDSC,
    LPCDSCBUFFERDESC lpcDSCBufferDesc,
    LPVOID* ppobj )
{
    LPWAVEFORMATEX  wfex;
    TRACE( "(%p,%p)\n", lpcDSCBufferDesc, ppobj );

    if (ipDSC == NULL) {
	WARN("invalid parameter: ipDSC == NULL\n");
	return DSERR_INVALIDPARAM;
    }

    if (lpcDSCBufferDesc == NULL) {
	WARN("invalid parameter: lpcDSCBufferDesc == NULL\n");
	return DSERR_INVALIDPARAM;
    }

    if (ppobj == NULL) {
	WARN("invalid parameter: ppobj == NULL\n");
	return DSERR_INVALIDPARAM;
    }

    if ( ((lpcDSCBufferDesc->dwSize != sizeof(DSCBUFFERDESC)) &&
          (lpcDSCBufferDesc->dwSize != sizeof(DSCBUFFERDESC1))) ||
        (lpcDSCBufferDesc->dwBufferBytes == 0) ||
        (lpcDSCBufferDesc->lpwfxFormat == NULL) ) {
	WARN("invalid lpcDSCBufferDesc\n");
	*ppobj = NULL;
	return DSERR_INVALIDPARAM;
    }

    if ( !ipDSC->device) {
	WARN("not initialized\n");
	*ppobj = NULL;
	return DSERR_UNINITIALIZED;
    }

    wfex = lpcDSCBufferDesc->lpwfxFormat;

    if (wfex) {
        TRACE("(formattag=0x%04x,chans=%d,samplerate=%ld,"
            "bytespersec=%ld,blockalign=%d,bitspersamp=%d,cbSize=%d)\n",
            wfex->wFormatTag, wfex->nChannels, wfex->nSamplesPerSec,
            wfex->nAvgBytesPerSec, wfex->nBlockAlign,
            wfex->wBitsPerSample, wfex->cbSize);

        if (wfex->wFormatTag == WAVE_FORMAT_PCM) {
	    ipDSC->device->pwfx = HeapAlloc(GetProcessHeap(),0,sizeof(WAVEFORMATEX));
            CopyMemory(ipDSC->device->pwfx, wfex, sizeof(WAVEFORMATEX));
	    ipDSC->device->pwfx->cbSize = 0;
	} else {
	    ipDSC->device->pwfx = HeapAlloc(GetProcessHeap(),0,sizeof(WAVEFORMATEX)+wfex->cbSize);
            CopyMemory(ipDSC->device->pwfx, wfex, sizeof(WAVEFORMATEX)+wfex->cbSize);
        }
    } else {
	WARN("lpcDSCBufferDesc->lpwfxFormat == 0\n");
	*ppobj = NULL;
	return DSERR_INVALIDPARAM; /* FIXME: DSERR_BADFORMAT ? */
    }

    *ppobj = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,
        sizeof(IDirectSoundCaptureBufferImpl));

    if ( *ppobj == NULL ) {
	WARN("out of memory\n");
	*ppobj = NULL;
	return DSERR_OUTOFMEMORY;
    } else {
    	HRESULT err = DS_OK;
        LPBYTE newbuf;
        DWORD buflen;
        IDirectSoundCaptureBufferImpl *This = (IDirectSoundCaptureBufferImpl *)*ppobj;

        This->ref = 1;
        This->dsound = ipDSC;
        This->dsound->device->capture_buffer = This;
	This->notify = NULL;
	This->nrofnotifies = 0;
	This->hwnotify = NULL;

        This->pdscbd = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,
            lpcDSCBufferDesc->dwSize);
        if (This->pdscbd)
            CopyMemory(This->pdscbd, lpcDSCBufferDesc, lpcDSCBufferDesc->dwSize);
        else {
            WARN("no memory\n");
            This->dsound->device->capture_buffer = 0;
            HeapFree( GetProcessHeap(), 0, This );
            *ppobj = NULL;
            return DSERR_OUTOFMEMORY;
        }

        This->lpVtbl = &dscbvt;

	if (ipDSC->device->driver) {
            if (This->dsound->device->drvdesc.dwFlags & DSDDESC_DOMMSYSTEMOPEN)
                FIXME("DSDDESC_DOMMSYSTEMOPEN not supported\n");

            if (This->dsound->device->drvdesc.dwFlags & DSDDESC_USESYSTEMMEMORY) {
                /* allocate buffer from system memory */
                buflen = lpcDSCBufferDesc->dwBufferBytes;
                TRACE("desired buflen=%ld, old buffer=%p\n", buflen, ipDSC->device->buffer);
                if (ipDSC->device->buffer)
                    newbuf = HeapReAlloc(GetProcessHeap(),0,ipDSC->device->buffer,buflen);
                else
                    newbuf = HeapAlloc(GetProcessHeap(),0,buflen);

                if (newbuf == NULL) {
                    WARN("failed to allocate capture buffer\n");
                    err = DSERR_OUTOFMEMORY;
                    /* but the old buffer might still exist and must be re-prepared */
                } else {
                    ipDSC->device->buffer = newbuf;
                    ipDSC->device->buflen = buflen;
                }
            } else {
                /* let driver allocate memory */
                ipDSC->device->buflen = lpcDSCBufferDesc->dwBufferBytes;
                /* FIXME: */
                HeapFree( GetProcessHeap(), 0, ipDSC->device->buffer);
                ipDSC->device->buffer = NULL;
            }

	    err = IDsCaptureDriver_CreateCaptureBuffer(ipDSC->device->driver,
		ipDSC->device->pwfx,0,0,&(ipDSC->device->buflen),&(ipDSC->device->buffer),(LPVOID*)&(ipDSC->device->hwbuf));
	    if (err != DS_OK) {
		WARN("IDsCaptureDriver_CreateCaptureBuffer failed\n");
		This->dsound->device->capture_buffer = 0;
		HeapFree( GetProcessHeap(), 0, This );
		*ppobj = NULL;
		return err;
	    }
	} else {
	    DWORD flags = CALLBACK_FUNCTION;
	    if (ds_hw_accel != DS_HW_ACCEL_EMULATION)
		flags |= WAVE_DIRECTSOUND;
            err = mmErr(waveInOpen(&(ipDSC->device->hwi),
                ipDSC->device->drvdesc.dnDevNode, ipDSC->device->pwfx,
                (DWORD_PTR)DSOUND_capture_callback, (DWORD)ipDSC->device, flags));
            if (err != DS_OK) {
                WARN("waveInOpen failed\n");
		This->dsound->device->capture_buffer = 0;
		HeapFree( GetProcessHeap(), 0, This );
		*ppobj = NULL;
		return err;
            }

	    buflen = lpcDSCBufferDesc->dwBufferBytes;
            TRACE("desired buflen=%ld, old buffer=%p\n", buflen, ipDSC->device->buffer);
	    if (ipDSC->device->buffer)
                newbuf = HeapReAlloc(GetProcessHeap(),0,ipDSC->device->buffer,buflen);
	    else
		newbuf = HeapAlloc(GetProcessHeap(),0,buflen);
            if (newbuf == NULL) {
                WARN("failed to allocate capture buffer\n");
                err = DSERR_OUTOFMEMORY;
                /* but the old buffer might still exist and must be re-prepared */
            } else {
                ipDSC->device->buffer = newbuf;
                ipDSC->device->buflen = buflen;
            }
	}
    }

    TRACE("returning DS_OK\n");
    return DS_OK;
}

/*******************************************************************************
 *		IDirectSoundCaptureNotify
 */
static HRESULT WINAPI IDirectSoundCaptureNotifyImpl_QueryInterface(
    LPDIRECTSOUNDNOTIFY iface,
    REFIID riid,
    LPVOID *ppobj)
{
    IDirectSoundCaptureNotifyImpl *This = (IDirectSoundCaptureNotifyImpl *)iface;
    TRACE("(%p,%s,%p)\n",This,debugstr_guid(riid),ppobj);

    if (This->dscb == NULL) {
	WARN("invalid parameter\n");
	return E_INVALIDARG;
    }

    return IDirectSoundCaptureBuffer_QueryInterface((LPDIRECTSOUNDCAPTUREBUFFER)This->dscb, riid, ppobj);
}

static ULONG WINAPI IDirectSoundCaptureNotifyImpl_AddRef(LPDIRECTSOUNDNOTIFY iface)
{
    IDirectSoundCaptureNotifyImpl *This = (IDirectSoundCaptureNotifyImpl *)iface;
    ULONG ref = InterlockedIncrement(&(This->ref));
    TRACE("(%p) ref was %ld\n", This, ref - 1);
    return ref;
}

static ULONG WINAPI IDirectSoundCaptureNotifyImpl_Release(LPDIRECTSOUNDNOTIFY iface)
{
    IDirectSoundCaptureNotifyImpl *This = (IDirectSoundCaptureNotifyImpl *)iface;
    ULONG ref = InterlockedDecrement(&(This->ref));
    TRACE("(%p) ref was %ld\n", This, ref + 1);

    if (!ref) {
        if (This->dscb->hwnotify)
            IDsDriverNotify_Release(This->dscb->hwnotify);
	This->dscb->notify=NULL;
	IDirectSoundCaptureBuffer_Release((LPDIRECTSOUNDCAPTUREBUFFER)This->dscb);
	HeapFree(GetProcessHeap(),0,This);
	TRACE("(%p) released\n", This);
    }
    return ref;
}

static HRESULT WINAPI IDirectSoundCaptureNotifyImpl_SetNotificationPositions(
    LPDIRECTSOUNDNOTIFY iface,
    DWORD howmuch,
    LPCDSBPOSITIONNOTIFY notify)
{
    IDirectSoundCaptureNotifyImpl *This = (IDirectSoundCaptureNotifyImpl *)iface;
    TRACE("(%p,0x%08lx,%p)\n",This,howmuch,notify);

    if (howmuch > 0 && notify == NULL) {
	WARN("invalid parameter: notify == NULL\n");
	return DSERR_INVALIDPARAM;
    }

    if (TRACE_ON(dsound)) {
	unsigned int i;
	for (i=0;i<howmuch;i++)
	    TRACE("notify at %ld to %p\n",
	    notify[i].dwOffset,notify[i].hEventNotify);
    }

    if (This->dscb->hwnotify) {
	HRESULT hres;
	hres = IDsDriverNotify_SetNotificationPositions(This->dscb->hwnotify, howmuch, notify);
	if (hres != DS_OK)
	    WARN("IDsDriverNotify_SetNotificationPositions failed\n");
	return hres;
    } else if (howmuch > 0) {
	/* Make an internal copy of the caller-supplied array.
	 * Replace the existing copy if one is already present. */
	if (This->dscb->notifies)
	    This->dscb->notifies = HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
		This->dscb->notifies, howmuch * sizeof(DSBPOSITIONNOTIFY));
	else
	    This->dscb->notifies = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
		howmuch * sizeof(DSBPOSITIONNOTIFY));

	if (This->dscb->notifies == NULL) {
	    WARN("out of memory\n");
	    return DSERR_OUTOFMEMORY;
	}
	CopyMemory(This->dscb->notifies, notify, howmuch * sizeof(DSBPOSITIONNOTIFY));
	This->dscb->nrofnotifies = howmuch;
    } else {
        HeapFree(GetProcessHeap(), 0, This->dscb->notifies);
        This->dscb->notifies = NULL;
        This->dscb->nrofnotifies = 0;
    }

    return S_OK;
}

static const IDirectSoundNotifyVtbl dscnvt =
{
    IDirectSoundCaptureNotifyImpl_QueryInterface,
    IDirectSoundCaptureNotifyImpl_AddRef,
    IDirectSoundCaptureNotifyImpl_Release,
    IDirectSoundCaptureNotifyImpl_SetNotificationPositions,
};

HRESULT WINAPI IDirectSoundCaptureNotifyImpl_Create(
    IDirectSoundCaptureBufferImpl *dscb,
    IDirectSoundCaptureNotifyImpl **pdscn)
{
    IDirectSoundCaptureNotifyImpl * dscn;
    TRACE("(%p,%p)\n",dscb,pdscn);

    dscn = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(dscn));

    if (dscn == NULL) {
	WARN("out of memory\n");
	return DSERR_OUTOFMEMORY;
    }

    dscn->ref = 0;
    dscn->lpVtbl = &dscnvt;
    dscn->dscb = dscb;
    dscb->notify = dscn;
    IDirectSoundCaptureBuffer_AddRef((LPDIRECTSOUNDCAPTUREBUFFER)dscb);

    *pdscn = dscn;
    return DS_OK;
}

/*******************************************************************************
 *		IDirectSoundCaptureBuffer
 */
static HRESULT WINAPI
IDirectSoundCaptureBufferImpl_QueryInterface(
    LPDIRECTSOUNDCAPTUREBUFFER8 iface,
    REFIID riid,
    LPVOID* ppobj )
{
    IDirectSoundCaptureBufferImpl *This = (IDirectSoundCaptureBufferImpl *)iface;
    HRESULT hres;
    TRACE( "(%p,%s,%p)\n", This, debugstr_guid(riid), ppobj );

    if (ppobj == NULL) {
	WARN("invalid parameter\n");
	return E_INVALIDARG;
    }

    *ppobj = NULL;

    if ( IsEqualGUID( &IID_IDirectSoundNotify, riid ) ) {
	if (!This->notify)
	    hres = IDirectSoundCaptureNotifyImpl_Create(This, &This->notify);
	if (This->notify) {
	    if (This->dsound->device->hwbuf) {
		hres = IDsCaptureDriverBuffer_QueryInterface(This->dsound->device->hwbuf,
		    &IID_IDsDriverNotify, (LPVOID*)&(This->hwnotify));
		if (hres != DS_OK) {
		    WARN("IDsCaptureDriverBuffer_QueryInterface failed\n");
		    *ppobj = 0;
		    return hres;
	        }
	    }

	    IDirectSoundNotify_AddRef((LPDIRECTSOUNDNOTIFY)This->notify);
	    *ppobj = (LPVOID)This->notify;
	    return DS_OK;
	}

	WARN("IID_IDirectSoundNotify\n");
	return E_FAIL;
    }

    if ( IsEqualGUID( &IID_IDirectSoundCaptureBuffer, riid ) ||
         IsEqualGUID( &IID_IDirectSoundCaptureBuffer8, riid ) ) {
	IDirectSoundCaptureBuffer8_AddRef(iface);
	*ppobj = This;
	return NO_ERROR;
    }

    FIXME("(%p,%s,%p) unsupported GUID\n", This, debugstr_guid(riid), ppobj);
    return E_NOINTERFACE;
}

static ULONG WINAPI
IDirectSoundCaptureBufferImpl_AddRef( LPDIRECTSOUNDCAPTUREBUFFER8 iface )
{
    IDirectSoundCaptureBufferImpl *This = (IDirectSoundCaptureBufferImpl *)iface;
    ULONG ref = InterlockedIncrement(&(This->ref));
    TRACE("(%p) ref was %ld\n", This, ref - 1);
    return ref;
}

static ULONG WINAPI
IDirectSoundCaptureBufferImpl_Release( LPDIRECTSOUNDCAPTUREBUFFER8 iface )
{
    IDirectSoundCaptureBufferImpl *This = (IDirectSoundCaptureBufferImpl *)iface;
    ULONG ref = InterlockedDecrement(&(This->ref));
    TRACE("(%p) ref was %ld\n", This, ref + 1);

    if (!ref) {
        TRACE("deleting object\n");
	if (This->dsound->device->state == STATE_CAPTURING)
	    This->dsound->device->state = STATE_STOPPING;

        HeapFree(GetProcessHeap(),0, This->pdscbd);

	if (This->dsound->device->hwi) {
	    waveInReset(This->dsound->device->hwi);
	    waveInClose(This->dsound->device->hwi);
            HeapFree(GetProcessHeap(),0, This->dsound->device->pwave);
            This->dsound->device->pwave = 0;
	    This->dsound->device->hwi = 0;
	}

	if (This->dsound->device->hwbuf)
	    IDsCaptureDriverBuffer_Release(This->dsound->device->hwbuf);

        /* remove from IDirectSoundCaptureImpl */
        if (This->dsound)
            This->dsound->device->capture_buffer = NULL;
        else
            ERR("does not reference dsound\n");

        if (This->notify)
	    IDirectSoundNotify_Release((LPDIRECTSOUNDNOTIFY)This->notify);

	HeapFree(GetProcessHeap(), 0, This->notifies);
        HeapFree( GetProcessHeap(), 0, This );
	TRACE("(%p) released\n", This);
    }
    return ref;
}

static HRESULT WINAPI
IDirectSoundCaptureBufferImpl_GetCaps(
    LPDIRECTSOUNDCAPTUREBUFFER8 iface,
    LPDSCBCAPS lpDSCBCaps )
{
    IDirectSoundCaptureBufferImpl *This = (IDirectSoundCaptureBufferImpl *)iface;
    TRACE( "(%p,%p)\n", This, lpDSCBCaps );

    if (This == NULL) {
        WARN("invalid parameter: This == NULL\n");
        return DSERR_INVALIDPARAM;
    }

    if (lpDSCBCaps == NULL) {
        WARN("invalid parameter: lpDSCBCaps == NULL\n");
        return DSERR_INVALIDPARAM;
    }

    if (lpDSCBCaps->dwSize < sizeof(DSCBCAPS)) {
        WARN("invalid parameter: lpDSCBCaps->dwSize = %ld < %d\n",
	    lpDSCBCaps->dwSize, sizeof(DSCBCAPS));
        return DSERR_INVALIDPARAM;
    }

    if (This->dsound == NULL) {
        WARN("invalid parameter: This->dsound == NULL\n");
        return DSERR_INVALIDPARAM;
    }

    lpDSCBCaps->dwSize = sizeof(DSCBCAPS);
    lpDSCBCaps->dwFlags = This->flags;
    lpDSCBCaps->dwBufferBytes = This->pdscbd->dwBufferBytes;
    lpDSCBCaps->dwReserved = 0;

    TRACE("returning DS_OK\n");
    return DS_OK;
}

static HRESULT WINAPI
IDirectSoundCaptureBufferImpl_GetCurrentPosition(
    LPDIRECTSOUNDCAPTUREBUFFER8 iface,
    LPDWORD lpdwCapturePosition,
    LPDWORD lpdwReadPosition )
{
    IDirectSoundCaptureBufferImpl *This = (IDirectSoundCaptureBufferImpl *)iface;
    HRESULT hres = DS_OK;
    TRACE( "(%p,%p,%p)\n", This, lpdwCapturePosition, lpdwReadPosition );

    if (This == NULL) {
        WARN("invalid parameter: This == NULL\n");
        return DSERR_INVALIDPARAM;
    }

    if (This->dsound == NULL) {
        WARN("invalid parameter: This->dsound == NULL\n");
        return DSERR_INVALIDPARAM;
    }

    if (This->dsound->device->driver) {
        hres = IDsCaptureDriverBuffer_GetPosition(This->dsound->device->hwbuf, lpdwCapturePosition, lpdwReadPosition );
	if (hres != DS_OK)
	    WARN("IDsCaptureDriverBuffer_GetPosition failed\n");
    } else if (This->dsound->device->hwi) {
	EnterCriticalSection(&(This->dsound->device->lock));
	TRACE("old This->dsound->device->state=%s\n",captureStateString[This->dsound->device->state]);
        if (lpdwCapturePosition) {
            MMTIME mtime;
            mtime.wType = TIME_BYTES;
            waveInGetPosition(This->dsound->device->hwi, &mtime, sizeof(mtime));
	    TRACE("mtime.u.cb=%ld,This->dsound->device->buflen=%ld\n", mtime.u.cb,
		This->dsound->device->buflen);
	    mtime.u.cb = mtime.u.cb % This->dsound->device->buflen;
            *lpdwCapturePosition = mtime.u.cb;
        }

	if (lpdwReadPosition) {
            if (This->dsound->device->state == STATE_STARTING) {
		if (lpdwCapturePosition)
		    This->dsound->device->read_position = *lpdwCapturePosition;
                This->dsound->device->state = STATE_CAPTURING;
            }
            *lpdwReadPosition = This->dsound->device->read_position;
        }
	TRACE("new This->dsound->device->state=%s\n",captureStateString[This->dsound->device->state]);
	LeaveCriticalSection(&(This->dsound->device->lock));
	if (lpdwCapturePosition) TRACE("*lpdwCapturePosition=%ld\n",*lpdwCapturePosition);
	if (lpdwReadPosition) TRACE("*lpdwReadPosition=%ld\n",*lpdwReadPosition);
    } else {
        WARN("no driver\n");
        hres = DSERR_NODRIVER;
    }

    TRACE("returning %08lx\n", hres);
    return hres;
}

static HRESULT WINAPI
IDirectSoundCaptureBufferImpl_GetFormat(
    LPDIRECTSOUNDCAPTUREBUFFER8 iface,
    LPWAVEFORMATEX lpwfxFormat,
    DWORD dwSizeAllocated,
    LPDWORD lpdwSizeWritten )
{
    IDirectSoundCaptureBufferImpl *This = (IDirectSoundCaptureBufferImpl *)iface;
    HRESULT hres = DS_OK;
    TRACE( "(%p,%p,0x%08lx,%p)\n", This, lpwfxFormat, dwSizeAllocated,
        lpdwSizeWritten );

    if (This == NULL) {
        WARN("invalid parameter: This == NULL\n");
        return DSERR_INVALIDPARAM;
    }

    if (This->dsound == NULL) {
        WARN("invalid parameter: This->dsound == NULL\n");
        return DSERR_INVALIDPARAM;
    }

    if (dwSizeAllocated > (sizeof(WAVEFORMATEX) + This->dsound->device->pwfx->cbSize))
        dwSizeAllocated = sizeof(WAVEFORMATEX) + This->dsound->device->pwfx->cbSize;

    if (lpwfxFormat) { /* NULL is valid (just want size) */
        CopyMemory(lpwfxFormat, This->dsound->device->pwfx, dwSizeAllocated);
        if (lpdwSizeWritten)
            *lpdwSizeWritten = dwSizeAllocated;
    } else {
        if (lpdwSizeWritten)
            *lpdwSizeWritten = sizeof(WAVEFORMATEX) + This->dsound->device->pwfx->cbSize;
        else {
            TRACE("invalid parameter: lpdwSizeWritten = NULL\n");
            hres = DSERR_INVALIDPARAM;
        }
    }

    TRACE("returning %08lx\n", hres);
    return hres;
}

static HRESULT WINAPI
IDirectSoundCaptureBufferImpl_GetStatus(
    LPDIRECTSOUNDCAPTUREBUFFER8 iface,
    LPDWORD lpdwStatus )
{
    IDirectSoundCaptureBufferImpl *This = (IDirectSoundCaptureBufferImpl *)iface;
    TRACE( "(%p, %p), thread is %04lx\n", This, lpdwStatus, GetCurrentThreadId() );

    if (This == NULL) {
        WARN("invalid parameter: This == NULL\n");
        return DSERR_INVALIDPARAM;
    }

    if (This->dsound == NULL) {
        WARN("invalid parameter: This->dsound == NULL\n");
        return DSERR_INVALIDPARAM;
    }

    if (lpdwStatus == NULL) {
        WARN("invalid parameter: lpdwStatus == NULL\n");
        return DSERR_INVALIDPARAM;
    }

    *lpdwStatus = 0;
    EnterCriticalSection(&(This->dsound->device->lock));

    TRACE("old This->dsound->state=%s, old lpdwStatus=%08lx\n",
	captureStateString[This->dsound->device->state],*lpdwStatus);
    if ((This->dsound->device->state == STATE_STARTING) ||
        (This->dsound->device->state == STATE_CAPTURING)) {
        *lpdwStatus |= DSCBSTATUS_CAPTURING;
        if (This->flags & DSCBSTART_LOOPING)
            *lpdwStatus |= DSCBSTATUS_LOOPING;
    }
    TRACE("new This->dsound->state=%s, new lpdwStatus=%08lx\n",
	captureStateString[This->dsound->device->state],*lpdwStatus);
    LeaveCriticalSection(&(This->dsound->device->lock));

    TRACE("status=%lx\n", *lpdwStatus);
    TRACE("returning DS_OK\n");
    return DS_OK;
}

static HRESULT WINAPI
IDirectSoundCaptureBufferImpl_Initialize(
    LPDIRECTSOUNDCAPTUREBUFFER8 iface,
    LPDIRECTSOUNDCAPTURE lpDSC,
    LPCDSCBUFFERDESC lpcDSCBDesc )
{
    IDirectSoundCaptureBufferImpl *This = (IDirectSoundCaptureBufferImpl *)iface;

    FIXME( "(%p,%p,%p): stub\n", This, lpDSC, lpcDSCBDesc );

    return DS_OK;
}

static HRESULT WINAPI
IDirectSoundCaptureBufferImpl_Lock(
    LPDIRECTSOUNDCAPTUREBUFFER8 iface,
    DWORD dwReadCusor,
    DWORD dwReadBytes,
    LPVOID* lplpvAudioPtr1,
    LPDWORD lpdwAudioBytes1,
    LPVOID* lplpvAudioPtr2,
    LPDWORD lpdwAudioBytes2,
    DWORD dwFlags )
{
    HRESULT hres = DS_OK;
    IDirectSoundCaptureBufferImpl *This = (IDirectSoundCaptureBufferImpl *)iface;
    TRACE( "(%p,%08lu,%08lu,%p,%p,%p,%p,0x%08lx) at %ld\n", This, dwReadCusor,
        dwReadBytes, lplpvAudioPtr1, lpdwAudioBytes1, lplpvAudioPtr2,
        lpdwAudioBytes2, dwFlags, GetTickCount() );

    if (This == NULL) {
        WARN("invalid parameter: This == NULL\n");
        return DSERR_INVALIDPARAM;
    }

    if (This->dsound == NULL) {
        WARN("invalid parameter: This->dsound == NULL\n");
        return DSERR_INVALIDPARAM;
    }

    if (lplpvAudioPtr1 == NULL) {
        WARN("invalid parameter: lplpvAudioPtr1 == NULL\n");
        return DSERR_INVALIDPARAM;
    }

    if (lpdwAudioBytes1 == NULL) {
        WARN("invalid parameter: lpdwAudioBytes1 == NULL\n");
        return DSERR_INVALIDPARAM;
    }

    EnterCriticalSection(&(This->dsound->device->lock));

    if (This->dsound->device->driver) {
        hres = IDsCaptureDriverBuffer_Lock(This->dsound->device->hwbuf, lplpvAudioPtr1,
                                           lpdwAudioBytes1, lplpvAudioPtr2,
                                           lpdwAudioBytes2, dwReadCusor,
                                           dwReadBytes, dwFlags);
	if (hres != DS_OK)
	    WARN("IDsCaptureDriverBuffer_Lock failed\n");
    } else if (This->dsound->device->hwi) {
        *lplpvAudioPtr1 = This->dsound->device->buffer + dwReadCusor;
        if ( (dwReadCusor + dwReadBytes) > This->dsound->device->buflen) {
            *lpdwAudioBytes1 = This->dsound->device->buflen - dwReadCusor;
	    if (lplpvAudioPtr2)
            	*lplpvAudioPtr2 = This->dsound->device->buffer;
	    if (lpdwAudioBytes2)
		*lpdwAudioBytes2 = dwReadBytes - *lpdwAudioBytes1;
        } else {
            *lpdwAudioBytes1 = dwReadBytes;
	    if (lplpvAudioPtr2)
            	*lplpvAudioPtr2 = 0;
	    if (lpdwAudioBytes2)
            	*lpdwAudioBytes2 = 0;
        }
    } else {
        TRACE("invalid call\n");
        hres = DSERR_INVALIDCALL;   /* DSERR_NODRIVER ? */
    }

    LeaveCriticalSection(&(This->dsound->device->lock));

    TRACE("returning %08lx\n", hres);
    return hres;
}

static HRESULT WINAPI
IDirectSoundCaptureBufferImpl_Start(
    LPDIRECTSOUNDCAPTUREBUFFER8 iface,
    DWORD dwFlags )
{
    HRESULT hres = DS_OK;
    IDirectSoundCaptureBufferImpl *This = (IDirectSoundCaptureBufferImpl *)iface;
    TRACE( "(%p,0x%08lx)\n", This, dwFlags );

    if (This == NULL) {
        WARN("invalid parameter: This == NULL\n");
        return DSERR_INVALIDPARAM;
    }

    if (This->dsound == NULL) {
        WARN("invalid parameter: This->dsound == NULL\n");
        return DSERR_INVALIDPARAM;
    }

    if ( (This->dsound->device->driver == 0) && (This->dsound->device->hwi == 0) ) {
        WARN("no driver\n");
        return DSERR_NODRIVER;
    }

    EnterCriticalSection(&(This->dsound->device->lock));

    This->flags = dwFlags;
    TRACE("old This->dsound->state=%s\n",captureStateString[This->dsound->device->state]);
    if (This->dsound->device->state == STATE_STOPPED)
        This->dsound->device->state = STATE_STARTING;
    else if (This->dsound->device->state == STATE_STOPPING)
        This->dsound->device->state = STATE_CAPTURING;
    TRACE("new This->dsound->device->state=%s\n",captureStateString[This->dsound->device->state]);

    LeaveCriticalSection(&(This->dsound->device->lock));

    if (This->dsound->device->driver) {
        hres = IDsCaptureDriverBuffer_Start(This->dsound->device->hwbuf, dwFlags);
	if (hres != DS_OK)
	    WARN("IDsCaptureDriverBuffer_Start failed\n");
    } else if (This->dsound->device->hwi) {
        IDirectSoundCaptureImpl* ipDSC = This->dsound;

        if (ipDSC->device->buffer) {
            if (This->nrofnotifies) {
            	int c;

		ipDSC->device->nrofpwaves = This->nrofnotifies;
		TRACE("nrofnotifies=%d\n", This->nrofnotifies);

                /* prepare headers */
		if (ipDSC->device->pwave)
                    ipDSC->device->pwave = HeapReAlloc(GetProcessHeap(),0,ipDSC->device->pwave,
	                ipDSC->device->nrofpwaves*sizeof(WAVEHDR));
		else
                    ipDSC->device->pwave = HeapAlloc(GetProcessHeap(),0,
	                ipDSC->device->nrofpwaves*sizeof(WAVEHDR));

                for (c = 0; c < ipDSC->device->nrofpwaves; c++) {
                    if (This->notifies[c].dwOffset == DSBPN_OFFSETSTOP) {
                        TRACE("got DSBPN_OFFSETSTOP\n");
                        ipDSC->device->nrofpwaves = c;
                        break;
                    }
                    if (c == 0) {
                        ipDSC->device->pwave[0].lpData = (LPSTR)ipDSC->device->buffer;
                        ipDSC->device->pwave[0].dwBufferLength =
                            This->notifies[0].dwOffset + 1;
                    } else {
                        ipDSC->device->pwave[c].lpData = (LPSTR)ipDSC->device->buffer +
                            This->notifies[c-1].dwOffset + 1;
                        ipDSC->device->pwave[c].dwBufferLength =
                            This->notifies[c].dwOffset -
                            This->notifies[c-1].dwOffset;
                    }
                    ipDSC->device->pwave[c].dwBytesRecorded = 0;
                    ipDSC->device->pwave[c].dwUser = (DWORD)ipDSC;
                    ipDSC->device->pwave[c].dwFlags = 0;
                    ipDSC->device->pwave[c].dwLoops = 0;
                    hres = mmErr(waveInPrepareHeader(ipDSC->device->hwi,
                        &(ipDSC->device->pwave[c]),sizeof(WAVEHDR)));
		    if (hres != DS_OK) {
                        WARN("waveInPrepareHeader failed\n");
			while (c--)
			    waveInUnprepareHeader(ipDSC->device->hwi,
				&(ipDSC->device->pwave[c]),sizeof(WAVEHDR));
			break;
		    }

	            hres = mmErr(waveInAddBuffer(ipDSC->device->hwi,
			&(ipDSC->device->pwave[c]), sizeof(WAVEHDR)));
		    if (hres != DS_OK) {
                        WARN("waveInAddBuffer failed\n");
                        while (c--)
			    waveInUnprepareHeader(ipDSC->device->hwi,
				&(ipDSC->device->pwave[c]),sizeof(WAVEHDR));
			break;
		    }
                }

                FillMemory(ipDSC->device->buffer, ipDSC->device->buflen,
                    (ipDSC->device->pwfx->wBitsPerSample == 8) ? 128 : 0);
            } else {
		TRACE("no notifiers specified\n");
		/* no notifiers specified so just create a single default header */
		ipDSC->device->nrofpwaves = 1;
		if (ipDSC->device->pwave)
                    ipDSC->device->pwave = HeapReAlloc(GetProcessHeap(),0,ipDSC->device->pwave,sizeof(WAVEHDR));
		else
                    ipDSC->device->pwave = HeapAlloc(GetProcessHeap(),0,sizeof(WAVEHDR));

                ipDSC->device->pwave[0].lpData = (LPSTR)ipDSC->device->buffer;
                ipDSC->device->pwave[0].dwBufferLength = ipDSC->device->buflen;
                ipDSC->device->pwave[0].dwBytesRecorded = 0;
                ipDSC->device->pwave[0].dwUser = (DWORD)ipDSC;
                ipDSC->device->pwave[0].dwFlags = 0;
                ipDSC->device->pwave[0].dwLoops = 0;

                hres = mmErr(waveInPrepareHeader(ipDSC->device->hwi,
                    &(ipDSC->device->pwave[0]),sizeof(WAVEHDR)));
                if (hres != DS_OK) {
		    WARN("waveInPrepareHeader failed\n");
                    waveInUnprepareHeader(ipDSC->device->hwi,
                        &(ipDSC->device->pwave[0]),sizeof(WAVEHDR));
		}
	        hres = mmErr(waveInAddBuffer(ipDSC->device->hwi,
		    &(ipDSC->device->pwave[0]), sizeof(WAVEHDR)));
		if (hres != DS_OK) {
		    WARN("waveInAddBuffer failed\n");
	    	    waveInUnprepareHeader(ipDSC->device->hwi,
			&(ipDSC->device->pwave[0]),sizeof(WAVEHDR));
		}
	    }
        }

        ipDSC->device->index = 0;
        ipDSC->device->read_position = 0;

	if (hres == DS_OK) {
	    /* start filling the first buffer */
	    hres = mmErr(waveInStart(ipDSC->device->hwi));
            if (hres != DS_OK)
                WARN("waveInStart failed\n");
        }

        if (hres != DS_OK) {
            WARN("calling waveInClose because of error\n");
            waveInClose(This->dsound->device->hwi);
            This->dsound->device->hwi = 0;
        }
    } else {
        WARN("no driver\n");
        hres = DSERR_NODRIVER;
    }

    TRACE("returning %08lx\n", hres);
    return hres;
}

static HRESULT WINAPI
IDirectSoundCaptureBufferImpl_Stop( LPDIRECTSOUNDCAPTUREBUFFER8 iface )
{
    HRESULT hres = DS_OK;
    IDirectSoundCaptureBufferImpl *This = (IDirectSoundCaptureBufferImpl *)iface;
    TRACE( "(%p)\n", This );

    if (This == NULL) {
        WARN("invalid parameter: This == NULL\n");
        return DSERR_INVALIDPARAM;
    }

    if (This->dsound == NULL) {
        WARN("invalid parameter: This->dsound == NULL\n");
        return DSERR_INVALIDPARAM;
    }

    EnterCriticalSection(&(This->dsound->device->lock));

    TRACE("old This->dsound->state=%s\n",captureStateString[This->dsound->device->state]);
    if (This->dsound->device->state == STATE_CAPTURING)
	This->dsound->device->state = STATE_STOPPING;
    else if (This->dsound->device->state == STATE_STARTING)
	This->dsound->device->state = STATE_STOPPED;
    TRACE("new This->dsound->device->state=%s\n",captureStateString[This->dsound->device->state]);

    LeaveCriticalSection(&(This->dsound->device->lock));

    if (This->dsound->device->driver) {
        hres = IDsCaptureDriverBuffer_Stop(This->dsound->device->hwbuf);
        if (hres != DS_OK)
            WARN("IDsCaptureDriverBuffer_Stop() failed\n");
    } else if (This->dsound->device->hwi) {
        hres = mmErr(waveInReset(This->dsound->device->hwi));
        if (hres != DS_OK)
            WARN("waveInReset() failed\n");
    } else {
	WARN("no driver\n");
        hres = DSERR_NODRIVER;
    }

    TRACE("returning %08lx\n", hres);
    return hres;
}

static HRESULT WINAPI
IDirectSoundCaptureBufferImpl_Unlock(
    LPDIRECTSOUNDCAPTUREBUFFER8 iface,
    LPVOID lpvAudioPtr1,
    DWORD dwAudioBytes1,
    LPVOID lpvAudioPtr2,
    DWORD dwAudioBytes2 )
{
    HRESULT hres = DS_OK;
    IDirectSoundCaptureBufferImpl *This = (IDirectSoundCaptureBufferImpl *)iface;
    TRACE( "(%p,%p,%08lu,%p,%08lu)\n", This, lpvAudioPtr1, dwAudioBytes1,
        lpvAudioPtr2, dwAudioBytes2 );

    if (This == NULL) {
        WARN("invalid parameter: This == NULL\n");
        return DSERR_INVALIDPARAM;
    }

    if (lpvAudioPtr1 == NULL) {
        WARN("invalid parameter: lpvAudioPtr1 == NULL\n");
        return DSERR_INVALIDPARAM;
    }

    if (This->dsound->device->driver) {
        hres = IDsCaptureDriverBuffer_Unlock(This->dsound->device->hwbuf, lpvAudioPtr1,
                                             dwAudioBytes1, lpvAudioPtr2, dwAudioBytes2);
	if (hres != DS_OK)
	    WARN("IDsCaptureDriverBuffer_Unlock failed\n");
    } else if (This->dsound->device->hwi) {
        This->dsound->device->read_position = (This->dsound->device->read_position +
            (dwAudioBytes1 + dwAudioBytes2)) % This->dsound->device->buflen;
    } else {
        WARN("invalid call\n");
        hres = DSERR_INVALIDCALL;
    }

    TRACE("returning %08lx\n", hres);
    return hres;
}

static HRESULT WINAPI
IDirectSoundCaptureBufferImpl_GetObjectInPath(
    LPDIRECTSOUNDCAPTUREBUFFER8 iface,
    REFGUID rguidObject,
    DWORD dwIndex,
    REFGUID rguidInterface,
    LPVOID* ppObject )
{
    IDirectSoundCaptureBufferImpl *This = (IDirectSoundCaptureBufferImpl *)iface;

    FIXME( "(%p,%s,%lu,%s,%p): stub\n", This, debugstr_guid(rguidObject),
        dwIndex, debugstr_guid(rguidInterface), ppObject );

    return DS_OK;
}

static HRESULT WINAPI
IDirectSoundCaptureBufferImpl_GetFXStatus(
    LPDIRECTSOUNDCAPTUREBUFFER8 iface,
    DWORD dwFXCount,
    LPDWORD pdwFXStatus )
{
    IDirectSoundCaptureBufferImpl *This = (IDirectSoundCaptureBufferImpl *)iface;

    FIXME( "(%p,%lu,%p): stub\n", This, dwFXCount, pdwFXStatus );

    return DS_OK;
}

static const IDirectSoundCaptureBuffer8Vtbl dscbvt =
{
    /* IUnknown methods */
    IDirectSoundCaptureBufferImpl_QueryInterface,
    IDirectSoundCaptureBufferImpl_AddRef,
    IDirectSoundCaptureBufferImpl_Release,

    /* IDirectSoundCaptureBuffer methods */
    IDirectSoundCaptureBufferImpl_GetCaps,
    IDirectSoundCaptureBufferImpl_GetCurrentPosition,
    IDirectSoundCaptureBufferImpl_GetFormat,
    IDirectSoundCaptureBufferImpl_GetStatus,
    IDirectSoundCaptureBufferImpl_Initialize,
    IDirectSoundCaptureBufferImpl_Lock,
    IDirectSoundCaptureBufferImpl_Start,
    IDirectSoundCaptureBufferImpl_Stop,
    IDirectSoundCaptureBufferImpl_Unlock,

    /* IDirectSoundCaptureBuffer methods */
    IDirectSoundCaptureBufferImpl_GetObjectInPath,
    IDirectSoundCaptureBufferImpl_GetFXStatus
};

/*******************************************************************************
 * DirectSoundCapture ClassFactory
 */

static HRESULT WINAPI
DSCCF_QueryInterface(LPCLASSFACTORY iface,REFIID riid,LPVOID *ppobj)
{
    IClassFactoryImpl *This = (IClassFactoryImpl *)iface;

    FIXME("(%p)->(%s,%p),stub!\n",This,debugstr_guid(riid),ppobj);
    return E_NOINTERFACE;
}

static ULONG WINAPI
DSCCF_AddRef(LPCLASSFACTORY iface)
{
    IClassFactoryImpl *This = (IClassFactoryImpl *)iface;
    ULONG ref = InterlockedIncrement(&(This->ref));
    TRACE("(%p) ref was %ld\n", This, ref - 1);
    return ref;
}

static ULONG WINAPI
DSCCF_Release(LPCLASSFACTORY iface)
{
    IClassFactoryImpl *This = (IClassFactoryImpl *)iface;
    ULONG ref = InterlockedDecrement(&(This->ref));
    TRACE("(%p) ref was %ld\n", This, ref + 1);
    /* static class, won't be  freed */
    return ref;
}

static HRESULT WINAPI
DSCCF_CreateInstance(
	LPCLASSFACTORY iface,LPUNKNOWN pOuter,REFIID riid,LPVOID *ppobj )
{
    IClassFactoryImpl *This = (IClassFactoryImpl *)iface;
    TRACE("(%p)->(%p,%s,%p)\n",This,pOuter,debugstr_guid(riid),ppobj);

    if (pOuter) {
        WARN("aggregation not supported\n");
        return CLASS_E_NOAGGREGATION;
    }

    if (ppobj == NULL) {
	WARN("invalid parameter\n");
	return E_INVALIDARG;
    }

    *ppobj = NULL;

    if ( IsEqualGUID( &IID_IDirectSoundCapture, riid ) )
	return DSOUND_CaptureCreate8((LPDIRECTSOUNDCAPTURE*)ppobj,pOuter);

    WARN("(%p,%p,%s,%p) Interface not found!\n",This,pOuter,debugstr_guid(riid),ppobj);
    return E_NOINTERFACE;
}

static HRESULT WINAPI
DSCCF_LockServer(LPCLASSFACTORY iface,BOOL dolock)
{
    IClassFactoryImpl *This = (IClassFactoryImpl *)iface;
    FIXME("(%p)->(%d),stub!\n",This,dolock);
    return S_OK;
}

static const IClassFactoryVtbl DSCCF_Vtbl =
{
    DSCCF_QueryInterface,
    DSCCF_AddRef,
    DSCCF_Release,
    DSCCF_CreateInstance,
    DSCCF_LockServer
};

IClassFactoryImpl DSOUND_CAPTURE_CF = { &DSCCF_Vtbl, 1 };
