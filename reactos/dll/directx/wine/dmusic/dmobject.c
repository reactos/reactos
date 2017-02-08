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

#include "dmusic_private.h"

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
