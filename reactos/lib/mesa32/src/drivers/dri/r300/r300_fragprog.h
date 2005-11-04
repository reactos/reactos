#ifndef __R300_FRAGPROG_H_
#define __R300_FRAGPROG_H_

#include "glheader.h"
#include "macros.h"
#include "enums.h"

#include "program.h"
#include "r300_context.h"
#include "nvfragprog.h"

/* representation of a register for emit_arith/swizzle */
typedef struct _pfs_reg_t {
	enum {
		REG_TYPE_INPUT,
		REG_TYPE_OUTPUT,
		REG_TYPE_TEMP,
		REG_TYPE_CONST
	} type:2;
	GLuint index:6;
	GLuint v_swz:5;
	GLuint s_swz:5;
	GLuint negate:1;    //XXX: we need to handle negate individually
	GLboolean valid:1;
} pfs_reg_t;

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
#define MAX_PFS_OP 11
#define OP(n) PFS_OP_##n

#define PFS_FLAG_SAT	(1 << 0)
#define PFS_FLAG_ABS	(1 << 1)

extern void translate_fragment_shader(struct r300_fragment_program *rp);

#endif /* __R300_FRAGPROG_H_ */

