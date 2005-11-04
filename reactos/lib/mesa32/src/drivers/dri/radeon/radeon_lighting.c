/* $XFree86: xc/lib/GL/mesa/src/drv/radeon/radeon_state.c,v 1.5 2002/09/16 18:05:20 eich Exp $ */
/*
 * Copyright 2000, 2001 VA Linux Systems Inc., Fremont, California.
 *
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * on the rights to use, copy, modify, merge, publish, distribute, sub
 * license, and/or sell copies of the Software, and to permit persons to whom
 * the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * VA LINUX SYSTEMS AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * Authors:
 *    Gareth Hughes <gareth@valinux.com>
 *    Keith Whitwell <keith@tungstengraphics.com>
 */

#include "glheader.h"
#include "imports.h"
#include "api_arrayelt.h"
/* #include "mmath.h" */
#include "enums.h"
#include "colormac.h"


#include "radeon_context.h"
#include "radeon_ioctl.h"
#include "radeon_state.h"
#include "radeon_tcl.h"
#include "radeon_tex.h"
#include "radeon_vtxfmt.h"



/* =============================================================
 * Materials
 */


/* Update on colormaterial, material emmissive/ambient, 
 * lightmodel.globalambient
 */
void update_global_ambient( GLcontext *ctx )
{
   radeonContextPtr rmesa = RADEON_CONTEXT(ctx);
   float *fcmd = (float *)RADEON_DB_STATE( glt );

   /* Need to do more if both emmissive & ambient are PREMULT:
    */
   if ((rmesa->hw.tcl.cmd[TCL_LIGHT_MODEL_CTL] &
       ((3 << RADEON_EMISSIVE_SOURCE_SHIFT) |
	(3 << RADEON_AMBIENT_SOURCE_SHIFT))) == 0) 
   {
      COPY_3V( &fcmd[GLT_RED], 
	       ctx->Light.Material[0].Emission);
      ACC_SCALE_3V( &fcmd[GLT_RED],
		   ctx->Light.Model.Ambient,
		   ctx->Light.Material[0].Ambient);
   } 
   else
   {
      COPY_3V( &fcmd[GLT_RED], ctx->Light.Model.Ambient );
   }
   
   RADEON_DB_STATECHANGE(rmesa, &rmesa->hw.glt);
}

/* Update on change to 
 *    - light[p].colors
 *    - light[p].enabled
 *    - material,
 *    - colormaterial enabled
 *    - colormaterial bitmask
 */
void update_light_colors( GLcontext *ctx, GLuint p )
{
   struct gl_light *l = &ctx->Light.Light[p];

/*     fprintf(stderr, "%s\n", __FUNCTION__); */

   if (l->Enabled) {
      radeonContextPtr rmesa = RADEON_CONTEXT(ctx);
      float *fcmd = (float *)RADEON_DB_STATE( lit[p] );
      GLuint bitmask = ctx->Light.ColorMaterialBitmask;
      struct gl_material *mat = &ctx->Light.Material[0];

      COPY_4V( &fcmd[LIT_AMBIENT_RED], l->Ambient );	 
      COPY_4V( &fcmd[LIT_DIFFUSE_RED], l->Diffuse );
      COPY_4V( &fcmd[LIT_SPECULAR_RED], l->Specular );
      
      if (!ctx->Light.ColorMaterialEnabled)
	 bitmask = 0;

      if ((bitmask & FRONT_AMBIENT_BIT) == 0) 
	 SELF_SCALE_3V( &fcmd[LIT_AMBIENT_RED], mat->Ambient );

      if ((bitmask & FRONT_DIFFUSE_BIT) == 0) 
	 SELF_SCALE_3V( &fcmd[LIT_DIFFUSE_RED], mat->Diffuse );
      
      if ((bitmask & FRONT_SPECULAR_BIT) == 0) 
	 SELF_SCALE_3V( &fcmd[LIT_SPECULAR_RED], mat->Specular );

      RADEON_DB_STATECHANGE( rmesa, &rmesa->hw.lit[p] );
   }
}

/* Also fallback for asym colormaterial mode in twoside lighting...
 */
void check_twoside_fallback( GLcontext *ctx )
{
   GLboolean fallback = GL_FALSE;

   if (ctx->Light.Enabled && ctx->Light.Model.TwoSide) {
      if (memcmp( &ctx->Light.Material[0],
		  &ctx->Light.Material[1],
		  sizeof(struct gl_material)) != 0)
	 fallback = GL_TRUE;  
      else if (ctx->Light.ColorMaterialEnabled &&
	       (ctx->Light.ColorMaterialBitmask & BACK_MATERIAL_BITS) != 
	       ((ctx->Light.ColorMaterialBitmask & FRONT_MATERIAL_BITS)<<1))
	 fallback = GL_TRUE;
   }

   TCL_FALLBACK( ctx, RADEON_TCL_FALLBACK_LIGHT_TWOSIDE, fallback );
}

void radeonColorMaterial( GLcontext *ctx, GLenum face, GLenum mode )
{
   if (ctx->Light.ColorMaterialEnabled) {
      radeonContextPtr rmesa = RADEON_CONTEXT(ctx);
      GLuint light_model_ctl = rmesa->hw.tcl.cmd[TCL_LIGHT_MODEL_CTL];
      GLuint mask = ctx->Light.ColorMaterialBitmask;

      /* Default to PREMULT:
       */
      light_model_ctl &= ~((3 << RADEON_EMISSIVE_SOURCE_SHIFT) |
			   (3 << RADEON_AMBIENT_SOURCE_SHIFT) |
			   (3 << RADEON_DIFFUSE_SOURCE_SHIFT) |
			   (3 << RADEON_SPECULAR_SOURCE_SHIFT)); 
   
      if (mask & FRONT_EMISSION_BIT) {
	 light_model_ctl |= (RADEON_LM_SOURCE_VERTEX_DIFFUSE <<
			     RADEON_EMISSIVE_SOURCE_SHIFT);
      }

      if (mask & FRONT_AMBIENT_BIT) {
	 light_model_ctl |= (RADEON_LM_SOURCE_VERTEX_DIFFUSE <<
			     RADEON_AMBIENT_SOURCE_SHIFT);
      }
	 
      if (mask & FRONT_DIFFUSE_BIT) {
	 light_model_ctl |= (RADEON_LM_SOURCE_VERTEX_DIFFUSE <<
			     RADEON_DIFFUSE_SOURCE_SHIFT);
      }
   
      if (mask & FRONT_SPECULAR_BIT) {
	 light_model_ctl |= (RADEON_LM_SOURCE_VERTEX_DIFFUSE <<
			     RADEON_SPECULAR_SOURCE_SHIFT);
      }
   
      if (light_model_ctl != rmesa->hw.tcl.cmd[TCL_LIGHT_MODEL_CTL]) {
	 GLuint p;

	 RADEON_STATECHANGE( rmesa, tcl );
	 rmesa->hw.tcl.cmd[TCL_LIGHT_MODEL_CTL] = light_model_ctl;      

	 for (p = 0 ; p < MAX_LIGHTS; p++) 
	    update_light_colors( ctx, p );
	 update_global_ambient( ctx );
      }
   }
   
   check_twoside_fallback( ctx );
}

void radeonUpdateMaterial( GLcontext *ctx )
{
   radeonContextPtr rmesa = RADEON_CONTEXT(ctx);
   GLfloat *fcmd = (GLfloat *)RADEON_DB_STATE( mtl );
   GLuint p;
   GLuint mask = ~0;
   
   if (ctx->Light.ColorMaterialEnabled)
      mask &= ~ctx->Light.ColorMaterialBitmask;

   if (RADEON_DEBUG & DEBUG_STATE)
      fprintf(stderr, "%s\n", __FUNCTION__);

      
   if (mask & FRONT_EMISSION_BIT) {
      fcmd[MTL_EMMISSIVE_RED]   = ctx->Light.Material[0].Emission[0];
      fcmd[MTL_EMMISSIVE_GREEN] = ctx->Light.Material[0].Emission[1];
      fcmd[MTL_EMMISSIVE_BLUE]  = ctx->Light.Material[0].Emission[2];
      fcmd[MTL_EMMISSIVE_ALPHA] = ctx->Light.Material[0].Emission[3];
   }
   if (mask & FRONT_AMBIENT_BIT) {
      fcmd[MTL_AMBIENT_RED]     = ctx->Light.Material[0].Ambient[0];
      fcmd[MTL_AMBIENT_GREEN]   = ctx->Light.Material[0].Ambient[1];
      fcmd[MTL_AMBIENT_BLUE]    = ctx->Light.Material[0].Ambient[2];
      fcmd[MTL_AMBIENT_ALPHA]   = ctx->Light.Material[0].Ambient[3];
   }
   if (mask & FRONT_DIFFUSE_BIT) {
      fcmd[MTL_DIFFUSE_RED]     = ctx->Light.Material[0].Diffuse[0];
      fcmd[MTL_DIFFUSE_GREEN]   = ctx->Light.Material[0].Diffuse[1];
      fcmd[MTL_DIFFUSE_BLUE]    = ctx->Light.Material[0].Diffuse[2];
      fcmd[MTL_DIFFUSE_ALPHA]   = ctx->Light.Material[0].Diffuse[3];
   }
   if (mask & FRONT_SPECULAR_BIT) {
      fcmd[MTL_SPECULAR_RED]    = ctx->Light.Material[0].Specular[0];
      fcmd[MTL_SPECULAR_GREEN]  = ctx->Light.Material[0].Specular[1];
      fcmd[MTL_SPECULAR_BLUE]   = ctx->Light.Material[0].Specular[2];
      fcmd[MTL_SPECULAR_ALPHA]  = ctx->Light.Material[0].Specular[3];
   }
   if (mask & FRONT_SHININESS_BIT) {
      fcmd[MTL_SHININESS]       = ctx->Light.Material[0].Shininess;
   }

   if (RADEON_DB_STATECHANGE( rmesa, &rmesa->hw.mtl )) {
      for (p = 0 ; p < MAX_LIGHTS; p++) 
	 update_light_colors( ctx, p );

      check_twoside_fallback( ctx );
      update_global_ambient( ctx );
   }
   else if (RADEON_DEBUG & (DEBUG_PRIMS|DEBUG_STATE))
      fprintf(stderr, "%s: Elided noop material call\n", __FUNCTION__);
}

/* _NEW_LIGHT
 * _NEW_MODELVIEW
 * _MESA_NEW_NEED_EYE_COORDS
 *
 * Uses derived state from mesa:
 *       _VP_inf_norm
 *       _h_inf_norm
 *       _Position
 *       _NormDirection
 *       _ModelViewInvScale
 *       _NeedEyeCoords
 *       _EyeZDir
 *
 * which are calculated in light.c and are correct for the current
 * lighting space (model or eye), hence dependencies on _NEW_MODELVIEW
 * and _MESA_NEW_NEED_EYE_COORDS.  
 */
void radeonUpdateLighting( GLcontext *ctx )
{
   radeonContextPtr rmesa = RADEON_CONTEXT(ctx);

   /* Have to check these, or have an automatic shortcircuit mechanism
    * to remove noop statechanges. (Or just do a better job on the
    * front end).
    */
   {
      GLuint tmp = rmesa->hw.tcl.cmd[TCL_LIGHT_MODEL_CTL];

      if (ctx->_NeedEyeCoords)
	 tmp &= ~RADEON_LIGHT_IN_MODELSPACE;
      else
	 tmp |= RADEON_LIGHT_IN_MODELSPACE;
      

      /* Leave this test disabled: (unexplained q3 lockup) (even with
         new packets)
      */
      if (tmp != rmesa->hw.tcl.cmd[TCL_LIGHT_MODEL_CTL]) 
      {
	 RADEON_STATECHANGE( rmesa, tcl );
	 rmesa->hw.tcl.cmd[TCL_LIGHT_MODEL_CTL] = tmp;
      }
   }

   {
      GLfloat *fcmd = (GLfloat *)RADEON_DB_STATE( eye );
      fcmd[EYE_X] = ctx->_EyeZDir[0];
      fcmd[EYE_Y] = ctx->_EyeZDir[1];
      fcmd[EYE_Z] = - ctx->_EyeZDir[2];
      fcmd[EYE_RESCALE_FACTOR] = ctx->_ModelViewInvScale;
      RADEON_DB_STATECHANGE( rmesa, &rmesa->hw.eye );
   }


/*     RADEON_STATECHANGE( rmesa, glt ); */

   if (ctx->Light.Enabled) {
      GLint p;
      for (p = 0 ; p < MAX_LIGHTS; p++) {
	 if (ctx->Light.Light[p].Enabled) {
	    struct gl_light *l = &ctx->Light.Light[p];
	    GLfloat *fcmd = (GLfloat *)RADEON_DB_STATE( lit[p] );
	    
	    if (l->EyePosition[3] == 0.0) {
	       COPY_3FV( &fcmd[LIT_POSITION_X], l->_VP_inf_norm ); 
	       COPY_3FV( &fcmd[LIT_DIRECTION_X], l->_h_inf_norm ); 
	       fcmd[LIT_POSITION_W] = 0;
	       fcmd[LIT_DIRECTION_W] = 0;
	    } else {
	       COPY_4V( &fcmd[LIT_POSITION_X], l->_Position );
	       fcmd[LIT_DIRECTION_X] = -l->_NormDirection[0];
	       fcmd[LIT_DIRECTION_Y] = -l->_NormDirection[1];
	       fcmd[LIT_DIRECTION_Z] = -l->_NormDirection[2];
	       fcmd[LIT_DIRECTION_W] = 0;
	    }

	    RADEON_DB_STATECHANGE( rmesa, &rmesa->hw.lit[p] );
	 }
      }
   }
}


void radeonLightfv( GLcontext *ctx, GLenum light,
		    GLenum pname, const GLfloat *params )
{
   radeonContextPtr rmesa = RADEON_CONTEXT(ctx);
   GLint p = light - GL_LIGHT0;
   struct gl_light *l = &ctx->Light.Light[p];
   GLfloat *fcmd = (GLfloat *)rmesa->hw.lit[p].cmd;
   

   switch (pname) {
   case GL_AMBIENT:		
   case GL_DIFFUSE:
   case GL_SPECULAR:
      update_light_colors( ctx, p );
      break;

   case GL_SPOT_DIRECTION: 
      /* picked up in update_light */	
      break;

   case GL_POSITION: {
      /* positions picked up in update_light, but can do flag here */	
      GLuint flag = (p&1)? RADEON_LIGHT_1_IS_LOCAL : RADEON_LIGHT_0_IS_LOCAL;
      GLuint idx = TCL_PER_LIGHT_CTL_0 + p/2;

      RADEON_STATECHANGE(rmesa, tcl);
      if (l->EyePosition[3] != 0.0F)
	 rmesa->hw.tcl.cmd[idx] |= flag;
      else
	 rmesa->hw.tcl.cmd[idx] &= ~flag;
      break;
   }

   case GL_SPOT_EXPONENT:
      RADEON_STATECHANGE(rmesa, lit[p]);
      fcmd[LIT_SPOT_EXPONENT] = params[0];
      break;

   case GL_SPOT_CUTOFF: {
      GLuint flag = (p&1) ? RADEON_LIGHT_1_IS_SPOT : RADEON_LIGHT_0_IS_SPOT;
      GLuint idx = TCL_PER_LIGHT_CTL_0 + p/2;

      RADEON_STATECHANGE(rmesa, lit[p]);
      fcmd[LIT_SPOT_CUTOFF] = l->_CosCutoff;

      RADEON_STATECHANGE(rmesa, tcl);
      if (l->SpotCutoff != 180.0F)
	 rmesa->hw.tcl.cmd[idx] |= flag;
      else
	 rmesa->hw.tcl.cmd[idx] &= ~flag;
      break;
   }

   case GL_CONSTANT_ATTENUATION:
      RADEON_STATECHANGE(rmesa, lit[p]);
      fcmd[LIT_ATTEN_CONST] = params[0];
      break;
   case GL_LINEAR_ATTENUATION:
      RADEON_STATECHANGE(rmesa, lit[p]);
      fcmd[LIT_ATTEN_LINEAR] = params[0];
      break;
   case GL_QUADRATIC_ATTENUATION:
      RADEON_STATECHANGE(rmesa, lit[p]);
      fcmd[LIT_ATTEN_QUADRATIC] = params[0];
      break;
   default:
      return;
   }

}

		  


void radeonLightModelfv( GLcontext *ctx, GLenum pname,
			 const GLfloat *param )
{
   radeonContextPtr rmesa = RADEON_CONTEXT(ctx);

   switch (pname) {
      case GL_LIGHT_MODEL_AMBIENT: 
	 update_global_ambient( ctx );
	 break;

      case GL_LIGHT_MODEL_LOCAL_VIEWER:
	 RADEON_STATECHANGE( rmesa, tcl );
	 if (ctx->Light.Model.LocalViewer)
	    rmesa->hw.tcl.cmd[TCL_LIGHT_MODEL_CTL] |= RADEON_LOCAL_VIEWER;
	 else
	    rmesa->hw.tcl.cmd[TCL_LIGHT_MODEL_CTL] &= ~RADEON_LOCAL_VIEWER;
         break;

      case GL_LIGHT_MODEL_TWO_SIDE:
	 RADEON_STATECHANGE( rmesa, tcl );
	 if (ctx->Light.Model.TwoSide)
	    rmesa->hw.tcl.cmd[TCL_UCP_VERT_BLEND_CTL] |= RADEON_LIGHT_TWOSIDE;
	 else
	    rmesa->hw.tcl.cmd[TCL_UCP_VERT_BLEND_CTL] &= ~RADEON_LIGHT_TWOSIDE;

	 check_twoside_fallback( ctx );

#if _HAVE_SWTNL
	 if (rmesa->TclFallback) {
	    radeonChooseRenderState( ctx );
	    radeonChooseVertexState( ctx );
	 }
#endif
         break;

      case GL_LIGHT_MODEL_COLOR_CONTROL:
	 radeonUpdateSpecular(ctx);

	 RADEON_STATECHANGE( rmesa, tcl );
	 if (ctx->Light.Model.ColorControl == GL_SEPARATE_SPECULAR_COLOR) 
	    rmesa->hw.tcl.cmd[TCL_LIGHT_MODEL_CTL] &= 
	       ~RADEON_DIFFUSE_SPECULAR_COMBINE;
	 else
	    rmesa->hw.tcl.cmd[TCL_LIGHT_MODEL_CTL] |= 
	       RADEON_DIFFUSE_SPECULAR_COMBINE;
         break;

      default:
         break;
   }
}


/* =============================================================
 * Fog
 */


static void radeonFogfv( GLcontext *ctx, GLenum pname, const GLfloat *param )
{
   radeonContextPtr rmesa = RADEON_CONTEXT(ctx);
   union { int i; float f; } c, d;
   GLchan col[4];

   c.i = rmesa->hw.fog.cmd[FOG_C];
   d.i = rmesa->hw.fog.cmd[FOG_D];

   switch (pname) {
   case GL_FOG_MODE:
      if (!ctx->Fog.Enabled)
	 return;
      RADEON_STATECHANGE(rmesa, tcl);
      rmesa->hw.tcl.cmd[TCL_UCP_VERT_BLEND_CTL] &= ~RADEON_TCL_FOG_MASK;
      switch (ctx->Fog.Mode) {
      case GL_LINEAR:
	 rmesa->hw.tcl.cmd[TCL_UCP_VERT_BLEND_CTL] |= RADEON_TCL_FOG_LINEAR;
	 if (ctx->Fog.Start == ctx->Fog.End) {
	    c.f = 1.0F;
	    d.f = 1.0F;
	 }
	 else {
	    c.f = ctx->Fog.End/(ctx->Fog.End-ctx->Fog.Start);
	    d.f = 1.0/(ctx->Fog.End-ctx->Fog.Start);
	 }
	 break;
      case GL_EXP:
	 rmesa->hw.tcl.cmd[TCL_UCP_VERT_BLEND_CTL] |= RADEON_TCL_FOG_EXP;
	 c.f = 0.0;
	 d.f = ctx->Fog.Density;
	 break;
      case GL_EXP2:
	 rmesa->hw.tcl.cmd[TCL_UCP_VERT_BLEND_CTL] |= RADEON_TCL_FOG_EXP2;
	 c.f = 0.0;
	 d.f = -(ctx->Fog.Density * ctx->Fog.Density);
	 break;
      default:
	 return;
      }
      break;
   case GL_FOG_DENSITY:
      switch (ctx->Fog.Mode) {
      case GL_EXP:
	 c.f = 0.0;
	 d.f = ctx->Fog.Density;
	 break;
      case GL_EXP2:
	 c.f = 0.0;
	 d.f = -(ctx->Fog.Density * ctx->Fog.Density);
	 break;
      default:
	 break;
      }
      break;
   case GL_FOG_START:
   case GL_FOG_END:
      if (ctx->Fog.Mode == GL_LINEAR) {
	 if (ctx->Fog.Start == ctx->Fog.End) {
	    c.f = 1.0F;
	    d.f = 1.0F;
	 } else {
	    c.f = ctx->Fog.End/(ctx->Fog.End-ctx->Fog.Start);
	    d.f = 1.0/(ctx->Fog.End-ctx->Fog.Start);
	 }
      }
      break;
   case GL_FOG_COLOR: 
      RADEON_STATECHANGE( rmesa, ctx );
      UNCLAMPED_FLOAT_TO_RGB_CHAN( col, ctx->Fog.Color );
      rmesa->hw.ctx.cmd[CTX_PP_FOG_COLOR] =
	 radeonPackColor( 4, col[0], col[1], col[2], 0 );
      break;
   case GL_FOG_COORDINATE_SOURCE_EXT: 
      /* What to do?
       */
      break;
   default:
      return;
   }

   if (c.i != rmesa->hw.fog.cmd[FOG_C] || d.i != rmesa->hw.fog.cmd[FOG_D]) {
      RADEON_STATECHANGE( rmesa, fog );
      rmesa->hw.fog.cmd[FOG_C] = c.i;
      rmesa->hw.fog.cmd[FOG_D] = d.i;
   }
}

/* Examine lighting and texture state to determine if separate specular
 * should be enabled.
 */
void radeonUpdateSpecular( GLcontext *ctx )
{
   radeonContextPtr rmesa = RADEON_CONTEXT(ctx);
   GLuint p = rmesa->hw.ctx.cmd[CTX_PP_CNTL];

   if (NEED_SECONDARY_COLOR(ctx)) {
      p |=  RADEON_SPECULAR_ENABLE;
   } else {
      p &= ~RADEON_SPECULAR_ENABLE;
   }

   if ( rmesa->hw.ctx.cmd[CTX_PP_CNTL] != p ) {
      RADEON_STATECHANGE( rmesa, ctx );
      rmesa->hw.ctx.cmd[CTX_PP_CNTL] = p;
   }

   /* Bizzare: have to leave lighting enabled to get fog.
    */
   RADEON_STATECHANGE( rmesa, tcl );
   if ((ctx->Light.Enabled &&
	ctx->Light.Model.ColorControl == GL_SEPARATE_SPECULAR_COLOR)) {
      rmesa->hw.tcl.cmd[TCL_OUTPUT_VTXSEL] |= RADEON_TCL_COMPUTE_SPECULAR;
      rmesa->hw.tcl.cmd[TCL_OUTPUT_VTXSEL] |= RADEON_TCL_COMPUTE_DIFFUSE;
      rmesa->hw.tcl.cmd[TCL_OUTPUT_VTXFMT] |= RADEON_TCL_VTX_PK_SPEC;
      rmesa->hw.tcl.cmd[TCL_OUTPUT_VTXFMT] |= RADEON_TCL_VTX_PK_DIFFUSE;
      rmesa->hw.tcl.cmd[TCL_LIGHT_MODEL_CTL] |= RADEON_LIGHTING_ENABLE;
   }
   else if (ctx->Fog.Enabled) {
      if (ctx->Light.Enabled) {
	 rmesa->hw.tcl.cmd[TCL_OUTPUT_VTXSEL] |= RADEON_TCL_COMPUTE_SPECULAR;
	 rmesa->hw.tcl.cmd[TCL_OUTPUT_VTXSEL] |= RADEON_TCL_COMPUTE_DIFFUSE;
	 rmesa->hw.tcl.cmd[TCL_OUTPUT_VTXFMT] |= RADEON_TCL_VTX_PK_SPEC;
	 rmesa->hw.tcl.cmd[TCL_OUTPUT_VTXFMT] |= RADEON_TCL_VTX_PK_DIFFUSE;
	 rmesa->hw.tcl.cmd[TCL_LIGHT_MODEL_CTL] |= RADEON_LIGHTING_ENABLE;
      } else {
	 rmesa->hw.tcl.cmd[TCL_OUTPUT_VTXSEL] |= RADEON_TCL_COMPUTE_SPECULAR;
	 rmesa->hw.tcl.cmd[TCL_OUTPUT_VTXSEL] &= ~RADEON_TCL_COMPUTE_DIFFUSE;
	 rmesa->hw.tcl.cmd[TCL_OUTPUT_VTXFMT] |= RADEON_TCL_VTX_PK_SPEC;
	 rmesa->hw.tcl.cmd[TCL_OUTPUT_VTXFMT] |= RADEON_TCL_VTX_PK_DIFFUSE;
	 rmesa->hw.tcl.cmd[TCL_LIGHT_MODEL_CTL] |= RADEON_LIGHTING_ENABLE;
      }
   }
   else if (ctx->Light.Enabled) {
      rmesa->hw.tcl.cmd[TCL_OUTPUT_VTXSEL] &= ~RADEON_TCL_COMPUTE_SPECULAR;
      rmesa->hw.tcl.cmd[TCL_OUTPUT_VTXSEL] |= RADEON_TCL_COMPUTE_DIFFUSE;
      rmesa->hw.tcl.cmd[TCL_OUTPUT_VTXFMT] &= ~RADEON_TCL_VTX_PK_SPEC;
      rmesa->hw.tcl.cmd[TCL_OUTPUT_VTXFMT] |= RADEON_TCL_VTX_PK_DIFFUSE;
      rmesa->hw.tcl.cmd[TCL_LIGHT_MODEL_CTL] |= RADEON_LIGHTING_ENABLE;
   } else if (ctx->Fog.ColorSumEnabled ) {
      rmesa->hw.tcl.cmd[TCL_OUTPUT_VTXSEL] &= ~RADEON_TCL_COMPUTE_SPECULAR;
      rmesa->hw.tcl.cmd[TCL_OUTPUT_VTXSEL] &= ~RADEON_TCL_COMPUTE_DIFFUSE;
      rmesa->hw.tcl.cmd[TCL_OUTPUT_VTXFMT] |= RADEON_TCL_VTX_PK_SPEC;
      rmesa->hw.tcl.cmd[TCL_OUTPUT_VTXFMT] |= RADEON_TCL_VTX_PK_DIFFUSE;
      rmesa->hw.tcl.cmd[TCL_LIGHT_MODEL_CTL] &= ~RADEON_LIGHTING_ENABLE;
   } else {
      rmesa->hw.tcl.cmd[TCL_OUTPUT_VTXSEL] &= ~RADEON_TCL_COMPUTE_SPECULAR;
      rmesa->hw.tcl.cmd[TCL_OUTPUT_VTXSEL] &= ~RADEON_TCL_COMPUTE_DIFFUSE;
      rmesa->hw.tcl.cmd[TCL_OUTPUT_VTXFMT] &= ~RADEON_TCL_VTX_PK_SPEC;
      rmesa->hw.tcl.cmd[TCL_OUTPUT_VTXFMT] |= RADEON_TCL_VTX_PK_DIFFUSE;
      rmesa->hw.tcl.cmd[TCL_LIGHT_MODEL_CTL] &= ~RADEON_LIGHTING_ENABLE;
   }

#if _HAVE_SWTNL
   /* Update vertex/render formats
    */
   if (rmesa->TclFallback) { 
      radeonChooseRenderState( ctx );
      radeonChooseVertexState( ctx );
   }
#endif
}



static void radeonLightingSpaceChange( GLcontext *ctx )
{
   radeonContextPtr rmesa = RADEON_CONTEXT(ctx);
   GLboolean tmp;
   RADEON_STATECHANGE( rmesa, tcl );

   if (RADEON_DEBUG & DEBUG_STATE)
      fprintf(stderr, "%s %d\n", __FUNCTION__, ctx->_NeedEyeCoords);

   if (ctx->_NeedEyeCoords)
      tmp = ctx->Transform.RescaleNormals;
   else
      tmp = !ctx->Transform.RescaleNormals;

   if ( tmp ) {
      rmesa->hw.tcl.cmd[TCL_LIGHT_MODEL_CTL] |=  RADEON_RESCALE_NORMALS;
   } else {
      rmesa->hw.tcl.cmd[TCL_LIGHT_MODEL_CTL] &= ~RADEON_RESCALE_NORMALS;
   }
}

void radeonInitLightStateFuncs( GLcontext *ctx )
{
   radeonContextPtr rmesa = RADEON_CONTEXT(ctx);
   int i;

   ctx->Driver.LightModelfv		= radeonLightModelfv; 
   ctx->Driver.Lightfv			= radeonLightfv; 
   ctx->Driver.Fogfv			= radeonFogfv;
   ctx->Driver.LightingSpaceChange      = radeonLightingSpaceChange;

   for (i = 0 ; i < 8; i++) {
      struct gl_light *l = &ctx->Light.Light[i];
      GLenum p = GL_LIGHT0 + i;
      *(float *)&(rmesa->hw.lit[i].cmd[LIT_RANGE_CUTOFF]) = FLT_MAX;

      ctx->Driver.Lightfv( ctx, p, GL_AMBIENT, l->Ambient );
      ctx->Driver.Lightfv( ctx, p, GL_DIFFUSE, l->Diffuse );
      ctx->Driver.Lightfv( ctx, p, GL_SPECULAR, l->Specular );
      ctx->Driver.Lightfv( ctx, p, GL_POSITION, 0 );
      ctx->Driver.Lightfv( ctx, p, GL_SPOT_DIRECTION, 0 );
      ctx->Driver.Lightfv( ctx, p, GL_SPOT_EXPONENT, &l->SpotExponent );
      ctx->Driver.Lightfv( ctx, p, GL_SPOT_CUTOFF, &l->SpotCutoff );
      ctx->Driver.Lightfv( ctx, p, GL_CONSTANT_ATTENUATION,
			   &l->ConstantAttenuation );
      ctx->Driver.Lightfv( ctx, p, GL_LINEAR_ATTENUATION, 
			   &l->LinearAttenuation );
      ctx->Driver.Lightfv( ctx, p, GL_QUADRATIC_ATTENUATION, 
		     &l->QuadraticAttenuation );
   }

   ctx->Driver.LightModelfv( ctx, GL_LIGHT_MODEL_AMBIENT, 
			     ctx->Light.Model.Ambient );

   ctx->Driver.Fogfv( ctx, GL_FOG_MODE, 0 );
   ctx->Driver.Fogfv( ctx, GL_FOG_DENSITY, &ctx->Fog.Density );
   ctx->Driver.Fogfv( ctx, GL_FOG_START, &ctx->Fog.Start );
   ctx->Driver.Fogfv( ctx, GL_FOG_END, &ctx->Fog.End );
   ctx->Driver.Fogfv( ctx, GL_FOG_COLOR, ctx->Fog.Color );
   ctx->Driver.Fogfv( ctx, GL_FOG_COORDINATE_SOURCE_EXT, 0 );
}
