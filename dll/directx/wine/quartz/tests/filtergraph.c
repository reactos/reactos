/*
 * Unit tests for Direct Show functions
 *
 * Copyright (C) 2004 Christian Costa
 * Copyright (C) 2008 Alexander Dorofeyev
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

#include "dshow.h"
#include "wine/strmbase.h"
#include "wine/test.h"

static const GUID testguid = {0xabbccdde};

static BOOL compare_time(ULONGLONG x, ULONGLONG y, unsigned int max_diff)
{
    ULONGLONG diff = x > y ? x - y : y - x;
    return diff <= max_diff;
}

static WCHAR *create_file(const WCHAR *name, const char *data, DWORD size)
{
    static WCHAR pathW[MAX_PATH];
    DWORD written;
    HANDLE file;

    GetTempPathW(ARRAY_SIZE(pathW), pathW);
    wcscat(pathW, name);
    file = CreateFileW(pathW, GENERIC_READ|GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, 0);
    ok(file != INVALID_HANDLE_VALUE, "Failed to create file %s, error %lu.\n",
            wine_dbgstr_w(pathW), GetLastError());
    WriteFile(file, data, size, &written, NULL);
    ok(written == size, "Failed to write file data, error %lu.\n", GetLastError());
    CloseHandle(file);

    return pathW;
}

static WCHAR *load_resource(const WCHAR *name)
{
    HRSRC res;
    void *ptr;

    res = FindResourceW(NULL, name, (const WCHAR *)RT_RCDATA);
    ok(!!res, "Failed to find resource %s, error %lu.\n", wine_dbgstr_w(name), GetLastError());
    ptr = LockResource(LoadResource(GetModuleHandleA(NULL), res));
    return create_file(name, ptr, SizeofResource(GetModuleHandleA(NULL), res));
}

static IFilterGraph2 *create_graph(void)
{
    IFilterGraph2 *ret;
    HRESULT hr;
    hr = CoCreateInstance(&CLSID_FilterGraph, NULL, CLSCTX_INPROC_SERVER, &IID_IFilterGraph2, (void **)&ret);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    return ret;
}

static ULONG get_refcount(void *iface)
{
    IUnknown *unknown = iface;
    IUnknown_AddRef(unknown);
    return IUnknown_Release(unknown);
}

#define check_interface(a, b, c) check_interface_(__LINE__, a, b, c)
static void check_interface_(unsigned int line, void *iface_ptr, REFIID iid, BOOL supported)
{
    IUnknown *iface = iface_ptr;
    HRESULT hr, expected_hr;
    IUnknown *unk;

    expected_hr = supported ? S_OK : E_NOINTERFACE;

    hr = IUnknown_QueryInterface(iface, iid, (void **)&unk);
    ok_(__FILE__, line)(hr == expected_hr, "Got hr %#lx, expected %#lx.\n", hr, expected_hr);
    if (SUCCEEDED(hr))
        IUnknown_Release(unk);
}

static void test_interfaces(void)
{
    IFilterGraph2 *graph = create_graph();

    check_interface(graph, &IID_IBasicAudio, TRUE);
    check_interface(graph, &IID_IBasicVideo, TRUE);
    check_interface(graph, &IID_IBasicVideo2, TRUE);
    check_interface(graph, &IID_IFilterGraph, TRUE);
    check_interface(graph, &IID_IFilterGraph2, TRUE);
    check_interface(graph, &IID_IFilterMapper, TRUE);
    check_interface(graph, &IID_IFilterMapper2, TRUE);
    check_interface(graph, &IID_IFilterMapper3, TRUE);
    check_interface(graph, &IID_IGraphBuilder, TRUE);
    check_interface(graph, &IID_IGraphConfig, TRUE);
    check_interface(graph, &IID_IGraphVersion, TRUE);
    check_interface(graph, &IID_IMediaControl, TRUE);
    check_interface(graph, &IID_IMediaEvent, TRUE);
    check_interface(graph, &IID_IMediaEventEx, TRUE);
    check_interface(graph, &IID_IMediaEventSink, TRUE);
    check_interface(graph, &IID_IMediaFilter, TRUE);
    check_interface(graph, &IID_IMediaPosition, TRUE);
    check_interface(graph, &IID_IMediaSeeking, TRUE);
    check_interface(graph, &IID_IObjectWithSite, TRUE);
    check_interface(graph, &IID_IVideoFrameStep, TRUE);
    check_interface(graph, &IID_IVideoWindow, TRUE);
    check_interface(graph, &IID_IUnknown, TRUE);

    check_interface(graph, &IID_IBaseFilter, FALSE);
    check_interface(graph, &IID_IDispatch, FALSE);

    IFilterGraph2_Release(graph);
}

static void test_basic_video(IFilterGraph2 *graph)
{
    IBasicVideo* pbv;
    LONG video_width, video_height, window_width;
    LONG left, top, width, height;
    HRESULT hr;

    hr = IFilterGraph2_QueryInterface(graph, &IID_IBasicVideo, (void **)&pbv);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    /* test get video size */
    hr = IBasicVideo_GetVideoSize(pbv, NULL, NULL);
    ok(hr == E_POINTER, "Got hr %#lx.\n", hr);
    hr = IBasicVideo_GetVideoSize(pbv, &video_width, NULL);
    ok(hr == E_POINTER, "Got hr %#lx.\n", hr);
    hr = IBasicVideo_GetVideoSize(pbv, NULL, &video_height);
    ok(hr == E_POINTER, "Got hr %#lx.\n", hr);
    hr = IBasicVideo_GetVideoSize(pbv, &video_width, &video_height);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    /* test source position */
    hr = IBasicVideo_GetSourcePosition(pbv, NULL, NULL, NULL, NULL);
    ok(hr == E_POINTER, "Got hr %#lx.\n", hr);
    hr = IBasicVideo_GetSourcePosition(pbv, &left, &top, NULL, NULL);
    ok(hr == E_POINTER, "Got hr %#lx.\n", hr);
    hr = IBasicVideo_GetSourcePosition(pbv, NULL, NULL, &width, &height);
    ok(hr == E_POINTER, "Got hr %#lx.\n", hr);
    hr = IBasicVideo_GetSourcePosition(pbv, &left, &top, &width, &height);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(left == 0, "Got left %ld.\n", left);
    ok(top == 0, "Got top %ld.\n", top);
    ok(width == video_width, "Expected width %ld, got %ld.\n", video_width, width);
    ok(height == video_height, "Expected height %ld, got %ld.\n", video_height, height);

    hr = IBasicVideo_SetSourcePosition(pbv, 0, 0, 0, 0);
    ok(hr == E_INVALIDARG, "Got hr %#lx.\n", hr);
    hr = IBasicVideo_SetSourcePosition(pbv, 0, 0, video_width*2, video_height*2);
    ok(hr == E_INVALIDARG, "Got hr %#lx.\n", hr);
    hr = IBasicVideo_put_SourceTop(pbv, -1);
    ok(hr == E_INVALIDARG, "Got hr %#lx.\n", hr);
    hr = IBasicVideo_put_SourceTop(pbv, 0);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IBasicVideo_put_SourceTop(pbv, 1);
    ok(hr == E_INVALIDARG, "Got hr %#lx.\n", hr);

    hr = IBasicVideo_SetSourcePosition(pbv, video_width, 0, video_width, video_height);
    ok(hr == E_INVALIDARG, "Got hr %#lx.\n", hr);
    hr = IBasicVideo_SetSourcePosition(pbv, 0, video_height, video_width, video_height);
    ok(hr == E_INVALIDARG, "Got hr %#lx.\n", hr);
    hr = IBasicVideo_SetSourcePosition(pbv, -1, 0, video_width, video_height);
    ok(hr == E_INVALIDARG, "Got hr %#lx.\n", hr);
    hr = IBasicVideo_SetSourcePosition(pbv, 0, -1, video_width, video_height);
    ok(hr == E_INVALIDARG, "Got hr %#lx.\n", hr);
    hr = IBasicVideo_SetSourcePosition(pbv, video_width/2, video_height/2, video_width, video_height);
    ok(hr == E_INVALIDARG, "Got hr %#lx.\n", hr);
    hr = IBasicVideo_SetSourcePosition(pbv, video_width/2, video_height/2, video_width, video_height);
    ok(hr == E_INVALIDARG, "Got hr %#lx.\n", hr);

    hr = IBasicVideo_SetSourcePosition(pbv, 0, 0, video_width, video_height+1);
    ok(hr == E_INVALIDARG, "Got hr %#lx.\n", hr);
    hr = IBasicVideo_SetSourcePosition(pbv, 0, 0, video_width+1, video_height);
    ok(hr == E_INVALIDARG, "Got hr %#lx.\n", hr);

    hr = IBasicVideo_SetSourcePosition(pbv, video_width/2, video_height/2, video_width/3+1, video_height/3+1);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IBasicVideo_get_SourceLeft(pbv, &left);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(left == video_width / 2, "Expected left %ld, got %ld.\n", video_width / 2, left);
    hr = IBasicVideo_get_SourceTop(pbv, &top);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(top == video_height / 2, "Expected top %ld, got %ld.\n", video_height / 2, top);
    hr = IBasicVideo_get_SourceWidth(pbv, &width);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(width == video_width / 3 + 1, "Expected width %ld, got %ld.\n", video_width / 3 + 1, width);
    hr = IBasicVideo_get_SourceHeight(pbv, &height);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(height == video_height / 3 + 1, "Expected height %ld, got %ld.\n", video_height / 3 + 1, height);

    hr = IBasicVideo_put_SourceLeft(pbv, video_width/3);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IBasicVideo_GetSourcePosition(pbv, &left, &top, &width, &height);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(left == video_width / 3, "Expected left %ld, got %ld.\n", video_width / 3, left);
    ok(width == video_width / 3 + 1, "Expected width %ld, got %ld.\n", video_width / 3 + 1, width);

    hr = IBasicVideo_put_SourceTop(pbv, video_height/3);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IBasicVideo_GetSourcePosition(pbv, &left, &top, &width, &height);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(top == video_height / 3, "Expected top %ld, got %ld.\n", video_height / 3, top);
    ok(height == video_height / 3 + 1, "Expected height %ld, got %ld.\n", video_height / 3 + 1, height);

    hr = IBasicVideo_put_SourceWidth(pbv, video_width/4+1);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IBasicVideo_GetSourcePosition(pbv, &left, &top, &width, &height);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(left == video_width / 3, "Expected left %ld, got %ld.\n", video_width / 3, left);
    ok(width == video_width / 4 + 1, "Expected width %ld, got %ld.\n", video_width / 4 + 1, width);

    hr = IBasicVideo_put_SourceHeight(pbv, video_height/4+1);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IBasicVideo_GetSourcePosition(pbv, &left, &top, &width, &height);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(top == video_height / 3, "Expected top %ld, got %ld.\n", video_height / 3, top);
    ok(height == video_height / 4 + 1, "Expected height %ld, got %ld.\n", video_height / 4 + 1, height);

    /* test destination rectangle */
    window_width = max(video_width, GetSystemMetrics(SM_CXMIN) - 2 * GetSystemMetrics(SM_CXFRAME));

    hr = IBasicVideo_GetDestinationPosition(pbv, NULL, NULL, NULL, NULL);
    ok(hr == E_POINTER, "Got hr %#lx.\n", hr);
    hr = IBasicVideo_GetDestinationPosition(pbv, &left, &top, NULL, NULL);
    ok(hr == E_POINTER, "Got hr %#lx.\n", hr);
    hr = IBasicVideo_GetDestinationPosition(pbv, NULL, NULL, &width, &height);
    ok(hr == E_POINTER, "Got hr %#lx.\n", hr);
    hr = IBasicVideo_GetDestinationPosition(pbv, &left, &top, &width, &height);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(left == 0, "Got left %ld.\n", left);
    ok(top == 0, "Got top %ld.\n", top);
    ok(width == window_width, "Expected width %ld, got %ld.\n", window_width, width);
    ok(height == video_height, "Expected height %ld, got %ld.\n", video_height, height);

    hr = IBasicVideo_SetDestinationPosition(pbv, 0, 0, 0, 0);
    ok(hr == E_INVALIDARG, "Got hr %#lx.\n", hr);
    hr = IBasicVideo_SetDestinationPosition(pbv, 0, 0, video_width*2, video_height*2);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IBasicVideo_put_DestinationLeft(pbv, -1);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IBasicVideo_put_DestinationLeft(pbv, 0);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IBasicVideo_put_DestinationLeft(pbv, 1);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IBasicVideo_SetDestinationPosition(pbv, video_width, 0, video_width, video_height);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IBasicVideo_SetDestinationPosition(pbv, 0, video_height, video_width, video_height);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IBasicVideo_SetDestinationPosition(pbv, -1, 0, video_width, video_height);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IBasicVideo_SetDestinationPosition(pbv, 0, -1, video_width, video_height);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IBasicVideo_SetDestinationPosition(pbv, video_width/2, video_height/2, video_width, video_height);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IBasicVideo_SetDestinationPosition(pbv, video_width/2, video_height/2, video_width, video_height);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IBasicVideo_SetDestinationPosition(pbv, 0, 0, video_width, video_height+1);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IBasicVideo_SetDestinationPosition(pbv, 0, 0, video_width+1, video_height);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IBasicVideo_SetDestinationPosition(pbv, video_width/2, video_height/2, video_width/3+1, video_height/3+1);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IBasicVideo_get_DestinationLeft(pbv, &left);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(left == video_width / 2, "Expected left %ld, got %ld.\n", video_width / 2, left);
    hr = IBasicVideo_get_DestinationTop(pbv, &top);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(top == video_height / 2, "Expected top %ld, got %ld.\n", video_height / 2, top);
    hr = IBasicVideo_get_DestinationWidth(pbv, &width);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(width == video_width / 3 + 1, "Expected width %ld, got %ld.\n", video_width / 3 + 1, width);
    hr = IBasicVideo_get_DestinationHeight(pbv, &height);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(height == video_height / 3 + 1, "Expected height %ld, got %ld.\n", video_height / 3 + 1, height);

    hr = IBasicVideo_put_DestinationLeft(pbv, video_width/3);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IBasicVideo_GetDestinationPosition(pbv, &left, &top, &width, &height);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(left == video_width / 3, "Expected left %ld, got %ld.\n", video_width / 3, left);
    ok(width == video_width / 3 + 1, "Expected width %ld, got %ld.\n", video_width / 3 + 1, width);

    hr = IBasicVideo_put_DestinationTop(pbv, video_height/3);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IBasicVideo_GetDestinationPosition(pbv, &left, &top, &width, &height);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(top == video_height / 3, "Expected top %ld, got %ld.\n", video_height / 3, top);
    ok(height == video_height / 3 + 1, "Expected height %ld, got %ld.\n", video_height / 3 + 1, height);

    hr = IBasicVideo_put_DestinationWidth(pbv, video_width/4+1);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IBasicVideo_GetDestinationPosition(pbv, &left, &top, &width, &height);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(left == video_width / 3, "Expected left %ld, got %ld.\n", video_width / 3, left);
    ok(width == video_width / 4 + 1, "Expected width %ld, got %ld.\n", video_width / 4 + 1, width);

    hr = IBasicVideo_put_DestinationHeight(pbv, video_height/4+1);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IBasicVideo_GetDestinationPosition(pbv, &left, &top, &width, &height);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(top == video_height / 3, "Expected top %ld, got %ld.\n", video_height / 3, top);
    ok(height == video_height / 4 + 1, "Expected height %ld, got %ld.\n", video_height / 4 + 1, height);

    /* reset source rectangle */
    hr = IBasicVideo_SetDefaultSourcePosition(pbv);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    /* reset destination position */
    hr = IBasicVideo_SetDestinationPosition(pbv, 0, 0, video_width, video_height);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    IBasicVideo_Release(pbv);
}

static void test_media_seeking(IFilterGraph2 *graph)
{
    IMediaSeeking *seeking;
    IMediaFilter *filter;
    LONGLONG pos, stop, duration;
    GUID format;
    HRESULT hr;

    IFilterGraph2_SetDefaultSyncSource(graph);
    hr = IFilterGraph2_QueryInterface(graph, &IID_IMediaSeeking, (void **)&seeking);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IFilterGraph2_QueryInterface(graph, &IID_IMediaFilter, (void **)&filter);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    format = GUID_NULL;
    hr = IMediaSeeking_GetTimeFormat(seeking, &format);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(IsEqualGUID(&format, &TIME_FORMAT_MEDIA_TIME), "got %s\n", wine_dbgstr_guid(&format));

    pos = 0xdeadbeef;
    hr = IMediaSeeking_ConvertTimeFormat(seeking, &pos, NULL, 0x123456789a, NULL);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(pos == 0x123456789a, "got %s\n", wine_dbgstr_longlong(pos));

    pos = 0xdeadbeef;
    hr = IMediaSeeking_ConvertTimeFormat(seeking, &pos, &TIME_FORMAT_MEDIA_TIME, 0x123456789a, NULL);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(pos == 0x123456789a, "got %s\n", wine_dbgstr_longlong(pos));

    pos = 0xdeadbeef;
    hr = IMediaSeeking_ConvertTimeFormat(seeking, &pos, NULL, 0x123456789a, &TIME_FORMAT_MEDIA_TIME);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(pos == 0x123456789a, "got %s\n", wine_dbgstr_longlong(pos));

    hr = IMediaSeeking_GetCurrentPosition(seeking, &pos);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(pos == 0, "got %s\n", wine_dbgstr_longlong(pos));

    hr = IMediaSeeking_GetDuration(seeking, &duration);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(duration > 0, "got %s\n", wine_dbgstr_longlong(duration));

    hr = IMediaSeeking_GetStopPosition(seeking, &stop);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(stop == duration || stop == duration + 1, "expected %s, got %s\n",
        wine_dbgstr_longlong(duration), wine_dbgstr_longlong(stop));

    hr = IMediaSeeking_SetPositions(seeking, NULL, AM_SEEKING_ReturnTime, NULL, AM_SEEKING_NoPositioning);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IMediaSeeking_SetPositions(seeking, NULL, AM_SEEKING_NoPositioning, NULL, AM_SEEKING_ReturnTime);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    pos = 0;
    hr = IMediaSeeking_SetPositions(seeking, &pos, AM_SEEKING_AbsolutePositioning, NULL, AM_SEEKING_NoPositioning);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    IMediaFilter_SetSyncSource(filter, NULL);
    pos = 0xdeadbeef;
    hr = IMediaSeeking_GetCurrentPosition(seeking, &pos);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(pos == 0, "Position != 0 (%s)\n", wine_dbgstr_longlong(pos));
    IFilterGraph2_SetDefaultSyncSource(graph);

    IMediaSeeking_Release(seeking);
    IMediaFilter_Release(filter);
}

static void test_state_change(IFilterGraph2 *graph)
{
    IMediaControl *control;
    OAFilterState state;
    HRESULT hr;

    hr = IFilterGraph2_QueryInterface(graph, &IID_IMediaControl, (void **)&control);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IMediaControl_GetState(control, 1000, &state);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(state == State_Stopped, "Got state %ld.\n", state);

    hr = IMediaControl_Run(control);
    ok(SUCCEEDED(hr), "Got hr %#lx.\n", hr);
    hr = IMediaControl_GetState(control, INFINITE, &state);
    ok(SUCCEEDED(hr), "Got hr %#lx.\n", hr);
    ok(state == State_Running, "Got state %ld.\n", state);

    hr = IMediaControl_Stop(control);
    ok(SUCCEEDED(hr), "Got hr %#lx.\n", hr);
    hr = IMediaControl_GetState(control, 1000, &state);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(state == State_Stopped, "Got state %ld.\n", state);

    hr = IMediaControl_Pause(control);
    ok(SUCCEEDED(hr), "Got hr %#lx.\n", hr);
    hr = IMediaControl_GetState(control, 1000, &state);
    flaky_wine
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(state == State_Paused, "Got state %ld.\n", state);

    hr = IMediaControl_Run(control);
    ok(SUCCEEDED(hr), "Got hr %#lx.\n", hr);
    hr = IMediaControl_GetState(control, 1000, &state);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(state == State_Running, "Got state %ld.\n", state);

    hr = IMediaControl_Pause(control);
    ok(SUCCEEDED(hr), "Got hr %#lx.\n", hr);
    hr = IMediaControl_GetState(control, 1000, &state);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(state == State_Paused, "Got state %ld.\n", state);

    hr = IMediaControl_Stop(control);
    ok(SUCCEEDED(hr), "Got hr %#lx.\n", hr);
    hr = IMediaControl_GetState(control, 1000, &state);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(state == State_Stopped, "Got state %ld.\n", state);

    IMediaControl_Release(control);
}

static void test_media_event(IFilterGraph2 *graph)
{
    IMediaEvent *media_event;
    IMediaSeeking *seeking;
    IMediaControl *control;
    IMediaFilter *filter;
    LONG_PTR lparam1, lparam2;
    LONGLONG current, stop;
    OAFilterState state;
    int got_eos = 0;
    HANDLE event;
    HRESULT hr;
    LONG code;

    hr = IFilterGraph2_QueryInterface(graph, &IID_IMediaFilter, (void **)&filter);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IFilterGraph2_QueryInterface(graph, &IID_IMediaControl, (void **)&control);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IFilterGraph2_QueryInterface(graph, &IID_IMediaEvent, (void **)&media_event);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IFilterGraph2_QueryInterface(graph, &IID_IMediaSeeking, (void **)&seeking);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IMediaControl_Stop(control);
    ok(SUCCEEDED(hr), "Got hr %#lx.\n", hr);
    hr = IMediaControl_GetState(control, 1000, &state);
    ok(hr == S_OK, "GetState() timed out\n");

    hr = IMediaSeeking_GetDuration(seeking, &stop);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    current = 0;
    hr = IMediaSeeking_SetPositions(seeking, &current, AM_SEEKING_AbsolutePositioning, &stop, AM_SEEKING_AbsolutePositioning);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IMediaFilter_SetSyncSource(filter, NULL);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IMediaEvent_GetEventHandle(media_event, (OAEVENT *)&event);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    /* flush existing events */
    while ((hr = IMediaEvent_GetEvent(media_event, &code, &lparam1, &lparam2, 0)) == S_OK);

    ok(WaitForSingleObject(event, 0) == WAIT_TIMEOUT, "event should not be signaled\n");

    hr = IMediaControl_Run(control);
    ok(SUCCEEDED(hr), "Got hr %#lx.\n", hr);

    while (!got_eos)
    {
        if (WaitForSingleObject(event, 5000) == WAIT_TIMEOUT)
            break;

        while ((hr = IMediaEvent_GetEvent(media_event, &code, &lparam1, &lparam2, 0)) == S_OK)
        {
            if (code == EC_COMPLETE)
            {
                got_eos = 1;
                break;
            }
        }
    }
    flaky_wine
    ok(got_eos, "didn't get EOS\n");

    hr = IMediaSeeking_GetCurrentPosition(seeking, &current);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    flaky_wine
    ok(current == stop, "expected %s, got %s\n", wine_dbgstr_longlong(stop), wine_dbgstr_longlong(current));

    hr = IMediaControl_Stop(control);
    ok(SUCCEEDED(hr), "Got hr %#lx.\n", hr);
    hr = IMediaControl_GetState(control, 1000, &state);
    ok(hr == S_OK, "GetState() timed out\n");

    hr = IFilterGraph2_SetDefaultSyncSource(graph);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    IMediaSeeking_Release(seeking);
    IMediaEvent_Release(media_event);
    IMediaControl_Release(control);
    IMediaFilter_Release(filter);
}

static void rungraph(IFilterGraph2 *graph, BOOL video)
{
    if (video)
        test_basic_video(graph);
    test_media_seeking(graph);
    test_state_change(graph);
    test_media_event(graph);
}

static HRESULT test_graph_builder_connect_file(WCHAR *filename, BOOL audio, BOOL video)
{
    IBaseFilter *source_filter, *renderer;
    IPin *pin_in, *pin_out;
    IFilterGraph2 *graph;
    IEnumPins *enumpins;
    HRESULT hr;

    if (video)
        hr = CoCreateInstance(&CLSID_VideoRenderer, NULL, CLSCTX_INPROC_SERVER,
                &IID_IBaseFilter, (void **)&renderer);
    else
        hr = CoCreateInstance(&CLSID_AudioRender, NULL, CLSCTX_INPROC_SERVER,
                &IID_IBaseFilter, (void **)&renderer);
    if (hr == VFW_E_NO_AUDIO_HARDWARE)
        return VFW_E_CANNOT_CONNECT;
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    graph = create_graph();

    IBaseFilter_EnumPins(renderer, &enumpins);
    IEnumPins_Next(enumpins, 1, &pin_in, NULL);
    IEnumPins_Release(enumpins);

    hr = IFilterGraph2_AddSourceFilter(graph, filename, NULL, &source_filter);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IFilterGraph2_AddFilter(graph, renderer, NULL);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IBaseFilter_FindPin(source_filter, L"Output", &pin_out);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IFilterGraph2_Connect(graph, pin_out, pin_in);

    if (SUCCEEDED(hr))
        rungraph(graph, video);

    IPin_Release(pin_in);
    IPin_Release(pin_out);
    IBaseFilter_Release(source_filter);
    IBaseFilter_Release(renderer);
    IFilterGraph2_Release(graph);

    return hr;
}

static void test_render_run(const WCHAR *file, BOOL audio, BOOL video)
{
    IFilterGraph2 *graph;
    HANDLE h;
    HRESULT hr;
    LONG refs;
    WCHAR *filename = load_resource(file);

    h = CreateFileW(filename, 0, 0, NULL, OPEN_EXISTING, 0, NULL);
    if (h == INVALID_HANDLE_VALUE) {
        skip("Could not read test file %s, skipping test\n", wine_dbgstr_w(file));
        DeleteFileW(filename);
        return;
    }
    CloseHandle(h);

    trace("running %s\n", wine_dbgstr_w(file));

    graph = create_graph();

    hr = IFilterGraph2_RenderFile(graph, filename, NULL);
    if (FAILED(hr))
    {
        skip("%s: codec not supported; skipping test\n", wine_dbgstr_w(file));

        refs = IFilterGraph2_Release(graph);
        ok(!refs, "Got outsanding refcount %ld.\n", refs);

        hr = test_graph_builder_connect_file(filename, audio, video);
        ok(hr == VFW_E_CANNOT_CONNECT, "Got hr %#lx.\n", hr);
    }
    else
    {
        if (audio)
            ok(hr == S_OK || hr == VFW_S_AUDIO_NOT_RENDERED, "Got hr %#lx.\n", hr);
        else
            ok(hr == S_OK, "Got hr %#lx.\n", hr);
        rungraph(graph, video);

        refs = IFilterGraph2_Release(graph);
        ok(!refs, "Got outsanding refcount %ld.\n", refs);

        hr = test_graph_builder_connect_file(filename, audio, video);
        if (audio && video)
            todo_wine ok(hr == VFW_S_PARTIAL_RENDER, "Got hr %#lx.\n", hr);
        else
            ok(hr == S_OK, "Got hr %#lx.\n", hr);
    }

    /* check reference leaks */
    h = CreateFileW(filename, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
    ok(h != INVALID_HANDLE_VALUE, "CreateFile failed: err=%ld\n", GetLastError());
    CloseHandle(h);

    DeleteFileW(filename);
}

static void test_enum_filters(void)
{
    IBaseFilter *filter1, *filter2, *filters[2];
    IFilterGraph2 *graph = create_graph();
    IEnumFilters *enum1, *enum2;
    ULONG count, ref;
    HRESULT hr;

    CoCreateInstance(&CLSID_AsyncReader, NULL, CLSCTX_INPROC_SERVER,
            &IID_IBaseFilter, (void **)&filter1);
    CoCreateInstance(&CLSID_AsyncReader, NULL, CLSCTX_INPROC_SERVER,
            &IID_IBaseFilter, (void **)&filter2);

    hr = IFilterGraph2_EnumFilters(graph, &enum1);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IEnumFilters_Next(enum1, 1, filters, NULL);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);

    hr = IEnumFilters_Skip(enum1, 0);
    ok(hr == E_INVALIDARG, "Got hr %#lx.\n", hr);

    hr = IEnumFilters_Skip(enum1, 1);
    ok(hr == E_INVALIDARG, "Got hr %#lx.\n", hr);

    IFilterGraph2_AddFilter(graph, filter1, NULL);
    IFilterGraph2_AddFilter(graph, filter2, NULL);

    hr = IEnumFilters_Next(enum1, 1, filters, NULL);
    ok(hr == VFW_E_ENUM_OUT_OF_SYNC, "Got hr %#lx.\n", hr);

    hr = IEnumFilters_Skip(enum1, 1);
    ok(hr == VFW_E_ENUM_OUT_OF_SYNC, "Got hr %#lx.\n", hr);

    hr = IEnumFilters_Reset(enum1);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IEnumFilters_Skip(enum1, 0);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IEnumFilters_Skip(enum1, 1);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IEnumFilters_Skip(enum1, 2);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);

    hr = IEnumFilters_Skip(enum1, 1);
    ok(hr == E_INVALIDARG, "Got hr %#lx.\n", hr);

    hr = IEnumFilters_Reset(enum1);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IEnumFilters_Skip(enum1, 2);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IEnumFilters_Skip(enum1, 1);
    ok(hr == E_INVALIDARG, "Got hr %#lx.\n", hr);

    hr = IEnumFilters_Reset(enum1);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IEnumFilters_Next(enum1, 1, filters, NULL);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(filters[0] == filter2, "Got filter %p.\n", filters[0]);
    IBaseFilter_Release(filters[0]);

    hr = IEnumFilters_Next(enum1, 1, filters, &count);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(count == 1, "Got count %lu.\n", count);
    ok(filters[0] == filter1, "Got filter %p.\n", filters[0]);
    IBaseFilter_Release(filters[0]);

    hr = IEnumFilters_Next(enum1, 1, filters, &count);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);
    ok(count == 0, "Got count %lu.\n", count);

    hr = IEnumFilters_Reset(enum1);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IEnumFilters_Next(enum1, 2, filters, &count);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(count == 2, "Got count %lu.\n", count);
    ok(filters[0] == filter2, "Got filter %p.\n", filters[0]);
    ok(filters[1] == filter1, "Got filter %p.\n", filters[1]);
    IBaseFilter_Release(filters[0]);
    IBaseFilter_Release(filters[1]);

    IFilterGraph2_RemoveFilter(graph, filter1);
    IFilterGraph2_AddFilter(graph, filter1, NULL);

    hr = IEnumFilters_Next(enum1, 2, filters, &count);
    ok(hr == VFW_E_ENUM_OUT_OF_SYNC, "Got hr %#lx.\n", hr);

    hr = IEnumFilters_Reset(enum1);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IEnumFilters_Next(enum1, 2, filters, &count);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(count == 2, "Got count %lu.\n", count);
    ok(filters[0] == filter1, "Got filter %p.\n", filters[0]);
    ok(filters[1] == filter2, "Got filter %p.\n", filters[1]);
    IBaseFilter_Release(filters[0]);
    IBaseFilter_Release(filters[1]);

    hr = IEnumFilters_Reset(enum1);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IEnumFilters_Clone(enum1, &enum2);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IEnumFilters_Skip(enum2, 1);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IEnumFilters_Next(enum2, 2, filters, &count);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);
    ok(count == 1, "Got count %lu.\n", count);
    ok(filters[0] == filter2, "Got filter %p.\n", filters[0]);
    IBaseFilter_Release(filters[0]);

    hr = IEnumFilters_Skip(enum1, 3);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);

    IEnumFilters_Release(enum2);
    IEnumFilters_Release(enum1);
    ref = IFilterGraph2_Release(graph);
    ok(!ref, "Got outstanding refcount %ld.\n", ref);
}

static DWORD WINAPI call_RenderFile_multithread(LPVOID lParam)
{
    WCHAR *filename = load_resource(L"test.avi");
    IFilterGraph2 *graph = lParam;
    HRESULT hr;

    hr = IFilterGraph2_RenderFile(graph, filename, NULL);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    rungraph(graph, TRUE);

    return 0;
}

static void test_render_with_multithread(void)
{
    IFilterGraph2 *graph;
    HANDLE thread;

    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

    graph = create_graph();

    thread = CreateThread(NULL, 0, call_RenderFile_multithread, graph, 0, NULL);

    ok(!WaitForSingleObject(thread, 10000), "Wait timed out.\n");
    IFilterGraph2_Release(graph);
    CloseHandle(thread);
    CoUninitialize();
}

struct testpin
{
    IPin IPin_iface;
    IMemInputPin IMemInputPin_iface;
    LONG ref;
    PIN_DIRECTION dir;
    struct testfilter *filter;
    IPin *peer;
    AM_MEDIA_TYPE *mt;
    WCHAR name[10];
    WCHAR id[10];

    IEnumMediaTypes IEnumMediaTypes_iface;
    const AM_MEDIA_TYPE *types;
    unsigned int type_count, enum_idx;
    AM_MEDIA_TYPE *request_mt, *accept_mt;
    const struct testpin *require_connected_pin;

    BOOL require_stopped_disconnect;

    HRESULT Connect_hr;
    HRESULT EnumMediaTypes_hr;
    HRESULT QueryInternalConnections_hr;

    BYTE input[64];
    ULONG input_size;
    HANDLE on_input_full;
};

struct testfilter
{
    IBaseFilter IBaseFilter_iface;
    LONG ref;
    IFilterGraph *graph;
    WCHAR *name;
    IReferenceClock *clock;

    IEnumPins IEnumPins_iface;
    struct testpin *pins;
    unsigned int pin_count, enum_idx;

    FILTER_STATE state;
    REFERENCE_TIME start_time;
    HRESULT state_hr, GetState_hr, seek_hr;
    FILTER_STATE expect_stop_prev, expect_run_prev;

    IAMFilterMiscFlags IAMFilterMiscFlags_iface;
    ULONG misc_flags;

    IMediaSeeking IMediaSeeking_iface;
    IMediaPosition IMediaPosition_iface;
    LONG seeking_ref;
    DWORD seek_caps;
    BOOL support_testguid, support_media_time;
    GUID time_format;
    LONGLONG seek_duration, seek_current, seek_stop;
    double seek_rate;

    IReferenceClock IReferenceClock_iface;

    IFileSourceFilter IFileSourceFilter_iface;
};

static inline struct testpin *impl_from_IEnumMediaTypes(IEnumMediaTypes *iface)
{
    return CONTAINING_RECORD(iface, struct testpin, IEnumMediaTypes_iface);
}

static HRESULT WINAPI testenummt_QueryInterface(IEnumMediaTypes *iface, REFIID iid, void **out)
{
    struct testpin *pin = impl_from_IEnumMediaTypes(iface);
    if (winetest_debug > 1) trace("%p->QueryInterface(%s)\n", pin, wine_dbgstr_guid(iid));

    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI testenummt_AddRef(IEnumMediaTypes *iface)
{
    struct testpin *pin = impl_from_IEnumMediaTypes(iface);
    return InterlockedIncrement(&pin->ref);
}

static ULONG WINAPI testenummt_Release(IEnumMediaTypes *iface)
{
    struct testpin *pin = impl_from_IEnumMediaTypes(iface);
    return InterlockedDecrement(&pin->ref);
}

static HRESULT WINAPI testenummt_Next(IEnumMediaTypes *iface, ULONG count, AM_MEDIA_TYPE **out, ULONG *fetched)
{
    struct testpin *pin = impl_from_IEnumMediaTypes(iface);
    unsigned int i;

    for (i = 0; i < count; ++i)
    {
        if (pin->enum_idx + i >= pin->type_count)
            break;

        out[i] = CoTaskMemAlloc(sizeof(*out[i]));
        *out[i] = pin->types[pin->enum_idx + i];
    }

    if (fetched)
        *fetched = i;
    pin->enum_idx += i;

    return (i == count) ? S_OK : S_FALSE;
}

static HRESULT WINAPI testenummt_Skip(IEnumMediaTypes *iface, ULONG count)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI testenummt_Reset(IEnumMediaTypes *iface)
{
    struct testpin *pin = impl_from_IEnumMediaTypes(iface);
    pin->enum_idx = 0;
    return S_OK;
}

static HRESULT WINAPI testenummt_Clone(IEnumMediaTypes *iface, IEnumMediaTypes **out)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static const IEnumMediaTypesVtbl testenummt_vtbl =
{
    testenummt_QueryInterface,
    testenummt_AddRef,
    testenummt_Release,
    testenummt_Next,
    testenummt_Skip,
    testenummt_Reset,
    testenummt_Clone,
};

static inline struct testpin *impl_from_IPin(IPin *iface)
{
    return CONTAINING_RECORD(iface, struct testpin, IPin_iface);
}

static inline struct testpin *impl_from_IMemInputPin(IMemInputPin *iface)
{
    return CONTAINING_RECORD(iface, struct testpin, IMemInputPin_iface);
}

static HRESULT WINAPI testpin_QueryInterface(IPin *iface, REFIID iid, void **out)
{
    struct testpin *pin = impl_from_IPin(iface);
    if (winetest_debug > 1) trace("%p->QueryInterface(%s)\n", pin, wine_dbgstr_guid(iid));

    if (IsEqualGUID(iid, &IID_IUnknown) || IsEqualGUID(iid, &IID_IPin))
    {
        *out = &pin->IPin_iface;
        IPin_AddRef(*out);
        return S_OK;
    }
    if (pin->IMemInputPin_iface.lpVtbl && IsEqualGUID(iid, &IID_IMemInputPin))
    {
        *out = &pin->IMemInputPin_iface;
        IMemInputPin_AddRef(*out);
        return S_OK;
    }

    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI testpin_AddRef(IPin *iface)
{
    struct testpin *pin = impl_from_IPin(iface);
    return InterlockedIncrement(&pin->ref);
}

static ULONG WINAPI testpin_Release(IPin *iface)
{
    struct testpin *pin = impl_from_IPin(iface);
    return InterlockedDecrement(&pin->ref);
}

static HRESULT WINAPI testpin_Disconnect(IPin *iface)
{
    struct testpin *pin = impl_from_IPin(iface);
    if (winetest_debug > 1) trace("%p->Disconnect()\n", pin);

    if (!pin->peer)
        return S_FALSE;

    if (pin->require_stopped_disconnect && pin->filter->state != State_Stopped)
        return VFW_E_NOT_STOPPED;

    IPin_Release(pin->peer);
    pin->peer = NULL;
    return S_OK;
}

static HRESULT WINAPI testpin_ConnectedTo(IPin *iface, IPin **peer)
{
    struct testpin *pin = impl_from_IPin(iface);
    if (winetest_debug > 1) trace("%p->ConnectedTo()\n", pin);

    *peer = pin->peer;
    if (*peer)
    {
        IPin_AddRef(*peer);
        return S_OK;
    }
    return VFW_E_NOT_CONNECTED;
}

static HRESULT WINAPI testpin_ConnectionMediaType(IPin *iface, AM_MEDIA_TYPE *mt)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI testpin_QueryPinInfo(IPin *iface, PIN_INFO *info)
{
    struct testpin *pin = impl_from_IPin(iface);
    if (winetest_debug > 1) trace("%p->QueryPinInfo()\n", pin);

    info->pFilter = &pin->filter->IBaseFilter_iface;
    IBaseFilter_AddRef(info->pFilter);
    info->dir = pin->dir;
    wcscpy(info->achName, pin->name);
    return S_OK;
}


static HRESULT WINAPI testpin_QueryDirection(IPin *iface, PIN_DIRECTION *dir)
{
    struct testpin *pin = impl_from_IPin(iface);
    if (winetest_debug > 1) trace("%p->QueryDirection()\n", pin);

    *dir = pin->dir;
    return S_OK;
}

static HRESULT WINAPI testpin_QueryId(IPin *iface, WCHAR **id)
{
    struct testpin *pin = impl_from_IPin(iface);
    if (winetest_debug > 1) trace("%p->QueryId()\n", iface);
    *id = CoTaskMemAlloc(sizeof(WCHAR)*11);
    wcscpy(*id, pin->id);
    return S_OK;
}

static HRESULT WINAPI testpin_QueryAccept(IPin *iface, const AM_MEDIA_TYPE *mt)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI testpin_EnumMediaTypes(IPin *iface, IEnumMediaTypes **out)
{
    struct testpin *pin = impl_from_IPin(iface);
    if (winetest_debug > 1) trace("%p->EnumMediaTypes()\n", pin);

    if (FAILED(pin->EnumMediaTypes_hr))
        return pin->EnumMediaTypes_hr;

    *out = &pin->IEnumMediaTypes_iface;
    IEnumMediaTypes_AddRef(*out);
    pin->enum_idx = 0;
    return S_OK;
}

static HRESULT WINAPI testpin_QueryInternalConnections(IPin *iface, IPin **out, ULONG *count)
{
    struct testpin *pin = impl_from_IPin(iface);
    if (winetest_debug > 1) trace("%p->QueryInternalConnections()\n", pin);

    *count = 0;
    return pin->QueryInternalConnections_hr;
}

static HRESULT WINAPI testpin_BeginFlush(IPin *iface)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI testpin_EndFlush(IPin * iface)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI testpin_NewSegment(IPin *iface, REFERENCE_TIME start, REFERENCE_TIME stop, double rate)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI testpin_EndOfStream(IPin *iface)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI no_Connect(IPin *iface, IPin *peer, const AM_MEDIA_TYPE *mt)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI no_ReceiveConnection(IPin *iface, IPin *peer, const AM_MEDIA_TYPE *mt)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI no_EnumMediaTypes(IPin *iface, IEnumMediaTypes **out)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI testsink_ReceiveConnection(IPin *iface, IPin *peer, const AM_MEDIA_TYPE *mt)
{
    struct testpin *pin = impl_from_IPin(iface);
    if (winetest_debug > 1) trace("%p->ReceiveConnection(%p)\n", pin, peer);

    if (pin->accept_mt && memcmp(pin->accept_mt, mt, sizeof(*mt)))
        return VFW_E_TYPE_NOT_ACCEPTED;

    if (pin->require_connected_pin && !pin->require_connected_pin->peer)
        return VFW_E_TYPE_NOT_ACCEPTED;

    pin->peer = peer;
    IPin_AddRef(peer);
    return S_OK;
}

static const IPinVtbl testsink_vtbl =
{
    testpin_QueryInterface,
    testpin_AddRef,
    testpin_Release,
    no_Connect,
    testsink_ReceiveConnection,
    testpin_Disconnect,
    testpin_ConnectedTo,
    testpin_ConnectionMediaType,
    testpin_QueryPinInfo,
    testpin_QueryDirection,
    testpin_QueryId,
    testpin_QueryAccept,
    no_EnumMediaTypes,
    testpin_QueryInternalConnections,
    testpin_EndOfStream,
    testpin_BeginFlush,
    testpin_EndFlush,
    testpin_NewSegment
};

static void testpin_init(struct testpin *pin, const IPinVtbl *vtbl, PIN_DIRECTION dir)
{
    memset(pin, 0, sizeof(*pin));
    pin->IPin_iface.lpVtbl = vtbl;
    pin->IEnumMediaTypes_iface.lpVtbl = &testenummt_vtbl;
    pin->ref = 1;
    pin->dir = dir;
    pin->Connect_hr = S_OK;
    pin->EnumMediaTypes_hr = S_OK;
    pin->QueryInternalConnections_hr = E_NOTIMPL;
    wsprintfW(pin->name, L"name%.4x", 0xFFFF&(intptr_t)pin);
    wsprintfW(pin->id, L"id%.4x", 0xFFFF&(intptr_t)pin);
}

static void testsink_init(struct testpin *pin)
{
    testpin_init(pin, &testsink_vtbl, PINDIR_INPUT);
}

static HRESULT WINAPI testsource_Connect(IPin *iface, IPin *peer, const AM_MEDIA_TYPE *mt)
{
    struct testpin *pin = impl_from_IPin(iface);
    HRESULT hr;
    if (winetest_debug > 1) trace("%p->Connect(%p)\n", pin, peer);

    if (FAILED(pin->Connect_hr))
        return pin->Connect_hr;

    if (pin->require_connected_pin && !pin->require_connected_pin->peer)
        return VFW_E_NO_ACCEPTABLE_TYPES;

    ok(!mt, "Got media type %p.\n", mt);

    if (SUCCEEDED(hr = IPin_ReceiveConnection(peer, &pin->IPin_iface, pin->request_mt)))
    {
        pin->peer = peer;
        IPin_AddRef(peer);
        return pin->Connect_hr;
    }
    return hr;
}

static const IPinVtbl testsource_vtbl =
{
    testpin_QueryInterface,
    testpin_AddRef,
    testpin_Release,
    testsource_Connect,
    no_ReceiveConnection,
    testpin_Disconnect,
    testpin_ConnectedTo,
    testpin_ConnectionMediaType,
    testpin_QueryPinInfo,
    testpin_QueryDirection,
    testpin_QueryId,
    testpin_QueryAccept,
    testpin_EnumMediaTypes,
    testpin_QueryInternalConnections,
    testpin_EndOfStream,
    testpin_BeginFlush,
    testpin_EndFlush,
    testpin_NewSegment
};

static void testsource_init(struct testpin *pin, const AM_MEDIA_TYPE *types, int type_count)
{
    testpin_init(pin, &testsource_vtbl, PINDIR_OUTPUT);
    pin->types = types;
    pin->type_count = type_count;
}

static inline struct testfilter *impl_from_IEnumPins(IEnumPins *iface)
{
    return CONTAINING_RECORD(iface, struct testfilter, IEnumPins_iface);
}

static HRESULT WINAPI testenumpins_QueryInterface(IEnumPins *iface, REFIID iid, void **out)
{
    ok(0, "Unexpected iid %s.\n", wine_dbgstr_guid(iid));
    return E_NOINTERFACE;
}

static ULONG WINAPI testenumpins_AddRef(IEnumPins * iface)
{
    struct testfilter *filter = impl_from_IEnumPins(iface);
    return InterlockedIncrement(&filter->ref);
}

static ULONG WINAPI testenumpins_Release(IEnumPins * iface)
{
    struct testfilter *filter = impl_from_IEnumPins(iface);
    return InterlockedDecrement(&filter->ref);
}

static HRESULT WINAPI testenumpins_Next(IEnumPins *iface, ULONG count, IPin **out, ULONG *fetched)
{
    struct testfilter *filter = impl_from_IEnumPins(iface);
    unsigned int i;

    for (i = 0; i < count; ++i)
    {
        if (filter->enum_idx + i >= filter->pin_count)
            break;

        out[i] = &filter->pins[filter->enum_idx + i].IPin_iface;
        IPin_AddRef(out[i]);
    }

    if (fetched)
        *fetched = i;
    filter->enum_idx += i;

    return (i == count) ? S_OK : S_FALSE;
}

static HRESULT WINAPI testenumpins_Skip(IEnumPins *iface, ULONG count)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI testenumpins_Reset(IEnumPins *iface)
{
    struct testfilter *filter = impl_from_IEnumPins(iface);
    filter->enum_idx = 0;
    return S_OK;
}

static HRESULT WINAPI testenumpins_Clone(IEnumPins *iface, IEnumPins **out)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static const IEnumPinsVtbl testenumpins_vtbl =
{
    testenumpins_QueryInterface,
    testenumpins_AddRef,
    testenumpins_Release,
    testenumpins_Next,
    testenumpins_Skip,
    testenumpins_Reset,
    testenumpins_Clone,
};

static inline struct testfilter *impl_from_IBaseFilter(IBaseFilter *iface)
{
    return CONTAINING_RECORD(iface, struct testfilter, IBaseFilter_iface);
}

static HRESULT WINAPI testfilter_QueryInterface(IBaseFilter *iface, REFIID iid, void **out)
{
    struct testfilter *filter = impl_from_IBaseFilter(iface);
    if (winetest_debug > 1) trace("%p->QueryInterface(%s)\n", filter, wine_dbgstr_guid(iid));

    if (IsEqualGUID(iid, &IID_IUnknown)
            || IsEqualGUID(iid, &IID_IPersist)
            || IsEqualGUID(iid, &IID_IMediaFilter)
            || IsEqualGUID(iid, &IID_IBaseFilter))
    {
        *out = &filter->IBaseFilter_iface;
    }
    else if (IsEqualGUID(iid, &IID_IAMFilterMiscFlags) && filter->IAMFilterMiscFlags_iface.lpVtbl)
    {
        *out = &filter->IAMFilterMiscFlags_iface;
    }
    else if (IsEqualGUID(iid, &IID_IMediaSeeking) && filter->IMediaSeeking_iface.lpVtbl)
    {
        *out = &filter->IMediaSeeking_iface;
    }
    else if (IsEqualGUID(iid, &IID_IMediaPosition) && filter->IMediaPosition_iface.lpVtbl)
    {
        *out = &filter->IMediaPosition_iface;
    }
    else if (IsEqualGUID(iid, &IID_IReferenceClock) && filter->IReferenceClock_iface.lpVtbl)
    {
        *out = &filter->IReferenceClock_iface;
    }
    else if (IsEqualGUID(iid, &IID_IFileSourceFilter) && filter->IFileSourceFilter_iface.lpVtbl)
    {
        *out = &filter->IFileSourceFilter_iface;
    }
    else
    {
        *out = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown *)*out);
    return S_OK;
}

static ULONG WINAPI testfilter_AddRef(IBaseFilter *iface)
{
    struct testfilter *filter = impl_from_IBaseFilter(iface);
    return InterlockedIncrement(&filter->ref);
}

static ULONG WINAPI testfilter_Release(IBaseFilter *iface)
{
    struct testfilter *filter = impl_from_IBaseFilter(iface);
    return InterlockedDecrement(&filter->ref);
}

static HRESULT WINAPI testfilter_GetClassID(IBaseFilter *iface, CLSID *clsid)
{
    if (winetest_debug > 1) trace("%p->GetClassID()\n", iface);
    memset(clsid, 0xde, sizeof(*clsid));
    return S_OK;
}

/* Downstream filters are always stopped before any filters they are connected
 * to upstream. Native actually implements this by topologically sorting filters
 * as they are connected. */
static void check_state_transition(struct testfilter *filter, FILTER_STATE expect)
{
    FILTER_STATE state;
    unsigned int i;
    PIN_INFO info;

    for (i = 0; i < filter->pin_count; ++i)
    {
        if (filter->pins[i].peer)
        {
            IPin_QueryPinInfo(filter->pins[i].peer, &info);
            IBaseFilter_GetState(info.pFilter, 0, &state);
            if (filter->pins[i].dir == PINDIR_OUTPUT)
                ok(state == expect, "Expected state %d for downstream filter %p, got %d.\n",
                        expect, info.pFilter, state);
            else
                ok(state == filter->state, "Expected state %d for upstream filter %p, got %d.\n",
                        filter->state, info.pFilter, state);
            IBaseFilter_Release(info.pFilter);
        }
    }
}

static HRESULT WINAPI testfilter_Stop(IBaseFilter *iface)
{
    struct testfilter *filter = impl_from_IBaseFilter(iface);
    if (winetest_debug > 1) trace("%p->Stop()\n", filter);

    todo_wine_if (filter->expect_stop_prev == State_Running)
        ok(filter->state == filter->expect_stop_prev, "Expected previous state %#x, got %#x.\n",
                filter->expect_stop_prev, filter->state);

    check_state_transition(filter, State_Stopped);

    filter->state = State_Stopped;
    return filter->state_hr;
}

static HRESULT WINAPI testfilter_Pause(IBaseFilter *iface)
{
    struct testfilter *filter = impl_from_IBaseFilter(iface);
    if (winetest_debug > 1) trace("%p->Pause()\n", filter);

    check_state_transition(filter, State_Paused);

    filter->state = State_Paused;
    return filter->state_hr;
}

static HRESULT WINAPI testfilter_Run(IBaseFilter *iface, REFERENCE_TIME start)
{
    struct testfilter *filter = impl_from_IBaseFilter(iface);
    if (winetest_debug > 1) trace("%p->Run(%s)\n", filter, wine_dbgstr_longlong(start));

    ok(filter->state == filter->expect_run_prev, "Expected previous state %#x, got %#x.\n",
            filter->expect_run_prev, filter->state);

    check_state_transition(filter, State_Running);

    filter->state = State_Running;
    filter->start_time = start;
    return filter->state_hr;
}

static HRESULT WINAPI testfilter_GetState(IBaseFilter *iface, DWORD timeout, FILTER_STATE *state)
{
    struct testfilter *filter = impl_from_IBaseFilter(iface);
    if (winetest_debug > 1) trace("%p->GetState(%lu)\n", filter, timeout);

    *state = filter->state;
    return filter->GetState_hr;
}

static HRESULT WINAPI testfilter_SetSyncSource(IBaseFilter *iface, IReferenceClock *clock)
{
    struct testfilter *filter = impl_from_IBaseFilter(iface);
    if (winetest_debug > 1) trace("%p->SetSyncSource(%p)\n", filter, clock);

    if (filter->clock)
        IReferenceClock_Release(filter->clock);
    if (clock)
        IReferenceClock_AddRef(clock);
    filter->clock = clock;
    return S_OK;
}

static HRESULT WINAPI testfilter_GetSyncSource(IBaseFilter *iface, IReferenceClock **clock)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI testfilter_EnumPins(IBaseFilter *iface, IEnumPins **out)
{
    struct testfilter *filter = impl_from_IBaseFilter(iface);
    if (winetest_debug > 1) trace("%p->EnumPins()\n", filter);

    *out = &filter->IEnumPins_iface;
    IEnumPins_AddRef(*out);
    filter->enum_idx = 0;
    return S_OK;
}

static HRESULT WINAPI testfilter_FindPin(IBaseFilter *iface, const WCHAR *id, IPin **pin)
{
    struct testfilter *filter = impl_from_IBaseFilter(iface);
    int i;
    if (winetest_debug > 1) trace("%p->FindPin(%ls)\n", filter, id);

    for (i = 0; i < filter->pin_count; ++i)
    {
        if (!wcscmp(id, filter->pins[i].id))
        {
            *pin = &filter->pins[i].IPin_iface;
            IPin_AddRef(*pin);
            return S_OK;
        }
    }
    return VFW_E_NOT_FOUND;
}

static HRESULT WINAPI testfilter_QueryFilterInfo(IBaseFilter *iface, FILTER_INFO *info)
{
    struct testfilter *filter = impl_from_IBaseFilter(iface);
    if (winetest_debug > 1) trace("%p->QueryFilterInfo()\n", filter);

    info->pGraph = filter->graph;
    if (filter->graph)
        IFilterGraph_AddRef(filter->graph);
    if (filter->name)
        wcscpy(info->achName, filter->name);
    else
        info->achName[0] = 0;
    return S_OK;
}

static HRESULT WINAPI testfilter_JoinFilterGraph(IBaseFilter *iface, IFilterGraph *graph, const WCHAR *name)
{
    struct testfilter *filter = impl_from_IBaseFilter(iface);
    if (winetest_debug > 1) trace("%p->JoinFilterGraph(%p, %s)\n", filter, graph, wine_dbgstr_w(name));

    filter->graph = graph;
    free(filter->name);
    filter->name = wcsdup(name);
    return S_OK;
}

static HRESULT WINAPI testfilter_QueryVendorInfo(IBaseFilter * iface, WCHAR **info)
{
    return E_NOTIMPL;
}

static const IBaseFilterVtbl testfilter_vtbl =
{
    testfilter_QueryInterface,
    testfilter_AddRef,
    testfilter_Release,
    testfilter_GetClassID,
    testfilter_Stop,
    testfilter_Pause,
    testfilter_Run,
    testfilter_GetState,
    testfilter_SetSyncSource,
    testfilter_GetSyncSource,
    testfilter_EnumPins,
    testfilter_FindPin,
    testfilter_QueryFilterInfo,
    testfilter_JoinFilterGraph,
    testfilter_QueryVendorInfo
};

static struct testfilter *impl_from_IAMFilterMiscFlags(IAMFilterMiscFlags *iface)
{
    return CONTAINING_RECORD(iface, struct testfilter, IAMFilterMiscFlags_iface);
}

static HRESULT WINAPI testmiscflags_QueryInterface(IAMFilterMiscFlags *iface, REFIID iid, void **out)
{
    struct testfilter *filter = impl_from_IAMFilterMiscFlags(iface);
    return IBaseFilter_QueryInterface(&filter->IBaseFilter_iface, iid, out);
}

static ULONG WINAPI testmiscflags_AddRef(IAMFilterMiscFlags *iface)
{
    struct testfilter *filter = impl_from_IAMFilterMiscFlags(iface);
    return InterlockedIncrement(&filter->ref);
}

static ULONG WINAPI testmiscflags_Release(IAMFilterMiscFlags *iface)
{
    struct testfilter *filter = impl_from_IAMFilterMiscFlags(iface);
    return InterlockedDecrement(&filter->ref);
}

static ULONG WINAPI testmiscflags_GetMiscFlags(IAMFilterMiscFlags *iface)
{
    struct testfilter *filter = impl_from_IAMFilterMiscFlags(iface);
    if (winetest_debug > 1) trace("%p->GetMiscFlags()\n", filter);
    return filter->misc_flags;
}

static const IAMFilterMiscFlagsVtbl testmiscflags_vtbl =
{
    testmiscflags_QueryInterface,
    testmiscflags_AddRef,
    testmiscflags_Release,
    testmiscflags_GetMiscFlags,
};

static struct testfilter *impl_from_IMediaSeeking(IMediaSeeking *iface)
{
    return CONTAINING_RECORD(iface, struct testfilter, IMediaSeeking_iface);
}

static HRESULT WINAPI testseek_QueryInterface(IMediaSeeking *iface, REFIID iid, void **out)
{
    struct testfilter *filter = impl_from_IMediaSeeking(iface);
    return IBaseFilter_QueryInterface(&filter->IBaseFilter_iface, iid, out);
}

static ULONG WINAPI testseek_AddRef(IMediaSeeking *iface)
{
    struct testfilter *filter = impl_from_IMediaSeeking(iface);
    InterlockedIncrement(&filter->seeking_ref);
    return InterlockedIncrement(&filter->ref);
}

static ULONG WINAPI testseek_Release(IMediaSeeking *iface)
{
    struct testfilter *filter = impl_from_IMediaSeeking(iface);
    InterlockedDecrement(&filter->seeking_ref);
    return InterlockedDecrement(&filter->ref);
}

static HRESULT WINAPI testseek_GetCapabilities(IMediaSeeking *iface, DWORD *caps)
{
    struct testfilter *filter = impl_from_IMediaSeeking(iface);
    if (winetest_debug > 1) trace("%p->GetCapabilities()\n", iface);
    *caps = filter->seek_caps;
    return S_OK;
}

static HRESULT WINAPI testseek_CheckCapabilities(IMediaSeeking *iface, DWORD *caps)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI testseek_IsFormatSupported(IMediaSeeking *iface, const GUID *format)
{
    struct testfilter *filter = impl_from_IMediaSeeking(iface);
    if (winetest_debug > 1) trace("%p->IsFormatSupported(%s)\n", iface, wine_dbgstr_guid(format));
    if (IsEqualGUID(format, &testguid) && !filter->support_testguid)
        return S_FALSE;
    if (IsEqualGUID(format, &TIME_FORMAT_MEDIA_TIME) && !filter->support_media_time)
        return S_FALSE;
    return S_OK;
}

static HRESULT WINAPI testseek_QueryPreferredFormat(IMediaSeeking *iface, GUID *format)
{
    if (winetest_debug > 1) trace("%p->QueryPreferredFormat()\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI testseek_GetTimeFormat(IMediaSeeking *iface, GUID *format)
{
    if (winetest_debug > 1) trace("%p->GetTimeFormat()\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI testseek_IsUsingTimeFormat(IMediaSeeking *iface, const GUID *format)
{
    if (winetest_debug > 1) trace("%p->IsUsingTimeFormat(%s)\n", iface, wine_dbgstr_guid(format));
    return S_OK;
}

static HRESULT WINAPI testseek_SetTimeFormat(IMediaSeeking *iface, const GUID *format)
{
    struct testfilter *filter = impl_from_IMediaSeeking(iface);
    if (winetest_debug > 1) trace("%p->SetTimeFormat(%s)\n", iface, wine_dbgstr_guid(format));
    ok(!IsEqualGUID(format, &GUID_NULL), "Got unexpected GUID_NULL.\n");
    if (IsEqualGUID(format, &testguid) && !filter->support_testguid)
        return E_INVALIDARG;
    filter->time_format = *format;
    return S_OK;
}

static HRESULT WINAPI testseek_GetDuration(IMediaSeeking *iface, LONGLONG *duration)
{
    struct testfilter *filter = impl_from_IMediaSeeking(iface);
    if (winetest_debug > 1) trace("%p->GetDuration()\n", iface);
    *duration = filter->seek_duration;
    return filter->seek_hr;
}

static HRESULT WINAPI testseek_GetStopPosition(IMediaSeeking *iface, LONGLONG *stop)
{
    struct testfilter *filter = impl_from_IMediaSeeking(iface);
    if (winetest_debug > 1) trace("%p->GetStopPosition()\n", iface);
    *stop = filter->seek_stop;
    return filter->seek_hr;
}

static HRESULT WINAPI testseek_GetCurrentPosition(IMediaSeeking *iface, LONGLONG *current)
{
    struct testfilter *filter = impl_from_IMediaSeeking(iface);
    if (winetest_debug > 1) trace("%p->GetCurrentPosition()\n", iface);
    ok(!filter->clock, "GetCurrentPosition() should only be called if there is no sync source.\n");
    *current = 0xdeadbeef;
    return S_OK;
}

static HRESULT WINAPI testseek_ConvertTimeFormat(IMediaSeeking *iface, LONGLONG *target,
    const GUID *target_format, LONGLONG source, const GUID *source_format)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI testseek_SetPositions(IMediaSeeking *iface, LONGLONG *current,
    DWORD current_flags, LONGLONG *stop, DWORD stop_flags )
{
    struct testfilter *filter = impl_from_IMediaSeeking(iface);
    if (winetest_debug > 1) trace("%p->SetPositions(%s, %#lx, %s, %#lx)\n",
            iface, wine_dbgstr_longlong(*current), current_flags, wine_dbgstr_longlong(*stop), stop_flags);
    ok(filter->state != State_Running, "Filter should be paused or stopped while seeking.\n");
    filter->seek_current = *current;
    filter->seek_stop = *stop;
    *current = 12340000;
    *stop = 43210000;
    return filter->seek_hr;
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
    struct testfilter *filter = impl_from_IMediaSeeking(iface);
    if (winetest_debug > 1) trace("%p->SetRate(%.16e)\n", iface, rate);
    filter->seek_rate = rate;
    return S_OK;
}

static HRESULT WINAPI testseek_GetRate(IMediaSeeking *iface, double *rate)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI testseek_GetPreroll(IMediaSeeking *iface, LONGLONG *preroll)
{
    if (winetest_debug > 1) trace("%p->GetPreroll()\n", iface);
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
    testseek_GetPreroll,
};

static struct testfilter *impl_from_IMediaPosition(IMediaPosition *iface)
{
    return CONTAINING_RECORD(iface, struct testfilter, IMediaPosition_iface);
}

static HRESULT WINAPI testpos_QueryInterface(IMediaPosition *iface, REFIID iid, void **out)
{
    struct testfilter *filter = impl_from_IMediaPosition(iface);
    return IBaseFilter_QueryInterface(&filter->IBaseFilter_iface, iid, out);
}

static ULONG WINAPI testpos_AddRef(IMediaPosition *iface)
{
    struct testfilter *filter = impl_from_IMediaPosition(iface);
    return IBaseFilter_AddRef(&filter->IBaseFilter_iface);
}

static ULONG WINAPI testpos_Release(IMediaPosition *iface)
{
    struct testfilter *filter = impl_from_IMediaPosition(iface);
    return IBaseFilter_Release(&filter->IBaseFilter_iface);
}

static HRESULT WINAPI testpos_GetTypeInfoCount(IMediaPosition *iface, UINT *count)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI testpos_GetTypeInfo(IMediaPosition *iface, UINT index, LCID lcid, ITypeInfo **typeinfo)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI testpos_GetIDsOfNames(IMediaPosition *iface, REFIID riid, LPOLESTR *names, UINT count, LCID lcid, DISPID *ids)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI testpos_Invoke(IMediaPosition *iface, DISPID id, REFIID iid, LCID lcid, WORD flags, DISPPARAMS *params, VARIANT *result, EXCEPINFO *excepinfo, UINT *error_arg)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI testpos_get_Duration(IMediaPosition *iface, REFTIME *length)
{
    if (winetest_debug > 1) trace("%p->get_Duration()\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI testpos_put_CurrentPosition(IMediaPosition *iface, REFTIME time)
{
    if (winetest_debug > 1) trace("%p->put_CurrentPosition(%f)\n", iface, time);
    return E_NOTIMPL;
}

static HRESULT WINAPI testpos_get_CurrentPosition(IMediaPosition *iface, REFTIME *time)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI testpos_get_StopTime(IMediaPosition *iface, REFTIME *time)
{
    if (winetest_debug > 1) trace("%p->get_StopTime()\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI testpos_put_StopTime(IMediaPosition *iface, REFTIME time)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI testpos_get_PrerollTime(IMediaPosition *iface, REFTIME *time)
{
    if (winetest_debug > 1) trace("%p->get_PrerollTime()\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI testpos_put_PrerollTime(IMediaPosition *iface, REFTIME time)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI testpos_put_Rate(IMediaPosition *iface, double rate)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI testpos_get_Rate(IMediaPosition *iface, double *rate)
{
    if (winetest_debug > 1) trace("%p->get_Rate()\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI testpos_CanSeekForward(IMediaPosition *iface, LONG *can_seek_forward)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI testpos_CanSeekBackward(IMediaPosition *iface, LONG *can_seek_backward)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static const IMediaPositionVtbl testpos_vtbl =
{
    testpos_QueryInterface,
    testpos_AddRef,
    testpos_Release,
    testpos_GetTypeInfoCount,
    testpos_GetTypeInfo,
    testpos_GetIDsOfNames,
    testpos_Invoke,
    testpos_get_Duration,
    testpos_put_CurrentPosition,
    testpos_get_CurrentPosition,
    testpos_get_StopTime,
    testpos_put_StopTime,
    testpos_get_PrerollTime,
    testpos_put_PrerollTime,
    testpos_put_Rate,
    testpos_get_Rate,
    testpos_CanSeekForward,
    testpos_CanSeekBackward,
};

static struct testfilter *impl_from_IReferenceClock(IReferenceClock *iface)
{
    return CONTAINING_RECORD(iface, struct testfilter, IReferenceClock_iface);
}

static HRESULT WINAPI testclock_QueryInterface(IReferenceClock *iface, REFIID iid, void **out)
{
    struct testfilter *filter = impl_from_IReferenceClock(iface);
    return IBaseFilter_QueryInterface(&filter->IBaseFilter_iface, iid, out);
}

static ULONG WINAPI testclock_AddRef(IReferenceClock *iface)
{
    struct testfilter *filter = impl_from_IReferenceClock(iface);
    return InterlockedIncrement(&filter->ref);
}

static ULONG WINAPI testclock_Release(IReferenceClock *iface)
{
    struct testfilter *filter = impl_from_IReferenceClock(iface);
    return InterlockedDecrement(&filter->ref);
}

static HRESULT WINAPI testclock_GetTime(IReferenceClock *iface, REFERENCE_TIME *time)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI testclock_AdviseTime(IReferenceClock *iface,
        REFERENCE_TIME base, REFERENCE_TIME offset, HEVENT event, DWORD_PTR *cookie)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI testclock_AdvisePeriodic(IReferenceClock *iface,
        REFERENCE_TIME start, REFERENCE_TIME period, HSEMAPHORE semaphore, DWORD_PTR *cookie)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI testclock_Unadvise(IReferenceClock *iface, DWORD_PTR cookie)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static const IReferenceClockVtbl testclock_vtbl =
{
    testclock_QueryInterface,
    testclock_AddRef,
    testclock_Release,
    testclock_GetTime,
    testclock_AdviseTime,
    testclock_AdvisePeriodic,
    testclock_Unadvise,
};

static struct testfilter *impl_from_IFileSourceFilter(IFileSourceFilter *iface)
{
    return CONTAINING_RECORD(iface, struct testfilter, IFileSourceFilter_iface);
}

static HRESULT WINAPI testfilesource_QueryInterface(IFileSourceFilter *iface, REFIID iid, void **out)
{
    struct testfilter *filter = impl_from_IFileSourceFilter(iface);
    return IBaseFilter_QueryInterface(&filter->IBaseFilter_iface, iid, out);
}

static ULONG WINAPI testfilesource_AddRef(IFileSourceFilter *iface)
{
    struct testfilter *filter = impl_from_IFileSourceFilter(iface);
    return InterlockedIncrement(&filter->ref);
}

static ULONG WINAPI testfilesource_Release(IFileSourceFilter *iface)
{
    struct testfilter *filter = impl_from_IFileSourceFilter(iface);
    return InterlockedDecrement(&filter->ref);
}

static HRESULT WINAPI testfilesource_Load(IFileSourceFilter *iface,
        const WCHAR *filename, const AM_MEDIA_TYPE *mt)
{
    if (winetest_debug > 1) trace("%p->Load(%ls)\n", iface, filename);
    return S_OK;
}

static HRESULT WINAPI testfilesource_GetCurFile(IFileSourceFilter *iface,
        WCHAR **filename, AM_MEDIA_TYPE *mt)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static const IFileSourceFilterVtbl testfilesource_vtbl =
{
    testfilesource_QueryInterface,
    testfilesource_AddRef,
    testfilesource_Release,
    testfilesource_Load,
    testfilesource_GetCurFile,
};

struct testfilter_cf
{
    IClassFactory IClassFactory_iface;
    struct testfilter *filter;
};

static void testfilter_init(struct testfilter *filter, struct testpin *pins, int pin_count)
{
    unsigned int i;

    memset(filter, 0, sizeof(*filter));
    filter->IBaseFilter_iface.lpVtbl = &testfilter_vtbl;
    filter->IEnumPins_iface.lpVtbl = &testenumpins_vtbl;
    filter->ref = 1;
    filter->pins = pins;
    filter->pin_count = pin_count;
    for (i = 0; i < pin_count; i++)
        pins[i].filter = filter;

    filter->state = State_Stopped;
    filter->expect_stop_prev = filter->expect_run_prev = State_Paused;
    filter->support_media_time = TRUE;
    filter->time_format = TIME_FORMAT_MEDIA_TIME;
}

static HRESULT WINAPI testfilter_cf_QueryInterface(IClassFactory *iface, REFIID iid, void **out)
{
    if (IsEqualGUID(iid, &IID_IUnknown) || IsEqualGUID(iid, &IID_IClassFactory))
    {
        *out = iface;
        return S_OK;
    }

    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI testfilter_cf_AddRef(IClassFactory *iface)
{
    return 2;
}

static ULONG WINAPI testfilter_cf_Release(IClassFactory *iface)
{
    return 1;
}

static HRESULT WINAPI testfilter_cf_CreateInstance(IClassFactory *iface, IUnknown *outer, REFIID iid, void **out)
{
    struct testfilter_cf *factory = CONTAINING_RECORD(iface, struct testfilter_cf, IClassFactory_iface);

    return IBaseFilter_QueryInterface(&factory->filter->IBaseFilter_iface, iid, out);
}

static HRESULT WINAPI testfilter_cf_LockServer(IClassFactory *iface, BOOL lock)
{
    return E_NOTIMPL;
}

static IClassFactoryVtbl testfilter_cf_vtbl =
{
    testfilter_cf_QueryInterface,
    testfilter_cf_AddRef,
    testfilter_cf_Release,
    testfilter_cf_CreateInstance,
    testfilter_cf_LockServer,
};

static void test_graph_builder_render(void)
{
    static const GUID sink1_clsid = {0x12345678};
    static const GUID sink2_clsid = {0x87654321};
    AM_MEDIA_TYPE source_type = {{0}};
    struct testpin source_pin, sink1_pin, sink2_pin, parser_pins[2];
    struct testfilter source, sink1, sink2, parser;
    struct testfilter_cf sink1_cf = { {&testfilter_cf_vtbl}, &sink1 };
    struct testfilter_cf sink2_cf = { {&testfilter_cf_vtbl}, &sink2 };

    IFilterGraph2 *graph = create_graph();
    REGFILTERPINS2 regpins = {0};
    REGPINTYPES regtypes = {0};
    REGFILTER2 regfilter = {0};
    IFilterMapper2 *mapper;
    DWORD cookie1, cookie2;
    HRESULT hr;
    ULONG ref;

    memset(&source_type.majortype, 0xcc, sizeof(GUID));
    testsource_init(&source_pin, &source_type, 1);
    testfilter_init(&source, &source_pin, 1);
    testsink_init(&sink1_pin);
    testfilter_init(&sink1, &sink1_pin, 1);
    testsink_init(&sink2_pin);
    testfilter_init(&sink2, &sink2_pin, 1);
    testsink_init(&parser_pins[0]);
    testsource_init(&parser_pins[1], &source_type, 1);
    testfilter_init(&parser, parser_pins, 2);

    IFilterGraph2_AddFilter(graph, &source.IBaseFilter_iface, NULL);
    IFilterGraph2_AddFilter(graph, &sink1.IBaseFilter_iface, NULL);
    IFilterGraph2_AddFilter(graph, &sink2.IBaseFilter_iface, NULL);

    hr = IFilterGraph2_Render(graph, &source_pin.IPin_iface);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(source_pin.peer == &sink2_pin.IPin_iface, "Got peer %p.\n", source_pin.peer);
    IFilterGraph2_Disconnect(graph, source_pin.peer);
    IFilterGraph2_Disconnect(graph, &source_pin.IPin_iface);

    IFilterGraph2_RemoveFilter(graph, &sink1.IBaseFilter_iface);
    IFilterGraph2_AddFilter(graph, &sink1.IBaseFilter_iface, NULL);

    hr = IFilterGraph2_Render(graph, &source_pin.IPin_iface);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(source_pin.peer == &sink1_pin.IPin_iface, "Got peer %p.\n", source_pin.peer);

    IFilterGraph2_Disconnect(graph, &source_pin.IPin_iface);
    IFilterGraph2_Disconnect(graph, &sink1_pin.IPin_iface);

    /* No preference is given to smaller chains. */

    IFilterGraph2_AddFilter(graph, &parser.IBaseFilter_iface, NULL);

    hr = IFilterGraph2_Render(graph, &source_pin.IPin_iface);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(source_pin.peer == &parser_pins[0].IPin_iface, "Got peer %p.\n", source_pin.peer);
    ok(parser_pins[1].peer == &sink1_pin.IPin_iface, "Got peer %p.\n", parser_pins[1].peer);
    IFilterGraph2_Disconnect(graph, source_pin.peer);
    IFilterGraph2_Disconnect(graph, &source_pin.IPin_iface);
    IFilterGraph2_Disconnect(graph, parser_pins[0].peer);
    IFilterGraph2_Disconnect(graph, &parser_pins[0].IPin_iface);

    IFilterGraph2_RemoveFilter(graph, &sink1.IBaseFilter_iface);
    IFilterGraph2_AddFilter(graph, &sink1.IBaseFilter_iface, NULL);

    hr = IFilterGraph2_Render(graph, &source_pin.IPin_iface);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(source_pin.peer == &sink1_pin.IPin_iface, "Got peer %p.\n", source_pin.peer);
    IFilterGraph2_Disconnect(graph, source_pin.peer);
    IFilterGraph2_Disconnect(graph, &source_pin.IPin_iface);

    /* A pin whose name (not ID) begins with a tilde is not rendered. */

    IFilterGraph2_RemoveFilter(graph, &sink2.IBaseFilter_iface);
    IFilterGraph2_RemoveFilter(graph, &parser.IBaseFilter_iface);
    IFilterGraph2_AddFilter(graph, &parser.IBaseFilter_iface, NULL);

    parser_pins[1].name[0] = '~';
    hr = IFilterGraph2_Render(graph, &source_pin.IPin_iface);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(source_pin.peer == &parser_pins[0].IPin_iface, "Got peer %p.\n", source_pin.peer);
    ok(!parser_pins[1].peer, "Got peer %p.\n", parser_pins[1].peer);
    ok(!sink1_pin.peer, "Got peer %p.\n", sink1_pin.peer);
    IFilterGraph2_Disconnect(graph, source_pin.peer);
    IFilterGraph2_Disconnect(graph, &source_pin.IPin_iface);

    parser_pins[1].name[0] = 0;
    parser_pins[1].id[0] = '~';
    hr = IFilterGraph2_Render(graph, &source_pin.IPin_iface);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(source_pin.peer == &parser_pins[0].IPin_iface, "Got peer %p.\n", source_pin.peer);
    ok(parser_pins[1].peer == &sink1_pin.IPin_iface, "Got peer %p.\n", parser_pins[1].peer);
    IFilterGraph2_Disconnect(graph, source_pin.peer);
    IFilterGraph2_Disconnect(graph, &source_pin.IPin_iface);

    ref = IFilterGraph2_Release(graph);
    ok(!ref, "Got outstanding refcount %ld.\n", ref);

    /* Test enumeration of filters from the registry. */

    CoRegisterClassObject(&sink1_clsid, (IUnknown *)&sink1_cf.IClassFactory_iface,
            CLSCTX_INPROC_SERVER, REGCLS_MULTIPLEUSE, &cookie1);
    CoRegisterClassObject(&sink2_clsid, (IUnknown *)&sink2_cf.IClassFactory_iface,
            CLSCTX_INPROC_SERVER, REGCLS_MULTIPLEUSE, &cookie2);

    CoCreateInstance(&CLSID_FilterMapper2, NULL, CLSCTX_INPROC_SERVER,
            &IID_IFilterMapper2, (void **)&mapper);

    regfilter.dwVersion = 2;
    regfilter.dwMerit = MERIT_UNLIKELY;
    regfilter.cPins2 = 1;
    regfilter.rgPins2 = &regpins;
    regpins.dwFlags = 0;
    regpins.cInstances = 1;
    regpins.nMediaTypes = 1;
    regpins.lpMediaType = &regtypes;
    regtypes.clsMajorType = &source_type.majortype;
    regtypes.clsMinorType = &MEDIASUBTYPE_NULL;
    hr = IFilterMapper2_RegisterFilter(mapper, &sink1_clsid, L"test", NULL, NULL, NULL, &regfilter);
    if (hr == E_ACCESSDENIED)
    {
        skip("Not enough permission to register filters.\n");
        goto out;
    }
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    graph = create_graph();
    IFilterGraph2_AddFilter(graph, &source.IBaseFilter_iface, NULL);

    regpins.dwFlags = REG_PINFLAG_B_RENDERER;
    IFilterMapper2_RegisterFilter(mapper, &sink2_clsid, L"test", NULL, NULL, NULL, &regfilter);

    hr = IFilterGraph2_Render(graph, &source_pin.IPin_iface);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(source_pin.peer == &sink2_pin.IPin_iface || source_pin.peer == &sink1_pin.IPin_iface,
            "Got peer %p.\n", source_pin.peer);

    ref = IFilterGraph2_Release(graph);
    ok(!ref, "Got outstanding refcount %ld.\n", ref);

    /* Preference is given to filters already in the graph. */

    graph = create_graph();
    IFilterGraph2_AddFilter(graph, &source.IBaseFilter_iface, NULL);
    IFilterGraph2_AddFilter(graph, &sink2.IBaseFilter_iface, NULL);

    hr = IFilterGraph2_Render(graph, &source_pin.IPin_iface);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(source_pin.peer == &sink2_pin.IPin_iface, "Got peer %p.\n", source_pin.peer);

    ref = IFilterGraph2_Release(graph);
    ok(!ref, "Got outstanding refcount %ld.\n", ref);

    /* No preference is given to renderer filters. */

    graph = create_graph();
    IFilterGraph2_AddFilter(graph, &source.IBaseFilter_iface, NULL);

    IFilterMapper2_UnregisterFilter(mapper, NULL, NULL, &sink1_clsid);
    IFilterMapper2_UnregisterFilter(mapper, NULL, NULL, &sink2_clsid);

    IFilterMapper2_RegisterFilter(mapper, &sink1_clsid, L"test", NULL, NULL, NULL, &regfilter);
    regpins.dwFlags = 0;
    IFilterMapper2_RegisterFilter(mapper, &sink2_clsid, L"test", NULL, NULL, NULL, &regfilter);

    hr = IFilterGraph2_Render(graph, &source_pin.IPin_iface);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(source_pin.peer == &sink2_pin.IPin_iface || source_pin.peer == &sink1_pin.IPin_iface,
            "Got peer %p.\n", source_pin.peer);

    ref = IFilterGraph2_Release(graph);
    ok(!ref, "Got outstanding refcount %ld.\n", ref);

    /* Preference is given to filters with higher merit. */

    graph = create_graph();
    IFilterGraph2_AddFilter(graph, &source.IBaseFilter_iface, NULL);

    IFilterMapper2_UnregisterFilter(mapper, NULL, NULL, &sink1_clsid);
    IFilterMapper2_UnregisterFilter(mapper, NULL, NULL, &sink2_clsid);

    regfilter.dwMerit = MERIT_UNLIKELY;
    IFilterMapper2_RegisterFilter(mapper, &sink1_clsid, L"test", NULL, NULL, NULL, &regfilter);
    regfilter.dwMerit = MERIT_PREFERRED;
    IFilterMapper2_RegisterFilter(mapper, &sink2_clsid, L"test", NULL, NULL, NULL, &regfilter);

    hr = IFilterGraph2_Render(graph, &source_pin.IPin_iface);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(source_pin.peer == &sink2_pin.IPin_iface, "Got peer %p.\n", source_pin.peer);

    ref = IFilterGraph2_Release(graph);
    ok(!ref, "Got outstanding refcount %ld.\n", ref);

    graph = create_graph();
    IFilterGraph2_AddFilter(graph, &source.IBaseFilter_iface, NULL);

    IFilterMapper2_UnregisterFilter(mapper, NULL, NULL, &sink1_clsid);
    IFilterMapper2_UnregisterFilter(mapper, NULL, NULL, &sink2_clsid);

    regfilter.dwMerit = MERIT_PREFERRED;
    IFilterMapper2_RegisterFilter(mapper, &sink1_clsid, L"test", NULL, NULL, NULL, &regfilter);
    regfilter.dwMerit = MERIT_UNLIKELY;
    IFilterMapper2_RegisterFilter(mapper, &sink2_clsid, L"test", NULL, NULL, NULL, &regfilter);

    hr = IFilterGraph2_Render(graph, &source_pin.IPin_iface);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(source_pin.peer == &sink1_pin.IPin_iface, "Got peer %p.\n", source_pin.peer);

    ref = IFilterGraph2_Release(graph);
    ok(!ref, "Got outstanding refcount %ld.\n", ref);

    /* Test AM_RENDEREX_RENDERTOEXISTINGRENDERERS. */

    graph = create_graph();
    IFilterGraph2_AddFilter(graph, &source.IBaseFilter_iface, NULL);

    hr = IFilterGraph2_RenderEx(graph, &source_pin.IPin_iface, AM_RENDEREX_RENDERTOEXISTINGRENDERERS, NULL);
    ok(hr == VFW_E_CANNOT_RENDER, "Got hr %#lx.\n", hr);

    IFilterGraph2_AddFilter(graph, &sink1.IBaseFilter_iface, NULL);

    hr = IFilterGraph2_RenderEx(graph, &source_pin.IPin_iface, AM_RENDEREX_RENDERTOEXISTINGRENDERERS, NULL);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(source_pin.peer == &sink1_pin.IPin_iface, "Got peer %p.\n", source_pin.peer);

    ref = IFilterGraph2_Release(graph);
    ok(!ref, "Got outstanding refcount %ld.\n", ref);

    IFilterMapper2_UnregisterFilter(mapper, NULL, NULL, &sink1_clsid);
    IFilterMapper2_UnregisterFilter(mapper, NULL, NULL, &sink2_clsid);

out:
    CoRevokeClassObject(cookie1);
    CoRevokeClassObject(cookie2);
    IFilterMapper2_Release(mapper);
    ok(source.ref == 1, "Got outstanding refcount %ld.\n", source.ref);
    ok(source_pin.ref == 1, "Got outstanding refcount %ld.\n", source_pin.ref);
    ok(sink1.ref == 1, "Got outstanding refcount %ld.\n", sink1.ref);
    ok(sink1_pin.ref == 1, "Got outstanding refcount %ld.\n", sink1_pin.ref);
    ok(sink2.ref == 1, "Got outstanding refcount %ld.\n", sink2.ref);
    ok(sink2_pin.ref == 1, "Got outstanding refcount %ld.\n", sink2_pin.ref);
    ok(parser.ref == 1, "Got outstanding refcount %ld.\n", parser.ref);
    ok(parser_pins[0].ref == 1, "Got outstanding refcount %ld.\n", parser_pins[0].ref);
    ok(parser_pins[1].ref == 1, "Got outstanding refcount %ld.\n", parser_pins[1].ref);
}

static void test_graph_builder_connect(void)
{
    static const GUID parser1_clsid = {0x12345678};
    static const GUID parser2_clsid = {0x87654321};
    AM_MEDIA_TYPE source_types[2] = {{{0}}}, sink_type = {{0}}, parser3_type = {{0}};
    struct testpin source_pin, sink_pin, sink2_pin, parser1_pins[3], parser2_pins[2], parser3_pins[2];
    struct testfilter source, sink, sink2, parser1, parser2, parser3;
    struct testfilter_cf parser1_cf = { {&testfilter_cf_vtbl}, &parser1 };
    struct testfilter_cf parser2_cf = { {&testfilter_cf_vtbl}, &parser2 };

    IFilterGraph2 *graph = create_graph();
    REGFILTERPINS2 regpins[2] = {{0}};
    REGPINTYPES regtypes = {0};
    REGFILTER2 regfilter = {0};
    IFilterMapper2 *mapper;
    DWORD cookie1, cookie2;
    HRESULT hr;
    ULONG ref;

    memset(&source_types[0].majortype, 0xcc, sizeof(GUID));
    memset(&source_types[1].majortype, 0xdd, sizeof(GUID));
    memset(&sink_type.majortype, 0x66, sizeof(GUID));
    testsource_init(&source_pin, source_types, 2);
    source_pin.request_mt = &source_types[1];
    testfilter_init(&source, &source_pin, 1);
    testsink_init(&sink_pin);
    testfilter_init(&sink, &sink_pin, 1);
    testsink_init(&sink2_pin);
    testfilter_init(&sink2, &sink2_pin, 1);

    testsink_init(&parser1_pins[0]);
    testsource_init(&parser1_pins[1], &sink_type, 1);
    parser1_pins[1].request_mt = &sink_type;
    testsource_init(&parser1_pins[2], &sink_type, 1);
    parser1_pins[2].request_mt = &sink_type;
    testfilter_init(&parser1, parser1_pins, 3);
    parser1.pin_count = 2;

    testsource_init(&parser2_pins[0], &sink_type, 1);
    testsink_init(&parser2_pins[1]);
    parser2_pins[0].request_mt = &sink_type;
    testfilter_init(&parser2, parser2_pins, 2);

    testsink_init(&parser3_pins[0]);
    testsource_init(&parser3_pins[1], &sink_type, 1);
    parser3_pins[1].request_mt = &parser3_type;
    testfilter_init(&parser3, parser3_pins, 2);

    IFilterGraph2_AddFilter(graph, &source.IBaseFilter_iface, NULL);
    IFilterGraph2_AddFilter(graph, &sink.IBaseFilter_iface, NULL);

    hr = IFilterGraph2_Connect(graph, &source_pin.IPin_iface, &sink_pin.IPin_iface);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(source_pin.peer == &sink_pin.IPin_iface, "Got peer %p.\n", source_pin.peer);
    IFilterGraph2_Disconnect(graph, source_pin.peer);
    IFilterGraph2_Disconnect(graph, &source_pin.IPin_iface);

    for (source_pin.Connect_hr = 0x00040200; source_pin.Connect_hr <= 0x000402ff;
            ++source_pin.Connect_hr)
    {
        hr = IFilterGraph2_Connect(graph, &source_pin.IPin_iface, &sink_pin.IPin_iface);
        ok(hr == source_pin.Connect_hr, "Got hr %#lx for Connect() hr %#lx.\n",
                hr, source_pin.Connect_hr);
        ok(source_pin.peer == &sink_pin.IPin_iface, "Got peer %p.\n", source_pin.peer);
        IFilterGraph2_Disconnect(graph, source_pin.peer);
        IFilterGraph2_Disconnect(graph, &source_pin.IPin_iface);
    }
    source_pin.Connect_hr = S_OK;

    sink_pin.accept_mt = &sink_type;
    hr = IFilterGraph2_Connect(graph, &source_pin.IPin_iface, &sink_pin.IPin_iface);
    ok(hr == VFW_E_CANNOT_CONNECT, "Got hr %#lx.\n", hr);
    ok(!source_pin.peer, "Got peer %p.\n", source_pin.peer);

    for (source_pin.Connect_hr = 0x80040200; source_pin.Connect_hr <= 0x800402ff;
            ++source_pin.Connect_hr)
    {
        hr = IFilterGraph2_Connect(graph, &source_pin.IPin_iface, &sink_pin.IPin_iface);
        if (source_pin.Connect_hr == VFW_E_NOT_CONNECTED
                || source_pin.Connect_hr == VFW_E_NO_AUDIO_HARDWARE)
            ok(hr == source_pin.Connect_hr, "Got hr %#lx for Connect() hr %#lx.\n",
                    hr, source_pin.Connect_hr);
        else
            ok(hr == VFW_E_CANNOT_CONNECT, "Got hr %#lx for Connect() hr %#lx.\n",
                    hr, source_pin.Connect_hr);
        ok(!source_pin.peer, "Got peer %p.\n", source_pin.peer);
        ok(!sink_pin.peer, "Got peer %p.\n", sink_pin.peer);
    }
    source_pin.Connect_hr = S_OK;

    for (source_pin.EnumMediaTypes_hr = 0x80040200; source_pin.EnumMediaTypes_hr <= 0x800402ff;
            ++source_pin.EnumMediaTypes_hr)
    {
        hr = IFilterGraph2_Connect(graph, &source_pin.IPin_iface, &sink_pin.IPin_iface);
        ok(hr == source_pin.EnumMediaTypes_hr, "Got hr %#lx for EnumMediaTypes() hr %#lx.\n",
                hr, source_pin.EnumMediaTypes_hr);
        ok(!source_pin.peer, "Got peer %p.\n", source_pin.peer);
        ok(!sink_pin.peer, "Got peer %p.\n", sink_pin.peer);
    }
    source_pin.EnumMediaTypes_hr = S_OK;

    /* Test usage of intermediate filters. Similarly to Render(), filters are
     * simply tried in enumeration order. */

    IFilterGraph2_AddFilter(graph, &parser1.IBaseFilter_iface, NULL);
    IFilterGraph2_AddFilter(graph, &parser2.IBaseFilter_iface, NULL);

    sink_pin.accept_mt = NULL;
    hr = IFilterGraph2_Connect(graph, &source_pin.IPin_iface, &sink_pin.IPin_iface);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(source_pin.peer == &sink_pin.IPin_iface, "Got peer %p.\n", source_pin.peer);
    IFilterGraph2_Disconnect(graph, source_pin.peer);
    IFilterGraph2_Disconnect(graph, &source_pin.IPin_iface);

    sink_pin.accept_mt = &sink_type;
    hr = IFilterGraph2_Connect(graph, &source_pin.IPin_iface, &sink_pin.IPin_iface);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(source_pin.peer == &parser2_pins[1].IPin_iface, "Got peer %p.\n", source_pin.peer);
    ok(sink_pin.peer == &parser2_pins[0].IPin_iface, "Got peer %p.\n", sink_pin.peer);
    IFilterGraph2_Disconnect(graph, source_pin.peer);
    IFilterGraph2_Disconnect(graph, &source_pin.IPin_iface);
    IFilterGraph2_Disconnect(graph, sink_pin.peer);
    IFilterGraph2_Disconnect(graph, &sink_pin.IPin_iface);

    for (source_pin.Connect_hr = 0x00040200; source_pin.Connect_hr <= 0x000402ff;
            ++source_pin.Connect_hr)
    {
        hr = IFilterGraph2_Connect(graph, &source_pin.IPin_iface, &sink_pin.IPin_iface);
        ok(hr == S_OK, "Got hr %#lx for Connect() hr %#lx.\n", hr, source_pin.Connect_hr);
        ok(source_pin.peer == &parser2_pins[1].IPin_iface, "Got peer %p.\n", source_pin.peer);
        ok(sink_pin.peer == &parser2_pins[0].IPin_iface, "Got peer %p.\n", sink_pin.peer);
        IFilterGraph2_Disconnect(graph, source_pin.peer);
        IFilterGraph2_Disconnect(graph, &source_pin.IPin_iface);
        IFilterGraph2_Disconnect(graph, sink_pin.peer);
        IFilterGraph2_Disconnect(graph, &sink_pin.IPin_iface);
    }
    source_pin.Connect_hr = S_OK;

    IFilterGraph2_RemoveFilter(graph, &parser1.IBaseFilter_iface);
    IFilterGraph2_AddFilter(graph, &parser1.IBaseFilter_iface, NULL);

    hr = IFilterGraph2_Connect(graph, &source_pin.IPin_iface, &sink_pin.IPin_iface);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(source_pin.peer == &parser1_pins[0].IPin_iface, "Got peer %p.\n", source_pin.peer);
    ok(sink_pin.peer == &parser1_pins[1].IPin_iface, "Got peer %p.\n", sink_pin.peer);
    IFilterGraph2_Disconnect(graph, source_pin.peer);
    IFilterGraph2_Disconnect(graph, &source_pin.IPin_iface);
    IFilterGraph2_Disconnect(graph, sink_pin.peer);
    IFilterGraph2_Disconnect(graph, &sink_pin.IPin_iface);

    /* No preference is given to smaller chains. */

    IFilterGraph2_AddFilter(graph, &parser3.IBaseFilter_iface, NULL);
    hr = IFilterGraph2_Connect(graph, &source_pin.IPin_iface, &sink_pin.IPin_iface);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(source_pin.peer == &parser3_pins[0].IPin_iface, "Got peer %p.\n", source_pin.peer);
    ok(parser3_pins[1].peer == &parser1_pins[0].IPin_iface, "Got peer %p.\n", parser3_pins[1].peer);
    ok(sink_pin.peer == &parser1_pins[1].IPin_iface, "Got peer %p.\n", sink_pin.peer);
    IFilterGraph2_Disconnect(graph, source_pin.peer);
    IFilterGraph2_Disconnect(graph, &source_pin.IPin_iface);
    IFilterGraph2_Disconnect(graph, parser3_pins[0].peer);
    IFilterGraph2_Disconnect(graph, &parser3_pins[0].IPin_iface);
    IFilterGraph2_Disconnect(graph, sink_pin.peer);
    IFilterGraph2_Disconnect(graph, &sink_pin.IPin_iface);

    IFilterGraph2_RemoveFilter(graph, &parser3.IBaseFilter_iface);
    IFilterGraph2_RemoveFilter(graph, &parser2.IBaseFilter_iface);

    /* Extra source pins on an intermediate filter are not rendered. */

    IFilterGraph2_RemoveFilter(graph, &parser1.IBaseFilter_iface);
    parser1.pin_count = 3;
    IFilterGraph2_AddFilter(graph, &parser1.IBaseFilter_iface, NULL);
    IFilterGraph2_AddFilter(graph, &sink2.IBaseFilter_iface, NULL);

    hr = IFilterGraph2_Connect(graph, &source_pin.IPin_iface, &sink_pin.IPin_iface);
    todo_wine ok(hr == VFW_S_PARTIAL_RENDER, "Got hr %#lx.\n", hr);
    ok(source_pin.peer == &parser1_pins[0].IPin_iface, "Got peer %p.\n", source_pin.peer);
    ok(sink_pin.peer == &parser1_pins[1].IPin_iface, "Got peer %p.\n", sink_pin.peer);
    ok(!parser1_pins[2].peer, "Got peer %p.\n", parser1_pins[2].peer);
    IFilterGraph2_Disconnect(graph, source_pin.peer);
    IFilterGraph2_Disconnect(graph, &source_pin.IPin_iface);
    IFilterGraph2_Disconnect(graph, sink_pin.peer);
    IFilterGraph2_Disconnect(graph, &sink_pin.IPin_iface);
    parser1.pin_count = 2;

    /* QueryInternalConnections is not used to find output pins. */

    parser1_pins[1].QueryInternalConnections_hr = S_OK;
    hr = IFilterGraph2_Connect(graph, &source_pin.IPin_iface, &sink_pin.IPin_iface);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(source_pin.peer == &parser1_pins[0].IPin_iface, "Got peer %p.\n", source_pin.peer);
    ok(sink_pin.peer == &parser1_pins[1].IPin_iface, "Got peer %p.\n", sink_pin.peer);
    IFilterGraph2_Disconnect(graph, source_pin.peer);
    IFilterGraph2_Disconnect(graph, &source_pin.IPin_iface);
    IFilterGraph2_Disconnect(graph, sink_pin.peer);
    IFilterGraph2_Disconnect(graph, &sink_pin.IPin_iface);
    parser1_pins[1].QueryInternalConnections_hr = E_NOTIMPL;

    /* A pin whose name (not ID) begins with a tilde is not connected. */

    parser1_pins[1].name[0] = '~';
    hr = IFilterGraph2_Connect(graph, &source_pin.IPin_iface, &sink_pin.IPin_iface);
    ok(hr == VFW_E_CANNOT_CONNECT, "Got hr %#lx.\n", hr);
    ok(!source_pin.peer, "Got peer %p.\n", source_pin.peer);

    parser1.pin_count = 3;
    hr = IFilterGraph2_Connect(graph, &source_pin.IPin_iface, &sink_pin.IPin_iface);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(source_pin.peer == &parser1_pins[0].IPin_iface, "Got peer %p.\n", source_pin.peer);
    ok(sink_pin.peer == &parser1_pins[2].IPin_iface, "Got peer %p.\n", sink_pin.peer);
    IFilterGraph2_Disconnect(graph, source_pin.peer);
    IFilterGraph2_Disconnect(graph, &source_pin.IPin_iface);
    IFilterGraph2_Disconnect(graph, sink_pin.peer);
    IFilterGraph2_Disconnect(graph, &sink_pin.IPin_iface);
    parser1.pin_count = 2;

    parser1_pins[1].name[0] = 0;
    parser1_pins[1].id[0] = '~';
    hr = IFilterGraph2_Connect(graph, &source_pin.IPin_iface, &sink_pin.IPin_iface);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(source_pin.peer == &parser1_pins[0].IPin_iface, "Got peer %p.\n", source_pin.peer);
    ok(sink_pin.peer == &parser1_pins[1].IPin_iface, "Got peer %p.\n", sink_pin.peer);
    IFilterGraph2_Disconnect(graph, source_pin.peer);
    IFilterGraph2_Disconnect(graph, &source_pin.IPin_iface);
    IFilterGraph2_Disconnect(graph, sink_pin.peer);
    IFilterGraph2_Disconnect(graph, &sink_pin.IPin_iface);

    hr = IFilterGraph2_Connect(graph, &parser1_pins[1].IPin_iface, &parser1_pins[0].IPin_iface);
    ok(hr == VFW_E_CANNOT_CONNECT, "Got hr %#lx.\n", hr);

    parser1_pins[0].QueryInternalConnections_hr = S_OK;
    hr = IFilterGraph2_Connect(graph, &parser1_pins[1].IPin_iface, &parser1_pins[0].IPin_iface);
    todo_wine ok(hr == S_OK, "Got hr %#lx.\n", hr);
    parser1_pins[0].QueryInternalConnections_hr = E_NOTIMPL;

    ref = IFilterGraph2_Release(graph);
    ok(!ref, "Got outstanding refcount %ld.\n", ref);

    /* The graph connects from source to sink, not from sink to source. */

    graph = create_graph();
    IFilterGraph2_AddFilter(graph, &source.IBaseFilter_iface, L"source");
    IFilterGraph2_AddFilter(graph, &parser1.IBaseFilter_iface, L"parser");
    IFilterGraph2_AddFilter(graph, &sink.IBaseFilter_iface, L"sink");

    parser1_pins[0].require_connected_pin = &parser1_pins[1];

    hr = IFilterGraph2_Connect(graph, &source_pin.IPin_iface, &sink_pin.IPin_iface);
    ok(hr == VFW_E_CANNOT_CONNECT, "Got hr %#lx.\n", hr);
    ok(!source_pin.peer, "Got peer %p.\n", source_pin.peer);
    ok(!sink_pin.peer, "Got peer %p.\n", sink_pin.peer);

    parser1_pins[0].require_connected_pin = NULL;
    parser1_pins[1].require_connected_pin = &parser1_pins[0];

    hr = IFilterGraph2_Connect(graph, &source_pin.IPin_iface, &sink_pin.IPin_iface);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(source_pin.peer == &parser1_pins[0].IPin_iface, "Got peer %p.\n", source_pin.peer);
    ok(sink_pin.peer == &parser1_pins[1].IPin_iface, "Got peer %p.\n", sink_pin.peer);

    parser1_pins[1].require_connected_pin = NULL;

    ref = IFilterGraph2_Release(graph);
    ok(!ref, "Got outstanding refcount %ld.\n", ref);

    /* Test enumeration of filters from the registry. */

    graph = create_graph();
    IFilterGraph2_AddFilter(graph, &source.IBaseFilter_iface, NULL);
    IFilterGraph2_AddFilter(graph, &sink.IBaseFilter_iface, NULL);

    CoRegisterClassObject(&parser1_clsid, (IUnknown *)&parser1_cf.IClassFactory_iface,
            CLSCTX_INPROC_SERVER, REGCLS_MULTIPLEUSE, &cookie1);
    CoRegisterClassObject(&parser2_clsid, (IUnknown *)&parser2_cf.IClassFactory_iface,
            CLSCTX_INPROC_SERVER, REGCLS_MULTIPLEUSE, &cookie2);

    CoCreateInstance(&CLSID_FilterMapper2, NULL, CLSCTX_INPROC_SERVER,
            &IID_IFilterMapper2, (void **)&mapper);

    regfilter.dwVersion = 2;
    regfilter.dwMerit = MERIT_UNLIKELY;
    regfilter.cPins2 = 2;
    regfilter.rgPins2 = regpins;
    regpins[0].dwFlags = 0;
    regpins[0].cInstances = 1;
    regpins[0].nMediaTypes = 1;
    regpins[0].lpMediaType = &regtypes;
    regpins[1].dwFlags = REG_PINFLAG_B_OUTPUT;
    regpins[1].cInstances = 1;
    regpins[1].nMediaTypes = 1;
    regpins[1].lpMediaType = &regtypes;
    regtypes.clsMajorType = &source_types[1].majortype;
    regtypes.clsMinorType = &MEDIASUBTYPE_NULL;
    hr = IFilterMapper2_RegisterFilter(mapper, &parser1_clsid, L"test", NULL, NULL, NULL, &regfilter);
    if (hr == E_ACCESSDENIED)
    {
        skip("Not enough permission to register filters.\n");
        goto out;
    }
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    IFilterMapper2_RegisterFilter(mapper, &parser2_clsid, L"test", NULL, NULL, NULL, &regfilter);

    hr = IFilterGraph2_Connect(graph, &source_pin.IPin_iface, &sink_pin.IPin_iface);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(source_pin.peer == &parser1_pins[0].IPin_iface
            || source_pin.peer == &parser2_pins[1].IPin_iface, "Got peer %p.\n", source_pin.peer);
    ok(sink_pin.peer == &parser1_pins[1].IPin_iface
            || sink_pin.peer == &parser2_pins[0].IPin_iface, "Got peer %p.\n", sink_pin.peer);

    ref = IFilterGraph2_Release(graph);
    ok(!ref, "Got outstanding refcount %ld.\n", ref);

    /* Preference is given to filters already in the graph. */

    graph = create_graph();
    IFilterGraph2_AddFilter(graph, &source.IBaseFilter_iface, NULL);
    IFilterGraph2_AddFilter(graph, &sink.IBaseFilter_iface, NULL);
    IFilterGraph2_AddFilter(graph, &parser1.IBaseFilter_iface, NULL);

    hr = IFilterGraph2_Connect(graph, &source_pin.IPin_iface, &sink_pin.IPin_iface);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(source_pin.peer == &parser1_pins[0].IPin_iface, "Got peer %p.\n", source_pin.peer);
    ok(sink_pin.peer == &parser1_pins[1].IPin_iface, "Got peer %p.\n", sink_pin.peer);

    ref = IFilterGraph2_Release(graph);
    ok(!ref, "Got outstanding refcount %ld.\n", ref);

    /* Preference is given to filters with higher merit. */

    graph = create_graph();
    IFilterGraph2_AddFilter(graph, &source.IBaseFilter_iface, NULL);
    IFilterGraph2_AddFilter(graph, &sink.IBaseFilter_iface, NULL);

    IFilterMapper2_UnregisterFilter(mapper, NULL, NULL, &parser1_clsid);
    IFilterMapper2_UnregisterFilter(mapper, NULL, NULL, &parser2_clsid);

    regfilter.dwMerit = MERIT_UNLIKELY;
    IFilterMapper2_RegisterFilter(mapper, &parser1_clsid, L"test", NULL, NULL, NULL, &regfilter);
    regfilter.dwMerit = MERIT_PREFERRED;
    IFilterMapper2_RegisterFilter(mapper, &parser2_clsid, L"test", NULL, NULL, NULL, &regfilter);

    hr = IFilterGraph2_Connect(graph, &source_pin.IPin_iface, &sink_pin.IPin_iface);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(source_pin.peer == &parser2_pins[1].IPin_iface, "Got peer %p.\n", source_pin.peer);
    ok(sink_pin.peer == &parser2_pins[0].IPin_iface, "Got peer %p.\n", sink_pin.peer);

    ref = IFilterGraph2_Release(graph);
    ok(!ref, "Got outstanding refcount %ld.\n", ref);

    graph = create_graph();
    IFilterGraph2_AddFilter(graph, &source.IBaseFilter_iface, NULL);
    IFilterGraph2_AddFilter(graph, &sink.IBaseFilter_iface, NULL);

    IFilterMapper2_UnregisterFilter(mapper, NULL, NULL, &parser1_clsid);
    IFilterMapper2_UnregisterFilter(mapper, NULL, NULL, &parser2_clsid);

    regfilter.dwMerit = MERIT_PREFERRED;
    IFilterMapper2_RegisterFilter(mapper, &parser1_clsid, L"test", NULL, NULL, NULL, &regfilter);
    regfilter.dwMerit = MERIT_UNLIKELY;
    IFilterMapper2_RegisterFilter(mapper, &parser2_clsid, L"test", NULL, NULL, NULL, &regfilter);

    hr = IFilterGraph2_Connect(graph, &source_pin.IPin_iface, &sink_pin.IPin_iface);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(source_pin.peer == &parser1_pins[0].IPin_iface, "Got peer %p.\n", source_pin.peer);
    ok(sink_pin.peer == &parser1_pins[1].IPin_iface, "Got peer %p.\n", sink_pin.peer);

    IFilterMapper2_UnregisterFilter(mapper, NULL, NULL, &parser1_clsid);
    IFilterMapper2_UnregisterFilter(mapper, NULL, NULL, &parser2_clsid);

out:
    CoRevokeClassObject(cookie1);
    CoRevokeClassObject(cookie2);
    IFilterMapper2_Release(mapper);
    ref = IFilterGraph2_Release(graph);
    ok(!ref, "Got outstanding refcount %ld.\n", ref);
    ok(source.ref == 1, "Got outstanding refcount %ld.\n", source.ref);
    ok(source_pin.ref == 1, "Got outstanding refcount %ld.\n", source_pin.ref);
    ok(sink.ref == 1, "Got outstanding refcount %ld.\n", sink.ref);
    ok(sink_pin.ref == 1, "Got outstanding refcount %ld.\n", sink_pin.ref);
    ok(parser1.ref == 1, "Got outstanding refcount %ld.\n", parser1.ref);
    ok(parser1_pins[0].ref == 1, "Got outstanding refcount %ld.\n", parser1_pins[0].ref);
    ok(parser1_pins[1].ref == 1, "Got outstanding refcount %ld.\n", parser1_pins[1].ref);
    ok(parser1_pins[2].ref == 1, "Got outstanding refcount %ld.\n", parser1_pins[2].ref);
    ok(parser2.ref == 1, "Got outstanding refcount %ld.\n", parser2.ref);
    ok(parser2_pins[0].ref == 1, "Got outstanding refcount %ld.\n", parser2_pins[0].ref);
    ok(parser2_pins[1].ref == 1, "Got outstanding refcount %ld.\n", parser2_pins[1].ref);
    ok(parser3.ref == 1, "Got outstanding refcount %ld.\n", parser3.ref);
    ok(parser3_pins[0].ref == 1, "Got outstanding refcount %ld.\n", parser3_pins[0].ref);
    ok(parser3_pins[1].ref == 1, "Got outstanding refcount %ld.\n", parser3_pins[1].ref);
}

static const GUID test_iid = {0x33333333};
static LONG outer_ref = 1;

static HRESULT WINAPI outer_QueryInterface(IUnknown *iface, REFIID iid, void **out)
{
    if (IsEqualGUID(iid, &IID_IUnknown)
            || IsEqualGUID(iid, &IID_IFilterGraph2)
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
    IFilterGraph2 *graph, *graph2;
    IFilterMapper2 *mapper;
    IUnknown *unk, *unk2;
    HRESULT hr;
    ULONG ref;

    graph = (IFilterGraph2 *)0xdeadbeef;
    hr = CoCreateInstance(&CLSID_FilterGraph, &test_outer, CLSCTX_INPROC_SERVER,
            &IID_IFilterGraph2, (void **)&graph);
    ok(hr == E_NOINTERFACE, "Got hr %#lx.\n", hr);
    ok(!graph, "Got interface %p.\n", graph);

    hr = CoCreateInstance(&CLSID_FilterGraph, &test_outer, CLSCTX_INPROC_SERVER,
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

    hr = IUnknown_QueryInterface(unk, &IID_IFilterGraph2, (void **)&graph);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IFilterGraph2_QueryInterface(graph, &IID_IUnknown, (void **)&unk2);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(unk2 == (IUnknown *)0xdeadbeef, "Got unexpected IUnknown %p.\n", unk2);

    hr = IFilterGraph2_QueryInterface(graph, &IID_IFilterGraph2, (void **)&graph2);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(graph2 == (IFilterGraph2 *)0xdeadbeef, "Got unexpected IFilterGraph2 %p.\n", graph2);

    hr = IUnknown_QueryInterface(unk, &test_iid, (void **)&unk2);
    ok(hr == E_NOINTERFACE, "Got hr %#lx.\n", hr);
    ok(!unk2, "Got unexpected IUnknown %p.\n", unk2);

    hr = IFilterGraph2_QueryInterface(graph, &test_iid, (void **)&unk2);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(unk2 == (IUnknown *)0xdeadbeef, "Got unexpected IUnknown %p.\n", unk2);

    IFilterGraph2_Release(graph);
    ref = IUnknown_Release(unk);
    ok(!ref, "Got unexpected refcount %ld.\n", ref);
    ok(outer_ref == 1, "Got unexpected refcount %ld.\n", outer_ref);

    /* Test the aggregated filter mapper. */

    graph = create_graph();

    ref = get_refcount(graph);
    ok(ref == 1, "Got unexpected refcount %ld.\n", ref);

    hr = IFilterGraph2_QueryInterface(graph, &IID_IFilterMapper2, (void **)&mapper);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    ref = get_refcount(graph);
    ok(ref == 2, "Got unexpected refcount %ld.\n", ref);
    ref = get_refcount(mapper);
    ok(ref == 2, "Got unexpected refcount %ld.\n", ref);

    hr = IFilterMapper2_QueryInterface(mapper, &IID_IFilterGraph2, (void **)&graph2);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(graph2 == graph, "Got unexpected IFilterGraph2 %p.\n", graph2);
    IFilterGraph2_Release(graph2);

    IFilterMapper2_Release(mapper);
    ref = IFilterGraph2_Release(graph);
    ok(!ref, "Got unexpected refcount %ld.\n", ref);
}

/* Test how methods from "control" interfaces (IBasicAudio, IBasicVideo,
 * IVideoWindow) are delegated to filters exposing those interfaces. */
static void test_control_delegation(void)
{
    IFilterGraph2 *graph = create_graph();
    IBasicAudio *audio, *filter_audio;
    IBaseFilter *renderer;
    IVideoWindow *window;
    IBasicVideo2 *video;
    ITypeInfo *typeinfo;
    unsigned int count;
    TYPEATTR *typeattr;
    HRESULT hr;
    LONG val;

    /* IBasicAudio */

    hr = IFilterGraph2_QueryInterface(graph, &IID_IBasicAudio, (void **)&audio);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IBasicAudio_GetTypeInfoCount(audio, &count);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(count == 1, "Got count %u.\n", count);

    hr = IBasicAudio_put_Volume(audio, -10);
    ok(hr == E_NOTIMPL, "Got hr %#lx.\n", hr);
    hr = IBasicAudio_get_Volume(audio, &val);
    ok(hr == E_NOTIMPL, "Got hr %#lx.\n", hr);
    hr = IBasicAudio_put_Balance(audio, 10);
    ok(hr == E_NOTIMPL, "Got hr %#lx.\n", hr);
    hr = IBasicAudio_get_Balance(audio, &val);
    ok(hr == E_NOTIMPL, "Got hr %#lx.\n", hr);

    hr = CoCreateInstance(&CLSID_DSoundRender, NULL, CLSCTX_INPROC_SERVER, &IID_IBaseFilter, (void **)&renderer);
    if (hr != VFW_E_NO_AUDIO_HARDWARE)
    {
        ok(hr == S_OK, "Got hr %#lx.\n", hr);

        hr = IFilterGraph2_AddFilter(graph, renderer, NULL);
        ok(hr == S_OK, "Got hr %#lx.\n", hr);

        hr = IBasicAudio_put_Volume(audio, -10);
        ok(hr == S_OK, "Got hr %#lx.\n", hr);
        hr = IBasicAudio_get_Volume(audio, &val);
        ok(hr == S_OK, "Got hr %#lx.\n", hr);
        ok(val == -10, "got %ld\n", val);
        hr = IBasicAudio_put_Balance(audio, 10);
        ok(hr == S_OK || hr == VFW_E_MONO_AUDIO_HW, "Got hr %#lx.\n", hr);
        hr = IBasicAudio_get_Balance(audio, &val);
        ok(hr == S_OK || hr == VFW_E_MONO_AUDIO_HW, "Got hr %#lx.\n", hr);
        if (hr == S_OK)
            ok(val == 10, "got balance %ld\n", val);

        hr = IBaseFilter_QueryInterface(renderer, &IID_IBasicAudio, (void **)&filter_audio);
        ok(hr == S_OK, "Got hr %#lx.\n", hr);

        hr = IBasicAudio_get_Volume(filter_audio, &val);
        ok(hr == S_OK, "Got hr %#lx.\n", hr);
        ok(val == -10, "got volume %ld\n", val);

        hr = IFilterGraph2_RemoveFilter(graph, renderer);
        ok(hr == S_OK, "Got hr %#lx.\n", hr);

        IBaseFilter_Release(renderer);
        IBasicAudio_Release(filter_audio);
    }

    hr = IBasicAudio_put_Volume(audio, -10);
    ok(hr == E_NOTIMPL, "Got hr %#lx.\n", hr);
    hr = IBasicAudio_get_Volume(audio, &val);
    ok(hr == E_NOTIMPL, "Got hr %#lx.\n", hr);
    hr = IBasicAudio_put_Balance(audio, 10);
    ok(hr == E_NOTIMPL, "Got hr %#lx.\n", hr);
    hr = IBasicAudio_get_Balance(audio, &val);
    ok(hr == E_NOTIMPL, "Got hr %#lx.\n", hr);

    IBasicAudio_Release(audio);

    /* IBasicVideo and IVideoWindow */

    hr = IFilterGraph2_QueryInterface(graph, &IID_IBasicVideo2, (void **)&video);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IFilterGraph2_QueryInterface(graph, &IID_IVideoWindow, (void **)&window);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    /* Unlike IBasicAudio, these return E_NOINTERFACE. */
    hr = IBasicVideo2_get_BitRate(video, &val);
    ok(hr == E_NOINTERFACE, "Got hr %#lx.\n", hr);

    hr = IBasicVideo2_GetTypeInfoCount(video, &count);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(count == 1, "Got count %u.\n", count);

    hr = IBasicVideo2_GetTypeInfo(video, 0, 0, &typeinfo);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = ITypeInfo_GetTypeAttr(typeinfo, &typeattr);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(typeattr->typekind == TKIND_DISPATCH, "Got kind %u.\n", typeattr->typekind);
    ok(IsEqualGUID(&typeattr->guid, &IID_IBasicVideo), "Got IID %s.\n", wine_dbgstr_guid(&typeattr->guid));
    ITypeInfo_ReleaseTypeAttr(typeinfo, typeattr);
    ITypeInfo_Release(typeinfo);

    hr = IVideoWindow_SetWindowForeground(window, OAFALSE);
    ok(hr == E_NOINTERFACE, "Got hr %#lx.\n", hr);

    hr = IVideoWindow_GetTypeInfoCount(window, &count);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(count == 1, "Got count %u.\n", count);

    hr = CoCreateInstance(&CLSID_VideoRenderer, NULL, CLSCTX_INPROC_SERVER, &IID_IBaseFilter, (void **)&renderer);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IFilterGraph2_AddFilter(graph, renderer, NULL);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IBasicVideo2_get_BitRate(video, &val);
    ok(hr == VFW_E_NOT_CONNECTED, "Got hr %#lx.\n", hr);
    hr = IVideoWindow_SetWindowForeground(window, OAFALSE);
    ok(hr == VFW_E_NOT_CONNECTED, "Got hr %#lx.\n", hr);

    val = 0xdeadbeef;
    hr = IVideoWindow_get_FullScreenMode(window, &val);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(val == OAFALSE, "Got fullscreen %lu\n", val);

    hr = IVideoWindow_put_FullScreenMode(window, OAFALSE);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);

    hr = IFilterGraph2_RemoveFilter(graph, renderer);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IBasicVideo2_get_BitRate(video, &val);
    ok(hr == E_NOINTERFACE, "Got hr %#lx.\n", hr);
    hr = IVideoWindow_SetWindowForeground(window, OAFALSE);
    ok(hr == E_NOINTERFACE, "Got hr %#lx.\n", hr);

    IBaseFilter_Release(renderer);
    IBasicVideo2_Release(video);
    IVideoWindow_Release(window);
    IFilterGraph2_Release(graph);
}

static void test_add_remove_filter(void)
{
    struct testfilter filter;

    IFilterGraph2 *graph = create_graph();
    IBaseFilter *ret_filter;
    HRESULT hr;
    LONG ref;

    testfilter_init(&filter, NULL, 0);

    hr = IFilterGraph2_FindFilterByName(graph, L"testid", &ret_filter);
    ok(hr == VFW_E_NOT_FOUND, "Got hr %#lx.\n", hr);
    ok(!ret_filter, "Got filter %p.\n", ret_filter);

    hr = IFilterGraph2_AddFilter(graph, &filter.IBaseFilter_iface, L"testid");
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(filter.graph == (IFilterGraph *)graph, "Got graph %p.\n", filter.graph);
    ok(!wcscmp(filter.name, L"testid"), "Got name %s.\n", wine_dbgstr_w(filter.name));

    hr = IFilterGraph2_FindFilterByName(graph, L"testid", &ret_filter);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(ret_filter == &filter.IBaseFilter_iface, "Got filter %p.\n", ret_filter);
    IBaseFilter_Release(ret_filter);

    hr = IFilterGraph2_RemoveFilter(graph, &filter.IBaseFilter_iface);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(!filter.graph, "Got graph %p.\n", filter.graph);
    ok(!filter.name, "Got name %s.\n", wine_dbgstr_w(filter.name));
    ok(!filter.clock, "Got clock %p,\n", filter.clock);
    ok(filter.ref == 1, "Got outstanding refcount %ld.\n", filter.ref);

    hr = IFilterGraph2_FindFilterByName(graph, L"testid", &ret_filter);
    ok(hr == VFW_E_NOT_FOUND, "Got hr %#lx.\n", hr);
    ok(!ret_filter, "Got filter %p.\n", ret_filter);

    hr = IFilterGraph2_AddFilter(graph, &filter.IBaseFilter_iface, NULL);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(filter.graph == (IFilterGraph *)graph, "Got graph %p.\n", filter.graph);
    ok(!wcscmp(filter.name, L"0001"), "Got name %s.\n", wine_dbgstr_w(filter.name));

    hr = IFilterGraph2_FindFilterByName(graph, L"0001", &ret_filter);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(ret_filter == &filter.IBaseFilter_iface, "Got filter %p.\n", ret_filter);
    IBaseFilter_Release(ret_filter);

    /* test releasing the filter graph while filters are still connected */
    ref = IFilterGraph2_Release(graph);
    ok(!ref, "Got outstanding refcount %ld.\n", ref);
    ok(!filter.graph, "Got graph %p.\n", filter.graph);
    ok(!filter.name, "Got name %s.\n", wine_dbgstr_w(filter.name));
    ok(!filter.clock, "Got clock %p.\n", filter.clock);
    ok(filter.ref == 1, "Got outstanding refcount %ld.\n", filter.ref);
}

static HRESULT WINAPI test_connect_direct_Connect(IPin *iface, IPin *peer, const AM_MEDIA_TYPE *mt)
{
    struct testpin *pin = impl_from_IPin(iface);
    if (winetest_debug > 1) trace("%p->Connect()\n", pin);

    pin->peer = peer;
    IPin_AddRef(peer);
    pin->mt = (AM_MEDIA_TYPE *)mt;
    return S_OK;
}

static const IPinVtbl test_connect_direct_vtbl =
{
    testpin_QueryInterface,
    testpin_AddRef,
    testpin_Release,
    test_connect_direct_Connect,
    no_ReceiveConnection,
    testpin_Disconnect,
    testpin_ConnectedTo,
    testpin_ConnectionMediaType,
    testpin_QueryPinInfo,
    testpin_QueryDirection,
    testpin_QueryId,
    testpin_QueryAccept,
    no_EnumMediaTypes,
    testpin_QueryInternalConnections,
    testpin_EndOfStream,
    testpin_BeginFlush,
    testpin_EndFlush,
    testpin_NewSegment
};

static void test_connect_direct_init(struct testpin *pin, PIN_DIRECTION dir)
{
    testpin_init(pin, &test_connect_direct_vtbl, dir);
}

static void test_connect_direct(void)
{
    struct testpin source_pin, sink_pin, parser1_pins[2], parser2_pins[2];
    struct testfilter source, sink, parser1, parser2;

    IFilterGraph2 *graph = create_graph();
    IMediaControl *control;
    AM_MEDIA_TYPE mt;
    HRESULT hr;
    ULONG ref;

    test_connect_direct_init(&source_pin, PINDIR_OUTPUT);
    testfilter_init(&source, &source_pin, 1);
    test_connect_direct_init(&sink_pin, PINDIR_INPUT);
    testfilter_init(&sink, &sink_pin, 1);
    test_connect_direct_init(&parser1_pins[0], PINDIR_INPUT);
    test_connect_direct_init(&parser1_pins[1], PINDIR_OUTPUT);
    testfilter_init(&parser1, parser1_pins, 2);
    test_connect_direct_init(&parser2_pins[0], PINDIR_INPUT);
    test_connect_direct_init(&parser2_pins[1], PINDIR_OUTPUT);
    testfilter_init(&parser2, parser2_pins, 2);

    hr = IFilterGraph2_AddFilter(graph, &source.IBaseFilter_iface, NULL);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IFilterGraph2_AddFilter(graph, &sink.IBaseFilter_iface, NULL);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    /* The filter graph does not prevent connection while it is running; only
     * individual filters do. */
    IFilterGraph2_QueryInterface(graph, &IID_IMediaControl, (void **)&control);
    hr = IMediaControl_Pause(control);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IFilterGraph2_ConnectDirect(graph, &source_pin.IPin_iface, &sink_pin.IPin_iface, NULL);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(source_pin.peer == &sink_pin.IPin_iface, "Got peer %p.\n", source_pin.peer);
    ok(!source_pin.mt, "Got mt %p.\n", source_pin.mt);
    ok(!sink_pin.peer, "Got peer %p.\n", sink_pin.peer);

    hr = IFilterGraph2_Disconnect(graph, &sink_pin.IPin_iface);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);
    ok(source_pin.peer == &sink_pin.IPin_iface, "Got peer %p.\n", source_pin.peer);
    ok(!sink_pin.peer, "Got peer %p.\n", sink_pin.peer);

    hr = IFilterGraph2_Disconnect(graph, &source_pin.IPin_iface);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(!source_pin.peer, "Got peer %p.\n", source_pin.peer);
    ok(!sink_pin.peer, "Got peer %p.\n", sink_pin.peer);

    hr = IFilterGraph2_Connect(graph, &source_pin.IPin_iface, &sink_pin.IPin_iface);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(source_pin.peer == &sink_pin.IPin_iface, "Got peer %p.\n", source_pin.peer);
    ok(!source_pin.mt, "Got mt %p.\n", source_pin.mt);
    ok(!sink_pin.peer, "Got peer %p.\n", sink_pin.peer);

    hr = IFilterGraph2_Disconnect(graph, &sink_pin.IPin_iface);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);
    ok(source_pin.peer == &sink_pin.IPin_iface, "Got peer %p.\n", source_pin.peer);
    ok(!sink_pin.peer, "Got peer %p.\n", sink_pin.peer);

    hr = IFilterGraph2_Disconnect(graph, &source_pin.IPin_iface);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(!source_pin.peer, "Got peer %p.\n", source_pin.peer);
    ok(!sink_pin.peer, "Got peer %p.\n", sink_pin.peer);

    /* Swap the pins when connecting. */
    hr = IFilterGraph2_ConnectDirect(graph, &sink_pin.IPin_iface, &source_pin.IPin_iface, NULL);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    todo_wine ok(sink_pin.peer == &source_pin.IPin_iface, "Got peer %p.\n", sink_pin.peer);
    ok(!sink_pin.mt, "Got mt %p.\n", sink_pin.mt);
    todo_wine ok(!source_pin.peer, "Got peer %p.\n", source_pin.peer);

    hr = IFilterGraph2_Disconnect(graph, &sink_pin.IPin_iface);
    todo_wine ok(hr == S_OK, "Got hr %#lx.\n", hr);
    todo_wine ok(!source_pin.peer, "Got peer %p.\n", source_pin.peer);
    ok(!sink_pin.peer, "Got peer %p.\n", sink_pin.peer);

    hr = IFilterGraph2_Connect(graph, &sink_pin.IPin_iface, &source_pin.IPin_iface);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    todo_wine ok(sink_pin.peer == &source_pin.IPin_iface, "Got peer %p.\n", sink_pin.peer);
    ok(!sink_pin.mt, "Got mt %p.\n", sink_pin.mt);
    todo_wine ok(!source_pin.peer, "Got peer %p.\n", source_pin.peer);

    hr = IFilterGraph2_Disconnect(graph, &sink_pin.IPin_iface);
    todo_wine ok(hr == S_OK, "Got hr %#lx.\n", hr);
    todo_wine ok(!source_pin.peer, "Got peer %p.\n", source_pin.peer);
    ok(!sink_pin.peer, "Got peer %p.\n", sink_pin.peer);

    /* Disconnect() does not disconnect the peer. */
    hr = IFilterGraph2_ConnectDirect(graph, &source_pin.IPin_iface, &sink_pin.IPin_iface, NULL);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(source_pin.peer == &sink_pin.IPin_iface, "Got peer %p.\n", source_pin.peer);
    ok(!source_pin.mt, "Got mt %p.\n", source_pin.mt);
    ok(!sink_pin.peer, "Got peer %p.\n", sink_pin.peer);

    sink_pin.peer = &source_pin.IPin_iface;
    IPin_AddRef(sink_pin.peer);

    hr = IFilterGraph2_Disconnect(graph, &sink_pin.IPin_iface);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(source_pin.peer == &sink_pin.IPin_iface, "Got peer %p.\n", source_pin.peer);
    ok(!sink_pin.peer, "Got peer %p.\n", sink_pin.peer);

    hr = IFilterGraph2_Disconnect(graph, &source_pin.IPin_iface);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(!source_pin.peer, "Got peer %p.\n", source_pin.peer);
    ok(!sink_pin.peer, "Got peer %p.\n", sink_pin.peer);

    /* Test specifying the media type. */
    hr = IFilterGraph2_ConnectDirect(graph, &source_pin.IPin_iface, &sink_pin.IPin_iface, &mt);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(source_pin.peer == &sink_pin.IPin_iface, "Got peer %p.\n", source_pin.peer);
    ok(source_pin.mt == &mt, "Got mt %p.\n", source_pin.mt);
    ok(!sink_pin.peer, "Got peer %p.\n", sink_pin.peer);
    hr = IFilterGraph2_Disconnect(graph, &source_pin.IPin_iface);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    /* Test Reconnect[Ex](). */

    hr = IFilterGraph2_Reconnect(graph, &source_pin.IPin_iface);
    todo_wine ok(hr == VFW_E_WRONG_STATE, "Got hr %#lx.\n", hr);
    hr = IFilterGraph2_Reconnect(graph, &sink_pin.IPin_iface);
    todo_wine ok(hr == VFW_E_WRONG_STATE, "Got hr %#lx.\n", hr);

    hr = IMediaControl_Stop(control);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IFilterGraph2_Reconnect(graph, &source_pin.IPin_iface);
    ok(hr == VFW_E_NOT_CONNECTED, "Got hr %#lx.\n", hr);
    hr = IFilterGraph2_Reconnect(graph, &sink_pin.IPin_iface);
    ok(hr == VFW_E_NOT_CONNECTED, "Got hr %#lx.\n", hr);

    hr = IFilterGraph2_ConnectDirect(graph, &source_pin.IPin_iface, &sink_pin.IPin_iface, &mt);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IFilterGraph2_Reconnect(graph, &source_pin.IPin_iface);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(source_pin.peer == &sink_pin.IPin_iface, "Got peer %p.\n", source_pin.peer);
    ok(!source_pin.mt, "Got mt %p.\n", source_pin.mt);
    ok(!sink_pin.peer, "Got peer %p.\n", sink_pin.peer);
    hr = IFilterGraph2_Disconnect(graph, &source_pin.IPin_iface);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IFilterGraph2_ConnectDirect(graph, &source_pin.IPin_iface, &sink_pin.IPin_iface, &mt);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    sink_pin.peer = &source_pin.IPin_iface;
    IPin_AddRef(sink_pin.peer);
    hr = IFilterGraph2_Reconnect(graph, &sink_pin.IPin_iface);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(source_pin.peer == &sink_pin.IPin_iface, "Got peer %p.\n", source_pin.peer);
    ok(!source_pin.mt, "Got mt %p.\n", source_pin.mt);
    ok(!sink_pin.peer, "Got peer %p.\n", sink_pin.peer);
    hr = IFilterGraph2_Disconnect(graph, &source_pin.IPin_iface);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IFilterGraph2_ReconnectEx(graph, &source_pin.IPin_iface, NULL);
    ok(hr == VFW_E_NOT_CONNECTED, "Got hr %#lx.\n", hr);
    hr = IFilterGraph2_ReconnectEx(graph, &sink_pin.IPin_iface, NULL);
    ok(hr == VFW_E_NOT_CONNECTED, "Got hr %#lx.\n", hr);

    hr = IFilterGraph2_ConnectDirect(graph, &source_pin.IPin_iface, &sink_pin.IPin_iface, &mt);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IFilterGraph2_ReconnectEx(graph, &source_pin.IPin_iface, NULL);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(source_pin.peer == &sink_pin.IPin_iface, "Got peer %p.\n", source_pin.peer);
    ok(!source_pin.mt, "Got mt %p.\n", source_pin.mt);
    ok(!sink_pin.peer, "Got peer %p.\n", sink_pin.peer);
    hr = IFilterGraph2_Disconnect(graph, &source_pin.IPin_iface);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IFilterGraph2_ConnectDirect(graph, &source_pin.IPin_iface, &sink_pin.IPin_iface, &mt);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IFilterGraph2_ReconnectEx(graph, &source_pin.IPin_iface, &mt);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(source_pin.peer == &sink_pin.IPin_iface, "Got peer %p.\n", source_pin.peer);
    ok(source_pin.mt == &mt, "Got mt %p.\n", source_pin.mt);
    ok(!sink_pin.peer, "Got peer %p.\n", sink_pin.peer);
    hr = IFilterGraph2_Disconnect(graph, &source_pin.IPin_iface);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    /* ConnectDirect() protects against cyclical connections. */
    hr = IFilterGraph2_AddFilter(graph, &parser1.IBaseFilter_iface, NULL);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IFilterGraph2_AddFilter(graph, &parser2.IBaseFilter_iface, NULL);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IFilterGraph2_ConnectDirect(graph, &parser1_pins[1].IPin_iface, &parser1_pins[0].IPin_iface, NULL);
    ok(hr == VFW_E_CIRCULAR_GRAPH, "Got hr %#lx.\n", hr);

    hr = IFilterGraph2_ConnectDirect(graph, &parser1_pins[1].IPin_iface, &parser2_pins[0].IPin_iface, NULL);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IFilterGraph2_ConnectDirect(graph, &parser2_pins[1].IPin_iface, &parser1_pins[0].IPin_iface, NULL);
    todo_wine ok(hr == VFW_E_CIRCULAR_GRAPH, "Got hr %#lx.\n", hr);
    IFilterGraph2_Disconnect(graph, &parser1_pins[1].IPin_iface);
    IFilterGraph2_Disconnect(graph, &parser2_pins[0].IPin_iface);

    parser1_pins[0].QueryInternalConnections_hr = S_OK;
    hr = IFilterGraph2_ConnectDirect(graph, &parser1_pins[1].IPin_iface, &parser1_pins[0].IPin_iface, NULL);
    todo_wine ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(!parser1_pins[0].peer, "Got peer %p.\n", parser1_pins[0].peer);
    todo_wine ok(parser1_pins[1].peer == &parser1_pins[0].IPin_iface, "Got peer %p.\n", parser1_pins[1].peer);
    IFilterGraph2_Disconnect(graph, &parser1_pins[0].IPin_iface);
    IFilterGraph2_Disconnect(graph, &parser1_pins[1].IPin_iface);

    hr = IFilterGraph2_ConnectDirect(graph, &parser1_pins[1].IPin_iface, &parser2_pins[0].IPin_iface, NULL);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IFilterGraph2_ConnectDirect(graph, &parser2_pins[1].IPin_iface, &parser1_pins[0].IPin_iface, NULL);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    IFilterGraph2_Disconnect(graph, &parser1_pins[1].IPin_iface);
    IFilterGraph2_Disconnect(graph, &parser2_pins[0].IPin_iface);

    hr = IFilterGraph2_RemoveFilter(graph, &parser1.IBaseFilter_iface);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IFilterGraph2_RemoveFilter(graph, &parser2.IBaseFilter_iface);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    /* Both pins are disconnected when a filter is removed. */
    hr = IFilterGraph2_ConnectDirect(graph, &source_pin.IPin_iface, &sink_pin.IPin_iface, &mt);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    sink_pin.peer = &source_pin.IPin_iface;
    IPin_AddRef(sink_pin.peer);
    hr = IFilterGraph2_RemoveFilter(graph, &source.IBaseFilter_iface);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(!source_pin.peer, "Got peer %p.\n", source_pin.peer);
    ok(!sink_pin.peer, "Got peer %p.\n", sink_pin.peer);

    /* If the filter cannot be disconnected, then RemoveFilter() fails. */

    hr = IFilterGraph2_AddFilter(graph, &source.IBaseFilter_iface, NULL);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IFilterGraph2_ConnectDirect(graph, &source_pin.IPin_iface, &sink_pin.IPin_iface, &mt);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    sink_pin.peer = &source_pin.IPin_iface;
    IPin_AddRef(sink_pin.peer);
    hr = IMediaControl_Pause(control);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    source_pin.require_stopped_disconnect = TRUE;
    hr = IFilterGraph2_RemoveFilter(graph, &source.IBaseFilter_iface);
    ok(hr == VFW_E_NOT_STOPPED, "Got hr %#lx.\n", hr);
    ok(source_pin.peer == &sink_pin.IPin_iface, "Got peer %p.\n", source_pin.peer);
    ok(!sink_pin.peer, "Got peer %p.\n", sink_pin.peer);

    sink_pin.peer = &source_pin.IPin_iface;
    IPin_AddRef(sink_pin.peer);
    source_pin.require_stopped_disconnect = FALSE;
    sink_pin.require_stopped_disconnect = TRUE;
    hr = IFilterGraph2_RemoveFilter(graph, &source.IBaseFilter_iface);
    ok(hr == VFW_E_NOT_STOPPED, "Got hr %#lx.\n", hr);
    ok(source_pin.peer == &sink_pin.IPin_iface, "Got peer %p.\n", source_pin.peer);
    ok(sink_pin.peer == &source_pin.IPin_iface, "Got peer %p.\n", sink_pin.peer);

    /* Filters are stopped, and pins disconnected, when the graph is destroyed. */

    IMediaControl_Release(control);
    ref = IFilterGraph2_Release(graph);
    ok(!ref, "Got outstanding refcount %ld.\n", ref);
    ok(source.ref == 1, "Got outstanding refcount %ld.\n", source.ref);
    ok(sink.ref == 1, "Got outstanding refcount %ld.\n", sink.ref);
    ok(source_pin.ref == 1, "Got outstanding refcount %ld.\n", source_pin.ref);
    todo_wine ok(sink_pin.ref == 1, "Got outstanding refcount %ld.\n", sink_pin.ref);
    ok(!source_pin.peer, "Got peer %p.\n", source_pin.peer);
    ok(!sink_pin.peer, "Got peer %p.\n", sink_pin.peer);
}

static void test_sync_source(void)
{
    struct testfilter filter1, filter2, filter3;

    IFilterGraph2 *graph = create_graph();
    IReferenceClock *systemclock, *clock;
    IMediaFilter *filter;
    HRESULT hr;
    ULONG ref;

    IFilterGraph2_QueryInterface(graph, &IID_IMediaFilter, (void **)&filter);

    testfilter_init(&filter1, NULL, 0);
    testfilter_init(&filter2, NULL, 0);
    testfilter_init(&filter3, NULL, 0);

    IFilterGraph2_AddFilter(graph, &filter1.IBaseFilter_iface, NULL);
    IFilterGraph2_AddFilter(graph, &filter2.IBaseFilter_iface, NULL);

    ok(!filter1.clock, "Got clock %p.\n", filter1.clock);
    ok(!filter2.clock, "Got clock %p.\n", filter2.clock);

    CoCreateInstance(&CLSID_SystemClock, NULL, CLSCTX_INPROC_SERVER,
            &IID_IReferenceClock, (void **)&systemclock);

    hr = IMediaFilter_SetSyncSource(filter, systemclock);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(filter1.clock == systemclock, "Got clock %p.\n", filter1.clock);
    ok(filter2.clock == systemclock, "Got clock %p.\n", filter2.clock);

    hr = IMediaFilter_GetSyncSource(filter, &clock);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(clock == systemclock, "Got clock %p.\n", clock);
    IReferenceClock_Release(clock);

    IFilterGraph2_AddFilter(graph, &filter3.IBaseFilter_iface, NULL);
    ok(filter3.clock == systemclock, "Got clock %p.\n", filter3.clock);

    hr = IMediaFilter_SetSyncSource(filter, NULL);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(!filter1.clock, "Got clock %p.\n", filter1.clock);
    ok(!filter2.clock, "Got clock %p.\n", filter2.clock);
    ok(!filter3.clock, "Got clock %p.\n", filter3.clock);

    hr = IMediaFilter_GetSyncSource(filter, &clock);
    todo_wine ok(hr == S_FALSE, "Got hr %#lx.\n", hr);
    ok(!clock, "Got clock %p.\n", clock);

    IReferenceClock_Release(systemclock);
    IMediaFilter_Release(filter);
    ref = IFilterGraph2_Release(graph);
    ok(!ref, "Got outstanding refcount %ld\n", ref);
    ok(filter1.ref == 1, "Got outstanding refcount %ld.\n", filter1.ref);
    ok(filter2.ref == 1, "Got outstanding refcount %ld.\n", filter2.ref);
    ok(filter3.ref == 1, "Got outstanding refcount %ld.\n", filter3.ref);
}

#define check_filter_state(a, b) check_filter_state_(__LINE__, a, b)
static void check_filter_state_(unsigned int line, IFilterGraph2 *graph, FILTER_STATE expect)
{
    IMediaFilter *mediafilter;
    IEnumFilters *filterenum;
    IMediaControl *control;
    OAFilterState oastate;
    IBaseFilter *filter;
    FILTER_STATE state;
    HRESULT hr;

    IFilterGraph2_QueryInterface(graph, &IID_IMediaFilter, (void **)&mediafilter);
    hr = IMediaFilter_GetState(mediafilter, 1000, &state);
    ok_(__FILE__, line)(hr == S_OK, "IMediaFilter_GetState() returned %#lx.\n", hr);
    ok_(__FILE__, line)(state == expect, "Expected state %u, got %u.\n", expect, state);
    IMediaFilter_Release(mediafilter);

    IFilterGraph2_QueryInterface(graph, &IID_IMediaControl, (void **)&control);
    hr = IMediaControl_GetState(control, 1000, &oastate);
    ok_(__FILE__, line)(hr == S_OK, "IMediaControl_GetState() returned %#lx.\n", hr);
    ok_(__FILE__, line)(state == expect, "Expected state %u, got %u.\n", expect, state);
    IMediaControl_Release(control);

    IFilterGraph2_EnumFilters(graph, &filterenum);
    while (IEnumFilters_Next(filterenum, 1, &filter, NULL) == S_OK)
    {
        hr = IBaseFilter_GetState(filter, 1000, &state);
        ok_(__FILE__, line)(hr == S_OK, "IBaseFilter_GetState() returned %#lx.\n", hr);
        ok_(__FILE__, line)(state == expect, "Expected state %u, got %u.\n", expect, state);
        IBaseFilter_Release(filter);
    }
    IEnumFilters_Release(filterenum);
}


static void test_filter_state(void)
{
    struct testfilter source, sink, dummy;
    struct testpin source_pin, sink_pin;

    IFilterGraph2 *graph = create_graph();
    REFERENCE_TIME start_time, time;
    IReferenceClock *clock;
    IMediaControl *control;
    IMediaSeeking *seeking;
    FILTER_STATE mf_state;
    IMediaFilter *filter;
    OAFilterState state;
    HRESULT hr;
    ULONG ref;

    testsource_init(&source_pin, NULL, 0);
    testsink_init(&sink_pin);
    testfilter_init(&source, &source_pin, 1);
    testfilter_init(&sink, &sink_pin, 1);
    testfilter_init(&dummy, NULL, 0);
    sink.IMediaSeeking_iface.lpVtbl = &testseek_vtbl;

    IFilterGraph2_QueryInterface(graph, &IID_IMediaFilter, (void **)&filter);
    IFilterGraph2_QueryInterface(graph, &IID_IMediaControl, (void **)&control);

    IFilterGraph2_AddFilter(graph, &source.IBaseFilter_iface, NULL);
    IFilterGraph2_AddFilter(graph, &sink.IBaseFilter_iface, NULL);
    IFilterGraph2_AddFilter(graph, &dummy.IBaseFilter_iface, NULL);
    /* Using IPin::Connect instead of IFilterGraph2::ConnectDirect to show that */
    /* FilterGraph does not rely on ::ConnectDirect to track filter connections. */
    IPin_Connect(&source_pin.IPin_iface, &sink_pin.IPin_iface, NULL);

    check_filter_state(graph, State_Stopped);

    hr = IMediaControl_Pause(control);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    check_filter_state(graph, State_Paused);

    /* Pausing sets the default sync source, if it's not already set. */

    hr = IMediaFilter_GetSyncSource(filter, &clock);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(!!clock, "Reference clock not set.\n");
    ok(source.clock == clock, "Expected %p, got %p.\n", clock, source.clock);
    ok(sink.clock == clock, "Expected %p, got %p.\n", clock, sink.clock);

    hr = IReferenceClock_GetTime(clock, &start_time);
    ok(SUCCEEDED(hr), "Got hr %#lx.\n", hr);
    hr = IMediaControl_Run(control);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    check_filter_state(graph, State_Running);
    if (winetest_interactive) /* Timing problems make this test too liable to fail. */
        ok(source.start_time >= start_time && source.start_time < start_time + 500 * 10000,
                "Expected time near %s, got %s.\n",
                wine_dbgstr_longlong(start_time), wine_dbgstr_longlong(source.start_time));
    ok(sink.start_time == source.start_time, "Expected time %s, got %s.\n",
        wine_dbgstr_longlong(source.start_time), wine_dbgstr_longlong(sink.start_time));

    hr = IMediaControl_Pause(control);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    check_filter_state(graph, State_Paused);

    sink.state = State_Stopped;
    hr = IMediaControl_GetState(control, 0, &state);
    ok(hr == E_FAIL, "Got hr %#lx.\n", hr);
    ok(state == State_Paused, "Got state %lu.\n", state);

    sink.state = State_Running;
    hr = IMediaControl_GetState(control, 0, &state);
    ok(hr == E_FAIL, "Got hr %#lx.\n", hr);
    ok(state == State_Paused, "Got state %lu.\n", state);

    sink.state = State_Paused;
    hr = IMediaControl_GetState(control, 0, &state);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(state == State_Paused, "Got state %lu.\n", state);

    hr = IMediaControl_Stop(control);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    check_filter_state(graph, State_Stopped);

    hr = IMediaControl_Run(control);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    check_filter_state(graph, State_Running);

    sink.state = State_Stopped;
    hr = IMediaControl_GetState(control, 0, &state);
    ok(hr == E_FAIL, "Got hr %#lx.\n", hr);
    ok(state == State_Running, "Got state %lu.\n", state);

    sink.state = State_Paused;
    hr = IMediaControl_GetState(control, 0, &state);
    ok(hr == VFW_S_STATE_INTERMEDIATE, "Got hr %#lx.\n", hr);
    ok(state == State_Running, "Got state %lu.\n", state);

    sink.state = State_Running;
    hr = IMediaControl_GetState(control, 0, &state);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(state == State_Running, "Got state %lu.\n", state);

    hr = IMediaControl_Stop(control);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    check_filter_state(graph, State_Stopped);

    sink.state = State_Running;
    hr = IMediaControl_GetState(control, 0, &state);
    ok(hr == E_FAIL, "Got hr %#lx.\n", hr);
    ok(state == State_Stopped, "Got state %lu.\n", state);

    sink.state = State_Paused;
    hr = IMediaControl_GetState(control, 0, &state);
    ok(hr == VFW_S_STATE_INTERMEDIATE, "Got hr %#lx.\n", hr);
    ok(state == State_Stopped, "Got state %lu.\n", state);

    sink.state = State_Stopped;
    hr = IMediaControl_GetState(control, 0, &state);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(state == State_Stopped, "Got state %lu.\n", state);

    hr = IMediaControl_Pause(control);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    check_filter_state(graph, State_Paused);

    hr = IMediaControl_StopWhenReady(control);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    check_filter_state(graph, State_Stopped);

    hr = IMediaControl_Run(control);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    check_filter_state(graph, State_Running);

    hr = IMediaControl_StopWhenReady(control);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    check_filter_state(graph, State_Stopped);

    hr = IMediaControl_StopWhenReady(control);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    check_filter_state(graph, State_Stopped);

    IReferenceClock_Release(clock);
    IMediaFilter_Release(filter);
    IMediaControl_Release(control);
    IFilterGraph2_Release(graph);

    /* Test same methods using IMediaFilter. */

    graph = create_graph();
    IFilterGraph2_QueryInterface(graph, &IID_IMediaFilter, (void **)&filter);
    IFilterGraph2_QueryInterface(graph, &IID_IMediaControl, (void **)&control);
    IFilterGraph2_QueryInterface(graph, &IID_IMediaSeeking, (void **)&seeking);

    /* Add the filters in reverse order this time. */
    IFilterGraph2_AddFilter(graph, &sink.IBaseFilter_iface, NULL);
    IFilterGraph2_AddFilter(graph, &source.IBaseFilter_iface, NULL);
    /* Using IPin::Connect instead of IFilterGraph2::ConnectDirect to show that */
    /* FilterGraph does not rely on ::ConnectDirect to track filter connections. */
    IPin_Connect(&source_pin.IPin_iface, &sink_pin.IPin_iface, NULL);

    hr = IMediaFilter_Pause(filter);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    check_filter_state(graph, State_Paused);

    hr = IMediaFilter_GetSyncSource(filter, &clock);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(!!clock, "Reference clock not set.\n");
    ok(source.clock == clock, "Expected %p, got %p.\n", clock, source.clock);
    ok(sink.clock == clock, "Expected %p, got %p.\n", clock, sink.clock);

    hr = IMediaFilter_Run(filter, 0xdeadbeef);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    check_filter_state(graph, State_Running);
    ok(source.start_time == 0xdeadbeef, "Got time %s.\n", wine_dbgstr_longlong(source.start_time));
    ok(sink.start_time == 0xdeadbeef, "Got time %s.\n", wine_dbgstr_longlong(sink.start_time));

    hr = IMediaFilter_Pause(filter);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    check_filter_state(graph, State_Paused);

    hr = IMediaFilter_Run(filter, 0xdeadf00d);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    check_filter_state(graph, State_Running);
    ok(source.start_time == 0xdeadf00d, "Got time %s.\n", wine_dbgstr_longlong(source.start_time));
    ok(sink.start_time == 0xdeadf00d, "Got time %s.\n", wine_dbgstr_longlong(sink.start_time));

    hr = IMediaFilter_Pause(filter);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    check_filter_state(graph, State_Paused);

    hr = IMediaFilter_Stop(filter);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    check_filter_state(graph, State_Stopped);

    source.expect_run_prev = sink.expect_run_prev = State_Stopped;
    hr = IReferenceClock_GetTime(clock, &start_time);
    ok(SUCCEEDED(hr), "Got hr %#lx.\n", hr);
    hr = IMediaFilter_Run(filter, 0);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    check_filter_state(graph, State_Running);
    if (winetest_interactive) /* Timing problems make this test too liable to fail. */
        ok(source.start_time >= start_time && source.start_time < start_time + 500 * 10000,
                "Expected time near %s, got %s.\n",
                wine_dbgstr_longlong(start_time), wine_dbgstr_longlong(source.start_time));
    ok(sink.start_time == source.start_time, "Expected time %s, got %s.\n",
        wine_dbgstr_longlong(source.start_time), wine_dbgstr_longlong(sink.start_time));

    Sleep(600);
    hr = IMediaFilter_Pause(filter);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    check_filter_state(graph, State_Paused);

    source.expect_run_prev = sink.expect_run_prev = State_Paused;
    hr = IMediaFilter_Run(filter, 0);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    check_filter_state(graph, State_Running);
    if (winetest_interactive) /* Timing problems make this test too liable to fail. */
        ok(source.start_time >= start_time && source.start_time < start_time + 500 * 10000,
                "Expected time near %s, got %s.\n",
                wine_dbgstr_longlong(start_time), wine_dbgstr_longlong(source.start_time));
    ok(sink.start_time == source.start_time, "Expected time %s, got %s.\n",
        wine_dbgstr_longlong(source.start_time), wine_dbgstr_longlong(sink.start_time));

    hr = IMediaFilter_Pause(filter);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    check_filter_state(graph, State_Paused);
    Sleep(600);

    start_time += 550 * 10000;
    hr = IMediaFilter_Run(filter, 0);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    check_filter_state(graph, State_Running);
    if (winetest_interactive) /* Timing problems make this test too liable to fail. */
        ok(source.start_time >= start_time && source.start_time < start_time + 500 * 10000,
                "Expected time near %s, got %s.\n",
                wine_dbgstr_longlong(start_time), wine_dbgstr_longlong(source.start_time));
    ok(sink.start_time == source.start_time, "Expected time %s, got %s.\n",
        wine_dbgstr_longlong(source.start_time), wine_dbgstr_longlong(sink.start_time));

    hr = IMediaFilter_Stop(filter);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    check_filter_state(graph, State_Stopped);

    /* Test removing the sync source. */

    IReferenceClock_Release(clock);
    IMediaFilter_SetSyncSource(filter, NULL);

    hr = IMediaControl_Run(control);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    check_filter_state(graph, State_Running);
    todo_wine ok(source.start_time > 0 && source.start_time < 500 * 10000,
            "Got time %s.\n", wine_dbgstr_longlong(source.start_time));
    ok(sink.start_time == source.start_time, "Expected time %s, got %s.\n",
        wine_dbgstr_longlong(source.start_time), wine_dbgstr_longlong(sink.start_time));

    hr = IMediaControl_Stop(control);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    check_filter_state(graph, State_Stopped);

    /* Test asynchronous state change. */

    sink.state_hr = S_FALSE;
    sink.GetState_hr = VFW_S_STATE_INTERMEDIATE;
    hr = IMediaControl_Pause(control);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);

    hr = IMediaControl_GetState(control, 0, &state);
    ok(hr == VFW_S_STATE_INTERMEDIATE, "Got hr %#lx.\n", hr);
    ok(state == State_Paused, "Got state %lu.\n", state);

    sink.state_hr = sink.GetState_hr = S_OK;
    hr = IMediaControl_GetState(control, 0, &state);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(state == State_Paused, "Got state %lu.\n", state);

    sink.state_hr = S_FALSE;
    sink.GetState_hr = VFW_S_STATE_INTERMEDIATE;
    hr = IMediaControl_Stop(control);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);

    hr = IMediaControl_GetState(control, 0, &state);
    ok(hr == VFW_S_STATE_INTERMEDIATE, "Got hr %#lx.\n", hr);
    ok(state == State_Stopped, "Got state %lu.\n", state);

    sink.state_hr = sink.GetState_hr = S_OK;
    hr = IMediaControl_GetState(control, 0, &state);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(state == State_Stopped, "Got state %lu.\n", state);

    /* Renderers are expected to block completing a state change into paused
     * until they receive a sample. Because the graph can transition from
     * stopped -> paused -> running in one call, which itself needs to be
     * asynchronous, it actually waits on a separate thread for all filters
     * to be ready, then calls IMediaFilter::Run() once they are.
     *
     * However, IMediaControl::GetState() will return VFW_S_STATE_INTERMEDIATE
     * if filters haven't caught up to the graph yet. To make matters worse, it
     * doesn't take the above into account, meaning that it'll gladly return
     * VFW_S_STATE_INTERMEDIATE even if passed an infinite timeout. */

    sink.state_hr = S_FALSE;
    sink.GetState_hr = VFW_S_STATE_INTERMEDIATE;
    hr = IMediaControl_Run(control);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);

    hr = IMediaControl_GetState(control, 0, &state);
    ok(hr == VFW_S_STATE_INTERMEDIATE, "Got hr %#lx.\n", hr);
    ok(state == State_Running, "Got state %lu.\n", state);
    ok(sink.state == State_Paused, "Got state %u.\n", sink.state);
    ok(source.state == State_Paused, "Got state %u.\n", source.state);

    /* SetPositions() does not pause the graph in this case, since it is
     * already in a paused state. */
    time = 0;
    hr = IMediaSeeking_SetPositions(seeking, &time, AM_SEEKING_AbsolutePositioning,
            NULL, AM_SEEKING_NoPositioning);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IMediaControl_Run(control);
    todo_wine ok(hr == S_FALSE, "Got hr %#lx.\n", hr);

    hr = IMediaControl_GetState(control, 0, &state);
    ok(hr == VFW_S_STATE_INTERMEDIATE, "Got hr %#lx.\n", hr);
    ok(state == State_Running, "Got state %lu.\n", state);
    ok(sink.state == State_Paused, "Got state %u.\n", sink.state);
    ok(source.state == State_Paused, "Got state %u.\n", source.state);

    sink.state_hr = sink.GetState_hr = S_OK;

    while ((hr = IMediaControl_GetState(control, INFINITE, &state)) == VFW_S_STATE_INTERMEDIATE)
    {
        ok(state == State_Running, "Got state %lu.\n", state);
        ok(sink.state == State_Paused, "Got state %u.\n", sink.state);
        ok(source.state == State_Paused, "Got state %u.\n", source.state);
        Sleep(10);
    }
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(state == State_Running, "Got state %lu.\n", state);
    ok(sink.state == State_Running, "Got state %u.\n", sink.state);
    ok(source.state == State_Running, "Got state %u.\n", source.state);

    /* The above logic does not apply to the running -> paused -> stopped
     * transition. The filter graph will stop a filter regardless of whether
     * it's completely paused. Inasmuch as stopping the filter is like flushing
     * it—i.e. it has to succeed—this makes sense. */

    sink.state_hr = S_FALSE;
    sink.GetState_hr = VFW_S_STATE_INTERMEDIATE;
    hr = IMediaControl_Stop(control);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);
    ok(sink.state == State_Stopped, "Got state %u.\n", sink.state);
    ok(source.state == State_Stopped, "Got state %u.\n", source.state);

    hr = IMediaControl_GetState(control, 0, &state);
    ok(hr == VFW_S_STATE_INTERMEDIATE, "Got hr %#lx.\n", hr);
    ok(state == State_Stopped, "Got state %lu.\n", state);

    hr = IMediaControl_Stop(control);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    sink.state_hr = sink.GetState_hr = S_OK;
    hr = IMediaControl_GetState(control, 0, &state);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(state == State_Stopped, "Got state %lu.\n", state);

    /* Try an asynchronous stopped->paused->running transition, but pause or
     * stop the graph before our filter is completely paused. */

    sink.state_hr = S_FALSE;
    sink.GetState_hr = VFW_S_STATE_INTERMEDIATE;
    hr = IMediaControl_Run(control);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);

    hr = IMediaControl_Pause(control);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);

    sink.state_hr = sink.GetState_hr = S_OK;
    hr = IMediaControl_GetState(control, 0, &state);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(state == State_Paused, "Got state %lu.\n", state);
    ok(sink.state == State_Paused, "Got state %u.\n", sink.state);
    ok(source.state == State_Paused, "Got state %u.\n", source.state);

    hr = IMediaControl_Stop(control);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    sink.state_hr = S_FALSE;
    sink.GetState_hr = VFW_S_STATE_INTERMEDIATE;
    hr = IMediaControl_Run(control);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);

    hr = IMediaControl_Stop(control);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);
    ok(sink.state == State_Stopped, "Got state %u.\n", sink.state);
    ok(source.state == State_Stopped, "Got state %u.\n", source.state);

    sink.state_hr = sink.GetState_hr = S_OK;
    hr = IMediaControl_GetState(control, 0, &state);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(state == State_Stopped, "Got state %lu.\n", state);
    ok(sink.state == State_Stopped, "Got state %u.\n", sink.state);
    ok(source.state == State_Stopped, "Got state %u.\n", source.state);

    /* Same, but tear down the graph instead. */

    sink.state_hr = S_FALSE;
    sink.GetState_hr = VFW_S_STATE_INTERMEDIATE;
    hr = IMediaControl_Run(control);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);

    IMediaFilter_Release(filter);
    IMediaControl_Release(control);
    IMediaSeeking_Release(seeking);
    ref = IFilterGraph2_Release(graph);
    ok(!ref, "Got outstanding refcount %ld.\n", ref);

    graph = create_graph();
    IFilterGraph2_QueryInterface(graph, &IID_IMediaFilter, (void **)&filter);
    IFilterGraph2_QueryInterface(graph, &IID_IMediaControl, (void **)&control);
    IFilterGraph2_QueryInterface(graph, &IID_IMediaSeeking, (void **)&seeking);
    IFilterGraph2_AddFilter(graph, &sink.IBaseFilter_iface, NULL);
    IFilterGraph2_AddFilter(graph, &source.IBaseFilter_iface, NULL);
    IPin_Connect(&source_pin.IPin_iface, &sink_pin.IPin_iface, NULL);

    /* This logic doesn't apply when using IMediaFilter methods directly. */

    source.expect_run_prev = sink.expect_run_prev = State_Stopped;
    sink.state_hr = S_FALSE;
    sink.GetState_hr = VFW_S_STATE_INTERMEDIATE;
    hr = IMediaFilter_Run(filter, 0);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);

    hr = IMediaFilter_GetState(filter, 0, &mf_state);
    ok(hr == VFW_S_STATE_INTERMEDIATE, "Got hr %#lx.\n", hr);
    ok(mf_state == State_Running, "Got state %u.\n", mf_state);
    ok(sink.state == State_Running, "Got state %u.\n", sink.state);
    ok(source.state == State_Running, "Got state %u.\n", source.state);

    sink.state_hr = sink.GetState_hr = S_OK;
    hr = IMediaFilter_GetState(filter, 0, &mf_state);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(mf_state == State_Running, "Got state %u.\n", mf_state);
    ok(sink.state == State_Running, "Got state %u.\n", sink.state);
    ok(source.state == State_Running, "Got state %u.\n", source.state);

    hr = IMediaFilter_Stop(filter);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(sink.state == State_Stopped, "Got state %u.\n", sink.state);
    ok(source.state == State_Stopped, "Got state %u.\n", source.state);

    source.expect_run_prev = sink.expect_run_prev = State_Paused;

    /* Test VFW_S_CANT_CUE. */

    sink.GetState_hr = VFW_S_CANT_CUE;
    hr = IMediaControl_Pause(control);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IMediaControl_GetState(control, 0, &state);
    ok(hr == VFW_S_CANT_CUE, "Got hr %#lx.\n", hr);
    ok(state == State_Paused, "Got state %lu.\n", state);

    sink.GetState_hr = VFW_S_STATE_INTERMEDIATE;
    source.GetState_hr = VFW_S_CANT_CUE;
    hr = IMediaControl_GetState(control, 0, &state);
    ok(hr == VFW_S_CANT_CUE, "Got hr %#lx.\n", hr);
    ok(state == State_Paused, "Got state %lu.\n", state);

    sink.GetState_hr = VFW_S_CANT_CUE;
    source.GetState_hr = VFW_S_STATE_INTERMEDIATE;
    hr = IMediaControl_GetState(control, 0, &state);
    ok(hr == VFW_S_CANT_CUE, "Got hr %#lx.\n", hr);
    ok(state == State_Paused, "Got state %lu.\n", state);

    sink.GetState_hr = source.GetState_hr = S_OK;

    hr = IMediaControl_Stop(control);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    sink.state_hr = S_FALSE;
    sink.GetState_hr = VFW_S_STATE_INTERMEDIATE;
    source.GetState_hr = VFW_S_CANT_CUE;
    hr = IMediaControl_Run(control);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);
    ok(sink.state == State_Running, "Got state %u.\n", sink.state);
    ok(source.state == State_Running, "Got state %u.\n", source.state);

    hr = IMediaControl_GetState(control, 0, &state);
    ok(hr == VFW_S_CANT_CUE, "Got hr %#lx.\n", hr);
    ok(state == State_Running, "Got state %lu.\n", state);
    ok(sink.state == State_Running, "Got state %u.\n", sink.state);
    ok(source.state == State_Running, "Got state %u.\n", source.state);

    sink.state_hr = sink.GetState_hr = source.GetState_hr = S_OK;

    hr = IMediaControl_Stop(control);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    /* Add and remove a filter while the graph is running. */

    hr = IFilterGraph2_AddFilter(graph, &dummy.IBaseFilter_iface, L"dummy");
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(dummy.state == State_Stopped, "Got state %#x.\n", dummy.state);

    hr = IMediaControl_Pause(control);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    check_filter_state(graph, State_Paused);
    ok(dummy.state == State_Paused, "Got state %#x.\n", dummy.state);

    hr = IFilterGraph2_RemoveFilter(graph, &dummy.IBaseFilter_iface);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(dummy.state == State_Paused, "Got state %#x.\n", dummy.state);

    hr = IFilterGraph2_AddFilter(graph, &dummy.IBaseFilter_iface, L"dummy");
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(dummy.state == State_Paused, "Got state %#x.\n", dummy.state);

    hr = IMediaControl_Stop(control);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    check_filter_state(graph, State_Stopped);

    hr = IFilterGraph2_RemoveFilter(graph, &dummy.IBaseFilter_iface);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(dummy.state == State_Stopped, "Got state %#x.\n", dummy.state);

    /* Destroying the graph while it's running stops all filters. */

    hr = IMediaControl_Run(control);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    check_filter_state(graph, State_Running);

    source.expect_stop_prev = sink.expect_stop_prev = State_Running;
    IMediaFilter_Release(filter);
    IMediaControl_Release(control);
    IMediaSeeking_Release(seeking);
    ref = IFilterGraph2_Release(graph);
    ok(!ref, "Got outstanding refcount %ld.\n", ref);
    ok(source.ref == 1, "Got outstanding refcount %ld.\n", source.ref);
    ok(sink.ref == 1, "Got outstanding refcount %ld.\n", sink.ref);
    ok(source_pin.ref == 1, "Got outstanding refcount %ld.\n", source_pin.ref);
    ok(sink_pin.ref == 1, "Got outstanding refcount %ld.\n", sink_pin.ref);
    ok(source.state == State_Stopped, "Got state %u.\n", source.state);
    ok(sink.state == State_Stopped, "Got state %u.\n", sink.state);
}

/* Helper function to check whether a filter is considered a renderer, i.e.
 * whether its EC_COMPLETE notification will be passed on to the application. */
static HRESULT check_ec_complete(IFilterGraph2 *graph, IBaseFilter *filter)
{
    IMediaEventSink *eventsink;
    LONG_PTR param1, param2;
    IMediaControl *control;
    IMediaEvent *eventsrc;
    HRESULT hr, ret_hr;
    LONG code;

    IFilterGraph2_QueryInterface(graph, &IID_IMediaControl, (void **)&control);
    IFilterGraph2_QueryInterface(graph, &IID_IMediaEvent, (void **)&eventsrc);
    IFilterGraph2_QueryInterface(graph, &IID_IMediaEventSink, (void **)&eventsink);

    IMediaControl_Run(control);

    hr = IMediaEvent_GetEvent(eventsrc, &code, &param1, &param2, 0);
    ok(hr == E_ABORT, "Got hr %#lx.\n", hr);

    hr = IMediaEventSink_Notify(eventsink, EC_COMPLETE, S_OK, (LONG_PTR)filter);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    ret_hr = IMediaEvent_GetEvent(eventsrc, &code, &param1, &param2, 0);
    if (ret_hr == S_OK)
    {
        ok(code == EC_COMPLETE, "Got code %#lx.\n", code);
        ok(param1 == S_OK, "Got param1 %#Ix.\n", param1);
        ok(!param2, "Got param2 %#Ix.\n", param2);
        hr = IMediaEvent_FreeEventParams(eventsrc, code, param1, param2);
        ok(hr == S_OK, "Got hr %#lx.\n", hr);

        hr = IMediaEvent_GetEvent(eventsrc, &code, &param1, &param2, 0);
        ok(hr == E_ABORT, "Got hr %#lx.\n", hr);
    }

    IMediaControl_Stop(control);

    IMediaControl_Release(control);
    IMediaEvent_Release(eventsrc);
    IMediaEventSink_Release(eventsink);
    return ret_hr;
}

static void test_ec_complete(void)
{
    struct testpin filter1_pin, filter2_pin, filter3_pin;
    struct testfilter filter1, filter2, filter3;

    IFilterGraph2 *graph = create_graph();
    IMediaEventSink *eventsink;
    LONG_PTR param1, param2;
    IMediaControl *control;
    IMediaEvent *eventsrc;
    LONG code, ref;
    HRESULT hr;

    testsink_init(&filter1_pin);
    testsink_init(&filter2_pin);
    testsink_init(&filter3_pin);
    testfilter_init(&filter1, &filter1_pin, 1);
    testfilter_init(&filter2, &filter2_pin, 1);
    testfilter_init(&filter3, &filter3_pin, 1);

    IFilterGraph2_QueryInterface(graph, &IID_IMediaControl, (void **)&control);
    IFilterGraph2_QueryInterface(graph, &IID_IMediaEvent, (void **)&eventsrc);
    IFilterGraph2_QueryInterface(graph, &IID_IMediaEventSink, (void **)&eventsink);

    IFilterGraph2_AddFilter(graph, &filter1.IBaseFilter_iface, NULL);
    IFilterGraph2_AddFilter(graph, &filter2.IBaseFilter_iface, NULL);
    IFilterGraph2_AddFilter(graph, &filter3.IBaseFilter_iface, NULL);

    /* EC_COMPLETE is only delivered to the user after all renderers deliver it. */

    filter1.IAMFilterMiscFlags_iface.lpVtbl = &testmiscflags_vtbl;
    filter2.IAMFilterMiscFlags_iface.lpVtbl = &testmiscflags_vtbl;
    filter3.IAMFilterMiscFlags_iface.lpVtbl = &testmiscflags_vtbl;
    filter1.misc_flags = filter2.misc_flags = AM_FILTER_MISC_FLAGS_IS_RENDERER;

    IMediaControl_Run(control);

    filter3.misc_flags = AM_FILTER_MISC_FLAGS_IS_RENDERER;

    while ((hr = IMediaEvent_GetEvent(eventsrc, &code, &param1, &param2, 0)) == S_OK)
    {
        ok(code != EC_COMPLETE, "Got unexpected EC_COMPLETE.\n");
        IMediaEvent_FreeEventParams(eventsrc, code, param1, param2);
    }
    ok(hr == E_ABORT, "Got hr %#lx.\n", hr);

    hr = IMediaEventSink_Notify(eventsink, EC_COMPLETE, S_OK, (LONG_PTR)&filter1.IBaseFilter_iface);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IMediaEvent_GetEvent(eventsrc, &code, &param1, &param2, 50);
    ok(hr == E_ABORT, "Got hr %#lx.\n", hr);

    hr = IMediaEventSink_Notify(eventsink, EC_COMPLETE, S_OK, (LONG_PTR)&filter2.IBaseFilter_iface);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IMediaEvent_GetEvent(eventsrc, &code, &param1, &param2, 0);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(code == EC_COMPLETE, "Got code %#lx.\n", code);
    ok(param1 == S_OK, "Got param1 %#Ix.\n", param1);
    ok(!param2, "Got param2 %#Ix.\n", param2);
    hr = IMediaEvent_FreeEventParams(eventsrc, code, param1, param2);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IMediaEvent_GetEvent(eventsrc, &code, &param1, &param2, 50);
    ok(hr == E_ABORT, "Got hr %#lx.\n", hr);

    hr = IMediaEventSink_Notify(eventsink, EC_COMPLETE, S_OK, (LONG_PTR)&filter3.IBaseFilter_iface);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IMediaEvent_GetEvent(eventsrc, &code, &param1, &param2, 50);
    ok(hr == E_ABORT, "Got hr %#lx.\n", hr);

    IMediaControl_Stop(control);

    /* Test CancelDefaultHandling(). */

    IMediaControl_Run(control);

    hr = IMediaEvent_CancelDefaultHandling(eventsrc, EC_COMPLETE);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IMediaEvent_GetEvent(eventsrc, &code, &param1, &param2, 50);
    ok(hr == E_ABORT, "Got hr %#lx.\n", hr);

    hr = IMediaEventSink_Notify(eventsink, EC_COMPLETE, S_OK, (LONG_PTR)&filter1.IBaseFilter_iface);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IMediaEvent_GetEvent(eventsrc, &code, &param1, &param2, 0);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(code == EC_COMPLETE, "Got code %#lx.\n", code);
    ok(param1 == S_OK, "Got param1 %#Ix.\n", param1);
    ok(param2 == (LONG_PTR)&filter1.IBaseFilter_iface, "Got param2 %#Ix.\n", param2);
    hr = IMediaEvent_FreeEventParams(eventsrc, code, param1, param2);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IMediaEvent_GetEvent(eventsrc, &code, &param1, &param2, 50);
    ok(hr == E_ABORT, "Got hr %#lx.\n", hr);

    hr = IMediaEventSink_Notify(eventsink, EC_COMPLETE, S_OK, (LONG_PTR)&filter3.IBaseFilter_iface);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IMediaEvent_GetEvent(eventsrc, &code, &param1, &param2, 0);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(code == EC_COMPLETE, "Got code %#lx.\n", code);
    ok(param1 == S_OK, "Got param1 %#Ix.\n", param1);
    ok(param2 == (LONG_PTR)&filter3.IBaseFilter_iface, "Got param2 %#Ix.\n", param2);
    hr = IMediaEvent_FreeEventParams(eventsrc, code, param1, param2);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IMediaEvent_GetEvent(eventsrc, &code, &param1, &param2, 50);
    ok(hr == E_ABORT, "Got hr %#lx.\n", hr);

    IMediaControl_Stop(control);
    hr = IMediaEvent_RestoreDefaultHandling(eventsrc, EC_COMPLETE);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    /* A filter counts as a renderer if it (1) exposes IAMFilterMiscFlags and
     * reports itself as a renderer, or (2) exposes IMediaSeeking or
     * IMediaPosition and has no output pins. Despite MSDN,
     * QueryInternalConnections() does not seem to be used. */

    IFilterGraph2_RemoveFilter(graph, &filter1.IBaseFilter_iface);
    IFilterGraph2_RemoveFilter(graph, &filter2.IBaseFilter_iface);
    IFilterGraph2_RemoveFilter(graph, &filter3.IBaseFilter_iface);
    filter1.misc_flags = 0;
    IFilterGraph2_AddFilter(graph, &filter1.IBaseFilter_iface, NULL);

    hr = check_ec_complete(graph, &filter1.IBaseFilter_iface);
    ok(hr == E_ABORT, "Got hr %#lx.\n", hr);

    IFilterGraph2_RemoveFilter(graph, &filter1.IBaseFilter_iface);
    filter1_pin.dir = PINDIR_INPUT;
    filter1.IAMFilterMiscFlags_iface.lpVtbl = NULL;
    filter1.IMediaSeeking_iface.lpVtbl = &testseek_vtbl;
    IFilterGraph2_AddFilter(graph, &filter1.IBaseFilter_iface, NULL);

    hr = check_ec_complete(graph, &filter1.IBaseFilter_iface);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    ok(filter1.seeking_ref > 0, "Unexpected seeking refcount %ld.\n", filter1.seeking_ref);
    IFilterGraph2_RemoveFilter(graph, &filter1.IBaseFilter_iface);
    ok(filter1.seeking_ref == 0, "Unexpected seeking refcount %ld.\n", filter1.seeking_ref);

    filter1_pin.dir = PINDIR_OUTPUT;
    IFilterGraph2_AddFilter(graph, &filter1.IBaseFilter_iface, NULL);

    hr = check_ec_complete(graph, &filter1.IBaseFilter_iface);
    ok(hr == E_ABORT, "Got hr %#lx.\n", hr);

    ok(filter1.seeking_ref > 0, "Unexpected seeking refcount %ld.\n", filter1.seeking_ref);
    IFilterGraph2_RemoveFilter(graph, &filter1.IBaseFilter_iface);
    ok(filter1.seeking_ref == 0, "Unexpected seeking refcount %ld.\n", filter1.seeking_ref);

    filter1_pin.dir = PINDIR_INPUT;
    filter1.support_media_time = FALSE;
    IFilterGraph2_AddFilter(graph, &filter1.IBaseFilter_iface, NULL);

    hr = check_ec_complete(graph, &filter1.IBaseFilter_iface);
    ok(hr == E_ABORT, "Got hr %#lx.\n", hr);

    ok(filter1.seeking_ref == 0, "Unexpected seeking refcount %ld.\n", filter1.seeking_ref);
    IFilterGraph2_RemoveFilter(graph, &filter1.IBaseFilter_iface);

    filter1.IMediaPosition_iface.lpVtbl = &testpos_vtbl;
    IFilterGraph2_AddFilter(graph, &filter1.IBaseFilter_iface, NULL);

    hr = check_ec_complete(graph, &filter1.IBaseFilter_iface);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    IFilterGraph2_RemoveFilter(graph, &filter1.IBaseFilter_iface);

    filter1.IMediaSeeking_iface.lpVtbl = NULL;
    filter1.IMediaPosition_iface.lpVtbl = NULL;
    filter1_pin.dir = PINDIR_INPUT;
    filter1.pin_count = 1;
    filter1_pin.QueryInternalConnections_hr = S_OK;
    IFilterGraph2_AddFilter(graph, &filter1.IBaseFilter_iface, NULL);

    hr = check_ec_complete(graph, &filter1.IBaseFilter_iface);
    ok(hr == E_ABORT, "Got hr %#lx.\n", hr);

    IFilterGraph2_RemoveFilter(graph, &filter1.IBaseFilter_iface);

    filter1.IMediaPosition_iface.lpVtbl = &testpos_vtbl;
    IFilterGraph2_AddFilter(graph, &filter1.IBaseFilter_iface, NULL);

    hr = check_ec_complete(graph, &filter1.IBaseFilter_iface);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    IMediaControl_Release(control);
    IMediaEvent_Release(eventsrc);
    IMediaEventSink_Release(eventsink);
    ref = IFilterGraph2_Release(graph);
    ok(!ref, "Got outstanding refcount %ld.\n", ref);
    ok(filter1.ref == 1, "Got outstanding refcount %ld.\n", filter1.ref);
    ok(filter2.ref == 1, "Got outstanding refcount %ld.\n", filter2.ref);
    ok(filter3.ref == 1, "Got outstanding refcount %ld.\n", filter3.ref);
}

static HRESULT WINAPI mpegtestsink_ReceiveConnection(IPin *iface, IPin *peer, const AM_MEDIA_TYPE *mt)
{
    struct testpin *pin = impl_from_IPin(iface);
    if (winetest_debug > 1) trace("%p->ReceiveConnection(%p)\n", pin, peer);

    if (!IsEqualGUID(&mt->majortype, &MEDIATYPE_Video))
        return VFW_E_TYPE_NOT_ACCEPTED;
    if (!IsEqualGUID(&mt->subtype, &MEDIASUBTYPE_MPEG1Payload))
        return VFW_E_TYPE_NOT_ACCEPTED;
    if (!IsEqualGUID(&mt->formattype, &FORMAT_MPEGVideo))
        return VFW_E_TYPE_NOT_ACCEPTED;

    pin->peer = peer;
    IPin_AddRef(peer);
    return S_OK;
}

static HRESULT WINAPI mpegtestpin_EndOfStream(IPin *iface)
{
    return S_OK;
}

static HRESULT WINAPI mpegtestpin_NewSegment(IPin *iface, REFERENCE_TIME start, REFERENCE_TIME stop, double rate)
{
    return S_OK;
}

static const IPinVtbl mpegtestsink_vtbl =
{
    testpin_QueryInterface,
    testpin_AddRef,
    testpin_Release,
    no_Connect,
    mpegtestsink_ReceiveConnection,
    testpin_Disconnect,
    testpin_ConnectedTo,
    testpin_ConnectionMediaType,
    testpin_QueryPinInfo,
    testpin_QueryDirection,
    testpin_QueryId,
    testpin_QueryAccept,
    testpin_EnumMediaTypes,
    testpin_QueryInternalConnections,
    mpegtestpin_EndOfStream,
    testpin_BeginFlush,
    testpin_EndFlush,
    mpegtestpin_NewSegment
};

static HRESULT WINAPI meminput_QueryInterface(IMemInputPin *iface, REFIID iid, void **out)
{
    struct testpin *pin = impl_from_IMemInputPin(iface);
    if (winetest_debug > 1) trace("%p->QueryInterface(%s)\n", pin, wine_dbgstr_guid(iid));

    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI meminput_AddRef(IMemInputPin *iface)
{
    struct testpin *pin = impl_from_IMemInputPin(iface);
    return InterlockedIncrement(&pin->ref);
}

static ULONG WINAPI meminput_Release(IMemInputPin *iface)
{
    struct testpin *pin = impl_from_IMemInputPin(iface);
    return InterlockedDecrement(&pin->ref);
}

static HRESULT STDMETHODCALLTYPE meminput_GetAllocator(IMemInputPin *iface, IMemAllocator **allocator)
{
    return VFW_E_NO_ALLOCATOR;
}

static HRESULT STDMETHODCALLTYPE meminput_NotifyAllocator(IMemInputPin *iface, IMemAllocator *allocator, BOOL readonly)
{
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE meminput_GetAllocatorRequirements(IMemInputPin *iface, ALLOCATOR_PROPERTIES *props)
{
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE meminput_Receive(IMemInputPin *iface, IMediaSample *sample)
{
    struct testpin *pin = impl_from_IMemInputPin(iface);
    size_t new_bytes;
    HRESULT hr;
    BYTE *ptr;
    long len;

    len = IMediaSample_GetActualDataLength(sample);
    hr = IMediaSample_GetPointer(sample, &ptr);

    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(len > 0, "Got %ld.\n", len);

    new_bytes = min(sizeof(pin->input) - pin->input_size, len);
    if (new_bytes)
    {
        memcpy(pin->input + pin->input_size, ptr, new_bytes);
        pin->input_size += new_bytes;
        if (pin->input_size == sizeof(pin->input))
            SetEvent(pin->on_input_full);
    }

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE meminput_ReceiveMultiple(IMemInputPin *iface, IMediaSample **samples, LONG n_samples, LONG *n_samples_processed)
{
    *n_samples_processed = 1;
    return IMemInputPin_Receive(iface, samples[0]);
}

static HRESULT STDMETHODCALLTYPE meminput_ReceiveCanBlock(IMemInputPin *iface) { return S_OK; }

static const IMemInputPinVtbl mpegtestsink_meminput_vtbl =
{
    meminput_QueryInterface,
    meminput_AddRef,
    meminput_Release,

    meminput_GetAllocator,
    meminput_NotifyAllocator,
    meminput_GetAllocatorRequirements,
    meminput_Receive,
    meminput_ReceiveMultiple,
    meminput_ReceiveCanBlock
};

static void test_renderfile_compressed(void)
{
    WCHAR *filename = load_resource(L"test.mpg");
    struct testpin sink_pin;
    struct testfilter sink;
    IMediaControl *control;
    IFilterGraph2 *graph;
    HRESULT hr;
    DWORD ret;

    graph = create_graph();
    testpin_init(&sink_pin, &mpegtestsink_vtbl, PINDIR_INPUT);
    sink_pin.IMemInputPin_iface.lpVtbl = &mpegtestsink_meminput_vtbl;
    testfilter_init(&sink, &sink_pin, 1);

    hr = IFilterGraph2_AddFilter(graph, &sink.IBaseFilter_iface, L"sink");
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IFilterGraph2_RenderFile(graph, filename, NULL);
    ok(hr == S_OK || hr == VFW_S_AUDIO_NOT_RENDERED, "Got hr %#lx.\n", hr);

    ok(sink_pin.peer != NULL, "Expected connection.\n");

    sink_pin.on_input_full = CreateEventW(NULL, FALSE, FALSE, NULL);

    hr = IFilterGraph2_QueryInterface(graph, &IID_IMediaControl, (void **)&control);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IMediaControl_Pause(control);
    ok(SUCCEEDED(hr), "Got hr %#lx.\n", hr);

    ret = WaitForSingleObject(sink_pin.on_input_full, 1000);
    ok(ret == WAIT_OBJECT_0, "Got %#lx.\n", ret);

    ok(sink_pin.input[0] == 0x00, "Expected MPEG sequence header.\n");
    ok(sink_pin.input[1] == 0x00, "Expected MPEG sequence header.\n");
    ok(sink_pin.input[2] == 0x01, "Expected MPEG sequence header.\n");
    ok(sink_pin.input[3] == 0xB3, "Expected MPEG sequence header.\n");

    IMediaControl_Stop(control);
    IFilterGraph2_Release(graph);
    DeleteFileW(filename);
}

static void test_renderfile_failure(void)
{
    static const char bogus_data[20] = {0xde, 0xad, 0xbe, 0xef};

    struct testfilter testfilter;
    IEnumFilters *filterenum;
    const WCHAR *filename;
    IFilterGraph2 *graph;
    IBaseFilter *filter;
    HRESULT hr;
    ULONG ref;
    BOOL ret;

    /* Windows removes the source filter from the graph if a RenderFile
     * call fails. It leaves the rest of the graph intact. */

    graph = create_graph();
    testfilter_init(&testfilter, NULL, 0);
    hr = IFilterGraph2_AddFilter(graph, &testfilter.IBaseFilter_iface, L"dummy");
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    filename = create_file(L"test.nonsense", bogus_data, sizeof(bogus_data));
    hr = IFilterGraph2_RenderFile(graph, filename, NULL);
    todo_wine ok(hr == VFW_E_UNSUPPORTED_STREAM, "Got hr %#lx.\n", hr);

    hr = IFilterGraph2_EnumFilters(graph, &filterenum);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IEnumFilters_Next(filterenum, 1, &filter, NULL);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(filter == &testfilter.IBaseFilter_iface, "Got unexpected filter %p.\n", filter);

    hr = IEnumFilters_Next(filterenum, 1, &filter, NULL);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);

    IEnumFilters_Release(filterenum);

    ref = IFilterGraph2_Release(graph);
    ok(!ref, "Got outstanding refcount %ld.\n", ref);
    ret = DeleteFileW(filename);
    ok(ret, "Failed to delete %s, error %lu.\n", debugstr_w(filename), GetLastError());
}

/* Remove and re-add the filter, to flush the graph's internal
 * IMediaSeeking cache. Don't expose IMediaSeeking when adding, to show
 * that it's only queried when needed. */
static void flush_cached_seeking(IFilterGraph2 *graph, struct testfilter *filter)
{
    IFilterGraph2_RemoveFilter(graph, &filter->IBaseFilter_iface);
    filter->IMediaSeeking_iface.lpVtbl = NULL;
    IFilterGraph2_AddFilter(graph, &filter->IBaseFilter_iface, NULL);
    filter->IMediaSeeking_iface.lpVtbl = &testseek_vtbl;
}

static void test_graph_seeking(void)
{
    struct testfilter filter1, filter2;

    LONGLONG time, current, stop, earliest, latest;
    IFilterGraph2 *graph = create_graph();
    IMediaEventSink *eventsink;
    IMediaControl *control;
    IMediaSeeking *seeking;
    IMediaFilter *filter;
    unsigned int i;
    double rate;
    GUID format;
    HRESULT hr;
    DWORD caps;
    ULONG ref;

    static const GUID *const all_formats[] =
    {
        NULL,
        &TIME_FORMAT_NONE,
        &TIME_FORMAT_FRAME,
        &TIME_FORMAT_SAMPLE,
        &TIME_FORMAT_FIELD,
        &TIME_FORMAT_BYTE,
        &TIME_FORMAT_MEDIA_TIME,
        &testguid,
    };

    static const GUID *const unsupported_formats[] =
    {
        &TIME_FORMAT_FRAME,
        &TIME_FORMAT_SAMPLE,
        &TIME_FORMAT_FIELD,
        &TIME_FORMAT_BYTE,
        &testguid,
    };

    testfilter_init(&filter1, NULL, 0);
    testfilter_init(&filter2, NULL, 0);

    IFilterGraph2_QueryInterface(graph, &IID_IMediaControl, (void **)&control);
    IFilterGraph2_QueryInterface(graph, &IID_IMediaSeeking, (void **)&seeking);
    IFilterGraph2_QueryInterface(graph, &IID_IMediaFilter, (void **)&filter);
    IFilterGraph2_QueryInterface(graph, &IID_IMediaEventSink, (void **)&eventsink);

    hr = IMediaSeeking_GetCapabilities(seeking, &caps);
    todo_wine ok(hr == S_OK, "Got hr %#lx.\n", hr);
    todo_wine ok(!caps, "Got caps %#lx.\n", caps);

    caps = 0;
    hr = IMediaSeeking_CheckCapabilities(seeking, &caps);
    todo_wine ok(hr == E_FAIL, "Got hr %#lx.\n", hr);
    ok(!caps, "Got caps %#lx.\n", caps);

    caps = AM_SEEKING_CanSeekAbsolute;
    hr = IMediaSeeking_CheckCapabilities(seeking, &caps);
    todo_wine ok(hr == E_FAIL, "Got hr %#lx.\n", hr);
    todo_wine ok(!caps, "Got caps %#lx.\n", caps);

    hr = IMediaSeeking_IsFormatSupported(seeking, NULL);
    todo_wine ok(hr == E_NOTIMPL, "Got hr %#lx.\n", hr);

    for (i = 0; i < ARRAY_SIZE(all_formats); ++i)
    {
        hr = IMediaSeeking_IsFormatSupported(seeking, all_formats[i]);
        todo_wine ok(hr == E_NOTIMPL, "Got hr %#lx for format %s.\n", hr, wine_dbgstr_guid(all_formats[i]));
    }

    hr = IMediaSeeking_QueryPreferredFormat(seeking, NULL);
    ok(hr == E_POINTER, "Got hr %#lx.\n", hr);
    hr = IMediaSeeking_QueryPreferredFormat(seeking, &format);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(IsEqualGUID(&format, &TIME_FORMAT_MEDIA_TIME), "Got format %s.\n", wine_dbgstr_guid(&format));

    hr = IMediaSeeking_GetTimeFormat(seeking, NULL);
    ok(hr == E_POINTER, "Got hr %#lx.\n", hr);
    hr = IMediaSeeking_GetTimeFormat(seeking, &format);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(IsEqualGUID(&format, &TIME_FORMAT_MEDIA_TIME), "Got format %s.\n", wine_dbgstr_guid(&format));

    hr = IMediaSeeking_IsUsingTimeFormat(seeking, NULL);
    ok(hr == E_POINTER, "Got hr %#lx.\n", hr);
    hr = IMediaSeeking_IsUsingTimeFormat(seeking, &TIME_FORMAT_MEDIA_TIME);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IMediaSeeking_IsUsingTimeFormat(seeking, &TIME_FORMAT_NONE);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);

    hr = IMediaSeeking_SetTimeFormat(seeking, &TIME_FORMAT_NONE);
    todo_wine ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IMediaSeeking_QueryPreferredFormat(seeking, &format);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(IsEqualGUID(&format, &TIME_FORMAT_MEDIA_TIME), "Got format %s.\n", wine_dbgstr_guid(&format));

    hr = IMediaSeeking_IsUsingTimeFormat(seeking, &TIME_FORMAT_MEDIA_TIME);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IMediaSeeking_IsUsingTimeFormat(seeking, &TIME_FORMAT_NONE);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);

    hr = IMediaSeeking_GetTimeFormat(seeking, &format);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(IsEqualGUID(&format, &TIME_FORMAT_MEDIA_TIME), "Got format %s.\n", wine_dbgstr_guid(&format));

    hr = IMediaSeeking_SetTimeFormat(seeking, &TIME_FORMAT_MEDIA_TIME);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    for (i = 0; i < ARRAY_SIZE(unsupported_formats); ++i)
    {
        hr = IMediaSeeking_SetTimeFormat(seeking, unsupported_formats[i]);
        todo_wine ok(hr == E_NOTIMPL, "Got hr %#lx for format %s.\n", hr, wine_dbgstr_guid(unsupported_formats[i]));
    }

    time = 0xdeadbeef;
    hr = IMediaSeeking_ConvertTimeFormat(seeking, &time, NULL, 0x123456789a, NULL);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(time == 0x123456789a, "Got time %s.\n", wine_dbgstr_longlong(time));

    time = 0xdeadbeef;
    hr = IMediaSeeking_ConvertTimeFormat(seeking, &time, &TIME_FORMAT_MEDIA_TIME, 0x123456789a, NULL);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(time == 0x123456789a, "Got time %s.\n", wine_dbgstr_longlong(time));

    time = 0xdeadbeef;
    hr = IMediaSeeking_ConvertTimeFormat(seeking, &time, NULL, 0x123456789a, &TIME_FORMAT_MEDIA_TIME);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(time == 0x123456789a, "Got time %s.\n", wine_dbgstr_longlong(time));

    time = 0xdeadbeef;
    hr = IMediaSeeking_ConvertTimeFormat(seeking, &time, &TIME_FORMAT_MEDIA_TIME, 0x123456789a, &TIME_FORMAT_MEDIA_TIME);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(time == 0x123456789a, "Got time %s.\n", wine_dbgstr_longlong(time));

    hr = IMediaSeeking_ConvertTimeFormat(seeking, &time, &TIME_FORMAT_NONE, 0x123456789a, &TIME_FORMAT_MEDIA_TIME);
    todo_wine ok(hr == E_NOTIMPL, "Got hr %#lx.\n", hr);
    hr = IMediaSeeking_ConvertTimeFormat(seeking, &time, &TIME_FORMAT_NONE, 0x123456789a, NULL);
    todo_wine ok(hr == E_NOTIMPL, "Got hr %#lx.\n", hr);
    hr = IMediaSeeking_ConvertTimeFormat(seeking, &time, &TIME_FORMAT_MEDIA_TIME, 0x123456789a, &TIME_FORMAT_NONE);
    todo_wine ok(hr == E_NOTIMPL, "Got hr %#lx.\n", hr);
    hr = IMediaSeeking_ConvertTimeFormat(seeking, &time, NULL, 0x123456789a, &TIME_FORMAT_NONE);
    todo_wine ok(hr == E_NOTIMPL, "Got hr %#lx.\n", hr);

    time = 0xdeadbeef;
    hr = IMediaSeeking_ConvertTimeFormat(seeking, &time, &TIME_FORMAT_NONE, 0x123456789a, &TIME_FORMAT_NONE);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(time == 0x123456789a, "Got time %s.\n", wine_dbgstr_longlong(time));

    time = 0xdeadbeef;
    hr = IMediaSeeking_ConvertTimeFormat(seeking, &time, &testguid, 0x123456789a, &testguid);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(time == 0x123456789a, "Got time %s.\n", wine_dbgstr_longlong(time));

    hr = IMediaSeeking_GetDuration(seeking, NULL);
    ok(hr == E_POINTER, "Got hr %#lx.\n", hr);
    hr = IMediaSeeking_GetDuration(seeking, &time);
    ok(hr == E_NOTIMPL, "Got hr %#lx.\n", hr);

    hr = IMediaSeeking_GetStopPosition(seeking, NULL);
    ok(hr == E_POINTER, "Got hr %#lx.\n", hr);
    hr = IMediaSeeking_GetStopPosition(seeking, &time);
    ok(hr == E_NOTIMPL, "Got hr %#lx.\n", hr);

    hr = IMediaSeeking_SetPositions(seeking, &current, 0, &stop, 0);
    ok(hr == E_NOTIMPL, "Got hr %#lx.\n", hr);

    hr = IMediaSeeking_GetPositions(seeking, NULL, NULL);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IMediaSeeking_GetPositions(seeking, NULL, &stop);
    ok(hr == E_NOTIMPL, "Got hr %#lx.\n", hr);
    hr = IMediaSeeking_GetPositions(seeking, &current, &stop);
    ok(hr == E_NOTIMPL, "Got hr %#lx.\n", hr);
    current = 0xdeadbeef;
    hr = IMediaSeeking_GetPositions(seeking, &current, NULL);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(!current, "Got time %s.\n", wine_dbgstr_longlong(time));

    hr = IMediaSeeking_GetCurrentPosition(seeking, NULL);
    ok(hr == E_POINTER, "Got hr %#lx.\n", hr);
    current = 0xdeadbeef;
    hr = IMediaSeeking_GetCurrentPosition(seeking, &current);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(!current, "Got time %s.\n", wine_dbgstr_longlong(time));

    hr = IMediaSeeking_GetAvailable(seeking, &earliest, &latest);
    todo_wine ok(hr == E_NOTIMPL, "Got hr %#lx.\n", hr);

    hr = IMediaSeeking_SetRate(seeking, 1.0);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IMediaSeeking_SetRate(seeking, 2.0);
    todo_wine ok(hr == E_NOTIMPL, "Got hr %#lx.\n", hr);

    hr = IMediaSeeking_GetRate(seeking, &rate);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(rate == 1.0, "Got rate %.16e.\n", rate);

    hr = IMediaSeeking_GetPreroll(seeking, &time);
    todo_wine ok(hr == E_NOTIMPL, "Got hr %#lx.\n", hr);

    /* Try with filters added. Note that a filter need only expose
     * IMediaSeeking—no other heuristics are used to determine if it is a
     * renderer. */

    IFilterGraph2_AddFilter(graph, &filter1.IBaseFilter_iface, NULL);
    filter1.IMediaSeeking_iface.lpVtbl = &testseek_vtbl;
    filter1.support_media_time = FALSE;
    filter1.support_testguid = TRUE;

    hr = IMediaSeeking_GetDuration(seeking, &time);
    ok(hr == E_NOTIMPL, "Got hr %#lx.\n", hr);

    hr = IMediaSeeking_SetTimeFormat(seeking, &testguid);
    todo_wine ok(hr == E_NOTIMPL, "Got hr %#lx.\n", hr);

    IFilterGraph2_RemoveFilter(graph, &filter1.IBaseFilter_iface);
    filter1.support_media_time = TRUE;
    filter1.support_testguid = FALSE;

    IFilterGraph2_AddFilter(graph, &filter1.IBaseFilter_iface, NULL);
    IFilterGraph2_AddFilter(graph, &filter2.IBaseFilter_iface, NULL);
    filter1.IMediaSeeking_iface.lpVtbl = &testseek_vtbl;
    filter2.IMediaSeeking_iface.lpVtbl = &testseek_vtbl;

    filter1.seek_caps = AM_SEEKING_CanDoSegments | AM_SEEKING_CanGetCurrentPos;
    filter2.seek_caps = AM_SEEKING_CanDoSegments | AM_SEEKING_CanGetDuration;
    hr = IMediaSeeking_GetCapabilities(seeking, &caps);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(caps == AM_SEEKING_CanDoSegments, "Got caps %#lx.\n", caps);
    ok(filter1.seeking_ref > 0, "Unexpected seeking refcount %ld.\n", filter1.seeking_ref);
    ok(filter2.seeking_ref > 0, "Unexpected seeking refcount %ld.\n", filter2.seeking_ref);

    flush_cached_seeking(graph, &filter1);
    flush_cached_seeking(graph, &filter2);

    caps = AM_SEEKING_CanDoSegments | AM_SEEKING_CanGetCurrentPos;
    hr = IMediaSeeking_CheckCapabilities(seeking, &caps);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);
    ok(caps == AM_SEEKING_CanDoSegments, "Got caps %#lx.\n", caps);

    caps = AM_SEEKING_CanDoSegments;
    hr = IMediaSeeking_CheckCapabilities(seeking, &caps);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(caps == AM_SEEKING_CanDoSegments, "Got caps %#lx.\n", caps);

    caps = AM_SEEKING_CanGetCurrentPos;
    hr = IMediaSeeking_CheckCapabilities(seeking, &caps);
    ok(hr == E_FAIL, "Got hr %#lx.\n", hr);
    ok(!caps, "Got caps %#lx.\n", caps);

    flush_cached_seeking(graph, &filter1);
    flush_cached_seeking(graph, &filter2);

    hr = IMediaSeeking_IsFormatSupported(seeking, &testguid);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);

    filter1.support_testguid = TRUE;
    hr = IMediaSeeking_IsFormatSupported(seeking, &testguid);
    todo_wine ok(hr == S_OK, "Got hr %#lx.\n", hr);

    filter1.support_testguid = FALSE;
    filter2.support_testguid = TRUE;
    hr = IMediaSeeking_IsFormatSupported(seeking, &testguid);
    todo_wine ok(hr == S_OK, "Got hr %#lx.\n", hr);

    /* Filters are not consulted about preferred formats. */
    hr = IMediaSeeking_QueryPreferredFormat(seeking, &format);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(IsEqualGUID(&format, &TIME_FORMAT_MEDIA_TIME), "Got format %s.\n", wine_dbgstr_guid(&format));

    hr = IMediaSeeking_GetTimeFormat(seeking, &format);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(IsEqualGUID(&format, &TIME_FORMAT_MEDIA_TIME), "Got format %s.\n", wine_dbgstr_guid(&format));

    filter2.support_testguid = FALSE;
    hr = IMediaSeeking_SetTimeFormat(seeking, &testguid);
    todo_wine ok(hr == E_FAIL, "Got hr %#lx.\n", hr);

    hr = IMediaSeeking_GetTimeFormat(seeking, &format);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(IsEqualGUID(&format, &TIME_FORMAT_MEDIA_TIME), "Got format %s.\n", wine_dbgstr_guid(&format));

    filter1.support_testguid = TRUE;
    hr = IMediaSeeking_SetTimeFormat(seeking, &testguid);
    todo_wine ok(hr == S_OK, "Got hr %#lx.\n", hr);
    todo_wine ok(IsEqualGUID(&filter1.time_format, &testguid), "Got format %s.\n",
            debugstr_guid(&filter1.time_format));
    ok(IsEqualGUID(&filter2.time_format, &TIME_FORMAT_MEDIA_TIME),
            "Got format %s.\n", debugstr_guid(&filter2.time_format));

    hr = IMediaSeeking_GetTimeFormat(seeking, &format);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    todo_wine ok(IsEqualGUID(&format, &testguid), "Got format %s.\n", wine_dbgstr_guid(&format));

    hr = IMediaSeeking_QueryPreferredFormat(seeking, &format);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(IsEqualGUID(&format, &TIME_FORMAT_MEDIA_TIME), "Got format %s.\n", wine_dbgstr_guid(&format));

    hr = IMediaSeeking_IsUsingTimeFormat(seeking, &TIME_FORMAT_MEDIA_TIME);
    todo_wine ok(hr == S_FALSE, "Got hr %#lx.\n", hr);
    hr = IMediaSeeking_IsUsingTimeFormat(seeking, &testguid);
    todo_wine ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IMediaSeeking_GetCapabilities(seeking, &caps);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(caps == AM_SEEKING_CanDoSegments, "Got caps %#lx.\n", caps);
    ok(filter1.seeking_ref > 0, "Unexpected seeking refcount %ld.\n", filter1.seeking_ref);
    ok(filter2.seeking_ref > 0, "Unexpected seeking refcount %ld.\n", filter2.seeking_ref);

    filter2.support_testguid = TRUE;
    hr = IMediaSeeking_SetTimeFormat(seeking, &testguid);
    todo_wine ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(IsEqualGUID(&filter1.time_format, &TIME_FORMAT_MEDIA_TIME),
            "Got format %s.\n", debugstr_guid(&filter1.time_format));
    todo_wine ok(IsEqualGUID(&filter2.time_format, &testguid),
            "Got format %s.\n", debugstr_guid(&filter2.time_format));

    filter1.support_media_time = FALSE;
    flush_cached_seeking(graph, &filter1);

    hr = IMediaSeeking_GetCapabilities(seeking, &caps);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(caps == (AM_SEEKING_CanDoSegments | AM_SEEKING_CanGetDuration), "Got caps %#lx.\n", caps);
    ok(!filter1.seeking_ref, "Unexpected seeking refcount %ld.\n", filter1.seeking_ref);
    ok(filter2.seeking_ref > 0, "Unexpected seeking refcount %ld.\n", filter2.seeking_ref);

    filter1.support_media_time = TRUE;
    filter1.support_testguid = FALSE;
    flush_cached_seeking(graph, &filter1);

    hr = IMediaSeeking_GetCapabilities(seeking, &caps);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(caps == AM_SEEKING_CanDoSegments, "Got caps %#lx.\n", caps);
    ok(filter1.seeking_ref > 0, "Unexpected seeking refcount %ld.\n", filter1.seeking_ref);
    ok(filter2.seeking_ref > 0, "Unexpected seeking refcount %ld.\n", filter2.seeking_ref);

    ok(IsEqualGUID(&filter1.time_format, &TIME_FORMAT_MEDIA_TIME),
            "Got format %s.\n", debugstr_guid(&filter1.time_format));
    todo_wine ok(IsEqualGUID(&filter2.time_format, &testguid),
            "Got format %s.\n", debugstr_guid(&filter2.time_format));
    hr = IMediaSeeking_GetTimeFormat(seeking, &format);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    todo_wine ok(IsEqualGUID(&format, &testguid), "Got format %s.\n", wine_dbgstr_guid(&format));

    hr = IMediaSeeking_SetTimeFormat(seeking, &TIME_FORMAT_NONE);
    todo_wine ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IMediaSeeking_GetTimeFormat(seeking, &format);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(IsEqualGUID(&format, &TIME_FORMAT_MEDIA_TIME), "Got format %s.\n", wine_dbgstr_guid(&format));

    time = 0xdeadbeef;
    hr = IMediaSeeking_ConvertTimeFormat(seeking, &time, &testguid, 0x123456789a, &testguid);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(time == 0x123456789a, "Got time %s.\n", wine_dbgstr_longlong(time));

    hr = IMediaSeeking_ConvertTimeFormat(seeking, &time, &testguid, 0x123456789a, &TIME_FORMAT_NONE);
    todo_wine ok(hr == E_NOTIMPL, "Got hr %#lx.\n", hr);

    flush_cached_seeking(graph, &filter1);
    flush_cached_seeking(graph, &filter2);

    filter1.seek_duration = 0x12345;
    filter2.seek_duration = 0x23456;
    hr = IMediaSeeking_GetDuration(seeking, &time);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(time == 0x23456, "Got time %s.\n", wine_dbgstr_longlong(time));

    filter2.seek_duration = 0x12345;
    filter1.seek_duration = 0x23456;
    hr = IMediaSeeking_GetDuration(seeking, &time);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(time == 0x23456, "Got time %s.\n", wine_dbgstr_longlong(time));

    filter1.seek_hr = filter2.seek_hr = 0xbeef;
    hr = IMediaSeeking_GetDuration(seeking, &time);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(time == 0x23456, "Got time %s.\n", wine_dbgstr_longlong(time));

    filter1.seek_hr = E_NOTIMPL;
    filter2.seek_hr = S_OK;
    hr = IMediaSeeking_GetDuration(seeking, &time);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(time == 0x12345, "Got time %s.\n", wine_dbgstr_longlong(time));

    filter1.seek_hr = 0xdeadbeef;
    hr = IMediaSeeking_GetDuration(seeking, &time);
    ok(hr == 0xdeadbeef, "Got hr %#lx.\n", hr);

    filter1.seek_hr = filter2.seek_hr = E_NOTIMPL;
    hr = IMediaSeeking_GetDuration(seeking, &time);
    ok(hr == E_NOTIMPL, "Got hr %#lx.\n", hr);
    filter1.seek_hr = filter2.seek_hr = S_OK;

    flush_cached_seeking(graph, &filter1);
    flush_cached_seeking(graph, &filter2);

    filter1.seek_stop = 0x54321;
    filter2.seek_stop = 0x65432;
    hr = IMediaSeeking_GetStopPosition(seeking, &time);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(time == 0x65432, "Got time %s.\n", wine_dbgstr_longlong(time));

    filter2.seek_stop = 0x54321;
    filter1.seek_stop = 0x65432;
    hr = IMediaSeeking_GetStopPosition(seeking, &time);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(time == 0x65432, "Got time %s.\n", wine_dbgstr_longlong(time));

    filter1.seek_hr = filter2.seek_hr = 0xbeef;
    hr = IMediaSeeking_GetStopPosition(seeking, &time);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(time == 0x65432, "Got time %s.\n", wine_dbgstr_longlong(time));

    filter1.seek_hr = E_NOTIMPL;
    filter2.seek_hr = S_OK;
    hr = IMediaSeeking_GetStopPosition(seeking, &time);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(time == 0x54321, "Got time %s.\n", wine_dbgstr_longlong(time));

    filter1.seek_hr = 0xdeadbeef;
    hr = IMediaSeeking_GetStopPosition(seeking, &time);
    ok(hr == 0xdeadbeef, "Got hr %#lx.\n", hr);

    filter1.seek_hr = filter2.seek_hr = E_NOTIMPL;
    hr = IMediaSeeking_GetStopPosition(seeking, &time);
    ok(hr == E_NOTIMPL, "Got hr %#lx.\n", hr);
    filter1.seek_hr = filter2.seek_hr = S_OK;

    flush_cached_seeking(graph, &filter1);
    flush_cached_seeking(graph, &filter2);

    hr = IMediaSeeking_GetCurrentPosition(seeking, &time);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(!time, "Got time %s.\n", wine_dbgstr_longlong(time));

    flush_cached_seeking(graph, &filter1);
    flush_cached_seeking(graph, &filter2);

    current = stop = 0xdeadbeef;
    hr = IMediaSeeking_GetPositions(seeking, &current, &stop);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(!current, "Got time %s.\n", wine_dbgstr_longlong(current));
    ok(stop == 0x65432, "Got time %s.\n", wine_dbgstr_longlong(stop));

    flush_cached_seeking(graph, &filter1);
    flush_cached_seeking(graph, &filter2);

    current = 0x123;
    stop = 0x321;
    hr = IMediaSeeking_SetPositions(seeking, &current, AM_SEEKING_AbsolutePositioning,
            &stop, AM_SEEKING_AbsolutePositioning);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(current == 0x123, "Got time %s.\n", wine_dbgstr_longlong(current));
    ok(stop == 0x321, "Got time %s.\n", wine_dbgstr_longlong(stop));
    ok(filter1.seek_current == 0x123, "Got time %s.\n", wine_dbgstr_longlong(filter1.seek_current));
    ok(filter1.seek_stop == 0x321, "Got time %s.\n", wine_dbgstr_longlong(filter1.seek_stop));
    ok(filter2.seek_current == 0x123, "Got time %s.\n", wine_dbgstr_longlong(filter2.seek_current));
    ok(filter2.seek_stop == 0x321, "Got time %s.\n", wine_dbgstr_longlong(filter2.seek_stop));

    filter1.seek_hr = filter2.seek_hr = 0xbeef;
    hr = IMediaSeeking_SetPositions(seeking, &current, AM_SEEKING_AbsolutePositioning,
            &stop, AM_SEEKING_AbsolutePositioning);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    filter1.seek_hr = E_NOTIMPL;
    filter2.seek_hr = S_OK;
    hr = IMediaSeeking_SetPositions(seeking, &current, AM_SEEKING_AbsolutePositioning,
            &stop, AM_SEEKING_AbsolutePositioning);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    filter1.seek_hr = 0xdeadbeef;
    hr = IMediaSeeking_SetPositions(seeking, &current, AM_SEEKING_AbsolutePositioning,
            &stop, AM_SEEKING_AbsolutePositioning);
    ok(hr == 0xdeadbeef, "Got hr %#lx.\n", hr);

    filter1.seek_hr = filter2.seek_hr = E_NOTIMPL;
    hr = IMediaSeeking_SetPositions(seeking, &current, AM_SEEKING_AbsolutePositioning,
            &stop, AM_SEEKING_AbsolutePositioning);
    ok(hr == E_NOTIMPL, "Got hr %#lx.\n", hr);
    filter1.seek_hr = filter2.seek_hr = S_OK;

    hr = IMediaSeeking_GetCurrentPosition(seeking, &time);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(time == 12340000, "Got time %s.\n", wine_dbgstr_longlong(time));

    current = stop = 0xdeadbeef;
    hr = IMediaSeeking_GetPositions(seeking, &current, &stop);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(current == 12340000, "Got time %s.\n", wine_dbgstr_longlong(current));
    ok(stop == 0x321, "Got time %s.\n", wine_dbgstr_longlong(stop));

    current = 0x123;
    stop = 0x321;
    hr = IMediaSeeking_SetPositions(seeking, &current, AM_SEEKING_AbsolutePositioning | AM_SEEKING_ReturnTime,
            &stop, AM_SEEKING_AbsolutePositioning);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(current == 12340000, "Got time %s.\n", wine_dbgstr_longlong(current));
    ok(stop == 0x321, "Got time %s.\n", wine_dbgstr_longlong(stop));
    ok(filter1.seek_current == 12340000, "Got time %s.\n", wine_dbgstr_longlong(filter1.seek_current));
    ok(filter1.seek_stop == 0x321, "Got time %s.\n", wine_dbgstr_longlong(filter1.seek_stop));
    ok(filter2.seek_current == 0x123, "Got time %s.\n", wine_dbgstr_longlong(filter2.seek_current));
    ok(filter2.seek_stop == 0x321, "Got time %s.\n", wine_dbgstr_longlong(filter2.seek_stop));

    current = 0x123;
    stop = 0x321;
    hr = IMediaSeeking_SetPositions(seeking, &current, AM_SEEKING_AbsolutePositioning,
            &stop, AM_SEEKING_AbsolutePositioning | AM_SEEKING_ReturnTime);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(current == 0x123, "Got time %s.\n", wine_dbgstr_longlong(current));
    ok(stop == 43210000, "Got time %s.\n", wine_dbgstr_longlong(stop));
    ok(filter1.seek_current == 0x123, "Got time %s.\n", wine_dbgstr_longlong(filter1.seek_current));
    ok(filter1.seek_stop == 43210000, "Got time %s.\n", wine_dbgstr_longlong(filter1.seek_stop));
    ok(filter2.seek_current == 0x123, "Got time %s.\n", wine_dbgstr_longlong(filter2.seek_current));
    ok(filter2.seek_stop == 0x321, "Got time %s.\n", wine_dbgstr_longlong(filter2.seek_stop));

    flush_cached_seeking(graph, &filter1);
    flush_cached_seeking(graph, &filter2);

    hr = IMediaSeeking_SetRate(seeking, 2.0);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    todo_wine ok(filter1.seek_rate == 2.0, "Got rate %.16e.\n", filter1.seek_rate);
    todo_wine ok(filter2.seek_rate == 2.0, "Got rate %.16e.\n", filter2.seek_rate);

    hr = IMediaSeeking_SetRate(seeking, 1.0);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    todo_wine ok(filter1.seek_rate == 1.0, "Got rate %.16e.\n", filter1.seek_rate);
    todo_wine ok(filter2.seek_rate == 1.0, "Got rate %.16e.\n", filter2.seek_rate);

    hr = IMediaSeeking_SetRate(seeking, -1.0);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    todo_wine ok(filter1.seek_rate == -1.0, "Got rate %.16e.\n", filter1.seek_rate);
    todo_wine ok(filter2.seek_rate == -1.0, "Got rate %.16e.\n", filter2.seek_rate);

    flush_cached_seeking(graph, &filter1);
    flush_cached_seeking(graph, &filter2);

    hr = IMediaSeeking_GetRate(seeking, &rate);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    todo_wine ok(rate == -1.0, "Got rate %.16e.\n", rate);

    hr = IMediaSeeking_SetRate(seeking, 1.0);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    /* Test how retrieving the current position behaves while the graph is
     * running. Apparently the graph caches the last position returned by
     * SetPositions() and then adds the clock offset to the stream start. */

    hr = IMediaControl_Run(control);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    /* Note that if the graph is running, it is paused while seeking. */
    current = 0;
    stop = 9000 * 10000;
    hr = IMediaSeeking_SetPositions(seeking, &current, AM_SEEKING_AbsolutePositioning,
            &stop, AM_SEEKING_AbsolutePositioning);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IMediaSeeking_GetCurrentPosition(seeking, &time);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    if (winetest_interactive) /* Timing problems make this test too liable to fail. */
        ok(compare_time(time, 1234 * 10000, 40 * 10000),
                "Expected about 1234ms, got %s.\n", wine_dbgstr_longlong(time));
    current = stop = 0xdeadbeef;
    hr = IMediaSeeking_GetPositions(seeking, &current, &stop);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    if (winetest_interactive) /* Timing problems make this test too liable to fail. */
        ok(compare_time(current, 1234 * 10000, 40 * 10000),
                "Expected about 1234ms, got %s.\n", wine_dbgstr_longlong(current));
    ok(stop == 9000 * 10000, "Got time %s.\n", wine_dbgstr_longlong(stop));

    /* This remains true even if NoFlush is specified. */
    current = 1000 * 10000;
    stop = 8000 * 10000;
    hr = IMediaSeeking_SetPositions(seeking, &current,
            AM_SEEKING_AbsolutePositioning | AM_SEEKING_NoFlush,
            &stop, AM_SEEKING_AbsolutePositioning | AM_SEEKING_NoFlush);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    Sleep(100);

    hr = IMediaSeeking_GetCurrentPosition(seeking, &time);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    if (winetest_interactive) /* Timing problems make this test too liable to fail. */
        ok(compare_time(time, 1334 * 10000, 80 * 10000),
                "Expected about 1334ms, got %s.\n", wine_dbgstr_longlong(time));
    current = stop = 0xdeadbeef;
    hr = IMediaSeeking_GetPositions(seeking, &current, &stop);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    if (winetest_interactive) /* Timing problems make this test too liable to fail. */
        ok(compare_time(current, 1334 * 10000, 80 * 10000),
                "Expected about 1334ms, got %s.\n", wine_dbgstr_longlong(current));
    ok(stop == 8000 * 10000, "Got time %s.\n", wine_dbgstr_longlong(stop));

    hr = IMediaControl_Pause(control);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    Sleep(100);
    hr = IMediaControl_Run(control);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IMediaSeeking_GetCurrentPosition(seeking, &time);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    if (winetest_interactive) /* Timing problems make this test too liable to fail. */
        ok(compare_time(time, 1334 * 10000, 80 * 10000),
                "Expected about 1334ms, got %s.\n", wine_dbgstr_longlong(time));
    current = stop = 0xdeadbeef;
    hr = IMediaSeeking_GetPositions(seeking, &current, &stop);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    if (winetest_interactive) /* Timing problems make this test too liable to fail. */
        ok(compare_time(current, 1334 * 10000, 80 * 10000),
                "Expected about 1334ms, got %s.\n", wine_dbgstr_longlong(current));
    ok(stop == 8000 * 10000, "Got time %s.\n", wine_dbgstr_longlong(stop));

    hr = IMediaControl_Stop(control);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IMediaSeeking_GetCurrentPosition(seeking, &time);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(time == 12340000, "Got time %s.\n", wine_dbgstr_longlong(time));

    hr = IMediaFilter_SetSyncSource(filter, NULL);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IMediaControl_Run(control);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    Sleep(100);
    hr = IMediaSeeking_GetCurrentPosition(seeking, &time);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    todo_wine ok(!time, "Got time %s.\n", wine_dbgstr_longlong(time));
    current = stop = 0xdeadbeef;
    hr = IMediaSeeking_GetPositions(seeking, &current, &stop);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    todo_wine ok(!current, "Got time %s.\n", wine_dbgstr_longlong(current));
    ok(!stop, "Got time %s.\n", wine_dbgstr_longlong(stop));

    hr = IMediaControl_Stop(control);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    /* GetCurrentPositions() will return the stop position once all renderers
     * report EC_COMPLETE. Atelier Sophie depends on this behaviour. */

    hr = IFilterGraph2_SetDefaultSyncSource(graph);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    filter1.seek_stop = 5000 * 10000;
    filter2.seek_stop = 6000 * 10000;

    hr = IMediaControl_Run(control);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IMediaSeeking_GetCurrentPosition(seeking, &time);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(time < 5000 * 10000, "Got time %s.\n", wine_dbgstr_longlong(time));

    hr = IMediaEventSink_Notify(eventsink, EC_COMPLETE, S_OK, (LONG_PTR)&filter1.IBaseFilter_iface);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IMediaSeeking_GetCurrentPosition(seeking, &time);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(time < 5000 * 10000, "Got time %s.\n", wine_dbgstr_longlong(time));

    hr = IMediaEventSink_Notify(eventsink, EC_COMPLETE, S_OK, (LONG_PTR)&filter2.IBaseFilter_iface);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IMediaSeeking_GetCurrentPosition(seeking, &time);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(time == 6000 * 10000, "Got time %s.\n", wine_dbgstr_longlong(time));

    hr = IMediaControl_Stop(control);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    filter1.seek_hr = filter2.seek_hr = E_NOTIMPL;
    filter1.seek_stop = filter2.seek_stop = 0xdeadbeef;

    hr = IMediaControl_Run(control);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IMediaSeeking_GetCurrentPosition(seeking, &time);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(time < 5000 * 10000, "Got time %s.\n", wine_dbgstr_longlong(time));

    hr = IMediaEventSink_Notify(eventsink, EC_COMPLETE, S_OK, (LONG_PTR)&filter1.IBaseFilter_iface);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IMediaEventSink_Notify(eventsink, EC_COMPLETE, S_OK, (LONG_PTR)&filter2.IBaseFilter_iface);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IMediaSeeking_GetCurrentPosition(seeking, &time);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(time == 6000 * 10000, "Got time %s.\n", wine_dbgstr_longlong(time));

    hr = IMediaSeeking_SetTimeFormat(seeking, &TIME_FORMAT_MEDIA_TIME);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IMediaSeeking_SetTimeFormat(seeking, &TIME_FORMAT_NONE);
    todo_wine ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IMediaControl_Stop(control);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    IMediaFilter_Release(filter);
    IMediaControl_Release(control);
    IMediaSeeking_Release(seeking);
    IMediaEventSink_Release(eventsink);

    ok(filter1.seeking_ref > 0, "Unexpected seeking refcount %ld.\n", filter1.seeking_ref);
    ok(filter2.seeking_ref > 0, "Unexpected seeking refcount %ld.\n", filter2.seeking_ref);

    ref = IFilterGraph2_Release(graph);
    ok(!ref, "Got outstanding refcount %ld.\n", ref);
    ok(filter1.ref == 1, "Got outstanding refcount %ld.\n", filter1.ref);
    ok(filter2.ref == 1, "Got outstanding refcount %ld.\n", filter2.ref);
    ok(filter1.seeking_ref == 0, "Unexpected seeking refcount %ld.\n", filter1.seeking_ref);
    ok(filter2.seeking_ref == 0, "Unexpected seeking refcount %ld.\n", filter2.seeking_ref);
}

static void test_default_sync_source(void)
{
    struct testpin source_pin, sink1_pin, sink2_pin;
    struct testfilter source, sink1, sink2;

    IFilterGraph2 *graph = create_graph();
    IReferenceClock *clock;
    IMediaFilter *filter;
    HRESULT hr;
    ULONG ref;

    testsink_init(&sink1_pin);
    testsink_init(&sink2_pin);
    testsource_init(&source_pin, NULL, 0);
    testfilter_init(&source, &source_pin, 1);
    testfilter_init(&sink1, &sink1_pin, 1);
    testfilter_init(&sink2, &sink2_pin, 1);

    IFilterGraph2_AddFilter(graph, &sink1.IBaseFilter_iface, NULL);
    IFilterGraph2_AddFilter(graph, &sink2.IBaseFilter_iface, NULL);
    IFilterGraph2_AddFilter(graph, &source.IBaseFilter_iface, NULL);
    IFilterGraph2_ConnectDirect(graph, &source_pin.IPin_iface, &sink1_pin.IPin_iface, NULL);

    IFilterGraph2_QueryInterface(graph, &IID_IMediaFilter, (void **)&filter);

    hr = IFilterGraph2_SetDefaultSyncSource(graph);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IMediaFilter_GetSyncSource(filter, &clock);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(!!clock, "Reference clock not set.\n");
    IReferenceClock_Release(clock);

    source.IReferenceClock_iface.lpVtbl = &testclock_vtbl;

    hr = IFilterGraph2_SetDefaultSyncSource(graph);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IMediaFilter_GetSyncSource(filter, &clock);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(clock == &source.IReferenceClock_iface, "Got unexpected clock.\n");
    IReferenceClock_Release(clock);

    /* The documentation says that connected filters are preferred, but this
     * does not in fact seem to be the case. */

    sink2.IReferenceClock_iface.lpVtbl = &testclock_vtbl;

    hr = IFilterGraph2_SetDefaultSyncSource(graph);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IMediaFilter_GetSyncSource(filter, &clock);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    todo_wine ok(clock == &sink2.IReferenceClock_iface, "Got unexpected clock.\n");
    IReferenceClock_Release(clock);

    sink1.IReferenceClock_iface.lpVtbl = &testclock_vtbl;

    hr = IFilterGraph2_SetDefaultSyncSource(graph);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IMediaFilter_GetSyncSource(filter, &clock);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    todo_wine ok(clock == &sink1.IReferenceClock_iface, "Got unexpected clock.\n");
    IReferenceClock_Release(clock);

    IMediaFilter_Release(filter);
    ref = IFilterGraph2_Release(graph);
    ok(!ref, "Got outstanding refcount %ld.\n", ref);
    ok(sink1.ref == 1, "Got outstanding refcount %ld.\n", sink1.ref);
    ok(sink2.ref == 1, "Got outstanding refcount %ld.\n", sink2.ref);
    ok(source.ref == 1, "Got outstanding refcount %ld.\n", source.ref);
}

static void test_add_source_filter(void)
{
    static const char bogus_data[20] = {0xde, 0xad, 0xbe, 0xef};
    static const char midi_data[20] = {'M','T','h','d'};

    IFilterGraph2 *graph = create_graph();
    IFileSourceFilter *filesource;
    IBaseFilter *filter, *filter2;
    FILTER_INFO filter_info;
    const WCHAR *filename;
    WCHAR *ret_filename;
    AM_MEDIA_TYPE mt;
    CLSID clsid;
    HRESULT hr;
    ULONG ref;
    BOOL ret;
    HKEY key;

    /* Test a file which should be registered by extension. */

    filename = create_file(L"test.mp3", midi_data, sizeof(midi_data));
    hr = IFilterGraph2_AddSourceFilter(graph, filename, L"test", &filter);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IBaseFilter_GetClassID(filter, &clsid);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(IsEqualGUID(&clsid, &CLSID_AsyncReader), "Got filter %s.\n", wine_dbgstr_guid(&clsid));
    hr = IBaseFilter_QueryFilterInfo(filter, &filter_info);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(!wcscmp(filter_info.achName, L"test"), "Got unexpected name %s.\n", wine_dbgstr_w(filter_info.achName));
    IFilterGraph_Release(filter_info.pGraph);

    hr = IBaseFilter_QueryInterface(filter, &IID_IFileSourceFilter, (void **)&filesource);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IFileSourceFilter_GetCurFile(filesource, &ret_filename, &mt);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(!wcscmp(ret_filename, filename), "Expected filename %s, got %s.\n",
            wine_dbgstr_w(filename), wine_dbgstr_w(ret_filename));
    ok(IsEqualGUID(&mt.majortype, &MEDIATYPE_Stream), "Got major type %s.\n", wine_dbgstr_guid(&mt.majortype));
    ok(IsEqualGUID(&mt.subtype, &MEDIASUBTYPE_MPEG1Audio), "Got subtype %s.\n", wine_dbgstr_guid(&mt.subtype));
    IFileSourceFilter_Release(filesource);
    CoTaskMemFree(ret_filename);
    FreeMediaType(&mt);

    hr = IFilterGraph2_AddSourceFilter(graph, filename, L"test", &filter2);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(filter2 != filter, "Filters shouldn't match.\n");
    hr = IFilterGraph2_RemoveFilter(graph, filter2);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ref = IBaseFilter_Release(filter2);
    ok(!ref, "Got outstanding refcount %ld.\n", ref);

    hr = IFilterGraph2_RemoveFilter(graph, filter);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ref = IBaseFilter_Release(filter);
    ok(!ref, "Got outstanding refcount %ld.\n", ref);
    ret = DeleteFileW(filename);
    ok(ret, "Failed to delete %s, error %lu.\n", wine_dbgstr_w(filename), GetLastError());

    /* Test a file which should be registered by signature. */

    filename = create_file(L"test.avi", midi_data, sizeof(midi_data));
    hr = IFilterGraph2_AddSourceFilter(graph, filename, NULL, &filter);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IBaseFilter_GetClassID(filter, &clsid);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(IsEqualGUID(&clsid, &CLSID_AsyncReader), "Got filter %s.\n", wine_dbgstr_guid(&clsid));
    hr = IBaseFilter_QueryFilterInfo(filter, &filter_info);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    todo_wine ok(!wcscmp(filter_info.achName, filename), "Got unexpected name %s.\n", wine_dbgstr_w(filter_info.achName));
    IFilterGraph_Release(filter_info.pGraph);

    hr = IBaseFilter_QueryInterface(filter, &IID_IFileSourceFilter, (void **)&filesource);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IFileSourceFilter_GetCurFile(filesource, &ret_filename, &mt);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(!wcscmp(ret_filename, filename), "Expected filename %s, got %s.\n",
            wine_dbgstr_w(filename), wine_dbgstr_w(ret_filename));
    ok(IsEqualGUID(&mt.majortype, &MEDIATYPE_Stream), "Got major type %s.\n", wine_dbgstr_guid(&mt.majortype));
    ok(IsEqualGUID(&mt.subtype, &MEDIATYPE_Midi), "Got subtype %s.\n", wine_dbgstr_guid(&mt.subtype));
    IFileSourceFilter_Release(filesource);
    CoTaskMemFree(ret_filename);
    FreeMediaType(&mt);

    hr = IFilterGraph2_RemoveFilter(graph, filter);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ref = IBaseFilter_Release(filter);
    ok(!ref, "Got outstanding refcount %ld.\n", ref);
    ret = DeleteFileW(filename);
    ok(ret, "Failed to delete %s, error %lu.\n", wine_dbgstr_w(filename), GetLastError());

    if (!RegCreateKeyA(HKEY_CLASSES_ROOT, "Media Type\\{abbccdde-0000-0000-0000-000000000000}"
            "\\{bccddeef-0000-0000-0000-000000000000}", &key))
    {
        static const GUID testfilter_clsid = {0x12345678};
        struct testpin testfilter_pin;
        struct testfilter testfilter;
        struct testfilter_cf cf = {{&testfilter_cf_vtbl}, &testfilter};
        DWORD cookie;

        testsource_init(&testfilter_pin, NULL, 0);
        testfilter_init(&testfilter, &testfilter_pin, 1);
        testfilter.IFileSourceFilter_iface.lpVtbl = &testfilesource_vtbl;

        CoRegisterClassObject(&testfilter_clsid, (IUnknown *)&cf.IClassFactory_iface,
                CLSCTX_INPROC_SERVER, REGCLS_MULTIPLEUSE, &cookie);
        RegSetValueExA(key, "0", 0, REG_SZ, (const BYTE *)"0,4,,deadbeef", 14);
        RegSetValueExA(key, "Source Filter", 0, REG_SZ, (const BYTE *)"{12345678-0000-0000-0000-000000000000}", 39);

        filename = create_file(L"test.avi", bogus_data, sizeof(bogus_data));
        hr = IFilterGraph2_AddSourceFilter(graph, filename, NULL, &filter);
        ok(hr == S_OK, "Got hr %#lx.\n", hr);
        ok(filter == &testfilter.IBaseFilter_iface, "Got unexpected filter %p.\n", filter);

        hr = IFilterGraph2_RemoveFilter(graph, filter);
        ok(hr == S_OK, "Got hr %#lx.\n", hr);
        IBaseFilter_Release(filter);
        ref = IBaseFilter_Release(&testfilter.IBaseFilter_iface);
        ok(!ref, "Got outstanding refcount %ld.\n", ref);
        ret = DeleteFileW(filename);
        ok(ret, "Failed to delete %s, error %lu.\n", wine_dbgstr_w(filename), GetLastError());
        RegDeleteKeyA(HKEY_CLASSES_ROOT, "Media Type\\{abbccdde-0000-0000-0000-000000000000}"
            "\\{bccddeef-0000-0000-0000-000000000000}");
        RegDeleteKeyA(HKEY_CLASSES_ROOT, "Media Type\\{abbccdde-0000-0000-0000-000000000000}");
        CoRevokeClassObject(cookie);
    }
    else
        skip("Not enough permission to register media types.\n");

    /* Test some failure cases. */

    filter = (IBaseFilter *)0xdeadbeef;
    hr = IFilterGraph2_AddSourceFilter(graph, L"", NULL, &filter);
    /* the BURIKO visual novel engine requires this exact error code */
    ok(hr == VFW_E_NOT_FOUND, "Got hr %#lx.\n", hr);
    ok(filter == (IBaseFilter *)0xdeadbeef, "Got %p.\n", filter);

    filter = (IBaseFilter *)0xdeadbeef;
    hr = IFilterGraph2_AddSourceFilter(graph, L"doesnt_exist.mp3", NULL, &filter);
    ok(hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND), "Got hr %#lx.\n", hr);
    ok(filter == (IBaseFilter *)0xdeadbeef, "Got %p.\n", filter);

    ref = IFilterGraph2_Release(graph);
    ok(!ref, "Got outstanding refcount %ld.\n", ref);
}

static HWND get_renderer_hwnd(IFilterGraph2 *graph)
{
    IEnumFilters *enum_filters;
    IEnumPins *enum_pins;
    IBaseFilter *filter;
    IOverlay *overlay;
    HWND hwnd = NULL;
    HRESULT hr;
    IPin *pin;

    hr = IFilterGraph2_EnumFilters(graph, &enum_filters);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    while (IEnumFilters_Next(enum_filters, 1, &filter, NULL) == S_OK)
    {
        hr = IBaseFilter_EnumPins(filter, &enum_pins);
        ok(hr == S_OK, "Got hr %#lx.\n", hr);
        hr = IEnumPins_Next(enum_pins, 1, &pin, NULL);
        ok(hr == S_OK, "Got hr %#lx.\n", hr);
        IEnumPins_Release(enum_pins);

        if (SUCCEEDED(IPin_QueryInterface(pin, &IID_IOverlay, (void **)&overlay)))
        {
            hr = IOverlay_GetWindowHandle(overlay, &hwnd);
            ok(hr == S_OK, "Got hr %#lx.\n", hr);
            IOverlay_Release(overlay);
        }

        IPin_Release(pin);
        IBaseFilter_Release(filter);
    }

    IEnumFilters_Release(enum_filters);

    return hwnd;
}

static BOOL expect_parent_message = TRUE;

static LRESULT CALLBACK parent_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    ok(expect_parent_message, "Got unexpected message %#x.\n", msg);
    return DefWindowProcA(hwnd, msg, wparam, lparam);
}

static void test_window_threading(void)
{
    static const WNDCLASSA class =
    {
        .lpfnWndProc = parent_proc,
        .lpszClassName = "quartz_test_parent",
    };
    WCHAR *filename = load_resource(L"test.avi");
    IFilterGraph2 *graph = create_graph();
    HWND hwnd, hwnd2, parent;
    IFilterGraph2 *graph2;
    IVideoWindow *window;
    DWORD tid, tid2;
    HRESULT hr;
    ULONG ref;
    BOOL ret;

    RegisterClassA(&class);

    parent = CreateWindowA("quartz_test_parent", NULL, WS_OVERLAPPEDWINDOW,
            50, 50, 150, 150, NULL, NULL, NULL, NULL);
    ok(!!parent, "Failed to create parent window.\n");

    hr = IFilterGraph2_RenderFile(graph, filename, NULL);
    if (FAILED(hr))
    {
        skip("Cannot render test file, hr %#lx.\n", hr);
        IFilterGraph2_Release(graph);
        DeleteFileW(filename);
        return;
    }
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    if ((hwnd = get_renderer_hwnd(graph)))
    {
        tid = GetWindowThreadProcessId(hwnd, NULL);
        ok(tid != GetCurrentThreadId(), "Window should have been created on a separate thread.\n");

        /* The thread should be processing messages, or this will hang. */
        SendMessageA(hwnd, WM_NULL, 0, 0);

        /* Media Player Classic deadlocks if the parent is sent any messages
         * while the video window is released. In particular, we must not send
         * WM_PARENTNOTIFY. This is not achieved through window styles. */
        hr = IFilterGraph2_QueryInterface(graph, &IID_IVideoWindow, (void **)&window);
        ok(hr == S_OK, "Got hr %#lx.\n", hr);
        hr = IVideoWindow_put_Owner(window, (OAHWND)parent);
        ok(hr == S_OK, "Got hr %#lx.\n", hr);
        IVideoWindow_Release(window);
        ok(!(GetWindowLongW(hwnd, GWL_EXSTYLE) & WS_EX_NOPARENTNOTIFY), "Window has WS_EX_NOPARENTNOTIFY.\n");

        graph2 = create_graph();
        hr = IFilterGraph2_RenderFile(graph2, filename, NULL);
        ok(hr == S_OK, "Got hr %#lx.\n", hr);
        hwnd2 = get_renderer_hwnd(graph);
        ok(!!hwnd2, "Failed to get renderer window.\n");
        tid2 = GetWindowThreadProcessId(hwnd, NULL);
        ok(tid2 == tid, "Expected thread to be shared.\n");

        ref = IFilterGraph2_Release(graph2);
        ok(!ref, "Got outstanding refcount %ld.\n", ref);
    }
    else
        skip("Could not find renderer window.\n");

    SetActiveWindow(parent);
    expect_parent_message = FALSE;
    ref = IFilterGraph2_Release(graph);
    ok(!ref, "Got outstanding refcount %ld.\n", ref);
    expect_parent_message = TRUE;

    hwnd = GetActiveWindow();
    ok(hwnd == parent, "Parent window lost focus, active window %p.\n", hwnd);

    hr = CoCreateInstance(&CLSID_FilterGraphNoThread, NULL, CLSCTX_INPROC_SERVER,
            &IID_IFilterGraph2, (void **)&graph);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IFilterGraph2_RenderFile(graph, filename, NULL);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    if ((hwnd = get_renderer_hwnd(graph)))
    {
        tid = GetWindowThreadProcessId(hwnd, NULL);
        ok(tid == GetCurrentThreadId(), "Window should be created on main thread.\n");
    }
    else
        skip("Could not find renderer window.\n");

    ref = IFilterGraph2_Release(graph);
    ok(!ref, "Got outstanding refcount %ld.\n", ref);

    DestroyWindow(parent);
    UnregisterClassA("quartz_test_parent", GetModuleHandleA(NULL));
    ret = DeleteFileW(filename);
    ok(ret, "Failed to delete file, error %lu.\n", GetLastError());
}

/* Hyperdevotion Noire needs to be able to Render() from UYVY. */
static void test_autoplug_uyvy(void)
{
    static const VIDEOINFOHEADER source_format =
    {
        .bmiHeader.biSize = sizeof(BITMAPINFOHEADER),
        .bmiHeader.biWidth = 600,
        .bmiHeader.biHeight = 400,
        .bmiHeader.biCompression = mmioFOURCC('U','Y','V','Y'),
        .bmiHeader.biBitCount = 16,
    };
    AM_MEDIA_TYPE source_type =
    {
        .majortype = MEDIATYPE_Video,
        .subtype = MEDIASUBTYPE_UYVY,
        .formattype = FORMAT_VideoInfo,
        .cbFormat = sizeof(source_format),
        .pbFormat = (BYTE *)&source_format,
    };

    IFilterGraph2 *graph = create_graph();
    struct testpin source_pin;
    struct testfilter source;
    HRESULT hr;
    ULONG ref;

    testsource_init(&source_pin, NULL, 0);
    testfilter_init(&source, &source_pin, 1);
    source_pin.request_mt = &source_type;

    IFilterGraph2_AddFilter(graph, &source.IBaseFilter_iface, L"source");

    /* Windows 2008 doesn't seem to have an UYVY decoder, and the testbot chalks
     * failure to decode up to missing audio hardware, even though we're not
     * trying to render audio. */
    hr = IFilterGraph2_Render(graph, &source_pin.IPin_iface);
    ok(hr == S_OK || hr == VFW_E_NO_AUDIO_HARDWARE, "Got hr %#lx.\n", hr);

    ref = IFilterGraph2_Release(graph);
    ok(!ref, "Got outstanding refcount %ld.\n", ref);
    ok(source.ref == 1, "Got outstanding refcount %ld.\n", source.ref);
    ok(source_pin.ref == 1, "Got outstanding refcount %ld.\n", source_pin.ref);
}

static void test_set_notify_flags(void)
{
    IFilterGraph2 *graph = create_graph();
    IMediaEventSink *media_event_sink;
    IMediaControl *media_control;
    IMediaEventEx *media_event;
    struct testfilter filter;
    LONG_PTR param1, param2;
    LONG code, flags;
    HANDLE event;
    HWND window;
    BSTR status;
    HRESULT hr;
    ULONG ref;
    MSG msg;

    window = CreateWindowA("static", NULL, WS_OVERLAPPEDWINDOW,
            50, 50, 150, 150, NULL, NULL, NULL, NULL);
    ok(!!window, "Failed to create window.\n");
    status = SysAllocString(L"status");

    hr = IFilterGraph2_QueryInterface(graph, &IID_IMediaControl, (void **)&media_control);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IFilterGraph2_QueryInterface(graph, &IID_IMediaEventEx, (void **)&media_event);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IFilterGraph2_QueryInterface(graph, &IID_IMediaEventSink, (void **)&media_event_sink);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    testfilter_init(&filter, NULL, 0);
    filter.IAMFilterMiscFlags_iface.lpVtbl = &testmiscflags_vtbl;
    filter.misc_flags = AM_FILTER_MISC_FLAGS_IS_RENDERER;

    hr = IFilterGraph2_AddFilter(graph, &filter.IBaseFilter_iface, NULL);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IMediaEventEx_GetEventHandle(media_event, (OAEVENT *)&event);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IMediaEventEx_SetNotifyWindow(media_event, (OAHWND)window, WM_USER, 0);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IMediaControl_Run(media_control);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    ok(WaitForSingleObject(event, 0) == 0, "Event should be signaled.\n");

    while (PeekMessageA(&msg, window, WM_USER, WM_USER, PM_REMOVE));

    hr = IMediaEventEx_SetNotifyFlags(media_event, 2);
    ok(hr == E_INVALIDARG, "Got hr %#lx.\n", hr);

    hr = IMediaEventEx_GetNotifyFlags(media_event, NULL);
    ok(hr == E_POINTER, "Got hr %#lx.\n", hr);

    flags = 0xdeadbeef;
    hr = IMediaEventEx_GetNotifyFlags(media_event, &flags);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(!flags, "Got flags %#lx\n", flags);

    hr = IMediaEventEx_SetNotifyFlags(media_event, AM_MEDIAEVENT_NONOTIFY);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    flags = 0xdeadbeef;
    hr = IMediaEventEx_GetNotifyFlags(media_event, &flags);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(flags == AM_MEDIAEVENT_NONOTIFY, "Got flags %#lx\n", flags);

    ok(WaitForSingleObject(event, 0) == WAIT_TIMEOUT, "Event should not be signaled.\n");

    hr = IMediaEventEx_GetEvent(media_event, &code, &param1, &param2, 50);
    ok(hr == E_ABORT, "Got hr %#lx.\n", hr);

    hr = IMediaEventSink_Notify(media_event_sink, EC_STATUS, (LONG_PTR)status, (LONG_PTR)status);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    ok(WaitForSingleObject(event, 0) == WAIT_TIMEOUT, "Event should not be signaled.\n");

    ok(!PeekMessageA(&msg, window, WM_USER, WM_USER, PM_REMOVE), "Window should not be notified.\n");

    hr = IMediaEventEx_GetEvent(media_event, &code, &param1, &param2, 50);
    ok(hr == E_ABORT, "Got hr %#lx.\n", hr);

    hr = IMediaEventSink_Notify(media_event_sink, EC_COMPLETE, S_OK,
            (LONG_PTR)&filter.IBaseFilter_iface);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    ok(WaitForSingleObject(event, 0) == 0, "Event should be signaled.\n");

    hr = IMediaControl_Stop(media_control);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    ok(WaitForSingleObject(event, 0) == 0, "Event should be signaled.\n");

    hr = IMediaControl_Pause(media_control);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    ok(WaitForSingleObject(event, 0) == 0, "Event should be signaled.\n");

    hr = IMediaControl_Run(media_control);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    ok(WaitForSingleObject(event, 0) == WAIT_TIMEOUT, "Event should not be signaled.\n");

    hr = IMediaEventSink_Notify(media_event_sink, EC_COMPLETE, S_OK,
            (LONG_PTR)&filter.IBaseFilter_iface);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    ok(WaitForSingleObject(event, 0) == 0, "Event should be signaled.\n");

    hr = IMediaEventEx_GetEvent(media_event, &code, &param1, &param2, 50);
    ok(hr == E_ABORT, "Got hr %#lx.\n", hr);

    ok(WaitForSingleObject(event, 0) == WAIT_TIMEOUT, "Event should not be signaled.\n");

    hr = IMediaEventEx_SetNotifyFlags(media_event, 0);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    flags = 0xdeadbeef;
    hr = IMediaEventEx_GetNotifyFlags(media_event, &flags);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(!flags, "Got flags %#lx\n", flags);

    hr = IMediaControl_Stop(media_control);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IMediaControl_Run(media_control);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IMediaEventSink_Notify(media_event_sink, EC_COMPLETE, S_OK,
            (LONG_PTR)&filter.IBaseFilter_iface);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IMediaEventEx_SetNotifyFlags(media_event, AM_MEDIAEVENT_NONOTIFY);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    ok(WaitForSingleObject(event, 0) == WAIT_TIMEOUT, "Event should not be signaled.\n");

    hr = IMediaControl_Stop(media_control);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    IMediaControl_Release(media_control);
    IMediaEventEx_Release(media_event);
    IMediaEventSink_Release(media_event_sink);
    ref = IFilterGraph2_Release(graph);
    ok(!ref, "Got outstanding refcount %ld.\n", ref);
    ok(filter.ref == 1, "Got outstanding refcount %ld.\n", filter.ref);

    SysFreeString(status);
    DestroyWindow(window);
}

#define check_events(a, b, c) check_events_(__LINE__, a, b, c)
static void check_events_(unsigned int line, IMediaEventEx *media_event,
        int expected_ec_complete_count, int expected_ec_status_count)
{
    int ec_complete_count = 0;
    int ec_status_count = 0;
    LONG_PTR param1, param2;
    HRESULT hr;
    LONG code;
    for (;;)
    {
        hr = IMediaEventEx_GetEvent(media_event, &code, &param1, &param2, 50);
        if (hr != S_OK)
            break;
        if (code == EC_COMPLETE)
            ++ec_complete_count;
        if (code == EC_STATUS)
            ++ec_status_count;
        hr = IMediaEventEx_FreeEventParams(media_event, code, param1, param2);
        ok(hr == S_OK, "Got hr %#lx.\n", hr);
    }
    ok(hr == E_ABORT, "Got hr %#lx.\n", hr);
    ok_(__FILE__, line)(ec_complete_count == expected_ec_complete_count,
        "Expected %d EC_COMPLETE events.\n", expected_ec_complete_count);
    ok_(__FILE__, line)(ec_status_count == expected_ec_status_count,
        "Expected %d EC_STATUS events.\n", expected_ec_status_count);
}

static void test_events(void)
{
    IFilterGraph2 *graph = create_graph();
    IMediaEventSink *media_event_sink;
    IMediaControl *media_control;
    IMediaEventEx *media_event;
    struct testfilter filter;
    LONG_PTR param1, param2;
    HANDLE event;
    BSTR status;
    HRESULT hr;
    ULONG ref;
    LONG code;

    status = SysAllocString(L"status");

    hr = IFilterGraph2_QueryInterface(graph, &IID_IMediaControl, (void **)&media_control);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IFilterGraph2_QueryInterface(graph, &IID_IMediaEventEx, (void **)&media_event);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IFilterGraph2_QueryInterface(graph, &IID_IMediaEventSink, (void **)&media_event_sink);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    testfilter_init(&filter, NULL, 0);
    filter.IAMFilterMiscFlags_iface.lpVtbl = &testmiscflags_vtbl;
    filter.misc_flags = AM_FILTER_MISC_FLAGS_IS_RENDERER;

    hr = IFilterGraph2_AddFilter(graph, &filter.IBaseFilter_iface, NULL);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IMediaEventEx_GetEventHandle(media_event, (OAEVENT *)&event);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    code = 0xdeadbeef;
    param1 = 0xdeadbeef;
    param2 = 0xdeadbeef;
    hr = IMediaEventEx_GetEvent(media_event, &code, &param1, &param2, 0);
    ok(hr == E_ABORT, "Got hr %#lx.\n", hr);
    ok(!code, "Got code %#lx.\n", code);
    todo_wine ok(!param1, "Got param1 %#Ix.\n", param1);
    todo_wine ok(!param2, "Got param2 %#Ix.\n", param2);

    /* EC_COMPLETE is ignored while in stopped or paused state. */

    hr = IMediaEventSink_Notify(media_event_sink, EC_STATUS, (LONG_PTR)status, (LONG_PTR)status);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IMediaEventSink_Notify(media_event_sink, EC_COMPLETE, S_OK,
            (LONG_PTR)&filter.IBaseFilter_iface);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IMediaEventSink_Notify(media_event_sink, EC_STATUS, (LONG_PTR)status, (LONG_PTR)status);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    check_events(media_event, 0, 2);

    hr = IMediaControl_Pause(media_control);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IMediaEventSink_Notify(media_event_sink, EC_STATUS, (LONG_PTR)status, (LONG_PTR)status);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IMediaEventSink_Notify(media_event_sink, EC_COMPLETE, S_OK,
            (LONG_PTR)&filter.IBaseFilter_iface);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IMediaEventSink_Notify(media_event_sink, EC_STATUS, (LONG_PTR)status, (LONG_PTR)status);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    check_events(media_event, 0, 2);

    hr = IMediaControl_Run(media_control);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    check_events(media_event, 0, 0);

    /* Pausing and then running the graph clears pending EC_COMPLETE events.
     * This remains true even with default handling canceled. */

    hr = IMediaEventEx_CancelDefaultHandling(media_event, EC_COMPLETE);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IMediaEventSink_Notify(media_event_sink, EC_STATUS, (LONG_PTR)status, (LONG_PTR)status);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IMediaEventSink_Notify(media_event_sink, EC_COMPLETE, S_OK,
            (LONG_PTR)&filter.IBaseFilter_iface);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IMediaEventSink_Notify(media_event_sink, EC_STATUS, (LONG_PTR)status, (LONG_PTR)status);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IMediaControl_Stop(media_control);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    check_events(media_event, 1, 2);

    hr = IMediaControl_Run(media_control);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IMediaEventSink_Notify(media_event_sink, EC_STATUS, (LONG_PTR)status, (LONG_PTR)status);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IMediaEventSink_Notify(media_event_sink, EC_COMPLETE, S_OK,
            (LONG_PTR)&filter.IBaseFilter_iface);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IMediaEventSink_Notify(media_event_sink, EC_STATUS, (LONG_PTR)status, (LONG_PTR)status);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IMediaControl_Stop(media_control);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IMediaControl_Run(media_control);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    check_events(media_event, 0, 2);

    hr = IMediaEventSink_Notify(media_event_sink, EC_STATUS, (LONG_PTR)status, (LONG_PTR)status);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IMediaEventSink_Notify(media_event_sink, EC_COMPLETE, S_OK,
            (LONG_PTR)&filter.IBaseFilter_iface);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IMediaEventSink_Notify(media_event_sink, EC_STATUS, (LONG_PTR)status, (LONG_PTR)status);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IMediaControl_Stop(media_control);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IMediaControl_Pause(media_control);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    check_events(media_event, 1, 2);

    hr = IMediaControl_Run(media_control);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IMediaEventSink_Notify(media_event_sink, EC_STATUS, (LONG_PTR)status, (LONG_PTR)status);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IMediaEventSink_Notify(media_event_sink, EC_COMPLETE, S_OK,
            (LONG_PTR)&filter.IBaseFilter_iface);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IMediaEventSink_Notify(media_event_sink, EC_STATUS, (LONG_PTR)status, (LONG_PTR)status);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IMediaControl_Pause(media_control);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IMediaControl_Run(media_control);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    check_events(media_event, 0, 2);

    /* GetEvent() resets the event object if there are no events available. */

    SetEvent(event);

    hr = IMediaEventEx_GetEvent(media_event, &code, &param1, &param2, 50);
    ok(hr == E_ABORT, "Got hr %#lx.\n", hr);

    ok(WaitForSingleObject(event, 0) == WAIT_TIMEOUT, "Event should not be signaled.\n");

    hr = IMediaControl_Stop(media_control);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    IMediaControl_Release(media_control);
    IMediaEventEx_Release(media_event);
    IMediaEventSink_Release(media_event_sink);
    ref = IFilterGraph2_Release(graph);
    ok(!ref, "Got outstanding refcount %ld.\n", ref);
    ok(filter.ref == 1, "Got outstanding refcount %ld.\n", filter.ref);

    SysFreeString(status);
}

static void test_event_dispatch(void)
{
    IFilterGraph2 *graph = create_graph();
    IMediaEventEx *event_ex;
    ITypeInfo *typeinfo;
    IMediaEvent *event;
    TYPEATTR *typeattr;
    unsigned int count;
    HRESULT hr;
    ULONG ref;

    IFilterGraph2_QueryInterface(graph, &IID_IMediaEvent, (void **)&event);
    IFilterGraph2_QueryInterface(graph, &IID_IMediaEventEx, (void **)&event_ex);
    ok((void *)event == event_ex, "Interface pointers didn't match.\n");
    IMediaEventEx_Release(event_ex);

    hr = IMediaEvent_GetTypeInfoCount(event, &count);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(count == 1, "Got count %u.\n", count);

    hr = IMediaEvent_GetTypeInfo(event, 0, 0, &typeinfo);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = ITypeInfo_GetTypeAttr(typeinfo, &typeattr);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(typeattr->typekind == TKIND_DISPATCH, "Got kind %u.\n", typeattr->typekind);
    ok(IsEqualGUID(&typeattr->guid, &IID_IMediaEvent), "Got IID %s.\n", debugstr_guid(&typeattr->guid));
    ITypeInfo_ReleaseTypeAttr(typeinfo, typeattr);
    ITypeInfo_Release(typeinfo);

    IMediaEvent_Release(event);
    ref = IFilterGraph2_Release(graph);
    ok(!ref, "Got outstanding refcount %ld.\n", ref);
}

START_TEST(filtergraph)
{
    CoInitializeEx(NULL, COINIT_MULTITHREADED);

    test_interfaces();
    test_render_run(L"test.avi", FALSE, TRUE);
    test_render_run(L"test.mpg", TRUE, TRUE);
    test_render_run(L"test.mp3", TRUE, FALSE);
    test_render_run(L"test.wav", TRUE, FALSE);
    test_enum_filters();
    test_graph_builder_render();
    test_graph_builder_connect();
    test_aggregation();
    test_control_delegation();
    test_add_remove_filter();
    test_connect_direct();
    test_sync_source();
    test_filter_state();
    test_ec_complete();
    test_renderfile_compressed();
    test_renderfile_failure();
    test_graph_seeking();
    test_default_sync_source();
    test_add_source_filter();
    test_window_threading();
    test_autoplug_uyvy();
    test_set_notify_flags();
    test_events();
    test_event_dispatch();

    CoUninitialize();
    test_render_with_multithread();
}
