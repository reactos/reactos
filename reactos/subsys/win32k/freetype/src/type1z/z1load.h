/***************************************************************************/
/*                                                                         */
/*  z1load.h                                                               */
/*                                                                         */
/*    Experimental Type 1 font loader (specification).                     */
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


#ifndef Z1LOAD_H
#define Z1LOAD_H

#include <freetype/internal/ftstream.h>
#include <freetype/internal/t1types.h>
#include <freetype/ftmm.h>


#ifdef FT_FLAT_COMPILE

#include "z1parse.h"

#else

#include <type1z/z1parse.h>

#endif


#ifdef __cplusplus
  extern "C" {
#endif

  typedef struct  Z1_Loader_
  {
    Z1_Parser  parser;          /* parser used to read the stream */

    FT_Int     num_chars;       /* number of characters in encoding */
    Z1_Table   encoding_table;  /* Z1_Table used to store the       */
                                /* encoding character names         */

    FT_Int     num_glyphs;
    Z1_Table   glyph_names;
    Z1_Table   charstrings;

    FT_Int     num_subrs;
    Z1_Table   subrs;
    FT_Bool    fontdata;

  } Z1_Loader;


  LOCAL_DEF
  FT_Error  Z1_Open_Face( T1_Face  face );

#ifndef Z1_CONFIG_OPTION_NO_MM_SUPPORT

  LOCAL_DEF
  FT_Error  Z1_Get_Multi_Master( T1_Face           face,
                                 FT_Multi_Master*  master );

  LOCAL_DEF
  FT_Error  Z1_Set_MM_Blend( T1_Face    face,
                             FT_UInt    num_coords,
                             FT_Fixed*  coords );

  LOCAL_DEF
  FT_Error  Z1_Set_MM_Design( T1_Face   face,
                              FT_UInt   num_coords,
                              FT_Long*  coords );

  LOCAL_DEF
  void  Z1_Done_Blend( T1_Face  face );

#endif /* !Z1_CONFIG_OPTION_NO_MM_SUPPORT */


#ifdef __cplusplus
  }
#endif

#endif /* Z1LOAD_H */


/* END */
