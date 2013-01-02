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
 * curvesub.c++
 *
 */

#include "glimports.h"
#include "myassert.h"
#include "mystdio.h"
#include "subdivider.h"
#include "renderhints.h"
#include "backend.h"
#include "quilt.h"
#include "curvelist.h"
#include "nurbsconsts.h"

/*--------------------------------------------------------------------------
 * drawCurves - main curve rendering entry point
 *--------------------------------------------------------------------------
 */

void
Subdivider::drawCurves( void )
{
    REAL 	from[1], to[1];
    Flist	bpts;
    qlist->getRange( from, to, bpts );

    renderhints.init( );

    backend.bgncurv();
    for( int i=bpts.start; i<bpts.end-1; i++ ) {
        REAL pta, ptb;
	pta = bpts.pts[i];
	ptb = bpts.pts[i+1];

	qlist->downloadAll( &pta, &ptb, backend );

	Curvelist curvelist( qlist, pta, ptb );
	samplingSplit( curvelist, renderhints.maxsubdivisions );
    }
    backend.endcurv();
}


/*--------------------------------------------------------------------------
 * samplingSplit - recursively subdivide patch, cull check each subpatch  
 *--------------------------------------------------------------------------
 */

void
Subdivider::samplingSplit( Curvelist& curvelist, int subdivisions )
{
    if( curvelist.cullCheck() == CULL_TRIVIAL_REJECT )  return;

    curvelist.getstepsize();

    if( curvelist.needsSamplingSubdivision() && (subdivisions > 0) ) {
	REAL mid = ( curvelist.range[0] + curvelist.range[1] ) * 0.5;
	Curvelist lowerlist( curvelist, mid );
	samplingSplit( lowerlist, subdivisions-1 ); // lower
	samplingSplit( curvelist, subdivisions-1 ); // upper
    } else {
	long nu = 1 + ((long) (curvelist.range[2] / curvelist.stepsize));
	backend.curvgrid( curvelist.range[0], curvelist.range[1], nu );
	backend.curvmesh( 0, nu );
    }
}

