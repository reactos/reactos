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

static void nv20AlphaFunc(GLcontext *ctx, GLenum func, GLfloat ref)
{
	nouveauContextPtr nmesa = NOUVEAU_CONTEXT(ctx);
	GLubyte ubRef;
	CLAMPED_FLOAT_TO_UBYTE(ubRef, ref);

	BEGIN_RING_CACHE(NvSub3D, NV20_TCL_PRIMITIVE_3D_ALPHA_FUNC_FUNC, 2);
	OUT_RING_CACHE(func);
	OUT_RING_CACHE(ubRef);
}

static void nv20BlendColor(GLcontext *ctx, const GLfloat color[4])
{
	nouveauContextPtr nmesa = NOUVEAU_CONTEXT(ctx); 
	GLubyte cf[4];

	CLAMPED_FLOAT_TO_UBYTE(cf[0], color[0]);
	CLAMPED_FLOAT_TO_UBYTE(cf[1], color[1]);
	CLAMPED_FLOAT_TO_UBYTE(cf[2], color[2]);
	CLAMPED_FLOAT_TO_UBYTE(cf[3], color[3]);

	BEGIN_RING_CACHE(NvSub3D, NV20_TCL_PRIMITIVE_3D_BLEND_COLOR, 1);
	OUT_RING_CACHE(PACK_COLOR_8888(cf[3], cf[1], cf[2], cf[0]));
}

static void nv20BlendEquationSeparate(GLcontext *ctx, GLenum modeRGB, GLenum modeA)
{
	nouveauContextPtr nmesa = NOUVEAU_CONTEXT(ctx);
	BEGIN_RING_CACHE(NvSub3D, NV20_TCL_PRIMITIVE_3D_BLEND_EQUATION, 1);
	OUT_RING_CACHE((modeA<<16) | modeRGB);
}


static void nv20BlendFuncSeparate(GLcontext *ctx, GLenum sfactorRGB, GLenum dfactorRGB,
		GLenum sfactorA, GLenum dfactorA)
{
	nouveauContextPtr nmesa = NOUVEAU_CONTEXT(ctx);
	BEGIN_RING_CACHE(NvSub3D, NV20_TCL_PRIMITIVE_3D_BLEND_FUNC_SRC, 2);
	OUT_RING_CACHE((sfactorA<<16) | sfactorRGB);
	OUT_RING_CACHE((dfactorA<<16) | dfactorRGB);
}

static void nv20Clear(GLcontext *ctx, GLbitfield mask)
{
	nouveauContextPtr nmesa = NOUVEAU_CONTEXT(ctx);
	GLuint hw_bufs = 0;

	if (mask & (BUFFER_BIT_FRONT_LEFT | BUFFER_BIT_BACK_LEFT))
		hw_bufs |= 0xf0;
	if (mask & (BUFFER_BIT_DEPTH))
		hw_bufs |= 0x03;

	if (hw_bufs) {
		BEGIN_RING_CACHE(NvSub3D, NV30_TCL_PRIMITIVE_3D_CLEAR_WHICH_BUFFERS, 1);
		OUT_RING_CACHE(hw_bufs);
	}
}

static void nv20ClearColor(GLcontext *ctx, const GLfloat color[4])
{
	nouveauContextPtr nmesa = NOUVEAU_CONTEXT(ctx);
	GLubyte c[4];
	UNCLAMPED_FLOAT_TO_RGBA_CHAN(c,color);
	BEGIN_RING_CACHE(NvSub3D, NV20_TCL_PRIMITIVE_3D_CLEAR_VALUE_ARGB, 1);
	OUT_RING_CACHE(PACK_COLOR_8888(c[3],c[0],c[1],c[2]));
}

static void nv20ClearDepth(GLcontext *ctx, GLclampd d)
{
	nouveauContextPtr nmesa = NOUVEAU_CONTEXT(ctx);
	nmesa->clear_value=((nmesa->clear_value&0x000000FF)|(((uint32_t)(d*0xFFFFFF))<<8));
	BEGIN_RING_CACHE(NvSub3D, NV20_TCL_PRIMITIVE_3D_CLEAR_VALUE_DEPTH, 1);
	OUT_RING_CACHE(nmesa->clear_value);
}

/* we're don't support indexed buffers
   void (*ClearIndex)(GLcontext *ctx, GLuint index)
 */

static void nv20ClearStencil(GLcontext *ctx, GLint s)
{
	nouveauContextPtr nmesa = NOUVEAU_CONTEXT(ctx);
	nmesa->clear_value=((nmesa->clear_value&0xFFFFFF00)|(s&0x000000FF));
	BEGIN_RING_CACHE(NvSub3D, NV20_TCL_PRIMITIVE_3D_CLEAR_VALUE_DEPTH, 1);
	OUT_RING_CACHE(nmesa->clear_value);
}

static void nv20ClipPlane(GLcontext *ctx, GLenum plane, const GLfloat *equation)
{
	nouveauContextPtr nmesa = NOUVEAU_CONTEXT(ctx);
	BEGIN_RING_CACHE(NvSub3D, NV20_TCL_PRIMITIVE_3D_CLIP_PLANE_A(plane), 4);
	OUT_RING_CACHEf(equation[0]);
	OUT_RING_CACHEf(equation[1]);
	OUT_RING_CACHEf(equation[2]);
	OUT_RING_CACHEf(equation[3]);
}

static void nv20ColorMask(GLcontext *ctx, GLboolean rmask, GLboolean gmask,
		GLboolean bmask, GLboolean amask )
{
	nouveauContextPtr nmesa = NOUVEAU_CONTEXT(ctx);
	BEGIN_RING_CACHE(NvSub3D, NV20_TCL_PRIMITIVE_3D_COLOR_MASK, 1);
	OUT_RING_CACHE(((amask && 0x01) << 24) | ((rmask && 0x01) << 16) | ((gmask && 0x01)<< 8) | ((bmask && 0x01) << 0));
}

static void nv20ColorMaterial(GLcontext *ctx, GLenum face, GLenum mode)
{
	// TODO I need love
}

static void nv20CullFace(GLcontext *ctx, GLenum mode)
{
	nouveauContextPtr nmesa = NOUVEAU_CONTEXT(ctx);
	BEGIN_RING_CACHE(NvSub3D, NV20_TCL_PRIMITIVE_3D_CULL_FACE, 1);
	OUT_RING_CACHE(mode);
}

static void nv20FrontFace(GLcontext *ctx, GLenum mode)
{
	nouveauContextPtr nmesa = NOUVEAU_CONTEXT(ctx);
	BEGIN_RING_CACHE(NvSub3D, NV20_TCL_PRIMITIVE_3D_FRONT_FACE, 1);
	OUT_RING_CACHE(mode);
}

static void nv20DepthFunc(GLcontext *ctx, GLenum func)
{
	nouveauContextPtr nmesa = NOUVEAU_CONTEXT(ctx);
	BEGIN_RING_CACHE(NvSub3D, NV20_TCL_PRIMITIVE_3D_DEPTH_FUNC, 1);
	OUT_RING_CACHE(func);
}

static void nv20DepthMask(GLcontext *ctx, GLboolean flag)
{
	nouveauContextPtr nmesa = NOUVEAU_CONTEXT(ctx);
	BEGIN_RING_CACHE(NvSub3D, NV20_TCL_PRIMITIVE_3D_DEPTH_WRITE_ENABLE, 1);
	OUT_RING_CACHE(flag);
}

static void nv20DepthRange(GLcontext *ctx, GLclampd nearval, GLclampd farval)
{
	nouveauContextPtr nmesa = NOUVEAU_CONTEXT(ctx);
	BEGIN_RING_CACHE(NvSub3D, NV20_TCL_PRIMITIVE_3D_DEPTH_RANGE_NEAR, 2);
	OUT_RING_CACHEf(nearval);
	OUT_RING_CACHEf(farval);
}

/** Specify the current buffer for writing */
//void (*DrawBuffer)( GLcontext *ctx, GLenum buffer );
/** Specify the buffers for writing for fragment programs*/
//void (*DrawBuffers)( GLcontext *ctx, GLsizei n, const GLenum *buffers );

static void nv20Enable(GLcontext *ctx, GLenum cap, GLboolean state)
{
	nouveauContextPtr nmesa = NOUVEAU_CONTEXT(ctx);
	switch(cap)
	{
		case GL_ALPHA_TEST:
			BEGIN_RING_CACHE(NvSub3D, NV20_TCL_PRIMITIVE_3D_ALPHA_FUNC_ENABLE, 1);
			OUT_RING_CACHE(state);
			break;
//		case GL_AUTO_NORMAL:
		case GL_BLEND:
			BEGIN_RING_CACHE(NvSub3D, NV20_TCL_PRIMITIVE_3D_BLEND_FUNC_ENABLE, 1);
			OUT_RING_CACHE(state);
			break;
		case GL_CLIP_PLANE0:
		case GL_CLIP_PLANE1:
		case GL_CLIP_PLANE2:
		case GL_CLIP_PLANE3:
		case GL_CLIP_PLANE4:
		case GL_CLIP_PLANE5:
			BEGIN_RING_CACHE(NvSub3D, NV20_TCL_PRIMITIVE_3D_CLIP_PLANE_ENABLE(cap-GL_CLIP_PLANE0), 1);
			OUT_RING_CACHE(state);
			break;
		case GL_COLOR_LOGIC_OP:
			BEGIN_RING_CACHE(NvSub3D, NV20_TCL_PRIMITIVE_3D_COLOR_LOGIC_OP_ENABLE, 1);
			OUT_RING_CACHE(state);
			break;
//		case GL_COLOR_MATERIAL:
//		case GL_COLOR_SUM_EXT:
//		case GL_COLOR_TABLE:
//		case GL_CONVOLUTION_1D:
//		case GL_CONVOLUTION_2D:
		case GL_CULL_FACE:
			BEGIN_RING_CACHE(NvSub3D, NV20_TCL_PRIMITIVE_3D_CULL_FACE_ENABLE, 1);
			OUT_RING_CACHE(state);
			break;
		case GL_DEPTH_TEST:
			BEGIN_RING_CACHE(NvSub3D, NV20_TCL_PRIMITIVE_3D_DEPTH_TEST_ENABLE, 1);
			OUT_RING_CACHE(state);
			break;
		case GL_DITHER:
			BEGIN_RING_CACHE(NvSub3D, NV20_TCL_PRIMITIVE_3D_DITHER_ENABLE, 1);
			OUT_RING_CACHE(state);
			break;
		case GL_FOG:
			BEGIN_RING_CACHE(NvSub3D, NV20_TCL_PRIMITIVE_3D_FOG_ENABLE, 1);
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
			uint32_t mask=0x11<<(2*(cap-GL_LIGHT0));
			nmesa->enabled_lights=((nmesa->enabled_lights&mask)|(mask*state));
			if (nmesa->lighting_enabled)
			{
				BEGIN_RING_CACHE(NvSub3D, NV20_TCL_PRIMITIVE_3D_ENABLED_LIGHTS, 1);
				OUT_RING_CACHE(nmesa->enabled_lights);
			}
			break;
			}
		case GL_LIGHTING:
			nmesa->lighting_enabled=state;
			BEGIN_RING_CACHE(NvSub3D, NV20_TCL_PRIMITIVE_3D_ENABLED_LIGHTS, 1);
			if (nmesa->lighting_enabled)
				OUT_RING_CACHE(nmesa->enabled_lights);
			else
				OUT_RING_CACHE(0x0);
			break;
		case GL_LINE_SMOOTH:
			BEGIN_RING_CACHE(NvSub3D, NV20_TCL_PRIMITIVE_3D_LINE_SMOOTH_ENABLE, 1);
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
			BEGIN_RING_CACHE(NvSub3D, NV20_TCL_PRIMITIVE_3D_NORMALIZE_ENABLE, 1);
			OUT_RING_CACHE(state);
			break;
//		case GL_POINT_SMOOTH:
		case GL_POLYGON_OFFSET_POINT:
			BEGIN_RING_CACHE(NvSub3D, NV20_TCL_PRIMITIVE_3D_POLYGON_OFFSET_POINT_ENABLE, 1);
			OUT_RING_CACHE(state);
			break;
		case GL_POLYGON_OFFSET_LINE:
			BEGIN_RING_CACHE(NvSub3D, NV20_TCL_PRIMITIVE_3D_POLYGON_OFFSET_LINE_ENABLE, 1);
			OUT_RING_CACHE(state);
			break;
		case GL_POLYGON_OFFSET_FILL:
			BEGIN_RING_CACHE(NvSub3D, NV20_TCL_PRIMITIVE_3D_POLYGON_OFFSET_FILL_ENABLE, 1);
			OUT_RING_CACHE(state);
			break;
		case GL_POLYGON_SMOOTH:
			BEGIN_RING_CACHE(NvSub3D, NV20_TCL_PRIMITIVE_3D_POLYGON_SMOOTH_ENABLE, 1);
			OUT_RING_CACHE(state);
			break;
		case GL_POLYGON_STIPPLE:
			BEGIN_RING_CACHE(NvSub3D, NV20_TCL_PRIMITIVE_3D_POLYGON_STIPPLE_ENABLE, 1);
			OUT_RING_CACHE(state);
			break;
//		case GL_POST_COLOR_MATRIX_COLOR_TABLE:
//		case GL_POST_CONVOLUTION_COLOR_TABLE:
//		case GL_RESCALE_NORMAL:
		case GL_SCISSOR_TEST:
			/* No enable bit, nv20Scissor will adjust to max range */
			ctx->Driver.Scissor(ctx, ctx->Scissor.X, ctx->Scissor.Y,
			      			 ctx->Scissor.Width, ctx->Scissor.Height);
			break;
//		case GL_SEPARABLE_2D:
		case GL_STENCIL_TEST:
			// TODO BACK and FRONT ?
			BEGIN_RING_CACHE(NvSub3D, NV20_TCL_PRIMITIVE_3D_STENCIL_ENABLE, 1);
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

static void nv20Fogfv(GLcontext *ctx, GLenum pname, const GLfloat *params)
{
    nouveauContextPtr nmesa = NOUVEAU_CONTEXT(ctx);
    switch(pname)
    {
        case GL_FOG_MODE:
            BEGIN_RING_CACHE(NvSub3D, NV20_TCL_PRIMITIVE_3D_FOG_MODE, 1);
            //OUT_RING_CACHE (params);
            break;
            /* TODO: unsure about the rest.*/
        default:
            break;
    }

}
   
static void nv20Hint(GLcontext *ctx, GLenum target, GLenum mode)
{
	// TODO I need love (fog and line_smooth hints)
}

// void (*IndexMask)(GLcontext *ctx, GLuint mask);

enum {
	SPOTLIGHT_NO_UPDATE,
	SPOTLIGHT_UPDATE_EXPONENT,
	SPOTLIGHT_UPDATE_DIRECTION,
	SPOTLIGHT_UPDATE_ALL
};

static void nv20Lightfv(GLcontext *ctx, GLenum light, GLenum pname, const GLfloat *params )
{
	nouveauContextPtr nmesa = NOUVEAU_CONTEXT(ctx);
	GLint p = light - GL_LIGHT0;
	struct gl_light *l = &ctx->Light.Light[p];
	int spotlight_update = SPOTLIGHT_NO_UPDATE;

	/* not sure where the fourth param value goes...*/
	switch(pname)
	{
		case GL_AMBIENT:
			BEGIN_RING_CACHE(NvSub3D, NV20_TCL_PRIMITIVE_3D_LIGHT_FRONT_SIDE_PRODUCT_AMBIENT_R(p), 3);
			OUT_RING_CACHEf(params[0]);
			OUT_RING_CACHEf(params[1]);
			OUT_RING_CACHEf(params[2]);
			break;
		case GL_DIFFUSE:
			BEGIN_RING_CACHE(NvSub3D, NV20_TCL_PRIMITIVE_3D_LIGHT_FRONT_SIDE_PRODUCT_DIFFUSE_R(p), 3);
			OUT_RING_CACHEf(params[0]);
			OUT_RING_CACHEf(params[1]);
			OUT_RING_CACHEf(params[2]);
			break;
		case GL_SPECULAR:
			BEGIN_RING_CACHE(NvSub3D, NV20_TCL_PRIMITIVE_3D_LIGHT_FRONT_SIDE_PRODUCT_SPECULAR_R(p), 3);
			OUT_RING_CACHEf(params[0]);
			OUT_RING_CACHEf(params[1]);
			OUT_RING_CACHEf(params[2]);
			break;
		case GL_POSITION:
			BEGIN_RING_CACHE(NvSub3D, NV20_TCL_PRIMITIVE_3D_LIGHT_POSITION_X(p), 3);
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
			BEGIN_RING_CACHE(NvSub3D, NV20_TCL_PRIMITIVE_3D_LIGHT_CONSTANT_ATTENUATION(p), 1);
			OUT_RING_CACHEf(*params);
			break;
		case GL_LINEAR_ATTENUATION:
			BEGIN_RING_CACHE(NvSub3D, NV20_TCL_PRIMITIVE_3D_LIGHT_LINEAR_ATTENUATION(p), 1);
			OUT_RING_CACHEf(*params);
			break;
		case GL_QUADRATIC_ATTENUATION:
			BEGIN_RING_CACHE(NvSub3D, NV20_TCL_PRIMITIVE_3D_LIGHT_QUADRATIC_ATTENUATION(p), 1);
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
				BEGIN_RING_CACHE(NvSub3D, NV20_TCL_PRIMITIVE_3D_LIGHT_SPOT_DIR_X(p), 3);
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
				BEGIN_RING_CACHE(NvSub3D, NV20_TCL_PRIMITIVE_3D_LIGHT_SPOT_CUTOFF_A(p), 3);
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
				BEGIN_RING_CACHE(NvSub3D, NV20_TCL_PRIMITIVE_3D_LIGHT_SPOT_CUTOFF_A(p), 7);
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


static void nv20LineStipple(GLcontext *ctx, GLint factor, GLushort pattern )
{
/*	nouveauContextPtr nmesa = NOUVEAU_CONTEXT(ctx);
	BEGIN_RING_CACHE(NvSub3D, NV20_TCL_PRIMITIVE_3D_LINE_STIPPLE_PATTERN, 1);
	OUT_RING_CACHE((pattern << 16) | factor);*/
}

static void nv20LineWidth(GLcontext *ctx, GLfloat width)
{
	nouveauContextPtr nmesa = NOUVEAU_CONTEXT(ctx);
	BEGIN_RING_CACHE(NvSub3D, NV20_TCL_PRIMITIVE_3D_LINE_WIDTH, 1);
	OUT_RING_CACHEf(width);
}

static void nv20LogicOpcode(GLcontext *ctx, GLenum opcode)
{
	nouveauContextPtr nmesa = NOUVEAU_CONTEXT(ctx);
	BEGIN_RING_CACHE(NvSub3D, NV20_TCL_PRIMITIVE_3D_COLOR_LOGIC_OP_OP, 1);
	OUT_RING_CACHE(opcode);
}

static void nv20PointParameterfv(GLcontext *ctx, GLenum pname, const GLfloat *params)
{
	/*TODO: not sure what goes here. */
	nouveauContextPtr nmesa = NOUVEAU_CONTEXT(ctx);
	
}

/** Specify the diameter of rasterized points */
static void nv20PointSize(GLcontext *ctx, GLfloat size)
{
	nouveauContextPtr nmesa = NOUVEAU_CONTEXT(ctx);
	BEGIN_RING_CACHE(NvSub3D, NV20_TCL_PRIMITIVE_3D_POINT_SIZE, 1);
	OUT_RING_CACHEf(size);
}

/** Select a polygon rasterization mode */
static void nv20PolygonMode(GLcontext *ctx, GLenum face, GLenum mode)
{
	nouveauContextPtr nmesa = NOUVEAU_CONTEXT(ctx);

	if (face == GL_FRONT || face == GL_FRONT_AND_BACK) {
		BEGIN_RING_CACHE(NvSub3D, NV20_TCL_PRIMITIVE_3D_POLYGON_MODE_FRONT, 1);
		OUT_RING_CACHE(mode);
	}
	if (face == GL_BACK || face == GL_FRONT_AND_BACK) {
		BEGIN_RING_CACHE(NvSub3D, NV20_TCL_PRIMITIVE_3D_POLYGON_MODE_BACK, 1);
		OUT_RING_CACHE(mode);
	}
}

/** Set the scale and units used to calculate depth values */
static void nv20PolygonOffset(GLcontext *ctx, GLfloat factor, GLfloat units)
{
        nouveauContextPtr nmesa = NOUVEAU_CONTEXT(ctx);
        BEGIN_RING_CACHE(NvSub3D, NV20_TCL_PRIMITIVE_3D_POLYGON_OFFSET_FACTOR, 2);
        OUT_RING_CACHEf(factor);
        OUT_RING_CACHEf(units);
}

/** Set the polygon stippling pattern */
static void nv20PolygonStipple(GLcontext *ctx, const GLubyte *mask )
{
        nouveauContextPtr nmesa = NOUVEAU_CONTEXT(ctx);
        BEGIN_RING_CACHE(NvSub3D, NV20_TCL_PRIMITIVE_3D_POLYGON_STIPPLE_PATTERN(0), 32);
        OUT_RING_CACHEp(mask, 32);
}

/* Specifies the current buffer for reading */
void (*ReadBuffer)( GLcontext *ctx, GLenum buffer );
/** Set rasterization mode */
void (*RenderMode)(GLcontext *ctx, GLenum mode );

/** Define the scissor box */
static void nv20Scissor(GLcontext *ctx, GLint x, GLint y, GLsizei w, GLsizei h)
{
        nouveauContextPtr nmesa = NOUVEAU_CONTEXT(ctx);

	/* There's no scissor enable bit, so adjust the scissor to cover the
	 * maximum draw buffer bounds
	 */
	if (!ctx->Scissor.Enabled) {
	   x = y = 0;
	   w = h = 4095;
	} else {
	   x += nmesa->drawX;
	   y += nmesa->drawY;
	}

        BEGIN_RING_CACHE(NvSub3D, NV20_TCL_PRIMITIVE_3D_SCISSOR_X2_X1, 1);
        OUT_RING_CACHE((w << 16) | x );
        BEGIN_RING_CACHE(NvSub3D, NV20_TCL_PRIMITIVE_3D_SCISSOR_Y2_Y1, 1);
        OUT_RING_CACHE((h << 16) | y );

}

/** Select flat or smooth shading */
static void nv20ShadeModel(GLcontext *ctx, GLenum mode)
{
	nouveauContextPtr nmesa = NOUVEAU_CONTEXT(ctx);

	BEGIN_RING_CACHE(NvSub3D, NV20_TCL_PRIMITIVE_3D_SHADE_MODEL, 1);
	OUT_RING_CACHE(mode);
}

/** OpenGL 2.0 two-sided StencilFunc */
static void nv20StencilFuncSeparate(GLcontext *ctx, GLenum face, GLenum func,
		GLint ref, GLuint mask)
{
	nouveauContextPtr nmesa = NOUVEAU_CONTEXT(ctx);

	BEGIN_RING_CACHE(NvSub3D, NV20_TCL_PRIMITIVE_3D_STENCIL_FUNC_FUNC, 3);
	OUT_RING_CACHE(func);
	OUT_RING_CACHE(ref);
	OUT_RING_CACHE(mask);
}

/** OpenGL 2.0 two-sided StencilMask */
static void nv20StencilMaskSeparate(GLcontext *ctx, GLenum face, GLuint mask)
{
	nouveauContextPtr nmesa = NOUVEAU_CONTEXT(ctx);

	BEGIN_RING_CACHE(NvSub3D, NV20_TCL_PRIMITIVE_3D_STENCIL_MASK, 1);
	OUT_RING_CACHE(mask);
}

/** OpenGL 2.0 two-sided StencilOp */
static void nv20StencilOpSeparate(GLcontext *ctx, GLenum face, GLenum fail,
		GLenum zfail, GLenum zpass)
{
	nouveauContextPtr nmesa = NOUVEAU_CONTEXT(ctx);

	BEGIN_RING_CACHE(NvSub3D, NV20_TCL_PRIMITIVE_3D_STENCIL_OP_FAIL, 1);
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

static void nv20TextureMatrix(GLcontext *ctx, GLuint unit, const GLmatrix *mat)
{
        nouveauContextPtr nmesa = NOUVEAU_CONTEXT(ctx);
        BEGIN_RING_CACHE(NvSub3D, NV20_TCL_PRIMITIVE_3D_TX_MATRIX(unit, 0), 16);
        /*XXX: This SHOULD work.*/
        OUT_RING_CACHEp(mat->m, 16);
}

/* Update anything that depends on the window position/size */
static void nv20WindowMoved(nouveauContextPtr nmesa)
{
	GLcontext *ctx = nmesa->glCtx;
	GLfloat *v = nmesa->viewport.m;
	GLuint w = ctx->Viewport.Width;
	GLuint h = ctx->Viewport.Height;
	GLuint x = ctx->Viewport.X + nmesa->drawX;
	GLuint y = ctx->Viewport.Y + nmesa->drawY;
	int i;

        BEGIN_RING_CACHE(NvSub3D, NV20_TCL_PRIMITIVE_3D_VIEWPORT_HORIZ, 2);
        OUT_RING_CACHE((w << 16) | x);
        OUT_RING_CACHE((h << 16) | y);

	BEGIN_RING_SIZE(NvSub3D, 0x02b4, 1);
	OUT_RING(0);

	BEGIN_RING_CACHE(NvSub3D,
	      NV20_TCL_PRIMITIVE_3D_VIEWPORT_CLIP_HORIZ(0), 1);
        OUT_RING_CACHE((4095 << 16) | 0);
	BEGIN_RING_CACHE(NvSub3D,
	      NV20_TCL_PRIMITIVE_3D_VIEWPORT_CLIP_VERT(0), 1);
        OUT_RING_CACHE((4095 << 16) | 0);
	for (i=1; i<8; i++) {
		BEGIN_RING_CACHE(NvSub3D,
		      NV20_TCL_PRIMITIVE_3D_VIEWPORT_CLIP_HORIZ(i), 1);
        	OUT_RING_CACHE(0);
		BEGIN_RING_CACHE(NvSub3D,
		      NV20_TCL_PRIMITIVE_3D_VIEWPORT_CLIP_VERT(i), 1);
	        OUT_RING_CACHE(0);
	}

	ctx->Driver.Scissor(ctx, ctx->Scissor.X, ctx->Scissor.Y,
	      		    ctx->Scissor.Width, ctx->Scissor.Height);

	/* TODO: recalc viewport scale coefs */
}

/* Initialise any card-specific non-GL related state */
static GLboolean nv20InitCard(nouveauContextPtr nmesa)
{
	nouveauObjectOnSubchannel(nmesa, NvSub3D, Nv3D);

	BEGIN_RING_SIZE(NvSub3D, NV20_TCL_PRIMITIVE_3D_SET_OBJECT1, 2);
	OUT_RING(NvDmaFB);	/* 184 dma_object1 */
	OUT_RING(NvDmaFB);	/* 188 dma_object2 */
	BEGIN_RING_SIZE(NvSub3D, NV20_TCL_PRIMITIVE_3D_SET_OBJECT3, 2);
	OUT_RING(NvDmaFB);	/* 194 dma_object3 */
	OUT_RING(NvDmaFB);	/* 198 dma_object4 */
	BEGIN_RING_SIZE(NvSub3D, NV20_TCL_PRIMITIVE_3D_SET_OBJECT8, 1);
	OUT_RING(NvDmaFB);	/* 1a8 dma_object8 */

	BEGIN_RING_SIZE(NvSub3D, 0x17e0, 3);
	OUT_RINGf(0.0);
	OUT_RINGf(0.0);
	OUT_RINGf(1.0);

	BEGIN_RING_SIZE(NvSub3D, 0x1e6c, 1);
	OUT_RING(0x0db6);
	BEGIN_RING_SIZE(NvSub3D, 0x0290, 1);
	OUT_RING(0x00100001);
	BEGIN_RING_SIZE(NvSub3D, 0x09fc, 1);
	OUT_RING(0);
	BEGIN_RING_SIZE(NvSub3D, 0x1d80, 1);
	OUT_RING(1);
	BEGIN_RING_SIZE(NvSub3D, 0x09f8, 1);
	OUT_RING(4);

	BEGIN_RING_SIZE(NvSub3D, 0x17ec, 3);
	OUT_RINGf(0.0);
	OUT_RINGf(1.0);
	OUT_RINGf(0.0);

	BEGIN_RING_SIZE(NvSub3D, 0x1d88, 1);
	OUT_RING(3);

	/* FIXME: More dma objects to setup ? */

	BEGIN_RING_SIZE(NvSub3D, 0x1e98, 1);
	OUT_RING(0);

	BEGIN_RING_SIZE(NvSub3D, 0x120, 3);
	OUT_RING(0);
	OUT_RING(1);
	OUT_RING(2);

	return GL_TRUE;
}

/* Update buffer offset/pitch/format */
static GLboolean nv20BindBuffers(nouveauContextPtr nmesa, int num_color,
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

        BEGIN_RING_CACHE(NvSub3D, NV20_TCL_PRIMITIVE_3D_VIEWPORT_HORIZ, 6);
        OUT_RING_CACHE((w << 16) | x);
        OUT_RING_CACHE((h << 16) | y);
	depth_pitch = (depth ? depth->pitch : color[0]->pitch);
	pitch = (depth_pitch<<16) | color[0]->pitch;
	format = 0x128;
	if (color[0]->mesa._ActualFormat != GL_RGBA8) {
		format = 0x123; /* R5G6B5 color buffer */
	}
	OUT_RING_CACHE(format);
	OUT_RING_CACHE(pitch);
	OUT_RING_CACHE(color[0]->offset);
	OUT_RING_CACHE(depth ? depth->offset : color[0]->offset);

	if (depth) {
		BEGIN_RING_SIZE(NvSub3D, NV20_TCL_PRIMITIVE_3D_LMA_DEPTH_BUFFER_PITCH, 2);
		/* TODO: use a different buffer */
		OUT_RING(depth->pitch);
		OUT_RING(depth->offset);
	}

	/* Always set to bottom left of buffer */
	BEGIN_RING_CACHE(NvSub3D, NV20_TCL_PRIMITIVE_3D_VIEWPORT_ORIGIN_X, 4);
	OUT_RING_CACHEf (0.0);
	OUT_RING_CACHEf ((GLfloat) h);
	OUT_RING_CACHEf (0.0);
	OUT_RING_CACHEf (0.0);

	return GL_TRUE;
}

void nv20InitStateFuncs(GLcontext *ctx, struct dd_function_table *func)
{
	nouveauContextPtr nmesa = NOUVEAU_CONTEXT(ctx);

	func->AlphaFunc			= nv20AlphaFunc;
	func->BlendColor		= nv20BlendColor;
	func->BlendEquationSeparate	= nv20BlendEquationSeparate;
	func->BlendFuncSeparate		= nv20BlendFuncSeparate;
	func->Clear			= nv20Clear;
	func->ClearColor		= nv20ClearColor;
	func->ClearDepth		= nv20ClearDepth;
	func->ClearStencil		= nv20ClearStencil;
	func->ClipPlane			= nv20ClipPlane;
	func->ColorMask			= nv20ColorMask;
	func->ColorMaterial		= nv20ColorMaterial;
	func->CullFace			= nv20CullFace;
	func->FrontFace			= nv20FrontFace;
	func->DepthFunc			= nv20DepthFunc;
	func->DepthMask			= nv20DepthMask;
	func->DepthRange		= nv20DepthRange;
	func->Enable			= nv20Enable;
	func->Fogfv			= nv20Fogfv;
	func->Hint			= nv20Hint;
	func->Lightfv			= nv20Lightfv;
/*	func->LightModelfv		= nv20LightModelfv; */
	func->LineStipple		= nv20LineStipple;
	func->LineWidth			= nv20LineWidth;
	func->LogicOpcode		= nv20LogicOpcode;
	func->PointParameterfv		= nv20PointParameterfv;
	func->PointSize			= nv20PointSize;
	func->PolygonMode		= nv20PolygonMode;
	func->PolygonOffset		= nv20PolygonOffset;
	func->PolygonStipple		= nv20PolygonStipple;
/*	func->ReadBuffer		= nv20ReadBuffer;*/
/*	func->RenderMode		= nv20RenderMode;*/
	func->Scissor			= nv20Scissor;
	func->ShadeModel		= nv20ShadeModel;
	func->StencilFuncSeparate	= nv20StencilFuncSeparate;
	func->StencilMaskSeparate	= nv20StencilMaskSeparate;
	func->StencilOpSeparate		= nv20StencilOpSeparate;
/*	func->TexGen			= nv20TexGen;*/
/*	func->TexParameter		= nv20TexParameter;*/
	func->TextureMatrix		= nv20TextureMatrix;

	nmesa->hw_func.InitCard		= nv20InitCard;
	nmesa->hw_func.BindBuffers	= nv20BindBuffers;
	nmesa->hw_func.WindowMoved	= nv20WindowMoved;
}

