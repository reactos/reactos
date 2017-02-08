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

#include <stdarg.h>

#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H

#define NONAMELESSSTRUCT
#define NONAMELESSUNION
#define COBJMACROS
#include <windef.h>
#include <winbase.h>
//#include "winuser.h"
#include <mmsystem.h>
//#include "mmddk.h"
#include <winternl.h>
#include <wine/debug.h>
#include <dsound.h>
#include "dsound_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(dsound);

/*****************************************************************************
 * IDirectSoundFullDuplex implementation structure
 */
typedef struct IDirectSoundFullDuplexImpl
{
    IUnknown               IUnknown_iface;
    IDirectSoundFullDuplex IDirectSoundFullDuplex_iface;
    LONG                   ref, refdsfd, numIfaces;
    IUnknown              *ds8_unk;     /* Aggregated IDirectSound8 */
    IUnknown              *dsc8_unk;    /* Aggregated IDirectSoundCapture8 */
} IDirectSoundFullDuplexImpl;

static void fullduplex_destroy(IDirectSoundFullDuplexImpl *This)
{
    IDirectSound8 *ds8;
    IDirectSoundCapture8 *dsc8;

    if (This->ds8_unk) {
        IUnknown_QueryInterface(This->ds8_unk, &IID_IDirectSound8, (void**)&ds8);
        while(IDirectSound8_Release(ds8) > 0);
        IUnknown_Release(This->ds8_unk);
    }
    if (This->dsc8_unk) {
        IUnknown_QueryInterface(This->dsc8_unk, &IID_IDirectSoundCapture8, (void**)&dsc8);
        while(IDirectSoundCapture_Release(dsc8) > 0);
        IUnknown_Release(This->dsc8_unk);
    }
    HeapFree(GetProcessHeap(), 0, This);
    TRACE("(%p) released\n", This);
}

/*******************************************************************************
 * IUnknown implemetation for DirectSoundFullDuplex
 */
static inline IDirectSoundFullDuplexImpl *impl_from_IUnknown(IUnknown *iface)
{
    return CONTAINING_RECORD(iface, IDirectSoundFullDuplexImpl, IUnknown_iface);
}

static HRESULT WINAPI IUnknownImpl_QueryInterface(IUnknown *iface, REFIID riid, void **ppv)
{
    IDirectSoundFullDuplexImpl *This = impl_from_IUnknown(iface);

    TRACE("(%p,%s,%p)\n", This, debugstr_guid(riid), ppv);

    if (!ppv) {
        WARN("invalid parameter\n");
        return E_INVALIDARG;
    }

    if (IsEqualIID(riid, &IID_IUnknown)) {
        IUnknown_AddRef(&This->IUnknown_iface);
        *ppv = &This->IUnknown_iface;
        return S_OK;
    } else if (IsEqualIID(riid, &IID_IDirectSoundFullDuplex)) {
        IDirectSoundFullDuplex_AddRef(&This->IDirectSoundFullDuplex_iface);
        *ppv = &This->IDirectSoundFullDuplex_iface;
        return S_OK;
    } else if (This->ds8_unk && (IsEqualIID(riid, &IID_IDirectSound) ||
                                 IsEqualIID(riid, &IID_IDirectSound8)))
        return IUnknown_QueryInterface(This->ds8_unk, riid, ppv);
    else if (This->dsc8_unk && IsEqualIID(riid, &IID_IDirectSoundCapture))
        return IUnknown_QueryInterface(This->dsc8_unk, riid, ppv);

    *ppv = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI IUnknownImpl_AddRef(IUnknown *iface)
{
    IDirectSoundFullDuplexImpl *This = impl_from_IUnknown(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    if(ref == 1)
        InterlockedIncrement(&This->numIfaces);
    return ref;
}

static ULONG WINAPI IUnknownImpl_Release(IUnknown *iface)
{
    IDirectSoundFullDuplexImpl *This = impl_from_IUnknown(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    if (!ref && !InterlockedDecrement(&This->numIfaces))
        fullduplex_destroy(This);
    return ref;
}

static const IUnknownVtbl unk_vtbl =
{
    IUnknownImpl_QueryInterface,
    IUnknownImpl_AddRef,
    IUnknownImpl_Release
};

/***************************************************************************
 * IDirectSoundFullDuplex implementation
 */
static inline IDirectSoundFullDuplexImpl *impl_from_IDirectSoundFullDuplex(IDirectSoundFullDuplex *iface)
{
    return CONTAINING_RECORD(iface, IDirectSoundFullDuplexImpl, IDirectSoundFullDuplex_iface);
}

static HRESULT WINAPI IDirectSoundFullDuplexImpl_QueryInterface(IDirectSoundFullDuplex *iface,
        REFIID riid, void **ppv)
{
    IDirectSoundFullDuplexImpl *This = impl_from_IDirectSoundFullDuplex(iface);
    TRACE("(%p,%s,%p)\n", This, debugstr_guid(riid), ppv);
    return IUnknown_QueryInterface(&This->IUnknown_iface, riid, ppv);
}

static ULONG WINAPI IDirectSoundFullDuplexImpl_AddRef(IDirectSoundFullDuplex *iface)
{
    IDirectSoundFullDuplexImpl *This = impl_from_IDirectSoundFullDuplex(iface);
    ULONG ref = InterlockedIncrement(&This->refdsfd);

    TRACE("(%p) ref=%d\n", This, ref);

    if(ref == 1)
        InterlockedIncrement(&This->numIfaces);
    return ref;
}

static ULONG WINAPI IDirectSoundFullDuplexImpl_Release(IDirectSoundFullDuplex *iface)
{
    IDirectSoundFullDuplexImpl *This = impl_from_IDirectSoundFullDuplex(iface);
    ULONG ref = InterlockedDecrement(&This->refdsfd);

    TRACE("(%p) ref=%d\n", This, ref);

    if (!ref && !InterlockedDecrement(&This->numIfaces))
        fullduplex_destroy(This);
    return ref;
}

static HRESULT WINAPI IDirectSoundFullDuplexImpl_Initialize(IDirectSoundFullDuplex *iface,
        const GUID *capture_dev, const GUID *render_dev, const DSCBUFFERDESC *cbufdesc,
        const DSBUFFERDESC *bufdesc, HWND hwnd, DWORD level, IDirectSoundCaptureBuffer8 **dscb8,
        IDirectSoundBuffer8 **dsb8)
{
    IDirectSoundFullDuplexImpl *This = impl_from_IDirectSoundFullDuplex(iface);
    IDirectSound8 *ds8 = NULL;
    IDirectSoundCapture8 *dsc8 = NULL;
    HRESULT hr;

    TRACE("(%p,%s,%s,%p,%p,%p,%x,%p,%p)\n", This, debugstr_guid(capture_dev),
            debugstr_guid(render_dev), cbufdesc, bufdesc, hwnd, level, dscb8, dsb8);

    if (!dscb8 || !dsb8)
        return E_INVALIDARG;

    *dscb8 = NULL;
    *dsb8 = NULL;

    if (This->ds8_unk || This->dsc8_unk) {
        WARN("already initialized\n");
        return DSERR_ALREADYINITIALIZED;
    }

    hr = IDirectSoundImpl_Create(&This->IUnknown_iface, &IID_IUnknown, (void**)&This->ds8_unk,
            TRUE);
    if (SUCCEEDED(hr)) {
        IUnknown_QueryInterface(This->ds8_unk, &IID_IDirectSound8, (void**)&ds8);
        hr = IDirectSound_Initialize(ds8, render_dev);
    }
    if (hr != DS_OK) {
        WARN("Creating/initializing IDirectSound8 failed\n");
        goto error;
    }

    IDirectSound8_SetCooperativeLevel(ds8, hwnd, level);

    hr = IDirectSound8_CreateSoundBuffer(ds8, bufdesc, (IDirectSoundBuffer**)dsb8, NULL);
    if (hr != DS_OK) {
        WARN("IDirectSoundBuffer_Create() failed\n");
        goto error;
    }

    hr = IDirectSoundCaptureImpl_Create(&This->IUnknown_iface, &IID_IUnknown,
            (void**)&This->dsc8_unk, TRUE);
    if (SUCCEEDED(hr)) {
        IUnknown_QueryInterface(This->dsc8_unk, &IID_IDirectSoundCapture8, (void**)&dsc8);
        hr = IDirectSoundCapture_Initialize(dsc8, capture_dev);
    }
    if (hr != DS_OK) {
        WARN("Creating/initializing IDirectSoundCapture8 failed\n");
        goto error;
    }

    hr = IDirectSoundCapture_CreateCaptureBuffer(dsc8, cbufdesc,
            (IDirectSoundCaptureBuffer**)dscb8, NULL);
    if (hr != DS_OK) {
        WARN("IDirectSoundCapture_CreateCaptureBuffer() failed\n");
        goto error;
    }

    IDirectSound8_Release(ds8);
    IDirectSoundCapture_Release(dsc8);
    return DS_OK;

error:
    if (*dsb8) {
        IDirectSoundBuffer8_Release(*dsb8);
        *dsb8 = NULL;
    }
    if (ds8)
        IDirectSound8_Release(ds8);
    if (This->ds8_unk) {
        IUnknown_Release(This->ds8_unk);
        This->ds8_unk = NULL;
    }
    if (*dscb8) {
        IDirectSoundCaptureBuffer8_Release(*dscb8);
        *dscb8 = NULL;
    }
    if (dsc8)
        IDirectSoundCapture_Release(dsc8);
    if (This->dsc8_unk) {
        IUnknown_Release(This->dsc8_unk);
        This->dsc8_unk = NULL;
    }
    return hr;
}

static const IDirectSoundFullDuplexVtbl dsfd_vtbl =
{
    /* IUnknown methods */
    IDirectSoundFullDuplexImpl_QueryInterface,
    IDirectSoundFullDuplexImpl_AddRef,
    IDirectSoundFullDuplexImpl_Release,

    /* IDirectSoundFullDuplex methods */
    IDirectSoundFullDuplexImpl_Initialize
};

HRESULT DSOUND_FullDuplexCreate(REFIID riid, void **ppv)
{
    IDirectSoundFullDuplexImpl *obj;
    HRESULT hr;

    TRACE("(%s, %p)\n", debugstr_guid(riid), ppv);

    *ppv = NULL;
    obj = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*obj));
    if (!obj) {
        WARN("out of memory\n");
        return DSERR_OUTOFMEMORY;
    }

    setup_dsound_options();

    obj->IDirectSoundFullDuplex_iface.lpVtbl = &dsfd_vtbl;
    obj->IUnknown_iface.lpVtbl = &unk_vtbl;
    obj->ref = 1;
    obj->refdsfd = 0;
    obj->numIfaces = 1;

    hr = IUnknown_QueryInterface(&obj->IUnknown_iface, riid, ppv);
    IUnknown_Release(&obj->IUnknown_iface);

    return hr;
}

/***************************************************************************
 * DirectSoundFullDuplexCreate [DSOUND.10]
 *
 * Create and initialize a DirectSoundFullDuplex interface.
 *
 * PARAMS
 *    capture_dev         [I] Address of sound capture device GUID.
 *    render_dev          [I] Address of sound render device GUID.
 *    cbufdesc            [I] Address of capture buffer description.
 *    bufdesc             [I] Address of  render buffer description.
 *    hwnd                [I] Handle to application window.
 *    level               [I] Cooperative level.
 *    dsfd                [O] Address where full duplex interface returned.
 *    dscb8               [0] Address where capture buffer interface returned.
 *    dsb8                [0] Address where render buffer interface returned.
 *    outer_unk           [I] Must be NULL.
 *
 * RETURNS
 *    Success: DS_OK
 *    Failure: DSERR_NOAGGREGATION, DSERR_ALLOCATED, DSERR_INVALIDPARAM,
 *             DSERR_OUTOFMEMORY DSERR_INVALIDCALL DSERR_NODRIVER
 */
HRESULT WINAPI DirectSoundFullDuplexCreate(const GUID *capture_dev, const GUID *render_dev,
        const DSCBUFFERDESC *cbufdesc, const DSBUFFERDESC *bufdesc, HWND hwnd, DWORD level,
        IDirectSoundFullDuplex **dsfd, IDirectSoundCaptureBuffer8 **dscb8,
        IDirectSoundBuffer8 **dsb8, IUnknown *outer_unk)
{
    HRESULT hr;

    TRACE("(%s,%s,%p,%p,%p,%x,%p,%p,%p,%p)\n", debugstr_guid(capture_dev),
            debugstr_guid(render_dev), cbufdesc, bufdesc, hwnd, level, dsfd, dscb8, dsb8,
            outer_unk);

    if (!dsfd)
        return DSERR_INVALIDPARAM;
    if (outer_unk) {
        *dsfd = NULL;
        return DSERR_NOAGGREGATION;
    }

    hr = DSOUND_FullDuplexCreate(&IID_IDirectSoundFullDuplex, (void**)dsfd);
    if (hr == DS_OK) {
        hr = IDirectSoundFullDuplex_Initialize(*dsfd, capture_dev, render_dev, cbufdesc, bufdesc,
                hwnd, level, dscb8, dsb8);
        if (hr != DS_OK) {
            IDirectSoundFullDuplex_Release(*dsfd);
            *dsfd = NULL;
            WARN("IDirectSoundFullDuplexImpl_Initialize failed\n");
        }
    }

    return hr;
}
