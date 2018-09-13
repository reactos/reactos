/*--------------------------------------------------------------------------*\
|   RLE.C - RLE Delta frame code                                             |
|@@BEGIN_MSINTERNAL									      |
|                                                                            |
|   History:                                                                 |
|   01/01/88 toddla     Created                                              |
|   10/30/90 davidmay   Reorganized, rewritten somewhat.                     |
|   07/11/91 dannymi    Un-hacked                                            |
|   09/15/91 ToddLa     Re-hacked                                            |
|@@END_MSINTERNAL									      |
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
#include <memory.h>     // for _fmemcmp()
#include "msrle.h"

#define RLE_ESCAPE  0
#define RLE_EOL     0
#define RLE_EOF     1
#define RLE_JMP     2
#define RLE_RUN     3

typedef BYTE huge * HPRLE;
typedef BYTE far  * LPRLE;

RGBTOL    gRgbTol = {0, 0};


//
//  RleDeltaFrame
//
//  Calculate the RLE bits to go from hdib1 to hdib2
//
//      hdibPrev    - Previous DIB
//      hdib        - DIB to RLE
//
//  returns
//
//      handle to a RLE DIB
//
BOOL FAR PASCAL RleDeltaFrame(
    LPBITMAPINFOHEADER  lpbiRle,    LPBYTE pbRle,
    LPBITMAPINFOHEADER  lpbiPrev,   LPBYTE pbPrev,
    LPBITMAPINFOHEADER  lpbiDib,    LPBYTE pbDib,
    int         iStart,
    int         iLen,
    long        tolTemporal,
    long        tolSpatial,
    int         maxRun,
    int         minJump)
{
    LPBITMAPINFOHEADER  lpbi;

    int    biHeight;
    UINT   cbJump=0;
    int    dy;

    if (!lpbiDib)
        return FALSE;

    if (maxRun == 0)
        maxRun = -1;

    if (minJump == 0)
        minJump = 4;

    //
    //  Get info on the source and dest dibs
    //
    lpbi = lpbiDib;
    biHeight = (int)lpbi->biHeight;

    if (iLen <= 0)
        iLen = biHeight;

    iLen = min(biHeight-iStart, iLen);

    //
    //  Hey! we only work with 8bpp DIBs if we get otherwise barf.
    //
    if (lpbi->biBitCount != 8 || lpbi->biCompression != BI_RGB)
        return FALSE;

#if 0 // CompressBegin does this..
    //
    // Set up the table for quick sum of squares calculation (see rle.h)
    //
    if (!MakeRgbTable(lpbi))
        return FALSE;
#endif

    //
    //  lock all the buffers, and start the delta framin'
    //
    lpbi  = lpbiRle;

    if (iStart > 0)
        pbDib = DibXYN(lpbiDib, pbDib,0,iStart,8);

    if (iStart > 0 && lpbiPrev)
        pbPrev = DibXYN(lpbiPrev,pbPrev,0,iStart,8);

    if (lpbiPrev == NULL)
        pbPrev = NULL;

    while(iStart > 0)
    {
	dy = min(iStart,255);
	*pbRle++ = RLE_ESCAPE;
	*pbRle++ = RLE_JMP;
	*pbRle++ = 0;
	*pbRle++ = (BYTE)dy;
        iStart  -= dy;
        cbJump  += 4;
    }

    lpbi->biHeight = iLen;


#ifdef _WIN32
    DeltaFrameC(
#else
    DeltaFrame386(
#endif
	lpbi, pbPrev, pbDib, pbRle, maxRun, minJump,
	gRgbTol.hpTable, tolTemporal, tolSpatial);


    lpbi->biHeight = biHeight;
    lpbi->biSizeImage += cbJump;  // adjust size to include JUMP!

    return TRUE;
}

/* Next is a table that, for each pair of palette entries, helps determine
   if two colours are close enough to be merged to a single colour

   Let's say the first pixel of a frame is black, and the same pixel in the
   next frame is gray.  Should you bother painting that gray pixel or let it
   stay black because it's close enough?  With this table, you have 2 palettes
   (one for each of the two frames you are comparing, or possibly two identical
   palettes if you are filtering a single DIB) and a table associated with
   those palettes.  You can index into the table with the colour number of the
   pixel in the first frame and the colour number of the pixel in the second
   frame.  The table value will be a number representing how different those
   two colours are.

   |Red1 - Red2|^2 + |Green1 - Green2|^2 + |Blue1 - Blue2|^2

   is that value (sum of squares of differences).  As soon as you start
   using this table with a pair of palettes, those hpals are put in this
   structure so that you know what pair of palettes the table is built with.
   If you change a palette, you need to recompute the table.  BUT:  you don't
   build the table at the beginning, you do it on demand.  Initially, the
   table is filled with a value of UNCOMPUTED, and as the values are needed,
   they are put into the table, so a second call to the CloseEnough routine
   with the same colours will exit extremely quickly with no calculations!

   Prepare the table for looking up quickly the sum of squares of colours
   of two palette entries (possibly in different palettes)              */


DWORD NEAR _fastcall RgbCompare(RGBQUAD rgb1, RGBQUAD rgb2)
{
    DWORD sum=0;

    //
    //  lets do some majic shit so the compiler generates "good" code.
    //
#define SUMSQ(a,b)                          \
    if (a > b)                              \
        sum += (WORD)(a-b) * (WORD)(a-b);   \
    else                                    \
        sum += (WORD)(b-a) * (WORD)(b-a);

    SUMSQ(rgb1.rgbRed,   rgb2.rgbRed);
    SUMSQ(rgb1.rgbGreen, rgb2.rgbGreen);
    SUMSQ(rgb1.rgbBlue,  rgb2.rgbBlue);

    return sum;
}

BOOL NEAR PASCAL MakeRgbTable(LPBITMAPINFOHEADER lpbi)
{
    UINT i, j;
    int  n=0;
    DWORD tol;

    if (!lpbi)
        return FALSE;

    if (lpbi->biClrUsed == 0)
        lpbi->biClrUsed = 1 << lpbi->biBitCount;

    /* If the palette passed in has a different number of colours than */
    /* the one in the table, we obviously need a new table */

    if (gRgbTol.hpTable == NULL ||
        (int)lpbi->biClrUsed != gRgbTol.ClrUsed ||
        _fmemcmp(lpbi+1, gRgbTol.argbq, gRgbTol.ClrUsed * sizeof(RGBQUAD)))
    {
        if (gRgbTol.hpTable == NULL)
        {
            gRgbTol.hpTable = (LPVOID)GlobalAllocPtr(GHND|GMEM_SHARE, 256L * 256L * sizeof(DWORD));

            if (gRgbTol.hpTable == NULL)
                return FALSE;
        }

        gRgbTol.ClrUsed = (int)lpbi->biClrUsed;          // get the actual colours

        for (i = 0; i < (UINT)gRgbTol.ClrUsed; i++)
            gRgbTol.argbq[i] = ((LPRGBQUAD)(lpbi + 1))[i];

        for (i = 0; i < (UINT)gRgbTol.ClrUsed; i++)
        {
            for (j = 0; j <= i; j++)
            {
                tol = RgbCompare(gRgbTol.argbq[i], gRgbTol.argbq[j]);

                gRgbTol.hpTable[256 * i + j] = tol;
                gRgbTol.hpTable[256 * j + i] = tol;
            }
        }
    }

    return TRUE;
}

#ifdef _WIN32

// ---- DeltaFrameC --------------------------------------------------------

#define TolLookUp(p, a, b)	( ((LPDWORD)p)[a * 256 + b] )

LPBYTE EncodeFragment(LPBYTE pIn, int len, LPBYTE pOut, LPDWORD pTol, DWORD tolerance, UINT maxrun);
LPBYTE EncodeAbsolute(LPBYTE pbDib, int len, LPBYTE pbRle);
int FindFragmentLength(LPBYTE pIn, LPBYTE pPrev, int len, UINT maxjmp, LPDWORD pTol, DWORD tol, PDWORD prunlen);

// rle format:
// byte 1: 0 - escape
//		byte 2: 0 - eol
//		byte 2: 1 - eof
//		byte 2: 2 - jump x, y (bytes 3, 4)
//		byte 2: >2 - absolute run of pixels - byte 2 is length
// byte 1: >0 - repeat solid colour - byte 1 is length
//		byte 2 is solid pixel to repeat
	
	


// compression - in df.asm for Win16
extern void DeltaFrameC(
    LPBITMAPINFOHEADER  lpbi,
    LPBYTE              pbPrev,
    LPBYTE              pbDib,
    LPBYTE		pbRle,
    UINT		MaxRunLength,
    UINT		MinJumpLength,
    LPDWORD             TolTable,
    DWORD               tolTemporal,
    DWORD               tolSpatial)
{
    int WidthBytes = (lpbi->biWidth+3) & (~3);
    int x, y;
    LPBYTE pbRle_Orig = pbRle;

    if ((MaxRunLength == 0) || (MaxRunLength > 255)) {
	MaxRunLength = 255;
    }

    if (pbPrev == NULL) {

	// no previous frame, just encode each line spatially

	for (y = lpbi->biHeight; y > 0; y--) {

	    pbRle = EncodeFragment(
			pbDib,
			lpbi->biWidth,
			pbRle,
			TolTable,
			tolSpatial,
			MaxRunLength);

	    // don't bother to insert an EOL if we are about to insert EOF
	    if (y > 0) {
		* (WORD FAR *)pbRle = RLE_ESCAPE | (RLE_EOL << 8);
		pbRle += sizeof(WORD);
	    }

    	    pbDib += WidthBytes;
	}
    } else {
	int jumpX = 0;
	int jumpY = 0;
	int frag, runlen;

	
	for (y = 0; y < lpbi->biHeight; y++) {

	    x = 0;		

	    while (x < lpbi->biWidth) {

		// see how much is not the same as the previous frame,
		// followed by how much is the same. frag is the length of
		// the not-similar fragment; runlen is the length of the
		// similar fragment.

    		frag = FindFragmentLength(
			    pbDib,
			    pbPrev,
			    lpbi->biWidth - x,
			    MinJumpLength,
			    TolTable,
			    tolTemporal,
			    &runlen
		);

		if (frag == 0) {

		    // no fragment, just a jump over the similar pixels.
		    //add up jumps until we need to output them
		    jumpX += runlen;
		    x += runlen;
		    pbPrev += runlen;
		    pbDib += runlen;
		} else {

		    // output any saved jumps
		    if (jumpX < 0) {

			// don't jump backwards - eol and jump forwards
			*(WORD FAR *)pbRle = RLE_ESCAPE | (RLE_EOL << 8);
			pbRle += sizeof(WORD);

			// jump is now across to current position,
			// and one fewer lines.
			jumpX = x;
			jumpY--;
		    }

		    while (jumpX + jumpY) {
			int delta;


			* (WORD FAR *)pbRle = RLE_ESCAPE | (RLE_JMP << 8);
			pbRle += sizeof(WORD);

			// max jump size is 255

			delta = min(255, jumpX);
			*pbRle++ = (BYTE) delta;
			jumpX -= delta;

			delta = min(255, jumpY);
			*pbRle++ = (BYTE) delta;
			jumpY -= delta;

		    }

		    // output the different fragment as a combination
		    // of solid runs and absolute pixels
		    pbRle = EncodeFragment(
				pbDib,
				frag,
				pbRle,
				TolTable,
				tolSpatial,
				MaxRunLength);
		    x += frag;
		    pbDib += frag;
		    pbPrev += frag;
		}
	    }

	    // end-of-line
	    jumpY++;

	    // advance past DWORD-rounding bytes
	    pbPrev += (WidthBytes - lpbi->biWidth);
	    pbDib += (WidthBytes - lpbi->biWidth);

	    //adjust jumpX
	    jumpX -= x;

	}
    }

    // end-of-frame
    * (WORD FAR *)pbRle = RLE_ESCAPE | (RLE_EOF << 8);
    pbRle += sizeof(WORD);

    // update lpbi to correct size and format
    lpbi->biSizeImage = (DWORD) (pbRle - pbRle_Orig);
    lpbi->biCompression = BI_RLE8;
}


//
// encode a sequence of pixels as a mixture of solid runs and absolute
// pixels. write the rle data to pbRle and return pointer to the next
// available rle buffer.
LPBYTE
EncodeFragment(
    LPBYTE pbDib,
    int width,
    LPBYTE pbRle,
    LPDWORD TolTable,
    DWORD tolerance,
    UINT MaxRunLength
)
{
    int maxrun, run;
    BYTE px;

    while (width > 0) {

	maxrun = min(255, width);
	MaxRunLength = min((int)MaxRunLength, maxrun);

	px = *pbDib;

	for (run = 0; run < maxrun; run++, pbDib++) {

	    // the same or similar ? - use tolerance table to compare pixel
	    // rgb values
	    // We're allowed a run of 255 if they're exact, but only a run of
	    // MaxRunLength if they're not exact, only close
	    if (px == *pbDib)
		continue;
	    if (TolLookUp(TolTable,px,*pbDib) <= tolerance &&
							run < (int)MaxRunLength)
		continue;

	    // not close enough - end run
	    break;
	}

	// we have found the end of a run of identical pixels
	
	// if the run is one pixel, then we switch into absolute mode.
	// however, we cannot encode absolute runs of less than RLE_RUN
	// pixels (the runlength code is an escape code and must not coincide
	// with RLE_JMP, RLE_EOL and RLE_EOF.

	if ((run > 1) || (width < RLE_RUN)) {

    	    // write out run length and colour
	    * (WORD FAR *)pbRle = run | (px << 8);
	    pbRle += sizeof(WORD);

	    width -= run;

	} else {

	    // we have a 'run' of one pixel - back up to point at this.
	    pbDib--;

	    // write out an absolute run. now we are in abs mode, we need
	    // a solid run of at least 4 pixels for it to be worth leaving
	    // and re-entering abs mode

	    for (run = 0; run < maxrun; run++) {

		// at the end of the fragment ?
		if ((maxrun - run) < 4) {
		    // yes - so no point in looking for a solid run -
		    // just dump all the remainder as an absolute block
		    pbRle = EncodeAbsolute(pbDib, maxrun, pbRle);
		    pbDib += maxrun;
		    width -= maxrun;
		    break;
		}

		px = pbDib[run];
		if ( (TolLookUp(TolTable,px,pbDib[run + 1]) <= tolerance) &&
		     (TolLookUp(TolTable,px,pbDib[run + 2]) <= tolerance) &&
		     (TolLookUp(TolTable,px,pbDib[run + 3]) <= tolerance)) {

			 // we have run bytes to encode followed by four
			 // similar pixels

			 pbRle = EncodeAbsolute(pbDib, run, pbRle);
			 pbDib += run;
			 width -= run;
			 break;
		}
	    }
	}
    }

    return pbRle;
}

LPBYTE
EncodeAbsolute(LPBYTE pbDib, int runlen, LPBYTE pbRle)
{
    if (runlen < RLE_RUN) {

	// cannot encode absolute runs of less than RLE_RUN as it
	// conflicts with other rle escapes - so encode each pixel
	// as a run of 1 of that pixel
	int i;
	for (i = 0; i < runlen; i++) {

	    * (WORD FAR *) pbRle = 1 | ((*pbDib++) << 8);
	    pbRle += sizeof(WORD);
	}
	return pbRle;

    }

    // absolute run of > RLE_RUN

    * (WORD FAR *)pbRle = RLE_ESCAPE | (runlen << 8);
    pbRle += sizeof(WORD);

    while (runlen >= 2) {
	* (WORD FAR *) pbRle = * (WORD UNALIGNED FAR *)pbDib;
	pbRle += sizeof(WORD);
	pbDib += sizeof(WORD);
	runlen -= 2;
    }

    // remember to keep word alignment
    if (runlen) {
	*pbRle++ = *pbDib++;
	*pbRle++ = 0;
    }

    return pbRle;
}

// count how many pixels are not the same as the previous frame, and how
// long is the run of similar pixels after it. We must find at least minjump
// similar pixels before we stop.
int
FindFragmentLength(
    LPBYTE pIn,
    LPBYTE pPrev,
    int len,
    UINT minjump,
    LPDWORD pTol,
    DWORD tol,
    PDWORD prunlen
)
{
    int x;
    int run = 0;

    for (x = 0; x < len; x++) {


	if ((*pIn == *pPrev) || (TolLookUp(pTol, *pIn, *pPrev) <= tol)) {
	    run++;
	} else {

	    // have we accumulated a run long enough to be worth
	    // returning ?

	    if (run >= (int)minjump) {

		*prunlen = run;
		return x - run;
	    } else {
		run = 0;
	    }
	}
	pIn++;
	pPrev++;
    }

    // end of line - did we find a run ?
    if (run < (int) minjump) {
	*prunlen = 0;
	return len;
    } else {
	*prunlen = run;
	return x - run;
    }
}



#endif
