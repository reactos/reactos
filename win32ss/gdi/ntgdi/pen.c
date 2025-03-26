/*
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS kernel
 * PURPOSE:           Pen functiona
 * FILE:              win32ss/gdi/ntgdi/pen.c
 * PROGRAMER:
 */

#include <win32k.h>

#define NDEBUG
#include <debug.h>

/* PRIVATE FUNCTIONS **********************************************************/

static
VOID
PEN_vInit(
    PPEN ppen)
{
    /* Start with kmode brush attribute */
    ppen->pBrushAttr = &ppen->BrushAttr;
}

PBRUSH
NTAPI
PEN_AllocPenWithHandle(
    VOID)
{
    PPEN ppen;

    ppen = (PBRUSH)GDIOBJ_AllocObjWithHandle(GDILoObjType_LO_PEN_TYPE, sizeof(PEN));
    if (ppen == NULL)
    {
        return NULL;
    }

    PEN_vInit(ppen);
    return ppen;
}

PBRUSH
NTAPI
PEN_AllocExtPenWithHandle(
    VOID)
{
    PPEN ppen;

    ppen = (PBRUSH)GDIOBJ_AllocObjWithHandle(GDILoObjType_LO_EXTPEN_TYPE, sizeof(PEN));
    if (ppen == NULL)
    {
        return NULL;
    }

    PEN_vInit(ppen);
    return ppen;
}

PBRUSH
FASTCALL
PEN_ShareLockPen(HPEN hobj)
{
    if ((GDI_HANDLE_GET_TYPE(hobj) != GDILoObjType_LO_PEN_TYPE) &&
        (GDI_HANDLE_GET_TYPE(hobj) != GDILoObjType_LO_EXTPEN_TYPE))
    {
        return NULL;
    }

    return (PBRUSH)GDIOBJ_ReferenceObjectByHandle(hobj, GDIObjType_BRUSH_TYPE);
}

HPEN
APIENTRY
IntGdiExtCreatePen(
    DWORD dwPenStyle,
    DWORD dwWidth,
    IN ULONG ulBrushStyle,
    IN ULONG ulColor,
    IN ULONG_PTR ulClientHatch,
    IN ULONG_PTR ulHatch,
    DWORD dwStyleCount,
    PULONG pStyle,
    IN ULONG cjDIB,
    IN BOOL bOldStylePen,
    IN OPTIONAL HBRUSH hbrush)
{
    HPEN hPen;
    PBRUSH pbrushPen;
    static ULONG aulStyleAlternate[] = { 1, 1 };
    static ULONG aulStyleDash[] = { 6, 2 };
    static ULONG aulStyleDot[] = { 1, 1 };
    static ULONG aulStyleDashDot[] = { 3, 2, 1, 2 };
    static ULONG aulStyleDashDotDot[] = { 3, 1, 1, 1, 1, 1 };
    ULONG i;

    dwWidth = abs(dwWidth);

    if ( (dwPenStyle & PS_STYLE_MASK) == PS_NULL)
    {
        return StockObjects[NULL_PEN];
    }

    if (bOldStylePen)
    {
        pbrushPen = PEN_AllocPenWithHandle();
    }
    else
    {
        pbrushPen = PEN_AllocExtPenWithHandle();
    }

    if (!pbrushPen)
    {
        EngSetLastError(ERROR_NOT_ENOUGH_MEMORY);
        DPRINT("Can't allocate pen\n");
        return 0;
    }

    hPen = pbrushPen->BaseObject.hHmgr;

    if (bOldStylePen)
    {
        // If nWidth is zero, the pen is a single pixel wide, regardless of the current transformation.
        if (!dwWidth && (dwPenStyle & PS_STYLE_MASK) != PS_SOLID)
            dwWidth = 1;
    }
    else
    {
        switch (dwPenStyle & PS_ENDCAP_MASK)
        {
            case PS_ENDCAP_ROUND:
            case PS_ENDCAP_SQUARE:
            case PS_ENDCAP_FLAT:
                break;

            default:
                goto ExitCleanup;
        }

        switch (dwPenStyle & PS_JOIN_MASK)
        {
            case PS_JOIN_ROUND:
            case PS_JOIN_BEVEL:
            case PS_JOIN_MITER:
                break;

            default:
                goto ExitCleanup;
        }

        switch (dwPenStyle & PS_TYPE_MASK)
        {
            case PS_COSMETIC:
                if (dwWidth != 1 || ulBrushStyle != BS_SOLID)
                    goto ExitCleanup;

                break;

            case PS_GEOMETRIC:
                break;

            default:
                goto ExitCleanup;
        }
    }

    pbrushPen->lWidth = dwWidth;
    FLOATOBJ_SetLong(&pbrushPen->eWidth, pbrushPen->lWidth);
    pbrushPen->ulPenStyle = dwPenStyle;
    pbrushPen->BrushAttr.lbColor = ulColor;
    pbrushPen->iBrushStyle = ulBrushStyle;
    // FIXME: Copy the bitmap first ?
    pbrushPen->hbmClient = (HANDLE)ulClientHatch;
    pbrushPen->dwStyleCount = 0;
    pbrushPen->pStyle = NULL;
    pbrushPen->ulStyleSize = 0;
    pbrushPen->flAttrs = bOldStylePen ? BR_IS_OLDSTYLEPEN : BR_IS_PEN;

    switch (dwPenStyle & PS_STYLE_MASK)
    {
    case PS_NULL:
        pbrushPen->flAttrs |= BR_IS_NULL;
        break;

    case PS_SOLID:
        pbrushPen->flAttrs |= BR_IS_SOLID;
        break;

    case PS_ALTERNATE:
        pbrushPen->flAttrs |= BR_IS_SOLID | BR_IS_DEFAULTSTYLE;
        pbrushPen->pStyle = aulStyleAlternate;
        pbrushPen->dwStyleCount = _countof(aulStyleAlternate);
        break;

    case PS_DOT:
        pbrushPen->flAttrs |= BR_IS_SOLID | BR_IS_DEFAULTSTYLE;
        pbrushPen->pStyle = aulStyleDot;
        pbrushPen->dwStyleCount = _countof(aulStyleDot);
        break;

    case PS_DASH:
        pbrushPen->flAttrs |= BR_IS_SOLID | BR_IS_DEFAULTSTYLE;
        pbrushPen->pStyle = aulStyleDash;
        pbrushPen->dwStyleCount = _countof(aulStyleDash);
        break;

    case PS_DASHDOT:
        pbrushPen->flAttrs |= BR_IS_SOLID | BR_IS_DEFAULTSTYLE;
        pbrushPen->pStyle = aulStyleDashDot;
        pbrushPen->dwStyleCount = _countof(aulStyleDashDot);
        break;

    case PS_DASHDOTDOT:
        pbrushPen->flAttrs |= BR_IS_SOLID | BR_IS_DEFAULTSTYLE;
        pbrushPen->pStyle = aulStyleDashDotDot;
        pbrushPen->dwStyleCount = _countof(aulStyleDashDotDot);
        break;

    case PS_INSIDEFRAME:
        pbrushPen->flAttrs |= (BR_IS_SOLID | BR_IS_INSIDEFRAME);
        break;

    case PS_USERSTYLE:
        {
            UINT i;
            BOOL has_neg = FALSE, all_zero = TRUE;

            for(i = 0; (i < dwStyleCount) && !has_neg; i++)
            {
                has_neg = has_neg || (((INT)(pStyle[i])) < 0);
                all_zero = all_zero && (pStyle[i] == 0);
            }

            if(all_zero || has_neg)
            {
                goto ExitCleanup;
            }
        }
        /* FIXME: What style here? */
        pbrushPen->flAttrs |= BR_IS_SOLID;
        pbrushPen->dwStyleCount = dwStyleCount;
        pbrushPen->pStyle = pStyle;
        break;

    default:
        DPRINT1("IntGdiExtCreatePen unknown penstyle %x\n", dwPenStyle);
        goto ExitCleanup;
    }

    if (pbrushPen->pStyle != NULL)
    {
        for (i = 0; i < pbrushPen->dwStyleCount; i++)
        {
            pbrushPen->ulStyleSize += pbrushPen->pStyle[i];
        }
    }

    NT_ASSERT((pbrushPen->dwStyleCount == 0) || (pbrushPen->pStyle != NULL));

    PEN_UnlockPen(pbrushPen);
    return hPen;

ExitCleanup:
    EngSetLastError(ERROR_INVALID_PARAMETER);
    pbrushPen->pStyle = NULL;
    GDIOBJ_vDeleteObject(&pbrushPen->BaseObject);

    return NULL;
}

VOID
FASTCALL
IntGdiSetSolidPenColor(HPEN hPen, COLORREF Color)
{
    PBRUSH pbrPen;

    pbrPen = PEN_ShareLockPen(hPen);
    if (pbrPen)
    {
        if (pbrPen->flAttrs & BR_IS_SOLID)
        {
            pbrPen->BrushAttr.lbColor = Color & 0xFFFFFF;
        }
        PEN_ShareUnlockPen(pbrPen);
    }
}

INT
APIENTRY
PEN_GetObject(PBRUSH pbrushPen, INT cbCount, PLOGPEN pBuffer)
{
    PLOGPEN pLogPen;
    PEXTLOGPEN pExtLogPen;
    INT cbRetCount;

    if (pbrushPen->flAttrs & BR_IS_OLDSTYLEPEN)
    {
        cbRetCount = sizeof(LOGPEN);
        if (pBuffer)
        {
            if (cbCount < cbRetCount) return 0;

            if (((pbrushPen->ulPenStyle & PS_STYLE_MASK) == PS_NULL) &&
                (cbCount == sizeof(EXTLOGPEN)))
            {
                pExtLogPen = (PEXTLOGPEN)pBuffer;
                pExtLogPen->elpPenStyle = pbrushPen->ulPenStyle;
                pExtLogPen->elpWidth = 0;
                pExtLogPen->elpBrushStyle = pbrushPen->iBrushStyle;
                pExtLogPen->elpColor = pbrushPen->BrushAttr.lbColor;
                pExtLogPen->elpHatch = 0;
                pExtLogPen->elpNumEntries = 0;
                cbRetCount = sizeof(EXTLOGPEN);
            }
            else
            {
                pLogPen = (PLOGPEN)pBuffer;
                pLogPen->lopnWidth.x = pbrushPen->lWidth;
                pLogPen->lopnWidth.y = 0;
                pLogPen->lopnStyle = pbrushPen->ulPenStyle;
                pLogPen->lopnColor = pbrushPen->BrushAttr.lbColor;
            }
        }
    }
    else
    {
        DWORD dwStyleCount = (pbrushPen->flAttrs & BR_IS_DEFAULTSTYLE) ?
            0 : pbrushPen->dwStyleCount;
        cbRetCount = sizeof(EXTLOGPEN) - sizeof(DWORD) + dwStyleCount * sizeof(DWORD);
        if (pBuffer)
        {
            ULONG i;

            if (cbCount < cbRetCount) return 0;
            pExtLogPen = (PEXTLOGPEN)pBuffer;
            pExtLogPen->elpPenStyle = pbrushPen->ulPenStyle;
            pExtLogPen->elpWidth = pbrushPen->lWidth;
            pExtLogPen->elpBrushStyle = pbrushPen->iBrushStyle;
            pExtLogPen->elpColor = pbrushPen->BrushAttr.lbColor;
            pExtLogPen->elpHatch = (ULONG_PTR)pbrushPen->hbmClient;
            pExtLogPen->elpNumEntries = dwStyleCount;
            for (i = 0; i < dwStyleCount; i++)
            {
                pExtLogPen->elpStyleEntry[i] = pbrushPen->pStyle[i];
            }
        }
    }

    return cbRetCount;
}


/* PUBLIC FUNCTIONS ***********************************************************/

HPEN
APIENTRY
NtGdiCreatePen(
    INT PenStyle,
    INT Width,
    COLORREF Color,
    IN HBRUSH hbr)
{
    if ((PenStyle < PS_SOLID) ||( PenStyle > PS_INSIDEFRAME))
    {
        EngSetLastError(ERROR_INVALID_PARAMETER);
        return NULL;
    }

    return IntGdiExtCreatePen(PenStyle,
                              Width,
                              BS_SOLID,
                              Color,
                              0,
                              0,
                              0,
                              NULL,
                              0,
                              TRUE,
                              hbr);
}

HPEN
APIENTRY
NtGdiExtCreatePen(
    DWORD dwPenStyle,
    DWORD ulWidth,
    IN ULONG ulBrushStyle,
    IN ULONG ulColor,
    IN ULONG_PTR ulClientHatch,
    IN ULONG_PTR ulHatch,
    DWORD dwStyleCount,
    PULONG pUnsafeStyle,
    IN ULONG cjDIB,
    IN BOOL bOldStylePen,
    IN OPTIONAL HBRUSH hBrush)
{
    NTSTATUS Status = STATUS_SUCCESS;
    DWORD* pSafeStyle = NULL;
    HPEN hPen;

    if ((int)dwStyleCount < 0) return 0;
    if (dwStyleCount > 16)
    {
        EngSetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }

    if (((dwPenStyle & PS_TYPE_MASK) == PS_COSMETIC) &&
        (ulBrushStyle != BS_SOLID))
    {
        EngSetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }

    if (((dwPenStyle & PS_STYLE_MASK) == PS_NULL) ||
        (ulBrushStyle == BS_NULL))
    {
        return StockObjects[NULL_PEN];
    }


    if ((ulBrushStyle == BS_PATTERN) ||
        (ulBrushStyle == BS_DIBPATTERN) ||
        (ulBrushStyle == BS_DIBPATTERNPT))
    {
        ulColor = 0;
    }
    else if ((ulBrushStyle != BS_SOLID) &&
             (ulBrushStyle != BS_HATCHED))
    {
        EngSetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }

    if ((dwPenStyle & PS_STYLE_MASK) != PS_USERSTYLE)
    {
        dwStyleCount = 0;
        pUnsafeStyle = NULL;
    }

    if (dwStyleCount > 0)
    {
        if (pUnsafeStyle == NULL)
        {
            EngSetLastError(ERROR_INVALID_PARAMETER);
            return 0;
        }

        pSafeStyle = ExAllocatePoolWithTag(NonPagedPool,
                                           dwStyleCount * sizeof(DWORD),
                                           GDITAG_PENSTYLE);
        if (!pSafeStyle)
        {
            SetLastNtError(ERROR_NOT_ENOUGH_MEMORY);
            return 0;
        }
        _SEH2_TRY
        {
            ProbeForRead(pUnsafeStyle, dwStyleCount * sizeof(DWORD), 1);
            RtlCopyMemory(pSafeStyle,
                          pUnsafeStyle,
                          dwStyleCount * sizeof(DWORD));
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            Status = _SEH2_GetExceptionCode();
        }
        _SEH2_END
        if(!NT_SUCCESS(Status))
        {
            SetLastNtError(Status);
            ExFreePoolWithTag(pSafeStyle, GDITAG_PENSTYLE);
            return 0;
        }
    }

    if (ulBrushStyle == BS_PATTERN)
    {
        _SEH2_TRY
        {
            ProbeForRead((PVOID)ulHatch, cjDIB, 1);
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            Status = _SEH2_GetExceptionCode();
        }
        _SEH2_END
        if(!NT_SUCCESS(Status))
        {
            SetLastNtError(Status);
            if (pSafeStyle) ExFreePoolWithTag(pSafeStyle, GDITAG_PENSTYLE);
            return 0;
        }
    }

    hPen = IntGdiExtCreatePen(dwPenStyle,
                              ulWidth,
                              ulBrushStyle,
                              ulColor,
                              ulClientHatch,
                              ulHatch,
                              dwStyleCount,
                              pSafeStyle,
                              cjDIB,
                              bOldStylePen,
                              hBrush);

    if (!hPen && pSafeStyle)
    {
        ExFreePoolWithTag(pSafeStyle, GDITAG_PENSTYLE);
    }

    return hPen;
}

/* EOF */
