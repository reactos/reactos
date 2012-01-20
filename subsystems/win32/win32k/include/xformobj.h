#pragma once

typedef struct _EXFORMOBJ
{
    MATRIX *pmx;
} EXFORMOBJ;

#define XFORMOBJ EXFORMOBJ
#define XFORMOBJ_iGetXform EXFORMOBJ_iGetXform
#define XFORMOBJ_iGetFloatObjXform EXFORMOBJ_iGetFloatObjXform
#define XFORMOBJ_bApplyXform EXFORMOBJ_bApplyXform
#define XFORMOBJ_vInit EXFORMOBJ_vInit
#define XFORMOBJ_pmx EXFORMOBJ_pmx
#define XFORMOBJ_iSetXform EXFORMOBJ_iSetXform
#define XFORMOBJ_iCombine EXFORMOBJ_iCombine
#define XFORMOBJ_iCombineXform EXFORMOBJ_iCombineXform
#define XFORMOBJ_iInverse EXFORMOBJ_iInverse

FORCEINLINE
VOID
XFORMOBJ_vInit(
    OUT XFORMOBJ *pxo,
    IN MATRIX *pmx)
{
    pxo->pmx = pmx;
}

FORCEINLINE
MATRIX*
XFORMOBJ_pmx(
    IN XFORMOBJ *pxo)
{
    return pxo->pmx;
}

ULONG
NTAPI
XFORMOBJ_iSetXform(
    OUT XFORMOBJ *pxo,
    IN const XFORML *pxform);

ULONG
NTAPI
XFORMOBJ_iCombine(
    IN XFORMOBJ *pxo,
    IN XFORMOBJ *pxo1,
    IN XFORMOBJ *pxo2);

ULONG
NTAPI
XFORMOBJ_iCombineXform(
    IN XFORMOBJ *pxo,
    IN XFORMOBJ *pxo1,
    IN XFORML *pxform,
    IN BOOL bLeftMultiply);

ULONG
NTAPI
XFORMOBJ_iInverse(
    OUT XFORMOBJ *pxoDst,
    IN XFORMOBJ *pxoSrc);

ULONG
APIENTRY
XFORMOBJ_iGetXform(
    IN XFORMOBJ *pxo,
    OUT XFORML *pxform);

BOOL
APIENTRY
XFORMOBJ_bApplyXform(
    IN XFORMOBJ *pxo,
    IN ULONG iMode,
    IN ULONG cPoints,
    IN PVOID pvIn,
    OUT PVOID pvOut);
