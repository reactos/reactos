/***************************************************************************/
/*                                                                         */
/*  t1objs.h                                                               */
/*                                                                         */
/*    Type 1 objects manager (specification).                              */
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


#ifndef T1OBJS_H
#define T1OBJS_H

#include <freetype/config/ftconfig.h>
#include <freetype/internal/ftobjs.h>
#include <freetype/internal/t1types.h>

#include <freetype/internal/t1errors.h>

#ifdef __cplusplus
  extern "C" {
#endif


  /* The following structures must be defined by the hinter */
  typedef struct T1_Size_Hints_   T1_Size_Hints;
  typedef struct T1_Glyph_Hints_  T1_Glyph_Hints;


  /*************************************************************************/
  /*                                                                       */
  /* <Type>                                                                */
  /*    T1_Driver                                                          */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A handle to a Type 1 driver object.                                */
  /*                                                                       */
  typedef struct T1_DriverRec_*  T1_Driver;


  /*************************************************************************/
  /*                                                                       */
  /* <Type>                                                                */
  /*    T1_Size                                                            */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A handle to a Type 1 size object.                                  */
  /*                                                                       */
  typedef struct T1_SizeRec_*  T1_Size;


  /*************************************************************************/
  /*                                                                       */
  /* <Type>                                                                */
  /*    T1_GlyphSlot                                                       */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A handle to a Type 1 glyph slot object.                            */
  /*                                                                       */
  typedef struct T1_GlyphSlotRec_*  T1_GlyphSlot;


  /*************************************************************************/
  /*                                                                       */
  /* <Type>                                                                */
  /*    T1_CharMap                                                         */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A handle to a Type 1 character mapping object.                     */
  /*                                                                       */
  /* <Note>                                                                */
  /*    The Type 1 format doesn't use a charmap but an encoding table.     */
  /*    The driver is responsible for making up charmap objects            */
  /*    corresponding to these tables.                                     */
  /*                                                                       */
  typedef struct T1_CharMapRec_*  T1_CharMap;


  /*************************************************************************/
  /*                                                                       */
  /*                  HERE BEGINS THE TYPE1 SPECIFIC STUFF                 */
  /*                                                                       */
  /*************************************************************************/


  /*************************************************************************/
  /*                                                                       */
  /* <Type>                                                                */
  /*    T1_SizeRec                                                         */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Type 1 size record.                                                */
  /*                                                                       */
  typedef struct  T1_SizeRec_
  {
    FT_SizeRec      root;
    FT_Bool         valid;
    T1_Size_Hints*  hints;  /* defined in the hinter. This allows */
                            /* us to experiment with different    */
                            /* hinting schemes without having to  */
                            /* change `t1objs' each time.         */
  } T1_SizeRec;


  /*************************************************************************/
  /*                                                                       */
  /* <Type>                                                                */
  /*    T1_GlyphSlotRec                                                    */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Type 1 glyph slot record.                                          */
  /*                                                                       */
  typedef struct  T1_GlyphSlotRec_
  {
    FT_GlyphSlotRec  root;

    FT_Bool          hint;
    FT_Bool          scaled;

    FT_Int           max_points;
    FT_Int           max_contours;

    FT_Fixed         x_scale;
    FT_Fixed         y_scale;

    T1_Glyph_Hints*  hints;  /* defined in the hinter */

  } T1_GlyphSlotRec;


  LOCAL_DEF
  FT_Error  T1_Init_Face( FT_Stream      stream,
                          T1_Face        face,
                          FT_Int         face_index,
                          FT_Int         num_params,
                          FT_Parameter*  params );

  LOCAL_DEF
  void  T1_Done_Face( T1_Face  face );

  LOCAL_DEF
  FT_Error  T1_Init_Size( T1_Size  size );

  LOCAL_DEF
  void  T1_Done_Size( T1_Size  size );

  LOCAL_DEF
  FT_Error  T1_Reset_Size( T1_Size  size );

  LOCAL_DEF
  FT_Error  T1_Init_GlyphSlot( T1_GlyphSlot  slot );

  LOCAL_DEF
  void  T1_Done_GlyphSlot( T1_GlyphSlot  slot );


#ifdef __cplusplus
  }
#endif

#endif /* T1OBJS_H */


/* END */
