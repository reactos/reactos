#include <ft2build.h>
#include FT_STROKER_H
#include FT_TRIGONOMETRY_H
#include FT_INTERNAL_MEMORY_H
#include FT_INTERNAL_DEBUG_H

 /***************************************************************************/
 /***************************************************************************/
 /*****                                                                 *****/
 /*****                       BEZIER COMPUTATIONS                       *****/
 /*****                                                                 *****/
 /***************************************************************************/
 /***************************************************************************/

#define FT_SMALL_CONIC_THRESHOLD   (FT_ANGLE_PI/6)
#define FT_SMALL_CUBIC_THRESHOLD   (FT_ANGLE_PI/6)
#define FT_EPSILON  2

#define FT_IS_SMALL(x)  ((x) > -FT_EPSILON && (x) < FT_EPSILON)

  static FT_Pos
  ft_pos_abs( FT_Pos  x )
  {
    return  x >= 0 ? x : -x ;
  }

  static void
  ft_conic_split( FT_Vector*  base )
  {
    FT_Pos  a, b;


    base[4].x = base[2].x;
    b = base[1].x;
    a = base[3].x = ( base[2].x + b )/2;
    b = base[1].x = ( base[0].x + b )/2;
    base[2].x = ( a + b )/2;

    base[4].y = base[2].y;
    b = base[1].y;
    a = base[3].y = ( base[2].y + b )/2;
    b = base[1].y = ( base[0].y + b )/2;
    base[2].y = ( a + b )/2;
  }


  static FT_Bool
  ft_conic_is_small_enough( FT_Vector*  base,
                            FT_Angle   *angle_in,
                            FT_Angle   *angle_out )
  {
    FT_Vector  d1, d2;
    FT_Angle   theta;
    FT_Int     close1, close2;

    d1.x = base[1].x - base[2].x;
    d1.y = base[1].y - base[2].y;
    d2.x = base[0].x - base[1].x;
    d2.y = base[0].y - base[1].y;

    close1 = FT_IS_SMALL(d1.x) && FT_IS_SMALL(d1.y);
    close2 = FT_IS_SMALL(d2.x) && FT_IS_SMALL(d2.y);

    if (close1)
    {
      if (close2)
        *angle_in = *angle_out = 0;
      else
        *angle_in = *angle_out = FT_Atan2( d2.x, d2.y );
    }
    else if (close2)
    {
      *angle_in = *angle_out = FT_Atan2( d1.x, d1.y );
    }
    else
    {
      *angle_in  = FT_Atan2( d1.x, d1.y );
      *angle_out = FT_Atan2( d2.x, d2.y );
    }

    theta = ft_pos_abs( FT_Angle_Diff( *angle_in, *angle_out ) );

    return FT_BOOL( theta < FT_SMALL_CONIC_THRESHOLD );
  }


  static void
  ft_cubic_split( FT_Vector*  base )
  {
    FT_Pos  a, b, c, d;


    base[6].x = base[3].x;
    c = base[1].x;
    d = base[2].x;
    base[1].x = a = ( base[0].x + c )/2;
    base[5].x = b = ( base[3].x + d )/2;
    c = ( c + d )/2;
    base[2].x = a = ( a + c )/2;
    base[4].x = b = ( b + c )/2;
    base[3].x = ( a + b )/2;

    base[6].y = base[3].y;
    c = base[1].y;
    d = base[2].y;
    base[1].y = a = ( base[0].y + c )/2;
    base[5].y = b = ( base[3].y + d )/2;
    c = ( c + d )/2;
    base[2].y = a = ( a + c )/2;
    base[4].y = b = ( b + c )/2;
    base[3].y = ( a + b )/2;
  }


  static FT_Bool
  ft_cubic_is_small_enough( FT_Vector*  base,
                            FT_Angle   *angle_in,
                            FT_Angle   *angle_mid,
                            FT_Angle   *angle_out )
  {
    FT_Vector  d1, d2, d3;
    FT_Angle   theta1, theta2;
    FT_Int     close1, close2, close3;

    d1.x = base[2].x - base[3].x;
    d1.y = base[2].y - base[3].y;
    d2.x = base[1].x - base[2].x;
    d2.y = base[1].y - base[2].y;
    d3.x = base[0].x - base[1].x;
    d3.y = base[0].y - base[1].y;

    close1 = FT_IS_SMALL(d1.x) && FT_IS_SMALL(d1.y);
    close2 = FT_IS_SMALL(d2.x) && FT_IS_SMALL(d2.y);
    close3 = FT_IS_SMALL(d3.x) && FT_IS_SMALL(d3.y);

    if (close1 || close3)
    {
      if (close2)
      {
        /* basically a point */
        *angle_in = *angle_out = *angle_mid = 0;
      }
      else if (close1)
      {
        *angle_in  = *angle_mid = FT_Atan2( d2.x, d2.y );
        *angle_out = FT_Atan2( d3.x, d3.y );
      }
      else  /* close2 */
      {
        *angle_in  = FT_Atan2( d1.x, d1.y );
        *angle_mid = *angle_out = FT_Atan2( d2.x, d2.y );
      }
    }
    else if (close2)
    {
      *angle_in  = *angle_mid = FT_Atan2( d1.x, d1.y );
      *angle_out = FT_Atan2( d3.x, d3.y );
    }
    else
    {
      *angle_in  = FT_Atan2( d1.x, d1.y );
      *angle_mid = FT_Atan2( d2.x, d2.y );
      *angle_out = FT_Atan2( d3.x, d3.y );
    }
    theta1 = ft_pos_abs( FT_Angle_Diff( *angle_in,  *angle_mid ) );
    theta2 = ft_pos_abs( FT_Angle_Diff( *angle_mid, *angle_out ) );

    return FT_BOOL( theta1 < FT_SMALL_CUBIC_THRESHOLD &&
                    theta2 < FT_SMALL_CUBIC_THRESHOLD );
  }



 /***************************************************************************/
 /***************************************************************************/
 /*****                                                                 *****/
 /*****                       STROKE BORDERS                            *****/
 /*****                                                                 *****/
 /***************************************************************************/
 /***************************************************************************/

  typedef enum
  {
    FT_STROKE_TAG_ON    = 1,   /* on-curve point  */
    FT_STROKE_TAG_CUBIC = 2,   /* cubic off-point */
    FT_STROKE_TAG_BEGIN = 4,   /* sub-path start  */
    FT_STROKE_TAG_END   = 8    /* sub-path end    */

  } FT_StrokeTags;


  typedef struct FT_StrokeBorderRec_
  {
    FT_UInt     num_points;
    FT_UInt     max_points;
    FT_Vector*  points;
    FT_Byte*    tags;
    FT_Bool     movable;
    FT_Int      start;    /* index of current sub-path start point */
    FT_Memory   memory;

  } FT_StrokeBorderRec, *FT_StrokeBorder;


  static FT_Error
  ft_stroke_border_grow( FT_StrokeBorder  border,
                         FT_UInt          new_points )
  {
    FT_UInt   old_max = border->max_points;
    FT_UInt   new_max = border->num_points + new_points;
    FT_Error  error   = 0;

    if ( new_max > old_max )
    {
      FT_UInt    cur_max = old_max;
      FT_Memory  memory  = border->memory;

      while ( cur_max < new_max )
        cur_max += (cur_max >> 1) + 16;

      if ( FT_RENEW_ARRAY( border->points, old_max, cur_max ) ||
           FT_RENEW_ARRAY( border->tags,   old_max, cur_max ) )
        goto Exit;

      border->max_points = cur_max;
    }
  Exit:
    return error;
  }

  static void
  ft_stroke_border_close( FT_StrokeBorder  border )
  {
    FT_ASSERT( border->start >= 0 );

    border->tags[ border->start        ] |= FT_STROKE_TAG_BEGIN;
    border->tags[ border->num_points-1 ] |= FT_STROKE_TAG_END;

    border->start   = -1;
    border->movable = 0;
  }


  static FT_Error
  ft_stroke_border_lineto( FT_StrokeBorder  border,
                           FT_Vector*       to,
                           FT_Bool          movable )
  {
    FT_Error  error = 0;

    FT_ASSERT( border->start >= 0 );

    if ( border->movable )
    {
      /* move last point */
      border->points[ border->num_points-1 ] = *to;
    }
    else
    {
      /* add one point */
      error = ft_stroke_border_grow( border, 1 );
      if (!error)
      {
        FT_Vector*  vec = border->points + border->num_points;
        FT_Byte*    tag = border->tags   + border->num_points;

        vec[0] = *to;
        tag[0] = FT_STROKE_TAG_ON;

        border->num_points += 1;
      }
    }
    border->movable = movable;
    return error;
  }


  static FT_Error
  ft_stroke_border_conicto( FT_StrokeBorder  border,
                            FT_Vector*       control,
                            FT_Vector*       to )
  {
    FT_Error  error;

    FT_ASSERT( border->start >= 0 );

    error = ft_stroke_border_grow( border, 2 );
    if (!error)
    {
      FT_Vector*  vec = border->points + border->num_points;
      FT_Byte*    tag = border->tags   + border->num_points;

      vec[0] = *control;
      vec[1] = *to;

      tag[0] = 0;
      tag[1] = FT_STROKE_TAG_ON;

      border->num_points += 2;
    }
    border->movable = 0;
    return error;
  }


  static FT_Error
  ft_stroke_border_cubicto( FT_StrokeBorder  border,
                            FT_Vector*       control1,
                            FT_Vector*       control2,
                            FT_Vector*       to )
  {
    FT_Error  error;

    FT_ASSERT( border->start >= 0 );

    error = ft_stroke_border_grow( border, 3 );
    if (!error)
    {
      FT_Vector*  vec = border->points + border->num_points;
      FT_Byte*    tag = border->tags   + border->num_points;

      vec[0] = *control1;
      vec[1] = *control2;
      vec[2] = *to;

      tag[0] = FT_STROKE_TAG_CUBIC;
      tag[1] = FT_STROKE_TAG_CUBIC;
      tag[2] = FT_STROKE_TAG_ON;

      border->num_points += 3;
    }
    border->movable = 0;
    return error;
  }


#define  FT_ARC_CUBIC_ANGLE  (FT_ANGLE_PI/2)


  static FT_Error
  ft_stroke_border_arcto( FT_StrokeBorder  border,
                          FT_Vector*       center,
                          FT_Fixed         radius,
                          FT_Angle         angle_start,
                          FT_Angle         angle_diff )
  {
    FT_Angle   total, angle, step, rotate, next, theta;
    FT_Vector  a, b, a2, b2;
    FT_Fixed   length;
    FT_Error   error = 0;

    /* compute start point */
    FT_Vector_From_Polar( &a, radius, angle_start );
    a.x += center->x;
    a.y += center->y;

    total  = angle_diff;
    angle  = angle_start;
    rotate = ( angle_diff >= 0 ) ? FT_ANGLE_PI2 : -FT_ANGLE_PI2;

    while (total != 0)
    {
      step = total;
      if ( step > FT_ARC_CUBIC_ANGLE )
        step   = FT_ARC_CUBIC_ANGLE;

      else if ( step < -FT_ARC_CUBIC_ANGLE )
        step = -FT_ARC_CUBIC_ANGLE;

      next  = angle + step;
      theta = step;
      if ( theta < 0 )
        theta = -theta;

      theta >>= 1;

      /* compute end point */
      FT_Vector_From_Polar( &b, radius, next );
      b.x += center->x;
      b.y += center->y;

      /* compute first and second control points */
      length = FT_MulDiv( radius, FT_Sin(theta)*4,
                          (0x10000L + FT_Cos(theta))*3 );

      FT_Vector_From_Polar( &a2, length, angle + rotate );
      a2.x += a.x;
      a2.y += a.y;

      FT_Vector_From_Polar( &b2, length, next - rotate );
      b2.x += b.x;
      b2.y += b.y;

      /* add cubic arc */
      error = ft_stroke_border_cubicto( border, &a2, &b2, &b );
      if (error) break;

      /* process the rest of the arc ?? */
      a      = b;
      total -= step;
      angle  = next;
    }
    return error;
  }


  static FT_Error
  ft_stroke_border_moveto( FT_StrokeBorder  border,
                           FT_Vector*       to )
  {
    /* close current open path if any ? */
    if ( border->start >= 0 )
      ft_stroke_border_close( border );

    border->start   = border->num_points;
    border->movable = 0;

    return ft_stroke_border_lineto( border, to, 0 );
  }


 static void
 ft_stroke_border_init( FT_StrokeBorder  border,
                        FT_Memory        memory )
 {
   border->memory = memory;
   border->points = NULL;
   border->tags   = NULL;

   border->num_points = 0;
   border->max_points = 0;
   border->start      = -1;
 }


 static void
 ft_stroke_border_reset( FT_StrokeBorder  border )
 {
   border->num_points = 0;
   border->start      = -1;
 }


 static void
 ft_stroke_border_done( FT_StrokeBorder  border )
 {
   FT_Memory  memory = border->memory;

   FT_FREE( border->points );
   FT_FREE( border->tags );

   border->num_points = 0;
   border->max_points = 0;
   border->start      = -1;
 }


 static FT_Error
 ft_stroke_border_get_counts( FT_StrokeBorder  border,
                              FT_UInt         *anum_points,
                              FT_UInt         *anum_contours )
 {
   FT_Error  error        = 0;
   FT_UInt   num_points   = 0;
   FT_UInt   num_contours = 0;

   FT_UInt     count      = border->num_points;
   FT_Vector*  point      = border->points;
   FT_Byte*    tags       = border->tags;
   FT_Int      in_contour = 0;

   for ( ; count > 0; count--, point++, tags++ )
   {
     if ( tags[0] & FT_STROKE_TAG_BEGIN )
     {
       if ( in_contour != 0 )
         goto Fail;

       in_contour = 1;
     }
     else if ( in_contour == 0 )
       goto Fail;

     if ( tags[0] & FT_STROKE_TAG_END )
     {
       if ( in_contour == 0 )
         goto Fail;

       in_contour = 0;
       num_contours++;
     }
   }
   if ( in_contour != 0 )
     goto Fail;

 Exit:
   *anum_points   = num_points;
   *anum_contours = num_contours;
   return error;

 Fail:
   num_points   = 0;
   num_contours = 0;
   goto Exit;
 }


 static void
 ft_stroke_border_export( FT_StrokeBorder  border,
                          FT_Outline*      outline )
 {
   /* copy point locations */
   FT_MEM_COPY( outline->points + outline->n_points,
                border->points,
                border->num_points * sizeof(FT_Vector) );

   /* copy tags */
   {
     FT_UInt   count = border->num_points;
     FT_Byte*  read  = border->tags;
     FT_Byte*  write = (FT_Byte*) outline->tags + outline->n_points;

     for ( ; count > 0; count--, read++, write++ )
     {
       if ( *read & FT_STROKE_TAG_ON )
         *write = FT_CURVE_TAG_ON;
       else if ( *read & FT_STROKE_TAG_CUBIC )
         *write = FT_CURVE_TAG_CUBIC;
       else
         *write = FT_CURVE_TAG_CONIC;
     }
   }

   /* copy contours */
   {
     FT_UInt    count = border->num_points;
     FT_Byte*   tags  = border->tags;
     FT_Short*  write = outline->contours + outline->n_contours;
     FT_Short   index = (FT_Short) outline->n_points;

     for ( ; count > 0; count--, tags++, write++, index++ )
     {
       if ( *tags & FT_STROKE_TAG_END )
       {
         *write++ = index;
         outline->n_contours++;
       }
     }
   }

   outline->n_points  = (short)( outline->n_points + border->num_points );

   FT_ASSERT( FT_Outline_Check( outline ) == 0 );
 }


 /***************************************************************************/
 /***************************************************************************/
 /*****                                                                 *****/
 /*****                           STROKER                               *****/
 /*****                                                                 *****/
 /***************************************************************************/
 /***************************************************************************/

#define  FT_SIDE_TO_ROTATE(s)   (FT_ANGLE_PI2 - (s)*FT_ANGLE_PI)

  typedef struct FT_StrokerRec_
  {
    FT_Angle             angle_in;
    FT_Angle             angle_out;
    FT_Vector            center;
    FT_Bool              first_point;
    FT_Bool              subpath_open;
    FT_Angle             subpath_angle;
    FT_Vector            subpath_start;

    FT_Stroker_LineCap   line_cap;
    FT_Stroker_LineJoin  line_join;
    FT_Fixed             miter_limit;
    FT_Fixed             radius;

    FT_Bool              valid;
    FT_StrokeBorderRec   borders[2];
    FT_Memory            memory;

  } FT_StrokerRec;


  FT_EXPORT_DEF( FT_Error )
  FT_Stroker_New( FT_Memory    memory,
                  FT_Stroker  *astroker )
  {
    FT_Error    error;
    FT_Stroker  stroker;

    if ( !FT_NEW( stroker ) )
    {
      stroker->memory = memory;

      ft_stroke_border_init( &stroker->borders[0], memory );
      ft_stroke_border_init( &stroker->borders[1], memory );
    }
    *astroker = stroker;
    return error;
  }


  FT_EXPORT_DEF( void )
  FT_Stroker_Set( FT_Stroker           stroker,
                  FT_Fixed             radius,
                  FT_Stroker_LineCap   line_cap,
                  FT_Stroker_LineJoin  line_join,
                  FT_Fixed             miter_limit )
  {
    stroker->radius      = radius;
    stroker->line_cap    = line_cap;
    stroker->line_join   = line_join;
    stroker->miter_limit = miter_limit;

    stroker->valid = 0;

    ft_stroke_border_reset( &stroker->borders[0] );
    ft_stroke_border_reset( &stroker->borders[1] );
  }


  FT_EXPORT_DEF( void )
  FT_Stroker_Done( FT_Stroker  stroker )
  {
    if ( stroker )
    {
      FT_Memory  memory = stroker->memory;

      ft_stroke_border_done( &stroker->borders[0] );
      ft_stroke_border_done( &stroker->borders[1] );

      stroker->memory = NULL;
      FT_FREE( stroker );
    }
  }



 /* creates a circular arc at a corner or cap */
  static FT_Error
  ft_stroker_arcto( FT_Stroker  stroker,
                    FT_Int      side )
  {
    FT_Angle          total, rotate;
    FT_Fixed          radius = stroker->radius;
    FT_Error          error  = 0;
    FT_StrokeBorder   border = stroker->borders + side;

    rotate = FT_SIDE_TO_ROTATE(side);

    total = FT_Angle_Diff( stroker->angle_in, stroker->angle_out );
    if (total == FT_ANGLE_PI)
      total = -rotate*2;

    error = ft_stroke_border_arcto( border,
                                    &stroker->center,
                                    radius,
                                    stroker->angle_in + rotate,
                                    total );
    border->movable = 0;
    return error;
  }


  /* adds a cap at the end of an opened path */
  static FT_Error
  ft_stroker_cap( FT_Stroker  stroker,
                  FT_Angle    angle,
                  FT_Int      side )
  {
    FT_Error  error  = 0;

    if ( stroker->line_cap == FT_STROKER_LINECAP_ROUND )
    {
      /* add a round cap */
      stroker->angle_in  = angle;
      stroker->angle_out = angle + FT_ANGLE_PI;
      error = ft_stroker_arcto( stroker, side );
    }
    else if ( stroker->line_cap == FT_STROKER_LINECAP_SQUARE )
    {
      /* add a square cap */
      FT_Vector         delta, delta2;
      FT_Angle          rotate = FT_SIDE_TO_ROTATE(side);
      FT_Fixed          radius = stroker->radius;
      FT_StrokeBorder   border = stroker->borders + side;

      FT_Vector_From_Polar( &delta2, radius, angle+rotate );
      FT_Vector_From_Polar( &delta,  radius, angle );

      delta.x += stroker->center.x + delta2.x;
      delta.y += stroker->center.y + delta2.y;

      error = ft_stroke_border_lineto( border, &delta, 0 );
      if (error) goto Exit;

      FT_Vector_From_Polar( &delta2, radius, angle-rotate );
      FT_Vector_From_Polar( &delta,  radius, angle );

      delta.x += delta2.x + stroker->center.x;
      delta.y += delta2.y + stroker->center.y;

      error = ft_stroke_border_lineto( border, &delta, 0 );
    }
  Exit:
    return error;
  }



 /* process an inside corner, i.e. compute intersection */
  static FT_Error
  ft_stroker_inside( FT_Stroker  stroker,
                     FT_Int      side)
  {
    FT_StrokeBorder  border = stroker->borders + side;
    FT_Angle         phi, theta, rotate;
    FT_Fixed         length, thcos, sigma;
    FT_Vector        delta;
    FT_Error         error = 0;


    rotate = FT_SIDE_TO_ROTATE(side);

    /* compute median angle */
    theta = FT_Angle_Diff( stroker->angle_in, stroker->angle_out );
    if ( theta == FT_ANGLE_PI )
      theta = rotate;
    else
      theta = theta/2;

    phi = stroker->angle_in + theta;

    thcos  = FT_Cos( theta );
    sigma  = FT_MulFix( stroker->miter_limit, thcos );

    if ( sigma < 0x10000L )
    {
      FT_Vector_From_Polar( &delta, stroker->radius, stroker->angle_out + rotate );
      delta.x += stroker->center.x;
      delta.y += stroker->center.y;
      border->movable = 0;
    }
    else
    {
      length = FT_DivFix( stroker->radius, thcos );

      FT_Vector_From_Polar( &delta, length, phi + rotate );
      delta.x += stroker->center.x;
      delta.y += stroker->center.y;
    }

    error = ft_stroke_border_lineto( border, &delta, 0 );

    return error;
  }


 /* process an outside corner, i.e. compute bevel/miter/round */
  static FT_Error
  ft_stroker_outside( FT_Stroker  stroker,
                      FT_Int      side )
  {
    FT_StrokeBorder  border = stroker->borders + side;
    FT_Error         error;
    FT_Angle         rotate;

    if ( stroker->line_join == FT_STROKER_LINEJOIN_ROUND )
    {
      error = ft_stroker_arcto( stroker, side );
    }
    else
    {
      /* this is a mitered or beveled corner */
      FT_Fixed   sigma, radius = stroker->radius;
      FT_Angle   theta, phi;
      FT_Fixed   thcos;
      FT_Bool    miter;

      rotate = FT_SIDE_TO_ROTATE(side);
      miter  = FT_BOOL( stroker->line_join == FT_STROKER_LINEJOIN_MITER );

      theta  = FT_Angle_Diff( stroker->angle_in, stroker->angle_out );
      if (theta == FT_ANGLE_PI)
        theta = rotate;
      else
        theta = theta/2;

      thcos  = FT_Cos( theta );
      sigma  = FT_MulFix( stroker->miter_limit, thcos );

      if ( sigma >= 0x10000L )
        miter = 0;

      phi = stroker->angle_in + theta + rotate;

      if (miter)  /* this is a miter (broken angle) */
      {
        FT_Vector  middle, delta;
        FT_Fixed  length;

        /* compute middle point */
        FT_Vector_From_Polar( &middle, FT_MulFix( radius, stroker->miter_limit ),
                              phi );
        middle.x += stroker->center.x;
        middle.y += stroker->center.y;

        /* compute first angle point */
        length = FT_MulFix( radius, FT_DivFix( 0x10000L - sigma,
                                    ft_pos_abs( FT_Sin( theta ) ) ) );

        FT_Vector_From_Polar( &delta, length, phi + rotate );
        delta.x += middle.x;
        delta.y += middle.y;

        error = ft_stroke_border_lineto( border, &delta, 0 );
        if (error) goto Exit;

        /* compute second angle point */
        FT_Vector_From_Polar( &delta, length, phi - rotate );
        delta.x += middle.x;
        delta.y += middle.y;

        error = ft_stroke_border_lineto( border, &delta, 0 );
        if (error) goto Exit;

        /* finally, add a movable end point */
        FT_Vector_From_Polar( &delta, radius, stroker->angle_out + rotate );
        delta.x += stroker->center.x;
        delta.y += stroker->center.y;

        error = ft_stroke_border_lineto( border, &delta, 1 );
      }
      else /* this is a bevel (intersection) */
      {
        FT_Fixed  length;
        FT_Vector  delta;

        length = FT_DivFix( stroker->radius, thcos );

        FT_Vector_From_Polar( &delta, length, phi );
        delta.x += stroker->center.x;
        delta.y += stroker->center.y;

        error = ft_stroke_border_lineto( border, &delta, 0 );
        if (error) goto Exit;

        /* now add end point */
        FT_Vector_From_Polar( &delta, stroker->radius, stroker->angle_out + rotate );
        delta.x += stroker->center.x;
        delta.y += stroker->center.y;

        error = ft_stroke_border_lineto( border, &delta, 1 );
      }
    }
  Exit:
    return error;
  }


  static FT_Error
  ft_stroker_process_corner( FT_Stroker  stroker )
  {
    FT_Error  error = 0;
    FT_Angle  turn;
    FT_Int    inside_side;

    turn = FT_Angle_Diff( stroker->angle_in, stroker->angle_out );

    /* no specific corner processing is required if the turn is 0 */
    if (turn == 0)
      goto Exit;

    /* when we turn to the right, the inside side is 0 */
    inside_side = 0;

    /* otherwise, the inside side is 1 */
    if (turn < 0)
      inside_side = 1;

    /* process the inside side */
    error = ft_stroker_inside( stroker, inside_side );
    if (error) goto Exit;

    /* process the outside side */
    error = ft_stroker_outside( stroker, 1-inside_side );

  Exit:
    return error;
  }


 /* add two points to the left and right borders corresponding to the */
 /* start of the subpath..                                            */
  static FT_Error
  ft_stroker_subpath_start( FT_Stroker  stroker,
                            FT_Angle    start_angle )
  {
    FT_Vector        delta;
    FT_Vector        point;
    FT_Error         error;
    FT_StrokeBorder  border;

    FT_Vector_From_Polar( &delta, stroker->radius, start_angle + FT_ANGLE_PI2 );

    point.x = stroker->center.x + delta.x;
    point.y = stroker->center.y + delta.y;

    border = stroker->borders;
    error = ft_stroke_border_moveto( border, &point );
    if (error) goto Exit;

    point.x = stroker->center.x - delta.x;
    point.y = stroker->center.y - delta.y;

    border++;
    error = ft_stroke_border_moveto( border, &point );

    /* save angle for last cap */
    stroker->subpath_angle = start_angle;
    stroker->first_point   = 0;

  Exit:
    return error;
  }


  FT_EXPORT_DEF( FT_Error )
  FT_Stroker_LineTo( FT_Stroker  stroker,
                     FT_Vector*  to )
  {
    FT_Error         error = 0;
    FT_StrokeBorder  border;
    FT_Vector        delta;
    FT_Angle         angle;
    FT_Int           side;

    delta.x = to->x - stroker->center.x;
    delta.y = to->y - stroker->center.y;

    angle = FT_Atan2( delta.x, delta.y );
    FT_Vector_From_Polar( &delta, stroker->radius, angle + FT_ANGLE_PI2 );

    /* process corner if necessary */
    if ( stroker->first_point )
    {
      /* this is the first segment of a subpath. We need to      */
      /* add a point to each border at their respective starting */
      /* point locations..                                       */
      error = ft_stroker_subpath_start( stroker, angle );
      if (error) goto Exit;
    }
    else
    {
      /* process the current corner */
      stroker->angle_out = angle;
      error = ft_stroker_process_corner( stroker );
      if (error) goto Exit;
    }

    /* now add a line segment to both the "inside" and "outside" paths */

    for ( border = stroker->borders, side = 1; side >= 0; side--, border++ )
    {
      FT_Vector  point;

      point.x = to->x + delta.x;
      point.y = to->y + delta.y;

      error = ft_stroke_border_lineto( border, &point, 1 );
      if (error) goto Exit;

      delta.x = -delta.x;
      delta.y = -delta.y;
    }

    stroker->angle_in  = angle;
    stroker->center    = *to;

  Exit:
    return error;
  }



  FT_EXPORT_DEF( FT_Error )
  FT_Stroker_ConicTo( FT_Stroker  stroker,
                      FT_Vector*  control,
                      FT_Vector*  to )
  {
    FT_Error    error = 0;
    FT_Vector   bez_stack[34];
    FT_Vector*  arc;
    FT_Vector*  limit = bez_stack + 30;
    FT_Angle    start_angle;
    FT_Bool     first_arc = 1;

    arc    = bez_stack;
    arc[0] = *to;
    arc[1] = *control;
    arc[2] = stroker->center;

    while ( arc >= bez_stack )
    {
      FT_Angle  angle_in, angle_out;

      angle_in = angle_out = 0;  /* remove compiler warnings */

      if ( arc < limit &&
           !ft_conic_is_small_enough( arc, &angle_in, &angle_out ) )
      {
        ft_conic_split( arc );
        arc += 2;
        continue;
      }

      if ( first_arc )
      {
        first_arc = 0;

        start_angle = angle_in;

        /* process corner if necessary */
        if ( stroker->first_point )
          error = ft_stroker_subpath_start( stroker, start_angle );
        else
        {
          stroker->angle_out = start_angle;
          error = ft_stroker_process_corner( stroker );
        }
      }

      /* the arc's angle is small enough, we can add it directly to each */
      /* border..                                                        */
      {
        FT_Vector  ctrl, end;
        FT_Angle   theta, phi, rotate;
        FT_Fixed  length;
        FT_Int     side;

        theta  = FT_Angle_Diff( angle_in, angle_out )/2;
        phi    = angle_in + theta;
        length = FT_DivFix( stroker->radius, FT_Cos(theta) );

        for ( side = 0; side <= 1; side++ )
        {
          rotate = FT_SIDE_TO_ROTATE(side);

          /* compute control point */
          FT_Vector_From_Polar( &ctrl, length, phi + rotate );
          ctrl.x += arc[1].x;
          ctrl.y += arc[1].y;

          /* compute end point */
          FT_Vector_From_Polar( &end, stroker->radius, angle_out + rotate );
          end.x += arc[0].x;
          end.y += arc[0].y;

          error = ft_stroke_border_conicto( stroker->borders + side, &ctrl, &end );
          if (error) goto Exit;
        }
      }

      arc -= 2;

      if (arc < bez_stack)
        stroker->angle_in = angle_out;
    }

    stroker->center = *to;

  Exit:
    return error;
  }



  FT_EXPORT_DEF( FT_Error )
  FT_Stroker_CubicTo( FT_Stroker  stroker,
                      FT_Vector*      control1,
                      FT_Vector*      control2,
                      FT_Vector*      to )
  {
    FT_Error    error = 0;
    FT_Vector   bez_stack[37];
    FT_Vector*  arc;
    FT_Vector*  limit = bez_stack + 32;
    FT_Angle    start_angle;
    FT_Bool     first_arc = 1;

    arc    = bez_stack;
    arc[0] = *to;
    arc[1] = *control2;
    arc[2] = *control1;
    arc[3] = stroker->center;

    while ( arc >= bez_stack )
    {
      FT_Angle  angle_in, angle_mid, angle_out;

      /* remove compiler warnings */
      angle_in = angle_out = angle_mid = 0;

      if ( arc < limit &&
           !ft_cubic_is_small_enough( arc, &angle_in, &angle_mid, &angle_out ) )
      {
        ft_cubic_split( arc );
        arc += 3;
        continue;
      }

      if ( first_arc )
      {
        first_arc = 0;

        /* process corner if necessary */
        start_angle = angle_in;

        if ( stroker->first_point )
          error = ft_stroker_subpath_start( stroker, start_angle );
        else
            {
              stroker->angle_out = start_angle;
              error = ft_stroker_process_corner( stroker );
            }
        if (error) goto Exit;
      }

      /* the arc's angle is small enough, we can add it directly to each */
      /* border..                                                        */
      {
        FT_Vector  ctrl1, ctrl2, end;
        FT_Angle   theta1, phi1, theta2, phi2, rotate;
        FT_Fixed   length1, length2;
        FT_Int     side;

        theta1  = ft_pos_abs( angle_mid - angle_in )/2;
        theta2  = ft_pos_abs( angle_out - angle_mid )/2;
        phi1    = (angle_mid+angle_in)/2;
        phi2    = (angle_mid+angle_out)/2;
        length1 = FT_DivFix( stroker->radius, FT_Cos(theta1) );
        length2 = FT_DivFix( stroker->radius, FT_Cos(theta2) );

        for ( side = 0; side <= 1; side++ )
        {
          rotate = FT_SIDE_TO_ROTATE(side);

          /* compute control points */
          FT_Vector_From_Polar( &ctrl1, length1, phi1 + rotate );
          ctrl1.x += arc[2].x;
          ctrl1.y += arc[2].y;

          FT_Vector_From_Polar( &ctrl2, length2, phi2 + rotate );
          ctrl2.x += arc[1].x;
          ctrl2.y += arc[1].y;

          /* compute end point */
          FT_Vector_From_Polar( &end, stroker->radius, angle_out + rotate );
          end.x += arc[0].x;
          end.y += arc[0].y;

          error = ft_stroke_border_cubicto( stroker->borders + side, &ctrl1, &ctrl2, &end );
          if (error) goto Exit;
        }
      }

      arc -= 3;
      if (arc < bez_stack)
        stroker->angle_in = angle_out;
    }

    stroker->center = *to;

  Exit:
    return error;
  }


  FT_EXPORT_DEF( FT_Error )
  FT_Stroker_BeginSubPath( FT_Stroker  stroker,
                           FT_Vector*  to,
                           FT_Bool     open )
  {
    /* we cannot process the first point, because there is not enough     */
    /* information regarding its corner/cap. The latter will be processed */
    /* in the "end_subpath" routine                                       */
    /*                                                                    */
    stroker->first_point   = 1;
    stroker->center        = *to;
    stroker->subpath_open  = open;

    /* record the subpath start point index for each border */
    stroker->subpath_start = *to;
    return 0;
  }


  static
  FT_Error  ft_stroker_add_reverse_left( FT_Stroker  stroker,
                                         FT_Bool     open )
  {
    FT_StrokeBorder  right  = stroker->borders + 0;
    FT_StrokeBorder  left   = stroker->borders + 1;
    FT_Int           new_points;
    FT_Error         error  = 0;

    FT_ASSERT( left->start >= 0 );

    new_points = left->num_points - left->start;
    if ( new_points > 0 )
    {
      error = ft_stroke_border_grow( right, (FT_UInt)new_points );
      if (error) goto Exit;
      {
        FT_Vector*    dst_point = right->points + right->num_points;
        FT_Byte*      dst_tag   = right->tags   + right->num_points;
        FT_Vector*    src_point = left->points  + left->num_points - 1;
        FT_Byte*      src_tag   = left->tags    + left->num_points - 1;

        while ( src_point >= left->points + left->start )
        {
          *dst_point = *src_point;
          *dst_tag   = *src_tag;

          if (open)
            dst_tag[0] &= ~(FT_STROKE_TAG_BEGIN | FT_STROKE_TAG_END);
          else
          {
            /* switch begin/end tags if necessary.. */
            if (dst_tag[0] & (FT_STROKE_TAG_BEGIN | FT_STROKE_TAG_END))
              dst_tag[0] ^= (FT_STROKE_TAG_BEGIN | FT_STROKE_TAG_END);
          }

          src_point--;
          src_tag--;
          dst_point++;
          dst_tag++;
        }
      }
      left->num_points   = left->start;
      right->num_points += new_points;

      right->movable = 0;
      left->movable  = 0;
    }
  Exit:
    return error;
  }


 /* there's a lot of magic in this function !! */
  FT_EXPORT_DEF( FT_Error )
  FT_Stroker_EndSubPath( FT_Stroker  stroker )
  {
    FT_Error  error  = 0;

    if ( stroker->subpath_open )
    {
      FT_StrokeBorder  right = stroker->borders;

      /* all right, this is an opened path, we need to add a cap between     */
      /* right & left, add the reverse of left, then add a final cap between */
      /* left & right..                                                      */
      error = ft_stroker_cap( stroker, stroker->angle_in, 0 );
      if (error) goto Exit;

      /* add reversed points from "left" to "right" */
      error = ft_stroker_add_reverse_left( stroker, 1 );
      if (error) goto Exit;

      /* now add the final cap */
      stroker->center = stroker->subpath_start;
      error = ft_stroker_cap( stroker, stroker->subpath_angle+FT_ANGLE_PI, 0 );
      if (error) goto Exit;

      /* now, end the right subpath accordingly. the left one is */
      /* rewind and doesn't need further processing..            */
      ft_stroke_border_close( right );
    }
    else
    {
      FT_Angle           turn;
      FT_Int             inside_side;

      /* process the corner ... */
      stroker->angle_out = stroker->subpath_angle;
      turn               = FT_Angle_Diff( stroker->angle_in, stroker->angle_out );

      /* no specific corner processing is required if the turn is 0 */
      if (turn != 0)
      {
        /* when we turn to the right, the inside side is 0 */
        inside_side = 0;

        /* otherwise, the inside side is 1 */
        if (turn < 0)
          inside_side = 1;

        /* IMPORTANT: WE DO NOT PROCESS THE INSIDE BORDER HERE !! */
        /* process the inside side */
        /* error = ft_stroker_inside( stroker, inside_side );
             if (error) goto Exit; */

        /* process the outside side */
        error = ft_stroker_outside( stroker, 1-inside_side );
        if (error) goto Exit;
      }

      /* we will first end our two subpaths */
      ft_stroke_border_close( stroker->borders + 0 );
      ft_stroke_border_close( stroker->borders + 1 );

      /* now, add the reversed left subpath to "right" */
      error = ft_stroker_add_reverse_left( stroker, 0 );
      if (error) goto Exit;
    }

  Exit:
    return error;
  }


  FT_EXPORT_DEF( FT_Error )
  FT_Stroker_GetCounts( FT_Stroker  stroker,
                        FT_UInt    *anum_points,
                        FT_UInt    *anum_contours )
  {
    FT_UInt   count1, count2, num_points   = 0;
    FT_UInt   count3, count4, num_contours = 0;
    FT_Error  error;

    error = ft_stroke_border_get_counts( stroker->borders+0, &count1, &count2 );
    if (error) goto Exit;

    error = ft_stroke_border_get_counts( stroker->borders+1, &count3, &count4 );
    if (error) goto Exit;

    num_points   = count1 + count3;
    num_contours = count2 + count4;

    stroker->valid = 1;

  Exit:
    *anum_points   = num_points;
    *anum_contours = num_contours;
    return error;
  }


  FT_EXPORT_DEF( void )
  FT_Stroker_Export( FT_Stroker   stroker,
                     FT_Outline*  outline )
  {
    if ( stroker->valid )
    {
      ft_stroke_border_export( stroker->borders+0, outline );
      ft_stroke_border_export( stroker->borders+1, outline );
    }
  }
