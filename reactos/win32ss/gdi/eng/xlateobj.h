 /*
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS kernel
 * PURPOSE:           XLATEOBJ structures and functions
 * FILE:              win32ss/gdi/eng/xlateobj.h
 * PROGRAMER:         Timo Kreuzer
 *
 */

struct _EXLATEOBJ;

_Function_class_(FN_XLATE)
typedef
ULONG
(FASTCALL *PFN_XLATE)(
    _In_ struct _EXLATEOBJ *pexlo,
    _In_ ULONG iColor);

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

extern EXLATEOBJ gexloTrivial;

_Notnull_
FORCEINLINE
PFN_XLATE
XLATEOBJ_pfnXlate(
    _In_ XLATEOBJ *pxlo)
{
    return ((PEXLATEOBJ)pxlo)->pfnXlate;
}

VOID
NTAPI
EXLATEOBJ_vInitialize(
    _Out_ PEXLATEOBJ pexlo,
    _In_opt_ PPALETTE ppalSrc,
    _In_opt_ PPALETTE ppalDst,
    _In_ COLORREF crSrcBackColor,
    _In_ COLORREF crDstBackColor,
    _In_ COLORREF crDstForeColor);

VOID
NTAPI
EXLATEOBJ_vInitXlateFromDCs(
    _Out_ PEXLATEOBJ pexlo,
    _In_ PDC pdcSrc,
    _In_ PDC pdcDst);

VOID
NTAPI
EXLATEOBJ_vInitSrcMonoXlate(
    _Out_ PEXLATEOBJ pexlo,
    _In_ PPALETTE ppalDst,
    _In_ COLORREF crBackgroundClr,
    _In_ COLORREF crForegroundClr);

VOID
NTAPI
EXLATEOBJ_vCleanup(
    _Inout_ PEXLATEOBJ pexlo);

