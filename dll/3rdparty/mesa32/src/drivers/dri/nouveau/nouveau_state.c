/**************************************************************************

Copyright 2006 Jeremy Kolb
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
#include "nouveau_state.h"
#include "nouveau_swtcl.h"
#include "nouveau_fifo.h"

#include "swrast/swrast.h"
#include "tnl/tnl.h"
#include "swrast_setup/swrast_setup.h"

#include "tnl/t_pipeline.h"

#include "mtypes.h"
#include "colormac.h"

static __inline__ GLuint nouveauPackColor(GLuint format,
				       GLubyte r, GLubyte g,
				       GLubyte b, GLubyte a)
{
   switch (format) {
   case 2:
      return PACK_COLOR_565( r, g, b );
   case 4:
      return PACK_COLOR_8888( r, g, b, a);
   default:
      fprintf(stderr, "unknown format %d\n", (int)format);
      return 0;
   }
}

static void nouveauCalcViewport(GLcontext *ctx)
{
    /* Calculate the Viewport Matrix */
    
    nouveauContextPtr nmesa = NOUVEAU_CONTEXT(ctx);
    const GLfloat *v = ctx->Viewport._WindowMap.m;
    GLfloat *m = nmesa->viewport.m;
    GLfloat xoffset = nmesa->drawX, yoffset = nmesa->drawY;
  
    nmesa->depth_scale = 1.0 / ctx->DrawBuffer->_DepthMaxF;

    m[MAT_SX] =   v[MAT_SX];
    m[MAT_TX] =   v[MAT_TX] + xoffset + SUBPIXEL_X;
    m[MAT_SY] = - v[MAT_SY];
    m[MAT_TY] =   v[MAT_TY] + yoffset + SUBPIXEL_Y;
    m[MAT_SZ] =   v[MAT_SZ] * nmesa->depth_scale;
    m[MAT_TZ] =   v[MAT_TZ] * nmesa->depth_scale;

    nmesa->hw_func.WindowMoved(nmesa);
}

static void nouveauViewport(GLcontext *ctx, GLint x, GLint y, GLsizei w, GLsizei h)
{
    /* 
     * Need to send (at least on an nv35 the following:
     * cons = 4 (this may be bytes per pixel)
     *
     * The viewport:
     * 445   0x0000bee0   {size: 0x0   channel: 0x1   cmd: 0x00009ee0} <-- VIEWPORT_SETUP/HEADER ?
     * 446   0x00000000   {size: 0x0   channel: 0x0   cmd: 0x00000000} <-- x * cons
     * 447   0x00000c80   {size: 0x0   channel: 0x0   cmd: 0x00000c80} <-- (height + x) * cons
     * 448   0x00000000   {size: 0x0   channel: 0x0   cmd: 0x00000000} <-- y * cons
     * 449   0x00000960   {size: 0x0   channel: 0x0   cmd: 0x00000960} <-- (width + y) * cons
     * 44a   0x00082a00   {size: 0x2   channel: 0x1   cmd: 0x00000a00} <-- VIEWPORT_DIMS
     * 44b   0x04000000  <-- (Width_from_glViewport << 16) | x
     * 44c   0x03000000  <-- (Height_from_glViewport << 16) | (win_height - height - y)
     *
     */
    
    nouveauCalcViewport(ctx);
}

static void nouveauDepthRange(GLcontext *ctx, GLclampd near, GLclampd far)
{
    nouveauCalcViewport(ctx);
}

static void nouveauDDUpdateHWState(GLcontext *ctx)
{
    nouveauContextPtr nmesa = NOUVEAU_CONTEXT(ctx);
    int new_state = nmesa->new_state;

    if ( new_state || nmesa->new_render_state & _NEW_TEXTURE )
    {
        nmesa->new_state = 0;

        /* Update the various parts of the context's state.
        */
        /*
        if ( new_state & NOUVEAU_NEW_ALPHA )
            nouveauUpdateAlphaMode( ctx );

        if ( new_state & NOUVEAU_NEW_DEPTH )
            nouveauUpdateZMode( ctx );

        if ( new_state & NOUVEAU_NEW_FOG )
            nouveauUpdateFogAttrib( ctx );

        if ( new_state & NOUVEAU_NEW_CLIP )
            nouveauUpdateClipping( ctx );

        if ( new_state & NOUVEAU_NEW_CULL )
            nouveauUpdateCull( ctx );

        if ( new_state & NOUVEAU_NEW_MASKS )
            nouveauUpdateMasks( ctx );

        if ( new_state & NOUVEAU_NEW_WINDOW )
            nouveauUpdateWindow( ctx );

        if ( nmesa->new_render_state & _NEW_TEXTURE ) {
            nouveauUpdateTextureState( ctx );
        }*/
    }
}

static void nouveauDDInvalidateState(GLcontext *ctx, GLuint new_state)
{
    _swrast_InvalidateState( ctx, new_state );
    _swsetup_InvalidateState( ctx, new_state );
    _vbo_InvalidateState( ctx, new_state );
    _tnl_InvalidateState( ctx, new_state );
    NOUVEAU_CONTEXT(ctx)->new_render_state |= new_state;
}

/* Initialize the context's hardware state. */
void nouveauDDInitState(nouveauContextPtr nmesa)
{
    uint32_t type = nmesa->screen->card->type;
    switch(type)
    {
        case NV_03:
            /* Unimplemented */
            break;
        case NV_04:
        case NV_05:
            nv04InitStateFuncs(nmesa->glCtx, &nmesa->glCtx->Driver);
            break;
        case NV_10:
            nv10InitStateFuncs(nmesa->glCtx, &nmesa->glCtx->Driver);
            break;
        case NV_20:
            nv20InitStateFuncs(nmesa->glCtx, &nmesa->glCtx->Driver);
            break;
        case NV_30:
        case NV_40:
        case NV_44:
            nv30InitStateFuncs(nmesa->glCtx, &nmesa->glCtx->Driver);
            break;
        case NV_50:
            nv50InitStateFuncs(nmesa->glCtx, &nmesa->glCtx->Driver);
            break;
        default:
            break;
    }
    nouveau_state_cache_init(nmesa);
}

/* Initialize the driver's state functions */
void nouveauDDInitStateFuncs(GLcontext *ctx)
{
   ctx->Driver.UpdateState		= nouveauDDInvalidateState;

   ctx->Driver.ClearIndex		= NULL;
   ctx->Driver.ClearColor		= NULL; //nouveauDDClearColor;
   ctx->Driver.ClearStencil		= NULL; //nouveauDDClearStencil;
   ctx->Driver.DrawBuffer		= NULL; //nouveauDDDrawBuffer;
   ctx->Driver.ReadBuffer		= NULL; //nouveauDDReadBuffer;

   ctx->Driver.IndexMask		= NULL;
   ctx->Driver.ColorMask		= NULL; //nouveauDDColorMask;
   ctx->Driver.AlphaFunc		= NULL; //nouveauDDAlphaFunc;
   ctx->Driver.BlendEquationSeparate	= NULL; //nouveauDDBlendEquationSeparate;
   ctx->Driver.BlendFuncSeparate	= NULL; //nouveauDDBlendFuncSeparate;
   ctx->Driver.ClearDepth		= NULL; //nouveauDDClearDepth;
   ctx->Driver.CullFace			= NULL; //nouveauDDCullFace;
   ctx->Driver.FrontFace		= NULL; //nouveauDDFrontFace;
   ctx->Driver.DepthFunc		= NULL; //nouveauDDDepthFunc;
   ctx->Driver.DepthMask		= NULL; //nouveauDDDepthMask;
   ctx->Driver.Enable			= NULL; //nouveauDDEnable;
   ctx->Driver.Fogfv			= NULL; //nouveauDDFogfv;
   ctx->Driver.Hint			= NULL;
   ctx->Driver.Lightfv			= NULL;
   ctx->Driver.LightModelfv		= NULL; //nouveauDDLightModelfv;
   ctx->Driver.LogicOpcode		= NULL; //nouveauDDLogicOpCode;
   ctx->Driver.PolygonMode		= NULL;
   ctx->Driver.PolygonStipple		= NULL; //nouveauDDPolygonStipple;
   ctx->Driver.RenderMode		= NULL; //nouveauDDRenderMode;
   ctx->Driver.Scissor			= NULL; //nouveauDDScissor;
   ctx->Driver.ShadeModel		= NULL; //nouveauDDShadeModel;
   ctx->Driver.StencilFuncSeparate	= NULL; //nouveauDDStencilFuncSeparate;
   ctx->Driver.StencilMaskSeparate	= NULL; //nouveauDDStencilMaskSeparate;
   ctx->Driver.StencilOpSeparate	= NULL; //nouveauDDStencilOpSeparate;

   ctx->Driver.DepthRange               = nouveauDepthRange;
   ctx->Driver.Viewport                 = nouveauViewport;

   /* Pixel path fallbacks.
    */
   ctx->Driver.Accum = _swrast_Accum;
   ctx->Driver.Bitmap = _swrast_Bitmap;
   ctx->Driver.CopyPixels = _swrast_CopyPixels;
   ctx->Driver.DrawPixels = _swrast_DrawPixels;
   ctx->Driver.ReadPixels = _swrast_ReadPixels;

   /* Swrast hooks for imaging extensions:
    */
   ctx->Driver.CopyColorTable = _swrast_CopyColorTable;
   ctx->Driver.CopyColorSubTable = _swrast_CopyColorSubTable;
   ctx->Driver.CopyConvolutionFilter1D = _swrast_CopyConvolutionFilter1D;
   ctx->Driver.CopyConvolutionFilter2D = _swrast_CopyConvolutionFilter2D;
}

#define STATE_INIT(a) if (ctx->Driver.a) ctx->Driver.a

void nouveauInitState(GLcontext *ctx)
{
    /*
     * Mesa should do this for us:
     */

    STATE_INIT(AlphaFunc)( ctx, 
            ctx->Color.AlphaFunc,
            ctx->Color.AlphaRef);

    STATE_INIT(BlendColor)( ctx,
            ctx->Color.BlendColor );

    STATE_INIT(BlendEquationSeparate)( ctx, 
            ctx->Color.BlendEquationRGB,
            ctx->Color.BlendEquationA);

    STATE_INIT(BlendFuncSeparate)( ctx,
            ctx->Color.BlendSrcRGB,
            ctx->Color.BlendDstRGB,
            ctx->Color.BlendSrcA,
            ctx->Color.BlendDstA);

    STATE_INIT(ClearColor)( ctx, ctx->Color.ClearColor);
    STATE_INIT(ClearDepth)( ctx, ctx->Depth.Clear);
    STATE_INIT(ClearStencil)( ctx, ctx->Stencil.Clear);

    STATE_INIT(ColorMask)( ctx, 
            ctx->Color.ColorMask[RCOMP],
            ctx->Color.ColorMask[GCOMP],
            ctx->Color.ColorMask[BCOMP],
            ctx->Color.ColorMask[ACOMP]);

    STATE_INIT(CullFace)( ctx, ctx->Polygon.CullFaceMode );
    STATE_INIT(DepthFunc)( ctx, ctx->Depth.Func );
    STATE_INIT(DepthMask)( ctx, ctx->Depth.Mask );

    STATE_INIT(Enable)( ctx, GL_ALPHA_TEST, ctx->Color.AlphaEnabled );
    STATE_INIT(Enable)( ctx, GL_BLEND, ctx->Color.BlendEnabled );
    STATE_INIT(Enable)( ctx, GL_COLOR_LOGIC_OP, ctx->Color.ColorLogicOpEnabled );
    STATE_INIT(Enable)( ctx, GL_COLOR_SUM, ctx->Fog.ColorSumEnabled );
    STATE_INIT(Enable)( ctx, GL_CULL_FACE, ctx->Polygon.CullFlag );
    STATE_INIT(Enable)( ctx, GL_DEPTH_TEST, ctx->Depth.Test );
    STATE_INIT(Enable)( ctx, GL_DITHER, ctx->Color.DitherFlag );
    STATE_INIT(Enable)( ctx, GL_FOG, ctx->Fog.Enabled );
    STATE_INIT(Enable)( ctx, GL_LIGHTING, ctx->Light.Enabled );
    STATE_INIT(Enable)( ctx, GL_LINE_SMOOTH, ctx->Line.SmoothFlag );
    STATE_INIT(Enable)( ctx, GL_LINE_STIPPLE, ctx->Line.StippleFlag );
    STATE_INIT(Enable)( ctx, GL_POINT_SMOOTH, ctx->Point.SmoothFlag );
    STATE_INIT(Enable)( ctx, GL_POLYGON_OFFSET_FILL, ctx->Polygon.OffsetFill);
    STATE_INIT(Enable)( ctx, GL_POLYGON_OFFSET_LINE, ctx->Polygon.OffsetLine);
    STATE_INIT(Enable)( ctx, GL_POLYGON_OFFSET_POINT, ctx->Polygon.OffsetPoint);
    STATE_INIT(Enable)( ctx, GL_POLYGON_SMOOTH, ctx->Polygon.SmoothFlag );
    STATE_INIT(Enable)( ctx, GL_POLYGON_STIPPLE, ctx->Polygon.StippleFlag );
    STATE_INIT(Enable)( ctx, GL_SCISSOR_TEST, ctx->Scissor.Enabled );
    STATE_INIT(Enable)( ctx, GL_STENCIL_TEST, ctx->Stencil.Enabled );
    STATE_INIT(Enable)( ctx, GL_TEXTURE_1D, GL_FALSE );
    STATE_INIT(Enable)( ctx, GL_TEXTURE_2D, GL_FALSE );
    STATE_INIT(Enable)( ctx, GL_TEXTURE_RECTANGLE_NV, GL_FALSE );
    STATE_INIT(Enable)( ctx, GL_TEXTURE_3D, GL_FALSE );
    STATE_INIT(Enable)( ctx, GL_TEXTURE_CUBE_MAP, GL_FALSE );

    STATE_INIT(Fogfv)( ctx, GL_FOG_COLOR, ctx->Fog.Color );
    STATE_INIT(Fogfv)( ctx, GL_FOG_MODE, 0 );
    STATE_INIT(Fogfv)( ctx, GL_FOG_DENSITY, &ctx->Fog.Density );
    STATE_INIT(Fogfv)( ctx, GL_FOG_START, &ctx->Fog.Start );
    STATE_INIT(Fogfv)( ctx, GL_FOG_END, &ctx->Fog.End );

    STATE_INIT(FrontFace)( ctx, ctx->Polygon.FrontFace );

    {
        GLfloat f = (GLfloat)ctx->Light.Model.ColorControl;
        STATE_INIT(LightModelfv)( ctx, GL_LIGHT_MODEL_COLOR_CONTROL, &f );
    }

    STATE_INIT(LineStipple)( ctx, ctx->Line.StippleFactor, ctx->Line.StipplePattern );
    STATE_INIT(LineWidth)( ctx, ctx->Line.Width );
    STATE_INIT(LogicOpcode)( ctx, ctx->Color.LogicOp );
    STATE_INIT(PointSize)( ctx, ctx->Point.Size );
    STATE_INIT(PolygonMode)( ctx, GL_FRONT, ctx->Polygon.FrontMode );
    STATE_INIT(PolygonMode)( ctx, GL_BACK, ctx->Polygon.BackMode );
    STATE_INIT(PolygonOffset)( ctx,
	    ctx->Polygon.OffsetFactor,
	    ctx->Polygon.OffsetUnits );
    STATE_INIT(PolygonStipple)( ctx, (const GLubyte *)ctx->PolygonStipple );
    STATE_INIT(ShadeModel)( ctx, ctx->Light.ShadeModel );
    STATE_INIT(StencilFuncSeparate)( ctx, GL_FRONT,
            ctx->Stencil.Function[0],
            ctx->Stencil.Ref[0],
            ctx->Stencil.ValueMask[0] );
    STATE_INIT(StencilFuncSeparate)( ctx, GL_BACK,
            ctx->Stencil.Function[1],
            ctx->Stencil.Ref[1],
            ctx->Stencil.ValueMask[1] );
    STATE_INIT(StencilMaskSeparate)( ctx, GL_FRONT, ctx->Stencil.WriteMask[0] );
    STATE_INIT(StencilMaskSeparate)( ctx, GL_BACK, ctx->Stencil.WriteMask[1] );
    STATE_INIT(StencilOpSeparate)( ctx, GL_FRONT,
            ctx->Stencil.FailFunc[0],
            ctx->Stencil.ZFailFunc[0],
            ctx->Stencil.ZPassFunc[0]);
    STATE_INIT(StencilOpSeparate)( ctx, GL_BACK,
            ctx->Stencil.FailFunc[1],
            ctx->Stencil.ZFailFunc[1],
            ctx->Stencil.ZPassFunc[1]);
}
