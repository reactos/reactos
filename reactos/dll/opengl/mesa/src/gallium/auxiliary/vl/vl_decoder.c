/**************************************************************************
 *
 * Copyright 2009 Younes Manton.
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

#include "pipe/p_video_decoder.h"

#include "util/u_video.h"

#include "vl_decoder.h"
#include "vl_mpeg12_decoder.h"

bool
vl_profile_supported(struct pipe_screen *screen, enum pipe_video_profile profile)
{
   assert(screen);
   switch (u_reduce_video_profile(profile)) {
      case PIPE_VIDEO_CODEC_MPEG12:
         return true;
      default:
         return false;
   }
}

struct pipe_video_decoder *
vl_create_decoder(struct pipe_context *pipe,
                  enum pipe_video_profile profile,
                  enum pipe_video_entrypoint entrypoint,
                  enum pipe_video_chroma_format chroma_format,
                  unsigned width, unsigned height, unsigned max_references,
                  bool expect_chunked_decode)
{
   unsigned buffer_width, buffer_height;
   bool pot_buffers;

   assert(pipe);
   assert(width > 0 && height > 0);
   
   pot_buffers = !pipe->screen->get_video_param
   (
      pipe->screen,
      profile,
      PIPE_VIDEO_CAP_NPOT_TEXTURES
   );

   buffer_width = pot_buffers ? util_next_power_of_two(width) : align(width, MACROBLOCK_WIDTH);
   buffer_height = pot_buffers ? util_next_power_of_two(height) : align(height, MACROBLOCK_HEIGHT);

   switch (u_reduce_video_profile(profile)) {
      case PIPE_VIDEO_CODEC_MPEG12:
         return vl_create_mpeg12_decoder(pipe, profile, entrypoint, chroma_format,
                                         buffer_width, buffer_height, max_references,
                                         expect_chunked_decode);
      default:
         return NULL;
   }
   return NULL;
}
