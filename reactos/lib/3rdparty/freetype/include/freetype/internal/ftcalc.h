/***************************************************************************/
/*                                                                         */
/*  ftcalc.h                                                               */
/*                                                                         */
/*    Arithmetic computations (specification).                             */
/*                                                                         */
/*  Copyright 1996-2001, 2002, 2003, 2004, 2005, 2006 by                   */
/*  David Turner, Robert Wilhelm, and Werner Lemberg.                      */
/*                                                                         */
/*  This file is part of the FreeType project, and may only be used,       */
/*  modified, and distributed under the terms of the FreeType project      */
/*  license, LICENSE.TXT.  By continuing to use, modify, or distribute     */
/*  this file you indicate that you have read the license and              */
/*  understand and accept it fully.                                        */
/*                                                                         */
/***************************************************************************/


#ifndef __FTCALC_H__
#define __FTCALC_H__


#include <ft2build.h>
#include FT_FREETYPE_H


FT_BEGIN_HEADER


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    FT_FixedSqrt                                                       */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Computes the square root of a 16.16 fixed point value.             */
  /*                                                                       */
  /* <Input>                                                               */
  /*    x :: The value to compute the root for.                            */
  /*                                                                       */
  /* <Return>                                                              */
  /*    The result of `sqrt(x)'.                                           */
  /*                                                                       */
  /* <Note>                                                                */
  /*    This function is not very fast.                                    */
  /*                                                                       */
  FT_BASE( FT_Int32 )
  FT_SqrtFixed( FT_Int32  x );


#ifdef FT_CONFIG_OPTION_OLD_INTERNALS

  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    FT_Sqrt32                                                          */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Computes the square root of an Int32 integer (which will be        */
  /*    handled as an unsigned long value).                                */
  /*                                                                       */
  /* <Input>                                                               */
  /*    x :: The value to compute the root for.                            */
  /*                                                                       */
  /* <Return>                                                              */
  /*    The result of `sqrt(x)'.                                           */
  /*                                                                       */
  FT_EXPORT( FT_Int32 )
  FT_Sqrt32( FT_Int32  x );

#endif /* FT_CONFIG_OPTION_OLD_INTERNALS */


  /*************************************************************************/
  /*                                                                       */
  /* FT_MulDiv() and FT_MulFix() are declared in freetype.h.               */
  /*                                                                       */
  /*************************************************************************/


#ifdef TT_USE_BYTECODE_INTERPRETER

  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    FT_MulDiv_No_Round                                                 */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A very simple function used to perform the computation `(a*b)/c'   */
  /*    (without rounding) with maximal accuracy (it uses a 64-bit         */
  /*    intermediate integer whenever necessary).                          */
  /*                                                                       */
  /*    This function isn't necessarily as fast as some processor specific */
  /*    operations, but is at least completely portable.                   */
  /*                                                                       */
  /* <Input>                                                               */
  /*    a :: The first multiplier.                                         */
  /*    b :: The second multiplier.                                        */
  /*    c :: The divisor.                                                  */
  /*                                                                       */
  /* <Return>                                                              */
  /*    The result of `(a*b)/c'.  This function never traps when trying to */
  /*    divide by zero; it simply returns `MaxInt' or `MinInt' depending   */
  /*    on the signs of `a' and `b'.                                       */
  /*                                                                       */
  FT_BASE( FT_Long )
  FT_MulDiv_No_Round( FT_Long  a,
                      FT_Long  b,
                      FT_Long  c );

#endif /* TT_USE_BYTECODE_INTERPRETER */


  /*
   *  Return -1, 0, or +1, depending on the orientation of a given corner.
   *  We use the Cartesian coordinate system, with positive vertical values
   *  going upwards.  The function returns +1 if the corner turns to the
   *  left, -1 to the right, and 0 for undecidable cases.
   */
  FT_BASE( FT_Int )
  ft_corner_orientation( FT_Pos  in_x,
                         FT_Pos  in_y,
                         FT_Pos  out_x,
                         FT_Pos  out_y );

  /*
   *  Return TRUE if a corner is flat or nearly flat.  This is equivalent to
   *  saying that the angle difference between the `in' and `out' vectors is
   *  very small.
   */
  FT_BASE( FT_Int )
  ft_corner_is_flat( FT_Pos  in_x,
                     FT_Pos  in_y,
                     FT_Pos  out_x,
                     FT_Pos  out_y );


#define INT_TO_F26DOT6( x )    ( (FT_Long)(x) << 6  )
#define INT_TO_F2DOT14( x )    ( (FT_Long)(x) << 14 )
#define INT_TO_FIXED( x )      ( (FT_Long)(x) << 16 )
#define F2DOT14_TO_FIXED( x )  ( (FT_Long)(x) << 2  )
#define FLOAT_TO_FIXED( x )    ( (FT_Long)( x * 65536.0 ) )

#define ROUND_F26DOT6( x )     ( x >= 0 ? (    ( (x) + 32 ) & -64 )     \
                                        : ( -( ( 32 - (x) ) & -64 ) ) )


FT_END_HEADER

#endif /* __FTCALC_H__ */


/* END */
