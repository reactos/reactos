/***************************************************************************/
/*                                                                         */
/*  ttsbit.h                                                               */
/*                                                                         */
/*    TrueType and OpenType embedded bitmap support (specification).       */
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


#ifndef TTSBIT_H
#define TTSBIT_H


#ifdef FT_FLAT_COMPILE

#include "ttload.h"

#else

#include <freetype/src/sfnt/ttload.h>

#endif


#ifdef __cplusplus
  extern "C" {
#endif


  LOCAL_DEF
  FT_Error  TT_Load_SBit_Strikes( TT_Face    face,
                                  FT_Stream  stream );

  LOCAL_DEF
  void  TT_Free_SBit_Strikes( TT_Face  face );

  LOCAL_DEF
  FT_Error  TT_Load_SBit_Image( TT_Face           face,
                                FT_Int            x_ppem,
                                FT_Int            y_ppem,
                                FT_UInt           glyph_index,
                                FT_UInt           load_flags,
                                FT_Stream         stream,
                                FT_Bitmap*        map,
                                TT_SBit_Metrics*  metrics );


#ifdef __cplusplus
  }
#endif


#endif /* TTSBIT_H */


/* END */
