/***************************************************************************/
/*                                                                         */
/*  ahtypes.h                                                              */
/*                                                                         */
/*    General types and definitions for the auto-hint module               */
/*    (specification only).                                                */
/*                                                                         */
/*  Copyright 2000-2001, 2002, 2003, 2004 Catharon Productions Inc.        */
/*  Author: David Turner                                                   */
/*                                                                         */
/*  This file is part of the Catharon Typography Project and shall only    */
/*  be used, modified, and distributed under the terms of the Catharon     */
/*  Open Source License that should come with this file under the name     */
/*  `CatharonLicense.txt'.  By continuing to use, modify, or distribute    */
/*  this file you indicate that you have read the license and              */
/*  understand and accept it fully.                                        */
/*                                                                         */
/*  Note that this license is compatible with the FreeType license.        */
/*                                                                         */
/***************************************************************************/


#ifndef __AHTYPES_H__
#define __AHTYPES_H__


#include <ft2build.h>
#include FT_INTERNAL_OBJECTS_H

#ifdef DEBUG_HINTER
#include <../src/autohint/ahloader.h>
#else
#include "ahloader.h"
#endif


#define xxAH_DEBUG


#ifdef AH_DEBUG

#include <stdio.h>
#define AH_LOG( x )  printf ## x

#else

#define AH_LOG( x )  do ; while ( 0 ) /* nothing */

#endif /* AH_DEBUG */


FT_BEGIN_HEADER


  /*************************************************************************/
  /*************************************************************************/
  /*************************************************************************/
  /****                                                                 ****/
  /**** COMPILE-TIME BUILD OPTIONS                                      ****/
  /****                                                                 ****/
  /**** Toggle these configuration macros to experiment with `features' ****/
  /**** of the auto-hinter.                                             ****/
  /****                                                                 ****/
  /*************************************************************************/
  /*************************************************************************/
  /*************************************************************************/


  /*************************************************************************/
  /*                                                                       */
  /* If this option is defined, only strong interpolation will be used to  */
  /* place the points between edges.  Otherwise, `smooth' points are       */
  /* detected and later hinted through weak interpolation to correct some  */
  /* unpleasant artefacts.                                                 */
  /*                                                                       */
#undef AH_OPTION_NO_WEAK_INTERPOLATION


  /*************************************************************************/
  /*                                                                       */
  /* Undefine this macro if you don't want to hint the metrics.  There is  */
  /* no reason to do this (at least for non-CJK scripts), except for       */
  /* experimentation.                                                      */
  /*                                                                       */
#undef  AH_HINT_METRICS


  /*************************************************************************/
  /*                                                                       */
  /* Define this macro if you do not want to insert extra edges at a       */
  /* glyph's x and y extremum (if there isn't one already available).      */
  /* This helps to reduce a number of artefacts and allows hinting of      */
  /* metrics.                                                              */
  /*                                                                       */
#undef AH_OPTION_NO_EXTREMUM_EDGES


  /* don't touch for now */
#define AH_MAX_WIDTHS   12
#define AH_MAX_HEIGHTS  12


  /*************************************************************************/
  /*************************************************************************/
  /*************************************************************************/
  /****                                                                 ****/
  /****   TYPE DEFINITIONS                                              ****/
  /****                                                                 ****/
  /*************************************************************************/
  /*************************************************************************/
  /*************************************************************************/


  /* see ahangles.h */
  typedef FT_Int  AH_Angle;


  /* hint flags */
#define AH_FLAG_NONE       0

  /* bezier control points flags */
#define AH_FLAG_CONIC                 1
#define AH_FLAG_CUBIC                 2
#define AH_FLAG_CONTROL               ( AH_FLAG_CONIC | AH_FLAG_CUBIC )

  /* extrema flags */
#define AH_FLAG_EXTREMA_X             4
#define AH_FLAG_EXTREMA_Y             8

  /* roundness */
#define AH_FLAG_ROUND_X              16
#define AH_FLAG_ROUND_Y              32

  /* touched */
#define AH_FLAG_TOUCH_X              64
#define AH_FLAG_TOUCH_Y             128

  /* weak interpolation */
#define AH_FLAG_WEAK_INTERPOLATION  256
#define AH_FLAG_INFLECTION          512

  typedef FT_Int AH_Flags;


  /* edge hint flags */
#define AH_EDGE_NORMAL  0
#define AH_EDGE_ROUND   1
#define AH_EDGE_SERIF   2
#define AH_EDGE_DONE    4

  typedef FT_Int  AH_Edge_Flags;


  /* hint directions -- the values are computed so that two vectors are */
  /* in opposite directions iff `dir1+dir2 == 0'                        */
#define AH_DIR_NONE    4
#define AH_DIR_RIGHT   1
#define AH_DIR_LEFT   -1
#define AH_DIR_UP      2
#define AH_DIR_DOWN   -2

  typedef FT_Int  AH_Direction;


  typedef struct AH_PointRec_*    AH_Point;
  typedef struct AH_SegmentRec_*  AH_Segment;
  typedef struct AH_EdgeRec_*     AH_Edge;


  /*************************************************************************/
  /*                                                                       */
  /* <Struct>                                                              */
  /*    AH_PointRec                                                        */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A structure used to model an outline point to the AH_OutlineRec    */
  /*    type.                                                              */
  /*                                                                       */
  /* <Fields>                                                              */
  /*    flags     :: The current point hint flags.                         */
  /*                                                                       */
  /*    ox, oy    :: The current original scaled coordinates.              */
  /*                                                                       */
  /*    fx, fy    :: The current coordinates in font units.                */
  /*                                                                       */
  /*    x,  y     :: The current hinted coordinates.                       */
  /*                                                                       */
  /*    u, v      :: Point coordinates -- meaning varies with context.     */
  /*                                                                       */
  /*    in_dir    :: The direction of the inwards vector (prev->point).    */
  /*                                                                       */
  /*    out_dir   :: The direction of the outwards vector (point->next).   */
  /*                                                                       */
  /*    next      :: The next point in same contour.                       */
  /*                                                                       */
  /*    prev      :: The previous point in same contour.                   */
  /*                                                                       */
  typedef struct  AH_PointRec_
  {
    AH_Flags      flags;    /* point flags used by hinter */
    FT_Pos        ox, oy;
    FT_Pos        fx, fy;
    FT_Pos        x,  y;
    FT_Pos        u,  v;

    AH_Direction  in_dir;   /* direction of inwards vector  */
    AH_Direction  out_dir;  /* direction of outwards vector */

    AH_Point      next;     /* next point in contour     */
    AH_Point      prev;     /* previous point in contour */

  } AH_PointRec;


  /*************************************************************************/
  /*                                                                       */
  /* <Struct>                                                              */
  /*    AH_SegmentRec                                                      */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A structure used to describe an edge segment to the auto-hinter.   */
  /*    A segment is simply a sequence of successive points located on the */
  /*    same horizontal or vertical `position', in a given direction.      */
  /*                                                                       */
  /* <Fields>                                                              */
  /*    flags      :: The segment edge flags (straight, rounded, etc.).    */
  /*                                                                       */
  /*    dir        :: The segment direction.                               */
  /*                                                                       */
  /*    min_coord  :: The minimum coordinate of the segment.               */
  /*                                                                       */
  /*    max_coord  :: The maximum coordinate of the segment.               */
  /*                                                                       */
  /*    edge       :: The edge of the current segment.                     */
  /*                                                                       */
  /*    edge_next  :: The next segment on the same edge.                   */
  /*                                                                       */
  /*    link       :: The pairing segment for this edge.                   */
  /*                                                                       */
  /*    serif      :: The primary segment for serifs.                      */
  /*                                                                       */
  /*    num_linked :: The number of other segments that link to this one.  */
  /*                                                                       */
  /*    score      :: Used to score the segment when selecting them.       */
  /*                                                                       */
  /*    first      :: The first point in the segment.                      */
  /*                                                                       */
  /*    last       :: The last point in the segment.                       */
  /*                                                                       */
  /*    contour    :: A pointer to the first point of the segment's        */
  /*                  contour.                                             */
  /*                                                                       */
  typedef struct  AH_SegmentRec_
  {
    AH_Edge_Flags  flags;
    AH_Direction   dir;
    FT_Pos         pos;         /* position of segment           */
    FT_Pos         min_coord;   /* minimum coordinate of segment */
    FT_Pos         max_coord;   /* maximum coordinate of segment */

    AH_Edge        edge;
    AH_Segment     edge_next;

    AH_Segment     link;        /* link segment               */
    AH_Segment     serif;       /* primary segment for serifs */
    FT_Pos         num_linked;  /* number of linked segments  */
    FT_Pos         score;

    AH_Point       first;       /* first point in edge segment             */
    AH_Point       last;        /* last point in edge segment              */
    AH_Point*      contour;     /* ptr to first point of segment's contour */

  } AH_SegmentRec;


  /*************************************************************************/
  /*                                                                       */
  /* <Struct>                                                              */
  /*    AH_EdgeRec                                                         */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A structure used to describe an edge, which really is a horizontal */
  /*    or vertical coordinate to be hinted depending on the segments      */
  /*    located on it.                                                     */
  /*                                                                       */
  /* <Fields>                                                              */
  /*    fpos       :: The original edge position in font units.            */
  /*                                                                       */
  /*    opos       :: The original scaled edge position.                   */
  /*                                                                       */
  /*    pos        :: The hinted edge position.                            */
  /*                                                                       */
  /*    flags      :: The segment edge flags (straight, rounded, etc.).    */
  /*                                                                       */
  /*    dir        :: The main segment direction on this edge.             */
  /*                                                                       */
  /*    scale      :: Scaling factor between original and hinted edge      */
  /*                  positions.                                           */
  /*                                                                       */
  /*    blue_edge  :: Indicate the blue zone edge this edge is related to. */
  /*                  Only set for some of the horizontal edges in a latin */
  /*                  font.                                                */
  /*                                                                       */
  /*    link       :: The linked edge.                                     */
  /*                                                                       */
  /*    serif      :: The serif edge.                                      */
  /*                                                                       */
  /*    num_linked :: The number of other edges that pair to this one.     */
  /*                                                                       */
  /*    score      :: Used to score the edge when selecting them.          */
  /*                                                                       */
  /*    first      :: The first edge segment.                              */
  /*                                                                       */
  /*    last       :: The last edge segment.                               */
  /*                                                                       */
  typedef struct  AH_EdgeRec_
  {
    FT_Pos         fpos;
    FT_Pos         opos;
    FT_Pos         pos;

    AH_Edge_Flags  flags;
    AH_Direction   dir;
    FT_Fixed       scale;
    FT_Pos*        blue_edge;

    AH_Edge        link;
    AH_Edge        serif;
    FT_Int         num_linked;

    FT_Int         score;

    AH_Segment     first;
    AH_Segment     last;


  } AH_EdgeRec;


  /* an outline as seen by the hinter */
  typedef struct  AH_OutlineRec_
  {
    FT_Memory     memory;

    AH_Direction  vert_major_dir;   /* vertical major direction   */
    AH_Direction  horz_major_dir;   /* horizontal major direction */

    FT_Fixed      x_scale;
    FT_Fixed      y_scale;
    FT_Pos        edge_distance_threshold;

    FT_Int        max_points;
    FT_Int        num_points;
    AH_Point      points;

    FT_Int        max_contours;
    FT_Int        num_contours;
    AH_Point*     contours;

    FT_Int        num_hedges;
    AH_Edge       horz_edges;

    FT_Int        num_vedges;
    AH_Edge       vert_edges;

    FT_Int        num_hsegments;
    AH_Segment    horz_segments;

    FT_Int        num_vsegments;
    AH_Segment    vert_segments;

  } AH_OutlineRec, *AH_Outline;


#ifdef FT_CONFIG_CHESTER_SMALL_F

#define AH_BLUE_CAPITAL_TOP     0                              /* THEZOCQS */
#define AH_BLUE_CAPITAL_BOTTOM  ( AH_BLUE_CAPITAL_TOP + 1 )    /* HEZLOCUS */
#define AH_BLUE_SMALL_F_TOP     ( AH_BLUE_CAPITAL_BOTTOM + 1 ) /* fijkdbh  */
#define AH_BLUE_SMALL_TOP       ( AH_BLUE_SMALL_F_TOP + 1 )    /* xzroesc  */
#define AH_BLUE_SMALL_BOTTOM    ( AH_BLUE_SMALL_TOP + 1 )      /* xzroesc  */
#define AH_BLUE_SMALL_MINOR     ( AH_BLUE_SMALL_BOTTOM + 1 )   /* pqgjy    */
#define AH_BLUE_MAX             ( AH_BLUE_SMALL_MINOR + 1 )

#else /* !FT_CONFIG_CHESTER_SMALL_F */

#define AH_BLUE_CAPITAL_TOP     0                              /* THEZOCQS */
#define AH_BLUE_CAPITAL_BOTTOM  ( AH_BLUE_CAPITAL_TOP + 1 )    /* HEZLOCUS */
#define AH_BLUE_SMALL_TOP       ( AH_BLUE_CAPITAL_BOTTOM + 1 ) /* xzroesc  */
#define AH_BLUE_SMALL_BOTTOM    ( AH_BLUE_SMALL_TOP + 1 )      /* xzroesc  */
#define AH_BLUE_SMALL_MINOR     ( AH_BLUE_SMALL_BOTTOM + 1 )   /* pqgjy    */
#define AH_BLUE_MAX             ( AH_BLUE_SMALL_MINOR + 1 )

#endif /* !FT_CONFIG_CHESTER_SMALL_F */

  typedef FT_Int  AH_Blue;


#define AH_HINTER_MONOCHROME  1
#define AH_HINTER_OPTIMIZE    2

  typedef FT_Int  AH_Hinter_Flags;


  /*************************************************************************/
  /*                                                                       */
  /* <Struct>                                                              */
  /*    AH_GlobalsRec                                                      */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Holds the global metrics for a given font face (be it in design    */
  /*    units or scaled pixel values).                                     */
  /*                                                                       */
  /* <Fields>                                                              */
  /*    num_widths  :: The number of widths.                               */
  /*                                                                       */
  /*    num_heights :: The number of heights.                              */
  /*                                                                       */
  /*    stds        :: A two-element array giving the default stem width   */
  /*                   and height.                                         */
  /*                                                                       */
  /*    widths      :: Snap widths, including standard one.                */
  /*                                                                       */
  /*    heights     :: Snap height, including standard one.                */
  /*                                                                       */
  /*    blue_refs   :: The reference positions of blue zones.              */
  /*                                                                       */
  /*    blue_shoots :: The overshoot positions of blue zones.              */
  /*                                                                       */
  typedef struct  AH_GlobalsRec_
  {
    FT_Int  num_widths;
    FT_Int  num_heights;

    FT_Pos  stds[2];

    FT_Pos  widths [AH_MAX_WIDTHS];
    FT_Pos  heights[AH_MAX_HEIGHTS];

    FT_Pos  blue_refs  [AH_BLUE_MAX];
    FT_Pos  blue_shoots[AH_BLUE_MAX];

  } AH_GlobalsRec, *AH_Globals;


  /*************************************************************************/
  /*                                                                       */
  /* <Struct>                                                              */
  /*    AH_Face_GlobalsRec                                                 */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Holds the complete global metrics for a given font face (i.e., the */
  /*    design units version + a scaled version + the current scales       */
  /*    used).                                                             */
  /*                                                                       */
  /* <Fields>                                                              */
  /*    face    :: A handle to the source face object                      */
  /*                                                                       */
  /*    design  :: The globals in font design units.                       */
  /*                                                                       */
  /*    scaled  :: Scaled globals in sub-pixel values.                     */
  /*                                                                       */
  /*    x_scale :: The current horizontal scale.                           */
  /*                                                                       */
  /*    y_scale :: The current vertical scale.                             */
  /*                                                                       */
  /*    control_overshoot ::                                               */
  /*               Currently unused.                                       */
  /*                                                                       */
  typedef struct  AH_Face_GlobalsRec_
  {
    FT_Face        face;
    AH_GlobalsRec  design;
    AH_GlobalsRec  scaled;
    FT_Fixed       x_scale;
    FT_Fixed       y_scale;
    FT_Bool        control_overshoot;

  } AH_Face_GlobalsRec, *AH_Face_Globals;


  typedef struct  AH_HinterRec_
  {
    FT_Memory        memory;
    AH_Hinter_Flags  flags;

    FT_Int           algorithm;
    FT_Face          face;

    AH_Face_Globals  globals;

    AH_Outline       glyph;

    AH_Loader        loader;
    FT_Vector        pp1;               /* horizontal phantom points */
    FT_Vector        pp2;
    /* we ignore vertical phantom points */

    FT_Bool          transformed;
    FT_Vector        trans_delta;
    FT_Matrix        trans_matrix;

    FT_Bool          do_horz_hints;     /* disable X hinting            */
    FT_Bool          do_vert_hints;     /* disable Y hinting            */
    FT_Bool          do_horz_snapping;  /* disable X stem size snapping */
    FT_Bool          do_vert_snapping;  /* disable Y stem size snapping */
    FT_Bool          do_stem_adjust;    /* disable light stem snapping  */

  } AH_HinterRec, *AH_Hinter;


#ifdef DEBUG_HINTER
  extern AH_Hinter  ah_debug_hinter;
  extern FT_Bool    ah_debug_disable_horz;
  extern FT_Bool    ah_debug_disable_vert;
#else
#define ah_debug_disable_horz  0
#define ah_debug_disable_vert  0
#endif /* DEBUG_HINTER */


FT_END_HEADER

#endif /* __AHTYPES_H__ */


/* END */
