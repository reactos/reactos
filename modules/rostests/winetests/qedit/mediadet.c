/*
 * Unit tests for Media Detector
 *
 * Copyright (C) 2008 Google (Lei Zhang, Dan Hipschman)
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
#define CONST_VTABLE

#include "ole2.h"
#include "vfwmsgs.h"
#include "uuids.h"
#include "wine/strmbase.h"
#include "wine/test.h"
#include "qedit.h"
#include "control.h"
#include "rc.h"

static ULONG get_refcount(void *iface)
{
    IUnknown *unknown = iface;
    IUnknown_AddRef(unknown);
    return IUnknown_Release(unknown);
}

static const GUID test_iid = {0x33333333};
static LONG outer_ref = 1;

static HRESULT WINAPI outer_QueryInterface(IUnknown *iface, REFIID iid, void **out)
{
    if (IsEqualGUID(iid, &IID_IUnknown)
            || IsEqualGUID(iid, &IID_IMediaDet)
            || IsEqualGUID(iid, &test_iid))
    {
        *out = (IUnknown *)0xdeadbeef;
        return S_OK;
    }
    ok(0, "unexpected call %s\n", wine_dbgstr_guid(iid));
    return E_NOINTERFACE;
}

static ULONG WINAPI outer_AddRef(IUnknown *iface)
{
    return InterlockedIncrement(&outer_ref);
}

static ULONG WINAPI outer_Release(IUnknown *iface)
{
    return InterlockedDecrement(&outer_ref);
}

static const IUnknownVtbl outer_vtbl =
{
    outer_QueryInterface,
    outer_AddRef,
    outer_Release,
};

static IUnknown test_outer = {&outer_vtbl};

static void test_aggregation(void)
{
    IMediaDet *detector, *detector2;
    IUnknown *unk, *unk2;
    HRESULT hr;
    ULONG ref;

    detector = (IMediaDet *)0xdeadbeef;
    hr = CoCreateInstance(&CLSID_MediaDet, &test_outer, CLSCTX_INPROC_SERVER,
            &IID_IMediaDet, (void **)&detector);
    ok(hr == E_NOINTERFACE, "Got hr %#lx.\n", hr);
    ok(!detector, "Got interface %p.\n", detector);

    hr = CoCreateInstance(&CLSID_MediaDet, &test_outer, CLSCTX_INPROC_SERVER,
            &IID_IUnknown, (void **)&unk);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(outer_ref == 1, "Got unexpected refcount %ld.\n", outer_ref);
    ok(unk != &test_outer, "Returned IUnknown should not be outer IUnknown.\n");
    ref = get_refcount(unk);
    ok(ref == 1, "Got unexpected refcount %ld.\n", ref);

    ref = IUnknown_AddRef(unk);
    ok(ref == 2, "Got unexpected refcount %ld.\n", ref);
    ok(outer_ref == 1, "Got unexpected refcount %ld.\n", outer_ref);

    ref = IUnknown_Release(unk);
    ok(ref == 1, "Got unexpected refcount %ld.\n", ref);
    ok(outer_ref == 1, "Got unexpected refcount %ld.\n", outer_ref);

    hr = IUnknown_QueryInterface(unk, &IID_IUnknown, (void **)&unk2);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(unk2 == unk, "Got unexpected IUnknown %p.\n", unk2);
    IUnknown_Release(unk2);

    hr = IUnknown_QueryInterface(unk, &IID_IMediaDet, (void **)&detector);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IMediaDet_QueryInterface(detector, &IID_IUnknown, (void **)&unk2);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(unk2 == (IUnknown *)0xdeadbeef, "Got unexpected IUnknown %p.\n", unk2);

    hr = IMediaDet_QueryInterface(detector, &IID_IMediaDet, (void **)&detector2);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(detector2 == (IMediaDet *)0xdeadbeef, "Got unexpected IMediaDet %p.\n", detector2);

    hr = IUnknown_QueryInterface(unk, &test_iid, (void **)&unk2);
    ok(hr == E_NOINTERFACE, "Got hr %#lx.\n", hr);
    ok(!unk2, "Got unexpected IUnknown %p.\n", unk2);

    hr = IMediaDet_QueryInterface(detector, &test_iid, (void **)&unk2);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(unk2 == (IUnknown *)0xdeadbeef, "Got unexpected IUnknown %p.\n", unk2);

    IMediaDet_Release(detector);
    ref = IUnknown_Release(unk);
    ok(!ref, "Got unexpected refcount %ld.\n", ref);
    ok(outer_ref == 1, "Got unexpected refcount %ld.\n", outer_ref);
}

struct testfilter
{
    struct strmbase_filter filter;
    struct strmbase_source source;
    IMediaSeeking IMediaSeeking_iface;
};

static inline struct testfilter *impl_from_strmbase_filter(struct strmbase_filter *iface)
{
    return CONTAINING_RECORD(iface, struct testfilter, filter);
}

static struct strmbase_pin *testfilter_get_pin(struct strmbase_filter *iface, unsigned int index)
{
    struct testfilter *filter = impl_from_strmbase_filter(iface);

    return index ? NULL : &filter->source.pin;
}

static void testfilter_destroy(struct strmbase_filter *iface)
{
    struct testfilter *filter = impl_from_strmbase_filter(iface);

    strmbase_source_cleanup(&filter->source);
    strmbase_filter_cleanup(&filter->filter);
}

static const struct strmbase_filter_ops testfilter_ops =
{
    .filter_get_pin = testfilter_get_pin,
    .filter_destroy = testfilter_destroy,
};

static inline struct testfilter *impl_from_strmbase_pin(struct strmbase_pin *iface)
{
    return CONTAINING_RECORD(iface, struct testfilter, source.pin);
}

static HRESULT testsource_get_media_type(struct strmbase_pin *iface, unsigned int index, AM_MEDIA_TYPE *mt)
{
    static const VIDEOINFOHEADER source_format =
    {
        .bmiHeader.biSize = sizeof(BITMAPINFOHEADER),
        .bmiHeader.biWidth = 640,
        .bmiHeader.biHeight = 480,
        .bmiHeader.biPlanes = 1,
        .bmiHeader.biBitCount = 24,
        .bmiHeader.biCompression = BI_RGB,
        .bmiHeader.biSizeImage = 640 * 480 * 3
    };

    if (index)
        return S_FALSE;

    mt->majortype = MEDIATYPE_Video;
    mt->subtype = MEDIASUBTYPE_RGB24;
    mt->bFixedSizeSamples = TRUE;
    mt->bTemporalCompression = FALSE;
    mt->lSampleSize = source_format.bmiHeader.biSizeImage;
    mt->formattype = FORMAT_VideoInfo;
    mt->pUnk = NULL;
    mt->cbFormat = sizeof(source_format);
    mt->pbFormat = CoTaskMemAlloc(mt->cbFormat);
    memcpy(mt->pbFormat, &source_format, mt->cbFormat);
    return S_OK;
}

static HRESULT testsource_query_interface(struct strmbase_pin *iface, REFIID iid, void **out)
{
    struct testfilter *filter = impl_from_strmbase_pin(iface);

    if (IsEqualGUID(iid, &IID_IMediaSeeking))
        *out = &filter->IMediaSeeking_iface;
    else
        return E_NOINTERFACE;

    IUnknown_AddRef((IUnknown*)*out);
    return S_OK;
}

static HRESULT WINAPI testsource_DecideAllocator(struct strmbase_source *iface,
        IMemInputPin *peer, IMemAllocator **allocator)
{
    return S_OK;
}

static const struct strmbase_source_ops testsource_ops =
{
    .base.pin_get_media_type = testsource_get_media_type,
    .base.pin_query_interface = testsource_query_interface,
    .pfnAttemptConnection = BaseOutputPinImpl_AttemptConnection,
    .pfnDecideAllocator = testsource_DecideAllocator,
};

static inline struct testfilter *impl_from_IMediaSeeking(IMediaSeeking *iface)
{
    return CONTAINING_RECORD(iface, struct testfilter, IMediaSeeking_iface);
}

static HRESULT WINAPI testseek_QueryInterface(IMediaSeeking *iface, REFIID iid, void **out)
{
    struct testfilter *filter = impl_from_IMediaSeeking(iface);
    return IPin_QueryInterface(&filter->source.pin.IPin_iface, iid, out);
}

static ULONG WINAPI testseek_AddRef(IMediaSeeking *iface)
{
    struct testfilter *filter = impl_from_IMediaSeeking(iface);
    return IPin_AddRef(&filter->source.pin.IPin_iface);
}

static ULONG WINAPI testseek_Release(IMediaSeeking *iface)
{
    struct testfilter *filter = impl_from_IMediaSeeking(iface);
    return IPin_Release(&filter->source.pin.IPin_iface);
}

static HRESULT WINAPI testseek_GetCapabilities(IMediaSeeking *iface, DWORD *caps)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI testseek_CheckCapabilities(IMediaSeeking *iface, DWORD *caps)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI testseek_IsFormatSupported(IMediaSeeking *iface, const GUID *format)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI testseek_QueryPreferredFormat(IMediaSeeking *iface, GUID *format)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI testseek_GetTimeFormat(IMediaSeeking *iface, GUID *format)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI testseek_IsUsingTimeFormat(IMediaSeeking *iface, const GUID *format)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI testseek_SetTimeFormat(IMediaSeeking *iface, const GUID *format)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI testseek_GetDuration(IMediaSeeking *iface, LONGLONG *duration)
{
    if (winetest_debug > 1) trace("IMediaSeeking_GetDuration()\n");

    *duration = 42000000;
    return S_OK;
}

static HRESULT WINAPI testseek_GetStopPosition(IMediaSeeking *iface, LONGLONG *stop)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI testseek_GetCurrentPosition(IMediaSeeking *iface, LONGLONG *current)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI testseek_ConvertTimeFormat(IMediaSeeking *iface, LONGLONG *target,
    const GUID *target_format, LONGLONG source, const GUID *source_format)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI testseek_SetPositions(IMediaSeeking *iface, LONGLONG *current,
    DWORD current_flags, LONGLONG *stop, DWORD stop_flags)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI testseek_GetPositions(IMediaSeeking *iface, LONGLONG *current, LONGLONG *stop)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI testseek_GetAvailable(IMediaSeeking *iface, LONGLONG *earliest, LONGLONG *latest)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI testseek_SetRate(IMediaSeeking *iface, double rate)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI testseek_GetRate(IMediaSeeking *iface, double *rate)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI testseek_GetPreroll(IMediaSeeking *iface, LONGLONG *preroll)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static const IMediaSeekingVtbl testseek_vtbl =
{
    testseek_QueryInterface,
    testseek_AddRef,
    testseek_Release,
    testseek_GetCapabilities,
    testseek_CheckCapabilities,
    testseek_IsFormatSupported,
    testseek_QueryPreferredFormat,
    testseek_GetTimeFormat,
    testseek_IsUsingTimeFormat,
    testseek_SetTimeFormat,
    testseek_GetDuration,
    testseek_GetStopPosition,
    testseek_GetCurrentPosition,
    testseek_ConvertTimeFormat,
    testseek_SetPositions,
    testseek_GetPositions,
    testseek_GetAvailable,
    testseek_SetRate,
    testseek_GetRate,
    testseek_GetPreroll
};

static void testfilter_init(struct testfilter *filter)
{
    static const GUID clsid = {0xabacab};

    memset(filter, 0, sizeof(*filter));
    strmbase_filter_init(&filter->filter, NULL, &clsid, &testfilter_ops);
    strmbase_source_init(&filter->source, &filter->filter, L"", &testsource_ops);
    filter->IMediaSeeking_iface.lpVtbl = &testseek_vtbl;
}

static WCHAR test_avi_filename[MAX_PATH];
static WCHAR test_sound_avi_filename[MAX_PATH];

static BOOL unpack_avi_file(int id, WCHAR name[MAX_PATH])
{
    static WCHAR temp_path[MAX_PATH];
    HRSRC res;
    HGLOBAL data;
    char *mem;
    DWORD size, written;
    HANDLE fh;
    BOOL ret;

    res = FindResourceW(NULL, MAKEINTRESOURCEW(id), MAKEINTRESOURCEW(AVI_RES_TYPE));
    if (!res)
        return FALSE;

    data = LoadResource(NULL, res);
    if (!data)
        return FALSE;

    mem = LockResource(data);
    if (!mem)
        return FALSE;

    size = SizeofResource(NULL, res);
    if (size == 0)
        return FALSE;

    if (!GetTempPathW(MAX_PATH, temp_path))
        return FALSE;

    /* We might end up relying on the extension here, so .TMP is no good.  */
    if (!GetTempFileNameW(temp_path, L"DES", 0, name))
        return FALSE;

    DeleteFileW(name);
    wcscpy(name + wcslen(name) - 3, L"avi");

    fh = CreateFileW(name, GENERIC_WRITE, 0, NULL, CREATE_NEW,
                     FILE_ATTRIBUTE_NORMAL, NULL);
    if (fh == INVALID_HANDLE_VALUE)
        return FALSE;

    ret = WriteFile(fh, mem, size, &written, NULL);
    CloseHandle(fh);
    return ret && written == size;
}

static BOOL init_tests(void)
{
    return unpack_avi_file(TEST_AVI_RES, test_avi_filename)
        && unpack_avi_file(TEST_SOUND_AVI_RES, test_sound_avi_filename);
}

static void test_mediadet(void)
{
    HRESULT hr;
    FILTER_INFO filter_info;
    AM_MEDIA_TYPE mt, *pmt;
    LONG index, ref, count;
    IEnumMediaTypes *type;
    IMediaDet *pM = NULL;
    BSTR filename = NULL;
    IBaseFilter *filter;
    IEnumPins *enumpins;
    IUnknown *unk;
    IPin *pin;
    GUID guid;
    BSTR bstr;
    double fps;
    int flags;
    int i;

    /* test.avi has one video stream.  */
    hr = CoCreateInstance(&CLSID_MediaDet, NULL, CLSCTX_INPROC_SERVER,
            &IID_IMediaDet, (LPVOID*)&pM);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(pM != NULL, "pM is NULL\n");

    filename = NULL;
    hr = IMediaDet_get_Filename(pM, &filename);
    /* Despite what MSDN claims, this returns S_OK.  */
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(filename == NULL, "IMediaDet_get_Filename\n");

    filename = (BSTR) -1;
    hr = IMediaDet_get_Filename(pM, &filename);
    /* Despite what MSDN claims, this returns S_OK.  */
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(filename == NULL, "IMediaDet_get_Filename\n");

    count = -1;
    hr = IMediaDet_get_OutputStreams(pM, &count);
    ok(hr == E_INVALIDARG, "Got hr %#lx.\n", hr);
    ok(count == -1, "Got %ld streams.\n", count);

    index = -1;
    /* The stream defaults to 0, even without a file!  */
    hr = IMediaDet_get_CurrentStream(pM, &index);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(index == 0, "Got stream index %ld.\n", index);

    hr = IMediaDet_get_CurrentStream(pM, NULL);
    ok(hr == E_POINTER, "Got hr %#lx.\n", hr);

    /* But put_CurrentStream doesn't.  */
    hr = IMediaDet_put_CurrentStream(pM, 0);
    ok(hr == E_INVALIDARG, "Got hr %#lx.\n", hr);

    hr = IMediaDet_put_CurrentStream(pM, -1);
    ok(hr == E_INVALIDARG, "Got hr %#lx.\n", hr);

    hr = IMediaDet_get_StreamMediaType(pM, &mt);
    ok(hr == E_INVALIDARG, "Got hr %#lx.\n", hr);

    hr = IMediaDet_get_StreamMediaType(pM, NULL);
    ok(hr == E_POINTER, "Got hr %#lx.\n", hr);

    hr = IMediaDet_get_StreamType(pM, &guid);
    ok(hr == E_INVALIDARG, "Got hr %#lx.\n", hr);

    hr = IMediaDet_get_StreamType(pM, NULL);
    ok(hr == E_POINTER, "Got hr %#lx.\n", hr);

    hr = IMediaDet_get_StreamTypeB(pM, &bstr);
    ok(hr == E_INVALIDARG, "Got hr %#lx.\n", hr);

    hr = IMediaDet_get_StreamTypeB(pM, NULL);
    ok(hr == E_INVALIDARG, "Got hr %#lx.\n", hr);

    hr = IMediaDet_get_Filter(pM, NULL);
    ok(hr == E_POINTER, "Got hr %#lx.\n", hr);

    unk = (IUnknown*)0xdeadbeef;
    hr = IMediaDet_get_Filter(pM, &unk);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);
    ok(!unk, "Got filter %p.\n", unk);

    filename = SysAllocString(test_avi_filename);
    hr = IMediaDet_put_Filename(pM, filename);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    SysFreeString(filename);

    index = -1;
    /* The stream defaults to 0.  */
    hr = IMediaDet_get_CurrentStream(pM, &index);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(index == 0, "Got stream index %ld.\n", index);

    ZeroMemory(&mt, sizeof mt);
    hr = IMediaDet_get_StreamMediaType(pM, &mt);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    CoTaskMemFree(mt.pbFormat);

    hr = IMediaDet_get_StreamType(pM, &guid);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(IsEqualGUID(&guid, &MEDIATYPE_Video), "Got major type %s.\n", debugstr_guid(&guid));

    hr = IMediaDet_get_StreamTypeB(pM, &bstr);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(!wcscmp(bstr, L"{73646976-0000-0010-8000-00AA00389B71}"),
            "Got major type %s.\n", debugstr_w(bstr));
    SysFreeString(bstr);

    /* Even before get_OutputStreams.  */
    hr = IMediaDet_put_CurrentStream(pM, 1);
    ok(hr == E_INVALIDARG, "Got hr %#lx.\n", hr);

    hr = IMediaDet_get_OutputStreams(pM, &count);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(count == 1, "Got %ld streams.\n", count);

    filename = NULL;
    hr = IMediaDet_get_Filename(pM, &filename);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(!wcscmp(filename, test_avi_filename), "Expected filename %s, got %s.\n",
            debugstr_w(test_avi_filename), debugstr_w(filename));
    SysFreeString(filename);

    hr = IMediaDet_get_Filename(pM, NULL);
    ok(hr == E_POINTER, "Got hr %#lx.\n", hr);

    index = -1;
    hr = IMediaDet_get_CurrentStream(pM, &index);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(index == 0, "Got stream index %ld.\n", index);

    hr = IMediaDet_get_CurrentStream(pM, NULL);
    ok(hr == E_POINTER, "Got hr %#lx.\n", hr);

    hr = IMediaDet_put_CurrentStream(pM, -1);
    ok(hr == E_INVALIDARG, "Got hr %#lx.\n", hr);

    hr = IMediaDet_put_CurrentStream(pM, 1);
    ok(hr == E_INVALIDARG, "Got hr %#lx.\n", hr);

    /* Try again.  */
    index = -1;
    hr = IMediaDet_get_CurrentStream(pM, &index);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(index == 0, "Got stream index %ld.\n", index);

    hr = IMediaDet_put_CurrentStream(pM, 0);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    index = -1;
    hr = IMediaDet_get_CurrentStream(pM, &index);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(index == 0, "Got stream index %ld.\n", index);

    ZeroMemory(&mt, sizeof mt);
    hr = IMediaDet_get_StreamMediaType(pM, &mt);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(IsEqualGUID(&mt.majortype, &MEDIATYPE_Video),
                 "IMediaDet_get_StreamMediaType\n");
    CoTaskMemFree(mt.pbFormat);

    hr = IMediaDet_get_StreamType(pM, &guid);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(IsEqualGUID(&guid, &MEDIATYPE_Video), "Got major type %s.\n", debugstr_guid(&guid));

    hr = IMediaDet_get_StreamTypeB(pM, &bstr);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(!wcscmp(bstr, L"{73646976-0000-0010-8000-00AA00389B71}"),
            "Got major type %s.\n", debugstr_w(bstr));
    SysFreeString(bstr);

    hr = IMediaDet_get_FrameRate(pM, NULL);
    ok(hr == E_POINTER, "Got hr %#lx.\n", hr);

    hr = IMediaDet_get_FrameRate(pM, &fps);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(fps == 10.0, "IMediaDet_get_FrameRate: fps is %f\n", fps);

    ref = IMediaDet_Release(pM);
    ok(!ref, "Got outstanding refcount %ld.\n", ref);

    /* test_sound.avi has one video stream and one audio stream.  */
    hr = CoCreateInstance(&CLSID_MediaDet, NULL, CLSCTX_INPROC_SERVER,
            &IID_IMediaDet, (LPVOID*)&pM);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(pM != NULL, "pM is NULL\n");

    filename = SysAllocString(test_sound_avi_filename);
    hr = IMediaDet_put_Filename(pM, filename);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    SysFreeString(filename);

    hr = IMediaDet_get_OutputStreams(pM, &count);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(count == 2, "Got %ld streams.\n", count);

    filename = NULL;
    hr = IMediaDet_get_Filename(pM, &filename);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(!wcscmp(filename, test_sound_avi_filename), "Expected filename %s, got %s.\n",
            debugstr_w(test_sound_avi_filename), debugstr_w(filename));
    SysFreeString(filename);

    /* I don't know if the stream order is deterministic.  Just check
       for both an audio and video stream.  */
    flags = 0;

    for (i = 0; i < 2; ++i)
    {
        hr = IMediaDet_put_CurrentStream(pM, i);
        ok(hr == S_OK, "Got hr %#lx.\n", hr);

        index = -1;
        hr = IMediaDet_get_CurrentStream(pM, &index);
        ok(hr == S_OK, "Got hr %#lx.\n", hr);
        ok(index == i, "Got stream index %ld.\n", index);

        ZeroMemory(&mt, sizeof mt);
        hr = IMediaDet_get_StreamMediaType(pM, &mt);
        ok(hr == S_OK, "Got hr %#lx.\n", hr);
        flags += (IsEqualGUID(&mt.majortype, &MEDIATYPE_Video)
                  ? 1
                  : (IsEqualGUID(&mt.majortype, &MEDIATYPE_Audio)
                     ? 2
                     : 0));

        if (IsEqualGUID(&mt.majortype, &MEDIATYPE_Audio))
        {
            hr = IMediaDet_get_StreamType(pM, &guid);
            ok(hr == S_OK, "Got hr %#lx.\n", hr);
            ok(IsEqualGUID(&guid, &MEDIATYPE_Audio), "Got major type %s.\n", debugstr_guid(&guid));

            hr = IMediaDet_get_StreamTypeB(pM, &bstr);
            ok(hr == S_OK, "Got hr %#lx.\n", hr);
            ok(!wcscmp(bstr, L"{73647561-0000-0010-8000-00AA00389B71}"),
                    "Got major type %s.\n", debugstr_w(bstr));
            SysFreeString(bstr);

            hr = IMediaDet_get_FrameRate(pM, &fps);
            ok(hr == VFW_E_INVALIDMEDIATYPE, "Got hr %#lx.\n", hr);
        }

        CoTaskMemFree(mt.pbFormat);
    }
    ok(flags == 3, "IMediaDet_get_StreamMediaType: flags are %i\n", flags);

    hr = IMediaDet_put_CurrentStream(pM, 2);
    ok(hr == E_INVALIDARG, "Got hr %#lx.\n", hr);

    index = -1;
    hr = IMediaDet_get_CurrentStream(pM, &index);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(index == 1, "Got stream index %ld.\n", index);

    unk = NULL;
    hr = IMediaDet_get_Filter(pM, &unk);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(!!unk, "Expected a non-NULL filter.\n");
    hr = IUnknown_QueryInterface(unk, &IID_IBaseFilter, (void**)&filter);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    IUnknown_Release(unk);

    hr = IBaseFilter_EnumPins(filter, &enumpins);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IEnumPins_Next(enumpins, 1, &pin, NULL);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IPin_EnumMediaTypes(pin, &type);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IEnumMediaTypes_Next(type, 1, &pmt, NULL);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(IsEqualGUID(&pmt->majortype, &MEDIATYPE_Stream), "Got major type %s.\n",
            debugstr_guid(&pmt->majortype));
    IEnumMediaTypes_Release(type);
    CoTaskMemFree(pmt->pbFormat);
    CoTaskMemFree(pmt);
    IPin_Release(pin);

    hr = IEnumPins_Next(enumpins, 1, &pin, NULL);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);
    IEnumPins_Release(enumpins);

    hr = IBaseFilter_QueryFilterInfo(filter, &filter_info);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(!wcscmp(filter_info.achName, L"Source"), "Got name %s.\n", debugstr_w(filter_info.achName));
    IFilterGraph_Release(filter_info.pGraph);
    IBaseFilter_Release(filter);

    ref = IMediaDet_Release(pM);
    ok(!ref, "Got outstanding refcount %ld.\n", ref);
}

static void test_put_filter(void)
{
    struct testfilter testfilter, testfilter2;
    IFilterGraph *graph;
    IBaseFilter *filter;
    IMediaDet *detector;
    LONG index, count;
    AM_MEDIA_TYPE mt;
    double duration;
    IUnknown *unk;
    BSTR filename;
    HRESULT hr;
    ULONG ref;

    hr = CoCreateInstance(&CLSID_MediaDet, NULL, CLSCTX_INPROC_SERVER,
            &IID_IMediaDet, (void **)&detector);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IMediaDet_put_Filter(detector, NULL);
    ok(hr == E_POINTER, "Got hr %#lx.\n", hr);

    hr = IMediaDet_get_Filter(detector, NULL);
    ok(hr == E_POINTER, "Got hr %#lx.\n", hr);

    hr = IMediaDet_get_StreamLength(detector, NULL);
    ok(hr == E_POINTER, "Got hr %#lx.\n", hr);

    hr = IMediaDet_get_StreamLength(detector, &duration);
    ok(hr == E_INVALIDARG, "Got hr %#lx.\n", hr);

    testfilter_init(&testfilter);
    hr = IMediaDet_put_Filter(detector, &testfilter.filter.IUnknown_inner);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IMediaDet_get_Filter(detector, &unk);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(!!unk, "Expected a non-NULL interface.\n");
    hr = IUnknown_QueryInterface(unk, &IID_IBaseFilter, (void **)&filter);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(filter == &testfilter.filter.IBaseFilter_iface, "Expected the same filter.\n");
    IBaseFilter_Release(filter);
    IUnknown_Release(unk);

    ok(!wcscmp(testfilter.filter.name, L"Source"), "Got name %s.\n",
            debugstr_w(testfilter.filter.name));
    graph = testfilter.filter.graph;
    IFilterGraph_AddRef(graph);

    testfilter_init(&testfilter2);
    hr = IMediaDet_put_Filter(detector, &testfilter2.filter.IUnknown_inner);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IMediaDet_get_Filter(detector, &unk);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(!!unk, "Expected a non-NULL interface.\n");
    hr = IUnknown_QueryInterface(unk, &IID_IBaseFilter, (void **)&filter);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(filter == &testfilter2.filter.IBaseFilter_iface, "Expected the same filter.\n");
    IBaseFilter_Release(filter);
    IUnknown_Release(unk);

    ok(testfilter2.filter.graph != graph, "Expected a different graph.\n");

    ref = IFilterGraph_Release(graph);
    ok(!ref, "Got outstanding refcount %ld.\n", ref);
    ref = IBaseFilter_Release(&testfilter.filter.IBaseFilter_iface);
    ok(!ref, "Got outstanding refcount %ld.\n", ref);

    count = 0xdeadbeef;
    hr = IMediaDet_get_OutputStreams(detector, &count);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(count == 1, "Got %ld streams.\n", count);

    index = 0xdeadbeef;
    hr = IMediaDet_get_CurrentStream(detector, &index);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(index == 0, "Got stream %ld.\n", index);

    filename = (BSTR)0xdeadbeef;
    hr = IMediaDet_get_Filename(detector, &filename);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(!filename, "Got filename %s.\n", debugstr_w(filename));

    hr = IMediaDet_get_StreamLength(detector, &duration);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(duration == 4.2, "Got duration %.16e.\n", duration);

    ref = IMediaDet_Release(detector);
    ok(!ref, "Got outstanding refcount %ld.\n", ref);
    ref = IBaseFilter_Release(&testfilter2.filter.IBaseFilter_iface);
    ok(!ref, "Got outstanding refcount %ld.\n", ref);

    hr = CoCreateInstance(&CLSID_MediaDet, NULL, CLSCTX_INPROC_SERVER,
            &IID_IMediaDet, (void **)&detector);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    filename = SysAllocString(test_sound_avi_filename);
    hr = IMediaDet_put_Filename(detector, filename);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    SysFreeString(filename);

    hr = IMediaDet_get_StreamMediaType(detector, &mt);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    FreeMediaType(&mt);

    hr = IMediaDet_get_Filter(detector, &unk);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IMediaDet_put_Filter(detector, unk);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    IUnknown_Release(unk);

    filename = (BSTR)0xdeadbeef;
    hr = IMediaDet_get_Filename(detector, &filename);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(!filename, "Got filename %s.\n", debugstr_w(filename));

    count = 0xdeadbeef;
    hr = IMediaDet_get_OutputStreams(detector, &count);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(count == 2, "Got %ld streams.\n", count);

    index = 0xdeadbeef;
    hr = IMediaDet_get_CurrentStream(detector, &index);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(index == 0, "Got stream %ld.\n", index);

    ref = IMediaDet_Release(detector);
    ok(!ref, "Got outstanding refcount %ld.\n", ref);
}

struct test_sample
{
    IMediaSample sample;
    LONG refcount;
};

static struct test_sample *impl_from_IMediaSample(IMediaSample *iface)
{
    return CONTAINING_RECORD(iface, struct test_sample, sample);
}

static HRESULT WINAPI ms_QueryInterface(IMediaSample *iface, REFIID riid,
        void **ppvObject)
{
    return E_NOTIMPL;
}

static ULONG WINAPI ms_AddRef(IMediaSample *iface)
{
    struct test_sample *sample = impl_from_IMediaSample(iface);
    return InterlockedIncrement(&sample->refcount);
}

static ULONG WINAPI ms_Release(IMediaSample *iface)
{
    struct test_sample *sample = impl_from_IMediaSample(iface);
    return InterlockedDecrement(&sample->refcount);
}

static HRESULT WINAPI ms_GetPointer(IMediaSample *iface, BYTE **ppBuffer)
{
    return E_NOTIMPL;
}

static LONG WINAPI ms_GetSize(IMediaSample *iface)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI ms_GetTime(IMediaSample *iface, REFERENCE_TIME *pTimeStart,
        REFERENCE_TIME *pTimeEnd)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI ms_SetTime(IMediaSample *iface, REFERENCE_TIME *pTimeStart,
        REFERENCE_TIME *pTimeEnd)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI ms_IsSyncPoint(IMediaSample *iface)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI ms_SetSyncPoint(IMediaSample *iface, BOOL bIsSyncPoint)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI ms_IsPreroll(IMediaSample *iface)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI ms_SetPreroll(IMediaSample *iface, BOOL bIsPreroll)
{
    return E_NOTIMPL;
}

static LONG WINAPI ms_GetActualDataLength(IMediaSample *iface)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI ms_SetActualDataLength(IMediaSample *iface, LONG length)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI ms_GetMediaType(IMediaSample *iface, AM_MEDIA_TYPE
        **ppMediaType)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI ms_SetMediaType(IMediaSample *iface, AM_MEDIA_TYPE *pMediaType)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI ms_IsDiscontinuity(IMediaSample *iface)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI ms_SetDiscontinuity(IMediaSample *iface, BOOL bDiscontinuity)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI ms_GetMediaTime(IMediaSample *iface, LONGLONG *pTimeStart,
        LONGLONG *pTimeEnd)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI ms_SetMediaTime(IMediaSample *iface, LONGLONG *pTimeStart,
        LONGLONG *pTimeEnd)
{
    return E_NOTIMPL;
}

static const IMediaSampleVtbl my_sample_vt = {
    ms_QueryInterface,
    ms_AddRef,
    ms_Release,
    ms_GetPointer,
    ms_GetSize,
    ms_GetTime,
    ms_SetTime,
    ms_IsSyncPoint,
    ms_SetSyncPoint,
    ms_IsPreroll,
    ms_SetPreroll,
    ms_GetActualDataLength,
    ms_SetActualDataLength,
    ms_GetMediaType,
    ms_SetMediaType,
    ms_IsDiscontinuity,
    ms_SetDiscontinuity,
    ms_GetMediaTime,
    ms_SetMediaTime
};

static struct test_sample my_sample = { {&my_sample_vt}, 0 };

static BOOL samplecb_called = FALSE;

static HRESULT WINAPI sgcb_QueryInterface(ISampleGrabberCB *iface, REFIID riid,
        void **ppvObject)
{
    return E_NOTIMPL;
}

static ULONG WINAPI sgcb_AddRef(ISampleGrabberCB *iface)
{
    return E_NOTIMPL;
}

static ULONG WINAPI sgcb_Release(ISampleGrabberCB *iface)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI sgcb_SampleCB(ISampleGrabberCB *iface, double SampleTime,
        IMediaSample *pSample)
{
    ok(pSample == &my_sample.sample, "Got wrong IMediaSample: %p, expected %p\n", pSample, &my_sample);
    samplecb_called = TRUE;
    IMediaSample_AddRef(pSample);
    return E_NOTIMPL;
}

static HRESULT WINAPI sgcb_BufferCB(ISampleGrabberCB *iface, double SampleTime,
        BYTE *pBuffer, LONG BufferLen)
{
    ok(0, "BufferCB should not have been called\n");
    return E_NOTIMPL;
}

static const ISampleGrabberCBVtbl sgcb_vt = {
    sgcb_QueryInterface,
    sgcb_AddRef,
    sgcb_Release,
    sgcb_SampleCB,
    sgcb_BufferCB
};

static ISampleGrabberCB my_sg_cb = { &sgcb_vt };

static void test_samplegrabber(void)
{
    ISampleGrabber *sg;
    IBaseFilter *bf;
    IPin *pin;
    IMemInputPin *inpin;
    IEnumPins *pins;
    HRESULT hr;
    FILTER_STATE fstate;
    ULONG refcount;

    /* Invalid RIID */
    hr = CoCreateInstance(&CLSID_SampleGrabber, NULL, CLSCTX_INPROC_SERVER, &IID_IClassFactory,
            (void**)&sg);
    ok(hr == E_NOINTERFACE, "Got hr %#lx.\n", hr);

    hr = CoCreateInstance(&CLSID_SampleGrabber, NULL, CLSCTX_INPROC_SERVER, &IID_ISampleGrabber,
            (void**)&sg);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = ISampleGrabber_QueryInterface(sg, &IID_IBaseFilter, (void**)&bf);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = ISampleGrabber_SetCallback(sg, &my_sg_cb, 0);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IBaseFilter_GetState(bf, 100, &fstate);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(fstate == State_Stopped, "Got wrong filter state: %u\n", fstate);

    hr = IBaseFilter_EnumPins(bf, &pins);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IEnumPins_Next(pins, 1, &pin, NULL);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    IEnumPins_Release(pins);

    hr = IPin_QueryInterface(pin, &IID_IMemInputPin, (void**)&inpin);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IMemInputPin_Receive(inpin, &my_sample.sample);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(samplecb_called == TRUE, "SampleCB should have been called\n");

    refcount = IUnknown_Release(&my_sample.sample);
    ok(!refcount, "Got unexpected refcount %ld.\n", refcount);

    IMemInputPin_Release(inpin);
    IPin_Release(pin);

    while (ISampleGrabber_Release(sg));
}

static void test_COM_sg_enumpins(void)
{
    IBaseFilter *bf;
    IEnumPins *pins, *pins2;
    IUnknown *unk;
    ULONG refcount;
    HRESULT hr;

    hr = CoCreateInstance(&CLSID_SampleGrabber, NULL, CLSCTX_INPROC_SERVER, &IID_IBaseFilter,
            (void**)&bf);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IBaseFilter_EnumPins(bf, &pins);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    /* Same refcount for all EnumPins interfaces */
    refcount = IEnumPins_AddRef(pins);
    ok(refcount == 2, "refcount == %lu, expected 2\n", refcount);
    hr = IEnumPins_QueryInterface(pins, &IID_IEnumPins, (void**)&pins2);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(pins == pins2, "QueryInterface for self failed (%p != %p)\n", pins, pins2);
    IEnumPins_Release(pins2);

    hr = IEnumPins_QueryInterface(pins, &IID_IUnknown, (void**)&unk);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    refcount = IUnknown_AddRef(unk);
    ok(refcount == 4, "refcount == %lu, expected 4\n", refcount);
    refcount = IUnknown_Release(unk);

    while (IEnumPins_Release(pins));
    IBaseFilter_Release(bf);
}

START_TEST(mediadet)
{
    IMediaDet *detector;
    HRESULT hr;
    BOOL ret;

    if (!init_tests())
    {
        skip("Couldn't initialize tests!\n");
        return;
    }

    CoInitialize(NULL);

    if (FAILED(hr = CoCreateInstance(&CLSID_MediaDet, NULL, CLSCTX_INPROC_SERVER,
            &IID_IMediaDet, (void **)&detector)))
    {
        /* qedit.dll does not exist on 2003. */
        win_skip("Failed to create media detector object, hr %#lx.\n", hr);
        return;
    }
    IMediaDet_Release(detector);

    test_aggregation();
    test_mediadet();
    test_put_filter();
    test_samplegrabber();
    test_COM_sg_enumpins();

    ret = DeleteFileW(test_avi_filename);
    ok(ret, "Failed to delete file, error %lu.\n", GetLastError());
    ret = DeleteFileW(test_sound_avi_filename);
    ok(ret, "Failed to delete file, error %lu.\n", GetLastError());

    CoUninitialize();
}
