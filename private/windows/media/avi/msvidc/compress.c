/*----------------------------------------------------------------------+
| compress.c - Microsoft Video 1 Compressor - compress code		|
|									|
|									|
| Copyright (c) 1990-1994 Microsoft Corporation.			|
| Portions Copyright Media Vision Inc.					|
| All Rights Reserved.							|
|									|
| You have a non-exclusive, worldwide, royalty-free, and perpetual	|
| license to use this source code in developing hardware, software	|
| (limited to drivers and other software required for hardware		|
| functionality), and firmware for video display and/or processing	|
| boards.   Microsoft makes no warranties, express or implied, with	|
| respect to the Video 1 codec, including without limitation warranties	|
| of merchantability or fitness for a particular purpose.  Microsoft	|
| shall not be liable for any damages whatsoever, including without	|
| limitation consequential damages arising from your use of the Video 1	|
| codec.								|
|									|
|									|
+----------------------------------------------------------------------*/
#include <windows.h>
#include <windowsx.h>
#include <mmsystem.h>       // for timeGetTime()
#include <win32.h>
#include "msvidc.h"

#include <memory.h>         // for _fmemcmp

//#include <limits.h>
//#include <aviffmt.h>

DWORD   numberOfBlocks;
DWORD   numberOfSolids;
DWORD   numberOfSolid4;
DWORD   numberOfSolid2;
DWORD   numberOfEdges;
DWORD   numberOfSkips;
DWORD   numberOfExtraSkips;
DWORD   numberOfSkipCodes;
DWORD   numberOfEvilRed;

/*******************************************************************
*******************************************************************/

#define SWAP(x,y) ( (x)^=(y), (y)^=(x), (x)^=(y) )

#define SWAPRGB(x,y)( SWAP((x).rgbRed,   (y).rgbRed),  \
                      SWAP((x).rgbGreen, (y).rgbGreen),\
                      SWAP((x).rgbBlue,  (y).rgbBlue) )

// Take an RGB quad value and figure out it's lumanence
// Y = 0.3*R + 0.59*G + 0.11*B
#define RgbToY(rgb) ((((WORD)((rgb).rgbRed)  * 30) + \
                      ((WORD)((rgb).rgbGreen)* 59) + \
                      ((WORD)((rgb).rgbBlue) * 11))/100)

#define RGB16(r,g,b)   ((((WORD)(r) >> 3) << 10) | \
                        (((WORD)(g) >> 3) << 5)  | \
                        (((WORD)(b) >> 3) << 0) )

#define RGBQ16(rgb)    RGB16((rgb).rgbRed,(rgb).rgbGreen,(rgb).rgbBlue)

// this array is used to associate each of the 16 luminance values
// with one of the sum values
BYTE meanIndex[16] = { 0, 0, 1, 1,
                       0, 0, 1, 1,
                       2, 2, 3, 3,
                       2, 2, 3, 3 };


/*****************************************************************************
 ****************************************************************************/



//
// map Quality into our threshold
//
//  Quality goes from ICQUALITY_LOW-ICQUALITY_HIGH (bad to good)
//
//  threshold = (Quality/ICQUALITY_HIGH)^THRESHOLD_POW * THRESHOLD_HIGH
//
DWORD FAR QualityToThreshold(DWORD dwQuality)
{
    #define THRESHOLD_HIGH ((256*256l)/2)
    #define THRESHOLD_POW  4
    double dw1;

    dw1 = (double)(dwQuality) / ICQUALITY_HIGH;

    // unbelievably enough, pow() doesn't work on alpha or mips!
    // also I can't believe this will be less efficient than pow(x, 4)
    dw1 = (dw1 * dw1 * dw1 * dw1);

    return (DWORD) (dw1 * THRESHOLD_HIGH);

    //return (DWORD)(pow((double)(dwQuality)/ICQUALITY_HIGH,THRESHOLD_POW) * THRESHOLD_HIGH);
}

/*******************************************************************
*******************************************************************/

//
// table to map a 5bit index (0-31) to a 8 bit value (0-255)
//
static BYTE aw5to8[32] = {(BYTE)-1};

//
// inverse table to map a RGB16 to a 8bit pixel
//
#define MAPRGB16(rgb16) lpITable[(rgb16)]
#define MAPRGB(rgb)     lpITable[RGBQ16(rgb)]


/*******************************************************************
*******************************************************************/

DWORD FAR CompressFrameBegin(LPBITMAPINFOHEADER lpbiIn, LPBITMAPINFOHEADER lpbiOut,
			     LPBYTE *lplpITable, RGBQUAD *prgbqOut)
{
    int i;

    //
    // init the 5bit to 8bit conversion table.
    //
    if (aw5to8[0] != 0)
        for (i=0; i<32; i++)
            aw5to8[i] = (BYTE)(i * 255 / 31);

    //
    // copy the color table to local storage
    //
    if (lpbiIn->biBitCount == 8)
    {
        if (lpbiIn->biClrUsed == 0)
            lpbiIn->biClrUsed = 256;
    }

    //
    //  if we are compressing to 8bit, then build a inverse table
    //
    if (lpbiOut->biBitCount == 8)
    {
        if (lpbiOut->biClrUsed == 0)
            lpbiOut->biClrUsed = 256;

        if (_fmemcmp((LPVOID)prgbqOut, (LPVOID)(lpbiOut+1),
            (int)lpbiOut->biClrUsed * sizeof(RGBQUAD)))
        {
            for (i=0; i<(int)lpbiOut->biClrUsed; i++)
                prgbqOut[i] = ((LPRGBQUAD)(lpbiOut+1))[i];

            if (*lplpITable)
                GlobalFreePtr(*lplpITable);

            *lplpITable = NULL;
        }

        if (*lplpITable == NULL)
        {
	    // !!! Need a critical section around this code!
            DPF(("Building ITable.... (%d colors)", (int)lpbiOut->biClrUsed));
            *lplpITable = MakeITable(prgbqOut, (int)lpbiOut->biClrUsed);
	    // !!! Critical section can end here....
        }

        if (*lplpITable == NULL)
            return (DWORD)ICERR_MEMORY;
    }

    return ICERR_OK;
}

/*******************************************************************
*******************************************************************/

DWORD FAR CompressFrameEnd(LPBYTE *lplpITable)
{
    if (*lplpITable)
        GlobalFreePtr(*lplpITable);

    *lplpITable = NULL;

    return ICERR_OK;
}

/*******************************************************************
*******************************************************************/

void FAR CompressFrameFree(void)
{
}

/*******************************************************************

GetCell - get a 4x4 cell from a image.

returns pointer to next cell.

*******************************************************************/

static LPVOID _FASTCALL
GetCell(LPBITMAPINFOHEADER lpbi, LPVOID lpBits, PCELL pCell)
{
    UINT WidthBytes;
    int  bits;
    int  i;
    int  x;
    int  y;
    BYTE b;
    HPBYTE pb;
    RGBQUAD FAR *prgbqIn;

    RGB555 rgb555;

    pb = lpBits;

    bits       = (int)lpbi->biBitCount;
    WidthBytes = DIBWIDTHBYTES(*lpbi);
    WidthBytes-= (WIDTH_CBLOCK * bits/8);

    ((HPBYTE)lpBits) += (WIDTH_CBLOCK * bits/8);       // "next" cell

    i = 0;

    switch (bits)
    {
        case 8:
	    prgbqIn = (RGBQUAD FAR *) (lpbi + 1);
            for( y = 0; y < HEIGHT_CBLOCK; y++ )
            {
                for( x = 0; x < WIDTH_CBLOCK; x++ )
                {
                    b = *pb++;
                    pCell[i++] = prgbqIn[b];
                }
                pb += WidthBytes; // next row in this block
            }
            break;

        case 16:
            for( y = 0; y < HEIGHT_CBLOCK; y++ )
            {
                for( x = 0; x < WIDTH_CBLOCK; x++ )
                {
                    rgb555 = *((HPRGB555)pb)++;
                    pCell[i].rgbRed   = aw5to8[(rgb555 >> 10) & 0x1F];
                    pCell[i].rgbGreen = aw5to8[(rgb555 >>  5) & 0x1F];
                    pCell[i].rgbBlue  = aw5to8[(rgb555 >>  0) & 0x1F];
                    i++;
                }
                pb += WidthBytes; // next row in this block
            }
            break;

        case 24:
            for( y = 0; y < HEIGHT_CBLOCK; y++ )
            {
                for( x = 0; x < WIDTH_CBLOCK; x++ )
                {
                    pCell[i].rgbBlue  = *pb++;
                    pCell[i].rgbGreen = *pb++;
                    pCell[i].rgbRed   = *pb++;
                    i++;
                }
                pb += WidthBytes; // next row in this block
            }
            break;

        case 32:
            for( y = 0; y < HEIGHT_CBLOCK; y++ )
            {
                for( x = 0; x < WIDTH_CBLOCK; x++ )
                {
                    pCell[i].rgbBlue  = *pb++;
                    pCell[i].rgbGreen = *pb++;
                    pCell[i].rgbRed   = *pb++;
                    pb++;
                    i++;
                }
                pb += WidthBytes; // next row in this block
            }
            break;
    }

    //
    //  return the pointer to the "next" cell
    //
    return lpBits;
}

/*******************************************************************

CmpCell - compares two 4x4 cells and returns an error value.

the error value is a sum of squares error.

the error value ranges from

    0           = exact
    3*256^2     = way off

*******************************************************************/

static DWORD _FASTCALL
CmpCell(PCELL cellA, PCELL cellB)
{
#if 0
    int   i;
    long  l;
    int   dr,dg,db;

    for (l=0,i=0; i < HEIGHT_CBLOCK*WIDTH_CBLOCK; i++)
    {
        dr = (int)cellA[i].rgbRed   - (int)cellB[i].rgbRed;
        dg = (int)cellA[i].rgbGreen - (int)cellB[i].rgbGreen;
        db = (int)cellA[i].rgbBlue  - (int)cellB[i].rgbBlue;

        l += ((long)dr * dr) + ((long)dg * dg) + ((long)db * db);
    }

    return l / (HEIGHT_CBLOCK*WIDTH_CBLOCK);
#else
    int   i;
    DWORD dw;

    //
    //
#define SUMSQ(a,b)                          \
    if (a > b)                              \
        dw += (UINT)(a-b) * (UINT)(a-b);    \
    else                                    \
        dw += (UINT)(b-a) * (UINT)(b-a);

    for (dw=0,i=0; i < HEIGHT_CBLOCK*WIDTH_CBLOCK; i++)
    {
        SUMSQ(cellA[i].rgbRed,   cellB[i].rgbRed);
        SUMSQ(cellA[i].rgbGreen, cellB[i].rgbGreen);
        SUMSQ(cellA[i].rgbBlue,  cellB[i].rgbBlue);
    }

    return dw / (HEIGHT_CBLOCK*WIDTH_CBLOCK);
#endif
}

#if 0  // This routine is unused
/*******************************************************************

MapCell - map a CELL full of 24bit values down to thier nearest
          colors in the 8bit palette

*******************************************************************/

static void _FASTCALL
MapCell(PCELL pCell)
{
    int i;
    int n;

    for (i=0; i < HEIGHT_CBLOCK*WIDTH_CBLOCK; i++)
    {
        n = MAPRGB(pCell[i]);       // map to nearest palette index
        pCell[i] = prgbqOut[n];      // ...and map back to a RGB
    }
}
#endif


//////////////////////////////////////////////////////////////////////////
//
//  take care of any outstanding skips
//
//////////////////////////////////////////////////////////////////////////

#define FlushSkips()                        \
                                            \
    while (SkipCount > 0)                   \
    {                                       \
        WORD w;                             \
                                            \
        w = min(SkipCount, SKIP_MAX);       \
        SkipCount -= w;                     \
        w |= SKIP_MAGIC;                    \
        *dst++ = w;                         \
        numberOfSkipCodes++;                \
        actualSize += 2;                    \
    }


/*******************************************************************
routine: CompressFrame16
purp:    compress a frame, outputing 16 bit compressed data.
returns: number of bytes in compressed buffer
*******************************************************************/

DWORD FAR CompressFrame16(LPBITMAPINFOHEADER  lpbi,           // DIB header to compress
                      LPVOID              lpBits,         // DIB bits to compress
                      LPVOID              lpData,         // put compressed data here
                      DWORD               threshold,      // edge threshold
                      DWORD               thresholdInter, // inter-frame threshold
                      LPBITMAPINFOHEADER  lpbiPrev,       // previous frame
                      LPVOID              lpPrev,         // previous frame
		      LONG (CALLBACK *Status) (LPARAM lParam, UINT message, LONG l),
		      LPARAM		  lParam,
                      PCELLS              pCells)
{
UINT            bix;
UINT            biy;
UINT            WidthBytes;
UINT            WidthBytesPrev;

WORD            SkipCount;

WORD            luminance[16], luminanceMean[4];
DWORD           luminanceSum;
WORD            sumR,sumG,sumB;
WORD            sumR0[4],sumG0[4],sumB0[4];
WORD            sumR1[4],sumG1[4],sumB1[4];
WORD            meanR0[4],meanG0[4],meanB0[4];
WORD            meanR1[4],meanG1[4],meanB1[4];
WORD            zeros[4], ones[4];
UINT            x,y;
WORD            mask;
HPBYTE          srcPtr;
HPBYTE          prvPtr;
DWORD           actualSize;
UINT            i;
UINT            mi;
HPWORD          dst;
RGBQUAD         rgb;
int		iStatusEvery;

#ifdef DEBUG
DWORD           time = timeGetTime();
#endif

    WidthBytes = DIBWIDTHBYTES(*lpbi);

    if (lpbiPrev)
        WidthBytesPrev = DIBWIDTHBYTES(*lpbiPrev);

    bix = (int)lpbi->biWidth/WIDTH_CBLOCK;
    biy = (int)lpbi->biHeight/HEIGHT_CBLOCK;

    if (bix < 100)
	iStatusEvery = 4;
    else if (bix < 200)
	iStatusEvery = 2;
    else
	iStatusEvery = 1;

    actualSize = 0;
    numberOfSkipCodes   = 0;
    numberOfSkips       = 0;
    numberOfExtraSkips  = 0;
    numberOfEdges       = 0;
    numberOfBlocks      = 0;
    numberOfSolids      = 0;
    numberOfSolid4      = 0;
    numberOfSolid2      = 0;
    numberOfEvilRed     = 0;

    dst = (HPWORD)lpData;
    SkipCount = 0;

    for( y = 0; y < biy; y++ )
    {
	if (Status && ((y % iStatusEvery) == 0)) {
	    if (Status(lParam, ICSTATUS_STATUS, (y * 100) / biy) != 0)
		return (DWORD) -1;
	}

        srcPtr = lpBits;
        prvPtr = lpPrev;

        for( x = 0; x < bix; x++ )
        {
//////////////////////////////////////////////////////////////////////////
//
// get the cell to compress from the image.
//
//////////////////////////////////////////////////////////////////////////
            srcPtr = GetCell(lpbi, srcPtr, pCells->cell);

//////////////////////////////////////////////////////////////////////////
//
//  see if it matches the cell in the previous frame.
//
//////////////////////////////////////////////////////////////////////////
            if (lpbiPrev)
            {
                prvPtr = GetCell(lpbiPrev, prvPtr, pCells->cellPrev);

                if (CmpCell(pCells->cell, pCells->cellPrev) <= thresholdInter)
                {
skip_cell:
                    numberOfSkips++;
                    SkipCount++;
                    continue;
                }
            }

//////////////////////////////////////////////////////////////////////////
// compute luminance of each pixel in the compression block
// sum the total luminance in the block
// find the pixels with the largest and smallest luminance
//////////////////////////////////////////////////////////////////////////

            luminanceSum = 0;
            sumR = 0;
            sumG = 0;
            sumB = 0;

            for (i = 0; i < HEIGHT_CBLOCK*WIDTH_CBLOCK; i++)
            {
                sumR += pCells->cell[i].rgbRed;
                sumG += pCells->cell[i].rgbGreen;
                sumB += pCells->cell[i].rgbBlue;

                luminance[i] = RgbToY(pCells->cell[i]);
                luminanceSum += luminance[i];
            }

//////////////////////////////////////////////////////////////////////////
//
// see if we make the cell a single color, and get away with it
//
//////////////////////////////////////////////////////////////////////////
            sumR /= HEIGHT_CBLOCK*WIDTH_CBLOCK;
            sumG /= HEIGHT_CBLOCK*WIDTH_CBLOCK;
            sumB /= HEIGHT_CBLOCK*WIDTH_CBLOCK;

            rgb.rgbRed   = (BYTE)sumR;
            rgb.rgbGreen = (BYTE)sumG;
            rgb.rgbBlue  = (BYTE)sumB;

            for (i=0; i < HEIGHT_CBLOCK*WIDTH_CBLOCK; i++)
                pCells->cellT[i] = rgb;

            if (CmpCell(pCells->cell, pCells->cellT) <= threshold)
            {
                if (lpbiPrev && CmpCell(pCells->cellT, pCells->cellPrev) <= thresholdInter)
                {
                    numberOfExtraSkips++;
                    goto skip_cell;
                }

                FlushSkips();

                // single color!!
solid_color:
                mask = RGB16(sumR, sumG, sumB) | 0x8000;

                if ((mask & ~SKIP_MASK) == SKIP_MAGIC)
                {
                    numberOfEvilRed++;
                    mask ^= SKIP_MAGIC;
                    mask |= 0x8000;
                }

                *dst++ = mask;
                numberOfSolids++;
                actualSize += 2;
                continue;
            }

//////////////////////////////////////////////////////////////////////////
//
//  make a 4x4 block
//
//////////////////////////////////////////////////////////////////////////

            luminanceMean[0] = (WORD)(luminanceSum >> 4);

            // zero summing arrays
            zeros[0]=0;
            ones[0] =0;
            sumR0[0]=0;
            sumR1[0]=0;
            sumG0[0]=0;
            sumG1[0]=0;
            sumB0[0]=0;
            sumB1[0]=0;

            // define which of the two colors to choose
            // for each pixel by creating the mask
            mask = 0;
            for( i=0; i < HEIGHT_CBLOCK*WIDTH_CBLOCK; i++ )
            {
                if( luminance[i] < luminanceMean[0] )
                {
                    // mask &= ~(1 << i);     // already clear
                    zeros[0]++;
                    sumR0[0] += pCells->cell[i].rgbRed;
                    sumG0[0] += pCells->cell[i].rgbGreen;
                    sumB0[0] += pCells->cell[i].rgbBlue;
                }
                else
                {
                    mask |= (1 << i);
                    ones[0]++;
                    sumR1[0] += pCells->cell[i].rgbRed;
                    sumG1[0] += pCells->cell[i].rgbGreen;
                    sumB1[0] += pCells->cell[i].rgbBlue;
                }
            }

            // define the "one" color as the mean of each element
            if( ones[0] != 0 )
            {
                meanR1[0] = sumR1[0] / ones[0];
                meanG1[0] = sumG1[0] / ones[0];
                meanB1[0] = sumB1[0] / ones[0];
            }
            else
            {
                meanR1[0] = meanG1[0] = meanB1[0] = 0;
            }

            if( zeros[0] != 0 )
            {
                meanR0[0] = sumR0[0] / zeros[0];
                meanG0[0] = sumG0[0] / zeros[0];
                meanB0[0] = sumB0[0] / zeros[0];
            }
            else
            {
                meanR0[0] = meanG0[0] = meanB0[0] = 0;
            }

            //
            // build the block and make sure, it is within error.
            //
            for( i=0; i < HEIGHT_CBLOCK*WIDTH_CBLOCK; i++ )
            {
                if( luminance[i] < luminanceMean[0] )
                {
                    pCells->cellT[i].rgbRed   = (BYTE)meanR0[0];
                    pCells->cellT[i].rgbGreen = (BYTE)meanG0[0];
                    pCells->cellT[i].rgbBlue  = (BYTE)meanB0[0];
                }
                else
                {
                    pCells->cellT[i].rgbRed   = (BYTE)meanR1[0];
                    pCells->cellT[i].rgbGreen = (BYTE)meanG1[0];
                    pCells->cellT[i].rgbBlue  = (BYTE)meanB1[0];
                }
            }

            if (CmpCell(pCells->cell, pCells->cellT) <= threshold)
            {
                if (lpbiPrev && CmpCell(pCells->cellT, pCells->cellPrev) <= thresholdInter)
                {
                    numberOfExtraSkips++;
                    goto skip_cell;
                }

                //
                // handle any outstanding skip codes
                //
                FlushSkips();

                //
                // we should never, ever generate a mask of all ones or
                // zeros!
                //
                if (mask == 0x0000)
                {
                    DPF(("4x4 generated a zero mask!"));
                    sumR = meanR0[0]; sumG = meanG0[0]; sumB = meanB0[0];
                    goto solid_color;
                }

                if (mask == 0xFFFF)
                {
                    DPF(("4x4 generated a FFFF mask!"));
                    sumR = meanR1[0]; sumG = meanG1[0]; sumB = meanB1[0];
                    goto solid_color;
                }


                //
                // remember the high bit of the mask is used to mark
                // a skip or solid color, so make sure the high bit
                // is zero.
                //
                if (mask & 0x8000)
                {
                    *dst++ = ~mask;
                    *dst++ = RGB16(meanR0[0],meanG0[0],meanB0[0]);
                    *dst++ = RGB16(meanR1[0],meanG1[0],meanB1[0]);
                }
                else
                {
                    *dst++ = mask;
                    *dst++ = RGB16(meanR1[0],meanG1[0],meanB1[0]);
                    *dst++ = RGB16(meanR0[0],meanG0[0],meanB0[0]);
                }
                actualSize += 6;
                numberOfBlocks++;

                continue;
            }

//////////////////////////////////////////////////////////////////////////
//
// see if we make the cell four solid colorls, and get away with it
//
//  C D E F
//  8 9 A C
//  4 5 6 7
//  0 1 2 3
//
//////////////////////////////////////////////////////////////////////////

#ifdef DEBUG
            for (i=0; i <= 10; i == 2 ? (i += 6) : (i += 2))
            {
                pCells->cellT[i].rgbRed   = (BYTE)(((WORD)pCells->cell[i].rgbRed
                                            + pCells->cell[i+1].rgbRed
                                            + pCells->cell[i+4].rgbRed
                                            + pCells->cell[i+5].rgbRed ) / 4);
                pCells->cellT[i].rgbGreen = (BYTE)(((WORD)pCells->cell[i].rgbGreen
                                            + pCells->cell[i+1].rgbGreen
                                            + pCells->cell[i+4].rgbGreen
                                            + pCells->cell[i+5].rgbGreen ) / 4);
                pCells->cellT[i].rgbBlue  = (BYTE)(((WORD)pCells->cell[i].rgbBlue
                                            + pCells->cell[i+1].rgbBlue
                                            + pCells->cell[i+4].rgbBlue
                                            + pCells->cell[i+5].rgbBlue ) / 4);
                pCells->cellT[i+1] = pCells->cellT[i+4]
                                   = pCells->cellT[i+5]
                                   = pCells->cellT[i];
            }

            if (CmpCell(pCells->cell, pCells->cellT) <= threshold)
            {
                // four colors
                numberOfSolid4++;
            }
#endif

//////////////////////////////////////////////////////////////////////////
//
//  make a 2x2 block
//
//////////////////////////////////////////////////////////////////////////

            FlushSkips();

            numberOfEdges++;

            luminanceMean[0] = (luminance[0]  + luminance[1]  + luminance[4]  + luminance[5])  / 4;
            luminanceMean[1] = (luminance[2]  + luminance[3]  + luminance[6]  + luminance[7])  / 4;
            luminanceMean[2] = (luminance[8]  + luminance[9]  + luminance[12] + luminance[13]) / 4;
            luminanceMean[3] = (luminance[10] + luminance[11] + luminance[14] + luminance[15]) / 4;

            // zero summing arrays
            zeros[0]=zeros[1]=zeros[2]=zeros[3]=0;
            ones[0]=ones[1]=ones[2]=ones[3]=0;
            sumR0[0]=sumR0[1]=sumR0[2]=sumR0[3]=0;
            sumR1[0]=sumR1[1]=sumR1[2]=sumR1[3]=0;
            sumG0[0]=sumG0[1]=sumG0[2]=sumG0[3]=0;
            sumG1[0]=sumG1[1]=sumG1[2]=sumG1[3]=0;
            sumB0[0]=sumB0[1]=sumB0[2]=sumB0[3]=0;
            sumB1[0]=sumB1[1]=sumB1[2]=sumB1[3]=0;

            // define which of the two colors to choose
            // for each pixel by creating the mask
            mask = 0;
            for( i=0; i < HEIGHT_CBLOCK*WIDTH_CBLOCK; i++ )
            {
                mi = meanIndex[i];
                if( luminance[i] < luminanceMean[mi] )
                {
                    // mask &= ~(1 << i);     // already clear
                    zeros[mi]++;
                    sumR0[mi] += pCells->cell[i].rgbRed;
                    sumG0[mi] += pCells->cell[i].rgbGreen;
                    sumB0[mi] += pCells->cell[i].rgbBlue;
                }
                else
                {
                    mask |= (1 << i);
                    ones[mi]++;
                    sumR1[mi] += pCells->cell[i].rgbRed;
                    sumG1[mi] += pCells->cell[i].rgbGreen;
                    sumB1[mi] += pCells->cell[i].rgbBlue;
                }
            }

            // store the mask

            if (mask & 0x8000)
                *dst++ = ~mask;
            else
                *dst++ = mask;

            actualSize += 2;

            // make the colors
            for( i=0; i < 4; i++ )
            {
                // define the "one" color as the mean of each element
                if( ones[i] != 0 )
                {
                    meanR1[i] = sumR1[i] / ones[i];
                    meanG1[i] = sumG1[i] / ones[i];
                    meanB1[i] = sumB1[i] / ones[i];
                }
                else
                {
                    meanR1[i] = meanG1[i] = meanB1[i] = 0;
                }

                if( zeros[i] != 0 )
                {
                    meanR0[i] = sumR0[i] / zeros[i];
                    meanG0[i] = sumG0[i] / zeros[i];
                    meanB0[i] = sumB0[i] / zeros[i];
                }
                else
                {
                    meanR0[i] = meanG0[i] = meanB0[i] = 0;
                }

                // convert to 555 and set bit 15 if this is an edge and
                // this is the first color

                if (mask & 0x8000)
                {
                    *dst++ = RGB16(meanR0[i],meanG0[i],meanB0[i]) | (i==0 ? 0x8000 : 0);
                    *dst++ = RGB16(meanR1[i],meanG1[i],meanB1[i]);
                }
                else
                {
                    *dst++ = RGB16(meanR1[i],meanG1[i],meanB1[i]) | (i==0 ? 0x8000 : 0);
                    *dst++ = RGB16(meanR0[i],meanG0[i],meanB0[i]);
                }
                actualSize += 4;
            }
        }

//////////////////////////////////////////////////////////////////////////
//
//  next scan.
//
//////////////////////////////////////////////////////////////////////////

        ((HPBYTE)lpBits) += WidthBytes * HEIGHT_CBLOCK;

        if (lpPrev)
            ((HPBYTE)lpPrev) += WidthBytesPrev * HEIGHT_CBLOCK;
    }

//////////////////////////////////////////////////////////////////////////
//
//  take care of any outstanding skips, !!! note we dont need this if we
//  assume a EOF!
//
//////////////////////////////////////////////////////////////////////////

    FlushSkips();

//////////////////////////////////////////////////////////////////////////
//
//  all done generate a EOF zero mask
//
//////////////////////////////////////////////////////////////////////////

    *dst++ = 0;
    actualSize += 2;

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

    DPF(("CompressFrame16:"));
    DPF(("           time: %ld", timeGetTime() - time));
    DPF(("            tol: %ld/%ld", threshold, thresholdInter));
    DPF(("           Size: %ld", actualSize));
    DPF(("          Skips: %ld (%ld)", numberOfSkips, numberOfSkipCodes));
    DPF(("    Extra Skips: %ld", numberOfExtraSkips));
    DPF(("          Solid: %ld", numberOfSolids));
    DPF(("            4x4: %ld", numberOfBlocks));
    DPF(("            2x2: %ld", numberOfEdges));
    DPF(("        EvilRed: %ld", numberOfEvilRed));
    DPF(("         4Solid: %ld", numberOfSolid4));

    return( actualSize );
}


/*******************************************************************
routine: CompressFrame8
purp:    compress a frame, outputing 8 bit compressed data.
returns: number of bytes in compressed buffer

!!! this is almost a 1:1 copy of the above routine help!
*******************************************************************/

DWORD FAR CompressFrame8(LPBITMAPINFOHEADER  lpbi,           // DIB header to compress
                     LPVOID              lpBits,         // DIB bits to compress
                     LPVOID              lpData,         // put compressed data here
                     DWORD               threshold,      // edge threshold
                     DWORD               thresholdInter, // inter-frame threshold
                     LPBITMAPINFOHEADER  lpbiPrev,       // previous frame
                     LPVOID              lpPrev,         // previous frame
		     LONG (CALLBACK *Status) (LPARAM lParam, UINT message, LONG l),
		     LPARAM		 lParam,
                     PCELLS              pCells,
		     LPBYTE		 lpITable,
		     RGBQUAD *		 prgbqOut)

{
UINT            bix;
UINT            biy;
UINT            WidthBytes;
UINT            WidthBytesPrev;

WORD            SkipCount;

WORD            luminance[16], luminanceMean[4];
DWORD           luminanceSum;
WORD            sumR,sumG,sumB;
WORD            sumR0[4],sumG0[4],sumB0[4];
WORD            sumR1[4],sumG1[4],sumB1[4];
WORD            meanR0[4],meanG0[4],meanB0[4];
WORD            meanR1[4],meanG1[4],meanB1[4];
WORD            zeros[4], ones[4];
UINT            x,y;
WORD            mask;
HPBYTE          srcPtr;
HPBYTE          prvPtr;
DWORD           actualSize;
UINT            i;
WORD            mi;
HPWORD          dst;
RGBQUAD         rgb,rgb0,rgb1;
BYTE            b, b0, b1;
WORD            w;
int		iStatusEvery;

#ifdef DEBUG
DWORD           time = timeGetTime();
#endif

    WidthBytes = DIBWIDTHBYTES(*lpbi);

    if (lpbiPrev)
        WidthBytesPrev = DIBWIDTHBYTES(*lpbiPrev);

    bix = (int)lpbi->biWidth/WIDTH_CBLOCK;
    biy = (int)lpbi->biHeight/HEIGHT_CBLOCK;

    if (bix < 100)
	iStatusEvery = 4;
    else if (bix < 200)
	iStatusEvery = 2;
    else
	iStatusEvery = 1;

    actualSize = 0;
    numberOfSkipCodes   = 0;
    numberOfSkips       = 0;
    numberOfExtraSkips  = 0;
    numberOfEdges       = 0;
    numberOfBlocks      = 0;
    numberOfSolids      = 0;
    numberOfSolid4      = 0;
    numberOfSolid2      = 0;
    numberOfEvilRed     = 0;

    dst = (HPWORD)lpData;
    SkipCount = 0;

    if (lpITable == NULL)
    {
        DPF(("ICM_COMPRESS_BEGIN not recieved!"));
        return 0;
    }

    for( y = 0; y < biy; y++ )
    {
        srcPtr = lpBits;
        prvPtr = lpPrev;

	if (Status && ((y % iStatusEvery) == 0)) {
	    if (Status(lParam, ICSTATUS_STATUS, (y * 100) / biy) != 0)
		return (DWORD) -1;
	}

        for( x = 0; x < bix; x++ )
        {
//////////////////////////////////////////////////////////////////////////
//
// get the cell to compress from the image.
//
//////////////////////////////////////////////////////////////////////////
            srcPtr = GetCell(lpbi, srcPtr, pCells->cell);

//////////////////////////////////////////////////////////////////////////
//
//  see if it matches the cell in the previous frame.
//
//////////////////////////////////////////////////////////////////////////
            if (lpbiPrev)
            {
                prvPtr = GetCell(lpbiPrev, prvPtr, pCells->cellPrev);

                if (CmpCell(pCells->cell, pCells->cellPrev) <= thresholdInter)
                {
skip_cell:
                    numberOfSkips++;
                    SkipCount++;
                    continue;
                }
            }

//////////////////////////////////////////////////////////////////////////
// compute luminance of each pixel in the compression block
// sum the total luminance in the block
// find the pixels with the largest and smallest luminance
//////////////////////////////////////////////////////////////////////////

            luminanceSum = 0;
            sumR = 0;
            sumG = 0;
            sumB = 0;

            for (i = 0; i < HEIGHT_CBLOCK*WIDTH_CBLOCK; i++)
            {
                sumR += pCells->cell[i].rgbRed;
                sumG += pCells->cell[i].rgbGreen;
                sumB += pCells->cell[i].rgbBlue;

                luminance[i] = RgbToY(pCells->cell[i]);
                luminanceSum += luminance[i];
            }

//////////////////////////////////////////////////////////////////////////
//
// see if we make the cell a single color, and get away with it
//
//////////////////////////////////////////////////////////////////////////
            sumR /= HEIGHT_CBLOCK*WIDTH_CBLOCK;
            sumG /= HEIGHT_CBLOCK*WIDTH_CBLOCK;
            sumB /= HEIGHT_CBLOCK*WIDTH_CBLOCK;

            rgb.rgbRed   = (BYTE)sumR;
            rgb.rgbGreen = (BYTE)sumG;
            rgb.rgbBlue  = (BYTE)sumB;

            b = MAPRGB(rgb);            // map color to 8bit
            rgb = prgbqOut[b];

            for (i=0; i < HEIGHT_CBLOCK*WIDTH_CBLOCK; i++)
                pCells->cellT[i] = rgb;

            if (CmpCell(pCells->cell, pCells->cellT) <= threshold)
            {
                if (lpbiPrev && CmpCell(pCells->cellT, pCells->cellPrev) <= thresholdInter)
                {
                    numberOfExtraSkips++;
                    goto skip_cell;
                }

                FlushSkips();

solid_color:
                // single color!!
                mask = SOLID_MAGIC | b;

                *dst++ = mask;
                numberOfSolids++;
                actualSize += 2;
                continue;
            }

//////////////////////////////////////////////////////////////////////////
//
//  make a 4x4 block
//
//////////////////////////////////////////////////////////////////////////

            luminanceMean[0] = (WORD)(luminanceSum >> 4);

            // zero summing arrays
            zeros[0]=0;
            ones[0] =0;
            sumR0[0]=0;
            sumR1[0]=0;
            sumG0[0]=0;
            sumG1[0]=0;
            sumB0[0]=0;
            sumB1[0]=0;

            // define which of the two colors to choose
            // for each pixel by creating the mask
            mask = 0;
            for( i=0; i < HEIGHT_CBLOCK*WIDTH_CBLOCK; i++ )
            {
                if( luminance[i] < luminanceMean[0] )
                {
                    // mask &= ~(1 << i);     // already clear
                    zeros[0]++;
                    sumR0[0] += pCells->cell[i].rgbRed;
                    sumG0[0] += pCells->cell[i].rgbGreen;
                    sumB0[0] += pCells->cell[i].rgbBlue;
                }
                else
                {
                    mask |= (1 << i);
                    ones[0]++;
                    sumR1[0] += pCells->cell[i].rgbRed;
                    sumG1[0] += pCells->cell[i].rgbGreen;
                    sumB1[0] += pCells->cell[i].rgbBlue;
                }
            }

            // define the "one" color as the mean of each element
            if( ones[0] != 0 )
            {
                meanR1[0] = sumR1[0] / ones[0];
                meanG1[0] = sumG1[0] / ones[0];
                meanB1[0] = sumB1[0] / ones[0];
            }
            else
            {
                meanR1[0] = meanG1[0] = meanB1[0] = 0;
            }

            if( zeros[0] != 0 )
            {
                meanR0[0] = sumR0[0] / zeros[0];
                meanG0[0] = sumG0[0] / zeros[0];
                meanB0[0] = sumB0[0] / zeros[0];
            }
            else
            {
                meanR0[0] = meanG0[0] = meanB0[0] = 0;
            }

            //
            // map colors to 8-bit
            //
            rgb0.rgbRed   = (BYTE)meanR0[0];
            rgb0.rgbGreen = (BYTE)meanG0[0];
            rgb0.rgbBlue  = (BYTE)meanB0[0];
            b0 = MAPRGB(rgb0);
            rgb0 = prgbqOut[b0];

            rgb1.rgbRed   = (BYTE)meanR1[0];
            rgb1.rgbGreen = (BYTE)meanG1[0];
            rgb1.rgbBlue  = (BYTE)meanB1[0];
            b1 = MAPRGB(rgb1);
            rgb1 = prgbqOut[b1];

            //
            // build the block and make sure, it is within error.
            //
            for( i=0; i < HEIGHT_CBLOCK*WIDTH_CBLOCK; i++ )
            {
                if( luminance[i] < luminanceMean[0] )
                    pCells->cellT[i] = rgb0;
                else
                    pCells->cellT[i] = rgb1;
            }

            if (CmpCell(pCells->cell, pCells->cellT) <= threshold)
            {
                if (lpbiPrev && CmpCell(pCells->cellT, pCells->cellPrev) <= thresholdInter)
                {
                    numberOfExtraSkips++;
                    goto skip_cell;
                }

                FlushSkips();

                // store the mask

                //
                // we should never, ever generate a mask of all ones or
                // zeros!
                //
                if (mask == 0x0000)
                {
                    DPF(("4x4 generated a zero mask!"));
                    b = b0;
                    goto solid_color;
                }

                if (mask == 0xFFFF)
                {
                    DPF(("4x4 generated a FFFF mask!"));
                    b = b1;
                    goto solid_color;
                }

                if (b0 == b1)
                {
                    DPF(("4x4 generated two colors the same!"));
                    b = b1;
                    goto solid_color;
                }

                //
                // remember the high bit of the mask is used to mark
                // a skip or solid color, so make sure the high bit
                // is zero.
                //
                if (mask & 0x8000)
                {
                    mask = ~mask;
                    SWAP(b0,b1);
                }

                *dst++ = mask;
                *dst++ = (WORD)b1 | ((WORD)b0 << 8);

                actualSize += 4;
                numberOfBlocks++;

                continue;
            }

//////////////////////////////////////////////////////////////////////////
//
//  make a 2x2 block
//
//////////////////////////////////////////////////////////////////////////

            FlushSkips();

            numberOfEdges++;

            luminanceMean[0] = (luminance[0]  + luminance[1]  + luminance[4]  + luminance[5])  / 4;
            luminanceMean[1] = (luminance[2]  + luminance[3]  + luminance[6]  + luminance[7])  / 4;
            luminanceMean[2] = (luminance[8]  + luminance[9]  + luminance[12] + luminance[13]) / 4;
            luminanceMean[3] = (luminance[10] + luminance[11] + luminance[14] + luminance[15]) / 4;

            // zero summing arrays
            zeros[0]=zeros[1]=zeros[2]=zeros[3]=0;
            ones[0]=ones[1]=ones[2]=ones[3]=0;
            sumR0[0]=sumR0[1]=sumR0[2]=sumR0[3]=0;
            sumR1[0]=sumR1[1]=sumR1[2]=sumR1[3]=0;
            sumG0[0]=sumG0[1]=sumG0[2]=sumG0[3]=0;
            sumG1[0]=sumG1[1]=sumG1[2]=sumG1[3]=0;
            sumB0[0]=sumB0[1]=sumB0[2]=sumB0[3]=0;
            sumB1[0]=sumB1[1]=sumB1[2]=sumB1[3]=0;

            // define which of the two colors to choose
            // for each pixel by creating the mask
            mask = 0;
            for( i=0; i < HEIGHT_CBLOCK*WIDTH_CBLOCK; i++ )
            {
                mi = meanIndex[i];
                if( luminance[i] < luminanceMean[mi] )
                {
                    // mask &= ~(1 << i);     // already clear
                    zeros[mi]++;
                    sumR0[mi] += pCells->cell[i].rgbRed;
                    sumG0[mi] += pCells->cell[i].rgbGreen;
                    sumB0[mi] += pCells->cell[i].rgbBlue;
                }
                else
                {
                    mask |= (1 << i);
                    ones[mi]++;
                    sumR1[mi] += pCells->cell[i].rgbRed;
                    sumG1[mi] += pCells->cell[i].rgbGreen;
                    sumB1[mi] += pCells->cell[i].rgbBlue;
                }
            }

            // store the mask
            //
            //  in the 8-bit case the mask must have the following form:
            //
            //      1X1XXXXXXXXXXXXX
            //
            //  these bits can be forces high by exchanging the colors for
            //  the top two cells.
            //

            w = mask;

            if (!(mask & 0x8000))
                w ^= 0xCC00;

            if (!(mask & 0x2000))
                w ^= 0x3300;

            *dst++ = w;
            actualSize += 2;

            // make the colors
            for( i=0; i < 4; i++ )
            {
                // define the "one" color as the mean of each element
                if( ones[i] != 0 )
                {
                    meanR1[i] = sumR1[i] / ones[i];
                    meanG1[i] = sumG1[i] / ones[i];
                    meanB1[i] = sumB1[i] / ones[i];
                }
                else
                {
                    meanR1[i] = meanG1[i] = meanB1[i] = 0;
                }

                if( zeros[i] != 0 )
                {
                    meanR0[i] = sumR0[i] / zeros[i];
                    meanG0[i] = sumG0[i] / zeros[i];
                    meanB0[i] = sumB0[i] / zeros[i];
                }
                else
                {
                    meanR0[i] = meanG0[i] = meanB0[i] = 0;
                }

                // convert to the 8bit palette, and write out the colors
                // make sure to exchange the colors if we hade to invert
                // the mask to normalize it.

                rgb0.rgbRed   = (BYTE)meanR0[i];
                rgb0.rgbGreen = (BYTE)meanG0[i];
                rgb0.rgbBlue  = (BYTE)meanB0[i];
                b0 = MAPRGB(rgb0);

                rgb1.rgbRed   = (BYTE)meanR1[i];
                rgb1.rgbGreen = (BYTE)meanG1[i];
                rgb1.rgbBlue  = (BYTE)meanB1[i];
                b1 = MAPRGB(rgb1);

                if (i==3 && !(mask & 0x8000))
                    SWAP(b0,b1);

                if (i==2 && !(mask & 0x2000))
                    SWAP(b0,b1);

                if (b0 == b0)
                {
                    numberOfSolid2++;
                }

                *dst++ = (WORD)b1 | ((WORD)b0 << 8);
                actualSize += 2;
            }
        }

//////////////////////////////////////////////////////////////////////////
//
//  next scan.
//
//////////////////////////////////////////////////////////////////////////

        ((HPBYTE)lpBits) += WidthBytes * HEIGHT_CBLOCK;

        if (lpPrev)
            ((HPBYTE)lpPrev) += WidthBytesPrev * HEIGHT_CBLOCK;
    }

//////////////////////////////////////////////////////////////////////////
//
//  take care of any outstanding skips, !!! note we dont need this if we
//  assume a EOF!
//
//////////////////////////////////////////////////////////////////////////

    FlushSkips();

//////////////////////////////////////////////////////////////////////////
//
//  all done generate a EOF zero mask
//
//////////////////////////////////////////////////////////////////////////

    *dst++ = 0;
    actualSize += 2;

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

    DPF(("CompressFrame8:"));
    DPF(("          time: %ld", timeGetTime() - time));
    DPF(("           tol: %ld/%ld", threshold, thresholdInter));
    DPF(("          Size: %ld", actualSize));
    DPF(("         Skips: %ld (%ld)", numberOfSkips, numberOfSkipCodes));
    DPF(("   Extra Skips: %ld", numberOfExtraSkips));
    DPF(("         Solid: %ld", numberOfSolids));
    DPF(("           4x4: %ld", numberOfBlocks));
    DPF(("           2x2: %ld", numberOfEdges));

    return( actualSize );
}


