/*
	File:		LHApplication.h

	Contains:	

	Written by:	U. J. Krabbenhoeft

	Copyright:	© 1993-1997 by Heidelberger Druckmaschinen AG, all rights reserved.

*/
#ifndef LHApplication_h
#define LHApplication_h

#ifdef __cplusplus
extern "C" {
#endif

#if PRAGMA_ALIGN_SUPPORTED
#pragma options align=mac68k
#endif

#if PRAGMA_IMPORT_SUPPORTED
#pragma import on
#endif

#define MAX_ELEMENT_PER_PIXEL   17 /* 16 Colors + Alpha */
typedef struct LH_CMBitmapPlane 
{
	char *						image[MAX_ELEMENT_PER_PIXEL];	/* used for plane-interleaved data		*/
	long 						width;							/* count of pixel in a line				*/
	long 						height;							/* line count							*/
	long 						rowBytes;						/* Offset to next line					*/
	long 						elementOffset;					/* Offset to next element in a plane	*/
	long 						pixelSize;						/* not used								*/
	CMBitmapColorSpace			space;							/* color space							*/
	long 						user1;							/* not used								*/
	long						user2;							/* not used								*/
} LH_CMBitmapPlane;

/* example:
	convert CMYK plane interleaved (InPtr) to BGR pixel interleaved (OutPtr)
	InBitMap.width = OutBitMap.width = 200;
	InBitMap.height = OutBitMap.height = 100;
	InBitMap.rowBytes = 200;
	InBitMap.elementOffset = 1;
	InBitMap.space = cmCMYK32Space;
	InBitMap.image[0] = InPtr+InBitMap.rowBytes*0;
	InBitMap.image[1] = InPtr+InBitMap.rowBytes*1;
	InBitMap.image[2] = InPtr+InBitMap.rowBytes*2;
	InBitMap.image[3] = InPtr+InBitMap.rowBytes*3;
	OutBitMap.rowBytes = 200 * 3;
	OutBitMap.elementOffset = 3;
	OutBitMap.space = cmRGB24Space;
 	OutBitMap.image[0] = OutPtr+2;
	OutBitMap.image[1] = OutPtr+1;
	OutBitMap.image[2] = OutPtr+0;
	
	Only 8 bit or 16 bit data allowed.
*/
#if PRAGMA_IMPORT_SUPPORTED
#pragma import off
#endif

#if PRAGMA_ALIGN_SUPPORTED
#pragma options align=reset
#endif

#ifdef __cplusplus
}
#endif
#endif

