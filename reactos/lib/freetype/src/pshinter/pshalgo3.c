/***************************************************************************/
/*                                                                         */
/*  pshalgo3.c                                                             */
/*                                                                         */
/*    PostScript hinting algorithm 3 (body).                               */
/*                                                                         */
/*  Copyright 2001, 2002 by                                                */
/*  David Turner, Robert Wilhelm, and Werner Lemberg.                      */
/*                                                                         */
/*  This file is part of the FreeType project, and may only be used        */
/*  modified and distributed under the terms of the FreeType project       */
/*  license, LICENSE.TXT.  By continuing to use, modify, or distribute     */
/*  this file you indicate that you have read the license and              */
/*  understand and accept it fully.                                        */
/*                                                                         */
/***************************************************************************/


#include <ft2build.h>
#include FT_INTERNAL_OBJECTS_H
#include FT_INTERNAL_DEBUG_H
#include "pshalgo3.h"


#undef  FT_COMPONENT
#define FT_COMPONENT  trace_pshalgo2


#ifdef DEBUG_HINTER
  PSH3_Hint_Table  ps3_debug_hint_table = 0;
  PSH3_HintFunc    ps3_debug_hint_func  = 0;
  PSH3_Glyph       ps3_debug_glyph      = 0;
#endif


#define  COMPUTE_INFLEXS  /* compute inflection points to optimize "S" and others */
#define  STRONGER         /* slightly increase the contrast of smooth hinting */

  /*************************************************************************/
  /*************************************************************************/
  /*****                                                               *****/
  /*****                  BASIC HINTS RECORDINGS                       *****/
  /*****                                                               *****/
  /*************************************************************************/
  /*************************************************************************/

  /* return true iff two stem hints overlap */
  static FT_Int
  psh3_hint_overlap( PSH3_Hint  hint1,
                     PSH3_Hint  hint2 )
  {
    return ( hint1->org_pos + hint1->org_len >= hint2->org_pos &&
             hint2->org_pos + hint2->org_len >= hint1->org_pos );
  }


  /* destroy hints table */
  static void
  psh3_hint_table_done( PSH3_Hint_Table  table,
                        FT_Memory        memory )
  {
    FT_FREE( table->zones );
    table->num_zones = 0;
    table->zone      = 0;

    FT_FREE( table->sort );
    FT_FREE( table->hints );
    table->num_hints   = 0;
    table->max_hints   = 0;
    table->sort_global = 0;
  }


  /* deactivate all hints in a table */
  static void
  psh3_hint_table_deactivate( PSH3_Hint_Table  table )
  {
    FT_UInt    count = table->max_hints;
    PSH3_Hint  hint  = table->hints;


    for ( ; count > 0; count--, hint++ )
    {
      psh3_hint_deactivate( hint );
      hint->order = -1;
    }
  }


  /* internal function used to record a new hint */
  static void
  psh3_hint_table_record( PSH3_Hint_Table  table,
                          FT_UInt          idx )
  {
    PSH3_Hint  hint = table->hints + idx;


    if ( idx >= table->max_hints )
    {
      FT_ERROR(( "psh3_hint_table_record: invalid hint index %d\n", idx ));
      return;
    }

    /* ignore active hints */
    if ( psh3_hint_is_active( hint ) )
      return;

    psh3_hint_activate( hint );

    /* now scan the current active hint set in order to determine */
    /* if we are overlapping with another segment                 */
    {
      PSH3_Hint*  sorted = table->sort_global;
      FT_UInt     count  = table->num_hints;
      PSH3_Hint   hint2;


      hint->parent = 0;
      for ( ; count > 0; count--, sorted++ )
      {
        hint2 = sorted[0];

        if ( psh3_hint_overlap( hint, hint2 ) )
        {
          hint->parent = hint2;
          break;
        }
      }
    }

    if ( table->num_hints < table->max_hints )
      table->sort_global[table->num_hints++] = hint;
    else
      FT_ERROR(( "psh3_hint_table_record: too many sorted hints!  BUG!\n" ));
  }


  static void
  psh3_hint_table_record_mask( PSH3_Hint_Table  table,
                               PS_Mask          hint_mask )
  {
    FT_Int    mask = 0, val = 0;
    FT_Byte*  cursor = hint_mask->bytes;
    FT_UInt   idx, limit;


    limit = hint_mask->num_bits;

    for ( idx = 0; idx < limit; idx++ )
    {
      if ( mask == 0 )
      {
        val  = *cursor++;
        mask = 0x80;
      }

      if ( val & mask )
        psh3_hint_table_record( table, idx );

      mask >>= 1;
    }
  }


  /* create hints table */
  static FT_Error
  psh3_hint_table_init( PSH3_Hint_Table  table,
                        PS_Hint_Table    hints,
                        PS_Mask_Table    hint_masks,
                        PS_Mask_Table    counter_masks,
                        FT_Memory        memory )
  {
    FT_UInt   count = hints->num_hints;
    FT_Error  error;

    FT_UNUSED( counter_masks );


    /* allocate our tables */
    if ( FT_NEW_ARRAY( table->sort,  2 * count     ) ||
         FT_NEW_ARRAY( table->hints,     count     ) ||
         FT_NEW_ARRAY( table->zones, 2 * count + 1 ) )
      goto Exit;

    table->max_hints   = count;
    table->sort_global = table->sort + count;
    table->num_hints   = 0;
    table->num_zones   = 0;
    table->zone        = 0;

    /* now, initialize the "hints" array */
    {
      PSH3_Hint  write = table->hints;
      PS_Hint    read  = hints->hints;


      for ( ; count > 0; count--, write++, read++ )
      {
        write->org_pos = read->pos;
        write->org_len = read->len;
        write->flags   = read->flags;
      }
    }

    /* we now need to determine the initial "parent" stems; first  */
    /* activate the hints that are given by the initial hint masks */
    if ( hint_masks )
    {
      FT_UInt  Count = hint_masks->num_masks;
      PS_Mask  Mask  = hint_masks->masks;


      table->hint_masks = hint_masks;

      for ( ; Count > 0; Count--, Mask++ )
        psh3_hint_table_record_mask( table, Mask );
    }

    /* now, do a linear parse in case some hints were left alone */
    if ( table->num_hints != table->max_hints )
    {
      FT_UInt  Index, Count;


      FT_ERROR(( "psh3_hint_table_init: missing/incorrect hint masks!\n" ));
      Count = table->max_hints;
      for ( Index = 0; Index < Count; Index++ )
        psh3_hint_table_record( table, Index );
    }

  Exit:
    return error;
  }


  static void
  psh3_hint_table_activate_mask( PSH3_Hint_Table  table,
                                 PS_Mask          hint_mask )
  {
    FT_Int    mask = 0, val = 0;
    FT_Byte*  cursor = hint_mask->bytes;
    FT_UInt   idx, limit, count;


    limit = hint_mask->num_bits;
    count = 0;

    psh3_hint_table_deactivate( table );

    for ( idx = 0; idx < limit; idx++ )
    {
      if ( mask == 0 )
      {
        val  = *cursor++;
        mask = 0x80;
      }

      if ( val & mask )
      {
        PSH3_Hint  hint = &table->hints[idx];


        if ( !psh3_hint_is_active( hint ) )
        {
          FT_UInt     count2;

#if 0
          PSH3_Hint*  sort = table->sort;
          PSH3_Hint   hint2;


          for ( count2 = count; count2 > 0; count2--, sort++ )
          {
            hint2 = sort[0];
            if ( psh3_hint_overlap( hint, hint2 ) )
              FT_ERROR(( "psh3_hint_table_activate_mask:"
                         " found overlapping hints\n" ))
          }
#else
          count2 = 0;
#endif

          if ( count2 == 0 )
          {
            psh3_hint_activate( hint );
            if ( count < table->max_hints )
              table->sort[count++] = hint;
            else
              FT_ERROR(( "psh3_hint_tableactivate_mask:"
                         " too many active hints\n" ));
          }
        }
      }

      mask >>= 1;
    }
    table->num_hints = count;

    /* now, sort the hints; they are guaranteed to not overlap */
    /* so we can compare their "org_pos" field directly        */
    {
      FT_Int      i1, i2;
      PSH3_Hint   hint1, hint2;
      PSH3_Hint*  sort = table->sort;


      /* a simple bubble sort will do, since in 99% of cases, the hints */
      /* will be already sorted -- and the sort will be linear          */
      for ( i1 = 1; i1 < (FT_Int)count; i1++ )
      {
        hint1 = sort[i1];
        for ( i2 = i1 - 1; i2 >= 0; i2-- )
        {
          hint2 = sort[i2];

          if ( hint2->org_pos < hint1->org_pos )
            break;

          sort[i2 + 1] = hint2;
          sort[i2]     = hint1;
        }
      }
    }
  }


  /*************************************************************************/
  /*************************************************************************/
  /*****                                                               *****/
  /*****               HINTS GRID-FITTING AND OPTIMIZATION             *****/
  /*****                                                               *****/
  /*************************************************************************/
  /*************************************************************************/

#if 1
  static FT_Pos
  psh3_dimension_quantize_len( PSH_Dimension  dim,
                               FT_Pos         len,
                               FT_Bool        do_snapping )
  {
    if ( len <= 64 )
      len = 64;
    else
    {
      FT_Pos  delta = len - dim->stdw.widths[0].cur;


      if ( delta < 0 )
        delta = -delta;

      if ( delta < 40 )
      {
        len = dim->stdw.widths[0].cur;
        if ( len < 48 )
          len = 48;
      }

      if ( len < 3 * 64 )
      {
        delta = ( len & 63 );
        len  &= -64;

        if ( delta < 10 )
          len += delta;

        else if ( delta < 32 )
          len += 10;

        else if ( delta < 54 )
          len += 54;

        else
          len += delta;
      }
      else
        len = ( len + 32 ) & -64;
    }

    if ( do_snapping )
      len = ( len + 32 ) & -64;

    return  len;
  }
#endif /* 0 */


#ifdef DEBUG_HINTER

  static void
  ps3_simple_scale( PSH3_Hint_Table  table,
                    FT_Fixed         scale,
                    FT_Fixed         delta,
                    FT_Int           dimension )
  {
    PSH3_Hint  hint;
    FT_UInt    count;


    for ( count = 0; count < table->max_hints; count++ )
    {
      hint = table->hints + count;

      hint->cur_pos = FT_MulFix( hint->org_pos, scale ) + delta;
      hint->cur_len = FT_MulFix( hint->org_len, scale );

      if ( ps3_debug_hint_func )
        ps3_debug_hint_func( hint, dimension );
    }
  }

#endif /* DEBUG_HINTER */


  static FT_Fixed
  psh3_hint_snap_stem_side_delta( FT_Fixed  pos,
                                  FT_Fixed  len )
  {
    FT_Fixed  delta1 = ( ( pos + 32 ) & -64 ) - pos;
    FT_Fixed  delta2 = ( ( pos + len + 32 ) & -64  ) - pos - len;


    if ( ABS( delta1 ) <= ABS( delta2 ) )
      return delta1;
    else
      return delta2;
  }


  static void
  psh3_hint_align( PSH3_Hint    hint,
                   PSH_Globals  globals,
                   FT_Int       dimension,
                   PSH3_Glyph   glyph )
  {
    PSH_Dimension  dim   = &globals->dimension[dimension];
    FT_Fixed       scale = dim->scale_mult;
    FT_Fixed       delta = dim->scale_delta;


    if ( !psh3_hint_is_fitted( hint ) )
    {
      FT_Pos  pos = FT_MulFix( hint->org_pos, scale ) + delta;
      FT_Pos  len = FT_MulFix( hint->org_len, scale );

      FT_Int            do_snapping;
      FT_Pos            fit_len;
      PSH_AlignmentRec  align;


      /* ignore stem alignments when requested through the hint flags */
      if ( ( dimension == 0 && !glyph->do_horz_hints ) ||
           ( dimension == 1 && !glyph->do_vert_hints ) )
      {
        hint->cur_pos = pos;
        hint->cur_len = len;

        psh3_hint_set_fitted( hint );
        return;
      }

     /* perform stem snapping when requested - this is necessary
      * for monochrome and LCD hinting modes only
      */
      do_snapping = ( dimension == 0 && glyph->do_horz_snapping ) ||
                    ( dimension == 1 && glyph->do_vert_snapping );

      hint->cur_len = fit_len = len;

      /* check blue zones for horizontal stems */
      align.align     = PSH_BLUE_ALIGN_NONE;
      align.align_bot = align.align_top = 0;

      if ( dimension == 1 )
        psh_blues_snap_stem( &globals->blues,
                             hint->org_pos + hint->org_len,
                             hint->org_pos,
                             &align );

      switch ( align.align )
      {
      case PSH_BLUE_ALIGN_TOP:
        /* the top of the stem is aligned against a blue zone */
        hint->cur_pos = align.align_top - fit_len;
        break;

      case PSH_BLUE_ALIGN_BOT:
        /* the bottom of the stem is aligned against a blue zone */
        hint->cur_pos = align.align_bot;
        break;

      case PSH_BLUE_ALIGN_TOP | PSH_BLUE_ALIGN_BOT:
        /* both edges of the stem are aligned against blue zones */
        hint->cur_pos = align.align_bot;
        hint->cur_len = align.align_top - align.align_bot;
        break;

      default:
        {
          PSH3_Hint  parent = hint->parent;


          if ( parent )
          {
            FT_Pos  par_org_center, par_cur_center;
            FT_Pos  cur_org_center, cur_delta;


            /* ensure that parent is already fitted */
            if ( !psh3_hint_is_fitted( parent ) )
              psh3_hint_align( parent, globals, dimension, glyph );

            par_org_center = parent->org_pos + ( parent->org_len >> 1 );
            par_cur_center = parent->cur_pos + ( parent->cur_len >> 1 );
            cur_org_center = hint->org_pos   + ( hint->org_len   >> 1 );

            cur_delta = FT_MulFix( cur_org_center - par_org_center, scale );
            pos       = par_cur_center + cur_delta - ( len >> 1 );
          }

          hint->cur_pos = pos;
          hint->cur_len = fit_len;

         /* stem adjustment tries to snap stem widths to standard
          * ones. this is important to prevent unpleasant rounding
          * artefacts...
          */
          if ( glyph->do_stem_adjust )
          {
            if ( len <= 64 )
            {
             /* the stem is less than one pixel, we will center it
              * around the nearest pixel center
              */
#if 1
              pos = ( pos + (len >> 1) ) & -64;
#else
             /* this seems to be a bug !! */
              pos = ( pos + ( (len >> 1) & -64 ) );
#endif
              len = 64;
            }
            else
            {
              len = psh3_dimension_quantize_len( dim, len, 0 );
            }
          }

          /* now that we have a good hinted stem width, try to position */
          /* the stem along a pixel grid integer coordinate             */
          hint->cur_pos = pos + psh3_hint_snap_stem_side_delta( pos, len );
          hint->cur_len = len;
        }
      }

      if ( do_snapping )
      {
        pos = hint->cur_pos;
        len = hint->cur_len;

        if ( len < 64 )
          len = 64;
        else
          len = ( len + 32 ) & -64;

        switch ( align.align )
        {
          case PSH_BLUE_ALIGN_TOP:
            hint->cur_pos = align.align_top - len;
            hint->cur_len = len;
            break;

          case PSH_BLUE_ALIGN_BOT:
            hint->cur_len = len;
            break;

          case PSH_BLUE_ALIGN_BOT | PSH_BLUE_ALIGN_TOP:
            /* don't touch */
            break;


          default:
            hint->cur_len = len;
            if ( len & 64 )
              pos = ( ( pos + ( len >> 1 ) ) & -64 ) + 32;
            else
              pos = ( pos + ( len >> 1 ) + 32 ) & -64;

            hint->cur_pos = pos - ( len >> 1 );
            hint->cur_len = len;
        }
      }

      psh3_hint_set_fitted( hint );

#ifdef DEBUG_HINTER
      if ( ps3_debug_hint_func )
        ps3_debug_hint_func( hint, dimension );
#endif
    }
  }


#if 0  /* not used for now, experimental */

 /*
  *  A variant to perform "light" hinting (i.e. FT_RENDER_MODE_LIGHT)
  *  of stems
  */
  static void
  psh3_hint_align_light( PSH3_Hint    hint,
                         PSH_Globals  globals,
                         FT_Int       dimension,
                         PSH3_Glyph   glyph )
  {
    PSH_Dimension  dim   = &globals->dimension[dimension];
    FT_Fixed       scale = dim->scale_mult;
    FT_Fixed       delta = dim->scale_delta;


    if ( !psh3_hint_is_fitted(hint) )
    {
      FT_Pos  pos = FT_MulFix( hint->org_pos, scale ) + delta;
      FT_Pos  len = FT_MulFix( hint->org_len, scale );

      FT_Pos  fit_len;

      PSH_AlignmentRec  align;

      /* ignore stem alignments when requested through the hint flags */
      if ( ( dimension == 0 && !glyph->do_horz_hints ) ||
           ( dimension == 1 && !glyph->do_vert_hints ) )
      {
        hint->cur_pos = pos;
        hint->cur_len = len;

        psh3_hint_set_fitted( hint );
        return;
      }

      fit_len = len;

      hint->cur_len = fit_len;

      /* check blue zones for horizontal stems */
      align.align = PSH_BLUE_ALIGN_NONE;
      align.align_bot = align.align_top = 0;

      if ( dimension == 1 )
        psh_blues_snap_stem( &globals->blues,
                             hint->org_pos + hint->org_len,
                             hint->org_pos,
                             &align );

      switch ( align.align )
      {
      case PSH_BLUE_ALIGN_TOP:
        /* the top of the stem is aligned against a blue zone */
        hint->cur_pos = align.align_top - fit_len;
        break;

      case PSH_BLUE_ALIGN_BOT:
        /* the bottom of the stem is aligned against a blue zone */
        hint->cur_pos = align.align_bot;
        break;

      case PSH_BLUE_ALIGN_TOP | PSH_BLUE_ALIGN_BOT:
        /* both edges of the stem are aligned against blue zones */
        hint->cur_pos = align.align_bot;
        hint->cur_len = align.align_top - align.align_bot;
        break;

      default:
        {
          PSH3_Hint  parent = hint->parent;


          if ( parent )
          {
            FT_Pos  par_org_center, par_cur_center;
            FT_Pos  cur_org_center, cur_delta;


            /* ensure that parent is already fitted */
            if ( !psh3_hint_is_fitted( parent ) )
              psh3_hint_align_light( parent, globals, dimension, glyph );

            par_org_center = parent->org_pos + ( parent->org_len / 2);
            par_cur_center = parent->cur_pos + ( parent->cur_len / 2);
            cur_org_center = hint->org_pos   + ( hint->org_len   / 2);

            cur_delta = FT_MulFix( cur_org_center - par_org_center, scale );
            pos       = par_cur_center + cur_delta - ( len >> 1 );
          }

         /* Stems less than one pixel wide are easy - we want to
          * make them as dark as possible, so they must fall within
          * one pixel. If the stem is split between two pixels
          * then snap the edge that is nearer to the pixel boundary
          * to the pixel boundary
          */
          if (len <= 64)
          {
            if ( ( pos + len + 63 ) / 64  != pos / 64 + 1 )
              pos += psh3_hint_snap_stem_side_delta ( pos, len );
          }
         /* Position stems other to minimize the amount of mid-grays.
          * There are, in general, two positions that do this,
          * illustrated as A) and B) below.
          *
          *   +                   +                   +                   +
          *
          * A)             |--------------------------------|
          * B)   |--------------------------------|
          * C)       |--------------------------------|
          *
          * Position A) (split the excess stem equally) should be better
          * for stems of width N + f where f < 0.5
          *
          * Position B) (split the deficiency equally) should be better
          * for stems of width N + f where f > 0.5
          *
          * It turns out though that minimizing the total number of lit
          * pixels is also important, so position C), with one edge
          * aligned with a pixel boundary is actually preferable
          * to A). There are also more possibile positions for C) than
          * for A) or B), so it involves less distortion of the overall
          * character shape.
          */
          else /* len > 64 */
          {
            FT_Fixed frac_len = len & 63;
            FT_Fixed center = pos + ( len >> 1 );
            FT_Fixed delta_a, delta_b;

            if ( ( len / 64 ) & 1 )
            {
              delta_a = ( center & -64 ) + 32 - center;
              delta_b = ( ( center + 32 ) & - 64 ) - center;
            }
            else
            {
              delta_a = ( ( center + 32 ) & - 64 ) - center;
              delta_b = ( center & -64 ) + 32 - center;
            }

           /* We choose between B) and C) above based on the amount
            * of fractinal stem width; for small amounts, choose
            * C) always, for large amounts, B) always, and inbetween,
            * pick whichever one involves less stem movement.
            */
            if (frac_len < 32)
            {
              pos += psh3_hint_snap_stem_side_delta ( pos, len );
            }
            else if (frac_len < 48)
            {
              FT_Fixed side_delta = psh3_hint_snap_stem_side_delta ( pos, len );

              if ( ABS( side_delta ) < ABS( delta_b ) )
                pos += side_delta;
              else
                pos += delta_b;
            }
            else
            {
              pos += delta_b;
            }
          }

          hint->cur_pos = pos;
        }
      }  /* switch */

      psh3_hint_set_fitted( hint );

#ifdef DEBUG_HINTER
      if ( ps3_debug_hint_func )
        ps3_debug_hint_func( hint, dimension );
#endif
    }
  }

#endif /* 0 */


  static void
  psh3_hint_table_align_hints( PSH3_Hint_Table  table,
                               PSH_Globals      globals,
                               FT_Int           dimension,
                               PSH3_Glyph       glyph )
  {
    PSH3_Hint      hint;
    FT_UInt        count;

#ifdef DEBUG_HINTER

    PSH_Dimension  dim   = &globals->dimension[dimension];
    FT_Fixed       scale = dim->scale_mult;
    FT_Fixed       delta = dim->scale_delta;


    if ( ps_debug_no_vert_hints && dimension == 0 )
    {
      ps3_simple_scale( table, scale, delta, dimension );
      return;
    }

    if ( ps_debug_no_horz_hints && dimension == 1 )
    {
      ps3_simple_scale( table, scale, delta, dimension );
      return;
    }

#endif /* DEBUG_HINTER*/

    hint  = table->hints;
    count = table->max_hints;

    for ( ; count > 0; count--, hint++ )
      psh3_hint_align( hint, globals, dimension, glyph );
  }


  /*************************************************************************/
  /*************************************************************************/
  /*****                                                               *****/
  /*****                POINTS INTERPOLATION ROUTINES                  *****/
  /*****                                                               *****/
  /*************************************************************************/
  /*************************************************************************/

#define PSH3_ZONE_MIN  -3200000L
#define PSH3_ZONE_MAX  +3200000L

#define xxDEBUG_ZONES


#ifdef DEBUG_ZONES

#include <stdio.h>

  static void
  psh3_print_zone( PSH3_Zone  zone )
  {
    printf( "zone [scale,delta,min,max] = [%.3f,%.3f,%d,%d]\n",
             zone->scale / 65536.0,
             zone->delta / 64.0,
             zone->min,
             zone->max );
  }

#else

#define psh3_print_zone( x )  do { } while ( 0 )

#endif /* DEBUG_ZONES */


  /*************************************************************************/
  /*************************************************************************/
  /*****                                                               *****/
  /*****                    HINTER GLYPH MANAGEMENT                    *****/
  /*****                                                               *****/
  /*************************************************************************/
  /*************************************************************************/

#ifdef COMPUTE_INFLEXS

  /* compute all inflex points in a given glyph */
  static void
  psh3_glyph_compute_inflections( PSH3_Glyph  glyph )
  {
    FT_UInt  n;


    for ( n = 0; n < glyph->num_contours; n++ )
    {
      PSH3_Point  first, start, end, before, after;
      FT_Angle    angle_in, angle_seg, angle_out;
      FT_Angle    diff_in, diff_out;
      FT_Int      finished = 0;


      /* we need at least 4 points to create an inflection point */
      if ( glyph->contours[n].count < 4 )
        continue;

      /* compute first segment in contour */
      first = glyph->contours[n].start;

      start = end = first;
      do
      {
        end = end->next;
        if ( end == first )
          goto Skip;

      } while ( PSH3_POINT_EQUAL_ORG( end, first ) );

      angle_seg = PSH3_POINT_ANGLE( start, end );

      /* extend the segment start whenever possible */
      before = start;
      do
      {
        do
        {
          start  = before;
          before = before->prev;
          if ( before == first )
            goto Skip;

        } while ( PSH3_POINT_EQUAL_ORG( before, start ) );

        angle_in = PSH3_POINT_ANGLE( before, start );

      } while ( angle_in == angle_seg );

      first   = start;
      diff_in = FT_Angle_Diff( angle_in, angle_seg );

      /* now, process all segments in the contour */
      do
      {
        /* first, extend current segment's end whenever possible */
        after = end;
        do
        {
          do
          {
            end   = after;
            after = after->next;
            if ( after == first )
              finished = 1;

          } while ( PSH3_POINT_EQUAL_ORG( end, after ) );

          angle_out = PSH3_POINT_ANGLE( end, after );

        } while ( angle_out == angle_seg );

        diff_out = FT_Angle_Diff( angle_seg, angle_out );

        if ( ( diff_in ^ diff_out ) < 0 )
        {
          /* diff_in and diff_out have different signs, we have */
          /* inflection points here...                          */

          do
          {
            psh3_point_set_inflex( start );
            start = start->next;
          }
          while ( start != end );

          psh3_point_set_inflex( start );
        }

        start     = end;
        end       = after;
        angle_seg = angle_out;
        diff_in   = diff_out;

      } while ( !finished );

    Skip:
      ;
    }
  }

#endif /* COMPUTE_INFLEXS */


  static void
  psh3_glyph_done( PSH3_Glyph  glyph )
  {
    FT_Memory  memory = glyph->memory;


    psh3_hint_table_done( &glyph->hint_tables[1], memory );
    psh3_hint_table_done( &glyph->hint_tables[0], memory );

    FT_FREE( glyph->points );
    FT_FREE( glyph->contours );

    glyph->num_points   = 0;
    glyph->num_contours = 0;

    glyph->memory = 0;
  }


  static int
  psh3_compute_dir( FT_Pos  dx,
                    FT_Pos  dy )
  {
    FT_Pos  ax, ay;
    int     result = PSH3_DIR_NONE;


    ax = ( dx >= 0 ) ? dx : -dx;
    ay = ( dy >= 0 ) ? dy : -dy;

    if ( ay * 12 < ax )
    {
      /* |dy| <<< |dx|  means a near-horizontal segment */
      result = ( dx >= 0 ) ? PSH3_DIR_RIGHT : PSH3_DIR_LEFT;
    }
    else if ( ax * 12 < ay )
    {
      /* |dx| <<< |dy|  means a near-vertical segment */
      result = ( dy >= 0 ) ? PSH3_DIR_UP : PSH3_DIR_DOWN;
    }

    return result;
  }


  /* load outline point coordinates into hinter glyph */
  static void
  psh3_glyph_load_points( PSH3_Glyph  glyph,
                          FT_Int      dimension )
  {
    FT_Vector*  vec   = glyph->outline->points;
    PSH3_Point  point = glyph->points;
    FT_UInt     count = glyph->num_points;


    for ( ; count > 0; count--, point++, vec++ )
    {
      point->flags2 = 0;
      point->hint   = NULL;
      if ( dimension == 0 )
      {
        point->org_u = vec->x;
        point->org_v = vec->y;
      }
      else
      {
        point->org_u = vec->y;
        point->org_v = vec->x;
      }

#ifdef DEBUG_HINTER
      point->org_x = vec->x;
      point->org_y = vec->y;
#endif

    }
  }


  /* save hinted point coordinates back to outline */
  static void
  psh3_glyph_save_points( PSH3_Glyph  glyph,
                          FT_Int      dimension )
  {
    FT_UInt     n;
    PSH3_Point  point = glyph->points;
    FT_Vector*  vec   = glyph->outline->points;
    char*       tags  = glyph->outline->tags;


    for ( n = 0; n < glyph->num_points; n++ )
    {
      if ( dimension == 0 )
        vec[n].x = point->cur_u;
      else
        vec[n].y = point->cur_u;

      if ( psh3_point_is_strong( point ) )
        tags[n] |= (char)( ( dimension == 0 ) ? 32 : 64 );

#ifdef DEBUG_HINTER

      if ( dimension == 0 )
      {
        point->cur_x   = point->cur_u;
        point->flags_x = point->flags2 | point->flags;
      }
      else
      {
        point->cur_y   = point->cur_u;
        point->flags_y = point->flags2 | point->flags;
      }

#endif

      point++;
    }
  }


  static FT_Error
  psh3_glyph_init( PSH3_Glyph   glyph,
                   FT_Outline*  outline,
                   PS_Hints     ps_hints,
                   PSH_Globals  globals )
  {
    FT_Error   error;
    FT_Memory  memory;


    /* clear all fields */
    FT_MEM_ZERO( glyph, sizeof ( *glyph ) );

    memory = globals->memory;

    /* allocate and setup points + contours arrays */
    if ( FT_NEW_ARRAY( glyph->points,   outline->n_points   ) ||
         FT_NEW_ARRAY( glyph->contours, outline->n_contours ) )
      goto Exit;

    glyph->num_points   = outline->n_points;
    glyph->num_contours = outline->n_contours;

    {
      FT_UInt       first = 0, next, n;
      PSH3_Point    points  = glyph->points;
      PSH3_Contour  contour = glyph->contours;


      for ( n = 0; n < glyph->num_contours; n++ )
      {
        FT_Int      count;
        PSH3_Point  point;


        next  = outline->contours[n] + 1;
        count = next - first;

        contour->start = points + first;
        contour->count = (FT_UInt)count;

        if ( count > 0 )
        {
          point = points + first;

          point->prev    = points + next - 1;
          point->contour = contour;

          for ( ; count > 1; count-- )
          {
            point[0].next = point + 1;
            point[1].prev = point;
            point++;
            point->contour = contour;
          }
          point->next = points + first;
        }

        contour++;
        first = next;
      }
    }

    {
      PSH3_Point  points = glyph->points;
      PSH3_Point  point  = points;
      FT_Vector*  vec    = outline->points;
      FT_UInt     n;


      for ( n = 0; n < glyph->num_points; n++, point++ )
      {
        FT_Int  n_prev = point->prev - points;
        FT_Int  n_next = point->next - points;
        FT_Pos  dxi, dyi, dxo, dyo;


        if ( !( outline->tags[n] & FT_CURVE_TAG_ON ) )
          point->flags = PSH3_POINT_OFF;

        dxi = vec[n].x - vec[n_prev].x;
        dyi = vec[n].y - vec[n_prev].y;

        point->dir_in = (FT_Char)psh3_compute_dir( dxi, dyi );

        dxo = vec[n_next].x - vec[n].x;
        dyo = vec[n_next].y - vec[n].y;

        point->dir_out = (FT_Char)psh3_compute_dir( dxo, dyo );

        /* detect smooth points */
        if ( point->flags & PSH3_POINT_OFF )
          point->flags |= PSH3_POINT_SMOOTH;
        else if ( point->dir_in  != PSH3_DIR_NONE ||
                  point->dir_out != PSH3_DIR_NONE )
        {
          if ( point->dir_in == point->dir_out )
            point->flags |= PSH3_POINT_SMOOTH;
        }
        else
        {
          FT_Angle  angle_in, angle_out, diff;


          angle_in  = FT_Atan2( dxi, dyi );
          angle_out = FT_Atan2( dxo, dyo );

          diff = angle_in - angle_out;
          if ( diff < 0 )
            diff = -diff;

          if ( diff > FT_ANGLE_PI )
            diff = FT_ANGLE_2PI - diff;

          if ( diff < FT_ANGLE_PI / 16 )
            point->flags |= PSH3_POINT_SMOOTH;
        }
      }
    }

    glyph->memory  = memory;
    glyph->outline = outline;
    glyph->globals = globals;

#ifdef COMPUTE_INFLEXS
    psh3_glyph_load_points( glyph, 0 );
    psh3_glyph_compute_inflections( glyph );
#endif /* COMPUTE_INFLEXS */

    /* now deal with hints tables */
    error = psh3_hint_table_init( &glyph->hint_tables [0],
                                  &ps_hints->dimension[0].hints,
                                  &ps_hints->dimension[0].masks,
                                  &ps_hints->dimension[0].counters,
                                  memory );
    if ( error )
      goto Exit;

    error = psh3_hint_table_init( &glyph->hint_tables [1],
                                  &ps_hints->dimension[1].hints,
                                  &ps_hints->dimension[1].masks,
                                  &ps_hints->dimension[1].counters,
                                  memory );
    if ( error )
      goto Exit;

  Exit:
    return error;
  }


  /* compute all extrema in a glyph for a given dimension */
  static void
  psh3_glyph_compute_extrema( PSH3_Glyph  glyph )
  {
    FT_UInt  n;


    /* first of all, compute all local extrema */
    for ( n = 0; n < glyph->num_contours; n++ )
    {
      PSH3_Point  first = glyph->contours[n].start;
      PSH3_Point  point, before, after;


      point  = first;
      before = point;
      after  = point;

      do
      {
        before = before->prev;
        if ( before == first )
          goto Skip;

      } while ( before->org_u == point->org_u );

      first = point = before->next;

      for (;;)
      {
        after = point;
        do
        {
          after = after->next;
          if ( after == first )
            goto Next;

        } while ( after->org_u == point->org_u );

        if ( before->org_u < point->org_u )
        {
          if ( after->org_u < point->org_u )
          {
            /* local maximum */
            goto Extremum;
          }
        }
        else /* before->org_u > point->org_u */
        {
          if ( after->org_u > point->org_u )
          {
            /* local minimum */
          Extremum:
            do
            {
              psh3_point_set_extremum( point );
              point = point->next;

            } while ( point != after );
          }
        }

        before = after->prev;
        point  = after;

      } /* for  */

    Next:
      ;
    }

    /* for each extrema, determine its direction along the */
    /* orthogonal axis                                     */
    for ( n = 0; n < glyph->num_points; n++ )
    {
      PSH3_Point  point, before, after;


      point  = &glyph->points[n];
      before = point;
      after  = point;

      if ( psh3_point_is_extremum( point ) )
      {
        do
        {
          before = before->prev;
          if ( before == point )
            goto Skip;

        } while ( before->org_v == point->org_v );

        do
        {
          after = after->next;
          if ( after == point )
            goto Skip;

        } while ( after->org_v == point->org_v );
      }

      if ( before->org_v < point->org_v &&
           after->org_v  > point->org_v )
      {
        psh3_point_set_positive( point );
      }
      else if ( before->org_v > point->org_v &&
                after->org_v  < point->org_v )
      {
        psh3_point_set_negative( point );
      }

    Skip:
      ;
    }
  }


#define PSH3_STRONG_THRESHOLD  30


  /* major_dir is the direction for points on the bottom/left of the stem; */
  /* Points on the top/right of the stem will have a direction of          */
  /* -major_dir.                                                           */

  static void
  psh3_hint_table_find_strong_point( PSH3_Hint_Table  table,
                                     PSH3_Point       point,
                                     FT_Int           major_dir )
  {
    PSH3_Hint*  sort      = table->sort;
    FT_UInt     num_hints = table->num_hints;
    FT_Int      point_dir = 0;


    if ( PSH3_DIR_COMPARE( point->dir_in, major_dir ) )
      point_dir = point->dir_in;

    else if ( PSH3_DIR_COMPARE( point->dir_out, major_dir ) )
      point_dir = point->dir_out;

    if ( point_dir )
    {
      FT_UInt  flag;


      for ( ; num_hints > 0; num_hints--, sort++ )
      {
        PSH3_Hint  hint = sort[0];
        FT_Pos     d;


        if ( point_dir == major_dir )
        {
          flag = PSH3_POINT_EDGE_MIN;
          d    = point->org_u - hint->org_pos;

          if ( ABS( d ) < PSH3_STRONG_THRESHOLD )
          {
          Is_Strong:
            psh3_point_set_strong( point );
            point->flags2 |= flag;
            point->hint    = hint;
            break;
          }
        }
        else if ( point_dir == -major_dir )
        {
          flag  = PSH3_POINT_EDGE_MAX;
          d     = point->org_u - hint->org_pos - hint->org_len;

          if ( ABS( d ) < PSH3_STRONG_THRESHOLD )
            goto Is_Strong;
        }
      }
    }

#if 1
    else if ( psh3_point_is_extremum( point ) )
    {
      /* treat extrema as special cases for stem edge alignment */
      FT_UInt  min_flag, max_flag;


      if ( major_dir == PSH3_DIR_HORIZONTAL )
      {
        min_flag = PSH3_POINT_POSITIVE;
        max_flag = PSH3_POINT_NEGATIVE;
      }
      else
      {
        min_flag = PSH3_POINT_NEGATIVE;
        max_flag = PSH3_POINT_POSITIVE;
      }

      for ( ; num_hints > 0; num_hints--, sort++ )
      {
        PSH3_Hint  hint = sort[0];
        FT_Pos     d, flag;


        if ( point->flags2 & min_flag )
        {
          flag = PSH3_POINT_EDGE_MIN;
          d    = point->org_u - hint->org_pos;

          if ( ABS( d ) < PSH3_STRONG_THRESHOLD )
          {
          Is_Strong2:
            point->flags2 |= flag;
            point->hint    = hint;
            psh3_point_set_strong( point );
            break;
          }
        }
        else if ( point->flags2 & max_flag )
        {
          flag = PSH3_POINT_EDGE_MAX;
          d    = point->org_u - hint->org_pos - hint->org_len;

          if ( ABS( d ) < PSH3_STRONG_THRESHOLD )
            goto Is_Strong2;
        }

        if ( point->org_u >= hint->org_pos                 &&
             point->org_u <= hint->org_pos + hint->org_len )
        {
          point->hint = hint;
        }
      }
    }

#endif /* 1 */
  }


  /* find strong points in a glyph */
  static void
  psh3_glyph_find_strong_points( PSH3_Glyph  glyph,
                                 FT_Int      dimension )
  {
    /* a point is strong if it is located on a stem                   */
    /* edge and has an "in" or "out" tangent to the hint's direction  */
    {
      PSH3_Hint_Table  table     = &glyph->hint_tables[dimension];
      PS_Mask          mask      = table->hint_masks->masks;
      FT_UInt          num_masks = table->hint_masks->num_masks;
      FT_UInt          first     = 0;
      FT_Int           major_dir = dimension == 0 ? PSH3_DIR_VERTICAL
                                                  : PSH3_DIR_HORIZONTAL;


      /* process secondary hints to "selected" points */
      if ( num_masks > 1 && glyph->num_points > 0 )
      {
        first = mask->end_point;
        mask++;
        for ( ; num_masks > 1; num_masks--, mask++ )
        {
          FT_UInt  next;
          FT_Int   count;


          next  = mask->end_point;
          count = next - first;
          if ( count > 0 )
          {
            PSH3_Point  point = glyph->points + first;


            psh3_hint_table_activate_mask( table, mask );

            for ( ; count > 0; count--, point++ )
              psh3_hint_table_find_strong_point( table, point, major_dir );
          }
          first = next;
        }
      }

      /* process primary hints for all points */
      if ( num_masks == 1 )
      {
        FT_UInt     count = glyph->num_points;
        PSH3_Point  point = glyph->points;


        psh3_hint_table_activate_mask( table, table->hint_masks->masks );
        for ( ; count > 0; count--, point++ )
        {
          if ( !psh3_point_is_strong( point ) )
            psh3_hint_table_find_strong_point( table, point, major_dir );
        }
      }

      /* now, certain points may have been attached to hint and */
      /* not marked as strong; update their flags then          */
      {
        FT_UInt     count = glyph->num_points;
        PSH3_Point  point = glyph->points;


        for ( ; count > 0; count--, point++ )
          if ( point->hint && !psh3_point_is_strong( point ) )
            psh3_point_set_strong( point );
      }
    }
  }


  /* interpolate strong points with the help of hinted coordinates */
  static void
  psh3_glyph_interpolate_strong_points( PSH3_Glyph  glyph,
                                        FT_Int      dimension )
  {
    PSH_Dimension  dim   = &glyph->globals->dimension[dimension];
    FT_Fixed       scale = dim->scale_mult;


    {
      FT_UInt     count = glyph->num_points;
      PSH3_Point  point = glyph->points;


      for ( ; count > 0; count--, point++ )
      {
        PSH3_Hint  hint = point->hint;


        if ( hint )
        {
          FT_Pos  delta;


          if ( psh3_point_is_edge_min( point ) )
          {
            point->cur_u = hint->cur_pos;
          }
          else if ( psh3_point_is_edge_max( point ) )
          {
            point->cur_u = hint->cur_pos + hint->cur_len;
          }
          else
          {
            delta = point->org_u - hint->org_pos;

            if ( delta <= 0 )
              point->cur_u = hint->cur_pos + FT_MulFix( delta, scale );

            else if ( delta >= hint->org_len )
              point->cur_u = hint->cur_pos + hint->cur_len +
                               FT_MulFix( delta - hint->org_len, scale );

            else if ( hint->org_len > 0 )
              point->cur_u = hint->cur_pos +
                               FT_MulDiv( delta, hint->cur_len,
                                          hint->org_len );
            else
              point->cur_u = hint->cur_pos;
          }
          psh3_point_set_fitted( point );
        }
      }
    }
  }


  static void
  psh3_glyph_interpolate_normal_points( PSH3_Glyph  glyph,
                                        FT_Int      dimension )
  {

#if 1

    PSH_Dimension  dim   = &glyph->globals->dimension[dimension];
    FT_Fixed       scale = dim->scale_mult;


    /* first technique: a point is strong if it is a local extrema */
    {
      FT_UInt     count = glyph->num_points;
      PSH3_Point  point = glyph->points;


      for ( ; count > 0; count--, point++ )
      {
        if ( psh3_point_is_strong( point ) )
          continue;

        /* sometimes, some local extremas are smooth points */
        if ( psh3_point_is_smooth( point ) )
        {
          if ( point->dir_in == PSH3_DIR_NONE  ||
               point->dir_in != point->dir_out )
            continue;

          if ( !psh3_point_is_extremum( point )   &&
               !psh3_point_is_inflex( point ) )
            continue;

          point->flags &= ~PSH3_POINT_SMOOTH;
        }

        /* find best enclosing point coordinates */
        {
          PSH3_Point  before = 0;
          PSH3_Point  after  = 0;

          FT_Pos      diff_before = -32000;
          FT_Pos      diff_after  =  32000;
          FT_Pos      u = point->org_u;

          FT_Int      count2 = glyph->num_points;
          PSH3_Point  cur    = glyph->points;


          for ( ; count2 > 0; count2--, cur++ )
          {
            if ( psh3_point_is_strong( cur ) )
            {
              FT_Pos  diff = cur->org_u - u;;


              if ( diff <= 0 )
              {
                if ( diff > diff_before )
                {
                  diff_before = diff;
                  before      = cur;
                }
              }
              else if ( diff >= 0 )
              {
                if ( diff < diff_after )
                {
                  diff_after = diff;
                  after      = cur;
                }
              }
            }
          }

          if ( !before )
          {
            if ( !after )
              continue;

            /* we are before the first strong point coordinate; */
            /* simply translate the point                       */
            point->cur_u = after->cur_u +
                           FT_MulFix( point->org_u - after->org_u, scale );
          }
          else if ( !after )
          {
            /* we are after the last strong point coordinate; */
            /* simply translate the point                     */
            point->cur_u = before->cur_u +
                           FT_MulFix( point->org_u - before->org_u, scale );
          }
          else
          {
            if ( diff_before == 0 )
              point->cur_u = before->cur_u;

            else if ( diff_after == 0 )
              point->cur_u = after->cur_u;

            else
              point->cur_u = before->cur_u +
                             FT_MulDiv( u - before->org_u,
                                        after->cur_u - before->cur_u,
                                        after->org_u - before->org_u );
          }

          psh3_point_set_fitted( point );
        }
      }
    }

#endif /* 1 */

  }


  /* interpolate other points */
  static void
  psh3_glyph_interpolate_other_points( PSH3_Glyph  glyph,
                                       FT_Int      dimension )
  {
    PSH_Dimension  dim          = &glyph->globals->dimension[dimension];
    FT_Fixed       scale        = dim->scale_mult;
    FT_Fixed       delta        = dim->scale_delta;
    PSH3_Contour   contour      = glyph->contours;
    FT_UInt        num_contours = glyph->num_contours;


    for ( ; num_contours > 0; num_contours--, contour++ )
    {
      PSH3_Point  start = contour->start;
      PSH3_Point  first, next, point;
      FT_UInt     fit_count;


      /* count the number of strong points in this contour */
      next      = start + contour->count;
      fit_count = 0;
      first     = 0;

      for ( point = start; point < next; point++ )
        if ( psh3_point_is_fitted( point ) )
        {
          if ( !first )
            first = point;

          fit_count++;
        }

      /* if there are less than 2 fitted points in the contour, we */
      /* simply scale and eventually translate the contour points  */
      if ( fit_count < 2 )
      {
        if ( fit_count == 1 )
          delta = first->cur_u - FT_MulFix( first->org_u, scale );

        for ( point = start; point < next; point++ )
          if ( point != first )
            point->cur_u = FT_MulFix( point->org_u, scale ) + delta;

        goto Next_Contour;
      }

      /* there are more than 2 strong points in this contour; we */
      /* need to interpolate weak points between them            */
      start = first;
      do
      {
        point = first;

        /* skip consecutive fitted points */
        for (;;)
        {
          next = first->next;
          if ( next == start )
            goto Next_Contour;

          if ( !psh3_point_is_fitted( next ) )
            break;

          first = next;
        }

        /* find next fitted point after unfitted one */
        for (;;)
        {
          next = next->next;
          if ( psh3_point_is_fitted( next ) )
            break;
        }

        /* now interpolate between them */
        {
          FT_Pos    org_a, org_ab, cur_a, cur_ab;
          FT_Pos    org_c, org_ac, cur_c;
          FT_Fixed  scale_ab;


          if ( first->org_u <= next->org_u )
          {
            org_a  = first->org_u;
            cur_a  = first->cur_u;
            org_ab = next->org_u - org_a;
            cur_ab = next->cur_u - cur_a;
          }
          else
          {
            org_a  = next->org_u;
            cur_a  = next->cur_u;
            org_ab = first->org_u - org_a;
            cur_ab = first->cur_u - cur_a;
          }

          scale_ab = 0x10000L;
          if ( org_ab > 0 )
            scale_ab = FT_DivFix( cur_ab, org_ab );

          point = first->next;
          do
          {
            org_c  = point->org_u;
            org_ac = org_c - org_a;

            if ( org_ac <= 0 )
            {
              /* on the left of the interpolation zone */
              cur_c = cur_a + FT_MulFix( org_ac, scale );
            }
            else if ( org_ac >= org_ab )
            {
              /* on the right on the interpolation zone */
              cur_c = cur_a + cur_ab + FT_MulFix( org_ac - org_ab, scale );
            }
            else
            {
              /* within the interpolation zone */
              cur_c = cur_a + FT_MulFix( org_ac, scale_ab );
            }

            point->cur_u = cur_c;

            point = point->next;

          } while ( point != next );
        }

        /* keep going until all points in the contours have been processed */
        first = next;

      } while ( first != start );

    Next_Contour:
      ;
    }
  }


  /*************************************************************************/
  /*************************************************************************/
  /*****                                                               *****/
  /*****                     HIGH-LEVEL INTERFACE                      *****/
  /*****                                                               *****/
  /*************************************************************************/
  /*************************************************************************/

  FT_Error
  ps3_hints_apply( PS_Hints        ps_hints,
                   FT_Outline*     outline,
                   PSH_Globals     globals,
                   FT_Render_Mode  hint_mode )
  {
    PSH3_GlyphRec  glyphrec;
    PSH3_Glyph     glyph = &glyphrec;
    FT_Error       error;
#ifdef DEBUG_HINTER
    FT_Memory      memory;
#endif
    FT_Int         dimension;


#ifdef DEBUG_HINTER

    memory = globals->memory;

    if ( ps3_debug_glyph )
    {
      psh3_glyph_done( ps3_debug_glyph );
      FT_FREE( ps3_debug_glyph );
    }

    if ( FT_NEW( glyph ) )
      return error;

    ps3_debug_glyph = glyph;

#endif /* DEBUG_HINTER */

    error = psh3_glyph_init( glyph, outline, ps_hints, globals );
    if ( error )
      goto Exit;

    glyph->do_horz_hints = 1;
    glyph->do_vert_hints = 1;

    glyph->do_horz_snapping = FT_BOOL( hint_mode == FT_RENDER_MODE_MONO ||
                                       hint_mode == FT_RENDER_MODE_LCD  );

    glyph->do_vert_snapping = FT_BOOL( hint_mode == FT_RENDER_MODE_MONO  ||
                                       hint_mode == FT_RENDER_MODE_LCD_V );

    glyph->do_stem_adjust   = FT_BOOL( hint_mode != FT_RENDER_MODE_LIGHT );

    for ( dimension = 0; dimension < 2; dimension++ )
    {
      /* load outline coordinates into glyph */
      psh3_glyph_load_points( glyph, dimension );

      /* compute local extrema */
      psh3_glyph_compute_extrema( glyph );

      /* compute aligned stem/hints positions */
      psh3_hint_table_align_hints( &glyph->hint_tables[dimension],
                                   glyph->globals,
                                   dimension,
                                   glyph );

      /* find strong points, align them, then interpolate others */
      psh3_glyph_find_strong_points( glyph, dimension );
      psh3_glyph_interpolate_strong_points( glyph, dimension );
      psh3_glyph_interpolate_normal_points( glyph, dimension );
      psh3_glyph_interpolate_other_points( glyph, dimension );

      /* save hinted coordinates back to outline */
      psh3_glyph_save_points( glyph, dimension );
    }

  Exit:

#ifndef DEBUG_HINTER
    psh3_glyph_done( glyph );
#endif

    return error;
  }


/* END */
