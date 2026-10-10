/*
 * Base IDirectMusicObject Implementation
 * Keep in sync with the master in dlls/dmusic/dmobject.h
 *
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

#include "wine/debug.h"

/* RIFF stream parsing */
struct chunk_entry;
struct chunk_entry {
    FOURCC id;
    DWORD size;
    FOURCC type;                        /* valid only for LIST and RIFF chunks */
    ULARGE_INTEGER offset;              /* chunk offset from start of stream */
    const struct chunk_entry *parent;   /* enclosing RIFF or LIST chunk */
};
#define MAKE_IDTYPE(id, type) (((UINT64)type << 32) | (UINT64)id)

HRESULT stream_read(IStream *stream, void *data, ULONG size);
HRESULT stream_get_chunk(IStream *stream, struct chunk_entry *chunk);
HRESULT stream_next_chunk(IStream *stream, struct chunk_entry *chunk);
HRESULT stream_skip_chunk(IStream *stream, const struct chunk_entry *chunk);

HRESULT stream_chunk_get_array(IStream *stream, const struct chunk_entry *chunk, void **array,
        unsigned int *count, DWORD elem_size);
HRESULT stream_chunk_get_data(IStream *stream, const struct chunk_entry *chunk, void *data,
        ULONG size);
HRESULT stream_chunk_get_wstr(IStream *stream, const struct chunk_entry *chunk, WCHAR *str,
        ULONG size);

HRESULT stream_get_loader(IStream *stream, IDirectMusicLoader **ret_loader);
HRESULT stream_get_object(IStream *stream, DMUS_OBJECTDESC *desc, REFIID riid, void **ret_iface);

static inline HRESULT stream_reset_chunk_data(IStream *stream, const struct chunk_entry *chunk)
{
    LARGE_INTEGER offset;

    offset.QuadPart = chunk->offset.QuadPart + sizeof(FOURCC) + sizeof(DWORD);
    if (chunk->id == FOURCC_RIFF || chunk->id == FOURCC_LIST)
        offset.QuadPart += sizeof(FOURCC);

    return IStream_Seek(stream, offset, STREAM_SEEK_SET, NULL);
}

static inline HRESULT stream_reset_chunk_start(IStream *stream, const struct chunk_entry *chunk)
{
    LARGE_INTEGER offset;

    offset.QuadPart = chunk->offset.QuadPart;

    return IStream_Seek(stream, offset, STREAM_SEEK_SET, NULL);
}


/* IDirectMusicObject base object */
struct dmobject {
    IDirectMusicObject IDirectMusicObject_iface;
    IPersistStream IPersistStream_iface;
    IUnknown *outer_unk;
    DMUS_OBJECTDESC desc;
};

void dmobject_init(struct dmobject *dmobj, const GUID *class, IUnknown *outer_unk);

/* Generic IDirectMusicObject methods */
HRESULT WINAPI dmobj_IDirectMusicObject_QueryInterface(IDirectMusicObject *iface, REFIID riid,
        void **ret_iface);
ULONG WINAPI dmobj_IDirectMusicObject_AddRef(IDirectMusicObject *iface);
ULONG WINAPI dmobj_IDirectMusicObject_Release(IDirectMusicObject *iface);
HRESULT WINAPI dmobj_IDirectMusicObject_GetDescriptor(IDirectMusicObject *iface,
        DMUS_OBJECTDESC *desc);
HRESULT WINAPI dmobj_IDirectMusicObject_SetDescriptor(IDirectMusicObject *iface,
        DMUS_OBJECTDESC *desc);

/* Helper for IDirectMusicObject::ParseDescriptor */
HRESULT dmobj_parsedescriptor(IStream *stream, const struct chunk_entry *riff,
        DMUS_OBJECTDESC *desc, DWORD supported);
/* Additional supported flags for dmobj_parsedescriptor.
   DMUS_OBJ_NAME is 'UNAM' chunk in UNFO list */
#define DMUS_OBJ_NAME_INAM   0x1000     /* 'INAM' chunk in UNFO list */
#define DMUS_OBJ_NAME_INFO   0x2000     /* 'INAM' chunk in INFO list */
#define DMUS_OBJ_GUID_DLID   0x4000     /* 'dlid' chunk instead of 'guid' */

/* 'DMRF' (reference list) helper */
HRESULT dmobj_parsereference(IStream *stream, const struct chunk_entry *list,
        IDirectMusicObject **dmobj);

/* Generic IPersistStream methods */
HRESULT WINAPI dmobj_IPersistStream_QueryInterface(IPersistStream *iface, REFIID riid,
        void **ret_iface);
ULONG WINAPI dmobj_IPersistStream_AddRef(IPersistStream *iface);
ULONG WINAPI dmobj_IPersistStream_Release(IPersistStream *iface);
HRESULT WINAPI dmobj_IPersistStream_GetClassID(IPersistStream *iface, CLSID *class);

/* IPersistStream methods not implemented in native */
HRESULT WINAPI unimpl_IPersistStream_GetClassID(IPersistStream *iface,
        CLSID *class);
HRESULT WINAPI unimpl_IPersistStream_IsDirty(IPersistStream *iface);
HRESULT WINAPI unimpl_IPersistStream_Save(IPersistStream *iface, IStream *stream,
        BOOL clear_dirty);
HRESULT WINAPI unimpl_IPersistStream_GetSizeMax(IPersistStream *iface,
        ULARGE_INTEGER *size);

/* Debugging helpers */
const char *debugstr_chunk(const struct chunk_entry *chunk);
const char *debugstr_dmguid(const GUID *id);
void dump_DMUS_OBJECTDESC(DMUS_OBJECTDESC *desc);
