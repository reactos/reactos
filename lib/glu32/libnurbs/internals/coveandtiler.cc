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
 * coveandtiler.c++
 *
 */

#include "glimports.h"
#include "myassert.h"
#include "mystdio.h"
#include "coveandtiler.h"
#include "gridvertex.h"
#include "gridtrimvertex.h"
#include "uarray.h"
#include "backend.h"


const int CoveAndTiler::MAXSTRIPSIZE = 1000;

CoveAndTiler::CoveAndTiler( Backend& b )
	    : backend( b )
{ }

CoveAndTiler::~CoveAndTiler( void )
{ }

inline void
CoveAndTiler::output( GridVertex &gv )
{
    backend.tmeshvert( &gv );
}

inline void
CoveAndTiler::output( TrimVertex *tv )
{
    backend.tmeshvert( tv );
}

inline void
CoveAndTiler::output( GridTrimVertex& g )
{
    backend.tmeshvert( &g );
}

void
CoveAndTiler::coveAndTile( void )
{
    long ustart = (top.ustart >= bot.ustart) ? top.ustart : bot.ustart;
    long uend	= (top.uend <= bot.uend)     ? top.uend   : bot.uend;
    if( ustart <= uend ) {
	tile( bot.vindex, ustart, uend );
	if( top.ustart >= bot.ustart )
	    coveUpperLeft();
	else
	    coveLowerLeft();

	if( top.uend <= bot.uend )
	    coveUpperRight();
	else
	    coveLowerRight();
    } else {
	TrimVertex blv, tlv, *bl, *tl;
	GridTrimVertex bllv, tllv;
	TrimVertex *lf = left.first();
	TrimVertex *ll = left.last();
	if( lf->param[0] >= ll->param[0] ) {
	    blv.param[0] = lf->param[0];
	    blv.param[1] = ll->param[1];
	    blv.nuid = 0; // XXX
	    assert( blv.param[1] == bot.vval );
	    bl = &blv;
	    tl = lf;
	    tllv.set( lf );
	    if( ll->param[0] > uarray.uarray[top.ustart-1] ) {
		bllv.set( ll );
		assert( ll->param[0] <= uarray.uarray[bot.ustart] );
	    } else {
		bllv.set( top.ustart-1, bot.vindex );
	    }
	    coveUpperLeftNoGrid( bl );
	} else {
	    tlv.param[0] = ll->param[0];
	    tlv.param[1] = lf->param[1];
	    tlv.nuid = 0; // XXX
	    assert( tlv.param[1] == top.vval );
	    tl = &tlv;
	    bl = ll;
	    bllv.set( ll );
	    if( lf->param[0] > uarray.uarray[bot.ustart-1] ) {
		assert( lf->param[0] <= uarray.uarray[bot.ustart] );
		tllv.set( lf );
	    } else {
		tllv.set( bot.ustart-1, top.vindex );
	    }
	    coveLowerLeftNoGrid( tl );
	}

	TrimVertex brv, trv, *br, *tr;
	GridTrimVertex brrv, trrv;
	TrimVertex *rf = right.first();
	TrimVertex *rl = right.last();

	if( rf->param[0] <= rl->param[0] ) {
	    brv.param[0] = rf->param[0];
	    brv.param[1] = rl->param[1];
	    brv.nuid = 0; // XXX
	    assert( brv.param[1] == bot.vval );
	    br = &brv;
	    tr = rf;
	    trrv.set( rf );
	    if( rl->param[0] < uarray.uarray[top.uend+1] ) {
		assert( rl->param[0] >= uarray.uarray[top.uend] );
		brrv.set( rl );
	    } else {
		brrv.set( top.uend+1, bot.vindex );
	    }
	    coveUpperRightNoGrid( br );
	} else {
	    trv.param[0] = rl->param[0];
	    trv.param[1] = rf->param[1];
	    trv.nuid = 0; // XXX
	    assert( trv.param[1] == top.vval );
	    tr = &trv;
	    br = rl;
	    brrv.set( rl );
	    if( rf->param[0] < uarray.uarray[bot.uend+1] ) {
		assert( rf->param[0] >= uarray.uarray[bot.uend] );
		trrv.set( rf );
	    } else {
		trrv.set( bot.uend+1, top.vindex );
	    }
	    coveLowerRightNoGrid( tr );
	}

	backend.bgntmesh( "doit" );
	output(trrv);
	output(tllv);
	output( tr );
	output( tl );
	output( br );
	output( bl );
	output(brrv);
	output(bllv);
	backend.endtmesh();
    }
}

void
CoveAndTiler::tile( long vindex, long ustart, long uend )
{
    long numsteps = uend - ustart;

    if( numsteps == 0 ) return;

    if( numsteps > MAXSTRIPSIZE ) {
	long umid = ustart + (uend - ustart) / 2;
	tile( vindex, ustart, umid );
	tile( vindex, umid, uend );
    } else {
	backend.surfmesh( ustart, vindex-1, numsteps, 1 );
    }
}

void
CoveAndTiler::coveUpperRight( void )
{
    GridVertex tgv( top.uend, top.vindex );
    GridVertex gv( top.uend, bot.vindex );

    right.first();
    backend.bgntmesh( "coveUpperRight" );
    output( right.next() );
    output( tgv );
    backend.swaptmesh();
    output( gv );
	coveUR();
    backend.endtmesh();
}

void
CoveAndTiler::coveUpperRightNoGrid( TrimVertex* br )
{
    backend.bgntmesh( "coveUpperRight" );
    output( right.first() );
    output( right.next() );
    backend.swaptmesh();
    output( br );
	coveUR();
    backend.endtmesh();
}

void
CoveAndTiler::coveUR( )
{
    GridVertex gv( top.uend, bot.vindex );
    TrimVertex *vert = right.next();
    if( vert == NULL ) return;

    assert( vert->param[0] >= uarray.uarray[gv.gparam[0]]  );

    if( gv.nextu() >= bot.uend ) {
	for( ; vert; vert = right.next() ) {
	    output( vert );
	    backend.swaptmesh();
	}
    } else while( 1 ) {
	if( vert->param[0] < uarray.uarray[gv.gparam[0]]  ) {
	    output( vert );
	    backend.swaptmesh();
	    vert = right.next();
	    if( vert == NULL ) break;
	} else {
	    backend.swaptmesh();
	    output( gv );
	    if( gv.nextu() == bot.uend ) {
		for( ; vert; vert = right.next() ) {
		    output( vert );
		    backend.swaptmesh();
		}
		break;
	    }
	}
    }
}

void
CoveAndTiler::coveUpperLeft( void )
{
    GridVertex tgv( top.ustart, top.vindex );
    GridVertex gv( top.ustart, bot.vindex );

    left.first();
    backend.bgntmesh( "coveUpperLeft" );
    output( tgv );
    output( left.next() );
    output( gv );
    backend.swaptmesh();
	coveUL();
    backend.endtmesh();
}

void
CoveAndTiler::coveUpperLeftNoGrid( TrimVertex* bl )
{
    backend.bgntmesh( "coveUpperLeftNoGrid" );
    output( left.first() );
    output( left.next() );
    output( bl );
    backend.swaptmesh();
	coveUL();
    backend.endtmesh();
}

void
CoveAndTiler::coveUL()
{
    GridVertex gv( top.ustart, bot.vindex );
    TrimVertex *vert = left.next();
    if( vert == NULL ) return;
    assert( vert->param[0] <= uarray.uarray[gv.gparam[0]]  );

    if( gv.prevu() <= bot.ustart ) {
	for( ; vert; vert = left.next() ) {
	    backend.swaptmesh();
	    output( vert );
	}
    } else while( 1 ) {
	if( vert->param[0] > uarray.uarray[gv.gparam[0]]  ) {
	    backend.swaptmesh();
	    output( vert );
	    vert = left.next();
	    if( vert == NULL ) break;
	} else {
	    output( gv );
	    backend.swaptmesh();
	    if( gv.prevu() == bot.ustart ) {
		for( ; vert; vert = left.next() ) {
		    backend.swaptmesh();
		    output( vert );
		}
		break;
	    }
	}
    }
}

void
CoveAndTiler::coveLowerLeft( void )
{
    GridVertex bgv( bot.ustart, bot.vindex );
    GridVertex gv( bot.ustart, top.vindex );

    left.last();
    backend.bgntmesh( "coveLowerLeft" );
    output( left.prev() );
    output( bgv );
    backend.swaptmesh();
    output( gv );
	coveLL();
    backend.endtmesh();
}

void
CoveAndTiler::coveLowerLeftNoGrid( TrimVertex* tl )
{
    backend.bgntmesh( "coveLowerLeft" );
    output( left.last() );
    output( left.prev() );
    backend.swaptmesh();
    output( tl );
	coveLL( );
    backend.endtmesh();
}

void
CoveAndTiler::coveLL()
{
    GridVertex gv( bot.ustart, top.vindex );
    TrimVertex *vert = left.prev();
    if( vert == NULL ) return;
    assert( vert->param[0] <= uarray.uarray[gv.gparam[0]]  );

    if( gv.prevu() <= top.ustart ) {
	for( ; vert; vert = left.prev() ) {
	    output( vert );
	    backend.swaptmesh();
	}
    } else while( 1 ) {
	if( vert->param[0] > uarray.uarray[gv.gparam[0]] ){
	    output( vert );
	    backend.swaptmesh();
	    vert = left.prev();
	    if( vert == NULL ) break;
	} else {
	    backend.swaptmesh();
	    output( gv );
	    if( gv.prevu() == top.ustart ) {
		for( ; vert; vert = left.prev() ) {
		    output( vert );
		    backend.swaptmesh();
		}
		break;
	    }
	}
    }
}

void
CoveAndTiler::coveLowerRight( void )
{
    GridVertex bgv( bot.uend, bot.vindex );
    GridVertex gv( bot.uend, top.vindex );

    right.last();
    backend.bgntmesh( "coveLowerRight" );       
    output( bgv );
    output( right.prev() );
    output( gv );
    backend.swaptmesh();
	coveLR();
    backend.endtmesh( );
}

void
CoveAndTiler::coveLowerRightNoGrid( TrimVertex* tr )
{
    backend.bgntmesh( "coveLowerRIght" );
    output( right.last() );
    output( right.prev() );
    output( tr );
    backend.swaptmesh();
	coveLR();
    backend.endtmesh();
}

void
CoveAndTiler::coveLR( )
{
    GridVertex gv( bot.uend, top.vindex );
    TrimVertex *vert = right.prev();
    if( vert == NULL ) return;
    assert( vert->param[0] >= uarray.uarray[gv.gparam[0]]  );

    if( gv.nextu() >= top.uend ) {
	for( ; vert; vert = right.prev() ) {
	    backend.swaptmesh();
	    output( vert );
	}
    } else while( 1 ) {
	if( vert->param[0] < uarray.uarray[gv.gparam[0]]  ) {
	    backend.swaptmesh();
	    output( vert );
	    vert = right.prev();
	    if( vert == NULL ) break;
	} else {
	    output( gv );
	    backend.swaptmesh();
	    if( gv.nextu() == top.uend ) {
		for( ; vert; vert = right.prev() ) {
		    backend.swaptmesh();
		    output( vert );
		}
		break;
	    }
	}
    }
}

