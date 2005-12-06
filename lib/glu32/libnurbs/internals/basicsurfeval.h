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
 * basicsurfeval.h
 *
 * $Date$ $Revision: 1.1 $
 * $Header: /cygdrive/c/RCVS/CVS/ReactOS/reactos/lib/glu32/libnurbs/internals/basicsurfeval.h,v 1.1 2004/02/02 16:39:10 navaraf Exp $
 */

#ifndef __glubasicsurfeval_h_
#define __glubasicsurfeval_h_

#include "types.h"
#include "displaymode.h"
#include "cachingeval.h"

class BasicSurfaceEvaluator : public CachingEvaluator {
public:
    virtual void	range2f( long, REAL *, REAL * );
    virtual void	domain2f( REAL, REAL, REAL, REAL );

    virtual void	enable( long );
    virtual void	disable( long );
    virtual void	bgnmap2f( long );
    virtual void	map2f( long, REAL, REAL, long, long, 
				     REAL, REAL, long, long, 
				     REAL *  );
    virtual void	mapgrid2f( long, REAL, REAL, long,  REAL, REAL );
    virtual void	mapmesh2f( long, long, long, long, long );
    virtual void	evalcoord2f( long, REAL, REAL );
    virtual void	evalpoint2i( long, long );
    virtual void	endmap2f( void );

    virtual void	polymode( long );
    virtual void 	bgnline( void );
    virtual void 	endline( void );
    virtual void 	bgnclosedline( void );
    virtual void 	endclosedline( void );
    virtual void 	bgntmesh( void );
    virtual void 	swaptmesh( void );
    virtual void 	endtmesh( void );
    virtual void 	bgnqstrip( void );
    virtual void 	endqstrip( void );

    virtual void 	bgntfan( void );
    virtual void 	endtfan( void );

    virtual void        evalUStrip(int n_upper, REAL v_upper, REAL* upper_val,
				     int n_lower, REAL v_lower, REAL* lower_val
      ) = 0;

    virtual void        evalVStrip(int n_left, REAL u_left, REAL* left_val,
				     int n_right, REAL u_right, REAL* right_val
      ) = 0;
    virtual void        inDoEvalCoord2NOGE(REAL u, REAL v, REAL* ret_point, REAL* ret_normal) = 0;
    virtual void        inDoEvalCoord2NOGE_BU(REAL u, REAL v, REAL* ret_point, REAL* ret_normal) = 0;
    virtual void        inDoEvalCoord2NOGE_BV(REAL u, REAL v, REAL* ret_point, REAL* ret_normal) = 0;
    virtual void inPreEvaluateBV_intfac(REAL v ) = 0;
    virtual void inPreEvaluateBU_intfac(REAL u ) = 0;
    
};

#endif /* __glubasicsurfeval_h_ */
