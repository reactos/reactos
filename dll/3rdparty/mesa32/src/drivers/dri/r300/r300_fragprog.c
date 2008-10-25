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

/**
 * \file
 *
 * \author Ben Skeggs <darktama@iinet.net.au>
 *
 * \author Jerome Glisse <j.glisse@gmail.com>
 *
 * \todo Depth write, WPOS/FOGC inputs
 *
 * \todo FogOption
 *
 * \todo Verify results of opcodes for accuracy, I've only checked them in
 * specific cases.
 */

#include "glheader.h"
#include "macros.h"
#include "enums.h"
#include "shader/prog_instruction.h"
#include "shader/prog_parameter.h"
#include "shader/prog_print.h"

#include "r300_context.h"
#include "r300_fragprog.h"
#include "r300_reg.h"
#include "r300_state.h"

/*
 * Usefull macros and values
 */
#define ERROR(fmt, args...) do {			\
		fprintf(stderr, "%s::%s(): " fmt "\n",	\
			__FILE__, __FUNCTION__, ##args);	\
		fp->error = GL_TRUE;			\
	} while(0)

#define PFS_INVAL 0xFFFFFFFF
#define COMPILE_STATE struct r300_pfs_compile_state *cs = fp->cs

#define SWIZZLE_XYZ		0
#define SWIZZLE_XXX		1
#define SWIZZLE_YYY		2
#define SWIZZLE_ZZZ		3
#define SWIZZLE_WWW		4
#define SWIZZLE_YZX		5
#define SWIZZLE_ZXY		6
#define SWIZZLE_WZY		7
#define SWIZZLE_111		8
#define SWIZZLE_000		9
#define SWIZZLE_HHH		10

#define swizzle(r, x, y, z, w) do_swizzle(fp, r,		\
					  ((SWIZZLE_##x<<0)|	\
					   (SWIZZLE_##y<<3)|	\
					   (SWIZZLE_##z<<6)|	\
					   (SWIZZLE_##w<<9)),	\
					  0)

#define REG_TYPE_INPUT		0
#define REG_TYPE_OUTPUT		1
#define REG_TYPE_TEMP		2
#define REG_TYPE_CONST		3

#define REG_TYPE_SHIFT		0
#define REG_INDEX_SHIFT		2
#define REG_VSWZ_SHIFT		8
#define REG_SSWZ_SHIFT		13
#define REG_NEGV_SHIFT		18
#define REG_NEGS_SHIFT		19
#define REG_ABS_SHIFT		20
#define REG_NO_USE_SHIFT	21	// Hack for refcounting
#define REG_VALID_SHIFT		22	// Does the register contain a defined value?
#define REG_BUILTIN_SHIFT   23	// Is it a builtin (like all zero/all one)?

#define REG_TYPE_MASK		(0x03 << REG_TYPE_SHIFT)
#define REG_INDEX_MASK		(0x3F << REG_INDEX_SHIFT)
#define REG_VSWZ_MASK		(0x1F << REG_VSWZ_SHIFT)
#define REG_SSWZ_MASK		(0x1F << REG_SSWZ_SHIFT)
#define REG_NEGV_MASK		(0x01 << REG_NEGV_SHIFT)
#define REG_NEGS_MASK		(0x01 << REG_NEGS_SHIFT)
#define REG_ABS_MASK		(0x01 << REG_ABS_SHIFT)
#define REG_NO_USE_MASK		(0x01 << REG_NO_USE_SHIFT)
#define REG_VALID_MASK		(0x01 << REG_VALID_SHIFT)
#define REG_BUILTIN_MASK	(0x01 << REG_BUILTIN_SHIFT)

#define REG(type, index, vswz, sswz, nouse, valid, builtin)	\
	(((type << REG_TYPE_SHIFT) & REG_TYPE_MASK) |			\
	 ((index << REG_INDEX_SHIFT) & REG_INDEX_MASK) |		\
	 ((nouse << REG_NO_USE_SHIFT) & REG_NO_USE_MASK) |		\
	 ((valid << REG_VALID_SHIFT) & REG_VALID_MASK) |		\
	 ((builtin << REG_BUILTIN_SHIFT) & REG_BUILTIN_MASK) |	\
	 ((vswz << REG_VSWZ_SHIFT) & REG_VSWZ_MASK) |			\
	 ((sswz << REG_SSWZ_SHIFT) & REG_SSWZ_MASK))
#define REG_GET_TYPE(reg)						\
	((reg & REG_TYPE_MASK) >> REG_TYPE_SHIFT)
#define REG_GET_INDEX(reg)						\
	((reg & REG_INDEX_MASK) >> REG_INDEX_SHIFT)
#define REG_GET_VSWZ(reg)						\
	((reg & REG_VSWZ_MASK) >> REG_VSWZ_SHIFT)
#define REG_GET_SSWZ(reg)						\
	((reg & REG_SSWZ_MASK) >> REG_SSWZ_SHIFT)
#define REG_GET_NO_USE(reg)						\
	((reg & REG_NO_USE_MASK) >> REG_NO_USE_SHIFT)
#define REG_GET_VALID(reg)						\
	((reg & REG_VALID_MASK) >> REG_VALID_SHIFT)
#define REG_GET_BUILTIN(reg)						\
	((reg & REG_BUILTIN_MASK) >> REG_BUILTIN_SHIFT)
#define REG_SET_TYPE(reg, type)						\
	reg = ((reg & ~REG_TYPE_MASK) |					\
	       ((type << REG_TYPE_SHIFT) & REG_TYPE_MASK))
#define REG_SET_INDEX(reg, index)					\
	reg = ((reg & ~REG_INDEX_MASK) |				\
	       ((index << REG_INDEX_SHIFT) & REG_INDEX_MASK))
#define REG_SET_VSWZ(reg, vswz)						\
	reg = ((reg & ~REG_VSWZ_MASK) |					\
	       ((vswz << REG_VSWZ_SHIFT) & REG_VSWZ_MASK))
#define REG_SET_SSWZ(reg, sswz)						\
	reg = ((reg & ~REG_SSWZ_MASK) |					\
	       ((sswz << REG_SSWZ_SHIFT) & REG_SSWZ_MASK))
#define REG_SET_NO_USE(reg, nouse)					\
	reg = ((reg & ~REG_NO_USE_MASK) |				\
	       ((nouse << REG_NO_USE_SHIFT) & REG_NO_USE_MASK))
#define REG_SET_VALID(reg, valid)					\
	reg = ((reg & ~REG_VALID_MASK) |				\
	       ((valid << REG_VALID_SHIFT) & REG_VALID_MASK))
#define REG_SET_BUILTIN(reg, builtin)					\
	reg = ((reg & ~REG_BUILTIN_MASK) |				\
	       ((builtin << REG_BUILTIN_SHIFT) & REG_BUILTIN_MASK))
#define REG_ABS(reg)							\
	reg = (reg | REG_ABS_MASK)
#define REG_NEGV(reg)							\
	reg = (reg | REG_NEGV_MASK)
#define REG_NEGS(reg)							\
	reg = (reg | REG_NEGS_MASK)

/*
 * Datas structures for fragment program generation
 */

/* description of r300 native hw instructions */
static const struct {
	const char *name;
	int argc;
	int v_op;
	int s_op;
} r300_fpop[] = {
	/* *INDENT-OFF* */
	{"MAD", 3, R300_FPI0_OUTC_MAD, R300_FPI2_OUTA_MAD},
	{"DP3", 2, R300_FPI0_OUTC_DP3, R300_FPI2_OUTA_DP4},
	{"DP4", 2, R300_FPI0_OUTC_DP4, R300_FPI2_OUTA_DP4},
	{"MIN", 2, R300_FPI0_OUTC_MIN, R300_FPI2_OUTA_MIN},
	{"MAX", 2, R300_FPI0_OUTC_MAX, R300_FPI2_OUTA_MAX},
	{"CMP", 3, R300_FPI0_OUTC_CMP, R300_FPI2_OUTA_CMP},
	{"FRC", 1, R300_FPI0_OUTC_FRC, R300_FPI2_OUTA_FRC},
	{"EX2", 1, R300_FPI0_OUTC_REPL_ALPHA, R300_FPI2_OUTA_EX2},
	{"LG2", 1, R300_FPI0_OUTC_REPL_ALPHA, R300_FPI2_OUTA_LG2},
	{"RCP", 1, R300_FPI0_OUTC_REPL_ALPHA, R300_FPI2_OUTA_RCP},
	{"RSQ", 1, R300_FPI0_OUTC_REPL_ALPHA, R300_FPI2_OUTA_RSQ},
	{"REPL_ALPHA", 1, R300_FPI0_OUTC_REPL_ALPHA, PFS_INVAL},
	{"CMPH", 3, R300_FPI0_OUTC_CMPH, PFS_INVAL},
	/* *INDENT-ON* */
};

/* vector swizzles r300 can support natively, with a couple of
 * cases we handle specially
 *
 * REG_VSWZ/REG_SSWZ is an index into this table
 */

/* mapping from SWIZZLE_* to r300 native values for scalar insns */
#define SWIZZLE_HALF 6

#define MAKE_SWZ3(x, y, z) (MAKE_SWIZZLE4(SWIZZLE_##x, \
					  SWIZZLE_##y, \
					  SWIZZLE_##z, \
					  SWIZZLE_ZERO))
/* native swizzles */
static const struct r300_pfs_swizzle {
	GLuint hash;		/* swizzle value this matches */
	GLuint base;		/* base value for hw swizzle */
	GLuint stride;		/* difference in base between arg0/1/2 */
	GLuint flags;
} v_swiz[] = {
	/* *INDENT-OFF* */
	{MAKE_SWZ3(X, Y, Z), R300_FPI0_ARGC_SRC0C_XYZ, 4, SLOT_SRC_VECTOR},
	{MAKE_SWZ3(X, X, X), R300_FPI0_ARGC_SRC0C_XXX, 4, SLOT_SRC_VECTOR},
	{MAKE_SWZ3(Y, Y, Y), R300_FPI0_ARGC_SRC0C_YYY, 4, SLOT_SRC_VECTOR},
	{MAKE_SWZ3(Z, Z, Z), R300_FPI0_ARGC_SRC0C_ZZZ, 4, SLOT_SRC_VECTOR},
	{MAKE_SWZ3(W, W, W), R300_FPI0_ARGC_SRC0A, 1, SLOT_SRC_SCALAR},
	{MAKE_SWZ3(Y, Z, X), R300_FPI0_ARGC_SRC0C_YZX, 1, SLOT_SRC_VECTOR},
	{MAKE_SWZ3(Z, X, Y), R300_FPI0_ARGC_SRC0C_ZXY, 1, SLOT_SRC_VECTOR},
	{MAKE_SWZ3(W, Z, Y), R300_FPI0_ARGC_SRC0CA_WZY, 1, SLOT_SRC_BOTH},
	{MAKE_SWZ3(ONE, ONE, ONE), R300_FPI0_ARGC_ONE, 0, 0},
	{MAKE_SWZ3(ZERO, ZERO, ZERO), R300_FPI0_ARGC_ZERO, 0, 0},
	{MAKE_SWZ3(HALF, HALF, HALF), R300_FPI0_ARGC_HALF, 0, 0},
	{PFS_INVAL, 0, 0, 0},
	/* *INDENT-ON* */
};

/* used during matching of non-native swizzles */
#define SWZ_X_MASK (7 << 0)
#define SWZ_Y_MASK (7 << 3)
#define SWZ_Z_MASK (7 << 6)
#define SWZ_W_MASK (7 << 9)
static const struct {
	GLuint hash;		/* used to mask matching swizzle components */
	int mask;		/* actual outmask */
	int count;		/* count of components matched */
} s_mask[] = {
	/* *INDENT-OFF* */
	{SWZ_X_MASK | SWZ_Y_MASK | SWZ_Z_MASK, 1 | 2 | 4, 3},
	{SWZ_X_MASK | SWZ_Y_MASK, 1 | 2, 2},
	{SWZ_X_MASK | SWZ_Z_MASK, 1 | 4, 2},
	{SWZ_Y_MASK | SWZ_Z_MASK, 2 | 4, 2},
	{SWZ_X_MASK, 1, 1},
	{SWZ_Y_MASK, 2, 1},
	{SWZ_Z_MASK, 4, 1},
	{PFS_INVAL, PFS_INVAL, PFS_INVAL}
	/* *INDENT-ON* */
};

static const struct {
	int base;		/* hw value of swizzle */
	int stride;		/* difference between SRC0/1/2 */
	GLuint flags;
} s_swiz[] = {
	/* *INDENT-OFF* */
	{R300_FPI2_ARGA_SRC0C_X, 3, SLOT_SRC_VECTOR},
	{R300_FPI2_ARGA_SRC0C_Y, 3, SLOT_SRC_VECTOR},
	{R300_FPI2_ARGA_SRC0C_Z, 3, SLOT_SRC_VECTOR},
	{R300_FPI2_ARGA_SRC0A, 1, SLOT_SRC_SCALAR},
	{R300_FPI2_ARGA_ZERO, 0, 0},
	{R300_FPI2_ARGA_ONE, 0, 0},
	{R300_FPI2_ARGA_HALF, 0, 0}
	/* *INDENT-ON* */
};

/* boiler-plate reg, for convenience */
static const GLuint undef = REG(REG_TYPE_TEMP,
				0,
				SWIZZLE_XYZ,
				SWIZZLE_W,
				GL_FALSE,
				GL_FALSE,
				GL_FALSE);

/* constant one source */
static const GLuint pfs_one = REG(REG_TYPE_CONST,
				  0,
				  SWIZZLE_111,
				  SWIZZLE_ONE,
				  GL_FALSE,
				  GL_TRUE,
				  GL_TRUE);

/* constant half source */
static const GLuint pfs_half = REG(REG_TYPE_CONST,
				   0,
				   SWIZZLE_HHH,
				   SWIZZLE_HALF,
				   GL_FALSE,
				   GL_TRUE,
				   GL_TRUE);

/* constant zero source */
static const GLuint pfs_zero = REG(REG_TYPE_CONST,
				   0,
				   SWIZZLE_000,
				   SWIZZLE_ZERO,
				   GL_FALSE,
				   GL_TRUE,
				   GL_TRUE);

/*
 * Common functions prototypes
 */
static void dump_program(struct r300_fragment_program *fp);
static void emit_arith(struct r300_fragment_program *fp, int op,
		       GLuint dest, int mask,
		       GLuint src0, GLuint src1, GLuint src2, int flags);

/**
 * Get an R300 temporary that can be written to in the given slot.
 */
static int get_hw_temp(struct r300_fragment_program *fp, int slot)
{
	COMPILE_STATE;
	int r;

	for (r = 0; r < PFS_NUM_TEMP_REGS; ++r) {
		if (cs->hwtemps[r].free >= 0 && cs->hwtemps[r].free <= slot)
			break;
	}

	if (r >= PFS_NUM_TEMP_REGS) {
		ERROR("Out of hardware temps\n");
		return 0;
	}
	// Reserved is used to avoid the following scenario:
	//  R300 temporary X is first assigned to Mesa temporary Y during vector ops
	//  R300 temporary X is then assigned to Mesa temporary Z for further vector ops
	//  Then scalar ops on Mesa temporary Z are emitted and move back in time
	//  to overwrite the value of temporary Y.
	// End scenario.
	cs->hwtemps[r].reserved = cs->hwtemps[r].free;
	cs->hwtemps[r].free = -1;

	// Reset to some value that won't mess things up when the user
	// tries to read from a temporary that hasn't been assigned a value yet.
	// In the normal case, vector_valid and scalar_valid should be set to
	// a sane value by the first emit that writes to this temporary.
	cs->hwtemps[r].vector_valid = 0;
	cs->hwtemps[r].scalar_valid = 0;

	if (r > fp->max_temp_idx)
		fp->max_temp_idx = r;

	return r;
}

/**
 * Get an R300 temporary that will act as a TEX destination register.
 */
static int get_hw_temp_tex(struct r300_fragment_program *fp)
{
	COMPILE_STATE;
	int r;

	for (r = 0; r < PFS_NUM_TEMP_REGS; ++r) {
		if (cs->used_in_node & (1 << r))
			continue;

		// Note: Be very careful here
		if (cs->hwtemps[r].free >= 0 && cs->hwtemps[r].free <= 0)
			break;
	}

	if (r >= PFS_NUM_TEMP_REGS)
		return get_hw_temp(fp, 0);	/* Will cause an indirection */

	cs->hwtemps[r].reserved = cs->hwtemps[r].free;
	cs->hwtemps[r].free = -1;

	// Reset to some value that won't mess things up when the user
	// tries to read from a temporary that hasn't been assigned a value yet.
	// In the normal case, vector_valid and scalar_valid should be set to
	// a sane value by the first emit that writes to this temporary.
	cs->hwtemps[r].vector_valid = cs->nrslots;
	cs->hwtemps[r].scalar_valid = cs->nrslots;

	if (r > fp->max_temp_idx)
		fp->max_temp_idx = r;

	return r;
}

/**
 * Mark the given hardware register as free.
 */
static void free_hw_temp(struct r300_fragment_program *fp, int idx)
{
	COMPILE_STATE;

	// Be very careful here. Consider sequences like
	//  MAD r0, r1,r2,r3
	//  TEX r4, ...
	// The TEX instruction may be moved in front of the MAD instruction
	// due to the way nodes work. We don't want to alias r1 and r4 in
	// this case.
	// I'm certain the register allocation could be further sanitized,
	// but it's tricky because of stuff that can happen inside emit_tex
	// and emit_arith.
	cs->hwtemps[idx].free = cs->nrslots + 1;
}

/**
 * Create a new Mesa temporary register.
 */
static GLuint get_temp_reg(struct r300_fragment_program *fp)
{
	COMPILE_STATE;
	GLuint r = undef;
	GLuint index;

	index = ffs(~cs->temp_in_use);
	if (!index) {
		ERROR("Out of program temps\n");
		return r;
	}

	cs->temp_in_use |= (1 << --index);
	cs->temps[index].refcount = 0xFFFFFFFF;
	cs->temps[index].reg = -1;

	REG_SET_TYPE(r, REG_TYPE_TEMP);
	REG_SET_INDEX(r, index);
	REG_SET_VALID(r, GL_TRUE);
	return r;
}

/**
 * Create a new Mesa temporary register that will act as the destination
 * register for a texture read.
 */
static GLuint get_temp_reg_tex(struct r300_fragment_program *fp)
{
	COMPILE_STATE;
	GLuint r = undef;
	GLuint index;

	index = ffs(~cs->temp_in_use);
	if (!index) {
		ERROR("Out of program temps\n");
		return r;
	}

	cs->temp_in_use |= (1 << --index);
	cs->temps[index].refcount = 0xFFFFFFFF;
	cs->temps[index].reg = get_hw_temp_tex(fp);

	REG_SET_TYPE(r, REG_TYPE_TEMP);
	REG_SET_INDEX(r, index);
	REG_SET_VALID(r, GL_TRUE);
	return r;
}

/**
 * Free a Mesa temporary and the associated R300 temporary.
 */
static void free_temp(struct r300_fragment_program *fp, GLuint r)
{
	COMPILE_STATE;
	GLuint index = REG_GET_INDEX(r);

	if (!(cs->temp_in_use & (1 << index)))
		return;

	if (REG_GET_TYPE(r) == REG_TYPE_TEMP) {
		free_hw_temp(fp, cs->temps[index].reg);
		cs->temps[index].reg = -1;
		cs->temp_in_use &= ~(1 << index);
	} else if (REG_GET_TYPE(r) == REG_TYPE_INPUT) {
		free_hw_temp(fp, cs->inputs[index].reg);
		cs->inputs[index].reg = -1;
	}
}

/**
 * Emit a hardware constant/parameter.
 *
 * \p cp Stable pointer to an array of 4 floats.
 *  The pointer must be stable in the sense that it remains to be valid
 *  and hold the contents of the constant/parameter throughout the lifetime
 *  of the fragment program (actually, up until the next time the fragment
 *  program is translated).
 */
static GLuint emit_const4fv(struct r300_fragment_program *fp,
			    const GLfloat * cp)
{
	GLuint reg = undef;
	int index;

	for (index = 0; index < fp->const_nr; ++index) {
		if (fp->constant[index] == cp)
			break;
	}

	if (index >= fp->const_nr) {
		if (index >= PFS_NUM_CONST_REGS) {
			ERROR("Out of hw constants!\n");
			return reg;
		}

		fp->const_nr++;
		fp->constant[index] = cp;
	}

	REG_SET_TYPE(reg, REG_TYPE_CONST);
	REG_SET_INDEX(reg, index);
	REG_SET_VALID(reg, GL_TRUE);
	return reg;
}

static inline GLuint negate(GLuint r)
{
	REG_NEGS(r);
	REG_NEGV(r);
	return r;
}

/* Hack, to prevent clobbering sources used multiple times when
 * emulating non-native instructions
 */
static inline GLuint keep(GLuint r)
{
	REG_SET_NO_USE(r, GL_TRUE);
	return r;
}

static inline GLuint absolute(GLuint r)
{
	REG_ABS(r);
	return r;
}

static int swz_native(struct r300_fragment_program *fp,
		      GLuint src, GLuint * r, GLuint arbneg)
{
	/* Native swizzle, handle negation */
	src = (src & ~REG_NEGS_MASK) | (((arbneg >> 3) & 1) << REG_NEGS_SHIFT);

	if ((arbneg & 0x7) == 0x0) {
		src = src & ~REG_NEGV_MASK;
		*r = src;
	} else if ((arbneg & 0x7) == 0x7) {
		src |= REG_NEGV_MASK;
		*r = src;
	} else {
		if (!REG_GET_VALID(*r))
			*r = get_temp_reg(fp);
		src |= REG_NEGV_MASK;
		emit_arith(fp,
			   PFS_OP_MAD,
			   *r, arbneg & 0x7, keep(src), pfs_one, pfs_zero, 0);
		src = src & ~REG_NEGV_MASK;
		emit_arith(fp,
			   PFS_OP_MAD,
			   *r,
			   (arbneg ^ 0x7) | WRITEMASK_W,
			   src, pfs_one, pfs_zero, 0);
	}

	return 3;
}

static int swz_emit_partial(struct r300_fragment_program *fp,
			    GLuint src,
			    GLuint * r, int mask, int mc, GLuint arbneg)
{
	GLuint tmp;
	GLuint wmask = 0;

	if (!REG_GET_VALID(*r))
		*r = get_temp_reg(fp);

	/* A partial match, VSWZ/mask define what parts of the
	 * desired swizzle we match
	 */
	if (mc + s_mask[mask].count == 3) {
		wmask = WRITEMASK_W;
		src |= ((arbneg >> 3) & 1) << REG_NEGS_SHIFT;
	}

	tmp = arbneg & s_mask[mask].mask;
	if (tmp) {
		tmp = tmp ^ s_mask[mask].mask;
		if (tmp) {
			emit_arith(fp,
				   PFS_OP_MAD,
				   *r,
				   arbneg & s_mask[mask].mask,
				   keep(src) | REG_NEGV_MASK,
				   pfs_one, pfs_zero, 0);
			if (!wmask) {
				REG_SET_NO_USE(src, GL_TRUE);
			} else {
				REG_SET_NO_USE(src, GL_FALSE);
			}
			emit_arith(fp,
				   PFS_OP_MAD,
				   *r, tmp | wmask, src, pfs_one, pfs_zero, 0);
		} else {
			if (!wmask) {
				REG_SET_NO_USE(src, GL_TRUE);
			} else {
				REG_SET_NO_USE(src, GL_FALSE);
			}
			emit_arith(fp,
				   PFS_OP_MAD,
				   *r,
				   (arbneg & s_mask[mask].mask) | wmask,
				   src | REG_NEGV_MASK, pfs_one, pfs_zero, 0);
		}
	} else {
		if (!wmask) {
			REG_SET_NO_USE(src, GL_TRUE);
		} else {
			REG_SET_NO_USE(src, GL_FALSE);
		}
		emit_arith(fp, PFS_OP_MAD,
			   *r,
			   s_mask[mask].mask | wmask,
			   src, pfs_one, pfs_zero, 0);
	}

	return s_mask[mask].count;
}

static GLuint do_swizzle(struct r300_fragment_program *fp,
			 GLuint src, GLuint arbswz, GLuint arbneg)
{
	GLuint r = undef;
	GLuint vswz;
	int c_mask = 0;
	int v_match = 0;

	/* If swizzling from something without an XYZW native swizzle,
	 * emit result to a temp, and do new swizzle from the temp.
	 */
#if 0
	if (REG_GET_VSWZ(src) != SWIZZLE_XYZ || REG_GET_SSWZ(src) != SWIZZLE_W) {
		GLuint temp = get_temp_reg(fp);
		emit_arith(fp,
			   PFS_OP_MAD,
			   temp, WRITEMASK_XYZW, src, pfs_one, pfs_zero, 0);
		src = temp;
	}
#endif

	if (REG_GET_VSWZ(src) != SWIZZLE_XYZ || REG_GET_SSWZ(src) != SWIZZLE_W) {
		GLuint vsrcswz =
		    (v_swiz[REG_GET_VSWZ(src)].
		     hash & (SWZ_X_MASK | SWZ_Y_MASK | SWZ_Z_MASK)) |
		    REG_GET_SSWZ(src) << 9;
		GLint i;

		GLuint newswz = 0;
		GLuint offset;
		for (i = 0; i < 4; ++i) {
			offset = GET_SWZ(arbswz, i);

			newswz |=
			    (offset <= 3) ? GET_SWZ(vsrcswz,
						    offset) << i *
			    3 : offset << i * 3;
		}

		arbswz = newswz & (SWZ_X_MASK | SWZ_Y_MASK | SWZ_Z_MASK);
		REG_SET_SSWZ(src, GET_SWZ(newswz, 3));
	} else {
		/* set scalar swizzling */
		REG_SET_SSWZ(src, GET_SWZ(arbswz, 3));

	}
	do {
		vswz = REG_GET_VSWZ(src);
		do {
			int chash;

			REG_SET_VSWZ(src, vswz);
			chash = v_swiz[REG_GET_VSWZ(src)].hash &
			    s_mask[c_mask].hash;

			if (chash == (arbswz & s_mask[c_mask].hash)) {
				if (s_mask[c_mask].count == 3) {
					v_match += swz_native(fp,
							      src, &r, arbneg);
				} else {
					v_match += swz_emit_partial(fp,
								    src,
								    &r,
								    c_mask,
								    v_match,
								    arbneg);
				}

				if (v_match == 3)
					return r;

				/* Fill with something invalid.. all 0's was
				 * wrong before, matched SWIZZLE_X.  So all
				 * 1's will be okay for now
				 */
				arbswz |= (PFS_INVAL & s_mask[c_mask].hash);
			}
		} while (v_swiz[++vswz].hash != PFS_INVAL);
		REG_SET_VSWZ(src, SWIZZLE_XYZ);
	} while (s_mask[++c_mask].hash != PFS_INVAL);

	ERROR("should NEVER get here\n");
	return r;
}

static GLuint t_src(struct r300_fragment_program *fp,
		    struct prog_src_register fpsrc)
{
	GLuint r = undef;

	switch (fpsrc.File) {
	case PROGRAM_TEMPORARY:
		REG_SET_INDEX(r, fpsrc.Index);
		REG_SET_VALID(r, GL_TRUE);
		REG_SET_TYPE(r, REG_TYPE_TEMP);
		break;
	case PROGRAM_INPUT:
		REG_SET_INDEX(r, fpsrc.Index);
		REG_SET_VALID(r, GL_TRUE);
		REG_SET_TYPE(r, REG_TYPE_INPUT);
		break;
	case PROGRAM_LOCAL_PARAM:
		r = emit_const4fv(fp,
				  fp->mesa_program.Base.LocalParams[fpsrc.
								    Index]);
		break;
	case PROGRAM_ENV_PARAM:
		r = emit_const4fv(fp,
				  fp->ctx->FragmentProgram.Parameters[fpsrc.
								      Index]);
		break;
	case PROGRAM_STATE_VAR:
	case PROGRAM_NAMED_PARAM:
		r = emit_const4fv(fp,
				  fp->mesa_program.Base.Parameters->
				  ParameterValues[fpsrc.Index]);
		break;
	default:
		ERROR("unknown SrcReg->File %x\n", fpsrc.File);
		return r;
	}

	/* no point swizzling ONE/ZERO/HALF constants... */
	if (REG_GET_VSWZ(r) < SWIZZLE_111 || REG_GET_SSWZ(r) < SWIZZLE_ZERO)
		r = do_swizzle(fp, r, fpsrc.Swizzle, fpsrc.NegateBase);
	return r;
}

static GLuint t_scalar_src(struct r300_fragment_program *fp,
			   struct prog_src_register fpsrc)
{
	struct prog_src_register src = fpsrc;
	int sc = GET_SWZ(fpsrc.Swizzle, 0);	/* X */

	src.Swizzle = ((sc << 0) | (sc << 3) | (sc << 6) | (sc << 9));

	return t_src(fp, src);
}

static GLuint t_dst(struct r300_fragment_program *fp,
		    struct prog_dst_register dest)
{
	GLuint r = undef;

	switch (dest.File) {
	case PROGRAM_TEMPORARY:
		REG_SET_INDEX(r, dest.Index);
		REG_SET_VALID(r, GL_TRUE);
		REG_SET_TYPE(r, REG_TYPE_TEMP);
		return r;
	case PROGRAM_OUTPUT:
		REG_SET_TYPE(r, REG_TYPE_OUTPUT);
		switch (dest.Index) {
		case FRAG_RESULT_COLR:
		case FRAG_RESULT_DEPR:
			REG_SET_INDEX(r, dest.Index);
			REG_SET_VALID(r, GL_TRUE);
			return r;
		default:
			ERROR("Bad DstReg->Index 0x%x\n", dest.Index);
			return r;
		}
	default:
		ERROR("Bad DstReg->File 0x%x\n", dest.File);
		return r;
	}
}

static int t_hw_src(struct r300_fragment_program *fp, GLuint src, GLboolean tex)
{
	COMPILE_STATE;
	int idx;
	int index = REG_GET_INDEX(src);

	switch (REG_GET_TYPE(src)) {
	case REG_TYPE_TEMP:
		/* NOTE: if reg==-1 here, a source is being read that
		 *       hasn't been written to. Undefined results.
		 */
		if (cs->temps[index].reg == -1)
			cs->temps[index].reg = get_hw_temp(fp, cs->nrslots);

		idx = cs->temps[index].reg;

		if (!REG_GET_NO_USE(src) && (--cs->temps[index].refcount == 0))
			free_temp(fp, src);
		break;
	case REG_TYPE_INPUT:
		idx = cs->inputs[index].reg;

		if (!REG_GET_NO_USE(src) && (--cs->inputs[index].refcount == 0))
			free_hw_temp(fp, cs->inputs[index].reg);
		break;
	case REG_TYPE_CONST:
		return (index | SRC_CONST);
	default:
		ERROR("Invalid type for source reg\n");
		return (0 | SRC_CONST);
	}

	if (!tex)
		cs->used_in_node |= (1 << idx);

	return idx;
}

static int t_hw_dst(struct r300_fragment_program *fp,
		    GLuint dest, GLboolean tex, int slot)
{
	COMPILE_STATE;
	int idx;
	GLuint index = REG_GET_INDEX(dest);
	assert(REG_GET_VALID(dest));

	switch (REG_GET_TYPE(dest)) {
	case REG_TYPE_TEMP:
		if (cs->temps[REG_GET_INDEX(dest)].reg == -1) {
			if (!tex) {
				cs->temps[index].reg = get_hw_temp(fp, slot);
			} else {
				cs->temps[index].reg = get_hw_temp_tex(fp);
			}
		}
		idx = cs->temps[index].reg;

		if (!REG_GET_NO_USE(dest) && (--cs->temps[index].refcount == 0))
			free_temp(fp, dest);

		cs->dest_in_node |= (1 << idx);
		cs->used_in_node |= (1 << idx);
		break;
	case REG_TYPE_OUTPUT:
		switch (index) {
		case FRAG_RESULT_COLR:
			fp->node[fp->cur_node].flags |=
			    R300_PFS_NODE_OUTPUT_COLOR;
			break;
		case FRAG_RESULT_DEPR:
			fp->node[fp->cur_node].flags |=
			    R300_PFS_NODE_OUTPUT_DEPTH;
			break;
		}
		return index;
		break;
	default:
		ERROR("invalid dest reg type %d\n", REG_GET_TYPE(dest));
		return 0;
	}

	return idx;
}

static void emit_nop(struct r300_fragment_program *fp)
{
	COMPILE_STATE;

	if (cs->nrslots >= PFS_MAX_ALU_INST) {
		ERROR("Out of ALU instruction slots\n");
		return;
	}

	fp->alu.inst[cs->nrslots].inst0 = NOP_INST0;
	fp->alu.inst[cs->nrslots].inst1 = NOP_INST1;
	fp->alu.inst[cs->nrslots].inst2 = NOP_INST2;
	fp->alu.inst[cs->nrslots].inst3 = NOP_INST3;
	cs->nrslots++;
}

static void emit_tex(struct r300_fragment_program *fp,
		     struct prog_instruction *fpi, int opcode)
{
	COMPILE_STATE;
	GLuint coord = t_src(fp, fpi->SrcReg[0]);
	GLuint dest = undef, rdest = undef;
	GLuint din, uin;
	int unit = fpi->TexSrcUnit;
	int hwsrc, hwdest;
	GLuint tempreg = 0;

	uin = cs->used_in_node;
	din = cs->dest_in_node;

	/* Resolve source/dest to hardware registers */
	if (opcode != R300_FPITX_OP_KIL) {
		if (fpi->TexSrcTarget == TEXTURE_RECT_INDEX) {
			/**
			 * Hardware uses [0..1]x[0..1] range for rectangle textures
			 * instead of [0..Width]x[0..Height].
			 * Add a scaling instruction.
			 *
			 * \todo Refactor this once we have proper rewriting/optimization
			 * support for programs.
			 */
			gl_state_index tokens[STATE_LENGTH] = {
				STATE_INTERNAL, STATE_R300_TEXRECT_FACTOR, 0, 0,
				0
			};
			int factor_index;
			GLuint factorreg;

			tokens[2] = unit;
			factor_index =
			    _mesa_add_state_reference(fp->mesa_program.Base.
						      Parameters, tokens);
			factorreg =
			    emit_const4fv(fp,
					  fp->mesa_program.Base.Parameters->
					  ParameterValues[factor_index]);
			tempreg = keep(get_temp_reg(fp));

			emit_arith(fp, PFS_OP_MAD, tempreg, WRITEMASK_XYZW,
				   coord, factorreg, pfs_zero, 0);

			/* Ensure correct node indirection */
			uin = cs->used_in_node;
			din = cs->dest_in_node;

			hwsrc = t_hw_src(fp, tempreg, GL_TRUE);
		} else {
			hwsrc = t_hw_src(fp, coord, GL_TRUE);
		}

		dest = t_dst(fp, fpi->DstReg);

		/* r300 doesn't seem to be able to do TEX->output reg */
		if (REG_GET_TYPE(dest) == REG_TYPE_OUTPUT) {
			rdest = dest;
			dest = get_temp_reg_tex(fp);
		}
		hwdest =
		    t_hw_dst(fp, dest, GL_TRUE,
			     fp->node[fp->cur_node].alu_offset);

		/* Use a temp that hasn't been used in this node, rather
		 * than causing an indirection
		 */
		if (uin & (1 << hwdest)) {
			free_hw_temp(fp, hwdest);
			hwdest = get_hw_temp_tex(fp);
			cs->temps[REG_GET_INDEX(dest)].reg = hwdest;
		}
	} else {
		hwdest = 0;
		unit = 0;
		hwsrc = t_hw_src(fp, coord, GL_TRUE);
	}

	/* Indirection if source has been written in this node, or if the
	 * dest has been read/written in this node
	 */
	if ((REG_GET_TYPE(coord) != REG_TYPE_CONST &&
	     (din & (1 << hwsrc))) || (uin & (1 << hwdest))) {

		/* Finish off current node */
		if (fp->node[fp->cur_node].alu_offset == cs->nrslots)
			emit_nop(fp);

		fp->node[fp->cur_node].alu_end =
		    cs->nrslots - fp->node[fp->cur_node].alu_offset - 1;
		assert(fp->node[fp->cur_node].alu_end >= 0);

		if (++fp->cur_node >= PFS_MAX_TEX_INDIRECT) {
			ERROR("too many levels of texture indirection\n");
			return;
		}

		/* Start new node */
		fp->node[fp->cur_node].tex_offset = fp->tex.length;
		fp->node[fp->cur_node].alu_offset = cs->nrslots;
		fp->node[fp->cur_node].tex_end = -1;
		fp->node[fp->cur_node].alu_end = -1;
		fp->node[fp->cur_node].flags = 0;
		cs->used_in_node = 0;
		cs->dest_in_node = 0;
	}

	if (fp->cur_node == 0)
		fp->first_node_has_tex = 1;

	fp->tex.inst[fp->tex.length++] = 0 | (hwsrc << R300_FPITX_SRC_SHIFT)
	    | (hwdest << R300_FPITX_DST_SHIFT)
	    | (unit << R300_FPITX_IMAGE_SHIFT)
	    /* not entirely sure about this */
	    | (opcode << R300_FPITX_OPCODE_SHIFT);

	cs->dest_in_node |= (1 << hwdest);
	if (REG_GET_TYPE(coord) != REG_TYPE_CONST)
		cs->used_in_node |= (1 << hwsrc);

	fp->node[fp->cur_node].tex_end++;

	/* Copy from temp to output if needed */
	if (REG_GET_VALID(rdest)) {
		emit_arith(fp, PFS_OP_MAD, rdest, WRITEMASK_XYZW, dest,
			   pfs_one, pfs_zero, 0);
		free_temp(fp, dest);
	}

	/* Free temp register */
	if (tempreg != 0)
		free_temp(fp, tempreg);
}

/**
 * Returns the first slot where we could possibly allow writing to dest,
 * according to register allocation.
 */
static int get_earliest_allowed_write(struct r300_fragment_program *fp,
				      GLuint dest, int mask)
{
	COMPILE_STATE;
	int idx;
	int pos;
	GLuint index = REG_GET_INDEX(dest);
	assert(REG_GET_VALID(dest));

	switch (REG_GET_TYPE(dest)) {
	case REG_TYPE_TEMP:
		if (cs->temps[index].reg == -1)
			return 0;

		idx = cs->temps[index].reg;
		break;
	case REG_TYPE_OUTPUT:
		return 0;
	default:
		ERROR("invalid dest reg type %d\n", REG_GET_TYPE(dest));
		return 0;
	}

	pos = cs->hwtemps[idx].reserved;
	if (mask & WRITEMASK_XYZ) {
		if (pos < cs->hwtemps[idx].vector_lastread)
			pos = cs->hwtemps[idx].vector_lastread;
	}
	if (mask & WRITEMASK_W) {
		if (pos < cs->hwtemps[idx].scalar_lastread)
			pos = cs->hwtemps[idx].scalar_lastread;
	}

	return pos;
}

/**
 * Allocates a slot for an ALU instruction that can consist of
 * a vertex part or a scalar part or both.
 *
 * Sources from src (src[0] to src[argc-1]) are added to the slot in the
 * appropriate position (vector and/or scalar), and their positions are
 * recorded in the srcpos array.
 *
 * This function emits instruction code for the source fetch and the
 * argument selection. It does not emit instruction code for the
 * opcode or the destination selection.
 *
 * @return the index of the slot
 */
static int find_and_prepare_slot(struct r300_fragment_program *fp,
				 GLboolean emit_vop,
				 GLboolean emit_sop,
				 int argc, GLuint * src, GLuint dest, int mask)
{
	COMPILE_STATE;
	int hwsrc[3];
	int srcpos[3];
	unsigned int used;
	int tempused;
	int tempvsrc[3];
	int tempssrc[3];
	int pos;
	int regnr;
	int i, j;

	// Determine instruction slots, whether sources are required on
	// vector or scalar side, and the smallest slot number where
	// all source registers are available
	used = 0;
	if (emit_vop)
		used |= SLOT_OP_VECTOR;
	if (emit_sop)
		used |= SLOT_OP_SCALAR;

	pos = get_earliest_allowed_write(fp, dest, mask);

	if (fp->node[fp->cur_node].alu_offset > pos)
		pos = fp->node[fp->cur_node].alu_offset;
	for (i = 0; i < argc; ++i) {
		if (!REG_GET_BUILTIN(src[i])) {
			if (emit_vop)
				used |= v_swiz[REG_GET_VSWZ(src[i])].flags << i;
			if (emit_sop)
				used |= s_swiz[REG_GET_SSWZ(src[i])].flags << i;
		}

		hwsrc[i] = t_hw_src(fp, src[i], GL_FALSE);	/* Note: sideeffects wrt refcounting! */
		regnr = hwsrc[i] & 31;

		if (REG_GET_TYPE(src[i]) == REG_TYPE_TEMP) {
			if (used & (SLOT_SRC_VECTOR << i)) {
				if (cs->hwtemps[regnr].vector_valid > pos)
					pos = cs->hwtemps[regnr].vector_valid;
			}
			if (used & (SLOT_SRC_SCALAR << i)) {
				if (cs->hwtemps[regnr].scalar_valid > pos)
					pos = cs->hwtemps[regnr].scalar_valid;
			}
		}
	}

	// Find a slot that fits
	for (;; ++pos) {
		if (cs->slot[pos].used & used & SLOT_OP_BOTH)
			continue;

		if (pos >= cs->nrslots) {
			if (cs->nrslots >= PFS_MAX_ALU_INST) {
				ERROR("Out of ALU instruction slots\n");
				return -1;
			}

			fp->alu.inst[pos].inst0 = NOP_INST0;
			fp->alu.inst[pos].inst1 = NOP_INST1;
			fp->alu.inst[pos].inst2 = NOP_INST2;
			fp->alu.inst[pos].inst3 = NOP_INST3;

			cs->nrslots++;
		}
		// Note: When we need both parts (vector and scalar) of a source,
		// we always try to put them into the same position. This makes the
		// code easier to read, and it is optimal (i.e. one doesn't gain
		// anything by splitting the parts).
		// It also avoids headaches with swizzles that access both parts (i.e WXY)
		tempused = cs->slot[pos].used;
		for (i = 0; i < 3; ++i) {
			tempvsrc[i] = cs->slot[pos].vsrc[i];
			tempssrc[i] = cs->slot[pos].ssrc[i];
		}

		for (i = 0; i < argc; ++i) {
			int flags = (used >> i) & SLOT_SRC_BOTH;

			if (!flags) {
				srcpos[i] = 0;
				continue;
			}

			for (j = 0; j < 3; ++j) {
				if ((tempused >> j) & flags & SLOT_SRC_VECTOR) {
					if (tempvsrc[j] != hwsrc[i])
						continue;
				}

				if ((tempused >> j) & flags & SLOT_SRC_SCALAR) {
					if (tempssrc[j] != hwsrc[i])
						continue;
				}

				break;
			}

			if (j == 3)
				break;

			srcpos[i] = j;
			tempused |= flags << j;
			if (flags & SLOT_SRC_VECTOR)
				tempvsrc[j] = hwsrc[i];
			if (flags & SLOT_SRC_SCALAR)
				tempssrc[j] = hwsrc[i];
		}

		if (i == argc)
			break;
	}

	// Found a slot, reserve it
	cs->slot[pos].used = tempused | (used & SLOT_OP_BOTH);
	for (i = 0; i < 3; ++i) {
		cs->slot[pos].vsrc[i] = tempvsrc[i];
		cs->slot[pos].ssrc[i] = tempssrc[i];
	}

	for (i = 0; i < argc; ++i) {
		if (REG_GET_TYPE(src[i]) == REG_TYPE_TEMP) {
			int regnr = hwsrc[i] & 31;

			if (used & (SLOT_SRC_VECTOR << i)) {
				if (cs->hwtemps[regnr].vector_lastread < pos)
					cs->hwtemps[regnr].vector_lastread =
					    pos;
			}
			if (used & (SLOT_SRC_SCALAR << i)) {
				if (cs->hwtemps[regnr].scalar_lastread < pos)
					cs->hwtemps[regnr].scalar_lastread =
					    pos;
			}
		}
	}

	// Emit the source fetch code
	fp->alu.inst[pos].inst1 &= ~R300_FPI1_SRC_MASK;
	fp->alu.inst[pos].inst1 |=
	    ((cs->slot[pos].vsrc[0] << R300_FPI1_SRC0C_SHIFT) |
	     (cs->slot[pos].vsrc[1] << R300_FPI1_SRC1C_SHIFT) |
	     (cs->slot[pos].vsrc[2] << R300_FPI1_SRC2C_SHIFT));

	fp->alu.inst[pos].inst3 &= ~R300_FPI3_SRC_MASK;
	fp->alu.inst[pos].inst3 |=
	    ((cs->slot[pos].ssrc[0] << R300_FPI3_SRC0A_SHIFT) |
	     (cs->slot[pos].ssrc[1] << R300_FPI3_SRC1A_SHIFT) |
	     (cs->slot[pos].ssrc[2] << R300_FPI3_SRC2A_SHIFT));

	// Emit the argument selection code
	if (emit_vop) {
		int swz[3];

		for (i = 0; i < 3; ++i) {
			if (i < argc) {
				swz[i] = (v_swiz[REG_GET_VSWZ(src[i])].base +
					  (srcpos[i] *
					   v_swiz[REG_GET_VSWZ(src[i])].
					   stride)) | ((src[i] & REG_NEGV_MASK)
						       ? ARG_NEG : 0) | ((src[i]
									  &
									  REG_ABS_MASK)
									 ?
									 ARG_ABS
									 : 0);
			} else {
				swz[i] = R300_FPI0_ARGC_ZERO;
			}
		}

		fp->alu.inst[pos].inst0 &=
		    ~(R300_FPI0_ARG0C_MASK | R300_FPI0_ARG1C_MASK |
		      R300_FPI0_ARG2C_MASK);
		fp->alu.inst[pos].inst0 |=
		    (swz[0] << R300_FPI0_ARG0C_SHIFT) | (swz[1] <<
							 R300_FPI0_ARG1C_SHIFT)
		    | (swz[2] << R300_FPI0_ARG2C_SHIFT);
	}

	if (emit_sop) {
		int swz[3];

		for (i = 0; i < 3; ++i) {
			if (i < argc) {
				swz[i] = (s_swiz[REG_GET_SSWZ(src[i])].base +
					  (srcpos[i] *
					   s_swiz[REG_GET_SSWZ(src[i])].
					   stride)) | ((src[i] & REG_NEGV_MASK)
						       ? ARG_NEG : 0) | ((src[i]
									  &
									  REG_ABS_MASK)
									 ?
									 ARG_ABS
									 : 0);
			} else {
				swz[i] = R300_FPI2_ARGA_ZERO;
			}
		}

		fp->alu.inst[pos].inst2 &=
		    ~(R300_FPI2_ARG0A_MASK | R300_FPI2_ARG1A_MASK |
		      R300_FPI2_ARG2A_MASK);
		fp->alu.inst[pos].inst2 |=
		    (swz[0] << R300_FPI2_ARG0A_SHIFT) | (swz[1] <<
							 R300_FPI2_ARG1A_SHIFT)
		    | (swz[2] << R300_FPI2_ARG2A_SHIFT);
	}

	return pos;
}

/**
 * Append an ALU instruction to the instruction list.
 */
static void emit_arith(struct r300_fragment_program *fp,
		       int op,
		       GLuint dest,
		       int mask,
		       GLuint src0, GLuint src1, GLuint src2, int flags)
{
	COMPILE_STATE;
	GLuint src[3] = { src0, src1, src2 };
	int hwdest;
	GLboolean emit_vop, emit_sop;
	int vop, sop, argc;
	int pos;

	vop = r300_fpop[op].v_op;
	sop = r300_fpop[op].s_op;
	argc = r300_fpop[op].argc;

	if (REG_GET_TYPE(dest) == REG_TYPE_OUTPUT &&
	    REG_GET_INDEX(dest) == FRAG_RESULT_DEPR) {
		if (mask & WRITEMASK_Z) {
			mask = WRITEMASK_W;
		} else {
			return;
		}
	}

	emit_vop = GL_FALSE;
	emit_sop = GL_FALSE;
	if ((mask & WRITEMASK_XYZ) || vop == R300_FPI0_OUTC_DP3)
		emit_vop = GL_TRUE;
	if ((mask & WRITEMASK_W) || vop == R300_FPI0_OUTC_REPL_ALPHA)
		emit_sop = GL_TRUE;

	pos =
	    find_and_prepare_slot(fp, emit_vop, emit_sop, argc, src, dest,
				  mask);
	if (pos < 0)
		return;

	hwdest = t_hw_dst(fp, dest, GL_FALSE, pos);	/* Note: Side effects wrt register allocation */

	if (flags & PFS_FLAG_SAT) {
		vop |= R300_FPI0_OUTC_SAT;
		sop |= R300_FPI2_OUTA_SAT;
	}

	/* Throw the pieces together and get FPI0/1 */
	if (emit_vop) {
		fp->alu.inst[pos].inst0 |= vop;

		fp->alu.inst[pos].inst1 |= hwdest << R300_FPI1_DSTC_SHIFT;

		if (REG_GET_TYPE(dest) == REG_TYPE_OUTPUT) {
			if (REG_GET_INDEX(dest) == FRAG_RESULT_COLR) {
				fp->alu.inst[pos].inst1 |=
				    (mask & WRITEMASK_XYZ) <<
				    R300_FPI1_DSTC_OUTPUT_MASK_SHIFT;
			} else
				assert(0);
		} else {
			fp->alu.inst[pos].inst1 |=
			    (mask & WRITEMASK_XYZ) <<
			    R300_FPI1_DSTC_REG_MASK_SHIFT;

			cs->hwtemps[hwdest].vector_valid = pos + 1;
		}
	}

	/* And now FPI2/3 */
	if (emit_sop) {
		fp->alu.inst[pos].inst2 |= sop;

		if (mask & WRITEMASK_W) {
			if (REG_GET_TYPE(dest) == REG_TYPE_OUTPUT) {
				if (REG_GET_INDEX(dest) == FRAG_RESULT_COLR) {
					fp->alu.inst[pos].inst3 |=
					    (hwdest << R300_FPI3_DSTA_SHIFT) |
					    R300_FPI3_DSTA_OUTPUT;
				} else if (REG_GET_INDEX(dest) ==
					   FRAG_RESULT_DEPR) {
					fp->alu.inst[pos].inst3 |=
					    R300_FPI3_DSTA_DEPTH;
				} else
					assert(0);
			} else {
				fp->alu.inst[pos].inst3 |=
				    (hwdest << R300_FPI3_DSTA_SHIFT) |
				    R300_FPI3_DSTA_REG;

				cs->hwtemps[hwdest].scalar_valid = pos + 1;
			}
		}
	}

	return;
}

#if 0
static GLuint get_attrib(struct r300_fragment_program *fp, GLuint attr)
{
	struct gl_fragment_program *mp = &fp->mesa_program;
	GLuint r = undef;

	if (!(mp->Base.InputsRead & (1 << attr))) {
		ERROR("Attribute %d was not provided!\n", attr);
		return undef;
	}

	REG_SET_TYPE(r, REG_TYPE_INPUT);
	REG_SET_INDEX(r, attr);
	REG_SET_VALID(r, GL_TRUE);
	return r;
}
#endif

static GLfloat SinCosConsts[2][4] = {
	{
	 1.273239545,		// 4/PI
	 -0.405284735,		// -4/(PI*PI)
	 3.141592654,		// PI
	 0.2225			// weight
	 },
	{
	 0.75,
	 0.0,
	 0.159154943,		// 1/(2*PI)
	 6.283185307		// 2*PI
	 }
};

/**
 * Emit a LIT instruction.
 * \p flags may be PFS_FLAG_SAT
 *
 * Definition of LIT (from ARB_fragment_program):
 * tmp = VectorLoad(op0);
 * if (tmp.x < 0) tmp.x = 0;
 * if (tmp.y < 0) tmp.y = 0;
 * if (tmp.w < -(128.0-epsilon)) tmp.w = -(128.0-epsilon);
 * else if (tmp.w > 128-epsilon) tmp.w = 128-epsilon;
 * result.x = 1.0;
 * result.y = tmp.x;
 * result.z = (tmp.x > 0) ? RoughApproxPower(tmp.y, tmp.w) : 0.0;
 * result.w = 1.0;
 *
 * The longest path of computation is the one leading to result.z,
 * consisting of 5 operations. This implementation of LIT takes
 * 5 slots. So unless there's some special undocumented opcode,
 * this implementation is potentially optimal. Unfortunately,
 * emit_arith is a bit too conservative because it doesn't understand
 * partial writes to the vector component.
 */
static const GLfloat LitConst[4] =
    { 127.999999, 127.999999, 127.999999, -127.999999 };

static void emit_lit(struct r300_fragment_program *fp,
		     GLuint dest, int mask, GLuint src, int flags)
{
	COMPILE_STATE;
	GLuint cnst;
	int needTemporary;
	GLuint temp;

	cnst = emit_const4fv(fp, LitConst);

	needTemporary = 0;
	if ((mask & WRITEMASK_XYZW) != WRITEMASK_XYZW) {
		needTemporary = 1;
	} else if (REG_GET_TYPE(dest) == REG_TYPE_OUTPUT) {
		// LIT is typically followed by DP3/DP4, so there's no point
		// in creating special code for this case
		needTemporary = 1;
	}

	if (needTemporary) {
		temp = keep(get_temp_reg(fp));
	} else {
		temp = keep(dest);
	}

	// Note: The order of emit_arith inside the slots is relevant,
	// because emit_arith only looks at scalar vs. vector when resolving
	// dependencies, and it does not consider individual vector components,
	// so swizzling between the two parts can create fake dependencies.

	// First slot
	emit_arith(fp, PFS_OP_MAX, temp, WRITEMASK_XY,
		   keep(src), pfs_zero, undef, 0);
	emit_arith(fp, PFS_OP_MAX, temp, WRITEMASK_W, src, cnst, undef, 0);

	// Second slot
	emit_arith(fp, PFS_OP_MIN, temp, WRITEMASK_Z,
		   swizzle(temp, W, W, W, W), cnst, undef, 0);
	emit_arith(fp, PFS_OP_LG2, temp, WRITEMASK_W,
		   swizzle(temp, Y, Y, Y, Y), undef, undef, 0);

	// Third slot
	// If desired, we saturate the y result here.
	// This does not affect the use as a condition variable in the CMP later
	emit_arith(fp, PFS_OP_MAD, temp, WRITEMASK_W,
		   temp, swizzle(temp, Z, Z, Z, Z), pfs_zero, 0);
	emit_arith(fp, PFS_OP_MAD, temp, WRITEMASK_Y,
		   swizzle(temp, X, X, X, X), pfs_one, pfs_zero, flags);

	// Fourth slot
	emit_arith(fp, PFS_OP_MAD, temp, WRITEMASK_X,
		   pfs_one, pfs_one, pfs_zero, 0);
	emit_arith(fp, PFS_OP_EX2, temp, WRITEMASK_W, temp, undef, undef, 0);

	// Fifth slot
	emit_arith(fp, PFS_OP_CMP, temp, WRITEMASK_Z,
		   pfs_zero, swizzle(temp, W, W, W, W),
		   negate(swizzle(temp, Y, Y, Y, Y)), flags);
	emit_arith(fp, PFS_OP_MAD, temp, WRITEMASK_W, pfs_one, pfs_one,
		   pfs_zero, 0);

	if (needTemporary) {
		emit_arith(fp, PFS_OP_MAD, dest, mask,
			   temp, pfs_one, pfs_zero, flags);
		free_temp(fp, temp);
	} else {
		// Decrease refcount of the destination
		t_hw_dst(fp, dest, GL_FALSE, cs->nrslots);
	}
}

static GLboolean parse_program(struct r300_fragment_program *fp)
{
	struct gl_fragment_program *mp = &fp->mesa_program;
	const struct prog_instruction *inst = mp->Base.Instructions;
	struct prog_instruction *fpi;
	GLuint src[3], dest, temp[2];
	int flags, mask = 0;
	int const_sin[2];

	if (!inst || inst[0].Opcode == OPCODE_END) {
		ERROR("empty program?\n");
		return GL_FALSE;
	}

	for (fpi = mp->Base.Instructions; fpi->Opcode != OPCODE_END; fpi++) {
		if (fpi->SaturateMode == SATURATE_ZERO_ONE)
			flags = PFS_FLAG_SAT;
		else
			flags = 0;

		if (fpi->Opcode != OPCODE_KIL) {
			dest = t_dst(fp, fpi->DstReg);
			mask = fpi->DstReg.WriteMask;
		}

		switch (fpi->Opcode) {
		case OPCODE_ABS:
			src[0] = t_src(fp, fpi->SrcReg[0]);
			emit_arith(fp, PFS_OP_MAD, dest, mask,
				   absolute(src[0]), pfs_one, pfs_zero, flags);
			break;
		case OPCODE_ADD:
			src[0] = t_src(fp, fpi->SrcReg[0]);
			src[1] = t_src(fp, fpi->SrcReg[1]);
			emit_arith(fp, PFS_OP_MAD, dest, mask,
				   src[0], pfs_one, src[1], flags);
			break;
		case OPCODE_CMP:
			src[0] = t_src(fp, fpi->SrcReg[0]);
			src[1] = t_src(fp, fpi->SrcReg[1]);
			src[2] = t_src(fp, fpi->SrcReg[2]);
			/* ARB_f_p - if src0.c < 0.0 ? src1.c : src2.c
			 *    r300 - if src2.c < 0.0 ? src1.c : src0.c
			 */
			emit_arith(fp, PFS_OP_CMP, dest, mask,
				   src[2], src[1], src[0], flags);
			break;
		case OPCODE_COS:
			/*
			 * cos using a parabola (see SIN):
			 * cos(x):
			 *   x = (x/(2*PI))+0.75
			 *   x = frac(x)
			 *   x = (x*2*PI)-PI
			 *   result = sin(x)
			 */
			temp[0] = get_temp_reg(fp);
			const_sin[0] = emit_const4fv(fp, SinCosConsts[0]);
			const_sin[1] = emit_const4fv(fp, SinCosConsts[1]);
			src[0] = t_scalar_src(fp, fpi->SrcReg[0]);

			/* add 0.5*PI and do range reduction */

			emit_arith(fp, PFS_OP_MAD, temp[0], WRITEMASK_X,
				   swizzle(src[0], X, X, X, X),
				   swizzle(const_sin[1], Z, Z, Z, Z),
				   swizzle(const_sin[1], X, X, X, X), 0);

			emit_arith(fp, PFS_OP_FRC, temp[0], WRITEMASK_X,
				   swizzle(temp[0], X, X, X, X),
				   undef, undef, 0);

			emit_arith(fp, PFS_OP_MAD, temp[0], WRITEMASK_Z, swizzle(temp[0], X, X, X, X), swizzle(const_sin[1], W, W, W, W),	//2*PI
				   negate(swizzle(const_sin[0], Z, Z, Z, Z)),	//-PI
				   0);

			/* SIN */

			emit_arith(fp, PFS_OP_MAD, temp[0],
				   WRITEMASK_X | WRITEMASK_Y, swizzle(temp[0],
								      Z, Z, Z,
								      Z),
				   const_sin[0], pfs_zero, 0);

			emit_arith(fp, PFS_OP_MAD, temp[0], WRITEMASK_X,
				   swizzle(temp[0], Y, Y, Y, Y),
				   absolute(swizzle(temp[0], Z, Z, Z, Z)),
				   swizzle(temp[0], X, X, X, X), 0);

			emit_arith(fp, PFS_OP_MAD, temp[0], WRITEMASK_Y,
				   swizzle(temp[0], X, X, X, X),
				   absolute(swizzle(temp[0], X, X, X, X)),
				   negate(swizzle(temp[0], X, X, X, X)), 0);

			emit_arith(fp, PFS_OP_MAD, dest, mask,
				   swizzle(temp[0], Y, Y, Y, Y),
				   swizzle(const_sin[0], W, W, W, W),
				   swizzle(temp[0], X, X, X, X), flags);

			free_temp(fp, temp[0]);
			break;
		case OPCODE_DP3:
			src[0] = t_src(fp, fpi->SrcReg[0]);
			src[1] = t_src(fp, fpi->SrcReg[1]);
			emit_arith(fp, PFS_OP_DP3, dest, mask,
				   src[0], src[1], undef, flags);
			break;
		case OPCODE_DP4:
			src[0] = t_src(fp, fpi->SrcReg[0]);
			src[1] = t_src(fp, fpi->SrcReg[1]);
			emit_arith(fp, PFS_OP_DP4, dest, mask,
				   src[0], src[1], undef, flags);
			break;
		case OPCODE_DPH:
			src[0] = t_src(fp, fpi->SrcReg[0]);
			src[1] = t_src(fp, fpi->SrcReg[1]);
			/* src0.xyz1 -> temp
			 * DP4 dest, temp, src1
			 */
#if 0
			temp[0] = get_temp_reg(fp);
			src[0].s_swz = SWIZZLE_ONE;
			emit_arith(fp, PFS_OP_MAD, temp[0], mask,
				   src[0], pfs_one, pfs_zero, 0);
			emit_arith(fp, PFS_OP_DP4, dest, mask,
				   temp[0], src[1], undef, flags);
			free_temp(fp, temp[0]);
#else
			emit_arith(fp, PFS_OP_DP4, dest, mask,
				   swizzle(src[0], X, Y, Z, ONE), src[1],
				   undef, flags);
#endif
			break;
		case OPCODE_DST:
			src[0] = t_src(fp, fpi->SrcReg[0]);
			src[1] = t_src(fp, fpi->SrcReg[1]);
			/* dest.y = src0.y * src1.y */
			if (mask & WRITEMASK_Y)
				emit_arith(fp, PFS_OP_MAD, dest, WRITEMASK_Y,
					   keep(src[0]), keep(src[1]),
					   pfs_zero, flags);
			/* dest.z = src0.z */
			if (mask & WRITEMASK_Z)
				emit_arith(fp, PFS_OP_MAD, dest, WRITEMASK_Z,
					   src[0], pfs_one, pfs_zero, flags);
			/* result.x = 1.0
			 * result.w = src1.w */
			if (mask & WRITEMASK_XW) {
				REG_SET_VSWZ(src[1], SWIZZLE_111);	/*Cheat */
				emit_arith(fp, PFS_OP_MAD, dest,
					   mask & WRITEMASK_XW,
					   src[1], pfs_one, pfs_zero, flags);
			}
			break;
		case OPCODE_EX2:
			src[0] = t_scalar_src(fp, fpi->SrcReg[0]);
			emit_arith(fp, PFS_OP_EX2, dest, mask,
				   src[0], undef, undef, flags);
			break;
		case OPCODE_FLR:
			src[0] = t_src(fp, fpi->SrcReg[0]);
			temp[0] = get_temp_reg(fp);
			/* FRC temp, src0
			 * MAD dest, src0, 1.0, -temp
			 */
			emit_arith(fp, PFS_OP_FRC, temp[0], mask,
				   keep(src[0]), undef, undef, 0);
			emit_arith(fp, PFS_OP_MAD, dest, mask,
				   src[0], pfs_one, negate(temp[0]), flags);
			free_temp(fp, temp[0]);
			break;
		case OPCODE_FRC:
			src[0] = t_src(fp, fpi->SrcReg[0]);
			emit_arith(fp, PFS_OP_FRC, dest, mask,
				   src[0], undef, undef, flags);
			break;
		case OPCODE_KIL:
			emit_tex(fp, fpi, R300_FPITX_OP_KIL);
			break;
		case OPCODE_LG2:
			src[0] = t_scalar_src(fp, fpi->SrcReg[0]);
			emit_arith(fp, PFS_OP_LG2, dest, mask,
				   src[0], undef, undef, flags);
			break;
		case OPCODE_LIT:
			src[0] = t_src(fp, fpi->SrcReg[0]);
			emit_lit(fp, dest, mask, src[0], flags);
			break;
		case OPCODE_LRP:
			src[0] = t_src(fp, fpi->SrcReg[0]);
			src[1] = t_src(fp, fpi->SrcReg[1]);
			src[2] = t_src(fp, fpi->SrcReg[2]);
			/* result = tmp0tmp1 + (1 - tmp0)tmp2
			 *        = tmp0tmp1 + tmp2 + (-tmp0)tmp2
			 *     MAD temp, -tmp0, tmp2, tmp2
			 *     MAD result, tmp0, tmp1, temp
			 */
			temp[0] = get_temp_reg(fp);
			emit_arith(fp, PFS_OP_MAD, temp[0], mask,
				   negate(keep(src[0])), keep(src[2]), src[2],
				   0);
			emit_arith(fp, PFS_OP_MAD, dest, mask,
				   src[0], src[1], temp[0], flags);
			free_temp(fp, temp[0]);
			break;
		case OPCODE_MAD:
			src[0] = t_src(fp, fpi->SrcReg[0]);
			src[1] = t_src(fp, fpi->SrcReg[1]);
			src[2] = t_src(fp, fpi->SrcReg[2]);
			emit_arith(fp, PFS_OP_MAD, dest, mask,
				   src[0], src[1], src[2], flags);
			break;
		case OPCODE_MAX:
			src[0] = t_src(fp, fpi->SrcReg[0]);
			src[1] = t_src(fp, fpi->SrcReg[1]);
			emit_arith(fp, PFS_OP_MAX, dest, mask,
				   src[0], src[1], undef, flags);
			break;
		case OPCODE_MIN:
			src[0] = t_src(fp, fpi->SrcReg[0]);
			src[1] = t_src(fp, fpi->SrcReg[1]);
			emit_arith(fp, PFS_OP_MIN, dest, mask,
				   src[0], src[1], undef, flags);
			break;
		case OPCODE_MOV:
		case OPCODE_SWZ:
			src[0] = t_src(fp, fpi->SrcReg[0]);
			emit_arith(fp, PFS_OP_MAD, dest, mask,
				   src[0], pfs_one, pfs_zero, flags);
			break;
		case OPCODE_MUL:
			src[0] = t_src(fp, fpi->SrcReg[0]);
			src[1] = t_src(fp, fpi->SrcReg[1]);
			emit_arith(fp, PFS_OP_MAD, dest, mask,
				   src[0], src[1], pfs_zero, flags);
			break;
		case OPCODE_POW:
			src[0] = t_scalar_src(fp, fpi->SrcReg[0]);
			src[1] = t_scalar_src(fp, fpi->SrcReg[1]);
			temp[0] = get_temp_reg(fp);
			emit_arith(fp, PFS_OP_LG2, temp[0], WRITEMASK_W,
				   src[0], undef, undef, 0);
			emit_arith(fp, PFS_OP_MAD, temp[0], WRITEMASK_W,
				   temp[0], src[1], pfs_zero, 0);
			emit_arith(fp, PFS_OP_EX2, dest, fpi->DstReg.WriteMask,
				   temp[0], undef, undef, 0);
			free_temp(fp, temp[0]);
			break;
		case OPCODE_RCP:
			src[0] = t_scalar_src(fp, fpi->SrcReg[0]);
			emit_arith(fp, PFS_OP_RCP, dest, mask,
				   src[0], undef, undef, flags);
			break;
		case OPCODE_RSQ:
			src[0] = t_scalar_src(fp, fpi->SrcReg[0]);
			emit_arith(fp, PFS_OP_RSQ, dest, mask,
				   absolute(src[0]), pfs_zero, pfs_zero, flags);
			break;
		case OPCODE_SCS:
			/*
			 * scs using a parabola :
			 * scs(x):
			 *   result.x = sin(-abs(x)+0.5*PI)  (cos)
			 *   result.y = sin(x)               (sin)
			 *
			 */
			temp[0] = get_temp_reg(fp);
			temp[1] = get_temp_reg(fp);
			const_sin[0] = emit_const4fv(fp, SinCosConsts[0]);
			const_sin[1] = emit_const4fv(fp, SinCosConsts[1]);
			src[0] = t_scalar_src(fp, fpi->SrcReg[0]);

			/* x = -abs(x)+0.5*PI */
			emit_arith(fp, PFS_OP_MAD, temp[0], WRITEMASK_Z, swizzle(const_sin[0], Z, Z, Z, Z),	//PI
				   pfs_half,
				   negate(abs
					  (swizzle(keep(src[0]), X, X, X, X))),
				   0);

			/* C*x (sin) */
			emit_arith(fp, PFS_OP_MAD, temp[0], WRITEMASK_W,
				   swizzle(const_sin[0], Y, Y, Y, Y),
				   swizzle(keep(src[0]), X, X, X, X),
				   pfs_zero, 0);

			/* B*x, C*x (cos) */
			emit_arith(fp, PFS_OP_MAD, temp[0],
				   WRITEMASK_X | WRITEMASK_Y, swizzle(temp[0],
								      Z, Z, Z,
								      Z),
				   const_sin[0], pfs_zero, 0);

			/* B*x (sin) */
			emit_arith(fp, PFS_OP_MAD, temp[1], WRITEMASK_W,
				   swizzle(const_sin[0], X, X, X, X),
				   keep(src[0]), pfs_zero, 0);

			/* y = B*x + C*x*abs(x) (sin) */
			emit_arith(fp, PFS_OP_MAD, temp[1], WRITEMASK_Z,
				   absolute(src[0]),
				   swizzle(temp[0], W, W, W, W),
				   swizzle(temp[1], W, W, W, W), 0);

			/* y = B*x + C*x*abs(x) (cos) */
			emit_arith(fp, PFS_OP_MAD, temp[1], WRITEMASK_W,
				   swizzle(temp[0], Y, Y, Y, Y),
				   absolute(swizzle(temp[0], Z, Z, Z, Z)),
				   swizzle(temp[0], X, X, X, X), 0);

			/* y*abs(y) - y (cos), y*abs(y) - y (sin) */
			emit_arith(fp, PFS_OP_MAD, temp[0],
				   WRITEMASK_X | WRITEMASK_Y, swizzle(temp[1],
								      W, Z, Y,
								      X),
				   absolute(swizzle(temp[1], W, Z, Y, X)),
				   negate(swizzle(temp[1], W, Z, Y, X)), 0);

			/* dest.xy = mad(temp.xy, P, temp2.wz) */
			emit_arith(fp, PFS_OP_MAD, dest,
				   mask & (WRITEMASK_X | WRITEMASK_Y), temp[0],
				   swizzle(const_sin[0], W, W, W, W),
				   swizzle(temp[1], W, Z, Y, X), flags);

			free_temp(fp, temp[0]);
			free_temp(fp, temp[1]);
			break;
		case OPCODE_SGE:
			src[0] = t_src(fp, fpi->SrcReg[0]);
			src[1] = t_src(fp, fpi->SrcReg[1]);
			temp[0] = get_temp_reg(fp);
			/* temp = src0 - src1
			 * dest.c = (temp.c < 0.0) ? 0 : 1
			 */
			emit_arith(fp, PFS_OP_MAD, temp[0], mask,
				   src[0], pfs_one, negate(src[1]), 0);
			emit_arith(fp, PFS_OP_CMP, dest, mask,
				   pfs_one, pfs_zero, temp[0], 0);
			free_temp(fp, temp[0]);
			break;
		case OPCODE_SIN:
			/*
			 *  using a parabola:
			 * sin(x) = 4/pi * x + -4/(pi*pi) * x * abs(x)
			 * extra precision is obtained by weighting against
			 * itself squared.
			 */

			temp[0] = get_temp_reg(fp);
			const_sin[0] = emit_const4fv(fp, SinCosConsts[0]);
			const_sin[1] = emit_const4fv(fp, SinCosConsts[1]);
			src[0] = t_scalar_src(fp, fpi->SrcReg[0]);

			/* do range reduction */

			emit_arith(fp, PFS_OP_MAD, temp[0], WRITEMASK_X,
				   swizzle(keep(src[0]), X, X, X, X),
				   swizzle(const_sin[1], Z, Z, Z, Z),
				   pfs_half, 0);

			emit_arith(fp, PFS_OP_FRC, temp[0], WRITEMASK_X,
				   swizzle(temp[0], X, X, X, X),
				   undef, undef, 0);

			emit_arith(fp, PFS_OP_MAD, temp[0], WRITEMASK_Z, swizzle(temp[0], X, X, X, X), swizzle(const_sin[1], W, W, W, W),	//2*PI
				   negate(swizzle(const_sin[0], Z, Z, Z, Z)),	//PI
				   0);

			/* SIN */

			emit_arith(fp, PFS_OP_MAD, temp[0],
				   WRITEMASK_X | WRITEMASK_Y, swizzle(temp[0],
								      Z, Z, Z,
								      Z),
				   const_sin[0], pfs_zero, 0);

			emit_arith(fp, PFS_OP_MAD, temp[0], WRITEMASK_X,
				   swizzle(temp[0], Y, Y, Y, Y),
				   absolute(swizzle(temp[0], Z, Z, Z, Z)),
				   swizzle(temp[0], X, X, X, X), 0);

			emit_arith(fp, PFS_OP_MAD, temp[0], WRITEMASK_Y,
				   swizzle(temp[0], X, X, X, X),
				   absolute(swizzle(temp[0], X, X, X, X)),
				   negate(swizzle(temp[0], X, X, X, X)), 0);

			emit_arith(fp, PFS_OP_MAD, dest, mask,
				   swizzle(temp[0], Y, Y, Y, Y),
				   swizzle(const_sin[0], W, W, W, W),
				   swizzle(temp[0], X, X, X, X), flags);

			free_temp(fp, temp[0]);
			break;
		case OPCODE_SLT:
			src[0] = t_src(fp, fpi->SrcReg[0]);
			src[1] = t_src(fp, fpi->SrcReg[1]);
			temp[0] = get_temp_reg(fp);
			/* temp = src0 - src1
			 * dest.c = (temp.c < 0.0) ? 1 : 0
			 */
			emit_arith(fp, PFS_OP_MAD, temp[0], mask,
				   src[0], pfs_one, negate(src[1]), 0);
			emit_arith(fp, PFS_OP_CMP, dest, mask,
				   pfs_zero, pfs_one, temp[0], 0);
			free_temp(fp, temp[0]);
			break;
		case OPCODE_SUB:
			src[0] = t_src(fp, fpi->SrcReg[0]);
			src[1] = t_src(fp, fpi->SrcReg[1]);
			emit_arith(fp, PFS_OP_MAD, dest, mask,
				   src[0], pfs_one, negate(src[1]), flags);
			break;
		case OPCODE_TEX:
			emit_tex(fp, fpi, R300_FPITX_OP_TEX);
			break;
		case OPCODE_TXB:
			emit_tex(fp, fpi, R300_FPITX_OP_TXB);
			break;
		case OPCODE_TXP:
			emit_tex(fp, fpi, R300_FPITX_OP_TXP);
			break;
		case OPCODE_XPD:{
				src[0] = t_src(fp, fpi->SrcReg[0]);
				src[1] = t_src(fp, fpi->SrcReg[1]);
				temp[0] = get_temp_reg(fp);
				/* temp = src0.zxy * src1.yzx */
				emit_arith(fp, PFS_OP_MAD, temp[0],
					   WRITEMASK_XYZ, swizzle(keep(src[0]),
								  Z, X, Y, W),
					   swizzle(keep(src[1]), Y, Z, X, W),
					   pfs_zero, 0);
				/* dest.xyz = src0.yzx * src1.zxy - temp
				 * dest.w       = undefined
				 * */
				emit_arith(fp, PFS_OP_MAD, dest,
					   mask & WRITEMASK_XYZ, swizzle(src[0],
									 Y, Z,
									 X, W),
					   swizzle(src[1], Z, X, Y, W),
					   negate(temp[0]), flags);
				/* cleanup */
				free_temp(fp, temp[0]);
				break;
			}
		default:
			ERROR("unknown fpi->Opcode %d\n", fpi->Opcode);
			break;
		}

		if (fp->error)
			return GL_FALSE;

	}

	return GL_TRUE;
}

static void insert_wpos(struct gl_program *prog)
{
	static gl_state_index tokens[STATE_LENGTH] = {
		STATE_INTERNAL, STATE_R300_WINDOW_DIMENSION, 0, 0, 0
	};
	struct prog_instruction *fpi;
	GLuint window_index;
	int i = 0;
	GLuint tempregi = prog->NumTemporaries;
	/* should do something else if no temps left... */
	prog->NumTemporaries++;

	fpi = _mesa_alloc_instructions(prog->NumInstructions + 3);
	_mesa_init_instructions(fpi, prog->NumInstructions + 3);

	/* perspective divide */
	fpi[i].Opcode = OPCODE_RCP;

	fpi[i].DstReg.File = PROGRAM_TEMPORARY;
	fpi[i].DstReg.Index = tempregi;
	fpi[i].DstReg.WriteMask = WRITEMASK_W;
	fpi[i].DstReg.CondMask = COND_TR;

	fpi[i].SrcReg[0].File = PROGRAM_INPUT;
	fpi[i].SrcReg[0].Index = FRAG_ATTRIB_WPOS;
	fpi[i].SrcReg[0].Swizzle = SWIZZLE_WWWW;
	i++;

	fpi[i].Opcode = OPCODE_MUL;

	fpi[i].DstReg.File = PROGRAM_TEMPORARY;
	fpi[i].DstReg.Index = tempregi;
	fpi[i].DstReg.WriteMask = WRITEMASK_XYZ;
	fpi[i].DstReg.CondMask = COND_TR;

	fpi[i].SrcReg[0].File = PROGRAM_INPUT;
	fpi[i].SrcReg[0].Index = FRAG_ATTRIB_WPOS;
	fpi[i].SrcReg[0].Swizzle = SWIZZLE_XYZW;

	fpi[i].SrcReg[1].File = PROGRAM_TEMPORARY;
	fpi[i].SrcReg[1].Index = tempregi;
	fpi[i].SrcReg[1].Swizzle = SWIZZLE_WWWW;
	i++;

	/* viewport transformation */
	window_index = _mesa_add_state_reference(prog->Parameters, tokens);

	fpi[i].Opcode = OPCODE_MAD;

	fpi[i].DstReg.File = PROGRAM_TEMPORARY;
	fpi[i].DstReg.Index = tempregi;
	fpi[i].DstReg.WriteMask = WRITEMASK_XYZ;
	fpi[i].DstReg.CondMask = COND_TR;

	fpi[i].SrcReg[0].File = PROGRAM_TEMPORARY;
	fpi[i].SrcReg[0].Index = tempregi;
	fpi[i].SrcReg[0].Swizzle =
	    MAKE_SWIZZLE4(SWIZZLE_X, SWIZZLE_Y, SWIZZLE_Z, SWIZZLE_ZERO);

	fpi[i].SrcReg[1].File = PROGRAM_STATE_VAR;
	fpi[i].SrcReg[1].Index = window_index;
	fpi[i].SrcReg[1].Swizzle =
	    MAKE_SWIZZLE4(SWIZZLE_X, SWIZZLE_Y, SWIZZLE_Z, SWIZZLE_ZERO);

	fpi[i].SrcReg[2].File = PROGRAM_STATE_VAR;
	fpi[i].SrcReg[2].Index = window_index;
	fpi[i].SrcReg[2].Swizzle =
	    MAKE_SWIZZLE4(SWIZZLE_X, SWIZZLE_Y, SWIZZLE_Z, SWIZZLE_ZERO);
	i++;

	_mesa_copy_instructions(&fpi[i], prog->Instructions,
				prog->NumInstructions);

	free(prog->Instructions);

	prog->Instructions = fpi;

	prog->NumInstructions += i;
	fpi = &prog->Instructions[prog->NumInstructions - 1];

	assert(fpi->Opcode == OPCODE_END);

	for (fpi = &prog->Instructions[3]; fpi->Opcode != OPCODE_END; fpi++) {
		for (i = 0; i < 3; i++)
			if (fpi->SrcReg[i].File == PROGRAM_INPUT &&
			    fpi->SrcReg[i].Index == FRAG_ATTRIB_WPOS) {
				fpi->SrcReg[i].File = PROGRAM_TEMPORARY;
				fpi->SrcReg[i].Index = tempregi;
			}
	}
}

/* - Init structures
 * - Determine what hwregs each input corresponds to
 */
static void init_program(r300ContextPtr r300, struct r300_fragment_program *fp)
{
	struct r300_pfs_compile_state *cs = NULL;
	struct gl_fragment_program *mp = &fp->mesa_program;
	struct prog_instruction *fpi;
	GLuint InputsRead = mp->Base.InputsRead;
	GLuint temps_used = 0;	/* for fp->temps[] */
	int i, j;

	/* New compile, reset tracking data */
	fp->optimization =
	    driQueryOptioni(&r300->radeon.optionCache, "fp_optimization");
	fp->translated = GL_FALSE;
	fp->error = GL_FALSE;
	fp->cs = cs = &(R300_CONTEXT(fp->ctx)->state.pfs_compile);
	fp->tex.length = 0;
	fp->cur_node = 0;
	fp->first_node_has_tex = 0;
	fp->const_nr = 0;
	fp->max_temp_idx = 0;
	fp->node[0].alu_end = -1;
	fp->node[0].tex_end = -1;

	_mesa_memset(cs, 0, sizeof(*fp->cs));
	for (i = 0; i < PFS_MAX_ALU_INST; i++) {
		for (j = 0; j < 3; j++) {
			cs->slot[i].vsrc[j] = SRC_CONST;
			cs->slot[i].ssrc[j] = SRC_CONST;
		}
	}

	/* Work out what temps the Mesa inputs correspond to, this must match
	 * what setup_rs_unit does, which shouldn't be a problem as rs_unit
	 * configures itself based on the fragprog's InputsRead
	 *
	 * NOTE: this depends on get_hw_temp() allocating registers in order,
	 * starting from register 0.
	 */

	/* Texcoords come first */
	for (i = 0; i < fp->ctx->Const.MaxTextureUnits; i++) {
		if (InputsRead & (FRAG_BIT_TEX0 << i)) {
			cs->inputs[FRAG_ATTRIB_TEX0 + i].refcount = 0;
			cs->inputs[FRAG_ATTRIB_TEX0 + i].reg =
			    get_hw_temp(fp, 0);
		}
	}
	InputsRead &= ~FRAG_BITS_TEX_ANY;

	/* fragment position treated as a texcoord */
	if (InputsRead & FRAG_BIT_WPOS) {
		cs->inputs[FRAG_ATTRIB_WPOS].refcount = 0;
		cs->inputs[FRAG_ATTRIB_WPOS].reg = get_hw_temp(fp, 0);
		insert_wpos(&mp->Base);
	}
	InputsRead &= ~FRAG_BIT_WPOS;

	/* Then primary colour */
	if (InputsRead & FRAG_BIT_COL0) {
		cs->inputs[FRAG_ATTRIB_COL0].refcount = 0;
		cs->inputs[FRAG_ATTRIB_COL0].reg = get_hw_temp(fp, 0);
	}
	InputsRead &= ~FRAG_BIT_COL0;

	/* Secondary color */
	if (InputsRead & FRAG_BIT_COL1) {
		cs->inputs[FRAG_ATTRIB_COL1].refcount = 0;
		cs->inputs[FRAG_ATTRIB_COL1].reg = get_hw_temp(fp, 0);
	}
	InputsRead &= ~FRAG_BIT_COL1;

	/* Anything else */
	if (InputsRead) {
		WARN_ONCE("Don't know how to handle inputs 0x%x\n", InputsRead);
		/* force read from hwreg 0 for now */
		for (i = 0; i < 32; i++)
			if (InputsRead & (1 << i))
				cs->inputs[i].reg = 0;
	}

	/* Pre-parse the mesa program, grabbing refcounts on input/temp regs.
	 * That way, we can free up the reg when it's no longer needed
	 */
	if (!mp->Base.Instructions) {
		ERROR("No instructions found in program\n");
		return;
	}

	for (fpi = mp->Base.Instructions; fpi->Opcode != OPCODE_END; fpi++) {
		int idx;

		for (i = 0; i < 3; i++) {
			idx = fpi->SrcReg[i].Index;
			switch (fpi->SrcReg[i].File) {
			case PROGRAM_TEMPORARY:
				if (!(temps_used & (1 << idx))) {
					cs->temps[idx].reg = -1;
					cs->temps[idx].refcount = 1;
					temps_used |= (1 << idx);
				} else
					cs->temps[idx].refcount++;
				break;
			case PROGRAM_INPUT:
				cs->inputs[idx].refcount++;
				break;
			default:
				break;
			}
		}

		idx = fpi->DstReg.Index;
		if (fpi->DstReg.File == PROGRAM_TEMPORARY) {
			if (!(temps_used & (1 << idx))) {
				cs->temps[idx].reg = -1;
				cs->temps[idx].refcount = 1;
				temps_used |= (1 << idx);
			} else
				cs->temps[idx].refcount++;
		}
	}
	cs->temp_in_use = temps_used;
}

static void update_params(struct r300_fragment_program *fp)
{
	struct gl_fragment_program *mp = &fp->mesa_program;

	/* Ask Mesa nicely to fill in ParameterValues for us */
	if (mp->Base.Parameters)
		_mesa_load_state_parameters(fp->ctx, mp->Base.Parameters);
}

void r300TranslateFragmentShader(r300ContextPtr r300,
				 struct r300_fragment_program *fp)
{
	struct r300_pfs_compile_state *cs = NULL;

	if (!fp->translated) {

		init_program(r300, fp);
		cs = fp->cs;

		if (parse_program(fp) == GL_FALSE) {
			dump_program(fp);
			return;
		}

		/* Finish off */
		fp->node[fp->cur_node].alu_end =
		    cs->nrslots - fp->node[fp->cur_node].alu_offset - 1;
		if (fp->node[fp->cur_node].tex_end < 0)
			fp->node[fp->cur_node].tex_end = 0;
		fp->alu_offset = 0;
		fp->alu_end = cs->nrslots - 1;
		fp->tex_offset = 0;
		fp->tex_end = fp->tex.length ? fp->tex.length - 1 : 0;
		assert(fp->node[fp->cur_node].alu_end >= 0);
		assert(fp->alu_end >= 0);

		fp->translated = GL_TRUE;
		if (RADEON_DEBUG & DEBUG_PIXEL)
			dump_program(fp);
		r300UpdateStateParameters(fp->ctx, _NEW_PROGRAM);
	}

	update_params(fp);
}

/* just some random things... */
static void dump_program(struct r300_fragment_program *fp)
{
	int n, i, j;
	static int pc = 0;

	fprintf(stderr, "pc=%d*************************************\n", pc++);

	fprintf(stderr, "Mesa program:\n");
	fprintf(stderr, "-------------\n");
	_mesa_print_program(&fp->mesa_program.Base);
	fflush(stdout);

	fprintf(stderr, "Hardware program\n");
	fprintf(stderr, "----------------\n");

	for (n = 0; n < (fp->cur_node + 1); n++) {
		fprintf(stderr, "NODE %d: alu_offset: %d, tex_offset: %d, "
			"alu_end: %d, tex_end: %d\n", n,
			fp->node[n].alu_offset,
			fp->node[n].tex_offset,
			fp->node[n].alu_end, fp->node[n].tex_end);

		if (fp->tex.length) {
			fprintf(stderr, "  TEX:\n");
			for (i = fp->node[n].tex_offset;
			     i <= fp->node[n].tex_offset + fp->node[n].tex_end;
			     ++i) {
				const char *instr;

				switch ((fp->tex.
					 inst[i] >> R300_FPITX_OPCODE_SHIFT) &
					15) {
				case R300_FPITX_OP_TEX:
					instr = "TEX";
					break;
				case R300_FPITX_OP_KIL:
					instr = "KIL";
					break;
				case R300_FPITX_OP_TXP:
					instr = "TXP";
					break;
				case R300_FPITX_OP_TXB:
					instr = "TXB";
					break;
				default:
					instr = "UNKNOWN";
				}

				fprintf(stderr,
					"    %s t%i, %c%i, texture[%i]   (%08x)\n",
					instr,
					(fp->tex.
					 inst[i] >> R300_FPITX_DST_SHIFT) & 31,
					(fp->tex.
					 inst[i] & R300_FPITX_SRC_CONST) ? 'c' :
					't',
					(fp->tex.
					 inst[i] >> R300_FPITX_SRC_SHIFT) & 31,
					(fp->tex.
					 inst[i] & R300_FPITX_IMAGE_MASK) >>
					R300_FPITX_IMAGE_SHIFT,
					fp->tex.inst[i]);
			}
		}

		for (i = fp->node[n].alu_offset;
		     i <= fp->node[n].alu_offset + fp->node[n].alu_end; ++i) {
			char srcc[3][10], dstc[20];
			char srca[3][10], dsta[20];
			char argc[3][20];
			char arga[3][20];
			char flags[5], tmp[10];

			for (j = 0; j < 3; ++j) {
				int regc = fp->alu.inst[i].inst1 >> (j * 6);
				int rega = fp->alu.inst[i].inst3 >> (j * 6);

				sprintf(srcc[j], "%c%i",
					(regc & 32) ? 'c' : 't', regc & 31);
				sprintf(srca[j], "%c%i",
					(rega & 32) ? 'c' : 't', rega & 31);
			}

			dstc[0] = 0;
			sprintf(flags, "%s%s%s",
				(fp->alu.inst[i].
				 inst1 & R300_FPI1_DSTC_REG_X) ? "x" : "",
				(fp->alu.inst[i].
				 inst1 & R300_FPI1_DSTC_REG_Y) ? "y" : "",
				(fp->alu.inst[i].
				 inst1 & R300_FPI1_DSTC_REG_Z) ? "z" : "");
			if (flags[0] != 0) {
				sprintf(dstc, "t%i.%s ",
					(fp->alu.inst[i].
					 inst1 >> R300_FPI1_DSTC_SHIFT) & 31,
					flags);
			}
			sprintf(flags, "%s%s%s",
				(fp->alu.inst[i].
				 inst1 & R300_FPI1_DSTC_OUTPUT_X) ? "x" : "",
				(fp->alu.inst[i].
				 inst1 & R300_FPI1_DSTC_OUTPUT_Y) ? "y" : "",
				(fp->alu.inst[i].
				 inst1 & R300_FPI1_DSTC_OUTPUT_Z) ? "z" : "");
			if (flags[0] != 0) {
				sprintf(tmp, "o%i.%s",
					(fp->alu.inst[i].
					 inst1 >> R300_FPI1_DSTC_SHIFT) & 31,
					flags);
				strcat(dstc, tmp);
			}

			dsta[0] = 0;
			if (fp->alu.inst[i].inst3 & R300_FPI3_DSTA_REG) {
				sprintf(dsta, "t%i.w ",
					(fp->alu.inst[i].
					 inst3 >> R300_FPI3_DSTA_SHIFT) & 31);
			}
			if (fp->alu.inst[i].inst3 & R300_FPI3_DSTA_OUTPUT) {
				sprintf(tmp, "o%i.w ",
					(fp->alu.inst[i].
					 inst3 >> R300_FPI3_DSTA_SHIFT) & 31);
				strcat(dsta, tmp);
			}
			if (fp->alu.inst[i].inst3 & R300_FPI3_DSTA_DEPTH) {
				strcat(dsta, "Z");
			}

			fprintf(stderr,
				"%3i: xyz: %3s %3s %3s -> %-20s (%08x)\n"
				"       w: %3s %3s %3s -> %-20s (%08x)\n", i,
				srcc[0], srcc[1], srcc[2], dstc,
				fp->alu.inst[i].inst1, srca[0], srca[1],
				srca[2], dsta, fp->alu.inst[i].inst3);

			for (j = 0; j < 3; ++j) {
				int regc = fp->alu.inst[i].inst0 >> (j * 7);
				int rega = fp->alu.inst[i].inst2 >> (j * 7);
				int d;
				char buf[20];

				d = regc & 31;
				if (d < 12) {
					switch (d % 4) {
					case R300_FPI0_ARGC_SRC0C_XYZ:
						sprintf(buf, "%s.xyz",
							srcc[d / 4]);
						break;
					case R300_FPI0_ARGC_SRC0C_XXX:
						sprintf(buf, "%s.xxx",
							srcc[d / 4]);
						break;
					case R300_FPI0_ARGC_SRC0C_YYY:
						sprintf(buf, "%s.yyy",
							srcc[d / 4]);
						break;
					case R300_FPI0_ARGC_SRC0C_ZZZ:
						sprintf(buf, "%s.zzz",
							srcc[d / 4]);
						break;
					}
				} else if (d < 15) {
					sprintf(buf, "%s.www", srca[d - 12]);
				} else if (d == 20) {
					sprintf(buf, "0.0");
				} else if (d == 21) {
					sprintf(buf, "1.0");
				} else if (d == 22) {
					sprintf(buf, "0.5");
				} else if (d >= 23 && d < 32) {
					d -= 23;
					switch (d / 3) {
					case 0:
						sprintf(buf, "%s.yzx",
							srcc[d % 3]);
						break;
					case 1:
						sprintf(buf, "%s.zxy",
							srcc[d % 3]);
						break;
					case 2:
						sprintf(buf, "%s.Wzy",
							srcc[d % 3]);
						break;
					}
				} else {
					sprintf(buf, "%i", d);
				}

				sprintf(argc[j], "%s%s%s%s",
					(regc & 32) ? "-" : "",
					(regc & 64) ? "|" : "",
					buf, (regc & 64) ? "|" : "");

				d = rega & 31;
				if (d < 9) {
					sprintf(buf, "%s.%c", srcc[d / 3],
						'x' + (char)(d % 3));
				} else if (d < 12) {
					sprintf(buf, "%s.w", srca[d - 9]);
				} else if (d == 16) {
					sprintf(buf, "0.0");
				} else if (d == 17) {
					sprintf(buf, "1.0");
				} else if (d == 18) {
					sprintf(buf, "0.5");
				} else {
					sprintf(buf, "%i", d);
				}

				sprintf(arga[j], "%s%s%s%s",
					(rega & 32) ? "-" : "",
					(rega & 64) ? "|" : "",
					buf, (rega & 64) ? "|" : "");
			}

			fprintf(stderr, "     xyz: %8s %8s %8s    op: %08x\n"
				"       w: %8s %8s %8s    op: %08x\n",
				argc[0], argc[1], argc[2],
				fp->alu.inst[i].inst0, arga[0], arga[1],
				arga[2], fp->alu.inst[i].inst2);
		}
	}
}
