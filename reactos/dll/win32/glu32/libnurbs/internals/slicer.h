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
*/

/*
 * slicer.h
 *
 * $Date$ $Revision: 1.1 $
 * $Header: /cygdrive/c/RCVS/CVS/ReactOS/reactos/lib/glu32/libnurbs/internals/slicer.h,v 1.1 2004/02/02 16:39:12 navaraf Exp $
 */

#ifndef __gluslicer_h_
#define __gluslicer_h_

#include "trimregion.h"
#include "mesher.h"
#include "coveandtiler.h"
#include "primitiveStream.h"
#include "rectBlock.h"

class Backend;
class Arc;
class TrimVertex;

class Slicer : public CoveAndTiler, public Mesher {
public:
    			Slicer( Backend & );
			~Slicer( void );
    void		slice( Arc_ptr );
    void		slice_old( Arc_ptr);
    void		slice_new( Arc_ptr );
    void                evalStream(primStream* );
    void                evalRBArray(rectBlockArray* rbArray, gridWrap* grid);

    void		outline( Arc_ptr );
    void		setstriptessellation( REAL, REAL );
    void		setisolines( int );

    void                set_ulinear(int ulinear_flag)
      {
	ulinear = ulinear_flag;
      }
    void                set_vlinear(int vlinear_flag)
      {
	vlinear = vlinear_flag;
      }
private:
    Backend&		backend;
    REAL		oneOverDu;
    REAL		du, dv;
    int			isolines;

    void		outline( void );
    void		initGridlines( void );
    void		advanceGridlines( long );

    int                ulinear; //indicate whether uorder is 2 or not
    int                vlinear; //indicate whether vorder is 2 or not
};
#endif /* __gluslicer_h_ */
