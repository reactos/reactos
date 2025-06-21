/*
 * File Source Filter
 *
 * Copyright 2003 Robert Shearman
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

#include "quartz_private.h"

#include "wine/debug.h"
#include "uuids.h"
#include "vfwmsgs.h"
#include "winbase.h"
#include "winreg.h"
#include "shlwapi.h"
#include <assert.h>

WINE_DEFAULT_DEBUG_CHANNEL(quartz);

static const AM_MEDIA_TYPE default_mt =
{
    {0xe436eb83,0x524f,0x11ce,{0x9f,0x53,0x00,0x20,0xaf,0x0b,0xa7,0x70}},   /* MEDIATYPE_Stream */
    {0,0,0,{0,0,0,0,0,0,0,0}},
    TRUE,
    FALSE,
    1,
    {0,0,0,{0,0,0,0,0,0,0,0}},
    NULL,
    0,
    NULL
};

struct request
{
    IMediaSample *sample;
    DWORD_PTR cookie;
    OVERLAPPED ovl;
};

struct async_reader
{
    struct strmbase_filter filter;
    IFileSourceFilter IFileSourceFilter_iface;

    struct strmbase_source source;
    IAsyncReader IAsyncReader_iface;

    LPOLESTR pszFileName;
    AM_MEDIA_TYPE mt;
    HANDLE file, port, io_thread;
    LARGE_INTEGER file_size;
    CRITICAL_SECTION sample_cs;
    BOOL flushing;
    struct request *requests;
    unsigned int max_requests;
    CONDITION_VARIABLE sample_cv;
};

static const struct strmbase_source_ops source_ops;

static inline struct async_reader *impl_from_strmbase_filter(struct strmbase_filter *iface)
{
    return CONTAINING_RECORD(iface, struct async_reader, filter);
}

static inline struct async_reader *impl_from_IFileSourceFilter(IFileSourceFilter *iface)
{
    return CONTAINING_RECORD(iface, struct async_reader, IFileSourceFilter_iface);
}

static const IFileSourceFilterVtbl FileSource_Vtbl;
static const IAsyncReaderVtbl FileAsyncReader_Vtbl;

static int byte_from_hex_char(WCHAR c)
{
    if ('0' <= c && c <= '9') return c - '0';
    if ('a' <= c && c <= 'f') return c - 'a' + 10;
    if ('A' <= c && c <= 'F') return c - 'A' + 10;
    return -1;
}

static BOOL process_pattern_string(const WCHAR *pattern, HANDLE file)
{
    ULONG size, offset, i, ret_size;
    BYTE *mask, *expect, *actual;
    int d;
    BOOL ret = TRUE;

    /* format: "offset, size, mask, value" */

    offset = wcstol(pattern, NULL, 10);

    if (!(pattern = wcschr(pattern, ',')))
        return FALSE;
    pattern++;

    size = wcstol(pattern, NULL, 10);
    mask = heap_alloc(size);
    expect = heap_alloc(size);
    memset(mask, 0xff, size);

    if (!(pattern = wcschr(pattern, ',')))
    {
        heap_free(mask);
        heap_free(expect);
        return FALSE;
    }
    pattern++;
    while (byte_from_hex_char(*pattern) == -1 && (*pattern != ','))
        pattern++;

    for (i = 0; (d = byte_from_hex_char(*pattern)) != -1 && (i/2 < size); pattern++, i++)
    {
        if (i % 2)
            mask[i / 2] |= d;
        else
            mask[i / 2] = d << 4;
    }

    if (!(pattern = wcschr(pattern, ',')))
    {
        heap_free(mask);
        heap_free(expect);
        return FALSE;
    }
    pattern++;
    while (byte_from_hex_char(*pattern) == -1 && (*pattern != ','))
        pattern++;

    for (i = 0; (d = byte_from_hex_char(*pattern)) != -1 && (i/2 < size); pattern++, i++)
    {
        if (i % 2)
            expect[i / 2] |= d;
        else
            expect[i / 2] = d << 4;
    }

    actual = heap_alloc(size);
    SetFilePointer(file, offset, NULL, FILE_BEGIN);
    if (!ReadFile(file, actual, size, &ret_size, NULL) || ret_size != size)
    {
        heap_free(actual);
        heap_free(expect);
        heap_free(mask);
        return FALSE;
    }

    for (i = 0; i < size; ++i)
    {
        if ((actual[i] & mask[i]) != expect[i])
        {
            ret = FALSE;
            break;
        }
    }

    heap_free(actual);
    heap_free(expect);
    heap_free(mask);

    /* If there is a following tuple, then we must match that as well. */
    if (ret && (pattern = wcschr(pattern, ',')))
        return process_pattern_string(pattern + 1, file);

    return ret;
}

BOOL get_media_type(const WCHAR *filename, GUID *majortype, GUID *subtype, GUID *source_clsid)
{
    WCHAR extensions_path[278] = L"Media Type\\Extensions\\";
    DWORD majortype_idx, size;
    const WCHAR *ext;
    HKEY parent_key;
    HANDLE file;

    if ((ext = wcsrchr(filename, '.')))
    {
        WCHAR guidstr[39];
        HKEY key;

        wcscat(extensions_path, ext);
        if (!RegOpenKeyExW(HKEY_CLASSES_ROOT, extensions_path, 0, KEY_READ, &key))
        {
            size = sizeof(guidstr);
            if (majortype && !RegQueryValueExW(key, L"Media Type", NULL, NULL, (BYTE *)guidstr, &size))
                CLSIDFromString(guidstr, majortype);

            size = sizeof(guidstr);
            if (subtype && !RegQueryValueExW(key, L"Subtype", NULL, NULL, (BYTE *)guidstr, &size))
                CLSIDFromString(guidstr, subtype);

            size = sizeof(guidstr);
            if (source_clsid && !RegQueryValueExW(key, L"Source Filter", NULL, NULL, (BYTE *)guidstr, &size))
                CLSIDFromString(guidstr, source_clsid);

            RegCloseKey(key);
            return FALSE;
        }
    }

    if ((file = CreateFileW(filename, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_DELETE, NULL,
            OPEN_EXISTING, 0, NULL)) == INVALID_HANDLE_VALUE)
    {
        WARN("Failed to open file %s, error %lu.\n", debugstr_w(filename), GetLastError());
        return FALSE;
    }

    if (RegOpenKeyExW(HKEY_CLASSES_ROOT, L"Media Type", 0, KEY_READ, &parent_key))
    {
        CloseHandle(file);
        return FALSE;
    }

    for (majortype_idx = 0; ; ++majortype_idx)
    {
        WCHAR majortype_str[39];
        HKEY majortype_key;
        DWORD subtype_idx;

        size = ARRAY_SIZE(majortype_str);
        if (RegEnumKeyExW(parent_key, majortype_idx, majortype_str, &size, NULL, NULL, NULL, NULL))
            break;

        if (!wcscmp(majortype_str, L"Extensions"))
            continue;

        if (RegOpenKeyExW(parent_key, majortype_str, 0, KEY_READ, &majortype_key))
            continue;

        for (subtype_idx = 0; ; ++subtype_idx)
        {
            WCHAR subtype_str[39], *pattern;
            DWORD value_idx, max_size;
            HKEY subtype_key;

            size = ARRAY_SIZE(subtype_str);
            if (RegEnumKeyExW(majortype_key, subtype_idx, subtype_str, &size, NULL, NULL, NULL, NULL))
                break;

            if (RegOpenKeyExW(majortype_key, subtype_str, 0, KEY_READ, &subtype_key))
                continue;

            if (RegQueryInfoKeyW(subtype_key, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, &max_size, NULL, NULL))
                continue;

            pattern = heap_alloc(max_size);

            for (value_idx = 0; ; ++value_idx)
            {
                /* The longest name we should encounter is "Source Filter". */
                WCHAR value_name[14], source_clsid_str[39];
                DWORD value_len = ARRAY_SIZE(value_name);

                size = max_size;
                if (RegEnumValueW(subtype_key, value_idx, value_name, &value_len,
                        NULL, NULL, (BYTE *)pattern, &max_size))
                    break;

                if (!wcscmp(value_name, L"Source Filter"))
                    continue;

                if (!process_pattern_string(pattern, file))
                    continue;

                if (majortype)
                    CLSIDFromString(majortype_str, majortype);
                if (subtype)
                    CLSIDFromString(subtype_str, subtype);
                size = sizeof(source_clsid_str);
                if (source_clsid && !RegQueryValueExW(subtype_key, L"Source Filter",
                        NULL, NULL, (BYTE *)source_clsid_str, &size))
                    CLSIDFromString(source_clsid_str, source_clsid);

                heap_free(pattern);
                RegCloseKey(subtype_key);
                RegCloseKey(majortype_key);
                RegCloseKey(parent_key);
                CloseHandle(file);
                return TRUE;
            }

            heap_free(pattern);
            RegCloseKey(subtype_key);
        }

        RegCloseKey(majortype_key);
    }

    RegCloseKey(parent_key);
    CloseHandle(file);
    return FALSE;
}

static struct strmbase_pin *async_reader_get_pin(struct strmbase_filter *iface, unsigned int index)
{
    struct async_reader *filter = impl_from_strmbase_filter(iface);

    if (!index && filter->pszFileName)
        return &filter->source.pin;
    return NULL;
}

static void async_reader_destroy(struct strmbase_filter *iface)
{
    struct async_reader *filter = impl_from_strmbase_filter(iface);

    if (filter->pszFileName)
    {
        unsigned int i;

        if (filter->source.pin.peer)
            IPin_Disconnect(filter->source.pin.peer);

        IPin_Disconnect(&filter->source.pin.IPin_iface);

        if (filter->requests)
        {
            for (i = 0; i < filter->max_requests; ++i)
                CloseHandle(filter->requests[i].ovl.hEvent);
            free(filter->requests);
        }
        CloseHandle(filter->file);
        strmbase_source_cleanup(&filter->source);

        free(filter->pszFileName);
        FreeMediaType(&filter->mt);
    }

    filter->sample_cs.DebugInfo->Spare[0] = 0;
    DeleteCriticalSection(&filter->sample_cs);

    PostQueuedCompletionStatus(filter->port, 0, 1, NULL);
    WaitForSingleObject(filter->io_thread, INFINITE);
    CloseHandle(filter->io_thread);
    CloseHandle(filter->port);

    strmbase_filter_cleanup(&filter->filter);
    free(filter);
}

static HRESULT async_reader_query_interface(struct strmbase_filter *iface, REFIID iid, void **out)
{
    struct async_reader *filter = impl_from_strmbase_filter(iface);

    if (IsEqualGUID(iid, &IID_IFileSourceFilter))
    {
        *out = &filter->IFileSourceFilter_iface;
        IUnknown_AddRef((IUnknown *)*out);
        return S_OK;
    }

    return E_NOINTERFACE;
}

static const struct strmbase_filter_ops filter_ops =
{
    .filter_get_pin = async_reader_get_pin,
    .filter_destroy = async_reader_destroy,
    .filter_query_interface = async_reader_query_interface,
};

static DWORD CALLBACK io_thread(void *arg)
{
    struct async_reader *filter = arg;
    struct request *req;
    OVERLAPPED *ovl;
    ULONG_PTR key;
    DWORD size;
    BOOL ret;

    SetThreadDescription(GetCurrentThread(), L"wine_qz_async_reader_io");

    for (;;)
    {
        ret = GetQueuedCompletionStatus(filter->port, &size, &key, &ovl, INFINITE);

        if (ret && key)
            break;

        EnterCriticalSection(&filter->sample_cs);

        req = CONTAINING_RECORD(ovl, struct request, ovl);
        TRACE("Got sample %Iu.\n", req - filter->requests);
        assert(req >= filter->requests && req < filter->requests + filter->max_requests);

        if (ret)
            WakeConditionVariable(&filter->sample_cv);
        else
        {
            ERR("GetQueuedCompletionStatus() returned failure, error %lu.\n", GetLastError());
            req->sample = NULL;
        }

        LeaveCriticalSection(&filter->sample_cs);
    }

    return 0;
}

HRESULT async_reader_create(IUnknown *outer, IUnknown **out)
{
    struct async_reader *object;

    if (!(object = calloc(1, sizeof(*object))))
        return E_OUTOFMEMORY;

    strmbase_filter_init(&object->filter, outer, &CLSID_AsyncReader, &filter_ops);

    object->IFileSourceFilter_iface.lpVtbl = &FileSource_Vtbl;
    object->IAsyncReader_iface.lpVtbl = &FileAsyncReader_Vtbl;

    InitializeCriticalSectionEx(&object->sample_cs, 0, RTL_CRITICAL_SECTION_FLAG_FORCE_DEBUG_INFO);
    object->sample_cs.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": FileAsyncReader.sample_cs");
    InitializeConditionVariable(&object->sample_cv);
    object->port = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
    object->io_thread = CreateThread(NULL, 0, io_thread, object, 0, NULL);

    TRACE("Created file source %p.\n", object);
    *out = &object->filter.IUnknown_inner;
    return S_OK;
}

static HRESULT WINAPI FileSource_QueryInterface(IFileSourceFilter * iface, REFIID riid, LPVOID * ppv)
{
    struct async_reader *filter = impl_from_IFileSourceFilter(iface);
    return IBaseFilter_QueryInterface(&filter->filter.IBaseFilter_iface, riid, ppv);
}

static ULONG WINAPI FileSource_AddRef(IFileSourceFilter * iface)
{
    struct async_reader *filter = impl_from_IFileSourceFilter(iface);
    return IBaseFilter_AddRef(&filter->filter.IBaseFilter_iface);
}

static ULONG WINAPI FileSource_Release(IFileSourceFilter * iface)
{
    struct async_reader *filter = impl_from_IFileSourceFilter(iface);
    return IBaseFilter_Release(&filter->filter.IBaseFilter_iface);
}

static HRESULT WINAPI FileSource_Load(IFileSourceFilter * iface, LPCOLESTR pszFileName, const AM_MEDIA_TYPE * pmt)
{
    struct async_reader *This = impl_from_IFileSourceFilter(iface);
    HANDLE hFile;

    TRACE("%p->(%s, %p)\n", This, debugstr_w(pszFileName), pmt);
    strmbase_dump_media_type(pmt);

    if (!pszFileName)
        return E_POINTER;

    /* open file */
    /* FIXME: check the sharing values that native uses */
    hFile = CreateFileW(pszFileName, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_DELETE, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL);

    if (hFile == INVALID_HANDLE_VALUE)
    {
        return HRESULT_FROM_WIN32(GetLastError());
    }

    if (!GetFileSizeEx(hFile, &This->file_size))
    {
        WARN("Could not get file size.\n");
        CloseHandle(hFile);
        return HRESULT_FROM_WIN32(GetLastError());
    }

    if (This->pszFileName)
    {
        free(This->pszFileName);
        FreeMediaType(&This->mt);
    }

    if (!(This->pszFileName = wcsdup(pszFileName)))
    {
        CloseHandle(hFile);
        return E_OUTOFMEMORY;
    }

    strmbase_source_init(&This->source, &This->filter, L"Output", &source_ops);
    BaseFilterImpl_IncrementPinVersion(&This->filter);

    This->file = hFile;
    This->flushing = FALSE;
    This->requests = NULL;

    if (!pmt)
    {
        CopyMediaType(&This->mt, &default_mt);
        if (get_media_type(pszFileName, &This->mt.majortype, &This->mt.subtype, NULL))
        {
            TRACE("Found major type %s, subtype %s.\n",
                    debugstr_guid(&This->mt.majortype), debugstr_guid(&This->mt.subtype));
        }
    }
    else
        CopyMediaType(&This->mt, pmt);

    return S_OK;
}

static HRESULT WINAPI FileSource_GetCurFile(IFileSourceFilter *iface, LPOLESTR *ppszFileName, AM_MEDIA_TYPE *mt)
{
    struct async_reader *This = impl_from_IFileSourceFilter(iface);

    TRACE("filter %p, filename %p, mt %p.\n", This, ppszFileName, mt);

    if (!ppszFileName)
        return E_POINTER;

    /* copy file name & media type if available, otherwise clear the outputs */
    if (This->pszFileName)
    {
        *ppszFileName = CoTaskMemAlloc((wcslen(This->pszFileName) + 1) * sizeof(WCHAR));
        wcscpy(*ppszFileName, This->pszFileName);
        if (mt)
            CopyMediaType(mt, &This->mt);
    }
    else
    {
        *ppszFileName = NULL;
        if (mt)
            memset(mt, 0, sizeof(AM_MEDIA_TYPE));
    }

    return S_OK;
}

static const IFileSourceFilterVtbl FileSource_Vtbl = 
{
    FileSource_QueryInterface,
    FileSource_AddRef,
    FileSource_Release,
    FileSource_Load,
    FileSource_GetCurFile
};

static inline struct async_reader *impl_from_strmbase_pin(struct strmbase_pin *iface)
{
    return CONTAINING_RECORD(iface, struct async_reader, source.pin);
}

static inline struct async_reader *impl_from_IAsyncReader(IAsyncReader *iface)
{
    return CONTAINING_RECORD(iface, struct async_reader, IAsyncReader_iface);
}

static HRESULT source_query_accept(struct strmbase_pin *iface, const AM_MEDIA_TYPE *mt)
{
    struct async_reader *filter = impl_from_strmbase_pin(iface);

    if (IsEqualGUID(&mt->majortype, &filter->mt.majortype)
            && (!IsEqualGUID(&mt->subtype, &GUID_NULL)
            || IsEqualGUID(&filter->mt.subtype, &GUID_NULL)))
        return S_OK;

    return S_FALSE;
}

static HRESULT source_get_media_type(struct strmbase_pin *iface, unsigned int index, AM_MEDIA_TYPE *mt)
{
    struct async_reader *filter = impl_from_strmbase_pin(iface);

    if (index > 1)
        return VFW_S_NO_MORE_ITEMS;

    if (index == 0)
        CopyMediaType(mt, &filter->mt);
    else if (index == 1)
        CopyMediaType(mt, &default_mt);
    return S_OK;
}

static HRESULT source_query_interface(struct strmbase_pin *iface, REFIID iid, void **out)
{
    struct async_reader *filter = impl_from_strmbase_pin(iface);

    if (IsEqualGUID(iid, &IID_IAsyncReader))
        *out = &filter->IAsyncReader_iface;
    else
        return E_NOINTERFACE;

    IUnknown_AddRef((IUnknown *)*out);
    return S_OK;
}

/* Function called as a helper to IPin_Connect */
/* specific AM_MEDIA_TYPE - it cannot be NULL */
/* this differs from standard OutputPin_AttemptConnection only in that it
 * doesn't need the IMemInputPin interface on the receiving pin */
static HRESULT WINAPI FileAsyncReaderPin_AttemptConnection(struct strmbase_source *This,
        IPin *pReceivePin, const AM_MEDIA_TYPE *pmt)
{
    HRESULT hr;

    TRACE("%p->(%p, %p)\n", This, pReceivePin, pmt);

    if (This->pin.ops->pin_query_accept(&This->pin, pmt) != S_OK)
        return VFW_E_TYPE_NOT_ACCEPTED;

    This->pin.peer = pReceivePin;
    IPin_AddRef(pReceivePin);
    CopyMediaType(&This->pin.mt, pmt);

    hr = IPin_ReceiveConnection(pReceivePin, &This->pin.IPin_iface, pmt);

    if (FAILED(hr))
    {
        IPin_Release(This->pin.peer);
        This->pin.peer = NULL;
        FreeMediaType(&This->pin.mt);
    }

    return hr;
}

static const struct strmbase_source_ops source_ops =
{
    .base.pin_query_accept = source_query_accept,
    .base.pin_get_media_type = source_get_media_type,
    .base.pin_query_interface = source_query_interface,
    .pfnAttemptConnection = FileAsyncReaderPin_AttemptConnection,
};

static HRESULT WINAPI FileAsyncReader_QueryInterface(IAsyncReader *iface, REFIID iid, void **out)
{
    struct async_reader *filter = impl_from_IAsyncReader(iface);
    return IPin_QueryInterface(&filter->source.pin.IPin_iface, iid, out);
}

static ULONG WINAPI FileAsyncReader_AddRef(IAsyncReader * iface)
{
    struct async_reader *filter = impl_from_IAsyncReader(iface);
    return IPin_AddRef(&filter->source.pin.IPin_iface);
}

static ULONG WINAPI FileAsyncReader_Release(IAsyncReader * iface)
{
    struct async_reader *filter = impl_from_IAsyncReader(iface);
    return IPin_Release(&filter->source.pin.IPin_iface);
}

static HRESULT WINAPI FileAsyncReader_RequestAllocator(IAsyncReader *iface,
        IMemAllocator *preferred, ALLOCATOR_PROPERTIES *props, IMemAllocator **ret_allocator)
{
    struct async_reader *filter = impl_from_IAsyncReader(iface);
    IMemAllocator *allocator;
    unsigned int i;
    HRESULT hr;

    TRACE("filter %p, preferred %p, props %p, ret_allocator %p.\n", filter, preferred, props, ret_allocator);

    if (!props->cbAlign)
        props->cbAlign = 1;

    *ret_allocator = NULL;

    if (preferred)
        IMemAllocator_AddRef(allocator = preferred);
    else if (FAILED(hr = CoCreateInstance(&CLSID_MemoryAllocator, NULL,
            CLSCTX_INPROC, &IID_IMemAllocator, (void **)&allocator)))
        return hr;

    if (FAILED(hr = IMemAllocator_SetProperties(allocator, props, props)))
    {
        IMemAllocator_Release(allocator);
        return hr;
    }

    if (filter->requests)
    {
        for (i = 0; i < filter->max_requests; ++i)
            CloseHandle(filter->requests[i].ovl.hEvent);
        free(filter->requests);
    }

    filter->max_requests = props->cBuffers;
    TRACE("Maximum request count: %u.\n", filter->max_requests);
    if (!(filter->requests = calloc(filter->max_requests, sizeof(filter->requests[0]))))
    {
        IMemAllocator_Release(allocator);
        return E_OUTOFMEMORY;
    }

    for (i = 0; i < filter->max_requests; ++i)
        filter->requests[i].ovl.hEvent = CreateEventW(NULL, TRUE, FALSE, NULL);

    *ret_allocator = allocator;
    return S_OK;
}

static HRESULT WINAPI FileAsyncReader_Request(IAsyncReader *iface, IMediaSample *sample, DWORD_PTR cookie)
{
    struct async_reader *filter = impl_from_IAsyncReader(iface);
    REFERENCE_TIME start, end;
    struct request *req;
    unsigned int i;
    HRESULT hr;
    BYTE *data;

    TRACE("filter %p, sample %p, cookie %#Ix.\n", filter, sample, cookie);

    if (!sample)
        return E_POINTER;

    if (FAILED(hr = IMediaSample_GetTime(sample, &start, &end)))
        return hr;

    if (BYTES_FROM_MEDIATIME(start) >= filter->file_size.QuadPart)
        return HRESULT_FROM_WIN32(ERROR_HANDLE_EOF);

    if (FAILED(hr = IMediaSample_GetPointer(sample, &data)))
        return hr;

    EnterCriticalSection(&filter->sample_cs);
    if (filter->flushing)
    {
        LeaveCriticalSection(&filter->sample_cs);
        return VFW_E_WRONG_STATE;
    }

    for (i = 0; i < filter->max_requests; ++i)
    {
        if (!filter->requests[i].sample)
            break;
    }
    assert(i < filter->max_requests);
    req = &filter->requests[i];

    req->ovl.Offset = BYTES_FROM_MEDIATIME(start);
    req->ovl.OffsetHigh = BYTES_FROM_MEDIATIME(start) >> 32;
    /* No reference is taken. */

    if (ReadFile(filter->file, data, BYTES_FROM_MEDIATIME(end - start), NULL, &req->ovl)
            || GetLastError() == ERROR_IO_PENDING)
    {
        hr = S_OK;
        req->sample = sample;
        req->cookie = cookie;
    }
    else
        hr = HRESULT_FROM_WIN32(GetLastError());

    LeaveCriticalSection(&filter->sample_cs);
    return hr;
}

static HRESULT WINAPI FileAsyncReader_WaitForNext(IAsyncReader *iface,
        DWORD timeout, IMediaSample **sample, DWORD_PTR *cookie)
{
    struct async_reader *filter = impl_from_IAsyncReader(iface);
    unsigned int i;

    TRACE("filter %p, timeout %lu, sample %p, cookie %p.\n", filter, timeout, sample, cookie);

    *sample = NULL;
    *cookie = 0;

    EnterCriticalSection(&filter->sample_cs);

    do
    {
        if (filter->flushing)
        {
            LeaveCriticalSection(&filter->sample_cs);
            return VFW_E_WRONG_STATE;
        }

        for (i = 0; i < filter->max_requests; ++i)
        {
            struct request *req = &filter->requests[i];
            DWORD size;

            if (req->sample && GetOverlappedResult(filter->file, &req->ovl, &size, FALSE))
            {
                REFERENCE_TIME start, end;

                IMediaSample_SetActualDataLength(req->sample, size);
                start = MEDIATIME_FROM_BYTES(((ULONGLONG)req->ovl.OffsetHigh << 32) + req->ovl.Offset);
                end = start + MEDIATIME_FROM_BYTES(size);
                IMediaSample_SetTime(req->sample, &start, &end);

                *sample = req->sample;
                *cookie = req->cookie;
                req->sample = NULL;

                LeaveCriticalSection(&filter->sample_cs);
                TRACE("Returning sample %u.\n", i);
                return S_OK;
            }
        }
    } while (SleepConditionVariableCS(&filter->sample_cv, &filter->sample_cs, timeout));

    LeaveCriticalSection(&filter->sample_cs);
    return VFW_E_TIMEOUT;
}

static BOOL sync_read(HANDLE file, LONGLONG offset, LONG length, BYTE *buffer, DWORD *read_len)
{
    OVERLAPPED ovl = {0};
    BOOL ret;

    ovl.hEvent = (HANDLE)((ULONG_PTR)CreateEventW(NULL, TRUE, FALSE, NULL) | 1);
    ovl.Offset = (DWORD)offset;
    ovl.OffsetHigh = offset >> 32;

    *read_len = 0;

    ret = ReadFile(file, buffer, length, NULL, &ovl);
    if (ret || GetLastError() == ERROR_IO_PENDING)
        ret = GetOverlappedResult(file, &ovl, read_len, TRUE);

    TRACE("Returning %lu bytes.\n", *read_len);

    CloseHandle(ovl.hEvent);
    return ret;
}

static HRESULT WINAPI FileAsyncReader_SyncReadAligned(IAsyncReader *iface, IMediaSample *sample)
{
    struct async_reader *filter = impl_from_IAsyncReader(iface);
    REFERENCE_TIME start_time, end_time;
    DWORD read_len;
    BYTE *buffer;
    LONG length;
    HRESULT hr;
    BOOL ret;

    TRACE("filter %p, sample %p.\n", filter, sample);

    hr = IMediaSample_GetTime(sample, &start_time, &end_time);

    if (SUCCEEDED(hr))
        hr = IMediaSample_GetPointer(sample, &buffer);

    if (SUCCEEDED(hr))
    {
        length = BYTES_FROM_MEDIATIME(end_time - start_time);
        ret = sync_read(filter->file, BYTES_FROM_MEDIATIME(start_time), length, buffer, &read_len);
        if (ret)
            hr = (read_len == length) ? S_OK : S_FALSE;
        else if (GetLastError() == ERROR_HANDLE_EOF)
            hr = S_OK;
        else
            hr = HRESULT_FROM_WIN32(GetLastError());
    }

    if (SUCCEEDED(hr))
        IMediaSample_SetActualDataLength(sample, read_len);

    return hr;
}

static HRESULT WINAPI FileAsyncReader_SyncRead(IAsyncReader *iface,
        LONGLONG offset, LONG length, BYTE *buffer)
{
    struct async_reader *filter = impl_from_IAsyncReader(iface);
    DWORD read_len;
    HRESULT hr;
    BOOL ret;

    TRACE("filter %p, offset %s, length %ld, buffer %p.\n",
            filter, wine_dbgstr_longlong(offset), length, buffer);

    ret = sync_read(filter->file, offset, length, buffer, &read_len);
    if (ret)
        hr = (read_len == length) ? S_OK : S_FALSE;
    else if (GetLastError() == ERROR_HANDLE_EOF)
        hr = S_FALSE;
    else
        hr = HRESULT_FROM_WIN32(GetLastError());

    return hr;
}

static HRESULT WINAPI FileAsyncReader_Length(IAsyncReader *iface, LONGLONG *total, LONGLONG *available)
{
    struct async_reader *filter = impl_from_IAsyncReader(iface);

    TRACE("iface %p, total %p, available %p.\n", iface, total, available);

    *available = *total = filter->file_size.QuadPart;

    return S_OK;
}

static HRESULT WINAPI FileAsyncReader_BeginFlush(IAsyncReader * iface)
{
    struct async_reader *filter = impl_from_IAsyncReader(iface);
    unsigned int i;

    TRACE("iface %p.\n", iface);

    EnterCriticalSection(&filter->sample_cs);

    filter->flushing = TRUE;
    for (i = 0; i < filter->max_requests; ++i)
        filter->requests[i].sample = NULL;
    CancelIoEx(filter->file, NULL);
    WakeAllConditionVariable(&filter->sample_cv);

    LeaveCriticalSection(&filter->sample_cs);

    return S_OK;
}

static HRESULT WINAPI FileAsyncReader_EndFlush(IAsyncReader * iface)
{
    struct async_reader *filter = impl_from_IAsyncReader(iface);

    TRACE("iface %p.\n", iface);

    EnterCriticalSection(&filter->sample_cs);

    filter->flushing = FALSE;

    LeaveCriticalSection(&filter->sample_cs);

    return S_OK;
}

static const IAsyncReaderVtbl FileAsyncReader_Vtbl = 
{
    FileAsyncReader_QueryInterface,
    FileAsyncReader_AddRef,
    FileAsyncReader_Release,
    FileAsyncReader_RequestAllocator,
    FileAsyncReader_Request,
    FileAsyncReader_WaitForNext,
    FileAsyncReader_SyncReadAligned,
    FileAsyncReader_SyncRead,
    FileAsyncReader_Length,
    FileAsyncReader_BeginFlush,
    FileAsyncReader_EndFlush,
};
