/* DirectMusic Wave Main
 *
 * Copyright (C) 2003-2004 Rok Mandeljc
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */


#include <stdio.h>
#include <stdarg.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "winnt.h"
#include "wingdi.h"
#include "winuser.h"
#include "winreg.h"
#include "objbase.h"
#include "rpcproxy.h"
#include "initguid.h"
#include "dmusici.h"

#include "dswave_private.h"
#include "dmusic_wave.h"
#include "dmobject.h"

WINE_DEFAULT_DEBUG_CHANNEL(dswave);

typedef struct {
        IClassFactory IClassFactory_iface;
} IClassFactoryImpl;

/******************************************************************
 *		DirectMusicWave ClassFactory
 */
static HRESULT WINAPI WaveCF_QueryInterface(IClassFactory * iface, REFIID riid, void **ppv)
{
        if (ppv == NULL)
                return E_POINTER;

        if (IsEqualGUID(&IID_IUnknown, riid))
                TRACE("(%p)->(IID_IUnknown %p)\n", iface, ppv);
        else if (IsEqualGUID(&IID_IClassFactory, riid))
                TRACE("(%p)->(IID_IClassFactory %p)\n", iface, ppv);
        else {
                FIXME("(%p)->(%s %p)\n", iface, debugstr_guid(riid), ppv);
                *ppv = NULL;
                return E_NOINTERFACE;
        }

        *ppv = iface;
        IUnknown_AddRef((IUnknown*)*ppv);
        return S_OK;
}

static ULONG WINAPI WaveCF_AddRef(IClassFactory * iface)
{
	return 2; /* non-heap based object */
}

static ULONG WINAPI WaveCF_Release(IClassFactory * iface)
{
	return 1; /* non-heap based object */
}

static HRESULT WINAPI WaveCF_CreateInstance(IClassFactory * iface, IUnknown *outer_unk, REFIID riid,
        void **ret_iface)
{
    IDirectMusicObject *object;
    HRESULT hr;

    TRACE("(%p, %s, %p)\n", outer_unk, debugstr_dmguid(riid), ret_iface);

    *ret_iface = NULL;
    if (outer_unk) return CLASS_E_NOAGGREGATION;
    if (FAILED(hr = wave_create(&object))) return hr;
    hr = IDirectMusicObject_QueryInterface(object, riid, ret_iface);
    IDirectMusicObject_Release(object);
    return hr;
}

static HRESULT WINAPI WaveCF_LockServer(IClassFactory * iface, BOOL dolock)
{
	TRACE("(%d)\n", dolock);
	return S_OK;
}

static const IClassFactoryVtbl WaveCF_Vtbl = {
	WaveCF_QueryInterface,
	WaveCF_AddRef,
	WaveCF_Release,
	WaveCF_CreateInstance,
	WaveCF_LockServer
};

static IClassFactoryImpl Wave_CF = {{&WaveCF_Vtbl}};

/******************************************************************
 *		DllGetClassObject (DSWAVE.@)
 *
 *
 */
HRESULT WINAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID *ppv)
{
	TRACE("(%s, %s, %p)\n", debugstr_dmguid(rclsid), debugstr_dmguid(riid), ppv);
	if (IsEqualCLSID (rclsid, &CLSID_DirectSoundWave) && IsEqualIID (riid, &IID_IClassFactory)) {
		*ppv = &Wave_CF;
		IClassFactory_AddRef((IClassFactory*)*ppv);
		return S_OK;
	}
	
    WARN("(%s, %s, %p): no interface found.\n", debugstr_dmguid(rclsid), debugstr_dmguid(riid), ppv);
    return CLASS_E_CLASSNOTAVAILABLE;
}
