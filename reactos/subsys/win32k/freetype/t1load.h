/***************************************************************************/
/*                                                                         */
/*  t1load.h                                                               */
/*                                                                         */
/*    Type 1 font loader (specification).                                  */
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


#ifndef T1LOAD_H
#define T1LOAD_H

#include <freetype/internal/ftstream.h>


#ifdef FT_FLAT_COMPILE

#include "t1parse.h"

#else

#include <type1/t1parse.h>

#endif


#ifdef __cplusplus
  extern "C" {
#endif

  LOCAL_DEF
  void  Init_T1_Parser( T1_Parser*    parser,
                        T1_Face       face,
                        T1_Tokenizer  tokenizer );


  LOCAL_DEF
  FT_Error  Parse_T1_FontProgram( T1_Parser*  parser );


#ifdef __cplusplus
  }
#endif

#endif /* T1LOAD_H */


/* END */
