/***************************************************************************/
/*                                                                         */
/*  t1parse.h                                                              */
/*                                                                         */
/*    Type 1 parser (specification).                                       */
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


  /*************************************************************************/
  /*                                                                       */
  /* The Type1 parser component is in charge of simply parsing the font    */
  /* input stream and convert simple tokens and elements into integers,    */
  /* floats, matrices, strings, etc.                                       */
  /*                                                                       */
  /* It is used by the Type1 loader.                                       */
  /*                                                                       */
  /*************************************************************************/


#ifndef T1PARSE_H
#define T1PARSE_H

#include <freetype/internal/ftstream.h>


#ifdef FT_FLAT_COMPILE

#include "t1tokens.h"

#else

#include <type1/t1tokens.h>

#endif


#ifdef __cplusplus
  extern "C" {
#endif


  /*************************************************************************/
  /*                                                                       */
  /* <Enum>                                                                */
  /*    T1_DictState                                                       */
  /*                                                                       */
  /* <Description>                                                         */
  /*    An enumeration used to describe the Type 1 parser's state, i.e.    */
  /*    which dictionary (or array) it is scanning and processing at the   */
  /*    current moment.                                                    */
  /*                                                                       */
  typedef enum  T1_DictState_
  {
    dict_none = 0,
    dict_font,          /* parsing the font dictionary              */
    dict_fontinfo,      /* parsing the font info dictionary         */
    dict_none2,         /* beginning to parse the encrypted section */
    dict_private,       /* parsing the private dictionary           */
    dict_encoding,      /* parsing the encoding array               */
    dict_subrs,         /* parsing the subrs array                  */
    dict_othersubrs,    /* parsing the othersubrs array (?)         */
    dict_charstrings,   /* parsing the charstrings dictionary       */
    dict_unknown_array, /* parsing/ignoring an unknown array        */
    dict_unknown_dict,  /* parsing/ignoring an unknown dictionary   */

    dict_max    /* do not remove from list */

  } T1_DictState;


  /*************************************************************************/
  /*                                                                       */
  /* <Struct>                                                              */
  /*    T1_Table                                                           */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A T1_Table is a simple object used to store an array of objects in */
  /*    a single memory block.                                             */
  /*                                                                       */
  /* <Fields>                                                              */
  /*    block     :: The address in memory of the growheap's block.  This  */
  /*                 can change between two object adds, due to            */
  /*                 reallocation.                                         */
  /*                                                                       */
  /*    cursor    :: The current top of the grow heap within its block.    */
  /*                                                                       */
  /*    capacity  :: The current size of the heap block.  Increments by    */
  /*                 1kByte chunks.                                        */
  /*                                                                       */
  /*    max_elems :: The maximum number of elements in table.              */
  /*                                                                       */
  /*    num_elems :: The current number of elements in table.              */
  /*                                                                       */
  /*    elements  :: A table of element addresses within the block.        */
  /*                                                                       */
  /*    lengths   :: A table of element sizes within the block.            */
  /*                                                                       */
  /*    memory    :: The object used for memory operations                 */
  /*                 (alloc/realloc).                                      */
  /*                                                                       */
  typedef struct  T1_Table_
  {
    FT_Byte*   block;          /* current memory block           */
    FT_Int     cursor;         /* current cursor in memory block */
    FT_Int     capacity;       /* current size of memory block   */

    FT_Int     max_elems;
    FT_Int     num_elems;
    FT_Byte**  elements;       /* addresses of table elements */
    FT_Int*    lengths;        /* lengths of table elements   */

    FT_Memory  memory;

  } T1_Table;


  /*************************************************************************/
  /*                                                                       */
  /* <Struct>                                                              */
  /*    T1_Parser                                                          */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A Type 1 parser.  This object is in charge of parsing Type 1 ASCII */
  /*    streams and builds dictionaries for a T1_Face object.              */
  /*                                                                       */
  /* <Fields>                                                              */
  /*    error            :: The current error code.  0 means success.      */
  /*                                                                       */
  /*    face             :: The target T1_Face object being built.         */
  /*                                                                       */
  /*    tokenizer        :: The tokenizer (lexical analyser) used for      */
  /*                        processing the input stream.                   */
  /*                                                                       */
  /*    dump_tokens      :: XXX                                            */
  /*                                                                       */
  /*    stack            :: The current token stack.  Note that we don't   */
  /*                        use intermediate Postscript objects here!      */
  /*                                                                       */
  /*    top              :: The current top of token stack.                */
  /*                                                                       */
  /*    limit            :: The current upper bound of the token stack.    */
  /*                        Used for overflow checks.                      */
  /*                                                                       */
  /*    args             :: The arguments of a given operator.  Used and   */
  /*                        increased by the various CopyXXX() functions.  */
  /*                                                                       */
  /*    state_index      :: The index of the top of the dictionary state   */
  /*                        stack.                                         */
  /*                                                                       */
  /*    state_stack      :: The dictionary states stack.                   */
  /*                                                                       */
  /*    table            :: A T1_Table object used to record various kinds */
  /*                        of dictionaries or arrays (like `/Encoding',   */
  /*                        `/Subrs', `/CharStrings').                     */
  /*                                                                       */
  /*    cur_name         :: XXX                                            */
  /*                                                                       */
  /*    encoding_type    :: XXX                                            */
  /*                                                                       */
  /*    encoding_names   :: XXX                                            */
  /*                                                                       */
  /*    encoding_lengths :: XXX                                            */
  /*                                                                       */
  /*    encoding_offsets :: XXX                                            */
  /*                                                                       */
  /*    subrs            :: XXX                                            */
  /*                                                                       */
  /*    charstrings      :: XXX                                            */
  /*                                                                       */
  typedef  struct  T1_Parser_
  {
    FT_Error         error;
    T1_Face          face;

    T1_Tokenizer     tokenizer;
    FT_Bool          dump_tokens;

    T1_Token         stack[T1_MAX_STACK_DEPTH];
    T1_Token*        top;
    T1_Token*        limit;
    T1_Token*        args;

    FT_Int           state_index;
    T1_DictState     state_stack[T1_MAX_DICT_DEPTH];

	T1_Table         table;

	FT_Int           cur_name;

	T1_EncodingType  encoding_type;
    FT_Byte*         encoding_names;
    FT_Int*          encoding_lengths;
    FT_Byte**        encoding_offsets;

    FT_Byte*         subrs;
    FT_Byte*         charstrings;

  } T1_Parser;


  LOCAL_DEF
  FT_Error  T1_New_Table( T1_Table*  table,
                          FT_Int     count,
                          FT_Memory  memory );

  LOCAL_DEF
  FT_Error  T1_Add_Table( T1_Table*  table,
                          FT_Int     index,
                          void*      object,
                          FT_Int     length );

  LOCAL_DEF
  void  T1_Done_Table( T1_Table*  table );


  LOCAL_DEF
  FT_String*  CopyString( T1_Parser*  parser );

  LOCAL_DEF
  FT_Long  CopyInteger( T1_Parser*  parser );

  LOCAL_DEF
  FT_Bool  CopyBoolean( T1_Parser*  parser );

  LOCAL_DEF
  FT_Long  CopyFloat( T1_Parser*  parser,
                      FT_Int      scale );

  LOCAL_DEF
  void  CopyBBox( T1_Parser*  parser,
                  FT_BBox*    bbox );

  LOCAL_DEF
  void  CopyMatrix( T1_Parser*  parser,
                    FT_Matrix*  matrix );

  LOCAL_DEF
  void  CopyArray( T1_Parser*  parser,
                   FT_Byte*    num_elements,
                   FT_Short*   elements,
                   FT_Int      max_elements );


#ifdef __cplusplus
  }
#endif

#endif /* T1PARSE_H */


/* END */
