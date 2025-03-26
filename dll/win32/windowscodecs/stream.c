/*
 * Copyright 2009 Tony Wasserka
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

#include "wine/debug.h"

#define COBJMACROS
#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#include "objbase.h"
#include "shlwapi.h"
#include "wincodecs_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(wincodecs);

/******************************************
 * StreamOnMemory implementation
 *
 * Used by IWICStream_InitializeFromMemory
 *
 */
typedef struct StreamOnMemory {
    IStream IStream_iface;
    LONG ref;

    BYTE *pbMemory;
    DWORD dwMemsize;
    DWORD dwCurPos;

    CRITICAL_SECTION lock; /* must be held when pbMemory or dwCurPos is accessed */
} StreamOnMemory;

static inline StreamOnMemory *StreamOnMemory_from_IStream(IStream *iface)
{
    return CONTAINING_RECORD(iface, StreamOnMemory, IStream_iface);
}

static HRESULT WINAPI StreamOnMemory_QueryInterface(IStream *iface,
    REFIID iid, void **ppv)
{
    TRACE("(%p,%s,%p)\n", iface, debugstr_guid(iid), ppv);

    if (!ppv) return E_INVALIDARG;

    if (IsEqualIID(&IID_IUnknown, iid) || IsEqualIID(&IID_IStream, iid) ||
        IsEqualIID(&IID_ISequentialStream, iid))
    {
        *ppv = iface;
        IUnknown_AddRef((IUnknown*)*ppv);
        return S_OK;
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }
}

static ULONG WINAPI StreamOnMemory_AddRef(IStream *iface)
{
    StreamOnMemory *This = StreamOnMemory_from_IStream(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) refcount=%lu\n", iface, ref);

    return ref;
}

static ULONG WINAPI StreamOnMemory_Release(IStream *iface)
{
    StreamOnMemory *This = StreamOnMemory_from_IStream(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) refcount=%lu\n", iface, ref);

    if (ref == 0) {
        This->lock.DebugInfo->Spare[0] = 0;
        DeleteCriticalSection(&This->lock);
        free(This);
    }
    return ref;
}

static HRESULT WINAPI StreamOnMemory_Read(IStream *iface,
    void *pv, ULONG cb, ULONG *pcbRead)
{
    StreamOnMemory *This = StreamOnMemory_from_IStream(iface);
    ULONG uBytesRead;

    TRACE("(%p, %p, %lu, %p)\n", This, pv, cb, pcbRead);

    if (!pv) return E_INVALIDARG;

    EnterCriticalSection(&This->lock);
    uBytesRead = min(cb, This->dwMemsize - This->dwCurPos);
    memmove(pv, This->pbMemory + This->dwCurPos, uBytesRead);
    This->dwCurPos += uBytesRead;
    LeaveCriticalSection(&This->lock);

    if (pcbRead) *pcbRead = uBytesRead;

    return S_OK;
}

static HRESULT WINAPI StreamOnMemory_Write(IStream *iface,
    void const *pv, ULONG cb, ULONG *pcbWritten)
{
    StreamOnMemory *This = StreamOnMemory_from_IStream(iface);
    HRESULT hr;

    TRACE("(%p, %p, %lu, %p)\n", This, pv, cb, pcbWritten);

    if (!pv) return E_INVALIDARG;

    EnterCriticalSection(&This->lock);
    if (cb > This->dwMemsize - This->dwCurPos) {
        hr = STG_E_MEDIUMFULL;
    }
    else {
        memmove(This->pbMemory + This->dwCurPos, pv, cb);
        This->dwCurPos += cb;
        hr = S_OK;
        if (pcbWritten) *pcbWritten = cb;
    }
    LeaveCriticalSection(&This->lock);

    return hr;
}

static HRESULT WINAPI StreamOnMemory_Seek(IStream *iface,
    LARGE_INTEGER dlibMove, DWORD dwOrigin, ULARGE_INTEGER *plibNewPosition)
{
    StreamOnMemory *This = StreamOnMemory_from_IStream(iface);
    LARGE_INTEGER NewPosition;
    HRESULT hr=S_OK;

    TRACE("(%p, %s, %ld, %p)\n", This, wine_dbgstr_longlong(dlibMove.QuadPart), dwOrigin, plibNewPosition);

    EnterCriticalSection(&This->lock);
    if (dwOrigin == STREAM_SEEK_SET) NewPosition.QuadPart = dlibMove.QuadPart;
    else if (dwOrigin == STREAM_SEEK_CUR) NewPosition.QuadPart = This->dwCurPos + dlibMove.QuadPart;
    else if (dwOrigin == STREAM_SEEK_END) NewPosition.QuadPart = This->dwMemsize + dlibMove.QuadPart;
    else hr = E_INVALIDARG;

    if (SUCCEEDED(hr)) {
        if (NewPosition.u.HighPart) hr = HRESULT_FROM_WIN32(ERROR_ARITHMETIC_OVERFLOW);
        else if (NewPosition.QuadPart > This->dwMemsize) hr = E_INVALIDARG;
        else if (NewPosition.QuadPart < 0) hr = E_INVALIDARG;
    }

    if (SUCCEEDED(hr)) {
        This->dwCurPos = NewPosition.u.LowPart;

        if(plibNewPosition) plibNewPosition->QuadPart = This->dwCurPos;
    }
    LeaveCriticalSection(&This->lock);

    return hr;
}

/* SetSize isn't implemented in the native windowscodecs DLL either */
static HRESULT WINAPI StreamOnMemory_SetSize(IStream *iface,
    ULARGE_INTEGER libNewSize)
{
    TRACE("(%p, %s)\n", iface, wine_dbgstr_longlong(libNewSize.QuadPart));
    return E_NOTIMPL;
}

/* CopyTo isn't implemented in the native windowscodecs DLL either */
static HRESULT WINAPI StreamOnMemory_CopyTo(IStream *iface,
    IStream *pstm, ULARGE_INTEGER cb, ULARGE_INTEGER *pcbRead, ULARGE_INTEGER *pcbWritten)
{
    TRACE("(%p, %p, %s, %p, %p)\n", iface, pstm, wine_dbgstr_longlong(cb.QuadPart), pcbRead, pcbWritten);
    return E_NOTIMPL;
}

static HRESULT WINAPI StreamOnMemory_Commit(IStream *iface,
    DWORD grfCommitFlags)
{
    TRACE("(%p, %#lx)\n", iface, grfCommitFlags);
    return S_OK;
}

/* Revert isn't implemented in the native windowscodecs DLL either */
static HRESULT WINAPI StreamOnMemory_Revert(IStream *iface)
{
    TRACE("(%p)\n", iface);
    return E_NOTIMPL;
}

/* LockRegion isn't implemented in the native windowscodecs DLL either */
static HRESULT WINAPI StreamOnMemory_LockRegion(IStream *iface,
    ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType)
{
    TRACE("(%p, %s, %s, %ld)\n", iface, wine_dbgstr_longlong(libOffset.QuadPart),
        wine_dbgstr_longlong(cb.QuadPart), dwLockType);
    return E_NOTIMPL;
}

/* UnlockRegion isn't implemented in the native windowscodecs DLL either */
static HRESULT WINAPI StreamOnMemory_UnlockRegion(IStream *iface,
    ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType)
{
    TRACE("(%p, %s, %s, %ld)\n", iface, wine_dbgstr_longlong(libOffset.QuadPart),
        wine_dbgstr_longlong(cb.QuadPart), dwLockType);
    return E_NOTIMPL;
}

static HRESULT WINAPI StreamOnMemory_Stat(IStream *iface,
    STATSTG *pstatstg, DWORD grfStatFlag)
{
    StreamOnMemory *This = StreamOnMemory_from_IStream(iface);
    TRACE("(%p, %p, %#lx)\n", This, pstatstg, grfStatFlag);

    if (!pstatstg) return E_INVALIDARG;

    ZeroMemory(pstatstg, sizeof(STATSTG));
    pstatstg->type = STGTY_STREAM;
    pstatstg->cbSize.QuadPart = This->dwMemsize;

    return S_OK;
}

/* Clone isn't implemented in the native windowscodecs DLL either */
static HRESULT WINAPI StreamOnMemory_Clone(IStream *iface,
    IStream **ppstm)
{
    TRACE("(%p, %p)\n", iface, ppstm);
    return E_NOTIMPL;
}


static const IStreamVtbl StreamOnMemory_Vtbl =
{
    /*** IUnknown methods ***/
    StreamOnMemory_QueryInterface,
    StreamOnMemory_AddRef,
    StreamOnMemory_Release,
    /*** ISequentialStream methods ***/
    StreamOnMemory_Read,
    StreamOnMemory_Write,
    /*** IStream methods ***/
    StreamOnMemory_Seek,
    StreamOnMemory_SetSize,
    StreamOnMemory_CopyTo,
    StreamOnMemory_Commit,
    StreamOnMemory_Revert,
    StreamOnMemory_LockRegion,
    StreamOnMemory_UnlockRegion,
    StreamOnMemory_Stat,
    StreamOnMemory_Clone,
};

/******************************************
 * StreamOnFileHandle implementation (internal)
 *
 */
typedef struct StreamOnFileHandle {
    IStream IStream_iface;
    LONG ref;

    HANDLE map;
    void *mem;
    IWICStream *stream;
} StreamOnFileHandle;

static inline StreamOnFileHandle *StreamOnFileHandle_from_IStream(IStream *iface)
{
    return CONTAINING_RECORD(iface, StreamOnFileHandle, IStream_iface);
}

static HRESULT WINAPI StreamOnFileHandle_QueryInterface(IStream *iface,
    REFIID iid, void **ppv)
{
    TRACE("(%p,%s,%p)\n", iface, debugstr_guid(iid), ppv);

    if (!ppv) return E_INVALIDARG;

    if (IsEqualIID(&IID_IUnknown, iid) || IsEqualIID(&IID_IStream, iid) ||
        IsEqualIID(&IID_ISequentialStream, iid))
    {
        *ppv = iface;
        IUnknown_AddRef((IUnknown*)*ppv);
        return S_OK;
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }
}

static ULONG WINAPI StreamOnFileHandle_AddRef(IStream *iface)
{
    StreamOnFileHandle *This = StreamOnFileHandle_from_IStream(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) refcount=%lu\n", iface, ref);

    return ref;
}

static ULONG WINAPI StreamOnFileHandle_Release(IStream *iface)
{
    StreamOnFileHandle *This = StreamOnFileHandle_from_IStream(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) refcount=%lu\n", iface, ref);

    if (ref == 0) {
        IWICStream_Release(This->stream);
        UnmapViewOfFile(This->mem);
        CloseHandle(This->map);
        free(This);
    }
    return ref;
}

static HRESULT WINAPI StreamOnFileHandle_Read(IStream *iface,
    void *pv, ULONG cb, ULONG *pcbRead)
{
    StreamOnFileHandle *This = StreamOnFileHandle_from_IStream(iface);
    TRACE("(%p, %p, %lu, %p)\n", This, pv, cb, pcbRead);

    return IWICStream_Read(This->stream, pv, cb, pcbRead);
}

static HRESULT WINAPI StreamOnFileHandle_Write(IStream *iface,
    void const *pv, ULONG cb, ULONG *pcbWritten)
{
    ERR("(%p, %p, %lu, %p)\n", iface, pv, cb, pcbWritten);
    return HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED);
}

static HRESULT WINAPI StreamOnFileHandle_Seek(IStream *iface,
    LARGE_INTEGER dlibMove, DWORD dwOrigin, ULARGE_INTEGER *plibNewPosition)
{
    StreamOnFileHandle *This = StreamOnFileHandle_from_IStream(iface);
    TRACE("(%p, %s, %ld, %p)\n", This, wine_dbgstr_longlong(dlibMove.QuadPart), dwOrigin, plibNewPosition);

    return IWICStream_Seek(This->stream, dlibMove, dwOrigin, plibNewPosition);
}

static HRESULT WINAPI StreamOnFileHandle_SetSize(IStream *iface,
    ULARGE_INTEGER libNewSize)
{
    TRACE("(%p, %s)\n", iface, wine_dbgstr_longlong(libNewSize.QuadPart));
    return E_NOTIMPL;
}

static HRESULT WINAPI StreamOnFileHandle_CopyTo(IStream *iface,
    IStream *pstm, ULARGE_INTEGER cb, ULARGE_INTEGER *pcbRead, ULARGE_INTEGER *pcbWritten)
{
    TRACE("(%p, %p, %s, %p, %p)\n", iface, pstm, wine_dbgstr_longlong(cb.QuadPart), pcbRead, pcbWritten);
    return E_NOTIMPL;
}

static HRESULT WINAPI StreamOnFileHandle_Commit(IStream *iface,
    DWORD grfCommitFlags)
{
    TRACE("(%p, %#lx)\n", iface, grfCommitFlags);
    return S_OK;
}

static HRESULT WINAPI StreamOnFileHandle_Revert(IStream *iface)
{
    TRACE("(%p)\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI StreamOnFileHandle_LockRegion(IStream *iface,
    ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType)
{
    TRACE("(%p, %s, %s, %ld)\n", iface, wine_dbgstr_longlong(libOffset.QuadPart),
        wine_dbgstr_longlong(cb.QuadPart), dwLockType);
    return E_NOTIMPL;
}

static HRESULT WINAPI StreamOnFileHandle_UnlockRegion(IStream *iface,
    ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType)
{
    TRACE("(%p, %s, %s, %ld)\n", iface, wine_dbgstr_longlong(libOffset.QuadPart),
        wine_dbgstr_longlong(cb.QuadPart), dwLockType);
    return E_NOTIMPL;
}

static HRESULT WINAPI StreamOnFileHandle_Stat(IStream *iface,
    STATSTG *pstatstg, DWORD grfStatFlag)
{
    StreamOnFileHandle *This = StreamOnFileHandle_from_IStream(iface);
    TRACE("(%p, %p, %#lx)\n", This, pstatstg, grfStatFlag);

    return IWICStream_Stat(This->stream, pstatstg, grfStatFlag);
}

static HRESULT WINAPI StreamOnFileHandle_Clone(IStream *iface,
    IStream **ppstm)
{
    TRACE("(%p, %p)\n", iface, ppstm);
    return E_NOTIMPL;
}

static const IStreamVtbl StreamOnFileHandle_Vtbl =
{
    /*** IUnknown methods ***/
    StreamOnFileHandle_QueryInterface,
    StreamOnFileHandle_AddRef,
    StreamOnFileHandle_Release,
    /*** ISequentialStream methods ***/
    StreamOnFileHandle_Read,
    StreamOnFileHandle_Write,
    /*** IStream methods ***/
    StreamOnFileHandle_Seek,
    StreamOnFileHandle_SetSize,
    StreamOnFileHandle_CopyTo,
    StreamOnFileHandle_Commit,
    StreamOnFileHandle_Revert,
    StreamOnFileHandle_LockRegion,
    StreamOnFileHandle_UnlockRegion,
    StreamOnFileHandle_Stat,
    StreamOnFileHandle_Clone,
};

/******************************************
 * StreamOnStreamRange implementation
 *
 * Used by IWICStream_InitializeFromIStreamRegion
 *
 */
typedef struct StreamOnStreamRange {
    IStream IStream_iface;
    LONG ref;

    IStream *stream;
    ULARGE_INTEGER pos;
    ULARGE_INTEGER offset;
    ULARGE_INTEGER max_size;

    CRITICAL_SECTION lock;
} StreamOnStreamRange;

static inline StreamOnStreamRange *StreamOnStreamRange_from_IStream(IStream *iface)
{
    return CONTAINING_RECORD(iface, StreamOnStreamRange, IStream_iface);
}

static HRESULT WINAPI StreamOnStreamRange_QueryInterface(IStream *iface,
    REFIID iid, void **ppv)
{
    TRACE("(%p,%s,%p)\n", iface, debugstr_guid(iid), ppv);

    if (!ppv) return E_INVALIDARG;

    if (IsEqualIID(&IID_IUnknown, iid) || IsEqualIID(&IID_IStream, iid) ||
        IsEqualIID(&IID_ISequentialStream, iid))
    {
        *ppv = iface;
        IUnknown_AddRef((IUnknown*)*ppv);
        return S_OK;
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }
}

static ULONG WINAPI StreamOnStreamRange_AddRef(IStream *iface)
{
    StreamOnStreamRange *This = StreamOnStreamRange_from_IStream(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) refcount=%lu\n", iface, ref);

    return ref;
}

static ULONG WINAPI StreamOnStreamRange_Release(IStream *iface)
{
    StreamOnStreamRange *This = StreamOnStreamRange_from_IStream(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) refcount=%lu\n", iface, ref);

    if (ref == 0) {
        This->lock.DebugInfo->Spare[0] = 0;
        DeleteCriticalSection(&This->lock);
        IStream_Release(This->stream);
        free(This);
    }
    return ref;
}

static HRESULT WINAPI StreamOnStreamRange_Read(IStream *iface,
    void *pv, ULONG cb, ULONG *pcbRead)
{
    StreamOnStreamRange *This = StreamOnStreamRange_from_IStream(iface);
    ULONG uBytesRead=0;
    HRESULT hr;
    ULARGE_INTEGER OldPosition;
    LARGE_INTEGER SetPosition;

    TRACE("(%p, %p, %lu, %p)\n", This, pv, cb, pcbRead);

    if (!pv) return E_INVALIDARG;

    EnterCriticalSection(&This->lock);
    SetPosition.QuadPart = 0;
    hr = IStream_Seek(This->stream, SetPosition, STREAM_SEEK_CUR, &OldPosition);
    if (SUCCEEDED(hr))
    {
        SetPosition.QuadPart = This->pos.QuadPart + This->offset.QuadPart;
        hr = IStream_Seek(This->stream, SetPosition, STREAM_SEEK_SET, NULL);
    }
    if (SUCCEEDED(hr))
    {
        if (This->pos.QuadPart + cb > This->max_size.QuadPart)
        {
            /* This would read past the end of the stream. */
            if (This->pos.QuadPart > This->max_size.QuadPart)
                cb = 0;
            else
                cb = This->max_size.QuadPart - This->pos.QuadPart;
        }
        hr = IStream_Read(This->stream, pv, cb, &uBytesRead);
        SetPosition.QuadPart = OldPosition.QuadPart;
        IStream_Seek(This->stream, SetPosition, STREAM_SEEK_SET, NULL);
    }
    if (SUCCEEDED(hr))
        This->pos.QuadPart += uBytesRead;
    LeaveCriticalSection(&This->lock);

    if (SUCCEEDED(hr) && pcbRead) *pcbRead = uBytesRead;

    return hr;
}

static HRESULT WINAPI StreamOnStreamRange_Write(IStream *iface,
    void const *pv, ULONG cb, ULONG *pcbWritten)
{
    StreamOnStreamRange *This = StreamOnStreamRange_from_IStream(iface);
    HRESULT hr;
    ULARGE_INTEGER OldPosition;
    LARGE_INTEGER SetPosition;
    ULONG uBytesWritten=0;
    TRACE("(%p, %p, %lu, %p)\n", This, pv, cb, pcbWritten);

    if (!pv) return E_INVALIDARG;

    EnterCriticalSection(&This->lock);
    SetPosition.QuadPart = 0;
    hr = IStream_Seek(This->stream, SetPosition, STREAM_SEEK_CUR, &OldPosition);
    if (SUCCEEDED(hr))
    {
        SetPosition.QuadPart = This->pos.QuadPart + This->offset.QuadPart;
        hr = IStream_Seek(This->stream, SetPosition, STREAM_SEEK_SET, NULL);
    }
    if (SUCCEEDED(hr))
    {
        if (This->pos.QuadPart + cb > This->max_size.QuadPart)
        {
            /* This would read past the end of the stream. */
            if (This->pos.QuadPart > This->max_size.QuadPart)
                cb = 0;
            else
                cb = This->max_size.QuadPart - This->pos.QuadPart;
        }
        hr = IStream_Write(This->stream, pv, cb, &uBytesWritten);
        SetPosition.QuadPart = OldPosition.QuadPart;
        IStream_Seek(This->stream, SetPosition, STREAM_SEEK_SET, NULL);
    }
    if (SUCCEEDED(hr))
        This->pos.QuadPart += uBytesWritten;
    LeaveCriticalSection(&This->lock);

    if (SUCCEEDED(hr) && pcbWritten) *pcbWritten = uBytesWritten;

    return hr;
}

static HRESULT WINAPI StreamOnStreamRange_Seek(IStream *iface,
    LARGE_INTEGER dlibMove, DWORD dwOrigin, ULARGE_INTEGER *plibNewPosition)
{
    StreamOnStreamRange *This = StreamOnStreamRange_from_IStream(iface);
    ULARGE_INTEGER NewPosition, actual_size;
    HRESULT hr=S_OK;
    STATSTG statstg;
    TRACE("(%p, %s, %ld, %p)\n", This, wine_dbgstr_longlong(dlibMove.QuadPart), dwOrigin, plibNewPosition);

    EnterCriticalSection(&This->lock);
    actual_size = This->max_size;
    if (dwOrigin == STREAM_SEEK_SET)
        NewPosition.QuadPart = dlibMove.QuadPart;
    else if (dwOrigin == STREAM_SEEK_CUR)
        NewPosition.QuadPart = This->pos.QuadPart + dlibMove.QuadPart;
    else if (dwOrigin == STREAM_SEEK_END)
    {
        hr = IStream_Stat(This->stream, &statstg, STATFLAG_NONAME);
        if (SUCCEEDED(hr))
        {
            if (This->max_size.QuadPart + This->offset.QuadPart > statstg.cbSize.QuadPart)
                actual_size.QuadPart = statstg.cbSize.QuadPart - This->offset.QuadPart;
            NewPosition.QuadPart = dlibMove.QuadPart + actual_size.QuadPart;
        }
    }
    else hr = E_INVALIDARG;

    if (SUCCEEDED(hr) && (NewPosition.u.HighPart != 0 || NewPosition.QuadPart > actual_size.QuadPart))
        hr = WINCODEC_ERR_VALUEOUTOFRANGE;

    if (SUCCEEDED(hr)) {
        This->pos.QuadPart = NewPosition.QuadPart;

        if(plibNewPosition) plibNewPosition->QuadPart = This->pos.QuadPart;
    }
    LeaveCriticalSection(&This->lock);

    return hr;
}

/* SetSize isn't implemented in the native windowscodecs DLL either */
static HRESULT WINAPI StreamOnStreamRange_SetSize(IStream *iface,
    ULARGE_INTEGER libNewSize)
{
    TRACE("(%p, %s)\n", iface, wine_dbgstr_longlong(libNewSize.QuadPart));
    return E_NOTIMPL;
}

/* CopyTo isn't implemented in the native windowscodecs DLL either */
static HRESULT WINAPI StreamOnStreamRange_CopyTo(IStream *iface,
    IStream *pstm, ULARGE_INTEGER cb, ULARGE_INTEGER *pcbRead, ULARGE_INTEGER *pcbWritten)
{
    TRACE("(%p, %p, %s, %p, %p)\n", iface, pstm, wine_dbgstr_longlong(cb.QuadPart),
        pcbRead, pcbWritten);
    return E_NOTIMPL;
}

/* Commit isn't implemented in the native windowscodecs DLL either */
static HRESULT WINAPI StreamOnStreamRange_Commit(IStream *iface,
    DWORD grfCommitFlags)
{
    StreamOnStreamRange *This = StreamOnStreamRange_from_IStream(iface);
    TRACE("(%p, %#lx)\n", This, grfCommitFlags);
    return IStream_Commit(This->stream, grfCommitFlags);
}

/* Revert isn't implemented in the native windowscodecs DLL either */
static HRESULT WINAPI StreamOnStreamRange_Revert(IStream *iface)
{
    TRACE("(%p)\n", iface);
    return E_NOTIMPL;
}

/* LockRegion isn't implemented in the native windowscodecs DLL either */
static HRESULT WINAPI StreamOnStreamRange_LockRegion(IStream *iface,
    ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType)
{
    TRACE("(%p, %s, %s, %ld)\n", iface, wine_dbgstr_longlong(libOffset.QuadPart),
        wine_dbgstr_longlong(cb.QuadPart), dwLockType);
    return E_NOTIMPL;
}

/* UnlockRegion isn't implemented in the native windowscodecs DLL either */
static HRESULT WINAPI StreamOnStreamRange_UnlockRegion(IStream *iface,
    ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType)
{
    TRACE("(%p, %s, %s, %ld)\n", iface, wine_dbgstr_longlong(libOffset.QuadPart),
        wine_dbgstr_longlong(cb.QuadPart), dwLockType);
    return E_NOTIMPL;
}

static HRESULT WINAPI StreamOnStreamRange_Stat(IStream *iface,
    STATSTG *pstatstg, DWORD grfStatFlag)
{
    StreamOnStreamRange *This = StreamOnStreamRange_from_IStream(iface);
    HRESULT hr;
    TRACE("(%p, %p, %#lx)\n", This, pstatstg, grfStatFlag);

    if (!pstatstg) return E_INVALIDARG;

    EnterCriticalSection(&This->lock);
    hr = IStream_Stat(This->stream, pstatstg, grfStatFlag);
    if (SUCCEEDED(hr))
    {
        pstatstg->cbSize.QuadPart -= This->offset.QuadPart;
        if (This->max_size.QuadPart < pstatstg->cbSize.QuadPart)
            pstatstg->cbSize.QuadPart = This->max_size.QuadPart;
    }

    LeaveCriticalSection(&This->lock);

    return hr;
}

/* Clone isn't implemented in the native windowscodecs DLL either */
static HRESULT WINAPI StreamOnStreamRange_Clone(IStream *iface,
    IStream **ppstm)
{
    TRACE("(%p, %p)\n", iface, ppstm);
    return E_NOTIMPL;
}

static const IStreamVtbl StreamOnStreamRange_Vtbl =
{
    /*** IUnknown methods ***/
    StreamOnStreamRange_QueryInterface,
    StreamOnStreamRange_AddRef,
    StreamOnStreamRange_Release,
    /*** ISequentialStream methods ***/
    StreamOnStreamRange_Read,
    StreamOnStreamRange_Write,
    /*** IStream methods ***/
    StreamOnStreamRange_Seek,
    StreamOnStreamRange_SetSize,
    StreamOnStreamRange_CopyTo,
    StreamOnStreamRange_Commit,
    StreamOnStreamRange_Revert,
    StreamOnStreamRange_LockRegion,
    StreamOnStreamRange_UnlockRegion,
    StreamOnStreamRange_Stat,
    StreamOnStreamRange_Clone,
};


/******************************************
 * IWICStream implementation
 *
 */
typedef struct IWICStreamImpl
{
    IWICStream IWICStream_iface;
    LONG ref;

    IStream *pStream;
} IWICStreamImpl;

static inline IWICStreamImpl *impl_from_IWICStream(IWICStream *iface)
{
    return CONTAINING_RECORD(iface, IWICStreamImpl, IWICStream_iface);
}

static HRESULT WINAPI IWICStreamImpl_QueryInterface(IWICStream *iface,
    REFIID iid, void **ppv)
{
    IWICStreamImpl *This = impl_from_IWICStream(iface);
    TRACE("(%p,%s,%p)\n", iface, debugstr_guid(iid), ppv);

    if (!ppv) return E_INVALIDARG;

    if (IsEqualIID(&IID_IUnknown, iid) || IsEqualIID(&IID_IStream, iid) ||
        IsEqualIID(&IID_ISequentialStream, iid) || IsEqualIID(&IID_IWICStream, iid))
    {
        *ppv = &This->IWICStream_iface;
        IUnknown_AddRef((IUnknown*)*ppv);
        return S_OK;
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }
}

static ULONG WINAPI IWICStreamImpl_AddRef(IWICStream *iface)
{
    IWICStreamImpl *This = impl_from_IWICStream(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) refcount=%lu\n", iface, ref);

    return ref;
}

static ULONG WINAPI IWICStreamImpl_Release(IWICStream *iface)
{
    IWICStreamImpl *This = impl_from_IWICStream(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) refcount=%lu\n", iface, ref);

    if (ref == 0) {
        if (This->pStream) IStream_Release(This->pStream);
        free(This);
    }
    return ref;
}

static HRESULT WINAPI IWICStreamImpl_Read(IWICStream *iface,
    void *pv, ULONG cb, ULONG *pcbRead)
{
    IWICStreamImpl *This = impl_from_IWICStream(iface);
    TRACE("(%p, %p, %lu, %p)\n", This, pv, cb, pcbRead);

    if (!This->pStream) return WINCODEC_ERR_NOTINITIALIZED;
    return IStream_Read(This->pStream, pv, cb, pcbRead);
}

static HRESULT WINAPI IWICStreamImpl_Write(IWICStream *iface,
    void const *pv, ULONG cb, ULONG *pcbWritten)
{
    IWICStreamImpl *This = impl_from_IWICStream(iface);
    TRACE("(%p, %p, %lu, %p)\n", This, pv, cb, pcbWritten);

    if (!This->pStream) return WINCODEC_ERR_NOTINITIALIZED;
    return IStream_Write(This->pStream, pv, cb, pcbWritten);
}

static HRESULT WINAPI IWICStreamImpl_Seek(IWICStream *iface,
    LARGE_INTEGER dlibMove, DWORD dwOrigin, ULARGE_INTEGER *plibNewPosition)
{
    IWICStreamImpl *This = impl_from_IWICStream(iface);
    TRACE("(%p, %s, %ld, %p)\n", This, wine_dbgstr_longlong(dlibMove.QuadPart),
        dwOrigin, plibNewPosition);

    if (!This->pStream) return WINCODEC_ERR_NOTINITIALIZED;
    return IStream_Seek(This->pStream, dlibMove, dwOrigin, plibNewPosition);
}

static HRESULT WINAPI IWICStreamImpl_SetSize(IWICStream *iface,
    ULARGE_INTEGER libNewSize)
{
    IWICStreamImpl *This = impl_from_IWICStream(iface);
    TRACE("(%p, %s)\n", This, wine_dbgstr_longlong(libNewSize.QuadPart));

    if (!This->pStream) return WINCODEC_ERR_NOTINITIALIZED;
    return IStream_SetSize(This->pStream, libNewSize);
}

static HRESULT WINAPI IWICStreamImpl_CopyTo(IWICStream *iface,
    IStream *pstm, ULARGE_INTEGER cb, ULARGE_INTEGER *pcbRead, ULARGE_INTEGER *pcbWritten)
{
    IWICStreamImpl *This = impl_from_IWICStream(iface);
    TRACE("(%p, %p, %s, %p, %p)\n", This, pstm, wine_dbgstr_longlong(cb.QuadPart), pcbRead, pcbWritten);

    if (!This->pStream) return WINCODEC_ERR_NOTINITIALIZED;
    return IStream_CopyTo(This->pStream, pstm, cb, pcbRead, pcbWritten);
}

static HRESULT WINAPI IWICStreamImpl_Commit(IWICStream *iface,
    DWORD grfCommitFlags)
{
    IWICStreamImpl *This = impl_from_IWICStream(iface);
    TRACE("(%p, %#lx)\n", This, grfCommitFlags);

    if (!This->pStream) return WINCODEC_ERR_NOTINITIALIZED;
    return IStream_Commit(This->pStream, grfCommitFlags);
}

static HRESULT WINAPI IWICStreamImpl_Revert(IWICStream *iface)
{
    IWICStreamImpl *This = impl_from_IWICStream(iface);
    TRACE("(%p)\n", This);

    if (!This->pStream) return WINCODEC_ERR_NOTINITIALIZED;
    return IStream_Revert(This->pStream);
}

static HRESULT WINAPI IWICStreamImpl_LockRegion(IWICStream *iface,
    ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType)
{
    IWICStreamImpl *This = impl_from_IWICStream(iface);
    TRACE("(%p, %s, %s, %ld)\n", This, wine_dbgstr_longlong(libOffset.QuadPart),
        wine_dbgstr_longlong(cb.QuadPart), dwLockType);

    if (!This->pStream) return WINCODEC_ERR_NOTINITIALIZED;
    return IStream_LockRegion(This->pStream, libOffset, cb, dwLockType);
}

static HRESULT WINAPI IWICStreamImpl_UnlockRegion(IWICStream *iface,
    ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType)
{
    IWICStreamImpl *This = impl_from_IWICStream(iface);
    TRACE("(%p, %s, %s, %ld)\n", This, wine_dbgstr_longlong(libOffset.QuadPart),
        wine_dbgstr_longlong(cb.QuadPart), dwLockType);

    if (!This->pStream) return WINCODEC_ERR_NOTINITIALIZED;
    return IStream_UnlockRegion(This->pStream, libOffset, cb, dwLockType);
}

static HRESULT WINAPI IWICStreamImpl_Stat(IWICStream *iface,
    STATSTG *pstatstg, DWORD grfStatFlag)
{
    IWICStreamImpl *This = impl_from_IWICStream(iface);
    TRACE("(%p, %p, %#lx)\n", This, pstatstg, grfStatFlag);

    if (!This->pStream) return WINCODEC_ERR_NOTINITIALIZED;
    return IStream_Stat(This->pStream, pstatstg, grfStatFlag);
}

static HRESULT WINAPI IWICStreamImpl_Clone(IWICStream *iface,
    IStream **ppstm)
{
    IWICStreamImpl *This = impl_from_IWICStream(iface);
    TRACE("(%p, %p)\n", This, ppstm);

    if (!This->pStream) return WINCODEC_ERR_NOTINITIALIZED;
    return IStream_Clone(This->pStream, ppstm);
}

static HRESULT WINAPI IWICStreamImpl_InitializeFromIStream(IWICStream *iface, IStream *stream)
{
    IWICStreamImpl *This = impl_from_IWICStream(iface);
    HRESULT hr = S_OK;

    TRACE("(%p, %p)\n", iface, stream);

    if (!stream) return E_INVALIDARG;
    if (This->pStream) return WINCODEC_ERR_WRONGSTATE;

    IStream_AddRef(stream);

    if (InterlockedCompareExchangePointer((void **)&This->pStream, stream, NULL))
    {
        /* Some other thread set the stream first. */
        IStream_Release(stream);
        hr = WINCODEC_ERR_WRONGSTATE;
    }

    return hr;
}

static HRESULT WINAPI IWICStreamImpl_InitializeFromFilename(IWICStream *iface,
    LPCWSTR wzFileName, DWORD dwDesiredAccess)
{
    IWICStreamImpl *This = impl_from_IWICStream(iface);
    HRESULT hr;
    DWORD dwMode;
    IStream *stream;

    TRACE("(%p, %s, %lu)\n", iface, debugstr_w(wzFileName), dwDesiredAccess);

    if (This->pStream) return WINCODEC_ERR_WRONGSTATE;

    if(dwDesiredAccess & GENERIC_WRITE)
        dwMode = STGM_SHARE_DENY_WRITE | STGM_WRITE | STGM_CREATE;
    else if(dwDesiredAccess & GENERIC_READ)
        dwMode = STGM_SHARE_DENY_WRITE | STGM_READ | STGM_FAILIFTHERE;
    else
        return E_INVALIDARG;

    hr = SHCreateStreamOnFileW(wzFileName, dwMode, &stream);

    if (SUCCEEDED(hr))
    {
        if (InterlockedCompareExchangePointer((void**)&This->pStream, stream, NULL))
        {
            /* Some other thread set the stream first. */
            IStream_Release(stream);
            hr = WINCODEC_ERR_WRONGSTATE;
        }
    }

    return hr;
}

/******************************************
 * IWICStream_InitializeFromMemory
 *
 * Initializes the internal IStream object to retrieve its data from a memory chunk.
 *
 * PARAMS
 *   pbBuffer     [I] pointer to the memory chunk
 *   cbBufferSize [I] number of bytes to use from the memory chunk
 *
 * RETURNS
 *   SUCCESS: S_OK
 *   FAILURE: E_INVALIDARG, if pbBuffer is NULL
 *            E_OUTOFMEMORY, if we run out of memory
 *            WINCODEC_ERR_WRONGSTATE, if the IStream object has already been initialized before
 *
 */
static HRESULT WINAPI IWICStreamImpl_InitializeFromMemory(IWICStream *iface,
    BYTE *pbBuffer, DWORD cbBufferSize)
{
    IWICStreamImpl *This = impl_from_IWICStream(iface);
    StreamOnMemory *pObject;
    TRACE("(%p, %p, %lu)\n", iface, pbBuffer, cbBufferSize);

    if (!pbBuffer) return E_INVALIDARG;
    if (This->pStream) return WINCODEC_ERR_WRONGSTATE;

    pObject = malloc(sizeof(StreamOnMemory));
    if (!pObject) return E_OUTOFMEMORY;

    pObject->IStream_iface.lpVtbl = &StreamOnMemory_Vtbl;
    pObject->ref = 1;
    pObject->pbMemory = pbBuffer;
    pObject->dwMemsize = cbBufferSize;
    pObject->dwCurPos = 0;
#ifdef __REACTOS__
    InitializeCriticalSection(&pObject->lock);
#else
    InitializeCriticalSectionEx(&pObject->lock, 0, RTL_CRITICAL_SECTION_FLAG_FORCE_DEBUG_INFO);
#endif
    pObject->lock.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": StreamOnMemory.lock");

    if (InterlockedCompareExchangePointer((void**)&This->pStream, pObject, NULL))
    {
        /* Some other thread set the stream first. */
        IStream_Release(&pObject->IStream_iface);
        return WINCODEC_ERR_WRONGSTATE;
    }

    return S_OK;
}

static HRESULT map_file(HANDLE file, HANDLE *map, void **mem, LARGE_INTEGER *size)
{
    *map = NULL;
    if (!GetFileSizeEx(file, size)) return HRESULT_FROM_WIN32(GetLastError());
    if (size->u.HighPart)
    {
        WARN("file too large\n");
        return E_FAIL;
    }
    if (!(*map = CreateFileMappingW(file, NULL, PAGE_READONLY, 0, size->u.LowPart, NULL)))
    {
        return HRESULT_FROM_WIN32(GetLastError());
    }
    if (!(*mem = MapViewOfFile(*map, FILE_MAP_READ, 0, 0, size->u.LowPart)))
    {
        CloseHandle(*map);
        return HRESULT_FROM_WIN32(GetLastError());
    }
    return S_OK;
}

HRESULT stream_initialize_from_filehandle(IWICStream *iface, HANDLE file)
{
    IWICStreamImpl *This = impl_from_IWICStream(iface);
    StreamOnFileHandle *pObject;
    IWICStream *stream = NULL;
    HANDLE map;
    void *mem;
    LARGE_INTEGER size;
    HRESULT hr;
    TRACE("(%p,%p)\n", iface, file);

    if (This->pStream) return WINCODEC_ERR_WRONGSTATE;

    hr = map_file(file, &map, &mem, &size);
    if (FAILED(hr)) return hr;

    hr = StreamImpl_Create(&stream);
    if (FAILED(hr)) goto error;

    hr = IWICStreamImpl_InitializeFromMemory(stream, mem, size.u.LowPart);
    if (FAILED(hr)) goto error;

    pObject = malloc(sizeof(StreamOnFileHandle));
    if (!pObject)
    {
        hr = E_OUTOFMEMORY;
        goto error;
    }
    pObject->IStream_iface.lpVtbl = &StreamOnFileHandle_Vtbl;
    pObject->ref = 1;
    pObject->map = map;
    pObject->mem = mem;
    pObject->stream = stream;

    if (InterlockedCompareExchangePointer((void**)&This->pStream, pObject, NULL))
    {
        /* Some other thread set the stream first. */
        IStream_Release(&pObject->IStream_iface);
        return WINCODEC_ERR_WRONGSTATE;
    }
    return S_OK;

error:
    if (stream) IWICStream_Release(stream);
    UnmapViewOfFile(mem);
    CloseHandle(map);
    return hr;
}

static HRESULT WINAPI IWICStreamImpl_InitializeFromIStreamRegion(IWICStream *iface,
    IStream *pIStream, ULARGE_INTEGER ulOffset, ULARGE_INTEGER ulMaxSize)
{
    IWICStreamImpl *This = impl_from_IWICStream(iface);
    StreamOnStreamRange *pObject;

    TRACE("(%p,%p,%s,%s)\n", iface, pIStream, wine_dbgstr_longlong(ulOffset.QuadPart),
        wine_dbgstr_longlong(ulMaxSize.QuadPart));

    if (!pIStream) return E_INVALIDARG;
    if (This->pStream) return WINCODEC_ERR_WRONGSTATE;

    pObject = malloc(sizeof(StreamOnStreamRange));
    if (!pObject) return E_OUTOFMEMORY;

    pObject->IStream_iface.lpVtbl = &StreamOnStreamRange_Vtbl;
    pObject->ref = 1;
    IStream_AddRef(pIStream);
    pObject->stream = pIStream;
    pObject->pos.QuadPart = 0;
    pObject->offset = ulOffset;
    pObject->max_size = ulMaxSize;
#ifdef __REACTOS__
    InitializeCriticalSection(&pObject->lock);
#else
    InitializeCriticalSectionEx(&pObject->lock, 0, RTL_CRITICAL_SECTION_FLAG_FORCE_DEBUG_INFO);
#endif
    pObject->lock.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": StreamOnStreamRange.lock");

    if (InterlockedCompareExchangePointer((void**)&This->pStream, pObject, NULL))
    {
        /* Some other thread set the stream first. */
        IStream_Release(&pObject->IStream_iface);
        return WINCODEC_ERR_WRONGSTATE;
    }

    return S_OK;
}


static const IWICStreamVtbl WICStream_Vtbl =
{
    /*** IUnknown methods ***/
    IWICStreamImpl_QueryInterface,
    IWICStreamImpl_AddRef,
    IWICStreamImpl_Release,
    /*** ISequentialStream methods ***/
    IWICStreamImpl_Read,
    IWICStreamImpl_Write,
    /*** IStream methods ***/
    IWICStreamImpl_Seek,
    IWICStreamImpl_SetSize,
    IWICStreamImpl_CopyTo,
    IWICStreamImpl_Commit,
    IWICStreamImpl_Revert,
    IWICStreamImpl_LockRegion,
    IWICStreamImpl_UnlockRegion,
    IWICStreamImpl_Stat,
    IWICStreamImpl_Clone,
    /*** IWICStream methods ***/
    IWICStreamImpl_InitializeFromIStream,
    IWICStreamImpl_InitializeFromFilename,
    IWICStreamImpl_InitializeFromMemory,
    IWICStreamImpl_InitializeFromIStreamRegion,
};

HRESULT StreamImpl_Create(IWICStream **stream)
{
    IWICStreamImpl *pObject;

    if( !stream ) return E_INVALIDARG;

    pObject = malloc(sizeof(IWICStreamImpl));
    if( !pObject ) {
        *stream = NULL;
        return E_OUTOFMEMORY;
    }

    pObject->IWICStream_iface.lpVtbl = &WICStream_Vtbl;
    pObject->ref = 1;
    pObject->pStream = NULL;

    *stream = &pObject->IWICStream_iface;

    return S_OK;
}
