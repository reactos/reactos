/*
 * IDirectMusic8 Implementation
 *
 * Copyright (C) 2003-2004 Rok Mandeljc
 * Copyright (C) 2012 Christian Costa
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

#include "dmusic_private.h"

#include <winreg.h>

static inline IDirectMusic8Impl *impl_from_IDirectMusic8(IDirectMusic8 *iface)
{
    return CONTAINING_RECORD(iface, IDirectMusic8Impl, IDirectMusic8_iface);
}

/* IDirectMusic8Impl IUnknown part: */
static HRESULT WINAPI IDirectMusic8Impl_QueryInterface(LPDIRECTMUSIC8 iface, REFIID riid, LPVOID *ret_iface)
{
    IDirectMusic8Impl *This = impl_from_IDirectMusic8(iface);

    TRACE("(%p)->(%s, %p)\n", iface, debugstr_dmguid(riid), ret_iface);

    if (IsEqualIID (riid, &IID_IUnknown) ||
        IsEqualIID (riid, &IID_IDirectMusic) ||
        IsEqualIID (riid, &IID_IDirectMusic2) ||
        IsEqualIID (riid, &IID_IDirectMusic8))
    {
        IDirectMusic8_AddRef(iface);
        *ret_iface = iface;
        return S_OK;
    }

    *ret_iface = NULL;

    WARN("(%p, %s, %p): not found\n", This, debugstr_dmguid(riid), ret_iface);

    return E_NOINTERFACE;
}

static ULONG WINAPI IDirectMusic8Impl_AddRef(LPDIRECTMUSIC8 iface)
{
    IDirectMusic8Impl *This = impl_from_IDirectMusic8(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p)->(): new ref = %u\n", This, ref);

    return ref;
}

static ULONG WINAPI IDirectMusic8Impl_Release(LPDIRECTMUSIC8 iface)
{
    IDirectMusic8Impl *This = impl_from_IDirectMusic8(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p)->(): new ref = %u\n", This, ref);

    if (!ref) {
        IReferenceClock_Release(&This->pMasterClock->IReferenceClock_iface);
        HeapFree(GetProcessHeap(), 0, This->system_ports);
        HeapFree(GetProcessHeap(), 0, This->ppPorts);
        HeapFree(GetProcessHeap(), 0, This);
        DMUSIC_UnlockModule();
    }

    return ref;
}

/* IDirectMusic8Impl IDirectMusic part: */
static HRESULT WINAPI IDirectMusic8Impl_EnumPort(LPDIRECTMUSIC8 iface, DWORD index, LPDMUS_PORTCAPS port_caps)
{
    IDirectMusic8Impl *This = impl_from_IDirectMusic8(iface);

    TRACE("(%p, %d, %p)\n", This, index, port_caps);

    if (!port_caps)
        return E_POINTER;

    if (index >= This->nb_system_ports)
        return S_FALSE;

    *port_caps = This->system_ports[index].caps;

    return S_OK;
}

static HRESULT WINAPI IDirectMusic8Impl_CreateMusicBuffer(LPDIRECTMUSIC8 iface, LPDMUS_BUFFERDESC buffer_desc, LPDIRECTMUSICBUFFER* buffer, LPUNKNOWN unkouter)
{
    IDirectMusic8Impl *This = impl_from_IDirectMusic8(iface);

    TRACE("(%p)->(%p, %p, %p)\n", This, buffer_desc, buffer, unkouter);

    if (unkouter)
        return CLASS_E_NOAGGREGATION;

    if (!buffer_desc || !buffer)
        return E_POINTER;

    return DMUSIC_CreateDirectMusicBufferImpl(buffer_desc, (LPVOID)buffer);
}

static HRESULT WINAPI IDirectMusic8Impl_CreatePort(LPDIRECTMUSIC8 iface, REFCLSID rclsid_port, LPDMUS_PORTPARAMS port_params, LPDIRECTMUSICPORT* port, LPUNKNOWN unkouter)
{
    IDirectMusic8Impl *This = impl_from_IDirectMusic8(iface);
    int i;
    DMUS_PORTCAPS port_caps;
    IDirectMusicPort* new_port = NULL;
    HRESULT hr;
    GUID default_port;
    const GUID *request_port = rclsid_port;

    TRACE("(%p)->(%s, %p, %p, %p)\n", This, debugstr_dmguid(rclsid_port), port_params, port, unkouter);

    if (!rclsid_port)
        return E_POINTER;
    if (!port_params)
        return E_INVALIDARG;
    if (!port)
        return E_POINTER;
    if (unkouter)
        return CLASS_E_NOAGGREGATION;

    if (TRACE_ON(dmusic))
        dump_DMUS_PORTPARAMS(port_params);

    ZeroMemory(&port_caps, sizeof(DMUS_PORTCAPS));
    port_caps.dwSize = sizeof(DMUS_PORTCAPS);

    if (IsEqualGUID(request_port, &GUID_NULL)) {
        hr = IDirectMusic8_GetDefaultPort(iface, &default_port);
        if(FAILED(hr))
            return hr;
        request_port = &default_port;
    }

    for (i = 0; S_FALSE != IDirectMusic8Impl_EnumPort(iface, i, &port_caps); i++) {
        if (IsEqualCLSID(request_port, &port_caps.guidPort)) {
            hr = This->system_ports[i].create(&IID_IDirectMusicPort, (LPVOID*)&new_port, (LPUNKNOWN)This, port_params, &port_caps, This->system_ports[i].device);
            if (FAILED(hr)) {
                 *port = NULL;
                 return hr;
            }
            This->nrofports++;
            if (!This->ppPorts)
                This->ppPorts = HeapAlloc(GetProcessHeap(), 0, sizeof(LPDIRECTMUSICPORT) * This->nrofports);
            else
                This->ppPorts = HeapReAlloc(GetProcessHeap(), 0, This->ppPorts, sizeof(LPDIRECTMUSICPORT) * This->nrofports);
            This->ppPorts[This->nrofports - 1] = new_port;
            *port = new_port;
            return S_OK;
        }
    }

    return E_NOINTERFACE;
}

static HRESULT WINAPI IDirectMusic8Impl_EnumMasterClock(LPDIRECTMUSIC8 iface, DWORD index, LPDMUS_CLOCKINFO clock_info)
{
    TRACE("(%p)->(%d, %p)\n", iface, index, clock_info);

    if (!clock_info)
        return E_POINTER;

    if (index > 1)
        return S_FALSE;

    if (!index)
    {
        static const GUID guid_system_clock = { 0x58d58419, 0x71b4, 0x11d1, { 0xa7, 0x4c, 0x00, 0x00, 0xf8, 0x75, 0xac, 0x12 } };
        static const WCHAR name_system_clock[] = { 'S','y','s','t','e','m',' ','C','l','o','c','k',0 };

        clock_info->ctType = 0;
        clock_info->guidClock = guid_system_clock;
        strcpyW(clock_info->wszDescription, name_system_clock);
    }
    else
    {
        static const GUID guid_dsound_clock = { 0x58d58420, 0x71b4, 0x11d1, { 0xa7, 0x4c, 0x00, 0x00, 0xf8, 0x75, 0xac, 0x12 } };
        static const WCHAR name_dsound_clock[] = { 'D','i','r','e','c','t','S','o','u','n','d',' ','C','l','o','c','k',0 };

        clock_info->ctType = 0;
        clock_info->guidClock = guid_dsound_clock;
        strcpyW(clock_info->wszDescription, name_dsound_clock);
    }

    return S_OK;
}

static HRESULT WINAPI IDirectMusic8Impl_GetMasterClock(LPDIRECTMUSIC8 iface, LPGUID guid_clock, IReferenceClock** reference_clock)
{
    IDirectMusic8Impl *This = impl_from_IDirectMusic8(iface);

    TRACE("(%p)->(%p, %p)\n", This, guid_clock, reference_clock);

    if (guid_clock)
        *guid_clock = This->pMasterClock->pClockInfo.guidClock;
    if (reference_clock) {
        *reference_clock = &This->pMasterClock->IReferenceClock_iface;
        IReferenceClock_AddRef(*reference_clock);
    }

    return S_OK;
}

static HRESULT WINAPI IDirectMusic8Impl_SetMasterClock(LPDIRECTMUSIC8 iface, REFGUID rguidClock)
{
    IDirectMusic8Impl *This = impl_from_IDirectMusic8(iface);

    FIXME("(%p)->(%s): stub\n", This, debugstr_dmguid(rguidClock));

    return S_OK;
}

static HRESULT WINAPI IDirectMusic8Impl_Activate(LPDIRECTMUSIC8 iface, BOOL enable)
{
    IDirectMusic8Impl *This = impl_from_IDirectMusic8(iface);
    int i;
    HRESULT hr;

    TRACE("(%p)->(%u)\n", This, enable);

    for (i = 0; i < This->nrofports; i++)
    {
        hr = IDirectMusicPort_Activate(This->ppPorts[i], enable);
        if (FAILED(hr))
            return hr;
    }

    return S_OK;
}

static HRESULT WINAPI IDirectMusic8Impl_GetDefaultPort(LPDIRECTMUSIC8 iface, LPGUID guid_port)
{
    IDirectMusic8Impl *This = impl_from_IDirectMusic8(iface);
    HKEY hkGUID;
    DWORD returnTypeGUID, sizeOfReturnBuffer = 50;
    char returnBuffer[51];
    GUID defaultPortGUID;
    WCHAR buff[51];

    TRACE("(%p)->(%p)\n", This, guid_port);

    if ((RegOpenKeyExA(HKEY_LOCAL_MACHINE, "Software\\Microsoft\\DirectMusic\\Defaults" , 0, KEY_READ, &hkGUID) != ERROR_SUCCESS) ||
        (RegQueryValueExA(hkGUID, "DefaultOutputPort", NULL, &returnTypeGUID, (LPBYTE)returnBuffer, &sizeOfReturnBuffer) != ERROR_SUCCESS))
    {
        WARN(": registry entry missing\n" );
        *guid_port = CLSID_DirectMusicSynth;
        return S_OK;
    }
    /* FIXME: Check return types to ensure we're interpreting data right */
    MultiByteToWideChar(CP_ACP, 0, returnBuffer, -1, buff, sizeof(buff) / sizeof(WCHAR));
    CLSIDFromString(buff, &defaultPortGUID);
    *guid_port = defaultPortGUID;

    return S_OK;
}

static HRESULT WINAPI IDirectMusic8Impl_SetDirectSound(LPDIRECTMUSIC8 iface, LPDIRECTSOUND dsound, HWND wnd)
{
    IDirectMusic8Impl *This = impl_from_IDirectMusic8(iface);

    FIXME("(%p)->(%p, %p): stub\n", This, dsound, wnd);

    return S_OK;
}

static HRESULT WINAPI IDirectMusic8Impl_SetExternalMasterClock(LPDIRECTMUSIC8 iface, IReferenceClock* clock)
{
    IDirectMusic8Impl *This = impl_from_IDirectMusic8(iface);

    FIXME("(%p)->(%p): stub\n", This, clock);

    return S_OK;
}

static const IDirectMusic8Vtbl DirectMusic8_Vtbl = {
    IDirectMusic8Impl_QueryInterface,
    IDirectMusic8Impl_AddRef,
    IDirectMusic8Impl_Release,
    IDirectMusic8Impl_EnumPort,
    IDirectMusic8Impl_CreateMusicBuffer,
    IDirectMusic8Impl_CreatePort,
    IDirectMusic8Impl_EnumMasterClock,
    IDirectMusic8Impl_GetMasterClock,
    IDirectMusic8Impl_SetMasterClock,
    IDirectMusic8Impl_Activate,
    IDirectMusic8Impl_GetDefaultPort,
    IDirectMusic8Impl_SetDirectSound,
    IDirectMusic8Impl_SetExternalMasterClock
};

static void create_system_ports_list(IDirectMusic8Impl* object)
{
    port_info * port;
    const WCHAR emulated[] = {' ','[','E','m','u','l','a','t','e','d',']',0};
    ULONG nb_ports;
    ULONG nb_midi_out;
    ULONG nb_midi_in;
    MIDIOUTCAPSW caps_out;
    MIDIINCAPSW caps_in;
    IDirectMusicSynth8* synth;
    HRESULT hr;
    ULONG i;

    TRACE("(%p)\n", object);

    /* NOTE:
       - it seems some native versions get the rest of devices through dmusic32.EnumLegacyDevices...*sigh*...which is undocumented
       - should we enum wave devices ? Native does not seem to
    */

    nb_midi_out = midiOutGetNumDevs();
    nb_midi_in = midiInGetNumDevs();
    nb_ports = 1 /* midi mapper */ + nb_midi_out + nb_midi_in + 1 /* synth port */;

    port = object->system_ports = HeapAlloc(GetProcessHeap(), 0, nb_ports * sizeof(port_info));
    if (!object->system_ports)
        return;

    /* Fill common port caps for all winmm ports */
    for (i = 0; i < (nb_ports - 1 /* synth port*/); i++)
    {
        object->system_ports[i].caps.dwSize = sizeof(DMUS_PORTCAPS);
        object->system_ports[i].caps.dwType = DMUS_PORT_WINMM_DRIVER;
        object->system_ports[i].caps.dwMemorySize = 0;
        object->system_ports[i].caps.dwMaxChannelGroups = 1;
        object->system_ports[i].caps.dwMaxVoices = 0;
        object->system_ports[i].caps.dwMaxAudioChannels = 0;
        object->system_ports[i].caps.dwEffectFlags = DMUS_EFFECT_NONE;
        /* Fake port GUID */
        object->system_ports[i].caps.guidPort = IID_IUnknown;
        object->system_ports[i].caps.guidPort.Data1 = i + 1;
    }

    /* Fill midi mapper port info */
    port->device = MIDI_MAPPER;
    port->create = DMUSIC_CreateMidiOutPortImpl;
    midiOutGetDevCapsW(MIDI_MAPPER, &caps_out, sizeof(caps_out));
    strcpyW(port->caps.wszDescription, caps_out.szPname);
    strcatW(port->caps.wszDescription, emulated);
    port->caps.dwFlags = DMUS_PC_SHAREABLE;
    port->caps.dwClass = DMUS_PC_OUTPUTCLASS;
    port++;

    /* Fill midi out port info */
    for (i = 0; i < nb_midi_out; i++)
    {
        port->device = i;
        port->create = DMUSIC_CreateMidiOutPortImpl;
        midiOutGetDevCapsW(i, &caps_out, sizeof(caps_out));
        strcpyW(port->caps.wszDescription, caps_out.szPname);
        strcatW(port->caps.wszDescription, emulated);
        port->caps.dwFlags = DMUS_PC_SHAREABLE | DMUS_PC_EXTERNAL;
        port->caps.dwClass = DMUS_PC_OUTPUTCLASS;
        port++;
    }

    /* Fill midi in port info */
    for (i = 0; i < nb_midi_in; i++)
    {
        port->device = i;
        port->create = DMUSIC_CreateMidiInPortImpl;
        midiInGetDevCapsW(i, &caps_in, sizeof(caps_in));
        strcpyW(port->caps.wszDescription, caps_in.szPname);
        strcatW(port->caps.wszDescription, emulated);
        port->caps.dwFlags = DMUS_PC_EXTERNAL;
        port->caps.dwClass = DMUS_PC_INPUTCLASS;
        port++;
    }

    /* Fill synth port info */
    port->create = DMUSIC_CreateSynthPortImpl;
    hr = CoCreateInstance(&CLSID_DirectMusicSynth, NULL, CLSCTX_INPROC_SERVER, &IID_IDirectMusicSynth8, (void**)&synth);
    if (SUCCEEDED(hr))
    {
        port->caps.dwSize = sizeof(port->caps);
        hr = IDirectMusicSynth8_GetPortCaps(synth, &port->caps);
        IDirectMusicSynth8_Release(synth);
    }
    if (FAILED(hr))
        nb_ports--;

    object->nb_system_ports = nb_ports;
}

/* For ClassFactory */
HRESULT WINAPI DMUSIC_CreateDirectMusicImpl(LPCGUID riid, LPVOID* ret_iface, LPUNKNOWN unkouter)
{
    IDirectMusic8Impl *dmusic;
    HRESULT ret;

    TRACE("(%s, %p, %p)\n", debugstr_guid(riid), ret_iface, unkouter);

    *ret_iface = NULL;
    if (unkouter)
        return CLASS_E_NOAGGREGATION;

    dmusic = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDirectMusic8Impl));
    if (!dmusic)
        return E_OUTOFMEMORY;

    dmusic->IDirectMusic8_iface.lpVtbl = &DirectMusic8_Vtbl;
    dmusic->ref = 1;
    dmusic->pMasterClock = NULL;
    dmusic->ppPorts = NULL;
    dmusic->nrofports = 0;
    ret = DMUSIC_CreateReferenceClockImpl(&IID_IReferenceClock, (LPVOID*)&dmusic->pMasterClock, NULL);
    if (FAILED(ret)) {
        HeapFree(GetProcessHeap(), 0, dmusic);
        return ret;
    }

    create_system_ports_list(dmusic);

    DMUSIC_LockModule();
    ret = IDirectMusic8Impl_QueryInterface(&dmusic->IDirectMusic8_iface, riid, ret_iface);
    IDirectMusic8_Release(&dmusic->IDirectMusic8_iface);

    return ret;
}
