/**************************************************************************
 *
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

#ifndef vl_mpeg12_bitstream_h
#define vl_mpeg12_bitstream_h

#include "vl_defines.h"
#include "vl_vlc.h"

struct vl_mpg12_bs
{
   struct pipe_video_decoder *decoder;

   struct pipe_mpeg12_picture_desc desc;
   struct dct_coeff *intra_dct_tbl;

   struct vl_vlc vlc;
   short pred_dc[3];
};

void
vl_mpg12_bs_init(struct vl_mpg12_bs *bs, struct pipe_video_decoder *decoder);

void
vl_mpg12_bs_set_picture_desc(struct vl_mpg12_bs *bs, struct pipe_mpeg12_picture_desc *picture);

void
vl_mpg12_bs_decode(struct vl_mpg12_bs *bs, unsigned num_buffers,
                   const void * const *buffers, const unsigned *sizes);

#endif /* vl_mpeg12_bitstream_h */
