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

/**
 * \file
 *
 * \author Keith Whitwell <keith@tungstengraphics.com>
 */

#include "glheader.h"
#include "mtypes.h"
#include "colormac.h"
#include "imports.h"
#include "macros.h"
#include "image.h"

#include "swrast_setup/swrast_setup.h"
#include "math/m_translate.h"
#include "tnl/tnl.h"
#include "tnl/t_context.h"

#include "r300_context.h"
#include "radeon_ioctl.h"
#include "r300_state.h"
#include "r300_emit.h"
#include "r300_ioctl.h"

#ifdef USER_BUFFERS
#include "r300_mem.h"
#endif

#if SWIZZLE_X != R300_INPUT_ROUTE_SELECT_X || \
    SWIZZLE_Y != R300_INPUT_ROUTE_SELECT_Y || \
    SWIZZLE_Z != R300_INPUT_ROUTE_SELECT_Z || \
    SWIZZLE_W != R300_INPUT_ROUTE_SELECT_W || \
    SWIZZLE_ZERO != R300_INPUT_ROUTE_SELECT_ZERO || \
    SWIZZLE_ONE != R300_INPUT_ROUTE_SELECT_ONE
#error Cannot change these!
#endif

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

static void r300EmitVec4(GLcontext * ctx,
			 struct r300_dma_region *rvb,
			 GLvoid * data, int stride, int count)
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

static void r300EmitVec8(GLcontext * ctx,
			 struct r300_dma_region *rvb,
			 GLvoid * data, int stride, int count)
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

static void r300EmitVec12(GLcontext * ctx,
			  struct r300_dma_region *rvb,
			  GLvoid * data, int stride, int count)
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

static void r300EmitVec16(GLcontext * ctx,
			  struct r300_dma_region *rvb,
			  GLvoid * data, int stride, int count)
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

static void r300EmitVec(GLcontext * ctx,
			struct r300_dma_region *rvb,
			GLvoid * data, int size, int stride, int count)
{
	r300ContextPtr rmesa = R300_CONTEXT(ctx);

	if (RADEON_DEBUG & DEBUG_VERTS)
		fprintf(stderr, "%s count %d size %d stride %d\n",
			__FUNCTION__, count, size, stride);

	/* Gets triggered when playing with future_hw_tcl_on ... */
	//assert(!rvb->buf);

	if (stride == 0) {
		r300AllocDmaRegion(rmesa, rvb, size * 4, 4);
		count = 1;
		rvb->aos_offset = GET_START(rvb);
		rvb->aos_stride = 0;
	} else {
		r300AllocDmaRegion(rmesa, rvb, size * count * 4, 4);	/* alignment? */
		rvb->aos_offset = GET_START(rvb);
		rvb->aos_stride = size;
	}

	/* Emit the data
	 */
	switch (size) {
	case 1:
		r300EmitVec4(ctx, rvb, data, stride, count);
		break;
	case 2:
		r300EmitVec8(ctx, rvb, data, stride, count);
		break;
	case 3:
		r300EmitVec12(ctx, rvb, data, stride, count);
		break;
	case 4:
		r300EmitVec16(ctx, rvb, data, stride, count);
		break;
	default:
		assert(0);
		_mesa_exit(-1);
		break;
	}

}

static GLuint t_type(struct dt *dt)
{
	switch (dt->type) {
	case GL_UNSIGNED_BYTE:
		return AOS_FORMAT_UBYTE;
	case GL_SHORT:
		return AOS_FORMAT_USHORT;
	case GL_FLOAT:
		return AOS_FORMAT_FLOAT;
	default:
		assert(0);
		break;
	}

	return AOS_FORMAT_FLOAT;
}

static GLuint t_vir0_size(struct dt *dt)
{
	switch (dt->type) {
	case GL_UNSIGNED_BYTE:
		return 4;
	case GL_SHORT:
		return 7;
	case GL_FLOAT:
		return dt->size - 1;
	default:
		assert(0);
		break;
	}

	return 0;
}

static GLuint t_aos_size(struct dt *dt)
{
	switch (dt->type) {
	case GL_UNSIGNED_BYTE:
		return 1;
	case GL_SHORT:
		return 2;
	case GL_FLOAT:
		return dt->size;
	default:
		assert(0);
		break;
	}

	return 0;
}

static GLuint t_vir0(uint32_t * dst, struct dt *dt, int *inputs,
		     GLint * tab, GLuint nr)
{
	GLuint i, dw;

	for (i = 0; i + 1 < nr; i += 2) {
		dw = t_vir0_size(&dt[tab[i]]) | (inputs[tab[i]] << 8) |
		    (t_type(&dt[tab[i]]) << 14);
		dw |=
		    (t_vir0_size(&dt[tab[i + 1]]) |
		     (inputs[tab[i + 1]] << 8) | (t_type(&dt[tab[i + 1]])
						  << 14)) << 16;

		if (i + 2 == nr) {
			dw |= (1 << (13 + 16));
		}
		dst[i >> 1] = dw;
	}

	if (nr & 1) {
		dw = t_vir0_size(&dt[tab[nr - 1]]) | (inputs[tab[nr - 1]]
						      << 8) |
		    (t_type(&dt[tab[nr - 1]]) << 14);
		dw |= 1 << 13;

		dst[nr >> 1] = dw;
	}

	return (nr + 1) >> 1;
}

static GLuint t_swizzle(int swizzle[4])
{
	return (swizzle[0] << R300_INPUT_ROUTE_X_SHIFT) |
	    (swizzle[1] << R300_INPUT_ROUTE_Y_SHIFT) |
	    (swizzle[2] << R300_INPUT_ROUTE_Z_SHIFT) |
	    (swizzle[3] << R300_INPUT_ROUTE_W_SHIFT);
}

static GLuint t_vir1(uint32_t * dst, int swizzle[][4], GLuint nr)
{
	GLuint i;

	for (i = 0; i + 1 < nr; i += 2) {
		dst[i >> 1] = t_swizzle(swizzle[i]) | R300_INPUT_ROUTE_ENABLE;
		dst[i >> 1] |=
		    (t_swizzle(swizzle[i + 1]) | R300_INPUT_ROUTE_ENABLE)
		    << 16;
	}

	if (nr & 1)
		dst[nr >> 1] =
		    t_swizzle(swizzle[nr - 1]) | R300_INPUT_ROUTE_ENABLE;

	return (nr + 1) >> 1;
}

static GLuint t_emit_size(struct dt *dt)
{
	return dt->size;
}

static GLuint t_vic(GLcontext * ctx, GLuint InputsRead)
{
	r300ContextPtr r300 = R300_CONTEXT(ctx);
	GLuint i, vic_1 = 0;

	if (InputsRead & (1 << VERT_ATTRIB_POS))
		vic_1 |= R300_INPUT_CNTL_POS;

	if (InputsRead & (1 << VERT_ATTRIB_NORMAL))
		vic_1 |= R300_INPUT_CNTL_NORMAL;

	if (InputsRead & (1 << VERT_ATTRIB_COLOR0))
		vic_1 |= R300_INPUT_CNTL_COLOR;

	r300->state.texture.tc_count = 0;
	for (i = 0; i < ctx->Const.MaxTextureUnits; i++)
		if (InputsRead & (1 << (VERT_ATTRIB_TEX0 + i))) {
			r300->state.texture.tc_count++;
			vic_1 |= R300_INPUT_CNTL_TC0 << i;
		}

	return vic_1;
}

/* Emit vertex data to GART memory
 * Route inputs to the vertex processor
 * This function should never return R300_FALLBACK_TCL when using software tcl.
 */

int r300EmitArrays(GLcontext * ctx)
{
	r300ContextPtr rmesa = R300_CONTEXT(ctx);
	r300ContextPtr r300 = rmesa;
	struct radeon_vertex_buffer *VB = &rmesa->state.VB;
	GLuint nr;
	GLuint count = VB->Count;
	GLuint i;
	GLuint InputsRead = 0, OutputsWritten = 0;
	int *inputs = NULL;
	int vir_inputs[VERT_ATTRIB_MAX];
	GLint tab[VERT_ATTRIB_MAX];
	int swizzle[VERT_ATTRIB_MAX][4];

	if (hw_tcl_on) {
		struct r300_vertex_program *prog =
		    (struct r300_vertex_program *)
		    CURRENT_VERTEX_SHADER(ctx);
		inputs = prog->inputs;
		InputsRead = CURRENT_VERTEX_SHADER(ctx)->key.InputsRead;
		OutputsWritten = CURRENT_VERTEX_SHADER(ctx)->key.OutputsWritten;
	} else {
		DECLARE_RENDERINPUTS(inputs_bitset);
		inputs = r300->state.sw_tcl_inputs;

		RENDERINPUTS_COPY(inputs_bitset,
				  TNL_CONTEXT(ctx)->render_inputs_bitset);

		assert(RENDERINPUTS_TEST(inputs_bitset, _TNL_ATTRIB_POS));
		InputsRead |= 1 << VERT_ATTRIB_POS;
		OutputsWritten |= 1 << VERT_RESULT_HPOS;

		assert(RENDERINPUTS_TEST(inputs_bitset, _TNL_ATTRIB_NORMAL)
		       == 0);

		assert(RENDERINPUTS_TEST(inputs_bitset, _TNL_ATTRIB_COLOR0));
		InputsRead |= 1 << VERT_ATTRIB_COLOR0;
		OutputsWritten |= 1 << VERT_RESULT_COL0;

		if (RENDERINPUTS_TEST(inputs_bitset, _TNL_ATTRIB_COLOR1)) {
			InputsRead |= 1 << VERT_ATTRIB_COLOR1;
			OutputsWritten |= 1 << VERT_RESULT_COL1;
		}

		for (i = 0; i < ctx->Const.MaxTextureUnits; i++)
			if (RENDERINPUTS_TEST
			    (inputs_bitset, _TNL_ATTRIB_TEX(i))) {
				InputsRead |= 1 << (VERT_ATTRIB_TEX0 + i);
				OutputsWritten |= 1 << (VERT_RESULT_TEX0 + i);
			}

		for (i = 0, nr = 0; i < VERT_ATTRIB_MAX; i++)
			if (InputsRead & (1 << i))
				inputs[i] = nr++;
			else
				inputs[i] = -1;

		if (!
		    (r300->radeon.radeonScreen->
		     chip_flags & RADEON_CHIPSET_TCL)) {
			/* Fixed, apply to vir0 only */
			memcpy(vir_inputs, inputs,
			       VERT_ATTRIB_MAX * sizeof(int));
			inputs = vir_inputs;

			if (InputsRead & VERT_ATTRIB_POS)
				inputs[VERT_ATTRIB_POS] = 0;

			if (InputsRead & (1 << VERT_ATTRIB_COLOR0))
				inputs[VERT_ATTRIB_COLOR0] = 2;

			if (InputsRead & (1 << VERT_ATTRIB_COLOR1))
				inputs[VERT_ATTRIB_COLOR1] = 3;

			for (i = VERT_ATTRIB_TEX0; i <= VERT_ATTRIB_TEX7; i++)
				if (InputsRead & (1 << i))
					inputs[i] = 6 + (i - VERT_ATTRIB_TEX0);
		}

		RENDERINPUTS_COPY(rmesa->state.render_inputs_bitset,
				  inputs_bitset);
	}
	assert(InputsRead);
	assert(OutputsWritten);

	for (i = 0, nr = 0; i < VERT_ATTRIB_MAX; i++)
		if (InputsRead & (1 << i))
			tab[nr++] = i;

	if (nr > R300_MAX_AOS_ARRAYS)
		return R300_FALLBACK_TCL;

	for (i = 0; i < nr; i++) {
		int ci;
		int comp_size, fix, found = 0;

		swizzle[i][0] = SWIZZLE_ZERO;
		swizzle[i][1] = SWIZZLE_ZERO;
		swizzle[i][2] = SWIZZLE_ZERO;
		swizzle[i][3] = SWIZZLE_ONE;

		for (ci = 0; ci < VB->AttribPtr[tab[i]].size; ci++)
			swizzle[i][ci] = ci;

#if MESA_BIG_ENDIAN
#define SWAP_INT(a, b) do { \
	int __temp; \
	__temp = a;\
	a = b; \
	b = __temp; \
} while (0)

		if (VB->AttribPtr[tab[i]].type == GL_UNSIGNED_BYTE) {
			SWAP_INT(swizzle[i][0], swizzle[i][3]);
			SWAP_INT(swizzle[i][1], swizzle[i][2]);
		}
#endif				/* MESA_BIG_ENDIAN */

		if (r300IsGartMemory(rmesa, VB->AttribPtr[tab[i]].data,
				     /*(count-1)*stride */ 4)) {
			if (VB->AttribPtr[tab[i]].stride % 4)
				return R300_FALLBACK_TCL;

			rmesa->state.aos[i].address =
			    VB->AttribPtr[tab[i]].data;
			rmesa->state.aos[i].start = 0;
			rmesa->state.aos[i].aos_offset =
			    r300GartOffsetFromVirtual(rmesa,
						      VB->
						      AttribPtr[tab[i]].data);
			rmesa->state.aos[i].aos_stride =
			    VB->AttribPtr[tab[i]].stride / 4;

			rmesa->state.aos[i].aos_size =
			    t_emit_size(&VB->AttribPtr[tab[i]]);
		} else {
			/* TODO: r300EmitVec can only handle 4 byte vectors */
			if (VB->AttribPtr[tab[i]].type != GL_FLOAT)
				return R300_FALLBACK_TCL;

			r300EmitVec(ctx, &rmesa->state.aos[i],
				    VB->AttribPtr[tab[i]].data,
				    t_emit_size(&VB->AttribPtr[tab[i]]),
				    VB->AttribPtr[tab[i]].stride, count);
		}

		rmesa->state.aos[i].aos_size =
		    t_aos_size(&VB->AttribPtr[tab[i]]);

		comp_size = _mesa_sizeof_type(VB->AttribPtr[tab[i]].type);

		for (fix = 0; fix <= 4 - VB->AttribPtr[tab[i]].size; fix++) {
			if ((rmesa->state.aos[i].aos_offset -
			     comp_size * fix) % 4)
				continue;

			found = 1;
			break;
		}

		if (found) {
			if (fix > 0) {
				WARN_ONCE("Feeling lucky?\n");
			}

			rmesa->state.aos[i].aos_offset -= comp_size * fix;

			for (ci = 0; ci < VB->AttribPtr[tab[i]].size; ci++)
				swizzle[i][ci] += fix;
		} else {
			WARN_ONCE
			    ("Cannot handle offset %x with stride %d, comp %d\n",
			     rmesa->state.aos[i].aos_offset,
			     rmesa->state.aos[i].aos_stride,
			     VB->AttribPtr[tab[i]].size);
			return R300_FALLBACK_TCL;
		}
	}

	/* setup INPUT_ROUTE */
	R300_STATECHANGE(r300, vir[0]);
	((drm_r300_cmd_header_t *) r300->hw.vir[0].cmd)->packet0.count =
	    t_vir0(&r300->hw.vir[0].cmd[R300_VIR_CNTL_0], VB->AttribPtr,
		   inputs, tab, nr);

	R300_STATECHANGE(r300, vir[1]);
	((drm_r300_cmd_header_t *) r300->hw.vir[1].cmd)->packet0.count =
	    t_vir1(&r300->hw.vir[1].cmd[R300_VIR_CNTL_0], swizzle, nr);

	/* Set up input_cntl */
	/* I don't think this is needed for vertex buffers, but it doesn't hurt anything */
	R300_STATECHANGE(r300, vic);
	r300->hw.vic.cmd[R300_VIC_CNTL_0] = 0x5555;	/* Hard coded value, no idea what it means */
	r300->hw.vic.cmd[R300_VIC_CNTL_1] = t_vic(ctx, InputsRead);

	/* Stage 3: VAP output */

	R300_STATECHANGE(r300, vof);

	r300->hw.vof.cmd[R300_VOF_CNTL_0] = 0;
	r300->hw.vof.cmd[R300_VOF_CNTL_1] = 0;

	if (OutputsWritten & (1 << VERT_RESULT_HPOS))
		r300->hw.vof.cmd[R300_VOF_CNTL_0] |=
		    R300_VAP_OUTPUT_VTX_FMT_0__POS_PRESENT;

	if (OutputsWritten & (1 << VERT_RESULT_COL0))
		r300->hw.vof.cmd[R300_VOF_CNTL_0] |=
		    R300_VAP_OUTPUT_VTX_FMT_0__COLOR_PRESENT;

	if (OutputsWritten & (1 << VERT_RESULT_COL1))
		r300->hw.vof.cmd[R300_VOF_CNTL_0] |=
		    R300_VAP_OUTPUT_VTX_FMT_0__COLOR_1_PRESENT;

	/*if(OutputsWritten & (1 << VERT_RESULT_BFC0))
	   r300->hw.vof.cmd[R300_VOF_CNTL_0] |= R300_VAP_OUTPUT_VTX_FMT_0__COLOR_2_PRESENT;

	   if(OutputsWritten & (1 << VERT_RESULT_BFC1))
	   r300->hw.vof.cmd[R300_VOF_CNTL_0] |= R300_VAP_OUTPUT_VTX_FMT_0__COLOR_3_PRESENT; */
	//if(OutputsWritten & (1 << VERT_RESULT_FOGC))

	if (OutputsWritten & (1 << VERT_RESULT_PSIZ))
		r300->hw.vof.cmd[R300_VOF_CNTL_0] |=
		    R300_VAP_OUTPUT_VTX_FMT_0__PT_SIZE_PRESENT;

	for (i = 0; i < ctx->Const.MaxTextureUnits; i++)
		if (OutputsWritten & (1 << (VERT_RESULT_TEX0 + i)))
			r300->hw.vof.cmd[R300_VOF_CNTL_1] |= (4 << (3 * i));

	rmesa->state.aos_count = nr;

	return R300_FALLBACK_NONE;
}

#ifdef USER_BUFFERS
void r300UseArrays(GLcontext * ctx)
{
	r300ContextPtr rmesa = R300_CONTEXT(ctx);
	int i;

	if (rmesa->state.elt_dma.buf)
		r300_mem_use(rmesa, rmesa->state.elt_dma.buf->id);

	for (i = 0; i < rmesa->state.aos_count; i++) {
		if (rmesa->state.aos[i].buf)
			r300_mem_use(rmesa, rmesa->state.aos[i].buf->id);
	}
}
#endif

void r300ReleaseArrays(GLcontext * ctx)
{
	r300ContextPtr rmesa = R300_CONTEXT(ctx);
	int i;

	r300ReleaseDmaRegion(rmesa, &rmesa->state.elt_dma, __FUNCTION__);
	for (i = 0; i < rmesa->state.aos_count; i++) {
		r300ReleaseDmaRegion(rmesa, &rmesa->state.aos[i], __FUNCTION__);
	}
}
