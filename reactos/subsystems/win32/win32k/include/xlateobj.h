 /*
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS kernel
 * PURPOSE:           XLATEOBJ structures and functions
 * FILE:              subsystem/win32/win32k/eng/objects.h
 * PROGRAMER:         Timo Kreuzer
 *
 */

#include <include/palette.h>

struct _EXLATEOBJ;

typedef ULONG (FASTCALL *PFN_XLATE)(struct _EXLATEOBJ *pexlo, ULONG iColor);

typedef struct _EXLATEOBJ
{
    XLATEOBJ xlo;

    PFN_XLATE pfnXlate;

    PPALETTE ppalSrc;
    PPALETTE ppalDst;
    PPALETTE ppalDstDc;

    HANDLE hColorTransform;

    union
    {
        ULONG aulXlate[6];
        struct
        {
            ULONG ulRedMask;
            ULONG ulGreenMask;
            ULONG ulBlueMask;
            ULONG ulRedShift;
            ULONG ulGreenShift;
            ULONG ulBlueShift;
        };
    };
} EXLATEOBJ, *PEXLATEOBJ;

void
DbgCmpXlate(XLATEOBJ *pxlo1, XLATEOBJ *pxlo2);

VOID NTAPI EXLATEOBJ_vInitialize(PEXLATEOBJ pexlo, PALETTE *ppalSrc, PALETTE *ppalDst, ULONG, ULONG, ULONG);
VOID NTAPI EXLATEOBJ_vInitXlateFromDCs(PEXLATEOBJ pexlo, PDC pdcSrc, PDC pdcDst);
VOID NTAPI EXLATEOBJ_vInitBrushXlate(PEXLATEOBJ pexlo, BRUSH *pbrush, SURFACE *psurf, COLORREF crForegroundClr, COLORREF crBackgroundClr);
VOID NTAPI EXLATEOBJ_vInitSrcMonoXlate(PEXLATEOBJ pexlo, PPALETTE ppalDst, ULONG Color0, ULONG Color1);
VOID NTAPI EXLATEOBJ_vCleanup(PEXLATEOBJ pexlo);

//#define XLATEOBJ_iXlate(pxo, Color) ((EXLATEOBJ*)pxo)->pfnXlate(pxo, Color)
