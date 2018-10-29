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

#pragma once

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

HRESULT stream_get_chunk(IStream *stream, struct chunk_entry *chunk) DECLSPEC_HIDDEN;
HRESULT stream_next_chunk(IStream *stream, struct chunk_entry *chunk) DECLSPEC_HIDDEN;
HRESULT stream_skip_chunk(IStream *stream, struct chunk_entry *chunk) DECLSPEC_HIDDEN;

HRESULT stream_chunk_get_data(IStream *stream, const struct chunk_entry *chunk, void *data,
        ULONG size) DECLSPEC_HIDDEN;
HRESULT stream_chunk_get_wstr(IStream *stream, const struct chunk_entry *chunk, WCHAR *str,
        ULONG size) DECLSPEC_HIDDEN;

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

const char *debugstr_chunk(const struct chunk_entry *chunk) DECLSPEC_HIDDEN;


/* IDirectMusicObject base object */
struct dmobject {
    IDirectMusicObject IDirectMusicObject_iface;
    IPersistStream IPersistStream_iface;
    IUnknown *outer_unk;
    DMUS_OBJECTDESC desc;
};

void dmobject_init(struct dmobject *dmobj, const GUID *class, IUnknown *outer_unk) DECLSPEC_HIDDEN;

/* Generic IDirectMusicObject methods */
HRESULT WINAPI dmobj_IDirectMusicObject_QueryInterface(IDirectMusicObject *iface, REFIID riid,
        void **ret_iface) DECLSPEC_HIDDEN;
ULONG WINAPI dmobj_IDirectMusicObject_AddRef(IDirectMusicObject *iface) DECLSPEC_HIDDEN;
ULONG WINAPI dmobj_IDirectMusicObject_Release(IDirectMusicObject *iface) DECLSPEC_HIDDEN;
HRESULT WINAPI dmobj_IDirectMusicObject_GetDescriptor(IDirectMusicObject *iface,
        DMUS_OBJECTDESC *desc) DECLSPEC_HIDDEN;
HRESULT WINAPI dmobj_IDirectMusicObject_SetDescriptor(IDirectMusicObject *iface,
        DMUS_OBJECTDESC *desc) DECLSPEC_HIDDEN;

/* Helper for IDirectMusicObject::ParseDescriptor */
HRESULT dmobj_parsedescriptor(IStream *stream, const struct chunk_entry *riff,
        DMUS_OBJECTDESC *desc, DWORD supported) DECLSPEC_HIDDEN;
/* Additional supported flags for dmobj_parsedescriptor.
   DMUS_OBJ_NAME is 'UNAM' chunk in UNFO list */
#define DMUS_OBJ_NAME_INAM   0x1000     /* 'INAM' chunk in UNFO list */
#define DMUS_OBJ_NAME_INFO   0x2000     /* 'INAM' chunk in INFO list */

/* Generic IPersistStream methods */
HRESULT WINAPI dmobj_IPersistStream_QueryInterface(IPersistStream *iface, REFIID riid,
        void **ret_iface) DECLSPEC_HIDDEN;
ULONG WINAPI dmobj_IPersistStream_AddRef(IPersistStream *iface) DECLSPEC_HIDDEN;
ULONG WINAPI dmobj_IPersistStream_Release(IPersistStream *iface) DECLSPEC_HIDDEN;
HRESULT WINAPI dmobj_IPersistStream_GetClassID(IPersistStream *iface, CLSID *class) DECLSPEC_HIDDEN;

/* IPersistStream methods not implemented in native */
HRESULT WINAPI unimpl_IPersistStream_GetClassID(IPersistStream *iface,
        CLSID *class) DECLSPEC_HIDDEN;
HRESULT WINAPI unimpl_IPersistStream_IsDirty(IPersistStream *iface) DECLSPEC_HIDDEN;
HRESULT WINAPI unimpl_IPersistStream_Save(IPersistStream *iface, IStream *stream,
        BOOL clear_dirty) DECLSPEC_HIDDEN;
HRESULT WINAPI unimpl_IPersistStream_GetSizeMax(IPersistStream *iface,
        ULARGE_INTEGER *size) DECLSPEC_HIDDEN;
