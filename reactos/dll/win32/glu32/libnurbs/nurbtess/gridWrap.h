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
** $Header: /cygdrive/c/RCVS/CVS/ReactOS/reactos/lib/glu32/libnurbs/nurbtess/gridWrap.h,v 1.1 2004/02/02 16:39:13 navaraf Exp $
*/

#ifndef _GRIDWRAP_H
#define _GRIDWRAP_H

#include <stdio.h>
#include "definitions.h"

#include "primitiveStream.h"
#include "zlassert.h"

class gridWrap{
  Int n_ulines;
  Int n_vlines;
  Real u_min, u_max;
  Real v_min, v_max;

  /*cache the coordinate values for efficiency. 
   *these are redundant information when 
   *the grid is uniform.
   */
  Real* u_values; /*size is n_ulines*/
  Real* v_values; /*size is n_vlines*/

  Int is_uniform; 

public:
  //uniform grid constructor
  gridWrap(Int nUlines, Int nVlines,
	   Real uMin, Real uMax,
	   Real vMin, Real vMax
	   );

  //nonuniform grid constructor.
  gridWrap(Int nUlines, Real *uvals,
	   Int nVlines, Real *vvlas
	   );
  ~gridWrap();
  
  void print();
  Int get_n_ulines() {return n_ulines;}
  Int get_n_vlines() {return n_vlines;}
  Real get_u_min() {return u_min;}
  Real get_u_max() {return u_max;}
  Real get_v_min() {return v_min;}
  Real get_v_max() {return v_max;}

  Real get_u_value(Int i) 
    {
      assert(i<n_ulines);
      /*if(i>=n_ulines){printf("ERROR, n_ulines=%i,i=%i\n",n_ulines,i);exit(0);}*/      
      return u_values[i];}
  Real get_v_value(Int j) {return v_values[j];}

  Real* get_u_values() {return u_values;}
  Real* get_v_values() {return v_values;}

  void outputFanWithPoint(Int v, Int uleft, Int uright, 
			  Real vert[2], primStream* pStream);

  void draw();

  Int isUniform() {return is_uniform;}
};

class gridBoundaryChain{
  gridWrap* grid;
  Int firstVlineIndex;
  Int nVlines;
  Int* ulineIndices; /*each v line has a boundary*/
  Int* innerIndices; /*the segment of the vertical gridline from */
                     /*(innerIndices[i], i) to (innerIndices[i+1], i-1) */
                     /*is inside the polygon: i=1,...,nVlines-1*/

  Real2* vertices; /*one grid point at each grid V-line, cached for efficiency*/

public:
  gridBoundaryChain(gridWrap* gr, Int first_vline_index, Int n_vlines, Int* uline_indices, Int* inner_indices);

  ~gridBoundaryChain()
    {
      free(innerIndices);
      free(ulineIndices);
      free(vertices);
    }

  /*i indexes the vlines in this chain.
   */
  Int getVlineIndex(Int i) {return firstVlineIndex-i;}
  Int getUlineIndex(Int i) {return ulineIndices[i];}
  Real get_u_value(Int i) {return vertices[i][0];}
  Real get_v_value(Int i) {return vertices[i][1];}
  Int get_nVlines() {return nVlines;}
  Int getInnerIndex(Int i) {return innerIndices[i];}
  Real getInner_u_value(Int i) {return grid->get_u_value(innerIndices[i]);}

  Real* get_vertex(Int i) {return vertices[i];}
  gridWrap* getGrid() {return grid;}
  void leftEndFan(Int i, primStream* pStream);
  void rightEndFan(Int i, primStream* pStream);

  Int lookfor(Real v, Int i1, Int i2); //find i in [i1,i2] so that  vertices[i][1]>= v > vertices[i+1][1]
  void draw();
  void drawInner();
};

#endif
