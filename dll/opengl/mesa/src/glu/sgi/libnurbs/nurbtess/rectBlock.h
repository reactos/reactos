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

#ifndef _RECTBLOCK_H
#define _RECTBLOCK_H

#include "definitions.h"
#include "gridWrap.h"

class rectBlock{
  Int upGridLineIndex;
  Int lowGridLineIndex;
  Int* leftIndices; //up to bottome
  Int* rightIndices; //up to bottom
public:
  //the arrays are copies.
  rectBlock(gridBoundaryChain* left, gridBoundaryChain* right, Int beginVline, Int endVline);
  ~rectBlock(); //free the two arrays

  Int get_upGridLineIndex() {return upGridLineIndex;}
  Int get_lowGridLineIndex() {return lowGridLineIndex;}
  Int* get_leftIndices() {return leftIndices;}
  Int* get_rightIndices() {return rightIndices;}

  Int num_quads();

  void print();
  void draw(Real* u_values, Real* v_values);
};


class rectBlockArray{
  rectBlock** array;
  Int n_elements;
  Int size;
public:
  rectBlockArray(Int s);
  ~rectBlockArray();//delete avarything including the blocks
  
  Int get_n_elements() {return n_elements;}
  rectBlock* get_element(Int i) {return array[i];}
  void insert(rectBlock* newBlock); //only take the pointer, not ther cotent

  Int num_quads();

  void print();
  void draw(Real* u_values, Real* v_values);
};
  


#endif

