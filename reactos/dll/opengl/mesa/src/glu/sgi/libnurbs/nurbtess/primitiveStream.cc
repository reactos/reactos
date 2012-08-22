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

#include "gluos.h"
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <GL/gl.h> 

#include "primitiveStream.h"

Int primStream::num_triangles()
{
  Int i;
  Int ret=0;
  for(i=0; i<index_lengths; i++)
    {
      ret += lengths[i]-2;
    }
  return ret;
}

	  

/*the begining of inserting a new primitive. 
 *reset counter to be 0.
 */
void primStream::begin()
{
  counter = 0;
}

void primStream::insert(Real u, Real v)
{
  /*if the space cannot hold u and v,
   *we have to expand the array
   */
  if(index_vertices+1 >= size_vertices) {
    Real* temp = (Real*) malloc (sizeof(Real) * (2*size_vertices + 2));
    assert(temp);
    
    /*copy*/
    for(Int i=0; i<index_vertices; i++)
      temp[i] = vertices[i];
    
    free(vertices);
    vertices = temp;
    size_vertices = 2*size_vertices + 2;
  }

  vertices[index_vertices++] = u;
  vertices[index_vertices++] = v;
  counter++;
}

/*the end of a primitive.
 *increase index_lengths 
 */
void primStream::end(Int type)
{
  Int i;
  /*if there is no vertex in this primitive, 
   *nothing needs to be done
   */
  if(counter == 0) return ;

  if(index_lengths >= size_lengths){
    Int* temp = (Int*) malloc(sizeof(Int) * (2*size_lengths + 2));
    assert(temp);
    Int* tempTypes = (Int*) malloc(sizeof(Int) * (2*size_lengths + 2));
    assert(tempTypes);
    
    /*copy*/
    for(i=0; i<index_lengths; i++){
      temp[i] = lengths[i];
      tempTypes[i] = types[i];
    }
    
    free(lengths);
    free(types);
    lengths = temp;
    types = tempTypes;
    size_lengths = 2*size_lengths + 2;
  }
  lengths[index_lengths] = counter;
  types[index_lengths] = type;
  index_lengths++;
}

void primStream::print()
{
  Int i,j,k;
  printf("index_lengths=%i,size_lengths=%i\n", index_lengths, size_lengths);
  printf("index_vertices=%i,size_vertices=%i\n", index_vertices, size_vertices);
  k=0;
  for(i=0; i<index_lengths; i++)
    {
      if(types[i] == PRIMITIVE_STREAM_FAN)
	printf("primitive-FAN:\n");
      else 
	printf("primitive-STRIP:\n");
      for(j=0; j<lengths[i]; j++)
	{
	  printf("(%f,%f) ", vertices[k], vertices[k+1]);
	  k += 2;	  
	}
      printf("\n");
    }
}

primStream::primStream(Int sizeLengths, Int sizeVertices)
{
  lengths = (Int*)malloc (sizeof(Int) * sizeLengths);
  assert(lengths);
  types = (Int*)malloc (sizeof(Int) * sizeLengths);
  assert(types);
  
  vertices = (Real*) malloc(sizeof(Real) * sizeVertices);
  assert(vertices);
  
  index_lengths = 0;
  index_vertices = 0;
  size_lengths = sizeLengths;
  size_vertices = sizeVertices; 

  counter = 0; 
}

primStream::~primStream()
{
  free(lengths);
  free(types);
  free(vertices);
}

void primStream::draw()
{
  Int i,j,k;
  k=0;
  for(i=0; i<index_lengths; i++)
    {
      switch(types[i]){
      case PRIMITIVE_STREAM_FAN:
	glBegin(GL_TRIANGLE_FAN);
	break;
      case PRIMITIVE_STREAM_STRIP:
	glBegin(GL_TRIANGLE_STRIP);
	break;
      }
      
      for(j=0; j<lengths[i]; j++){
	glVertex2fv(vertices+k);
	k += 2;
      }
      glEnd();
    }
}

