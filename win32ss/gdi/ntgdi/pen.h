#pragma once

/* Internal interface */

typedef BRUSH PEN, *PPEN;

PPEN
NTAPI
PEN_AllocPenWithHandle(
    VOID);

PPEN
NTAPI
PEN_AllocExtPenWithHandle(
    VOID);

#define PEN_UnlockPen(pPenObj) GDIOBJ_vUnlockObject((POBJ)pPenObj)
#define PEN_ShareUnlockPen(ppen) GDIOBJ_vDereferenceObject((POBJ)ppen)

PPEN
FASTCALL
PEN_ShareLockPen(HPEN hpen);

INT
NTAPI
PEN_GetObject(
    _In_ PPEN pPen,
    _In_ INT Count,
    _Out_ PLOGPEN Buffer);

VOID FASTCALL AddPenLinesBounds(PDC,int,POINT *);

#define IntIsEffectiveWidePen(pbrLine) ( \
    (pbrLine)->lWidth > 1 && \
    ((pbrLine->flAttrs & BR_IS_OLDSTYLEPEN) || \
     ((pbrLine)->ulPenStyle & PS_TYPE_MASK) == PS_GEOMETRIC) \
)
