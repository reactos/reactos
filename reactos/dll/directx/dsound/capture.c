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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
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
#include "winuser.h"
#include "mmsystem.h"
#include "mmddk.h"
#include "winternl.h"
#include "winnls.h"
#include "wine/debug.h"
#include "dsound.h"
#include "dsdriver.h"
#include "dsound_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(dsound);

/*****************************************************************************
 * IDirectSoundCapture implementation structure
 */
struct IDirectSoundCaptureImpl
{
    /* IUnknown fields */
    const IDirectSoundCaptureVtbl     *lpVtbl;
    LONG                               ref;

    DirectSoundCaptureDevice          *device;
};

static HRESULT IDirectSoundCaptureImpl_Create(LPDIRECTSOUNDCAPTURE8 * ppds);


/*****************************************************************************
 * IDirectSoundCaptureNotify implementation structure
 */
struct IDirectSoundCaptureNotifyImpl
{
    /* IUnknown fields */
    const IDirectSoundNotifyVtbl       *lpVtbl;
    LONG                                ref;
    IDirectSoundCaptureBufferImpl*      dscb;
};

static HRESULT IDirectSoundCaptureNotifyImpl_Create(IDirectSoundCaptureBufferImpl *dscb,
                                                    IDirectSoundCaptureNotifyImpl ** pdscn);


DirectSoundCaptureDevice * DSOUND_capture[MAXWAVEDRIVERS];

static HRESULT DirectSoundCaptureDevice_Create(DirectSoundCaptureDevice ** ppDevice);

static const char * const captureStateString[] = {
    "STATE_STOPPED",
    "STATE_STARTING",
    "STATE_CAPTURING",
    "STATE_STOPPING"
};

HRESULT DSOUND_CaptureCreate(
    REFIID riid,
    LPDIRECTSOUNDCAPTURE *ppDSC)
{
    LPDIRECTSOUNDCAPTURE pDSC;
    HRESULT hr;
    TRACE("(%s, %p)\n", debugstr_guid(riid), ppDSC);

    if (!IsEqualIID(riid, &IID_IUnknown) &&
        !IsEqualIID(riid, &IID_IDirectSoundCapture)) {
        *ppDSC = 0;
        return E_NOINTERFACE;
    }

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

HRESULT DSOUND_CaptureCreate8(
    REFIID riid,
    LPDIRECTSOUNDCAPTURE8 *ppDSC8)
{
    LPDIRECTSOUNDCAPTURE8 pDSC8;
    HRESULT hr;
    TRACE("(%s, %p)\n", debugstr_guid(riid), ppDSC8);

    if (!IsEqualIID(riid, &IID_IUnknown) &&
        !IsEqualIID(riid, &IID_IDirectSoundCapture8)) {
        *ppDSC8 = 0;
        return E_NOINTERFACE;
    }

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

    hr = DSOUND_CaptureCreate(&IID_IDirectSoundCapture, &pDSC);
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

    hr = DSOUND_CaptureCreate8(&IID_IDirectSoundCapture8, &pDSC8);
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
                    err = mmErr(waveInMessage(UlongToHandle(wid),DRV_QUERYDSOUNDDESC,(DWORD_PTR)&desc,0));
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
        err = mmErr(waveInMessage(UlongToHandle(wid),DRV_QUERYDSOUNDDESC,(DWORD_PTR)&desc,0));
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
                    err = mmErr(waveInMessage(UlongToHandle(wid),DRV_QUERYDSOUNDDESC,(DWORD_PTR)&desc,0));
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
        err = mmErr(waveInMessage(UlongToHandle(wid),DRV_QUERYDSOUNDDESC,(DWORD_PTR)&desc,0));
	if (err == DS_OK) {
            TRACE("calling lpDSEnumCallback(%s,\"%s\",\"%s\",%p)\n",
                  debugstr_guid(&DSOUND_capture_guids[wid]),desc.szDesc,desc.szDrvname,lpContext);
            MultiByteToWideChar( CP_ACP, 0, desc.szDesc, -1,
                                 wDesc, sizeof(wDesc)/sizeof(WCHAR) );
            MultiByteToWideChar( CP_ACP, 0, desc.szDrvname, -1,
                                 wName, sizeof(wName)/sizeof(WCHAR) );
            if (lpDSEnumCallback(&DSOUND_capture_guids[wid], wDesc, wName, lpContext) == FALSE)
                return DS_OK;
	}
    }

    return DS_OK;
}

static void capture_CheckNotify(IDirectSoundCaptureBufferImpl *This, DWORD from, DWORD len)
{
    int i;
    for (i = 0; i < This->nrofnotifies; ++i) {
        LPDSBPOSITIONNOTIFY event = This->notifies + i;
        DWORD offset = event->dwOffset;
        TRACE("checking %d, position %d, event = %p\n", i, offset, event->hEventNotify);

        if (offset == DSBPN_OFFSETSTOP) {
            if (!from && !len) {
                SetEvent(event->hEventNotify);
                TRACE("signalled event %p (%d)\n", event->hEventNotify, i);
                return;
            }
            else return;
        }

        if (offset >= from && offset < (from + len))
        {
            TRACE("signalled event %p (%d)\n", event->hEventNotify, i);
            SetEvent(event->hEventNotify);
        }
    }
}

static void CALLBACK
DSOUND_capture_callback(HWAVEIN hwi, UINT msg, DWORD_PTR dwUser, DWORD_PTR dw1,
                        DWORD_PTR dw2)
{
    DirectSoundCaptureDevice * This = (DirectSoundCaptureDevice*)dwUser;
    IDirectSoundCaptureBufferImpl * Moi = This->capture_buffer;
    TRACE("(%p,%08x(%s),%08lx,%08lx,%08lx) entering at %d\n",hwi,msg,
	msg == MM_WIM_OPEN ? "MM_WIM_OPEN" : msg == MM_WIM_CLOSE ? "MM_WIM_CLOSE" :
	msg == MM_WIM_DATA ? "MM_WIM_DATA" : "UNKNOWN",dwUser,dw1,dw2,GetTickCount());

    if (msg == MM_WIM_DATA) {
    	EnterCriticalSection( &(This->lock) );
	TRACE("DirectSoundCapture msg=MM_WIM_DATA, old This->state=%s, old This->index=%d\n",
	    captureStateString[This->state],This->index);
	if (This->state != STATE_STOPPED) {
	    int index = This->index;
	    if (This->state == STATE_STARTING)
		This->state = STATE_CAPTURING;
	    capture_CheckNotify(Moi, (DWORD_PTR)This->pwave[index].lpData - (DWORD_PTR)This->buffer, This->pwave[index].dwBufferLength);
	    This->index = (++This->index) % This->nrofpwaves;
	    if ( (This->index == 0) && !(This->capture_buffer->flags & DSCBSTART_LOOPING) ) {
		TRACE("end of buffer\n");
		This->state = STATE_STOPPED;
		capture_CheckNotify(Moi, 0, 0);
	    } else {
		if (This->state == STATE_CAPTURING) {
		    waveInUnprepareHeader(hwi, &(This->pwave[index]), sizeof(WAVEHDR));
		    waveInPrepareHeader(hwi, &(This->pwave[index]), sizeof(WAVEHDR));
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

/***************************************************************************
 * IDirectSoundCaptureImpl
 */
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
    TRACE("(%p) ref was %d\n", This, ref - 1);
    return ref;
}

static ULONG WINAPI
IDirectSoundCaptureImpl_Release( LPDIRECTSOUNDCAPTURE iface )
{
    IDirectSoundCaptureImpl *This = (IDirectSoundCaptureImpl *)iface;
    ULONG ref = InterlockedDecrement(&(This->ref));
    TRACE("(%p) ref was %d\n", This, ref + 1);

    if (!ref) {
        if (This->device)
            DirectSoundCaptureDevice_Release(This->device);

        HeapFree( GetProcessHeap(), 0, This );
        TRACE("(%p) released\n", This);
    }
    return ref;
}

HRESULT WINAPI IDirectSoundCaptureImpl_CreateCaptureBuffer(
    LPDIRECTSOUNDCAPTURE iface,
    LPCDSCBUFFERDESC lpcDSCBufferDesc,
    LPDIRECTSOUNDCAPTUREBUFFER* lplpDSCaptureBuffer,
    LPUNKNOWN pUnk )
{
    HRESULT hr;
    IDirectSoundCaptureImpl *This = (IDirectSoundCaptureImpl *)iface;

    TRACE( "(%p,%p,%p,%p)\n",iface,lpcDSCBufferDesc,lplpDSCaptureBuffer,pUnk);

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

    hr = IDirectSoundCaptureBufferImpl_Create(This->device,
        (IDirectSoundCaptureBufferImpl **)lplpDSCaptureBuffer, lpcDSCBufferDesc);

    if (hr != DS_OK)
	WARN("IDirectSoundCaptureBufferImpl_Create failed\n");

    return hr;
}

HRESULT WINAPI IDirectSoundCaptureImpl_GetCaps(
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
	WARN("invalid parameter: lpDSCCaps->dwSize = %d\n", lpDSCCaps->dwSize);
	return DSERR_INVALIDPARAM;
    }

    lpDSCCaps->dwFlags = This->device->drvcaps.dwFlags;
    lpDSCCaps->dwFormats = This->device->drvcaps.dwFormats;
    lpDSCCaps->dwChannels = This->device->drvcaps.dwChannels;

    TRACE("(flags=0x%08x,format=0x%08x,channels=%d)\n",lpDSCCaps->dwFlags,
        lpDSCCaps->dwFormats, lpDSCCaps->dwChannels);

    return DS_OK;
}

HRESULT WINAPI IDirectSoundCaptureImpl_Initialize(
    LPDIRECTSOUNDCAPTURE iface,
    LPCGUID lpcGUID )
{
    IDirectSoundCaptureImpl *This = (IDirectSoundCaptureImpl *)iface;
    TRACE("(%p,%s)\n", This, debugstr_guid(lpcGUID));

    if (This->device != NULL) {
	WARN("already initialized\n");
	return DSERR_ALREADYINITIALIZED;
    }
    return DirectSoundCaptureDevice_Initialize(&This->device, lpcGUID);
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

static HRESULT IDirectSoundCaptureImpl_Create(
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
    TRACE("(%p) ref was %d\n", This, ref - 1);
    return ref;
}

static ULONG WINAPI IDirectSoundCaptureNotifyImpl_Release(LPDIRECTSOUNDNOTIFY iface)
{
    IDirectSoundCaptureNotifyImpl *This = (IDirectSoundCaptureNotifyImpl *)iface;
    ULONG ref = InterlockedDecrement(&(This->ref));
    TRACE("(%p) ref was %d\n", This, ref + 1);

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
    TRACE("(%p,0x%08x,%p)\n",This,howmuch,notify);

    if (howmuch > 0 && notify == NULL) {
	WARN("invalid parameter: notify == NULL\n");
	return DSERR_INVALIDPARAM;
    }

    if (TRACE_ON(dsound)) {
	unsigned int i;
	for (i=0;i<howmuch;i++)
            TRACE("notify at %d to %p\n",
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

static HRESULT IDirectSoundCaptureNotifyImpl_Create(
    IDirectSoundCaptureBufferImpl *dscb,
    IDirectSoundCaptureNotifyImpl **pdscn)
{
    IDirectSoundCaptureNotifyImpl * dscn;
    TRACE("(%p,%p)\n",dscb,pdscn);

    dscn = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*dscn));

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
	    IDirectSoundNotify_AddRef((LPDIRECTSOUNDNOTIFY)This->notify);
	    if (This->device->hwbuf && !This->hwnotify) {
		hres = IDsCaptureDriverBuffer_QueryInterface(This->device->hwbuf,
		    &IID_IDsDriverNotify, (LPVOID*)&(This->hwnotify));
		if (hres != DS_OK) {
		    WARN("IDsCaptureDriverBuffer_QueryInterface failed\n");
		    IDirectSoundNotify_Release((LPDIRECTSOUNDNOTIFY)This->notify);
		    *ppobj = 0;
		    return hres;
	        }
	    }

            *ppobj = This->notify;
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
    TRACE("(%p) ref was %d\n", This, ref - 1);
    return ref;
}

static ULONG WINAPI
IDirectSoundCaptureBufferImpl_Release( LPDIRECTSOUNDCAPTUREBUFFER8 iface )
{
    IDirectSoundCaptureBufferImpl *This = (IDirectSoundCaptureBufferImpl *)iface;
    ULONG ref = InterlockedDecrement(&(This->ref));
    TRACE("(%p) ref was %d\n", This, ref + 1);

    if (!ref) {
        TRACE("deleting object\n");
	if (This->device->state == STATE_CAPTURING)
	    This->device->state = STATE_STOPPING;

        HeapFree(GetProcessHeap(),0, This->pdscbd);

	if (This->device->hwi) {
	    waveInReset(This->device->hwi);
	    waveInClose(This->device->hwi);
            HeapFree(GetProcessHeap(),0, This->device->pwave);
            This->device->pwave = 0;
	    This->device->hwi = 0;
	}

	if (This->device->hwbuf)
	    IDsCaptureDriverBuffer_Release(This->device->hwbuf);

        /* remove from DirectSoundCaptureDevice */
        This->device->capture_buffer = NULL;

        if (This->notify)
	    IDirectSoundNotify_Release((LPDIRECTSOUNDNOTIFY)This->notify);

        /* If driver manages its own buffer, IDsCaptureDriverBuffer_Release
           should have freed the buffer. Prevent freeing it again in
           IDirectSoundCaptureBufferImpl_Create */
        if (!(This->device->drvdesc.dwFlags & DSDDESC_USESYSTEMMEMORY))
	    This->device->buffer = NULL;

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

    if (lpDSCBCaps == NULL) {
        WARN("invalid parameter: lpDSCBCaps == NULL\n");
        return DSERR_INVALIDPARAM;
    }

    if (lpDSCBCaps->dwSize < sizeof(DSCBCAPS)) {
        WARN("invalid parameter: lpDSCBCaps->dwSize = %d\n", lpDSCBCaps->dwSize);
        return DSERR_INVALIDPARAM;
    }

    if (This->device == NULL) {
        WARN("invalid parameter: This->device == NULL\n");
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

    if (This->device == NULL) {
        WARN("invalid parameter: This->device == NULL\n");
        return DSERR_INVALIDPARAM;
    }

    if (This->device->driver) {
        hres = IDsCaptureDriverBuffer_GetPosition(This->device->hwbuf, lpdwCapturePosition, lpdwReadPosition );
	if (hres != DS_OK)
	    WARN("IDsCaptureDriverBuffer_GetPosition failed\n");
    } else if (This->device->hwi) {
        DWORD pos;

        EnterCriticalSection(&This->device->lock);
        pos = (DWORD_PTR)This->device->pwave[This->device->index].lpData - (DWORD_PTR)This->device->buffer;
        if (lpdwCapturePosition)
            *lpdwCapturePosition = (This->device->pwave[This->device->index].dwBufferLength + pos) % This->device->buflen;
        if (lpdwReadPosition)
            *lpdwReadPosition = pos;
        LeaveCriticalSection(&This->device->lock);

    } else {
        WARN("no driver\n");
        hres = DSERR_NODRIVER;
    }

    TRACE("cappos=%d readpos=%d\n", (lpdwCapturePosition?*lpdwCapturePosition:-1), (lpdwReadPosition?*lpdwReadPosition:-1));
    TRACE("returning %08x\n", hres);
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
    TRACE( "(%p,%p,0x%08x,%p)\n", This, lpwfxFormat, dwSizeAllocated,
        lpdwSizeWritten );

    if (This->device == NULL) {
        WARN("invalid parameter: This->device == NULL\n");
        return DSERR_INVALIDPARAM;
    }

    if (dwSizeAllocated > (sizeof(WAVEFORMATEX) + This->device->pwfx->cbSize))
        dwSizeAllocated = sizeof(WAVEFORMATEX) + This->device->pwfx->cbSize;

    if (lpwfxFormat) { /* NULL is valid (just want size) */
        CopyMemory(lpwfxFormat, This->device->pwfx, dwSizeAllocated);
        if (lpdwSizeWritten)
            *lpdwSizeWritten = dwSizeAllocated;
    } else {
        if (lpdwSizeWritten)
            *lpdwSizeWritten = sizeof(WAVEFORMATEX) + This->device->pwfx->cbSize;
        else {
            TRACE("invalid parameter: lpdwSizeWritten = NULL\n");
            hres = DSERR_INVALIDPARAM;
        }
    }

    TRACE("returning %08x\n", hres);
    return hres;
}

static HRESULT WINAPI
IDirectSoundCaptureBufferImpl_GetStatus(
    LPDIRECTSOUNDCAPTUREBUFFER8 iface,
    LPDWORD lpdwStatus )
{
    IDirectSoundCaptureBufferImpl *This = (IDirectSoundCaptureBufferImpl *)iface;
    TRACE( "(%p, %p), thread is %04x\n", This, lpdwStatus, GetCurrentThreadId() );

    if (This->device == NULL) {
        WARN("invalid parameter: This->device == NULL\n");
        return DSERR_INVALIDPARAM;
    }

    if (lpdwStatus == NULL) {
        WARN("invalid parameter: lpdwStatus == NULL\n");
        return DSERR_INVALIDPARAM;
    }

    *lpdwStatus = 0;
    EnterCriticalSection(&(This->device->lock));

    TRACE("old This->device->state=%s, old lpdwStatus=%08x\n",
	captureStateString[This->device->state],*lpdwStatus);
    if ((This->device->state == STATE_STARTING) ||
        (This->device->state == STATE_CAPTURING)) {
        *lpdwStatus |= DSCBSTATUS_CAPTURING;
        if (This->flags & DSCBSTART_LOOPING)
            *lpdwStatus |= DSCBSTATUS_LOOPING;
    }
    TRACE("new This->device->state=%s, new lpdwStatus=%08x\n",
	captureStateString[This->device->state],*lpdwStatus);
    LeaveCriticalSection(&(This->device->lock));

    TRACE("status=%x\n", *lpdwStatus);
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
    TRACE( "(%p,%08u,%08u,%p,%p,%p,%p,0x%08x) at %d\n", This, dwReadCusor,
        dwReadBytes, lplpvAudioPtr1, lpdwAudioBytes1, lplpvAudioPtr2,
        lpdwAudioBytes2, dwFlags, GetTickCount() );

    if (This->device == NULL) {
        WARN("invalid parameter: This->device == NULL\n");
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

    EnterCriticalSection(&(This->device->lock));

    if (This->device->driver) {
        hres = IDsCaptureDriverBuffer_Lock(This->device->hwbuf, lplpvAudioPtr1,
                                           lpdwAudioBytes1, lplpvAudioPtr2,
                                           lpdwAudioBytes2, dwReadCusor,
                                           dwReadBytes, dwFlags);
	if (hres != DS_OK)
	    WARN("IDsCaptureDriverBuffer_Lock failed\n");
    } else if (This->device->hwi) {
        *lplpvAudioPtr1 = This->device->buffer + dwReadCusor;
        if ( (dwReadCusor + dwReadBytes) > This->device->buflen) {
            *lpdwAudioBytes1 = This->device->buflen - dwReadCusor;
	    if (lplpvAudioPtr2)
            	*lplpvAudioPtr2 = This->device->buffer;
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

    LeaveCriticalSection(&(This->device->lock));

    TRACE("returning %08x\n", hres);
    return hres;
}

static HRESULT WINAPI
IDirectSoundCaptureBufferImpl_Start(
    LPDIRECTSOUNDCAPTUREBUFFER8 iface,
    DWORD dwFlags )
{
    HRESULT hres = DS_OK;
    IDirectSoundCaptureBufferImpl *This = (IDirectSoundCaptureBufferImpl *)iface;
    TRACE( "(%p,0x%08x)\n", This, dwFlags );

    if (This->device == NULL) {
        WARN("invalid parameter: This->device == NULL\n");
        return DSERR_INVALIDPARAM;
    }

    if ( (This->device->driver == 0) && (This->device->hwi == 0) ) {
        WARN("no driver\n");
        return DSERR_NODRIVER;
    }

    EnterCriticalSection(&(This->device->lock));

    This->flags = dwFlags;
    TRACE("old This->state=%s\n",captureStateString[This->device->state]);
    if (This->device->state == STATE_STOPPED)
        This->device->state = STATE_STARTING;
    else if (This->device->state == STATE_STOPPING)
        This->device->state = STATE_CAPTURING;
    TRACE("new This->device->state=%s\n",captureStateString[This->device->state]);

    LeaveCriticalSection(&(This->device->lock));

    if (This->device->driver) {
        hres = IDsCaptureDriverBuffer_Start(This->device->hwbuf, dwFlags);
	if (hres != DS_OK)
	    WARN("IDsCaptureDriverBuffer_Start failed\n");
    } else if (This->device->hwi) {
        DirectSoundCaptureDevice *device = This->device;

        if (device->buffer) {
            int c;
            DWORD blocksize = DSOUND_fraglen(device->pwfx->nSamplesPerSec, device->pwfx->nBlockAlign);
            device->nrofpwaves = device->buflen / blocksize + !!(device->buflen % blocksize);
            TRACE("nrofpwaves=%d\n", device->nrofpwaves);

            /* prepare headers */
            if (device->pwave)
                device->pwave = HeapReAlloc(GetProcessHeap(), 0,device->pwave, device->nrofpwaves*sizeof(WAVEHDR));
            else
                device->pwave = HeapAlloc(GetProcessHeap(), 0, device->nrofpwaves*sizeof(WAVEHDR));

            for (c = 0; c < device->nrofpwaves; ++c) {
                device->pwave[c].lpData = (char *)device->buffer + c * blocksize;
                if (c + 1 == device->nrofpwaves)
                    device->pwave[c].dwBufferLength = device->buflen - c * blocksize;
                else
                    device->pwave[c].dwBufferLength = blocksize;
                device->pwave[c].dwBytesRecorded = 0;
                device->pwave[c].dwUser = (DWORD_PTR)device;
                device->pwave[c].dwFlags = 0;
                device->pwave[c].dwLoops = 0;
                hres = mmErr(waveInPrepareHeader(device->hwi, &(device->pwave[c]),sizeof(WAVEHDR)));
                if (hres != DS_OK) {
                    WARN("waveInPrepareHeader failed\n");
                    while (c--)
                        waveInUnprepareHeader(device->hwi, &(device->pwave[c]),sizeof(WAVEHDR));
                    break;
                }

                hres = mmErr(waveInAddBuffer(device->hwi, &(device->pwave[c]), sizeof(WAVEHDR)));
                if (hres != DS_OK) {
                    WARN("waveInAddBuffer failed\n");
                    while (c--)
                        waveInUnprepareHeader(device->hwi, &(device->pwave[c]),sizeof(WAVEHDR));
                    break;
                }
            }

            FillMemory(device->buffer, device->buflen, (device->pwfx->wBitsPerSample == 8) ? 128 : 0);
        }

        device->index = 0;

	if (hres == DS_OK) {
	    /* start filling the first buffer */
	    hres = mmErr(waveInStart(device->hwi));
            if (hres != DS_OK)
                WARN("waveInStart failed\n");
        }

        if (hres != DS_OK) {
            WARN("calling waveInClose because of error\n");
            waveInClose(device->hwi);
            device->hwi = 0;
        }
    } else {
        WARN("no driver\n");
        hres = DSERR_NODRIVER;
    }

    TRACE("returning %08x\n", hres);
    return hres;
}

static HRESULT WINAPI
IDirectSoundCaptureBufferImpl_Stop( LPDIRECTSOUNDCAPTUREBUFFER8 iface )
{
    HRESULT hres = DS_OK;
    IDirectSoundCaptureBufferImpl *This = (IDirectSoundCaptureBufferImpl *)iface;
    TRACE( "(%p)\n", This );

    if (This->device == NULL) {
        WARN("invalid parameter: This->device == NULL\n");
        return DSERR_INVALIDPARAM;
    }

    EnterCriticalSection(&(This->device->lock));

    TRACE("old This->device->state=%s\n",captureStateString[This->device->state]);
    if (This->device->state == STATE_CAPTURING)
	This->device->state = STATE_STOPPING;
    else if (This->device->state == STATE_STARTING)
	This->device->state = STATE_STOPPED;
    TRACE("new This->device->state=%s\n",captureStateString[This->device->state]);

    LeaveCriticalSection(&(This->device->lock));

    if (This->device->driver) {
        hres = IDsCaptureDriverBuffer_Stop(This->device->hwbuf);
        if (hres != DS_OK)
            WARN("IDsCaptureDriverBuffer_Stop() failed\n");
    } else if (This->device->hwi) {
        hres = mmErr(waveInReset(This->device->hwi));
        if (hres != DS_OK)
            WARN("waveInReset() failed\n");
    } else {
	WARN("no driver\n");
        hres = DSERR_NODRIVER;
    }

    TRACE("returning %08x\n", hres);
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
    TRACE( "(%p,%p,%08u,%p,%08u)\n", This, lpvAudioPtr1, dwAudioBytes1,
        lpvAudioPtr2, dwAudioBytes2 );

    if (lpvAudioPtr1 == NULL) {
        WARN("invalid parameter: lpvAudioPtr1 == NULL\n");
        return DSERR_INVALIDPARAM;
    }

    if (This->device->driver) {
        hres = IDsCaptureDriverBuffer_Unlock(This->device->hwbuf, lpvAudioPtr1,
                                             dwAudioBytes1, lpvAudioPtr2, dwAudioBytes2);
	if (hres != DS_OK)
	    WARN("IDsCaptureDriverBuffer_Unlock failed\n");
    } else if (!This->device->hwi) {
        WARN("invalid call\n");
        hres = DSERR_INVALIDCALL;
    }

    TRACE("returning %08x\n", hres);
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

    FIXME( "(%p,%s,%u,%s,%p): stub\n", This, debugstr_guid(rguidObject),
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

    FIXME( "(%p,%u,%p): stub\n", This, dwFXCount, pdwFXStatus );

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

HRESULT IDirectSoundCaptureBufferImpl_Create(
    DirectSoundCaptureDevice *device,
    IDirectSoundCaptureBufferImpl ** ppobj,
    LPCDSCBUFFERDESC lpcDSCBufferDesc)
{
    LPWAVEFORMATEX  wfex;
    TRACE( "(%p,%p,%p)\n", device, ppobj, lpcDSCBufferDesc);

    if (ppobj == NULL) {
	WARN("invalid parameter: ppobj == NULL\n");
	return DSERR_INVALIDPARAM;
    }

    if (!device) {
	WARN("not initialized\n");
        *ppobj = NULL;
	return DSERR_UNINITIALIZED;
    }

    if (lpcDSCBufferDesc == NULL) {
	WARN("invalid parameter: lpcDSCBufferDesc == NULL\n");
        *ppobj = NULL;
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

    wfex = lpcDSCBufferDesc->lpwfxFormat;

    if (wfex) {
        TRACE("(formattag=0x%04x,chans=%d,samplerate=%d,"
            "bytespersec=%d,blockalign=%d,bitspersamp=%d,cbSize=%d)\n",
            wfex->wFormatTag, wfex->nChannels, wfex->nSamplesPerSec,
            wfex->nAvgBytesPerSec, wfex->nBlockAlign,
            wfex->wBitsPerSample, wfex->cbSize);

        if (wfex->wFormatTag == WAVE_FORMAT_PCM) {
	    device->pwfx = HeapAlloc(GetProcessHeap(),0,sizeof(WAVEFORMATEX));
            *device->pwfx = *wfex;
	    device->pwfx->cbSize = 0;
	} else {
	    device->pwfx = HeapAlloc(GetProcessHeap(),0,sizeof(WAVEFORMATEX)+wfex->cbSize);
            CopyMemory(device->pwfx, wfex, sizeof(WAVEFORMATEX)+wfex->cbSize);
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
        IDirectSoundCaptureBufferImpl *This = *ppobj;

        This->ref = 1;
        This->device = device;
        This->device->capture_buffer = This;
	This->notify = NULL;
	This->nrofnotifies = 0;
	This->hwnotify = NULL;

        This->pdscbd = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,
            lpcDSCBufferDesc->dwSize);
        if (This->pdscbd)
            CopyMemory(This->pdscbd, lpcDSCBufferDesc, lpcDSCBufferDesc->dwSize);
        else {
            WARN("no memory\n");
            This->device->capture_buffer = 0;
            HeapFree( GetProcessHeap(), 0, This );
            *ppobj = NULL;
            return DSERR_OUTOFMEMORY;
        }

        This->lpVtbl = &dscbvt;

	if (device->driver) {
            if (This->device->drvdesc.dwFlags & DSDDESC_DOMMSYSTEMOPEN)
                FIXME("DSDDESC_DOMMSYSTEMOPEN not supported\n");

            if (This->device->drvdesc.dwFlags & DSDDESC_USESYSTEMMEMORY) {
                /* allocate buffer from system memory */
                buflen = lpcDSCBufferDesc->dwBufferBytes;
                TRACE("desired buflen=%d, old buffer=%p\n", buflen, device->buffer);
                if (device->buffer)
                    newbuf = HeapReAlloc(GetProcessHeap(),0,device->buffer,buflen);
                else
                    newbuf = HeapAlloc(GetProcessHeap(),0,buflen);

                if (newbuf == NULL) {
                    WARN("failed to allocate capture buffer\n");
                    err = DSERR_OUTOFMEMORY;
                    /* but the old buffer might still exist and must be re-prepared */
                } else {
                    device->buffer = newbuf;
                    device->buflen = buflen;
                }
            } else {
                /* let driver allocate memory */
                device->buflen = lpcDSCBufferDesc->dwBufferBytes;
                /* FIXME: */
                HeapFree( GetProcessHeap(), 0, device->buffer);
                device->buffer = NULL;
            }

	    err = IDsCaptureDriver_CreateCaptureBuffer(device->driver,
		device->pwfx,0,0,&(device->buflen),&(device->buffer),(LPVOID*)&(device->hwbuf));
	    if (err != DS_OK) {
		WARN("IDsCaptureDriver_CreateCaptureBuffer failed\n");
		This->device->capture_buffer = 0;
		HeapFree( GetProcessHeap(), 0, This );
		*ppobj = NULL;
		return err;
	    }
	} else {
	    DWORD flags = CALLBACK_FUNCTION;
            err = mmErr(waveInOpen(&(device->hwi),
                device->drvdesc.dnDevNode, device->pwfx,
                (DWORD_PTR)DSOUND_capture_callback, (DWORD_PTR)device, flags));
            if (err != DS_OK) {
                WARN("waveInOpen failed\n");
		This->device->capture_buffer = 0;
		HeapFree( GetProcessHeap(), 0, This );
		*ppobj = NULL;
		return err;
            }

	    buflen = lpcDSCBufferDesc->dwBufferBytes;
            TRACE("desired buflen=%d, old buffer=%p\n", buflen, device->buffer);
	    if (device->buffer)
                newbuf = HeapReAlloc(GetProcessHeap(),0,device->buffer,buflen);
	    else
		newbuf = HeapAlloc(GetProcessHeap(),0,buflen);
            if (newbuf == NULL) {
                WARN("failed to allocate capture buffer\n");
                err = DSERR_OUTOFMEMORY;
                /* but the old buffer might still exist and must be re-prepared */
            } else {
                device->buffer = newbuf;
                device->buflen = buflen;
            }
	}
    }

    TRACE("returning DS_OK\n");
    return DS_OK;
}

/*******************************************************************************
 * DirectSoundCaptureDevice
 */
HRESULT DirectSoundCaptureDevice_Initialize(
    DirectSoundCaptureDevice ** ppDevice,
    LPCGUID lpcGUID)
{
    HRESULT err = DSERR_INVALIDPARAM;
    unsigned wid, widn;
    BOOLEAN found = FALSE;
    GUID devGUID;
    DirectSoundCaptureDevice *device = *ppDevice;
    TRACE("(%p, %s)\n", ppDevice, debugstr_guid(lpcGUID));

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

    *ppDevice = device;
    device->guid = devGUID;

    /* Disable the direct sound driver to force emulation if requested. */
    device->driver = NULL;
    if (ds_hw_accel != DS_HW_ACCEL_EMULATION)
    {
        err = mmErr(waveInMessage(UlongToHandle(wid),DRV_QUERYDSOUNDIFACE,(DWORD_PTR)&device->driver,0));
        if ( (err != DS_OK) && (err != DSERR_UNSUPPORTED) ) {
            WARN("waveInMessage failed; err=%x\n",err);
            return err;
        }
    }
    err = DS_OK;

    /* Get driver description */
    if (device->driver) {
        TRACE("using DirectSound driver\n");
        err = IDsCaptureDriver_GetDriverDesc(device->driver, &(device->drvdesc));
	if (err != DS_OK) {
	    WARN("IDsCaptureDriver_GetDriverDesc failed\n");
	    return err;
	}
    } else {
        TRACE("using WINMM\n");
        /* if no DirectSound interface available, use WINMM API instead */
        device->drvdesc.dwFlags = DSDDESC_DOMMSYSTEMOPEN |
            DSDDESC_DOMMSYSTEMSETFORMAT;
    }

    device->drvdesc.dnDevNode = wid;

    /* open the DirectSound driver if available */
    if (device->driver && (err == DS_OK))
        err = IDsCaptureDriver_Open(device->driver);

    if (err == DS_OK) {
        *ppDevice = device;

        /* the driver is now open, so it's now allowed to call GetCaps */
        if (device->driver) {
	    device->drvcaps.dwSize = sizeof(device->drvcaps);
            err = IDsCaptureDriver_GetCaps(device->driver,&(device->drvcaps));
	    if (err != DS_OK) {
		WARN("IDsCaptureDriver_GetCaps failed\n");
		return err;
	    }
        } else /*if (device->hwi)*/ {
            WAVEINCAPSA    wic;
            err = mmErr(waveInGetDevCapsA((UINT)device->drvdesc.dnDevNode, &wic, sizeof(wic)));

            if (err == DS_OK) {
                device->drvcaps.dwFlags = 0;
                lstrcpynA(device->drvdesc.szDrvname, wic.szPname,
                          sizeof(device->drvdesc.szDrvname));

                device->drvcaps.dwFlags |= DSCCAPS_EMULDRIVER;
                device->drvcaps.dwFormats = wic.dwFormats;
                device->drvcaps.dwChannels = wic.wChannels;
            }
        }
    }

    return err;
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
    device->lock.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": DirectSoundCaptureDevice.lock");

    *ppDevice = device;

    return DS_OK;
}

ULONG DirectSoundCaptureDevice_Release(
    DirectSoundCaptureDevice * device)
{
    ULONG ref = InterlockedDecrement(&(device->ref));
    TRACE("(%p) ref was %d\n", device, ref + 1);

    if (!ref) {
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
