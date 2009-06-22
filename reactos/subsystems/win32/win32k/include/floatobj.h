#ifndef _WIN32K_FLOATOBJ_H_
#define _WIN32K_FLOATOBJ_H_

#if defined(_X86_)

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
	ULONG Shift = 32 - pef->lExp;
	if (Shift > 31) return FALSE;
	return (((pef->lMant >> Shift) << Shift) == pef->lMant);
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

#define FLOATOBJ_Set0(fo) (fo)->ul1 = 0; (fo)->ul2 = 0;
#define FLOATOBJ_Set1(fo) (fo)->ul1 = 0x40000000; (fo)->ul2 = 2;

#else

#define _FLOATOBJ_Equal(pf,pf1) (*(pf) == *(pf1))
#define _FLOATOBJ_GetLong(pf) ((LONG)*(pf))
#define _FLOATOBJ_IsLong(pf) ((FLOAT)((LONG)*(pf)) == *(pf))
#define _FLOATOBJ_Equal0(pf) (*(pf) == 0.)
#define _FLOATOBJ_Equal1(pf) (*(pf) == 1.)
#define _FLOATOBJ_GetFix(pf) ((LONG)(*(pf) * 16.))

#define FLOATOBJ_Set0(fo) *(fo) = 0; 
#define FLOATOBJ_Set1(fo) *(fo) = 1;

#endif

#endif /* not _WIN32K_FLOATOBJ_H_ */
