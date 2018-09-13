/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    foncache.c

Abstract:

    This is the console fullscreen driver for the VGA card.

Environment:

    kernel mode only

Notes:

Revision History:

--*/

#include "stdarg.h"
#include "stdio.h"
#include "ntddk.h"
#include "fsvga.h"
#include "fsvgalog.h"



#define ADD_IMAGE     1
#define REPLACE_IMAGE 2


#define CALC_BITMAP_BITS_FOR_X( FontSizeX, dwAlign ) \
    ( ( ( FontSizeX * BITMAP_BITS_PIXEL + (dwAlign-1) ) & ~(dwAlign-1)) >> BITMAP_ARRAY_BYTE )



DWORD
CalcBitmapBufferSize(
    IN COORD FontSize,
    IN DWORD dwAlign
    )
{
    DWORD uiCount;

    uiCount = CALC_BITMAP_BITS_FOR_X(FontSize.X,
                                     (dwAlign==BYTE_ALIGN ? BITMAP_BITS_BYTE_ALIGN : BITMAP_BITS_WORD_ALIGN));
    uiCount = uiCount * BITMAP_PLANES * FontSize.Y;
    return uiCount;
}


VOID
AlignCopyMemory(
    OUT PBYTE pDestBits,
    IN DWORD dwDestAlign,
    IN PBYTE pSrcBits,
    IN DWORD dwSrcAlign,
    IN COORD FontSize
    )
{
    DWORD dwDestBufferSize;
    COORD coord;

    try
    {

        if (dwDestAlign == dwSrcAlign) {
            dwDestBufferSize = CalcBitmapBufferSize(FontSize, dwDestAlign);
            RtlCopyMemory(pDestBits, pSrcBits, dwDestBufferSize);
            return;
        }

        switch (dwDestAlign) {
            default:
            case WORD_ALIGN:
                switch (dwSrcAlign) {
                    default:
                    //
                    // pDest = WORD, pSrc = WORD
                    //
                    case WORD_ALIGN:
                        dwDestBufferSize = CalcBitmapBufferSize(FontSize, dwDestAlign);
                        RtlCopyMemory(pDestBits, pSrcBits, dwDestBufferSize);
                        break;
                    //
                    // pDest = WORD, pSrc = BYTE
                    //
                    case BYTE_ALIGN:
                        dwDestBufferSize = CalcBitmapBufferSize(FontSize, dwDestAlign);
                        if (((FontSize.X % BITMAP_BITS_BYTE_ALIGN) == 0) &&
                            ((FontSize.X % BITMAP_BITS_WORD_ALIGN) == 0)   ) {
                            RtlCopyMemory(pDestBits, pSrcBits, dwDestBufferSize);
                        }
                        else {
                            RtlZeroMemory(pDestBits, dwDestBufferSize);
                            for (coord.Y=0; coord.Y < FontSize.Y; coord.Y++) {
                                for (coord.X=0;
                                     coord.X < CALC_BITMAP_BITS_FOR_X(FontSize.X, BITMAP_BITS_BYTE_ALIGN);
                                     coord.X++) {
                                    *pDestBits++ = *pSrcBits++;
                                }
                                if (CALC_BITMAP_BITS_FOR_X(FontSize.X, BITMAP_BITS_BYTE_ALIGN) & 1)
                                    pDestBits++;
                            }
                        }
                        break;
                }
                break;
            case BYTE_ALIGN:
                switch (dwSrcAlign) {
                    //
                    // pDest = BYTE, pSrc = BYTE
                    //
                    case BYTE_ALIGN:
                        dwDestBufferSize = CalcBitmapBufferSize(FontSize, dwDestAlign);
                        RtlCopyMemory(pDestBits, pSrcBits, dwDestBufferSize);
                        break;
                    default:
                    //
                    // pDest = BYTE, pSrc = WORD
                    //
                    case WORD_ALIGN:
                        dwDestBufferSize = CalcBitmapBufferSize(FontSize, dwDestAlign);
                        if (((FontSize.X % BITMAP_BITS_BYTE_ALIGN) == 0) &&
                            ((FontSize.X % BITMAP_BITS_WORD_ALIGN) == 0)   ) {
                            RtlCopyMemory(pDestBits, pSrcBits, dwDestBufferSize);
                        }
                        else {
                            RtlZeroMemory(pDestBits, dwDestBufferSize);
                            for (coord.Y=0; coord.Y < FontSize.Y; coord.Y++) {
                                for (coord.X=0;
                                     coord.X < CALC_BITMAP_BITS_FOR_X(FontSize.X, BITMAP_BITS_BYTE_ALIGN);
                                     coord.X++) {
                                    *pDestBits++ = *pSrcBits++;
                                }
                                if (CALC_BITMAP_BITS_FOR_X(FontSize.X, BITMAP_BITS_BYTE_ALIGN) & 1)
                                    pSrcBits++;
                            }
                        }
                        break;
                }
                break;
        }

    }
    except (EXCEPTION_EXECUTE_HANDLER)
    {
    }
}
