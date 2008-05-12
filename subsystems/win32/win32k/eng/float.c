/*
 *  ReactOS W32 Subsystem
 *  Copyright (C) 1998, 1999, 2000, 2001, 2002, 2003 ReactOS Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/* $Id$
 *
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS kernel
 * PURPOSE:           Engine floating point functions
 * FILE:              subsys/win32k/eng/float.c
 * PROGRAMER:         Jason Filby
 * REVISION HISTORY:
 */

/* INCLUDES *****************************************************************/

#include <w32k.h>

#define NDEBUG
#include <debug.h>

/* DEFINES *****************************************************************/

#ifdef _M_IX86
#ifdef __GNUC__
#define FLOAT_TO_INT(in,out)  \
           __asm__ __volatile__ ("fistpl %0" : "=m" (out) : "t" (in) : "st");
#else
#define FLOAT_TO_INT(in,out) \
          __asm fld in \
          __asm fistp out
#endif
#else
#define FLOAT_TO_INT(in,out)  \
          out = (long)in;
#endif

/* the following deal with IEEE single-precision numbers */
#define EXCESS          126L
#define SIGNBIT         0x80000000L
#define SIGN(fp)        ((fp) & SIGNBIT)
#define EXP(fp)         (((fp) >> 23L) & 0xFF)
#define MANT(fp)        ((fp) & 0x7FFFFFL)
#define PACK(s,e,m)     ((s) | ((e) << 23L) | (m))

/* FUNCTIONS *****************************************************************/

BOOL
STDCALL
EngRestoreFloatingPointState ( IN VOID *Buffer )
{
  NTSTATUS Status;
  Status = KeRestoreFloatingPointState((PKFLOATING_SAVE)Buffer);
  if (Status != STATUS_SUCCESS)
    {
      return FALSE;
    }
  return TRUE;
}

ULONG
STDCALL
EngSaveFloatingPointState(OUT VOID  *Buffer,
     IN ULONG  BufferSize)
{
  KFLOATING_SAVE TempBuffer;
  NTSTATUS Status;
  if (Buffer == NULL || BufferSize == 0)
    {
      /* Check for floating point support. */
      Status = KeSaveFloatingPointState(&TempBuffer);
      if (Status != STATUS_SUCCESS)
 {
   return(0);
 }
      KeRestoreFloatingPointState(&TempBuffer);
      return(sizeof(KFLOATING_SAVE));
    }
  if (BufferSize < sizeof(KFLOATING_SAVE))
    {
      return(0);
    }
  Status = KeSaveFloatingPointState((PKFLOATING_SAVE)Buffer);
  if (!NT_SUCCESS(Status))
    {
      return FALSE;
    }
  return TRUE;
}

VOID
FASTCALL
EF_Negate(EFLOAT_S * efp)
{
 efp->lMant = -efp->lMant;
}

LONG
FASTCALL
EFtoF( EFLOAT_S * efp)
{
 long Mant, Exp, Sign = 0;

 if (!efp->lMant) return 0;

 Mant = efp->lMant;
 Exp = efp->lExp;
 Sign = SIGN(Mant);

//// M$ storage emulation
 if( Sign ) Mant = -Mant;
 Mant = ((Mant & 0x3fffffff) >> 7);
 Exp += (EXCESS-1);
////
 Mant = MANT(Mant);
 return PACK(Sign, Exp, Mant);
}

VOID
FASTCALL
FtoEF( EFLOAT_S * efp, FLOATL f)
{
 long Mant, Exp, Sign = 0;
 gxf_long worker;

#ifdef _X86_
 worker.l = f; // It's a float stored in a long.
#else
 worker.f = f;
#endif

 Exp = EXP(worker.l);
 Mant = MANT(worker.l);
 if (SIGN(worker.l)) Sign = -1;
//// M$ storage emulation
 Mant = ((Mant << 7) | 0x40000000);
 Mant ^= Sign;
 Mant -= Sign;
 Exp -= (EXCESS-1);
////
 efp->lMant = Mant;
 efp->lExp = Exp;
}

VOID
STDCALL
FLOATOBJ_Add (
 IN OUT PFLOATOBJ  pf,
 IN PFLOATOBJ      pf1
 )
{
  // www.osr.com/ddk/graphics/gdifncs_2i3r.htm
  EFLOAT_S * efp = (EFLOAT_S *)pf;
  EFLOAT_S * efp1 = (EFLOAT_S *)pf1;
  gxf_long f;
  gxf_long f1;
  f.l = EFtoF(efp);
  f1.l = EFtoF(efp1);
  f.f = f.f + f1.f;
#ifdef _X86_
  FtoEF( efp, f.l );
#else
  FtoEF( efp, f.f );
#endif
}

VOID
STDCALL
FLOATOBJ_AddFloat(
 IN OUT PFLOATOBJ  pf,
 IN FLOATL  f
 )
{
  // www.osr.com/ddk/graphics/gdifncs_0ip3.htm
  EFLOAT_S * efp = (EFLOAT_S *)pf;
  gxf_long fe;
  gxf_long f1;
  fe.l = EFtoF(efp);
#ifdef _X86_
  f1.l = f;
#else
  f1.f = f;
#endif
  fe.f = fe.f + f1.f;
#ifdef _X86_
  FtoEF( efp, fe.l );
#else
  FtoEF( efp, fe.f );
#endif
}

VOID FASTCALL
XForm2MatrixS( MATRIX_S * Matrix, PXFORM XForm)
{
gxf_long f;
  f.f = XForm->eM11;
  FtoEF( &Matrix->efM11, f.l);
  f.f = XForm->eM12;
  FtoEF( &Matrix->efM12, f.l);
  f.f = XForm->eM21;
  FtoEF( &Matrix->efM21, f.l);
  f.f = XForm->eM22;
  FtoEF( &Matrix->efM22, f.l);
  f.f = XForm->eDx;
  FtoEF( &Matrix->efDx, f.l);
  f.f = XForm->eDy;
  FtoEF( &Matrix->efDy, f.l);
}

VOID FASTCALL
MatrixS2XForm( PXFORM XForm, MATRIX_S * Matrix)
{
gxf_long f;
  f.l =  EFtoF(&Matrix->efM11);
  XForm->eM11 = f.f;
  f.l =  EFtoF(&Matrix->efM12);
  XForm->eM12 = f.f;
  f.l =  EFtoF(&Matrix->efM21);
  XForm->eM21 = f.f;
  f.l =  EFtoF(&Matrix->efM22);
  XForm->eM22 = f.f;
  f.l =  EFtoF(&Matrix->efDx);
  XForm->eDx = f.f;
  f.l =  EFtoF(&Matrix->efDy);
  XForm->eDy = f.f;
}


VOID
STDCALL
FLOATOBJ_AddLong(
 IN OUT PFLOATOBJ  pf,
 IN LONG  l
 )
{
  // www.osr.com/ddk/graphics/gdifncs_12jr.htm
  EFLOAT_S * efp = (EFLOAT_S *)pf;
  gxf_long f;
  f.l = EFtoF(efp);
  f.f = f.f + l;
#ifdef _X86_
  FtoEF( efp, f.l );
#else
  FtoEF( efp, f.f );
#endif
}

VOID
STDCALL
FLOATOBJ_Div(
 IN OUT PFLOATOBJ  pf,
 IN PFLOATOBJ  pf1
 )
{
  // www.osr.com/ddk/graphics/gdifncs_3ndz.htm
  EFLOAT_S * efp = (EFLOAT_S *)pf;
  EFLOAT_S * efp1 = (EFLOAT_S *)pf1;
  gxf_long f;
  gxf_long f1;
  f.l = EFtoF(efp);
  f1.l = EFtoF(efp1);
  f.f = f.f / f1.f;
#ifdef _X86_
  FtoEF( efp, f.l );
#else
  FtoEF( efp, f.f );
#endif
}

VOID
STDCALL
FLOATOBJ_DivFloat(
 IN OUT PFLOATOBJ  pf,
 IN FLOATL  f
 )
{
  // www.osr.com/ddk/graphics/gdifncs_0gfb.htm
  EFLOAT_S * efp = (EFLOAT_S *)pf;
  gxf_long fe;
  gxf_long f1;
  fe.l = EFtoF(efp);
#ifdef _X86_
  f1.l = f;
#else
  f1.f = f;
#endif
  fe.f = fe.f / f1.f;
#ifdef _X86_
  FtoEF( efp, fe.l );
#else
  FtoEF( efp, fe.f );
#endif
}

VOID
STDCALL
FLOATOBJ_DivLong(
 IN OUT PFLOATOBJ  pf,
 IN LONG  l
 )
{
  // www.osr.com/ddk/graphics/gdifncs_6jdz.htm
  EFLOAT_S * efp = (EFLOAT_S *)pf;
  gxf_long f;
  f.l = EFtoF(efp);
  f.f = f.f / l;
#ifdef _X86_
  FtoEF( efp, f.l );
#else
  FtoEF( efp, f.f );
#endif
}

BOOL
STDCALL
FLOATOBJ_Equal(
 IN PFLOATOBJ  pf,
 IN PFLOATOBJ  pf1
 )
{
  // www.osr.com/ddk/graphics/gdifncs_6ysn.htm
  EFLOAT_S * efp = (EFLOAT_S *)pf;
  EFLOAT_S * efp1 = (EFLOAT_S *)pf1;
  gxf_long f;
  gxf_long f1;
  f.l = EFtoF(efp);
  f1.l = EFtoF(efp1);
  if (f.f == f1.f) return TRUE;
  return FALSE;
}

BOOL
STDCALL
FLOATOBJ_EqualLong(
 IN PFLOATOBJ  pf,
 IN LONG  l
 )
{
  // www.osr.com/ddk/graphics/gdifncs_1pgn.htm
  EFLOAT_S * efp = (EFLOAT_S *)pf;
  gxf_long f;
  f.l = EFtoF(efp);
  if (f.f == l) return TRUE;
  return FALSE;
}

LONG
STDCALL
FLOATOBJ_GetFloat ( IN PFLOATOBJ pf )
{
  // www.osr.com/ddk/graphics/gdifncs_4d5z.htm
  EFLOAT_S * efp = (EFLOAT_S *)pf;
  return EFtoF(efp);
}

LONG
STDCALL
FLOATOBJ_GetLong ( IN PFLOATOBJ pf )
{
  // www.osr.com/ddk/graphics/gdifncs_0tgn.htm
  EFLOAT_S * efp = (EFLOAT_S *)pf;
  gxf_long f;
  long l;

  f.l = EFtoF( efp );
  FLOAT_TO_INT(f.f, l); // Let FPP handle it the fasty haxy way.

  return l;
}

BOOL
STDCALL
FLOATOBJ_GreaterThan(
 IN PFLOATOBJ  pf,
 IN PFLOATOBJ  pf1
 )
{
  // www.osr.com/ddk/graphics/gdifncs_8n53.htm
  EFLOAT_S * efp = (EFLOAT_S *)pf;
  EFLOAT_S * efp1 = (EFLOAT_S *)pf1;
  gxf_long f;
  gxf_long f1;
  f.l = EFtoF(efp);
  f1.l = EFtoF(efp1);
  if(f.f > f1.f) return TRUE;
  return FALSE;
}

BOOL
STDCALL
FLOATOBJ_GreaterThanLong(
 IN PFLOATOBJ  pf,
 IN LONG  l
 )
{
  // www.osr.com/ddk/graphics/gdifncs_6gx3.htm
  EFLOAT_S * efp = (EFLOAT_S *)pf;
  gxf_long f;
  f.l = EFtoF(efp);
  if (f.f > l) return TRUE;
  return FALSE;
}

BOOL
STDCALL
FLOATOBJ_LessThan(
 IN PFLOATOBJ  pf,
 IN PFLOATOBJ  pf1
 )
{
  // www.osr.com/ddk/graphics/gdifncs_1ynb.htm
  EFLOAT_S * efp = (EFLOAT_S *)pf;
  EFLOAT_S * efp1 = (EFLOAT_S *)pf1;
  gxf_long f;
  gxf_long f1;
  f.l = EFtoF(efp);
  f1.l = EFtoF(efp1);
  if(f.f < f1.f) return TRUE;
  return FALSE;
}

BOOL
STDCALL
FLOATOBJ_LessThanLong(
 IN PFLOATOBJ  pf,
 IN LONG  l
 )
{
  // www.osr.com/ddk/graphics/gdifncs_9nzb.htm
  EFLOAT_S * efp = (EFLOAT_S *)pf;
  gxf_long f;
  f.l = EFtoF(efp);
  if (f.f < l) return TRUE;
  return FALSE;
}

VOID
STDCALL
FLOATOBJ_Mul(
 IN OUT PFLOATOBJ  pf,
 IN PFLOATOBJ  pf1
 )
{
  // www.osr.com/ddk/graphics/gdifncs_8ppj.htm
  EFLOAT_S * efp = (EFLOAT_S *)pf;
  EFLOAT_S * efp1 = (EFLOAT_S *)pf1;
  gxf_long f;
  gxf_long f1;
  f.l = EFtoF(efp);
  f1.l = EFtoF(efp1);
  f.f = f1.f * f.f;
#ifdef _X86_
  FtoEF( efp, f.l );
#else
  FtoEF( efp, f.f );
#endif
}

VOID
STDCALL
FLOATOBJ_MulFloat(
 IN OUT PFLOATOBJ  pf,
 IN FLOATL  f
 )
{
  // www.osr.com/ddk/graphics/gdifncs_3puv.htm
  EFLOAT_S * efp = (EFLOAT_S *)pf;
  gxf_long fe;
  gxf_long f1;
  fe.l = EFtoF(efp);
#ifdef _X86_
  f1.l = f;
#else
  f1.f = f;
#endif
  fe.f = f1.f * fe.f;
#ifdef _X86_
  FtoEF( efp, fe.l );
#else
  FtoEF( efp, fe.f );
#endif
}

VOID
STDCALL
FLOATOBJ_MulLong(
 IN OUT PFLOATOBJ  pf,
 IN LONG  l
 )
{
  // www.osr.com/ddk/graphics/gdifncs_56lj.htm
  EFLOAT_S * efp = (EFLOAT_S *)pf;
  gxf_long f;
  f.l = EFtoF(efp);
  f.f = f.f * l;
#ifdef _X86_
  FtoEF( efp, f.l );
#else
  FtoEF( efp, f.f );
#endif
}

VOID
STDCALL
FLOATOBJ_Neg ( IN OUT PFLOATOBJ pf )
{
  // www.osr.com/ddk/graphics/gdifncs_14pz.htm
  EFLOAT_S * efp = (EFLOAT_S *)pf;
  EF_Negate(efp);
}

VOID
STDCALL
FLOATOBJ_SetFloat(
 OUT PFLOATOBJ  pf,
 IN FLOATL  f
 )
{
  // www.osr.com/ddk/graphics/gdifncs_1prb.htm
  EFLOAT_S * efp = (EFLOAT_S *)pf;
  FtoEF( efp, f);
}

VOID
STDCALL
FLOATOBJ_SetLong(
 OUT PFLOATOBJ  pf,
 IN LONG  l
 )
{
  // www.osr.com/ddk/graphics/gdifncs_0gpz.htm
  EFLOAT_S * efp = (EFLOAT_S *)pf;
  gxf_long f;
  f.f = (float) l; // Convert it now.
#ifdef _X86_
  FtoEF( efp, f.l );
#else
  FtoEF( efp, f.f );
#endif
}

VOID
STDCALL
FLOATOBJ_Sub(
 IN OUT PFLOATOBJ  pf,
 IN PFLOATOBJ  pf1
 )
{
  // www.osr.com/ddk/graphics/gdifncs_6lyf.htm
  EFLOAT_S * efp = (EFLOAT_S *)pf;
  EFLOAT_S * efp1 = (EFLOAT_S *)pf1;
  gxf_long f;
  gxf_long f1;
  f.l = EFtoF(efp);
  f1.l = EFtoF(efp1);
  f.f = f.f - f1.f;
#ifdef _X86_
  FtoEF( efp, f.l );
#else
  FtoEF( efp, f.f );
#endif
}

VOID
STDCALL
FLOATOBJ_SubFloat(
 IN OUT PFLOATOBJ  pf,
 IN FLOATL  f
 )
{
  // www.osr.com/ddk/graphics/gdifncs_2zvr.htm
  EFLOAT_S * efp = (EFLOAT_S *)pf;
  gxf_long fe;
  gxf_long f1;
  fe.l = EFtoF(efp);
#ifdef _X86_
  f1.l = f;
#else
  f1.f = f;
#endif
  fe.f = fe.f - f1.f;
#ifdef _X86_
  FtoEF( efp, fe.l );
#else
  FtoEF( efp, fe.f );
#endif
}

VOID
STDCALL
FLOATOBJ_SubLong(
 IN OUT PFLOATOBJ  pf,
 IN LONG  l
 )
{
  // www.osr.com/ddk/graphics/gdifncs_852f.htm
  EFLOAT_S * efp = (EFLOAT_S *)pf;
  gxf_long f;
  f.l = EFtoF(efp);
  f.f = f.f - l;
#ifdef _X86_
  FtoEF( efp, f.l );
#else
  FtoEF( efp, f.f );
#endif
}
