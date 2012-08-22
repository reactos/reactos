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


static void swap(void *v[], int i, int j)
{
  void *temp;
  temp = v[i];
  v[i] = v[j];
  v[j] = temp;
}

/*as an example to use this function to
 *sort integers, you need to supply the function
 *int comp(int *i1, int *i2)
 *{
 *  if( *i1 < * i2) return -1;
 *  else return 1;
 *}
 *and an array of pointers to integers: 
 * int *v[100] (allocate space for where  each v[i] points to).
 *then you can call:
 * quicksort( (void**)v, left, right, (int (*)(void *, void *))comp)
 */
void quicksort(void *v[], int left, int right,
	       int (*comp) (void *, void *))
{
  int i, last;
  if(left >= right) /*do nothing if array contains */
    return;         /*fewer than two elements*/
  
  swap(v, left, (left+right)/2);
  last = left;
  for(i=left+1; i<=right; i++)
    if((*comp)(v[i], v[left])<0)
      swap(v, ++last, i);
  swap(v, left, last);
  quicksort(v, left, last-1, comp);
  quicksort(v, last+1, right, comp);
}
