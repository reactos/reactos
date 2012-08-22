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
*/
/*
*/

#include <stdlib.h>
#include <stdio.h>

#include "polyUtil.h"

Real area(Real A[2], Real B[2], Real C[2])
{
  Real Bx, By, Cx, Cy;
  Bx = B[0] - A[0];
  By = B[1] - A[1];
  Cx = C[0] - A[0];
  Cy = C[1] - A[1];
  return Bx*Cy - Cx*By;

/*  return (B[0]-A[0])*(C[1]-A[1]) - (C[0]-A[0])*(B[1]-A[1]);*/
}

/*given a directed line A->B, and a point P, 
 *determine whether P is to the left of AB.
 *the line A->B (imagine it has beedn extended both 
 *end to the infinity) divides the plan into two 
 *half planes. When we walk from A to B, one
 *half is to the left and the other half is to the right.
 *return 1 if P is to the left.
 *if P is on AB, 0 is returned.
 */
Int pointLeftLine(Real A[2], Real B[2],  Real P[2])
{
  if(area(A, B, P) >0) return 1;
  else return 0;
}

/*given two directed line: A -> B -> C, and another point P.
 *determine whether P is to the left hand side of A->B->C.
 *Think of BA and BC extended as two rays. So that the plane is
 * divided into two parts. One part is to the left we  walk from A 
 *to B and to C, the other part is to the right.
 * In order for P to be the left, P must be either to the left 
 *of 
 */
Int pointLeft2Lines(Real A[2], Real B[2], Real C[2], Real P[2])
{
  Int C_left_AB = (area(A, B, C)>0);
  Int P_left_AB = (area(A, B, P)>0);
  Int P_left_BC = (area(B, C, P)>0);

  if(C_left_AB)
    {
      return (P_left_AB && P_left_BC);
    }
  else
    return  (P_left_AB || P_left_BC);
}
