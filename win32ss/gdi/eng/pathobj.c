/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS win32 subsystem
 * PURPOSE:          PATHOBJ service routines
 * FILE:             win32ss/gdi/eng/pathobj.c
 * PROGRAMERS:       Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 */

/* INCLUDES *****************************************************************/

#include <win32k.h>
#undef XFORMOBJ

#define NDEBUG
#include <debug.h>

/* TYPES *********************************************************************/

/* extended PATHDATA */
typedef struct _EXTPATHDATA
{
    PATHDATA pd;
    struct _EXTPATHDATA *ppdNext;
} EXTPATHDATA, *PEXTPATHDATA;

/* extended PATHOBJ */
typedef struct _EXTPATHOBJ
{
    PATHOBJ po;
    PEXTPATHDATA ppdFirst;
    PEXTPATHDATA ppdLast;
    PEXTPATHDATA ppdCurrent;
} EXTPATHOBJ, *PEXTPATHOBJ;

/* FUNCTIONS *****************************************************************/

/* FIXME: set last error */
/* FIXME: PATHOBJ_vEnumStartClipLines and PATHOBJ_bEnumClipLines */

/*
 * @implemented
 */
PATHOBJ*
APIENTRY
EngCreatePath(VOID)
{
    PEXTPATHOBJ pPathObj;
    const ULONG size = sizeof(EXTPATHOBJ);

    pPathObj = ExAllocatePoolWithTag(PagedPool, size, GDITAG_PATHOBJ);
    if (pPathObj == NULL)
    {
        return NULL;
    }

    RtlZeroMemory(pPathObj, size);
    return &pPathObj->po;
}

/*
 * @implemented
 */
VOID
APIENTRY
EngDeletePath(IN PATHOBJ *ppo)
{
    PEXTPATHOBJ pPathObj;
    PEXTPATHDATA ppd, ppdNext;

    pPathObj = (PEXTPATHOBJ)ppo;
    if (pPathObj == NULL)
        return;

    for (ppd = pPathObj->ppdFirst; ppd; ppd = ppdNext)
    {
        ppdNext = ppd->ppdNext;
        ExFreePoolWithTag(ppd, GDITAG_PATHOBJ);
    }
    ExFreePoolWithTag(pPathObj, GDITAG_PATHOBJ);
}

/*
 * @implemented
 */
BOOL
APIENTRY
PATHOBJ_bCloseFigure(IN PATHOBJ *ppo)
{
    PEXTPATHDATA ppd;
    PEXTPATHOBJ pPathObj = (PEXTPATHOBJ)ppo;
    if (pPathObj == NULL)
        return FALSE;

    ppd = pPathObj->ppdLast;
    if (ppd == NULL)
        return FALSE;

    ppd->pd.flags |= PD_CLOSEFIGURE | PD_ENDSUBPATH;
    return TRUE;
}

/*
 * @implemented
 */
VOID
APIENTRY
PATHOBJ_vEnumStart(IN PATHOBJ *ppo)
{
    PEXTPATHOBJ pPathObj = (PEXTPATHOBJ)ppo;
    if (pPathObj == NULL)
        return;

    pPathObj->ppdCurrent = pPathObj->ppdFirst;
}

/*
 * @implemented
 */
BOOL
APIENTRY
PATHOBJ_bEnum(
    IN  PATHOBJ   *ppo,
    OUT PATHDATA  *ppd)
{
    PEXTPATHOBJ pPathObj = (PEXTPATHOBJ)ppo;
    if (pPathObj == NULL || pPathObj->ppdCurrent == NULL)
        return FALSE;

    *ppd = pPathObj->ppdCurrent->pd;

    pPathObj->ppdCurrent = pPathObj->ppdCurrent->ppdNext;
    return (pPathObj->ppdCurrent != NULL);
}

/*
 * @implemented
 */
BOOL
APIENTRY
PATHOBJ_bMoveTo(
    IN PATHOBJ  *ppo,
    IN POINTFIX  ptfx)
{
    PEXTPATHOBJ pPathObj;
    PEXTPATHDATA ppd, ppdLast;

    pPathObj = (PEXTPATHOBJ)ppo;
    if (pPathObj == NULL)
        return FALSE;

    /* allocate a subpath data */
    ppd = ExAllocatePoolWithTag(PagedPool, sizeof(EXTPATHDATA), GDITAG_PATHOBJ);
    if (ppd == NULL)
        return FALSE;

    RtlZeroMemory(ppd, sizeof(EXTPATHDATA));

    /* add the first point to the subpath */
    ppd->pd.flags = PD_BEGINSUBPATH;
    ppd->pd.count = 1;
    ppd->pd.pptfx = ExAllocatePoolWithTag(PagedPool, sizeof(POINTFIX), GDITAG_PATHOBJ);
    if (ppd->pd.pptfx == NULL)
    {
        ExFreePoolWithTag(ppd, GDITAG_PATHOBJ);
        return FALSE;
    }
    ppd->pd.pptfx[0] = ptfx;

    ppdLast = pPathObj->ppdLast;
    if (ppdLast)
    {
        /* end the last subpath */
        ppdLast->pd.flags |= PD_ENDSUBPATH;

        /* add the subpath to the last */
        ppdLast->ppdNext = ppd;
        pPathObj->ppdLast = ppd;
    }
    else
    {
        /* add the subpath */
        pPathObj->ppdLast = pPathObj->ppdFirst = ppd;
    }

    pPathObj->po.cCurves++;

    return TRUE;
}

/*
 * @implemented
 */
BOOL
APIENTRY
PATHOBJ_bPolyLineTo(
    IN PATHOBJ  *ppo,
    IN POINTFIX  *pptfx,
    IN ULONG  cptfx)
{
    PEXTPATHOBJ pPathObj;
    PEXTPATHDATA ppd, ppdLast;
    PPOINTFIX pptfxNew, pptfxOld;
    ULONG size;

    pPathObj = (PEXTPATHOBJ)ppo;
    if (pPathObj == NULL || pptfx == NULL || cptfx == 0)
        return FALSE;

    ppdLast = pPathObj->ppdLast;
    if (ppdLast == NULL)
    {
        /* allocate a subpath data */
        ppd = ExAllocatePoolWithTag(PagedPool, sizeof(EXTPATHDATA), GDITAG_PATHOBJ);
        if (ppd == NULL)
            return FALSE;

        /* store data */
        RtlZeroMemory(ppd, sizeof(EXTPATHDATA));
        ppd->pd.flags = PD_BEGINSUBPATH;

        size = cptfx * sizeof(POINTFIX);
        pptfxNew = ExAllocatePoolWithTag(PagedPool, size, GDITAG_PATHOBJ);
        if (pptfxNew == NULL)
        {
            ExFreePoolWithTag(ppd, GDITAG_PATHOBJ);
            return FALSE;
        }
        RtlCopyMemory(pptfxNew, pptfx, size);
        ppd->pd.pptfx = pptfxNew;
        ppd->pd.count = cptfx;

        /* set the subpath */
        pPathObj->ppdLast = pPathObj->ppdFirst = ppd;

        pPathObj->po.cCurves++;
    }
    else if (ppdLast->pd.flags & (PD_BEZIERS | PD_ENDSUBPATH))
    {
        /* allocate a subpath data */
        ppd = ExAllocatePoolWithTag(PagedPool, sizeof(EXTPATHDATA), GDITAG_PATHOBJ);
        if (ppd == NULL)
            return FALSE;

        /* store data */
        RtlZeroMemory(ppd, sizeof(EXTPATHDATA));
        ppd->pd.flags = 0;
        ppd->pd.count = cptfx;

        size = cptfx * sizeof(POINTFIX);
        pptfxNew = ExAllocatePoolWithTag(PagedPool, size, GDITAG_PATHOBJ);
        if (pptfxNew == NULL)
        {
            ExFreePoolWithTag(ppd, GDITAG_PATHOBJ);
            return FALSE;
        }
        RtlCopyMemory(pptfxNew, pptfx, size);
        ppd->pd.pptfx = pptfxNew;

        /* add to last */
        ppdLast->ppdNext = ppd;
        pPathObj->ppdLast = ppd;

        pPathObj->po.cCurves++;
    }
    else
    {
        /* concatenate ppdLast->pd.pptfx and pptfx */
        size = (ppdLast->pd.count + cptfx) * sizeof(POINTFIX);
        pptfxNew = ExAllocatePoolWithTag(PagedPool, size, GDITAG_PATHOBJ);
        if (pptfxNew == NULL)
            return FALSE;

        size = ppdLast->pd.count * sizeof(POINTFIX);
        RtlCopyMemory(&pptfxNew[0], ppdLast->pd.pptfx, size);
        size = cptfx * sizeof(POINTFIX);
        RtlCopyMemory(&pptfxNew[ppdLast->pd.count], pptfx, size);

        pptfxOld = ppdLast->pd.pptfx;
        ppdLast->pd.pptfx = pptfxNew;
        ppdLast->pd.count += cptfx;
        ExFreePoolWithTag(pptfxOld, GDITAG_PATHOBJ);
    }

    return TRUE;
}

/*
 * @implemented
 */
BOOL
APIENTRY
PATHOBJ_bPolyBezierTo(
    IN PATHOBJ  *ppo,
    IN POINTFIX  *pptfx,
    IN ULONG  cptfx)
{
    PEXTPATHOBJ pPathObj;
    PEXTPATHDATA ppd, ppdLast;
    PPOINTFIX pptfxNew, pptfxOld;
    ULONG size;

    pPathObj = (PEXTPATHOBJ)ppo;
    if (pPathObj == NULL || pptfx == NULL || cptfx == 0)
        return FALSE;

    ppdLast = pPathObj->ppdLast;
    if (ppdLast == NULL)
    {
        /* allocate a subpath data */
        ppd = ExAllocatePoolWithTag(PagedPool, sizeof(EXTPATHDATA), GDITAG_PATHOBJ);
        if (ppd == NULL)
            return FALSE;

        /* store data */
        RtlZeroMemory(ppd, sizeof(EXTPATHDATA));
        ppd->pd.flags = PD_BEGINSUBPATH | PD_BEZIERS;

        size = cptfx * sizeof(POINTFIX);
        pptfxNew = ExAllocatePoolWithTag(PagedPool, size, GDITAG_PATHOBJ);
        if (pptfxNew == NULL)
        {
            ExFreePoolWithTag(ppd, GDITAG_PATHOBJ);
            return FALSE;
        }
        RtlCopyMemory(pptfxNew, pptfx, size);
        ppd->pd.pptfx = pptfxNew;
        ppd->pd.count = cptfx;

        /* set the subpath */
        pPathObj->ppdLast = pPathObj->ppdFirst = ppd;

        pPathObj->po.cCurves++;
    }
    else if (!(ppdLast->pd.flags & PD_BEZIERS) || (ppdLast->pd.flags & PD_ENDSUBPATH))
    {
        /* allocate a subpath data */
        ppd = ExAllocatePoolWithTag(PagedPool, sizeof(EXTPATHDATA), GDITAG_PATHOBJ);
        if (ppd == NULL)
            return FALSE;

        /* store data */
        RtlZeroMemory(ppd, sizeof(EXTPATHDATA));
        ppd->pd.flags = PD_BEZIERS;
        ppd->pd.count = cptfx;

        size = cptfx * sizeof(POINTFIX);
        pptfxNew = ExAllocatePoolWithTag(PagedPool, size, GDITAG_PATHOBJ);
        if (pptfxNew == NULL)
        {
            ExFreePoolWithTag(ppd, GDITAG_PATHOBJ);
            return FALSE;
        }
        RtlCopyMemory(pptfxNew, pptfx, size);
        ppd->pd.pptfx = pptfxNew;

        /* add to last */
        ppdLast->ppdNext = ppd;
        pPathObj->ppdLast = ppd;

        pPathObj->po.cCurves++;
    }
    else
    {
        /* concatenate ppdLast->pd.pptfx and pptfx */
        size = (ppdLast->pd.count + cptfx) * sizeof(POINTFIX);
        pptfxNew = ExAllocatePoolWithTag(PagedPool, size, GDITAG_PATHOBJ);
        if (pptfxNew == NULL)
            return FALSE;

        size = ppdLast->pd.count * sizeof(POINTFIX);
        RtlCopyMemory(&pptfxNew[0], ppdLast->pd.pptfx, size);
        size = cptfx * sizeof(POINTFIX);
        RtlCopyMemory(&pptfxNew[ppdLast->pd.count], pptfx, size);

        pptfxOld = ppdLast->pd.pptfx;
        ppdLast->pd.pptfx = pptfxNew;
        ppdLast->pd.count += cptfx;
        ExFreePoolWithTag(pptfxOld, GDITAG_PATHOBJ);
    }

    pPathObj->po.fl |= PO_BEZIERS;

    return TRUE;
}

VOID
APIENTRY
PATHOBJ_vEnumStartClipLines(
    IN PATHOBJ  *ppo,
    IN CLIPOBJ  *pco,
    IN SURFOBJ  *pso,
    IN LINEATTRS  *pla)
{
    UNIMPLEMENTED;
}

BOOL
APIENTRY
PATHOBJ_bEnumClipLines(
    IN PATHOBJ  *ppo,
    IN ULONG  cb,
    OUT CLIPLINE  *pcl)
{
    UNIMPLEMENTED;
    return FALSE;
}

/*
 * @implemented
 */
VOID
APIENTRY
PATHOBJ_vGetBounds(
    IN PATHOBJ  *ppo,
    OUT PRECTFX  prectfx)
{
    FIX xLeft, yTop, xRight, yBottom;
    PEXTPATHOBJ pPathObj;
    PEXTPATHDATA ppd, ppdNext;
    ULONG i;

    pPathObj = (PEXTPATHOBJ)ppo;
    if (pPathObj == NULL || prectfx == NULL)
        return;

    yTop = xLeft = MAXLONG;
    yBottom = xRight = MINLONG;

    for (ppd = pPathObj->ppdFirst; ppd; ppd = ppdNext)
    {
        ppdNext = ppd->ppdNext;
        for (i = 0; i < ppd->pd.count; ++i)
        {
            PPOINTFIX pptfx = &ppd->pd.pptfx[i];
            if (pptfx->x < xLeft)
                xLeft = pptfx->x;
            if (pptfx->x > xRight)
                xRight = pptfx->x;
            if (pptfx->y < yTop)
                yTop = pptfx->y;
            if (pptfx->y > yBottom)
                yBottom = pptfx->y;
        }
    }

    if (xLeft <= xRight && yTop <= yBottom)
    {
        prectfx->xLeft = xLeft;
        prectfx->yTop = yTop;
        prectfx->xRight = xRight + 1;
        prectfx->yBottom = yBottom + 1;
    }
    else
    {
        RtlZeroMemory(prectfx, sizeof(*prectfx));
    }
}

/* EOF */
