/***************************************************************************/
/*                                                                         */
/*  cidparse.c                                                             */
/*                                                                         */
/*    CID-keyed Type1 parser (body).                                       */
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


#include <freetype/internal/ftdebug.h>
#include <freetype/internal/ftcalc.h>
#include <freetype/internal/ftobjs.h>
#include <freetype/internal/ftstream.h>
#include <freetype/internal/t1errors.h>


#ifdef FT_FLAT_COMPILE

#include "cidparse.h"

#else

#include <freetype/src/cid/cidparse.h>

#endif


#include <string.h>     /* for strncmp() */


  /*************************************************************************/
  /*                                                                       */
  /* The macro FT_COMPONENT is used in trace mode.  It is an implicit      */
  /* parameter of the FT_TRACE() and FT_ERROR() macros, used to print/log  */
  /* messages during execution.                                            */
  /*                                                                       */
#undef FT_COMPONENT
#define FT_COMPONENT  trace_cidparse


#if 0

  /*************************************************************************/
  /*************************************************************************/
  /*************************************************************************/
  /*****                                                               *****/
  /*****           IMPLEMENTATION OF CID_TABLE OBJECT                  *****/
  /*****                                                               *****/
  /*************************************************************************/
  /*************************************************************************/
  /*************************************************************************/


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    CID_New_Table                                                      */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Initializes a CID_Table.                                           */
  /*                                                                       */
  /* <InOut>                                                               */
  /*    table  :: The address of the target table.                         */
  /*                                                                       */
  /* <Input>                                                               */
  /*    count  :: The table size, i.e., the maximal number of elements.    */
  /*                                                                       */
  /*    memory :: The memory object to be used for all subsequent          */
  /*              reallocations.                                           */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  LOCAL_FUNC
  FT_Error  CID_New_Table( CID_Table*  table,
                           FT_Int      count,
                           FT_Memory   memory )
  {
    FT_Error  error;


    table->memory = memory;
    if ( ALLOC_ARRAY( table->elements, count, FT_Byte*  ) ||
         ALLOC_ARRAY( table->lengths, count, FT_Byte* )   )
      goto Exit;

    table->max_elems = count;
    table->init      = 0xDEADBEEFL;
    table->num_elems = 0;
    table->block     = 0;
    table->capacity  = 0;
    table->cursor    = 0;

  Exit:
    if ( error )
      FREE( table->elements );

    return error;
  }


  static
  void  shift_elements( CID_Table*  table,
                        FT_Byte*    old_base )
  {
    FT_Long    delta  = table->block - old_base;
    FT_Byte**  offset = table->elements;
    FT_Byte**  limit  = offset + table->max_elems;


    if ( delta )
      for ( ; offset < limit; offset++ )
      {
        if ( offset[0] )
          offset[0] += delta;
      }
  }


  static
  FT_Error  reallocate_t1_table( CID_Table*  table,
                                 FT_Int      new_size )
  {
    FT_Memory  memory   = table->memory;
    FT_Byte*   old_base = table->block;
    FT_Error   error;


    /* realloc the base block */
    if ( REALLOC( table->block, table->capacity, new_size ) )
      return error;

    table->capacity = new_size;

    /* shift all offsets when needed */
    if ( old_base )
      shift_elements( table, old_base );

    return T1_Err_Ok;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    CID_Add_Table                                                      */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Adds an object to a CID_Table, possibly growing its memory block.  */
  /*                                                                       */
  /* <InOut>                                                               */
  /*    table  :: The target table.                                        */
  /*                                                                       */
  /* <Input>                                                               */
  /*    index  :: The index of the object in the table.                    */
  /*                                                                       */
  /*    object :: The address of the object to copy in the memory.         */
  /*                                                                       */
  /*    length :: The length in bytes of the source object.                */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.  An error is returned if    */
  /*    reallocation fails.                                                */
  /*                                                                       */
  LOCAL_FUNC
  FT_Error  CID_Add_Table( CID_Table*  table,
                           FT_Int      index,
                           void*       object,
                           FT_Int      length )
  {
    if ( index < 0 || index > table->max_elems )
    {
      FT_ERROR(( "CID_Add_Table: invalid index\n" ));
      return T1_Err_Syntax_Error;
    }

    /* grow the base block if needed */
    if ( table->cursor + length > table->capacity )
    {
      FT_Error  error;
      FT_Int    new_size = table->capacity;


      while ( new_size < table->cursor + length )
        new_size += 1024;

      error = reallocate_t1_table( table, new_size );
      if ( error )
        return error;
    }

    /* add the object to the base block and adjust offset */
    table->elements[index] = table->block + table->cursor;
    table->lengths [index] = length;

    MEM_Copy( table->block + table->cursor, object, length );

    table->cursor += length;

    return T1_Err_Ok;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    CID_Done_Table                                                     */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Finalizes a CID_Table (reallocate it to its current cursor).       */
  /*                                                                       */
  /* <InOut>                                                               */
  /*    table :: The target table.                                         */
  /*                                                                       */
  /* <Note>                                                                */
  /*    This function does NOT release the heap's memory block.  It is up  */
  /*    to the caller to clean it, or reference it in its own structures.  */
  /*                                                                       */
  LOCAL_FUNC
  void  CID_Done_Table( CID_Table*  table )
  {
    FT_Memory  memory = table->memory;
    FT_Error   error;
    FT_Byte*   old_base;


    /* should never fail, as rec.cursor <= rec.size */
    old_base = table->block;
    if ( !old_base )
      return;

    (void)REALLOC( table->block, table->capacity, table->cursor );
    table->capacity = table->cursor;

    if ( old_base != table->block )
      shift_elements( table, old_base );
  }


  LOCAL_FUNC
  void  CID_Release_Table( CID_Table*  table )
  {
    FT_Memory  memory = table->memory;


    if ( table->init == 0xDEADBEEFL )
    {
      FREE( table->block );
      FREE( table->elements );
      FREE( table->lengths );
      table->init = 0;
    }
  }

#endif /* 0 */


  /*************************************************************************/
  /*************************************************************************/
  /*************************************************************************/
  /*****                                                               *****/
  /*****                    INPUT STREAM PARSER                        *****/
  /*****                                                               *****/
  /*************************************************************************/
  /*************************************************************************/
  /*************************************************************************/


#define IS_CID_WHITESPACE( c )  ( (c) == ' '  || (c) == '\t' )
#define IS_CID_LINESPACE( c )   ( (c) == '\r' || (c) == '\n' )

#define IS_CID_SPACE( c )  ( IS_CID_WHITESPACE( c ) || IS_CID_LINESPACE( c ) )


  LOCAL_FUNC
  void  CID_Skip_Spaces( CID_Parser*  parser )
  {
    FT_Byte* cur   = parser->cursor;
    FT_Byte* limit = parser->limit;


    while ( cur < limit )
    {
      FT_Byte  c = *cur;


      if ( !IS_CID_SPACE( c ) )
        break;
      cur++;
    }

    parser->cursor = cur;
  }


  LOCAL_FUNC
  void  CID_ToToken( CID_Parser*     parser,
                     CID_Token_Rec*  token )
  {
    FT_Byte*  cur;
    FT_Byte*  limit;
    FT_Byte   starter, ender;
    FT_Int    embed;


    token->type  = t1_token_none;
    token->start = 0;
    token->limit = 0;

    /* first of all, skip space */
    CID_Skip_Spaces( parser );

    cur   = parser->cursor;
    limit = parser->limit;

    if ( cur < limit )
    {
      switch ( *cur )
      {
        /************* check for strings ***********************/
      case '(':
        token->type = t1_token_string;
        ender = ')';
        goto Lookup_Ender;

        /************* check for programs/array ****************/
      case '{':
        token->type = t1_token_array;
        ender = '}';
        goto Lookup_Ender;

        /************* check for table/array ******************/
      case '[':
        token->type = t1_token_array;
        ender = ']';

      Lookup_Ender:
        embed   = 1;
        starter = *cur++;
        token->start = cur;

        while ( cur < limit )
        {
          if ( *cur == starter )
            embed++;
          else if ( *cur == ender )
          {
            embed--;
            if ( embed <= 0 )
            {
              token->limit = cur++;
              break;
            }
          }
          cur++;
        }
        break;

        /* **************** otherwise, it is any token **********/
      default:
        token->start = cur++;
        token->type  = t1_token_any;
        while ( cur < limit && !IS_CID_SPACE( *cur ) )
          cur++;

        token->limit = cur;
      }

      if ( !token->limit )
      {
        token->start = 0;
        token->type  = t1_token_none;
      }

      parser->cursor = cur;
    }
  }


  LOCAL_FUNC
  void  CID_ToTokenArray( CID_Parser*     parser,
                          CID_Token_Rec*  tokens,
                          FT_UInt         max_tokens,
                          FT_Int*         pnum_tokens )
  {
    CID_Token_Rec  master;


    *pnum_tokens = -1;

    CID_ToToken( parser, &master );

    if ( master.type == t1_token_array )
    {
      FT_Byte*        old_cursor = parser->cursor;
      FT_Byte*        old_limit  = parser->limit;
      CID_Token_Rec*  cur        = tokens;
      CID_Token_Rec*  limit      = cur + max_tokens;


      parser->cursor = master.start;
      parser->limit  = master.limit;

      while ( parser->cursor < parser->limit )
      {
        CID_Token_Rec  token;


        CID_ToToken( parser, &token );
        if ( !token.type )
          break;

        if ( cur < limit )
          *cur = token;

        cur++;
      }

      *pnum_tokens = cur - tokens;

      parser->cursor = old_cursor;
      parser->limit  = old_limit;
    }
  }


  static
  FT_Long  t1_toint( FT_Byte**  cursor,
                     FT_Byte*   limit )
  {
    FT_Long   result = 0;
    FT_Byte*  cur    = *cursor;
    FT_Byte   c, d;


    for ( ; cur < limit; cur++ )
    {
      c = *cur;
      d = (FT_Byte)( c - '0' );
      if ( d < 10 )
        break;

      if ( c == '-' )
      {
        cur++;
        break;
      }
    }

    if ( cur < limit )
    {
      do
      {
        d = (FT_Byte)( cur[0] - '0' );
        if ( d >= 10 )
          break;

        result = result * 10 + d;
        cur++;

      } while ( cur < limit );

      if ( c == '-' )
        result = -result;
    }

    *cursor = cur;

    return result;
  }


  static
  FT_Long  t1_tofixed( FT_Byte**  cursor,
                       FT_Byte*   limit,
                       FT_Long    power_ten )
  {
    FT_Byte*  cur = *cursor;
    FT_Long   num, divider, result;
    FT_Int    sign = 0;
    FT_Byte   d;


    if ( cur >= limit )
      return 0;

    /* first of all, read the integer part */
    result  = t1_toint( &cur, limit ) << 16;
    num     = 0;
    divider = 1;

    if ( result < 0 )
    {
      sign   = 1;
      result = -result;
    }

    if ( cur >= limit )
      goto Exit;

    /* read decimal part, if any */
    if ( *cur == '.' && cur + 1 < limit )
    {
      cur++;

      for (;;)
      {
        d = (FT_Byte)( *cur - '0' );
        if ( d >= 10 )
          break;

        if ( divider < 10000000L )
        {
          num      = num * 10 + d;
          divider *= 10;
        }

        cur++;
        if ( cur >= limit )
          break;
      }
    }

    /* read exponent, if any */
    if ( cur + 1 < limit && ( *cur == 'e' || *cur == 'E' ) )
    {
      cur++;
      power_ten += t1_toint( &cur, limit );
    }

  Exit:
    /* raise to power of ten if needed */
    while ( power_ten > 0 )
    {
      result = result * 10;
      num    = num * 10;
      power_ten--;
    }

    while ( power_ten < 0 )
    {
      result  = result / 10;
      divider = divider * 10;
      power_ten++;
    }

    if ( num )
      result += FT_DivFix( num, divider );

    if ( sign )
      result = -result;

    *cursor = cur;

    return result;
  }


  static
  int  t1_tobool( FT_Byte**  cursor,
                  FT_Byte*   limit )
  {
    FT_Byte*  cur    = *cursor;
    FT_Bool   result = 0;


    /* return 1 if we find a "true", 0 otherwise */
    if ( cur + 3 < limit &&
         cur[0] == 't'   &&
         cur[1] == 'r'   &&
         cur[2] == 'u'   &&
         cur[3] == 'e'   )
    {
      result = 1;
      cur   += 5;
    }
    else if ( cur + 4 < limit &&
              cur[0] == 'f'   &&
              cur[1] == 'a'   &&
              cur[2] == 'l'   &&
              cur[3] == 's'   &&
              cur[4] == 'e'   )
    {
      result = 0;
      cur   += 6;
    }
    *cursor = cur;
    return result;
  }


  static
  FT_Int  t1_tocoordarray( FT_Byte**  cursor,
                           FT_Byte*   limit,
                           FT_Int     max_coords,
                           FT_Short*  coords )
  {
    FT_Byte*  cur   = *cursor;
    FT_Int    count = 0;
    FT_Byte   c, ender;


    if ( cur >= limit )
      goto Exit;

    /* check for the beginning of an array. */
    /* If not, only one number will be read */
    c     = *cur;
    ender = 0;

    if ( c == '[' )
      ender = ']';

    if ( c == '{' )
      ender = '}';

    if ( ender )
      cur++;

    /* now, read the coordinates */
    for ( ; cur < limit; )
    {
      /* skip whitespace in front of data */
      for (;;)
      {
        c = *cur;
        if ( c != ' ' && c != '\t' )
          break;

        cur++;
        if ( cur >= limit )
          goto Exit;
      }

      if ( count >= max_coords || c == ender )
        break;

      coords[count] = (FT_Short)( t1_tofixed( &cur, limit, 0 ) >> 16 );
      count++;

      if ( !ender )
        break;
    }

  Exit:
    *cursor = cur;
    return count;
  }


  static
  FT_Int  t1_tofixedarray( FT_Byte**  cursor,
                           FT_Byte*   limit,
                           FT_Int     max_values,
                           FT_Fixed*  values,
                           FT_Int     power_ten )
  {
    FT_Byte*  cur   = *cursor;
    FT_Int    count = 0;
    FT_Byte   c, ender;


    if ( cur >= limit )
      goto Exit;

    /* check for the beginning of an array. */
    /* If not, only one number will be read */
    c     = *cur;
    ender = 0;

    if ( c == '[' )
      ender = ']';

    if ( c == '{' )
      ender = '}';

    if ( ender )
      cur++;

    /* now, read the values */
    for ( ; cur < limit; )
    {
      /* skip whitespace in front of data */
      for (;;)
      {
        c = *cur;
        if ( c != ' ' && c != '\t' )
          break;

        cur++;
        if ( cur >= limit )
          goto Exit;
      }

      if ( count >= max_values || c == ender )
        break;

      values[count] = t1_tofixed( &cur, limit, power_ten );
      count++;

      if ( !ender )
        break;
    }

  Exit:
    *cursor = cur;

    return count;
  }


  /* Loads a simple field (i.e. non-table) into the current */
  /* list of objects                                        */
  LOCAL_FUNC
  FT_Error  CID_Load_Field( CID_Parser*           parser,
                            const CID_Field_Rec*  field,
                            void*                 object )
  {
    CID_Token_Rec  token;
    FT_Byte*      cur;
    FT_Byte*      limit;
    FT_UInt       count;
    FT_UInt       index;
    FT_Error      error;


    CID_ToToken( parser, &token );
    if ( !token.type )
      goto Fail;

    count = 1;
    index = 0;
    cur   = token.start;
    limit = token.limit;

    {
      FT_Byte*   q = (FT_Byte*)object + field->offset;
      FT_Long    val;
      FT_String* string;


      switch ( field->type )
      {
      case t1_field_bool:
        val = t1_tobool( &cur, limit );
        goto Store_Integer;

      case t1_field_fixed:
        val = t1_tofixed( &cur, limit, 0 );
        goto Store_Integer;

      case t1_field_integer:
        val = t1_toint( &cur, limit );

      Store_Integer:
        switch ( field->size )
        {
          case 1:
            *(FT_Byte*)q = (FT_Byte)val;
            break;

          case 2:
            *(FT_UShort*)q = (FT_UShort)val;
            break;

          case 4:
            *(FT_Int32*)q = (FT_Int)val;
            break;

          default:  /* for 64-bit systems */
            *(FT_Long*)q = val;
        }
        break;

      case t1_field_string:
        {
          FT_Memory  memory = parser->memory;
          FT_UInt    len    = limit-cur;


          if ( ALLOC( string, len + 1 ) )
            goto Exit;

          MEM_Copy( string, cur, len );
          string[len] = 0;

          *(FT_String**)q = string;
        }
        break;

      default:
        /* an error occurred */
        goto Fail;
      }
    }

    error = 0;

  Exit:
    return error;

  Fail:
    error = T1_Err_Invalid_File_Format;
    goto Exit;
  }


#define T1_MAX_TABLE_ELEMENTS  32


  LOCAL_FUNC
  FT_Error  CID_Load_Field_Table( CID_Parser*           parser,
                                  const CID_Field_Rec*  field,
                                  void*                 object )
  {
    CID_Token_Rec   elements[T1_MAX_TABLE_ELEMENTS];
    CID_Token_Rec*  token;
    FT_Int          num_elements;
    FT_Error        error = 0;
    FT_Byte*        old_cursor;
    FT_Byte*        old_limit;
    CID_Field_Rec   fieldrec = *(CID_Field_Rec*)field;


    fieldrec.type = t1_field_integer;
    if ( field->type == t1_field_fixed_array )
      fieldrec.type = t1_field_fixed;

    CID_ToTokenArray( parser, elements, 32, &num_elements );
    if ( num_elements < 0 )
      goto Fail;

    if ( num_elements > T1_MAX_TABLE_ELEMENTS )
      num_elements = T1_MAX_TABLE_ELEMENTS;

    old_cursor = parser->cursor;
    old_limit  = parser->limit;

    /* we store the elements count */
    if ( field->count_offset )
      *(FT_Byte*)( (FT_Byte*)object + field->count_offset ) = num_elements;

    /* we now load each element, adjusting the field.offset on each one */
    token = elements;
    for ( ; num_elements > 0; num_elements--, token++ )
    {
      parser->cursor = token->start;
      parser->limit  = token->limit;
      CID_Load_Field( parser, &fieldrec, object );
      fieldrec.offset += fieldrec.size;
    }

    parser->cursor = old_cursor;
    parser->limit  = old_limit;

  Exit:
    return error;

  Fail:
    error = T1_Err_Invalid_File_Format;
    goto Exit;
  }


  LOCAL_FUNC
  FT_Long  CID_ToInt( CID_Parser*  parser )
  {
    return t1_toint( &parser->cursor, parser->limit );
  }


  LOCAL_FUNC
  FT_Int  CID_ToCoordArray( CID_Parser*  parser,
                            FT_Int       max_coords,
                            FT_Short*    coords )
  {
    return t1_tocoordarray( &parser->cursor, parser->limit,
                            max_coords, coords );
  }


  LOCAL_FUNC
  FT_Int  CID_ToFixedArray( CID_Parser*  parser,
                            FT_Int       max_values,
                            FT_Fixed*    values,
                            FT_Int       power_ten )
  {
    return t1_tofixedarray( &parser->cursor, parser->limit,
                            max_values, values, power_ten );
  }


#if 0

  /* return the value of an hexadecimal digit */
  static
  int  hexa_value( char  c )
  {
    unsigned int  d;


    d = (unsigned int)( c - '0' );
    if ( d <= 9 )
      return (int)d;

    d = (unsigned int)( c - 'a' );
    if ( d <= 5 )
      return (int)( d + 10 );

    d = (unsigned int)( c - 'A' );
    if ( d <= 5 )
      return (int)( d + 10 );

    return -1;
  }

#endif /* 0 */


  LOCAL_FUNC
  FT_Error  CID_New_Parser( CID_Parser*  parser,
                            FT_Stream    stream,
                            FT_Memory    memory )
  {
    FT_Error  error;
    FT_ULong  base_offset, offset, ps_len;
    FT_Byte   buffer[256 + 10];
    FT_Int    buff_len;


    MEM_Set( parser, 0, sizeof ( *parser ) );
    parser->stream = stream;
    parser->memory = memory;

    base_offset = FILE_Pos();

    /* first of all, check the font format in the  header */
    if ( ACCESS_Frame( 31 ) )
      goto Exit;

    if ( strncmp( (char *)stream->cursor,
                  "%!PS-Adobe-3.0 Resource-CIDFont", 31 ) )
    {
      FT_TRACE2(( "[not a valid CID-keyed font]\n" ));
      error = FT_Err_Unknown_File_Format;
    }

    FORGET_Frame();
    if ( error )
      goto Exit;

    /* now, read the rest of the file, until we find a `StartData' */
    buff_len = 256;
    for (;;)
    {
      FT_Byte  *p, *limit = buffer + 256;

      /* fill input buffer */
      buff_len -= 256;
      if ( buff_len > 0 )
        MEM_Move( buffer, limit, buff_len );

      if ( FILE_Read( buffer, 256 + 10 - buff_len ) )
        goto Exit;

      buff_len = 256 + 10;

      /* look for `StartData' */
      for ( p = buffer; p < limit; p++ )
      {
        if ( p[0] == 'S' && strncmp( (char*)p, "StartData", 9 ) == 0 )
        {
          /* save offset of binary data after `StartData' */
          offset = FILE_Pos() - ( limit - p ) + 10;
          goto Found;
        }
      }
    }

  Found:
    /* we have found the start of the binary data.  We will now        */
    /* rewind and extract the frame of corresponding to the Postscript */
    /* section                                                         */

    ps_len = offset - base_offset;
    if ( FILE_Seek( base_offset )                    ||
         EXTRACT_Frame( ps_len, parser->postscript ) )
      goto Exit;

    parser->data_offset    = offset;
    parser->postscript_len = ps_len;
    parser->cursor         = parser->postscript;
    parser->limit          = parser->cursor + ps_len;
    parser->num_dict       = -1;

  Exit:
    return error;
  }


  LOCAL_FUNC
  void  CID_Done_Parser( CID_Parser*  parser )
  {
    /* always free the private dictionary */
    if ( parser->postscript )
    {
      FT_Stream  stream = parser->stream;


      RELEASE_Frame( parser->postscript );
    }
  }


/* END */
