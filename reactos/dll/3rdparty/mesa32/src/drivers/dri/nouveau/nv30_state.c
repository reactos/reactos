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

#define NOUVEAU_CARD_USING_SHADERS (nmesa->screen->card->type >= NV_40)

static void nv30AlphaFunc(GLcontext *ctx, GLenum func, GLfloat ref)
{
	nouveauContextPtr nmesa = NOUVEAU_CONTEXT(ctx);
	GLubyte ubRef;
	CLAMPED_FLOAT_TO_UBYTE(ubRef, ref);

	BEGIN_RING_CACHE(NvSub3D, NV30_TCL_PRIMITIVE_3D_ALPHA_FUNC_FUNC, 2);
	OUT_RING_CACHE(func);     /* NV30_TCL_PRIMITIVE_3D_ALPHA_FUNC_FUNC */
	OUT_RING_CACHE(ubRef);    /* NV30_TCL_PRIMITIVE_3D_ALPHA_FUNC_REF  */
}

static void nv30BlendColor(GLcontext *ctx, const GLfloat color[4])
{
	nouveauContextPtr nmesa = NOUVEAU_CONTEXT(ctx); 
	GLubyte cf[4];

	CLAMPED_FLOAT_TO_UBYTE(cf[0], color[0]);
	CLAMPED_FLOAT_TO_UBYTE(cf[1], color[1]);
	CLAMPED_FLOAT_TO_UBYTE(cf[2], color[2]);
	CLAMPED_FLOAT_TO_UBYTE(cf[3], color[3]);

	BEGIN_RING_CACHE(NvSub3D, NV30_TCL_PRIMITIVE_3D_BLEND_COLOR, 1);
	OUT_RING_CACHE(PACK_COLOR_8888(cf[3], cf[1], cf[2], cf[0]));
}

static void nv30BlendEquationSeparate(GLcontext *ctx, GLenum modeRGB, GLenum modeA)
{
	nouveauContextPtr nmesa = NOUVEAU_CONTEXT(ctx);
	BEGIN_RING_CACHE(NvSub3D, NV30_TCL_PRIMITIVE_3D_BLEND_EQUATION, 1);
	OUT_RING_CACHE((modeA<<16) | modeRGB);
}


static void nv30BlendFuncSeparate(GLcontext *ctx, GLenum sfactorRGB, GLenum dfactorRGB,
		GLenum sfactorA, GLenum dfactorA)
{
	nouveauContextPtr nmesa = NOUVEAU_CONTEXT(ctx);
	BEGIN_RING_CACHE(NvSub3D, NV30_TCL_PRIMITIVE_3D_BLEND_FUNC_SRC, 2);
	OUT_RING_CACHE((sfactorA<<16) | sfactorRGB);
	OUT_RING_CACHE((dfactorA<<16) | dfactorRGB);
}

static void nv30Clear(GLcontext *ctx, GLbitfield mask)
{
	nouveauContextPtr nmesa = NOUVEAU_CONTEXT(ctx);
	GLuint hw_bufs = 0;

	if (mask & (BUFFER_BIT_FRONT_LEFT | BUFFER_BIT_BACK_LEFT))
		hw_bufs |= 0xf0;
	if (mask & (BUFFER_BIT_DEPTH))
		hw_bufs |= 0x03;

	if (hw_bufs) {
		BEGIN_RING_SIZE(NvSub3D, NV30_TCL_PRIMITIVE_3D_CLEAR_WHICH_BUFFERS, 1);
		OUT_RING(hw_bufs);
	}
}

static void nv30ClearColor(GLcontext *ctx, const GLfloat color[4])
{
	nouveauContextPtr nmesa = NOUVEAU_CONTEXT(ctx);
	GLubyte c[4];
	UNCLAMPED_FLOAT_TO_RGBA_CHAN(c,color);
	BEGIN_RING_CACHE(NvSub3D, NV30_TCL_PRIMITIVE_3D_CLEAR_VALUE_ARGB, 1);
	OUT_RING_CACHE(PACK_COLOR_8888(c[3],c[0],c[1],c[2]));
}

static void nv30ClearDepth(GLcontext *ctx, GLclampd d)
{
	nouveauContextPtr nmesa = NOUVEAU_CONTEXT(ctx);
	nmesa->clear_value=((nmesa->clear_value&0x000000FF)|(((uint32_t)(d*0xFFFFFF))<<8));
	BEGIN_RING_CACHE(NvSub3D, NV30_TCL_PRIMITIVE_3D_CLEAR_VALUE_DEPTH, 1);
	OUT_RING_CACHE(nmesa->clear_value);
}

/* we're don't support indexed buffers
   void (*ClearIndex)(GLcontext *ctx, GLuint index)
 */

static void nv30ClearStencil(GLcontext *ctx, GLint s)
{
	nouveauContextPtr nmesa = NOUVEAU_CONTEXT(ctx);
	nmesa->clear_value=((nmesa->clear_value&0xFFFFFF00)|(s&0x000000FF));
	BEGIN_RING_CACHE(NvSub3D, NV30_TCL_PRIMITIVE_3D_CLEAR_VALUE_DEPTH, 1);
	OUT_RING_CACHE(nmesa->clear_value);
}

static void nv30ClipPlane(GLcontext *ctx, GLenum plane, const GLfloat *equation)
{
	nouveauContextPtr nmesa = NOUVEAU_CONTEXT(ctx);

	if (NOUVEAU_CARD_USING_SHADERS)
		return;

	plane -= GL_CLIP_PLANE0;
	BEGIN_RING_CACHE(NvSub3D, NV30_TCL_PRIMITIVE_3D_CLIP_PLANE_A(plane), 4);
	OUT_RING_CACHEf(equation[0]);
	OUT_RING_CACHEf(equation[1]);
	OUT_RING_CACHEf(equation[2]);
	OUT_RING_CACHEf(equation[3]);
}

static void nv30ColorMask(GLcontext *ctx, GLboolean rmask, GLboolean gmask,
		GLboolean bmask, GLboolean amask )
{
	nouveauContextPtr nmesa = NOUVEAU_CONTEXT(ctx);
	BEGIN_RING_CACHE(NvSub3D, NV30_TCL_PRIMITIVE_3D_COLOR_MASK, 1);
	OUT_RING_CACHE(((amask && 0x01) << 24) | ((rmask && 0x01) << 16) | ((gmask && 0x01)<< 8) | ((bmask && 0x01) << 0));
}

static void nv30ColorMaterial(GLcontext *ctx, GLenum face, GLenum mode)
{
	// TODO I need love
}

static void nv30CullFace(GLcontext *ctx, GLenum mode)
{
	nouveauContextPtr nmesa = NOUVEAU_CONTEXT(ctx);
	BEGIN_RING_CACHE(NvSub3D, NV30_TCL_PRIMITIVE_3D_CULL_FACE, 1);
	OUT_RING_CACHE(mode);
}

static void nv30FrontFace(GLcontext *ctx, GLenum mode)
{
	nouveauContextPtr nmesa = NOUVEAU_CONTEXT(ctx);
	BEGIN_RING_CACHE(NvSub3D, NV30_TCL_PRIMITIVE_3D_FRONT_FACE, 1);
	OUT_RING_CACHE(mode);
}

static void nv30DepthFunc(GLcontext *ctx, GLenum func)
{
	nouveauContextPtr nmesa = NOUVEAU_CONTEXT(ctx);
	BEGIN_RING_CACHE(NvSub3D, NV30_TCL_PRIMITIVE_3D_DEPTH_FUNC, 1);
	OUT_RING_CACHE(func);
}

static void nv30DepthMask(GLcontext *ctx, GLboolean flag)
{
	nouveauContextPtr nmesa = NOUVEAU_CONTEXT(ctx);
	BEGIN_RING_CACHE(NvSub3D, NV30_TCL_PRIMITIVE_3D_DEPTH_WRITE_ENABLE, 1);
	OUT_RING_CACHE(flag);
}

static void nv30DepthRange(GLcontext *ctx, GLclampd nearval, GLclampd farval)
{
	nouveauContextPtr nmesa = NOUVEAU_CONTEXT(ctx);
	BEGIN_RING_CACHE(NvSub3D, NV30_TCL_PRIMITIVE_3D_DEPTH_RANGE_NEAR, 2);
	OUT_RING_CACHEf(nearval);
	OUT_RING_CACHEf(farval);
}

/** Specify the current buffer for writing */
//void (*DrawBuffer)( GLcontext *ctx, GLenum buffer );
/** Specify the buffers for writing for fragment programs*/
//void (*DrawBuffers)( GLcontext *ctx, GLsizei n, const GLenum *buffers );

static void nv30Enable(GLcontext *ctx, GLenum cap, GLboolean state)
{
	nouveauContextPtr nmesa = NOUVEAU_CONTEXT(ctx);
	switch(cap)
	{
		case GL_ALPHA_TEST:
			BEGIN_RING_CACHE(NvSub3D, NV30_TCL_PRIMITIVE_3D_ALPHA_FUNC_ENABLE, 1);
			OUT_RING_CACHE(state);
			break;
//		case GL_AUTO_NORMAL:
		case GL_BLEND:
			BEGIN_RING_CACHE(NvSub3D, NV30_TCL_PRIMITIVE_3D_BLEND_FUNC_ENABLE, 1);
			OUT_RING_CACHE(state);
			break;
		case GL_CLIP_PLANE0:
		case GL_CLIP_PLANE1:
		case GL_CLIP_PLANE2:
		case GL_CLIP_PLANE3:
		case GL_CLIP_PLANE4:
		case GL_CLIP_PLANE5:
			if (NOUVEAU_CARD_USING_SHADERS) {
				nouveauShader *nvs = (nouveauShader *)ctx->VertexProgram._Current;
				if (nvs)
					nvs->translated = GL_FALSE;
			} else {
				BEGIN_RING_CACHE(NvSub3D, NV30_TCL_PRIMITIVE_3D_CLIP_PLANE_ENABLE(cap-GL_CLIP_PLANE0), 1);
				OUT_RING_CACHE(state);
			}
			break;
		case GL_COLOR_LOGIC_OP:
			BEGIN_RING_CACHE(NvSub3D, NV30_TCL_PRIMITIVE_3D_COLOR_LOGIC_OP_ENABLE, 1);
			OUT_RING_CACHE(state);
			break;
//		case GL_COLOR_MATERIAL:
//		case GL_COLOR_SUM_EXT:
//		case GL_COLOR_TABLE:
//		case GL_CONVOLUTION_1D:
//		case GL_CONVOLUTION_2D:
		case GL_CULL_FACE:
			BEGIN_RING_CACHE(NvSub3D, NV30_TCL_PRIMITIVE_3D_CULL_FACE_ENABLE, 1);
			OUT_RING_CACHE(state);
			break;
		case GL_DEPTH_TEST:
			BEGIN_RING_CACHE(NvSub3D, NV30_TCL_PRIMITIVE_3D_DEPTH_TEST_ENABLE, 1);
			OUT_RING_CACHE(state);
			break;
		case GL_DITHER:
			BEGIN_RING_CACHE(NvSub3D, NV30_TCL_PRIMITIVE_3D_DITHER_ENABLE, 1);
			OUT_RING_CACHE(state);
			break;
		case GL_FOG:
			if (NOUVEAU_CARD_USING_SHADERS)
				break;
			BEGIN_RING_CACHE(NvSub3D, NV30_TCL_PRIMITIVE_3D_FOG_ENABLE, 1);
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

			if (NOUVEAU_CARD_USING_SHADERS)
				break;

			nmesa->enabled_lights=((nmesa->enabled_lights&mask)|(mask*state));
			if (nmesa->lighting_enabled)
			{
				BEGIN_RING_CACHE(NvSub3D, NV30_TCL_PRIMITIVE_3D_ENABLED_LIGHTS, 1);
				OUT_RING_CACHE(nmesa->enabled_lights);
			}
			break;
			}
		case GL_LIGHTING:
			if (NOUVEAU_CARD_USING_SHADERS)
				break;

			nmesa->lighting_enabled=state;
			BEGIN_RING_CACHE(NvSub3D, NV30_TCL_PRIMITIVE_3D_ENABLED_LIGHTS, 1);
			if (nmesa->lighting_enabled)
				OUT_RING_CACHE(nmesa->enabled_lights);
			else
				OUT_RING_CACHE(0x0);
			break;
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
		case GL_NORMALIZE:
			if (nmesa->screen->card->type != NV_44) {
				BEGIN_RING_CACHE(NvSub3D, NV30_TCL_PRIMITIVE_3D_NORMALIZE_ENABLE, 1);
				OUT_RING_CACHE(state);
			}
			break;
//		case GL_POINT_SMOOTH:
		case GL_POLYGON_OFFSET_POINT:
			BEGIN_RING_CACHE(NvSub3D, NV30_TCL_PRIMITIVE_3D_POLYGON_OFFSET_POINT_ENABLE, 1);
			OUT_RING_CACHE(state);
			break;
		case GL_POLYGON_OFFSET_LINE:
			BEGIN_RING_CACHE(NvSub3D, NV30_TCL_PRIMITIVE_3D_POLYGON_OFFSET_LINE_ENABLE, 1);
			OUT_RING_CACHE(state);
			break;
		case GL_POLYGON_OFFSET_FILL:
			BEGIN_RING_CACHE(NvSub3D, NV30_TCL_PRIMITIVE_3D_POLYGON_OFFSET_FILL_ENABLE, 1);
			OUT_RING_CACHE(state);
			break;
		case GL_POLYGON_SMOOTH:
			BEGIN_RING_CACHE(NvSub3D, NV30_TCL_PRIMITIVE_3D_POLYGON_SMOOTH_ENABLE, 1);
			OUT_RING_CACHE(state);
			break;
		case GL_POLYGON_STIPPLE:
			BEGIN_RING_CACHE(NvSub3D, NV30_TCL_PRIMITIVE_3D_POLYGON_STIPPLE_ENABLE, 1);
			OUT_RING_CACHE(state);
			break;
//		case GL_POST_COLOR_MATRIX_COLOR_TABLE:
//		case GL_POST_CONVOLUTION_COLOR_TABLE:
//		case GL_RESCALE_NORMAL:
		case GL_SCISSOR_TEST:
			/* No enable bit, nv30Scissor will adjust to max range */
			ctx->Driver.Scissor(ctx, ctx->Scissor.X, ctx->Scissor.Y,
			      			 ctx->Scissor.Width, ctx->Scissor.Height);
			break;
//		case GL_SEPARABLE_2D:
		case GL_STENCIL_TEST:
			// TODO BACK and FRONT ?
			BEGIN_RING_CACHE(NvSub3D, NV30_TCL_PRIMITIVE_3D_STENCIL_FRONT_ENABLE, 1);
			OUT_RING_CACHE(state);
			BEGIN_RING_CACHE(NvSub3D, NV30_TCL_PRIMITIVE_3D_STENCIL_BACK_ENABLE, 1);
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

static void nv30Fogfv(GLcontext *ctx, GLenum pname, const GLfloat *params)
{
    nouveauContextPtr nmesa = NOUVEAU_CONTEXT(ctx);

    if (NOUVEAU_CARD_USING_SHADERS)
        return;

    switch(pname)
    {
    case GL_FOG_MODE:
    {
        int mode = 0;
        /* The modes are different in GL and the card.  */
        switch(ctx->Fog.Mode)
        {
        case GL_LINEAR:
            mode = 0x804;
            break;
        case GL_EXP:
            mode = 0x802;
            break;
        case GL_EXP2:
            mode = 0x803;
            break;
        }
	BEGIN_RING_CACHE(NvSub3D, NV30_TCL_PRIMITIVE_3D_FOG_MODE, 1);
	OUT_RING_CACHE (mode);
	break;
    }
    case GL_FOG_COLOR:
    {
	GLubyte c[4];
	UNCLAMPED_FLOAT_TO_RGBA_CHAN(c,params);
        BEGIN_RING_CACHE(NvSub3D, NV30_TCL_PRIMITIVE_3D_FOG_COLOR, 1);
        /* nvidia ignores the alpha channel */
	OUT_RING_CACHE(PACK_COLOR_8888_REV(c[0],c[1],c[2],c[3]));
        break;
    }
    case GL_FOG_DENSITY:
    case GL_FOG_START:
    case GL_FOG_END:
    {
        GLfloat f=0., c=0.;
        switch(ctx->Fog.Mode)
        {
        case GL_LINEAR:
            f = -1.0/(ctx->Fog.End - ctx->Fog.Start);
            c = ctx->Fog.Start/(ctx->Fog.End - ctx->Fog.Start) + 2.001953;
            break;
        case GL_EXP:
            f = -0.090168*ctx->Fog.Density;
            c = 1.5;
        case GL_EXP2:
            f = -0.212330*ctx->Fog.Density;
            c = 1.5;
        }
        BEGIN_RING_CACHE(NvSub3D, NV30_TCL_PRIMITIVE_3D_FOG_EQUATION_LINEAR, 1);
        OUT_RING_CACHE(f);
        BEGIN_RING_CACHE(NvSub3D, NV30_TCL_PRIMITIVE_3D_FOG_EQUATION_CONSTANT, 1);
        OUT_RING_CACHE(c);
        BEGIN_RING_CACHE(NvSub3D, NV30_TCL_PRIMITIVE_3D_FOG_EQUATION_QUADRATIC, 1);
        OUT_RING_CACHE(0); /* Is this always the same? */
        break;
    }
//    case GL_FOG_COORD_SRC:
    default:
        break;
    }
}
   
static void nv30Hint(GLcontext *ctx, GLenum target, GLenum mode)
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

static void nv30Lightfv(GLcontext *ctx, GLenum light, GLenum pname, const GLfloat *params )
{
	nouveauContextPtr nmesa = NOUVEAU_CONTEXT(ctx);
	GLint p = light - GL_LIGHT0;
	struct gl_light *l = &ctx->Light.Light[p];
	int spotlight_update = SPOTLIGHT_NO_UPDATE;

	if (NOUVEAU_CARD_USING_SHADERS)
	   return;

	/* not sure where the fourth param value goes...*/
	switch(pname)
	{
		case GL_AMBIENT:
			BEGIN_RING_CACHE(NvSub3D, NV30_TCL_PRIMITIVE_3D_LIGHT_FRONT_SIDE_PRODUCT_AMBIENT_R(p), 3);
			OUT_RING_CACHEf(params[0]);
			OUT_RING_CACHEf(params[1]);
			OUT_RING_CACHEf(params[2]);
			break;
		case GL_DIFFUSE:
			BEGIN_RING_CACHE(NvSub3D, NV30_TCL_PRIMITIVE_3D_LIGHT_FRONT_SIDE_PRODUCT_DIFFUSE_R(p), 3);
			OUT_RING_CACHEf(params[0]);
			OUT_RING_CACHEf(params[1]);
			OUT_RING_CACHEf(params[2]);
			break;
		case GL_SPECULAR:
			BEGIN_RING_CACHE(NvSub3D, NV30_TCL_PRIMITIVE_3D_LIGHT_FRONT_SIDE_PRODUCT_SPECULAR_R(p), 3);
			OUT_RING_CACHEf(params[0]);
			OUT_RING_CACHEf(params[1]);
			OUT_RING_CACHEf(params[2]);
			break;
		case GL_POSITION:
			BEGIN_RING_CACHE(NvSub3D, NV30_TCL_PRIMITIVE_3D_LIGHT_POSITION_X(p), 3);
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
			BEGIN_RING_CACHE(NvSub3D, NV30_TCL_PRIMITIVE_3D_LIGHT_CONSTANT_ATTENUATION(p), 1);
			OUT_RING_CACHEf(*params);
			break;
		case GL_LINEAR_ATTENUATION:
			BEGIN_RING_CACHE(NvSub3D, NV30_TCL_PRIMITIVE_3D_LIGHT_LINEAR_ATTENUATION(p), 1);
			OUT_RING_CACHEf(*params);
			break;
		case GL_QUADRATIC_ATTENUATION:
			BEGIN_RING_CACHE(NvSub3D, NV30_TCL_PRIMITIVE_3D_LIGHT_QUADRATIC_ATTENUATION(p), 1);
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
				BEGIN_RING_CACHE(NvSub3D, NV30_TCL_PRIMITIVE_3D_LIGHT_SPOT_DIR_X(p), 3);
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
				BEGIN_RING_CACHE(NvSub3D, NV30_TCL_PRIMITIVE_3D_LIGHT_SPOT_CUTOFF_A(p), 3);
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
				BEGIN_RING_CACHE(NvSub3D, NV30_TCL_PRIMITIVE_3D_LIGHT_SPOT_CUTOFF_A(p), 7);
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
void (*LightModelfv)(GLcontext *ctx, GLenum pname, const GLfloat *params);


static void nv30LineStipple(GLcontext *ctx, GLint factor, GLushort pattern )
{
	nouveauContextPtr nmesa = NOUVEAU_CONTEXT(ctx);
	BEGIN_RING_CACHE(NvSub3D, NV30_TCL_PRIMITIVE_3D_LINE_STIPPLE_PATTERN, 1);
	OUT_RING_CACHE((pattern << 16) | factor);
}

static void nv30LineWidth(GLcontext *ctx, GLfloat width)
{
	nouveauContextPtr nmesa = NOUVEAU_CONTEXT(ctx);
	GLubyte ubWidth;

	ubWidth = (GLubyte)(width * 8.0) & 0xFF;

	BEGIN_RING_CACHE(NvSub3D, NV30_TCL_PRIMITIVE_3D_LINE_WIDTH_SMOOTH, 1);
	OUT_RING_CACHE(ubWidth);
}

static void nv30LogicOpcode(GLcontext *ctx, GLenum opcode)
{
	nouveauContextPtr nmesa = NOUVEAU_CONTEXT(ctx);
	BEGIN_RING_CACHE(NvSub3D, NV30_TCL_PRIMITIVE_3D_COLOR_LOGIC_OP_OP, 1);
	OUT_RING_CACHE(opcode);
}

static void nv30PointParameterfv(GLcontext *ctx, GLenum pname, const GLfloat *params)
{
	/*TODO: not sure what goes here. */
	nouveauContextPtr nmesa = NOUVEAU_CONTEXT(ctx);
	
}

/** Specify the diameter of rasterized points */
static void nv30PointSize(GLcontext *ctx, GLfloat size)
{
	nouveauContextPtr nmesa = NOUVEAU_CONTEXT(ctx);
	BEGIN_RING_CACHE(NvSub3D, NV30_TCL_PRIMITIVE_3D_POINT_SIZE, 1);
	OUT_RING_CACHEf(size);
}

/** Select a polygon rasterization mode */
static void nv30PolygonMode(GLcontext *ctx, GLenum face, GLenum mode)
{
	nouveauContextPtr nmesa = NOUVEAU_CONTEXT(ctx);

	if (face == GL_FRONT || face == GL_FRONT_AND_BACK) {
		BEGIN_RING_CACHE(NvSub3D, NV30_TCL_PRIMITIVE_3D_POLYGON_MODE_FRONT, 1);
		OUT_RING_CACHE(mode);
	}
	if (face == GL_BACK || face == GL_FRONT_AND_BACK) {
		BEGIN_RING_CACHE(NvSub3D, NV30_TCL_PRIMITIVE_3D_POLYGON_MODE_BACK, 1);
		OUT_RING_CACHE(mode);
	}
}

/** Set the scale and units used to calculate depth values */
static void nv30PolygonOffset(GLcontext *ctx, GLfloat factor, GLfloat units)
{
        nouveauContextPtr nmesa = NOUVEAU_CONTEXT(ctx);
        BEGIN_RING_CACHE(NvSub3D, NV30_TCL_PRIMITIVE_3D_POLYGON_OFFSET_FACTOR, 2);
        OUT_RING_CACHEf(factor);

        /* Looks like we always multiply units by 2.0... according to the dumps.*/
        OUT_RING_CACHEf(units * 2.0);
}

/** Set the polygon stippling pattern */
static void nv30PolygonStipple(GLcontext *ctx, const GLubyte *mask )
{
        nouveauContextPtr nmesa = NOUVEAU_CONTEXT(ctx);
        BEGIN_RING_CACHE(NvSub3D, NV30_TCL_PRIMITIVE_3D_POLYGON_STIPPLE_PATTERN(0), 32);
        OUT_RING_CACHEp(mask, 32);
}

/* Specifies the current buffer for reading */
void (*ReadBuffer)( GLcontext *ctx, GLenum buffer );
/** Set rasterization mode */
void (*RenderMode)(GLcontext *ctx, GLenum mode );

/** Define the scissor box */
static void nv30Scissor(GLcontext *ctx, GLint x, GLint y, GLsizei w, GLsizei h)
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

        BEGIN_RING_CACHE(NvSub3D, NV30_TCL_PRIMITIVE_3D_SCISSOR_WIDTH_XPOS, 2);
        OUT_RING_CACHE(((w) << 16) | x);
        OUT_RING_CACHE(((h) << 16) | y);
}

/** Select flat or smooth shading */
static void nv30ShadeModel(GLcontext *ctx, GLenum mode)
{
	nouveauContextPtr nmesa = NOUVEAU_CONTEXT(ctx);

	BEGIN_RING_CACHE(NvSub3D, NV30_TCL_PRIMITIVE_3D_SHADE_MODEL, 1);
	OUT_RING_CACHE(mode);
}

/** OpenGL 2.0 two-sided StencilFunc */
static void nv30StencilFuncSeparate(GLcontext *ctx, GLenum face, GLenum func,
		GLint ref, GLuint mask)
{
	nouveauContextPtr nmesa = NOUVEAU_CONTEXT(ctx);

	if (face == GL_FRONT || face == GL_FRONT_AND_BACK) {
		BEGIN_RING_CACHE(NvSub3D, NV30_TCL_PRIMITIVE_3D_STENCIL_FRONT_FUNC_FUNC, 3);
		OUT_RING_CACHE(func);
		OUT_RING_CACHE(ref);
		OUT_RING_CACHE(mask);
	}
	if (face == GL_BACK || face == GL_FRONT_AND_BACK) {
		BEGIN_RING_CACHE(NvSub3D, NV30_TCL_PRIMITIVE_3D_STENCIL_BACK_FUNC_FUNC, 3);
		OUT_RING_CACHE(func);
		OUT_RING_CACHE(ref);
		OUT_RING_CACHE(mask);
	}
}

/** OpenGL 2.0 two-sided StencilMask */
static void nv30StencilMaskSeparate(GLcontext *ctx, GLenum face, GLuint mask)
{
	nouveauContextPtr nmesa = NOUVEAU_CONTEXT(ctx);

	if (face == GL_FRONT || face == GL_FRONT_AND_BACK) {
		BEGIN_RING_CACHE(NvSub3D, NV30_TCL_PRIMITIVE_3D_STENCIL_FRONT_MASK, 1);
		OUT_RING_CACHE(mask);
	}
	if (face == GL_BACK || face == GL_FRONT_AND_BACK) {
		BEGIN_RING_CACHE(NvSub3D, NV30_TCL_PRIMITIVE_3D_STENCIL_BACK_MASK, 1);
		OUT_RING_CACHE(mask);
	}
}

/** OpenGL 2.0 two-sided StencilOp */
static void nv30StencilOpSeparate(GLcontext *ctx, GLenum face, GLenum fail,
		GLenum zfail, GLenum zpass)
{
	nouveauContextPtr nmesa = NOUVEAU_CONTEXT(ctx);

	if (face == GL_FRONT || face == GL_FRONT_AND_BACK) {
		BEGIN_RING_CACHE(NvSub3D, NV30_TCL_PRIMITIVE_3D_STENCIL_FRONT_OP_FAIL, 3);
		OUT_RING_CACHE(fail);
		OUT_RING_CACHE(zfail);
		OUT_RING_CACHE(zpass);
	}
	if (face == GL_BACK || face == GL_FRONT_AND_BACK) {
		BEGIN_RING_CACHE(NvSub3D, NV30_TCL_PRIMITIVE_3D_STENCIL_BACK_OP_FAIL, 3);
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

static void nv30TextureMatrix(GLcontext *ctx, GLuint unit, const GLmatrix *mat)
{
        nouveauContextPtr nmesa = NOUVEAU_CONTEXT(ctx);

	if (!NOUVEAU_CARD_USING_SHADERS) {
		BEGIN_RING_CACHE(NvSub3D,
				 NV30_TCL_PRIMITIVE_3D_TX_MATRIX(unit, 0), 16);
		/*XXX: This SHOULD work.*/
		OUT_RING_CACHEp(mat->m, 16);
	}
}

static void nv30WindowMoved(nouveauContextPtr nmesa)
{
	GLcontext *ctx = nmesa->glCtx;
	GLfloat *v = nmesa->viewport.m;
	GLuint w = ctx->Viewport.Width;
	GLuint h = ctx->Viewport.Height;
	GLuint x = ctx->Viewport.X + nmesa->drawX;
	GLuint y = ctx->Viewport.Y + nmesa->drawY;

        BEGIN_RING_CACHE(NvSub3D, NV30_TCL_PRIMITIVE_3D_VIEWPORT_DIMS_0, 2);
        OUT_RING_CACHE((w << 16) | x);
        OUT_RING_CACHE((h << 16) | y);
	/* something to do with clears, possibly doesn't belong here */
	BEGIN_RING_CACHE(NvSub3D,
	      NV30_TCL_PRIMITIVE_3D_VIEWPORT_COLOR_BUFFER_OFS0, 2);
        OUT_RING_CACHE(((w+x) << 16) | x);
        OUT_RING_CACHE(((h+y) << 16) | y);
	/* viewport transform */
	BEGIN_RING_CACHE(NvSub3D, NV30_TCL_PRIMITIVE_3D_VIEWPORT_XFRM_OX, 8);
	OUT_RING_CACHEf (v[MAT_TX]);
	OUT_RING_CACHEf (v[MAT_TY]);
	OUT_RING_CACHEf (v[MAT_TZ]);
	OUT_RING_CACHEf (0.0);
	OUT_RING_CACHEf (v[MAT_SX]);
	OUT_RING_CACHEf (v[MAT_SY]);
	OUT_RING_CACHEf (v[MAT_SZ]);
	OUT_RING_CACHEf (0.0);

	ctx->Driver.Scissor(ctx, ctx->Scissor.X, ctx->Scissor.Y,
	      		    ctx->Scissor.Width, ctx->Scissor.Height);
}

static GLboolean nv30InitCard(nouveauContextPtr nmesa)
{
	int i;
	nouveauObjectOnSubchannel(nmesa, NvSub3D, Nv3D);

	BEGIN_RING_SIZE(NvSub3D, NV30_TCL_PRIMITIVE_3D_SET_OBJECT1, 3);
	OUT_RING(NvDmaFB);
	OUT_RING(NvDmaAGP);
        OUT_RING(NvDmaFB);
	BEGIN_RING_SIZE(NvSub3D, NV30_TCL_PRIMITIVE_3D_SET_OBJECT8, 1);
	OUT_RING(NvDmaFB);
	BEGIN_RING_SIZE(NvSub3D, NV30_TCL_PRIMITIVE_3D_SET_OBJECT4, 2);
	OUT_RING(NvDmaFB);
	OUT_RING(NvDmaFB);
        BEGIN_RING_SIZE(NvSub3D, 0x1b0, 1); /* SET_OBJECT8B*/
        OUT_RING(NvDmaFB);

        for(i = 0x2c8; i <= 0x2fc; i += 4)
        {
            BEGIN_RING_SIZE(NvSub3D, i, 1);
            OUT_RING(0x0);
        }

	BEGIN_RING_SIZE(NvSub3D, 0x0220, 1);
	OUT_RING(1);

	BEGIN_RING_SIZE(NvSub3D, 0x03b0, 1);
	OUT_RING(0x00100000);
	BEGIN_RING_SIZE(NvSub3D, 0x1454, 1);
	OUT_RING(0);
	BEGIN_RING_SIZE(NvSub3D, 0x1d80, 1);
	OUT_RING(3);
	
	/* NEW */
       	BEGIN_RING_SIZE(NvSub3D, 0x1e98, 1);
        OUT_RING(0);
        BEGIN_RING_SIZE(NvSub3D, 0x17e0, 3);
        OUT_RING(0);
        OUT_RING(0);
        OUT_RING(0x3f800000);
        BEGIN_RING_SIZE(NvSub3D, 0x1f80, 16);
        OUT_RING(0); OUT_RING(0); OUT_RING(0); OUT_RING(0); 
        OUT_RING(0); OUT_RING(0); OUT_RING(0); OUT_RING(0); 
        OUT_RING(0x0000ffff);
        OUT_RING(0); OUT_RING(0); OUT_RING(0); OUT_RING(0); 
        OUT_RING(0); OUT_RING(0); OUT_RING(0); 
/*
        BEGIN_RING_SIZE(NvSub3D, 0x100, 2);
        OUT_RING(0);
        OUT_RING(0);
*/
        BEGIN_RING_SIZE(NvSub3D, 0x120, 3);
        OUT_RING(0);
        OUT_RING(1);
        OUT_RING(2);

        BEGIN_RING_SIZE(NvSub3D, 0x1d88, 1);
        OUT_RING(0x00001200);

	BEGIN_RING_SIZE(NvSub3D, NV30_TCL_PRIMITIVE_3D_RC_ENABLE, 1);
	OUT_RING       (0);

	return GL_TRUE;
}

static GLboolean nv40InitCard(nouveauContextPtr nmesa)
{
	nouveauObjectOnSubchannel(nmesa, NvSub3D, Nv3D);

	BEGIN_RING_SIZE(NvSub3D, NV30_TCL_PRIMITIVE_3D_SET_OBJECT1, 2);
	OUT_RING(NvDmaFB);
	OUT_RING(NvDmaFB);
	BEGIN_RING_SIZE(NvSub3D, NV30_TCL_PRIMITIVE_3D_SET_OBJECT8, 1);
	OUT_RING(NvDmaFB);
	BEGIN_RING_SIZE(NvSub3D, NV30_TCL_PRIMITIVE_3D_SET_OBJECT4, 2);
	OUT_RING(NvDmaFB);
	OUT_RING(NvDmaFB);
	BEGIN_RING_SIZE(NvSub3D, 0x0220, 1);
	OUT_RING(1);

	BEGIN_RING_SIZE(NvSub3D, 0x1ea4, 3);
	OUT_RING(0x00000010);
	OUT_RING(0x01000100);
	OUT_RING(0xff800006);
	BEGIN_RING_SIZE(NvSub3D, 0x1fc4, 1);
	OUT_RING(0x06144321);
	BEGIN_RING_SIZE(NvSub3D, 0x1fc8, 2);
	OUT_RING(0xedcba987);
	OUT_RING(0x00000021);
	BEGIN_RING_SIZE(NvSub3D, 0x1fd0, 1);
	OUT_RING(0x00171615);
	BEGIN_RING_SIZE(NvSub3D, 0x1fd4, 1);
	OUT_RING(0x001b1a19);

	BEGIN_RING_SIZE(NvSub3D, 0x1ef8, 1);
	OUT_RING(0x0020ffff);
	BEGIN_RING_SIZE(NvSub3D, 0x1d64, 1);
	OUT_RING(0x00d30000);
	BEGIN_RING_SIZE(NvSub3D, 0x1e94, 1);
	OUT_RING(0x00000001);

	return GL_TRUE;
}

static GLboolean nv30BindBuffers(nouveauContextPtr nmesa, int num_color,
		nouveau_renderbuffer **color,
		nouveau_renderbuffer *depth)
{
	GLuint x, y, w, h;

	w = color[0]->mesa.Width;
	h = color[0]->mesa.Height;
	x = nmesa->drawX;
	y = nmesa->drawY;

	if (num_color != 1)
		return GL_FALSE;
	BEGIN_RING_SIZE(NvSub3D, NV30_TCL_PRIMITIVE_3D_VIEWPORT_COLOR_BUFFER_DIM0, 5);
	OUT_RING        (((w+x)<<16)|x);
	OUT_RING        (((h+y)<<16)|y);
	if (color[0]->mesa._ActualFormat == GL_RGBA8)
		OUT_RING        (0x148);
	else
		OUT_RING        (0x143);
	if (nmesa->screen->card->type >= NV_40)
		OUT_RING        (color[0]->pitch);
	else
		OUT_RING        (color[0]->pitch | (depth ? (depth->pitch << 16): 0));
	OUT_RING        (color[0]->offset);

	if (depth) {
		BEGIN_RING_SIZE(NvSub3D, NV30_TCL_PRIMITIVE_3D_DEPTH_OFFSET, 1);
		OUT_RING        (depth->offset);
		if (nmesa->screen->card->type >= NV_40) {
			BEGIN_RING_SIZE(NvSub3D, NV30_TCL_PRIMITIVE_3D_LMA_DEPTH_BUFFER_PITCH, 1);
			OUT_RING        (depth->pitch);
		}
	}

	return GL_TRUE;
}

void nv30InitStateFuncs(GLcontext *ctx, struct dd_function_table *func)
{
	nouveauContextPtr nmesa = NOUVEAU_CONTEXT(ctx);

	func->AlphaFunc			= nv30AlphaFunc;
	func->BlendColor		= nv30BlendColor;
	func->BlendEquationSeparate	= nv30BlendEquationSeparate;
	func->BlendFuncSeparate		= nv30BlendFuncSeparate;
	func->Clear			= nv30Clear;
	func->ClearColor		= nv30ClearColor;
	func->ClearDepth		= nv30ClearDepth;
	func->ClearStencil		= nv30ClearStencil;
	func->ClipPlane			= nv30ClipPlane;
	func->ColorMask			= nv30ColorMask;
	func->ColorMaterial		= nv30ColorMaterial;
	func->CullFace			= nv30CullFace;
	func->FrontFace			= nv30FrontFace;
	func->DepthFunc			= nv30DepthFunc;
	func->DepthMask			= nv30DepthMask;
	func->DepthRange                = nv30DepthRange;
	func->Enable			= nv30Enable;
	func->Fogfv			= nv30Fogfv;
	func->Hint			= nv30Hint;
	func->Lightfv			= nv30Lightfv;
/*	func->LightModelfv		= nv30LightModelfv; */
	func->LineStipple		= nv30LineStipple;
	func->LineWidth			= nv30LineWidth;
	func->LogicOpcode		= nv30LogicOpcode;
	func->PointParameterfv		= nv30PointParameterfv;
	func->PointSize			= nv30PointSize;
	func->PolygonMode		= nv30PolygonMode;
	func->PolygonOffset		= nv30PolygonOffset;
	func->PolygonStipple		= nv30PolygonStipple;
#if 0
	func->ReadBuffer		= nv30ReadBuffer;
	func->RenderMode		= nv30RenderMode;
#endif
	func->Scissor			= nv30Scissor;
	func->ShadeModel		= nv30ShadeModel;
	func->StencilFuncSeparate	= nv30StencilFuncSeparate;
	func->StencilMaskSeparate	= nv30StencilMaskSeparate;
	func->StencilOpSeparate		= nv30StencilOpSeparate;
#if 0
	func->TexGen			= nv30TexGen;
	func->TexParameter		= nv30TexParameter;
#endif
	func->TextureMatrix		= nv30TextureMatrix;


	if (nmesa->screen->card->type >= NV_40)
	   nmesa->hw_func.InitCard	= nv40InitCard;
	else 
	   nmesa->hw_func.InitCard	= nv30InitCard;
	nmesa->hw_func.BindBuffers	= nv30BindBuffers;
	nmesa->hw_func.WindowMoved	= nv30WindowMoved;
}

