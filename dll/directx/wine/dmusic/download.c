/*
 * IDirectMusicDownload Implementation
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

struct download
{
    IDirectMusicDownload IDirectMusicDownload_iface;
    LONG ref;

    DWORD size;
    BYTE data[];
};

C_ASSERT(sizeof(struct download) == offsetof(struct download, data[0]));

static inline struct download *impl_from_IDirectMusicDownload(IDirectMusicDownload *iface)
{
    return CONTAINING_RECORD(iface, struct download, IDirectMusicDownload_iface);
}

static HRESULT WINAPI download_QueryInterface(IDirectMusicDownload *iface, REFIID riid, void **ret_iface)
{
    TRACE("(%p, %s, %p)\n", iface, debugstr_dmguid(riid), ret_iface);

    if (IsEqualIID(riid, &IID_IUnknown) ||
        IsEqualIID(riid, &IID_IDirectMusicDownload))
    {
        IDirectMusicDownload_AddRef(iface);
        *ret_iface = iface;
        return S_OK;
    }

    *ret_iface = NULL;
    WARN("(%p, %s, %p): not found\n", iface, debugstr_dmguid(riid), ret_iface);
    return E_NOINTERFACE;
}

static ULONG WINAPI download_AddRef(IDirectMusicDownload *iface)
{
    struct download *This = impl_from_IDirectMusicDownload(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p): new ref = %lu\n", iface, ref);

    return ref;
}

static ULONG WINAPI download_Release(IDirectMusicDownload *iface)
{
    struct download *This = impl_from_IDirectMusicDownload(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p): new ref = %lu\n", iface, ref);

    if (!ref) {
        free(This);
    }

    return ref;
}

static HRESULT WINAPI download_GetBuffer(IDirectMusicDownload *iface, void **buffer, DWORD *size)
{
    struct download *This = impl_from_IDirectMusicDownload(iface);

    TRACE("(%p, %p, %p)\n", iface, buffer, size);

    *buffer = This->data;
    *size = This->size;

    return S_OK;
}

static const IDirectMusicDownloadVtbl download_vtbl =
{
    download_QueryInterface,
    download_AddRef,
    download_Release,
    download_GetBuffer,
};

HRESULT download_create(DWORD size, IDirectMusicDownload **ret_iface)
{
    struct download *download;

    *ret_iface = NULL;
    if (!(download = malloc(offsetof(struct download, data[size])))) return E_OUTOFMEMORY;
    download->IDirectMusicDownload_iface.lpVtbl = &download_vtbl;
    download->ref = 1;
    download->size = size;

    TRACE("Created DirectMusicDownload %p\n", download);
    *ret_iface = &download->IDirectMusicDownload_iface;
    return S_OK;
}
