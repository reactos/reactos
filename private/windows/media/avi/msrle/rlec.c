/*--------------------------------------------------------------------------*\
|   RLEC.C - MS-CRUNCH                                                       |
|//@@BEGIN_MSINTERNAL									      |
|   History:                                                                 |
|   01/01/88 toddla     Created                                              |
|   10/30/90 davidmay   Reorganized, rewritten somewhat.                     |
|   07/11/91 dannymi    Un-hacked                                            |
|   09/15/91 ToddLa     Re-hacked                                            |
|//@@END_MSINTERNAL									      |
|                                                                            |
\*--------------------------------------------------------------------------*/
/**************************************************************************
 *
 *  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
 *  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
 *  PURPOSE.
 *
 *  Copyright (c) 1991 - 1995  Microsoft Corporation.  All Rights Reserved.
 *
 **************************************************************************/

#include <windows.h>
#include <windowsx.h>
#include "msrle.h"

#ifdef _WIN32
#define _huge
#endif

//
//	make a copy of a DIB that is not packed.
//
__inline static LPVOID CopyDib(LPBITMAPINFOHEADER lpbi, LPVOID lpS)
{
    LPVOID lpD;
    BYTE _huge *s;
    BYTE _huge *d;
    long lImageHeader, lImageData, lImageSize;

    if (!lpbi || !lpS)
	return NULL;

    FixBitmapInfo(lpbi);

    lpD = GlobalAllocPtr(GHND, lImageSize = DibSize(lpbi));

    if (lpD)
    {
	// Copy the bitmapinfoheader and colours
	s = (LPVOID)lpbi;
	d = (LPVOID)lpD;
	lImageData = DibSizeImageX(lpbi);  // grab the number of data bytes
	lImageHeader = lImageSize - lImageData;	 // save header+colortable size
#if 0
        while (lImageHeader-- > 0)
            *d++ = *s++;
#else
    	memcpy(d, s, lImageHeader);	   // copy the header+colortable to new Dib
	d += lImageHeader;		   // step pointer to Data piece
#endif

	// Copy the image
	s = (LPVOID)lpS;
#if 0
        while (lImageData-- > 0)
            *d++ = *s++;
#else
    	memcpy(d, s, lImageData);	   // copy data bytes to new Dib
#endif
    }

    return lpD;
}

//
// CrunchDib() - make a DIB fit into a specific size, damnit!
//
BOOL FAR PASCAL CrunchDib(PRLEINST pri,
    LPBITMAPINFOHEADER  lpbiRle, LPBYTE lpRle,
    LPBITMAPINFOHEADER  lpbiFrom,LPBYTE lpFrom,
    LPBITMAPINFOHEADER  lpbiTo,  LPBYTE lpTo)
{
    long dwSize = 0L, dwLastSize = 0L;
    long lCurParm = 0L;
    long lTempMax;             // highest value before halving
    long tolMax;
    long lTempMin = 0L;
    BOOL fInterlaceNow = FALSE; // time to try interlacing?
    long lBumpUp = 2048L;       // bump the parameter up by this amount
    int iStart, iLen;

    BOOL fSpatialAdaptive;
    BOOL fTemporalAdaptive;
    long tolTemporal;
    long tolSpatial;
    int  minJump;
    int  maxRun;

    int FIRSTTRY = 1024;        // use this parameter value as a first guess
    int CWND = 250;             // Give up searching for the perfect parameter
				// when the window is smaller than this

    lTempMax = pri->RleState.tolMax;       // highest value before halving
	
    // No Previous DIB -- we want a full frame, so no interlacing allowed
    // (infinite tolerance allowed before frame halving)
    if (lpbiFrom == NULL)
	lTempMax = MAXTOL;

    // In case we were passed a bogus value -- don't allow frame halving at all
    if (lTempMax < 0)
	lTempMax = MAXTOL;

    tolMax = lTempMax;

    tolTemporal = pri->RleState.tolTemporal;
    tolSpatial  = pri->RleState.tolSpatial;
    fSpatialAdaptive = (pri->RleState.tolSpatial == ADAPTIVE);
    fTemporalAdaptive = (pri->RleState.tolTemporal == ADAPTIVE);
    maxRun      = pri->RleState.iMaxRunLen;
    minJump     = 4;

    // No Previous DIB - we should do a full frame, so no interlacing and
    // allow spatial compression to be adaptive to do the compression since
    // we can't do temporal compression.
    if (lpbiFrom == NULL) {
	pri->iStart = 0;
	fSpatialAdaptive = TRUE;
    }

    iStart = pri->iStart;
    iLen   = -1;

    if (!lpbiTo) {
        DPF(("Crunch Error - Invalid DIB or HPAL"));
	goto return_failure;
    }

    //
    // In the previous frame, we did the bottom only,
    // so now we need to do the top
    //
    // If lpbiFrom is NULL, we don't want to do this--we want to make
    // a full frame, even though the last one was a first half.
    //
    if (iStart > 0 && pri->lpbiPrev) {
	fInterlaceNow = TRUE;	// Only do half of the frame.
        lpbiTo = pri->lpbiPrev;
        lpTo   = DibPtr(lpbiTo); // This will be a packed DIB

	lTempMin = 0L;
	lTempMax = MAXTOL;      // no limit to how fuzzy you can get before
	tolMax   = MAXTOL;	// interlacing since we already are doing it

        DPF(("SECOND HALF OF INTERLACE"));

	//
	//  copy over the color table from the last DIB to the empty RLE
	//  to delay any palette change....
	//
        hmemcpy(lpbiRle,lpbiTo,lpbiTo->biSize+(int)lpbiTo->biClrUsed*sizeof(RGBQUAD));
    } else {
	iStart = 0;
    }

// OK. Here's where we work on getting the frame down in size!

// First, try an EXACT RLE with no fuzziness.  If that works, no need to degrade
// the image quality at all!

    if (!RleDeltaFrame(lpbiRle,lpRle,lpbiFrom,lpFrom,lpbiTo,lpTo,iStart,iLen,0L,0L,0,0)) {
        DPF(("Crunch Error - Lossless RleDeltaFrame failed"));
	goto return_failure;
    }
	
    dwSize = lpbiRle->biSizeImage;

    DPF(("tolTemporal = 0, tolSpatial = 0, Size = %ld", dwSize));

    // Exact RLE worked!
    if (dwSize < pri->RleState.lMaxFrameSize) {

	if (fInterlaceNow)
	    pri->iStart = 0;	// we did 2nd half, so next time do full dib

	goto return_success;
    }

    if (pri->lLastParm)		// this value worked last time, so try it now!
				// unless of course, it's too big.
	lCurParm = min(pri->lLastParm, lTempMax);
    else if (lTempMax == MAXTOL) // no limit to what parameter can be
	lCurParm = FIRSTTRY;	// so make the 1st value reasonable
    else
	lCurParm = lTempMax;    // There is a limit on how big the parm can be.
				// Start as big as possible, so that if that
				// doesn't fit, we can give up right away

    goto skip_if;               // skip the big IF

noskip_if:

// This first condition tests to see if the current attempt yielded a frame
// that was still too big, and we have just tried the largest parameter
// possible.  It looks like we will never get the frame small enough!
// Our only hope is to interlace the frames, if we're allowed to.

	if (dwSize > pri->RleState.lMaxFrameSize && lCurParm > tolMax-1)
	{

	// It looks like either we're a keyframe and can't interlace, or
	// we've been trying interlacing and we're STILL not small enough.
	// There is nothing else we can do.  Give up.
	// NOTE: this shouldn't happen if the parameter is allowed to grow
	// arbitrarily!

	    if (fInterlaceNow || !lpbiFrom) {


		if (!lpbiFrom)
		    goto return_success;

		if (iStart > 0) {       // This was 2nd frame of a pair (top)
		    pri->iStart = 0;
		    lCurParm = 0L;      // don't remember this value because
					// this frame halving value won't help
					// us next frame when we aren't using
					// frame halving any more.
		} else {        // This was the first frame of a pair (bottom).
				// Remember to do the 2nd frame next time
		    pri->iStart += iLen;
		}
		goto return_success;

	// We are allowed to interlace, so we can prepare to.
	// Gee, I hope this isn't the last frame in the movie
	// (there will be no frame to do the 2nd half of!! )

	    } else {
                fInterlaceNow = TRUE;

                DPF(("FIRST HALF OF INTERLACE"));

		iStart   = 0;
                iLen     = (int)lpbiTo->biHeight/2;
		lCurParm = 0L;                  // start with no fuzziness
		lTempMin = 0L;
		lTempMax = MAXTOL;       // no limit to fuzziness
		tolMax   = MAXTOL;
	    }

// This condition tests to see if the size is still too big after this attempt,
// and the window of parameter values that we can try is still large enough
// to try some more values.  If so, we shrink the window a bit (the new lowest
// value worth trying is the current value, and we bump the current value up by
// half of the window size, but not TOO much.  You see, if our parameter is too
// high, then we binary search smaller values between 0 and this value.  But if
// the parameter is too small, how do we binary search through here and
// infinity? (actually 195,075)  So, we just increase the parameter by 2048.
// Next time we need to increase it, we will increase by 4096, 8192, etc.
// This way, we will quickly get to the limit of 195,075.  Perhaps the frame
// cannot possibly be crunched as small as it needs to be.  The program
// shouldn't take forever to realize this and get to 195,075.  But we shouldn't
// binary search between 0 and 195,075 because it will waste time getting down
// to the small values like 1000 that most movies will need.  This is the
// best compromise.  Hope that wasn't too long winded!  :-)

	} else if ((dwSize > pri->RleState.lMaxFrameSize) &&
	    ((lTempMax - lTempMin) > CWND))
	{
	    lTempMin = lCurParm;
	    if (lTempMax == MAXTOL){ // upper limit is still unbounded so
					    // leap way higher to our next try
		if (MAXTOL - lCurParm < lBumpUp)
		    lCurParm = MAXTOL;
		else
		    lCurParm += lBumpUp;
		lBumpUp *= 2;
	    } else
                lCurParm += (lTempMax - lCurParm) >> 1;

// For this condition, we are still too big, but the window is getting so small
// that we fear we will never find a value that works!  Let's say we know that
// 200 gives a frame that is too big, and 210 gives a frame that is too small.
// Should we bother searching any more?  NO!!!  That would waste time.  Let's
// just give up and take the 210 value (too small is better than too large)
// and continue.  The next time through this loop, it will give up when it sees
// that the window is too small and the current attempt produced a frame that
// was small enough, even though it was a little smaller than we wanted.

	} else if (dwSize > pri->RleState.lMaxFrameSize) {
	    lCurParm = lTempMax;

// This condtion says that the size is too small to accept, and the window
// of values to try is still large enough to warrant trying again.  So, we
// close the window a bit by setting the new highest value worth trying to
// the current value, and dropping the current value by half.

	} else if ((dwSize < pri->RleState.lMinFrameSize) && ((lTempMax - lTempMin) > CWND)) {
	    lTempMax = lCurParm;
            lCurParm -= (lCurParm - lTempMin) >> 1;

// Here is the catch all last else of the if.  If it gets here, then the frame
// is either just the perfect size and we can quit, or it's too small, but
// we've determined that we can't be bothered to search any more, so we're going
// to quit anyway.

	} else {
	    if (fInterlaceNow) {        // we were interlacing
		if (iStart > 0) {       // this was 2nd half of a pair (top)
		    pri->iStart = 0;
		    lCurParm = 0L;      // don't remember this value because
					// this frame halving value won't help
					// us next frame when we aren't using
					// frame halving any more.
		} else {                // This was 1st half of a pair (bottom)
		    pri->iStart = iLen; // next time, do 2nd half
		}
	    }
	    goto return_success;
	}

skip_if:

// We know that the previous attempt to RLE didn't work, so try again with
// the new values.

	Yield();

	// Set the TEMPORAL and SPATIAL values.
        // NOTE: if we are only working with a single DIB, (no lpbiFrom),
	// TEMPORAL compression won't work, so we enabled SPATIAL adaptive.
	// The TEMPORAL value will be ignored in that case.

	if (fSpatialAdaptive && fTemporalAdaptive) {
            tolSpatial = lCurParm>>3; // lCurParm/8;
	    tolTemporal = lCurParm;
	} else if (fTemporalAdaptive)
	    tolTemporal = lCurParm;
	else if (fSpatialAdaptive)
	    tolSpatial = lCurParm;

        if (!RleDeltaFrame(lpbiRle,lpRle,lpbiFrom,lpFrom,lpbiTo,lpTo,iStart,iLen,tolTemporal,tolSpatial,maxRun,minJump)) {
            DPF(("Crunch Error - Rle Delta Frame failed"));
	    goto return_failure;
	}

	// Remember the size of the last attempt, and take size of this attempt

	dwLastSize = dwSize;

        dwSize = lpbiRle->biSizeImage;

        DPF(("tolTemporal=%ld, tolSpatial=%ld, Size=%ld", tolTemporal, tolSpatial, dwSize));

    goto noskip_if;     // Go back and see how we did!

return_failure:
	pri->lLastParm = 0L;
        return FALSE;

return_success:
//	if (lCurParm)	// putting this line in won't let frame halving
			// threshold value get tried first.  But it will
			// avoid trashing old values that worked.  If you
			// understand this comment, you probably didn't need
			// to read it!!
	    pri->lLastParm = lCurParm;

        if (pri->lpbiPrev)
	{
            GlobalFreePtr(pri->lpbiPrev);
            pri->lpbiPrev = NULL;
	}

        if (lpbiRle)
	{
	    if (pri->iStart)
	    {
                lpbiRle->biCompression = BI_DIBX;     // 1st part of DIB. Not
                pri->lpbiPrev = CopyDib(lpbiTo, lpTo);// complete until next
            }                                         // BI_RLE8 is seen.
	    else
	    {
                lpbiRle->biCompression = BI_RLE8;
	    }
	}

        return TRUE;
}

BOOL FAR PASCAL SplitDib(PRLEINST pri,
    LPBITMAPINFOHEADER  lpbiRle, LPBYTE pbRle,
    LPBITMAPINFOHEADER  lpbiPrev,LPBYTE pbPrev,
    LPBITMAPINFOHEADER  lpbiDib, LPBYTE pbDib)
{
    int iStart, iLen, iMin, iMax;
    DWORD dwSize;
    BOOL f;

    iStart = iMin = 0;
    iLen   = iMax = (int)lpbiDib->biHeight - iStart;

    for(;;)
    {
        f = RleDeltaFrame(
                lpbiRle, pbRle,
                lpbiPrev,pbPrev,
                lpbiDib, pbDib,
                iStart,iLen,
		pri->RleState.tolTemporal,
		pri->RleState.tolSpatial,
		pri->RleState.iMaxRunLen,4);

        if (!f)
            return FALSE;

        dwSize = lpbiRle->biSizeImage;

        DPF(("iStart=%d, iLen=%d, Size=%ld, Max=%ld", iStart, iLen, dwSize, pri->RleState.lMaxFrameSize));

	if (dwSize < (DWORD)pri->RleState.lMaxFrameSize)
	{
            iMin = iLen;

	    if (iMax-iMin <= 1)
	    {
		pri->iStart += iLen;

		if (pri->iStart >= (int)lpbiDib->biHeight)
		    pri->iStart = 0;

                return TRUE;
	    }
	}
	else
	    iMax = iLen - 1;

	if (iStart != pri->iStart)
	{
	    iStart = pri->iStart;
	    iLen = iMax = (int)lpbiDib->biHeight - iStart;
	}
	else
	{
	    iLen = (iMin + iMax) / 2;
	}
    }
}
