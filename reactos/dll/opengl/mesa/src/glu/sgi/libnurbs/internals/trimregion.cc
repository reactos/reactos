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
 * trimregion.c++
 *
 */

#include "glimports.h"
#include "myassert.h"
#include "mystdio.h"
#include "trimregion.h"

TrimRegion::TrimRegion( void )
{
}

void
TrimRegion::setDu( REAL du )
{
    oneOverDu = 1.0/du;
}

void
TrimRegion::init( long npts, Arc_ptr extrema )
{
    left.init( npts, extrema, extrema->pwlArc->npts - 1 ); 
    left.getNextPt();

    right.init( npts, extrema, 0 ); 
    right.getPrevPt();
}

void
TrimRegion::getPts( Arc_ptr extrema )
{
    left.getNextPts( extrema );
    right.getPrevPts( extrema );
}

void
TrimRegion::getPts( Backend &backend )
{
    left.getNextPts( bot.vval, backend );
    right.getPrevPts( bot.vval, backend );
}

void 
TrimRegion::getGridExtent( void )
{
    getGridExtent( left.last(), right.last() );
}

void
TrimRegion::getGridExtent( TrimVertex *l, TrimVertex *r )
{
    bot.ustart = (long) ((l->param[0] - uarray.uarray[0])*oneOverDu);
    if( l->param[0] >= uarray.uarray[bot.ustart] ) bot.ustart++;
//  if( l->param[0] > uarray.uarray[bot.ustart] ) bot.ustart++;
    assert( l->param[0] <= uarray.uarray[bot.ustart] );
    assert( l->param[0] >= uarray.uarray[bot.ustart-1] );

    bot.uend = (long) ((r->param[0] - uarray.uarray[0])*oneOverDu);
    if( uarray.uarray[bot.uend] >= r->param[0] ) bot.uend--;
//  if( uarray.uarray[bot.uend] > r->param[0] ) bot.uend--;
    assert( r->param[0] >= uarray.uarray[bot.uend] );
    assert( r->param[0] <= uarray.uarray[bot.uend+1] );
}

int
TrimRegion::canTile( void )
{
    TrimVertex *lf = left.first();
    TrimVertex *ll = left.last();
    TrimVertex *l = ( ll->param[0] > lf->param[0] ) ? ll : lf; 

    TrimVertex *rf = right.first();
    TrimVertex *rl = right.last();
    TrimVertex *r = ( rl->param[0] < rf->param[0] ) ? rl : rf;
    return (l->param[0] <= r->param[0]) ? 1 : 0;
}

