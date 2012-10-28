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
#include "glimports.h"
#include "myassert.h"
#include "nurbsconsts.h"
#include "trimvertex.h"
#include "dataTransform.h"

extern directedLine* arcLoopToDLineLoop(Arc_ptr loop);

#if 0 // UNUSED
static directedLine* copy_loop(Arc_ptr loop, Real2* vertArray, int& index, directedLine dline_buf[], sampledLine sline_buf[], int& index_dline)
{
  directedLine *ret;
  int old_index = index;
  int i = index;
  int j;
  for(j=0; j<loop->pwlArc->npts-1; j++, i++)
    {
      vertArray[i][0] = loop->pwlArc->pts[j].param[0];
      vertArray[i][1] = loop->pwlArc->pts[j].param[1];
    }  
  loop->clearmark();

  for(Arc_ptr jarc = loop->next; jarc != loop; jarc=jarc->next)
    {
      for(j=0; j<jarc->pwlArc->npts-1; j++, i++)
	{
	  vertArray[i][0] = jarc->pwlArc->pts[j].param[0];
	  vertArray[i][1] = jarc->pwlArc->pts[j].param[1];
	}
      jarc->clearmark();
    }
  //copy the first vertex again
  vertArray[i][0] = loop->pwlArc->pts[0].param[0];
  vertArray[i][1] = loop->pwlArc->pts[0].param[1];  
  i++;
  index=i;

  directedLine* dline;
  sampledLine* sline;
  sline = &sline_buf[index_dline];
  dline = &dline_buf[index_dline];
  sline->init(2, &vertArray[old_index]);
  dline->init(INCREASING, sline);
  ret = dline;
  index_dline++;

  for(i=old_index+1; i<= index-2; i++)
    {
      sline = &sline_buf[index_dline];
      dline = &dline_buf[index_dline];
      sline->init(2, &vertArray[i]);
      dline->init(INCREASING, sline);
      ret->insert(dline);
      index_dline++;
    }
  return ret;
}
#endif

#if 0 // UNUSED
static int num_edges(Bin& bin)
{
  int sum=0;
  for(Arc_ptr jarc = bin.firstarc(); jarc; jarc=bin.nextarc())
    sum += jarc->pwlArc->npts-1;
  return sum;
}
#endif

/*
directedLine* bin_to_DLineLoops(Bin& bin)
{
  directedLine *ret=NULL;
  directedLine *temp;

  int numedges = num_edges(bin);
  directedLine* dline_buf = new directedLine[numedges]; //not work for N32?
  sampledLine* sline_buf=new sampledLine[numedges];

  Real2* vertArray = new Real2[numedges*2];
  int index = 0;
  int index_dline = 0;
  bin.markall();

  for(Arc_ptr jarc = bin.firstarc(); jarc; jarc=bin.nextarc())
    {
      if(jarc->ismarked())
	{
	  assert(jarc->check() != 0);
	  Arc_ptr jarchead = jarc;
	  do {
	    jarc->clearmark();
	    jarc = jarc->next;
	  } while(jarc != jarchead);
	  temp=copy_loop(jarchead, vertArray, index, dline_buf, sline_buf, index_dline);
	  ret = temp->insertPolygon(ret);
	}
    }

  return ret;
}
*/


directedLine* bin_to_DLineLoops(Bin& bin)
{
  directedLine *ret=NULL;
  directedLine *temp;
  bin.markall();
  for(Arc_ptr jarc=bin.firstarc(); jarc; jarc=bin.nextarc()){
    if(jarc->ismarked()) {
      assert(jarc->check() != 0);
      Arc_ptr jarchead = jarc;
      do {
	jarc->clearmark();
	jarc = jarc->next;
      } while(jarc != jarchead);
      temp =  arcLoopToDLineLoop(jarc);
      ret = temp->insertPolygon(ret);
    }
  }
  return ret;
}

directedLine* o_pwlcurve_to_DLines(directedLine* original, O_pwlcurve* pwl)
{
  directedLine* ret = original;
  for(Int i=0; i<pwl->npts-1; i++)
    {
      sampledLine* sline = new sampledLine(2);
      sline->setPoint(0, pwl->pts[i].param);
      sline->setPoint(1, pwl->pts[i+1].param);
      directedLine* dline = new directedLine(INCREASING, sline);
      if(ret == NULL)
	ret = dline;
      else
	ret->insert(dline);
    }
  return ret;      
}

directedLine* o_curve_to_DLineLoop(O_curve* cur)
{
  directedLine *ret;
  if(cur == NULL)
    return NULL;
  assert(cur->curvetype == ct_pwlcurve);
  ret = o_pwlcurve_to_DLines(NULL, cur->curve.o_pwlcurve);
  for(O_curve* temp = cur->next; temp != NULL; temp = temp->next)
    {
      assert(temp->curvetype == ct_pwlcurve);
      ret = o_pwlcurve_to_DLines(ret, temp->curve.o_pwlcurve);
    }
  return ret;
}

directedLine* o_trim_to_DLineLoops(O_trim* trim)
{
  O_trim* temp;
  directedLine *ret;
  if(trim == NULL)
    return NULL;
  ret = o_curve_to_DLineLoop(trim->o_curve);

  for(temp=trim->next; temp != NULL; temp = temp->next)
    {
      ret = ret->insertPolygon(o_curve_to_DLineLoop(temp->o_curve));
    }
  return ret;
}
