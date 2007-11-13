/* GGI-Driver for MESA
 * 
 * Copyright (C) 1997  Uwe Maurer
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 * ---------------------------------------------------------------------
 * This code was derived from the following source of information:
 *
 * svgamesa.c and ddsample.c by Brian Paul
 * 
 */

#include <ggi/mesa/ggimesa.h>
#include <ggi/mesa/ggimesa_int.h>
#include <ggi/mesa/debug.h>
#include "swrast/swrast.h"

#define RMASK ((1<<R)-1)
#define GMASK ((1<<G)-1)
#define BMASK ((1<<B)-1)

#define RS (8-R)
#define GS (8-G)
#define BS (8-B)

#define PACK(color) (((color[RCOMP]>>RS) << (G+B)) |  \
                     ((color[GCOMP]>>GS) << B)     |  \
                     ((color[BCOMP]>>BS)))

#define FLIP(coord) (LIBGGI_VIRTY(ggi_ctx->ggi_visual) - (coord) - 1)


/**********************************************************************/
/*****            Write spans of pixels                           *****/
/**********************************************************************/

void GGIwrite_ci32_span(const GLcontext *ctx, GLuint n, GLint x, GLint y,
			const GLuint ci[], const GLubyte mask[])
{
	ggi_mesa_context_t ggi_ctx = (ggi_mesa_context_t)ctx->DriverCtx;
	FB_TYPE *fb;
	fb = (FB_TYPE *)((char *)LIBGGI_CURWRITE(ggi_ctx->ggi_visual) +
			 FLIP(y)*LIBGGI_FB_W_STRIDE(ggi_ctx->ggi_visual)) + x;

	if (mask) {
		while (n--) {
			if (*mask++)
				*fb = *ci;
			fb++;
			ci++;
		}
	} else {
		while (n--) *fb++ = *ci++;
	}
}

void GGIwrite_ci8_span(const GLcontext *ctx, GLuint n, GLint x, GLint y,
		       const GLubyte ci[], const GLubyte mask[])
{
	ggi_mesa_context_t ggi_ctx = (ggi_mesa_context_t)ctx->DriverCtx;
	FB_TYPE *fb;
	fb = (FB_TYPE *)((char *)LIBGGI_CURWRITE(ggi_ctx->ggi_visual) +
			 FLIP(y)*LIBGGI_FB_W_STRIDE(ggi_ctx->ggi_visual)) + x;

	if (mask) {
		while (n--) {
			if (*mask++)
				*fb = *ci;
			fb++;
			ci++;
		}	
	} else {
		while (n--) *fb++ = *ci++;
	}
}


void GGIwrite_rgba_span(const GLcontext *ctx, GLuint n, GLint x, GLint y,
                        const GLchan rgba[][4], const GLubyte mask[])
{
	ggi_mesa_context_t ggi_ctx = (ggi_mesa_context_t)ctx->DriverCtx;
	FB_TYPE *fb;
	fb = (FB_TYPE *)((char *)LIBGGI_CURWRITE(ggi_ctx->ggi_visual) +
			 FLIP(y)*LIBGGI_FB_W_STRIDE(ggi_ctx->ggi_visual)) + x;

	if (mask) {
		while (n--) {
			if (*mask++)
				*fb = PACK(rgba[0]);
			fb++;
			rgba++;
		}
	} else {
		while (n--) {
			*fb++ = PACK(rgba[0]);
			rgba++;
		}
	}
}

void GGIwrite_rgb_span(const GLcontext *ctx, GLuint n, GLint x, GLint y,
		       const GLchan rgba[][3], const GLubyte mask[])
{
	ggi_mesa_context_t ggi_ctx = (ggi_mesa_context_t)ctx->DriverCtx;
	FB_TYPE *fb;
	fb = (FB_TYPE *)((char *)LIBGGI_CURWRITE(ggi_ctx->ggi_visual) +
			 FLIP(y)*LIBGGI_FB_W_STRIDE(ggi_ctx->ggi_visual)) + x;

	if (mask) {
		while (n--) {
			if (*mask++)
				*fb = PACK(rgba[0]);
			fb++;
			rgba++;
		}
	} else {
		while (n--) {
			*fb++ = PACK(rgba[0]);
			rgba++;
		}
	}
}


void GGIwrite_mono_rgba_span(const GLcontext *ctx, GLuint n, GLint x, GLint y,
			     const GLchan color[4], const GLubyte mask[])
{
	ggi_mesa_context_t ggi_ctx = (ggi_mesa_context_t)ctx->DriverCtx;
	FB_TYPE *fb;
	fb = (FB_TYPE *)((char *)LIBGGI_CURWRITE(ggi_ctx->ggi_visual) +
			 FLIP(y)*LIBGGI_FB_W_STRIDE(ggi_ctx->ggi_visual)) + x;

	if (mask) {
		while (n--){
			if (*mask++)
				*fb = PACK(color);
			++fb;
		}
	} else {
		while (n--) 
			*fb++ = PACK(color);

	        /* Alternatively we could write a potentialy faster HLine
		ggiSetGCForeground(ggi_ctx->ggi_visual, color);
		ggiDrawHLine(ggi_ctx->ggi_visual,x,FLIP(y),n);
		*/
	}
}

void GGIwrite_mono_ci_span(const GLcontext *ctx, GLuint n, GLint x, GLint y,
			   const GLuint ci, const GLubyte mask[])
{
	ggi_mesa_context_t ggi_ctx = (ggi_mesa_context_t)ctx->DriverCtx;
	FB_TYPE *fb;
	fb = (FB_TYPE *)((char *)LIBGGI_CURWRITE(ggi_ctx->ggi_visual) +
			 FLIP(y)*LIBGGI_FB_W_STRIDE(ggi_ctx->ggi_visual)) + x;

	if (mask){
		while (n--){
			if (*mask++)
				*fb = ci;
			++fb;
		}
	} else {
		while (n--) 
		        *fb++ = ci;

	        /* Alternatively we could write a potentialy faster HLine
		ggiSetGCForeground(ggi_ctx->ggi_visual, ci);
		ggiDrawHLine(ggi_ctx->ggi_visual, x, FLIP(y), n);
		*/
	}
}


/**********************************************************************/
/*****                 Read spans of pixels                       *****/
/**********************************************************************/


void GGIread_ci32_span(const GLcontext *ctx,
		       GLuint n, GLint x, GLint y, GLuint ci[])
{
	ggi_mesa_context_t ggi_ctx = (ggi_mesa_context_t)ctx->DriverCtx;
	FB_TYPE *fb;
	fb = (FB_TYPE *)((char *)LIBGGI_CURWRITE(ggi_ctx->ggi_visual) +
			 FLIP(y)*LIBGGI_FB_W_STRIDE(ggi_ctx->ggi_visual)) + x;

	while (n--)
		*ci++ = (GLuint)*fb++;
}

void GGIread_rgba_span(const GLcontext *ctx,
		       GLuint n, GLint x, GLint y, GLchan rgba[][4])
{
	ggi_mesa_context_t ggi_ctx = (ggi_mesa_context_t)ctx->DriverCtx;
	FB_TYPE color;
	FB_TYPE *fb;
	fb = (FB_TYPE *)((char *)LIBGGI_CURWRITE(ggi_ctx->ggi_visual) +
			 FLIP(y)*LIBGGI_FB_W_STRIDE(ggi_ctx->ggi_visual)) + x;
	
	while (n--) {
		color = *fb++;
		rgba[0][RCOMP] = (GLubyte) (color>>(G+B))<<RS;  
		rgba[0][GCOMP] = (GLubyte) ((color>>B)& ((1<<G)-1))<<GS;  
		rgba[0][BCOMP] = (GLubyte) (color & ((1<<B)-1))<<BS;  
		rgba[0][ACOMP] = 0;
		rgba++;
	}
}

/**********************************************************************/
/*****                  Write arrays of pixels                    *****/
/**********************************************************************/

void GGIwrite_ci32_pixels(const GLcontext *ctx,
			  GLuint n, const GLint x[], const GLint y[],
			  const GLuint ci[], const GLubyte mask[])
{
	ggi_mesa_context_t ggi_ctx = (ggi_mesa_context_t)ctx->DriverCtx;
	int stride = LIBGGI_FB_W_STRIDE(ggi_ctx->ggi_visual);
	char *fb = (char *)LIBGGI_CURWRITE(ggi_ctx->ggi_visual);

	while (n--) {
		if (*mask++){
			FB_TYPE *dst = (FB_TYPE*)(fb + FLIP(*y)*stride) + *x;
			*dst = *ci;
		}
		ci++;
		x++;
		y++;
	}
}

void GGIwrite_mono_ci_pixels(const GLcontext *ctx,
			     GLuint n, const GLint x[], const GLint y[],
			     GLuint ci, const GLubyte mask[])
{
	ggi_mesa_context_t ggi_ctx = (ggi_mesa_context_t)ctx->DriverCtx;
	int stride = LIBGGI_FB_W_STRIDE(ggi_ctx->ggi_visual);
	char *fb = (char *)LIBGGI_CURWRITE(ggi_ctx->ggi_visual);

	while (n--) {
		if (*mask++){
			FB_TYPE *dst = (FB_TYPE*)(fb + FLIP(*y)*stride) + *x;
			*dst = ci;
		}
		x++;
		y++;
	}
}

void GGIwrite_rgba_pixels(const GLcontext *ctx,
			  GLuint n, const GLint x[], const GLint y[],
			  const GLchan rgba[][4], const GLubyte mask[])
{
	ggi_mesa_context_t ggi_ctx = (ggi_mesa_context_t)ctx->DriverCtx;
	int stride = LIBGGI_FB_W_STRIDE(ggi_ctx->ggi_visual);
	char *fb = (char *)LIBGGI_CURWRITE(ggi_ctx->ggi_visual);

	while (n--) {
		if (*mask++){
			FB_TYPE *dst = (FB_TYPE*)(fb + FLIP(*y)*stride) + *x;
			*dst = PACK(rgba[0]);
		}
		x++;
		y++;
		rgba++;
	}
}

void GGIwrite_mono_rgba_pixels(const GLcontext *ctx,
			       GLuint n, const GLint x[], const GLint y[],
			       const GLchan rgba[4], const GLubyte mask[])
{
	ggi_mesa_context_t ggi_ctx = (ggi_mesa_context_t)ctx->DriverCtx;
	int stride = LIBGGI_FB_W_STRIDE(ggi_ctx->ggi_visual);
	char *fb = (char *)LIBGGI_CURWRITE(ggi_ctx->ggi_visual);

	while (n--) {
		if (*mask++){
			FB_TYPE *dst = (FB_TYPE*)(fb + FLIP(*y)*stride) + *x;
			*dst = PACK(rgba);
		}
		
		x++;
		y++;
	}
}

/**********************************************************************/
/*****                   Read arrays of pixels                    *****/
/**********************************************************************/

void GGIread_ci32_pixels(const GLcontext *ctx,
			 GLuint n, const GLint x[], const GLint y[],
			 GLuint ci[], const GLubyte mask[])
{
	ggi_mesa_context_t ggi_ctx = (ggi_mesa_context_t)ctx->DriverCtx;
	int stride = LIBGGI_FB_W_STRIDE(ggi_ctx->ggi_visual);
	char *fb = (char *)LIBGGI_CURWRITE(ggi_ctx->ggi_visual);

	while (n--) {
		if (*mask++){
			FB_TYPE *src = (FB_TYPE*)(fb + FLIP(*y)*stride) + *x;
			*ci = *src;
		}
		ci++;
		x++;
		y++;
	}
}

void GGIread_rgba_pixels(const GLcontext *ctx,
			 GLuint n, const GLint x[], const GLint y[],
			 GLubyte rgba[][4], const GLubyte mask[])
{
	ggi_mesa_context_t ggi_ctx = (ggi_mesa_context_t)ctx->DriverCtx;
	int stride = LIBGGI_FB_W_STRIDE(ggi_ctx->ggi_visual);
	char *fb = (char *)LIBGGI_CURWRITE(ggi_ctx->ggi_visual);
	FB_TYPE color;

	while (n--) {
		if (*mask++) {	
			FB_TYPE *src = (FB_TYPE*)(fb + FLIP(*y)*stride) + *x;
			color = *src;

			rgba[0][RCOMP] = (GLubyte)(color>>(G+B))<<RS;  
			rgba[0][GCOMP] = (GLubyte)((color>>B)& ((1<<G)-1))<<GS;
			rgba[0][BCOMP] = (GLubyte) (color & ((1<<B)-1))<<BS;  
			rgba[0][ACOMP] = 0;
		}	
		x++;
		y++;
		rgba++;
	}
}

void GGIset_buffer(GLcontext *ctx, GLframebuffer *buffer, GLenum mode)
{
}

int GGIsetup_driver(ggi_mesa_context_t ggi_ctx)
{
	struct swrast_device_driver *swdd =
		_swrast_GetDeviceDriverReference(ggi_ctx->gl_ctx);

	GGIMESADPRINT_LIBS("linear_%d: GGIsetup_driver\n", sizeof(FB_TYPE)*8);
	
	swdd->WriteRGBASpan	= GGIwrite_rgba_span;
	swdd->WriteRGBSpan	= GGIwrite_rgb_span;
	swdd->WriteMonoRGBASpan	= GGIwrite_mono_rgba_span;
	swdd->WriteRGBAPixels	= GGIwrite_rgba_pixels;
	swdd->WriteMonoRGBAPixels = GGIwrite_mono_rgba_pixels;

	swdd->WriteCI32Span       = GGIwrite_ci32_span;
	swdd->WriteCI8Span       = GGIwrite_ci8_span;
	swdd->WriteMonoCISpan   = GGIwrite_mono_ci_span;
	swdd->WriteCI32Pixels     = GGIwrite_ci32_pixels;
	swdd->WriteMonoCIPixels = GGIwrite_mono_ci_pixels;

	swdd->ReadCI32Span = GGIread_ci32_span;
	swdd->ReadRGBASpan = GGIread_rgba_span;
	swdd->ReadCI32Pixels = GGIread_ci32_pixels;
	swdd->ReadRGBAPixels = GGIread_rgba_pixels;

	swdd->SetBuffer = GGIset_buffer;	

	return 0;
}

static int GGIopen(ggi_visual_t vis,struct ggi_dlhandle *dlh,
		   const char *args,void *argptr, uint32 *dlret)
{
	GGIMESADPRINT_CORE("linear_%d: GGIOpen\n", sizeof(FB_TYPE)*8);
	LIBGGI_MESAEXT(vis)->setup_driver = GGIsetup_driver;

	*dlret = GGI_DL_OPDRAW;
	return 0;
}

int DLOPENFUNC(int func, void **funcptr)
{
	switch (func) {
	case GGIFUNC_open:
		*funcptr = GGIopen;
		return 0;
	case GGIFUNC_exit:
	case GGIFUNC_close:
		*funcptr = NULL;
		return 0;
	default:
		*funcptr = NULL;
	}
	return GGI_ENOTFOUND;
}

