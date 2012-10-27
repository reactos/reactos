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
 * patchlist.c++
 *
 */

#include <stdio.h>
#include "glimports.h"
#include "myassert.h"
#include "mystdio.h"
#include "quilt.h"
#include "patchlist.h"
#include "patch.h"

Patchlist::Patchlist( Quilt *quilts, REAL *pta, REAL *ptb )
{
    patch = 0;
    for( Quilt *q = quilts; q; q = q->next ) 
	patch = new Patch( q, pta, ptb, patch );
    pspec[0].range[0] = pta[0];
    pspec[0].range[1] = ptb[0];
    pspec[0].range[2] = ptb[0] - pta[0];
 
    pspec[1].range[0] = pta[1];
    pspec[1].range[1] = ptb[1];
    pspec[1].range[2] = ptb[1] - pta[1];
}

Patchlist::Patchlist( Patchlist &upper, int param,  REAL value)
{
    Patchlist &lower = *this;
    patch = 0;
    for( Patch *p = upper.patch; p; p = p->next )
	patch = new Patch( *p, param, value, patch );

    if( param == 0 ) {
	lower.pspec[0].range[0] = upper.pspec[0].range[0];
	lower.pspec[0].range[1] = value;
	lower.pspec[0].range[2] = value - upper.pspec[0].range[0];
	upper.pspec[0].range[0] = value;
	upper.pspec[0].range[2] = upper.pspec[0].range[1] - value;
	lower.pspec[1] = upper.pspec[1];
    } else {
	lower.pspec[0] = upper.pspec[0];
	lower.pspec[1].range[0] = upper.pspec[1].range[0];
	lower.pspec[1].range[1] = value;
	lower.pspec[1].range[2] = value - upper.pspec[1].range[0];
	upper.pspec[1].range[0] = value;
	upper.pspec[1].range[2] = upper.pspec[1].range[1] - value;
    }
}

Patchlist::~Patchlist()
{
    while( patch ) {
	Patch *p = patch;
	patch = patch->next;
	delete p;
    }
}

int
Patchlist::cullCheck( void )
{
    for( Patch *p = patch; p; p = p->next )
	if( p->cullCheck() == CULL_TRIVIAL_REJECT )
	    return CULL_TRIVIAL_REJECT;
    return CULL_ACCEPT;
}

void
Patchlist::getRanges(REAL ranges[4])
{
  ranges[0] = pspec[0].range[0];
  ranges[1] = pspec[0].range[1];
  ranges[2] = pspec[1].range[0];
  ranges[3] = pspec[1].range[1];
}

void
Patchlist::getstepsize( void )
{
    pspec[0].stepsize    = pspec[0].range[2];
    pspec[0].sidestep[0] = pspec[0].range[2];
    pspec[0].sidestep[1] = pspec[0].range[2];

    pspec[1].stepsize    = pspec[1].range[2];
    pspec[1].sidestep[0] = pspec[1].range[2];
    pspec[1].sidestep[1] = pspec[1].range[2];

    for( Patch *p = patch; p; p = p->next ) {
	p->getstepsize();
	p->clamp();
	pspec[0].stepsize    =  ((p->pspec[0].stepsize < pspec[0].stepsize) ? p->pspec[0].stepsize : pspec[0].stepsize);
	pspec[0].sidestep[0] =  ((p->pspec[0].sidestep[0] < pspec[0].sidestep[0]) ? p->pspec[0].sidestep[0] : pspec[0].sidestep[0]);
	pspec[0].sidestep[1] =  ((p->pspec[0].sidestep[1] < pspec[0].sidestep[1]) ? p->pspec[0].sidestep[1] : pspec[0].sidestep[1]);
	pspec[1].stepsize    =  ((p->pspec[1].stepsize < pspec[1].stepsize) ? p->pspec[1].stepsize : pspec[1].stepsize);
	pspec[1].sidestep[0] =  ((p->pspec[1].sidestep[0] < pspec[1].sidestep[0]) ? p->pspec[1].sidestep[0] : pspec[1].sidestep[0]);
	pspec[1].sidestep[1] =  ((p->pspec[1].sidestep[1] < pspec[1].sidestep[1]) ? p->pspec[1].sidestep[1] : pspec[1].sidestep[1]);
    }
}

void
Patchlist::bbox( void )
{
    for( Patch *p = patch; p; p = p->next )
	p->bbox();
}

int
Patchlist::needsNonSamplingSubdivision( void )
{
    notInBbox = 0;
    for( Patch *p = patch; p; p = p->next )
	notInBbox |= p->needsNonSamplingSubdivision();
    return notInBbox;
}

int
Patchlist::needsSamplingSubdivision( void )
{
    pspec[0].needsSubdivision = 0;
    pspec[1].needsSubdivision = 0;

    for( Patch *p = patch; p; p = p->next ) {
	pspec[0].needsSubdivision |= p->pspec[0].needsSubdivision;
	pspec[1].needsSubdivision |= p->pspec[0].needsSubdivision;
    }
    return (pspec[0].needsSubdivision || pspec[1].needsSubdivision) ? 1 : 0;
}

int
Patchlist::needsSubdivision( int param )
{
    return pspec[param].needsSubdivision;
}
