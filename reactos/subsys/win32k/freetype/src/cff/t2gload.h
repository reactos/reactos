/***************************************************************************/
/*                                                                         */
/*  t2gload.h                                                              */
/*                                                                         */
/*    OpenType Glyph Loader (specification).                               */
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


#ifndef T2GLOAD_H
#define T2GLOAD_H

#include <freetype/freetype.h>


#ifdef FT_FLAT_COMPILE

#include "t2objs.h"

#else

#include <cff/t2objs.h>

#endif


#ifdef __cplusplus
  extern "C" {
#endif


#define T2_MAX_OPERANDS     48
#define T2_MAX_SUBRS_CALLS  32


  /*************************************************************************/
  /*                                                                       */
  /* <Structure>                                                           */
  /*    T2_Builder                                                         */
  /*                                                                       */
  /* <Description>                                                         */
  /*     A structure used during glyph loading to store its outline.       */
  /*                                                                       */
  /* <Fields>                                                              */
  /*    memory       :: The current memory object.                         */
  /*                                                                       */
  /*    face         :: The current face object.                           */
  /*                                                                       */
  /*    glyph        :: The current glyph slot.                            */
  /*                                                                       */
  /*    current      :: The current glyph outline.                         */
  /*                                                                       */
  /*    base         :: The base glyph outline.                            */
  /*                                                                       */
  /*    max_points   :: maximum points in builder outline                  */
  /*                                                                       */
  /*    max_contours :: Maximal number of contours in builder outline.     */
  /*                                                                       */
  /*    last         :: The last point position.                           */
  /*                                                                       */
  /*    scale_x      :: The horizontal scale (FUnits to sub-pixels).       */
  /*                                                                       */
  /*    scale_y      :: The vertical scale (FUnits to sub-pixels).         */
  /*                                                                       */
  /*    pos_x        :: The horizontal translation (if composite glyph).   */
  /*                                                                       */
  /*    pos_y        :: The vertical translation (if composite glyph).     */
  /*                                                                       */
  /*    left_bearing :: The left side bearing point.                       */
  /*                                                                       */
  /*    advance      :: The horizontal advance vector.                     */
  /*                                                                       */
  /*    bbox         :: Unused.                                            */
  /*                                                                       */
  /*    path_begun   :: A flag which indicates that a new path has begun.  */
  /*                                                                       */
  /*    load_points  :: If this flag is not set, no points are loaded.     */
  /*                                                                       */
  /*    no_recurse   :: Set but not used.                                  */
  /*                                                                       */
  /*    error        :: An error code that is only used to report memory   */
  /*                    allocation problems.                               */
  /*                                                                       */
  /*    metrics_only :: A boolean indicating that we only want to compute  */
  /*                    the metrics of a given glyph, not load all of its  */
  /*                    points.                                            */
  /*                                                                       */
  typedef struct  T2_Builder_
  {
    FT_Memory        memory;
    TT_Face          face;
    T2_GlyphSlot     glyph;
    FT_GlyphLoader*  loader;
    FT_Outline*      base;
    FT_Outline*      current;

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

  } T2_Builder;


  /* execution context charstring zone */

  typedef struct  T2_Decoder_Zone_
  {
    FT_Byte*  base;
    FT_Byte*  limit;
    FT_Byte*  cursor;

  } T2_Decoder_Zone;


  typedef struct  T2_Decoder_
  {
    T2_Builder        builder;
    CFF_Font*         cff;

    FT_Fixed          stack[T2_MAX_OPERANDS + 1];
    FT_Fixed*         top;

    T2_Decoder_Zone   zones[T2_MAX_SUBRS_CALLS + 1];
    T2_Decoder_Zone*  zone;

    FT_Int            flex_state;
    FT_Int            num_flex_vectors;
    FT_Vector         flex_vectors[7];

    FT_Pos            glyph_width;
    FT_Pos            nominal_width;

    FT_Bool           read_width;
    FT_Int            num_hints;
    FT_Fixed*         buildchar;
    FT_Int            len_buildchar;

    FT_UInt           num_locals;
    FT_UInt           num_globals;

    FT_Int            locals_bias;
    FT_Int            globals_bias;

    FT_Byte**         locals;
    FT_Byte**         globals;

  } T2_Decoder;


  LOCAL_DEF
  void  T2_Init_Decoder( T2_Decoder*   decoder,
                         TT_Face       face,
                         T2_Size       size,
                         T2_GlyphSlot  slot );

  LOCAL_DEF
  void  T2_Prepare_Decoder( T2_Decoder*  decoder,
                            FT_UInt      glyph_index );

#if 0  /* unused until we support pure CFF fonts */

  /* Compute the maximum advance width of a font through quick parsing */
  LOCAL_DEF
  FT_Error  T2_Compute_Max_Advance( TT_Face  face,
                                    FT_Int*  max_advance );

#endif /* 0 */

  LOCAL_DEF
  FT_Error  T2_Parse_CharStrings( T2_Decoder*  decoder,
                                  FT_Byte*     charstring_base,
                                  FT_Int       charstring_len );

  LOCAL_DEF
  FT_Error  T2_Load_Glyph( T2_GlyphSlot  glyph,
                           T2_Size       size,
                           FT_Int        glyph_index,
                           FT_Int        load_flags );


#ifdef __cplusplus
  }
#endif


#endif /* T2GLOAD_H */


/* END */
