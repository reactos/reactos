/*
 * IDirectMusicInstrument Implementation
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

#include "dmusic_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(dmusic);

static const GUID IID_IDirectMusicInstrumentPRIVATE = { 0xbcb20080, 0xa40c, 0x11d1, { 0x86, 0xbc, 0x00, 0xc0, 0x4f, 0xbf, 0x8f, 0xef } };

/* IDirectMusicInstrument IUnknown part: */
static HRESULT WINAPI IDirectMusicInstrumentImpl_QueryInterface(LPDIRECTMUSICINSTRUMENT iface, REFIID riid, LPVOID *ret_iface)
{
    TRACE("(%p)->(%s, %p)\n", iface, debugstr_dmguid(riid), ret_iface);

    if (IsEqualIID(riid, &IID_IUnknown) ||
        IsEqualIID(riid, &IID_IDirectMusicInstrument))
    {
        *ret_iface = iface;
        IDirectMusicInstrument_AddRef(iface);
        return S_OK;
    }
    else if (IsEqualIID(riid, &IID_IDirectMusicInstrumentPRIVATE))
    {
        /* it seems to me that this interface is only basic IUnknown, without any
         * other inherited functions... *sigh* this is the worst scenario, since it means
         * that whoever calls it knows the layout of original implementation table and therefore
         * tries to get data by direct access... expect crashes
         */
        FIXME("*sigh*... requested private/unspecified interface\n");

        *ret_iface = iface;
        IDirectMusicInstrument_AddRef(iface);
        return S_OK;
    }

    WARN("(%p)->(%s, %p): not found\n", iface, debugstr_dmguid(riid), ret_iface);

    return E_NOINTERFACE;
}

static ULONG WINAPI IDirectMusicInstrumentImpl_AddRef(LPDIRECTMUSICINSTRUMENT iface)
{
    IDirectMusicInstrumentImpl *This = impl_from_IDirectMusicInstrument(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p)->(): new ref = %u\n", iface, ref);

    return ref;
}

static ULONG WINAPI IDirectMusicInstrumentImpl_Release(LPDIRECTMUSICINSTRUMENT iface)
{
    IDirectMusicInstrumentImpl *This = impl_from_IDirectMusicInstrument(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p)->(): new ref = %u\n", iface, ref);

    if (!ref)
    {
        ULONG i;

        HeapFree(GetProcessHeap(), 0, This->regions);
        for (i = 0; i < This->nb_articulations; i++)
            HeapFree(GetProcessHeap(), 0, This->articulations->connections);
        HeapFree(GetProcessHeap(), 0, This->articulations);
        HeapFree(GetProcessHeap(), 0, This);
        DMUSIC_UnlockModule();
    }

    return ref;
}

/* IDirectMusicInstrumentImpl IDirectMusicInstrument part: */
static HRESULT WINAPI IDirectMusicInstrumentImpl_GetPatch(LPDIRECTMUSICINSTRUMENT iface, DWORD* pdwPatch)
{
    IDirectMusicInstrumentImpl *This = impl_from_IDirectMusicInstrument(iface);

    TRACE("(%p)->(%p)\n", This, pdwPatch);

    *pdwPatch = MIDILOCALE2Patch(&This->header.Locale);

    return S_OK;
}

static HRESULT WINAPI IDirectMusicInstrumentImpl_SetPatch(LPDIRECTMUSICINSTRUMENT iface, DWORD dwPatch)
{
    IDirectMusicInstrumentImpl *This = impl_from_IDirectMusicInstrument(iface);

    TRACE("(%p)->(%d): stub\n", This, dwPatch);

    Patch2MIDILOCALE(dwPatch, &This->header.Locale);

    return S_OK;
}

static const IDirectMusicInstrumentVtbl DirectMusicInstrument_Vtbl =
{
    IDirectMusicInstrumentImpl_QueryInterface,
    IDirectMusicInstrumentImpl_AddRef,
    IDirectMusicInstrumentImpl_Release,
    IDirectMusicInstrumentImpl_GetPatch,
    IDirectMusicInstrumentImpl_SetPatch
};

/* for ClassFactory */
HRESULT DMUSIC_CreateDirectMusicInstrumentImpl (LPCGUID lpcGUID, LPVOID* ppobj, LPUNKNOWN pUnkOuter) {
	IDirectMusicInstrumentImpl* dminst;
        HRESULT hr;

	dminst = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDirectMusicInstrumentImpl));
	if (NULL == dminst) {
		*ppobj = NULL;
		return E_OUTOFMEMORY;
	}
	dminst->IDirectMusicInstrument_iface.lpVtbl = &DirectMusicInstrument_Vtbl;
        dminst->ref = 1;

        DMUSIC_LockModule();
        hr = IDirectMusicInstrument_QueryInterface(&dminst->IDirectMusicInstrument_iface, lpcGUID,
                ppobj);
        IDirectMusicInstrument_Release(&dminst->IDirectMusicInstrument_iface);

        return hr;
}

static HRESULT read_from_stream(IStream *stream, void *data, ULONG size)
{
    ULONG bytes_read;
    HRESULT hr;

    hr = IStream_Read(stream, data, size, &bytes_read);
    if(FAILED(hr)){
        TRACE("IStream_Read failed: %08x\n", hr);
        return hr;
    }
    if (bytes_read < size) {
        TRACE("Didn't read full chunk: %u < %u\n", bytes_read, size);
        return E_FAIL;
    }

    return S_OK;
}

static inline DWORD subtract_bytes(DWORD len, DWORD bytes)
{
    if(bytes > len){
        TRACE("Apparent mismatch in chunk lengths? %u bytes remaining, %u bytes read\n", len, bytes);
        return 0;
    }
    return len - bytes;
}

static inline HRESULT advance_stream(IStream *stream, ULONG bytes)
{
    LARGE_INTEGER move;
    HRESULT ret;

    move.QuadPart = bytes;

    ret = IStream_Seek(stream, move, STREAM_SEEK_CUR, NULL);
    if (FAILED(ret))
        WARN("IStream_Seek failed: %08x\n", ret);

    return ret;
}

static HRESULT load_region(IDirectMusicInstrumentImpl *This, IStream *stream, instrument_region *region, ULONG length)
{
    HRESULT ret;
    DMUS_PRIVATE_CHUNK chunk;

    TRACE("(%p, %p, %p, %u)\n", This, stream, region, length);

    while (length)
    {
        ret = read_from_stream(stream, &chunk, sizeof(chunk));
        if (FAILED(ret))
            return ret;

        length = subtract_bytes(length, sizeof(chunk));

        switch (chunk.fccID)
        {
            case FOURCC_RGNH:
                TRACE("RGNH chunk (region header): %u bytes\n", chunk.dwSize);

                ret = read_from_stream(stream, &region->header, sizeof(region->header));
                if (FAILED(ret))
                    return ret;

                length = subtract_bytes(length, sizeof(region->header));
                break;

            case FOURCC_WSMP:
                TRACE("WSMP chunk (wave sample): %u bytes\n", chunk.dwSize);

                ret = read_from_stream(stream, &region->wave_sample, sizeof(region->wave_sample));
                if (FAILED(ret))
                    return ret;
                length = subtract_bytes(length, sizeof(region->wave_sample));

                if (!(region->loop_present = (chunk.dwSize != sizeof(region->wave_sample))))
                    break;

                ret = read_from_stream(stream, &region->wave_loop, sizeof(region->wave_loop));
                if (FAILED(ret))
                    return ret;

                length = subtract_bytes(length, sizeof(region->wave_loop));
                break;

            case FOURCC_WLNK:
                TRACE("WLNK chunk (wave link): %u bytes\n", chunk.dwSize);

                ret = read_from_stream(stream, &region->wave_link, sizeof(region->wave_link));
                if (FAILED(ret))
                    return ret;

                length = subtract_bytes(length, sizeof(region->wave_link));
                break;

            default:
                TRACE("Unknown chunk %s (skipping): %u bytes\n", debugstr_fourcc(chunk.fccID), chunk.dwSize);

                ret = advance_stream(stream, chunk.dwSize);
                if (FAILED(ret))
                    return ret;

                length = subtract_bytes(length, chunk.dwSize);
                break;
        }
    }

    return S_OK;
}

static HRESULT load_articulation(IDirectMusicInstrumentImpl *This, IStream *stream, ULONG length)
{
    HRESULT ret;
    instrument_articulation *articulation;

    if (!This->articulations)
        This->articulations = HeapAlloc(GetProcessHeap(), 0, sizeof(*This->articulations));
    else
        This->articulations = HeapReAlloc(GetProcessHeap(), 0, This->articulations, sizeof(*This->articulations) * (This->nb_articulations + 1));
    if (!This->articulations)
        return E_OUTOFMEMORY;

    articulation = &This->articulations[This->nb_articulations];

    ret = read_from_stream(stream, &articulation->connections_list, sizeof(CONNECTIONLIST));
    if (FAILED(ret))
        return ret;

    articulation->connections = HeapAlloc(GetProcessHeap(), 0, sizeof(CONNECTION) * articulation->connections_list.cConnections);
    if (!articulation->connections)
        return E_OUTOFMEMORY;

    ret = read_from_stream(stream, articulation->connections, sizeof(CONNECTION) * articulation->connections_list.cConnections);
    if (FAILED(ret))
    {
        HeapFree(GetProcessHeap(), 0, articulation->connections);
        return ret;
    }

    subtract_bytes(length, sizeof(CONNECTIONLIST) + sizeof(CONNECTION) * articulation->connections_list.cConnections);

    This->nb_articulations++;

    return S_OK;
}

/* Function that loads all instrument data and which is called from IDirectMusicCollection_GetInstrument as in native */
HRESULT IDirectMusicInstrumentImpl_CustomLoad(IDirectMusicInstrument *iface, IStream *stream)
{
    IDirectMusicInstrumentImpl *This = impl_from_IDirectMusicInstrument(iface);
    HRESULT hr;
    DMUS_PRIVATE_CHUNK chunk;
    ULONG i = 0;
    ULONG length = This->length;

    TRACE("(%p, %p): offset = 0x%s, length = %u)\n", This, stream, wine_dbgstr_longlong(This->liInstrumentPosition.QuadPart), This->length);

    if (This->loaded)
        return S_OK;

    hr = IStream_Seek(stream, This->liInstrumentPosition, STREAM_SEEK_SET, NULL);
    if (FAILED(hr))
    {
        WARN("IStream_Seek failed: %08x\n", hr);
        return DMUS_E_UNSUPPORTED_STREAM;
    }

    This->regions = HeapAlloc(GetProcessHeap(), 0, sizeof(*This->regions) * This->header.cRegions);
    if (!This->regions)
        return E_OUTOFMEMORY;

    while (length)
    {
        hr = read_from_stream(stream, &chunk, sizeof(chunk));
        if (FAILED(hr))
            goto error;

        length = subtract_bytes(length, sizeof(chunk) + chunk.dwSize);

        switch (chunk.fccID)
        {
            case FOURCC_INSH:
            case FOURCC_DLID:
                TRACE("Chunk %s: %u bytes\n", debugstr_fourcc(chunk.fccID), chunk.dwSize);

                /* Instrument header and id are already set so just skip */
                hr = advance_stream(stream, chunk.dwSize);
                if (FAILED(hr))
                    goto error;

                break;

            case FOURCC_LIST: {
                DWORD size = chunk.dwSize;

                TRACE("LIST chunk: %u bytes\n", chunk.dwSize);

                hr = read_from_stream(stream, &chunk.fccID, sizeof(chunk.fccID));
                if (FAILED(hr))
                    goto error;

                size = subtract_bytes(size, sizeof(chunk.fccID));

                switch (chunk.fccID)
                {
                    case FOURCC_LRGN:
                        TRACE("LRGN chunk (regions list): %u bytes\n", size);

                        while (size)
                        {
                            hr = read_from_stream(stream, &chunk, sizeof(chunk));
                            if (FAILED(hr))
                                goto error;

                            if (chunk.fccID != FOURCC_LIST)
                            {
                                TRACE("Unknown chunk %s: %u bytes\n", debugstr_fourcc(chunk.fccID), chunk.dwSize);
                                goto error;
                            }

                            hr = read_from_stream(stream, &chunk.fccID, sizeof(chunk.fccID));
                            if (FAILED(hr))
                                goto error;

                            if (chunk.fccID == FOURCC_RGN)
                            {
                                TRACE("RGN chunk (region): %u bytes\n", chunk.dwSize);
                                hr = load_region(This, stream, &This->regions[i++], chunk.dwSize - sizeof(chunk.fccID));
                            }
                            else
                            {
                                TRACE("Unknown chunk %s: %u bytes\n", debugstr_fourcc(chunk.fccID), chunk.dwSize);
                                hr = advance_stream(stream, chunk.dwSize - sizeof(chunk.fccID));
                            }
                            if (FAILED(hr))
                                goto error;

                            size = subtract_bytes(size, chunk.dwSize + sizeof(chunk));
                        }
                        break;

                    case FOURCC_LART:
                        TRACE("LART chunk (articulations list): %u bytes\n", size);

                        while (size)
                        {
                            hr = read_from_stream(stream, &chunk, sizeof(chunk));
                            if (FAILED(hr))
                                goto error;

                            if (chunk.fccID == FOURCC_ART1)
                            {
                                TRACE("ART1 chunk (level 1 articulation): %u bytes\n", chunk.dwSize);
                                hr = load_articulation(This, stream, chunk.dwSize);
                            }
                            else
                            {
                                TRACE("Unknown chunk %s: %u bytes\n", debugstr_fourcc(chunk.fccID), chunk.dwSize);
                                hr = advance_stream(stream, chunk.dwSize);
                            }
                            if (FAILED(hr))
                                goto error;

                            size = subtract_bytes(size, chunk.dwSize + sizeof(chunk));
                        }
                        break;

                    default:
                        TRACE("Unknown chunk %s: %u bytes\n", debugstr_fourcc(chunk.fccID), chunk.dwSize);

                        hr = advance_stream(stream, chunk.dwSize - sizeof(chunk.fccID));
                        if (FAILED(hr))
                            goto error;

                        size = subtract_bytes(size, chunk.dwSize - sizeof(chunk.fccID));
                        break;
                }
                break;
            }

            default:
                TRACE("Unknown chunk %s: %u bytes\n", debugstr_fourcc(chunk.fccID), chunk.dwSize);

                hr = advance_stream(stream, chunk.dwSize);
                if (FAILED(hr))
                    goto error;

                break;
        }
    }

    This->loaded = TRUE;

    return S_OK;

error:
    HeapFree(GetProcessHeap(), 0, This->regions);
    This->regions = NULL;

    return DMUS_E_UNSUPPORTED_STREAM;
}
