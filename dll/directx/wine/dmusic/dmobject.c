/*
 * Base IDirectMusicObject Implementation
 * Keep in sync with the master in dlls/dmusic/dmobject.c
 *
 * Copyright (C) 2003-2004 Rok Mandeljc
 * Copyright (C) 2014 Michael Stefaniuc
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

#define COBJMACROS
#include <assert.h>
#include "objbase.h"
#include "dmusici.h"
#include "dmusicf.h"
#include "dmusics.h"
#include "dmobject.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(dmobj);
WINE_DECLARE_DEBUG_CHANNEL(dmfile);

/* Debugging helpers */
const char *debugstr_dmguid(const GUID *id) {
    unsigned int i;
#define X(guid) { &guid, #guid }
    static const struct {
        const GUID *guid;
        const char *name;
    } guids[] = {
        /* CLSIDs */
        X(CLSID_AudioVBScript),
        X(CLSID_DirectMusic),
        X(CLSID_DirectMusicAudioPathConfig),
        X(CLSID_DirectMusicAuditionTrack),
        X(CLSID_DirectMusicBand),
        X(CLSID_DirectMusicBandTrack),
        X(CLSID_DirectMusicChordMapTrack),
        X(CLSID_DirectMusicChordMap),
        X(CLSID_DirectMusicChordTrack),
        X(CLSID_DirectMusicCollection),
        X(CLSID_DirectMusicCommandTrack),
        X(CLSID_DirectMusicComposer),
        X(CLSID_DirectMusicContainer),
        X(CLSID_DirectMusicGraph),
        X(CLSID_DirectMusicLoader),
        X(CLSID_DirectMusicLyricsTrack),
        X(CLSID_DirectMusicMarkerTrack),
        X(CLSID_DirectMusicMelodyFormulationTrack),
        X(CLSID_DirectMusicMotifTrack),
        X(CLSID_DirectMusicMuteTrack),
        X(CLSID_DirectMusicParamControlTrack),
        X(CLSID_DirectMusicPatternTrack),
        X(CLSID_DirectMusicPerformance),
        X(CLSID_DirectMusicScript),
        X(CLSID_DirectMusicScriptAutoImpSegment),
        X(CLSID_DirectMusicScriptAutoImpPerformance),
        X(CLSID_DirectMusicScriptAutoImpSegmentState),
        X(CLSID_DirectMusicScriptAutoImpAudioPathConfig),
        X(CLSID_DirectMusicScriptAutoImpAudioPath),
        X(CLSID_DirectMusicScriptAutoImpSong),
        X(CLSID_DirectMusicScriptSourceCodeLoader),
        X(CLSID_DirectMusicScriptTrack),
        X(CLSID_DirectMusicSection),
        X(CLSID_DirectMusicSegment),
        X(CLSID_DirectMusicSegmentState),
        X(CLSID_DirectMusicSegmentTriggerTrack),
        X(CLSID_DirectMusicSegTriggerTrack),
        X(CLSID_DirectMusicSeqTrack),
        X(CLSID_DirectMusicSignPostTrack),
        X(CLSID_DirectMusicSong),
        X(CLSID_DirectMusicStyle),
        X(CLSID_DirectMusicStyleTrack),
        X(CLSID_DirectMusicSynth),
        X(CLSID_DirectMusicSynthSink),
        X(CLSID_DirectMusicSysExTrack),
        X(CLSID_DirectMusicTemplate),
        X(CLSID_DirectMusicTempoTrack),
        X(CLSID_DirectMusicTimeSigTrack),
        X(CLSID_DirectMusicWaveTrack),
        X(CLSID_DirectSoundWave),
        /* IIDs */
        X(IID_IDirectMusic),
        X(IID_IDirectMusic2),
        X(IID_IDirectMusic8),
        X(IID_IDirectMusicAudioPath),
        X(IID_IDirectMusicBand),
        X(IID_IDirectMusicBuffer),
        X(IID_IDirectMusicChordMap),
        X(IID_IDirectMusicCollection),
        X(IID_IDirectMusicComposer),
        X(IID_IDirectMusicContainer),
        X(IID_IDirectMusicDownload),
        X(IID_IDirectMusicDownloadedInstrument),
        X(IID_IDirectMusicGetLoader),
        X(IID_IDirectMusicGraph),
        X(IID_IDirectMusicInstrument),
        X(IID_IDirectMusicLoader),
        X(IID_IDirectMusicLoader8),
        X(IID_IDirectMusicObject),
        X(IID_IDirectMusicPatternTrack),
        X(IID_IDirectMusicPerformance),
        X(IID_IDirectMusicPerformance2),
        X(IID_IDirectMusicPerformance8),
        X(IID_IDirectMusicPort),
        X(IID_IDirectMusicPortDownload),
        X(IID_IDirectMusicScript),
        X(IID_IDirectMusicSegment),
        X(IID_IDirectMusicSegment2),
        X(IID_IDirectMusicSegment8),
        X(IID_IDirectMusicSegmentState),
        X(IID_IDirectMusicSegmentState8),
        X(IID_IDirectMusicStyle),
        X(IID_IDirectMusicStyle8),
        X(IID_IDirectMusicSynth),
        X(IID_IDirectMusicSynth8),
        X(IID_IDirectMusicSynthSink),
        X(IID_IDirectMusicThru),
        X(IID_IDirectMusicTool),
        X(IID_IDirectMusicTool8),
        X(IID_IDirectMusicTrack),
        X(IID_IDirectMusicTrack8),
        X(IID_IUnknown),
        X(IID_IPersistStream),
        X(IID_IStream),
        X(IID_IClassFactory),
        /* GUIDs */
        X(GUID_DirectMusicAllTypes),
        X(GUID_NOTIFICATION_CHORD),
        X(GUID_NOTIFICATION_COMMAND),
        X(GUID_NOTIFICATION_MEASUREANDBEAT),
        X(GUID_NOTIFICATION_PERFORMANCE),
        X(GUID_NOTIFICATION_RECOMPOSE),
        X(GUID_NOTIFICATION_SEGMENT),
        X(GUID_BandParam),
        X(GUID_ChordParam),
        X(GUID_CommandParam),
        X(GUID_CommandParam2),
        X(GUID_CommandParamNext),
        X(GUID_IDirectMusicBand),
        X(GUID_IDirectMusicChordMap),
        X(GUID_IDirectMusicStyle),
        X(GUID_MuteParam),
        X(GUID_Play_Marker),
        X(GUID_RhythmParam),
        X(GUID_TempoParam),
        X(GUID_TimeSignature),
        X(GUID_Valid_Start_Time),
        X(GUID_Clear_All_Bands),
        X(GUID_ConnectToDLSCollection),
        X(GUID_Disable_Auto_Download),
        X(GUID_DisableTempo),
        X(GUID_DisableTimeSig),
        X(GUID_Download),
        X(GUID_DownloadToAudioPath),
        X(GUID_Enable_Auto_Download),
        X(GUID_EnableTempo),
        X(GUID_EnableTimeSig),
        X(GUID_IgnoreBankSelectForGM),
        X(GUID_SeedVariations),
        X(GUID_StandardMIDIFile),
        X(GUID_Unload),
        X(GUID_UnloadFromAudioPath),
        X(GUID_Variations),
        X(GUID_PerfMasterTempo),
        X(GUID_PerfMasterVolume),
        X(GUID_PerfMasterGrooveLevel),
        X(GUID_PerfAutoDownload),
        X(GUID_DefaultGMCollection),
        X(GUID_Synth_Default),
        X(GUID_Buffer_Reverb),
        X(GUID_Buffer_EnvReverb),
        X(GUID_Buffer_Stereo),
        X(GUID_Buffer_3D_Dry),
        X(GUID_Buffer_Mono),
        X(GUID_DMUS_PROP_GM_Hardware),
        X(GUID_DMUS_PROP_GS_Capable),
        X(GUID_DMUS_PROP_GS_Hardware),
        X(GUID_DMUS_PROP_DLS1),
        X(GUID_DMUS_PROP_DLS2),
        X(GUID_DMUS_PROP_Effects),
        X(GUID_DMUS_PROP_INSTRUMENT2),
        X(GUID_DMUS_PROP_LegacyCaps),
        X(GUID_DMUS_PROP_MemorySize),
        X(GUID_DMUS_PROP_SampleMemorySize),
        X(GUID_DMUS_PROP_SamplePlaybackRate),
        X(GUID_DMUS_PROP_SetSynthSink),
        X(GUID_DMUS_PROP_SinkUsesDSound),
        X(GUID_DMUS_PROP_SynthSink_DSOUND),
        X(GUID_DMUS_PROP_SynthSink_WAVE),
        X(GUID_DMUS_PROP_Volume),
        X(GUID_DMUS_PROP_WavesReverb),
        X(GUID_DMUS_PROP_WriteLatency),
        X(GUID_DMUS_PROP_WritePeriod),
        X(GUID_DMUS_PROP_XG_Capable),
        X(GUID_DMUS_PROP_XG_Hardware)
    };
#undef X

    if (!id)
        return "(null)";

    for (i = 0; i < ARRAY_SIZE(guids); i++)
        if (IsEqualGUID(id, guids[i].guid))
            return guids[i].name;

    return debugstr_guid(id);
}

void dump_DMUS_OBJECTDESC(DMUS_OBJECTDESC *desc)
{
    if (!desc || !TRACE_ON(dmfile))
        return;

    TRACE_(dmfile)("DMUS_OBJECTDESC (%p):", desc);
    TRACE_(dmfile)(" - dwSize = %lu\n", desc->dwSize);

#define X(flag) if (desc->dwValidData & flag) TRACE_(dmfile)(#flag " ")
    TRACE_(dmfile)(" - dwValidData = %#08lx ( ", desc->dwValidData);
    X(DMUS_OBJ_OBJECT);
    X(DMUS_OBJ_CLASS);
    X(DMUS_OBJ_NAME);
    X(DMUS_OBJ_CATEGORY);
    X(DMUS_OBJ_FILENAME);
    X(DMUS_OBJ_FULLPATH);
    X(DMUS_OBJ_URL);
    X(DMUS_OBJ_VERSION);
    X(DMUS_OBJ_DATE);
    X(DMUS_OBJ_LOADED);
    X(DMUS_OBJ_MEMORY);
    X(DMUS_OBJ_STREAM);
    TRACE_(dmfile)(")\n");
#undef X

    if (desc->dwValidData & DMUS_OBJ_CLASS)
        TRACE_(dmfile)(" - guidClass = %s\n", debugstr_dmguid(&desc->guidClass));
    if (desc->dwValidData & DMUS_OBJ_OBJECT)
        TRACE_(dmfile)(" - guidObject = %s\n", debugstr_guid(&desc->guidObject));

    if (desc->dwValidData & DMUS_OBJ_DATE) {
        SYSTEMTIME time;
        FileTimeToSystemTime(&desc->ftDate, &time);
        TRACE_(dmfile)(" - ftDate = \'%04u-%02u-%02u %02u:%02u:%02u\'\n",
                time.wYear, time.wMonth, time.wDay, time.wHour, time.wMinute, time.wSecond);
    }
    if (desc->dwValidData & DMUS_OBJ_VERSION)
        TRACE_(dmfile)(" - vVersion = \'%u,%u,%u,%u\'\n",
                HIWORD(desc->vVersion.dwVersionMS), LOWORD(desc->vVersion.dwVersionMS),
                HIWORD(desc->vVersion.dwVersionLS), LOWORD(desc->vVersion.dwVersionLS));
    if (desc->dwValidData & DMUS_OBJ_NAME)
        TRACE_(dmfile)(" - wszName = %s\n", debugstr_w(desc->wszName));
    if (desc->dwValidData & DMUS_OBJ_CATEGORY)
        TRACE_(dmfile)(" - wszCategory = %s\n", debugstr_w(desc->wszCategory));
    if (desc->dwValidData & DMUS_OBJ_FILENAME)
        TRACE_(dmfile)(" - wszFileName = %s\n", debugstr_w(desc->wszFileName));
    if (desc->dwValidData & DMUS_OBJ_MEMORY)
        TRACE_(dmfile)(" - llMemLength = 0x%s - pbMemData = %p\n",
                wine_dbgstr_longlong(desc->llMemLength), desc->pbMemData);
    if (desc->dwValidData & DMUS_OBJ_STREAM)
        TRACE_(dmfile)(" - pStream = %p\n", desc->pStream);
}


/* RIFF format parsing */
#define CHUNK_HDR_SIZE (sizeof(FOURCC) + sizeof(DWORD))

const char *debugstr_chunk(const struct chunk_entry *chunk)
{
    const char *type = "";

    if (!chunk)
        return "(null)";
    if (chunk->id == FOURCC_RIFF || chunk->id == FOURCC_LIST)
        type = wine_dbg_sprintf("type %s, ", debugstr_fourcc(chunk->type));
    return wine_dbg_sprintf("%s chunk, %ssize %lu", debugstr_fourcc(chunk->id), type, chunk->size);
}

HRESULT stream_read(IStream *stream, void *data, ULONG size)
{
    ULONG read;
    HRESULT hr;

    hr = IStream_Read(stream, data, size, &read);
    if (FAILED(hr))
        TRACE_(dmfile)("IStream_Read failed: %#lx\n", hr);
    else if (!read && read < size) {
        /* All or nothing: Handle a partial read due to end of stream as an error */
        TRACE_(dmfile)("Short read: %lu < %lu\n", read, size);
        return E_FAIL;
    }

    return hr;
}

HRESULT stream_get_chunk(IStream *stream, struct chunk_entry *chunk)
{
    static const LARGE_INTEGER zero;
    ULONGLONG ck_end = 0, p_end = 0;
    HRESULT hr;

    hr = IStream_Seek(stream, zero, STREAM_SEEK_CUR, &chunk->offset);
    if (FAILED(hr))
        return hr;
    assert(!(chunk->offset.QuadPart & 1));
    if (chunk->parent) {
        p_end = chunk->parent->offset.QuadPart + CHUNK_HDR_SIZE + ((chunk->parent->size + 1) & ~1);
        if (chunk->offset.QuadPart == p_end)
            return S_FALSE;
        ck_end = chunk->offset.QuadPart + CHUNK_HDR_SIZE;
        if (ck_end > p_end) {
            WARN_(dmfile)("No space for sub-chunk header in parent chunk: ends at offset %s > %s\n",
                    wine_dbgstr_longlong(ck_end), wine_dbgstr_longlong(p_end));
            return E_FAIL;
        }
    }

    hr = stream_read(stream, chunk, CHUNK_HDR_SIZE);
    if (hr != S_OK)
        return hr;
    if (chunk->parent) {
        ck_end += (chunk->size + 1) & ~1;
        if (ck_end > p_end) {
            WARN_(dmfile)("No space for sub-chunk data in parent chunk: ends at offset %s > %s\n",
                    wine_dbgstr_longlong(ck_end), wine_dbgstr_longlong(p_end));
            return E_FAIL;
        }
    }

    if (chunk->id != FOURCC_LIST && chunk->id != FOURCC_RIFF)
        chunk->type = 0;
    else if ((hr = stream_read(stream, &chunk->type, sizeof(FOURCC))) != S_OK)
        return hr != S_FALSE ? hr : E_FAIL;

    TRACE_(dmfile)("Returning %s\n", debugstr_chunk(chunk));

    return S_OK;
}

HRESULT stream_skip_chunk(IStream *stream, const struct chunk_entry *chunk)
{
    LARGE_INTEGER end;

    end.QuadPart = (chunk->offset.QuadPart + CHUNK_HDR_SIZE + chunk->size + 1) & ~(ULONGLONG)1;

    return IStream_Seek(stream, end, STREAM_SEEK_SET, NULL);
}

HRESULT stream_next_chunk(IStream *stream, struct chunk_entry *chunk)
{
    HRESULT hr;

    if (chunk->id) {
        hr = stream_skip_chunk(stream, chunk);
        if (FAILED(hr))
            return hr;
    }

    return stream_get_chunk(stream, chunk);
}

/* Reads chunk data of the form:
   DWORD     - size of array element
   element[] - Array of elements
   The caller needs to free() the array.
*/
HRESULT stream_chunk_get_array(IStream *stream, const struct chunk_entry *chunk, void **array,
        unsigned int *count, DWORD elem_size)
{
    DWORD size;
    HRESULT hr;

    *array = NULL;
    *count = 0;

    if (chunk->size < sizeof(DWORD)) {
        WARN_(dmfile)("%s: Too short to read element size\n", debugstr_chunk(chunk));
        return E_FAIL;
    }
    if (FAILED(hr = stream_read(stream, &size, sizeof(DWORD))))
        return hr;
    if (size != elem_size) {
        WARN_(dmfile)("%s: Array element size mismatch: got %lu, expected %lu\n",
                debugstr_chunk(chunk), size, elem_size);
        return DMUS_E_UNSUPPORTED_STREAM;
    }

    *count = (chunk->size - sizeof(DWORD)) / elem_size;
    size = *count * elem_size;
    if (!(*array = malloc(size)))
        return E_OUTOFMEMORY;
    if (FAILED(hr = stream_read(stream, *array, size))) {
        free(*array);
        *array = NULL;
        return hr;
    }

    if (chunk->size > size + sizeof(DWORD)) {
        WARN_(dmfile)("%s: Extraneous data at end of array\n", debugstr_chunk(chunk));
        stream_skip_chunk(stream, chunk);
        return S_FALSE;
    }
    return S_OK;
}

HRESULT stream_chunk_get_data(IStream *stream, const struct chunk_entry *chunk, void *data,
        ULONG size)
{
    if (chunk->size != size) {
        WARN_(dmfile)("Chunk %s (size %lu, offset %s) doesn't contains the expected data size %lu\n",
                debugstr_fourcc(chunk->id), chunk->size,
                wine_dbgstr_longlong(chunk->offset.QuadPart), size);
        return E_FAIL;
    }
    return stream_read(stream, data, size);
}

HRESULT stream_chunk_get_wstr(IStream *stream, const struct chunk_entry *chunk, WCHAR *str,
        ULONG size)
{
    ULONG len;
    HRESULT hr;

    hr = IStream_Read(stream, str, min(chunk->size, size), &len);
    if (FAILED(hr))
        return hr;

    /* Don't assume the string is properly zero terminated */
    str[min(len, size - 1)] = 0;

    if (len < chunk->size)
        return S_FALSE;
    return S_OK;
}

HRESULT stream_get_loader(IStream *stream, IDirectMusicLoader **ret_loader)
{
    IDirectMusicGetLoader *getter;
    HRESULT hr;

    if (SUCCEEDED(hr = IStream_QueryInterface(stream, &IID_IDirectMusicGetLoader, (void**)&getter)))
    {
        hr = IDirectMusicGetLoader_GetLoader(getter, ret_loader);
        IDirectMusicGetLoader_Release(getter);
    }

    if (FAILED(hr)) *ret_loader = NULL;
    return hr;
}

HRESULT stream_get_object(IStream *stream, DMUS_OBJECTDESC *desc, REFIID iid, void **ret_iface)
{
    IDirectMusicLoader *loader;
    HRESULT hr;

    if (SUCCEEDED(hr = stream_get_loader(stream, &loader)))
    {
        hr = IDirectMusicLoader_GetObject(loader, desc, iid, ret_iface);
        IDirectMusicLoader_Release(loader);
    }

    if (FAILED(hr)) *ret_iface = NULL;
    return hr;
}


/* Generic IDirectMusicObject methods */
static inline struct dmobject *impl_from_IDirectMusicObject(IDirectMusicObject *iface)
{
    return CONTAINING_RECORD(iface, struct dmobject, IDirectMusicObject_iface);
}

HRESULT WINAPI dmobj_IDirectMusicObject_QueryInterface(IDirectMusicObject *iface, REFIID riid,
        void **ret_iface)
{
    struct dmobject *This = impl_from_IDirectMusicObject(iface);
    return IUnknown_QueryInterface(This->outer_unk, riid, ret_iface);
}

ULONG WINAPI dmobj_IDirectMusicObject_AddRef(IDirectMusicObject *iface)
{
    struct dmobject *This = impl_from_IDirectMusicObject(iface);
    return IUnknown_AddRef(This->outer_unk);
}

ULONG WINAPI dmobj_IDirectMusicObject_Release(IDirectMusicObject *iface)
{
    struct dmobject *This = impl_from_IDirectMusicObject(iface);
    return IUnknown_Release(This->outer_unk);
}

HRESULT WINAPI dmobj_IDirectMusicObject_GetDescriptor(IDirectMusicObject *iface,
        DMUS_OBJECTDESC *desc)
{
    struct dmobject *This = impl_from_IDirectMusicObject(iface);

    TRACE("(%p/%p)->(%p)\n", iface, This, desc);

    if (!desc)
        return E_POINTER;

    memcpy(desc, &This->desc, This->desc.dwSize);

    return S_OK;
}

HRESULT WINAPI dmobj_IDirectMusicObject_SetDescriptor(IDirectMusicObject *iface,
        DMUS_OBJECTDESC *desc)
{
    struct dmobject *This = impl_from_IDirectMusicObject(iface);
    HRESULT ret = S_OK;

    TRACE("(%p, %p)\n", iface, desc);

    if (!desc)
        return E_POINTER;

    /* Immutable property */
    if (desc->dwValidData & DMUS_OBJ_CLASS)
    {
        desc->dwValidData &= ~DMUS_OBJ_CLASS;
        ret = S_FALSE;
    }
    /* Set only valid fields */
    if (desc->dwValidData & DMUS_OBJ_OBJECT)
        This->desc.guidObject = desc->guidObject;
    if (desc->dwValidData & DMUS_OBJ_NAME)
        lstrcpynW(This->desc.wszName, desc->wszName, DMUS_MAX_NAME);
    if (desc->dwValidData & DMUS_OBJ_CATEGORY)
        lstrcpynW(This->desc.wszCategory, desc->wszCategory, DMUS_MAX_CATEGORY);
    if (desc->dwValidData & DMUS_OBJ_FILENAME)
        lstrcpynW(This->desc.wszFileName, desc->wszFileName, DMUS_MAX_FILENAME);
    if (desc->dwValidData & DMUS_OBJ_VERSION)
        This->desc.vVersion = desc->vVersion;
    if (desc->dwValidData & DMUS_OBJ_DATE)
        This->desc.ftDate = desc->ftDate;
    if (desc->dwValidData & DMUS_OBJ_MEMORY) {
        This->desc.llMemLength = desc->llMemLength;
        memcpy(This->desc.pbMemData, desc->pbMemData, desc->llMemLength);
    }
    if (desc->dwValidData & DMUS_OBJ_STREAM)
        IStream_Clone(desc->pStream, &This->desc.pStream);

    This->desc.dwValidData |= desc->dwValidData;

    return ret;
}

/* Helper for IDirectMusicObject::ParseDescriptor */
static inline void info_get_name(IStream *stream, const struct chunk_entry *info,
        DMUS_OBJECTDESC *desc)
{
    struct chunk_entry chunk = {.parent = info};
    char name[DMUS_MAX_NAME];
    ULONG len;
    HRESULT hr = E_FAIL;

    while (stream_next_chunk(stream, &chunk) == S_OK)
        if (chunk.id == mmioFOURCC('I','N','A','M'))
            hr = IStream_Read(stream, name, min(chunk.size, sizeof(name)), &len);

    if (SUCCEEDED(hr)) {
        len = MultiByteToWideChar(CP_ACP, 0, name, len, desc->wszName, sizeof(desc->wszName));
        desc->wszName[min(len, sizeof(desc->wszName) - 1)] = 0;
        desc->dwValidData |= DMUS_OBJ_NAME;
    }
}

static inline void unfo_get_name(IStream *stream, const struct chunk_entry *unfo,
        DMUS_OBJECTDESC *desc, BOOL inam)
{
    struct chunk_entry chunk = {.parent = unfo};

    while (stream_next_chunk(stream, &chunk) == S_OK)
        if (chunk.id == DMUS_FOURCC_UNAM_CHUNK || (inam && chunk.id == mmioFOURCC('I','N','A','M')))
            if (stream_chunk_get_wstr(stream, &chunk, desc->wszName, sizeof(desc->wszName)) == S_OK)
                desc->dwValidData |= DMUS_OBJ_NAME;
}

HRESULT dmobj_parsedescriptor(IStream *stream, const struct chunk_entry *riff,
        DMUS_OBJECTDESC *desc, DWORD supported)
{
    struct chunk_entry chunk = {.parent = riff};
    HRESULT hr;

    TRACE("Looking for %#lx in %p: %s\n", supported, stream, debugstr_chunk(riff));

    desc->dwValidData = 0;
    desc->dwSize = sizeof(*desc);

    while ((hr = stream_next_chunk(stream, &chunk)) == S_OK) {
        switch (chunk.id) {
            case DMUS_FOURCC_CATEGORY_CHUNK:
                if ((supported & DMUS_OBJ_CATEGORY) && stream_chunk_get_wstr(stream, &chunk,
                            desc->wszCategory, sizeof(desc->wszCategory)) == S_OK)
                    desc->dwValidData |= DMUS_OBJ_CATEGORY;
                break;
            case DMUS_FOURCC_DATE_CHUNK:
                if ((supported & DMUS_OBJ_DATE) && stream_chunk_get_data(stream, &chunk,
                            &desc->ftDate, sizeof(desc->ftDate)) == S_OK)
                    desc->dwValidData |= DMUS_OBJ_DATE;
                break;
            case DMUS_FOURCC_FILE_CHUNK:
                if ((supported & DMUS_OBJ_FILENAME) && stream_chunk_get_wstr(stream, &chunk,
                            desc->wszFileName, sizeof(desc->wszFileName)) == S_OK)
                    desc->dwValidData |= DMUS_OBJ_FILENAME;
                break;
            case FOURCC_DLID:
                if (!(supported & DMUS_OBJ_GUID_DLID)) break;
                if ((supported & DMUS_OBJ_OBJECT) && stream_chunk_get_data(stream, &chunk,
                            &desc->guidObject, sizeof(desc->guidObject)) == S_OK)
                    desc->dwValidData |= DMUS_OBJ_OBJECT;
                break;
            case DMUS_FOURCC_GUID_CHUNK:
                if ((supported & DMUS_OBJ_GUID_DLID)) break;
                if ((supported & DMUS_OBJ_OBJECT) && stream_chunk_get_data(stream, &chunk,
                            &desc->guidObject, sizeof(desc->guidObject)) == S_OK)
                    desc->dwValidData |= DMUS_OBJ_OBJECT;
                break;
            case DMUS_FOURCC_NAME_CHUNK:
                if ((supported & DMUS_OBJ_NAME) && stream_chunk_get_wstr(stream, &chunk,
                            desc->wszName, sizeof(desc->wszName)) == S_OK)
                    desc->dwValidData |= DMUS_OBJ_NAME;
                break;
            case DMUS_FOURCC_VERSION_CHUNK:
                if ((supported & DMUS_OBJ_VERSION) && stream_chunk_get_data(stream, &chunk,
                            &desc->vVersion, sizeof(desc->vVersion)) == S_OK)
                    desc->dwValidData |= DMUS_OBJ_VERSION;
                break;
            case FOURCC_LIST:
                if (chunk.type == DMUS_FOURCC_UNFO_LIST && (supported & DMUS_OBJ_NAME))
                    unfo_get_name(stream, &chunk, desc, supported & DMUS_OBJ_NAME_INAM);
                else if (chunk.type == DMUS_FOURCC_INFO_LIST && (supported & DMUS_OBJ_NAME_INFO))
                    info_get_name(stream, &chunk, desc);
                break;
        }
    }
    TRACE("Found %#lx\n", desc->dwValidData);

    return hr;
}

HRESULT dmobj_parsereference(IStream *stream, const struct chunk_entry *list,
        IDirectMusicObject **dmobj)
{
    struct chunk_entry chunk = {.parent = list};
    DMUS_OBJECTDESC desc;
    DMUS_IO_REFERENCE reference;
    HRESULT hr;

    if (FAILED(hr = stream_next_chunk(stream, &chunk)))
        return hr;
    if (chunk.id != DMUS_FOURCC_REF_CHUNK)
        return DMUS_E_UNSUPPORTED_STREAM;

    if (FAILED(hr = stream_chunk_get_data(stream, &chunk, &reference, sizeof(reference)))) {
        WARN("Failed to read data of %s\n", debugstr_chunk(&chunk));
        return hr;
    }
    TRACE("REFERENCE guidClassID %s, dwValidData %#lx\n", debugstr_dmguid(&reference.guidClassID),
            reference.dwValidData);

    if (FAILED(hr = dmobj_parsedescriptor(stream, list, &desc, reference.dwValidData)))
        return hr;
    desc.guidClass = reference.guidClassID;
    desc.dwValidData |= DMUS_OBJ_CLASS;
    dump_DMUS_OBJECTDESC(&desc);

    return stream_get_object(stream, &desc, &IID_IDirectMusicObject, (void **)dmobj);
}

/* Generic IPersistStream methods */
static inline struct dmobject *impl_from_IPersistStream(IPersistStream *iface)
{
    return CONTAINING_RECORD(iface, struct dmobject, IPersistStream_iface);
}

HRESULT WINAPI dmobj_IPersistStream_QueryInterface(IPersistStream *iface, REFIID riid,
        void **ret_iface)
{
    struct dmobject *This = impl_from_IPersistStream(iface);
    return IUnknown_QueryInterface(This->outer_unk, riid, ret_iface);
}

ULONG WINAPI dmobj_IPersistStream_AddRef(IPersistStream *iface)
{
    struct dmobject *This = impl_from_IPersistStream(iface);
    return IUnknown_AddRef(This->outer_unk);
}

ULONG WINAPI dmobj_IPersistStream_Release(IPersistStream *iface)
{
    struct dmobject *This = impl_from_IPersistStream(iface);
    return IUnknown_Release(This->outer_unk);
}

HRESULT WINAPI dmobj_IPersistStream_GetClassID(IPersistStream *iface, CLSID *class)
{
    struct dmobject *This = impl_from_IPersistStream(iface);

    TRACE("(%p, %p)\n", This, class);

    if (!class)
        return E_POINTER;

    *class = This->desc.guidClass;

    return S_OK;
}

/* IPersistStream methods not implemented in native */
HRESULT WINAPI unimpl_IPersistStream_GetClassID(IPersistStream *iface, CLSID *class)
{
    TRACE("(%p, %p): method not implemented\n", iface, class);
    return E_NOTIMPL;
}

HRESULT WINAPI unimpl_IPersistStream_IsDirty(IPersistStream *iface)
{
    TRACE("(%p): method not implemented, always returning S_FALSE\n", iface);
    return S_FALSE;
}

HRESULT WINAPI unimpl_IPersistStream_Save(IPersistStream *iface, IStream *stream,
        BOOL clear_dirty)
{
    TRACE("(%p, %p, %d): method not implemented\n", iface, stream, clear_dirty);
    return E_NOTIMPL;
}

HRESULT WINAPI unimpl_IPersistStream_GetSizeMax(IPersistStream *iface, ULARGE_INTEGER *size)
{
    TRACE("(%p, %p): method not implemented\n", iface, size);
    return E_NOTIMPL;
}


void dmobject_init(struct dmobject *dmobj, const GUID *class, IUnknown *outer_unk)
{
    dmobj->outer_unk = outer_unk;
    dmobj->desc.dwSize = sizeof(dmobj->desc);
    dmobj->desc.dwValidData = DMUS_OBJ_CLASS;
    dmobj->desc.guidClass = *class;
}
