/*----------------------------------------------------------------------+
| compress.h - Microsoft Video 1 Compressor - compress header file	|
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

/*******************************************************************

    encoding a skip - if the mask (first word) has the high bit set
    then it is either a skip cell code, or a solid color code.

    you can't encode a r=01 solid color, this is how we tell a skip
    from a solid (not a big loss)

*******************************************************************/
#define SKIP_MAX    SKIP_MASK
#define SKIP_MASK   ((WORD) (((1<<10)-1)))
#define MAGIC_MASK  ~SKIP_MASK
#define SKIP_MAGIC  0x8400          // r=01
#define SOLID_MAGIC 0x8000
#define MASK_MAGIC  0xA000

/*******************************************************************
*******************************************************************/

extern DWORD   numberOfBlocks;
extern DWORD   numberOfSolids;
extern DWORD   numberOfSolid4;
extern DWORD   numberOfEdges;
extern DWORD   numberOfSkips;
extern DWORD   numberOfSkipCodes;

/*******************************************************************
*******************************************************************/

//
//  this is a CELL (4x4) array of RGBQUADs
//
typedef RGBQUAD CELL[HEIGHT_CBLOCK * WIDTH_CBLOCK];
typedef RGBQUAD *PCELL;

typedef struct _CELLS {
    CELL cell;
    CELL cellT;
    CELL cellPrev;
} CELLS;
typedef CELLS * PCELLS;

/*******************************************************************
routine: CompressFrame
purp:   compress a frame
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
 			  LPARAM lParam,
                          PCELLS pCells);

DWORD FAR CompressFrame8(LPBITMAPINFOHEADER  lpbi,           // DIB header to compress
                         LPVOID              lpBits,         // DIB bits to compress
                         LPVOID              lpData,         // put compressed data here
                         DWORD               threshold,      // edge threshold
                         DWORD               thresholdInter, // inter-frame threshold
                         LPBITMAPINFOHEADER  lpbiPrev,       // previous frame
                         LPVOID              lpPrev,         // previous frame
			 LONG (CALLBACK *Status) (LPARAM lParam, UINT message, LONG l),
			 LPARAM lParam,
                         PCELLS pCells,
			 LPBYTE lpITable,
			 RGBQUAD *prgbqOut);


DWORD FAR CompressFrameBegin(LPBITMAPINFOHEADER lpbiIn, LPBITMAPINFOHEADER lpbiOut,
			     LPBYTE *lplpITable, RGBQUAD *prgbIn);
DWORD FAR CompressFrameEnd(LPBYTE *lplpITable);

void FAR CompressFrameFree(void);

DWORD FAR QualityToThreshold(DWORD dwQuality);

