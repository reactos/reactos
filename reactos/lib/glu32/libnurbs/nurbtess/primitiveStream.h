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
** $Header: /cygdrive/c/RCVS/CVS/ReactOS/reactos/lib/glu32/libnurbs/nurbtess/primitiveStream.h,v 1.1 2004/02/02 16:39:13 navaraf Exp $
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

