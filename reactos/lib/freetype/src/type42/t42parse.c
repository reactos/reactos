/***************************************************************************/
/*                                                                         */
/*  t42parse.c                                                             */
/*                                                                         */
/*    Type 42 font parser (body).                                          */
/*                                                                         */
/*  Copyright 2002, 2003 by Roberto Alameda.                               */
/*                                                                         */
/*  This file is part of the FreeType project, and may only be used,       */
/*  modified, and distributed under the terms of the FreeType project      */
/*  license, LICENSE.TXT.  By continuing to use, modify, or distribute     */
/*  this file you indicate that you have read the license and              */
/*  understand and accept it fully.                                        */
/*                                                                         */
/***************************************************************************/


#include "t42parse.h"
#include "t42error.h"
#include FT_INTERNAL_DEBUG_H
#include FT_INTERNAL_STREAM_H
#include FT_LIST_H
#include FT_INTERNAL_POSTSCRIPT_AUX_H


  /*************************************************************************/
  /*                                                                       */
  /* The macro FT_COMPONENT is used in trace mode.  It is an implicit      */
  /* parameter of the FT_TRACE() and FT_ERROR() macros, used to print/log  */
  /* messages during execution.                                            */
  /*                                                                       */
#undef  FT_COMPONENT
#define FT_COMPONENT  trace_t42


  static void
  t42_parse_font_matrix( T42_Face    face,
                         T42_Loader  loader );
  static void
  t42_parse_encoding( T42_Face    face,
                      T42_Loader  loader );

  static void
  t42_parse_charstrings( T42_Face    face,
                         T42_Loader  loader );

  static void
  t42_parse_sfnts( T42_Face    face,
                   T42_Loader  loader );


  static const
  T1_FieldRec  t42_keywords[] = {

#undef  FT_STRUCTURE
#define FT_STRUCTURE  T1_FontInfo
#undef  T1CODE
#define T1CODE        T1_FIELD_LOCATION_FONT_INFO

    T1_FIELD_STRING( "version",            version )
    T1_FIELD_STRING( "Notice",             notice )
    T1_FIELD_STRING( "FullName",           full_name )
    T1_FIELD_STRING( "FamilyName",         family_name )
    T1_FIELD_STRING( "Weight",             weight )
    T1_FIELD_NUM   ( "ItalicAngle",        italic_angle )
    T1_FIELD_BOOL  ( "isFixedPitch",       is_fixed_pitch )
    T1_FIELD_NUM   ( "UnderlinePosition",  underline_position )
    T1_FIELD_NUM   ( "UnderlineThickness", underline_thickness )

#undef  FT_STRUCTURE
#define FT_STRUCTURE  T1_FontRec
#undef  T1CODE
#define T1CODE        T1_FIELD_LOCATION_FONT_DICT

    T1_FIELD_KEY  ( "FontName",    font_name )
    T1_FIELD_NUM  ( "PaintType",   paint_type )
    T1_FIELD_NUM  ( "FontType",    font_type )
    T1_FIELD_FIXED( "StrokeWidth", stroke_width )

#undef  FT_STRUCTURE
#define FT_STRUCTURE  FT_BBox
#undef  T1CODE
#define T1CODE        T1_FIELD_LOCATION_BBOX

    T1_FIELD_BBOX("FontBBox", xMin )

    T1_FIELD_CALLBACK( "FontMatrix",  t42_parse_font_matrix )
    T1_FIELD_CALLBACK( "Encoding",    t42_parse_encoding )
    T1_FIELD_CALLBACK( "CharStrings", t42_parse_charstrings )
    T1_FIELD_CALLBACK( "sfnts",       t42_parse_sfnts )

    { 0, T1_FIELD_LOCATION_CID_INFO, T1_FIELD_TYPE_NONE, 0, 0, 0, 0, 0 }
  };


#define T42_KEYWORD_COUNT                                          \
          ( sizeof ( t42_keywords ) / sizeof ( t42_keywords[0] ) )


#define T1_Add_Table( p, i, o, l )  (p)->funcs.add( (p), i, o, l )
#define T1_Done_Table( p )          \
          do                        \
          {                         \
            if ( (p)->funcs.done )  \
              (p)->funcs.done( p ); \
          } while ( 0 )
#define T1_Release_Table( p )          \
          do                           \
          {                            \
            if ( (p)->funcs.release )  \
              (p)->funcs.release( p ); \
          } while ( 0 )

#define T1_Skip_Spaces( p )  (p)->root.funcs.skip_spaces( &(p)->root )
#define T1_Skip_Alpha( p )   (p)->root.funcs.skip_alpha ( &(p)->root )

#define T1_ToInt( p )       (p)->root.funcs.to_int( &(p)->root )
#define T1_ToFixed( p, t )  (p)->root.funcs.to_fixed( &(p)->root, t )

#define T1_ToCoordArray( p, m, c )                           \
          (p)->root.funcs.to_coord_array( &(p)->root, m, c )
#define T1_ToFixedArray( p, m, f, t )                           \
          (p)->root.funcs.to_fixed_array( &(p)->root, m, f, t )
#define T1_ToToken( p, t )                          \
          (p)->root.funcs.to_token( &(p)->root, t )
#define T1_ToTokenArray( p, t, m, c )                           \
          (p)->root.funcs.to_token_array( &(p)->root, t, m, c )

#define T1_Load_Field( p, f, o, m, pf )                         \
          (p)->root.funcs.load_field( &(p)->root, f, o, m, pf )
#define T1_Load_Field_Table( p, f, o, m, pf )                         \
          (p)->root.funcs.load_field_table( &(p)->root, f, o, m, pf )


  /********************* Parsing Functions ******************/

  FT_LOCAL_DEF( FT_Error )
  t42_parser_init( T42_Parser     parser,
                   FT_Stream      stream,
                   FT_Memory      memory,
                   PSAux_Service  psaux )
  {
    FT_Error  error = T42_Err_Ok;
    FT_Long   size;


    psaux->ps_parser_funcs->init( &parser->root, 0, 0, memory );

    parser->stream    = stream;
    parser->base_len  = 0;
    parser->base_dict = 0;
    parser->in_memory = 0;

    /*******************************************************************/
    /*                                                                 */
    /* Here a short summary of what is going on:                       */
    /*                                                                 */
    /*   When creating a new Type 42 parser, we try to locate and load */
    /*   the base dictionary, loading the whole font into memory.      */
    /*                                                                 */
    /*   When `loading' the base dictionary, we only setup pointers in */
    /*   the case of a memory-based stream.  Otherwise, we allocate    */
    /*   and load the base dictionary in it.                           */
    /*                                                                 */
    /*   parser->in_memory is set if we have a memory stream.          */
    /*                                                                 */

    if ( FT_STREAM_SEEK( 0L ) )
      goto Exit;

    size = stream->size;

    /* now, try to load `size' bytes of the `base' dictionary we */
    /* found previously                                          */

    /* if it is a memory-based resource, set up pointers */
    if ( !stream->read )
    {
      parser->base_dict = (FT_Byte*)stream->base + stream->pos;
      parser->base_len  = size;
      parser->in_memory = 1;

      /* check that the `size' field is valid */
      if ( FT_STREAM_SKIP( size ) )
        goto Exit;
    }
    else
    {
      /* read segment in memory */
      if ( FT_ALLOC( parser->base_dict, size )       ||
           FT_STREAM_READ( parser->base_dict, size ) )
        goto Exit;

      parser->base_len = size;
    }

    /* Now check font format; we must see `%!PS-TrueTypeFont' */
    if (size <= 17                                    ||
        ( ft_strncmp( (const char*)parser->base_dict,
                      "%!PS-TrueTypeFont", 17) )      )
      error = T42_Err_Unknown_File_Format;
    else
    {
      parser->root.base   = parser->base_dict;
      parser->root.cursor = parser->base_dict;
      parser->root.limit  = parser->root.cursor + parser->base_len;
    }

  Exit:
    if ( error && !parser->in_memory )
      FT_FREE( parser->base_dict );

    return error;
  }


  FT_LOCAL_DEF( void )
  t42_parser_done( T42_Parser  parser )
  {
    FT_Memory  memory = parser->root.memory;


    /* free the base dictionary only when we have a disk stream */
    if ( !parser->in_memory )
      FT_FREE( parser->base_dict );

    parser->root.funcs.done( &parser->root );
  }


  static int
  t42_is_alpha( FT_Byte  c )
  {
    /* Note: we must accept "+" as a valid character, as it is used in */
    /*       embedded type1 fonts in PDF documents.                    */
    /*                                                                 */
    return ( ft_isalnum( c ) ||
             c == '.'        ||
             c == '_'        ||
             c == '-'        ||
             c == '+'        );
  }


  static int
  t42_is_space( FT_Byte  c )
  {
    return ( c == ' ' || c == '\t' || c == '\r' || c == '\n' );
  }


  static void
  t42_parse_font_matrix( T42_Face    face,
                         T42_Loader  loader )
  {
    T42_Parser  parser = &loader->parser;
    FT_Matrix*  matrix = &face->type1.font_matrix;
    FT_Vector*  offset = &face->type1.font_offset;
    FT_Face     root   = (FT_Face)&face->root;
    FT_Fixed    temp[6];
    FT_Fixed    temp_scale;


    (void)T1_ToFixedArray( parser, 6, temp, 3 );

    temp_scale = ABS( temp[3] );

    /* Set Units per EM based on FontMatrix values.  We set the value to */
    /* 1000 / temp_scale, because temp_scale was already multiplied by   */
    /* 1000 (in t1_tofixed, from psobjs.c).                              */

    root->units_per_EM = (FT_UShort)( FT_DivFix( 1000 * 0x10000L,
                                                 temp_scale ) >> 16 );

    /* we need to scale the values by 1.0/temp_scale */
    if ( temp_scale != 0x10000L ) {
      temp[0] = FT_DivFix( temp[0], temp_scale );
      temp[1] = FT_DivFix( temp[1], temp_scale );
      temp[2] = FT_DivFix( temp[2], temp_scale );
      temp[4] = FT_DivFix( temp[4], temp_scale );
      temp[5] = FT_DivFix( temp[5], temp_scale );
      temp[3] = 0x10000L;
    }

    matrix->xx = temp[0];
    matrix->yx = temp[1];
    matrix->xy = temp[2];
    matrix->yy = temp[3];

    /* note that the offsets must be expressed in integer font units */
    offset->x  = temp[4] >> 16;
    offset->y  = temp[5] >> 16;
  }


  static void
  t42_parse_encoding( T42_Face    face,
                      T42_Loader  loader )
  {
    T42_Parser     parser = &loader->parser;
    FT_Byte*       cur    = parser->root.cursor;
    FT_Byte*       limit  = parser->root.limit;

    PSAux_Service  psaux  = (PSAux_Service)face->psaux;


    /* skip whitespace */
    while ( t42_is_space( *cur ) )
    {
      cur++;
      if ( cur >= limit )
      {
        FT_ERROR(( "t42_parse_encoding: out of bounds!\n" ));
        parser->root.error = T42_Err_Invalid_File_Format;
        return;
      }
    }

    /* if we have a number, then the encoding is an array, */
    /* and we must load it now                             */
    if ( (FT_Byte)( *cur - '0' ) < 10 )
    {
      T1_Encoding  encode     = &face->type1.encoding;
      FT_Int       count, n;
      PS_Table     char_table = &loader->encoding_table;
      FT_Memory    memory     = parser->root.memory;
      FT_Error     error;


      /* read the number of entries in the encoding, should be 256 */
      count = (FT_Int)T1_ToInt( parser );
      if ( parser->root.error )
        return;

      /* we use a T1_Table to store our charnames */
      loader->num_chars = encode->num_chars = count;
      if ( FT_NEW_ARRAY( encode->char_index, count ) ||
           FT_NEW_ARRAY( encode->char_name,  count ) ||
           FT_SET_ERROR( psaux->ps_table_funcs->init(
                           char_table, count, memory ) ) )
      {
        parser->root.error = error;
        return;
      }

      /* We need to `zero' out encoding_table.elements */
      for ( n = 0; n < count; n++ )
      {
        char*  notdef = (char *)".notdef";


        T1_Add_Table( char_table, n, notdef, 8 );
      }

      /* Now, we will need to read a record of the form         */
      /* ... charcode /charname ... for each entry in our table */
      /*                                                        */
      /* We simply look for a number followed by an immediate   */
      /* name.  Note that this ignores correctly the sequence   */
      /* that is often seen in type1 fonts:                     */
      /*                                                        */
      /*   0 1 255 { 1 index exch /.notdef put } for dup        */
      /*                                                        */
      /* used to clean the encoding array before anything else. */
      /*                                                        */
      /* We stop when we encounter a `def'.                     */

      cur   = parser->root.cursor;
      limit = parser->root.limit;
      n     = 0;

      for ( ; cur < limit; )
      {
        FT_Byte  c;


        c = *cur;

        /* we stop when we encounter a `def' */
        if ( c == 'd' && cur + 3 < limit )
        {
          if ( cur[1] == 'e'       &&
               cur[2] == 'f'       &&
               t42_is_space( cur[-1] ) &&
               t42_is_space( cur[3] )  )
          {
            FT_TRACE6(( "encoding end\n" ));
            break;
          }
        }

        /* otherwise, we must find a number before anything else */
        if ( (FT_Byte)( c - '0' ) < 10 )
        {
          FT_Int  charcode;


          parser->root.cursor = cur;
          charcode = (FT_Int)T1_ToInt( parser );
          cur      = parser->root.cursor;

          /* skip whitespace */
          while ( cur < limit && t42_is_space( *cur ) )
            cur++;

          if ( cur < limit && *cur == '/' )
          {
            /* bingo, we have an immediate name -- it must be a */
            /* character name                                   */
            FT_Byte*  cur2 = cur + 1;
            FT_Int    len;


            while ( cur2 < limit && t42_is_alpha( *cur2 ) )
              cur2++;

            len = (FT_Int)( cur2 - cur - 1 );

            parser->root.error = T1_Add_Table( char_table, charcode,
                                               cur + 1, len + 1 );
            char_table->elements[charcode][len] = '\0';
            if ( parser->root.error )
              return;

            cur = cur2;
          }
        }
        else
          cur++;
      }

      face->type1.encoding_type  = T1_ENCODING_TYPE_ARRAY;
      parser->root.cursor        = cur;
    }
    /* Otherwise, we should have either `StandardEncoding', */
    /* `ExpertEncoding', or `ISOLatin1Encoding'             */
    else
    {
      if ( cur + 17 < limit                                            &&
           ft_strncmp( (const char*)cur, "StandardEncoding", 16 ) == 0 )
        face->type1.encoding_type = T1_ENCODING_TYPE_STANDARD;

      else if ( cur + 15 < limit                                          &&
                ft_strncmp( (const char*)cur, "ExpertEncoding", 14 ) == 0 )
        face->type1.encoding_type = T1_ENCODING_TYPE_EXPERT;

      else if ( cur + 18 < limit                                             &&
                ft_strncmp( (const char*)cur, "ISOLatin1Encoding", 17 ) == 0 )
        face->type1.encoding_type = T1_ENCODING_TYPE_ISOLATIN1;

      else {
        FT_ERROR(( "t42_parse_encoding: invalid token!\n" ));
        parser->root.error = T42_Err_Invalid_File_Format;
      }
    }
  }


  static FT_UInt
  t42_hexval( FT_Byte  v )
  {
    FT_UInt  d;

    d = (FT_UInt)( v - 'A' );
    if ( d < 6 )
    {
      d += 10;
      goto Exit;
    }

    d = (FT_UInt)( v - 'a' );
    if ( d < 6 )
    {
      d += 10;
      goto Exit;
    }

    d = (FT_UInt)( v - '0' );
    if ( d < 10 )
      goto Exit;

    d = 0;

  Exit:
    return d;
  }


  static void
  t42_parse_sfnts( T42_Face    face,
                   T42_Loader  loader )
  {
    T42_Parser  parser = &loader->parser;
    FT_Memory   memory = parser->root.memory;
    FT_Byte*    cur    = parser->root.cursor;
    FT_Byte*    limit  = parser->root.limit;
    FT_Error    error;
    FT_Int      num_tables = 0, status;
    FT_ULong    count, ttf_size = 0, string_size = 0;
    FT_Bool     in_string  = 0;
    FT_Byte     v = 0;


    /* The format is `/sfnts [ <...> <...> ... ] def' */

    while ( t42_is_space( *cur ) )
      cur++;

    if (*cur++ == '[')
    {
      status = 0;
      count = 0;
    }
    else
    {
      FT_ERROR(( "t42_parse_sfnts: can't find begin of sfnts vector!\n" ));
      error = T42_Err_Invalid_File_Format;
      goto Fail;
    }

    while ( cur < limit - 2 )
    {
      while ( t42_is_space( *cur ) )
        cur++;

      switch ( *cur )
      {
      case ']':
        parser->root.cursor = cur++;
        return;

      case '<':
        in_string   = 1;
        string_size = 0;
        cur++;
        continue;

      case '>':
        if ( !in_string )
        {
          FT_ERROR(( "t42_parse_sfnts: found unpaired `>'!\n" ));
          error = T42_Err_Invalid_File_Format;
          goto Fail;
        }

        /* A string can have, as a last byte,         */
        /* a zero byte for padding.  If so, ignore it */
        if ( ( v == 0 ) && ( string_size % 2 == 1 ) )
          count--;
        in_string = 0;
        cur++;
        continue;

      case '%':
        if ( !in_string )
        {
          /* Comment found; skip till end of line */
          while ( *cur != '\n' )
            cur++;
          continue;
        }
        else
        {
          FT_ERROR(( "t42_parse_sfnts: found `%' in string!\n" ));
          error = T42_Err_Invalid_File_Format;
          goto Fail;
        }

      default:
        if ( !ft_xdigit( *cur ) || !ft_xdigit( *(cur + 1) ) )
        {
          FT_ERROR(( "t42_parse_sfnts: found non-hex characters in string" ));
          error = T42_Err_Invalid_File_Format;
          goto Fail;
        }

        v = (FT_Byte)( 16 * t42_hexval( cur[0] ) + t42_hexval( cur[1] ) );
        cur += 2;
        string_size++;
      }

      switch ( status )
      {
      case 0: /* The '[' was read, so load offset table, 12 bytes */
        if ( count < 12 )
        {
          face->ttf_data[count++] = v;
          continue;
        }
        else
        {
          num_tables = 16 * face->ttf_data[4] + face->ttf_data[5];
          status     = 1;
          ttf_size   = 12 + 16 * num_tables;

          if ( FT_REALLOC( face->ttf_data, 12, ttf_size ) )
            goto Fail;
        }
        /* No break, fall-through */

      case 1: /* The offset table is read; read now the table directory */
        if ( count < ttf_size )
        {
          face->ttf_data[count++] = v;
          continue;
        }
        else
        {
          int      i;
          FT_ULong len;


          for ( i = 0; i < num_tables; i++ )
          {
            FT_Byte*  p = face->ttf_data + 12 + 16*i + 12;

            len = FT_PEEK_ULONG( p );

            /* Pad to a 4-byte boundary length */
            ttf_size += ( len + 3 ) & ~3;
          }

          status         = 2;
          face->ttf_size = ttf_size;

          if ( FT_REALLOC( face->ttf_data, 12 + 16 * num_tables,
                           ttf_size + 1 ) )
            goto Fail;
        }
        /* No break, fall-through */

      case 2: /* We are reading normal tables; just swallow them */
        face->ttf_data[count++] = v;

      }
    }

    /* If control reaches this point, the format was not valid */
    error = T42_Err_Invalid_File_Format;

  Fail:
    parser->root.error = error;
  }


  static void
  t42_parse_charstrings( T42_Face    face,
                         T42_Loader  loader )
  {
    T42_Parser     parser     = &loader->parser;
    PS_Table       code_table = &loader->charstrings;
    PS_Table       name_table = &loader->glyph_names;
    FT_Memory      memory     = parser->root.memory;
    FT_Error       error;

    PSAux_Service  psaux      = (PSAux_Service)face->psaux;

    FT_Byte*       cur;
    FT_Byte*       limit      = parser->root.limit;
    FT_Int         n;


    loader->num_glyphs = (FT_Int)T1_ToInt( parser );
    if ( parser->root.error )
      return;

    /* initialize tables */

    error = psaux->ps_table_funcs->init( code_table,
                                         loader->num_glyphs,
                                         memory );
    if ( error )
      goto Fail;

    error = psaux->ps_table_funcs->init( name_table,
                                         loader->num_glyphs,
                                         memory );
    if ( error )
      goto Fail;

    n = 0;

    for (;;)
    {
      /* the format is simple:                    */
      /*   `/glyphname' + index + def             */
      /*                                          */
      /* note that we stop when we find an `end'  */
      /*                                          */
      T1_Skip_Spaces( parser );

      cur = parser->root.cursor;
      if ( cur >= limit )
        break;

      /* we stop when we find an `end' keyword */
      if ( *cur   == 'e'   &&
           cur + 3 < limit &&
           cur[1] == 'n'   &&
           cur[2] == 'd'   )
        break;

      if ( *cur != '/' )
        T1_Skip_Alpha( parser );
      else
      {
        FT_Byte*  cur2 = cur + 1;
        FT_Int    len;


        while ( cur2 < limit && t42_is_alpha( *cur2 ) )
          cur2++;
        len = (FT_Int)( cur2 - cur - 1 );

        error = T1_Add_Table( name_table, n, cur + 1, len + 1 );
        if ( error )
          goto Fail;

        /* add a trailing zero to the name table */
        name_table->elements[n][len] = '\0';

        parser->root.cursor = cur2;
        T1_Skip_Spaces( parser );

        cur2 = cur = parser->root.cursor;
        if ( cur >= limit )
          break;

        while ( cur2 < limit && t42_is_alpha( *cur2 ) )
          cur2++;
        len = (FT_Int)( cur2 - cur );

        error = T1_Add_Table( code_table, n, cur, len + 1 );
        if ( error )
          goto Fail;

        code_table->elements[n][len] = '\0';

        n++;
        if ( n >= loader->num_glyphs )
          break;
      }
    }

    /* Index 0 must be a .notdef element */
    if ( ft_strcmp( (char *)name_table->elements[0], ".notdef" ) )
    {
      FT_ERROR(( "t42_parse_charstrings: Index 0 is not `.notdef'!\n" ));
      error = T42_Err_Invalid_File_Format;
      goto Fail;
    }

    loader->num_glyphs = n;
    return;

  Fail:
    parser->root.error = error;
  }


  static FT_Error
  t42_load_keyword( T42_Face    face,
                    T42_Loader  loader,
                    T1_Field    field )
  {
    FT_Error  error;
    void*     dummy_object;
    void**    objects;
    FT_UInt   max_objects = 0;


    /* if the keyword has a dedicated callback, call it */
    if ( field->type == T1_FIELD_TYPE_CALLBACK ) {
      field->reader( (FT_Face)face, loader );
      error = loader->parser.root.error;
      goto Exit;
    }

    /* now, the keyword is either a simple field, or a table of fields; */
    /* we are now going to take care of it                              */
    switch ( field->location )
    {
    case T1_FIELD_LOCATION_FONT_INFO:
      dummy_object = &face->type1.font_info;
      break;

    case T1_FIELD_LOCATION_BBOX:
      dummy_object = &face->type1.font_bbox;
      break;

    default:
      dummy_object = &face->type1;
    }

    objects = &dummy_object;

    if ( field->type == T1_FIELD_TYPE_INTEGER_ARRAY ||
         field->type == T1_FIELD_TYPE_FIXED_ARRAY   )
      error = T1_Load_Field_Table( &loader->parser, field,
                                   objects, max_objects, 0 );
    else
      error = T1_Load_Field( &loader->parser, field,
                             objects, max_objects, 0 );

   Exit:
    return error;
  }


  FT_LOCAL_DEF( FT_Error )
  t42_parse_dict( T42_Face    face,
                  T42_Loader  loader,
                  FT_Byte*    base,
                  FT_Long     size )
  {
    T42_Parser  parser     = &loader->parser;
    FT_Byte*    cur        = base;
    FT_Byte*    limit      = cur + size;
    FT_UInt     n_keywords = (FT_UInt)( sizeof ( t42_keywords ) /
                                        sizeof ( t42_keywords[0] ) );

    FT_Byte     keyword_flags[T42_KEYWORD_COUNT];

    {
      FT_UInt  n;


      for ( n = 0; n < T42_KEYWORD_COUNT; n++ )
        keyword_flags[n] = 0;
    }

    parser->root.cursor = base;
    parser->root.limit  = base + size;
    parser->root.error  = 0;

    for ( ; cur < limit; cur++ )
    {
      /* look for `FontDirectory', which causes problems on some fonts */
      if ( *cur == 'F' && cur + 25 < limit                    &&
           ft_strncmp( (char*)cur, "FontDirectory", 13 ) == 0 )
      {
        FT_Byte*  cur2;


        /* skip the `FontDirectory' keyword */
        cur += 13;
        cur2 = cur;

        /* lookup the `known' keyword */
        while ( cur < limit && *cur != 'k'           &&
                ft_strncmp( (char*)cur, "known", 5 ) )
          cur++;

        if ( cur < limit )
        {
          T1_TokenRec  token;


          /* skip the `known' keyword and the token following it */
          cur += 5;
          loader->parser.root.cursor = cur;
          T1_ToToken( &loader->parser, &token );

          /* if the last token was an array, skip it! */
          if ( token.type == T1_TOKEN_TYPE_ARRAY )
            cur2 = parser->root.cursor;
        }
        cur = cur2;
      }
      /* look for immediates */
      else if ( *cur == '/' && cur + 2 < limit )
      {
        FT_Byte*  cur2;
        FT_UInt    i, len;


        cur++;
        cur2 = cur;
        while ( cur2 < limit && t42_is_alpha( *cur2 ) )
          cur2++;

        len  = (FT_UInt)( cur2 - cur );
        if ( len > 0 && len < 22 ) /* XXX What shall it this 22? */
        {
          /* now, compare the immediate name to the keyword table */

          /* Loop through all known keywords */
          for ( i = 0; i < n_keywords; i++ )
          {
            T1_Field  keyword = (T1_Field)&t42_keywords[i];
            FT_Byte   *name   = (FT_Byte*)keyword->ident;


            if ( !name )
              continue;

            if ( ( len == ft_strlen( (const char *)name ) ) &&
                 ( ft_memcmp( cur, name, len ) == 0 )       )
            {
              /* we found it -- run the parsing callback! */
              parser->root.cursor = cur2;
              T1_Skip_Spaces( parser );

              /* only record the first instance of each field/keyword */
              /* to deal with synthetic fonts correctly               */
              if ( keyword_flags[i] == 0 )
              {
                parser->root.error = t42_load_keyword(face,
                                                      loader,
                                                      keyword );
                if ( parser->root.error )
                  return parser->root.error;
              }
              keyword_flags[i] = 1;

              cur = parser->root.cursor;
              break;
            }
          }
        }
      }
    }
    return parser->root.error;
  }


  FT_LOCAL_DEF( void )
  t42_loader_init( T42_Loader  loader,
                   T42_Face    face )
  {
    FT_UNUSED( face );

    FT_MEM_ZERO( loader, sizeof ( *loader ) );
    loader->num_glyphs = 0;
    loader->num_chars  = 0;

    /* initialize the tables -- simply set their `init' field to 0 */
    loader->encoding_table.init = 0;
    loader->charstrings.init    = 0;
    loader->glyph_names.init    = 0;
  }


  FT_LOCAL_DEF( void )
  t42_loader_done( T42_Loader  loader )
  {
    T42_Parser  parser = &loader->parser;


    /* finalize tables */
    T1_Release_Table( &loader->encoding_table );
    T1_Release_Table( &loader->charstrings );
    T1_Release_Table( &loader->glyph_names );

    /* finalize parser */
    t42_parser_done( parser );
  }


/* END */
