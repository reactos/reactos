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
 * monotonizer.c++
 *
 */

#include "glimports.h"
#include "mystdio.h"
#include "myassert.h"
#include "arc.h"
#include "arctess.h"
#include "bezierarc.h"
#include "bin.h"
#include "mapdesc.h"
#include "nurbsconsts.h"
#include "subdivider.h"

/*-----------------------------------------------------------------------------
 * Subdivider::decompose - break all curves into monotone arcs
 *-----------------------------------------------------------------------------
 */
int
Subdivider::decompose( Bin& bin, REAL geo_stepsize )
{
    Arc_ptr jarc;
    for( jarc=bin.firstarc(); jarc; jarc=bin.nextarc() ) {
	if( ! jarc->isTessellated() ) {
	    /* points have not been transformed, therefore they may be either
	       homogeneous or inhomogeneous */
	    tessellate( jarc, geo_stepsize );
	    if( jarc->isDisconnected() || jarc->next->isDisconnected() ) 
		return 1;
	}
    }

    for( jarc=bin.firstarc(); jarc; jarc=bin.nextarc() ) {
	monotonize( jarc, bin );
    }

#ifndef NDEBUG
    for( jarc=bin.firstarc(); jarc; jarc=bin.nextarc() ) {
	assert( isMonotone( jarc ) != 0 );
    }
#endif

    return 0;
}

void
Subdivider::tessellate( Arc_ptr jarc, REAL geo_stepsize )
{
    BezierArc *b = jarc->bezierArc;
    Mapdesc *mapdesc = b->mapdesc;

    if( mapdesc->isRational() ) {
	REAL max = mapdesc->calcVelocityRational( b->cpts, b->stride, b->order );
	REAL arc_stepsize = (max > 1.0) ? (1.0/max) : 1.0; 
	if( jarc->bezierArc->order != 2 )
	    arctessellator.tessellateNonlinear( jarc, geo_stepsize, arc_stepsize, 1 );
	else {
	    arctessellator.tessellateLinear( jarc, geo_stepsize, arc_stepsize, 1 );
	}
    } else { 
	REAL max = mapdesc->calcVelocityNonrational( b->cpts, b->stride, b->order );
	REAL arc_stepsize = (max > 1.0) ? (1.0/max) : 1.0; 
	if( jarc->bezierArc->order != 2 )
	    arctessellator.tessellateNonlinear( jarc, geo_stepsize, arc_stepsize, 0 );
	else {
	    arctessellator.tessellateLinear( jarc, geo_stepsize, arc_stepsize, 0 );
	}
    }
}

/*-------------------------------------------------------------------------
 * Subdivider::monotonize - break up a jordan arc into s,t-monotone
 *	components.  This code will remove degenerate segments, including
 *	arcs of only a single point.
 *-------------------------------------------------------------------------
 */
void
Subdivider::monotonize( Arc_ptr jarc, Bin& bin )
{
    TrimVertex  *firstvert = jarc->pwlArc->pts;
    TrimVertex  *lastvert = firstvert + (jarc->pwlArc->npts - 1);
    long	uid = jarc->nuid;
    arc_side	side = jarc->getside();
    dir		sdir = none;
    dir		tdir = none;
    int 	degenerate = 1;

    int		nudegenerate;
    int		change;

    TrimVertex *vert;
    for( vert = firstvert; vert != lastvert; vert++ ) {

        nudegenerate = 1;
        change = 0;

	/* check change relative to s axis, clear degenerate bit if needed */
        REAL sdiff = vert[1].param[0] - vert[0].param[0]; 
        if( sdiff == 0 ) {
	    if( sdir != same ) {
	        sdir = same;
		change = 1;
	    }
        } else if( sdiff < 0.0 ) {
	    if( sdir != down ) {
	        sdir = down;
		change = 1;
	    }
	    nudegenerate = 0;
        } else {
	    if( sdir != up ) {
	        sdir = up;
		change = 1;
	    }
	    nudegenerate = 0;
        }
    
	/* check change relative to t axis, clear degenerate bit if needed */
        REAL tdiff = vert[1].param[1] - vert[0].param[1]; 
        if( tdiff == 0 ) { 
	    if( tdir != same ) {
	        tdir = same;
	 	change = 1;
	    }
        } else if( tdiff < 0.0 ) {
	    if( tdir != down ) {
	        tdir = down;
		change = 1;
	    }
	    nudegenerate = 0;
        } else {
	    if( tdir != up ) {
	        tdir = up;
		change = 1;
	    }
	    nudegenerate = 0;
        }
    
	if( change ) {
	    if( ! degenerate ) {
	        /* make last segment into separate pwl curve */
	        jarc->pwlArc->npts = vert - firstvert + 1;
	        jarc = (new(arcpool) Arc( side, uid ))->append( jarc );
	        jarc->pwlArc = new(pwlarcpool) PwlArc();
		bin.addarc( jarc );
	    }
	    firstvert = jarc->pwlArc->pts = vert;
	    degenerate = nudegenerate;
	} 
    }
    jarc->pwlArc->npts = vert - firstvert + 1;

    if( degenerate ) {
	/* remove jarc from circularly linked list */
	jarc->prev->next = jarc->next;
	jarc->next->prev = jarc->prev;

        assert( jarc->prev->check(  ) != 0 );
        assert( jarc->next->check(  ) != 0 );

	/* remove jarc from bin */
	bin.remove_this_arc( jarc  );

	jarc->pwlArc->deleteMe( pwlarcpool ); jarc->pwlArc = 0;
	jarc->deleteMe( arcpool );
    } 
}

/*-------------------------------------------------------------------------
 * Subdivider::isMonotone - return true if arc is monotone AND non-degenerate
 *-------------------------------------------------------------------------
 */
int
Subdivider::isMonotone( Arc_ptr jarc )
{
    TrimVertex  *firstvert = jarc->pwlArc->pts;
    TrimVertex  *lastvert = firstvert + (jarc->pwlArc->npts - 1);

    if( firstvert == lastvert ) return 1;

    TrimVertex	*vert = firstvert;
    enum dir	sdir;
    enum dir	tdir;

    REAL diff = vert[1].param[0] - vert[0].param[0]; 
    if( diff == 0.0 ) 
        sdir = same;
    else if( diff < 0.0 )
        sdir = down;
    else
        sdir = up;

    diff = vert[1].param[1] - vert[0].param[1]; 
    if( diff == 0.0 ) 
        tdir = same;
    else if( diff < 0.0 )
        tdir = down;
    else
        tdir = up;

    if( (sdir == same) && (tdir == same) ) return 0;
    
    for( ++vert ; vert != lastvert; vert++ ) {
        diff = vert[1].param[0] - vert[0].param[0]; 
        if( diff == 0.0 ) {
	    if( sdir != same ) return 0;
        } else if( diff < 0.0 ) {
	    if( sdir != down ) return 0;
        } else {
	    if( sdir != up ) return 0;
        }

        diff = vert[1].param[1] - vert[0].param[1]; 
        if( diff == 0.0 ) {
	    if( tdir != same ) return 0;
        } else if( diff < 0.0 ) {
	    if( tdir != down ) return 0;
        } else {
	    if( tdir != up ) return 0;
        }
    }
    return 1;
}

