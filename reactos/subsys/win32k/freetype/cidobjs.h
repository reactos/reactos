/***************************************************************************/
/*                                                                         */
/*  cidobjs.h                                                              */
/*                                                                         */
/*    CID objects manager (specification).                                 */
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


#ifndef CIDOBJS_H
#define CIDOBJS_H

#include <freetype/internal/ftobjs.h>
#include <freetype/config/ftconfig.h>
#include <freetype/internal/t1errors.h>
#include <freetype/internal/t1types.h>


#ifdef __cplusplus
  extern "C" {
#endif


  /* The following structures must be defined by the hinter */
  typedef struct CID_Size_Hints_   CID_Size_Hints;
  typedef struct CID_Glyph_Hints_  CID_Glyph_Hints;


  /*************************************************************************/
  /*                                                                       */
  /* <Type>                                                                */
  /*    CID_Driver                                                         */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A handle to a Type 1 driver object.                                */
  /*                                                                       */
  typedef struct CID_DriverRec_*  CID_Driver;


  /*************************************************************************/
  /*                                                                       */
  /* <Type>                                                                */
  /*    CID_Size                                                           */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A handle to a Type 1 size object.                                  */
  /*                                                                       */
  typedef struct CID_SizeRec_*  CID_Size;


  /*************************************************************************/
  /*                                                                       */
  /* <Type>                                                                */
  /*    CID_GlyphSlot                                                      */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A handle to a Type 1 glyph slot object.                            */
  /*                                                                       */
  typedef struct CID_GlyphSlotRec_*  CID_GlyphSlot;


  /*************************************************************************/
  /*                                                                       */
  /* <Type>                                                                */
  /*    CID_CharMap                                                        */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A handle to a Type 1 character mapping object.                     */
  /*                                                                       */
  /* <Note>                                                                */
  /*    The Type 1 format doesn't use a charmap but an encoding table.     */
  /*    The driver is responsible for making up charmap objects            */
  /*    corresponding to these tables.                                     */
  /*                                                                       */
  typedef struct CID_CharMapRec_*  CID_CharMap;


  /*************************************************************************/
  /*                                                                       */
  /* HERE BEGINS THE TYPE 1 SPECIFIC STUFF                                 */
  /*                                                                       */
  /*************************************************************************/


  typedef struct  CID_SizeRec_
  {
    FT_SizeRec  root;
    FT_Bool     valid;

  } CID_SizeRec;


  typedef struct  CID_GlyphSlotRec_
  {
    FT_GlyphSlotRec  root;

    FT_Bool          hint;
    FT_Bool          scaled;

    FT_Fixed         x_scale;
    FT_Fixed         y_scale;

  } CID_GlyphSlotRec;


  LOCAL_DEF
  FT_Error  CID_Init_Face( FT_Stream      stream,
                           CID_Face       face,
                           FT_Int         face_index,
                           FT_Int         num_params,
                           FT_Parameter*  params );

  LOCAL_DEF
  void  CID_Done_Face( CID_Face  face );


  LOCAL_DEF
  FT_Error  CID_Init_Driver( CID_Driver  driver );

  LOCAL_DEF
  void  CID_Done_Driver( CID_Driver  driver );


#ifdef __cplusplus
  }
#endif


#endif /* CIDOBJS_H */


/* END */
