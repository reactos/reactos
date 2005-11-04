/**************************************************************************

Copyright 2001 2d3d Inc., Delray Beach, FL

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
ATI, VA LINUX SYSTEMS AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
USE OR OTHER DEALINGS IN THE SOFTWARE.

**************************************************************************/

/* $XFree86: xc/lib/GL/mesa/src/drv/i830/i830_debug.c,v 1.3 2002/12/10 01:26:53 dawes Exp $ */

/*
 * Author:
 *   Jeff Hartmann <jhartmann@2d3d.com>
 */

#include "glheader.h"
#include "context.h"
#include "macros.h"
#include "enums.h"
#include "dd.h"
                                            
#include "mm.h"

#include "i830_screen.h"
#include "i830_dri.h"

#include "i830_context.h"
#include "i830_state.h"
#include "i830_tex.h"
#include "i830_tris.h"
#include "i830_ioctl.h"
#include "i830_debug.h"

#include "swrast/swrast.h"
#include "array_cache/acache.h"
#include "tnl/tnl.h"
#include "swrast_setup/swrast_setup.h"
                                        
#include "tnl/t_pipeline.h"


#define TINY_VERTEX_FORMAT      (STATE3D_VERTEX_FORMAT_CMD | \
				 VRTX_TEX_COORD_COUNT(0) | \
				 VRTX_HAS_DIFFUSE | \
				 VRTX_HAS_XYZ)

#define NOTEX_VERTEX_FORMAT     (STATE3D_VERTEX_FORMAT_CMD | \
				 VRTX_TEX_COORD_COUNT(0) | \
				 VRTX_HAS_DIFFUSE | \
				 VRTX_HAS_SPEC | \
				 VRTX_HAS_XYZW)

#define TEX0_VERTEX_FORMAT      (STATE3D_VERTEX_FORMAT_CMD | \
				 VRTX_TEX_COORD_COUNT(1) | \
				 VRTX_HAS_DIFFUSE | \
				 VRTX_HAS_SPEC | \
				 VRTX_HAS_XYZW)

#define TEX1_VERTEX_FORMAT      (STATE3D_VERTEX_FORMAT_CMD | \
				 VRTX_TEX_COORD_COUNT(2) | \
				 VRTX_HAS_DIFFUSE | \
				 VRTX_HAS_SPEC | \
				 VRTX_HAS_XYZW)

#define PROJ_VF2		(STATE3D_VERTEX_FORMAT_2_CMD |		\
	    			 VRTX_TEX_SET_0_FMT(TEXCOORDFMT_3D) |	\
				 VRTX_TEX_SET_1_FMT(TEXCOORDFMT_3D) |	\
				 VRTX_TEX_SET_2_FMT(TEXCOORDFMT_3D) |	\
				 VRTX_TEX_SET_3_FMT(TEXCOORDFMT_3D))

#define NON_PROJ_VF2		(STATE3D_VERTEX_FORMAT_2_CMD |		\
	    			 VRTX_TEX_SET_0_FMT(TEXCOORDFMT_2D) |	\
				 VRTX_TEX_SET_1_FMT(TEXCOORDFMT_2D) |	\
				 VRTX_TEX_SET_2_FMT(TEXCOORDFMT_2D) |	\
				 VRTX_TEX_SET_3_FMT(TEXCOORDFMT_2D))

void i830DumpContextState( i830ContextPtr imesa )
{
   GLuint *Context = imesa->Setup;
   
   fprintf(stderr, "%s\n", __FUNCTION__);
   fprintf(stderr, "STATE1 : 0x%08x\n", Context[I830_CTXREG_STATE1]);
   fprintf(stderr, "STATE2 : 0x%08x\n", Context[I830_CTXREG_STATE2]);
   fprintf(stderr, "STATE3 : 0x%08x\n", Context[I830_CTXREG_STATE3]);
   fprintf(stderr, "STATE4 : 0x%08x\n", Context[I830_CTXREG_STATE4]);
   fprintf(stderr, "STATE5 : 0x%08x\n", Context[I830_CTXREG_STATE5]);
   fprintf(stderr, "IALPHAB : 0x%08x\n", Context[I830_CTXREG_IALPHAB]);
   fprintf(stderr, "STENCILTST : 0x%08x\n", Context[I830_CTXREG_STENCILTST]);
   fprintf(stderr, "ENABLES_1 : 0x%08x\n", Context[I830_CTXREG_ENABLES_1]);
   fprintf(stderr, "ENABLES_2 : 0x%08x\n", Context[I830_CTXREG_ENABLES_2]);
   fprintf(stderr, "AA : 0x%08x\n", Context[I830_CTXREG_AA]);
   fprintf(stderr, "FOGCOLOR : 0x%08x\n", Context[I830_CTXREG_FOGCOLOR]);
   fprintf(stderr, "BCOLOR0 : 0x%08x\n", Context[I830_CTXREG_BLENDCOLR0]);
   fprintf(stderr, "BCOLOR : 0x%08x\n", Context[I830_CTXREG_BLENDCOLR]);
   fprintf(stderr, "VF : 0x%08x\n", Context[I830_CTXREG_VF]);
   fprintf(stderr, "VF2 : 0x%08x\n", Context[I830_CTXREG_VF2]);
   fprintf(stderr, "MCSB0 : 0x%08x\n", Context[I830_CTXREG_MCSB0]);
   fprintf(stderr, "MCSB1 : 0x%08x\n", Context[I830_CTXREG_MCSB1]);
}

void i830DumpBufferState( i830ContextPtr imesa )
{
   GLuint *Buffer = imesa->BufferSetup;

   fprintf(stderr, "%s\n", __FUNCTION__);
   fprintf(stderr, "CBUFADDR : 0x%08x\n", Buffer[I830_DESTREG_CBUFADDR]);
   fprintf(stderr, "DBUFADDR : 0x%08x\n", Buffer[I830_DESTREG_DBUFADDR]);
   fprintf(stderr, "DV0 : 0x%08x\n", Buffer[I830_DESTREG_DV0]);
   fprintf(stderr, "DV1 : 0x%08x\n", Buffer[I830_DESTREG_DV1]);
   fprintf(stderr, "SENABLE : 0x%08x\n", Buffer[I830_DESTREG_SENABLE]);
   fprintf(stderr, "SR0 : 0x%08x\n", Buffer[I830_DESTREG_SR0]);
   fprintf(stderr, "SR1 : 0x%08x\n", Buffer[I830_DESTREG_SR1]);
   fprintf(stderr, "SR2 : 0x%08x\n", Buffer[I830_DESTREG_SR2]);
   fprintf(stderr, "DR0 : 0x%08x\n", Buffer[I830_DESTREG_DR0]);
   fprintf(stderr, "DR1 : 0x%08x\n", Buffer[I830_DESTREG_DR1]);
   fprintf(stderr, "DR2 : 0x%08x\n", Buffer[I830_DESTREG_DR2]);
   fprintf(stderr, "DR3 : 0x%08x\n", Buffer[I830_DESTREG_DR3]);
   fprintf(stderr, "DR4 : 0x%08x\n", Buffer[I830_DESTREG_DR4]);
}

void i830DumpStippleState( i830ContextPtr imesa )
{
   GLuint *Buffer = imesa->BufferSetup;

   fprintf(stderr, "%s\n", __FUNCTION__);
   fprintf(stderr, "ST1 : 0x%08x\n", Buffer[I830_STPREG_ST1]);
}

void i830DumpTextureState( i830ContextPtr imesa, int unit )
{
   i830TextureObjectPtr t = imesa->CurrentTexObj[unit];

   if(t) {
      fprintf(stderr, "%s : unit %d\n", __FUNCTION__, unit);
      fprintf(stderr, "TM0LI : 0x%08x\n", t->Setup[I830_TEXREG_TM0LI]);
      fprintf(stderr, "TM0S0 : 0x%08x\n", t->Setup[I830_TEXREG_TM0S0]);
      fprintf(stderr, "TM0S1 : 0x%08x\n", t->Setup[I830_TEXREG_TM0S1]);
      fprintf(stderr, "TM0S2 : 0x%08x\n", t->Setup[I830_TEXREG_TM0S2]);
      fprintf(stderr, "TM0S3 : 0x%08x\n", t->Setup[I830_TEXREG_TM0S3]);
      fprintf(stderr, "TM0S4 : 0x%08x\n", t->Setup[I830_TEXREG_TM0S4]);
      fprintf(stderr, "NOP0 : 0x%08x\n", t->Setup[I830_TEXREG_NOP0]);
      fprintf(stderr, "NOP1 : 0x%08x\n", t->Setup[I830_TEXREG_NOP1]);
      fprintf(stderr, "NOP2 : 0x%08x\n", t->Setup[I830_TEXREG_NOP2]);
      fprintf(stderr, "MCS : 0x%08x\n", t->Setup[I830_TEXREG_MCS]);
   }
}

void i830DumpTextureBlendState( i830ContextPtr imesa, int unit )
{
   GLuint *TexBlend = imesa->TexBlend[unit];
   GLuint length = imesa->TexBlendWordsUsed[unit];
   int i;

   fprintf(stderr, "%s : unit %d : length %d\n", __FUNCTION__, unit, length);
   for(i = 0; i < length; i++) {
      fprintf(stderr, "[%d] : 0x%08x\n", i, TexBlend[i]);
   }
}

void i830VertexSanity( i830ContextPtr imesa, drmI830Vertex vertex )
{
   I830SAREAPtr sarea = imesa->sarea;
   char *prim_name;
   int size = 0;
   int vfmt_size = 0;
   int hw_nr_vertex = 0;
   int hw_start_vertex = 0;

   /* Do a bunch of sanity checks on the vertices sent to the hardware */

   size = vertex.used - 4;
   if(imesa->vertex_size && (size % imesa->vertex_size) != 0) {
      fprintf(stderr, "\n\nVertex size does not match imesa "
	      "internal state\n");
      fprintf(stderr, "Buffer size : %d\n", size);
      fprintf(stderr, "Vertex size : %d\n", imesa->vertex_size);
   }

   /* Check to see if the vertex format is good, and get its size */
   if (sarea->ContextState[I830_CTXREG_VF] == TINY_VERTEX_FORMAT) {
      vfmt_size = 16; /* 4 dwords */
   } else if (sarea->ContextState[I830_CTXREG_VF] == 
	      NOTEX_VERTEX_FORMAT) {
      vfmt_size = 24; /* 6 dwords */
   } else if (sarea->ContextState[I830_CTXREG_VF] == 
	      TEX0_VERTEX_FORMAT) {
      vfmt_size = 32; /* 8 dwords */
      if (sarea->ContextState[I830_CTXREG_VF2] != NON_PROJ_VF2) {
	 fprintf(stderr, "\n\nTex 0 vertex format, but proj "
		 "texturing\n");
      }
   } else if(sarea->ContextState[I830_CTXREG_VF] == 
	     TEX1_VERTEX_FORMAT) {
      if (sarea->ContextState[I830_CTXREG_VF2] == NON_PROJ_VF2)
	vfmt_size = 40; /* 10 dwords */
      else
	vfmt_size = 48; /* 12 dwords */
   } else {
      fprintf(stderr, "\n\nUnknown vertex format : vf : %08x "
	      "vf2 : %08x\n",
	      sarea->ContextState[I830_CTXREG_VF],
	      sarea->ContextState[I830_CTXREG_VF2]);
   }

   if(vfmt_size && (size % vfmt_size) != 0) {
      fprintf(stderr, "\n\nVertex size does not match hardware "
	      "internal state\n");
      fprintf(stderr, "Buffer size : %d\n", size);
      fprintf(stderr, "Vertex size : %d\n", vfmt_size);
   }

   switch(sarea->vertex_prim) {
   case PRIM3D_POINTLIST:
      hw_start_vertex = 0;
      hw_nr_vertex = 1;
      prim_name = "PointList";
      break;

   case PRIM3D_LINELIST:
      hw_start_vertex = 0;
      hw_nr_vertex = 2;
      prim_name = "LineList";
      break;

   case PRIM3D_LINESTRIP:
      hw_start_vertex = 2;
      hw_nr_vertex = 1;
      prim_name = "LineStrip";
      break;

   case PRIM3D_TRILIST:
      hw_start_vertex = 0;
      hw_nr_vertex = 3;
      prim_name = "TriList";
      break;

   case PRIM3D_TRISTRIP:
      hw_start_vertex = 3;
      hw_nr_vertex = 1;
      prim_name = "TriStrip";
      break;

   case PRIM3D_TRIFAN:
      hw_start_vertex = 3;
      hw_nr_vertex = 1;
      prim_name = "TriFan";
      break;

   case PRIM3D_POLY:
      hw_start_vertex = 3;
      hw_nr_vertex = 1;
      prim_name = "Polygons";
      break;
   default:
      prim_name = "Unknown";
      fprintf(stderr, "\n\nUnknown primitive type : %08x\n",
	      sarea->vertex_prim);
   }
	    
   if (hw_nr_vertex && vfmt_size) {
      int temp_size = size - (hw_start_vertex * vfmt_size);
      int remaining = (temp_size % (hw_nr_vertex * vfmt_size));

      if (remaining != 0) {
	 fprintf(stderr, "\n\nThis buffer contains an improper"
		 " multiple of vertices for this primitive : %s\n",
		 prim_name);
	 fprintf(stderr, "Number of vertices in buffer : %d\n",
		 size / vfmt_size);
	 fprintf(stderr, "temp_size : %d\n", temp_size);
	 fprintf(stderr, "remaining vertices : %d", 
		 remaining / vfmt_size);
      }
   }
   if (vfmt_size) {
      fprintf(stderr, "\n\nPrim name (%s), vertices (%d)\n",
	      prim_name,
	      size / vfmt_size);
   }
}

void i830EmitHwStateLockedDebug( i830ContextPtr imesa )
{
   int i;

   if ((imesa->dirty & I830_UPLOAD_TEX0_IMAGE) && imesa->CurrentTexObj[0]) {
      i830UploadTexImagesLocked(imesa, imesa->CurrentTexObj[0]);
   }

   if ((imesa->dirty & I830_UPLOAD_TEX1_IMAGE) && imesa->CurrentTexObj[1]) {
      i830UploadTexImagesLocked(imesa, imesa->CurrentTexObj[1]);
   }

   if (imesa->dirty & I830_UPLOAD_CTX) {
      memcpy( imesa->sarea->ContextState,
	     imesa->Setup, sizeof(imesa->Setup) );
      i830DumpContextState(imesa);
   }

   for(i = 0; i < I830_TEXTURE_COUNT; i++) {
      if ((imesa->dirty & I830_UPLOAD_TEX_N(i)) && imesa->CurrentTexObj[i]) {
	 unsigned * TexState;

	 imesa->sarea->dirty |= I830_UPLOAD_TEX_N(i);

	 switch( i ) {
	 case 0:
	 case 1:
	    TexState = imesa->sarea->TexState[i];
	    break;

	 case 2:
	    TexState = imesa->sarea->TexState2;
	    break;

	 case 3:
	    TexState = imesa->sarea->TexState3;
	    break;
	 }

	 memcpy(TexState, imesa->CurrentTexObj[i]->Setup,
		sizeof(imesa->sarea->TexState[i]));
	 i830DumpTextureState(imesa, i);
      }
   }
   /* Need to figure out if texturing state, or enable changed. */

   for(i = 0; i < I830_TEXBLEND_COUNT; i++) {
      if (imesa->dirty & I830_UPLOAD_TEXBLEND_N(i)) {
	 unsigned * TexBlendState;
	 unsigned * words_used;
	 
	 imesa->sarea->dirty |= I830_UPLOAD_TEXBLEND_N(i);

	 switch( i ) {
	 case 0:
	 case 1:
	    TexBlendState = imesa->sarea->TexBlendState[i];
	    words_used = & imesa->sarea->TexBlendStateWordsUsed[i];
	    break;

	 case 2:
	    TexBlendState = imesa->sarea->TexBlendState2;
	    words_used = & imesa->sarea->TexBlendStateWordsUsed2;
	    break;

	 case 3:
	    TexBlendState = imesa->sarea->TexBlendState3;
	    words_used = & imesa->sarea->TexBlendStateWordsUsed3;
	    break;
	 }

	 memcpy(TexBlendState, imesa->TexBlend[i],
		imesa->TexBlendWordsUsed[i] * 4);
	 *words_used = imesa->TexBlendWordsUsed[i];

	 i830DumpTextureBlendState(imesa, i);
      }
   }

   if (imesa->dirty & I830_UPLOAD_BUFFERS) {
      memcpy( imesa->sarea->BufferState,imesa->BufferSetup, 
	      sizeof(imesa->BufferSetup) );
      i830DumpBufferState(imesa);
   }

   if (imesa->dirty & I830_UPLOAD_STIPPLE) {
      fprintf(stderr, "UPLOAD_STIPPLE\n");
      memcpy( imesa->sarea->StippleState,imesa->StippleSetup, 
	      sizeof(imesa->StippleSetup) );
      i830DumpStippleState(imesa);
   }

   if (imesa->dirty & I830_UPLOAD_TEX_PALETTE_SHARED) {
      memcpy( imesa->sarea->Palette[0],imesa->palette,
	      sizeof(imesa->sarea->Palette[0]));
   } else {
      i830TextureObjectPtr p;
      if (imesa->dirty & I830_UPLOAD_TEX_PALETTE_N(0)) {
	 p = imesa->CurrentTexObj[0];
	 memcpy( imesa->sarea->Palette[0],p->palette,
		sizeof(imesa->sarea->Palette[0]));
      }
      if (imesa->dirty & I830_UPLOAD_TEX_PALETTE_N(1)) {
	 p = imesa->CurrentTexObj[1];
	 memcpy( imesa->sarea->Palette[1],
		 p->palette,
		 sizeof(imesa->sarea->Palette[1]));
      }
   }
   imesa->sarea->dirty |= (imesa->dirty & ~(I830_UPLOAD_TEX_MASK | 
					    I830_UPLOAD_TEXBLEND_MASK));

   imesa->upload_cliprects = GL_TRUE;
   imesa->dirty = 0;
}
