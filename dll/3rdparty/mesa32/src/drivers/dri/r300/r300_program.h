/*
Copyright (C) 2004 Nicolai Haehnle.  All Rights Reserved.

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
 *   Nicolai Haehnle <prefect_@gmx.net>
 */

#ifndef __R300_PROGRAM_H__
#define __R300_PROGRAM_H__

#include "r300_reg.h"

/**
 * Vertex program helper macros
 */

/* Produce out dword */
#define VP_OUTCLASS_TMP		R300_VPI_OUT_REG_CLASS_TEMPORARY
#define VP_OUTCLASS_OUT		R300_VPI_OUT_REG_CLASS_RESULT

#define VP_OUTMASK_X	R300_VPI_OUT_WRITE_X
#define VP_OUTMASK_Y	R300_VPI_OUT_WRITE_Y
#define VP_OUTMASK_Z	R300_VPI_OUT_WRITE_Z
#define VP_OUTMASK_W	R300_VPI_OUT_WRITE_W
#define VP_OUTMASK_XY	(VP_OUTMASK_X|VP_OUTMASK_Y)
#define VP_OUTMASK_XZ	(VP_OUTMASK_X|VP_OUTMASK_Z)
#define VP_OUTMASK_XW	(VP_OUTMASK_X|VP_OUTMASK_W)
#define VP_OUTMASK_XYZ	(VP_OUTMASK_XY|VP_OUTMASK_Z)
#define VP_OUTMASK_XYW	(VP_OUTMASK_XY|VP_OUTMASK_W)
#define VP_OUTMASK_XZW	(VP_OUTMASK_XZ|VP_OUTMASK_W)
#define VP_OUTMASK_XYZW	(VP_OUTMASK_XYZ|VP_OUTMASK_W)
#define VP_OUTMASK_YZ	(VP_OUTMASK_Y|VP_OUTMASK_Z)
#define VP_OUTMASK_YW	(VP_OUTMASK_Y|VP_OUTMASK_W)
#define VP_OUTMASK_YZW	(VP_OUTMASK_YZ|VP_OUTMASK_W)
#define VP_OUTMASK_ZW	(VP_OUTMASK_Z|VP_OUTMASK_W)

#define VP_OUT(instr,outclass,outidx,outmask) \
	(R300_VPI_OUT_OP_##instr |				\
	((outidx) << R300_VPI_OUT_REG_INDEX_SHIFT) |		\
	VP_OUTCLASS_##outclass |				\
	VP_OUTMASK_##outmask)

/* Produce in dword */
#define VP_INCLASS_TMP		R300_VPI_IN_REG_CLASS_TEMPORARY
#define VP_INCLASS_IN		R300_VPI_IN_REG_CLASS_ATTRIBUTE
#define VP_INCLASS_CONST	R300_VPI_IN_REG_CLASS_PARAMETER

#define VP_IN(class,idx) \
	(((idx) << R300_VPI_IN_REG_INDEX_SHIFT) |		\
	VP_INCLASS_##class |					\
	(R300_VPI_IN_SELECT_X << R300_VPI_IN_X_SHIFT) |		\
	(R300_VPI_IN_SELECT_Y << R300_VPI_IN_Y_SHIFT) |		\
	(R300_VPI_IN_SELECT_Z << R300_VPI_IN_Z_SHIFT) |		\
	(R300_VPI_IN_SELECT_W << R300_VPI_IN_W_SHIFT))
#define VP_ZERO() \
	((R300_VPI_IN_SELECT_ZERO << R300_VPI_IN_X_SHIFT) |	\
	(R300_VPI_IN_SELECT_ZERO << R300_VPI_IN_Y_SHIFT) |	\
	(R300_VPI_IN_SELECT_ZERO << R300_VPI_IN_Z_SHIFT) |	\
	(R300_VPI_IN_SELECT_ZERO << R300_VPI_IN_W_SHIFT))
#define VP_ONE() \
	((R300_VPI_IN_SELECT_ONE << R300_VPI_IN_X_SHIFT) |	\
	(R300_VPI_IN_SELECT_ONE << R300_VPI_IN_Y_SHIFT) |	\
	(R300_VPI_IN_SELECT_ONE << R300_VPI_IN_Z_SHIFT) |	\
	(R300_VPI_IN_SELECT_ONE << R300_VPI_IN_W_SHIFT))

#define VP_NEG(in,comp)		((in) ^ (R300_VPI_IN_NEG_##comp))
#define VP_NEGALL(in,comp)	VP_NEG(VP_NEG(VP_NEG(VP_NEG((in),X),Y),Z),W)

/**
 * Fragment program helper macros
 */

/* Produce unshifted source selectors */
#define FP_TMP(idx) (idx)
#define FP_CONST(idx) ((idx) | (1 << 5))

/* Produce source/dest selector dword */
#define FP_SELC_MASK_NO		0
#define FP_SELC_MASK_X		1
#define FP_SELC_MASK_Y		2
#define FP_SELC_MASK_XY		3
#define FP_SELC_MASK_Z		4
#define FP_SELC_MASK_XZ		5
#define FP_SELC_MASK_YZ		6
#define FP_SELC_MASK_XYZ	7

#define FP_SELC(destidx,regmask,outmask,src0,src1,src2) \
	(((destidx) << R300_FPI1_DSTC_SHIFT) |		\
	 (FP_SELC_MASK_##regmask << 23) |		\
	 (FP_SELC_MASK_##outmask << 26) |		\
	 ((src0) << R300_FPI1_SRC0C_SHIFT) |		\
	 ((src1) << R300_FPI1_SRC1C_SHIFT) |		\
	 ((src2) << R300_FPI1_SRC2C_SHIFT))

#define FP_SELA_MASK_NO		0
#define FP_SELA_MASK_W		1

#define FP_SELA(destidx,regmask,outmask,src0,src1,src2) \
	(((destidx) << R300_FPI3_DSTA_SHIFT) |		\
	 (FP_SELA_MASK_##regmask << 23) |		\
	 (FP_SELA_MASK_##outmask << 24) |		\
	 ((src0) << R300_FPI3_SRC0A_SHIFT) |		\
	 ((src1) << R300_FPI3_SRC1A_SHIFT) |		\
	 ((src2) << R300_FPI3_SRC2A_SHIFT))

/* Produce unshifted argument selectors */
#define FP_ARGC(source)	R300_FPI0_ARGC_##source
#define FP_ARGA(source) R300_FPI2_ARGA_##source
#define FP_ABS(arg) ((arg) | (1 << 6))
#define FP_NEG(arg) ((arg) ^ (1 << 5))

/* Produce instruction dword */
#define FP_INSTRC(opcode,arg0,arg1,arg2) \
	(R300_FPI0_OUTC_##opcode | 		\
	((arg0) << R300_FPI0_ARG0C_SHIFT) |	\
	((arg1) << R300_FPI0_ARG1C_SHIFT) |	\
	((arg2) << R300_FPI0_ARG2C_SHIFT))

#define FP_INSTRA(opcode,arg0,arg1,arg2) \
	(R300_FPI2_OUTA_##opcode | 		\
	((arg0) << R300_FPI2_ARG0A_SHIFT) |	\
	((arg1) << R300_FPI2_ARG1A_SHIFT) |	\
	((arg2) << R300_FPI2_ARG2A_SHIFT))

extern void debug_vp(GLcontext * ctx, struct gl_vertex_program *vp);

#endif				/* __R300_PROGRAM_H__ */
