/***************************************************************************/
/*                                                                         */
/*  afangles.c                                                             */
/*                                                                         */
/*    Routines used to compute vector angles with limited accuracy         */
/*    and very high speed.  It also contains sorting routines (body).      */
/*                                                                         */
/*  Copyright 2003, 2004, 2005 by                                          */
/*  David Turner, Robert Wilhelm, and Werner Lemberg.                      */
/*                                                                         */
/*  This file is part of the FreeType project, and may only be used,       */
/*  modified, and distributed under the terms of the FreeType project      */
/*  license, LICENSE.TXT.  By continuing to use, modify, or distribute     */
/*  this file you indicate that you have read the license and              */
/*  understand and accept it fully.                                        */
/*                                                                         */
/***************************************************************************/


#include "aftypes.h"


/*
 * a python script used to generate the following table
 *

import sys, math

units = 256
scale = units/math.pi
comma = ""

print ""
print "table of arctan( 1/2^n ) for PI = " + repr( units / 65536.0 ) + " units"

r = [-1] + range( 32 )

for n in r:
    if n >= 0:
        x = 1.0 / ( 2.0 ** n )   # tangent value
    else:
        x = 2.0 ** ( -n )

    angle  = math.atan( x )      # arctangent
    angle2 = angle * scale       # arctangent in FT_Angle units

    # determine which integer value for angle gives the best tangent
    lo  = int( angle2 )
    hi  = lo + 1
    tlo = math.tan( lo / scale )
    thi = math.tan( hi / scale )

    errlo = abs( tlo - x )
    errhi = abs( thi - x )

    angle2 = hi
    if errlo < errhi:
        angle2 = lo

    if angle2 <= 0:
        break

    sys.stdout.write( comma + repr( int( angle2 ) ) )
    comma = ", "

*
* end of python script
*/


  /* this table was generated for AF_ANGLE_PI = 256 */
#define AF_ANGLE_MAX_ITERS  8
#define AF_TRIG_MAX_ITERS   8

  static const FT_Fixed
  af_angle_arctan_table[9] =
  {
    90, 64, 38, 20, 10, 5, 3, 1, 1
  };


  static FT_Int
  af_angle_prenorm( FT_Vector*  vec )
  {
    FT_Fixed  x, y, z;
    FT_Int    shift;


    x = vec->x;
    y = vec->y;

    z     = ( ( x >= 0 ) ? x : - x ) | ( (y >= 0) ? y : -y );
    shift = 0;

    if ( z < ( 1L << 27 ) )
    {
      do
      {
        shift++;
        z <<= 1;
      } while ( z < ( 1L << 27 ) );

      vec->x = x << shift;
      vec->y = y << shift;
    }
    else if ( z > ( 1L << 28 ) )
    {
      do
      {
        shift++;
        z >>= 1;
      } while ( z > ( 1L << 28 ) );

      vec->x = x >> shift;
      vec->y = y >> shift;
      shift  = -shift;
    }
    return shift;
  }


  static void
  af_angle_pseudo_polarize( FT_Vector*  vec )
  {
    FT_Fixed         theta;
    FT_Fixed         yi, i;
    FT_Fixed         x, y;
    const FT_Fixed  *arctanptr;


    x = vec->x;
    y = vec->y;

    /* Get the vector into the right half plane */
    theta = 0;
    if ( x < 0 )
    {
      x = -x;
      y = -y;
      theta = AF_ANGLE_PI;
    }

    if ( y > 0 )
      theta = -theta;

    arctanptr = af_angle_arctan_table;

    if ( y < 0 )
    {
      /* Rotate positive */
      yi     = y + ( x << 1 );
      x      = x - ( y << 1 );
      y      = yi;
      theta -= *arctanptr++;  /* Subtract angle */
    }
    else
    {
      /* Rotate negative */
      yi     = y - ( x << 1 );
      x      = x + ( y << 1 );
      y      = yi;
      theta += *arctanptr++;  /* Add angle */
    }

    i = 0;
    do
    {
      if ( y < 0 )
      {
        /* Rotate positive */
        yi     = y + ( x >> i );
        x      = x - ( y >> i );
        y      = yi;
        theta -= *arctanptr++;
      }
      else
      {
        /* Rotate negative */
        yi     = y - ( x >> i );
        x      = x + ( y >> i );
        y      = yi;
        theta += *arctanptr++;
      }
    } while ( ++i < AF_TRIG_MAX_ITERS );

#if 0
    /* round theta */
    if ( theta >= 0 )
      theta =  FT_PAD_ROUND( theta, 2 );
    else
      theta = -FT_PAD_ROUND( -theta, 2 );
#endif

    vec->x = x;
    vec->y = theta;
  }


  /* cf. documentation in fttrigon.h */

  FT_LOCAL_DEF( AF_Angle )
  af_angle_atan( FT_Fixed  dx,
                 FT_Fixed  dy )
  {
    FT_Vector  v;


    if ( dx == 0 && dy == 0 )
      return 0;

    v.x = dx;
    v.y = dy;
    af_angle_prenorm( &v );
    af_angle_pseudo_polarize( &v );

    return v.y;
  }


  FT_LOCAL_DEF( AF_Angle )
  af_angle_diff( AF_Angle  angle1,
                 AF_Angle  angle2 )
  {
    AF_Angle  delta = angle2 - angle1;


    delta %= AF_ANGLE_2PI;
    if ( delta < 0 )
      delta += AF_ANGLE_2PI;

    if ( delta > AF_ANGLE_PI )
      delta -= AF_ANGLE_2PI;

    return delta;
  }


  FT_LOCAL_DEF( void )
  af_sort_pos( FT_UInt  count,
               FT_Pos*  table )
  {
    FT_UInt  i, j;
    FT_Pos   swap;


    for ( i = 1; i < count; i++ )
    {
      for ( j = i; j > 0; j-- )
      {
        if ( table[j] > table[j - 1] )
          break;

        swap         = table[j];
        table[j]     = table[j - 1];
        table[j - 1] = swap;
      }
    }
  }


  FT_LOCAL_DEF( void )
  af_sort_widths( FT_UInt   count,
                  AF_Width  table )
  {
    FT_UInt      i, j;
    AF_WidthRec  swap;


    for ( i = 1; i < count; i++ )
    {
      for ( j = i; j > 0; j-- )
      {
        if ( table[j].org > table[j - 1].org )
          break;

        swap         = table[j];
        table[j]     = table[j - 1];
        table[j - 1] = swap;
      }
    }
  }


#ifdef TEST

#include <stdio.h>
#include <math.h>

int main( void )
{
  int  angle;
  int  dist;


  for ( dist = 100; dist < 1000; dist++ )
  {
    for ( angle = AF_ANGLE_PI; angle < AF_ANGLE_2PI * 4; angle++ )
    {
      double  a = ( angle * 3.1415926535 ) / ( 1.0 * AF_ANGLE_PI );
      int     dx, dy, angle1, angle2, delta;


      dx = dist * cos( a );
      dy = dist * sin( a );

      angle1 = ( ( atan2( dy, dx ) * AF_ANGLE_PI ) / 3.1415926535 );
      angle2 = af_angle_atan( dx, dy );
      delta  = ( angle2 - angle1 ) % AF_ANGLE_2PI;
      if ( delta < 0 )
        delta = -delta;

      if ( delta >= 2 )
      {
        printf( "dist:%4d angle:%4d => (%4d,%4d) angle1:%4d angle2:%4d\n",
                dist, angle, dx, dy, angle1, angle2 );
      }
    }
  }
  return 0;
}

#endif /* TEST */


/* END */
