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

#include "qcap_private.h"
#include "vfw.h"
#include "aviriff.h"

WINE_DEFAULT_DEBUG_CHANNEL(quartz);

#define MAX_PIN_NO 128
#define AVISUPERINDEX_ENTRIES 2000
#define AVISTDINDEX_ENTRIES 4000
#define ALIGN(x) ((x+1)/2*2)

typedef struct {
    struct strmbase_sink pin;
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
    BYTE indx_data[FIELD_OFFSET(AVISUPERINDEX, aIndex[AVISUPERINDEX_ENTRIES])];

    /* movi chunk */
    int ix_off;
    AVISTDINDEX *ix;
    BYTE ix_data[FIELD_OFFSET(AVISTDINDEX, aIndex[AVISTDINDEX_ENTRIES])];

    IMediaSample *samples_head;
    IMemAllocator *samples_allocator;
} AviMuxIn;

typedef struct {
    struct strmbase_filter filter;
    IConfigAviMux IConfigAviMux_iface;
    IConfigInterleaving IConfigInterleaving_iface;
    IMediaSeeking IMediaSeeking_iface;
    IPersistMediaPropertyBag IPersistMediaPropertyBag_iface;
    ISpecifyPropertyPages ISpecifyPropertyPages_iface;

    InterleavingMode mode;
    REFERENCE_TIME interleave;
    REFERENCE_TIME preroll;

    struct strmbase_source source;
    IQualityControl IQualityControl_iface;

    int input_pin_no;
    AviMuxIn *in[MAX_PIN_NO-1];

    REFERENCE_TIME start, stop;
    AVIMAINHEADER avih;

    int idx1_entries;
    int idx1_size;
    AVIINDEXENTRY *idx1;

    int cur_stream;
    LONGLONG cur_time;

    int buf_pos;
    BYTE buf[65536];

    int movi_off;
    int out_pos;
    int size;
    IStream *stream;
} AviMux;

static HRESULT create_input_pin(AviMux*);

static inline AviMux* impl_from_strmbase_filter(struct strmbase_filter *filter)
{
    return CONTAINING_RECORD(filter, AviMux, filter);
}

static struct strmbase_pin *avi_mux_get_pin(struct strmbase_filter *iface, unsigned int index)
{
    AviMux *filter = impl_from_strmbase_filter(iface);

    if (!index)
        return &filter->source.pin;
    else if (index <= filter->input_pin_no)
        return &filter->in[index - 1]->pin.pin;
    return NULL;
}

static void avi_mux_destroy(struct strmbase_filter *iface)
{
    AviMux *filter = impl_from_strmbase_filter(iface);
    int i;

    strmbase_source_cleanup(&filter->source);

    for (i = 0; i < filter->input_pin_no; ++i)
    {
        IPin_Disconnect(&filter->in[i]->pin.pin.IPin_iface);
        IMemAllocator_Release(filter->in[i]->samples_allocator);
        filter->in[i]->samples_allocator = NULL;
        strmbase_sink_cleanup(&filter->in[i]->pin);
        free(filter->in[i]);
    }

    free(filter->idx1);
    strmbase_filter_cleanup(&filter->filter);
    free(filter);
}

static HRESULT avi_mux_query_interface(struct strmbase_filter *iface, REFIID iid, void **out)
{
    AviMux *filter = impl_from_strmbase_filter(iface);

    if (IsEqualGUID(iid, &IID_IConfigAviMux))
        *out = &filter->IConfigAviMux_iface;
    else if (IsEqualGUID(iid, &IID_IConfigInterleaving))
        *out = &filter->IConfigInterleaving_iface;
    else if (IsEqualGUID(iid, &IID_IMediaSeeking))
        *out = &filter->IMediaSeeking_iface;
    else if (IsEqualGUID(iid, &IID_IPersistMediaPropertyBag))
        *out = &filter->IPersistMediaPropertyBag_iface;
    else if (IsEqualGUID(iid, &IID_ISpecifyPropertyPages))
        *out = &filter->ISpecifyPropertyPages_iface;
    else
        return E_NOINTERFACE;

    IUnknown_AddRef((IUnknown *)*out);
    return S_OK;
}

static HRESULT out_flush(AviMux *This)
{
    ULONG written;
    HRESULT hr;

    if(!This->buf_pos)
        return S_OK;

    hr = IStream_Write(This->stream, This->buf, This->buf_pos, &written);
    if(FAILED(hr))
        return hr;
    if (written != This->buf_pos)
        return E_FAIL;

    This->buf_pos = 0;
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
    hr = IStream_Seek(This->stream, li, STREAM_SEEK_SET, NULL);
    if(FAILED(hr))
        return hr;

    This->out_pos = pos;
    if(This->out_pos > This->size)
        This->size = This->out_pos;
    return hr;
}

static HRESULT out_write(AviMux *This, const void *data, int size)
{
    int chunk_size;
    HRESULT hr;

    while(1) {
        if (size > sizeof(This->buf) - This->buf_pos)
            chunk_size = sizeof(This->buf) - This->buf_pos;
        else
            chunk_size = size;

        memcpy(This->buf + This->buf_pos, data, chunk_size);
        size -= chunk_size;
        data = (const BYTE*)data + chunk_size;
        This->buf_pos += chunk_size;
        This->out_pos += chunk_size;
        if (This->out_pos > This->size)
            This->size = This->out_pos;

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
        AVIINDEXENTRY *new_idx = realloc(avimux->idx1, sizeof(*avimux->idx1) * 2 * avimux->idx1_size);
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

    if (avimux->cur_stream != avimuxin->stream_id)
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

        if (avimuxin->stream_time + (closing ? 0 : avimuxin->strh.dwScale) > avimux->cur_time
                && !(flags & AM_SAMPLE_TIMEDISCONTINUITY))
        {
            if(closing)
                break;

            avimux->cur_stream++;
            if(avimux->cur_stream >= avimux->input_pin_no-1) {
                avimux->cur_time += avimux->interleave;
                avimux->cur_stream = 0;
            }
            avimuxin = avimux->in[avimux->cur_stream];
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

            avimuxin->ix_off = avimux->size;
            avimux->size += sizeof(avimuxin->ix_data);
        }

        if(*head_prev == avimuxin->samples_head)
            avimuxin->samples_head = NULL;
        else
            *head_prev = *prev;

        avimuxin->stream_time += avimuxin->strh.dwScale;
        avimuxin->strh.dwLength++;
        if(!(flags & AM_SAMPLE_TIMEDISCONTINUITY)) {
            if(!avimuxin->ix->qwBaseOffset)
                avimuxin->ix->qwBaseOffset = avimux->size;
            avimuxin->ix->aIndex[avimuxin->ix->nEntriesInUse].dwOffset =
                    avimux->size + sizeof(RIFFCHUNK) - avimuxin->ix->qwBaseOffset;

            hr = out_seek(avimux, avimux->size);
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
                avimux->idx1[avimux->idx1_entries-1].dwChunkOffset : avimux->size, size);
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

static HRESULT avi_mux_cleanup_stream(struct strmbase_filter *iface)
{
    AviMux *This = impl_from_strmbase_filter(iface);
    HRESULT hr;
    int i;

    if (This->stream)
    {
        AVIEXTHEADER dmlh;
        RIFFCHUNK rc;
        RIFFLIST rl;
        int idx1_off, empty_stream;

        empty_stream = This->cur_stream;
        for(i=empty_stream+1; ; i++) {
            if(i >= This->input_pin_no-1)
                i = 0;
            if(i == empty_stream)
                break;

            This->cur_stream = i;
            hr = flush_queue(This, This->in[This->cur_stream], TRUE);
            if(FAILED(hr))
                return hr;
        }

        idx1_off = This->size;
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
            if(!This->in[i]->pin.pin.peer)
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
        rl.cb = This->size - sizeof(RIFFCHUNK) - 2 * sizeof(int);
        rl.fccListType = FCC('A','V','I',' ');
        hr = out_write(This, &rl, sizeof(rl));
        if(FAILED(hr))
            return hr;

        rl.fcc = FCC('L','I','S','T');
        rl.cb = This->movi_off - sizeof(RIFFLIST) - sizeof(RIFFCHUNK);
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
            if(!This->in[i]->pin.pin.peer)
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

        rl.cb = idx1_off - This->movi_off - sizeof(RIFFCHUNK);
        rl.fccListType = FCC('m','o','v','i');
        out_write(This, &rl, sizeof(rl));
        out_flush(This);

        IStream_Release(This->stream);
        This->stream = NULL;
    }

    return S_OK;
}

static HRESULT avi_mux_init_stream(struct strmbase_filter *iface)
{
    AviMux *This = impl_from_strmbase_filter(iface);
    HRESULT hr;
    int i, stream_id;

    if(This->mode != INTERLEAVE_FULL) {
        FIXME("mode not supported (%d)\n", This->mode);
        return E_NOTIMPL;
    }

    for(i=0; i<This->input_pin_no; i++) {
        IMediaSeeking *ms;
        LONGLONG cur, stop;

        if(!This->in[i]->pin.pin.peer)
            continue;

        hr = IPin_QueryInterface(This->in[i]->pin.pin.peer,
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

    if (This->source.pMemInputPin)
    {
        hr = IMemInputPin_QueryInterface(This->source.pMemInputPin,
                &IID_IStream, (void **)&This->stream);
        if(FAILED(hr))
            return hr;
    }

    This->idx1_entries = 0;
    if(!This->idx1_size) {
        This->idx1_size = 1024;
        if (!(This->idx1 = malloc(sizeof(*This->idx1) * This->idx1_size)))
            return E_OUTOFMEMORY;
    }

    This->size = 3*sizeof(RIFFLIST) + sizeof(AVIMAINHEADER) + sizeof(AVIEXTHEADER);
    This->start = -1;
    This->stop = -1;
    memset(&This->avih, 0, sizeof(This->avih));
    for(i=0; i<This->input_pin_no; i++) {
        if(!This->in[i]->pin.pin.peer)
            continue;

        This->avih.dwStreams++;
        This->size += sizeof(RIFFLIST) + sizeof(AVISTREAMHEADER) + sizeof(RIFFCHUNK)
                + This->in[i]->strf->cb + sizeof(This->in[i]->indx_data);

        This->in[i]->strh.dwScale = MulDiv(This->in[i]->avg_time_per_frame, This->interleave, 10000000);
        This->in[i]->strh.dwRate = This->interleave;

        hr = IMemAllocator_Commit(This->in[i]->pin.pAllocator);
        if(FAILED(hr)) {
            if (This->stream)
            {
                IStream_Release(This->stream);
                This->stream = NULL;
            }
            return hr;
        }
    }

    This->movi_off = This->size;
    This->size += sizeof(RIFFLIST);

    idx1_add_entry(This, FCC('7','F','x','x'), 0, This->movi_off + sizeof(RIFFLIST), 0);

    stream_id = 0;
    for(i=0; i<This->input_pin_no; i++) {
        if(!This->in[i]->pin.pin.peer)
            continue;

        This->in[i]->ix_off = This->size;
        This->size += sizeof(This->in[i]->ix_data);
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

    This->buf_pos = 0;
    This->out_pos = 0;

    This->avih.fcc = ckidMAINAVIHEADER;
    This->avih.cb = sizeof(AVIMAINHEADER) - sizeof(RIFFCHUNK);
    /* TODO: Use first video stream */
    This->avih.dwMicroSecPerFrame = This->in[0]->avg_time_per_frame/10;
    This->avih.dwPaddingGranularity = 1;
    This->avih.dwFlags = AVIF_TRUSTCKTYPE | AVIF_HASINDEX;
    This->avih.dwWidth = ((BITMAPINFOHEADER*)This->in[0]->strf->data)->biWidth;
    This->avih.dwHeight = ((BITMAPINFOHEADER*)This->in[0]->strf->data)->biHeight;

    return S_OK;
}

static const struct strmbase_filter_ops filter_ops =
{
    .filter_get_pin = avi_mux_get_pin,
    .filter_destroy = avi_mux_destroy,
    .filter_query_interface = avi_mux_query_interface,
    .filter_init_stream = avi_mux_init_stream,
    .filter_cleanup_stream = avi_mux_cleanup_stream,
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
    FIXME("filter %p, index %ld, stub!\n", This, iStream);
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
        if(This->source.pin.peer) {
            HRESULT hr = IFilterGraph_Reconnect(This->filter.graph, &This->source.pin.IPin_iface);
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
    FIXME("(%p)->(%p %#lx %p %#lx)\n", This, pCurrent, dwCurrentFlags, pStop, dwStopFlags);
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

static inline AviMux *impl_from_source_pin(struct strmbase_pin *iface)
{
    return CONTAINING_RECORD(iface, AviMux, source.pin);
}

static HRESULT source_query_interface(struct strmbase_pin *iface, REFIID iid, void **out)
{
    AviMux *filter = impl_from_source_pin(iface);

    if (IsEqualGUID(iid, &IID_IQualityControl))
        *out = &filter->IQualityControl_iface;
    else
        return E_NOINTERFACE;

    IUnknown_AddRef((IUnknown *)*out);
    return S_OK;
}

static HRESULT source_query_accept(struct strmbase_pin *iface, const AM_MEDIA_TYPE *mt)
{
    if (!IsEqualGUID(&mt->majortype, &GUID_NULL) && !IsEqualGUID(&mt->majortype, &MEDIATYPE_Stream))
        return S_FALSE;
    if (!IsEqualGUID(&mt->subtype, &GUID_NULL) && !IsEqualGUID(&mt->subtype, &MEDIASUBTYPE_Avi))
        return S_FALSE;
    return S_OK;
}

static HRESULT WINAPI AviMuxOut_AttemptConnection(struct strmbase_source *iface,
        IPin *pReceivePin, const AM_MEDIA_TYPE *pmt)
{
    AviMux *filter = impl_from_source_pin(&iface->pin);
    PIN_DIRECTION dir;
    unsigned int i;
    HRESULT hr;

    hr = IPin_QueryDirection(pReceivePin, &dir);
    if(hr==S_OK && dir!=PINDIR_INPUT)
        return VFW_E_INVALID_DIRECTION;

    if (FAILED(hr = BaseOutputPinImpl_AttemptConnection(iface, pReceivePin, pmt)))
        return hr;

    for (i = 0; i < filter->input_pin_no; ++i)
    {
        if (!filter->in[i]->pin.pin.peer)
            continue;

        hr = IFilterGraph_Reconnect(filter->filter.graph, &filter->in[i]->pin.pin.IPin_iface);
        if (FAILED(hr))
        {
            IPin_Disconnect(&iface->pin.IPin_iface);
            break;
        }
    }

    return hr;
}

static HRESULT source_get_media_type(struct strmbase_pin *base, unsigned int iPosition, AM_MEDIA_TYPE *amt)
{
    TRACE("(%p)->(%d %p)\n", base, iPosition, amt);

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

static HRESULT WINAPI AviMuxOut_DecideAllocator(struct strmbase_source *base,
        IMemInputPin *pPin, IMemAllocator **pAlloc)
{
    ALLOCATOR_PROPERTIES req, actual;
    HRESULT hr;

    TRACE("(%p)->(%p %p)\n", base, pPin, pAlloc);

    if (FAILED(hr = CoCreateInstance(&CLSID_MemoryAllocator, NULL,
            CLSCTX_INPROC_SERVER, &IID_IMemAllocator, (void **)pAlloc)))
    {
        ERR("Failed to create allocator, hr %#lx.\n", hr);
        return hr;
    }

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

static const struct strmbase_source_ops source_ops =
{
    .base.pin_query_interface = source_query_interface,
    .base.pin_query_accept = source_query_accept,
    .base.pin_get_media_type = source_get_media_type,
    .pfnAttemptConnection = AviMuxOut_AttemptConnection,
    .pfnDecideAllocator = AviMuxOut_DecideAllocator,
};

static inline AviMux* impl_from_out_IQualityControl(IQualityControl *iface)
{
    return CONTAINING_RECORD(iface, AviMux, IQualityControl_iface);
}

static HRESULT WINAPI AviMuxOut_QualityControl_QueryInterface(
        IQualityControl *iface, REFIID riid, void **ppv)
{
    AviMux *This = impl_from_out_IQualityControl(iface);
    return IPin_QueryInterface(&This->source.pin.IPin_iface, riid, ppv);
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
        IBaseFilter *sender, Quality q)
{
    AviMux *filter = impl_from_out_IQualityControl(iface);

    FIXME("filter %p, sender %p, type %#x, proportion %ld, late %s, timestamp %s, stub!\n",
            filter, sender, q.Type, q.Proportion,
            wine_dbgstr_longlong(q.Late), wine_dbgstr_longlong(q.TimeStamp));

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

static inline AviMuxIn *impl_sink_from_strmbase_pin(struct strmbase_pin *iface)
{
    return CONTAINING_RECORD(iface, AviMuxIn, pin.pin);
}

static HRESULT sink_query_interface(struct strmbase_pin *iface, REFIID iid, void **out)
{
    AviMuxIn *pin = impl_sink_from_strmbase_pin(iface);

    if (IsEqualGUID(iid, &IID_IAMStreamControl))
        *out = &pin->IAMStreamControl_iface;
    else if (IsEqualGUID(iid, &IID_IMemInputPin))
        *out = &pin->pin.IMemInputPin_iface;
    else if (IsEqualGUID(iid, &IID_IPropertyBag))
        *out = &pin->IPropertyBag_iface;
    else if (IsEqualGUID(iid, &IID_IQualityControl))
        *out = &pin->IQualityControl_iface;
    else
        return E_NOINTERFACE;

    IUnknown_AddRef((IUnknown *)*out);
    return S_OK;
}

static HRESULT sink_query_accept(struct strmbase_pin *base, const AM_MEDIA_TYPE *pmt)
{
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

static HRESULT WINAPI AviMuxIn_Receive(struct strmbase_sink *base, IMediaSample *pSample)
{
    AviMux *avimux = impl_from_strmbase_filter(base->pin.filter);
    AviMuxIn *avimuxin = CONTAINING_RECORD(base, AviMuxIn, pin);
    REFERENCE_TIME start, stop;
    IMediaSample *sample;
    int frames_no;
    IMediaSample2 *ms2;
    BYTE *frame, *buf;
    DWORD max_size, size;
    DWORD flags;
    HRESULT hr;

    TRACE("pin %p, pSample %p.\n", avimuxin, pSample);

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

    if(!avimuxin->pin.pin.mt.bTemporalCompression)
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

static HRESULT avi_mux_sink_connect(struct strmbase_sink *iface, IPin *peer, const AM_MEDIA_TYPE *pmt)
{
    AviMuxIn *avimuxin = impl_sink_from_strmbase_pin(&iface->pin);
    AviMux *This = impl_from_strmbase_filter(iface->pin.filter);
    HRESULT hr;

    if(!pmt)
        return E_POINTER;

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
        if (FAILED(hr))
            return hr;

        size = pmt->cbFormat - FIELD_OFFSET(VIDEOINFOHEADER, bmiHeader);
        avimuxin->strf = malloc(sizeof(RIFFCHUNK) + ALIGN(FIELD_OFFSET(BITMAPINFO, bmiColors[vih->bmiHeader.biClrUsed])));
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

static void avi_mux_sink_disconnect(struct strmbase_sink *iface)
{
    AviMuxIn *avimuxin = impl_sink_from_strmbase_pin(&iface->pin);
    IMediaSample **prev, *cur;

    IMemAllocator_Decommit(avimuxin->samples_allocator);
    while(avimuxin->samples_head) {
        cur = avimuxin->samples_head;
        if (FAILED(IMediaSample_GetPointer(cur, (BYTE **)&prev)))
            break;
        prev--;

        cur = avimuxin->samples_head;
        avimuxin->samples_head = *prev;
        IMediaSample_Release(cur);

        if(cur == avimuxin->samples_head)
            avimuxin->samples_head = NULL;
    }
    free(avimuxin->strf);
    avimuxin->strf = NULL;
}

static const struct strmbase_sink_ops sink_ops =
{
    .base.pin_query_interface = sink_query_interface,
    .base.pin_query_accept = sink_query_accept,
    .pfnReceive = AviMuxIn_Receive,
    .sink_connect = avi_mux_sink_connect,
    .sink_disconnect = avi_mux_sink_disconnect,
};

static inline AviMux* impl_from_in_IPin(IPin *iface)
{
    struct strmbase_pin *pin = CONTAINING_RECORD(iface, struct strmbase_pin, IPin_iface);
    return impl_from_strmbase_filter(pin->filter);
}

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
        const REFERENCE_TIME *start, DWORD cookie)
{
    FIXME("iface %p, start %p, cookie %#lx, stub!\n", iface, start, cookie);
    return E_NOTIMPL;
}

static HRESULT WINAPI AviMuxIn_AMStreamControl_StopAt(IAMStreamControl *iface,
        const REFERENCE_TIME *stop, BOOL send_extra, DWORD cookie)
{
    FIXME("iface %p, stop %p, send_extra %d, cookie %#lx, stub!\n", iface, stop, send_extra, cookie);
    return E_NOTIMPL;
}

static HRESULT WINAPI AviMuxIn_AMStreamControl_GetInfo(IAMStreamControl *iface,
        AM_STREAM_INFO *info)
{
    FIXME("iface %p, info %p, stub!\n", iface, info);
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
    return CONTAINING_RECORD(iface, AviMuxIn, pin.IMemInputPin_iface);
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

    TRACE("pin %p, ppAllocator %p.\n", avimuxin, ppAllocator);

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
    ALLOCATOR_PROPERTIES props;
    HRESULT hr;

    TRACE("pin %p, pAllocator %p, bReadOnly %d.\n", avimuxin, pAllocator, bReadOnly);

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

    TRACE("pin %p, pProps %p.\n", avimuxin, pProps);

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

    TRACE("pin %p, pSample %p.\n", avimuxin, pSample);

    return avimuxin->pin.pFuncsTable->pfnReceive(&avimuxin->pin, pSample);
}

static HRESULT WINAPI AviMuxIn_MemInputPin_ReceiveMultiple(IMemInputPin *iface,
        IMediaSample **pSamples, LONG nSamples, LONG *nSamplesProcessed)
{
    AviMuxIn *avimuxin = AviMuxIn_from_IMemInputPin(iface);
    HRESULT hr = S_OK;

    TRACE("pin %p, pSamples %p, nSamples %ld, nSamplesProcessed %p.\n",
            avimuxin, pSamples, nSamples, nSamplesProcessed);

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

    TRACE("avimuxin %p.\n", avimuxin);

    if(!This->source.pMemInputPin)
        return S_FALSE;

    hr = IMemInputPin_ReceiveCanBlock(This->source.pMemInputPin);
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
        const WCHAR *name, VARIANT *value, IErrorLog *error_log)
{
    FIXME("iface %p, name %s, value %p, error_log %p, stub!\n",
            iface, debugstr_w(name), value, error_log);
    return E_NOTIMPL;
}

static HRESULT WINAPI AviMuxIn_PropertyBag_Write(IPropertyBag *iface,
        const WCHAR *name, VARIANT *value)
{
    FIXME("iface %p, name %s, value %s, stub!\n",
            iface, debugstr_w(name), debugstr_variant(value));
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
        IBaseFilter *filter, Quality q)
{
    FIXME("iface %p, filter %p, type %u, proportion %ld, late %s, timestamp %s, stub!\n",
            iface, filter, q.Type, q.Proportion, wine_dbgstr_longlong(q.Late),
            wine_dbgstr_longlong(q.TimeStamp));
    return E_NOTIMPL;
}

static HRESULT WINAPI AviMuxIn_QualityControl_SetSink(IQualityControl *iface, IQualityControl *sink)
{
    FIXME("iface %p, sink %p, stub!\n", iface, sink);
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
    AviMuxIn *object;
    WCHAR name[19];
    HRESULT hr;

    if(avimux->input_pin_no >= MAX_PIN_NO-1)
        return E_FAIL;

    swprintf(name, ARRAY_SIZE(name), L"Input %02u", avimux->input_pin_no + 1);

    if (!(object = calloc(1, sizeof(*object))))
        return E_OUTOFMEMORY;

    strmbase_sink_init(&object->pin, &avimux->filter, name, &sink_ops, NULL);
    object->pin.IMemInputPin_iface.lpVtbl = &AviMuxIn_MemInputPinVtbl;
    object->IAMStreamControl_iface.lpVtbl = &AviMuxIn_AMStreamControlVtbl;
    object->IPropertyBag_iface.lpVtbl = &AviMuxIn_PropertyBagVtbl;
    object->IQualityControl_iface.lpVtbl = &AviMuxIn_QualityControlVtbl;

    hr = CoCreateInstance(&CLSID_MemoryAllocator, NULL, CLSCTX_INPROC_SERVER,
            &IID_IMemAllocator, (void **)&object->samples_allocator);
    if (FAILED(hr))
    {
        strmbase_sink_cleanup(&object->pin);
        free(object);
        return hr;
    }

    hr = CoCreateInstance(&CLSID_MemoryAllocator, NULL, CLSCTX_INPROC_SERVER,
            &IID_IMemAllocator, (void **)&object->pin.pAllocator);
    if (FAILED(hr))
    {
        IMemAllocator_Release(object->samples_allocator);
        strmbase_sink_cleanup(&object->pin);
        free(object);
        return hr;
    }

    object->indx = (AVISUPERINDEX *)&object->indx_data;
    object->ix = (AVISTDINDEX *)object->ix_data;

    avimux->in[avimux->input_pin_no++] = object;
    return S_OK;
}

HRESULT avi_mux_create(IUnknown *outer, IUnknown **out)
{
    AviMux *avimux;
    HRESULT hr;

    if (!(avimux = calloc(1, sizeof(AviMux))))
        return E_OUTOFMEMORY;

    strmbase_filter_init(&avimux->filter, outer, &CLSID_AviDest, &filter_ops);
    avimux->IConfigAviMux_iface.lpVtbl = &ConfigAviMuxVtbl;
    avimux->IConfigInterleaving_iface.lpVtbl = &ConfigInterleavingVtbl;
    avimux->IMediaSeeking_iface.lpVtbl = &MediaSeekingVtbl;
    avimux->IPersistMediaPropertyBag_iface.lpVtbl = &PersistMediaPropertyBagVtbl;
    avimux->ISpecifyPropertyPages_iface.lpVtbl = &SpecifyPropertyPagesVtbl;

    strmbase_source_init(&avimux->source, &avimux->filter, L"AVI Out", &source_ops);
    avimux->IQualityControl_iface.lpVtbl = &AviMuxOut_QualityControlVtbl;
    avimux->cur_stream = 0;
    avimux->cur_time = 0;
    avimux->stream = NULL;

    hr = create_input_pin(avimux);
    if(FAILED(hr)) {
        strmbase_source_cleanup(&avimux->source);
        strmbase_filter_cleanup(&avimux->filter);
        free(avimux);
        return hr;
    }

    avimux->interleave = 10000000;

    TRACE("Created AVI mux %p.\n", avimux);
    *out = &avimux->filter.IUnknown_inner;
    return S_OK;
}
