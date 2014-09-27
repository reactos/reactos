#pragma once

#if defined(_M_IX86)

FORCEINLINE
BOOL
_FLOATOBJ_Equal(FLOATOBJ *pf1, FLOATOBJ *pf2)
{
    EFLOAT_S *pef1 = (EFLOAT_S*)pf1;
    EFLOAT_S *pef2 = (EFLOAT_S*)pf2;
    return (pef1->lMant == pef2->lMant && pef1->lExp == pef2->lExp);
}

FORCEINLINE
LONG
_FLOATOBJ_GetLong(FLOATOBJ *pf)
{
    EFLOAT_S *pef = (EFLOAT_S*)pf;
    return pef->lMant >> (32 - pef->lExp);
}

FORCEINLINE
LONG
_FLOATOBJ_GetFix(FLOATOBJ *pf)
{
    EFLOAT_S *pef = (EFLOAT_S*)pf;
    LONG Shift = (28 - pef->lExp);
    return (Shift >= 0 ? pef->lMant >> Shift : pef->lMant << -Shift);
}

FORCEINLINE
BOOL
_FLOATOBJ_IsLong(FLOATOBJ *pf)
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
_FLOATOBJ_Equal0(FLOATOBJ *pf)
{
    EFLOAT_S *pef = (EFLOAT_S*)pf;
    return (pef->lMant == 0 && pef->lExp == 0);
}

FORCEINLINE
BOOL
_FLOATOBJ_Equal1(FLOATOBJ *pf)
{
    EFLOAT_S *pef = (EFLOAT_S*)pf;
    return (pef->lMant == 0x40000000 && pef->lExp == 2);
}

extern const FLOATOBJ gef0;
extern const FLOATOBJ gef1;
extern const FLOATOBJ gef16;

#define FLOATOBJ_0 {0x00000000, 0x00000000}
#define FLOATOBJ_1 {0x40000000, 0x00000002}
#define FLOATOBJ_16 {0x40000000, 0x00000006}
#define FLOATOBJ_1_16 {0x40000000, 0xfffffffe}

#define FLOATOBJ_Set0(fo) do { (fo)->ul1 = 0; (fo)->ul2 = 0; } while (0)
#define FLOATOBJ_Set1(fo) do { (fo)->ul1 = 0x40000000; (fo)->ul2 = 2; } while (0)

#else

#define _FLOATOBJ_Equal(pf,pf1) (*(pf) == *(pf1))
#define _FLOATOBJ_GetLong(pf) ((LONG)*(pf))
#define _FLOATOBJ_IsLong(pf) ((FLOAT)((LONG)*(pf)) == *(pf))
#define _FLOATOBJ_Equal0(pf) (*(pf) == 0.)
#define _FLOATOBJ_Equal1(pf) (*(pf) == 1.)
#define _FLOATOBJ_GetFix(pf) ((LONG)(*(pf) * 16.))

#define FLOATOBJ_0 0.
#define FLOATOBJ_1 1.
#define FLOATOBJ_16 16.
#define FLOATOBJ_1_16 (1./16.)

static const FLOATOBJ gef0 = 0.;
static const FLOATOBJ gef1 = 1.;
static const FLOATOBJ gef16 = 16.;

#define FLOATOBJ_Set0(fo) *(fo) = 0;
#define FLOATOBJ_Set1(fo) *(fo) = 1;

#endif
