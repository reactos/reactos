/*              DirectSoundFullDuplex
 *
 * Copyright 1998 Marcus Meissner
 * Copyright 1998 Rob Riggs
 * Copyright 2000-2001 TransGaming Technologies, Inc.
 * Copyright 2005 Robert Reif
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

#include "dsound_private.h"

/*****************************************************************************
 * IDirectSoundFullDuplex implementation structure
 */
typedef struct IDirectSoundFullDuplexImpl
{
    /* IUnknown fields */
    const IDirectSoundFullDuplexVtbl *lpVtbl;
    LONG                              ref;

    /* IDirectSoundFullDuplexImpl fields */
    IDirectSound8                    *renderer_device;
    IDirectSoundCapture              *capture_device;

    LPUNKNOWN                         pUnknown;
    LPDIRECTSOUND8                    pDS8;
    LPDIRECTSOUNDCAPTURE              pDSC;
} IDirectSoundFullDuplexImpl;

typedef struct IDirectSoundFullDuplex_IUnknown {
    const IUnknownVtbl         *lpVtbl;
    LONG                        ref;
    IDirectSoundFullDuplexImpl *pdsfd;
} IDirectSoundFullDuplex_IUnknown;

typedef struct IDirectSoundFullDuplex_IDirectSound8 {
    const IDirectSound8Vtbl    *lpVtbl;
    LONG                        ref;
    IDirectSoundFullDuplexImpl *pdsfd;
} IDirectSoundFullDuplex_IDirectSound8;

typedef struct IDirectSoundFullDuplex_IDirectSoundCapture {
    const IDirectSoundCaptureVtbl *lpVtbl;
    LONG                           ref;
    IDirectSoundFullDuplexImpl    *pdsfd;
} IDirectSoundFullDuplex_IDirectSoundCapture;

/*******************************************************************************
 * IUnknown
 */
static HRESULT WINAPI IDirectSoundFullDuplex_IUnknown_QueryInterface(
    LPUNKNOWN iface,
    REFIID riid,
    LPVOID * ppobj)
{
    IDirectSoundFullDuplex_IUnknown *This = (IDirectSoundFullDuplex_IUnknown *)iface;
    TRACE("(%p,%s,%p)\n",This,debugstr_guid(riid),ppobj);
    return IDirectSoundFullDuplex_QueryInterface((LPDIRECTSOUNDFULLDUPLEX)This->pdsfd, riid, ppobj);
}

static ULONG WINAPI IDirectSoundFullDuplex_IUnknown_AddRef(
    LPUNKNOWN iface)
{
    IDirectSoundFullDuplex_IUnknown *This = (IDirectSoundFullDuplex_IUnknown *)iface;
    ULONG ref = InterlockedIncrement(&(This->ref));
    TRACE("(%p) ref was %d\n", This, ref - 1);
    return ref;
}

static ULONG WINAPI IDirectSoundFullDuplex_IUnknown_Release(
    LPUNKNOWN iface)
{
    IDirectSoundFullDuplex_IUnknown *This = (IDirectSoundFullDuplex_IUnknown *)iface;
    ULONG ref = InterlockedDecrement(&(This->ref));
    TRACE("(%p) ref was %d\n", This, ref + 1);
    if (!ref) {
        This->pdsfd->pUnknown = NULL;
        HeapFree(GetProcessHeap(), 0, This);
        TRACE("(%p) released\n", This);
    }
    return ref;
}

static const IUnknownVtbl DirectSoundFullDuplex_Unknown_Vtbl =
{
    IDirectSoundFullDuplex_IUnknown_QueryInterface,
    IDirectSoundFullDuplex_IUnknown_AddRef,
    IDirectSoundFullDuplex_IUnknown_Release
};

static HRESULT IDirectSoundFullDuplex_IUnknown_Create(
    LPDIRECTSOUNDFULLDUPLEX pdsfd,
    LPUNKNOWN * ppunk)
{
    IDirectSoundFullDuplex_IUnknown * pdsfdunk;
    TRACE("(%p,%p)\n",pdsfd,ppunk);

    if (pdsfd == NULL) {
        ERR("invalid parameter: pdsfd == NULL\n");
        return DSERR_INVALIDPARAM;
    }

    if (ppunk == NULL) {
        ERR("invalid parameter: ppunk == NULL\n");
        return DSERR_INVALIDPARAM;
    }

    pdsfdunk = HeapAlloc(GetProcessHeap(),0,sizeof(*pdsfdunk));
    if (pdsfdunk == NULL) {
        WARN("out of memory\n");
        *ppunk = NULL;
        return DSERR_OUTOFMEMORY;
    }

    pdsfdunk->lpVtbl = &DirectSoundFullDuplex_Unknown_Vtbl;
    pdsfdunk->ref = 0;
    pdsfdunk->pdsfd = (IDirectSoundFullDuplexImpl *)pdsfd;

    *ppunk = (LPUNKNOWN)pdsfdunk;

    return DS_OK;
}

/*******************************************************************************
 * IDirectSoundFullDuplex_IDirectSound8
 */
static HRESULT WINAPI IDirectSoundFullDuplex_IDirectSound8_QueryInterface(
    LPDIRECTSOUND8 iface,
    REFIID riid,
    LPVOID * ppobj)
{
    IDirectSoundFullDuplex_IDirectSound8 *This = (IDirectSoundFullDuplex_IDirectSound8 *)iface;
    TRACE("(%p,%s,%p)\n",This,debugstr_guid(riid),ppobj);
    return IDirectSoundFullDuplex_QueryInterface((LPDIRECTSOUNDFULLDUPLEX)This->pdsfd, riid, ppobj);
}

static ULONG WINAPI IDirectSoundFullDuplex_IDirectSound8_AddRef(
    LPDIRECTSOUND8 iface)
{
    IDirectSoundFullDuplex_IDirectSound8 *This = (IDirectSoundFullDuplex_IDirectSound8 *)iface;
    ULONG ref = InterlockedIncrement(&(This->ref));
    TRACE("(%p) ref was %d\n", This, ref - 1);
    return ref;
}

static ULONG WINAPI IDirectSoundFullDuplex_IDirectSound8_Release(
    LPDIRECTSOUND8 iface)
{
    IDirectSoundFullDuplex_IDirectSound8 *This = (IDirectSoundFullDuplex_IDirectSound8 *)iface;
    ULONG ref = InterlockedDecrement(&(This->ref));
    TRACE("(%p) ref was %d\n", This, ref + 1);
    if (!ref) {
        This->pdsfd->pDS8 = NULL;
        HeapFree(GetProcessHeap(), 0, This);
        TRACE("(%p) released\n", This);
    }
    return ref;
}

static HRESULT WINAPI IDirectSoundFullDuplex_IDirectSound8_CreateSoundBuffer(
    LPDIRECTSOUND8 iface,
    LPCDSBUFFERDESC dsbd,
    LPLPDIRECTSOUNDBUFFER ppdsb,
    LPUNKNOWN lpunk)
{
    IDirectSoundFullDuplex_IDirectSound8 *This = (IDirectSoundFullDuplex_IDirectSound8 *)iface;
    TRACE("(%p,%p,%p,%p)\n",This,dsbd,ppdsb,lpunk);
    return IDirectSound8_CreateSoundBuffer(This->pdsfd->renderer_device,dsbd,ppdsb,lpunk);
}

static HRESULT WINAPI IDirectSoundFullDuplex_IDirectSound8_GetCaps(
    LPDIRECTSOUND8 iface,
    LPDSCAPS lpDSCaps)
{
    IDirectSoundFullDuplex_IDirectSound8 *This = (IDirectSoundFullDuplex_IDirectSound8 *)iface;
    TRACE("(%p,%p)\n",This,lpDSCaps);
    return IDirectSound8_GetCaps(This->pdsfd->renderer_device, lpDSCaps);
}

static HRESULT WINAPI IDirectSoundFullDuplex_IDirectSound8_DuplicateSoundBuffer(
    LPDIRECTSOUND8 iface,
    LPDIRECTSOUNDBUFFER psb,
    LPLPDIRECTSOUNDBUFFER ppdsb)
{
    IDirectSoundFullDuplex_IDirectSound8 *This = (IDirectSoundFullDuplex_IDirectSound8 *)iface;
    TRACE("(%p,%p,%p)\n",This,psb,ppdsb);
    return IDirectSound8_DuplicateSoundBuffer(This->pdsfd->renderer_device,psb,ppdsb);
}

static HRESULT WINAPI IDirectSoundFullDuplex_IDirectSound8_SetCooperativeLevel(
    LPDIRECTSOUND8 iface,
    HWND hwnd,
    DWORD level)
{
    IDirectSoundFullDuplex_IDirectSound8 *This = (IDirectSoundFullDuplex_IDirectSound8 *)iface;
    TRACE("(%p,%p,%s)\n",This,hwnd,dumpCooperativeLevel(level));
    return IDirectSound8_SetCooperativeLevel(This->pdsfd->renderer_device,hwnd,level);
}

static HRESULT WINAPI IDirectSoundFullDuplex_IDirectSound8_Compact(
    LPDIRECTSOUND8 iface)
{
    IDirectSoundFullDuplex_IDirectSound8 *This = (IDirectSoundFullDuplex_IDirectSound8 *)iface;
    TRACE("(%p)\n", This);
    return IDirectSound8_Compact(This->pdsfd->renderer_device);
}

static HRESULT WINAPI IDirectSoundFullDuplex_IDirectSound8_GetSpeakerConfig(
    LPDIRECTSOUND8 iface,
    LPDWORD lpdwSpeakerConfig)
{
    IDirectSoundFullDuplex_IDirectSound8 *This = (IDirectSoundFullDuplex_IDirectSound8 *)iface;
    TRACE("(%p, %p)\n", This, lpdwSpeakerConfig);
    return IDirectSound8_GetSpeakerConfig(This->pdsfd->renderer_device,lpdwSpeakerConfig);
}

static HRESULT WINAPI IDirectSoundFullDuplex_IDirectSound8_SetSpeakerConfig(
    LPDIRECTSOUND8 iface,
    DWORD config)
{
    IDirectSoundFullDuplex_IDirectSound8 *This = (IDirectSoundFullDuplex_IDirectSound8 *)iface;
    TRACE("(%p,0x%08x)\n",This,config);
    return IDirectSound8_SetSpeakerConfig(This->pdsfd->renderer_device,config);
}

static HRESULT WINAPI IDirectSoundFullDuplex_IDirectSound8_Initialize(
    LPDIRECTSOUND8 iface,
    LPCGUID lpcGuid)
{
    IDirectSoundFullDuplex_IDirectSound8 *This = (IDirectSoundFullDuplex_IDirectSound8 *)iface;
    TRACE("(%p, %s)\n", This, debugstr_guid(lpcGuid));
    return IDirectSound8_Initialize(This->pdsfd->renderer_device,lpcGuid);
}

static HRESULT WINAPI IDirectSoundFullDuplex_IDirectSound8_VerifyCertification(
    LPDIRECTSOUND8 iface,
    DWORD *cert)
{
    IDirectSoundFullDuplex_IDirectSound8 *This = (IDirectSoundFullDuplex_IDirectSound8 *)iface;
    TRACE("(%p, %p)\n", This, cert);
    return IDirectSound8_VerifyCertification(This->pdsfd->renderer_device,cert);
}

static const IDirectSound8Vtbl DirectSoundFullDuplex_DirectSound8_Vtbl =
{
    IDirectSoundFullDuplex_IDirectSound8_QueryInterface,
    IDirectSoundFullDuplex_IDirectSound8_AddRef,
    IDirectSoundFullDuplex_IDirectSound8_Release,
    IDirectSoundFullDuplex_IDirectSound8_CreateSoundBuffer,
    IDirectSoundFullDuplex_IDirectSound8_GetCaps,
    IDirectSoundFullDuplex_IDirectSound8_DuplicateSoundBuffer,
    IDirectSoundFullDuplex_IDirectSound8_SetCooperativeLevel,
    IDirectSoundFullDuplex_IDirectSound8_Compact,
    IDirectSoundFullDuplex_IDirectSound8_GetSpeakerConfig,
    IDirectSoundFullDuplex_IDirectSound8_SetSpeakerConfig,
    IDirectSoundFullDuplex_IDirectSound8_Initialize,
    IDirectSoundFullDuplex_IDirectSound8_VerifyCertification
};

static HRESULT IDirectSoundFullDuplex_IDirectSound8_Create(
    LPDIRECTSOUNDFULLDUPLEX pdsfd,
    LPDIRECTSOUND8 * ppds8)
{
    IDirectSoundFullDuplex_IDirectSound8 * pdsfdds8;
    TRACE("(%p,%p)\n",pdsfd,ppds8);

    if (pdsfd == NULL) {
        ERR("invalid parameter: pdsfd == NULL\n");
        return DSERR_INVALIDPARAM;
    }

    if (ppds8 == NULL) {
        ERR("invalid parameter: ppds8 == NULL\n");
        return DSERR_INVALIDPARAM;
    }

    if (((IDirectSoundFullDuplexImpl*)pdsfd)->renderer_device == NULL) {
        WARN("not initialized\n");
        *ppds8 = NULL;
        return DSERR_UNINITIALIZED;
    }

    pdsfdds8 = HeapAlloc(GetProcessHeap(),0,sizeof(*pdsfdds8));
    if (pdsfdds8 == NULL) {
        WARN("out of memory\n");
        *ppds8 = NULL;
        return DSERR_OUTOFMEMORY;
    }

    pdsfdds8->lpVtbl = &DirectSoundFullDuplex_DirectSound8_Vtbl;
    pdsfdds8->ref = 0;
    pdsfdds8->pdsfd = (IDirectSoundFullDuplexImpl *)pdsfd;

    *ppds8 = (LPDIRECTSOUND8)pdsfdds8;

    return DS_OK;
}

/*******************************************************************************
 * IDirectSoundFullDuplex_IDirectSoundCapture
 */
static HRESULT WINAPI IDirectSoundFullDuplex_IDirectSoundCapture_QueryInterface(
    LPDIRECTSOUNDCAPTURE iface,
    REFIID riid,
    LPVOID * ppobj)
{
    IDirectSoundFullDuplex_IDirectSoundCapture *This = (IDirectSoundFullDuplex_IDirectSoundCapture *)iface;
    TRACE("(%p,%s,%p)\n",This,debugstr_guid(riid),ppobj);
    return IDirectSoundFullDuplex_QueryInterface((LPDIRECTSOUNDFULLDUPLEX)This->pdsfd, riid, ppobj);
}

static ULONG WINAPI IDirectSoundFullDuplex_IDirectSoundCapture_AddRef(
    LPDIRECTSOUNDCAPTURE iface)
{
    IDirectSoundFullDuplex_IDirectSoundCapture *This = (IDirectSoundFullDuplex_IDirectSoundCapture *)iface;
    ULONG ref = InterlockedIncrement(&(This->ref));
    TRACE("(%p) ref was %d\n", This, ref - 1);
    return ref;
}

static ULONG WINAPI IDirectSoundFullDuplex_IDirectSoundCapture_Release(
    LPDIRECTSOUNDCAPTURE iface)
{
    IDirectSoundFullDuplex_IDirectSoundCapture *This = (IDirectSoundFullDuplex_IDirectSoundCapture *)iface;
    ULONG ref = InterlockedDecrement(&(This->ref));
    TRACE("(%p) ref was %d\n", This, ref + 1);
    if (!ref) {
        This->pdsfd->pDSC = NULL;
        HeapFree(GetProcessHeap(), 0, This);
        TRACE("(%p) released\n", This);
    }
    return ref;
}

static HRESULT WINAPI IDirectSoundFullDuplex_IDirectSoundCapture_CreateCaptureBuffer(
    LPDIRECTSOUNDCAPTURE iface,
    LPCDSCBUFFERDESC lpcDSCBufferDesc,
    LPDIRECTSOUNDCAPTUREBUFFER* lplpDSCaptureBuffer,
    LPUNKNOWN pUnk)
{
    IDirectSoundFullDuplex_IDirectSoundCapture *This = (IDirectSoundFullDuplex_IDirectSoundCapture *)iface;
    TRACE("(%p,%p,%p,%p)\n",This,lpcDSCBufferDesc,lplpDSCaptureBuffer,pUnk);
    return IDirectSoundCapture_CreateCaptureBuffer(This->pdsfd->capture_device,lpcDSCBufferDesc,lplpDSCaptureBuffer,pUnk);
}

static HRESULT WINAPI IDirectSoundFullDuplex_IDirectSoundCapture_GetCaps(
    LPDIRECTSOUNDCAPTURE iface,
    LPDSCCAPS lpDSCCaps)
{
    IDirectSoundFullDuplex_IDirectSoundCapture *This = (IDirectSoundFullDuplex_IDirectSoundCapture *)iface;
    TRACE("(%p,%p)\n",This,lpDSCCaps);
    return IDirectSoundCapture_GetCaps(This->pdsfd->capture_device, lpDSCCaps);
}

static HRESULT WINAPI IDirectSoundFullDuplex_IDirectSoundCapture_Initialize(
    LPDIRECTSOUNDCAPTURE iface,
    LPCGUID lpcGUID)
{
    IDirectSoundFullDuplex_IDirectSoundCapture *This = (IDirectSoundFullDuplex_IDirectSoundCapture *)iface;
    TRACE("(%p, %s)\n", This, debugstr_guid(lpcGUID));
    return IDirectSoundCapture_Initialize(This->pdsfd->capture_device,lpcGUID);
}

static const IDirectSoundCaptureVtbl DirectSoundFullDuplex_DirectSoundCapture_Vtbl =
{
    IDirectSoundFullDuplex_IDirectSoundCapture_QueryInterface,
    IDirectSoundFullDuplex_IDirectSoundCapture_AddRef,
    IDirectSoundFullDuplex_IDirectSoundCapture_Release,
    IDirectSoundFullDuplex_IDirectSoundCapture_CreateCaptureBuffer,
    IDirectSoundFullDuplex_IDirectSoundCapture_GetCaps,
    IDirectSoundFullDuplex_IDirectSoundCapture_Initialize
};

static HRESULT IDirectSoundFullDuplex_IDirectSoundCapture_Create(
    LPDIRECTSOUNDFULLDUPLEX pdsfd,
    LPDIRECTSOUNDCAPTURE8 * ppdsc8)
{
    IDirectSoundFullDuplex_IDirectSoundCapture * pdsfddsc;
    TRACE("(%p,%p)\n",pdsfd,ppdsc8);

    if (pdsfd == NULL) {
        ERR("invalid parameter: pdsfd == NULL\n");
        return DSERR_INVALIDPARAM;
    }

    if (ppdsc8 == NULL) {
        ERR("invalid parameter: ppdsc8 == NULL\n");
        return DSERR_INVALIDPARAM;
    }

    if (((IDirectSoundFullDuplexImpl*)pdsfd)->capture_device == NULL) {
        WARN("not initialized\n");
        *ppdsc8 = NULL;
        return DSERR_UNINITIALIZED;
    }

    pdsfddsc = HeapAlloc(GetProcessHeap(),0,sizeof(*pdsfddsc));
    if (pdsfddsc == NULL) {
        WARN("out of memory\n");
        *ppdsc8 = NULL;
        return DSERR_OUTOFMEMORY;
    }

    pdsfddsc->lpVtbl = &DirectSoundFullDuplex_DirectSoundCapture_Vtbl;
    pdsfddsc->ref = 0;
    pdsfddsc->pdsfd = (IDirectSoundFullDuplexImpl *)pdsfd;

    *ppdsc8 = (LPDIRECTSOUNDCAPTURE)pdsfddsc;

    return DS_OK;
}

/***************************************************************************
 * IDirectSoundFullDuplexImpl
 */
static ULONG WINAPI
IDirectSoundFullDuplexImpl_AddRef( LPDIRECTSOUNDFULLDUPLEX iface )
{
    IDirectSoundFullDuplexImpl *This = (IDirectSoundFullDuplexImpl *)iface;
    ULONG ref = InterlockedIncrement(&(This->ref));
    TRACE("(%p) ref was %d\n", This, ref - 1);
    return ref;
}

static HRESULT WINAPI
IDirectSoundFullDuplexImpl_QueryInterface(
    LPDIRECTSOUNDFULLDUPLEX iface,
    REFIID riid,
    LPVOID* ppobj )
{
    IDirectSoundFullDuplexImpl *This = (IDirectSoundFullDuplexImpl *)iface;
    TRACE( "(%p,%s,%p)\n", This, debugstr_guid(riid), ppobj );

    if (ppobj == NULL) {
	WARN("invalid parameter\n");
	return E_INVALIDARG;
    }

    *ppobj = NULL;

    if (IsEqualIID(riid, &IID_IUnknown)) {
        if (!This->pUnknown) {
            IDirectSoundFullDuplex_IUnknown_Create(iface, &This->pUnknown);
            if (!This->pUnknown) {
                WARN("IDirectSoundFullDuplex_IUnknown_Create() failed\n");
                *ppobj = NULL;
                return E_NOINTERFACE;
            }
        }
        IDirectSoundFullDuplex_IUnknown_AddRef(This->pUnknown);
        *ppobj = This->pUnknown;
        return S_OK;
    } else if (IsEqualIID(riid, &IID_IDirectSoundFullDuplex)) {
        IDirectSoundFullDuplexImpl_AddRef(iface);
        *ppobj = This;
        return S_OK;
    } else if (IsEqualIID(riid, &IID_IDirectSound)
               || IsEqualIID(riid, &IID_IDirectSound8)) {
        if (!This->pDS8) {
            IDirectSoundFullDuplex_IDirectSound8_Create(iface, &This->pDS8);
            if (!This->pDS8) {
                WARN("IDirectSoundFullDuplex_IDirectSound8_Create() failed\n");
                *ppobj = NULL;
                return E_NOINTERFACE;
            }
        }
        IDirectSoundFullDuplex_IDirectSound8_AddRef(This->pDS8);
        *ppobj = This->pDS8;
        return S_OK;
    } else if (IsEqualIID(riid, &IID_IDirectSoundCapture)) {
        if (!This->pDSC) {
            IDirectSoundFullDuplex_IDirectSoundCapture_Create(iface, &This->pDSC);
            if (!This->pDSC) {
                WARN("IDirectSoundFullDuplex_IDirectSoundCapture_Create() failed\n");
                *ppobj = NULL;
                return E_NOINTERFACE;
            }
        }
        IDirectSoundFullDuplex_IDirectSoundCapture_AddRef(This->pDSC);
        *ppobj = This->pDSC;
        return S_OK;
    }

    return E_NOINTERFACE;
}

static ULONG WINAPI
IDirectSoundFullDuplexImpl_Release( LPDIRECTSOUNDFULLDUPLEX iface )
{
    IDirectSoundFullDuplexImpl *This = (IDirectSoundFullDuplexImpl *)iface;
    ULONG ref = InterlockedDecrement(&(This->ref));
    TRACE("(%p) ref was %d\n", This, ref - 1);

    if (!ref) {
        if (This->capture_device)
            IDirectSoundCapture_Release(This->capture_device);
        if (This->renderer_device)
            IDirectSound_Release(This->renderer_device);
        HeapFree( GetProcessHeap(), 0, This );
	TRACE("(%p) released\n", This);
    }
    return ref;
}

static HRESULT WINAPI
IDirectSoundFullDuplexImpl_Initialize(
    LPDIRECTSOUNDFULLDUPLEX iface,
    LPCGUID pCaptureGuid,
    LPCGUID pRendererGuid,
    LPCDSCBUFFERDESC lpDscBufferDesc,
    LPCDSBUFFERDESC lpDsBufferDesc,
    HWND hWnd,
    DWORD dwLevel,
    LPLPDIRECTSOUNDCAPTUREBUFFER8 lplpDirectSoundCaptureBuffer8,
    LPLPDIRECTSOUNDBUFFER8 lplpDirectSoundBuffer8 )
{
    HRESULT hr;
    IDirectSoundFullDuplexImpl *This = (IDirectSoundFullDuplexImpl *)iface;

    TRACE("(%p,%s,%s,%p,%p,%p,%x,%p,%p)\n", This,
        debugstr_guid(pCaptureGuid), debugstr_guid(pRendererGuid),
        lpDscBufferDesc, lpDsBufferDesc, hWnd, dwLevel,
        lplpDirectSoundCaptureBuffer8, lplpDirectSoundBuffer8);

    if (!lplpDirectSoundCaptureBuffer8 || !lplpDirectSoundBuffer8)
        return E_INVALIDARG;

    if (This->renderer_device != NULL || This->capture_device != NULL) {
        WARN("already initialized\n");
        *lplpDirectSoundCaptureBuffer8 = NULL;
        *lplpDirectSoundBuffer8 = NULL;
        return DSERR_ALREADYINITIALIZED;
    }

    hr = DSOUND_Create8(&IID_IDirectSound8, &This->renderer_device);
    if (SUCCEEDED(hr))
        hr = IDirectSound_Initialize(This->renderer_device, pRendererGuid);
    if (hr != DS_OK) {
        WARN("DirectSoundDevice_Initialize() failed\n");
        *lplpDirectSoundCaptureBuffer8 = NULL;
        *lplpDirectSoundBuffer8 = NULL;
        return hr;
    }

    IDirectSound8_SetCooperativeLevel(This->renderer_device, hWnd, dwLevel);

    hr = IDirectSound8_CreateSoundBuffer(This->renderer_device, lpDsBufferDesc,
        (IDirectSoundBuffer**)lplpDirectSoundBuffer8, NULL);
    if (hr != DS_OK) {
        WARN("IDirectSoundBufferImpl_Create() failed\n");
        *lplpDirectSoundCaptureBuffer8 = NULL;
        *lplpDirectSoundBuffer8 = NULL;
        return hr;
    }

    hr = DSOUND_CaptureCreate8(&IID_IDirectSoundCapture8, &This->capture_device);
    if (SUCCEEDED(hr))
        hr = IDirectSoundCapture_Initialize(This->capture_device, pCaptureGuid);
    if (hr != DS_OK) {
        WARN("DirectSoundCaptureDevice_Initialize() failed\n");
        *lplpDirectSoundCaptureBuffer8 = NULL;
        *lplpDirectSoundBuffer8 = NULL;
        return hr;
    }

    hr = IDirectSoundCapture_CreateCaptureBuffer(This->capture_device,
        lpDscBufferDesc,
        (IDirectSoundCaptureBuffer**)lplpDirectSoundCaptureBuffer8,
        NULL);
    if (hr != DS_OK) {
        WARN("IDirectSoundCaptureBufferImpl_Create() failed\n");
        *lplpDirectSoundCaptureBuffer8 = NULL;
        *lplpDirectSoundBuffer8 = NULL;
        return hr;
    }

    return hr;
}

static const IDirectSoundFullDuplexVtbl dsfdvt =
{
    /* IUnknown methods */
    IDirectSoundFullDuplexImpl_QueryInterface,
    IDirectSoundFullDuplexImpl_AddRef,
    IDirectSoundFullDuplexImpl_Release,

    /* IDirectSoundFullDuplex methods */
    IDirectSoundFullDuplexImpl_Initialize
};

HRESULT DSOUND_FullDuplexCreate(
    REFIID riid,
    LPDIRECTSOUNDFULLDUPLEX* ppDSFD)
{
    IDirectSoundFullDuplexImpl *This = NULL;
    TRACE("(%s, %p)\n", debugstr_guid(riid), ppDSFD);

    if (ppDSFD == NULL) {
        WARN("invalid parameter: ppDSFD == NULL\n");
        return DSERR_INVALIDPARAM;
    }

    if (!IsEqualIID(riid, &IID_IUnknown) &&
        !IsEqualIID(riid, &IID_IDirectSoundFullDuplex)) {
        *ppDSFD = 0;
        return E_NOINTERFACE;
    }

    /* Get dsound configuration */
    setup_dsound_options();

    This = HeapAlloc(GetProcessHeap(),
        HEAP_ZERO_MEMORY, sizeof(IDirectSoundFullDuplexImpl));

    if (This == NULL) {
        WARN("out of memory\n");
        *ppDSFD = NULL;
        return DSERR_OUTOFMEMORY;
    }

    This->lpVtbl = &dsfdvt;
    This->ref = 1;
    This->capture_device = NULL;
    This->renderer_device = NULL;

    *ppDSFD = (LPDIRECTSOUNDFULLDUPLEX)This;

    return DS_OK;
}

/***************************************************************************
 * DirectSoundFullDuplexCreate [DSOUND.10]
 *
 * Create and initialize a DirectSoundFullDuplex interface.
 *
 * PARAMS
 *    pcGuidCaptureDevice [I] Address of sound capture device GUID.
 *    pcGuidRenderDevice  [I] Address of sound render device GUID.
 *    pcDSCBufferDesc     [I] Address of capture buffer description.
 *    pcDSBufferDesc      [I] Address of  render buffer description.
 *    hWnd                [I] Handle to application window.
 *    dwLevel             [I] Cooperative level.
 *    ppDSFD              [O] Address where full duplex interface returned.
 *    ppDSCBuffer8        [0] Address where capture buffer interface returned.
 *    ppDSBuffer8         [0] Address where render buffer interface returned.
 *    pUnkOuter           [I] Must be NULL.
 *
 * RETURNS
 *    Success: DS_OK
 *    Failure: DSERR_NOAGGREGATION, DSERR_ALLOCATED, DSERR_INVALIDPARAM,
 *             DSERR_OUTOFMEMORY DSERR_INVALIDCALL DSERR_NODRIVER
 */
HRESULT WINAPI
DirectSoundFullDuplexCreate(
    LPCGUID pcGuidCaptureDevice,
    LPCGUID pcGuidRenderDevice,
    LPCDSCBUFFERDESC pcDSCBufferDesc,
    LPCDSBUFFERDESC pcDSBufferDesc,
    HWND hWnd,
    DWORD dwLevel,
    LPDIRECTSOUNDFULLDUPLEX *ppDSFD,
    LPDIRECTSOUNDCAPTUREBUFFER8 *ppDSCBuffer8,
    LPDIRECTSOUNDBUFFER8 *ppDSBuffer8,
    LPUNKNOWN pUnkOuter)
{
    HRESULT hres;
    IDirectSoundFullDuplexImpl *This = NULL;
    TRACE("(%s,%s,%p,%p,%p,%x,%p,%p,%p,%p)\n",
        debugstr_guid(pcGuidCaptureDevice), debugstr_guid(pcGuidRenderDevice),
        pcDSCBufferDesc, pcDSBufferDesc, hWnd, dwLevel, ppDSFD, ppDSCBuffer8,
        ppDSBuffer8, pUnkOuter);

    if (pUnkOuter) {
        WARN("pUnkOuter != 0\n");
        *ppDSFD = NULL;
        return DSERR_NOAGGREGATION;
    }

    if (pcDSCBufferDesc == NULL) {
        WARN("invalid parameter: pcDSCBufferDesc == NULL\n");
        *ppDSFD = NULL;
        return DSERR_INVALIDPARAM;
    }

    if (pcDSBufferDesc == NULL) {
        WARN("invalid parameter: pcDSBufferDesc == NULL\n");
        *ppDSFD = NULL;
        return DSERR_INVALIDPARAM;
    }

    if (ppDSFD == NULL) {
        WARN("invalid parameter: ppDSFD == NULL\n");
        return DSERR_INVALIDPARAM;
    }

    if (ppDSCBuffer8 == NULL) {
        WARN("invalid parameter: ppDSCBuffer8 == NULL\n");
        *ppDSFD = NULL;
        return DSERR_INVALIDPARAM;
    }

    if (ppDSBuffer8 == NULL) {
        WARN("invalid parameter: ppDSBuffer8 == NULL\n");
        *ppDSFD = NULL;
        return DSERR_INVALIDPARAM;
    }

    hres = DSOUND_FullDuplexCreate(&IID_IDirectSoundFullDuplex, (LPDIRECTSOUNDFULLDUPLEX*)&This);
    if (FAILED(hres)) return hres;

    hres = IDirectSoundFullDuplexImpl_Initialize((LPDIRECTSOUNDFULLDUPLEX)This,
                                                 pcGuidCaptureDevice,
                                                 pcGuidRenderDevice,
                                                 pcDSCBufferDesc,
                                                 pcDSBufferDesc,
                                                 hWnd, dwLevel, ppDSCBuffer8,
                                                 ppDSBuffer8);
    if (hres != DS_OK) {
        IUnknown_Release((LPDIRECTSOUNDFULLDUPLEX)This);
        WARN("IDirectSoundFullDuplexImpl_Initialize failed\n");
        *ppDSFD = NULL;
    } else
        *ppDSFD = (LPDIRECTSOUNDFULLDUPLEX)This;

    return hres;
}
