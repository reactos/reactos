/*
 * Header file for Wine's strmbase implementation
 *
 * Copyright 2003 Robert Shearman
 * Copyright 2010 Aric Stewart, CodeWeavers
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

#include "dshow.h"
#include "wine/debug.h"

HRESULT WINAPI CopyMediaType(AM_MEDIA_TYPE * pDest, const AM_MEDIA_TYPE *pSrc);
void WINAPI FreeMediaType(AM_MEDIA_TYPE * pMediaType);
AM_MEDIA_TYPE * WINAPI CreateMediaType(AM_MEDIA_TYPE const * pSrc);
void WINAPI DeleteMediaType(AM_MEDIA_TYPE * pMediaType);

void strmbase_dump_media_type(const AM_MEDIA_TYPE *mt);

static inline const char *debugstr_time(REFERENCE_TIME time)
{
    ULONGLONG abstime = time >= 0 ? time : -time;
    unsigned int i = 0, j = 0;
    char buffer[23], rev[23];

    while (abstime || i <= 8)
    {
        buffer[i++] = '0' + (abstime % 10);
        abstime /= 10;
        if (i == 7) buffer[i++] = '.';
    }
    if (time < 0) buffer[i++] = '-';

    while (i--) rev[j++] = buffer[i];
    while (rev[j-1] == '0' && rev[j-2] != '.') --j;
    rev[j] = 0;

    return wine_dbg_sprintf("%s", rev);
}

/* Pin functions */

struct strmbase_pin
{
    IPin IPin_iface;
    struct strmbase_filter *filter;
    PIN_DIRECTION dir;
    WCHAR name[128];
    WCHAR id[128];
    IPin *peer;
    AM_MEDIA_TYPE mt;

    const struct strmbase_pin_ops *ops;
};

struct strmbase_pin_ops
{
    /* Required for QueryAccept(), Connect(), ReceiveConnection(). */
    HRESULT (*pin_query_accept)(struct strmbase_pin *pin, const AM_MEDIA_TYPE *mt);
    /* Required for EnumMediaTypes(). */
    HRESULT (*pin_get_media_type)(struct strmbase_pin *pin, unsigned int index, AM_MEDIA_TYPE *mt);
    HRESULT (*pin_query_interface)(struct strmbase_pin *pin, REFIID iid, void **out);
};

struct strmbase_source
{
    struct strmbase_pin pin;
    IMemInputPin *pMemInputPin;
    IMemAllocator *pAllocator;

    const struct strmbase_source_ops *pFuncsTable;
};

typedef HRESULT (WINAPI *BaseOutputPin_AttemptConnection)(struct strmbase_source *pin, IPin *peer, const AM_MEDIA_TYPE *mt);
typedef HRESULT (WINAPI *BaseOutputPin_DecideBufferSize)(struct strmbase_source *pin, IMemAllocator *allocator, ALLOCATOR_PROPERTIES *props);
typedef HRESULT (WINAPI *BaseOutputPin_DecideAllocator)(struct strmbase_source *pin, IMemInputPin *peer, IMemAllocator **allocator);

struct strmbase_source_ops
{
    struct strmbase_pin_ops base;

    /* Required for Connect(). */
    BaseOutputPin_AttemptConnection pfnAttemptConnection;
    /* Required for BaseOutputPinImpl_DecideAllocator */
    BaseOutputPin_DecideBufferSize pfnDecideBufferSize;
    /* Required for BaseOutputPinImpl_AttemptConnection */
    BaseOutputPin_DecideAllocator pfnDecideAllocator;

    void (*source_disconnect)(struct strmbase_source *pin);
};

struct strmbase_sink
{
    struct strmbase_pin pin;

    IMemInputPin IMemInputPin_iface;
    IMemAllocator *pAllocator;
    BOOL flushing;
    IMemAllocator *preferred_allocator;

    const struct strmbase_sink_ops *pFuncsTable;
};

typedef HRESULT (WINAPI *BaseInputPin_Receive)(struct strmbase_sink *This, IMediaSample *pSample);

struct strmbase_sink_ops
{
    struct strmbase_pin_ops base;
    BaseInputPin_Receive pfnReceive;
    HRESULT (*sink_connect)(struct strmbase_sink *pin, IPin *peer, const AM_MEDIA_TYPE *mt);
    void (*sink_disconnect)(struct strmbase_sink *pin);
    HRESULT (*sink_eos)(struct strmbase_sink *pin);
    HRESULT (*sink_begin_flush)(struct strmbase_sink *pin);
    HRESULT (*sink_end_flush)(struct strmbase_sink *pin);
    HRESULT (*sink_new_segment)(struct strmbase_sink *pin, REFERENCE_TIME start, REFERENCE_TIME stop, double rate);
};

/* Base Pin */
HRESULT WINAPI BaseOutputPinImpl_DecideAllocator(struct strmbase_source *pin, IMemInputPin *peer, IMemAllocator **allocator);
HRESULT WINAPI BaseOutputPinImpl_AttemptConnection(struct strmbase_source *pin, IPin *peer, const AM_MEDIA_TYPE *mt);

void strmbase_source_cleanup(struct strmbase_source *pin);
void strmbase_source_init(struct strmbase_source *pin, struct strmbase_filter *filter,
        const WCHAR *name, const struct strmbase_source_ops *func_table);

void strmbase_sink_init(struct strmbase_sink *pin, struct strmbase_filter *filter,
        const WCHAR *name, const struct strmbase_sink_ops *ops, IMemAllocator *allocator);
void strmbase_sink_cleanup(struct strmbase_sink *pin);

struct strmbase_filter
{
    IBaseFilter IBaseFilter_iface;
    IUnknown IUnknown_inner;
    IUnknown *outer_unk;
    LONG refcount;
    CRITICAL_SECTION filter_cs;
    CRITICAL_SECTION stream_cs;

    FILTER_STATE state;
    IReferenceClock *clock;
    WCHAR name[128];
    IFilterGraph *graph;
    CLSID clsid;
    LONG pin_version;

    const struct strmbase_filter_ops *ops;
};

struct strmbase_filter_ops
{
    struct strmbase_pin *(*filter_get_pin)(struct strmbase_filter *iface, unsigned int index);
    void (*filter_destroy)(struct strmbase_filter *iface);
    HRESULT (*filter_query_interface)(struct strmbase_filter *iface, REFIID iid, void **out);

    HRESULT (*filter_init_stream)(struct strmbase_filter *iface);
    HRESULT (*filter_start_stream)(struct strmbase_filter *iface, REFERENCE_TIME time);
    HRESULT (*filter_stop_stream)(struct strmbase_filter *iface);
    HRESULT (*filter_cleanup_stream)(struct strmbase_filter *iface);
    HRESULT (*filter_wait_state)(struct strmbase_filter *iface, DWORD timeout);
};

VOID WINAPI BaseFilterImpl_IncrementPinVersion(struct strmbase_filter *filter);

void strmbase_filter_init(struct strmbase_filter *filter, IUnknown *outer,
        const CLSID *clsid, const struct strmbase_filter_ops *func_table);
void strmbase_filter_cleanup(struct strmbase_filter *filter);

/* Source Seeking */
typedef HRESULT (WINAPI *SourceSeeking_ChangeRate)(IMediaSeeking *iface);
typedef HRESULT (WINAPI *SourceSeeking_ChangeStart)(IMediaSeeking *iface);
typedef HRESULT (WINAPI *SourceSeeking_ChangeStop)(IMediaSeeking *iface);

typedef struct SourceSeeking
{
	IMediaSeeking IMediaSeeking_iface;

	ULONG refCount;
	SourceSeeking_ChangeStop fnChangeStop;
	SourceSeeking_ChangeStart fnChangeStart;
	SourceSeeking_ChangeRate fnChangeRate;
	DWORD dwCapabilities;
	double dRate;
	LONGLONG llCurrent, llStop, llDuration;
	GUID timeformat;
	CRITICAL_SECTION cs;
} SourceSeeking;

HRESULT strmbase_seeking_init(SourceSeeking *seeking, const IMediaSeekingVtbl *vtbl,
        SourceSeeking_ChangeStop fnChangeStop, SourceSeeking_ChangeStart fnChangeStart,
        SourceSeeking_ChangeRate fnChangeRate);
void strmbase_seeking_cleanup(SourceSeeking *seeking);

HRESULT WINAPI SourceSeekingImpl_GetCapabilities(IMediaSeeking * iface, DWORD * pCapabilities);
HRESULT WINAPI SourceSeekingImpl_CheckCapabilities(IMediaSeeking * iface, DWORD * pCapabilities);
HRESULT WINAPI SourceSeekingImpl_IsFormatSupported(IMediaSeeking * iface, const GUID * pFormat);
HRESULT WINAPI SourceSeekingImpl_QueryPreferredFormat(IMediaSeeking * iface, GUID * pFormat);
HRESULT WINAPI SourceSeekingImpl_GetTimeFormat(IMediaSeeking * iface, GUID * pFormat);
HRESULT WINAPI SourceSeekingImpl_IsUsingTimeFormat(IMediaSeeking * iface, const GUID * pFormat);
HRESULT WINAPI SourceSeekingImpl_SetTimeFormat(IMediaSeeking * iface, const GUID * pFormat);
HRESULT WINAPI SourceSeekingImpl_GetDuration(IMediaSeeking * iface, LONGLONG * pDuration);
HRESULT WINAPI SourceSeekingImpl_GetStopPosition(IMediaSeeking * iface, LONGLONG * pStop);
HRESULT WINAPI SourceSeekingImpl_GetCurrentPosition(IMediaSeeking * iface, LONGLONG * pCurrent);
HRESULT WINAPI SourceSeekingImpl_ConvertTimeFormat(IMediaSeeking * iface, LONGLONG * pTarget, const GUID * pTargetFormat, LONGLONG Source, const GUID * pSourceFormat);
HRESULT WINAPI SourceSeekingImpl_SetPositions(IMediaSeeking * iface, LONGLONG * pCurrent, DWORD dwCurrentFlags, LONGLONG * pStop, DWORD dwStopFlags);
HRESULT WINAPI SourceSeekingImpl_GetPositions(IMediaSeeking * iface, LONGLONG * pCurrent, LONGLONG * pStop);
HRESULT WINAPI SourceSeekingImpl_GetAvailable(IMediaSeeking * iface, LONGLONG * pEarliest, LONGLONG * pLatest);
HRESULT WINAPI SourceSeekingImpl_SetRate(IMediaSeeking * iface, double dRate);
HRESULT WINAPI SourceSeekingImpl_GetRate(IMediaSeeking * iface, double * dRate);
HRESULT WINAPI SourceSeekingImpl_GetPreroll(IMediaSeeking * iface, LONGLONG * pPreroll);

enum strmbase_type_id
{
    IBasicAudio_tid,
    IBasicVideo_tid,
    IMediaControl_tid,
    IMediaEvent_tid,
    IMediaPosition_tid,
    IVideoWindow_tid,
    last_tid
};

HRESULT strmbase_get_typeinfo(enum strmbase_type_id tid, ITypeInfo **typeinfo);
void strmbase_release_typelibs(void);

struct strmbase_passthrough
{
    ISeekingPassThru ISeekingPassThru_iface;
    IMediaSeeking IMediaSeeking_iface;
    IMediaPosition IMediaPosition_iface;

    IUnknown *outer_unk;
    IPin *pin;
    BOOL renderer;
    BOOL timevalid;
    CRITICAL_SECTION time_cs;
    REFERENCE_TIME time_earliest;
};

void strmbase_passthrough_init(struct strmbase_passthrough *passthrough, IUnknown *outer);
void strmbase_passthrough_cleanup(struct strmbase_passthrough *passthrough);

void strmbase_passthrough_eos(struct strmbase_passthrough *passthrough);
void strmbase_passthrough_invalidate_time(struct strmbase_passthrough *passthrough);
void strmbase_passthrough_update_time(struct strmbase_passthrough *passthrough, REFERENCE_TIME time);

struct strmbase_renderer
{
    struct strmbase_filter filter;
    struct strmbase_passthrough passthrough;
    IQualityControl IQualityControl_iface;

    struct strmbase_sink sink;

    /* Signaled when the filter has completed a state change. The filter waits
     * for this event in IBaseFilter::GetState(). */
    HANDLE state_event;
    /* Signaled when the sample presentation time occurs. The streaming thread
     * waits for this event in Receive() if applicable. */
    HANDLE advise_event;
    /* Signaled when the filter is running. The streaming thread waits for this
     * event in Receive() while paused. */
    HANDLE run_event;
    /* Signaled when a flush or state change occurs, i.e. anything that needs
     * to immediately unblock the streaming thread. */
    HANDLE flush_event;
    REFERENCE_TIME stream_start;

    IMediaSample *current_sample;

    IQualityControl *qc_sink;
    REFERENCE_TIME last_left, avg_duration, avg_pt;
    double avg_rate;

    const struct strmbase_renderer_ops *ops;

    BOOL eos;
};

struct strmbase_renderer_ops
{
    HRESULT (*renderer_query_accept)(struct strmbase_renderer *iface, const AM_MEDIA_TYPE *mt);
    HRESULT (*renderer_render)(struct strmbase_renderer *iface, IMediaSample *sample);
    void (*renderer_init_stream)(struct strmbase_renderer *iface);
    void (*renderer_start_stream)(struct strmbase_renderer *iface);
    void (*renderer_stop_stream)(struct strmbase_renderer *iface);
    HRESULT (*renderer_connect)(struct strmbase_renderer *iface, const AM_MEDIA_TYPE *mt);
    void (*renderer_disconnect)(struct strmbase_renderer *iface);
    void (*renderer_destroy)(struct strmbase_renderer *iface);
    HRESULT (*renderer_query_interface)(struct strmbase_renderer *iface, REFIID iid, void **out);
    HRESULT (*renderer_pin_query_interface)(struct strmbase_renderer *iface, REFIID iid, void **out);
};

void strmbase_renderer_init(struct strmbase_renderer *filter, IUnknown *outer,
        const CLSID *clsid, const WCHAR *sink_name, const struct strmbase_renderer_ops *ops);
void strmbase_renderer_cleanup(struct strmbase_renderer *filter);
