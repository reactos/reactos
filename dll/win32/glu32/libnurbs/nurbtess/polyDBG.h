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
** $Date$ $Revision: 1.1 $
*/
/*
** $Header: /cygdrive/c/RCVS/CVS/ReactOS/reactos/lib/glu32/libnurbs/nurbtess/polyDBG.h,v 1.1 2004/02/02 16:39:13 navaraf Exp $
*/

#ifndef _POLYDBG_H
#define _POLYDBG_H

#include "definitions.h"
#include "directedLine.h"
#include "monoTriangulation.h"

Int DBG_edgesIntersectGen(Real A[2], Real B[2], Real C[2], Real D[2]);
Int DBG_intersectChain(vertexArray* chain, Int start, Int end, Real A[2], Real B[2]);


Int DBG_edgesIntersect(directedLine* l1, directedLine* l2);
Int DBG_polygonSelfIntersect(directedLine* poly);
Int DBG_edgeIntersectPoly(directedLine* edge, directedLine* poly);
Int DBG_polygonsIntersect(directedLine* p1, directedLine* p2);
Int DBG_polygonListIntersect(directedLine* pList);

Int DBG_isCounterclockwise(directedLine* poly);
Int DBG_rayIntersectEdge(Real v0[2], Real dx, Real dy, Real v10[2], Real v1[2], Real v2[2]);
Int DBG_pointInsidePoly(Real v[2], directedLine* poly);
Int DBG_enclosingPolygons(directedLine* poly, directedLine* list);
void  DBG_reverse(directedLine* poly);
Int DBG_check(directedLine *polyList);

Int DBG_isConvex(directedLine *poly);
Int DBG_is_U_direction(directedLine *poly);
Int DBG_is_U_monotone(directedLine* poly);

directedLine* DBG_cutIntersectionAllPoly(directedLine* list);
directedLine* DBG_cutIntersectionPoly(directedLine *polygon, int& cutOccur);

sampledLine*  DBG_collectSampledLinesAllPoly(directedLine *polygonList);

void  DBG_collectSampledLinesPoly(directedLine *polygon, sampledLine*& retHead, sampledLine*& retTail);

#endif
