/*
 * Copyright (C) 2005 Jerome Glisse.  All Rights Reserved.
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
#include "r300_fragprog.h"
#include "r300_reg.h"


#define I0_000	( (R300_FPI0_OUTC_MAD) |				\
		  (R300_FPI0_ARGC_ZERO) |				\
		  (R300_FPI0_ARGC_ZERO << R300_FPI0_ARG1C_SHIFT) |	\
		  (R300_FPI0_ARGC_ZERO << R300_FPI0_ARG2C_SHIFT) )
#define I0_111	( (R300_FPI0_OUTC_MAD) |				\
		  (R300_FPI0_ARGC_ZERO) |				\
		  (R300_FPI0_ARGC_ZERO << R300_FPI0_ARG1C_SHIFT) |	\
		  (R300_FPI0_ARGC_ONE  << R300_FPI0_ARG2C_SHIFT) )
#define I0_XXX	( (R300_FPI0_OUTC_MAD) |				\
		  (R300_FPI0_ARGC_SRC0C_XXX) |				\
		  (R300_FPI0_ARGC_ONE  << R300_FPI0_ARG1C_SHIFT) |	\
		  (R300_FPI0_ARGC_ZERO << R300_FPI0_ARG2C_SHIFT) )
#define I0_YYY	( (R300_FPI0_OUTC_MAD) |				\
		  (R300_FPI0_ARGC_SRC0C_YYY) |				\
		  (R300_FPI0_ARGC_ONE  << R300_FPI0_ARG1C_SHIFT) |	\
		  (R300_FPI0_ARGC_ZERO << R300_FPI0_ARG2C_SHIFT) )
#define I0_ZZZ	( (R300_FPI0_OUTC_MAD) |				\
		  (R300_FPI0_ARGC_SRC0C_ZZZ) |				\
		  (R300_FPI0_ARGC_ONE  << R300_FPI0_ARG1C_SHIFT) |	\
		  (R300_FPI0_ARGC_ZERO << R300_FPI0_ARG2C_SHIFT) )
#define I0_XYZ	( (R300_FPI0_OUTC_MAD) |				\
		  (R300_FPI0_ARGC_SRC0C_XYZ) |				\
		  (R300_FPI0_ARGC_ONE  << R300_FPI0_ARG1C_SHIFT) |	\
		  (R300_FPI0_ARGC_ZERO << R300_FPI0_ARG2C_SHIFT) )
#define I0_YZX	( (R300_FPI0_OUTC_MAD) |				\
		  (R300_FPI0_ARGC_SRC0C_YZX) |				\
		  (R300_FPI0_ARGC_ONE  << R300_FPI0_ARG1C_SHIFT) |	\
		  (R300_FPI0_ARGC_ZERO << R300_FPI0_ARG2C_SHIFT) )
#define I0_ZXY	( (R300_FPI0_OUTC_MAD) |				\
		  (R300_FPI0_ARGC_SRC0C_ZXY) |				\
		  (R300_FPI0_ARGC_ONE  << R300_FPI0_ARG1C_SHIFT) |	\
		  (R300_FPI0_ARGC_ZERO << R300_FPI0_ARG2C_SHIFT) )
#define I0_WZY	( (R300_FPI0_OUTC_MAD) |				\
		  (R300_FPI0_ARGC_SRC0CA_WZY) |				\
		  (R300_FPI0_ARGC_ONE  << R300_FPI0_ARG1C_SHIFT) |	\
		  (R300_FPI0_ARGC_ZERO << R300_FPI0_ARG2C_SHIFT) )

#define IEMPTY	0
#define I1_CST	R300_FPI1_SRC0C_CONST

#define I1_XYZ	( R300_FPI1_SRC1C_CONST |	\
		  R300_FPI1_SRC2C_CONST |	\
		  R300_FPI1_DSTC_REG_X |	\
		  R300_FPI1_DSTC_REG_Y |	\
		  R300_FPI1_DSTC_REG_Z )
#define I1_XY_	( R300_FPI1_SRC1C_CONST |	\
		  R300_FPI1_SRC2C_CONST |	\
		  R300_FPI1_DSTC_REG_X |	\
		  R300_FPI1_DSTC_REG_Y )	
#define I1_X_Z	( R300_FPI1_SRC1C_CONST |	\
		  R300_FPI1_SRC2C_CONST |	\
		  R300_FPI1_DSTC_REG_X |	\
		  R300_FPI1_DSTC_REG_Z )	
#define I1__YZ	( R300_FPI1_SRC1C_CONST |	\
		  R300_FPI1_SRC2C_CONST |	\
		  R300_FPI1_DSTC_REG_Y |	\
		  R300_FPI1_DSTC_REG_Z )
#define I1_X__	( R300_FPI1_SRC1C_CONST |	\
		  R300_FPI1_SRC2C_CONST |	\
		  R300_FPI1_DSTC_REG_X )
#define I1__Y_	( R300_FPI1_SRC1C_CONST |	\
		  R300_FPI1_SRC2C_CONST |	\
		  R300_FPI1_DSTC_REG_Y )
#define I1___Z	( R300_FPI1_SRC1C_CONST |	\
		  R300_FPI1_SRC2C_CONST |	\
		  R300_FPI1_DSTC_REG_Z )

#define SEMPTY	{0,{0,0,0,0},{0,0,0,0,0,0,0,0}}

struct r300_fragment_program_swizzle r300_swizzle [512] = {
	/* XXX */
	{1,{0,0,0,0},{ I0_XXX, I1_XYZ,
		       0, 0, 0, 0, 0, 0 } },
	/* YXX */
	{2,{0,0,0,0},{ I0_YYY, I1_X__,
		       I0_XXX, I1__YZ,
		       0,0,
		       0,0 } },
	/* ZXX */
	{2,{0,0,0,0},{ I0_ZZZ, I1_X__,
		       I0_XXX, I1__YZ,
		       0,0,
		       0,0 } },
	/* WXX */
	{2,{0,0,0,0},{ I0_WZY, I1_X__,
		       I0_XXX, I1__YZ,
		       0,0,
		       0,0} },
	/* 0XX */
	{2,{0,2,0,0},{ I0_XXX, I1__YZ,
		       I0_000, I1_X__ | I1_CST,
		       0,0,
		       0,0 } },
	/* 1XX */
	{2,{0,2,0,0},{ I0_XXX, I1__YZ,
		       I0_111, I1_X__ | I1_CST,
		       0,0,0,0}},
	SEMPTY,SEMPTY,
	/* XYX */
	{2,{0,0,0,0},{ I0_YYY, I1__Y_,
		       I0_XXX, I1_X_Z,
		       0,0,0,0}},
	/* YYX */
	{2,{0,0,0,0},{ I0_YYY, I1_XY_,
		       I0_XXX, I1___Z,
		       0,0,0,0}},
	/* ZYX */
	{3,{0,0,0,0},{ I0_ZZZ, I1_X__,
		       I0_YYY, I1__Y_,
		       I0_XXX, I1___Z,
		       0,0}},
	/* WYX */
	{3,{0,0,0,0},{ I0_WZY, I1_X__,
		       I0_YYY, I1__Y_,
		       I0_XXX, I1___Z,
		       0,0}},
	/* 0YX */
	{3,{0,0,2,0},{ I0_YYY, I1__Y_,
		       I0_XXX, I1___Z,
	               I0_000, I1_X__ | I1_CST,
		       0,0}},
	/* 1YX */
	{3,{0,0,2,0},{ I0_YYY, I1__Y_,
		       I0_XXX, I1___Z,
		       I0_111, I1_X__ | I1_CST,
		       0,0}},
	SEMPTY,SEMPTY,
	/* XZX */
	{2,{0,0,0,0},{ I0_YZX, I1__YZ,
		       I0_XXX, I1_X__,
		       0,0,0,0}},
	/* YZX */
	{1,{0,0,0,0},{ I0_YZX, I1_XYZ,
		       0, 0, 0, 0, 0, 0 } },
	/* ZZX */
	{2,{0,0,0,0},{ I0_YZX, I1__YZ,
		       I0_ZZZ, I1_X__,0,0,0,0}},
	/* WZX */
	{2,{0,0,0,0},{ I0_WZY, I1__YZ,
		       I0_XXX, I1_X__,0,0,0,0}},
	/* 0ZX */
	{2,{0,2,0,0},{ I0_YZX, I1__YZ,
		       I0_000, I1_X__ | I1_CST,
	               0,0,0,0}},
	/* 1ZX */
	{2,{0,2,0,0},{ I0_YZX, I1__YZ,
		       I0_111, I1_X__ | I1_CST,
	               0,0,0,0}},
	SEMPTY,SEMPTY,
	/* XWX */
	{3,{0,1,0,0},{ I0_WZY, I1_X__,
		       I0_XXX, I1__Y_,
		       I0_XXX, I1_X_Z,
		       0,0}},
	/* YWX */
	{3,{0,1,0,0},{ I0_WZY, I1_X__,
		       I0_XXX, I1__Y_,
		       I0_YZX, I1_X_Z,
		       0,0}},
	/* ZWX */
	{4,{0,1,0,0},{ I0_WZY, I1_X__,
		       I0_XXX, I1__Y_,
		       I0_ZZZ, I1_X__,
		       I0_XXX, I1___Z } },
	/* WWX */
	{3,{0,1,0,0},{ I0_WZY, I1_X__,
		       I0_XXX, I1_XY_,
		       I0_YZX, I1___Z,
		       0,0}},
	/* 0WX */
	{4,{0,1,0,2},{ I0_WZY, I1_X__,
		       I0_XXX, I1__Y_,
		       I0_YZX, I1___Z,
		       I0_000, I1_X__ | I1_CST } },
	/* 1WX */
	{4,{0,1,0,2},{ I0_WZY, I1_X__,
		       I0_XXX, I1__Y_,
		       I0_YZX, I1___Z,
		       I0_111, I1_X__ | I1_CST } },
	SEMPTY,SEMPTY,
	/* X0X */
	{2,{0,2,0,0},{ I0_XXX, I1_X_Z,
		       I0_000, I1__Y_ | I1_CST,
		       0,0,0,0}},
	/* Y0X */
	{2,{0,2,0,0},{ I0_YZX, I1_X_Z,
		       I0_000, I1__Y_ | I1_CST,
		       0,0,0,0}},
	/* Z0X */
	{3,{0,2,0,0},{ I0_XXX, I1___Z,
		       I0_000, I1__Y_ | I1_CST,
		       I0_ZZZ, I1_X__,
		       0,0}},
	/* W0X */
	{3,{0,2,0,0},{ I0_XXX, I1___Z,
		       I0_000, I1__Y_ | I1_CST,
		       I0_WZY, I1_X__,
		       0,0}},
	/* 00X */
	{2,{0,2,0,0},{ I0_XXX, I1___Z,
		       I0_000, I1_XY_ | I1_CST,
		       0,0,0,0}},
	/* 10X */
	{3,{0,2,0,0},{ I0_XXX, I1___Z,
		       I0_000, I1__Y_ | I1_CST,
		       I0_111, I1_X__ | I1_CST,
		       0,0}},
	SEMPTY,SEMPTY,
	/* X1X */
	{2,{0,2,0,0},{ I0_XXX, I1_X_Z,
		       I0_111, I1__Y_ | I1_CST,
		       0,0,0,0}},
	/* Y1X */
	{2,{0,2,0,0},{ I0_YZX, I1_X_Z,
		       I0_111, I1__Y_ | I1_CST,
		       0,0,0,0}},
	/* Z1X */
	{3,{0,2,0,0},{ I0_XXX, I1___Z,
		       I0_111, I1__Y_ | I1_CST,
		       I0_ZZZ, I1_X__,
		       0,0}},
	/* W1X */
	{3,{0,2,0,0},{ I0_XXX, I1___Z,
		       I0_111, I1__Y_ | I1_CST,
		       I0_WZY, I1_X__,
		       0,0}},
	/* 01X */
	{3,{0,2,0,0},{ I0_XXX, I1___Z,
		       I0_111, I1__Y_ | I1_CST,
		       I0_000, I1_X__ | I1_CST,
		       0,0}},
	/* 11X */
	{2,{0,2,0,0},{ I0_XXX, I1___Z,
		       I0_111, I1_XY_ | I1_CST,
		       0,0,0,0}},
	SEMPTY,SEMPTY,SEMPTY,SEMPTY,SEMPTY,SEMPTY,SEMPTY,SEMPTY,SEMPTY,SEMPTY,
	SEMPTY,SEMPTY,SEMPTY,SEMPTY,SEMPTY,SEMPTY,SEMPTY,SEMPTY,
	/* XXY */
	{2,{0,0,0,0},{ I0_YYY, I1___Z,
		       I0_XXX, I1_XY_,
		       0,0,0,0}},
	/* YXY */
	{2,{0,0,0,0},{ I0_YYY, I1_X_Z,
		       I0_XXX, I1__Y_,
		       0,0,0,0}},
	/* ZXY */
	{1,{0,0,0,0},{ I0_ZXY, I1_XYZ,
		       0, 0, 0, 0, 0, 0 } },
	/* WXY */
	{2,{0,0,0,0},{ I0_WZY, I1_X__,
		       I0_ZXY, I1__YZ,
		       0,0,0,0}},
	/* 0XY */
	{2,{0,0,0,0},{ I0_ZXY, I1__YZ,
		       I0_000, I1_X__ | I1_CST,
		       0,0,0,0}},
	/* 1XY */
	{2,{0,0,0,0},{ I0_ZXY, I1__YZ,
		       I0_111, I1_X__ | I1_CST,
		       0,0,0,0}},
	SEMPTY,SEMPTY,
	/* XYY */
	{2,{0,0,0,0},{ I0_YYY, I1__YZ,
		       I0_XXX, I1_X__,
		       0,0,0,0}},
	/* YYY */
	{1,{0,0,0,0},{ I0_YYY, I1_XYZ,
		       0, 0, 0, 0, 0, 0 } },
	/* ZYY */
	{2,{0,0,0,0},{ I0_YYY, I1__YZ,
		       I0_ZZZ, I1_X__,
		       0,0,0,0}},
	/* WYY */
	{2,{0,0,0,0},{ I0_YYY, I1__YZ,
		       I0_WZY, I1_X__,
		       0,0,0,0}},
	/* 0YY */
	{2,{0,0,0,0},{ I0_YYY, I1__YZ,
		       I0_000, I1_X__ | I1_CST,
		       0,0,0,0}},
	/* 1YY */
	{2,{0,0,0,0},{ I0_YYY, I1__YZ,
		       I0_111, I1_X__ | I1_CST,
		       0,0,0,0}},
	SEMPTY,SEMPTY,
	/* XZY */
	{2,{0,0,0,0},{ I0_WZY, I1__YZ,
		       I0_XXX, I1_X__,
		       0,0,0,0}},
	/* YZY */
	{2,{0,0,0,0},{ I0_WZY, I1__YZ,
		       I0_YYY, I1_X__,
		       0,0,0,0}},
	/* ZZY */
	{2,{0,0,0,0},{ I0_WZY, I1__YZ,
		       I0_ZZZ, I1_X__,
		       0,0,0,0}},
	/* WZY */
	{1,{0,0,0,0},{ I0_WZY, I1_XYZ,
		       0, 0, 0, 0, 0, 0 } },
	/* 0ZY */
	{2,{0,0,0,0},{ I0_WZY, I1__YZ,
		       I0_000, I1_X__ | I1_CST,
		       0,0,0,0}},
	/* 1ZY */
	{2,{0,0,0,0},{ I0_WZY, I1__YZ,
		       I0_111, I1_X__ | I1_CST,
		       0,0,0,0}},
	SEMPTY,SEMPTY,
	/* XWY */
	{4,{0,1,0,0},{ I0_WZY, I1_X__,
		       I0_XXX, I1__Y_,
		       I0_XXX, I1_X__,
		       I0_YYY, I1___Z }	},
	/* YWY */
	{3,{0,1,0,0},{ I0_WZY, I1_X__,
		       I0_XXX, I1__Y_,
		       I0_YYY, I1_X_Z,
		       0,0}},
	/* ZWY */
	{3,{0,1,0,0},{ I0_WZY, I1_X__,
		       I0_XXX, I1__Y_,
		       I0_ZXY, I1_X_Z,
		       0,0}},
	/* WWY */
	{3,{0,1,0,0},{ I0_WZY, I1_X__,
		       I0_XXX, I1_XY_,
		       I0_ZXY, I1___Z,
		       0,0}},
	/* 0WY */
	{4,{0,1,0,0},{ I0_WZY, I1_X__,
		       I0_XXX, I1__Y_,
		       I0_ZXY, I1___Z,
		       I0_000, I1_X__ | I1_CST } },
	/* 1WY */
	{4,{0,1,0,0},{ I0_WZY, I1_X__,
		       I0_XXX, I1__Y_,
		       I0_ZXY, I1___Z,
		       I0_111, I1_X__ | I1_CST } },
	SEMPTY,SEMPTY,
	/* X0Y */
	{3,{0,2,0,0},{ I0_XXX, I1_X__,
		       I0_000, I1__Y_ | I1_CST,
		       I0_YYY, I1___Z,
		       0,0}},
	/* Y0Y */
	{2,{0,2,0,0},{ I0_YYY, I1_X_Z,
		       I0_000, I1__Y_ | I1_CST,
		       0,0,0,0}},
	/* Z0Y */
	{2,{0,2,0,0},{ I0_ZXY, I1_X_Z,
		       I0_000, I1__Y_ | I1_CST,
		       0,0,0,0}},
	/* W0Y */
	{2,{0,2,0,0},{ I0_WZY, I1_X_Z,
		       I0_000, I1__Y_ | I1_CST,
		       0,0,0,0}},
	/* 00Y */
	{2,{0,2,0,0},{ I0_YYY, I1___Z,
		       I0_000, I1_XY_ | I1_CST,
		       0,0,0,0}},
	/* 10Y */
	{3,{0,2,0,0},{ I0_YYY, I1___Z,
		       I0_000, I1__Y_ | I1_CST,
		       I0_111, I1_X__ | I1_CST,
		       0,0}},
	SEMPTY,SEMPTY,
	/* X1Y */
	{3,{0,2,0,0},{ I0_XXX, I1_X__,
		       I0_111, I1__Y_ | I1_CST,
		       I0_YYY, I1___Z,
		       0,0}},
	/* Y1Y */
	{2,{0,2,0,0},{ I0_YYY, I1_X_Z,
		       I0_111, I1__Y_ | I1_CST,
		       0,0,0,0}},
	/* Z1Y */
	{2,{0,2,0,0},{ I0_ZXY, I1_X_Z,
		       I0_111, I1__Y_ | I1_CST,
		       0,0,0,0}},
	/* W1Y */
	{3,{0,2,0,0},{ I0_WZY, I1_X_Z,
		       I0_111, I1__Y_ | I1_CST,
		       0,0,0,0}},
	/* 01Y */
	{3,{0,2,0,0},{ I0_YYY, I1___Z,
		       I0_111, I1__Y_ | I1_CST,
		       I0_000, I1_X__ | I1_CST,
		       0,0}},
	/* 11Y */
	{2,{0,2,0,0},{ I0_YYY, I1___Z,
		       I0_111, I1_XY_ | I1_CST,
		       0,0,0,0}},
	SEMPTY,SEMPTY,SEMPTY,SEMPTY,SEMPTY,SEMPTY,SEMPTY,SEMPTY,SEMPTY,SEMPTY,
	SEMPTY,SEMPTY,SEMPTY,SEMPTY,SEMPTY,SEMPTY,SEMPTY,SEMPTY,
	/* XXZ */
	{2,{0,0,0,0},{ I0_XXX, I1_XY_,
		       I0_ZZZ, I1___Z,
		       0,0,0,0}},
	/* YXZ */
	{3,{0,0,0,0},{ I0_XXX, I1__Y_,
		       I0_YYY, I1_X__,
		       I0_ZZZ, I1___Z,
		       0,0}},
	/* ZXZ */
	{2,{0,0,0,0},{ I0_XXX, I1__Y_,
		       I0_ZZZ, I1_X_Z,
		       0,0,0,0}},
	/* WXZ */
	{3,{0,0,0,0},{ I0_XXX, I1__Y_,
		       I0_ZZZ, I1___Z,
		       I0_WZY, I1_X__,
		       0,0}},
	/* 0XZ */
	{3,{0,0,2,0},{ I0_XXX, I1__Y_,
		       I0_ZZZ, I1___Z,
		       I0_000, I1_X__ | I1_CST,
		       0,0}},
	/* 1XZ */
	{3,{0,0,2,0},{ I0_XXX, I1__Y_,
		       I0_ZZZ, I1___Z,
		       I0_111, I1_X__ | I1_CST,
		       0,0}},
	SEMPTY,SEMPTY,
	/* XYZ */
	{1,{0,0,0,0},{ I0_XYZ, I1_XYZ,
		       0, 0, 0, 0, 0, 0 } },
	/* YYZ */
	{2,{0,0,0,0},{ I0_ZZZ, I1___Z,
		       I0_YYY, I1_XY_,
		       0,0,0,0}},
	/* ZYZ */
	{2,{0,0,0,0},{ I0_ZZZ, I1_X_Z,
		       I0_YYY, I1__Y_,
		       0,0,0,0}},
	/* WYZ */
	{2,{0,0,0,0},{ I0_XYZ, I1__YZ,
		       I0_WZY, I1_X__,
		       0,0,0,0}},
	/* 0YZ */
	{2,{0,2,0,0},{ I0_XYZ, I1__YZ,
		       I0_000, I1_X__ | I1_CST,
		       0,0,0,0}},
	/* 1YZ */
	{2,{0,2,0,0},{ I0_XYZ, I1__YZ,
		       I0_111, I1_X__ | I1_CST,
		       0,0,0,0}},
	SEMPTY,SEMPTY,
	/* XZZ */
	{2,{0,0,0,0},{ I0_ZZZ, I1__YZ,
		       I0_XXX, I1_X__,
		       0,0,0,0}},
	/* YZZ */
	{2,{0,0,0,0},{ I0_ZZZ, I1__YZ,
		       I0_YYY, I1_X__,
		       0,0,0,0}},
	/* ZZZ */
	{1,{0,0,0,0},{ I0_ZZZ, I1_XYZ,
		       0, 0, 0, 0, 0, 0 } },
	/* WZZ */
	{2,{0,0,0,0},{ I0_ZZZ, I1__YZ,
		       I0_WZY, I1_X__,
		       0,0,0,0}},
	/* 0ZZ */
	{2,{0,2,0,0},{ I0_ZZZ, I1__YZ,
		       I0_000, I1_X__ | I1_CST,
		       0,0,0,0}},
	/* 1ZZ */
	{2,{0,2,0,0},{ I0_ZZZ, I1__YZ,
		       I0_111, I1_X__ | I1_CST,
		       0,0,0,0}},
	SEMPTY,SEMPTY,
	/* XWZ */
	{3,{0,1,0,0},{ I0_WZY, I1_X__,
		       I0_XXX, I1__Y_,
		       I0_XYZ, I1_X_Z,
		       0,0}},
	/* YWZ */
	{4,{0,1,0,0},{ I0_WZY, I1_X__,
		       I0_XXX, I1__Y_,
		       I0_YYY, I1_X__,
		       I0_XYZ, I1___Z } },
	/* ZWZ */
	{3,{0,1,0,0},{ I0_WZY, I1_X__,
		       I0_XXX, I1__Y_,
		       I0_ZZZ, I1_X_Z,
		       0,0}},
	/* WWZ */
	{3,{0,1,0,0},{ I0_WZY, I1_X__,
		       I0_XXX, I1_XY_,
		       I0_XYZ, I1___Z,
		       0,0}},
	/* 0WZ */
	{4,{0,1,0,2},{ I0_WZY, I1_X__,
		       I0_XXX, I1__Y_,
		       I0_XYZ, I1___Z,
		       I0_000, I1_X__ | I1_CST } },
	/* 1WZ */
	{4,{0,1,0,2},{ I0_WZY, I1_X__,
		       I0_XXX, I1__Y_,
		       I0_XYZ, I1___Z,
		       I0_111, I1_X__ | I1_CST } },
	SEMPTY,SEMPTY,
	/* X0Z */
	{2,{0,2,0,0},{ I0_XYZ, I1_X_Z,
		       I0_000, I1__Y_ | I1_CST,
		       0,0,0,0}},
	/* Y0Z */
	{3,{0,2,0,0},{ I0_ZZZ, I1___Z,
		       I0_000, I1__Y_ | I1_CST,
		       I0_YYY, I1_X__,
		       0,0}},
	/* Z0Z */
	{2,{0,2,0,0},{ I0_ZZZ, I1_X_Z,
		       I0_000, I1__Y_ | I1_CST,
		       0,0,0,0}},
	/* W0Z */
	{3,{0,2,0,0},{ I0_ZZZ, I1___Z,
		       I0_000, I1__Y_ | I1_CST,
		       I0_WZY, I1_X__,
		       0,0}},
	/* 00Z */
	{2,{0,2,0,0},{ I0_ZZZ, I1___Z,
		       I0_000, I1_XY_ | I1_CST,
		       0,0,0,0}},
	/* 10Z */
	{3,{0,2,2,0},{ I0_ZZZ, I1___Z,
		       I0_000, I1__Y_ | I1_CST,
		       I0_111, I1_X__ | I1_CST,
		       0,0}},
	SEMPTY,SEMPTY,
	/* X1Z */
	{2,{0,2,0,0},{ I0_XYZ, I1_X_Z,
		       I0_111, I1__Y_ | I1_CST,
		       0,0,0,0}},
	/* Y1Z */
	{3,{0,2,0,0},{ I0_ZZZ, I1___Z,
		       I0_111, I1__Y_ | I1_CST,
		       I0_YYY, I1_X__,
		       0,0}},
	/* Z1Z */
	{2,{0,2,0,0},{ I0_ZZZ, I1_X_Z,
		       I0_111, I1__Y_ | I1_CST,
		       0,0,0,0}},
	/* W1Z */
	{3,{0,2,0,0},{ I0_ZZZ, I1___Z,
		       I0_111, I1__Y_ | I1_CST,
		       I0_WZY, I1_X__,
		       0,0}},
	/* 01Z */
	{3,{0,2,2,0},{ I0_ZZZ, I1___Z,
		       I0_111, I1__Y_ | I1_CST,
		       I0_000, I1_X__ | I1_CST,
		       0,0}},
	/* 11Z */
	{2,{0,2,0,0},{ I0_ZZZ, I1___Z,
		       I0_111, I1_XY_ | I1_CST,
		       0,0,0,0}},
	SEMPTY,SEMPTY,SEMPTY,SEMPTY,SEMPTY,SEMPTY,SEMPTY,SEMPTY,SEMPTY,SEMPTY,
	SEMPTY,SEMPTY,SEMPTY,SEMPTY,SEMPTY,SEMPTY,SEMPTY,SEMPTY,
	/* XXW */
	{3,{0,1,0,0},{ I0_WZY, I1_X__,
		       I0_XXX, I1___Z,
		       I0_XXX, I1_XY_,
		       0,0}},
	/* YXW */
	{4,{0,1,0,0},{ I0_WZY, I1_X__,
		       I0_XXX, I1___Z,
		       I0_XXX, I1__Y_,
		       I0_YYY, I1_X__ } },
	/* ZXW */
	{3,{0,1,0,0},{ I0_WZY, I1_X__,
		       I0_XXX, I1___Z,
		       I0_ZXY, I1_XY_,
		       0,0}},
	/* WXW */
	{3,{0,1,0,0},{ I0_WZY, I1_X__,
		       I0_XXX, I1_X_Z,
		       I0_XXX, I1__Y_,
		       0,0}},
	/* 0XW */
	{4,{0,1,0,2},{ I0_WZY, I1_X__,
		       I0_XXX, I1___Z,
		       I0_XXX, I1__Y_,
		       I0_000, I1_X__ | I1_CST } },
	/* 1XW */
	{4,{0,1,0,2},{ I0_WZY, I1_X__,
		       I0_XXX, I1___Z,
		       I0_XXX, I1__Y_,
		       I0_111, I1_X__ | I1_CST } },
	SEMPTY,SEMPTY,
	/* XYW */
	{3,{0,1,0,0},{ I0_WZY, I1_X__,
		       I0_XXX, I1___Z,
		       I0_XYZ, I1_XY_,
		       0,0}},
	/* YYW */
	{3,{0,1,0,0},{ I0_WZY, I1_X__,
		       I0_XXX, I1___Z,
		       I0_YYY, I1_XY_,
		       0,0}},
	/* ZYW */
	{4,{0,1,0,0},{ I0_WZY, I1_X__,
		       I0_XXX, I1___Z,
		       I0_XYZ, I1__Y_,
		       I0_ZZZ, I1_X__ } },
	/* WYW */
	{3,{0,1,0,0},{ I0_WZY, I1_X__,
		       I0_XXX, I1_X_Z,
		       I0_YYY, I1__Y_,
		       0,0}},
	/* 0YW */
	{4,{0,1,0,2},{ I0_WZY, I1_X__,
		       I0_XXX, I1___Z,
		       I0_YYY, I1__Y_,
		       I0_000, I1_X__ | I1_CST } },
	/* 1YW */
	{4,{0,1,0,2},{ I0_WZY, I1_X__,
		       I0_XXX, I1___Z,
		       I0_YYY, I1__Y_,
		       I0_111, I1_X__ | I1_CST } },

	SEMPTY,SEMPTY,
	/* XZW */
	{4,{0,1,0,0},{ I0_WZY, I1_X__,
		       I0_XXX, I1___Z,
		       I0_XYZ, I1_X__,
		       I0_ZZZ, I1__Y_ } },
	/* YZW */
	{3,{0,1,0,0},{ I0_WZY, I1_X__,
		       I0_XXX, I1___Z,
		       I0_YZX, I1_XY_,
		       0,0}},
	/* ZZW */
	{3,{0,1,0,0},{ I0_WZY, I1_X__,
		       I0_XXX, I1___Z,
		       I0_ZZZ, I1_XY_,
		       0,0}},
	/* WZW */
	{3,{0,1,0,0},{ I0_WZY, I1_X__,
		       I0_XXX, I1_X_Z,
		       I0_ZZZ, I1__Y_,
		       0,0}},
	/* 0ZW */
	{4,{0,1,0,2},{ I0_WZY, I1_X__,
		       I0_XXX, I1___Z,
		       I0_ZZZ, I1__Y_,
		       I0_000, I1_X__ | I1_CST } },
	/* 1ZW */
	{4,{0,1,0,2},{ I0_WZY, I1_X__,
		       I0_XXX, I1___Z,
		       I0_ZZZ, I1__Y_,
		       I0_111, I1_X__ | I1_CST } },

	SEMPTY,SEMPTY,
	/* XWW */
	{3,{0,1,0,0},{ I0_WZY, I1_X__,
		       I0_XXX, I1__YZ,
		       I0_XYZ, I1_X__,
		       0,0}},
	/* YWW */
	{3,{0,1,0,0},{ I0_WZY, I1_X__,
		       I0_XXX, I1__YZ,
		       I0_YYY, I1_X__,
		       0,0}},
	/* ZWW */
	{3,{0,1,0,0},{ I0_WZY, I1_X__,
		       I0_XXX, I1__YZ,
		       I0_ZZZ, I1_X__,
		       0,0}},
	/* WWW */
	{2,{0,1,0,0},{ I0_WZY, I1_X__,
		       I0_XXX, I1_XYZ,
		       0,0,0,0}},
	/* 0WW */
	{3,{0,1,2,0},{ I0_WZY, I1_X__,
		       I0_XXX, I1__YZ,
		       I0_000, I1_X__ | I1_CST,
		       0,0}},
	/* 1WW */
	{3,{0,1,2,0},{ I0_WZY, I1_X__,
		       I0_XXX, I1__YZ,
		       I0_111, I1_X__ | I1_CST,
		       0,0}},
	SEMPTY,SEMPTY,
	/* X0W */
	{4,{0,1,0,2},{ I0_WZY, I1_X__,
		       I0_XXX, I1___Z,
		       I0_XYZ, I1_X__,
		       I0_000, I1__Y_ | I1_CST } },
	/* Y0W */
	{4,{0,1,0,2},{ I0_WZY, I1_X__,
		       I0_XXX, I1___Z,
		       I0_YYY, I1_X__,
		       I0_000, I1__Y_ | I1_CST } },
	/* Z0W */
	{4,{0,1,0,2},{ I0_WZY, I1_X__,
		       I0_XXX, I1___Z,
		       I0_ZZZ, I1_X__,
		       I0_000, I1__Y_ | I1_CST } },
	/* 00W */
	{3,{0,1,0,2},{ I0_WZY, I1_X__,
		       I0_XXX, I1___Z,
		       I0_000, I1_XY_ | I1_CST,
		       0,0}},
	/* 10W */
	{4,{0,1,2,2},{ I0_WZY, I1_X__,
		       I0_XXX, I1___Z,
		       I0_111, I1_X__ | I1_CST,
		       I0_000, I1__Y_ | I1_CST } },
	SEMPTY,SEMPTY,
	/* X1W */
	{4,{0,1,0,2},{ I0_WZY, I1_X__,
		       I0_XXX, I1___Z,
		       I0_XYZ, I1_X__,
		       I0_111, I1__Y_ | I1_CST } },
	/* Y1W */
	{4,{0,1,0,2},{ I0_WZY, I1_X__,
		       I0_XXX, I1___Z,
		       I0_YYY, I1_X__,
		       I0_111, I1__Y_ | I1_CST } },
	/* Z1W */
	{4,{0,1,0,2},{ I0_WZY, I1_X__,
		       I0_XXX, I1___Z,
		       I0_ZZZ, I1_X__,
		       I0_111, I1__Y_ | I1_CST } },
	/* 01W */
	{4,{0,1,2,2},{ I0_WZY, I1_X__,
		       I0_XXX, I1___Z,
		       I0_000, I1_X__ | I1_CST,
		       I0_111, I1__Y_ | I1_CST } },
	/* 11W */
	{3,{0,1,0,2},{ I0_WZY, I1_X__,
		       I0_XXX, I1___Z,
		       I0_111, I1_XY_ | I1_CST,
		       0,0}},
	SEMPTY,SEMPTY,SEMPTY,SEMPTY,SEMPTY,SEMPTY,SEMPTY,SEMPTY,SEMPTY,SEMPTY,
	SEMPTY,SEMPTY,SEMPTY,SEMPTY,SEMPTY,SEMPTY,SEMPTY,SEMPTY,
	/* XX0 */
	{2,{2,0,0,0},{ I0_000, I1___Z | I1_CST,
		       I0_XXX, I1_XY_,
		       0,0,0,0}},
	/* YX0 */
	{3,{2,0,0,0},{ I0_000, I1___Z | I1_CST,
		       I0_XXX, I1__Y_,
		       I0_YYY, I1_X__,
		       0,0}},
	/* ZX0 */
	{2,{2,0,0,0},{ I0_000, I1___Z | I1_CST,
		       I0_ZXY, I1_XY_,
		       0,0,0,0}},
	/* WX0 */
	{3,{2,0,0,0},{ I0_000, I1___Z | I1_CST,
		       I0_XXX, I1__Y_,
		       I0_WZY, I1_X__,
		       0,0}},
	/* 0X0 */
	{2,{2,0,0,0},{ I0_000, I1_X_Z | I1_CST,
		       I0_XXX, I1__Y_,
		       0,0,0,0}},
	/* 1X0 */
	{3,{2,0,2,0},{ I0_000, I1___Z | I1_CST,
		       I0_XXX, I1__Y_,
		       I0_111, I1_X__ | I1_CST,
		       0,0}},
	SEMPTY,SEMPTY,
	/* XY0 */
	{2,{2,0,0,0},{ I0_000, I1___Z | I1_CST,
		       I0_XYZ, I1_XY_,
		       0,0,0,0}},
	/* YY0 */
	{2,{2,0,0,0},{ I0_000, I1___Z | I1_CST,
		       I0_YYY, I1_XY_,
		       0,0,0,0}},
	/* ZY0 */
	{3,{2,0,0,0},{ I0_000, I1___Z | I1_CST,
		       I0_ZZZ, I1_X__,
		       I0_YYY, I1__Y_,
		       0,0}},
	/* WY0 */
	{3,{2,0,0,0},{ I0_000, I1___Z | I1_CST,
		       I0_XYZ, I1__Y_,
		       I0_WZY, I1_X__,
		       0,0}},
	/* 0Y0 */
	{2,{2,0,0,0},{ I0_000, I1_X_Z | I1_CST,
		       I0_XYZ, I1__Y_,
		       0,0,0,0}},
	/* 1Y0 */
	{3,{2,0,2,0},{ I0_000, I1___Z | I1_CST,
		       I0_XYZ, I1__Y_,
		       I0_111, I1_X__ | I1_CST,
		       0,0}},
	SEMPTY,SEMPTY,
	/* XZ0 */
	{3,{2,0,0,0},{ I0_000, I1___Z | I1_CST,
		       I0_XYZ, I1_X__,
		       I0_ZZZ, I1__Y_,
		       0,0}},
	/* YZ0 */
	{2,{2,0,0,0},{ I0_000, I1___Z | I1_CST,
		       I0_YZX, I1_XY_,
		       0,0,0,0}},
	/* ZZ0 */
	{2,{2,0,0,0},{ I0_000, I1___Z | I1_CST,
		       I0_ZZZ, I1_XY_,
		       0,0,0,0}},
	/* WZ0 */
	{2,{2,0,0,0},{ I0_000, I1___Z | I1_CST,
		       I0_WZY, I1_XY_,
		       0,0,0,0}},
	/* 0Z0 */
	{2,{2,0,0,0},{ I0_000, I1_X_Z | I1_CST,
		       I0_ZZZ, I1__Y_,
		       0,0,0,0}},
	/* 1Z0 */
	{3,{2,0,2,0},{ I0_000, I1___Z | I1_CST,
		       I0_ZZZ, I1__Y_,
		       I0_111, I1_X__ | I1_CST,
		       0,0}},
	SEMPTY,SEMPTY,
	/* XW0 */
	{4,{0,1,2,0},{ I0_WZY, I1_XYZ,
		       I0_XXX, I1__Y_,
		       I0_000, I1___Z | I1_CST,
		       I0_XYZ, I1_X__ } },
	/* YW0 */
	{4,{0,1,2,0},{ I0_WZY, I1_XYZ,
		       I0_XXX, I1__Y_,
		       I0_000, I1___Z | I1_CST,
		       I0_YYY, I1_X__ } },
	/* ZW0 */
	{4,{0,1,2,0},{ I0_WZY, I1_XYZ,
		       I0_XXX, I1__Y_,
		       I0_000, I1___Z | I1_CST,
		       I0_ZZZ, I1_X__ } },
	/* WW0 */
	{3,{0,1,2,0},{ I0_WZY, I1_XYZ,
		       I0_XXX, I1_XY_,
		       I0_000, I1___Z | I1_CST,
		       0,0}},
	/* 0W0 */
	{3,{0,1,2,0},{ I0_WZY, I1_XYZ,
		       I0_XXX, I1__Y_,
		       I0_000, I1_X_Z | I1_CST,
		       0,0}},
	/* 1W0 */
	{4,{0,1,2,2},{ I0_WZY, I1_XYZ,
		       I0_XXX, I1__Y_,
		       I0_000, I1___Z | I1_CST,
		       I0_111, I1_X__ | I1_CST } },
	SEMPTY,SEMPTY,
	/* X00 */
	{2,{2,0,0,0},{ I0_000, I1__YZ | I1_CST,
		       I0_XYZ, I1_X__,
		       0,0,0,0}},
	/* Y00 */
	{2,{2,0,0,0},{ I0_000, I1__YZ | I1_CST,
		       I0_YYY, I1_X__,
		       0,0,0,0}},
	/* Z00 */
	{2,{2,0,0,0},{ I0_000, I1__YZ | I1_CST,
		       I0_ZZZ, I1_X__,
		       0,0,0,0}},
	/* W00 */
	{2,{2,0,0,0},{ I0_000, I1__YZ | I1_CST,
		       I0_WZY, I1_X__,
		       0,0,0,0}},
	/* 000 */
	{1,{2,0,0,0},{ I0_000, I1_XYZ | I1_CST,
		       0, 0, 0, 0, 0, 0 } },
	/* 100 */
	{2,{2,2,0,0},{ I0_000, I1__YZ | I1_CST,
		       I0_111, I1_X__ | I1_CST,
		       0,0,0,0}},
	SEMPTY,SEMPTY,
	/* X10 */
	{3,{2,0,2,0},{ I0_000, I1___Z | I1_CST,
		       I0_XYZ, I1_X__,
		       I0_111, I1__Y_ | I1_CST,
		       0,0}},
	/* Y10 */
	{3,{2,0,2,0},{ I0_000, I1___Z | I1_CST,
		       I0_YYY, I1_X__,
		       I0_111, I1__Y_ | I1_CST,
		       0,0}},
	/* Z10 */
	{3,{2,0,2,0},{ I0_000, I1___Z | I1_CST,
		       I0_ZZZ, I1_X__,
		       I0_111, I1__Y_ | I1_CST,
		       0,0}},
	/* W10 */
	{3,{2,0,2,0},{ I0_000, I1___Z | I1_CST,
		       I0_WZY, I1_X__,
		       I0_111, I1__Y_ | I1_CST,
		       0,0}},
	/* 010 */
	{2,{2,2,0,0},{ I0_000, I1_X_Z | I1_CST,
		       I0_111, I1__Y_ | I1_CST,
		       0, 0, 0, 0 } },
	/* 110 */
	{2,{2,2,0,0},{ I0_000, I1___Z | I1_CST,
		       I0_111, I1_XY_ | I1_CST,
		       0,0,0,0}},
	SEMPTY,SEMPTY,SEMPTY,SEMPTY,SEMPTY,SEMPTY,SEMPTY,SEMPTY,SEMPTY,SEMPTY,
	SEMPTY,SEMPTY,SEMPTY,SEMPTY,SEMPTY,SEMPTY,SEMPTY,SEMPTY,



	/* XX1 */
	{2,{2,0,0,0},{ I0_111, I1___Z | I1_CST,
		       I0_XXX, I1_XY_,
		       0,0,0,0}},
	/* YX1 */
	{3,{2,0,0,0},{ I0_111, I1___Z | I1_CST,
		       I0_XXX, I1__Y_,
		       I0_YYY, I1_X__,
		       0,0}},
	/* ZX1 */
	{2,{2,0,0,0},{ I0_111, I1___Z | I1_CST,
		       I0_ZXY, I1_XY_,
		       0,0,0,0}},
	/* WX1 */
	{3,{2,0,0,0},{ I0_111, I1___Z | I1_CST,
		       I0_XXX, I1__Y_,
		       I0_WZY, I1_X__,
		       0,0}},
	/* 0X1 */
	{3,{2,0,2,0},{ I0_111, I1___Z | I1_CST,
		       I0_XXX, I1__Y_,
		       I0_000, I1_X__ | I1_CST,
		       0,0}},
	/* 1X1 */
	{2,{2,0,0,0},{ I0_111, I1_X_Z | I1_CST,
		       I0_XXX, I1__Y_,
		       0,0,0,0}},
	SEMPTY,SEMPTY,
	/* XY1 */
	{2,{2,0,0,0},{ I0_111, I1___Z | I1_CST,
		       I0_XYZ, I1_XY_,
		       0,0,0,0}},
	/* YY1 */
	{2,{2,0,0,0},{ I0_111, I1___Z | I1_CST,
		       I0_YYY, I1_XY_,
		       0,0,0,0}},
	/* ZY1 */
	{3,{2,0,0,0},{ I0_111, I1___Z | I1_CST,
		       I0_ZZZ, I1_X__,
		       I0_YYY, I1__Y_,
		       0,0}},
	/* WY1 */
	{3,{2,0,0,0},{ I0_111, I1___Z | I1_CST,
		       I0_XYZ, I1__Y_,
		       I0_WZY, I1_X__,
		       0,0}},
	/* 0Y1 */
	{3,{2,0,2,0},{ I0_111, I1___Z | I1_CST,
		       I0_XYZ, I1__Y_,
		       I0_000, I1_X__ | I1_CST,
		       0,0}},
	/* 1Y1 */
	{2,{2,0,0,0},{ I0_111, I1_X_Z | I1_CST,
		       I0_XYZ, I1__Y_,
		       0,0,0,0}},
	SEMPTY,SEMPTY,
	/* XZ1 */
	{3,{2,0,0,0},{ I0_111, I1___Z | I1_CST,
		       I0_XYZ, I1_X__,
		       I0_ZZZ, I1__Y_,
		       0,0}},
	/* YZ1 */
	{2,{2,0,0,0},{ I0_111, I1___Z | I1_CST,
		       I0_YZX, I1_XY_,
		       0,0,0,0}},
	/* ZZ1 */
	{2,{2,0,0,0},{ I0_111, I1___Z | I1_CST,
		       I0_ZZZ, I1_XY_,
		       0,0,0,0}},
	/* WZ1 */
	{2,{2,0,0,0},{ I0_111, I1___Z | I1_CST,
		       I0_WZY, I1_XY_,
		       0,0,0,0}},
	/* 0Z1 */
	{3,{2,0,2,0},{ I0_111, I1___Z | I1_CST,
		       I0_ZZZ, I1__Y_,
		       I0_000, I1_X__ | I1_CST,
		       0,0}},
	/* 1Z1 */
	{2,{2,0,0,0},{ I0_111, I1_X_Z | I1_CST,
		       I0_ZZZ, I1__Y_,
		       0,0,0,0}},
	SEMPTY,SEMPTY,
	/* XW1 */
	{4,{0,1,2,0},{ I0_WZY, I1_XYZ,
		       I0_XXX, I1__Y_,
		       I0_000, I1___Z | I1_CST,
		       I0_XYZ, I1_X__ } },
	/* YW1 */
	{4,{0,1,2,0},{ I0_WZY, I1_XYZ,
		       I0_XXX, I1__Y_,
		       I0_111, I1___Z | I1_CST,
		       I0_YYY, I1_X__ } },
	/* ZW1 */
	{4,{0,1,2,0},{ I0_WZY, I1_XYZ,
		       I0_XXX, I1__Y_,
		       I0_111, I1___Z | I1_CST,
		       I0_ZZZ, I1_X__ } },
	/* WW1 */
	{3,{0,1,2,0},{ I0_WZY, I1_XYZ,
		       I0_XXX, I1_XY_,
		       I0_111, I1___Z | I1_CST,
		       0,0}},
	/* 0W1 */
	{4,{0,1,2,2},{ I0_WZY, I1_XYZ,
		       I0_XXX, I1__Y_,
		       I0_111, I1___Z | I1_CST,
		       I0_000, I1_X__ | I1_CST } },
	/* 1W1 */
	{3,{0,1,2,0},{ I0_WZY, I1_XYZ,
		       I0_XXX, I1__Y_,
		       I0_111, I1_X_Z | I1_CST,
		       0,0}},
	SEMPTY,SEMPTY,
	/* X01 */
	{3,{2,0,2,0},{ I0_111, I1___Z | I1_CST,
		       I0_XYZ, I1_X__,
		       I0_000, I1__Y_ | I1_CST,
		       0,0}},
	/* Y01 */
	{3,{2,0,2,0},{ I0_111, I1___Z | I1_CST,
		       I0_YYY, I1_X__,
		       I0_000, I1__Y_ | I1_CST,
		       0,0}},
	/* Z01 */
	{3,{2,0,2,0},{ I0_111, I1___Z | I1_CST,
		       I0_ZZZ, I1_X__,
		       I0_000, I1__Y_ | I1_CST,
		       0,0}},
	/* W01 */
	{3,{2,0,2,0},{ I0_111, I1___Z | I1_CST,
		       I0_WZY, I1_X__,
		       I0_000, I1__Y_ | I1_CST,
		       0,0}},
	/* 001 */
	{2,{2,2,0,0},{ I0_111, I1___Z | I1_CST,
		       I0_000, I1_XY_ | I1_CST,
		       0,0,0,0}},
	/* 101 */
	{2,{2,2,0,0},{ I0_111, I1_X_Z | I1_CST,
		       I0_000, I1__Y_ | I1_CST,
		       0, 0, 0, 0 } },
	SEMPTY,SEMPTY,
	/* X11 */
	{2,{2,0,0,0},{ I0_111, I1__YZ | I1_CST,
		       I0_XYZ, I1_X__,
		       0,0,0,0}},
	/* Y11 */
	{2,{2,0,0,0},{ I0_111, I1__YZ | I1_CST,
		       I0_YYY, I1_X__,
		       0,0,0,0}},
	/* Z11 */
	{2,{2,0,0,0},{ I0_111, I1__YZ | I1_CST,
		       I0_ZZZ, I1_X__,
		       0,0,0,0}},
	/* W11 */
	{2,{2,0,0,0},{ I0_111, I1__YZ | I1_CST,
		       I0_WZY, I1_X__,
		       0,0,0,0}},
	/* 011 */
	{2,{2,2,0,0},{ I0_111, I1__YZ | I1_CST,
		       I0_000, I1_X__ | I1_CST,
		       0,0,0,0}},
	/* 111 */
	{1,{2,0,0,0},{ I0_111, I1_XYZ | I1_CST,
		       0, 0, 0, 0, 0, 0 } },
	SEMPTY,SEMPTY,SEMPTY,SEMPTY,SEMPTY,SEMPTY,SEMPTY,SEMPTY,SEMPTY,SEMPTY,
	SEMPTY,SEMPTY,SEMPTY,SEMPTY,SEMPTY,SEMPTY,SEMPTY,SEMPTY,SEMPTY,SEMPTY,
	SEMPTY,SEMPTY,SEMPTY,SEMPTY,SEMPTY,SEMPTY,SEMPTY,SEMPTY,SEMPTY,SEMPTY,
	SEMPTY,SEMPTY,SEMPTY,SEMPTY,SEMPTY,SEMPTY,SEMPTY,SEMPTY,SEMPTY,SEMPTY,
	SEMPTY,SEMPTY,SEMPTY,SEMPTY,SEMPTY,SEMPTY,SEMPTY,SEMPTY,SEMPTY,SEMPTY,
	SEMPTY,SEMPTY,SEMPTY,SEMPTY,SEMPTY,SEMPTY,SEMPTY,SEMPTY,SEMPTY,SEMPTY,
	SEMPTY,SEMPTY,SEMPTY,SEMPTY,SEMPTY,SEMPTY,SEMPTY,SEMPTY,SEMPTY,SEMPTY,
	SEMPTY,SEMPTY,SEMPTY,SEMPTY,SEMPTY,SEMPTY,SEMPTY,SEMPTY,SEMPTY,SEMPTY,
	SEMPTY,SEMPTY,SEMPTY,SEMPTY,SEMPTY,SEMPTY,SEMPTY,SEMPTY,SEMPTY,SEMPTY,
	SEMPTY,SEMPTY,SEMPTY,SEMPTY,SEMPTY,SEMPTY,SEMPTY,SEMPTY,SEMPTY,SEMPTY,
	SEMPTY,SEMPTY,SEMPTY,SEMPTY,SEMPTY,SEMPTY,SEMPTY,SEMPTY,SEMPTY,SEMPTY,
	SEMPTY,SEMPTY,SEMPTY,SEMPTY,SEMPTY,SEMPTY,SEMPTY,SEMPTY,SEMPTY,SEMPTY,
	SEMPTY,SEMPTY,SEMPTY,SEMPTY,SEMPTY,SEMPTY,SEMPTY,SEMPTY,SEMPTY,SEMPTY,
	SEMPTY,SEMPTY,SEMPTY,SEMPTY,SEMPTY,SEMPTY,SEMPTY,SEMPTY,SEMPTY,SEMPTY,
	SEMPTY,SEMPTY,SEMPTY,SEMPTY,SEMPTY,SEMPTY,SEMPTY,SEMPTY
};

/******************************************************************************
* Color source mask table
******************************************************************************/

#define S_111	R300_FPI0_ARGC_ONE
#define S_000	R300_FPI0_ARGC_ZERO

#define S0XXX	R300_FPI0_ARGC_SRC0C_XXX
#define S0YYY	R300_FPI0_ARGC_SRC0C_YYY
#define S0ZZZ	R300_FPI0_ARGC_SRC0C_ZZZ
#define S0XYZ	R300_FPI0_ARGC_SRC0C_XYZ
#define S0ZXY	R300_FPI0_ARGC_SRC0C_ZXY
#define S0YZX	R300_FPI0_ARGC_SRC0C_YZX
#define S0WZY	R300_FPI0_ARGC_SRC0CA_WZY
#define S0WZY	R300_FPI0_ARGC_SRC0CA_WZY

#define S1XXX	R300_FPI0_ARGC_SRC1C_XXX
#define S1YYY	R300_FPI0_ARGC_SRC1C_YYY
#define S1ZZZ	R300_FPI0_ARGC_SRC1C_ZZZ
#define S1XYZ	R300_FPI0_ARGC_SRC1C_XYZ
#define S1ZXY	R300_FPI0_ARGC_SRC1C_ZXY
#define S1YZX	R300_FPI0_ARGC_SRC1C_YZX
#define S1WZY	R300_FPI0_ARGC_SRC1CA_WZY

#define S2XXX	R300_FPI0_ARGC_SRC2C_XXX
#define S2YYY	R300_FPI0_ARGC_SRC2C_YYY
#define S2ZZZ	R300_FPI0_ARGC_SRC2C_ZZZ
#define S2XYZ	R300_FPI0_ARGC_SRC2C_XYZ
#define S2ZXY	R300_FPI0_ARGC_SRC2C_ZXY
#define S2YZX	R300_FPI0_ARGC_SRC2C_YZX
#define S2WZY	R300_FPI0_ARGC_SRC2CA_WZY

#define ntnat	32

const GLuint r300_swz_srcc_mask[3][512] = {
	{
		S0XXX,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,
		ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,S0YZX,ntnat,ntnat,
		ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,
		ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,
		ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,
		ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,
		ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,S0ZXY,ntnat,ntnat,ntnat,
		ntnat,ntnat,ntnat,S0YYY,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,
		ntnat,ntnat,ntnat,S0WZY,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,
		ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,
		ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,
		ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,
		ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,
		ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,S0XYZ,ntnat,ntnat,ntnat,
		ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,S0ZZZ,ntnat,ntnat,ntnat,
		ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,
		ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,
		ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,
		ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,
		ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,
		ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,
		ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,
		ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,
		ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,
		ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,
		ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,
		ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,
		ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,
		ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,
		ntnat,ntnat,S_000,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,
		ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,
		ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,
		ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,
		ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,
		ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,
		ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,
		ntnat,ntnat,ntnat,ntnat,ntnat,S_111,ntnat,ntnat,ntnat,ntnat,
		ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,
		ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,
		ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,
		ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,
		ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,
		ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,
		ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,
		ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,
		ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,
		ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,
		ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,
		ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,
		ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,
		ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,
		ntnat,ntnat
	},
	{
		S1XXX,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,
		ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,S1YZX,ntnat,ntnat,
		ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,
		ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,
		ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,
		ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,
		ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,S1ZXY,ntnat,ntnat,ntnat,
		ntnat,ntnat,ntnat,S1YYY,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,
		ntnat,ntnat,ntnat,S1WZY,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,
		ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,
		ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,
		ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,
		ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,
		ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,S1XYZ,ntnat,ntnat,ntnat,
		ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,S1ZZZ,ntnat,ntnat,ntnat,
		ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,
		ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,
		ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,
		ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,
		ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,
		ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,
		ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,
		ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,
		ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,
		ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,
		ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,
		ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,
		ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,
		ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,
		ntnat,ntnat,S_000,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,
		ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,
		ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,
		ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,
		ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,
		ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,
		ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,
		ntnat,ntnat,ntnat,ntnat,ntnat,S_111,ntnat,ntnat,ntnat,ntnat,
		ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,
		ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,
		ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,
		ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,
		ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,
		ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,
		ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,
		ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,
		ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,
		ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,
		ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,
		ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,
		ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,
		ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,
		ntnat,ntnat
	},
	{
		S2XXX,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,
		ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,S2YZX,ntnat,ntnat,
		ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,
		ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,
		ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,
		ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,
		ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,S2ZXY,ntnat,ntnat,ntnat,
		ntnat,ntnat,ntnat,S2YYY,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,
		ntnat,ntnat,ntnat,S2WZY,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,
		ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,
		ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,
		ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,
		ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,
		ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,S2XYZ,ntnat,ntnat,ntnat,
		ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,S2ZZZ,ntnat,ntnat,ntnat,
		ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,
		ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,
		ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,
		ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,
		ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,
		ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,
		ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,
		ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,
		ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,
		ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,
		ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,
		ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,
		ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,
		ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,
		ntnat,ntnat,S_000,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,
		ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,
		ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,
		ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,
		ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,
		ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,
		ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,
		ntnat,ntnat,ntnat,ntnat,ntnat,S_111,ntnat,ntnat,ntnat,ntnat,
		ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,
		ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,
		ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,
		ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,
		ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,
		ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,
		ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,
		ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,
		ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,
		ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,
		ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,
		ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,
		ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,
		ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,ntnat,
		ntnat,ntnat
	}
};

/******************************************************************************
* Alpha source mask table
******************************************************************************/

GLuint r300_swz_srca_mask[3][6] = {
	{ R300_FPI2_ARGA_SRC0C_X,
	  R300_FPI2_ARGA_SRC0C_Y,
	  R300_FPI2_ARGA_SRC0C_Z,
	  R300_FPI2_ARGA_SRC0A,
	  R300_FPI2_ARGA_ZERO,
	  R300_FPI2_ARGA_ONE },
	{ R300_FPI2_ARGA_SRC1C_X,
	  R300_FPI2_ARGA_SRC1C_Y,
	  R300_FPI2_ARGA_SRC1C_Z,
	  R300_FPI2_ARGA_SRC1A,
	  R300_FPI2_ARGA_ZERO,
	  R300_FPI2_ARGA_ONE },
	{ R300_FPI2_ARGA_SRC2C_X,
	  R300_FPI2_ARGA_SRC2C_Y,
	  R300_FPI2_ARGA_SRC2C_Z,
	  R300_FPI2_ARGA_SRC2A,
	  R300_FPI2_ARGA_ZERO,
	  R300_FPI2_ARGA_ONE },
};
