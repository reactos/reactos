#ifndef __R300_VERTPROG_H_
#define __R300_VERTPROG_H_

#include "r300_reg.h"

typedef struct {
	GLuint op;
	GLuint src[3];
} VERTEX_SHADER_INSTRUCTION;

#define VSF_FLAG_X	1
#define VSF_FLAG_Y	2
#define VSF_FLAG_Z	4
#define VSF_FLAG_W	8
#define VSF_FLAG_XYZ	(VSF_FLAG_X | VSF_FLAG_Y | VSF_FLAG_Z)
#define VSF_FLAG_ALL  0xf
#define VSF_FLAG_NONE  0

#define VSF_OUT_CLASS_TMP	0
#define VSF_OUT_CLASS_ADDR	1
#define VSF_OUT_CLASS_RESULT	2

/* first DWORD of an instruction */

/* possible operations: 
    DOT, MUL, ADD, MAD, FRC, MAX, MIN, SGE, SLT, EXP, LOG, LIT, POW, RCP, RSQ, EX2,
    LG2, MAD_2 */

#define MAKE_VSF_OP(op, out_reg_index, out_reg_fields, class) \
   ((op)  \
  	| ((out_reg_index) << R300_VPI_OUT_REG_INDEX_SHIFT) 	\
 	 | ((out_reg_fields) << 20) 	\
  	| ( (class) << 8 ) )

#define EASY_VSF_OP(op, out_reg_index, out_reg_fields, class) \
	MAKE_VSF_OP(R300_VPI_OUT_OP_##op, out_reg_index, VSF_FLAG_##out_reg_fields, VSF_OUT_CLASS_##class) \

/* according to Nikolai, the subsequent 3 DWORDs are sources, use same define for each */

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
	( ((in_reg_index)<<R300_VPI_IN_REG_INDEX_SHIFT) \
	   | ((comp_x)<<R300_VPI_IN_X_SHIFT) \
	   | ((comp_y)<<R300_VPI_IN_Y_SHIFT) \
	   | ((comp_z)<<R300_VPI_IN_Z_SHIFT) \
	   | ((comp_w)<<R300_VPI_IN_W_SHIFT) \
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
