#ifndef __PIXEL_SHADER_H__
#define __PIXEL_SHADER_H__

#include "r300_reg.h"


/* INSTR 0 */

#define PFS_OP_MAD	0
#define PFS_OP_DP3	1
#define PFS_OP_DP4	2
#define PFS_OP_MIN	4
#define PFS_OP_MAX	5
#define PFS_OP_CMP	8
#define PFS_OP_FRC	9
#define PFS_OP_OUTC_REPL_ALPHA	10

/* "or" these with arg0 value to negate or take absolute value of an argument */
#define PFS_ARG_NEG  (1<<5)
#define PFS_ARG_ABS  (1<<6)

#define MAKE_PFS_INSTR0(op, arg0, arg1, arg2, flags) \
	( ((op)<<23) \
	  | ((arg0)<<R300_FPI0_ARG0C_SHIFT) \
	  | ((arg1)<<R300_FPI0_ARG1C_SHIFT) \
	  | ((arg2)<<R300_FPI0_ARG2C_SHIFT) \
	  | (flags) \
	)

#define PFS_FLAG_X	1
#define PFS_FLAG_Y	2
#define PFS_FLAG_XY	3
#define PFS_FLAG_Z	4
#define PFS_FLAG_XZ	5
#define PFS_FLAG_YZ	6
#define PFS_FLAG_ALL	7
#define PFS_FLAG_NONE	0

#define EASY_PFS_INSTR0(op, arg0, arg1, arg2) \
	MAKE_PFS_INSTR0(PFS_OP_##op, \
		R300_FPI0_ARGC_##arg0, \
		R300_FPI0_ARGC_##arg1, \
		R300_FPI0_ARGC_##arg2, \
		0)

/* INSTR 1 */

#define PFS_FLAG_CONST (1<<5)

#define MAKE_PFS_INSTR1(dstc, src0, src1, src2, reg, output) \
	((src0) | ((src1) << R300_FPI1_SRC1C_SHIFT) \
	  | ((src2)<<R300_FPI1_SRC2C_SHIFT) \
	  | ((dstc) << R300_FPI1_DSTC_SHIFT) \
	  | ((reg) << 23) | ((output)<<26))

#define EASY_PFS_INSTR1(dstc, src0, src1, src2, reg, output) \
	MAKE_PFS_INSTR1(dstc, src0, src1, src2, PFS_FLAG_##reg, PFS_FLAG_##output)

/* INSTR 2 */

/* you can "or" PFS_ARG_NEG with these values to negate them */

#define MAKE_PFS_INSTR2(op, arg0, arg1, arg2, flags) \
	(((op) << 23) | \
	  ((arg0)<<R300_FPI2_ARG0A_SHIFT) | \
	  ((arg1)<<R300_FPI2_ARG1A_SHIFT) | \
	  ((arg2)<<R300_FPI2_ARG2A_SHIFT) | \
	  (flags))

#define EASY_PFS_INSTR2(op, arg0, arg1, arg2) \
	MAKE_PFS_INSTR2(R300_FPI2_OUTA_##op, \
		R300_FPI2_ARGA_##arg0, \
		R300_FPI2_ARGA_##arg1, \
		R300_FPI2_ARGA_##arg2, \
		0)


/* INSTR 3 */

#define PFS_FLAG_NONE	0
#define PFS_FLAG_REG	1
#define PFS_FLAG_OUTPUT	2
#define PFS_FLAG_BOTH	3

#define MAKE_PFS_INSTR3(dstc, src0, src1, src2, flags) \
	((src0) | ((src1) << R300_FPI1_SRC1C_SHIFT) \
	  | ((src2)<<R300_FPI1_SRC2C_SHIFT) \
	  | ((dstc) << R300_FPI1_DSTC_SHIFT) \
	  | ((flags) << 23))

#define EASY_PFS_INSTR3(dstc, src0, src1, src2, flag) \
	MAKE_PFS_INSTR3(dstc, src0, src1, src2, PFS_FLAG_##flag)

	/* What are 0's ORed with flags ? They are register numbers that
	   just happen to be 0 */
#define PFS_NOP	{ \
		EASY_PFS_INSTR0(MAD, SRC0C_XYZ, ONE, ZERO), \
		EASY_PFS_INSTR1(0, 0, 0 | PFS_FLAG_CONST, 0 | PFS_FLAG_CONST, NONE, ALL), \
		EASY_PFS_INSTR2(MAD, SRC0A, ONE, ZERO), \
		EASY_PFS_INSTR3(0, 0, 0 | PFS_FLAG_CONST, 0 | PFS_FLAG_CONST, OUTPUT) \
		} 
	
#endif
