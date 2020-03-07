/*
 * Copyright (C) 2013 Piotr Caban for CodeWeavers
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
#include <stdio.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "wtypes.h"
#include "dshow.h"
#include "vfw.h"
#include "aviriff.h"

#include "qcap_main.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(qcap);

#define MAX_PIN_NO 128
#define AVISUPERINDEX_ENTRIES 2000
#define AVISTDINDEX_ENTRIES 4000
#define ALIGN(x) ((x+1)/2*2)

typedef struct {
    BaseOutputPin pin;
    IQualityControl IQualityControl_iface;

    int cur_stream;
    LONGLONG cur_time;

    int buf_pos;
    BYTE buf[65536];

    int movi_off;
    int out_pos;
    int size;
    IStream *stream;
} AviMuxOut;

typedef struct {
    BaseInputPin pin;
    IAMStreamControl IAMStreamControl_iface;
    IPropertyBag IPropertyBag_iface;
    IQualityControl IQualityControl_iface;

    REFERENCE_TIME avg_time_per_frame;
    REFERENCE_TIME stop;
    int stream_id;
    LONGLONG stream_time;

    /* strl chunk */
    AVISTREAMHEADER strh;
    struct {
        FOURCC fcc;
        DWORD cb;
        BYTE data[1];
    } *strf;
    AVISUPERINDEX *indx;
#ifdef __REACTOS__
    BYTE indx_data[FIELD_OFFSET(AVISUPERINDEX, aIndex) + AVISUPERINDEX_ENTRIES * sizeof(struct _avisuperindex_entry)];
#else
    BYTE indx_data[FIELD_OFFSET(AVISUPERINDEX, aIndex[AVISUPERINDEX_ENTRIES])];
#endif

    /* movi chunk */
    int ix_off;
    AVISTDINDEX *ix;
#ifdef __REACTOS__
    BYTE ix_data[FIELD_OFFSET(AVISTDINDEX, aIndex) + AVISTDINDEX_ENTRIES * sizeof(struct _avisuperindex_entry)];
#else
    BYTE ix_data[FIELD_OFFSET(AVISTDINDEX, aIndex[AVISTDINDEX_ENTRIES])];
#endif

    IMediaSample *samples_head;
    IMemAllocator *samples_allocator;
} AviMuxIn;

typedef struct {
    BaseFilter filter;
    IConfigAviMux IConfigAviMux_iface;
    IConfigInterleaving IConfigInterleaving_iface;
    IMediaSeeking IMediaSeeking_iface;
    IPersistMediaPropertyBag IPersistMediaPropertyBag_iface;
    ISpecifyPropertyPages ISpecifyPropertyPages_iface;

    InterleavingMode mode;
    REFERENCE_TIME interleave;
    REFERENCE_TIME preroll;

    AviMuxOut *out;
    int input_pin_no;
    AviMuxIn *in[MAX_PIN_NO-1];

    REFERENCE_TIME start, stop;
    AVIMAINHEADER avih;

    int idx1_entries;
    int idx1_size;
    AVIINDEXENTRY *idx1;
} AviMux;

static HRESULT create_input_pin(AviMux*);

static inline AviMux* impl_from_BaseFilter(BaseFilter *filter)
{
    return CONTAINING_RECORD(filter, AviMux, filter);
}

static IPin* WINAPI AviMux_GetPin(BaseFilter *iface, int pos)
{
    AviMux *This = impl_from_BaseFilter(iface);

    TRACE("(%p)->(%d)\n", This, pos);

    if(pos == 0) {
        IPin_AddRef(&This->out->pin.pin.IPin_iface);
        return &This->out->pin.pin.IPin_iface;
    }else if(pos>0 && pos<=This->input_pin_no) {
        IPin_AddRef(&This->in[pos-1]->pin.pin.IPin_iface);
        return &This->in[pos-1]->pin.pin.IPin_iface;
    }

    return NULL;
}

static LONG WINAPI AviMux_GetPinCount(BaseFilter *iface)
{
    AviMux *This = impl_from_BaseFilter(iface);
    TRACE("(%p)\n", This);
    return This->input_pin_no+1;
}

static const BaseFilterFuncTable filter_func_table = {
    AviMux_GetPin,
    AviMux_GetPinCount
};

static inline AviMux* impl_from_IBaseFilter(IBaseFilter *iface)
{
    BaseFilter *filter = CONTAINING_RECORD(iface, BaseFilter, IBaseFilter_iface);
    return impl_from_BaseFilter(filter);
}

static HRESULT WINAPI AviMux_QueryInterface(IBaseFilter *iface, REFIID riid, void **ppv)
{
    AviMux *This = impl_from_IBaseFilter(iface);

    TRACE("(%p)->(%s %p)\n", This, debugstr_guid(riid), ppv);

    if(IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_IPersist) ||
            IsEqualIID(riid, &IID_IMediaFilter) || IsEqualIID(riid, &IID_IBaseFilter))
        *ppv = &This->filter.IBaseFilter_iface;
    else if(IsEqualIID(riid, &IID_IConfigAviMux))
        *ppv = &This->IConfigAviMux_iface;
    else if(IsEqualIID(riid, &IID_IConfigInterleaving))
        *ppv = &This->IConfigInterleaving_iface;
    else if(IsEqualIID(riid, &IID_IMediaSeeking))
        *ppv = &This->IMediaSeeking_iface;
    else if(IsEqualIID(riid, &IID_IPersistMediaPropertyBag))
        *ppv = &This->IPersistMediaPropertyBag_iface;
    else if(IsEqualIID(riid, &IID_ISpecifyPropertyPages))
        *ppv = &This->ISpecifyPropertyPages_iface;
    else {
        FIXME("no interface for %s\n", debugstr_guid(riid));
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI AviMux_Release(IBaseFilter *iface)
{
    AviMux *This = impl_from_IBaseFilter(iface);
    ULONG ref = BaseFilterImpl_Release(iface);

    TRACE("(%p) new refcount: %u\n", This, ref);

    if(!ref) {
        int i;

        BaseOutputPinImpl_Release(&This->out->pin.pin.IPin_iface);

        for(i=0; i<This->input_pin_no; i++) {
            IPin_Disconnect(&This->in[i]->pin.pin.IPin_iface);
            IMemAllocator_Release(This->in[i]->samples_allocator);
            This->in[i]->samples_allocator = NULL;
            BaseInputPinImpl_Release(&This->in[i]->pin.pin.IPin_iface);
        }

        HeapFree(GetProcessHeap(), 0, This->idx1);
        HeapFree(GetProcessHeap(), 0, This);
        ObjectRefCount(FALSE);
    }
    return ref;
}

static HRESULT out_flush(AviMux *This)
{
    ULONG written;
    HRESULT hr;

    if(!This->out->buf_pos)
        return S_OK;

    hr = IStream_Write(This->out->stream, This->out->buf, This->out->buf_pos, &written);
    if(FAILED(hr))
        return hr;
    if(written != This->out->buf_pos)
        return E_FAIL;

    This->out->buf_pos = 0;
    return S_OK;
}

static HRESULT out_seek(AviMux *This, int pos)
{
    LARGE_INTEGER li;
    HRESULT hr;

    hr = out_flush(This);
    if(FAILED(hr))
        return hr;

    li.QuadPart = pos;
    hr = IStream_Seek(This->out->stream, li, STREAM_SEEK_SET, NULL);
    if(FAILED(hr))
        return hr;

    This->out->out_pos = pos;
    if(This->out->out_pos > This->out->size)
        This->out->size = This->out->out_pos;
    return hr;
}

static HRESULT out_write(AviMux *This, const void *data, int size)
{
    int chunk_size;
    HRESULT hr;

    while(1) {
        if(size > sizeof(This->out->buf)-This->out->buf_pos)
            chunk_size = sizeof(This->out->buf)-This->out->buf_pos;
        else
            chunk_size = size;

        memcpy(This->out->buf + This->out->buf_pos, data, chunk_size);
        size -= chunk_size;
        data = (const BYTE*)data + chunk_size;
        This->out->buf_pos += chunk_size;
        This->out->out_pos += chunk_size;
        if(This->out->out_pos > This->out->size)
            This->out->size = This->out->out_pos;

        if(!size)
            break;
        hr = out_flush(This);
        if(FAILED(hr))
            return hr;
    }

    return S_OK;
}

static inline HRESULT idx1_add_entry(AviMux *avimux, DWORD ckid, DWORD flags, DWORD off, DWORD len)
{
    if(avimux->idx1_entries == avimux->idx1_size) {
        AVIINDEXENTRY *new_idx = HeapReAlloc(GetProcessHeap(), 0, avimux->idx1,
                sizeof(*avimux->idx1)*2*avimux->idx1_size);
        if(!new_idx)
            return E_OUTOFMEMORY;

        avimux->idx1_size *= 2;
        avimux->idx1 = new_idx;
    }

    avimux->idx1[avimux->idx1_entries].ckid = ckid;
    avimux->idx1[avimux->idx1_entries].dwFlags = flags;
    avimux->idx1[avimux->idx1_entries].dwChunkOffset = off;
    avimux->idx1[avimux->idx1_entries].dwChunkLength = len;
    avimux->idx1_entries++;
    return S_OK;
}

static HRESULT flush_queue(AviMux *avimux, AviMuxIn *avimuxin, BOOL closing)
{
    IMediaSample *sample, **prev, **head_prev;
    BYTE *data;
    RIFFCHUNK rf;
    DWORD size;
    DWORD flags;
    HRESULT hr;

    if(avimux->out->cur_stream != avimuxin->stream_id)
        return S_OK;

    while(avimuxin->samples_head) {
        hr = IMediaSample_GetPointer(avimuxin->samples_head, (BYTE**)&head_prev);
        if(FAILED(hr))
            return hr;
        head_prev--;

        hr = IMediaSample_GetPointer(*head_prev, (BYTE**)&prev);
        if(FAILED(hr))
            return hr;
        prev--;

        sample = *head_prev;
        size = IMediaSample_GetActualDataLength(sample);
        hr = IMediaSample_GetPointer(sample, &data);
        if(FAILED(hr))
            return hr;
        flags = IMediaSample_IsDiscontinuity(sample)==S_OK ? AM_SAMPLE_TIMEDISCONTINUITY : 0;
        if(IMediaSample_IsSyncPoint(sample) == S_OK)
            flags |= AM_SAMPLE_SPLICEPOINT;

        if(avimuxin->stream_time + (closing ? 0 : avimuxin->strh.dwScale) > avimux->out->cur_time &&
                !(flags & AM_SAMPLE_TIMEDISCONTINUITY)) {
            if(closing)
                break;

            avimux->out->cur_stream++;
            if(avimux->out->cur_stream >= avimux->input_pin_no-1) {
                avimux->out->cur_time += avimux->interleave;
                avimux->out->cur_stream = 0;
            }
            avimuxin = avimux->in[avimux->out->cur_stream];
            continue;
        }

        if(avimuxin->ix->nEntriesInUse == AVISTDINDEX_ENTRIES) {
            /* TODO: use output pins Deliver/Receive method */
            hr = out_seek(avimux, avimuxin->ix_off);
            if(FAILED(hr))
                return hr;
            hr = out_write(avimux, avimuxin->ix, sizeof(avimuxin->ix_data));
            if(FAILED(hr))
                return hr;

            avimuxin->indx->aIndex[avimuxin->indx->nEntriesInUse].qwOffset = avimuxin->ix_off;
            avimuxin->indx->aIndex[avimuxin->indx->nEntriesInUse].dwSize = sizeof(avimuxin->ix_data);
            avimuxin->indx->aIndex[avimuxin->indx->nEntriesInUse].dwDuration = AVISTDINDEX_ENTRIES;
            avimuxin->indx->nEntriesInUse++;

            memset(avimuxin->ix->aIndex, 0, sizeof(avimuxin->ix->aIndex)*avimuxin->ix->nEntriesInUse);
            avimuxin->ix->nEntriesInUse = 0;
            avimuxin->ix->qwBaseOffset = 0;

            avimuxin->ix_off = avimux->out->size;
            avimux->out->size += sizeof(avimuxin->ix_data);
        }

        if(*head_prev == avimuxin->samples_head)
            avimuxin->samples_head = NULL;
        else
            *head_prev = *prev;

        avimuxin->stream_time += avimuxin->strh.dwScale;
        avimuxin->strh.dwLength++;
        if(!(flags & AM_SAMPLE_TIMEDISCONTINUITY)) {
            if(!avimuxin->ix->qwBaseOffset)
                avimuxin->ix->qwBaseOffset = avimux->out->size;
            avimuxin->ix->aIndex[avimuxin->ix->nEntriesInUse].dwOffset = avimux->out->size
                + sizeof(RIFFCHUNK) - avimuxin->ix->qwBaseOffset;

            hr = out_seek(avimux, avimux->out->size);
            if(FAILED(hr)) {
                IMediaSample_Release(sample);
                return hr;
            }
        }
        avimuxin->ix->aIndex[avimuxin->ix->nEntriesInUse].dwSize = size |
            (flags & AM_SAMPLE_SPLICEPOINT ? 0 : AVISTDINDEX_DELTAFRAME);
        avimuxin->ix->nEntriesInUse++;

        rf.fcc = FCC('0'+avimuxin->stream_id/10, '0'+avimuxin->stream_id%10,
                'd', flags & AM_SAMPLE_SPLICEPOINT ? 'b' : 'c');
        rf.cb = size;
        hr = idx1_add_entry(avimux, rf.fcc, flags & AM_SAMPLE_SPLICEPOINT ? AVIIF_KEYFRAME : 0,
                flags & AM_SAMPLE_TIMEDISCONTINUITY ?
                avimux->idx1[avimux->idx1_entries-1].dwChunkOffset : avimux->out->size, size);
        if(FAILED(hr)) {
            IMediaSample_Release(sample);
            return hr;
        }

        if(!(flags & AM_SAMPLE_TIMEDISCONTINUITY)) {
            hr = out_write(avimux, &rf, sizeof(rf));
            if(FAILED(hr)) {
                IMediaSample_Release(sample);
                return hr;
            }
            hr = out_write(avimux, data, size);
            if(FAILED(hr)) {
                IMediaSample_Release(sample);
                return hr;
            }
            flags = 0;
            hr = out_write(avimux, &flags, ALIGN(rf.cb)-rf.cb);
            if(FAILED(hr)) {
                IMediaSample_Release(sample);
                return hr;
            }
        }
        IMediaSample_Release(sample);
    }
    return S_OK;
}

static HRESULT queue_sample(AviMux *avimux, AviMuxIn *avimuxin, IMediaSample *sample)
{
    IMediaSample **prev, **head_prev;
    HRESULT hr;

    hr = IMediaSample_GetPointer(sample, (BYTE**)&prev);
    if(FAILED(hr))
        return hr;
    prev--;

    if(avimuxin->samples_head) {
        hr = IMediaSample_GetPointer(avimuxin->samples_head, (BYTE**)&head_prev);
        if(FAILED(hr))
            return hr;
        head_prev--;

        *prev = *head_prev;
        *head_prev = sample;
    }else {
        *prev = sample;
    }
    avimuxin->samples_head = sample;
    IMediaSample_AddRef(sample);

    return flush_queue(avimux, avimuxin, FALSE);
}

static HRESULT WINAPI AviMux_Stop(IBaseFilter *iface)
{
    AviMux *This = impl_from_IBaseFilter(iface);
    HRESULT hr;
    int i;

    TRACE("(%p)\n", This);

    if(This->filter.state == State_Stopped)
        return S_OK;

    if(This->out->stream) {
        AVIEXTHEADER dmlh;
        RIFFCHUNK rc;
        RIFFLIST rl;
        int idx1_off, empty_stream;

        empty_stream = This->out->cur_stream;
        for(i=empty_stream+1; ; i++) {
            if(i >= This->input_pin_no-1)
                i = 0;
            if(i == empty_stream)
                break;

            This->out->cur_stream = i;
            hr = flush_queue(This, This->in[This->out->cur_stream], TRUE);
            if(FAILED(hr))
                return hr;
        }

        idx1_off = This->out->size;
        rc.fcc = ckidAVIOLDINDEX;
        rc.cb = This->idx1_entries * sizeof(*This->idx1);
        hr = out_write(This, &rc, sizeof(rc));
        if(FAILED(hr))
            return hr;
        hr = out_write(This, This->idx1, This->idx1_entries * sizeof(*This->idx1));
        if(FAILED(hr))
            return hr;
        /* native writes 8 '\0' characters after the end of RIFF data */
        i = 0;
        hr = out_write(This, &i, sizeof(i));
        if(FAILED(hr))
            return hr;
        hr = out_write(This, &i, sizeof(i));
        if(FAILED(hr))
            return hr;

        for(i=0; i<This->input_pin_no; i++) {
            if(!This->in[i]->pin.pin.pConnectedTo)
                continue;

            hr = out_seek(This, This->in[i]->ix_off);
            if(FAILED(hr))
                return hr;

            This->in[i]->indx->aIndex[This->in[i]->indx->nEntriesInUse].qwOffset = This->in[i]->ix_off;
            This->in[i]->indx->aIndex[This->in[i]->indx->nEntriesInUse].dwSize = sizeof(This->in[i]->ix_data);
            This->in[i]->indx->aIndex[This->in[i]->indx->nEntriesInUse].dwDuration = This->in[i]->strh.dwLength;
            if(This->in[i]->indx->nEntriesInUse) {
                This->in[i]->indx->aIndex[This->in[i]->indx->nEntriesInUse].dwDuration -=
                    This->in[i]->indx->aIndex[This->in[i]->indx->nEntriesInUse-1].dwDuration;
            }
            This->in[i]->indx->nEntriesInUse++;
            hr = out_write(This, This->in[i]->ix, sizeof(This->in[i]->ix_data));
            if(FAILED(hr))
                return hr;
        }

        hr = out_seek(This, 0);
        if(FAILED(hr))
            return hr;

        rl.fcc = FCC('R','I','F','F');
        rl.cb = This->out->size-sizeof(RIFFCHUNK)-2*sizeof(int);
        rl.fccListType = FCC('A','V','I',' ');
        hr = out_write(This, &rl, sizeof(rl));
        if(FAILED(hr))
            return hr;

        rl.fcc = FCC('L','I','S','T');
        rl.cb = This->out->movi_off - sizeof(RIFFLIST) - sizeof(RIFFCHUNK);
        rl.fccListType = FCC('h','d','r','l');
        hr = out_write(This, &rl, sizeof(rl));
        if(FAILED(hr))
            return hr;

        /* FIXME: set This->avih.dwMaxBytesPerSec value */
        This->avih.dwTotalFrames = (This->stop-This->start) / 10 / This->avih.dwMicroSecPerFrame;
        hr = out_write(This, &This->avih, sizeof(This->avih));
        if(FAILED(hr))
            return hr;

        for(i=0; i<This->input_pin_no; i++) {
            if(!This->in[i]->pin.pin.pConnectedTo)
                continue;

            rl.cb = sizeof(FOURCC) + sizeof(AVISTREAMHEADER) + sizeof(RIFFCHUNK) +
                This->in[i]->strf->cb + sizeof(This->in[i]->indx_data);
            rl.fccListType = ckidSTREAMLIST;
            hr = out_write(This, &rl, sizeof(rl));
            if(FAILED(hr))
                return hr;

            hr = out_write(This, &This->in[i]->strh, sizeof(AVISTREAMHEADER));
            if(FAILED(hr))
                return hr;

            hr = out_write(This, This->in[i]->strf, sizeof(RIFFCHUNK) + This->in[i]->strf->cb);
            if(FAILED(hr))
                return hr;

            hr = out_write(This, This->in[i]->indx, sizeof(This->in[i]->indx_data));
            if(FAILED(hr))
                return hr;
        }

        rl.cb = sizeof(dmlh) + sizeof(FOURCC);
        rl.fccListType = ckidODML;
        hr = out_write(This, &rl, sizeof(rl));
        if(FAILED(hr))
            return hr;

        memset(&dmlh, 0, sizeof(dmlh));
        dmlh.fcc = ckidAVIEXTHEADER;
        dmlh.cb = sizeof(dmlh) - sizeof(RIFFCHUNK);
        dmlh.dwGrandFrames = This->in[0]->strh.dwLength;
        hr = out_write(This, &dmlh, sizeof(dmlh));

        rl.cb = idx1_off - This->out->movi_off - sizeof(RIFFCHUNK);
        rl.fccListType = FCC('m','o','v','i');
        out_write(This, &rl, sizeof(rl));
        out_flush(This);

        IStream_Release(This->out->stream);
        This->out->stream = NULL;
    }

    This->filter.state = State_Stopped;
    return S_OK;
}

static HRESULT WINAPI AviMux_Pause(IBaseFilter *iface)
{
    AviMux *This = impl_from_IBaseFilter(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI AviMux_Run(IBaseFilter *iface, REFERENCE_TIME tStart)
{
    AviMux *This = impl_from_IBaseFilter(iface);
    HRESULT hr;
    int i, stream_id;

    TRACE("(%p)->(%s)\n", This, wine_dbgstr_longlong(tStart));

    if(This->filter.state == State_Running)
        return S_OK;

    if(This->mode != INTERLEAVE_FULL) {
        FIXME("mode not supported (%d)\n", This->mode);
        return E_NOTIMPL;
    }

    if(tStart)
        FIXME("tStart parameter ignored\n");

    for(i=0; i<This->input_pin_no; i++) {
        IMediaSeeking *ms;
        LONGLONG cur, stop;

        if(!This->in[i]->pin.pin.pConnectedTo)
            continue;

        hr = IPin_QueryInterface(This->in[i]->pin.pin.pConnectedTo,
                &IID_IMediaSeeking, (void**)&ms);
        if(FAILED(hr))
            continue;

        hr = IMediaSeeking_GetPositions(ms, &cur, &stop);
        if(FAILED(hr)) {
            IMediaSeeking_Release(ms);
            continue;
        }

        FIXME("Use IMediaSeeking to fill stream header\n");
        IMediaSeeking_Release(ms);
    }

    if(This->out->pin.pMemInputPin) {
        hr = IMemInputPin_QueryInterface(This->out->pin.pMemInputPin,
                &IID_IStream, (void**)&This->out->stream);
        if(FAILED(hr))
            return hr;
    }

    This->idx1_entries = 0;
    if(!This->idx1_size) {
        This->idx1_size = 1024;
        This->idx1 = HeapAlloc(GetProcessHeap(), 0, sizeof(*This->idx1)*This->idx1_size);
        if(!This->idx1)
            return E_OUTOFMEMORY;
    }

    This->out->size = 3*sizeof(RIFFLIST) + sizeof(AVIMAINHEADER) + sizeof(AVIEXTHEADER);
    This->start = -1;
    This->stop = -1;
    memset(&This->avih, 0, sizeof(This->avih));
    for(i=0; i<This->input_pin_no; i++) {
        if(!This->in[i]->pin.pin.pConnectedTo)
            continue;

        This->avih.dwStreams++;
        This->out->size += sizeof(RIFFLIST) + sizeof(AVISTREAMHEADER) + sizeof(RIFFCHUNK)
            + This->in[i]->strf->cb + sizeof(This->in[i]->indx_data);

        This->in[i]->strh.dwScale = MulDiv(This->in[i]->avg_time_per_frame, This->interleave, 10000000);
        This->in[i]->strh.dwRate = This->interleave;

        hr = IMemAllocator_Commit(This->in[i]->pin.pAllocator);
        if(FAILED(hr)) {
            if(This->out->stream) {
                IStream_Release(This->out->stream);
                This->out->stream = NULL;
            }
            return hr;
        }
    }

    This->out->movi_off = This->out->size;
    This->out->size += sizeof(RIFFLIST);

    idx1_add_entry(This, FCC('7','F','x','x'), 0, This->out->movi_off+sizeof(RIFFLIST), 0);

    stream_id = 0;
    for(i=0; i<This->input_pin_no; i++) {
        if(!This->in[i]->pin.pin.pConnectedTo)
            continue;

        This->in[i]->ix_off = This->out->size;
        This->out->size += sizeof(This->in[i]->ix_data);
        This->in[i]->ix->fcc = FCC('i','x','0'+stream_id/10,'0'+stream_id%10);
        This->in[i]->ix->cb = sizeof(This->in[i]->ix_data) - sizeof(RIFFCHUNK);
        This->in[i]->ix->wLongsPerEntry = 2;
        This->in[i]->ix->bIndexSubType = 0;
        This->in[i]->ix->bIndexType = AVI_INDEX_OF_CHUNKS;
        This->in[i]->ix->dwChunkId = FCC('0'+stream_id/10,'0'+stream_id%10,'d','b');
        This->in[i]->ix->qwBaseOffset = 0;

        This->in[i]->indx->fcc = ckidAVISUPERINDEX;
        This->in[i]->indx->cb = sizeof(This->in[i]->indx_data) - sizeof(RIFFCHUNK);
        This->in[i]->indx->wLongsPerEntry = 4;
        This->in[i]->indx->bIndexSubType = 0;
        This->in[i]->indx->bIndexType = AVI_INDEX_OF_INDEXES;
        This->in[i]->indx->dwChunkId = This->in[i]->ix->dwChunkId;
        This->in[i]->stream_id = stream_id++;
    }

    This->out->buf_pos = 0;
    This->out->out_pos = 0;

    This->avih.fcc = ckidMAINAVIHEADER;
    This->avih.cb = sizeof(AVIMAINHEADER) - sizeof(RIFFCHUNK);
    /* TODO: Use first video stream */
    This->avih.dwMicroSecPerFrame = This->in[0]->avg_time_per_frame/10;
    This->avih.dwPaddingGranularity = 1;
    This->avih.dwFlags = AVIF_TRUSTCKTYPE | AVIF_HASINDEX;
    This->avih.dwWidth = ((BITMAPINFOHEADER*)This->in[0]->strf->data)->biWidth;
    This->avih.dwHeight = ((BITMAPINFOHEADER*)This->in[0]->strf->data)->biHeight;

    This->filter.state = State_Running;
    return S_OK;
}

static HRESULT WINAPI AviMux_EnumPins(IBaseFilter *iface, IEnumPins **ppEnum)
{
    AviMux *This = impl_from_IBaseFilter(iface);
    TRACE("(%p)->(%p)\n", This, ppEnum);
    return BaseFilterImpl_EnumPins(iface, ppEnum);
}

static HRESULT WINAPI AviMux_FindPin(IBaseFilter *iface, LPCWSTR Id, IPin **ppPin)
{
    AviMux *This = impl_from_IBaseFilter(iface);
    int i;

    TRACE("(%p)->(%s %p)\n", This, debugstr_w(Id), ppPin);

    if(!Id || !ppPin)
        return E_POINTER;

    if(!lstrcmpiW(Id, This->out->pin.pin.pinInfo.achName)) {
        IPin_AddRef(&This->out->pin.pin.IPin_iface);
        *ppPin = &This->out->pin.pin.IPin_iface;
        return S_OK;
    }

    for(i=0; i<This->input_pin_no; i++) {
        if(lstrcmpiW(Id, This->in[i]->pin.pin.pinInfo.achName))
            continue;

        IPin_AddRef(&This->in[i]->pin.pin.IPin_iface);
        *ppPin = &This->in[i]->pin.pin.IPin_iface;
        return S_OK;
    }

    return VFW_E_NOT_FOUND;
}

static HRESULT WINAPI AviMux_QueryFilterInfo(IBaseFilter *iface, FILTER_INFO *pInfo)
{
    AviMux *This = impl_from_IBaseFilter(iface);
    FIXME("(%p)->(%p)\n", This, pInfo);
    return E_NOTIMPL;
}

static HRESULT WINAPI AviMux_QueryVendorInfo(IBaseFilter *iface, LPWSTR *pVendorInfo)
{
    AviMux *This = impl_from_IBaseFilter(iface);
    FIXME("(%p)->(%p)\n", This, pVendorInfo);
    return E_NOTIMPL;
}

static const IBaseFilterVtbl AviMuxVtbl = {
    AviMux_QueryInterface,
    BaseFilterImpl_AddRef,
    AviMux_Release,
    BaseFilterImpl_GetClassID,
    AviMux_Stop,
    AviMux_Pause,
    AviMux_Run,
    BaseFilterImpl_GetState,
    BaseFilterImpl_SetSyncSource,
    BaseFilterImpl_GetSyncSource,
    AviMux_EnumPins,
    AviMux_FindPin,
    AviMux_QueryFilterInfo,
    BaseFilterImpl_JoinFilterGraph,
    AviMux_QueryVendorInfo
};

static inline AviMux* impl_from_IConfigAviMux(IConfigAviMux *iface)
{
    return CONTAINING_RECORD(iface, AviMux, IConfigAviMux_iface);
}

static HRESULT WINAPI ConfigAviMux_QueryInterface(
        IConfigAviMux *iface, REFIID riid, void **ppv)
{
    AviMux *This = impl_from_IConfigAviMux(iface);
    return IBaseFilter_QueryInterface(&This->filter.IBaseFilter_iface, riid, ppv);
}

static ULONG WINAPI ConfigAviMux_AddRef(IConfigAviMux *iface)
{
    AviMux *This = impl_from_IConfigAviMux(iface);
    return IBaseFilter_AddRef(&This->filter.IBaseFilter_iface);
}

static ULONG WINAPI ConfigAviMux_Release(IConfigAviMux *iface)
{
    AviMux *This = impl_from_IConfigAviMux(iface);
    return IBaseFilter_Release(&This->filter.IBaseFilter_iface);
}

static HRESULT WINAPI ConfigAviMux_SetMasterStream(IConfigAviMux *iface, LONG iStream)
{
    AviMux *This = impl_from_IConfigAviMux(iface);
    FIXME("(%p)->(%d)\n", This, iStream);
    return E_NOTIMPL;
}

static HRESULT WINAPI ConfigAviMux_GetMasterStream(IConfigAviMux *iface, LONG *pStream)
{
    AviMux *This = impl_from_IConfigAviMux(iface);
    FIXME("(%p)->(%p)\n", This, pStream);
    return E_NOTIMPL;
}

static HRESULT WINAPI ConfigAviMux_SetOutputCompatibilityIndex(
        IConfigAviMux *iface, BOOL fOldIndex)
{
    AviMux *This = impl_from_IConfigAviMux(iface);
    FIXME("(%p)->(%x)\n", This, fOldIndex);
    return E_NOTIMPL;
}

static HRESULT WINAPI ConfigAviMux_GetOutputCompatibilityIndex(
        IConfigAviMux *iface, BOOL *pfOldIndex)
{
    AviMux *This = impl_from_IConfigAviMux(iface);
    FIXME("(%p)->(%p)\n", This, pfOldIndex);
    return E_NOTIMPL;
}

static const IConfigAviMuxVtbl ConfigAviMuxVtbl = {
    ConfigAviMux_QueryInterface,
    ConfigAviMux_AddRef,
    ConfigAviMux_Release,
    ConfigAviMux_SetMasterStream,
    ConfigAviMux_GetMasterStream,
    ConfigAviMux_SetOutputCompatibilityIndex,
    ConfigAviMux_GetOutputCompatibilityIndex
};

static inline AviMux* impl_from_IConfigInterleaving(IConfigInterleaving *iface)
{
    return CONTAINING_RECORD(iface, AviMux, IConfigInterleaving_iface);
}

static HRESULT WINAPI ConfigInterleaving_QueryInterface(
        IConfigInterleaving *iface, REFIID riid, void **ppv)
{
    AviMux *This = impl_from_IConfigInterleaving(iface);
    return IBaseFilter_QueryInterface(&This->filter.IBaseFilter_iface, riid, ppv);
}

static ULONG WINAPI ConfigInterleaving_AddRef(IConfigInterleaving *iface)
{
    AviMux *This = impl_from_IConfigInterleaving(iface);
    return IBaseFilter_AddRef(&This->filter.IBaseFilter_iface);
}

static ULONG WINAPI ConfigInterleaving_Release(IConfigInterleaving *iface)
{
    AviMux *This = impl_from_IConfigInterleaving(iface);
    return IBaseFilter_Release(&This->filter.IBaseFilter_iface);
}

static HRESULT WINAPI ConfigInterleaving_put_Mode(
        IConfigInterleaving *iface, InterleavingMode mode)
{
    AviMux *This = impl_from_IConfigInterleaving(iface);

    TRACE("(%p)->(%d)\n", This, mode);

    if(mode>INTERLEAVE_NONE_BUFFERED)
        return E_INVALIDARG;

    if(This->mode != mode) {
        if(This->out->pin.pin.pConnectedTo) {
            HRESULT hr = IFilterGraph_Reconnect(This->filter.filterInfo.pGraph,
                    &This->out->pin.pin.IPin_iface);
            if(FAILED(hr))
                return hr;
        }

        This->mode = mode;
    }

    return S_OK;
}

static HRESULT WINAPI ConfigInterleaving_get_Mode(
        IConfigInterleaving *iface, InterleavingMode *pMode)
{
    AviMux *This = impl_from_IConfigInterleaving(iface);
    FIXME("(%p)->(%p)\n", This, pMode);
    return E_NOTIMPL;
}

static HRESULT WINAPI ConfigInterleaving_put_Interleaving(IConfigInterleaving *iface,
        const REFERENCE_TIME *prtInterleave, const REFERENCE_TIME *prtPreroll)
{
    AviMux *This = impl_from_IConfigInterleaving(iface);

    TRACE("(%p)->(%p %p)\n", This, prtInterleave, prtPreroll);

    if(prtInterleave)
        This->interleave = *prtInterleave;
    if(prtPreroll)
        This->preroll = *prtPreroll;
    return S_OK;
}

static HRESULT WINAPI ConfigInterleaving_get_Interleaving(IConfigInterleaving *iface,
        REFERENCE_TIME *prtInterleave, REFERENCE_TIME *prtPreroll)
{
    AviMux *This = impl_from_IConfigInterleaving(iface);
    FIXME("(%p)->(%p %p)\n", This, prtInterleave, prtPreroll);
    return E_NOTIMPL;
}

static const IConfigInterleavingVtbl ConfigInterleavingVtbl = {
    ConfigInterleaving_QueryInterface,
    ConfigInterleaving_AddRef,
    ConfigInterleaving_Release,
    ConfigInterleaving_put_Mode,
    ConfigInterleaving_get_Mode,
    ConfigInterleaving_put_Interleaving,
    ConfigInterleaving_get_Interleaving
};

static inline AviMux* impl_from_IMediaSeeking(IMediaSeeking *iface)
{
    return CONTAINING_RECORD(iface, AviMux, IMediaSeeking_iface);
}

static HRESULT WINAPI MediaSeeking_QueryInterface(
        IMediaSeeking *iface, REFIID riid, void **ppv)
{
    AviMux *This = impl_from_IMediaSeeking(iface);
    return IBaseFilter_QueryInterface(&This->filter.IBaseFilter_iface, riid, ppv);
}

static ULONG WINAPI MediaSeeking_AddRef(IMediaSeeking *iface)
{
    AviMux *This = impl_from_IMediaSeeking(iface);
    return IBaseFilter_AddRef(&This->filter.IBaseFilter_iface);
}

static ULONG WINAPI MediaSeeking_Release(IMediaSeeking *iface)
{
    AviMux *This = impl_from_IMediaSeeking(iface);
    return IBaseFilter_Release(&This->filter.IBaseFilter_iface);
}

static HRESULT WINAPI MediaSeeking_GetCapabilities(
        IMediaSeeking *iface, DWORD *pCapabilities)
{
    AviMux *This = impl_from_IMediaSeeking(iface);
    FIXME("(%p)->(%p)\n", This, pCapabilities);
    return E_NOTIMPL;
}

static HRESULT WINAPI MediaSeeking_CheckCapabilities(
        IMediaSeeking *iface, DWORD *pCapabilities)
{
    AviMux *This = impl_from_IMediaSeeking(iface);
    FIXME("(%p)->(%p)\n", This, pCapabilities);
    return E_NOTIMPL;
}

static HRESULT WINAPI MediaSeeking_IsFormatSupported(
        IMediaSeeking *iface, const GUID *pFormat)
{
    AviMux *This = impl_from_IMediaSeeking(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_guid(pFormat));
    return E_NOTIMPL;
}

static HRESULT WINAPI MediaSeeking_QueryPreferredFormat(
        IMediaSeeking *iface, GUID *pFormat)
{
    AviMux *This = impl_from_IMediaSeeking(iface);
    FIXME("(%p)->(%p)\n", This, pFormat);
    return E_NOTIMPL;
}

static HRESULT WINAPI MediaSeeking_GetTimeFormat(
        IMediaSeeking *iface, GUID *pFormat)
{
    AviMux *This = impl_from_IMediaSeeking(iface);
    FIXME("(%p)->(%p)\n", This, pFormat);
    return E_NOTIMPL;
}

static HRESULT WINAPI MediaSeeking_IsUsingTimeFormat(
        IMediaSeeking *iface, const GUID *pFormat)
{
    AviMux *This = impl_from_IMediaSeeking(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_guid(pFormat));
    return E_NOTIMPL;
}

static HRESULT WINAPI MediaSeeking_SetTimeFormat(
        IMediaSeeking *iface, const GUID *pFormat)
{
    AviMux *This = impl_from_IMediaSeeking(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_guid(pFormat));
    return E_NOTIMPL;
}

static HRESULT WINAPI MediaSeeking_GetDuration(
        IMediaSeeking *iface, LONGLONG *pDuration)
{
    AviMux *This = impl_from_IMediaSeeking(iface);
    FIXME("(%p)->(%p)\n", This, pDuration);
    return E_NOTIMPL;
}

static HRESULT WINAPI MediaSeeking_GetStopPosition(
        IMediaSeeking *iface, LONGLONG *pStop)
{
    AviMux *This = impl_from_IMediaSeeking(iface);
    FIXME("(%p)->(%p)\n", This, pStop);
    return E_NOTIMPL;
}

static HRESULT WINAPI MediaSeeking_GetCurrentPosition(
        IMediaSeeking *iface, LONGLONG *pCurrent)
{
    AviMux *This = impl_from_IMediaSeeking(iface);
    FIXME("(%p)->(%p)\n", This, pCurrent);
    return E_NOTIMPL;
}

static HRESULT WINAPI MediaSeeking_ConvertTimeFormat(IMediaSeeking *iface, LONGLONG *pTarget,
        const GUID *pTargetFormat, LONGLONG Source, const GUID *pSourceFormat)
{
    AviMux *This = impl_from_IMediaSeeking(iface);
    FIXME("(%p)->(%p %s %s %s)\n", This, pTarget, debugstr_guid(pTargetFormat),
            wine_dbgstr_longlong(Source), debugstr_guid(pSourceFormat));
    return E_NOTIMPL;
}

static HRESULT WINAPI MediaSeeking_SetPositions(IMediaSeeking *iface, LONGLONG *pCurrent,
        DWORD dwCurrentFlags, LONGLONG *pStop, DWORD dwStopFlags)
{
    AviMux *This = impl_from_IMediaSeeking(iface);
    FIXME("(%p)->(%p %x %p %x)\n", This, pCurrent, dwCurrentFlags, pStop, dwStopFlags);
    return E_NOTIMPL;
}

static HRESULT WINAPI MediaSeeking_GetPositions(IMediaSeeking *iface,
        LONGLONG *pCurrent, LONGLONG *pStop)
{
    AviMux *This = impl_from_IMediaSeeking(iface);
    FIXME("(%p)->(%p %p)\n", This, pCurrent, pStop);
    return E_NOTIMPL;
}

static HRESULT WINAPI MediaSeeking_GetAvailable(IMediaSeeking *iface,
        LONGLONG *pEarliest, LONGLONG *pLatest)
{
    AviMux *This = impl_from_IMediaSeeking(iface);
    FIXME("(%p)->(%p %p)\n", This, pEarliest, pLatest);
    return E_NOTIMPL;
}

static HRESULT WINAPI MediaSeeking_SetRate(IMediaSeeking *iface, double dRate)
{
    AviMux *This = impl_from_IMediaSeeking(iface);
    FIXME("(%p)->(%lf)\n", This, dRate);
    return E_NOTIMPL;
}

static HRESULT WINAPI MediaSeeking_GetRate(IMediaSeeking *iface, double *pdRate)
{
    AviMux *This = impl_from_IMediaSeeking(iface);
    FIXME("(%p)->(%p)\n", This, pdRate);
    return E_NOTIMPL;
}

static HRESULT WINAPI MediaSeeking_GetPreroll(IMediaSeeking *iface, LONGLONG *pllPreroll)
{
    AviMux *This = impl_from_IMediaSeeking(iface);
    FIXME("(%p)->(%p)\n", This, pllPreroll);
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

static inline AviMux* impl_from_IPersistMediaPropertyBag(IPersistMediaPropertyBag *iface)
{
    return CONTAINING_RECORD(iface, AviMux, IPersistMediaPropertyBag_iface);
}

static HRESULT WINAPI PersistMediaPropertyBag_QueryInterface(
        IPersistMediaPropertyBag *iface, REFIID riid, void **ppv)
{
    AviMux *This = impl_from_IPersistMediaPropertyBag(iface);
    return IBaseFilter_QueryInterface(&This->filter.IBaseFilter_iface, riid, ppv);
}

static ULONG WINAPI PersistMediaPropertyBag_AddRef(IPersistMediaPropertyBag *iface)
{
    AviMux *This = impl_from_IPersistMediaPropertyBag(iface);
    return IBaseFilter_AddRef(&This->filter.IBaseFilter_iface);
}

static ULONG WINAPI PersistMediaPropertyBag_Release(IPersistMediaPropertyBag *iface)
{
    AviMux *This = impl_from_IPersistMediaPropertyBag(iface);
    return IBaseFilter_Release(&This->filter.IBaseFilter_iface);
}

static HRESULT WINAPI PersistMediaPropertyBag_GetClassID(
        IPersistMediaPropertyBag *iface, CLSID *pClassID)
{
    AviMux *This = impl_from_IPersistMediaPropertyBag(iface);
    return IBaseFilter_GetClassID(&This->filter.IBaseFilter_iface, pClassID);
}

static HRESULT WINAPI PersistMediaPropertyBag_InitNew(IPersistMediaPropertyBag *iface)
{
    AviMux *This = impl_from_IPersistMediaPropertyBag(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI PersistMediaPropertyBag_Load(IPersistMediaPropertyBag *iface,
        IMediaPropertyBag *pPropBag, IErrorLog *pErrorLog)
{
    AviMux *This = impl_from_IPersistMediaPropertyBag(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI PersistMediaPropertyBag_Save(IPersistMediaPropertyBag *iface,
        IMediaPropertyBag *pPropBag, BOOL fClearDirty, BOOL fSaveAllProperties)
{
    AviMux *This = impl_from_IPersistMediaPropertyBag(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static const IPersistMediaPropertyBagVtbl PersistMediaPropertyBagVtbl = {
    PersistMediaPropertyBag_QueryInterface,
    PersistMediaPropertyBag_AddRef,
    PersistMediaPropertyBag_Release,
    PersistMediaPropertyBag_GetClassID,
    PersistMediaPropertyBag_InitNew,
    PersistMediaPropertyBag_Load,
    PersistMediaPropertyBag_Save
};

static inline AviMux* impl_from_ISpecifyPropertyPages(ISpecifyPropertyPages *iface)
{
    return CONTAINING_RECORD(iface, AviMux, ISpecifyPropertyPages_iface);
}

static HRESULT WINAPI SpecifyPropertyPages_QueryInterface(
        ISpecifyPropertyPages *iface, REFIID riid, void **ppv)
{
    AviMux *This = impl_from_ISpecifyPropertyPages(iface);
    return IBaseFilter_QueryInterface(&This->filter.IBaseFilter_iface, riid, ppv);
}

static ULONG WINAPI SpecifyPropertyPages_AddRef(ISpecifyPropertyPages *iface)
{
    AviMux *This = impl_from_ISpecifyPropertyPages(iface);
    return IBaseFilter_AddRef(&This->filter.IBaseFilter_iface);
}

static ULONG WINAPI SpecifyPropertyPages_Release(ISpecifyPropertyPages *iface)
{
    AviMux *This = impl_from_ISpecifyPropertyPages(iface);
    return IBaseFilter_Release(&This->filter.IBaseFilter_iface);
}

static HRESULT WINAPI SpecifyPropertyPages_GetPages(
        ISpecifyPropertyPages *iface, CAUUID *pPages)
{
    AviMux *This = impl_from_ISpecifyPropertyPages(iface);
    FIXME("(%p)->(%p)\n", This, pPages);
    return E_NOTIMPL;
}

static const ISpecifyPropertyPagesVtbl SpecifyPropertyPagesVtbl = {
    SpecifyPropertyPages_QueryInterface,
    SpecifyPropertyPages_AddRef,
    SpecifyPropertyPages_Release,
    SpecifyPropertyPages_GetPages
};

static HRESULT WINAPI AviMuxOut_AttemptConnection(BasePin *base,
        IPin *pReceivePin, const AM_MEDIA_TYPE *pmt)
{
    PIN_DIRECTION dir;
    HRESULT hr;

    TRACE("(%p)->(%p AM_MEDIA_TYPE(%p))\n", base, pReceivePin, pmt);
    dump_AM_MEDIA_TYPE(pmt);

    hr = IPin_QueryDirection(pReceivePin, &dir);
    if(hr==S_OK && dir!=PINDIR_INPUT)
        return VFW_E_INVALID_DIRECTION;

    return BaseOutputPinImpl_AttemptConnection(base, pReceivePin, pmt);
}

static LONG WINAPI AviMuxOut_GetMediaTypeVersion(BasePin *base)
{
    return 0;
}

static HRESULT WINAPI AviMuxOut_GetMediaType(BasePin *base, int iPosition, AM_MEDIA_TYPE *amt)
{
    TRACE("(%p)->(%d %p)\n", base, iPosition, amt);

    if(iPosition < 0)
        return E_INVALIDARG;
    if(iPosition > 0)
        return VFW_S_NO_MORE_ITEMS;

    amt->majortype = MEDIATYPE_Stream;
    amt->subtype = MEDIASUBTYPE_Avi;
    amt->bFixedSizeSamples = TRUE;
    amt->bTemporalCompression = FALSE;
    amt->lSampleSize = 1;
    amt->formattype = GUID_NULL;
    amt->pUnk = NULL;
    amt->cbFormat = 0;
    amt->pbFormat = NULL;
    return S_OK;
}

static HRESULT WINAPI AviMuxOut_DecideAllocator(BaseOutputPin *base,
        IMemInputPin *pPin, IMemAllocator **pAlloc)
{
    ALLOCATOR_PROPERTIES req, actual;
    HRESULT hr;

    TRACE("(%p)->(%p %p)\n", base, pPin, pAlloc);

    hr = BaseOutputPinImpl_InitAllocator(base, pAlloc);
    if(FAILED(hr))
        return hr;

    hr = IMemInputPin_GetAllocatorRequirements(pPin, &req);
    if(FAILED(hr))
        req.cbAlign = 1;
    req.cBuffers = 32;
    req.cbBuffer = 0;
    req.cbPrefix = 0;

    hr = IMemAllocator_SetProperties(*pAlloc, &req, &actual);
    if(FAILED(hr))
        return hr;

    return IMemInputPin_NotifyAllocator(pPin, *pAlloc, TRUE);
}

static HRESULT WINAPI AviMuxOut_BreakConnect(BaseOutputPin *base)
{
    FIXME("(%p)\n", base);
    return E_NOTIMPL;
}

static const BaseOutputPinFuncTable AviMuxOut_BaseOutputFuncTable = {
    {
        NULL,
        AviMuxOut_AttemptConnection,
        AviMuxOut_GetMediaTypeVersion,
        AviMuxOut_GetMediaType
    },
    NULL,
    AviMuxOut_DecideAllocator,
    AviMuxOut_BreakConnect
};

static inline AviMux* impl_from_out_IPin(IPin *iface)
{
    BasePin *bp = CONTAINING_RECORD(iface, BasePin, IPin_iface);
    IBaseFilter *bf = bp->pinInfo.pFilter;

    return impl_from_IBaseFilter(bf);
}

static HRESULT WINAPI AviMuxOut_QueryInterface(IPin *iface, REFIID riid, void **ppv)
{
    AviMux *This = impl_from_out_IPin(iface);

    TRACE("(%p)->(%s %p)\n", This, debugstr_guid(riid), ppv);

    if(IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_IPin))
        *ppv = iface;
    else if(IsEqualIID(riid, &IID_IQualityControl))
        *ppv = &This->out->IQualityControl_iface;
    else {
        FIXME("no interface for %s\n", debugstr_guid(riid));
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI AviMuxOut_AddRef(IPin *iface)
{
    AviMux *This = impl_from_out_IPin(iface);
    return IBaseFilter_AddRef(&This->filter.IBaseFilter_iface);
}

static ULONG WINAPI AviMuxOut_Release(IPin *iface)
{
    AviMux *This = impl_from_out_IPin(iface);
    return IBaseFilter_Release(&This->filter.IBaseFilter_iface);
}

static HRESULT WINAPI AviMuxOut_Connect(IPin *iface,
        IPin *pReceivePin, const AM_MEDIA_TYPE *pmt)
{
    AviMux *This = impl_from_out_IPin(iface);
    HRESULT hr;
    int i;

    TRACE("(%p)->(%p AM_MEDIA_TYPE(%p))\n", This, pReceivePin, pmt);
    dump_AM_MEDIA_TYPE(pmt);

    hr = BaseOutputPinImpl_Connect(iface, pReceivePin, pmt);
    if(FAILED(hr))
        return hr;

    for(i=0; i<This->input_pin_no; i++) {
        if(!This->in[i]->pin.pin.pConnectedTo)
            continue;

        hr = IFilterGraph_Reconnect(This->filter.filterInfo.pGraph, &This->in[i]->pin.pin.IPin_iface);
        if(FAILED(hr)) {
            BaseOutputPinImpl_Disconnect(iface);
            break;
        }
    }

    if(hr == S_OK)
        IBaseFilter_AddRef(&This->filter.IBaseFilter_iface);
    return hr;
}

static HRESULT WINAPI AviMuxOut_ReceiveConnection(IPin *iface,
        IPin *pConnector, const AM_MEDIA_TYPE *pmt)
{
    AviMux *This = impl_from_out_IPin(iface);
    TRACE("(%p)->(%p AM_MEDIA_TYPE(%p)\n", This, pConnector, pmt);
    dump_AM_MEDIA_TYPE(pmt);
    return BaseOutputPinImpl_ReceiveConnection(iface, pConnector, pmt);
}

static HRESULT WINAPI AviMuxOut_Disconnect(IPin *iface)
{
    AviMux *This = impl_from_out_IPin(iface);
    HRESULT hr;

    TRACE("(%p)\n", This);

    hr = BaseOutputPinImpl_Disconnect(iface);
    if(hr == S_OK)
        IBaseFilter_Release(&This->filter.IBaseFilter_iface);
    return hr;
}

static HRESULT WINAPI AviMuxOut_ConnectedTo(IPin *iface, IPin **pPin)
{
    AviMux *This = impl_from_out_IPin(iface);
    TRACE("(%p)->(%p)\n", This, pPin);
    return BasePinImpl_ConnectedTo(iface, pPin);
}

static HRESULT WINAPI AviMuxOut_ConnectionMediaType(IPin *iface, AM_MEDIA_TYPE *pmt)
{
    AviMux *This = impl_from_out_IPin(iface);
    TRACE("(%p)->(%p)\n", This, pmt);
    return BasePinImpl_ConnectionMediaType(iface, pmt);
}

static HRESULT WINAPI AviMuxOut_QueryPinInfo(IPin *iface, PIN_INFO *pInfo)
{
    AviMux *This = impl_from_out_IPin(iface);
    TRACE("(%p)->(%p)\n", This, pInfo);
    return BasePinImpl_QueryPinInfo(iface, pInfo);
}

static HRESULT WINAPI AviMuxOut_QueryDirection(IPin *iface, PIN_DIRECTION *pPinDir)
{
    AviMux *This = impl_from_out_IPin(iface);
    TRACE("(%p)->(%p)\n", This, pPinDir);
    return BasePinImpl_QueryDirection(iface, pPinDir);
}

static HRESULT WINAPI AviMuxOut_QueryId(IPin *iface, LPWSTR *Id)
{
    AviMux *This = impl_from_out_IPin(iface);
    TRACE("(%p)->(%p)\n", This, Id);
    return BasePinImpl_QueryId(iface, Id);
}

static HRESULT WINAPI AviMuxOut_QueryAccept(IPin *iface, const AM_MEDIA_TYPE *pmt)
{
    AviMux *This = impl_from_out_IPin(iface);
    TRACE("(%p)->(AM_MEDIA_TYPE(%p))\n", This, pmt);
    dump_AM_MEDIA_TYPE(pmt);
    return BasePinImpl_QueryAccept(iface, pmt);
}

static HRESULT WINAPI AviMuxOut_EnumMediaTypes(IPin *iface, IEnumMediaTypes **ppEnum)
{
    AviMux *This = impl_from_out_IPin(iface);
    TRACE("(%p)->(%p)\n", This, ppEnum);
    return BasePinImpl_EnumMediaTypes(iface, ppEnum);
}

static HRESULT WINAPI AviMuxOut_QueryInternalConnections(
        IPin *iface, IPin **apPin, ULONG *nPin)
{
    AviMux *This = impl_from_out_IPin(iface);
    FIXME("(%p)->(%p %p)\n", This, apPin, nPin);
    return E_NOTIMPL;
}

static HRESULT WINAPI AviMuxOut_EndOfStream(IPin *iface)
{
    AviMux *This = impl_from_out_IPin(iface);
    TRACE("(%p)\n", This);
    return BaseOutputPinImpl_EndOfStream(iface);
}

static HRESULT WINAPI AviMuxOut_BeginFlush(IPin *iface)
{
    AviMux *This = impl_from_out_IPin(iface);
    TRACE("(%p)\n", This);
    return BaseOutputPinImpl_BeginFlush(iface);
}

static HRESULT WINAPI AviMuxOut_EndFlush(IPin *iface)
{
    AviMux *This = impl_from_out_IPin(iface);
    TRACE("(%p)\n", This);
    return BaseOutputPinImpl_EndFlush(iface);
}

static HRESULT WINAPI AviMuxOut_NewSegment(IPin *iface, REFERENCE_TIME tStart,
        REFERENCE_TIME tStop, double dRate)
{
    AviMux *This = impl_from_out_IPin(iface);
    TRACE("(%p)->(%s %s %f)\n", This, wine_dbgstr_longlong(tStart), wine_dbgstr_longlong(tStop), dRate);
    return BasePinImpl_NewSegment(iface, tStart, tStop, dRate);
}

static const IPinVtbl AviMuxOut_PinVtbl = {
    AviMuxOut_QueryInterface,
    AviMuxOut_AddRef,
    AviMuxOut_Release,
    AviMuxOut_Connect,
    AviMuxOut_ReceiveConnection,
    AviMuxOut_Disconnect,
    AviMuxOut_ConnectedTo,
    AviMuxOut_ConnectionMediaType,
    AviMuxOut_QueryPinInfo,
    AviMuxOut_QueryDirection,
    AviMuxOut_QueryId,
    AviMuxOut_QueryAccept,
    AviMuxOut_EnumMediaTypes,
    AviMuxOut_QueryInternalConnections,
    AviMuxOut_EndOfStream,
    AviMuxOut_BeginFlush,
    AviMuxOut_EndFlush,
    AviMuxOut_NewSegment
};

static inline AviMux* impl_from_out_IQualityControl(IQualityControl *iface)
{
    AviMuxOut *amo = CONTAINING_RECORD(iface, AviMuxOut, IQualityControl_iface);
    return impl_from_IBaseFilter(amo->pin.pin.pinInfo.pFilter);
}

static HRESULT WINAPI AviMuxOut_QualityControl_QueryInterface(
        IQualityControl *iface, REFIID riid, void **ppv)
{
    AviMux *This = impl_from_out_IQualityControl(iface);
    return IPin_QueryInterface(&This->out->pin.pin.IPin_iface, riid, ppv);
}

static ULONG WINAPI AviMuxOut_QualityControl_AddRef(IQualityControl *iface)
{
    AviMux *This = impl_from_out_IQualityControl(iface);
    return IBaseFilter_AddRef(&This->filter.IBaseFilter_iface);
}

static ULONG WINAPI AviMuxOut_QualityControl_Release(IQualityControl *iface)
{
    AviMux *This = impl_from_out_IQualityControl(iface);
    return IBaseFilter_Release(&This->filter.IBaseFilter_iface);
}

static HRESULT WINAPI AviMuxOut_QualityControl_Notify(IQualityControl *iface,
        IBaseFilter *pSelf, Quality q)
{
    AviMux *This = impl_from_out_IQualityControl(iface);
    FIXME("(%p)->(%p { 0x%x %u %s %s })\n", This, pSelf,
            q.Type, q.Proportion,
            wine_dbgstr_longlong(q.Late),
            wine_dbgstr_longlong(q.TimeStamp));
    return E_NOTIMPL;
}

static HRESULT WINAPI AviMuxOut_QualityControl_SetSink(
        IQualityControl *iface, IQualityControl *piqc)
{
    AviMux *This = impl_from_out_IQualityControl(iface);
    FIXME("(%p)->(%p)\n", This, piqc);
    return E_NOTIMPL;
}

static const IQualityControlVtbl AviMuxOut_QualityControlVtbl = {
    AviMuxOut_QualityControl_QueryInterface,
    AviMuxOut_QualityControl_AddRef,
    AviMuxOut_QualityControl_Release,
    AviMuxOut_QualityControl_Notify,
    AviMuxOut_QualityControl_SetSink
};

static HRESULT WINAPI AviMuxIn_CheckMediaType(BasePin *base, const AM_MEDIA_TYPE *pmt)
{
    TRACE("(%p:%s)->(AM_MEDIA_TYPE(%p))\n", base, debugstr_w(base->pinInfo.achName), pmt);
    dump_AM_MEDIA_TYPE(pmt);

    if(IsEqualIID(&pmt->majortype, &MEDIATYPE_Audio) &&
            IsEqualIID(&pmt->formattype, &FORMAT_WaveFormatEx))
        return S_OK;
    if(IsEqualIID(&pmt->majortype, &MEDIATYPE_Interleaved) &&
            IsEqualIID(&pmt->formattype, &FORMAT_DvInfo))
        return S_OK;
    if(IsEqualIID(&pmt->majortype, &MEDIATYPE_Video) &&
            (IsEqualIID(&pmt->formattype, &FORMAT_VideoInfo) ||
             IsEqualIID(&pmt->formattype, &FORMAT_DvInfo)))
        return S_OK;
    return S_FALSE;
}

static LONG WINAPI AviMuxIn_GetMediaTypeVersion(BasePin *base)
{
    return 0;
}

static HRESULT WINAPI AviMuxIn_GetMediaType(BasePin *base, int iPosition, AM_MEDIA_TYPE *amt)
{
    return S_FALSE;
}

static HRESULT WINAPI AviMuxIn_Receive(BaseInputPin *base, IMediaSample *pSample)
{
    AviMuxIn *avimuxin = CONTAINING_RECORD(base, AviMuxIn, pin);
    AviMux *avimux = impl_from_IBaseFilter(base->pin.pinInfo.pFilter);
    REFERENCE_TIME start, stop;
    IMediaSample *sample;
    int frames_no;
    IMediaSample2 *ms2;
    BYTE *frame, *buf;
    DWORD max_size, size;
    DWORD flags;
    HRESULT hr;

    TRACE("(%p:%s)->(%p)\n", base, debugstr_w(base->pin.pinInfo.achName), pSample);

    hr = IMediaSample_QueryInterface(pSample, &IID_IMediaSample2, (void**)&ms2);
    if(SUCCEEDED(hr)) {
        AM_SAMPLE2_PROPERTIES props;

        memset(&props, 0, sizeof(props));
        hr = IMediaSample2_GetProperties(ms2, sizeof(props), (BYTE*)&props);
        IMediaSample2_Release(ms2);
        if(FAILED(hr))
            return hr;

        flags = props.dwSampleFlags;
        frame = props.pbBuffer;
        size = props.lActual;
    }else {
        flags = IMediaSample_IsSyncPoint(pSample) == S_OK ? AM_SAMPLE_SPLICEPOINT : 0;
        hr = IMediaSample_GetPointer(pSample, &frame);
        if(FAILED(hr))
            return hr;
        size = IMediaSample_GetActualDataLength(pSample);
    }

    if(!avimuxin->pin.pin.mtCurrent.bTemporalCompression)
        flags |= AM_SAMPLE_SPLICEPOINT;

    hr = IMediaSample_GetTime(pSample, &start, &stop);
    if(FAILED(hr))
        return hr;

    if(avimuxin->stop>stop)
        return VFW_E_START_TIME_AFTER_END;

    if(avimux->start == -1)
        avimux->start = start;
    if(avimux->stop < stop)
        avimux->stop = stop;

    if(avimux->avih.dwSuggestedBufferSize < ALIGN(size)+sizeof(RIFFCHUNK))
        avimux->avih.dwSuggestedBufferSize = ALIGN(size) + sizeof(RIFFCHUNK);
    if(avimuxin->strh.dwSuggestedBufferSize < ALIGN(size)+sizeof(RIFFCHUNK))
        avimuxin->strh.dwSuggestedBufferSize = ALIGN(size) + sizeof(RIFFCHUNK);

    frames_no = 1;
    if(avimuxin->stop!=-1 && start > avimuxin->stop) {
        frames_no += (double)(start - avimuxin->stop) / 10000000
                * avimuxin->strh.dwRate / avimuxin->strh.dwScale + 0.5;
    }
    avimuxin->stop = stop;

    while(--frames_no) {
        /* TODO: store all control frames in one buffer */
        hr = IMemAllocator_GetBuffer(avimuxin->samples_allocator, &sample, NULL, NULL, 0);
        if(FAILED(hr))
            return hr;
        hr = IMediaSample_SetActualDataLength(sample, 0);
        if(SUCCEEDED(hr))
            hr = IMediaSample_SetDiscontinuity(sample, TRUE);
        if(SUCCEEDED(hr))
            hr = IMediaSample_SetSyncPoint(sample, FALSE);
        if(SUCCEEDED(hr))
            hr = queue_sample(avimux, avimuxin, sample);
        IMediaSample_Release(sample);
        if(FAILED(hr))
            return hr;
    }

    hr = IMemAllocator_GetBuffer(avimuxin->samples_allocator, &sample, NULL, NULL, 0);
    if(FAILED(hr))
        return hr;
    max_size = IMediaSample_GetSize(sample);
    if(size > max_size)
        size = max_size;
    hr = IMediaSample_SetActualDataLength(sample, size);
    if(SUCCEEDED(hr))
        hr = IMediaSample_SetDiscontinuity(sample, FALSE);
    if(SUCCEEDED(hr))
        hr = IMediaSample_SetSyncPoint(sample, flags & AM_SAMPLE_SPLICEPOINT);
    /* TODO: avoid unnecessary copying */
    if(SUCCEEDED(hr))
        hr = IMediaSample_GetPointer(sample, &buf);
    if(SUCCEEDED(hr)) {
        memcpy(buf, frame, size);
        hr = queue_sample(avimux, avimuxin, sample);
    }
    IMediaSample_Release(sample);

    return hr;
}

static const BaseInputPinFuncTable AviMuxIn_BaseInputFuncTable = {
    {
        AviMuxIn_CheckMediaType,
        NULL,
        AviMuxIn_GetMediaTypeVersion,
        AviMuxIn_GetMediaType
    },
    AviMuxIn_Receive
};

static inline AviMux* impl_from_in_IPin(IPin *iface)
{
    BasePin *bp = CONTAINING_RECORD(iface, BasePin, IPin_iface);
    IBaseFilter *bf = bp->pinInfo.pFilter;

    return impl_from_IBaseFilter(bf);
}

static inline AviMuxIn* AviMuxIn_from_IPin(IPin *iface)
{
    BasePin *bp = CONTAINING_RECORD(iface, BasePin, IPin_iface);
    BaseInputPin *bip = CONTAINING_RECORD(bp, BaseInputPin, pin);
    return CONTAINING_RECORD(bip, AviMuxIn, pin);
}

static HRESULT WINAPI AviMuxIn_QueryInterface(IPin *iface, REFIID riid, void **ppv)
{
    AviMux *This = impl_from_in_IPin(iface);
    AviMuxIn *avimuxin = AviMuxIn_from_IPin(iface);

    TRACE("(%p:%s)->(%s %p)\n", This, debugstr_w(avimuxin->pin.pin.pinInfo.achName),
            debugstr_guid(riid), ppv);

    if(IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_IPin))
        *ppv = &avimuxin->pin.pin.IPin_iface;
    else if(IsEqualIID(riid, &IID_IAMStreamControl))
        *ppv = &avimuxin->IAMStreamControl_iface;
    else if(IsEqualIID(riid, &IID_IMemInputPin))
        *ppv = &avimuxin->pin.IMemInputPin_iface;
    else if(IsEqualIID(riid, &IID_IPropertyBag))
        *ppv = &avimuxin->IPropertyBag_iface;
    else if(IsEqualIID(riid, &IID_IQualityControl))
        *ppv = &avimuxin->IQualityControl_iface;
    else {
        FIXME("no interface for %s\n", debugstr_guid(riid));
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI AviMuxIn_AddRef(IPin *iface)
{
    AviMux *This = impl_from_in_IPin(iface);
    return IBaseFilter_AddRef(&This->filter.IBaseFilter_iface);
}

static ULONG WINAPI AviMuxIn_Release(IPin *iface)
{
    AviMux *This = impl_from_in_IPin(iface);
    return IBaseFilter_Release(&This->filter.IBaseFilter_iface);
}

static HRESULT WINAPI AviMuxIn_Connect(IPin *iface,
        IPin *pReceivePin, const AM_MEDIA_TYPE *pmt)
{
    AviMux *This = impl_from_in_IPin(iface);
    AviMuxIn *avimuxin = AviMuxIn_from_IPin(iface);
    TRACE("(%p:%s)->(%p AM_MEDIA_TYPE(%p))\n", This,
            debugstr_w(avimuxin->pin.pin.pinInfo.achName), pReceivePin, pmt);
    dump_AM_MEDIA_TYPE(pmt);
    return BaseInputPinImpl_Connect(iface, pReceivePin, pmt);
}

static HRESULT WINAPI AviMuxIn_ReceiveConnection(IPin *iface,
        IPin *pConnector, const AM_MEDIA_TYPE *pmt)
{
    AviMux *This = impl_from_in_IPin(iface);
    AviMuxIn *avimuxin = AviMuxIn_from_IPin(iface);
    HRESULT hr;

    TRACE("(%p:%s)->(%p AM_MEDIA_TYPE(%p))\n", This,
            debugstr_w(avimuxin->pin.pin.pinInfo.achName), pConnector, pmt);
    dump_AM_MEDIA_TYPE(pmt);

    if(!pmt)
        return E_POINTER;

    hr = BaseInputPinImpl_ReceiveConnection(iface, pConnector, pmt);
    if(FAILED(hr))
        return hr;

    if(IsEqualIID(&pmt->majortype, &MEDIATYPE_Video) &&
            IsEqualIID(&pmt->formattype, &FORMAT_VideoInfo)) {
        ALLOCATOR_PROPERTIES req, act;
        VIDEOINFOHEADER *vih;
        int size;

        vih = (VIDEOINFOHEADER*)pmt->pbFormat;
        avimuxin->strh.fcc = ckidSTREAMHEADER;
        avimuxin->strh.cb = sizeof(AVISTREAMHEADER) - FIELD_OFFSET(AVISTREAMHEADER, fccType);
        avimuxin->strh.fccType = streamtypeVIDEO;
        /* FIXME: fccHandler should be set differently */
        avimuxin->strh.fccHandler = vih->bmiHeader.biCompression ?
            vih->bmiHeader.biCompression : FCC('D','I','B',' ');
        avimuxin->avg_time_per_frame = vih->AvgTimePerFrame;
        avimuxin->stop = -1;

        req.cBuffers = 32;
        req.cbBuffer = vih->bmiHeader.biSizeImage;
        req.cbAlign = 1;
        req.cbPrefix = sizeof(void*);
        hr = IMemAllocator_SetProperties(avimuxin->samples_allocator, &req, &act);
        if(SUCCEEDED(hr))
            hr = IMemAllocator_Commit(avimuxin->samples_allocator);
        if(FAILED(hr)) {
            BasePinImpl_Disconnect(iface);
            return hr;
        }

        size = pmt->cbFormat - FIELD_OFFSET(VIDEOINFOHEADER, bmiHeader);
        avimuxin->strf = CoTaskMemAlloc(sizeof(RIFFCHUNK) + ALIGN(FIELD_OFFSET(BITMAPINFO, bmiColors[vih->bmiHeader.biClrUsed])));
        avimuxin->strf->fcc = ckidSTREAMFORMAT;
        avimuxin->strf->cb = FIELD_OFFSET(BITMAPINFO, bmiColors[vih->bmiHeader.biClrUsed]);
        if(size > avimuxin->strf->cb)
            size = avimuxin->strf->cb;
        memcpy(avimuxin->strf->data, &vih->bmiHeader, size);
    }else {
        FIXME("format not supported: %s %s\n", debugstr_guid(&pmt->majortype),
                debugstr_guid(&pmt->formattype));
        return E_NOTIMPL;
    }

    return create_input_pin(This);
}

static HRESULT WINAPI AviMuxIn_Disconnect(IPin *iface)
{
    AviMux *This = impl_from_in_IPin(iface);
    AviMuxIn *avimuxin = AviMuxIn_from_IPin(iface);
    IMediaSample **prev, *cur;
    HRESULT hr;

    TRACE("(%p:%s)\n", This, debugstr_w(avimuxin->pin.pin.pinInfo.achName));

    hr = BasePinImpl_Disconnect(iface);
    if(FAILED(hr))
        return hr;

    IMemAllocator_Decommit(avimuxin->samples_allocator);
    while(avimuxin->samples_head) {
        cur = avimuxin->samples_head;
        hr = IMediaSample_GetPointer(cur, (BYTE**)&prev);
        if(FAILED(hr))
            break;
        prev--;

        cur = avimuxin->samples_head;
        avimuxin->samples_head = *prev;
        IMediaSample_Release(cur);

        if(cur == avimuxin->samples_head)
            avimuxin->samples_head = NULL;
    }
    CoTaskMemFree(avimuxin->strf);
    avimuxin->strf = NULL;
    return hr;
}

static HRESULT WINAPI AviMuxIn_ConnectedTo(IPin *iface, IPin **pPin)
{
    AviMux *This = impl_from_in_IPin(iface);
    AviMuxIn *avimuxin = AviMuxIn_from_IPin(iface);
    TRACE("(%p:%s)->(%p)\n", This, debugstr_w(avimuxin->pin.pin.pinInfo.achName), pPin);
    return BasePinImpl_ConnectedTo(iface, pPin);
}

static HRESULT WINAPI AviMuxIn_ConnectionMediaType(IPin *iface, AM_MEDIA_TYPE *pmt)
{
    AviMux *This = impl_from_in_IPin(iface);
    AviMuxIn *avimuxin = AviMuxIn_from_IPin(iface);
    TRACE("(%p:%s)->(%p)\n", This, debugstr_w(avimuxin->pin.pin.pinInfo.achName), pmt);
    return BasePinImpl_ConnectionMediaType(iface, pmt);
}

static HRESULT WINAPI AviMuxIn_QueryPinInfo(IPin *iface, PIN_INFO *pInfo)
{
    AviMux *This = impl_from_in_IPin(iface);
    AviMuxIn *avimuxin = AviMuxIn_from_IPin(iface);
    TRACE("(%p:%s)->(%p)\n", This, debugstr_w(avimuxin->pin.pin.pinInfo.achName), pInfo);
    return BasePinImpl_QueryPinInfo(iface, pInfo);
}

static HRESULT WINAPI AviMuxIn_QueryDirection(IPin *iface, PIN_DIRECTION *pPinDir)
{
    AviMux *This = impl_from_in_IPin(iface);
    AviMuxIn *avimuxin = AviMuxIn_from_IPin(iface);
    TRACE("(%p:%s)->(%p)\n", This, debugstr_w(avimuxin->pin.pin.pinInfo.achName), pPinDir);
    return BasePinImpl_QueryDirection(iface, pPinDir);
}

static HRESULT WINAPI AviMuxIn_QueryId(IPin *iface, LPWSTR *Id)
{
    AviMux *This = impl_from_in_IPin(iface);
    AviMuxIn *avimuxin = AviMuxIn_from_IPin(iface);
    TRACE("(%p:%s)->(%p)\n", This, debugstr_w(avimuxin->pin.pin.pinInfo.achName), Id);
    return BasePinImpl_QueryId(iface, Id);
}

static HRESULT WINAPI AviMuxIn_QueryAccept(IPin *iface, const AM_MEDIA_TYPE *pmt)
{
    AviMux *This = impl_from_in_IPin(iface);
    AviMuxIn *avimuxin = AviMuxIn_from_IPin(iface);
    TRACE("(%p:%s)->(AM_MEDIA_TYPE(%p))\n", This,
            debugstr_w(avimuxin->pin.pin.pinInfo.achName), pmt);
    dump_AM_MEDIA_TYPE(pmt);
    return BasePinImpl_QueryAccept(iface, pmt);
}

static HRESULT WINAPI AviMuxIn_EnumMediaTypes(IPin *iface, IEnumMediaTypes **ppEnum)
{
    AviMux *This = impl_from_in_IPin(iface);
    AviMuxIn *avimuxin = AviMuxIn_from_IPin(iface);
    TRACE("(%p:%s)->(%p)\n", This, debugstr_w(avimuxin->pin.pin.pinInfo.achName), ppEnum);
    return BasePinImpl_EnumMediaTypes(iface, ppEnum);
}

static HRESULT WINAPI AviMuxIn_QueryInternalConnections(
        IPin *iface, IPin **apPin, ULONG *nPin)
{
    AviMux *This = impl_from_in_IPin(iface);
    AviMuxIn *avimuxin = AviMuxIn_from_IPin(iface);
    TRACE("(%p:%s)->(%p %p)\n", This, debugstr_w(avimuxin->pin.pin.pinInfo.achName), apPin, nPin);
    return BasePinImpl_QueryInternalConnections(iface, apPin, nPin);
}

static HRESULT WINAPI AviMuxIn_EndOfStream(IPin *iface)
{
    AviMux *This = impl_from_in_IPin(iface);
    AviMuxIn *avimuxin = AviMuxIn_from_IPin(iface);
    TRACE("(%p:%s)\n", This, debugstr_w(avimuxin->pin.pin.pinInfo.achName));
    return BaseInputPinImpl_EndOfStream(iface);
}

static HRESULT WINAPI AviMuxIn_BeginFlush(IPin *iface)
{
    AviMux *This = impl_from_in_IPin(iface);
    AviMuxIn *avimuxin = AviMuxIn_from_IPin(iface);
    TRACE("(%p:%s)\n", This, debugstr_w(avimuxin->pin.pin.pinInfo.achName));
    return BaseInputPinImpl_BeginFlush(iface);
}

static HRESULT WINAPI AviMuxIn_EndFlush(IPin *iface)
{
    AviMux *This = impl_from_in_IPin(iface);
    AviMuxIn *avimuxin = AviMuxIn_from_IPin(iface);
    TRACE("(%p:%s)\n", This, debugstr_w(avimuxin->pin.pin.pinInfo.achName));
    return BaseInputPinImpl_EndFlush(iface);
}

static HRESULT WINAPI AviMuxIn_NewSegment(IPin *iface, REFERENCE_TIME tStart,
        REFERENCE_TIME tStop, double dRate)
{
    AviMux *This = impl_from_in_IPin(iface);
    AviMuxIn *avimuxin = AviMuxIn_from_IPin(iface);
    TRACE("(%p:%s)->(%s %s %f)\n", This, debugstr_w(avimuxin->pin.pin.pinInfo.achName),
         wine_dbgstr_longlong(tStart), wine_dbgstr_longlong(tStop), dRate);
    return BasePinImpl_NewSegment(iface, tStart, tStop, dRate);
}

static const IPinVtbl AviMuxIn_PinVtbl = {
    AviMuxIn_QueryInterface,
    AviMuxIn_AddRef,
    AviMuxIn_Release,
    AviMuxIn_Connect,
    AviMuxIn_ReceiveConnection,
    AviMuxIn_Disconnect,
    AviMuxIn_ConnectedTo,
    AviMuxIn_ConnectionMediaType,
    AviMuxIn_QueryPinInfo,
    AviMuxIn_QueryDirection,
    AviMuxIn_QueryId,
    AviMuxIn_QueryAccept,
    AviMuxIn_EnumMediaTypes,
    AviMuxIn_QueryInternalConnections,
    AviMuxIn_EndOfStream,
    AviMuxIn_BeginFlush,
    AviMuxIn_EndFlush,
    AviMuxIn_NewSegment
};

static inline AviMuxIn* AviMuxIn_from_IAMStreamControl(IAMStreamControl *iface)
{
    return CONTAINING_RECORD(iface, AviMuxIn, IAMStreamControl_iface);
}

static HRESULT WINAPI AviMuxIn_AMStreamControl_QueryInterface(
        IAMStreamControl *iface, REFIID riid, void **ppv)
{
    AviMuxIn *avimuxin = AviMuxIn_from_IAMStreamControl(iface);
    return IPin_QueryInterface(&avimuxin->pin.pin.IPin_iface, riid, ppv);
}

static ULONG WINAPI AviMuxIn_AMStreamControl_AddRef(IAMStreamControl *iface)
{
    AviMuxIn *avimuxin = AviMuxIn_from_IAMStreamControl(iface);
    AviMux *This = impl_from_in_IPin(&avimuxin->pin.pin.IPin_iface);
    return IBaseFilter_AddRef(&This->filter.IBaseFilter_iface);
}

static ULONG WINAPI AviMuxIn_AMStreamControl_Release(IAMStreamControl *iface)
{
    AviMuxIn *avimuxin = AviMuxIn_from_IAMStreamControl(iface);
    AviMux *This = impl_from_in_IPin(&avimuxin->pin.pin.IPin_iface);
    return IBaseFilter_Release(&This->filter.IBaseFilter_iface);
}

static HRESULT WINAPI AviMuxIn_AMStreamControl_StartAt(IAMStreamControl *iface,
        const REFERENCE_TIME *ptStart, DWORD dwCookie)
{
    AviMuxIn *avimuxin = AviMuxIn_from_IAMStreamControl(iface);
    AviMux *This = impl_from_in_IPin(&avimuxin->pin.pin.IPin_iface);
    FIXME("(%p:%s)->(%p %x)\n", This,
            debugstr_w(avimuxin->pin.pin.pinInfo.achName), ptStart, dwCookie);
    return E_NOTIMPL;
}

static HRESULT WINAPI AviMuxIn_AMStreamControl_StopAt(IAMStreamControl *iface,
        const REFERENCE_TIME *ptStop, BOOL bSendExtra, DWORD dwCookie)
{
    AviMuxIn *avimuxin = AviMuxIn_from_IAMStreamControl(iface);
    AviMux *This = impl_from_in_IPin(&avimuxin->pin.pin.IPin_iface);
    FIXME("(%p:%s)->(%p %x %x)\n", This, debugstr_w(avimuxin->pin.pin.pinInfo.achName),
            ptStop, bSendExtra, dwCookie);
    return E_NOTIMPL;
}

static HRESULT WINAPI AviMuxIn_AMStreamControl_GetInfo(
        IAMStreamControl *iface, AM_STREAM_INFO *pInfo)
{
    AviMuxIn *avimuxin = AviMuxIn_from_IAMStreamControl(iface);
    AviMux *This = impl_from_in_IPin(&avimuxin->pin.pin.IPin_iface);
    FIXME("(%p:%s)->(%p)\n", This, debugstr_w(avimuxin->pin.pin.pinInfo.achName), pInfo);
    return E_NOTIMPL;
}

static const IAMStreamControlVtbl AviMuxIn_AMStreamControlVtbl = {
    AviMuxIn_AMStreamControl_QueryInterface,
    AviMuxIn_AMStreamControl_AddRef,
    AviMuxIn_AMStreamControl_Release,
    AviMuxIn_AMStreamControl_StartAt,
    AviMuxIn_AMStreamControl_StopAt,
    AviMuxIn_AMStreamControl_GetInfo
};

static inline AviMuxIn* AviMuxIn_from_IMemInputPin(IMemInputPin *iface)
{
    BaseInputPin *bip = CONTAINING_RECORD(iface, BaseInputPin, IMemInputPin_iface);
    return CONTAINING_RECORD(bip, AviMuxIn, pin);
}

static HRESULT WINAPI AviMuxIn_MemInputPin_QueryInterface(
        IMemInputPin *iface, REFIID riid, void **ppv)
{
    AviMuxIn *avimuxin = AviMuxIn_from_IMemInputPin(iface);
    return IPin_QueryInterface(&avimuxin->pin.pin.IPin_iface, riid, ppv);
}

static ULONG WINAPI AviMuxIn_MemInputPin_AddRef(IMemInputPin *iface)
{
    AviMuxIn *avimuxin = AviMuxIn_from_IMemInputPin(iface);
    AviMux *This = impl_from_in_IPin(&avimuxin->pin.pin.IPin_iface);
    return IBaseFilter_AddRef(&This->filter.IBaseFilter_iface);
}

static ULONG WINAPI AviMuxIn_MemInputPin_Release(IMemInputPin *iface)
{
    AviMuxIn *avimuxin = AviMuxIn_from_IMemInputPin(iface);
    AviMux *This = impl_from_in_IPin(&avimuxin->pin.pin.IPin_iface);
    return IBaseFilter_Release(&This->filter.IBaseFilter_iface);
}

static HRESULT WINAPI AviMuxIn_MemInputPin_GetAllocator(
        IMemInputPin *iface, IMemAllocator **ppAllocator)
{
    AviMuxIn *avimuxin = AviMuxIn_from_IMemInputPin(iface);
    AviMux *This = impl_from_in_IPin(&avimuxin->pin.pin.IPin_iface);

    TRACE("(%p:%s)->(%p)\n", This, debugstr_w(avimuxin->pin.pin.pinInfo.achName), ppAllocator);

    if(!ppAllocator)
        return E_POINTER;

    IMemAllocator_AddRef(avimuxin->pin.pAllocator);
    *ppAllocator = avimuxin->pin.pAllocator;
    return S_OK;
}

static HRESULT WINAPI AviMuxIn_MemInputPin_NotifyAllocator(
        IMemInputPin *iface, IMemAllocator *pAllocator, BOOL bReadOnly)
{
    AviMuxIn *avimuxin = AviMuxIn_from_IMemInputPin(iface);
    AviMux *This = impl_from_in_IPin(&avimuxin->pin.pin.IPin_iface);
    ALLOCATOR_PROPERTIES props;
    HRESULT hr;

    TRACE("(%p:%s)->(%p %x)\n", This, debugstr_w(avimuxin->pin.pin.pinInfo.achName),
            pAllocator, bReadOnly);

    if(!pAllocator)
        return E_POINTER;

    memset(&props, 0, sizeof(props));
    hr = IMemAllocator_GetProperties(pAllocator, &props);
    if(FAILED(hr))
        return hr;

    props.cbAlign = 1;
    props.cbPrefix = 8;
    return IMemAllocator_SetProperties(avimuxin->pin.pAllocator, &props, &props);
}

static HRESULT WINAPI AviMuxIn_MemInputPin_GetAllocatorRequirements(
        IMemInputPin *iface, ALLOCATOR_PROPERTIES *pProps)
{
    AviMuxIn *avimuxin = AviMuxIn_from_IMemInputPin(iface);
    AviMux *This = impl_from_in_IPin(&avimuxin->pin.pin.IPin_iface);

    TRACE("(%p:%s)->(%p)\n", This, debugstr_w(avimuxin->pin.pin.pinInfo.achName), pProps);

    if(!pProps)
        return E_POINTER;

    pProps->cbAlign = 1;
    pProps->cbPrefix = 8;
    return S_OK;
}

static HRESULT WINAPI AviMuxIn_MemInputPin_Receive(
        IMemInputPin *iface, IMediaSample *pSample)
{
    AviMuxIn *avimuxin = AviMuxIn_from_IMemInputPin(iface);
    AviMux *This = impl_from_in_IPin(&avimuxin->pin.pin.IPin_iface);

    TRACE("(%p:%s)->(%p)\n", This, debugstr_w(avimuxin->pin.pin.pinInfo.achName), pSample);

    return avimuxin->pin.pFuncsTable->pfnReceive(&avimuxin->pin, pSample);
}

static HRESULT WINAPI AviMuxIn_MemInputPin_ReceiveMultiple(IMemInputPin *iface,
        IMediaSample **pSamples, LONG nSamples, LONG *nSamplesProcessed)
{
    AviMuxIn *avimuxin = AviMuxIn_from_IMemInputPin(iface);
    AviMux *This = impl_from_in_IPin(&avimuxin->pin.pin.IPin_iface);
    HRESULT hr = S_OK;

    TRACE("(%p:%s)->(%p %d %p)\n", This, debugstr_w(avimuxin->pin.pin.pinInfo.achName),
            pSamples, nSamples, nSamplesProcessed);

    for(*nSamplesProcessed=0; *nSamplesProcessed<nSamples; (*nSamplesProcessed)++)
    {
        hr = avimuxin->pin.pFuncsTable->pfnReceive(&avimuxin->pin, pSamples[*nSamplesProcessed]);
        if(hr != S_OK)
            break;
    }

    return hr;
}

static HRESULT WINAPI AviMuxIn_MemInputPin_ReceiveCanBlock(IMemInputPin *iface)
{
    AviMuxIn *avimuxin = AviMuxIn_from_IMemInputPin(iface);
    AviMux *This = impl_from_in_IPin(&avimuxin->pin.pin.IPin_iface);
    HRESULT hr;

    TRACE("(%p:%s)\n", This, debugstr_w(avimuxin->pin.pin.pinInfo.achName));

    if(!This->out->pin.pMemInputPin)
        return S_FALSE;

    hr = IMemInputPin_ReceiveCanBlock(This->out->pin.pMemInputPin);
    return hr != S_FALSE ? S_OK : S_FALSE;
}

static const IMemInputPinVtbl AviMuxIn_MemInputPinVtbl = {
    AviMuxIn_MemInputPin_QueryInterface,
    AviMuxIn_MemInputPin_AddRef,
    AviMuxIn_MemInputPin_Release,
    AviMuxIn_MemInputPin_GetAllocator,
    AviMuxIn_MemInputPin_NotifyAllocator,
    AviMuxIn_MemInputPin_GetAllocatorRequirements,
    AviMuxIn_MemInputPin_Receive,
    AviMuxIn_MemInputPin_ReceiveMultiple,
    AviMuxIn_MemInputPin_ReceiveCanBlock
};

static inline AviMuxIn* AviMuxIn_from_IPropertyBag(IPropertyBag *iface)
{
    return CONTAINING_RECORD(iface, AviMuxIn, IPropertyBag_iface);
}

static HRESULT WINAPI AviMuxIn_PropertyBag_QueryInterface(
        IPropertyBag *iface, REFIID riid, void **ppv)
{
    AviMuxIn *avimuxin = AviMuxIn_from_IPropertyBag(iface);
    return IPin_QueryInterface(&avimuxin->pin.pin.IPin_iface, riid, ppv);
}

static ULONG WINAPI AviMuxIn_PropertyBag_AddRef(IPropertyBag *iface)
{
    AviMuxIn *avimuxin = AviMuxIn_from_IPropertyBag(iface);
    AviMux *This = impl_from_in_IPin(&avimuxin->pin.pin.IPin_iface);
    return IBaseFilter_AddRef(&This->filter.IBaseFilter_iface);
}

static ULONG WINAPI AviMuxIn_PropertyBag_Release(IPropertyBag *iface)
{
    AviMuxIn *avimuxin = AviMuxIn_from_IPropertyBag(iface);
    AviMux *This = impl_from_in_IPin(&avimuxin->pin.pin.IPin_iface);
    return IBaseFilter_Release(&This->filter.IBaseFilter_iface);
}

static HRESULT WINAPI AviMuxIn_PropertyBag_Read(IPropertyBag *iface,
        LPCOLESTR pszPropName, VARIANT *pVar, IErrorLog *pErrorLog)
{
    AviMuxIn *avimuxin = AviMuxIn_from_IPropertyBag(iface);
    AviMux *This = impl_from_in_IPin(&avimuxin->pin.pin.IPin_iface);
    FIXME("(%p:%s)->(%s %p %p)\n", This, debugstr_w(avimuxin->pin.pin.pinInfo.achName),
            debugstr_w(pszPropName), pVar, pErrorLog);
    return E_NOTIMPL;
}

static HRESULT WINAPI AviMuxIn_PropertyBag_Write(IPropertyBag *iface,
        LPCOLESTR pszPropName, VARIANT *pVar)
{
    AviMuxIn *avimuxin = AviMuxIn_from_IPropertyBag(iface);
    AviMux *This = impl_from_in_IPin(&avimuxin->pin.pin.IPin_iface);
    FIXME("(%p:%s)->(%s %p)\n", This, debugstr_w(avimuxin->pin.pin.pinInfo.achName),
            debugstr_w(pszPropName), pVar);
    return E_NOTIMPL;
}

static const IPropertyBagVtbl AviMuxIn_PropertyBagVtbl = {
    AviMuxIn_PropertyBag_QueryInterface,
    AviMuxIn_PropertyBag_AddRef,
    AviMuxIn_PropertyBag_Release,
    AviMuxIn_PropertyBag_Read,
    AviMuxIn_PropertyBag_Write
};

static inline AviMuxIn* AviMuxIn_from_IQualityControl(IQualityControl *iface)
{
    return CONTAINING_RECORD(iface, AviMuxIn, IQualityControl_iface);
}

static HRESULT WINAPI AviMuxIn_QualityControl_QueryInterface(
        IQualityControl *iface, REFIID riid, void **ppv)
{
    AviMuxIn *avimuxin = AviMuxIn_from_IQualityControl(iface);
    return IPin_QueryInterface(&avimuxin->pin.pin.IPin_iface, riid, ppv);
}

static ULONG WINAPI AviMuxIn_QualityControl_AddRef(IQualityControl *iface)
{
    AviMuxIn *avimuxin = AviMuxIn_from_IQualityControl(iface);
    AviMux *This = impl_from_in_IPin(&avimuxin->pin.pin.IPin_iface);
    return IBaseFilter_AddRef(&This->filter.IBaseFilter_iface);
}

static ULONG WINAPI AviMuxIn_QualityControl_Release(IQualityControl *iface)
{
    AviMuxIn *avimuxin = AviMuxIn_from_IQualityControl(iface);
    AviMux *This = impl_from_in_IPin(&avimuxin->pin.pin.IPin_iface);
    return IBaseFilter_Release(&This->filter.IBaseFilter_iface);
}

static HRESULT WINAPI AviMuxIn_QualityControl_Notify(IQualityControl *iface,
        IBaseFilter *pSelf, Quality q)
{
    AviMuxIn *avimuxin = AviMuxIn_from_IQualityControl(iface);
    AviMux *This = impl_from_in_IPin(&avimuxin->pin.pin.IPin_iface);
    FIXME("(%p:%s)->(%p { 0x%x %u %s %s })\n", This, debugstr_w(avimuxin->pin.pin.pinInfo.achName), pSelf,
            q.Type, q.Proportion,
            wine_dbgstr_longlong(q.Late),
            wine_dbgstr_longlong(q.TimeStamp));
    return E_NOTIMPL;
}

static HRESULT WINAPI AviMuxIn_QualityControl_SetSink(
        IQualityControl *iface, IQualityControl *piqc)
{
    AviMuxIn *avimuxin = AviMuxIn_from_IQualityControl(iface);
    AviMux *This = impl_from_in_IPin(&avimuxin->pin.pin.IPin_iface);
    FIXME("(%p:%s)->(%p)\n", This, debugstr_w(avimuxin->pin.pin.pinInfo.achName), piqc);
    return E_NOTIMPL;
}

static const IQualityControlVtbl AviMuxIn_QualityControlVtbl = {
    AviMuxIn_QualityControl_QueryInterface,
    AviMuxIn_QualityControl_AddRef,
    AviMuxIn_QualityControl_Release,
    AviMuxIn_QualityControl_Notify,
    AviMuxIn_QualityControl_SetSink
};

static HRESULT create_input_pin(AviMux *avimux)
{
    static const WCHAR name[] = {'I','n','p','u','t',' ','0','0',0};
    PIN_INFO info;
    HRESULT hr;

    if(avimux->input_pin_no >= MAX_PIN_NO-1)
        return E_FAIL;

    info.dir = PINDIR_INPUT;
    info.pFilter = &avimux->filter.IBaseFilter_iface;
    memcpy(info.achName, name, sizeof(name));
    info.achName[7] = '0' + (avimux->input_pin_no+1) % 10;
    info.achName[6] = '0' + (avimux->input_pin_no+1) / 10;

    hr = BaseInputPin_Construct(&AviMuxIn_PinVtbl, sizeof(AviMuxIn), &info,
            &AviMuxIn_BaseInputFuncTable, &avimux->filter.csFilter, NULL, (IPin**)&avimux->in[avimux->input_pin_no]);
    if(FAILED(hr))
        return hr;
    avimux->in[avimux->input_pin_no]->pin.IMemInputPin_iface.lpVtbl = &AviMuxIn_MemInputPinVtbl;
    avimux->in[avimux->input_pin_no]->IAMStreamControl_iface.lpVtbl = &AviMuxIn_AMStreamControlVtbl;
    avimux->in[avimux->input_pin_no]->IPropertyBag_iface.lpVtbl = &AviMuxIn_PropertyBagVtbl;
    avimux->in[avimux->input_pin_no]->IQualityControl_iface.lpVtbl = &AviMuxIn_QualityControlVtbl;

    avimux->in[avimux->input_pin_no]->samples_head = NULL;
    hr = CoCreateInstance(&CLSID_MemoryAllocator, NULL, CLSCTX_INPROC_SERVER,
            &IID_IMemAllocator, (void**)&avimux->in[avimux->input_pin_no]->samples_allocator);
    if(FAILED(hr)) {
        BaseInputPinImpl_Release(&avimux->in[avimux->input_pin_no]->pin.pin.IPin_iface);
        return hr;
    }

    hr = CoCreateInstance(&CLSID_MemoryAllocator, NULL, CLSCTX_INPROC_SERVER,
            &IID_IMemAllocator, (void**)&avimux->in[avimux->input_pin_no]->pin.pAllocator);
    if(FAILED(hr)) {
        IMemAllocator_Release(avimux->in[avimux->input_pin_no]->samples_allocator);
        BaseInputPinImpl_Release(&avimux->in[avimux->input_pin_no]->pin.pin.IPin_iface);
        return hr;
    }

    avimux->in[avimux->input_pin_no]->stream_time = 0;
    memset(&avimux->in[avimux->input_pin_no]->strh, 0, sizeof(avimux->in[avimux->input_pin_no]->strh));
    avimux->in[avimux->input_pin_no]->strf = NULL;
    memset(&avimux->in[avimux->input_pin_no]->indx_data, 0, sizeof(avimux->in[avimux->input_pin_no]->indx_data));
    memset(&avimux->in[avimux->input_pin_no]->ix_data, 0, sizeof(avimux->in[avimux->input_pin_no]->ix_data));
    avimux->in[avimux->input_pin_no]->indx = (AVISUPERINDEX*)&avimux->in[avimux->input_pin_no]->indx_data;
    avimux->in[avimux->input_pin_no]->ix = (AVISTDINDEX*)avimux->in[avimux->input_pin_no]->ix_data;

    avimux->input_pin_no++;
    return S_OK;
}

IUnknown* WINAPI QCAP_createAVIMux(IUnknown *pUnkOuter, HRESULT *phr)
{
    static const WCHAR output_name[] = {'A','V','I',' ','O','u','t',0};

    AviMux *avimux;
    PIN_INFO info;
    HRESULT hr;

    TRACE("(%p)\n", pUnkOuter);

    if(pUnkOuter) {
        *phr = CLASS_E_NOAGGREGATION;
        return NULL;
    }

    avimux = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(AviMux));
    if(!avimux) {
        *phr = E_OUTOFMEMORY;
        return NULL;
    }

    BaseFilter_Init(&avimux->filter, &AviMuxVtbl, &CLSID_AviDest,
            (DWORD_PTR)(__FILE__ ": AviMux.csFilter"), &filter_func_table);
    avimux->IConfigAviMux_iface.lpVtbl = &ConfigAviMuxVtbl;
    avimux->IConfigInterleaving_iface.lpVtbl = &ConfigInterleavingVtbl;
    avimux->IMediaSeeking_iface.lpVtbl = &MediaSeekingVtbl;
    avimux->IPersistMediaPropertyBag_iface.lpVtbl = &PersistMediaPropertyBagVtbl;
    avimux->ISpecifyPropertyPages_iface.lpVtbl = &SpecifyPropertyPagesVtbl;

    info.dir = PINDIR_OUTPUT;
    info.pFilter = &avimux->filter.IBaseFilter_iface;
    lstrcpyW(info.achName, output_name);
    hr = BaseOutputPin_Construct(&AviMuxOut_PinVtbl, sizeof(AviMuxOut), &info,
            &AviMuxOut_BaseOutputFuncTable, &avimux->filter.csFilter, (IPin**)&avimux->out);
    if(FAILED(hr)) {
        BaseFilterImpl_Release(&avimux->filter.IBaseFilter_iface);
        HeapFree(GetProcessHeap(), 0, avimux);
        *phr = hr;
        return NULL;
    }
    avimux->out->IQualityControl_iface.lpVtbl = &AviMuxOut_QualityControlVtbl;
    avimux->out->cur_stream = 0;
    avimux->out->cur_time = 0;
    avimux->out->stream = NULL;

    hr = create_input_pin(avimux);
    if(FAILED(hr)) {
        BaseOutputPinImpl_Release(&avimux->out->pin.pin.IPin_iface);
        BaseFilterImpl_Release(&avimux->filter.IBaseFilter_iface);
        HeapFree(GetProcessHeap(), 0, avimux);
        *phr = hr;
        return NULL;
    }

    avimux->interleave = 10000000;

    ObjectRefCount(TRUE);
    *phr = S_OK;
    return (IUnknown*)&avimux->filter.IBaseFilter_iface;
}
