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
 * hull.c++
 *
 */

#include "glimports.h"
#include "myassert.h"
#include "mystdio.h"
#include "hull.h"
#include "gridvertex.h"
#include "gridtrimvertex.h"
#include "gridline.h"
#include "trimline.h"
#include "uarray.h"
#include "trimregion.h"

Hull::Hull( void )
{}

Hull::~Hull( void )
{}

/*----------------------------------------------------------------------
 * Hull:init - this routine does the initialization needed before any
 *	 	calls to nextupper or nextlower can be made.
 *----------------------------------------------------------------------
 */
void
Hull::init( void )
{
    TrimVertex *lfirst = left.first();
    TrimVertex *llast = left.last();
    if( lfirst->param[0] <= llast->param[0] ) {
	fakeleft.init( left.first() );
	upper.left = &fakeleft;
	lower.left = &left;
    } else {
	fakeleft.init( left.last() );
	lower.left = &fakeleft;
 	upper.left = &left;
    }
    upper.left->last();
    lower.left->first();

    if( top.ustart <= top.uend ) {
	upper.line = &top;
	upper.index = top.ustart;
    } else
	upper.line = 0;

    if( bot.ustart <= bot.uend ) {
	lower.line = &bot;
	lower.index = bot.ustart;
    } else
	lower.line = 0;

    TrimVertex *rfirst = right.first();
    TrimVertex *rlast = right.last();
    if( rfirst->param[0] <= rlast->param[0] ) {
	fakeright.init( right.last() );
	lower.right = &fakeright;
	upper.right = &right;
    } else {
	fakeright.init( right.first() );
	upper.right = &fakeright;
	lower.right = &right;
    }
    upper.right->first();
    lower.right->last();
}

/*----------------------------------------------------------------------
 * nextupper - find next vertex on upper hull of trim region.
 *		 - if vertex is on trim curve, set vtop point to 
 *		   that vertex.  if vertex is on grid, set vtop to
 *		   point to temporary area and stuff coordinants into
 *		   temporary vertex.  Also, place grid coords in temporary
 *		   grid vertex.
 *----------------------------------------------------------------------
 */
GridTrimVertex *
Hull::nextupper( GridTrimVertex *gv )
{
    if( upper.left ) {
	gv->set( upper.left->prev() );
	if( gv->isTrimVert() ) return gv;
	upper.left = 0;
    } 

    if( upper.line ) {
	assert( upper.index <= upper.line->uend );
	gv->set( uarray.uarray[upper.index], upper.line->vval );
	gv->set( upper.index, upper.line->vindex );
	if( upper.index++ == upper.line->uend ) upper.line = 0;
	return gv; 
    } 

    if( upper.right ) {
	gv->set( upper.right->next() );
	if( gv->isTrimVert() ) return gv;
	upper.right = 0;
    } 

    return 0; 
}

GridTrimVertex *
Hull::nextlower( register GridTrimVertex *gv )
{
    if( lower.left ) {
	gv->set( lower.left->next() );
	if( gv->isTrimVert() ) return gv;
	lower.left = 0;
    } 

    if( lower.line ) {
	gv->set( uarray.uarray[lower.index], lower.line->vval );
	gv->set( lower.index, lower.line->vindex );
	if( lower.index++ == lower.line->uend ) lower.line = 0;
	return gv;
    } 

    if( lower.right ) {
	gv->set( lower.right->prev() );
	if( gv->isTrimVert() ) return gv;
	lower.right = 0;
    } 

    return 0;
}

