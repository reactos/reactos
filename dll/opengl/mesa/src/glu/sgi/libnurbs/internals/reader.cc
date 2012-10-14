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
 *  reader.c++
 *
 */

#include <stdio.h>
#include "glimports.h"
#include "nurbsconsts.h"
#include "reader.h"
#include "trimvertex.h"
#include "simplemath.h"

//when read a pwlCurve, if two consecutive points are the same, then 
//eliminate one of them. This makes the tessellator more robust. The spec
//assumes the application makes sure there are no redundant points. 
//but in Inspector, the trim curves seem to have redundant points a lot.
//I guess other similar users may have the same problem.

#define ELIMINATE_REDUNDANT_POINTS 

#ifdef  ELIMINATE_REDUNDANT_POINTS 
#define equal(x,y) ( glu_abs(x-y) <= 0.00001)
#endif

#ifdef ELIMINATE_REDUNDANT_POINTS 
O_pwlcurve::O_pwlcurve( long _type, long count, INREAL *array, long byte_stride, TrimVertex *trimpts )
{
    next = 0;
    used = 0;
    owner = 0;
    pts = trimpts;
    npts = (int) count;
    save = 0;
    int i;

    /* copy user data into internal trimming data structures */
    switch( _type ) {
        case N_P2D: {
	    TrimVertex *v = pts;
            TrimVertex *prev = NULL;
	    int num = 0;
	    int doit;
	    for(i=0; i<count; i++) {
	        doit = 1;
         	if(prev != NULL)
		  {
		    if(equal(prev->param[0], array[0]) && equal(prev->param[1], array[1]))
		      {
			doit = 0;
		      }
		  }
		
		if(doit)
		  {
		    v->param[0] = (REAL) array[0];
		    v->param[1] = (REAL) array[1];
		    prev = v;
		    v++;
                    num++;
		  }
		array = (INREAL *) (((char *) array) + byte_stride);
	      }
	    npts = num;
	    break;
	}
        case N_P2DR: {
	    TrimVertex *v = pts;
    	    for( TrimVertex *lastv = v + count; v != lastv; v++ ) {
	        v->param[0] = (REAL) array[0] / (REAL) array[2];
	        v->param[1] = (REAL) array[1] / (REAL) array[2];
		array = (INREAL *) (((char *) array) + byte_stride);
	    }
	    break;
	}
    }
}
#else
O_pwlcurve::O_pwlcurve( long _type, long count, INREAL *array, long byte_stride, TrimVertex *trimpts )
{
    next = 0;
    used = 0;
    owner = 0;
    pts = trimpts;
    npts = (int) count;
    save = 0;

    /* copy user data into internal trimming data structures */
    switch( _type ) {
        case N_P2D: {
	    TrimVertex *v = pts;
    	    for( TrimVertex *lastv = v + count; v != lastv; v++ ) {
	        v->param[0] = (REAL) array[0];
	        v->param[1] = (REAL) array[1];
		array = (INREAL *) (((char *) array) + byte_stride);
	    }
	    break;
	}
        case N_P2DR: {
	    TrimVertex *v = pts;
    	    for( TrimVertex *lastv = v + count; v != lastv; v++ ) {
	        v->param[0] = (REAL) array[0] / (REAL) array[2];
	        v->param[1] = (REAL) array[1] / (REAL) array[2];
		array = (INREAL *) (((char *) array) + byte_stride);
	    }
	    break;
	}
    }
}
#endif 





