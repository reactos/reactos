/***************************************************************************/
/*                                                                         */
/*  t1gload.h                                                              */
/*                                                                         */
/*    Type 1 Glyph Loader (specification).                                 */
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


#ifndef T1GLOAD_H
#define T1GLOAD_H


#ifdef FT_FLAT_COMPILE

#include "t1objs.h"

#else

#include <type1/t1objs.h>

#endif


#ifdef __cplusplus
  extern "C" {
#endif


  typedef struct T1_Builder_  T1_Builder;

  typedef FT_Error  (*T1_Builder_EndChar)( T1_Builder*  loader );

  typedef FT_Error  (*T1_Builder_Sbw)( T1_Builder*  loader,
                                       FT_Pos       sbx,
                                       FT_Pos       sby,
                                       FT_Pos       wx,
                                       FT_Pos       wy );

  typedef FT_Error  (*T1_Builder_ClosePath)( T1_Builder*  loader );

  typedef FT_Error  (*T1_Builder_RLineTo)( T1_Builder*  loader,
                                           FT_Pos       dx,
                                           FT_Pos       dy );

  typedef FT_Error  (*T1_Builder_RMoveTo)( T1_Builder*  loader,
                                           FT_Pos       dx,
                                           FT_Pos       dy );

  typedef FT_Error  (*T1_Builder_RCurveTo)( T1_Builder*  loader,
                                            FT_Pos       dx1,
                                            FT_Pos       dy1,
                                            FT_Pos       dx2,
                                            FT_Pos       dy2,
                                            FT_Pos       dx3,
                                            FT_Pos       dy3 );


  /*************************************************************************/
  /*                                                                       */
  /* <Structure>                                                           */
  /*    T1_Builder_Funcs                                                   */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A structure to store the address of various functions used by a    */
  /*    glyph builder to implement the outline's `path construction'.      */
  /*                                                                       */
  typedef struct  T1_Builder_Funcs_
  {
    T1_Builder_EndChar    end_char;
    T1_Builder_Sbw        set_bearing_point;
    T1_Builder_ClosePath  close_path;
    T1_Builder_RLineTo    rline_to;
    T1_Builder_RMoveTo    rmove_to;
    T1_Builder_RCurveTo   rcurve_to;

  } T1_Builder_Funcs;


  /*************************************************************************/
  /*                                                                       */
  /* <Structure>                                                           */
  /*    T1_Builder                                                         */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A structure used during glyph loading to store its outline.        */
  /*                                                                       */
  /* <Fields>                                                              */
  /*    memory       :: The current memory object.                         */
  /*                                                                       */
  /*    face         :: The current face object.                           */
  /*                                                                       */
  /*    size         :: The current size object.                           */
  /*                                                                       */
  /*    glyph        :: The current glyph slot.                            */
  /*                                                                       */
  /*    loader       :: The current glyph loader.                          */
  /*                                                                       */
  /*    current      :: The current glyph outline.                         */
  /*                                                                       */
  /*    base         :: The base glyph outline.                            */
  /*                                                                       */
  /*    last         :: The last point position.                           */
  /*                                                                       */
  /*    scale_x      :: The horizontal scale (FUnits to sub-pixels).       */
  /*                                                                       */
  /*    scale_y      :: The vertical scale (FUnits to sub-pixels).         */
  /*                                                                       */
  /*    pos_x        :: The horizontal translation (for composite glyphs). */
  /*                                                                       */
  /*    pos_y        :: The vertical translation (for composite glyphs).   */
  /*                                                                       */
  /*    left_bearing :: The left side bearing point.                       */
  /*                                                                       */
  /*    advance      :: The horizontal advance vector.                     */
  /*                                                                       */
  /*    no_recurse   ::                                                    */
  /*                                                                       */
  /*    bbox         :: The glyph's bounding box.                          */
  /*                                                                       */
  /*    path_begun   :: A flag which indicates that a new path has begun.  */
  /*                                                                       */
  /*    load_points  :: A flag which indicates, if not set, that no points */
  /*                    are loaded.                                        */
  /*                                                                       */
  /*    pass         :: The pass number for multi-pass hinters.            */
  /*                                                                       */
  /*    hint_point   :: The index of the next point to hint.               */
  /*                                                                       */
  /*    funcs        :: A table of builder functions used to perform the   */
  /*                    outline's path construction.                       */
  /*                                                                       */
  struct  T1_Builder_
  {
    FT_Memory        memory;
    T1_Face          face;
    T1_Size          size;
    T1_GlyphSlot     glyph;
    FT_GlyphLoader*  loader;

    FT_Outline*      current;       /* the current glyph outline   */
    FT_Outline*      base;          /* the composite glyph outline */

    FT_Vector        last;

    FT_Fixed         scale_x;
    FT_Fixed         scale_y;

    FT_Pos           pos_x;
    FT_Pos           pos_y;

    FT_Vector        left_bearing;
    FT_Vector        advance;
    FT_Bool          no_recurse;

    FT_BBox          bbox;          /* bounding box */
    FT_Bool          path_begun;
    FT_Bool          load_points;

    FT_Int           pass;
    FT_Int           hint_point;

    /* path construction function interface */
    T1_Builder_Funcs  funcs;
  };


  typedef FT_Error  (*T1_Hinter_ChangeHints)( T1_Builder*  builder );

  typedef FT_Error  (*T1_Hinter_DotSection)( T1_Builder*  builder );

  typedef FT_Error  (*T1_Hinter_Stem)( T1_Builder*  builder,
                                       FT_Pos       pos,
                                       FT_Pos       width,
                                       FT_Bool      vertical );

  typedef FT_Error  (*T1_Hinter_Stem3)( T1_Builder*  builder,
                                        FT_Pos       pos0,
                                        FT_Pos       width0,
                                        FT_Pos       pos1,
                                        FT_Pos       width1,
                                        FT_Pos       pos2,
                                        FT_Pos       width2,
                                        FT_Bool      vertical );


  /*************************************************************************/
  /*                                                                       */
  /* <Structure>                                                           */
  /*    T1_Hinter_Funcs                                                    */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A structure to store the address of various functions used by a    */
  /*    Type 1 hinter to perform outline hinting.                          */
  /*                                                                       */
  typedef struct  T1_Hinter_Func_
  {
    T1_Hinter_ChangeHints  change_hints;
    T1_Hinter_DotSection   dot_section;
    T1_Hinter_Stem         stem;
    T1_Hinter_Stem3        stem3;

  } T1_Hinter_Funcs;


  typedef enum  T1_Operator_
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

  } T1_Operator;


  /* execution context charstring zone */
  typedef struct  T1_Decoder_Zone_
  {
    FT_Byte*  base;
    FT_Byte*  limit;
    FT_Byte*  cursor;

  } T1_Decoder_Zone;


  typedef struct  T1_Decoder_
  {
    T1_Builder        builder;
    T1_Hinter_Funcs   hinter;

    FT_Int            stack[T1_MAX_CHARSTRINGS_OPERANDS];
    FT_Int*           top;

    T1_Decoder_Zone   zones[T1_MAX_SUBRS_CALLS + 1];
    T1_Decoder_Zone*  zone;

    FT_Int            flex_state;
    FT_Int            num_flex_vectors;
    FT_Vector         flex_vectors[7];

  } T1_Decoder;


  LOCAL_DEF
  void  T1_Init_Builder( T1_Builder*              builder,
                         T1_Face                  face,
                         T1_Size                  size,
                         T1_GlyphSlot             glyph,
                         const T1_Builder_Funcs*  funcs );

  LOCAL_DEF
  void T1_Done_Builder( T1_Builder*  builder );

  LOCAL_DEF
  void  T1_Init_Decoder( T1_Decoder*             decoder,
                         const T1_Hinter_Funcs*  funcs );

  LOCAL_DEF
  FT_Error  T1_Compute_Max_Advance( T1_Face  face,
                                    FT_Int*  max_advance );

  LOCAL_DEF
  FT_Error  T1_Parse_CharStrings( T1_Decoder*  decoder,
                                  FT_Byte*     charstring_base,
                                  FT_Int       charstring_len,
                                  FT_Int       num_subrs,
                                  FT_Byte**    subrs_base,
                                  FT_Int*      subrs_len );

  LOCAL_DEF
  FT_Error  T1_Add_Points( T1_Builder*  builder,
                           FT_Int       num_points );

  LOCAL_DEF
  FT_Error  T1_Add_Contours( T1_Builder*  builder,
                             FT_Int       num_contours );

  LOCAL_DEF
  FT_Error  T1_Load_Glyph( T1_GlyphSlot  glyph,
                           T1_Size       size,
                           FT_Int        glyph_index,
                           FT_Int        load_flags );


#ifdef __cplusplus
  }
#endif


#endif /* T1GLOAD_H */


/* END */
