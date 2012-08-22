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


/**
 * @file
 * 
 * Abstract graphics pipe state objects.
 *
 * Basic notes:
 *   1. Want compact representations, so we use bitfields.
 *   2. Put bitfields before other (GLfloat) fields.
 */


#ifndef PIPE_STATE_H
#define PIPE_STATE_H

#include "p_compiler.h"
#include "p_defines.h"
#include "p_format.h"


#ifdef __cplusplus
extern "C" {
#endif


/**
 * Implementation limits
 */
#define PIPE_MAX_ATTRIBS          32
#define PIPE_MAX_CLIP_PLANES       8
#define PIPE_MAX_COLOR_BUFS        8
#define PIPE_MAX_CONSTANT_BUFFERS 32
#define PIPE_MAX_SAMPLERS         16
#define PIPE_MAX_VERTEX_SAMPLERS  16
#define PIPE_MAX_GEOMETRY_SAMPLERS  16
#define PIPE_MAX_SHADER_INPUTS    32
#define PIPE_MAX_SHADER_OUTPUTS   32
#define PIPE_MAX_SHADER_RESOURCES 32
#define PIPE_MAX_TEXTURE_LEVELS   16
#define PIPE_MAX_SO_BUFFERS        4


struct pipe_reference
{
   int32_t count; /* atomic */
};



/**
 * Primitive (point/line/tri) rasterization info
 */
struct pipe_rasterizer_state
{
   unsigned flatshade:1;
   unsigned light_twoside:1;
   unsigned clamp_vertex_color:1;
   unsigned clamp_fragment_color:1;
   unsigned front_ccw:1;
   unsigned cull_face:2;      /**< PIPE_FACE_x */
   unsigned fill_front:2;     /**< PIPE_POLYGON_MODE_x */
   unsigned fill_back:2;      /**< PIPE_POLYGON_MODE_x */
   unsigned offset_point:1;
   unsigned offset_line:1;
   unsigned offset_tri:1;
   unsigned scissor:1;
   unsigned poly_smooth:1;
   unsigned poly_stipple_enable:1;
   unsigned point_smooth:1;
   unsigned sprite_coord_mode:1;     /**< PIPE_SPRITE_COORD_ */
   unsigned point_quad_rasterization:1; /** points rasterized as quads or points */
   unsigned point_size_per_vertex:1; /**< size computed in vertex shader */
   unsigned multisample:1;         /* XXX maybe more ms state in future */
   unsigned line_smooth:1;
   unsigned line_stipple_enable:1;
   unsigned line_last_pixel:1;

   /**
    * Use the first vertex of a primitive as the provoking vertex for
    * flat shading.
    */
   unsigned flatshade_first:1;

   /**
    * When true, triangle rasterization uses (0.5, 0.5) pixel centers
    * for determining pixel ownership.
    *
    * When false, triangle rasterization uses (0,0) pixel centers for
    * determining pixel ownership.
    *
    * Triangle rasterization always uses a 'top,left' rule for pixel
    * ownership, this just alters which point we consider the pixel
    * center for that test.
    */
   unsigned gl_rasterization_rules:1;

   /**
    * When true, rasterization is disabled and no pixels are written.
    * This only makes sense with the Stream Out functionality.
    */
   unsigned rasterizer_discard:1;

   /**
    * When false, depth clipping is disabled and the depth value will be
    * clamped later at the per-pixel level before depth testing.
    * This depends on PIPE_CAP_DEPTH_CLIP_DISABLE.
    */
   unsigned depth_clip:1;

   /**
    * Enable bits for clipping half-spaces.
    * This applies to both user clip planes and shader clip distances.
    * Note that if the bound shader exports any clip distances, these
    * replace all user clip planes, and clip half-spaces enabled here
    * but not written by the shader count as disabled.
    */
   unsigned clip_plane_enable:PIPE_MAX_CLIP_PLANES;

   unsigned line_stipple_factor:8;  /**< [1..256] actually */
   unsigned line_stipple_pattern:16;

   unsigned sprite_coord_enable:PIPE_MAX_SHADER_OUTPUTS;

   float line_width;
   float point_size;           /**< used when no per-vertex size */
   float offset_units;
   float offset_scale;
   float offset_clamp;
};


struct pipe_poly_stipple
{
   unsigned stipple[32];
};


struct pipe_viewport_state
{
   float scale[4];
   float translate[4];
};


struct pipe_scissor_state
{
   unsigned minx:16;
   unsigned miny:16;
   unsigned maxx:16;
   unsigned maxy:16;
};


struct pipe_clip_state
{
   float ucp[PIPE_MAX_CLIP_PLANES][4];
};


/**
 * Stream output for vertex transform feedback.
 */
struct pipe_stream_output_info
{
   unsigned num_outputs;
   /** stride for an entire vertex, only used if all output_buffers are 0 */
   unsigned stride;
   /**
    * Array of stream outputs, in the order they are to be written in.
    * Selected components are tightly packed into the output buffer.
    */
   struct {
      unsigned register_index:8; /**< 0 to PIPE_MAX_SHADER_OUTPUTS */
      unsigned register_mask:4;  /**< TGSI_WRITEMASK_x */
      unsigned output_buffer:4;  /**< 0 to PIPE_MAX_SO_BUFFERS */
   } output[PIPE_MAX_SHADER_OUTPUTS];
};


struct pipe_shader_state
{
   const struct tgsi_token *tokens;
   struct pipe_stream_output_info stream_output;
};


struct pipe_depth_state 
{
   unsigned enabled:1;         /**< depth test enabled? */
   unsigned writemask:1;       /**< allow depth buffer writes? */
   unsigned func:3;            /**< depth test func (PIPE_FUNC_x) */
};


struct pipe_stencil_state
{
   unsigned enabled:1;  /**< stencil[0]: stencil enabled, stencil[1]: two-side enabled */
   unsigned func:3;     /**< PIPE_FUNC_x */
   unsigned fail_op:3;  /**< PIPE_STENCIL_OP_x */
   unsigned zpass_op:3; /**< PIPE_STENCIL_OP_x */
   unsigned zfail_op:3; /**< PIPE_STENCIL_OP_x */
   unsigned valuemask:8;
   unsigned writemask:8;
};


struct pipe_alpha_state
{
   unsigned enabled:1;
   unsigned func:3;     /**< PIPE_FUNC_x */
   float ref_value;     /**< reference value */
};


struct pipe_depth_stencil_alpha_state
{
   struct pipe_depth_state depth;
   struct pipe_stencil_state stencil[2]; /**< [0] = front, [1] = back */
   struct pipe_alpha_state alpha;
};


struct pipe_rt_blend_state
{
   unsigned blend_enable:1;

   unsigned rgb_func:3;          /**< PIPE_BLEND_x */
   unsigned rgb_src_factor:5;    /**< PIPE_BLENDFACTOR_x */
   unsigned rgb_dst_factor:5;    /**< PIPE_BLENDFACTOR_x */

   unsigned alpha_func:3;        /**< PIPE_BLEND_x */
   unsigned alpha_src_factor:5;  /**< PIPE_BLENDFACTOR_x */
   unsigned alpha_dst_factor:5;  /**< PIPE_BLENDFACTOR_x */

   unsigned colormask:4;         /**< bitmask of PIPE_MASK_R/G/B/A */
};

struct pipe_blend_state
{
   unsigned independent_blend_enable:1;
   unsigned logicop_enable:1;
   unsigned logicop_func:4;      /**< PIPE_LOGICOP_x */
   unsigned dither:1;
   unsigned alpha_to_coverage:1;
   unsigned alpha_to_one:1;
   struct pipe_rt_blend_state rt[PIPE_MAX_COLOR_BUFS];
};


struct pipe_blend_color
{
   float color[4];
};

struct pipe_stencil_ref
{
   ubyte ref_value[2];
};

struct pipe_framebuffer_state
{
   unsigned width, height;

   /** multiple color buffers for multiple render targets */
   unsigned nr_cbufs;
   struct pipe_surface *cbufs[PIPE_MAX_COLOR_BUFS];

   struct pipe_surface *zsbuf;      /**< Z/stencil buffer */
};


/**
 * Texture sampler state.
 */
struct pipe_sampler_state
{
   unsigned wrap_s:3;            /**< PIPE_TEX_WRAP_x */
   unsigned wrap_t:3;            /**< PIPE_TEX_WRAP_x */
   unsigned wrap_r:3;            /**< PIPE_TEX_WRAP_x */
   unsigned min_img_filter:2;    /**< PIPE_TEX_FILTER_x */
   unsigned min_mip_filter:2;    /**< PIPE_TEX_MIPFILTER_x */
   unsigned mag_img_filter:2;    /**< PIPE_TEX_FILTER_x */
   unsigned compare_mode:1;      /**< PIPE_TEX_COMPARE_x */
   unsigned compare_func:3;      /**< PIPE_FUNC_x */
   unsigned normalized_coords:1; /**< Are coords normalized to [0,1]? */
   unsigned max_anisotropy:6;
   unsigned seamless_cube_map:1;
   float lod_bias;               /**< LOD/lambda bias */
   float min_lod, max_lod;       /**< LOD clamp range, after bias */
   union pipe_color_union border_color;
};


/**
 * A view into a texture that can be bound to a color render target /
 * depth stencil attachment point.
 */
struct pipe_surface
{
   struct pipe_reference reference;
   struct pipe_resource *texture; /**< resource into which this is a view  */
   struct pipe_context *context; /**< context this view belongs to */
   enum pipe_format format;

   /* XXX width/height should be removed */
   unsigned width;               /**< logical width in pixels */
   unsigned height;              /**< logical height in pixels */

   unsigned usage;               /**< bitmask of PIPE_BIND_x */

   union {
      struct {
         unsigned level;
         unsigned first_layer:16;
         unsigned last_layer:16;
      } tex;
      struct {
         unsigned first_element;
         unsigned last_element;
      } buf;
   } u;
};


/**
 * A view into a texture that can be bound to a shader stage.
 */
struct pipe_sampler_view
{
   struct pipe_reference reference;
   enum pipe_format format;      /**< typed PIPE_FORMAT_x */
   struct pipe_resource *texture; /**< texture into which this is a view  */
   struct pipe_context *context; /**< context this view belongs to */
   union {
      struct {
         unsigned first_layer:16;     /**< first layer to use for array textures */
         unsigned last_layer:16;      /**< last layer to use for array textures */
         unsigned first_level:8;      /**< first mipmap level to use */
         unsigned last_level:8;       /**< last mipmap level to use */
      } tex;
      struct {
         unsigned first_element;
         unsigned last_element;
      } buf;
   } u;
   unsigned swizzle_r:3;         /**< PIPE_SWIZZLE_x for red component */
   unsigned swizzle_g:3;         /**< PIPE_SWIZZLE_x for green component */
   unsigned swizzle_b:3;         /**< PIPE_SWIZZLE_x for blue component */
   unsigned swizzle_a:3;         /**< PIPE_SWIZZLE_x for alpha component */
};


/**
 * Subregion of 1D/2D/3D image resource.
 */
struct pipe_box
{
   unsigned x;
   unsigned y;
   unsigned z;
   unsigned width;
   unsigned height;
   unsigned depth;
};


/**
 * A memory object/resource such as a vertex buffer or texture.
 */
struct pipe_resource
{
   struct pipe_reference reference;
   struct pipe_screen *screen; /**< screen that this texture belongs to */
   enum pipe_texture_target target; /**< PIPE_TEXTURE_x */
   enum pipe_format format;         /**< PIPE_FORMAT_x */

   unsigned width0;
   unsigned height0;
   unsigned depth0;
   unsigned array_size;

   unsigned last_level:8;    /**< Index of last mipmap level present/defined */
   unsigned nr_samples:8;    /**< for multisampled surfaces, nr of samples */
   unsigned usage:8;         /**< PIPE_USAGE_x (not a bitmask) */

   unsigned bind;            /**< bitmask of PIPE_BIND_x */
   unsigned flags;           /**< bitmask of PIPE_RESOURCE_FLAG_x */
};


/**
 * Transfer object.  For data transfer to/from a resource.
 */
struct pipe_transfer
{
   struct pipe_resource *resource; /**< resource to transfer to/from  */
   unsigned level;                 /**< texture mipmap level */
   enum pipe_transfer_usage usage;
   struct pipe_box box;            /**< region of the resource to access */
   unsigned stride;                /**< row stride in bytes */
   unsigned layer_stride;          /**< image/layer stride in bytes */
   void *data;
};



/**
 * A vertex buffer.  Typically, all the vertex data/attributes for
 * drawing something will be in one buffer.  But it's also possible, for
 * example, to put colors in one buffer and texcoords in another.
 */
struct pipe_vertex_buffer
{
   unsigned stride;    /**< stride to same attrib in next vertex, in bytes */
   unsigned buffer_offset;  /**< offset to start of data in buffer, in bytes */
   struct pipe_resource *buffer;  /**< the actual buffer */
};


/**
 * A stream output target. The structure specifies the range vertices can
 * be written to.
 *
 * In addition to that, the structure should internally maintain the offset
 * into the buffer, which should be incremented everytime something is written
 * (appended) to it. The internal offset is buffer_offset + how many bytes
 * have been written. The internal offset can be stored on the device
 * and the CPU actually doesn't have to query it.
 *
 * Use PIPE_QUERY_SO_STATISTICS to know how many primitives have
 * actually been written.
 */
struct pipe_stream_output_target
{
   struct pipe_reference reference;
   struct pipe_resource *buffer; /**< buffer into which this is a target view */
   struct pipe_context *context; /**< context this view belongs to */

   unsigned buffer_offset;  /**< offset where data should be written, in bytes */
   unsigned buffer_size;    /**< how much data is allowed to be written */
};


/**
 * Information to describe a vertex attribute (position, color, etc)
 */
struct pipe_vertex_element
{
   /** Offset of this attribute, in bytes, from the start of the vertex */
   unsigned src_offset;

   /** Instance data rate divisor. 0 means this is per-vertex data,
    *  n means per-instance data used for n consecutive instances (n > 0).
    */
   unsigned instance_divisor;

   /** Which vertex_buffer (as given to pipe->set_vertex_buffer()) does
    * this attribute live in?
    */
   unsigned vertex_buffer_index;
 
   enum pipe_format src_format;
};


/**
 * An index buffer.  When an index buffer is bound, all indices to vertices
 * will be looked up in the buffer.
 */
struct pipe_index_buffer
{
   unsigned index_size;  /**< size of an index, in bytes */
   unsigned offset;  /**< offset to start of data in buffer, in bytes */
   struct pipe_resource *buffer; /**< the actual buffer */
};


/**
 * Information to describe a draw_vbo call.
 */
struct pipe_draw_info
{
   boolean indexed;  /**< use index buffer */

   unsigned mode;  /**< the mode of the primitive */
   unsigned start;  /**< the index of the first vertex */
   unsigned count;  /**< number of vertices */

   unsigned start_instance; /**< first instance id */
   unsigned instance_count; /**< number of instances */

   /**
    * For indexed drawing, these fields apply after index lookup.
    */
   int index_bias; /**< a bias to be added to each index */
   unsigned min_index; /**< the min index */
   unsigned max_index; /**< the max index */

   /**
    * Primitive restart enable/index (only applies to indexed drawing)
    */
   boolean primitive_restart;
   unsigned restart_index;

   /**
    * Stream output target. If not NULL, it's used to provide the 'count'
    * parameter based on the number vertices captured by the stream output
    * stage. (or generally, based on the number of bytes captured)
    *
    * Only 'mode', 'start_instance', and 'instance_count' are taken into
    * account, all the other variables from pipe_draw_info are ignored.
    *
    * 'start' is implicitly 0 and 'count' is set as discussed above.
    * The draw command is non-indexed.
    *
    * Note that this only provides the count. The vertex buffers must
    * be set via set_vertex_buffers manually.
    */
   struct pipe_stream_output_target *count_from_stream_output;
};


/**
 * Information to describe a resource_resolve call.
 */
struct pipe_resolve_info
{
   struct {
      struct pipe_resource *res;
      unsigned level;
      unsigned layer;
      int x0; /**< always left */
      int y0; /**< always top */
      int x1; /**< determines scale if PIPE_CAP_SCALED_RESOLVE is supported */
      int y1; /**< determines scale if PIPE_CAP_SCALED_RESOLVE is supported */
   } dst;

   struct {
      struct pipe_resource *res;
      unsigned layer;
      int x0;
      int y0;
      int x1; /**< may be < x0 only if PIPE_CAP_SCALED_RESOLVE is supported */
      int y1; /**< may be < y1 even if PIPE_CAP_SCALED_RESOLVE not supported */
   } src;

   unsigned mask; /**< PIPE_MASK_RGBA, Z, S or ZS */
};


#ifdef __cplusplus
}
#endif
   
#endif
