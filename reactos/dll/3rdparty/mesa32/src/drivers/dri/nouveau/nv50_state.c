/**************************************************************************

Copyright 2006 Nouveau
All Rights Reserved.

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
on the rights to use, copy, modify, merge, publish, distribute, sub
license, and/or sell copies of the Software, and to permit persons to whom
the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice (including the next
paragraph) shall be included in all copies or substantial portions of the
Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
ERIC ANHOLT OR SILICON INTEGRATED SYSTEMS CORP BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
USE OR OTHER DEALINGS IN THE SOFTWARE.

**************************************************************************/

#include "nouveau_context.h"
#include "nouveau_object.h"
#include "nouveau_fifo.h"
#include "nouveau_reg.h"
#include "nouveau_state.h"

#include "tnl/t_pipeline.h"

#include "mtypes.h"
#include "colormac.h"

static void nv50AlphaFunc(GLcontext *ctx, GLenum func, GLfloat ref)
{
	nouveauContextPtr nmesa = NOUVEAU_CONTEXT(ctx);
	GLubyte ubRef;
	CLAMPED_FLOAT_TO_UBYTE(ubRef, ref);

	BEGIN_RING_CACHE(NvSub3D, NV50_TCL_PRIMITIVE_3D_ALPHA_FUNC_REF, 2);
	OUT_RING_CACHE(ubRef);
	OUT_RING_CACHE(func);
}

static void nv50BlendColor(GLcontext *ctx, const GLfloat color[4])
{
	nouveauContextPtr nmesa = NOUVEAU_CONTEXT(ctx); 

	BEGIN_RING_CACHE(NvSub3D, NV50_TCL_PRIMITIVE_3D_BLEND_COLOR_R, 4);
	OUT_RING_CACHEf(color[0]);
	OUT_RING_CACHEf(color[1]);
	OUT_RING_CACHEf(color[2]);
	OUT_RING_CACHEf(color[3]);
}

static void nv50BlendEquationSeparate(GLcontext *ctx, GLenum modeRGB, GLenum modeA)
{
	nouveauContextPtr nmesa = NOUVEAU_CONTEXT(ctx);

	BEGIN_RING_CACHE(NvSub3D, NV50_TCL_PRIMITIVE_3D_BLEND_EQUATION_RGB, 1);
	OUT_RING_CACHE(modeRGB);
	BEGIN_RING_CACHE(NvSub3D, NV50_TCL_PRIMITIVE_3D_BLEND_EQUATION_ALPHA, 1);
	OUT_RING_CACHE(modeA);
}


static void nv50BlendFuncSeparate(GLcontext *ctx, GLenum sfactorRGB, GLenum dfactorRGB,
		GLenum sfactorA, GLenum dfactorA)
{
	nouveauContextPtr nmesa = NOUVEAU_CONTEXT(ctx);

	BEGIN_RING_CACHE(NvSub3D, NV50_TCL_PRIMITIVE_3D_BLEND_FUNC_SRC_RGB, 2);
	OUT_RING_CACHE(sfactorRGB);	/* FIXME, sometimes has |0x4000 */
	OUT_RING_CACHE(dfactorRGB);	/* FIXME, sometimes has |0x4000 */
	BEGIN_RING_CACHE(NvSub3D, NV50_TCL_PRIMITIVE_3D_BLEND_FUNC_SRC_ALPHA, 2);
	OUT_RING_CACHE(sfactorA);	/* FIXME, sometimes has |0x4000 */
	OUT_RING_CACHE(dfactorA);	/* FIXME, sometimes has |0x4000 */
}

static void nv50Clear(GLcontext *ctx, GLbitfield mask)
{
	nouveauContextPtr nmesa = NOUVEAU_CONTEXT(ctx);
	GLuint hw_bufs = 0;

	if (mask & (BUFFER_BIT_FRONT_LEFT | BUFFER_BIT_BACK_LEFT))
		hw_bufs |= 0x3c;
	if (mask & (BUFFER_BIT_STENCIL))
		hw_bufs |= 0x02;
	if (mask & (BUFFER_BIT_DEPTH))
		hw_bufs |= 0x01;

	if (hw_bufs) {
		BEGIN_RING_SIZE(NvSub3D, NV50_TCL_PRIMITIVE_3D_CLEAR_BUFFERS, 1);
		OUT_RING(hw_bufs);
	}
}

static void nv50ClearColor(GLcontext *ctx, const GLfloat color[4])
{
	nouveauContextPtr nmesa = NOUVEAU_CONTEXT(ctx);

	BEGIN_RING_CACHE(NvSub3D, NV50_TCL_PRIMITIVE_3D_CLEAR_COLOR_R, 4);
	OUT_RING_CACHEf(color[0]);
	OUT_RING_CACHEf(color[1]);
	OUT_RING_CACHEf(color[2]);
	OUT_RING_CACHEf(color[3]);
}

static void nv50ClearDepth(GLcontext *ctx, GLclampd d)
{
	nouveauContextPtr nmesa = NOUVEAU_CONTEXT(ctx);

	BEGIN_RING_CACHE(NvSub3D, NV50_TCL_PRIMITIVE_3D_CLEAR_DEPTH, 1);
	OUT_RING_CACHEf(d);
}

/* we're don't support indexed buffers
   void (*ClearIndex)(GLcontext *ctx, GLuint index)
 */

static void nv50ClearStencil(GLcontext *ctx, GLint s)
{
	nouveauContextPtr nmesa = NOUVEAU_CONTEXT(ctx);

	BEGIN_RING_CACHE(NvSub3D, NV50_TCL_PRIMITIVE_3D_CLEAR_STENCIL, 1);
	OUT_RING_CACHE(s);
}

static void nv50ClipPlane(GLcontext *ctx, GLenum plane, const GLfloat *equation)
{
	/* Only using shaders */
}

static void nv50ColorMask(GLcontext *ctx, GLboolean rmask, GLboolean gmask,
		GLboolean bmask, GLboolean amask )
{
	nouveauContextPtr nmesa = NOUVEAU_CONTEXT(ctx);
	int i;

	BEGIN_RING_CACHE(NvSub3D, NV50_TCL_PRIMITIVE_3D_COLOR_MASK(0), 8);
	for (i=0; i<8; i++) {
		OUT_RING_CACHE(((amask && 0x01) << 12) | ((bmask && 0x01) << 8) | ((gmask && 0x01)<< 4) | ((rmask && 0x01) << 0));
	}
}

static void nv50ColorMaterial(GLcontext *ctx, GLenum face, GLenum mode)
{
	// TODO I need love
}

static void nv50CullFace(GLcontext *ctx, GLenum mode)
{
	nouveauContextPtr nmesa = NOUVEAU_CONTEXT(ctx);

	BEGIN_RING_CACHE(NvSub3D, NV50_TCL_PRIMITIVE_3D_CULL_FACE, 1);
	OUT_RING_CACHE(mode);
}

static void nv50FrontFace(GLcontext *ctx, GLenum mode)
{
	nouveauContextPtr nmesa = NOUVEAU_CONTEXT(ctx);
	BEGIN_RING_CACHE(NvSub3D, NV50_TCL_PRIMITIVE_3D_FRONT_FACE, 1);
	OUT_RING_CACHE(mode);
}

static void nv50DepthFunc(GLcontext *ctx, GLenum func)
{
	nouveauContextPtr nmesa = NOUVEAU_CONTEXT(ctx);
	BEGIN_RING_CACHE(NvSub3D, NV50_TCL_PRIMITIVE_3D_DEPTH_FUNC, 1);
	OUT_RING_CACHE(func);
}

static void nv50DepthMask(GLcontext *ctx, GLboolean flag)
{
	nouveauContextPtr nmesa = NOUVEAU_CONTEXT(ctx);
	BEGIN_RING_CACHE(NvSub3D, NV50_TCL_PRIMITIVE_3D_DEPTH_WRITE_ENABLE, 1);
	OUT_RING_CACHE(flag);
}

static void nv50DepthRange(GLcontext *ctx, GLclampd nearval, GLclampd farval)
{
	nouveauContextPtr nmesa = NOUVEAU_CONTEXT(ctx);
	BEGIN_RING_CACHE(NvSub3D, NV50_TCL_PRIMITIVE_3D_DEPTH_RANGE_NEAR, 2);
	OUT_RING_CACHEf(nearval);
	OUT_RING_CACHEf(farval);
}

/** Specify the current buffer for writing */
//void (*DrawBuffer)( GLcontext *ctx, GLenum buffer );
/** Specify the buffers for writing for fragment programs*/
//void (*DrawBuffers)( GLcontext *ctx, GLsizei n, const GLenum *buffers );

static void nv50Enable(GLcontext *ctx, GLenum cap, GLboolean state)
{
	nouveauContextPtr nmesa = NOUVEAU_CONTEXT(ctx);
	switch(cap)
	{
		case GL_ALPHA_TEST:
			BEGIN_RING_CACHE(NvSub3D, NV50_TCL_PRIMITIVE_3D_ALPHA_FUNC_ENABLE, 1);
			OUT_RING_CACHE(state);
			break;
//		case GL_AUTO_NORMAL:
//		case GL_BLEND:
//		case GL_CLIP_PLANE0:
//		case GL_CLIP_PLANE1:
//		case GL_CLIP_PLANE2:
//		case GL_CLIP_PLANE3:
//		case GL_CLIP_PLANE4:
//		case GL_CLIP_PLANE5:
		case GL_COLOR_LOGIC_OP:
			BEGIN_RING_CACHE(NvSub3D, NV50_TCL_PRIMITIVE_3D_LOGIC_OP_ENABLE, 1);
			OUT_RING_CACHE(state);
			break;
//		case GL_COLOR_MATERIAL:
//		case GL_COLOR_SUM_EXT:
//		case GL_COLOR_TABLE:
//		case GL_CONVOLUTION_1D:
//		case GL_CONVOLUTION_2D:
		case GL_CULL_FACE:
			BEGIN_RING_CACHE(NvSub3D, NV50_TCL_PRIMITIVE_3D_CULL_FACE_ENABLE, 1);
			OUT_RING_CACHE(state);
			break;
		case GL_DEPTH_TEST:
			BEGIN_RING_CACHE(NvSub3D, NV50_TCL_PRIMITIVE_3D_DEPTH_TEST_ENABLE, 1);
			OUT_RING_CACHE(state);
			break;
//		case GL_DITHER:
//		case GL_FOG:
//		case GL_HISTOGRAM:
//		case GL_INDEX_LOGIC_OP:
//		case GL_LIGHT0:
//		case GL_LIGHT1:
//		case GL_LIGHT2:
//		case GL_LIGHT3:
//		case GL_LIGHT4:
//		case GL_LIGHT5:
//		case GL_LIGHT6:
//		case GL_LIGHT7:
//		case GL_LIGHTING:
		case GL_LINE_SMOOTH:
			BEGIN_RING_CACHE(NvSub3D, NV50_TCL_PRIMITIVE_3D_LINE_SMOOTH_ENABLE, 1);
			OUT_RING_CACHE(state);
			break;
		case GL_LINE_STIPPLE:
			BEGIN_RING_CACHE(NvSub3D, NV50_TCL_PRIMITIVE_3D_LINE_STIPPLE_ENABLE, 1);
			OUT_RING_CACHE(state);
			break;
//		case GL_MAP1_COLOR_4:
//		case GL_MAP1_INDEX:
//		case GL_MAP1_NORMAL:
//		case GL_MAP1_TEXTURE_COORD_1:
//		case GL_MAP1_TEXTURE_COORD_2:
//		case GL_MAP1_TEXTURE_COORD_3:
//		case GL_MAP1_TEXTURE_COORD_4:
//		case GL_MAP1_VERTEX_3:
//		case GL_MAP1_VERTEX_4:
//		case GL_MAP2_COLOR_4:
//		case GL_MAP2_INDEX:
//		case GL_MAP2_NORMAL:
//		case GL_MAP2_TEXTURE_COORD_1:
//		case GL_MAP2_TEXTURE_COORD_2:
//		case GL_MAP2_TEXTURE_COORD_3:
//		case GL_MAP2_TEXTURE_COORD_4:
//		case GL_MAP2_VERTEX_3:
//		case GL_MAP2_VERTEX_4:
//		case GL_MINMAX:
//		case GL_NORMALIZE:
//		case GL_POINT_SMOOTH:
		case GL_POLYGON_OFFSET_POINT:
			BEGIN_RING_CACHE(NvSub3D, NV50_TCL_PRIMITIVE_3D_POLYGON_OFFSET_POINT_ENABLE, 1);
			OUT_RING_CACHE(state);
			break;
		case GL_POLYGON_OFFSET_LINE:
			BEGIN_RING_CACHE(NvSub3D, NV50_TCL_PRIMITIVE_3D_POLYGON_OFFSET_LINE_ENABLE, 1);
			OUT_RING_CACHE(state);
			break;
		case GL_POLYGON_OFFSET_FILL:
			BEGIN_RING_CACHE(NvSub3D, NV50_TCL_PRIMITIVE_3D_POLYGON_OFFSET_FILL_ENABLE, 1);
			OUT_RING_CACHE(state);
			break;
		case GL_POLYGON_SMOOTH:
			BEGIN_RING_CACHE(NvSub3D, NV50_TCL_PRIMITIVE_3D_POLYGON_SMOOTH_ENABLE, 1);
			OUT_RING_CACHE(state);
			break;
		case GL_POLYGON_STIPPLE:
			BEGIN_RING_CACHE(NvSub3D, NV50_TCL_PRIMITIVE_3D_POLYGON_STIPPLE_ENABLE, 1);
			OUT_RING_CACHE(state);
			break;
//		case GL_POST_COLOR_MATRIX_COLOR_TABLE:
//		case GL_POST_CONVOLUTION_COLOR_TABLE:
//		case GL_RESCALE_NORMAL:
		case GL_SCISSOR_TEST:
			/* No enable bit, nv50Scissor will adjust to max range */
			ctx->Driver.Scissor(ctx, ctx->Scissor.X, ctx->Scissor.Y,
			      			 ctx->Scissor.Width, ctx->Scissor.Height);
			break;
//		case GL_SEPARABLE_2D:
		case GL_STENCIL_TEST:
			// TODO BACK and FRONT ?
			BEGIN_RING_CACHE(NvSub3D, NV50_TCL_PRIMITIVE_3D_STENCIL_FRONT_ENABLE, 1);
			OUT_RING_CACHE(state);
			BEGIN_RING_CACHE(NvSub3D, NV50_TCL_PRIMITIVE_3D_STENCIL_BACK_ENABLE, 1);
			OUT_RING_CACHE(state);
			break;
//		case GL_TEXTURE_GEN_Q:
//		case GL_TEXTURE_GEN_R:
//		case GL_TEXTURE_GEN_S:
//		case GL_TEXTURE_GEN_T:
//		case GL_TEXTURE_1D:
//		case GL_TEXTURE_2D:
//		case GL_TEXTURE_3D:
	}
}

static void nv50Fogfv(GLcontext *ctx, GLenum pname, const GLfloat *params)
{
	/* Only using shaders */
}
   
static void nv50Hint(GLcontext *ctx, GLenum target, GLenum mode)
{
	// TODO I need love (fog and line_smooth hints)
}

// void (*IndexMask)(GLcontext *ctx, GLuint mask);

static void nv50Lightfv(GLcontext *ctx, GLenum light, GLenum pname, const GLfloat *params )
{
	/* Only with shaders */
}

/** Set the lighting model parameters */
void (*LightModelfv)(GLcontext *ctx, GLenum pname, const GLfloat *params);


static void nv50LineStipple(GLcontext *ctx, GLint factor, GLushort pattern )
{
	nouveauContextPtr nmesa = NOUVEAU_CONTEXT(ctx);

	BEGIN_RING_CACHE(NvSub3D, NV50_TCL_PRIMITIVE_3D_LINE_STIPPLE_PATTERN, 1);
	OUT_RING_CACHE((pattern << 8) | factor);
}

static void nv50LineWidth(GLcontext *ctx, GLfloat width)
{
	nouveauContextPtr nmesa = NOUVEAU_CONTEXT(ctx);

	BEGIN_RING_CACHE(NvSub3D, NV50_TCL_PRIMITIVE_3D_LINE_WIDTH, 1);
	OUT_RING_CACHEf(width);
}

static void nv50LogicOpcode(GLcontext *ctx, GLenum opcode)
{
	nouveauContextPtr nmesa = NOUVEAU_CONTEXT(ctx);

	BEGIN_RING_CACHE(NvSub3D, NV50_TCL_PRIMITIVE_3D_LOGIC_OP_OP, 1);
	OUT_RING_CACHE(opcode);
}

static void nv50PointParameterfv(GLcontext *ctx, GLenum pname, const GLfloat *params)
{
	/*TODO: not sure what goes here. */
	nouveauContextPtr nmesa = NOUVEAU_CONTEXT(ctx);
	
}

/** Specify the diameter of rasterized points */
static void nv50PointSize(GLcontext *ctx, GLfloat size)
{
	nouveauContextPtr nmesa = NOUVEAU_CONTEXT(ctx);

	BEGIN_RING_CACHE(NvSub3D, NV50_TCL_PRIMITIVE_3D_POINT_SIZE, 1);
	OUT_RING_CACHEf(size);
}

/** Select a polygon rasterization mode */
static void nv50PolygonMode(GLcontext *ctx, GLenum face, GLenum mode)
{
	nouveauContextPtr nmesa = NOUVEAU_CONTEXT(ctx);

	if (face == GL_FRONT || face == GL_FRONT_AND_BACK) {
		BEGIN_RING_CACHE(NvSub3D, NV50_TCL_PRIMITIVE_3D_POLYGON_MODE_FRONT, 1);
		OUT_RING_CACHE(mode);
	}
	if (face == GL_BACK || face == GL_FRONT_AND_BACK) {
		BEGIN_RING_CACHE(NvSub3D, NV50_TCL_PRIMITIVE_3D_POLYGON_MODE_BACK, 1);
		OUT_RING_CACHE(mode);
	}
}

/** Set the scale and units used to calculate depth values */
static void nv50PolygonOffset(GLcontext *ctx, GLfloat factor, GLfloat units)
{
        nouveauContextPtr nmesa = NOUVEAU_CONTEXT(ctx);
 
        BEGIN_RING_CACHE(NvSub3D, NV50_TCL_PRIMITIVE_3D_POLYGON_OFFSET_FACTOR, 1);
        OUT_RING_CACHEf(factor);
        BEGIN_RING_CACHE(NvSub3D, NV50_TCL_PRIMITIVE_3D_POLYGON_OFFSET_UNITS, 1);
        OUT_RING_CACHEf(units);
}

/** Set the polygon stippling pattern */
static void nv50PolygonStipple(GLcontext *ctx, const GLubyte *mask )
{
        nouveauContextPtr nmesa = NOUVEAU_CONTEXT(ctx);
        BEGIN_RING_CACHE(NvSub3D, NV50_TCL_PRIMITIVE_3D_POLYGON_STIPPLE_PATTERN(0), 32);
        OUT_RING_CACHEp(mask, 32);
}

/* Specifies the current buffer for reading */
void (*ReadBuffer)( GLcontext *ctx, GLenum buffer );
/** Set rasterization mode */
void (*RenderMode)(GLcontext *ctx, GLenum mode );

/** Define the scissor box */
static void nv50Scissor(GLcontext *ctx, GLint x, GLint y, GLsizei w, GLsizei h)
{
        nouveauContextPtr nmesa = NOUVEAU_CONTEXT(ctx);

	/* There's no scissor enable bit, so adjust the scissor to cover the
	 * maximum draw buffer bounds
	 */
	if (!ctx->Scissor.Enabled) {
	   x = y = 0;
	   w = h = 8191;
	} else {
	   x += nmesa->drawX;
	   y += nmesa->drawY;
	}

        BEGIN_RING_CACHE(NvSub3D, NV50_TCL_PRIMITIVE_3D_SCISSOR_WIDTH_XPOS, 2);
        OUT_RING_CACHE(((w) << 16) | x);
        OUT_RING_CACHE(((h) << 16) | y);
}

/** Select flat or smooth shading */
static void nv50ShadeModel(GLcontext *ctx, GLenum mode)
{
	nouveauContextPtr nmesa = NOUVEAU_CONTEXT(ctx);

	BEGIN_RING_CACHE(NvSub3D, NV50_TCL_PRIMITIVE_3D_SHADE_MODEL, 1);
	OUT_RING_CACHE(mode);
}

/** OpenGL 2.0 two-sided StencilFunc */
static void nv50StencilFuncSeparate(GLcontext *ctx, GLenum face, GLenum func,
		GLint ref, GLuint mask)
{
	nouveauContextPtr nmesa = NOUVEAU_CONTEXT(ctx);

	if (face == GL_FRONT || face == GL_FRONT_AND_BACK) {
		BEGIN_RING_CACHE(NvSub3D, NV50_TCL_PRIMITIVE_3D_STENCIL_FRONT_FUNC_FUNC, 1);
		OUT_RING_CACHE(func);
		BEGIN_RING_CACHE(NvSub3D, NV50_TCL_PRIMITIVE_3D_STENCIL_FRONT_FUNC_REF, 1);
		OUT_RING_CACHE(ref);
		BEGIN_RING_CACHE(NvSub3D, NV50_TCL_PRIMITIVE_3D_STENCIL_FRONT_FUNC_MASK, 1);
		OUT_RING_CACHE(mask);
	}
	if (face == GL_BACK || face == GL_FRONT_AND_BACK) {
		BEGIN_RING_CACHE(NvSub3D, NV50_TCL_PRIMITIVE_3D_STENCIL_BACK_FUNC_FUNC, 2);
		OUT_RING_CACHE(func);
		OUT_RING_CACHE(ref);
		BEGIN_RING_CACHE(NvSub3D, NV50_TCL_PRIMITIVE_3D_STENCIL_BACK_FUNC_MASK, 1);
		OUT_RING_CACHE(mask);
	}
}

/** OpenGL 2.0 two-sided StencilMask */
static void nv50StencilMaskSeparate(GLcontext *ctx, GLenum face, GLuint mask)
{
	nouveauContextPtr nmesa = NOUVEAU_CONTEXT(ctx);

	if (face == GL_FRONT || face == GL_FRONT_AND_BACK) {
		BEGIN_RING_CACHE(NvSub3D, NV50_TCL_PRIMITIVE_3D_STENCIL_FRONT_MASK, 1);
		OUT_RING_CACHE(mask);
	}
	if (face == GL_BACK || face == GL_FRONT_AND_BACK) {
		BEGIN_RING_CACHE(NvSub3D, NV50_TCL_PRIMITIVE_3D_STENCIL_BACK_MASK, 1);
		OUT_RING_CACHE(mask);
	}
}

/** OpenGL 2.0 two-sided StencilOp */
static void nv50StencilOpSeparate(GLcontext *ctx, GLenum face, GLenum fail,
		GLenum zfail, GLenum zpass)
{
	nouveauContextPtr nmesa = NOUVEAU_CONTEXT(ctx);

	if (face == GL_FRONT || face == GL_FRONT_AND_BACK) {
		BEGIN_RING_CACHE(NvSub3D, NV50_TCL_PRIMITIVE_3D_STENCIL_FRONT_OP_FAIL, 3);
		OUT_RING_CACHE(fail);
		OUT_RING_CACHE(zfail);
		OUT_RING_CACHE(zpass);
	}
	if (face == GL_BACK || face == GL_FRONT_AND_BACK) {
		BEGIN_RING_CACHE(NvSub3D, NV50_TCL_PRIMITIVE_3D_STENCIL_BACK_OP_FAIL, 3);
		OUT_RING_CACHE(fail);
		OUT_RING_CACHE(zfail);
		OUT_RING_CACHE(zpass);
	}
}

/** Control the generation of texture coordinates */
void (*TexGen)(GLcontext *ctx, GLenum coord, GLenum pname,
		const GLfloat *params);
/** Set texture environment parameters */
void (*TexEnv)(GLcontext *ctx, GLenum target, GLenum pname,
		const GLfloat *param);
/** Set texture parameters */
void (*TexParameter)(GLcontext *ctx, GLenum target,
		struct gl_texture_object *texObj,
		GLenum pname, const GLfloat *params);

static void nv50TextureMatrix(GLcontext *ctx, GLuint unit, const GLmatrix *mat)
{
	/* Only with shaders */
}

static void nv50WindowMoved(nouveauContextPtr nmesa)
{
	GLcontext *ctx = nmesa->glCtx;
	GLfloat *v = nmesa->viewport.m;
	GLuint w = ctx->Viewport.Width;
	GLuint h = ctx->Viewport.Height;
	GLuint x = ctx->Viewport.X + nmesa->drawX;
	GLuint y = ctx->Viewport.Y + nmesa->drawY;
	int i;

	BEGIN_RING_CACHE(NvSub3D,
	      NV50_TCL_PRIMITIVE_3D_VIEWPORT_CLIP_HORIZ(0), 2);
        OUT_RING_CACHE((8191 << 16) | 0);
        OUT_RING_CACHE((8191 << 16) | 0);
	for (i=1; i<8; i++) {
		BEGIN_RING_CACHE(NvSub3D,
		      NV50_TCL_PRIMITIVE_3D_VIEWPORT_CLIP_HORIZ(i), 2);
        	OUT_RING_CACHE(0);
	        OUT_RING_CACHE(0);
	}

	ctx->Driver.Scissor(ctx, ctx->Scissor.X, ctx->Scissor.Y,
	      		    ctx->Scissor.Width, ctx->Scissor.Height);
}

static GLboolean nv50InitCard(nouveauContextPtr nmesa)
{
	int i,j;

	nouveauObjectOnSubchannel(nmesa, NvSub3D, Nv3D);

	BEGIN_RING_SIZE(NvSub3D, 0x1558, 1);
	OUT_RING(1);

	BEGIN_RING_SIZE(NvSub3D, NV50_TCL_PRIMITIVE_3D_SET_OBJECT_1(0), 8);
	for (i=0; i<8; i++) {
		OUT_RING(NvDmaFB);
	}

	BEGIN_RING_SIZE(NvSub3D, NV50_TCL_PRIMITIVE_3D_SET_OBJECT_0(0), 12);
	for (i=0; i<12; i++) {
		OUT_RING(NvDmaFB);
	}

	BEGIN_RING_SIZE(NvSub3D, 0x121c, 1);
	OUT_RING(1);

	for (i=0; i<8; i++) {
		BEGIN_RING_SIZE(NvSub3D, 0x0200 + (i*0x20), 5);
		for (j=0; j<5; j++) {
			OUT_RING(0);
		}
	}

	BEGIN_RING_SIZE(NvSub3D, 0x0fe0, 5);
	OUT_RING(0);
	OUT_RING(0);
	OUT_RING(0x16);
	OUT_RING(0);
	OUT_RING(0);

	return GL_FALSE;
}

static GLboolean nv50BindBuffers(nouveauContextPtr nmesa, int num_color,
      				 nouveau_renderbuffer **color,
				 nouveau_renderbuffer *depth)
{
	return GL_FALSE;
}

void nv50InitStateFuncs(GLcontext *ctx, struct dd_function_table *func)
{
	nouveauContextPtr nmesa = NOUVEAU_CONTEXT(ctx);

	func->AlphaFunc			= nv50AlphaFunc;
	func->BlendColor		= nv50BlendColor;
	func->BlendEquationSeparate	= nv50BlendEquationSeparate;
	func->BlendFuncSeparate		= nv50BlendFuncSeparate;
	func->Clear			= nv50Clear;
	func->ClearColor		= nv50ClearColor;
	func->ClearDepth		= nv50ClearDepth;
	func->ClearStencil		= nv50ClearStencil;
	func->ClipPlane			= nv50ClipPlane;
	func->ColorMask			= nv50ColorMask;
	func->ColorMaterial		= nv50ColorMaterial;
	func->CullFace			= nv50CullFace;
	func->FrontFace			= nv50FrontFace;
	func->DepthFunc			= nv50DepthFunc;
	func->DepthMask			= nv50DepthMask;
	func->DepthRange		= nv50DepthRange;
	func->Enable			= nv50Enable;
	func->Fogfv			= nv50Fogfv;
	func->Hint			= nv50Hint;
	func->Lightfv			= nv50Lightfv;
/*	func->LightModelfv		= nv50LightModelfv;	 */
	func->LineStipple		= nv50LineStipple;
	func->LineWidth			= nv50LineWidth;
	func->LogicOpcode		= nv50LogicOpcode;
	func->PointParameterfv		= nv50PointParameterfv;
	func->PointSize			= nv50PointSize;
	func->PolygonMode		= nv50PolygonMode;
	func->PolygonOffset		= nv50PolygonOffset;
	func->PolygonStipple		= nv50PolygonStipple;
/*	func->ReadBuffer		= nv50ReadBuffer;	*/
/*	func->RenderMode		= nv50RenderMode;	*/
	func->Scissor			= nv50Scissor;
	func->ShadeModel		= nv50ShadeModel;
	func->StencilFuncSeparate	= nv50StencilFuncSeparate;
	func->StencilMaskSeparate	= nv50StencilMaskSeparate;
	func->StencilOpSeparate		= nv50StencilOpSeparate;
/*	func->TexGen			= nv50TexGen;		*/
/*	func->TexParameter		= nv50TexParameter;	*/
	func->TextureMatrix		= nv50TextureMatrix;

	nmesa->hw_func.InitCard		= nv50InitCard;
	nmesa->hw_func.BindBuffers	= nv50BindBuffers;
	nmesa->hw_func.WindowMoved	= nv50WindowMoved;
}
