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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
/*
 * TODO:
 *	Implement DirectSoundFullDuplex support.
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

static HRESULT WINAPI IDirectSoundFullDuplexImpl_Initialize(
    LPDIRECTSOUNDFULLDUPLEX iface,
    LPCGUID pCaptureGuid,
    LPCGUID pRendererGuid,
    LPCDSCBUFFERDESC lpDscBufferDesc,
    LPCDSBUFFERDESC lpDsBufferDesc,
    HWND hWnd,
    DWORD dwLevel,
    LPLPDIRECTSOUNDCAPTUREBUFFER8 lplpDirectSoundCaptureBuffer8,
    LPLPDIRECTSOUNDBUFFER8 lplpDirectSoundBuffer8 );

static const IDirectSoundFullDuplexVtbl dsfdvt;

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
    IDirectSoundFullDuplexImpl** ippDSFD=(IDirectSoundFullDuplexImpl**)ppDSFD;
    TRACE("(%s,%s,%p,%p,%p,%lx,%p,%p,%p,%p)\n", debugstr_guid(pcGuidCaptureDevice),
	debugstr_guid(pcGuidRenderDevice), pcDSCBufferDesc, pcDSBufferDesc,
	hWnd, dwLevel, ppDSFD, ppDSCBuffer8, ppDSBuffer8, pUnkOuter);

    if ( pUnkOuter ) {
	WARN("pUnkOuter != 0\n");
        return DSERR_NOAGGREGATION;
    }

    *ippDSFD = HeapAlloc(GetProcessHeap(),
	HEAP_ZERO_MEMORY, sizeof(IDirectSoundFullDuplexImpl));

    if (*ippDSFD == NULL) {
	WARN("out of memory\n");
	return DSERR_OUTOFMEMORY;
    } else {
	HRESULT hres;
        IDirectSoundFullDuplexImpl *This = (IDirectSoundFullDuplexImpl *)*ippDSFD;

        This->ref = 1;
        This->lpVtbl = &dsfdvt;

        InitializeCriticalSection( &(This->lock) );
        This->lock.DebugInfo->Spare[0] = (DWORD_PTR)"DSDUPLEX_lock";

        hres = IDirectSoundFullDuplexImpl_Initialize( (LPDIRECTSOUNDFULLDUPLEX)This,
                                                      pcGuidCaptureDevice, pcGuidRenderDevice,
                                                      pcDSCBufferDesc, pcDSBufferDesc,
                                                      hWnd, dwLevel, ppDSCBuffer8, ppDSBuffer8);
	if (hres != DS_OK)
	    WARN("IDirectSoundFullDuplexImpl_Initialize failed\n");
	return hres;
    }
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
    return E_NOINTERFACE;
}

static ULONG WINAPI
IDirectSoundFullDuplexImpl_AddRef( LPDIRECTSOUNDFULLDUPLEX iface )
{
    IDirectSoundFullDuplexImpl *This = (IDirectSoundFullDuplexImpl *)iface;
    ULONG ref = InterlockedIncrement(&(This->ref));
    TRACE("(%p) ref was %ld\n", This, ref - 1);
    return ref;
}

static ULONG WINAPI
IDirectSoundFullDuplexImpl_Release( LPDIRECTSOUNDFULLDUPLEX iface )
{
    IDirectSoundFullDuplexImpl *This = (IDirectSoundFullDuplexImpl *)iface;
    ULONG ref = InterlockedDecrement(&(This->ref));
    TRACE("(%p) ref was %ld\n", This, ref - 1);

    if (!ref) {
        This->lock.DebugInfo->Spare[0] = 0;
        DeleteCriticalSection( &(This->lock) );
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
    IDirectSoundFullDuplexImpl *This = (IDirectSoundFullDuplexImpl *)iface;
    IDirectSoundCaptureBufferImpl** ippdscb=(IDirectSoundCaptureBufferImpl**)lplpDirectSoundCaptureBuffer8;
    IDirectSoundBufferImpl** ippdsc=(IDirectSoundBufferImpl**)lplpDirectSoundBuffer8;

    FIXME( "(%p,%s,%s,%p,%p,%p,%lx,%p,%p) stub!\n", This, debugstr_guid(pCaptureGuid),
	debugstr_guid(pRendererGuid), lpDscBufferDesc, lpDsBufferDesc, hWnd, dwLevel,
	ippdscb, ippdsc);

    return E_FAIL;
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

/*******************************************************************************
 * DirectSoundFullDuplex ClassFactory
 */

static HRESULT WINAPI
DSFDCF_QueryInterface(LPCLASSFACTORY iface,REFIID riid,LPVOID *ppobj)
{
    IClassFactoryImpl *This = (IClassFactoryImpl *)iface;

    FIXME("(%p)->(%s,%p),stub!\n",This,debugstr_guid(riid),ppobj);
    return E_NOINTERFACE;
}

static ULONG WINAPI
DSFDCF_AddRef(LPCLASSFACTORY iface)
{
    IClassFactoryImpl *This = (IClassFactoryImpl *)iface;
    TRACE("(%p) ref was %ld\n", This, This->ref);
    return InterlockedIncrement(&(This->ref));
}

static ULONG WINAPI
DSFDCF_Release(LPCLASSFACTORY iface)
{
    IClassFactoryImpl *This = (IClassFactoryImpl *)iface;
    /* static class, won't be  freed */
    TRACE("(%p) ref was %ld\n", This, This->ref);
    return InterlockedDecrement(&(This->ref));
}

static HRESULT WINAPI
DSFDCF_CreateInstance(
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

    if ( IsEqualGUID( &IID_IDirectSoundFullDuplex, riid ) ) {
	/* FIXME: how do we do this one ? */
	FIXME("not implemented\n");
	return E_NOINTERFACE;
    }

    WARN("(%p,%p,%s,%p) Interface not found!\n",This,pOuter,debugstr_guid(riid),ppobj);	
    return E_NOINTERFACE;
}

static HRESULT WINAPI
DSFDCF_LockServer(LPCLASSFACTORY iface,BOOL dolock)
{
    IClassFactoryImpl *This = (IClassFactoryImpl *)iface;
    FIXME("(%p)->(%d),stub!\n",This,dolock);
    return S_OK;
}

static const IClassFactoryVtbl DSFDCF_Vtbl =
{
    DSFDCF_QueryInterface,
    DSFDCF_AddRef,
    DSFDCF_Release,
    DSFDCF_CreateInstance,
    DSFDCF_LockServer
};

IClassFactoryImpl DSOUND_FULLDUPLEX_CF = { &DSFDCF_Vtbl, 1 };
