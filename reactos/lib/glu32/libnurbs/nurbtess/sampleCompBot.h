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
**
** $Date: 2004/02/02 16:39:13 $ $Revision: 1.1 $
*/
/*
** $Header: /cygdrive/c/RCVS/CVS/ReactOS/reactos/lib/glu32/libnurbs/nurbtess/sampleCompBot.h,v 1.1 2004/02/02 16:39:13 navaraf Exp $
*/

#ifndef _SAMPLECOMPBOT_H
#define _SAMPLECOMPBOT_H

#include "sampleMonoPoly.h"

void findBotLeftSegment(vertexArray* leftChain, 
			Int leftEnd,
			Int leftCorner,
			Real u,
			Int& ret_index_mono,
			Int& ret_index_pass);

void findBotRightSegment(vertexArray* rightChain, 
			 Int rightEnd,
			 Int rightCorner,
			 Real u,
			 Int& ret_index_mono,
			 Int& ret_index_pass);


void sampleBotRightWithGridLinePost(Real* botVertex,
				    vertexArray* rightChain,
				    Int rightEnd,
				    Int segIndexMono,
				    Int segIndexPass,
				    Int rightCorner,
				    gridWrap* grid,
				    Int gridV,
				    Int leftU,
				    Int rightU,
				    primStream* pStream);


void sampleBotRightWithGridLine(Real* botVertex, 
				vertexArray* rightChain,
				Int rightEnd,
				Int rightCorner,
				gridWrap* grid,
				Int gridV,
				Int leftU,
				Int rightU,
				primStream* pStream);


void sampleBotLeftWithGridLinePost(Real* botVertex,
				   vertexArray* leftChain,
				   Int leftEnd,
				   Int segIndexMono,
				   Int segIndexPass,
				   Int leftCorner,
				   gridWrap* grid,
				   Int gridV,
				   Int leftU, 
				   Int rightU,
				   primStream* pStream);


void sampleBotLeftWithGridLine(Real* botVertex,
			       vertexArray* leftChain,
			       Int leftEnd,
			       Int leftCorner,
			       gridWrap* grid,
			       Int gridV,
			       Int leftU,
			       Int rightU,
			       primStream* pStream);


Int findBotSeparator(vertexArray* leftChain,
		     Int leftEnd,
		     Int leftCorner,
		     vertexArray* rightChain,
		     Int rightEnd,
		     Int rightCorner,
		     Int& ret_sep_left,
		     Int& ret_sep_right);

void sampleCompBot(Real* botVertex,
		   vertexArray* leftChain,
		   Int leftEnd,
		   vertexArray* rightChain,
		   Int rightEnd,
		   gridBoundaryChain* leftGridChain,
		   gridBoundaryChain* rightGridChain,
		   Int gridIndex,
		   Int down_leftCornerWhere,
		   Int down_leftCornerIndex,
		   Int down_rightCornerWhere,
		   Int down_rightCornerIndex,
		   primStream* pStream);

void sampleCompBotSimple(Real* botVertex,
		   vertexArray* leftChain,
		   Int leftEnd,
		   vertexArray* rightChain,
		   Int rightEnd,
		   gridBoundaryChain* leftGridChain,
		   gridBoundaryChain* rightGridChain,
		   Int gridIndex,
		   Int down_leftCornerWhere,
		   Int down_leftCornerIndex,
		   Int down_rightCornerWhere,
		   Int down_rightCornerIndex,
		   primStream* pStream);

#endif
