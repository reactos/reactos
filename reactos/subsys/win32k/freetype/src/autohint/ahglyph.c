/***************************************************************************/
/*                                                                         */
/*  ahglyph.c                                                              */
/*                                                                         */
/*    Routines used to load and analyze a given glyph before hinting       */
/*    (body).                                                              */
/*                                                                         */
/*  Copyright 2000 Catharon Productions Inc.                               */
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


#ifdef FT_FLAT_COMPILE

#include "ahglyph.h"
#include "ahangles.h"
#include "ahglobal.h"

#else

#include <freetype/src/autohint/ahglyph.h>
#include <freetype/src/autohint/ahangles.h>
#include <freetype/src/autohint/ahglobal.h>

#endif


#include <stdio.h>


#define xxxAH_DEBUG_GLYPH


  /* compute the direction value of a given vector.. */
  static
  AH_Direction  ah_compute_direction( FT_Pos  dx,
                                      FT_Pos  dy )
  {
    AH_Direction  dir;
    FT_Pos        ax = ABS( dx );
    FT_Pos        ay = ABS( dy );


    dir = ah_dir_none;

    /* test for vertical direction */
    if ( ax * 12 < ay )
    {
      dir = dy > 0 ? ah_dir_up : ah_dir_down;
    }
    /* test for horizontal direction */
    else if ( ay * 12 < ax )
    {
      dir = dx > 0 ? ah_dir_right : ah_dir_left;
    }

    return dir;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    ah_outline_new                                                     */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Creates a new and empty AH_Outline object.                         */
  /*                                                                       */
  LOCAL_FUNC
  FT_Error  ah_outline_new( FT_Memory     memory,
                            AH_Outline**  aoutline )
  {
    FT_Error     error;
    AH_Outline*  outline;


    if ( !ALLOC( outline, sizeof ( *outline ) ) )
    {
      outline->memory = memory;
      *aoutline = outline;
    }

    return error;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    ah_outline_done                                                    */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Destroys a given AH_Outline object.                                */
  /*                                                                       */
  LOCAL_FUNC
  void  ah_outline_done( AH_Outline*  outline )
  {
    FT_Memory memory = outline->memory;


    FREE( outline->horz_edges );
    FREE( outline->horz_segments );
    FREE( outline->contours );
    FREE( outline->points );

    FREE( outline );
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    ah_outline_save                                                    */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Saves the content of a given AH_Outline object into a face's glyph */
  /*    slot.                                                              */
  /*                                                                       */
  LOCAL_FUNC
  void  ah_outline_save( AH_Outline*  outline,
                         AH_Loader*   gloader )
  {
    AH_Point*   point = outline->points;
    AH_Point*   limit = point + outline->num_points;
    FT_Vector*  vec   = gloader->current.outline.points;
    char*       tag   = gloader->current.outline.tags;


    /* we assume that the glyph loader has already been checked for storage */
    for ( ; point < limit; point++, vec++, tag++ )
    {
      vec->x = point->x;
      vec->y = point->y;

      if ( point->flags & ah_flah_conic )
        tag[0] = FT_Curve_Tag_Conic;
      else if ( point->flags & ah_flah_cubic )
        tag[0] = FT_Curve_Tag_Cubic;
      else
        tag[0] = FT_Curve_Tag_On;
    }
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    ah_outline_load                                                    */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Loads an unscaled outline from a glyph slot into an AH_Outline     */
  /*    object.                                                            */
  /*                                                                       */
  LOCAL_FUNC
  FT_Error  ah_outline_load( AH_Outline*  outline,
                             FT_Face      face )
  {
    FT_Memory   memory       = outline->memory;
    FT_Error    error        = FT_Err_Ok;
    FT_Outline* source       = &face->glyph->outline;
    FT_Int      num_points   = source->n_points;
    FT_Int      num_contours = source->n_contours;
    AH_Point*   points;


    /* check arguments */
    if ( !face                                          ||
         !face->size                                    ||
         face->glyph->format != ft_glyph_format_outline )
      return FT_Err_Invalid_Argument;

    /* first of all, reallocate the contours array if necessary */
    if ( num_contours > outline->max_contours )
    {
      FT_Int  new_contours = ( num_contours + 3 ) & -4;


      if ( REALLOC_ARRAY( outline->contours, outline->max_contours,
                          new_contours, AH_Point* ) )
        goto Exit;

      outline->max_contours = new_contours;
    }

    /* then, realloc the points, segments & edges arrays if needed */
    if ( num_points > outline->max_points )
    {
      FT_Int  news = ( num_points + 7 ) & -8;
      FT_Int  max  = outline->max_points;


      if ( REALLOC_ARRAY( outline->points, max, news, AH_Point )          ||
           REALLOC_ARRAY( outline->horz_edges, max, news, AH_Edge )       ||
           REALLOC_ARRAY( outline->horz_segments, max, news, AH_Segment ) )
        goto Exit;

      /* readjust some pointers */
      outline->vert_edges    = outline->horz_edges + ( news >> 1 );
      outline->vert_segments = outline->horz_segments + ( news >> 1 );
      outline->max_points    = news;
    }

    outline->num_points   = num_points;
    outline->num_contours = num_contours;

    outline->num_hedges    = 0;
    outline->num_vedges    = 0;
    outline->num_hsegments = 0;
    outline->num_vsegments = 0;

    /* Compute the vertical and horizontal major directions; this is     */
    /* currently done by inspecting the `ft_outline_reverse_fill' flag.  */
    /* However, some fonts have improper glyphs, and it'd be a good idea */
    /* to be able to re-compute these values on the fly.                 */
    outline->vert_major_dir = ah_dir_up;
    outline->horz_major_dir = ah_dir_left;

    if ( source->flags & ft_outline_reverse_fill )
    {
      outline->vert_major_dir = ah_dir_down;
      outline->horz_major_dir = ah_dir_right;
    }

    outline->x_scale = face->size->metrics.x_scale;
    outline->y_scale = face->size->metrics.y_scale;

    points = outline->points;

    {
      /* do one thing at a time -- it is easier to understand, and */
      /* the code is clearer                                       */
      AH_Point*  point = points;
      AH_Point*  limit = point + outline->num_points;


      /* compute coordinates */
      {
        FT_Vector*  vec     = source->points;
        FT_Fixed    x_scale = outline->x_scale;
        FT_Fixed    y_scale = outline->y_scale;


        for (; point < limit; vec++, point++ )
        {
          point->fx = vec->x;
          point->fy = vec->y;
          point->ox = point->x = FT_MulFix( vec->x, x_scale );
          point->oy = point->y = FT_MulFix( vec->y, y_scale );

          point->flags = 0;
        }
      }

      /* compute Bezier flags */
      {
        char*  tag = source->tags;


        for ( point = points; point < limit; point++, tag++ )
        {
          switch ( FT_CURVE_TAG( *tag ) )
          {
          case FT_Curve_Tag_Conic:
            point->flags = ah_flah_conic; break;
          case FT_Curve_Tag_Cubic:
            point->flags = ah_flah_cubic; break;
          default:
            ;
          }
        }
      }

      /* compute `next' and `prev' */
      {
        FT_Int     contour_index;
        AH_Point*  prev;
        AH_Point*  first;
        AH_Point*  end;


        contour_index = 0;

        first = points;
        end   = points + source->contours[0];
        prev  = end;

        for ( point = points; point < limit; point++ )
        {
          point->prev = prev;
          if ( point < end )
          {
            point->next = point + 1;
            prev        = point;
          }
          else
          {
            point->next = first;
            contour_index++;
            if ( point + 1 < limit )
            {
              end   = points + source->contours[contour_index];
              first = point + 1;
              prev  = end;
            }
          }
        }
      }

      /* set-up the contours array */
      {
        AH_Point**  contour  = outline->contours;
        AH_Point**  limit    = contour + outline->num_contours;
        short*      end      = source->contours;
        short       index    = 0;


        for ( ; contour < limit; contour++, end++ )
        {
          contour[0] = points + index;
          index      = end[0] + 1;
        }
      }

      /* compute directions of in & out vectors */
      {
        for ( point = points; point < limit; point++ )
        {
          AH_Point*  prev;
          AH_Point*  next;
          FT_Vector  vec;


          prev  = point->prev;
          vec.x = point->fx - prev->fx;
          vec.y = point->fy - prev->fy;

          point->in_dir = ah_compute_direction( vec.x, vec.y );

#ifndef AH_OPTION_NO_WEAK_INTERPOLATION
          point->in_angle = ah_angle( &vec );
#endif

          next  = point->next;
          vec.x = next->fx - point->fx;
          vec.y = next->fy - point->fy;

          point->out_dir = ah_compute_direction( vec.x, vec.y );

#ifndef AH_OPTION_NO_WEAK_INTERPOLATION
          point->out_angle = ah_angle( &vec );

          {
            AH_Angle  delta = point->in_angle - point->out_angle;


            if ( delta < 0 )
              delta = -delta;
            if ( delta < 2 )
              point->flags |= ah_flah_weak_interpolation;
          }

#if 0
          if ( point->flags & ( ah_flah_conic | ah_flah_cubic ) )
            point->flags |= ah_flah_weak_interpolation;
#endif

#endif /* !AH_OPTION_NO_WEAK_INTERPOLATION */

#ifdef AH_OPTION_NO_STRONG_INTERPOLATION
          point->flags |= ah_flah_weak_interpolation;
#endif
        }
      }
    }

  Exit:
    return error;
  }


  LOCAL_FUNC
  void  ah_setup_uv( AH_Outline*  outline,
                     AH_UV        source )
  {
    AH_Point*  point = outline->points;
    AH_Point*  limit = point + outline->num_points;


    for ( ; point < limit; point++ )
    {
      FT_Pos  u, v;


      switch ( source )
      {
      case ah_uv_fxy:
        u = point->fx;
        v = point->fy;
        break;
      case ah_uv_fyx:
        u = point->fy;
        v = point->fx;
        break;
      case ah_uv_oxy:
        u = point->ox;
        v = point->oy;
        break;
      case ah_uv_oyx:
        u = point->oy;
        v = point->ox;
        break;
      case ah_uv_yx:
        u = point->y;
        v = point->x;
        break;
      case ah_uv_ox:
        u = point->x;
        v = point->ox;
        break;
      case ah_uv_oy:
        u = point->y;
        v = point->oy;
        break;
      default:
        u = point->x;
        v = point->y;
        break;
      }
      point->u = u;
      point->v = v;
    }
  }


  LOCAL_FUNC
  void  ah_outline_compute_segments( AH_Outline*  outline )
  {
    int           dimension;
    AH_Segment*   segments;
    FT_Int*       p_num_segments;
    AH_Direction  segment_dir;
    AH_Direction  major_dir;


    segments       = outline->horz_segments;
    p_num_segments = &outline->num_hsegments;
    major_dir      = ah_dir_right;      /* This value must be positive! */
    segment_dir    = major_dir;

    /* set up (u,v) in each point */
    ah_setup_uv( outline, ah_uv_fyx );

    for ( dimension = 1; dimension >= 0; dimension-- )
    {
      AH_Point**   contour       = outline->contours;
      AH_Point**   contour_limit = contour + outline->num_contours;
      AH_Segment*  segment       = segments;
      FT_Int       num_segments  = 0;

#ifdef AH_HINT_METRICS
      AH_Point*    min_point = 0;
      AH_Point*    max_point = 0;
      FT_Pos       min_coord = 32000;
      FT_Pos       max_coord = -32000;
#endif


      /* do each contour separately */
      for ( ; contour < contour_limit; contour++ )
      {
        AH_Point*  point   = contour[0];
        AH_Point*  last    = point->prev;
        int        on_edge = 0;
        FT_Pos     min_pos = +32000;  /* minimum segment pos != min_coord */
        FT_Pos     max_pos = -32000;  /* maximum segment pos != max_coord */
        FT_Bool    passed;


#ifdef AH_HINT_METRICS
        if ( point->u < min_coord )
        {
          min_coord = point->u;
          min_point = point;
        }
        if ( point->u > max_coord )
        {
          max_coord = point->u;
          max_point = point;
        }
#endif

        if ( point == last )  /* skip singletons -- just in case? */
          continue;

        if ( ABS( last->out_dir )  == major_dir &&
             ABS( point->out_dir ) == major_dir )
        {
          /* we are already on an edge, try to locate its start */
          last = point;

          for (;;)
          {
            point = point->prev;
            if ( ABS( point->out_dir ) != major_dir )
            {
              point = point->next;
              break;
            }
            if ( point == last )
              break;
          }

        }

        last   = point;
        passed = 0;

        for (;;)
        {
          FT_Pos  u, v;


          if ( on_edge )
          {
            u = point->u;
            if ( u < min_pos )
              min_pos = u;
            if ( u > max_pos )
              max_pos = u;

            if ( point->out_dir != segment_dir || point == last )
            {
              /* we are just leaving an edge; record a new segment! */
              segment->last = point;
              segment->pos  = ( min_pos + max_pos ) >> 1;

              /* a segment is round if either its first or last point */
              /* is a control point                                   */
              if ( ( segment->first->flags | point->flags ) &
                     ah_flah_control                        )
                segment->flags |= ah_edge_round;

              /* compute segment size */
              min_pos = max_pos = point->v;

              v = segment->first->v;
              if ( v < min_pos )
                min_pos = v;
              if ( v > max_pos )
                max_pos = v;

              segment->min_coord = min_pos;
              segment->max_coord = max_pos;

              on_edge = 0;
              num_segments++;
              segment++;
              /* fallthrough */
            }
          }

          /* now exit if we are at the start/end point */
          if ( point == last )
          {
            if ( passed )
              break;
            passed = 1;
          }

          if ( !on_edge && ABS( point->out_dir ) == major_dir )
          {
            /* this is the start of a new segment! */
            segment_dir = point->out_dir;

            /* clear all segment fields */
            memset( segment, 0, sizeof ( *segment ) );

            segment->dir      = segment_dir;
            segment->flags    = ah_edge_normal;
            min_pos = max_pos = point->u;
            segment->first    = point;
            segment->last     = point;
            segment->contour  = contour;
            on_edge           = 1;

            if ( point == max_point )
              max_point = 0;

            if ( point == min_point )
              min_point = 0;
          }

          point = point->next;
        }

      } /* contours */

#ifdef AH_HINT_METRICS
      /* we need to ensure that there are edges on the left-most and  */
      /* right-most points of the glyph in order to hint the metrics; */
      /* we do this by inserting fake segments when needed            */
      if ( dimension == 0 )
      {
        AH_Point*  point = outline->points;
        AH_Point*  limit = point + outline->num_points;

        AH_Point*  min_point = 0;
        AH_Point*  max_point = 0;
        FT_Pos     min_pos = 32000;
        FT_Pos     max_pos = -32000;


        /* compute minimum and maximum points */
        for ( ; point < limit; point++ )
        {
          FT_Pos  x = point->fx;


          if ( x < min_pos )
          {
            min_pos   = x;
            min_point = point;
          }
          if ( x > max_pos )
          {
            max_pos   = x;
            max_point = point;
          }
        }

        /* insert minimum segment */
        if ( min_point )
        {
          /* clear all segment fields */
          memset( segment, 0, sizeof ( *segment ) );

          segment->dir   = segment_dir;
          segment->flags = ah_edge_normal;
          segment->first = min_point;
          segment->last  = min_point;
          segment->pos   = min_pos;

          num_segments++;
          segment++;
        }

        /* insert maximum segment */
        if ( max_point )
        {
          /* clear all segment fields */
          memset( segment, 0, sizeof ( *segment ) );

          segment->dir   = segment_dir;
          segment->flags = ah_edge_normal;
          segment->first = max_point;
          segment->last  = max_point;
          segment->pos   = max_pos;

          num_segments++;
          segment++;
        }
      }
#endif /* AH_HINT_METRICS */

      *p_num_segments = num_segments;

      segments       = outline->vert_segments;
      major_dir      = ah_dir_up;
      p_num_segments = &outline->num_vsegments;
      ah_setup_uv( outline, ah_uv_fxy );
    }
  }


  LOCAL_FUNC
  void  ah_outline_link_segments( AH_Outline*  outline )
  {
    AH_Segment*  segments;
    AH_Segment*  limit;
    int          dimension;


    ah_setup_uv( outline, ah_uv_fyx );

    segments = outline->horz_segments;
    limit    = segments + outline->num_hsegments;

    for ( dimension = 1; dimension >= 0; dimension-- )
    {
      AH_Segment*  seg1;
      AH_Segment*  seg2;


      /* now compare each segment to the others */
      for ( seg1 = segments; seg1 < limit; seg1++ )
      {
        FT_Pos       best_score   = 32000;
        AH_Segment*  best_segment = 0;


        /* the fake segments are introduced to hint the metrics -- */
        /* we must never link them to anything                     */
        if ( seg1->first == seg1->last )
          continue;

        for ( seg2 = segments; seg2 < limit; seg2++ )
          if ( seg1 != seg2 && seg1->dir + seg2->dir == 0 )
          {
            FT_Pos   pos1 = seg1->pos;
            FT_Pos   pos2 = seg2->pos;
            FT_Bool  is_dir;
            FT_Bool  is_pos;


            /* check that the segments are correctly oriented and */
            /* positioned to form a black distance                */

            is_dir = ( seg1->dir == outline->horz_major_dir ||
                       seg1->dir == outline->vert_major_dir );
            is_pos = pos1 > pos2;

            if ( pos1 == pos2 || !(is_dir ^ is_pos) )
              continue;

            /* Check the two segments.  We now have a better algorithm */
            /* that doesn't rely on the segment points themselves but  */
            /* on their relative position.  This gets rids of many     */
            /* unpleasant artefacts and incorrect stem/serifs          */
            /* computations.                                           */

            /* first of all, compute the size of the `common' height */
            {
              FT_Pos  min = seg1->min_coord;
              FT_Pos  max = seg1->max_coord;
              FT_Pos  len, score;
              FT_Pos  size1, size2;


              size1 = max - min;
              size2 = seg2->max_coord - seg2->min_coord;

              if ( min < seg2->min_coord )
                min = seg2->min_coord;

              if ( max < seg2->max_coord )
                max = seg2->max_coord;

              len   = max - min;
              score = seg2->pos - seg1->pos;
              if ( score < 0 )
                score = -score;

              /* before comparing the scores, take care that the segments */
              /* are really facing each other (often not for italics..)   */
              if ( 4 * len >= size1 && 4 * len >= size2 )
                if ( score < best_score )
                {
                  best_score   = score;
                  best_segment = seg2;
                }
            }
          }

        if ( best_segment )
        {
          seg1->link  = best_segment;
          seg1->score = best_score;

          best_segment->num_linked++;
        }


      } /* edges 1 */

      /* now, compute the `serif' segments */
      for ( seg1 = segments; seg1 < limit; seg1++ )
      {
        seg2 = seg1->link;

        if ( seg2 && seg2->link != seg1 )
        {
          seg1->link  = 0;
          seg1->serif = seg2->link;
        }
      }

      ah_setup_uv( outline, ah_uv_fxy );

      segments = outline->vert_segments;
      limit    = segments + outline->num_vsegments;
    }
  }


#ifdef AH_DEBUG_GLYPH

  /* A function used to dump the array of linked segments */
  void  ah_dump_segments( AH_Outline*  outline )
  {
    AH_Segment*  segments;
    AH_Segment*  limit;
    AH_Point*    points;
    FT_Int       dimension;


    points   = outline->points;
    segments = outline->horz_segments;
    limit    = segments + outline->num_hsegments;

    for ( dimension = 1; dimension >= 0; dimension-- )
    {
      AH_Segment*  seg;


      printf ( "Table of %s segments:\n",
               !dimension ? "vertical" : "horizontal" );
      printf ( "  [ index |  pos |  dir  | link | serif |"
               " numl | first | start ]\n" );

      for ( seg = segments; seg < limit; seg++ )
      {
        printf ( "  [ %5d | %4d | %5s | %4d | %5d | %4d | %5d | %5d ]\n",
                 seg - segments,
                 (int)seg->pos,
                 seg->dir == ah_dir_up
                   ? "up"
                   : ( seg->dir == ah_dir_down
                         ? "down"
                         : ( seg->dir == ah_dir_left
                               ? "left"
                               : ( seg->dir == ah_dir_right
                                     ? "right"
                                     : "none" ) ) ),
                 seg->link ? (seg->link-segments) : -1,
                 seg->serif ? (seg->serif-segments) : -1,
                 (int)seg->num_linked,
                 seg->first - points,
                 seg->last - points );
      }

      segments = outline->vert_segments;
      limit    = segments + outline->num_vsegments;
    }
  }

#endif /* AH_DEBUG_GLYPH */


  static
  void  ah_outline_compute_edges( AH_Outline*  outline )
  {
    AH_Edge*      edges;
    AH_Segment*   segments;
    AH_Segment*   segment_limit;
    AH_Direction  up_dir;
    FT_Int*       p_num_edges;
    FT_Int        dimension;
    FT_Fixed      scale;
    FT_Pos        edge_distance_threshold;


    edges         = outline->horz_edges;
    segments      = outline->horz_segments;
    segment_limit = segments + outline->num_hsegments;
    p_num_edges   = &outline->num_hedges;
    up_dir        = ah_dir_right;
    scale         = outline->y_scale;

    for ( dimension = 1; dimension >= 0; dimension-- )
    {
      AH_Edge*     edge;
      AH_Edge*     edge_limit;  /* really == edge + num_edges */
      AH_Segment*  seg;


      /*********************************************************************/
      /*                                                                   */
      /* We will begin by generating a sorted table of edges for the       */
      /* current direction.  To do so, we simply scan each segment and try */
      /* to find an edge in our table that corresponds to its position.    */
      /*                                                                   */
      /* If no edge is found, we create and insert a new edge in the       */
      /* sorted table.  Otherwise, we simply add the segment to the edge's */
      /* list which will be processed in the second step to compute the    */
      /* edge's properties.                                                */
      /*                                                                   */
      /* Note that the edges table is sorted along the segment/edge        */
      /* position.                                                         */
      /*                                                                   */
      /*********************************************************************/

      edge_distance_threshold = FT_MulFix( outline->edge_distance_threshold,
                                           scale );
      if ( edge_distance_threshold > 64 / 4 )
        edge_distance_threshold = 64 / 4;

      edge_limit = edges;
      for ( seg = segments; seg < segment_limit; seg++ )
      {
        AH_Edge*  found = 0;


        /* look for an edge corresponding to the segment */
        for ( edge = edges; edge < edge_limit; edge++ )
        {
          FT_Pos  dist;


          dist = seg->pos - edge->fpos;
          if ( dist < 0 )
            dist = -dist;

          dist = FT_MulFix( dist, scale );
          if ( dist < edge_distance_threshold )
          {
            found = edge;
            break;
          }
        }

        if ( !found )
        {
          /* insert a new edge in the list and */
          /* sort according to the position    */
          while ( edge > edges && edge[-1].fpos > seg->pos )
          {
            edge[0] = edge[-1];
            edge--;
          }
          edge_limit++;

          /* clear all edge fields */
          memset( edge, 0, sizeof ( *edge ) );

          /* add the segment to the new edge's list */
          edge->first    = seg;
          edge->last     = seg;
          edge->fpos     = seg->pos;
          edge->opos     = edge->pos = FT_MulFix( seg->pos, scale );
          seg->edge_next = seg;
        }
        else
        {
          /* if an edge was found, simply add the segment to the edge's */
          /* list                                                       */
          seg->edge_next        = edge->first;
          edge->last->edge_next = seg;
          edge->last            = seg;
        }
      }

      *p_num_edges = edge_limit - edges;


      /*********************************************************************/
      /*                                                                   */
      /* Good, we will now compute each edge's properties according to     */
      /* segments found on its position.  Basically, these are:            */
      /*                                                                   */
      /*  - edge's main direction                                          */
      /*  - stem edge, serif edge or both (which defaults to stem then)    */
      /*  - rounded edge, straigth or both (which defaults to straight)    */
      /*  - link for edge                                                  */
      /*                                                                   */
      /*********************************************************************/

      /* first of all, set the `edge' field in each segment -- this is */
      /* required in order to compute edge links                       */
      for ( edge = edges; edge < edge_limit; edge++ )
      {
        seg = edge->first;
        if ( seg )
          do
          {
            seg->edge = edge;
            seg       = seg->edge_next;
          }
          while ( seg != edge->first );
      }

      /* now, compute each edge properties */
      for ( edge = edges; edge < edge_limit; edge++ )
      {
        int  is_round    = 0;  /* does it contain round segments?    */
        int  is_straight = 0;  /* does it contain straight segments? */
        int  ups         = 0;  /* number of upwards segments         */
        int  downs       = 0;  /* number of downwards segments       */


        seg = edge->first;

        do
        {
          FT_Bool  is_serif;


          /* check for roundness of segment */
          if ( seg->flags & ah_edge_round )
            is_round++;
          else
            is_straight++;

          /* check for segment direction */
          if ( seg->dir == up_dir )
            ups   += seg->max_coord-seg->min_coord;
          else
            downs += seg->max_coord-seg->min_coord;

          /* check for links -- if seg->serif is set, then seg->link must */
          /* be ignored                                                   */
          is_serif = seg->serif && seg->serif->edge != edge;

          if ( seg->link || is_serif )
          {
            AH_Edge*     edge2;
            AH_Segment*  seg2;


            edge2 = edge->link;
            seg2  = seg->link;

            if ( is_serif )
            {
              seg2  = seg->serif;
              edge2 = edge->serif;
            }

            if ( edge2 )
            {
              FT_Pos  edge_delta;
              FT_Pos  seg_delta;


              edge_delta = edge->fpos - edge2->fpos;
              if ( edge_delta < 0 )
                edge_delta = -edge_delta;

              seg_delta = seg->pos - seg2->pos;
              if ( seg_delta < 0 )
                seg_delta = -seg_delta;

              if ( seg_delta < edge_delta )
                edge2 = seg2->edge;
            }
            else
              edge2 = seg2->edge;

            if ( is_serif )
              edge->serif = edge2;
            else
              edge->link  = edge2;
          }

          seg = seg->edge_next;

        } while ( seg != edge->first );

        /* set the round/straight flags */
        edge->flags = ah_edge_normal;

        if ( is_straight == 0 && is_round )
          edge->flags |= ah_edge_round;

        /* set the edge's main direction */
        edge->dir = ah_dir_none;

        if ( ups > downs )
          edge->dir = up_dir;

        else if ( ups < downs )
          edge->dir = - up_dir;

        else if ( ups == downs )
          edge->dir = 0;  /* both up and down !! */

        /* gets rid of serifs if link is set                */
        /* XXX: This gets rid of many unpleasant artefacts! */
        /*      Example: the `c' in cour.pfa at size 13     */

        if ( edge->serif && edge->link )
          edge->serif = 0;
      }

      edges         = outline->vert_edges;
      segments      = outline->vert_segments;
      segment_limit = segments + outline->num_vsegments;
      p_num_edges   = &outline->num_vedges;
      up_dir        = ah_dir_up;
      scale         = outline->x_scale;
    }
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    ah_outline_detect_features                                         */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Performs feature detection on a given AH_Outline object.           */
  /*                                                                       */
  LOCAL_FUNC
  void  ah_outline_detect_features( AH_Outline*  outline )
  {
    ah_outline_compute_segments( outline );
    ah_outline_link_segments   ( outline );
    ah_outline_compute_edges   ( outline );
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    ah_outline_compute_blue_edges                                      */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Computes the `blue edges' in a given outline (i.e. those that must */
  /*    be snapped to a blue zone edge (top or bottom).                    */
  /*                                                                       */
  LOCAL_FUNC
  void  ah_outline_compute_blue_edges( AH_Outline*       outline,
                                       AH_Face_Globals*  face_globals )
  {
    AH_Edge*     edge    = outline->horz_edges;
    AH_Edge*     limit   = edge + outline->num_hedges;
    AH_Globals*  globals = &face_globals->design;
    FT_Fixed     y_scale = outline->y_scale;


    /* compute for each horizontal edge, which blue zone is closer */
    for ( ; edge < limit; edge++ )
    {
      AH_Blue  blue;
      FT_Pos*  best_blue = 0;
      FT_Pos   best_dist;  /* initial threshold */


      /* compute the initial threshold as a fraction of the EM size */
      best_dist = FT_MulFix( face_globals->face->units_per_EM / 40, y_scale );
      if ( best_dist > 64 / 4 )
        best_dist = 64 / 4;

      for ( blue = ah_blue_capital_top; blue < ah_blue_max; blue++ )
      {
        /* if it is a top zone, check for right edges -- if it is a bottom */
        /* zone, check for left edges                                      */
        /*                                                                 */
        /* of course, that's for TrueType XXX                              */
        FT_Bool  is_top_blue  = AH_IS_TOP_BLUE( blue );
        FT_Bool  is_major_dir = edge->dir == outline->horz_major_dir;


        /* if it is a top zone, the edge must be against the major    */
        /* direction; if it is a bottom zone, it must be in the major */
        /* direction                                                  */
        if ( is_top_blue ^ is_major_dir )
        {
          FT_Pos   dist;
          FT_Pos*  blue_pos = globals->blue_refs + blue;


          /* first of all, compare it to the reference position */
          dist = edge->fpos - *blue_pos;
          if ( dist < 0 )
            dist = -dist;

          dist = FT_MulFix( dist, y_scale );
          if ( dist < best_dist )
          {
            best_dist = dist;
            best_blue = blue_pos;
          }

          /* now, compare it to the overshoot position if the edge is     */
          /* rounded, and if the edge is over the reference position of a */
          /* top zone, or under the reference position of a bottom zone   */
          if ( edge->flags & ah_edge_round && dist != 0 )
          {
            FT_Bool  is_under_ref = edge->fpos < *blue_pos;


            if ( is_top_blue ^ is_under_ref )
            {
              blue_pos = globals->blue_shoots + blue;
              dist = edge->fpos - *blue_pos;
              if ( dist < 0 )
                dist = -dist;

              dist = FT_MulFix( dist, y_scale );
              if ( dist < best_dist )
              {
                best_dist = dist;
                best_blue = blue_pos;
              }
            }
          }
        }
      }

      if ( best_blue )
        edge->blue_edge = best_blue;
    }
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    ah_outline_scale_blue_edges                                        */
  /*                                                                       */
  /* <Description>                                                         */
  /*    This functions must be called before hinting in order to re-adjust */
  /*    the contents of the detected edges (basically change the `blue     */
  /*    edge' pointer from `design units' to `scaled ones').               */
  /*                                                                       */
  LOCAL_FUNC
  void  ah_outline_scale_blue_edges( AH_Outline*       outline,
                                     AH_Face_Globals*  globals )
  {
    AH_Edge*  edge  = outline->horz_edges;
    AH_Edge*  limit = edge + outline->num_hedges;
    FT_Int    delta;


    delta = globals->scaled.blue_refs - globals->design.blue_refs;

    for ( ; edge < limit; edge++ )
    {
      if ( edge->blue_edge )
        edge->blue_edge += delta;
    }
  }


#ifdef AH_DEBUG_GLYPH

  void  ah_dump_edges( AH_Outline*  outline )
  {
    AH_Edge*     edges;
    AH_Edge*     limit;
    AH_Segment*  segments;
    FT_Int       dimension;


    edges    = outline->horz_edges;
    limit    = edges + outline->num_hedges;
    segments = outline->horz_segments;

    for ( dimension = 1; dimension >= 0; dimension-- )
    {
      AH_Edge*  edge;


      printf ( "Table of %s edges:\n",
               !dimension ? "vertical" : "horizontal" );
      printf ( "  [ index |  pos |  dir  | link |"
               " serif | blue | opos  |  pos  ]\n" );

      for ( edge = edges; edge < limit; edge++ )
      {
        printf ( "  [ %5d | %4d | %5s | %4d | %5d |  %c  | %5.2f | %5.2f ]\n",
                 edge - edges,
                 (int)edge->fpos,
                 edge->dir == ah_dir_up
                   ? "up"
                   : ( edge->dir == ah_dir_down
                         ? "down"
                         : ( edge->dir == ah_dir_left
                               ? "left"
                               : ( edge->dir == ah_dir_right
                                     ? "right"
                                     : "none" ) ) ),
                 edge->link ? ( edge->link - edges ) : -1,
                 edge->serif ? ( edge->serif - edges ) : -1,
                 edge->blue_edge ? 'y' : 'n',
                 edge->opos / 64.0,
                 edge->pos / 64.0 );
      }

      edges = outline->vert_edges;
      limit = edges + outline->num_vedges;
      segments = outline->vert_segments;
    }
  }

#endif /* AH_DEBUG_GLYPH */


/* END */
