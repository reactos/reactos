#ifndef __VERTEX_SHADER_H__
#define __VERTEX_SHADER_H__

#include "r200_reg.h"

typedef struct {
   uint32_t op;
   uint32_t src0;
   uint32_t src1;
   uint32_t src2;
} VERTEX_SHADER_INSTRUCTION;

extern void r200InitShaderFuncs(struct dd_function_table *functions);
extern void r200SetupVertexProg( GLcontext *ctx );

#define VSF_FLAG_X	1
#define VSF_FLAG_Y	2
#define VSF_FLAG_Z	4
#define VSF_FLAG_W	8
#define VSF_FLAG_XYZ	(VSF_FLAG_X | VSF_FLAG_Y | VSF_FLAG_Z)
#define VSF_FLAG_ALL	0xf
#define VSF_FLAG_NONE	0

#define R200_VSF_MAX_INST	128
#define R200_VSF_MAX_PARAM	192
#define R200_VSF_MAX_TEMPS	12

#define R200_VPI_OUT_REG_INDEX_SHIFT            13
#define R200_VPI_OUT_REG_INDEX_MASK             (31 << 13) /* GUESS based on fglrx native limits */

#define R200_VPI_OUT_WRITE_X                    (1 << 20)
#define R200_VPI_OUT_WRITE_Y                    (1 << 21)
#define R200_VPI_OUT_WRITE_Z                    (1 << 22)
#define R200_VPI_OUT_WRITE_W                    (1 << 23)

#define R200_VPI_IN_REG_CLASS_TEMPORARY         (0 << 0)
#define R200_VPI_IN_REG_CLASS_ATTRIBUTE         (1 << 0)
#define R200_VPI_IN_REG_CLASS_PARAMETER         (2 << 0)
#define R200_VPI_IN_REG_CLASS_NONE              (9 << 0)
#define R200_VPI_IN_REG_CLASS_MASK              (31 << 0) /* GUESS */

#define R200_VPI_IN_REG_INDEX_SHIFT             5
#define R200_VPI_IN_REG_INDEX_MASK              (255 << 5) /* GUESS based on fglrx native limits */

/* The R200 can select components from the input register arbitrarily.
// Use the following constants, shifted by the component shift you
// want to select */
#define R200_VPI_IN_SELECT_X    0
#define R200_VPI_IN_SELECT_Y    1
#define R200_VPI_IN_SELECT_Z    2
#define R200_VPI_IN_SELECT_W    3
#define R200_VPI_IN_SELECT_ZERO 4
#define R200_VPI_IN_SELECT_ONE  5
#define R200_VPI_IN_SELECT_MASK 7

#define R200_VPI_IN_X_SHIFT                     13
#define R200_VPI_IN_Y_SHIFT                     16
#define R200_VPI_IN_Z_SHIFT                     19
#define R200_VPI_IN_W_SHIFT                     22

#define R200_VPI_IN_NEG_X                       (1 << 25)
#define R200_VPI_IN_NEG_Y                       (1 << 26)
#define R200_VPI_IN_NEG_Z                       (1 << 27)
#define R200_VPI_IN_NEG_W                       (1 << 28)

#define R200_VSF_OUT_CLASS_TMP			(0 << 8)
#define R200_VSF_OUT_CLASS_ADDR			(3 << 8)
#define R200_VSF_OUT_CLASS_RESULT_POS		(4 << 8)
#define R200_VSF_OUT_CLASS_RESULT_COLOR		(5 << 8)
#define R200_VSF_OUT_CLASS_RESULT_TEXC		(6 << 8)
#define R200_VSF_OUT_CLASS_RESULT_FOGC		(7 << 8)
#define R200_VSF_OUT_CLASS_RESULT_POINTSIZE	(8 << 8)
#define R200_VSF_OUT_CLASS_MASK			(31 << 8)

/* opcodes - they all are the same as on r300 it seems, however
   LIT and POW require different setup */
#define R200_VPI_OUT_OP_DOT                     (1 << 0)
#define R200_VPI_OUT_OP_MUL                     (2 << 0)
#define R200_VPI_OUT_OP_ADD                     (3 << 0)
#define R200_VPI_OUT_OP_MAD                     (4 << 0)
#define R200_VPI_OUT_OP_DST                     (5 << 0)
#define R200_VPI_OUT_OP_FRC                     (6 << 0)
#define R200_VPI_OUT_OP_MAX                     (7 << 0)
#define R200_VPI_OUT_OP_MIN                     (8 << 0)
#define R200_VPI_OUT_OP_SGE                     (9 << 0)
#define R200_VPI_OUT_OP_SLT                     (10 << 0)

#define R200_VPI_OUT_OP_ARL                     (13 << 0)

#define R200_VPI_OUT_OP_EXP                     (65 << 0)
#define R200_VPI_OUT_OP_LOG                     (66 << 0)
/* base e exp. Useful for fog. */
#define R200_VPI_OUT_OP_EXP_E                   (67 << 0)

#define R200_VPI_OUT_OP_LIT                     (68 << 0)
#define R200_VPI_OUT_OP_POW                     (69 << 0)
#define R200_VPI_OUT_OP_RCP                     (70 << 0)
#define R200_VPI_OUT_OP_RSQ                     (72 << 0)

#define R200_VPI_OUT_OP_EX2                     (75 << 0)
#define R200_VPI_OUT_OP_LG2                     (76 << 0)

#define R200_VPI_OUT_OP_MAD_2                   (128 << 0)

/* first CARD32 of an instruction */

/* possible operations: 
    DOT, MUL, ADD, MAD, FRC, MAX, MIN, SGE, SLT, EXP, LOG, LIT, POW, RCP, RSQ, EX2,
    LG2, MAD_2, ARL */

#define MAKE_VSF_OP(op, out_reg, out_reg_fields) \
   ((op) | (out_reg) | ((out_reg_fields) << 20) )

#define VSF_IN_CLASS_TMP	0
#define VSF_IN_CLASS_ATTR	1
#define VSF_IN_CLASS_PARAM	2
#define VSF_IN_CLASS_NONE	9

#define VSF_IN_COMPONENT_X	0
#define VSF_IN_COMPONENT_Y	1
#define VSF_IN_COMPONENT_Z	2
#define VSF_IN_COMPONENT_W	3
#define VSF_IN_COMPONENT_ZERO	4
#define VSF_IN_COMPONENT_ONE	5

#define MAKE_VSF_SOURCE(in_reg_index, comp_x, comp_y, comp_z, comp_w, class, negate) \
	( ((in_reg_index)<<R200_VPI_IN_REG_INDEX_SHIFT) \
	   | ((comp_x)<<R200_VPI_IN_X_SHIFT) \
	   | ((comp_y)<<R200_VPI_IN_Y_SHIFT) \
	   | ((comp_z)<<R200_VPI_IN_Z_SHIFT) \
	   | ((comp_w)<<R200_VPI_IN_W_SHIFT) \
	   | ((negate)<<25) | ((class)))

#define EASY_VSF_SOURCE(in_reg_index, comp_x, comp_y, comp_z, comp_w, class, negate) \
	MAKE_VSF_SOURCE(in_reg_index, \
		VSF_IN_COMPONENT_##comp_x, \
		VSF_IN_COMPONENT_##comp_y, \
		VSF_IN_COMPONENT_##comp_z, \
		VSF_IN_COMPONENT_##comp_w, \
		VSF_IN_CLASS_##class, VSF_FLAG_##negate)

/* special sources: */

/* (1.0,1.0,1.0,1.0) vector (ATTR, plain ) */
#define VSF_ATTR_UNITY(reg) 	EASY_VSF_SOURCE(reg, ONE, ONE, ONE, ONE, ATTR, NONE)
#define VSF_UNITY(reg) 	EASY_VSF_SOURCE(reg, ONE, ONE, ONE, ONE, NONE, NONE)

/* contents of unmodified register */
#define VSF_REG(reg) 	EASY_VSF_SOURCE(reg, X, Y, Z, W, ATTR, NONE)

/* contents of unmodified parameter */
#define VSF_PARAM(reg) 	EASY_VSF_SOURCE(reg, X, Y, Z, W, PARAM, NONE)

/* contents of unmodified temporary register */
#define VSF_TMP(reg) 	EASY_VSF_SOURCE(reg, X, Y, Z, W, TMP, NONE)

/* components of ATTR register */
#define VSF_ATTR_X(reg) EASY_VSF_SOURCE(reg, X, X, X, X, ATTR, NONE)
#define VSF_ATTR_Y(reg) EASY_VSF_SOURCE(reg, Y, Y, Y, Y, ATTR, NONE)
#define VSF_ATTR_Z(reg) EASY_VSF_SOURCE(reg, Z, Z, Z, Z, ATTR, NONE)
#define VSF_ATTR_W(reg) EASY_VSF_SOURCE(reg, W, W, W, W, ATTR, NONE)

#endif
