/***************************************************************************/
/*                                                                         */
/*  z1afm.h                                                                */
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


#ifndef Z1AFM_H
#define Z1AFM_H


#ifdef FT_FLAT_COMPILE

#include "z1objs.h"

#else

#include <type1z/z1objs.h>

#endif


#ifdef __cplusplus
  extern "C" {
#endif


  typedef struct  Z1_Kern_Pair_
  {
    FT_UInt    glyph1;
    FT_UInt    glyph2;
    FT_Vector  kerning;

  } Z1_Kern_Pair;


  typedef struct  Z1_AFM_
  {
    FT_Int         num_pairs;
    Z1_Kern_Pair*  kern_pairs;

  } Z1_AFM;


  LOCAL_DEF
  FT_Error  Z1_Read_AFM( FT_Face    face,
                         FT_Stream  stream );

  LOCAL_DEF
  void  Z1_Done_AFM( FT_Memory  memory,
                     Z1_AFM*    afm );

  LOCAL_DEF
  void  Z1_Get_Kerning( Z1_AFM*     afm,
                        FT_UInt     glyph1,
                        FT_UInt     glyph2,
                        FT_Vector*  kerning );


#ifdef __cplusplus
  }
#endif


#endif /* Z1AFM_H */


/* END */
