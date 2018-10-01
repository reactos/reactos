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
#include "dmobject.h"
#include "wine/debug.h"

#include "dmusic_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(dmobj);
WINE_DECLARE_DEBUG_CHANNEL(dmfile);

/* RIFF format parsing */
#define CHUNK_HDR_SIZE (sizeof(FOURCC) + sizeof(DWORD))

#ifndef __REACTOS__
static inline const char *debugstr_fourcc(DWORD fourcc)
{
    if (!fourcc) return "''";
    return wine_dbg_sprintf("'%c%c%c%c'", (char)(fourcc), (char)(fourcc >> 8),
            (char)(fourcc >> 16), (char)(fourcc >> 24));
}
#endif

const char *debugstr_chunk(const struct chunk_entry *chunk)
{
    const char *type = "";

    if (!chunk)
        return "(null)";
    if (chunk->id == FOURCC_RIFF || chunk->id == FOURCC_LIST)
        type = wine_dbg_sprintf("type %s, ", debugstr_fourcc(chunk->type));
    return wine_dbg_sprintf("%s chunk, %ssize %u", debugstr_fourcc(chunk->id), type, chunk->size);
}

static HRESULT stream_read(IStream *stream, void *data, ULONG size)
{
    ULONG read;
    HRESULT hr;

    hr = IStream_Read(stream, data, size, &read);
    if (FAILED(hr))
        TRACE_(dmfile)("IStream_Read failed: %08x\n", hr);
    else if (!read && read < size) {
        /* All or nothing: Handle a partial read due to end of stream as an error */
        TRACE_(dmfile)("Short read: %u < %u\n", read, size);
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

    if (chunk->id == FOURCC_LIST || chunk->id == FOURCC_RIFF) {
        hr = stream_read(stream, &chunk->type, sizeof(FOURCC));
        if (hr != S_OK)
            return hr != S_FALSE ? hr : E_FAIL;
    }

    TRACE_(dmfile)("Returning %s\n", debugstr_chunk(chunk));

    return S_OK;
}

HRESULT stream_skip_chunk(IStream *stream, struct chunk_entry *chunk)
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

HRESULT stream_chunk_get_data(IStream *stream, const struct chunk_entry *chunk, void *data,
        ULONG size)
{
    if (chunk->size != size) {
        WARN_(dmfile)("Chunk %s (size %u, offset %s) doesn't contains the expected data size %u\n",
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
#ifndef __REACTOS__
    struct chunk_entry chunk = {.parent = info};
#else
    struct chunk_entry chunk = { 0, 0, 0, {{0}}, info };
#endif
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
#ifndef __REACTOS__
    struct chunk_entry chunk = {.parent = unfo};
#else
    struct chunk_entry chunk = { 0, 0, 0, {{0}}, unfo };
#endif

    while (stream_next_chunk(stream, &chunk) == S_OK)
        if (chunk.id == DMUS_FOURCC_UNAM_CHUNK || (inam && chunk.id == mmioFOURCC('I','N','A','M')))
            if (stream_chunk_get_wstr(stream, &chunk, desc->wszName, sizeof(desc->wszName)) == S_OK)
                desc->dwValidData |= DMUS_OBJ_NAME;
}

HRESULT dmobj_parsedescriptor(IStream *stream, const struct chunk_entry *riff,
        DMUS_OBJECTDESC *desc, DWORD supported)
{
#ifndef __REACTOS__
    struct chunk_entry chunk = {.parent = riff};
#else
    struct chunk_entry chunk = { 0, 0, 0, {{0}}, riff };
#endif
    HRESULT hr;

    TRACE("Looking for %#x in %p: %s\n", supported, stream, debugstr_chunk(riff));

    desc->dwValidData = 0;
    desc->dwSize = sizeof(*desc);

    while ((hr = stream_next_chunk(stream, &chunk)) == S_OK) {
        switch (chunk.id) {
            case DMUS_FOURCC_GUID_CHUNK:
                if ((supported & DMUS_OBJ_OBJECT) && stream_chunk_get_data(stream, &chunk,
                            &desc->guidObject, sizeof(desc->guidObject)) == S_OK)
                    desc->dwValidData |= DMUS_OBJ_OBJECT;
                break;
            case DMUS_FOURCC_CATEGORY_CHUNK:
                if ((supported & DMUS_OBJ_CATEGORY) && stream_chunk_get_wstr(stream, &chunk,
                            desc->wszCategory, sizeof(desc->wszCategory)) == S_OK)
                    desc->dwValidData |= DMUS_OBJ_CATEGORY;
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
    TRACE("Found %#x\n", desc->dwValidData);

    return hr;
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
