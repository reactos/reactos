/***************************************************************************/
/*                                                                         */
/*  cidparse.h                                                             */
/*                                                                         */
/*    CID-keyed Type1 parser (specification).                              */
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


#ifndef CIDPARSE_H
#define CIDPARSE_H

#include <freetype/internal/t1types.h>


#ifdef __cplusplus
  extern "C" {
#endif


#if 0

  /*************************************************************************/
  /*                                                                       */
  /* <Struct>                                                              */
  /*    CID_Table                                                          */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A CID_Table is a simple object used to store an array of objects   */
  /*    in a single memory block.                                          */
  /*                                                                       */
  /* <Fields>                                                              */
  /*    block     :: The address in memory of the growheap's block.  This  */
  /*                 can change between two object adds, due to the use    */
  /*                 of `realloc()'.                                       */
  /*                                                                       */
  /*    cursor    :: The current top of the growheap within its block.     */
  /*                                                                       */
  /*    capacity  :: The current size of the heap block.  Increments by    */
  /*                 blocks of 1 kByte.                                    */
  /*                                                                       */
  /*    init      :: A boolean.  Set when the table has been initialized   */
  /*                 (the table user should set this field).               */
  /*                                                                       */
  /*    max_elems :: The maximal number of elements in the table.          */
  /*                                                                       */
  /*    num_elems :: The current number of elements (in use) in the table. */
  /*                                                                       */
  /*    elements  :: A table of element addresses within the block.        */
  /*                                                                       */
  /*    lengths   :: A table of element sizes within the block.            */
  /*                                                                       */
  /*    memory    :: The memory object used for memory operations          */
  /*                 (allocation resp. reallocation).                      */
  /*                                                                       */
  typedef struct  CID_Table_
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

  } CID_Table;


  LOCAL_DEF
  FT_Error  CID_New_Table( CID_Table*  table,
                           FT_Int      count,
                           CID_Memory  memory );

  LOCAL_DEF
  FT_Error  CID_Add_Table( CID_Table*  table,
                           FT_Int      index,
                           void*       object,
                           FT_Int      length );

  LOCAL_DEF
  void  CID_Release_Table( CID_Table*  table );

#endif /* 0 */


  /*************************************************************************/
  /*                                                                       */
  /* <Struct>                                                              */
  /*    CID_Parser                                                         */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A CID_Parser is an object used to parse a Type 1 fonts very        */
  /*    quickly.                                                           */
  /*                                                                       */
  /* <Fields>                                                              */
  /*    stream         :: The current input stream.                        */
  /*                                                                       */
  /*    memory         :: The current memory object.                       */
  /*                                                                       */
  /*    postscript     :: A pointer to the data to be parsed.              */
  /*                                                                       */
  /*    postscript_len :: The length of the data to be parsed.             */
  /*                                                                       */
  /*    data_offset    :: The start position of the binary data (i.e., the */
  /*                      end of the data to be parsed.                    */
  /*                                                                       */
  /*    cursor         :: The current parser cursor.                       */
  /*                                                                       */
  /*    limit          :: The current parser limit (i.e., the first byte   */
  /*                      after the current dictionary).                   */
  /*                                                                       */
  /*    error          :: The current parsing error.                       */
  /*                                                                       */
  /*    cid            :: A structure which holds the information about    */
  /*                      the current font.                                */
  /*                                                                       */
  /*    num_dict       :: The number of font dictionaries.                 */
  /*                                                                       */
  typedef struct CID_Parser_
  {
    FT_Stream  stream;
    FT_Memory  memory;

    FT_Byte*   postscript;
    FT_Int     postscript_len;

    FT_ULong   data_offset;

    FT_Byte*   cursor;
    FT_Byte*   limit;
    FT_Error   error;

    CID_Info*  cid;
    FT_Int     num_dict;

  } CID_Parser;


  LOCAL_DEF
  FT_Error  CID_New_Parser( CID_Parser*  parser,
                            FT_Stream    stream,
                            FT_Memory    memory );

  LOCAL_DEF
  void  CID_Done_Parser( CID_Parser*  parser );


  /*************************************************************************/
  /*                                                                       */
  /*                            PARSING ROUTINES                           */
  /*                                                                       */
  /*************************************************************************/

  LOCAL_DEF
  FT_Long  CID_ToInt( CID_Parser*  parser );

  LOCAL_DEF
  FT_Int  CID_ToCoordArray( CID_Parser*  parser,
                            FT_Int       max_coords,
                            FT_Short*    coords );

  LOCAL_DEF
  FT_Int  CID_ToFixedArray( CID_Parser*  parser,
                            FT_Int       max_values,
                            FT_Fixed*    values,
                            FT_Int       power_ten );

  LOCAL_DEF
  void  CID_Skip_Spaces( CID_Parser*  parser );


  /* simple enumeration type used to identify token types */
  typedef enum  CID_Token_Type_
  {
    t1_token_none = 0,
    t1_token_any,
    t1_token_string,
    t1_token_array,

    /* do not remove */
    t1_token_max

  } CID_Token_Type;


  /* a simple structure used to identify tokens */
  typedef struct  CID_Token_Rec_
  {
    FT_Byte*        start;   /* first character of token in input stream */
    FT_Byte*        limit;   /* first character after the token          */
    CID_Token_Type  type;    /* type of token                            */

  } CID_Token_Rec;


  LOCAL_DEF
  void  CID_ToToken( CID_Parser*     parser,
                     CID_Token_Rec*  token );


  /* enumeration type used to identify object fields */
  typedef enum  CID_Field_Type_
  {
    t1_field_none = 0,
    t1_field_bool,
    t1_field_integer,
    t1_field_fixed,
    t1_field_string,
    t1_field_integer_array,
    t1_field_fixed_array,
    t1_field_callback,

    /* do not remove */
    t1_field_max

  } CID_Field_Type;

  typedef enum  CID_Field_Location_
  {
    t1_field_cid_info,
    t1_field_font_dict,
    t1_field_font_info,
    t1_field_private,

    /* do not remove */
    t1_field_location_max

  } CID_Field_Location;


  typedef FT_Error  (*CID_Field_Parser)( CID_Face     face,
                                         CID_Parser*  parser );

  /* structure type used to model object fields */
  typedef struct  CID_Field_Rec_
  {
    const char*         ident;        /* field identifier                  */
    CID_Field_Location  location;
    CID_Field_Type      type;         /* type of field                     */
    CID_Field_Parser    reader;
    FT_UInt             offset;       /* offset of field in object         */
    FT_UInt             size;         /* size of field in bytes            */
    FT_UInt             array_max;    /* maximal number of elements for    */
                                      /* array                             */
    FT_UInt             count_offset; /* offset of element count for       */
                                      /* arrays                            */
  } CID_Field_Rec;


#define CID_FIELD_REF( s, f )  ( ((s*)0)->f )

#define CID_NEW_SIMPLE_FIELD( _ident, _type, _fname )         \
          {                                                   \
            _ident, T1CODE, _type,                            \
            0,                                                \
            (FT_UInt)(char*)&CID_FIELD_REF( T1TYPE, _fname ), \
            sizeof ( CID_FIELD_REF( T1TYPE, _fname ) ),       \
            0, 0                                              \
          },

#define CID_NEW_CALLBACK_FIELD( _ident, _reader ) \
          {                                       \
            _ident, T1CODE, t1_field_callback,    \
            _reader,                              \
            0, 0,                                 \
            0, 0                                  \
          },

#define CID_NEW_TABLE_FIELD( _ident, _type, _fname, _max )           \
          {                                                          \
            _ident, T1CODE, _type,                                   \
            0,                                                       \
            (FT_UInt)(char*)&CID_FIELD_REF( T1TYPE, _fname ),        \
            sizeof ( CID_FIELD_REF( T1TYPE, _fname )[0] ),           \
            _max,                                                    \
            (FT_UInt)(char*)&CID_FIELD_REF( T1TYPE, num_ ## _fname ) \
          },

#define CID_NEW_TABLE_FIELD2( _ident, _type, _fname, _max )   \
          {                                                   \
            _ident, T1CODE, _type,                            \
            0,                                                \
            (FT_UInt)(char*)&CID_FIELD_REF( T1TYPE, _fname ), \
            sizeof ( CID_FIELD_REF( T1TYPE, _fname )[0] ),    \
            _max, 0                                           \
          },


#define CID_FIELD_BOOL( _ident, _fname )                           \
          CID_NEW_SIMPLE_FIELD( _ident, t1_field_bool, _fname )

#define CID_FIELD_NUM( _ident, _fname )                            \
          CID_NEW_SIMPLE_FIELD( _ident, t1_field_integer, _fname )

#define CID_FIELD_FIXED( _ident, _fname )                          \
          CID_NEW_SIMPLE_FIELD( _ident, t1_field_fixed, _fname )

#define CID_FIELD_STRING( _ident, _fname )                         \
          CID_NEW_SIMPLE_FIELD( _ident, t1_field_string, _fname )

#define CID_FIELD_NUM_TABLE( _ident, _fname, _fmax )               \
          CID_NEW_TABLE_FIELD( _ident, t1_field_integer_array,     \
                               _fname, _fmax )

#define CID_FIELD_FIXED_TABLE( _ident, _fname, _fmax )             \
          CID_NEW_TABLE_FIELD( _ident, t1_field_fixed_array,       \
                               _fname, _fmax )

#define CID_FIELD_NUM_TABLE2( _ident, _fname, _fmax )              \
          CID_NEW_TABLE_FIELD2( _ident, t1_field_integer_array,    \
                                _fname, _fmax )

#define CID_FIELD_FIXED_TABLE2( _ident, _fname, _fmax )            \
          CID_NEW_TABLE_FIELD2( _ident, t1_field_fixed_array,      \
                                _fname, _fmax )

#define CID_FIELD_CALLBACK( _ident, _name )                        \
          CID_NEW_CALLBACK_FIELD( _ident, parse_ ## _name )


  LOCAL_DEF
  FT_Error  CID_Load_Field( CID_Parser*           parser,
                            const CID_Field_Rec*  field,
                            void*                 object );

  LOCAL_DEF
  FT_Error  CID_Load_Field_Table( CID_Parser*           parser,
                                  const CID_Field_Rec*  field,
                                  void*                 object );


#ifdef __cplusplus
  }
#endif


#endif /* CIDPARSE_H */


/* END */
