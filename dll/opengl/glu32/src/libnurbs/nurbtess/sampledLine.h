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

#ifndef _SAMPLEDLINE_H
#define _SAMPLEDLINE_H

#include "definitions.h"

class sampledLine{
  Int npoints;
  Real2 *points;

public:
  sampledLine(Int n_points);
  sampledLine(Int n_points, Real  pts[][2]);
  sampledLine(Real pt1[2], Real pt2[2]);
  sampledLine(); //special, careful about memory
  ~sampledLine();

  void init(Int n_points, Real2 *pts);//special, careful about memory

  void setPoint(Int i, Real p[2]) ;

  sampledLine* insert(sampledLine *nline);
  void deleteList();

  Int get_npoints() {return npoints;}
  Real2* get_points() {return points;}

  //u_reso is number of segments (may not be integer) per unit u
  void tessellate(Real u_reso, Real v_reso);//n segments
  void tessellateAll(Real u_reso, Real v_reso);

  void print();
  
  sampledLine* next;
};




#endif
