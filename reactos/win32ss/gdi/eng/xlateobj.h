 /*
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS kernel
 * PURPOSE:           XLATEOBJ structures and functions
 * FILE:              subsystem/win32/win32k/eng/objects.h
 * PROGRAMER:         Timo Kreuzer
 *
 */

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

PFN_XLATE
FORCEINLINE
XLATEOBJ_pfnXlate(XLATEOBJ *pxlo)
{
    return ((PEXLATEOBJ)pxlo)->pfnXlate;
}

extern EXLATEOBJ gexloTrivial;

ULONG
FASTCALL
EXLATEOBJ_iXlateTrivial(PEXLATEOBJ pexlo, ULONG iColor);

void
DbgCmpXlate(XLATEOBJ *pxlo1, XLATEOBJ *pxlo2);

VOID NTAPI EXLATEOBJ_vInitialize(PEXLATEOBJ pexlo, PPALETTE ppalSrc, PPALETTE ppalDst, ULONG, ULONG, ULONG);
VOID NTAPI EXLATEOBJ_vInitXlateFromDCs(PEXLATEOBJ pexlo, PDC pdcSrc, PDC pdcDst);
VOID NTAPI EXLATEOBJ_vInitSrcMonoXlate(PEXLATEOBJ pexlo, PPALETTE ppalDst, ULONG Color0, ULONG Color1);
VOID NTAPI EXLATEOBJ_vCleanup(PEXLATEOBJ pexlo);
