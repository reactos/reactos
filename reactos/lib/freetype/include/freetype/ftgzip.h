/***************************************************************************/
/*                                                                         */
/*  ftgzip.h                                                               */
/*                                                                         */
/*    Gzip-compressed stream support.                                      */
/*                                                                         */
/*  Copyright 2002 by                                                      */
/*  David Turner, Robert Wilhelm, and Werner Lemberg.                      */
/*                                                                         */
/*  This file is part of the FreeType project, and may only be used,       */
/*  modified, and distributed under the terms of the FreeType project      */
/*  license, LICENSE.TXT.  By continuing to use, modify, or distribute     */
/*  this file you indicate that you have read the license and              */
/*  understand and accept it fully.                                        */
/*                                                                         */
/***************************************************************************/


#ifndef __FTXF86_H__
#define __FTXF86_H__

#include <ft2build.h>
#include FT_FREETYPE_H

FT_BEGIN_HEADER

  /*************************************************************************/
  /*                                                                       */
  /* <Section>                                                             */
  /*    gzip                                                               */
  /*                                                                       */
  /* <Title>                                                               */
  /*    GZIP Streams                                                       */
  /*                                                                       */
  /* <Abstract>                                                            */
  /*    Using gzip-compressed font files                                   */
  /*                                                                       */
  /* <Description>                                                         */
  /*    This section contains the declaration of Gzip-specific functions.  */
  /*                                                                       */
  /*************************************************************************/

 /************************************************************************
  *
  * @type: FT_Stream_OpenGzip
  *
  * @description:
  *   open a new stream to parse gzip-compressed font files. This is
  *   mainly used to support the compressed *.pcf.gz fonts that come
  *   with XFree86
  *
  * @input:
  *   stream :: target embedding stream
  *   source :: source stream, used to
  *
  * @return:
  *   error code. 0 means success
  *
  * @note:
  *   the source stream must be opened _before_ calling this function.
  *
  *   calling @FT_Stream_Close on the new stream will *not* call
  *   @FT_Stream_Close on the source stream. None of the stream objects
  *   will be released to the heap.
  *
  *   the stream implementation is very basic, and resets the decompression
  *   process each time seeking backwards is needed within the stream
  *
  *   in certain builds of the library, gzip compression recognition is
  *   automatic when calling @FT_New_Face or @FT_Open_Face. This means that
  *   if no font driver is capable of handling the raw compressed file,
  *   the library will try to open a gzip stream from it and re-open
  *   the face with it.
  *
  *   this function may return "FT_Err_Unimplemented" if your build of
  *   FreeType was not compiled with zlib support.
  */
  FT_EXPORT( FT_Error )
  FT_Stream_OpenGzip( FT_Stream    stream,
                      FT_Stream    source );

 /* */

FT_END_HEADER

#endif /* __FTXF86_H__ */
