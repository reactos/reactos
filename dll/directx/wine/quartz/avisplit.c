/*
 * AVI Splitter Filter
 *
 * Copyright 2003 Robert Shearman
 * Copyright 2004-2005 Christian Costa
 * Copyright 2008 Maarten Lankhorst
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
/* FIXME:
 * - Reference leaks, if they still exist
 * - Files without an index are not handled correctly yet.
 * - When stopping/starting, a sample is lost. This should be compensated by
 *   keeping track of previous index/position.
 * - Debugging channels are noisy at the moment, especially with thread
 *   related messages, however this is the only correct thing to do right now,
 *   since wine doesn't correctly handle all messages yet.
 */

#include "quartz_private.h"

#define TWOCCFromFOURCC(fcc) HIWORD(fcc)

/* four character codes used in AVI files */
#define ckidINFO       mmioFOURCC('I','N','F','O')
#define ckidREC        mmioFOURCC('R','E','C',' ')

typedef struct StreamData
{
    DWORD dwSampleSize;
    FLOAT fSamplesPerSec;
    DWORD dwLength;

    AVISTREAMHEADER streamheader;
    DWORD entries;
    AVISTDINDEX **stdindex;
    DWORD frames;
    BOOL seek;

    /* Position, in index units */
    DWORD pos, pos_next, index, index_next;

    /* Packet handling: a thread is created and waits on the packet event handle
     * On an event acquire the sample lock, addref the sample and set it to NULL,
     * then queue a new packet.
     */
    HANDLE thread, packet_queued;
    IMediaSample *sample;

    /* Amount of preroll samples for this stream */
    DWORD preroll;
} StreamData;

typedef struct AVISplitterImpl
{
    ParserImpl Parser;
    RIFFCHUNK CurrentChunk;
    LONGLONG CurrentChunkOffset; /* in media time */
    LONGLONG EndOfFile;
    AVIMAINHEADER AviHeader;
    AVIEXTHEADER ExtHeader;

    AVIOLDINDEX *oldindex;
    DWORD offset;

    StreamData *streams;
} AVISplitterImpl;

struct thread_args {
    AVISplitterImpl *This;
    DWORD stream;
};

static inline AVISplitterImpl *impl_from_IMediaSeeking( IMediaSeeking *iface )
{
    return CONTAINING_RECORD(iface, AVISplitterImpl, Parser.sourceSeeking.IMediaSeeking_iface);
}

/* The threading stuff cries for an explanation
 *
 * PullPin starts processing and calls AVISplitter_first_request
 * AVISplitter_first_request creates a thread for each stream
 * A stream can be audio, video, subtitles or something undefined.
 *
 * AVISplitter_first_request loads a single packet to each but one stream,
 * and queues it for that last stream. This is to prevent WaitForNext to time
 * out badly.
 *
 * The processing loop is entered. It calls IAsyncReader_WaitForNext in the
 * PullPin. Every time it receives a packet, it will call AVISplitter_Sample
 * AVISplitter_Sample will signal the relevant thread that a new sample is
 * arrived, when that thread is ready it will read the packet and transmits
 * it downstream with AVISplitter_Receive
 *
 * Threads terminate upon receiving NULL as packet or when ANY error code
 * != S_OK occurs. This means that any error is fatal to processing.
 */

static HRESULT AVISplitter_SendEndOfFile(AVISplitterImpl *This, DWORD streamnumber)
{
    IPin* ppin = NULL;
    HRESULT hr;

    TRACE("End of file reached\n");

    hr = IPin_ConnectedTo(This->Parser.ppPins[streamnumber+1], &ppin);
    if (SUCCEEDED(hr))
    {
        hr = IPin_EndOfStream(ppin);
        IPin_Release(ppin);
    }
    TRACE("--> %x\n", hr);

    /* Force the pullpin thread to stop */
    return S_FALSE;
}

/* Thread worker horse */
static HRESULT AVISplitter_next_request(AVISplitterImpl *This, DWORD streamnumber)
{
    StreamData *stream = This->streams + streamnumber;
    PullPin *pin = This->Parser.pInputPin;
    IMediaSample *sample = NULL;
    HRESULT hr;
    ULONG ref;

    TRACE("(%p, %u)->()\n", This, streamnumber);

    hr = IMemAllocator_GetBuffer(pin->pAlloc, &sample, NULL, NULL, 0);
    if (hr != S_OK)
        ERR("... %08x?\n", hr);

    if (SUCCEEDED(hr))
    {
        LONGLONG rtSampleStart;
        /* Add 4 for the next header, which should hopefully work */
        LONGLONG rtSampleStop;

        stream->pos = stream->pos_next;
        stream->index = stream->index_next;

        IMediaSample_SetDiscontinuity(sample, stream->seek);
        stream->seek = FALSE;
        if (stream->preroll)
        {
            --stream->preroll;
            IMediaSample_SetPreroll(sample, TRUE);
        }
        else
            IMediaSample_SetPreroll(sample, FALSE);
        IMediaSample_SetSyncPoint(sample, TRUE);

        if (stream->stdindex)
        {
            AVISTDINDEX *index = stream->stdindex[stream->index];
            AVISTDINDEX_ENTRY *entry = &index->aIndex[stream->pos];

            /* End of file */
            if (stream->index >= stream->entries)
            {
                TRACE("END OF STREAM ON %u\n", streamnumber);
                IMediaSample_Release(sample);
                return S_FALSE;
            }

            rtSampleStart = index->qwBaseOffset;
            rtSampleStart += entry->dwOffset;
            rtSampleStart = MEDIATIME_FROM_BYTES(rtSampleStart);

            ++stream->pos_next;
            if (index->nEntriesInUse == stream->pos_next)
            {
                stream->pos_next = 0;
                ++stream->index_next;
            }

            rtSampleStop = rtSampleStart + MEDIATIME_FROM_BYTES(entry->dwSize & ~(1 << 31));

            TRACE("offset(%u) size(%u)\n", (DWORD)BYTES_FROM_MEDIATIME(rtSampleStart), (DWORD)BYTES_FROM_MEDIATIME(rtSampleStop - rtSampleStart));
        }
        else if (This->oldindex)
        {
            DWORD flags = This->oldindex->aIndex[stream->pos].dwFlags;
            DWORD size = This->oldindex->aIndex[stream->pos].dwSize;

            /* End of file */
            if (stream->index)
            {
                TRACE("END OF STREAM ON %u\n", streamnumber);
                IMediaSample_Release(sample);
                return S_FALSE;
            }

            rtSampleStart = MEDIATIME_FROM_BYTES(This->offset);
            rtSampleStart += MEDIATIME_FROM_BYTES(This->oldindex->aIndex[stream->pos].dwOffset);
            rtSampleStop = rtSampleStart + MEDIATIME_FROM_BYTES(size);
            if (flags & AVIIF_MIDPART)
            {
                FIXME("Only stand alone frames are currently handled correctly!\n");
            }
            if (flags & AVIIF_LIST)
            {
                FIXME("Not sure if this is handled correctly\n");
                rtSampleStart += MEDIATIME_FROM_BYTES(sizeof(RIFFLIST));
                rtSampleStop += MEDIATIME_FROM_BYTES(sizeof(RIFFLIST));
            }
            else
            {
                rtSampleStart += MEDIATIME_FROM_BYTES(sizeof(RIFFCHUNK));
                rtSampleStop += MEDIATIME_FROM_BYTES(sizeof(RIFFCHUNK));
            }

            /* Slow way of finding next index */
            do {
                stream->pos_next++;
            } while (stream->pos_next * sizeof(This->oldindex->aIndex[0]) < This->oldindex->cb
                     && StreamFromFOURCC(This->oldindex->aIndex[stream->pos_next].dwChunkId) != streamnumber);

            /* End of file soon */
            if (stream->pos_next * sizeof(This->oldindex->aIndex[0]) >= This->oldindex->cb)
            {
                stream->pos_next = 0;
                ++stream->index_next;
            }
        }
        else /* TODO: Generate an index automagically */
        {
            ERR("CAN'T PLAY WITHOUT AN INDEX! SOS! SOS! SOS!\n");
            assert(0);
        }

        if (rtSampleStart != rtSampleStop)
        {
            IMediaSample_SetTime(sample, &rtSampleStart, &rtSampleStop);
            hr = IAsyncReader_Request(pin->pReader, sample, streamnumber);

            if (FAILED(hr))
            {
                ref = IMediaSample_Release(sample);
                assert(ref == 0);
            }
        }
        else
        {
            stream->sample = sample;
            IMediaSample_SetActualDataLength(sample, 0);
            SetEvent(stream->packet_queued);
        }
    }
    else
    {
        if (sample)
        {
            ERR("There should be no sample!\n");
            ref = IMediaSample_Release(sample);
            assert(ref == 0);
        }
    }
    TRACE("--> %08x\n", hr);

    return hr;
}

static HRESULT AVISplitter_Receive(AVISplitterImpl *This, IMediaSample *sample, DWORD streamnumber)
{
    Parser_OutputPin *pin = unsafe_impl_Parser_OutputPin_from_IPin(This->Parser.ppPins[1+streamnumber]);
    HRESULT hr;
    LONGLONG start, stop, rtstart, rtstop;
    StreamData *stream = &This->streams[streamnumber];

    start = pin->dwSamplesProcessed;
    start *= stream->streamheader.dwScale;
    start *= 10000000;
    start /= stream->streamheader.dwRate;

    if (stream->streamheader.dwSampleSize)
    {
        ULONG len = IMediaSample_GetActualDataLength(sample);
        ULONG size = stream->streamheader.dwSampleSize;

        pin->dwSamplesProcessed += len / size;
    }
    else
        ++pin->dwSamplesProcessed;

    stop = pin->dwSamplesProcessed;
    stop *= stream->streamheader.dwScale;
    stop *= 10000000;
    stop /= stream->streamheader.dwRate;

    if (IMediaSample_IsDiscontinuity(sample) == S_OK) {
        IPin *victim;
        EnterCriticalSection(&This->Parser.filter.csFilter);
        pin->pin.pin.tStart = start;
        pin->pin.pin.dRate = This->Parser.sourceSeeking.dRate;
        hr = IPin_ConnectedTo(&pin->pin.pin.IPin_iface, &victim);
        if (hr == S_OK)
        {
            hr = IPin_NewSegment(victim, start, This->Parser.sourceSeeking.llStop,
                                 This->Parser.sourceSeeking.dRate);
            if (hr != S_OK)
                FIXME("NewSegment returns %08x\n", hr);
            IPin_Release(victim);
        }
        LeaveCriticalSection(&This->Parser.filter.csFilter);
        if (hr != S_OK)
            return hr;
    }
    rtstart = (double)(start - pin->pin.pin.tStart) / pin->pin.pin.dRate;
    rtstop = (double)(stop - pin->pin.pin.tStart) / pin->pin.pin.dRate;
    IMediaSample_SetMediaTime(sample, &start, &stop);
    IMediaSample_SetTime(sample, &rtstart, &rtstop);
    IMediaSample_SetMediaTime(sample, &start, &stop);

    hr = BaseOutputPinImpl_Deliver(&pin->pin, sample);

/* Uncomment this if you want to debug the time differences between the
 * different streams, it is useful for that
 *
    FIXME("stream %u, hr: %08x, Start: %u.%03u, Stop: %u.%03u\n", streamnumber, hr,
           (DWORD)(start / 10000000), (DWORD)((start / 10000)%1000),
           (DWORD)(stop / 10000000), (DWORD)((stop / 10000)%1000));
*/
    return hr;
}

static DWORD WINAPI AVISplitter_thread_reader(LPVOID data)
{
    struct thread_args *args = data;
    AVISplitterImpl *This = args->This;
    DWORD streamnumber = args->stream;
    HRESULT hr = S_OK;

    do
    {
        HRESULT nexthr = S_FALSE;
        IMediaSample *sample;

        WaitForSingleObject(This->streams[streamnumber].packet_queued, INFINITE);
        sample = This->streams[streamnumber].sample;
        This->streams[streamnumber].sample = NULL;
        if (!sample)
            break;

        nexthr = AVISplitter_next_request(This, streamnumber);

        hr = AVISplitter_Receive(This, sample, streamnumber);
        if (hr != S_OK)
            FIXME("Receiving error: %08x\n", hr);

        IMediaSample_Release(sample);
        if (hr == S_OK)
            hr = nexthr;
        if (nexthr == S_FALSE)
            AVISplitter_SendEndOfFile(This, streamnumber);
    } while (hr == S_OK);

    if (hr != S_FALSE)
        FIXME("Thread %u terminated with hr %08x!\n", streamnumber, hr);
    else
        TRACE("Thread %u terminated properly\n", streamnumber);
    return hr;
}

static HRESULT AVISplitter_Sample(LPVOID iface, IMediaSample * pSample, DWORD_PTR cookie)
{
    AVISplitterImpl *This = iface;
    StreamData *stream = This->streams + cookie;
    HRESULT hr = S_OK;

    if (!IMediaSample_GetActualDataLength(pSample))
    {
        ERR("Received empty sample\n");
        return S_OK;
    }

    /* Send the sample to whatever thread is appropriate
     * That thread should also not have a sample queued at the moment
     */
    /* Debugging */
    TRACE("(%p)->(%p size: %u, %lu)\n", This, pSample, IMediaSample_GetActualDataLength(pSample), cookie);
    assert(cookie < This->Parser.cStreams);
    assert(!stream->sample);
    assert(WaitForSingleObject(stream->packet_queued, 0) == WAIT_TIMEOUT);

    IMediaSample_AddRef(pSample);

    stream->sample = pSample;
    SetEvent(stream->packet_queued);

    return hr;
}

static HRESULT AVISplitter_done_process(LPVOID iface);

/* On the first request we have to be sure that (cStreams-1) samples have
 * already been processed, because otherwise some pins might not ever finish
 * a Pause state change
 */
static HRESULT AVISplitter_first_request(LPVOID iface)
{
    AVISplitterImpl *This = iface;
    HRESULT hr = S_OK;
    DWORD x;
    IMediaSample *sample = NULL;
    BOOL have_sample = FALSE;

    TRACE("(%p)->()\n", This);

    for (x = 0; x < This->Parser.cStreams; ++x)
    {
        StreamData *stream = This->streams + x;

        /* Nothing should be running at this point */
        assert(!stream->thread);

        assert(!sample);
        /* It could be we asked the thread to terminate, and the thread
         * already terminated before receiving the deathwish */
        ResetEvent(stream->packet_queued);

        stream->pos_next = stream->pos;
        stream->index_next = stream->index;

        /* This was sent after stopped->paused or stopped->playing, so set seek */
        stream->seek = TRUE;

        /* There should be a packet queued from AVISplitter_next_request last time
         * It needs to be done now because this is the only way to ensure that every
         * stream will have at least 1 packet processed
         * If this is done after the threads start it could go all awkward and we
         * would have no guarantees that it's successful at all
         */

        if (have_sample)
        {
            DWORD_PTR dwUser = ~0;
            hr = IAsyncReader_WaitForNext(This->Parser.pInputPin->pReader, 10000, &sample, &dwUser);
            assert(hr == S_OK);
            assert(sample);

            AVISplitter_Sample(iface, sample, dwUser);
            IMediaSample_Release(sample);
        }

        hr = AVISplitter_next_request(This, x);
        TRACE("-->%08x\n", hr);

        /* Could be an EOF instead */
        have_sample = (hr == S_OK);
        if (hr == S_FALSE)
            AVISplitter_SendEndOfFile(This, x);

        if (FAILED(hr) && hr != VFW_E_NOT_CONNECTED)
            break;
        hr = S_OK;
    }

    /* FIXME: Don't do this for each pin that sent an EOF */
    for (x = 0; x < This->Parser.cStreams && SUCCEEDED(hr); ++x)
    {
        struct thread_args *args;
        DWORD tid;

        if ((This->streams[x].stdindex && This->streams[x].index_next >= This->streams[x].entries) ||
            (!This->streams[x].stdindex && This->streams[x].index_next))
        {
            This->streams[x].thread = NULL;
            continue;
        }

        args = CoTaskMemAlloc(sizeof(*args));
        args->This = This;
        args->stream = x;
        This->streams[x].thread = CreateThread(NULL, 0, AVISplitter_thread_reader, args, 0, &tid);
        TRACE("Created stream %u thread 0x%08x\n", x, tid);
    }

    if (FAILED(hr))
        ERR("Horsemen of the apocalypse came to bring error 0x%08x\n", hr);

    return hr;
}

static HRESULT AVISplitter_done_process(LPVOID iface)
{
    AVISplitterImpl *This = iface;
    DWORD x;
    ULONG ref;

    for (x = 0; x < This->Parser.cStreams; ++x)
    {
        StreamData *stream = This->streams + x;

        TRACE("Waiting for %u to terminate\n", x);
        /* Make the thread return first */
        SetEvent(stream->packet_queued);
        assert(WaitForSingleObject(stream->thread, 100000) != WAIT_TIMEOUT);
        CloseHandle(stream->thread);
        stream->thread = NULL;

        if (stream->sample)
        {
            ref = IMediaSample_Release(stream->sample);
            assert(ref == 0);
        }
        stream->sample = NULL;

        ResetEvent(stream->packet_queued);
    }
    TRACE("All threads are now terminated\n");

    return S_OK;
}

static HRESULT AVISplitter_QueryAccept(LPVOID iface, const AM_MEDIA_TYPE * pmt)
{
    if (IsEqualIID(&pmt->majortype, &MEDIATYPE_Stream) && IsEqualIID(&pmt->subtype, &MEDIASUBTYPE_Avi))
        return S_OK;
    return S_FALSE;
}

static HRESULT AVISplitter_ProcessIndex(AVISplitterImpl *This, AVISTDINDEX **index, LONGLONG qwOffset, DWORD cb)
{
    AVISTDINDEX *pIndex;
    DWORD x;
    int rest;

    *index = NULL;
    if (cb < sizeof(AVISTDINDEX))
    {
        FIXME("size %u too small\n", cb);
        return E_INVALIDARG;
    }

    pIndex = CoTaskMemAlloc(cb);
    if (!pIndex)
        return E_OUTOFMEMORY;

    IAsyncReader_SyncRead((impl_PullPin_from_IPin(This->Parser.ppPins[0]))->pReader, qwOffset, cb, (BYTE *)pIndex);
    rest = cb - sizeof(AVISUPERINDEX) + sizeof(RIFFCHUNK) + sizeof(pIndex->aIndex);

    TRACE("FOURCC: %s\n", debugstr_an((char *)&pIndex->fcc, 4));
    TRACE("wLongsPerEntry: %hd\n", pIndex->wLongsPerEntry);
    TRACE("bIndexSubType: %u\n", pIndex->bIndexSubType);
    TRACE("bIndexType: %u\n", pIndex->bIndexType);
    TRACE("nEntriesInUse: %u\n", pIndex->nEntriesInUse);
    TRACE("dwChunkId: %.4s\n", (char *)&pIndex->dwChunkId);
    TRACE("qwBaseOffset: %x%08x\n", (DWORD)(pIndex->qwBaseOffset >> 32), (DWORD)pIndex->qwBaseOffset);
    TRACE("dwReserved_3: %u\n", pIndex->dwReserved_3);

    if (pIndex->bIndexType != AVI_INDEX_OF_CHUNKS
        || pIndex->wLongsPerEntry != 2
        || rest < (pIndex->nEntriesInUse * sizeof(DWORD) * pIndex->wLongsPerEntry)
        || (pIndex->bIndexSubType != AVI_INDEX_SUB_DEFAULT))
    {
        FIXME("Invalid index chunk encountered: %u/%u, %u/%u, %u/%u, %u/%u\n",
              pIndex->bIndexType, AVI_INDEX_OF_CHUNKS, pIndex->wLongsPerEntry, 2,
              rest, (DWORD)(pIndex->nEntriesInUse * sizeof(DWORD) * pIndex->wLongsPerEntry),
              pIndex->bIndexSubType, AVI_INDEX_SUB_DEFAULT);
        *index = NULL;
        return E_INVALIDARG;
    }

    for (x = 0; x < pIndex->nEntriesInUse; ++x)
    {
        BOOL keyframe = !(pIndex->aIndex[x].dwSize >> 31);
        DWORDLONG offset = pIndex->qwBaseOffset + pIndex->aIndex[x].dwOffset;
        TRACE("dwOffset: %x%08x\n", (DWORD)(offset >> 32), (DWORD)offset);
        TRACE("dwSize: %u\n", (pIndex->aIndex[x].dwSize & ~(1<<31)));
        TRACE("Frame is a keyframe: %s\n", keyframe ? "yes" : "no");
    }

    *index = pIndex;
    return S_OK;
}

static HRESULT AVISplitter_ProcessOldIndex(AVISplitterImpl *This)
{
    ULONGLONG mov_pos = BYTES_FROM_MEDIATIME(This->CurrentChunkOffset) - sizeof(DWORD);
    AVIOLDINDEX *pAviOldIndex = This->oldindex;
    int relative = -1;
    DWORD x;

    for (x = 0; x < pAviOldIndex->cb / sizeof(pAviOldIndex->aIndex[0]); ++x)
    {
        DWORD temp, temp2 = 0, offset, chunkid;
        PullPin *pin = This->Parser.pInputPin;

        offset = pAviOldIndex->aIndex[x].dwOffset;
        chunkid = pAviOldIndex->aIndex[x].dwChunkId;

        TRACE("dwChunkId: %.4s\n", (char *)&chunkid);
        TRACE("dwFlags: %08x\n", pAviOldIndex->aIndex[x].dwFlags);
        TRACE("dwOffset (%s): %08x\n", relative ? "relative" : "absolute", offset);
        TRACE("dwSize: %08x\n", pAviOldIndex->aIndex[x].dwSize);

        /* Only scan once, or else this will take too long */
        if (relative == -1)
        {
            IAsyncReader_SyncRead(pin->pReader, offset, sizeof(DWORD), (BYTE *)&temp);
            relative = (chunkid != temp);

            if (chunkid == mmioFOURCC('7','F','x','x')
                && ((char *)&temp)[0] == 'i' && ((char *)&temp)[1] == 'x')
                relative = FALSE;

            if (relative)
            {
                if (offset + mov_pos < BYTES_FROM_MEDIATIME(This->EndOfFile))
                    IAsyncReader_SyncRead(pin->pReader, offset + mov_pos, sizeof(DWORD), (BYTE *)&temp2);

                if (chunkid == mmioFOURCC('7','F','x','x')
                    && ((char *)&temp2)[0] == 'i' && ((char *)&temp2)[1] == 'x')
                {
                    /* Do nothing, all is great */
                }
                else if (temp2 != chunkid)
                {
                    ERR("Faulty index or bug in handling: Wanted FCC: %s, Abs FCC: %s (@ %x), Rel FCC: %s (@ %.0x%08x)\n",
                        debugstr_an((char *)&chunkid, 4), debugstr_an((char *)&temp, 4), offset,
                        debugstr_an((char *)&temp2, 4), (DWORD)((mov_pos + offset) >> 32), (DWORD)(mov_pos + offset));
                    relative = -1;
                }
                else
                    TRACE("Scanned dwChunkId: %s\n", debugstr_an((char *)&temp2, 4));
            }
            else if (!relative)
                TRACE("Scanned dwChunkId: %s\n", debugstr_an((char *)&temp, 4));
        }
        /* Only dump one packet */
        else break;
    }

    if (relative == -1)
    {
        FIXME("Dropping index: no idea whether it is relative or absolute\n");
        CoTaskMemFree(This->oldindex);
        This->oldindex = NULL;
    }
    else if (!relative)
        This->offset = 0;
    else
        This->offset = (DWORD)mov_pos;

    return S_OK;
}

static HRESULT AVISplitter_ProcessStreamList(AVISplitterImpl * This, const BYTE * pData, DWORD cb, ALLOCATOR_PROPERTIES *props)
{
    PIN_INFO piOutput;
    const RIFFCHUNK * pChunk;
    HRESULT hr;
    AM_MEDIA_TYPE amt;
    float fSamplesPerSec = 0.0f;
    DWORD dwSampleSize = 0;
    DWORD dwLength = 0;
    DWORD nstdindex = 0;
    static const WCHAR wszStreamTemplate[] = {'S','t','r','e','a','m',' ','%','0','2','d',0};
    StreamData *stream;

    ZeroMemory(&amt, sizeof(amt));
    piOutput.dir = PINDIR_OUTPUT;
    piOutput.pFilter = (IBaseFilter *)This;
    wsprintfW(piOutput.achName, wszStreamTemplate, This->Parser.cStreams);
    This->streams = CoTaskMemRealloc(This->streams, sizeof(StreamData) * (This->Parser.cStreams+1));
    stream = This->streams + This->Parser.cStreams;
    ZeroMemory(stream, sizeof(*stream));

    for (pChunk = (const RIFFCHUNK *)pData; 
         ((const BYTE *)pChunk >= pData) && ((const BYTE *)pChunk + sizeof(RIFFCHUNK) < pData + cb) && (pChunk->cb > 0); 
         pChunk = (const RIFFCHUNK *)((const BYTE*)pChunk + sizeof(RIFFCHUNK) + pChunk->cb)     
        )
    {
        switch (pChunk->fcc)
        {
        case ckidSTREAMHEADER:
            {
                const AVISTREAMHEADER * pStrHdr = (const AVISTREAMHEADER *)pChunk;
                TRACE("processing stream header\n");
                stream->streamheader = *pStrHdr;

                fSamplesPerSec = (float)pStrHdr->dwRate / (float)pStrHdr->dwScale;
                CoTaskMemFree(amt.pbFormat);
                amt.pbFormat = NULL;
                amt.cbFormat = 0;

                switch (pStrHdr->fccType)
                {
                case streamtypeVIDEO:
                    amt.formattype = FORMAT_VideoInfo;
                    break;
                case streamtypeAUDIO:
                    amt.formattype = FORMAT_WaveFormatEx;
                    break;
                default:
                    FIXME("fccType %.4s not handled yet\n", (const char *)&pStrHdr->fccType);
                    amt.formattype = FORMAT_None;
                }
                amt.majortype = MEDIATYPE_Video;
                amt.majortype.Data1 = pStrHdr->fccType;
                amt.subtype = MEDIATYPE_Video;
                amt.subtype.Data1 = pStrHdr->fccHandler;
                TRACE("Subtype FCC: %.04s\n", (LPCSTR)&pStrHdr->fccHandler);
                amt.lSampleSize = pStrHdr->dwSampleSize;
                amt.bFixedSizeSamples = (amt.lSampleSize != 0);

                /* FIXME: Is this right? */
                if (!amt.lSampleSize)
                {
                    amt.lSampleSize = 1;
                    dwSampleSize = 1;
                }

                amt.bTemporalCompression = IsEqualGUID(&amt.majortype, &MEDIATYPE_Video); /* FIXME? */
                dwSampleSize = pStrHdr->dwSampleSize;
                dwLength = pStrHdr->dwLength;
                if (!dwLength)
                    dwLength = This->AviHeader.dwTotalFrames;

                if (pStrHdr->dwSuggestedBufferSize && pStrHdr->dwSuggestedBufferSize > props->cbBuffer)
                    props->cbBuffer = pStrHdr->dwSuggestedBufferSize;

                break;
            }
        case ckidSTREAMFORMAT:
            TRACE("processing stream format data\n");
            if (IsEqualIID(&amt.formattype, &FORMAT_VideoInfo))
            {
                VIDEOINFOHEADER * pvi;
                /* biCompression member appears to override the value in the stream header.
                 * i.e. the stream header can say something completely contradictory to what
                 * is in the BITMAPINFOHEADER! */
                if (pChunk->cb < sizeof(BITMAPINFOHEADER))
                {
                    ERR("Not enough bytes for BITMAPINFOHEADER\n");
                    return E_FAIL;
                }
                amt.cbFormat = sizeof(VIDEOINFOHEADER) - sizeof(BITMAPINFOHEADER) + pChunk->cb;
                amt.pbFormat = CoTaskMemAlloc(amt.cbFormat);
                ZeroMemory(amt.pbFormat, amt.cbFormat);
                pvi = (VIDEOINFOHEADER *)amt.pbFormat;
                pvi->AvgTimePerFrame = (LONGLONG)(10000000.0 / fSamplesPerSec);

                CopyMemory(&pvi->bmiHeader, pChunk + 1, pChunk->cb);
                if (pvi->bmiHeader.biCompression)
                    amt.subtype.Data1 = pvi->bmiHeader.biCompression;
            }
            else if (IsEqualIID(&amt.formattype, &FORMAT_WaveFormatEx))
            {
                amt.cbFormat = pChunk->cb;
                if (amt.cbFormat < sizeof(WAVEFORMATEX))
                    amt.cbFormat = sizeof(WAVEFORMATEX);
                amt.pbFormat = CoTaskMemAlloc(amt.cbFormat);
                ZeroMemory(amt.pbFormat, amt.cbFormat);
                CopyMemory(amt.pbFormat, pChunk + 1, pChunk->cb);
            }
            else
            {
                amt.cbFormat = pChunk->cb;
                amt.pbFormat = CoTaskMemAlloc(amt.cbFormat);
                CopyMemory(amt.pbFormat, pChunk + 1, amt.cbFormat);
            }
            break;
        case ckidSTREAMNAME:
            TRACE("processing stream name\n");
            /* FIXME: this doesn't exactly match native version (we omit the "##)" prefix), but hey... */
            MultiByteToWideChar(CP_ACP, 0, (LPCSTR)(pChunk + 1), pChunk->cb, piOutput.achName, sizeof(piOutput.achName) / sizeof(piOutput.achName[0]));
            break;
        case ckidSTREAMHANDLERDATA:
            FIXME("process stream handler data\n");
            break;
        case ckidAVIPADDING:
            TRACE("JUNK chunk ignored\n");
            break;
        case ckidAVISUPERINDEX:
        {
            const AVISUPERINDEX *pIndex = (const AVISUPERINDEX *)pChunk;
            DWORD x;
            UINT rest = pIndex->cb - sizeof(AVISUPERINDEX) + sizeof(RIFFCHUNK) + sizeof(pIndex->aIndex[0]) * ANYSIZE_ARRAY;

            if (pIndex->cb < sizeof(AVISUPERINDEX) - sizeof(RIFFCHUNK))
            {
                FIXME("size %u\n", pIndex->cb);
                break;
            }

            if (nstdindex++ > 0)
            {
                ERR("Stream %d got more than 1 superindex?\n", This->Parser.cStreams);
                break;
            }

            TRACE("wLongsPerEntry: %hd\n", pIndex->wLongsPerEntry);
            TRACE("bIndexSubType: %u\n", pIndex->bIndexSubType);
            TRACE("bIndexType: %u\n", pIndex->bIndexType);
            TRACE("nEntriesInUse: %u\n", pIndex->nEntriesInUse);
            TRACE("dwChunkId: %.4s\n", (const char *)&pIndex->dwChunkId);
            if (pIndex->dwReserved[0])
                TRACE("dwReserved[0]: %u\n", pIndex->dwReserved[0]);
            if (pIndex->dwReserved[1])
                TRACE("dwReserved[1]: %u\n", pIndex->dwReserved[1]);
            if (pIndex->dwReserved[2])
                TRACE("dwReserved[2]: %u\n", pIndex->dwReserved[2]);

            if (pIndex->bIndexType != AVI_INDEX_OF_INDEXES
                || pIndex->wLongsPerEntry != 4
                || rest < (pIndex->nEntriesInUse * sizeof(DWORD) * pIndex->wLongsPerEntry)
                || (pIndex->bIndexSubType != AVI_INDEX_SUB_2FIELD && pIndex->bIndexSubType != AVI_INDEX_SUB_DEFAULT))
            {
                FIXME("Invalid index chunk encountered\n");
                break;
            }

            stream->entries = pIndex->nEntriesInUse;
            stream->stdindex = CoTaskMemRealloc(stream->stdindex, sizeof(*stream->stdindex) * stream->entries);
            for (x = 0; x < pIndex->nEntriesInUse; ++x)
            {
                TRACE("qwOffset: %x%08x\n", (DWORD)(pIndex->aIndex[x].qwOffset >> 32), (DWORD)pIndex->aIndex[x].qwOffset);
                TRACE("dwSize: %u\n", pIndex->aIndex[x].dwSize);
                TRACE("dwDuration: %u (unreliable)\n", pIndex->aIndex[x].dwDuration);

                AVISplitter_ProcessIndex(This, &stream->stdindex[x], pIndex->aIndex[x].qwOffset, pIndex->aIndex[x].dwSize);
            }
            break;
        }
        default:
            FIXME("unknown chunk type \"%.04s\" ignored\n", (LPCSTR)&pChunk->fcc);
        }
    }

    if (IsEqualGUID(&amt.formattype, &FORMAT_WaveFormatEx))
    {
        amt.subtype = MEDIATYPE_Video;
        amt.subtype.Data1 = ((WAVEFORMATEX *)amt.pbFormat)->wFormatTag;
    }

    dump_AM_MEDIA_TYPE(&amt);
    TRACE("fSamplesPerSec = %f\n", (double)fSamplesPerSec);
    TRACE("dwSampleSize = %x\n", dwSampleSize);
    TRACE("dwLength = %x\n", dwLength);

    stream->fSamplesPerSec = fSamplesPerSec;
    stream->dwSampleSize = dwSampleSize;
    stream->dwLength = dwLength; /* TODO: Use this for mediaseeking */
    stream->packet_queued = CreateEventW(NULL, 0, 0, NULL);

    hr = Parser_AddPin(&(This->Parser), &piOutput, props, &amt);
    CoTaskMemFree(amt.pbFormat);


    return hr;
}

static HRESULT AVISplitter_ProcessODML(AVISplitterImpl * This, const BYTE * pData, DWORD cb)
{
    const RIFFCHUNK * pChunk;

    for (pChunk = (const RIFFCHUNK *)pData;
         ((const BYTE *)pChunk >= pData) && ((const BYTE *)pChunk + sizeof(RIFFCHUNK) < pData + cb) && (pChunk->cb > 0);
         pChunk = (const RIFFCHUNK *)((const BYTE*)pChunk + sizeof(RIFFCHUNK) + pChunk->cb)
        )
    {
        switch (pChunk->fcc)
        {
        case ckidAVIEXTHEADER:
            {
                int x;
                const AVIEXTHEADER * pExtHdr = (const AVIEXTHEADER *)pChunk;

                TRACE("processing extension header\n");
                if (pExtHdr->cb != sizeof(AVIEXTHEADER) - sizeof(RIFFCHUNK))
                {
                    FIXME("Size: %u\n", pExtHdr->cb);
                    break;
                }
                TRACE("dwGrandFrames: %u\n", pExtHdr->dwGrandFrames);
                for (x = 0; x < 61; ++x)
                    if (pExtHdr->dwFuture[x])
                        FIXME("dwFuture[%i] = %u (0x%08x)\n", x, pExtHdr->dwFuture[x], pExtHdr->dwFuture[x]);
                This->ExtHeader = *pExtHdr;
                break;
            }
        default:
            FIXME("unknown chunk type \"%.04s\" ignored\n", (LPCSTR)&pChunk->fcc);
        }
    }

    return S_OK;
}

static HRESULT AVISplitter_InitializeStreams(AVISplitterImpl *This)
{
    unsigned int x;

    if (This->oldindex)
    {
        DWORD nMax, n;

        for (x = 0; x < This->Parser.cStreams; ++x)
        {
            This->streams[x].frames = 0;
            This->streams[x].pos = ~0;
            This->streams[x].index = 0;
        }

        nMax = This->oldindex->cb / sizeof(This->oldindex->aIndex[0]);

        /* Ok, maybe this is more of an exercise to see if I interpret everything correctly or not, but that is useful for now. */
        for (n = 0; n < nMax; ++n)
        {
            DWORD streamId = StreamFromFOURCC(This->oldindex->aIndex[n].dwChunkId);
            if (streamId >= This->Parser.cStreams)
            {
                FIXME("Stream id %s ignored\n", debugstr_an((char*)&This->oldindex->aIndex[n].dwChunkId, 4));
                continue;
            }
            if (This->streams[streamId].pos == ~0U)
                This->streams[streamId].pos = n;

            if (This->streams[streamId].streamheader.dwSampleSize)
                This->streams[streamId].frames += This->oldindex->aIndex[n].dwSize / This->streams[streamId].streamheader.dwSampleSize;
            else
                ++This->streams[streamId].frames;
        }

        for (x = 0; x < This->Parser.cStreams; ++x)
        {
            if ((DWORD)This->streams[x].frames != This->streams[x].streamheader.dwLength)
            {
                FIXME("stream %u: frames found: %u, frames meant to be found: %u\n", x, (DWORD)This->streams[x].frames, This->streams[x].streamheader.dwLength);
            }
        }

    }
    else if (!This->streams[0].entries)
    {
        for (x = 0; x < This->Parser.cStreams; ++x)
        {
            This->streams[x].frames = This->streams[x].streamheader.dwLength;
        }
        /* MS Avi splitter does seek through the whole file, we should! */
        ERR("We should be manually seeking through the entire file to build an index, because the index is missing!!!\n");
        return E_NOTIMPL;
    }

    /* Not much here yet */
    for (x = 0; x < This->Parser.cStreams; ++x)
    {
        StreamData *stream = This->streams + x;
        DWORD y;
        DWORD64 frames = 0;

        stream->seek = TRUE;

        if (stream->stdindex)
        {
            stream->index = 0;
            stream->pos = 0;
            for (y = 0; y < stream->entries; ++y)
            {
                if (stream->streamheader.dwSampleSize)
                {
                    DWORD z;

                    for (z = 0; z < stream->stdindex[y]->nEntriesInUse; ++z)
                    {
                        UINT len = stream->stdindex[y]->aIndex[z].dwSize & ~(1 << 31);
                        frames += len / stream->streamheader.dwSampleSize + !!(len % stream->streamheader.dwSampleSize);
                    }
                }
                else
                    frames += stream->stdindex[y]->nEntriesInUse;
            }
        }
        else frames = stream->frames;

        frames *= stream->streamheader.dwScale;
        /* Keep accuracy as high as possible for duration */
        This->Parser.sourceSeeking.llDuration = frames * 10000000;
        This->Parser.sourceSeeking.llDuration /= stream->streamheader.dwRate;
        This->Parser.sourceSeeking.llStop = This->Parser.sourceSeeking.llDuration;
        This->Parser.sourceSeeking.llCurrent = 0;

        frames /= stream->streamheader.dwRate;

        TRACE("Duration: %d days, %d hours, %d minutes and %d.%03u seconds\n", (DWORD)(frames / 86400),
        (DWORD)((frames % 86400) / 3600), (DWORD)((frames % 3600) / 60), (DWORD)(frames % 60),
        (DWORD)(This->Parser.sourceSeeking.llDuration/10000) % 1000);
    }

    return S_OK;
}

static HRESULT AVISplitter_Disconnect(LPVOID iface);

/* FIXME: fix leaks on failure here */
static HRESULT AVISplitter_InputPin_PreConnect(IPin * iface, IPin * pConnectPin, ALLOCATOR_PROPERTIES *props)
{
    PullPin *This = impl_PullPin_from_IPin(iface);
    HRESULT hr;
    RIFFLIST list;
    LONGLONG pos = 0; /* in bytes */
    BYTE * pBuffer;
    RIFFCHUNK * pCurrentChunk;
    LONGLONG total, avail;
    ULONG x;
    DWORD indexes;

    AVISplitterImpl * pAviSplit = (AVISplitterImpl *)This->pin.pinInfo.pFilter;

    hr = IAsyncReader_SyncRead(This->pReader, pos, sizeof(list), (BYTE *)&list);
    pos += sizeof(list);

    if (list.fcc != FOURCC_RIFF)
    {
        ERR("Input stream not a RIFF file\n");
        return E_FAIL;
    }
    if (list.fccListType != formtypeAVI)
    {
        ERR("Input stream not an AVI RIFF file\n");
        return E_FAIL;
    }

    hr = IAsyncReader_SyncRead(This->pReader, pos, sizeof(list), (BYTE *)&list);
    if (list.fcc != FOURCC_LIST)
    {
        ERR("Expected LIST chunk, but got %.04s\n", (LPSTR)&list.fcc);
        return E_FAIL;
    }
    if (list.fccListType != listtypeAVIHEADER)
    {
        ERR("Header list expected. Got: %.04s\n", (LPSTR)&list.fccListType);
        return E_FAIL;
    }

    pBuffer = HeapAlloc(GetProcessHeap(), 0, list.cb - sizeof(RIFFLIST) + sizeof(RIFFCHUNK));
    hr = IAsyncReader_SyncRead(This->pReader, pos + sizeof(list), list.cb - sizeof(RIFFLIST) + sizeof(RIFFCHUNK), pBuffer);

    pAviSplit->AviHeader.cb = 0;

    /* Stream list will set the buffer size here, so set a default and allow an override */
    props->cbBuffer = 0x20000;

    for (pCurrentChunk = (RIFFCHUNK *)pBuffer; (BYTE *)pCurrentChunk + sizeof(*pCurrentChunk) < pBuffer + list.cb; pCurrentChunk = (RIFFCHUNK *)(((BYTE *)pCurrentChunk) + sizeof(*pCurrentChunk) + pCurrentChunk->cb))
    {
        RIFFLIST * pList;

        switch (pCurrentChunk->fcc)
        {
        case ckidMAINAVIHEADER:
            /* AVIMAINHEADER includes the structure that is pCurrentChunk at the moment */
            memcpy(&pAviSplit->AviHeader, pCurrentChunk, sizeof(pAviSplit->AviHeader));
            break;
        case FOURCC_LIST:
            pList = (RIFFLIST *)pCurrentChunk;
            switch (pList->fccListType)
            {
            case ckidSTREAMLIST:
                hr = AVISplitter_ProcessStreamList(pAviSplit, (BYTE *)pCurrentChunk + sizeof(RIFFLIST), pCurrentChunk->cb + sizeof(RIFFCHUNK) - sizeof(RIFFLIST), props);
                break;
            case ckidODML:
                hr = AVISplitter_ProcessODML(pAviSplit, (BYTE *)pCurrentChunk + sizeof(RIFFLIST), pCurrentChunk->cb + sizeof(RIFFCHUNK) - sizeof(RIFFLIST));
                break;
            }
            break;
        case ckidAVIPADDING:
            /* ignore */
            break;
        default:
            FIXME("unrecognised header list type: %.04s\n", (LPSTR)&pCurrentChunk->fcc);
        }
    }
    HeapFree(GetProcessHeap(), 0, pBuffer);

    if (pAviSplit->AviHeader.cb != sizeof(pAviSplit->AviHeader) - sizeof(RIFFCHUNK))
    {
        ERR("Avi Header wrong size!\n");
        return E_FAIL;
    }

    pos += sizeof(RIFFCHUNK) + list.cb;
    hr = IAsyncReader_SyncRead(This->pReader, pos, sizeof(list), (BYTE *)&list);

    while (list.fcc == ckidAVIPADDING || (list.fcc == FOURCC_LIST && list.fccListType != listtypeAVIMOVIE))
    {
        pos += sizeof(RIFFCHUNK) + list.cb;

        hr = IAsyncReader_SyncRead(This->pReader, pos, sizeof(list), (BYTE *)&list);
    }

    if (list.fcc != FOURCC_LIST)
    {
        ERR("Expected LIST, but got %.04s\n", (LPSTR)&list.fcc);
        return E_FAIL;
    }
    if (list.fccListType != listtypeAVIMOVIE)
    {
        ERR("Expected AVI movie list, but got %.04s\n", (LPSTR)&list.fccListType);
        return E_FAIL;
    }

    IAsyncReader_Length(This->pReader, &total, &avail);

    /* FIXME: AVIX files are extended beyond the FOURCC chunk "AVI ", and thus won't be played here,
     * once I get one of the files I'll try to fix it */
    if (hr == S_OK)
    {
        This->rtStart = pAviSplit->CurrentChunkOffset = MEDIATIME_FROM_BYTES(pos + sizeof(RIFFLIST));
        pos += list.cb + sizeof(RIFFCHUNK);

        pAviSplit->EndOfFile = This->rtStop = MEDIATIME_FROM_BYTES(pos);
        if (pos > total)
        {
            ERR("File smaller (%x%08x) then EndOfFile (%x%08x)\n", (DWORD)(total >> 32), (DWORD)total, (DWORD)(pAviSplit->EndOfFile >> 32), (DWORD)pAviSplit->EndOfFile);
            return E_FAIL;
        }

        hr = IAsyncReader_SyncRead(This->pReader, BYTES_FROM_MEDIATIME(pAviSplit->CurrentChunkOffset), sizeof(pAviSplit->CurrentChunk), (BYTE *)&pAviSplit->CurrentChunk);
    }

    props->cbAlign = 1;
    props->cbPrefix = 0;
    /* Comrades, prevent shortage of buffers, or you will feel the consequences! DA! */
    props->cBuffers = 2 * pAviSplit->Parser.cStreams;

    /* Now peek into the idx1 index, if available */
    if (hr == S_OK && (total - pos) > sizeof(RIFFCHUNK))
    {
        memset(&list, 0, sizeof(list));

        hr = IAsyncReader_SyncRead(This->pReader, pos, sizeof(list), (BYTE *)&list);
        if (list.fcc == ckidAVIOLDINDEX)
        {
            pAviSplit->oldindex = CoTaskMemRealloc(pAviSplit->oldindex, list.cb + sizeof(RIFFCHUNK));
            if (pAviSplit->oldindex)
            {
                hr = IAsyncReader_SyncRead(This->pReader, pos, sizeof(RIFFCHUNK) + list.cb, (BYTE *)pAviSplit->oldindex);
                if (hr == S_OK)
                {
                    hr = AVISplitter_ProcessOldIndex(pAviSplit);
                }
                else
                {
                    CoTaskMemFree(pAviSplit->oldindex);
                    pAviSplit->oldindex = NULL;
                    hr = S_OK;
                }
            }
        }
    }

    indexes = 0;
    for (x = 0; x < pAviSplit->Parser.cStreams; ++x)
        if (pAviSplit->streams[x].entries)
            ++indexes;

    if (indexes)
    {
        CoTaskMemFree(pAviSplit->oldindex);
        pAviSplit->oldindex = NULL;
        if (indexes < pAviSplit->Parser.cStreams)
        {
            /* This error could possible be survived by switching to old type index,
             * but I would rather find out why it doesn't find everything here
             */
            ERR("%d indexes expected, but only have %d\n", indexes, pAviSplit->Parser.cStreams);
            indexes = 0;
        }
    }
    else if (pAviSplit->oldindex)
        indexes = pAviSplit->Parser.cStreams;

    if (!indexes && pAviSplit->AviHeader.dwFlags & AVIF_MUSTUSEINDEX)
    {
        FIXME("No usable index was found!\n");
        hr = E_FAIL;
    }

    /* Now, set up the streams */
    if (hr == S_OK)
        hr = AVISplitter_InitializeStreams(pAviSplit);

    if (hr != S_OK)
    {
        AVISplitter_Disconnect(pAviSplit);
        return E_FAIL;
    }

    TRACE("AVI File ok\n");

    return hr;
}

static HRESULT AVISplitter_Flush(LPVOID iface)
{
    AVISplitterImpl *This = iface;
    DWORD x;
    ULONG ref;

    TRACE("(%p)->()\n", This);

    for (x = 0; x < This->Parser.cStreams; ++x)
    {
        StreamData *stream = This->streams + x;

        if (stream->sample)
        {
            ref = IMediaSample_Release(stream->sample);
            assert(ref == 0);
        }
        stream->sample = NULL;

        ResetEvent(stream->packet_queued);
        assert(!stream->thread);
    }

    return S_OK;
}

static HRESULT AVISplitter_Disconnect(LPVOID iface)
{
    AVISplitterImpl *This = iface;
    ULONG x;

    /* TODO: Remove other memory that's allocated during connect */
    CoTaskMemFree(This->oldindex);
    This->oldindex = NULL;

    for (x = 0; x < This->Parser.cStreams; ++x)
    {
        DWORD i;

        StreamData *stream = &This->streams[x];

        for (i = 0; i < stream->entries; ++i)
            CoTaskMemFree(stream->stdindex[i]);

        CoTaskMemFree(stream->stdindex);
        CloseHandle(stream->packet_queued);
    }
    CoTaskMemFree(This->streams);
    This->streams = NULL;
    return S_OK;
}

static ULONG WINAPI AVISplitter_Release(IBaseFilter *iface)
{
    AVISplitterImpl *This = (AVISplitterImpl *)iface;
    ULONG ref;

    ref = InterlockedDecrement(&This->Parser.filter.refCount);

    TRACE("(%p)->() Release from %d\n", This, ref + 1);

    if (!ref)
    {
        AVISplitter_Flush(This);
        Parser_Destroy(&This->Parser);
    }

    return ref;
}

static HRESULT WINAPI AVISplitter_seek(IMediaSeeking *iface)
{
    AVISplitterImpl *This = impl_from_IMediaSeeking(iface);
    PullPin *pPin = This->Parser.pInputPin;
    LONGLONG newpos, endpos;
    DWORD x;

    newpos = This->Parser.sourceSeeking.llCurrent;
    endpos = This->Parser.sourceSeeking.llDuration;

    if (newpos > endpos)
    {
        WARN("Requesting position %x%08x beyond end of stream %x%08x\n", (DWORD)(newpos>>32), (DWORD)newpos, (DWORD)(endpos>>32), (DWORD)endpos);
        return E_INVALIDARG;
    }

    FIXME("Moving position to %u.%03u s!\n", (DWORD)(newpos / 10000000), (DWORD)((newpos / 10000)%1000));

    EnterCriticalSection(&pPin->thread_lock);
    /* Send a flush to all output pins */
    IPin_BeginFlush(&pPin->pin.IPin_iface);

    /* Make sure this is done while stopped, BeginFlush takes care of this */
    EnterCriticalSection(&This->Parser.filter.csFilter);
    for (x = 0; x < This->Parser.cStreams; ++x)
    {
        Parser_OutputPin *pin = unsafe_impl_Parser_OutputPin_from_IPin(This->Parser.ppPins[1+x]);
        StreamData *stream = This->streams + x;
        LONGLONG wanted_frames;
        DWORD last_keyframe = 0, last_keyframeidx = 0, preroll = 0;

        wanted_frames = newpos;
        wanted_frames *= stream->streamheader.dwRate;
        wanted_frames /= 10000000;
        wanted_frames /= stream->streamheader.dwScale;

        pin->dwSamplesProcessed = 0;
        stream->index = 0;
        stream->pos = 0;
        stream->seek = TRUE;
        if (stream->stdindex)
        {
            DWORD y, z = 0;

            for (y = 0; y < stream->entries; ++y)
            {
                for (z = 0; z < stream->stdindex[y]->nEntriesInUse; ++z)
                {
                    if (stream->streamheader.dwSampleSize)
                    {
                        ULONG len = stream->stdindex[y]->aIndex[z].dwSize & ~(1 << 31);
                        ULONG size = stream->streamheader.dwSampleSize;

                        pin->dwSamplesProcessed += len / size;
                        if (len % size)
                            ++pin->dwSamplesProcessed;
                    }
                    else ++pin->dwSamplesProcessed;

                    if (!(stream->stdindex[y]->aIndex[z].dwSize >> 31))
                    {
                        last_keyframe = z;
                        last_keyframeidx = y;
                        preroll = 0;
                    }
                    else
                        ++preroll;

                    if (pin->dwSamplesProcessed >= wanted_frames)
                        break;
                }
                if (pin->dwSamplesProcessed >= wanted_frames)
                    break;
            }
            stream->index = last_keyframeidx;
            stream->pos = last_keyframe;
        }
        else
        {
            DWORD nMax, n;
            nMax = This->oldindex->cb / sizeof(This->oldindex->aIndex[0]);

            for (n = 0; n < nMax; ++n)
            {
                DWORD streamId = StreamFromFOURCC(This->oldindex->aIndex[n].dwChunkId);
                if (streamId != x)
                    continue;

                if (stream->streamheader.dwSampleSize)
                {
                    ULONG len = This->oldindex->aIndex[n].dwSize;
                    ULONG size = stream->streamheader.dwSampleSize;

                    pin->dwSamplesProcessed += len / size;
                    if (len % size)
                        ++pin->dwSamplesProcessed;
                }
                else ++pin->dwSamplesProcessed;

                if (This->oldindex->aIndex[n].dwFlags & AVIIF_KEYFRAME)
                {
                    last_keyframe = n;
                    preroll = 0;
                }
                else
                    ++preroll;

                if (pin->dwSamplesProcessed >= wanted_frames)
                    break;
            }
            assert(n < nMax);
            stream->pos = last_keyframe;
            stream->index = 0;
        }
        stream->preroll = preroll;
        stream->seek = TRUE;
    }
    LeaveCriticalSection(&This->Parser.filter.csFilter);

    TRACE("Done flushing\n");
    IPin_EndFlush(&pPin->pin.IPin_iface);
    LeaveCriticalSection(&pPin->thread_lock);

    return S_OK;
}

static const IBaseFilterVtbl AVISplitterImpl_Vtbl =
{
    Parser_QueryInterface,
    Parser_AddRef,
    AVISplitter_Release,
    Parser_GetClassID,
    Parser_Stop,
    Parser_Pause,
    Parser_Run,
    Parser_GetState,
    Parser_SetSyncSource,
    Parser_GetSyncSource,
    Parser_EnumPins,
    Parser_FindPin,
    Parser_QueryFilterInfo,
    Parser_JoinFilterGraph,
    Parser_QueryVendorInfo
};

HRESULT AVISplitter_create(IUnknown * pUnkOuter, LPVOID * ppv)
{
    HRESULT hr;
    AVISplitterImpl * This;

    TRACE("(%p, %p)\n", pUnkOuter, ppv);

    *ppv = NULL;

    if (pUnkOuter)
        return CLASS_E_NOAGGREGATION;

    /* Note: This memory is managed by the transform filter once created */
    This = CoTaskMemAlloc(sizeof(AVISplitterImpl));

    This->streams = NULL;
    This->oldindex = NULL;

    hr = Parser_Create(&(This->Parser), &AVISplitterImpl_Vtbl, &CLSID_AviSplitter, AVISplitter_Sample, AVISplitter_QueryAccept, AVISplitter_InputPin_PreConnect, AVISplitter_Flush, AVISplitter_Disconnect, AVISplitter_first_request, AVISplitter_done_process, NULL, AVISplitter_seek, NULL);

    if (FAILED(hr))
        return hr;

    *ppv = This;

    return hr;
}
