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
 * curvelist.c++
 *
 */

#include "glimports.h"
#include "myassert.h"
#include "mystdio.h"
#include "quilt.h"
#include "curvelist.h"
#include "curve.h"
#include "types.h"

Curvelist::Curvelist( Quilt *quilts, REAL pta, REAL ptb )
{
    curve = 0;
    for( Quilt *q = quilts; q; q = q->next ) 
	curve = new Curve( q, pta, ptb, curve );
    range[0] = pta;
    range[1] = ptb;
    range[2] = ptb - pta;
    needsSubdivision = 0;
    stepsize = 0;
}

Curvelist::Curvelist( Curvelist &upper, REAL value )
{
    curve = 0;
    for( Curve *c = upper.curve; c; c = c->next )
	curve = new Curve( *c, value, curve );

    range[0] = upper.range[0];
    range[1] = value;
    range[2] = value - upper.range[0];
    upper.range[0] = value;
    upper.range[2] = upper.range[1] - value;
    needsSubdivision = 0;
    stepsize = 0;
}

Curvelist::~Curvelist()
{
    while( curve ) {
	Curve *c = curve;
	curve = curve->next;
	delete c;
    }
}

int
Curvelist::cullCheck( void )
{
    for( Curve *c = curve; c; c = c->next )
	if( c->cullCheck() == CULL_TRIVIAL_REJECT )
	    return CULL_TRIVIAL_REJECT;
    return CULL_ACCEPT;
}

void
Curvelist::getstepsize( void )
{
    stepsize = range[2];
    Curve *c;
    for( c = curve; c; c = c->next ) {
	c->getstepsize();
	c->clamp();
	stepsize =  ((c->stepsize < stepsize) ? c->stepsize : stepsize);
	if( c->needsSamplingSubdivision() ) break;
    }
    needsSubdivision = ( c ) ? 1 : 0;
}

int
Curvelist::needsSamplingSubdivision( void )
{
    return needsSubdivision;
}

