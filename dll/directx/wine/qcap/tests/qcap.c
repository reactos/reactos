/*
 *    QCAP tests
 *
 * Copyright 2013 Damjan Jovanovic
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

#include "windef.h"
#include "winbase.h"
#define COBJMACROS
#include <dshow.h>
#include <guiddef.h>
#include <devguid.h>
#include <stdio.h>

#include "wine/strmbase.h"
#include "wine/test.h"

#define DEFINE_EXPECT(func) \
    static BOOL expect_ ## func = FALSE, called_ ## func = FALSE

#define SET_EXPECT(func) \
    expect_ ## func = TRUE

#define CHECK_EXPECT2(func) \
    do { \
        ok(expect_ ##func, "unexpected call " #func "\n"); \
        called_ ## func = TRUE; \
    }while(0)

#define CHECK_EXPECT(func) \
    do { \
        CHECK_EXPECT2(func); \
        expect_ ## func = FALSE; \
    }while(0)

#define CHECK_CALLED(func) \
    do { \
        ok(called_ ## func, "expected " #func "\n"); \
        expect_ ## func = called_ ## func = FALSE; \
    }while(0)

DEFINE_EXPECT(ReceiveConnection);
DEFINE_EXPECT(GetAllocatorRequirements);
DEFINE_EXPECT(NotifyAllocator);
DEFINE_EXPECT(Reconnect);
DEFINE_EXPECT(MediaSeeking_GetPositions);
DEFINE_EXPECT(MemAllocator_GetProperties);
DEFINE_EXPECT(MemInputPin_QueryInterface_IStream);
DEFINE_EXPECT(MediaSample_QueryInterface_MediaSample2);
DEFINE_EXPECT(MediaSample_IsDiscontinuity);
DEFINE_EXPECT(MediaSample_IsPreroll);
DEFINE_EXPECT(MediaSample_IsSyncPoint);
DEFINE_EXPECT(MediaSample_GetTime);
DEFINE_EXPECT(MediaSample_GetMediaType);
DEFINE_EXPECT(MediaSample_GetPointer);
DEFINE_EXPECT(MediaSample_GetActualDataLength);
DEFINE_EXPECT(MediaSample_GetSize);
DEFINE_EXPECT(MediaSample_GetMediaTime);

typedef enum {
    SOURCE_FILTER,
    SINK_FILTER,
    INTERMEDIATE_FILTER,
    NOT_FILTER
} filter_type;

static const char* debugstr_filter_type(filter_type type)
{
    switch(type) {
    case SOURCE_FILTER:
        return "SOURCE_FILTER";
    case SINK_FILTER:
        return "SINK_FILTER";
    case INTERMEDIATE_FILTER:
        return "INTERMEDIATE_FILTER";
    default:
        return "NOT_FILTER";
    }
}

typedef enum {
    BASEFILTER_ENUMPINS,
    ENUMPINS_NEXT,
    PIN_QUERYDIRECTION,
    PIN_CONNECTEDTO,
    PIN_QUERYPININFO,
    KSPROPERTYSET_GET,
    PIN_ENUMMEDIATYPES,
    ENUMMEDIATYPES_RESET,
    ENUMMEDIATYPES_NEXT,
    GRAPHBUILDER_CONNECT,
    BASEFILTER_GETSTATE,
    BASEFILTER_QUERYINTERFACE,
    END
} call_id;

static const struct {
    call_id call_id;
    filter_type filter_type;
    BOOL wine_missing;
    BOOL wine_extra;
    BOOL optional; /* fails on wine if missing */
    BOOL broken;
} *current_calls_list;
int call_no;

static void check_calls_list(const char *func, call_id id, filter_type type)
{
    if(!current_calls_list)
        return;

    while(current_calls_list[call_no].wine_missing || current_calls_list[call_no].wine_extra ||
         current_calls_list[call_no].optional || current_calls_list[call_no].broken) {
        if(current_calls_list[call_no].wine_missing) {
            todo_wine ok((current_calls_list[call_no].call_id == id && current_calls_list[call_no].filter_type == type) ||
                    broken(current_calls_list[call_no].optional && (current_calls_list[call_no].call_id != id ||
                            current_calls_list[call_no].filter_type != type)),
                    "missing call, got %s(%d), expected %d (%d)\n", func, id, current_calls_list[call_no].call_id, call_no);

            if(current_calls_list[call_no].call_id != id || current_calls_list[call_no].filter_type != type)
                call_no++;
            else
                break;
        }else if(current_calls_list[call_no].wine_extra) {
            todo_wine ok(current_calls_list[call_no].call_id != id || current_calls_list[call_no].filter_type != type,
                    "extra call, got %s(%d) (%d)\n", func, id, call_no);

            if(current_calls_list[call_no].call_id == id && current_calls_list[call_no].filter_type == type) {
                call_no++;
                return;
            }
            call_no++;
        }else if(current_calls_list[call_no].optional) {
            ok((current_calls_list[call_no].call_id == id && current_calls_list[call_no].filter_type == type) ||
                    broken(current_calls_list[call_no].call_id != id || current_calls_list[call_no].filter_type != type),
                    "unexpected call: %s on %s (%d)\n", func, debugstr_filter_type(type), call_no);

            if(current_calls_list[call_no].call_id != id || current_calls_list[call_no].filter_type != type)
                call_no++;
            else
                break;
        }else if(current_calls_list[call_no].broken) {
            ok(broken(current_calls_list[call_no].call_id == id && current_calls_list[call_no].filter_type == type) ||
                    (current_calls_list[call_no].call_id != id || current_calls_list[call_no].filter_type != type),
                    "unexpected call: %s on %s (%d)\n", func, debugstr_filter_type(type), call_no);

            if(current_calls_list[call_no].call_id == id && current_calls_list[call_no].filter_type == type)
                break;
            call_no++;
        }
    }

    ok(current_calls_list[call_no].call_id == id, "unexpected call: %s on %s (%d)\n",
            func, debugstr_filter_type(type), call_no);
    if(current_calls_list[call_no].call_id != id)
        return;

    ok(current_calls_list[call_no].filter_type == type, "unexpected call: %s on %s (%d)\n",
            func, debugstr_filter_type(type), call_no);
    if(current_calls_list[call_no].filter_type != type)
        return;

    call_no++;
}

static HRESULT WINAPI GraphBuilder_QueryInterface(
        IGraphBuilder *iface, REFIID riid, void **ppv)
{
    if(IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_IFilterGraph)
            || IsEqualIID(riid, &IID_IGraphBuilder))
    {
        *ppv = iface;
        return S_OK;
    }

    ok(IsEqualIID(riid, &IID_IMediaEvent) || IsEqualIID(riid, &IID_IMediaEventSink),
            "QueryInterface(%s)\n", wine_dbgstr_guid(riid));
    *ppv = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI GraphBuilder_AddRef(IGraphBuilder *iface)
{
    return 2;
}

static ULONG WINAPI GraphBuilder_Release(IGraphBuilder *iface)
{
    return 1;
}

static HRESULT WINAPI GraphBuilder_AddFilter(IGraphBuilder *iface,
        IBaseFilter *pFilter, LPCWSTR pName)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI GraphBuilder_RemoveFilter(
        IGraphBuilder *iface, IBaseFilter *pFilter)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI GraphBuilder_EnumFilters(
        IGraphBuilder *iface, IEnumFilters **ppEnum)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI GraphBuilder_FindFilterByName(IGraphBuilder *iface,
        LPCWSTR pName, IBaseFilter **ppFilter)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI GraphBuilder_ConnectDirect(IGraphBuilder *iface,
        IPin *ppinOut, IPin *ppinIn, const AM_MEDIA_TYPE *pmt)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI GraphBuilder_Reconnect(IGraphBuilder *iface, IPin *ppin)
{
    CHECK_EXPECT(Reconnect);
    return S_OK;
}

static HRESULT WINAPI GraphBuilder_Disconnect(IGraphBuilder *iface, IPin *ppin)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI GraphBuilder_SetDefaultSyncSource(IGraphBuilder *iface)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI GraphBuilder_Connect(IGraphBuilder *iface, IPin *ppinOut, IPin *ppinIn)
{
    check_calls_list("GraphBuilder_Connect", GRAPHBUILDER_CONNECT, NOT_FILTER);
    return S_OK;
}

static HRESULT WINAPI GraphBuilder_Render(IGraphBuilder *iface, IPin *ppinOut)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI GraphBuilder_RenderFile(IGraphBuilder *iface,
        LPCWSTR lpcwstrFile, LPCWSTR lpcwstrPlayList)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI GraphBuilder_AddSourceFilter(IGraphBuilder *iface, LPCWSTR lpcwstrFileName,
        LPCWSTR lpcwstrFilterName, IBaseFilter **ppFilter)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI GraphBuilder_SetLogFile(IGraphBuilder *iface, DWORD_PTR hFile)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI GraphBuilder_Abort(IGraphBuilder *iface)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI GraphBuilder_ShouldOperationContinue(IGraphBuilder *iface)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static const IGraphBuilderVtbl GraphBuilder_vtbl = {
    GraphBuilder_QueryInterface,
    GraphBuilder_AddRef,
    GraphBuilder_Release,
    GraphBuilder_AddFilter,
    GraphBuilder_RemoveFilter,
    GraphBuilder_EnumFilters,
    GraphBuilder_FindFilterByName,
    GraphBuilder_ConnectDirect,
    GraphBuilder_Reconnect,
    GraphBuilder_Disconnect,
    GraphBuilder_SetDefaultSyncSource,
    GraphBuilder_Connect,
    GraphBuilder_Render,
    GraphBuilder_RenderFile,
    GraphBuilder_AddSourceFilter,
    GraphBuilder_SetLogFile,
    GraphBuilder_Abort,
    GraphBuilder_ShouldOperationContinue
};

static IGraphBuilder GraphBuilder = {&GraphBuilder_vtbl};

typedef struct {
    IBaseFilter IBaseFilter_iface;
    IEnumPins IEnumPins_iface;
    IPin IPin_iface;
    IKsPropertySet IKsPropertySet_iface;
    IMemInputPin IMemInputPin_iface;
    IMediaSeeking IMediaSeeking_iface;
    IEnumMediaTypes IEnumMediaTypes_iface;

    PIN_DIRECTION dir;
    filter_type filter_type;

    int enum_pins_pos;
    int enum_media_types_pos;
} test_filter;

static test_filter* impl_from_IBaseFilter(IBaseFilter *iface)
{
    return CONTAINING_RECORD(iface, test_filter, IBaseFilter_iface);
}

static HRESULT WINAPI BaseFilter_QueryInterface(IBaseFilter *iface, REFIID riid, void **ppv)
{
    test_filter *This = impl_from_IBaseFilter(iface);

    if(IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_IPersist)
            || IsEqualIID(riid, &IID_IMediaFilter) || IsEqualIID(riid, &IID_IBaseFilter)) {
        *ppv = iface;
        return S_OK;
    }

    check_calls_list("BaseFilter_QueryInterface", BASEFILTER_QUERYINTERFACE, This->filter_type);
    ok(IsEqualIID(riid, &IID_IPin), "riid = %s\n", wine_dbgstr_guid(riid));
    return E_NOINTERFACE;
}

static ULONG WINAPI BaseFilter_AddRef(IBaseFilter *iface)
{
    return 2;
}

static ULONG WINAPI BaseFilter_Release(IBaseFilter *iface)
{
    return 1;
}

static HRESULT WINAPI BaseFilter_GetClassID(IBaseFilter *iface, CLSID *pClassID)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI BaseFilter_Stop(IBaseFilter *iface)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI BaseFilter_Pause(IBaseFilter *iface)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI BaseFilter_Run(IBaseFilter *iface, REFERENCE_TIME tStart)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI BaseFilter_GetState(IBaseFilter *iface,
        DWORD dwMilliSecsTimeout, FILTER_STATE *State)
{
    test_filter *This = impl_from_IBaseFilter(iface);
    check_calls_list("BaseFilter_GetState", BASEFILTER_GETSTATE, This->filter_type);
    return E_NOTIMPL;
}

static HRESULT WINAPI BaseFilter_SetSyncSource(
        IBaseFilter *iface, IReferenceClock *pClock)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI BaseFilter_GetSyncSource(
        IBaseFilter *iface, IReferenceClock **pClock)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI BaseFilter_EnumPins(
        IBaseFilter *iface, IEnumPins **ppEnum)
{
    test_filter *This = impl_from_IBaseFilter(iface);
    check_calls_list("BaseFilter_EnumPins", BASEFILTER_ENUMPINS, This->filter_type);

    *ppEnum = &This->IEnumPins_iface;
    This->enum_pins_pos = 0;
    return S_OK;
}

static HRESULT WINAPI BaseFilter_FindPin(IBaseFilter *iface,
        LPCWSTR Id, IPin **ppPin)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI BaseFilter_QueryFilterInfo(IBaseFilter *iface, FILTER_INFO *pInfo)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI BaseFilter_JoinFilterGraph(IBaseFilter *iface,
        IFilterGraph *pGraph, LPCWSTR pName)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI BaseFilter_QueryVendorInfo(IBaseFilter *iface, LPWSTR *pVendorInfo)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static const IBaseFilterVtbl BaseFilterVtbl = {
    BaseFilter_QueryInterface,
    BaseFilter_AddRef,
    BaseFilter_Release,
    BaseFilter_GetClassID,
    BaseFilter_Stop,
    BaseFilter_Pause,
    BaseFilter_Run,
    BaseFilter_GetState,
    BaseFilter_SetSyncSource,
    BaseFilter_GetSyncSource,
    BaseFilter_EnumPins,
    BaseFilter_FindPin,
    BaseFilter_QueryFilterInfo,
    BaseFilter_JoinFilterGraph,
    BaseFilter_QueryVendorInfo
};

static test_filter* impl_from_IEnumPins(IEnumPins *iface)
{
    return CONTAINING_RECORD(iface, test_filter, IEnumPins_iface);
}

static HRESULT WINAPI EnumPins_QueryInterface(IEnumPins *iface, REFIID riid, void **ppv)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static ULONG WINAPI EnumPins_AddRef(IEnumPins *iface)
{
    return 2;
}

static ULONG WINAPI EnumPins_Release(IEnumPins *iface)
{
    return 1;
}

static HRESULT WINAPI EnumPins_Next(IEnumPins *iface,
        ULONG cPins, IPin **ppPins, ULONG *pcFetched)
{
    test_filter *This = impl_from_IEnumPins(iface);
    check_calls_list("EnumPins_Next", ENUMPINS_NEXT, This->filter_type);

    ok(cPins == 1, "cPins = %ld\n", cPins);
    ok(ppPins != NULL, "ppPins == NULL\n");
    ok(pcFetched != NULL, "pcFetched == NULL\n");

    if(This->enum_pins_pos++ < (This->filter_type == INTERMEDIATE_FILTER ? 2 : 1)) {
        *ppPins = &This->IPin_iface;
        *pcFetched = 1;
        return S_OK;
    }
    *pcFetched = 0;
    return S_FALSE;
}

static HRESULT WINAPI EnumPins_Skip(IEnumPins *iface, ULONG cPins)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI EnumPins_Reset(IEnumPins *iface)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI EnumPins_Clone(IEnumPins *iface, IEnumPins **ppEnum)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static const IEnumPinsVtbl EnumPinsVtbl = {
    EnumPins_QueryInterface,
    EnumPins_AddRef,
    EnumPins_Release,
    EnumPins_Next,
    EnumPins_Skip,
    EnumPins_Reset,
    EnumPins_Clone
};

static test_filter* impl_from_IPin(IPin *iface)
{
    return CONTAINING_RECORD(iface, test_filter, IPin_iface);
}

static HRESULT WINAPI Pin_QueryInterface(IPin *iface, REFIID riid, void **ppv)
{
    test_filter *This = impl_from_IPin(iface);

    if(IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_IPin)) {
        *ppv = iface;
        return S_OK;
    }

    if(IsEqualIID(riid, &IID_IKsPropertySet)) {
        *ppv = &This->IKsPropertySet_iface;
        return S_OK;
    }

    if(IsEqualIID(riid, &IID_IMemInputPin)) {
        *ppv = &This->IMemInputPin_iface;
        return S_OK;
    }

    if(IsEqualIID(riid, &IID_IMediaSeeking)) {
        *ppv = &This->IMediaSeeking_iface;
        return S_OK;
    }

    ok(0, "unexpected call: %s\n", wine_dbgstr_guid(riid));
    *ppv = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI Pin_AddRef(IPin *iface)
{
    return 2;
}

static ULONG WINAPI Pin_Release(IPin *iface)
{
    return 1;
}

static HRESULT WINAPI Pin_Connect(IPin *iface, IPin *pReceivePin, const AM_MEDIA_TYPE *pmt)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI Pin_ReceiveConnection(IPin *iface,
        IPin *pConnector, const AM_MEDIA_TYPE *pmt)
{
    CHECK_EXPECT(ReceiveConnection);

    ok(IsEqualIID(&pmt->majortype, &MEDIATYPE_Stream), "majortype = %s\n",
            wine_dbgstr_guid(&pmt->majortype));
    ok(IsEqualIID(&pmt->subtype, &MEDIASUBTYPE_Avi), "subtype = %s\n",
            wine_dbgstr_guid(&pmt->subtype));
    ok(pmt->bFixedSizeSamples, "bFixedSizeSamples = %x\n", pmt->bFixedSizeSamples);
    ok(!pmt->bTemporalCompression, "bTemporalCompression = %x\n", pmt->bTemporalCompression);
    ok(pmt->lSampleSize == 1, "lSampleSize = %ld\n", pmt->lSampleSize);
    ok(IsEqualIID(&pmt->formattype, &GUID_NULL), "formattype = %s\n",
            wine_dbgstr_guid(&pmt->formattype));
    ok(!pmt->pUnk, "pUnk = %p\n", pmt->pUnk);
    ok(!pmt->cbFormat, "cbFormat = %ld\n", pmt->cbFormat);
    ok(!pmt->pbFormat, "pbFormat = %p\n", pmt->pbFormat);
    return S_OK;
}

static HRESULT WINAPI Pin_Disconnect(IPin *iface)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI Pin_ConnectedTo(IPin *iface, IPin **pPin)
{
    test_filter *This = impl_from_IPin(iface);
    check_calls_list("Pin_ConnectedTo", PIN_CONNECTEDTO, This->filter_type);

    *pPin = NULL;
    return S_OK;
}

static HRESULT WINAPI Pin_ConnectionMediaType(IPin *iface, AM_MEDIA_TYPE *pmt)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI Pin_QueryPinInfo(IPin *iface, PIN_INFO *pInfo)
{
    test_filter *This = impl_from_IPin(iface);
    check_calls_list("Pin_QueryPinInfo", PIN_QUERYPININFO, This->filter_type);
    return E_NOTIMPL;
}

static HRESULT WINAPI Pin_QueryDirection(IPin *iface, PIN_DIRECTION *pPinDir)
{
    test_filter *This = impl_from_IPin(iface);
    check_calls_list("Pin_QueryDirection", PIN_QUERYDIRECTION, This->filter_type);

    *pPinDir = This->dir;
    if(This->filter_type==INTERMEDIATE_FILTER && This->enum_pins_pos==2)
        *pPinDir = PINDIR_INPUT;
    return S_OK;
}

static HRESULT WINAPI Pin_QueryId(IPin *iface, LPWSTR *Id)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI Pin_QueryAccept(IPin *iface, const AM_MEDIA_TYPE *pmt)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI Pin_EnumMediaTypes(IPin *iface, IEnumMediaTypes **ppEnum)
{
    test_filter *This = impl_from_IPin(iface);
    check_calls_list("Pin_EnumMediaTypes", PIN_ENUMMEDIATYPES, This->filter_type);

    ok(ppEnum != NULL, "ppEnum == NULL\n");
    *ppEnum = &This->IEnumMediaTypes_iface;
    return S_OK;
}

static HRESULT WINAPI Pin_QueryInternalConnections(IPin *iface, IPin **apPin, ULONG *nPin)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI Pin_EndOfStream(IPin *iface)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI Pin_BeginFlush(IPin *iface)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI Pin_EndFlush(IPin *iface)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI Pin_NewSegment(IPin *iface, REFERENCE_TIME tStart,
        REFERENCE_TIME tStop, double dRate)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static const IPinVtbl PinVtbl = {
    Pin_QueryInterface,
    Pin_AddRef,
    Pin_Release,
    Pin_Connect,
    Pin_ReceiveConnection,
    Pin_Disconnect,
    Pin_ConnectedTo,
    Pin_ConnectionMediaType,
    Pin_QueryPinInfo,
    Pin_QueryDirection,
    Pin_QueryId,
    Pin_QueryAccept,
    Pin_EnumMediaTypes,
    Pin_QueryInternalConnections,
    Pin_EndOfStream,
    Pin_BeginFlush,
    Pin_EndFlush,
    Pin_NewSegment
};

static test_filter* impl_from_IKsPropertySet(IKsPropertySet *iface)
{
    return CONTAINING_RECORD(iface, test_filter, IKsPropertySet_iface);
}

static HRESULT WINAPI KsPropertySet_QueryInterface(IKsPropertySet *iface, REFIID riid, void **ppv)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static ULONG WINAPI KsPropertySet_AddRef(IKsPropertySet *iface)
{
    return 2;
}

static ULONG WINAPI KsPropertySet_Release(IKsPropertySet *iface)
{
    return 1;
}

static HRESULT WINAPI KsPropertySet_Set(IKsPropertySet *iface, REFGUID guidPropSet, DWORD dwPropID,
        LPVOID pInstanceData, DWORD cbInstanceData, LPVOID pPropData, DWORD cbPropData)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI KsPropertySet_Get(IKsPropertySet *iface, REFGUID guidPropSet, DWORD dwPropID,
        LPVOID pInstanceData, DWORD cbInstanceData, LPVOID pPropData, DWORD cbPropData, DWORD *pcbReturned)
{
    test_filter *This = impl_from_IKsPropertySet(iface);
    check_calls_list("KsPropertySet_Get", KSPROPERTYSET_GET, This->filter_type);

    ok(IsEqualIID(guidPropSet, &AMPROPSETID_Pin), "guidPropSet = %s\n", wine_dbgstr_guid(guidPropSet));
    ok(dwPropID == 0, "dwPropID = %ld\n", dwPropID);
    ok(pInstanceData == NULL, "pInstanceData != NULL\n");
    ok(cbInstanceData == 0, "cbInstanceData != 0\n");
    ok(cbPropData == sizeof(GUID), "cbPropData = %ld\n", cbPropData);
    *pcbReturned = sizeof(GUID);
    memcpy(pPropData, &PIN_CATEGORY_EDS, sizeof(GUID));
    return S_OK;
}

static HRESULT WINAPI KsPropertySet_QuerySupported(IKsPropertySet *iface,
        REFGUID guidPropSet, DWORD dwPropID, DWORD *pTypeSupport)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static const IKsPropertySetVtbl KsPropertySetVtbl = {
    KsPropertySet_QueryInterface,
    KsPropertySet_AddRef,
    KsPropertySet_Release,
    KsPropertySet_Set,
    KsPropertySet_Get,
    KsPropertySet_QuerySupported
};

static IStream *avi_stream;
static HRESULT WINAPI MemInputPin_QueryInterface(IMemInputPin *iface, REFIID riid, void **ppv)
{
    if(IsEqualIID(riid, &IID_IStream)) {
        CHECK_EXPECT(MemInputPin_QueryInterface_IStream);

        if(!avi_stream)
            return E_NOINTERFACE;

        *ppv = avi_stream;
        IStream_AddRef(avi_stream);
        return S_OK;
    }

    ok(0, "unexpected call: %s\n", wine_dbgstr_guid(riid));
    return E_NOTIMPL;
}

static ULONG WINAPI MemInputPin_AddRef(IMemInputPin *iface)
{
    return 2;
}

static ULONG WINAPI MemInputPin_Release(IMemInputPin *iface)
{
    return 1;
}

static HRESULT WINAPI MemInputPin_GetAllocator(IMemInputPin *iface, IMemAllocator **ppAllocator)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI MemInputPin_NotifyAllocator(IMemInputPin *iface,
        IMemAllocator *pAllocator, BOOL bReadOnly)
{
    ALLOCATOR_PROPERTIES ap;
    HRESULT hr;

    CHECK_EXPECT(NotifyAllocator);

    ok(pAllocator != NULL, "pAllocator = %p\n", pAllocator);
    ok(bReadOnly, "bReadOnly = %x\n", bReadOnly);

    hr = IMemAllocator_GetProperties(pAllocator, &ap);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(ap.cBuffers == 32, "cBuffers = %ld\n", ap.cBuffers);
    ok(ap.cbBuffer == 0, "cbBuffer = %ld\n", ap.cbBuffer);
    ok(ap.cbAlign == 1, "cbAlign = %ld\n", ap.cbAlign);
    ok(ap.cbPrefix == 0, "cbPrefix = %ld\n", ap.cbPrefix);
    return S_OK;
}

static HRESULT WINAPI MemInputPin_GetAllocatorRequirements(
        IMemInputPin *iface, ALLOCATOR_PROPERTIES *pProps)
{
    CHECK_EXPECT(GetAllocatorRequirements);
    return E_NOTIMPL;
}

static HRESULT WINAPI MemInputPin_Receive(IMemInputPin *iface, IMediaSample *pSample)
{
    REFERENCE_TIME off, tmp;
    LARGE_INTEGER li;
    BYTE *data;
    HRESULT hr;

    hr = IMediaSample_GetTime(pSample, &off, &tmp);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IMediaSample_GetPointer(pSample, &data);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    li.QuadPart = off;
    IStream_Seek(avi_stream, li, STREAM_SEEK_SET, NULL);
    IStream_Write(avi_stream, data, IMediaSample_GetActualDataLength(pSample), NULL);
    return S_OK;
}

static HRESULT WINAPI MemInputPin_ReceiveMultiple(IMemInputPin *iface,
        IMediaSample **pSamples, LONG nSamples, LONG *nSamplesProcessed)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI MemInputPin_ReceiveCanBlock(IMemInputPin *iface)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static const IMemInputPinVtbl MemInputPinVtbl = {
    MemInputPin_QueryInterface,
    MemInputPin_AddRef,
    MemInputPin_Release,
    MemInputPin_GetAllocator,
    MemInputPin_NotifyAllocator,
    MemInputPin_GetAllocatorRequirements,
    MemInputPin_Receive,
    MemInputPin_ReceiveMultiple,
    MemInputPin_ReceiveCanBlock
};

static HRESULT WINAPI MediaSeeking_QueryInterface(
        IMediaSeeking *iface, REFIID riid, void **ppv)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static ULONG WINAPI MediaSeeking_AddRef(IMediaSeeking *iface)
{
    return 2;
}

static ULONG WINAPI MediaSeeking_Release(IMediaSeeking *iface)
{
    return 1;
}

static HRESULT WINAPI MediaSeeking_GetCapabilities(
        IMediaSeeking *iface, DWORD *pCapabilities)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI MediaSeeking_CheckCapabilities(
        IMediaSeeking *iface, DWORD *pCapabilities)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI MediaSeeking_IsFormatSupported(
        IMediaSeeking *iface, const GUID *pFormat)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI MediaSeeking_QueryPreferredFormat(
        IMediaSeeking *iface, GUID *pFormat)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI MediaSeeking_GetTimeFormat(
        IMediaSeeking *iface, GUID *pFormat)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI MediaSeeking_IsUsingTimeFormat(
        IMediaSeeking *iface, const GUID *pFormat)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI MediaSeeking_SetTimeFormat(
        IMediaSeeking *iface, const GUID *pFormat)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI MediaSeeking_GetDuration(
        IMediaSeeking *iface, LONGLONG *pDuration)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI MediaSeeking_GetStopPosition(
        IMediaSeeking *iface, LONGLONG *pStop)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI MediaSeeking_GetCurrentPosition(
        IMediaSeeking *iface, LONGLONG *pCurrent)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI MediaSeeking_ConvertTimeFormat(IMediaSeeking *iface, LONGLONG *pTarget,
        const GUID *pTargetFormat, LONGLONG Source, const GUID *pSourceFormat)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI MediaSeeking_SetPositions(IMediaSeeking *iface, LONGLONG *pCurrent,
        DWORD dwCurrentFlags, LONGLONG *pStop, DWORD dwStopFlags)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI MediaSeeking_GetPositions(IMediaSeeking *iface,
        LONGLONG *pCurrent, LONGLONG *pStop)
{
    CHECK_EXPECT(MediaSeeking_GetPositions);
    return E_NOTIMPL;
}

static HRESULT WINAPI MediaSeeking_GetAvailable(IMediaSeeking *iface,
        LONGLONG *pEarliest, LONGLONG *pLatest)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI MediaSeeking_SetRate(IMediaSeeking *iface, double dRate)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI MediaSeeking_GetRate(IMediaSeeking *iface, double *pdRate)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI MediaSeeking_GetPreroll(IMediaSeeking *iface, LONGLONG *pllPreroll)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static const IMediaSeekingVtbl MediaSeekingVtbl = {
    MediaSeeking_QueryInterface,
    MediaSeeking_AddRef,
    MediaSeeking_Release,
    MediaSeeking_GetCapabilities,
    MediaSeeking_CheckCapabilities,
    MediaSeeking_IsFormatSupported,
    MediaSeeking_QueryPreferredFormat,
    MediaSeeking_GetTimeFormat,
    MediaSeeking_IsUsingTimeFormat,
    MediaSeeking_SetTimeFormat,
    MediaSeeking_GetDuration,
    MediaSeeking_GetStopPosition,
    MediaSeeking_GetCurrentPosition,
    MediaSeeking_ConvertTimeFormat,
    MediaSeeking_SetPositions,
    MediaSeeking_GetPositions,
    MediaSeeking_GetAvailable,
    MediaSeeking_SetRate,
    MediaSeeking_GetRate,
    MediaSeeking_GetPreroll
};

static test_filter* impl_from_IEnumMediaTypes(IEnumMediaTypes *iface)
{
    return CONTAINING_RECORD(iface, test_filter, IEnumMediaTypes_iface);
}

static HRESULT WINAPI EnumMediaTypes_QueryInterface(IEnumMediaTypes *iface, REFIID riid, void **ppv)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static ULONG WINAPI EnumMediaTypes_AddRef(IEnumMediaTypes *iface)
{
    return 2;
}

static ULONG WINAPI EnumMediaTypes_Release(IEnumMediaTypes *iface)
{
    return 1;
}

static HRESULT WINAPI EnumMediaTypes_Next(IEnumMediaTypes *iface, ULONG cMediaTypes,
        AM_MEDIA_TYPE **ppMediaTypes, ULONG *pcFetched)
{
    test_filter *This = impl_from_IEnumMediaTypes(iface);
    check_calls_list("EnumMediaTypes_Next", ENUMMEDIATYPES_NEXT, This->filter_type);

    ok(cMediaTypes == 1, "cMediaTypes = %ld\n", cMediaTypes);
    ok(ppMediaTypes != NULL, "ppMediaTypes == NULL\n");
    ok(pcFetched != NULL, "pcFetched == NULL\n");

    if(!This->enum_media_types_pos++) {
        ppMediaTypes[0] = CoTaskMemAlloc(sizeof(AM_MEDIA_TYPE));
        memset(ppMediaTypes[0], 0, sizeof(AM_MEDIA_TYPE));
        ppMediaTypes[0]->majortype = MEDIATYPE_Video;
        *pcFetched = 1;
        return S_OK;
    }

    *pcFetched = 0;
    return S_FALSE;
}

static HRESULT WINAPI EnumMediaTypes_Skip(IEnumMediaTypes *iface, ULONG cMediaTypes)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI EnumMediaTypes_Reset(IEnumMediaTypes *iface)
{
    test_filter *This = impl_from_IEnumMediaTypes(iface);
    check_calls_list("EnumMediaTypes_Reset", ENUMMEDIATYPES_RESET, This->filter_type);

    This->enum_media_types_pos = 0;
    return S_OK;
}

static HRESULT WINAPI EnumMediaTypes_Clone(IEnumMediaTypes *iface, IEnumMediaTypes **ppEnum)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static const IEnumMediaTypesVtbl EnumMediaTypesVtbl = {
    EnumMediaTypes_QueryInterface,
    EnumMediaTypes_AddRef,
    EnumMediaTypes_Release,
    EnumMediaTypes_Next,
    EnumMediaTypes_Skip,
    EnumMediaTypes_Reset,
    EnumMediaTypes_Clone
};

static void init_test_filter(test_filter *This, PIN_DIRECTION dir, filter_type type)
{
    memset(This, 0, sizeof(*This));
    This->IBaseFilter_iface.lpVtbl = &BaseFilterVtbl;
    This->IEnumPins_iface.lpVtbl = &EnumPinsVtbl;
    This->IPin_iface.lpVtbl = &PinVtbl;
    This->IKsPropertySet_iface.lpVtbl = &KsPropertySetVtbl;
    This->IMemInputPin_iface.lpVtbl = &MemInputPinVtbl;
    This->IMediaSeeking_iface.lpVtbl = &MediaSeekingVtbl;
    This->IEnumMediaTypes_iface.lpVtbl = &EnumMediaTypesVtbl;

    This->dir = dir;
    This->filter_type = type;
}

static HRESULT WINAPI MemAllocator_QueryInterface(IMemAllocator *iface, REFIID riid, void **ppvObject)
{
    if(IsEqualIID(riid, &IID_IUnknown)) {
        *ppvObject = iface;
        return S_OK;
    }

    ok(0, "unexpected call: %s\n", wine_dbgstr_guid(riid));
    return E_NOTIMPL;
}

static ULONG WINAPI MemAllocator_AddRef(IMemAllocator *iface)
{
    return 2;
}

static ULONG WINAPI MemAllocator_Release(IMemAllocator *iface)
{
    return 1;
}

static HRESULT WINAPI MemAllocator_SetProperties(IMemAllocator *iface,
        ALLOCATOR_PROPERTIES *pRequest, ALLOCATOR_PROPERTIES *pActual)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI MemAllocator_GetProperties(IMemAllocator *iface, ALLOCATOR_PROPERTIES *pProps)
{
    CHECK_EXPECT2(MemAllocator_GetProperties);

    pProps->cBuffers = 1;
    pProps->cbBuffer = 1024;
    pProps->cbAlign = 0;
    pProps->cbPrefix = 0;
    return S_OK;
}

static HRESULT WINAPI MemAllocator_Commit(IMemAllocator *iface)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI MemAllocator_Decommit(IMemAllocator *iface)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI MemAllocator_GetBuffer(IMemAllocator *iface, IMediaSample **ppBuffer,
        REFERENCE_TIME *pStartTime, REFERENCE_TIME *pEndTime, DWORD dwFlags)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI MemAllocator_ReleaseBuffer(IMemAllocator *iface, IMediaSample *pBuffer)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static const IMemAllocatorVtbl MemAllocatorVtbl = {
    MemAllocator_QueryInterface,
    MemAllocator_AddRef,
    MemAllocator_Release,
    MemAllocator_SetProperties,
    MemAllocator_GetProperties,
    MemAllocator_Commit,
    MemAllocator_Decommit,
    MemAllocator_GetBuffer,
    MemAllocator_ReleaseBuffer
};
IMemAllocator MemAllocator = {&MemAllocatorVtbl};

static HRESULT WINAPI MediaSample_QueryInterface(IMediaSample* This, REFIID riid, void **ppv)
{
    if(IsEqualIID(riid, &IID_IMediaSample2))
        CHECK_EXPECT(MediaSample_QueryInterface_MediaSample2);
    else
        ok(0, "MediaSample_QueryInterface: %s\n", wine_dbgstr_guid(riid));

    *ppv = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI MediaSample_AddRef(IMediaSample* This)
{
    return 2;
}

static ULONG WINAPI MediaSample_Release(IMediaSample* This)
{
    return 1;
}

static BYTE buf[1024];
static HRESULT WINAPI MediaSample_GetPointer(IMediaSample* This, BYTE **ppBuffer)
{
    CHECK_EXPECT2(MediaSample_GetPointer);
    *ppBuffer = buf;
    memset(buf, 'z', sizeof(buf));
    return S_OK;
}

static LONG WINAPI MediaSample_GetSize(IMediaSample* This)
{
    CHECK_EXPECT2(MediaSample_GetSize);
    return sizeof(buf);
}

static REFERENCE_TIME start_time, end_time;
static HRESULT WINAPI MediaSample_GetTime(IMediaSample* This,
        REFERENCE_TIME *pTimeStart, REFERENCE_TIME *pTimeEnd)
{
    CHECK_EXPECT2(MediaSample_GetTime);
    *pTimeStart = start_time;
    *pTimeEnd = end_time;
    return S_OK;
}

static HRESULT WINAPI MediaSample_SetTime(IMediaSample* This,
        REFERENCE_TIME *pTimeStart, REFERENCE_TIME *pTimeEnd)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI MediaSample_IsSyncPoint(IMediaSample* This)
{
    CHECK_EXPECT2(MediaSample_IsSyncPoint);
    return S_OK;
}

static HRESULT WINAPI MediaSample_SetSyncPoint(IMediaSample* This, BOOL bIsSyncPoint)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI MediaSample_IsPreroll(IMediaSample* This)
{
    CHECK_EXPECT2(MediaSample_IsPreroll);
    return S_FALSE;
}

static HRESULT WINAPI MediaSample_SetPreroll(IMediaSample* This, BOOL bIsPreroll)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static LONG WINAPI MediaSample_GetActualDataLength(IMediaSample* This)
{
    CHECK_EXPECT2(MediaSample_GetActualDataLength);
    return sizeof(buf);
}

static HRESULT WINAPI MediaSample_SetActualDataLength(IMediaSample* This, LONG length)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI MediaSample_GetMediaType(IMediaSample* This, AM_MEDIA_TYPE **ppMediaType)
{
    CHECK_EXPECT2(MediaSample_GetMediaType);
    *ppMediaType = NULL;
    return S_FALSE;
}

static HRESULT WINAPI MediaSample_SetMediaType(IMediaSample* This, AM_MEDIA_TYPE *pMediaType)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI MediaSample_IsDiscontinuity(IMediaSample* This)
{
    CHECK_EXPECT(MediaSample_IsDiscontinuity);
    return S_FALSE;
}

static HRESULT WINAPI MediaSample_SetDiscontinuity(IMediaSample* This, BOOL bDiscontinuity)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI MediaSample_GetMediaTime(IMediaSample* This,
        LONGLONG *pTimeStart, LONGLONG *pTimeEnd)
{
    CHECK_EXPECT(MediaSample_GetMediaTime);
    return E_NOTIMPL;
}

static HRESULT WINAPI MediaSample_SetMediaTime(IMediaSample* This,
        LONGLONG *pTimeStart, LONGLONG *pTimeEnd)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static const IMediaSampleVtbl MediaSampleVtbl = {
    MediaSample_QueryInterface,
    MediaSample_AddRef,
    MediaSample_Release,
    MediaSample_GetPointer,
    MediaSample_GetSize,
    MediaSample_GetTime,
    MediaSample_SetTime,
    MediaSample_IsSyncPoint,
    MediaSample_SetSyncPoint,
    MediaSample_IsPreroll,
    MediaSample_SetPreroll,
    MediaSample_GetActualDataLength,
    MediaSample_SetActualDataLength,
    MediaSample_GetMediaType,
    MediaSample_SetMediaType,
    MediaSample_IsDiscontinuity,
    MediaSample_SetDiscontinuity,
    MediaSample_GetMediaTime,
    MediaSample_SetMediaTime,
};
IMediaSample MediaSample = {&MediaSampleVtbl};

static void test_AviMux(char *arg)
{
    test_filter source_filter, sink_filter;
    VIDEOINFO videoinfo;
    IPin *avimux_in, *avimux_out, *pin;
    AM_MEDIA_TYPE source_media_type;
    PIN_DIRECTION dir;
    IBaseFilter *avimux;
    IEnumPins *ep;
    IMemInputPin *memin;
    ALLOCATOR_PROPERTIES props;
    IMemAllocator *memalloc;
    IConfigInterleaving *ci;
    FILTER_STATE state;
    HRESULT hr;
    ULONG ref;

    init_test_filter(&source_filter, PINDIR_OUTPUT, SOURCE_FILTER);
    init_test_filter(&sink_filter, PINDIR_INPUT, SINK_FILTER);

    hr = CoCreateInstance(&CLSID_AviDest, NULL, CLSCTX_INPROC_SERVER, &IID_IBaseFilter, (void**)&avimux);
    ok(hr == S_OK || broken(hr == REGDB_E_CLASSNOTREG),
            "Got hr %#lx.\n", hr);
    if(hr != S_OK) {
        win_skip("AVI Mux filter is not registered\n");
        return;
    }

    hr = IBaseFilter_EnumPins(avimux, &ep);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IEnumPins_Next(ep, 1, &avimux_out, NULL);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IPin_QueryDirection(avimux_out, &dir);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(dir == PINDIR_OUTPUT, "dir = %d\n", dir);

    hr = IEnumPins_Next(ep, 1, &avimux_in, NULL);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IPin_QueryDirection(avimux_in, &dir);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(dir == PINDIR_INPUT, "dir = %d\n", dir);
    IEnumPins_Release(ep);

    hr = IPin_ReceiveConnection(avimux_in, &source_filter.IPin_iface, NULL);
    ok(hr == E_POINTER, "Got hr %#lx.\n", hr);

    current_calls_list = NULL;
    memset(&source_media_type, 0, sizeof(AM_MEDIA_TYPE));
    memset(&videoinfo, 0, sizeof(VIDEOINFO));
    source_media_type.majortype = MEDIATYPE_Video;
    source_media_type.subtype = MEDIASUBTYPE_RGB32;
    source_media_type.formattype = FORMAT_VideoInfo;
    source_media_type.bFixedSizeSamples = TRUE;
    source_media_type.lSampleSize = 40000;
    source_media_type.cbFormat = sizeof(VIDEOINFO);
    source_media_type.pbFormat = (BYTE*)&videoinfo;
    videoinfo.AvgTimePerFrame = 333333;
    videoinfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    videoinfo.bmiHeader.biWidth = 100;
    videoinfo.bmiHeader.biHeight = 100;
    videoinfo.bmiHeader.biPlanes = 1;
    videoinfo.bmiHeader.biBitCount = 32;
    videoinfo.bmiHeader.biSizeImage = 40000;
    videoinfo.bmiHeader.biClrImportant = 256;
    hr = IPin_ReceiveConnection(avimux_in, &source_filter.IPin_iface, &source_media_type);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IPin_ConnectedTo(avimux_in, &pin);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(pin == &source_filter.IPin_iface, "incorrect pin: %p, expected %p\n",
            pin, &source_filter.IPin_iface);

    hr = IPin_Connect(avimux_out, &source_filter.IPin_iface, NULL);
    ok(hr == VFW_E_INVALID_DIRECTION, "Got hr %#lx.\n", hr);

    hr = IBaseFilter_JoinFilterGraph(avimux, (IFilterGraph*)&GraphBuilder, NULL);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    SET_EXPECT(ReceiveConnection);
    SET_EXPECT(GetAllocatorRequirements);
    SET_EXPECT(NotifyAllocator);
    SET_EXPECT(Reconnect);
    hr = IPin_Connect(avimux_out, &sink_filter.IPin_iface, NULL);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    CHECK_CALLED(ReceiveConnection);
    CHECK_CALLED(GetAllocatorRequirements);
    CHECK_CALLED(NotifyAllocator);
    CHECK_CALLED(Reconnect);

    hr = IPin_ConnectedTo(avimux_out, &pin);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(pin == &sink_filter.IPin_iface, "incorrect pin: %p, expected %p\n",
            pin, &source_filter.IPin_iface);

    hr = IPin_QueryInterface(avimux_in, &IID_IMemInputPin, (void**)&memin);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    props.cBuffers = 0xdeadbee1;
    props.cbBuffer = 0xdeadbee2;
    props.cbAlign = 0xdeadbee3;
    props.cbPrefix = 0xdeadbee4;
    hr = IMemInputPin_GetAllocatorRequirements(memin, &props);
    ok(hr==S_OK || broken(hr==E_INVALIDARG), "Got hr %#lx.\n", hr);
    if(hr == S_OK) {
        ok(props.cBuffers == 0xdeadbee1, "cBuffers = %ld\n", props.cBuffers);
        ok(props.cbBuffer == 0xdeadbee2, "cbBuffer = %ld\n", props.cbBuffer);
        ok(props.cbAlign == 1, "cbAlign = %ld\n", props.cbAlign);
        ok(props.cbPrefix == 8, "cbPrefix = %ld\n", props.cbPrefix);
    }

    hr = IMemInputPin_GetAllocator(memin, &memalloc);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    props.cBuffers = 0xdeadbee1;
    props.cbBuffer = 0xdeadbee2;
    props.cbAlign = 0xdeadbee3;
    props.cbPrefix = 0xdeadbee4;
    hr = IMemAllocator_GetProperties(memalloc, &props);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(props.cBuffers == 0, "cBuffers = %ld\n", props.cBuffers);
    ok(props.cbBuffer == 0, "cbBuffer = %ld\n", props.cbBuffer);
    ok(props.cbAlign == 0, "cbAlign = %ld\n", props.cbAlign);
    ok(props.cbPrefix == 0, "cbPrefix = %ld\n", props.cbPrefix);
    IMemAllocator_Release(memalloc);

    hr = IBaseFilter_QueryInterface(avimux, &IID_IConfigInterleaving, (void**)&ci);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IConfigInterleaving_put_Mode(ci, 5);
    ok(hr == E_INVALIDARG, "Got hr %#lx.\n", hr);
    SET_EXPECT(Reconnect);
    hr = IConfigInterleaving_put_Mode(ci, INTERLEAVE_FULL);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    CHECK_CALLED(Reconnect);
    IConfigInterleaving_Release(ci);

    hr = IBaseFilter_GetState(avimux, 0, &state);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(state == State_Stopped, "state = %d\n", state);

    SET_EXPECT(MemAllocator_GetProperties);
    hr = IMemInputPin_NotifyAllocator(memin, &MemAllocator, TRUE);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    CHECK_CALLED(MemAllocator_GetProperties);

    hr = IMemInputPin_GetAllocator(memin, &memalloc);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(memalloc != &MemAllocator, "memalloc == &MemAllocator\n");
    IMemAllocator_Release(memalloc);

    hr = CreateStreamOnHGlobal(NULL, TRUE, &avi_stream);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    SET_EXPECT(MediaSeeking_GetPositions);
    SET_EXPECT(MemInputPin_QueryInterface_IStream);
    hr = IBaseFilter_Run(avimux, 0);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    CHECK_CALLED(MediaSeeking_GetPositions);

    hr = IBaseFilter_GetState(avimux, 0, &state);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(state == State_Running, "state = %d\n", state);

    SET_EXPECT(MediaSample_QueryInterface_MediaSample2);
    SET_EXPECT(MediaSample_IsDiscontinuity);
    SET_EXPECT(MediaSample_IsPreroll);
    SET_EXPECT(MediaSample_IsSyncPoint);
    SET_EXPECT(MediaSample_GetTime);
    SET_EXPECT(MediaSample_GetMediaType);
    SET_EXPECT(MediaSample_GetPointer);
    SET_EXPECT(MediaSample_GetActualDataLength);
    SET_EXPECT(MediaSample_GetSize);
    SET_EXPECT(MediaSample_GetMediaTime);
    start_time = end_time = 0;
    hr = IMemInputPin_Receive(memin, &MediaSample);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    CHECK_CALLED(MediaSample_QueryInterface_MediaSample2);
    todo_wine CHECK_CALLED(MediaSample_IsDiscontinuity);
    todo_wine CHECK_CALLED(MediaSample_IsPreroll);
    CHECK_CALLED(MediaSample_IsSyncPoint);
    CHECK_CALLED(MediaSample_GetTime);
    todo_wine CHECK_CALLED(MediaSample_GetMediaType);
    CHECK_CALLED(MediaSample_GetPointer);
    CHECK_CALLED(MediaSample_GetActualDataLength);
    todo_wine CHECK_CALLED(MediaSample_GetSize);
    todo_wine CHECK_CALLED(MediaSample_GetMediaTime);

    SET_EXPECT(MediaSample_QueryInterface_MediaSample2);
    SET_EXPECT(MediaSample_IsDiscontinuity);
    SET_EXPECT(MediaSample_IsPreroll);
    SET_EXPECT(MediaSample_IsSyncPoint);
    SET_EXPECT(MediaSample_GetTime);
    SET_EXPECT(MediaSample_GetMediaType);
    SET_EXPECT(MediaSample_GetPointer);
    SET_EXPECT(MediaSample_GetActualDataLength);
    SET_EXPECT(MediaSample_GetSize);
    SET_EXPECT(MediaSample_GetMediaTime);
    hr = IMemInputPin_Receive(memin, &MediaSample);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    CHECK_CALLED(MediaSample_QueryInterface_MediaSample2);
    todo_wine CHECK_CALLED(MediaSample_IsDiscontinuity);
    todo_wine CHECK_CALLED(MediaSample_IsPreroll);
    CHECK_CALLED(MediaSample_IsSyncPoint);
    CHECK_CALLED(MediaSample_GetTime);
    todo_wine CHECK_CALLED(MediaSample_GetMediaType);
    CHECK_CALLED(MediaSample_GetPointer);
    CHECK_CALLED(MediaSample_GetActualDataLength);
    todo_wine CHECK_CALLED(MediaSample_GetSize);
    todo_wine CHECK_CALLED(MediaSample_GetMediaTime);

    SET_EXPECT(MediaSample_QueryInterface_MediaSample2);
    SET_EXPECT(MediaSample_IsDiscontinuity);
    SET_EXPECT(MediaSample_IsPreroll);
    SET_EXPECT(MediaSample_IsSyncPoint);
    SET_EXPECT(MediaSample_GetTime);
    SET_EXPECT(MediaSample_GetMediaType);
    SET_EXPECT(MediaSample_GetPointer);
    SET_EXPECT(MediaSample_GetActualDataLength);
    SET_EXPECT(MediaSample_GetSize);
    SET_EXPECT(MediaSample_GetMediaTime);
    start_time = 20000000;
    end_time = 21000000;
    hr = IMemInputPin_Receive(memin, &MediaSample);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    CHECK_CALLED(MediaSample_QueryInterface_MediaSample2);
    todo_wine CHECK_CALLED(MediaSample_IsDiscontinuity);
    todo_wine CHECK_CALLED(MediaSample_IsPreroll);
    CHECK_CALLED(MediaSample_IsSyncPoint);
    CHECK_CALLED(MediaSample_GetTime);
    todo_wine CHECK_CALLED(MediaSample_GetMediaType);
    CHECK_CALLED(MediaSample_GetPointer);
    CHECK_CALLED(MediaSample_GetActualDataLength);
    todo_wine CHECK_CALLED(MediaSample_GetSize);
    todo_wine CHECK_CALLED(MediaSample_GetMediaTime);
    IMemInputPin_Release(memin);

    hr = IBaseFilter_Stop(avimux);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    CHECK_CALLED(MemInputPin_QueryInterface_IStream);

    hr = IBaseFilter_GetState(avimux, 0, &state);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(state == State_Stopped, "state = %d\n", state);

    hr = IPin_Disconnect(avimux_out);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    IPin_Release(avimux_in);
    IPin_Release(avimux_out);
    ref = IBaseFilter_Release(avimux);
    ok(ref == 0, "Avi Mux filter was not destroyed (%ld)\n", ref);

    if(arg && !strcmp(arg, "save")) {
        LARGE_INTEGER li;
        char buf[1024];
        ULONG read;
        HANDLE *f;

        f = CreateFileA("avimux.avi", GENERIC_WRITE, 0, NULL,
                CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        ok(f != INVALID_HANDLE_VALUE, "CreateFile failed\n");

        li.QuadPart = 0;
        hr = IStream_Seek(avi_stream, li, STREAM_SEEK_SET, NULL);
        ok(hr == S_OK, "Got hr %#lx.\n", hr);

        while(1) {
            hr = IStream_Read(avi_stream, buf, sizeof(buf), &read);
            if(FAILED(hr)) {
                ok(0, "Got hr %#lx.\n", hr);
                break;
            }
            if(!read)
                break;
            ok(WriteFile(f, buf, read, &read, NULL), "WriteFile failed\n");
            if(hr == S_FALSE)
                break;
        }
        CloseHandle(f);
    }

    ref = IStream_Release(avi_stream);
    ok(ref == 0, "IStream was not destroyed (%ld)\n", ref);
}

START_TEST(qcap)
{
    if (SUCCEEDED(CoInitialize(NULL)))
    {
        int arg_c;
        char **arg_v;

        arg_c = winetest_get_mainargs(&arg_v);

        test_AviMux(arg_c>2 ? arg_v[2] : NULL);

        CoUninitialize();
    }
    else
        skip("CoInitialize failed\n");
}
