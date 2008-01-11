/*
 * Copyright (C) 2005 Ben Skeggs.
 *
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial
 * portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE COPYRIGHT OWNER(S) AND/OR ITS SUPPLIERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

/*
 * Authors:
 *   Ben Skeggs <darktama@iinet.net.au>
 *   Jerome Glisse <j.glisse@gmail.com>
 */
#ifndef __R300_FRAGPROG_H_
#define __R300_FRAGPROG_H_

#include "glheader.h"
#include "macros.h"
#include "enums.h"
#include "shader/program.h"
#include "shader/prog_instruction.h"

#include "r300_context.h"

typedef struct r300_fragment_program_swizzle {
	GLuint length;
	GLuint src[4];
	GLuint inst[8];
} r300_fragment_program_swizzle_t;

/* supported hw opcodes */
#define PFS_OP_MAD 0
#define PFS_OP_DP3 1
#define PFS_OP_DP4 2
#define PFS_OP_MIN 3
#define PFS_OP_MAX 4
#define PFS_OP_CMP 5
#define PFS_OP_FRC 6
#define PFS_OP_EX2 7
#define PFS_OP_LG2 8
#define PFS_OP_RCP 9
#define PFS_OP_RSQ 10
#define PFS_OP_REPL_ALPHA 11
#define PFS_OP_CMPH 12
#define MAX_PFS_OP 12

#define PFS_FLAG_SAT	(1 << 0)
#define PFS_FLAG_ABS	(1 << 1)

#define ARG_NEG			(1 << 5)
#define ARG_ABS			(1 << 6)
#define ARG_MASK		(127 << 0)
#define ARG_STRIDE		7
#define SRC_CONST		(1 << 5)
#define SRC_MASK		(63 << 0)
#define SRC_STRIDE		6

#define NOP_INST0 (						 \
		(R300_FPI0_OUTC_MAD) |				 \
		(R300_FPI0_ARGC_ZERO << R300_FPI0_ARG0C_SHIFT) | \
		(R300_FPI0_ARGC_ZERO << R300_FPI0_ARG1C_SHIFT) | \
		(R300_FPI0_ARGC_ZERO << R300_FPI0_ARG2C_SHIFT))
#define NOP_INST1 (					     \
		((0 | SRC_CONST) << R300_FPI1_SRC0C_SHIFT) | \
		((0 | SRC_CONST) << R300_FPI1_SRC1C_SHIFT) | \
		((0 | SRC_CONST) << R300_FPI1_SRC2C_SHIFT))
#define NOP_INST2 ( \
		(R300_FPI2_OUTA_MAD) |				 \
		(R300_FPI2_ARGA_ZERO << R300_FPI2_ARG0A_SHIFT) | \
		(R300_FPI2_ARGA_ZERO << R300_FPI2_ARG1A_SHIFT) | \
		(R300_FPI2_ARGA_ZERO << R300_FPI2_ARG2A_SHIFT))
#define NOP_INST3 (					     \
		((0 | SRC_CONST) << R300_FPI3_SRC0A_SHIFT) | \
		((0 | SRC_CONST) << R300_FPI3_SRC1A_SHIFT) | \
		((0 | SRC_CONST) << R300_FPI3_SRC2A_SHIFT))

#define DRI_CONF_FP_OPTIMIZATION_SPEED   0
#define DRI_CONF_FP_OPTIMIZATION_QUALITY 1

struct r300_fragment_program;

extern void r300TranslateFragmentShader(r300ContextPtr r300,
					struct r300_fragment_program *fp);

#endif
