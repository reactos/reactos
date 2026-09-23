/* DirectMusic Main
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

#include "dmusic_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(dmusic);

typedef struct {
        IClassFactory IClassFactory_iface;
        HRESULT (*create_instance)(IUnknown **ret_iface);
} IClassFactoryImpl;

/******************************************************************
 *      IClassFactory implementation
 */
static inline IClassFactoryImpl *impl_from_IClassFactory(IClassFactory *iface)
{
        return CONTAINING_RECORD(iface, IClassFactoryImpl, IClassFactory_iface);
}

static HRESULT WINAPI ClassFactory_QueryInterface(IClassFactory *iface, REFIID riid, void **ppv)
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

static ULONG WINAPI ClassFactory_AddRef(IClassFactory *iface)
{
        return 2; /* non-heap based object */
}

static ULONG WINAPI ClassFactory_Release(IClassFactory *iface)
{
        return 1; /* non-heap based object */
}

static HRESULT WINAPI ClassFactory_CreateInstance(IClassFactory *iface, IUnknown *unk_outer, REFIID riid, void **ret_iface)
{
    IClassFactoryImpl *This = impl_from_IClassFactory(iface);
    IUnknown *object;
    HRESULT hr;

    TRACE("(%p, %s, %p)\n", unk_outer, debugstr_dmguid(riid), ret_iface);

    *ret_iface = NULL;
    if (unk_outer) return CLASS_E_NOAGGREGATION;
    if (SUCCEEDED(hr = This->create_instance(&object)))
    {
        hr = IUnknown_QueryInterface(object, riid, ret_iface);
        IUnknown_Release(object);
    }

    return hr;
}

static HRESULT WINAPI ClassFactory_LockServer(IClassFactory *iface, BOOL dolock)
{
        TRACE("(%d)\n", dolock);
        return S_OK;
}

static const IClassFactoryVtbl classfactory_vtbl = {
        ClassFactory_QueryInterface,
        ClassFactory_AddRef,
        ClassFactory_Release,
        ClassFactory_CreateInstance,
        ClassFactory_LockServer
};

static IClassFactoryImpl DirectMusic_CF = {{&classfactory_vtbl}, music_create};
static IClassFactoryImpl Collection_CF = {{&classfactory_vtbl}, collection_create};



/******************************************************************
 *		DllGetClassObject (DMUSIC.@)
 *
 *
 */
HRESULT WINAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID *ppv)
{
	TRACE("(%s, %s, %p)\n", debugstr_dmguid(rclsid), debugstr_dmguid(riid), ppv);
	if (IsEqualCLSID (rclsid, &CLSID_DirectMusic) && IsEqualIID (riid, &IID_IClassFactory)) {
		*ppv = &DirectMusic_CF;
		IClassFactory_AddRef((IClassFactory*)*ppv);
		return S_OK;
	} else if (IsEqualCLSID (rclsid, &CLSID_DirectMusicCollection) && IsEqualIID (riid, &IID_IClassFactory)) {
		*ppv = &Collection_CF;
		IClassFactory_AddRef((IClassFactory*)*ppv);
		return S_OK;
	}
	
    WARN("(%s, %s, %p): no interface found.\n", debugstr_dmguid(rclsid), debugstr_dmguid(riid), ppv);
    return CLASS_E_CLASSNOTAVAILABLE;
}

/******************************************************************
 *		Helper functions
 *
 *
 */
/* dwPatch from MIDILOCALE */
DWORD MIDILOCALE2Patch (const MIDILOCALE *pLocale) {
	DWORD dwPatch = 0;
	if (!pLocale) return 0;
	dwPatch |= (pLocale->ulBank & F_INSTRUMENT_DRUMS); /* set drum bit */
	dwPatch |= ((pLocale->ulBank & 0x00007F7F) << 8); /* set MIDI bank location */
	dwPatch |= (pLocale->ulInstrument & 0x0000007F); /* set PC value */
	return dwPatch;	
}

/* MIDILOCALE from dwPatch */
void Patch2MIDILOCALE (DWORD dwPatch, LPMIDILOCALE pLocale) {
	memset (pLocale, 0, sizeof(MIDILOCALE));
	
	pLocale->ulInstrument = (dwPatch & 0x7F); /* get PC value */
	pLocale->ulBank = ((dwPatch & 0x007F7F00) >> 8); /* get MIDI bank location */
	pLocale->ulBank |= (dwPatch & F_INSTRUMENT_DRUMS); /* get drum bit */
}

/* generic flag-dumping function */
static const char* debugstr_flags (DWORD flags, const flag_info* names, size_t num_names){
	char buffer[128] = "", *ptr = &buffer[0];
	unsigned int i;
	int size = sizeof(buffer);
	
	for (i=0; i < num_names; i++)
	{
		if ((flags & names[i].val) ||	/* standard flag*/
			((!flags) && (!names[i].val))) { /* zero value only */
				int cnt = snprintf(ptr, size, "%s ", names[i].name);
				if (cnt < 0 || cnt >= size) break;
				size -= cnt;
				ptr += cnt;
		}
	}
	
	return wine_dbg_sprintf("%s", buffer);
}

/* Dump DMUS_PORTPARAMS flags */
static const char* debugstr_DMUS_PORTPARAMS_FLAGS(DWORD flagmask)
{
    static const flag_info flags[] = {
        FE(DMUS_PORTPARAMS_VOICES),
        FE(DMUS_PORTPARAMS_CHANNELGROUPS),
        FE(DMUS_PORTPARAMS_AUDIOCHANNELS),
        FE(DMUS_PORTPARAMS_SAMPLERATE),
        FE(DMUS_PORTPARAMS_EFFECTS),
        FE(DMUS_PORTPARAMS_SHARE)
    };
    return debugstr_flags(flagmask, flags, ARRAY_SIZE(flags));
}

/* Dump whole DMUS_PORTPARAMS struct */
void dump_DMUS_PORTPARAMS(LPDMUS_PORTPARAMS params)
{
    TRACE("DMUS_PORTPARAMS (%p):\n", params);
    TRACE(" - dwSize = %ld\n", params->dwSize);
    TRACE(" - dwValidParams = %s\n", debugstr_DMUS_PORTPARAMS_FLAGS(params->dwValidParams));
    if (params->dwValidParams & DMUS_PORTPARAMS_VOICES)        TRACE(" - dwVoices = %lu\n", params->dwVoices);
    if (params->dwValidParams & DMUS_PORTPARAMS_CHANNELGROUPS) TRACE(" - dwChannelGroup = %lu\n", params->dwChannelGroups);
    if (params->dwValidParams & DMUS_PORTPARAMS_AUDIOCHANNELS) TRACE(" - dwAudioChannels = %lu\n", params->dwAudioChannels);
    if (params->dwValidParams & DMUS_PORTPARAMS_SAMPLERATE)    TRACE(" - dwSampleRate = %lu\n", params->dwSampleRate);
    if (params->dwValidParams & DMUS_PORTPARAMS_EFFECTS)       TRACE(" - dwEffectFlags = %lx\n", params->dwEffectFlags);
    if (params->dwValidParams & DMUS_PORTPARAMS_SHARE)         TRACE(" - fShare = %u\n", params->fShare);
}
