#include <windows.h>
#include <windowsx.h>
#include <win32.h>
#include <mmsystem.h>
#include <vfw.h>
#include "aviview.h"
#include "audplay.h"
#include "aviplay.h"

#ifdef DEBUG
    extern void FAR CDECL dprintf(LPSTR, ...);
    #define DPF dprintf
    #define DPF2 / ## /
    #define DPF3 / ## /
#else
    #define DPF / ## /
    #define DPF2 / ## /
    #define DPF3 / ## /
#endif

#define ProfBegin() ProfClear(); ProfSampRate(5,1); ProfStart();
#define ProfEnd()   ProfStop(); ProfFlush();

#define FillR(hdc, x, y, dx, dy, rgb) \
    SetBkColor(hdc, rgb);             \
    SetRect(&rc, x, y, x+dx, y+dy);   \
    ExtTextOut(hdc, 0, 0, ETO_OPAQUE, &rc, NULL, 0, NULL);

// Why is this necessary?
#define HPBYTE	BYTE huge *

// !!! All of this is out of thin air
#define MAXNUMSTREAMS		10
#define AUD_BUFFERS_MAX_SIZE	4096L	// never read > this many bytes
#define NUM_WAVE_HEADERS	32

#define YIELD_WAIT_TIME		150
#define READ_WAIT_TIME		150
#define DECOMPRESS_WAIT_TIME	150
#define DRAW_WAIT_TIME		150

#ifndef WIN32
extern LONG FAR PASCAL muldiv32(LONG, LONG, LONG);
#endif

extern BOOL gfCheat, gfDecompress;	// do we cheat? does our queue hold
					// compressed/decompressed data?
extern BOOL gfYieldBound, gfReadBound;	// pretend to be these things?
extern BOOL gfDecompressBound, gfDrawBound;

static LPVOID AllocMem(DWORD dw);

///////////////////////////////////////////////////////////////////////////
//
// useful macros
//
///////////////////////////////////////////////////////////////////////////

#define ALIGNULONG(i)     ((i+3)&(~3))                  /* ULONG aligned ! */
#define WIDTHBYTES(i)     ((unsigned)((i+31)&(~31))/8)  /* ULONG aligned ! */
#define DIBWIDTHBYTES(bi) (int)WIDTHBYTES((int)(bi).biWidth * (int)(bi).biBitCount)
#define DIBPTR(lpbi) ((LPBYTE)(lpbi) + \
	    (int)(lpbi)->biSize + \
	    (int)(lpbi)->biClrUsed * sizeof(RGBQUAD) )

extern BOOL gfPlaying; // are we playing?
extern HWND ghwndApp;
extern HACCEL ghAccel;

// for DPF's
int		Skip, Jump, Empty, Cheat;
LONG		frPlayed, frStart;
BOOL		fAudioBroke;

// Decide whether or not you want the profiler running
#define PROFILE

#ifdef PROFILE
    #define TIME(x) \
        LONG time ## x,   cnt ## x;

    #define ZERO(x) \
        time ## x = 0, cnt ## x = 0

    #define START(x) \
        time ## x -= (LONG)timeGetTime()

    #define END(x) \
        (time ## x += (LONG)timeGetTime()), cnt ## x ++
#else
    #define TIME(x)
    #define ZERO(x)
    #define START(x)
    #define END(x)
#endif

TIME(Total);	// for profiling
TIME(Other);
TIME(Time);
TIME(Key);
TIME(Read);
TIME(Copy);
TIME(Draw);
TIME(Audio);
TIME(Decomp);
TIME(Yield);
TIME(Free);

// wFlags in Q structure
#define FAST_TEMPORAL		1

typedef struct {
    HWND 	hwnd;
    HDC		hdc;
    LPBITMAPINFOHEADER lpbi;
    HDRAWDIB	hdd;
    RECT	rc;
} VIDEORENDER, FAR *LPVIDEORENDER;

typedef struct {
    HWAVEOUT	hWaveOut;
    WAVEHDR	wavehdr[NUM_WAVE_HEADERS];
    BOOL	fBusy[NUM_WAVE_HEADERS];	// waiting for WHDR_DONE
} AUDIORENDER, FAR *LPAUDIORENDER;

typedef struct _QENTRY {
    LONG        start;          // first TIME!!! sample number in our buffer
    LONG	length;		// number of samples in our buffer
    LONG	size;		// length in bytes of the buffer
    struct _QENTRY huge *next;	// where to find the next bunch of data
		// after struct comes the compressed data from the stream
} QENTRY, huge *HPQENTRY;


typedef struct {
    PAVISTREAM  pavi;	// stream we'll read from
    DWORD	fccType;// what kind of stream is this? (eg. streamtypeVIDEO)
    DWORD       dwSampleSize;   // 0 if samples are variable length

    //!!!!
    HIC		hic;		// a compressor that can decompress the stream
    WORD        wFlags;		// eg. FAST_TEMPORAL

    LPVOID lpfmtIn;		// format of the compressed data in the stream
    LPVOID lpfmtOut;    	// decompressed format stored in queue

    LPVOID lpRender;		// handle to rendering information
    //!!!

    LONG	timeStart;	// the time when the stream started playing
    LONG	sampleStart;	// the first sample number we played
    LONG	streamEnd;	// the last sample in the stream

    HPBYTE	buffer; 	// points to the beginning of the buffer
    LONG	bufsize;	// size of buffer
    LONG	buffree;	// how many of these bytes are free?
    HPQENTRY	head;		// where next chunk from disk is written
    HPQENTRY	tail;		// other end of queue - start of real data
    HPQENTRY	read;		// where render will read from to get data
    HPQENTRY    recent; 	// last thing read into the q (to update ->next)
    int         count;  	// how many chunks of junk in the queue?
    int         countR;  	// how many of them haven't started rendering?

    long        pos;    	// which sample we'll start reading next
    DWORD	dwSampPerRead; 	// How many samples to read at a time

    HPBYTE	lpBitsIn;	// buffer for a stream read
    LONG	cbBitsIn;	// size of buffer
    LONG	cbBitsInUsed;	// amount of data read into buffer
    BOOL	fBitsInReady;	// does this buffer have frame data in it?
    LONG	lBitsInSample;	// 1st sample in the buffer

} QUEUE, *PQUEUE, FAR *LPQUEUE;

typedef struct {
    int		count;
    LPQUEUE	queue[MAXNUMSTREAMS];
} AVIQUEUE, *PAVIQUEUE, far *LPAVIQUEUE;

// global - for aviTime()
LPAVIQUEUE	qAVI;

/***************************************************************************/
/***************************************************************************/
/*****  INTERNAL FUNCTIONS THAT KNOW ABOUT SPECIFIC STREAM TYPES  **********/
/***************************************************************************/
/***************************************************************************/


//
// Wait the specified number of milliseconds
//
void NEAR PASCAL Wait(LONG msec)
{
    LONG	l;

    l = timeGetTime();
    while ((LONG)timeGetTime() < l + msec);
    return;
}


//
// Determine if the given stream is compressed.
//
BOOL NEAR qIsCompressed(LPQUEUE q)
{
    if (q->fccType == streamtypeVIDEO) {
	// If we want DRAWDIB to decompress for us, just pretend we're
	// not comrpressed!
	return gfDecompress &&
		((LPBITMAPINFOHEADER)q->lpfmtIn)->biCompression != BI_RGB;
    } else {
	return FALSE;	// !!! decompression is pretty video specific now.
    }
}


//
// Return how much space it will take to decompress the given bits
//
LONG NEAR qDecompressedSize(LPQUEUE q, HPBYTE hp, LONG	cb)
{
    if (q->fccType == streamtypeVIDEO) {
	if (qIsCompressed(q))
	    return ((LPBITMAPINFOHEADER)q->lpfmtOut)->biSizeImage;
	else
	    return cb;
    } else {
	return cb;	// it's not compressed
    }
}


//
// Locate and return the HIC of a compressor that can decompress the given
// type of format from the given type of stream.
// This will return the output format that it will decompress into.
//
HIC NEAR qLocate(LPQUEUE q, LPVOID FAR *lplpfmtOut) {
    DWORD   fccHandler;
    HIC	    hic;
    LONG    cb;

    if (q->fccType == streamtypeVIDEO) {

	if (lplpfmtOut == NULL)
	    return NULL;

	// ICM won't search for compressors to decompress BI_RLE8.
	// We need to provide the handler of a known decompressor that comes
	// with our AVIFile read API code. !!! HACK
	if (((LPBITMAPINFOHEADER)q->lpfmtIn)->biCompression == BI_RLE8)
	    fccHandler = mmioFOURCC('R','L','E',' ');
	else
            fccHandler = 0;

        // trust that the default format to decompress to is something usable
        hic = ICLocate(ICTYPE_VIDEO, fccHandler, q->lpfmtIn, NULL,
		ICMODE_DECOMPRESS);
	if (hic == NULL)
	    return NULL;

	// get ready for the Decompress calls we'll be making later
        *lplpfmtOut = GlobalAllocPtr(GMEM_MOVEABLE,
		sizeof(BITMAPINFOHEADER) + 256*sizeof(RGBQUAD));
	if (*lplpfmtOut == NULL) {
	    ICClose(hic);
	    return NULL;
	}

        //!!! pass the size of the video so it knows whether to dither or not.
	// Making it stretch the dither will look ugly!
        ICGetDisplayFormat(hic, q->lpfmtIn, *lplpfmtOut, 0, 0, 0);
	// !!! ICM bug... biSizeImage is not set right by ICGetDisplayFormat
	// Luckily I happen to know it's uncompressed and can set it myself
	((LPBITMAPINFOHEADER)*lplpfmtOut)->biSizeImage =
		((LPBITMAPINFOHEADER)*lplpfmtOut)->biHeight *
		DIBWIDTHBYTES(*(LPBITMAPINFOHEADER)(*lplpfmtOut));

    } else {
	hic = NULL;

	if (hic) {
	    // get ready for the Decompress calls we'll be making later
	    cb = ICDecompressGetFormatSize(hic, q->lpfmtIn);
            *lplpfmtOut = GlobalAllocPtr(GMEM_MOVEABLE, cb);
	    if (*lplpfmtOut == NULL) {
	        ICClose(hic);
	        return NULL;
	    }
	    ICDecompressGetFormat(hic, q->lpfmtIn, *lplpfmtOut);
	}
    }

    return hic;
}


//
// Set flags specific to the type of data we're handling.
//
WORD NEAR qSetFlags(LPQUEUE q)
{
    ICINFO	icinfo;
    WORD	wFlags = 0;

    if (q->fccType == streamtypeVIDEO) {
	// Figure out if we can do fast temporal compression or not.
	// (Do we need to decompress on top of the previous frame?)
	if (q->hic) {
	    ICGetInfo(q->hic, &icinfo, sizeof(ICINFO));
	    if (icinfo.dwFlags & VIDCF_FASTTEMPORALD)
	        wFlags |= FAST_TEMPORAL;
	}
    }

    q->wFlags = wFlags;
    return wFlags;
}


//
// Pick an arbitrary size to make the buffer in the queue
//
BOOL NEAR qBufferStuff(LPQUEUE q)
{
    LONG	cb;

    // !!! Pick a better size for our video queue?
    #define NUM_VID_BUFFERS	6

    if (q->fccType == streamtypeVIDEO) {
	if (q->lpfmtOut) {	// stream is compressed, use uncompressed size
	    cb = ((LPBITMAPINFOHEADER)(q->lpfmtOut))->biSizeImage;
	    q->bufsize = cb * NUM_VID_BUFFERS + NUM_VID_BUFFERS *sizeof(QENTRY);
	} else {	// stream is not compressed, use input size
	    cb = ((LPBITMAPINFOHEADER)(q->lpfmtIn))->biSizeImage;
	    q->bufsize = cb * NUM_VID_BUFFERS + NUM_VID_BUFFERS *sizeof(QENTRY);
	}
	q->dwSampPerRead = 1;
	q->buffree = q->bufsize;


    // We want n reads of Audio to exactly fill the buffer so no space is wasted
    // and we'll never waste time trying to read into buffers that are too small
    } else if (q->fccType == streamtypeAUDIO) {
	if (q->dwSampleSize == 0) {
	    DPF("***********ASSERT! Audio has variable sample size!");
	}
	q->dwSampPerRead = AUD_BUFFERS_MAX_SIZE / q->dwSampleSize;
	cb = AUD_BUFFERS_MAX_SIZE;
	q->bufsize =  cb * NUM_WAVE_HEADERS + NUM_WAVE_HEADERS * sizeof(QENTRY);
	q->buffree = q->bufsize;

    } else {
	cb = 0;
	q->dwSampPerRead = 0;
	q->bufsize = 0;
	q->buffree = q->bufsize;

    }

    // Make a buffer to read the stream data into.  Unfortunately, I have
    // no good way of knowing how big the buffer needs to be.  I can't
    // re-alloc it bigger while we're playing if my guess is wrong (not
    // enough time).  Hopefully 3/2 the uncompressed size is big enough
    //
    // Use DOS Memory for speed
    q->fBitsInReady = FALSE;
    q->cbBitsInUsed = 0;
    q->cbBitsIn = cb * 3 / 2;
    q->lpBitsIn = AllocMem(cb * 3 / 2);	// !!! What is the real size?
    if (q->lpBitsIn == NULL)
        return FALSE;
    return TRUE;
}


// Return the decompressed format
LPVOID NEAR qFmt(LPQUEUE q)
{
    // If we're decompressing, we know it already
    if (qIsCompressed(q))
        return q->lpfmtOut;
    // If not, it's the same as the input format
    else {
	return q->lpfmtIn;
    }
}

// !!! ouch
LPVOID NEAR PASCAL qRead(LPQUEUE q);
BOOL NEAR PASCAL qEat(LPQUEUE q);
LPVOID NEAR PASCAL qPeek(LPQUEUE q);
LONG NEAR PASCAL qPeekSize(LPQUEUE q);


BOOL NEAR qRender(LPQUEUE q, LPVOID lpBits, LONG cbBits, BOOL fRender)
{
    #define VidRender ((LPVIDEORENDER)q->lpRender)
    #define AudRender ((LPAUDIORENDER)q->lpRender)

    if (q->fccType == streamtypeVIDEO) {
        RECT rc = VidRender->rc;
	WORD	wFlags = DDF_SAME_HDC | DDF_SAME_DRAW;

        if (lpBits == NULL)
	    return FALSE;

	// We don't want to draw this, but the decompressor needs to see it
	// (eg. for temporal compression)
	if (!fRender)
	    wFlags |= DDF_DONTDRAW;

        START(Draw);
	DrawDibDraw(VidRender->hdd, VidRender->hdc,
                    rc.left, rc.top,
                    rc.right - rc.left,
                    rc.bottom - rc.top,
                    VidRender->lpbi, lpBits, 0, 0, -1, -1,
                    wFlags);
	if (fRender)
	    qEat(q);	// we're done with this right away - remove w/o render
 	if (!gfDecompress && gfDecompressBound)
	    Wait(DECOMPRESS_WAIT_TIME);
	if (gfDrawBound)
	    Wait(DRAW_WAIT_TIME);
	DPF3("DRAW: Rendered a video frame");
        END(Draw);

	return TRUE;

    } else if (q->fccType == streamtypeAUDIO) {
	BOOL	f = FALSE;
        int	i;
        UINT	w;

	START(Audio);
	//
	// First of all, free up any buffers from the queue that are done
	// !!! Assumes they come back in the order they were sent
	//
	for (i = 0; i < NUM_WAVE_HEADERS; i++) {
	    if (AudRender->fBusy[i] &&
			(AudRender->wavehdr[i].dwFlags & WHDR_DONE)) {
	        DPF3("AUDIO: Wave Buffer %d freed", i);
			qEat(q);	// remove from queue - without rendering it
			AudRender->fBusy[i] = FALSE;
	    }
	}

	for (i = 0; i < NUM_WAVE_HEADERS; i++) {
	    if (!(AudRender->wavehdr[i].dwFlags & WHDR_DONE))
		break;
	}
	if (i == NUM_WAVE_HEADERS && !fAudioBroke &&
					q->pos < AVIStreamEnd(q->pavi)) {
	    DPF("AUDIO: ************** AUDIO BROKE!!! ***************");
	    fAudioBroke = TRUE;
	}

	if (!lpBits || !fRender) {
	    DPF3("AUDIO: No bits to render");
	    END(Audio);
	    return FALSE;
	}

	for (i = 0; i < NUM_WAVE_HEADERS; i++) {
	    if ((AudRender->wavehdr[i].dwFlags & WHDR_DONE) &&
				AudRender->hWaveOut) {
		AudRender->wavehdr[i].lpData = lpBits;
		AudRender->wavehdr[i].dwBufferLength = cbBits;
		AudRender->fBusy[i] = TRUE;
		w = waveOutWrite(AudRender->hWaveOut, &AudRender->wavehdr[i],
			sizeof(WAVEHDR));
		f = TRUE;
		DPF3("AUDIO: Wrote audio buffer %d", i);

		// We used some data - advance the read pointer so the next
		// read will give us new wave data
		qRead(q);

		break;
	    }

	}
	if (i == NUM_WAVE_HEADERS) {	// braces necessary
	    DPF3("AUDIO: Can't render - no free buffers");
	}

	END(Audio);
	return f;
    } else {
	return FALSE;
    }
}

void NEAR qRenderFini(LPQUEUE q)
{
    #define VidRender ((LPVIDEORENDER)q->lpRender)
    #define AudRender ((LPAUDIORENDER)q->lpRender)

    int		i;
    UINT	w;

    if (q->fccType == streamtypeVIDEO) {
	SelectPalette(VidRender->hdc, GetStockObject(DEFAULT_PALETTE), FALSE);
	RealizePalette(VidRender->hdc);

	DrawDibClose(VidRender->hdd);
	ReleaseDC(VidRender->hwnd, VidRender->hdc);

	GlobalFreePtr(VidRender);

    } else if (q->fccType == streamtypeAUDIO) {

	if (AudRender->hWaveOut) {
	    w = waveOutReset(AudRender->hWaveOut);
	    for (i = 0; i < NUM_WAVE_HEADERS; i++) {
	        // set these back to what they were when we prepared them
	        AudRender->wavehdr[i].lpData = (LPBYTE) q->buffer;
	        AudRender->wavehdr[i].dwBufferLength = q->bufsize;
	        waveOutUnprepareHeader(AudRender->hWaveOut,
			&AudRender->wavehdr[i], sizeof(WAVEHDR));
	    }
	    w = waveOutClose(AudRender->hWaveOut);
	}

	GlobalFreePtr(AudRender);

    } else {
    }
}


BOOL NEAR qRenderInit(LPQUEUE q, HWND hwnd, RECT rc)
{
    #define VidRender ((LPVIDEORENDER)q->lpRender)
    #define AudRender ((LPAUDIORENDER)q->lpRender)

    int		i;
    UINT	w;

    if (q->fccType == streamtypeVIDEO) {
	LPVOID		lpfmt = qFmt(q);

	VidRender = (LPVIDEORENDER)GlobalAllocPtr(GMEM_MOVEABLE,
		sizeof(VIDEORENDER));
	if (VidRender == NULL)
	    return FALSE;

	VidRender->hwnd = hwnd;
	VidRender->hdc = GetDC(hwnd);
	VidRender->lpbi = (LPBITMAPINFOHEADER)lpfmt;
	VidRender->hdd = DrawDibOpen();
	VidRender->rc = rc;

	// !!! Error code?
	DrawDibBegin(VidRender->hdd, VidRender->hdc,
	    rc.right - rc.left,
	    rc.bottom - rc.top,
	    VidRender->lpbi,
	    (int)VidRender->lpbi->biWidth,
	    (int)VidRender->lpbi->biHeight, 0);

	DrawDibRealize(VidRender->hdd, VidRender->hdc, FALSE);

	q->timeStart = timeGetTime();	// start the clock

	return TRUE;

    } else if (q->fccType == streamtypeAUDIO) {

	AudRender = (LPAUDIORENDER)GlobalAllocPtr(GMEM_MOVEABLE,
		sizeof(AUDIORENDER));
	if (AudRender == NULL)
	    return FALSE;

	w = waveOutOpen(&AudRender->hWaveOut, WAVE_MAPPER, q->lpfmtIn,
			0, 0L, 0);
	if (w) {	// close the device and try once more before giving up

	    // !!! Hack for known bugs in other people's stuff
	    LPWAVEFORMAT	lpwf = q->lpfmtIn;
	    if (lpwf->wFormatTag == WAVE_FORMAT_PCM) {
	        lpwf->nAvgBytesPerSec = lpwf->nSamplesPerSec*lpwf->nBlockAlign;
	    }

	    sndPlaySound(NULL, 0);
	    w = waveOutOpen(&AudRender->hWaveOut, WAVE_MAPPER, q->lpfmtIn,
			0, 0L, 0);
	}
	if (w) {
	   DPF("AUDIO: *************Cannot open the wave device");
	   AudRender->hWaveOut = NULL;	// paranoia?
	   return FALSE;
	}
		
	for (i = 0; i < NUM_WAVE_HEADERS; i++) {
	    AudRender->fBusy[i] = FALSE;	// not outstanding
	    AudRender->wavehdr[i].dwFlags = 0;
	    AudRender->wavehdr[i].lpData = (LPBYTE) q->buffer;
	    AudRender->wavehdr[i].dwBufferLength = q->bufsize;
	    if (waveOutPrepareHeader(AudRender->hWaveOut,&AudRender->wavehdr[i],
					sizeof(WAVEHDR))) {
		DPF("AUDIO: *************Cannot prepare header %d", i);
		qRenderFini(q);
		return FALSE;
	    }
	    AudRender->wavehdr[i].dwFlags |= WHDR_DONE;  // nuked by Prepare
	}

	// Pause for now so we can start instantly by un-pausing
	waveOutPause(AudRender->hWaveOut);

	// we must pre-stuff our audio wave buffers with all the data we have
	// so that whenever a buffer is free we know it's because it's done with
	// some more data
	// !!! If we have more wavebuffers than data, the leftovers will think
	// !!! that they've completed and destroy some audio!
	while (qRender(q, qPeek(q), qPeekSize(q), TRUE));  //don't remove from Q

	q->timeStart = timeGetTime();	// start the clock

	return TRUE;
    } else {
	return 0;
    }
}



/***************************************************************************/
/***************************************************************************/
/*************  END OF SPECIFIC STREAM TYPE FUNCTIONS   ********************/
/***************************************************************************/
/***************************************************************************/


//
// Determine if a queue is getting low on data.
// Returns the percentage of queue buffer space that is full.
//
int NEAR qStarved(LPQUEUE q)
{
    int i;

    i = (int)(100L - (q->buffree * 100L / q->bufsize));
    DPF3("STARVED: %ld%%", i);
    return i;
}


// Shut down the queueing system
void NEAR PASCAL qFini(LPQUEUE q)
{
    if (q->lpfmtIn)
	GlobalFreePtr(q->lpfmtIn);
    q->lpfmtIn = NULL;

    if (q->hic) {
	ICDecompressEnd(q->hic);
	ICClose(q->hic);

	if (q->lpfmtOut)
	    GlobalFreePtr(q->lpfmtOut);
	q->lpfmtOut = NULL;
    }
    q->hic = NULL;

    if (q->lpBitsIn)
	GlobalFreePtr(q->lpBitsIn);
    q->lpBitsIn = NULL;

    if (q->buffer)
	GlobalFreePtr(q->buffer);
    q->buffer = NULL;
    q->head = q->tail = q->read = NULL;
}


// initialize the queueing system
BOOL NEAR PASCAL qInit(PAVISTREAM pavi, LPQUEUE q)
{
    LONG	cb;
    AVISTREAMINFO avis;

    if (q == NULL)
	return FALSE;

    q->pavi = pavi;
    q->lpfmtIn = q->lpfmtOut = NULL;
    q->buffer = q->lpBitsIn = NULL;
    q->recent = NULL;
    q->hic = NULL;

    if (pavi == NULL)
	return FALSE;

    if (AVIStreamInfo(pavi, &avis, sizeof(avis)) != AVIERR_OK)
	goto qInitError;
    q->fccType = avis.fccType;
    q->dwSampleSize = avis.dwSampleSize;

    // Get the format of the compressed data from the stream
    AVIStreamReadFormat(pavi, AVIStreamStart(pavi), NULL, &cb);
    q->lpfmtIn = GlobalAllocPtr(GMEM_MOVEABLE, cb);
    if (q->lpfmtIn == NULL)
	goto qInitError;
    AVIStreamReadFormat(pavi, 0, q->lpfmtIn, &cb);

    // Maybe we haven't been given the size of a compressed image. Fix it.
    if (((LPBITMAPINFOHEADER)q->lpfmtIn)->biSizeImage == 0)
        ((LPBITMAPINFOHEADER)q->lpfmtIn)->biSizeImage =
		avis.dwSuggestedBufferSize;
    if (((LPBITMAPINFOHEADER)q->lpfmtIn)->biSizeImage == 0)
        ((LPBITMAPINFOHEADER)q->lpfmtIn)->biSizeImage = 20000;	// !!!

    // Open a compressor that can decompress it
    if (qIsCompressed(q)) {

	// Find a decompressor and get the decompressed format
	q->hic = qLocate(q, &(q->lpfmtOut));
	if (q->hic == NULL)
	    goto qInitError;

	ICDecompressBegin(q->hic, q->lpfmtIn, q->lpfmtOut);

    }

    // Pick a buffer size, and the number of samples to read each time
    // !!! Must have a valid output format before we call this.
    if (!qBufferStuff(q))
	goto qInitError;

    qSetFlags(q);

    // Queue starts empty, and is about to read the first sample
    q->count = q->countR = 0;
    q->pos = q->sampleStart = AVIStreamStart(pavi);
    q->streamEnd = AVIStreamEnd(pavi);

    q->buffer = GlobalAllocPtr(GMEM_MOVEABLE, q->bufsize);
    q->head = q->tail = q->read = (HPQENTRY)q->buffer;
    if (q->buffer == NULL)
	goto qInitError;

    return TRUE;

qInitError:
    qFini(q);
    return FALSE;
}


#if 0
// return the previous buffer of the queue
HPSTR NEAR qPrev(LPQUEUE q, HPSTR hp)
{

    // !!! Even if the queue is empty, the old information is still there.

    // Special case that we're at the beginning of the buffer
    if (hp == q->buffer)
	return hp + (QSIZE - 1) * q->size;
    else
	return hp - q->size;
}
#endif


#define HeadBuf(q)	(HPBYTE)((q)->head + 1)
#define TailBuf(q)	(HPBYTE)((q)->tail + 1)
#define ReadBuf(q)	(HPBYTE)((q)->read + 1)
#define BufEnd(q)	(HPBYTE)((HPBYTE)((q)->buffer) + (q)->bufsize)

// Decompress the entry in our input buffer into the queue
BOOL NEAR qDecompress(LPQUEUE q)
{
    LONG	lBytesNeeded, lBytesFree;
    LONG	lKey;
    BOOL	fHack;

    // Sometimes the RLE8 file has RGB frames in it and will blow if we
    // try to decompress them.
    // !!! fix RLEC so this stupid hack is not needed!
    #define qlpbi ((LPBITMAPINFOHEADER)q->lpfmtIn)
    fHack = q->fccType == streamtypeVIDEO &&
		qlpbi->biCompression == BI_RLE8 &&
		(LONG)(qlpbi->biSizeImage) == q->cbBitsInUsed;

    lBytesNeeded = (qIsCompressed(q) && !fHack) ?
	  qDecompressedSize(q, q->lpBitsIn, q->cbBitsInUsed) : q->cbBitsInUsed;

    // How many contiguous bytes do we have left in the queue?
    if ((HPBYTE)(q->tail) <= (HPBYTE)(q->head)) {
	lBytesFree = BufEnd(q) - HeadBuf(q);
	// If head and tail are equal - some special cases
	if ((HPBYTE)(q->tail) == (HPBYTE)(q->head)) {
	    if (q->count > 0)
		lBytesFree = 0;
	    else {
		q->head = q->tail = q->read = (HPQENTRY)q->buffer;
		lBytesFree = BufEnd(q) - HeadBuf(q);
	    }
	}
    } else {
	    lBytesFree = (HPBYTE)(q->tail) - HeadBuf(q);
    }

    // Not enough space in the queue to decompress this frame!
    if (lBytesFree < lBytesNeeded) {
	// Did we fail because we're at the end of the queue?  Then
	// try reading into the beginning of the queue.
	if ((HPBYTE)(q->head) > (HPBYTE)(q->tail) &&
			(HPBYTE)(q->tail) != (HPBYTE)(q->buffer)) {
	    q->head = (HPQENTRY)q->buffer;
	    if (q->countR == 0)
		q->read = q->head;
	    lBytesFree = (HPBYTE)(q->tail) - HeadBuf(q);
	} else {
	    lBytesFree = 0;
	}
    }

    // Still not enough space in the queue?  Then the queue is really full
    if (lBytesFree < lBytesNeeded) {
	DPF3("Q too full to decompress into");
	return FALSE;
    }

    //
    // Now decompress the frame into our buffer
    // If we're not compressed, we can do a straight copy.
    //
    if (fHack || !qIsCompressed(q)) {
	START(Copy);
	hmemcpy(HeadBuf(q), q->lpBitsIn, lBytesNeeded);
	END(Copy);
    } else {

	// !!! This is kind of video specific, isn't it? What will decompressing
	// audio look like?

	START(Key);
	lKey = AVIStreamFindKeyFrame(q->pavi, q->lBitsInSample, 0);
	END(Key);

	// We need to copy of the previous bits and decompress on top of them
	// !!! This assumes the previous entry is the previous bits!
	// !!! And that they're still there!
	if (!(q->wFlags & FAST_TEMPORAL) && lKey != q->lBitsInSample &&
								q->recent) {
	    START(Copy);
	    hmemcpy(HeadBuf(q), q->recent + 1, q->recent->size);
	    END(Copy);
	}

	START(Decomp);
	ICDecompress(q->hic, 0, q->lpfmtIn, q->lpBitsIn, q->lpfmtOut,
		HeadBuf(q));
	if (gfDecompressBound)
	    Wait(DECOMPRESS_WAIT_TIME);
	DPF3("DECOMPRESS: %ld --> %ld bytes", q->cbBitsInUsed, lBytesNeeded);
	END(Decomp);
    }

    // Now recognize that less space is available in the queue
    q->buffree -= lBytesNeeded + sizeof(QENTRY);

    // Fix the link from the previous read to this one in case it's moved
    if (q->recent && q->count)
	q->recent->next = q->head;

    q->recent = q->head;
    q->head->start = q->pos;
    // !!! Audio picked it's own number of samples to read -- video read one
    q->head->length = q->dwSampleSize ? lBytesNeeded / q->dwSampleSize
				: q->dwSampPerRead;
    q->head->size = lBytesNeeded;

    q->head = (HPQENTRY)(HeadBuf(q) + lBytesNeeded);

    if ((HPBYTE)(q->head) > BufEnd(q)) {
	DPF("*************Head went past Buffer End!");
    }

    if ((HPBYTE)(q->head) >= BufEnd(q))
	q->head = (HPQENTRY)q->buffer;

    // Initially, the block we just read in will say its next block will be
    // wherever the new head is.  This could change later.
    q->recent->next = q->head;

    q->pos += q->dwSampleSize ? lBytesNeeded / q->dwSampleSize
				: q->dwSampPerRead;
    q->count++;
    q->countR++;
    q->fBitsInReady = FALSE;

    return TRUE;
}

// read something into the queue
BOOL NEAR qFill(LPQUEUE q)
{
    LONG	lBytesRead;
    HRESULT	hRet;
    BOOL	f = FALSE;

    // If there is already a compressed frame in the buffer, stuff it in the
    // queue before reading another frame.
    if (q->fBitsInReady) {
	DPF3("Purging a previous buffer");
	f = qDecompress(q);
    }
    // The buffer is still full!  I guess the queue was full. Bail.
    if (q->fBitsInReady) {
	DPF3("Can't purge buffer!");
	return FALSE;
    }

    // Seems we're at the end of the stream! No more to read.
    if (q->pos >= q->streamEnd)
	return FALSE;

    START(Read);
    // For fixed sample sizes, read a "convenient" number of samples, ie.
    // how ever many samples are left in this AVI chunk.  Never read more than
    // a certain amount, though.
    if (q->dwSampleSize) {
	// !!! READ CONVENIENT NUMBER!!!
	hRet = AVIStreamRead(q->pavi, q->pos, q->dwSampPerRead, q->lpBitsIn,
		min((DWORD)q->cbBitsIn, q->dwSampPerRead * q->dwSampleSize),
		&lBytesRead, NULL);
    // for variable length samples, just read normally whatever we'd decided
    } else {
	hRet = AVIStreamRead(q->pavi, q->pos, q->dwSampPerRead, q->lpBitsIn,
		q->cbBitsIn, &lBytesRead, NULL);
    }
    if (gfReadBound)
	Wait(READ_WAIT_TIME);
    DPF3("READ: Read %ld bytes", lBytesRead);
    END(Read);

    if (hRet == AVIERR_OK) {
        q->fBitsInReady = TRUE;
        q->cbBitsInUsed = lBytesRead;
        q->lBitsInSample = q->pos;
    } else {
	DPF("******************Stream read failed");
	return FALSE;	// uh oh!  This shouldn't happen!
    }

    // Now decompress into the queue
    return (f || qDecompress(q));
}


// Fill the entire queue
void NEAR qPrime(LPQUEUE q) // Preroll for clockwork people
{
    while (qFill(q))
	;
}



// Return the number of entries in the queue
int NEAR qCount(LPQUEUE q)
{
    return q->count;
}

// Return the lowest sample number in the queue that hasn't been rendered yet.
// If the queue is empty, it will return the next sample it will read, because
// it would then become the lowest sample in the queue.
LONG NEAR PASCAL qSample(LPQUEUE q)
{
    if (q->countR == 0)
	return q->pos;
    else
	return q->read->start;
}


//
// Return a pointer to the lowest sample in the queue that hasn't been
// rendered yet.  Don't eat it out of the queue because it's still needed,
// and don't skip past it or anything.
//
LPVOID NEAR PASCAL qPeek(LPQUEUE q)
{
    if (q->countR == 0)
	return NULL;

    return ReadBuf(q);
}


//
// Return the size of what qPeek() would return.
//
LONG NEAR PASCAL qPeekSize(LPQUEUE q)
{
    if (q->countR == 0)
	return 0;

    return q->read->size;
}


//
// Return a pointer to the lowest sample in the queue.  Don't eat it out of the
// queue because it's still needed, but subsequent Peeks will get newer data
//
LPVOID NEAR PASCAL qRead(LPQUEUE q)
{
    LPVOID	lp;

    if (q->countR == 0)
	return NULL;

    lp = ReadBuf(q);

    // We can't destroy this data yet... so don't move the tail
    q->read = (HPQENTRY)(q->read->next);
    q->countR--;

    // paranoia
    if (q->countR == 0)
	q->read = q->head;

    return lp;
}

//
//  return the Next sample that will be read into the queue.
//
long qPos(LPQUEUE q)
{
    return q->pos;
}


//
//  Remove something from the queue without decompressing or remembering it.
//  But pass it to the renderer in case it needs to see the bits (eg. temporal
//  compression).
//
BOOL NEAR qSkip(LPQUEUE q)
{

    // !!! This code should be identical to qEat() except it renders!

    if (q->count == 0)
	return FALSE;

    // More space is now available in the queue
    q->buffree += sizeof(QENTRY) + q->tail->size;

    if (q->count == 1) {
	// Renderer may need to see the data even though we're skipping it
	// (e.g. for temporal video compression)
	qRender(q, qPeek(q), qPeekSize(q), FALSE);	// DON'T ACTUALLY RENDER
        q->tail = q->read = q->head = (HPQENTRY)q->buffer;
	q->countR = 0;
// !!!	q->recent = NULL;	// hmemcpy needs recent bits for non key frames
    } else {
	qRender(q, qPeek(q), qPeekSize(q), FALSE);	// DON'T ACTUALLY RENDER
	if (q->tail == q->read && q->countR) {
	    q->countR--;
	    q->read = (HPQENTRY)(q->tail->next);
	}
	q->tail = (HPQENTRY)(q->tail->next);
    }

    if ((HPBYTE)(q->tail) >= BufEnd(q)) {
	DPF("******************Tail went past Buffer End!");
    }

    q->count--;
    return TRUE;
}


//
// Remove the first thing in the queue, without sending it to the renderer first
// Returns whether or not anything was removed.
//
BOOL NEAR PASCAL qEat(LPQUEUE q)
{

    // !!! This code should be identical to qSkip() except it doesn't render!

    if (q->count == 0)
	return FALSE;

    // More space is now available in the queue
    q->buffree += sizeof(QENTRY) + q->tail->size;

    if (q->count == 1) {
        q->tail = q->read = q->head = (HPQENTRY)q->buffer;
	q->countR = 0;
// !!!	q->recent = NULL;	// hmemcpy needs recent bits for non key frames
    } else {
	if (q->tail == q->read && q->countR) {
	    q->countR--;
	    q->read = (HPQENTRY)(q->tail->next);
	}
	q->tail = (HPQENTRY)(q->tail->next);
    }

    if ((HPBYTE)(q->tail) >= BufEnd(q)) {
	DPF("******************Tail went past Buffer End!");
    }

    q->count--;

    return TRUE;
}


//
//  set the Next sample to be read into the queue using the time provided
//
void NEAR qSeek(LPQUEUE q, LONG pos)
{
    // !!! Don't necessarily just empty the queue!
    while (qEat(q));	// remove from queue without rendering
    q->fBitsInReady = FALSE;	// never bother to decompress this guy

    // !!! Do something intelligent if they seek to something in the queue
    // !!! already
    if (pos < q->pos) {
	DPF("******************Seeking backwards!");
    }
    q->pos = pos;

    // USED FOR DPRINTF ONLY
    if (q->fccType == streamtypeVIDEO)
	frPlayed = max(frPlayed, pos - 1);
}

//
// Inform a stream what time it is.
//
void NEAR PASCAL qInformTime(LPQUEUE q, LONG time)
{
    LONG	fr, frNextKey, frPrevKey;

    if (q->fccType == streamtypeVIDEO) {
	LONG	frEnd;

	// What frame of video should be showing?
	fr = AVIStreamTimeToSample(q->pavi, time) + q->sampleStart;
	DPF2("VIDEO: Time for frame %ld", fr);

	// Don't go past the end
	frEnd = AVIStreamEnd(q->pavi);
	if (fr >= frEnd)
	    return;

	// for DPF's at end
	frPlayed = max(frPlayed, fr);

        // The earliest frame available is later in the movie, so we'll
	// need to wait until we can draw something.  (We're caught up).
        if (qSample(q) > fr) {
            START(Free);
            DPF2("VIDEO: First available frame is %ld. Free time!", qSample(q));
            qFill(q);		// read at LEAST one no matter how full we are
	    END(Free);
	    goto DontStarve;	// !!! make sure we're aren't starving?
        }

        //
        // the frame we want is not in the q at all, and not the next frame
        // what should we do?
        //
        if (fr - qPos(q) > 0) {

                START(Key);
                frPrevKey = AVIStreamFindKeyFrame(q->pavi, fr,SEARCH_BACKWARD);
                frNextKey = AVIStreamFindKeyFrame(q->pavi, fr, SEARCH_FORWARD);
                END(Key);

                DPF2("VIDEO: Panic!  qPos=%ld prev=%ld fr=%ld next=%ld",qPos(q),frPrevKey,fr,frNextKey);

		// If the previous key frame is in the queue somewhere, let's
		// draw it !!!
		if (qCount(q) &&
			frPrevKey >= qSample(q) && frPrevKey < qPos(q)) {
		    while (qSample(q) < frPrevKey) {	// find the prev key
			qSkip(q);
                        Skip++;			// remember we skipped a frame
		    }
		    if (qSample(q) == frPrevKey) {
		        DPF2("VIDEO: Found PREV key %ld in queue", frPrevKey);
		        qRender(q, qPeek(q), qPeekSize(q), TRUE); // draw it
			if (gfCheat) Cheat++;	// not at exact time we wanted
		    }
		    if (qCount(q) == 0)
			Empty++;
                }

		// !!! Random if statement
		if (frPrevKey >= qPos(q) &&
				(fr - frPrevKey) <= (frNextKey - fr)) {

                    DPF2("VIDEO: Jumping %d to PREV key frame %ld", (int)(frPrevKey - qSample(q)), frPrevKey);
		    Jump += (int)(frPrevKey - qSample(q));
		    Empty++;
                    qSeek(q, frPrevKey);
		    if (qFill(q)) {		// get prev key frame
		        qRender(q, qPeek(q), qPeekSize(q), TRUE);  // draw it
			if (fr != frPrevKey && gfCheat)
			    Cheat++;		// weren't supposed to draw now
		    }
		    //qFill(q);	// !!! waste of time?
                    return; 	// !!! goto DontStarve would waste time ???

                } else if (frNextKey >= fr && frNextKey < frEnd) {
                    DPF2("VIDEO: Jumping %d to NEXT key frame %ld", (int)(frNextKey - qSample(q)), frNextKey);
		    Jump += (int)(frNextKey - qSample(q));
		    Empty++;
		    if (frPrevKey >= qPos(q)) {
                        qSeek(q, frPrevKey);
		        if (qFill(q)) {			// get prev key frame
		            qRender(q, qPeek(q), qPeekSize(q), TRUE); // draw it
			    Jump -= 1;			// we didn't jump this 1
			    if (gfCheat) Cheat++;
		        }
		    }
                    qSeek(q, frNextKey);
		    qFill(q);	// put something in the empty queue
                    return; // !!! goto DontStarve ???

                } else {	// braces necessary
		    DPF2("VIDEO: Not jumping anywhere. End of movie");
		}
        }

        // The frame available is too early, get some more frames until we
	// have the one we need.
        while (!qCount(q) || qSample(q) < fr) {
                DPF2("VIDEO: We're behind!  Count=%d, Available=%ld Want=%ld", qCount(q), qSample(q), fr);

                if (qCount(q) == 0) {  // get another sample if we're empty
                    Empty++;		// remember we've been empty
		    DPF2("VIDEO: Queue is empty. Reading frame %ld, want frame %ld - SAME???", qPos(q), fr);
                    if (!qFill(q)) {   // don't get caught in endless loop
			DPF("VIDEO: ********Assertion failure! Heading south!");
                        break;
                    }
		    continue;
                }

		if (qSample(q) == fr - 1 && gfCheat) {
        	    // Cheat! If we only need to skip one frame, draw it now
        	    // and pretend we never skipped it
	    	    DPF2("VIDEO: Cheating at frame %ld", fr - 1);
	    	    qRender(q, qPeek(q), qPeekSize(q), TRUE);
		    Cheat++;
		    // !!! Return now? Or always draw frame fr next?
		} else {
                    Skip++;		// remember we skipped a frame
                    qSkip(q);		// skip the frame we'll never need
		}

	}

	// Something went wrong, abort
        if (qSample(q) != fr) {
                DPF("VIDEO: ***********Assertion failure!  Wanted frame %ld but we died at frame %ld with %d entries in the queue", fr, qSample(q), qCount(q));
		// !!! How to abort the main loop from here?
                return;
        }

        // Read something if we're empty - we're about to need it
	if (qCount(q) == 0) {
		DPF("VIDEO: *************Why are we empty?");
                Empty++;		// remember we've been empty
		if (!qFill(q))		// can't draw if this fails
		    return;
	}

        // Eat a frame and draw it
        qRender(q, qPeek(q), qPeekSize(q), TRUE);

DontStarve:

	// It's bad to let the queue get too empty
        while (qStarved(q) < 50 && qFill(q)) {	// braces necessary
	    DPF2("VIDEO: Filling a starving queue");
	}

    } else if (q->fccType == streamtypeAUDIO) {

	// I don't care what time it is, I'm going to read and play audio
	// as fast as I possibly can!

	// Send all the information we can to the wave device to make sure
	// audio never breaks.  If there's nothing to send, it'll at least
	// notice all of the buffers that are done and let them be re-used.
	while (qRender(q, qPeek(q), qPeekSize(q), TRUE));

	// Now Read and render at least one chunk of audio
	qFill(q);
	qRender(q, qPeek(q), qPeekSize(q), TRUE);

	// If we're starving, keep reading and rendering
	while (qStarved(q) < 50 && qFill(q)) {
	    DPF2("AUDIO: Filling a starving queue");
	    qRender(q, qPeek(q), qPeekSize(q), TRUE);
	}

    } else {
	return;
    }
}

// !!! static variables
static LONG timeDriverPrev = -1, timeClockBase, timeBase;


///////////////////////////////////////////////////////////////////////////////
// Video Stream method:
//	Just take the current time minus the start time
// Audio Stream method:
// 	We can't just use timeGetTime cuz the audio clock drifts from the real
// time clock and we need to sync to the audio we're hearing, even if it's the
// wrong time.  But we can't use the wave driver call to find out what time it
// is either, cuz it may only be accurate to 1/5 of a second.  So we have to
// use a combination of the two.
///////////////////////////////////////////////////////////////////////////////
LONG NEAR PASCAL qNow(LPQUEUE q)
{

    if (q->fccType == streamtypeVIDEO) {
	return timeGetTime() - q->timeStart;

    } else if (q->fccType == streamtypeAUDIO) {
        MMTIME	mmtime;
	LONG	now, timeDriver, l;

	//
	// Get the current time
	//
	now = timeGetTime();

	//
	// Ask the wave driver how long it's been playing for
	//
	if (((LPAUDIORENDER)q->lpRender)->hWaveOut) {
            mmtime.wType = TIME_SAMPLES;
            waveOutGetPosition(((LPAUDIORENDER)q->lpRender)->hWaveOut,
		    &mmtime, sizeof(mmtime));
            if (mmtime.wType == TIME_SAMPLES)
	        timeDriver = AVIStreamSampleToTime(q->pavi, q->sampleStart) +
			muldiv32(mmtime.u.sample, 1000,
				((LPWAVEFORMAT)q->lpfmtIn)->nSamplesPerSec);
            else if (mmtime.wType == TIME_BYTES)
	        timeDriver = AVIStreamSampleToTime(q->pavi, q->sampleStart) +
			muldiv32(mmtime.u.cb, 1000,
				((LPWAVEFORMAT)q->lpfmtIn)->nAvgBytesPerSec);
            else
	        timeDriver = -1;
	} else
	    timeDriver = -1;

	//
        // Something's wrong with the audio clock.. just use the main clock
	//
        if (timeDriver == -1) {
	    DPF("AUDIO: **********Can't get current time from audio driver!");
	    return now - q->timeStart;
	}

        //
        // Audio driver still thinks it's playing the same spot as last time
        //
        if (timeDriver == timeDriverPrev) {
	    // !!! Assumes timeDriver was 0 at the beginning of play
            l = now - timeClockBase + timeDriverPrev - timeBase;

	//
        // Ah!  A new sample of audio being played
	//
        } else {
            timeClockBase = now;
            timeDriverPrev = timeDriver;
	    // !!! Assumes timeDriver was 0 at the beginning of play
	    l = timeDriverPrev - timeBase;
        }

        return l;

    } else {
	return timeGetTime() - q->timeStart;
    }
}


//
// Set the first sample we will play, so we can time things properly
// This is the first thing in our queue, or the sample we're going to read next
// if it's empty.
//
void NEAR PASCAL qSetStartSample(LPQUEUE q)
{
    q->sampleStart = qSample(q);
}

//
// Set the start time of the movie to the current time
//
void NEAR PASCAL qStartClock(LPQUEUE q)
{

    q->timeStart = timeGetTime();

    if (q->fccType == streamtypeAUDIO) {
	MMTIME	mmtime;
	LONG	timeDriver;

	if (!((LPAUDIORENDER)q->lpRender)->hWaveOut) {
	    timeBase = timeDriver = -1;
	    DPF("AUDIO:	Can't Start Clock");
	    return;
	}

	// un-pause the device
	waveOutRestart(((LPAUDIORENDER)q->lpRender)->hWaveOut);

	//
	// Ask the wave driver how long it's been playing for
	//
        mmtime.wType = TIME_SAMPLES;
        waveOutGetPosition(((LPAUDIORENDER)q->lpRender)->hWaveOut,
		&mmtime, sizeof(mmtime));
        if (mmtime.wType == TIME_SAMPLES)
	    timeDriver = AVIStreamSampleToTime(q->pavi, q->sampleStart) +
		muldiv32(mmtime.u.sample, 1000,
			((LPWAVEFORMAT)q->lpfmtIn)->nSamplesPerSec);
        else if (mmtime.wType == TIME_BYTES)
	    timeDriver = AVIStreamSampleToTime(q->pavi, q->sampleStart) +
		muldiv32(mmtime.u.cb, 1000,
			((LPWAVEFORMAT)q->lpfmtIn)->nAvgBytesPerSec);
        else
	    timeDriver = -1;

	timeBase = timeDriver;
    }
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////    Top layer q routines an application would call    //////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

LPAVIQUEUE NEAR PASCAL QueueInit(PAVIFILE pfile)
{
    PAVISTREAM	pavi;
    LPAVIQUEUE	aviQ;
    int		i;

    aviQ = (LPAVIQUEUE)GlobalAllocPtr(GMEM_MOVEABLE, sizeof(AVIQUEUE));
    if (!aviQ)
	return NULL;

    for (i = 0; i < MAXNUMSTREAMS; i++) {
	if (AVIFileGetStream(pfile, &pavi, 0L, i) != AVIERR_OK)
	    break;
        aviQ->queue[i] = (LPQUEUE)GlobalAllocPtr(GMEM_MOVEABLE, sizeof(QUEUE));
	if (!qInit(pavi, aviQ->queue[i]))
	    goto QIError;
    }
    aviQ->count = i;

    if (i == 0)
	return NULL;

    return aviQ;

QIError:
    for (--i; i >= 0; i--) {
	if (aviQ->queue[i]) {
	    GlobalFreePtr(aviQ->queue[i]);
	    qFini(aviQ->queue[i]);
	}
    }
    return NULL;
}


//
// Throw away all the queue stuff
//
void NEAR PASCAL QueueFini(LPAVIQUEUE q)
{
    int	i;

    // Just tell each stream to seek there
    for (i = 0; i < q->count; i++) {
	qFini(q->queue[i]);
	AVIStreamClose(q->queue[i]->pavi);
	GlobalFreePtr(q->queue[i]);
    }

    GlobalFreePtr(q);
}


//
// Seek to a certain spot in the movie.  We are given a TIME and convert it to
// a sample number for each stream.  This is the point we'll start playing
// from, so remember this sample number.
//
void NEAR PASCAL QueueSeek(LPAVIQUEUE q, LONG time)
{
    int	i;

    // Just tell each stream to seek there
    for (i = 0; i < q->count; i++) {
	qSeek(q->queue[i], AVIStreamTimeToSample(q->queue[i]->pavi, time));
	qSetStartSample(q->queue[i]);
	// FOR DPF ONLY
	if (q->queue[i]->fccType == streamtypeVIDEO)
	    frStart = frPlayed = qPos(q->queue[i]);
    }

}


//
// Prime all the queues -- (fill them up entirely)
//
void NEAR PASCAL QueuePrime(LPAVIQUEUE q)
{
    int	i;

    // Prime each queue
    for (i = 0; i < q->count; i++) {
	qPrime(q->queue[i]);
    }
}


//
// Get the time when the movie ends
//
LONG NEAR PASCAL QueueGetEndTime(LPAVIQUEUE q)
{
    int		i;
    LONG	time = 0;

    // Ask each stream
    for (i = 0; i < q->count; i++) {
	time = max(time, AVIStreamEndTime(q->queue[i]->pavi));
    }
    return time;
}


//
// Get the decompressed video format that will be used to display this movie
// (use the first video stream found).  If DRAWDIB is decompressing for us,
// we don't know it and will return some compressed format !!!
//
LPVOID NEAR PASCAL QueueGetVideoDisplayFormat(LPAVIQUEUE q)
{
    int		i;

    // Ask each stream
    for (i = 0; i < q->count; i++) {
	if (q->queue[i]->fccType == streamtypeVIDEO)
	    return qFmt(q->queue[i]);
    }
    return NULL;	// no video streams
}


//
// Prepare to render each stream.  We are passed pointers to an array of
// hwnd's and rc's to use for the different video streams.  Audio streams ignore
// those parameters.
// !!! Does audio need anything passed to it?
// !!! Return an error code?  Abort on error? Continue anyway?
//
BOOL NEAR PASCAL QueueRenderInit(LPAVIQUEUE q, HWND FAR *phwnd, RECT FAR *prc)
{
    int		i, v = 0;

    // Init each stream, give different parms to each video stream
    for (i = 0; i < q->count; i++) {
	qRenderInit(q->queue[i], *(phwnd + v), *(prc + v));
	if (q->queue[i]->fccType == streamtypeVIDEO)
	    v++;
    }
    return TRUE;
}


//
// Finish up rendering.
//
void NEAR PASCAL QueueRenderFini(LPAVIQUEUE q)
{
    int		i;

    // Tell each stream
    for (i = 0; i < q->count; i++) {
	qRenderFini(q->queue[i]);
    }
}


//
// Inform the master queue what time it is.  This should be called often.
//
void NEAR PASCAL QueueInformTime(LPAVIQUEUE q, LONG time)
{
    int	i;

    // Just tell each stream what time it is
    for (i = 0; i < q->count; i++) {
	qInformTime(q->queue[i], time);
    }
}

void NEAR PASCAL QueueStartClock(LPAVIQUEUE q)
{
    int	i;

    // Set the play start time for each stream
    for (i = 0; i < q->count; i++) {
	qStartClock(q->queue[i]);
    }

}


//
// Find out how many milliseconds since we started playing.  Ask an AUDIO stream
// first, if there is one (they have priority).  If not, ask a VIDEO stream.
// If not, ask any stream.
//
LONG NEAR PASCAL QueueNow(LPAVIQUEUE q)
{
    int		i;

    for (i = 0; i < q->count; i++) {
	if (q->queue[i]->fccType == streamtypeAUDIO)
	    return qNow(q->queue[i]);
    }
    for (i = 0; i < q->count; i++) {
	if (q->queue[i]->fccType == streamtypeVIDEO)
	    return qNow(q->queue[i]);
    }
    return qNow(q->queue[0]);
}

BOOL NEAR PASCAL WinYield()
{
    MSG msg;
    BOOL fAbort=FALSE;

    START(Yield);
    DPF2("YIELDING...");
    while(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
    {
	if (msg.message == WM_KEYDOWN && msg.wParam == VK_ESCAPE)
            fAbort = TRUE;
	if (msg.message == WM_SYSCOMMAND && (msg.wParam & 0xFFF0) == SC_CLOSE)
	    fAbort = TRUE;

	if (TranslateAccelerator(ghwndApp, ghAccel, &msg))
	    continue;
	
	TranslateMessage(&msg);
	DispatchMessage(&msg);
    }
    if (gfYieldBound)
	Wait(YIELD_WAIT_TIME);
    END(Yield);
    return fAbort;
}

//
//  should have function take :
//
//      pavis[] and a count
//      along with a "draw" proc () to render wierd custom streams
//

LONG FAR PASCAL aviPlay(HWND hwnd, PAVIFILE pfile, LONG movieStart)
{
    extern UINT gwZoom;

    LONG	l = 0, movieEnd;
    int		iYield=0;
    RECT	rcFrame, rc;
    LPBITMAPINFOHEADER lpbi;
    HDC		hdc;

    // Clear these out, so we don't abort by mistake
    GetAsyncKeyState(VK_ESCAPE);
    GetAsyncKeyState(VK_RBUTTON);

    fAudioBroke = TRUE;

    // Init the queue and fill it up
    if ((qAVI = QueueInit(pfile)) == NULL)
        return movieStart;		// error code?
    QueueSeek(qAVI, movieStart);	// decide where we'll start playing
    QueuePrime(qAVI);			// pre-stuff all the queues


    // When does this AVI end?
    movieEnd = QueueGetEndTime(qAVI);

    GetClientRect(hwnd, &rc);
////PatBlt(hdc, 0, 0, rc.right, rc.bottom, WHITENESS);

    //
    // Calculate the location to play the video based on its frame size
    //
    lpbi = (LPBITMAPINFOHEADER)QueueGetVideoDisplayFormat(qAVI);
    rcFrame.left   = rc.right / 2 -((int)lpbi->biWidth*gwZoom/4)/2;
    rcFrame.top    = 40; //!!! yStreamTop + TSPACE;
    rcFrame.right  = rcFrame.left + (int)lpbi->biWidth*gwZoom/4;
    rcFrame.bottom = rcFrame.top +  (int)lpbi->biHeight*gwZoom/4;

    // Play the AVI in our window, centred.
    // !!! This will die if > 1 video stream in a movie!
    QueueRenderInit(qAVI, &hwnd, &rcFrame);

    hdc = GetDC(hwnd);

    if (WinYield())	// let palette change happen
      //  goto byebye;

//	ProfBegin();

	ZERO(Total);
	ZERO(Other);
	ZERO(Time);
	ZERO(Key);
	ZERO(Read);
	ZERO(Copy);
	ZERO(Draw);
	ZERO(Audio);
	ZERO(Decomp);
	ZERO(Yield);
	ZERO(Free);

	// for DPF
 	Skip = Jump = Empty = Cheat = 0;
	fAudioBroke = FALSE;

 	// Call just before main loop to set start time and waveBase hack
	QueueStartClock(qAVI);

        START(Total);

        while (1) {

	    // We've been told to stop, so do so
	    // -1 means close after stopping
            if (gfPlaying == 0 || gfPlaying == -1)
                break;

	    // What time is it right now?
            START(Time);
            // What time in the movie are we at right now?
	    l = QueueNow(qAVI);    // elapsed time since play start
            END(Time);
	    DPF3("Time %ld", l);

	    // Ah!  The movie is done!
	    if (l > movieEnd - movieStart)
                break;

	    QueueInformTime(qAVI, l);

#ifdef DDEBUG
	    {
	    int i;
	    char ach[128];

            i = wsprintfA(ach,
              "Time %d.%02d S %d J %d E %d            ",
              (int)(l/1000), (int)(l%1000)/10,
              Skip, Jump, Empty);

            SetBkColor(hdc, RGB(255,255,255));
            TextOutA(hdc, 0, 0, ach, i);
	    }
#endif

#ifdef DDEBUG	// !!! move into a specific stream
            #define W 16
            #define H 16
            i = qCount(q) * W;
            FillR(hdc, 4,   20, i, H, RGB(255,255,0));
            FillR(hdc, 4+i, 20, QSIZE*W-i, H, RGB(255,0,0));

            i = (fr - qSample(q)) * W;
            FillR(hdc, 4+i, 20, 1, H, RGB(0,0,0));
#endif

	    // Yield every once in a while.  Always yielding makes performance
	    // plummet.
	    if ((++iYield % 8) == 0) {
	        if (WinYield())
		    break;
	    }

        }

        END(Total);

//	ProfEnd();

        timeOther =
            timeTotal -
            timeTime -
            timeKey  -
            timeRead -
            timeCopy -
            timeDraw -
            timeAudio -
            timeDecomp -
            timeYield;

byebye:

#ifdef PROFILE
    DPF("***********************************************************");
    if (fAudioBroke) {	// braces necessary
        DPF("******************  AUDIO BROKE!!! ************************");
    }
    DPF("Total Frames: %d", frPlayed - frStart + 1);
    DPF("Frames skipped: %d", Skip);
    DPF("Frames  jumped: %d", Jump);
    DPF("Total Frames missed: %d, (%d %%)", Skip + Jump, (int)(100l * (Skip + Jump) / (frPlayed - frStart + 1)));
    DPF("Times cheated: %d", Cheat);
    DPF("Times empty: %d", Empty);

    #define SEC(x)    SECA(x), SECB(x), (timeTotal ? (int)(time ## x * 100 / timeTotal) : 0)
    #define SECA(x)   (time ## x / 1000l) , (time ## x % 1000l)
    #define SECB(x)   (cnt ## x ? (time ## x / cnt ## x / 1000l) : 0l), (cnt ## x ? ((time ## x / cnt ## x) % 1000l) : 0l)

    DPF("    timeTotal:      %3ld.%03ldsec (%3ld.%03ldsec) %d%%",SEC(Total));
    DPF("    timeOther:      %3ld.%03ldsec (%3ld.%03ldsec) %d%%",SEC(Other));
    DPF("    timeTime:       %3ld.%03ldsec (%3ld.%03ldsec) %d%%",SEC(Time));
    DPF("    timeKey:        %3ld.%03ldsec (%3ld.%03ldsec) %d%%",SEC(Key));
    DPF("    timeRead:       %3ld.%03ldsec (%3ld.%03ldsec) %d%%",SEC(Read));
    DPF("    timeCopy:       %3ld.%03ldsec (%3ld.%03ldsec) %d%%",SEC(Copy));
    DPF("    timeDraw:       %3ld.%03ldsec (%3ld.%03ldsec) %d%%",SEC(Draw));
    DPF("    timeAudio:      %3ld.%03ldsec (%3ld.%03ldsec) %d%%",SEC(Audio));
    DPF("    timeDecompress: %3ld.%03ldsec (%3ld.%03ldsec) %d%%",SEC(Decomp));
    DPF("    timeYield:      %3ld.%03ldsec (%3ld.%03ldsec) %d%%",SEC(Yield));
    DPF("");
    DPF("    timeFree:       %3ld.%03ldsec (%3ld.%03ldsec) %d%%",SEC(Free));
    DPF("***********************************************************");
#endif

    ReleaseDC(hwnd, hdc);

    QueueRenderFini(qAVI);
    QueueFini(qAVI);
    InvalidateRect(hwnd, NULL, TRUE);	// we've hosed their DC

    // Tell where the movie stopped
    return movieStart + l;
}

void FAR PASCAL aviStop(void)
{
    gfPlaying = 0;
}

LONG FAR PASCAL aviTime(void)
{
    if (gfPlaying && qAVI)
        return QueueNow(qAVI);
    else
	return -1;
}

/***************************************************************************
 *
 * @doc INTERNAL MCIAVI
 *
 * @api LPVOID | AllocMem | try to allocate DOS memory (< 1Mb)
 *
 * @parm DWORD | dw | size in bytes
 *
 ***************************************************************************/

static LPVOID AllocMem(DWORD dw)
{
#ifndef WIN32
    /* Memory allocation internal routines */

    extern DWORD FAR PASCAL GlobalDosAlloc(DWORD);

    LPVOID p;

    if (p = (LPVOID)MAKELONG(0, LOWORD(GlobalDosAlloc(dw))))
    {
        GlobalReAlloc((HANDLE)HIWORD((DWORD)p), 0, GMEM_MODIFY|GMEM_SHARE);
        return p;
    }
    else
#endif
    {
        DPF("Couldn't get DOS Memory");
        return GlobalLock(GlobalAlloc(GMEM_MOVEABLE|GMEM_SHARE, dw));
    }
}

/*****************************************************************************
 *
 * dprintf() is called by the DPF macro if DEBUG is defined at compile time.
 *
 * The messages will be send to COM1: like any debug message. To
 * enable debug output, add the following to WIN.INI :
 *
 * [debug]
 * AVIView=1
 *
 ****************************************************************************/

#ifdef DEBUG

#define MODNAME "AVIView"

void FAR cdecl dprintf(LPSTR szFormat, ...)
{
    char ach[128];
    va_list va;

    static BOOL fDebug = -1;

    if (fDebug == -1)
        fDebug = GetProfileIntA("Debug", MODNAME, FALSE);

    if (!fDebug)
        return;

    lstrcpyA(ach, MODNAME ": ");
    va_start(va, szFormat);
    wvsprintfA(ach+lstrlenA(ach),szFormat, va);
    va_end(va);
    lstrcatA(ach, "\r\n");

    OutputDebugStringA(ach);
}

#endif
