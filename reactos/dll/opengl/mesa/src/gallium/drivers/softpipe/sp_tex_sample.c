/**************************************************************************
 * 
 * Copyright 2007 Tungsten Graphics, Inc., Cedar Park, Texas.
 * All Rights Reserved.
 * Copyright 2008-2010 VMware, Inc.  All rights reserved.
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
 * Texture sampling
 *
 * Authors:
 *   Brian Paul
 *   Keith Whitwell
 */

#include "pipe/p_context.h"
#include "pipe/p_defines.h"
#include "pipe/p_shader_tokens.h"
#include "util/u_math.h"
#include "util/u_memory.h"
#include "sp_quad.h"   /* only for #define QUAD_* tokens */
#include "sp_tex_sample.h"
#include "sp_tex_tile_cache.h"


/** Set to one to help debug texture sampling */
#define DEBUG_TEX 0


/*
 * Return fractional part of 'f'.  Used for computing interpolation weights.
 * Need to be careful with negative values.
 * Note, if this function isn't perfect you'll sometimes see 1-pixel bands
 * of improperly weighted linear-filtered textures.
 * The tests/texwrap.c demo is a good test.
 */
static INLINE float
frac(float f)
{
   return f - floorf(f);
}



/**
 * Linear interpolation macro
 */
static INLINE float
lerp(float a, float v0, float v1)
{
   return v0 + a * (v1 - v0);
}


/**
 * Do 2D/bilinear interpolation of float values.
 * v00, v10, v01 and v11 are typically four texture samples in a square/box.
 * a and b are the horizontal and vertical interpolants.
 * It's important that this function is inlined when compiled with
 * optimization!  If we find that's not true on some systems, convert
 * to a macro.
 */
static INLINE float
lerp_2d(float a, float b,
        float v00, float v10, float v01, float v11)
{
   const float temp0 = lerp(a, v00, v10);
   const float temp1 = lerp(a, v01, v11);
   return lerp(b, temp0, temp1);
}


/**
 * As above, but 3D interpolation of 8 values.
 */
static INLINE float
lerp_3d(float a, float b, float c,
        float v000, float v100, float v010, float v110,
        float v001, float v101, float v011, float v111)
{
   const float temp0 = lerp_2d(a, b, v000, v100, v010, v110);
   const float temp1 = lerp_2d(a, b, v001, v101, v011, v111);
   return lerp(c, temp0, temp1);
}



/**
 * Compute coord % size for repeat wrap modes.
 * Note that if coord is negative, coord % size doesn't give the right
 * value.  To avoid that problem we add a large multiple of the size
 * (rather than using a conditional).
 */
static INLINE int
repeat(int coord, unsigned size)
{
   return (coord + size * 1024) % size;
}


/**
 * Apply texture coord wrapping mode and return integer texture indexes
 * for a vector of four texcoords (S or T or P).
 * \param wrapMode  PIPE_TEX_WRAP_x
 * \param s  the incoming texcoords
 * \param size  the texture image size
 * \param icoord  returns the integer texcoords
 * \return  integer texture index
 */
static void
wrap_nearest_repeat(const float s[4], unsigned size, int icoord[4])
{
   uint ch;
   /* s limited to [0,1) */
   /* i limited to [0,size-1] */
   for (ch = 0; ch < 4; ch++) {
      int i = util_ifloor(s[ch] * size);
      icoord[ch] = repeat(i, size);
   }
}


static void
wrap_nearest_clamp(const float s[4], unsigned size, int icoord[4])
{
   uint ch;
   /* s limited to [0,1] */
   /* i limited to [0,size-1] */
   for (ch = 0; ch < 4; ch++) {
      if (s[ch] <= 0.0F)
         icoord[ch] = 0;
      else if (s[ch] >= 1.0F)
         icoord[ch] = size - 1;
      else
         icoord[ch] = util_ifloor(s[ch] * size);
   }
}


static void
wrap_nearest_clamp_to_edge(const float s[4], unsigned size, int icoord[4])
{
   uint ch;
   /* s limited to [min,max] */
   /* i limited to [0, size-1] */
   const float min = 1.0F / (2.0F * size);
   const float max = 1.0F - min;
   for (ch = 0; ch < 4; ch++) {
      if (s[ch] < min)
         icoord[ch] = 0;
      else if (s[ch] > max)
         icoord[ch] = size - 1;
      else
         icoord[ch] = util_ifloor(s[ch] * size);
   }
}


static void
wrap_nearest_clamp_to_border(const float s[4], unsigned size, int icoord[4])
{
   uint ch;
   /* s limited to [min,max] */
   /* i limited to [-1, size] */
   const float min = -1.0F / (2.0F * size);
   const float max = 1.0F - min;
   for (ch = 0; ch < 4; ch++) {
      if (s[ch] <= min)
         icoord[ch] = -1;
      else if (s[ch] >= max)
         icoord[ch] = size;
      else
         icoord[ch] = util_ifloor(s[ch] * size);
   }
}


static void
wrap_nearest_mirror_repeat(const float s[4], unsigned size, int icoord[4])
{
   uint ch;
   const float min = 1.0F / (2.0F * size);
   const float max = 1.0F - min;
   for (ch = 0; ch < 4; ch++) {
      const int flr = util_ifloor(s[ch]);
      float u = frac(s[ch]);
      if (flr & 1)
         u = 1.0F - u;
      if (u < min)
         icoord[ch] = 0;
      else if (u > max)
         icoord[ch] = size - 1;
      else
         icoord[ch] = util_ifloor(u * size);
   }
}


static void
wrap_nearest_mirror_clamp(const float s[4], unsigned size, int icoord[4])
{
   uint ch;
   for (ch = 0; ch < 4; ch++) {
      /* s limited to [0,1] */
      /* i limited to [0,size-1] */
      const float u = fabsf(s[ch]);
      if (u <= 0.0F)
         icoord[ch] = 0;
      else if (u >= 1.0F)
         icoord[ch] = size - 1;
      else
         icoord[ch] = util_ifloor(u * size);
   }
}


static void
wrap_nearest_mirror_clamp_to_edge(const float s[4], unsigned size,
                                  int icoord[4])
{
   uint ch;
   /* s limited to [min,max] */
   /* i limited to [0, size-1] */
   const float min = 1.0F / (2.0F * size);
   const float max = 1.0F - min;
   for (ch = 0; ch < 4; ch++) {
      const float u = fabsf(s[ch]);
      if (u < min)
         icoord[ch] = 0;
      else if (u > max)
         icoord[ch] = size - 1;
      else
         icoord[ch] = util_ifloor(u * size);
   }
}


static void
wrap_nearest_mirror_clamp_to_border(const float s[4], unsigned size,
                                    int icoord[4])
{
   uint ch;
   /* s limited to [min,max] */
   /* i limited to [0, size-1] */
   const float min = -1.0F / (2.0F * size);
   const float max = 1.0F - min;
   for (ch = 0; ch < 4; ch++) {
      const float u = fabsf(s[ch]);
      if (u < min)
         icoord[ch] = -1;
      else if (u > max)
         icoord[ch] = size;
      else
         icoord[ch] = util_ifloor(u * size);
   }
}


/**
 * Used to compute texel locations for linear sampling for four texcoords.
 * \param wrapMode  PIPE_TEX_WRAP_x
 * \param s  the texcoords
 * \param size  the texture image size
 * \param icoord0  returns first texture indexes
 * \param icoord1  returns second texture indexes (usually icoord0 + 1)
 * \param w  returns blend factor/weight between texture indexes
 * \param icoord  returns the computed integer texture coords
 */
static void
wrap_linear_repeat(const float s[4], unsigned size,
                   int icoord0[4], int icoord1[4], float w[4])
{
   uint ch;
   for (ch = 0; ch < 4; ch++) {
      float u = s[ch] * size - 0.5F;
      icoord0[ch] = repeat(util_ifloor(u), size);
      icoord1[ch] = repeat(icoord0[ch] + 1, size);
      w[ch] = frac(u);
   }
}


static void
wrap_linear_clamp(const float s[4], unsigned size,
                  int icoord0[4], int icoord1[4], float w[4])
{
   uint ch;
   for (ch = 0; ch < 4; ch++) {
      float u = CLAMP(s[ch], 0.0F, 1.0F);
      u = u * size - 0.5f;
      icoord0[ch] = util_ifloor(u);
      icoord1[ch] = icoord0[ch] + 1;
      w[ch] = frac(u);
   }
}


static void
wrap_linear_clamp_to_edge(const float s[4], unsigned size,
                          int icoord0[4], int icoord1[4], float w[4])
{
   uint ch;
   for (ch = 0; ch < 4; ch++) {
      float u = CLAMP(s[ch], 0.0F, 1.0F);
      u = u * size - 0.5f;
      icoord0[ch] = util_ifloor(u);
      icoord1[ch] = icoord0[ch] + 1;
      if (icoord0[ch] < 0)
         icoord0[ch] = 0;
      if (icoord1[ch] >= (int) size)
         icoord1[ch] = size - 1;
      w[ch] = frac(u);
   }
}


static void
wrap_linear_clamp_to_border(const float s[4], unsigned size,
                            int icoord0[4], int icoord1[4], float w[4])
{
   const float min = -1.0F / (2.0F * size);
   const float max = 1.0F - min;
   uint ch;
   for (ch = 0; ch < 4; ch++) {
      float u = CLAMP(s[ch], min, max);
      u = u * size - 0.5f;
      icoord0[ch] = util_ifloor(u);
      icoord1[ch] = icoord0[ch] + 1;
      w[ch] = frac(u);
   }
}


static void
wrap_linear_mirror_repeat(const float s[4], unsigned size,
                          int icoord0[4], int icoord1[4], float w[4])
{
   uint ch;
   for (ch = 0; ch < 4; ch++) {
      const int flr = util_ifloor(s[ch]);
      float u = frac(s[ch]);
      if (flr & 1)
         u = 1.0F - u;
      u = u * size - 0.5F;
      icoord0[ch] = util_ifloor(u);
      icoord1[ch] = icoord0[ch] + 1;
      if (icoord0[ch] < 0)
         icoord0[ch] = 0;
      if (icoord1[ch] >= (int) size)
         icoord1[ch] = size - 1;
      w[ch] = frac(u);
   }
}


static void
wrap_linear_mirror_clamp(const float s[4], unsigned size,
                         int icoord0[4], int icoord1[4], float w[4])
{
   uint ch;
   for (ch = 0; ch < 4; ch++) {
      float u = fabsf(s[ch]);
      if (u >= 1.0F)
         u = (float) size;
      else
         u *= size;
      u -= 0.5F;
      icoord0[ch] = util_ifloor(u);
      icoord1[ch] = icoord0[ch] + 1;
      w[ch] = frac(u);
   }
}


static void
wrap_linear_mirror_clamp_to_edge(const float s[4], unsigned size,
                                 int icoord0[4], int icoord1[4], float w[4])
{
   uint ch;
   for (ch = 0; ch < 4; ch++) {
      float u = fabsf(s[ch]);
      if (u >= 1.0F)
         u = (float) size;
      else
         u *= size;
      u -= 0.5F;
      icoord0[ch] = util_ifloor(u);
      icoord1[ch] = icoord0[ch] + 1;
      if (icoord0[ch] < 0)
         icoord0[ch] = 0;
      if (icoord1[ch] >= (int) size)
         icoord1[ch] = size - 1;
      w[ch] = frac(u);
   }
}


static void
wrap_linear_mirror_clamp_to_border(const float s[4], unsigned size,
                                   int icoord0[4], int icoord1[4], float w[4])
{
   const float min = -1.0F / (2.0F * size);
   const float max = 1.0F - min;
   uint ch;
   for (ch = 0; ch < 4; ch++) {
      float u = fabsf(s[ch]);
      if (u <= min)
         u = min * size;
      else if (u >= max)
         u = max * size;
      else
         u *= size;
      u -= 0.5F;
      icoord0[ch] = util_ifloor(u);
      icoord1[ch] = icoord0[ch] + 1;
      w[ch] = frac(u);
   }
}


/**
 * PIPE_TEX_WRAP_CLAMP for nearest sampling, unnormalized coords.
 */
static void
wrap_nearest_unorm_clamp(const float s[4], unsigned size, int icoord[4])
{
   uint ch;
   for (ch = 0; ch < 4; ch++) {
      int i = util_ifloor(s[ch]);
      icoord[ch]= CLAMP(i, 0, (int) size-1);
   }
}


/**
 * PIPE_TEX_WRAP_CLAMP_TO_BORDER for nearest sampling, unnormalized coords.
 */
static void
wrap_nearest_unorm_clamp_to_border(const float s[4], unsigned size,
                                   int icoord[4])
{
   uint ch;
   for (ch = 0; ch < 4; ch++) {
      icoord[ch]= util_ifloor( CLAMP(s[ch], -0.5F, (float) size + 0.5F) );
   }
}


/**
 * PIPE_TEX_WRAP_CLAMP_TO_EDGE for nearest sampling, unnormalized coords.
 */
static void
wrap_nearest_unorm_clamp_to_edge(const float s[4], unsigned size,
                                 int icoord[4])
{
   uint ch;
   for (ch = 0; ch < 4; ch++) {
      icoord[ch]= util_ifloor( CLAMP(s[ch], 0.5F, (float) size - 0.5F) );
   }
}


/**
 * PIPE_TEX_WRAP_CLAMP for linear sampling, unnormalized coords.
 */
static void
wrap_linear_unorm_clamp(const float s[4], unsigned size,
                        int icoord0[4], int icoord1[4], float w[4])
{
   uint ch;
   for (ch = 0; ch < 4; ch++) {
      /* Not exactly what the spec says, but it matches NVIDIA output */
      float u = CLAMP(s[ch] - 0.5F, 0.0f, (float) size - 1.0f);
      icoord0[ch] = util_ifloor(u);
      icoord1[ch] = icoord0[ch] + 1;
      w[ch] = frac(u);
   }
}


/**
 * PIPE_TEX_WRAP_CLAMP_TO_BORDER for linear sampling, unnormalized coords.
 */
static void
wrap_linear_unorm_clamp_to_border(const float s[4], unsigned size,
                                  int icoord0[4], int icoord1[4], float w[4])
{
   uint ch;
   for (ch = 0; ch < 4; ch++) {
      float u = CLAMP(s[ch], -0.5F, (float) size + 0.5F);
      u -= 0.5F;
      icoord0[ch] = util_ifloor(u);
      icoord1[ch] = icoord0[ch] + 1;
      if (icoord1[ch] > (int) size - 1)
         icoord1[ch] = size - 1;
      w[ch] = frac(u);
   }
}


/**
 * PIPE_TEX_WRAP_CLAMP_TO_EDGE for linear sampling, unnormalized coords.
 */
static void
wrap_linear_unorm_clamp_to_edge(const float s[4], unsigned size,
                                int icoord0[4], int icoord1[4], float w[4])
{
   uint ch;
   for (ch = 0; ch < 4; ch++) {
      float u = CLAMP(s[ch], +0.5F, (float) size - 0.5F);
      u -= 0.5F;
      icoord0[ch] = util_ifloor(u);
      icoord1[ch] = icoord0[ch] + 1;
      if (icoord1[ch] > (int) size - 1)
         icoord1[ch] = size - 1;
      w[ch] = frac(u);
   }
}


/**
 * Do coordinate to array index conversion.  For array textures.
 */
static INLINE void
wrap_array_layer(const float coord[4], unsigned size, int layer[4])
{
   uint ch;
   for (ch = 0; ch < 4; ch++) {
      int c = util_ifloor(coord[ch] + 0.5F);
      layer[ch] = CLAMP(c, 0, size - 1);
   }
}


/**
 * Examine the quad's texture coordinates to compute the partial
 * derivatives w.r.t X and Y, then compute lambda (level of detail).
 */
static float
compute_lambda_1d(const struct sp_sampler_variant *samp,
                  const float s[QUAD_SIZE],
                  const float t[QUAD_SIZE],
                  const float p[QUAD_SIZE])
{
   const struct pipe_resource *texture = samp->view->texture;
   float dsdx = fabsf(s[QUAD_BOTTOM_RIGHT] - s[QUAD_BOTTOM_LEFT]);
   float dsdy = fabsf(s[QUAD_TOP_LEFT]     - s[QUAD_BOTTOM_LEFT]);
   float rho = MAX2(dsdx, dsdy) * u_minify(texture->width0, samp->view->u.tex.first_level);

   return util_fast_log2(rho);
}


static float
compute_lambda_2d(const struct sp_sampler_variant *samp,
                  const float s[QUAD_SIZE],
                  const float t[QUAD_SIZE],
                  const float p[QUAD_SIZE])
{
   const struct pipe_resource *texture = samp->view->texture;
   float dsdx = fabsf(s[QUAD_BOTTOM_RIGHT] - s[QUAD_BOTTOM_LEFT]);
   float dsdy = fabsf(s[QUAD_TOP_LEFT]     - s[QUAD_BOTTOM_LEFT]);
   float dtdx = fabsf(t[QUAD_BOTTOM_RIGHT] - t[QUAD_BOTTOM_LEFT]);
   float dtdy = fabsf(t[QUAD_TOP_LEFT]     - t[QUAD_BOTTOM_LEFT]);
   float maxx = MAX2(dsdx, dsdy) * u_minify(texture->width0, samp->view->u.tex.first_level);
   float maxy = MAX2(dtdx, dtdy) * u_minify(texture->height0, samp->view->u.tex.first_level);
   float rho  = MAX2(maxx, maxy);

   return util_fast_log2(rho);
}


static float
compute_lambda_3d(const struct sp_sampler_variant *samp,
                  const float s[QUAD_SIZE],
                  const float t[QUAD_SIZE],
                  const float p[QUAD_SIZE])
{
   const struct pipe_resource *texture = samp->view->texture;
   float dsdx = fabsf(s[QUAD_BOTTOM_RIGHT] - s[QUAD_BOTTOM_LEFT]);
   float dsdy = fabsf(s[QUAD_TOP_LEFT]     - s[QUAD_BOTTOM_LEFT]);
   float dtdx = fabsf(t[QUAD_BOTTOM_RIGHT] - t[QUAD_BOTTOM_LEFT]);
   float dtdy = fabsf(t[QUAD_TOP_LEFT]     - t[QUAD_BOTTOM_LEFT]);
   float dpdx = fabsf(p[QUAD_BOTTOM_RIGHT] - p[QUAD_BOTTOM_LEFT]);
   float dpdy = fabsf(p[QUAD_TOP_LEFT]     - p[QUAD_BOTTOM_LEFT]);
   float maxx = MAX2(dsdx, dsdy) * u_minify(texture->width0, samp->view->u.tex.first_level);
   float maxy = MAX2(dtdx, dtdy) * u_minify(texture->height0, samp->view->u.tex.first_level);
   float maxz = MAX2(dpdx, dpdy) * u_minify(texture->depth0, samp->view->u.tex.first_level);
   float rho;

   rho = MAX2(maxx, maxy);
   rho = MAX2(rho, maxz);

   return util_fast_log2(rho);
}


/**
 * Compute lambda for a vertex texture sampler.
 * Since there aren't derivatives to use, just return 0.
 */
static float
compute_lambda_vert(const struct sp_sampler_variant *samp,
                    const float s[QUAD_SIZE],
                    const float t[QUAD_SIZE],
                    const float p[QUAD_SIZE])
{
   return 0.0f;
}



/**
 * Get a texel from a texture, using the texture tile cache.
 *
 * \param addr  the template tex address containing cube, z, face info.
 * \param x  the x coord of texel within 2D image
 * \param y  the y coord of texel within 2D image
 * \param rgba  the quad to put the texel/color into
 *
 * XXX maybe move this into sp_tex_tile_cache.c and merge with the
 * sp_get_cached_tile_tex() function.  Also, get 4 texels instead of 1...
 */




static INLINE const float *
get_texel_2d_no_border(const struct sp_sampler_variant *samp,
		       union tex_tile_address addr, int x, int y)
{
   const struct softpipe_tex_cached_tile *tile;

   addr.bits.x = x / TILE_SIZE;
   addr.bits.y = y / TILE_SIZE;
   y %= TILE_SIZE;
   x %= TILE_SIZE;

   tile = sp_get_cached_tile_tex(samp->cache, addr);

   return &tile->data.color[y][x][0];
}


static INLINE const float *
get_texel_2d(const struct sp_sampler_variant *samp,
	     union tex_tile_address addr, int x, int y)
{
   const struct pipe_resource *texture = samp->view->texture;
   unsigned level = addr.bits.level;

   if (x < 0 || x >= (int) u_minify(texture->width0, level) ||
       y < 0 || y >= (int) u_minify(texture->height0, level)) {
      return samp->sampler->border_color.f;
   }
   else {
      return get_texel_2d_no_border( samp, addr, x, y );
   }
}


/* Gather a quad of adjacent texels within a tile:
 */
static INLINE void
get_texel_quad_2d_no_border_single_tile(const struct sp_sampler_variant *samp,
					union tex_tile_address addr, 
					unsigned x, unsigned y, 
					const float *out[4])
{
   const struct softpipe_tex_cached_tile *tile;

   addr.bits.x = x / TILE_SIZE;
   addr.bits.y = y / TILE_SIZE;
   y %= TILE_SIZE;
   x %= TILE_SIZE;

   tile = sp_get_cached_tile_tex(samp->cache, addr);
      
   out[0] = &tile->data.color[y  ][x  ][0];
   out[1] = &tile->data.color[y  ][x+1][0];
   out[2] = &tile->data.color[y+1][x  ][0];
   out[3] = &tile->data.color[y+1][x+1][0];
}


/* Gather a quad of potentially non-adjacent texels:
 */
static INLINE void
get_texel_quad_2d_no_border(const struct sp_sampler_variant *samp,
			    union tex_tile_address addr,
			    int x0, int y0, 
			    int x1, int y1,
			    const float *out[4])
{
   out[0] = get_texel_2d_no_border( samp, addr, x0, y0 );
   out[1] = get_texel_2d_no_border( samp, addr, x1, y0 );
   out[2] = get_texel_2d_no_border( samp, addr, x0, y1 );
   out[3] = get_texel_2d_no_border( samp, addr, x1, y1 );
}

/* Can involve a lot of unnecessary checks for border color:
 */
static INLINE void
get_texel_quad_2d(const struct sp_sampler_variant *samp,
		  union tex_tile_address addr,
		  int x0, int y0, 
		  int x1, int y1,
		  const float *out[4])
{
   out[0] = get_texel_2d( samp, addr, x0, y0 );
   out[1] = get_texel_2d( samp, addr, x1, y0 );
   out[3] = get_texel_2d( samp, addr, x1, y1 );
   out[2] = get_texel_2d( samp, addr, x0, y1 );
}



/* 3d variants:
 */
static INLINE const float *
get_texel_3d_no_border(const struct sp_sampler_variant *samp,
                       union tex_tile_address addr, int x, int y, int z)
{
   const struct softpipe_tex_cached_tile *tile;

   addr.bits.x = x / TILE_SIZE;
   addr.bits.y = y / TILE_SIZE;
   addr.bits.z = z;
   y %= TILE_SIZE;
   x %= TILE_SIZE;

   tile = sp_get_cached_tile_tex(samp->cache, addr);

   return &tile->data.color[y][x][0];
}


static INLINE const float *
get_texel_3d(const struct sp_sampler_variant *samp,
	     union tex_tile_address addr, int x, int y, int z)
{
   const struct pipe_resource *texture = samp->view->texture;
   unsigned level = addr.bits.level;

   if (x < 0 || x >= (int) u_minify(texture->width0, level) ||
       y < 0 || y >= (int) u_minify(texture->height0, level) ||
       z < 0 || z >= (int) u_minify(texture->depth0, level)) {
      return samp->sampler->border_color.f;
   }
   else {
      return get_texel_3d_no_border( samp, addr, x, y, z );
   }
}


/* Get texel pointer for 1D array texture */
static INLINE const float *
get_texel_1d_array(const struct sp_sampler_variant *samp,
                   union tex_tile_address addr, int x, int y)
{
   const struct pipe_resource *texture = samp->view->texture;
   unsigned level = addr.bits.level;

   if (x < 0 || x >= (int) u_minify(texture->width0, level)) {
      return samp->sampler->border_color.f;
   }
   else {
      return get_texel_2d_no_border(samp, addr, x, y);
   }
}


/* Get texel pointer for 2D array texture */
static INLINE const float *
get_texel_2d_array(const struct sp_sampler_variant *samp,
                   union tex_tile_address addr, int x, int y, int layer)
{
   const struct pipe_resource *texture = samp->view->texture;
   unsigned level = addr.bits.level;

   assert(layer < texture->array_size);

   if (x < 0 || x >= (int) u_minify(texture->width0, level) ||
       y < 0 || y >= (int) u_minify(texture->height0, level)) {
      return samp->sampler->border_color.f;
   }
   else {
      return get_texel_3d_no_border(samp, addr, x, y, layer);
   }
}


/**
 * Given the logbase2 of a mipmap's base level size and a mipmap level,
 * return the size (in texels) of that mipmap level.
 * For example, if level[0].width = 256 then base_pot will be 8.
 * If level = 2, then we'll return 64 (the width at level=2).
 * Return 1 if level > base_pot.
 */
static INLINE unsigned
pot_level_size(unsigned base_pot, unsigned level)
{
   return (base_pot >= level) ? (1 << (base_pot - level)) : 1;
}


static void
print_sample(const char *function, float rgba[NUM_CHANNELS][QUAD_SIZE])
{
   debug_printf("%s %g %g %g %g, %g %g %g %g, %g %g %g %g, %g %g %g %g\n",
                function,
                rgba[0][0], rgba[1][0], rgba[2][0], rgba[3][0],
                rgba[0][1], rgba[1][1], rgba[2][1], rgba[3][1],
                rgba[0][2], rgba[1][2], rgba[2][2], rgba[3][2],
                rgba[0][3], rgba[1][3], rgba[2][3], rgba[3][3]);
}


/* Some image-filter fastpaths:
 */
static INLINE void
img_filter_2d_linear_repeat_POT(struct tgsi_sampler *tgsi_sampler,
                                const float s[QUAD_SIZE],
                                const float t[QUAD_SIZE],
                                const float p[QUAD_SIZE],
                                const float c0[QUAD_SIZE],
                                enum tgsi_sampler_control control,
                                float rgba[NUM_CHANNELS][QUAD_SIZE])
{
   const struct sp_sampler_variant *samp = sp_sampler_variant(tgsi_sampler);
   unsigned  j;
   unsigned level = samp->level;
   unsigned xpot = pot_level_size(samp->xpot, level);
   unsigned ypot = pot_level_size(samp->ypot, level);
   unsigned xmax = (xpot - 1) & (TILE_SIZE - 1); /* MIN2(TILE_SIZE, xpot) - 1; */
   unsigned ymax = (ypot - 1) & (TILE_SIZE - 1); /* MIN2(TILE_SIZE, ypot) - 1; */
   union tex_tile_address addr;

   addr.value = 0;
   addr.bits.level = samp->level;

   for (j = 0; j < QUAD_SIZE; j++) {
      int c;

      float u = s[j] * xpot - 0.5F;
      float v = t[j] * ypot - 0.5F;

      int uflr = util_ifloor(u);
      int vflr = util_ifloor(v);

      float xw = u - (float)uflr;
      float yw = v - (float)vflr;

      int x0 = uflr & (xpot - 1);
      int y0 = vflr & (ypot - 1);

      const float *tx[4];      
      
      /* Can we fetch all four at once:
       */
      if (x0 < xmax && y0 < ymax) {
         get_texel_quad_2d_no_border_single_tile(samp, addr, x0, y0, tx);
      }
      else {
         unsigned x1 = (x0 + 1) & (xpot - 1);
         unsigned y1 = (y0 + 1) & (ypot - 1);
         get_texel_quad_2d_no_border(samp, addr, x0, y0, x1, y1, tx);
      }

      /* interpolate R, G, B, A */
      for (c = 0; c < 4; c++) {
         rgba[c][j] = lerp_2d(xw, yw, 
                              tx[0][c], tx[1][c], 
                              tx[2][c], tx[3][c]);
      }
   }

   if (DEBUG_TEX) {
      print_sample(__FUNCTION__, rgba);
   }
}


static INLINE void
img_filter_2d_nearest_repeat_POT(struct tgsi_sampler *tgsi_sampler,
                                 const float s[QUAD_SIZE],
                                 const float t[QUAD_SIZE],
                                 const float p[QUAD_SIZE],
                                 const float c0[QUAD_SIZE],
                                 enum tgsi_sampler_control control,
                                 float rgba[NUM_CHANNELS][QUAD_SIZE])
{
   const struct sp_sampler_variant *samp = sp_sampler_variant(tgsi_sampler);
   unsigned  j;
   unsigned level = samp->level;
   unsigned xpot = pot_level_size(samp->xpot, level);
   unsigned ypot = pot_level_size(samp->ypot, level);
   union tex_tile_address addr;

   addr.value = 0;
   addr.bits.level = samp->level;

   for (j = 0; j < QUAD_SIZE; j++) {
      int c;

      float u = s[j] * xpot;
      float v = t[j] * ypot;

      int uflr = util_ifloor(u);
      int vflr = util_ifloor(v);

      int x0 = uflr & (xpot - 1);
      int y0 = vflr & (ypot - 1);

      const float *out = get_texel_2d_no_border(samp, addr, x0, y0);

      for (c = 0; c < 4; c++) {
         rgba[c][j] = out[c];
      }
   }

   if (DEBUG_TEX) {
      print_sample(__FUNCTION__, rgba);
   }
}


static INLINE void
img_filter_2d_nearest_clamp_POT(struct tgsi_sampler *tgsi_sampler,
                                const float s[QUAD_SIZE],
                                const float t[QUAD_SIZE],
                                const float p[QUAD_SIZE],
                                const float c0[QUAD_SIZE],
                                enum tgsi_sampler_control control,
                                float rgba[NUM_CHANNELS][QUAD_SIZE])
{
   const struct sp_sampler_variant *samp = sp_sampler_variant(tgsi_sampler);
   unsigned  j;
   unsigned level = samp->level;
   unsigned xpot = pot_level_size(samp->xpot, level);
   unsigned ypot = pot_level_size(samp->ypot, level);
   union tex_tile_address addr;

   addr.value = 0;
   addr.bits.level = samp->level;

   for (j = 0; j < QUAD_SIZE; j++) {
      int c;

      float u = s[j] * xpot;
      float v = t[j] * ypot;

      int x0, y0;
      const float *out;

      x0 = util_ifloor(u);
      if (x0 < 0) 
         x0 = 0;
      else if (x0 > xpot - 1)
         x0 = xpot - 1;

      y0 = util_ifloor(v);
      if (y0 < 0) 
         y0 = 0;
      else if (y0 > ypot - 1)
         y0 = ypot - 1;
      
      out = get_texel_2d_no_border(samp, addr, x0, y0);

      for (c = 0; c < 4; c++) {
         rgba[c][j] = out[c];
      }
   }

   if (DEBUG_TEX) {
      print_sample(__FUNCTION__, rgba);
   }
}


static void
img_filter_1d_nearest(struct tgsi_sampler *tgsi_sampler,
                        const float s[QUAD_SIZE],
                        const float t[QUAD_SIZE],
                        const float p[QUAD_SIZE],
                        const float c0[QUAD_SIZE],
                        enum tgsi_sampler_control control,
                        float rgba[NUM_CHANNELS][QUAD_SIZE])
{
   const struct sp_sampler_variant *samp = sp_sampler_variant(tgsi_sampler);
   const struct pipe_resource *texture = samp->view->texture;
   unsigned level0, j;
   int width;
   int x[4];
   union tex_tile_address addr;

   level0 = samp->level;
   width = u_minify(texture->width0, level0);

   assert(width > 0);

   addr.value = 0;
   addr.bits.level = samp->level;

   samp->nearest_texcoord_s(s, width, x);

   for (j = 0; j < QUAD_SIZE; j++) {
      const float *out = get_texel_2d(samp, addr, x[j], 0);
      int c;
      for (c = 0; c < 4; c++) {
         rgba[c][j] = out[c];
      }
   }

   if (DEBUG_TEX) {
      print_sample(__FUNCTION__, rgba);
   }
}


static void
img_filter_1d_array_nearest(struct tgsi_sampler *tgsi_sampler,
                            const float s[QUAD_SIZE],
                            const float t[QUAD_SIZE],
                            const float p[QUAD_SIZE],
                            const float c0[QUAD_SIZE],
                            enum tgsi_sampler_control control,
                            float rgba[NUM_CHANNELS][QUAD_SIZE])
{
   const struct sp_sampler_variant *samp = sp_sampler_variant(tgsi_sampler);
   const struct pipe_resource *texture = samp->view->texture;
   unsigned level0, j;
   int width;
   int x[4], layer[4];
   union tex_tile_address addr;

   level0 = samp->level;
   width = u_minify(texture->width0, level0);

   assert(width > 0);

   addr.value = 0;
   addr.bits.level = samp->level;

   samp->nearest_texcoord_s(s, width, x);
   wrap_array_layer(t, texture->array_size, layer);

   for (j = 0; j < QUAD_SIZE; j++) {
      const float *out = get_texel_1d_array(samp, addr, x[j], layer[j]);
      int c;
      for (c = 0; c < 4; c++) {
         rgba[c][j] = out[c];
      }
   }

   if (DEBUG_TEX) {
      print_sample(__FUNCTION__, rgba);
   }
}


static void
img_filter_2d_nearest(struct tgsi_sampler *tgsi_sampler,
                      const float s[QUAD_SIZE],
                      const float t[QUAD_SIZE],
                      const float p[QUAD_SIZE],
                      const float c0[QUAD_SIZE],
                      enum tgsi_sampler_control control,
                      float rgba[NUM_CHANNELS][QUAD_SIZE])
{
   const struct sp_sampler_variant *samp = sp_sampler_variant(tgsi_sampler);
   const struct pipe_resource *texture = samp->view->texture;
   unsigned level0, j;
   int width, height;
   int x[4], y[4];
   union tex_tile_address addr;


   level0 = samp->level;
   width = u_minify(texture->width0, level0);
   height = u_minify(texture->height0, level0);

   assert(width > 0);
   assert(height > 0);
 
   addr.value = 0;
   addr.bits.level = samp->level;

   samp->nearest_texcoord_s(s, width, x);
   samp->nearest_texcoord_t(t, height, y);

   for (j = 0; j < QUAD_SIZE; j++) {
      const float *out = get_texel_2d(samp, addr, x[j], y[j]);
      int c;
      for (c = 0; c < 4; c++) {
         rgba[c][j] = out[c];
      }
   }

   if (DEBUG_TEX) {
      print_sample(__FUNCTION__, rgba);
   }
}


static void
img_filter_2d_array_nearest(struct tgsi_sampler *tgsi_sampler,
                            const float s[QUAD_SIZE],
                            const float t[QUAD_SIZE],
                            const float p[QUAD_SIZE],
                            const float c0[QUAD_SIZE],
                            enum tgsi_sampler_control control,
                            float rgba[NUM_CHANNELS][QUAD_SIZE])
{
   const struct sp_sampler_variant *samp = sp_sampler_variant(tgsi_sampler);
   const struct pipe_resource *texture = samp->view->texture;
   unsigned level0, j;
   int width, height;
   int x[4], y[4], layer[4];
   union tex_tile_address addr;

   level0 = samp->level;
   width = u_minify(texture->width0, level0);
   height = u_minify(texture->height0, level0);

   assert(width > 0);
   assert(height > 0);
 
   addr.value = 0;
   addr.bits.level = samp->level;

   samp->nearest_texcoord_s(s, width, x);
   samp->nearest_texcoord_t(t, height, y);
   wrap_array_layer(p, texture->array_size, layer);

   for (j = 0; j < QUAD_SIZE; j++) {
      const float *out = get_texel_2d_array(samp, addr, x[j], y[j], layer[j]);
      int c;
      for (c = 0; c < 4; c++) {
         rgba[c][j] = out[c];
      }
   }

   if (DEBUG_TEX) {
      print_sample(__FUNCTION__, rgba);
   }
}


static INLINE union tex_tile_address
face(union tex_tile_address addr, unsigned face )
{
   addr.bits.face = face;
   return addr;
}


static void
img_filter_cube_nearest(struct tgsi_sampler *tgsi_sampler,
                        const float s[QUAD_SIZE],
                        const float t[QUAD_SIZE],
                        const float p[QUAD_SIZE],
                        const float c0[QUAD_SIZE],
                        enum tgsi_sampler_control control,
                        float rgba[NUM_CHANNELS][QUAD_SIZE])
{
   const struct sp_sampler_variant *samp = sp_sampler_variant(tgsi_sampler);
   const struct pipe_resource *texture = samp->view->texture;
   const unsigned *faces = samp->faces; /* zero when not cube-mapping */
   unsigned level0, j;
   int width, height;
   int x[4], y[4];
   union tex_tile_address addr;

   level0 = samp->level;
   width = u_minify(texture->width0, level0);
   height = u_minify(texture->height0, level0);

   assert(width > 0);
   assert(height > 0);
 
   addr.value = 0;
   addr.bits.level = samp->level;

   samp->nearest_texcoord_s(s, width, x);
   samp->nearest_texcoord_t(t, height, y);

   for (j = 0; j < QUAD_SIZE; j++) {
      const float *out = get_texel_2d(samp, face(addr, faces[j]), x[j], y[j]);
      int c;
      for (c = 0; c < 4; c++) {
         rgba[c][j] = out[c];
      }
   }

   if (DEBUG_TEX) {
      print_sample(__FUNCTION__, rgba);
   }
}


static void
img_filter_3d_nearest(struct tgsi_sampler *tgsi_sampler,
                      const float s[QUAD_SIZE],
                      const float t[QUAD_SIZE],
                      const float p[QUAD_SIZE],
                      const float c0[QUAD_SIZE],
                      enum tgsi_sampler_control control,
                      float rgba[NUM_CHANNELS][QUAD_SIZE])
{
   const struct sp_sampler_variant *samp = sp_sampler_variant(tgsi_sampler);
   const struct pipe_resource *texture = samp->view->texture;
   unsigned level0, j;
   int width, height, depth;
   int x[4], y[4], z[4];
   union tex_tile_address addr;

   level0 = samp->level;
   width = u_minify(texture->width0, level0);
   height = u_minify(texture->height0, level0);
   depth = u_minify(texture->depth0, level0);

   assert(width > 0);
   assert(height > 0);
   assert(depth > 0);

   samp->nearest_texcoord_s(s, width,  x);
   samp->nearest_texcoord_t(t, height, y);
   samp->nearest_texcoord_p(p, depth,  z);

   addr.value = 0;
   addr.bits.level = samp->level;

   for (j = 0; j < QUAD_SIZE; j++) {
      const float *out = get_texel_3d(samp, addr, x[j], y[j], z[j]);
      int c;
      for (c = 0; c < 4; c++) {
         rgba[c][j] = out[c];
      }      
   }
}


static void
img_filter_1d_linear(struct tgsi_sampler *tgsi_sampler,
                     const float s[QUAD_SIZE],
                     const float t[QUAD_SIZE],
                     const float p[QUAD_SIZE],
                     const float c0[QUAD_SIZE],
                     enum tgsi_sampler_control control,
                     float rgba[NUM_CHANNELS][QUAD_SIZE])
{
   const struct sp_sampler_variant *samp = sp_sampler_variant(tgsi_sampler);
   const struct pipe_resource *texture = samp->view->texture;
   unsigned level0, j;
   int width;
   int x0[4], x1[4];
   float xw[4]; /* weights */
   union tex_tile_address addr;

   level0 = samp->level;
   width = u_minify(texture->width0, level0);

   assert(width > 0);

   addr.value = 0;
   addr.bits.level = samp->level;

   samp->linear_texcoord_s(s, width, x0, x1, xw);

   for (j = 0; j < QUAD_SIZE; j++) {
      const float *tx0 = get_texel_2d(samp, addr, x0[j], 0);
      const float *tx1 = get_texel_2d(samp, addr, x1[j], 0);
      int c;

      /* interpolate R, G, B, A */
      for (c = 0; c < 4; c++) {
         rgba[c][j] = lerp(xw[j], tx0[c], tx1[c]);
      }
   }
}


static void
img_filter_1d_array_linear(struct tgsi_sampler *tgsi_sampler,
                           const float s[QUAD_SIZE],
                           const float t[QUAD_SIZE],
                           const float p[QUAD_SIZE],
                           const float c0[QUAD_SIZE],
                           enum tgsi_sampler_control control,
                           float rgba[NUM_CHANNELS][QUAD_SIZE])
{
   const struct sp_sampler_variant *samp = sp_sampler_variant(tgsi_sampler);
   const struct pipe_resource *texture = samp->view->texture;
   unsigned level0, j;
   int width;
   int x0[4], x1[4], layer[4];
   float xw[4]; /* weights */
   union tex_tile_address addr;

   level0 = samp->level;
   width = u_minify(texture->width0, level0);

   assert(width > 0);

   addr.value = 0;
   addr.bits.level = samp->level;

   samp->linear_texcoord_s(s, width, x0, x1, xw);
   wrap_array_layer(t, texture->array_size, layer);

   for (j = 0; j < QUAD_SIZE; j++) {
      const float *tx0 = get_texel_1d_array(samp, addr, x0[j], layer[j]);
      const float *tx1 = get_texel_1d_array(samp, addr, x1[j], layer[j]);
      int c;

      /* interpolate R, G, B, A */
      for (c = 0; c < 4; c++) {
         rgba[c][j] = lerp(xw[j], tx0[c], tx1[c]);
      }
   }
}


static void
img_filter_2d_linear(struct tgsi_sampler *tgsi_sampler,
                     const float s[QUAD_SIZE],
                     const float t[QUAD_SIZE],
                     const float p[QUAD_SIZE],
                     const float c0[QUAD_SIZE],
                     enum tgsi_sampler_control control,
                     float rgba[NUM_CHANNELS][QUAD_SIZE])
{
   const struct sp_sampler_variant *samp = sp_sampler_variant(tgsi_sampler);
   const struct pipe_resource *texture = samp->view->texture;
   unsigned level0, j;
   int width, height;
   int x0[4], y0[4], x1[4], y1[4];
   float xw[4], yw[4]; /* weights */
   union tex_tile_address addr;

   level0 = samp->level;
   width = u_minify(texture->width0, level0);
   height = u_minify(texture->height0, level0);

   assert(width > 0);
   assert(height > 0);

   addr.value = 0;
   addr.bits.level = samp->level;

   samp->linear_texcoord_s(s, width,  x0, x1, xw);
   samp->linear_texcoord_t(t, height, y0, y1, yw);

   for (j = 0; j < QUAD_SIZE; j++) {
      const float *tx0 = get_texel_2d(samp, addr, x0[j], y0[j]);
      const float *tx1 = get_texel_2d(samp, addr, x1[j], y0[j]);
      const float *tx2 = get_texel_2d(samp, addr, x0[j], y1[j]);
      const float *tx3 = get_texel_2d(samp, addr, x1[j], y1[j]);
      int c;

      /* interpolate R, G, B, A */
      for (c = 0; c < 4; c++) {
         rgba[c][j] = lerp_2d(xw[j], yw[j],
                              tx0[c], tx1[c],
                              tx2[c], tx3[c]);
      }
   }
}


static void
img_filter_2d_array_linear(struct tgsi_sampler *tgsi_sampler,
                           const float s[QUAD_SIZE],
                           const float t[QUAD_SIZE],
                           const float p[QUAD_SIZE],
                           const float c0[QUAD_SIZE],
                           enum tgsi_sampler_control control,
                           float rgba[NUM_CHANNELS][QUAD_SIZE])
{
   const struct sp_sampler_variant *samp = sp_sampler_variant(tgsi_sampler);
   const struct pipe_resource *texture = samp->view->texture;
   unsigned level0, j;
   int width, height;
   int x0[4], y0[4], x1[4], y1[4], layer[4];
   float xw[4], yw[4]; /* weights */
   union tex_tile_address addr;

   level0 = samp->level;
   width = u_minify(texture->width0, level0);
   height = u_minify(texture->height0, level0);

   assert(width > 0);
   assert(height > 0);

   addr.value = 0;
   addr.bits.level = samp->level;

   samp->linear_texcoord_s(s, width,  x0, x1, xw);
   samp->linear_texcoord_t(t, height, y0, y1, yw);
   wrap_array_layer(p, texture->array_size, layer);

   for (j = 0; j < QUAD_SIZE; j++) {
      const float *tx0 = get_texel_2d_array(samp, addr, x0[j], y0[j], layer[j]);
      const float *tx1 = get_texel_2d_array(samp, addr, x1[j], y0[j], layer[j]);
      const float *tx2 = get_texel_2d_array(samp, addr, x0[j], y1[j], layer[j]);
      const float *tx3 = get_texel_2d_array(samp, addr, x1[j], y1[j], layer[j]);
      int c;

      /* interpolate R, G, B, A */
      for (c = 0; c < 4; c++) {
         rgba[c][j] = lerp_2d(xw[j], yw[j],
                              tx0[c], tx1[c],
                              tx2[c], tx3[c]);
      }
   }
}


static void
img_filter_cube_linear(struct tgsi_sampler *tgsi_sampler,
                       const float s[QUAD_SIZE],
                       const float t[QUAD_SIZE],
                       const float p[QUAD_SIZE],
                       const float c0[QUAD_SIZE],
                       enum tgsi_sampler_control control,
                       float rgba[NUM_CHANNELS][QUAD_SIZE])
{
   const struct sp_sampler_variant *samp = sp_sampler_variant(tgsi_sampler);
   const struct pipe_resource *texture = samp->view->texture;
   const unsigned *faces = samp->faces; /* zero when not cube-mapping */
   unsigned level0, j;
   int width, height;
   int x0[4], y0[4], x1[4], y1[4];
   float xw[4], yw[4]; /* weights */
   union tex_tile_address addr;

   level0 = samp->level;
   width = u_minify(texture->width0, level0);
   height = u_minify(texture->height0, level0);

   assert(width > 0);
   assert(height > 0);

   addr.value = 0;
   addr.bits.level = samp->level;

   samp->linear_texcoord_s(s, width,  x0, x1, xw);
   samp->linear_texcoord_t(t, height, y0, y1, yw);

   for (j = 0; j < QUAD_SIZE; j++) {
      union tex_tile_address addrj = face(addr, faces[j]);
      const float *tx0 = get_texel_2d(samp, addrj, x0[j], y0[j]);
      const float *tx1 = get_texel_2d(samp, addrj, x1[j], y0[j]);
      const float *tx2 = get_texel_2d(samp, addrj, x0[j], y1[j]);
      const float *tx3 = get_texel_2d(samp, addrj, x1[j], y1[j]);
      int c;

      /* interpolate R, G, B, A */
      for (c = 0; c < 4; c++) {
         rgba[c][j] = lerp_2d(xw[j], yw[j],
                              tx0[c], tx1[c],
                              tx2[c], tx3[c]);
      }
   }
}


static void
img_filter_3d_linear(struct tgsi_sampler *tgsi_sampler,
                     const float s[QUAD_SIZE],
                     const float t[QUAD_SIZE],
                     const float p[QUAD_SIZE],
                     const float c0[QUAD_SIZE],
                     enum tgsi_sampler_control control,
                     float rgba[NUM_CHANNELS][QUAD_SIZE])
{
   const struct sp_sampler_variant *samp = sp_sampler_variant(tgsi_sampler);
   const struct pipe_resource *texture = samp->view->texture;
   unsigned level0, j;
   int width, height, depth;
   int x0[4], x1[4], y0[4], y1[4], z0[4], z1[4];
   float xw[4], yw[4], zw[4]; /* interpolation weights */
   union tex_tile_address addr;

   level0 = samp->level;
   width = u_minify(texture->width0, level0);
   height = u_minify(texture->height0, level0);
   depth = u_minify(texture->depth0, level0);

   addr.value = 0;
   addr.bits.level = level0;

   assert(width > 0);
   assert(height > 0);
   assert(depth > 0);

   samp->linear_texcoord_s(s, width,  x0, x1, xw);
   samp->linear_texcoord_t(t, height, y0, y1, yw);
   samp->linear_texcoord_p(p, depth,  z0, z1, zw);

   for (j = 0; j < QUAD_SIZE; j++) {
      int c;

      const float *tx00 = get_texel_3d(samp, addr, x0[j], y0[j], z0[j]);
      const float *tx01 = get_texel_3d(samp, addr, x1[j], y0[j], z0[j]);
      const float *tx02 = get_texel_3d(samp, addr, x0[j], y1[j], z0[j]);
      const float *tx03 = get_texel_3d(samp, addr, x1[j], y1[j], z0[j]);
      
      const float *tx10 = get_texel_3d(samp, addr, x0[j], y0[j], z1[j]);
      const float *tx11 = get_texel_3d(samp, addr, x1[j], y0[j], z1[j]);
      const float *tx12 = get_texel_3d(samp, addr, x0[j], y1[j], z1[j]);
      const float *tx13 = get_texel_3d(samp, addr, x1[j], y1[j], z1[j]);
      
      /* interpolate R, G, B, A */
      for (c = 0; c < 4; c++) {
         rgba[c][j] = lerp_3d(xw[j], yw[j], zw[j],
                              tx00[c], tx01[c],
                              tx02[c], tx03[c],
                              tx10[c], tx11[c],
                              tx12[c], tx13[c]);
      }
   }
}


/* Calculate level of detail for every fragment.
 * Note that lambda has already been biased by global LOD bias.
 */
static INLINE void
compute_lod(const struct pipe_sampler_state *sampler,
            const float biased_lambda,
            const float lodbias[QUAD_SIZE],
            float lod[QUAD_SIZE])
{
   uint i;

   for (i = 0; i < QUAD_SIZE; i++) {
      lod[i] = biased_lambda + lodbias[i];
      lod[i] = CLAMP(lod[i], sampler->min_lod, sampler->max_lod);
   }
}


static void
mip_filter_linear(struct tgsi_sampler *tgsi_sampler,
                  const float s[QUAD_SIZE],
                  const float t[QUAD_SIZE],
                  const float p[QUAD_SIZE],
                  const float c0[QUAD_SIZE],
                  enum tgsi_sampler_control control,
                  float rgba[NUM_CHANNELS][QUAD_SIZE])
{
   struct sp_sampler_variant *samp = sp_sampler_variant(tgsi_sampler);
   const struct pipe_resource *texture = samp->view->texture;
   int level0;
   float lambda;
   float lod[QUAD_SIZE];

   if (control == tgsi_sampler_lod_bias) {
      lambda = samp->compute_lambda(samp, s, t, p) + samp->sampler->lod_bias;
      compute_lod(samp->sampler, lambda, c0, lod);
   } else {
      assert(control == tgsi_sampler_lod_explicit);

      memcpy(lod, c0, sizeof(lod));
   }

   /* XXX: Take into account all lod values.
    */
   lambda = lod[0];
   level0 = samp->view->u.tex.first_level + (int)lambda;

   if (lambda < 0.0) { 
      samp->level = samp->view->u.tex.first_level;
      samp->mag_img_filter(tgsi_sampler, s, t, p, NULL, tgsi_sampler_lod_bias, rgba);
   }
   else if (level0 >= texture->last_level) {
      samp->level = texture->last_level;
      samp->min_img_filter(tgsi_sampler, s, t, p, NULL, tgsi_sampler_lod_bias, rgba);
   }
   else {
      float levelBlend = frac(lambda);
      float rgba0[4][4];
      float rgba1[4][4];
      int c,j;

      samp->level = level0;
      samp->min_img_filter(tgsi_sampler, s, t, p, NULL, tgsi_sampler_lod_bias, rgba0);

      samp->level = level0+1;
      samp->min_img_filter(tgsi_sampler, s, t, p, NULL, tgsi_sampler_lod_bias, rgba1);

      for (j = 0; j < QUAD_SIZE; j++) {
         for (c = 0; c < 4; c++) {
            rgba[c][j] = lerp(levelBlend, rgba0[c][j], rgba1[c][j]);
         }
      }
   }

   if (DEBUG_TEX) {
      print_sample(__FUNCTION__, rgba);
   }
}


/**
 * Compute nearest mipmap level from texcoords.
 * Then sample the texture level for four elements of a quad.
 * \param c0  the LOD bias factors, or absolute LODs (depending on control)
 */
static void
mip_filter_nearest(struct tgsi_sampler *tgsi_sampler,
                   const float s[QUAD_SIZE],
                   const float t[QUAD_SIZE],
                   const float p[QUAD_SIZE],
                   const float c0[QUAD_SIZE],
                   enum tgsi_sampler_control control,
                   float rgba[NUM_CHANNELS][QUAD_SIZE])
{
   struct sp_sampler_variant *samp = sp_sampler_variant(tgsi_sampler);
   const struct pipe_resource *texture = samp->view->texture;
   float lambda;
   float lod[QUAD_SIZE];

   if (control == tgsi_sampler_lod_bias) {
      lambda = samp->compute_lambda(samp, s, t, p) + samp->sampler->lod_bias;
      compute_lod(samp->sampler, lambda, c0, lod);
   } else {
      assert(control == tgsi_sampler_lod_explicit);

      memcpy(lod, c0, sizeof(lod));
   }

   /* XXX: Take into account all lod values.
    */
   lambda = lod[0];

   if (lambda < 0.0) { 
      samp->level = samp->view->u.tex.first_level;
      samp->mag_img_filter(tgsi_sampler, s, t, p, NULL, tgsi_sampler_lod_bias, rgba);
   }
   else {
      samp->level = samp->view->u.tex.first_level + (int)(lambda + 0.5F) ;
      samp->level = MIN2(samp->level, (int)texture->last_level);
      samp->min_img_filter(tgsi_sampler, s, t, p, NULL, tgsi_sampler_lod_bias, rgba);
   }

   if (DEBUG_TEX) {
      print_sample(__FUNCTION__, rgba);
   }
}


static void
mip_filter_none(struct tgsi_sampler *tgsi_sampler,
                const float s[QUAD_SIZE],
                const float t[QUAD_SIZE],
                const float p[QUAD_SIZE],
                const float c0[QUAD_SIZE],
                enum tgsi_sampler_control control,
                float rgba[NUM_CHANNELS][QUAD_SIZE])
{
   struct sp_sampler_variant *samp = sp_sampler_variant(tgsi_sampler);
   float lambda;
   float lod[QUAD_SIZE];

   if (control == tgsi_sampler_lod_bias) {
      lambda = samp->compute_lambda(samp, s, t, p) + samp->sampler->lod_bias;
      compute_lod(samp->sampler, lambda, c0, lod);
   } else {
      assert(control == tgsi_sampler_lod_explicit);

      memcpy(lod, c0, sizeof(lod));
   }

   /* XXX: Take into account all lod values.
    */
   lambda = lod[0];

   samp->level = samp->view->u.tex.first_level;
   if (lambda < 0.0) { 
      samp->mag_img_filter(tgsi_sampler, s, t, p, NULL, tgsi_sampler_lod_bias, rgba);
   }
   else {
      samp->min_img_filter(tgsi_sampler, s, t, p, NULL, tgsi_sampler_lod_bias, rgba);
   }
}


/* For anisotropic filtering */
#define WEIGHT_LUT_SIZE 1024

static float *weightLut = NULL;

/**
 * Creates the look-up table used to speed-up EWA sampling
 */
static void
create_filter_table(void)
{
   unsigned i;
   if (!weightLut) {
      weightLut = (float *) MALLOC(WEIGHT_LUT_SIZE * sizeof(float));

      for (i = 0; i < WEIGHT_LUT_SIZE; ++i) {
         float alpha = 2;
         float r2 = (float) i / (float) (WEIGHT_LUT_SIZE - 1);
         float weight = (float) exp(-alpha * r2);
         weightLut[i] = weight;
      }
   }
}


/**
 * Elliptical weighted average (EWA) filter for producing high quality
 * anisotropic filtered results.
 * Based on the Higher Quality Elliptical Weighted Avarage Filter
 * published by Paul S. Heckbert in his Master's Thesis
 * "Fundamentals of Texture Mapping and Image Warping" (1989)
 */
static void
img_filter_2d_ewa(struct tgsi_sampler *tgsi_sampler,
                  const float s[QUAD_SIZE],
                  const float t[QUAD_SIZE],
                  const float p[QUAD_SIZE],
                  const float c0[QUAD_SIZE],
                  enum tgsi_sampler_control control,
                  const float dudx, const float dvdx,
                  const float dudy, const float dvdy,
                  float rgba[NUM_CHANNELS][QUAD_SIZE])
{
   const struct sp_sampler_variant *samp = sp_sampler_variant(tgsi_sampler);
   const struct pipe_resource *texture = samp->view->texture;

   unsigned level0 = samp->level > 0 ? samp->level : 0;
   float scaling = 1.0 / (1 << level0);
   int width = u_minify(texture->width0, level0);
   int height = u_minify(texture->height0, level0);

   float ux = dudx * scaling;
   float vx = dvdx * scaling;
   float uy = dudy * scaling;
   float vy = dvdy * scaling;

   /* compute ellipse coefficients to bound the region: 
    * A*x*x + B*x*y + C*y*y = F.
    */
   float A = vx*vx+vy*vy+1;
   float B = -2*(ux*vx+uy*vy);
   float C = ux*ux+uy*uy+1;
   float F = A*C-B*B/4.0;

   /* check if it is an ellipse */
   /* ASSERT(F > 0.0); */

   /* Compute the ellipse's (u,v) bounding box in texture space */
   float d = -B*B+4.0*C*A;
   float box_u = 2.0 / d * sqrt(d*C*F); /* box_u -> half of bbox with   */
   float box_v = 2.0 / d * sqrt(A*d*F); /* box_v -> half of bbox height */

   float rgba_temp[NUM_CHANNELS][QUAD_SIZE];
   float s_buffer[QUAD_SIZE];
   float t_buffer[QUAD_SIZE];
   float weight_buffer[QUAD_SIZE];
   unsigned buffer_next;
   int j;
   float den;// = 0.0F;
   float ddq;
   float U;// = u0 - tex_u;
   int v;

   /* Scale ellipse formula to directly index the Filter Lookup Table.
    * i.e. scale so that F = WEIGHT_LUT_SIZE-1
    */
   double formScale = (double) (WEIGHT_LUT_SIZE - 1) / F;
   A *= formScale;
   B *= formScale;
   C *= formScale;
   /* F *= formScale; */ /* no need to scale F as we don't use it below here */

   /* For each quad, the du and dx values are the same and so the ellipse is
    * also the same. Note that texel/image access can only be performed using
    * a quad, i.e. it is not possible to get the pixel value for a single
    * tex coord. In order to have a better performance, the access is buffered
    * using the s_buffer/t_buffer and weight_buffer. Only when the buffer is full,
    * then the pixel values are read from the image.
    */
   ddq = 2 * A;
   
   for (j = 0; j < QUAD_SIZE; j++) {
      /* Heckbert MS thesis, p. 59; scan over the bounding box of the ellipse
       * and incrementally update the value of Ax^2+Bxy*Cy^2; when this
       * value, q, is less than F, we're inside the ellipse
       */
      float tex_u = -0.5F + s[j] * texture->width0 * scaling;
      float tex_v = -0.5F + t[j] * texture->height0 * scaling;

      int u0 = (int) floorf(tex_u - box_u);
      int u1 = (int) ceilf(tex_u + box_u);
      int v0 = (int) floorf(tex_v - box_v);
      int v1 = (int) ceilf(tex_v + box_v);

      float num[4] = {0.0F, 0.0F, 0.0F, 0.0F};
      buffer_next = 0;
      den = 0;
      U = u0 - tex_u;
      for (v = v0; v <= v1; ++v) {
         float V = v - tex_v;
         float dq = A * (2 * U + 1) + B * V;
         float q = (C * V + B * U) * V + A * U * U;

         int u;
         for (u = u0; u <= u1; ++u) {
            /* Note that the ellipse has been pre-scaled so F = WEIGHT_LUT_SIZE - 1 */
            if (q < WEIGHT_LUT_SIZE) {
               /* as a LUT is used, q must never be negative;
                * should not happen, though
                */
               const int qClamped = q >= 0.0F ? q : 0;
               float weight = weightLut[qClamped];

               weight_buffer[buffer_next] = weight;
               s_buffer[buffer_next] = u / ((float) width);
               t_buffer[buffer_next] = v / ((float) height);
            
               buffer_next++;
               if (buffer_next == QUAD_SIZE) {
                  /* 4 texel coords are in the buffer -> read it now */
                  unsigned jj;
                  /* it is assumed that samp->min_img_filter is set to
                   * img_filter_2d_nearest or one of the
                   * accelerated img_filter_2d_nearest_XXX functions.
                   */
                  samp->min_img_filter(tgsi_sampler, s_buffer, t_buffer, p, NULL,
                                        tgsi_sampler_lod_bias, rgba_temp);
                  for (jj = 0; jj < buffer_next; jj++) {
                     num[0] += weight_buffer[jj] * rgba_temp[0][jj];
                     num[1] += weight_buffer[jj] * rgba_temp[1][jj];
                     num[2] += weight_buffer[jj] * rgba_temp[2][jj];
                     num[3] += weight_buffer[jj] * rgba_temp[3][jj];
                  }

                  buffer_next = 0;
               }

               den += weight;
            }
            q += dq;
            dq += ddq;
         }
      }

      /* if the tex coord buffer contains unread values, we will read them now.
       * Note that in most cases we have to read more pixel values than required,
       * however, as the img_filter_2d_nearest function(s) does not have a count
       * parameter, we need to read the whole quad and ignore the unused values
       */
      if (buffer_next > 0) {
         unsigned jj;
         /* it is assumed that samp->min_img_filter is set to
          * img_filter_2d_nearest or one of the
          * accelerated img_filter_2d_nearest_XXX functions.
          */
         samp->min_img_filter(tgsi_sampler, s_buffer, t_buffer, p, NULL,
                               tgsi_sampler_lod_bias, rgba_temp);
         for (jj = 0; jj < buffer_next; jj++) {
            num[0] += weight_buffer[jj] * rgba_temp[0][jj];
            num[1] += weight_buffer[jj] * rgba_temp[1][jj];
            num[2] += weight_buffer[jj] * rgba_temp[2][jj];
            num[3] += weight_buffer[jj] * rgba_temp[3][jj];
         }
      }

      if (den <= 0.0F) {
         /* Reaching this place would mean
          * that no pixels intersected the ellipse.
          * This should never happen because
          * the filter we use always
          * intersects at least one pixel.
          */

         /*rgba[0]=0;
         rgba[1]=0;
         rgba[2]=0;
         rgba[3]=0;*/
         /* not enough pixels in resampling, resort to direct interpolation */
         samp->min_img_filter(tgsi_sampler, s, t, p, NULL, tgsi_sampler_lod_bias, rgba_temp);
         den = 1;
         num[0] = rgba_temp[0][j];
         num[1] = rgba_temp[1][j];
         num[2] = rgba_temp[2][j];
         num[3] = rgba_temp[3][j];
      }

      rgba[0][j] = num[0] / den;
      rgba[1][j] = num[1] / den;
      rgba[2][j] = num[2] / den;
      rgba[3][j] = num[3] / den;
   }
}


/**
 * Sample 2D texture using an anisotropic filter.
 */
static void
mip_filter_linear_aniso(struct tgsi_sampler *tgsi_sampler,
                        const float s[QUAD_SIZE],
                        const float t[QUAD_SIZE],
                        const float p[QUAD_SIZE],
                        const float c0[QUAD_SIZE],
                        enum tgsi_sampler_control control,
                        float rgba[NUM_CHANNELS][QUAD_SIZE])
{
   struct sp_sampler_variant *samp = sp_sampler_variant(tgsi_sampler);
   const struct pipe_resource *texture = samp->view->texture;
   int level0;
   float lambda;
   float lod[QUAD_SIZE];

   float s_to_u = u_minify(texture->width0, samp->view->u.tex.first_level);
   float t_to_v = u_minify(texture->height0, samp->view->u.tex.first_level);
   float dudx = (s[QUAD_BOTTOM_RIGHT] - s[QUAD_BOTTOM_LEFT]) * s_to_u;
   float dudy = (s[QUAD_TOP_LEFT]     - s[QUAD_BOTTOM_LEFT]) * s_to_u;
   float dvdx = (t[QUAD_BOTTOM_RIGHT] - t[QUAD_BOTTOM_LEFT]) * t_to_v;
   float dvdy = (t[QUAD_TOP_LEFT]     - t[QUAD_BOTTOM_LEFT]) * t_to_v;
   
   if (control == tgsi_sampler_lod_bias) {
      /* note: instead of working with Px and Py, we will use the 
       * squared length instead, to avoid sqrt.
       */
      float Px2 = dudx * dudx + dvdx * dvdx;
      float Py2 = dudy * dudy + dvdy * dvdy;

      float Pmax2;
      float Pmin2;
      float e;
      const float maxEccentricity = samp->sampler->max_anisotropy * samp->sampler->max_anisotropy;
      
      if (Px2 < Py2) {
         Pmax2 = Py2;
         Pmin2 = Px2;
      }
      else {
         Pmax2 = Px2;
         Pmin2 = Py2;
      }
      
      /* if the eccentricity of the ellipse is too big, scale up the shorter
       * of the two vectors to limit the maximum amount of work per pixel
       */
      e = Pmax2 / Pmin2;
      if (e > maxEccentricity) {
         /* float s=e / maxEccentricity;
            minor[0] *= s;
            minor[1] *= s;
            Pmin2 *= s; */
         Pmin2 = Pmax2 / maxEccentricity;
      }
      
      /* note: we need to have Pmin=sqrt(Pmin2) here, but we can avoid
       * this since 0.5*log(x) = log(sqrt(x))
       */
      lambda = 0.5F * util_fast_log2(Pmin2) + samp->sampler->lod_bias;
      compute_lod(samp->sampler, lambda, c0, lod);
   }
   else {
      assert(control == tgsi_sampler_lod_explicit);

      memcpy(lod, c0, sizeof(lod));
   }
   
   /* XXX: Take into account all lod values.
    */
   lambda = lod[0];
   level0 = samp->view->u.tex.first_level + (int)lambda;

   /* If the ellipse covers the whole image, we can
    * simply return the average of the whole image.
    */
   if (level0 >= (int) texture->last_level) {
      samp->level = texture->last_level;
      samp->min_img_filter(tgsi_sampler, s, t, p, NULL, tgsi_sampler_lod_bias, rgba);
   }
   else {
      /* don't bother interpolating between multiple LODs; it doesn't
       * seem to be worth the extra running time.
       */
      samp->level = level0;
      img_filter_2d_ewa(tgsi_sampler, s, t, p, NULL, tgsi_sampler_lod_bias,
                        dudx, dvdx, dudy, dvdy, rgba);
   }

   if (DEBUG_TEX) {
      print_sample(__FUNCTION__, rgba);
   }
}



/**
 * Specialized version of mip_filter_linear with hard-wired calls to
 * 2d lambda calculation and 2d_linear_repeat_POT img filters.
 */
static void
mip_filter_linear_2d_linear_repeat_POT(
   struct tgsi_sampler *tgsi_sampler,
   const float s[QUAD_SIZE],
   const float t[QUAD_SIZE],
   const float p[QUAD_SIZE],
   const float c0[QUAD_SIZE],
   enum tgsi_sampler_control control,
   float rgba[NUM_CHANNELS][QUAD_SIZE])
{
   struct sp_sampler_variant *samp = sp_sampler_variant(tgsi_sampler);
   const struct pipe_resource *texture = samp->view->texture;
   int level0;
   float lambda;
   float lod[QUAD_SIZE];

   if (control == tgsi_sampler_lod_bias) {
      lambda = samp->compute_lambda(samp, s, t, p) + samp->sampler->lod_bias;
      compute_lod(samp->sampler, lambda, c0, lod);
   } else {
      assert(control == tgsi_sampler_lod_explicit);

      memcpy(lod, c0, sizeof(lod));
   }

   /* XXX: Take into account all lod values.
    */
   lambda = lod[0];
   level0 = samp->view->u.tex.first_level + (int)lambda;

   /* Catches both negative and large values of level0:
    */
   if ((unsigned)level0 >= texture->last_level) { 
      if (level0 < 0)
         samp->level = samp->view->u.tex.first_level;
      else
         samp->level = texture->last_level;

      img_filter_2d_linear_repeat_POT(tgsi_sampler, s, t, p, NULL, tgsi_sampler_lod_bias, rgba);
   }
   else {
      float levelBlend = frac(lambda);
      float rgba0[4][4];
      float rgba1[4][4];
      int c,j;

      samp->level = level0;
      img_filter_2d_linear_repeat_POT(tgsi_sampler, s, t, p, NULL, tgsi_sampler_lod_bias, rgba0);

      samp->level = level0+1;
      img_filter_2d_linear_repeat_POT(tgsi_sampler, s, t, p, NULL, tgsi_sampler_lod_bias, rgba1);

      for (j = 0; j < QUAD_SIZE; j++) {
         for (c = 0; c < 4; c++) {
            rgba[c][j] = lerp(levelBlend, rgba0[c][j], rgba1[c][j]);
         }
      }
   }

   if (DEBUG_TEX) {
      print_sample(__FUNCTION__, rgba);
   }
}



/**
 * Do shadow/depth comparisons.
 */
static void
sample_compare(struct tgsi_sampler *tgsi_sampler,
               const float s[QUAD_SIZE],
               const float t[QUAD_SIZE],
               const float p[QUAD_SIZE],
               const float c0[QUAD_SIZE],
               enum tgsi_sampler_control control,
               float rgba[NUM_CHANNELS][QUAD_SIZE])
{
   struct sp_sampler_variant *samp = sp_sampler_variant(tgsi_sampler);
   const struct pipe_sampler_state *sampler = samp->sampler;
   int j, k0, k1, k2, k3;
   float val;
   float pc0, pc1, pc2, pc3;

   samp->mip_filter(tgsi_sampler, s, t, p, c0, control, rgba);

   /**
    * Compare texcoord 'p' (aka R) against texture value 'rgba[0]'
    * for 2D Array texture we need to use the 'c0' (aka Q).
    * When we sampled the depth texture, the depth value was put into all
    * RGBA channels.  We look at the red channel here.
    */

   if (samp->view->texture->target == PIPE_TEXTURE_2D_ARRAY ||
       samp->view->texture->target == PIPE_TEXTURE_CUBE) {
      pc0 = CLAMP(c0[0], 0.0F, 1.0F);
      pc1 = CLAMP(c0[1], 0.0F, 1.0F);
      pc2 = CLAMP(c0[2], 0.0F, 1.0F);
      pc3 = CLAMP(c0[3], 0.0F, 1.0F);
   } else {
      pc0 = CLAMP(p[0], 0.0F, 1.0F);
      pc1 = CLAMP(p[1], 0.0F, 1.0F);
      pc2 = CLAMP(p[2], 0.0F, 1.0F);
      pc3 = CLAMP(p[3], 0.0F, 1.0F);
   }
   /* compare four texcoords vs. four texture samples */
   switch (sampler->compare_func) {
   case PIPE_FUNC_LESS:
      k0 = pc0 < rgba[0][0];
      k1 = pc1 < rgba[0][1];
      k2 = pc2 < rgba[0][2];
      k3 = pc3 < rgba[0][3];
      break;
   case PIPE_FUNC_LEQUAL:
      k0 = pc0 <= rgba[0][0];
      k1 = pc1 <= rgba[0][1];
      k2 = pc2 <= rgba[0][2];
      k3 = pc3 <= rgba[0][3];
      break;
   case PIPE_FUNC_GREATER:
      k0 = pc0 > rgba[0][0];
      k1 = pc1 > rgba[0][1];
      k2 = pc2 > rgba[0][2];
      k3 = pc3 > rgba[0][3];
      break;
   case PIPE_FUNC_GEQUAL:
      k0 = pc0 >= rgba[0][0];
      k1 = pc1 >= rgba[0][1];
      k2 = pc2 >= rgba[0][2];
      k3 = pc3 >= rgba[0][3];
      break;
   case PIPE_FUNC_EQUAL:
      k0 = pc0 == rgba[0][0];
      k1 = pc1 == rgba[0][1];
      k2 = pc2 == rgba[0][2];
      k3 = pc3 == rgba[0][3];
      break;
   case PIPE_FUNC_NOTEQUAL:
      k0 = pc0 != rgba[0][0];
      k1 = pc1 != rgba[0][1];
      k2 = pc2 != rgba[0][2];
      k3 = pc3 != rgba[0][3];
      break;
   case PIPE_FUNC_ALWAYS:
      k0 = k1 = k2 = k3 = 1;
      break;
   case PIPE_FUNC_NEVER:
      k0 = k1 = k2 = k3 = 0;
      break;
   default:
      k0 = k1 = k2 = k3 = 0;
      assert(0);
      break;
   }

   if (sampler->mag_img_filter == PIPE_TEX_FILTER_LINEAR) {
      /* convert four pass/fail values to an intensity in [0,1] */
      val = 0.25F * (k0 + k1 + k2 + k3);

      /* XXX returning result for default GL_DEPTH_TEXTURE_MODE = GL_LUMINANCE */
      for (j = 0; j < 4; j++) {
	 rgba[0][j] = rgba[1][j] = rgba[2][j] = val;
	 rgba[3][j] = 1.0F;
      }
   } else {
      for (j = 0; j < 4; j++) {
	 rgba[0][j] = k0;
	 rgba[1][j] = k1;
	 rgba[2][j] = k2;
	 rgba[3][j] = 1.0F;
      }
   }
}


/**
 * Use 3D texcoords to choose a cube face, then sample the 2D cube faces.
 * Put face info into the sampler faces[] array.
 */
static void
sample_cube(struct tgsi_sampler *tgsi_sampler,
            const float s[QUAD_SIZE],
            const float t[QUAD_SIZE],
            const float p[QUAD_SIZE],
            const float c0[QUAD_SIZE],
            enum tgsi_sampler_control control,
            float rgba[NUM_CHANNELS][QUAD_SIZE])
{
   struct sp_sampler_variant *samp = sp_sampler_variant(tgsi_sampler);
   unsigned j;
   float ssss[4], tttt[4];

   /*
     major axis
     direction    target                             sc     tc    ma
     ----------   -------------------------------    ---    ---   ---
     +rx          TEXTURE_CUBE_MAP_POSITIVE_X_EXT    -rz    -ry   rx
     -rx          TEXTURE_CUBE_MAP_NEGATIVE_X_EXT    +rz    -ry   rx
     +ry          TEXTURE_CUBE_MAP_POSITIVE_Y_EXT    +rx    +rz   ry
     -ry          TEXTURE_CUBE_MAP_NEGATIVE_Y_EXT    +rx    -rz   ry
     +rz          TEXTURE_CUBE_MAP_POSITIVE_Z_EXT    +rx    -ry   rz
     -rz          TEXTURE_CUBE_MAP_NEGATIVE_Z_EXT    -rx    -ry   rz
   */

   /* Choose the cube face and compute new s/t coords for the 2D face.
    *
    * Use the same cube face for all four pixels in the quad.
    *
    * This isn't ideal, but if we want to use a different cube face
    * per pixel in the quad, we'd have to also compute the per-face
    * LOD here too.  That's because the four post-face-selection
    * texcoords are no longer related to each other (they're
    * per-face!)  so we can't use subtraction to compute the partial
    * deriviates to compute the LOD.  Doing so (near cube edges
    * anyway) gives us pretty much random values.
    */
   {
      /* use the average of the four pixel's texcoords to choose the face */
      const float rx = 0.25F * (s[0] + s[1] + s[2] + s[3]);
      const float ry = 0.25F * (t[0] + t[1] + t[2] + t[3]);
      const float rz = 0.25F * (p[0] + p[1] + p[2] + p[3]);
      const float arx = fabsf(rx), ary = fabsf(ry), arz = fabsf(rz);

      if (arx >= ary && arx >= arz) {
         float sign = (rx >= 0.0F) ? 1.0F : -1.0F;
         uint face = (rx >= 0.0F) ? PIPE_TEX_FACE_POS_X : PIPE_TEX_FACE_NEG_X;
         for (j = 0; j < QUAD_SIZE; j++) {
            const float ima = -0.5F / fabsf(s[j]);
            ssss[j] = sign *  p[j] * ima + 0.5F;
            tttt[j] =         t[j] * ima + 0.5F;
            samp->faces[j] = face;
         }
      }
      else if (ary >= arx && ary >= arz) {
         float sign = (ry >= 0.0F) ? 1.0F : -1.0F;
         uint face = (ry >= 0.0F) ? PIPE_TEX_FACE_POS_Y : PIPE_TEX_FACE_NEG_Y;
         for (j = 0; j < QUAD_SIZE; j++) {
            const float ima = -0.5F / fabsf(t[j]);
            ssss[j] =        -s[j] * ima + 0.5F;
            tttt[j] = sign * -p[j] * ima + 0.5F;
            samp->faces[j] = face;
         }
      }
      else {
         float sign = (rz >= 0.0F) ? 1.0F : -1.0F;
         uint face = (rz >= 0.0F) ? PIPE_TEX_FACE_POS_Z : PIPE_TEX_FACE_NEG_Z;
         for (j = 0; j < QUAD_SIZE; j++) {
            const float ima = -0.5F / fabsf(p[j]);
            ssss[j] = sign * -s[j] * ima + 0.5F;
            tttt[j] =         t[j] * ima + 0.5F;
            samp->faces[j] = face;
         }
      }
   }

   /* In our little pipeline, the compare stage is next.  If compare
    * is not active, this will point somewhere deeper into the
    * pipeline, eg. to mip_filter or even img_filter.
    */
   samp->compare(tgsi_sampler, ssss, tttt, NULL, c0, control, rgba);
}

static void do_swizzling(const struct sp_sampler_variant *samp,
                         float in[NUM_CHANNELS][QUAD_SIZE],
                         float out[NUM_CHANNELS][QUAD_SIZE])
{
   int j;
   const unsigned swizzle_r = samp->key.bits.swizzle_r;
   const unsigned swizzle_g = samp->key.bits.swizzle_g;
   const unsigned swizzle_b = samp->key.bits.swizzle_b;
   const unsigned swizzle_a = samp->key.bits.swizzle_a;

   switch (swizzle_r) {
   case PIPE_SWIZZLE_ZERO:
      for (j = 0; j < 4; j++)
         out[0][j] = 0.0f;
      break;
   case PIPE_SWIZZLE_ONE:
      for (j = 0; j < 4; j++)
         out[0][j] = 1.0f;
      break;
   default:
      assert(swizzle_r < 4);
      for (j = 0; j < 4; j++)
         out[0][j] = in[swizzle_r][j];
   }

   switch (swizzle_g) {
   case PIPE_SWIZZLE_ZERO:
      for (j = 0; j < 4; j++)
         out[1][j] = 0.0f;
      break;
   case PIPE_SWIZZLE_ONE:
      for (j = 0; j < 4; j++)
         out[1][j] = 1.0f;
      break;
   default:
      assert(swizzle_g < 4);
      for (j = 0; j < 4; j++)
         out[1][j] = in[swizzle_g][j];
   }

   switch (swizzle_b) {
   case PIPE_SWIZZLE_ZERO:
      for (j = 0; j < 4; j++)
         out[2][j] = 0.0f;
      break;
   case PIPE_SWIZZLE_ONE:
      for (j = 0; j < 4; j++)
         out[2][j] = 1.0f;
      break;
   default:
      assert(swizzle_b < 4);
      for (j = 0; j < 4; j++)
         out[2][j] = in[swizzle_b][j];
   }

   switch (swizzle_a) {
   case PIPE_SWIZZLE_ZERO:
      for (j = 0; j < 4; j++)
         out[3][j] = 0.0f;
      break;
   case PIPE_SWIZZLE_ONE:
      for (j = 0; j < 4; j++)
         out[3][j] = 1.0f;
      break;
   default:
      assert(swizzle_a < 4);
      for (j = 0; j < 4; j++)
         out[3][j] = in[swizzle_a][j];
   }
}

static void
sample_swizzle(struct tgsi_sampler *tgsi_sampler,
               const float s[QUAD_SIZE],
               const float t[QUAD_SIZE],
               const float p[QUAD_SIZE],
               const float c0[QUAD_SIZE],
               enum tgsi_sampler_control control,
               float rgba[NUM_CHANNELS][QUAD_SIZE])
{
   struct sp_sampler_variant *samp = sp_sampler_variant(tgsi_sampler);
   float rgba_temp[NUM_CHANNELS][QUAD_SIZE];

   samp->sample_target(tgsi_sampler, s, t, p, c0, control, rgba_temp);

   do_swizzling(samp, rgba_temp, rgba);
}


static wrap_nearest_func
get_nearest_unorm_wrap(unsigned mode)
{
   switch (mode) {
   case PIPE_TEX_WRAP_CLAMP:
      return wrap_nearest_unorm_clamp;
   case PIPE_TEX_WRAP_CLAMP_TO_EDGE:
      return wrap_nearest_unorm_clamp_to_edge;
   case PIPE_TEX_WRAP_CLAMP_TO_BORDER:
      return wrap_nearest_unorm_clamp_to_border;
   default:
      assert(0);
      return wrap_nearest_unorm_clamp;
   }
}


static wrap_nearest_func
get_nearest_wrap(unsigned mode)
{
   switch (mode) {
   case PIPE_TEX_WRAP_REPEAT:
      return wrap_nearest_repeat;
   case PIPE_TEX_WRAP_CLAMP:
      return wrap_nearest_clamp;
   case PIPE_TEX_WRAP_CLAMP_TO_EDGE:
      return wrap_nearest_clamp_to_edge;
   case PIPE_TEX_WRAP_CLAMP_TO_BORDER:
      return wrap_nearest_clamp_to_border;
   case PIPE_TEX_WRAP_MIRROR_REPEAT:
      return wrap_nearest_mirror_repeat;
   case PIPE_TEX_WRAP_MIRROR_CLAMP:
      return wrap_nearest_mirror_clamp;
   case PIPE_TEX_WRAP_MIRROR_CLAMP_TO_EDGE:
      return wrap_nearest_mirror_clamp_to_edge;
   case PIPE_TEX_WRAP_MIRROR_CLAMP_TO_BORDER:
      return wrap_nearest_mirror_clamp_to_border;
   default:
      assert(0);
      return wrap_nearest_repeat;
   }
}


static wrap_linear_func
get_linear_unorm_wrap(unsigned mode)
{
   switch (mode) {
   case PIPE_TEX_WRAP_CLAMP:
      return wrap_linear_unorm_clamp;
   case PIPE_TEX_WRAP_CLAMP_TO_EDGE:
      return wrap_linear_unorm_clamp_to_edge;
   case PIPE_TEX_WRAP_CLAMP_TO_BORDER:
      return wrap_linear_unorm_clamp_to_border;
   default:
      assert(0);
      return wrap_linear_unorm_clamp;
   }
}


static wrap_linear_func
get_linear_wrap(unsigned mode)
{
   switch (mode) {
   case PIPE_TEX_WRAP_REPEAT:
      return wrap_linear_repeat;
   case PIPE_TEX_WRAP_CLAMP:
      return wrap_linear_clamp;
   case PIPE_TEX_WRAP_CLAMP_TO_EDGE:
      return wrap_linear_clamp_to_edge;
   case PIPE_TEX_WRAP_CLAMP_TO_BORDER:
      return wrap_linear_clamp_to_border;
   case PIPE_TEX_WRAP_MIRROR_REPEAT:
      return wrap_linear_mirror_repeat;
   case PIPE_TEX_WRAP_MIRROR_CLAMP:
      return wrap_linear_mirror_clamp;
   case PIPE_TEX_WRAP_MIRROR_CLAMP_TO_EDGE:
      return wrap_linear_mirror_clamp_to_edge;
   case PIPE_TEX_WRAP_MIRROR_CLAMP_TO_BORDER:
      return wrap_linear_mirror_clamp_to_border;
   default:
      assert(0);
      return wrap_linear_repeat;
   }
}


static compute_lambda_func
get_lambda_func(const union sp_sampler_key key)
{
   if (key.bits.processor == TGSI_PROCESSOR_VERTEX)
      return compute_lambda_vert;
   
   switch (key.bits.target) {
   case PIPE_TEXTURE_1D:
   case PIPE_TEXTURE_1D_ARRAY:
      return compute_lambda_1d;
   case PIPE_TEXTURE_2D:
   case PIPE_TEXTURE_2D_ARRAY:
   case PIPE_TEXTURE_RECT:
   case PIPE_TEXTURE_CUBE:
      return compute_lambda_2d;
   case PIPE_TEXTURE_3D:
      return compute_lambda_3d;
   default:
      assert(0);
      return compute_lambda_1d;
   }
}


static filter_func
get_img_filter(const union sp_sampler_key key,
               unsigned filter,
               const struct pipe_sampler_state *sampler)
{
   switch (key.bits.target) {
   case PIPE_TEXTURE_1D:
      if (filter == PIPE_TEX_FILTER_NEAREST) 
         return img_filter_1d_nearest;
      else
         return img_filter_1d_linear;
      break;
   case PIPE_TEXTURE_1D_ARRAY:
      if (filter == PIPE_TEX_FILTER_NEAREST) 
         return img_filter_1d_array_nearest;
      else
         return img_filter_1d_array_linear;
      break;
   case PIPE_TEXTURE_2D:
   case PIPE_TEXTURE_RECT:
      /* Try for fast path:
       */
      if (key.bits.is_pot &&
          sampler->wrap_s == sampler->wrap_t &&
          sampler->normalized_coords) 
      {
         switch (sampler->wrap_s) {
         case PIPE_TEX_WRAP_REPEAT:
            switch (filter) {
            case PIPE_TEX_FILTER_NEAREST:
               return img_filter_2d_nearest_repeat_POT;
            case PIPE_TEX_FILTER_LINEAR:
               return img_filter_2d_linear_repeat_POT;
            default:
               break;
            }
            break;
         case PIPE_TEX_WRAP_CLAMP:
            switch (filter) {
            case PIPE_TEX_FILTER_NEAREST:
               return img_filter_2d_nearest_clamp_POT;
            default:
               break;
            }
         }
      }
      /* Otherwise use default versions:
       */
      if (filter == PIPE_TEX_FILTER_NEAREST) 
         return img_filter_2d_nearest;
      else
         return img_filter_2d_linear;
      break;
   case PIPE_TEXTURE_2D_ARRAY:
      if (filter == PIPE_TEX_FILTER_NEAREST) 
         return img_filter_2d_array_nearest;
      else
         return img_filter_2d_array_linear;
      break;
   case PIPE_TEXTURE_CUBE:
      if (filter == PIPE_TEX_FILTER_NEAREST) 
         return img_filter_cube_nearest;
      else
         return img_filter_cube_linear;
      break;
   case PIPE_TEXTURE_3D:
      if (filter == PIPE_TEX_FILTER_NEAREST) 
         return img_filter_3d_nearest;
      else
         return img_filter_3d_linear;
      break;
   default:
      assert(0);
      return img_filter_1d_nearest;
   }
}


/**
 * Bind the given texture object and texture cache to the sampler variant.
 */
void
sp_sampler_variant_bind_view( struct sp_sampler_variant *samp,
                              struct softpipe_tex_tile_cache *tex_cache,
                              const struct pipe_sampler_view *view )
{
   const struct pipe_resource *texture = view->texture;

   samp->view = view;
   samp->cache = tex_cache;
   samp->xpot = util_logbase2( texture->width0 );
   samp->ypot = util_logbase2( texture->height0 );
   samp->level = view->u.tex.first_level;
}


void
sp_sampler_variant_destroy( struct sp_sampler_variant *samp )
{
   FREE(samp);
}

static void
sample_get_dims(struct tgsi_sampler *tgsi_sampler, int level,
		int dims[4])
{
    struct sp_sampler_variant *samp = sp_sampler_variant(tgsi_sampler);
    const struct pipe_sampler_view *view = samp->view;
    const struct pipe_resource *texture = view->texture;

    /* undefined according to EXT_gpu_program */
    level += view->u.tex.first_level;
    if (level > view->u.tex.last_level)
	return;

    dims[0] = u_minify(texture->width0, level);

    switch(texture->target) {
    case PIPE_TEXTURE_1D_ARRAY:
       dims[1] = texture->array_size;
       /* fallthrough */
    case PIPE_TEXTURE_1D:
    case PIPE_BUFFER:
       return;
    case PIPE_TEXTURE_2D_ARRAY:
       dims[2] = texture->array_size;
       /* fallthrough */
    case PIPE_TEXTURE_2D:
    case PIPE_TEXTURE_CUBE:
    case PIPE_TEXTURE_RECT:
       dims[1] = u_minify(texture->height0, level);
       return;
    case PIPE_TEXTURE_3D:
       dims[1] = u_minify(texture->height0, level);
       dims[2] = u_minify(texture->depth0, level);
       return;
    default:
       assert(!"unexpected texture target in sample_get_dims()");
       return;
    }
}

/* this function is only used for unfiltered texel gets
   via the TGSI TXF opcode. */
static void
sample_get_texels(struct tgsi_sampler *tgsi_sampler,
	   const int v_i[QUAD_SIZE],
	   const int v_j[QUAD_SIZE],
	   const int v_k[QUAD_SIZE],
	   const int lod[QUAD_SIZE],
	   const int8_t offset[3],
	   float rgba[NUM_CHANNELS][QUAD_SIZE])
{
   const struct sp_sampler_variant *samp = sp_sampler_variant(tgsi_sampler);
   union tex_tile_address addr;
   const struct pipe_resource *texture = samp->view->texture;
   int j, c;
   const float *tx;
   bool need_swizzle = (samp->key.bits.swizzle_r != PIPE_SWIZZLE_RED ||
                        samp->key.bits.swizzle_g != PIPE_SWIZZLE_GREEN ||
                        samp->key.bits.swizzle_b != PIPE_SWIZZLE_BLUE ||
                        samp->key.bits.swizzle_a != PIPE_SWIZZLE_ALPHA);

   addr.value = 0;
   /* TODO write a better test for LOD */
   addr.bits.level = lod[0];

   switch(texture->target) {
   case PIPE_TEXTURE_1D:
      for (j = 0; j < QUAD_SIZE; j++) {
	 tx = get_texel_2d(samp, addr, v_i[j] + offset[0], 0);
	 for (c = 0; c < 4; c++) {
	    rgba[c][j] = tx[c];
	 }
      }
      break;
   case PIPE_TEXTURE_1D_ARRAY:
      for (j = 0; j < QUAD_SIZE; j++) {
	 tx = get_texel_1d_array(samp, addr, v_i[j] + offset[0],
				 v_j[j] + offset[1]);
	 for (c = 0; c < 4; c++) {
	    rgba[c][j] = tx[c];
	 }
      }
      break;
   case PIPE_TEXTURE_2D:
   case PIPE_TEXTURE_RECT:
      for (j = 0; j < QUAD_SIZE; j++) {
	 tx = get_texel_2d(samp, addr, v_i[j] + offset[0],
			   v_j[j] + offset[1]);
	 for (c = 0; c < 4; c++) {
	    rgba[c][j] = tx[c];
	 }
      }
      break;
   case PIPE_TEXTURE_2D_ARRAY:
      for (j = 0; j < QUAD_SIZE; j++) {
	 tx = get_texel_2d_array(samp, addr, v_i[j] + offset[0],
				 v_j[j] + offset[1],
				 v_k[j] + offset[2]);
	 for (c = 0; c < 4; c++) {
	    rgba[c][j] = tx[c];
	 }
      }
      break;
   case PIPE_TEXTURE_3D:
      for (j = 0; j < QUAD_SIZE; j++) {
	 tx = get_texel_3d(samp, addr, v_i[j] + offset[0], 
			   v_j[j] + offset[1],
			   v_k[j] + offset[2]);
	 for (c = 0; c < 4; c++) {
	    rgba[c][j] = tx[c];
	 }
      }
      break;
   case PIPE_TEXTURE_CUBE: /* TXF can't work on CUBE according to spec */
   default:
      assert(!"Unknown or CUBE texture type in TXF processing\n");
      break;
   }

   if (need_swizzle) {
      float rgba_temp[NUM_CHANNELS][QUAD_SIZE];
      memcpy(rgba_temp, rgba, sizeof(rgba_temp));
      do_swizzling(samp, rgba_temp, rgba);
   }
}
/**
 * Create a sampler variant for a given set of non-orthogonal state.
 */
struct sp_sampler_variant *
sp_create_sampler_variant( const struct pipe_sampler_state *sampler,
                           const union sp_sampler_key key )
{
   struct sp_sampler_variant *samp = CALLOC_STRUCT(sp_sampler_variant);
   if (!samp)
      return NULL;

   samp->sampler = sampler;
   samp->key = key;

   /* Note that (for instance) linear_texcoord_s and
    * nearest_texcoord_s may be active at the same time, if the
    * sampler min_img_filter differs from its mag_img_filter.
    */
   if (sampler->normalized_coords) {
      samp->linear_texcoord_s = get_linear_wrap( sampler->wrap_s );
      samp->linear_texcoord_t = get_linear_wrap( sampler->wrap_t );
      samp->linear_texcoord_p = get_linear_wrap( sampler->wrap_r );
      
      samp->nearest_texcoord_s = get_nearest_wrap( sampler->wrap_s );
      samp->nearest_texcoord_t = get_nearest_wrap( sampler->wrap_t );
      samp->nearest_texcoord_p = get_nearest_wrap( sampler->wrap_r );
   }
   else {
      samp->linear_texcoord_s = get_linear_unorm_wrap( sampler->wrap_s );
      samp->linear_texcoord_t = get_linear_unorm_wrap( sampler->wrap_t );
      samp->linear_texcoord_p = get_linear_unorm_wrap( sampler->wrap_r );
      
      samp->nearest_texcoord_s = get_nearest_unorm_wrap( sampler->wrap_s );
      samp->nearest_texcoord_t = get_nearest_unorm_wrap( sampler->wrap_t );
      samp->nearest_texcoord_p = get_nearest_unorm_wrap( sampler->wrap_r );
   }
   
   samp->compute_lambda = get_lambda_func( key );

   samp->min_img_filter = get_img_filter(key, sampler->min_img_filter, sampler);
   samp->mag_img_filter = get_img_filter(key, sampler->mag_img_filter, sampler);

   switch (sampler->min_mip_filter) {
   case PIPE_TEX_MIPFILTER_NONE:
      if (sampler->min_img_filter == sampler->mag_img_filter) 
         samp->mip_filter = samp->min_img_filter;         
      else
         samp->mip_filter = mip_filter_none;
      break;

   case PIPE_TEX_MIPFILTER_NEAREST:
      samp->mip_filter = mip_filter_nearest;
      break;

   case PIPE_TEX_MIPFILTER_LINEAR:
      if (key.bits.is_pot &&
          sampler->min_img_filter == sampler->mag_img_filter &&
          sampler->normalized_coords &&
          sampler->wrap_s == PIPE_TEX_WRAP_REPEAT &&
          sampler->wrap_t == PIPE_TEX_WRAP_REPEAT &&
          sampler->min_img_filter == PIPE_TEX_FILTER_LINEAR) {
         samp->mip_filter = mip_filter_linear_2d_linear_repeat_POT;
      }
      else {
         samp->mip_filter = mip_filter_linear;
      }
      
      /* Anisotropic filtering extension. */
      if (sampler->max_anisotropy > 1) {
      	samp->mip_filter = mip_filter_linear_aniso;
      	
      	/* Override min_img_filter: 
      	 * min_img_filter needs to be set to NEAREST since we need to access
      	 * each texture pixel as it is and weight it later; using linear
      	 * filters will have incorrect results.
      	 * By setting the filter to NEAREST here, we can avoid calling the
      	 * generic img_filter_2d_nearest in the anisotropic filter function,
      	 * making it possible to use one of the accelerated implementations 
      	 */
      	samp->min_img_filter = get_img_filter(key, PIPE_TEX_FILTER_NEAREST, sampler);
      	
      	/* on first access create the lookup table containing the filter weights. */
        if (!weightLut) {
           create_filter_table();
        }
      }
      
      break;
   }

   if (sampler->compare_mode != PIPE_TEX_COMPARE_NONE) {
      samp->compare = sample_compare;
   }
   else {
      /* Skip compare operation by promoting the mip_filter function
       * pointer:
       */
      samp->compare = samp->mip_filter;
   }
   
   if (key.bits.target == PIPE_TEXTURE_CUBE) {
      samp->sample_target = sample_cube;
   }
   else {
      samp->faces[0] = 0;
      samp->faces[1] = 0;
      samp->faces[2] = 0;
      samp->faces[3] = 0;

      /* Skip cube face determination by promoting the compare
       * function pointer:
       */
      samp->sample_target = samp->compare;
   }

   if (key.bits.swizzle_r != PIPE_SWIZZLE_RED ||
       key.bits.swizzle_g != PIPE_SWIZZLE_GREEN ||
       key.bits.swizzle_b != PIPE_SWIZZLE_BLUE ||
       key.bits.swizzle_a != PIPE_SWIZZLE_ALPHA) {
      samp->base.get_samples = sample_swizzle;
   }
   else {
      samp->base.get_samples = samp->sample_target;
   }

   samp->base.get_dims = sample_get_dims;
   samp->base.get_texel = sample_get_texels;
   return samp;
}
