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
