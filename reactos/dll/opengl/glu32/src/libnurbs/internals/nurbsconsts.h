/*
 * SGI FREE SOFTWARE LICENSE B (Version 2.0, Sept. 18, 2008)
 * Copyright (C) 1991-2000 Silicon Graphics, Inc. All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice including the dates of first publication and
 * either this permission notice or a reference to
 * http://oss.sgi.com/projects/FreeB/
 * shall be included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * SILICON GRAPHICS, INC. BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * Except as contained in this notice, the name of Silicon Graphics, Inc.
 * shall not be used in advertising or otherwise to promote the sale, use or
 * other dealings in this Software without prior written authorization from
 * Silicon Graphics, Inc.
 */

/*
 * nurbsconsts.h
 *
 */

#ifndef __glunurbsconsts_h_
#define __glunurbsconsts_h_

/* NURBS Properties - one set per map, 
   each takes a single INREAL arg */
#define N_SAMPLING_TOLERANCE  	1
#define N_S_RATE		6
#define N_T_RATE		7
#define N_CLAMPFACTOR		13
#define 	N_NOCLAMPING		0.0
#define N_MINSAVINGS		14
#define 	N_NOSAVINGSSUBDIVISION	0.0

/* NURBS Properties - one set per map, 
   each takes an enumerated value */
#define N_CULLING		2
#define 	N_NOCULLING		0.0
#define 	N_CULLINGON		1.0
#define N_SAMPLINGMETHOD	10
#define 	N_NOSAMPLING		0.0
#define 	N_FIXEDRATE		3.0
#define 	N_DOMAINDISTANCE	2.0
#define 	N_PARAMETRICDISTANCE	5.0
#define 	N_PATHLENGTH		6.0
#define 	N_SURFACEAREA		7.0
#define         N_OBJECTSPACE_PARA           8.0
#define         N_OBJECTSPACE_PATH           9.0
#define N_BBOX_SUBDIVIDING	17
#define 	N_NOBBOXSUBDIVISION	0.0
#define 	N_BBOXTIGHT		1.0
#define 	N_BBOXROUND		2.0

/* NURBS Rendering Properties - one set per renderer
   each takes an enumerated value */
#define N_DISPLAY		3
#define 	N_FILL			1.0 	
#define 	N_OUTLINE_POLY		2.0
#define 	N_OUTLINE_TRI		3.0
#define 	N_OUTLINE_QUAD	 	4.0
#define 	N_OUTLINE_PATCH		5.0
#define 	N_OUTLINE_PARAM		6.0
#define 	N_OUTLINE_PARAM_S	7.0
#define 	N_OUTLINE_PARAM_ST 	8.0
#define 	N_OUTLINE_SUBDIV 	9.0
#define 	N_OUTLINE_SUBDIV_S 	10.0
#define 	N_OUTLINE_SUBDIV_ST 	11.0
#define 	N_ISOLINE_S		12.0
#define N_ERRORCHECKING		4
#define 	N_NOMSG			0.0
#define 	N_MSG			1.0

/* GL 4.0 propeties not defined above */
#ifndef N_PIXEL_TOLERANCE
#define N_PIXEL_TOLERANCE	N_SAMPLING_TOLERANCE
#define N_ERROR_TOLERANCE	20
#define N_SUBDIVISIONS		5
#define N_TILES			8
#define N_TMP1			9
#define N_TMP2			N_SAMPLINGMETHOD
#define N_TMP3			11
#define N_TMP4			12
#define N_TMP5			N_CLAMPFACTOR
#define N_TMP6			N_MINSAVINGS
#define N_S_STEPS		N_S_RATE
#define N_T_STEPS		N_T_RATE
#endif

/* NURBS Rendering Properties - one set per map,
   each takes an INREAL matrix argument */
#define N_CULLINGMATRIX		1
#define N_SAMPLINGMATRIX	2
#define N_BBOXMATRIX		3


/* NURBS Rendering Properties - one set per map,
   each takes an INREAL vector argument */
#define	N_BBOXSIZE		4

/* type argument for trimming curves */
#ifndef N_P2D
#define N_P2D 	 		0x8	
#define N_P2DR 	 		0xd
#endif	

#endif /* __glunurbsconsts_h_ */
