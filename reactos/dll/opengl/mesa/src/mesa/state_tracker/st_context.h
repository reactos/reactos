/**************************************************************************
 * 
 * Copyright 2003 Tungsten Graphics, Inc., Cedar Park, Texas.
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

#ifndef ST_CONTEXT_H
#define ST_CONTEXT_H

#include "main/mtypes.h"
#include "pipe/p_state.h"
#include "state_tracker/st_api.h"

struct bitmap_cache;
struct blit_state;
struct dd_function_table;
struct draw_context;
struct draw_stage;
struct gen_mipmap_state;
struct st_context;
struct st_fragment_program;


#define ST_NEW_MESA                    0x1 /* Mesa state has changed */
#define ST_NEW_FRAGMENT_PROGRAM        0x2
#define ST_NEW_VERTEX_PROGRAM          0x4
#define ST_NEW_FRAMEBUFFER             0x8
#define ST_NEW_EDGEFLAGS_DATA          0x10
#define ST_NEW_GEOMETRY_PROGRAM        0x20


struct st_state_flags {
   GLuint mesa;
   GLuint st;
};

struct st_tracked_state {
   const char *name;
   struct st_state_flags dirty;
   void (*update)( struct st_context *st );
};



struct st_context
{
   struct st_context_iface iface;

   struct gl_context *ctx;

   struct pipe_context *pipe;

   struct draw_context *draw;  /**< For selection/feedback/rastpos only */
   struct draw_stage *feedback_stage;  /**< For GL_FEEDBACK rendermode */
   struct draw_stage *selection_stage;  /**< For GL_SELECT rendermode */
   struct draw_stage *rastpos_stage;  /**< For glRasterPos */
   GLboolean sw_primitive_restart;


   /* On old libGL's for linux we need to invalidate the drawables
    * on glViewpport calls, this is set via a option.
    */
   boolean invalidate_on_gl_viewport;

   /* Some state is contained in constant objects.
    * Other state is just parameter values.
    */
   struct {
      struct pipe_blend_state               blend;
      struct pipe_depth_stencil_alpha_state depth_stencil;
      struct pipe_rasterizer_state          rasterizer;
      struct pipe_sampler_state             samplers[PIPE_MAX_SAMPLERS];
      struct pipe_sampler_state             vertex_samplers[PIPE_MAX_VERTEX_SAMPLERS];
      struct pipe_clip_state clip;
      struct {
         void *ptr;
         unsigned size;
      } constants[PIPE_SHADER_TYPES];
      struct pipe_framebuffer_state framebuffer;
      struct pipe_sampler_view *sampler_views[PIPE_MAX_SAMPLERS];
      struct pipe_sampler_view *sampler_vertex_views[PIPE_MAX_VERTEX_SAMPLERS];
      struct pipe_scissor_state scissor;
      struct pipe_viewport_state viewport;
      unsigned sample_mask;

      GLuint num_samplers;
      GLuint num_vertex_samplers;
      GLuint num_textures;
      GLuint num_vertex_textures;

      GLuint poly_stipple[32];  /**< In OpenGL's bottom-to-top order */
   } state;

   char vendor[100];
   char renderer[100];

   struct st_state_flags dirty;

   GLboolean missing_textures;
   GLboolean vertdata_edgeflags;

   /** Mapping from VERT_RESULT_x to post-transformed vertex slot */
   const GLuint *vertex_result_to_slot;

   struct st_vertex_program *vp;    /**< Currently bound vertex program */
   struct st_fragment_program *fp;  /**< Currently bound fragment program */
   struct st_geometry_program *gp;  /**< Currently bound geometry program */

   struct st_vp_variant *vp_variant;
   struct st_fp_variant *fp_variant;
   struct st_gp_variant *gp_variant;

   struct gl_texture_object *default_texture;

   struct {
      struct gl_program_cache *cache;
      struct st_fragment_program *program;  /**< cur pixel transfer prog */
      GLuint xfer_prog_sn;  /**< pixel xfer program serial no. */
      GLuint user_prog_sn;  /**< user fragment program serial no. */
      struct st_fragment_program *combined_prog;
      GLuint combined_prog_sn;
      struct pipe_resource *pixelmap_texture;
      struct pipe_sampler_view *pixelmap_sampler_view;
      boolean pixelmap_enabled;  /**< use the pixelmap texture? */
   } pixel_xfer;

   /** for glBitmap */
   struct {
      struct pipe_rasterizer_state rasterizer;
      struct pipe_sampler_state samplers[2];
      enum pipe_format tex_format;
      void *vs;
      float vertices[4][3][4];  /**< vertex pos + color + texcoord */
      struct pipe_resource *vbuf;
      unsigned vbuf_slot;       /* next free slot in vbuf */
      struct bitmap_cache *cache;
   } bitmap;

   /** for glDraw/CopyPixels */
   struct {
      struct gl_fragment_program *shaders[4];
      void *vert_shaders[2];   /**< ureg shaders */
   } drawpix;

   /** for glClear */
   struct {
      struct pipe_rasterizer_state raster;
      struct pipe_viewport_state viewport;
      void *vs;
      void *fs;
      float vertices[4][2][4];  /**< vertex pos + color */
      struct pipe_resource *vbuf;
      unsigned vbuf_slot;
      boolean enable_ds_separate;
   } clear;

   /** used for anything using util_draw_vertex_buffer */
   struct pipe_vertex_element velems_util_draw[3];

   void *passthrough_fs;  /**< simple pass-through frag shader */

   enum pipe_texture_target internal_target;
   struct gen_mipmap_state *gen_mipmap;
   struct blit_state *blit;

   struct cso_context *cso_context;

   int force_msaa;
   void *winsys_drawable_handle;

   /* User vertex buffers. */
   struct {
      struct pipe_resource *buffer;

      /** Element size */
      GLuint element_size;

      /** Attribute stride */
      GLsizei stride;
   } user_attrib[PIPE_MAX_ATTRIBS];
   unsigned num_user_attribs;

   /* Active render condition. */
   struct pipe_query *render_condition;
   unsigned condition_mode;

   int32_t draw_stamp;
   int32_t read_stamp;
};


/* Need this so that we can implement Mesa callbacks in this module.
 */
static INLINE struct st_context *st_context(struct gl_context *ctx)
{
   return ctx->st;
}


/**
 * Wrapper for struct gl_framebuffer.
 * This is an opaque type to the outside world.
 */
struct st_framebuffer
{
   struct gl_framebuffer Base;
   void *Private;

   struct st_framebuffer_iface *iface;
   enum st_attachment_type statts[ST_ATTACHMENT_COUNT];
   unsigned num_statts;
   int32_t stamp;
   int32_t iface_stamp;
};


extern void st_init_driver_functions(struct dd_function_table *functions);

void st_invalidate_state(struct gl_context * ctx, GLuint new_state);



#define Y_0_TOP 1
#define Y_0_BOTTOM 2

static INLINE GLuint
st_fb_orientation(const struct gl_framebuffer *fb)
{
   if (fb && fb->Name == 0) {
      /* Drawing into a window (on-screen buffer).
       *
       * Negate Y scale to flip image vertically.
       * The NDC Y coords prior to viewport transformation are in the range
       * [y=-1=bottom, y=1=top]
       * Hardware window coords are in the range [y=0=top, y=H-1=bottom] where
       * H is the window height.
       * Use the viewport transformation to invert Y.
       */
      return Y_0_TOP;
   }
   else {
      /* Drawing into user-created FBO (very likely a texture).
       *
       * For textures, T=0=Bottom, so by extension Y=0=Bottom for rendering.
       */
      return Y_0_BOTTOM;
   }
}


/** clear-alloc a struct-sized object, with casting */
#define ST_CALLOC_STRUCT(T)   (struct T *) calloc(1, sizeof(struct T))


extern int
st_get_msaa(void);

extern struct st_context *
st_create_context(gl_api api, struct pipe_context *pipe,
                  const struct gl_config *visual,
                  struct st_context *share);

extern void
st_destroy_context(struct st_context *st);


#endif
