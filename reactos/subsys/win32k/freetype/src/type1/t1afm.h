/***************************************************************************/
/*                                                                         */
/*  t1afm.h                                                                */
/*                                                                         */
/*    AFM support for Type 1 fonts (specification).                        */
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


#ifndef T1AFM_H
#define T1AFM_H

#include <freetype/internal/ftobjs.h>


#ifdef __cplusplus
  extern "C" {
#endif


  typedef struct  T1_Kern_Pair_
  {
    FT_UInt    glyph1;
    FT_UInt    glyph2;
    FT_Vector  kerning;

  } T1_Kern_Pair;


  typedef struct  T1_AFM_
  {
    FT_Int         num_pairs;
    T1_Kern_Pair*  kern_pairs;

  } T1_AFM;


  LOCAL_DEF
  FT_Error  T1_Read_AFM( FT_Face    face,
                         FT_Stream  stream );

  LOCAL_DEF
  void  T1_Done_AFM( FT_Memory  memory,
                     T1_AFM*    afm );

  LOCAL_DEF
  void  T1_Get_Kerning( T1_AFM*     afm,
                        FT_UInt     glyph1,
                        FT_UInt     glyph2,
                        FT_Vector*  kerning );


#ifdef __cplusplus
  }
#endif


#endif /* T1AFM_H */


/* END */
