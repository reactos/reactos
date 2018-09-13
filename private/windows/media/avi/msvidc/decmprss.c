/*----------------------------------------------------------------------+
| decmprss.c - Microsoft Video 1 Compressor - decompress code		|
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
#ifdef _WIN32
//#ifdef DEBUG   DEBUG is not defined on NT until win32.h is included...
//               Always define here so that the ntrtl headers get included
#ifndef CHICAGO
#if DBG
// We only want this stuff in the debug build
#define MEASURE_PERFORMANCE
#endif
#endif
//#endif
#endif

#ifdef MEASURE_PERFORMANCE  // Displays frame decompress times on the debugger
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#endif

#include <windows.h>
#include <win32.h>
#include "msvidc.h"

#ifdef DEBUG
    #undef INLINE   // Make debugging easier - less code movement
    #define INLINE
#else
#undef MEASURE_PERFORMANCE  // Turn it off for non debug builds
#endif

#ifdef MEASURE_PERFORMANCE

STATICDT LARGE_INTEGER PC1;    /* current counter value    */
STATICDT LARGE_INTEGER PC2;    /* current counter value    */
STATICDT LARGE_INTEGER PC3;    /* current counter value    */

STATICFN VOID StartCounting(VOID)
{
    QueryPerformanceCounter(&PC1);
    return;
}

STATICFN VOID EndCounting(LPSTR szId)
{
    QueryPerformanceCounter(&PC2);
    PC3.QuadPart = PC2.QuadPart - PC1.QuadPart;
    DPF(("%s: %d ticks", szId, PC3.LowPart));
    return;
}

#else
#define StartCounting()
#define EndCounting(x)

#endif

/*
 * dither table pointers declared and initialised in msvidc.c
 */
extern LPVOID lpDitherTable;

/*
 * these two pointers point into the lpDitherTable
 */
LPBYTE lpLookup;
LPWORD lpScale;

/*
**  Lookup table for expanding 4 bits into 4 bytes
*/
CONST DWORD ExpansionTable[16] = {
                              0x00000000,
                              0x000000FF,
                              0x0000FF00,
                              0x0000FFFF,
                              0x00FF0000,
                              0x00FF00FF,
                              0x00FFFF00,
                              0x00FFFFFF,
                              0xFF000000,
                              0xFF0000FF,
                              0xFF00FF00,
                              0xFF00FFFF,
                              0xFFFF0000,
                              0xFFFF00FF,
                              0xFFFFFF00,
                              0xFFFFFFFF
};

/*
 * Lookup table to turn a bitmask to a byte mask
 */
DWORD Bits2Bytes[13] = {0,          0xffff, 0xffff0000, 0xffffffff,
                        0xffff,     0,      0,          0,
			0xffff0000, 0,      0,          0,
			0xffffffff};

//#include <limits.h>
//#include <mmsystem.h>
//#include <aviffmt.h>

#define RGB555toRGBTRIPLE( rgbT, rgb ) rgbT.rgbtRed=(BYTE)((rgb & 0x7c00) >> 7); \
                                       rgbT.rgbtGreen=(BYTE)((rgb & 0x3e0) >>2); \
                                       rgbT.rgbtBlue=(BYTE)((rgb & 0x1f) << 3)

static WORD edgeBitMask[HEIGHT_CBLOCK*WIDTH_CBLOCK] = {
    0x0001,0x0002,0x0010,0x0020,
    0x0004,0x0008,0x0040,0x0080,
    0x0100,0x0200,0x1000,0x2000,
    0x0400,0x0800,0x4000,0x8000
};

/* make a DWORD that has four copies of the byte x */
#define MAKE4(x)        ( (x << 24) | (x << 16) | (x << 8) | x)

/* make a DWORD that has two copies of the byte x (low word) and two of y */
#define MAKE22(x, y)    ( (y << 24) | (y << 16) | (x << 8) | (x))

/**************************************************************************
compute a pointer into a DIB handling correctly "upside" down DIBs
***************************************************************************/
STATICFN LPVOID DibXY(LPBITMAPINFOHEADER lpbi, LPBYTE lpBits, LONG x, LONG y, INT FAR *pWidthBytes)
{
    int WidthBytes;

    if (x > 0)
        ((BYTE FAR *)lpBits) += ((int)x * (int)lpbi->biBitCount) >> 3;

    WidthBytes = (((((int)lpbi->biWidth * (int)lpbi->biBitCount) >> 3) + 3)&~3);

    if (lpbi->biHeight < 0)
    {
        WidthBytes = -WidthBytes;
        ((BYTE _huge *)lpBits) += lpbi->biSizeImage + WidthBytes;
    }

    if (y > 0)
        ((BYTE _huge *)lpBits) += ((long)y * WidthBytes);

    if (pWidthBytes)
        *pWidthBytes = WidthBytes;

    return lpBits;
}

/*
 * 16-bit decompression to 24-bit RGB--------------------------------------
 */

/*************************************************
purp:   decompress a 4 by 4 compression block to RGBDWORD
entry:  uncmp == address of the destination uncompressed image
        cmp == address of the compressed image
exit:   returns updated address of the compressed image
        and 16 pixels are generated
*************************************************/

// note that the skip count is now stored in the parent stack frame
// and passed as a pointer pSkipCount. This ensures that we are multithread
// safe.

STATICFN HPWORD INLINE DecompressCBlockToRGBTRIPLE(
    HPRGBTRIPLE uncmp,
    HPWORD cmp,
    INT bytesPerRow,
    LONG FAR * pSkipCount
)
{
UINT by;
UINT bx;
UINT y;
UINT x;
WORD mask;
WORD color0;
WORD color1;
WORD bitMask;
RGBTRIPLE rgbTriple0;
RGBTRIPLE rgbTriple1;
HPRGBTRIPLE row;
HPRGBTRIPLE blockRow;
HPRGBTRIPLE blockColumn;
WORD *pEdgeBitMask;


    // check for outstanding skips

    if (*pSkipCount > 0)
    {
        // NOT YET IMPLEMENTED Assert(!"Skip count should be handled by caller");
        (*pSkipCount) --;
        return cmp;
    }

    // get mask and init bit mask
    mask = *cmp++;

    // check for a skip or a solid color

    if (mask & 0x8000)
    {
        if ((mask & ~SKIP_MASK) == SKIP_MAGIC)
        {
            *pSkipCount = (mask & SKIP_MASK);

#ifdef _WIN32
            Assert(*pSkipCount != 0);  // break (on debug builds) if SkipCount == 0
#endif

            (*pSkipCount)--;
            return cmp;
        }
        else
        {
            // solid color
            RGB555toRGBTRIPLE( rgbTriple1, mask );
            for( row = uncmp,y=0; y < HEIGHT_CBLOCK; y++, row = NEXT_RGBT_PIXEL_ROW( row, bytesPerRow ) )
                for( x=0; x < WIDTH_CBLOCK; x++ )
                    row[x] = rgbTriple1;

            return cmp;
        }
    }

    bitMask = 1;
    pEdgeBitMask = edgeBitMask;
    if( (*cmp & 0x8000) != 0 )
    {   // this is an edge with 4 color pairs in four small blocks
        blockRow = uncmp;
        for( by=0; by < 2; by++, blockRow = NEXT_BLOCK_ROW( blockRow, bytesPerRow, EDGE_HEIGHT_CBLOCK ) )
        {
            blockColumn = blockRow;
            for( bx=0; bx < 2; bx++, blockColumn += EDGE_WIDTH_CBLOCK )
            {
                color1 = *cmp++;
                RGB555toRGBTRIPLE( rgbTriple1, color1 );
                color0 = *cmp++;
                RGB555toRGBTRIPLE( rgbTriple0, color0 );
                row = blockColumn;
                for( y=0; y < EDGE_HEIGHT_CBLOCK; y++, row = NEXT_RGBT_PIXEL_ROW( row, bytesPerRow ) )
                {
                    for( x=0; x < EDGE_WIDTH_CBLOCK; x++ )
                    {
                        if( (mask & *pEdgeBitMask++ ) != 0 )
                            row[x] = rgbTriple1;
                        else
                            row[x] = rgbTriple0;
                        bitMask <<= 1;
                    }
                }
            }
        }
    }
    else
    {   // not an edge with only 1 colour pair and one large block
        color1 = *cmp++;
        RGB555toRGBTRIPLE( rgbTriple1, color1 );
        color0 = *cmp++;
        RGB555toRGBTRIPLE( rgbTriple0, color0 );
        row = uncmp;
        for( y=0; y < HEIGHT_CBLOCK; y++, row = NEXT_RGBT_PIXEL_ROW( row, bytesPerRow ) )
        {
            for( x=0; x < WIDTH_CBLOCK; x++ )
            {
                if( (mask & bitMask ) != 0 )
                    row[x] = rgbTriple1;
                else
                    row[x] = rgbTriple0;
                bitMask <<= 1;
            }
        }
    }
    return( cmp );
}


/*************************************************
purp:   decompress the image to RGBTRIPLE
entry:  lpinst = pointer to instance data
        hpCompressed = pointer to compressed data
exit:   returns number of bytes in the uncompressed image
        lpinst->hDib = handle to the uncompressed image
*************************************************/

DWORD FAR PASCAL DecompressFrame24(LPBITMAPINFOHEADER lpbiIn,  LPVOID lpIn,
                    LPBITMAPINFOHEADER lpbiOut, LPVOID lpOut, LONG x, LONG y)
{
HPWORD	    cmp = (HPWORD)lpIn;
int	    bix;
int	    biy;
HPRGBTRIPLE blockRow;
HPRGBTRIPLE blockColumn;
int	    bytesPerRow;
DWORD	    actualSize;
LONG	    SkipCount = 0;

    DPF(("DecompressFrame24:\n"));
    bix = (UINT)((UINT)lpbiIn->biWidth / WIDTH_CBLOCK);
    biy = (UINT)((UINT)lpbiIn->biHeight / HEIGHT_CBLOCK);

    StartCounting();
    blockRow = DibXY(lpbiOut, lpOut, x, y, &bytesPerRow);

    for( y=0; y < biy; y++, blockRow = NEXT_BLOCK_ROW( blockRow, bytesPerRow, HEIGHT_CBLOCK ) )
    {
        blockColumn = blockRow;
        for( x=0; x < bix; x++, blockColumn += WIDTH_CBLOCK )
        {
            cmp = DecompressCBlockToRGBTRIPLE( blockColumn, cmp, bytesPerRow, &SkipCount);
        }
    }

    actualSize = bytesPerRow*biy*HEIGHT_CBLOCK;
    EndCounting("Decompress frame24 took");
    return( actualSize );
}

/*************************************************
*************************************************/
/*
 * -------- 8-bit decompression ----------------------------------------
 *
 *
 * The input stream consists of four cases, handled like this:
 *
 * SKIP		lower 10 bits have skip count
 *     	Return the skip count to the caller (must be multi-thread safe).
 *	Caller will advance the source pointer past the correct number of
 * 	of skipped cells.
 *
 * SOLID	lower 8 bits is solid colour for entire cell
 *	Write the colour to each pixel, four pixels (one DWORD) at
 *	a time.
 *
 * Mask + 2 colours
 *	1s in the mask represent the first colour, 0s the second colour.
 *      Pixels are represented thus:
 *		
 *		C D E F
 *		8 9 A B
 *		4 5 6 7
 *		0 1 2 3
 *
 *      To write four pixels at once, we rely on the fact that:
 *		(a ^ b) ^ a == b
 * 	and also that a ^ 0 == a.
 *	We create a DWORD (Czero) containing four copies of the colour 0, and
 *	another DWORD (Cxor) containing four copies of (colour 0 ^ colour 1).
 *      Then we convert each bit in the mask (1 or 0) into a byte (0xff or 0),
 *      and combining four mask bytes into a DWORD. Then we can select
 *      four pixels at once (AND the mask with Czero and then XOR with Cxor).
 *
 * Mask + 8 colours.
 *	1s and 0s represent two colours as before, but the cell is divided
 *	into 4 subcells with two colours per subcell. The first pair of
 *	colours are for subcell 0145, then 2367, 89cd and abef.
 *	
 *	We use the same algorithm as for the mask+2 case except that when
 *	making the mask, we need colours from the second pair in the top
 *	two bytes of Czero and Cxor, and that we need to change colours
 *	again after two rows.
 *
 * -----------------------------------------------------------------------
 */	

/*
 * DecompressCBlockTo8
 *
 *
 * decompress one cell to 16 8-bit pixels.
 *
 * parameters:
 *   uncmp-     pointer to de-compressed buffer for this block.
 *   cmp -      pointer to compressed data for this block
 *   bytes.. -  size of one row of de-compressed data
 *   pSkipCount - place to return the skipcount if non-zero.
 *
 * returns:
 *   pointer to the next block of compressed data to use.
 */
STATICFN HPWORD INLINE DecompressCBlockTo8(
    HPBYTE uncmp,
    HPWORD cmp,
    INT bytesPerRow,
    LONG FAR * pSkipCount
    )
{
UINT    y;
WORD    mask;
BYTE    b0,b1;
HPBYTE  row;
BYTE	b2, b3;
DWORD	Czero, Cxor;
DWORD	dwBytes;


    // skip counts should be handled by caller
#ifdef _WIN32
    Assert(*pSkipCount == 0);
#endif

    /* first word is the escape word or bit mask */
    mask = *cmp++;

    /*
     * is this an escape ?
     */
    if (mask & 0x8000)
    {

	/* yes - this is either a SKIP code, a solid colour, or an edge
	 * cell (mask + 8 colours).
	 */

        if ((mask & ~SKIP_MASK) == SKIP_MAGIC)
        {
            *pSkipCount = (mask & SKIP_MASK);

#ifdef _WIN32
            Assert(*pSkipCount != 0);  // break (on debug builds) if SkipCount == 0
#endif

            (*pSkipCount)--;      // the current cell
            return cmp;
        }
        else if ((mask & ~SKIP_MASK) == SOLID_MAGIC)
        {
            // solid color
            DWORD  dw;

            //b0 = LOBYTE(mask);
            //dw = b0 | b0<<8 | b0<<16 | b0<<24;
            dw = LOBYTE(mask);
            dw = MAKE4(dw);

#ifdef _WIN32
            Assert(HEIGHT_CBLOCK == 4);    // If this ever changes...
            Assert(WIDTH_CBLOCK == 4);
#endif

            for(y = 0, row = uncmp; y < HEIGHT_CBLOCK;y++, row+= bytesPerRow) {

                // We know we will iterate 4 times (WIDTH_CBLOCK) storing
                // 4 bytes of colour b0 in 4 adjacent rows
                *(DWORD UNALIGNED HUGE *)row = dw;
            }

            return cmp;
        }
        else // this is an edge with 4 color pairs in four small blocks
        {

	    /* read 4 colours, and make AND and XOR masks */
            b0 = *((LPBYTE)cmp)++;
            b1 = *((LPBYTE)cmp)++;
            b2 = *((LPBYTE)cmp)++;
            b3 = *((LPBYTE)cmp)++;
	    Czero = MAKE22(b1, b3);
	    Cxor = Czero ^ MAKE22(b0, b2);

	    row = uncmp;

	    /* first two rows  - top two subcells */
            for (y = 0; y < 2; y++) {

                /* turn bitmask into byte mask */
                dwBytes = ExpansionTable[mask & 0x0f];

                /* select colours and write to dest */
                *( (DWORD UNALIGNED HUGE *)row) = (dwBytes & Cxor) ^ Czero;

                row += bytesPerRow;
                mask >>= 4;

            }

	    /* second two rows  - bottom two subcells */

	    /* read last four colours and make masks */
            b0 = *((LPBYTE)cmp)++;
            b1 = *((LPBYTE)cmp)++;
            b2 = *((LPBYTE)cmp)++;
            b3 = *((LPBYTE)cmp)++;
	    Czero = MAKE22(b1, b3);
	    Cxor = Czero ^ MAKE22(b0, b2);

            for (y = 0; y < 2; y++) {

                /* turn bitmask into byte mask */
                dwBytes = ExpansionTable[mask & 0x0f];

                /* select both colours and write to dest */
                *( (DWORD UNALIGNED HUGE *)row) = (dwBytes & Cxor) ^ Czero;

                row += bytesPerRow;
                mask >>= 4;

            }
        }
    }
    else // not an edge with only 1 colour pair and one large block
    {
	/* use and, xor to map several colours at once.
	 * relies on (Czero ^ Cone) ^ Czero == Cone and Czero ^ 0 == Czero.
	 */


	/* read colours */
	b1 = *((LPBYTE)cmp)++;
	b0 = *((LPBYTE)cmp)++;
	row = uncmp;

	/* make two DWORDs, one with four copies of colour 0, and one
	 * with four copies of (b0 ^ b1).
	 */
	Czero = MAKE4(b0);
	Cxor = Czero ^ MAKE4(b1);

	for (y = 0; y < 4; y++) {
	
            /* turn bitmask into byte mask */
            dwBytes = ExpansionTable[mask & 0x0f];

            /* select both colours and write to dest */
            *( (DWORD UNALIGNED HUGE *)row) = (dwBytes & Cxor) ^ Czero;

            row += bytesPerRow;
	    mask >>= 4;

	}
    }
    return( cmp );
}

/*************************************************
*************************************************/

/*
 * decompress a CRAM-8 DIB to an 8-bit DIB
 *
 * Loop calling DecompressCBlockTo8 for each cell in the input
 * stream. This writes a block of 16 pixels and returns us the
 * pointer for the next block.
 */
DWORD FAR PASCAL DecompressFrame8(LPBITMAPINFOHEADER lpbiIn,  LPVOID lpIn,
                    LPBITMAPINFOHEADER lpbiOut, LPVOID lpOut, LONG x, LONG y)
{
HPWORD  cmp = (HPWORD)lpIn;
int     bix;
int     biy;
HPBYTE  blockRow;
HPBYTE  blockColumn;
LONG SkipCount8 = 0;			// multithread-safe - cannot be static

int     bytesPerRow;

    DPF(("DecompressFrame8:\n"));
    bix = (int)((UINT)lpbiIn->biWidth / WIDTH_CBLOCK);
    biy = (int)((UINT)lpbiIn->biHeight / HEIGHT_CBLOCK);

    StartCounting();
    blockRow = DibXY(lpbiOut, lpOut, x, y, &bytesPerRow);
    for( y=biy; y--; blockRow += bytesPerRow * HEIGHT_CBLOCK )
    {
        blockColumn = blockRow;
        for( x=bix; x--; blockColumn += WIDTH_CBLOCK )
        {
            cmp = DecompressCBlockTo8(blockColumn, cmp, bytesPerRow, &SkipCount8);

            // See if the SkipCount has been set.  If so we want to move to
            // the next location rather than calling DecompressCBlock every
            // time around the loop.  Keep the test simple to minimise the
            // overhead on every iteration that the Skipcount is 0.
            if (SkipCount8) {

                if ((x -= SkipCount8) <0) { // extends past this row
                    LONG SkipRows;

                    // More than just the remainder of this row to skip
                    SkipCount8 =-x;  // These bits are on the next row(s)
                    // SkipCount8 will be >0 otherwise we would have gone
                    // down the else leg.

                    // Calculate how many complete and partial rows to skip.
                    // We know we have skipped at least one row.  The plan
                    // is to restart the X loop at some point along the row.
                    // If the skipcount takes us exactly to the end of a row
                    // we drop out of the x loop, and let the outer y loop do
                    // the decrement.  This takes care of the case when the
                    // skipcount takes us to the very end of the image.

                    SkipRows = 1 + (SkipCount8-1)/bix;

                    // Decrement the row count and set new blockrow start

#ifdef _WIN32
                    if (y<SkipRows) {
                        Assert(y >= SkipRows);
                        SkipRows = y;
                    }
#endif

                    // Unless we have finished we need to reset blockRow
                    y -= SkipRows;
                    // y might be 0, but we must still complete the last row
                    blockRow += bytesPerRow*HEIGHT_CBLOCK*SkipRows;

                    // Calculate the offset into the next row we will process
                    x = SkipCount8%bix;  // This may be 0

                    if (x) {

                        // Set block column by the amount along the row
                        // this iteration is starting, making allowance for
                        // the "for x..." loop iterating blockColumn once.
                        blockColumn = blockRow + ((x-1)*WIDTH_CBLOCK);

                        x=bix-x;  // Get the counter correct
                    }

                    SkipCount8 = 0; // Skip count now exhausted (so am I)

                } else {
                    // SkipCount has been exhausted by this row
                    // Either the row has completed, or there is more data
                    // on this row.   Check...
                    if (x) {
                        // More of this row left
                        // Worry about moving blockColumn on the right amount
                        blockColumn += WIDTH_CBLOCK*SkipCount8;
                    } // else x==0 and we will drop out of the "for x..." loop
                      // blockColumn will be reset when we reenter the x loop
                    SkipCount8=0;
                }
            }
        }
    }
    EndCounting("Decompress 8bit took");

    return 0;
}


#ifdef _WIN32

/* ---- 8-bit X2 decompress - in asm for Win16 ---------------------------*/

/*
 * decompress one block, stretching by 2.
 *
 * parameters:
 *   uncmp-     pointer to de-compressed buffer for this block.
 *   cmp -      pointer to compressed data for this block
 *   bytes.. -  size of one row of de-compressed data
 *
 * returns:
 *   pointer to the next block of compressed data.
 *
 * Given same incoming data, write a block of four pixels for every
 * pixel in original compressed image. Uses same techniques as
 * unstretched routine, masking and writing four pixels (one dword)
 * at a time.
 *
 * Stretching by 2 is done by simple pixel duplication.
 * Experiments were done (x86) to only store every other line, then to use
 * memcpy to fill in the gaps.  This is slower than writing two identical
 * lines as you proceed.
 *
 * Skip counts are returned (via pSkipCount) to the caller, who will handle
 * advancing the source pointer accordingly.
 *
 */

STATICFN HPWORD INLINE DecompressCBlockTo8X2(
    HPBYTE uncmp,
    HPWORD cmp,
    INT bytesPerRow,
    LONG FAR * pSkipCount)
{
    UINT    y;
    UINT    dx, dy;
    WORD    mask;
    BYTE    b0,b1;
    HPBYTE  row;
    DWORD Czero, Cxor, dwBytes;
    DWORD Ctwo, Cxor2;

    // skip counts should be handled by caller
#ifdef _WIN32
    Assert (*pSkipCount == 0);
#endif

    // get mask and init bit mask
    mask = *cmp++;

    // check for a skip or a solid color

    if (mask & 0x8000)
    {
        if ((mask & ~SKIP_MASK) == SKIP_MAGIC)
        {
            *pSkipCount = (mask & SKIP_MASK);

#ifdef _WIN32
            Assert(*pSkipCount != 0);  // break (on debug builds) if SkipCount == 0
#endif

            (*pSkipCount)--;
            return cmp;
        }
        else if ((mask & ~SKIP_MASK) == SOLID_MAGIC)
        {
            // solid color
            DWORD  dw;

            //b0 = LOBYTE(mask);
            //dw = b0 | b0<<8 | b0<<16 | b0<<24;
            dw = LOBYTE(mask);
            dw = MAKE4(dw);

#ifdef _WIN32
            Assert(HEIGHT_CBLOCK == 4);    // If this ever changes...
            Assert(WIDTH_CBLOCK == 4);
#endif

            dx = WIDTH_CBLOCK * 2;
            dy = HEIGHT_CBLOCK * 2;
            for(row = uncmp; dy--; row+= bytesPerRow) {

                // We know we will iterate 8 times (dx) value storing
                // 4 bytes of colour b0 in eight adjacent rows
                *(DWORD UNALIGNED HUGE *)row = dw;
                *((DWORD UNALIGNED HUGE *)row+1) = dw;
            }

            return cmp;
        }
        else // this is an edge with 4 color pairs in four small blocks
        {
	    /* read 2 colours, and make AND and XOR masks for first subcell*/
            b0 = *((LPBYTE)cmp)++;
            b1 = *((LPBYTE)cmp)++;
	    Czero = MAKE4(b1);
	    Cxor = Czero ^ MAKE4(b0);

	    /* colour masks for second subcell */
            b0 = *((LPBYTE)cmp)++;
            b1 = *((LPBYTE)cmp)++;
	    Ctwo = MAKE4(b1);
	    Cxor2 = Ctwo ^ MAKE4(b0);

	    row = uncmp;

	    /* first two rows  - top two subcells */
            for (y = 0; y < 2; y++) {

                /* --- first subcell (two pixels) ----  */

                /* turn bitmask into byte mask */
#if 0
                dwBytes = ((mask & 1) ? 0xffff: 0) |
                   ((mask & 2) ? 0xffff0000 : 0);
#else
                dwBytes = Bits2Bytes[mask&3];
#endif

                /* select both colours and write to dest */
                dwBytes = (dwBytes & Cxor) ^ Czero;
                *( (DWORD UNALIGNED HUGE *)row) = dwBytes;
                *( (DWORD UNALIGNED HUGE *)(row + bytesPerRow)) = dwBytes;

                /* ---- second subcell (two pixels) --- */
                /* turn bitmask into byte mask */
#if 0
                dwBytes = ((mask & 4) ? 0xffff: 0) |
                   ((mask & 8) ? 0xffff0000 : 0);
#else
                dwBytes = Bits2Bytes[mask&0xc];
#endif

                /* select both colours and write to dest */
                dwBytes = (dwBytes & Cxor2) ^ Ctwo;
                *( (DWORD UNALIGNED HUGE *)(row + sizeof(DWORD))) = dwBytes;
                *( (DWORD UNALIGNED HUGE *)(row + sizeof(DWORD) + bytesPerRow)) = dwBytes;

                row += bytesPerRow * 2;
                mask >>= 4;

            }

	    /* second two rows  - bottom two subcells */

	    /* read 2 colours, and make AND and XOR masks for first subcell*/
            b0 = *((LPBYTE)cmp)++;
            b1 = *((LPBYTE)cmp)++;
	    Czero = MAKE4(b1);
	    Cxor = Czero ^ MAKE4(b0);

	    /* colour masks for second subcell */
            b0 = *((LPBYTE)cmp)++;
            b1 = *((LPBYTE)cmp)++;
	    Ctwo = MAKE4(b1);
	    Cxor2 = Ctwo ^ MAKE4(b0);


            for (y = 0; y < 2; y++) {

                /* --- first subcell (two pixels) ----  */

                /* turn bitmask into byte mask */
#if 0
                dwBytes = ((mask & 1) ? 0xffff: 0) |
                   ((mask & 2) ? 0xffff0000 : 0);
#else
                dwBytes = Bits2Bytes[mask&3];
#endif

                /* select both colours and write to dest */
                dwBytes = (dwBytes & Cxor) ^ Czero;
                *( (DWORD UNALIGNED HUGE *)row) = dwBytes;
                *( (DWORD UNALIGNED HUGE *)(row + bytesPerRow)) = dwBytes;

                /* ---- second subcell (two pixels) --- */
                /* turn bitmask into byte mask */
#if 0
                dwBytes = ((mask & 4) ? 0xffff: 0) |
                   ((mask & 8) ? 0xffff0000 : 0);
#else
                dwBytes = Bits2Bytes[mask&0xc];
#endif

                /* select both colours and write to dest */
                dwBytes = (dwBytes & Cxor2) ^ Ctwo;
                *( (DWORD UNALIGNED HUGE *)(row + sizeof(DWORD))) = dwBytes;
                *( (DWORD UNALIGNED HUGE *)(row + sizeof(DWORD) + bytesPerRow)) = dwBytes;

                row += bytesPerRow * 2;
                mask >>= 4;

            }
        }
    }
    else // not an edge with only 1 color pair and one large block
    {
	/* use and, xor to map several colours at once.
	 * relies on (Czero ^ Cone) ^ Czero == Cone and Czero ^ 0 == Czero.
	 */

	/* read colours */
        b1 = *((LPBYTE)cmp)++;
        b0 = *((LPBYTE)cmp)++;
	row = uncmp;

	/* make two DWORDs, one with four copies of colour 0, and one
	 * with four copies of (b0 ^ b1).
	 */
	Czero = MAKE4(b0);
	Cxor = Czero ^ MAKE4(b1);

	for (y = 0; y < 4; y++) {
	
            /* --- first two pixels in row ----  */

            /* turn bitmask into byte mask */
#if 0
            dwBytes = ((mask & 1) ? 0xffff: 0) |
                   ((mask & 2) ? 0xffff0000 : 0);
#else
            dwBytes = Bits2Bytes[mask&3];
#endif

            /* select both colours and write to dest */
	    dwBytes = (dwBytes & Cxor) ^ Czero;
            *( (DWORD UNALIGNED HUGE *)row) = dwBytes;
            *( (DWORD UNALIGNED HUGE *)(row + bytesPerRow)) = dwBytes;
	

	    /* ---- second two pixels in row ---- */
            /* turn bitmask into byte mask */
#if 0
            dwBytes = ((mask & 4) ? 0xffff: 0) |
               ((mask & 8) ? 0xffff0000 : 0);
#else
            dwBytes = Bits2Bytes[mask&0xc];
#endif

            /* select both colours and write to dest */
	    dwBytes = (dwBytes & Cxor) ^ Czero;
            *( (DWORD UNALIGNED HUGE *)(row + sizeof(DWORD))) = dwBytes;
            *( (DWORD UNALIGNED HUGE *)(row + sizeof(DWORD) + bytesPerRow)) = dwBytes;

            row += bytesPerRow * 2;
	    mask >>= 4;
	}
    }
    return( cmp );
}

/*
 * decompress one frame, stretching by 2.
 *
 * parameters:
 *   lpbiIn     pointer to compressed buffer for this frame
 *   lpIn       pointer to compressed data for this block
 *   lpbiOut    pointer to decompressed bitmap header
 *   lpOut      pointer to where to store the decompressed data
 *
 * returns:
 *   0 on success
 *
 * Uses  DecompressCBlockTo8X2 (see above) to do the decompression.
 * This also returns (via a pointer to SkipCount8X2) the count of cells
 * to skip. We can then move the source and target pointers on
 * until the SkipCount is exhausted.
 */

DWORD FAR PASCAL DecompressFrame8X2C(LPBITMAPINFOHEADER lpbiIn,  LPVOID lpIn,
                    LPBITMAPINFOHEADER lpbiOut, LPVOID lpOut, LONG x, LONG y)
{
HPWORD  cmp = (HPWORD)lpIn;
int     bix;
int     biy;
HPBYTE  blockRow;
HPBYTE  blockColumn;
LONG    SkipCount8X2 = 0;
int     bytesPerRow;


    DPF(("DecompressFrame8X2C:\n"));
    bix = (int)(lpbiIn->biWidth) / (WIDTH_CBLOCK);
    biy = (int)(lpbiIn->biHeight) / (HEIGHT_CBLOCK);

    StartCounting();

    blockRow = DibXY(lpbiOut, lpOut, x, y, &bytesPerRow);

    for( y=biy; y--; blockRow += bytesPerRow * HEIGHT_CBLOCK*2 )
    {
        blockColumn = blockRow;

        for( x=bix; x--; blockColumn += WIDTH_CBLOCK*2 )
        {
            cmp = DecompressCBlockTo8X2(blockColumn, cmp, bytesPerRow, &SkipCount8X2);

            // See if the SkipCount has been set.  If so we want to move to
            // the next location rather than calling DecompressCBlock every
            // time around the loop.  Keep the test simple to minimise the
            // overhead on every iteration that the Skipcount is 0.
            if (SkipCount8X2) {

                if ((x -= SkipCount8X2) <0) { // extends past this row
                    LONG SkipRows;

                    // More than just the remainder of this row to skip
                    SkipCount8X2 =-x;  // These bits are on the next row(s)
                    // SkipCount8X2 will be >0 otherwise we would have gone
                    // down the else leg.

                    // Calculate how many complete and partial rows to skip.
                    // We know we have skipped at least one row.  The plan
                    // is to restart the X loop at some point along the row.
                    // If the skipcount takes us exactly to the end of a row
                    // we drop out of the x loop, and let the outer y loop do
                    // the decrement.  This takes care of the case when the
                    // skipcount takes us to the very end of the image.

                    SkipRows = 1 + (SkipCount8X2-1)/bix;

                    // Decrement the row count and set new blockrow start

#ifdef _WIN32
                    if (y<SkipRows) {
                        Assert(y >= SkipRows);
                        SkipRows = y;
                    }
#endif

                    // Unless we have finished we need to reset blockRow
                    y -= SkipRows;
                    // y might be 0, but we must still complete the last row
                    blockRow += bytesPerRow*HEIGHT_CBLOCK*2*SkipRows;

                    // Calculate the offset into the next row we will process
                    x = SkipCount8X2%bix;  // This may be 0

                    if (x) {

                        // Set block column by the amount along the row
                        // this iteration is starting, making allowance for
                        // the "for x..." loop iterating blockColumn once.
                        blockColumn = blockRow + ((x-1)*WIDTH_CBLOCK*2);

                        x=bix-x;  // Get the counter correct
                    }

                    SkipCount8X2 = 0; // Skip count now exhausted (so am I)

                } else {
                    // SkipCount has been exhausted by this row
                    // Either the row has completed, or there is more data
                    // on this row.   Check...
                    if (x) {
                        // More of this row left
                        // Worry about moving blockColumn on the right amount
                        blockColumn += WIDTH_CBLOCK*2*SkipCount8X2;
                    } // else x==0 and we will drop out of the "for x..." loop
                      // blockColumn will be reset when we reenter the x loop
                    SkipCount8X2=0;
                }
            }
        }
    }

    EndCounting("Decompress and stretch 8x2 took");
    return 0;
}

/*
 * -------- 16-bit decompression ----------------------------------------
 *
 *
 * CRAM-16 has 16-bit mask or escape code, together with 16-bit (RGB555)
 * colour words. We decode to 16 bits, to 24-bits (above), and to 8 bits
 * stretched 1:1 and 1:2 (this case DecompressFrame16To8X2C does
 * decompression, dithering and stretching in one pass.
 *
 * The input stream consists of four cases:
 *
 * SOLID	top bit set, lower 15 bits is solid colour for entire cell
 *      If the red element (bits 9-14) = '00001', then this is not a solid
 *	colour but a skip count.
 *	Write the colour to each pixel, two pixels (one DWORD) at
 *	a time.
 *
 * SKIP		top 6 bits = 100001xxxxxxxxxx, lower 10 bits have skip count
 *      Store the skip count via a pointer to a variable passed by the.
 *	parent - this way the skip count is maintained across calls
 *
 * Mask + 2 colours   (top bit 0, bit 15 of first colour word also 0)
 *	1s in the mask represent the first colour, 0s the second colour.
 *      Pixels are represented thus:
 *		
 *		C D E F
 *		8 9 A B
 *		4 5 6 7
 *		0 1 2 3
 *
 *
 * Mask + 8 colours.	(top bit 0, bit 15 of first colour word == 1)
 * 	
 *	1s and 0s represent two colours as before, but the cell is divided
 *	into 4 subcells with two colours per subcell. The first pair of
 *	colours are for subcell 0145, then 2367, 89cd and abef.
 *	
 *
 * Dithering:
 *
 *   we use the table method from drawdib\dith775.c, and we import the
 * same tables and palette by including their header file. We have a fixed
 * palette in which we have 7 levels of red, 7 levels of green and 5 levels of
 * blue (= 245 combinations) in a 256-colour palette. We use tables
 * to quantize the colour elements to 7 levels, combine them into an 8-bit
 * value and then lookup in a table that maps this combination to the actual
 * palette. Before quantizing, we add on small corrections (less than one
 * level) based on the x,y position of the pixel to balance the
 * colour over a 4x4 pixel area: this makes the decompression slightly more
 * awkward since we dither differently for any x, y position within the cell.
 *
 * -----------------------------------------------------------------------
 */	

/* ---- 16-bit decompress to 16 bits ----------------------------------*/


/*
 * decompress one 16bpp block to RGB555.
 *
 * parameters:
 *   uncmp-     pointer to de-compressed buffer for this block.
 *   cmp -      pointer to compressed data for this block
 *   bytes.. -  size of one row of de-compressed data
 *   pSkipCount - outstanding count of cells to skip - set here and just stored
 * 		in parent stack frame for multi-thread-safe continuity.
 *
 * returns:
 *   pointer to the next block of compressed data.
 *
 */

STATICFN HPWORD INLINE
DecompressCBlock16To555(
    HPBYTE uncmp,
    HPWORD cmp,
    INT bytesPerRow,
    LONG FAR *pSkipCount
)
{
    UINT    y;
    WORD    mask;
    WORD    col0, col1;
    HPBYTE  row;
    DWORD Czero, Cxor, Ctwo, Cxor2, dwBytes;

    // check for outstanding skips

    if (*pSkipCount > 0)
    {
        (*pSkipCount)--;
        return cmp;
    }

    // get mask and init bit mask
    mask = *cmp++;

    // check for a skip or a solid color

    if (mask & 0x8000)
        {
        if ((mask & ~SKIP_MASK) == SKIP_MAGIC)
            {
            *pSkipCount = (mask & SKIP_MASK);

#ifdef _WIN32
            Assert(*pSkipCount != 0);  // break (on debug builds) if SkipCount == 0
#endif

            (*pSkipCount)--;
            return cmp;
            }
        else /* must be solid colour */
        {

	    /* write four rows of 4 2-byte pixels of col0 */

	    /* solid colour is lower 15 bits of mask */
            col0 = mask & 0x7fff;
	    Czero = col0 | (col0 << 16);

            for(row = uncmp, y = 0; y < HEIGHT_CBLOCK; y++, row+= bytesPerRow) {


                *(DWORD UNALIGNED HUGE *)row = Czero;
		*((DWORD UNALIGNED HUGE *)row+1) = Czero;

        }

            return cmp;
    }
    }


    /* in 16-bit CRAM, both 4-pair and 1-pair cells have bit 15 of mask set
     * to zero. We distinguish between them based on bit 15 of the first
     * colour. if this is set, this is the 4-pair edge case cell.
     */
    if (*cmp & 0x8000) {
        // this is an edge with 4 colour pairs in four small blocks

	/* read 2 colours, and make AND and XOR masks for first subcell*/
	col0 = *cmp++;
	col1 = *cmp++;
	Czero = col1 | (col1 << 16);
	Cxor = Czero ^ (col0 | (col0 << 16));

	/* colour masks for second subcell */
	col0 = *cmp++;
	col1 = *cmp++;
	Ctwo = col1 | (col1 << 16);
	Cxor2 = Ctwo ^ (col0 | (col0 << 16));


	row = uncmp;

	/* first two rows  - top two subcells */
	for (y = 0; y < 2; y++) {

	    /* --- first subcell (two pixels) ----  */

	    /* turn bitmask into byte mask */
#if 0
            dwBytes = ((mask & 1) ? 0xffff: 0) |
                   ((mask & 2) ? 0xffff0000 : 0);
#else
            dwBytes = Bits2Bytes[mask&3];
#endif

	    /* select both colours and write to dest */
	    dwBytes = (dwBytes & Cxor) ^ Czero;
	    *( (DWORD UNALIGNED HUGE *)row) = dwBytes;


	    /* ---- second subcell (two pixels) --- */
	    /* turn bitmask into byte mask */
#if 0
            dwBytes = ((mask & 4) ? 0xffff: 0) |
               ((mask & 8) ? 0xffff0000 : 0);
#else
            dwBytes = Bits2Bytes[mask&0xc];
#endif

	    /* select both colours and write to dest */
	    dwBytes = (dwBytes & Cxor2) ^ Ctwo;
	    *( (DWORD UNALIGNED HUGE *)(row + sizeof(DWORD))) = dwBytes;

	    row += bytesPerRow;
	    mask >>= 4;


	}

	/* second two rows  - bottom two subcells */

	/* read 2 colours, and make AND and XOR masks for first subcell*/
	col0 = *cmp++;
	col1 = *cmp++;
	Czero = col1 | (col1 << 16);
	Cxor = Czero ^ (col0 | (col0 << 16));

	/* colour masks for second subcell */
	col0 = *cmp++;
	col1 = *cmp++;
	Ctwo = col1 | (col1 << 16);
	Cxor2 = Ctwo ^ (col0 | (col0 << 16));



	for (y = 0; y < 2; y++) {

	    /* --- first subcell (two pixels) ----  */

	    /* turn bitmask into byte mask */
#if 0
            dwBytes = ((mask & 1) ? 0xffff: 0) |
                   ((mask & 2) ? 0xffff0000 : 0);
#else
            dwBytes = Bits2Bytes[mask&3];
#endif

	    /* select both colours and write to dest */
	    dwBytes = (dwBytes & Cxor) ^ Czero;
	    *( (DWORD UNALIGNED HUGE *)row) = dwBytes;


	    /* ---- second subcell (two pixels) --- */
	    /* turn bitmask into byte mask */
#if 0
            dwBytes = ((mask & 4) ? 0xffff: 0) |
               ((mask & 8) ? 0xffff0000 : 0);
#else
            dwBytes = Bits2Bytes[mask&0xc];
#endif

	    /* select both colours and write to dest */
	    dwBytes = (dwBytes & Cxor2) ^ Ctwo;
	    *( (DWORD UNALIGNED HUGE *)(row + sizeof(DWORD))) = dwBytes;

	    row += bytesPerRow;
	    mask >>= 4;

	}

    } else {
    	// not an edge with only 1 colour pair and one large block


	/* read colours */
	col0 = *cmp++;
	col1 = *cmp++;
	Czero = col1 | (col1 << 16);
	Cxor = Czero ^ (col0 | (col0 << 16));

	row = uncmp;

	for (y = 0; y < 4; y++) {


            /* --- first two pixels in row ----  */

            /* turn bitmask into byte mask */
#if 0
            dwBytes = ((mask & 1) ? 0xffff: 0) |
                   ((mask & 2) ? 0xffff0000 : 0);
#else
            dwBytes = Bits2Bytes[mask&3];
#endif

            /* select both colours and write to dest */
	    dwBytes = (dwBytes & Cxor) ^ Czero;
            *( (DWORD UNALIGNED HUGE *)row) = dwBytes;
	

	    /* ---- second two pixels in row ---- */
            /* turn bitmask into byte mask */
#if 0
            dwBytes = ((mask & 4) ? 0xffff: 0) |
               ((mask & 8) ? 0xffff0000 : 0);
#else
            dwBytes = Bits2Bytes[mask&0xc];
#endif

            /* select both colours and write to dest */
	    dwBytes = (dwBytes & Cxor) ^ Czero;
            *( (DWORD UNALIGNED HUGE *)(row + sizeof(DWORD))) = dwBytes;

            row += bytesPerRow;
	    mask >>= 4;

	}
    }
    return( cmp );

}


DWORD FAR PASCAL DecompressFrame16To555C(LPBITMAPINFOHEADER lpbiIn,  LPVOID lpIn,
                    LPBITMAPINFOHEADER lpbiOut, LPVOID lpOut, LONG x, LONG y)
{
HPWORD  cmp = (HPWORD)lpIn;
INT     bix;
INT     biy;
HPBYTE  blockRow;
HPBYTE  blockColumn;
LONG SkipCount = 0;
INT     bytesPerRow;

    DPF(("DecompressFrame16To555C:\n"));
    bix = (UINT)(lpbiIn->biWidth) / (WIDTH_CBLOCK);   // No negative values in
    biy = (UINT)(lpbiIn->biHeight) / (HEIGHT_CBLOCK); // width or height fields

    StartCounting();

    blockRow = DibXY(lpbiOut, lpOut, x, y, &bytesPerRow);

    for( y=0; y < biy; y++, blockRow += bytesPerRow * HEIGHT_CBLOCK )
    {
        blockColumn = blockRow;
        for( x=0; x < bix; x++, blockColumn += (WIDTH_CBLOCK * sizeof(WORD)))
        {
            cmp = DecompressCBlock16To555(blockColumn, cmp, bytesPerRow, &SkipCount);
        }
    }

    EndCounting("Decompress Frame16To555C took");

    return 0;
}


// 16-bit 565 decompression


// macro to convert a 15-bit 555 colour to a 16-bit 565 colour
#define RGB555_TO_RGB565(c)	(c = ( ((c & 0x7fe0) << 1) | (c & 0x1f)))


/*
 * decompress one 16bpp block to RGB565.
 *
 * same as RGB555 but we need a colour translation between 555->565
 *
 * parameters:
 *   uncmp-     pointer to de-compressed buffer for this block.
 *   cmp -      pointer to compressed data for this block
 *   bytes.. -  size of one row of de-compressed data
 *   pSkipCount - outstanding count of cells to skip - set here and just stored
 * 		in parent stack frame for multi-thread-safe continuity.
 *
 * returns:
 *   pointer to the next block of compressed data.
 *
 */

STATICFN HPWORD INLINE
DecompressCBlock16To565(
    HPBYTE uncmp,
    HPWORD cmp,
    INT bytesPerRow,
    LONG FAR * pSkipCount
)
{
    UINT    y;
    WORD    mask;
    WORD    col0, col1;
    HPBYTE  row;
    DWORD Czero, Cxor, Ctwo, Cxor2, dwBytes;

    // check for outstanding skips

    if (*pSkipCount > 0)
    {
        (*pSkipCount)--;
        return cmp;
    }

    // get mask and init bit mask
    mask = *cmp++;

    // check for a skip or a solid color

    if (mask & 0x8000)
        {
        if ((mask & ~SKIP_MASK) == SKIP_MAGIC)
            {
            *pSkipCount = (mask & SKIP_MASK);

#ifdef _WIN32
            Assert(*pSkipCount != 0);  // break (on debug builds) if SkipCount == 0
#endif

            (*pSkipCount)--;
            return cmp;
            }
        else /* must be solid colour */
        {

	    /* write four rows of 4 2-byte pixels of col0 */

	    /* solid colour is lower 15 bits of mask */
            col0 = mask & 0x7fff;
	    RGB555_TO_RGB565(col0);
	    Czero = col0 | (col0 << 16);

            for(row = uncmp, y = 0; y < HEIGHT_CBLOCK; y++, row+= bytesPerRow) {


                *(DWORD UNALIGNED HUGE *)row = Czero;
		*((DWORD UNALIGNED HUGE *)row+1) = Czero;

        }

            return cmp;
    }
    }


    /* in 16-bit CRAM, both 4-pair and 1-pair cells have bit 15 of mask set
     * to zero. We distinguish between them based on bit 15 of the first
     * colour. if this is set, this is the 4-pair edge case cell.
     */
    if (*cmp & 0x8000) {
        // this is an edge with 4 colour pairs in four small blocks

	/* read 2 colours, and make AND and XOR masks for first subcell*/
	col0 = *cmp++;
	RGB555_TO_RGB565(col0);
	col1 = *cmp++;
	RGB555_TO_RGB565(col1);
	Czero = col1 | (col1 << 16);
	Cxor = Czero ^ (col0 | (col0 << 16));

	/* colour masks for second subcell */
	col0 = *cmp++;
	RGB555_TO_RGB565(col0);
	col1 = *cmp++;
	RGB555_TO_RGB565(col1);
	Ctwo = col1 | (col1 << 16);
	Cxor2 = Ctwo ^ (col0 | (col0 << 16));


	row = uncmp;

	/* first two rows  - top two subcells */
	for (y = 0; y < 2; y++) {

	    /* --- first subcell (two pixels) ----  */

	    /* turn bitmask into byte mask */
#if 0
            dwBytes = ((mask & 1) ? 0xffff: 0) |
                   ((mask & 2) ? 0xffff0000 : 0);
#else
            dwBytes = Bits2Bytes[mask&3];
#endif

	    /* select both colours and write to dest */
	    dwBytes = (dwBytes & Cxor) ^ Czero;
	    *( (DWORD UNALIGNED HUGE *)row) = dwBytes;


	    /* ---- second subcell (two pixels) --- */
	    /* turn bitmask into byte mask */
#if 0
            dwBytes = ((mask & 4) ? 0xffff: 0) |
               ((mask & 8) ? 0xffff0000 : 0);
#else
            dwBytes = Bits2Bytes[mask&0xc];
#endif

	    /* select both colours and write to dest */
	    dwBytes = (dwBytes & Cxor2) ^ Ctwo;
	    *( (DWORD UNALIGNED HUGE *)(row + sizeof(DWORD))) = dwBytes;

	    row += bytesPerRow;
	    mask >>= 4;


	}

	/* second two rows  - bottom two subcells */

	/* read 2 colours, and make AND and XOR masks for first subcell*/
	col0 = *cmp++;
	RGB555_TO_RGB565(col0);
	col1 = *cmp++;
	RGB555_TO_RGB565(col1);
	Czero = col1 | (col1 << 16);
	Cxor = Czero ^ (col0 | (col0 << 16));

	/* colour masks for second subcell */
	col0 = *cmp++;
	RGB555_TO_RGB565(col0);
	col1 = *cmp++;
	RGB555_TO_RGB565(col1);
	Ctwo = col1 | (col1 << 16);
	Cxor2 = Ctwo ^ (col0 | (col0 << 16));



	for (y = 0; y < 2; y++) {

	    /* --- first subcell (two pixels) ----  */

	    /* turn bitmask into byte mask */
#if 0
            dwBytes = ((mask & 1) ? 0xffff: 0) |
                   ((mask & 2) ? 0xffff0000 : 0);
#else
            dwBytes = Bits2Bytes[mask&3];
#endif

	    /* select both colours and write to dest */
	    dwBytes = (dwBytes & Cxor) ^ Czero;
	    *( (DWORD UNALIGNED HUGE *)row) = dwBytes;


	    /* ---- second subcell (two pixels) --- */
	    /* turn bitmask into byte mask */
#if 0
            dwBytes = ((mask & 4) ? 0xffff: 0) |
               ((mask & 8) ? 0xffff0000 : 0);
#else
            dwBytes = Bits2Bytes[mask&0xc];
#endif

	    /* select both colours and write to dest */
	    dwBytes = (dwBytes & Cxor2) ^ Ctwo;
	    *( (DWORD UNALIGNED HUGE *)(row + sizeof(DWORD))) = dwBytes;

	    row += bytesPerRow;
	    mask >>= 4;

	}

    } else {
    	// not an edge with only 1 colour pair and one large block


	/* read colours */
	col0 = *cmp++;
	RGB555_TO_RGB565(col0);
	col1 = *cmp++;
	RGB555_TO_RGB565(col1);
	Czero = col1 | (col1 << 16);
	Cxor = Czero ^ (col0 | (col0 << 16));

	row = uncmp;

	for (y = 0; y < 4; y++) {


            /* --- first two pixels in row ----  */

            /* turn bitmask into byte mask */
#if 0
            dwBytes = ((mask & 1) ? 0xffff: 0) |
                   ((mask & 2) ? 0xffff0000 : 0);
#else
            dwBytes = Bits2Bytes[mask&3];
#endif

            /* select both colours and write to dest */
	    dwBytes = (dwBytes & Cxor) ^ Czero;
            *( (DWORD UNALIGNED HUGE *)row) = dwBytes;
	

	    /* ---- second two pixels in row ---- */
            /* turn bitmask into byte mask */
#if 0
            dwBytes = ((mask & 4) ? 0xffff: 0) |
               ((mask & 8) ? 0xffff0000 : 0);
#else
            dwBytes = Bits2Bytes[mask&0xc];
#endif

            /* select both colours and write to dest */
	    dwBytes = (dwBytes & Cxor) ^ Czero;
            *( (DWORD UNALIGNED HUGE *)(row + sizeof(DWORD))) = dwBytes;

            row += bytesPerRow;
	    mask >>= 4;

	}
    }
    return( cmp );

}

DWORD FAR PASCAL DecompressFrame16To565C(LPBITMAPINFOHEADER lpbiIn,  LPVOID lpIn,
                    LPBITMAPINFOHEADER lpbiOut, LPVOID lpOut, LONG x, LONG y)
{
HPWORD  cmp = (HPWORD)lpIn;
INT     bix;
INT     biy;
HPBYTE  blockRow;
HPBYTE  blockColumn;
LONG	SkipCount = 0;
INT     bytesPerRow;

    DPF(("DecompressFrame16To565C:\n"));
    bix = (UINT)(lpbiIn->biWidth) / (WIDTH_CBLOCK);   // No negative values in
    biy = (UINT)(lpbiIn->biHeight) / (HEIGHT_CBLOCK); // width or height fields

    StartCounting();
    blockRow = DibXY(lpbiOut, lpOut, x, y, &bytesPerRow);

    for( y=0; y < biy; y++, blockRow += bytesPerRow * HEIGHT_CBLOCK )
    {
        blockColumn = blockRow;
        for( x=0; x < bix; x++, blockColumn += (WIDTH_CBLOCK * sizeof(WORD)))
        {
            cmp = DecompressCBlock16To565(blockColumn, cmp, bytesPerRow, &SkipCount);
        }
    }
    EndCounting("Decompress Frame16To565C took");

    return 0;
}


/* ---- 16-bit decompress & dither to 8 bit - in asm for Win16 ---------------------------*/

/*
 * dither using SCALE method. see dcram168.asm or drawdib\dith775a.asm
 *
 * 	8-bit colour = lookup[ scale[ rgb555] + err]
 *
 * where error is one of the values in the 4x4 array below to balance
 * the colour.
 */

/*
 * dither error array - values to add to rgb value after scaling before
 * converting to 8 bits. Balances colour over a 4x4 matrix
 */
int ditherr[4][4] = {
	{0,    3283, 4924, 8207},
	{6565, 6566, 1641, 1642},
	{3283, 0,    8207, 4924},
	{6566, 4925, 3282, 1641}
};

/* scale the rgb555 first by lookup in lpScale[rgb555] */
#define DITHER16TO8(col, x, y)		lpLookup[col + ditherr[(y)&3][(x)&3]]


/*
 * decompress one 16bpp block, and dither to 8 bpp using table dither method.
 *
 * parameters:
 *   uncmp-     pointer to de-compressed buffer for this block.
 *   cmp -      pointer to compressed data for this block
 *   bytes.. -  size of one row of de-compressed data
 *   pSkipCount - skipcount stored in parent stack frame
 *
 * returns:
 *   pointer to the next block of compressed data.
 *
 */

STATICFN HPWORD INLINE
DecompressCBlock16To8(
    HPBYTE uncmp,
    HPWORD cmp,
    INT bytesPerRow,
    LONG * pSkipCount
)
{
    UINT    y;
    WORD    mask;
    WORD    col0, col1, col2, col3;
    HPBYTE  row;
    DWORD Czero, Cone, Cxor, dwBytes;

    // check for outstanding skips

    if (*pSkipCount > 0)
    {
        Assert(!"Skip count should be handled by caller");
        (*pSkipCount)--;
        return cmp;
    }

    // get mask and init bit mask
    mask = *cmp++;

    // check for a skip or a solid color

    if (mask & 0x8000)
    {
        if ((mask & ~SKIP_MASK) == SKIP_MAGIC)
        {
            *pSkipCount = (mask & SKIP_MASK);

#ifdef _WIN32
            Assert(*pSkipCount != 0);  // break (on debug builds) if SkipCount == 0
#endif

            (*pSkipCount)--;
            return cmp;
        }
        else /* must be solid colour */
        {

	    /* solid colour is lower 15 bits of mask */
            col0 = lpScale[mask & 0x7fff];

            for(row = uncmp, y = 0; y < HEIGHT_CBLOCK; y++, row+= bytesPerRow) {

		/* convert colour once for each row */
		Czero = (DITHER16TO8(col0, 0, y) ) |
			(DITHER16TO8(col0, 1, y) << 8 ) |
			(DITHER16TO8(col0, 2, y) << 16 ) |
			(DITHER16TO8(col0, 3, y) << 24 );

                *(DWORD UNALIGNED HUGE *)row = Czero;
            }

            return cmp;
        }
    }


    /* in 16-bit CRAM, both 4-pair and 1-pair cells have bit 15 of mask set
     * to zero. We distinguish between them based on bit 15 of the first
     * colour. if this is set, this is the 4-pair edge case cell.
     */
    if (*cmp & 0x8000) {
        // this is an edge with 4 color pairs in four small blocks

	col0 = lpScale[(*cmp++) & 0x7fff];
	col1 = lpScale[(*cmp++) & 0x7fff];
	col2 = lpScale[(*cmp++) & 0x7fff];
	col3 = lpScale[(*cmp++) & 0x7fff];

	row = uncmp;

	/* first two rows  - top two subcells */
	for (y = 0; y < 2; y++) {


	    /* dithering requires that we make different
	     * colour masks depending on x and y position - and
	     * therefore re-do it each row
	     */
	    Czero = (DITHER16TO8(col1, 0, y) ) |
		    (DITHER16TO8(col1, 1, y) << 8 ) |
		    (DITHER16TO8(col3, 2, y) << 16 ) |
		    (DITHER16TO8(col3, 3, y) << 24 );
	
	    Cone = (DITHER16TO8(col0, 0, y) ) |
		    (DITHER16TO8(col0, 1, y) << 8 ) |
		    (DITHER16TO8(col2, 2, y) << 16 ) |
		    (DITHER16TO8(col2, 3, y) << 24 );

	    Cxor = Czero ^ Cone;

	    /* turn bitmask into byte mask */
            dwBytes = ExpansionTable[mask & 0x0f];

	    /* select colours and write to dest */
	    *( (DWORD UNALIGNED HUGE *)row) = (dwBytes & Cxor) ^ Czero;

	    row += bytesPerRow;
	    mask >>= 4;

	}

	/* second two rows  - bottom two subcells */

	/* read last four colours  */
	col0 = lpScale[(*cmp++) & 0x7fff];
	col1 = lpScale[(*cmp++) & 0x7fff];
	col2 = lpScale[(*cmp++) & 0x7fff];
	col3 = lpScale[(*cmp++) & 0x7fff];


	for (; y < 4; y++) {

	    /* dithering requires that we make different
	     * colour masks depending on x and y position - and
	     * therefore re-do it each row
	     */
	    Czero = (DITHER16TO8(col1, 0, y) ) |
		    (DITHER16TO8(col1, 1, y) << 8 ) |
		    (DITHER16TO8(col3, 2, y) << 16 ) |
		    (DITHER16TO8(col3, 3, y) << 24 );
	
	    Cone = (DITHER16TO8(col0, 0, y) ) |
		    (DITHER16TO8(col0, 1, y) << 8 ) |
		    (DITHER16TO8(col2, 2, y) << 16 ) |
		    (DITHER16TO8(col2, 3, y) << 24 );

	    Cxor = Czero ^ Cone;

	    /* turn bitmask into byte mask */
            dwBytes = ExpansionTable[mask & 0x0f];

	    /* select both colours and write to dest */
	    *( (DWORD UNALIGNED HUGE *)row) = (dwBytes & Cxor) ^ Czero;

	    row += bytesPerRow;
	    mask >>= 4;

	}
    } else {
    	// not an edge with only 1 colour pair and one large block


	/* read colours */
	col0 = lpScale[(*cmp++) & 0x7fff];
	col1 = lpScale[(*cmp++) & 0x7fff];

	row = uncmp;

	for (y = 0; y < 4; y++) {

	    Czero = (DITHER16TO8(col1, 0, y) ) |
		    (DITHER16TO8(col1, 1, y) << 8 ) |
		    (DITHER16TO8(col1, 2, y) << 16 ) |
		    (DITHER16TO8(col1, 3, y) << 24 );
	
	    Cone =  (DITHER16TO8(col0, 0, y) ) |
		    (DITHER16TO8(col0, 1, y) << 8 ) |
		    (DITHER16TO8(col0, 2, y) << 16 ) |
		    (DITHER16TO8(col0, 3, y) << 24 );

	    Cxor = Czero ^ Cone;

            /* turn bitmask into byte mask */
            dwBytes = ExpansionTable[mask & 0x0f];

            /* select both colours and write to dest */
            *( (DWORD UNALIGNED HUGE *)row) = (dwBytes & Cxor) ^ Czero;

            row += bytesPerRow;
	    mask >>= 4;

	}
    }
    return( cmp );

}

DWORD FAR PASCAL DecompressFrame16To8C(LPBITMAPINFOHEADER lpbiIn,  LPVOID lpIn,
                    LPBITMAPINFOHEADER lpbiOut, LPVOID lpOut, LONG x, LONG y)
{
HPWORD  cmp = (HPWORD)lpIn;
INT     bix;
INT     biy;
HPBYTE  blockRow;
HPBYTE  blockColumn;
LONG	SkipCount = 0;
INT     bytesPerRow;

    DPF(("DecompressFrame16To8C:\n"));
    /* init dither table pointers. lpDitherTable is inited in msvidc. */
    lpScale = lpDitherTable;
    lpLookup = (LPBYTE) &lpScale[32768];

    bix = (UINT)(lpbiIn->biWidth) / (WIDTH_CBLOCK);   // No negative values in
    biy = (UINT)(lpbiIn->biHeight) / (HEIGHT_CBLOCK); // width or height fields

    StartCounting();

    blockRow = DibXY(lpbiOut, lpOut, x, y, &bytesPerRow);

    for( y=biy; y--; blockRow += bytesPerRow * HEIGHT_CBLOCK )
    {
        blockColumn = blockRow;
        for( x=bix; x--; blockColumn += WIDTH_CBLOCK)
        {
            cmp = DecompressCBlock16To8(blockColumn, cmp, bytesPerRow, &SkipCount);

            // See if the SkipCount has been set.  If so we want to move to
            // the next location rather than calling DecompressCBlock every
            // time around the loop.  Keep the test simple to minimise the
            // overhead on every iteration that the Skipcount is 0.
            if (SkipCount) {

                if ((x -= SkipCount) <0) { // extends past this row
                    LONG SkipRows;

                    // More than just the remainder of this row to skip
                    SkipCount =-x;  // These bits are on the next row(s)
                    // SkipCount will be >0 otherwise we would have gone
                    // down the else leg.

                    // Calculate how many complete and partial rows to skip.
                    // We know we have skipped at least one row.  The plan
                    // is to restart the X loop at some point along the row.
                    // If the skipcount takes us exactly to the end of a row
                    // we drop out of the x loop, and let the outer y loop do
                    // the decrement.  This takes care of the case when the
                    // skipcount takes us to the very end of the image.

                    SkipRows = 1 + (SkipCount-1)/bix;

                    // Decrement the row count and set new blockrow start

#ifdef _WIN32
                    if (y<SkipRows) {
                        Assert(y >= SkipRows);
                        SkipRows = y;
                    }
#endif

                    // Unless we have finished we need to reset blockRow
                    y -= SkipRows;
                    // y might be 0, but we must still complete the last row
                    blockRow += bytesPerRow*HEIGHT_CBLOCK*SkipRows;

                    // Calculate the offset into the next row we will process
                    x = SkipCount%bix;  // This may be 0

                    if (x) {

                        // Set block column by the amount along the row
                        // this iteration is starting, making allowance for
                        // the "for x..." loop iterating blockColumn once.
                        blockColumn = blockRow + ((x-1)*WIDTH_CBLOCK);

                        x=bix-x;  // Get the counter correct
                    }

                    SkipCount = 0; // Skip count now exhausted (so am I)

                } else {
                    // SkipCount has been exhausted by this row
                    // Either the row has completed, or there is more data
                    // on this row.   Check...
                    if (x) {
                        // More of this row left
                        // Worry about moving blockColumn on the right amount
                        blockColumn += WIDTH_CBLOCK*SkipCount;
                    } // else x==0 and we will drop out of the "for x..." loop
                      // blockColumn will be reset when we reenter the x loop
                    SkipCount=0;
                }
            }
        }
    }
    EndCounting("Decompress Frame16To8C took");

    return 0;
}

/* -- 16-bit decompress to 8-bit X2 -----------------------------------*/

/*
 * given a 16-bit CRAM input stream, decompress and dither to 8
 * bits and stretch by 2 in both dimensions (ie draw each pixel 4 times).
 */

/*
 * decompress one 16bpp block, and dither to 8 bpp using table dither method.
 * write each pixel 4 times to stretch X 2.
 *
 * parameters:
 *   uncmp-     pointer to de-compressed buffer for this block.
 *   cmp -      pointer to compressed data for this block
 *   bytes.. -  size of one row of de-compressed data
 *   pSkipCount - skip count held in parent stack frame
 *
 * returns:
 *   pointer to the next block of compressed data.
 *
 */

STATICFN HPWORD INLINE
DecompressCBlock16To8X2(
    HPBYTE uncmp,
    HPWORD cmp,
    INT bytesPerRow,
    LONG * pSkipCount
)
{
    UINT    x, y;
    WORD    mask;
    WORD    col0, col1, col2, col3;
    HPBYTE  row, col;
    DWORD Czero;

    // check for outstanding skips

    if (*pSkipCount > 0)
    {
        Assert(!"Skip count should be handled by caller");
        (*pSkipCount)--;
        return cmp;
    }

    // get mask and init bit mask
    mask = *cmp++;

    // check for a skip or a solid color

    if (mask & 0x8000)
    {
        if ((mask & ~SKIP_MASK) == SKIP_MAGIC)
        {
            *pSkipCount = (mask & SKIP_MASK);

#ifdef _WIN32
            Assert(*pSkipCount != 0);  // break (on debug builds) if SkipCount == 0
#endif

            (*pSkipCount)--;
            return cmp;
        }
        else /* must be solid colour */
        {

	    /* solid colour is lower 15 bits of mask */
            col0 = lpScale[mask & 0x7fff];


            for(row = uncmp, y = 0; y < HEIGHT_CBLOCK*2; y++, row+= bytesPerRow) {

		/* convert colour once for each row */
		Czero = (DITHER16TO8(col0, 0, (y&3)) ) |
			(DITHER16TO8(col0, 1, (y&3)) << 8 ) |
			(DITHER16TO8(col0, 2, (y&3)) << 16 ) |
			(DITHER16TO8(col0, 3, (y&3)) << 24 );

                *(DWORD UNALIGNED HUGE *)row = Czero;
                *((DWORD UNALIGNED HUGE *)row + 1) = Czero;

            }

            return cmp;
        }
    }


    /* in 16-bit CRAM, both 4-pair and 1-pair cells have bit 15 of mask set
     * to zero. We distinguish between them based on bit 15 of the first
     * colour. if this is set, this is the 4-pair edge case cell.
     */
    if (*cmp & 0x8000) {
        // this is an edge with 4 colour pairs in four small blocks

	row = uncmp;

	/* first two rows  - top two subcells */
	for (y = 0; y < HEIGHT_CBLOCK*2; y += 2) {

	    /* read colours at start, and again half-way through */
    	    if ((y == 0) || (y == HEIGHT_CBLOCK)) {

		col0 = lpScale[(*cmp++) & 0x7fff];
		col1 = lpScale[(*cmp++) & 0x7fff];
		col2 = lpScale[(*cmp++) & 0x7fff];
		col3 = lpScale[(*cmp++) & 0x7fff];

	    }

	    col = row;

	    /* first two pixels (first subcell) */
	    for (x = 0; x < WIDTH_CBLOCK; x += 2) {
		if (mask & 1) {
	    	    *col = DITHER16TO8(col0, (x & 3), (y&3));
		    *(col + bytesPerRow) =
			DITHER16TO8(col0, (x&3), ((y+1) & 3));


		    col++;

	    	    *col = DITHER16TO8(col0, ((x+1)&3), ((y)&3));
	    	    *(col + bytesPerRow) =
			DITHER16TO8(col0, ((x+1)&3), ((y+1)&3));

		} else {
	    	    *col = DITHER16TO8(col1, (x & 3), (y&3));
		    *(col + bytesPerRow) =
			DITHER16TO8(col1, (x&3), ((y+1) & 3));

		    col++;

	    	    *col = DITHER16TO8(col1, ((x+1)&3), ((y)&3));
	    	    *(col + bytesPerRow) =
			DITHER16TO8(col1, ((x+1)&3), ((y+1)&3));
		}
		col++;
		mask >>= 1;
	    }

	    /* second two pixels (second subcell) */
	    for (; x < WIDTH_CBLOCK*2; x += 2) {
		if (mask & 1) {
	    	    *col = DITHER16TO8(col2, (x & 3), (y&3));
		    *(col + bytesPerRow) =
			DITHER16TO8(col2, (x&3), ((y+1) & 3));

                    col++;

	    	    *col = DITHER16TO8(col2, ((x+1)&3), ((y)&3));
	    	    *(col + bytesPerRow) =
			DITHER16TO8(col2, ((x+1)&3), ((y+1)&3));
		} else {
	    	    *col = DITHER16TO8(col3, (x & 3), (y&3));
		    *(col + bytesPerRow) =
			DITHER16TO8(col3, (x&3), ((y+1) & 3));

                    col++;
	    	    *col = DITHER16TO8(col3, ((x+1)&3), ((y)&3));
	    	    *(col + bytesPerRow) =
			DITHER16TO8(col3, ((x+1)&3), ((y+1)&3));
		}
		col++;
		mask >>= 1;
	    }
	    row += bytesPerRow * 2;
    	}

    } else {
    	// not an edge with only 1 colour pair and one large block


	/* read colours */
	col0 = lpScale[(*cmp++) & 0x7fff];
	col1 = lpScale[(*cmp++) & 0x7fff];

	row = uncmp;

	for (y = 0; y < HEIGHT_CBLOCK*2; y += 2) {

	    col = row;
	    for (x = 0; x < WIDTH_CBLOCK*2; x += 2) {
		if (mask & 1) {
	    	    *col = DITHER16TO8(col0, (x & 3), (y&3));
		    *(col + bytesPerRow) =
			DITHER16TO8(col0, (x&3), ((y+1) & 3));

                    col++;
	    	    *col = DITHER16TO8(col0, ((x+1)&3), ((y)&3));
	    	    *(col + bytesPerRow) =
			DITHER16TO8(col0, ((x+1)&3), ((y+1)&3));

		} else {
	    	    *col = DITHER16TO8(col1, (x & 3), (y&3));
		    *(col + bytesPerRow) =
			DITHER16TO8(col1, (x&3), ((y+1) & 3));

		    col++;

	    	    *col = DITHER16TO8(col1, ((x+1)&3), ((y)&3));
	    	    *(col + bytesPerRow) =
			DITHER16TO8(col1, ((x+1)&3), ((y+1)&3));
		}
		col++;
		mask >>= 1;
	    }
	    row += bytesPerRow * 2;
	}
    }
    return( cmp );

}


DWORD FAR PASCAL DecompressFrame16To8X2C(LPBITMAPINFOHEADER lpbiIn,  LPVOID lpIn,
                    LPBITMAPINFOHEADER lpbiOut, LPVOID lpOut, LONG x, LONG y)
{
HPWORD  cmp = (HPWORD)lpIn;
INT     bix;
INT     biy;
HPBYTE  blockRow;
HPBYTE  blockColumn;
LONG	SkipCount = 0;
INT     bytesPerRow;

    DPF(("DecompressFrame16To8X2C:\n"));
    /* init dither table pointers. lpDitherTable is inited in msvidc. */
    lpScale = lpDitherTable;
    lpLookup = (LPBYTE) &lpScale[32768];

    StartCounting();

    bix = (UINT)(lpbiIn->biWidth) / (WIDTH_CBLOCK);   // No negative values in
    biy = (UINT)(lpbiIn->biHeight) / (HEIGHT_CBLOCK); // width or height fields

    blockRow = DibXY(lpbiOut, lpOut, x, y, &bytesPerRow);

    for( y=biy; y--; blockRow += bytesPerRow * HEIGHT_CBLOCK *2 )
    {
        blockColumn = blockRow;
        for( x=bix; x--; blockColumn += WIDTH_CBLOCK*2)
        {
            cmp = DecompressCBlock16To8X2(blockColumn, cmp, bytesPerRow, &SkipCount);

            // See if the SkipCount has been set.  If so we want to move to
            // the next location rather than calling DecompressCBlock every
            // time around the loop.  Keep the test simple to minimise the
            // overhead on every iteration that the Skipcount is 0.
            if (SkipCount) {

                if ((x -= SkipCount) <0) { // extends past this row
                    LONG SkipRows;

                    // More than just the remainder of this row to skip
                    SkipCount =-x;  // These bits are on the next row(s)
                    // SkipCount will be >0 otherwise we would have gone
                    // down the else leg.

                    // Calculate how many complete and partial rows to skip.
                    // We know we have skipped at least one row.  The plan
                    // is to restart the X loop at some point along the row.
                    // If the skipcount takes us exactly to the end of a row
                    // we drop out of the x loop, and let the outer y loop do
                    // the decrement.  This takes care of the case when the
                    // skipcount takes us to the very end of the image.

                    SkipRows = 1 + (SkipCount-1)/bix;

                    // Decrement the row count and set new blockrow start

#ifdef _WIN32
                    if (y<SkipRows) {
                        Assert(y >= SkipRows);
                        SkipRows = y;
                    }
#endif

                    // Unless we have finished we need to reset blockRow
                    y -= SkipRows;
                    // y might be 0, but we must still complete the last row
                    blockRow += bytesPerRow*HEIGHT_CBLOCK*2*SkipRows;

                    // Calculate the offset into the next row we will process
                    x = SkipCount%bix;  // This may be 0

                    if (x) {

                        // Set block column by the amount along the row
                        // this iteration is starting, making allowance for
                        // the "for x..." loop iterating blockColumn once.
                        blockColumn = blockRow + ((x-1)*WIDTH_CBLOCK*2);

                        x=bix-x;  // Get the counter correct
                    }

                    SkipCount = 0; // Skip count now exhausted (so am I)

                } else {
                    // SkipCount has been exhausted by this row
                    // Either the row has completed, or there is more data
                    // on this row.   Check...
                    if (x) {
                        // More of this row left
                        // Worry about moving blockColumn on the right amount
                        blockColumn += WIDTH_CBLOCK*2*SkipCount;
                    } // else x==0 and we will drop out of the "for x..." loop
                      // blockColumn will be reset when we reenter the x loop
                    SkipCount=0;
                }
            }
        }
    }
    EndCounting("Decompress Frame16To8x2C took");

    return 0;
}

#endif
