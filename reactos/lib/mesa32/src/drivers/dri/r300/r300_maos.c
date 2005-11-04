/* $XFree86: xc/lib/GL/mesa/src/drv/r300/r300_maos_arrays.c,v 1.3 2003/02/23 23:59:01 dawes Exp $ */
/*
Copyright (C) The Weather Channel, Inc.  2002.  All Rights Reserved.

The Weather Channel (TM) funded Tungsten Graphics to develop the
initial release of the Radeon 8500 driver under the XFree86 license.
This notice must be preserved.

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice (including the
next paragraph) shall be included in all copies or substantial
portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE COPYRIGHT OWNER(S) AND/OR ITS SUPPLIERS BE
LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

**************************************************************************/

/*
 * Authors:
 *   Keith Whitwell <keith@tungstengraphics.com>
 */

#include "glheader.h"
#include "mtypes.h"
#include "colormac.h"
#include "imports.h"
#include "macros.h"

#include "swrast_setup/swrast_setup.h"
#include "math/m_translate.h"
#include "tnl/tnl.h"
#include "tnl/t_context.h"

#include "r300_context.h"
#include "radeon_ioctl.h"
#include "r300_state.h"
#include "r300_maos.h"
#include "r300_ioctl.h"

#define DEBUG_ALL DEBUG_VERTS


#if defined(USE_X86_ASM)
#define COPY_DWORDS( dst, src, nr )					\
do {									\
	int __tmp;							\
	__asm__ __volatile__( "rep ; movsl"				\
			      : "=%c" (__tmp), "=D" (dst), "=S" (__tmp)	\
			      : "0" (nr),				\
			        "D" ((long)dst),			\
			        "S" ((long)src) );			\
} while (0)
#else
#define COPY_DWORDS( dst, src, nr )		\
do {						\
   int j;					\
   for ( j = 0 ; j < nr ; j++ )			\
      dst[j] = ((int *)src)[j];			\
   dst += nr;					\
} while (0)
#endif

static void emit_vec4(GLcontext * ctx,
		      struct r300_dma_region *rvb,
		      GLvoid *data, int stride, int count)
{
	int i;
	int *out = (int *)(rvb->address + rvb->start);

	if (RADEON_DEBUG & DEBUG_VERTS)
		fprintf(stderr, "%s count %d stride %d\n",
			__FUNCTION__, count, stride);

	if (stride == 4)
		COPY_DWORDS(out, data, count);
	else
		for (i = 0; i < count; i++) {
			out[0] = *(int *)data;
			out++;
			data += stride;
		}
}

static void emit_vec8(GLcontext * ctx,
		      struct r300_dma_region *rvb,
		      GLvoid *data, int stride, int count)
{
	int i;
	int *out = (int *)(rvb->address + rvb->start);

	if (RADEON_DEBUG & DEBUG_VERTS)
		fprintf(stderr, "%s count %d stride %d\n",
			__FUNCTION__, count, stride);

	if (stride == 8)
		COPY_DWORDS(out, data, count * 2);
	else
		for (i = 0; i < count; i++) {
			out[0] = *(int *)data;
			out[1] = *(int *)(data + 4);
			out += 2;
			data += stride;
		}
}

static void emit_vec12(GLcontext * ctx,
		       struct r300_dma_region *rvb,
		       GLvoid *data, int stride, int count)
{
	int i;
	int *out = (int *)(rvb->address + rvb->start);

	if (RADEON_DEBUG & DEBUG_VERTS)
		fprintf(stderr, "%s count %d stride %d out %p data %p\n",
			__FUNCTION__, count, stride, (void *)out, (void *)data);

	if (stride == 12)
		COPY_DWORDS(out, data, count * 3);
	else
		for (i = 0; i < count; i++) {
			out[0] = *(int *)data;
			out[1] = *(int *)(data + 4);
			out[2] = *(int *)(data + 8);
			out += 3;
			data += stride;
		}
}

static void emit_vec16(GLcontext * ctx,
		       struct r300_dma_region *rvb,
		       GLvoid *data, int stride, int count)
{
	int i;
	int *out = (int *)(rvb->address + rvb->start);

	if (RADEON_DEBUG & DEBUG_VERTS)
		fprintf(stderr, "%s count %d stride %d\n",
			__FUNCTION__, count, stride);

	if (stride == 16)
		COPY_DWORDS(out, data, count * 4);
	else
		for (i = 0; i < count; i++) {
			out[0] = *(int *)data;
			out[1] = *(int *)(data + 4);
			out[2] = *(int *)(data + 8);
			out[3] = *(int *)(data + 12);
			out += 4;
			data += stride;
		}
}

static void emit_vector(GLcontext * ctx,
			struct r300_dma_region *rvb,
			GLvoid *data, int size, int stride, int count)
{
	r300ContextPtr rmesa = R300_CONTEXT(ctx);

	if (RADEON_DEBUG & DEBUG_VERTS)
		fprintf(stderr, "%s count %d size %d stride %d\n",
			__FUNCTION__, count, size, stride);

	if(r300IsGartMemory(rmesa, data, size*stride)){
		rvb->address = rmesa->radeon.radeonScreen->gartTextures.map;
		rvb->start = (char *)data - rvb->address;
		rvb->aos_offset = r300GartOffsetFromVirtual(rmesa, data);
		
		if(stride == 0)
			rvb->aos_stride	= 0;
		else
			rvb->aos_stride	= stride / 4;
		
		rvb->aos_size = size;
		return;
	}
	
	/* Gets triggered when playing with future_hw_tcl_on ...*/
	//assert(!rvb->buf);

	if (stride == 0) {
		r300AllocDmaRegion(rmesa, rvb, size * 4, 4);
		count = 1;
		rvb->aos_offset	= GET_START(rvb);
		rvb->aos_stride	= 0;
		rvb->aos_size	= size;
	} else {
		r300AllocDmaRegion(rmesa, rvb, size * count * 4, 4);	/* alignment? */
		rvb->aos_offset	= GET_START(rvb);
		rvb->aos_stride	= size;
		rvb->aos_size	= size;
	}
	
	/* Emit the data
	 */
	switch (size) {
	case 1:
		emit_vec4(ctx, rvb, data, stride, count);
		break;
	case 2:
		emit_vec8(ctx, rvb, data, stride, count);
		break;
	case 3:
		emit_vec12(ctx, rvb, data, stride, count);
		break;
	case 4:
		emit_vec16(ctx, rvb, data, stride, count);
		break;
	default:
		assert(0);
		exit(1);
		break;
	}

}

void r300EmitElts(GLcontext * ctx, GLuint *elts, unsigned long n_elts)
{
	r300ContextPtr rmesa = R300_CONTEXT(ctx);
	struct r300_dma_region *rvb=&rmesa->state.elt_dma;
	unsigned short int *out;
	int i;
	
	if(r300IsGartMemory(rmesa, elts, n_elts*sizeof(unsigned short int))){
		rvb->address = rmesa->radeon.radeonScreen->gartTextures.map;
		rvb->start = (char *)elts - rvb->address;
		rvb->aos_offset = rmesa->radeon.radeonScreen->gart_texture_offset + rvb->start;
		return ;
	}
	
	r300AllocDmaRegion(rmesa, rvb, n_elts*sizeof(unsigned short int), 2);
	
	out = (unsigned short int *)(rvb->address + rvb->start);
	
	for(i=0; i < n_elts; i++)
		out[i]=(unsigned short int)elts[i];
}

/* Emit vertex data to GART memory (unless immediate mode)
 * Route inputs to the vertex processor
 */
void r300EmitArrays(GLcontext * ctx, GLboolean immd)
{
	r300ContextPtr rmesa = R300_CONTEXT(ctx);
	r300ContextPtr r300 = rmesa;
	struct vertex_buffer *VB = &TNL_CONTEXT(ctx)->vb;
	GLuint nr = 0;
	GLuint count = VB->Count;
	GLuint dw,mask;
	GLuint vic_1 = 0;	/* R300_VAP_INPUT_CNTL_1 */
	GLuint aa_vap_reg = 0; /* VAP register assignment */
	GLuint i;
	GLuint inputs = 0;
	

#define CONFIGURE_AOS(r, f, v, sz, cn) { \
		if (RADEON_DEBUG & DEBUG_STATE) \
			fprintf(stderr, "Enabling "#v "\n"); \
		if (++nr >= R300_MAX_AOS_ARRAYS) { \
			fprintf(stderr, "Aieee! AOS array count exceeded!\n"); \
			exit(-1); \
		} \
		\
		if (hw_tcl_on == GL_FALSE) \
			rmesa->state.aos[nr-1].aos_reg = aa_vap_reg++; \
		rmesa->state.aos[nr-1].aos_format = f; \
		if (immd) { \
			rmesa->state.aos[nr-1].aos_size = 4; \
			rmesa->state.aos[nr-1].aos_stride = 4; \
			rmesa->state.aos[nr-1].aos_offset = 0; \
		} else { \
			emit_vector(ctx, \
						&rmesa->state.aos[nr-1], \
						v->data, \
						sz, \
						v->stride, \
						cn); \
		rmesa->state.vap_reg.r=rmesa->state.aos[nr-1].aos_reg; \
		} \
}

	if (hw_tcl_on) {
		GLuint InputsRead = CURRENT_VERTEX_SHADER(ctx)->InputsRead;
		struct r300_vertex_program *prog=(struct r300_vertex_program *)CURRENT_VERTEX_SHADER(ctx);
		if (InputsRead & (1<<VERT_ATTRIB_POS)) {
			inputs |= _TNL_BIT_POS;
			rmesa->state.aos[nr++].aos_reg = prog->inputs[VERT_ATTRIB_POS];
		}
		if (InputsRead & (1<<VERT_ATTRIB_NORMAL)) {
			inputs |= _TNL_BIT_NORMAL;
			rmesa->state.aos[nr++].aos_reg = prog->inputs[VERT_ATTRIB_NORMAL];
		}
		if (InputsRead & (1<<VERT_ATTRIB_COLOR0)) {
			inputs |= _TNL_BIT_COLOR0;
			rmesa->state.aos[nr++].aos_reg = prog->inputs[VERT_ATTRIB_COLOR0];
		}
		if (InputsRead & (1<<VERT_ATTRIB_COLOR1)) {
			inputs |= _TNL_BIT_COLOR1;
			rmesa->state.aos[nr++].aos_reg = prog->inputs[VERT_ATTRIB_COLOR1];
		}
		if (InputsRead & (1<<VERT_ATTRIB_FOG)) {
			inputs |= _TNL_BIT_FOG;
			rmesa->state.aos[nr++].aos_reg = prog->inputs[VERT_ATTRIB_FOG];
		}
		if(ctx->Const.MaxTextureUnits > 8) { /* Not sure if this can even happen... */
			fprintf(stderr, "%s: Cant handle that many inputs\n", __FUNCTION__);
			exit(-1);
		}
		for (i=0;i<ctx->Const.MaxTextureUnits;i++) {
			if (InputsRead & (1<<(VERT_ATTRIB_TEX0+i))) {
				inputs |= _TNL_BIT_TEX0<<i;
				rmesa->state.aos[nr++].aos_reg = prog->inputs[VERT_ATTRIB_TEX0+i];
			}
		}
		nr = 0;
	} else {
		inputs = TNL_CONTEXT(ctx)->render_inputs;
	}
	rmesa->state.render_inputs = inputs;

	if (inputs & _TNL_BIT_POS) {
		CONFIGURE_AOS(i_coords,	AOS_FORMAT_FLOAT,
						VB->ObjPtr,
						immd ? 4 : VB->ObjPtr->size,
						count);

		vic_1 |= R300_INPUT_CNTL_POS;
	}

	if (inputs & _TNL_BIT_NORMAL) {
		CONFIGURE_AOS(i_normal,	AOS_FORMAT_FLOAT,
						VB->NormalPtr,
						immd ? 4 : VB->NormalPtr->size,
						count);

		vic_1 |= R300_INPUT_CNTL_NORMAL;
	}

	if (inputs & _TNL_BIT_COLOR0) {
		int emitsize=4;

		if (!immd) {
			if (VB->ColorPtr[0]->size == 4 &&
			    (VB->ColorPtr[0]->stride != 0 ||
			     VB->ColorPtr[0]->data[0][3] != 1.0)) {
				emitsize = 4;
			} else {
				emitsize = 3;
			}
		}

		CONFIGURE_AOS(i_color[0], AOS_FORMAT_FLOAT_COLOR,
						VB->ColorPtr[0],
						immd ? 4 : emitsize,
						count);

		vic_1 |= R300_INPUT_CNTL_COLOR;
	}

	if (inputs & _TNL_BIT_COLOR1) {
		CONFIGURE_AOS(i_color[1], AOS_FORMAT_FLOAT_COLOR,
						VB->SecondaryColorPtr[0],
						immd ? 4 : VB->SecondaryColorPtr[0]->size,
						count);
	}

#if 0
	if (inputs & _TNL_BIT_FOG) {
		CONFIGURE_AOS(	AOS_FORMAT_FLOAT,
						VB->FogCoordPtr,
						immd ? 4 : VB->FogCoordPtr->size,
						count);
	}
#endif

	r300->state.texture.tc_count = 0;
	for (i = 0; i < ctx->Const.MaxTextureUnits; i++) {
		if (inputs & (_TNL_BIT_TEX0 << i)) {
			CONFIGURE_AOS(i_tex[i], AOS_FORMAT_FLOAT,
							VB->TexCoordPtr[i],
							immd ? 4 : VB->TexCoordPtr[i]->size,
							count);

			vic_1 |= R300_INPUT_CNTL_TC0 << i;
			r300->state.texture.tc_count++;
		}
	}
	
#define SHOW_INFO(n) do { \
	if (RADEON_DEBUG & DEBUG_ALL) { \
	fprintf(stderr, "RR[%d] - sz=%d, reg=%d, fmt=%d -- st=%d, of=0x%08x\n", \
		n, \
		r300->state.aos[n].aos_size, \
		r300->state.aos[n].aos_reg, \
		r300->state.aos[n].aos_format, \
		r300->state.aos[n].aos_stride, \
		r300->state.aos[n].aos_offset); \
	} \
} while(0);

	/* setup INPUT_ROUTE */
	R300_STATECHANGE(r300, vir[0]);
	for(i=0;i+1<nr;i+=2){
		SHOW_INFO(i)
		SHOW_INFO(i+1)
		dw=(r300->state.aos[i].aos_size-1)
		| ((r300->state.aos[i].aos_reg)<<8)
		| (r300->state.aos[i].aos_format<<14)
		| (((r300->state.aos[i+1].aos_size-1)
		| ((r300->state.aos[i+1].aos_reg)<<8)
		| (r300->state.aos[i+1].aos_format<<14))<<16);

		if(i+2==nr){
			dw|=(1<<(13+16));
			}
		r300->hw.vir[0].cmd[R300_VIR_CNTL_0+(i>>1)]=dw;
		}
	if(nr & 1){
		SHOW_INFO(nr-1)
		dw=(r300->state.aos[nr-1].aos_size-1)
		| (r300->state.aos[nr-1].aos_format<<14)
		| ((r300->state.aos[nr-1].aos_reg)<<8)
		| (1<<13);
		r300->hw.vir[0].cmd[R300_VIR_CNTL_0+(nr>>1)]=dw;
		//fprintf(stderr, "vir0 dw=%08x\n", dw);
		}
	/* Set the rest of INPUT_ROUTE_0 to 0 */
	//for(i=((count+1)>>1); i<8; i++)r300->hw.vir[0].cmd[R300_VIR_CNTL_0+i]=(0x0);
	((drm_r300_cmd_header_t*)r300->hw.vir[0].cmd)->packet0.count = (nr+1)>>1;


	/* Mesa assumes that all missing components are from (0, 0, 0, 1) */
#define ALL_COMPONENTS ((R300_INPUT_ROUTE_SELECT_X<<R300_INPUT_ROUTE_X_SHIFT) \
		| (R300_INPUT_ROUTE_SELECT_Y<<R300_INPUT_ROUTE_Y_SHIFT) \
		| (R300_INPUT_ROUTE_SELECT_Z<<R300_INPUT_ROUTE_Z_SHIFT) \
		| (R300_INPUT_ROUTE_SELECT_W<<R300_INPUT_ROUTE_W_SHIFT))

#define ALL_DEFAULT ((R300_INPUT_ROUTE_SELECT_ZERO<<R300_INPUT_ROUTE_X_SHIFT) \
		| (R300_INPUT_ROUTE_SELECT_ZERO<<R300_INPUT_ROUTE_Y_SHIFT) \
		| (R300_INPUT_ROUTE_SELECT_ZERO<<R300_INPUT_ROUTE_Z_SHIFT) \
		| (R300_INPUT_ROUTE_SELECT_ONE<<R300_INPUT_ROUTE_W_SHIFT))

	R300_STATECHANGE(r300, vir[1]);

	for(i=0;i+1<nr;i+=2){
		/* do i first.. */
		mask=(1<<(r300->state.aos[i].aos_size*3))-1;
		dw=(ALL_COMPONENTS & mask)
		| (ALL_DEFAULT & ~mask)
		| R300_INPUT_ROUTE_ENABLE;

		/* i+1 */
		mask=(1<<(r300->state.aos[i+1].aos_size*3))-1;
		dw|=(
		(ALL_COMPONENTS & mask)
		| (ALL_DEFAULT & ~mask)
		| R300_INPUT_ROUTE_ENABLE
		)<<16;

		r300->hw.vir[1].cmd[R300_VIR_CNTL_0+(i>>1)]=dw;
		}
	if(nr & 1){
		mask=(1<<(r300->state.aos[nr-1].aos_size*3))-1;
		dw=(ALL_COMPONENTS & mask)
		| (ALL_DEFAULT & ~mask)
		| R300_INPUT_ROUTE_ENABLE;
		r300->hw.vir[1].cmd[R300_VIR_CNTL_0+(nr>>1)]=dw;
		//fprintf(stderr, "vir1 dw=%08x\n", dw);
		}
	/* Set the rest of INPUT_ROUTE_1 to 0 */
	//for(i=((count+1)>>1); i<8; i++)r300->hw.vir[1].cmd[R300_VIR_CNTL_0+i]=0x0;
	((drm_r300_cmd_header_t*)r300->hw.vir[1].cmd)->packet0.count = (nr+1)>>1;

	/* Set up input_cntl */
	/* I don't think this is needed for vertex buffers, but it doesn't hurt anything */
	R300_STATECHANGE(r300, vic);
	r300->hw.vic.cmd[R300_VIC_CNTL_0]=0x5555;  /* Hard coded value, no idea what it means */
	r300->hw.vic.cmd[R300_VIC_CNTL_1]=vic_1;

#if 0
	r300->hw.vic.cmd[R300_VIC_CNTL_1]=0;

	if(r300->state.render_inputs & _TNL_BIT_POS)
		r300->hw.vic.cmd[R300_VIC_CNTL_1]|=R300_INPUT_CNTL_POS;

	if(r300->state.render_inputs & _TNL_BIT_NORMAL)
		r300->hw.vic.cmd[R300_VIC_CNTL_1]|=R300_INPUT_CNTL_NORMAL;

	if(r300->state.render_inputs & _TNL_BIT_COLOR0)
		r300->hw.vic.cmd[R300_VIC_CNTL_1]|=R300_INPUT_CNTL_COLOR;

	for(i=0;i < ctx->Const.MaxTextureUnits;i++)
		if(r300->state.render_inputs & (_TNL_BIT_TEX0<<i))
			r300->hw.vic.cmd[R300_VIC_CNTL_1]|=(R300_INPUT_CNTL_TC0<<i);
#endif

	/* Stage 3: VAP output */
	
	R300_STATECHANGE(r300, vof);
	
	r300->hw.vof.cmd[R300_VOF_CNTL_0]=0;
	r300->hw.vof.cmd[R300_VOF_CNTL_1]=0;
	if (hw_tcl_on){
		GLuint OutputsWritten = CURRENT_VERTEX_SHADER(ctx)->OutputsWritten;
		
		if(OutputsWritten & (1<<VERT_RESULT_HPOS))
			r300->hw.vof.cmd[R300_VOF_CNTL_0] |= R300_VAP_OUTPUT_VTX_FMT_0__POS_PRESENT;
		if(OutputsWritten & (1<<VERT_RESULT_COL0))
			r300->hw.vof.cmd[R300_VOF_CNTL_0] |= R300_VAP_OUTPUT_VTX_FMT_0__COLOR_PRESENT;
		/*if(OutputsWritten & (1<<VERT_RESULT_COL1))
			r300->hw.vof.cmd[R300_VOF_CNTL_0] |= R300_VAP_OUTPUT_VTX_FMT_0__COLOR_1_PRESENT;
		if(OutputsWritten & (1<<VERT_RESULT_BFC0))
			r300->hw.vof.cmd[R300_VOF_CNTL_0] |= R300_VAP_OUTPUT_VTX_FMT_0__COLOR_2_PRESENT;
		if(OutputsWritten & (1<<VERT_RESULT_BFC1))
			r300->hw.vof.cmd[R300_VOF_CNTL_0] |= R300_VAP_OUTPUT_VTX_FMT_0__COLOR_3_PRESENT;*/
		//if(OutputsWritten & (1<<VERT_RESULT_FOGC))

		if(OutputsWritten & (1<<VERT_RESULT_PSIZ))
			r300->hw.vof.cmd[R300_VOF_CNTL_0] |= R300_VAP_OUTPUT_VTX_FMT_0__PT_SIZE_PRESENT;

		for(i=0;i < ctx->Const.MaxTextureUnits;i++)
			if(OutputsWritten & (1<<(VERT_RESULT_TEX0+i)))
				r300->hw.vof.cmd[R300_VOF_CNTL_1] |= (4<<(3*i));
	} else {
		if(inputs & _TNL_BIT_POS)
			r300->hw.vof.cmd[R300_VOF_CNTL_0] |= R300_VAP_OUTPUT_VTX_FMT_0__POS_PRESENT;
		if(inputs & _TNL_BIT_COLOR0)
			r300->hw.vof.cmd[R300_VOF_CNTL_0] |= R300_VAP_OUTPUT_VTX_FMT_0__COLOR_PRESENT;
		if(inputs & _TNL_BIT_COLOR1)
			r300->hw.vof.cmd[R300_VOF_CNTL_0] |= R300_VAP_OUTPUT_VTX_FMT_0__COLOR_1_PRESENT;

		for(i=0;i < ctx->Const.MaxTextureUnits;i++)
			if(inputs & (_TNL_BIT_TEX0<<i))
				r300->hw.vof.cmd[R300_VOF_CNTL_1]|=(4<<(3*i));
	}

	rmesa->state.aos_count = nr;
}

void r300ReleaseArrays(GLcontext * ctx)
{
	r300ContextPtr rmesa = R300_CONTEXT(ctx);
	int i;

	r300ReleaseDmaRegion(rmesa, &rmesa->state.elt_dma, __FUNCTION__);
	for (i=0;i<rmesa->state.aos_count;i++) {
		r300ReleaseDmaRegion(rmesa, &rmesa->state.aos[i], __FUNCTION__);
	}
}
