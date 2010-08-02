/* DirectSound
 *
 * Copyright 1998 Marcus Meissner
 * Copyright 1998 Rob Riggs
 * Copyright 2000-2002 TransGaming Technologies, Inc.
 * Copyright 2004 Robert Reif
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
#include <stdio.h>

#define NONAMELESSSTRUCT
#define NONAMELESSUNION
#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "mmsystem.h"
#include "winternl.h"
#include "mmddk.h"
#include "wingdi.h"
#include "mmreg.h"
#include "ks.h"
#include "ksmedia.h"
#include "wine/debug.h"
#include "dsound.h"
#include "dsdriver.h"
#include "dsound_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(dsound);

/*****************************************************************************
 * IDirectSound COM components
 */
struct IDirectSound_IUnknown {
    const IUnknownVtbl         *lpVtbl;
    LONG                        ref;
    LPDIRECTSOUND8              pds;
};

static HRESULT IDirectSound_IUnknown_Create(LPDIRECTSOUND8 pds, LPUNKNOWN * ppunk);

struct IDirectSound_IDirectSound {
    const IDirectSoundVtbl     *lpVtbl;
    LONG                        ref;
    LPDIRECTSOUND8              pds;
};

static HRESULT IDirectSound_IDirectSound_Create(LPDIRECTSOUND8 pds, LPDIRECTSOUND * ppds);

/*****************************************************************************
 * IDirectSound8 COM components
 */
struct IDirectSound8_IUnknown {
    const IUnknownVtbl         *lpVtbl;
    LONG                        ref;
    LPDIRECTSOUND8              pds;
};

static HRESULT IDirectSound8_IUnknown_Create(LPDIRECTSOUND8 pds, LPUNKNOWN * ppunk);
static ULONG WINAPI IDirectSound8_IUnknown_AddRef(LPUNKNOWN iface);

struct IDirectSound8_IDirectSound {
    const IDirectSoundVtbl     *lpVtbl;
    LONG                        ref;
    LPDIRECTSOUND8              pds;
};

static HRESULT IDirectSound8_IDirectSound_Create(LPDIRECTSOUND8 pds, LPDIRECTSOUND * ppds);
static ULONG WINAPI IDirectSound8_IDirectSound_AddRef(LPDIRECTSOUND iface);

struct IDirectSound8_IDirectSound8 {
    const IDirectSound8Vtbl    *lpVtbl;
    LONG                        ref;
    LPDIRECTSOUND8              pds;
};

static HRESULT IDirectSound8_IDirectSound8_Create(LPDIRECTSOUND8 pds, LPDIRECTSOUND8 * ppds);
static ULONG WINAPI IDirectSound8_IDirectSound8_AddRef(LPDIRECTSOUND8 iface);

/*****************************************************************************
 * IDirectSound implementation structure
 */
struct IDirectSoundImpl
{
    LONG                        ref;

    DirectSoundDevice          *device;
    LPUNKNOWN                   pUnknown;
    LPDIRECTSOUND               pDS;
    LPDIRECTSOUND8              pDS8;
};

static HRESULT IDirectSoundImpl_Create(LPDIRECTSOUND8 * ppds);

static ULONG WINAPI IDirectSound_IUnknown_AddRef(LPUNKNOWN iface);
static ULONG WINAPI IDirectSound_IDirectSound_AddRef(LPDIRECTSOUND iface);

static HRESULT DirectSoundDevice_VerifyCertification(DirectSoundDevice * device, LPDWORD pdwCertified);

const char * dumpCooperativeLevel(DWORD level)
{
    static char unknown[32];
#define LE(x) case x: return #x
    switch (level) {
        LE(DSSCL_NORMAL);
        LE(DSSCL_PRIORITY);
        LE(DSSCL_EXCLUSIVE);
        LE(DSSCL_WRITEPRIMARY);
    }
#undef LE
    sprintf(unknown, "Unknown(%08x)", (UINT)level);
    return unknown;
}

static void _dump_DSCAPS(DWORD xmask) {
    struct {
        DWORD   mask;
        const char    *name;
    } flags[] = {
#define FE(x) { x, #x },
        FE(DSCAPS_PRIMARYMONO)
        FE(DSCAPS_PRIMARYSTEREO)
        FE(DSCAPS_PRIMARY8BIT)
        FE(DSCAPS_PRIMARY16BIT)
        FE(DSCAPS_CONTINUOUSRATE)
        FE(DSCAPS_EMULDRIVER)
        FE(DSCAPS_CERTIFIED)
        FE(DSCAPS_SECONDARYMONO)
        FE(DSCAPS_SECONDARYSTEREO)
        FE(DSCAPS_SECONDARY8BIT)
        FE(DSCAPS_SECONDARY16BIT)
#undef FE
    };
    unsigned int     i;

    for (i=0;i<sizeof(flags)/sizeof(flags[0]);i++)
        if ((flags[i].mask & xmask) == flags[i].mask)
            TRACE("%s ",flags[i].name);
}

static void _dump_DSBCAPS(DWORD xmask) {
    struct {
        DWORD   mask;
        const char    *name;
    } flags[] = {
#define FE(x) { x, #x },
        FE(DSBCAPS_PRIMARYBUFFER)
        FE(DSBCAPS_STATIC)
        FE(DSBCAPS_LOCHARDWARE)
        FE(DSBCAPS_LOCSOFTWARE)
        FE(DSBCAPS_CTRL3D)
        FE(DSBCAPS_CTRLFREQUENCY)
        FE(DSBCAPS_CTRLPAN)
        FE(DSBCAPS_CTRLVOLUME)
        FE(DSBCAPS_CTRLPOSITIONNOTIFY)
        FE(DSBCAPS_STICKYFOCUS)
        FE(DSBCAPS_GLOBALFOCUS)
        FE(DSBCAPS_GETCURRENTPOSITION2)
        FE(DSBCAPS_MUTE3DATMAXDISTANCE)
#undef FE
    };
    unsigned int     i;

    for (i=0;i<sizeof(flags)/sizeof(flags[0]);i++)
        if ((flags[i].mask & xmask) == flags[i].mask)
            TRACE("%s ",flags[i].name);
}

/*******************************************************************************
 *		IDirectSoundImpl_DirectSound
 */
static HRESULT DSOUND_QueryInterface(
    LPDIRECTSOUND8 iface,
    REFIID riid,
    LPVOID * ppobj)
{
    IDirectSoundImpl *This = (IDirectSoundImpl *)iface;
    TRACE("(%p,%s,%p)\n",This,debugstr_guid(riid),ppobj);

    if (ppobj == NULL) {
        WARN("invalid parameter\n");
        return E_INVALIDARG;
    }

    if (IsEqualIID(riid, &IID_IUnknown)) {
        if (!This->pUnknown) {
            IDirectSound_IUnknown_Create(iface, &This->pUnknown);
            if (!This->pUnknown) {
                WARN("IDirectSound_IUnknown_Create() failed\n");
                *ppobj = NULL;
                return E_NOINTERFACE;
            }
        }
        IDirectSound_IUnknown_AddRef(This->pUnknown);
        *ppobj = This->pUnknown;
        return S_OK;
    } else if (IsEqualIID(riid, &IID_IDirectSound)) {
        if (!This->pDS) {
            IDirectSound_IDirectSound_Create(iface, &This->pDS);
            if (!This->pDS) {
                WARN("IDirectSound_IDirectSound_Create() failed\n");
                *ppobj = NULL;
                return E_NOINTERFACE;
            }
        }
        IDirectSound_IDirectSound_AddRef(This->pDS);
        *ppobj = This->pDS;
        return S_OK;
    }

    *ppobj = NULL;
    WARN("Unknown IID %s\n",debugstr_guid(riid));
    return E_NOINTERFACE;
}

static HRESULT DSOUND_QueryInterface8(
    LPDIRECTSOUND8 iface,
    REFIID riid,
    LPVOID * ppobj)
{
    IDirectSoundImpl *This = (IDirectSoundImpl *)iface;
    TRACE("(%p,%s,%p)\n",This,debugstr_guid(riid),ppobj);

    if (ppobj == NULL) {
        WARN("invalid parameter\n");
        return E_INVALIDARG;
    }

    if (IsEqualIID(riid, &IID_IUnknown)) {
        if (!This->pUnknown) {
            IDirectSound8_IUnknown_Create(iface, &This->pUnknown);
            if (!This->pUnknown) {
                WARN("IDirectSound8_IUnknown_Create() failed\n");
                *ppobj = NULL;
                return E_NOINTERFACE;
            }
        }
        IDirectSound8_IUnknown_AddRef(This->pUnknown);
        *ppobj = This->pUnknown;
        return S_OK;
    } else if (IsEqualIID(riid, &IID_IDirectSound)) {
        if (!This->pDS) {
            IDirectSound8_IDirectSound_Create(iface, &This->pDS);
            if (!This->pDS) {
                WARN("IDirectSound8_IDirectSound_Create() failed\n");
                *ppobj = NULL;
                return E_NOINTERFACE;
            }
        }
        IDirectSound8_IDirectSound_AddRef(This->pDS);
        *ppobj = This->pDS;
        return S_OK;
    } else if (IsEqualIID(riid, &IID_IDirectSound8)) {
        if (!This->pDS8) {
            IDirectSound8_IDirectSound8_Create(iface, &This->pDS8);
            if (!This->pDS8) {
                WARN("IDirectSound8_IDirectSound8_Create() failed\n");
                *ppobj = NULL;
                return E_NOINTERFACE;
            }
        }
        IDirectSound8_IDirectSound8_AddRef(This->pDS8);
        *ppobj = This->pDS8;
        return S_OK;
    }

    *ppobj = NULL;
    WARN("Unknown IID %s\n",debugstr_guid(riid));
    return E_NOINTERFACE;
}

static ULONG IDirectSoundImpl_AddRef(
    LPDIRECTSOUND8 iface)
{
    IDirectSoundImpl *This = (IDirectSoundImpl *)iface;
    ULONG ref = InterlockedIncrement(&(This->ref));
    TRACE("(%p) ref was %d\n", This, ref - 1);
    return ref;
}

static ULONG IDirectSoundImpl_Release(
    LPDIRECTSOUND8 iface)
{
    IDirectSoundImpl *This = (IDirectSoundImpl *)iface;
    ULONG ref = InterlockedDecrement(&(This->ref));
    TRACE("(%p) ref was %d\n", This, ref + 1);

    if (!ref) {
        if (This->device)
            DirectSoundDevice_Release(This->device);
        HeapFree(GetProcessHeap(),0,This);
        TRACE("(%p) released\n", This);
    }
    return ref;
}

static HRESULT IDirectSoundImpl_Create(
    LPDIRECTSOUND8 * ppDS)
{
    IDirectSoundImpl* pDS;
    TRACE("(%p)\n",ppDS);

    /* Allocate memory */
    pDS = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(IDirectSoundImpl));
    if (pDS == NULL) {
        WARN("out of memory\n");
        *ppDS = NULL;
        return DSERR_OUTOFMEMORY;
    }

    pDS->ref    = 0;
    pDS->device = NULL;

    *ppDS = (LPDIRECTSOUND8)pDS;

    return DS_OK;
}

/*******************************************************************************
 *		IDirectSound_IUnknown
 */
static HRESULT WINAPI IDirectSound_IUnknown_QueryInterface(
    LPUNKNOWN iface,
    REFIID riid,
    LPVOID * ppobj)
{
    IDirectSound_IUnknown *This = (IDirectSound_IUnknown *)iface;
    TRACE("(%p,%s,%p)\n",This,debugstr_guid(riid),ppobj);
    return DSOUND_QueryInterface(This->pds, riid, ppobj);
}

static ULONG WINAPI IDirectSound_IUnknown_AddRef(
    LPUNKNOWN iface)
{
    IDirectSound_IUnknown *This = (IDirectSound_IUnknown *)iface;
    ULONG ref = InterlockedIncrement(&(This->ref));
    TRACE("(%p) ref was %d\n", This, ref - 1);
    return ref;
}

static ULONG WINAPI IDirectSound_IUnknown_Release(
    LPUNKNOWN iface)
{
    IDirectSound_IUnknown *This = (IDirectSound_IUnknown *)iface;
    ULONG ref = InterlockedDecrement(&(This->ref));
    TRACE("(%p) ref was %d\n", This, ref + 1);
    if (!ref) {
        ((IDirectSoundImpl*)This->pds)->pUnknown = NULL;
        IDirectSoundImpl_Release(This->pds);
        HeapFree(GetProcessHeap(), 0, This);
        TRACE("(%p) released\n", This);
    }
    return ref;
}

static const IUnknownVtbl DirectSound_Unknown_Vtbl =
{
    IDirectSound_IUnknown_QueryInterface,
    IDirectSound_IUnknown_AddRef,
    IDirectSound_IUnknown_Release
};

static HRESULT IDirectSound_IUnknown_Create(
    LPDIRECTSOUND8 pds,
    LPUNKNOWN * ppunk)
{
    IDirectSound_IUnknown * pdsunk;
    TRACE("(%p,%p)\n",pds,ppunk);

    if (ppunk == NULL) {
        ERR("invalid parameter: ppunk == NULL\n");
        return DSERR_INVALIDPARAM;
    }

    if (pds == NULL) {
        ERR("invalid parameter: pds == NULL\n");
        *ppunk = NULL;
        return DSERR_INVALIDPARAM;
    }

    pdsunk = HeapAlloc(GetProcessHeap(),0,sizeof(*pdsunk));
    if (pdsunk == NULL) {
        WARN("out of memory\n");
        *ppunk = NULL;
        return DSERR_OUTOFMEMORY;
    }

    pdsunk->lpVtbl = &DirectSound_Unknown_Vtbl;
    pdsunk->ref = 0;
    pdsunk->pds = pds;

    IDirectSoundImpl_AddRef(pds);
    *ppunk = (LPUNKNOWN)pdsunk;

    return DS_OK;
}

/*******************************************************************************
 *		IDirectSound_IDirectSound
 */
static HRESULT WINAPI IDirectSound_IDirectSound_QueryInterface(
    LPDIRECTSOUND iface,
    REFIID riid,
    LPVOID * ppobj)
{
    IDirectSound_IDirectSound *This = (IDirectSound_IDirectSound *)iface;
    TRACE("(%p,%s,%p)\n",This,debugstr_guid(riid),ppobj);
    return DSOUND_QueryInterface(This->pds, riid, ppobj);
}

static ULONG WINAPI IDirectSound_IDirectSound_AddRef(
    LPDIRECTSOUND iface)
{
    IDirectSound_IDirectSound *This = (IDirectSound_IDirectSound *)iface;
    ULONG ref = InterlockedIncrement(&(This->ref));
    TRACE("(%p) ref was %d\n", This, ref - 1);
    return ref;
}

static ULONG WINAPI IDirectSound_IDirectSound_Release(
    LPDIRECTSOUND iface)
{
    IDirectSound_IDirectSound *This = (IDirectSound_IDirectSound *)iface;
    ULONG ref = InterlockedDecrement(&(This->ref));
    TRACE("(%p) ref was %d\n", This, ref + 1);
    if (!ref) {
        ((IDirectSoundImpl*)This->pds)->pDS = NULL;
        IDirectSoundImpl_Release(This->pds);
        HeapFree(GetProcessHeap(), 0, This);
        TRACE("(%p) released\n", This);
    }
    return ref;
}

static HRESULT WINAPI IDirectSound_IDirectSound_CreateSoundBuffer(
    LPDIRECTSOUND iface,
    LPCDSBUFFERDESC dsbd,
    LPLPDIRECTSOUNDBUFFER ppdsb,
    LPUNKNOWN lpunk)
{
    IDirectSound_IDirectSound *This = (IDirectSound_IDirectSound *)iface;
    TRACE("(%p,%p,%p,%p)\n",This,dsbd,ppdsb,lpunk);
    return DirectSoundDevice_CreateSoundBuffer(((IDirectSoundImpl *)This->pds)->device,dsbd,ppdsb,lpunk,FALSE);
}

static HRESULT WINAPI IDirectSound_IDirectSound_GetCaps(
    LPDIRECTSOUND iface,
    LPDSCAPS lpDSCaps)
{
    IDirectSound_IDirectSound *This = (IDirectSound_IDirectSound *)iface;
    TRACE("(%p,%p)\n",This,lpDSCaps);
    return DirectSoundDevice_GetCaps(((IDirectSoundImpl *)This->pds)->device, lpDSCaps);
}

static HRESULT WINAPI IDirectSound_IDirectSound_DuplicateSoundBuffer(
    LPDIRECTSOUND iface,
    LPDIRECTSOUNDBUFFER psb,
    LPLPDIRECTSOUNDBUFFER ppdsb)
{
    IDirectSound_IDirectSound *This = (IDirectSound_IDirectSound *)iface;
    TRACE("(%p,%p,%p)\n",This,psb,ppdsb);
    return DirectSoundDevice_DuplicateSoundBuffer(((IDirectSoundImpl *)This->pds)->device,psb,ppdsb);
}

static HRESULT WINAPI IDirectSound_IDirectSound_SetCooperativeLevel(
    LPDIRECTSOUND iface,
    HWND hwnd,
    DWORD level)
{
    IDirectSound_IDirectSound *This = (IDirectSound_IDirectSound *)iface;
    TRACE("(%p,%p,%s)\n",This,hwnd,dumpCooperativeLevel(level));
    return DirectSoundDevice_SetCooperativeLevel(((IDirectSoundImpl *)This->pds)->device, hwnd, level);
}

static HRESULT WINAPI IDirectSound_IDirectSound_Compact(
    LPDIRECTSOUND iface)
{
    IDirectSound_IDirectSound *This = (IDirectSound_IDirectSound *)iface;
    TRACE("(%p)\n", This);
    return DirectSoundDevice_Compact(((IDirectSoundImpl *)This->pds)->device);
}

static HRESULT WINAPI IDirectSound_IDirectSound_GetSpeakerConfig(
    LPDIRECTSOUND iface,
    LPDWORD lpdwSpeakerConfig)
{
    IDirectSound_IDirectSound *This = (IDirectSound_IDirectSound *)iface;
    TRACE("(%p, %p)\n", This, lpdwSpeakerConfig);
    return DirectSoundDevice_GetSpeakerConfig(((IDirectSoundImpl *)This->pds)->device,lpdwSpeakerConfig);
}

static HRESULT WINAPI IDirectSound_IDirectSound_SetSpeakerConfig(
    LPDIRECTSOUND iface,
    DWORD config)
{
    IDirectSound_IDirectSound *This = (IDirectSound_IDirectSound *)iface;
    TRACE("(%p,0x%08x)\n",This,config);
    return DirectSoundDevice_SetSpeakerConfig(((IDirectSoundImpl *)This->pds)->device,config);
}

static HRESULT WINAPI IDirectSound_IDirectSound_Initialize(
    LPDIRECTSOUND iface,
    LPCGUID lpcGuid)
{
    IDirectSound_IDirectSound *This = (IDirectSound_IDirectSound *)iface;
    TRACE("(%p, %s)\n", This, debugstr_guid(lpcGuid));
    return DirectSoundDevice_Initialize(&((IDirectSoundImpl *)This->pds)->device,lpcGuid);
}

static const IDirectSoundVtbl DirectSound_DirectSound_Vtbl =
{
    IDirectSound_IDirectSound_QueryInterface,
    IDirectSound_IDirectSound_AddRef,
    IDirectSound_IDirectSound_Release,
    IDirectSound_IDirectSound_CreateSoundBuffer,
    IDirectSound_IDirectSound_GetCaps,
    IDirectSound_IDirectSound_DuplicateSoundBuffer,
    IDirectSound_IDirectSound_SetCooperativeLevel,
    IDirectSound_IDirectSound_Compact,
    IDirectSound_IDirectSound_GetSpeakerConfig,
    IDirectSound_IDirectSound_SetSpeakerConfig,
    IDirectSound_IDirectSound_Initialize
};

static HRESULT IDirectSound_IDirectSound_Create(
    LPDIRECTSOUND8  pds,
    LPDIRECTSOUND * ppds)
{
    IDirectSound_IDirectSound * pdsds;
    TRACE("(%p,%p)\n",pds,ppds);

    if (ppds == NULL) {
        ERR("invalid parameter: ppds == NULL\n");
        return DSERR_INVALIDPARAM;
    }

    if (pds == NULL) {
        ERR("invalid parameter: pds == NULL\n");
        *ppds = NULL;
        return DSERR_INVALIDPARAM;
    }

    pdsds = HeapAlloc(GetProcessHeap(),0,sizeof(*pdsds));
    if (pdsds == NULL) {
        WARN("out of memory\n");
        *ppds = NULL;
        return DSERR_OUTOFMEMORY;
    }

    pdsds->lpVtbl = &DirectSound_DirectSound_Vtbl;
    pdsds->ref = 0;
    pdsds->pds = pds;

    IDirectSoundImpl_AddRef(pds);
    *ppds = (LPDIRECTSOUND)pdsds;

    return DS_OK;
}

/*******************************************************************************
 *		IDirectSound8_IUnknown
 */
static HRESULT WINAPI IDirectSound8_IUnknown_QueryInterface(
    LPUNKNOWN iface,
    REFIID riid,
    LPVOID * ppobj)
{
    IDirectSound_IUnknown *This = (IDirectSound_IUnknown *)iface;
    TRACE("(%p,%s,%p)\n",This,debugstr_guid(riid),ppobj);
    return DSOUND_QueryInterface8(This->pds, riid, ppobj);
}

static ULONG WINAPI IDirectSound8_IUnknown_AddRef(
    LPUNKNOWN iface)
{
    IDirectSound_IUnknown *This = (IDirectSound_IUnknown *)iface;
    ULONG ref = InterlockedIncrement(&(This->ref));
    TRACE("(%p) ref was %d\n", This, ref - 1);
    return ref;
}

static ULONG WINAPI IDirectSound8_IUnknown_Release(
    LPUNKNOWN iface)
{
    IDirectSound_IUnknown *This = (IDirectSound_IUnknown *)iface;
    ULONG ref = InterlockedDecrement(&(This->ref));
    TRACE("(%p) ref was %d\n", This, ref + 1);
    if (!ref) {
        ((IDirectSoundImpl*)This->pds)->pUnknown = NULL;
        IDirectSoundImpl_Release(This->pds);
        HeapFree(GetProcessHeap(), 0, This);
        TRACE("(%p) released\n", This);
    }
    return ref;
}

static const IUnknownVtbl DirectSound8_Unknown_Vtbl =
{
    IDirectSound8_IUnknown_QueryInterface,
    IDirectSound8_IUnknown_AddRef,
    IDirectSound8_IUnknown_Release
};

static HRESULT IDirectSound8_IUnknown_Create(
    LPDIRECTSOUND8 pds,
    LPUNKNOWN * ppunk)
{
    IDirectSound8_IUnknown * pdsunk;
    TRACE("(%p,%p)\n",pds,ppunk);

    if (ppunk == NULL) {
        ERR("invalid parameter: ppunk == NULL\n");
        return DSERR_INVALIDPARAM;
    }

    if (pds == NULL) {
        ERR("invalid parameter: pds == NULL\n");
        *ppunk = NULL;
        return DSERR_INVALIDPARAM;
    }

    pdsunk = HeapAlloc(GetProcessHeap(),0,sizeof(*pdsunk));
    if (pdsunk == NULL) {
        WARN("out of memory\n");
        *ppunk = NULL;
        return DSERR_OUTOFMEMORY;
    }

    pdsunk->lpVtbl = &DirectSound8_Unknown_Vtbl;
    pdsunk->ref = 0;
    pdsunk->pds = pds;

    IDirectSoundImpl_AddRef(pds);
    *ppunk = (LPUNKNOWN)pdsunk;

    return DS_OK;
}

/*******************************************************************************
 *		IDirectSound8_IDirectSound
 */
static HRESULT WINAPI IDirectSound8_IDirectSound_QueryInterface(
    LPDIRECTSOUND iface,
    REFIID riid,
    LPVOID * ppobj)
{
    IDirectSound8_IDirectSound *This = (IDirectSound8_IDirectSound *)iface;
    TRACE("(%p,%s,%p)\n",This,debugstr_guid(riid),ppobj);
    return DSOUND_QueryInterface8(This->pds, riid, ppobj);
}

static ULONG WINAPI IDirectSound8_IDirectSound_AddRef(
    LPDIRECTSOUND iface)
{
    IDirectSound8_IDirectSound *This = (IDirectSound8_IDirectSound *)iface;
    ULONG ref = InterlockedIncrement(&(This->ref));
    TRACE("(%p) ref was %d\n", This, ref - 1);
    return ref;
}

static ULONG WINAPI IDirectSound8_IDirectSound_Release(
    LPDIRECTSOUND iface)
{
    IDirectSound8_IDirectSound *This = (IDirectSound8_IDirectSound *)iface;
    ULONG ref = InterlockedDecrement(&(This->ref));
    TRACE("(%p) ref was %d\n", This, ref + 1);
    if (!ref) {
        ((IDirectSoundImpl*)This->pds)->pDS = NULL;
        IDirectSoundImpl_Release(This->pds);
        HeapFree(GetProcessHeap(), 0, This);
        TRACE("(%p) released\n", This);
    }
    return ref;
}

static HRESULT WINAPI IDirectSound8_IDirectSound_CreateSoundBuffer(
    LPDIRECTSOUND iface,
    LPCDSBUFFERDESC dsbd,
    LPLPDIRECTSOUNDBUFFER ppdsb,
    LPUNKNOWN lpunk)
{
    IDirectSound8_IDirectSound *This = (IDirectSound8_IDirectSound *)iface;
    TRACE("(%p,%p,%p,%p)\n",This,dsbd,ppdsb,lpunk);
    return DirectSoundDevice_CreateSoundBuffer(((IDirectSoundImpl *)This->pds)->device,dsbd,ppdsb,lpunk,TRUE);
}

static HRESULT WINAPI IDirectSound8_IDirectSound_GetCaps(
    LPDIRECTSOUND iface,
    LPDSCAPS lpDSCaps)
{
    IDirectSound8_IDirectSound *This = (IDirectSound8_IDirectSound *)iface;
    TRACE("(%p,%p)\n",This,lpDSCaps);
    return DirectSoundDevice_GetCaps(((IDirectSoundImpl *)This->pds)->device, lpDSCaps);
}

static HRESULT WINAPI IDirectSound8_IDirectSound_DuplicateSoundBuffer(
    LPDIRECTSOUND iface,
    LPDIRECTSOUNDBUFFER psb,
    LPLPDIRECTSOUNDBUFFER ppdsb)
{
    IDirectSound8_IDirectSound *This = (IDirectSound8_IDirectSound *)iface;
    TRACE("(%p,%p,%p)\n",This,psb,ppdsb);
    return DirectSoundDevice_DuplicateSoundBuffer(((IDirectSoundImpl *)This->pds)->device,psb,ppdsb);
}

static HRESULT WINAPI IDirectSound8_IDirectSound_SetCooperativeLevel(
    LPDIRECTSOUND iface,
    HWND hwnd,
    DWORD level)
{
    IDirectSound8_IDirectSound *This = (IDirectSound8_IDirectSound *)iface;
    TRACE("(%p,%p,%s)\n",This,hwnd,dumpCooperativeLevel(level));
    return DirectSoundDevice_SetCooperativeLevel(((IDirectSoundImpl *)This->pds)->device, hwnd, level);
}

static HRESULT WINAPI IDirectSound8_IDirectSound_Compact(
    LPDIRECTSOUND iface)
{
    IDirectSound8_IDirectSound *This = (IDirectSound8_IDirectSound *)iface;
    TRACE("(%p)\n", This);
    return DirectSoundDevice_Compact(((IDirectSoundImpl *)This->pds)->device);
}

static HRESULT WINAPI IDirectSound8_IDirectSound_GetSpeakerConfig(
    LPDIRECTSOUND iface,
    LPDWORD lpdwSpeakerConfig)
{
    IDirectSound8_IDirectSound *This = (IDirectSound8_IDirectSound *)iface;
    TRACE("(%p, %p)\n", This, lpdwSpeakerConfig);
    return DirectSoundDevice_GetSpeakerConfig(((IDirectSoundImpl *)This->pds)->device,lpdwSpeakerConfig);
}

static HRESULT WINAPI IDirectSound8_IDirectSound_SetSpeakerConfig(
    LPDIRECTSOUND iface,
    DWORD config)
{
    IDirectSound8_IDirectSound *This = (IDirectSound8_IDirectSound *)iface;
    TRACE("(%p,0x%08x)\n",This,config);
    return DirectSoundDevice_SetSpeakerConfig(((IDirectSoundImpl *)This->pds)->device,config);
}

static HRESULT WINAPI IDirectSound8_IDirectSound_Initialize(
    LPDIRECTSOUND iface,
    LPCGUID lpcGuid)
{
    IDirectSound8_IDirectSound *This = (IDirectSound8_IDirectSound *)iface;
    TRACE("(%p, %s)\n", This, debugstr_guid(lpcGuid));
    return DirectSoundDevice_Initialize(&((IDirectSoundImpl *)This->pds)->device,lpcGuid);
}

static const IDirectSoundVtbl DirectSound8_DirectSound_Vtbl =
{
    IDirectSound8_IDirectSound_QueryInterface,
    IDirectSound8_IDirectSound_AddRef,
    IDirectSound8_IDirectSound_Release,
    IDirectSound8_IDirectSound_CreateSoundBuffer,
    IDirectSound8_IDirectSound_GetCaps,
    IDirectSound8_IDirectSound_DuplicateSoundBuffer,
    IDirectSound8_IDirectSound_SetCooperativeLevel,
    IDirectSound8_IDirectSound_Compact,
    IDirectSound8_IDirectSound_GetSpeakerConfig,
    IDirectSound8_IDirectSound_SetSpeakerConfig,
    IDirectSound8_IDirectSound_Initialize
};

static HRESULT IDirectSound8_IDirectSound_Create(
    LPDIRECTSOUND8 pds,
    LPDIRECTSOUND * ppds)
{
    IDirectSound8_IDirectSound * pdsds;
    TRACE("(%p,%p)\n",pds,ppds);

    if (ppds == NULL) {
        ERR("invalid parameter: ppds == NULL\n");
        return DSERR_INVALIDPARAM;
    }

    if (pds == NULL) {
        ERR("invalid parameter: pds == NULL\n");
        *ppds = NULL;
        return DSERR_INVALIDPARAM;
    }

    pdsds = HeapAlloc(GetProcessHeap(),0,sizeof(*pdsds));
    if (pdsds == NULL) {
        WARN("out of memory\n");
        *ppds = NULL;
        return DSERR_OUTOFMEMORY;
    }

    pdsds->lpVtbl = &DirectSound8_DirectSound_Vtbl;
    pdsds->ref = 0;
    pdsds->pds = pds;

    IDirectSoundImpl_AddRef(pds);
    *ppds = (LPDIRECTSOUND)pdsds;

    return DS_OK;
}

/*******************************************************************************
 *		IDirectSound8_IDirectSound8
 */
static HRESULT WINAPI IDirectSound8_IDirectSound8_QueryInterface(
    LPDIRECTSOUND8 iface,
    REFIID riid,
    LPVOID * ppobj)
{
    IDirectSound8_IDirectSound8 *This = (IDirectSound8_IDirectSound8 *)iface;
    TRACE("(%p,%s,%p)\n",This,debugstr_guid(riid),ppobj);
    return DSOUND_QueryInterface8(This->pds, riid, ppobj);
}

static ULONG WINAPI IDirectSound8_IDirectSound8_AddRef(
    LPDIRECTSOUND8 iface)
{
    IDirectSound8_IDirectSound8 *This = (IDirectSound8_IDirectSound8 *)iface;
    ULONG ref = InterlockedIncrement(&(This->ref));
    TRACE("(%p) ref was %d\n", This, ref - 1);
    return ref;
}

static ULONG WINAPI IDirectSound8_IDirectSound8_Release(
    LPDIRECTSOUND8 iface)
{
    IDirectSound8_IDirectSound8 *This = (IDirectSound8_IDirectSound8 *)iface;
    ULONG ref = InterlockedDecrement(&(This->ref));
    TRACE("(%p) ref was %d\n", This, ref + 1);
    if (!ref) {
        ((IDirectSoundImpl*)This->pds)->pDS8 = NULL;
        IDirectSoundImpl_Release(This->pds);
        HeapFree(GetProcessHeap(), 0, This);
        TRACE("(%p) released\n", This);
    }
    return ref;
}

static HRESULT WINAPI IDirectSound8_IDirectSound8_CreateSoundBuffer(
    LPDIRECTSOUND8 iface,
    LPCDSBUFFERDESC dsbd,
    LPLPDIRECTSOUNDBUFFER ppdsb,
    LPUNKNOWN lpunk)
{
    IDirectSound8_IDirectSound8 *This = (IDirectSound8_IDirectSound8 *)iface;
    TRACE("(%p,%p,%p,%p)\n",This,dsbd,ppdsb,lpunk);
    return DirectSoundDevice_CreateSoundBuffer(((IDirectSoundImpl *)This->pds)->device,dsbd,ppdsb,lpunk,TRUE);
}

static HRESULT WINAPI IDirectSound8_IDirectSound8_GetCaps(
    LPDIRECTSOUND8 iface,
    LPDSCAPS lpDSCaps)
{
    IDirectSound8_IDirectSound *This = (IDirectSound8_IDirectSound *)iface;
    TRACE("(%p,%p)\n",This,lpDSCaps);
    return DirectSoundDevice_GetCaps(((IDirectSoundImpl *)This->pds)->device, lpDSCaps);
}

static HRESULT WINAPI IDirectSound8_IDirectSound8_DuplicateSoundBuffer(
    LPDIRECTSOUND8 iface,
    LPDIRECTSOUNDBUFFER psb,
    LPLPDIRECTSOUNDBUFFER ppdsb)
{
    IDirectSound8_IDirectSound8 *This = (IDirectSound8_IDirectSound8 *)iface;
    TRACE("(%p,%p,%p)\n",This,psb,ppdsb);
    return DirectSoundDevice_DuplicateSoundBuffer(((IDirectSoundImpl *)This->pds)->device,psb,ppdsb);
}

static HRESULT WINAPI IDirectSound8_IDirectSound8_SetCooperativeLevel(
    LPDIRECTSOUND8 iface,
    HWND hwnd,
    DWORD level)
{
    IDirectSound8_IDirectSound8 *This = (IDirectSound8_IDirectSound8 *)iface;
    TRACE("(%p,%p,%s)\n",This,hwnd,dumpCooperativeLevel(level));
    return DirectSoundDevice_SetCooperativeLevel(((IDirectSoundImpl *)This->pds)->device, hwnd, level);
}

static HRESULT WINAPI IDirectSound8_IDirectSound8_Compact(
    LPDIRECTSOUND8 iface)
{
    IDirectSound8_IDirectSound8 *This = (IDirectSound8_IDirectSound8 *)iface;
    TRACE("(%p)\n", This);
    return DirectSoundDevice_Compact(((IDirectSoundImpl *)This->pds)->device);
}

static HRESULT WINAPI IDirectSound8_IDirectSound8_GetSpeakerConfig(
    LPDIRECTSOUND8 iface,
    LPDWORD lpdwSpeakerConfig)
{
    IDirectSound8_IDirectSound8 *This = (IDirectSound8_IDirectSound8 *)iface;
    TRACE("(%p, %p)\n", This, lpdwSpeakerConfig);
    return DirectSoundDevice_GetSpeakerConfig(((IDirectSoundImpl *)This->pds)->device,lpdwSpeakerConfig);
}

static HRESULT WINAPI IDirectSound8_IDirectSound8_SetSpeakerConfig(
    LPDIRECTSOUND8 iface,
    DWORD config)
{
    IDirectSound8_IDirectSound8 *This = (IDirectSound8_IDirectSound8 *)iface;
    TRACE("(%p,0x%08x)\n",This,config);
    return DirectSoundDevice_SetSpeakerConfig(((IDirectSoundImpl *)This->pds)->device,config);
}

static HRESULT WINAPI IDirectSound8_IDirectSound8_Initialize(
    LPDIRECTSOUND8 iface,
    LPCGUID lpcGuid)
{
    IDirectSound8_IDirectSound8 *This = (IDirectSound8_IDirectSound8 *)iface;
    TRACE("(%p, %s)\n", This, debugstr_guid(lpcGuid));
    return DirectSoundDevice_Initialize(&((IDirectSoundImpl *)This->pds)->device,lpcGuid);
}

static HRESULT WINAPI IDirectSound8_IDirectSound8_VerifyCertification(
    LPDIRECTSOUND8 iface,
    LPDWORD pdwCertified)
{
    IDirectSound8_IDirectSound8 *This = (IDirectSound8_IDirectSound8 *)iface;
    TRACE("(%p, %p)\n", This, pdwCertified);
    return DirectSoundDevice_VerifyCertification(((IDirectSoundImpl *)This->pds)->device,pdwCertified);
}

static const IDirectSound8Vtbl DirectSound8_DirectSound8_Vtbl =
{
    IDirectSound8_IDirectSound8_QueryInterface,
    IDirectSound8_IDirectSound8_AddRef,
    IDirectSound8_IDirectSound8_Release,
    IDirectSound8_IDirectSound8_CreateSoundBuffer,
    IDirectSound8_IDirectSound8_GetCaps,
    IDirectSound8_IDirectSound8_DuplicateSoundBuffer,
    IDirectSound8_IDirectSound8_SetCooperativeLevel,
    IDirectSound8_IDirectSound8_Compact,
    IDirectSound8_IDirectSound8_GetSpeakerConfig,
    IDirectSound8_IDirectSound8_SetSpeakerConfig,
    IDirectSound8_IDirectSound8_Initialize,
    IDirectSound8_IDirectSound8_VerifyCertification
};

static HRESULT IDirectSound8_IDirectSound8_Create(
    LPDIRECTSOUND8 pds,
    LPDIRECTSOUND8 * ppds)
{
    IDirectSound8_IDirectSound8 * pdsds;
    TRACE("(%p,%p)\n",pds,ppds);

    if (ppds == NULL) {
        ERR("invalid parameter: ppds == NULL\n");
        return DSERR_INVALIDPARAM;
    }

    if (pds == NULL) {
        ERR("invalid parameter: pds == NULL\n");
        *ppds = NULL;
        return DSERR_INVALIDPARAM;
    }

    pdsds = HeapAlloc(GetProcessHeap(),0,sizeof(*pdsds));
    if (pdsds == NULL) {
        WARN("out of memory\n");
        *ppds = NULL;
        return DSERR_OUTOFMEMORY;
    }

    pdsds->lpVtbl = &DirectSound8_DirectSound8_Vtbl;
    pdsds->ref = 0;
    pdsds->pds = pds;

    IDirectSoundImpl_AddRef(pds);
    *ppds = (LPDIRECTSOUND8)pdsds;

    return DS_OK;
}

HRESULT DSOUND_Create(
    REFIID riid,
    LPDIRECTSOUND *ppDS)
{
    LPDIRECTSOUND8 pDS;
    HRESULT hr;
    TRACE("(%s, %p)\n", debugstr_guid(riid), ppDS);

    if (!IsEqualIID(riid, &IID_IUnknown) &&
        !IsEqualIID(riid, &IID_IDirectSound)) {
        *ppDS = 0;
        return E_NOINTERFACE;
    }

    /* Get dsound configuration */
    setup_dsound_options();

    hr = IDirectSoundImpl_Create(&pDS);
    if (hr == DS_OK) {
        hr = IDirectSound_IDirectSound_Create(pDS, ppDS);
        if (*ppDS)
            IDirectSound_IDirectSound_AddRef(*ppDS);
        else {
            WARN("IDirectSound_IDirectSound_Create failed\n");
            IDirectSound8_Release(pDS);
        }
    } else {
        WARN("IDirectSoundImpl_Create failed\n");
        *ppDS = 0;
    }

    return hr;
}

/*******************************************************************************
 *		DirectSoundCreate (DSOUND.1)
 *
 *  Creates and initializes a DirectSound interface.
 *
 *  PARAMS
 *     lpcGUID   [I] Address of the GUID that identifies the sound device.
 *     ppDS      [O] Address of a variable to receive the interface pointer.
 *     pUnkOuter [I] Must be NULL.
 *
 *  RETURNS
 *     Success: DS_OK
 *     Failure: DSERR_ALLOCATED, DSERR_INVALIDPARAM, DSERR_NOAGGREGATION,
 *              DSERR_NODRIVER, DSERR_OUTOFMEMORY
 */
HRESULT WINAPI DirectSoundCreate(
    LPCGUID lpcGUID,
    LPDIRECTSOUND *ppDS,
    IUnknown *pUnkOuter)
{
    HRESULT hr;
    LPDIRECTSOUND pDS;

    TRACE("(%s,%p,%p)\n",debugstr_guid(lpcGUID),ppDS,pUnkOuter);

    if (ppDS == NULL) {
        WARN("invalid parameter: ppDS == NULL\n");
        return DSERR_INVALIDPARAM;
    }

    if (pUnkOuter != NULL) {
        WARN("invalid parameter: pUnkOuter != NULL\n");
        *ppDS = 0;
        return DSERR_INVALIDPARAM;
    }

    hr = DSOUND_Create(&IID_IDirectSound, &pDS);
    if (hr == DS_OK) {
        hr = IDirectSound_Initialize(pDS, lpcGUID);
        if (hr != DS_OK) {
            if (hr != DSERR_ALREADYINITIALIZED) {
                IDirectSound_Release(pDS);
                pDS = 0;
            } else
                hr = DS_OK;
        }
    }

    *ppDS = pDS;

    return hr;
}

HRESULT DSOUND_Create8(
    REFIID riid,
    LPDIRECTSOUND8 *ppDS)
{
    LPDIRECTSOUND8 pDS;
    HRESULT hr;
    TRACE("(%s, %p)\n", debugstr_guid(riid), ppDS);

    if (!IsEqualIID(riid, &IID_IUnknown) &&
        !IsEqualIID(riid, &IID_IDirectSound) &&
        !IsEqualIID(riid, &IID_IDirectSound8)) {
        *ppDS = 0;
        return E_NOINTERFACE;
    }

    /* Get dsound configuration */
    setup_dsound_options();

    hr = IDirectSoundImpl_Create(&pDS);
    if (hr == DS_OK) {
        hr = IDirectSound8_IDirectSound8_Create(pDS, ppDS);
        if (*ppDS)
            IDirectSound8_IDirectSound8_AddRef(*ppDS);
        else {
            WARN("IDirectSound8_IDirectSound8_Create failed\n");
            IDirectSound8_Release(pDS);
        }
    } else {
        WARN("IDirectSoundImpl_Create failed\n");
        *ppDS = 0;
    }

    return hr;
}

/*******************************************************************************
 *        DirectSoundCreate8 (DSOUND.11)
 *
 *  Creates and initializes a DirectSound8 interface.
 *
 *  PARAMS
 *     lpcGUID   [I] Address of the GUID that identifies the sound device.
 *     ppDS      [O] Address of a variable to receive the interface pointer.
 *     pUnkOuter [I] Must be NULL.
 *
 *  RETURNS
 *     Success: DS_OK
 *     Failure: DSERR_ALLOCATED, DSERR_INVALIDPARAM, DSERR_NOAGGREGATION,
 *              DSERR_NODRIVER, DSERR_OUTOFMEMORY
 */
HRESULT WINAPI DirectSoundCreate8(
    LPCGUID lpcGUID,
    LPDIRECTSOUND8 *ppDS,
    IUnknown *pUnkOuter)
{
    HRESULT hr;
    LPDIRECTSOUND8 pDS;

    TRACE("(%s,%p,%p)\n",debugstr_guid(lpcGUID),ppDS,pUnkOuter);

    if (ppDS == NULL) {
        WARN("invalid parameter: ppDS == NULL\n");
        return DSERR_INVALIDPARAM;
    }

    if (pUnkOuter != NULL) {
        WARN("invalid parameter: pUnkOuter != NULL\n");
        *ppDS = 0;
        return DSERR_INVALIDPARAM;
    }

    hr = DSOUND_Create8(&IID_IDirectSound8, &pDS);
    if (hr == DS_OK) {
        hr = IDirectSound8_Initialize(pDS, lpcGUID);
        if (hr != DS_OK) {
            if (hr != DSERR_ALREADYINITIALIZED) {
                IDirectSound8_Release(pDS);
                pDS = 0;
            } else
                hr = DS_OK;
        }
    }

    *ppDS = pDS;

    return hr;
}

/*******************************************************************************
 *        DirectSoundDevice
 */
static HRESULT DirectSoundDevice_Create(DirectSoundDevice ** ppDevice)
{
    DirectSoundDevice * device;
    TRACE("(%p)\n", ppDevice);

    /* Allocate memory */
    device = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(DirectSoundDevice));
    if (device == NULL) {
        WARN("out of memory\n");
        return DSERR_OUTOFMEMORY;
    }

    device->ref            = 1;
    device->priolevel      = DSSCL_NORMAL;
    device->state          = STATE_STOPPED;
    device->speaker_config = DSSPEAKER_STEREO | (DSSPEAKER_GEOMETRY_NARROW << 16);

    /* 3D listener initial parameters */
    device->ds3dl.dwSize   = sizeof(DS3DLISTENER);
    device->ds3dl.vPosition.x = 0.0;
    device->ds3dl.vPosition.y = 0.0;
    device->ds3dl.vPosition.z = 0.0;
    device->ds3dl.vVelocity.x = 0.0;
    device->ds3dl.vVelocity.y = 0.0;
    device->ds3dl.vVelocity.z = 0.0;
    device->ds3dl.vOrientFront.x = 0.0;
    device->ds3dl.vOrientFront.y = 0.0;
    device->ds3dl.vOrientFront.z = 1.0;
    device->ds3dl.vOrientTop.x = 0.0;
    device->ds3dl.vOrientTop.y = 1.0;
    device->ds3dl.vOrientTop.z = 0.0;
    device->ds3dl.flDistanceFactor = DS3D_DEFAULTDISTANCEFACTOR;
    device->ds3dl.flRolloffFactor = DS3D_DEFAULTROLLOFFFACTOR;
    device->ds3dl.flDopplerFactor = DS3D_DEFAULTDOPPLERFACTOR;

    device->prebuf = ds_snd_queue_max;
    device->guid = GUID_NULL;

    /* Set default wave format (may need it for waveOutOpen) */
    device->pwfx = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(WAVEFORMATEX));
    if (device->pwfx == NULL) {
        WARN("out of memory\n");
        HeapFree(GetProcessHeap(),0,device);
        return DSERR_OUTOFMEMORY;
    }

    /* We rely on the sound driver to return the actual sound format of
     * the device if it does not support 22050x8x2 and is given the
     * WAVE_DIRECTSOUND flag.
     */
    device->pwfx->wFormatTag = WAVE_FORMAT_PCM;
    device->pwfx->nSamplesPerSec = ds_default_sample_rate;
    device->pwfx->wBitsPerSample = ds_default_bits_per_sample;
    device->pwfx->nChannels = 2;
    device->pwfx->nBlockAlign = device->pwfx->wBitsPerSample * device->pwfx->nChannels / 8;
    device->pwfx->nAvgBytesPerSec = device->pwfx->nSamplesPerSec * device->pwfx->nBlockAlign;
    device->pwfx->cbSize = 0;

    InitializeCriticalSection(&(device->mixlock));
    device->mixlock.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": DirectSoundDevice.mixlock");

    RtlInitializeResource(&(device->buffer_list_lock));

   *ppDevice = device;

    return DS_OK;
}

static ULONG DirectSoundDevice_AddRef(DirectSoundDevice * device)
{
    ULONG ref = InterlockedIncrement(&(device->ref));
    TRACE("(%p) ref was %d\n", device, ref - 1);
    return ref;
}

ULONG DirectSoundDevice_Release(DirectSoundDevice * device)
{
    HRESULT hr;
    ULONG ref = InterlockedDecrement(&(device->ref));
    TRACE("(%p) ref was %u\n", device, ref + 1);
    if (!ref) {
        int i;
        timeKillEvent(device->timerID);
        timeEndPeriod(DS_TIME_RES);

        /* The kill event should have allowed the timer process to expire
         * but try to grab the lock just in case. Can't hold lock because
         * IDirectSoundBufferImpl_Destroy also grabs the lock */
        RtlAcquireResourceShared(&(device->buffer_list_lock), TRUE);
        RtlReleaseResource(&(device->buffer_list_lock));

        /* It is allowed to release this object even when buffers are playing */
        if (device->buffers) {
            WARN("%d secondary buffers not released\n", device->nrofbuffers);
            for( i=0;i<device->nrofbuffers;i++)
                IDirectSoundBufferImpl_Destroy(device->buffers[i]);
        }

        if (device->primary) {
            WARN("primary buffer not released\n");
            IDirectSoundBuffer8_Release((LPDIRECTSOUNDBUFFER8)device->primary);
        }

        hr = DSOUND_PrimaryDestroy(device);
        if (hr != DS_OK)
            WARN("DSOUND_PrimaryDestroy failed\n");

        if (device->driver)
            IDsDriver_Close(device->driver);

        if (device->drvdesc.dwFlags & DSDDESC_DOMMSYSTEMOPEN)
            waveOutClose(device->hwo);

        if (device->driver)
            IDsDriver_Release(device->driver);

        DSOUND_renderer[device->drvdesc.dnDevNode] = NULL;

        HeapFree(GetProcessHeap(), 0, device->tmp_buffer);
        HeapFree(GetProcessHeap(), 0, device->mix_buffer);
        if (device->drvdesc.dwFlags & DSDDESC_USESYSTEMMEMORY)
            HeapFree(GetProcessHeap(), 0, device->buffer);
        RtlDeleteResource(&device->buffer_list_lock);
        device->mixlock.DebugInfo->Spare[0] = 0;
        DeleteCriticalSection(&device->mixlock);
        HeapFree(GetProcessHeap(),0,device);
        TRACE("(%p) released\n", device);
    }
    return ref;
}

HRESULT DirectSoundDevice_GetCaps(
    DirectSoundDevice * device,
    LPDSCAPS lpDSCaps)
{
    TRACE("(%p,%p)\n",device,lpDSCaps);

    if (device == NULL) {
        WARN("not initialized\n");
        return DSERR_UNINITIALIZED;
    }

    if (lpDSCaps == NULL) {
        WARN("invalid parameter: lpDSCaps = NULL\n");
        return DSERR_INVALIDPARAM;
    }

    /* check if there is enough room */
    if (lpDSCaps->dwSize < sizeof(*lpDSCaps)) {
        WARN("invalid parameter: lpDSCaps->dwSize = %d\n", lpDSCaps->dwSize);
        return DSERR_INVALIDPARAM;
    }

    lpDSCaps->dwFlags                           = device->drvcaps.dwFlags;
    if (TRACE_ON(dsound)) {
        TRACE("(flags=0x%08x:\n",lpDSCaps->dwFlags);
        _dump_DSCAPS(lpDSCaps->dwFlags);
        TRACE(")\n");
    }
    lpDSCaps->dwMinSecondarySampleRate          = device->drvcaps.dwMinSecondarySampleRate;
    lpDSCaps->dwMaxSecondarySampleRate          = device->drvcaps.dwMaxSecondarySampleRate;
    lpDSCaps->dwPrimaryBuffers                  = device->drvcaps.dwPrimaryBuffers;
    lpDSCaps->dwMaxHwMixingAllBuffers           = device->drvcaps.dwMaxHwMixingAllBuffers;
    lpDSCaps->dwMaxHwMixingStaticBuffers        = device->drvcaps.dwMaxHwMixingStaticBuffers;
    lpDSCaps->dwMaxHwMixingStreamingBuffers     = device->drvcaps.dwMaxHwMixingStreamingBuffers;
    lpDSCaps->dwFreeHwMixingAllBuffers          = device->drvcaps.dwFreeHwMixingAllBuffers;
    lpDSCaps->dwFreeHwMixingStaticBuffers       = device->drvcaps.dwFreeHwMixingStaticBuffers;
    lpDSCaps->dwFreeHwMixingStreamingBuffers    = device->drvcaps.dwFreeHwMixingStreamingBuffers;
    lpDSCaps->dwMaxHw3DAllBuffers               = device->drvcaps.dwMaxHw3DAllBuffers;
    lpDSCaps->dwMaxHw3DStaticBuffers            = device->drvcaps.dwMaxHw3DStaticBuffers;
    lpDSCaps->dwMaxHw3DStreamingBuffers         = device->drvcaps.dwMaxHw3DStreamingBuffers;
    lpDSCaps->dwFreeHw3DAllBuffers              = device->drvcaps.dwFreeHw3DAllBuffers;
    lpDSCaps->dwFreeHw3DStaticBuffers           = device->drvcaps.dwFreeHw3DStaticBuffers;
    lpDSCaps->dwFreeHw3DStreamingBuffers        = device->drvcaps.dwFreeHw3DStreamingBuffers;
    lpDSCaps->dwTotalHwMemBytes                 = device->drvcaps.dwTotalHwMemBytes;
    lpDSCaps->dwFreeHwMemBytes                  = device->drvcaps.dwFreeHwMemBytes;
    lpDSCaps->dwMaxContigFreeHwMemBytes         = device->drvcaps.dwMaxContigFreeHwMemBytes;

    /* driver doesn't have these */
    lpDSCaps->dwUnlockTransferRateHwBuffers     = 4096; /* But we have none... */
    lpDSCaps->dwPlayCpuOverheadSwBuffers        = 1;    /* 1% */

    return DS_OK;
}

HRESULT DirectSoundDevice_Initialize(DirectSoundDevice ** ppDevice, LPCGUID lpcGUID)
{
    HRESULT hr = DS_OK;
    unsigned wod, wodn;
    BOOLEAN found = FALSE;
    GUID devGUID;
    DirectSoundDevice * device = *ppDevice;
    TRACE("(%p,%s)\n",ppDevice,debugstr_guid(lpcGUID));

    if (*ppDevice != NULL) {
        WARN("already initialized\n");
        return DSERR_ALREADYINITIALIZED;
    }

    /* Default device? */
    if (!lpcGUID || IsEqualGUID(lpcGUID, &GUID_NULL))
        lpcGUID = &DSDEVID_DefaultPlayback;

    if (GetDeviceID(lpcGUID, &devGUID) != DS_OK) {
        WARN("invalid parameter: lpcGUID\n");
        return DSERR_INVALIDPARAM;
    }

    /* Enumerate WINMM audio devices and find the one we want */
    wodn = waveOutGetNumDevs();
    if (!wodn) {
        WARN("no driver\n");
        return DSERR_NODRIVER;
    }

    for (wod=0; wod<wodn; wod++) {
        if (IsEqualGUID( &devGUID, &DSOUND_renderer_guids[wod])) {
            found = TRUE;
            break;
        }
    }

    if (found == FALSE) {
        WARN("No device found matching given ID!\n");
        return DSERR_NODRIVER;
    }

    if (DSOUND_renderer[wod]) {
        if (IsEqualGUID(&devGUID, &DSOUND_renderer[wod]->guid)) {
            device = DSOUND_renderer[wod];
            DirectSoundDevice_AddRef(device);
            *ppDevice = device;
            return DS_OK;
        } else {
            ERR("device GUID doesn't match\n");
            hr = DSERR_GENERIC;
            return hr;
        }
    } else {
        hr = DirectSoundDevice_Create(&device);
        if (hr != DS_OK) {
            WARN("DirectSoundDevice_Create failed\n");
            return hr;
        }
    }

    *ppDevice = device;
    device->guid = devGUID;
    device->driver = NULL;

    device->drvdesc.dnDevNode = wod;
    hr = DSOUND_ReopenDevice(device, FALSE);
    if (FAILED(hr))
    {
        WARN("DSOUND_ReopenDevice failed: %08x\n", hr);
        return hr;
    }

    if (device->driver) {
        /* the driver is now open, so it's now allowed to call GetCaps */
        hr = IDsDriver_GetCaps(device->driver,&(device->drvcaps));
        if (hr != DS_OK) {
            WARN("IDsDriver_GetCaps failed\n");
            return hr;
        }
    } else {
        WAVEOUTCAPSA woc;
        hr = mmErr(waveOutGetDevCapsA(device->drvdesc.dnDevNode, &woc, sizeof(woc)));
        if (hr != DS_OK) {
            WARN("waveOutGetDevCaps failed\n");
            return hr;
        }
        ZeroMemory(&device->drvcaps, sizeof(device->drvcaps));
        if ((woc.dwFormats & WAVE_FORMAT_1M08) ||
            (woc.dwFormats & WAVE_FORMAT_2M08) ||
            (woc.dwFormats & WAVE_FORMAT_4M08) ||
            (woc.dwFormats & WAVE_FORMAT_48M08) ||
            (woc.dwFormats & WAVE_FORMAT_96M08)) {
            device->drvcaps.dwFlags |= DSCAPS_PRIMARY8BIT;
            device->drvcaps.dwFlags |= DSCAPS_PRIMARYMONO;
        }
        if ((woc.dwFormats & WAVE_FORMAT_1M16) ||
            (woc.dwFormats & WAVE_FORMAT_2M16) ||
            (woc.dwFormats & WAVE_FORMAT_4M16) ||
            (woc.dwFormats & WAVE_FORMAT_48M16) ||
            (woc.dwFormats & WAVE_FORMAT_96M16)) {
            device->drvcaps.dwFlags |= DSCAPS_PRIMARY16BIT;
            device->drvcaps.dwFlags |= DSCAPS_PRIMARYMONO;
        }
        if ((woc.dwFormats & WAVE_FORMAT_1S08) ||
            (woc.dwFormats & WAVE_FORMAT_2S08) ||
            (woc.dwFormats & WAVE_FORMAT_4S08) ||
            (woc.dwFormats & WAVE_FORMAT_48S08) ||
            (woc.dwFormats & WAVE_FORMAT_96S08)) {
            device->drvcaps.dwFlags |= DSCAPS_PRIMARY8BIT;
            device->drvcaps.dwFlags |= DSCAPS_PRIMARYSTEREO;
        }
        if ((woc.dwFormats & WAVE_FORMAT_1S16) ||
            (woc.dwFormats & WAVE_FORMAT_2S16) ||
            (woc.dwFormats & WAVE_FORMAT_4S16) ||
            (woc.dwFormats & WAVE_FORMAT_48S16) ||
            (woc.dwFormats & WAVE_FORMAT_96S16)) {
            device->drvcaps.dwFlags |= DSCAPS_PRIMARY16BIT;
            device->drvcaps.dwFlags |= DSCAPS_PRIMARYSTEREO;
        }
        if (ds_emuldriver)
            device->drvcaps.dwFlags |= DSCAPS_EMULDRIVER;
        device->drvcaps.dwMinSecondarySampleRate = DSBFREQUENCY_MIN;
        device->drvcaps.dwMaxSecondarySampleRate = DSBFREQUENCY_MAX;
        ZeroMemory(&device->volpan, sizeof(device->volpan));
    }

    hr = DSOUND_PrimaryCreate(device);
    if (hr == DS_OK) {
        UINT triggertime = DS_TIME_DEL, res = DS_TIME_RES, id;
        TIMECAPS time;

        DSOUND_renderer[device->drvdesc.dnDevNode] = device;
        timeGetDevCaps(&time, sizeof(TIMECAPS));
        TRACE("Minimum timer resolution: %u, max timer: %u\n", time.wPeriodMin, time.wPeriodMax);
        if (triggertime < time.wPeriodMin)
            triggertime = time.wPeriodMin;
        if (res < time.wPeriodMin)
            res = time.wPeriodMin;
        if (timeBeginPeriod(res) == TIMERR_NOCANDO)
            WARN("Could not set minimum resolution, don't expect sound\n");
        id = timeSetEvent(triggertime, res, DSOUND_timer, (DWORD_PTR)device, TIME_PERIODIC | TIME_KILL_SYNCHRONOUS);
        if (!id)
        {
            WARN("Timer not created! Retrying without TIME_KILL_SYNCHRONOUS\n");
            id = timeSetEvent(triggertime, res, DSOUND_timer, (DWORD_PTR)device, TIME_PERIODIC);
            if (!id) ERR("Could not create timer, sound playback will not occur\n");
        }
        DSOUND_renderer[device->drvdesc.dnDevNode]->timerID = id;
    } else {
        WARN("DSOUND_PrimaryCreate failed\n");
    }

    return hr;
}

HRESULT DirectSoundDevice_CreateSoundBuffer(
    DirectSoundDevice * device,
    LPCDSBUFFERDESC dsbd,
    LPLPDIRECTSOUNDBUFFER ppdsb,
    LPUNKNOWN lpunk,
    BOOL from8)
{
    HRESULT hres = DS_OK;
    TRACE("(%p,%p,%p,%p)\n",device,dsbd,ppdsb,lpunk);

    if (device == NULL) {
        WARN("not initialized\n");
        return DSERR_UNINITIALIZED;
    }

    if (dsbd == NULL) {
        WARN("invalid parameter: dsbd == NULL\n");
        return DSERR_INVALIDPARAM;
    }

    if (dsbd->dwSize != sizeof(DSBUFFERDESC) &&
        dsbd->dwSize != sizeof(DSBUFFERDESC1)) {
        WARN("invalid parameter: dsbd\n");
        return DSERR_INVALIDPARAM;
    }

    if (ppdsb == NULL) {
        WARN("invalid parameter: ppdsb == NULL\n");
        return DSERR_INVALIDPARAM;
    }
    *ppdsb = NULL;

    if (TRACE_ON(dsound)) {
        TRACE("(structsize=%d)\n",dsbd->dwSize);
        TRACE("(flags=0x%08x:\n",dsbd->dwFlags);
        _dump_DSBCAPS(dsbd->dwFlags);
        TRACE(")\n");
        TRACE("(bufferbytes=%d)\n",dsbd->dwBufferBytes);
        TRACE("(lpwfxFormat=%p)\n",dsbd->lpwfxFormat);
    }

    if (dsbd->dwFlags & DSBCAPS_PRIMARYBUFFER) {
        if (dsbd->lpwfxFormat != NULL) {
            WARN("invalid parameter: dsbd->lpwfxFormat must be NULL for "
                 "primary buffer\n");
            return DSERR_INVALIDPARAM;
        }

        if (device->primary) {
            WARN("Primary Buffer already created\n");
            IDirectSoundBuffer_AddRef((LPDIRECTSOUNDBUFFER8)(device->primary));
            *ppdsb = (LPDIRECTSOUNDBUFFER)(device->primary);
        } else {
           device->dsbd = *dsbd;
           device->dsbd.dwFlags &= ~(DSBCAPS_LOCHARDWARE | DSBCAPS_LOCSOFTWARE);
           if (device->hwbuf)
               device->dsbd.dwFlags |= DSBCAPS_LOCHARDWARE;
           else device->dsbd.dwFlags |= DSBCAPS_LOCSOFTWARE;
           hres = PrimaryBufferImpl_Create(device, &(device->primary), &(device->dsbd));
           if (device->primary) {
               IDirectSoundBuffer_AddRef((LPDIRECTSOUNDBUFFER8)(device->primary));
               *ppdsb = (LPDIRECTSOUNDBUFFER)(device->primary);
           } else
               WARN("PrimaryBufferImpl_Create failed\n");
        }
    } else {
        IDirectSoundBufferImpl * dsb;
        WAVEFORMATEXTENSIBLE *pwfxe;

        if (dsbd->lpwfxFormat == NULL) {
            WARN("invalid parameter: dsbd->lpwfxFormat can't be NULL for "
                 "secondary buffer\n");
            return DSERR_INVALIDPARAM;
        }
        pwfxe = (WAVEFORMATEXTENSIBLE*)dsbd->lpwfxFormat;

        if (pwfxe->Format.wBitsPerSample != 16 && pwfxe->Format.wBitsPerSample != 8 && pwfxe->Format.wFormatTag != WAVE_FORMAT_EXTENSIBLE)
        {
            WARN("wBitsPerSample=%d needs a WAVEFORMATEXTENSIBLE\n", dsbd->lpwfxFormat->wBitsPerSample);
            return DSERR_CONTROLUNAVAIL;
        }
        if (pwfxe->Format.wFormatTag == WAVE_FORMAT_EXTENSIBLE)
        {
            if (pwfxe->Format.cbSize < (sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX)))
            {
                WARN("Too small a cbSize %u\n", pwfxe->Format.cbSize);
                return DSERR_INVALIDPARAM;
            }

            if (pwfxe->Format.cbSize > (sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX)))
            {
                WARN("Too big a cbSize %u\n", pwfxe->Format.cbSize);
                return DSERR_CONTROLUNAVAIL;
            }

            if (!IsEqualGUID(&pwfxe->SubFormat, &KSDATAFORMAT_SUBTYPE_PCM))
            {
                if (!IsEqualGUID(&pwfxe->SubFormat, &GUID_NULL))
                    FIXME("SubFormat %s not supported right now.\n", debugstr_guid(&pwfxe->SubFormat));
                return DSERR_INVALIDPARAM;
            }
            if (pwfxe->Samples.wValidBitsPerSample > dsbd->lpwfxFormat->wBitsPerSample)
            {
                WARN("Samples.wValidBitsPerSample(%d) > Format.wBitsPerSample (%d)\n", pwfxe->Samples.wValidBitsPerSample, pwfxe->Format.wBitsPerSample);
                return DSERR_INVALIDPARAM;
            }
            if (pwfxe->Samples.wValidBitsPerSample && pwfxe->Samples.wValidBitsPerSample < dsbd->lpwfxFormat->wBitsPerSample)
            {
                FIXME("Non-packed formats not supported right now: %d/%d\n", pwfxe->Samples.wValidBitsPerSample, dsbd->lpwfxFormat->wBitsPerSample);
                return DSERR_CONTROLUNAVAIL;
            }
        }

        TRACE("(formattag=0x%04x,chans=%d,samplerate=%d,"
              "bytespersec=%d,blockalign=%d,bitspersamp=%d,cbSize=%d)\n",
              dsbd->lpwfxFormat->wFormatTag, dsbd->lpwfxFormat->nChannels,
              dsbd->lpwfxFormat->nSamplesPerSec,
              dsbd->lpwfxFormat->nAvgBytesPerSec,
              dsbd->lpwfxFormat->nBlockAlign,
              dsbd->lpwfxFormat->wBitsPerSample, dsbd->lpwfxFormat->cbSize);

        if (from8 && (dsbd->dwFlags & DSBCAPS_CTRL3D) && (dsbd->lpwfxFormat->nChannels != 1)) {
            WARN("invalid parameter: 3D buffer format must be mono\n");
            return DSERR_INVALIDPARAM;
        }

        hres = IDirectSoundBufferImpl_Create(device, &dsb, dsbd);
        if (dsb) {
            hres = SecondaryBufferImpl_Create(dsb, (SecondaryBufferImpl**)ppdsb);
            if (*ppdsb) {
                dsb->secondary = (SecondaryBufferImpl*)*ppdsb;
                IDirectSoundBuffer_AddRef(*ppdsb);
            } else
                WARN("SecondaryBufferImpl_Create failed\n");
        } else
           WARN("IDirectSoundBufferImpl_Create failed\n");
   }

   return hres;
}

HRESULT DirectSoundDevice_DuplicateSoundBuffer(
    DirectSoundDevice * device,
    LPDIRECTSOUNDBUFFER psb,
    LPLPDIRECTSOUNDBUFFER ppdsb)
{
    HRESULT hres = DS_OK;
    IDirectSoundBufferImpl* dsb;
    TRACE("(%p,%p,%p)\n",device,psb,ppdsb);

    if (device == NULL) {
        WARN("not initialized\n");
        return DSERR_UNINITIALIZED;
    }

    if (psb == NULL) {
        WARN("invalid parameter: psb == NULL\n");
        return DSERR_INVALIDPARAM;
    }

    if (ppdsb == NULL) {
        WARN("invalid parameter: ppdsb == NULL\n");
        return DSERR_INVALIDPARAM;
    }

    /* make sure we have a secondary buffer */
    if ((PrimaryBufferImpl *)psb == device->primary) {
        WARN("trying to duplicate primary buffer\n");
        *ppdsb = NULL;
        return DSERR_INVALIDCALL;
    }

    /* duplicate the actual buffer implementation */
    hres = IDirectSoundBufferImpl_Duplicate(device, &dsb,
                                           ((SecondaryBufferImpl *)psb)->dsb);

    if (hres == DS_OK) {
        /* create a new secondary buffer using the new implementation */
        hres = SecondaryBufferImpl_Create(dsb, (SecondaryBufferImpl**)ppdsb);
        if (*ppdsb) {
            dsb->secondary = (SecondaryBufferImpl*)*ppdsb;
            IDirectSoundBuffer_AddRef((LPDIRECTSOUNDBUFFER8)*ppdsb);
        } else {
            WARN("SecondaryBufferImpl_Create failed\n");
            IDirectSoundBuffer_AddRef((LPDIRECTSOUNDBUFFER8)dsb);
            IDirectSoundBuffer_Release((LPDIRECTSOUNDBUFFER8)dsb);
        }
    }

    return hres;
}

HRESULT DirectSoundDevice_SetCooperativeLevel(
    DirectSoundDevice * device,
    HWND hwnd,
    DWORD level)
{
    TRACE("(%p,%p,%s)\n",device,hwnd,dumpCooperativeLevel(level));

    if (device == NULL) {
        WARN("not initialized\n");
        return DSERR_UNINITIALIZED;
    }

    if (level==DSSCL_PRIORITY || level==DSSCL_EXCLUSIVE) {
        WARN("level=%s not fully supported\n",
             level==DSSCL_PRIORITY ? "DSSCL_PRIORITY" : "DSSCL_EXCLUSIVE");
    }

    device->priolevel = level;
    return DS_OK;
}

HRESULT DirectSoundDevice_Compact(
    DirectSoundDevice * device)
{
    TRACE("(%p)\n", device);

    if (device == NULL) {
        WARN("not initialized\n");
        return DSERR_UNINITIALIZED;
    }

    if (device->priolevel < DSSCL_PRIORITY) {
        WARN("incorrect priority level\n");
        return DSERR_PRIOLEVELNEEDED;
    }

    return DS_OK;
}

HRESULT DirectSoundDevice_GetSpeakerConfig(
    DirectSoundDevice * device,
    LPDWORD lpdwSpeakerConfig)
{
    TRACE("(%p, %p)\n", device, lpdwSpeakerConfig);

    if (device == NULL) {
        WARN("not initialized\n");
        return DSERR_UNINITIALIZED;
    }

    if (lpdwSpeakerConfig == NULL) {
        WARN("invalid parameter: lpdwSpeakerConfig == NULL\n");
        return DSERR_INVALIDPARAM;
    }

    WARN("not fully functional\n");
    *lpdwSpeakerConfig = device->speaker_config;
    return DS_OK;
}

HRESULT DirectSoundDevice_SetSpeakerConfig(
    DirectSoundDevice * device,
    DWORD config)
{
    TRACE("(%p,0x%08x)\n",device,config);

    if (device == NULL) {
        WARN("not initialized\n");
        return DSERR_UNINITIALIZED;
    }

    device->speaker_config = config;
    WARN("not fully functional\n");
    return DS_OK;
}

static HRESULT DirectSoundDevice_VerifyCertification(
    DirectSoundDevice * device,
    LPDWORD pdwCertified)
{
    TRACE("(%p, %p)\n",device,pdwCertified);

    if (device == NULL) {
        WARN("not initialized\n");
        return DSERR_UNINITIALIZED;
    }

    if (device->drvcaps.dwFlags & DSCAPS_CERTIFIED)
        *pdwCertified = DS_CERTIFIED;
    else
        *pdwCertified = DS_UNCERTIFIED;

    return DS_OK;
}

/*
 * Add secondary buffer to buffer list.
 * Gets exclusive access to buffer for writing.
 */
HRESULT DirectSoundDevice_AddBuffer(
    DirectSoundDevice * device,
    IDirectSoundBufferImpl * pDSB)
{
    IDirectSoundBufferImpl **newbuffers;
    HRESULT hr = DS_OK;

    TRACE("(%p, %p)\n", device, pDSB);

    RtlAcquireResourceExclusive(&(device->buffer_list_lock), TRUE);

    if (device->buffers)
        newbuffers = HeapReAlloc(GetProcessHeap(),0,device->buffers,sizeof(IDirectSoundBufferImpl*)*(device->nrofbuffers+1));
    else
        newbuffers = HeapAlloc(GetProcessHeap(),0,sizeof(IDirectSoundBufferImpl*)*(device->nrofbuffers+1));

    if (newbuffers) {
        device->buffers = newbuffers;
        device->buffers[device->nrofbuffers] = pDSB;
        device->nrofbuffers++;
        TRACE("buffer count is now %d\n", device->nrofbuffers);
    } else {
        ERR("out of memory for buffer list! Current buffer count is %d\n", device->nrofbuffers);
        hr = DSERR_OUTOFMEMORY;
    }

    RtlReleaseResource(&(device->buffer_list_lock));

    return hr;
}

/*
 * Remove secondary buffer from buffer list.
 * Gets exclusive access to buffer for writing.
 */
HRESULT DirectSoundDevice_RemoveBuffer(
    DirectSoundDevice * device,
    IDirectSoundBufferImpl * pDSB)
{
    int i;
    HRESULT hr = DS_OK;

    TRACE("(%p, %p)\n", device, pDSB);

    RtlAcquireResourceExclusive(&(device->buffer_list_lock), TRUE);

    for (i = 0; i < device->nrofbuffers; i++)
        if (device->buffers[i] == pDSB)
            break;

    if (i < device->nrofbuffers) {
        /* Put the last buffer of the list in the (now empty) position */
        device->buffers[i] = device->buffers[device->nrofbuffers - 1];
        device->nrofbuffers--;
        device->buffers = HeapReAlloc(GetProcessHeap(),0,device->buffers,sizeof(LPDIRECTSOUNDBUFFER8)*device->nrofbuffers);
        TRACE("buffer count is now %d\n", device->nrofbuffers);
    }

    if (device->nrofbuffers == 0) {
        HeapFree(GetProcessHeap(),0,device->buffers);
        device->buffers = NULL;
    }

    RtlReleaseResource(&(device->buffer_list_lock));

    return hr;
}
