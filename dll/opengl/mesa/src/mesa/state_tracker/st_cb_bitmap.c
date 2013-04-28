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

 /*
  * Authors:
  *   Brian Paul
  */

#include "main/imports.h"
#include "main/image.h"
#include "main/bufferobj.h"
#include "main/macros.h"
#include "main/mfeatures.h"
#include "main/pbo.h"
#include "program/program.h"
#include "program/prog_print.h"

#include "st_context.h"
#include "st_atom.h"
#include "st_atom_constbuf.h"
#include "st_program.h"
#include "st_cb_bitmap.h"
#include "st_texture.h"

#include "pipe/p_context.h"
#include "pipe/p_defines.h"
#include "pipe/p_shader_tokens.h"
#include "util/u_inlines.h"
#include "util/u_draw_quad.h"
#include "util/u_simple_shaders.h"
#include "program/prog_instruction.h"
#include "cso_cache/cso_context.h"


#if FEATURE_drawpix

/**
 * glBitmaps are drawn as textured quads.  The user's bitmap pattern
 * is stored in a texture image.  An alpha8 texture format is used.
 * The fragment shader samples a bit (texel) from the texture, then
 * discards the fragment if the bit is off.
 *
 * Note that we actually store the inverse image of the bitmap to
 * simplify the fragment program.  An "on" bit gets stored as texel=0x0
 * and an "off" bit is stored as texel=0xff.  Then we kill the
 * fragment if the negated texel value is less than zero.
 */


/**
 * The bitmap cache attempts to accumulate multiple glBitmap calls in a
 * buffer which is then rendered en mass upon a flush, state change, etc.
 * A wide, short buffer is used to target the common case of a series
 * of glBitmap calls being used to draw text.
 */
static GLboolean UseBitmapCache = GL_TRUE;


#define BITMAP_CACHE_WIDTH  512
#define BITMAP_CACHE_HEIGHT 32

struct bitmap_cache
{
   /** Window pos to render the cached image */
   GLint xpos, ypos;
   /** Bounds of region used in window coords */
   GLint xmin, ymin, xmax, ymax;

   GLfloat color[4];

   /** Bitmap's Z position */
   GLfloat zpos;

   struct pipe_resource *texture;
   struct pipe_transfer *trans;

   GLboolean empty;

   /** An I8 texture image: */
   ubyte *buffer;
};


/** Epsilon for Z comparisons */
#define Z_EPSILON 1e-06


/**
 * Make fragment program for glBitmap:
 *   Sample the texture and kill the fragment if the bit is 0.
 * This program will be combined with the user's fragment program.
 */
static struct st_fragment_program *
make_bitmap_fragment_program(struct gl_context *ctx, GLuint samplerIndex)
{
   struct st_context *st = st_context(ctx);
   struct st_fragment_program *stfp;
   struct gl_program *p;
   GLuint ic = 0;

   p = ctx->Driver.NewProgram(ctx, GL_FRAGMENT_PROGRAM_ARB, 0);
   if (!p)
      return NULL;

   p->NumInstructions = 3;

   p->Instructions = _mesa_alloc_instructions(p->NumInstructions);
   if (!p->Instructions) {
      ctx->Driver.DeleteProgram(ctx, p);
      return NULL;
   }
   _mesa_init_instructions(p->Instructions, p->NumInstructions);

   /* TEX tmp0, fragment.texcoord[0], texture[0], 2D; */
   p->Instructions[ic].Opcode = OPCODE_TEX;
   p->Instructions[ic].DstReg.File = PROGRAM_TEMPORARY;
   p->Instructions[ic].DstReg.Index = 0;
   p->Instructions[ic].SrcReg[0].File = PROGRAM_INPUT;
   p->Instructions[ic].SrcReg[0].Index = FRAG_ATTRIB_TEX0;
   p->Instructions[ic].TexSrcUnit = samplerIndex;
   p->Instructions[ic].TexSrcTarget = TEXTURE_2D_INDEX;
   ic++;

   /* KIL if -tmp0 < 0 # texel=0 -> keep / texel=0 -> discard */
   p->Instructions[ic].Opcode = OPCODE_KIL;
   p->Instructions[ic].SrcReg[0].File = PROGRAM_TEMPORARY;

   if (st->bitmap.tex_format == PIPE_FORMAT_L8_UNORM)
      p->Instructions[ic].SrcReg[0].Swizzle = SWIZZLE_XXXX;

   p->Instructions[ic].SrcReg[0].Index = 0;
   p->Instructions[ic].SrcReg[0].Negate = NEGATE_XYZW;
   ic++;

   /* END; */
   p->Instructions[ic++].Opcode = OPCODE_END;

   assert(ic == p->NumInstructions);

   p->InputsRead = FRAG_BIT_TEX0;
   p->OutputsWritten = 0x0;
   p->SamplersUsed = (1 << samplerIndex);

   stfp = (struct st_fragment_program *) p;
   stfp->Base.UsesKill = GL_TRUE;

   return stfp;
}


static struct gl_program *
make_bitmap_fragment_program_glsl(struct st_context *st,
                                  struct st_fragment_program *orig,
                                  GLuint samplerIndex)
{
   struct gl_context *ctx = st->ctx;
   struct st_fragment_program *fp = (struct st_fragment_program *)
      ctx->Driver.NewProgram(ctx, GL_FRAGMENT_PROGRAM_ARB, 0);

   if (!fp)
      return NULL;
   
   get_bitmap_visitor(fp, orig->glsl_to_tgsi, samplerIndex);
   return &fp->Base.Base;
}


static int
find_free_bit(uint bitfield)
{
   int i;
   for (i = 0; i < 32; i++) {
      if ((bitfield & (1 << i)) == 0) {
         return i;
      }
   }
   return -1;
}


/**
 * Combine basic bitmap fragment program with the user-defined program.
 * \param st  current context
 * \param fpIn  the incoming fragment program
 * \param fpOut  the new fragment program which does fragment culling
 * \param bitmap_sampler  sampler number for the bitmap texture
 */
void
st_make_bitmap_fragment_program(struct st_context *st,
                                struct gl_fragment_program *fpIn,
                                struct gl_fragment_program **fpOut,
                                GLuint *bitmap_sampler)
{
   struct st_fragment_program *bitmap_prog;
   struct st_fragment_program *stfpIn = (struct st_fragment_program *) fpIn;
   struct gl_program *newProg;
   uint sampler;

   /*
    * Generate new program which is the user-defined program prefixed
    * with the bitmap sampler/kill instructions.
    */
   sampler = find_free_bit(fpIn->Base.SamplersUsed);
   
   if (stfpIn->glsl_to_tgsi)
      newProg = make_bitmap_fragment_program_glsl(st, stfpIn, sampler);
   else {
      bitmap_prog = make_bitmap_fragment_program(st->ctx, sampler);

      newProg = _mesa_combine_programs(st->ctx,
                                       &bitmap_prog->Base.Base,
                                       &fpIn->Base);
      /* done with this after combining */
      st_reference_fragprog(st, &bitmap_prog, NULL);
   }

#if 0
   {
      printf("Combined bitmap program:\n");
      _mesa_print_program(newProg);
      printf("InputsRead: 0x%x\n", newProg->InputsRead);
      printf("OutputsWritten: 0x%x\n", newProg->OutputsWritten);
      _mesa_print_parameter_list(newProg->Parameters);
   }
#endif

   /* return results */
   *fpOut = (struct gl_fragment_program *) newProg;
   *bitmap_sampler = sampler;
}


/**
 * Copy user-provide bitmap bits into texture buffer, expanding
 * bits into texels.
 * "On" bits will set texels to 0x0.
 * "Off" bits will not modify texels.
 * Note that the image is actually going to be upside down in
 * the texture.  We deal with that with texcoords.
 */
static void
unpack_bitmap(struct st_context *st,
              GLint px, GLint py, GLsizei width, GLsizei height,
              const struct gl_pixelstore_attrib *unpack,
              const GLubyte *bitmap,
              ubyte *destBuffer, uint destStride)
{
   destBuffer += py * destStride + px;

   _mesa_expand_bitmap(width, height, unpack, bitmap,
                       destBuffer, destStride, 0x0);
}


/**
 * Create a texture which represents a bitmap image.
 */
static struct pipe_resource *
make_bitmap_texture(struct gl_context *ctx, GLsizei width, GLsizei height,
                    const struct gl_pixelstore_attrib *unpack,
                    const GLubyte *bitmap)
{
   struct st_context *st = st_context(ctx);
   struct pipe_context *pipe = st->pipe;
   struct pipe_transfer *transfer;
   ubyte *dest;
   struct pipe_resource *pt;

   /* PBO source... */
   bitmap = _mesa_map_pbo_source(ctx, unpack, bitmap);
   if (!bitmap) {
      return NULL;
   }

   /**
    * Create texture to hold bitmap pattern.
    */
   pt = st_texture_create(st, st->internal_target, st->bitmap.tex_format,
                          0, width, height, 1, 1,
                          PIPE_BIND_SAMPLER_VIEW);
   if (!pt) {
      _mesa_unmap_pbo_source(ctx, unpack);
      return NULL;
   }

   transfer = pipe_get_transfer(st->pipe, pt, 0, 0,
                                PIPE_TRANSFER_WRITE,
                                0, 0, width, height);

   dest = pipe_transfer_map(pipe, transfer);

   /* Put image into texture transfer */
   memset(dest, 0xff, height * transfer->stride);
   unpack_bitmap(st, 0, 0, width, height, unpack, bitmap,
                 dest, transfer->stride);

   _mesa_unmap_pbo_source(ctx, unpack);

   /* Release transfer */
   pipe_transfer_unmap(pipe, transfer);
   pipe->transfer_destroy(pipe, transfer);

   return pt;
}

static GLuint
setup_bitmap_vertex_data(struct st_context *st, bool normalized,
                         int x, int y, int width, int height,
                         float z, const float color[4])
{
   struct pipe_context *pipe = st->pipe;
   const struct gl_framebuffer *fb = st->ctx->DrawBuffer;
   const GLfloat fb_width = (GLfloat)fb->Width;
   const GLfloat fb_height = (GLfloat)fb->Height;
   const GLfloat x0 = (GLfloat)x;
   const GLfloat x1 = (GLfloat)(x + width);
   const GLfloat y0 = (GLfloat)y;
   const GLfloat y1 = (GLfloat)(y + height);
   GLfloat sLeft = (GLfloat)0.0, sRight = (GLfloat)1.0;
   GLfloat tTop = (GLfloat)0.0, tBot = (GLfloat)1.0 - tTop;
   const GLfloat clip_x0 = (GLfloat)(x0 / fb_width * 2.0 - 1.0);
   const GLfloat clip_y0 = (GLfloat)(y0 / fb_height * 2.0 - 1.0);
   const GLfloat clip_x1 = (GLfloat)(x1 / fb_width * 2.0 - 1.0);
   const GLfloat clip_y1 = (GLfloat)(y1 / fb_height * 2.0 - 1.0);
   const GLuint max_slots = 1; /* 4096 / sizeof(st->bitmap.vertices); */
   GLuint i;

   if(!normalized)
   {
      sRight = (GLfloat) width;
      tBot = (GLfloat) height;
   }

   /* XXX: Need to improve buffer_write to allow NO_WAIT (as well as
    * no_flush) updates to buffers where we know there is no conflict
    * with previous data.  Currently using max_slots > 1 will cause
    * synchronous rendering if the driver flushes its command buffers
    * between one bitmap and the next.  Our flush hook below isn't
    * sufficient to catch this as the driver doesn't tell us when it
    * flushes its own command buffers.  Until this gets fixed, pay the
    * price of allocating a new buffer for each bitmap cache-flush to
    * avoid synchronous rendering.
    */
   if (st->bitmap.vbuf_slot >= max_slots) {
      pipe_resource_reference(&st->bitmap.vbuf, NULL);
      st->bitmap.vbuf_slot = 0;
   }

   if (!st->bitmap.vbuf) {
      st->bitmap.vbuf = pipe_buffer_create(pipe->screen, 
                                           PIPE_BIND_VERTEX_BUFFER,
                                           PIPE_USAGE_STREAM,
                                           max_slots *
                                           sizeof(st->bitmap.vertices));
      if (!st->bitmap.vbuf) {
         /* out of memory */
         return 0;
      }
   }

   /* Positions are in clip coords since we need to do clipping in case
    * the bitmap quad goes beyond the window bounds.
    */
   st->bitmap.vertices[0][0][0] = clip_x0;
   st->bitmap.vertices[0][0][1] = clip_y0;
   st->bitmap.vertices[0][2][0] = sLeft;
   st->bitmap.vertices[0][2][1] = tTop;

   st->bitmap.vertices[1][0][0] = clip_x1;
   st->bitmap.vertices[1][0][1] = clip_y0;
   st->bitmap.vertices[1][2][0] = sRight;
   st->bitmap.vertices[1][2][1] = tTop;
   
   st->bitmap.vertices[2][0][0] = clip_x1;
   st->bitmap.vertices[2][0][1] = clip_y1;
   st->bitmap.vertices[2][2][0] = sRight;
   st->bitmap.vertices[2][2][1] = tBot;
   
   st->bitmap.vertices[3][0][0] = clip_x0;
   st->bitmap.vertices[3][0][1] = clip_y1;
   st->bitmap.vertices[3][2][0] = sLeft;
   st->bitmap.vertices[3][2][1] = tBot;
   
   /* same for all verts: */
   for (i = 0; i < 4; i++) {
      st->bitmap.vertices[i][0][2] = z;
      st->bitmap.vertices[i][0][3] = 1.0f;
      st->bitmap.vertices[i][1][0] = color[0];
      st->bitmap.vertices[i][1][1] = color[1];
      st->bitmap.vertices[i][1][2] = color[2];
      st->bitmap.vertices[i][1][3] = color[3];
      st->bitmap.vertices[i][2][2] = 0.0; /*R*/
      st->bitmap.vertices[i][2][3] = 1.0; /*Q*/
   }

   /* put vertex data into vbuf */
   pipe_buffer_write_nooverlap(st->pipe,
                               st->bitmap.vbuf,
                               st->bitmap.vbuf_slot
                               * sizeof(st->bitmap.vertices),
                               sizeof st->bitmap.vertices,
                               st->bitmap.vertices);

   return st->bitmap.vbuf_slot++ * sizeof st->bitmap.vertices;
}



/**
 * Render a glBitmap by drawing a textured quad
 */
static void
draw_bitmap_quad(struct gl_context *ctx, GLint x, GLint y, GLfloat z,
                 GLsizei width, GLsizei height,
                 struct pipe_sampler_view *sv,
                 const GLfloat *color)
{
   struct st_context *st = st_context(ctx);
   struct pipe_context *pipe = st->pipe;
   struct cso_context *cso = st->cso_context;
   struct st_fp_variant *fpv;
   struct st_fp_variant_key key;
   GLuint maxSize;
   GLuint offset;

   memset(&key, 0, sizeof(key));
   key.st = st;
   key.bitmap = GL_TRUE;

   fpv = st_get_fp_variant(st, st->fp, &key);

   /* As an optimization, Mesa's fragment programs will sometimes get the
    * primary color from a statevar/constant rather than a varying variable.
    * when that's the case, we need to ensure that we use the 'color'
    * parameter and not the current attribute color (which may have changed
    * through glRasterPos and state validation.
    * So, we force the proper color here.  Not elegant, but it works.
    */
   {
      GLfloat colorSave[4];
      COPY_4V(colorSave, ctx->Current.Attrib[VERT_ATTRIB_COLOR0]);
      COPY_4V(ctx->Current.Attrib[VERT_ATTRIB_COLOR0], color);
      st_upload_constants(st, fpv->parameters, PIPE_SHADER_FRAGMENT);
      COPY_4V(ctx->Current.Attrib[VERT_ATTRIB_COLOR0], colorSave);
   }


   /* limit checks */
   /* XXX if the bitmap is larger than the max texture size, break
    * it up into chunks.
    */
   maxSize = 1 << (pipe->screen->get_param(pipe->screen,
                                    PIPE_CAP_MAX_TEXTURE_2D_LEVELS) - 1);
   assert(width <= (GLsizei)maxSize);
   assert(height <= (GLsizei)maxSize);

   cso_save_rasterizer(cso);
   cso_save_samplers(cso);
   cso_save_fragment_sampler_views(cso);
   cso_save_viewport(cso);
   cso_save_fragment_shader(cso);
   cso_save_stream_outputs(cso);
   cso_save_vertex_shader(cso);
   cso_save_geometry_shader(cso);
   cso_save_vertex_elements(cso);
   cso_save_vertex_buffers(cso);

   /* rasterizer state: just scissor */
   st->bitmap.rasterizer.scissor = ctx->Scissor.Enabled;
   cso_set_rasterizer(cso, &st->bitmap.rasterizer);

   /* fragment shader state: TEX lookup program */
   cso_set_fragment_shader_handle(cso, fpv->driver_shader);

   /* vertex shader state: position + texcoord pass-through */
   cso_set_vertex_shader_handle(cso, st->bitmap.vs);

   /* geometry shader state: disabled */
   cso_set_geometry_shader_handle(cso, NULL);

   /* user samplers, plus our bitmap sampler */
   {
      struct pipe_sampler_state *samplers[PIPE_MAX_SAMPLERS];
      uint num = MAX2(fpv->bitmap_sampler + 1, st->state.num_samplers);
      uint i;
      for (i = 0; i < st->state.num_samplers; i++) {
         samplers[i] = &st->state.samplers[i];
      }
      samplers[fpv->bitmap_sampler] =
         &st->bitmap.samplers[sv->texture->target != PIPE_TEXTURE_RECT];
      cso_set_samplers(cso, num, (const struct pipe_sampler_state **) samplers);
   }

   /* user textures, plus the bitmap texture */
   {
      struct pipe_sampler_view *sampler_views[PIPE_MAX_SAMPLERS];
      uint num = MAX2(fpv->bitmap_sampler + 1, st->state.num_textures);
      memcpy(sampler_views, st->state.sampler_views, sizeof(sampler_views));
      sampler_views[fpv->bitmap_sampler] = sv;
      cso_set_fragment_sampler_views(cso, num, sampler_views);
   }

   /* viewport state: viewport matching window dims */
   {
      const struct gl_framebuffer *fb = st->ctx->DrawBuffer;
      const GLboolean invert = (st_fb_orientation(fb) == Y_0_TOP);
      const GLfloat width = (GLfloat)fb->Width;
      const GLfloat height = (GLfloat)fb->Height;
      struct pipe_viewport_state vp;
      vp.scale[0] =  0.5f * width;
      vp.scale[1] = height * (invert ? -0.5f : 0.5f);
      vp.scale[2] = 0.5f;
      vp.scale[3] = 1.0f;
      vp.translate[0] = 0.5f * width;
      vp.translate[1] = 0.5f * height;
      vp.translate[2] = 0.5f;
      vp.translate[3] = 0.0f;
      cso_set_viewport(cso, &vp);
   }

   cso_set_vertex_elements(cso, 3, st->velems_util_draw);
   cso_set_stream_outputs(st->cso_context, 0, NULL, 0);

   /* convert Z from [0,1] to [-1,-1] to match viewport Z scale/bias */
   z = z * 2.0f - 1.0f;

   /* draw textured quad */
   offset = setup_bitmap_vertex_data(st,
                                     sv->texture->target != PIPE_TEXTURE_RECT,
                                     x, y, width, height, z, color);

   if (st->bitmap.vbuf) {
      util_draw_vertex_buffer(pipe, st->cso_context, st->bitmap.vbuf, offset,
                              PIPE_PRIM_TRIANGLE_FAN,
                              4,  /* verts */
                              3); /* attribs/vert */
   }

   /* restore state */
   cso_restore_rasterizer(cso);
   cso_restore_samplers(cso);
   cso_restore_fragment_sampler_views(cso);
   cso_restore_viewport(cso);
   cso_restore_fragment_shader(cso);
   cso_restore_vertex_shader(cso);
   cso_restore_geometry_shader(cso);
   cso_restore_vertex_elements(cso);
   cso_restore_vertex_buffers(cso);
   cso_restore_stream_outputs(cso);
}


static void
reset_cache(struct st_context *st)
{
   struct pipe_context *pipe = st->pipe;
   struct bitmap_cache *cache = st->bitmap.cache;

   /*memset(cache->buffer, 0xff, sizeof(cache->buffer));*/
   cache->empty = GL_TRUE;

   cache->xmin = 1000000;
   cache->xmax = -1000000;
   cache->ymin = 1000000;
   cache->ymax = -1000000;

   if (cache->trans) {
      pipe->transfer_destroy(pipe, cache->trans);
      cache->trans = NULL;
   }

   assert(!cache->texture);

   /* allocate a new texture */
   cache->texture = st_texture_create(st, PIPE_TEXTURE_2D,
                                      st->bitmap.tex_format, 0,
                                      BITMAP_CACHE_WIDTH, BITMAP_CACHE_HEIGHT,
                                      1, 1,
				      PIPE_BIND_SAMPLER_VIEW);
}


/** Print bitmap image to stdout (debug) */
static void
print_cache(const struct bitmap_cache *cache)
{
   int i, j, k;

   for (i = 0; i < BITMAP_CACHE_HEIGHT; i++) {
      k = BITMAP_CACHE_WIDTH * (BITMAP_CACHE_HEIGHT - i - 1);
      for (j = 0; j < BITMAP_CACHE_WIDTH; j++) {
         if (cache->buffer[k])
            printf("X");
         else
            printf(" ");
         k++;
      }
      printf("\n");
   }
}


/**
 * Create gallium pipe_transfer object for the bitmap cache.
 */
static void
create_cache_trans(struct st_context *st)
{
   struct pipe_context *pipe = st->pipe;
   struct bitmap_cache *cache = st->bitmap.cache;

   if (cache->trans)
      return;

   /* Map the texture transfer.
    * Subsequent glBitmap calls will write into the texture image.
    */
   cache->trans = pipe_get_transfer(st->pipe, cache->texture, 0, 0,
                                    PIPE_TRANSFER_WRITE, 0, 0,
                                    BITMAP_CACHE_WIDTH,
                                    BITMAP_CACHE_HEIGHT);
   cache->buffer = pipe_transfer_map(pipe, cache->trans);

   /* init image to all 0xff */
   memset(cache->buffer, 0xff, cache->trans->stride * BITMAP_CACHE_HEIGHT);
}


/**
 * If there's anything in the bitmap cache, draw/flush it now.
 */
void
st_flush_bitmap_cache(struct st_context *st)
{
   if (!st->bitmap.cache->empty) {
      struct bitmap_cache *cache = st->bitmap.cache;

      if (st->ctx->DrawBuffer) {
         struct pipe_context *pipe = st->pipe;
         struct pipe_sampler_view *sv;

         assert(cache->xmin <= cache->xmax);
 
/*         printf("flush size %d x %d  at %d, %d\n",
                cache->xmax - cache->xmin,
                cache->ymax - cache->ymin,
                cache->xpos, cache->ypos);
*/

         /* The texture transfer has been mapped until now.
          * So unmap and release the texture transfer before drawing.
          */
         if (cache->trans) {
            if (0)
               print_cache(cache);
            pipe_transfer_unmap(pipe, cache->trans);
            cache->buffer = NULL;

            pipe->transfer_destroy(pipe, cache->trans);
            cache->trans = NULL;
         }

         sv = st_create_texture_sampler_view(st->pipe, cache->texture);
         if (sv) {
            draw_bitmap_quad(st->ctx,
                             cache->xpos,
                             cache->ypos,
                             cache->zpos,
                             BITMAP_CACHE_WIDTH, BITMAP_CACHE_HEIGHT,
                             sv,
                             cache->color);

            pipe_sampler_view_reference(&sv, NULL);
         }
      }

      /* release/free the texture */
      pipe_resource_reference(&cache->texture, NULL);

      reset_cache(st);
   }
}


/**
 * Flush bitmap cache and release vertex buffer.
 */
void
st_flush_bitmap( struct st_context *st )
{
   st_flush_bitmap_cache(st);

   /* Release vertex buffer to avoid synchronous rendering if we were
    * to map it in the next frame.
    */
   pipe_resource_reference(&st->bitmap.vbuf, NULL);
   st->bitmap.vbuf_slot = 0;
}


/**
 * Try to accumulate this glBitmap call in the bitmap cache.
 * \return  GL_TRUE for success, GL_FALSE if bitmap is too large, etc.
 */
static GLboolean
accum_bitmap(struct st_context *st,
             GLint x, GLint y, GLsizei width, GLsizei height,
             const struct gl_pixelstore_attrib *unpack,
             const GLubyte *bitmap )
{
   struct bitmap_cache *cache = st->bitmap.cache;
   int px = -999, py = -999;
   const GLfloat z = st->ctx->Current.RasterPos[2];

   if (width > BITMAP_CACHE_WIDTH ||
       height > BITMAP_CACHE_HEIGHT)
      return GL_FALSE; /* too big to cache */

   if (!cache->empty) {
      px = x - cache->xpos;  /* pos in buffer */
      py = y - cache->ypos;
      if (px < 0 || px + width > BITMAP_CACHE_WIDTH ||
          py < 0 || py + height > BITMAP_CACHE_HEIGHT ||
          !TEST_EQ_4V(st->ctx->Current.RasterColor, cache->color) ||
          ((fabs(z - cache->zpos) > Z_EPSILON))) {
         /* This bitmap would extend beyond cache bounds, or the bitmap
          * color is changing
          * so flush and continue.
          */
         st_flush_bitmap_cache(st);
      }
   }

   if (cache->empty) {
      /* Initialize.  Center bitmap vertically in the buffer. */
      px = 0;
      py = (BITMAP_CACHE_HEIGHT - height) / 2;
      cache->xpos = x;
      cache->ypos = y - py;
      cache->zpos = z;
      cache->empty = GL_FALSE;
      COPY_4FV(cache->color, st->ctx->Current.RasterColor);
   }

   assert(px != -999);
   assert(py != -999);

   if (x < cache->xmin)
      cache->xmin = x;
   if (y < cache->ymin)
      cache->ymin = y;
   if (x + width > cache->xmax)
      cache->xmax = x + width;
   if (y + height > cache->ymax)
      cache->ymax = y + height;

   /* create the transfer if needed */
   create_cache_trans(st);

   unpack_bitmap(st, px, py, width, height, unpack, bitmap,
                 cache->buffer, BITMAP_CACHE_WIDTH);

   return GL_TRUE; /* accumulated */
}



/**
 * Called via ctx->Driver.Bitmap()
 */
static void
st_Bitmap(struct gl_context *ctx, GLint x, GLint y,
          GLsizei width, GLsizei height,
          const struct gl_pixelstore_attrib *unpack, const GLubyte *bitmap )
{
   struct st_context *st = st_context(ctx);
   struct pipe_resource *pt;

   if (width == 0 || height == 0)
      return;

   st_validate_state(st);

   if (!st->bitmap.vs) {
      /* create pass-through vertex shader now */
      const uint semantic_names[] = { TGSI_SEMANTIC_POSITION,
                                      TGSI_SEMANTIC_COLOR,
                                      TGSI_SEMANTIC_GENERIC };
      const uint semantic_indexes[] = { 0, 0, 0 };
      st->bitmap.vs = util_make_vertex_passthrough_shader(st->pipe, 3,
                                                          semantic_names,
                                                          semantic_indexes);
   }

   if (UseBitmapCache && accum_bitmap(st, x, y, width, height, unpack, bitmap))
      return;

   pt = make_bitmap_texture(ctx, width, height, unpack, bitmap);
   if (pt) {
      struct pipe_sampler_view *sv =
         st_create_texture_sampler_view(st->pipe, pt);

      assert(pt->target == PIPE_TEXTURE_2D || pt->target == PIPE_TEXTURE_RECT);

      if (sv) {
         draw_bitmap_quad(ctx, x, y, ctx->Current.RasterPos[2],
                          width, height, sv,
                          st->ctx->Current.RasterColor);

         pipe_sampler_view_reference(&sv, NULL);
      }

      /* release/free the texture */
      pipe_resource_reference(&pt, NULL);
   }
}


/** Per-context init */
void
st_init_bitmap_functions(struct dd_function_table *functions)
{
   functions->Bitmap = st_Bitmap;
}


/** Per-context init */
void
st_init_bitmap(struct st_context *st)
{
   struct pipe_sampler_state *sampler = &st->bitmap.samplers[0];
   struct pipe_context *pipe = st->pipe;
   struct pipe_screen *screen = pipe->screen;

   /* init sampler state once */
   memset(sampler, 0, sizeof(*sampler));
   sampler->wrap_s = PIPE_TEX_WRAP_CLAMP;
   sampler->wrap_t = PIPE_TEX_WRAP_CLAMP;
   sampler->wrap_r = PIPE_TEX_WRAP_CLAMP;
   sampler->min_img_filter = PIPE_TEX_FILTER_NEAREST;
   sampler->min_mip_filter = PIPE_TEX_MIPFILTER_NONE;
   sampler->mag_img_filter = PIPE_TEX_FILTER_NEAREST;
   st->bitmap.samplers[1] = *sampler;
   st->bitmap.samplers[1].normalized_coords = 1;

   /* init baseline rasterizer state once */
   memset(&st->bitmap.rasterizer, 0, sizeof(st->bitmap.rasterizer));
   st->bitmap.rasterizer.gl_rasterization_rules = 1;
   st->bitmap.rasterizer.depth_clip = 1;

   /* find a usable texture format */
   if (screen->is_format_supported(screen, PIPE_FORMAT_I8_UNORM,
                                   PIPE_TEXTURE_2D, 0,
                                   PIPE_BIND_SAMPLER_VIEW)) {
      st->bitmap.tex_format = PIPE_FORMAT_I8_UNORM;
   }
   else if (screen->is_format_supported(screen, PIPE_FORMAT_A8_UNORM,
                                        PIPE_TEXTURE_2D, 0,
                                        PIPE_BIND_SAMPLER_VIEW)) {
      st->bitmap.tex_format = PIPE_FORMAT_A8_UNORM;
   }
   else if (screen->is_format_supported(screen, PIPE_FORMAT_L8_UNORM,
                                        PIPE_TEXTURE_2D, 0,
                                        PIPE_BIND_SAMPLER_VIEW)) {
      st->bitmap.tex_format = PIPE_FORMAT_L8_UNORM;
   }
   else {
      /* XXX support more formats */
      assert(0);
   }

   /* alloc bitmap cache object */
   st->bitmap.cache = ST_CALLOC_STRUCT(bitmap_cache);

   reset_cache(st);
}


/** Per-context tear-down */
void
st_destroy_bitmap(struct st_context *st)
{
   struct pipe_context *pipe = st->pipe;
   struct bitmap_cache *cache = st->bitmap.cache;

   if (st->bitmap.vs) {
      cso_delete_vertex_shader(st->cso_context, st->bitmap.vs);
      st->bitmap.vs = NULL;
   }

   if (st->bitmap.vbuf) {
      pipe_resource_reference(&st->bitmap.vbuf, NULL);
      st->bitmap.vbuf = NULL;
   }

   if (cache) {
      if (cache->trans) {
         pipe_transfer_unmap(pipe, cache->trans);
         pipe->transfer_destroy(pipe, cache->trans);
      }
      pipe_resource_reference(&st->bitmap.cache->texture, NULL);
      free(st->bitmap.cache);
      st->bitmap.cache = NULL;
   }
}

#endif /* FEATURE_drawpix */
