/***************************************************************************/
/*                                                                         */
/*  cidload.c                                                              */
/*                                                                         */
/*    CID-keyed Type1 font loader (body).                                  */
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
#include FT_INTERNAL_DEBUG_H
#include FT_CONFIG_CONFIG_H
#include FT_MULTIPLE_MASTERS_H
#include FT_INTERNAL_TYPE1_TYPES_H

#include "cidload.h"

#include "ciderrs.h"


  /*************************************************************************/
  /*                                                                       */
  /* The macro FT_COMPONENT is used in trace mode.  It is an implicit      */
  /* parameter of the FT_TRACE() and FT_ERROR() macros, used to print/log  */
  /* messages during execution.                                            */
  /*                                                                       */
#undef  FT_COMPONENT
#define FT_COMPONENT  trace_cidload


  /* read a single offset */
  FT_LOCAL_DEF( FT_Long )
  cid_get_offset( FT_Byte**  start,
                  FT_Byte    offsize )
  {
    FT_Long   result;
    FT_Byte*  p = *start;


    for ( result = 0; offsize > 0; offsize-- )
    {
      result <<= 8;
      result  |= *p++;
    }

    *start = p;
    return result;
  }


  FT_LOCAL_DEF( void )
  cid_decrypt( FT_Byte*   buffer,
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


  /*************************************************************************/
  /*************************************************************************/
  /*****                                                               *****/
  /*****                    TYPE 1 SYMBOL PARSING                      *****/
  /*****                                                               *****/
  /*************************************************************************/
  /*************************************************************************/


  static FT_Error
  cid_load_keyword( CID_Face        face,
                    CID_Loader*     loader,
                    const T1_Field  keyword )
  {
    FT_Error      error;
    CID_Parser*   parser = &loader->parser;
    FT_Byte*      object;
    void*         dummy_object;
    CID_FaceInfo  cid = &face->cid;


    /* if the keyword has a dedicated callback, call it */
    if ( keyword->type == T1_FIELD_TYPE_CALLBACK )
    {
      keyword->reader( (FT_Face)face, parser );
      error = parser->root.error;
      goto Exit;
    }

    /* we must now compute the address of our target object */
    switch ( keyword->location )
    {
    case T1_FIELD_LOCATION_CID_INFO:
      object = (FT_Byte*)cid;
      break;

    case T1_FIELD_LOCATION_FONT_INFO:
      object = (FT_Byte*)&cid->font_info;
      break;

    default:
      {
        CID_FaceDict  dict;


        if ( parser->num_dict < 0 )
        {
          FT_ERROR(( "cid_load_keyword: invalid use of `%s'!\n",
                     keyword->ident ));
          error = CID_Err_Syntax_Error;
          goto Exit;
        }

        dict = cid->font_dicts + parser->num_dict;
        switch ( keyword->location )
        {
        case T1_FIELD_LOCATION_PRIVATE:
          object = (FT_Byte*)&dict->private_dict;
          break;

        default:
          object = (FT_Byte*)dict;
        }
      }
    }

    dummy_object = object;

    /* now, load the keyword data in the object's field(s) */
    if ( keyword->type == T1_FIELD_TYPE_INTEGER_ARRAY ||
         keyword->type == T1_FIELD_TYPE_FIXED_ARRAY   )
      error = cid_parser_load_field_table( &loader->parser, keyword,
                                    &dummy_object );
    else
      error = cid_parser_load_field( &loader->parser, keyword, &dummy_object );
  Exit:
    return error;
  }


  FT_CALLBACK_DEF( FT_Error )
  parse_font_bbox( CID_Face     face,
                   CID_Parser*  parser )
  {
    FT_Fixed  temp[4];
    FT_BBox*  bbox = &face->cid.font_bbox;


    (void)cid_parser_to_fixed_array( parser, 4, temp, 0 );
    bbox->xMin = FT_RoundFix( temp[0] );
    bbox->yMin = FT_RoundFix( temp[1] );
    bbox->xMax = FT_RoundFix( temp[2] );
    bbox->yMax = FT_RoundFix( temp[3] );

    return CID_Err_Ok;       /* this is a callback function; */
                            /* we must return an error code */
  }


  FT_CALLBACK_DEF( FT_Error )
  parse_font_matrix( CID_Face     face,
                     CID_Parser*  parser )
  {
    FT_Matrix*    matrix;
    FT_Vector*    offset;
    CID_FaceDict  dict;
    FT_Face       root = (FT_Face)&face->root;
    FT_Fixed      temp[6];
    FT_Fixed      temp_scale;


    if ( parser->num_dict >= 0 )
    {
      dict   = face->cid.font_dicts + parser->num_dict;
      matrix = &dict->font_matrix;
      offset = &dict->font_offset;

      (void)cid_parser_to_fixed_array( parser, 6, temp, 3 );

      temp_scale = ABS( temp[3] );

      /* Set Units per EM based on FontMatrix values.  We set the value to */
      /* `1000/temp_scale', because temp_scale was already multiplied by   */
      /* 1000 (in t1_tofixed(), from psobjs.c).                            */
      root->units_per_EM = (FT_UShort)( FT_DivFix( 0x10000L,
                                        FT_DivFix( temp_scale, 1000 ) ) );

      /* we need to scale the values by 1.0/temp[3] */
      if ( temp_scale != 0x10000L )
      {
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

      /* note that the font offsets are expressed in integer font units */
      offset->x  = temp[4] >> 16;
      offset->y  = temp[5] >> 16;
    }

    return CID_Err_Ok;       /* this is a callback function; */
                            /* we must return an error code */
  }


  FT_CALLBACK_DEF( FT_Error )
  parse_fd_array( CID_Face     face,
                  CID_Parser*  parser )
  {
    CID_FaceInfo  cid    = &face->cid;
    FT_Memory     memory = face->root.memory;
    FT_Error      error  = CID_Err_Ok;
    FT_Long       num_dicts;


    num_dicts = cid_parser_to_int( parser );

    if ( !cid->font_dicts )
    {
      FT_Int  n;


      if ( FT_NEW_ARRAY( cid->font_dicts, num_dicts ) )
        goto Exit;

      cid->num_dicts = (FT_UInt)num_dicts;

      /* don't forget to set a few defaults */
      for ( n = 0; n < cid->num_dicts; n++ )
      {
        CID_FaceDict  dict = cid->font_dicts + n;


        /* default value for lenIV */
        dict->private_dict.lenIV = 4;
      }
    }

  Exit:
    return error;
  }


  static
  const T1_FieldRec  cid_field_records[] =
  {

#include "cidtoken.h"

    T1_FIELD_CALLBACK( "FontBBox", parse_font_bbox )
    T1_FIELD_CALLBACK( "FDArray", parse_fd_array )
    T1_FIELD_CALLBACK( "FontMatrix", parse_font_matrix )
    { 0, T1_FIELD_LOCATION_CID_INFO, T1_FIELD_TYPE_NONE, 0, 0, 0, 0, 0 }
  };


  static int
  is_alpha( char  c )
  {
    return ( ft_isalnum( (int)c ) ||
             c == '.'             ||
             c == '_'             );
  }


  static FT_Error
  cid_parse_dict( CID_Face     face,
                  CID_Loader*  loader,
                  FT_Byte*     base,
                  FT_Long      size )
  {
    CID_Parser*  parser = &loader->parser;


    parser->root.cursor = base;
    parser->root.limit  = base + size;
    parser->root.error  = 0;

    {
      FT_Byte*  cur   = base;
      FT_Byte*  limit = cur + size;


      for ( ;cur < limit; cur++ )
      {
        /* look for `%ADOBeginFontDict' */
        if ( *cur == '%' && cur + 20 < limit &&
             ft_strncmp( (char*)cur, "%ADOBeginFontDict", 17 ) == 0 )
        {
          cur += 17;

          /* if /FDArray was found, then cid->num_dicts is > 0, and */
          /* we can start increasing parser->num_dict               */
          if ( face->cid.num_dicts > 0 )
            parser->num_dict++;
        }
        /* look for immediates */
        else if ( *cur == '/' && cur + 2 < limit )
        {
          FT_Byte*  cur2;
          FT_Int    len;


          cur++;

          cur2 = cur;
          while ( cur2 < limit && is_alpha( *cur2 ) )
            cur2++;

          len = (FT_Int)( cur2 - cur );
          if ( len > 0 && len < 22 )
          {
            /* now compare the immediate name to the keyword table */
            T1_Field  keyword = (T1_Field) cid_field_records;


            for (;;)
            {
              FT_Byte*  name;


              name = (FT_Byte*)keyword->ident;
              if ( !name )
                break;

              if ( cur[0] == name[0]                          &&
                   len == (FT_Int)ft_strlen( (const char*)name ) )
              {
                FT_Int  n;


                for ( n = 1; n < len; n++ )
                  if ( cur[n] != name[n] )
                    break;

                if ( n >= len )
                {
                  /* we found it - run the parsing callback */
                  parser->root.cursor = cur2;
                  cid_parser_skip_spaces( parser );
                  parser->root.error = cid_load_keyword( face,
                                                         loader,
                                                         keyword );
                  if ( parser->root.error )
                    return parser->root.error;

                  cur = parser->root.cursor;
                  break;
                }
              }
              keyword++;
            }
          }
        }
      }
    }
    return parser->root.error;
  }


  /* read the subrmap and the subrs of each font dict */
  static FT_Error
  cid_read_subrs( CID_Face  face )
  {
    CID_FaceInfo  cid    = &face->cid;
    FT_Memory     memory = face->root.memory;
    FT_Stream     stream = face->root.stream;
    FT_Error      error;
    FT_Int        n;
    CID_Subrs     subr;
    FT_UInt       max_offsets = 0;
    FT_ULong*     offsets = 0;


    if ( FT_NEW_ARRAY( face->subrs, cid->num_dicts ) )
      goto Exit;

    subr = face->subrs;
    for ( n = 0; n < cid->num_dicts; n++, subr++ )
    {
      CID_FaceDict  dict  = cid->font_dicts + n;
      FT_Int        lenIV = dict->private_dict.lenIV;
      FT_UInt       count, num_subrs = dict->num_subrs;
      FT_ULong      data_len;
      FT_Byte*      p;


      /* reallocate offsets array if needed */
      if ( num_subrs + 1 > max_offsets )
      {
        FT_UInt  new_max = ( num_subrs + 1 + 3 ) & -4;


        if ( FT_RENEW_ARRAY( offsets, max_offsets, new_max ) )
          goto Fail;

        max_offsets = new_max;
      }

      /* read the subrmap's offsets */
      if ( FT_STREAM_SEEK( cid->data_offset + dict->subrmap_offset ) ||
           FT_FRAME_ENTER( ( num_subrs + 1 ) * dict->sd_bytes )   )
        goto Fail;

      p = (FT_Byte*)stream->cursor;
      for ( count = 0; count <= num_subrs; count++ )
        offsets[count] = cid_get_offset( &p, (FT_Byte)dict->sd_bytes );

      FT_FRAME_EXIT();

      /* now, compute the size of subrs charstrings, */
      /* allocate, and read them                     */
      data_len = offsets[num_subrs] - offsets[0];

      if ( FT_NEW_ARRAY( subr->code, num_subrs + 1 ) ||
               FT_ALLOC( subr->code[0], data_len )   )
        goto Fail;

      if ( FT_STREAM_SEEK( cid->data_offset + offsets[0] ) ||
           FT_STREAM_READ( subr->code[0], data_len )  )
        goto Fail;

      /* set up pointers */
      for ( count = 1; count <= num_subrs; count++ )
      {
        FT_ULong  len;


        len               = offsets[count] - offsets[count - 1];
        subr->code[count] = subr->code[count - 1] + len;
      }

      /* decrypt subroutines, but only if lenIV >= 0 */
      if ( lenIV >= 0 )
      {
        for ( count = 0; count < num_subrs; count++ )
        {
          FT_ULong  len;


          len = offsets[count + 1] - offsets[count];
          cid_decrypt( subr->code[count], len, 4330 );
        }
      }

      subr->num_subrs = num_subrs;
    }

  Exit:
    FT_FREE( offsets );
    return error;

  Fail:
    if ( face->subrs )
    {
      for ( n = 0; n < cid->num_dicts; n++ )
      {
        if ( face->subrs[n].code )
          FT_FREE( face->subrs[n].code[0] );

        FT_FREE( face->subrs[n].code );
      }
      FT_FREE( face->subrs );
    }
    goto Exit;
  }


  static void
  t1_init_loader( CID_Loader*  loader,
                  CID_Face     face )
  {
    FT_UNUSED( face );

    FT_MEM_ZERO( loader, sizeof ( *loader ) );
  }


  static void
  t1_done_loader( CID_Loader*  loader )
  {
    CID_Parser*  parser = &loader->parser;


    /* finalize parser */
    cid_parser_done( parser );
  }


  FT_LOCAL_DEF( FT_Error )
  cid_face_open( CID_Face  face )
  {
    CID_Loader   loader;
    CID_Parser*  parser;
    FT_Error     error;


    t1_init_loader( &loader, face );

    parser = &loader.parser;
    error = cid_parser_new( parser, face->root.stream, face->root.memory,
                            (PSAux_Service)face->psaux );
    if ( error )
      goto Exit;

    error = cid_parse_dict( face, &loader,
                            parser->postscript,
                            parser->postscript_len );
    if ( error )
      goto Exit;

    face->cid.data_offset = loader.parser.data_offset;
    error = cid_read_subrs( face );

  Exit:
    t1_done_loader( &loader );
    return error;
  }


/* END */
