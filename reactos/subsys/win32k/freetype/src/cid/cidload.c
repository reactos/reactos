/***************************************************************************/
/*                                                                         */
/*  cidload.c                                                              */
/*                                                                         */
/*    CID-keyed Type1 font loader (body).                                  */
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
#include <freetype/config/ftconfig.h>
#include <freetype/ftmm.h>

#include <freetype/internal/t1types.h>
#include <freetype/internal/t1errors.h>


#ifdef FT_FLAT_COMPILE

#include "cidload.h"

#else

#include <cid/cidload.h>

#endif


#include <stdio.h>
#include <ctype.h>  /* for isspace(), isalnum() */


  /*************************************************************************/
  /*                                                                       */
  /* The macro FT_COMPONENT is used in trace mode.  It is an implicit      */
  /* parameter of the FT_TRACE() and FT_ERROR() macros, used to print/log  */
  /* messages during execution.                                            */
  /*                                                                       */
#undef  FT_COMPONENT
#define FT_COMPONENT  trace_cidload


  /* read a single offset */
  LOCAL_FUNC
  FT_Long  cid_get_offset( FT_Byte**  start,
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


  LOCAL_FUNC
  void  cid_decrypt( FT_Byte*   buffer,
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


  /*************************************************************************/
  /*************************************************************************/
  /*****                                                               *****/
  /*****                    TYPE 1 SYMBOL PARSING                      *****/
  /*****                                                               *****/
  /*************************************************************************/
  /*************************************************************************/


  static
  FT_Error  cid_load_keyword( CID_Face              face,
                              CID_Loader*           loader,
                              const CID_Field_Rec*  keyword )
  {
    FT_Error     error;
    CID_Parser*  parser = &loader->parser;
    FT_Byte*     object;
    CID_Info*    cid = &face->cid;


    /* if the keyword has a dedicated callback, call it */
    if ( keyword->type == t1_field_callback )
    {
      error = keyword->reader( face, parser );
      goto Exit;
    }

    /* we must now compute the address of our target object */
    switch ( keyword->location )
    {
    case t1_field_cid_info:
      object = (FT_Byte*)cid;
      break;

    case t1_field_font_info:
      object = (FT_Byte*)&cid->font_info;
      break;

    default:
      {
        CID_FontDict*  dict;


        if ( parser->num_dict < 0 )
        {
          FT_ERROR(( "cid_load_keyword: invalid use of `%s'!\n",
                     keyword->ident ));
          error = T1_Err_Syntax_Error;
          goto Exit;
        }

        dict = cid->font_dicts + parser->num_dict;
        switch ( keyword->location )
        {
        case t1_field_private:
          object = (FT_Byte*)&dict->private_dict;
          break;

        default:
          object = (FT_Byte*)dict;
        }
      }
    }

    /* now, load the keyword data in the object's field(s) */
    if ( keyword->type == t1_field_integer_array ||
         keyword->type == t1_field_fixed_array   )
      error = CID_Load_Field_Table( parser, keyword, object );
    else
      error = CID_Load_Field( parser, keyword, object );

  Exit:
    return error;
  }


  static
  FT_Error  parse_font_bbox( CID_Face     face,
                             CID_Parser*  parser )
  {
    FT_Short  temp[4];
    FT_BBox*  bbox = &face->cid.font_bbox;


    (void)CID_ToCoordArray( parser, 4, temp );
    bbox->xMin = temp[0];
    bbox->yMin = temp[1];
    bbox->xMax = temp[2];
    bbox->yMax = temp[3];

    return T1_Err_Ok;       /* this is a callback function; */
                            /* we must return an error code */
  }


  static
  FT_Error  parse_font_matrix( CID_Face     face,
                               CID_Parser*  parser )
  {
    FT_Matrix*     matrix;
    CID_FontDict*  dict;
    FT_Fixed       temp[4];


    if ( parser->num_dict >= 0 )
    {
      dict   = face->cid.font_dicts + parser->num_dict;
      matrix = &dict->font_matrix;

      (void)CID_ToFixedArray( parser, 4, temp, 3 );
      matrix->xx = temp[0];
      matrix->yx = temp[1];
      matrix->xy = temp[2];
      matrix->yy = temp[3];
    }

    return T1_Err_Ok;       /* this is a callback function; */
                            /* we must return an error code */
  }


  static
  FT_Error  parse_fd_array( CID_Face     face,
                            CID_Parser*  parser )
  {
    CID_Info*  cid    = &face->cid;
    FT_Memory  memory = face->root.memory;
    FT_Error   error  = T1_Err_Ok;
    FT_Long    num_dicts;


    num_dicts = CID_ToInt( parser );

    if ( !cid->font_dicts )
    {
      FT_Int  n;


      if ( ALLOC_ARRAY( cid->font_dicts, num_dicts, CID_FontDict ) )
        goto Exit;

      cid->num_dicts = (FT_UInt)num_dicts;

      /* don't forget to set a few defaults */
      for ( n = 0; n < cid->num_dicts; n++ )
      {
        CID_FontDict*  dict = cid->font_dicts + n;


        /* default value for lenIV */
        dict->private_dict.lenIV = 4;
      }
    }

  Exit:
    return error;
  }


  static
  const CID_Field_Rec  t1_field_records[] =
  {

#ifdef FT_FLAT_COMPILE

#include "cidtokens.h"

#else

#include <cid/cidtokens.h>

#endif

    { 0, t1_field_cid_info, t1_field_none, 0, 0, 0, 0, 0 }
  };


  static
  int  is_alpha( char  c )
  {
    return ( isalnum( c ) ||
             c == '.'     ||
             c == '_'     );
  }


  static
  void  skip_whitespace( CID_Parser*  parser )
  {
    FT_Byte*  cur = parser->cursor;


    while ( cur < parser->limit && isspace( *cur ) )
      cur++;

    parser->cursor = cur;
  }


  static
  FT_Error  parse_dict( CID_Face     face,
                        CID_Loader*  loader,
                        FT_Byte*     base,
                        FT_Long      size )
  {
    CID_Parser*  parser = &loader->parser;


    parser->cursor = base;
    parser->limit  = base + size;
    parser->error  = 0;

    {
      FT_Byte*  cur   = base;
      FT_Byte*  limit = cur + size;


      for ( ;cur < limit; cur++ )
      {
        /* look for `%ADOBeginFontDict' */
        if ( *cur == '%' && cur + 20 < limit &&
             strncmp( (char*)cur, "%ADOBeginFontDict", 17 ) == 0 )
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

          len = cur2 - cur;
          if ( len > 0 && len < 22 )
          {
            /* now compare the immediate name to the keyword table */
            const CID_Field_Rec*  keyword = t1_field_records;


            for (;;)
            {
              FT_Byte*  name;


              name = (FT_Byte*)keyword->ident;
              if ( !name )
                break;

              if ( cur[0] == name[0]                          &&
                   len == (FT_Int)strlen( (const char*)name ) )
              {
                FT_Int  n;


                for ( n = 1; n < len; n++ )
                  if ( cur[n] != name[n] )
                    break;

                if ( n >= len )
                {
                  /* we found it - run the parsing callback */
                  parser->cursor = cur2;
                  skip_whitespace( parser );
                  parser->error = cid_load_keyword( face, loader, keyword );
                  if ( parser->error )
                    return parser->error;

                  cur = parser->cursor;
                  break;
                }
              }
              keyword++;
            }
          }
        }
      }
    }
    return parser->error;
  }


  /* read the subrmap and the subrs of each font dict */
  static
  FT_Error  cid_read_subrs( CID_Face  face )
  {
    CID_Info*   cid    = &face->cid;
    FT_Memory   memory = face->root.memory;
    FT_Stream   stream = face->root.stream;
    FT_Error    error;
    FT_Int      n;
    CID_Subrs*  subr;
    FT_UInt     max_offsets = 0;
    FT_ULong*   offsets = 0;


    if ( ALLOC_ARRAY( face->subrs, cid->num_dicts, CID_Subrs ) )
      goto Exit;

    subr = face->subrs;
    for ( n = 0; n < cid->num_dicts; n++, subr++ )
    {
      CID_FontDict*  dict = cid->font_dicts + n;
      FT_UInt        count, num_subrs = dict->num_subrs;
      FT_ULong       data_len;
      FT_Byte*       p;


      /* reallocate offsets array if needed */
      if ( num_subrs + 1 > max_offsets )
      {
        FT_UInt  new_max = ( num_subrs + 1 + 3 ) & -4;


        if ( REALLOC_ARRAY( offsets, max_offsets, new_max, FT_ULong ) )
          goto Fail;

        max_offsets = new_max;
      }

      /* read the subrmap's offsets */
      if ( FILE_Seek( cid->data_offset + dict->subrmap_offset ) ||
           ACCESS_Frame( ( num_subrs + 1 ) * dict->sd_bytes )   )
        goto Fail;

      p = (FT_Byte*)stream->cursor;
      for ( count = 0; count <= num_subrs; count++ )
        offsets[count] = cid_get_offset( &p, (FT_Byte)dict->sd_bytes );

      FORGET_Frame();

      /* now, compute the size of subrs charstrings, */
      /* allocate, and read them                     */
      data_len = offsets[num_subrs] - offsets[0];

      if ( ALLOC_ARRAY( subr->code, num_subrs + 1, FT_Byte* ) ||
           ALLOC( subr->code[0], data_len )                   )
        goto Fail;

      if ( FILE_Seek( cid->data_offset + offsets[0] ) ||
           FILE_Read( subr->code[0], data_len )  )
        goto Exit;

      /* set up pointers */
      for ( count = 1; count <= num_subrs; count++ )
      {
        FT_UInt  len;


        len               = offsets[count] - offsets[count - 1];
        subr->code[count] = subr->code[count - 1] + len;
      }

      /* decrypt subroutines */
      for ( count = 0; count < num_subrs; count++ )
      {
        FT_UInt  len;


        len = offsets[count + 1] - offsets[count];
        cid_decrypt( subr->code[count], len, 4330 );
      }

      subr->num_subrs = num_subrs;
    }

  Exit:
    FREE( offsets );
    return error;

  Fail:
    if ( face->subrs )
    {
      for ( n = 0; n < cid->num_dicts; n++ )
      {
        if ( face->subrs[n].code )
          FREE( face->subrs[n].code[0] );

        FREE( face->subrs[n].code );
      }
      FREE( face->subrs );
    }
    goto Exit;
  }


  static
  void t1_init_loader( CID_Loader*  loader,
                       CID_Face     face )
  {
    FT_UNUSED( face );

    MEM_Set( loader, 0, sizeof ( *loader ) );
  }


  static
  void t1_done_loader( CID_Loader*  loader )
  {
    CID_Parser*  parser = &loader->parser;


    /* finalize parser */
    CID_Done_Parser( parser );
  }


  LOCAL_FUNC
  FT_Error  CID_Open_Face( CID_Face  face )
  {
    CID_Loader  loader;
    CID_Parser* parser;
    FT_Error   error;


    t1_init_loader( &loader, face );

    parser = &loader.parser;
    error = CID_New_Parser( parser, face->root.stream, face->root.memory );
    if ( error )
      goto Exit;

    error = parse_dict( face, &loader,
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
