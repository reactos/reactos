/***************************************************************************/
/*                                                                         */
/*  t2objs.h                                                               */
/*                                                                         */
/*    OpenType objects manager (specification).                            */
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


#ifndef T2OBJS_H
#define T2OBJS_H


#include <freetype/internal/ftobjs.h>
#include <freetype/internal/t2types.h>
#include <freetype/internal/t2errors.h>
#include <freetype/internal/psnames.h>

#ifdef __cplusplus
  extern "C" {
#endif


  /*************************************************************************/
  /*                                                                       */
  /* <Type>                                                                */
  /*    T2_Driver                                                          */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A handle to an OpenType driver object.                             */
  /*                                                                       */
  typedef struct T2_DriverRec_*  T2_Driver;

  typedef TT_Face  T2_Face;


  /*************************************************************************/
  /*                                                                       */
  /* <Type>                                                                */
  /*    T2_Size                                                            */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A handle to an OpenType size object.                               */
  /*                                                                       */
  typedef FT_Size  T2_Size;


  /*************************************************************************/
  /*                                                                       */
  /* <Type>                                                                */
  /*    T2_GlyphSlot                                                       */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A handle to an OpenType glyph slot object.                         */
  /*                                                                       */
  typedef struct T2_GlyphSlotRec_
  {
    FT_GlyphSlotRec  root;

    FT_Bool          hint;
    FT_Bool          scaled;

    FT_Fixed         x_scale;
    FT_Fixed         y_scale;

  } T2_GlyphSlotRec, *T2_GlyphSlot;



  /*************************************************************************/
  /*                                                                       */
  /* Subglyph transformation record.                                       */
  /*                                                                       */
  typedef struct  T2_Transform_
  {
    FT_Fixed    xx, xy;     /* transformation matrix coefficients */
    FT_Fixed    yx, yy;
    FT_F26Dot6  ox, oy;     /* offsets        */

  } T2_Transform;


  /* this is only used in the case of a pure CFF font with no charmap */
  typedef struct  T2_CharMapRec_
  {
    TT_CharMapRec  root;
    PS_Unicodes    unicodes;

  } T2_CharMapRec, *T2_CharMap;


  /***********************************************************************/
  /*                                                                     */
  /* TrueType driver class.                                              */
  /*                                                                     */
  typedef struct  T2_DriverRec_
  {
    FT_DriverRec  root;

    void*         extension_component;

  } T2_DriverRec;


  /*************************************************************************/
  /*                                                                       */
  /* Face functions                                                        */
  /*                                                                       */
  LOCAL_DEF
  FT_Error  T2_Init_Face( FT_Stream      stream,
                          T2_Face        face,
                          FT_Int         face_index,
                          FT_Int         num_params,
                          FT_Parameter*  params );

  LOCAL_DEF
  void  T2_Done_Face( T2_Face  face );


  /*************************************************************************/
  /*                                                                       */
  /* Driver functions                                                      */
  /*                                                                       */
  LOCAL_DEF
  FT_Error  T2_Init_Driver( T2_Driver  driver );

  LOCAL_DEF
  void  T2_Done_Driver( T2_Driver  driver );


#ifdef __cplusplus
  }
#endif


#endif /* T2OBJS_H */


/* END */
