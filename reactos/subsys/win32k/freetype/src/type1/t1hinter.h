/***************************************************************************/
/*                                                                         */
/*  t1hinter.h                                                             */
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


#ifndef T1HINTER_H
#define T1HINTER_H


#ifdef FT_FLAT_COMPILE

#include "t1objs.h"
#include "t1gload.h"

#else

#include <type1/t1objs.h>
#include <type1/t1gload.h>

#endif


#ifdef __cplusplus
  extern "C" {
#endif


  /*************************************************************************/
  /*                                                                       */
  /* <Struct>                                                              */
  /*   T1_Snap_Zone                                                        */
  /*                                                                       */
  /* <Description>                                                         */
  /*   A `snap zone' is used to model either a blue zone or a stem width   */
  /*   at a given character size.  It is made of a minimum and maximum     */
  /*   edge, defined in 26.6 pixels, as well as an `original' and          */
  /*   `scaled' position.                                                  */
  /*                                                                       */
  /*   The position corresponds to the stem width (for stem snap zones)    */
  /*   or to the blue position (for blue zones).                           */
  /*                                                                       */
  /* <Fields>                                                              */
  /*   orus  :: The original position in font units.                       */
  /*                                                                       */
  /*   pix   :: The current position in sub-pixel units.                   */
  /*                                                                       */
  /*   min   :: The minimum boundary in sub-pixel units.                   */
  /*                                                                       */
  /*   max   :: The maximum boundary in sub-pixel units.                   */
  /*                                                                       */
  typedef struct  T1_Snap_Zone_
  {
    FT_Pos  orus;
    FT_Pos  pix;
    FT_Pos  min;
    FT_Pos  max;

  } T1_Snap_Zone;


  /*************************************************************************/
  /*                                                                       */
  /* <Struct>                                                              */
  /*   T1_Edge                                                             */
  /*                                                                       */
  /* <Description>                                                         */
  /*   A very simple structure used to model a stem edge.                  */
  /*                                                                       */
  /* <Fields>                                                              */
  /*   orus  :: The original edge position in font units.                  */
  /*                                                                       */
  /*   pix   :: The scaled edge position in sub-pixel units.               */
  /*                                                                       */
  typedef struct  T1_Edge_
  {
    FT_Pos  orus;
    FT_Pos  pix;

  } T1_Edge;


  /*************************************************************************/
  /*                                                                       */
  /* <Struct>                                                              */
  /*    T1_Stem_Hint                                                       */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A simple structure used to model a stem hint.                      */
  /*                                                                       */
  /* <Fields>                                                              */
  /*    min_edge   :: The hint's minimum edge.                             */
  /*                                                                       */
  /*    max_edge   :: The hint's maximum edge.                             */
  /*                                                                       */
  /*    hint_flags :: Some flags describing the stem properties.           */
  /*                                                                       */
  /* <Note>                                                                */
  /*    The min and max edges of a ghost stem have the same position, even */
  /*    if they are coded in a weird way in the charstrings.               */
  /*                                                                       */
  typedef struct  T1_Stem_Hint_
  {
    T1_Edge  min_edge;
    T1_Edge  max_edge;
    FT_Int   hint_flags;

  } T1_Stem_Hint;


#define T1_HINT_FLAG_ACTIVE      1    /* indicates an active stem */
#define T1_HINT_FLAG_MIN_BORDER  2    /* unused for now           */
#define T1_HINT_FLAG_MAX_BORDER  4    /* unused for now           */

  /* hinter's configuration constants */
#define T1_HINTER_MAX_BLUES  24    /* maximum number of blue zones      */
#define T1_HINTER_MAX_SNAPS  16    /* maximum number of stem snap zones */
#define T1_HINTER_MAX_EDGES  64    /* maximum number of stem hints      */


  /*************************************************************************/
  /*                                                                       */
  /* <Struct>                                                              */
  /*   T1_Size_Hints                                                       */
  /*                                                                       */
  /* <Description>                                                         */
  /*   A structure used to model the hinting information related to a size */
  /*   object.                                                             */
  /*                                                                       */
  /* <Fields>                                                              */
  /*   supress_overshoots :: A boolean flag to tell whether overshoot      */
  /*                         supression should occur.                      */
  /*                                                                       */
  /*   num_blue_zones     :: The total number of blue zones (top+bottom).  */
  /*                                                                       */
  /*   num_bottom_zones   :: The number of bottom zones.                   */
  /*                                                                       */
  /*   blue_zones         :: The blue zones table.  Bottom zones are       */
  /*                         stored first in the table, followed by all    */
  /*                         top zones.                                    */
  /*                                                                       */
  /*   num_snap_widths    :: The number of horizontal stem snap zones.     */
  /*                                                                       */
  /*   snap_widths        :: An array of horizontal stem snap zones.       */
  /*                                                                       */
  /*   num_snap_heights   :: The number of vertical stem snap zones.       */
  /*                                                                       */
  /*   snap_heights       :: An array of vertical stem snap zones.         */
  /*                                                                       */
  struct  T1_Size_Hints_
  {
    FT_Bool       supress_overshoots;

    FT_Int        num_blue_zones;
    FT_Int        num_bottom_zones;
    T1_Snap_Zone  blue_zones[T1_HINTER_MAX_BLUES];

    FT_Int        num_snap_widths;
    T1_Snap_Zone  snap_widths[T1_HINTER_MAX_SNAPS];

    FT_Int        num_snap_heights;
    T1_Snap_Zone  snap_heights[T1_HINTER_MAX_SNAPS];
  };


  /*************************************************************************/
  /*                                                                       */
  /* <Struct>                                                              */
  /*    T1_Stem_Table                                                      */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A simple structure used to model a set of stem hints in a single   */
  /*    direction during the loading of a given glyph outline.  Not all    */
  /*    stem hints are active at a time.  Moreover, stems must be sorted   */
  /*    regularly.                                                         */
  /*                                                                       */
  /* <Fields>                                                              */
  /*    num_stems  :: The total number of stems in the table.              */
  /*                                                                       */
  /*    num_active :: The number of active stems in the table.             */
  /*                                                                       */
  /*    stems      :: A table of all stems.                                */
  /*                                                                       */
  /*    sort       :: A table of indices into the stems table, used to     */
  /*                  keep a sorted list of the active stems.              */
  /*                                                                       */
  typedef struct  T1_Stem_Table_
  {
    FT_Int        num_stems;
    FT_Int        num_active;

    T1_Stem_Hint  stems[T1_HINTER_MAX_EDGES];
    FT_Int        sort [T1_HINTER_MAX_EDGES];

  } T1_Stem_Table;


  /*************************************************************************/
  /*                                                                       */
  /* <Struct>                                                              */
  /*   T1_Glyph_Hints                                                      */
  /*                                                                       */
  /* <Description>                                                         */
  /*   A structure used to model the stem hints of a given glyph outline   */
  /*   during glyph loading.                                               */
  /*                                                                       */
  /* <Fields>                                                              */
  /*   hori_stems :: The horizontal stem hints table.                      */
  /*   vert_stems :: The vertical stem hints table.                        */
  /*                                                                       */
  struct T1_Glyph_Hints_
  {
    T1_Stem_Table  hori_stems;
    T1_Stem_Table  vert_stems;
  };


  /*************************************************************************/
  /*                                                                       */
  /* <Data>                                                                */
  /*    t1_hinter_funcs                                                    */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A table containing the address of various functions used during    */
  /*    the loading of an hinted scaled outline.                           */
  /*                                                                       */
  extern const T1_Hinter_Funcs  t1_hinter_funcs;


  LOCAL_DEF
  FT_Error  T1_New_Size_Hinter( T1_Size  size );

  LOCAL_DEF
  void  T1_Done_Size_Hinter( T1_Size  size );

  LOCAL_DEF
  FT_Error  T1_Reset_Size_Hinter( T1_Size  size );

  LOCAL_DEF
  FT_Error  T1_New_Glyph_Hinter( T1_GlyphSlot  glyph );

  LOCAL_DEF
  void T1_Done_Glyph_Hinter( T1_GlyphSlot  glyph );


  LOCAL_DEF
  void  T1_Hint_Points( T1_Builder*  builder );

  LOCAL_DEF
  void  T1_Hint_Stems( T1_Builder*  builder );


#ifdef __cplusplus
  }
#endif


#endif /* T1HINTER_H */


/* END */
