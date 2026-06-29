/*
 * HGLOBAL Stream implementation
 *
 * Copyright 1999 Francis Beaudet
 * Copyright 2016 Dmitry Timoshkov
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#define COBJMACROS
#include "objbase.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(storage);

struct handle_wrapper
{
    LONG ref;
    HGLOBAL hglobal;
    ULONG size;
    BOOL delete_on_release;
};

static void handle_addref(struct handle_wrapper *handle)
{
    InterlockedIncrement(&handle->ref);
}

static void handle_release(struct handle_wrapper *handle)
{
    ULONG ref = InterlockedDecrement(&handle->ref);

    if (!ref)
    {
        if (handle->delete_on_release) GlobalFree(handle->hglobal);
        free(handle);
    }
}

static struct handle_wrapper *handle_create(HGLOBAL hglobal, BOOL delete_on_release)
{
    struct handle_wrapper *handle;

    handle = malloc(sizeof(*handle));
    if (!handle) return NULL;

    /* allocate a handle if one is not supplied */
    if (!hglobal) hglobal = GlobalAlloc(GMEM_MOVEABLE | GMEM_NODISCARD | GMEM_SHARE, 0);
    if (!hglobal)
    {
        free(handle);
        return NULL;
    }
    handle->ref = 1;
    handle->hglobal = hglobal;
    handle->size = GlobalSize(hglobal);
    handle->delete_on_release = delete_on_release;

    return handle;
}

struct hglobal_stream
{
    IStream IStream_iface;
    LONG ref;

    struct handle_wrapper *handle;
    ULARGE_INTEGER position;
};

static inline struct hglobal_stream *impl_from_IStream(IStream *iface)
{
    return CONTAINING_RECORD(iface, struct hglobal_stream, IStream_iface);
}

static const IStreamVtbl hglobalstreamvtbl;

static struct hglobal_stream *hglobalstream_construct(void)
{
    struct hglobal_stream *object = calloc(1, sizeof(*object));

    if (object)
    {
        object->IStream_iface.lpVtbl = &hglobalstreamvtbl;
        object->ref = 1;
    }
    return object;
}

static HRESULT WINAPI stream_QueryInterface(IStream *iface, REFIID riid, void **obj)
{
    if (!obj)
        return E_INVALIDARG;

    if (IsEqualIID(&IID_IUnknown, riid) ||
        IsEqualIID(&IID_ISequentialStream, riid) ||
        IsEqualIID(&IID_IStream, riid))
    {
        *obj = iface;
        IStream_AddRef(iface);
        return S_OK;
    }

    *obj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI stream_AddRef(IStream *iface)
{
    struct hglobal_stream *stream = impl_from_IStream(iface);
    return InterlockedIncrement(&stream->ref);
}

static ULONG WINAPI stream_Release(IStream *iface)
{
    struct hglobal_stream *stream = impl_from_IStream(iface);
    ULONG ref = InterlockedDecrement(&stream->ref);

    if (!ref)
    {
        handle_release(stream->handle);
        free(stream);
    }

    return ref;
}

static HRESULT WINAPI stream_Read(IStream *iface, void *pv, ULONG cb, ULONG *read_len)
{
    struct hglobal_stream *stream = impl_from_IStream(iface);
    ULONG dummy, len;
    char *buffer;

    TRACE("%p, %p, %ld, %p\n", iface, pv, cb, read_len);

    if (!read_len)
        read_len = &dummy;

    if (stream->handle->size >= stream->position.LowPart)
        len = min(stream->handle->size - stream->position.LowPart, cb);
    else
        len = 0;

    buffer = GlobalLock(stream->handle->hglobal);
    if (!buffer)
    {
        WARN("Failed to lock hglobal %p\n", stream->handle->hglobal);
        *read_len = 0;
        return S_OK;
    }

    memcpy(pv, buffer + stream->position.LowPart, len);
    stream->position.LowPart += len;

    *read_len = len;

    GlobalUnlock(stream->handle->hglobal);

    return S_OK;
}

static HRESULT WINAPI stream_Write(IStream *iface, const void *pv, ULONG cb, ULONG *written)
{
    struct hglobal_stream *stream = impl_from_IStream(iface);
    ULARGE_INTEGER size;
    ULONG dummy = 0;
    char *buffer;

    TRACE("%p, %p, %ld, %p\n", iface, pv, cb, written);

    if (!written)
        written = &dummy;

    if (!cb)
        goto out;

    *written = 0;

    size.HighPart = 0;
    size.LowPart = stream->position.LowPart + cb;

    if (size.LowPart > stream->handle->size)
    {
        /* grow stream */
        HRESULT hr = IStream_SetSize(iface, size);
        if (FAILED(hr))
        {
            ERR("IStream_SetSize failed with error %#lx\n", hr);
            return hr;
        }
    }

    buffer = GlobalLock(stream->handle->hglobal);
    if (!buffer)
    {
        WARN("write to invalid hglobal %p\n", stream->handle->hglobal);
        return S_OK;
    }

    memcpy(buffer + stream->position.LowPart, pv, cb);
    stream->position.LowPart += cb;

    GlobalUnlock(stream->handle->hglobal);

out:
    *written = cb;

    return S_OK;
}

static HRESULT WINAPI stream_Seek(IStream *iface, LARGE_INTEGER move, DWORD origin,
        ULARGE_INTEGER *pos)
{
    struct hglobal_stream *stream = impl_from_IStream(iface);
    ULARGE_INTEGER position = stream->position;
    HRESULT hr = S_OK;

    TRACE("%p, %s, %ld, %p\n", iface, wine_dbgstr_longlong(move.QuadPart), origin, pos);

    switch (origin)
    {
        case STREAM_SEEK_SET:
            position.QuadPart = 0;
            break;
        case STREAM_SEEK_CUR:
            break;
        case STREAM_SEEK_END:
            position.QuadPart = stream->handle->size;
            break;
        default:
            hr = STG_E_SEEKERROR;
            goto end;
    }

    position.HighPart = 0;
    position.LowPart += move.QuadPart;

    if (move.LowPart >= 0x80000000 && position.LowPart >= move.LowPart)
    {
        /* We tried to seek backwards and went past the start. */
        hr = STG_E_SEEKERROR;
        goto end;
    }

    stream->position = position;

end:
    if (pos) *pos = stream->position;

    return hr;
}

static HRESULT WINAPI stream_SetSize(IStream *iface, ULARGE_INTEGER size)
{
    struct hglobal_stream *stream = impl_from_IStream(iface);
    HGLOBAL hglobal;

    TRACE("%p, %s\n", iface, wine_dbgstr_longlong(size.QuadPart));

    if (stream->handle->size == size.LowPart)
        return S_OK;

    hglobal = GlobalReAlloc(stream->handle->hglobal, size.LowPart, GMEM_MOVEABLE);
    if (!hglobal)
        return E_OUTOFMEMORY;

    stream->handle->hglobal = hglobal;
    stream->handle->size = size.LowPart;

    return S_OK;
}

static HRESULT WINAPI stream_CopyTo(IStream *iface, IStream *dest, ULARGE_INTEGER cb,
        ULARGE_INTEGER *read_len, ULARGE_INTEGER *written)
{
    ULARGE_INTEGER total_read, total_written;
    HRESULT hr = S_OK;
    BYTE buffer[128];

    TRACE("%p, %p, %ld, %p, %p\n", iface, dest, cb.LowPart, read_len, written);

    if (!dest)
        return STG_E_INVALIDPOINTER;

    total_read.QuadPart = 0;
    total_written.QuadPart = 0;

    while (cb.QuadPart > 0)
    {
        ULONG chunk_size = cb.QuadPart >= sizeof(buffer) ? sizeof(buffer) : cb.LowPart;
        ULONG chunk_read, chunk_written;

        hr = IStream_Read(iface, buffer, chunk_size, &chunk_read);
        if (FAILED(hr))
            break;

        total_read.QuadPart += chunk_read;

        if (chunk_read)
        {
            hr = IStream_Write(dest, buffer, chunk_read, &chunk_written);
            if (FAILED(hr))
                break;

            total_written.QuadPart += chunk_written;
        }

        if (chunk_read != chunk_size)
            cb.QuadPart = 0;
        else
            cb.QuadPart -= chunk_read;
    }

    if (read_len)
        read_len->QuadPart = total_read.QuadPart;
    if (written)
        written->QuadPart = total_written.QuadPart;

    return hr;
}

static HRESULT WINAPI stream_Commit(IStream *iface, DWORD flags)
{
    return S_OK;
}

static HRESULT WINAPI stream_Revert(IStream *iface)
{
    return S_OK;
}

static HRESULT WINAPI stream_LockRegion(IStream *iface, ULARGE_INTEGER offset,
        ULARGE_INTEGER len, DWORD lock_type)
{
    return STG_E_INVALIDFUNCTION;
}

static HRESULT WINAPI stream_UnlockRegion(IStream *iface, ULARGE_INTEGER offset,
        ULARGE_INTEGER len, DWORD lock_type)
{
    return S_OK;
}

static HRESULT WINAPI stream_Stat(IStream *iface, STATSTG *pstatstg, DWORD flags)
{
    struct hglobal_stream *stream = impl_from_IStream(iface);

    memset(pstatstg, 0, sizeof(STATSTG));

    pstatstg->pwcsName = NULL;
    pstatstg->type = STGTY_STREAM;
    pstatstg->cbSize.QuadPart = stream->handle->size;

    return S_OK;
}

static HRESULT WINAPI stream_Clone(IStream *iface, IStream **ppstm)
{
    struct hglobal_stream *stream = impl_from_IStream(iface), *clone;
    ULARGE_INTEGER dummy;
    LARGE_INTEGER offset;

    TRACE("%p, %p\n", iface, ppstm);

    *ppstm = NULL;

    clone = hglobalstream_construct();
    if (!clone) return E_OUTOFMEMORY;

    *ppstm = &clone->IStream_iface;
    handle_addref(stream->handle);
    clone->handle = stream->handle;

    offset.QuadPart = (LONGLONG)stream->position.QuadPart;
    IStream_Seek(*ppstm, offset, STREAM_SEEK_SET, &dummy);
    return S_OK;
}

static const IStreamVtbl hglobalstreamvtbl =
{
    stream_QueryInterface,
    stream_AddRef,
    stream_Release,
    stream_Read,
    stream_Write,
    stream_Seek,
    stream_SetSize,
    stream_CopyTo,
    stream_Commit,
    stream_Revert,
    stream_LockRegion,
    stream_UnlockRegion,
    stream_Stat,
    stream_Clone
};

/***********************************************************************
 *           CreateStreamOnHGlobal     (combase.@)
 */
HRESULT WINAPI CreateStreamOnHGlobal(HGLOBAL hGlobal, BOOL delete_on_release, IStream **stream)
{
    struct hglobal_stream *object;

    if (!stream)
        return E_INVALIDARG;

    object = hglobalstream_construct();
    if (!object) return E_OUTOFMEMORY;

    object->handle = handle_create(hGlobal, delete_on_release);
    if (!object->handle)
    {
        free(object);
        return E_OUTOFMEMORY;
    }

    *stream = &object->IStream_iface;

    return S_OK;
}

/***********************************************************************
 *           GetHGlobalFromStream     (combase.@)
 */
HRESULT WINAPI GetHGlobalFromStream(IStream *stream, HGLOBAL *phglobal)
{
    struct hglobal_stream *object;

    if (!stream || !phglobal)
        return E_INVALIDARG;

    object = impl_from_IStream(stream);

    if (object->IStream_iface.lpVtbl == &hglobalstreamvtbl)
        *phglobal = object->handle->hglobal;
    else
    {
        *phglobal = 0;
        return E_INVALIDARG;
    }

    return S_OK;
}
