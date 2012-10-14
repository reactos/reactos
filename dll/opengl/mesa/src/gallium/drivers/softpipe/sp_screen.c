/**************************************************************************
 *
 * Copyright 2008 Tungsten Graphics, Inc., Cedar Park, Texas.
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


#include "util/u_memory.h"
#include "util/u_format.h"
#include "util/u_format_s3tc.h"
#include "util/u_video.h"
#include "pipe/p_defines.h"
#include "pipe/p_screen.h"
#include "draw/draw_context.h"
#include "vl/vl_decoder.h"
#include "vl/vl_video_buffer.h"

#include "state_tracker/sw_winsys.h"
#include "tgsi/tgsi_exec.h"

#include "sp_texture.h"
#include "sp_screen.h"
#include "sp_context.h"
#include "sp_fence.h"
#include "sp_public.h"

DEBUG_GET_ONCE_BOOL_OPTION(use_llvm, "SOFTPIPE_USE_LLVM", FALSE)

static const char *
softpipe_get_vendor(struct pipe_screen *screen)
{
   return "VMware, Inc.";
}


static const char *
softpipe_get_name(struct pipe_screen *screen)
{
   return "softpipe";
}


static int
softpipe_get_param(struct pipe_screen *screen, enum pipe_cap param)
{
   switch (param) {
   case PIPE_CAP_MAX_COMBINED_SAMPLERS:
      return PIPE_MAX_SAMPLERS + PIPE_MAX_VERTEX_SAMPLERS;
   case PIPE_CAP_NPOT_TEXTURES:
      return 1;
   case PIPE_CAP_TWO_SIDED_STENCIL:
      return 1;
   case PIPE_CAP_SM3:
      return 1;
   case PIPE_CAP_ANISOTROPIC_FILTER:
      return 1;
   case PIPE_CAP_POINT_SPRITE:
      return 1;
   case PIPE_CAP_MAX_RENDER_TARGETS:
      return PIPE_MAX_COLOR_BUFS;
   case PIPE_CAP_OCCLUSION_QUERY:
      return 1;
   case PIPE_CAP_TIMER_QUERY:
      return 1;
   case PIPE_CAP_TEXTURE_MIRROR_CLAMP:
      return 1;
   case PIPE_CAP_TEXTURE_SHADOW_MAP:
      return 1;
   case PIPE_CAP_TEXTURE_SWIZZLE:
      return 1;
   case PIPE_CAP_MAX_TEXTURE_2D_LEVELS:
      return SP_MAX_TEXTURE_2D_LEVELS;
   case PIPE_CAP_MAX_TEXTURE_3D_LEVELS:
      return SP_MAX_TEXTURE_3D_LEVELS;
   case PIPE_CAP_MAX_TEXTURE_CUBE_LEVELS:
      return SP_MAX_TEXTURE_2D_LEVELS;
   case PIPE_CAP_BLEND_EQUATION_SEPARATE:
      return 1;
   case PIPE_CAP_INDEP_BLEND_ENABLE:
      return 1;
   case PIPE_CAP_INDEP_BLEND_FUNC:
      return 1;
   case PIPE_CAP_TGSI_FS_COORD_ORIGIN_UPPER_LEFT:
   case PIPE_CAP_TGSI_FS_COORD_ORIGIN_LOWER_LEFT:
   case PIPE_CAP_TGSI_FS_COORD_PIXEL_CENTER_HALF_INTEGER:
   case PIPE_CAP_TGSI_FS_COORD_PIXEL_CENTER_INTEGER:
      return 1;
   case PIPE_CAP_MAX_STREAM_OUTPUT_BUFFERS:
      return PIPE_MAX_SO_BUFFERS;
   case PIPE_CAP_MAX_STREAM_OUTPUT_SEPARATE_COMPONENTS:
   case PIPE_CAP_MAX_STREAM_OUTPUT_INTERLEAVED_COMPONENTS:
      return 16*4;
   case PIPE_CAP_PRIMITIVE_RESTART:
      return 1;
   case PIPE_CAP_DEPTHSTENCIL_CLEAR_SEPARATE:
      return 0;
   case PIPE_CAP_SHADER_STENCIL_EXPORT:
      return 1;
   case PIPE_CAP_TGSI_INSTANCEID:
   case PIPE_CAP_VERTEX_ELEMENT_INSTANCE_DIVISOR:
      return 1;
   case PIPE_CAP_MAX_TEXTURE_ARRAY_LAYERS:
      return 256; /* for GL3 */
   case PIPE_CAP_MIN_TEXEL_OFFSET:
      return -8;
   case PIPE_CAP_MAX_TEXEL_OFFSET:
      return 7;
   case PIPE_CAP_CONDITIONAL_RENDER:
      return 1;
   case PIPE_CAP_FRAGMENT_COLOR_CLAMP_CONTROL:
      return 1;
   default:
      return 0;
   }
}

static int
softpipe_get_shader_param(struct pipe_screen *screen, unsigned shader, enum pipe_shader_cap param)
{
#ifdef HAVE_LLVM
   struct softpipe_screen *sp_screen = softpipe_screen(screen);
#endif
   switch(shader)
   {
   case PIPE_SHADER_FRAGMENT:
      return tgsi_exec_get_shader_param(param);
   case PIPE_SHADER_VERTEX:
   case PIPE_SHADER_GEOMETRY:
      switch (param) {
      case PIPE_SHADER_CAP_MAX_TEXTURE_SAMPLERS:
#ifdef HAVE_LLVM
         if (sp_screen->use_llvm)
            /* Softpipe doesn't yet know how to tell draw/llvm about textures */
            return 0;
#endif
         return PIPE_MAX_VERTEX_SAMPLERS;
      case PIPE_SHADER_CAP_INTEGERS:
#ifdef HAVE_LLVM /* gallivm doesn't support integers yet */
         if (sp_screen->use_llvm)
            return 0;
#endif
         /* fallthrough */
      default:
         return draw_get_shader_param(shader, param);
      }
   default:
      return 0;
   }
}

static float
softpipe_get_paramf(struct pipe_screen *screen, enum pipe_capf param)
{
   switch (param) {
   case PIPE_CAPF_MAX_LINE_WIDTH:
      /* fall-through */
   case PIPE_CAPF_MAX_LINE_WIDTH_AA:
      return 255.0; /* arbitrary */
   case PIPE_CAPF_MAX_POINT_WIDTH:
      /* fall-through */
   case PIPE_CAPF_MAX_POINT_WIDTH_AA:
      return 255.0; /* arbitrary */
   case PIPE_CAPF_MAX_TEXTURE_ANISOTROPY:
      return 16.0;
   case PIPE_CAPF_MAX_TEXTURE_LOD_BIAS:
      return 16.0; /* arbitrary */
   default:
      return 0;
   }
}

static int
softpipe_get_video_param(struct pipe_screen *screen,
                         enum pipe_video_profile profile,
                         enum pipe_video_cap param)
{
   switch (param) {
   case PIPE_VIDEO_CAP_SUPPORTED:
       return vl_profile_supported(screen, profile);
   case PIPE_VIDEO_CAP_NPOT_TEXTURES:
      return 0;
   case PIPE_VIDEO_CAP_MAX_WIDTH:
   case PIPE_VIDEO_CAP_MAX_HEIGHT:
      return vl_video_buffer_max_size(screen);
   default:
      return 0;
   }
}

/**
 * Query format support for creating a texture, drawing surface, etc.
 * \param format  the format to test
 * \param type  one of PIPE_TEXTURE, PIPE_SURFACE
 */
static boolean
softpipe_is_format_supported( struct pipe_screen *screen,
                              enum pipe_format format,
                              enum pipe_texture_target target,
                              unsigned sample_count,
                              unsigned bind)
{
   struct sw_winsys *winsys = softpipe_screen(screen)->winsys;
   const struct util_format_description *format_desc;

   assert(target == PIPE_BUFFER ||
          target == PIPE_TEXTURE_1D ||
          target == PIPE_TEXTURE_1D_ARRAY ||
          target == PIPE_TEXTURE_2D ||
          target == PIPE_TEXTURE_2D_ARRAY ||
          target == PIPE_TEXTURE_RECT ||
          target == PIPE_TEXTURE_3D ||
          target == PIPE_TEXTURE_CUBE);

   format_desc = util_format_description(format);
   if (!format_desc)
      return FALSE;

   if (sample_count > 1)
      return FALSE;

   if (bind & (PIPE_BIND_DISPLAY_TARGET |
               PIPE_BIND_SCANOUT |
               PIPE_BIND_SHARED)) {
      if(!winsys->is_displaytarget_format_supported(winsys, bind, format))
         return FALSE;
   }

   if (bind & PIPE_BIND_RENDER_TARGET) {
      if (format_desc->colorspace == UTIL_FORMAT_COLORSPACE_ZS)
         return FALSE;

      /*
       * Although possible, it is unnatural to render into compressed or YUV
       * surfaces. So disable these here to avoid going into weird paths
       * inside the state trackers.
       */
      if (format_desc->block.width != 1 ||
          format_desc->block.height != 1)
         return FALSE;
   }

   if (bind & PIPE_BIND_DEPTH_STENCIL) {
      if (format_desc->colorspace != UTIL_FORMAT_COLORSPACE_ZS)
         return FALSE;
   }

   /*
    * All other operations (sampling, transfer, etc).
    */

   if (format_desc->layout == UTIL_FORMAT_LAYOUT_S3TC) {
      return util_format_s3tc_enabled;
   }

   /*
    * Everything else should be supported by u_format.
    */
   return TRUE;
}


static void
softpipe_destroy_screen( struct pipe_screen *screen )
{
   struct softpipe_screen *sp_screen = softpipe_screen(screen);
   struct sw_winsys *winsys = sp_screen->winsys;

   if(winsys->destroy)
      winsys->destroy(winsys);

   FREE(screen);
}


/* This is often overriden by the co-state tracker.
 */
static void
softpipe_flush_frontbuffer(struct pipe_screen *_screen,
                           struct pipe_resource *resource,
                           unsigned level, unsigned layer,
                           void *context_private)
{
   struct softpipe_screen *screen = softpipe_screen(_screen);
   struct sw_winsys *winsys = screen->winsys;
   struct softpipe_resource *texture = softpipe_resource(resource);

   assert(texture->dt);
   if (texture->dt)
      winsys->displaytarget_display(winsys, texture->dt, context_private);
}

/**
 * Create a new pipe_screen object
 * Note: we're not presently subclassing pipe_screen (no softpipe_screen).
 */
struct pipe_screen *
softpipe_create_screen(struct sw_winsys *winsys)
{
   struct softpipe_screen *screen = CALLOC_STRUCT(softpipe_screen);

   if (!screen)
      return NULL;

   screen->winsys = winsys;

   screen->base.winsys = NULL;
   screen->base.destroy = softpipe_destroy_screen;

   screen->base.get_name = softpipe_get_name;
   screen->base.get_vendor = softpipe_get_vendor;
   screen->base.get_param = softpipe_get_param;
   screen->base.get_shader_param = softpipe_get_shader_param;
   screen->base.get_paramf = softpipe_get_paramf;
   screen->base.get_video_param = softpipe_get_video_param;
   screen->base.is_format_supported = softpipe_is_format_supported;
   screen->base.is_video_format_supported = vl_video_buffer_is_format_supported;
   screen->base.context_create = softpipe_create_context;
   screen->base.flush_frontbuffer = softpipe_flush_frontbuffer;

   screen->use_llvm = debug_get_option_use_llvm();

   util_format_s3tc_init();

   softpipe_init_screen_texture_funcs(&screen->base);
   softpipe_init_screen_fence_funcs(&screen->base);

   return &screen->base;
}
