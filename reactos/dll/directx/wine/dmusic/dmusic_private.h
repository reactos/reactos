/*
 * DirectMusic Private Include
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

#ifndef __WINE_DMUSIC_PRIVATE_H
#define __WINE_DMUSIC_PRIVATE_H

#include <wine/config.h>

#include <stdarg.h>

#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H

#define COBJMACROS

#include <windef.h>
#include <winbase.h>
#include <objbase.h>
#include <dmusici.h>
#include <dmusics.h>

#include <wine/debug.h>
#include <wine/list.h>
#include <wine/unicode.h>

#include "dmobject.h"

WINE_DEFAULT_DEBUG_CHANNEL(dmusic);

/*****************************************************************************
 * Interfaces
 */
typedef struct IDirectMusic8Impl IDirectMusic8Impl;
typedef struct IDirectMusicBufferImpl IDirectMusicBufferImpl;
typedef struct IDirectMusicDownloadedInstrumentImpl IDirectMusicDownloadedInstrumentImpl;
typedef struct IDirectMusicDownloadImpl IDirectMusicDownloadImpl;
typedef struct IReferenceClockImpl IReferenceClockImpl;

typedef struct IDirectMusicInstrumentImpl IDirectMusicInstrumentImpl;

typedef struct SynthPortImpl SynthPortImpl;

/*****************************************************************************
 * Some stuff to make my life easier :=)
 */
 
/* some sort of aux. midi channel: big fake at the moment; accepts only priority
   changes... more coming soon */
typedef struct DMUSIC_PRIVATE_MCHANNEL_ {
	DWORD priority;
} DMUSIC_PRIVATE_MCHANNEL, *LPDMUSIC_PRIVATE_MCHANNEL;

/* some sort of aux. channel group: collection of 16 midi channels */
typedef struct DMUSIC_PRIVATE_CHANNEL_GROUP_ {
	DMUSIC_PRIVATE_MCHANNEL channel[16]; /* 16 channels in a group */
} DMUSIC_PRIVATE_CHANNEL_GROUP, *LPDMUSIC_PRIVATE_CHANNEL_GROUP;

typedef struct port_info {
    DMUS_PORTCAPS caps;
    HRESULT (*create)(LPCGUID guid, LPVOID *object, LPUNKNOWN unkouter, LPDMUS_PORTPARAMS port_params, LPDMUS_PORTCAPS port_caps, DWORD device);
    ULONG device;
} port_info;

typedef struct instrument_region {
    RGNHEADER header;
    WAVELINK wave_link;
    WSMPL wave_sample;
    WLOOP wave_loop;
    BOOL loop_present;
} instrument_region;

typedef struct instrument_articulation {
    CONNECTIONLIST connections_list;
    CONNECTION *connections;
} instrument_articulation;

/*****************************************************************************
 * ClassFactory
 */

/* CLSID */
extern HRESULT WINAPI DMUSIC_CreateDirectMusicImpl (LPCGUID lpcGUID, LPVOID* ppobj, LPUNKNOWN pUnkOuter) DECLSPEC_HIDDEN;
extern HRESULT WINAPI DMUSIC_CreateDirectMusicCollectionImpl(LPCGUID lpcGUID, LPVOID* ppobj, LPUNKNOWN pUnkOuter) DECLSPEC_HIDDEN;

/* Internal */
extern HRESULT DMUSIC_CreateDirectMusicBufferImpl(LPDMUS_BUFFERDESC desc, LPVOID* ret_iface) DECLSPEC_HIDDEN;
extern HRESULT DMUSIC_CreateDirectMusicDownloadImpl (LPCGUID lpcGUID, LPVOID* ppobj, LPUNKNOWN pUnkOuter) DECLSPEC_HIDDEN;
extern HRESULT DMUSIC_CreateReferenceClockImpl (LPCGUID lpcGUID, LPVOID* ppobj, LPUNKNOWN pUnkOuter) DECLSPEC_HIDDEN;
extern HRESULT DMUSIC_CreateDirectMusicInstrumentImpl (LPCGUID lpcGUID, LPVOID* ppobj, LPUNKNOWN pUnkOuter) DECLSPEC_HIDDEN;

/*****************************************************************************
 * IDirectMusic8Impl implementation structure
 */
struct IDirectMusic8Impl {
    /* IUnknown fields */
    IDirectMusic8 IDirectMusic8_iface;
    LONG ref;

    /* IDirectMusicImpl fields */
    IReferenceClockImpl* pMasterClock;
    IDirectMusicPort** ppPorts;
    int nrofports;
    port_info* system_ports;
    int nb_system_ports;
};

/*****************************************************************************
 * IDirectMusicBufferImpl implementation structure
 */
struct IDirectMusicBufferImpl {
    /* IUnknown fields */
    IDirectMusicBuffer IDirectMusicBuffer_iface;
    LONG ref;

    /* IDirectMusicBufferImpl fields */
    GUID format;
    DWORD size;
    LPBYTE data;
    DWORD write_pos;
    REFERENCE_TIME start_time;
};

/*****************************************************************************
 * IDirectMusicDownloadedInstrumentImpl implementation structure
 */
struct IDirectMusicDownloadedInstrumentImpl {
    /* IUnknown fields */
    IDirectMusicDownloadedInstrument IDirectMusicDownloadedInstrument_iface;
    LONG ref;

    /* IDirectMusicDownloadedInstrumentImpl fields */
    BOOL downloaded;
    void *data;
};

/*****************************************************************************
 * IDirectMusicDownloadImpl implementation structure
 */
struct IDirectMusicDownloadImpl {
    /* IUnknown fields */
    IDirectMusicDownload IDirectMusicDownload_iface;
    LONG ref;

    /* IDirectMusicDownloadImpl fields */
};

/*****************************************************************************
 * SynthPortImpl implementation structure
 */
struct SynthPortImpl {
    /* IUnknown fields */
    IDirectMusicPort IDirectMusicPort_iface;
    IDirectMusicPortDownload IDirectMusicPortDownload_iface;
    IDirectMusicThru IDirectMusicThru_iface;
    LONG ref;

    /* IDirectMusicPort fields */
    IDirectSound* pDirectSound;
    IReferenceClock* pLatencyClock;
    IDirectMusicSynth* synth;
    IDirectMusicSynthSink* synth_sink;
    BOOL fActive;
    DMUS_PORTCAPS caps;
    DMUS_PORTPARAMS params;
    int nrofgroups;
    DMUSIC_PRIVATE_CHANNEL_GROUP group[1];
};

/** Internal factory */
extern HRESULT DMUSIC_CreateSynthPortImpl(LPCGUID guid, LPVOID *object, LPUNKNOWN unkouter, LPDMUS_PORTPARAMS port_params, LPDMUS_PORTCAPS port_caps, DWORD device) DECLSPEC_HIDDEN;
extern HRESULT DMUSIC_CreateMidiOutPortImpl(LPCGUID guid, LPVOID *object, LPUNKNOWN unkouter, LPDMUS_PORTPARAMS port_params, LPDMUS_PORTCAPS port_caps, DWORD device) DECLSPEC_HIDDEN;
extern HRESULT DMUSIC_CreateMidiInPortImpl(LPCGUID guid, LPVOID *object, LPUNKNOWN unkouter, LPDMUS_PORTPARAMS port_params, LPDMUS_PORTCAPS port_caps, DWORD device) DECLSPEC_HIDDEN;

/*****************************************************************************
 * IReferenceClockImpl implementation structure
 */
struct IReferenceClockImpl {
    /* IUnknown fields */
    IReferenceClock IReferenceClock_iface;
    LONG ref;

    /* IReferenceClockImpl fields */
    REFERENCE_TIME rtTime;
    DMUS_CLOCKINFO pClockInfo;
};

typedef struct _DMUS_PRIVATE_INSTRUMENT_ENTRY {
	struct list entry; /* for listing elements */
	IDirectMusicInstrument* pInstrument;
} DMUS_PRIVATE_INSTRUMENTENTRY, *LPDMUS_PRIVATE_INSTRUMENTENTRY;

typedef struct _DMUS_PRIVATE_POOLCUE {
	struct list entry; /* for listing elements */
} DMUS_PRIVATE_POOLCUE, *LPDMUS_PRIVATE_POOLCUE;

/*****************************************************************************
 * IDirectMusicInstrumentImpl implementation structure
 */
struct IDirectMusicInstrumentImpl {
    /* IUnknown fields */
    IDirectMusicInstrument IDirectMusicInstrument_iface;
    LONG ref;

    /* IDirectMusicInstrumentImpl fields */
    LARGE_INTEGER liInstrumentPosition; /* offset in a stream where instrument chunk can be found */
    ULONG length; /* Length of the instrument in the stream */
    GUID id;
    INSTHEADER header;
    WCHAR wszName[DMUS_MAX_NAME];
    /* instrument data */
    BOOL loaded;
    instrument_region *regions;
    ULONG nb_articulations;
    instrument_articulation *articulations;
};

static inline IDirectMusicInstrumentImpl *impl_from_IDirectMusicInstrument(IDirectMusicInstrument *iface)
{
    return CONTAINING_RECORD(iface, IDirectMusicInstrumentImpl, IDirectMusicInstrument_iface);
}

/* custom :) */
extern HRESULT IDirectMusicInstrumentImpl_CustomLoad(IDirectMusicInstrument *iface, IStream *stream) DECLSPEC_HIDDEN;

/**********************************************************************
 * Dll lifetime tracking declaration for dmusic.dll
 */
extern LONG DMUSIC_refCount DECLSPEC_HIDDEN;
static inline void DMUSIC_LockModule(void) { InterlockedIncrement( &DMUSIC_refCount ); }
static inline void DMUSIC_UnlockModule(void) { InterlockedDecrement( &DMUSIC_refCount ); }


/*****************************************************************************
 * Misc.
 */
/* for simpler reading */
typedef struct _DMUS_PRIVATE_CHUNK {
	FOURCC fccID; /* FOURCC ID of the chunk */
	DWORD dwSize; /* size of the chunk */
} DMUS_PRIVATE_CHUNK, *LPDMUS_PRIVATE_CHUNK;

/* used for generic dumping (copied from ddraw) */
typedef struct {
    DWORD val;
    const char* name;
} flag_info;

typedef struct {
    const GUID *guid;
    const char* name;
} guid_info;

#define FE(x) { x, #x }	
#define GE(x) { &x, #x }

/* dwPatch from MIDILOCALE */
extern DWORD MIDILOCALE2Patch (const MIDILOCALE *pLocale) DECLSPEC_HIDDEN;
/* MIDILOCALE from dwPatch */
extern void Patch2MIDILOCALE (DWORD dwPatch, LPMIDILOCALE pLocale) DECLSPEC_HIDDEN;

/* check whether the given DWORD is even (return 0) or odd (return 1) */
extern int even_or_odd (DWORD number) DECLSPEC_HIDDEN;
/* FOURCC to string conversion for debug messages */
extern const char *debugstr_fourcc (DWORD fourcc) DECLSPEC_HIDDEN;
/* returns name of given GUID */
extern const char *debugstr_dmguid (const GUID *id) DECLSPEC_HIDDEN;
/* Dump whole DMUS_OBJECTDESC struct */
extern void dump_DMUS_OBJECTDESC(LPDMUS_OBJECTDESC desc) DECLSPEC_HIDDEN;
/* Dump whole DMUS_PORTPARAMS struct */
extern void dump_DMUS_PORTPARAMS(LPDMUS_PORTPARAMS params) DECLSPEC_HIDDEN;

#endif /* __WINE_DMUSIC_PRIVATE_H */
