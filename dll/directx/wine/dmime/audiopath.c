/* IDirectMusicAudioPath Implementation
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

#include "dmime_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(dmime);

struct IDirectMusicAudioPathImpl {
    IDirectMusicAudioPath IDirectMusicAudioPath_iface;
    LONG ref;
    IDirectMusicPerformance8* pPerf;
    IDirectMusicGraph*        pToolGraph;
    IDirectSoundBuffer*       pDSBuffer;
    IDirectSoundBuffer*       pPrimary;

    BOOL fActive;
};

struct audio_path_pchannel_to_buffer
{
    struct list entry;
    DMUS_IO_PCHANNELTOBUFFER_HEADER header;
    GUID guids[];
};

C_ASSERT(sizeof(struct audio_path_pchannel_to_buffer) == offsetof(struct audio_path_pchannel_to_buffer, guids[0]));

struct audio_path_port_config
{
    struct list entry;
    DMUS_IO_PORTCONFIG_HEADER header;
    DMUS_PORTPARAMS8 params;

    struct list pchannel_to_buffer_entries;
};

struct audio_path_config
{
    IUnknown IUnknown_iface;
    struct dmobject dmobj;
    LONG ref;

    struct list port_config_entries;
};

static inline struct IDirectMusicAudioPathImpl *impl_from_IDirectMusicAudioPath(IDirectMusicAudioPath *iface)
{
    return CONTAINING_RECORD(iface, struct IDirectMusicAudioPathImpl, IDirectMusicAudioPath_iface);
}

static inline struct audio_path_config *impl_from_IPersistStream(IPersistStream *iface)
{
    return CONTAINING_RECORD(iface, struct audio_path_config, dmobj.IPersistStream_iface);
}

void set_audiopath_perf_pointer(IDirectMusicAudioPath *iface, IDirectMusicPerformance8 *performance)
{
    struct IDirectMusicAudioPathImpl *This = impl_from_IDirectMusicAudioPath(iface);
    This->pPerf = performance;
}

void set_audiopath_dsound_buffer(IDirectMusicAudioPath *iface, IDirectSoundBuffer *buffer)
{
    struct IDirectMusicAudioPathImpl *This = impl_from_IDirectMusicAudioPath(iface);
    This->pDSBuffer = buffer;
}

void set_audiopath_primary_dsound_buffer(IDirectMusicAudioPath *iface, IDirectSoundBuffer *buffer)
{
    struct IDirectMusicAudioPathImpl *This = impl_from_IDirectMusicAudioPath(iface);
    This->pPrimary = buffer;
}

/*****************************************************************************
 * IDirectMusicAudioPathImpl implementation
 */
static HRESULT WINAPI IDirectMusicAudioPathImpl_QueryInterface (IDirectMusicAudioPath *iface, REFIID riid, void **ppobj)
{
    struct IDirectMusicAudioPathImpl *This = impl_from_IDirectMusicAudioPath(iface);

    TRACE("(%p, %s, %p)\n", This, debugstr_dmguid(riid), ppobj);

    *ppobj = NULL;

    if (IsEqualIID (riid, &IID_IDirectMusicAudioPath) || IsEqualIID (riid, &IID_IUnknown))
        *ppobj = &This->IDirectMusicAudioPath_iface;

    if (*ppobj) {
        IUnknown_AddRef((IUnknown*)*ppobj);
        return S_OK;
    }

    WARN("(%p, %s, %p): not found\n", This, debugstr_dmguid(riid), ppobj);
    return E_NOINTERFACE;
}

static ULONG WINAPI IDirectMusicAudioPathImpl_AddRef (IDirectMusicAudioPath *iface)
{
    struct IDirectMusicAudioPathImpl *This = impl_from_IDirectMusicAudioPath(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p): ref=%ld\n", This, ref);

    return ref;
}

static ULONG WINAPI IDirectMusicAudioPathImpl_Release (IDirectMusicAudioPath *iface)
{
    struct IDirectMusicAudioPathImpl *This = impl_from_IDirectMusicAudioPath(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p): ref=%ld\n", This, ref);

    if (ref == 0) {
        if (This->pPrimary)
            IDirectSoundBuffer_Release(This->pPrimary);
        if (This->pDSBuffer)
            IDirectSoundBuffer_Release(This->pDSBuffer);
        This->pPerf = NULL;
        free(This);
    }

    return ref;
}

static HRESULT WINAPI IDirectMusicAudioPathImpl_GetObjectInPath (IDirectMusicAudioPath *iface, DWORD dwPChannel, DWORD dwStage, DWORD dwBuffer,
    REFGUID guidObject, DWORD dwIndex, REFGUID iidInterface, void** ppObject)
{
	struct IDirectMusicAudioPathImpl *This = impl_from_IDirectMusicAudioPath(iface);
	HRESULT hr;

	FIXME("(%p, %ld, %ld, %ld, %s, %ld, %s, %p): stub\n", This, dwPChannel, dwStage, dwBuffer, debugstr_dmguid(guidObject),
            dwIndex, debugstr_dmguid(iidInterface), ppObject);
	    
	switch (dwStage) {
        case DMUS_PATH_BUFFER:
          if (This->pDSBuffer)
          {
              if (IsEqualIID (iidInterface, &IID_IUnknown) ||
                  IsEqualIID (iidInterface, &IID_IDirectSoundBuffer)  ||
                  IsEqualIID (iidInterface, &IID_IDirectSoundBuffer8) ||
                  IsEqualIID (iidInterface, &IID_IDirectSound3DBuffer)) {
                  return IDirectSoundBuffer_QueryInterface(This->pDSBuffer, iidInterface, ppObject);
              }

              WARN("Unsupported interface %s\n", debugstr_dmguid(iidInterface));
              *ppObject = NULL;
              return E_NOINTERFACE;
          }
          break;

	case DMUS_PATH_PRIMARY_BUFFER: {
	  if (IsEqualIID (iidInterface, &IID_IDirectSound3DListener)) {
            IDirectSoundBuffer_QueryInterface(This->pPrimary, &IID_IDirectSound3DListener, ppObject);
	    return S_OK;
	  } else {
	    FIXME("bad iid...\n");
	  }
	}
	break;

	case DMUS_PATH_AUDIOPATH_GRAPH:
	  {
	    if (IsEqualIID (iidInterface, &IID_IDirectMusicGraph)) {
	      if (NULL == This->pToolGraph) {
		IDirectMusicGraph* pGraph;
		hr = create_dmgraph(&IID_IDirectMusicGraph, (void**)&pGraph);
		if (FAILED(hr))
		  return hr;
		This->pToolGraph = pGraph;
	      }
	      *ppObject = This->pToolGraph;
	      IDirectMusicGraph_AddRef((LPDIRECTMUSICGRAPH) *ppObject);
	      return S_OK;
	    }
	  }
	  break;

	case DMUS_PATH_AUDIOPATH_TOOL:
	  {
	    /* TODO */
	  }
	  break;

	case DMUS_PATH_PERFORMANCE:
	  {
	    /* TODO check wanted GUID */
	    *ppObject = This->pPerf;
	    IUnknown_AddRef((LPUNKNOWN) *ppObject);
	    return S_OK;
	  }
	  break;

	case DMUS_PATH_PERFORMANCE_GRAPH:
	  {
	    IDirectMusicGraph* pPerfoGraph = NULL; 
	    IDirectMusicPerformance8_GetGraph(This->pPerf, &pPerfoGraph);
	    if (NULL == pPerfoGraph) {
	      IDirectMusicGraph* pGraph = NULL;
	      hr = create_dmgraph(&IID_IDirectMusicGraph, (void**)&pGraph);
	      if (FAILED(hr))
		return hr;
	      IDirectMusicPerformance8_SetGraph(This->pPerf, pGraph);
	      pPerfoGraph = pGraph;
	    }
	    *ppObject = pPerfoGraph;
	    return S_OK;
	  }
	  break;

	case DMUS_PATH_PERFORMANCE_TOOL:
	default:
	  break;
	}

	*ppObject = NULL;
	return E_INVALIDARG;
}

static HRESULT WINAPI IDirectMusicAudioPathImpl_Activate(IDirectMusicAudioPath *iface, BOOL activate)
{
    struct IDirectMusicAudioPathImpl *This = impl_from_IDirectMusicAudioPath(iface);

    FIXME("(%p, %d): semi-stub\n", This, activate);

    if (!!activate == This->fActive)
        return S_FALSE;

    if (!activate && This->pDSBuffer) {
        /* Path is being deactivated */
        IDirectSoundBuffer_Stop(This->pDSBuffer);
    }

    This->fActive = !!activate;

    return S_OK;
}

static HRESULT WINAPI IDirectMusicAudioPathImpl_SetVolume (IDirectMusicAudioPath *iface, LONG lVolume, DWORD dwDuration)
{
  struct IDirectMusicAudioPathImpl *This = impl_from_IDirectMusicAudioPath(iface);
  FIXME("(%p, %li, %ld): stub\n", This, lVolume, dwDuration);
  return S_OK;
}

static HRESULT WINAPI IDirectMusicAudioPathImpl_ConvertPChannel (IDirectMusicAudioPath *iface, DWORD dwPChannelIn, DWORD* pdwPChannelOut)
{
  struct IDirectMusicAudioPathImpl *This = impl_from_IDirectMusicAudioPath(iface);
  FIXME("(%p, %ld, %p): stub\n", This, dwPChannelIn, pdwPChannelOut);
  return S_OK;
}

static const IDirectMusicAudioPathVtbl DirectMusicAudioPathVtbl = {
  IDirectMusicAudioPathImpl_QueryInterface,
  IDirectMusicAudioPathImpl_AddRef,
  IDirectMusicAudioPathImpl_Release,
  IDirectMusicAudioPathImpl_GetObjectInPath,
  IDirectMusicAudioPathImpl_Activate,
  IDirectMusicAudioPathImpl_SetVolume,
  IDirectMusicAudioPathImpl_ConvertPChannel
};

/* IDirectMusicObject */
static HRESULT WINAPI path_config_IDirectMusicObject_ParseDescriptor(IDirectMusicObject *iface,
        IStream *stream, DMUS_OBJECTDESC *desc)
{
    struct chunk_entry riff = {0};
    HRESULT hr;

    TRACE("(%p, %p, %p)\n", iface, stream, desc);

    if (!stream)
        return E_POINTER;
    if (!desc || desc->dwSize != sizeof(*desc))
        return E_INVALIDARG;

    if ((hr = stream_get_chunk(stream, &riff)) != S_OK)
        return hr;
    if (riff.id != FOURCC_RIFF || riff.type != DMUS_FOURCC_AUDIOPATH_FORM) {
        TRACE("loading failed: unexpected %s\n", debugstr_chunk(&riff));
        stream_skip_chunk(stream, &riff);
        return DMUS_E_CHUNKNOTFOUND;
    }

    hr = dmobj_parsedescriptor(stream, &riff, desc,
            DMUS_OBJ_OBJECT|DMUS_OBJ_NAME|DMUS_OBJ_CATEGORY|DMUS_OBJ_VERSION);
    if (FAILED(hr))
        return hr;

    desc->guidClass = CLSID_DirectMusicAudioPathConfig;
    desc->dwValidData |= DMUS_OBJ_CLASS;

    dump_DMUS_OBJECTDESC(desc);
    return S_OK;
}

static const IDirectMusicObjectVtbl dmobject_vtbl = {
    dmobj_IDirectMusicObject_QueryInterface,
    dmobj_IDirectMusicObject_AddRef,
    dmobj_IDirectMusicObject_Release,
    dmobj_IDirectMusicObject_GetDescriptor,
    dmobj_IDirectMusicObject_SetDescriptor,
    path_config_IDirectMusicObject_ParseDescriptor
};

static HRESULT parse_pchannel_to_buffers_list(struct audio_path_port_config *This, IStream *stream, const struct chunk_entry *pchl)
{
    struct chunk_entry chunk = { .parent = pchl };
    struct audio_path_pchannel_to_buffer *pchannel_to_buffer = NULL;
    DMUS_IO_PCHANNELTOBUFFER_HEADER header;
    SIZE_T bytes;
    ULONG i;
    HRESULT hr;

    while ((hr = stream_next_chunk(stream, &chunk)) == S_OK)
    {
        switch (chunk.id)
        {
        case DMUS_FOURCC_PCHANNELS_ITEM:
            hr = stream_read(stream, &header, sizeof(header));
            if (FAILED(hr))
                break;

            TRACE("Got PChannelToBuffer header:\n");
            TRACE("\tdwPChannelCount: %lu\n", header.dwPChannelCount);
            TRACE("\tdwPChannelBase: %lu\n", header.dwPChannelBase);
            TRACE("\tdwBufferCount: %lu\n", header.dwBufferCount);
            TRACE("\tdwFlags: %lx\n", header.dwFlags);

            bytes = chunk.size - sizeof(header);

            if (bytes != sizeof(GUID) * header.dwBufferCount)
            {
                WARN("Invalid size of GUIDs array\n");
                hr = E_FAIL;
                break;
            }

            pchannel_to_buffer = calloc(1, sizeof(*pchannel_to_buffer) + bytes);
            if (!pchannel_to_buffer)
            {
                hr = E_OUTOFMEMORY;
                break;
            }

            pchannel_to_buffer->header = header;
            TRACE("number of GUIDs: %lu\n", pchannel_to_buffer->header.dwBufferCount);

            hr = stream_read(stream, pchannel_to_buffer->guids, bytes);
            if (FAILED(hr))
                break;

            for (i = 0; i < pchannel_to_buffer->header.dwBufferCount; i++)
                TRACE("\tguids[%lu]: %s\n", i, debugstr_dmguid(&pchannel_to_buffer->guids[i]));

            list_add_tail(&This->pchannel_to_buffer_entries, &pchannel_to_buffer->entry);
            pchannel_to_buffer = NULL;
            break;
        case FOURCC_LIST:
            if (chunk.type == DMUS_FOURCC_UNFO_LIST)
                TRACE("Unfo list in PChannelToBuffer is ignored\n");
            else
                WARN("Unknown %s\n", debugstr_chunk(&chunk));
            break;
        case MAKEFOURCC('p','b','g','d'):
            FIXME("Undocumented %s\n", debugstr_chunk(&chunk));
            break;
        default:
            WARN("Unknown %s\n", debugstr_chunk(&chunk));
            break;
        }

        if (FAILED(hr))
            break;
    }

    if (FAILED(hr) && pchannel_to_buffer)
        free(pchannel_to_buffer);
    return hr;
}

static void port_config_destroy(struct audio_path_port_config *port_config)
{
    struct audio_path_pchannel_to_buffer *pchannel_to_buffer, *next_pchannel_to_buffer;
    LIST_FOR_EACH_ENTRY_SAFE(pchannel_to_buffer, next_pchannel_to_buffer, &port_config->pchannel_to_buffer_entries,
        struct audio_path_pchannel_to_buffer, entry)
    {
        list_remove(&pchannel_to_buffer->entry);
        free(pchannel_to_buffer);
    }
    free(port_config);
}

static HRESULT parse_port_config_list(struct audio_path_config *This, IStream *stream, const struct chunk_entry *pcfl)
{
    struct chunk_entry chunk = { .parent = pcfl };
    struct audio_path_port_config *port_config = calloc(1, sizeof(*port_config));
    HRESULT hr;

    if (!port_config)
        return E_OUTOFMEMORY;

    list_init(&port_config->pchannel_to_buffer_entries);

    while ((hr = stream_next_chunk(stream, &chunk)) == S_OK)
    {
        switch (chunk.id)
        {
        case DMUS_FOURCC_PORTCONFIG_ITEM:
            hr = stream_chunk_get_data(stream, &chunk, &port_config->header, sizeof(port_config->header));
            if (FAILED(hr))
                break;
            TRACE("Got PortConfig header:\n");
            TRACE("\tdwPChannelBase: %lu\n", port_config->header.dwPChannelBase);
            TRACE("\tdwPChannelCount: %lu\n", port_config->header.dwPChannelCount);
            TRACE("\tdwFlags: %lx\n", port_config->header.dwFlags);
            TRACE("\tguidPort: %s\n", debugstr_dmguid(&port_config->header.guidPort));
            break;
        case DMUS_FOURCC_PORTPARAMS_ITEM:
            hr = stream_chunk_get_data(stream, &chunk, &port_config->params, sizeof(port_config->params));
            if (FAILED(hr))
                break;
            TRACE("Got PortConfig params:\n");
            TRACE("\tdwSize: %lu\n", port_config->params.dwSize);
            TRACE("\tdwValidParams: %lx\n", port_config->params.dwValidParams);
            if (port_config->params.dwValidParams & DMUS_PORTPARAMS_VOICES)
                TRACE("\tvoices: %lu\n", port_config->params.dwVoices);
            if (port_config->params.dwValidParams & DMUS_PORTPARAMS_CHANNELGROUPS)
                TRACE("\tchannel groups: %lu\n", port_config->params.dwChannelGroups);
            if (port_config->params.dwValidParams & DMUS_PORTPARAMS_AUDIOCHANNELS)
                TRACE("\taudio channels: %lu\n", port_config->params.dwAudioChannels);
            if (port_config->params.dwValidParams & DMUS_PORTPARAMS_SAMPLERATE)
                TRACE("\tsample rate: %lu\n", port_config->params.dwSampleRate);
            if (port_config->params.dwValidParams & DMUS_PORTPARAMS_EFFECTS)
                TRACE("\teffects: %lx\n", port_config->params.dwEffectFlags);
            if (port_config->params.dwValidParams & DMUS_PORTPARAMS_SHARE)
                TRACE("\tshare: %d\n", port_config->params.fShare);
            if (port_config->params.dwValidParams & DMUS_PORTPARAMS_FEATURES)
                TRACE("\tfeatures: %lx\n", port_config->params.dwFeatures);
            break;
        case FOURCC_LIST:
            if (chunk.type == DMUS_FOURCC_DSBUFFER_LIST)
                FIXME("DirectSound buffer descriptors is not supported\n");
            else if (chunk.type == DMUS_FOURCC_PCHANNELS_LIST)
                hr = parse_pchannel_to_buffers_list(port_config, stream, &chunk);
            else if (chunk.type == DMUS_FOURCC_UNFO_LIST)
                TRACE("Unfo list in PortConfig is ignored\n");
            else
                WARN("Unknown %s\n", debugstr_chunk(&chunk));
            break;
        default:
            WARN("Unknown %s\n", debugstr_chunk(&chunk));
        }

        if (FAILED(hr))
            break;
    }

    if (FAILED(hr))
        port_config_destroy(port_config);
    else
        list_add_tail(&This->port_config_entries, &port_config->entry);
    return hr;
}

static HRESULT parse_port_configs_list(struct audio_path_config *This, IStream *stream, const struct chunk_entry *pcsl)
{
    struct chunk_entry chunk = { .parent = pcsl };
    HRESULT hr;

    while ((hr = stream_next_chunk(stream, &chunk)) == S_OK)
    {
        switch (MAKE_IDTYPE(chunk.id, chunk.type))
        {
        case MAKE_IDTYPE(FOURCC_LIST, DMUS_FOURCC_PORTCONFIG_LIST):
            hr = parse_port_config_list(This, stream, &chunk);
            if (FAILED(hr))
                return hr;
            break;
        default:
            WARN("Unknown %s\n", debugstr_chunk(&chunk));
            break;
        }
    }
    return SUCCEEDED(hr) ? S_OK : hr;
}

/* IPersistStream */
static HRESULT WINAPI path_config_IPersistStream_Load(IPersistStream *iface, IStream *stream)
{
    struct audio_path_config *This = impl_from_IPersistStream(iface);
    struct chunk_entry riff = {0}, chunk = {0};
    HRESULT hr = S_OK;

    FIXME("(%p, %p): Loading\n", This, stream);

    if (!stream)
        return E_POINTER;

    hr = stream_get_chunk(stream, &riff);
    if (FAILED(hr))
    {
        WARN("Failed to get chunk %#lx.\n", hr);
        return hr;
    }

    if (MAKE_IDTYPE(riff.id, riff.type) != MAKE_IDTYPE(FOURCC_RIFF, DMUS_FOURCC_AUDIOPATH_FORM))
    {
        WARN("loading failed: unexpected %s\n", debugstr_chunk(&riff));
        return E_UNEXPECTED;
    }

    if (FAILED(hr = dmobj_parsedescriptor(stream, &riff, &This->dmobj.desc,
            DMUS_OBJ_OBJECT | DMUS_OBJ_VERSION | DMUS_OBJ_NAME | DMUS_OBJ_CATEGORY))
                || FAILED(hr = stream_reset_chunk_data(stream, &riff)))
    {
        WARN("Failed to parse descriptor %#lx.\n", hr);
        return hr;
    }
    This->dmobj.desc.guidClass = CLSID_DirectMusicAudioPathConfig;
    This->dmobj.desc.dwValidData |= DMUS_OBJ_CLASS;

    chunk.parent = &riff;
    while ((hr = stream_next_chunk(stream, &chunk)) == S_OK)
    {
        switch (MAKE_IDTYPE(chunk.id, chunk.type))
        {
        case MAKE_IDTYPE(FOURCC_LIST,  DMUS_FOURCC_PORTCONFIGS_LIST):
            hr = parse_port_configs_list(This, stream, &chunk);
            if (FAILED(hr))
                return hr;
        case MAKE_IDTYPE(FOURCC_LIST, DMUS_FOURCC_DSBUFFER_LIST):
            FIXME("buffer attributes are not supported\n");
            break;
        case MAKE_IDTYPE(FOURCC_LIST, MAKEFOURCC('p','a','p','d')):
            FIXME("Undocumented %s\n", debugstr_chunk(&chunk));
            break;
        case MAKE_IDTYPE(FOURCC_LIST, DMUS_FOURCC_UNFO_LIST):
            WARN("Unknown %s\n", debugstr_chunk(&chunk));
            break;
        case DMUS_FOURCC_GUID_CHUNK:
        case DMUS_FOURCC_VERSION_CHUNK:
            /* Already parsed in dmobj_parsedescriptor. */
            break;
        default:
            WARN("Unknown %s\n", debugstr_chunk(&chunk));
            break;
        }
    }
    TRACE("Finished parsing %#lx\n", hr);
    return SUCCEEDED(hr) ? S_OK : hr;
}

static const IPersistStreamVtbl persiststream_vtbl = {
    dmobj_IPersistStream_QueryInterface,
    dmobj_IPersistStream_AddRef,
    dmobj_IPersistStream_Release,
    dmobj_IPersistStream_GetClassID,
    unimpl_IPersistStream_IsDirty,
    path_config_IPersistStream_Load,
    unimpl_IPersistStream_Save,
    unimpl_IPersistStream_GetSizeMax
};

static inline struct audio_path_config *impl_from_IUnknown(IUnknown *iface)
{
    return CONTAINING_RECORD(iface, struct audio_path_config, IUnknown_iface);
}

static HRESULT WINAPI path_config_IUnknown_QueryInterface(IUnknown *iface, REFIID riid, void **ppobj)
{
    struct audio_path_config *This = impl_from_IUnknown(iface);

    TRACE("(%p, %s, %p)\n", This, debugstr_dmguid(riid), ppobj);

    *ppobj = NULL;

    if (IsEqualIID (riid, &IID_IUnknown))
        *ppobj = &This->IUnknown_iface;
    else if (IsEqualIID (riid, &IID_IDirectMusicObject))
        *ppobj = &This->dmobj.IDirectMusicObject_iface;
    else if (IsEqualIID (riid, &IID_IPersistStream))
        *ppobj = &This->dmobj.IPersistStream_iface;

    if (*ppobj)
    {
        IUnknown_AddRef((IUnknown*)*ppobj);
        return S_OK;
    }

    WARN("(%p, %s, %p): not found\n", This, debugstr_dmguid(riid), ppobj);
    return E_NOINTERFACE;
}

static ULONG WINAPI path_config_IUnknown_AddRef(IUnknown *unk)
{
    struct audio_path_config *This = impl_from_IUnknown(unk);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p): ref=%ld\n", This, ref);

    return ref;
}

static ULONG WINAPI path_config_IUnknown_Release(IUnknown *unk)
{
    struct audio_path_config *This = impl_from_IUnknown(unk);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p): ref=%ld\n", This, ref);

    if (!ref)
    {
        struct audio_path_port_config *config, *next_config;
        LIST_FOR_EACH_ENTRY_SAFE(config, next_config, &This->port_config_entries, struct audio_path_port_config, entry)
        {
            list_remove(&config->entry);
            port_config_destroy(config);
        }
        free(This);
    }
    return ref;
}

static const IUnknownVtbl path_config_unk_vtbl =
{
    path_config_IUnknown_QueryInterface,
    path_config_IUnknown_AddRef,
    path_config_IUnknown_Release,
};

HRESULT path_config_get_audio_path_params(IUnknown *iface, WAVEFORMATEX *format, DSBUFFERDESC *desc, DMUS_PORTPARAMS *params)
{
    struct audio_path_config *This = impl_from_IUnknown(iface);
    struct list *first_port_config, *first_pchannel_to_buffer;
    struct audio_path_port_config *port_config;
    struct audio_path_pchannel_to_buffer *pchannel_to_buffer;
    GUID *guids;

    first_port_config = list_head(&This->port_config_entries);
    if (list_next(&This->port_config_entries, first_port_config))
        FIXME("Only one port config supported. %p -> %p\n", first_port_config, list_next(&This->port_config_entries, first_port_config));
    port_config = LIST_ENTRY(first_port_config, struct audio_path_port_config, entry);
    first_pchannel_to_buffer = list_head(&port_config->pchannel_to_buffer_entries);
    if (list_next(&port_config->pchannel_to_buffer_entries, first_pchannel_to_buffer))
        FIXME("Only one pchannel to buffer entry supported.\n");
    pchannel_to_buffer = LIST_ENTRY(first_pchannel_to_buffer, struct audio_path_pchannel_to_buffer, entry);

    /* Secondary buffer description */
    memset(format, 0, sizeof(*format));
    format->wFormatTag = WAVE_FORMAT_PCM;
    format->nChannels = 1;
    format->nSamplesPerSec = 44000;
    format->nAvgBytesPerSec = 44000 * 2;
    format->nBlockAlign = 2;
    format->wBitsPerSample = 16;
    format->cbSize = 0;

    memset(desc, 0, sizeof(*desc));
    desc->dwSize = sizeof(*desc);
    desc->dwFlags = DSBCAPS_CTRLFX | DSBCAPS_CTRLVOLUME | DSBCAPS_GLOBALFOCUS;
    desc->dwBufferBytes = DSBSIZE_MIN;
    desc->dwReserved = 0;
    desc->lpwfxFormat = format;
    desc->guid3DAlgorithm = GUID_NULL;

    guids = pchannel_to_buffer->guids;
    if (pchannel_to_buffer->header.dwBufferCount == 2)
    {
        if ((!IsEqualGUID(&guids[0], &GUID_Buffer_Reverb) && !IsEqualGUID(&guids[0], &GUID_Buffer_Stereo)) ||
                (!IsEqualGUID(&guids[1], &GUID_Buffer_Reverb) && !IsEqualGUID(&guids[1], &GUID_Buffer_Stereo)) ||
                IsEqualGUID(&guids[0], &guids[1]))
            FIXME("Only a stereo plus reverb buffer is supported\n");
        else
        {
            desc->dwFlags |= DSBCAPS_CTRLPAN | DSBCAPS_CTRLFREQUENCY;
            format->nChannels = 2;
            format->nBlockAlign *= 2;
            format->nAvgBytesPerSec *= 2;
        }
    }
    else if (pchannel_to_buffer->header.dwBufferCount == 1)
    {
        if (IsEqualGUID(guids, &GUID_Buffer_Stereo))
        {
            desc->dwFlags |= DSBCAPS_CTRLPAN | DSBCAPS_CTRLFREQUENCY;
            format->nChannels = 2;
            format->nBlockAlign *= 2;
            format->nAvgBytesPerSec *= 2;
        }
        else if (IsEqualGUID(guids, &GUID_Buffer_3D_Dry))
            desc->dwFlags |= DSBCAPS_CTRL3D | DSBCAPS_CTRLFREQUENCY | DSBCAPS_MUTE3DATMAXDISTANCE;
        else if (IsEqualGUID(guids, &GUID_Buffer_Mono))
            desc->dwFlags |= DSBCAPS_CTRLPAN | DSBCAPS_CTRLFREQUENCY;
        else
            FIXME("Unsupported buffer guid %s\n", debugstr_dmguid(guids));
    }
    else
        FIXME("Multiple buffers not supported\n");

    *params = port_config->params;
    if (!(params->dwValidParams & DMUS_PORTPARAMS_CHANNELGROUPS))
    {
        params->dwValidParams |= DMUS_PORTPARAMS_CHANNELGROUPS;
        params->dwChannelGroups = (port_config->header.dwPChannelCount + 15) / 16;
    }
    if (!(params->dwValidParams & DMUS_PORTPARAMS_AUDIOCHANNELS))
    {
        params->dwValidParams |= DMUS_PORTPARAMS_AUDIOCHANNELS;
        params->dwAudioChannels = format->nChannels;
    }
    return S_OK;
}

/* for ClassFactory */
HRESULT create_dmaudiopath(REFIID riid, void **ppobj)
{
    IDirectMusicAudioPathImpl* obj;
    HRESULT hr;

    *ppobj = NULL;
    if (!(obj = calloc(1, sizeof(*obj)))) return E_OUTOFMEMORY;
    obj->IDirectMusicAudioPath_iface.lpVtbl = &DirectMusicAudioPathVtbl;
    obj->ref = 1;

    hr = IDirectMusicAudioPath_QueryInterface(&obj->IDirectMusicAudioPath_iface, riid, ppobj);
    IDirectMusicAudioPath_Release(&obj->IDirectMusicAudioPath_iface);
    return hr;
}

HRESULT create_dmaudiopath_config(REFIID riid, void **ppobj)
{
    struct audio_path_config *obj;
    HRESULT hr;

    *ppobj = NULL;
    if (!(obj = calloc(1, sizeof(*obj)))) return E_OUTOFMEMORY;
    dmobject_init(&obj->dmobj, &CLSID_DirectMusicAudioPathConfig, &obj->IUnknown_iface);
    obj->IUnknown_iface.lpVtbl = &path_config_unk_vtbl;
    obj->dmobj.IDirectMusicObject_iface.lpVtbl = &dmobject_vtbl;
    obj->dmobj.IPersistStream_iface.lpVtbl = &persiststream_vtbl;
    obj->ref = 1;

    list_init(&obj->port_config_entries);

    hr = IUnknown_QueryInterface(&obj->IUnknown_iface, riid, ppobj);
    IUnknown_Release(&obj->IUnknown_iface);
    return hr;
}
