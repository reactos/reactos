/***************************************************************************/
/*                                                                         */
/*  ahhint.c                                                               */
/*                                                                         */
/*    Glyph hinter (body).                                                 */
/*                                                                         */
/*  Copyright 2000-2001, 2002, 2003, 2004 Catharon Productions Inc.        */
/*  Author: David Turner                                                   */
/*                                                                         */
/*  This file is part of the Catharon Typography Project and shall only    */
/*  be used, modified, and distributed under the terms of the Catharon     */
/*  Open Source License that should come with this file under the name     */
/*  `CatharonLicense.txt'.  By continuing to use, modify, or distribute    */
/*  this file you indicate that you have read the license and              */
/*  understand and accept it fully.                                        */
/*                                                                         */
/*  Note that this license is compatible with the FreeType license.        */
/*                                                                         */
/***************************************************************************/


#include <ft2build.h>
#include "ahhint.h"
#include "ahglyph.h"
#include "ahangles.h"
#include "aherrors.h"
#include FT_OUTLINE_H


#define FACE_GLOBALS( face )  ( (AH_Face_Globals)(face)->autohint.data )

#define AH_USE_IUP
#define OPTIM_STEM_SNAP


  /*************************************************************************/
  /*************************************************************************/
  /****                                                                 ****/
  /****   Hinting routines                                              ****/
  /****                                                                 ****/
  /*************************************************************************/
  /*************************************************************************/

  /* snap a given width in scaled coordinates to one of the */
  /* current standard widths                                */
  static FT_Pos
  ah_snap_width( FT_Pos*  widths,
                 FT_Int   count,
                 FT_Pos   width )
  {
    int     n;
    FT_Pos  best      = 64 + 32 + 2;
    FT_Pos  reference = width;
    FT_Pos  scaled;


    for ( n = 0; n < count; n++ )
    {
      FT_Pos  w;
      FT_Pos  dist;


      w = widths[n];
      dist = width - w;
      if ( dist < 0 )
        dist = -dist;
      if ( dist < best )
      {
        best      = dist;
        reference = w;
      }
    }

    scaled = FT_PIX_ROUND( reference );

    if ( width >= reference )
    {
      if ( width < scaled + 48 )
        width = reference;
    }
    else
    {
      if ( width > scaled - 48 )
        width = reference;
    }

    return width;
  }


  /* compute the snapped width of a given stem */

#ifdef FT_CONFIG_CHESTER_SERIF

  static FT_Pos
  ah_compute_stem_width( AH_Hinter      hinter,
                         int            vertical,
                         FT_Pos         width,
                         AH_Edge_Flags  base_flags,
                         AH_Edge_Flags  stem_flags )
  {
    AH_Globals  globals = &hinter->globals->scaled;
    FT_Pos      dist    = width;
    FT_Int      sign    = 0;


    if ( dist < 0 )
    {
      dist = -width;
      sign = 1;
    }

    if ( !hinter->do_stem_adjust )
    {
      /* leave stem widths unchanged */
    }
    else if ( (  vertical && !hinter->do_vert_snapping ) ||
              ( !vertical && !hinter->do_horz_snapping ) )
    {
      /* smooth hinting process: very lightly quantize the stem width */
      /*                                                              */

      /* leave the widths of serifs alone */

      if ( ( stem_flags & AH_EDGE_SERIF ) && vertical && ( dist < 3 * 64 ) )
        goto Done_Width;

      else if ( ( base_flags & AH_EDGE_ROUND ) )
      {
        if ( dist < 80 )
          dist = 64;
      }
      else if ( dist < 56 )
        dist = 56;

      {
        FT_Pos  delta = dist - globals->stds[vertical];


        if ( delta < 0 )
          delta = -delta;

        if ( delta < 40 )
        {
          dist = globals->stds[vertical];
          if ( dist < 48 )
            dist = 48;

          goto Done_Width;
        }

        if ( dist < 3 * 64 )
        {
          delta  = dist & 63;
          dist  &= -64;

          if ( delta < 10 )
            dist += delta;

          else if ( delta < 32 )
            dist += 10;

          else if ( delta < 54 )
            dist += 54;

          else
            dist += delta;
        }
        else
          dist = ( dist + 32 ) & ~63;
      }
    }
    else
    {
      /* strong hinting process: snap the stem width to integer pixels */
      /*                                                               */
      if ( vertical )
      {
        dist = ah_snap_width( globals->heights, globals->num_heights, dist );

        /* in the case of vertical hinting, always round */
        /* the stem heights to integer pixels            */
        if ( dist >= 64 )
          dist = ( dist + 16 ) & ~63;
        else
          dist = 64;
      }
      else
      {
        dist = ah_snap_width( globals->widths, globals->num_widths, dist );

        if ( hinter->flags & AH_HINTER_MONOCHROME )
        {
          /* monochrome horizontal hinting: snap widths to integer pixels */
          /* with a different threshold                                   */
          if ( dist < 64 )
            dist = 64;
          else
            dist = ( dist + 32 ) & ~63;
        }
        else
        {
          /* for horizontal anti-aliased hinting, we adopt a more subtle */
          /* approach: we strengthen small stems, round stems whose size */
          /* is between 1 and 2 pixels to an integer, otherwise nothing  */
          if ( dist < 48 )
            dist = ( dist + 64 ) >> 1;

          else if ( dist < 128 )
            dist = ( dist + 22 ) & ~63;
          else
            /* XXX: round otherwise to prevent color fringes in LCD mode */
            dist = ( dist + 32 ) & ~63;
        }
      }
    }

  Done_Width:
    if ( sign )
      dist = -dist;

    return dist;
  }

#else /* !FT_CONFIG_CHESTER_SERIF */

  static FT_Pos
  ah_compute_stem_width( AH_Hinter  hinter,
                         int        vertical,
                         FT_Pos     width )
  {
    AH_Globals  globals = &hinter->globals->scaled;
    FT_Pos      dist    = width;
    FT_Int      sign    = 0;


    if ( dist < 0 )
    {
      dist = -width;
      sign = 1;
    }

    if ( !hinter->do_stem_adjust )
    {
      /* leave stem widths unchanged */
    }
    else if ( (  vertical && !hinter->do_vert_snapping ) ||
              ( !vertical && !hinter->do_horz_snapping ) )
    {
      /* smooth hinting process: very lightly quantize the stem width */
      /*                                                              */
      if ( dist < 64 )
        dist = 64;

      {
        FT_Pos  delta = dist - globals->stds[vertical];


        if ( delta < 0 )
          delta = -delta;

        if ( delta < 40 )
        {
          dist = globals->stds[vertical];
          if ( dist < 48 )
            dist = 48;
        }

        if ( dist < 3 * 64 )
        {
          delta  = dist & 63;
          dist  &= -64;

          if ( delta < 10 )
            dist += delta;

          else if ( delta < 32 )
            dist += 10;

          else if ( delta < 54 )
            dist += 54;

          else
            dist += delta;
        }
        else
          dist = ( dist + 32 ) & ~63;
      }
    }
    else
    {
      /* strong hinting process: snap the stem width to integer pixels */
      /*                                                               */
      if ( vertical )
      {
        dist = ah_snap_width( globals->heights, globals->num_heights, dist );

        /* in the case of vertical hinting, always round */
        /* the stem heights to integer pixels            */
        if ( dist >= 64 )
          dist = ( dist + 16 ) & ~63;
        else
          dist = 64;
      }
      else
      {
        dist = ah_snap_width( globals->widths, globals->num_widths, dist );

        if ( hinter->flags & AH_HINTER_MONOCHROME )
        {
          /* monochrome horizontal hinting: snap widths to integer pixels */
          /* with a different threshold                                   */
          if ( dist < 64 )
            dist = 64;
          else
            dist = ( dist + 32 ) & ~63;
        }
        else
        {
          /* for horizontal anti-aliased hinting, we adopt a more subtle */
          /* approach: we strengthen small stems, round stems whose size */
          /* is between 1 and 2 pixels to an integer, otherwise nothing  */
          if ( dist < 48 )
            dist = ( dist + 64 ) >> 1;

          else if ( dist < 128 )
            dist = ( dist + 22 ) & ~63;
          else
            /* XXX: round otherwise to prevent color fringes in LCD mode */
            dist = ( dist + 32 ) & ~63;
        }
      }
    }

    if ( sign )
      dist = -dist;

    return dist;
  }

#endif /* !FT_CONFIG_CHESTER_SERIF */


  /* align one stem edge relative to the previous stem edge */
  static void
  ah_align_linked_edge( AH_Hinter  hinter,
                        AH_Edge    base_edge,
                        AH_Edge    stem_edge,
                        int        vertical )
  {
    FT_Pos  dist = stem_edge->opos - base_edge->opos;

#ifdef FT_CONFIG_CHESTER_SERIF

    FT_Pos  fitted_width = ah_compute_stem_width( hinter,
                                                  vertical,
                                                  dist,
                                                  base_edge->flags,
                                                  stem_edge->flags );


    stem_edge->pos = base_edge->pos + fitted_width;

#else

    stem_edge->pos = base_edge->pos +
                     ah_compute_stem_width( hinter, vertical, dist );

#endif

  }


  static void
  ah_align_serif_edge( AH_Hinter  hinter,
                       AH_Edge    base,
                       AH_Edge    serif,
                       int        vertical )
  {
    FT_Pos  dist;
    FT_Pos  sign = 1;

    FT_UNUSED( hinter );
    FT_UNUSED( vertical );


    dist = serif->opos - base->opos;
    if ( dist < 0 )
    {
      dist = -dist;
      sign = -1;
    }

#if 0
    /* do not touch serifs widths! */
    if ( base->flags & AH_EDGE_DONE )
    {
      if ( dist >= 64 )
        dist = ( dist + 8 ) & ~63;

      else if ( dist <= 32 && !vertical )
        dist = ( dist + 33 ) >> 1;

      else
        dist = 0;
    }
#endif

    serif->pos = base->pos + sign * dist;
  }


  /*************************************************************************/
  /*************************************************************************/
  /*************************************************************************/
  /****                                                                 ****/
  /****       E D G E   H I N T I N G                                   ****/
  /****                                                                 ****/
  /*************************************************************************/
  /*************************************************************************/
  /*************************************************************************/


  FT_LOCAL_DEF( void )
  ah_hinter_hint_edges( AH_Hinter  hinter )
  {
    AH_Edge     edges;
    AH_Edge     edge_limit;
    AH_Outline  outline = hinter->glyph;
    FT_Int      dimension;
    FT_Int      n_edges;


    edges      = outline->horz_edges;
    edge_limit = edges + outline->num_hedges;

    for ( dimension = 1; dimension >= 0; dimension-- )
    {
      AH_Edge  edge;
      AH_Edge  anchor = 0;
      int      has_serifs = 0;


      if ( !hinter->do_horz_hints && !dimension )
        goto Next_Dimension;

      if ( !hinter->do_vert_hints && dimension )
        goto Next_Dimension;

      /* we begin by aligning all stems relative to the blue zone */
      /* if needed -- that's only for horizontal edges            */
      if ( dimension )
      {
        for ( edge = edges; edge < edge_limit; edge++ )
        {
          FT_Pos*      blue;
          AH_EdgeRec  *edge1, *edge2;


          if ( edge->flags & AH_EDGE_DONE )
            continue;

          blue  = edge->blue_edge;
          edge1 = 0;
          edge2 = edge->link;

          if ( blue )
          {
            edge1 = edge;
          }
          else if ( edge2 && edge2->blue_edge )
          {
            blue  = edge2->blue_edge;
            edge1 = edge2;
            edge2 = edge;
          }

          if ( !edge1 )
            continue;

          edge1->pos    = blue[0];
          edge1->flags |= AH_EDGE_DONE;

          if ( edge2 && !edge2->blue_edge )
          {
            ah_align_linked_edge( hinter, edge1, edge2, dimension );
            edge2->flags |= AH_EDGE_DONE;
          }

          if ( !anchor )
            anchor = edge;
        }
      }

      /* now we will align all stem edges, trying to maintain the */
      /* relative order of stems in the glyph                     */
      for ( edge = edges; edge < edge_limit; edge++ )
      {
        AH_EdgeRec*  edge2;


        if ( edge->flags & AH_EDGE_DONE )
          continue;

        /* skip all non-stem edges */
        edge2 = edge->link;
        if ( !edge2 )
        {
          has_serifs++;
          continue;
        }

        /* now align the stem */

        /* this should not happen, but it's better to be safe */
        if ( edge2->blue_edge || edge2 < edge )
        {
          ah_align_linked_edge( hinter, edge2, edge, dimension );
          edge->flags |= AH_EDGE_DONE;
          continue;
        }

        if ( !anchor )
        {

#ifdef FT_CONFIG_CHESTER_STEM

          FT_Pos  org_len, org_center, cur_len;
          FT_Pos  cur_pos1, error1, error2, u_off, d_off;


          org_len = edge2->opos - edge->opos;
          cur_len = ah_compute_stem_width( hinter, dimension, org_len,
                                           edge->flags, edge2->flags );

          if ( cur_len <= 64 )
            u_off = d_off = 32;
          else
          {
            u_off = 38;
            d_off = 26;
          }

          if ( cur_len < 96 )
          {
            org_center = edge->opos + ( org_len >> 1 );

            cur_pos1   = FT_PIX_ROUND( org_center );

            error1 = org_center - ( cur_pos1 - u_off );
            if ( error1 < 0 )
              error1 = -error1;

            error2 = org_center - ( cur_pos1 + d_off );
            if ( error2 < 0 )
              error2 = -error2;

            if ( error1 < error2 )
              cur_pos1 -= u_off;
            else
              cur_pos1 += d_off;

            edge->pos  = cur_pos1 - cur_len / 2;
            edge2->pos = cur_pos1 + cur_len / 2;

          }
          else
            edge->pos = FT_PIX_ROUND( edge->opos );

          anchor = edge;

          edge->flags |= AH_EDGE_DONE;

          ah_align_linked_edge( hinter, edge, edge2, dimension );

#else /* !FT_CONFIG_CHESTER_STEM */

          edge->pos = FT_PIX_ROUND( edge->opos );
          anchor    = edge;

          edge->flags |= AH_EDGE_DONE;

          ah_align_linked_edge( hinter, edge, edge2, dimension );

#endif /* !FT_CONFIG_CHESTER_STEM */

        }
        else
        {
          FT_Pos  org_pos, org_len, org_center, cur_len;
          FT_Pos  cur_pos1, cur_pos2, delta1, delta2;


          org_pos    = anchor->pos + ( edge->opos - anchor->opos );
          org_len    = edge2->opos - edge->opos;
          org_center = org_pos + ( org_len >> 1 );

#ifdef FT_CONFIG_CHESTER_SERIF

          cur_len = ah_compute_stem_width( hinter, dimension, org_len,
                                           edge->flags, edge2->flags  );


#else  /* !FT_CONFIG_CHESTER_SERIF */

          cur_len = ah_compute_stem_width( hinter, dimension, org_len );

#endif /* !FT_CONFIG_CHESTER_SERIF */

#ifdef FT_CONFIG_CHESTER_STEM

          if ( cur_len < 96 )
          {
            FT_Pos  u_off, d_off;


            cur_pos1 = FT_PIX_ROUND( org_center );

            if (cur_len <= 64 )
              u_off = d_off = 32;
            else
            {
              u_off = 38;
              d_off = 26;
            }

            delta1 = org_center - ( cur_pos1 - u_off );
            if ( delta1 < 0 )
              delta1 = -delta1;

            delta2 = org_center - ( cur_pos1 + d_off );
            if ( delta2 < 0 )
              delta2 = -delta2;

            if ( delta1 < delta2 )
              cur_pos1 -= u_off;
            else
              cur_pos1 += d_off;

            edge->pos  = cur_pos1 - cur_len / 2;
            edge2->pos = cur_pos1 + cur_len / 2;
          }
          else
          {
            org_pos    = anchor->pos + ( edge->opos - anchor->opos );
            org_len    = edge2->opos - edge->opos;
            org_center = org_pos + ( org_len >> 1 );

            cur_len    = ah_compute_stem_width( hinter, dimension, org_len,
                                                edge->flags, edge2->flags );

            cur_pos1   = FT_PIX_ROUND( org_pos );
            delta1     = ( cur_pos1 + ( cur_len >> 1 ) - org_center );
            if ( delta1 < 0 )
              delta1 = -delta1;

            cur_pos2   = FT_PIX_ROUND( org_pos + org_len ) - cur_len;
            delta2     = ( cur_pos2 + ( cur_len >> 1 ) - org_center );
            if ( delta2 < 0 )
              delta2 = -delta2;

            edge->pos  = ( delta1 < delta2 ) ? cur_pos1 : cur_pos2;
            edge2->pos = edge->pos + cur_len;
          }

#else /* !FT_CONFIG_CHESTER_STEM */

          cur_pos1   = FT_PIX_ROUND( org_pos );
          delta1     = ( cur_pos1 + ( cur_len >> 1 ) - org_center );
          if ( delta1 < 0 )
            delta1 = -delta1;

          cur_pos2   = FT_PIX_ROUND( org_pos + org_len ) - cur_len;
          delta2     = ( cur_pos2 + ( cur_len >> 1 ) - org_center );
          if ( delta2 < 0 )
            delta2 = -delta2;

          edge->pos  = ( delta1 <= delta2 ) ? cur_pos1 : cur_pos2;
          edge2->pos = edge->pos + cur_len;

#endif /* !FT_CONFIG_CHESTER_STEM */

          edge->flags  |= AH_EDGE_DONE;
          edge2->flags |= AH_EDGE_DONE;

          if ( edge > edges && edge->pos < edge[-1].pos )
            edge->pos = edge[-1].pos;
        }
      }

      /* make sure that lowercase m's maintain their symmetry */

      /* In general, lowercase m's have six vertical edges if they are sans */
      /* serif, or twelve if they are avec serif.  This implementation is   */
      /* based on that assumption, and seems to work very well with most    */
      /* faces.  However, if for a certain face this assumption is not      */
      /* true, the m is just rendered like before.  In addition, any stem   */
      /* correction will only be applied to symmetrical glyphs (even if the */
      /* glyph is not an m), so the potential for unwanted distortion is    */
      /* relatively low.                                                    */

      /* We don't handle horizontal edges since we can't easily assure that */
      /* the third (lowest) stem aligns with the base line; it might end up */
      /* one pixel higher or lower.                                         */

      n_edges = (FT_Int)( edge_limit - edges );
      if ( !dimension && ( n_edges == 6 || n_edges == 12 ) )
      {
        AH_EdgeRec  *edge1, *edge2, *edge3;
        FT_Pos       dist1, dist2, span, delta;


        if ( n_edges == 6 )
        {
          edge1 = edges;
          edge2 = edges + 2;
          edge3 = edges + 4;
        }
        else
        {
          edge1 = edges + 1;
          edge2 = edges + 5;
          edge3 = edges + 9;
        }

        dist1 = edge2->opos - edge1->opos;
        dist2 = edge3->opos - edge2->opos;

        span = dist1 - dist2;
        if ( span < 0 )
          span = -span;

        if ( span < 8 )
        {
          delta = edge3->pos - ( 2 * edge2->pos - edge1->pos );
          edge3->pos -= delta;
          if ( edge3->link )
            edge3->link->pos -= delta;

          /* move the serifs along with the stem */
          if ( n_edges == 12 )
          {
            ( edges + 8 )->pos -= delta;
            ( edges + 11 )->pos -= delta;
          }

          edge3->flags |= AH_EDGE_DONE;
          if ( edge3->link )
            edge3->link->flags |= AH_EDGE_DONE;
        }
      }

      if ( !has_serifs )
        goto Next_Dimension;

      /* now hint the remaining edges (serifs and single) in order */
      /* to complete our processing                                */
      for ( edge = edges; edge < edge_limit; edge++ )
      {
        if ( edge->flags & AH_EDGE_DONE )
          continue;

        if ( edge->serif )
          ah_align_serif_edge( hinter, edge->serif, edge, dimension );
        else if ( !anchor )
        {
          edge->pos = FT_PIX_ROUND( edge->opos );
          anchor    = edge;
        }
        else
          edge->pos = anchor->pos +
                      FT_PIX_ROUND( edge->opos - anchor->opos );

        edge->flags |= AH_EDGE_DONE;

        if ( edge > edges && edge->pos < edge[-1].pos )
          edge->pos = edge[-1].pos;

        if ( edge + 1 < edge_limit        &&
             edge[1].flags & AH_EDGE_DONE &&
             edge->pos > edge[1].pos      )
          edge->pos = edge[1].pos;
      }

    Next_Dimension:
      edges      = outline->vert_edges;
      edge_limit = edges + outline->num_vedges;
    }
  }


  /*************************************************************************/
  /*************************************************************************/
  /*************************************************************************/
  /****                                                                 ****/
  /****       P O I N T   H I N T I N G                                 ****/
  /****                                                                 ****/
  /*************************************************************************/
  /*************************************************************************/
  /*************************************************************************/

  static void
  ah_hinter_align_edge_points( AH_Hinter  hinter )
  {
    AH_Outline  outline = hinter->glyph;
    AH_Edge     edges;
    AH_Edge     edge_limit;
    FT_Int      dimension;


    edges      = outline->horz_edges;
    edge_limit = edges + outline->num_hedges;

    for ( dimension = 1; dimension >= 0; dimension-- )
    {
      AH_Edge  edge;


      edge = edges;
      for ( ; edge < edge_limit; edge++ )
      {
        /* move the points of each segment     */
        /* in each edge to the edge's position */
        AH_Segment  seg = edge->first;


        do
        {
          AH_Point  point = seg->first;


          for (;;)
          {
            if ( dimension )
            {
              point->y      = edge->pos;
              point->flags |= AH_FLAG_TOUCH_Y;
            }
            else
            {
              point->x      = edge->pos;
              point->flags |= AH_FLAG_TOUCH_X;
            }

            if ( point == seg->last )
              break;

            point = point->next;
          }

          seg = seg->edge_next;

        } while ( seg != edge->first );
      }

      edges      = outline->vert_edges;
      edge_limit = edges + outline->num_vedges;
    }
  }


  /* hint the strong points -- this is equivalent to the TrueType `IP' */
  /* hinting instruction                                               */
  static void
  ah_hinter_align_strong_points( AH_Hinter  hinter )
  {
    AH_Outline  outline = hinter->glyph;
    FT_Int      dimension;
    AH_Edge     edges;
    AH_Edge     edge_limit;
    AH_Point    points;
    AH_Point    point_limit;
    AH_Flags    touch_flag;


    points      = outline->points;
    point_limit = points + outline->num_points;

    edges       = outline->horz_edges;
    edge_limit  = edges + outline->num_hedges;
    touch_flag  = AH_FLAG_TOUCH_Y;

    for ( dimension = 1; dimension >= 0; dimension-- )
    {
      AH_Point  point;
      AH_Edge   edge;


      if ( edges < edge_limit )
        for ( point = points; point < point_limit; point++ )
        {
          FT_Pos  u, ou, fu;  /* point position */
          FT_Pos  delta;


          if ( point->flags & touch_flag )
            continue;

#ifndef AH_OPTION_NO_WEAK_INTERPOLATION
          /* if this point is candidate to weak interpolation, we will  */
          /* interpolate it after all strong points have been processed */
          if (  ( point->flags & AH_FLAG_WEAK_INTERPOLATION ) &&
               !( point->flags & AH_FLAG_INFLECTION )         )
            continue;
#endif

          if ( dimension )
          {
            u  = point->fy;
            ou = point->oy;
          }
          else
          {
            u  = point->fx;
            ou = point->ox;
          }

          fu = u;

          /* is the point before the first edge? */
          edge  = edges;
          delta = edge->fpos - u;
          if ( delta >= 0 )
          {
            u = edge->pos - ( edge->opos - ou );
            goto Store_Point;
          }

          /* is the point after the last edge? */
          edge  = edge_limit - 1;
          delta = u - edge->fpos;
          if ( delta >= 0 )
          {
            u = edge->pos + ( ou - edge->opos );
            goto Store_Point;
          }

#if 1
          {
            FT_UInt  min, max, mid;
            FT_Pos   fpos;


            /* find enclosing edges */
            min = 0;
            max = (FT_UInt)( edge_limit - edges );

            while ( min < max )
            {
              mid  = ( max + min ) >> 1;
              edge = edges + mid;
              fpos = edge->fpos;

              if ( u < fpos )
                max = mid;
              else if ( u > fpos )
                min = mid + 1;
              else
              {
                /* we are on the edge */
                u = edge->pos;
                goto Store_Point;
              }
            }

            {
              AH_Edge  before = edges + min - 1;
              AH_Edge  after  = edges + min + 0;


              /* assert( before && after && before != after ) */
              if ( before->scale == 0 )
                before->scale = FT_DivFix( after->pos - before->pos,
                                           after->fpos - before->fpos );

              u = before->pos + FT_MulFix( fu - before->fpos,
                                           before->scale );
            }
          }

#else /* !0 */

          /* otherwise, interpolate the point in between */
          {
            AH_Edge  before = 0;
            AH_Edge  after  = 0;


            for ( edge = edges; edge < edge_limit; edge++ )
            {
              if ( u == edge->fpos )
              {
                u = edge->pos;
                goto Store_Point;
              }
              if ( u < edge->fpos )
                break;
              before = edge;
            }

            for ( edge = edge_limit - 1; edge >= edges; edge-- )
            {
              if ( u == edge->fpos )
              {
                u = edge->pos;
                goto Store_Point;
              }
              if ( u > edge->fpos )
                break;
              after = edge;
            }

            if ( before->scale == 0 )
              before->scale = FT_DivFix( after->pos - before->pos,
                                        after->fpos - before->fpos );

            u = before->pos + FT_MulFix( fu - before->fpos,
                                        before->scale );
          }

#endif /* !0 */

        Store_Point:

          /* save the point position */
          if ( dimension )
            point->y = u;
          else
            point->x = u;

          point->flags |= touch_flag;
        }

      edges      = outline->vert_edges;
      edge_limit = edges + outline->num_vedges;
      touch_flag = AH_FLAG_TOUCH_X;
    }
  }


#ifndef AH_OPTION_NO_WEAK_INTERPOLATION

  static void
  ah_iup_shift( AH_Point  p1,
                AH_Point  p2,
                AH_Point  ref )
  {
    AH_Point  p;
    FT_Pos    delta = ref->u - ref->v;


    for ( p = p1; p < ref; p++ )
      p->u = p->v + delta;

    for ( p = ref + 1; p <= p2; p++ )
      p->u = p->v + delta;
  }


  static void
  ah_iup_interp( AH_Point  p1,
                 AH_Point  p2,
                 AH_Point  ref1,
                 AH_Point  ref2 )
  {
    AH_Point  p;
    FT_Pos    u;
    FT_Pos    v1 = ref1->v;
    FT_Pos    v2 = ref2->v;
    FT_Pos    d1 = ref1->u - v1;
    FT_Pos    d2 = ref2->u - v2;


    if ( p1 > p2 )
      return;

    if ( v1 == v2 )
    {
      for ( p = p1; p <= p2; p++ )
      {
        u = p->v;

        if ( u <= v1 )
          u += d1;
        else
          u += d2;

        p->u = u;
      }
      return;
    }

    if ( v1 < v2 )
    {
      for ( p = p1; p <= p2; p++ )
      {
        u = p->v;

        if ( u <= v1 )
          u += d1;
        else if ( u >= v2 )
          u += d2;
        else
          u = ref1->u + FT_MulDiv( u - v1, ref2->u - ref1->u, v2 - v1 );

        p->u = u;
      }
    }
    else
    {
      for ( p = p1; p <= p2; p++ )
      {
        u = p->v;

        if ( u <= v2 )
          u += d2;
        else if ( u >= v1 )
          u += d1;
        else
          u = ref1->u + FT_MulDiv( u - v1, ref2->u - ref1->u, v2 - v1 );

        p->u = u;
      }
    }
  }


  /* interpolate weak points -- this is equivalent to the TrueType `IUP' */
  /* hinting instruction                                                 */
  static void
  ah_hinter_align_weak_points( AH_Hinter  hinter )
  {
    AH_Outline  outline = hinter->glyph;
    FT_Int      dimension;
    AH_Point    points;
    AH_Point    point_limit;
    AH_Point*   contour_limit;
    AH_Flags    touch_flag;


    points      = outline->points;
    point_limit = points + outline->num_points;

    /* PASS 1: Move segment points to edge positions */

    touch_flag = AH_FLAG_TOUCH_Y;

    contour_limit = outline->contours + outline->num_contours;

    ah_setup_uv( outline, AH_UV_OY );

    for ( dimension = 1; dimension >= 0; dimension-- )
    {
      AH_Point   point;
      AH_Point   end_point;
      AH_Point   first_point;
      AH_Point*  contour;


      point   = points;
      contour = outline->contours;

      for ( ; contour < contour_limit; contour++ )
      {
        point       = *contour;
        end_point   = point->prev;
        first_point = point;

        while ( point <= end_point && !( point->flags & touch_flag ) )
          point++;

        if ( point <= end_point )
        {
          AH_Point  first_touched = point;
          AH_Point  cur_touched   = point;


          point++;
          while ( point <= end_point )
          {
            if ( point->flags & touch_flag )
            {
              /* we found two successive touched points; we interpolate */
              /* all contour points between them                        */
              ah_iup_interp( cur_touched + 1, point - 1,
                             cur_touched, point );
              cur_touched = point;
            }
            point++;
          }

          if ( cur_touched == first_touched )
          {
            /* this is a special case: only one point was touched in the */
            /* contour; we thus simply shift the whole contour           */
            ah_iup_shift( first_point, end_point, cur_touched );
          }
          else
          {
            /* now interpolate after the last touched point to the end */
            /* of the contour                                          */
            ah_iup_interp( cur_touched + 1, end_point,
                           cur_touched, first_touched );

            /* if the first contour point isn't touched, interpolate */
            /* from the contour start to the first touched point     */
            if ( first_touched > points )
              ah_iup_interp( first_point, first_touched - 1,
                             cur_touched, first_touched );
          }
        }
      }

      /* now save the interpolated values back to x/y */
      if ( dimension )
      {
        for ( point = points; point < point_limit; point++ )
          point->y = point->u;

        touch_flag = AH_FLAG_TOUCH_X;
        ah_setup_uv( outline, AH_UV_OX );
      }
      else
      {
        for ( point = points; point < point_limit; point++ )
          point->x = point->u;

        break;  /* exit loop */
      }
    }
  }

#endif /* !AH_OPTION_NO_WEAK_INTERPOLATION */


  FT_LOCAL_DEF( void )
  ah_hinter_align_points( AH_Hinter  hinter )
  {
    ah_hinter_align_edge_points( hinter );

#ifndef AH_OPTION_NO_STRONG_INTERPOLATION
    ah_hinter_align_strong_points( hinter );
#endif

#ifndef AH_OPTION_NO_WEAK_INTERPOLATION
    ah_hinter_align_weak_points( hinter );
#endif
  }


  /*************************************************************************/
  /*************************************************************************/
  /*************************************************************************/
  /****                                                                 ****/
  /****       H I N T E R   O B J E C T   M E T H O D S                 ****/
  /****                                                                 ****/
  /*************************************************************************/
  /*************************************************************************/
  /*************************************************************************/


  /* scale and fit the global metrics */
  static void
  ah_hinter_scale_globals( AH_Hinter  hinter,
                           FT_Fixed   x_scale,
                           FT_Fixed   y_scale )
  {
    FT_Int           n;
    AH_Face_Globals  globals = hinter->globals;
    AH_Globals       design  = &globals->design;
    AH_Globals       scaled  = &globals->scaled;


    /* copy content */
    *scaled = *design;

    /* scale the standard widths & heights */
    for ( n = 0; n < design->num_widths; n++ )
      scaled->widths[n] = FT_MulFix( design->widths[n], x_scale );

    for ( n = 0; n < design->num_heights; n++ )
      scaled->heights[n] = FT_MulFix( design->heights[n], y_scale );

    scaled->stds[0] = ( design->num_widths  > 0 ) ? scaled->widths[0]  : 32000;
    scaled->stds[1] = ( design->num_heights > 0 ) ? scaled->heights[0] : 32000;

    /* scale the blue zones */
    for ( n = 0; n < AH_BLUE_MAX; n++ )
    {
      FT_Pos  delta, delta2;


      delta = design->blue_shoots[n] - design->blue_refs[n];
      delta2 = delta;
      if ( delta < 0 )
        delta2 = -delta2;
      delta2 = FT_MulFix( delta2, y_scale );

      if ( delta2 < 32 )
        delta2 = 0;
      else if ( delta2 < 64 )
        delta2 = 32 + ( ( ( delta2 - 32 ) + 16 ) & ~31 );
      else
        delta2 = FT_PIX_ROUND( delta2 );

      if ( delta < 0 )
        delta2 = -delta2;

      scaled->blue_refs[n] =
        FT_PIX_ROUND( FT_MulFix( design->blue_refs[n], y_scale ) );

      scaled->blue_shoots[n] = scaled->blue_refs[n] + delta2;
    }

    globals->x_scale = x_scale;
    globals->y_scale = y_scale;
  }


  static void
  ah_hinter_align( AH_Hinter  hinter )
  {
    ah_hinter_align_edge_points( hinter );
    ah_hinter_align_points( hinter );
  }


  /* finalize a hinter object */
  FT_LOCAL_DEF( void )
  ah_hinter_done( AH_Hinter  hinter )
  {
    if ( hinter )
    {
      FT_Memory  memory = hinter->memory;


      ah_loader_done( hinter->loader );
      ah_outline_done( hinter->glyph );

      /* note: the `globals' pointer is _not_ owned by the hinter */
      /*       but by the current face object; we don't need to   */
      /*       release it                                         */
      hinter->globals = 0;
      hinter->face    = 0;

      FT_FREE( hinter );
    }
  }


  /* create a new empty hinter object */
  FT_LOCAL_DEF( FT_Error )
  ah_hinter_new( FT_Library  library,
                 AH_Hinter  *ahinter )
  {
    AH_Hinter  hinter = 0;
    FT_Memory  memory = library->memory;
    FT_Error   error;


    *ahinter = 0;

    /* allocate object */
    if ( FT_NEW( hinter ) )
      goto Exit;

    hinter->memory = memory;
    hinter->flags  = 0;

    /* allocate outline and loader */
    error = ah_outline_new( memory, &hinter->glyph )  ||
            ah_loader_new ( memory, &hinter->loader ) ||
            ah_loader_create_extra( hinter->loader );
    if ( error )
      goto Exit;

    *ahinter = hinter;

  Exit:
    if ( error )
      ah_hinter_done( hinter );

    return error;
  }


  /* create a face's autohint globals */
  FT_LOCAL_DEF( FT_Error )
  ah_hinter_new_face_globals( AH_Hinter   hinter,
                              FT_Face     face,
                              AH_Globals  globals )
  {
    FT_Error         error;
    FT_Memory        memory = hinter->memory;
    AH_Face_Globals  face_globals;


    if ( FT_NEW( face_globals ) )
      goto Exit;

    hinter->face    = face;
    hinter->globals = face_globals;

    if ( globals )
      face_globals->design = *globals;
    else
      ah_hinter_compute_globals( hinter );

    face->autohint.data      = face_globals;
    face->autohint.finalizer = (FT_Generic_Finalizer)
                                 ah_hinter_done_face_globals;
    face_globals->face       = face;

  Exit:
    return error;
  }


  /* discard a face's autohint globals */
  FT_LOCAL_DEF( void )
  ah_hinter_done_face_globals( AH_Face_Globals  globals )
  {
    FT_Face    face   = globals->face;
    FT_Memory  memory = face->memory;


    FT_FREE( globals );
  }


  static FT_Error
  ah_hinter_load( AH_Hinter  hinter,
                  FT_UInt    glyph_index,
                  FT_Int32   load_flags,
                  FT_UInt    depth )
  {
    FT_Face           face     = hinter->face;
    FT_GlyphSlot      slot     = face->glyph;
    FT_Slot_Internal  internal = slot->internal;
    FT_Fixed          x_scale  = hinter->globals->x_scale;
    FT_Fixed          y_scale  = hinter->globals->y_scale;
    FT_Error          error;
    AH_Outline        outline  = hinter->glyph;
    AH_Loader         gloader  = hinter->loader;


    /* load the glyph */
    error = FT_Load_Glyph( face, glyph_index, load_flags );
    if ( error )
      goto Exit;

    /* Set `hinter->transformed' after loading with FT_LOAD_NO_RECURSE. */
    hinter->transformed = internal->glyph_transformed;

    if ( hinter->transformed )
    {
      FT_Matrix  imatrix;


      imatrix              = internal->glyph_matrix;
      hinter->trans_delta  = internal->glyph_delta;
      hinter->trans_matrix = imatrix;

      FT_Matrix_Invert( &imatrix );
      FT_Vector_Transform( &hinter->trans_delta, &imatrix );
    }

    /* set linear horizontal metrics */
    slot->linearHoriAdvance = slot->metrics.horiAdvance;
    slot->linearVertAdvance = slot->metrics.vertAdvance;

    switch ( slot->format )
    {
    case FT_GLYPH_FORMAT_OUTLINE:

      /* translate glyph outline if we need to */
      if ( hinter->transformed )
      {
        FT_UInt     n     = slot->outline.n_points;
        FT_Vector*  point = slot->outline.points;


        for ( ; n > 0; point++, n-- )
        {
          point->x += hinter->trans_delta.x;
          point->y += hinter->trans_delta.y;
        }
      }

      /* copy the outline points in the loader's current               */
      /* extra points which is used to keep original glyph coordinates */
      error = ah_loader_check_points( gloader, slot->outline.n_points + 4,
                                      slot->outline.n_contours );
      if ( error )
        goto Exit;

      FT_ARRAY_COPY( gloader->current.extra_points, slot->outline.points,
                     slot->outline.n_points );

      FT_ARRAY_COPY( gloader->current.outline.contours, slot->outline.contours,
                     slot->outline.n_contours );

      FT_ARRAY_COPY( gloader->current.outline.tags, slot->outline.tags,
                     slot->outline.n_points );

      gloader->current.outline.n_points   = slot->outline.n_points;
      gloader->current.outline.n_contours = slot->outline.n_contours;

      /* compute original horizontal phantom points, ignoring vertical ones */
      hinter->pp1.x = 0;
      hinter->pp1.y = 0;
      hinter->pp2.x = FT_MulFix( slot->metrics.horiAdvance, x_scale );
      hinter->pp2.y = 0;

      /* be sure to check for spacing glyphs */
      if ( slot->outline.n_points == 0 )
        goto Hint_Metrics;

      /* now load the slot image into the auto-outline and run the */
      /* automatic hinting process                                 */
      error = ah_outline_load( outline, x_scale, y_scale, face );
      if ( error )
        goto Exit;

      /* perform feature detection */
      ah_outline_detect_features( outline );

      if ( hinter->do_vert_hints )
      {
        ah_outline_compute_blue_edges( outline, hinter->globals );
        ah_outline_scale_blue_edges( outline, hinter->globals );
      }

      /* perform alignment control */
      ah_hinter_hint_edges( hinter );
      ah_hinter_align( hinter );

      /* now save the current outline into the loader's current table */
      ah_outline_save( outline, gloader );

      /* we now need to hint the metrics according to the change in */
      /* width/positioning that occured during the hinting process  */
      if ( outline->num_vedges > 0 )
      {
        FT_Pos   old_advance, old_rsb, old_lsb, new_lsb, pp1x_uh, pp2x_uh;
        AH_Edge  edge1 = outline->vert_edges;     /* leftmost edge  */
        AH_Edge  edge2 = edge1 +
                         outline->num_vedges - 1; /* rightmost edge */


        old_advance = hinter->pp2.x;
        old_rsb     = old_advance - edge2->opos;
        old_lsb     = edge1->opos;
        new_lsb     = edge1->pos;

        /* remember unhinted values to later account for rounding errors */

        pp1x_uh = new_lsb    - old_lsb;
        pp2x_uh = edge2->pos + old_rsb;

        /* prefer too much space over too little space for very small sizes */

        if ( old_lsb < 24 )
          pp1x_uh -= 5;

        if ( old_rsb < 24 )
          pp2x_uh += 5;

        hinter->pp1.x = FT_PIX_ROUND( pp1x_uh );
        hinter->pp2.x = FT_PIX_ROUND( pp2x_uh );

        slot->lsb_delta = hinter->pp1.x - pp1x_uh;
        slot->rsb_delta = hinter->pp2.x - pp2x_uh;

#if 0
        /* try to fix certain bad advance computations */
        if ( hinter->pp2.x + hinter->pp1.x == edge2->pos && old_rsb > 4 )
          hinter->pp2.x += 64;
#endif
      }

      else
      {
        hinter->pp1.x = ( hinter->pp1.x + 32 ) & -64;
        hinter->pp2.x = ( hinter->pp2.x + 32 ) & -64;
      }

      /* good, we simply add the glyph to our loader's base */
      ah_loader_add( gloader );
      break;

    case FT_GLYPH_FORMAT_COMPOSITE:
      {
        FT_UInt      nn, num_subglyphs = slot->num_subglyphs;
        FT_UInt      num_base_subgs, start_point;
        FT_SubGlyph  subglyph;


        start_point = gloader->base.outline.n_points;

        /* first of all, copy the subglyph descriptors in the glyph loader */
        error = ah_loader_check_subglyphs( gloader, num_subglyphs );
        if ( error )
          goto Exit;

        FT_ARRAY_COPY( gloader->current.subglyphs, slot->subglyphs,
                       num_subglyphs );

        gloader->current.num_subglyphs = num_subglyphs;
        num_base_subgs = gloader->base.num_subglyphs;

        /* now, read each subglyph independently */
        for ( nn = 0; nn < num_subglyphs; nn++ )
        {
          FT_Vector  pp1, pp2;
          FT_Pos     x, y;
          FT_UInt    num_points, num_new_points, num_base_points;


          /* gloader.current.subglyphs can change during glyph loading due */
          /* to re-allocation -- we must recompute the current subglyph on */
          /* each iteration                                                */
          subglyph = gloader->base.subglyphs + num_base_subgs + nn;

          pp1 = hinter->pp1;
          pp2 = hinter->pp2;

          num_base_points = gloader->base.outline.n_points;

          error = ah_hinter_load( hinter, subglyph->index,
                                  load_flags, depth + 1 );
          if ( error )
            goto Exit;

          /* recompute subglyph pointer */
          subglyph = gloader->base.subglyphs + num_base_subgs + nn;

          if ( subglyph->flags & FT_SUBGLYPH_FLAG_USE_MY_METRICS )
          {
            pp1 = hinter->pp1;
            pp2 = hinter->pp2;
          }
          else
          {
            hinter->pp1 = pp1;
            hinter->pp2 = pp2;
          }

          num_points     = gloader->base.outline.n_points;
          num_new_points = num_points - num_base_points;

          /* now perform the transform required for this subglyph */

          if ( subglyph->flags & ( FT_SUBGLYPH_FLAG_SCALE    |
                                   FT_SUBGLYPH_FLAG_XY_SCALE |
                                   FT_SUBGLYPH_FLAG_2X2      ) )
          {
            FT_Vector*  cur   = gloader->base.outline.points +
                                num_base_points;
            FT_Vector*  org   = gloader->base.extra_points +
                                num_base_points;
            FT_Vector*  limit = cur + num_new_points;


            for ( ; cur < limit; cur++, org++ )
            {
              FT_Vector_Transform( cur, &subglyph->transform );
              FT_Vector_Transform( org, &subglyph->transform );
            }
          }

          /* apply offset */

          if ( !( subglyph->flags & FT_SUBGLYPH_FLAG_ARGS_ARE_XY_VALUES ) )
          {
            FT_Int      k = subglyph->arg1;
            FT_UInt     l = subglyph->arg2;
            FT_Vector*  p1;
            FT_Vector*  p2;


            if ( start_point + k >= num_base_points         ||
                               l >= (FT_UInt)num_new_points )
            {
              error = AH_Err_Invalid_Composite;
              goto Exit;
            }

            l += num_base_points;

            /* for now, only use the current point coordinates;    */
            /* we may consider another approach in the near future */
            p1 = gloader->base.outline.points + start_point + k;
            p2 = gloader->base.outline.points + start_point + l;

            x = p1->x - p2->x;
            y = p1->y - p2->y;
          }
          else
          {
            x = FT_MulFix( subglyph->arg1, x_scale );
            y = FT_MulFix( subglyph->arg2, y_scale );

            x = FT_PIX_ROUND(x);
            y = FT_PIX_ROUND(y);
          }

          {
            FT_Outline  dummy = gloader->base.outline;


            dummy.points  += num_base_points;
            dummy.n_points = (short)num_new_points;

            FT_Outline_Translate( &dummy, x, y );
          }
        }
      }
      break;

    default:
      /* we don't support other formats (yet?) */
      error = AH_Err_Unimplemented_Feature;
    }

  Hint_Metrics:
    if ( depth == 0 )
    {
      FT_BBox  bbox;


      /* transform the hinted outline if needed */
      if ( hinter->transformed )
        FT_Outline_Transform( &gloader->base.outline, &hinter->trans_matrix );

      /* we must translate our final outline by -pp1.x and compute */
      /* the new metrics                                           */
      if ( hinter->pp1.x )
        FT_Outline_Translate( &gloader->base.outline, -hinter->pp1.x, 0 );

      FT_Outline_Get_CBox( &gloader->base.outline, &bbox );
      bbox.xMin  = FT_PIX_FLOOR(  bbox.xMin );
      bbox.yMin  = FT_PIX_FLOOR(  bbox.yMin );
      bbox.xMax  = FT_PIX_CEIL( bbox.xMax );
      bbox.yMax  = FT_PIX_CEIL( bbox.yMax );

      slot->metrics.width        = bbox.xMax - bbox.xMin;
      slot->metrics.height       = bbox.yMax - bbox.yMin;
      slot->metrics.horiBearingX = bbox.xMin;
      slot->metrics.horiBearingY = bbox.yMax;

      /* for mono-width fonts (like Andale, Courier, etc.) we need */
      /* to keep the original rounded advance width                */
      if ( !FT_IS_FIXED_WIDTH( slot->face ) )
        slot->metrics.horiAdvance = hinter->pp2.x - hinter->pp1.x;
      else
        slot->metrics.horiAdvance = FT_MulFix( slot->metrics.horiAdvance,
                                               x_scale );

      slot->metrics.horiAdvance = FT_PIX_ROUND( slot->metrics.horiAdvance );

      /* now copy outline into glyph slot */
      ah_loader_rewind( slot->internal->loader );
      error = ah_loader_copy_points( slot->internal->loader, gloader );
      if ( error )
        goto Exit;

      slot->outline = slot->internal->loader->base.outline;
      slot->format  = FT_GLYPH_FORMAT_OUTLINE;
    }

#ifdef DEBUG_HINTER
    ah_debug_hinter = hinter;
#endif

  Exit:
    return error;
  }


  /* load and hint a given glyph */
  FT_LOCAL_DEF( FT_Error )
  ah_hinter_load_glyph( AH_Hinter     hinter,
                        FT_GlyphSlot  slot,
                        FT_Size       size,
                        FT_UInt       glyph_index,
                        FT_Int32      load_flags )
  {
    FT_Face          face         = slot->face;
    FT_Error         error;
    FT_Fixed         x_scale      = size->metrics.x_scale;
    FT_Fixed         y_scale      = size->metrics.y_scale;
    AH_Face_Globals  face_globals = FACE_GLOBALS( face );
    FT_Render_Mode   hint_mode    = FT_LOAD_TARGET_MODE( load_flags );


    /* first of all, we need to check that we're using the correct face and */
    /* global hints to load the glyph                                       */
    if ( hinter->face != face || hinter->globals != face_globals )
    {
      hinter->face = face;
      if ( !face_globals )
      {
        error = ah_hinter_new_face_globals( hinter, face, 0 );
        if ( error )
          goto Exit;

      }
      hinter->globals = FACE_GLOBALS( face );
      face_globals    = FACE_GLOBALS( face );

    }

#ifdef FT_CONFIG_CHESTER_BLUE_SCALE

   /* try to optimize the y_scale so that the top of non-capital letters
    * is aligned on a pixel boundary whenever possible
    */
    {
      AH_Globals  design = &face_globals->design;
      FT_Pos      shoot  = design->blue_shoots[AH_BLUE_SMALL_TOP];


      /* the value of 'shoot' will be -1000 if the font doesn't have */
      /* small latin letters; we simply check the sign here...       */
      if ( shoot > 0 )
      {
        FT_Pos  scaled = FT_MulFix( shoot, y_scale );
        FT_Pos  fitted = FT_PIX_ROUND( scaled );


        if ( scaled != fitted )
        {
         /* adjust y_scale
          */
          y_scale = FT_MulDiv( y_scale, fitted, scaled );

         /* adust x_scale
          */
          if ( fitted < scaled )
            x_scale -= x_scale / 50;  /* x_scale*0.98 with integers */
        }
      }
    }

#endif /* FT_CONFIG_CHESTER_BLUE_SCALE */

    /* now, we must check the current character pixel size to see if we */
    /* need to rescale the global metrics                               */
    if ( face_globals->x_scale != x_scale ||
         face_globals->y_scale != y_scale )
      ah_hinter_scale_globals( hinter, x_scale, y_scale );

    ah_loader_rewind( hinter->loader );

    /* reset hinting flags according to load flags and current render target */
    hinter->do_horz_hints = FT_BOOL( !(load_flags & FT_LOAD_NO_AUTOHINT) );
    hinter->do_vert_hints = FT_BOOL( !(load_flags & FT_LOAD_NO_AUTOHINT) );

#ifdef DEBUG_HINTER
    hinter->do_horz_hints = !ah_debug_disable_vert;  /* not a bug, the meaning */
    hinter->do_vert_hints = !ah_debug_disable_horz;  /* of h/v is inverted!    */
#endif

    /* we snap the width of vertical stems for the monochrome and         */
    /* horizontal LCD rendering targets only.  Corresponds to X snapping. */
    hinter->do_horz_snapping = FT_BOOL( hint_mode == FT_RENDER_MODE_MONO ||
                                        hint_mode == FT_RENDER_MODE_LCD  );

    /* we snap the width of horizontal stems for the monochrome and     */
    /* vertical LCD rendering targets only.  Corresponds to Y snapping. */
    hinter->do_vert_snapping = FT_BOOL( hint_mode == FT_RENDER_MODE_MONO   ||
                                        hint_mode == FT_RENDER_MODE_LCD_V  );

    hinter->do_stem_adjust   = FT_BOOL( hint_mode != FT_RENDER_MODE_LIGHT );

    load_flags |= FT_LOAD_NO_SCALE
                | FT_LOAD_IGNORE_TRANSFORM;
    load_flags &= ~FT_LOAD_RENDER;

    error = ah_hinter_load( hinter, glyph_index, load_flags, 0 );

  Exit:
    return error;
  }


  /* retrieve a face's autohint globals for client applications */
  FT_LOCAL_DEF( void )
  ah_hinter_get_global_hints( AH_Hinter  hinter,
                              FT_Face    face,
                              void**     global_hints,
                              long*      global_len )
  {
    AH_Globals  globals = 0;
    FT_Memory   memory  = hinter->memory;
    FT_Error    error;


    /* allocate new master globals */
    if ( FT_NEW( globals ) )
      goto Fail;

    /* compute face globals if needed */
    if ( !FACE_GLOBALS( face ) )
    {
      error = ah_hinter_new_face_globals( hinter, face, 0 );
      if ( error )
        goto Fail;
    }

    *globals      = FACE_GLOBALS( face )->design;
    *global_hints = globals;
    *global_len   = sizeof( *globals );

    return;

  Fail:
    FT_FREE( globals );

    *global_hints = 0;
    *global_len   = 0;
  }


  FT_LOCAL_DEF( void )
  ah_hinter_done_global_hints( AH_Hinter  hinter,
                               void*      global_hints )
  {
    FT_Memory  memory = hinter->memory;


    FT_FREE( global_hints );
  }


/* END */
