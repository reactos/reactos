/***************************************************************************/
/*                                                                         */
/*  ftbbox.c                                                               */
/*                                                                         */
/*    FreeType bbox computation (body).                                    */
/*                                                                         */
/*  Copyright 1996-2002, 2004, 2006, 2010, 2013 by                         */
/*  David Turner, Robert Wilhelm, and Werner Lemberg.                      */
/*                                                                         */
/*  This file is part of the FreeType project, and may only be used        */
/*  modified and distributed under the terms of the FreeType project       */
/*  license, LICENSE.TXT.  By continuing to use, modify, or distribute     */
/*  this file you indicate that you have read the license and              */
/*  understand and accept it fully.                                        */
/*                                                                         */
/***************************************************************************/


  /*************************************************************************/
  /*                                                                       */
  /* This component has a _single_ role: to compute exact outline bounding */
  /* boxes.                                                                */
  /*                                                                       */
  /*************************************************************************/


#include <ft2build.h>
#include FT_INTERNAL_DEBUG_H

#include FT_BBOX_H
#include FT_IMAGE_H
#include FT_OUTLINE_H
#include FT_INTERNAL_CALC_H
#include FT_INTERNAL_OBJECTS_H


  typedef struct  TBBox_Rec_
  {
    FT_Vector  last;
    FT_BBox    bbox;

  } TBBox_Rec;


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    BBox_Move_To                                                       */
  /*                                                                       */
  /* <Description>                                                         */
  /*    This function is used as a `move_to' and `line_to' emitter during  */
  /*    FT_Outline_Decompose().  It simply records the destination point   */
  /*    in `user->last'; no further computations are necessary since we    */
  /*    use the cbox as the starting bbox which must be refined.           */
  /*                                                                       */
  /* <Input>                                                               */
  /*    to   :: A pointer to the destination vector.                       */
  /*                                                                       */
  /* <InOut>                                                               */
  /*    user :: A pointer to the current walk context.                     */
  /*                                                                       */
  /* <Return>                                                              */
  /*    Always 0.  Needed for the interface only.                          */
  /*                                                                       */
  static int
  BBox_Move_To( FT_Vector*  to,
                TBBox_Rec*  user )
  {
    user->last = *to;

    return 0;
  }


#define CHECK_X( p, bbox )  \
          ( p->x < bbox.xMin || p->x > bbox.xMax )

#define CHECK_Y( p, bbox )  \
          ( p->y < bbox.yMin || p->y > bbox.yMax )


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    BBox_Conic_Check                                                   */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Find the extrema of a 1-dimensional conic Bezier curve and update  */
  /*    a bounding range.  This version uses direct computation, as it     */
  /*    doesn't need square roots.                                         */
  /*                                                                       */
  /* <Input>                                                               */
  /*    y1  :: The start coordinate.                                       */
  /*                                                                       */
  /*    y2  :: The coordinate of the control point.                        */
  /*                                                                       */
  /*    y3  :: The end coordinate.                                         */
  /*                                                                       */
  /* <InOut>                                                               */
  /*    min :: The address of the current minimum.                         */
  /*                                                                       */
  /*    max :: The address of the current maximum.                         */
  /*                                                                       */
  static void
  BBox_Conic_Check( FT_Pos   y1,
                    FT_Pos   y2,
                    FT_Pos   y3,
                    FT_Pos*  min,
                    FT_Pos*  max )
  {
    /* This function is only called when a control off-point is outside */
    /* the bbox that contains all on-points.  It finds a local extremum */
    /* within the segment, equal to (y1*y3 - y2*y2)/(y1 - 2*y2 + y3).   */
    /* Or, offsetting from y2, we get                                   */

    y1 -= y2;
    y3 -= y2;
    y2 += FT_MulDiv( y1, y3, y1 + y3 );

    if ( y2 < *min )
      *min = y2;
    if ( y2 > *max )
      *max = y2;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    BBox_Conic_To                                                      */
  /*                                                                       */
  /* <Description>                                                         */
  /*    This function is used as a `conic_to' emitter during               */
  /*    FT_Outline_Decompose().  It checks a conic Bezier curve with the   */
  /*    current bounding box, and computes its extrema if necessary to     */
  /*    update it.                                                         */
  /*                                                                       */
  /* <Input>                                                               */
  /*    control :: A pointer to a control point.                           */
  /*                                                                       */
  /*    to      :: A pointer to the destination vector.                    */
  /*                                                                       */
  /* <InOut>                                                               */
  /*    user    :: The address of the current walk context.                */
  /*                                                                       */
  /* <Return>                                                              */
  /*    Always 0.  Needed for the interface only.                          */
  /*                                                                       */
  /* <Note>                                                                */
  /*    In the case of a non-monotonous arc, we compute directly the       */
  /*    extremum coordinates, as it is sufficiently fast.                  */
  /*                                                                       */
  static int
  BBox_Conic_To( FT_Vector*  control,
                 FT_Vector*  to,
                 TBBox_Rec*  user )
  {
    /* we don't need to check `to' since it is always an `on' point, thus */
    /* within the bbox                                                    */

    if ( CHECK_X( control, user->bbox ) )
      BBox_Conic_Check( user->last.x,
                        control->x,
                        to->x,
                        &user->bbox.xMin,
                        &user->bbox.xMax );

    if ( CHECK_Y( control, user->bbox ) )
      BBox_Conic_Check( user->last.y,
                        control->y,
                        to->y,
                        &user->bbox.yMin,
                        &user->bbox.yMax );

    user->last = *to;

    return 0;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    BBox_Cubic_Check                                                   */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Find the extrema of a 1-dimensional cubic Bezier curve and         */
  /*    update a bounding range.  This version uses iterative splitting    */
  /*    because it is faster than the exact solution with square roots.    */
  /*                                                                       */
  /* <Input>                                                               */
  /*    p1  :: The start coordinate.                                       */
  /*                                                                       */
  /*    p2  :: The coordinate of the first control point.                  */
  /*                                                                       */
  /*    p3  :: The coordinate of the second control point.                 */
  /*                                                                       */
  /*    p4  :: The end coordinate.                                         */
  /*                                                                       */
  /* <InOut>                                                               */
  /*    min :: The address of the current minimum.                         */
  /*                                                                       */
  /*    max :: The address of the current maximum.                         */
  /*                                                                       */
  static FT_Pos
  update_cubic_max( FT_Pos  q1,
                    FT_Pos  q2,
                    FT_Pos  q3,
                    FT_Pos  q4,
                    FT_Pos  max )
  {
    /* for a cubic segment to possibly reach new maximum, at least */
    /* one of its off-points must stay above the current value     */
    while ( q2 > max || q3 > max )
    {
      /* determine which half contains the maximum and split */
      if ( q1 + q2 > q3 + q4 ) /* first half */
      {
        q4 = q4 + q3;
        q3 = q3 + q2;
        q2 = q2 + q1;
        q4 = q4 + q3;
        q3 = q3 + q2;
        q4 = ( q4 + q3 ) / 8;
        q3 = q3 / 4;
        q2 = q2 / 2;
      }
      else                     /* second half */
      {
        q1 = q1 + q2;
        q2 = q2 + q3;
        q3 = q3 + q4;
        q1 = q1 + q2;
        q2 = q2 + q3;
        q1 = ( q1 + q2 ) / 8;
        q2 = q2 / 4;
        q3 = q3 / 2;
      }

      /* check whether either end reached the maximum */
      if ( q1 == q2 && q1 >= q3 )
      {
        max = q1;
        break;
      }
      if ( q3 == q4 && q2 <= q4 )
      {
        max = q4;
        break;
      }
    }

    return max;
  }


  static void
  BBox_Cubic_Check( FT_Pos   p1,
                    FT_Pos   p2,
                    FT_Pos   p3,
                    FT_Pos   p4,
                    FT_Pos*  min,
                    FT_Pos*  max )
  {
    FT_Pos  nmin, nmax;
    FT_Int  shift;


    /* This function is only called when a control off-point is outside  */
    /* the bbox that contains all on-points.  It finds a local extremum  */
    /* within the segment using iterative bisection of the segment.      */
    /* The fixed-point arithmetic of bisection is inherently stable      */
    /* but may loose accuracy in the two lowest bits.  To compensate,    */
    /* we upscale the segment if there is room.  Large values may need   */
    /* to be downscaled to avoid overflows during bisection.             */
    /* The control off-point outside the bbox is likely to have the top  */
    /* absolute value among arguments.                                   */

    shift = 27 - FT_MSB( FT_ABS( p2 ) | FT_ABS( p3 ) );

    if ( shift > 0 )
    {
      /* upscaling too much just wastes time */
      if ( shift > 2 )
        shift = 2;

      p1 <<=  shift;
      p2 <<=  shift;
      p3 <<=  shift;
      p4 <<=  shift;
      nmin = *min << shift;
      nmax = *max << shift;
    }
    else
    {
      p1 >>= -shift;
      p2 >>= -shift;
      p3 >>= -shift;
      p4 >>= -shift;
      nmin = *min >> -shift;
      nmax = *max >> -shift;
    }

    nmax =  update_cubic_max(  p1,  p2,  p3,  p4,  nmax );

    /* now flip the signs to update the minimum */
    nmin = -update_cubic_max( -p1, -p2, -p3, -p4, -nmin );

    if ( shift > 0 )
    {
      nmin >>=  shift;
      nmax >>=  shift;
    }
    else
    {
      nmin <<= -shift;
      nmax <<= -shift;
    }

    if ( nmin < *min )
      *min = nmin;
    if ( nmax > *max )
      *max = nmax;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    BBox_Cubic_To                                                      */
  /*                                                                       */
  /* <Description>                                                         */
  /*    This function is used as a `cubic_to' emitter during               */
  /*    FT_Outline_Decompose().  It checks a cubic Bezier curve with the   */
  /*    current bounding box, and computes its extrema if necessary to     */
  /*    update it.                                                         */
  /*                                                                       */
  /* <Input>                                                               */
  /*    control1 :: A pointer to the first control point.                  */
  /*                                                                       */
  /*    control2 :: A pointer to the second control point.                 */
  /*                                                                       */
  /*    to       :: A pointer to the destination vector.                   */
  /*                                                                       */
  /* <InOut>                                                               */
  /*    user     :: The address of the current walk context.               */
  /*                                                                       */
  /* <Return>                                                              */
  /*    Always 0.  Needed for the interface only.                          */
  /*                                                                       */
  /* <Note>                                                                */
  /*    In the case of a non-monotonous arc, we don't compute directly     */
  /*    extremum coordinates, we subdivide instead.                        */
  /*                                                                       */
  static int
  BBox_Cubic_To( FT_Vector*  control1,
                 FT_Vector*  control2,
                 FT_Vector*  to,
                 TBBox_Rec*  user )
  {
    /* We don't need to check `to' since it is always an on-point,    */
    /* thus within the bbox.  Only segments with an off-point outside */
    /* the bbox can possibly reach new extreme values.                */

    if ( CHECK_X( control1, user->bbox ) ||
         CHECK_X( control2, user->bbox ) )
      BBox_Cubic_Check( user->last.x,
                        control1->x,
                        control2->x,
                        to->x,
                        &user->bbox.xMin,
                        &user->bbox.xMax );

    if ( CHECK_Y( control1, user->bbox ) ||
         CHECK_Y( control2, user->bbox ) )
      BBox_Cubic_Check( user->last.y,
                        control1->y,
                        control2->y,
                        to->y,
                        &user->bbox.yMin,
                        &user->bbox.yMax );

    user->last = *to;

    return 0;
  }

FT_DEFINE_OUTLINE_FUNCS(bbox_interface,
    (FT_Outline_MoveTo_Func) BBox_Move_To,
    (FT_Outline_LineTo_Func) BBox_Move_To,
    (FT_Outline_ConicTo_Func)BBox_Conic_To,
    (FT_Outline_CubicTo_Func)BBox_Cubic_To,
    0, 0
  )

  /* documentation is in ftbbox.h */

  FT_EXPORT_DEF( FT_Error )
  FT_Outline_Get_BBox( FT_Outline*  outline,
                       FT_BBox     *abbox )
  {
    FT_BBox     cbox;
    FT_BBox     bbox;
    FT_Vector*  vec;
    FT_UShort   n;


    if ( !abbox )
      return FT_THROW( Invalid_Argument );

    if ( !outline )
      return FT_THROW( Invalid_Outline );

    /* if outline is empty, return (0,0,0,0) */
    if ( outline->n_points == 0 || outline->n_contours <= 0 )
    {
      abbox->xMin = abbox->xMax = 0;
      abbox->yMin = abbox->yMax = 0;
      return 0;
    }

    /* We compute the control box as well as the bounding box of  */
    /* all `on' points in the outline.  Then, if the two boxes    */
    /* coincide, we exit immediately.                             */

    vec = outline->points;
    bbox.xMin = bbox.xMax = cbox.xMin = cbox.xMax = vec->x;
    bbox.yMin = bbox.yMax = cbox.yMin = cbox.yMax = vec->y;
    vec++;

    for ( n = 1; n < outline->n_points; n++ )
    {
      FT_Pos  x = vec->x;
      FT_Pos  y = vec->y;


      /* update control box */
      if ( x < cbox.xMin ) cbox.xMin = x;
      if ( x > cbox.xMax ) cbox.xMax = x;

      if ( y < cbox.yMin ) cbox.yMin = y;
      if ( y > cbox.yMax ) cbox.yMax = y;

      if ( FT_CURVE_TAG( outline->tags[n] ) == FT_CURVE_TAG_ON )
      {
        /* update bbox for `on' points only */
        if ( x < bbox.xMin ) bbox.xMin = x;
        if ( x > bbox.xMax ) bbox.xMax = x;

        if ( y < bbox.yMin ) bbox.yMin = y;
        if ( y > bbox.yMax ) bbox.yMax = y;
      }

      vec++;
    }

    /* test two boxes for equality */
    if ( cbox.xMin < bbox.xMin || cbox.xMax > bbox.xMax ||
         cbox.yMin < bbox.yMin || cbox.yMax > bbox.yMax )
    {
      /* the two boxes are different, now walk over the outline to */
      /* get the Bezier arc extrema.                               */

      FT_Error   error;
      TBBox_Rec  user;

#ifdef FT_CONFIG_OPTION_PIC
      FT_Outline_Funcs bbox_interface;
      Init_Class_bbox_interface(&bbox_interface);
#endif

      user.bbox = bbox;

      error = FT_Outline_Decompose( outline, &bbox_interface, &user );
      if ( error )
        return error;

      *abbox = user.bbox;
    }
    else
      *abbox = bbox;

    return FT_Err_Ok;
  }


/* END */
