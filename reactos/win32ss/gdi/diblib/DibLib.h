
#ifdef _MSC_VER
#pragma warning(disable:4711)
#endif

#define FASTCALL __fastcall

#include <stdarg.h>
#include <windef.h>
#include <wingdi.h>
#include <winddi.h>

#ifdef _OPTIMIZE_DIBLIB
#ifdef _MSC_VER
#pragma optimize("g", on)
#else
#pragma GCC optimize("O3")
#endif
#endif

typedef
ULONG
(FASTCALL *PFN_XLATE)(XLATEOBJ* pxlo, ULONG ulColor);

extern const BYTE ajShift4[2];

#include "DibLib_interface.h"

#define _DibXlate(pBltData, ulColor) (pBltData->pfnXlate(pBltData->pxlo, ulColor))

#define __PASTE_(s1,s2) s1##s2
#define __PASTE(s1,s2) __PASTE_(s1,s2)

#define __DIB_FUNCTION_NAME_SRCDST2(name, src_bpp, dst_bpp) Dib_ ## name ## _S ## src_bpp ## _D ## dst_bpp
#define __DIB_FUNCTION_NAME_SRCDST(name, src_bpp, dst_bpp) __DIB_FUNCTION_NAME_SRCDST2(name, src_bpp, dst_bpp)

#define __DIB_FUNCTION_NAME_DST2(name, dst_bpp) Dib_ ## name ## _D ## dst_bpp
#define __DIB_FUNCTION_NAME_DST(name, src_bpp, dst_bpp) __DIB_FUNCTION_NAME_DST2(name, dst_bpp)
#define __DIB_FUNCTION_NAME_SRCDSTEQ(name, src_bpp, dst_bpp) __PASTE(__DIB_FUNCTION_NAME_SRCDST2(name, src_bpp, dst_bpp), _EqSurf)
#define __DIB_FUNCTION_NAME_SRCDSTEQL2R(name, src_bpp, dst_bpp) __PASTE(__DIB_FUNCTION_NAME_SRCDST2(name, src_bpp, dst_bpp), _EqSurfL2R)
#define __DIB_FUNCTION_NAME_SRCDSTEQR2L(name, src_bpp, dst_bpp) __PASTE(__DIB_FUNCTION_NAME_SRCDST2(name, src_bpp, dst_bpp), _EqSurfR2L)

#define _ReadPixel_1(pjSource, jShift) (((*(pjSource)) >> (jShift)) & 1)
#define _WritePixel_1(pjDest, jShift, ulColor) (void)(*(pjDest) = (UCHAR)((*(pjDest) & ~(1<<(jShift))) | ((ulColor)<<(jShift))))
#define _NextPixel_1(ppj, pjShift)    (void)(((*(pjShift))--), (*(pjShift) &= 7), (*(ppj) += (*(pjShift) == 7)))
#define _NextPixelR2L_1(ppj, pjShift) (void)(((*(pjShift))++), (*(pjShift) &= 7), (*(ppj) -= (*(pjShift) == 0)))
#define _SHIFT_1(x) x
#define _CALCSHIFT_1(pShift, x) (void)(*(pShift) = (7 - ((x) & 7)))

#define _ReadPixel_4(pjSource, jShift) (((*(pjSource)) >> (jShift)) & 15)
#define _WritePixel_4(pjDest, jShift, ulColor) (void)(*(pjDest) = (UCHAR)((*(pjDest) & ~(15<<(jShift))) | ((ulColor)<<(jShift))))
#define _NextPixel_4(ppj, pjShift) (void)((*(ppj) += (*(pjShift) & 1)), (*(pjShift)) -= 4, *(pjShift) &= 7)
#define _NextPixelR2L_4(ppj, pjShift) (void)((*(pjShift)) -= 4, *(pjShift) &= 7, (*(ppj) -= (*(pjShift) & 1)))
#define _SHIFT_4(x) x
#define _CALCSHIFT_4(pShift, x) (void)(*(pShift) = ajShift4[(x) & 1])

#define _ReadPixel_8(pjSource, x)  (*(UCHAR*)(pjSource))
#define _WritePixel_8(pjDest, x, ulColor) (void)(*(UCHAR*)(pjDest) = (UCHAR)(ulColor))
#define _NextPixel_8(ppj, pjShift) (void)(*(ppj) += 1)
#define _NextPixelR2L_8(ppj, pjShift) (void)(*(ppj) -= 1)
#define _SHIFT_8(x)
#define _CALCSHIFT_8(pShift, x)

#define _ReadPixel_16(pjSource, x)  (*(USHORT*)(pjSource))
#define _WritePixel_16(pjDest, x, ulColor) (void)(*(USHORT*)(pjDest) = (USHORT)(ulColor))
#define _NextPixel_16(ppj, pjShift) (void)(*(ppj) += 2)
#define _NextPixelR2L_16(ppj, pjShift) (void)(*(ppj) -= 2)
#define _SHIFT_16(x)
#define _CALCSHIFT_16(pShift, x)

#define _ReadPixel_24(pjSource, x)  ((pjSource)[0] | ((pjSource)[1] << 8) | ((pjSource)[2] << 16))
#define _WritePixel_24(pjDest, x, ulColor) (void)(((pjDest)[0] = ((ulColor)&0xFF)),((pjDest)[1] = (((ulColor)>>8)&0xFF)),((pjDest)[2] = (((ulColor)>>16)&0xFF)))
#define _NextPixel_24(ppj, pjShift) (void)(*(ppj) += 3)
#define _NextPixelR2L_24(ppj, pjShift) (void)(*(ppj) -= 3)
#define _SHIFT_24(x)
#define _CALCSHIFT_24(pShift, x)

#define _ReadPixel_32(pjSource, x)  (*(ULONG*)(pjSource))
#define _WritePixel_32(pjDest, x, ulColor) (void)(*(ULONG*)(pjDest) = (ulColor))
#define _NextPixel_32(ppj, pjShift) (void)(*(ppj) += 4)
#define _NextPixelR2L_32(ppj, pjShift) (void)(*(ppj) -= 4)
#define _SHIFT_32(x)
#define _CALCSHIFT_32(pShift, x)

