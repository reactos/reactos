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
#include "glimports.h"
#include "zlassert.h"
#include <GL/gl.h>

#include "rectBlock.h"

rectBlock::rectBlock(gridBoundaryChain* left, gridBoundaryChain* right, Int beginVline, Int endVline)
{
  Int i;


  upGridLineIndex = left->getVlineIndex(beginVline);

  lowGridLineIndex = left->getVlineIndex(endVline);

  Int n = upGridLineIndex-lowGridLineIndex+1; //number of grid lines
  leftIndices = (Int*) malloc(sizeof(Int) * n);
  assert(leftIndices);
  rightIndices = (Int*) malloc(sizeof(Int) * n);
  assert(rightIndices);
  for(i=0; i<n; i++)
    {

      leftIndices[i] = left->getInnerIndex(i+beginVline);
      rightIndices[i] = right->getInnerIndex(i+beginVline);
    }
}


rectBlock::~rectBlock()
{
  free(leftIndices);
  free(rightIndices);
}

void rectBlock::print()
{
  Int i;
  printf("block:\n");
  for(i=upGridLineIndex; i >= lowGridLineIndex; i--)
    {
      printf("gridline %i, (%i,%i)\n", i, leftIndices[upGridLineIndex-i], rightIndices[upGridLineIndex-i]);
    }
}



void rectBlock::draw(Real* u_values, Real* v_values)
{
  Int i,j,k;
  //upgrid line to bot grid line
#ifdef DEBUG
printf("upGridLineIndex=%i, lowGridLineIndex=%i\n", upGridLineIndex, lowGridLineIndex);
#endif
  for(k=0, i=upGridLineIndex; i > lowGridLineIndex; i--, k++)
    {
      glBegin(GL_QUAD_STRIP);

      for(j=leftIndices[k+1]; j<= rightIndices[k+1]; j++)
	{
	  glVertex2f(u_values[j], v_values[i]);
	  glVertex2f(u_values[j], v_values[i-1]);
	}
      glEnd();
    }  
}


Int rectBlock::num_quads()
{
  Int ret=0;
  Int k,i;
  for(k=0, i=upGridLineIndex; i>lowGridLineIndex; i--, k++)
    {
      ret += (rightIndices[k+1]-leftIndices[k+1]);
    }
  return ret;
}

Int rectBlockArray::num_quads()
{
  Int ret=0;
  for(Int i=0; i<n_elements; i++)
    ret += array[i]->num_quads();
  return ret;
}

rectBlockArray::rectBlockArray(Int s)
{
  Int i;
  n_elements = 0;
  size = s;
  array = (rectBlock**) malloc(sizeof(rectBlock*) * s);
  assert(array);
//initialization
  for(i=0; i<s; i++)
    array[i] = NULL;
}

rectBlockArray::~rectBlockArray()
{
  Int i;
  for(i=0; i<size; i++)
    {
      if(array[i] != NULL)
	delete array[i];
    }
  free(array);
}

//put to the end of the array, check the size
void rectBlockArray::insert(rectBlock* newBlock)
{
  Int i;
  if(n_elements == size) //full
    {
      rectBlock** temp = (rectBlock**) malloc(sizeof(rectBlock) * (2*size+1));
      assert(temp);
      //initialization
      for(i=0; i<2*size+1; i++)
	temp[i] = NULL;
      
      for(i=0; i<n_elements; i++)
	temp[i] = array[i];
      
      free(array);
      array = temp;
      size = 2*size +  1;
    }
  
  array[n_elements++] = newBlock;
}

void rectBlockArray::print()
{
  Int i;
  for(i=0; i<n_elements; i++)
    array[i]->print();
}

void rectBlockArray::draw(Real* u_values, Real* v_values)
{
  Int i;
  for(i=0; i<n_elements; i++)
    array[i]->draw(u_values, v_values);
}










