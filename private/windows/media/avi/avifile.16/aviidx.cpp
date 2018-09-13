/******************************************************************************

   Copyright (C) Microsoft Corporation 1985-1991. All rights reserved.

   AVIIDX.C - AVI Index stuff

*****************************************************************************/

#include <win32.h>      // Win16/32 porting
#include <avifmt.h>     // for stream type
#include "aviidx.h"

#ifdef AVIIDX_READONLY
    #include "common.h"     // for DEBUG
#else
    #include "debug.h"      // for DEBUG
#endif

/***************************************************************************
 ***************************************************************************/

#define INDEXALLOC      512
#define STACK           _based(_segname("_STACK"))

/***************************************************************************
 ***************************************************************************/

//
// used by SearchIndex() to return where a sample is
//
typedef struct {
    LONG    lx;             // index position
    LONG    lPos;           // position in samples.
    LONG    lSize;          // size in samples.
    LONG    lOffset;        // file offset.
    LONG    lLength;        // size in bytes.
}   IDXPOS;

/***************************************************************************
 *
 * @doc INTERNAL
 *
 * @api PAVIINDEX | IndexAddFileIndex
 *
 *  add a bunch of entries from a AVIFILE index to the index.
 *
 ***************************************************************************/

EXTERN_C PAVIINDEX IndexAddFileIndex(PAVIINDEX px, AVIINDEXENTRY _huge *pidx, LONG cnt, LONG lAdjust, BOOL fRle)
{
    LONG        lx;
    LONG        l;
    LONG        lxRec;
    DWORD       ckid;
    UINT        stream;
    DWORD       offset;
    DWORD       length;
    UINT        flags;

    Assert(px);
    Assert(pidx);

    if (px == NULL || pidx == NULL)
        return NULL;

    Assert(sizeof(AVIINDEXENTRY) > sizeof(AVIIDX));

    //
    // grow the index if needed.
    //
    if (px->nIndex + cnt > px->nIndexSize) {

        LONG grow = px->nIndex + cnt - px->nIndexSize;
        LPVOID p;

        if (grow < INDEXALLOC)
            grow = INDEXALLOC;

        p = (LPVOID)GlobalReAllocPtr(px,sizeof(AVIINDEX) +
                (px->nIndexSize + grow) * sizeof(AVIIDX),
		GMEM_MOVEABLE | GMEM_SHARE);

	if (!p)
            return NULL;
	
        px = (PAVIINDEX)p;
        px->nIndexSize += grow;
    }

    for (lxRec=-1,l=0; l < cnt; l++,pidx++) {

        lx = px->nIndex + l;

        //
        // adjust the offset to be absolute
        //
        offset = pidx->dwChunkOffset + lAdjust;
        length = pidx->dwChunkLength;
        ckid   = pidx->ckid;
        stream = StreamFromFOURCC(ckid);
        flags  = 0;

        if (ckid == listtypeAVIRECORD)
            stream = STREAM_REC;

        if (ckid == listtypeAVIRECORD)
            lxRec = lx;

        //
        // handle over flows in a "sane" way.
        //
        if (offset >= MAX_OFFSET)
            break;

        if (stream >= MAX_STREAM)
            break;

        if (length >= MAX_LENGTH)
            length = MAX_LENGTH-1;

        if (pidx->dwFlags & AVIIF_KEYFRAME)
            flags |= IDX_KEY;
        else
            flags |= IDX_NONKEY;

        //
        // length == 0 samples are not real
        //
        if (length == 0)
            flags &= ~(IDX_NONKEY|IDX_KEY);

        //
        // mark palette changes
        //
        if (TWOCCFromFOURCC(ckid) == cktypePALchange) {
            flags |= IDX_PAL;
            flags &= ~(IDX_NONKEY|IDX_KEY);
        }

        //
        // fix up bogus index's by adding any missing KEYFRAME
        // bits. ie this only applies for silly RLE files.
        //
        if (fRle && length > 0 && TWOCCFromFOURCC(ckid) == cktypeDIBbits)
            flags |= IDX_KEY;

        //
        // do we need to support these?
        //
        if (fRle && TWOCCFromFOURCC(ckid) == aviTWOCC('d', 'x'))
            flags |= IDX_HALF;

        //
        // audio is always a key.
        //
        if (TWOCCFromFOURCC(ckid) == cktypeWAVEbytes)
            flags |= IDX_KEY|IDX_NONKEY;    //hack to get audio back!

        //
        // make sure records are marked as contining a key
        //
        //if (lxRec > 0 && (flags & IDX_KEY))
        //  IndexSetKey(px, lxRec);

        IndexSetFlags(px,lx,flags);
        IndexSetOffset(px,lx,offset);
        IndexSetLength(px,lx,length);
        IndexSetStream(px,lx,stream);

    }

    cnt = l;
    px->nIndex += cnt;

    return px;
}

/***************************************************************************
 ***************************************************************************/

static LONG FAR PASCAL mmioReadProc(HMMIO hmmio, LONG lSeek, LONG lRead, LPVOID lpBuffer)
{
    if (mmioSeek(hmmio, lSeek, SEEK_SET) == -1)
        return -1;

    if (mmioRead(hmmio, (HPSTR)lpBuffer, lRead) != lRead)
        return -1;

    return lRead;
}

/***************************************************************************
 ***************************************************************************/

static LONG FAR PASCAL mmioWriteProc(HMMIO hmmio, LONG lSeek, LONG lWrite, LPVOID lpBuffer)
{
    if (mmioSeek(hmmio, lSeek, SEEK_SET) == -1)
        return -1;

    if (mmioWrite(hmmio, (HPSTR)lpBuffer, lWrite) != lWrite)
        return -1;

    return lWrite;
}

/***************************************************************************
 *
 * @doc INTERNAL
 *
 * @api PSTREAMINDEX | MakeStreamIndex
 *
 *  makes a STREAMINDEX structure that will be used later to read/find
 *  samples in a stream.
 *
 ***************************************************************************/

EXTERN_C PSTREAMINDEX MakeStreamIndex(PAVIINDEX px, UINT stream, LONG lStart, LONG lSampleSize, HANDLE hFile, STREAMIOPROC ReadProc, STREAMIOPROC WriteProc)
{
    LONG         lPos;
    LONG         lx;
    PSTREAMINDEX psx;

    Assert(px);

    if (px == NULL)
        return NULL;

    psx = (PSTREAMINDEX)LocalAlloc(LPTR, sizeof(STREAMINDEX));

    if (psx == NULL)
        return NULL;

    //!!! fixed length sample streams should never have this

    if (lSampleSize != 0 && lStart < 0) {
#ifdef DEBUG
        AssertSz(0, "Audio streams should not have initial frames");
#endif
        lStart = 0;
    }

    psx->px             = px;
    psx->lStart         = lStart;
    psx->lSampleSize    = lSampleSize;
    psx->lMaxSampleSize = 0;
    psx->stream         = stream;
    psx->flags          = 0;

    psx->lStart         = lStart;
    psx->lxStart        = IndexFirst(px, stream);

    psx->lPos           = lStart;
    psx->lx             = psx->lxStart;

    psx->lFrames        = 0;
    psx->lKeyFrames     = 0;
    psx->lPalFrames     = 0;
    psx->lNulFrames     = 0;

    psx->hFile          = hFile;

    if (ReadProc == NULL)
        psx->Read       = (STREAMIOPROC)mmioReadProc;
    else
        psx->Read       = ReadProc;

    if (WriteProc == NULL)
        psx->Write      = (STREAMIOPROC)mmioWriteProc;
    else
        psx->Write      = WriteProc;

    lPos = lStart;

    for (lx = psx->lxStart; lx >= 0 && lx < px->nIndex; lx=IndexNext(px, lx, 0)) {

        if (psx->lMaxSampleSize < IndexLength(px, lx))
            psx->lMaxSampleSize = IndexLength(px, lx);

        //
        // make sure the start sample is a key frame (unless it's wave data!)
        //
        if (lPos == 0 || (lPos >= 0 && lPos == psx->lStart)) {
	    if ((IndexFlags(px, lx) & (IDX_KEY|IDX_NONKEY)) !=
						(IDX_KEY|IDX_NONKEY)) {
		IndexSetKey(px, lx);
	    }
	}

        //
	// make sure sample size is correct
	//
        if (psx->lSampleSize &&
                ((IndexLength(px, lx) % lSampleSize) != 0)) {
            DPF("Bad chunk size found: forcing sample size to 0.\n");
            psx->lSampleSize = 0;
	}

        //
        //  or all the flags together so we can see what a stream has.
        //
        psx->flags |= IndexFlags(px, lx);

        //
        //  check for all key frames.
        //
        if (IndexFlags(px, lx) & IDX_KEY)
            psx->lKeyFrames++;

        //
        //  check for all palette changes
        //
        if (IndexFlags(px, lx) & IDX_PAL)
            psx->lPalFrames++;

        //
        //  check for empty frames
        //
        if (IndexLength(px, lx) == 0)
            psx->lNulFrames++;

        //
        // advance the position
        //
        if (!(IndexFlags(px,lx) & IDX_NOTIME)) {
            if (lSampleSize)
                lPos += IndexLength(px, lx) / lSampleSize;
            else
                lPos++;
        }

        psx->lFrames++;
    }

    //
    //  correct the length
    //
    psx->lEnd = lPos;

    DPF("MakeStreamIndex  stream=#%d lStart=%ld, lEnd=%ld\n", stream, psx->lStart, psx->lEnd);
    DPF("                 lFrames = %ld, lKeys = %ld, lPals = %ld, lEmpty = %ld\n", psx->lFrames, psx->lKeyFrames, psx->lPalFrames, psx->lNulFrames);

    return psx;
}

#ifndef AVIIDX_READONLY

/***************************************************************************
 *
 * @doc INTERNAL
 *
 * @api PAVIINDEX | IndexGetFileIndex
 *
 *     make a file index out of a in memory index
 *
 ***************************************************************************/

EXTERN_C LONG IndexGetFileIndex(PAVIINDEX px, LONG l, LONG cnt, PAVIINDEXENTRY pidx, LONG lAdjust)
{
    LONG            lx;
    DWORD           ckid;
    UINT            stream;
    DWORD           offset;
    DWORD           length;
    UINT            flags;
    DWORD           dwFlags;

    Assert(pidx);
    Assert(px);

    if (pidx == NULL || px == NULL)
        return NULL;

    Assert(sizeof(AVIINDEXENTRY) > sizeof(AVIIDX));

    for (lx=l; lx < px->nIndex && lx < l+cnt; lx++) {
        //
        // adjust the offset to be relative
        //
        offset = IndexOffset(px,lx) + lAdjust;
        length = IndexLength(px,lx);
        stream = IndexStream(px,lx);
        flags  = IndexFlags(px, lx);

        if (length == MAX_LENGTH-1) {
        }

        ckid = MAKEAVICKID(0, stream);
        dwFlags = 0;

        //
        //  set the flags, there are only a few flags in file index's
        //  AVIIF_KEYFRAME, AVIIF_LIST, AVIIF_NOTIME
        //
        if (flags & IDX_KEY)
            dwFlags |= AVIIF_KEYFRAME;

        if (flags & IDX_PAL)
            dwFlags |= AVIIF_NOTIME;

        if (stream == STREAM_REC)
            dwFlags |= AVIIF_LIST;

        //
        //  now figure out the ckid
        //
        if (stream == STREAM_REC)
            ckid = listtypeAVIRECORD;

        else if ((flags & (IDX_KEY|IDX_NONKEY)) == (IDX_KEY|IDX_NONKEY))
            ckid |= MAKELONG(0, aviTWOCC('w', 'b'));

        else if (flags & IDX_PAL)
            ckid |= MAKELONG(0, aviTWOCC('p', 'c'));

        else if (flags & IDX_HALF)
            ckid |= MAKELONG(0, aviTWOCC('d', 'x'));

        else if (flags & IDX_KEY)
            ckid |= MAKELONG(0, aviTWOCC('d', 'b'));

        else
            ckid |= MAKELONG(0, aviTWOCC('d', 'c'));

        //
        // set the info
        //
        pidx->dwChunkOffset = offset;
        pidx->dwChunkLength = length;
        pidx->dwFlags       = dwFlags;
        pidx->ckid          = ckid;

        pidx++;
    }

    return lx - l;  // return count copied
}

/***************************************************************************
 *
 * @doc INTERNAL
 *
 * @api PAVIINDEX | IndexCreate | make a index.
 *
 ***************************************************************************/

EXTERN_C PAVIINDEX IndexCreate(void)
{
    PAVIINDEX px;

    px = (PAVIINDEX)GlobalAllocPtr(GHND | GMEM_SHARE,
        sizeof(AVIINDEX) + INDEXALLOC * sizeof(AVIIDX));

    if (px == NULL)
        return NULL;

    px->nIndex      = 0;          // index size
    px->nIndexSize  = INDEXALLOC; // allocated size

    return px;
}

#endif // AVIIDX_READONLY

/***************************************************************************
 *
 * @doc INTERNAL MCIAVI
 *
 * @api LONG | IndexFirst | returns the first index entry for a stream
 *
 * @rdesc returns the first index entry, -1 for error
 *
 ***************************************************************************/

EXTERN_C LONG IndexFirst(PAVIINDEX px, UINT stream)
{
    LONG l;

    Assert(px);

    for (l=0; l<px->nIndex; l++) {

        if (IndexStream(px, l) == stream)
            return l;
    }

    return ERR_IDX;
}

/***************************************************************************
 *
 * @doc INTERNAL MCIAVI
 *
 * @api LONG | IndexNext | go forward in a index
 *
 ***************************************************************************/

EXTERN_C LONG IndexNext(PAVIINDEX px, LONG l, UINT f)
{
    BYTE bStream;

    Assert(px);

    if (l < 0 || l >= px->nIndex)
        return ERR_IDX;

    bStream = IndexStream(px, l);

    for (l++; l<px->nIndex; l++) {

        if (IndexStream(px, l) != bStream)
            continue;

        if (!f || (IndexFlags(px, l) & f))
            return l;
    }

    return ERR_IDX;
}

/***************************************************************************
 *
 * @doc INTERNAL MCIAVI
 *
 * @api LONG | IndexPrev | step backward in a stream
 *
 ***************************************************************************/
EXTERN_C LONG IndexPrev(PAVIINDEX px, LONG l, UINT f)
{
    BYTE bStream;

    Assert(px);

    if (l < 0 || l >= px->nIndex)
        return ERR_IDX;

    bStream = IndexStream(px, l);

    for (l--; l>=0; l--) {

        if (IndexStream(px, l) != bStream)
            continue;

        if (!f || (IndexFlags(px, l) & f))
            return l;
    }

    return ERR_IDX;
}

///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////

INLINE BOOL StreamNext(PSTREAMINDEX psx, LONG FAR& l, LONG FAR& lPos, UINT flags)
{
    BYTE                bStream = psx->stream;
    LONG                lSampleSize = psx->lSampleSize;
    LONG                lSave = l;
    LONG                lPosSave = lPos;
    PAVIINDEX           px = psx->px;

    Assert(px && l >= 0 && l < px->nIndex);

    if (lSampleSize == 0) {

        lPos += 1;
        l++;

        for (; l<px->nIndex; l++) {

            if (IndexStream(px, l) != bStream)
                continue;

            if (!flags || (IndexFlags(px, l) & flags))
                return TRUE;

	    if (!(IndexFlags(px, l) & IDX_NOTIME))
		lPos += 1;
        }
    }
    else {

        lPos += IndexLength(px, l) / lSampleSize;
        l++;

        for (; l<px->nIndex; l++) {

            if (IndexStream(px, l) != bStream)
                continue;

            if (!flags || (IndexFlags(px, l) & flags))
                return TRUE;

            lPos += IndexLength(px, l) / lSampleSize;
        }
    }

    lPos = lPosSave;
    l    = lSave;

    return FALSE;
}

///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////

INLINE BOOL StreamPrev(PSTREAMINDEX psx, LONG FAR& l, LONG FAR& lPos, UINT flags)
{
    BYTE                bStream = psx->stream;
    LONG                lSampleSize = psx->lSampleSize;
    LONG                lSave = l;
    LONG                lPosSave = lPos;
    PAVIINDEX           px = psx->px;

    Assert(px && l >= 0 && l < px->nIndex);

    if (lSampleSize == 0) {

        for (l--;l>=0;l--) {

            if (IndexStream(px, l) != bStream)
                continue;

	    if (!(IndexFlags(px, l) & IDX_NOTIME))
		lPos -= 1;

            if (!flags || (IndexFlags(px, l) & flags))
                return TRUE;
        }
    }
    else {
        for (l--;l>=0;l--) {

            if (IndexStream(px, l) != bStream)
                continue;

            lPos -= IndexLength(px, l) / lSampleSize;

            if (!flags || (IndexFlags(px, l) & flags))
                return TRUE;
        }
    }

    lPos = lPosSave;
    l    = lSave;

    return FALSE;
}

///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////

static LONG SearchIndex(PSTREAMINDEX psx,LONG lPos,UINT uFlags,IDXPOS FAR *pos)
{
    LONG                l;
    LONG                lScan;
    LONG                lFound;
    LONG                lFoundPos;
    LONG                lLen;
    UINT                flags;
    PAVIINDEX           px = psx->px;

    Assert(psx);
    Assert(psx->px);

    if (psx == NULL)
        return ERR_POS;

    if (lPos < psx->lStart)
        return ERR_POS;

    if (lPos >= psx->lEnd)
        return ERR_POS;

    //
    // figure out where to start in the index.
    //
    if (psx->lx != -1) {
        lScan  = psx->lPos;
        l      = psx->lx;
    }
    else {
        DPF3("Starting index search at begining\n");
        lScan = psx->lStart;

        for (l=0; l<px->nIndex; l++)
            if (IndexStream(px, l) == (UINT)psx->stream)
                break;
    }

    Assert(l >= 0 && l < px->nIndex);
    Assert(IndexStream(px, l) == psx->stream);

#ifdef DEBUG
    if (!(uFlags & FIND_DIR))
        uFlags |= FIND_PREV;

    switch (uFlags & (FIND_TYPE|FIND_DIR)) {
        case FIND_NEXT|FIND_KEY:    DPF3("SearchIndex(%d): %ld next key, start=%ld",psx->stream, lPos, lScan); break;
        case FIND_NEXT|FIND_ANY:    DPF3("SearchIndex(%d): %ld next any, start=%ld",psx->stream, lPos, lScan); break;
        case FIND_NEXT|FIND_FORMAT: DPF3("SearchIndex(%d): %ld next fmt, start=%ld",psx->stream, lPos, lScan); break;
        case FIND_NEXT:             DPF3("SearchIndex(%d): %ld next    , start=%ld",psx->stream, lPos, lScan); break;

        case FIND_PREV|FIND_KEY:    DPF3("SearchIndex(%d): %ld prev key, start=%ld",psx->stream, lPos, lScan); break;
        case FIND_PREV|FIND_ANY:    DPF3("SearchIndex(%d): %ld prev any, start=%ld",psx->stream, lPos, lScan); break;
        case FIND_PREV|FIND_FORMAT: DPF3("SearchIndex(%d): %ld prev fmt, start=%ld",psx->stream, lPos, lScan); break;
        case FIND_PREV:             DPF3("SearchIndex(%d): %ld prev    , start=%ld",psx->stream, lPos, lScan); break;
    }

    LONG time = timeGetTime();
#endif

    lLen = psx->lSampleSize == 0 ? 1 : IndexLength(px, l) / psx->lSampleSize;

    if (lScan+lLen <= lPos) {
        //
        // search forward for this position
        //
        while (lScan <= lPos) {

            lFound = l;
            lFoundPos = lScan;

            if (lScan == lPos)
                break;

            if (!StreamNext(psx, l, lScan, IDX_KEY|IDX_NONKEY))
                break;
        }

        if ((lScan > lPos) && !(uFlags & FIND_NEXT)) {
            lScan = lFoundPos;
            l     = lFound;
        }
    }
    else if (lScan > lPos) {
        //
        // search backward for this position
        //
        while (lScan > lPos) {

            lFound = l;
            lFoundPos = lScan;

            if (!StreamPrev(psx, l, lScan, IDX_KEY|IDX_NONKEY))
                break;
        }

        if (uFlags & FIND_NEXT) {
            lScan = lFoundPos;
            l     = lFound;
        }
    }
    else {
        Assert(lScan <= lPos && lPos < lScan+lLen);
    }

    Assert(l >= 0 && l < px->nIndex);
    Assert(IndexStream(px, l) == psx->stream);

    //
    //  cache what we found.
    //
    psx->lx   = l;
    psx->lPos = lScan;

    if (uFlags & FIND_TYPE) {

        switch (uFlags & FIND_TYPE) {
            case FIND_ANY:      flags = IDX_KEY|IDX_NONKEY; break;
            case FIND_FORMAT:   flags = IDX_PAL;            break;
            case FIND_KEY:      flags = IDX_KEY;            break;
        }

        if (!(IndexFlags(px, l) & flags)) {

            if (!(uFlags & FIND_NEXT)) {
                if (!StreamPrev(psx, l, lScan, flags)) {
                    DPF3("!, EOI, time = %ld\n", timeGetTime() - time);
                    return ERR_POS;
                }
            }
            else {
                if (!StreamNext(psx, l, lScan, flags)) {
                    DPF3("!, EOI, time = %ld\n", timeGetTime() - time);
                    return ERR_POS;
                }
            }
        }

        Assert(l >= 0 && l < px->nIndex);
        Assert(IndexStream(px, l) == psx->stream);
        Assert(IndexFlags(px, l) & flags);
    }

    Assert(lScan >= psx->lStart && lScan < psx->lEnd);

    DPF3("!, found %ld, time = %ld\n", lScan, timeGetTime() - time);

    if (pos == NULL)
        return lScan;

    if (psx->lSampleSize != 0) {

        lLen = IndexLength(px, l);

        if (lLen == MAX_LENGTH-1)
            lLen = 0x7FFFFFFF;

        if (psx->lSampleSize > 1)
            lLen /= psx->lSampleSize;
    }
    else {
        lLen = 1;
    }

    pos->lx      = l;
    pos->lPos    = lScan;
    pos->lSize   = lLen;
    pos->lOffset = IndexOffset(px, l);
    pos->lLength = IndexLength(px, l);

    //
    //  if the FIND_TYPE is not one of FIND_ANY, FIND_KEY, FIND_FORMAT
    //  make sure we realy found the wanted sample.
    //
    if ((uFlags & FIND_TYPE) == 0) {
        if (lPos < lScan || lPos >= lScan+lLen) {
            pos->lOffset = -1;
            pos->lLength = 0;
            pos->lSize   = 0;
            pos->lPos    = lPos;
        }
        else if (psx->lSampleSize > 0) {
            pos->lOffset += (lPos - lScan) * psx->lSampleSize;
            pos->lLength -= (lPos - lScan) * psx->lSampleSize;
            pos->lSize   -= (lPos - lScan);
            pos->lPos     = lPos;
        }
    }

    return pos->lPos;
}

/***************************************************************************
 *
 * @doc INTERNAL MCIAVI
 *
 * @api LONG | FindSample | find a sample in a stream
 *
 ***************************************************************************/

EXTERN_C LONG StreamFindSample(PSTREAMINDEX psx,LONG lPos,UINT uFlags)
{
    Assert(psx);
    Assert(psx->px);

    if (lPos < psx->lStart)
        return ERR_POS;

    if (lPos >= psx->lEnd)
        return ERR_POS;

    if ((uFlags & FIND_RET) == FIND_POS) {

        switch (uFlags & FIND_TYPE) {
            case FIND_FORMAT:
                if (psx->lPalFrames == 0) {
                    if ((uFlags & FIND_NEXT) && lPos > psx->lStart)
                        return ERR_POS;
                    else
                        return psx->lStart;
                }
                break;

            case FIND_ANY:
                if (psx->lNulFrames == 0) {
                    return lPos;
                }
                break;

            case FIND_KEY:
                if (psx->lKeyFrames == psx->lFrames) {
                    return lPos;
                }
                break;

            default:
                return lPos;
        }

        return SearchIndex(psx, lPos, uFlags, NULL);
    }
    else {
        IDXPOS pos;

        if (SearchIndex(psx, lPos, uFlags, &pos) == ERR_POS)
            return ERR_POS;

        switch (uFlags & FIND_RET) {
            case FIND_POS:
                return pos.lPos;

            case FIND_OFFSET:
                return pos.lOffset + 8;

            case FIND_LENGTH:
                return pos.lLength;

            case FIND_SIZE:
                return pos.lSize;

            case FIND_INDEX:
                return pos.lx;
        }
    }

    return ERR_POS;
}

/***************************************************************************
 *
 * @doc INTERNAL MCIAVI
 *
 * @api LONG | StreamRead | read from a stream
 *
 ***************************************************************************/

EXTERN_C LONG StreamRead(
    PSTREAMINDEX  psx,
    LONG          lStart,
    LONG          lSamples,
    LPVOID        lpBuffer,
    LONG          cbBuffer)
{
    LONG          lBytes;
    LONG          lSampleSize;
    LONG          lSeek;
    LONG          lRead;
    IDXPOS        pos;

    Assert(psx);
    Assert(psx->px);
    Assert(psx->hFile);
    Assert(psx->Read);

    if (lStart < psx->lStart)
        return -1;

    if (lStart >= psx->lEnd)
        return -1;

    //
    // find nearest chunk
    //
    if (SearchIndex(psx, lStart, FIND_PREV, &pos) == ERR_POS)
        return -1;

    //
    // only continue if the sample we want is in here.
    //
    if (lStart < pos.lPos || lStart >= pos.lPos + pos.lSize)
        return 0;

    //
    // if they give us a NULL buffer dummy up the cbBuffer so we return
    // what we would have read if we had enough room
    //
    if (lpBuffer == NULL && cbBuffer == 0 && lSamples != 0)
        cbBuffer = 0x7FFFFFFF;

    if (lSampleSize = psx->lSampleSize) {

        // If they wanted to read/write only a "convenient amount",
        // pretend the buffer is only large enough to hold the
        // rest of this chunk.

        if (lSamples == -1l)
            cbBuffer = min(cbBuffer, pos.lLength);

        /* Fixed-length samples, if lSamples is zero, just fill the buffer. */

        if (lSamples > 0)
            lSamples = min(lSamples, cbBuffer / lSampleSize);
        else
            lSamples = cbBuffer / lSampleSize;

        lBytes = lSamples * lSampleSize;
    } else {
        lBytes = pos.lLength;
    }

    if (lpBuffer == NULL)
        return lBytes;

    if (cbBuffer < lBytes)
        return -1;   // buffer is too small

#define WORDALIGN(x) ((x) + ((x) & 1))
    
    if (lSampleSize == 0)
    {
        DWORD adw[2];
        psx->Read(psx->hFile, pos.lOffset, sizeof(adw), adw);
        Assert(StreamFromFOURCC(adw[0]) == psx->stream);
        Assert(WORDALIGN(adw[1]) == WORDALIGN((DWORD)pos.lLength));
	pos.lLength = adw[1];	// !!! Make netware video work!
	lBytes = pos.lLength;
    }
    else
    {
#ifdef DEBUG
        IDXPOS x;
        DWORD  adw[2];
        SearchIndex(psx, lStart, FIND_PREV|FIND_ANY, &x);
        psx->Read(psx->hFile, x.lOffset, sizeof(adw), adw);
        Assert(StreamFromFOURCC(adw[0]) == psx->stream);
        Assert(WORDALIGN(adw[1]) == WORDALIGN((DWORD)x.lLength));
#endif
    }

    cbBuffer = lBytes;
    lBytes = 0;

    while (cbBuffer > 0) {

        lSeek = pos.lOffset + 8;
        lRead = min(pos.lLength,cbBuffer);

	if (lRead <= 0) {
            DPF3("!!!! lRead <= 0 in AVIStreamRead\n");
	    break;
        }

        DPF3("StreamRead: %ld bytes @%ld\n", lRead, lSeek);

        if (psx->Read(psx->hFile, lSeek, lRead, lpBuffer) != lRead)
            return -1;

	lBytes   += lRead;
	cbBuffer -= lRead;

	if (cbBuffer > 0) {
	    if (lSampleSize == 0) {
		DPF("%ld bytes to read, but sample size is 0!\n", cbBuffer);
		break;
	    }

            lpBuffer = (LPVOID) (((BYTE _huge *)lpBuffer) + lRead);

            lStart += lRead / lSampleSize;
            lStart = SearchIndex(psx, lStart, FIND_PREV, &pos);

            if (lStart == ERR_POS)
		break;
        }
    }

    //
    // success return number of bytes read
    //
    return lBytes;
}

/***************************************************************************
 *
 * @doc INTERNAL MCIAVI
 *
 * @api LONG | StreamWrite | write to a stream
 *
 ***************************************************************************/

EXTERN_C LONG StreamWrite(
    PSTREAMINDEX  psx,
    LONG          lStart,
    LONG          lSamples,
    LPVOID        lpBuffer,
    LONG          cbBuffer)
{
    return -1;
}
