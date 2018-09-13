/*********************************************************************
   Interlace.h

   Header file for interlace module.
 *********************************************************************/
#ifndef  _INTERLACE_H
#define  _INTERLACE_H

#include "Image.h"

#define  NUM_PASSES  7

#pragma pack(2)      /* Apparently, this is reasonably important  */

/* The primary state data structure.           */
typedef struct
{
	IFLCLASS Class;
	long     iImageHeight;
	long     iImageWidth;
	long     cbPixelSize;

	long     cScanBlocks;
	long     cPassScanLines[NUM_PASSES];
	long     cTotalScanLines;
	long     iPass;
	long     iPassLine;
	long     iScanLine;
	long     iImageLine;

} ADAM7_STRUCT, *pADAM7_STRUCT;


//************************************************************************************
// Given an image described by the parameters of the ADAM7_STRUCT, calculate the
// number of scan lines in the image file which has been interlaced using the Adam7 
// scheme.
//************************************************************************************
int iADAM7CalculateNumberOfScanLines(pADAM7_STRUCT ptAdam7);

//************************************************************************************
// Generate a deinterlaced DIB; i.e., each pixel is in BGR in the case
// of RGB/RGBA image classes, and raster line data is stored in a contiguous block.
//************************************************************************************
// The CALLING application is responsible for deallocating the structure created by
// this function.
LPBYTE *ppbADAM7InitDIBPointers(LPBYTE pbDIB, pADAM7_STRUCT ptAdam7, DWORD cbImageLine);

// The following returns TRUE if the scan line was an empty scan line.
BOOL ADAM7AddRowToDIB(LPBYTE *ppbDIBPtrs, LPBYTE pbScanLine, pADAM7_STRUCT ptAdam7);

//************************************************************************************

//************************************************************************************
// Generate a deinterlaced image; i.e., each pixel is in RGB in the case
// of RGB/RGBA image classes, and raster line data may not necessarily be stored 
// in one contiguous block of memory.
//************************************************************************************

// The following returns TRUE if the scan line was an empty scan line.
BOOL ADAM7AddRowToImageBuffer(LPBYTE ppbInmageBuffer[], LPBYTE pbScanLine, pADAM7_STRUCT ptAdam7);
//************************************************************************************

//************************************************************************************
// Generate a deinterlaced alpha channel data block.
//************************************************************************************
BOOL ADAM7RMFDeinterlaceAlpha(LPWORD *ppwInterlaced, LPWORD *ppwDeinterlaced,
                              IFL_ALPHA_CHANNEL_INFO *ptIFLAlphaInfo);

#endif // _INTERLACE_H