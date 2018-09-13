/****************************************************************************
 *
 *   dibmap.h
 *
 *   Microsoft Video for Windows Sample Capture Class
 *
 *   Copyright (c) 1992-1994 Microsoft Corporation.  All Rights Reserved.
 *
 *    You have a royalty-free right to use, modify, reproduce and
 *    distribute the Sample Files (and/or any modified version) in
 *    any way you find useful, provided that you agree that
 *    Microsoft has no warranty obligations or liability for any
 *    Sample Application Files which are modified.
 *
 ***************************************************************************/

#ifndef _LPHISTOGRAM_DEFINED
#define _LPHISTOGRAM_DEFINED
typedef DWORD HUGE * LPHISTOGRAM;
#endif

#define RGB16(r,g,b) (\
            (((WORD)(r) >> 3) << 10) |  \
            (((WORD)(g) >> 3) << 5)  |  \
            (((WORD)(b) >> 3) << 0)  )

LPHISTOGRAM     InitHistogram(LPHISTOGRAM lpHistogram);
void            FreeHistogram(LPHISTOGRAM lpHistogram);
HPALETTE        HistogramPalette(LPHISTOGRAM lpHistogram, LPBYTE lp16to8, int nColors);
BOOL            DibHistogram(LPBITMAPINFOHEADER lpbi, LPBYTE lpBits, int x, int y, int dx, int dy, LPHISTOGRAM lpHistogram);
HANDLE          DibReduce(LPBITMAPINFOHEADER lpbi, LPBYTE lpBits, HPALETTE hpal, LPBYTE lp16to8);
