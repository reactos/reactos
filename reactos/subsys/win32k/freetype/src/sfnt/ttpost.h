/***************************************************************************/
/*                                                                         */
/*  ttpost.h                                                               */
/*                                                                         */
/*    Postcript name table processing for TrueType and OpenType fonts      */
/*    (specification).                                                     */
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


#ifndef TTPOST_H
#define TTPOST_H

#include <freetype/config/ftconfig.h>
#include <freetype/internal/tttypes.h>

#ifdef __cplusplus
  extern "C" {
#endif


#define TT_Err_Invalid_Post_Table_Format  0x0B00
#define TT_Err_Invalid_Post_Table         0x0B01


  LOCAL_DEF
  FT_Error TT_Get_PS_Name( TT_Face      face,
                           FT_UInt      index,
                           FT_String**  PSname );

  LOCAL_DEF
  void  TT_Free_Post_Names( TT_Face  face );


#ifdef __cplusplus
  }
#endif


#endif /* TTPOST_H */


/* END */
