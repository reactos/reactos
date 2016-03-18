/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS System Libraries
 * FILE:            win32ss/gdi/gdi32/objects/efloat.c
 * PURPOSE:         Functions to convert between FLOAT and EFLOAT
 * PROGRAMMER:      James Tabor
 */
#include <precomp.h>

/* the following deal with IEEE single-precision numbers */
#define EXCESS          126L
#define SIGNBIT         0x80000000L
#define SIGN(fp)        ((fp) & SIGNBIT)
#define EXP(fp)         (((fp) >> 23L) & 0xFF)
#define MANT(fp)        ((fp) & 0x7FFFFFL)
#define PACK(s,e,m)     ((s) | ((e) << 23L) | (m))

FLOATL
FASTCALL
EFtoF(EFLOAT_S * efp)
{
    ULONG Mant, Exp, Sign;

    if (!efp->lMant) return 0;

    Mant = efp->lMant;
    Exp = efp->lExp;
    Sign = SIGN(Mant);

    if (Sign) Mant = -(LONG)Mant;
    Mant >>= 7;
    Exp += (EXCESS-1);

    Mant = MANT(Mant);
    return PACK(Sign, Exp, Mant);
}

VOID
FASTCALL
FtoEF( EFLOAT_S * efp, FLOATL f)
{
    ULONG Mant, Exp, Sign = 0;
    gxf_long worker;

#ifdef _X86_
    worker.l = f; // It's a float stored in a long.
#else
    worker.f = f;
#endif

    Exp = EXP(worker.l);
    Mant = MANT(worker.l);
    if (SIGN(worker.l)) Sign = -1;

    Mant = ((Mant << 7) | 0x40000000);
    Mant ^= Sign;
    Mant -= Sign;
    Exp -= (EXCESS-1);

    efp->lMant = Mant;
    efp->lExp = Exp;
}
