/**************************************************************************

Copyright 2007 Stephane Marchesin
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
#include "nouveau_msg.h"

#include "tnl/t_pipeline.h"

#include "mtypes.h"
#include "colormac.h"

static uint32_t nv04_compare_func(GLuint f)
{
	switch ( f ) {
		case GL_NEVER:		return 1;
		case GL_LESS:		return 2;
		case GL_EQUAL:		return 3;
		case GL_LEQUAL:		return 4;
		case GL_GREATER:	return 5;
		case GL_NOTEQUAL:	return 6;
		case GL_GEQUAL:		return 7;
		case GL_ALWAYS:		return 8;
	}
	WARN_ONCE("Unable to find the function\n");
	return 0;
}

static uint32_t nv04_blend_func(GLuint f)
{
	switch ( f ) {
		case GL_ZERO:			return 0x1;
		case GL_ONE:			return 0x2;
		case GL_SRC_COLOR:		return 0x3;
		case GL_ONE_MINUS_SRC_COLOR:	return 0x4;
		case GL_SRC_ALPHA:		return 0x5;
		case GL_ONE_MINUS_SRC_ALPHA:	return 0x6;
		case GL_DST_ALPHA:		return 0x7;
		case GL_ONE_MINUS_DST_ALPHA:	return 0x8;
		case GL_DST_COLOR:		return 0x9;
		case GL_ONE_MINUS_DST_COLOR:	return 0xA;
		case GL_SRC_ALPHA_SATURATE:	return 0xB;
	}
	WARN_ONCE("Unable to find the function 0x%x\n",f);
	return 0;
}

static void nv04_emit_control(GLcontext *ctx)
{
	nouveauContextPtr nmesa = NOUVEAU_CONTEXT(ctx);
	uint32_t control,cull;
	GLubyte alpha_ref;

	CLAMPED_FLOAT_TO_UBYTE(alpha_ref, ctx->Color.AlphaRef);
	control=alpha_ref;
	control|=(nv04_compare_func(ctx->Color.AlphaFunc)<<8);
	control|=(ctx->Color.AlphaEnabled<<12);
	control|=(1<<13);
	control|=(ctx->Depth.Test<<14);
	control|=(nv04_compare_func(ctx->Depth.Func)<<16);
	if ((ctx->Polygon.CullFlag)&&(ctx->Polygon.CullFaceMode!=GL_FRONT_AND_BACK))
	{
		if ((ctx->Polygon.FrontFace==GL_CW)&&(ctx->Polygon.CullFaceMode==GL_FRONT))
			cull=2;
		if ((ctx->Polygon.FrontFace==GL_CW)&&(ctx->Polygon.CullFaceMode==GL_BACK))
			cull=3;
		if ((ctx->Polygon.FrontFace==GL_CCW)&&(ctx->Polygon.CullFaceMode==GL_FRONT))
			cull=3;
		if ((ctx->Polygon.FrontFace==GL_CCW)&&(ctx->Polygon.CullFaceMode==GL_BACK))
			cull=2;
	}
	else
		if (ctx->Polygon.CullFaceMode==GL_FRONT_AND_BACK)
			cull=0;
		else
			cull=1;
	control|=(cull<<20);
	control|=(ctx->Color.DitherFlag<<22);
	if ((ctx->Depth.Test)&&(ctx->Depth.Mask))
		control|=(1<<24);

	control|=(1<<30); // integer zbuffer format

	BEGIN_RING_CACHE(NvSub3D, NV04_DX5_TEXTURED_TRIANGLE_CONTROL, 1);
	OUT_RING_CACHE(control);
}

static void nv04_emit_blend(GLcontext *ctx)
{
	nouveauContextPtr nmesa = NOUVEAU_CONTEXT(ctx);
	uint32_t blend;

	blend=0x4; // texture MODULATE_ALPHA
	blend|=0x20; // alpha is MSB
	switch(ctx->Light.ShadeModel) {
		case GL_SMOOTH:blend|=(1<<6);break;
		case GL_FLAT:  blend|=(2<<6);break;
		default:break;
	}
	if (ctx->Hint.PerspectiveCorrection!=GL_FASTEST)
		blend|=(1<<8);
	blend|=(ctx->Fog.Enabled<<16);
	blend|=(ctx->Color.BlendEnabled<<20);
	blend|=(nv04_blend_func(ctx->Color.BlendSrcRGB)<<24);
	blend|=(nv04_blend_func(ctx->Color.BlendDstRGB)<<28);

	BEGIN_RING_CACHE(NvSub3D, NV04_DX5_TEXTURED_TRIANGLE_BLEND, 1);
	OUT_RING_CACHE(blend);
}

static void nv04_emit_fog_color(GLcontext *ctx)
{
	nouveauContextPtr nmesa = NOUVEAU_CONTEXT(ctx);
	GLubyte c[4];
	c[0] = FLOAT_TO_UBYTE( ctx->Fog.Color[0] );
	c[1] = FLOAT_TO_UBYTE( ctx->Fog.Color[1] );
	c[2] = FLOAT_TO_UBYTE( ctx->Fog.Color[2] );
	c[3] = FLOAT_TO_UBYTE( ctx->Fog.Color[3] );
	BEGIN_RING_CACHE(NvSub3D, NV04_DX5_TEXTURED_TRIANGLE_FOG_COLOR, 1);
	OUT_RING_CACHE(PACK_COLOR_8888_REV(c[0],c[1],c[2],c[3]));
}

static void nv04AlphaFunc(GLcontext *ctx, GLenum func, GLfloat ref)
{
	nv04_emit_control(ctx);
}

static void nv04BlendColor(GLcontext *ctx, const GLfloat color[4])
{
	nv04_emit_blend(ctx);
}

static void nv04BlendEquationSeparate(GLcontext *ctx, GLenum modeRGB, GLenum modeA)
{
	nv04_emit_blend(ctx);
}


static void nv04BlendFuncSeparate(GLcontext *ctx, GLenum sfactorRGB, GLenum dfactorRGB,
		GLenum sfactorA, GLenum dfactorA)
{
	nv04_emit_blend(ctx);
}

static void nv04Clear(GLcontext *ctx, GLbitfield mask)
{
	/* TODO */
}

static void nv04ClearColor(GLcontext *ctx, const GLfloat color[4])
{
	/* TODO */
}

static void nv04ClearDepth(GLcontext *ctx, GLclampd d)
{
	/* TODO */
}

static void nv04ClearStencil(GLcontext *ctx, GLint s)
{
	/* TODO */
}

static void nv04ClipPlane(GLcontext *ctx, GLenum plane, const GLfloat *equation)
{
	/* TODO */
}

static void nv04ColorMask(GLcontext *ctx, GLboolean rmask, GLboolean gmask,
		GLboolean bmask, GLboolean amask )
{
	/* TODO */
}

static void nv04ColorMaterial(GLcontext *ctx, GLenum face, GLenum mode)
{
	/* TODO I need love */
}

static void nv04CullFace(GLcontext *ctx, GLenum mode)
{
	nv04_emit_control(ctx);
}

static void nv04FrontFace(GLcontext *ctx, GLenum mode)
{
	/* TODO */
}

static void nv04DepthFunc(GLcontext *ctx, GLenum func)
{
	nv04_emit_control(ctx);
}

static void nv04DepthMask(GLcontext *ctx, GLboolean flag)
{
	/* TODO */
}

static void nv04DepthRange(GLcontext *ctx, GLclampd nearval, GLclampd farval)
{
	/* TODO */
}

/** Specify the current buffer for writing */
//void (*DrawBuffer)( GLcontext *ctx, GLenum buffer );
/** Specify the buffers for writing for fragment programs*/
//void (*DrawBuffers)( GLcontext *ctx, GLsizei n, const GLenum *buffers );

static void nv04Enable(GLcontext *ctx, GLenum cap, GLboolean state)
{
	switch(cap)
	{
		case GL_ALPHA_TEST:
			nv04_emit_control(ctx);
			break;
//		case GL_AUTO_NORMAL:
		case GL_BLEND:
			nv04_emit_blend(ctx);
			break;
//		case GL_CLIP_PLANE0:
//		case GL_CLIP_PLANE1:
//		case GL_CLIP_PLANE2:
//		case GL_CLIP_PLANE3:
//		case GL_CLIP_PLANE4:
//		case GL_CLIP_PLANE5:
//		case GL_COLOR_LOGIC_OP:
//		case GL_COLOR_MATERIAL:
//		case GL_COLOR_SUM_EXT:
//		case GL_COLOR_TABLE:
//		case GL_CONVOLUTION_1D:
//		case GL_CONVOLUTION_2D:
		case GL_CULL_FACE:
			nv04_emit_control(ctx);
			break;
		case GL_DEPTH_TEST:
			nv04_emit_control(ctx);
			break;
		case GL_DITHER:
			nv04_emit_control(ctx);
			break;
		case GL_FOG:
			nv04_emit_blend(ctx);
			nv04_emit_fog_color(ctx);
			break;
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
//		case GL_LINE_SMOOTH:
//		case GL_LINE_STIPPLE:
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
//		case GL_POLYGON_OFFSET_POINT:
//		case GL_POLYGON_OFFSET_LINE:
//		case GL_POLYGON_OFFSET_FILL:
//		case GL_POLYGON_SMOOTH:
//		case GL_POLYGON_STIPPLE:
//		case GL_POST_COLOR_MATRIX_COLOR_TABLE:
//		case GL_POST_CONVOLUTION_COLOR_TABLE:
//		case GL_RESCALE_NORMAL:
//		case GL_SCISSOR_TEST:
//		case GL_SEPARABLE_2D:
//		case GL_STENCIL_TEST:
//		case GL_TEXTURE_GEN_Q:
//		case GL_TEXTURE_GEN_R:
//		case GL_TEXTURE_GEN_S:
//		case GL_TEXTURE_GEN_T:
//		case GL_TEXTURE_1D:
//		case GL_TEXTURE_2D:
//		case GL_TEXTURE_3D:
	}
}

static void nv04Fogfv(GLcontext *ctx, GLenum pname, const GLfloat *params)
{
	nv04_emit_blend(ctx);
	nv04_emit_fog_color(ctx);
}
   
static void nv04Hint(GLcontext *ctx, GLenum target, GLenum mode)
{
	switch(target)
	{
		case GL_PERSPECTIVE_CORRECTION_HINT:nv04_emit_blend(ctx);break;
		default:break;
	}
}

static void nv04LineStipple(GLcontext *ctx, GLint factor, GLushort pattern )
{
	/* TODO not even in your dreams */
}

static void nv04LineWidth(GLcontext *ctx, GLfloat width)
{
	/* TODO */
}

static void nv04LogicOpcode(GLcontext *ctx, GLenum opcode)
{
	/* TODO */
}

static void nv04PointParameterfv(GLcontext *ctx, GLenum pname, const GLfloat *params)
{
	/* TODO */
}

static void nv04PointSize(GLcontext *ctx, GLfloat size)
{
	/* TODO */
}

static void nv04PolygonMode(GLcontext *ctx, GLenum face, GLenum mode)
{
	/* TODO */
}

/** Set the scale and units used to calculate depth values */
static void nv04PolygonOffset(GLcontext *ctx, GLfloat factor, GLfloat units)
{
	/* TODO */
}

/** Set the polygon stippling pattern */
static void nv04PolygonStipple(GLcontext *ctx, const GLubyte *mask )
{
	/* TODO */
}

/* Specifies the current buffer for reading */
void (*ReadBuffer)( GLcontext *ctx, GLenum buffer );
/** Set rasterization mode */
void (*RenderMode)(GLcontext *ctx, GLenum mode );

/** Define the scissor box */
static void nv04Scissor(GLcontext *ctx, GLint x, GLint y, GLsizei w, GLsizei h)
{
	/* TODO */
}

/** Select flat or smooth shading */
static void nv04ShadeModel(GLcontext *ctx, GLenum mode)
{
	nv04_emit_blend(ctx);
}

/** OpenGL 2.0 two-sided StencilFunc */
static void nv04StencilFuncSeparate(GLcontext *ctx, GLenum face, GLenum func,
		GLint ref, GLuint mask)
{
	/* TODO */
}

/** OpenGL 2.0 two-sided StencilMask */
static void nv04StencilMaskSeparate(GLcontext *ctx, GLenum face, GLuint mask)
{
	/* TODO */
}

/** OpenGL 2.0 two-sided StencilOp */
static void nv04StencilOpSeparate(GLcontext *ctx, GLenum face, GLenum fail,
		GLenum zfail, GLenum zpass)
{
	/* TODO */
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

/* Update anything that depends on the window position/size */
static void nv04WindowMoved(nouveauContextPtr nmesa)
{
}

/* Initialise any card-specific non-GL related state */
static GLboolean nv04InitCard(nouveauContextPtr nmesa)
{
	nouveauObjectOnSubchannel(nmesa, NvSub3D, Nv3D);
	nouveauObjectOnSubchannel(nmesa, NvSubCtxSurf3D, NvCtxSurf3D);

	BEGIN_RING_SIZE(NvSubCtxSurf3D, NV04_CONTEXT_SURFACES_3D_DMA_NOTIFY, 3);
	OUT_RING(NvDmaFB);
	OUT_RING(NvDmaFB);
	OUT_RING(NvDmaFB);
	BEGIN_RING_SIZE(NvSub3D, NV04_DX5_TEXTURED_TRIANGLE_SURFACE, 1);
	OUT_RING(NvCtxSurf3D);
	return GL_TRUE;
}

/* Update buffer offset/pitch/format */
static GLboolean nv04BindBuffers(nouveauContextPtr nmesa, int num_color,
		nouveau_renderbuffer **color,
		nouveau_renderbuffer *depth)
{
	GLuint x, y, w, h;
	uint32_t depth_pitch=(depth?depth->pitch:0+15)&~15+16;
	if (depth_pitch<256) depth_pitch=256;

	w = color[0]->mesa.Width;
	h = color[0]->mesa.Height;
	x = nmesa->drawX;
	y = nmesa->drawY;

	BEGIN_RING_SIZE(NvSubCtxSurf3D, NV04_CONTEXT_SURFACES_3D_FORMAT, 1);
	if (color[0]->mesa._ActualFormat == GL_RGBA8)
		OUT_RING(0x108/*A8R8G8B8*/);
	else
		OUT_RING(0x103/*R5G6B5*/);

	/* FIXME pitches have to be aligned ! */
	BEGIN_RING_SIZE(NvSubCtxSurf3D, NV04_CONTEXT_SURFACES_3D_PITCH, 2);
	OUT_RING(color[0]->pitch|(depth_pitch<<16));
	OUT_RING(color[0]->offset);
	if (depth) {
		BEGIN_RING_SIZE(NvSubCtxSurf3D, NV04_CONTEXT_SURFACES_3D_OFFSET_ZETA, 1);
		OUT_RING(depth->offset);
	}

//	BEGIN_RING_SIZE(NvSubCtxSurf3D, NV04_CONTEXT_SURFACES_3D_CLIP_HORIZONTAL, 2);
//	OUT_RING((w<<16)|x);
//	OUT_RING((h<<16)|y);


	/* FIXME not sure... */
/*	BEGIN_RING_SIZE(NvSubCtxSurf3D, NV04_CONTEXT_SURFACES_3D_CLIP_SIZE, 1);
	OUT_RING((h<<16)|w);*/

	return GL_TRUE;
}

void nv04InitStateFuncs(GLcontext *ctx, struct dd_function_table *func)
{
	nouveauContextPtr nmesa = NOUVEAU_CONTEXT(ctx);

	func->AlphaFunc			= nv04AlphaFunc;
	func->BlendColor		= nv04BlendColor;
	func->BlendEquationSeparate	= nv04BlendEquationSeparate;
	func->BlendFuncSeparate		= nv04BlendFuncSeparate;
	func->Clear			= nv04Clear;
	func->ClearColor		= nv04ClearColor;
	func->ClearDepth		= nv04ClearDepth;
	func->ClearStencil		= nv04ClearStencil;
	func->ClipPlane			= nv04ClipPlane;
	func->ColorMask			= nv04ColorMask;
	func->ColorMaterial		= nv04ColorMaterial;
	func->CullFace			= nv04CullFace;
	func->FrontFace			= nv04FrontFace;
	func->DepthFunc			= nv04DepthFunc;
	func->DepthMask			= nv04DepthMask;
	func->DepthRange		= nv04DepthRange;
	func->Enable			= nv04Enable;
	func->Fogfv			= nv04Fogfv;
	func->Hint			= nv04Hint;
/*	func->Lightfv			= nv04Lightfv;*/
/*	func->LightModelfv		= nv04LightModelfv; */
	func->LineStipple		= nv04LineStipple;		/* Not for NV04 */
	func->LineWidth			= nv04LineWidth;
	func->LogicOpcode		= nv04LogicOpcode;
	func->PointParameterfv		= nv04PointParameterfv;
	func->PointSize			= nv04PointSize;
	func->PolygonMode		= nv04PolygonMode;
	func->PolygonOffset		= nv04PolygonOffset;
	func->PolygonStipple		= nv04PolygonStipple;		/* Not for NV04 */
/*	func->ReadBuffer		= nv04ReadBuffer;*/
/*	func->RenderMode		= nv04RenderMode;*/
	func->Scissor			= nv04Scissor;
	func->ShadeModel		= nv04ShadeModel;
	func->StencilFuncSeparate	= nv04StencilFuncSeparate;
	func->StencilMaskSeparate	= nv04StencilMaskSeparate;
	func->StencilOpSeparate		= nv04StencilOpSeparate;
/*	func->TexGen			= nv04TexGen;*/
/*	func->TexParameter		= nv04TexParameter;*/
/*	func->TextureMatrix		= nv04TextureMatrix;*/

	nmesa->hw_func.InitCard		= nv04InitCard;
	nmesa->hw_func.BindBuffers	= nv04BindBuffers;
	nmesa->hw_func.WindowMoved	= nv04WindowMoved;
}
