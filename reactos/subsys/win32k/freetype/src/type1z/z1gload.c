/***************************************************************************/
/*                                                                         */
/*  z1gload.c                                                              */
/*                                                                         */
/*    Experimental Type 1 Glyph Loader (body).                             */
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


#ifdef FT_FLAT_COMPILE

#include "z1gload.h"

#else

#include <freetype/src/type1z/z1gload.h>

#endif


#include <freetype/internal/ftdebug.h>
#include <freetype/internal/ftstream.h>
#include <freetype/ftoutln.h>

#include <string.h>     /* for strcmp() */


  /*************************************************************************/
  /*                                                                       */
  /* The macro FT_COMPONENT is used in trace mode.  It is an implicit      */
  /* parameter of the FT_TRACE() and FT_ERROR() macros, used to print/log  */
  /* messages during execution.                                            */
  /*                                                                       */
#undef  FT_COMPONENT
#define FT_COMPONENT  trace_z1gload


  typedef enum  Z1_Operator_
  {
    op_none = 0,
    op_endchar,
    op_hsbw,
    op_seac,
    op_sbw,
    op_closepath,
    op_hlineto,
    op_hmoveto,
    op_hvcurveto,
    op_rlineto,
    op_rmoveto,
    op_rrcurveto,
    op_vhcurveto,
    op_vlineto,
    op_vmoveto,
    op_dotsection,
    op_hstem,
    op_hstem3,
    op_vstem,
    op_vstem3,
    op_div,
    op_callothersubr,
    op_callsubr,
    op_pop,
    op_return,
    op_setcurrentpoint,

    op_max    /* never remove this one */

  } Z1_Operator;

  static
  const FT_Int  t1_args_count[op_max] =
  {
    0, /* none */
    0, /* endchar */
    2, /* hsbw */
    5, /* seac */
    4, /* sbw */
    0, /* closepath */
    1, /* hlineto */
    1, /* hmoveto */
    4, /* hvcurveto */
    2, /* rlineto */
    2, /* rmoveto */
    6, /* rrcurveto */
    4, /* vhcurveto */
    1, /* vlineto */
    1, /* vmoveto */
    0, /* dotsection */
    2, /* hstem */
    6, /* hstem3 */
    2, /* vstem */
    6, /* vstem3 */
    2, /* div */
   -1, /* callothersubr */
    1, /* callsubr */
    0, /* pop */
    0, /* return */
    2  /* setcurrentpoint */
  };


  /*************************************************************************/
  /*************************************************************************/
  /*************************************************************************/
  /**********                                                      *********/
  /**********                                                      *********/
  /**********             GENERIC CHARSTRING PARSING               *********/
  /**********                                                      *********/
  /**********                                                      *********/
  /*************************************************************************/
  /*************************************************************************/
  /*************************************************************************/


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    Z1_Init_Builder                                                    */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Initializes a given glyph builder.                                 */
  /*                                                                       */
  /* <InOut>                                                               */
  /*    builder :: A pointer to the glyph builder to initialize.           */
  /*                                                                       */
  /* <Input>                                                               */
  /*    face    :: The current face object.                                */
  /*                                                                       */
  /*    size    :: The current size object.                                */
  /*                                                                       */
  /*    glyph   :: The current glyph object.                               */
  /*                                                                       */
  LOCAL_FUNC
  void  Z1_Init_Builder( Z1_Builder*   builder,
                         T1_Face       face,
                         Z1_Size       size,
                         Z1_GlyphSlot  glyph )
  {
    builder->path_begun  = 0;
    builder->load_points = 1;

    builder->face   = face;
    builder->glyph  = glyph;
    builder->memory = face->root.memory;

    if ( glyph )
    {
      FT_GlyphLoader*  loader = glyph->root.loader;


      builder->loader  = loader;
      builder->current = &loader->current.outline;
      builder->base    = &loader->base.outline;

      FT_GlyphLoader_Rewind( loader );
    }

    if ( size )
    {
      builder->scale_x = size->root.metrics.x_scale;
      builder->scale_y = size->root.metrics.y_scale;
    }

    builder->pos_x = 0;
    builder->pos_y = 0;

    builder->left_bearing.x = 0;
    builder->left_bearing.y = 0;
    builder->advance.x      = 0;
    builder->advance.y      = 0;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    Z1_Done_Builder                                                    */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Finalizes a given glyph builder.  Its contents can still be used   */
  /*    after the call, but the function saves important information       */
  /*    within the corresponding glyph slot.                               */
  /*                                                                       */
  /* <Input>                                                               */
  /*    builder :: A pointer to the glyph builder to finalize.             */
  /*                                                                       */
  LOCAL_FUNC
  void Z1_Done_Builder( Z1_Builder*  builder )
  {
    Z1_GlyphSlot  glyph = builder->glyph;


    if ( glyph )
      glyph->root.outline = *builder->base;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    Z1_Init_Decoder                                                    */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Initializes a given glyph decoder.                                 */
  /*                                                                       */
  /* <InOut>                                                               */
  /*    decoder :: A pointer to the glyph builder to initialize.           */
  /*                                                                       */
  LOCAL_FUNC
  void  Z1_Init_Decoder( Z1_Decoder* decoder )
  {
    decoder->top              = 0;
    decoder->zone             = 0;
    decoder->flex_state       = 0;
    decoder->num_flex_vectors = 0;
    decoder->blend            = 0;

    /* Clear loader */
    MEM_Set( &decoder->builder, 0, sizeof ( decoder->builder ) );
  }


  /* check that there is enough space for `count' more points */
  static
  FT_Error  check_points( Z1_Builder*  builder,
                          FT_Int       count )
  {
    return FT_GlyphLoader_Check_Points( builder->loader, count, 0 );
  }


  /* add a new point; do not check space */
  static
  void  add_point( Z1_Builder*  builder,
                   FT_Pos       x,
                   FT_Pos       y,
                   FT_Byte      flag )
  {
    FT_Outline*  outline = builder->current;


    if ( builder->load_points )
    {
      FT_Vector*  point   = outline->points + outline->n_points;
      FT_Byte*    control = (FT_Byte*)outline->tags + outline->n_points;


      point->x = x;
      point->y = y;
      *control = flag ? FT_Curve_Tag_On : FT_Curve_Tag_Cubic;

      builder->last = *point;
    }

    outline->n_points++;
  }


  /* check space for a new on-curve point, then add it */
  static
  FT_Error  add_point1( Z1_Builder*  builder,
                        FT_Pos       x,
                        FT_Pos       y )
  {
    FT_Error  error;


    error = check_points( builder, 1 );
    if ( !error )
      add_point( builder, x, y, 1 );

    return error;
  }


  /* check space for a new contour, then add it */
  static
  FT_Error  add_contour( Z1_Builder*  builder )
  {
    FT_Outline*  outline = builder->current;
    FT_Error     error;


    if ( !builder->load_points )
    {
      outline->n_contours++;
      return FT_Err_Ok;
    }

    /* reallocate contours array if necessary */
    error = FT_GlyphLoader_Check_Points( builder->loader, 0, 1 );
    if ( !error )
    {
      if ( outline->n_contours > 0 )
        outline->contours[outline->n_contours - 1] = outline->n_points - 1;

      outline->n_contours++;
    }

    return error;
  }


  /* if a path was begun, add its first on-curve point */
  static
  FT_Error  start_point( Z1_Builder*  builder,
                         FT_Pos       x,
                         FT_Pos       y )
  {
    /* test whether we are building a new contour */
    if ( !builder->path_begun )
    {
      FT_Error  error;


      builder->path_begun = 1;
      error = add_contour( builder );
      if ( error )
        return error;
    }
    return add_point1( builder, x, y );
  }


  /* close the current contour */
  static
  void  close_contour( Z1_Builder*  builder )
  {
    FT_Outline*  outline = builder->current;


    /* XXX: we must not include the last point in the path if it */
    /*      is located on the first point                        */
    if ( outline->n_points > 1 )
    {
      FT_Int      first = 0;
      FT_Vector*  p1    = outline->points + first;
      FT_Vector*  p2    = outline->points + outline->n_points-1;


      if ( outline->n_contours > 1 )
      {
        first = outline->contours[outline->n_contours - 2] + 1;
        p1    = outline->points + first;
      }

      if ( p1->x == p2->x && p1->y == p2->y )
        outline->n_points--;
    }

    if ( outline->n_contours > 0 )
      outline->contours[outline->n_contours - 1] = outline->n_points - 1;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    lookup_glyph_by_stdcharcode                                        */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Looks up a given glyph by its StandardEncoding charcode.  Used     */
  /*    to implement the SEAC Type 1 operator.                             */
  /*                                                                       */
  /* <Input>                                                               */
  /*    face     :: The current face object.                               */
  /*                                                                       */
  /*    charcode :: The character code to look for.                        */
  /*                                                                       */
  /* <Return>                                                              */
  /*    A glyph index in the font face.  Returns -1 if the corresponding   */
  /*    glyph wasn't found.                                                */
  /*                                                                       */
  static
  FT_Int  lookup_glyph_by_stdcharcode( T1_Face  face,
                                       FT_Int   charcode )
  {
    FT_Int              n;
    const FT_String*    glyph_name;
    PSNames_Interface*  psnames = (PSNames_Interface*)face->psnames;


    /* check range of standard char code */
    if ( charcode < 0 || charcode > 255 )
      return -1;

    glyph_name = psnames->adobe_std_strings(
                   psnames->adobe_std_encoding[charcode]);

    for ( n = 0; n < face->type1.num_glyphs; n++ )
    {
      FT_String*  name = (FT_String*)face->type1.glyph_names[n];


      if ( name && strcmp( name,glyph_name ) == 0 )
        return n;
    }

    return -1;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    t1operator_seac                                                    */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Implements the `seac' Type 1 operator for a Type 1 decoder.        */
  /*                                                                       */
  /* <Input>                                                               */
  /*    decoder :: The current CID decoder.                                */
  /*                                                                       */
  /*    asb     :: The accent's side bearing.                              */
  /*                                                                       */
  /*    adx     :: The horizontal offset of the accent.                    */
  /*                                                                       */
  /*    ady     :: The vertical offset of the accent.                      */
  /*                                                                       */
  /*    bchar   :: The base character's StandardEncoding charcode.         */
  /*                                                                       */
  /*    achar   :: The accent character's StandardEncoding charcode.       */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  static
  FT_Error  t1operator_seac( Z1_Decoder*  decoder,
                             FT_Pos       asb,
                             FT_Pos       adx,
                             FT_Pos       ady,
                             FT_Int       bchar,
                             FT_Int       achar )
  {
    FT_Error     error;
    FT_Int       bchar_index, achar_index, n_base_points;
    FT_Outline*  base = decoder->builder.base;
    FT_Vector    left_bearing, advance;
    T1_Face      face  = decoder->builder.face;
    T1_Font*     type1 = &face->type1;


    bchar_index = lookup_glyph_by_stdcharcode( face, bchar );
    achar_index = lookup_glyph_by_stdcharcode( face, achar );

    if ( bchar_index < 0 || achar_index < 0 )
    {
      FT_ERROR(( "t1operator_seac:" ));
      FT_ERROR(( " invalid seac character code arguments\n" ));
      return T1_Err_Syntax_Error;
    }

    /* if we are trying to load a composite glyph, do not load the */
    /* accent character and return the array of subglyphs.         */
    if ( decoder->builder.no_recurse )
    {
      FT_GlyphSlot     glyph  = (FT_GlyphSlot)decoder->builder.glyph;
      FT_GlyphLoader*  loader = glyph->loader;
      FT_SubGlyph*     subg;


      /* reallocate subglyph array if necessary */
      error = FT_GlyphLoader_Check_Subglyphs( loader, 2 );
      if ( error )
        goto Exit;

      subg = loader->current.subglyphs;

      /* subglyph 0 = base character */
      subg->index = bchar_index;
      subg->flags = FT_SUBGLYPH_FLAG_ARGS_ARE_XY_VALUES |
                    FT_SUBGLYPH_FLAG_USE_MY_METRICS;
      subg->arg1  = 0;
      subg->arg2  = 0;
      subg++;

      /* subglyph 1 = accent character */
      subg->index = achar_index;
      subg->flags = FT_SUBGLYPH_FLAG_ARGS_ARE_XY_VALUES;
      subg->arg1  = adx - asb;
      subg->arg2  = ady;

      /* set up remaining glyph fields */
      glyph->num_subglyphs = 2;
      glyph->subglyphs     = loader->base.subglyphs;
      glyph->format        = ft_glyph_format_composite;

      loader->current.num_subglyphs = 2;
    }

    /* First load `bchar' in builder */
    /* now load the unscaled outline */

    FT_GlyphLoader_Prepare( decoder->builder.loader );  /* prepare loader */

    error = Z1_Parse_CharStrings( decoder,
                                  type1->charstrings    [bchar_index],
                                  type1->charstrings_len[bchar_index],
                                  type1->num_subrs,
                                  type1->subrs,
                                  type1->subrs_len );
    if ( error )
      goto Exit;

    n_base_points = base->n_points;

    /* save the left bearing and width of the base character */
    /* as they will be erased by the next load.              */

    left_bearing = decoder->builder.left_bearing;
    advance      = decoder->builder.advance;

    decoder->builder.left_bearing.x = 0;
    decoder->builder.left_bearing.y = 0;

    /* Now load `achar' on top of */
    /* the base outline           */
    error = Z1_Parse_CharStrings( decoder,
                                  type1->charstrings    [achar_index],
                                  type1->charstrings_len[achar_index],
                                  type1->num_subrs,
                                  type1->subrs,
                                  type1->subrs_len );
    if ( error )
      return error;

    /* restore the left side bearing and   */
    /* advance width of the base character */

    decoder->builder.left_bearing = left_bearing;
    decoder->builder.advance      = advance;

    /* Finally, move the accent */
    if ( decoder->builder.load_points )
    {
      FT_Outline  dummy;


      dummy.n_points = base->n_points - n_base_points;
      dummy.points   = base->points   + n_base_points;
      FT_Outline_Translate( &dummy, adx - asb, ady );
    }

  Exit:
    return error;
  }


#define USE_ARGS( n )  do                            \
                       {                             \
                         top -= n;                   \
                         if ( top < decoder->stack ) \
                           goto Stack_Underflow;     \
                       } while ( 0 )


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    Z1_Parse_CharStrings                                               */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Parses a given Type 1 charstrings program.                         */
  /*                                                                       */
  /* <Input>                                                               */
  /*    decoder         :: The current Type 1 decoder.                     */
  /*                                                                       */
  /*    charstring_base :: The base address of the charstring stream.      */
  /*                                                                       */
  /*    charstring_len  :: The length in bytes of the charstring stream.   */
  /*                                                                       */
  /*    num_subrs       :: The number of sub-routines.                     */
  /*                                                                       */
  /*    subrs_base      :: An array of sub-routines addresses.             */
  /*                                                                       */
  /*    subrs_len       :: An array of sub-routines lengths.               */
  /*                                                                       */
  /* <Return>                                                              */
  /*    Free error code.  0 means success.                                 */
  /*                                                                       */
  LOCAL_FUNC
  FT_Error  Z1_Parse_CharStrings( Z1_Decoder*  decoder,
                                  FT_Byte*     charstring_base,
                                  FT_Int       charstring_len,
                                  FT_Int       num_subrs,
                                  FT_Byte**    subrs_base,
                                  FT_Int*      subrs_len )
  {
    FT_Error          error;
    Z1_Decoder_Zone*  zone;
    FT_Byte*          ip;
    FT_Byte*          limit;
    Z1_Builder*       builder = &decoder->builder;
    FT_Outline*       outline;
    FT_Pos            x, y;


    /* First of all, initialize the decoder */
    decoder->top  = decoder->stack;
    decoder->zone = decoder->zones;
    zone          = decoder->zones;

    builder->path_begun  = 0;

    zone->base           = charstring_base;
    limit = zone->limit  = charstring_base + charstring_len;
    ip    = zone->cursor = zone->base;

    error   = T1_Err_Ok;
    outline = builder->current;

    x = builder->pos_x;
    y = builder->pos_y;

    /* now, execute loop */
    while ( ip < limit )
    {
      FT_Int*      top   = decoder->top;
      Z1_Operator  op    = op_none;
      FT_Long      value = 0;


      /*********************************************************************/
      /*                                                                   */
      /* Decode operator or operand                                        */
      /*                                                                   */
      /*                                                                   */

      /* first of all, decompress operator or value */
      switch ( *ip++ )
      {
      case 1:
        op = op_hstem;
        break;

      case 3:
        op = op_vstem;
        break;
      case 4:
        op = op_vmoveto;
        break;
      case 5:
        op = op_rlineto;
        break;
      case 6:
        op = op_hlineto;
        break;
      case 7:
        op = op_vlineto;
        break;
      case 8:
        op = op_rrcurveto;
        break;
      case 9:
        op = op_closepath;
        break;
      case 10:
        op = op_callsubr;
        break;
      case 11:
        op = op_return;
        break;

      case 13:
        op = op_hsbw;
        break;
      case 14:
        op = op_endchar;
        break;

      case 21:
        op = op_rmoveto;
        break;
      case 22:
        op = op_hmoveto;
        break;

      case 30:
        op = op_vhcurveto;
        break;
      case 31:
        op = op_hvcurveto;
        break;

      case 12:
        if ( ip > limit )
        {
          FT_ERROR(( "Z1_Parse_CharStrings: invalid escape (12+EOF)\n" ));
          goto Syntax_Error;
        }

        switch ( *ip++ )
        {
        case 0:
          op = op_dotsection;
          break;
        case 1:
          op = op_vstem3;
          break;
        case 2:
          op = op_hstem3;
          break;
        case 6:
          op = op_seac;
          break;
        case 7:
          op = op_sbw;
          break;
        case 12:
          op = op_div;
          break;
        case 16:
          op = op_callothersubr;
          break;
        case 17:
          op = op_pop;
          break;
        case 33:
          op = op_setcurrentpoint;
          break;

        default:
          FT_ERROR(( "Z1_Parse_CharStrings: invalid escape (12+%d)\n",
                     ip[-1] ));
          goto Syntax_Error;
        }
        break;

      case 255:    /* four bytes integer */
        if ( ip + 4 > limit )
        {
          FT_ERROR(( "Z1_Parse_CharStrings: unexpected EOF in integer\n" ));
          goto Syntax_Error;
        }

        value = ( (FT_Long)ip[0] << 24 ) |
                ( (FT_Long)ip[1] << 16 ) |
                ( (FT_Long)ip[2] << 8  ) |
                           ip[3];
        ip += 4;
        break;

      default:
        if ( ip[-1] >= 32 )
        {
          if ( ip[-1] < 247 )
            value = (FT_Long)ip[-1] - 139;
          else
          {
            if ( ++ip > limit )
            {
              FT_ERROR(( "Z1_Parse_CharStrings:" ));
              FT_ERROR(( " unexpected EOF in integer\n" ));
              goto Syntax_Error;
            }

          if ( ip[-2] < 251 )
            value =  ( (FT_Long)( ip[-2] - 247 ) << 8 ) + ip[-1] + 108;
          else
            value = -( ( ( (FT_Long)ip[-2] - 251 ) << 8 ) + ip[-1] + 108 );
          }
        }
        else
        {
          FT_ERROR(( "Z1_Parse_CharStrings: invalid byte (%d)\n",
                     ip[-1] ));
          goto Syntax_Error;
        }
      }

      /*********************************************************************/
      /*                                                                   */
      /*  Push value on stack, or process operator                         */
      /*                                                                   */
      /*                                                                   */
      if ( op == op_none )
      {
        if ( top - decoder->stack >= T1_MAX_CHARSTRINGS_OPERANDS )
        {
          FT_ERROR(( "Z1_Parse_CharStrings: stack overflow!\n" ));
          goto Syntax_Error;
        }

        FT_TRACE4(( " %ld", value ));

        *top++       = value;
        decoder->top = top;
      }
      else if ( op == op_callothersubr )  /* callothersubr */
      {
        FT_TRACE4(( " callothersubr" ));

        if ( top - decoder->stack < 2 )
          goto Stack_Underflow;

        top -= 2;
        switch ( top[1] )
        {
        case 1:                     /* start flex feature */
          if ( top[0] != 0 )
            goto Unexpected_OtherSubr;

          decoder->flex_state        = 1;
          decoder->num_flex_vectors  = 0;
          if ( start_point( builder, x, y ) ||
               check_points( builder, 6 )   )
            goto Memory_Error;
          break;

        case 2:                     /* add flex vectors */
          {
            FT_Int  index;

            if ( top[0] != 0 )
              goto Unexpected_OtherSubr;

            /* note that we should not add a point for index 0; */
            /* this will move our current position to the flex  */
            /* point without adding any point to the outline    */
            index = decoder->num_flex_vectors++;
            if ( index > 0 && index < 7 )
              add_point( builder,
                         x,
                         y,
                         (FT_Byte)( index == 3 || index == 6 ) );
          }
          break;

        case 0:                     /* end flex feature */
          if ( top[0] != 3 )
            goto Unexpected_OtherSubr;

          if ( decoder->flex_state       == 0 ||
               decoder->num_flex_vectors != 7 )
          {
            FT_ERROR(( "Z1_Parse_CharStrings: unexpected flex end\n" ));
            goto Syntax_Error;
          }

          /* now consume the remaining `pop pop setcurpoint' */
          if ( ip + 6 > limit ||
               ip[0] != 12 || ip[1] != 17 || /* pop */
               ip[2] != 12 || ip[3] != 17 || /* pop */
               ip[4] != 12 || ip[5] != 33 )  /* setcurpoint */
          {
            FT_ERROR(( "Z1_Parse_CharStrings: invalid flex charstring\n" ));
            goto Syntax_Error;
          }

          ip += 6;
          decoder->flex_state = 0;
          break;

        case 3:                     /* change hints */
          if ( top[0] != 1 )
            goto Unexpected_OtherSubr;

          /* eat the following `pop' */
          if ( ip + 2 > limit )
          {
            FT_ERROR(( "Z1_Parse_CharStrings: invalid escape (12+%d)\n",
                       ip[-1] ));
            goto Syntax_Error;
          }

          if ( ip[0] != 12 || ip[1] != 17 )
          {
            FT_ERROR(( "Z1_Parse_CharStrings:" ));
            FT_ERROR(( " `pop' expected, found (%d %d)\n",
                       ip[0], ip[1] ));
            goto Syntax_Error;
          }
          ip += 2;
          break;

        case 12:
        case 13:
          /* counter control hints, clear stack */
          top = decoder->stack;
          break;

        case 14:
        case 15:
        case 16:
        case 17:
        case 18:                    /* multiple masters */
          {
            T1_Blend*  blend = decoder->blend;
            FT_UInt    num_points, nn, mm;
            FT_Int*    delta;
            FT_Int*    values;


            if ( !blend )
            {
              FT_ERROR(( "Z1_Parse_CharStrings:" ));
              FT_ERROR(( " unexpected multiple masters operator!\n" ));
              goto Syntax_Error;
            }

            num_points = top[1] - 13 + ( top[1] == 18 );
            if ( top[0] != (FT_Int)( num_points * blend->num_designs ) )
            {
              FT_ERROR(( "Z1_Parse_CharStrings:" ));
              FT_ERROR(( " incorrect number of mm arguments\n" ));
              goto Syntax_Error;
            }

            top -= blend->num_designs*num_points;
            if ( top < decoder->stack )
              goto Stack_Underflow;

            /* we want to compute:                                   */
            /*                                                       */
            /*  a0*w0 + a1*w1 + ... + ak*wk                          */
            /*                                                       */
            /* but we only have the a0, a1-a0, a2-a0, .. ak-a0       */
            /* however, given that w0 + w1 + ... + wk == 1, we can   */
            /* rewrite it easily as:                                 */
            /*                                                       */
            /*  a0 + (a1-a0)*w1 + (a2-a0)*w2 + .. + (ak-a0)*wk       */
            /*                                                       */
            /* where k == num_designs-1                              */
            /*                                                       */
            /* I guess that's why it's written in this `compact'     */
            /* form.                                                 */
            /*                                                       */
            delta  = top + num_points;
            values = top;
            for ( nn = 0; nn < num_points; nn++ )
            {
              FT_Int  x = values[0];


              for ( mm = 1; mm < blend->num_designs; mm++ )
                x += FT_MulFix( *delta++, blend->weight_vector[mm] );

              *values++ = x;
            }
            /* note that `top' will be incremented later by calls to `pop' */
            break;
          }

        default:
        Unexpected_OtherSubr:
          FT_ERROR(( "Z1_Parse_CharStrings: invalid othersubr [%d %d]!\n",
                     top[0], top[1] ));
          goto Syntax_Error;
        }
        decoder->top = top;
      }
      else  /* general operator */
      {
        FT_Int  num_args = t1_args_count[op];


        if ( top - decoder->stack < num_args )
          goto Stack_Underflow;

        top -= num_args;

        switch ( op )
        {
        case op_endchar:
          FT_TRACE4(( " endchar" ));

          close_contour( builder );

          /* add current outline to the glyph slot */
          FT_GlyphLoader_Add( builder->loader );

          /* return now! */
          FT_TRACE4(( "\n\n" ));
          return T1_Err_Ok;

        case op_hsbw:
          FT_TRACE4(( " hsbw" ));

          builder->left_bearing.x += top[0];
          builder->advance.x       = top[1];
          builder->advance.y       = 0;

          builder->last.x = x = top[0];
          builder->last.y = y = 0;

          /* the `metrics_only' indicates that we only want to compute */
          /* the glyph's metrics (lsb + advance width), not load the   */
          /* rest of it; so exit immediately                           */
          if ( builder->metrics_only )
            return T1_Err_Ok;

          break;

        case op_seac:
          /* return immediately after the processing */
          return t1operator_seac( decoder, top[0], top[1],
                                           top[2], top[3], top[4] );

        case op_sbw:
          FT_TRACE4(( " sbw" ));

          builder->left_bearing.x += top[0];
          builder->left_bearing.y += top[1];
          builder->advance.x       = top[2];
          builder->advance.y       = top[3];

          builder->last.x = x = top[0];
          builder->last.y = y = top[1];

          /* the `metrics_only' indicates that we only want to compute */
          /* the glyph's metrics (lsb + advance width), not load the   */
          /* rest of it; so exit immediately                           */
          if ( builder->metrics_only )
            return T1_Err_Ok;

          break;

        case op_closepath:
          FT_TRACE4(( " closepath" ));

          close_contour( builder );
          builder->path_begun = 0;
          break;

        case op_hlineto:
          FT_TRACE4(( " hlineto" ));

          if ( start_point( builder, x, y ) )
            goto Memory_Error;

          x += top[0];
          goto Add_Line;

        case op_hmoveto:
          FT_TRACE4(( " hmoveto" ));

          x += top[0];
          break;

        case op_hvcurveto:
          FT_TRACE4(( " hvcurveto" ));

          if ( start_point( builder, x, y ) ||
               check_points( builder, 3 )   )
            goto Memory_Error;

          x += top[0];
          add_point( builder, x, y, 0 );
          x += top[1];
          y += top[2];
          add_point( builder, x, y, 0 );
          y += top[3];
          add_point( builder, x, y, 1 );
          break;

        case op_rlineto:
          FT_TRACE4(( " rlineto" ));

          if ( start_point( builder, x, y ) )
            goto Memory_Error;

          x += top[0];
          y += top[1];

        Add_Line:
          if ( add_point1( builder, x, y ) )
            goto Memory_Error;
          break;

        case op_rmoveto:
          FT_TRACE4(( " rmoveto" ));

          x += top[0];
          y += top[1];
          break;

        case op_rrcurveto:
          FT_TRACE4(( " rcurveto" ));

          if ( start_point( builder, x, y ) ||
               check_points( builder, 3 )   )
            goto Memory_Error;

          x += top[0];
          y += top[1];
          add_point( builder, x, y, 0 );

          x += top[2];
          y += top[3];
          add_point( builder, x, y, 0 );

          x += top[4];
          y += top[5];
          add_point( builder, x, y, 1 );
          break;

        case op_vhcurveto:
          FT_TRACE4(( " vhcurveto" ));

          if ( start_point( builder, x, y ) ||
               check_points( builder, 3 )   )
            goto Memory_Error;

          y += top[0];
          add_point( builder, x, y, 0 );
          x += top[1];
          y += top[2];
          add_point( builder, x, y, 0 );
          x += top[3];
          add_point( builder, x, y, 1 );
          break;

        case op_vlineto:
          FT_TRACE4(( " vlineto" ));

          if ( start_point( builder, x, y ) )
            goto Memory_Error;

          y += top[0];
          goto Add_Line;

        case op_vmoveto:
          FT_TRACE4(( " vmoveto" ));

          y += top[0];
          break;

        case op_div:
          FT_TRACE4(( " div" ));

          if ( top[1] )
          {
            *top = top[0] / top[1];
            ++top;
          }
          else
          {
            FT_ERROR(( "Z1_Parse_CharStrings: division by 0\n" ));
            goto Syntax_Error;
          }
          break;

        case op_callsubr:
          {
            FT_Int  index;


            FT_TRACE4(( " callsubr" ));

            index = top[0];
            if ( index < 0 || index >= num_subrs )
            {
              FT_ERROR(( "Z1_Parse_CharStrings: invalid subrs index\n" ));
              goto Syntax_Error;
            }

            if ( zone - decoder->zones >= T1_MAX_SUBRS_CALLS )
            {
              FT_ERROR(( "Z1_Parse_CharStrings: too many nested subrs\n" ));
              goto Syntax_Error;
            }

            zone->cursor = ip;  /* save current instruction pointer */

            zone++;
            zone->base   = subrs_base[index];
            zone->limit  = zone->base + subrs_len[index];
            zone->cursor = zone->base;

            if ( !zone->base )
            {
              FT_ERROR(( "Z1_Parse_CharStrings: invoking empty subrs!\n" ));
              goto Syntax_Error;
            }

            decoder->zone = zone;
            ip            = zone->base;
            limit         = zone->limit;
            break;
          }

        case op_pop:
          FT_TRACE4(( " pop" ));

          /* theorically, the arguments are already on the stack */
          top++;
          break;

        case op_return:
          FT_TRACE4(( " return" ));

          if ( zone <= decoder->zones )
          {
            FT_ERROR(( "Z1_Parse_CharStrings: unexpected return\n" ));
            goto Syntax_Error;
          }

          zone--;
          ip            = zone->cursor;
          limit         = zone->limit;
          decoder->zone = zone;
          break;

        case op_dotsection:
          FT_TRACE4(( " dotsection" ));

          break;

        case op_hstem:
          FT_TRACE4(( " hstem" ));

          break;

        case op_hstem3:
          FT_TRACE4(( " hstem3" ));

          break;

        case op_vstem:
          FT_TRACE4(( " vstem" ));

          break;

        case op_vstem3:
          FT_TRACE4(( " vstem3" ));

          break;

        case op_setcurrentpoint:
          FT_TRACE4(( " setcurrentpoint" ));

          FT_ERROR(( "Z1_Parse_CharStrings:" ));
          FT_ERROR(( " unexpected `setcurrentpoint'\n" ));
          goto Syntax_Error;

        default:
          FT_ERROR(( "Z1_Parse_CharStrings: unhandled opcode %d\n", op ));
          goto Syntax_Error;
        }

        decoder->top = top;

      } /* general operator processing */

    } /* while ip < limit */

    FT_TRACE4(( "..end..\n\n" ));
    return error;

  Syntax_Error:
    return T1_Err_Syntax_Error;

  Stack_Underflow:
    return T1_Err_Stack_Underflow;

  Memory_Error:
    return builder->error;
  }


  /*************************************************************************/
  /*************************************************************************/
  /*************************************************************************/
  /**********                                                      *********/
  /**********            COMPUTE THE MAXIMUM ADVANCE WIDTH         *********/
  /**********                                                      *********/
  /**********    The following code is in charge of computing      *********/
  /**********    the maximum advance width of the font.  It        *********/
  /**********    quickly processes each glyph charstring to        *********/
  /**********    extract the value from either a `sbw' or `seac'   *********/
  /**********    operator.                                         *********/
  /**********                                                      *********/
  /*************************************************************************/
  /*************************************************************************/
  /*************************************************************************/


  LOCAL_FUNC
  FT_Error  Z1_Compute_Max_Advance( T1_Face  face,
                                    FT_Int*  max_advance )
  {
    FT_Error    error;
    Z1_Decoder  decoder;
    FT_Int      glyph_index;
    T1_Font*    type1 = &face->type1;


    *max_advance = 0;

    /* Initialize load decoder */
    Z1_Init_Decoder( &decoder );
    Z1_Init_Builder( &decoder.builder, face, 0, 0 );

    decoder.blend                = face->blend;
    decoder.builder.metrics_only = 1;
    decoder.builder.load_points  = 0;

    /* for each glyph, parse the glyph charstring and extract */
    /* the advance width                                      */
    for ( glyph_index = 0; glyph_index < type1->num_glyphs; glyph_index++ )
    {
      /* now get load the unscaled outline */
      error = Z1_Parse_CharStrings( &decoder,
                                    type1->charstrings    [glyph_index],
                                    type1->charstrings_len[glyph_index],
                                    type1->num_subrs,
                                    type1->subrs,
                                    type1->subrs_len );
      /* ignore the error if one occured - skip to next glyph */
    }

    *max_advance = decoder.builder.advance.x;
    return T1_Err_Ok;
  }


  /*************************************************************************/
  /*************************************************************************/
  /*************************************************************************/
  /**********                                                      *********/
  /**********               UNHINTED GLYPH LOADER                  *********/
  /**********                                                      *********/
  /**********    The following code is in charge of loading a      *********/
  /**********    single outline.  It completely ignores hinting    *********/
  /**********    and is used when FT_LOAD_NO_HINTING is set.       *********/
  /**********                                                      *********/
  /**********      The Type 1 hinter is located in `t1hint.c'      *********/
  /**********                                                      *********/
  /*************************************************************************/
  /*************************************************************************/
  /*************************************************************************/


  LOCAL_FUNC
  FT_Error  Z1_Load_Glyph( Z1_GlyphSlot  glyph,
                           Z1_Size       size,
                           FT_Int        glyph_index,
                           FT_Int        load_flags )
  {
    FT_Error    error;
    Z1_Decoder  decoder;
    T1_Face     face = (T1_Face)glyph->root.face;
    FT_Bool     hinting;
    T1_Font*    type1 = &face->type1;


    if ( load_flags & FT_LOAD_NO_RECURSE )
      load_flags |= FT_LOAD_NO_SCALE | FT_LOAD_NO_HINTING;

    glyph->x_scale = size->root.metrics.x_scale;
    glyph->y_scale = size->root.metrics.y_scale;

    glyph->root.outline.n_points   = 0;
    glyph->root.outline.n_contours = 0;

    hinting = ( load_flags & FT_LOAD_NO_SCALE   ) == 0 &&
              ( load_flags & FT_LOAD_NO_HINTING ) == 0;

    glyph->root.format = ft_glyph_format_outline;

    Z1_Init_Decoder( &decoder );
    Z1_Init_Builder( &decoder.builder, face, size, glyph );

    decoder.blend              = ((T1_Face)glyph->root.face)->blend;
    decoder.builder.no_recurse = ( load_flags & FT_LOAD_NO_RECURSE ) != 0;

    /* now load the unscaled outline */
    error = Z1_Parse_CharStrings( &decoder,
                                  type1->charstrings    [glyph_index],
                                  type1->charstrings_len[glyph_index],
                                  type1->num_subrs,
                                  type1->subrs,
                                  type1->subrs_len );

    /* save new glyph tables */
    Z1_Done_Builder( &decoder.builder );

    /* now, set the metrics -- this is rather simple, as   */
    /* the left side bearing is the xMin, and the top side */
    /* bearing the yMax                                    */
    if ( !error )
    {
      glyph->root.outline.flags &= ft_outline_owner;
      glyph->root.outline.flags |= ft_outline_reverse_fill;
      
      /* for composite glyphs, return only left side bearing and */
      /* advance width                                           */
      if ( load_flags & FT_LOAD_NO_RECURSE )
      {
        glyph->root.metrics.horiBearingX = decoder.builder.left_bearing.x;
        glyph->root.metrics.horiAdvance  = decoder.builder.advance.x;
      }
      else
      {
        FT_BBox           cbox;
        FT_Glyph_Metrics* metrics = &glyph->root.metrics;


        /* copy the _unscaled_ advance width */
        metrics->horiAdvance = decoder.builder.advance.x;

        /* make up vertical metrics */
        metrics->vertBearingX = 0;
        metrics->vertBearingY = 0;
        metrics->vertAdvance  = 0;

        glyph->root.format = ft_glyph_format_outline;

        if ( size && size->root.metrics.y_ppem < 24 )
          glyph->root.outline.flags |= ft_outline_high_precision;

#if 0
        glyph->root.outline.second_pass    = TRUE;
        glyph->root.outline.high_precision = size->root.metrics.y_ppem < 24;
        glyph->root.outline.dropout_mode   = 2;
#endif /* 0 */

        if ( ( load_flags & FT_LOAD_NO_SCALE ) == 0 )
        {
          /* scale the outline and the metrics */
          FT_Int       n;
          FT_Outline*  cur = decoder.builder.base;
          FT_Vector*   vec = cur->points;
          FT_Fixed     x_scale = glyph->x_scale;
          FT_Fixed     y_scale = glyph->y_scale;


          /* First of all, scale the points */
          for ( n = cur->n_points; n > 0; n--, vec++ )
          {
            vec->x = FT_MulFix( vec->x, x_scale );
            vec->y = FT_MulFix( vec->y, y_scale );
          }

          FT_Outline_Get_CBox( &glyph->root.outline, &cbox );

          /* Then scale the metrics */
          metrics->horiAdvance  = FT_MulFix( metrics->horiAdvance,  x_scale );
          metrics->vertAdvance  = FT_MulFix( metrics->vertAdvance,  y_scale );

          metrics->vertBearingX = FT_MulFix( metrics->vertBearingX, x_scale );
          metrics->vertBearingY = FT_MulFix( metrics->vertBearingY, y_scale );
        }

        /* apply the font matrix */
        FT_Outline_Transform( &glyph->root.outline,
                              &face->type1.font_matrix );

        /* compute the other metrics */
        FT_Outline_Get_CBox( &glyph->root.outline, &cbox );

        /* grid fit the bounding box if necessary */
        if ( hinting )
        {
          cbox.xMin &= -64;
          cbox.yMin &= -64;
          cbox.xMax  = ( cbox.xMax+63 ) & -64;
          cbox.yMax  = ( cbox.yMax+63 ) & -64;
        }

        metrics->width  = cbox.xMax - cbox.xMin;
        metrics->height = cbox.yMax - cbox.yMin;

        metrics->horiBearingX = cbox.xMin;
        metrics->horiBearingY = cbox.yMax;
      }
    }
    return error;
  }


/* END */
