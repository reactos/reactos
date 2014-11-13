/*
 * PROJECT:         ReactOS win32 kernel mode subsystem
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            win32ss/gdi/ntgdi/xformobj.c
 * PURPOSE:         XFORMOBJ API
 * PROGRAMMER:      Timo Kreuzer
 */

/** Includes ******************************************************************/

#include <win32k.h>
#define NDEBUG
#include <debug.h>

C_ASSERT(sizeof(FIX) == sizeof(LONG));
#define FIX2LONG(x) (((x) + 8) >> 4)
#define LONG2FIX(x) ((x) << 4)

#define FLOATOBJ_Equal _FLOATOBJ_Equal
#define FLOATOBJ_GetLong _FLOATOBJ_GetLong
#define FLOATOBJ_GetFix _FLOATOBJ_GetFix
#define FLOATOBJ_IsLong _FLOATOBJ_IsLong
#define FLOATOBJ_Equal0 _FLOATOBJ_Equal0
#define FLOATOBJ_Equal1 _FLOATOBJ_Equal1

/** Inline helper functions ***************************************************/

/*
 * Inline helper to calculate pfo1 * pfo2 + pfo3 * pfo4
 */
FORCEINLINE
VOID
MulAdd(
    PFLOATOBJ pfoDest,
    PFLOATOBJ pfo1,
    PFLOATOBJ pfo2,
    PFLOATOBJ pfo3,
    PFLOATOBJ pfo4)
{
    FLOATOBJ foTmp;

    *pfoDest = *pfo1;
    FLOATOBJ_Mul(pfoDest, pfo2);
    foTmp = *pfo3;
    FLOATOBJ_Mul(&foTmp, pfo4);
    FLOATOBJ_Add(pfoDest, &foTmp);
}

/*
 * Inline helper to calculate pfo1 * l2 + pfo3 * l4
 */
FORCEINLINE
VOID
MulAddLong(
    PFLOATOBJ pfoDest,
    PFLOATOBJ pfo1,
    LONG l2,
    PFLOATOBJ pfo3,
    LONG l4)
{
    FLOATOBJ foTmp;

    *pfoDest = *pfo1;
    FLOATOBJ_MulLong(pfoDest, l2);
    foTmp = *pfo3;
    FLOATOBJ_MulLong(&foTmp, l4);
    FLOATOBJ_Add(pfoDest, &foTmp);
}

/*
 * Inline helper to calculate pfo1 * pfo2 - pfo3 * pfo4
 */
FORCEINLINE
VOID
MulSub(
    PFLOATOBJ pfoDest,
    PFLOATOBJ pfo1,
    PFLOATOBJ pfo2,
    PFLOATOBJ pfo3,
    PFLOATOBJ pfo4)
{
    FLOATOBJ foTmp;

    *pfoDest = *pfo1;
    FLOATOBJ_Mul(pfoDest, pfo2);
    foTmp = *pfo3;
    FLOATOBJ_Mul(&foTmp, pfo4);
    FLOATOBJ_Sub(pfoDest, &foTmp);
}

/*
 * Inline helper to get the complexity hint from flAccel
 */
FORCEINLINE
ULONG
HintFromAccel(ULONG flAccel)
{
    switch (flAccel & (XFORM_SCALE|XFORM_UNITY|XFORM_NO_TRANSLATION))
    {
        case (XFORM_SCALE|XFORM_UNITY|XFORM_NO_TRANSLATION):
            return GX_IDENTITY;
        case (XFORM_SCALE|XFORM_UNITY):
            return GX_OFFSET;
        case XFORM_SCALE:
            return GX_SCALE;
        default:
            return GX_GENERAL;
    }
}

/** Internal functions ********************************************************/

ULONG
NTAPI
XFORMOBJ_UpdateAccel(
    IN XFORMOBJ *pxo)
{
    PMATRIX pmx = XFORMOBJ_pmx(pxo);

    /* Copy Dx and Dy to FIX format */
    pmx->fxDx = FLOATOBJ_GetFix(&pmx->efDx);
    pmx->fxDy = FLOATOBJ_GetFix(&pmx->efDy);

    pmx->flAccel = 0;

    if (FLOATOBJ_Equal0(&pmx->efDx) &&
        FLOATOBJ_Equal0(&pmx->efDy))
    {
        pmx->flAccel |= XFORM_NO_TRANSLATION;
    }

    if (FLOATOBJ_Equal0(&pmx->efM12) &&
        FLOATOBJ_Equal0(&pmx->efM21))
    {
        pmx->flAccel |= XFORM_SCALE;
    }

    if (FLOATOBJ_Equal1(&pmx->efM11) &&
        FLOATOBJ_Equal1(&pmx->efM22))
    {
        pmx->flAccel |= XFORM_UNITY;
    }

    if (FLOATOBJ_IsLong(&pmx->efM11) && FLOATOBJ_IsLong(&pmx->efM12) &&
        FLOATOBJ_IsLong(&pmx->efM21) && FLOATOBJ_IsLong(&pmx->efM22))
    {
        pmx->flAccel |= XFORM_INTEGER;
    }

    return HintFromAccel(pmx->flAccel);
}


ULONG
NTAPI
XFORMOBJ_iSetXform(
    OUT XFORMOBJ *pxo,
    IN const XFORML *pxform)
{
    PMATRIX pmx = XFORMOBJ_pmx(pxo);

    /* Check parameters */
    if (!pxo || !pxform) return DDI_ERROR;

    /* Check if the xform is valid */
    if ((pxform->eM11 == 0) || (pxform->eM22 == 0)) return DDI_ERROR;

    /* Copy members */
    FLOATOBJ_SetFloat(&pmx->efM11, pxform->eM11);
    FLOATOBJ_SetFloat(&pmx->efM12, pxform->eM12);
    FLOATOBJ_SetFloat(&pmx->efM21, pxform->eM21);
    FLOATOBJ_SetFloat(&pmx->efM22, pxform->eM22);
    FLOATOBJ_SetFloat(&pmx->efDx, pxform->eDx);
    FLOATOBJ_SetFloat(&pmx->efDy, pxform->eDy);

    /* Update accelerators and return complexity */
    return XFORMOBJ_UpdateAccel(pxo);
}


/*
 * Multiplies pxo1 with pxo2 and stores the result in pxo.
 * returns complexity hint
 * | efM11 efM12 0 |
 * | efM21 efM22 0 |
 * | efDx  efDy  1 |
 */
ULONG
NTAPI
XFORMOBJ_iCombine(
    IN XFORMOBJ *pxo,
    IN XFORMOBJ *pxo1,
    IN XFORMOBJ *pxo2)
{
    MATRIX mx;
    PMATRIX pmx, pmx1, pmx2;

    /* Get the source matrices */
    pmx1 = XFORMOBJ_pmx(pxo1);
    pmx2 = XFORMOBJ_pmx(pxo2);

    /* Do a 3 x 3 matrix multiplication with mx as destinantion */
    MulAdd(&mx.efM11, &pmx1->efM11, &pmx2->efM11, &pmx1->efM12, &pmx2->efM21);
    MulAdd(&mx.efM12, &pmx1->efM11, &pmx2->efM12, &pmx1->efM12, &pmx2->efM22);
    MulAdd(&mx.efM21, &pmx1->efM21, &pmx2->efM11, &pmx1->efM22, &pmx2->efM21);
    MulAdd(&mx.efM22, &pmx1->efM21, &pmx2->efM12, &pmx1->efM22, &pmx2->efM22);
    MulAdd(&mx.efDx, &pmx1->efDx, &pmx2->efM11, &pmx1->efDy, &pmx2->efM21);
    FLOATOBJ_Add(&mx.efDx, &pmx2->efDx);
    MulAdd(&mx.efDy, &pmx1->efDx, &pmx2->efM12, &pmx1->efDy, &pmx2->efM22);
    FLOATOBJ_Add(&mx.efDy, &pmx2->efDy);

    /* Copy back */
    pmx = XFORMOBJ_pmx(pxo);
    *pmx = mx;

    /* Update accelerators and return complexity */
    return XFORMOBJ_UpdateAccel(pxo);
}


ULONG
NTAPI
XFORMOBJ_iCombineXform(
    IN XFORMOBJ *pxo,
    IN XFORMOBJ *pxo1,
    IN XFORML *pxform,
    IN BOOL bLeftMultiply)
{
    MATRIX mx;
    XFORMOBJ xo2;

    XFORMOBJ_vInit(&xo2, &mx);
    XFORMOBJ_iSetXform(&xo2, pxform);

    if (bLeftMultiply)
    {
        return XFORMOBJ_iCombine(pxo, &xo2, pxo1);
    }
    else
    {
        return XFORMOBJ_iCombine(pxo, pxo1, &xo2);
    }
}

/*
 * A^-1 = adj(A) / det(AT)
 * A^-1 = 1/(a*d - b*c) * (a22,-a12,a21,-a11)
 */
ULONG
NTAPI
XFORMOBJ_iInverse(
    OUT XFORMOBJ *pxoDst,
    IN XFORMOBJ *pxoSrc)
{
    PMATRIX pmxDst, pmxSrc;
    FLOATOBJ foDet;
    XFORM xformSrc;

    pmxDst = XFORMOBJ_pmx(pxoDst);
    pmxSrc = XFORMOBJ_pmx(pxoSrc);

    XFORMOBJ_iGetXform(pxoSrc, (XFORML*)&xformSrc);

    /* det = M11 * M22 - M12 * M21 */
    MulSub(&foDet, &pmxSrc->efM11, &pmxSrc->efM22, &pmxSrc->efM12, &pmxSrc->efM21);

    if (FLOATOBJ_Equal0(&foDet))
    {
        /* Determinant is 0! */
        return DDI_ERROR;
    }

    /* Calculate adj(A) / det(A) */
    pmxDst->efM11 = pmxSrc->efM22;
    FLOATOBJ_Div(&pmxDst->efM11, &foDet);
    pmxDst->efM22 = pmxSrc->efM11;
    FLOATOBJ_Div(&pmxDst->efM22, &foDet);

    /* The other 2 are negative, negate foDet for that */
    FLOATOBJ_Neg(&foDet);
    pmxDst->efM12 = pmxSrc->efM12;
    FLOATOBJ_Div(&pmxDst->efM12, &foDet);
    pmxDst->efM21 = pmxSrc->efM21;
    FLOATOBJ_Div(&pmxDst->efM21, &foDet);

    /* Calculate the inverted x shift: Dx' = -Dx * M11' - Dy * M21' */
    pmxDst->efDx = pmxSrc->efDx;
    FLOATOBJ_Neg(&pmxDst->efDx);
    MulSub(&pmxDst->efDx, &pmxDst->efDx, &pmxDst->efM11, &pmxSrc->efDy, &pmxDst->efM21);

    /* Calculate the inverted y shift: Dy' = -Dy * M22' - Dx * M12' */
    pmxDst->efDy = pmxSrc->efDy;
    FLOATOBJ_Neg(&pmxDst->efDy);
    MulSub(&pmxDst->efDy, &pmxDst->efDy, &pmxDst->efM22, &pmxSrc->efDx, &pmxDst->efM12);

    /* Update accelerators and return complexity */
    return XFORMOBJ_UpdateAccel(pxoDst);
}


BOOL
NTAPI
XFORMOBJ_bXformFixPoints(
    IN XFORMOBJ  *pxo,
    IN ULONG  cPoints,
    IN PPOINTL  pptIn,
    OUT PPOINTL  pptOut)
{
    PMATRIX pmx;
    INT i;
    FLOATOBJ fo1, fo2;
    FLONG flAccel;

    pmx = XFORMOBJ_pmx(pxo);
    flAccel = pmx->flAccel;

    if ((flAccel & (XFORM_SCALE|XFORM_UNITY)) == (XFORM_SCALE|XFORM_UNITY))
    {
        /* Identity transformation, nothing todo */
    }
    else if (flAccel & XFORM_INTEGER)
    {
        if (flAccel & XFORM_UNITY)
        {
            /* 1-scale integer transform */
            LONG lM12 = FLOATOBJ_GetLong(&pmx->efM12);
            LONG lM21 = FLOATOBJ_GetLong(&pmx->efM21);

            i = cPoints - 1;
            do
            {
                LONG x = pptIn[i].x + pptIn[i].y * lM21;
                LONG y = pptIn[i].y + pptIn[i].x * lM12;
                pptOut[i].y = y;
                pptOut[i].x = x;
            }
            while (--i >= 0);
        }
        else if (flAccel & XFORM_SCALE)
        {
            /* Diagonal integer transform */
            LONG lM11 = FLOATOBJ_GetLong(&pmx->efM11);
            LONG lM22 = FLOATOBJ_GetLong(&pmx->efM22);

            i = cPoints - 1;
            do
            {
                pptOut[i].x = pptIn[i].x * lM11;
                pptOut[i].y = pptIn[i].y * lM22;
            }
            while (--i >= 0);
        }
        else
        {
            /* Full integer transform */
            LONG lM11 = FLOATOBJ_GetLong(&pmx->efM11);
            LONG lM12 = FLOATOBJ_GetLong(&pmx->efM12);
            LONG lM21 = FLOATOBJ_GetLong(&pmx->efM21);
            LONG lM22 = FLOATOBJ_GetLong(&pmx->efM22);

            i = cPoints - 1;
            do
            {
                LONG x;
                x  = pptIn[i].x * lM11;
                x += pptIn[i].y * lM21;
                pptOut[i].y  = pptIn[i].y * lM22;
                pptOut[i].y += pptIn[i].x * lM12;
                pptOut[i].x = x;
            }
            while (--i >= 0);
        }
    }
    else if (flAccel & XFORM_UNITY)
    {
        /* 1-scale transform */
        i = cPoints - 1;
        do
        {
            fo1 = pmx->efM21;
            FLOATOBJ_MulLong(&fo1, pptIn[i].y);
            fo2 = pmx->efM12;
            FLOATOBJ_MulLong(&fo2, pptIn[i].x);
            pptOut[i].x = pptIn[i].x + FLOATOBJ_GetLong(&fo1);
            pptOut[i].y = pptIn[i].y + FLOATOBJ_GetLong(&fo2);
        }
        while (--i >= 0);
    }
    else if (flAccel & XFORM_SCALE)
    {
        /* Diagonal float transform */
        i = cPoints - 1;
        do
        {
            fo1 = pmx->efM11;
            FLOATOBJ_MulLong(&fo1, pptIn[i].x);
            pptOut[i].x = FLOATOBJ_GetLong(&fo1);
            fo2 = pmx->efM22;
            FLOATOBJ_MulLong(&fo2, pptIn[i].y);
            pptOut[i].y = FLOATOBJ_GetLong(&fo2);
        }
        while (--i >= 0);
    }
    else
    {
        /* Full float transform */
        i = cPoints - 1;
        do
        {
            MulAddLong(&fo1, &pmx->efM11, pptIn[i].x, &pmx->efM21, pptIn[i].y);
            MulAddLong(&fo2, &pmx->efM12, pptIn[i].x, &pmx->efM22, pptIn[i].y);
            pptOut[i].x = FLOATOBJ_GetLong(&fo1);
            pptOut[i].y = FLOATOBJ_GetLong(&fo2);
        }
        while (--i >= 0);
    }

    if (!(pmx->flAccel & XFORM_NO_TRANSLATION))
    {
        /* Translate points */
        i = cPoints - 1;
        do
        {
            pptOut[i].x += pmx->fxDx;
            pptOut[i].y += pmx->fxDy;
        }
        while (--i >= 0);
    }

    return TRUE;
}

/** Public functions **********************************************************/

// www.osr.com/ddk/graphics/gdifncs_0s2v.htm
ULONG
APIENTRY
XFORMOBJ_iGetXform(
    IN XFORMOBJ *pxo,
    OUT XFORML *pxform)
{
    PMATRIX pmx = XFORMOBJ_pmx(pxo);

    /* Check parameters */
    if (!pxo || !pxform)
    {
        return DDI_ERROR;
    }

    /* Copy members */
    pxform->eM11 = FLOATOBJ_GetFloat(&pmx->efM11);
    pxform->eM12 = FLOATOBJ_GetFloat(&pmx->efM12);
    pxform->eM21 = FLOATOBJ_GetFloat(&pmx->efM21);
    pxform->eM22 = FLOATOBJ_GetFloat(&pmx->efM22);
    pxform->eDx = FLOATOBJ_GetFloat(&pmx->efDx);
    pxform->eDy = FLOATOBJ_GetFloat(&pmx->efDy);

    /* Return complexity hint */
    return HintFromAccel(pmx->flAccel);
}


// www.osr.com/ddk/graphics/gdifncs_5ig7.htm
ULONG
APIENTRY
XFORMOBJ_iGetFloatObjXform(
    IN XFORMOBJ *pxo,
    OUT FLOATOBJ_XFORM *pxfo)
{
    PMATRIX pmx = XFORMOBJ_pmx(pxo);

    /* Check parameters */
    if (!pxo || !pxfo)
    {
        return DDI_ERROR;
    }

    /* Copy members */
    pxfo->eM11 = pmx->efM11;
    pxfo->eM12 = pmx->efM12;
    pxfo->eM21 = pmx->efM21;
    pxfo->eM22 = pmx->efM22;
    pxfo->eDx = pmx->efDx;
    pxfo->eDy = pmx->efDy;

    /* Return complexity hint */
    return HintFromAccel(pmx->flAccel);
}


// www.osr.com/ddk/graphics/gdifncs_027b.htm
BOOL
APIENTRY
XFORMOBJ_bApplyXform(
    IN XFORMOBJ *pxo,
    IN ULONG iMode,
    IN ULONG cPoints,
    IN PVOID pvIn,
    OUT PVOID pvOut)
{
    MATRIX mx;
    XFORMOBJ xoInv;
    POINTL *pptl;
    INT i;

    /* Check parameters */
    if (!pxo || !pvIn || !pvOut || cPoints < 1)
    {
        return FALSE;
    }

    /* Use inverse xform? */
    if (iMode == XF_INV_FXTOL || iMode == XF_INV_LTOL)
    {
        XFORMOBJ_vInit(&xoInv, &mx);
        if (XFORMOBJ_iInverse(&xoInv, pxo) == DDI_ERROR)
        {
            return FALSE;
        }
        pxo = &xoInv;
    }

    /* Convert POINTL to POINTFIX? */
    if (iMode == XF_LTOFX || iMode == XF_LTOL || iMode == XF_INV_LTOL)
    {
        pptl = pvIn;
        for (i = cPoints - 1; i >= 0; i--)
        {
            pptl[i].x = LONG2FIX(pptl[i].x);
            pptl[i].y = LONG2FIX(pptl[i].y);
        }
    }

    /* Do the actual fixpoint transformation */
    if (!XFORMOBJ_bXformFixPoints(pxo, cPoints, pvIn, pvOut))
    {
        return FALSE;
    }

    /* Convert POINTFIX to POINTL? */
    if (iMode == XF_INV_FXTOL || iMode == XF_INV_LTOL || iMode == XF_LTOL)
    {
        pptl = pvOut;
        for (i = cPoints - 1; i >= 0; i--)
        {
            pptl[i].x = FIX2LONG(pptl[i].x);
            pptl[i].y = FIX2LONG(pptl[i].y);
        }
    }

    return TRUE;
}

/* EOF */
