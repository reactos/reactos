/***************************************************************************/
/*                                                                         */
/*  ftcalc.h                                                               */
/*                                                                         */
/*    Arithmetic computations (specification).                             */
/*                                                                         */
/*  Copyright 1996-2000 by                                                 */
/*  David Turner, Robert Wilhelm, and Werner Lemberg.                      */
/*                                                                         */
/*  This file is part of the FreeType project, and may only be used,       */
/*  modified, and distributed under the terms of the FreeType project      */
/*  license, LICENSE.TXT.  By continuing to use, modify, or distribute     */
/*  this file you indicate that you have read the license and              */
/*  understand and accept it fully.                                        */
/*                                                                         */
/***************************************************************************/


#ifndef FTCALC_H
#define FTCALC_H

#include <freetype/freetype.h>
#include <freetype/config/ftconfig.h>   /* for LONG64 */

#ifdef __cplusplus
  extern "C" {
#endif


#ifdef LONG64


  typedef INT64  FT_Int64;

#define ADD_64( x, y, z )  z = (x) + (y)
#define MUL_64( x, y, z )  z = (FT_Int64)(x) * (y)

#define DIV_64( x, y )     ( (x) / (y) )


#ifdef FT_CONFIG_OPTION_OLD_CALCS

#define SQRT_64( z )  FT_Sqrt64( z )

  FT_EXPORT_DEF( FT_Int32 )  FT_Sqrt64( FT_Int64  l );

#endif /* FT_CONFIG_OPTION_OLD_CALCS */


#else /* LONG64 */


  typedef struct  FT_Int64_
  {
    FT_UInt32  lo;
    FT_UInt32  hi;

  } FT_Int64;


#define ADD_64( x, y, z )  FT_Add64( &x, &y, &z )
#define MUL_64( x, y, z )  FT_MulTo64( x, y, &z )
#define DIV_64( x, y )     FT_Div64by32( &x, y )


  FT_EXPORT_DEF( void )  FT_Add64( FT_Int64*  x,
                                   FT_Int64*  y,
                                   FT_Int64*  z );

  FT_EXPORT_DEF( void )  FT_MulTo64( FT_Int32   x,
                                     FT_Int32   y,
                                     FT_Int64*  z );

  FT_EXPORT_DEF( FT_Int32 )  FT_Div64by32( FT_Int64*  x,
                                           FT_Int32   y );


#ifdef FT_CONFIG_OPTION_OLD_CALCS

#define SQRT_64( z )  FT_Sqrt64( &z )

  FT_EXPORT_DEF( FT_Int32 )  FT_Sqrt64( FT_Int64*  x );

#endif /* FT_CONFIG_OPTION_OLD_CALCS */


#endif /* LONG64 */


#ifndef FT_CONFIG_OPTION_OLD_CALCS

#define SQRT_32( x )  FT_Sqrt32( x )

  BASE_DEF( FT_Int32 )  FT_Sqrt32( FT_Int32  x );

#endif /* !FT_CONFIG_OPTION_OLD_CALCS */


  /*************************************************************************/
  /*                                                                       */
  /* FT_MulDiv() and FT_MulFix() are declared in freetype.h.               */
  /*                                                                       */
  /*************************************************************************/


#define INT_TO_F26DOT6( x )    ( (FT_Long)(x) << 6  )
#define INT_TO_F2DOT14( x )    ( (FT_Long)(x) << 14 )
#define INT_TO_FIXED( x )      ( (FT_Long)(x) << 16 )
#define F2DOT14_TO_FIXED( x )  ( (FT_Long)(x) << 2  )
#define FLOAT_TO_FIXED( x )    ( (FT_Long)( x * 65536.0 ) )

#define ROUND_F26DOT6( x )     ( x >= 0 ? (    ( (x) + 32 ) & -64 )     \
                                        : ( -( ( 32 - (x) ) & -64 ) ) )


#ifdef __cplusplus
  }
#endif

#endif /* FTCALC_H */


/* END */
