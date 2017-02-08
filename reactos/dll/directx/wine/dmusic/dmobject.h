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
