/***************************************************************************/
/*                                                                         */
/*  ftlzw.c                                                                */
/*                                                                         */
/*    FreeType support for .Z compressed files.                            */
/*                                                                         */
/*  This optional component relies on NetBSD's zopen().  It should mainly  */
/*  be used to parse compressed PCF fonts, as found with many X11 server   */
/*  distributions.                                                         */
/*                                                                         */
/*  Copyright 2004 by                                                      */
/*  Albert Chin-A-Young.                                                   */
/*                                                                         */
/*  Based on code in src/gzip/ftgzip.c, Copyright 2004 by                  */
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
#include FT_INTERNAL_MEMORY_H
#include FT_INTERNAL_STREAM_H
#include FT_INTERNAL_DEBUG_H
#include <string.h>
#include <stdio.h>


#include FT_MODULE_ERRORS_H

#undef __FTERRORS_H__

#define FT_ERR_PREFIX  LZW_Err_
#define FT_ERR_BASE    FT_Mod_Err_LZW

#include FT_ERRORS_H


#ifdef FT_CONFIG_OPTION_USE_LZW

#include "zopen.h"


/***************************************************************************/
/***************************************************************************/
/*****                                                                 *****/
/*****                  M E M O R Y   M A N A G E M E N T              *****/
/*****                                                                 *****/
/***************************************************************************/
/***************************************************************************/

/***************************************************************************/
/***************************************************************************/
/*****                                                                 *****/
/*****                   F I L E   D E S C R I P T O R                 *****/
/*****                                                                 *****/
/***************************************************************************/
/***************************************************************************/

#define  FT_LZW_BUFFER_SIZE  4096

  typedef struct FT_LZWFileRec_
  {
    FT_Stream   source;         /* parent/source stream        */
    FT_Stream   stream;         /* embedding stream            */
    FT_Memory   memory;         /* memory allocator            */
    s_zstate_t  zstream;        /* lzw input stream            */

    FT_ULong    start;          /* starting position, after .Z header */
    FT_Byte     input[FT_LZW_BUFFER_SIZE];  /* input buffer */

    FT_Byte     buffer[FT_LZW_BUFFER_SIZE]; /* output buffer */
    FT_ULong    pos;            /* position in output          */
    FT_Byte*    cursor;
    FT_Byte*    limit;

  } FT_LZWFileRec, *FT_LZWFile;


  /* check and skip .Z header */
  static FT_Error
  ft_lzw_check_header( FT_Stream  stream )
  {
    FT_Error  error;
    FT_Byte   head[2];


    if ( FT_STREAM_SEEK( 0 )       ||
         FT_STREAM_READ( head, 2 ) )
      goto Exit;

    /* head[0] && head[1] are the magic numbers     */
    if ( head[0] != 0x1f ||
         head[1] != 0x9d )
      error = LZW_Err_Invalid_File_Format;

  Exit:
    return error;
  }


  static FT_Error
  ft_lzw_file_init( FT_LZWFile  zip,
                    FT_Stream   stream,
                    FT_Stream   source )
  {
    s_zstate_t*  zstream = &zip->zstream;
    FT_Error     error   = LZW_Err_Ok;


    zip->stream = stream;
    zip->source = source;
    zip->memory = stream->memory;

    zip->limit  = zip->buffer + FT_LZW_BUFFER_SIZE;
    zip->cursor = zip->limit;
    zip->pos    = 0;

    /* check and skip .Z header */
    {
      stream = source;

      error = ft_lzw_check_header( source );
      if ( error )
        goto Exit;

      zip->start = FT_STREAM_POS();
    }

    /* initialize internal lzw variable */
    zinit( zstream );

    zstream->avail_in    = 0;
    zstream->next_in     = zip->buffer;
    zstream->zs_in_count = source->size - 2;

    if ( zstream->next_in == NULL )
      error = LZW_Err_Invalid_File_Format;

  Exit:
    return error;
  }


  static void
  ft_lzw_file_done( FT_LZWFile  zip )
  {
    s_zstate_t*  zstream = &zip->zstream;


    /* clear the rest */
    zstream->next_in   = NULL;
    zstream->next_out  = NULL;
    zstream->avail_in  = 0;
    zstream->avail_out = 0;
    zstream->total_in  = 0;
    zstream->total_out = 0;

    zip->memory = NULL;
    zip->source = NULL;
    zip->stream = NULL;
  }


  static FT_Error
  ft_lzw_file_reset( FT_LZWFile  zip )
  {
    FT_Stream  stream = zip->source;
    FT_Error   error;


    if ( !FT_STREAM_SEEK( zip->start ) )
    {
      s_zstate_t*  zstream = &zip->zstream;


      zinit( zstream );

      zstream->avail_in    = 0;
      zstream->next_in     = zip->input;
      zstream->total_in    = 0;
      zstream->avail_out   = 0;
      zstream->next_out    = zip->buffer;
      zstream->total_out   = 0;
      zstream->zs_in_count = zip->source->size - 2;

      zip->limit  = zip->buffer + FT_LZW_BUFFER_SIZE;
      zip->cursor = zip->limit;
      zip->pos    = 0;
    }

    return error;
  }


  static FT_Error
  ft_lzw_file_fill_input( FT_LZWFile  zip )
  {
    s_zstate_t*  zstream = &zip->zstream;
    FT_Stream    stream  = zip->source;
    FT_ULong     size;


    if ( stream->read )
    {
      size = stream->read( stream, stream->pos, zip->input,
                           FT_LZW_BUFFER_SIZE );
      if ( size == 0 )
        return LZW_Err_Invalid_Stream_Operation;
    }
    else
    {
      size = stream->size - stream->pos;
      if ( size > FT_LZW_BUFFER_SIZE )
        size = FT_LZW_BUFFER_SIZE;

      if ( size == 0 )
        return LZW_Err_Invalid_Stream_Operation;

      FT_MEM_COPY( zip->input, stream->base + stream->pos, size );
    }
    stream->pos += size;

    zstream->next_in  = zip->input;
    zstream->avail_in = size;

    return LZW_Err_Ok;
  }



  static FT_Error
  ft_lzw_file_fill_output( FT_LZWFile  zip )
  {
    s_zstate_t*  zstream = &zip->zstream;
    FT_Error     error   = 0;


    zip->cursor        = zip->buffer;
    zstream->next_out  = zip->cursor;
    zstream->avail_out = FT_LZW_BUFFER_SIZE;

    while ( zstream->avail_out > 0 )
    {
      int  num_read = 0;


      if ( zstream->avail_in == 0 )
      {
        error = ft_lzw_file_fill_input( zip );
        if ( error )
          break;
      }

      num_read = zread( zstream );

      if ( num_read == -1 && zstream->zs_in_count == 0 )
      {
        zip->limit = zstream->next_out;
        if ( zip->limit == zip->cursor )
          error = LZW_Err_Invalid_Stream_Operation;
        break;
      }
      else if ( num_read == -1 )
        break;
      else
        zstream->avail_out -= num_read;
    }

    return error;
  }


  /* fill output buffer; `count' must be <= FT_LZW_BUFFER_SIZE */
  static FT_Error
  ft_lzw_file_skip_output( FT_LZWFile  zip,
                           FT_ULong    count )
  {
    FT_Error  error = LZW_Err_Ok;
    FT_ULong  delta;


    for (;;)
    {
      delta = (FT_ULong)( zip->limit - zip->cursor );
      if ( delta >= count )
        delta = count;

      zip->cursor += delta;
      zip->pos    += delta;

      count -= delta;
      if ( count == 0 )
        break;

      error = ft_lzw_file_fill_output( zip );
      if ( error )
        break;
    }

    return error;
  }


  static FT_ULong
  ft_lzw_file_io( FT_LZWFile  zip,
                  FT_ULong    pos,
                  FT_Byte*    buffer,
                  FT_ULong    count )
  {
    FT_ULong  result = 0;
    FT_Error  error;


    /* Teset inflate stream if we're seeking backwards.        */
    /* Yes, that is not too efficient, but it saves memory :-) */
    if ( pos < zip->pos )
    {
      error = ft_lzw_file_reset( zip );
      if ( error )
        goto Exit;
    }

    /* skip unwanted bytes */
    if ( pos > zip->pos )
    {
      error = ft_lzw_file_skip_output( zip, (FT_ULong)( pos - zip->pos ) );
      if ( error )
        goto Exit;
    }

    if ( count == 0 )
      goto Exit;

    /* now read the data */
    for (;;)
    {
      FT_ULong  delta;


      delta = (FT_ULong)( zip->limit - zip->cursor );
      if ( delta >= count )
        delta = count;

      FT_MEM_COPY( buffer, zip->cursor, delta );
      buffer      += delta;
      result      += delta;
      zip->cursor += delta;
      zip->pos    += delta;

      count -= delta;
      if ( count == 0 )
        break;

      error = ft_lzw_file_fill_output( zip );
      if ( error )
        break;
    }

  Exit:
    return result;
  }


/***************************************************************************/
/***************************************************************************/
/*****                                                                 *****/
/*****            L Z W   E M B E D D I N G   S T R E A M              *****/
/*****                                                                 *****/
/***************************************************************************/
/***************************************************************************/

  static void
  ft_lzw_stream_close( FT_Stream  stream )
  {
    FT_LZWFile  zip    = (FT_LZWFile)stream->descriptor.pointer;
    FT_Memory   memory = stream->memory;


    if ( zip )
    {
      /* finalize lzw file descriptor */
      ft_lzw_file_done( zip );

      FT_FREE( zip );

      stream->descriptor.pointer = NULL;
    }
  }


  static FT_ULong
  ft_lzw_stream_io( FT_Stream  stream,
                    FT_ULong   pos,
                    FT_Byte*   buffer,
                    FT_ULong   count )
  {
    FT_LZWFile  zip = (FT_LZWFile)stream->descriptor.pointer;


    return ft_lzw_file_io( zip, pos, buffer, count );
  }


  FT_EXPORT_DEF( FT_Error )
  FT_Stream_OpenLZW( FT_Stream  stream,
                     FT_Stream  source )
  {
    FT_Error    error;
    FT_Memory   memory = source->memory;
    FT_LZWFile  zip;


    FT_ZERO( stream );
    stream->memory = memory;

    if ( !FT_NEW( zip ) )
    {
      error = ft_lzw_file_init( zip, stream, source );
      if ( error )
      {
        FT_FREE( zip );
        goto Exit;
      }

      stream->descriptor.pointer = zip;
    }

    stream->size  = 0x7FFFFFFFL;  /* don't know the real size! */
    stream->pos   = 0;
    stream->base  = 0;
    stream->read  = ft_lzw_stream_io;
    stream->close = ft_lzw_stream_close;

  Exit:
    return error;
  }

#include "zopen.c"


#else  /* !FT_CONFIG_OPTION_USE_LZW */


  FT_EXPORT_DEF( FT_Error )
  FT_Stream_OpenLZW( FT_Stream  stream,
                     FT_Stream  source )
  {
    FT_UNUSED( stream );
    FT_UNUSED( source );

    return LZW_Err_Unimplemented_Feature;
  }


#endif /* !FT_CONFIG_OPTION_USE_LZW */


/* END */
