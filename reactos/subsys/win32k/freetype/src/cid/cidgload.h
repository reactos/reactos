/***************************************************************************/
/*                                                                         */
/*  cidgload.h                                                             */
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


#ifndef CIDGLOAD_H
#define CIDGLOAD_H


#ifdef FT_FLAT_COMPILE

#include "cidobjs.h"

#else

#include <cid/cidobjs.h>

#endif


#ifdef __cplusplus
  extern "C" {
#endif


  /*************************************************************************/
  /*                                                                       */
  /* <Structure>                                                           */
  /*    CID_Builder                                                        */
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
  typedef struct  CID_Builder_
  {
    FT_Memory        memory;
    CID_Face         face;
    CID_GlyphSlot    glyph;
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

  } CID_Builder;


  /* execution context charstring zone */

  typedef struct  CID_Decoder_Zone_
  {
    FT_Byte*  base;
    FT_Byte*  limit;
    FT_Byte*  cursor;

  } CID_Decoder_Zone;


  typedef struct  CID_Decoder_
  {
    CID_Builder        builder;

    FT_Int             stack[T1_MAX_CHARSTRINGS_OPERANDS];
    FT_Int*            top;

    CID_Decoder_Zone   zones[T1_MAX_SUBRS_CALLS + 1];
    CID_Decoder_Zone*  zone;

    FT_Matrix          font_matrix;
    CID_Subrs*         subrs;
    FT_UInt            lenIV;

    FT_Int             flex_state;
    FT_Int             num_flex_vectors;
    FT_Vector          flex_vectors[7];

  } CID_Decoder;


  LOCAL_DEF
  void  CID_Init_Builder( CID_Builder*   builder,
                          CID_Face       face,
                          CID_Size       size,
                          CID_GlyphSlot  glyph );

  LOCAL_DEF
  void CID_Done_Builder( CID_Builder*  builder );


  LOCAL_DEF
  void CID_Init_Decoder( CID_Decoder*  decoder );


#if 0

  /* Compute the maximum advance width of a font through quick parsing */
  LOCAL_DEF
  FT_Error  CID_Compute_Max_Advance( CID_Face  face,
                                     FT_Int*   max_advance );

#endif

  /* This function is exported, because it is used by the T1Dump utility */
  LOCAL_DEF
  FT_Error  CID_Parse_CharStrings( CID_Decoder*  decoder,
                                   FT_Byte*      charstring_base,
                                   FT_Int        charstring_len );

  LOCAL_DEF
  FT_Error  CID_Load_Glyph( CID_GlyphSlot  glyph,
                            CID_Size       size,
                            FT_Int         glyph_index,
                            FT_Int         load_flags );


#ifdef __cplusplus
  }
#endif


#endif /* CIDGLOAD_H */


/* END */
