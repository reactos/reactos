/***************************************************************************/
/*                                                                         */
/*  ftsynth.c                                                              */
/*                                                                         */
/*    FreeType synthesizing code for emboldening and slanting (body).      */
/*                                                                         */
/*  Copyright 2000-2001, 2002, 2003 by                                     */
/*  David Turner, Robert Wilhelm, and Werner Lemberg.                      */
/*                                                                         */
/*  This file is part of the FreeType project, and may only be used,       */
/*  modified, and distributed under the terms of the FreeType project      */
/*  license, LICENSE.TXT.  By continuing to use, modify, or distribute     */
/*  this file you indicate that you have read the license and              */
/*  understand and accept it fully.                                        */
/*                                                                         */
/***************************************************************************/


#include <ft2build.h>
#include FT_INTERNAL_OBJECTS_H
#include FT_INTERNAL_CALC_H
#include FT_OUTLINE_H
#include FT_TRIGONOMETRY_H
#include FT_SYNTHESIS_H


#define FT_BOLD_THRESHOLD  0x0100


  /*************************************************************************/
  /*************************************************************************/
  /****                                                                 ****/
  /****   EXPERIMENTAL OBLIQUING SUPPORT                                ****/
  /****                                                                 ****/
  /*************************************************************************/
  /*************************************************************************/

  FT_EXPORT_DEF( void )
  FT_GlyphSlot_Oblique( FT_GlyphSlot  slot )
  {
    FT_Matrix    transform;
    FT_Outline*  outline = &slot->outline;


    /* only oblique outline glyphs */
    if ( slot->format != FT_GLYPH_FORMAT_OUTLINE )
      return;

    /* we don't touch the advance width */

    /* For italic, simply apply a shear transform, with an angle */
    /* of about 12 degrees.                                      */

    transform.xx = 0x10000L;
    transform.yx = 0x00000L;

    transform.xy = 0x06000L;
    transform.yy = 0x10000L;

    FT_Outline_Transform( outline, &transform );
  }


  /*************************************************************************/
  /*************************************************************************/
  /****                                                                 ****/
  /****   EXPERIMENTAL EMBOLDENING/OUTLINING SUPPORT                    ****/
  /****                                                                 ****/
  /*************************************************************************/
  /*************************************************************************/



  static int
  ft_test_extrema( FT_Outline*  outline,
                   int          n )
  {
    FT_Vector  *prev, *cur, *next;
    FT_Pos      product;
    FT_Int      c, first, last;


    /* we need to compute the `previous' and `next' point */
    /* for these extrema.                                 */
    cur  = outline->points + n;
    prev = cur - 1;
    next = cur + 1;

    first = 0;
    for ( c = 0; c < outline->n_contours; c++ )
    {
      last = outline->contours[c];

      if ( n == first )
        prev = outline->points + last;

      if ( n == last )
        next = outline->points + first;

      first = last + 1;
    }

    product = FT_MulDiv( cur->x - prev->x,   /* in.x  */
                         next->y - cur->y,   /* out.y */
                         0x40 )
              -
              FT_MulDiv( cur->y - prev->y,   /* in.y  */
                         next->x - cur->x,   /* out.x */
                         0x40 );

    if ( product )
      product = product > 0 ? 1 : -1;

    return product;
  }


  /* Compute the orientation of path filling.  It differs between TrueType */
  /* and Type1 formats.  We could use the `FT_OUTLINE_REVERSE_FILL' flag,  */
  /* but it is better to re-compute it directly (it seems that this flag   */
  /* isn't correctly set for some weird composite glyphs currently).       */
  /*                                                                       */
  /* We do this by computing bounding box points, and computing their      */
  /* curvature.                                                            */
  /*                                                                       */
  /* The function returns either 1 or -1.                                  */
  /*                                                                       */
  static int
  ft_get_orientation( FT_Outline*  outline )
  {
    FT_BBox  box;
    FT_BBox  indices;
    int      n, last;


    indices.xMin = -1;
    indices.yMin = -1;
    indices.xMax = -1;
    indices.yMax = -1;

    box.xMin = box.yMin =  32767;
    box.xMax = box.yMax = -32768;

    /* is it empty ? */
    if ( outline->n_contours < 1 )
      return 1;

    last = outline->contours[outline->n_contours - 1];

    for ( n = 0; n <= last; n++ )
    {
      FT_Pos  x, y;


      x = outline->points[n].x;
      if ( x < box.xMin )
      {
        box.xMin     = x;
        indices.xMin = n;
      }
      if ( x > box.xMax )
      {
        box.xMax     = x;
        indices.xMax = n;
      }

      y = outline->points[n].y;
      if ( y < box.yMin )
      {
        box.yMin     = y;
        indices.yMin = n;
      }
      if ( y > box.yMax )
      {
        box.yMax     = y;
        indices.yMax = n;
      }
    }

    /* test orientation of the xmin */
    n = ft_test_extrema( outline, indices.xMin );
    if ( n )
      goto Exit;

    n = ft_test_extrema( outline, indices.yMin );
    if ( n )
      goto Exit;

    n = ft_test_extrema( outline, indices.xMax );
    if ( n )
      goto Exit;

    n = ft_test_extrema( outline, indices.yMax );
    if ( !n )
      n = 1;

  Exit:
    return n;
  }


  FT_EXPORT_DEF( void )
  FT_GlyphSlot_Embolden( FT_GlyphSlot  slot )
  {
    FT_Vector*   points;
    FT_Vector    v_prev, v_first, v_next, v_cur;
    FT_Pos       distance;
    FT_Outline*  outline = &slot->outline;
    FT_Face      face = FT_SLOT_FACE( slot );
    FT_Angle     rotate, angle_in, angle_out;
    FT_Int       c, n, first, orientation;


    /* only embolden outline glyph images */
    if ( slot->format != FT_GLYPH_FORMAT_OUTLINE )
      return;

    /* compute control distance */
    distance = FT_MulFix( face->units_per_EM / 60,
                          face->size->metrics.y_scale );

    orientation = ft_get_orientation( outline );
    rotate      = FT_ANGLE_PI2*orientation;

    points = outline->points;

    first = 0;
    for ( c = 0; c < outline->n_contours; c++ )
    {
      int  last = outline->contours[c];


      v_first = points[first];
      v_prev  = points[last];
      v_cur   = v_first;

      for ( n = first; n <= last; n++ )
      {
        FT_Pos     d;
        FT_Vector  in, out;
        FT_Fixed   scale;
        FT_Angle   angle_diff;


        if ( n < last ) v_next = points[n + 1];
        else            v_next = v_first;

        /* compute the in and out vectors */
        in.x  = v_cur.x - v_prev.x;
        in.y  = v_cur.y - v_prev.y;

        out.x = v_next.x - v_cur.x;
        out.y = v_next.y - v_cur.y;

        angle_in   = FT_Atan2( in.x, in.y );
        angle_out  = FT_Atan2( out.x, out.y );
        angle_diff = FT_Angle_Diff( angle_in, angle_out );
        scale      = FT_Cos( angle_diff/2 );

        if ( scale < 0x400L && scale > -0x400L )
        {
          if ( scale >= 0 )
            scale = 0x400L;
          else
            scale = -0x400L;
        }

        d = FT_DivFix( distance, scale );

        FT_Vector_From_Polar( &in, d, angle_in + angle_diff/2 - rotate );

        outline->points[n].x = v_cur.x + distance + in.x;
        outline->points[n].y = v_cur.y + distance + in.y;

        v_prev = v_cur;
        v_cur  = v_next;
      }

      first = last + 1;
    }

    slot->metrics.horiAdvance =
      ( slot->metrics.horiAdvance + distance*4 ) & ~63;
  }


/* END */
