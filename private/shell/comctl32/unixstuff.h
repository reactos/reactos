#ifndef _COMCTL32_UNIXSTUFF_H_
#define _COMCTL32_UNIXSTUFF_H_

#define ARROW_WIDTH  4
#define ARROW_HEIGHT 4

#define CHECK_FREE( ptr ) { if(ptr) { free(ptr); ptr = NULL; } }

STDAPI_(void) UnixPaintArrow(HDC hDC, BOOL bHoriz, BOOL bDown, int nXCenter, int nYCenter, int nWidth, int nHeight);

/* We need this for the unaligned template classes to work after
 * fixing the mmsystem.h header.
 */
#if defined(MWBIG_ENDIAN)
#ifdef mmioFOURCC
#undef mmioFOURCC
#endif
#define mmioFOURCC(ch0, ch1, ch2, ch3) MAKEFOURCC(ch3, ch2, ch1, ch0)
#endif

#endif 
