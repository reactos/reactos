/*
** License Applicability. Except to the extent portions of this file are
** made subject to an alternative license as permitted in the SGI Free
** Software License B, Version 1.1 (the "License"), the contents of this
** file are subject only to the provisions of the License. You may not use
** this file except in compliance with the License. You may obtain a copy
** of the License at Silicon Graphics, Inc., attn: Legal Services, 1600
** Amphitheatre Parkway, Mountain View, CA 94043-1351, or at:
**
** http://oss.sgi.com/projects/FreeB
**
** Note that, as provided in the License, the Software is distributed on an
** "AS IS" basis, with ALL EXPRESS AND IMPLIED WARRANTIES AND CONDITIONS
** DISCLAIMED, INCLUDING, WITHOUT LIMITATION, ANY IMPLIED WARRANTIES AND
** CONDITIONS OF MERCHANTABILITY, SATISFACTORY QUALITY, FITNESS FOR A
** PARTICULAR PURPOSE, AND NON-INFRINGEMENT.
**
** Original Code. The Original Code is: OpenGL Sample Implementation,
** Version 1.2.1, released January 26, 2000, developed by Silicon Graphics,
** Inc. The Original Code is Copyright (c) 1991-2000 Silicon Graphics, Inc.
** Copyright in any portions created by third parties is as indicated
** elsewhere herein. All Rights Reserved.
**
** Additional Notice Provisions: The application programming interfaces
** established by SGI in conjunction with the Original Code are The
** OpenGL(R) Graphics System: A Specification (Version 1.2.1), released
** April 1, 1999; The OpenGL(R) Graphics System Utility Library (Version
** 1.3), released November 4, 1998; and OpenGL(R) Graphics with the X
** Window System(R) (Version 1.3), released October 19, 1998. This software
** was created using the OpenGL(R) version 1.2.1 Sample Implementation
** published by SGI, but has not been independently verified as being
** compliant with the OpenGL(R) version 1.2.1 Specification.
*/

/*
 * nurbsconsts.h
 *
 * $Date: 2004/02/02 16:39:12 $ $Revision: 1.1 $
 * $Header: /cygdrive/c/RCVS/CVS/ReactOS/reactos/lib/glu32/libnurbs/internals/nurbsconsts.h,v 1.1 2004/02/02 16:39:12 navaraf Exp $
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
