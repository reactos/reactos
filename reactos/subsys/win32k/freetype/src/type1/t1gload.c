/***************************************************************************/
/*                                                                         */
/*  t1gload.c                                                              */
/*                                                                         */
/*    Type 1 Glyph Loader (body).                                          */
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

#include "t1gload.h"

#ifndef T1_CONFIG_OPTION_DISABLE_HINTER
#include "t1hinter.h"
#endif

#else /* FT_FLAT_COMPILE */

#include <type1/t1gload.h>

#ifndef T1_CONFIG_OPTION_DISABLE_HINTER
#include <type1/t1hinter.h>
#endif

#endif /* FT_FLAT_COMPILE */


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
#define FT_COMPONENT  trace_t1gload


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


  static
  void  T1_Reset_Builder( T1_Builder*  builder,
                          FT_Bool      reset_base )
  {
    builder->pos_x = 0;
    builder->pos_y = 0;

    builder->left_bearing.x = 0;
    builder->left_bearing.y = 0;
    builder->advance.x      = 0;
    builder->advance.y      = 0;

    builder->pass       = 0;
    builder->hint_point = 0;

    if ( builder->loader )
    {
      if ( reset_base )
        FT_GlyphLoader_Rewind( builder->loader );

      FT_GlyphLoader_Prepare( builder->loader );
    }
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    T1_Init_Builder                                                    */
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
  /*    funcs   :: Glyph builder functions (or `methods').                 */
  /*                                                                       */
  LOCAL_FUNC
  void  T1_Init_Builder( T1_Builder*              builder,
                         T1_Face                  face,
                         T1_Size                  size,
                         T1_GlyphSlot             glyph,
                         const T1_Builder_Funcs*  funcs )
  {
    builder->funcs       = *funcs;
    builder->path_begun  = 0;
    builder->load_points = 1;

    builder->face   = face;
    builder->size   = size;
    builder->glyph  = glyph;
    builder->memory = face->root.memory;

    if ( glyph )
    {
      FT_GlyphLoader*  loader = FT_SLOT( glyph )->loader;


      builder->loader  = loader;
      builder->base    = &loader->base.outline;
      builder->current = &loader->current.outline;
    }

    if ( size )
    {
      builder->scale_x = size->root.metrics.x_scale;
      builder->scale_y = size->root.metrics.y_scale;
    }

    T1_Reset_Builder( builder, 1 );
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    T1_Done_Builder                                                    */
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
  void  T1_Done_Builder( T1_Builder*  builder )
  {
    T1_GlyphSlot  glyph = builder->glyph;


    if ( glyph )
      glyph->root.outline = *builder->base;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    T1_Init_Decoder                                                    */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Initializes a given glyph decoder.                                 */
  /*                                                                       */
  /* <InOut>                                                               */
  /*    decoder :: A pointer to the glyph builder to initialize.           */
  /*                                                                       */
  /* <Input>                                                               */
  /*    funcs   :: The hinting functions interface.                        */
  /*                                                                       */
  LOCAL_FUNC
  void  T1_Init_Decoder( T1_Decoder*             decoder,
                         const T1_Hinter_Funcs*  funcs )
  {
    decoder->hinter = *funcs;    /* copy hinter interface */
    decoder->top    = 0;
    decoder->zone   = 0;

    decoder->flex_state       = 0;
    decoder->num_flex_vectors = 0;

    /* Clear loader */
    MEM_Set( &decoder->builder, 0, sizeof ( decoder->builder ) );
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
                   psnames->adobe_std_encoding[charcode] );

    for ( n = 0; n < face->type1.num_glyphs; n++ )
    {
      FT_String*  name = (FT_String*)face->type1.glyph_names[n];


      if ( name && strcmp( name, glyph_name ) == 0 )
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
  FT_Error  t1operator_seac( T1_Decoder*  decoder,
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
      FT_GlyphSlot     glyph = (FT_GlyphSlot)decoder->builder.glyph;
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
      glyph->subglyphs     = loader->current.subglyphs;
      glyph->format        = ft_glyph_format_composite;

      loader->current.num_subglyphs = 2;
      goto Exit;
    }

    /* First load `bchar' in builder */
    /* now load the unscaled outline */

    if ( decoder->builder.loader )
      FT_GlyphLoader_Prepare( decoder->builder.loader );  /* prepare loader */

    error = T1_Parse_CharStrings( decoder,
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

    /* Now load `achar' on top of the base outline */
    error = T1_Parse_CharStrings( decoder,
                                  type1->charstrings    [achar_index],
                                  type1->charstrings_len[achar_index],
                                  type1->num_subrs,
                                  type1->subrs,
                                  type1->subrs_len );
    if ( error )
      return error;

    /* restore the left side bearing and */
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


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    t1operator_flex                                                    */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Implements the `flex' Type 1 operator for a Type 1 decoder.        */
  /*                                                                       */
  /* <Input>                                                               */
  /*    decoder   :: The current Type 1 decoder.                           */
  /*    threshold :: The threshold.                                        */
  /*    end_x     :: The horizontal position of the final flex point.      */
  /*    end_y     :: The vertical position of the final flex point.        */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  static
  FT_Error  t1operator_flex( T1_Decoder*  decoder,
                             FT_Pos       threshold,
                             FT_Pos       end_x,
                             FT_Pos       end_y )
  {
    FT_Vector    vec;
    FT_Vector*   flex  = decoder->flex_vectors;
    FT_Int       n;

    FT_UNUSED( threshold );
    FT_UNUSED( end_x );
    FT_UNUSED( end_y );


    /* we don't even try to test the threshold in the non-hinting   */
    /* builder, even if the flex operator is said to be a path      */
    /* construction statement in the specification.  This is better */
    /* left to the hinter.                                          */

    flex = decoder->flex_vectors;
    vec  = *flex++;

    for ( n = 0; n < 6; n++ )
    {
      flex->x += vec.x;
      flex->y += vec.y;

      vec = *flex++;
    }

    flex  = decoder->flex_vectors;

    return  decoder->builder.funcs.rcurve_to( &decoder->builder,
                                              flex[0].x, flex[0].y,
                                              flex[1].x, flex[1].y,
                                              flex[2].x, flex[2].y ) ||

            decoder->builder.funcs.rcurve_to( &decoder->builder,
                                              flex[3].x, flex[3].y,
                                              flex[4].x, flex[4].y,
                                              flex[5].x, flex[5].y );
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    T1_Parse_CharStrings                                               */
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
  FT_Error  T1_Parse_CharStrings( T1_Decoder*  decoder,
                                  FT_Byte*     charstring_base,
                                  FT_Int       charstring_len,
                                  FT_Int       num_subrs,
                                  FT_Byte**    subrs_base,
                                  FT_Int*      subrs_len )
  {
    FT_Error           error;
    T1_Decoder_Zone*   zone;
    FT_Byte*           ip;
    FT_Byte*           limit;
    T1_Builder*        builder = &decoder->builder;
    T1_Builder_Funcs*  builds  = &builder->funcs;
    T1_Hinter_Funcs*   hints   = &decoder->hinter;

    static
    const FT_Int  args_count[op_max] =
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


    /* First of all, initialize the decoder */
    decoder->top  = decoder->stack;
    decoder->zone = decoder->zones;
    zone          = decoder->zones;

    builder->path_begun  = 0;

    zone->base           = charstring_base;
    limit = zone->limit  = charstring_base + charstring_len;
    ip    = zone->cursor = zone->base;

    error = T1_Err_Ok;

    /* now, execute loop */
    while ( ip < limit )
    {
      FT_Int*      top   = decoder->top;
      T1_Operator  op    = op_none;
      FT_Long      value = 0;


      /* Start with the decompression of operator or value */
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
          FT_ERROR(( "T1_Parse_CharStrings: invalid escape (12+EOF)\n" ));
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
          FT_ERROR(( "T1_Parse_CharStrings: invalid escape (12+%d)\n",
                     ip[-1] ));
          goto Syntax_Error;
        }
        break;

      case 255:    /* four bytes integer */
        if ( ip + 4 > limit )
        {
          FT_ERROR(( "T1_Parse_CharStrings: unexpected EOF in integer\n" ));
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
              FT_ERROR(( "T1_Parse_CharStrings:" ));
              FT_ERROR(( " unexpected EOF in integer\n" ));
              goto Syntax_Error;
            }

            if ( ip[-2] < 251 )
              value = ((FT_Long)( ip[-2] - 247 ) << 8 ) + ip[-1] + 108;
            else
              value = -( ( ( (FT_Long)ip[-2] - 251 ) << 8 ) + ip[-1] + 108 );
          }
        }
        else
        {
          FT_ERROR(( "T1_Parse_CharStrings: invalid byte (%d)\n",
                     ip[-1] ));
          goto Syntax_Error;
        }
      }

      /* push value if necessary */
      if ( op == op_none )
      {
        if ( top - decoder->stack >= T1_MAX_CHARSTRINGS_OPERANDS )
        {
          FT_ERROR(( "T1_Parse_CharStrings: stack overflow!\n" ));
          goto Syntax_Error;
        }

        *top++       = value;
        decoder->top = top;
      }

      else if ( op == op_callothersubr ) /* check arguments differently */
      {
        if ( top - decoder->stack < 2 )
          goto Stack_Underflow;

        top -= 2;

        switch ( top[1] )
        {
        case 1:   /* start flex feature */
          if ( top[0] != 0 )
            goto Unexpected_OtherSubr;

          decoder->flex_state        = 1;
          decoder->num_flex_vectors  = 0;
          decoder->flex_vectors[0].x = 0;
          decoder->flex_vectors[0].y = 0;
          break;

        case 2:   /* add flex vector */
          {
            FT_Int      index;
            FT_Vector*  flex;


            if ( top[0] != 0 )
              goto Unexpected_OtherSubr;

            top -= 2;
            if ( top < decoder->stack )
              goto Stack_Underflow;

            index = decoder->num_flex_vectors++;
            if ( index >= 7 )
            {
              FT_ERROR(( "T1_Parse_CharStrings: too many flex vectors!\n" ));
              goto Syntax_Error;
            }

            flex = decoder->flex_vectors + index;
            flex->x += top[0];
            flex->y += top[1];
          }
          break;

        case 0:   /* end flex feature */
          if ( decoder->flex_state       == 0 ||
               decoder->num_flex_vectors != 7 )
          {
            FT_ERROR(( "T1_Parse_CharStrings: unexpected flex end\n" ));
            goto Syntax_Error;
          }

          if ( top[0] != 3 )
            goto Unexpected_OtherSubr;

          top -= 3;
          if ( top < decoder->stack )
            goto Stack_Underflow;

          /* now consume the remaining `pop pop setcurrentpoint' */
          if ( ip + 6 > limit              ||
               ip[0] != 12  || ip[1] != 17 ||   /* pop */
               ip[2] != 12  || ip[3] != 17 ||   /* pop */
               ip[4] != 12  || ip[5] != 33 )    /* setcurrentpoint */
          {
            FT_ERROR(( "T1_Parse_CharStrings: invalid flex charstring\n" ));
            goto Syntax_Error;
          }

          decoder->flex_state = 0;
          decoder->top        = top;

          error = t1operator_flex( decoder, top[0], top[1], top[2] );
          break;

        case 3:  /* change hints */
          if ( top[0] != 1 )
            goto Unexpected_OtherSubr;

          /* eat the following `pop' */
          if ( ip + 2 > limit )
          {
            FT_ERROR(( "T1_Parse_CharStrings: invalid escape (12+%d)\n",
                       ip[-1] ));
            goto Syntax_Error;
          }

          if ( ip[0] != 12 || ip[1] != 17 )
          {
            FT_ERROR(( "T1_Parse_CharStrings:" ));
            FT_ERROR(( " `pop' expected, found (%d %d)\n", ip[0], ip[1] ));
            goto Syntax_Error;
          }

          ip += 2;

          error  = hints->change_hints( builder );
          break;

        default:
          /* invalid OtherSubrs call */
        Unexpected_OtherSubr:
          FT_ERROR(( "T1_Parse_CharStrings: unexpected OtherSubrs [%d %d]\n",
                     top[0], top[1] ));
          goto Syntax_Error;
        }
        decoder->top = top;
      }
      else
      {
        FT_Int  num_args = args_count[op];


        if ( top - decoder->stack < num_args )
          goto Stack_Underflow;

        top -= num_args;

        switch ( op )
        {
        case op_endchar:
          error = builds->end_char( builder );
          break;

        case op_hsbw:
          error = builds->set_bearing_point( builder, top[0], 0,
                                                      top[1], 0 );
          break;

        case op_seac:
          /* return immediately after the processing */
          return t1operator_seac( decoder, top[0], top[1],
                                           top[2], top[3], top[4] );

        case op_sbw:
          error = builds->set_bearing_point( builder, top[0], top[1],
                                                      top[2], top[3] );
          break;

        case op_closepath:
          error = builds->close_path( builder );
          break;

        case op_hlineto:
          error = builds->rline_to( builder, top[0], 0 );
          break;

        case op_hmoveto:
          error = builds->rmove_to( builder, top[0], 0 );
          break;

        case op_hvcurveto:
          error = builds->rcurve_to( builder, top[0], 0,
                                              top[1], top[2],
                                              0,      top[3] );
          break;

        case op_rlineto:
          error = builds->rline_to( builder, top[0], top[1] );
          break;

        case op_rmoveto:
          /* ignore operator when in flex mode */
          if ( decoder->flex_state == 0 )
            error = builds->rmove_to( builder, top[0], top[1] );
          else
            top += 2;
          break;

        case op_rrcurveto:
          error = builds->rcurve_to( builder, top[0], top[1],
                                              top[2], top[3],
                                              top[4], top[5] );
          break;

        case op_vhcurveto:
          error = builds->rcurve_to( builder,      0, top[0],
                                              top[1], top[2],
                                              top[3],      0 );
          break;

        case op_vlineto:
          error = builds->rline_to( builder, 0, top[0] );
          break;

        case op_vmoveto:
          error = builds->rmove_to( builder, 0, top[0] );
          break;

        case op_dotsection:
          error = hints->dot_section( builder );
          break;

        case op_hstem:
          error = hints->stem( builder, top[0], top[1], 0 );
          break;

        case op_hstem3:
          error = hints->stem3( builder, top[0], top[1], top[2],
                                         top[3], top[4], top[5], 0 );
          break;

        case op_vstem:
          error = hints->stem( builder, top[0], top[1], 1 );
          break;

        case op_vstem3:
          error = hints->stem3( builder, top[0], top[1], top[2],
                                         top[3], top[4], top[5], 1 );
          break;

        case op_div:
          if ( top[1] )
          {
            *top = top[0] / top[1];
            ++top;
          }
          else
          {
            FT_ERROR(( "T1_Parse_CHarStrings: division by 0\n" ));
            goto Syntax_Error;
          }
          break;

        case op_callsubr:
          {
            FT_Int  index = top[0];


            if ( index < 0 || index >= num_subrs )
            {
              FT_ERROR(( "T1_Parse_CharStrings: invalid subrs index\n" ));
              goto Syntax_Error;
            }

            if ( zone - decoder->zones >= T1_MAX_SUBRS_CALLS )
            {
              FT_ERROR(( "T1_Parse_CharStrings: too many nested subrs\n" ));
              goto Syntax_Error;
            }

            zone->cursor = ip;  /* save current instruction pointer */

            zone++;
            zone->base   = subrs_base[index];
            zone->limit  = zone->base + subrs_len[index];
            zone->cursor = zone->base;

            if ( !zone->base )
            {
              FT_ERROR(( "T1_Parse_CharStrings: invoking empty subrs!\n" ));
              goto Syntax_Error;
            }

            decoder->zone = zone;
            ip            = zone->base;
            limit         = zone->limit;
          }
          break;

        case op_pop:
          FT_ERROR(( "T1_Parse_CharStrings: unexpected POP\n" ));
          goto Syntax_Error;

        case op_return:
          if ( zone <= decoder->zones )
          {
            FT_ERROR(( "T1_Parse_CharStrings: unexpected return\n" ));
            goto Syntax_Error;
          }

          zone--;
          ip            = zone->cursor;
          limit         = zone->limit;
          decoder->zone = zone;
          break;

        case op_setcurrentpoint:
          FT_ERROR(( "T1_Parse_CharStrings:" ));
          FT_ERROR(( " unexpected `setcurrentpoint'\n" ));
          goto Syntax_Error;
          break;

        default:
          FT_ERROR(( "T1_Parse_CharStrings: unhandled opcode %d\n", op ));
          goto Syntax_Error;
        }

        decoder->top = top;
      }
    }

    return error;

  Syntax_Error:
    return T1_Err_Syntax_Error;

  Stack_Underflow:
    return T1_Err_Stack_Underflow;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    T1_Add_Points                                                      */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Checks that there is enough room in the current load glyph outline */
  /*    to accept `num_points' additional outline points.  If not, this    */
  /*    function grows the load outline's arrays accordingly.              */
  /*                                                                       */
  /* <Input>                                                               */
  /*    builder    :: A pointer to the glyph builder object.               */
  /*                                                                       */
  /*    num_points :: The number of points that will be added later.       */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  /* <Note>                                                                */
  /*    This function does NOT update the points count in the glyph        */
  /*    builder.  This must be done by the caller itself, after this       */
  /*    function has been invoked.                                         */
  /*                                                                       */
  LOCAL_FUNC
  FT_Error  T1_Add_Points( T1_Builder*  builder,
                           FT_Int       num_points )
  {
    return FT_GlyphLoader_Check_Points( builder->loader, num_points, 0 );
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    T1_Add_Contours                                                    */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Checks that there is enough room in the current load glyph outline */
  /*    to accept `num_contours' additional contours.  If not, this        */
  /*    function grows the load outline's arrays accordingly.              */
  /*                                                                       */
  /* <Input>                                                               */
  /*    builder      :: A pointer to the glyph builder object.             */
  /*                                                                       */
  /*    num_contours :: The number of contours that will be added later.   */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  /* <Note>                                                                */
  /*    This function does NOT update the contours count in the load glyph */
  /*    This must be done by the caller itself, after this function is     */
  /*    invoked.                                                           */
  /*                                                                       */
  LOCAL_FUNC
  FT_Error  T1_Add_Contours( T1_Builder*  builder,
                             FT_Int       num_contours )
  {
    return FT_GlyphLoader_Check_Points( builder->loader, 0, num_contours );
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


  static
  FT_Error  maxadv_sbw( T1_Decoder*  decoder,
                        FT_Pos       sbx,
                        FT_Pos       sby,
                        FT_Pos       wx,
                        FT_Pos       wy )
  {
    FT_UNUSED( sbx );
    FT_UNUSED( sby );
    FT_UNUSED( wy );

    if ( wx > decoder->builder.advance.x )
      decoder->builder.advance.x = wx;

    return -1;    /* return an error code to exit the Type 1 parser */
                  /* immediately.                                   */
  }


  static
  FT_Int  maxadv_error( void )
  {
    /* we should never reach this code, unless with a buggy font */
    return -2;
  }


  /* the maxadv_gbuilder_interface is used when computing the maximum  */
  /* advance width of all glyphs in a given font.  We only process the */
  /* `sbw' operator here, and return an error for all others.          */

  /* Note that `seac' is processed by the T1_Decoder.                  */
  static
  const T1_Builder_Funcs  maxadv_builder_interface =
  {
    (T1_Builder_EndChar)  maxadv_error,
    (T1_Builder_Sbw)      maxadv_sbw,
    (T1_Builder_ClosePath)maxadv_error,
    (T1_Builder_RLineTo)  maxadv_error,
    (T1_Builder_RMoveTo)  maxadv_error,
    (T1_Builder_RCurveTo) maxadv_error
  };


  /* the maxadv_hinter_interface always return an error. */
  static
  const T1_Hinter_Funcs  maxadv_hinter_interface =
  {
    (T1_Hinter_DotSection) maxadv_error,
    (T1_Hinter_ChangeHints)maxadv_error,
    (T1_Hinter_Stem)       maxadv_error,
    (T1_Hinter_Stem3)      maxadv_error,
  };


  LOCAL_FUNC
  FT_Error  T1_Compute_Max_Advance( T1_Face  face,
                                    FT_Int*  max_advance )
  {
    FT_Error    error;
    T1_Decoder  decoder;
    FT_Int      glyph_index;
    T1_Font*    type1 = &face->type1;


    *max_advance = 0;

    /* Initialize load decoder */
    T1_Init_Decoder( &decoder, &maxadv_hinter_interface );

    T1_Init_Builder( &decoder.builder, face, 0, 0,
                     &maxadv_builder_interface );

    /* For each glyph, parse the glyph charstring and extract */
    /* the advance width.                                     */
    for ( glyph_index = 0; glyph_index < type1->num_glyphs; glyph_index++ )
    {
      /* now get load the unscaled outline */
      error = T1_Parse_CharStrings( &decoder,
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


  static
  FT_Error  close_open_path( T1_Builder*  builder )
  {
    FT_Error     error;
    FT_Outline*  cur = builder->current;
    FT_Int       num_points;
    FT_Int       first_point;


    /* Some fonts, like Hershey, are made of `open paths' which are     */
    /* now managed directly by FreeType.  In this case, it is necessary */
    /* to close the path by duplicating its points in reverse order,    */
    /* which is precisely the purpose of this function.                 */

    /* first compute the number of points to duplicate */
    if ( cur->n_contours > 1 )
      first_point = cur->contours[cur->n_contours - 2] + 1;
    else
      first_point = 0;

    num_points = cur->n_points - first_point - 2;
    if ( num_points > 0 )
    {
      FT_Vector*  source_point;
      char*       source_tags;
      FT_Vector*  point;
      char*       tags;


      error = T1_Add_Points( builder, num_points );
      if ( error )
        return error;

      point = cur->points + cur->n_points;
      tags  = cur->tags   + cur->n_points;

      source_point = point - 2;
      source_tags  = tags - 2;

      cur->n_points += num_points;

      if ( builder->load_points )
        do
        {
          *point++ = *source_point--;
          *tags++  = *source_tags--;
          num_points--;

        } while ( num_points > 0 );
    }

    builder->path_begun = 0;
    return T1_Err_Ok;
  }


  static
  FT_Error  gload_closepath( T1_Builder*  builder )
  {
    FT_Outline*  cur = builder->current;


    /* XXXX: We must not include the last point in the path if it */
    /*       is located on the first point.                       */
    if ( cur->n_points > 1 )
    {
      FT_Int      first = 0;
      FT_Vector*  p1    = cur->points + first;
      FT_Vector*  p2    = cur->points + cur->n_points - 1;


      if ( cur->n_contours > 1 )
      {
        first = cur->contours[cur->n_contours - 2] + 1;
        p1    = cur->points + first;
      }

      if ( p1->x == p2->x && p1->y == p2->y )
        cur->n_points--;
    }

    /* save current contour, if any */
    if ( cur->n_contours > 0 )
      cur->contours[cur->n_contours - 1] = cur->n_points - 1;

#ifndef T1_CONFIG_OPTION_DISABLE_HINTER
    /* hint last points if necessary -- this is not strictly required    */
    /* there, but it helps for debugging, and doesn't affect performance */
    if ( builder->pass == 1 )
      T1_Hint_Points( builder );
#endif

    builder->path_begun = 0;
    return T1_Err_Ok;
  }


  static
  FT_Error  gload_endchar( T1_Builder*  builder )
  {
    FT_Error  error;


    /* close path if needed */
    if ( builder->path_begun )
    {
      error = close_open_path( builder );
      if ( error )
        return error;
    }

    error = gload_closepath( builder );

    FT_GlyphLoader_Add( builder->loader );

    return error;
  }


  static
  FT_Error  gload_sbw( T1_Builder*  builder,
                       FT_Pos       sbx,
                       FT_Pos       sby,
                       FT_Pos       wx,
                       FT_Pos       wy )
  {
    builder->left_bearing.x += sbx;
    builder->left_bearing.y += sby;
    builder->advance.x       = wx;
    builder->advance.y       = wy;

    builder->last.x = sbx;
    builder->last.y = sby;

    return 0;
  }


  static
  FT_Error  gload_rlineto( T1_Builder*  builder,
                           FT_Pos       dx,
                           FT_Pos       dy )
  {
    FT_Error     error;
    FT_Outline*  cur = builder->current;
    FT_Vector    vec;


    /* grow buffer if necessary */
    error = T1_Add_Points( builder, 1 );
    if ( error )
      return error;

    if ( builder->load_points )
    {
      /* save point */
      vec.x = builder->last.x + dx;
      vec.y = builder->last.y + dy;

      cur->points[cur->n_points] = vec;
      cur->tags  [cur->n_points] = FT_Curve_Tag_On;

      builder->last = vec;
    }
    cur->n_points++;

    builder->path_begun = 1;

    return T1_Err_Ok;
  }


  static
  FT_Error  gload_rmoveto( T1_Builder*  builder,
                           FT_Pos       dx,
                           FT_Pos       dy )
  {
    FT_Error     error;
    FT_Outline*  cur = builder->current;
    FT_Vector    vec;


    /* in the case where `path_begun' is set, we have an `rmoveto' */
    /* after some normal path definition. If the face's paint type */
    /* is set to 1, this means that we have an `open path', also   */
    /* called a `stroke'.  The FreeType raster doesn't support     */
    /* opened paths, so we'll close it explicitely there.          */

    if ( builder->path_begun && builder->face->type1.paint_type == 1 )
    {
      if ( builder->face->type1.paint_type == 1 )
      {
        error = close_open_path( builder );
        if ( error )
          return error;
      }
    }

    /* grow buffer if necessary */
    error = T1_Add_Contours( builder, 1 ) ||
            T1_Add_Points  ( builder, 1 );
    if ( error )
      return error;

    /* save current contour, if any */
    if ( cur->n_contours > 0 )
      cur->contours[cur->n_contours - 1] = cur->n_points - 1;

    if ( builder->load_points )
    {
      /* save point */
      vec.x = builder->last.x + dx;
      vec.y = builder->last.y + dy;
      cur->points[cur->n_points] = vec;
      cur->tags  [cur->n_points] = FT_Curve_Tag_On;

      builder->last = vec;
    }

    cur->n_contours++;
    cur->n_points++;

    return T1_Err_Ok;
  }


  static
  FT_Error  gload_rrcurveto( T1_Builder*  builder,
                             FT_Pos       dx1,
                             FT_Pos       dy1,
                             FT_Pos       dx2,
                             FT_Pos       dy2,
                             FT_Pos       dx3,
                             FT_Pos       dy3 )
  {
    FT_Error     error;
    FT_Outline*  cur = builder->current;
    FT_Vector    vec;
    FT_Vector*   points;
    char*        tags;


    /* grow buffer if necessary */
    error = T1_Add_Points( builder, 3 );
    if ( error )
      return error;

    if ( builder->load_points )
    {
      /* save point */
      points = cur->points + cur->n_points;
      tags   = cur->tags  + cur->n_points;

      vec.x = builder->last.x + dx1;
      vec.y = builder->last.y + dy1;
      points[0] = vec;
      tags[0]   = FT_Curve_Tag_Cubic;

      vec.x += dx2;
      vec.y += dy2;
      points[1] = vec;
      tags[1]   = FT_Curve_Tag_Cubic;

      vec.x += dx3;
      vec.y += dy3;
      points[2] = vec;
      tags[2]   = FT_Curve_Tag_On;

      builder->last = vec;
    }

    cur->n_points      += 3;
    builder->path_begun = 1;

    return T1_Err_Ok;
  }


  static
  FT_Error  gload_ignore( void )
  {
    return 0;
  }


  static
  const T1_Builder_Funcs  gload_builder_interface =
  {
    gload_endchar,
    gload_sbw,
    gload_closepath,
    gload_rlineto,
    gload_rmoveto,
    gload_rrcurveto
  };


  static
  const T1_Builder_Funcs  gload_builder_interface_null =
  {
    (T1_Builder_EndChar)  gload_ignore,
    (T1_Builder_Sbw)      gload_sbw,      /* record left bearing */
    (T1_Builder_ClosePath)gload_ignore,
    (T1_Builder_RLineTo)  gload_ignore,
    (T1_Builder_RMoveTo)  gload_ignore,
    (T1_Builder_RCurveTo) gload_ignore
  };


  static
  const T1_Hinter_Funcs   gload_hinter_interface =
  {
    (T1_Hinter_DotSection) gload_ignore,   /* dotsection         */
    (T1_Hinter_ChangeHints)gload_ignore,   /* changehints        */
    (T1_Hinter_Stem)       gload_ignore,   /* hstem & vstem      */
    (T1_Hinter_Stem3)      gload_ignore,   /* hstem3 & vestem3   */
  };


#ifndef T1_CONFIG_OPTION_DISABLE_HINTER


  /*************************************************************************/
  /*                                                                       */
  /*  Hinter overview:                                                     */
  /*                                                                       */
  /*    This is a two-pass hinter.  On the first pass, the hints are all   */
  /*    recorded by the hinter, and no point is loaded in the outline.     */
  /*                                                                       */
  /*    When the first pass is finished, all stems hints are grid-fitted   */
  /*    at once.                                                           */
  /*                                                                       */
  /*    Then, a second pass is performed to load the outline points as     */
  /*    well as hint/scale them correctly.                                 */
  /*                                                                       */
  /*************************************************************************/


  static
  FT_Error  t1_load_hinted_glyph( T1_Decoder*  decoder,
                                  FT_UInt      glyph_index,
                                  FT_Bool      recurse )
  {
    T1_Builder*      builder = &decoder->builder;
    T1_GlyphSlot     glyph   = builder->glyph;
    T1_Font*         type1   = &builder->face->type1;
    FT_UInt          old_points, old_contours;
    FT_GlyphLoader*  loader = decoder->builder.loader;
    FT_Error         error;


    /* Pass 1 -- try to load first glyph, simply recording points */
    old_points   = loader->base.outline.n_points;
    old_contours = loader->base.outline.n_contours;

    FT_GlyphLoader_Prepare( decoder->builder.loader );

    T1_Reset_Builder( builder, 0 );

    builder->no_recurse                = recurse;
    builder->pass                      = 0;
    glyph->hints->hori_stems.num_stems = 0;
    glyph->hints->vert_stems.num_stems = 0;

    error = T1_Parse_CharStrings( decoder,
                                  type1->charstrings    [glyph_index],
                                  type1->charstrings_len[glyph_index],
                                  type1->num_subrs,
                                  type1->subrs,
                                  type1->subrs_len );
    if ( error )
      goto Exit;

    /* check for composite (i.e. `seac' operator) */
    if ( glyph->root.format == ft_glyph_format_composite )
    {
      /* this is a composite glyph, we must then load the first one, */
      /* then load the second one on top of it and translate it by a */
      /* fixed amount.                                               */
      FT_UInt       n_base_points;
      FT_SubGlyph*  subglyph = loader->base.subglyphs;
      T1_Size       size = builder->size;
      FT_Pos        dx, dy;
      FT_Vector     left_bearing, advance;


      /* clean glyph format */
      glyph->root.format = ft_glyph_format_none;

      /* First load `bchar' in builder */
      builder->no_recurse = 0;
      error = t1_load_hinted_glyph( decoder, subglyph->index, 0 );
      if ( error )
        goto Exit;

      /* save the left bearing and width of the base character */
      /* as they will be erased by the next load               */
      left_bearing = builder->left_bearing;
      advance      = builder->advance;

      /* Then load `achar' in builder */
      n_base_points = builder->base->n_points;
      subglyph++;
      error = t1_load_hinted_glyph( decoder, subglyph->index, 0 );
      if ( error )
        goto Exit;

      /* Finally, move the accent */
      dx = FT_MulFix( subglyph->arg1, size->root.metrics.x_scale );
      dy = FT_MulFix( subglyph->arg2, size->root.metrics.y_scale );
      dx = ( dx + 32 ) & -64;
      dy = ( dy + 32 ) & -64;
      {
        FT_Outline  dummy;


        dummy.n_points = loader->base.outline.n_points - n_base_points;
        dummy.points   = loader->base.outline.points   + n_base_points;

        FT_Outline_Translate( &dummy, dx, dy );
      }

      /* restore the left side bearing and   */
      /* advance width of the base character */
      builder->left_bearing = left_bearing;
      builder->advance      = advance;
    }
    else
    {
      /* All right, pass 1 is finished, now grid-fit all stem hints */
      T1_Hint_Stems( &decoder->builder );

      /* undo the end-char */
      builder->base->n_points   = old_points;
      builder->base->n_contours = old_contours;

      /* Pass 2 -- record and scale/hint the points */
      T1_Reset_Builder( builder, 0 );

      builder->pass       = 1;
      builder->no_recurse = 0;

      error = T1_Parse_CharStrings( decoder,
                                    type1->charstrings    [glyph_index],
                                    type1->charstrings_len[glyph_index],
                                    type1->num_subrs,
                                    type1->subrs,
                                    type1->subrs_len );
    }

    /* save new glyph tables */
    if ( recurse )
      T1_Done_Builder( builder );

  Exit:
    return error;
  }


#endif /* !T1_CONFIG_OPTION_DISABLE_HINTER */


  LOCAL_FUNC
  FT_Error  T1_Load_Glyph( T1_GlyphSlot  glyph,
                           T1_Size       size,
                           FT_Int        glyph_index,
                           FT_Int        load_flags )
  {
    FT_Error    error;
    T1_Decoder  decoder;
    T1_Face     face = (T1_Face)glyph->root.face;
    FT_Bool     hinting;
    T1_Font*    type1 = &face->type1;


    if ( load_flags & FT_LOAD_NO_RECURSE )
      load_flags |= FT_LOAD_NO_SCALE | FT_LOAD_NO_HINTING;

    glyph->x_scale = size->root.metrics.x_scale;
    glyph->y_scale = size->root.metrics.y_scale;

    glyph->root.outline.n_points   = 0;
    glyph->root.outline.n_contours = 0;

    glyph->root.format = ft_glyph_format_outline;  /* by default */

    hinting = 0;

#ifndef T1_CONFIG_OPTION_DISABLE_HINTER

    hinting = ( load_flags & ( FT_LOAD_NO_SCALE | FT_LOAD_NO_HINTING ) ) == 0;

    if ( hinting )
    {
      T1_Init_Decoder( &decoder, &t1_hinter_funcs );
      T1_Init_Builder( &decoder.builder, face, size, glyph,
                       &gload_builder_interface );

      error = t1_load_hinted_glyph( &decoder, glyph_index, 1 );
    }
    else

#endif /* !T1_CONFIG_OPTION_DISABLE_HINTER */

    {
      T1_Init_Decoder( &decoder, &gload_hinter_interface );

      T1_Init_Builder( &decoder.builder, face, size, glyph,
                       &gload_builder_interface );

      decoder.builder.no_recurse = ( load_flags & FT_LOAD_NO_RECURSE ) != 0;

      /* now load the unscaled outline */
      error = T1_Parse_CharStrings( &decoder,
                                    type1->charstrings    [glyph_index],
                                    type1->charstrings_len[glyph_index],
                                    type1->num_subrs,
                                    type1->subrs,
                                    type1->subrs_len );

      /* save new glyph tables */
      T1_Done_Builder( &decoder.builder );
    }

    /* Now, set the metrics -- this is rather simple, as   */
    /* the left side bearing is the xMin, and the top side */
    /* bearing the yMax                                    */
    if ( !error )
    {
      /* for composite glyphs, return only the left side bearing and the */
      /* advance width                                                   */
      if ( glyph->root.format == ft_glyph_format_composite )
      {
        glyph->root.metrics.horiBearingX = decoder.builder.left_bearing.x;
        glyph->root.metrics.horiAdvance  = decoder.builder.advance.x;
      }
      else
      {
        FT_BBox           cbox;
        FT_Glyph_Metrics* metrics = &glyph->root.metrics;


        /* apply the font matrix */
        FT_Outline_Transform( &glyph->root.outline,
                              &face->type1.font_matrix );

        FT_Outline_Get_CBox( &glyph->root.outline, &cbox );

        /* grid fit the bounding box if necessary */
        if ( hinting )
        {
          cbox.xMin &= -64;
          cbox.yMin &= -64;
          cbox.xMax  = ( cbox.xMax + 63 ) & -64;
          cbox.yMax  = ( cbox.yMax + 63 ) & -64;
        }

        metrics->width  = cbox.xMax - cbox.xMin;
        metrics->height = cbox.yMax - cbox.yMin;

        metrics->horiBearingX = cbox.xMin;
        metrics->horiBearingY = cbox.yMax;

        /* copy the _unscaled_ advance width */
        metrics->horiAdvance = decoder.builder.advance.x;

        /* make up vertical metrics */
        metrics->vertBearingX = 0;
        metrics->vertBearingY = 0;
        metrics->vertAdvance  = 0;

        glyph->root.format = ft_glyph_format_outline;

        glyph->root.outline.flags = 0;

        if ( size->root.metrics.y_ppem < 24 )
          glyph->root.outline.flags |= ft_outline_high_precision;

        glyph->root.outline.flags |= ft_outline_reverse_fill;

        if ( hinting )
        {
          /* adjust the advance width               */
          /* XXX TODO: consider stem hints grid-fit */
          metrics->horiAdvance  = FT_MulFix( metrics->horiAdvance,
                                             glyph->x_scale );
        }
        else if ( ( load_flags & FT_LOAD_NO_SCALE ) == 0 )
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

          /* Then scale the metrics */
          metrics->width  = FT_MulFix( metrics->width,  x_scale );
          metrics->height = FT_MulFix( metrics->height, y_scale );

          metrics->horiBearingX = FT_MulFix( metrics->horiBearingX, x_scale );
          metrics->horiBearingY = FT_MulFix( metrics->horiBearingY, y_scale );

          metrics->vertBearingX = FT_MulFix( metrics->vertBearingX, x_scale );
          metrics->vertBearingY = FT_MulFix( metrics->vertBearingY, y_scale );

          metrics->horiAdvance  = FT_MulFix( metrics->horiAdvance,  x_scale );
          metrics->vertAdvance  = FT_MulFix( metrics->vertAdvance,  y_scale );
        }
      }
    }

    return error;
  }


/* END */
