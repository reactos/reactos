/***************************************************************************/
/*                                                                         */
/*  ttpload.h                                                              */
/*                                                                         */
/*    TrueType glyph data/program tables loader (specification).           */
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


#ifndef TTPLOAD_H
#define TTPLOAD_H

#include <freetype/internal/tttypes.h>


#ifdef __cplusplus
  extern "C" {
#endif


  LOCAL_DEF
  FT_Error  TT_Load_Locations( TT_Face    face,
                               FT_Stream  stream );

  LOCAL_DEF
  FT_Error  TT_Load_CVT( TT_Face    face,
                         FT_Stream  stream );

  LOCAL_DEF
  FT_Error  TT_Load_Programs( TT_Face    face,
                              FT_Stream  stream );


#ifdef __cplusplus
  }
#endif


#endif /* TTPLOAD_H */


/* END */
