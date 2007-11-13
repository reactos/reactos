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

#include "tnl/t_pipeline.h"

#include "mtypes.h"
#include "colormac.h"

static void nv10ViewportScale(nouveauContextPtr nmesa)
{
	GLcontext *ctx = nmesa->glCtx;
	GLuint w = ctx->Viewport.Width;
	GLuint h = ctx->Viewport.Height;

	GLfloat max_depth = (ctx->Viewport.Near + ctx->Viewport.Far) * 0.5;
/*	if (ctx->DrawBuffer) {
		switch (ctx->DrawBuffer->_DepthBuffer->DepthBits) {
			case 16:
				max_depth *= 32767.0;
				break;
			case 24:
				max_depth *= 16777215.0;
				break;
		}
	} else {*/
		/* Default to 24 bits range */	
		max_depth *= 16777215.0;
/*	}*/

	BEGIN_RING_CACHE(NvSub3D, NV10_TCL_PRIMITIVE_3D_VIEWPORT_SCALE_X, 4);
	OUT_RING_CACHEf ((((GLfloat) w) * 0.5) - 2048.0);
	OUT_RING_CACHEf ((((GLfloat) h) * 0.5) - 2048.0);
	OUT_RING_CACHEf (max_depth);
	OUT_RING_CACHEf (0.0);
}

static void nv10AlphaFunc(GLcontext *ctx, GLenum func, GLfloat ref)
{
	nouveauContextPtr nmesa = NOUVEAU_CONTEXT(ctx);
	GLubyte ubRef;
	CLAMPED_FLOAT_TO_UBYTE(ubRef, ref);

	BEGIN_RING_CACHE(NvSub3D, NV10_TCL_PRIMITIVE_3D_ALPHA_FUNC_FUNC, 2);
	OUT_RING_CACHE(func);
	OUT_RING_CACHE(ubRef);
}

static void nv10BlendColor(GLcontext *ctx, const GLfloat color[4])
{
	nouveauContextPtr nmesa = NOUVEAU_CONTEXT(ctx); 
	GLubyte cf[4];

	CLAMPED_FLOAT_TO_UBYTE(cf[0], color[0]);
	CLAMPED_FLOAT_TO_UBYTE(cf[1], color[1]);
	CLAMPED_FLOAT_TO_UBYTE(cf[2], color[2]);
	CLAMPED_FLOAT_TO_UBYTE(cf[3], color[3]);

	BEGIN_RING_CACHE(NvSub3D, NV10_TCL_PRIMITIVE_3D_BLEND_COLOR, 1);
	OUT_RING_CACHE(PACK_COLOR_8888(cf[3], cf[1], cf[2], cf[0]));
}

static void nv10BlendEquationSeparate(GLcontext *ctx, GLenum modeRGB, GLenum modeA)
{
	nouveauContextPtr nmesa = NOUVEAU_CONTEXT(ctx);

	assert( modeRGB == modeA );

	BEGIN_RING_CACHE(NvSub3D, NV10_TCL_PRIMITIVE_3D_BLEND_EQUATION, 1);
	OUT_RING_CACHE(modeRGB);
}


static void nv10BlendFuncSeparate(GLcontext *ctx, GLenum sfactorRGB, GLenum dfactorRGB,
		GLenum sfactorA, GLenum dfactorA)
{
	nouveauContextPtr nmesa = NOUVEAU_CONTEXT(ctx);

	assert( sfactorRGB == sfactorA );
	assert( dfactorRGB == dfactorA );

	BEGIN_RING_CACHE(NvSub3D, NV10_TCL_PRIMITIVE_3D_BLEND_FUNC_SRC, 2);
	OUT_RING_CACHE(sfactorRGB);
	OUT_RING_CACHE(dfactorRGB);
}

static void nv10Clear(GLcontext *ctx, GLbitfield mask)
{
	/* TODO */
}

static void nv10ClearColor(GLcontext *ctx, const GLfloat color[4])
{
	nouveauContextPtr nmesa = NOUVEAU_CONTEXT(ctx);
	GLubyte c[4];
	UNCLAMPED_FLOAT_TO_RGBA_CHAN(c,color);
	nmesa->clear_color_value = PACK_COLOR_8888(c[3],c[0],c[1],c[2]);
}

static void nv10ClearDepth(GLcontext *ctx, GLclampd d)
{
	nouveauContextPtr nmesa = NOUVEAU_CONTEXT(ctx);

/*	switch (ctx->DrawBuffer->_DepthBuffer->DepthBits) {
		case 16:
			nmesa->clear_value = (uint32_t)(d*0x7FFF);
			break;
		case 24:*/
			nmesa->clear_value = ((nmesa->clear_value&0x000000FF) |
				(((uint32_t)(d*0xFFFFFF))<<8));
/*			break;
	}*/
}

static void nv10ClearStencil(GLcontext *ctx, GLint s)
{
	nouveauContextPtr nmesa = NOUVEAU_CONTEXT(ctx);

/*	if (ctx->DrawBuffer->_DepthBuffer->DepthBits == 24) {*/
		nmesa->clear_value = ((nmesa->clear_value&0xFFFFFF00)|
			(s&0x000000FF));
/*	}*/
}

static void nv10ClipPlane(GLcontext *ctx, GLenum plane, const GLfloat *equation)
{
	nouveauContextPtr nmesa = NOUVEAU_CONTEXT(ctx);
	BEGIN_RING_CACHE(NvSub3D, NV10_TCL_PRIMITIVE_3D_CLIP_PLANE_A(plane), 4);
	OUT_RING_CACHEf(equation[0]);
	OUT_RING_CACHEf(equation[1]);
	OUT_RING_CACHEf(equation[2]);
	OUT_RING_CACHEf(equation[3]);
}

static void nv10ColorMask(GLcontext *ctx, GLboolean rmask, GLboolean gmask,
		GLboolean bmask, GLboolean amask )
{
	nouveauContextPtr nmesa = NOUVEAU_CONTEXT(ctx);
	BEGIN_RING_CACHE(NvSub3D, NV10_TCL_PRIMITIVE_3D_COLOR_MASK, 1);
	OUT_RING_CACHE(((amask && 0x01) << 24) | ((rmask && 0x01) << 16) | ((gmask && 0x01)<< 8) | ((bmask && 0x01) << 0));
}

static void nv10ColorMaterial(GLcontext *ctx, GLenum face, GLenum mode)
{
	/* TODO I need love */
}

static void nv10CullFace(GLcontext *ctx, GLenum mode)
{
	nouveauContextPtr nmesa = NOUVEAU_CONTEXT(ctx);
	BEGIN_RING_CACHE(NvSub3D, NV10_TCL_PRIMITIVE_3D_CULL_FACE, 1);
	OUT_RING_CACHE(mode);
}

static void nv10FrontFace(GLcontext *ctx, GLenum mode)
{
	nouveauContextPtr nmesa = NOUVEAU_CONTEXT(ctx);
	BEGIN_RING_CACHE(NvSub3D, NV10_TCL_PRIMITIVE_3D_FRONT_FACE, 1);
	OUT_RING_CACHE(mode);
}

static void nv10DepthFunc(GLcontext *ctx, GLenum func)
{
	nouveauContextPtr nmesa = NOUVEAU_CONTEXT(ctx);
	BEGIN_RING_CACHE(NvSub3D, NV10_TCL_PRIMITIVE_3D_DEPTH_FUNC, 1);
	OUT_RING_CACHE(func);
}

static void nv10DepthMask(GLcontext *ctx, GLboolean flag)
{
	nouveauContextPtr nmesa = NOUVEAU_CONTEXT(ctx);
	BEGIN_RING_CACHE(NvSub3D, NV10_TCL_PRIMITIVE_3D_DEPTH_WRITE_ENABLE, 1);
	OUT_RING_CACHE(flag);
}

static void nv10DepthRange(GLcontext *ctx, GLclampd nearval, GLclampd farval)
{
	nouveauContextPtr nmesa = NOUVEAU_CONTEXT(ctx);

	GLfloat depth_scale = 16777216.0;
	if (ctx->DrawBuffer->_DepthBuffer->DepthBits == 16) {
		depth_scale = 32768.0;
	}

	BEGIN_RING_CACHE(NvSub3D, NV10_TCL_PRIMITIVE_3D_DEPTH_RANGE_NEAR, 2);
	OUT_RING_CACHEf(nearval * depth_scale);
	OUT_RING_CACHEf(farval * depth_scale);

	nv10ViewportScale(nmesa);
}

/** Specify the current buffer for writing */
//void (*DrawBuffer)( GLcontext *ctx, GLenum buffer );
/** Specify the buffers for writing for fragment programs*/
//void (*DrawBuffers)( GLcontext *ctx, GLsizei n, const GLenum *buffers );

static void nv10Enable(GLcontext *ctx, GLenum cap, GLboolean state)
{
	nouveauContextPtr nmesa = NOUVEAU_CONTEXT(ctx);
	switch(cap)
	{
		case GL_ALPHA_TEST:
			BEGIN_RING_CACHE(NvSub3D, NV10_TCL_PRIMITIVE_3D_ALPHA_FUNC_ENABLE, 1);
			OUT_RING_CACHE(state);
			break;
//		case GL_AUTO_NORMAL:
		case GL_BLEND:
			BEGIN_RING_CACHE(NvSub3D, NV10_TCL_PRIMITIVE_3D_BLEND_FUNC_ENABLE, 1);
			OUT_RING_CACHE(state);
			break;
		case GL_CLIP_PLANE0:
		case GL_CLIP_PLANE1:
		case GL_CLIP_PLANE2:
		case GL_CLIP_PLANE3:
		case GL_CLIP_PLANE4:
		case GL_CLIP_PLANE5:
			BEGIN_RING_CACHE(NvSub3D, NV10_TCL_PRIMITIVE_3D_CLIP_PLANE_ENABLE(cap-GL_CLIP_PLANE0), 1);
			OUT_RING_CACHE(state);
			break;
		case GL_COLOR_LOGIC_OP:
			BEGIN_RING_CACHE(NvSub3D, NV10_TCL_PRIMITIVE_3D_COLOR_LOGIC_OP_ENABLE, 1);
			OUT_RING_CACHE(state);
			break;
//		case GL_COLOR_MATERIAL:
//		case GL_COLOR_SUM_EXT:
//		case GL_COLOR_TABLE:
//		case GL_CONVOLUTION_1D:
//		case GL_CONVOLUTION_2D:
		case GL_CULL_FACE:
			BEGIN_RING_CACHE(NvSub3D, NV10_TCL_PRIMITIVE_3D_CULL_FACE_ENABLE, 1);
			OUT_RING_CACHE(state);
			break;
		case GL_DEPTH_TEST:
			BEGIN_RING_CACHE(NvSub3D, NV10_TCL_PRIMITIVE_3D_DEPTH_TEST_ENABLE, 1);
			OUT_RING_CACHE(state);
			break;
		case GL_DITHER:
			BEGIN_RING_CACHE(NvSub3D, NV10_TCL_PRIMITIVE_3D_DITHER_ENABLE, 1);
			OUT_RING_CACHE(state);
			break;
		case GL_FOG:
			BEGIN_RING_CACHE(NvSub3D, NV10_TCL_PRIMITIVE_3D_FOG_ENABLE, 1);
			OUT_RING_CACHE(state);
			break;
//		case GL_HISTOGRAM:
//		case GL_INDEX_LOGIC_OP:
		case GL_LIGHT0:
		case GL_LIGHT1:
		case GL_LIGHT2:
		case GL_LIGHT3:
		case GL_LIGHT4:
		case GL_LIGHT5:
		case GL_LIGHT6:
		case GL_LIGHT7:
			{
			uint32_t mask=1<<(2*(cap-GL_LIGHT0));
			nmesa->enabled_lights=((nmesa->enabled_lights&mask)|(mask*state));
			if (nmesa->lighting_enabled)
			{
				BEGIN_RING_CACHE(NvSub3D, NV10_TCL_PRIMITIVE_3D_ENABLED_LIGHTS, 1);
				OUT_RING_CACHE(nmesa->enabled_lights);
			}
			break;
			}
		case GL_LIGHTING:
			nmesa->lighting_enabled=state;
			BEGIN_RING_CACHE(NvSub3D, NV10_TCL_PRIMITIVE_3D_ENABLED_LIGHTS, 1);
			if (nmesa->lighting_enabled)
				OUT_RING_CACHE(nmesa->enabled_lights);
			else
				OUT_RING_CACHE(0x0);
			break;
		case GL_LINE_SMOOTH:
			BEGIN_RING_CACHE(NvSub3D, NV10_TCL_PRIMITIVE_3D_LINE_SMOOTH_ENABLE, 1);
			OUT_RING_CACHE(state);
			break;
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
		case GL_NORMALIZE:
			BEGIN_RING_CACHE(NvSub3D, NV10_TCL_PRIMITIVE_3D_NORMALIZE_ENABLE, 1);
			OUT_RING_CACHE(state);
			break;
//		case GL_POINT_SMOOTH:
		case GL_POLYGON_OFFSET_POINT:
			BEGIN_RING_CACHE(NvSub3D, NV10_TCL_PRIMITIVE_3D_POLYGON_OFFSET_POINT_ENABLE, 1);
			OUT_RING_CACHE(state);
			break;
		case GL_POLYGON_OFFSET_LINE:
			BEGIN_RING_CACHE(NvSub3D, NV10_TCL_PRIMITIVE_3D_POLYGON_OFFSET_LINE_ENABLE, 1);
			OUT_RING_CACHE(state);
			break;
		case GL_POLYGON_OFFSET_FILL:
			BEGIN_RING_CACHE(NvSub3D, NV10_TCL_PRIMITIVE_3D_POLYGON_OFFSET_FILL_ENABLE, 1);
			OUT_RING_CACHE(state);
			break;
		case GL_POLYGON_SMOOTH:
			BEGIN_RING_CACHE(NvSub3D, NV10_TCL_PRIMITIVE_3D_POLYGON_SMOOTH_ENABLE, 1);
			OUT_RING_CACHE(state);
			break;
//		case GL_POLYGON_STIPPLE:
//		case GL_POST_COLOR_MATRIX_COLOR_TABLE:
//		case GL_POST_CONVOLUTION_COLOR_TABLE:
//		case GL_RESCALE_NORMAL:
//		case GL_SCISSOR_TEST:
//		case GL_SEPARABLE_2D:
		case GL_STENCIL_TEST:
			// TODO BACK and FRONT ?
			BEGIN_RING_CACHE(NvSub3D, NV10_TCL_PRIMITIVE_3D_STENCIL_ENABLE, 1);
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

static void nv10Fogfv(GLcontext *ctx, GLenum pname, const GLfloat *params)
{
    nouveauContextPtr nmesa = NOUVEAU_CONTEXT(ctx);
    switch(pname)
    {
        case GL_FOG_MODE:
            //BEGIN_RING_CACHE(NvSub3D, NV10_TCL_PRIMITIVE_3D_FOG_MODE, 1);
            //OUT_RING_CACHE (params);
            break;
            /* TODO: unsure about the rest.*/
        default:
            break;
    }

}
   
static void nv10Hint(GLcontext *ctx, GLenum target, GLenum mode)
{
	/* TODO I need love (fog and line_smooth hints) */
}

// void (*IndexMask)(GLcontext *ctx, GLuint mask);

enum {
	SPOTLIGHT_NO_UPDATE,
	SPOTLIGHT_UPDATE_EXPONENT,
	SPOTLIGHT_UPDATE_DIRECTION,
	SPOTLIGHT_UPDATE_ALL
};

static void nv10Lightfv(GLcontext *ctx, GLenum light, GLenum pname, const GLfloat *params )
{
	nouveauContextPtr nmesa = NOUVEAU_CONTEXT(ctx);
	GLint p = light - GL_LIGHT0;
	struct gl_light *l = &ctx->Light.Light[p];
	int spotlight_update = SPOTLIGHT_NO_UPDATE;

	switch(pname)
	{
		case GL_AMBIENT:
			BEGIN_RING_CACHE(NvSub3D, NV10_TCL_PRIMITIVE_3D_LIGHT_FRONT_SIDE_PRODUCT_AMBIENT_R(p), 3);
			OUT_RING_CACHEf(params[0]);
			OUT_RING_CACHEf(params[1]);
			OUT_RING_CACHEf(params[2]);
			break;
		case GL_DIFFUSE:
			BEGIN_RING_CACHE(NvSub3D, NV10_TCL_PRIMITIVE_3D_LIGHT_FRONT_SIDE_PRODUCT_DIFFUSE_R(p), 3);
			OUT_RING_CACHEf(params[0]);
			OUT_RING_CACHEf(params[1]);
			OUT_RING_CACHEf(params[2]);
			break;
		case GL_SPECULAR:
			BEGIN_RING_CACHE(NvSub3D, NV10_TCL_PRIMITIVE_3D_LIGHT_FRONT_SIDE_PRODUCT_SPECULAR_R(p), 3);
			OUT_RING_CACHEf(params[0]);
			OUT_RING_CACHEf(params[1]);
			OUT_RING_CACHEf(params[2]);
			break;
		case GL_POSITION:
			BEGIN_RING_CACHE(NvSub3D, NV10_TCL_PRIMITIVE_3D_LIGHT_POSITION_X(p), 3);
			OUT_RING_CACHEf(params[0]);
			OUT_RING_CACHEf(params[1]);
			OUT_RING_CACHEf(params[2]);
			break;
		case GL_SPOT_DIRECTION:
			spotlight_update = SPOTLIGHT_UPDATE_DIRECTION;
			break;
		case GL_SPOT_EXPONENT:
			spotlight_update = SPOTLIGHT_UPDATE_EXPONENT;
			break;
		case GL_SPOT_CUTOFF:
			spotlight_update = SPOTLIGHT_UPDATE_ALL;
			break;
		case GL_CONSTANT_ATTENUATION:
			BEGIN_RING_CACHE(NvSub3D, NV10_TCL_PRIMITIVE_3D_LIGHT_CONSTANT_ATTENUATION(p), 1);
			OUT_RING_CACHEf(*params);
			break;
		case GL_LINEAR_ATTENUATION:
			BEGIN_RING_CACHE(NvSub3D, NV10_TCL_PRIMITIVE_3D_LIGHT_LINEAR_ATTENUATION(p), 1);
			OUT_RING_CACHEf(*params);
			break;
		case GL_QUADRATIC_ATTENUATION:
			BEGIN_RING_CACHE(NvSub3D, NV10_TCL_PRIMITIVE_3D_LIGHT_QUADRATIC_ATTENUATION(p), 1);
			OUT_RING_CACHEf(*params);
			break;
		default:
			break;
	}

	switch(spotlight_update) {
		case SPOTLIGHT_UPDATE_DIRECTION:
			{
				GLfloat x,y,z;
				GLfloat spot_light_coef_a = 1.0 / (l->_CosCutoff - 1.0);
				x = spot_light_coef_a * l->_NormDirection[0];
				y = spot_light_coef_a * l->_NormDirection[1];
				z = spot_light_coef_a * l->_NormDirection[2];
				BEGIN_RING_CACHE(NvSub3D, NV10_TCL_PRIMITIVE_3D_LIGHT_SPOT_DIR_X(p), 3);
				OUT_RING_CACHEf(x);
				OUT_RING_CACHEf(y);
				OUT_RING_CACHEf(z);
			}
			break;
		case SPOTLIGHT_UPDATE_EXPONENT:
			{
				GLfloat cc,lc,qc;
				cc = 1.0;	/* FIXME: These need to be correctly computed */
				lc = 0.0;
				qc = 2.0;
				BEGIN_RING_CACHE(NvSub3D, NV10_TCL_PRIMITIVE_3D_LIGHT_SPOT_CUTOFF_A(p), 3);
				OUT_RING_CACHEf(cc);
				OUT_RING_CACHEf(lc);
				OUT_RING_CACHEf(qc);
			}
			break;
		case SPOTLIGHT_UPDATE_ALL:
			{
				GLfloat cc,lc,qc, x,y,z, c;
				GLfloat spot_light_coef_a = 1.0 / (l->_CosCutoff - 1.0);
				cc = 1.0;	/* FIXME: These need to be correctly computed */
				lc = 0.0;
				qc = 2.0;
				x = spot_light_coef_a * l->_NormDirection[0];
				y = spot_light_coef_a * l->_NormDirection[1];
				z = spot_light_coef_a * l->_NormDirection[2];
				c = spot_light_coef_a + 1.0;
				BEGIN_RING_CACHE(NvSub3D, NV10_TCL_PRIMITIVE_3D_LIGHT_SPOT_CUTOFF_A(p), 7);
				OUT_RING_CACHEf(cc);
				OUT_RING_CACHEf(lc);
				OUT_RING_CACHEf(qc);
				OUT_RING_CACHEf(x);
				OUT_RING_CACHEf(y);
				OUT_RING_CACHEf(z);
				OUT_RING_CACHEf(c);
			}
			break;
		default:
			break;
	}
}

/** Set the lighting model parameters */
static void (*LightModelfv)(GLcontext *ctx, GLenum pname, const GLfloat *params);


static void nv10LineStipple(GLcontext *ctx, GLint factor, GLushort pattern )
{
	/* Not for NV10 */
}

static void nv10LineWidth(GLcontext *ctx, GLfloat width)
{
	nouveauContextPtr nmesa = NOUVEAU_CONTEXT(ctx);
	BEGIN_RING_CACHE(NvSub3D, NV10_TCL_PRIMITIVE_3D_LINE_WIDTH, 1);
	OUT_RING_CACHE(((int) (width * 8.0)) & -4);
}

static void nv10LogicOpcode(GLcontext *ctx, GLenum opcode)
{
	nouveauContextPtr nmesa = NOUVEAU_CONTEXT(ctx);
	BEGIN_RING_CACHE(NvSub3D, NV10_TCL_PRIMITIVE_3D_COLOR_LOGIC_OP_OP, 1);
	OUT_RING_CACHE(opcode);
}

static void nv10PointParameterfv(GLcontext *ctx, GLenum pname, const GLfloat *params)
{
	/*TODO: not sure what goes here. */
	nouveauContextPtr nmesa = NOUVEAU_CONTEXT(ctx);
	
}

static void nv10PointSize(GLcontext *ctx, GLfloat size)
{
	nouveauContextPtr nmesa = NOUVEAU_CONTEXT(ctx);
	BEGIN_RING_CACHE(NvSub3D, NV10_TCL_PRIMITIVE_3D_POINT_SIZE, 1);
	OUT_RING_CACHE(((int) (size * 8.0)) & -4);
}

static void nv10PolygonMode(GLcontext *ctx, GLenum face, GLenum mode)
{
	nouveauContextPtr nmesa = NOUVEAU_CONTEXT(ctx);

	if (face == GL_FRONT || face == GL_FRONT_AND_BACK) {
		BEGIN_RING_CACHE(NvSub3D, NV10_TCL_PRIMITIVE_3D_POLYGON_MODE_FRONT, 1);
		OUT_RING_CACHE(mode);
	}
	if (face == GL_BACK || face == GL_FRONT_AND_BACK) {
		BEGIN_RING_CACHE(NvSub3D, NV10_TCL_PRIMITIVE_3D_POLYGON_MODE_BACK, 1);
		OUT_RING_CACHE(mode);
	}
}

/** Set the scale and units used to calculate depth values */
static void nv10PolygonOffset(GLcontext *ctx, GLfloat factor, GLfloat units)
{
        nouveauContextPtr nmesa = NOUVEAU_CONTEXT(ctx);
        BEGIN_RING_CACHE(NvSub3D, NV10_TCL_PRIMITIVE_3D_POLYGON_OFFSET_FACTOR, 2);
        OUT_RING_CACHEf(factor);
        OUT_RING_CACHEf(units);
}

/** Set the polygon stippling pattern */
static void nv10PolygonStipple(GLcontext *ctx, const GLubyte *mask )
{
	/* Not for NV10 */
}

/* Specifies the current buffer for reading */
void (*ReadBuffer)( GLcontext *ctx, GLenum buffer );
/** Set rasterization mode */
void (*RenderMode)(GLcontext *ctx, GLenum mode );

/** Define the scissor box */
static void nv10Scissor(GLcontext *ctx, GLint x, GLint y, GLsizei w, GLsizei h)
{
}

/** Select flat or smooth shading */
static void nv10ShadeModel(GLcontext *ctx, GLenum mode)
{
	nouveauContextPtr nmesa = NOUVEAU_CONTEXT(ctx);

	BEGIN_RING_CACHE(NvSub3D, NV10_TCL_PRIMITIVE_3D_SHADE_MODEL, 1);
	OUT_RING_CACHE(mode);
}

/** OpenGL 2.0 two-sided StencilFunc */
static void nv10StencilFuncSeparate(GLcontext *ctx, GLenum face, GLenum func,
		GLint ref, GLuint mask)
{
	nouveauContextPtr nmesa = NOUVEAU_CONTEXT(ctx);

	/* NV10 do not have separate FRONT and BACK stencils */
	BEGIN_RING_CACHE(NvSub3D, NV10_TCL_PRIMITIVE_3D_STENCIL_FUNC_FUNC, 3);
	OUT_RING_CACHE(func);
	OUT_RING_CACHE(ref);
	OUT_RING_CACHE(mask);
}

/** OpenGL 2.0 two-sided StencilMask */
static void nv10StencilMaskSeparate(GLcontext *ctx, GLenum face, GLuint mask)
{
	nouveauContextPtr nmesa = NOUVEAU_CONTEXT(ctx);

	/* NV10 do not have separate FRONT and BACK stencils */
	BEGIN_RING_CACHE(NvSub3D, NV10_TCL_PRIMITIVE_3D_STENCIL_MASK, 1);
	OUT_RING_CACHE(mask);
}

/** OpenGL 2.0 two-sided StencilOp */
static void nv10StencilOpSeparate(GLcontext *ctx, GLenum face, GLenum fail,
		GLenum zfail, GLenum zpass)
{
	nouveauContextPtr nmesa = NOUVEAU_CONTEXT(ctx);

	/* NV10 do not have separate FRONT and BACK stencils */
	BEGIN_RING_CACHE(NvSub3D, NV10_TCL_PRIMITIVE_3D_STENCIL_OP_FAIL, 3);
	OUT_RING_CACHE(fail);
	OUT_RING_CACHE(zfail);
	OUT_RING_CACHE(zpass);
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

static void nv10TextureMatrix(GLcontext *ctx, GLuint unit, const GLmatrix *mat)
{
        nouveauContextPtr nmesa = NOUVEAU_CONTEXT(ctx);
        BEGIN_RING_CACHE(NvSub3D, NV10_TCL_PRIMITIVE_3D_TX_MATRIX(unit, 0), 16);
        /*XXX: This SHOULD work.*/
        OUT_RING_CACHEp(mat->m, 16);
}

/* Update anything that depends on the window position/size */
static void nv10WindowMoved(nouveauContextPtr nmesa)
{
	GLcontext *ctx = nmesa->glCtx;
	GLfloat *v = nmesa->viewport.m;
	GLuint w = ctx->Viewport.Width;
	GLuint h = ctx->Viewport.Height;
	GLuint x = ctx->Viewport.X + nmesa->drawX;
	GLuint y = ctx->Viewport.Y + nmesa->drawY;
	int i;

        BEGIN_RING_CACHE(NvSub3D, NV10_TCL_PRIMITIVE_3D_VIEWPORT_HORIZ, 2);
        OUT_RING_CACHE((w << 16) | x);
        OUT_RING_CACHE((h << 16) | y);

	/* something to do with clears, possibly doesn't belong here */
	BEGIN_RING_SIZE(NvSub3D, 0x02b4, 1);
	OUT_RING(0);

	BEGIN_RING_CACHE(NvSub3D,
	      NV10_TCL_PRIMITIVE_3D_VIEWPORT_CLIP_HORIZ(0), 1);
        OUT_RING_CACHE(((w+x-1) << 16) | x | 0x08000800);
	BEGIN_RING_CACHE(NvSub3D,
	      NV10_TCL_PRIMITIVE_3D_VIEWPORT_CLIP_VERT(0), 1);
        OUT_RING_CACHE(((h+y-1) << 16) | y | 0x08000800);
	for (i=1; i<8; i++) {
		BEGIN_RING_CACHE(NvSub3D,
		      NV10_TCL_PRIMITIVE_3D_VIEWPORT_CLIP_HORIZ(i), 1);
        	OUT_RING_CACHE(0);
		BEGIN_RING_CACHE(NvSub3D,
		      NV10_TCL_PRIMITIVE_3D_VIEWPORT_CLIP_VERT(i), 1);
	        OUT_RING_CACHE(0);
	}

	nv10ViewportScale(nmesa);
}

/* Initialise any card-specific non-GL related state */
static GLboolean nv10InitCard(nouveauContextPtr nmesa)
{
	nouveauObjectOnSubchannel(nmesa, NvSub3D, Nv3D);

	BEGIN_RING_SIZE(NvSub3D, NV10_TCL_PRIMITIVE_3D_SET_DMA_IN_MEMORY0, 2);
	OUT_RING(NvDmaFB);	/* 184 dma_in_memory0 */
	OUT_RING(NvDmaFB);	/* 188 dma_in_memory1 */
	BEGIN_RING_SIZE(NvSub3D, NV10_TCL_PRIMITIVE_3D_SET_DMA_IN_MEMORY2, 2);
	OUT_RING(NvDmaFB);	/* 194 dma_in_memory2 */
	OUT_RING(NvDmaFB);	/* 198 dma_in_memory3 */

	BEGIN_RING_SIZE(NvSub3D, 0x0290, 1);
	OUT_RING(0x00100001);
	BEGIN_RING_SIZE(NvSub3D, 0x03f4, 1);
	OUT_RING(0);

	/* not for nv10, only for >= nv11 */
	if ((nmesa->screen->card->id>>4) >= 0x11) {
	        BEGIN_RING_SIZE(NvSub3D, 0x120, 3);
        	OUT_RING(0);
	        OUT_RING(1);
	        OUT_RING(2);
	}

	return GL_TRUE;
}

/* Update buffer offset/pitch/format */
static GLboolean nv10BindBuffers(nouveauContextPtr nmesa, int num_color,
				 nouveau_renderbuffer **color,
				 nouveau_renderbuffer *depth)
{
	GLuint x, y, w, h;
	GLuint pitch, format, depth_pitch;

	w = color[0]->mesa.Width;
	h = color[0]->mesa.Height;
	x = nmesa->drawX;
	y = nmesa->drawY;

	if (num_color != 1)
		return GL_FALSE;

        BEGIN_RING_CACHE(NvSub3D, NV10_TCL_PRIMITIVE_3D_VIEWPORT_HORIZ, 6);
        OUT_RING_CACHE((w << 16) | x);
        OUT_RING_CACHE((h << 16) | y);
	depth_pitch = (depth ? depth->pitch : color[0]->pitch);
	pitch = (depth_pitch<<16) | color[0]->pitch;
	format = 0x108;
	if (color[0]->mesa._ActualFormat != GL_RGBA8) {
		format = 0x103; /* R5G6B5 color buffer */
	}
	OUT_RING_CACHE(format);
	OUT_RING_CACHE(pitch);
	OUT_RING_CACHE(color[0]->offset);
	OUT_RING_CACHE(depth ? depth->offset : color[0]->offset);

	/* Always set to bottom left of buffer */
	BEGIN_RING_CACHE(NvSub3D, NV10_TCL_PRIMITIVE_3D_VIEWPORT_ORIGIN_X, 4);
	OUT_RING_CACHEf (0.0);
	OUT_RING_CACHEf ((GLfloat) h);
	OUT_RING_CACHEf (0.0);
	OUT_RING_CACHEf (0.0);

	return GL_TRUE;
}

void nv10InitStateFuncs(GLcontext *ctx, struct dd_function_table *func)
{
	nouveauContextPtr nmesa = NOUVEAU_CONTEXT(ctx);

	func->AlphaFunc			= nv10AlphaFunc;
	func->BlendColor		= nv10BlendColor;
	func->BlendEquationSeparate	= nv10BlendEquationSeparate;
	func->BlendFuncSeparate		= nv10BlendFuncSeparate;
	func->Clear			= nv10Clear;
	func->ClearColor		= nv10ClearColor;
	func->ClearDepth		= nv10ClearDepth;
	func->ClearStencil		= nv10ClearStencil;
	func->ClipPlane			= nv10ClipPlane;
	func->ColorMask			= nv10ColorMask;
	func->ColorMaterial		= nv10ColorMaterial;
	func->CullFace			= nv10CullFace;
	func->FrontFace			= nv10FrontFace;
	func->DepthFunc			= nv10DepthFunc;
	func->DepthMask			= nv10DepthMask;
	func->DepthRange		= nv10DepthRange;
	func->Enable			= nv10Enable;
	func->Fogfv			= nv10Fogfv;
	func->Hint			= nv10Hint;
	func->Lightfv			= nv10Lightfv;
/*	func->LightModelfv		= nv10LightModelfv; */
	func->LineStipple		= nv10LineStipple;		/* Not for NV10 */
	func->LineWidth			= nv10LineWidth;
	func->LogicOpcode		= nv10LogicOpcode;
	func->PointParameterfv		= nv10PointParameterfv;
	func->PointSize			= nv10PointSize;
	func->PolygonMode		= nv10PolygonMode;
	func->PolygonOffset		= nv10PolygonOffset;
	func->PolygonStipple		= nv10PolygonStipple;		/* Not for NV10 */
/*	func->ReadBuffer		= nv10ReadBuffer;*/
/*	func->RenderMode		= nv10RenderMode;*/
	func->Scissor			= nv10Scissor;
	func->ShadeModel		= nv10ShadeModel;
	func->StencilFuncSeparate	= nv10StencilFuncSeparate;
	func->StencilMaskSeparate	= nv10StencilMaskSeparate;
	func->StencilOpSeparate		= nv10StencilOpSeparate;
/*	func->TexGen			= nv10TexGen;*/
/*	func->TexParameter		= nv10TexParameter;*/
	func->TextureMatrix		= nv10TextureMatrix;

	nmesa->hw_func.InitCard		= nv10InitCard;
	nmesa->hw_func.BindBuffers	= nv10BindBuffers;
	nmesa->hw_func.WindowMoved	= nv10WindowMoved;
}
