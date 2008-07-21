/*
 * Copyright 1998-2003 VIA Technologies, Inc. All Rights Reserved.
 * Copyright 2001-2003 S3 Graphics, Inc. All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sub license,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * VIA, S3 GRAPHICS, AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include <stdio.h>

#include "glheader.h"
#include "context.h"
#include "macros.h"
#include "colormac.h"
#include "enums.h"
#include "dd.h"

#include "mm.h"
#include "via_context.h"
#include "via_state.h"
#include "via_tex.h"
#include "via_tris.h"
#include "via_ioctl.h"
#include "via_3d_reg.h"

#include "swrast/swrast.h"
#include "vbo/vbo.h"
#include "tnl/tnl.h"
#include "swrast_setup/swrast_setup.h"

#include "tnl/t_pipeline.h"


static GLuint ROP[16] = {
    HC_HROP_BLACK,    /* GL_CLEAR           0                      	*/
    HC_HROP_DPa,      /* GL_AND             s & d                  	*/
    HC_HROP_PDna,     /* GL_AND_REVERSE     s & ~d  			*/
    HC_HROP_P,        /* GL_COPY            s                       	*/
    HC_HROP_DPna,     /* GL_AND_INVERTED    ~s & d                      */
    HC_HROP_D,        /* GL_NOOP            d  		                */
    HC_HROP_DPx,      /* GL_XOR             s ^ d                       */
    HC_HROP_DPo,      /* GL_OR              s | d                       */
    HC_HROP_DPon,     /* GL_NOR             ~(s | d)                    */
    HC_HROP_DPxn,     /* GL_EQUIV           ~(s ^ d)                    */
    HC_HROP_Dn,       /* GL_INVERT          ~d                       	*/
    HC_HROP_PDno,     /* GL_OR_REVERSE      s | ~d                      */
    HC_HROP_Pn,       /* GL_COPY_INVERTED   ~s                       	*/
    HC_HROP_DPno,     /* GL_OR_INVERTED     ~s | d                      */
    HC_HROP_DPan,     /* GL_NAND            ~(s & d)                    */
    HC_HROP_WHITE     /* GL_SET             1                       	*/
};

/*
 * Compute the 'S5.5' lod bias factor from the floating point OpenGL bias.
 */
static GLuint viaComputeLodBias(GLfloat bias)
{
   int b = (int) (bias * 32.0);
   if (b > 511)
      b = 511;
   else if (b < -512)
      b = -512;
   return (GLuint) b;
}

void viaEmitState(struct via_context *vmesa)
{
   GLcontext *ctx = vmesa->glCtx;
   GLuint i = 0;
   GLuint j = 0;
   RING_VARS;

   viaCheckDma(vmesa, 0x110);
    
   BEGIN_RING(5);
   OUT_RING( HC_HEADER2 );
   OUT_RING( (HC_ParaType_NotTex << 16) );
   OUT_RING( ((HC_SubA_HEnable << 24) | vmesa->regEnable) );
   OUT_RING( ((HC_SubA_HFBBMSKL << 24) | vmesa->regHFBBMSKL) );    
   OUT_RING( ((HC_SubA_HROP << 24) | vmesa->regHROP) );        
   ADVANCE_RING();
    
   if (vmesa->have_hw_stencil) {
      GLuint pitch, format, offset;
	
      format = HC_HZWBFM_24;	    	
      offset = vmesa->depth.offset;
      pitch = vmesa->depth.pitch;
	
      BEGIN_RING(6);
      OUT_RING( (HC_SubA_HZWBBasL << 24) | (offset & 0xFFFFFF) );
      OUT_RING( (HC_SubA_HZWBBasH << 24) | ((offset & 0xFF000000) >> 24) );	
      OUT_RING( (HC_SubA_HZWBType << 24) | HC_HDBLoc_Local | HC_HZONEasFF_MASK |
	         format | pitch );            
      OUT_RING( (HC_SubA_HZWTMD << 24) | vmesa->regHZWTMD );
      OUT_RING( (HC_SubA_HSTREF << 24) | vmesa->regHSTREF );
      OUT_RING( (HC_SubA_HSTMD << 24) | vmesa->regHSTMD );
      ADVANCE_RING();
   }
   else if (vmesa->hasDepth) {
      GLuint pitch, format, offset;
	
      if (vmesa->depthBits == 16) {
	 format = HC_HZWBFM_16;
      }	    
      else {
	 format = HC_HZWBFM_32;
      }
	    
	    
      offset = vmesa->depth.offset;
      pitch = vmesa->depth.pitch;
	
      BEGIN_RING(4);
      OUT_RING( (HC_SubA_HZWBBasL << 24) | (offset & 0xFFFFFF) );
      OUT_RING( (HC_SubA_HZWBBasH << 24) | ((offset & 0xFF000000) >> 24) );
      OUT_RING( (HC_SubA_HZWBType << 24) | HC_HDBLoc_Local | HC_HZONEasFF_MASK |
	         format | pitch );
      OUT_RING( (HC_SubA_HZWTMD << 24) | vmesa->regHZWTMD );
      ADVANCE_RING();
   }
    
   if (ctx->Color.AlphaEnabled) {
      BEGIN_RING(1);
      OUT_RING( (HC_SubA_HATMD << 24) | vmesa->regHATMD );
      ADVANCE_RING();
      i++;
   }   

   if (ctx->Color.BlendEnabled) {
      BEGIN_RING(11);
      OUT_RING( (HC_SubA_HABLCsat << 24) | vmesa->regHABLCsat );
      OUT_RING( (HC_SubA_HABLCop  << 24) | vmesa->regHABLCop ); 
      OUT_RING( (HC_SubA_HABLAsat << 24) | vmesa->regHABLAsat );        
      OUT_RING( (HC_SubA_HABLAop  << 24) | vmesa->regHABLAop ); 
      OUT_RING( (HC_SubA_HABLRCa  << 24) | vmesa->regHABLRCa ); 
      OUT_RING( (HC_SubA_HABLRFCa << 24) | vmesa->regHABLRFCa );        
      OUT_RING( (HC_SubA_HABLRCbias << 24) | vmesa->regHABLRCbias ); 
      OUT_RING( (HC_SubA_HABLRCb  << 24) | vmesa->regHABLRCb ); 
      OUT_RING( (HC_SubA_HABLRFCb << 24) | vmesa->regHABLRFCb );        
      OUT_RING( (HC_SubA_HABLRAa  << 24) | vmesa->regHABLRAa ); 
      OUT_RING( (HC_SubA_HABLRAb  << 24) | vmesa->regHABLRAb ); 
      ADVANCE_RING();
   }
    
   if (ctx->Fog.Enabled) {
      BEGIN_RING(3);
      OUT_RING( (HC_SubA_HFogLF << 24) | vmesa->regHFogLF ); 
      OUT_RING( (HC_SubA_HFogCL << 24) | vmesa->regHFogCL ); 
      OUT_RING( (HC_SubA_HFogCH << 24) | vmesa->regHFogCH ); 
      ADVANCE_RING();
   }
    
   if (ctx->Line.StippleFlag) {
      BEGIN_RING(2);
      OUT_RING( (HC_SubA_HLP << 24) | ctx->Line.StipplePattern ); 
      OUT_RING( (HC_SubA_HLPRF << 24) | ctx->Line.StippleFactor );
      ADVANCE_RING();
   }

   BEGIN_RING(1);
   OUT_RING( (HC_SubA_HPixGC << 24) | 0x0 ); 
   ADVANCE_RING();
    
   QWORD_PAD_RING();


   if (ctx->Texture._EnabledUnits) {
    
      struct gl_texture_unit *texUnit0 = &ctx->Texture.Unit[0];
      struct gl_texture_unit *texUnit1 = &ctx->Texture.Unit[1];

      {
	 GLuint nDummyValue = 0;

	 BEGIN_RING( 8 );
	 OUT_RING( HC_HEADER2 );
	 OUT_RING( (HC_ParaType_Tex << 16) | (HC_SubType_TexGeneral << 24) );

	 if (texUnit0->Enabled && texUnit1->Enabled) {
	    nDummyValue = (HC_SubA_HTXSMD << 24) | (1 << 3);                
	 }
	 else {
	    nDummyValue = (HC_SubA_HTXSMD << 24) | 0;
	 }

	 if (vmesa->clearTexCache) {
	    vmesa->clearTexCache = 0;
	    OUT_RING( nDummyValue | HC_HTXCHCLR_MASK );
	    OUT_RING( nDummyValue );
	 }
	 else {
	    OUT_RING( nDummyValue );
	    OUT_RING( nDummyValue );
	 }

	 OUT_RING( HC_HEADER2 );
	 OUT_RING( HC_ParaType_NotTex << 16 );
	 OUT_RING( (HC_SubA_HEnable << 24) | vmesa->regEnable );
	 OUT_RING( (HC_SubA_HEnable << 24) | vmesa->regEnable );
	 ADVANCE_RING();
      }

      if (texUnit0->Enabled) {
	 struct gl_texture_object *texObj = texUnit0->_Current;
	 struct via_texture_object *t = (struct via_texture_object *)texObj;
	 GLuint numLevels = t->lastLevel - t->firstLevel + 1;
	 if (VIA_DEBUG & DEBUG_STATE) {
	    fprintf(stderr, "texture0 enabled\n");
	 }		
	 if (numLevels == 8) {
	    BEGIN_RING(27);
	    OUT_RING( HC_HEADER2 );
	    OUT_RING( (HC_ParaType_Tex << 16) |  (0 << 24) );
	    OUT_RING( t->regTexFM );
	    OUT_RING( (HC_SubA_HTXnL0OS << 24) |
	       ((t->lastLevel) << HC_HTXnLVmax_SHIFT) | t->firstLevel );
	    OUT_RING( t->regTexWidthLog2[0] );
	    OUT_RING( t->regTexWidthLog2[1] );
	    OUT_RING( t->regTexHeightLog2[0] );
	    OUT_RING( t->regTexHeightLog2[1] );
	    OUT_RING( t->regTexBaseH[0] );
	    OUT_RING( t->regTexBaseH[1] );
	    OUT_RING( t->regTexBaseH[2] );
	    OUT_RING( t->regTexBaseAndPitch[0].baseL );
	    OUT_RING( t->regTexBaseAndPitch[0].pitchLog2 );
	    OUT_RING( t->regTexBaseAndPitch[1].baseL );
	    OUT_RING( t->regTexBaseAndPitch[1].pitchLog2 );
	    OUT_RING( t->regTexBaseAndPitch[2].baseL );
	    OUT_RING( t->regTexBaseAndPitch[2].pitchLog2 );
	    OUT_RING( t->regTexBaseAndPitch[3].baseL );
	    OUT_RING( t->regTexBaseAndPitch[3].pitchLog2 );
	    OUT_RING( t->regTexBaseAndPitch[4].baseL );
	    OUT_RING( t->regTexBaseAndPitch[4].pitchLog2 );
	    OUT_RING( t->regTexBaseAndPitch[5].baseL );
	    OUT_RING( t->regTexBaseAndPitch[5].pitchLog2 );
	    OUT_RING( t->regTexBaseAndPitch[6].baseL );
	    OUT_RING( t->regTexBaseAndPitch[6].pitchLog2 );
	    OUT_RING( t->regTexBaseAndPitch[7].baseL );
	    OUT_RING( t->regTexBaseAndPitch[7].pitchLog2 );
	    ADVANCE_RING();
	 }
	 else if (numLevels > 1) {

	    BEGIN_RING(12 + numLevels * 2);
	    OUT_RING( HC_HEADER2 );
	    OUT_RING( (HC_ParaType_Tex << 16) |  (0 << 24) );
	    OUT_RING( t->regTexFM );
	    OUT_RING( (HC_SubA_HTXnL0OS << 24) |
	       ((t->lastLevel) << HC_HTXnLVmax_SHIFT) | t->firstLevel );
	    OUT_RING( t->regTexWidthLog2[0] );
	    OUT_RING( t->regTexHeightLog2[0] );
		
	    if (numLevels > 6) {
	       OUT_RING( t->regTexWidthLog2[1] );
	       OUT_RING( t->regTexHeightLog2[1] );
	    }
                
	    OUT_RING( t->regTexBaseH[0] );
		
	    if (numLevels > 3) {
	       OUT_RING( t->regTexBaseH[1] );
	    }
	    if (numLevels > 6) {
	       OUT_RING( t->regTexBaseH[2] );
	    }
	    if (numLevels > 9)  {
	       OUT_RING( t->regTexBaseH[3] );
	    }

	    for (j = 0; j < numLevels; j++) {
	       OUT_RING( t->regTexBaseAndPitch[j].baseL );
	       OUT_RING( t->regTexBaseAndPitch[j].pitchLog2 );
	    }

	    ADVANCE_RING_VARIABLE();
	 }
	 else {

	    BEGIN_RING(9);
	    OUT_RING( HC_HEADER2 );
	    OUT_RING( (HC_ParaType_Tex << 16) |  (0 << 24) );
	    OUT_RING( t->regTexFM );
	    OUT_RING( (HC_SubA_HTXnL0OS << 24) |
	       ((t->lastLevel) << HC_HTXnLVmax_SHIFT) | t->firstLevel );
	    OUT_RING( t->regTexWidthLog2[0] );
	    OUT_RING( t->regTexHeightLog2[0] );
	    OUT_RING( t->regTexBaseH[0] );
	    OUT_RING( t->regTexBaseAndPitch[0].baseL );
	    OUT_RING( t->regTexBaseAndPitch[0].pitchLog2 );
	    ADVANCE_RING();
	 }

	 BEGIN_RING(14);
	 OUT_RING( (HC_SubA_HTXnTB << 24) | vmesa->regHTXnTB[0] );
	 OUT_RING( (HC_SubA_HTXnMPMD << 24) | vmesa->regHTXnMPMD[0] );
	 OUT_RING( (HC_SubA_HTXnTBLCsat << 24) | vmesa->regHTXnTBLCsat[0] );
	 OUT_RING( (HC_SubA_HTXnTBLCop << 24) | vmesa->regHTXnTBLCop[0] );
	 OUT_RING( (HC_SubA_HTXnTBLMPfog << 24) | vmesa->regHTXnTBLMPfog[0] );
	 OUT_RING( (HC_SubA_HTXnTBLAsat << 24) | vmesa->regHTXnTBLAsat[0] );
	 OUT_RING( (HC_SubA_HTXnTBLRCb << 24) | vmesa->regHTXnTBLRCb[0] );
	 OUT_RING( (HC_SubA_HTXnTBLRAa << 24) | vmesa->regHTXnTBLRAa[0] );
	 OUT_RING( (HC_SubA_HTXnTBLRFog << 24) | vmesa->regHTXnTBLRFog[0] );
	 OUT_RING( (HC_SubA_HTXnTBLRCa << 24) | vmesa->regHTXnTBLRCa[0] );
	 OUT_RING( (HC_SubA_HTXnTBLRCc << 24) | vmesa->regHTXnTBLRCc[0] );
	 OUT_RING( (HC_SubA_HTXnTBLRCbias << 24) | vmesa->regHTXnTBLRCbias[0] );
	 OUT_RING( (HC_SubA_HTXnTBC << 24) | vmesa->regHTXnTBC[0] );
	 OUT_RING( (HC_SubA_HTXnTRAH << 24) | vmesa->regHTXnTRAH[0] );
/* 	 OUT_RING( (HC_SubA_HTXnCLODu << 24) | vmesa->regHTXnCLOD[0] ); */
	 ADVANCE_RING();

	 /* KW:  This test never succeeds:
	  */
	 if (t->regTexFM == HC_HTXnFM_Index8) {
	    const struct gl_color_table *table = &texObj->Palette;
	    const GLfloat *tableF = table->TableF;

	    BEGIN_RING(2 + table->Size);
	    OUT_RING( HC_HEADER2 );
	    OUT_RING( (HC_ParaType_Palette << 16) | (0 << 24) );
	    for (j = 0; j < table->Size; j++) 
	       OUT_RING( tableF[j] );
	    ADVANCE_RING();
	       
	 }

	 QWORD_PAD_RING();
      }
	
      if (texUnit1->Enabled) {
	 struct gl_texture_object *texObj = texUnit1->_Current;
	 struct via_texture_object *t = (struct via_texture_object *)texObj;
	 GLuint numLevels = t->lastLevel - t->firstLevel + 1;
	 int texunit = (texUnit0->Enabled ? 1 : 0);
	 if (VIA_DEBUG & DEBUG_STATE) {
	    fprintf(stderr, "texture1 enabled\n");
	 }		
	 if (numLevels == 8) {
	    BEGIN_RING(27);
	    OUT_RING( HC_HEADER2 );
	    OUT_RING( (HC_ParaType_Tex << 16) |  (texunit << 24) );
	    OUT_RING( t->regTexFM );
	    OUT_RING( (HC_SubA_HTXnL0OS << 24) |
	       ((t->lastLevel) << HC_HTXnLVmax_SHIFT) | t->firstLevel );
	    OUT_RING( t->regTexWidthLog2[0] );
	    OUT_RING( t->regTexWidthLog2[1] );
	    OUT_RING( t->regTexHeightLog2[0] );
	    OUT_RING( t->regTexHeightLog2[1] );
	    OUT_RING( t->regTexBaseH[0] );
	    OUT_RING( t->regTexBaseH[1] );
	    OUT_RING( t->regTexBaseH[2] );
	    OUT_RING( t->regTexBaseAndPitch[0].baseL );
	    OUT_RING( t->regTexBaseAndPitch[0].pitchLog2 );
	    OUT_RING( t->regTexBaseAndPitch[1].baseL );
	    OUT_RING( t->regTexBaseAndPitch[1].pitchLog2 );
	    OUT_RING( t->regTexBaseAndPitch[2].baseL );
	    OUT_RING( t->regTexBaseAndPitch[2].pitchLog2 );
	    OUT_RING( t->regTexBaseAndPitch[3].baseL );
	    OUT_RING( t->regTexBaseAndPitch[3].pitchLog2 );
	    OUT_RING( t->regTexBaseAndPitch[4].baseL );
	    OUT_RING( t->regTexBaseAndPitch[4].pitchLog2 );
	    OUT_RING( t->regTexBaseAndPitch[5].baseL );
	    OUT_RING( t->regTexBaseAndPitch[5].pitchLog2 );
	    OUT_RING( t->regTexBaseAndPitch[6].baseL );
	    OUT_RING( t->regTexBaseAndPitch[6].pitchLog2 );
	    OUT_RING( t->regTexBaseAndPitch[7].baseL );
	    OUT_RING( t->regTexBaseAndPitch[7].pitchLog2 );
	    ADVANCE_RING();
	 }
	 else if (numLevels > 1) {
	    BEGIN_RING(12 + numLevels * 2);
	    OUT_RING( HC_HEADER2 );
	    OUT_RING( (HC_ParaType_Tex << 16) |  (texunit << 24) );
	    OUT_RING( t->regTexFM );
	    OUT_RING( (HC_SubA_HTXnL0OS << 24) |
	       ((t->lastLevel) << HC_HTXnLVmax_SHIFT) | t->firstLevel );
	    OUT_RING( t->regTexWidthLog2[0] );
	    OUT_RING( t->regTexHeightLog2[0] );
		
	    if (numLevels > 6) {
	       OUT_RING( t->regTexWidthLog2[1] );
	       OUT_RING( t->regTexHeightLog2[1] );
	       i += 2;
	    }
                
	    OUT_RING( t->regTexBaseH[0] );
		
	    if (numLevels > 3) { 
	       OUT_RING( t->regTexBaseH[1] );
	    }
	    if (numLevels > 6) {
	       OUT_RING( t->regTexBaseH[2] );
	    }
	    if (numLevels > 9)  {
	       OUT_RING( t->regTexBaseH[3] );
	    }
		
	    for (j = 0; j < numLevels; j++) {
	       OUT_RING( t->regTexBaseAndPitch[j].baseL );
	       OUT_RING( t->regTexBaseAndPitch[j].pitchLog2 );
	    }
	    ADVANCE_RING_VARIABLE();
	 }
	 else {
	    BEGIN_RING(9);
	    OUT_RING( HC_HEADER2 );
	    OUT_RING( (HC_ParaType_Tex << 16) |  (texunit << 24) );
	    OUT_RING( t->regTexFM );
	    OUT_RING( (HC_SubA_HTXnL0OS << 24) |
	       ((t->lastLevel) << HC_HTXnLVmax_SHIFT) | t->firstLevel );
	    OUT_RING( t->regTexWidthLog2[0] );
	    OUT_RING( t->regTexHeightLog2[0] );
	    OUT_RING( t->regTexBaseH[0] );
	    OUT_RING( t->regTexBaseAndPitch[0].baseL );
	    OUT_RING( t->regTexBaseAndPitch[0].pitchLog2 );
	    ADVANCE_RING();
	 }

	 BEGIN_RING(14);
	 OUT_RING( (HC_SubA_HTXnTB << 24) | vmesa->regHTXnTB[1] );
	 OUT_RING( (HC_SubA_HTXnMPMD << 24) | vmesa->regHTXnMPMD[1] );
	 OUT_RING( (HC_SubA_HTXnTBLCsat << 24) | vmesa->regHTXnTBLCsat[1] );
	 OUT_RING( (HC_SubA_HTXnTBLCop << 24) | vmesa->regHTXnTBLCop[1] );
	 OUT_RING( (HC_SubA_HTXnTBLMPfog << 24) | vmesa->regHTXnTBLMPfog[1] );
	 OUT_RING( (HC_SubA_HTXnTBLAsat << 24) | vmesa->regHTXnTBLAsat[1] );
	 OUT_RING( (HC_SubA_HTXnTBLRCb << 24) | vmesa->regHTXnTBLRCb[1] );
	 OUT_RING( (HC_SubA_HTXnTBLRAa << 24) | vmesa->regHTXnTBLRAa[1] );
	 OUT_RING( (HC_SubA_HTXnTBLRFog << 24) | vmesa->regHTXnTBLRFog[1] );
	 OUT_RING( (HC_SubA_HTXnTBLRCa << 24) | vmesa->regHTXnTBLRCa[1] );
	 OUT_RING( (HC_SubA_HTXnTBLRCc << 24) | vmesa->regHTXnTBLRCc[1] );
	 OUT_RING( (HC_SubA_HTXnTBLRCbias << 24) | vmesa->regHTXnTBLRCbias[1] );
	 OUT_RING( (HC_SubA_HTXnTBC << 24) | vmesa->regHTXnTBC[1] );
	 OUT_RING( (HC_SubA_HTXnTRAH << 24) | vmesa->regHTXnTRAH[1] );
/* 	 OUT_RING( (HC_SubA_HTXnCLODu << 24) | vmesa->regHTXnCLOD[1] ); */
	 ADVANCE_RING();

	 /* KW:  This test never succeeds:
	  */
	 if (t->regTexFM == HC_HTXnFM_Index8) {
	    const struct gl_color_table *table = &texObj->Palette;
	    const GLfloat *tableF = table->TableF;

	    BEGIN_RING(2 + table->Size);
	    OUT_RING( HC_HEADER2 );
	    OUT_RING( (HC_ParaType_Palette << 16) | (texunit << 24) );
	    for (j = 0; j < table->Size; j++) {
	       OUT_RING( tableF[j] );
	    }
	    ADVANCE_RING();
	 }

	 QWORD_PAD_RING();
      }
   }
    
#if 0
   /* Polygon stipple is broken - for certain stipple values,
    * eg. 0xf0f0f0f0, the hardware will refuse to accept the stipple.
    * Coincidentally, conform generates just such a stipple.
    */
   if (ctx->Polygon.StippleFlag) {
      GLuint *stipple = &ctx->PolygonStipple[0];
      __DRIdrawablePrivate *dPriv = vmesa->driDrawable;
      struct via_renderbuffer *const vrb = 
	(struct via_renderbuffer *) dPriv->driverPrivate;
      GLint i;
        
      BEGIN_RING(38);
      OUT_RING( HC_HEADER2 );             

      OUT_RING( ((HC_ParaType_Palette << 16) | (HC_SubType_Stipple << 24)) );
      for (i = 31; i >= 0; i--) {
	 GLint j;
	 GLuint k = 0;

	 /* Need to flip bits left to right:
	  */
	 for (j = 0 ; j < 32; j++)
	    if (stipple[i] & (1<<j))
	       k |= 1 << (31-j);

	 OUT_RING( k );     
      }

      OUT_RING( HC_HEADER2 );                     
      OUT_RING( (HC_ParaType_NotTex << 16) );
      OUT_RING( (HC_SubA_HSPXYOS << 24) );
      OUT_RING( (HC_SubA_HSPXYOS << 24) );

      ADVANCE_RING();
   }
#endif
   
   vmesa->newEmitState = 0;
}


static __inline__ GLuint viaPackColor(GLuint bpp,
                                      GLubyte r, GLubyte g,
                                      GLubyte b, GLubyte a)
{
    switch (bpp) {
    case 16:
        return PACK_COLOR_565(r, g, b);
    case 32:
        return PACK_COLOR_8888(a, r, g, b);        
    default:
       assert(0);
       return 0;
   }
}

static void viaBlendEquationSeparate(GLcontext *ctx,
				     GLenum rgbMode, 
				     GLenum aMode)
{
    if (VIA_DEBUG & DEBUG_STATE) 
       fprintf(stderr, "%s in\n", __FUNCTION__);

    /* GL_EXT_blend_equation_separate not supported */
    ASSERT(rgbMode == aMode);

    /* Can only do GL_ADD equation in hardware */
    FALLBACK(VIA_CONTEXT(ctx), VIA_FALLBACK_BLEND_EQ, 
	     rgbMode != GL_FUNC_ADD_EXT);

    /* BlendEquation sets ColorLogicOpEnabled in an unexpected
     * manner.
     */
    FALLBACK(VIA_CONTEXT(ctx), VIA_FALLBACK_LOGICOP,
             (ctx->Color.ColorLogicOpEnabled &&
              ctx->Color.LogicOp != GL_COPY));
}

static void viaBlendFunc(GLcontext *ctx, GLenum sfactor, GLenum dfactor)
{
    struct via_context *vmesa = VIA_CONTEXT(ctx);
    GLboolean fallback = GL_FALSE;
    if (VIA_DEBUG & DEBUG_STATE) 
       fprintf(stderr, "%s in\n", __FUNCTION__);

    switch (ctx->Color.BlendSrcRGB) {
    case GL_SRC_ALPHA_SATURATE:  
    case GL_CONSTANT_COLOR:
    case GL_ONE_MINUS_CONSTANT_COLOR:
    case GL_CONSTANT_ALPHA:
    case GL_ONE_MINUS_CONSTANT_ALPHA:
        fallback = GL_TRUE;
        break;
    default:
        break;
    }

    switch (ctx->Color.BlendDstRGB) {
    case GL_CONSTANT_COLOR:
    case GL_ONE_MINUS_CONSTANT_COLOR:
    case GL_CONSTANT_ALPHA:
    case GL_ONE_MINUS_CONSTANT_ALPHA:
        fallback = GL_TRUE;
        break;
    default:
        break;
    }

    FALLBACK(vmesa, VIA_FALLBACK_BLEND_FUNC, fallback);
}

/* Shouldn't be called as the extension is disabled.
 */
static void viaBlendFuncSeparate(GLcontext *ctx, GLenum sfactorRGB,
                                 GLenum dfactorRGB, GLenum sfactorA,
                                 GLenum dfactorA)
{
    if (dfactorRGB != dfactorA || sfactorRGB != sfactorA) {
        _mesa_error(ctx, GL_INVALID_OPERATION, "glBlendEquation (disabled)");
    }

    viaBlendFunc(ctx, sfactorRGB, dfactorRGB);
}




/* =============================================================
 * Hardware clipping
 */
static void viaScissor(GLcontext *ctx, GLint x, GLint y,
                       GLsizei w, GLsizei h)
{
    struct via_context *vmesa = VIA_CONTEXT(ctx);

    if (!vmesa->driDrawable)
       return;

    if (VIA_DEBUG & DEBUG_STATE)
       fprintf(stderr, "%s %d,%d %dx%d, drawH %d\n", __FUNCTION__, 
	       x,y,w,h, vmesa->driDrawable->h);

    if (vmesa->scissor) {
        VIA_FLUSH_DMA(vmesa); /* don't pipeline cliprect changes */
    }

    vmesa->scissorRect.x1 = x;
    vmesa->scissorRect.y1 = vmesa->driDrawable->h - y - h;
    vmesa->scissorRect.x2 = x + w;
    vmesa->scissorRect.y2 = vmesa->driDrawable->h - y;
}

static void viaEnable(GLcontext *ctx, GLenum cap, GLboolean state)
{
   struct via_context *vmesa = VIA_CONTEXT(ctx);

   switch (cap) {
   case GL_SCISSOR_TEST:
      VIA_FLUSH_DMA(vmesa);
      vmesa->scissor = state;
      break;
   default:
      break;
   }
}



/* Fallback to swrast for select and feedback.
 */
static void viaRenderMode(GLcontext *ctx, GLenum mode)
{
    FALLBACK(VIA_CONTEXT(ctx), VIA_FALLBACK_RENDERMODE, (mode != GL_RENDER));
}


static void viaDrawBuffer(GLcontext *ctx, GLenum mode)
{
   struct via_context *vmesa = VIA_CONTEXT(ctx);

   if (VIA_DEBUG & (DEBUG_DRI|DEBUG_STATE)) 
      fprintf(stderr, "%s in\n", __FUNCTION__);

   if (!ctx->DrawBuffer)
      return;

   switch ( ctx->DrawBuffer->_ColorDrawBufferMask[0] ) {
   case BUFFER_BIT_FRONT_LEFT:
      VIA_FLUSH_DMA(vmesa);
      vmesa->drawBuffer = &vmesa->front;
      FALLBACK(vmesa, VIA_FALLBACK_DRAW_BUFFER, GL_FALSE);
      break;
   case BUFFER_BIT_BACK_LEFT:
      VIA_FLUSH_DMA(vmesa);
      vmesa->drawBuffer = &vmesa->back;
      FALLBACK(vmesa, VIA_FALLBACK_DRAW_BUFFER, GL_FALSE);
      break;
   default:
      FALLBACK(vmesa, VIA_FALLBACK_DRAW_BUFFER, GL_TRUE);
      return;
   }


   viaXMesaWindowMoved(vmesa);
}

static void viaClearColor(GLcontext *ctx, const GLfloat color[4])
{
    struct via_context *vmesa = VIA_CONTEXT(ctx);
    GLubyte pcolor[4];
    CLAMPED_FLOAT_TO_UBYTE(pcolor[0], color[0]);
    CLAMPED_FLOAT_TO_UBYTE(pcolor[1], color[1]);
    CLAMPED_FLOAT_TO_UBYTE(pcolor[2], color[2]);
    CLAMPED_FLOAT_TO_UBYTE(pcolor[3], color[3]);
    vmesa->ClearColor = viaPackColor(vmesa->viaScreen->bitsPerPixel,
                                     pcolor[0], pcolor[1],
                                     pcolor[2], pcolor[3]);
}

#define WRITEMASK_ALPHA_SHIFT 31
#define WRITEMASK_RED_SHIFT   30
#define WRITEMASK_GREEN_SHIFT 29
#define WRITEMASK_BLUE_SHIFT  28

static void viaColorMask(GLcontext *ctx,
			 GLboolean r, GLboolean g,
			 GLboolean b, GLboolean a)
{
   struct via_context *vmesa = VIA_CONTEXT( ctx );

   if (VIA_DEBUG & DEBUG_STATE)
      fprintf(stderr, "%s r(%d) g(%d) b(%d) a(%d)\n", __FUNCTION__, r, g, b, a);

   vmesa->ClearMask = (((!r) << WRITEMASK_RED_SHIFT) |
		       ((!g) << WRITEMASK_GREEN_SHIFT) |
		       ((!b) << WRITEMASK_BLUE_SHIFT) |
		       ((!a) << WRITEMASK_ALPHA_SHIFT));
}



/* This hardware just isn't capable of private back buffers without
 * glitches and/or a hefty locking scheme.
 */
void viaCalcViewport(GLcontext *ctx)
{
    struct via_context *vmesa = VIA_CONTEXT(ctx);
    __DRIdrawablePrivate *dPriv = vmesa->driDrawable;
    struct via_renderbuffer *const vrb = 
      (struct via_renderbuffer *) dPriv->driverPrivate;
    const GLfloat *v = ctx->Viewport._WindowMap.m;
    GLfloat *m = vmesa->ViewportMatrix.m;
    
    m[MAT_SX] =   v[MAT_SX];
    m[MAT_TX] =   v[MAT_TX] + vrb->drawX + SUBPIXEL_X;
    m[MAT_SY] = - v[MAT_SY];
    m[MAT_TY] = - v[MAT_TY] + vrb->drawY + SUBPIXEL_Y + vrb->drawH;
    m[MAT_SZ] =   v[MAT_SZ] * (1.0 / vmesa->depth_max);
    m[MAT_TZ] =   v[MAT_TZ] * (1.0 / vmesa->depth_max);
}

static void viaViewport(GLcontext *ctx,
                        GLint x, GLint y,
                        GLsizei width, GLsizei height)
{
    viaCalcViewport(ctx);
}

static void viaDepthRange(GLcontext *ctx,
                          GLclampd nearval, GLclampd farval)
{
    viaCalcViewport(ctx);
}

void viaInitState(GLcontext *ctx)
{
    struct via_context *vmesa = VIA_CONTEXT(ctx);

    vmesa->regCmdB = HC_ACMD_HCmdB;
    vmesa->regEnable = HC_HenCW_MASK;

   /* Mesa should do this for us:
    */

   ctx->Driver.BlendEquationSeparate( ctx, 
				      ctx->Color.BlendEquationRGB,
				      ctx->Color.BlendEquationA);

   ctx->Driver.BlendFuncSeparate( ctx,
				  ctx->Color.BlendSrcRGB,
				  ctx->Color.BlendDstRGB,
				  ctx->Color.BlendSrcA,
				  ctx->Color.BlendDstA);

   ctx->Driver.Scissor( ctx, ctx->Scissor.X, ctx->Scissor.Y,
			ctx->Scissor.Width, ctx->Scissor.Height );

   ctx->Driver.DrawBuffer( ctx, ctx->Color.DrawBuffer[0] );
}

/**
 * Convert S and T texture coordinate wrap modes to hardware bits.
 */
static u_int32_t
get_wrap_mode( GLenum sWrap, GLenum tWrap )
{
    u_int32_t v = 0;


    switch( sWrap ) {
    case GL_REPEAT:
	v |= HC_HTXnMPMD_Srepeat;
	break;
    case GL_CLAMP:
    case GL_CLAMP_TO_EDGE:
	v |= HC_HTXnMPMD_Sclamp;
	break;
    case GL_MIRRORED_REPEAT:
	v |= HC_HTXnMPMD_Smirror;
	break;
    }

    switch( tWrap ) {
    case GL_REPEAT:
	v |= HC_HTXnMPMD_Trepeat;
	break;
    case GL_CLAMP:
    case GL_CLAMP_TO_EDGE:
	v |= HC_HTXnMPMD_Tclamp;
	break;
    case GL_MIRRORED_REPEAT:
	v |= HC_HTXnMPMD_Tmirror;
	break;
    }
    
    return v;
}

static u_int32_t
get_minmag_filter( GLenum min, GLenum mag )
{
    u_int32_t v = 0;

    switch (min) {
    case GL_NEAREST:
        v = HC_HTXnFLSs_Nearest |
            HC_HTXnFLTs_Nearest;
        break;
    case GL_LINEAR:
        v = HC_HTXnFLSs_Linear |
            HC_HTXnFLTs_Linear;
        break;
    case GL_NEAREST_MIPMAP_NEAREST:
        v = HC_HTXnFLSs_Nearest |
            HC_HTXnFLTs_Nearest;
        v |= HC_HTXnFLDs_Nearest;
        break;
    case GL_LINEAR_MIPMAP_NEAREST:
        v = HC_HTXnFLSs_Linear |
            HC_HTXnFLTs_Linear;
        v |= HC_HTXnFLDs_Nearest;
        break;
    case GL_NEAREST_MIPMAP_LINEAR:
        v = HC_HTXnFLSs_Nearest |
            HC_HTXnFLTs_Nearest;
        v |= HC_HTXnFLDs_Linear;
        break;
    case GL_LINEAR_MIPMAP_LINEAR:
        v = HC_HTXnFLSs_Linear |
            HC_HTXnFLTs_Linear;
        v |= HC_HTXnFLDs_Linear;
        break;
    default:
        break;
    }

    switch (mag) {
    case GL_LINEAR:
        v |= HC_HTXnFLSe_Linear |
             HC_HTXnFLTe_Linear;
	break;
    case GL_NEAREST:
        v |= HC_HTXnFLSe_Nearest |
             HC_HTXnFLTe_Nearest;
	break;
    default:
        break;
    }

    return v;
}


static GLboolean viaChooseTextureState(GLcontext *ctx) 
{
    struct via_context *vmesa = VIA_CONTEXT(ctx);
    struct gl_texture_unit *texUnit0 = &ctx->Texture.Unit[0];
    struct gl_texture_unit *texUnit1 = &ctx->Texture.Unit[1];

    if (texUnit0->_ReallyEnabled || texUnit1->_ReallyEnabled) {
        vmesa->regEnable |= HC_HenTXMP_MASK | HC_HenTXCH_MASK | HC_HenTXPP_MASK;

        if (texUnit0->_ReallyEnabled) {
            struct gl_texture_object *texObj = texUnit0->_Current;
   
	    vmesa->regHTXnTB[0] = get_minmag_filter( texObj->MinFilter,
						    texObj->MagFilter );

	    vmesa->regHTXnMPMD[0] &= ~(HC_HTXnMPMD_SMASK | HC_HTXnMPMD_TMASK);
	    vmesa->regHTXnMPMD[0] |= get_wrap_mode( texObj->WrapS,
						   texObj->WrapT );

	    vmesa->regHTXnTB[0] &= ~(HC_HTXnTB_TBC_S | HC_HTXnTB_TBC_T);
            if (texObj->Image[0][texObj->BaseLevel]->Border > 0) {
	       vmesa->regHTXnTB[0] |= (HC_HTXnTB_TBC_S | HC_HTXnTB_TBC_T);
	       vmesa->regHTXnTBC[0] = 
		  PACK_COLOR_888(FLOAT_TO_UBYTE(texObj->BorderColor[0]),
				 FLOAT_TO_UBYTE(texObj->BorderColor[1]),
				 FLOAT_TO_UBYTE(texObj->BorderColor[2]));
	       vmesa->regHTXnTRAH[0] = FLOAT_TO_UBYTE(texObj->BorderColor[3]);
            }

	    if (texUnit0->LodBias != 0.0f) {
	       GLuint b = viaComputeLodBias(texUnit0->LodBias);
	       vmesa->regHTXnTB[0] &= ~HC_HTXnFLDs_MASK;
	       vmesa->regHTXnTB[0] |= HC_HTXnFLDs_ConstLOD;
	       vmesa->regHTXnCLOD[0] = (b&0x1f) | (((~b)&0x1f)<<10); /* FIXME */
	    }

	    if (!viaTexCombineState( vmesa, texUnit0->_CurrentCombine, 0 )) {
	       if (VIA_DEBUG & DEBUG_TEXTURE)
		  fprintf(stderr, "viaTexCombineState failed for unit 0\n");
	       return GL_FALSE;
	    }
        }

        if (texUnit1->_ReallyEnabled) {
            struct gl_texture_object *texObj = texUnit1->_Current;

	    vmesa->regHTXnTB[1] = get_minmag_filter( texObj->MinFilter,
						    texObj->MagFilter );
	    vmesa->regHTXnMPMD[1] &= ~(HC_HTXnMPMD_SMASK | HC_HTXnMPMD_TMASK);
	    vmesa->regHTXnMPMD[1] |= get_wrap_mode( texObj->WrapS,
						   texObj->WrapT );

	    vmesa->regHTXnTB[1] &= ~(HC_HTXnTB_TBC_S | HC_HTXnTB_TBC_T);
            if (texObj->Image[0][texObj->BaseLevel]->Border > 0) {
	       vmesa->regHTXnTB[1] |= (HC_HTXnTB_TBC_S | HC_HTXnTB_TBC_T);
	       vmesa->regHTXnTBC[1] = 
		  PACK_COLOR_888(FLOAT_TO_UBYTE(texObj->BorderColor[0]),
				 FLOAT_TO_UBYTE(texObj->BorderColor[1]),
				 FLOAT_TO_UBYTE(texObj->BorderColor[2]));
	       vmesa->regHTXnTRAH[1] = FLOAT_TO_UBYTE(texObj->BorderColor[3]);
            }


	    if (texUnit1->LodBias != 0.0f) {
	       GLuint b = viaComputeLodBias(texUnit1->LodBias);
	       vmesa->regHTXnTB[1] &= ~HC_HTXnFLDs_MASK;
	       vmesa->regHTXnTB[1] |= HC_HTXnFLDs_ConstLOD;
	       vmesa->regHTXnCLOD[1] = (b&0x1f) | (((~b)&0x1f)<<10); /* FIXME */
	    }

	    if (!viaTexCombineState( vmesa, texUnit1->_CurrentCombine, 1 )) {
	       if (VIA_DEBUG & DEBUG_TEXTURE)
		  fprintf(stderr, "viaTexCombineState failed for unit 1\n");
	       return GL_FALSE;
	    }
        }
    }
    else {
        vmesa->regEnable &= ~(HC_HenTXMP_MASK | HC_HenTXCH_MASK | 
			      HC_HenTXPP_MASK);
    }
    
    return GL_TRUE;
}

static void viaChooseColorState(GLcontext *ctx) 
{
    struct via_context *vmesa = VIA_CONTEXT(ctx);
    GLenum s = ctx->Color.BlendSrcRGB;
    GLenum d = ctx->Color.BlendDstRGB;

    /* The HW's blending equation is:
     * (Ca * FCa + Cbias + Cb * FCb) << Cshift
     */

    if (ctx->Color.BlendEnabled) {
        vmesa->regEnable |= HC_HenABL_MASK;
        /* Ca  -- always from source color.
         */
        vmesa->regHABLCsat = HC_HABLCsat_MASK | HC_HABLCa_OPC | HC_HABLCa_Csrc;
        /* Aa  -- always from source alpha.
         */
        vmesa->regHABLAsat = HC_HABLAsat_MASK | HC_HABLAa_OPA | HC_HABLAa_Asrc;
        /* FCa -- depend on following condition.
         * FAa -- depend on following condition.
         */
        switch (s) {
        case GL_ZERO:
            /* (0, 0, 0, 0)
             */
            vmesa->regHABLCsat |= HC_HABLFCa_OPC | HC_HABLFCa_HABLRCa;
            vmesa->regHABLAsat |= HC_HABLFAa_OPA | HC_HABLFAa_HABLFRA;
            vmesa->regHABLRFCa = 0x0;
            vmesa->regHABLRAa = 0x0;
            break;
        case GL_ONE:
            /* (1, 1, 1, 1)
             */
            vmesa->regHABLCsat |= HC_HABLFCa_InvOPC | HC_HABLFCa_HABLRCa;
            vmesa->regHABLAsat |= HC_HABLFAa_InvOPA | HC_HABLFAa_HABLFRA;
            vmesa->regHABLRFCa = 0x0;
            vmesa->regHABLRAa = 0x0;
            break;
        case GL_SRC_COLOR:
            /* (Rs, Gs, Bs, As)
             */
            vmesa->regHABLCsat |= HC_HABLFCa_OPC | HC_HABLFCa_Csrc;
            vmesa->regHABLAsat |= HC_HABLFAa_OPA | HC_HABLFAa_Asrc;
            break;
        case GL_ONE_MINUS_SRC_COLOR:
            /* (1, 1, 1, 1) - (Rs, Gs, Bs, As)
             */
            vmesa->regHABLCsat |= HC_HABLFCa_InvOPC | HC_HABLFCa_Csrc;
            vmesa->regHABLAsat |= HC_HABLFAa_InvOPA | HC_HABLFAa_Asrc;
            break;
        case GL_DST_COLOR:
            /* (Rd, Gd, Bd, Ad)
             */
            vmesa->regHABLCsat |= HC_HABLFCa_OPC | HC_HABLFCa_Cdst;
            vmesa->regHABLAsat |= HC_HABLFAa_OPA | HC_HABLFAa_Adst;
            break;
        case GL_ONE_MINUS_DST_COLOR:
            /* (1, 1, 1, 1) - (Rd, Gd, Bd, Ad)
             */
            vmesa->regHABLCsat |= HC_HABLFCa_InvOPC | HC_HABLFCa_Cdst;
            vmesa->regHABLAsat |= HC_HABLFAa_InvOPA | HC_HABLFAa_Adst;
            break;
        case GL_SRC_ALPHA:
            /* (As, As, As, As)
             */
            vmesa->regHABLCsat |= HC_HABLFCa_OPC | HC_HABLFCa_Asrc;
            vmesa->regHABLAsat |= HC_HABLFAa_OPA | HC_HABLFAa_Asrc;
            break;
        case GL_ONE_MINUS_SRC_ALPHA:
            /* (1, 1, 1, 1) - (As, As, As, As)
             */
            vmesa->regHABLCsat |= HC_HABLFCa_InvOPC | HC_HABLFCa_Asrc;
            vmesa->regHABLAsat |= HC_HABLFAa_InvOPA | HC_HABLFAa_Asrc;
            break;
        case GL_DST_ALPHA:
            {
                if (vmesa->viaScreen->bitsPerPixel == 16) {
                    /* (1, 1, 1, 1)
                     */
                    vmesa->regHABLCsat |= (HC_HABLFCa_InvOPC | 
					   HC_HABLFCa_HABLRCa);
                    vmesa->regHABLAsat |= (HC_HABLFAa_InvOPA | 
					   HC_HABLFAa_HABLFRA);
                    vmesa->regHABLRFCa = 0x0;
                    vmesa->regHABLRAa = 0x0;
                }
                else {
                    /* (Ad, Ad, Ad, Ad)
                     */
                    vmesa->regHABLCsat |= HC_HABLFCa_OPC | HC_HABLFCa_Adst;
                    vmesa->regHABLAsat |= HC_HABLFAa_OPA | HC_HABLFAa_Adst;
                }
            }
            break;
        case GL_ONE_MINUS_DST_ALPHA:
            {
                if (vmesa->viaScreen->bitsPerPixel == 16) {
                    /* (1, 1, 1, 1) - (1, 1, 1, 1) = (0, 0, 0, 0)
                     */
                    vmesa->regHABLCsat |= HC_HABLFCa_OPC | HC_HABLFCa_HABLRCa;
                    vmesa->regHABLAsat |= HC_HABLFAa_OPA | HC_HABLFAa_HABLFRA;
                    vmesa->regHABLRFCa = 0x0;
                    vmesa->regHABLRAa = 0x0;
                }
                else {
                    /* (1, 1, 1, 1) - (Ad, Ad, Ad, Ad)
                     */
                    vmesa->regHABLCsat |= HC_HABLFCa_InvOPC | HC_HABLFCa_Adst;
                    vmesa->regHABLAsat |= HC_HABLFAa_InvOPA | HC_HABLFAa_Adst;
                }
            }
            break;
        case GL_SRC_ALPHA_SATURATE:
            {
                if (vmesa->viaScreen->bitsPerPixel == 16) {
                    /* (f, f, f, 1), f = min(As, 1 - Ad) = min(As, 1 - 1) = 0
                     * So (f, f, f, 1) = (0, 0, 0, 1)
                     */
                    vmesa->regHABLCsat |= HC_HABLFCa_OPC | HC_HABLFCa_HABLRCa;
                    vmesa->regHABLAsat |= (HC_HABLFAa_InvOPA | 
					   HC_HABLFAa_HABLFRA);
                    vmesa->regHABLRFCa = 0x0;
                    vmesa->regHABLRAa = 0x0;
                }
                else {
                    /* (f, f, f, 1), f = min(As, 1 - Ad)
                     */
                    vmesa->regHABLCsat |= (HC_HABLFCa_OPC | 
					   HC_HABLFCa_mimAsrcInvAdst);
                    vmesa->regHABLAsat |= (HC_HABLFAa_InvOPA | 
					   HC_HABLFAa_HABLFRA);
                    vmesa->regHABLRFCa = 0x0;
                    vmesa->regHABLRAa = 0x0;
                }
            }
            break;
        }

        /* Op is add.
         */

        /* bias is 0.
         */
        vmesa->regHABLCsat |= HC_HABLCbias_HABLRCbias;
        vmesa->regHABLAsat |= HC_HABLAbias_HABLRAbias;

        /* Cb  -- always from destination color.
         */
        vmesa->regHABLCop = HC_HABLCb_OPC | HC_HABLCb_Cdst;
        /* Ab  -- always from destination alpha.
         */
        vmesa->regHABLAop = HC_HABLAb_OPA | HC_HABLAb_Adst;
        /* FCb -- depend on following condition.
         */
        switch (d) {
        case GL_ZERO:
            /* (0, 0, 0, 0)
             */
            vmesa->regHABLCop |= HC_HABLFCb_OPC | HC_HABLFCb_HABLRCb;
            vmesa->regHABLAop |= HC_HABLFAb_OPA | HC_HABLFAb_HABLFRA;
            vmesa->regHABLRFCb = 0x0;
            vmesa->regHABLRAb = 0x0;
            break;
        case GL_ONE:
            /* (1, 1, 1, 1)
             */
            vmesa->regHABLCop |= HC_HABLFCb_InvOPC | HC_HABLFCb_HABLRCb;
            vmesa->regHABLAop |= HC_HABLFAb_InvOPA | HC_HABLFAb_HABLFRA;
            vmesa->regHABLRFCb = 0x0;
            vmesa->regHABLRAb = 0x0;
            break;
        case GL_SRC_COLOR:
            /* (Rs, Gs, Bs, As)
             */
            vmesa->regHABLCop |= HC_HABLFCb_OPC | HC_HABLFCb_Csrc;
            vmesa->regHABLAop |= HC_HABLFAb_OPA | HC_HABLFAb_Asrc;
            break;
        case GL_ONE_MINUS_SRC_COLOR:
            /* (1, 1, 1, 1) - (Rs, Gs, Bs, As)
             */
            vmesa->regHABLCop |= HC_HABLFCb_InvOPC | HC_HABLFCb_Csrc;
            vmesa->regHABLAop |= HC_HABLFAb_InvOPA | HC_HABLFAb_Asrc;
            break;
        case GL_DST_COLOR:
            /* (Rd, Gd, Bd, Ad)
             */
            vmesa->regHABLCop |= HC_HABLFCb_OPC | HC_HABLFCb_Cdst;
            vmesa->regHABLAop |= HC_HABLFAb_OPA | HC_HABLFAb_Adst;
            break;
        case GL_ONE_MINUS_DST_COLOR:
            /* (1, 1, 1, 1) - (Rd, Gd, Bd, Ad)
             */
            vmesa->regHABLCop |= HC_HABLFCb_InvOPC | HC_HABLFCb_Cdst;
            vmesa->regHABLAop |= HC_HABLFAb_InvOPA | HC_HABLFAb_Adst;
            break;
        case GL_SRC_ALPHA:
            /* (As, As, As, As)
             */
            vmesa->regHABLCop |= HC_HABLFCb_OPC | HC_HABLFCb_Asrc;
            vmesa->regHABLAop |= HC_HABLFAb_OPA | HC_HABLFAb_Asrc;
            break;
        case GL_ONE_MINUS_SRC_ALPHA:
            /* (1, 1, 1, 1) - (As, As, As, As)
             */
            vmesa->regHABLCop |= HC_HABLFCb_InvOPC | HC_HABLFCb_Asrc;
            vmesa->regHABLAop |= HC_HABLFAb_InvOPA | HC_HABLFAb_Asrc;
            break;
        case GL_DST_ALPHA:
            {
                if (vmesa->viaScreen->bitsPerPixel == 16) {
                    /* (1, 1, 1, 1)
                     */
                    vmesa->regHABLCop |= HC_HABLFCb_InvOPC | HC_HABLFCb_HABLRCb;
                    vmesa->regHABLAop |= HC_HABLFAb_InvOPA | HC_HABLFAb_HABLFRA;
                    vmesa->regHABLRFCb = 0x0;
                    vmesa->regHABLRAb = 0x0;
                }
                else {
                    /* (Ad, Ad, Ad, Ad)
                     */
                    vmesa->regHABLCop |= HC_HABLFCb_OPC | HC_HABLFCb_Adst;
                    vmesa->regHABLAop |= HC_HABLFAb_OPA | HC_HABLFAb_Adst;
                }
            }
            break;
        case GL_ONE_MINUS_DST_ALPHA:
            {
                if (vmesa->viaScreen->bitsPerPixel == 16) {
                    /* (1, 1, 1, 1) - (1, 1, 1, 1) = (0, 0, 0, 0)
                     */
                    vmesa->regHABLCop |= HC_HABLFCb_OPC | HC_HABLFCb_HABLRCb;
                    vmesa->regHABLAop |= HC_HABLFAb_OPA | HC_HABLFAb_HABLFRA;
                    vmesa->regHABLRFCb = 0x0;
                    vmesa->regHABLRAb = 0x0;
                }
                else {
                    /* (1, 1, 1, 1) - (Ad, Ad, Ad, Ad)
                     */
                    vmesa->regHABLCop |= HC_HABLFCb_InvOPC | HC_HABLFCb_Adst;
                    vmesa->regHABLAop |= HC_HABLFAb_InvOPA | HC_HABLFAb_Adst;
                }
            }
            break;
        default:
            vmesa->regHABLCop |= HC_HABLFCb_OPC | HC_HABLFCb_HABLRCb;
            vmesa->regHABLAop |= HC_HABLFAb_OPA | HC_HABLFAb_HABLFRA;
            vmesa->regHABLRFCb = 0x0;
            vmesa->regHABLRAb = 0x0;
            break;
        }

        if (vmesa->viaScreen->bitsPerPixel <= 16)
            vmesa->regEnable &= ~HC_HenDT_MASK;

    }
    else {
        vmesa->regEnable &= (~HC_HenABL_MASK);
    }

    if (ctx->Color.AlphaEnabled) {
        vmesa->regEnable |= HC_HenAT_MASK;
        vmesa->regHATMD = FLOAT_TO_UBYTE(ctx->Color.AlphaRef) |
            ((ctx->Color.AlphaFunc - GL_NEVER) << 8);
    }
    else {
        vmesa->regEnable &= (~HC_HenAT_MASK);
    }

    if (ctx->Color.DitherFlag && (vmesa->viaScreen->bitsPerPixel < 32)) {
        if (ctx->Color.BlendEnabled) {
            vmesa->regEnable &= ~HC_HenDT_MASK;
        }
        else {
            vmesa->regEnable |= HC_HenDT_MASK;
        }
    }


    vmesa->regEnable &= ~HC_HenDT_MASK;

    if (ctx->Color.ColorLogicOpEnabled) 
        vmesa->regHROP = ROP[ctx->Color.LogicOp & 0xF];
    else
        vmesa->regHROP = HC_HROP_P;

    vmesa->regHFBBMSKL = PACK_COLOR_888(ctx->Color.ColorMask[0],
					ctx->Color.ColorMask[1],
					ctx->Color.ColorMask[2]);
    vmesa->regHROP |= ctx->Color.ColorMask[3];

    if (ctx->Color.ColorMask[3])
        vmesa->regEnable |= HC_HenAW_MASK;
    else
        vmesa->regEnable &= ~HC_HenAW_MASK;
}

static void viaChooseFogState(GLcontext *ctx) 
{
    struct via_context *vmesa = VIA_CONTEXT(ctx);

    if (ctx->Fog.Enabled) {
        GLubyte r, g, b, a;

        vmesa->regEnable |= HC_HenFOG_MASK;

        /* Use fog equation 0 (OpenGL's default) & local fog.
         */
        vmesa->regHFogLF = 0x0;

        r = (GLubyte)(ctx->Fog.Color[0] * 255.0F);
        g = (GLubyte)(ctx->Fog.Color[1] * 255.0F);
        b = (GLubyte)(ctx->Fog.Color[2] * 255.0F);
        a = (GLubyte)(ctx->Fog.Color[3] * 255.0F);
        vmesa->regHFogCL = (r << 16) | (g << 8) | b;
        vmesa->regHFogCH = a;
    }
    else {
        vmesa->regEnable &= ~HC_HenFOG_MASK;
    }
}

static void viaChooseDepthState(GLcontext *ctx) 
{
    struct via_context *vmesa = VIA_CONTEXT(ctx);
    if (ctx->Depth.Test) {
        vmesa->regEnable |= HC_HenZT_MASK;
        if (ctx->Depth.Mask)
            vmesa->regEnable |= HC_HenZW_MASK;
        else
            vmesa->regEnable &= (~HC_HenZW_MASK);
	vmesa->regHZWTMD = (ctx->Depth.Func - GL_NEVER) << 16;
	
    }
    else {
        vmesa->regEnable &= ~HC_HenZT_MASK;
        
        /*=* [DBG] racer : can't display cars in car selection menu *=*/
	/*if (ctx->Depth.Mask)
            vmesa->regEnable |= HC_HenZW_MASK;
        else
            vmesa->regEnable &= (~HC_HenZW_MASK);*/
	vmesa->regEnable &= (~HC_HenZW_MASK);
    }
}

static void viaChooseLineState(GLcontext *ctx) 
{
    struct via_context *vmesa = VIA_CONTEXT(ctx);

    if (ctx->Line.StippleFlag) {
        vmesa->regEnable |= HC_HenLP_MASK;
        vmesa->regHLP = ctx->Line.StipplePattern;
        vmesa->regHLPRF = ctx->Line.StippleFactor;
    }
    else {
        vmesa->regEnable &= ~HC_HenLP_MASK;
    }
}

static void viaChoosePolygonState(GLcontext *ctx) 
{
    struct via_context *vmesa = VIA_CONTEXT(ctx);

#if 0
    /* Polygon stipple is broken - see via_state.c
     */
    if (ctx->Polygon.StippleFlag) {
        vmesa->regEnable |= HC_HenSP_MASK;
    }
    else {
        vmesa->regEnable &= ~HC_HenSP_MASK;
    }
#else
    FALLBACK(vmesa, VIA_FALLBACK_POLY_STIPPLE, 
	     ctx->Polygon.StippleFlag);
#endif

    if (ctx->Polygon.CullFlag) {
        vmesa->regEnable |= HC_HenFBCull_MASK;
    }
    else {
        vmesa->regEnable &= ~HC_HenFBCull_MASK;
    }
}

static void viaChooseStencilState(GLcontext *ctx) 
{
    struct via_context *vmesa = VIA_CONTEXT(ctx);
    
    if (ctx->Stencil.Enabled) {
        GLuint temp;

        vmesa->regEnable |= HC_HenST_MASK;
        temp = (ctx->Stencil.Ref[0] & 0xFF) << HC_HSTREF_SHIFT;
        temp |= 0xFF << HC_HSTOPMSK_SHIFT;
        temp |= (ctx->Stencil.ValueMask[0] & 0xFF);
        vmesa->regHSTREF = temp;

        temp = (ctx->Stencil.Function[0] - GL_NEVER) << 16;

        switch (ctx->Stencil.FailFunc[0]) {
        case GL_KEEP:
            temp |= HC_HSTOPSF_KEEP;
            break;
        case GL_ZERO:
            temp |= HC_HSTOPSF_ZERO;
            break;
        case GL_REPLACE:
            temp |= HC_HSTOPSF_REPLACE;
            break;
        case GL_INVERT:
            temp |= HC_HSTOPSF_INVERT;
            break;
        case GL_INCR:
            temp |= HC_HSTOPSF_INCR;
            break;
        case GL_DECR:
            temp |= HC_HSTOPSF_DECR;
            break;
        }

        switch (ctx->Stencil.ZFailFunc[0]) {
        case GL_KEEP:
            temp |= HC_HSTOPSPZF_KEEP;
            break;
        case GL_ZERO:
            temp |= HC_HSTOPSPZF_ZERO;
            break;
        case GL_REPLACE:
            temp |= HC_HSTOPSPZF_REPLACE;
            break;
        case GL_INVERT:
            temp |= HC_HSTOPSPZF_INVERT;
            break;
        case GL_INCR:
            temp |= HC_HSTOPSPZF_INCR;
            break;
        case GL_DECR:
            temp |= HC_HSTOPSPZF_DECR;
            break;
        }

        switch (ctx->Stencil.ZPassFunc[0]) {
        case GL_KEEP:
            temp |= HC_HSTOPSPZP_KEEP;
            break;
        case GL_ZERO:
            temp |= HC_HSTOPSPZP_ZERO;
            break;
        case GL_REPLACE:
            temp |= HC_HSTOPSPZP_REPLACE;
            break;
        case GL_INVERT:
            temp |= HC_HSTOPSPZP_INVERT;
            break;
        case GL_INCR:
            temp |= HC_HSTOPSPZP_INCR;
            break;
        case GL_DECR:
            temp |= HC_HSTOPSPZP_DECR;
            break;
        }
        vmesa->regHSTMD = temp;
    }
    else {
        vmesa->regEnable &= ~HC_HenST_MASK;
    }
}



static void viaChooseTriangle(GLcontext *ctx) 
{       
    struct via_context *vmesa = VIA_CONTEXT(ctx);

    if (ctx->Polygon.CullFlag == GL_TRUE) {
        switch (ctx->Polygon.CullFaceMode) {
        case GL_FRONT:
            if (ctx->Polygon.FrontFace == GL_CCW)
                vmesa->regCmdB |= HC_HBFace_MASK;
            else
                vmesa->regCmdB &= ~HC_HBFace_MASK;
            break;
        case GL_BACK:
            if (ctx->Polygon.FrontFace == GL_CW)
                vmesa->regCmdB |= HC_HBFace_MASK;
            else
                vmesa->regCmdB &= ~HC_HBFace_MASK;
            break;
        case GL_FRONT_AND_BACK:
            return;
        }
    }
}

void viaValidateState( GLcontext *ctx )
{
    struct via_context *vmesa = VIA_CONTEXT(ctx);

    if (vmesa->newState & _NEW_TEXTURE) {
       GLboolean ok = (viaChooseTextureState(ctx) &&
		       viaUpdateTextureState(ctx));

       FALLBACK(vmesa, VIA_FALLBACK_TEXTURE, !ok);
    }

    if (vmesa->newState & _NEW_COLOR)
        viaChooseColorState(ctx);

    if (vmesa->newState & _NEW_DEPTH)
        viaChooseDepthState(ctx);

    if (vmesa->newState & _NEW_FOG)
        viaChooseFogState(ctx);

    if (vmesa->newState & _NEW_LINE)
        viaChooseLineState(ctx);

    if (vmesa->newState & (_NEW_POLYGON | _NEW_POLYGONSTIPPLE)) {
        viaChoosePolygonState(ctx);
	viaChooseTriangle(ctx);
    }

    if ((vmesa->newState & _NEW_STENCIL) && vmesa->have_hw_stencil)
        viaChooseStencilState(ctx);
    
    if (ctx->_TriangleCaps & DD_SEPARATE_SPECULAR)
        vmesa->regEnable |= HC_HenCS_MASK;
    else
        vmesa->regEnable &= ~HC_HenCS_MASK;

    if (ctx->Point.SmoothFlag ||
	ctx->Line.SmoothFlag ||
	ctx->Polygon.SmoothFlag)
        vmesa->regEnable |= HC_HenAA_MASK;
    else 
        vmesa->regEnable &= ~HC_HenAA_MASK;

    vmesa->newEmitState |= vmesa->newState;
    vmesa->newState = 0;
}

static void viaInvalidateState(GLcontext *ctx, GLuint newState)
{
    struct via_context *vmesa = VIA_CONTEXT(ctx);

    VIA_FINISH_PRIM( vmesa );
    vmesa->newState |= newState;

    _swrast_InvalidateState(ctx, newState);
    _swsetup_InvalidateState(ctx, newState);
    _vbo_InvalidateState(ctx, newState);
    _tnl_InvalidateState(ctx, newState);
}

void viaInitStateFuncs(GLcontext *ctx)
{
    /* Callbacks for internal Mesa events.
     */
    ctx->Driver.UpdateState = viaInvalidateState;

    /* API callbacks
     */
    ctx->Driver.BlendEquationSeparate = viaBlendEquationSeparate;
    ctx->Driver.BlendFuncSeparate = viaBlendFuncSeparate;
    ctx->Driver.ClearColor = viaClearColor;
    ctx->Driver.ColorMask = viaColorMask;
    ctx->Driver.DrawBuffer = viaDrawBuffer;
    ctx->Driver.RenderMode = viaRenderMode;
    ctx->Driver.Scissor = viaScissor;
    ctx->Driver.DepthRange = viaDepthRange;
    ctx->Driver.Viewport = viaViewport;
    ctx->Driver.Enable = viaEnable;

    /* XXX this should go away */
    ctx->Driver.ResizeBuffers = viaReAllocateBuffers;
}
