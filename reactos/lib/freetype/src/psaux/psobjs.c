/***************************************************************************/
/*                                                                         */
/*  psobjs.c                                                               */
/*                                                                         */
/*    Auxiliary functions for PostScript fonts (body).                     */
/*                                                                         */
/*  Copyright 1996-2001, 2002 by                                           */
/*  David Turner, Robert Wilhelm, and Werner Lemberg.                      */
/*                                                                         */
/*  This file is part of the FreeType project, and may only be used,       */
/*  modified, and distributed under the terms of the FreeType project      */
/*  license, LICENSE.TXT.  By continuing to use, modify, or distribute     */
/*  this file you indicate that you have read the license and              */
/*  understand and accept it fully.                                        */
/*                                                                         */
/***************************************************************************/


#include <ft2build.h>
#include FT_INTERNAL_POSTSCRIPT_AUX_H
#include FT_INTERNAL_DEBUG_H

#include "psobjs.h"

#include "psauxerr.h"


  /*************************************************************************/
  /*************************************************************************/
  /*****                                                               *****/
  /*****                             PS_TABLE                          *****/
  /*****                                                               *****/
  /*************************************************************************/
  /*************************************************************************/

  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    ps_table_new                                                       */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Initializes a PS_Table.                                            */
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
  FT_LOCAL_DEF( FT_Error )
  ps_table_new( PS_Table   table,
                FT_Int     count,
                FT_Memory  memory )
  {
    FT_Error  error;


    table->memory = memory;
    if ( FT_NEW_ARRAY( table->elements, count ) ||
         FT_NEW_ARRAY( table->lengths,  count ) )
      goto Exit;

    table->max_elems = count;
    table->init      = 0xDEADBEEFUL;
    table->num_elems = 0;
    table->block     = 0;
    table->capacity  = 0;
    table->cursor    = 0;

    *(PS_Table_FuncsRec*)&table->funcs = ps_table_funcs;

  Exit:
    if ( error )
      FT_FREE( table->elements );

    return error;
  }


  static void
  shift_elements( PS_Table  table,
                  FT_Byte*  old_base )
  {
    FT_Long    delta  = (FT_Long)( table->block - old_base );
    FT_Byte**  offset = table->elements;
    FT_Byte**  limit  = offset + table->max_elems;


    for ( ; offset < limit; offset++ )
    {
      if ( offset[0] )
        offset[0] += delta;
    }
  }


  static FT_Error
  reallocate_t1_table( PS_Table  table,
                       FT_Long   new_size )
  {
    FT_Memory  memory   = table->memory;
    FT_Byte*   old_base = table->block;
    FT_Error   error;


    /* allocate new base block */
    if ( FT_ALLOC( table->block, new_size ) )
    {
      table->block = old_base;
      return error;
    }

    /* copy elements and shift offsets */
    if (old_base )
    {
      FT_MEM_COPY( table->block, old_base, table->capacity );
      shift_elements( table, old_base );
      FT_FREE( old_base );
    }

    table->capacity = new_size;

    return PSaux_Err_Ok;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    ps_table_add                                                       */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Adds an object to a PS_Table, possibly growing its memory block.   */
  /*                                                                       */
  /* <InOut>                                                               */
  /*    table  :: The target table.                                        */
  /*                                                                       */
  /* <Input>                                                               */
  /*    idx  :: The index of the object in the table.                      */
  /*                                                                       */
  /*    object :: The address of the object to copy in memory.             */
  /*                                                                       */
  /*    length :: The length in bytes of the source object.                */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.  An error is returned if a  */
  /*    reallocation fails.                                                */
  /*                                                                       */
  FT_LOCAL_DEF( FT_Error )
  ps_table_add( PS_Table  table,
                FT_Int    idx,
                void*     object,
                FT_Int    length )
  {
    if ( idx < 0 || idx > table->max_elems )
    {
      FT_ERROR(( "ps_table_add: invalid index\n" ));
      return PSaux_Err_Invalid_Argument;
    }

    /* grow the base block if needed */
    if ( table->cursor + length > table->capacity )
    {
      FT_Error   error;
      FT_Offset  new_size  = table->capacity;
      FT_Long    in_offset;


      in_offset = (FT_Long)((FT_Byte*)object - table->block);
      if ( (FT_ULong)in_offset >= table->capacity )
        in_offset = -1;

      while ( new_size < table->cursor + length )
      {
        /* increase size by 25% and round up to the nearest multiple of 1024 */
        new_size += (new_size >> 2) + 1;
        new_size  = ( new_size + 1023 ) & -1024;
      }

      error = reallocate_t1_table( table, new_size );
      if ( error )
        return error;

      if ( in_offset >= 0 )
        object = table->block + in_offset;
    }

    /* add the object to the base block and adjust offset */
    table->elements[idx] = table->block + table->cursor;
    table->lengths [idx] = length;
    FT_MEM_COPY( table->block + table->cursor, object, length );

    table->cursor += length;
    return PSaux_Err_Ok;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    ps_table_done                                                      */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Finalizes a PS_TableRec (i.e., reallocate it to its current        */
  /*    cursor).                                                           */
  /*                                                                       */
  /* <InOut>                                                               */
  /*    table :: The target table.                                         */
  /*                                                                       */
  /* <Note>                                                                */
  /*    This function does NOT release the heap's memory block.  It is up  */
  /*    to the caller to clean it, or reference it in its own structures.  */
  /*                                                                       */
  FT_LOCAL_DEF( void )
  ps_table_done( PS_Table  table )
  {
    FT_Memory  memory = table->memory;
    FT_Error   error;
    FT_Byte*   old_base = table->block;


    /* should never fail, because rec.cursor <= rec.size */
    if ( !old_base )
      return;

    if ( FT_ALLOC( table->block, table->cursor ) )
      return;
    FT_MEM_COPY( table->block, old_base, table->cursor );
    shift_elements( table, old_base );

    table->capacity = table->cursor;
    FT_FREE( old_base );

    FT_UNUSED( error );
  }


  FT_LOCAL_DEF( void )
  ps_table_release( PS_Table  table )
  {
    FT_Memory  memory = table->memory;


    if ( (FT_ULong)table->init == 0xDEADBEEFUL )
    {
      FT_FREE( table->block );
      FT_FREE( table->elements );
      FT_FREE( table->lengths );
      table->init = 0;
    }
  }


  /*************************************************************************/
  /*************************************************************************/
  /*****                                                               *****/
  /*****                            T1 PARSER                          *****/
  /*****                                                               *****/
  /*************************************************************************/
  /*************************************************************************/


#define IS_T1_WHITESPACE( c )  ( (c) == ' '  || (c) == '\t' )
#define IS_T1_LINESPACE( c )   ( (c) == '\r' || (c) == '\n' )

#define IS_T1_SPACE( c )  ( IS_T1_WHITESPACE( c ) || IS_T1_LINESPACE( c ) )


  FT_LOCAL_DEF( void )
  ps_parser_skip_spaces( PS_Parser  parser )
  {
    FT_Byte* cur   = parser->cursor;
    FT_Byte* limit = parser->limit;


    while ( cur < limit )
    {
      FT_Byte  c = *cur;


      if ( !IS_T1_SPACE( c ) )
        break;
      cur++;
    }
    parser->cursor = cur;
  }


  FT_LOCAL_DEF( void )
  ps_parser_skip_alpha( PS_Parser  parser )
  {
    FT_Byte* cur   = parser->cursor;
    FT_Byte* limit = parser->limit;


    while ( cur < limit )
    {
      FT_Byte  c = *cur;


      if ( IS_T1_SPACE( c ) )
        break;
      cur++;
    }
    parser->cursor = cur;
  }


  FT_LOCAL_DEF( void )
  ps_parser_to_token( PS_Parser  parser,
                      T1_Token   token )
  {
    FT_Byte*  cur;
    FT_Byte*  limit;
    FT_Byte   starter, ender;
    FT_Int    embed;


    token->type  = T1_TOKEN_TYPE_NONE;
    token->start = 0;
    token->limit = 0;

    /* first of all, skip space */
    ps_parser_skip_spaces( parser );

    cur   = parser->cursor;
    limit = parser->limit;

    if ( cur < limit )
    {
      switch ( *cur )
      {
        /************* check for strings ***********************/
      case '(':
        token->type = T1_TOKEN_TYPE_STRING;
        ender = ')';
        goto Lookup_Ender;

        /************* check for programs/array ****************/
      case '{':
        token->type = T1_TOKEN_TYPE_ARRAY;
        ender = '}';
        goto Lookup_Ender;

        /************* check for table/array ******************/
      case '[':
        token->type = T1_TOKEN_TYPE_ARRAY;
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
        token->type  = T1_TOKEN_TYPE_ANY;
        while ( cur < limit && !IS_T1_SPACE( *cur ) )
          cur++;

        token->limit = cur;
      }

      if ( !token->limit )
      {
        token->start = 0;
        token->type  = T1_TOKEN_TYPE_NONE;
      }

      parser->cursor = cur;
    }
  }


  FT_LOCAL_DEF( void )
  ps_parser_to_token_array( PS_Parser  parser,
                            T1_Token   tokens,
                            FT_UInt    max_tokens,
                            FT_Int*    pnum_tokens )
  {
    T1_TokenRec  master;


    *pnum_tokens = -1;

    ps_parser_to_token( parser, &master );
    if ( master.type == T1_TOKEN_TYPE_ARRAY )
    {
      FT_Byte*  old_cursor = parser->cursor;
      FT_Byte*  old_limit  = parser->limit;
      T1_Token  cur        = tokens;
      T1_Token  limit      = cur + max_tokens;


      parser->cursor = master.start;
      parser->limit  = master.limit;

      while ( parser->cursor < parser->limit )
      {
        T1_TokenRec  token;


        ps_parser_to_token( parser, &token );
        if ( !token.type )
          break;

        if ( cur < limit )
          *cur = token;

        cur++;
      }

      *pnum_tokens = (FT_Int)( cur - tokens );

      parser->cursor = old_cursor;
      parser->limit  = old_limit;
    }
  }


  static FT_Long
  T1Radix( FT_Long    radixBase,
           FT_Byte**  cur,
           FT_Byte*   limit )
  {
    FT_Long  result = 0;
    FT_Byte  radixEndChar0 =
               (FT_Byte)( radixBase > 10 ? '9' + 1 : '0' + radixBase );
    FT_Byte  radixEndChar1 =
               (FT_Byte)( 'A' + radixBase - 10 );
    FT_Byte  radixEndChar2 =
               (FT_Byte)( 'a' + radixBase - 10 );


    while( *cur < limit )
    {
      if ( (*cur)[0] >= '0' && (*cur)[0] < radixEndChar0 )
        result = result * radixBase + (*cur)[0] - '0';

      else if ( radixBase > 10 &&
                (*cur)[0] >= 'A' && (*cur)[0] < radixEndChar1 )
        result = result * radixBase + ( (*cur)[0] - 'A' + 10 );

      else if ( radixBase > 10 &&
                (*cur)[0] >= 'a' && (*cur)[0] < radixEndChar2 )
        result = result * radixBase + ( (*cur)[0] - 'a' + 10 );

      else
        return result;

      (*cur)++;
    }

    return result;
  }


  static FT_Long
  t1_toint( FT_Byte**  cursor,
            FT_Byte*   limit )
  {
    FT_Long   result = 0;
    FT_Byte*  cur    = *cursor;
    FT_Byte   c      = '\0', d;


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
        {
          if ( cur[0] == '#' )
          {
            cur++;
            result = T1Radix( result, &cur, limit );
          }
          break;
        }

        result = result * 10 + d;
        cur++;

      } while ( cur < limit );

      if ( c == '-' )
        result = -result;
    }

    *cursor = cur;
    return result;
  }


  static FT_Long
  t1_tofixed( FT_Byte**  cursor,
              FT_Byte*   limit,
              FT_Long    power_ten )
  {
    FT_Byte*  cur  = *cursor;
    FT_Long   num, divider, result;
    FT_Int    sign = 0;
    FT_Byte   d;


    if ( cur >= limit )
      return 0;

    /* first of all, check the sign */
    if ( *cur == '-' )
    {
      sign = 1;
      cur++;
    }

    /* then, read the integer part, if any */
    if ( *cur != '.' )
      result = t1_toint( &cur, limit ) << 16;
    else
      result = 0;

    num     = 0;
    divider = 1;

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


  static FT_Int
  t1_tocoordarray( FT_Byte**  cursor,
                   FT_Byte*   limit,
                   FT_Int     max_coords,
                   FT_Short*  coords )
  {
    FT_Byte*  cur   = *cursor;
    FT_Int    count = 0;
    FT_Byte   c, ender;


    if ( cur >= limit )
      goto Exit;

    /* check for the beginning of an array; if not, only one number will */
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


  static FT_Int
  t1_tofixedarray( FT_Byte**  cursor,
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

  static FT_String*
  t1_tostring( FT_Byte**  cursor,
               FT_Byte*   limit,
               FT_Memory  memory )
  {
    FT_Byte*    cur = *cursor;
    FT_PtrDist  len = 0;
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
    if ( cur >= limit || FT_ALLOC( result, len + 1 ) )
      return 0;

    /* now copy the string */
    FT_MEM_COPY( result, *cursor, len );
    result[len] = '\0';
    *cursor = cur;
    return result;
  }

#endif /* 0 */


  static int
  t1_tobool( FT_Byte**  cursor,
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
  FT_LOCAL_DEF( FT_Error )
  ps_parser_load_field( PS_Parser       parser,
                        const T1_Field  field,
                        void**          objects,
                        FT_UInt         max_objects,
                        FT_ULong*       pflags )
  {
    T1_TokenRec  token;
    FT_Byte*     cur;
    FT_Byte*     limit;
    FT_UInt      count;
    FT_UInt      idx;
    FT_Error     error;


    ps_parser_to_token( parser, &token );
    if ( !token.type )
      goto Fail;

    count = 1;
    idx   = 0;
    cur   = token.start;
    limit = token.limit;

    /* we must detect arrays */
    if ( field->type == T1_FIELD_TYPE_BBOX )
    {
      T1_TokenRec  token2;
      FT_Byte*     old_cur   = parser->cursor;
      FT_Byte*     old_limit = parser->limit;


      parser->cursor = token.start;
      parser->limit  = token.limit;

      ps_parser_to_token( parser, &token2 );
      parser->cursor = old_cur;
      parser->limit  = old_limit;

      if ( token2.type == T1_TOKEN_TYPE_ARRAY )
        goto FieldArray;
    }
    else if ( token.type == T1_TOKEN_TYPE_ARRAY )
    {
    FieldArray:
      /* if this is an array, and we have no blend, an error occurs */
      if ( max_objects == 0 )
        goto Fail;

      count = max_objects;
      idx = 1;
    }

    for ( ; count > 0; count--, idx++ )
    {
      FT_Byte*    q = (FT_Byte*)objects[idx] + field->offset;
      FT_Long     val;
      FT_String*  string;


      switch ( field->type )
      {
      case T1_FIELD_TYPE_BOOL:
        val = t1_tobool( &cur, limit );
        goto Store_Integer;

      case T1_FIELD_TYPE_FIXED:
        val = t1_tofixed( &cur, limit, 3 );
        goto Store_Integer;

      case T1_FIELD_TYPE_INTEGER:
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

        default:                /* for 64-bit systems */
          *(FT_Long*)q = val;
        }
        break;

      case T1_FIELD_TYPE_STRING:
        {
          FT_Memory  memory = parser->memory;
          FT_UInt    len    = (FT_UInt)( limit - cur );


          if ( *(FT_String**)q )
            /* with synthetic fonts, it's possible to find a field twice */
            break;

          if ( FT_ALLOC( string, len + 1 ) )
            goto Exit;

          FT_MEM_COPY( string, cur, len );
          string[len] = 0;

          *(FT_String**)q = string;
        }
        break;

      case T1_FIELD_TYPE_BBOX:
        {
          FT_Fixed  temp[4];
          FT_BBox*  bbox = (FT_BBox*)q;


          /* we need the '[' and ']' delimiters */
          token.start--;
          token.limit++;
          (void)t1_tofixedarray( &token.start, token.limit, 4, temp, 0 );

          bbox->xMin = FT_RoundFix( temp[0] );
          bbox->yMin = FT_RoundFix( temp[1] );
          bbox->xMax = FT_RoundFix( temp[2] );
          bbox->yMax = FT_RoundFix( temp[3] );
        }
        break;

      default:
        /* an error occured */
        goto Fail;
      }
    }

#if 0  /* obsolete - keep for reference */
    if ( pflags )
      *pflags |= 1L << field->flag_bit;
#else
    FT_UNUSED( pflags );
#endif

    error = PSaux_Err_Ok;

  Exit:
    return error;

  Fail:
    error = PSaux_Err_Invalid_File_Format;
    goto Exit;
  }


#define T1_MAX_TABLE_ELEMENTS  32


  FT_LOCAL_DEF( FT_Error )
  ps_parser_load_field_table( PS_Parser       parser,
                              const T1_Field  field,
                              void**          objects,
                              FT_UInt         max_objects,
                              FT_ULong*       pflags )
  {
    T1_TokenRec  elements[T1_MAX_TABLE_ELEMENTS];
    T1_Token     token;
    FT_Int       num_elements;
    FT_Error     error = 0;
    FT_Byte*     old_cursor;
    FT_Byte*     old_limit;
    T1_FieldRec  fieldrec = *(T1_Field)field;


#if 1
    fieldrec.type = T1_FIELD_TYPE_INTEGER;
    if ( field->type == T1_FIELD_TYPE_FIXED_ARRAY )
      fieldrec.type = T1_FIELD_TYPE_FIXED;
#endif

    ps_parser_to_token_array( parser, elements, 32, &num_elements );
    if ( num_elements < 0 )
      goto Fail;

    if ( num_elements > T1_MAX_TABLE_ELEMENTS )
      num_elements = T1_MAX_TABLE_ELEMENTS;

    old_cursor = parser->cursor;
    old_limit  = parser->limit;

    /* we store the elements count */
    *(FT_Byte*)( (FT_Byte*)objects[0] + field->count_offset ) =
      (FT_Byte)num_elements;

    /* we now load each element, adjusting the field.offset on each one */
    token = elements;
    for ( ; num_elements > 0; num_elements--, token++ )
    {
      parser->cursor = token->start;
      parser->limit  = token->limit;
      ps_parser_load_field( parser, &fieldrec, objects, max_objects, 0 );
      fieldrec.offset += fieldrec.size;
    }

#if 0  /* obsolete -- keep for reference */
    if ( pflags )
      *pflags |= 1L << field->flag_bit;
#else
    FT_UNUSED( pflags );
#endif

    parser->cursor = old_cursor;
    parser->limit  = old_limit;

  Exit:
    return error;

  Fail:
    error = PSaux_Err_Invalid_File_Format;
    goto Exit;
  }


  FT_LOCAL_DEF( FT_Long )
  ps_parser_to_int( PS_Parser  parser )
  {
    return t1_toint( &parser->cursor, parser->limit );
  }


  FT_LOCAL_DEF( FT_Fixed )
  ps_parser_to_fixed( PS_Parser  parser,
                      FT_Int     power_ten )
  {
    return t1_tofixed( &parser->cursor, parser->limit, power_ten );
  }


  FT_LOCAL_DEF( FT_Int )
  ps_parser_to_coord_array( PS_Parser  parser,
                            FT_Int     max_coords,
                            FT_Short*  coords )
  {
    return t1_tocoordarray( &parser->cursor, parser->limit,
                            max_coords, coords );
  }


  FT_LOCAL_DEF( FT_Int )
  ps_parser_to_fixed_array( PS_Parser  parser,
                            FT_Int     max_values,
                            FT_Fixed*  values,
                            FT_Int     power_ten )
  {
    return t1_tofixedarray( &parser->cursor, parser->limit,
                            max_values, values, power_ten );
  }


#if 0

  FT_LOCAL_DEF( FT_String* )
  T1_ToString( PS_Parser  parser )
  {
    return t1_tostring( &parser->cursor, parser->limit, parser->memory );
  }


  FT_LOCAL_DEF( FT_Bool )
  T1_ToBool( PS_Parser  parser )
  {
    return t1_tobool( &parser->cursor, parser->limit );
  }

#endif /* 0 */


  FT_LOCAL_DEF( void )
  ps_parser_init( PS_Parser  parser,
                  FT_Byte*   base,
                  FT_Byte*   limit,
                  FT_Memory  memory )
  {
    parser->error  = 0;
    parser->base   = base;
    parser->limit  = limit;
    parser->cursor = base;
    parser->memory = memory;
    parser->funcs  = ps_parser_funcs;
  }


  FT_LOCAL_DEF( void )
  ps_parser_done( PS_Parser  parser )
  {
    FT_UNUSED( parser );
  }


  /*************************************************************************/
  /*************************************************************************/
  /*****                                                               *****/
  /*****                            T1 BUILDER                         *****/
  /*****                                                               *****/
  /*************************************************************************/
  /*************************************************************************/

  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    t1_builder_init                                                    */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Initializes a given glyph builder.                                 */
  /*                                                                       */
  /* <InOut>                                                               */
  /*    builder :: A pointer to the glyph builder to initialize.           */
  /*                                                                       */
  /* <Input>                                                               */
  /*    face    :: The current face object.                                */
  /*                                                                       */
  /*    size    :: The current size object.                                */
  /*                                                                       */
  /*    glyph   :: The current glyph object.                               */
  /*                                                                       */
  /*    hinting :: Whether hinting should be applied.                      */
  /*                                                                       */
  FT_LOCAL_DEF( void )
  t1_builder_init( T1_Builder    builder,
                   FT_Face       face,
                   FT_Size       size,
                   FT_GlyphSlot  glyph,
                   FT_Bool       hinting )
  {
    builder->path_begun  = 0;
    builder->load_points = 1;

    builder->face   = face;
    builder->glyph  = glyph;
    builder->memory = face->memory;

    if ( glyph )
    {
      FT_GlyphLoader  loader = glyph->internal->loader;


      builder->loader  = loader;
      builder->base    = &loader->base.outline;
      builder->current = &loader->current.outline;
      FT_GlyphLoader_Rewind( loader );

      builder->hints_globals = size->internal;
      builder->hints_funcs   = 0;

      if ( hinting )
        builder->hints_funcs = glyph->internal->glyph_hints;
    }

    if ( size )
    {
      builder->scale_x = size->metrics.x_scale;
      builder->scale_y = size->metrics.y_scale;
    }

    builder->pos_x = 0;
    builder->pos_y = 0;

    builder->left_bearing.x = 0;
    builder->left_bearing.y = 0;
    builder->advance.x      = 0;
    builder->advance.y      = 0;

    builder->funcs = t1_builder_funcs;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    t1_builder_done                                                    */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Finalizes a given glyph builder.  Its contents can still be used   */
  /*    after the call, but the function saves important information       */
  /*    within the corresponding glyph slot.                               */
  /*                                                                       */
  /* <Input>                                                               */
  /*    builder :: A pointer to the glyph builder to finalize.             */
  /*                                                                       */
  FT_LOCAL_DEF( void )
  t1_builder_done( T1_Builder  builder )
  {
    FT_GlyphSlot  glyph = builder->glyph;


    if ( glyph )
      glyph->outline = *builder->base;
  }


  /* check that there is enough space for `count' more points */
  FT_LOCAL_DEF( FT_Error )
  t1_builder_check_points( T1_Builder  builder,
                           FT_Int      count )
  {
    return FT_GlyphLoader_CheckPoints( builder->loader, count, 0 );
  }


  /* add a new point, do not check space */
  FT_LOCAL_DEF( void )
  t1_builder_add_point( T1_Builder  builder,
                        FT_Pos      x,
                        FT_Pos      y,
                        FT_Byte     flag )
  {
    FT_Outline*  outline = builder->current;


    if ( builder->load_points )
    {
      FT_Vector*  point   = outline->points + outline->n_points;
      FT_Byte*    control = (FT_Byte*)outline->tags + outline->n_points;


      if ( builder->shift )
      {
        x >>= 16;
        y >>= 16;
      }
      point->x = x;
      point->y = y;
      *control = (FT_Byte)( flag ? FT_CURVE_TAG_ON : FT_CURVE_TAG_CUBIC );

      builder->last = *point;
    }
    outline->n_points++;
  }


  /* check space for a new on-curve point, then add it */
  FT_LOCAL_DEF( FT_Error )
  t1_builder_add_point1( T1_Builder  builder,
                         FT_Pos      x,
                         FT_Pos      y )
  {
    FT_Error  error;


    error = t1_builder_check_points( builder, 1 );
    if ( !error )
      t1_builder_add_point( builder, x, y, 1 );

    return error;
  }


  /* check room for a new contour, then add it */
  FT_LOCAL_DEF( FT_Error )
  t1_builder_add_contour( T1_Builder  builder )
  {
    FT_Outline*  outline = builder->current;
    FT_Error     error;


    if ( !builder->load_points )
    {
      outline->n_contours++;
      return PSaux_Err_Ok;
    }

    error = FT_GlyphLoader_CheckPoints( builder->loader, 0, 1 );
    if ( !error )
    {
      if ( outline->n_contours > 0 )
        outline->contours[outline->n_contours - 1] =
          (short)( outline->n_points - 1 );

      outline->n_contours++;
    }

    return error;
  }


  /* if a path was begun, add its first on-curve point */
  FT_LOCAL_DEF( FT_Error )
  t1_builder_start_point( T1_Builder  builder,
                          FT_Pos      x,
                          FT_Pos      y )
  {
    FT_Error  error = 0;


    /* test whether we are building a new contour */
    if ( !builder->path_begun )
    {
      builder->path_begun = 1;
      error = t1_builder_add_contour( builder );
      if ( !error )
        error = t1_builder_add_point1( builder, x, y );
    }
    return error;
  }


  /* close the current contour */
  FT_LOCAL_DEF( void )
  t1_builder_close_contour( T1_Builder  builder )
  {
    FT_Outline*  outline = builder->current;


    /* XXXX: We must not include the last point in the path if it */
    /*       is located on the first point.                       */
    if ( outline->n_points > 1 )
    {
      FT_Int      first   = 0;
      FT_Vector*  p1      = outline->points + first;
      FT_Vector*  p2      = outline->points + outline->n_points - 1;
      FT_Byte*    control = (FT_Byte*)outline->tags + outline->n_points - 1;


      if ( outline->n_contours > 1 )
      {
        first = outline->contours[outline->n_contours - 2] + 1;
        p1    = outline->points + first;
      }

      /* `delete' last point only if it coincides with the first */
      /* point and it is not a control point (which can happen). */
      if ( p1->x == p2->x && p1->y == p2->y )
        if ( *control == FT_CURVE_TAG_ON )
          outline->n_points--;
    }

    if ( outline->n_contours > 0 )
      outline->contours[outline->n_contours - 1] =
        (short)( outline->n_points - 1 );
  }


  /*************************************************************************/
  /*************************************************************************/
  /*****                                                               *****/
  /*****                            OTHER                              *****/
  /*****                                                               *****/
  /*************************************************************************/
  /*************************************************************************/

  FT_LOCAL_DEF( void )
  t1_decrypt( FT_Byte*   buffer,
              FT_Offset  length,
              FT_UShort  seed )
  {
    while ( length > 0 )
    {
      FT_Byte  plain;


      plain     = (FT_Byte)( *buffer ^ ( seed >> 8 ) );
      seed      = (FT_UShort)( ( *buffer + seed ) * 52845U + 22719 );
      *buffer++ = plain;
      length--;
    }
  }


/* END */
