/*
 * Copyright 2002 Jon Griffiths
 * Copyright 2016 Sebastian Lackner
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

#include <stdarg.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "initguid.h"
#include "ocidl.h"
#include "featurestagingapi.h"
#include "shellscalingapi.h"
#define WINSHLWAPI
#include "shlwapi.h"

#include "wine/debug.h"
#include "wine/heap.h"

WINE_DEFAULT_DEBUG_CHANNEL(shcore);

static DWORD shcore_tls;
static IUnknown *process_ref;

BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, void *reserved)
{
    TRACE("%p, %lu, %p.\n", instance, reason, reserved);

    switch (reason)
    {
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(instance);
            shcore_tls = TlsAlloc();
            break;
        case DLL_PROCESS_DETACH:
            if (reserved) break;
            if (shcore_tls != TLS_OUT_OF_INDEXES)
                TlsFree(shcore_tls);
            break;
    }

    return TRUE;
}

HRESULT WINAPI GetProcessDpiAwareness(HANDLE process, PROCESS_DPI_AWARENESS *value)
{
#ifdef __REACTOS__
    UNIMPLEMENTED;
    return E_FAIL;
#else
    if (GetProcessDpiAwarenessInternal( process, (DPI_AWARENESS *)value )) return S_OK;
    return HRESULT_FROM_WIN32( GetLastError() );
#endif
}

HRESULT WINAPI SetProcessDpiAwareness(PROCESS_DPI_AWARENESS value)
{
#ifdef __REACTOS__
    UNIMPLEMENTED;
    return E_FAIL;
#else
    if (SetProcessDpiAwarenessInternal( value )) return S_OK;
    return HRESULT_FROM_WIN32( GetLastError() );
#endif
}

HRESULT WINAPI GetDpiForMonitor(HMONITOR monitor, MONITOR_DPI_TYPE type, UINT *x, UINT *y)
{
#ifdef __REACTOS__
    UNIMPLEMENTED;
    return E_FAIL;
#else
    if (GetDpiForMonitorInternal( monitor, type, x, y )) return S_OK;
    return HRESULT_FROM_WIN32( GetLastError() );
#endif
}

HRESULT WINAPI GetScaleFactorForMonitor(HMONITOR monitor, DEVICE_SCALE_FACTOR *scale)
{
    FIXME("(%p %p): stub\n", monitor, scale);

    *scale = SCALE_100_PERCENT;
    return S_OK;
}

DEVICE_SCALE_FACTOR WINAPI GetScaleFactorForDevice(DISPLAY_DEVICE_TYPE device_type)
{
    FIXME("%d\n", device_type);

    return SCALE_100_PERCENT;
}

HRESULT WINAPI _IStream_Read(IStream *stream, void *dest, ULONG size)
{
    ULONG read;
    HRESULT hr;

    TRACE("%p, %p, %lu.\n", stream, dest, size);

    hr = IStream_Read(stream, dest, size, &read);
    if (SUCCEEDED(hr) && read != size)
        hr = E_FAIL;
    return hr;
}

HRESULT WINAPI IStream_Reset(IStream *stream)
{
    static const LARGE_INTEGER zero;

    TRACE("(%p)\n", stream);

    return IStream_Seek(stream, zero, 0, NULL);
}

HRESULT WINAPI IStream_Size(IStream *stream, ULARGE_INTEGER *size)
{
    STATSTG statstg;
    HRESULT hr;

    TRACE("(%p, %p)\n", stream, size);

    memset(&statstg, 0, sizeof(statstg));

    hr = IStream_Stat(stream, &statstg, STATFLAG_NONAME);

    if (SUCCEEDED(hr) && size)
        *size = statstg.cbSize;
    return hr;
}

HRESULT WINAPI _IStream_Write(IStream *stream, const void *src, ULONG size)
{
    ULONG written;
    HRESULT hr;

    TRACE("%p, %p, %lu.\n", stream, src, size);

    hr = IStream_Write(stream, src, size, &written);
    if (SUCCEEDED(hr) && written != size)
        hr = E_FAIL;

    return hr;
}

void WINAPI IUnknown_AtomicRelease(IUnknown **obj)
{
    TRACE("(%p)\n", obj);

    if (!obj || !*obj)
        return;

    IUnknown_Release(*obj);
    *obj = NULL;
}

HRESULT WINAPI IUnknown_GetSite(IUnknown *unk, REFIID iid, void **site)
{
    IObjectWithSite *obj = NULL;
    HRESULT hr = E_INVALIDARG;

    TRACE("(%p, %s, %p)\n", unk, debugstr_guid(iid), site);

    if (unk && iid && site)
    {
        hr = IUnknown_QueryInterface(unk, &IID_IObjectWithSite, (void **)&obj);
        if (SUCCEEDED(hr) && obj)
        {
            hr = IObjectWithSite_GetSite(obj, iid, site);
            IObjectWithSite_Release(obj);
        }
    }

    return hr;
}

HRESULT WINAPI IUnknown_QueryService(IUnknown *obj, REFGUID sid, REFIID iid, void **out)
{
    IServiceProvider *provider = NULL;
    HRESULT hr;

    if (!out)
        return E_FAIL;

    *out = NULL;

    if (!obj)
        return E_FAIL;

    hr = IUnknown_QueryInterface(obj, &IID_IServiceProvider, (void **)&provider);
    if (hr == S_OK && provider)
    {
        TRACE("Using provider %p.\n", provider);

        hr = IServiceProvider_QueryService(provider, sid, iid, out);

        TRACE("Provider %p returned %p.\n", provider, *out);

        IServiceProvider_Release(provider);
    }

    return hr;
}

void WINAPI IUnknown_Set(IUnknown **dest, IUnknown *src)
{
    TRACE("(%p, %p)\n", dest, src);

    IUnknown_AtomicRelease(dest);

    if (src)
    {
        IUnknown_AddRef(src);
        *dest = src;
    }
}

HRESULT WINAPI IUnknown_SetSite(IUnknown *obj, IUnknown *site)
{
    IInternetSecurityManager *sec_manager;
    IObjectWithSite *objwithsite;
    HRESULT hr;

    if (!obj)
        return E_FAIL;

    hr = IUnknown_QueryInterface(obj, &IID_IObjectWithSite, (void **)&objwithsite);
    TRACE("ObjectWithSite %p, hr %#lx.\n", objwithsite, hr);
    if (SUCCEEDED(hr))
    {
        hr = IObjectWithSite_SetSite(objwithsite, site);
        TRACE("SetSite() hr %#lx.\n", hr);
        IObjectWithSite_Release(objwithsite);
    }
    else
    {
        hr = IUnknown_QueryInterface(obj, &IID_IInternetSecurityManager, (void **)&sec_manager);
        TRACE("InternetSecurityManager %p, hr %#lx.\n", sec_manager, hr);
        if (FAILED(hr))
            return hr;

        hr = IInternetSecurityManager_SetSecuritySite(sec_manager, (IInternetSecurityMgrSite *)site);
        TRACE("SetSecuritySite() hr %#lx.\n", hr);
        IInternetSecurityManager_Release(sec_manager);
    }

    return hr;
}

HRESULT WINAPI SetCurrentProcessExplicitAppUserModelID(const WCHAR *appid)
{
    FIXME("%s: stub\n", debugstr_w(appid));
    return S_OK;
}

HRESULT WINAPI GetCurrentProcessExplicitAppUserModelID(const WCHAR **appid)
{
    FIXME("%p: stub\n", appid);
    *appid = NULL;
    return E_NOTIMPL;
}

/*************************************************************************
 * CommandLineToArgvW            [SHCORE.@]
 *
 * We must interpret the quotes in the command line to rebuild the argv
 * array correctly:
 * - arguments are separated by spaces or tabs
 * - quotes serve as optional argument delimiters
 *   '"a b"'   -> 'a b'
 * - escaped quotes must be converted back to '"'
 *   '\"'      -> '"'
 * - consecutive backslashes preceding a quote see their number halved with
 *   the remainder escaping the quote:
 *   2n   backslashes + quote -> n backslashes + quote as an argument delimiter
 *   2n+1 backslashes + quote -> n backslashes + literal quote
 * - backslashes that are not followed by a quote are copied literally:
 *   'a\b'     -> 'a\b'
 *   'a\\b'    -> 'a\\b'
 * - in quoted strings, consecutive quotes see their number divided by three
 *   with the remainder modulo 3 deciding whether to close the string or not.
 *   Note that the opening quote must be counted in the consecutive quotes,
 *   that's the (1+) below:
 *   (1+) 3n   quotes -> n quotes
 *   (1+) 3n+1 quotes -> n quotes plus closes the quoted string
 *   (1+) 3n+2 quotes -> n+1 quotes plus closes the quoted string
 * - in unquoted strings, the first quote opens the quoted string and the
 *   remaining consecutive quotes follow the above rule.
 */
WCHAR** WINAPI CommandLineToArgvW(const WCHAR *cmdline, int *numargs)
{
    int qcount, bcount;
    const WCHAR *s;
    WCHAR **argv;
    DWORD argc;
    WCHAR *d;

    if (!numargs)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return NULL;
    }

    if (*cmdline == 0)
    {
        /* Return the path to the executable */
        DWORD len, deslen = MAX_PATH, size;

        size = sizeof(WCHAR *) * 2 + deslen * sizeof(WCHAR);
        for (;;)
        {
            if (!(argv = LocalAlloc(LMEM_FIXED, size))) return NULL;
            len = GetModuleFileNameW(0, (WCHAR *)(argv + 2), deslen);
            if (!len)
            {
                LocalFree(argv);
                return NULL;
            }
            if (len < deslen) break;
            deslen *= 2;
            size = sizeof(WCHAR *) * 2 + deslen * sizeof(WCHAR);
            LocalFree(argv);
        }
        argv[0] = (WCHAR *)(argv + 2);
        argv[1] = NULL;
        *numargs = 1;

        return argv;
    }

    /* --- First count the arguments */
    argc = 1;
    s = cmdline;
    /* The first argument, the executable path, follows special rules */
    if (*s == '"')
    {
        /* The executable path ends at the next quote, no matter what */
        s++;
        while (*s)
            if (*s++ == '"')
                break;
    }
    else
    {
        /* The executable path ends at the next space, no matter what */
        while (*s && *s != ' ' && *s != '\t')
            s++;
    }
    /* skip to the first argument, if any */
    while (*s == ' ' || *s == '\t')
        s++;
    if (*s)
        argc++;

    /* Analyze the remaining arguments */
    qcount = bcount = 0;
    while (*s)
    {
        if ((*s == ' ' || *s == '\t') && qcount == 0)
        {
            /* skip to the next argument and count it if any */
            while (*s == ' ' || *s == '\t')
                s++;
            if (*s)
                argc++;
            bcount = 0;
        }
        else if (*s == '\\')
        {
            /* '\', count them */
            bcount++;
            s++;
        }
        else if (*s == '"')
        {
            /* '"' */
            if ((bcount & 1) == 0)
                qcount++; /* unescaped '"' */
            s++;
            bcount = 0;
            /* consecutive quotes, see comment in copying code below */
            while (*s == '"')
            {
                qcount++;
                s++;
            }
            qcount = qcount % 3;
            if (qcount == 2)
                qcount = 0;
        }
        else
        {
            /* a regular character */
            bcount = 0;
            s++;
        }
    }

    /* Allocate in a single lump, the string array, and the strings that go
     * with it. This way the caller can make a single LocalFree() call to free
     * both, as per MSDN.
     */
    argv = LocalAlloc(LMEM_FIXED, (argc + 1) * sizeof(WCHAR *) + (lstrlenW(cmdline) + 1) * sizeof(WCHAR));
    if (!argv)
        return NULL;

    /* --- Then split and copy the arguments */
    argv[0] = d = lstrcpyW((WCHAR *)(argv + argc + 1), cmdline);
    argc = 1;
    /* The first argument, the executable path, follows special rules */
    if (*d == '"')
    {
        /* The executable path ends at the next quote, no matter what */
        s = d + 1;
        while (*s)
        {
            if (*s == '"')
            {
                s++;
                break;
            }
            *d++ = *s++;
        }
    }
    else
    {
        /* The executable path ends at the next space, no matter what */
        while (*d && *d != ' ' && *d != '\t')
            d++;
        s = d;
        if (*s)
            s++;
    }
    /* close the executable path */
    *d++ = 0;
    /* skip to the first argument and initialize it if any */
    while (*s == ' ' || *s == '\t')
        s++;
    if (!*s)
    {
        /* There are no parameters so we are all done */
        argv[argc] = NULL;
        *numargs = argc;
        return argv;
    }

    /* Split and copy the remaining arguments */
    argv[argc++] = d;
    qcount = bcount = 0;
    while (*s)
    {
        if ((*s == ' ' || *s == '\t') && qcount == 0)
        {
            /* close the argument */
            *d++ = 0;
            bcount = 0;

            /* skip to the next one and initialize it if any */
            do {
                s++;
            } while (*s == ' ' || *s == '\t');
            if (*s)
                argv[argc++] = d;
        }
        else if (*s=='\\')
        {
            *d++ = *s++;
            bcount++;
        }
        else if (*s == '"')
        {
            if ((bcount & 1) == 0)
            {
                /* Preceded by an even number of '\', this is half that
                 * number of '\', plus a quote which we erase.
                 */
                d -= bcount / 2;
                qcount++;
            }
            else
            {
                /* Preceded by an odd number of '\', this is half that
                 * number of '\' followed by a '"'
                 */
                d = d - bcount / 2 - 1;
                *d++ = '"';
            }
            s++;
            bcount = 0;
            /* Now count the number of consecutive quotes. Note that qcount
             * already takes into account the opening quote if any, as well as
             * the quote that lead us here.
             */
            while (*s == '"')
            {
                if (++qcount == 3)
                {
                    *d++ = '"';
                    qcount = 0;
                }
                s++;
            }
            if (qcount == 2)
                qcount = 0;
        }
        else
        {
            /* a regular character */
            *d++ = *s++;
            bcount = 0;
        }
    }
    *d = '\0';
    argv[argc] = NULL;
    *numargs = argc;

    return argv;
}

struct shstream
{
    IStream IStream_iface;
    LONG refcount;

    union
    {
        struct
        {
            BYTE *buffer;
            DWORD length;
            DWORD position;

            HKEY hkey;
            WCHAR *valuename;
        } mem;
        struct
        {
            HANDLE handle;
            DWORD mode;
            WCHAR *path;
        } file;
    } u;
};

static inline struct shstream *impl_from_IStream(IStream *iface)
{
    return CONTAINING_RECORD(iface, struct shstream, IStream_iface);
}

static HRESULT WINAPI shstream_QueryInterface(IStream *iface, REFIID riid, void **out)
{
    struct shstream *stream = impl_from_IStream(iface);

    TRACE("(%p)->(%s, %p)\n", stream, debugstr_guid(riid), out);

    if (IsEqualIID(riid, &IID_IUnknown) ||
        IsEqualIID(riid, &IID_IStream) ||
        IsEqualIID(riid, &IID_ISequentialStream))
    {
        *out = iface;
        IStream_AddRef(iface);
        return S_OK;
    }

    *out = NULL;
    WARN("Unsupported interface %s.\n", debugstr_guid(riid));
    return E_NOINTERFACE;
}

static ULONG WINAPI shstream_AddRef(IStream *iface)
{
    struct shstream *stream = impl_from_IStream(iface);
    ULONG refcount = InterlockedIncrement(&stream->refcount);

    TRACE("%p, refcount %lu.\n", iface, refcount);

    return refcount;
}

static ULONG WINAPI memstream_Release(IStream *iface)
{
    struct shstream *stream = impl_from_IStream(iface);
    ULONG refcount = InterlockedDecrement(&stream->refcount);

    TRACE("%p, refcount %lu.\n", iface, refcount);

    if (!refcount)
    {
        heap_free(stream->u.mem.buffer);
        heap_free(stream);
    }

    return refcount;
}

static HRESULT WINAPI memstream_Read(IStream *iface, void *buff, ULONG buff_size, ULONG *read_len)
{
    struct shstream *stream = impl_from_IStream(iface);
    DWORD length;

    TRACE("%p, %p, %lu, %p.\n", iface, buff, buff_size, read_len);

    if (stream->u.mem.position >= stream->u.mem.length)
    {
        if (read_len)
            *read_len = 0;
        return S_FALSE;
    }

    length = stream->u.mem.length - stream->u.mem.position;
    if (buff_size < length)
        length = buff_size;

    memmove(buff, stream->u.mem.buffer + stream->u.mem.position, length);
    stream->u.mem.position += length;

    if (read_len)
        *read_len = length;

    return S_OK;
}

static HRESULT WINAPI memstream_Write(IStream *iface, const void *buff, ULONG buff_size, ULONG *written)
{
    struct shstream *stream = impl_from_IStream(iface);
    DWORD length = stream->u.mem.position + buff_size;

    TRACE("%p, %p, %lu, %p.\n", iface, buff, buff_size, written);

    if (length < stream->u.mem.position) /* overflow */
        return STG_E_INSUFFICIENTMEMORY;

    if (length > stream->u.mem.length)
    {
        BYTE *buffer = HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, stream->u.mem.buffer, length);
        if (!buffer)
            return STG_E_INSUFFICIENTMEMORY;

        stream->u.mem.length = length;
        stream->u.mem.buffer = buffer;
    }
    memmove(stream->u.mem.buffer + stream->u.mem.position, buff, buff_size);
    stream->u.mem.position += buff_size; /* adjust pointer */

    if (written)
        *written = buff_size;

    return S_OK;
}

static HRESULT WINAPI memstream_Seek(IStream *iface, LARGE_INTEGER move, DWORD origin, ULARGE_INTEGER*new_pos)
{
    struct shstream *stream = impl_from_IStream(iface);
    LARGE_INTEGER tmp;

    TRACE("%p, %s, %ld, %p.\n", iface, wine_dbgstr_longlong(move.QuadPart), origin, new_pos);

    if (origin == STREAM_SEEK_SET)
        tmp = move;
    else if (origin == STREAM_SEEK_CUR)
        tmp.QuadPart = stream->u.mem.position + move.QuadPart;
    else if (origin == STREAM_SEEK_END)
        tmp.QuadPart = stream->u.mem.length + move.QuadPart;
    else
        return STG_E_INVALIDPARAMETER;

    if (tmp.QuadPart < 0)
        return STG_E_INVALIDFUNCTION;

    /* we cut off the high part here */
    stream->u.mem.position = tmp.u.LowPart;

    if (new_pos)
        new_pos->QuadPart = stream->u.mem.position;
    return S_OK;
}

static HRESULT WINAPI memstream_SetSize(IStream *iface, ULARGE_INTEGER new_size)
{
    struct shstream *stream = impl_from_IStream(iface);
    DWORD length;
    BYTE *buffer;

    TRACE("(%p, %s)\n", stream, wine_dbgstr_longlong(new_size.QuadPart));

    /* we cut off the high part here */
    length = new_size.u.LowPart;
    buffer = HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, stream->u.mem.buffer, length);
    if (!buffer)
        return STG_E_INSUFFICIENTMEMORY;

    stream->u.mem.buffer = buffer;
    stream->u.mem.length = length;

    return S_OK;
}

static HRESULT WINAPI shstream_CopyTo(IStream *iface, IStream *dest, ULARGE_INTEGER size,
        ULARGE_INTEGER *read_len, ULARGE_INTEGER *written)
{
    struct shstream *stream = impl_from_IStream(iface);
    ULARGE_INTEGER total_read, total_written;
    HRESULT hr = S_OK;
    BYTE buffer[0x400];

    TRACE("(%p, %p, %s, %p, %p)\n", stream, dest, wine_dbgstr_longlong(size.QuadPart), read_len, written);

    if (!dest)
        return E_POINTER;

    total_read.QuadPart = 0;
    total_written.QuadPart = 0;

    while (size.QuadPart > 0)
    {
        ULONG chunk_size = size.QuadPart >= sizeof(buffer) ? sizeof(buffer) : size.u.LowPart;
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
            size.QuadPart = 0;
        else
            size.QuadPart -= chunk_read;
    }

    if (read_len)
        read_len->QuadPart = total_read.QuadPart;
    if (written)
        written->QuadPart = total_written.QuadPart;

    return hr;
}

static HRESULT WINAPI shstream_Commit(IStream *iface, DWORD flags)
{
    TRACE("%p, %#lx.\n", iface, flags);

    /* Commit is not supported by this stream */
    return E_NOTIMPL;
}

static HRESULT WINAPI shstream_Revert(IStream *iface)
{
    struct shstream *stream = impl_from_IStream(iface);

    TRACE("(%p)\n", stream);

    /* revert not supported by this stream */
    return E_NOTIMPL;
}

static HRESULT WINAPI shstream_LockRegion(IStream *iface, ULARGE_INTEGER offset, ULARGE_INTEGER size, DWORD lock_type)
{
    struct shstream *stream = impl_from_IStream(iface);

    TRACE("(%p)\n", stream);

    /* lock/unlock not supported by this stream */
    return E_NOTIMPL;
}

static HRESULT WINAPI shstream_UnlockRegion(IStream *iface, ULARGE_INTEGER offset, ULARGE_INTEGER size, DWORD lock_type)
{
    struct shstream *stream = impl_from_IStream(iface);

    TRACE("(%p)\n", stream);

    /* lock/unlock not supported by this stream */
    return E_NOTIMPL;
}

static HRESULT WINAPI memstream_Stat(IStream *iface, STATSTG *statstg, DWORD flags)
{
    struct shstream *stream = impl_from_IStream(iface);

    TRACE("%p, %p, %#lx.\n", iface, statstg, flags);

    memset(statstg, 0, sizeof(*statstg));
    statstg->type = STGTY_STREAM;
    statstg->cbSize.QuadPart = stream->u.mem.length;
    statstg->grfMode = STGM_READWRITE;

    return S_OK;
}

static HRESULT WINAPI shstream_Clone(IStream *iface, IStream **dest)
{
    struct shstream *stream = impl_from_IStream(iface);

    TRACE("(%p, %p)\n", stream, dest);

    *dest = NULL;

    /* clone not supported by this stream */
    return E_NOTIMPL;
}

static const IStreamVtbl memstreamvtbl =
{
    shstream_QueryInterface,
    shstream_AddRef,
    memstream_Release,
    memstream_Read,
    memstream_Write,
    memstream_Seek,
    memstream_SetSize,
    shstream_CopyTo,
    shstream_Commit,
    shstream_Revert,
    shstream_LockRegion,
    shstream_UnlockRegion,
    memstream_Stat,
    shstream_Clone,
};

static struct shstream *shstream_create(const IStreamVtbl *vtbl, const BYTE *data, UINT data_len)
{
    struct shstream *stream;

    if (!data)
        data_len = 0;

    stream = heap_alloc(sizeof(*stream));
    stream->IStream_iface.lpVtbl = vtbl;
    stream->refcount = 1;
    stream->u.mem.buffer = heap_alloc(data_len);
    if (!stream->u.mem.buffer)
    {
        heap_free(stream);
        return NULL;
    }
    memcpy(stream->u.mem.buffer, data, data_len);
    stream->u.mem.length = data_len;
    stream->u.mem.position = 0;

    return stream;
}

/*************************************************************************
 * SHCreateMemStream   [SHCORE.@]
 *
 * Create an IStream object on a block of memory.
 *
 * PARAMS
 * data     [I] Memory block to create the IStream object on
 * data_len [I] Length of data block
 *
 * RETURNS
 * Success: A pointer to the IStream object.
 * Failure: NULL, if any parameters are invalid or an error occurs.
 *
 * NOTES
 *  A copy of the memory block is made, it's freed when the stream is released.
 */
IStream * WINAPI SHCreateMemStream(const BYTE *data, UINT data_len)
{
    struct shstream *stream;

    TRACE("(%p, %u)\n", data, data_len);

    stream = shstream_create(&memstreamvtbl, data, data_len);
    return stream ? &stream->IStream_iface : NULL;
}

static ULONG WINAPI filestream_Release(IStream *iface)
{
    struct shstream *stream = impl_from_IStream(iface);
    ULONG refcount = InterlockedDecrement(&stream->refcount);

    TRACE("%p, refcount %lu.\n", iface, refcount);

    if (!refcount)
    {
        CloseHandle(stream->u.file.handle);
        heap_free(stream->u.file.path);
        heap_free(stream);
    }

    return refcount;
}

static HRESULT WINAPI filestream_Read(IStream *iface, void *buff, ULONG size, ULONG *read_len)
{
    struct shstream *stream = impl_from_IStream(iface);
    DWORD read = 0;

    TRACE("%p, %p, %lu, %p.\n", iface, buff, size, read_len);

    if (!ReadFile(stream->u.file.handle, buff, size, &read, NULL))
    {
        WARN("error %ld reading file\n", GetLastError());
        return S_FALSE;
    }

    if (read_len)
        *read_len = read;

    return read == size ? S_OK : S_FALSE;
}

static HRESULT WINAPI filestream_Write(IStream *iface, const void *buff, ULONG size, ULONG *written)
{
    struct shstream *stream = impl_from_IStream(iface);
    DWORD written_len = 0;

    TRACE("%p, %p, %lu, %p.\n", iface, buff, size, written);

    switch (stream->u.file.mode & 0xf)
    {
        case STGM_WRITE:
        case STGM_READWRITE:
            break;
        default:
            return STG_E_ACCESSDENIED;
    }

    if (!WriteFile(stream->u.file.handle, buff, size, &written_len, NULL))
        return HRESULT_FROM_WIN32(GetLastError());

    if (written)
        *written = written_len;

    return S_OK;
}

static HRESULT WINAPI filestream_Seek(IStream *iface, LARGE_INTEGER move, DWORD origin, ULARGE_INTEGER *new_pos)
{
    struct shstream *stream = impl_from_IStream(iface);
    DWORD position;

    TRACE("%p, %s, %ld, %p.\n", iface, wine_dbgstr_longlong(move.QuadPart), origin, new_pos);

    position = SetFilePointer(stream->u.file.handle, move.u.LowPart, NULL, origin);
    if (position == INVALID_SET_FILE_POINTER)
        return HRESULT_FROM_WIN32(GetLastError());

    if (new_pos)
    {
        new_pos->u.HighPart = 0;
        new_pos->u.LowPart = position;
    }

    return S_OK;
}

static HRESULT WINAPI filestream_SetSize(IStream *iface, ULARGE_INTEGER size)
{
    struct shstream *stream = impl_from_IStream(iface);
    LARGE_INTEGER origin, move;

    TRACE("(%p, %s)\n", stream, wine_dbgstr_longlong(size.QuadPart));

    move.QuadPart = 0;
    if (!SetFilePointerEx(stream->u.file.handle, move, &origin, FILE_CURRENT))
        return E_FAIL;

    move.QuadPart = size.QuadPart;
    if (!SetFilePointerEx(stream->u.file.handle, move, NULL, FILE_BEGIN))
        return E_FAIL;

    if (stream->u.file.mode != STGM_READ)
    {
        if (!SetEndOfFile(stream->u.file.handle))
            return E_FAIL;
        if (!SetFilePointerEx(stream->u.file.handle, origin, NULL, FILE_BEGIN))
            return E_FAIL;
    }

    return S_OK;
}

static HRESULT WINAPI filestream_CopyTo(IStream *iface, IStream *dest, ULARGE_INTEGER size,
        ULARGE_INTEGER *read_len, ULARGE_INTEGER *written)
{
    struct shstream *stream = impl_from_IStream(iface);
    HRESULT hr = S_OK;
    char buff[1024];

    TRACE("(%p, %p, %s, %p, %p)\n", stream, dest, wine_dbgstr_longlong(size.QuadPart), read_len, written);

    if (read_len)
        read_len->QuadPart = 0;
    if (written)
        written->QuadPart = 0;

    if (!dest)
        return S_OK;

    while (size.QuadPart)
    {
        ULONG left, read_chunk, written_chunk;

        left = size.QuadPart > sizeof(buff) ? sizeof(buff) : size.QuadPart;

        /* Read */
        hr = IStream_Read(iface, buff, left, &read_chunk);
        if (FAILED(hr) || read_chunk == 0)
            break;
        if (read_len)
            read_len->QuadPart += read_chunk;

        /* Write */
        hr = IStream_Write(dest, buff, read_chunk, &written_chunk);
        if (written_chunk)
            written->QuadPart += written_chunk;
        if (FAILED(hr) || written_chunk != left)
            break;

        size.QuadPart -= left;
    }

    return hr;
}

static HRESULT WINAPI filestream_Commit(IStream *iface, DWORD flags)
{
    TRACE("%p, %#lx.\n", iface, flags);

    return S_OK;
}

static HRESULT WINAPI filestream_Stat(IStream *iface, STATSTG *statstg, DWORD flags)
{
    struct shstream *stream = impl_from_IStream(iface);
    BY_HANDLE_FILE_INFORMATION fi;

    TRACE("%p, %p, %#lx.\n", iface, statstg, flags);

    if (!statstg)
        return STG_E_INVALIDPOINTER;

    memset(&fi, 0, sizeof(fi));
    GetFileInformationByHandle(stream->u.file.handle, &fi);

    if (flags & STATFLAG_NONAME)
        statstg->pwcsName = NULL;
    else
    {
        int len = lstrlenW(stream->u.file.path);
        if ((statstg->pwcsName = CoTaskMemAlloc((len + 1) * sizeof(WCHAR))))
            memcpy(statstg->pwcsName, stream->u.file.path, (len + 1) * sizeof(WCHAR));
    }
    statstg->type = 0;
    statstg->cbSize.u.LowPart = fi.nFileSizeLow;
    statstg->cbSize.u.HighPart = fi.nFileSizeHigh;
    statstg->mtime = fi.ftLastWriteTime;
    statstg->ctime = fi.ftCreationTime;
    statstg->atime = fi.ftLastAccessTime;
    statstg->grfMode = stream->u.file.mode;
    statstg->grfLocksSupported = 0;
    memcpy(&statstg->clsid, &IID_IStream, sizeof(CLSID));
    statstg->grfStateBits = 0;
    statstg->reserved = 0;

    return S_OK;
}

static const IStreamVtbl filestreamvtbl =
{
    shstream_QueryInterface,
    shstream_AddRef,
    filestream_Release,
    filestream_Read,
    filestream_Write,
    filestream_Seek,
    filestream_SetSize,
    filestream_CopyTo,
    filestream_Commit,
    shstream_Revert,
    shstream_LockRegion,
    shstream_UnlockRegion,
    filestream_Stat,
    shstream_Clone,
};

/*************************************************************************
 * SHCreateStreamOnFileEx   [SHCORE.@]
 */
HRESULT WINAPI SHCreateStreamOnFileEx(const WCHAR *path, DWORD mode, DWORD attributes,
    BOOL create, IStream *template, IStream **ret)
{
    DWORD access, share, creation_disposition, len;
    struct shstream *stream;
    HANDLE hFile;

    TRACE("%s, %ld, %#lx, %d, %p, %p)\n", debugstr_w(path), mode, attributes,
        create, template, ret);

    if (!path || !ret || template)
        return E_INVALIDARG;

    *ret = NULL;

    /* Access */
    switch (mode & 0xf)
    {
        case STGM_WRITE:
        case STGM_READWRITE:
            access = GENERIC_READ | GENERIC_WRITE;
            break;
        case STGM_READ:
            access = GENERIC_READ;
            break;
        default:
            return E_INVALIDARG;
    }

    /* Sharing */
    switch (mode & 0xf0)
    {
        case 0:
        case STGM_SHARE_DENY_NONE:
            share = FILE_SHARE_READ | FILE_SHARE_WRITE;
            break;
        case STGM_SHARE_DENY_READ:
            share = FILE_SHARE_WRITE;
            break;
        case STGM_SHARE_DENY_WRITE:
            share = FILE_SHARE_READ;
            break;
        case STGM_SHARE_EXCLUSIVE:
            share = 0;
            break;
        default:
            return E_INVALIDARG;
    }

    switch (mode & 0xf000)
    {
        case STGM_FAILIFTHERE:
            creation_disposition = create ? CREATE_NEW : OPEN_EXISTING;
            break;
        case STGM_CREATE:
            creation_disposition = CREATE_ALWAYS;
            break;
        default:
            return E_INVALIDARG;
    }

    hFile = CreateFileW(path, access, share, NULL, creation_disposition, attributes, 0);
    if (hFile == INVALID_HANDLE_VALUE)
        return HRESULT_FROM_WIN32(GetLastError());

    stream = heap_alloc(sizeof(*stream));
    stream->IStream_iface.lpVtbl = &filestreamvtbl;
    stream->refcount = 1;
    stream->u.file.handle = hFile;
    stream->u.file.mode = mode;

    len = lstrlenW(path);
    stream->u.file.path = heap_alloc((len + 1) * sizeof(WCHAR));
    memcpy(stream->u.file.path, path, (len + 1) * sizeof(WCHAR));

    *ret = &stream->IStream_iface;

    return S_OK;
}

/*************************************************************************
 * SHCreateStreamOnFileW   [SHCORE.@]
 */
HRESULT WINAPI SHCreateStreamOnFileW(const WCHAR *path, DWORD mode, IStream **stream)
{
    TRACE("%s, %#lx, %p.\n", debugstr_w(path), mode, stream);

    if (!path || !stream)
        return E_INVALIDARG;

    if ((mode & (STGM_CONVERT | STGM_DELETEONRELEASE | STGM_TRANSACTED)) != 0)
        return E_INVALIDARG;

    return SHCreateStreamOnFileEx(path, mode, 0, FALSE, NULL, stream);
}

/*************************************************************************
 * SHCreateStreamOnFileA   [SHCORE.@]
 */
HRESULT WINAPI SHCreateStreamOnFileA(const char *path, DWORD mode, IStream **stream)
{
    WCHAR *pathW;
    HRESULT hr;
    DWORD len;

    TRACE("%s, %#lx, %p.\n", debugstr_a(path), mode, stream);

    if (!path)
        return HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND);

    len = MultiByteToWideChar(CP_ACP, 0, path, -1, NULL, 0);
    pathW = heap_alloc(len * sizeof(WCHAR));
    if (!pathW)
        return E_OUTOFMEMORY;

    MultiByteToWideChar(CP_ACP, 0, path, -1, pathW, len);
    hr = SHCreateStreamOnFileW(pathW, mode, stream);
    heap_free(pathW);

    return hr;
}

static ULONG WINAPI regstream_Release(IStream *iface)
{
    struct shstream *stream = impl_from_IStream(iface);
    ULONG refcount = InterlockedDecrement(&stream->refcount);

    TRACE("%p, refcount %lu.\n", iface, refcount);

    if (!refcount)
    {
        if (stream->u.mem.hkey)
        {
            if (stream->u.mem.length)
                RegSetValueExW(stream->u.mem.hkey, stream->u.mem.valuename, 0, REG_BINARY,
                        (const BYTE *)stream->u.mem.buffer, stream->u.mem.length);
            else
                RegDeleteValueW(stream->u.mem.hkey, stream->u.mem.valuename);
            RegCloseKey(stream->u.mem.hkey);
        }
        CoTaskMemFree(stream->u.mem.valuename);
        heap_free(stream->u.mem.buffer);
        heap_free(stream);
    }

    return refcount;
}

static const IStreamVtbl regstreamvtbl =
{
    shstream_QueryInterface,
    shstream_AddRef,
    regstream_Release,
    memstream_Read,
    memstream_Write,
    memstream_Seek,
    memstream_SetSize,
    shstream_CopyTo,
    shstream_Commit,
    shstream_Revert,
    shstream_LockRegion,
    shstream_UnlockRegion,
    memstream_Stat,
    shstream_Clone,
};

/*************************************************************************
 * SHOpenRegStream2W        [SHCORE.@]
 */
IStream * WINAPI SHOpenRegStream2W(HKEY hKey, const WCHAR *subkey, const WCHAR *value, DWORD mode)
{
    struct shstream *stream;
    HKEY hStrKey = NULL;
    BYTE *buff = NULL;
    DWORD length = 0;
    LONG ret;

    TRACE("%p, %s, %s, %#lx.\n", hKey, debugstr_w(subkey), debugstr_w(value), mode);

    if (mode == STGM_READ)
        ret = RegOpenKeyExW(hKey, subkey, 0, KEY_READ, &hStrKey);
    else /* in write mode we make sure the subkey exits */
        ret = RegCreateKeyExW(hKey, subkey, 0, NULL, 0, KEY_READ | KEY_WRITE, NULL, &hStrKey, NULL);

    if (ret == ERROR_SUCCESS)
    {
        if (mode == STGM_READ || mode == STGM_READWRITE)
        {
            /* read initial data */
            ret = RegQueryValueExW(hStrKey, value, 0, 0, 0, &length);
            if (ret == ERROR_SUCCESS && length)
            {
                buff = heap_alloc(length);
                RegQueryValueExW(hStrKey, value, 0, 0, buff, &length);
            }
        }

        if (!length)
            buff = heap_alloc(length);

        stream = shstream_create(&regstreamvtbl, buff, length);
        heap_free(buff);
        if (stream)
        {
            stream->u.mem.hkey = hStrKey;
            SHStrDupW(value, &stream->u.mem.valuename);
            return &stream->IStream_iface;
        }
    }

    if (hStrKey)
        RegCloseKey(hStrKey);

    return NULL;
}

/*************************************************************************
 * SHOpenRegStream2A        [SHCORE.@]
 */
IStream * WINAPI SHOpenRegStream2A(HKEY hKey, const char *subkey, const char *value, DWORD mode)
{
    WCHAR *subkeyW = NULL, *valueW = NULL;
    IStream *stream;

    TRACE("%p, %s, %s, %#lx.\n", hKey, debugstr_a(subkey), debugstr_a(value), mode);

    if (subkey && FAILED(SHStrDupA(subkey, &subkeyW)))
        return NULL;
    if (value && FAILED(SHStrDupA(value, &valueW)))
    {
        CoTaskMemFree(subkeyW);
        return NULL;
    }

    stream = SHOpenRegStream2W(hKey, subkeyW, valueW, mode);
    CoTaskMemFree(subkeyW);
    CoTaskMemFree(valueW);
    return stream;
}

/*************************************************************************
 * SHOpenRegStreamA        [SHCORE.@]
 */
IStream * WINAPI SHOpenRegStreamA(HKEY hkey, const char *subkey, const char *value, DWORD mode)
{
    WCHAR *subkeyW = NULL, *valueW = NULL;
    IStream *stream;

    TRACE("%p, %s, %s, %#lx.\n", hkey, debugstr_a(subkey), debugstr_a(value), mode);

    if (subkey && FAILED(SHStrDupA(subkey, &subkeyW)))
        return NULL;
    if (value && FAILED(SHStrDupA(value, &valueW)))
    {
        CoTaskMemFree(subkeyW);
        return NULL;
    }

    stream = SHOpenRegStreamW(hkey, subkeyW, valueW, mode);
    CoTaskMemFree(subkeyW);
    CoTaskMemFree(valueW);
    return stream;
}

static ULONG WINAPI dummystream_AddRef(IStream *iface)
{
    TRACE("()\n");
    return 2;
}

static ULONG WINAPI dummystream_Release(IStream *iface)
{
    TRACE("()\n");
    return 1;
}

static HRESULT WINAPI dummystream_Read(IStream *iface, void *buff, ULONG buff_size, ULONG *read_len)
{
    if (read_len)
        *read_len = 0;

    return E_NOTIMPL;
}

static const IStreamVtbl dummystreamvtbl =
{
    shstream_QueryInterface,
    dummystream_AddRef,
    dummystream_Release,
    dummystream_Read,
    memstream_Write,
    memstream_Seek,
    memstream_SetSize,
    shstream_CopyTo,
    shstream_Commit,
    shstream_Revert,
    shstream_LockRegion,
    shstream_UnlockRegion,
    memstream_Stat,
    shstream_Clone,
};

static struct shstream dummyregstream = { { &dummystreamvtbl } };

/*************************************************************************
 * SHOpenRegStreamW        [SHCORE.@]
 */
IStream * WINAPI SHOpenRegStreamW(HKEY hkey, const WCHAR *subkey, const WCHAR *value, DWORD mode)
{
    IStream *stream;

    TRACE("%p, %s, %s, %#lx.\n", hkey, debugstr_w(subkey), debugstr_w(value), mode);
    stream = SHOpenRegStream2W(hkey, subkey, value, mode);
    return stream ? stream : &dummyregstream.IStream_iface;
}

struct threadref
{
    IUnknown IUnknown_iface;
    LONG *refcount;
};

static inline struct threadref *threadref_impl_from_IUnknown(IUnknown *iface)
{
    return CONTAINING_RECORD(iface, struct threadref, IUnknown_iface);
}

static HRESULT WINAPI threadref_QueryInterface(IUnknown *iface, REFIID riid, void **out)
{
    struct threadref *threadref = threadref_impl_from_IUnknown(iface);

    TRACE("(%p, %s, %p)\n", threadref, debugstr_guid(riid), out);

    if (out == NULL)
        return E_POINTER;

    if (IsEqualGUID(&IID_IUnknown, riid))
    {
        *out = iface;
        IUnknown_AddRef(iface);
        return S_OK;
    }

    *out = NULL;
    WARN("Interface %s not supported.\n", debugstr_guid(riid));
    return E_NOINTERFACE;
}

static ULONG WINAPI threadref_AddRef(IUnknown *iface)
{
    struct threadref *threadref = threadref_impl_from_IUnknown(iface);
    LONG refcount = InterlockedIncrement(threadref->refcount);

    TRACE("%p, refcount %ld.\n", threadref, refcount);

    return refcount;
}

static ULONG WINAPI threadref_Release(IUnknown *iface)
{
    struct threadref *threadref = threadref_impl_from_IUnknown(iface);
    LONG refcount = InterlockedDecrement(threadref->refcount);

    TRACE("%p, refcount %ld.\n", threadref, refcount);

    if (!refcount)
        heap_free(threadref);

    return refcount;
}

static const IUnknownVtbl threadrefvtbl =
{
    threadref_QueryInterface,
    threadref_AddRef,
    threadref_Release,
};

/*************************************************************************
 * SHCreateThreadRef        [SHCORE.@]
 */
HRESULT WINAPI SHCreateThreadRef(LONG *refcount, IUnknown **out)
{
    struct threadref *threadref;

    TRACE("(%p, %p)\n", refcount, out);

    if (!refcount || !out)
        return E_INVALIDARG;

    *out = NULL;

    threadref = heap_alloc(sizeof(*threadref));
    if (!threadref)
        return E_OUTOFMEMORY;
    threadref->IUnknown_iface.lpVtbl = &threadrefvtbl;
    threadref->refcount = refcount;

    *refcount = 1;
    *out = &threadref->IUnknown_iface;

    TRACE("Created %p.\n", threadref);
    return S_OK;
}

/*************************************************************************
 * SHGetThreadRef        [SHCORE.@]
 */
HRESULT WINAPI SHGetThreadRef(IUnknown **out)
{
    TRACE("(%p)\n", out);

    if (shcore_tls == TLS_OUT_OF_INDEXES)
        return E_NOINTERFACE;

    *out = TlsGetValue(shcore_tls);
    if (!*out)
        return E_NOINTERFACE;

    IUnknown_AddRef(*out);
    return S_OK;
}

/*************************************************************************
 * SHSetThreadRef        [SHCORE.@]
 */
HRESULT WINAPI SHSetThreadRef(IUnknown *obj)
{
    TRACE("(%p)\n", obj);

    if (shcore_tls == TLS_OUT_OF_INDEXES)
        return E_NOINTERFACE;

    TlsSetValue(shcore_tls, obj);
    return S_OK;
}

/*************************************************************************
 * SHReleaseThreadRef        [SHCORE.@]
 */
HRESULT WINAPI SHReleaseThreadRef(void)
{
    FIXME("() - stub!\n");
    return S_OK;
}

/*************************************************************************
 * GetProcessReference        [SHCORE.@]
 */
HRESULT WINAPI GetProcessReference(IUnknown **obj)
{
    TRACE("(%p)\n", obj);

    *obj = process_ref;

    if (!process_ref)
        return E_FAIL;

    if (*obj)
        IUnknown_AddRef(*obj);

    return S_OK;
}

/*************************************************************************
 * SetProcessReference        [SHCORE.@]
 */
void WINAPI SetProcessReference(IUnknown *obj)
{
    TRACE("(%p)\n", obj);

    process_ref = obj;
}

struct thread_data
{
    LPTHREAD_START_ROUTINE thread_proc;
    LPTHREAD_START_ROUTINE callback;
    void *data;
    DWORD flags;
    HANDLE hEvent;
    IUnknown *thread_ref;
    IUnknown *process_ref;
};

static DWORD WINAPI shcore_thread_wrapper(void *data)
{
    struct thread_data thread_data;
    HRESULT hr = E_FAIL;
    DWORD retval;

    TRACE("(%p)\n", data);

    /* We are now executing in the context of the newly created thread.
     * So we copy the data passed to us (it is on the stack of the function
     * that called us, which is waiting for us to signal an event before
     * returning). */
    thread_data = *(struct thread_data *)data;

    if (thread_data.flags & CTF_COINIT)
    {
        hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
        if (FAILED(hr))
            hr = CoInitializeEx(NULL, COINIT_DISABLE_OLE1DDE);
    }

    if (thread_data.callback)
        thread_data.callback(thread_data.data);

    /* Signal the thread that created us; it can return now. */
    SetEvent(thread_data.hEvent);

    /* Execute the callers start code. */
    retval = thread_data.thread_proc(thread_data.data);

    /* Release thread and process references. */
    if (thread_data.thread_ref)
        IUnknown_Release(thread_data.thread_ref);

    if (thread_data.process_ref)
        IUnknown_Release(thread_data.process_ref);

    if (SUCCEEDED(hr))
        CoUninitialize();

    return retval;
}

/*************************************************************************
 *      SHCreateThread        [SHCORE.@]
 */
BOOL WINAPI SHCreateThread(LPTHREAD_START_ROUTINE thread_proc, void *data, DWORD flags, LPTHREAD_START_ROUTINE callback)
{
    struct thread_data thread_data;
    BOOL called = FALSE;

    TRACE("%p, %p, %#lx, %p.\n", thread_proc, data, flags, callback);

    thread_data.thread_proc = thread_proc;
    thread_data.callback = callback;
    thread_data.data = data;
    thread_data.flags = flags;
    thread_data.hEvent = CreateEventW(NULL, FALSE, FALSE, NULL);

    if (flags & CTF_THREAD_REF)
        SHGetThreadRef(&thread_data.thread_ref);
    else
        thread_data.thread_ref = NULL;

    if (flags & CTF_PROCESS_REF)
        GetProcessReference(&thread_data.process_ref);
    else
        thread_data.process_ref = NULL;

    /* Create the thread */
    if (thread_data.hEvent)
    {
        HANDLE hThread;
        DWORD retval;

        hThread = CreateThread(NULL, 0, shcore_thread_wrapper, &thread_data, 0, &retval);
        if (hThread)
        {
            /* Wait for the thread to signal us to continue */
            WaitForSingleObject(thread_data.hEvent, INFINITE);
            CloseHandle(hThread);
            called = TRUE;
        }
        CloseHandle(thread_data.hEvent);
    }

    if (!called)
    {
        if (!thread_data.callback && flags & CTF_INSIST)
        {
            /* Couldn't call, call synchronously */
            thread_data.thread_proc(data);
            called = TRUE;
        }
        else
        {
            if (thread_data.thread_ref)
                IUnknown_Release(thread_data.thread_ref);

            if (thread_data.process_ref)
                IUnknown_Release(thread_data.process_ref);
        }
    }

    return called;
}

/*************************************************************************
 * SHStrDupW    [SHCORE.@]
 */
HRESULT WINAPI SHStrDupW(const WCHAR *src, WCHAR **dest)
{
    size_t len;

    TRACE("(%s, %p)\n", debugstr_w(src), dest);

    *dest = NULL;

    if (!src)
        return E_INVALIDARG;

    len = (lstrlenW(src) + 1) * sizeof(WCHAR);
    *dest = CoTaskMemAlloc(len);
    if (!*dest)
        return E_OUTOFMEMORY;

    memcpy(*dest, src, len);

    return S_OK;
}

/*************************************************************************
 * SHStrDupA    [SHCORE.@]
 */
HRESULT WINAPI SHStrDupA(const char *src, WCHAR **dest)
{
    DWORD len;

    *dest = NULL;

    if (!src)
        return E_INVALIDARG;

    len = MultiByteToWideChar(CP_ACP, 0, src, -1, NULL, 0);
    *dest = CoTaskMemAlloc(len * sizeof(WCHAR));
    if (!*dest)
        return E_OUTOFMEMORY;

    MultiByteToWideChar(CP_ACP, 0, src, -1, *dest, len);

    return S_OK;
}

/*************************************************************************
 * SHAnsiToAnsi        [SHCORE.@]
 */
DWORD WINAPI SHAnsiToAnsi(const char *src, char *dest, int dest_len)
{
    DWORD ret;

    TRACE("(%s, %p, %d)\n", debugstr_a(src), dest, dest_len);

    if (!src || !dest || dest_len <= 0)
        return 0;

    lstrcpynA(dest, src, dest_len);
    ret = strlen(dest);

    return src[ret] ? 0 : ret + 1;
}

/*************************************************************************
 * SHUnicodeToAnsi        [SHCORE.@]
 */
DWORD WINAPI SHUnicodeToAnsi(const WCHAR *src, char *dest, int dest_len)
{
    int ret = 1;

    TRACE("(%s, %p, %d)\n", debugstr_w(src), dest, dest_len);

    if (!dest || !dest_len)
        return 0;

    if (src)
    {
        ret = WideCharToMultiByte(CP_ACP, 0, src, -1, dest, dest_len, NULL, NULL);
        if (!ret)
        {
            dest[dest_len - 1] = 0;
            ret = dest_len;
        }
    }
    else
        dest[0] = 0;

    return ret;
}

/*************************************************************************
 * SHUnicodeToUnicode        [SHCORE.@]
 */
DWORD WINAPI SHUnicodeToUnicode(const WCHAR *src, WCHAR *dest, int dest_len)
{
    DWORD ret;

    TRACE("(%s, %p, %d)\n", debugstr_w(src), dest, dest_len);

    if (!src || !dest || dest_len <= 0)
        return 0;

    lstrcpynW(dest, src, dest_len);
    ret = lstrlenW(dest);

    return src[ret] ? 0 : ret + 1;
}

/*************************************************************************
 * SHAnsiToUnicode        [SHCORE.@]
 */
DWORD WINAPI SHAnsiToUnicode(const char *src, WCHAR *dest, int dest_len)
{
    int ret = 1;

    TRACE("(%s, %p, %d)\n", debugstr_a(src), dest, dest_len);

    if (!dest || !dest_len)
        return 0;

    if (src)
    {
        ret = MultiByteToWideChar(CP_ACP, 0, src, -1, dest, dest_len);
        if (!ret)
        {
            dest[dest_len - 1] = 0;
            ret = dest_len;
        }
    }
    else
        dest[0] = 0;

    return ret;
}

/*************************************************************************
 * SHRegDuplicateHKey        [SHCORE.@]
 */
HKEY WINAPI SHRegDuplicateHKey(HKEY hKey)
{
    HKEY newKey = 0;

    RegOpenKeyExW(hKey, 0, 0, MAXIMUM_ALLOWED, &newKey);
    TRACE("new key is %p\n", newKey);
    return newKey;
}

/*************************************************************************
 * SHDeleteEmptyKeyW        [SHCORE.@]
 */
DWORD WINAPI SHDeleteEmptyKeyW(HKEY hkey, const WCHAR *subkey)
{
    DWORD ret, count = 0;
    HKEY hsubkey = 0;

    TRACE("(%p, %s)\n", hkey, debugstr_w(subkey));

    ret = RegOpenKeyExW(hkey, subkey, 0, KEY_READ, &hsubkey);
    if (!ret)
    {
        ret = RegQueryInfoKeyW(hsubkey, NULL, NULL, NULL, &count,
                             NULL, NULL, NULL, NULL, NULL, NULL, NULL);
        RegCloseKey(hsubkey);
        if (!ret)
        {
            if (count)
                ret = ERROR_KEY_HAS_CHILDREN;
            else
                ret = RegDeleteKeyW(hkey, subkey);
        }
    }

    return ret;
}

/*************************************************************************
 * SHDeleteEmptyKeyA        [SHCORE.@]
 */
DWORD WINAPI SHDeleteEmptyKeyA(HKEY hkey, const char *subkey)
{
    WCHAR *subkeyW = NULL;
    DWORD ret;

    TRACE("(%p, %s)\n", hkey, debugstr_a(subkey));

    if (subkey && FAILED(SHStrDupA(subkey, &subkeyW)))
        return ERROR_OUTOFMEMORY;

    ret = SHDeleteEmptyKeyW(hkey, subkeyW);
    CoTaskMemFree(subkeyW);
    return ret;
}

/*************************************************************************
 * SHDeleteKeyW        [SHCORE.@]
 */
DWORD WINAPI SHDeleteKeyW(HKEY hkey, const WCHAR *subkey)
{
    TRACE("(%p, %s)\n", hkey, debugstr_w(subkey));

    return RegDeleteTreeW(hkey, subkey);
}

/*************************************************************************
 * SHDeleteKeyA        [SHCORE.@]
 */
DWORD WINAPI SHDeleteKeyA(HKEY hkey, const char *subkey)
{
    TRACE("(%p, %s)\n", hkey, debugstr_a(subkey));

    return RegDeleteTreeA(hkey, subkey);
}

/*************************************************************************
 * SHDeleteValueW        [SHCORE.@]
 */
DWORD WINAPI SHDeleteValueW(HKEY hkey, const WCHAR *subkey, const WCHAR *value)
{
    HKEY hsubkey;
    DWORD ret;

    TRACE("(%p, %s, %s)\n", hkey, debugstr_w(subkey), debugstr_w(value));

    ret = RegOpenKeyExW(hkey, subkey, 0, KEY_SET_VALUE, &hsubkey);
    if (!ret)
    {
        ret = RegDeleteValueW(hsubkey, value);
        RegCloseKey(hsubkey);
    }

    return ret;
}

/*************************************************************************
 * SHDeleteValueA        [SHCORE.@]
 */
DWORD WINAPI SHDeleteValueA(HKEY hkey, const char *subkey, const char *value)
{
    WCHAR *subkeyW = NULL, *valueW = NULL;
    DWORD ret;

    TRACE("(%p, %s, %s)\n", hkey, debugstr_a(subkey), debugstr_a(value));

    if (subkey && FAILED(SHStrDupA(subkey, &subkeyW)))
        return ERROR_OUTOFMEMORY;
    if (value && FAILED(SHStrDupA(value, &valueW)))
    {
        CoTaskMemFree(subkeyW);
        return ERROR_OUTOFMEMORY;
    }

    ret = SHDeleteValueW(hkey, subkeyW, valueW);
    CoTaskMemFree(subkeyW);
    CoTaskMemFree(valueW);
    return ret;
}

/*************************************************************************
 * SHCopyKeyA        [SHCORE.@]
 */
DWORD WINAPI SHCopyKeyA(HKEY hkey_src, const char *subkey, HKEY hkey_dst, DWORD reserved)
{
    WCHAR *subkeyW = NULL;
    DWORD ret;

    TRACE("%p, %s, %p, %ld.\n", hkey_src, debugstr_a(subkey), hkey_dst, reserved);

    if (subkey && FAILED(SHStrDupA(subkey, &subkeyW)))
        return 0;

    ret = SHCopyKeyW(hkey_src, subkeyW, hkey_dst, reserved);
    CoTaskMemFree(subkeyW);
    return ret;
}

/*************************************************************************
 * SHCopyKeyW        [SHCORE.@]
 */
DWORD WINAPI SHCopyKeyW(HKEY hkey_src, const WCHAR *subkey, HKEY hkey_dst, DWORD reserved)
{
    DWORD key_count = 0, value_count = 0, max_key_len = 0;
    WCHAR name[MAX_PATH], *ptr_name = name;
    BYTE buff[1024], *ptr = buff;
    DWORD max_data_len = 0, i;
    DWORD ret = 0;

    TRACE("%p, %s, %p, %ld.\n", hkey_src, debugstr_w(subkey), hkey_dst, reserved);

    if (!hkey_dst || !hkey_src)
        return ERROR_INVALID_PARAMETER;

    if (subkey)
        ret = RegOpenKeyExW(hkey_src, subkey, 0, KEY_ALL_ACCESS, &hkey_src);

    if (ret)
        hkey_src = NULL; /* Don't close this key since we didn't open it */
    else
    {
        DWORD max_value_len;

        ret = RegQueryInfoKeyW(hkey_src, NULL, NULL, NULL, &key_count, &max_key_len,
                NULL, &value_count, &max_value_len, &max_data_len, NULL, NULL);
        if (!ret)
        {
            /* Get max size for key/value names */
            max_key_len = max(max_key_len, max_value_len);

            if (max_key_len++ > MAX_PATH - 1)
                ptr_name = heap_alloc(max_key_len * sizeof(WCHAR));

            if (max_data_len > sizeof(buff))
                ptr = heap_alloc(max_data_len);

            if (!ptr_name || !ptr)
                ret = ERROR_NOT_ENOUGH_MEMORY;
        }
    }

    for (i = 0; i < key_count && !ret; i++)
    {
        HKEY hsubkey_src, hsubkey_dst;
        DWORD length = max_key_len;

        ret = RegEnumKeyExW(hkey_src, i, ptr_name, &length, NULL, NULL, NULL, NULL);
        if (!ret)
        {
            ret = RegOpenKeyExW(hkey_src, ptr_name, 0, KEY_READ, &hsubkey_src);
            if (!ret)
            {
                /* Create destination sub key */
                ret = RegCreateKeyW(hkey_dst, ptr_name, &hsubkey_dst);
                if (!ret)
                {
                    /* Recursively copy keys and values from the sub key */
                    ret = SHCopyKeyW(hsubkey_src, NULL, hsubkey_dst, 0);
                    RegCloseKey(hsubkey_dst);
                }
            }
            RegCloseKey(hsubkey_src);
        }
    }

    /* Copy all the values in this key */
    for (i = 0; i < value_count && !ret; i++)
    {
        DWORD length = max_key_len, type, data_len = max_data_len;

        ret = RegEnumValueW(hkey_src, i, ptr_name, &length, NULL, &type, ptr, &data_len);
        if (!ret) {
            ret = SHSetValueW(hkey_dst, NULL, ptr_name, type, ptr, data_len);
        }
    }

    /* Free buffers if allocated */
    if (ptr_name != name)
        heap_free(ptr_name);
    if (ptr != buff)
        heap_free(ptr);

    if (subkey && hkey_src)
        RegCloseKey(hkey_src);

    return ret;
}


/*************************************************************************
 * SHEnumKeyExA        [SHCORE.@]
 */
LONG WINAPI SHEnumKeyExA(HKEY hkey, DWORD index, char *subkey, DWORD *length)
{
    TRACE("%p, %ld, %s, %p.\n", hkey, index, debugstr_a(subkey), length);

    return RegEnumKeyExA(hkey, index, subkey, length, NULL, NULL, NULL, NULL);
}

/*************************************************************************
 * SHEnumKeyExW        [SHCORE.@]
 */
LONG WINAPI SHEnumKeyExW(HKEY hkey, DWORD index, WCHAR *subkey, DWORD *length)
{
    TRACE("%p, %ld, %s, %p.\n", hkey, index, debugstr_w(subkey), length);

    return RegEnumKeyExW(hkey, index, subkey, length, NULL, NULL, NULL, NULL);
}

/*************************************************************************
 * SHEnumValueA        [SHCORE.@]
 */
LONG WINAPI SHEnumValueA(HKEY hkey, DWORD index, char *value, DWORD *length, DWORD *type,
        void *data, DWORD *data_len)
{
    TRACE("%p, %ld, %s, %p, %p, %p, %p.\n", hkey, index, debugstr_a(value), length, type, data, data_len);

    return RegEnumValueA(hkey, index, value, length, NULL, type, data, data_len);
}

/*************************************************************************
 * SHEnumValueW        [SHCORE.@]
 */
LONG WINAPI SHEnumValueW(HKEY hkey, DWORD index, WCHAR *value, DWORD *length, DWORD *type,
        void *data, DWORD *data_len)
{
    TRACE("%p, %ld, %s, %p, %p, %p, %p.\n", hkey, index, debugstr_w(value), length, type, data, data_len);

    return RegEnumValueW(hkey, index, value, length, NULL, type, data, data_len);
}

/*************************************************************************
 * SHQueryValueExW    [SHCORE.@]
 */
DWORD WINAPI SHQueryValueExW(HKEY hkey, const WCHAR *name, DWORD *reserved, DWORD *type,
        void *buff, DWORD *buff_len)
{
    DWORD ret, value_type, data_len = 0;

    TRACE("(%p, %s, %p, %p, %p, %p)\n", hkey, debugstr_w(name), reserved, type, buff, buff_len);

    if (buff_len)
        data_len = *buff_len;

    ret = RegQueryValueExW(hkey, name, reserved, &value_type, buff, &data_len);
    if (ret != ERROR_SUCCESS && ret != ERROR_MORE_DATA)
        return ret;

    if (buff_len && value_type == REG_EXPAND_SZ)
    {
        DWORD length;
        WCHAR *value;

        if (!buff || ret == ERROR_MORE_DATA)
        {
            length = data_len;
            value = heap_alloc(length);
            RegQueryValueExW(hkey, name, reserved, NULL, (BYTE *)value, &length);
            length = ExpandEnvironmentStringsW(value, NULL, 0);
        }
        else
        {
            length = (lstrlenW(buff) + 1) * sizeof(WCHAR);
            value = heap_alloc(length);
            memcpy(value, buff, length);
            length = ExpandEnvironmentStringsW(value, buff, *buff_len / sizeof(WCHAR));
            if (length > *buff_len) ret = ERROR_MORE_DATA;
        }
        data_len = max(data_len, length);
        heap_free(value);
    }

    if (type)
        *type = value_type == REG_EXPAND_SZ ? REG_SZ : value_type;
    if (buff_len)
        *buff_len = data_len;
    return ret;
}

/*************************************************************************
 * SHQueryValueExA    [SHCORE.@]
 */
DWORD WINAPI SHQueryValueExA(HKEY hkey, const char *name, DWORD *reserved, DWORD *type,
        void *buff, DWORD *buff_len)
{
    DWORD ret, value_type, data_len = 0;

    TRACE("(%p, %s, %p, %p, %p, %p)\n", hkey, debugstr_a(name), reserved, type, buff, buff_len);

    if (buff_len)
        data_len = *buff_len;

    ret = RegQueryValueExA(hkey, name, reserved, &value_type, buff, &data_len);
    if (ret != ERROR_SUCCESS && ret != ERROR_MORE_DATA)
        return ret;

    if (buff_len && value_type == REG_EXPAND_SZ)
    {
        DWORD length;
        char *value;

        if (!buff || ret == ERROR_MORE_DATA)
        {
            length = data_len;
            value = heap_alloc(length);
            RegQueryValueExA(hkey, name, reserved, NULL, (BYTE *)value, &length);
            length = ExpandEnvironmentStringsA(value, NULL, 0);
        }
        else
        {
            length = strlen(buff) + 1;
            value = heap_alloc(length);
            memcpy(value, buff, length);
            length = ExpandEnvironmentStringsA(value, buff, *buff_len);
            if (length > *buff_len) ret = ERROR_MORE_DATA;
        }
        data_len = max(data_len, length);
        heap_free(value);
    }

    if (type)
        *type = value_type == REG_EXPAND_SZ ? REG_SZ : value_type;
    if (buff_len)
        *buff_len = data_len;
    return ret;
}

/*************************************************************************
 * SHGetValueA        [SHCORE.@]
 */
DWORD WINAPI SHGetValueA(HKEY hkey, const char *subkey, const char *value,
       DWORD *type, void *data, DWORD *data_len)
{
    HKEY hsubkey = 0;
    DWORD ret = 0;

    TRACE("(%p, %s, %s, %p, %p, %p)\n", hkey, debugstr_a(subkey), debugstr_a(value),
            type, data, data_len);

    if (subkey)
        ret = RegOpenKeyExA(hkey, subkey, 0, KEY_QUERY_VALUE, &hsubkey);

    if (!ret)
    {
        ret = SHQueryValueExA(hsubkey ? hsubkey : hkey, value, 0, type, data, data_len);
        if (subkey)
            RegCloseKey(hsubkey);
    }

    return ret;
}

/*************************************************************************
 * SHGetValueW        [SHCORE.@]
 */
DWORD WINAPI SHGetValueW(HKEY hkey, const WCHAR *subkey, const WCHAR *value,
        DWORD *type, void *data, DWORD *data_len)
{
    HKEY hsubkey = 0;
    DWORD ret = 0;

    TRACE("(%p, %s, %s, %p, %p, %p)\n", hkey, debugstr_w(subkey), debugstr_w(value),
            type, data, data_len);

    if (subkey)
        ret = RegOpenKeyExW(hkey, subkey, 0, KEY_QUERY_VALUE, &hsubkey);

    if (!ret)
    {
        ret = SHQueryValueExW(hsubkey ? hsubkey : hkey, value, 0, type, data, data_len);
        if (subkey)
            RegCloseKey(hsubkey);
    }

    return ret;
}

/*************************************************************************
 * SHRegGetIntW        [SHCORE.280]
 */
int WINAPI SHRegGetIntW(HKEY hkey, const WCHAR *value, int default_value)
{
    WCHAR buff[32];
    DWORD buff_len;

    TRACE("(%p, %s, %d)\n", hkey, debugstr_w(value), default_value);

    buff[0] = 0;
    buff_len = sizeof(buff);
    if (SHQueryValueExW(hkey, value, 0, 0, buff, &buff_len))
        return default_value;

    if (*buff >= '0' && *buff <= '9')
        return wcstol(buff, NULL, 10);

    return default_value;
}

/*************************************************************************
 * SHRegGetPathA        [SHCORE.@]
 */
DWORD WINAPI SHRegGetPathA(HKEY hkey, const char *subkey, const char *value, char *path, DWORD flags)
{
    DWORD length = MAX_PATH;

    TRACE("%p, %s, %s, %p, %#lx.\n", hkey, debugstr_a(subkey), debugstr_a(value), path, flags);

    return SHGetValueA(hkey, subkey, value, 0, path, &length);
}

/*************************************************************************
 * SHRegGetPathW        [SHCORE.@]
 */
DWORD WINAPI SHRegGetPathW(HKEY hkey, const WCHAR *subkey, const WCHAR *value, WCHAR *path, DWORD flags)
{
    DWORD length = MAX_PATH;

    TRACE("%p, %s, %s, %p, %#lx.\n", hkey, debugstr_w(subkey), debugstr_w(value), path, flags);

    return SHGetValueW(hkey, subkey, value, 0, path, &length);
}

/*************************************************************************
 * SHSetValueW       [SHCORE.@]
 */
DWORD WINAPI SHSetValueW(HKEY hkey, const WCHAR *subkey, const WCHAR *value, DWORD type,
        const void *data, DWORD data_len)
{
    DWORD ret = ERROR_SUCCESS, dummy;
    HKEY hsubkey;

    TRACE("%p, %s, %s, %ld, %p, %ld.\n", hkey, debugstr_w(subkey), debugstr_w(value),
            type, data, data_len);

    if (subkey && *subkey)
        ret = RegCreateKeyExW(hkey, subkey, 0, NULL, 0, KEY_SET_VALUE, NULL, &hsubkey, &dummy);
    else
        hsubkey = hkey;

    if (!ret)
    {
        ret = RegSetValueExW(hsubkey, value, 0, type, data, data_len);
        if (hsubkey != hkey)
            RegCloseKey(hsubkey);
    }

    return ret;
}

/*************************************************************************
 * SHSetValueA        [SHCORE.@]
 */
DWORD WINAPI SHSetValueA(HKEY hkey, const char *subkey, const char *value,
        DWORD type, const void *data, DWORD data_len)
{
    DWORD ret = ERROR_SUCCESS, dummy;
    HKEY hsubkey;

    TRACE("%p, %s, %s, %ld, %p, %ld.\n", hkey, debugstr_a(subkey), debugstr_a(value),
            type, data, data_len);

    if (subkey && *subkey)
        ret = RegCreateKeyExA(hkey, subkey, 0, NULL, 0, KEY_SET_VALUE, NULL, &hsubkey, &dummy);
    else
        hsubkey = hkey;

    if (!ret)
    {
        ret = RegSetValueExA(hsubkey, value, 0, type, data, data_len);
        if (hsubkey != hkey)
            RegCloseKey(hsubkey);
    }

    return ret;
}

/*************************************************************************
 * SHRegSetPathA       [SHCORE.@]
 */
DWORD WINAPI SHRegSetPathA(HKEY hkey, const char *subkey, const char *value, const char *path, DWORD flags)
{
    FIXME("%p, %s, %s, %s, %#lx - semi-stub\n", hkey, debugstr_a(subkey),
            debugstr_a(value), debugstr_a(path), flags);

    /* FIXME: PathUnExpandEnvStringsA() */

    return SHSetValueA(hkey, subkey, value, REG_SZ, path, lstrlenA(path));
}

/*************************************************************************
 * SHRegSetPathW       [SHCORE.@]
 */
DWORD WINAPI SHRegSetPathW(HKEY hkey, const WCHAR *subkey, const WCHAR *value, const WCHAR *path, DWORD flags)
{
    FIXME("%p, %s, %s, %s, %#lx semi-stub\n", hkey, debugstr_w(subkey),
            debugstr_w(value), debugstr_w(path), flags);

    /* FIXME: PathUnExpandEnvStringsW(); */

    return SHSetValueW(hkey, subkey, value, REG_SZ, path, lstrlenW(path));
}

/*************************************************************************
 * SHQueryInfoKeyA       [SHCORE.@]
 */
LONG WINAPI SHQueryInfoKeyA(HKEY hkey, DWORD *subkeys, DWORD *subkey_max, DWORD *values, DWORD *value_max)
{
    TRACE("(%p, %p, %p, %p, %p)\n", hkey, subkeys, subkey_max, values, value_max);

    return RegQueryInfoKeyA(hkey, NULL, NULL, NULL, subkeys, subkey_max, NULL, values, value_max, NULL, NULL, NULL);
}

/*************************************************************************
 * SHQueryInfoKeyW       [SHCORE.@]
 */
LONG WINAPI SHQueryInfoKeyW(HKEY hkey, DWORD *subkeys, DWORD *subkey_max, DWORD *values, DWORD *value_max)
{
    TRACE("(%p, %p, %p, %p, %p)\n", hkey, subkeys, subkey_max, values, value_max);

    return RegQueryInfoKeyW(hkey, NULL, NULL, NULL, subkeys, subkey_max, NULL, values, value_max, NULL, NULL, NULL);
}

/*************************************************************************
 * IsOS        [SHCORE.@]
 */
BOOL WINAPI IsOS(DWORD feature)
{
    DWORD platform, majorv, minorv;
    OSVERSIONINFOA osvi;

    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOA);
    if (!GetVersionExA(&osvi))
        return FALSE;

    majorv = osvi.dwMajorVersion;
    minorv = osvi.dwMinorVersion;
    platform = osvi.dwPlatformId;

#define ISOS_RETURN(x) \
    TRACE("(%#lx) ret %d\n",feature,(x)); \
    return (x)

    switch(feature)  {
    case OS_WIN32SORGREATER:
        ISOS_RETURN(platform == VER_PLATFORM_WIN32s
                 || platform == VER_PLATFORM_WIN32_WINDOWS);
    case OS_NT:
        ISOS_RETURN(platform == VER_PLATFORM_WIN32_NT);
    case OS_WIN95ORGREATER:
        ISOS_RETURN(platform == VER_PLATFORM_WIN32_WINDOWS);
    case OS_NT4ORGREATER:
        ISOS_RETURN(platform == VER_PLATFORM_WIN32_NT && majorv >= 4);
    case OS_WIN2000ORGREATER_ALT:
    case OS_WIN2000ORGREATER:
        ISOS_RETURN(platform == VER_PLATFORM_WIN32_NT && majorv >= 5);
    case OS_WIN98ORGREATER:
        ISOS_RETURN(platform == VER_PLATFORM_WIN32_WINDOWS && minorv >= 10);
    case OS_WIN98_GOLD:
        ISOS_RETURN(platform == VER_PLATFORM_WIN32_WINDOWS && minorv == 10);
    case OS_WIN2000PRO:
        ISOS_RETURN(platform == VER_PLATFORM_WIN32_NT && majorv >= 5);
    case OS_WIN2000SERVER:
        ISOS_RETURN(platform == VER_PLATFORM_WIN32_NT && (minorv == 0 || minorv == 1));
    case OS_WIN2000ADVSERVER:
        ISOS_RETURN(platform == VER_PLATFORM_WIN32_NT && (minorv == 0 || minorv == 1));
    case OS_WIN2000DATACENTER:
        ISOS_RETURN(platform == VER_PLATFORM_WIN32_NT && (minorv == 0 || minorv == 1));
    case OS_WIN2000TERMINAL:
        ISOS_RETURN(platform == VER_PLATFORM_WIN32_NT && (minorv == 0 || minorv == 1));
    case OS_EMBEDDED:
        FIXME("(OS_EMBEDDED) What should we return here?\n");
        return FALSE;
    case OS_TERMINALCLIENT:
        FIXME("(OS_TERMINALCLIENT) What should we return here?\n");
        return FALSE;
    case OS_TERMINALREMOTEADMIN:
        FIXME("(OS_TERMINALREMOTEADMIN) What should we return here?\n");
        return FALSE;
    case OS_WIN95_GOLD:
        ISOS_RETURN(platform == VER_PLATFORM_WIN32_WINDOWS && minorv == 0);
    case OS_MEORGREATER:
        ISOS_RETURN(platform == VER_PLATFORM_WIN32_WINDOWS && minorv >= 90);
    case OS_XPORGREATER:
        ISOS_RETURN(platform == VER_PLATFORM_WIN32_NT && majorv >= 5 && minorv >= 1);
    case OS_HOME:
        ISOS_RETURN(platform == VER_PLATFORM_WIN32_NT && majorv >= 5 && minorv >= 1);
    case OS_PROFESSIONAL:
        ISOS_RETURN(platform == VER_PLATFORM_WIN32_NT);
    case OS_DATACENTER:
        ISOS_RETURN(platform == VER_PLATFORM_WIN32_NT);
    case OS_ADVSERVER:
        ISOS_RETURN(platform == VER_PLATFORM_WIN32_NT && majorv >= 5);
    case OS_SERVER:
        ISOS_RETURN(platform == VER_PLATFORM_WIN32_NT);
    case OS_TERMINALSERVER:
        ISOS_RETURN(platform == VER_PLATFORM_WIN32_NT);
    case OS_PERSONALTERMINALSERVER:
        ISOS_RETURN(platform == VER_PLATFORM_WIN32_NT && minorv >= 1 && majorv >= 5);
    case OS_FASTUSERSWITCHING:
        FIXME("(OS_FASTUSERSWITCHING) What should we return here?\n");
        return TRUE;
    case OS_WELCOMELOGONUI:
        FIXME("(OS_WELCOMELOGONUI) What should we return here?\n");
        return FALSE;
    case OS_DOMAINMEMBER:
        FIXME("(OS_DOMAINMEMBER) What should we return here?\n");
        return TRUE;
    case OS_ANYSERVER:
        ISOS_RETURN(platform == VER_PLATFORM_WIN32_NT);
    case OS_WOW6432:
        {
            BOOL is_wow64;
            IsWow64Process(GetCurrentProcess(), &is_wow64);
            return is_wow64;
        }
    case OS_WEBSERVER:
        ISOS_RETURN(platform == VER_PLATFORM_WIN32_NT);
    case OS_SMALLBUSINESSSERVER:
        ISOS_RETURN(platform == VER_PLATFORM_WIN32_NT);
    case OS_TABLETPC:
        FIXME("(OS_TABLETPC) What should we return here?\n");
        return FALSE;
    case OS_SERVERADMINUI:
        FIXME("(OS_SERVERADMINUI) What should we return here?\n");
        return FALSE;
    case OS_MEDIACENTER:
        FIXME("(OS_MEDIACENTER) What should we return here?\n");
        return FALSE;
    case OS_APPLIANCE:
        FIXME("(OS_APPLIANCE) What should we return here?\n");
        return FALSE;
    case 0x25: /*OS_VISTAORGREATER*/
        ISOS_RETURN(platform == VER_PLATFORM_WIN32_NT && majorv >= 6);
    }

#undef ISOS_RETURN

    WARN("(%#lx) unknown parameter\n", feature);

    return FALSE;
}

/*************************************************************************
 * SubscribeFeatureStateChangeNotification        [SHCORE.@]
 */
void WINAPI SubscribeFeatureStateChangeNotification(FEATURE_STATE_CHANGE_SUBSCRIPTION *subscription,
                                                    FEATURE_STATE_CHANGE_CALLBACK *callback, void *context)
{
    FIXME("(%p, %p, %p) stub\n", subscription, callback, context);
}

/*************************************************************************
 * GetFeatureEnabledState        [SHCORE.@]
 */
FEATURE_ENABLED_STATE WINAPI GetFeatureEnabledState(UINT32 feature, FEATURE_CHANGE_TIME change_time)
{
    FIXME("(%u, %u) stub\n", feature, change_time);
    return FEATURE_ENABLED_STATE_DEFAULT;
}
