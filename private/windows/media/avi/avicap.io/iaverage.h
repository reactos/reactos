/****************************************************************************
 *
 *   iaverage.h
 *
 *   Image averaging
 *
 *   Copyright (c) 1992-1994 Microsoft Corporation.  All Rights Reserved.
 *
 ***************************************************************************/

#ifndef _INC_AVERAGE
#define _INC_AVERAGE

#ifdef __cplusplus
extern "C" {            /* Assume C declarations for C++ */
#endif  /* __cplusplus */

#ifndef RC_INVOKED
#pragma pack(1)         /* Assume byte packing throughout */
#endif  /* RC_INVOKED */

#ifndef RC_INVOKED

// -------------------------
//  Structures
// -------------------------

typedef struct tagiAverage {
    BITMAPINFO      bi;                 // Copy of source format
    RGBQUAD         pe[256];            // Copy of color table
    LPBYTE          lpInverseMap;       // rgb15 to palette index
    LPWORD          lpRGB;              // accumulator
    WORD            iCount;             // Count of images accumulated
} IAVERAGE, *PIAVERAGE, FAR *LPIAVERAGE;

BOOL iaverageInit   (LPIAVERAGE FAR * lppia, LPBITMAPINFO lpbi, HPALETTE hPal);
BOOL iaverageFini   (LPIAVERAGE lpia);
BOOL iaverageZero   (LPIAVERAGE lpia);
BOOL iaverageSum    (LPIAVERAGE lpia, LPVOID lpBits);
BOOL iaverageDivide (LPIAVERAGE lpia, LPVOID lpBits);
BOOL CrunchDIB(
    LPIAVERAGE lpia,
    LPBITMAPINFOHEADER  lpbiSrc,    // BITMAPINFO of source
    LPVOID              lpSrc,      // input bits to crunch
    LPBITMAPINFOHEADER  lpbiDst,    // BITMAPINFO of dest
    LPVOID              lpDst);     // output bits to crunch

#endif  /* RC_INVOKED */


#ifndef RC_INVOKED
#pragma pack()          /* Revert to default packing */
#endif  /* RC_INVOKED */

#ifdef __cplusplus
}                       /* End of extern "C" { */
#endif    /* __cplusplus */

#endif /* INC_AVERAGE */


