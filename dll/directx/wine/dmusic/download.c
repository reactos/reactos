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

static inline IDirectMusicDownloadImpl* impl_from_IDirectMusicDownload(IDirectMusicDownload *iface)
{
    return CONTAINING_RECORD(iface, IDirectMusicDownloadImpl, IDirectMusicDownload_iface);
}

/* IDirectMusicDownloadImpl IUnknown part: */
static HRESULT WINAPI IDirectMusicDownloadImpl_QueryInterface(IDirectMusicDownload *iface, REFIID riid, void **ret_iface)
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

static ULONG WINAPI IDirectMusicDownloadImpl_AddRef(IDirectMusicDownload *iface)
{
    IDirectMusicDownloadImpl *This = impl_from_IDirectMusicDownload(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p)->(): new ref = %u\n", iface, ref);

    return ref;
}

static ULONG WINAPI IDirectMusicDownloadImpl_Release(IDirectMusicDownload *iface)
{
    IDirectMusicDownloadImpl *This = impl_from_IDirectMusicDownload(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p)->(): new ref = %u\n", iface, ref);

    if (!ref) {
        HeapFree(GetProcessHeap(), 0, This);
        DMUSIC_UnlockModule();
    }

    return ref;
}

/* IDirectMusicDownloadImpl IDirectMusicDownload part: */
static HRESULT WINAPI IDirectMusicDownloadImpl_GetBuffer(IDirectMusicDownload *iface, void **buffer, DWORD *size)
{
    FIXME("(%p, %p, %p): stub\n", iface, buffer, size);

    return S_OK;
}

static const IDirectMusicDownloadVtbl DirectMusicDownload_Vtbl = {
    IDirectMusicDownloadImpl_QueryInterface,
    IDirectMusicDownloadImpl_AddRef,
    IDirectMusicDownloadImpl_Release,
    IDirectMusicDownloadImpl_GetBuffer
};

/* for ClassFactory */
HRESULT DMUSIC_CreateDirectMusicDownloadImpl(const GUID *guid, void **ret_iface, IUnknown *unk_outer)
{
    IDirectMusicDownloadImpl *download;

    download = HeapAlloc(GetProcessHeap(), 0, sizeof(*download));
    if (!download)
    {
        *ret_iface = NULL;
        return E_OUTOFMEMORY;
    }

    download->IDirectMusicDownload_iface.lpVtbl = &DirectMusicDownload_Vtbl;
    download->ref = 1;
    *ret_iface = download;

    DMUSIC_LockModule();
    return S_OK;
}
