/***************************************************************************/
/*                                                                         */
/*  sfobjs.h                                                               */
/*                                                                         */
/*    SFNT object management (specification).                              */
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


#ifndef SFOBJS_H
#define SFOBJS_H

#include <freetype/internal/sfnt.h>
#include <freetype/internal/ftobjs.h>


#ifdef __cplusplus
  extern "C" {
#endif


  LOCAL_DEF
  FT_Error  SFNT_Init_Face( FT_Stream      stream,
                            TT_Face        face,
                            FT_Int         face_index,
                            FT_Int         num_params,
                            FT_Parameter*  params );

  LOCAL_DEF
  FT_Error  SFNT_Load_Face( FT_Stream      stream,
                            TT_Face        face,
                            FT_Int         face_index,
                            FT_Int         num_params,
                            FT_Parameter*  params );

  LOCAL_DEF
  void  SFNT_Done_Face( TT_Face  face );


#ifdef __cplusplus
  }
#endif


#endif /* SFDRIVER_H */


/* END */
