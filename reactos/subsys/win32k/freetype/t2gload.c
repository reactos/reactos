/***************************************************************************/
/*                                                                         */
/*  t2gload.c                                                              */
/*                                                                         */
/*    OpenType Glyph Loader (body).                                        */
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


#include <freetype/internal/ftdebug.h>
#include <freetype/internal/ftcalc.h>
#include <freetype/internal/ftstream.h>
#include <freetype/internal/sfnt.h>
#include <freetype/ftoutln.h>
#include <freetype/tttags.h>


#ifdef FT_FLAT_COMPILE

#include "t2load.h"
#include "t2gload.h"

#else

#include <cff/t2load.h>
#include <cff/t2gload.h>

#endif


#include <freetype/internal/t2errors.h>


  /*************************************************************************/
  /*                                                                       */
  /* The macro FT_COMPONENT is used in trace mode.  It is an implicit      */
  /* parameter of the FT_TRACE() and FT_ERROR() macros, used to print/log  */
  /* messages during execution.                                            */
  /*                                                                       */
#undef  FT_COMPONENT
#define FT_COMPONENT  trace_t2gload


  typedef enum  T2_Operator_
  {
    t2_op_unknown = 0,

    t2_op_rmoveto,
    t2_op_hmoveto,
    t2_op_vmoveto,

    t2_op_rlineto,
    t2_op_hlineto,
    t2_op_vlineto,

    t2_op_rrcurveto,
    t2_op_hhcurveto,
    t2_op_hvcurveto,
    t2_op_rcurveline,
    t2_op_rlinecurve,
    t2_op_vhcurveto,
    t2_op_vvcurveto,

    t2_op_flex,
    t2_op_hflex,
    t2_op_hflex1,
    t2_op_flex1,

    t2_op_endchar,

    t2_op_hstem,
    t2_op_vstem,
    t2_op_hstemhm,
    t2_op_vstemhm,

    t2_op_hintmask,
    t2_op_cntrmask,

    t2_op_abs,
    t2_op_add,
    t2_op_sub,
    t2_op_div,
    t2_op_neg,
    t2_op_random,
    t2_op_mul,
    t2_op_sqrt,

    t2_op_blend,

    t2_op_drop,
    t2_op_exch,
    t2_op_index,
    t2_op_roll,
    t2_op_dup,

    t2_op_put,
    t2_op_get,
    t2_op_store,
    t2_op_load,

    t2_op_and,
    t2_op_or,
    t2_op_not,
    t2_op_eq,
    t2_op_ifelse,

    t2_op_callsubr,
    t2_op_callgsubr,
    t2_op_return,

    /* do not remove */
    t2_op_max

  } T2_Operator;


#define T2_COUNT_CHECK_WIDTH  0x80
#define T2_COUNT_EXACT        0x40
#define T2_COUNT_CLEAR_STACK  0x20


  static const FT_Byte  t2_argument_counts[] =
  {
    0,  /* unknown */

    2 | T2_COUNT_CHECK_WIDTH | T2_COUNT_EXACT, /* rmoveto */
    1 | T2_COUNT_CHECK_WIDTH | T2_COUNT_EXACT,
    1 | T2_COUNT_CHECK_WIDTH | T2_COUNT_EXACT,

    0 | T2_COUNT_CLEAR_STACK,  /* rlineto */
    0 | T2_COUNT_CLEAR_STACK,
    0 | T2_COUNT_CLEAR_STACK,

    0 | T2_COUNT_CLEAR_STACK,  /* rrcurveto */
    0 | T2_COUNT_CLEAR_STACK,
    0 | T2_COUNT_CLEAR_STACK,
    0 | T2_COUNT_CLEAR_STACK,
    0 | T2_COUNT_CLEAR_STACK,
    0 | T2_COUNT_CLEAR_STACK,
    0 | T2_COUNT_CLEAR_STACK,

    13, /* flex */
    7,
    9,
    11,

    0, /* endchar */

    2 | T2_COUNT_CHECK_WIDTH, /* hstem */
    2 | T2_COUNT_CHECK_WIDTH,
    2 | T2_COUNT_CHECK_WIDTH,
    2 | T2_COUNT_CHECK_WIDTH,

    0, /* hintmask */
    0, /* cntrmask */

    1, /* abs */
    2,
    2,
    2,
    1,
    0,
    2,
    1,

    1, /* blend */

    1, /* drop */
    2,
    1,
    2,
    1,

    2, /* put */
    1,
    4,
    3,

    2, /* and */
    2,
    1,
    2,
    4,

    1, /* callsubr */
    1,
    0
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
  /*    T2_Init_Builder                                                    */
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
  static
  void  T2_Init_Builder( T2_Builder*   builder,
                         TT_Face       face,
                         T2_Size       size,
                         T2_GlyphSlot  glyph )
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
      builder->base    = &loader->base.outline;
      builder->current = &loader->current.outline;
      FT_GlyphLoader_Rewind( loader );
    }

    if ( size )
    {
      builder->scale_x = size->metrics.x_scale;
      builder->scale_y = size->metrics.y_scale;
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
  /*    T2_Done_Builder                                                    */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Finalizes a given glyph builder.  Its contents can still be used   */
  /*    after the call, but the function saves important information       */
  /*    within the corresponding glyph slot.                               */
  /*                                                                       */
  /* <Input>                                                               */
  /*    builder :: A pointer to the glyph builder to finalize.             */
  /*                                                                       */
  static
  void  T2_Done_Builder( T2_Builder*  builder )
  {
    T2_GlyphSlot  glyph = builder->glyph;


    if ( glyph )
      glyph->root.outline = *builder->base;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    t2_compute_bias                                                    */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Computes the bias value in dependence of the number of glyph       */
  /*    subroutines.                                                       */
  /*                                                                       */
  /* <Input>                                                               */
  /*    num_subrs :: The number of glyph subroutines.                      */
  /*                                                                       */
  /* <Return>                                                              */
  /*    The bias value.                                                    */
  static
  FT_Int  t2_compute_bias( FT_UInt  num_subrs )
  {
    FT_Int  result;


    if ( num_subrs < 1240 )
      result = 107;
    else if ( num_subrs < 33900 )
      result = 1131;
    else
      result = 32768;

    return result;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    T2_Init_Decoder                                                    */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Initializes a given glyph decoder.                                 */
  /*                                                                       */
  /* <InOut>                                                               */
  /*    decoder :: A pointer to the glyph builder to initialize.           */
  /*                                                                       */
  /* <Input>                                                               */
  /*    face    :: The current face object.                                */
  /*                                                                       */
  /*    size    :: The current size object.                                */
  /*                                                                       */
  /*    slot    :: The current glyph object.                               */
  /*                                                                       */
  LOCAL_FUNC
  void  T2_Init_Decoder( T2_Decoder*  decoder,
                         TT_Face      face,
                         T2_Size      size,
                         T2_GlyphSlot slot )
  {
    CFF_Font*  cff = (CFF_Font*)face->extra.data;


    /* clear everything */
    MEM_Set( decoder, 0, sizeof ( *decoder ) );

    /* initialize builder */
    T2_Init_Builder( &decoder->builder, face, size, slot );

    /* initialize Type2 decoder */
    decoder->num_globals  = cff->num_global_subrs;
    decoder->globals      = cff->global_subrs;
    decoder->globals_bias = t2_compute_bias( decoder->num_globals );
  }


  /* this function is used to select the locals subrs array */
  LOCAL_DEF
  void  T2_Prepare_Decoder( T2_Decoder*  decoder,
                            FT_UInt      glyph_index )
  {
    CFF_Font*     cff = (CFF_Font*)decoder->builder.face->extra.data;
    CFF_SubFont*  sub = &cff->top_font;


    /* manage CID fonts */
    if ( cff->num_subfonts >= 1 )
    {
      FT_Byte  fd_index = CFF_Get_FD( &cff->fd_select, glyph_index );


      sub = cff->subfonts[fd_index];
    }

    decoder->num_locals    = sub->num_local_subrs;
    decoder->locals        = sub->local_subrs;
    decoder->locals_bias   = t2_compute_bias( decoder->num_locals );

    decoder->glyph_width   = sub->private_dict.default_width;
    decoder->nominal_width = sub->private_dict.nominal_width;
  }


  /* check that there is enough room for `count' more points */
  static
  FT_Error  check_points( T2_Builder*  builder,
                          FT_Int       count )
  {
    return FT_GlyphLoader_Check_Points( builder->loader, count, 0 );
  }


  /* add a new point, do not check space */
  static
  void  add_point( T2_Builder*  builder,
                   FT_Pos       x,
                   FT_Pos       y,
                   FT_Byte      flag )
  {
    FT_Outline*  outline = builder->current;


    if ( builder->load_points )
    {
      FT_Vector*  point   = outline->points + outline->n_points;
      FT_Byte*    control = (FT_Byte*)outline->tags + outline->n_points;


      point->x = x >> 16;
      point->y = y >> 16;
      *control = flag ? FT_Curve_Tag_On : FT_Curve_Tag_Cubic;

      builder->last = *point;
    }
    outline->n_points++;
  }


  /* check space for a new on-curve point, then add it */
  static
  FT_Error  add_point1( T2_Builder*  builder,
                        FT_Pos       x,
                        FT_Pos       y )
  {
    FT_Error  error;


    error = check_points( builder, 1 );
    if ( !error )
      add_point( builder, x, y, 1 );

    return error;
  }


  /* check room for a new contour, then add it */
  static
  FT_Error  add_contour( T2_Builder*  builder )
  {
    FT_Outline*  outline = builder->current;
    FT_Error     error;


    if ( !builder->load_points )
    {
      outline->n_contours++;
      return T2_Err_Ok;
    }

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
  FT_Error  start_point( T2_Builder*  builder,
                         FT_Pos       x,
                         FT_Pos       y )
  {
    FT_Error  error = 0;


    /* test whether we are building a new contour */
    if ( !builder->path_begun )
    {
      builder->path_begun = 1;
      error = add_contour( builder );
      if ( !error )
        error = add_point1( builder, x, y );
    }
    return error;
  }


  /* close the current contour */
  static
  void  close_contour( T2_Builder*  builder )
  {
    FT_Outline*  outline = builder->current;

    /* XXXX: We must not include the last point in the path if it */
    /*       is located on the first point.                       */
    if ( outline->n_points > 1 )
    {
      FT_Int      first = 0;
      FT_Vector*  p1    = outline->points + first;
      FT_Vector*  p2    = outline->points + outline->n_points - 1;

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


#define USE_ARGS( n )  do                            \
                       {                             \
                         top -= n;                   \
                         if ( top < decoder->stack ) \
                           goto Stack_Underflow;     \
                       } while ( 0 )


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    T2_Parse_CharStrings                                               */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Parses a given Type 2 charstrings program.                         */
  /*                                                                       */
  /* <InOut>                                                               */
  /*    decoder         :: The current Type 1 decoder.                     */
  /*                                                                       */
  /* <Input>                                                               */
  /*    charstring_base :: The base of the charstring stream.              */
  /*                                                                       */
  /*    charstring_len  :: The length in bytes of the charstring stream.   */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  LOCAL_FUNC
  FT_Error  T2_Parse_CharStrings( T2_Decoder*  decoder,
                                  FT_Byte*     charstring_base,
                                  FT_Int       charstring_len )
  {
    FT_Error          error;
    T2_Decoder_Zone*  zone;
    FT_Byte*          ip;
    FT_Byte*          limit;
    T2_Builder*       builder = &decoder->builder;
    FT_Outline*       outline;
    FT_Pos            x, y;
    FT_Fixed          seed;
    FT_Fixed*         stack;


    /* set default width */
    decoder->num_hints  = 0;
    decoder->read_width = 1;

    /* compute random seed from stack address of parameter */
    seed = (FT_Fixed)(char*)&seed           ^
           (FT_Fixed)(char*)&decoder        ^
           (FT_Fixed)(char*)&charstring_base;
    seed = ( seed ^ ( seed >> 10 ) ^ ( seed >> 20 ) ) & 0xFFFF;
    if ( seed == 0 )
      seed = 0x7384;

    /* initialize the decoder */
    decoder->top  = decoder->stack;
    decoder->zone = decoder->zones;
    zone          = decoder->zones;
    stack         = decoder->top;

    builder->path_begun = 0;

    zone->base           = charstring_base;
    limit = zone->limit  = charstring_base + charstring_len;
    ip    = zone->cursor = zone->base;

    error   = T2_Err_Ok;
    outline = builder->current;

    x = builder->pos_x;
    y = builder->pos_y;

    /* now, execute loop */
    while ( ip < limit )
    {
      T2_Operator  op;
      FT_Byte      v;
      FT_Byte      count;


      /********************************************************************/
      /*                                                                  */
      /* Decode operator or operand                                       */
      /*                                                                  */
      v = *ip++;
      if ( v >= 32 || v == 28 )
      {
        FT_Int    shift = 16;
        FT_Int32  val;


        /* this is an operand, push it on the stack */
        if ( v == 28 )
        {
          if ( ip + 1 >= limit )
            goto Syntax_Error;
          val = (FT_Short)( ( (FT_Short)ip[0] << 8 ) | ip[1] );
          ip += 2;
        }
        else if ( v < 247 )
          val = (FT_Long)v - 139;
        else if ( v < 251 )
        {
          if ( ip >= limit )
            goto Syntax_Error;
          val = ( (FT_Long)v - 247 ) * 256 + *ip++ + 108;
        }
        else if ( v < 255 )
        {
          if ( ip >= limit )
            goto Syntax_Error;
          val = -( (FT_Long)v - 251 ) * 256 - *ip++ - 108;
        }
        else
        {
          if ( ip + 3 >= limit )
            goto Syntax_Error;
          val = ( (FT_Int32)ip[0] << 24 ) |
                ( (FT_Int32)ip[1] << 16 ) |
                ( (FT_Int32)ip[2] <<  8 ) |
                            ip[3];
          ip    += 4;
          shift  = 0;
        }
        if ( decoder->top - stack >= T2_MAX_OPERANDS )
          goto Stack_Overflow;

        val           <<= shift;
        *decoder->top++ = val;

#ifdef FT_DEBUG_LEVEL_TRACE
        if ( !( val & 0xFFFF ) )
          FT_TRACE4(( " %d", (FT_Int32)( val >> 16 ) ));
        else
          FT_TRACE4(( " %.2f", val/65536.0 ));
#endif

      }
      else
      {
        FT_Fixed*  args     = decoder->top;
        FT_Int     num_args = args - decoder->stack;
        FT_Int     req_args;


        /* find operator */
        op = t2_op_unknown;

        switch ( v )
        {
        case 1:
          op = t2_op_hstem;
          break;
        case 3:
          op = t2_op_vstem;
          break;
        case 4:
          op = t2_op_vmoveto;
          break;
        case 5:
          op = t2_op_rlineto;
          break;
        case 6:
          op = t2_op_hlineto;
          break;
        case 7:
          op = t2_op_vlineto;
          break;
        case 8:
          op = t2_op_rrcurveto;
          break;
        case 10:
          op = t2_op_callsubr;
          break;
        case 11:
          op = t2_op_return;
          break;
        case 12:
          {
            if ( ip >= limit )
              goto Syntax_Error;
            v = *ip++;

            switch ( v )
            {
            case 3:
              op = t2_op_and;
              break;
            case 4:
              op = t2_op_or;
              break;
            case 5:
              op = t2_op_not;
              break;
            case 8:
              op = t2_op_store;
              break;
            case 9:
              op = t2_op_abs;
              break;
            case 10:
              op = t2_op_add;
              break;
            case 11:
              op = t2_op_sub;
              break;
            case 12:
              op = t2_op_div;
              break;
            case 13:
              op = t2_op_load;
              break;
            case 14:
              op = t2_op_neg;
              break;
            case 15:
              op = t2_op_eq;
              break;
            case 18:
              op = t2_op_drop;
              break;
            case 20:
              op = t2_op_put;
              break;
            case 21:
              op = t2_op_get;
              break;
            case 22:
              op = t2_op_ifelse;
              break;
            case 23:
              op = t2_op_random;
              break;
            case 24:
              op = t2_op_mul;
              break;
            case 26:
              op = t2_op_sqrt;
              break;
            case 27:
              op = t2_op_dup;
              break;
            case 28:
              op = t2_op_exch;
              break;
            case 29:
              op = t2_op_index;
              break;
            case 30:
              op = t2_op_roll;
              break;
            case 34:
              op = t2_op_hflex;
              break;
            case 35:
              op = t2_op_flex;
              break;
            case 36:
              op = t2_op_hflex1;
              break;
            case 37:
              op = t2_op_flex1;
              break;
            default:
              /* decrement ip for syntax error message */
              ip--;
            }
          }
          break;
        case 14:
          op = t2_op_endchar;
          break;
        case 16:
          op = t2_op_blend;
          break;
        case 18:
          op = t2_op_hstemhm;
          break;
        case 19:
          op = t2_op_hintmask;
          break;
        case 20:
          op = t2_op_cntrmask;
          break;
        case 21:
          op = t2_op_rmoveto;
          break;
        case 22:
          op = t2_op_hmoveto;
          break;
        case 23:
          op = t2_op_vstemhm;
          break;
        case 24:
          op = t2_op_rcurveline;
          break;
        case 25:
          op = t2_op_rlinecurve;
          break;
        case 26:
          op = t2_op_vvcurveto;
          break;
        case 27:
          op = t2_op_hhcurveto;
          break;
        case 29:
          op = t2_op_callgsubr;
          break;
        case 30:
          op = t2_op_vhcurveto;
          break;
        case 31:
          op = t2_op_hvcurveto;
          break;
        default:
          ;
        }
        if ( op == t2_op_unknown )
          goto Syntax_Error;

        /* check arguments */
        req_args = count = t2_argument_counts[op];
        if ( req_args & T2_COUNT_CHECK_WIDTH )
        {
          args = stack;
          if ( num_args & 1 && decoder->read_width )
          {
            decoder->glyph_width = decoder->nominal_width +
                                     ( stack[0] >> 16 );
            num_args--;
            args++;
          }
          decoder->read_width  = 0;
          req_args = 0;
        }

        req_args &= 15;
        if ( num_args < req_args )
          goto Stack_Underflow;
        args     -= req_args;
        num_args -= req_args;

        switch ( op )
        {
        case t2_op_hstem:
        case t2_op_vstem:
        case t2_op_hstemhm:
        case t2_op_vstemhm:
          /* if the number of arguments is not even, the first one */
          /* is simply the glyph width, encoded as the difference  */
          /* to nominalWidthX                                      */
          FT_TRACE4(( op == t2_op_hstem   ? " hstem"   :
                      op == t2_op_vstem   ? " vstem"   :
                      op == t2_op_hstemhm ? " hstemhm" :
                                            " vstemhm" ));
          decoder->num_hints += num_args / 2;
          args = stack;
          break;

        case t2_op_hintmask:
        case t2_op_cntrmask:
          FT_TRACE4(( op == t2_op_hintmask ? " hintmask"
                                           : " cntrmask" ));

          decoder->num_hints += num_args / 2;
          ip += ( decoder->num_hints + 7 ) >> 3;
          if ( ip >= limit )
            goto Syntax_Error;
          args = stack;
          break;

        case t2_op_rmoveto:
          FT_TRACE4(( " rmoveto" ));

          close_contour( builder );
          builder->path_begun = 0;
          x   += args[0];
          y   += args[1];
          args = stack;
          break;

        case t2_op_vmoveto:
          FT_TRACE4(( " vmoveto" ));

          close_contour( builder );
          builder->path_begun = 0;
          y   += args[0];
          args = stack;
          break;

        case t2_op_hmoveto:
          FT_TRACE4(( " hmoveto" ));

          close_contour( builder );
          builder->path_begun = 0;
          x   += args[0];
          args = stack;
          break;

        case t2_op_rlineto:
          FT_TRACE4(( " rlineto" ));

          if ( start_point ( builder, x, y )         ||
               check_points( builder, num_args / 2 ) )
            goto Memory_Error;

          if ( num_args < 2 || num_args & 1 )
            goto Stack_Underflow;

          args = stack;
          while ( args < decoder->top )
          {
            x += args[0];
            y += args[1];
            add_point( builder, x, y, 1 );
            args += 2;
          }
          args = stack;
          break;

        case t2_op_hlineto:
        case t2_op_vlineto:
          {
            FT_Int  phase = ( op == t2_op_hlineto );


            FT_TRACE4(( op == t2_op_hlineto ? " hlineto"
                                            : " vlineto" ));

            if ( start_point ( builder, x, y )     ||
                 check_points( builder, num_args ) )
              goto Memory_Error;

            args = stack;
            while (args < decoder->top )
            {
              if ( phase )
                x += args[0];
              else
                y += args[0];

              if ( add_point1( builder, x, y ) )
                goto Memory_Error;

              args++;
              phase ^= 1;
            }
            args = stack;
          }
          break;

        case t2_op_rrcurveto:
          FT_TRACE4(( " rrcurveto" ));

          /* check number of arguments; must be a multiple of 6 */
          if ( num_args % 6 != 0 )
            goto Stack_Underflow;

          if ( start_point ( builder, x, y )         ||
               check_points( builder, num_args / 2 ) )
            goto Memory_Error;

          args = stack;
          while ( args < decoder->top )
          {
            x += args[0];
            y += args[1];
            add_point( builder, x, y, 0 );
            x += args[2];
            y += args[3];
            add_point( builder, x, y, 0 );
            x += args[4];
            y += args[5];
            add_point( builder, x, y, 1 );
            args += 6;
          }
          args = stack;
          break;

        case t2_op_vvcurveto:
          FT_TRACE4(( " vvcurveto" ));

          if ( start_point ( builder, x, y ) )
            goto Memory_Error;

          args = stack;
          if ( num_args & 1 )
          {
            x += args[0];
            args++;
            num_args--;
          }

          if ( num_args % 4 != 0 )
            goto Stack_Underflow;

          if ( check_points( builder, 3 * ( num_args / 4 ) ) )
            goto Memory_Error;

          while ( args < decoder->top )
          {
            y += args[0];
            add_point( builder, x, y, 0 );
            x += args[1];
            y += args[2];
            add_point( builder, x, y, 0 );
            y += args[3];
            add_point( builder, x, y, 1 );
            args += 4;
          }
          args = stack;
          break;

        case t2_op_hhcurveto:
          FT_TRACE4(( " hhcurveto" ));

          if ( start_point ( builder, x, y ) )
            goto Memory_Error;

          args = stack;
          if ( num_args & 1 )
          {
            y += args[0];
            args++;
            num_args--;
          }

          if ( num_args % 4 != 0 )
            goto Stack_Underflow;

          if ( check_points( builder, 3 * ( num_args / 4 ) ) )
            goto Memory_Error;

          while ( args < decoder->top )
          {
            x += args[0];
            add_point( builder, x, y, 0 );
            x += args[1];
            y += args[2];
            add_point( builder, x, y, 0 );
            x += args[3];
            add_point( builder, x, y, 1 );
            args += 4;
          }
          args = stack;
          break;

        case t2_op_vhcurveto:
        case t2_op_hvcurveto:
          {
            FT_Int  phase;


            FT_TRACE4(( op == t2_op_vhcurveto ? " vhcurveto"
                                              : " hvcurveto" ));

            if ( start_point ( builder, x, y ) )
              goto Memory_Error;

            args = stack;
            if (num_args < 4 || ( num_args % 4 ) > 1 )
              goto Stack_Underflow;

            if ( check_points( builder, ( num_args / 4 ) * 3 ) )
              goto Stack_Underflow;

            phase = ( op == t2_op_hvcurveto );

            while ( num_args >= 4 )
            {
              num_args -= 4;
              if ( phase )
              {
                x += args[0];
                add_point( builder, x, y, 0 );
                x += args[1];
                y += args[2];
                add_point( builder, x, y, 0 );
                y += args[3];
                if ( num_args == 1 )
                  x += args[4];
                add_point( builder, x, y, 1 );
              }
              else
              {
                y += args[0];
                add_point( builder, x, y, 0 );
                x += args[1];
                y += args[2];
                add_point( builder, x, y, 0 );
                x += args[3];
                if ( num_args == 1 )
                  y += args[4];
                add_point( builder, x, y, 1 );
              }
              args  += 4;
              phase ^= 1;
            }
            args = stack;
          }
          break;

        case t2_op_rlinecurve:
          {
            FT_Int  num_lines = ( num_args - 6 ) / 2;


            FT_TRACE4(( " rlinecurve" ));

            if ( num_args < 8 || ( num_args - 6 ) & 1 )
              goto Stack_Underflow;

            if ( start_point( builder, x, y )           ||
                 check_points( builder, num_lines + 3 ) )
              goto Memory_Error;

            args = stack;

            /* first, add the line segments */
            while ( num_lines > 0 )
            {
              x += args[0];
              y += args[1];
              add_point( builder, x, y, 1 );
              args += 2;
              num_lines--;
            }

            /* then the curve */
            x += args[0];
            y += args[1];
            add_point( builder, x, y, 0 );
            x += args[2];
            y += args[3];
            add_point( builder, x, y, 0 );
            x += args[4];
            y += args[5];
            add_point( builder, x, y, 1 );
            args = stack;
          }
          break;

        case t2_op_rcurveline:
          {
            FT_Int  num_curves = ( num_args - 2 ) / 6;


            FT_TRACE4(( " rcurveline" ));

            if ( num_args < 8 || ( num_args - 2 ) % 6 )
              goto Stack_Underflow;

            if ( start_point ( builder, x, y )             ||
                 check_points( builder, num_curves*3 + 2 ) )
              goto Memory_Error;

            args = stack;

            /* first, add the curves */
            while ( num_curves > 0 )
            {
              x += args[0];
              y += args[1];
              add_point( builder, x, y, 0 );
              x += args[2];
              y += args[3];
              add_point( builder, x, y, 0 );
              x += args[4];
              y += args[5];
              add_point( builder, x, y, 1 );
              args += 6;
              num_curves--;
            }

            /* then the final line */
            x += args[0];
            y += args[1];
            add_point( builder, x, y, 1 );
            args = stack;
          }
          break;

        case t2_op_hflex1:
          {
            FT_Pos start_y;


            FT_TRACE4(( " hflex1" ));

            args = stack;

            /* adding five more points; 4 control points, 1 on-curve point */
            /* make sure we have enough space for the start point if it    */
            /* needs to be added..                                         */
            if ( start_point( builder, x, y ) ||
                 check_points( builder, 6 )   )
              goto Memory_Error;

            /* Record the starting point's y postion for later use */
            start_y = y;

            /* first control point */
            x += args[0];
            y += args[1];
            add_point( builder, x, y, 0 );

            /* second control point */
            x += args[2];
            y += args[3];
            add_point( builder, x, y, 0 );

            /* join point; on curve, with y-value the same as the last */
            /* control point's y-value                                 */
            x += args[4];
            add_point( builder, x, y, 1 );

            /* third control point, with y-value the same as the join */
            /* point's y-value                                        */
            x += args[5];
            add_point( builder, x, y, 0 );

            /* fourth control point */
            x += args[6];
            y += args[7];
            add_point( builder, x, y, 0 );

            /* ending point, with y-value the same as the start   */
            x += args[8];
            y  = start_y;
            add_point( builder, x, y, 1 );

            args = stack;
            break;
          }

        case t2_op_hflex:
          {
            FT_Pos start_y;


            FT_TRACE4(( " hflex" ));

            args = stack;

            /* adding six more points; 4 control points, 2 on-curve points */
            if ( start_point( builder, x, y ) ||
                 check_points ( builder, 6 )  )
              goto Memory_Error;

            /* record the starting point's y-position for later use */
            start_y = y;

            /* first control point */
            x += args[0];
            add_point( builder, x, y, 0 );

            /* second control point */
            x += args[1];
            y += args[2];
            add_point( builder, x, y, 0 );

            /* join point; on curve, with y-value the same as the last */
            /* control point's y-value                                 */
            x += args[3];
            add_point( builder, x, y, 1 );

            /* third control point, with y-value the same as the join */
            /* point's y-value                                        */
            x += args[4];
            add_point( builder, x, y, 0 );

            /* fourth control point */
            x += args[5];
            y  = start_y;
            add_point( builder, x, y, 0 );

            /* ending point, with y-value the same as the start point's */
            /* y-value -- we don't add this point, though               */
            x += args[6];
            add_point( builder, x, y, 1 );

            args = stack;
            break;
          }

        case t2_op_flex1:
          {
            FT_Pos  start_x, start_y; /* record start x, y values for alter */
                                      /* use                                */
            FT_Int  dx = 0, dy = 0;   /* used in horizontal/vertical        */
                                      /* algorithm below                    */
            FT_Int  horizontal, count;


            FT_TRACE4(( " flex1" ));

            /* adding six more points; 4 control points, 2 on-curve points */
            if ( start_point( builder, x, y ) ||
                 check_points( builder, 6 )   )
               goto Memory_Error;

            /* record the starting point's x, y postion for later use */
            start_x = x;
            start_y = y;

            /* XXX: figure out whether this is supposed to be a horizontal */
            /*      or vertical flex; the Type 2 specification is vague... */

            args = stack;

            /* grab up to the last argument */
            for ( count = 5; count > 0; count-- )
            {
              dx += args[0];
              dy += args[1];
              args += 2;
            }

            /* rewind */
            args = stack;

            if ( dx < 0 ) dx = -dx;
            if ( dy < 0 ) dy = -dy;

            /* strange test, but here it is... */
            horizontal = ( dx > dy );

            for ( count = 5; count > 0; count-- )
            {
              x += args[0];
              y += args[1];
              add_point( builder, x, y, (FT_Bool)( count == 3 ) );
              args += 2;
            }

            /* is last operand an x- or y-delta? */
            if ( horizontal )
            {
              x += args[0];
              y  = start_y;
            }
            else
            {
              x  = start_x;
              y += args[0];
            }

            add_point( builder, x, y, 1 );

            args = stack;
            break;
           }

        case t2_op_flex:
          {
            FT_UInt  count;


            FT_TRACE4(( " flex" ));

            if ( start_point( builder, x, y ) ||
                 check_points( builder, 6 )   )
              goto Memory_Error;

            args = stack;
            for ( count = 6; count > 0; count-- )
            {
              x += args[0];
              y += args[1];
              add_point( builder, x, y,
                         (FT_Bool)( count == 3 || count == 0 ) );
              args += 2;
            }

            args = stack;
          }
          break;

        case t2_op_endchar:
          FT_TRACE4(( " endchar" ));

          close_contour( builder );

          /* add current outline to the glyph slot */
          FT_GlyphLoader_Add( builder->loader );

          /* return now! */
          FT_TRACE4(( "\n\n" ));
          return T2_Err_Ok;

        case t2_op_abs:
          FT_TRACE4(( " abs" ));

          if ( args[0] < 0 )
            args[0] = -args[0];
          args++;
          break;

        case t2_op_add:
          FT_TRACE4(( " add" ));

          args[0] += args[1];
          args++;
          break;

        case t2_op_sub:
          FT_TRACE4(( " sub" ));

          args[0] -= args[1];
          args++;
          break;

        case t2_op_div:
          FT_TRACE4(( " div" ));

          args[0] = FT_DivFix( args[0], args[1] );
          args++;
          break;

        case t2_op_neg:
          FT_TRACE4(( " neg" ));

          args[0] = -args[0];
          args++;
          break;

        case t2_op_random:
          {
            FT_Fixed  rand;


            FT_TRACE4(( " rand" ));

            rand = seed;
            if ( rand >= 0x8000 )
              rand++;

            args[0] = rand;
            seed    = FT_MulFix( seed, 0x10000L - seed );
            if ( seed == 0 )
              seed += 0x2873;
            args++;
          }
          break;

        case t2_op_mul:
          FT_TRACE4(( " mul" ));

          args[0] = FT_MulFix( args[0], args[1] );
          args++;
          break;

        case t2_op_sqrt:
          FT_TRACE4(( " sqrt" ));

          if ( args[0] > 0 )
          {
            FT_Int    count = 9;
            FT_Fixed  root  = args[0];
            FT_Fixed  new_root;


            for (;;)
            {
              new_root = ( root + FT_DivFix(args[0],root) + 1 ) >> 1;
              if ( new_root == root || count <= 0 )
                break;
              root = new_root;
            }
            args[0] = new_root;
          }
          else
            args[0] = 0;
          args++;
          break;

        case t2_op_drop:
          /* nothing */
          FT_TRACE4(( " drop" ));

          break;

        case t2_op_exch:
          {
            FT_Fixed  tmp;


            FT_TRACE4(( " exch" ));

            tmp     = args[0];
            args[0] = args[1];
            args[1] = tmp;
            args   += 2;
          }
          break;

        case t2_op_index:
          {
            FT_Int  index = args[0] >> 16;


            FT_TRACE4(( " index" ));

            if ( index < 0 )
              index = 0;
            else if ( index > num_args - 2 )
              index = num_args - 2;
            args[0] = args[-( index + 1 )];
            args++;
          }
          break;

        case t2_op_roll:
          {
            FT_Int count = (FT_Int)( args[0] >> 16 );
            FT_Int index = (FT_Int)( args[1] >> 16 );


            FT_TRACE4(( " roll" ));

            if ( count <= 0 )
              count = 1;

            args -= count;
            if ( args < stack )
              goto Stack_Underflow;

            if ( index >= 0 )
            {
              while ( index > 0 )
              {
                FT_Fixed tmp = args[count - 1];
                FT_Int   i;


                for ( i = count - 2; i >= 0; i-- )
                  args[i + 1] = args[i];
                args[0] = tmp;
                index--;
              }
            }
            else
            {
              while ( index < 0 )
              {
                FT_Fixed  tmp = args[0];
                FT_Int    i;


                for ( i = 0; i < count - 1; i++ )
                  args[i] = args[i + 1];
                args[count - 1] = tmp;
                index++;
              }
            }
            args += count;
          }
          break;

        case t2_op_dup:
          FT_TRACE4(( " dup" ));

          args[1] = args[0];
          args++;
          break;

        case t2_op_put:
          {
            FT_Fixed  val   = args[0];
            FT_Int    index = (FT_Int)( args[1] >> 16 );


            FT_TRACE4(( " put" ));

            if ( index >= 0 && index < decoder->len_buildchar )
              decoder->buildchar[index] = val;
          }
          break;

        case t2_op_get:
          {
            FT_Int   index = (FT_Int)( args[0] >> 16 );
            FT_Fixed val   = 0;


            FT_TRACE4(( " get" ));

            if ( index >= 0 && index < decoder->len_buildchar )
              val = decoder->buildchar[index];

            args[0] = val;
            args++;
          }
          break;

        case t2_op_store:
          FT_TRACE4(( " store "));

          goto Unimplemented;

        case t2_op_load:
          FT_TRACE4(( " load" ));

          goto Unimplemented;

        case t2_op_and:
          {
            FT_Fixed  cond = args[0] && args[1];


            FT_TRACE4(( " and" ));

            args[0] = cond ? 0x10000L : 0;
            args++;
          }
          break;

        case t2_op_or:
          {
            FT_Fixed  cond = args[0] || args[1];


            FT_TRACE4(( " or" ));

            args[0] = cond ? 0x10000L : 0;
            args++;
          }
          break;

        case t2_op_eq:
          {
            FT_Fixed  cond = !args[0];


            FT_TRACE4(( " eq" ));

            args[0] = cond ? 0x10000L : 0;
            args++;
          }
          break;

        case t2_op_ifelse:
          {
            FT_Fixed  cond = (args[2] <= args[3]);


            FT_TRACE4(( " ifelse" ));

            if ( !cond )
              args[0] = args[1];
            args++;
          }
          break;

        case t2_op_callsubr:
          {
            FT_UInt  index = (FT_UInt)( ( args[0] >> 16 ) +
                                        decoder->locals_bias );


            FT_TRACE4(( " callsubr(%d)", index ));

            if ( index >= decoder->num_locals )
            {
              FT_ERROR(( "T2_Parse_CharStrings:" ));
              FT_ERROR(( "  invalid local subr index\n" ));
              goto Syntax_Error;
            }

            if ( zone - decoder->zones >= T2_MAX_SUBRS_CALLS )
            {
              FT_ERROR(( "T2_Parse_CharStrings: too many nested subrs\n" ));
              goto Syntax_Error;
            }

            zone->cursor = ip;  /* save current instruction pointer */

            zone++;
            zone->base   = decoder->locals[index];
            zone->limit  = decoder->locals[index+1];
            zone->cursor = zone->base;

            if ( !zone->base )
            {
              FT_ERROR(( "T2_Parse_CharStrings: invoking empty subrs!\n" ));
              goto Syntax_Error;
            }

            decoder->zone = zone;
            ip            = zone->base;
            limit         = zone->limit;
          }
          break;

        case t2_op_callgsubr:
          {
            FT_UInt  index = (FT_UInt)( ( args[0] >> 16 ) +
                                        decoder->globals_bias );


            FT_TRACE4(( " callgsubr(%d)", index ));

            if ( index >= decoder->num_globals )
            {
              FT_ERROR(( "T2_Parse_CharStrings:" ));
              FT_ERROR(( " invalid global subr index\n" ));
              goto Syntax_Error;
            }

            if ( zone - decoder->zones >= T2_MAX_SUBRS_CALLS )
            {
              FT_ERROR(( "T2_Parse_CharStrings: too many nested subrs\n" ));
              goto Syntax_Error;
            }

            zone->cursor = ip;  /* save current instruction pointer */

            zone++;
            zone->base   = decoder->globals[index];
            zone->limit  = decoder->globals[index+1];
            zone->cursor = zone->base;

            if ( !zone->base )
            {
              FT_ERROR(( "T2_Parse_CharStrings: invoking empty subrs!\n" ));
              goto Syntax_Error;
            }

            decoder->zone = zone;
            ip            = zone->base;
            limit         = zone->limit;
          }
          break;

        case t2_op_return:
          FT_TRACE4(( " return" ));

          if ( decoder->zone <= decoder->zones )
          {
            FT_ERROR(( "T2_Parse_CharStrings: unexpected return\n" ));
            goto Syntax_Error;
          }

          decoder->zone--;
          zone  = decoder->zone;
          ip    = zone->cursor;
          limit = zone->limit;
          break;

        default:
        Unimplemented:
          FT_ERROR(( "Unimplemented opcode: %d", ip[-1] ));

          if ( ip[-1] == 12 )
            FT_ERROR(( " %d", ip[0] ));
          FT_ERROR(( "\n" ));

          return T2_Err_Unimplemented_Feature;
        }

      decoder->top = args;

      } /* general operator processing */

    } /* while ip < limit */

    FT_TRACE4(( "..end..\n\n" ));

    return error;

  Syntax_Error:
    FT_TRACE4(( "T2_Parse_CharStrings: syntax error!" ));
    return T2_Err_Invalid_File_Format;

  Stack_Underflow:
    FT_TRACE4(( "T2_Parse_CharStrings: stack underflow!" ));
    return T2_Err_Too_Few_Arguments;

  Stack_Overflow:
    FT_TRACE4(( "T2_Parse_CharStrings: stack overflow!" ));
    return T2_Err_Stack_Overflow;

  Memory_Error:
    return builder->error;
  }


  /*************************************************************************/
  /*************************************************************************/
  /*************************************************************************/
  /**********                                                      *********/
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


#if 0 /* unused until we support pure CFF fonts */


  LOCAL_FUNC
  FT_Error  T2_Compute_Max_Advance( TT_Face  face,
                                    FT_Int*  max_advance )
  {
    FT_Error    error = 0;
    T2_Decoder  decoder;
    FT_Int      glyph_index;
    CFF_Font*   cff = (CFF_Font*)face->other;


    *max_advance = 0;

    /* Initialize load decoder */
    T2_Init_Decoder( &decoder, face, 0, 0 );

    decoder.builder.metrics_only = 1;
    decoder.builder.load_points  = 0;

    /* For each glyph, parse the glyph charstring and extract */
    /* the advance width.                                     */
    for ( glyph_index = 0; glyph_index < face->root.num_glyphs;
          glyph_index++ )
    {
      FT_Byte*  charstring;
      FT_ULong  charstring_len;


      /* now get load the unscaled outline */
      error = T2_Access_Element( &cff->charstrings_index, glyph_index,
                                 &charstring, &charstring_len );
      if ( !error )
      {
        T2_Prepare_Decoder( &decoder, glyph_index );
        error = T2_Parse_CharStrings( &decoder, charstring, charstring_len );

        T2_Forget_Element( &cff->charstrings_index, &charstring );
      }

      /* ignore the error if one has occurred -- skip to next glyph */
      error = 0;
    }

    *max_advance = decoder.builder.advance.x;

    return T2_Err_Ok;
  }


#endif /* 0 */


  /*************************************************************************/
  /*************************************************************************/
  /*************************************************************************/
  /**********                                                      *********/
  /**********                                                      *********/
  /**********               UNHINTED GLYPH LOADER                  *********/
  /**********                                                      *********/
  /**********    The following code is in charge of loading a      *********/
  /**********    single outline.  It completely ignores hinting    *********/
  /**********    and is used when FT_LOAD_NO_HINTING is set.       *********/
  /**********                                                      *********/
  /*************************************************************************/
  /*************************************************************************/
  /*************************************************************************/


  LOCAL_FUNC
  FT_Error  T2_Load_Glyph( T2_GlyphSlot  glyph,
                           T2_Size       size,
                           FT_Int        glyph_index,
                           FT_Int        load_flags )
  {
    FT_Error    error;
    T2_Decoder  decoder;
    TT_Face     face = (TT_Face)glyph->root.face;
    FT_Bool     hinting;
    CFF_Font*   cff = (CFF_Font*)face->extra.data;


    if ( load_flags & FT_LOAD_NO_RECURSE )
      load_flags |= FT_LOAD_NO_SCALE | FT_LOAD_NO_HINTING;

    glyph->x_scale = 0x10000L;
    glyph->y_scale = 0x10000L;
    if ( size )
    {
      glyph->x_scale = size->metrics.x_scale;
      glyph->y_scale = size->metrics.y_scale;
    }

    glyph->root.outline.n_points   = 0;
    glyph->root.outline.n_contours = 0;

    hinting = ( load_flags & FT_LOAD_NO_SCALE   ) == 0 &&
              ( load_flags & FT_LOAD_NO_HINTING ) == 0;

    glyph->root.format = ft_glyph_format_outline;  /* by default */

    {
      FT_Byte*  charstring;
      FT_ULong  charstring_len;


      T2_Init_Decoder( &decoder, face, size, glyph );

      decoder.builder.no_recurse =
        (FT_Bool)( ( load_flags & FT_LOAD_NO_RECURSE ) != 0 );

      /* now load the unscaled outline */
      error = T2_Access_Element( &cff->charstrings_index, glyph_index,
                                 &charstring, &charstring_len );
      if ( !error )
      {
        T2_Prepare_Decoder( &decoder, glyph_index );
        error = T2_Parse_CharStrings( &decoder, charstring, charstring_len );

        T2_Forget_Element( &cff->charstrings_index, &charstring );
      }

      /* save new glyph tables */
      T2_Done_Builder( &decoder.builder );
    }

    /* Now, set the metrics -- this is rather simple, as   */
    /* the left side bearing is the xMin, and the top side */
    /* bearing the yMax.                                   */
    if ( !error )
    {
      /* for composite glyphs, return only left side bearing and */
      /* advance width                                           */
      if ( glyph->root.format == ft_glyph_format_composite )
      {
        glyph->root.metrics.horiBearingX = decoder.builder.left_bearing.x;
        glyph->root.metrics.horiAdvance  = decoder.glyph_width;
      }
      else
      {
        FT_BBox            cbox;
        FT_Glyph_Metrics*  metrics = &glyph->root.metrics;


        /* copy the _unscaled_ advance width */
        metrics->horiAdvance = decoder.glyph_width;

        /* make up vertical metrics */
        metrics->vertBearingX = 0;
        metrics->vertBearingY = 0;
        metrics->vertAdvance  = 0;

        glyph->root.format = ft_glyph_format_outline;

        glyph->root.outline.flags = 0;
        if ( size && size->metrics.y_ppem < 24 )
          glyph->root.outline.flags |= ft_outline_high_precision;

        glyph->root.outline.flags |= ft_outline_reverse_fill;

        if ( ( load_flags & FT_LOAD_NO_SCALE ) == 0 )
        {
          /* scale the outline and the metrics */
          FT_Int       n;
          FT_Outline*  cur     = &glyph->root.outline;
          FT_Vector*   vec     = cur->points;
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

#if 0
        /* apply the font matrix */
        FT_Outline_Transform( &glyph->root.outline, cff->font_matrix );
#endif

        /* compute the other metrics */
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
      }
    }

    return error;
  }


/* END */
