/***************************************************************************/
/*                                                                         */
/*  z1gload.h                                                              */
/*                                                                         */
/*    Experimental Type 1 Glyph Loader (specification).                    */
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


#ifndef Z1GLOAD_H
#define Z1GLOAD_H


#ifdef FT_FLAT_COMPILE

#include "z1objs.h"

#else

#include <type1z/z1objs.h>

#endif


#ifdef __cplusplus
  extern "C" {
#endif


  /*************************************************************************/
  /*                                                                       */
  /* <Structure>                                                           */
  /*    Z1_Builder                                                         */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A structure used during glyph loading to store its outline.        */
  /*                                                                       */
  /* <Fields>                                                              */
  /*    memory       :: The current memory object.                         */
  /*                                                                       */
  /*    face         :: The current face object.                           */
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
  /*    error        :: The current error code.                            */
  /*                                                                       */
  /*    metrics_only :: A flag whether to compute metrics only.            */
  /*                                                                       */
  typedef struct  Z1_Builder_
  {
    FT_Memory        memory;
    T1_Face          face;
    Z1_GlyphSlot     glyph;
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

    FT_BBox          bbox;          /* bounding box */
    FT_Bool          path_begun;
    FT_Bool          load_points;
    FT_Bool          no_recurse;

    FT_Error         error;         /* only used for memory errors */
    FT_Bool          metrics_only;

  } Z1_Builder;


  /* execution context charstring zone */
  typedef struct  Z1_Decoder_Zone_
  {
    FT_Byte*  base;
    FT_Byte*  limit;
    FT_Byte*  cursor;

  } Z1_Decoder_Zone;


  typedef struct  Z1_Decoder_
  {
    Z1_Builder        builder;

    FT_Int            stack[T1_MAX_CHARSTRINGS_OPERANDS];
    FT_Int*           top;

    Z1_Decoder_Zone   zones[T1_MAX_SUBRS_CALLS + 1];
    Z1_Decoder_Zone*  zone;

    FT_Int            flex_state;
    FT_Int            num_flex_vectors;
    FT_Vector         flex_vectors[7];

    T1_Blend*         blend;  /* for multiple masters */

  } Z1_Decoder;


  LOCAL_DEF
  void  Z1_Init_Builder( Z1_Builder*   builder,
                         T1_Face       face,
                         Z1_Size       size,
                         Z1_GlyphSlot  glyph );

  LOCAL_DEF
  void  Z1_Done_Builder( Z1_Builder*  builder );

  LOCAL_DEF
  void  Z1_Init_Decoder( Z1_Decoder*  decoder );

  LOCAL_DEF
  FT_Error  Z1_Compute_Max_Advance( T1_Face  face,
                                    FT_Int*  max_advance );

  LOCAL_DEF
  FT_Error   Z1_Parse_CharStrings( Z1_Decoder*  decoder,
                                   FT_Byte*     charstring_base,
                                   FT_Int       charstring_len,
                                   FT_Int       num_subrs,
                                   FT_Byte**    subrs_base,
                                   FT_Int*      subrs_len );

  LOCAL_DEF
  FT_Error  Z1_Load_Glyph( Z1_GlyphSlot  glyph,
                           Z1_Size       size,
                           FT_Int        glyph_index,
                           FT_Int        load_flags );


#ifdef __cplusplus
  }
#endif


#endif /* Z1GLOAD_H */


/* END */
