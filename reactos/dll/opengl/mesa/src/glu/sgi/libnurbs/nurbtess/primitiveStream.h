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

/*we do not use the constans GL_... so that this file is independent of 
 *<GL/gl.h>
 */

#ifndef _PRIMITIVE_STREAM_H
#define _PRIMITIVE_STREAM_H

enum {PRIMITIVE_STREAM_FAN, PRIMITIVE_STREAM_STRIP};

#include "definitions.h"

class primStream {
  Int *lengths; /*length[i]=number of vertices of ith primitive*/
  Int *types; /*each primive has a type: FAN or STREAM*/
  Real *vertices; /*the size >= 2 * num_vertices, each vertex (u,v)*/

  /*the following size information are used for dynamic arrays*/
  Int index_lengths; /*the current available entry*/
  Int size_lengths; /*the allocated size of the array: lengths*/
  Int index_vertices;
  Int size_vertices;

  /*the vertex is inserted one by one. counter is used to 
   *count the number of vertices which have been inserted so far in 
   *the current primitive
   */
  Int counter;

public:  
  primStream(Int sizeLengths, Int sizeVertices);
  ~primStream();

  Int get_n_prims() //num of primitives
    {
      return index_lengths;
    }
  Int get_type(Int i)  //the type of ith primitive
    {
      return  types[i];
    }
  Int get_length(Int i) //the length of the ith primitive
    {
      return lengths[i];
    }
  Real* get_vertices() {return vertices;}

  /*the begining of inserting a new primitive. 
   *reset counter to be 0.
   */
  void begin();
  void insert(Real u, Real v);
  void insert(Real v[2]) {insert(v[0], v[1]);}
  void end(Int type);
  
  Int num_triangles();
  
  void triangle(Real A[2], Real B[2], Real C[2])
    {
      begin();
      insert(A);
      insert(B);
      insert(C);
      end(PRIMITIVE_STREAM_FAN);
    }
  void print();
  void draw(); /*using GL to draw the primitives*/
};






  

#endif

