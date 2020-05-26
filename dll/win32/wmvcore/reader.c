/*
 * Copyright 2012 Austin English
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

#include "wmvcore.h"

#include "wmsdk.h"
#include "wine/debug.h"
#include "wine/heap.h"

WINE_DEFAULT_DEBUG_CHANNEL(wmvcore);

typedef struct {
    IWMReader IWMReader_iface;
    IWMReaderAdvanced6 IWMReaderAdvanced6_iface;
    IWMReaderAccelerator IWMReaderAccelerator_iface;
    IWMReaderNetworkConfig2 IWMReaderNetworkConfig2_iface;
    IWMReaderStreamClock IWMReaderStreamClock_iface;
    IWMReaderTypeNegotiation IWMReaderTypeNegotiation_iface;
    IWMReaderTimecode IWMReaderTimecode_iface;
    IWMReaderPlaylistBurn IWMReaderPlaylistBurn_iface;
    IWMHeaderInfo3 IWMHeaderInfo3_iface;
    IWMLanguageList IWMLanguageList_iface;
    IReferenceClock IReferenceClock_iface;
    IWMProfile3 IWMProfile3_iface;
    IWMPacketSize2 IWMPacketSize2_iface;
    LONG ref;
} WMReader;

static inline WMReader *impl_from_IWMReader(IWMReader *iface)
{
    return CONTAINING_RECORD(iface, WMReader, IWMReader_iface);
}

static HRESULT WINAPI WMReader_QueryInterface(IWMReader *iface, REFIID riid, void **ppv)
{
    WMReader *This = impl_from_IWMReader(iface);

    if(IsEqualGUID(riid, &IID_IUnknown)) {
        TRACE("(%p)->(IID_IUnknown %p)\n", This, ppv);
        *ppv = &This->IWMReader_iface;
    }else if(IsEqualGUID(riid, &IID_IWMReader)) {
        TRACE("(%p)->(IID_IWMReader %p)\n", This, ppv);
        *ppv = &This->IWMReader_iface;
    }else if(IsEqualGUID(riid, &IID_IWMReaderAdvanced)) {
        TRACE("(%p)->(IID_IWMReaderAdvanced %p)\n", This, ppv);
        *ppv = &This->IWMReaderAdvanced6_iface;
    }else if(IsEqualGUID(riid, &IID_IWMReaderAdvanced2)) {
        TRACE("(%p)->(IID_IWMReaderAdvanced2 %p)\n", This, ppv);
        *ppv = &This->IWMReaderAdvanced6_iface;
    }else if(IsEqualGUID(riid, &IID_IWMReaderAdvanced3)) {
        TRACE("(%p)->(IID_IWMReaderAdvanced3 %p)\n", This, ppv);
        *ppv = &This->IWMReaderAdvanced6_iface;
    }else if(IsEqualGUID(riid, &IID_IWMReaderAdvanced4)) {
        TRACE("(%p)->(IID_IWMReaderAdvanced4 %p)\n", This, ppv);
        *ppv = &This->IWMReaderAdvanced6_iface;
    }else if(IsEqualGUID(riid, &IID_IWMReaderAdvanced5)) {
        TRACE("(%p)->(IID_IWMReaderAdvanced5 %p)\n", This, ppv);
        *ppv = &This->IWMReaderAdvanced6_iface;
    }else if(IsEqualGUID(riid, &IID_IWMReaderAdvanced6)) {
        TRACE("(%p)->(IID_IWMReaderAdvanced6 %p)\n", This, ppv);
        *ppv = &This->IWMReaderAdvanced6_iface;
    }else if(IsEqualGUID(riid, &IID_IWMReaderAccelerator)) {
        TRACE("(%p)->(IID_IWMReaderAccelerator %p)\n", This, ppv);
        *ppv = &This->IWMReaderAccelerator_iface;
    }else if(IsEqualGUID(riid, &IID_IWMReaderNetworkConfig)) {
        TRACE("(%p)->(IWMReaderNetworkConfig %p)\n", This, ppv);
        *ppv = &This->IWMReaderNetworkConfig2_iface;
    }else if(IsEqualGUID(riid, &IID_IWMReaderNetworkConfig2)) {
        TRACE("(%p)->(IWMReaderNetworkConfig2 %p)\n", This, ppv);
        *ppv = &This->IWMReaderNetworkConfig2_iface;
    }else if(IsEqualGUID(riid, &IID_IWMReaderStreamClock)) {
        TRACE("(%p)->(IWMReaderStreamClock %p)\n", This, ppv);
        *ppv = &This->IWMReaderStreamClock_iface;
    }else if(IsEqualGUID(riid, &IID_IWMReaderTypeNegotiation)) {
        TRACE("(%p)->(IWMReaderTypeNegotiation %p)\n", This, ppv);
        *ppv = &This->IWMReaderTypeNegotiation_iface;
    }else if(IsEqualGUID(riid, &IID_IWMReaderTimecode)) {
        TRACE("(%p)->(IWMReaderTimecode %p)\n", This, ppv);
        *ppv = &This->IWMReaderTimecode_iface;
    }else if(IsEqualGUID(riid, &IID_IWMReaderPlaylistBurn)) {
        TRACE("(%p)->(IWMReaderPlaylistBurn %p)\n", This, ppv);
        *ppv = &This->IWMReaderPlaylistBurn_iface;
    }else if(IsEqualGUID(riid, &IID_IWMHeaderInfo)) {
        TRACE("(%p)->(IWMHeaderInfo %p)\n", This, ppv);
        *ppv = &This->IWMHeaderInfo3_iface;
    }else if(IsEqualGUID(riid, &IID_IWMHeaderInfo2)) {
        TRACE("(%p)->(IWMHeaderInfo2 %p)\n", This, ppv);
        *ppv = &This->IWMHeaderInfo3_iface;
    }else if(IsEqualGUID(riid, &IID_IWMHeaderInfo3)) {
        TRACE("(%p)->(IWMHeaderInfo3 %p)\n", This, ppv);
        *ppv = &This->IWMHeaderInfo3_iface;
    }else if(IsEqualGUID(riid, &IID_IWMLanguageList)) {
        TRACE("(%p)->(IWMLanguageList %p)\n", This, ppv);
        *ppv = &This->IWMLanguageList_iface;
    }else if(IsEqualGUID(riid, &IID_IReferenceClock)) {
        TRACE("(%p)->(IWMLanguageList %p)\n", This, ppv);
        *ppv = &This->IReferenceClock_iface;
    }else if(IsEqualGUID(riid, &IID_IWMProfile)) {
        TRACE("(%p)->(IWMProfile %p)\n", This, ppv);
        *ppv = &This->IWMProfile3_iface;
    }else if(IsEqualGUID(riid, &IID_IWMProfile2)) {
        TRACE("(%p)->(IWMProfile2 %p)\n", This, ppv);
        *ppv = &This->IWMProfile3_iface;
    }else if(IsEqualGUID(riid, &IID_IWMProfile3)) {
        TRACE("(%p)->(IWMProfile3 %p)\n", This, ppv);
        *ppv = &This->IWMProfile3_iface;
    }else if(IsEqualGUID(riid, &IID_IWMPacketSize)) {
        TRACE("(%p)->(IWMPacketSize %p)\n", This, ppv);
        *ppv = &This->IWMPacketSize2_iface;
    }else if(IsEqualGUID(riid, &IID_IWMPacketSize2)) {
        TRACE("(%p)->(IWMPacketSize2 %p)\n", This, ppv);
        *ppv = &This->IWMPacketSize2_iface;
    }else {
        *ppv = NULL;
        FIXME("(%p)->(%s %p)\n", This, debugstr_guid(riid), ppv);
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI WMReader_AddRef(IWMReader *iface)
{
    WMReader *This = impl_from_IWMReader(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    return ref;
}

static ULONG WINAPI WMReader_Release(IWMReader *iface)
{
    WMReader *This = impl_from_IWMReader(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    if(!ref)
        heap_free(This);

    return ref;
}

static HRESULT WINAPI WMReader_Open(IWMReader *iface, const WCHAR *url, IWMReaderCallback *callback, void *context)
{
    WMReader *This = impl_from_IWMReader(iface);
    FIXME("(%p)->(%s %p %p)\n", This, debugstr_w(url), callback, context);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMReader_Close(IWMReader *iface)
{
    WMReader *This = impl_from_IWMReader(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMReader_GetOutputCount(IWMReader *iface, DWORD *outputs)
{
    WMReader *This = impl_from_IWMReader(iface);
    FIXME("(%p)->(%p)\n", This, outputs);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMReader_GetOutputProps(IWMReader *iface, DWORD output_num, IWMOutputMediaProps **output)
{
    WMReader *This = impl_from_IWMReader(iface);
    FIXME("(%p)->(%u %p)\n", This, output_num, output);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMReader_SetOutputProps(IWMReader *iface, DWORD output_num, IWMOutputMediaProps *output)
{
    WMReader *This = impl_from_IWMReader(iface);
    FIXME("(%p)->(%u %p)\n", This, output_num, output);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMReader_GetOutputFormatCount(IWMReader *iface, DWORD output_num, DWORD *formats)
{
    WMReader *This = impl_from_IWMReader(iface);
    FIXME("(%p)->(%u %p)\n", This, output_num, formats);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMReader_GetOutputFormat(IWMReader *iface, DWORD output_num, DWORD format_num, IWMOutputMediaProps **props)
{
    WMReader *This = impl_from_IWMReader(iface);
    FIXME("(%p)->(%u %u %p)\n", This, output_num, format_num, props);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMReader_Start(IWMReader *iface, QWORD start, QWORD duration, float rate, void *context)
{
    WMReader *This = impl_from_IWMReader(iface);
    FIXME("(%p)->(%s %s %f %p)\n", This, wine_dbgstr_longlong(start), wine_dbgstr_longlong(duration), rate, context);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMReader_Stop(IWMReader *iface)
{
    WMReader *This = impl_from_IWMReader(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMReader_Pause(IWMReader *iface)
{
    WMReader *This = impl_from_IWMReader(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMReader_Resume(IWMReader *iface)
{
    WMReader *This = impl_from_IWMReader(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static const IWMReaderVtbl WMReaderVtbl = {
    WMReader_QueryInterface,
    WMReader_AddRef,
    WMReader_Release,
    WMReader_Open,
    WMReader_Close,
    WMReader_GetOutputCount,
    WMReader_GetOutputProps,
    WMReader_SetOutputProps,
    WMReader_GetOutputFormatCount,
    WMReader_GetOutputFormat,
    WMReader_Start,
    WMReader_Stop,
    WMReader_Pause,
    WMReader_Resume
};

static inline WMReader *impl_from_IWMReaderAdvanced6(IWMReaderAdvanced6 *iface)
{
    return CONTAINING_RECORD(iface, WMReader, IWMReaderAdvanced6_iface);
}

static HRESULT WINAPI WMReaderAdvanced_QueryInterface(IWMReaderAdvanced6 *iface, REFIID riid, void **ppv)
{
    WMReader *This = impl_from_IWMReaderAdvanced6(iface);
    return IWMReader_QueryInterface(&This->IWMReader_iface, riid, ppv);
}

static ULONG WINAPI WMReaderAdvanced_AddRef(IWMReaderAdvanced6 *iface)
{
    WMReader *This = impl_from_IWMReaderAdvanced6(iface);
    return IWMReader_AddRef(&This->IWMReader_iface);
}

static ULONG WINAPI WMReaderAdvanced_Release(IWMReaderAdvanced6 *iface)
{
    WMReader *This = impl_from_IWMReaderAdvanced6(iface);
    return IWMReader_Release(&This->IWMReader_iface);
}

static HRESULT WINAPI WMReaderAdvanced_SetUserProvidedClock(IWMReaderAdvanced6 *iface, BOOL user_clock)
{
    WMReader *This = impl_from_IWMReaderAdvanced6(iface);
    FIXME("(%p)->(%x)\n", This, user_clock);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMReaderAdvanced_GetUserProvidedClock(IWMReaderAdvanced6 *iface, BOOL *user_clock)
{
    WMReader *This = impl_from_IWMReaderAdvanced6(iface);
    FIXME("(%p)->(%p)\n", This, user_clock);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMReaderAdvanced_DeliverTime(IWMReaderAdvanced6 *iface, QWORD time)
{
    WMReader *This = impl_from_IWMReaderAdvanced6(iface);
    FIXME("(%p)->(%s)\n", This, wine_dbgstr_longlong(time));
    return E_NOTIMPL;
}

static HRESULT WINAPI WMReaderAdvanced_SetManualStreamSelection(IWMReaderAdvanced6 *iface, BOOL selection)
{
    WMReader *This = impl_from_IWMReaderAdvanced6(iface);
    FIXME("(%p)->(%x)\n", This, selection);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMReaderAdvanced_GetManualStreamSelection(IWMReaderAdvanced6 *iface, BOOL *selection)
{
    WMReader *This = impl_from_IWMReaderAdvanced6(iface);
    FIXME("(%p)->(%p)\n", This, selection);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMReaderAdvanced_SetStreamsSelected(IWMReaderAdvanced6 *iface, WORD stream_count,
        WORD *stream_numbers, WMT_STREAM_SELECTION *selections)
{
    WMReader *This = impl_from_IWMReaderAdvanced6(iface);
    FIXME("(%p)->(%d %p %p)\n", This, stream_count, stream_numbers, selections);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMReaderAdvanced_GetStreamSelected(IWMReaderAdvanced6 *iface, WORD stream_num,
        WMT_STREAM_SELECTION *selection)
{
    WMReader *This = impl_from_IWMReaderAdvanced6(iface);
    FIXME("(%p)->(%d %p)\n", This, stream_num, selection);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMReaderAdvanced_SetReceiveSelectionCallbacks(IWMReaderAdvanced6 *iface, BOOL get_callbacks)
{
    WMReader *This = impl_from_IWMReaderAdvanced6(iface);
    FIXME("(%p)->(%x)\n", This, get_callbacks);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMReaderAdvanced_GetReceiveSelectionCallbacks(IWMReaderAdvanced6 *iface, BOOL *get_callbacks)
{
    WMReader *This = impl_from_IWMReaderAdvanced6(iface);
    FIXME("(%p)->(%p)\n", This, get_callbacks);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMReaderAdvanced_SetReceiveStreamSamples(IWMReaderAdvanced6 *iface, WORD stream_num,
        BOOL receive_stream_samples)
{
    WMReader *This = impl_from_IWMReaderAdvanced6(iface);
    FIXME("(%p)->(%d %x)\n", This, stream_num, receive_stream_samples);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMReaderAdvanced_GetReceiveStreamSamples(IWMReaderAdvanced6 *iface, WORD stream_num,
        BOOL *receive_stream_samples)
{
    WMReader *This = impl_from_IWMReaderAdvanced6(iface);
    FIXME("(%p)->(%d %p)\n", This, stream_num, receive_stream_samples);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMReaderAdvanced_SetAllocateForOutput(IWMReaderAdvanced6 *iface, DWORD output_num, BOOL allocate)
{
    WMReader *This = impl_from_IWMReaderAdvanced6(iface);
    FIXME("(%p)->(%d %x)\n", This, output_num, allocate);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMReaderAdvanced_GetAllocateForOutput(IWMReaderAdvanced6 *iface, DWORD output_num, BOOL *allocate)
{
    WMReader *This = impl_from_IWMReaderAdvanced6(iface);
    FIXME("(%p)->(%d %p)\n", This, output_num, allocate);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMReaderAdvanced_SetAllocateForStream(IWMReaderAdvanced6 *iface, WORD output_num, BOOL allocate)
{
    WMReader *This = impl_from_IWMReaderAdvanced6(iface);
    FIXME("(%p)->(%d %x)\n", This, output_num, allocate);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMReaderAdvanced_GetAllocateForStream(IWMReaderAdvanced6 *iface, WORD output_num, BOOL *allocate)
{
    WMReader *This = impl_from_IWMReaderAdvanced6(iface);
    FIXME("(%p)->(%d %p)\n", This, output_num, allocate);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMReaderAdvanced_GetStatistics(IWMReaderAdvanced6 *iface, WM_READER_STATISTICS *statistics)
{
    WMReader *This = impl_from_IWMReaderAdvanced6(iface);
    FIXME("(%p)->(%p)\n", This, statistics);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMReaderAdvanced_SetClientInfo(IWMReaderAdvanced6 *iface, WM_READER_CLIENTINFO *client_info)
{
    WMReader *This = impl_from_IWMReaderAdvanced6(iface);
    FIXME("(%p)->(%p)\n", This, client_info);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMReaderAdvanced_GetMaxOutputSampleSize(IWMReaderAdvanced6 *iface, DWORD output, DWORD *max)
{
    WMReader *This = impl_from_IWMReaderAdvanced6(iface);
    FIXME("(%p)->(%d %p)\n", This, output, max);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMReaderAdvanced_GetMaxStreamSampleSize(IWMReaderAdvanced6 *iface, WORD stream, DWORD *max)
{
    WMReader *This = impl_from_IWMReaderAdvanced6(iface);
    FIXME("(%p)->(%d %p)\n", This, stream, max);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMReaderAdvanced_NotifyLateDelivery(IWMReaderAdvanced6 *iface, QWORD lateness)
{
    WMReader *This = impl_from_IWMReaderAdvanced6(iface);
    FIXME("(%p)->(%s)\n", This, wine_dbgstr_longlong(lateness));
    return E_NOTIMPL;
}

static HRESULT WINAPI WMReaderAdvanced2_SetPlayMode(IWMReaderAdvanced6 *iface, WMT_PLAY_MODE mode)
{
    WMReader *This = impl_from_IWMReaderAdvanced6(iface);
    FIXME("(%p)->(%d)\n", This, mode);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMReaderAdvanced2_GetPlayMode(IWMReaderAdvanced6 *iface, WMT_PLAY_MODE *mode)
{
    WMReader *This = impl_from_IWMReaderAdvanced6(iface);
    FIXME("(%p)->(%p)\n", This, mode);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMReaderAdvanced2_GetBufferProgress(IWMReaderAdvanced6 *iface, DWORD *percent, QWORD *buffering)
{
    WMReader *This = impl_from_IWMReaderAdvanced6(iface);
    FIXME("(%p)->(%p %p)\n", This, percent, buffering);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMReaderAdvanced2_GetDownloadProgress(IWMReaderAdvanced6 *iface, DWORD *percent,
        QWORD *bytes_downloaded, QWORD *download)
{
    WMReader *This = impl_from_IWMReaderAdvanced6(iface);
    FIXME("(%p)->(%p %p %p)\n", This, percent, bytes_downloaded, download);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMReaderAdvanced2_GetSaveAsProgress(IWMReaderAdvanced6 *iface, DWORD *percent)
{
    WMReader *This = impl_from_IWMReaderAdvanced6(iface);
    FIXME("(%p)->(%p)\n", This, percent);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMReaderAdvanced2_SaveFileAs(IWMReaderAdvanced6 *iface, const WCHAR *filename)
{
    WMReader *This = impl_from_IWMReaderAdvanced6(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(filename));
    return E_NOTIMPL;
}

static HRESULT WINAPI WMReaderAdvanced2_GetProtocolName(IWMReaderAdvanced6 *iface, WCHAR *protocol, DWORD *protocol_len)
{
    WMReader *This = impl_from_IWMReaderAdvanced6(iface);
    FIXME("(%p)->(%p %p)\n", This, protocol, protocol_len);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMReaderAdvanced2_StartAtMarker(IWMReaderAdvanced6 *iface, WORD marker_index,
        QWORD duration, float rate, void *context)
{
    WMReader *This = impl_from_IWMReaderAdvanced6(iface);
    FIXME("(%p)->(%d %s %f %p)\n", This, marker_index, wine_dbgstr_longlong(duration), rate, context);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMReaderAdvanced2_GetOutputSetting(IWMReaderAdvanced6 *iface, DWORD output_num,
        const WCHAR *name, WMT_ATTR_DATATYPE *type, BYTE *value, WORD *length)
{
    WMReader *This = impl_from_IWMReaderAdvanced6(iface);
    FIXME("(%p)->(%d %s %p %p %p)\n", This, output_num, debugstr_w(name), type, value, length);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMReaderAdvanced2_SetOutputSetting(IWMReaderAdvanced6 *iface, DWORD output_num,
        const WCHAR *name, WMT_ATTR_DATATYPE type, const BYTE *value, WORD length)
{
    WMReader *This = impl_from_IWMReaderAdvanced6(iface);
    FIXME("(%p)->(%d %s %d %p %d)\n", This, output_num, debugstr_w(name), type, value, length);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMReaderAdvanced2_Preroll(IWMReaderAdvanced6 *iface, QWORD start, QWORD duration, float rate)
{
    WMReader *This = impl_from_IWMReaderAdvanced6(iface);
    FIXME("(%p)->(%s %s %f)\n", This, wine_dbgstr_longlong(start), wine_dbgstr_longlong(duration), rate);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMReaderAdvanced2_SetLogClientID(IWMReaderAdvanced6 *iface, BOOL log_client_id)
{
    WMReader *This = impl_from_IWMReaderAdvanced6(iface);
    FIXME("(%p)->(%x)\n", This, log_client_id);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMReaderAdvanced2_GetLogClientID(IWMReaderAdvanced6 *iface, BOOL *log_client_id)
{
    WMReader *This = impl_from_IWMReaderAdvanced6(iface);
    FIXME("(%p)->(%p)\n", This, log_client_id);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMReaderAdvanced2_StopBuffering(IWMReaderAdvanced6 *iface)
{
    WMReader *This = impl_from_IWMReaderAdvanced6(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMReaderAdvanced2_OpenStream(IWMReaderAdvanced6 *iface, IStream *stream,
        IWMReaderCallback *callback, void *context)
{
    WMReader *This = impl_from_IWMReaderAdvanced6(iface);
    FIXME("(%p)->(%p %p %p)\n", This, stream, callback, context);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMReaderAdvanced3_StopNetStreaming(IWMReaderAdvanced6 *iface)
{
    WMReader *This = impl_from_IWMReaderAdvanced6(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMReaderAdvanced3_StartAtPosition(IWMReaderAdvanced6 *iface, WORD stream_num,
        void *offset_start, void *duration, WMT_OFFSET_FORMAT format, float rate, void *context)
{
    WMReader *This = impl_from_IWMReaderAdvanced6(iface);
    FIXME("(%p)->(%d %p %p %d %f %p)\n", This, stream_num, offset_start, duration, format, rate, context);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMReaderAdvanced4_GetLanguageCount(IWMReaderAdvanced6 *iface, DWORD output_num, WORD *language_count)
{
    WMReader *This = impl_from_IWMReaderAdvanced6(iface);
    FIXME("(%p)->(%d %p)\n", This, output_num, language_count);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMReaderAdvanced4_GetLanguage(IWMReaderAdvanced6 *iface, DWORD output_num,
       WORD language, WCHAR *language_string, WORD *language_string_len)
{
    WMReader *This = impl_from_IWMReaderAdvanced6(iface);
    FIXME("(%p)->(%d %x %p %p)\n", This, output_num, language, language_string, language_string_len);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMReaderAdvanced4_GetMaxSpeedFactor(IWMReaderAdvanced6 *iface, double *factor)
{
    WMReader *This = impl_from_IWMReaderAdvanced6(iface);
    FIXME("(%p)->(%p)\n", This, factor);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMReaderAdvanced4_IsUsingFastCache(IWMReaderAdvanced6 *iface, BOOL *using_fast_cache)
{
    WMReader *This = impl_from_IWMReaderAdvanced6(iface);
    FIXME("(%p)->(%p)\n", This, using_fast_cache);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMReaderAdvanced4_AddLogParam(IWMReaderAdvanced6 *iface, const WCHAR *namespace,
        const WCHAR *name, const WCHAR *value)
{
    WMReader *This = impl_from_IWMReaderAdvanced6(iface);
    FIXME("(%p)->(%s %s %s)\n", This, debugstr_w(namespace), debugstr_w(name), debugstr_w(value));
    return E_NOTIMPL;
}

static HRESULT WINAPI WMReaderAdvanced4_SendLogParams(IWMReaderAdvanced6 *iface)
{
    WMReader *This = impl_from_IWMReaderAdvanced6(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMReaderAdvanced4_CanSaveFileAs(IWMReaderAdvanced6 *iface, BOOL *can_save)
{
    WMReader *This = impl_from_IWMReaderAdvanced6(iface);
    FIXME("(%p)->(%p)\n", This, can_save);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMReaderAdvanced4_CancelSaveFileAs(IWMReaderAdvanced6 *iface)
{
    WMReader *This = impl_from_IWMReaderAdvanced6(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMReaderAdvanced4_GetURL(IWMReaderAdvanced6 *iface, WCHAR *url, DWORD *url_len)
{
    WMReader *This = impl_from_IWMReaderAdvanced6(iface);
    FIXME("(%p)->(%p %p)\n", This, url, url_len);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMReaderAdvanced5_SetPlayerHook(IWMReaderAdvanced6 *iface, DWORD output_num, IWMPlayerHook *hook)
{
    WMReader *This = impl_from_IWMReaderAdvanced6(iface);
    FIXME("(%p)->(%d %p)\n", This, output_num, hook);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMReaderAdvanced6_SetProtextStreamSamples(IWMReaderAdvanced6 *iface, BYTE *cert,
        DWORD cert_size, DWORD cert_type, DWORD flags, BYTE *initialization_vector, DWORD *initialization_vector_size)
{
    WMReader *This = impl_from_IWMReaderAdvanced6(iface);
    FIXME("(%p)->(%p %d %d %x %p %p)\n", This, cert, cert_size, cert_type, flags, initialization_vector,
          initialization_vector_size);
    return E_NOTIMPL;
}

static const IWMReaderAdvanced6Vtbl WMReaderAdvanced6Vtbl = {
    WMReaderAdvanced_QueryInterface,
    WMReaderAdvanced_AddRef,
    WMReaderAdvanced_Release,
    WMReaderAdvanced_SetUserProvidedClock,
    WMReaderAdvanced_GetUserProvidedClock,
    WMReaderAdvanced_DeliverTime,
    WMReaderAdvanced_SetManualStreamSelection,
    WMReaderAdvanced_GetManualStreamSelection,
    WMReaderAdvanced_SetStreamsSelected,
    WMReaderAdvanced_GetStreamSelected,
    WMReaderAdvanced_SetReceiveSelectionCallbacks,
    WMReaderAdvanced_GetReceiveSelectionCallbacks,
    WMReaderAdvanced_SetReceiveStreamSamples,
    WMReaderAdvanced_GetReceiveStreamSamples,
    WMReaderAdvanced_SetAllocateForOutput,
    WMReaderAdvanced_GetAllocateForOutput,
    WMReaderAdvanced_SetAllocateForStream,
    WMReaderAdvanced_GetAllocateForStream,
    WMReaderAdvanced_GetStatistics,
    WMReaderAdvanced_SetClientInfo,
    WMReaderAdvanced_GetMaxOutputSampleSize,
    WMReaderAdvanced_GetMaxStreamSampleSize,
    WMReaderAdvanced_NotifyLateDelivery,
    WMReaderAdvanced2_SetPlayMode,
    WMReaderAdvanced2_GetPlayMode,
    WMReaderAdvanced2_GetBufferProgress,
    WMReaderAdvanced2_GetDownloadProgress,
    WMReaderAdvanced2_GetSaveAsProgress,
    WMReaderAdvanced2_SaveFileAs,
    WMReaderAdvanced2_GetProtocolName,
    WMReaderAdvanced2_StartAtMarker,
    WMReaderAdvanced2_GetOutputSetting,
    WMReaderAdvanced2_SetOutputSetting,
    WMReaderAdvanced2_Preroll,
    WMReaderAdvanced2_SetLogClientID,
    WMReaderAdvanced2_GetLogClientID,
    WMReaderAdvanced2_StopBuffering,
    WMReaderAdvanced2_OpenStream,
    WMReaderAdvanced3_StopNetStreaming,
    WMReaderAdvanced3_StartAtPosition,
    WMReaderAdvanced4_GetLanguageCount,
    WMReaderAdvanced4_GetLanguage,
    WMReaderAdvanced4_GetMaxSpeedFactor,
    WMReaderAdvanced4_IsUsingFastCache,
    WMReaderAdvanced4_AddLogParam,
    WMReaderAdvanced4_SendLogParams,
    WMReaderAdvanced4_CanSaveFileAs,
    WMReaderAdvanced4_CancelSaveFileAs,
    WMReaderAdvanced4_GetURL,
    WMReaderAdvanced5_SetPlayerHook,
    WMReaderAdvanced6_SetProtextStreamSamples
};

static inline WMReader *impl_from_IWMReaderAccelerator(IWMReaderAccelerator *iface)
{
    return CONTAINING_RECORD(iface, WMReader, IWMReaderAccelerator_iface);
}

static HRESULT WINAPI reader_accl_QueryInterface(IWMReaderAccelerator *iface, REFIID riid, void **object)
{
    WMReader *This = impl_from_IWMReaderAccelerator(iface);
    return IWMReader_QueryInterface(&This->IWMReader_iface, riid, object);
}

static ULONG WINAPI reader_accl_AddRef(IWMReaderAccelerator *iface)
{
    WMReader *This = impl_from_IWMReaderAccelerator(iface);
    return IWMReader_AddRef(&This->IWMReader_iface);
}

static ULONG WINAPI reader_accl_Release(IWMReaderAccelerator *iface)
{
    WMReader *This = impl_from_IWMReaderAccelerator(iface);
    return IWMReader_Release(&This->IWMReader_iface);
}

static HRESULT WINAPI reader_accl_GetCodecInterface(IWMReaderAccelerator *iface, DWORD output, REFIID riid, void **codec)
{
    WMReader *This = impl_from_IWMReaderAccelerator(iface);

    FIXME("%p, %d, %s, %p\n", This, output, debugstr_guid(riid), codec);

    return E_NOTIMPL;
}

static HRESULT WINAPI reader_accl_Notify(IWMReaderAccelerator *iface, DWORD output, WM_MEDIA_TYPE *subtype)
{
    WMReader *This = impl_from_IWMReaderAccelerator(iface);

    FIXME("%p, %d, %p\n", This, output, subtype);

    return E_NOTIMPL;
}

static const IWMReaderAcceleratorVtbl WMReaderAcceleratorVtbl = {
    reader_accl_QueryInterface,
    reader_accl_AddRef,
    reader_accl_Release,
    reader_accl_GetCodecInterface,
    reader_accl_Notify
};

static inline WMReader *impl_from_IWMReaderNetworkConfig2(IWMReaderNetworkConfig2 *iface)
{
    return CONTAINING_RECORD(iface, WMReader, IWMReaderNetworkConfig2_iface);
}

static HRESULT WINAPI networkconfig_QueryInterface(IWMReaderNetworkConfig2 *iface, REFIID riid, void **ppv)
{
    WMReader *This = impl_from_IWMReaderNetworkConfig2(iface);
    return IWMReader_QueryInterface(&This->IWMReader_iface, riid, ppv);
}

static ULONG WINAPI networkconfig_AddRef(IWMReaderNetworkConfig2 *iface)
{
    WMReader *This = impl_from_IWMReaderNetworkConfig2(iface);
    return IWMReader_AddRef(&This->IWMReader_iface);
}

static ULONG WINAPI networkconfig_Release(IWMReaderNetworkConfig2 *iface)
{
    WMReader *This = impl_from_IWMReaderNetworkConfig2(iface);
    return IWMReader_Release(&This->IWMReader_iface);
}

static HRESULT WINAPI networkconfig_GetBufferingTime(IWMReaderNetworkConfig2 *iface, QWORD *buffering_time)
{
    WMReader *This = impl_from_IWMReaderNetworkConfig2(iface);
    FIXME("%p, %p\n", This, buffering_time);
    return E_NOTIMPL;
}

static HRESULT WINAPI networkconfig_SetBufferingTime(IWMReaderNetworkConfig2 *iface, QWORD buffering_time)
{
    WMReader *This = impl_from_IWMReaderNetworkConfig2(iface);
    FIXME("%p, %s\n", This, wine_dbgstr_longlong(buffering_time));
    return E_NOTIMPL;
}

static HRESULT WINAPI networkconfig_GetUDPPortRanges(IWMReaderNetworkConfig2 *iface, WM_PORT_NUMBER_RANGE *array,
        DWORD *ranges)
{
    WMReader *This = impl_from_IWMReaderNetworkConfig2(iface);
    FIXME("%p, %p, %p\n", This, array, ranges);
    return E_NOTIMPL;
}

static HRESULT WINAPI networkconfig_SetUDPPortRanges(IWMReaderNetworkConfig2 *iface, WM_PORT_NUMBER_RANGE *array,
        DWORD ranges)
{
    WMReader *This = impl_from_IWMReaderNetworkConfig2(iface);
    FIXME("%p, %p, %u\n", This, array, ranges);
    return E_NOTIMPL;
}

static HRESULT WINAPI networkconfig_GetProxySettings(IWMReaderNetworkConfig2 *iface, const WCHAR *protocol,
        WMT_PROXY_SETTINGS *proxy)
{
    WMReader *This = impl_from_IWMReaderNetworkConfig2(iface);
    FIXME("%p, %s, %p\n", This, debugstr_w(protocol), proxy);
    return E_NOTIMPL;
}

static HRESULT WINAPI networkconfig_SetProxySettings(IWMReaderNetworkConfig2 *iface, LPCWSTR protocol,
        WMT_PROXY_SETTINGS proxy)
{
    WMReader *This = impl_from_IWMReaderNetworkConfig2(iface);
    FIXME("%p, %s, %d\n", This, debugstr_w(protocol), proxy);
    return E_NOTIMPL;
}

static HRESULT WINAPI networkconfig_GetProxyHostName(IWMReaderNetworkConfig2 *iface, const WCHAR *protocol,
        WCHAR *hostname, DWORD *size)
{
    WMReader *This = impl_from_IWMReaderNetworkConfig2(iface);
    FIXME("%p, %s, %p, %p\n", This, debugstr_w(protocol), hostname, size);
    return E_NOTIMPL;
}

static HRESULT WINAPI networkconfig_SetProxyHostName(IWMReaderNetworkConfig2 *iface, const WCHAR *protocol,
        const WCHAR *hostname)
{
    WMReader *This = impl_from_IWMReaderNetworkConfig2(iface);
    FIXME("%p, %s, %s\n", This, debugstr_w(protocol), debugstr_w(hostname));
    return E_NOTIMPL;
}

static HRESULT WINAPI networkconfig_GetProxyPort(IWMReaderNetworkConfig2 *iface, const WCHAR *protocol,
        DWORD *port)
{
    WMReader *This = impl_from_IWMReaderNetworkConfig2(iface);
    FIXME("%p, %s, %p\n", This, debugstr_w(protocol), port);
    return E_NOTIMPL;
}

static HRESULT WINAPI networkconfig_SetProxyPort(IWMReaderNetworkConfig2 *iface, const WCHAR *protocol,
        DWORD port)
{
    WMReader *This = impl_from_IWMReaderNetworkConfig2(iface);
    FIXME("%p, %s, %u\n", This, debugstr_w(protocol), port);
    return E_NOTIMPL;
}

static HRESULT WINAPI networkconfig_GetProxyExceptionList(IWMReaderNetworkConfig2 *iface, const WCHAR *protocol,
        WCHAR *exceptions, DWORD *count)
{
    WMReader *This = impl_from_IWMReaderNetworkConfig2(iface);
    FIXME("%p, %s, %p, %p\n", This, debugstr_w(protocol), exceptions, count);
    return E_NOTIMPL;
}

static HRESULT WINAPI networkconfig_SetProxyExceptionList(IWMReaderNetworkConfig2 *iface, const WCHAR *protocol,
        const WCHAR *exceptions)
{
    WMReader *This = impl_from_IWMReaderNetworkConfig2(iface);
    FIXME("%p, %s, %s\n", This, debugstr_w(protocol), debugstr_w(exceptions));
    return E_NOTIMPL;
}

static HRESULT WINAPI networkconfig_GetProxyBypassForLocal(IWMReaderNetworkConfig2 *iface, const WCHAR *protocol,
        BOOL *bypass)
{
    WMReader *This = impl_from_IWMReaderNetworkConfig2(iface);
    FIXME("%p, %s, %p\n", This, debugstr_w(protocol), bypass);
    return E_NOTIMPL;
}

static HRESULT WINAPI networkconfig_SetProxyBypassForLocal(IWMReaderNetworkConfig2 *iface, const WCHAR *protocol,
        BOOL bypass)
{
    WMReader *This = impl_from_IWMReaderNetworkConfig2(iface);
    FIXME("%p, %s, %d\n", This, debugstr_w(protocol), bypass);
    return E_NOTIMPL;
}

static HRESULT WINAPI networkconfig_GetForceRerunAutoProxyDetection(IWMReaderNetworkConfig2 *iface,
        BOOL *detection)
{
    WMReader *This = impl_from_IWMReaderNetworkConfig2(iface);
    FIXME("%p, %p\n", This, detection);
    return E_NOTIMPL;
}

static HRESULT WINAPI networkconfig_SetForceRerunAutoProxyDetection(IWMReaderNetworkConfig2 *iface,
        BOOL detection)
{
    WMReader *This = impl_from_IWMReaderNetworkConfig2(iface);
    FIXME("%p, %d\n", This, detection);
    return E_NOTIMPL;
}

static HRESULT WINAPI networkconfig_GetEnableMulticast(IWMReaderNetworkConfig2 *iface, BOOL *multicast)
{
    WMReader *This = impl_from_IWMReaderNetworkConfig2(iface);
    FIXME("%p, %p\n", This, multicast);
    return E_NOTIMPL;
}

static HRESULT WINAPI networkconfig_SetEnableMulticast(IWMReaderNetworkConfig2 *iface, BOOL multicast)
{
    WMReader *This = impl_from_IWMReaderNetworkConfig2(iface);
    FIXME("%p, %d\n", This, multicast);
    return E_NOTIMPL;
}

static HRESULT WINAPI networkconfig_GetEnableHTTP(IWMReaderNetworkConfig2 *iface, BOOL *enable)
{
    WMReader *This = impl_from_IWMReaderNetworkConfig2(iface);
    FIXME("%p, %p\n", This, enable);
    return E_NOTIMPL;
}

static HRESULT WINAPI networkconfig_SetEnableHTTP(IWMReaderNetworkConfig2 *iface, BOOL enable)
{
    WMReader *This = impl_from_IWMReaderNetworkConfig2(iface);
    FIXME("%p, %d\n", This, enable);
    return E_NOTIMPL;
}

static HRESULT WINAPI networkconfig_GetEnableUDP(IWMReaderNetworkConfig2 *iface, BOOL *enable)
{
    WMReader *This = impl_from_IWMReaderNetworkConfig2(iface);
    FIXME("%p, %p\n", This, enable);
    return E_NOTIMPL;
}

static HRESULT WINAPI networkconfig_SetEnableUDP(IWMReaderNetworkConfig2 *iface, BOOL enable)
{
    WMReader *This = impl_from_IWMReaderNetworkConfig2(iface);
    FIXME("%p, %d\n", This, enable);
    return E_NOTIMPL;
}

static HRESULT WINAPI networkconfig_GetEnableTCP(IWMReaderNetworkConfig2 *iface, BOOL *enable)
{
    WMReader *This = impl_from_IWMReaderNetworkConfig2(iface);
    FIXME("%p, %p\n", This, enable);
    return E_NOTIMPL;
}

static HRESULT WINAPI networkconfig_SetEnableTCP(IWMReaderNetworkConfig2 *iface, BOOL enable)
{
    WMReader *This = impl_from_IWMReaderNetworkConfig2(iface);
    FIXME("%p, %d\n", This, enable);
    return E_NOTIMPL;
}

static HRESULT WINAPI networkconfig_ResetProtocolRollover(IWMReaderNetworkConfig2 *iface)
{
    WMReader *This = impl_from_IWMReaderNetworkConfig2(iface);
    FIXME("%p\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI networkconfig_GetConnectionBandwidth(IWMReaderNetworkConfig2 *iface, DWORD *bandwidth)
{
    WMReader *This = impl_from_IWMReaderNetworkConfig2(iface);
    FIXME("%p, %p\n", This, bandwidth);
    return E_NOTIMPL;
}

static HRESULT WINAPI networkconfig_SetConnectionBandwidth(IWMReaderNetworkConfig2 *iface, DWORD bandwidth)
{
    WMReader *This = impl_from_IWMReaderNetworkConfig2(iface);
    FIXME("%p, %u\n", This, bandwidth);
    return E_NOTIMPL;
}

static HRESULT WINAPI networkconfig_GetNumProtocolsSupported(IWMReaderNetworkConfig2 *iface, DWORD *protocols)
{
    WMReader *This = impl_from_IWMReaderNetworkConfig2(iface);
    FIXME("%p, %p\n", This, protocols);
    return E_NOTIMPL;
}

static HRESULT WINAPI networkconfig_GetSupportedProtocolName(IWMReaderNetworkConfig2 *iface, DWORD protocol_num,
        WCHAR *protocol, DWORD *size)
{
    WMReader *This = impl_from_IWMReaderNetworkConfig2(iface);
    FIXME("%p, %u, %p %p\n", This, protocol_num, protocol, size);
    return E_NOTIMPL;
}

static HRESULT WINAPI networkconfig_AddLoggingUrl(IWMReaderNetworkConfig2 *iface, const WCHAR *url)
{
    WMReader *This = impl_from_IWMReaderNetworkConfig2(iface);
    FIXME("%p, %s\n", This, debugstr_w(url));
    return E_NOTIMPL;
}

static HRESULT WINAPI networkconfig_GetLoggingUrl(IWMReaderNetworkConfig2 *iface, DWORD index, WCHAR *url,
        DWORD *size)
{
    WMReader *This = impl_from_IWMReaderNetworkConfig2(iface);
    FIXME("%p, %u, %p, %p\n", This, index, url, size);
    return E_NOTIMPL;
}

static HRESULT WINAPI networkconfig_GetLoggingUrlCount(IWMReaderNetworkConfig2 *iface, DWORD *count)
{
    WMReader *This = impl_from_IWMReaderNetworkConfig2(iface);
    FIXME("%p, %p\n", This, count);
    return E_NOTIMPL;
}

static HRESULT WINAPI networkconfig_ResetLoggingUrlList(IWMReaderNetworkConfig2 *iface)
{
    WMReader *This = impl_from_IWMReaderNetworkConfig2(iface);
    FIXME("%p\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI networkconfig_GetEnableContentCaching(IWMReaderNetworkConfig2 *iface, BOOL *enable)
{
    WMReader *This = impl_from_IWMReaderNetworkConfig2(iface);
    FIXME("%p, %p\n", This, enable);
    return E_NOTIMPL;
}

static HRESULT WINAPI networkconfig_SetEnableContentCaching(IWMReaderNetworkConfig2 *iface, BOOL enable)
{
    WMReader *This = impl_from_IWMReaderNetworkConfig2(iface);
    FIXME("%p, %d\n", This, enable);
    return E_NOTIMPL;
}

static HRESULT WINAPI networkconfig_GetEnableFastCache(IWMReaderNetworkConfig2 *iface, BOOL *enable)
{
    WMReader *This = impl_from_IWMReaderNetworkConfig2(iface);
    FIXME("%p, %p\n", This, enable);
    return E_NOTIMPL;
}

static HRESULT WINAPI networkconfig_SetEnableFastCache(IWMReaderNetworkConfig2 *iface, BOOL enable)
{
    WMReader *This = impl_from_IWMReaderNetworkConfig2(iface);
    FIXME("%p, %d\n", This, enable);
    return E_NOTIMPL;
}

static HRESULT WINAPI networkconfig_GetAcceleratedStreamingDuration(IWMReaderNetworkConfig2 *iface,
        QWORD *duration)
{
    WMReader *This = impl_from_IWMReaderNetworkConfig2(iface);
    FIXME("%p, %p\n", This, duration);
    return E_NOTIMPL;
}

static HRESULT WINAPI networkconfig_SetAcceleratedStreamingDuration(IWMReaderNetworkConfig2 *iface,
        QWORD duration)
{
    WMReader *This = impl_from_IWMReaderNetworkConfig2(iface);
    FIXME("%p, %s\n", This, wine_dbgstr_longlong(duration));
    return E_NOTIMPL;
}

static HRESULT WINAPI networkconfig_GetAutoReconnectLimit(IWMReaderNetworkConfig2 *iface, DWORD *limit)
{
    WMReader *This = impl_from_IWMReaderNetworkConfig2(iface);
    FIXME("%p, %p\n", This, limit);
    return E_NOTIMPL;
}

static HRESULT WINAPI networkconfig_SetAutoReconnectLimit(IWMReaderNetworkConfig2 *iface, DWORD limit)
{
    WMReader *This = impl_from_IWMReaderNetworkConfig2(iface);
    FIXME("%p, %u\n", This, limit);
    return E_NOTIMPL;
}

static HRESULT WINAPI networkconfig_GetEnableResends(IWMReaderNetworkConfig2 *iface, BOOL *enable)
{
    WMReader *This = impl_from_IWMReaderNetworkConfig2(iface);
    FIXME("%p, %p\n", This, enable);
    return E_NOTIMPL;
}

static HRESULT WINAPI networkconfig_SetEnableResends(IWMReaderNetworkConfig2 *iface, BOOL enable)
{
    WMReader *This = impl_from_IWMReaderNetworkConfig2(iface);
    FIXME("%p, %u\n", This, enable);
    return E_NOTIMPL;
}

static HRESULT WINAPI networkconfig_GetEnableThinning(IWMReaderNetworkConfig2 *iface, BOOL *enable)
{
    WMReader *This = impl_from_IWMReaderNetworkConfig2(iface);
    FIXME("%p, %p\n", This, enable);
    return E_NOTIMPL;
}

static HRESULT WINAPI networkconfig_SetEnableThinning(IWMReaderNetworkConfig2 *iface, BOOL enable)
{
    WMReader *This = impl_from_IWMReaderNetworkConfig2(iface);
    FIXME("%p, %u\n", This, enable);
    return E_NOTIMPL;
}

static HRESULT WINAPI networkconfig_GetMaxNetPacketSize(IWMReaderNetworkConfig2 *iface, DWORD *packet_size)
{
    WMReader *This = impl_from_IWMReaderNetworkConfig2(iface);
    FIXME("%p, %p\n", This, packet_size);
    return E_NOTIMPL;
}

static const IWMReaderNetworkConfig2Vtbl WMReaderNetworkConfig2Vtbl =
{
    networkconfig_QueryInterface,
    networkconfig_AddRef,
    networkconfig_Release,
    networkconfig_GetBufferingTime,
    networkconfig_SetBufferingTime,
    networkconfig_GetUDPPortRanges,
    networkconfig_SetUDPPortRanges,
    networkconfig_GetProxySettings,
    networkconfig_SetProxySettings,
    networkconfig_GetProxyHostName,
    networkconfig_SetProxyHostName,
    networkconfig_GetProxyPort,
    networkconfig_SetProxyPort,
    networkconfig_GetProxyExceptionList,
    networkconfig_SetProxyExceptionList,
    networkconfig_GetProxyBypassForLocal,
    networkconfig_SetProxyBypassForLocal,
    networkconfig_GetForceRerunAutoProxyDetection,
    networkconfig_SetForceRerunAutoProxyDetection,
    networkconfig_GetEnableMulticast,
    networkconfig_SetEnableMulticast,
    networkconfig_GetEnableHTTP,
    networkconfig_SetEnableHTTP,
    networkconfig_GetEnableUDP,
    networkconfig_SetEnableUDP,
    networkconfig_GetEnableTCP,
    networkconfig_SetEnableTCP,
    networkconfig_ResetProtocolRollover,
    networkconfig_GetConnectionBandwidth,
    networkconfig_SetConnectionBandwidth,
    networkconfig_GetNumProtocolsSupported,
    networkconfig_GetSupportedProtocolName,
    networkconfig_AddLoggingUrl,
    networkconfig_GetLoggingUrl,
    networkconfig_GetLoggingUrlCount,
    networkconfig_ResetLoggingUrlList,
    networkconfig_GetEnableContentCaching,
    networkconfig_SetEnableContentCaching,
    networkconfig_GetEnableFastCache,
    networkconfig_SetEnableFastCache,
    networkconfig_GetAcceleratedStreamingDuration,
    networkconfig_SetAcceleratedStreamingDuration,
    networkconfig_GetAutoReconnectLimit,
    networkconfig_SetAutoReconnectLimit,
    networkconfig_GetEnableResends,
    networkconfig_SetEnableResends,
    networkconfig_GetEnableThinning,
    networkconfig_SetEnableThinning,
    networkconfig_GetMaxNetPacketSize
};

static inline WMReader *impl_from_IWMReaderStreamClock(IWMReaderStreamClock *iface)
{
    return CONTAINING_RECORD(iface, WMReader, IWMReaderStreamClock_iface);
}

static HRESULT WINAPI readclock_QueryInterface(IWMReaderStreamClock *iface, REFIID riid, void **ppv)
{
    WMReader *This = impl_from_IWMReaderStreamClock(iface);
    return IWMReader_QueryInterface(&This->IWMReader_iface, riid, ppv);
}

static ULONG WINAPI readclock_AddRef(IWMReaderStreamClock *iface)
{
    WMReader *This = impl_from_IWMReaderStreamClock(iface);
    return IWMReader_AddRef(&This->IWMReader_iface);
}

static ULONG WINAPI readclock_Release(IWMReaderStreamClock *iface)
{
    WMReader *This = impl_from_IWMReaderStreamClock(iface);
    return IWMReader_Release(&This->IWMReader_iface);
}

static HRESULT WINAPI readclock_GetTime(IWMReaderStreamClock *iface, QWORD *now)
{
    WMReader *This = impl_from_IWMReaderStreamClock(iface);
    FIXME("%p, %p\n", This, now);
    return E_NOTIMPL;
}

static HRESULT WINAPI readclock_SetTimer(IWMReaderStreamClock *iface, QWORD when, void *param, DWORD *id)
{
    WMReader *This = impl_from_IWMReaderStreamClock(iface);
    FIXME("%p, %s, %p, %p\n", This, wine_dbgstr_longlong(when), param, id);
    return E_NOTIMPL;
}

static HRESULT WINAPI readclock_KillTimer(IWMReaderStreamClock *iface, DWORD id)
{
    WMReader *This = impl_from_IWMReaderStreamClock(iface);
    FIXME("%p, %d\n", This, id);
    return E_NOTIMPL;
}

static const IWMReaderStreamClockVtbl WMReaderStreamClockVtbl =
{
    readclock_QueryInterface,
    readclock_AddRef,
    readclock_Release,
    readclock_GetTime,
    readclock_SetTimer,
    readclock_KillTimer
};

static inline WMReader *impl_from_IWMReaderTypeNegotiation(IWMReaderTypeNegotiation *iface)
{
    return CONTAINING_RECORD(iface, WMReader, IWMReaderTypeNegotiation_iface);
}

static HRESULT WINAPI negotiation_QueryInterface(IWMReaderTypeNegotiation *iface, REFIID riid, void **ppv)
{
    WMReader *This = impl_from_IWMReaderTypeNegotiation(iface);
    return IWMReader_QueryInterface(&This->IWMReader_iface, riid, ppv);
}

static ULONG WINAPI negotiation_AddRef(IWMReaderTypeNegotiation *iface)
{
    WMReader *This = impl_from_IWMReaderTypeNegotiation(iface);
    return IWMReader_AddRef(&This->IWMReader_iface);
}

static ULONG WINAPI negotiation_Release(IWMReaderTypeNegotiation *iface)
{
    WMReader *This = impl_from_IWMReaderTypeNegotiation(iface);
    return IWMReader_Release(&This->IWMReader_iface);
}

static HRESULT WINAPI negotiation_TryOutputProps(IWMReaderTypeNegotiation *iface, DWORD output, IWMOutputMediaProps *props)
{
    WMReader *This = impl_from_IWMReaderTypeNegotiation(iface);
    FIXME("%p, %d, %p\n", This, output, props);
    return E_NOTIMPL;
}

static const IWMReaderTypeNegotiationVtbl WMReaderTypeNegotiationVtbl =
{
    negotiation_QueryInterface,
    negotiation_AddRef,
    negotiation_Release,
    negotiation_TryOutputProps
};

static inline WMReader *impl_from_IWMReaderTimecode(IWMReaderTimecode *iface)
{
    return CONTAINING_RECORD(iface, WMReader, IWMReaderTimecode_iface);
}

static HRESULT WINAPI timecode_QueryInterface(IWMReaderTimecode *iface, REFIID riid, void **ppv)
{
    WMReader *This = impl_from_IWMReaderTimecode(iface);
    return IWMReader_QueryInterface(&This->IWMReader_iface, riid, ppv);
}

static ULONG WINAPI timecode_AddRef(IWMReaderTimecode *iface)
{
    WMReader *This = impl_from_IWMReaderTimecode(iface);
    return IWMReader_AddRef(&This->IWMReader_iface);
}

static ULONG WINAPI timecode_Release(IWMReaderTimecode *iface)
{
    WMReader *This = impl_from_IWMReaderTimecode(iface);
    return IWMReader_Release(&This->IWMReader_iface);
}

static HRESULT WINAPI timecode_GetTimecodeRangeCount(IWMReaderTimecode *iface, WORD num, WORD *count)
{
    WMReader *This = impl_from_IWMReaderTimecode(iface);
    FIXME("%p, %d, %p\n", This, num, count);
    return E_NOTIMPL;
}

static HRESULT WINAPI timecode_GetTimecodeRangeBounds(IWMReaderTimecode *iface, WORD stream, WORD range,
        DWORD *start_timecode, DWORD *end_timecode)
{
    WMReader *This = impl_from_IWMReaderTimecode(iface);
    FIXME("%p, %d, %d, %p, %p\n", This, stream, range, start_timecode, end_timecode);
    return E_NOTIMPL;
}

static const IWMReaderTimecodeVtbl WMReaderTimecodeVtbl =
{
    timecode_QueryInterface,
    timecode_AddRef,
    timecode_Release,
    timecode_GetTimecodeRangeCount,
    timecode_GetTimecodeRangeBounds
};


static inline WMReader *impl_from_IWMReaderPlaylistBurn(IWMReaderPlaylistBurn *iface)
{
    return CONTAINING_RECORD(iface, WMReader, IWMReaderPlaylistBurn_iface);
}

static HRESULT WINAPI playlist_QueryInterface(IWMReaderPlaylistBurn *iface, REFIID riid, void **ppv)
{
    WMReader *This = impl_from_IWMReaderPlaylistBurn(iface);
    return IWMReader_QueryInterface(&This->IWMReader_iface, riid, ppv);
}

static ULONG WINAPI playlist_AddRef(IWMReaderPlaylistBurn *iface)
{
    WMReader *This = impl_from_IWMReaderPlaylistBurn(iface);
    return IWMReader_AddRef(&This->IWMReader_iface);
}

static ULONG WINAPI playlist_Release(IWMReaderPlaylistBurn *iface)
{
    WMReader *This = impl_from_IWMReaderPlaylistBurn(iface);
    return IWMReader_Release(&This->IWMReader_iface);
}

static HRESULT WINAPI playlist_InitPlaylistBurn(IWMReaderPlaylistBurn *iface, DWORD count,
        LPCWSTR_WMSDK_TYPE_SAFE *filenames, IWMStatusCallback *callback, void *context)
{
    WMReader *This = impl_from_IWMReaderPlaylistBurn(iface);
    FIXME("%p, %d, %p, %p, %p\n", This, count, filenames, callback, context);
    return E_NOTIMPL;
}

static HRESULT WINAPI playlist_GetInitResults(IWMReaderPlaylistBurn *iface, DWORD count, HRESULT *stat)
{
    WMReader *This = impl_from_IWMReaderPlaylistBurn(iface);
    FIXME("%p, %d, %p\n", This, count, stat);
    return E_NOTIMPL;
}

static HRESULT WINAPI playlist_Cancel(IWMReaderPlaylistBurn *iface)
{
    WMReader *This = impl_from_IWMReaderPlaylistBurn(iface);
    FIXME("%p\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI playlist_EndPlaylistBurn(IWMReaderPlaylistBurn *iface, HRESULT result)
{
    WMReader *This = impl_from_IWMReaderPlaylistBurn(iface);
    FIXME("%p, 0x%08x\n", This, result);
    return E_NOTIMPL;
}

static const IWMReaderPlaylistBurnVtbl WMReaderPlaylistBurnVtbl =
{
    playlist_QueryInterface,
    playlist_AddRef,
    playlist_Release,
    playlist_InitPlaylistBurn,
    playlist_GetInitResults,
    playlist_Cancel,
    playlist_EndPlaylistBurn
};

static inline WMReader *impl_from_IWMHeaderInfo3(IWMHeaderInfo3 *iface)
{
    return CONTAINING_RECORD(iface, WMReader, IWMHeaderInfo3_iface);
}

static HRESULT WINAPI headerinfo_QueryInterface(IWMHeaderInfo3 *iface, REFIID riid, void **ppv)
{
    WMReader *This = impl_from_IWMHeaderInfo3(iface);
    return IWMReader_QueryInterface(&This->IWMReader_iface, riid, ppv);
}

static ULONG WINAPI headerinfo_AddRef(IWMHeaderInfo3 *iface)
{
    WMReader *This = impl_from_IWMHeaderInfo3(iface);
    return IWMReader_AddRef(&This->IWMReader_iface);
}

static ULONG WINAPI headerinfo_Release(IWMHeaderInfo3 *iface)
{
    WMReader *This = impl_from_IWMHeaderInfo3(iface);
    return IWMReader_Release(&This->IWMReader_iface);
}

static HRESULT WINAPI headerinfo_GetAttributeCount(IWMHeaderInfo3 *iface, WORD stream_num, WORD *attributes)
{
    WMReader *This = impl_from_IWMHeaderInfo3(iface);
    FIXME("%p, %d, %p\n", This, stream_num, attributes);
    return E_NOTIMPL;
}

static HRESULT WINAPI headerinfo_GetAttributeByIndex(IWMHeaderInfo3 *iface, WORD index, WORD *stream_num,
        WCHAR *name, WORD *name_len, WMT_ATTR_DATATYPE *type, BYTE *value, WORD *length)
{
    WMReader *This = impl_from_IWMHeaderInfo3(iface);
    FIXME("%p, %d, %p, %p, %p, %p, %p, %p\n", This, index, stream_num, name, name_len, type,
            value, length);
    return E_NOTIMPL;
}

static HRESULT WINAPI headerinfo_GetAttributeByName(IWMHeaderInfo3 *iface, WORD *stream_num, LPCWSTR name,
        WMT_ATTR_DATATYPE *type, BYTE *value, WORD *length)
{
    WMReader *This = impl_from_IWMHeaderInfo3(iface);
    FIXME("%p, %p, %s, %p, %p, %p\n", This, stream_num, debugstr_w(name), type, value, length);
    return E_NOTIMPL;
}

static HRESULT WINAPI headerinfo_SetAttribute(IWMHeaderInfo3 *iface, WORD stream_num, LPCWSTR name,
        WMT_ATTR_DATATYPE type, const BYTE *value, WORD length)
{
    WMReader *This = impl_from_IWMHeaderInfo3(iface);
    FIXME("%p, %d, %s, %d, %p, %d\n", This, stream_num, debugstr_w(name), type, value, length);
    return E_NOTIMPL;
}

static HRESULT WINAPI headerinfo_GetMarkerCount(IWMHeaderInfo3 *iface, WORD *markers)
{
    WMReader *This = impl_from_IWMHeaderInfo3(iface);
    FIXME("%p, %p\n", This, markers);
    return E_NOTIMPL;
}

static HRESULT WINAPI headerinfo_GetMarker(IWMHeaderInfo3 *iface, WORD index, WCHAR *marker_name,
        WORD *marker_len, QWORD *marker_time)
{
    WMReader *This = impl_from_IWMHeaderInfo3(iface);
    FIXME("%p, %d, %p, %p, %p\n", This, index, marker_name, marker_len, marker_time);
    return E_NOTIMPL;
}

static HRESULT WINAPI headerinfo_AddMarker(IWMHeaderInfo3 *iface, LPCWSTR_WMSDK_TYPE_SAFE marker_name,
        QWORD marker_time)
{
    WMReader *This = impl_from_IWMHeaderInfo3(iface);
    FIXME("%p, %p, %s\n", This, marker_name, wine_dbgstr_longlong(marker_time));
    return E_NOTIMPL;
}

static HRESULT WINAPI headerinfo_RemoveMarker(IWMHeaderInfo3 *iface, WORD index)
{
    WMReader *This = impl_from_IWMHeaderInfo3(iface);
    FIXME("%p, %d\n", This, index);
    return E_NOTIMPL;
}

static HRESULT WINAPI headerinfo_GetScriptCount(IWMHeaderInfo3 *iface, WORD *scripts)
{
    WMReader *This = impl_from_IWMHeaderInfo3(iface);
    FIXME("%p, %p\n", This, scripts);
    return E_NOTIMPL;
}

static HRESULT WINAPI headerinfo_GetScript(IWMHeaderInfo3 *iface, WORD index, WCHAR *type,
        WORD *type_len, WCHAR *command, WORD *command_len, QWORD *script_time)
{
    WMReader *This = impl_from_IWMHeaderInfo3(iface);
    FIXME("%p, %d, %p, %p, %p, %p, %p\n", This, index, type, type_len, command, command_len, script_time);
    return E_NOTIMPL;
}

static HRESULT WINAPI headerinfo_AddScript(IWMHeaderInfo3 *iface, LPCWSTR_WMSDK_TYPE_SAFE type,
        LPCWSTR_WMSDK_TYPE_SAFE command, QWORD script_time)
{
    WMReader *This = impl_from_IWMHeaderInfo3(iface);
    FIXME("%p, %s, %s, %s\n", This, debugstr_w(type), debugstr_w(command), wine_dbgstr_longlong(script_time));
    return E_NOTIMPL;
}

static HRESULT WINAPI headerinfo_RemoveScript(IWMHeaderInfo3 *iface, WORD index)
{
    WMReader *This = impl_from_IWMHeaderInfo3(iface);
    FIXME("%p, %d\n", This, index);
    return E_NOTIMPL;
}

static HRESULT WINAPI headerinfo_GetCodecInfoCount(IWMHeaderInfo3 *iface, DWORD *codec_infos)
{
    WMReader *This = impl_from_IWMHeaderInfo3(iface);
    FIXME("%p, %p\n", This, codec_infos);
    return E_NOTIMPL;
}

static HRESULT WINAPI headerinfo_GetCodecInfo(IWMHeaderInfo3 *iface, DWORD index, WORD *name_len,
        WCHAR *name, WORD *description_len, WCHAR *description, WMT_CODEC_INFO_TYPE *codec_type,
        WORD *codec_info_cnt, BYTE *codec_info)
{
    WMReader *This = impl_from_IWMHeaderInfo3(iface);
    FIXME("%p, %d, %p, %p, %p, %p, %p, %p, %p\n", This, index, name_len, name, description_len,
            description, codec_type, codec_info_cnt, codec_info);
    return E_NOTIMPL;
}

static HRESULT WINAPI headerinfo_GetAttributeCountEx(IWMHeaderInfo3 *iface, WORD stream_num, WORD *attributes)
{
    WMReader *This = impl_from_IWMHeaderInfo3(iface);
    FIXME("%p, %d, %p\n", This, stream_num, attributes);
    return E_NOTIMPL;
}

static HRESULT WINAPI headerinfo_GetAttributeIndices(IWMHeaderInfo3 *iface, WORD stream_num, LPCWSTR name,
        WORD *lang_index, WORD *indices, WORD *count)
{
    WMReader *This = impl_from_IWMHeaderInfo3(iface);
    FIXME("%p, %d, %s, %p, %p, %p\n", This, stream_num, debugstr_w(name), lang_index, indices, count);
    return E_NOTIMPL;
}

static HRESULT WINAPI headerinfo_GetAttributeByIndexEx(IWMHeaderInfo3 *iface, WORD stream_num,
        WORD index, LPWSTR name, WORD *name_len, WMT_ATTR_DATATYPE *type, WORD *lang_index,
        BYTE *value, DWORD *data_len)
{
    WMReader *This = impl_from_IWMHeaderInfo3(iface);
    FIXME("%p, %d, %d, %s, %p, %p, %p, %p, %p\n", This, stream_num, index, debugstr_w(name), name_len,
            type, lang_index, value, data_len);
    return E_NOTIMPL;
}

static HRESULT WINAPI headerinfo_ModifyAttribute(IWMHeaderInfo3 *iface, WORD stream_num,
        WORD index, WMT_ATTR_DATATYPE type, WORD lang_index, const BYTE *value, DWORD length)
{
    WMReader *This = impl_from_IWMHeaderInfo3(iface);
    FIXME("%p, %d, %d, %d, %d, %p, %d\n", This, stream_num, index, type, lang_index, value, length);
    return E_NOTIMPL;
}

static HRESULT WINAPI headerinfo_AddAttribute(IWMHeaderInfo3 *iface, WORD stream_num, LPCWSTR name,
        WORD *index, WMT_ATTR_DATATYPE type, WORD lang_index, const BYTE *value, DWORD length)
{
    WMReader *This = impl_from_IWMHeaderInfo3(iface);
    FIXME("%p, %d, %s, %p, %d, %d, %p, %d\n", This, stream_num, debugstr_w(name), index,
            type, lang_index, value, length);
    return E_NOTIMPL;
}

static HRESULT WINAPI headerinfo_DeleteAttribute(IWMHeaderInfo3 *iface, WORD stream_num, WORD index)
{
    WMReader *This = impl_from_IWMHeaderInfo3(iface);
    FIXME("%p, %d, %d\n", This, stream_num, index);
    return E_NOTIMPL;
}

static HRESULT WINAPI headerinfo_AddCodecInfo(IWMHeaderInfo3 *iface, LPCWSTR_WMSDK_TYPE_SAFE name,
        LPCWSTR_WMSDK_TYPE_SAFE description, WMT_CODEC_INFO_TYPE codec_type, WORD codec_info_cnt,
        BYTE *codec_info)
{
    WMReader *This = impl_from_IWMHeaderInfo3(iface);
    FIXME("%p, %p, %p, %d, %d, %p\n", This, name, description, codec_type, codec_info_cnt,
            codec_info);
    return E_NOTIMPL;
}

static const IWMHeaderInfo3Vtbl WMHeaderInfo3Vtbl =
{
    headerinfo_QueryInterface,
    headerinfo_AddRef,
    headerinfo_Release,
    headerinfo_GetAttributeCount,
    headerinfo_GetAttributeByIndex,
    headerinfo_GetAttributeByName,
    headerinfo_SetAttribute,
    headerinfo_GetMarkerCount,
    headerinfo_GetMarker,
    headerinfo_AddMarker,
    headerinfo_RemoveMarker,
    headerinfo_GetScriptCount,
    headerinfo_GetScript,
    headerinfo_AddScript,
    headerinfo_RemoveScript,
    headerinfo_GetCodecInfoCount,
    headerinfo_GetCodecInfo,
    headerinfo_GetAttributeCountEx,
    headerinfo_GetAttributeIndices,
    headerinfo_GetAttributeByIndexEx,
    headerinfo_ModifyAttribute,
    headerinfo_AddAttribute,
    headerinfo_DeleteAttribute,
    headerinfo_AddCodecInfo
};


static inline WMReader *impl_from_IWMLanguageList(IWMLanguageList *iface)
{
    return CONTAINING_RECORD(iface, WMReader, IWMLanguageList_iface);
}

static HRESULT WINAPI langlist_QueryInterface(IWMLanguageList *iface, REFIID riid, void **ppv)
{
    WMReader *This = impl_from_IWMLanguageList(iface);
    return IWMReader_QueryInterface(&This->IWMReader_iface, riid, ppv);
}

static ULONG WINAPI langlist_AddRef(IWMLanguageList *iface)
{
    WMReader *This = impl_from_IWMLanguageList(iface);
    return IWMReader_AddRef(&This->IWMReader_iface);
}

static ULONG WINAPI langlist_Release(IWMLanguageList *iface)
{
    WMReader *This = impl_from_IWMLanguageList(iface);
    return IWMReader_Release(&This->IWMReader_iface);
}

static HRESULT WINAPI langlist_GetLanguageCount(IWMLanguageList *iface, WORD *count)
{
    WMReader *This = impl_from_IWMLanguageList(iface);
    FIXME("%p, %p\n", This, count);
    return E_NOTIMPL;
}

static HRESULT WINAPI langlist_GetLanguageDetails(IWMLanguageList *iface, WORD index,
        WCHAR *language, WORD *length)
{
    WMReader *This = impl_from_IWMLanguageList(iface);
    FIXME("%p, %d, %p, %p\n", This, index, language, length);
    return E_NOTIMPL;
}

static HRESULT WINAPI langlist_AddLanguageByRFC1766String(IWMLanguageList *iface, LPCWSTR_WMSDK_TYPE_SAFE language,
        WORD *index)
{
    WMReader *This = impl_from_IWMLanguageList(iface);
    FIXME("%p, %p, %p\n", This, language, index);
    return E_NOTIMPL;
}

static const IWMLanguageListVtbl WMLanguageListVtbl =
{
    langlist_QueryInterface,
    langlist_AddRef,
    langlist_Release,
    langlist_GetLanguageCount,
    langlist_GetLanguageDetails,
    langlist_AddLanguageByRFC1766String
};

static inline WMReader *impl_from_IReferenceClock(IReferenceClock *iface)
{
    return CONTAINING_RECORD(iface, WMReader, IReferenceClock_iface);
}

static HRESULT WINAPI refclock_QueryInterface(IReferenceClock *iface, REFIID riid, void **ppv)
{
    WMReader *This = impl_from_IReferenceClock(iface);
    return IWMReader_QueryInterface(&This->IWMReader_iface, riid, ppv);
}

static ULONG WINAPI refclock_AddRef(IReferenceClock *iface)
{
    WMReader *This = impl_from_IReferenceClock(iface);
    return IWMReader_AddRef(&This->IWMReader_iface);
}

static ULONG WINAPI refclock_Release(IReferenceClock *iface)
{
    WMReader *This = impl_from_IReferenceClock(iface);
    return IWMReader_Release(&This->IWMReader_iface);
}

static HRESULT WINAPI refclock_GetTime(IReferenceClock *iface, REFERENCE_TIME *time)
{
    WMReader *This = impl_from_IReferenceClock(iface);
    FIXME("%p, %p\n", This, time);
    return E_NOTIMPL;
}

static HRESULT WINAPI refclock_AdviseTime(IReferenceClock *iface, REFERENCE_TIME basetime,
        REFERENCE_TIME streamtime, HEVENT event, DWORD_PTR *cookie)
{
    WMReader *This = impl_from_IReferenceClock(iface);
    FIXME("%p, %s, %s, %lu, %p\n", This, wine_dbgstr_longlong(basetime),
            wine_dbgstr_longlong(streamtime), event, cookie);
    return E_NOTIMPL;
}

static HRESULT WINAPI refclock_AdvisePeriodic(IReferenceClock *iface, REFERENCE_TIME starttime,
        REFERENCE_TIME period, HSEMAPHORE semaphore, DWORD_PTR *cookie)
{
    WMReader *This = impl_from_IReferenceClock(iface);
    FIXME("%p, %s, %s, %lu, %p\n", This, wine_dbgstr_longlong(starttime),
            wine_dbgstr_longlong(period), semaphore, cookie);
    return E_NOTIMPL;
}

static HRESULT WINAPI refclock_Unadvise(IReferenceClock *iface, DWORD_PTR cookie)
{
    WMReader *This = impl_from_IReferenceClock(iface);
    FIXME("%p, %lu\n", This, cookie);
    return E_NOTIMPL;
}

static const IReferenceClockVtbl ReferenceClockVtbl =
{
    refclock_QueryInterface,
    refclock_AddRef,
    refclock_Release,
    refclock_GetTime,
    refclock_AdviseTime,
    refclock_AdvisePeriodic,
    refclock_Unadvise
};

static inline WMReader *impl_from_IWMProfile3(IWMProfile3 *iface)
{
    return CONTAINING_RECORD(iface, WMReader, IWMProfile3_iface);
}

static HRESULT WINAPI profile3_QueryInterface(IWMProfile3 *iface, REFIID riid, void **ppv)
{
    WMReader *This = impl_from_IWMProfile3(iface);
    return IWMReader_QueryInterface(&This->IWMReader_iface, riid, ppv);
}

static ULONG WINAPI profile3_AddRef(IWMProfile3 *iface)
{
    WMReader *This = impl_from_IWMProfile3(iface);
    return IWMReader_AddRef(&This->IWMReader_iface);
}

static ULONG WINAPI profile3_Release(IWMProfile3 *iface)
{
    WMReader *This = impl_from_IWMProfile3(iface);
    return IWMReader_Release(&This->IWMReader_iface);
}

static HRESULT WINAPI profile3_GetVersion(IWMProfile3 *iface, WMT_VERSION *version)
{
    WMReader *This = impl_from_IWMProfile3(iface);
    FIXME("%p, %p\n", This, version);
    return E_NOTIMPL;
}

static HRESULT WINAPI profile3_GetName(IWMProfile3 *iface, WCHAR *name, DWORD *length)
{
    WMReader *This = impl_from_IWMProfile3(iface);
    FIXME("%p, %p, %p\n", This, name, length);
    return E_NOTIMPL;
}

static HRESULT WINAPI profile3_SetName(IWMProfile3 *iface, const WCHAR *name)
{
    WMReader *This = impl_from_IWMProfile3(iface);
    FIXME("%p, %s\n", This, debugstr_w(name));
    return E_NOTIMPL;
}

static HRESULT WINAPI profile3_GetDescription(IWMProfile3 *iface, WCHAR *description, DWORD *length)
{
    WMReader *This = impl_from_IWMProfile3(iface);
    FIXME("%p, %p, %p\n", This, description, length);
    return E_NOTIMPL;
}

static HRESULT WINAPI profile3_SetDescription(IWMProfile3 *iface, const WCHAR *description)
{
    WMReader *This = impl_from_IWMProfile3(iface);
    FIXME("%p, %s\n", This, debugstr_w(description));
    return E_NOTIMPL;
}

static HRESULT WINAPI profile3_GetStreamCount(IWMProfile3 *iface, DWORD *count)
{
    WMReader *This = impl_from_IWMProfile3(iface);
    FIXME("%p, %p\n", This, count);
    return E_NOTIMPL;
}

static HRESULT WINAPI profile3_GetStream(IWMProfile3 *iface, DWORD index, IWMStreamConfig **config)
{
    WMReader *This = impl_from_IWMProfile3(iface);
    FIXME("%p, %d, %p\n", This, index, config);
    return E_NOTIMPL;
}

static HRESULT WINAPI profile3_GetStreamByNumber(IWMProfile3 *iface, WORD stream, IWMStreamConfig **config)
{
    WMReader *This = impl_from_IWMProfile3(iface);
    FIXME("%p, %d, %p\n", This, stream, config);
    return E_NOTIMPL;
}

static HRESULT WINAPI profile3_RemoveStream(IWMProfile3 *iface, IWMStreamConfig *config)
{
    WMReader *This = impl_from_IWMProfile3(iface);
    FIXME("%p, %p\n", This, config);
    return E_NOTIMPL;
}

static HRESULT WINAPI profile3_RemoveStreamByNumber(IWMProfile3 *iface, WORD stream)
{
    WMReader *This = impl_from_IWMProfile3(iface);
    FIXME("%p, %d\n", This, stream);
    return E_NOTIMPL;
}

static HRESULT WINAPI profile3_AddStream(IWMProfile3 *iface, IWMStreamConfig *config)
{
    WMReader *This = impl_from_IWMProfile3(iface);
    FIXME("%p, %p\n", This, config);
    return E_NOTIMPL;
}

static HRESULT WINAPI profile3_ReconfigStream(IWMProfile3 *iface, IWMStreamConfig *config)
{
    WMReader *This = impl_from_IWMProfile3(iface);
    FIXME("%p, %p\n", This, config);
    return E_NOTIMPL;
}

static HRESULT WINAPI profile3_CreateNewStream(IWMProfile3 *iface, REFGUID type, IWMStreamConfig **config)
{
    WMReader *This = impl_from_IWMProfile3(iface);
    FIXME("%p, %s, %p\n", This, debugstr_guid(type), config);
    return E_NOTIMPL;
}

static HRESULT WINAPI profile3_GetMutualExclusionCount(IWMProfile3 *iface, DWORD *count)
{
    WMReader *This = impl_from_IWMProfile3(iface);
    FIXME("%p, %p\n", This, count);
    return E_NOTIMPL;
}

static HRESULT WINAPI profile3_GetMutualExclusion(IWMProfile3 *iface, DWORD index, IWMMutualExclusion **mutual)
{
    WMReader *This = impl_from_IWMProfile3(iface);
    FIXME("%p, %d, %p\n", This, index, mutual);
    return E_NOTIMPL;
}

static HRESULT WINAPI profile3_RemoveMutualExclusion(IWMProfile3 *iface, IWMMutualExclusion *mutual)
{
    WMReader *This = impl_from_IWMProfile3(iface);
    FIXME("%p, %p\n", This, mutual);
    return E_NOTIMPL;
}

static HRESULT WINAPI profile3_AddMutualExclusion(IWMProfile3 *iface, IWMMutualExclusion *mutual)
{
    WMReader *This = impl_from_IWMProfile3(iface);
    FIXME("%p, %p\n", This, mutual);
    return E_NOTIMPL;
}

static HRESULT WINAPI profile3_CreateNewMutualExclusion(IWMProfile3 *iface, IWMMutualExclusion **mutual)
{
    WMReader *This = impl_from_IWMProfile3(iface);
    FIXME("%p, %p\n", This, mutual);
    return E_NOTIMPL;
}

static HRESULT WINAPI profile3_GetProfileID(IWMProfile3 *iface, GUID *guid)
{
    WMReader *This = impl_from_IWMProfile3(iface);
    FIXME("%p, %p\n", This, guid);
    return E_NOTIMPL;
}

static HRESULT WINAPI profile3_GetStorageFormat(IWMProfile3 *iface, WMT_STORAGE_FORMAT *storage)
{
    WMReader *This = impl_from_IWMProfile3(iface);
    FIXME("%p, %p\n", This, storage);
    return E_NOTIMPL;
}

static HRESULT WINAPI profile3_SetStorageFormat(IWMProfile3 *iface, WMT_STORAGE_FORMAT storage)
{
    WMReader *This = impl_from_IWMProfile3(iface);
    FIXME("%p, %d\n", This, storage);
    return E_NOTIMPL;
}

static HRESULT WINAPI profile3_GetBandwidthSharingCount(IWMProfile3 *iface, DWORD *count)
{
    WMReader *This = impl_from_IWMProfile3(iface);
    FIXME("%p, %p\n", This, count);
    return E_NOTIMPL;
}

static HRESULT WINAPI profile3_GetBandwidthSharing(IWMProfile3 *iface, DWORD index, IWMBandwidthSharing **bandwidth)
{
    WMReader *This = impl_from_IWMProfile3(iface);
    FIXME("%p, %d, %p\n", This, index, bandwidth);
    return E_NOTIMPL;
}

static HRESULT WINAPI profile3_RemoveBandwidthSharing( IWMProfile3 *iface, IWMBandwidthSharing *bandwidth)
{
    WMReader *This = impl_from_IWMProfile3(iface);
    FIXME("%p, %p\n", This, bandwidth);
    return E_NOTIMPL;
}

static HRESULT WINAPI profile3_AddBandwidthSharing(IWMProfile3 *iface, IWMBandwidthSharing *bandwidth)
{
    WMReader *This = impl_from_IWMProfile3(iface);
    FIXME("%p, %p\n", This, bandwidth);
    return E_NOTIMPL;
}

static HRESULT WINAPI profile3_CreateNewBandwidthSharing( IWMProfile3 *iface, IWMBandwidthSharing **bandwidth)
{
    WMReader *This = impl_from_IWMProfile3(iface);
    FIXME("%p, %p\n", This, bandwidth);
    return E_NOTIMPL;
}

static HRESULT WINAPI profile3_GetStreamPrioritization(IWMProfile3 *iface, IWMStreamPrioritization **stream)
{
    WMReader *This = impl_from_IWMProfile3(iface);
    FIXME("%p, %p\n", This, stream);
    return E_NOTIMPL;
}

static HRESULT WINAPI profile3_SetStreamPrioritization(IWMProfile3 *iface, IWMStreamPrioritization *stream)
{
    WMReader *This = impl_from_IWMProfile3(iface);
    FIXME("%p, %p\n", This, stream);
    return E_NOTIMPL;
}

static HRESULT WINAPI profile3_RemoveStreamPrioritization(IWMProfile3 *iface)
{
    WMReader *This = impl_from_IWMProfile3(iface);
    FIXME("%p\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI profile3_CreateNewStreamPrioritization(IWMProfile3 *iface, IWMStreamPrioritization **stream)
{
    WMReader *This = impl_from_IWMProfile3(iface);
    FIXME("%p, %p\n", This, stream);
    return E_NOTIMPL;
}

static HRESULT WINAPI profile3_GetExpectedPacketCount(IWMProfile3 *iface, QWORD duration, QWORD *packets)
{
    WMReader *This = impl_from_IWMProfile3(iface);
    FIXME("%p, %s, %p\n", This, wine_dbgstr_longlong(duration), packets);
    return E_NOTIMPL;
}

static const IWMProfile3Vtbl WMProfile3Vtbl =
{
    profile3_QueryInterface,
    profile3_AddRef,
    profile3_Release,
    profile3_GetVersion,
    profile3_GetName,
    profile3_SetName,
    profile3_GetDescription,
    profile3_SetDescription,
    profile3_GetStreamCount,
    profile3_GetStream,
    profile3_GetStreamByNumber,
    profile3_RemoveStream,
    profile3_RemoveStreamByNumber,
    profile3_AddStream,
    profile3_ReconfigStream,
    profile3_CreateNewStream,
    profile3_GetMutualExclusionCount,
    profile3_GetMutualExclusion,
    profile3_RemoveMutualExclusion,
    profile3_AddMutualExclusion,
    profile3_CreateNewMutualExclusion,
    profile3_GetProfileID,
    profile3_GetStorageFormat,
    profile3_SetStorageFormat,
    profile3_GetBandwidthSharingCount,
    profile3_GetBandwidthSharing,
    profile3_RemoveBandwidthSharing,
    profile3_AddBandwidthSharing,
    profile3_CreateNewBandwidthSharing,
    profile3_GetStreamPrioritization,
    profile3_SetStreamPrioritization,
    profile3_RemoveStreamPrioritization,
    profile3_CreateNewStreamPrioritization,
    profile3_GetExpectedPacketCount
};

static inline WMReader *impl_from_IWMPacketSize2(IWMPacketSize2 *iface)
{
    return CONTAINING_RECORD(iface, WMReader, IWMPacketSize2_iface);
}

static HRESULT WINAPI packetsize_QueryInterface(IWMPacketSize2 *iface, REFIID riid, void **ppv)
{
    WMReader *This = impl_from_IWMPacketSize2(iface);
    return IWMReader_QueryInterface(&This->IWMReader_iface, riid, ppv);
}

static ULONG WINAPI packetsize_AddRef(IWMPacketSize2 *iface)
{
    WMReader *This = impl_from_IWMPacketSize2(iface);
    return IWMReader_AddRef(&This->IWMReader_iface);
}

static ULONG WINAPI packetsize_Release(IWMPacketSize2 *iface)
{
    WMReader *This = impl_from_IWMPacketSize2(iface);
    return IWMReader_Release(&This->IWMReader_iface);
}

static HRESULT WINAPI packetsize_GetMaxPacketSize(IWMPacketSize2 *iface, DWORD *size)
{
    WMReader *This = impl_from_IWMPacketSize2(iface);
    FIXME("%p, %p\n", This, size);
    return E_NOTIMPL;
}

static HRESULT WINAPI packetsize_SetMaxPacketSize(IWMPacketSize2 *iface, DWORD size)
{
    WMReader *This = impl_from_IWMPacketSize2(iface);
    FIXME("%p, %d\n", This, size);
    return E_NOTIMPL;
}

static HRESULT WINAPI packetsize_GetMinPacketSize(IWMPacketSize2 *iface, DWORD *size)
{
    WMReader *This = impl_from_IWMPacketSize2(iface);
    FIXME("%p, %p\n", This, size);
    return E_NOTIMPL;
}

static HRESULT WINAPI packetsize_SetMinPacketSize(IWMPacketSize2 *iface, DWORD size)
{
    WMReader *This = impl_from_IWMPacketSize2(iface);
    FIXME("%p, %d\n", This, size);
    return E_NOTIMPL;
}

static const IWMPacketSize2Vtbl WMPacketSize2Vtbl =
{
    packetsize_QueryInterface,
    packetsize_AddRef,
    packetsize_Release,
    packetsize_GetMaxPacketSize,
    packetsize_SetMaxPacketSize,
    packetsize_GetMinPacketSize,
    packetsize_SetMinPacketSize
};

HRESULT WINAPI WMCreateReader(IUnknown *reserved, DWORD rights, IWMReader **ret_reader)
{
    WMReader *reader;

    TRACE("(%p, %x, %p)\n", reserved, rights, ret_reader);

    reader = heap_alloc(sizeof(*reader));
    if(!reader)
        return E_OUTOFMEMORY;

    reader->IWMReader_iface.lpVtbl = &WMReaderVtbl;
    reader->IWMReaderAdvanced6_iface.lpVtbl = &WMReaderAdvanced6Vtbl;
    reader->IWMReaderAccelerator_iface.lpVtbl = &WMReaderAcceleratorVtbl;
    reader->IWMReaderNetworkConfig2_iface.lpVtbl = &WMReaderNetworkConfig2Vtbl;
    reader->IWMReaderStreamClock_iface.lpVtbl = &WMReaderStreamClockVtbl;
    reader->IWMReaderTypeNegotiation_iface.lpVtbl = &WMReaderTypeNegotiationVtbl;
    reader->IWMReaderTimecode_iface.lpVtbl = &WMReaderTimecodeVtbl;
    reader->IWMReaderPlaylistBurn_iface.lpVtbl = &WMReaderPlaylistBurnVtbl;
    reader->IWMHeaderInfo3_iface.lpVtbl = &WMHeaderInfo3Vtbl;
    reader->IWMLanguageList_iface.lpVtbl = &WMLanguageListVtbl;
    reader->IReferenceClock_iface.lpVtbl = &ReferenceClockVtbl;
    reader->IWMProfile3_iface.lpVtbl = &WMProfile3Vtbl;
    reader->IWMPacketSize2_iface.lpVtbl = &WMPacketSize2Vtbl;
    reader->ref = 1;

    *ret_reader = &reader->IWMReader_iface;
    return S_OK;
}

HRESULT WINAPI WMCreateReaderPriv(IWMReader **ret_reader)
{
    return WMCreateReader(NULL, 0, ret_reader);
}

typedef struct {
    IWMSyncReader IWMSyncReader_iface;
    LONG ref;
} WMSyncReader;

static inline WMSyncReader *impl_from_IWMSyncReader(IWMSyncReader *iface)
{
    return CONTAINING_RECORD(iface, WMSyncReader, IWMSyncReader_iface);
}

static HRESULT WINAPI WMSyncReader_QueryInterface(IWMSyncReader *iface, REFIID riid, void **ppv)
{
    WMSyncReader *This = impl_from_IWMSyncReader(iface);

    if(IsEqualGUID(riid, &IID_IUnknown)) {
        TRACE("(%p)->(IID_IUnknown %p)\n", This, ppv);
        *ppv = &This->IWMSyncReader_iface;
    }else if(IsEqualGUID(riid, &IID_IWMSyncReader)) {
        TRACE("(%p)->(IID_IWMSyncReader %p)\n", This, ppv);
        *ppv = &This->IWMSyncReader_iface;
    }else {
        *ppv = NULL;
        FIXME("(%p)->(%s %p)\n", This, debugstr_guid(riid), ppv);
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI WMSyncReader_AddRef(IWMSyncReader *iface)
{
    WMSyncReader *This = impl_from_IWMSyncReader(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    return ref;
}

static ULONG WINAPI WMSyncReader_Release(IWMSyncReader *iface)
{
    WMSyncReader *This = impl_from_IWMSyncReader(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    if(!ref)
        heap_free(This);

    return ref;
}

static HRESULT WINAPI WMSyncReader_Close(IWMSyncReader *iface)
{
    WMSyncReader *This = impl_from_IWMSyncReader(iface);
    FIXME("(%p): stub!\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMSyncReader_GetMaxOutputSampleSize(IWMSyncReader *iface, DWORD output, DWORD *max)
{
    WMSyncReader *This = impl_from_IWMSyncReader(iface);
    FIXME("(%p)->(%d %p): stub!\n", This, output, max);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMSyncReader_GetMaxStreamSampleSize(IWMSyncReader *iface, WORD stream, DWORD *max)
{
    WMSyncReader *This = impl_from_IWMSyncReader(iface);
    FIXME("(%p)->(%d %p): stub!\n", This, stream, max);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMSyncReader_GetNextSample(IWMSyncReader *iface, WORD stream, INSSBuffer **sample,
        QWORD *sample_time, QWORD *sample_duration, DWORD *flags, DWORD *output_num, WORD *stream_num)
{
    WMSyncReader *This = impl_from_IWMSyncReader(iface);
    FIXME("(%p)->(%d %p %p %p %p %p %p): stub!\n", This, stream, sample, sample_time,
          sample_duration, flags, output_num, stream_num);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMSyncReader_GetOutputCount(IWMSyncReader *iface, DWORD *outputs)
{
    WMSyncReader *This = impl_from_IWMSyncReader(iface);
    FIXME("(%p)->(%p): stub!\n", This, outputs);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMSyncReader_GetOutputFormat(IWMSyncReader *iface, DWORD output_num, DWORD format_num,
        IWMOutputMediaProps **props)
{
    WMSyncReader *This = impl_from_IWMSyncReader(iface);
    FIXME("(%p)->(%u %u %p): stub!\n", This, output_num, format_num, props);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMSyncReader_GetOutputFormatCount(IWMSyncReader *iface, DWORD output_num, DWORD *formats)
{
    WMSyncReader *This = impl_from_IWMSyncReader(iface);
    FIXME("(%p)->(%u %p): stub!\n", This, output_num, formats);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMSyncReader_GetOutputNumberForStream(IWMSyncReader *iface, WORD stream_num, DWORD *output_num)
{
    WMSyncReader *This = impl_from_IWMSyncReader(iface);
    FIXME("(%p)->(%u %p): stub!\n", This, stream_num, output_num);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMSyncReader_GetOutputProps(IWMSyncReader *iface, DWORD output_num, IWMOutputMediaProps **output)
{
    WMSyncReader *This = impl_from_IWMSyncReader(iface);
    FIXME("(%p)->(%u %p): stub!\n", This, output_num, output);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMSyncReader_GetOutputSetting(IWMSyncReader *iface, DWORD output_num, const WCHAR *name,
        WMT_ATTR_DATATYPE *type, BYTE *value, WORD *length)
{
    WMSyncReader *This = impl_from_IWMSyncReader(iface);
    FIXME("(%p)->(%u %s %p %p %p): stub!\n", This, output_num, debugstr_w(name), type, value, length);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMSyncReader_GetReadStreamSamples(IWMSyncReader *iface, WORD stream_num, BOOL *compressed)
{
    WMSyncReader *This = impl_from_IWMSyncReader(iface);
    FIXME("(%p)->(%d %p): stub!\n", This, stream_num, compressed);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMSyncReader_GetStreamNumberForOutput(IWMSyncReader *iface, DWORD output, WORD *stream_num)
{
    WMSyncReader *This = impl_from_IWMSyncReader(iface);
    FIXME("(%p)->(%u %p): stub!\n", This, output, stream_num);
    return S_OK;
}

static HRESULT WINAPI WMSyncReader_GetStreamSelected(IWMSyncReader *iface, WORD stream_num, WMT_STREAM_SELECTION *selection)
{
    WMSyncReader *This = impl_from_IWMSyncReader(iface);
    FIXME("(%p)->(%d %p): stub!\n", This, stream_num, selection);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMSyncReader_Open(IWMSyncReader *iface, const WCHAR *filename)
{
    WMSyncReader *This = impl_from_IWMSyncReader(iface);
    FIXME("(%p)->(%s): stub!\n", This, debugstr_w(filename));
    return E_NOTIMPL;
}

static HRESULT WINAPI WMSyncReader_OpenStream(IWMSyncReader *iface, IStream *stream)
{
    WMSyncReader *This = impl_from_IWMSyncReader(iface);
    FIXME("(%p)->(%p): stub!\n", This, stream);
    return S_OK;
}

static HRESULT WINAPI WMSyncReader_SetOutputProps(IWMSyncReader *iface, DWORD output_num, IWMOutputMediaProps *output)
{
    WMSyncReader *This = impl_from_IWMSyncReader(iface);
    FIXME("(%p)->(%u %p): stub!\n", This, output_num, output);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMSyncReader_SetOutputSetting(IWMSyncReader *iface, DWORD output_num, const WCHAR *name,
        WMT_ATTR_DATATYPE type, const BYTE *value, WORD length)
{
    WMSyncReader *This = impl_from_IWMSyncReader(iface);
    FIXME("(%p)->(%d %s %d %p %d): stub!\n", This, output_num, debugstr_w(name), type, value, length);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMSyncReader_SetRange(IWMSyncReader *iface, QWORD start, LONGLONG duration)
{
    WMSyncReader *This = impl_from_IWMSyncReader(iface);
    FIXME("(%p)->(%s %s): stub!\n", This, wine_dbgstr_longlong(start), wine_dbgstr_longlong(duration));
    return E_NOTIMPL;
}

static HRESULT WINAPI WMSyncReader_SetRangeByFrame(IWMSyncReader *iface, WORD stream_num, QWORD frame_num,
        LONGLONG frames)
{
    WMSyncReader *This = impl_from_IWMSyncReader(iface);
    FIXME("(%p)->(%d %s %s): stub!\n", This, stream_num, wine_dbgstr_longlong(frame_num), wine_dbgstr_longlong(frames));
    return E_NOTIMPL;
}

static HRESULT WINAPI WMSyncReader_SetReadStreamSamples(IWMSyncReader *iface, WORD stream_num, BOOL compressed)
{
    WMSyncReader *This = impl_from_IWMSyncReader(iface);
    FIXME("(%p)->(%u %x): stub!\n", This, stream_num, compressed);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMSyncReader_SetStreamsSelected(IWMSyncReader *iface, WORD stream_count,
        WORD *stream_numbers, WMT_STREAM_SELECTION *selections)
{
    WMSyncReader *This = impl_from_IWMSyncReader(iface);
    FIXME("(%p)->(%d %p %p): stub!\n", This, stream_count, stream_numbers, selections);
    return S_OK;
}

static const IWMSyncReaderVtbl WMSyncReaderVtbl = {
    WMSyncReader_QueryInterface,
    WMSyncReader_AddRef,
    WMSyncReader_Release,
    WMSyncReader_Open,
    WMSyncReader_Close,
    WMSyncReader_SetRange,
    WMSyncReader_SetRangeByFrame,
    WMSyncReader_GetNextSample,
    WMSyncReader_SetStreamsSelected,
    WMSyncReader_GetStreamSelected,
    WMSyncReader_SetReadStreamSamples,
    WMSyncReader_GetReadStreamSamples,
    WMSyncReader_GetOutputSetting,
    WMSyncReader_SetOutputSetting,
    WMSyncReader_GetOutputCount,
    WMSyncReader_GetOutputProps,
    WMSyncReader_SetOutputProps,
    WMSyncReader_GetOutputFormatCount,
    WMSyncReader_GetOutputFormat,
    WMSyncReader_GetOutputNumberForStream,
    WMSyncReader_GetStreamNumberForOutput,
    WMSyncReader_GetMaxOutputSampleSize,
    WMSyncReader_GetMaxStreamSampleSize,
    WMSyncReader_OpenStream
};

HRESULT WINAPI WMCreateSyncReader(IUnknown *pcert, DWORD rights, IWMSyncReader **syncreader)
{
    WMSyncReader *sync;

    TRACE("(%p, %x, %p)\n", pcert, rights, syncreader);

    sync = heap_alloc(sizeof(*sync));

    if (!sync)
        return E_OUTOFMEMORY;

    sync->IWMSyncReader_iface.lpVtbl = &WMSyncReaderVtbl;
    sync->ref = 1;

    *syncreader = &sync->IWMSyncReader_iface;

    return S_OK;
}

HRESULT WINAPI WMCreateSyncReaderPriv(IWMSyncReader **syncreader)
{
    return WMCreateSyncReader(NULL, 0, syncreader);
}
