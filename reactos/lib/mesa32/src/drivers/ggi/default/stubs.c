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

#include <stdio.h>

#include <ggi/internal/ggi-dl.h>
#include <ggi/mesa/ggimesa_int.h>
#include <ggi/mesa/debug.h>

#include "swrast/swrast.h"
//#include "swrast_setup/swrast_setup.h"
//#include "swrast/s_context.h"
//#include "swrast/s_depth.h"
//#include "swrast/s_triangle.h"

#define FLIP(coord) (LIBGGI_MODE(ggi_ctx->ggi_visual)->visible.y-(coord)-1)

/**********************************************************************/
/*****            Write spans of pixels                           *****/
/**********************************************************************/

void GGIwrite_ci32_span(const GLcontext *ctx,
                         GLuint n, GLint x, GLint y,
                         const GLuint ci[],
                         const GLubyte mask[] )
{
	ggi_mesa_context_t ggi_ctx = (ggi_mesa_context_t)ctx->DriverCtx;
	y = FLIP(y);
	if (mask)
	{
		while (n--) {
			if (*mask++)
				ggiPutPixel(ggi_ctx->ggi_visual, x, y, *ci);
			x++;
			ci++;
		}
	}
	else
	{
		while (n--)
			ggiPutPixel(ggi_ctx->ggi_visual, x++, y, *ci++);
	}
}

void GGIwrite_ci8_span(const GLcontext *ctx,
                         GLuint n, GLint x, GLint y,
                         const GLubyte ci[],
                         const GLubyte mask[] )
{
	ggi_mesa_context_t ggi_ctx = (ggi_mesa_context_t)ctx->DriverCtx;
	y = FLIP(y);
	if (mask)
	{
		while (n--) {
			if (*mask++)
				ggiPutPixel(ggi_ctx->ggi_visual, x, y, *ci);
			x++;
			ci++;
		}
	}
	else
	{
		while (n--)
			ggiPutPixel(ggi_ctx->ggi_visual, x++, y, *ci++);
	}
}

void GGIwrite_mono_ci_span(const GLcontext *ctx, GLuint n, GLint x, GLint y,
			   const GLuint ci, const GLubyte mask[])
{
	ggi_mesa_context_t ggi_ctx = (ggi_mesa_context_t)ctx->DriverCtx;
	y = FLIP(y);
	if (mask)
	{
		while (n--) {
			if (*mask++)
				ggiPutPixel(ggi_ctx->ggi_visual, x, y, ci);
			x++;
		}
	}
	else
	{
		while (n--)
			ggiPutPixel(ggi_ctx->ggi_visual, x++, y, ci);
	}
}

void GGIwrite_mono_rgba_span(const GLcontext *ctx, GLuint n, GLint x, GLint y,
			     const GLchan rgba[4], const GLubyte mask[])
{
	ggi_mesa_context_t ggi_ctx = (ggi_mesa_context_t)ctx->DriverCtx;
	ggi_color rgb;
	ggi_pixel col;	

	y = FLIP(y);

	rgb.r = (uint16)(rgba[RCOMP]) << SHIFT;
	rgb.g = (uint16)(rgba[GCOMP]) << SHIFT;
	rgb.b = (uint16)(rgba[BCOMP]) << SHIFT;
	col = ggiMapColor(ggi_ctx->ggi_visual, &rgb);

	if (mask)
	{
		while (n--) {
			if (*mask++)
				ggiPutPixel(ggi_ctx->ggi_visual, x, y, col);
			x++;
		}
	}
	else
	{
		ggiDrawHLine(ggi_ctx->ggi_visual, x, y, n);
	}
}

void GGIwrite_rgba_span( const GLcontext *ctx,
                          GLuint n, GLint x, GLint y,
                          const GLubyte rgba[][4],
                          const GLubyte mask[])
{
	ggi_mesa_context_t ggi_ctx = (ggi_mesa_context_t)ctx->DriverCtx;
	ggi_color rgb;
	ggi_pixel col;	
	y = FLIP(y);

	if (mask)
	{
		while (n--) {
			if (*mask++) 
			{
				rgb.r = (uint16)(rgba[0][RCOMP]) << SHIFT;
				rgb.g = (uint16)(rgba[0][GCOMP]) << SHIFT;
				rgb.b = (uint16)(rgba[0][BCOMP]) << SHIFT;
				col = ggiMapColor(ggi_ctx->ggi_visual, &rgb);
				ggiPutPixel(ggi_ctx->ggi_visual, x, y, col);
			}
			x++;
			rgba++;
		}
	}
	else
	{
		while (n--)
		{
			rgb.r = (uint16)(rgba[0][RCOMP]) << SHIFT;
			rgb.g = (uint16)(rgba[0][GCOMP]) << SHIFT;
			rgb.b = (uint16)(rgba[0][BCOMP]) << SHIFT;
			col = ggiMapColor(ggi_ctx->ggi_visual, &rgb);
			ggiPutPixel(ggi_ctx->ggi_visual, x++, y, col);
			rgba++;
		}
	}
}

void GGIwrite_rgb_span( const GLcontext *ctx,
                          GLuint n, GLint x, GLint y,
                          const GLubyte rgba[][3],
                          const GLubyte mask[] )
{
	ggi_mesa_context_t ggi_ctx = (ggi_mesa_context_t)ctx->DriverCtx;
	ggi_color rgb;
	ggi_pixel col;	
	y = FLIP(y);

	if (mask)
	{
		while (n--) {
			if (*mask++) 
			{
				rgb.r = (uint16)(rgba[0][RCOMP]) << SHIFT;
				rgb.g = (uint16)(rgba[0][GCOMP]) << SHIFT;
				rgb.b = (uint16)(rgba[0][BCOMP]) << SHIFT;
				col = ggiMapColor(ggi_ctx->ggi_visual, &rgb);
				ggiPutPixel(ggi_ctx->ggi_visual, x, y, col);
			}
			x++;
			rgba++;
		}
	}
	else
	{
		while (n--)
		{
			rgb.r = (uint16)(rgba[0][RCOMP]) << SHIFT;
			rgb.g = (uint16)(rgba[0][GCOMP]) << SHIFT;
			rgb.b = (uint16)(rgba[0][BCOMP]) << SHIFT;
			col = ggiMapColor(ggi_ctx->ggi_visual, &rgb);
			ggiPutPixel(ggi_ctx->ggi_visual, x++, y, col);
			rgba++;
		}
	}
}



/**********************************************************************/
/*****                 Read spans of pixels                       *****/
/**********************************************************************/


void GGIread_ci32_span( const GLcontext *ctx,
                         GLuint n, GLint x, GLint y, GLuint ci[])
{
	ggi_mesa_context_t ggi_ctx = (ggi_mesa_context_t)ctx->DriverCtx;
	y = FLIP(y);
	while (n--)
		ggiGetPixel(ggi_ctx->ggi_visual, x++, y, ci++);
}

void GGIread_rgba_span( const GLcontext *ctx,
                         GLuint n, GLint x, GLint y,
                         GLubyte rgba[][4])
{
	ggi_mesa_context_t ggi_ctx = (ggi_mesa_context_t)ctx->DriverCtx;
	ggi_color rgb;
	ggi_pixel col;

	y = FLIP(y);

	while (n--)
	{
		ggiGetPixel(ggi_ctx->ggi_visual, x++, y, &col);
		ggiUnmapPixel(ggi_ctx->ggi_visual, col, &rgb);
		rgba[0][RCOMP] = (GLubyte) (rgb.r >> SHIFT);
		rgba[0][GCOMP] = (GLubyte) (rgb.g >> SHIFT);
		rgba[0][BCOMP] = (GLubyte) (rgb.b >> SHIFT);
		rgba[0][ACOMP] = 0;
		rgba++;
	}
}

/**********************************************************************/
/*****                  Write arrays of pixels                    *****/
/**********************************************************************/

void GGIwrite_ci32_pixels( const GLcontext *ctx,
                            GLuint n, const GLint x[], const GLint y[],
                            const GLuint ci[], const GLubyte mask[] )
{
	ggi_mesa_context_t ggi_ctx = (ggi_mesa_context_t)ctx->DriverCtx;
	while (n--) {
		if (*mask++)
			ggiPutPixel(ggi_ctx->ggi_visual, *x, FLIP(*y), *ci);
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
	while (n--) {
		if (*mask++)
			ggiPutPixel(ggi_ctx->ggi_visual, *x, FLIP(*y), ci);
		x++;
		y++;
	}
}

void GGIwrite_rgba_pixels( const GLcontext *ctx,
                            GLuint n, const GLint x[], const GLint y[],
                            const GLubyte rgba[][4],
                            const GLubyte mask[] )
{
	ggi_mesa_context_t ggi_ctx = (ggi_mesa_context_t)ctx->DriverCtx;
	ggi_pixel col;
	ggi_color rgb;
	while (n--) {
		if (*mask++) {
			rgb.r = (uint16)(rgba[0][RCOMP]) << SHIFT;
			rgb.g = (uint16)(rgba[0][GCOMP]) << SHIFT;
			rgb.b = (uint16)(rgba[0][BCOMP]) << SHIFT;
			col = ggiMapColor(ggi_ctx->ggi_visual, &rgb);
			ggiPutPixel(ggi_ctx->ggi_visual, *x, FLIP(*y), col);
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
	ggi_color rgb;
	ggi_pixel col;	

	rgb.r = (uint16)(rgba[RCOMP]) << SHIFT;
	rgb.g = (uint16)(rgba[GCOMP]) << SHIFT;
	rgb.b = (uint16)(rgba[BCOMP]) << SHIFT;
	col = ggiMapColor(ggi_ctx->ggi_visual, &rgb);
	
	while (n--) {
		if (*mask++)
			ggiPutPixel(ggi_ctx->ggi_visual, *x, FLIP(*y), col);
		x++;
		y++;
	}
}

/**********************************************************************/
/*****                   Read arrays of pixels                    *****/
/**********************************************************************/

void GGIread_ci32_pixels( const GLcontext *ctx,
                           GLuint n, const GLint x[], const GLint y[],
                           GLuint ci[], const GLubyte mask[])
{
	ggi_mesa_context_t ggi_ctx = (ggi_mesa_context_t)ctx->DriverCtx;
	while (n--) {
		if (*mask++) 
			ggiGetPixel(ggi_ctx->ggi_visual, *x, FLIP(*y), ci);
		ci++;
		x++;
		y++;
	}
}

void GGIread_rgba_pixels( const GLcontext *ctx,
                           GLuint n, const GLint x[], const GLint y[],
                           GLubyte rgba[][4],
                           const GLubyte mask[] )
{
	ggi_mesa_context_t ggi_ctx = (ggi_mesa_context_t)ctx->DriverCtx;
	ggi_color rgb;
	ggi_pixel col;

	while (n--)
	{
		if (*mask++)
		{
			ggiGetPixel(ggi_ctx->ggi_visual, *x, FLIP(*y), &col);
			ggiUnmapPixel(ggi_ctx->ggi_visual, col, &rgb);
			rgba[0][RCOMP] = rgb.r >> SHIFT; 
			rgba[0][GCOMP] = rgb.g >> SHIFT;
			rgba[0][BCOMP] = rgb.b >> SHIFT;
			rgba[0][ACOMP] = 0;
		}	
		x++;
		y++;
		rgba++;
	}
}

int GGIextend_visual(ggi_visual_t vis)
{
	return 0;
}

//static swrast_tri_func ggimesa_stubs_get_triangle_func(GLcontext *ctx);

int GGIsetup_driver(ggi_mesa_context_t ggi_ctx)
{
	struct swrast_device_driver *swdd =
		_swrast_GetDeviceDriverReference(ggi_ctx->gl_ctx);

	GGIMESADPRINT_CORE("stubs: setup_driver\n");
	
	swdd->WriteRGBASpan	= GGIwrite_rgba_span;
	swdd->WriteRGBSpan	= GGIwrite_rgb_span;
	swdd->WriteMonoRGBASpan = GGIwrite_mono_rgba_span;
	swdd->WriteRGBAPixels   = GGIwrite_rgba_pixels;
	swdd->WriteMonoRGBAPixels = GGIwrite_mono_rgba_pixels;

	swdd->WriteCI32Span     = GGIwrite_ci32_span;
	swdd->WriteCI8Span      = GGIwrite_ci8_span;
	swdd->WriteMonoCISpan	= GGIwrite_mono_ci_span;
	swdd->WriteCI32Pixels   = GGIwrite_ci32_pixels;
	swdd->WriteMonoCIPixels = GGIwrite_mono_ci_pixels;

	swdd->ReadCI32Span 	= GGIread_ci32_span;
	swdd->ReadRGBASpan	= GGIread_rgba_span;
	swdd->ReadCI32Pixels	= GGIread_ci32_pixels;
	swdd->ReadRGBAPixels	= GGIread_rgba_pixels;
	
	return 0;
}

void GGIupdate_state(ggi_mesa_context_t *ctx)
{
	//ctx->Driver.TriangleFunc = _swsetup_Triangle;
}

/*
void GGItriangle_flat(GLcontext *ctx, const SWvertex *v0, const SWvertex *v1, const SWvertex *v2)
{
//#define INTERP_Z 1
#define DEPTH_TYPE DEFAULT_SOFTWARE_DEPTH_TYPE	  
	
#define SETUP_CODE	       	\
	ggi_color color;	\
	color.r = v0->color[0];	\
	color.g = v0->color[1];	\
	color.b = v0->color[2];	\
	color.a = v0->color[3];	\
	ggiSetGCForeground(VIS, ggiMapColor(VIS, &color));

#define INNER_LOOP(LEFT,RIGHT,Y)				\
		ggiDrawHLine(VIS,LEFT,FLIP(Y),RIGHT-LEFT);	
		
#include "swrast/s_tritemp.h"
}


static void GGItriangle_flat_depth(GLcontext *ctx, const SWvertex *v0, const SWvertex *v1, const SWvertex *v2)
{
#define INTERP_Z 1
#define DEPTH_TYPE DEFAULT_SOFTWARE_DEPTH_TYPE

#define SETUP_CODE	       	\
	ggi_color color;	\
	color.r = v0->color[0];	\
	color.g = v0->color[1];	\
	color.b = v0->color[2];	\
	color.a = v0->color[3];	\
	ggiSetGCForeground(VIS, ggiMapColor(VIS, &color));
	
#define INNER_LOOP(LEFT,RIGHT,Y)					\
	{								\
	GLint i,xx=LEFT,yy=FLIP(Y),n=RIGHT-LEFT,length=0;		\
	GLint startx=xx;						\
	for (i=0;i<n;i++){						\
		GLdepth z=FixedToDepth(ffz);				\
		if (z<zRow[i])						\
		{							\
			zRow[i]=z;					\
			length++;					\
		}							\
		else							\
		{							\
			if (length)					\
			{ 						\
				ggiDrawHLine(VIS,startx,yy,length);	\
				length=0;				\
			}						\
			startx=xx+i+1;					\
		}							\
		ffz+=fdzdx;						\
	}								\
	if (length) ggiDrawHLine(VIS,startx,yy,length);			\
	}

#include "swrast/s_tritemp.h"
}


static swrast_tri_func ggimesa_stubs_get_triangle_func(GLcontext *ctx)
{
	if (ctx->Stencil.Enabled) return NULL;
	if (ctx->Polygon.SmoothFlag) return NULL;
	if (ctx->Polygon.StippleFlag) return NULL;
	if (ctx->Texture._ReallyEnabled) return NULL;  
	if (ctx->Light.ShadeModel==GL_SMOOTH) return NULL;
	if (ctx->Depth.Test && ctx->Depth.Func != GL_LESS) return NULL;

	if (ctx->Depth.Test) 
	  return GGItriangle_flat_depth;

	return GGItriangle_flat;	
}
*/
static int GGIopen(ggi_visual_t vis, struct ggi_dlhandle *dlh,
			const char *args, void *argptr, uint32 *dlret)
{
       	LIBGGI_MESAEXT(vis)->update_state = GGIupdate_state;
	LIBGGI_MESAEXT(vis)->setup_driver = GGIsetup_driver;

	*dlret = GGI_DL_OPDRAW;
	return 0;
}

int MesaGGIdl_stubs(int func, void **funcptr)
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
