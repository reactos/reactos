#include "aftypes.h"

  /* this table was generated for AF_ANGLE_PI = 256 */
#define AF_ANGLE_MAX_ITERS  8
#define AF_TRIG_MAX_ITERS   9

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
      theta = 2 * AF_ANGLE_PI2;
    }

    if ( y > 0 )
      theta = - theta;

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

    /* round theta */
    if ( theta >= 0 )
      theta = FT_PAD_ROUND( theta, 4 );
    else
      theta = - FT_PAD_ROUND( theta, 4 );

    vec->x = x;
    vec->y = theta;
  }


  /* documentation is in fttrigon.h */

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


 /* well, this needs to be somewhere, right :-)
  */

  FT_LOCAL_DEF( void )
  af_sort_pos( FT_UInt   count,
               FT_Pos*   table )
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
