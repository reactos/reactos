#include "precomp.h"

/* the following deal with IEEE single-precision numbers */
#define EXCESS          126L
#define SIGNBIT         0x80000000L
#define SIGN(fp)        ((fp) & SIGNBIT)
#define EXP(fp)         (((fp) >> 23L) & 0xFF)
#define MANT(fp)        ((fp) & 0x7FFFFFL)
#define PACK(s,e,m)     ((s) | ((e) << 23L) | (m))

// Sames as lrintf.
#ifdef __GNUC__
#define FLOAT_TO_INT(in,out)  \
           __asm__ __volatile__ ("fistpl %0" : "=m" (out) : "t" (in) : "st");
#else
#define FLOAT_TO_INT(in,out) \
          __asm fld in \
          __asm fistp out
#endif

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


VOID FASTCALL
CoordCnvP(MATRIX_S * mx, LPPOINT Point)
{
  FLOAT x, y;
  gxf_long a, b;
  
  x = (FLOAT)Point->x;
  y = (FLOAT)Point->y;

  a.l = EFtoF( &mx->efM11 );
  b.l = EFtoF( &mx->efM21 );
  x = x * a.f + y * b.f + mx->fxDx;

  a.l = EFtoF( &mx->efM12 );
  b.l = EFtoF( &mx->efM22 );
  y = x * a.f + y * b.f + mx->fxDy;

  FLOAT_TO_INT(x, Point->x );
  FLOAT_TO_INT(y, Point->y );
}


BOOL
STDCALL
DPtoLP ( HDC hDC, LPPOINT Points, INT Count )
{
#if 0
  INT i;
  PDC_ATTR Dc_Attr;
 
  if (!GdiGetHandleUserData((HGDIOBJ) hDC, (PVOID) &Dc_Attr)) return FALSE;

  if (Dc_Attr->flXform & ( DEVICE_TO_WORLD_INVALID | // Force a full recalibration!
                           PAGE_XLATE_CHANGED      | // Changes or Updates have been made,
                           PAGE_EXTENTS_CHANGED    | // do processing in kernel space.
                           WORLD_XFORM_CHANGED )
#endif
    return NtGdiTransformPoints( hDC, Points, Points, Count, 0); // Last is 0 or 2
#if 0
  else
  {
    for ( i = 0; i < Count; i++ )
      CoordCnvP ( &Dc_Attr->mxDevicetoWorld, &Points[i] );
  }
  return TRUE;
#endif
}


BOOL
STDCALL
LPtoDP ( HDC hDC, LPPOINT Points, INT Count )
{
#if 0
  INT i;
  PDC_ATTR Dc_Attr;
 
  if (!GdiGetHandleUserData((HGDIOBJ) hDC, (PVOID) &Dc_Attr)) return FALSE;

  if (Dc_Attr->flXform & ( PAGE_XLATE_CHANGED   |  // Check for Changes and Updates
                           PAGE_EXTENTS_CHANGED |
                           WORLD_XFORM_CHANGED )
#endif
    return NtGdiTransformPoints( hDC, Points, Points, Count, 0);
#if 0
  else
  {
    for ( i = 0; i < Count; i++ )
      CoordCnvP ( &Dc_Attr->mxWorldToDevice, &Points[i] );
  }
  return TRUE;
#endif
}

