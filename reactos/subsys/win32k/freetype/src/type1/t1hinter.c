/***************************************************************************/
/*                                                                         */
/*  t1hinter.c                                                             */
/*                                                                         */
/*    Type 1 hinter (body).                                                */
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


  /*************************************************************************/
  /*                                                                       */
  /* The Hinter is in charge of fitting th scaled outline to the pixel     */
  /* grid in order to considerably improve the quality of the Type 1 font  */
  /* driver's output.                                                      */
  /*                                                                       */
  /*************************************************************************/


#include <freetype/internal/ftdebug.h>


#ifdef FT_FLAT_COMPILE

#include "t1objs.h"
#include "t1hinter.h"

#else

#include <freetype/src/type1/t1objs.h>
#include <freetype/src/type1/t1hinter.h>

#endif


  /*************************************************************************/
  /*                                                                       */
  /* The macro FT_COMPONENT is used in trace mode.  It is an implicit      */
  /* parameter of the FT_TRACE() and FT_ERROR() macros, used to print/log  */
  /* messages during execution.                                            */
  /*                                                                       */
#undef  FT_COMPONENT
#define FT_COMPONENT  trace_t1hint


#undef  ONE_PIXEL
#define ONE_PIXEL  64

#undef  ROUND
#define ROUND( x )    ( ( x + ONE_PIXEL / 2 ) & -ONE_PIXEL )

#undef  SCALE
#define SCALE( val )  FT_MulFix( val, scale )

/* various constants used to describe the alignment of a horizontal */
/* stem with regards to the blue zones                              */

#define T1_ALIGN_NONE    0
#define T1_ALIGN_BOTTOM  1
#define T1_ALIGN_TOP     2
#define T1_ALIGN_BOTH    3


  /* very simple bubble sort (not a lot of elements, mostly */
  /* pre-sorted, no need for quicksort)                     */

  static
  void  t1_sort_blues( FT_Int*  blues,
                       FT_Int   count )
  {
    FT_Int   i, swap;
    FT_Int*  cur;


    for ( i = 2; i < count; i += 2 )
    {
      cur = blues + i;
      do
      {
        if ( cur[-1] < cur[0] )
          break;

        swap = cur[-2]; cur[-2] = cur[0]; cur[0] = swap;
        swap = cur[-1]; cur[-1] = cur[1]; cur[1] = swap;
        cur -= 2;

      } while ( cur > blues );
    }
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    t1_set_blue_zones                                                  */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Sets a size object's blue zones during reset.  This will compute   */
  /*    the `snap' zone corresponding to each blue zone.                   */
  /*                                                                       */
  /* <InOut>                                                               */
  /*    size :: A handle to target size object.                            */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  /* <Note>                                                                */
  /*    This functions does the following:                                 */
  /*                                                                       */
  /*    1. It extracts the bottom and top blue zones from the face object. */
  /*                                                                       */
  /*    2. Each zone is then grown by `BlueFuzz', overlapping is           */
  /*       eliminated by adjusting the zone edges appropriately.           */
  /*                                                                       */
  /*    3. For each zone, we keep its original font units position, its    */
  /*       original scaled position, as well as its grown/adjusted edges.  */
  /*                                                                       */
  static
  FT_Error  t1_set_blue_zones( T1_Size  size )
  {
    T1_Face         face = (T1_Face)size->root.face;
    T1_Private*     priv = &face->type1.private_dict;
    FT_Int          n;
    FT_Int          blues[24];
    FT_Int          num_bottom;
    FT_Int          num_top;
    FT_Int          num_blues;
    T1_Size_Hints*  hints = size->hints;
    T1_Snap_Zone*   zone;
    FT_Pos          pix, orus;
    FT_Pos          min, max, threshold;
    FT_Fixed        scale;
    FT_Bool         is_bottom;


    /***********************************************************************/
    /*                                                                     */
    /*  copy bottom and top blue zones in local arrays                     */
    /*                                                                     */

    /* First of all, check the sizes of the /BlueValues and /OtherBlues */
    /* tables.  They all must contain an even number of arguments.      */
    if ( priv->num_other_blues & 1 ||
         priv->num_blue_values & 1 )
    {
      FT_ERROR(( "t1_set_blue_zones: odd number of blue values\n" ));
      return T1_Err_Syntax_Error;
    }

    /* copy the bottom blue zones from /OtherBlues */
    num_top    = 0;
    num_bottom = priv->num_other_blues;

    for ( n = 0; n < num_bottom; n++ )
      blues[n] = priv->other_blues[n];

    /* add the first blue zone in /BlueValues to the table */
    num_top = priv->num_blue_values - 2;
    if ( num_top >= 0 )
    {
      blues[num_bottom    ] = priv->blue_values[0];
      blues[num_bottom + 1] = priv->blue_values[1];

      num_bottom += 2;
    }

    /* sort the bottom blue zones */
    t1_sort_blues( blues, num_bottom );

    hints->num_bottom_zones = num_bottom >> 1;

    /* now copy the /BlueValues to the top of the blues array */
    if ( num_top > 0 )
    {
      for ( n = 0; n < num_top; n++ )
        blues[num_bottom + n] = priv->blue_values[n + 2];

      /* sort the top blue zones */
      t1_sort_blues( blues + num_bottom, num_top );
    }
    else
      num_top = 0;

    num_blues             = num_top + num_bottom;
    hints->num_blue_zones = ( num_blues ) >> 1;

    /***********************************************************************/
    /*                                                                     */
    /* build blue snap zones from the local blues arrays                   */
    /*                                                                     */

    scale     = size->root.metrics.y_scale;
    zone      = hints->blue_zones;
    threshold = ONE_PIXEL / 4;   /* 0.25 pixels */

    for ( n = 0; n < num_blues; n += 2, zone++ )
    {
      is_bottom = n < num_bottom ? 1 : 0;

      orus = blues[n + is_bottom];  /* get alignement coordinate */
      pix  = SCALE( orus );         /* scale it                  */

      min  = SCALE( blues[n    ] - priv->blue_fuzz );
      max  = SCALE( blues[n + 1] + priv->blue_fuzz );

      if ( min > pix - threshold )
        min = pix - threshold;
      if ( max < pix + threshold )
        max = pix + threshold;

      zone->orus = orus;
      zone->pix  = pix;
      zone->min  = min;
      zone->max  = max;
    }

    /* adjust edges in case of overlap */
    zone = hints->blue_zones;
    for ( n = 0; n < num_blues - 2; n += 2, zone++ )
    {
      if ( n != num_bottom - 2       &&
           zone[0].max > zone[1].min )
        zone[0].max = zone[1].min = ( zone[0].pix + zone[1].pix ) / 2;
    }

    /* compare the current pixel size with the BlueScale value */
    /* to know whether to supress overshoots                   */

    hints->supress_overshoots =
      size->root.metrics.y_ppem < FT_MulFix( 1000, priv->blue_scale );

#ifdef FT_DEBUG_LEVEL_TRACE

    /* now print the new blue values in tracing mode */

    FT_TRACE2(( "Blue Zones for size object at $%08lx:\n", (long)size ));
    FT_TRACE2(( "   orus    pix    min   max\n" ));
    FT_TRACE2(( "-------------------------------\n" ));

    zone = hints->blue_zones;
    for ( n = 0; n < hints->num_blue_zones; n++ )
    {
      FT_TRACE2(( "    %3d   %.2f   %.2f  %.2f\n",
                  zone->orus,
                  zone->pix / 64.0,
                  zone->min / 64.0,
                  zone->max / 64.0 ));
      zone++;
    }
    FT_TRACE2(( "\nOvershoots are %s\n\n",
                hints->supress_overshoots ? "supressed" : "active" ));

#endif /* DEBUG_LEVEL_TRACE */

    return T1_Err_Ok;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    t1_set_snap_zones                                                  */
  /*                                                                       */
  /* <Description>                                                         */
  /*    This function set a size object's stem snap zones.                 */
  /*                                                                       */
  /* <InOut>                                                               */
  /*    size :: A handle to the target size object.                        */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  /* <Note>                                                                */
  /*    This function performs the following:                              */
  /*                                                                       */
  /*    1. It reads and scales the stem snap widths from the parent face.  */
  /*                                                                       */
  /*    2. A `snap zone' is computed for each snap width, by `growing' it  */
  /*       with a threshold of 1/2 pixel.  Overlapping is avoided through  */
  /*       proper edge adjustment.                                         */
  /*                                                                       */
  /*    3. Each width whose zone contain the scaled standard set width is  */
  /*       removed from the table.                                         */
  /*                                                                       */
  /*    4. Finally, the standard set width is scaled, and its correponding */
  /*       `snap zone' is inserted into the sorted snap zones table.       */
  /*                                                                       */
  static
  FT_Error  t1_set_snap_zones( T1_Size  size )
  {
    FT_Int          n, direction, n_zones, num_zones;
    T1_Snap_Zone*   zone;
    T1_Snap_Zone*   base_zone;
    FT_Short*       orgs;
    FT_Pos          standard_width;
    FT_Fixed        scale;

    T1_Face         face  = (T1_Face)size->root.face;
    T1_Private*     priv  = &face->type1.private_dict;
    T1_Size_Hints*  hints = size->hints;


    /* start with horizontal snap zones */
    direction      = 0;
    standard_width = priv->standard_width[0];
    n_zones        = priv->num_snap_widths;
    base_zone      = hints->snap_widths;
    orgs           = priv->snap_widths;
    scale          = size->root.metrics.x_scale;

    while ( direction < 2 )
    {
      /*********************************************************************/
      /*                                                                   */
      /* Read and scale stem snap widths table from the physical font      */
      /* record.                                                           */
      /*                                                                   */

      FT_Pos  prev, orus, pix, min, max, threshold;


      threshold = ONE_PIXEL / 4;
      zone      = base_zone;

      if ( n_zones > 0 )
      {
        orus = *orgs++;
        pix  = SCALE( orus );
        min  = pix - threshold;
        max  = pix + threshold;

        zone->orus = orus;
        zone->pix  = pix;
        zone->min  = min;
        prev       = pix;

        for ( n = 1; n < n_zones; n++ )
        {
          orus = *orgs++;
          pix  = SCALE( orus );

          if ( pix - prev < 2 * threshold )
          {
            min = max = ( pix + prev ) / 2;
          }
          else
            min = pix - threshold;

          zone->max = max;
          zone++;
          zone->orus = orus;
          zone->pix  = pix;
          zone->min  = min;

          max  = pix + threshold;
          prev = pix;
        }
        zone->max = max;
      }

#ifdef FT_DEBUG_LEVEL_TRACE

      /* print the scaled stem snap values in tracing mode */

      FT_TRACE2(( "Set_Snap_Zones: first %s pass\n",
                  direction ? "vertical" : "horizontal" ));

      FT_TRACE2(( "Scaled original stem snap zones:\n" ));
      FT_TRACE2(( "   orus   pix   min   max\n" ));
      FT_TRACE2(( "-----------------------------\n" ));

      zone = base_zone;
      for ( n = 0; n < n_zones; n++, zone++ )
        FT_TRACE2(( "  %3d  %.2f  %.2f  %.2f\n",
                    zone->orus,
                    zone->pix / 64.0,
                    zone->min / 64.0,
                    zone->max / 64.0 ));
      FT_TRACE2(( "\n" ));

      FT_TRACE2(( "Standard width = %d\n", standard_width ));

#endif /* FT_DEBUG_LEVEL_TRACE */

      /*********************************************************************/
      /*                                                                   */
      /* Now, each snap width which is in the range of the standard set    */
      /* width will be removed from the list.                              */
      /*                                                                   */

      if ( standard_width > 0 )
      {
        T1_Snap_Zone*  parent;
        FT_Pos         std_pix, std_min, std_max;


        std_pix = SCALE( standard_width );

        std_min = std_pix - threshold;
        std_max = std_pix + threshold;

        num_zones = 0;
        zone      = base_zone;
        parent    = base_zone;

        for ( n = 0; n < n_zones; n++ )
        {
          if ( zone->pix >= std_min && zone->pix <= std_max )
          {
            /* this zone must be removed from the list */
            if ( std_min > zone->min )
              std_min = zone->min;
            if ( std_max < zone->max )
              std_max = zone->max;
          }
          else
          {
            *parent++ = *zone;
            num_zones++;
          }
          zone++;
        }

        /*******************************************************************/
        /*                                                                 */
        /* Now, insert the standard width zone                             */
        /*                                                                 */

        zone = base_zone + num_zones;
        while ( zone > base_zone && zone[-1].pix > std_max )
        {
          zone[0] = zone[-1];
          zone--;
        }

        /* check border zones */
        if ( zone > base_zone && zone[-1].max > std_min )
          zone[-1].max = std_min;

        if ( zone < base_zone + num_zones && zone[1].min < std_max )
          zone[1].min = std_max;

        zone->orus = standard_width;
        zone->pix  = std_pix;
        zone->min  = std_min;
        zone->max  = std_max;

        num_zones++;
      }
      else
        num_zones = n_zones;

      /* save total number of stem snaps now */
      if ( direction )
        hints->num_snap_heights = num_zones;
      else
        hints->num_snap_widths  = num_zones;

#ifdef FT_DEBUG_LEVEL_TRACE

      /* print the scaled stem snap values in tracing mode */

      FT_TRACE2(( "Set_Snap_Zones: second %s pass\n",
                  direction ? "vertical" : "horizontal" ));

      FT_TRACE2(( "Scaled clipped stem snap zones:\n" ));
      FT_TRACE2(( "   orus   pix   min   max\n" ));
      FT_TRACE2(( "-----------------------------\n" ));

      zone = base_zone;
      for ( n = 0; n < num_zones; n++, zone++ )
        FT_TRACE2(( "  %3d  %.2f  %.2f  %.2f\n",
                    zone->orus,
                    zone->pix / 64.0,
                    zone->min / 64.0,
                    zone->max / 64.0 ));
      FT_TRACE2(( "\n" ));

      FT_TRACE2(( "Standard width = %d\n", standard_width ));

#endif /* FT_DEBUG_LEVEL_TRACE */

      /* continue with vertical snap zone */
      direction++;
      standard_width = priv->standard_height[0];
      n_zones        = priv->num_snap_heights;
      base_zone      = hints->snap_heights;
      orgs           = priv->snap_heights;
      scale          = size->root.metrics.y_scale;
    }

    return T1_Err_Ok;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    T1_New_Size_Hinter                                                 */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Allocates a new hinter structure for a given size object.          */
  /*                                                                       */
  /* <InOut>                                                               */
  /*    size :: A handle to the target size object.                        */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType Error code.  0 means success.                             */
  /*                                                                       */
  LOCAL_FUNC
  FT_Error  T1_New_Size_Hinter( T1_Size  size )
  {
    FT_Memory  memory = size->root.face->memory;


    return MEM_Alloc( size->hints, sizeof ( *size->hints ) );
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    T1_Done_Size_Hinter                                                */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Releases a given size object's hinter structure.                   */
  /*                                                                       */
  /* <Input>                                                               */
  /*    size :: A handle to the target size object.                        */
  /*                                                                       */
  LOCAL_FUNC
  void  T1_Done_Size_Hinter( T1_Size  size )
  {
    FT_Memory  memory = size->root.face->memory;


    FREE( size->hints );
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    T1_Reset_Size_Hinter                                               */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Recomputes hinting information when a given size object has        */
  /*    changed its resolutions/char sizes/pixel sizes.                    */
  /*                                                                       */
  /* <InOut>                                                               */
  /*    size :: A handle to the size object.                               */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  LOCAL_FUNC
  FT_Error  T1_Reset_Size_Hinter( T1_Size  size )
  {
    return t1_set_blue_zones( size ) || t1_set_snap_zones( size );
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    T1_New_Glyph_Hinter                                                */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Allocates a new hinter structure for a given glyph slot.           */
  /*                                                                       */
  /* <InOut>                                                               */
  /*    glyph :: A handle to the target glyph slot.                        */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  LOCAL_FUNC
  FT_Error  T1_New_Glyph_Hinter( T1_GlyphSlot  glyph )
  {
    FT_Memory  memory = glyph->root.face->memory;


    return MEM_Alloc( glyph->hints, sizeof ( *glyph->hints ) );
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    T1_Done_Glyph_Hinter                                               */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Releases a given glyph slot's hinter structure.                    */
  /*                                                                       */
  /* <Input>                                                               */
  /*    glyph :: A handle to the glyph slot.                               */
  /*                                                                       */
  LOCAL_FUNC
  void  T1_Done_Glyph_Hinter( T1_GlyphSlot  glyph )
  {
    FT_Memory  memory = glyph->root.face->memory;


    FREE( glyph->hints );
  }


  /*************************************************************************/
  /*************************************************************************/
  /*************************************************************************/
  /**********                                                     **********/
  /**********               HINTED GLYPH LOADER                   **********/
  /**********                                                     **********/
  /**********    The following code is in charge of the first     **********/
  /**********    and second pass when loading a single outline    **********/
  /**********                                                     **********/
  /*************************************************************************/
  /*************************************************************************/
  /*************************************************************************/


  static
  FT_Error  t1_hinter_ignore( void )
  {
    /* do nothing, used for `dotsection' which is unsupported for now */
    return 0;
  }


  static
  FT_Error  t1_hinter_stem( T1_Builder*  builder,
                            FT_Pos       pos,
                            FT_Int       width,
                            FT_Bool      vertical )
  {
    T1_Stem_Table*   stem_table;
    T1_Stem_Hint*    stems;
    T1_Stem_Hint*    cur_stem;
    FT_Int           min, max, n, num_stems;
    FT_Bool          new_stem;
    T1_Glyph_Hints*  hinter = builder->glyph->hints;


    /* select the appropriate stem array */
    stem_table = vertical ? &hinter->vert_stems : &hinter->hori_stems;
    stems      = stem_table->stems;
    num_stems  = stem_table->num_stems;

    /* Compute minimum and maximum coord for the stem */
    min = pos + ( vertical
                ? builder->left_bearing.x
                : builder->left_bearing.y );

    if ( width >= 0 )
      max = min + width;
    else
    {
      /* a negative width indicates a `ghost' stem */
      if ( width == -21 )
        min += width;

      max = min;
    }

    /* Now scan the array.  If we find a stem with the same borders */
    /* simply activate it.                                          */
    cur_stem = stems;
    new_stem = 1;

    for ( n = 0; n < num_stems; n++, cur_stem++ )
    {
      if ( cur_stem->min_edge.orus == min &&
           cur_stem->max_edge.orus == max )
      {
        /* This stem is already in the table, simply activate it */
        if ( ( cur_stem->hint_flags & T1_HINT_FLAG_ACTIVE ) == 0 )
        {
          cur_stem->hint_flags |= T1_HINT_FLAG_ACTIVE;
          stem_table->num_active++;
        }
        new_stem = 0;
        break;
      }
    }

    /* add a new stem to the array if necessary */
    if ( new_stem )
    {
      if ( cur_stem >= stems + T1_HINTER_MAX_EDGES )
      {
        FT_ERROR(( "t1_hinter_stem: too many stems in glyph charstring\n" ));
        return T1_Err_Syntax_Error;
      }

      /* on the first pass, we record the stem, otherwise, this is */
      /* a bug in the glyph loader!                                */
      if ( builder->pass == 0 )
      {
        cur_stem->min_edge.orus = min;
        cur_stem->max_edge.orus = max;
        cur_stem->hint_flags    = T1_HINT_FLAG_ACTIVE;

        stem_table->num_stems++;
        stem_table->num_active++;
      }
      else
      {
        FT_ERROR(( "t1_hinter_stem:" ));
        FT_ERROR(( " fatal glyph loader bug -- pass2-stem\n" ));
        return T1_Err_Syntax_Error;
      }
    }

    return T1_Err_Ok;
  }


  static
  FT_Error  t1_hinter_stem3( T1_Builder*  builder,
                             FT_Pos       pos0,
                             FT_Int       width0,
                             FT_Pos       pos1,
                             FT_Int       width1,
                             FT_Pos       pos2,
                             FT_Int       width2,
                             FT_Bool      vertical )
  {
    /* For now, simply call `stem' 3 times */
    return t1_hinter_stem( builder, pos0, width0, vertical ) ||
           t1_hinter_stem( builder, pos1, width1, vertical ) ||
           t1_hinter_stem( builder, pos2, width2, vertical );
  }


  static
  FT_Error  t1_hinter_changehints( T1_Builder*  builder )
  {
    FT_Int           dimension;
    T1_Stem_Table*   stem_table;
    T1_Glyph_Hints*  hinter = builder->glyph->hints;


    /* If we are in the second pass of glyph hinting, we must     */
    /* call the function T1_Hint_Points() on the builder in order */
    /* to force the fit the latest points to the pixel grid.      */
    if ( builder->pass == 1 )
      T1_Hint_Points( builder );

    /* Simply de-activate all hints in all arrays */
    stem_table = &hinter->hori_stems;

    for ( dimension = 2; dimension > 0; dimension-- )
    {
      T1_Stem_Hint*  cur   = stem_table->stems;
      T1_Stem_Hint*  limit = cur + stem_table->num_stems;


      for ( ; cur < limit; cur++ )
        cur->hint_flags &= ~T1_HINT_FLAG_ACTIVE;

      stem_table->num_active = 0;
      stem_table = &hinter->vert_stems;
    }

    return T1_Err_Ok;
  }


  const T1_Hinter_Funcs  t1_hinter_funcs =
  {
    (T1_Hinter_ChangeHints)t1_hinter_changehints,
    (T1_Hinter_DotSection) t1_hinter_ignore,
    (T1_Hinter_Stem)       t1_hinter_stem,
    (T1_Hinter_Stem3)      t1_hinter_stem3
  };


  /*************************************************************************/
  /*************************************************************************/
  /*************************************************************************/
  /**********                                                      *********/
  /**********                                                      *********/
  /**********                STEM HINTS MANAGEMENT                 *********/
  /**********                                                      *********/
  /**********     The following code is in charge of computing     *********/
  /**********     the placement of each scaled stem hint.          *********/
  /**********                                                      *********/
  /*************************************************************************/
  /*************************************************************************/
  /*************************************************************************/


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    t1_sort_hints                                                      */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Sorta the list of active stems in increasing order, through the    */
  /*    `sort' indexing table.                                             */
  /*                                                                       */
  /* <InOut>                                                               */
  /*    table :: A stem hints table.                                       */
  /*                                                                       */
  static
  void  t1_sort_hints( T1_Stem_Table*  table )
  {
    FT_Int         num_stems  = table->num_stems;
    FT_Int         num_active = 0;
    FT_Int*        sort       = table->sort;
    T1_Stem_Hint*  stems      = table->stems;
    FT_Int         n;


    /* record active stems in sort table */
    for ( n = 0; n < num_stems; n++ )
    {
      if ( stems[n].hint_flags & T1_HINT_FLAG_ACTIVE )
        sort[num_active++] = n;
    }

    /* Now sort the indices.  There are usually very few stems, */
    /* and they are pre-sorted in 90% cases, so we choose a     */
    /* simple bubble sort (quicksort would be slower).          */
    for ( n = 1; n < num_active; n++ )
    {
      FT_Int         p   = n - 1;
      T1_Stem_Hint*  cur = stems + sort[n];


      do
      {
        FT_Int         swap;
        T1_Stem_Hint*  prev = stems + sort[p];


        /* note that by definition, the active stems cannot overlap        */
        /* so we simply compare their `min' to sort them (we could compare */
        /* their max values also; this wouldn't change anything).          */
        if ( prev->min_edge.orus <= cur->min_edge.orus )
          break;

        /* swap elements */
        swap        = sort[p    ];
        sort[p    ] = sort[p + 1];
        sort[p + 1] = swap;
        p--;

      } while ( p >= 0 );
    }

    table->num_active = num_active;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    t1_hint_horizontal_stems                                           */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Computes the location of each scaled horizontal stem hint.  This   */
  /*    takes care of the blue zones and the horizontal stem snap table.   */
  /*                                                                       */
  /* <Input>                                                               */
  /*    table     :: The horizontal stem hints table.                      */
  /*                                                                       */
  /*    hints     :: The current size's hint structure.                    */
  /*                                                                       */
  /*    blueShift :: The value of the /BlueShift as taken from the face    */
  /*                 object.                                               */
  /*                                                                       */
  /*    scale     :: The 16.16 scale used to convert outline units to      */
  /*                 26.6 pixels.                                          */
  /*                                                                       */
  /* <Note>                                                                */
  /*    For now, all stems are hinted independently from each other.  It   */
  /*    might be necessary, for better performance, to introduce the       */
  /*    notion of `controlled' hints describing things like counter-stems, */
  /*    stem3, as well as overlapping stems control.                       */
  /*                                                                       */
  static
  void  t1_hint_horizontal_stems( T1_Stem_Table*  table,
                                  T1_Size_Hints*  hints,
                                  FT_Pos          blueShift,
                                  FT_Fixed        scale )
  {
    T1_Stem_Hint*  stem      = table->stems;
    T1_Stem_Hint*  limit     = stem + table->num_stems;


    /* first of all, scale the blueShift */
    blueShift = SCALE( blueShift );

    /* then scan the horizontal stem table */
    for ( ; stem < limit; stem++ )
    {
      FT_Pos  bottom_orus = stem->min_edge.orus;
      FT_Pos  top_orus    = stem->max_edge.orus;

      FT_Pos  top_pix     = SCALE( top_orus );
      FT_Pos  bottom_pix  = SCALE( bottom_orus );
      FT_Pos  width_pix   = top_pix - bottom_pix;

      FT_Pos  bottom      = bottom_pix;
      FT_Pos  top         = top_pix;
      FT_Int  align       = T1_ALIGN_NONE;


      /*********************************************************************/
      /*                                                                   */
      /* Snap pixel width if in stem snap range                            */
      /*                                                                   */

      {
        T1_Snap_Zone*  zone       = hints->snap_heights;
        T1_Snap_Zone*  zone_limit = zone + hints->num_snap_heights;
        FT_Pos         best_dist  = 32000;
        T1_Snap_Zone*  best_zone  = 0;


        for ( ; zone < zone_limit; zone++ )
        {
          FT_Pos  dist;


          dist = width_pix - zone->min;
          if ( dist < 0 )
            dist = -dist;
          if ( dist < best_dist )
          {
            best_zone = zone;
            best_dist = dist;
          }
        }

        if ( best_zone )
        {
          if ( width_pix > best_zone->pix )
          {
            width_pix -= 0x20;
            if ( width_pix < best_zone->pix )
              width_pix = best_zone->pix;
          }
          else
          {
            width_pix += 0x20;
            if ( width_pix > best_zone->pix )
              width_pix = best_zone->pix;
          }
        }
      }

      /*********************************************************************/
      /*                                                                   */
      /* round width - minimum 1 pixel if this isn't a ghost stem          */
      /*                                                                   */

      if ( width_pix > 0 )
        width_pix = width_pix < ONE_PIXEL ? ONE_PIXEL : ROUND( width_pix );


      /*********************************************************************/
      /*                                                                   */
      /* Now check for bottom blue zones alignement                        */
      /*                                                                   */

      {
        FT_Int         num_blues  = hints->num_bottom_zones;
        T1_Snap_Zone*  blue       = hints->blue_zones;
        T1_Snap_Zone*  blue_limit = blue + num_blues;


        for ( ; blue < blue_limit; blue++ )
        {
          if ( bottom_pix < blue->min )
            break;

          if ( bottom_pix <= blue->max )
          {
            align  = T1_ALIGN_BOTTOM;
            bottom = ROUND( blue->pix );

            /* implement blue shift */
            if ( !hints->supress_overshoots )
            {
              FT_Pos  delta = blue->pix - bottom_pix;


              delta   = delta < blueShift ? 0 : ROUND( delta );
              bottom -= delta;
            }
          }
        }
      }

      /*********************************************************************/
      /*                                                                   */
      /* check for top blue zones alignement                               */
      /*                                                                   */

      {
        FT_Int         num_blues  = hints->num_blue_zones -
                                    hints->num_bottom_zones;

        T1_Snap_Zone*  blue       = hints->blue_zones +
                                    hints->num_bottom_zones;

        T1_Snap_Zone*  blue_limit = blue + num_blues;


        for ( ; blue < blue_limit; blue++ )
        {
          if ( top_pix < blue->min )
            break;

          if ( top_pix <= blue->max )
          {
            align |= T1_ALIGN_TOP;
            top    = ROUND( blue->pix );

            /* implement blue shift */
            if ( !hints->supress_overshoots )
            {
              FT_Pos  delta = top - blue->pix;


              delta  = delta < blueShift ? 0 : ROUND( delta );
              top   += delta;
            }
          }
        }
      }

      /*********************************************************************/
      /*                                                                   */
      /* compute the hinted stem position, according to its alignment      */
      /*                                                                   */

      switch ( align )
      {
      case T1_ALIGN_BOTTOM:  /* bottom zone alignment */
        bottom_pix = bottom;
        top_pix    = bottom + width_pix;
        break;

      case T1_ALIGN_TOP:     /* top zone alignment */
        top_pix    = top;
        bottom_pix = top - width_pix;
        break;

      case T1_ALIGN_BOTH:    /* bottom+top zone alignment */
        bottom_pix = bottom;
        top_pix    = top;
        break;

      default:               /* no alignment */
        /* XXX TODO: Add management of controlled stems */
        bottom = ( SCALE( bottom_orus + top_orus ) - width_pix ) / 2;

        bottom_pix = ROUND( bottom );
        top_pix    = bottom_pix + width_pix;
      }

      stem->min_edge.pix = bottom_pix;
      stem->max_edge.pix = top_pix;
    }
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    t1_hint_vertical_stems                                             */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Computes the location of each scaled vertical stem hint.  This     */
  /*    takes care of the vertical stem snap table.                        */
  /*                                                                       */
  /* <Input>                                                               */
  /*    table :: The vertical stem hints table.                            */
  /*    hints :: The current size's hint structure.                        */
  /*    scale :: The 16.16 scale used to convert outline units to          */
  /*             26.6 pixels.                                              */
  /*                                                                       */
  /* <Note>                                                                */
  /*    For now, all stems are hinted independently from each other.  It   */
  /*    might be necessary, for better performance, to introduce the       */
  /*    notion of `controlled' hints describing things like counter-stems, */
  /*    stem3 as well as overlapping stems control.                        */
  /*                                                                       */
  static
  void  t1_hint_vertical_stems( T1_Stem_Table*  table,
                                T1_Size_Hints*  hints,
                                FT_Fixed        scale )
  {
    T1_Stem_Hint*  stem  = table->stems;
    T1_Stem_Hint*  limit = stem + table->num_stems;


    for ( ; stem < limit; stem++ )
    {
      FT_Pos  stem_left  = stem->min_edge.orus;
      FT_Pos  stem_right = stem->max_edge.orus;
      FT_Pos  width_pix, left;


      width_pix = SCALE( stem_right - stem_left );

      /* Snap pixel width if in stem snap range */
      {
        T1_Snap_Zone*  zone       = hints->snap_heights;
        T1_Snap_Zone*  zone_limit = zone + hints->num_snap_heights;
        FT_Pos         best_dist  = 32000;
        T1_Snap_Zone*  best_zone  = 0;


        for ( ; zone < zone_limit; zone++ )
        {
          FT_Pos  dist;


          dist = width_pix - zone->min;
          if ( dist < 0 )
            dist = -dist;
          if ( dist < best_dist )
          {
            best_zone = zone;
            best_dist = dist;
          }
        }

        if ( best_zone )
        {
          if ( width_pix > best_zone->pix )
          {
            width_pix -= 0x20;
            if ( width_pix < best_zone->pix )
              width_pix = best_zone->pix;
          }
          else
          {
            width_pix += 0x20;
            if ( width_pix > best_zone->pix )
              width_pix = best_zone->pix;
          }
        }
      }

      /* round width - minimum 1 pixel if this isn't a ghost stem */
      if ( width_pix > 0 )
        width_pix = width_pix < ONE_PIXEL ? ONE_PIXEL
                                          : ROUND( width_pix );

      /* now place the snapped and rounded stem   */

      /* XXX TODO: implement controlled stems for the overlapping */
      /*           cases                                          */

      left = ( SCALE( stem_left + stem_right ) - width_pix ) / 2;

      stem->min_edge.pix = ROUND( left );
      stem->max_edge.pix = stem->min_edge.pix + width_pix;
    }
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    t1_hint_point                                                      */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Grid-fit a coordinate with regards to a given stem hints table.    */
  /*                                                                       */
  /* <Input>                                                               */
  /*    table :: The source stem hints table.                              */
  /*    coord :: The original coordinate, expressed in font units.         */
  /*    scale :: The 16.16 scale used to convert font units into           */
  /*             26.6 pixels.                                              */
  /*                                                                       */
  /* <Return>                                                              */
  /*    The hinted/scaled value in 26.6 pixels.                            */
  /*                                                                       */
  /* <Note>                                                                */
  /*    For now, all stems are hinted independently from each other.  It   */
  /*    might be necessary, for better performance, to introduce the       */
  /*    notion of `controlled' hints describing things like counter-stems, */
  /*    stem3 as well as overlapping stems control.                        */
  /*                                                                       */
  static
  FT_Pos  t1_hint_point( T1_Stem_Table*  table,
                         FT_Pos          coord,
                         FT_Fixed        scale )
  {
    FT_Int         num_active = table->num_active;
    FT_Int         n;
    T1_Stem_Hint*  prev = 0;
    T1_Stem_Hint*  cur  = 0;
    T1_Edge*       min;
    T1_Edge*       max;
    FT_Pos         delta;


    /* only hint when there is at least one stem defined */
    if ( num_active <= 0 )
      return SCALE( coord );

    /* scan the stem table to determine placement of coordinate */
    /* relative to the list of sorted and stems                 */
    for ( n = 0; n < num_active; n++, prev = cur )
    {
      cur = table->stems + table->sort[n];

      /* is it on the left of the current edge? */
      delta = cur->min_edge.orus - coord;
      if ( delta == 0 )
        return cur->min_edge.pix;

      if ( delta > 0 )
      {
        /* if this is the left of the first edge, simply shift */
        if ( !prev )
          return cur->min_edge.pix - SCALE( delta );

        /* otherwise, interpolate between the maximum of the */
        /* previous stem, and the minimum of the current one */
        min = &prev->max_edge;
        max = &cur->min_edge;

        goto Interpolate;
      }

      /* is it within the current edge? */
      delta = cur->max_edge.orus - coord;
      if ( delta == 0 )
        return cur->max_edge.pix;

      if ( delta > 0 )
      {
        /* interpolate within the stem */
        min = &cur->min_edge;
        max = &cur->max_edge;

        goto Interpolate;
      }
    }

    /* apparently, this coordinate is on the right of the last stem */
    delta = coord - cur->max_edge.orus;
    return cur->max_edge.pix + SCALE( delta );

  Interpolate:
    return min->pix + FT_MulDiv( coord     - min->orus,
                                 max->pix  - min->pix,
                                 max->orus - min->orus );
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*   T1_Hint_Points                                                      */
  /*                                                                       */
  /* <Description>                                                         */
  /*   This function grid-fits several points in a given Type 1 builder    */
  /*   at once.                                                            */
  /*                                                                       */
  /* <Input>                                                               */
  /*   builder :: A handle to target Type 1 builder.                       */
  /*                                                                       */
  LOCAL_FUNC
  void  T1_Hint_Points( T1_Builder*  builder )
  {
    FT_Int    first   = builder->hint_point;
    FT_Int    last    = builder->current->n_points - 1;

    T1_Size   size    = builder->size;
    FT_Fixed  scale_x = size->root.metrics.x_scale;
    FT_Fixed  scale_y = size->root.metrics.y_scale;

    T1_Glyph_Hints*  hints      = builder->glyph->hints;
    T1_Stem_Table*   hori_stems = &hints->hori_stems;
    T1_Stem_Table*   vert_stems = &hints->vert_stems;

    FT_Vector*  cur   = builder->current->points + first;
    FT_Vector*  limit = cur + last - first + 1;


    /* first of all, sort the active stem hints */
    t1_sort_hints( hori_stems );
    t1_sort_hints( vert_stems );

    for ( ; cur < limit; cur++ )
    {
      cur->x = t1_hint_point( vert_stems, cur->x, scale_x );
      cur->y = t1_hint_point( hori_stems, cur->y, scale_y );
    }

    builder->hint_point = builder->current->n_points;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    T1_Hint_Stems                                                      */
  /*                                                                       */
  /* <Description>                                                         */
  /*    This function is used to compute the location of each stem hint    */
  /*    between the first and second passes of the glyph loader on the     */
  /*    charstring.                                                        */
  /*                                                                       */
  /* <Input>                                                               */
  /*    builder :: A handle to the target builder.                         */
  /*                                                                       */
  LOCAL_FUNC
  void  T1_Hint_Stems( T1_Builder*  builder )
  {
    T1_Glyph_Hints*  hints = builder->glyph->hints;
    T1_Private*      priv  = &builder->face->type1.private_dict;

    T1_Size   size    = builder->size;
    FT_Fixed  scale_x = size->root.metrics.x_scale;
    FT_Fixed  scale_y = size->root.metrics.y_scale;


    t1_hint_horizontal_stems( &hints->hori_stems,
                              builder->size->hints,
                              priv->blue_shift,
                              scale_y );

    t1_hint_vertical_stems( &hints->vert_stems,
                            builder->size->hints,
                            scale_x );
  }


/* END */
