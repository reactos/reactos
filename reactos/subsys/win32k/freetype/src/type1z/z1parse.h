/***************************************************************************/
/*                                                                         */
/*  z1parse.h                                                              */
/*                                                                         */
/*    Experimental Type 1 parser (specification).                          */
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


#ifndef Z1PARSE_H
#define Z1PARSE_H

#include <freetype/internal/t1types.h>

#ifdef __cplusplus
  extern "C" {
#endif


  /* simple enumeration type used to identify token types */
  typedef enum  Z1_Token_Type_
  {
    t1_token_none = 0,
    t1_token_any,
    t1_token_string,
    t1_token_array,

    /* do not remove */
    t1_token_max

  } Z1_Token_Type;


  /* a simple structure used to identify tokens */
  typedef struct  Z1_Token_Rec_
  {
    FT_Byte*       start;   /* first character of token in input stream */
    FT_Byte*       limit;   /* first character after the token          */
    Z1_Token_Type  type;    /* type of token..                          */

  } Z1_Token_Rec;


  /* enumeration type used to identify object fields */
  typedef enum  Z1_Field_Type_
  {
    t1_field_none = 0,
    t1_field_bool,
    t1_field_integer,
    t1_field_fixed,
    t1_field_string,
    t1_field_integer_array,
    t1_field_fixed_array,

    /* do not remove */
    t1_field_max

  } Z1_Field_Type;


  /* structure type used to model object fields */
  typedef struct  Z1_Field_Rec_
  {
    Z1_Field_Type  type;          /* type of field                        */
    FT_UInt        offset;        /* offset of field in object            */
    FT_UInt        size;          /* size of field in bytes               */
    FT_UInt        array_max;     /* maximum number of elements for array */
    FT_UInt        count_offset;  /* offset of element count for arrays   */
    FT_Int         flag_bit;      /* bit number for field flag            */

  } Z1_Field_Rec;


#define Z1_FIELD_REF( s, f )  ( ((s*)0)->f )

#define Z1_FIELD_BOOL( _ftype, _fname )                      \
          {                                                  \
            t1_field_bool,                                   \
            (FT_UInt)(char*)&Z1_FIELD_REF( _ftype, _fname ), \
            sizeof ( Z1_FIELD_REF( _ftype, _fname ) ),       \
            0, 0, 0                                          \
          }

#define Z1_FIELD_NUM( _ftype, _fname )                       \
          {                                                  \
            t1_field_integer,                                \
            (FT_UInt)(char*)&Z1_FIELD_REF( _ftype, _fname ), \
            sizeof ( Z1_FIELD_REF( _ftype, _fname ) ),       \
            0, 0, 0                                          \
          }

#define Z1_FIELD_FIXED( _ftype, _fname, _power )             \
          {                                                  \
            t1_field_fixed,                                  \
            (FT_UInt)(char*)&Z1_FIELD_REF( _ftype, _fname ), \
            sizeof ( Z1_FIELD_REF( _ftype, _fname ) ),       \
            0, 0, 0                                          \
          }

#define Z1_FIELD_STRING( _ftype, _fname )                    \
          {                                                  \
            t1_field_string,                                 \
            (FT_UInt)(char*)&Z1_FIELD_REF( _ftype, _fname ), \
            sizeof ( Z1_FIELD_REF( _ftype, _fname ) ),       \
            0, 0, 0                                          \
          }

#define Z1_FIELD_NUM_ARRAY( _ftype, _fname, _fcount, _fmax )  \
          {                                                   \
            t1_field_integer,                                 \
            (FT_UInt)(char*)&Z1_FIELD_REF( _ftype, _fname ),  \
            sizeof ( Z1_FIELD_REF( _ftype, _fname )[0] ),     \
            _fmax,                                            \
            (FT_UInt)(char*)&Z1_FIELD_REF( _ftype, _fcount ), \
            0                                                 \
          }

#define Z1_FIELD_FIXED_ARRAY( _ftype, _fname, _fcount, _fmax ) \
          {                                                    \
            t1_field_fixed,                                    \
            (FT_UInt)(char*)&Z1_FIELD_REF( _ftype, _fname ),   \
            sizeof ( Z1_FIELD_REF( _ftype, _fname )[0] ),      \
            _fmax,                                             \
            (FT_UInt)(char*)&Z1_FIELD_REF( _ftype, _fcount ),  \
            0                                                  \
          }

#define Z1_FIELD_NUM_ARRAY2( _ftype, _fname, _fmax )         \
          {                                                  \
            t1_field_integer,                                \
            (FT_UInt)(char*)&Z1_FIELD_REF( _ftype, _fname ), \
            sizeof ( Z1_FIELD_REF( _ftype, _fname )[0] ),    \
            _fmax,                                           \
            0, 0                                             \
          }

#define Z1_FIELD_FIXED_ARRAY2( _ftype, _fname, _fmax )       \
          {                                                  \
            t1_field_fixed,                                  \
            (FT_UInt)(char*)&Z1_FIELD_REF( _ftype, _fname ), \
            sizeof ( Z1_FIELD_REF( _ftype, _fname )[0] ),    \
            _fmax,                                           \
            0, 0                                             \
          }


  /*************************************************************************/
  /*                                                                       */
  /* <Struct>                                                              */
  /*    Z1_Table                                                           */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A Z1_Table is a simple object used to store an array of objects in */
  /*    a single memory block.                                             */
  /*                                                                       */
  /* <Fields>                                                              */
  /*    block     :: The address in memory of the growheap's block.  This  */
  /*                 can change between two object adds, due to the use of */
  /*                 reallocation.                                         */
  /*                                                                       */
  /*    cursor    :: The current top of the grow heap within its block.    */
  /*                                                                       */
  /*    capacity  :: The current size of the heap block.  Increments in    */
  /*                 1kByte blocks.                                        */
  /*                                                                       */
  /*    init      :: A boolean.  Set when the table has been initialized   */
  /*                 (the table user should set this field).               */
  /*                                                                       */
  /*    max_elems :: The maximum number of elements in the table.          */
  /*                                                                       */
  /*    num_elems :: The current number of elements in the table.          */
  /*                                                                       */
  /*    elements  :: A table of element addresses within the block.        */
  /*                                                                       */
  /*    lengths   :: A table of element sizes within the block.            */
  /*                                                                       */
  /*    memory    :: The memory object used for memory operations          */
  /*                 (allocation/reallocation).                            */
  /*                                                                       */
  typedef struct  Z1_Table_
  {
    FT_Byte*   block;          /* current memory block           */
    FT_Int     cursor;         /* current cursor in memory block */
    FT_Int     capacity;       /* current size of memory block   */
    FT_Long    init;

    FT_Int     max_elems;
    FT_Int     num_elems;
    FT_Byte**  elements;       /* addresses of table elements */
    FT_Int*    lengths;        /* lengths of table elements   */

    FT_Memory  memory;

  } Z1_Table;


  /*************************************************************************/
  /*                                                                       */
  /* <Struct>                                                              */
  /*    Z1_Parser                                                          */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A Z1_Parser is an object used to parse a Type 1 fonts very         */
  /*    quickly.                                                           */
  /*                                                                       */
  /* <Fields>                                                              */
  /*    stream       :: The current input stream.                          */
  /*                                                                       */
  /*    memory       :: The current memory object.                         */
  /*                                                                       */
  /*    base_dict    :: A pointer to the top-level dictionary.             */
  /*                                                                       */
  /*    base_len     :: The length in bytes of the top dictionary.         */
  /*                                                                       */
  /*    private_dict :: A pointer to the private dictionary.               */
  /*                                                                       */
  /*    private_len  :: The length in bytes of the private dictionary.     */
  /*                                                                       */
  /*    in_pfb       :: A boolean.  Indicates that we are handling a PFB   */
  /*                    file.                                              */
  /*                                                                       */
  /*    in_memory    :: A boolean.  Indicates a memory-based stream.       */
  /*                                                                       */
  /*    single_block :: A boolean.  Indicates that the private dictionary  */
  /*                    is stored in lieu of the base dictionary.          */
  /*                                                                       */
  /*    cursor       :: The current parser cursor.                         */
  /*                                                                       */
  /*    limit        :: The current parser limit (first byte after the     */
  /*                    current dictionary).                               */
  /*                                                                       */
  /*    error        :: The current parsing error.                         */
  /*                                                                       */
  typedef struct  Z1_Parser_
  {
    FT_Stream  stream;
    FT_Memory  memory;

    FT_Byte*   base_dict;
    FT_Int     base_len;

    FT_Byte*   private_dict;
    FT_Int     private_len;

    FT_Byte    in_pfb;
    FT_Byte    in_memory;
    FT_Byte    single_block;

    FT_Byte*   cursor;
    FT_Byte*   limit;
    FT_Error   error;

  } Z1_Parser;


  LOCAL_DEF
  FT_Error  Z1_New_Table( Z1_Table*  table,
                          FT_Int     count,
                          FT_Memory  memory );


  LOCAL_DEF
  FT_Error  Z1_Add_Table( Z1_Table*  table,
                          FT_Int     index,
                          void*      object,
                          FT_Int     length );

#if 0
  LOCAL_DEF
  void  Z1_Done_Table( Z1_Table*  table );
#endif

  LOCAL_DEF
  void  Z1_Release_Table( Z1_Table*  table );

  LOCAL_DEF
  FT_Long  Z1_ToInt( Z1_Parser*  parser );

  LOCAL_DEF
  FT_Long  Z1_ToFixed( Z1_Parser*  parser,
                       FT_Int      power_ten );

  LOCAL_DEF
  FT_Int  Z1_ToCoordArray( Z1_Parser*  parser,
                           FT_Int      max_coords,
                           FT_Short*   coords );

  LOCAL_DEF
  FT_Int  Z1_ToFixedArray( Z1_Parser*  parser,
                           FT_Int      max_values,
                           FT_Fixed*   values,
                           FT_Int      power_ten );

#if 0
  LOCAL_DEF
  FT_String*  Z1_ToString( Z1_Parser*  parser );

  LOCAL_DEF
  FT_Bool  Z1_ToBool( Z1_Parser*  parser );
#endif


  LOCAL_DEF
  void  Z1_Skip_Spaces( Z1_Parser*  parser );

  LOCAL_DEF
  void  Z1_ToToken( Z1_Parser*     parser,
                    Z1_Token_Rec*  token );

  LOCAL_FUNC
  void  Z1_ToTokenArray( Z1_Parser*     parser,
                         Z1_Token_Rec*  tokens,
                         FT_UInt        max_tokens,
                         FT_Int*        pnum_tokens );

  LOCAL_DEF
  FT_Error  Z1_Load_Field( Z1_Parser*           parser,
                           const Z1_Field_Rec*  field,
                           void**               objects,
                           FT_UInt              max_objects,
                           FT_ULong*            pflags );

  LOCAL_DEF
  FT_Error  Z1_Load_Field_Table( Z1_Parser*           parser,
                                 const Z1_Field_Rec*  field,
                                 void**               objects,
                                 FT_UInt              max_objects,
                                 FT_ULong*            pflags );


  LOCAL_DEF
  FT_Error  Z1_New_Parser( Z1_Parser*  parser,
                           FT_Stream   stream,
                           FT_Memory   memory );

  LOCAL_DEF
  FT_Error  Z1_Get_Private_Dict( Z1_Parser*  parser );

  LOCAL_DEF
  void  Z1_Decrypt( FT_Byte*   buffer,
                    FT_Int     length,
                    FT_UShort  seed );

  LOCAL_DEF
  void  Z1_Done_Parser( Z1_Parser*  parser );

#ifdef __cplusplus
  }
#endif

#endif /* Z1PARSE_H */


/* END */
