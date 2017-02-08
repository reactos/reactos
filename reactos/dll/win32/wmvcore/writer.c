/*
 * Copyright 2015 Jacek Caban for CodeWeavers
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
#include "wmsdkidl.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(wmvcore);

typedef struct {
    IWMWriter IWMWriter_iface;
    IWMWriterAdvanced3 IWMWriterAdvanced3_iface;
    LONG ref;
} WMWriter;

static inline WMWriter *impl_from_IWMWriter(IWMWriter *iface)
{
    return CONTAINING_RECORD(iface, WMWriter, IWMWriter_iface);
}

static HRESULT WINAPI WMWriter_QueryInterface(IWMWriter *iface, REFIID riid, void **ppv)
{
    WMWriter *This = impl_from_IWMWriter(iface);

    if(IsEqualGUID(&IID_IUnknown, riid)) {
        TRACE("(%p)->(IID_IUnknown %p)\n", This, ppv);
        *ppv = &This->IWMWriter_iface;
    }else if(IsEqualGUID(&IID_IWMWriter, riid)) {
        TRACE("(%p)->(IID_IWMWriter %p)\n", This, ppv);
        *ppv = &This->IWMWriter_iface;
    }else if(IsEqualGUID(&IID_IWMWriterAdvanced, riid)) {
        TRACE("(%p)->(IID_IWMWriterAdvanced %p)\n", This, ppv);
        *ppv = &This->IWMWriterAdvanced3_iface;
    }else if(IsEqualGUID(&IID_IWMWriterAdvanced2, riid)) {
        TRACE("(%p)->(IID_IWMWriterAdvanced2 %p)\n", This, ppv);
        *ppv = &This->IWMWriterAdvanced3_iface;
    }else if(IsEqualGUID(&IID_IWMWriterAdvanced3, riid)) {
        TRACE("(%p)->(IID_IWMWriterAdvanced3 %p)\n", This, ppv);
        *ppv = &This->IWMWriterAdvanced3_iface;
    }else {
        FIXME("Unsupported iface %s\n", debugstr_guid(riid));
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI WMWriter_AddRef(IWMWriter *iface)
{
    WMWriter *This = impl_from_IWMWriter(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    return ref;
}

static ULONG WINAPI WMWriter_Release(IWMWriter *iface)
{
    WMWriter *This = impl_from_IWMWriter(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    if(!ref)
        heap_free(This);

    return ref;
}

static HRESULT WINAPI WMWriter_SetProfileByID(IWMWriter *iface, REFGUID guidProfile)
{
    WMWriter *This = impl_from_IWMWriter(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_guid(guidProfile));
    return E_NOTIMPL;
}

static HRESULT WINAPI WMWriter_SetProfile(IWMWriter *iface, IWMProfile *profile)
{
    WMWriter *This = impl_from_IWMWriter(iface);
    FIXME("(%p)->(%p)\n", This, profile);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMWriter_SetOutputFilename(IWMWriter *iface, const WCHAR *filename)
{
    WMWriter *This = impl_from_IWMWriter(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(filename));
    return E_NOTIMPL;
}

static HRESULT WINAPI WMWriter_GetInputCount(IWMWriter *iface, DWORD *pcInputs)
{
    WMWriter *This = impl_from_IWMWriter(iface);
    FIXME("(%p)->(%p)\n", This, pcInputs);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMWriter_GetInputProps(IWMWriter *iface, DWORD dwInputNum, IWMInputMediaProps **input)
{
    WMWriter *This = impl_from_IWMWriter(iface);
    FIXME("(%p)->(%d %p)\n", This, dwInputNum, input);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMWriter_SetInputProps(IWMWriter *iface, DWORD dwInputNum, IWMInputMediaProps *input)
{
    WMWriter *This = impl_from_IWMWriter(iface);
    FIXME("(%p)->(%d %p)\n", This, dwInputNum, input);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMWriter_GetInputFormatCount(IWMWriter *iface, DWORD dwInputNumber, DWORD *pcFormat)
{
    WMWriter *This = impl_from_IWMWriter(iface);
    FIXME("(%p)->(%d %p)\n", This, dwInputNumber, pcFormat);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMWriter_GetInputFormat(IWMWriter *iface, DWORD dwInputNumber, DWORD dwFormatNumber,
        IWMInputMediaProps **props)
{
    WMWriter *This = impl_from_IWMWriter(iface);
    FIXME("(%p)->(%d %d %p)\n", This, dwInputNumber, dwFormatNumber, props);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMWriter_BeginWriting(IWMWriter *iface)
{
    WMWriter *This = impl_from_IWMWriter(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMWriter_EndWriting(IWMWriter *iface)
{
    WMWriter *This = impl_from_IWMWriter(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMWriter_AllocateSample(IWMWriter *iface, DWORD size, INSSBuffer **sample)
{
    WMWriter *This = impl_from_IWMWriter(iface);
    FIXME("(%p)->(%d %p)\n", This, size, sample);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMWriter_WriteSample(IWMWriter *iface, DWORD dwInputNum, QWORD cnsSampleTime,
        DWORD flags, INSSBuffer *sample)
{
    WMWriter *This = impl_from_IWMWriter(iface);
    FIXME("(%p)->(%d %s %x %p)\n", This, dwInputNum, wine_dbgstr_longlong(cnsSampleTime), flags, sample);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMWriter_Flush(IWMWriter *iface)
{
    WMWriter *This = impl_from_IWMWriter(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static const IWMWriterVtbl WMWriterVtbl = {
    WMWriter_QueryInterface,
    WMWriter_AddRef,
    WMWriter_Release,
    WMWriter_SetProfileByID,
    WMWriter_SetProfile,
    WMWriter_SetOutputFilename,
    WMWriter_GetInputCount,
    WMWriter_GetInputProps,
    WMWriter_SetInputProps,
    WMWriter_GetInputFormatCount,
    WMWriter_GetInputFormat,
    WMWriter_BeginWriting,
    WMWriter_EndWriting,
    WMWriter_AllocateSample,
    WMWriter_WriteSample,
    WMWriter_Flush
};

static inline WMWriter *impl_from_IWMWriterAdvanced3(IWMWriterAdvanced3 *iface)
{
    return CONTAINING_RECORD(iface, WMWriter, IWMWriterAdvanced3_iface);
}

static HRESULT WINAPI WMWriterAdvanced_QueryInterface(IWMWriterAdvanced3 *iface, REFIID riid, void **ppv)
{
    WMWriter *This = impl_from_IWMWriterAdvanced3(iface);
    return IWMWriter_QueryInterface(&This->IWMWriter_iface, riid, ppv);
}

static ULONG WINAPI WMWriterAdvanced_AddRef(IWMWriterAdvanced3 *iface)
{
    WMWriter *This = impl_from_IWMWriterAdvanced3(iface);
    return IWMWriter_AddRef(&This->IWMWriter_iface);
}

static ULONG WINAPI WMWriterAdvanced_Release(IWMWriterAdvanced3 *iface)
{
    WMWriter *This = impl_from_IWMWriterAdvanced3(iface);
    return IWMWriter_Release(&This->IWMWriter_iface);
}

static HRESULT WINAPI WMWriterAdvanced_GetSinkCount(IWMWriterAdvanced3 *iface, DWORD *sinks)
{
    WMWriter *This = impl_from_IWMWriterAdvanced3(iface);
    FIXME("(%p)->(%p)\n", This, sinks);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMWriterAdvanced_GetSink(IWMWriterAdvanced3 *iface, DWORD sink_num, IWMWriterSink **sink)
{
    WMWriter *This = impl_from_IWMWriterAdvanced3(iface);
    FIXME("(%p)->(%u %p)\n", This, sink_num, sink);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMWriterAdvanced_AddSink(IWMWriterAdvanced3 *iface, IWMWriterSink *sink)
{
    WMWriter *This = impl_from_IWMWriterAdvanced3(iface);
    FIXME("(%p)->(%p)\n", This, sink);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMWriterAdvanced_RemoveSink(IWMWriterAdvanced3 *iface, IWMWriterSink *sink)
{
    WMWriter *This = impl_from_IWMWriterAdvanced3(iface);
    FIXME("(%p)->(%p)\n", This, sink);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMWriterAdvanced_WriteStreamSample(IWMWriterAdvanced3 *iface, WORD stream_num,
        QWORD sample_time, DWORD sample_send_time, QWORD sample_duration, DWORD flags, INSSBuffer *sample)
{
    WMWriter *This = impl_from_IWMWriterAdvanced3(iface);
    FIXME("(%p)->(%u %s %u %s %x %p)\n", This, stream_num, wine_dbgstr_longlong(sample_time),
          sample_send_time, wine_dbgstr_longlong(sample_duration), flags, sample);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMWriterAdvanced_SetLiveSource(IWMWriterAdvanced3 *iface, BOOL is_live_source)
{
    WMWriter *This = impl_from_IWMWriterAdvanced3(iface);
    FIXME("(%p)->(%x)\n", This, is_live_source);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMWriterAdvanced_IsRealTime(IWMWriterAdvanced3 *iface, BOOL *real_time)
{
    WMWriter *This = impl_from_IWMWriterAdvanced3(iface);
    FIXME("(%p)->(%p)\n", This, real_time);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMWriterAdvanced_GetWriterTime(IWMWriterAdvanced3 *iface, QWORD *current_time)
{
    WMWriter *This = impl_from_IWMWriterAdvanced3(iface);
    FIXME("(%p)->(%p)\n", This, current_time);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMWriterAdvanced_GetStatistics(IWMWriterAdvanced3 *iface, WORD stream_num, WM_WRITER_STATISTICS *stats)
{
    WMWriter *This = impl_from_IWMWriterAdvanced3(iface);
    FIXME("(%p)->(%u %p)\n", This, stream_num, stats);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMWriterAdvanced_SetSyncTolerance(IWMWriterAdvanced3 *iface, DWORD window)
{
    WMWriter *This = impl_from_IWMWriterAdvanced3(iface);
    FIXME("(%p)->(%u)\n", This, window);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMWriterAdvanced_GetSyncTolerance(IWMWriterAdvanced3 *iface, DWORD *window)
{
    WMWriter *This = impl_from_IWMWriterAdvanced3(iface);
    FIXME("(%p)->(%p)\n", This, window);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMWriterAdvanced2_GetInputSetting(IWMWriterAdvanced3 *iface, DWORD input_num,
        const WCHAR *name, WMT_ATTR_DATATYPE *time, BYTE *value, WORD *length)
{
    WMWriter *This = impl_from_IWMWriterAdvanced3(iface);
    FIXME("(%p)->(%u %s %p %p %p)\n", This, input_num, debugstr_w(name), time, value, length);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMWriterAdvanced2_SetInputSetting(IWMWriterAdvanced3 *iface, DWORD input_num,
        const WCHAR *name, WMT_ATTR_DATATYPE type, const BYTE *value, WORD length)
{
    WMWriter *This = impl_from_IWMWriterAdvanced3(iface);
    FIXME("(%p)->(%u %s %d %p %u)\n", This, input_num, debugstr_w(name), type, value, length);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMWriterAdvanced3_GetStatisticsEx(IWMWriterAdvanced3 *iface, WORD stream_num,
        WM_WRITER_STATISTICS_EX *stats)
{
    WMWriter *This = impl_from_IWMWriterAdvanced3(iface);
    FIXME("(%p)->(%u %p)\n", This, stream_num, stats);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMWriterAdvanced3_SetNonBlocking(IWMWriterAdvanced3 *iface)
{
    WMWriter *This = impl_from_IWMWriterAdvanced3(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static const IWMWriterAdvanced3Vtbl WMWriterAdvanced3Vtbl = {
    WMWriterAdvanced_QueryInterface,
    WMWriterAdvanced_AddRef,
    WMWriterAdvanced_Release,
    WMWriterAdvanced_GetSinkCount,
    WMWriterAdvanced_GetSink,
    WMWriterAdvanced_AddSink,
    WMWriterAdvanced_RemoveSink,
    WMWriterAdvanced_WriteStreamSample,
    WMWriterAdvanced_SetLiveSource,
    WMWriterAdvanced_IsRealTime,
    WMWriterAdvanced_GetWriterTime,
    WMWriterAdvanced_GetStatistics,
    WMWriterAdvanced_SetSyncTolerance,
    WMWriterAdvanced_GetSyncTolerance,
    WMWriterAdvanced2_GetInputSetting,
    WMWriterAdvanced2_SetInputSetting,
    WMWriterAdvanced3_GetStatisticsEx,
    WMWriterAdvanced3_SetNonBlocking
};

HRESULT WINAPI WMCreateWriter(IUnknown *reserved, IWMWriter **writer)
{
    WMWriter *ret;

    TRACE("(%p %p)\n", reserved, writer);

    ret = heap_alloc(sizeof(*ret));
    if(!ret)
        return E_OUTOFMEMORY;

    ret->IWMWriter_iface.lpVtbl = &WMWriterVtbl;
    ret->IWMWriterAdvanced3_iface.lpVtbl = &WMWriterAdvanced3Vtbl;
    ret->ref = 1;

    *writer = &ret->IWMWriter_iface;
    return S_OK;
}
