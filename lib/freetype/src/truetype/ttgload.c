/***************************************************************************/
/*                                                                         */
/*  ttgload.c                                                              */
/*                                                                         */
/*    TrueType Glyph Loader (body).                                        */
/*                                                                         */
/*  Copyright 1996-2001, 2002, 2003, 2004, 2005 by                         */
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
#include FT_INTERNAL_DEBUG_H
#include FT_INTERNAL_CALC_H
#include FT_INTERNAL_STREAM_H
#include FT_INTERNAL_SFNT_H
#include FT_TRUETYPE_TAGS_H
#include FT_OUTLINE_H

#include "ttgload.h"
#include "ttpload.h"

#ifdef TT_CONFIG_OPTION_GX_VAR_SUPPORT
#include "ttgxvar.h"
#endif

#include "tterrors.h"


  /*************************************************************************/
  /*                                                                       */
  /* The macro FT_COMPONENT is used in trace mode.  It is an implicit      */
  /* parameter of the FT_TRACE() and FT_ERROR() macros, used to print/log  */
  /* messages during execution.                                            */
  /*                                                                       */
#undef  FT_COMPONENT
#define FT_COMPONENT  trace_ttgload


  /*************************************************************************/
  /*                                                                       */
  /* Composite font flags.                                                 */
  /*                                                                       */
#define ARGS_ARE_WORDS             0x0001
#define ARGS_ARE_XY_VALUES         0x0002
#define ROUND_XY_TO_GRID           0x0004
#define WE_HAVE_A_SCALE            0x0008
/* reserved                        0x0010 */
#define MORE_COMPONENTS            0x0020
#define WE_HAVE_AN_XY_SCALE        0x0040
#define WE_HAVE_A_2X2              0x0080
#define WE_HAVE_INSTR              0x0100
#define USE_MY_METRICS             0x0200
#define OVERLAP_COMPOUND           0x0400
#define SCALED_COMPONENT_OFFSET    0x0800
#define UNSCALED_COMPONENT_OFFSET  0x1000


/* Maximum recursion depth we allow for composite glyphs.
 * The TrueType spec doesn't say anything about recursion,
 * so it isn't clear that recursion is allowed at all. But
 * we'll be generous.
 */
#define TT_MAX_COMPOSITE_RECURSE 5



  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    tt_face_get_metrics                                                */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Returns the horizontal or vertical metrics in font units for a     */
  /*    given glyph.  The metrics are the left side bearing (resp. top     */
  /*    side bearing) and advance width (resp. advance height).            */
  /*                                                                       */
  /* <Input>                                                               */
  /*    header  :: A pointer to either the horizontal or vertical metrics  */
  /*               structure.                                              */
  /*                                                                       */
  /*    idx     :: The glyph index.                                        */
  /*                                                                       */
  /* <Output>                                                              */
  /*    bearing :: The bearing, either left side or top side.              */
  /*                                                                       */
  /*    advance :: The advance width resp. advance height.                 */
  /*                                                                       */
  /* <Note>                                                                */
  /*    This function will much probably move to another component in the  */
  /*    near future, but I haven't decided which yet.                      */
  /*                                                                       */
#ifdef FT_OPTIMIZE_MEMORY

  static void
  tt_face_get_metrics( TT_Face     face,
                       FT_Bool     vertical,
                       FT_UInt     idx,
                       FT_Short   *abearing,
                       FT_UShort  *aadvance )
  {
    TT_HoriHeader*  header;
    FT_Byte*        p;
    FT_Byte*        limit;
    FT_UShort       k;


    if ( vertical )
    {
      header = (TT_HoriHeader*)&face->vertical;
      p      = face->vert_metrics;
      limit  = p + face->vert_metrics_size;
    }
    else
    {
      header = &face->horizontal;
      p      = face->horz_metrics;
      limit  = p + face->horz_metrics_size;
    }
    
    k = header->number_Of_HMetrics;
    
    if ( k > 0 )
    {
      if ( idx < (FT_UInt)k )
      {
        p += 4 * idx;
        if ( p + 4 > limit )
          goto NoData;
          
        *aadvance = FT_NEXT_USHORT( p );
        *abearing = FT_NEXT_SHORT( p );
      }
      else
      {
        p += 4 * ( k - 1 );
        if ( p + 4 > limit )
          goto NoData;
          
        *aadvance = FT_NEXT_USHORT( p );
        p += 2 + 2 * ( idx - k );
        if ( p + 2 > limit )
          *abearing = 0;
        else
          *abearing = FT_PEEK_SHORT( p );
      }
    }
    else
    {
    NoData:
      *abearing = 0;
      *aadvance = 0;
    }
  }

#else /* !FT_OPTIMIZE_MEMORY */

  static void
  tt_face_get_metrics( TT_Face     face,
                       FT_Bool     vertical,
                       FT_UInt     idx,
                       FT_Short   *abearing,
                       FT_UShort  *aadvance )
  {
    TT_HoriHeader*  header = vertical ? (TT_HoriHeader*)&face->vertical
                                      :                 &face->horizontal;
    TT_LongMetrics  longs_m;
    FT_UShort       k      = header->number_Of_HMetrics;


    if ( k == 0 )
    {
      *abearing = *aadvance = 0;
      return;
    }

    if ( idx < (FT_UInt)k )
    {
      longs_m   = (TT_LongMetrics)header->long_metrics + idx;
      *abearing = longs_m->bearing;
      *aadvance = longs_m->advance;
    }
    else
    {
      *abearing = ((TT_ShortMetrics*)header->short_metrics)[idx - k];
      *aadvance = ((TT_LongMetrics)header->long_metrics)[k - 1].advance;
    }
  }

#endif /* !FT_OPTIMIZE_MEMORY */


  /*************************************************************************/
  /*                                                                       */
  /* Returns the horizontal metrics in font units for a given glyph.  If   */
  /* `check' is true, take care of monospaced fonts by returning the       */
  /* advance width maximum.                                                */
  /*                                                                       */
  static void
  Get_HMetrics( TT_Face     face,
                FT_UInt     idx,
                FT_Bool     check,
                FT_Short*   lsb,
                FT_UShort*  aw )
  {
    tt_face_get_metrics( face, 0, idx, lsb, aw );

    if ( check && face->postscript.isFixedPitch )
      *aw = face->horizontal.advance_Width_Max;
  }


  /*************************************************************************/
  /*                                                                       */
  /* Returns the advance width table for a given pixel size if it is found */
  /* in the font's `hdmx' table (if any).                                  */
  /*                                                                       */
  static FT_Byte*
  Get_Advance_WidthPtr( TT_Face  face,
                        FT_Int   ppem,
                        FT_UInt  gindex )
  {
#ifdef FT_OPTIMIZE_MEMORY

    FT_UInt   nn;
    FT_Byte*  result      = NULL;
    FT_ULong  record_size = face->hdmx_record_size;
    FT_Byte*  record      = face->hdmx_table + 8;
    

    for ( nn = 0; nn < face->hdmx_record_count; nn++ )
      if ( face->hdmx_record_sizes[nn] == ppem )
      {
        gindex += 2;
        if ( gindex < record_size )
          result = record + gindex;
        break;
      }

    return result;

#else

    FT_UShort  n;


    for ( n = 0; n < face->hdmx.num_records; n++ )
      if ( face->hdmx.records[n].ppem == ppem )
        return &face->hdmx.records[n].widths[gindex];

    return NULL;

#endif
  }


  /*************************************************************************/
  /*                                                                       */
  /* Returns the vertical metrics in font units for a given glyph.         */
  /* Greg Hitchcock from Microsoft told us that if there were no `vmtx'    */
  /* table, typoAscender/Descender from the `OS/2' table would be used     */
  /* instead, and if there were no `OS/2' table, use ascender/descender    */
  /* from the `hhea' table.  But that is not what Microsoft's rasterizer   */
  /* apparently does: It uses the ppem value as the advance height, and    */
  /* sets the top side bearing to be zero.                                 */
  /*                                                                       */
  /* The monospace `check' is probably not meaningful here, but we leave   */
  /* it in for a consistent interface.                                     */
  /*                                                                       */
  static void
  Get_VMetrics( TT_Face     face,
                FT_UInt     idx,
                FT_Bool     check,
                FT_Short*   tsb,
                FT_UShort*  ah )
  {
    FT_UNUSED( check );

    if ( face->vertical_info )
      tt_face_get_metrics( face, 1, idx, tsb, ah );

#if 1             /* Emperically determined, at variance with what MS said */

    else
    {
      *tsb = 0;
      *ah  = face->root.units_per_EM;
    }

#else      /* This is what MS said to do.  It isn't what they do, however. */

    else if ( face->os2.version != 0xFFFFU )
    {
      *tsb = face->os2.sTypoAscender;
      *ah  = face->os2.sTypoAscender - face->os2.sTypoDescender;
    }
    else
    {
      *tsb = face->horizontal.Ascender;
      *ah  = face->horizontal.Ascender - face->horizontal.Descender;
    }

#endif

  }


#define cur_to_org( n, zone ) \
          FT_ARRAY_COPY( (zone)->org, (zone)->cur, (n) )

#define org_to_cur( n, zone ) \
          FT_ARRAY_COPY( (zone)->cur, (zone)->org, (n) )


  /*************************************************************************/
  /*                                                                       */
  /* Translates an array of coordinates.                                   */
  /*                                                                       */
  static void
  translate_array( FT_UInt     n,
                   FT_Vector*  coords,
                   FT_Pos      delta_x,
                   FT_Pos      delta_y )
  {
    FT_UInt  k;


    if ( delta_x )
      for ( k = 0; k < n; k++ )
        coords[k].x += delta_x;

    if ( delta_y )
      for ( k = 0; k < n; k++ )
        coords[k].y += delta_y;
  }


  static void
  tt_prepare_zone( TT_GlyphZone  zone,
                   FT_GlyphLoad  load,
                   FT_UInt       start_point,
                   FT_UInt       start_contour )
  {
    zone->n_points   = (FT_UShort)( load->outline.n_points - start_point );
    zone->n_contours = (FT_Short) ( load->outline.n_contours - start_contour );
    zone->org        = load->extra_points + start_point;
    zone->cur        = load->outline.points + start_point;
    zone->tags       = (FT_Byte*)load->outline.tags + start_point;
    zone->contours   = (FT_UShort*)load->outline.contours + start_contour;
  }


#undef  IS_HINTED
#define IS_HINTED( flags )  ( ( flags & FT_LOAD_NO_HINTING ) == 0 )


  /*************************************************************************/
  /*                                                                       */
  /* The following functions are used by default with TrueType fonts.      */
  /* However, they can be replaced by alternatives if we need to support   */
  /* TrueType-compressed formats (like MicroType) in the future.           */
  /*                                                                       */
  /*************************************************************************/

  FT_CALLBACK_DEF( FT_Error )
  TT_Access_Glyph_Frame( TT_Loader  loader,
                         FT_UInt    glyph_index,
                         FT_ULong   offset,
                         FT_UInt    byte_count )
  {
    FT_Error   error;
    FT_Stream  stream = loader->stream;

    /* for non-debug mode */
    FT_UNUSED( glyph_index );


    FT_TRACE5(( "Glyph %ld\n", glyph_index ));

    /* the following line sets the `error' variable through macros! */
    if ( FT_STREAM_SEEK( offset ) || FT_FRAME_ENTER( byte_count ) )
      return error;

    return TT_Err_Ok;
  }


  FT_CALLBACK_DEF( void )
  TT_Forget_Glyph_Frame( TT_Loader  loader )
  {
    FT_Stream  stream = loader->stream;


    FT_FRAME_EXIT();
  }


  FT_CALLBACK_DEF( FT_Error )
  TT_Load_Glyph_Header( TT_Loader  loader )
  {
    FT_Stream  stream   = loader->stream;
    FT_Int     byte_len = loader->byte_len - 10;


    if ( byte_len < 0 )
      return TT_Err_Invalid_Outline;

    loader->n_contours = FT_GET_SHORT();

    loader->bbox.xMin = FT_GET_SHORT();
    loader->bbox.yMin = FT_GET_SHORT();
    loader->bbox.xMax = FT_GET_SHORT();
    loader->bbox.yMax = FT_GET_SHORT();

    FT_TRACE5(( "  # of contours: %d\n", loader->n_contours ));
    FT_TRACE5(( "  xMin: %4d  xMax: %4d\n", loader->bbox.xMin,
                                            loader->bbox.xMax ));
    FT_TRACE5(( "  yMin: %4d  yMax: %4d\n", loader->bbox.yMin,
                                            loader->bbox.yMax ));
    loader->byte_len = byte_len;

    return TT_Err_Ok;
  }


  FT_CALLBACK_DEF( FT_Error )
  TT_Load_Simple_Glyph( TT_Loader  load )
  {
    FT_Error        error;
    FT_Stream       stream     = load->stream;
    FT_GlyphLoader  gloader    = load->gloader;
    FT_Int          n_contours = load->n_contours;
    FT_Outline*     outline;
    TT_Face         face       = (TT_Face)load->face;
    TT_GlyphSlot    slot       = (TT_GlyphSlot)load->glyph;
    FT_UShort       n_ins;
    FT_Int          n, n_points;
    FT_Int          byte_len   = load->byte_len;

    FT_Byte         *flag, *flag_limit;
    FT_Byte         c, count;
    FT_Vector       *vec, *vec_limit;
    FT_Pos          x;
    FT_Short        *cont, *cont_limit;


    /* reading the contours' endpoints & number of points */
    cont       = gloader->current.outline.contours;
    cont_limit = cont + n_contours;

    /* check space for contours array + instructions count */
    byte_len -= 2 * ( n_contours + 1 );
    if ( byte_len < 0 )
      goto Invalid_Outline;

    for ( ; cont < cont_limit; cont++ )
      cont[0] = FT_GET_USHORT();

    n_points = 0;
    if ( n_contours > 0 )
      n_points = cont[-1] + 1;

    error = FT_GlyphLoader_CheckPoints( gloader, n_points + 4, 0 );
    if ( error )
      goto Fail;

    /* we'd better check the contours table right now */
    outline = &gloader->current.outline;

    for ( cont = outline->contours + 1; cont < cont_limit; cont++ )
      if ( cont[-1] >= cont[0] )
        goto Invalid_Outline;

    /* reading the bytecode instructions */
    slot->control_len  = 0;
    slot->control_data = 0;

    n_ins = FT_GET_USHORT();

    FT_TRACE5(( "  Instructions size: %u\n", n_ins ));

    if ( n_ins > face->max_profile.maxSizeOfInstructions )
    {
      FT_TRACE0(( "TT_Load_Simple_Glyph: Too many instructions!\n" ));
      error = TT_Err_Too_Many_Hints;
      goto Fail;
    }

    byte_len -= (FT_Int)n_ins;
    if ( byte_len < 0 )
    {
      FT_TRACE0(( "TT_Load_Simple_Glyph: Instruction count mismatch!\n" ));
      error = TT_Err_Too_Many_Hints;
      goto Fail;
    }

#ifdef TT_CONFIG_OPTION_BYTECODE_INTERPRETER

    if ( ( load->load_flags                        &
         ( FT_LOAD_NO_SCALE | FT_LOAD_NO_HINTING ) ) == 0 &&
           load->instructions )
    {
      slot->control_len  = n_ins;
      slot->control_data = load->instructions;

      FT_MEM_COPY( load->instructions, stream->cursor, (FT_Long)n_ins );
    }

#endif /* TT_CONFIG_OPTION_BYTECODE_INTERPRETER */

    stream->cursor += (FT_Int)n_ins;

    /* reading the point tags */
    flag       = (FT_Byte*)outline->tags;
    flag_limit = flag + n_points;

    FT_ASSERT( flag != NULL );

    while ( flag < flag_limit )
    {
      if ( --byte_len < 0 )
        goto Invalid_Outline;

      *flag++ = c = FT_GET_BYTE();
      if ( c & 8 )
      {
        if ( --byte_len < 0 )
          goto Invalid_Outline;

        count = FT_GET_BYTE();
        if ( flag + (FT_Int)count > flag_limit )
          goto Invalid_Outline;

        for ( ; count > 0; count-- )
          *flag++ = c;
      }
    }

    /* check that there is enough room to load the coordinates */
    for ( flag = (FT_Byte*)outline->tags; flag < flag_limit; flag++ )
    {
      if ( *flag & 2 )
        byte_len -= 1;
      else if ( ( *flag & 16 ) == 0 )
        byte_len -= 2;

      if ( *flag & 4 )
        byte_len -= 1;
      else if ( ( *flag & 32 ) == 0 )
        byte_len -= 2;
    }

    if ( byte_len < 0 )
      goto Invalid_Outline;

    /* reading the X coordinates */

    vec       = outline->points;
    vec_limit = vec + n_points;
    flag      = (FT_Byte*)outline->tags;
    x         = 0;

    for ( ; vec < vec_limit; vec++, flag++ )
    {
      FT_Pos  y = 0;


      if ( *flag & 2 )
      {
        y = (FT_Pos)FT_GET_BYTE();
        if ( ( *flag & 16 ) == 0 )
          y = -y;
      }
      else if ( ( *flag & 16 ) == 0 )
        y = (FT_Pos)FT_GET_SHORT();

      x     += y;
      vec->x = x;
    }

    /* reading the Y coordinates */

    vec       = gloader->current.outline.points;
    vec_limit = vec + n_points;
    flag      = (FT_Byte*)outline->tags;
    x         = 0;

    for ( ; vec < vec_limit; vec++, flag++ )
    {
      FT_Pos  y = 0;


      if ( *flag & 4 )
      {
        y = (FT_Pos)FT_GET_BYTE();
        if ( ( *flag & 32 ) == 0 )
          y = -y;
      }
      else if ( ( *flag & 32 ) == 0 )
        y = (FT_Pos)FT_GET_SHORT();

      x     += y;
      vec->y = x;
    }

    /* clear the touch tags */
    for ( n = 0; n < n_points; n++ )
      outline->tags[n] &= FT_CURVE_TAG_ON;

    outline->n_points   = (FT_UShort)n_points;
    outline->n_contours = (FT_Short) n_contours;

    load->byte_len = byte_len;

  Fail:
    return error;

  Invalid_Outline:
    error = TT_Err_Invalid_Outline;
    goto Fail;
  }


  FT_CALLBACK_DEF( FT_Error )
  TT_Load_Composite_Glyph( TT_Loader  loader )
  {
    FT_Error        error;
    FT_Stream       stream  = loader->stream;
    FT_GlyphLoader  gloader = loader->gloader;
    FT_SubGlyph     subglyph;
    FT_UInt         num_subglyphs;
    FT_Int          byte_len = loader->byte_len;


    num_subglyphs = 0;

    do
    {
      FT_Fixed  xx, xy, yy, yx;


      /* check that we can load a new subglyph */
      error = FT_GlyphLoader_CheckSubGlyphs( gloader, num_subglyphs + 1 );
      if ( error )
        goto Fail;

      /* check space */
      byte_len -= 4;
      if ( byte_len < 0 )
        goto Invalid_Composite;

      subglyph = gloader->current.subglyphs + num_subglyphs;

      subglyph->arg1 = subglyph->arg2 = 0;

      subglyph->flags = FT_GET_USHORT();
      subglyph->index = FT_GET_USHORT();

      /* check space */
      byte_len -= 2;
      if ( subglyph->flags & ARGS_ARE_WORDS )
        byte_len -= 2;
      if ( subglyph->flags & WE_HAVE_A_SCALE )
        byte_len -= 2;
      else if ( subglyph->flags & WE_HAVE_AN_XY_SCALE )
        byte_len -= 4;
      else if ( subglyph->flags & WE_HAVE_A_2X2 )
        byte_len -= 8;

      if ( byte_len < 0 )
        goto Invalid_Composite;

      /* read arguments */
      if ( subglyph->flags & ARGS_ARE_WORDS )
      {
        subglyph->arg1 = FT_GET_SHORT();
        subglyph->arg2 = FT_GET_SHORT();
      }
      else
      {
        subglyph->arg1 = FT_GET_CHAR();
        subglyph->arg2 = FT_GET_CHAR();
      }

      /* read transform */
      xx = yy = 0x10000L;
      xy = yx = 0;

      if ( subglyph->flags & WE_HAVE_A_SCALE )
      {
        xx = (FT_Fixed)FT_GET_SHORT() << 2;
        yy = xx;
      }
      else if ( subglyph->flags & WE_HAVE_AN_XY_SCALE )
      {
        xx = (FT_Fixed)FT_GET_SHORT() << 2;
        yy = (FT_Fixed)FT_GET_SHORT() << 2;
      }
      else if ( subglyph->flags & WE_HAVE_A_2X2 )
      {
        xx = (FT_Fixed)FT_GET_SHORT() << 2;
        yx = (FT_Fixed)FT_GET_SHORT() << 2;
        xy = (FT_Fixed)FT_GET_SHORT() << 2;
        yy = (FT_Fixed)FT_GET_SHORT() << 2;
      }

      subglyph->transform.xx = xx;
      subglyph->transform.xy = xy;
      subglyph->transform.yx = yx;
      subglyph->transform.yy = yy;

      num_subglyphs++;

    } while ( subglyph->flags & MORE_COMPONENTS );

    gloader->current.num_subglyphs = num_subglyphs;

#ifdef TT_CONFIG_OPTION_BYTECODE_INTERPRETER
    {
      /* we must undo the FT_FRAME_ENTER in order to point to the */
      /* composite instructions, if we find some.               */
      /* we will process them later...                          */
      /*                                                        */
      loader->ins_pos = (FT_ULong)( FT_STREAM_POS() +
                                    stream->cursor - stream->limit );
    }
#endif

    loader->byte_len = byte_len;

  Fail:
    return error;

  Invalid_Composite:
    error = TT_Err_Invalid_Composite;
    goto Fail;
  }


  FT_LOCAL_DEF( void )
  TT_Init_Glyph_Loading( TT_Face  face )
  {
    face->access_glyph_frame   = TT_Access_Glyph_Frame;
    face->read_glyph_header    = TT_Load_Glyph_Header;
    face->read_simple_glyph    = TT_Load_Simple_Glyph;
    face->read_composite_glyph = TT_Load_Composite_Glyph;
    face->forget_glyph_frame   = TT_Forget_Glyph_Frame;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    TT_Process_Simple_Glyph                                            */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Once a simple glyph has been loaded, it needs to be processed.     */
  /*    Usually, this means scaling and hinting through bytecode           */
  /*    interpretation.                                                    */
  /*                                                                       */
  static FT_Error
  TT_Process_Simple_Glyph( TT_Loader  load,
                           FT_Bool    debug )
  {
    FT_GlyphLoader  gloader  = load->gloader;
    FT_Outline*     outline  = &gloader->current.outline;
    FT_UInt         n_points = outline->n_points;
#ifdef TT_CONFIG_OPTION_BYTECODE_INTERPRETER
    FT_UInt         n_ins;
#endif
    TT_GlyphZone    zone     = &load->zone;
    FT_Error        error    = TT_Err_Ok;

    FT_UNUSED( debug );  /* used by truetype interpreter only */


#ifdef TT_CONFIG_OPTION_BYTECODE_INTERPRETER
    n_ins = load->glyph->control_len;
#endif

    /* add shadow points */

    /* Add two horizontal shadow points at n and n+1.   */
    /* We need the left side bearing and advance width. */
    /* Add two vertical shadow points at n+2 and n+3.   */
    /* We need the top side bearing and advance height. */

    {
      FT_Vector*  pp1;
      FT_Vector*  pp2;
      FT_Vector*  pp3;
      FT_Vector*  pp4;


      /* pp1 = xMin - lsb */
      pp1    = outline->points + n_points;
      pp1->x = load->bbox.xMin - load->left_bearing;
      pp1->y = 0;

      /* pp2 = pp1 + aw */
      pp2    = pp1 + 1;
      pp2->x = pp1->x + load->advance;
      pp2->y = 0;

      /* pp3 = top side bearing */
      pp3    = pp1 + 2;
      pp3->x = 0;
      pp3->y = load->top_bearing + load->bbox.yMax;

      /* pp4 = pp3 - ah */
      pp4    = pp1 + 3;
      pp4->x = 0;
      pp4->y = pp3->y - load->vadvance;

      outline->tags[n_points    ] = 0;
      outline->tags[n_points + 1] = 0;
      outline->tags[n_points + 2] = 0;
      outline->tags[n_points + 3] = 0;
    }

    /* Note that we return four more points that are not */
    /* part of the glyph outline.                        */

    n_points += 4;

#ifdef TT_CONFIG_OPTION_GX_VAR_SUPPORT

    if ( ((TT_Face)load->face)->doblend )
    {
      /* Deltas apply to the unscaled data. */
      FT_Vector*    deltas;
      FT_Memory     memory       = load->face->memory;
      FT_StreamRec  saved_stream = *(load->stream);
      FT_UInt       i;


      /* TT_Vary_Get_Glyph_Deltas uses a frame, thus we have to save */
      /* (and restore) the current one                               */
      load->stream->cursor = 0;
      load->stream->limit  = 0;

      error = TT_Vary_Get_Glyph_Deltas( (TT_Face)(load->face),
                                        load->glyph_index,
                                        &deltas,
                                        n_points );

      *(load->stream) = saved_stream;

      if ( error )
        goto Exit;

      for ( i = 0; i < n_points; ++i )
      {
        outline->points[i].x += deltas[i].x;
        outline->points[i].y += deltas[i].y;
      }

      FT_FREE( deltas );
    }

#endif /* TT_CONFIG_OPTION_GX_VAR_SUPPORT */

    /* set up zone for hinting */
    tt_prepare_zone( zone, &gloader->current, 0, 0 );

    /* eventually scale the glyph */
    if ( !( load->load_flags & FT_LOAD_NO_SCALE ) )
    {
      FT_Vector*  vec     = zone->cur;
      FT_Vector*  limit   = vec + n_points;
      FT_Fixed    x_scale = ((TT_Size)load->size)->metrics.x_scale;
      FT_Fixed    y_scale = ((TT_Size)load->size)->metrics.y_scale;

      /* first scale the glyph points */
      for ( ; vec < limit; vec++ )
      {
        vec->x = FT_MulFix( vec->x, x_scale );
        vec->y = FT_MulFix( vec->y, y_scale );
      }
    }

    cur_to_org( n_points, zone );

    /* eventually hint the glyph */
    if ( IS_HINTED( load->load_flags ) )
    {
      FT_Pos  x = zone->org[n_points-4].x;


      x = FT_PIX_ROUND( x ) - x;
      translate_array( n_points, zone->org, x, 0 );

      org_to_cur( n_points, zone );

      zone->cur[n_points - 3].x = FT_PIX_ROUND( zone->cur[n_points - 3].x );
      zone->cur[n_points - 1].y = FT_PIX_ROUND( zone->cur[n_points - 1].y );

#ifdef TT_CONFIG_OPTION_BYTECODE_INTERPRETER

      /* now consider hinting */
      if ( n_ins > 0 )
      {
        error = TT_Set_CodeRange( load->exec, tt_coderange_glyph,
                                  load->exec->glyphIns, n_ins );
        if ( error )
          goto Exit;

        load->exec->is_composite     = FALSE;
        load->exec->pts              = *zone;
        load->exec->pts.n_points    += 4;

        error = TT_Run_Context( load->exec, debug );
        if ( error && load->exec->pedantic_hinting )
          goto Exit;

        error = TT_Err_Ok;  /* ignore bytecode errors in non-pedantic mode */
      }

#endif /* TT_CONFIG_OPTION_BYTECODE_INTERPRETER */

    }

    /* save glyph phantom points */
    if ( !load->preserve_pps )
    {
      load->pp1 = zone->cur[n_points - 4];
      load->pp2 = zone->cur[n_points - 3];
      load->pp3 = zone->cur[n_points - 2];
      load->pp4 = zone->cur[n_points - 1];
    }

#if defined( TT_CONFIG_OPTION_BYTECODE_INTERPRETER ) || \
    defined( TT_CONFIG_OPTION_GX_VAR_SUPPORT )
  Exit:
#endif

    return error;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    load_truetype_glyph                                                */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Loads a given truetype glyph.  Handles composites and uses a       */
  /*    TT_Loader object.                                                  */
  /*                                                                       */
  static FT_Error
  load_truetype_glyph( TT_Loader  loader,
                       FT_UInt    glyph_index,
                       FT_UInt    recurse_count )
  {

#ifdef TT_CONFIG_OPTION_BYTECODE_INTERPRETER
    FT_Stream       stream = loader->stream;
#endif

    FT_Error        error;
    TT_Face         face = (TT_Face)loader->face;
    FT_ULong        offset;
    FT_Int          contours_count;
    FT_UInt         num_points, count;
    FT_Fixed        x_scale, y_scale;
    FT_GlyphLoader  gloader = loader->gloader;
    FT_Bool         opened_frame = 0;

#ifdef FT_CONFIG_OPTION_INCREMENTAL
    FT_StreamRec    inc_stream;
    FT_Data         glyph_data;
    FT_Bool         glyph_data_loaded = 0;
#endif

#ifdef TT_CONFIG_OPTION_GX_VAR_SUPPORT
    FT_Vector      *deltas;
#endif


    if ( recurse_count >= TT_MAX_COMPOSITE_RECURSE )
    {
      error = TT_Err_Invalid_Composite;
      goto Exit;
    }

    /* check glyph index */
    if ( glyph_index >= (FT_UInt)face->root.num_glyphs )
    {
      error = TT_Err_Invalid_Glyph_Index;
      goto Exit;
    }

    loader->glyph_index = glyph_index;
    num_points          = 0;

    x_scale = 0x10000L;
    y_scale = 0x10000L;
    if ( ( loader->load_flags & FT_LOAD_NO_SCALE ) == 0 )
    {
      x_scale = ((TT_Size)loader->size)->metrics.x_scale;
      y_scale = ((TT_Size)loader->size)->metrics.y_scale;
    }

    /* get metrics, horizontal and vertical */
    {
      FT_Short   left_bearing = 0, top_bearing = 0;
      FT_UShort  advance_width = 0, advance_height = 0;


      Get_HMetrics( face, glyph_index,
                    (FT_Bool)!( loader->load_flags &
                                FT_LOAD_IGNORE_GLOBAL_ADVANCE_WIDTH ),
                    &left_bearing,
                    &advance_width );
      Get_VMetrics( face, glyph_index,
                    (FT_Bool)!( loader->load_flags &
                                FT_LOAD_IGNORE_GLOBAL_ADVANCE_WIDTH ),
                    &top_bearing,
                    &advance_height );

#ifdef FT_CONFIG_OPTION_INCREMENTAL

      /* If this is an incrementally loaded font see if there are */
      /* overriding metrics for this glyph.                       */
      if ( face->root.internal->incremental_interface &&
           face->root.internal->incremental_interface->funcs->get_glyph_metrics )
      {
        FT_Incremental_MetricsRec  metrics;


        metrics.bearing_x = left_bearing;
        metrics.bearing_y = 0;
        metrics.advance = advance_width;
        error = face->root.internal->incremental_interface->funcs->get_glyph_metrics(
                  face->root.internal->incremental_interface->object,
                  glyph_index, FALSE, &metrics );
        if ( error )
          goto Exit;
        left_bearing  = (FT_Short)metrics.bearing_x;
        advance_width = (FT_UShort)metrics.advance;
      }

# if 0
      /* GWW: Do I do the same for vertical metrics ??? */
      if ( face->root.internal->incremental_interface &&
           face->root.internal->incremental_interface->funcs->get_glyph_metrics )
      {
        FT_Incremental_MetricsRec  metrics;


        metrics.bearing_x = 0;
        metrics.bearing_y = top_bearing;
        metrics.advance = advance_height;
        error = face->root.internal->incremental_interface->funcs->get_glyph_metrics(
                  face->root.internal->incremental_interface->object,
                  glyph_index, TRUE, &metrics );
        if ( error )
          goto Exit;
        top_bearing  = (FT_Short)metrics.bearing_y;
        advance_height = (FT_UShort)metrics.advance;
      }
# endif

#endif /* FT_CONFIG_OPTION_INCREMENTAL */

      loader->left_bearing = left_bearing;
      loader->advance      = advance_width;
      loader->top_bearing  = top_bearing;
      loader->vadvance     = advance_height;

      if ( !loader->linear_def )
      {
        loader->linear_def = 1;
        loader->linear     = advance_width;
      }
    }

#ifdef FT_CONFIG_OPTION_INCREMENTAL

    /* Set `offset' to the start of the glyph program relative to the  */
    /* start of the 'glyf' table, and `count' to the length of the     */
    /* glyph program in bytes.                                         */
    /*                                                                 */
    /* If we are loading glyph data via the incremental interface, set */
    /* the loader stream to a memory stream reading the data returned  */
    /* by the interface.                                               */

    if ( face->root.internal->incremental_interface )
    {
      error = face->root.internal->incremental_interface->funcs->get_glyph_data(
                face->root.internal->incremental_interface->object,
                glyph_index, &glyph_data );
      if ( error )
        goto Exit;

      glyph_data_loaded = 1;
      offset            = 0;
      count             = glyph_data.length;

      FT_MEM_ZERO( &inc_stream, sizeof ( inc_stream ) );
      FT_Stream_OpenMemory( &inc_stream,
                            glyph_data.pointer, glyph_data.length );

      loader->stream = &inc_stream;
    }
    else

#endif /* FT_CONFIG_OPTION_INCREMENTAL */

    offset = tt_face_get_location( face, glyph_index, &count );

    if ( count == 0 )
    {
      /* as described by Frederic Loyer, these are spaces, and */
      /* not the unknown glyph.                                */
      loader->bbox.xMin = 0;
      loader->bbox.xMax = 0;
      loader->bbox.yMin = 0;
      loader->bbox.yMax = 0;

      loader->pp1.x = 0;
      loader->pp2.x = loader->advance;
      loader->pp3.y = 0;
      loader->pp4.y = loader->pp3.y-loader->vadvance;

#ifdef TT_CONFIG_OPTION_GX_VAR_SUPPORT
      if ( ((TT_Face)(loader->face))->doblend )
      {
        /* this must be done before scaling */
        FT_Memory  memory = loader->face->memory;


        error = TT_Vary_Get_Glyph_Deltas( (TT_Face)(loader->face),
                                          glyph_index, &deltas, 4 );
        if ( error )
          goto Exit;

        loader->pp1.x += deltas[0].x; loader->pp1.y += deltas[0].y;
        loader->pp2.x += deltas[1].x; loader->pp2.y += deltas[1].y;
        loader->pp3.x += deltas[2].x; loader->pp3.y += deltas[2].y;
        loader->pp4.x += deltas[3].x; loader->pp4.y += deltas[3].y;

        FT_FREE( deltas );
      }
#endif

      if ( ( loader->load_flags & FT_LOAD_NO_SCALE ) == 0 )
      {
        loader->pp2.x = FT_MulFix( loader->pp2.x, x_scale );
        loader->pp4.y = FT_MulFix( loader->pp4.y, y_scale );
      }

#ifdef TT_CONFIG_OPTION_BYTECODE_INTERPRETER

      if ( loader->exec )
        loader->exec->glyphSize = 0;

#endif

      error = TT_Err_Ok;
      goto Exit;
    }

    loader->byte_len = (FT_Int)count;

    offset = loader->glyf_offset + offset;

    /* access glyph frame */
    error = face->access_glyph_frame( loader, glyph_index, offset, count );
    if ( error )
      goto Exit;

    opened_frame = 1;

    /* read first glyph header */
    error = face->read_glyph_header( loader );
    if ( error )
      goto Fail;

    contours_count = loader->n_contours;

    count -= 10;

    loader->pp1.x = loader->bbox.xMin - loader->left_bearing;
    loader->pp1.y = 0;
    loader->pp2.x = loader->pp1.x + loader->advance;
    loader->pp2.y = 0;

    loader->pp3.x = 0;
    loader->pp3.y = loader->top_bearing + loader->bbox.yMax;
    loader->pp4.x = 0;
    loader->pp4.y = loader->pp3.y - loader->vadvance;

    /***********************************************************************/
    /***********************************************************************/
    /***********************************************************************/

    /* if it is a simple glyph, load it */

    if ( contours_count >= 0 )
    {
      /* check that we can add the contours to the glyph */
      error = FT_GlyphLoader_CheckPoints( gloader, 0, contours_count );
      if ( error )
        goto Fail;

      error = face->read_simple_glyph( loader );
      if ( error )
        goto Fail;

#ifdef TT_CONFIG_OPTION_BYTECODE_INTERPRETER

      {
        TT_Size  size = (TT_Size)loader->size;


        error = TT_Process_Simple_Glyph( loader,
                                         (FT_Bool)( size && size->debug ) );
      }

#else

      error = TT_Process_Simple_Glyph( loader, 0 );

#endif

      if ( error )
        goto Fail;

      FT_GlyphLoader_Add( gloader );

      /* Note: We could have put the simple loader source there */
      /*       but the code is fat enough already :-)           */
    }

    /***********************************************************************/
    /***********************************************************************/
    /***********************************************************************/

    /* otherwise, load a composite! */
    else if ( contours_count == -1 )
    {
      TT_GlyphSlot  glyph = (TT_GlyphSlot)loader->glyph;
      FT_UInt       start_point;
#ifdef TT_CONFIG_OPTION_BYTECODE_INTERPRETER
      FT_UInt       start_contour;
      FT_ULong      ins_pos;  /* position of composite instructions, if any */
#endif


      /* for each subglyph, read composite header */
      start_point   = gloader->base.outline.n_points;
#ifdef TT_CONFIG_OPTION_BYTECODE_INTERPRETER
      start_contour = gloader->base.outline.n_contours;
#endif

      error = face->read_composite_glyph( loader );
      if ( error )
        goto Fail;

#ifdef TT_CONFIG_OPTION_BYTECODE_INTERPRETER
      ins_pos = loader->ins_pos;
#endif
      face->forget_glyph_frame( loader );
      opened_frame = 0;

#ifdef TT_CONFIG_OPTION_GX_VAR_SUPPORT

      if ( face->doblend )
      {
        FT_Int       i, limit;
        FT_SubGlyph  subglyph;
        FT_Memory    memory = face->root.memory;


        /* this provides additional offsets */
        /* for each component's translation */

        if ( (error = TT_Vary_Get_Glyph_Deltas(
                        face,
                        glyph_index,
                        &deltas,
                        gloader->current.num_subglyphs + 4 )) != 0 )
          goto Exit;

        /* Note: No subglyph reallocation here, our pointers are stable. */
        subglyph = gloader->current.subglyphs + gloader->base.num_subglyphs;
        limit    = gloader->current.num_subglyphs;

        for ( i = 0; i < limit; ++i, ++subglyph )
        {
          if ( subglyph->flags & ARGS_ARE_XY_VALUES )
          {
            subglyph->arg1 += deltas[i].x;
            subglyph->arg2 += deltas[i].y;
          }
        }

        loader->pp1.x += deltas[i + 0].x; loader->pp1.y += deltas[i + 0].y;
        loader->pp2.x += deltas[i + 1].x; loader->pp2.y += deltas[i + 1].y;
        loader->pp3.x += deltas[i + 2].x; loader->pp3.y += deltas[i + 2].y;
        loader->pp4.x += deltas[i + 3].x; loader->pp4.y += deltas[i + 3].y;

        FT_FREE( deltas );
      }

#endif /* TT_CONFIG_OPTION_GX_VAR_SUPPORT */

      if ( ( loader->load_flags & FT_LOAD_NO_SCALE ) == 0 )
      {
        loader->pp1.x = FT_MulFix( loader->pp1.x, x_scale );
        loader->pp2.x = FT_MulFix( loader->pp2.x, x_scale );
        loader->pp3.y = FT_MulFix( loader->pp3.y, y_scale );
        loader->pp4.y = FT_MulFix( loader->pp4.y, y_scale );
      }

      /* if the flag FT_LOAD_NO_RECURSE is set, we return the subglyph */
      /* `as is' in the glyph slot (the client application will be     */
      /* responsible for interpreting these data)...                   */
      /*                                                               */
      if ( loader->load_flags & FT_LOAD_NO_RECURSE )
      {
        /* set up remaining glyph fields */
        FT_GlyphLoader_Add( gloader );

        glyph->num_subglyphs = gloader->base.num_subglyphs;
        glyph->format        = FT_GLYPH_FORMAT_COMPOSITE;
        glyph->subglyphs     = gloader->base.subglyphs;

        goto Exit;
      }

      /*********************************************************************/
      /*********************************************************************/
      /*********************************************************************/

      /* Now, read each subglyph independently. */
      {
        FT_Int       n, num_base_points, num_new_points;
        FT_SubGlyph  subglyph       = 0;

        FT_UInt      num_subglyphs  = gloader->current.num_subglyphs;
        FT_UInt      num_base_subgs = gloader->base.num_subglyphs;


        FT_GlyphLoader_Add( gloader );

        for ( n = 0; n < (FT_Int)num_subglyphs; n++ )
        {
          FT_Vector  pp1, pp2, pp3, pp4;
          FT_Pos     x, y;


          /* Each time we call load_truetype_glyph in this loop, the   */
          /* value of `gloader.base.subglyphs' can change due to table */
          /* reallocations.  We thus need to recompute the subglyph    */
          /* pointer on each iteration.                                */
          subglyph = gloader->base.subglyphs + num_base_subgs + n;

          pp1 = loader->pp1;
          pp2 = loader->pp2;
          pp3 = loader->pp3;
          pp4 = loader->pp4;

          num_base_points = gloader->base.outline.n_points;

          error = load_truetype_glyph( loader, subglyph->index,
                                       recurse_count + 1 );
          if ( error )
            goto Fail;

          /* restore subglyph pointer */
          subglyph = gloader->base.subglyphs + num_base_subgs + n;

          if ( subglyph->flags & USE_MY_METRICS )
          {
            pp1 = loader->pp1;
            pp2 = loader->pp2;
            pp3 = loader->pp3;
            pp4 = loader->pp4;
          }
          else
          {
            loader->pp1 = pp1;
            loader->pp2 = pp2;
            loader->pp3 = pp3;
            loader->pp4 = pp4;
          }

          num_points = gloader->base.outline.n_points;

          num_new_points = num_points - num_base_points;

          /* now perform the transform required for this subglyph */

          if ( subglyph->flags & ( WE_HAVE_A_SCALE     |
                                   WE_HAVE_AN_XY_SCALE |
                                   WE_HAVE_A_2X2       ) )
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

          if ( !( subglyph->flags & ARGS_ARE_XY_VALUES ) )
          {
            FT_UInt     k = subglyph->arg1;
            FT_UInt     l = subglyph->arg2;
            FT_Vector*  p1;
            FT_Vector*  p2;


            if ( start_point + k >= (FT_UInt)num_base_points ||
                               l >= (FT_UInt)num_new_points  )
            {
              error = TT_Err_Invalid_Composite;
              goto Fail;
            }

            l += num_base_points;

            p1 = gloader->base.outline.points + start_point + k;
            p2 = gloader->base.outline.points + start_point + l;

            x = p1->x - p2->x;
            y = p1->y - p2->y;
          }
          else
          {
            x = subglyph->arg1;
            y = subglyph->arg2;

  /* Use a default value dependent on                                     */
  /* TT_CONFIG_OPTION_COMPONENT_OFFSET_SCALED.  This is useful for old TT */
  /* fonts which don't set the xxx_COMPONENT_OFFSET bit.                  */

#ifdef TT_CONFIG_OPTION_COMPONENT_OFFSET_SCALED
            if ( !( subglyph->flags & UNSCALED_COMPONENT_OFFSET ) &&
#else
            if (  ( subglyph->flags & SCALED_COMPONENT_OFFSET ) &&
#endif
                  ( subglyph->flags & ( WE_HAVE_A_SCALE     |
                                        WE_HAVE_AN_XY_SCALE |
                                        WE_HAVE_A_2X2       )) )
            {
#if 0

  /*************************************************************************/
  /*                                                                       */
  /* This algorithm is what Apple documents.  But it doesn't work.         */
  /*                                                                       */
              int  a = subglyph->transform.xx > 0 ?  subglyph->transform.xx
                                                  : -subglyph->transform.xx;
              int  b = subglyph->transform.yx > 0 ?  subglyph->transform.yx
                                                  : -subglyph->transform.yx;
              int  c = subglyph->transform.xy > 0 ?  subglyph->transform.xy
                                                  : -subglyph->transform.xy;
              int  d = subglyph->transform.yy > 0 ? subglyph->transform.yy
                                                  : -subglyph->transform.yy;
              int  m = a > b ? a : b;
              int  n = c > d ? c : d;


              if ( a - b <= 33 && a - b >= -33 )
                m *= 2;
              if ( c - d <= 33 && c - d >= -33 )
                n *= 2;
              x = FT_MulFix( x, m );
              y = FT_MulFix( y, n );

#else /* 0 */

  /*************************************************************************/
  /*                                                                       */
  /* This algorithm is a guess and works much better than the above.       */
  /*                                                                       */
              FT_Fixed  mac_xscale = FT_SqrtFixed(
                                       FT_MulFix( subglyph->transform.xx,
                                                  subglyph->transform.xx ) +
                                       FT_MulFix( subglyph->transform.xy,
                                                  subglyph->transform.xy) );
              FT_Fixed  mac_yscale = FT_SqrtFixed(
                                       FT_MulFix( subglyph->transform.yy,
                                                  subglyph->transform.yy ) +
                                       FT_MulFix( subglyph->transform.yx,
                                                  subglyph->transform.yx ) );


              x = FT_MulFix( x, mac_xscale );
              y = FT_MulFix( y, mac_yscale );
#endif /* 0 */

            }

            if ( !( loader->load_flags & FT_LOAD_NO_SCALE ) )
            {
              x = FT_MulFix( x, x_scale );
              y = FT_MulFix( y, y_scale );

              if ( subglyph->flags & ROUND_XY_TO_GRID )
              {
                x = FT_PIX_ROUND( x );
                y = FT_PIX_ROUND( y );
              }
            }
          }

          if ( x || y )
          {
            translate_array( num_new_points,
                             gloader->base.outline.points + num_base_points,
                             x, y );

            translate_array( num_new_points,
                             gloader->base.extra_points + num_base_points,
                             x, y );
          }
        }

        /*******************************************************************/
        /*******************************************************************/
        /*******************************************************************/

        /* we have finished loading all sub-glyphs; now, look for */
        /* instructions for this composite!                       */

#ifdef TT_CONFIG_OPTION_BYTECODE_INTERPRETER

        if ( num_subglyphs > 0               &&
             loader->exec                    &&
             ins_pos > 0                     &&
             subglyph->flags & WE_HAVE_INSTR )
        {
          FT_UShort       n_ins;
          TT_ExecContext  exec = loader->exec;
          TT_GlyphZone    pts;
          FT_Vector*      pp1;


          /* read size of instructions */
          if ( FT_STREAM_SEEK( ins_pos ) ||
               FT_READ_USHORT( n_ins ) )
            goto Fail;
          FT_TRACE5(( "  Instructions size = %d\n", n_ins ));

          /* in some fonts? */
          if ( n_ins == 0xFFFFU )
            n_ins = 0;

          /* check it */
          if ( n_ins > face->max_profile.maxSizeOfInstructions )
          {
            FT_TRACE0(( "Too many instructions (%d) in composite glyph %ld\n",
                        n_ins, subglyph->index ));
            error = TT_Err_Too_Many_Hints;
            goto Fail;
          }

          /* read the instructions */
          if ( FT_STREAM_READ( exec->glyphIns, n_ins ) )
            goto Fail;

          glyph->control_data = exec->glyphIns;
          glyph->control_len  = n_ins;

          error = TT_Set_CodeRange( exec,
                                    tt_coderange_glyph,
                                    exec->glyphIns,
                                    n_ins );
          if ( error )
            goto Fail;

          error = FT_GlyphLoader_CheckPoints( gloader, num_points + 4, 0 );
          if ( error )
            goto Fail;

          /* prepare the execution context */
          tt_prepare_zone( &exec->pts, &gloader->base,
                           start_point, start_contour );
          pts = &exec->pts;

          pts->n_points   = (short)( num_points + 4 );
          pts->n_contours = gloader->base.outline.n_contours;

          /* add phantom points */
          pp1    = pts->cur + num_points;
          pp1[0] = loader->pp1;
          pp1[1] = loader->pp2;
          pp1[2] = loader->pp3;
          pp1[3] = loader->pp4;

          pts->tags[num_points    ] = 0;
          pts->tags[num_points + 1] = 0;
          pts->tags[num_points + 2] = 0;
          pts->tags[num_points + 3] = 0;

          /* if hinting, round the phantom points */
          if ( IS_HINTED( loader->load_flags ) )
          {
            pp1[0].x = FT_PIX_ROUND( loader->pp1.x );
            pp1[1].x = FT_PIX_ROUND( loader->pp2.x );
            pp1[2].y = FT_PIX_ROUND( loader->pp3.y );
            pp1[3].y = FT_PIX_ROUND( loader->pp4.y );
          }

          {
            FT_UInt  k;


            for ( k = 0; k < num_points; k++ )
              pts->tags[k] &= FT_CURVE_TAG_ON;
          }

          cur_to_org( num_points + 4, pts );

          /* now consider hinting */
          if ( IS_HINTED( loader->load_flags ) && n_ins > 0 )
          {
            exec->is_composite     = TRUE;
            error = TT_Run_Context( exec, ((TT_Size)loader->size)->debug );
            if ( error && exec->pedantic_hinting )
              goto Fail;
          }

          /* save glyph origin and advance points */
          loader->pp1 = pp1[0];
          loader->pp2 = pp1[1];
          loader->pp3 = pp1[2];
          loader->pp4 = pp1[3];
        }

#endif /* TT_CONFIG_OPTION_BYTECODE_INTERPRETER */

      }
      /* end of composite loading */
    }
    else
    {
      /* invalid composite count ( negative but not -1 ) */
      error = TT_Err_Invalid_Outline;
      goto Fail;
    }

    /***********************************************************************/
    /***********************************************************************/
    /***********************************************************************/

  Fail:
    if ( opened_frame )
      face->forget_glyph_frame( loader );

  Exit:

#ifdef FT_CONFIG_OPTION_INCREMENTAL
    if ( glyph_data_loaded )
      face->root.internal->incremental_interface->funcs->free_glyph_data(
        face->root.internal->incremental_interface->object,
        &glyph_data );
#endif

    return error;
  }


  static FT_Error
  compute_glyph_metrics( TT_Loader   loader,
                         FT_UInt     glyph_index )
  {
    FT_BBox       bbox;
    TT_Face       face = (TT_Face)loader->face;
    FT_Fixed      y_scale;
    TT_GlyphSlot  glyph = loader->glyph;
    TT_Size       size = (TT_Size)loader->size;


    y_scale = 0x10000L;
    if ( ( loader->load_flags & FT_LOAD_NO_SCALE ) == 0 )
      y_scale = size->root.metrics.y_scale;

    if ( glyph->format != FT_GLYPH_FORMAT_COMPOSITE )
    {
      glyph->outline.flags &= ~FT_OUTLINE_SINGLE_PASS;

      /* copy outline to our glyph slot */
      FT_GlyphLoader_CopyPoints( glyph->internal->loader, loader->gloader );
      glyph->outline = glyph->internal->loader->base.outline;

      /* translate array so that (0,0) is the glyph's origin */
      FT_Outline_Translate( &glyph->outline, -loader->pp1.x, 0 );

      FT_Outline_Get_CBox( &glyph->outline, &bbox );

      if ( IS_HINTED( loader->load_flags ) )
      {
        /* grid-fit the bounding box */
        bbox.xMin = FT_PIX_FLOOR( bbox.xMin );
        bbox.yMin = FT_PIX_FLOOR( bbox.yMin );
        bbox.xMax = FT_PIX_CEIL( bbox.xMax );
        bbox.yMax = FT_PIX_CEIL( bbox.yMax );
      }
    }
    else
      bbox = loader->bbox;

    /* get the device-independent horizontal advance.  It is scaled later */
    /* by the base layer.                                                 */
    {
      FT_Pos  advance = loader->linear;


      /* the flag FT_LOAD_IGNORE_GLOBAL_ADVANCE_WIDTH was introduced to */
      /* correctly support DynaLab fonts, which have an incorrect       */
      /* `advance_Width_Max' field!  It is used, to my knowledge,       */
      /* exclusively in the X-TrueType font server.                     */
      /*                                                                */
      if ( face->postscript.isFixedPitch                                     &&
           ( loader->load_flags & FT_LOAD_IGNORE_GLOBAL_ADVANCE_WIDTH ) == 0 )
        advance = face->horizontal.advance_Width_Max;

      /* we need to return the advance in font units in linearHoriAdvance, */
      /* it will be scaled later by the base layer.                        */
      glyph->linearHoriAdvance = advance;
    }

    glyph->metrics.horiBearingX = bbox.xMin;
    glyph->metrics.horiBearingY = bbox.yMax;
    glyph->metrics.horiAdvance  = loader->pp2.x - loader->pp1.x;

    /* don't forget to hint the advance when we need to */
    if ( IS_HINTED( loader->load_flags ) )
      glyph->metrics.horiAdvance = FT_PIX_ROUND( glyph->metrics.horiAdvance );

    /* Now take care of vertical metrics.  In the case where there is    */
    /* no vertical information within the font (relatively common), make */
    /* up some metrics by `hand'...                                      */

    {
      FT_Short   top_bearing;    /* vertical top side bearing (EM units) */
      FT_UShort  advance_height; /* vertical advance height   (EM units) */

      FT_Pos     left;     /* scaled vertical left side bearing */
      FT_Pos     top;      /* scaled vertical top side bearing  */
      FT_Pos     advance;  /* scaled vertical advance height    */


      /* Get the unscaled top bearing and advance height. */
      if ( face->vertical_info &&
           face->vertical.number_Of_VMetrics > 0 )
      {
        advance_height = (FT_UShort)( loader->pp4.y - loader->pp3.y );
        top_bearing    = (FT_Short)( loader->pp3.y - bbox.yMax );
      }
      else
      {
        /* Make up the distances from the horizontal header.   */

        /* NOTE: The OS/2 values are the only `portable' ones, */
        /*       which is why we use them, if there is an OS/2 */
        /*       table in the font.  Otherwise, we use the     */
        /*       values defined in the horizontal header.      */
        /*                                                     */
        /* NOTE2: The sTypoDescender is negative, which is why */
        /*        we compute the baseline-to-baseline distance */
        /*        here with:                                   */
        /*             ascender - descender + linegap          */
        /*                                                     */
        /* NOTE3: This is different from what MS's rasterizer  */
        /*        appears to do when getting default values    */
        /*        for the vertical phantom points.  We leave   */
        /*        the old code untouched, but relying on       */
        /*        phantom points alone might be reasonable     */
        /*        (i.e., removing the `if' above).             */
        if ( face->os2.version != 0xFFFFU )
        {
          top_bearing    = (FT_Short)( face->os2.sTypoLineGap / 2 );
          advance_height = (FT_UShort)( face->os2.sTypoAscender -
                                        face->os2.sTypoDescender +
                                        face->os2.sTypoLineGap );
        }
        else
        {
          top_bearing    = (FT_Short)( face->horizontal.Line_Gap / 2 );
          advance_height = (FT_UShort)( face->horizontal.Ascender  +
                                        face->horizontal.Descender +
                                        face->horizontal.Line_Gap );
        }
      }

#ifdef FT_CONFIG_OPTION_INCREMENTAL

      /* If this is an incrementally loaded font see if there are */
      /* overriding metrics for this glyph.                       */
      if ( face->root.internal->incremental_interface &&
           face->root.internal->incremental_interface->funcs->get_glyph_metrics )
      {
        FT_Incremental_MetricsRec  metrics;
        FT_Error                   error = TT_Err_Ok;


        metrics.bearing_x = 0;
        metrics.bearing_y = top_bearing;
        metrics.advance = advance_height;
        error =
          face->root.internal->incremental_interface->funcs->get_glyph_metrics(
            face->root.internal->incremental_interface->object,
            glyph_index, TRUE, &metrics );

        if ( error )
          return error;

        top_bearing    = (FT_Short)metrics.bearing_y;
        advance_height = (FT_UShort)metrics.advance;
      }

      /* GWW: Do vertical metrics get loaded incrementally too? */

#endif /* FT_CONFIG_OPTION_INCREMENTAL */

      /* We must adjust the top_bearing value from the bounding box given */
      /* in the glyph header to the bounding box calculated with          */
      /* FT_Get_Outline_CBox().                                           */

      /* scale the metrics */
      if ( !( loader->load_flags & FT_LOAD_NO_SCALE ) )
      {
        top     = FT_MulFix( top_bearing + loader->bbox.yMax, y_scale )
                    - bbox.yMax;
        advance = FT_MulFix( advance_height, y_scale );
      }
      else
      {
        top     = top_bearing + loader->bbox.yMax - bbox.yMax;
        advance = advance_height;
      }

      /* set the advance height in design units.  It is scaled later by */
      /* the base layer.                                                */
      glyph->linearVertAdvance = advance_height;

      /* XXX: for now, we have no better algorithm for the lsb, but it */
      /*      should work fine.                                        */
      /*                                                               */
      left = ( bbox.xMin - bbox.xMax ) / 2;

      /* grid-fit them if necessary */
      if ( IS_HINTED( loader->load_flags ) )
      {
        left    = FT_PIX_FLOOR( left );
        top     = FT_PIX_CEIL( top );
        advance = FT_PIX_ROUND( advance );
      }

      glyph->metrics.vertBearingX = left;
      glyph->metrics.vertBearingY = top;
      glyph->metrics.vertAdvance  = advance;
    }

    /* adjust advance width to the value contained in the hdmx table */
    if ( !face->postscript.isFixedPitch && size &&
         IS_HINTED( loader->load_flags )        )
    {
      FT_Byte*  widthp = Get_Advance_WidthPtr( face,
                                               size->root.metrics.x_ppem,
                                               glyph_index );


      if ( widthp )
        glyph->metrics.horiAdvance = *widthp << 6;
    }

    /* set glyph dimensions */
    glyph->metrics.width  = bbox.xMax - bbox.xMin;
    glyph->metrics.height = bbox.yMax - bbox.yMin;

    return 0;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    TT_Load_Glyph                                                      */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A function used to load a single glyph within a given glyph slot,  */
  /*    for a given size.                                                  */
  /*                                                                       */
  /* <Input>                                                               */
  /*    glyph       :: A handle to a target slot object where the glyph    */
  /*                   will be loaded.                                     */
  /*                                                                       */
  /*    size        :: A handle to the source face size at which the glyph */
  /*                   must be scaled/loaded.                              */
  /*                                                                       */
  /*    glyph_index :: The index of the glyph in the font file.            */
  /*                                                                       */
  /*    load_flags  :: A flag indicating what to load for this glyph.  The */
  /*                   FT_LOAD_XXX constants can be used to control the    */
  /*                   glyph loading process (e.g., whether the outline    */
  /*                   should be scaled, whether to load bitmaps or not,   */
  /*                   whether to hint the outline, etc).                  */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  FT_LOCAL_DEF( FT_Error )
  TT_Load_Glyph( TT_Size       size,
                 TT_GlyphSlot  glyph,
                 FT_UInt       glyph_index,
                 FT_Int32      load_flags )
  {
    SFNT_Service  sfnt;
    TT_Face       face;
    FT_Stream     stream;
    FT_Error      error;
    TT_LoaderRec  loader;


    face   = (TT_Face)glyph->face;
    sfnt   = (SFNT_Service)face->sfnt;
    stream = face->root.stream;
    error  = TT_Err_Ok;

    if ( !size || ( load_flags & FT_LOAD_NO_SCALE )   ||
                  ( load_flags & FT_LOAD_NO_RECURSE ) )
    {
      size        = NULL;
      load_flags |= FT_LOAD_NO_SCALE   |
                    FT_LOAD_NO_HINTING |
                    FT_LOAD_NO_BITMAP;
    }

    glyph->num_subglyphs = 0;

#ifdef TT_CONFIG_OPTION_EMBEDDED_BITMAPS

    /* try to load embedded bitmap if any              */
    /*                                                 */
    /* XXX: The convention should be emphasized in     */
    /*      the documents because it can be confusing. */
    if ( size                                    &&
         size->strike_index != 0xFFFFU           &&
         sfnt->load_sbits                        &&
         ( load_flags & FT_LOAD_NO_BITMAP ) == 0 )

    {
      TT_SBit_MetricsRec  metrics;


      error = sfnt->load_sbit_image( face,
                                     (FT_ULong)size->strike_index,
                                     glyph_index,
                                     (FT_Int)load_flags,
                                     stream,
                                     &glyph->bitmap,
                                     &metrics );
      if ( !error )
      {
        glyph->outline.n_points   = 0;
        glyph->outline.n_contours = 0;

        glyph->metrics.width  = (FT_Pos)metrics.width  << 6;
        glyph->metrics.height = (FT_Pos)metrics.height << 6;

        glyph->metrics.horiBearingX = (FT_Pos)metrics.horiBearingX << 6;
        glyph->metrics.horiBearingY = (FT_Pos)metrics.horiBearingY << 6;
        glyph->metrics.horiAdvance  = (FT_Pos)metrics.horiAdvance  << 6;

        glyph->metrics.vertBearingX = (FT_Pos)metrics.vertBearingX << 6;
        glyph->metrics.vertBearingY = (FT_Pos)metrics.vertBearingY << 6;
        glyph->metrics.vertAdvance  = (FT_Pos)metrics.vertAdvance  << 6;

        glyph->format = FT_GLYPH_FORMAT_BITMAP;
        if ( load_flags & FT_LOAD_VERTICAL_LAYOUT )
        {
          glyph->bitmap_left = metrics.vertBearingX;
          glyph->bitmap_top  = metrics.vertBearingY;
        }
        else
        {
          glyph->bitmap_left = metrics.horiBearingX;
          glyph->bitmap_top  = metrics.horiBearingY;
        }
        return error;
      }
    }

#endif /* TT_CONFIG_OPTION_EMBEDDED_BITMAPS */

    /* return immediately if we only want the embedded bitmaps */
    if ( load_flags & FT_LOAD_SBITS_ONLY )
      return TT_Err_Invalid_Argument;

    /* seek to the beginning of the glyph table.  For Type 42 fonts      */
    /* the table might be accessed from a Postscript stream or something */
    /* else...                                                           */

#ifdef FT_CONFIG_OPTION_INCREMENTAL

    /* Don't look for the glyph table if this is an incremental font. */
    if ( !face->root.internal->incremental_interface )

#endif

    {
      error = face->goto_table( face, TTAG_glyf, stream, 0 );
      if ( error )
      {
        FT_ERROR(( "TT_Load_Glyph: could not access glyph table\n" ));
        goto Exit;
      }
    }

    FT_MEM_ZERO( &loader, sizeof ( loader ) );

    /* update the glyph zone bounds */
    {
      FT_GlyphLoader  gloader = FT_FACE_DRIVER( face )->glyph_loader;


      loader.gloader = gloader;

      FT_GlyphLoader_Rewind( gloader );

      tt_prepare_zone( &loader.zone, &gloader->base, 0, 0 );
      tt_prepare_zone( &loader.base, &gloader->base, 0, 0 );
    }

#ifdef TT_CONFIG_OPTION_BYTECODE_INTERPRETER

    if ( size )
    {
      /* query new execution context */
      loader.exec = size->debug ? size->context : TT_New_Context( face );
      if ( !loader.exec )
        return TT_Err_Could_Not_Find_Context;

      TT_Load_Context( loader.exec, face, size );
      loader.instructions = loader.exec->glyphIns;

      /* load default graphics state - if needed */
      if ( size->GS.instruct_control & 2 )
        loader.exec->GS = tt_default_graphics_state;

      loader.exec->pedantic_hinting =
         FT_BOOL( load_flags & FT_LOAD_PEDANTIC );

      loader.exec->grayscale =
         FT_BOOL( FT_LOAD_TARGET_MODE( load_flags ) != FT_LOAD_TARGET_MONO );
    }

#endif /* TT_CONFIG_OPTION_BYTECODE_INTERPRETER */

    /* clear all outline flags, except the `owner' one */
    glyph->outline.flags = 0;

    /* let's initialize the rest of our loader now */

    loader.load_flags    = load_flags;

    loader.face   = (FT_Face)face;
    loader.size   = (FT_Size)size;
    loader.glyph  = (FT_GlyphSlot)glyph;
    loader.stream = stream;

#ifdef FT_CONFIG_OPTION_INCREMENTAL

    if ( face->root.internal->incremental_interface )
      loader.glyf_offset = 0;
    else

#endif

      loader.glyf_offset = FT_STREAM_POS();

#ifdef TT_CONFIG_OPTION_BYTECODE_INTERPRETER

    /* if the cvt program has disabled hinting, the argument */
    /* is ignored.                                           */
    if ( size && ( size->GS.instruct_control & 1 ) )
      loader.load_flags |= FT_LOAD_NO_HINTING;

#endif /* TT_CONFIG_OPTION_BYTECODE_INTERPRETER */

    /* Main loading loop */
    glyph->format        = FT_GLYPH_FORMAT_OUTLINE;
    glyph->num_subglyphs = 0;

    error = load_truetype_glyph( &loader, glyph_index, 0 );
    if ( !error )
      compute_glyph_metrics( &loader, glyph_index );

#ifdef TT_CONFIG_OPTION_BYTECODE_INTERPRETER

    if ( !size || !size->debug )
      TT_Done_Context( loader.exec );

#endif /* TT_CONFIG_OPTION_BYTECODE_INTERPRETER */

    /* Set the `high precision' bit flag.                           */
    /* This is _critical_ to get correct output for monochrome      */
    /* TrueType glyphs at all sizes using the bytecode interpreter. */
    /*                                                              */
    if ( size && size->root.metrics.y_ppem < 24 )
      glyph->outline.flags |= FT_OUTLINE_HIGH_PRECISION;

  Exit:
    return error;
  }


/* END */
