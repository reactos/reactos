/***************************************************************************/
/*                                                                         */
/*  cidload.h                                                              */
/*                                                                         */
/*    CID-keyed Type1 font loader (specification).                         */
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


#ifndef CIDLOAD_H
#define CIDLOAD_H

#include <freetype/internal/ftstream.h>


#ifdef FT_FLAT_COMPILE

#include "cidparse.h"

#else

#include <freetype/src/cid/cidparse.h>

#endif


#ifdef __cplusplus
  extern "C" {
#endif


  typedef struct  CID_Loader_
  {
    CID_Parser  parser;          /* parser used to read the stream */
    FT_Int      num_chars;       /* number of characters in encoding */

  } CID_Loader;


  LOCAL_DEF
  FT_Long  cid_get_offset( FT_Byte**  start,
                           FT_Byte    offsize );

  LOCAL_DEF
  void  cid_decrypt( FT_Byte*   buffer,
                     FT_Int     length,
                     FT_UShort  seed );

  LOCAL_DEF
  FT_Error  CID_Open_Face( CID_Face  face );


#ifdef __cplusplus
  }
#endif


#endif /* CIDLOAD_H */


/* END */
