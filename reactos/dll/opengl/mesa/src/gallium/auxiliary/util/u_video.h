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

#ifndef U_VIDEO_H
#define U_VIDEO_H

#ifdef __cplusplus
extern "C" {
#endif

#include "pipe/p_defines.h"
#include "pipe/p_video_enums.h"

/* u_reduce_video_profile() needs these */
#include "pipe/p_compiler.h"
#include "util/u_debug.h"

static INLINE enum pipe_video_codec
u_reduce_video_profile(enum pipe_video_profile profile)
{
   switch (profile)
   {
      case PIPE_VIDEO_PROFILE_MPEG1:
      case PIPE_VIDEO_PROFILE_MPEG2_SIMPLE:
      case PIPE_VIDEO_PROFILE_MPEG2_MAIN:
         return PIPE_VIDEO_CODEC_MPEG12;

      case PIPE_VIDEO_PROFILE_MPEG4_SIMPLE:
      case PIPE_VIDEO_PROFILE_MPEG4_ADVANCED_SIMPLE:
         return PIPE_VIDEO_CODEC_MPEG4;

      case PIPE_VIDEO_PROFILE_VC1_SIMPLE:
      case PIPE_VIDEO_PROFILE_VC1_MAIN:
      case PIPE_VIDEO_PROFILE_VC1_ADVANCED:
         return PIPE_VIDEO_CODEC_VC1;

      case PIPE_VIDEO_PROFILE_MPEG4_AVC_BASELINE:
      case PIPE_VIDEO_PROFILE_MPEG4_AVC_MAIN:
      case PIPE_VIDEO_PROFILE_MPEG4_AVC_HIGH:
         return PIPE_VIDEO_CODEC_MPEG4_AVC;

      default:
         assert(0);
         return PIPE_VIDEO_CODEC_UNKNOWN;
   }
}

#ifdef __cplusplus
}
#endif

#endif /* U_VIDEO_H */
