/***************************************************************************/
/*                                                                         */
/*  z1parse.c                                                              */
/*                                                                         */
/*    Experimental Type 1 parser (body).                                   */
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
  /* The Type 1 parser is in charge of the following:                      */
  /*                                                                       */
  /*  - provide an implementation of a growing sequence of objects called  */
  /*    a `Z1_Table' (used to build various tables needed by the loader).  */
  /*                                                                       */
  /*  - opening .pfb and .pfa files to extract their top-level and private */
  /*    dictionaries.                                                      */
  /*                                                                       */
  /*  - read numbers, arrays & strings from any dictionary.                */
  /*                                                                       */
  /* See `z1load.c' to see how data is loaded from the font file.          */
  /*                                                                       */
  /*************************************************************************/


#include <freetype/internal/ftdebug.h>
#include <freetype/internal/ftcalc.h>
#include <freetype/internal/ftobjs.h>
#include <freetype/internal/ftstream.h>
#include <freetype/internal/t1errors.h>


#ifdef FT_FLAT_COMPILE

#include "z1parse.h"

#else

#include <freetype/src/type1z/z1parse.h>

#endif


#include <string.h>     /* for strncmp() */


  /*************************************************************************/
  /*                                                                       */
  /* The macro FT_COMPONENT is used in trace mode.  It is an implicit      */
  /* parameter of the FT_TRACE() and FT_ERROR() macros, used to print/log  */
  /* messages during execution.                                            */
  /*                                                                       */
#undef  FT_COMPONENT
#define FT_COMPONENT  trace_z1parse


  /*************************************************************************/
  /*************************************************************************/
  /*************************************************************************/
  /*****                                                               *****/
  /*****              IMPLEMENTATION OF Z1_TABLE OBJECT                *****/
  /*****                                                               *****/
  /*************************************************************************/
  /*************************************************************************/
  /*************************************************************************/


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    Z1_New_Table                                                       */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Initialises a Z1_Table.                                            */
  /*                                                                       */
  /* <InOut>                                                               */
  /*    table  :: The address of the target table.                         */
  /*                                                                       */
  /* <Input>                                                               */
  /*    count  :: The table size = the maximum number of elements.         */
  /*                                                                       */
  /*    memory :: The memory object to use for all subsequent              */
  /*              reallocations.                                           */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  LOCAL_FUNC
  FT_Error  Z1_New_Table( Z1_Table*  table,
                          FT_Int     count,
                          FT_Memory  memory )
  {
    FT_Error  error;


    table->memory = memory;
    if ( ALLOC_ARRAY( table->elements, count, FT_Byte*  ) ||
         ALLOC_ARRAY( table->lengths, count, FT_Byte* )   )
      goto Exit;

    table->max_elems = count;
    table->init      = 0xdeadbeef;
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
  void  shift_elements( Z1_Table*  table,
                        FT_Byte*   old_base )
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
  FT_Error  reallocate_t1_table( Z1_Table*  table,
                                 FT_Int     new_size )
  {
    FT_Memory  memory   = table->memory;
    FT_Byte*   old_base = table->block;
    FT_Error   error;


    /* reallocate the base block */
    if ( REALLOC( table->block, table->capacity, new_size ) )
      return error;

    table->capacity = new_size;

    /* shift all offsets if necessary */
    if ( old_base )
      shift_elements( table, old_base );

    return T1_Err_Ok;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    Z1_Add_Table                                                       */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Adds an object to a Z1_Table, possibly growing its memory block.   */
  /*                                                                       */
  /* <InOut>                                                               */
  /*    table  :: The target table.                                        */
  /*                                                                       */
  /* <Input>                                                               */
  /*    index  :: The index of the object in the table.                    */
  /*                                                                       */
  /*    object :: The address of the object to copy in memory.             */
  /*                                                                       */
  /*    length :: The length in bytes of the source object.                */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.  An error is returned if a  */
  /*    reallocation fails.                                                */
  /*                                                                       */
  LOCAL_FUNC
  FT_Error  Z1_Add_Table( Z1_Table*  table,
                          FT_Int     index,
                          void*      object,
                          FT_Int     length )
  {
    if ( index < 0 || index > table->max_elems )
    {
      FT_ERROR(( "Z1_Add_Table: invalid index\n" ));
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


#if 0

  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    Z1_Done_Table                                                      */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Finalizes a Z1_Table (i.e., reallocate it to its current cursor).  */
  /*                                                                       */
  /* <InOut>                                                               */
  /*    table :: The target table.                                         */
  /*                                                                       */
  /* <Note>                                                                */
  /*    This function does NOT release the heap's memory block.  It is up  */
  /*    to the caller to clean it, or reference it in its own structures.  */
  /*                                                                       */
  LOCAL_FUNC
  void  Z1_Done_Table( Z1_Table*  table )
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

#endif /* 0 */


  LOCAL_FUNC
  void  Z1_Release_Table( Z1_Table*  table )
  {
    FT_Memory  memory = table->memory;


    if ( table->init == (FT_Long)0xDEADBEEF )
    {
      FREE( table->block );
      FREE( table->elements );
      FREE( table->lengths );
      table->init = 0;
    }
  }


  /*************************************************************************/
  /*************************************************************************/
  /*************************************************************************/
  /*****                                                               *****/
  /*****                   INPUT STREAM PARSER                         *****/
  /*****                                                               *****/
  /*************************************************************************/
  /*************************************************************************/
  /*************************************************************************/


#define IS_Z1_WHITESPACE( c )  ( (c) == ' '  || (c) == '\t' )
#define IS_Z1_LINESPACE( c )   ( (c) == '\r' || (c) == '\n' )

#define IS_Z1_SPACE( c )  ( IS_Z1_WHITESPACE( c ) || IS_Z1_LINESPACE( c ) )


  LOCAL_FUNC
  void  Z1_Skip_Spaces( Z1_Parser*  parser )
  {
    FT_Byte* cur   = parser->cursor;
    FT_Byte* limit = parser->limit;


    while ( cur < limit )
    {
      FT_Byte  c = *cur;


      if ( !IS_Z1_SPACE( c ) )
        break;
      cur++;
    }
    parser->cursor = cur;
  }


  LOCAL_FUNC
  void  Z1_ToToken( Z1_Parser*     parser,
                    Z1_Token_Rec*  token )
  {
    FT_Byte*  cur;
    FT_Byte*  limit;
    FT_Byte   starter, ender;
    FT_Int    embed;


    token->type  = t1_token_none;
    token->start = 0;
    token->limit = 0;

    /* first of all, skip space */
    Z1_Skip_Spaces( parser );

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

        /* **************** otherwise, it's any token **********/
      default:
        token->start = cur++;
        token->type  = t1_token_any;
        while ( cur < limit && !IS_Z1_SPACE( *cur ) )
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
  void  Z1_ToTokenArray( Z1_Parser*     parser,
                         Z1_Token_Rec*  tokens,
                         FT_UInt        max_tokens,
                         FT_Int*        pnum_tokens )
  {
    Z1_Token_Rec  master;


    *pnum_tokens = -1;

    Z1_ToToken( parser, &master );
    if ( master.type == t1_token_array )
    {
      FT_Byte*       old_cursor = parser->cursor;
      FT_Byte*       old_limit  = parser->limit;
      Z1_Token_Rec*  cur        = tokens;
      Z1_Token_Rec*  limit      = cur + max_tokens;


      parser->cursor = master.start;
      parser->limit  = master.limit;

      while ( parser->cursor < parser->limit )
      {
        Z1_Token_Rec  token;


        Z1_ToToken( parser, &token );
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
    FT_Byte* cur  = *cursor;
    FT_Long  num, divider, result;
    FT_Int   sign = 0;
    FT_Byte  d;


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

    /* check for the beginning of an array. If not, only one number will */
    /* be read                                                           */
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


    if ( cur >= limit ) goto Exit;

    /* check for the beginning of an array. If not, only one number will */
    /* be read                                                           */
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


#if 0

  static
  FT_String*  t1_tostring( FT_Byte**  cursor,
                           FT_Byte*   limit,
                           FT_Memory  memory )
  {
    FT_Byte*    cur = *cursor;
    FT_Int      len = 0;
    FT_Int      count;
    FT_String*  result;
    FT_Error    error;


    /* XXX: some stupid fonts have a `Notice' or `Copyright' string     */
    /*      that simply doesn't begin with an opening parenthesis, even */
    /*      though they have a closing one!  E.g. "amuncial.pfb"        */
    /*                                                                  */
    /*      We must deal with these ill-fated cases there.  Note that   */
    /*      these fonts didn't work with the old Type 1 driver as the   */
    /*      notice/copyright was not recognized as a valid string token */
    /*      and made the old token parser commit errors.                */

    while ( cur < limit && ( *cur == ' ' || *cur == '\t' ) )
      cur++;
    if ( cur + 1 >= limit )
      return 0;

    if ( *cur == '(' )
      cur++;  /* skip the opening parenthesis, if there is one */

    *cursor = cur;
    count   = 0;

    /* then, count its length */
    for ( ; cur < limit; cur++ )
    {
      if ( *cur == '(' )
        count++;

      else if ( *cur == ')' )
      {
        count--;
        if ( count < 0 )
          break;
      }
    }

    len = cur - *cursor;
    if ( cur >= limit || ALLOC( result, len + 1 ) )
      return 0;

    /* now copy the string */
    MEM_Copy( result, *cursor, len );
    result[len] = '\0';
    *cursor = cur;
    return result;
  }

#endif /* 0 */


  static
  int  t1_tobool( FT_Byte**  cursor,
                  FT_Byte*   limit )
  {
    FT_Byte*  cur    = *cursor;
    FT_Bool   result = 0;


    /* return 1 if we find `true', 0 otherwise */
    if ( cur + 3 < limit &&
         cur[0] == 't' &&
         cur[1] == 'r' &&
         cur[2] == 'u' &&
         cur[3] == 'e' )
    {
      result = 1;
      cur   += 5;
    }
    else if ( cur + 4 < limit &&
              cur[0] == 'f' &&
              cur[1] == 'a' &&
              cur[2] == 'l' &&
              cur[3] == 's' &&
              cur[4] == 'e' )
    {
      result = 0;
      cur   += 6;
    }

    *cursor = cur;
    return result;
  }


  /* Load a simple field (i.e. non-table) into the current list of objects */
  LOCAL_FUNC
  FT_Error  Z1_Load_Field( Z1_Parser*           parser,
                           const Z1_Field_Rec*  field,
                           void**               objects,
                           FT_UInt              max_objects,
                           FT_ULong*            pflags )
  {
    Z1_Token_Rec  token;
    FT_Byte*      cur;
    FT_Byte*      limit;
    FT_UInt       count;
    FT_UInt       index;
    FT_Error      error;


    Z1_ToToken( parser, &token );
    if ( !token.type )
      goto Fail;

    count = 1;
    index = 0;
    cur   = token.start;
    limit = token.limit;

    if ( token.type == t1_token_array )
    {
      /* if this is an array, and we have no blend, an error occurs */
      if ( max_objects == 0 )
        goto Fail;

      count = max_objects;
      index = 1;
    }

    for ( ; count > 0; count--, index++ )
    {
      FT_Byte*    q = (FT_Byte*)objects[index] + field->offset;
      FT_Long     val;
      FT_String*  string;

      switch ( field->type )
      {
      case t1_field_bool:
        val = t1_tobool( &cur, limit );
        goto Store_Integer;

      case t1_field_fixed:
        val = t1_tofixed( &cur, limit, 3 );
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
          *(FT_UInt32*)q = (FT_UInt32)val;
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
        /* an error occured */
        goto Fail;
      }
    }

    if ( pflags )
      *pflags |= 1L << field->flag_bit;

    error = FT_Err_Ok;

  Exit:
    return error;

  Fail:
    error = T1_Err_Invalid_File_Format;
    goto Exit;
  }


#define T1_MAX_TABLE_ELEMENTS  32


  LOCAL_FUNC
  FT_Error  Z1_Load_Field_Table( Z1_Parser*           parser,
                                 const Z1_Field_Rec*  field,
                                 void**               objects,
                                 FT_UInt              max_objects,
                                 FT_ULong*            pflags )
  {
    Z1_Token_Rec   elements[T1_MAX_TABLE_ELEMENTS];
    Z1_Token_Rec*  token;
    FT_Int         num_elements;
    FT_Error       error = 0;
    FT_Byte*       old_cursor;
    FT_Byte*       old_limit;
    Z1_Field_Rec   fieldrec = *(Z1_Field_Rec*)field;


    Z1_ToTokenArray( parser, elements, 32, &num_elements );
    if ( num_elements < 0 )
      goto Fail;

    if ( num_elements > T1_MAX_TABLE_ELEMENTS )
      num_elements = T1_MAX_TABLE_ELEMENTS;

    old_cursor = parser->cursor;
    old_limit  = parser->limit;

    /* we store the elements count */
    *(FT_Byte*)( (FT_Byte*)objects[0] + field->count_offset ) = num_elements;

    /* we now load each element, adjusting the field.offset on each one */
    token = elements;
    for ( ; num_elements > 0; num_elements--, token++ )
    {
      parser->cursor = token->start;
      parser->limit  = token->limit;
      Z1_Load_Field( parser, &fieldrec, objects, max_objects, 0 );
      fieldrec.offset += fieldrec.size;
    }

    if ( pflags )
      *pflags |= 1L << field->flag_bit;

    parser->cursor = old_cursor;
    parser->limit  = old_limit;

  Exit:
    return error;

  Fail:
    error = T1_Err_Invalid_File_Format;
    goto Exit;
  }


  LOCAL_FUNC
  FT_Long  Z1_ToInt  ( Z1_Parser*  parser )
  {
    return t1_toint( &parser->cursor, parser->limit );
  }


  LOCAL_FUNC
  FT_Long  Z1_ToFixed( Z1_Parser*  parser,
                       FT_Int      power_ten )
  {
    return t1_tofixed( &parser->cursor, parser->limit, power_ten );
  }


  LOCAL_FUNC
  FT_Int  Z1_ToCoordArray( Z1_Parser*  parser,
                           FT_Int      max_coords,
                           FT_Short*   coords )
  {
    return t1_tocoordarray( &parser->cursor, parser->limit,
                            max_coords, coords );
  }


  LOCAL_FUNC
  FT_Int  Z1_ToFixedArray( Z1_Parser*  parser,
                           FT_Int      max_values,
                           FT_Fixed*   values,
                           FT_Int      power_ten )
  {
    return t1_tofixedarray( &parser->cursor, parser->limit,
                            max_values, values, power_ten );
  }


#if 0

  LOCAL_FUNC
  FT_String*  Z1_ToString( Z1_Parser*  parser )
  {
    return t1_tostring( &parser->cursor, parser->limit, parser->memory );
  }


  LOCAL_FUNC
  FT_Bool  Z1_ToBool( Z1_Parser*  parser )
  {
    return t1_tobool( &parser->cursor, parser->limit );
  }

#endif /* 0 */


  static
  FT_Error  read_pfb_tag( FT_Stream   stream,
                          FT_UShort*  tag,
                          FT_Long*    size )
  {
    FT_Error  error;


    if ( READ_UShort( *tag ) )
      goto Exit;

    if ( *tag == 0x8001 || *tag == 0x8002 )
    {
      FT_Long  asize;


      if ( READ_ULong( asize ) )
        goto Exit;

      /* swap between big and little endianness */
      *size  = ( ( asize & 0xFF000000L ) >> 24 ) |
               ( ( asize & 0x00FF0000L ) >> 8  ) |
               ( ( asize & 0x0000FF00L ) << 8  ) |
               ( ( asize & 0x000000FFL ) << 24 );
    }

  Exit:
    return error;
  }


  LOCAL_FUNC
  FT_Error  Z1_New_Parser( Z1_Parser*  parser,
                           FT_Stream   stream,
                           FT_Memory   memory )
  {
    FT_Error  error;
    FT_UShort tag;
    FT_Long   size;


    parser->stream       = stream;
    parser->memory       = memory;
    parser->base_len     = 0;
    parser->base_dict    = 0;
    parser->private_len  = 0;
    parser->private_dict = 0;
    parser->in_pfb       = 0;
    parser->in_memory    = 0;
    parser->single_block = 0;

    parser->cursor       = 0;
    parser->limit        = 0;

    /******************************************************************/
    /*                                                                */
    /* Here a short summary of what is going on:                      */
    /*                                                                */
    /*   When creating a new Type 1 parser, we try to locate and load */
    /*   the base dictionary if this is possible (i.e. for PFB        */
    /*   files).  Otherwise, we load the whole font into memory.      */
    /*                                                                */
    /*   When `loading' the base dictionary, we only setup pointers   */
    /*   in the case of a memory-based stream.  Otherwise, we         */
    /*   allocate and load the base dictionary in it.                 */
    /*                                                                */
    /*   parser->in_pfb is set if we are in a binary (".pfb") font.   */
    /*   parser->in_memory is set if we have a memory stream.         */
    /*                                                                */

    /* try to compute the size of the base dictionary;   */
    /* look for a Postscript binary file tag, i.e 0x8001 */
    if ( FILE_Seek( 0L ) )
      goto Exit;

    error = read_pfb_tag( stream, &tag, &size );
    if ( error )
      goto Exit;

    if ( tag != 0x8001 )
    {
      /* assume that this is a PFA file for now; an error will */
      /* be produced later when more things are checked        */
      (void)FILE_Seek( 0L );
      size = stream->size;
    }
    else
      parser->in_pfb = 1;

    /* now, try to load `size' bytes of the `base' dictionary we */
    /* found previously                                          */

    /* if it is a memory-based resource, set up pointers */
    if ( !stream->read )
    {
      parser->base_dict = (FT_Byte*)stream->base + stream->pos;
      parser->base_len  = size;
      parser->in_memory = 1;

      /* check that the `size' field is valid */
      if ( FILE_Skip( size ) )
        goto Exit;
    }
    else
    {
      /* read segment in memory */
      if ( ALLOC( parser->base_dict, size )     ||
           FILE_Read( parser->base_dict, size ) )
        goto Exit;
      parser->base_len = size;
    }

    /* Now check font format; we must see `%!PS-AdobeFont-1' */
    /* or `%!FontType'                                       */
    {
      if ( size <= 16                                    ||
           ( strncmp( (const char*)parser->base_dict,
                      "%!PS-AdobeFont-1", 16 )        &&
             strncmp( (const char*)parser->base_dict,
                      "%!FontType", 10 )              )  )
      {
        FT_TRACE2(( "[not a Type1 font]\n" ));
        error = FT_Err_Unknown_File_Format;
      }
      else
      {
        parser->cursor = parser->base_dict;
        parser->limit  = parser->cursor + parser->base_len;
      }
    }

  Exit:
    if ( error && !parser->in_memory )
      FREE( parser->base_dict );

    return error;
  }


  LOCAL_FUNC
  void  Z1_Done_Parser( Z1_Parser*  parser )
  {
    FT_Memory   memory = parser->memory;


    /* always free the private dictionary */
    FREE( parser->private_dict );

    /* free the base dictionary only when we have a disk stream */
    if ( !parser->in_memory )
      FREE( parser->base_dict );
  }


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


  LOCAL_FUNC
  void  Z1_Decrypt( FT_Byte*   buffer,
                    FT_Int     length,
                    FT_UShort  seed )
  {
    while ( length > 0 )
    {
      FT_Byte  plain;


      plain     = ( *buffer ^ ( seed >> 8 ) );
      seed      = ( *buffer + seed ) * 52845 + 22719;
      *buffer++ = plain;
      length--;
    }
  }


  LOCAL_FUNC
  FT_Error  Z1_Get_Private_Dict( Z1_Parser*  parser )
  {
    FT_Stream  stream = parser->stream;
    FT_Memory  memory = parser->memory;
    FT_Error   error  = 0;
    FT_Long    size;


    if ( parser->in_pfb )
    {
      /* in the case of the PFB format, the private dictionary can be  */
      /* made of several segments.  We thus first read the number of   */
      /* segments to compute the total size of the private dictionary  */
      /* then re-read them into memory.                                */
      FT_Long    start_pos = FILE_Pos();
      FT_UShort  tag;
      FT_Long    size;


      parser->private_len = 0;
      for (;;)
      {
        error = read_pfb_tag( stream, &tag, &size );
        if ( error )
          goto Fail;

        if ( tag != 0x8002 )
          break;

        parser->private_len += size;

        if ( FILE_Skip( size ) )
          goto Fail;
      }

      /* Check that we have a private dictionary there */
      /* and allocate private dictionary buffer        */
      if ( parser->private_len == 0 )
      {
        FT_ERROR(( "Z1_Get_Private_Dict:" ));
        FT_ERROR(( " invalid private dictionary section\n" ));
        error = T1_Err_Invalid_File_Format;
        goto Fail;
      }

      if ( FILE_Seek( start_pos )                             ||
           ALLOC( parser->private_dict, parser->private_len ) )
        goto Fail;

      parser->private_len = 0;
      for (;;)
      {
        error = read_pfb_tag( stream, &tag, &size );
        if ( error || tag != 0x8002 )
        {
          error = FT_Err_Ok;
          break;
        }

        if ( FILE_Read( parser->private_dict + parser->private_len, size ) )
          goto Fail;

        parser->private_len += size;
      }
    }
    else
    {
      /* we have already `loaded' the whole PFA font file into memory; */
      /* if this is a memory resource, allocate a new block to hold    */
      /* the private dict. Otherwise, simply overwrite into the base   */
      /* dictionary block in the heap.                                 */

      /* first of all, look at the `eexec' keyword */
      FT_Byte*  cur   = parser->base_dict;
      FT_Byte*  limit = cur + parser->base_len;
      FT_Byte   c;


      for (;;)
      {
        c = cur[0];
        if ( c == 'e' && cur + 9 < limit )  /* 9 = 5 letters for `eexec' + */
                                            /* newline + 4 chars           */
        {
          if ( cur[1] == 'e' && cur[2] == 'x' &&
               cur[3] == 'e' && cur[4] == 'c' )
          {
            cur += 6; /* we skip the newling after the `eexec' */

            /* XXX: Some fonts use DOS-linefeeds, i.e. \r\n; we need to */
            /*      skip the extra \n if we find it                     */
            if ( cur[0] == '\n' )
              cur++;

            break;
          }
        }
        cur++;
        if ( cur >= limit )
        {
          FT_ERROR(( "Z1_Get_Private_Dict:" ));
          FT_ERROR(( " could not find `eexec' keyword\n" ));
          error = T1_Err_Invalid_File_Format;
          goto Exit;
        }
      }

      /* now determine where to write the _encrypted_ binary private  */
      /* dictionary.  We overwrite the base dictionary for disk-based */
      /* resources and allocate a new block otherwise                 */

      size = parser->base_len - ( cur - parser->base_dict);

      if ( parser->in_memory )
      {
        /* note that we allocate one more byte to put a terminating `0' */
        if ( ALLOC( parser->private_dict, size + 1 ) )
          goto Fail;
        parser->private_len = size;
      }
      else
      {
        parser->single_block = 1;
        parser->private_dict = parser->base_dict;
        parser->private_len  = size;
        parser->base_dict    = 0;
        parser->base_len     = 0;
      }

      /* now determine whether the private dictionary is encoded in binary */
      /* or hexadecimal ASCII format -- decode it accordingly              */

      /* we need to access the next 4 bytes (after the final \r following */
      /* the `eexec' keyword); if they all are hexadecimal digits, then   */
      /* we have a case of ASCII storage                                  */

      if ( ( hexa_value( cur[0] ) | hexa_value( cur[1] ) |
             hexa_value( cur[2] ) | hexa_value( cur[3] ) ) < 0 )

        /* binary encoding -- `simply' copy the private dict */
        MEM_Copy( parser->private_dict, cur, size );

      else
      {
        /* ASCII hexadecimal encoding */

        FT_Byte*  write;
        FT_Int    count;


        write = parser->private_dict;
        count = 0;

        for ( ;cur < limit; cur++ )
        {
          int  hex1;


          /* check for newline */
          if ( cur[0] == '\r' || cur[0] == '\n' )
            continue;

          /* exit if we have a non-hexadecimal digit that isn't a newline */
          hex1 = hexa_value( cur[0] );
          if ( hex1 < 0 || cur + 1 >= limit )
            break;

          /* otherwise, store byte */
          *write++ = ( hex1 << 4 ) | hexa_value( cur[1] );
          count++;
          cur++;
        }

        /* put a safeguard */
        parser->private_len = write - parser->private_dict;
        *write++ = 0;
      }
    }

    /* we now decrypt the encoded binary private dictionary */
    Z1_Decrypt( parser->private_dict, parser->private_len, 55665 );
    parser->cursor = parser->private_dict;
    parser->limit  = parser->cursor + parser->private_len;

  Fail:
  Exit:
    return error;
  }


/* END */
