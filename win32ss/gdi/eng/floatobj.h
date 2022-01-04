#pragma once

C_ASSERT(sizeof(FIX) == sizeof(LONG));
#define FIX2LONG(x) (((x) + 8) >> 4)
#define LONG2FIX(x) ((x) << 4)

#if defined(_M_IX86)

FORCEINLINE
BOOL
_FLOATOBJ_Equal(FLOATOBJ *pf1, FLOATOBJ *pf2)
{
    EFLOAT_S *pef1 = (EFLOAT_S*)pf1;
    EFLOAT_S *pef2 = (EFLOAT_S*)pf2;
    return (pef1->lMant == pef2->lMant && pef1->lExp == pef2->lExp);
}
#define FLOATOBJ_Equal _FLOATOBJ_Equal

/*!
 * \brief Converts a FLOATOBJ into a LONG by truncating the value to integer
 *
 * \param pf - Pointer to a FLOATOBJ containing the value to convert
 *
 * \param pl - Pointer to a variable that receives the result
 *
 * \return TRUE if the function succeeded, FALSE if the result would overflow
 *         a LONG.
 */
FORCEINLINE
BOOL
FLOATOBJ_bConvertToLong(FLOATOBJ *pf, PLONG pl)
{
    EFLOAT_S *pef = (EFLOAT_S*)pf;

    if (pef->lExp > 32)
    {
        return FALSE;
    }

    if (pef->lExp < 2)
    {
        *pl = 0;
        return TRUE;
    }

    *pl = EngMulDiv(pef->lMant, 1 << (pef->lExp - 2), 0x40000000);
    return TRUE;
}

FORCEINLINE
LONG
FLOATOBJ_GetFix(FLOATOBJ *pf)
{
    EFLOAT_S *pef = (EFLOAT_S*)pf;
    LONG Shift = (28 - pef->lExp);
    return (Shift >= 0 ? pef->lMant >> Shift : pef->lMant << -Shift);
}

FORCEINLINE
BOOL
FLOATOBJ_IsLong(FLOATOBJ *pf)
{
    EFLOAT_S *pef = (EFLOAT_S*)pf;
    ULONG ulShift = pef->lExp;
    if (ulShift < 32)
        return ((pef->lMant << ulShift) == 0);
    else
        return (ulShift == 32);
}

FORCEINLINE
BOOL
FLOATOBJ_Equal0(FLOATOBJ *pf)
{
    EFLOAT_S *pef = (EFLOAT_S*)pf;
    return (pef->lMant == 0 && pef->lExp == 0);
}

FORCEINLINE
BOOL
FLOATOBJ_Equal1(FLOATOBJ *pf)
{
    EFLOAT_S *pef = (EFLOAT_S*)pf;
    return (pef->lMant == 0x40000000 && pef->lExp == 2);
}

extern const FLOATOBJ gef0;
extern const FLOATOBJ gef1;
extern const FLOATOBJ gef2;
extern const FLOATOBJ gef16;

#define FLOATOBJ_0 {0x00000000, 0x00000000}
#define FLOATOBJ_1 {0x40000000, 0x00000002}
#define FLOATOBJ_16 {0x40000000, 0x00000006}
#define FLOATOBJ_1_16 {0x40000000, 0xfffffffe}

#define FLOATOBJ_Set0(fo) do { (fo)->ul1 = 0; (fo)->ul2 = 0; } while (0)
#define FLOATOBJ_Set1(fo) do { (fo)->ul1 = 0x40000000; (fo)->ul2 = 2; } while (0)

#else

#define FLOATOBJ_bConvertToLong(pf, pl) (*pl = (LONG)*pf, TRUE)
#define FLOATOBJ_IsLong(pf) ((FLOAT)((LONG)*(pf)) == *(pf))
#define FLOATOBJ_Equal0(pf) (*(pf) == 0.)
#define FLOATOBJ_Equal1(pf) (*(pf) == 1.)
#define FLOATOBJ_GetFix(pf) ((LONG)(*(pf) * 16.))

#define FLOATOBJ_0 0.
#define FLOATOBJ_1 1.
#define FLOATOBJ_16 16.
#define FLOATOBJ_1_16 (1./16.)

static const FLOATOBJ gef0 = 0.;
static const FLOATOBJ gef1 = 1.;
static const FLOATOBJ gef2 = 2.;
static const FLOATOBJ gef16 = 16.;

#define FLOATOBJ_Set0(fo) *(fo) = 0;
#define FLOATOBJ_Set1(fo) *(fo) = 1;

#endif
