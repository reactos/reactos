/**************************************************************************
 * 
 * Copyright 2007 Tungsten Graphics, Inc., Cedar Park, Texas.
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

#ifndef SP_TEXTURE_H
#define SP_TEXTURE_H


#include "pipe/p_state.h"
#include "sp_limits.h"


struct pipe_context;
struct pipe_screen;
struct softpipe_context;


/**
 * Subclass of pipe_resource.
 */
struct softpipe_resource
{
   struct pipe_resource base;

   unsigned long level_offset[SP_MAX_TEXTURE_2D_LEVELS];
   unsigned stride[SP_MAX_TEXTURE_2D_LEVELS];

   /**
    * Display target, only valid for PIPE_TEXTURE_2D with the
    * PIPE_BIND_DISPLAY_TARGET usage.
    */
   struct sw_displaytarget *dt;

   /**
    * Malloc'ed data for regular buffers and textures, or a mapping to dt above.
    */
   void *data;

   /* True if texture images are power-of-two in all dimensions:
    */
   boolean pot;
   boolean userBuffer;

   unsigned timestamp;
};


/**
 * Subclass of pipe_transfer.
 */
struct softpipe_transfer
{
   struct pipe_transfer base;

   unsigned long offset;
};


/** cast wrappers */
static INLINE struct softpipe_resource *
softpipe_resource(struct pipe_resource *pt)
{
   return (struct softpipe_resource *) pt;
}

static INLINE struct softpipe_transfer *
softpipe_transfer(struct pipe_transfer *pt)
{
   return (struct softpipe_transfer *) pt;
}


extern void
softpipe_init_screen_texture_funcs(struct pipe_screen *screen);

extern void
softpipe_init_texture_funcs(struct pipe_context *pipe);


#endif /* SP_TEXTURE */
