/***************************************************************************/
/*                                                                         */
/*  z1objs.h                                                               */
/*                                                                         */
/*    Experimental Type 1 objects manager (specification).                 */
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


#ifndef Z1OBJS_H
#define Z1OBJS_H

#include <freetype/internal/ftobjs.h>
#include <freetype/config/ftconfig.h>
#include <freetype/internal/t1errors.h>
#include <freetype/internal/t1types.h>

#ifdef __cplusplus
  extern "C" {
#endif

  /* The following structures must be defined by the hinter */
  typedef struct Z1_Size_Hints_   Z1_Size_Hints;
  typedef struct Z1_Glyph_Hints_  Z1_Glyph_Hints;


  /*************************************************************************/
  /*                                                                       */
  /* <Type>                                                                */
  /*    Z1_Driver                                                          */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A handle to a Type 1 driver object.                                */
  /*                                                                       */
  typedef struct Z1_DriverRec_   *Z1_Driver;


  /*************************************************************************/
  /*                                                                       */
  /* <Type>                                                                */
  /*    Z1_Size                                                            */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A handle to a Type 1 size object.                                  */
  /*                                                                       */
  typedef struct Z1_SizeRec_*  Z1_Size;


  /*************************************************************************/
  /*                                                                       */
  /* <Type>                                                                */
  /*    Z1_GlyphSlot                                                       */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A handle to a Type 1 glyph slot object.                            */
  /*                                                                       */
  typedef struct Z1_GlyphSlotRec_*  Z1_GlyphSlot;


  /*************************************************************************/
  /*                                                                       */
  /* <Type>                                                                */
  /*    Z1_CharMap                                                         */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A handle to a Type 1 character mapping object.                     */
  /*                                                                       */
  /* <Note>                                                                */
  /*    The Type 1 format doesn't use a charmap but an encoding table.     */
  /*    The driver is responsible for making up charmap objects            */
  /*    corresponding to these tables.                                     */
  /*                                                                       */
  typedef struct Z1_CharMapRec_*   Z1_CharMap;


  /*************************************************************************/
  /*                                                                       */
  /*                  HERE BEGINS THE TYPE1 SPECIFIC STUFF                 */
  /*                                                                       */
  /*************************************************************************/


  /*************************************************************************/
  /*                                                                       */
  /* <Type>                                                                */
  /*    Z1_SizeRec                                                         */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Type 1 size record.                                                */
  /*                                                                       */
  typedef struct  Z1_SizeRec_
  {
    FT_SizeRec      root;
    FT_Bool         valid;
    Z1_Size_Hints*  hints;  /* defined in the hinter. This allows */
                            /* us to experiment with different    */
                            /* hinting schemes without having to  */
                            /* change `z1objs' each time.         */
  } Z1_SizeRec;


  /*************************************************************************/
  /*                                                                       */
  /* <Type>                                                                */
  /*    Z1_GlyphSlotRec                                                    */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Type 1 glyph slot record.                                          */
  /*                                                                       */
  typedef struct  Z1_GlyphSlotRec_
  {
    FT_GlyphSlotRec  root;

    FT_Bool          hint;
    FT_Bool          scaled;

    FT_Int           max_points;
    FT_Int           max_contours;

    FT_Fixed         x_scale;
    FT_Fixed         y_scale;

    Z1_Glyph_Hints*  hints;  /* defined in the hinter */

  } Z1_GlyphSlotRec;


  LOCAL_DEF
  FT_Error  Z1_Init_Face( FT_Stream      stream,
                          T1_Face        face,
                          FT_Int         face_index,
                          FT_Int         num_params,
                          FT_Parameter*  params );

  LOCAL_DEF
  void  Z1_Done_Face( T1_Face  face );

  LOCAL_DEF
  FT_Error  Z1_Init_Driver( Z1_Driver  driver );

  LOCAL_DEF
  void  Z1_Done_Driver( Z1_Driver  driver );


#ifdef __cplusplus
  }
#endif

#endif /* Z1OBJS_H */


/* END */
