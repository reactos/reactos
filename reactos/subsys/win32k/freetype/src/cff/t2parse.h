/***************************************************************************/
/*                                                                         */
/*  t2parse.h                                                              */
/*                                                                         */
/*    OpenType parser (specification).                                     */
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


#ifndef T2PARSE_H
#define T2PARSE_H

#include <freetype/internal/t2types.h>
#include <freetype/internal/ftobjs.h>

#define T2_MAX_STACK_DEPTH  96

#define T2CODE_TOPDICT  0x1000
#define T2CODE_PRIVATE  0x2000


#ifdef __cplusplus
  extern "C" {
#endif


  typedef struct  T2_Parser_
  {
    FT_Byte*   start;
    FT_Byte*   limit;
    FT_Byte*   cursor;

    FT_Byte*   stack[T2_MAX_STACK_DEPTH + 1];
    FT_Byte**  top;

    FT_UInt    object_code;
    void*      object;

  } T2_Parser;


  LOCAL_DEF
  void  T2_Parser_Init( T2_Parser*  parser,
                        FT_UInt     code,
                        void*       object );

  LOCAL_DEF
  FT_Error  T2_Parser_Run( T2_Parser*  parser,
                           FT_Byte*    start,
                           FT_Byte*    limit );


#ifdef __cplusplus
  }
#endif


#endif /* T2PARSE_H */


/* END */
