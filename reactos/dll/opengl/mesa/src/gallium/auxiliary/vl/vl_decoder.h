/**************************************************************************
 *
 * Copyright 2009 Younes Manton.
 * Copyright 2011 Christian König.
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL TUNGSTEN GRAPHICS AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/

#ifndef vl_decoder_h
#define vl_decoder_h

#include "pipe/p_video_decoder.h"

/**
 * check if a given profile is supported with shader based decoding
 */
bool
vl_profile_supported(struct pipe_screen *screen, enum pipe_video_profile profile);

/**
 * standard implementation of pipe->create_video_decoder
 */
struct pipe_video_decoder *
vl_create_decoder(struct pipe_context *pipe,
                  enum pipe_video_profile profile,
                  enum pipe_video_entrypoint entrypoint,
                  enum pipe_video_chroma_format chroma_format,
                  unsigned width, unsigned height, unsigned max_references,
                  bool expect_chunked_decode);

#endif /* vl_decoder_h */
