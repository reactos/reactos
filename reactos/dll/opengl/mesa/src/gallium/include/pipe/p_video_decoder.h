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

#ifndef PIPE_VIDEO_CONTEXT_H
#define PIPE_VIDEO_CONTEXT_H

#ifdef __cplusplus
extern "C" {
#endif

#include "pipe/p_video_state.h"

struct pipe_screen;
struct pipe_surface;
struct pipe_macroblock;
struct pipe_picture_desc;
struct pipe_fence_handle;

/**
 * Gallium video decoder for a specific codec/profile
 */
struct pipe_video_decoder
{
   struct pipe_context *context;

   enum pipe_video_profile profile;
   enum pipe_video_entrypoint entrypoint;
   enum pipe_video_chroma_format chroma_format;
   unsigned width;
   unsigned height;
   unsigned max_references;

   /**
    * destroy this video decoder
    */
   void (*destroy)(struct pipe_video_decoder *decoder);

   /**
    * set the picture parameters for the next frame
    * only used for bitstream decoding
    */
   void (*set_picture_parameters)(struct pipe_video_decoder *decoder,
                                  struct pipe_picture_desc *picture);

   /**
    * set the quantification matrixes
    */
   void (*set_quant_matrix)(struct pipe_video_decoder *decoder,
                            const struct pipe_quant_matrix *matrix);

   /**
    * set target where video data is decoded to
    */
   void (*set_decode_target)(struct pipe_video_decoder *decoder,
                             struct pipe_video_buffer *target);

   /**
    * set reference frames for motion compensation
    */
   void (*set_reference_frames)(struct pipe_video_decoder *decoder,
                                struct pipe_video_buffer **ref_frames,
                                unsigned num_ref_frames);

   /**
    * start decoding of a new frame
    */
   void (*begin_frame)(struct pipe_video_decoder *decoder);

   /**
    * decode a macroblock
    */
   void (*decode_macroblock)(struct pipe_video_decoder *decoder,
                             const struct pipe_macroblock *macroblocks,
                             unsigned num_macroblocks);

   /**
    * decode a bitstream
    */
   void (*decode_bitstream)(struct pipe_video_decoder *decoder,
                            unsigned num_buffers,
                            const void * const *buffers,
                            const unsigned *sizes);

   /**
    * end decoding of the current frame
    */
   void (*end_frame)(struct pipe_video_decoder *decoder);

   /**
    * flush any outstanding command buffers to the hardware
    * should be called before a video_buffer is acessed by the state tracker again
    */
   void (*flush)(struct pipe_video_decoder *decoder);
};

/**
 * output for decoding / input for displaying
 */
struct pipe_video_buffer
{
   struct pipe_context *context;

   enum pipe_format buffer_format;
   enum pipe_video_chroma_format chroma_format;
   unsigned width;
   unsigned height;

   /**
    * destroy this video buffer
    */
   void (*destroy)(struct pipe_video_buffer *buffer);

   /**
    * get a individual sampler view for each plane
    */
   struct pipe_sampler_view **(*get_sampler_view_planes)(struct pipe_video_buffer *buffer);

   /**
    * get a individual sampler view for each component
    */
   struct pipe_sampler_view **(*get_sampler_view_components)(struct pipe_video_buffer *buffer);

   /**
    * get a individual surfaces for each plane
    */
   struct pipe_surface **(*get_surfaces)(struct pipe_video_buffer *buffer);

   /*
    * auxiliary associated data
    */
   void *associated_data;

   /*
    * decoder where the associated data came from
    */
   struct pipe_video_decoder *decoder;

   /*
    * destroy the associated data
    */
   void (*destroy_associated_data)(void *associated_data);
};

#ifdef __cplusplus
}
#endif

#endif /* PIPE_VIDEO_CONTEXT_H */
