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
 * nurbstess.c++
 *
 * $Date: 2004/02/02 16:39:12 $ $Revision: 1.1 $
 * $Header: /cygdrive/c/RCVS/CVS/ReactOS/reactos/lib/glu32/libnurbs/internals/nurbstess.cc,v 1.1 2004/02/02 16:39:12 navaraf Exp $
 */

#include "glimports.h"
#include "myassert.h"
#include "mysetjmp.h"
#include "mystdio.h"
#include "nurbsconsts.h"
#include "nurbstess.h"
#include "bufpool.h"
#include "quilt.h"
#include "knotvector.h"
#include "mapdesc.h"
#include "maplist.h"

void 
NurbsTessellator::set_domain_distance_u_rate(REAL u_rate)
{
  subdivider.set_domain_distance_u_rate(u_rate);
}

void 
NurbsTessellator::set_domain_distance_v_rate(REAL v_rate)
{
  subdivider.set_domain_distance_v_rate(v_rate);
}

void
NurbsTessellator::set_is_domain_distance_sampling(int flag)
{
  subdivider.set_is_domain_distance_sampling(flag);
}

void
NurbsTessellator::resetObjects( void )
{
    subdivider.clear();
}

void
NurbsTessellator::makeobj( int )
{
#ifndef NDEBUG
   dprintf( "makeobj\n" );
#endif
}

void
NurbsTessellator::closeobj( void )
{
#ifndef NDEBUG
   dprintf( "closeobj\n" );
#endif
}

void
NurbsTessellator::bgnrender( void )
{
#ifndef NDEBUG
   dprintf( "bgnrender\n" );
#endif
}

void
NurbsTessellator::endrender( void )
{
#ifndef NDEBUG
    dprintf( "endrender\n" );
#endif
}

/*-----------------------------------------------------------------------------
 * do_freebgnsurface - free o_surface structure 
 *
 * Client: do_freeall(), bgnsurface()
 *-----------------------------------------------------------------------------
 */
void
NurbsTessellator::do_freebgnsurface( O_surface *o_surface )
{
    o_surface->deleteMe( o_surfacePool );
}


/*-----------------------------------------------------------------------------
 * do_bgnsurface - begin the display of a surface
 *
 * Client: bgnsurface()
 *-----------------------------------------------------------------------------
 */
void
NurbsTessellator::do_bgnsurface( O_surface *o_surface )
{
    if( inSurface ) {
	do_nurbserror( 27 );
	endsurface();
    }
    inSurface = 1;

    if( ! playBack ) bgnrender();

    isTrimModified = 0;
    isSurfaceModified = 0;
    isDataValid = 1;
    numTrims = 0;
    currentSurface = o_surface;
    nextTrim = &( currentSurface->o_trim );
    nextNurbssurface = &( currentSurface->o_nurbssurface );
}

/*-----------------------------------------------------------------------------
 * do_bgncurve - begin the display of a curve 
 * 
 * Client: bgncurve()
 *-----------------------------------------------------------------------------
 */
void
NurbsTessellator::do_bgncurve( O_curve *o_curve )
{
    if ( inCurve ) {
	do_nurbserror( 6 );
	endcurve();
    }

    inCurve = 1;
    currentCurve = o_curve;
    currentCurve->curvetype = ct_none;

    if( inTrim ) {
        if( *nextCurve != o_curve ) {
	    isCurveModified = 1;
	    *nextCurve = o_curve;
	}
    } else {
        if( ! playBack ) bgnrender();
        isDataValid = 1;
    }
    nextCurve = &(o_curve->next);
    nextPwlcurve = &(o_curve->curve.o_pwlcurve);
    nextNurbscurve = &(o_curve->curve.o_nurbscurve);
}

/*-----------------------------------------------------------------------------
 * do_endcurve -
 * 
 * Client: endcurve()
 *-----------------------------------------------------------------------------
 */
    
void
NurbsTessellator::do_endcurve( void )
{
    if( ! inCurve ) {
	do_nurbserror( 7 );
	return;
    }
    inCurve = 0;

    *nextCurve = 0;
    if (currentCurve->curvetype == ct_nurbscurve)
	*nextNurbscurve = 0;
    else
	*nextPwlcurve = 0;

    if ( ! inTrim ) {
        if( ! isDataValid ) {
            do_freecurveall( currentCurve ); 
	    return;
        }

	int errval;
	errval = ::mysetjmp( jumpbuffer );
	if( errval == 0 ) {
	    if( currentCurve->curvetype == ct_nurbscurve ) {
		subdivider.beginQuilts();
		for( O_nurbscurve *n = currentCurve->curve.o_nurbscurve; n != 0; n = n->next ) 
		    subdivider.addQuilt( n->bezier_curves );
		subdivider.endQuilts();
		subdivider.drawCurves(); 
		if( ! playBack ) endrender();
	    } else {
		/* XXX */
	        if( ! playBack ) endrender();
	        /*do_draw_pwlcurve( currentCurve->curve.o_pwlcurve ) */;
	        do_nurbserror( 9 );
	    }
	} else {
	    if( ! playBack ) endrender();
	    do_nurbserror( errval );
	}
	do_freecurveall( currentCurve );
	resetObjects();
    }
}

/*-----------------------------------------------------------------------------
 * do_endsurface - mark end of surface, display surface, free immediate data 
 *
 * Client:
 *-----------------------------------------------------------------------------
 */
void
NurbsTessellator::do_endsurface( void )
{
    if( inTrim ) {
	do_nurbserror( 12 );
	endtrim();
    }

    if( ! inSurface ) {
	do_nurbserror( 13 );
	return;
    }
    inSurface = 0;

    *nextNurbssurface = 0;

    if( ! isDataValid ) {
        do_freeall( ); 
	return;
    }

    if( *nextTrim != 0 ) {
	isTrimModified = 1;
        *nextTrim = 0;
    }

    int errval;

    errval = ::mysetjmp( jumpbuffer );
    if( errval == 0 ) {
        if( numTrims > 0 ) {

	    subdivider.beginTrims();
	    for( O_trim	*trim = currentSurface->o_trim; trim; trim = trim->next ) {
		subdivider.beginLoop();
		for( O_curve *curve = trim->o_curve; curve; curve = curve->next ) {  
		    curve->used = 0;
		    assert( curve->curvetype != ct_none );
		    if (curve->curvetype == ct_pwlcurve) {
			O_pwlcurve *c = curve->curve.o_pwlcurve; 
			subdivider.addArc( c->npts, c->pts, curve->nuid );
		    } else {
			Quilt	   *quilt = curve->curve.o_nurbscurve->bezier_curves;
			Quiltspec  *qspec = quilt->qspec;
			REAL       *cpts  = quilt->cpts + qspec->offset;
			REAL       *cptsend = cpts + (qspec->width * qspec->order * qspec->stride);
			for( ; cpts != cptsend; cpts += qspec->order*qspec->stride ) 
			     subdivider.addArc( cpts, quilt, curve->nuid );
		    }
		}
		subdivider.endLoop();
	    }
	    subdivider.endTrims();
	}

	subdivider.beginQuilts();
	for( O_nurbssurface *n = currentSurface->o_nurbssurface; n; n = n->next ) 
	    subdivider.addQuilt( n->bezier_patches );
	subdivider.endQuilts();
        subdivider.drawSurfaces( currentSurface->nuid ); 
	if( ! playBack ) endrender();
    } else {
	if( ! playBack ) endrender();
	do_nurbserror( errval );
    }

    do_freeall( );
    resetObjects();
}

/*-----------------------------------------------------------------------------
 * do_freeall - free all data allocated in immediate mode
 *
 * Client:
 *-----------------------------------------------------------------------------
 */
void
NurbsTessellator::do_freeall( void )
{
    for( O_trim *o_trim = currentSurface->o_trim; o_trim; ) {
	O_trim *next_o_trim = o_trim->next;
        for( O_curve *curve = o_trim->o_curve; curve; ) {
	    O_curve *next_o_curve = curve->next;
	    do_freecurveall( curve );
	    curve = next_o_curve;
	}
	if( o_trim->save == 0 ) do_freebgntrim( o_trim );
	o_trim = next_o_trim;
    }

    O_nurbssurface *nurbss, *next_nurbss;
    for( nurbss= currentSurface->o_nurbssurface; nurbss; nurbss = next_nurbss) {
	next_nurbss = nurbss->next;
	if( nurbss->save == 0 )
	    do_freenurbssurface( nurbss );
	else
	    nurbss->used = 0;
    }

    if( currentSurface->save == 0 ) do_freebgnsurface( currentSurface );
}

void
NurbsTessellator::do_freecurveall( O_curve *curve )
{
    assert( curve->curvetype != ct_none );

    if( curve->curvetype == ct_nurbscurve ) {
	O_nurbscurve *ncurve, *next_ncurve;
	for( ncurve=curve->curve.o_nurbscurve; ncurve; ncurve=next_ncurve ) {
	    next_ncurve = ncurve->next;
	    if( ncurve->save == 0 )
		do_freenurbscurve( ncurve );
	    else
		ncurve->used = 0;
	}
    } else {
	O_pwlcurve *pcurve, *next_pcurve;
	for( pcurve=curve->curve.o_pwlcurve; pcurve; pcurve=next_pcurve ) {
	    next_pcurve = pcurve->next;
	    if( pcurve->save == 0 )
		do_freepwlcurve( pcurve );
	    else
		pcurve->used = 0;
	}
    }
    if( curve->save == 0 )
        do_freebgncurve( curve );
}


/*-----------------------------------------------------------------------------
 * do_freebgntrim - free the space allocated for a trim loop
 *
 * Client:
 *-----------------------------------------------------------------------------
 */
void
NurbsTessellator::do_freebgntrim( O_trim *o_trim )
{ 
    o_trim->deleteMe( o_trimPool );
}


/*-----------------------------------------------------------------------------
 * do_bgntrim - link in a trim loop to the current trimmed surface description
 *
 * Client: bgntrim()
 *-----------------------------------------------------------------------------
 */
void
NurbsTessellator::do_bgntrim( O_trim *o_trim )
{

    if( ! inSurface ) {
	do_nurbserror( 15 );
	bgnsurface( 0 );
	inSurface = 2;
    }

    if( inTrim ) {
	do_nurbserror( 16 );
	endtrim();
    }
    inTrim = 1;

    if( *nextTrim != o_trim ) {
	isTrimModified = 1;	
        *nextTrim = o_trim;
    }

    currentTrim = o_trim;
    nextTrim = &(o_trim->next);
    nextCurve = &(o_trim->o_curve);
}


/*-----------------------------------------------------------------------------
 * do_endtrim - mark the end of the current trim loop 
 *
 * Client: endtrim()
 *-----------------------------------------------------------------------------
 */
void
NurbsTessellator::do_endtrim( void )
{
    if( ! inTrim ) {
	do_nurbserror( 17 );
	return;
    }
    inTrim = 0;

    if( currentTrim->o_curve == 0 ) {
	do_nurbserror( 18 );
	isDataValid = 0;
    }

    numTrims++;
   
    if( *nextCurve != 0 ) {
	isTrimModified = 1;
        *nextCurve = 0;	
    }
}

/*-----------------------------------------------------------------------------
 * do_freepwlcurve -
 * 
 * Client:
 *-----------------------------------------------------------------------------
 */
void
NurbsTessellator::do_freepwlcurve( O_pwlcurve *o_pwlcurve )
{
    o_pwlcurve->deleteMe( o_pwlcurvePool );
}

void
NurbsTessellator::do_freebgncurve( O_curve *o_curve )
{
    o_curve->deleteMe( o_curvePool );
}

/*-----------------------------------------------------------------------------
 * do_pwlcurve - link in pwl trim loop to the current surface description
 * 
 * Client: pwlcurve()
 *-----------------------------------------------------------------------------
 */
void
NurbsTessellator::do_pwlcurve( O_pwlcurve *o_pwlcurve )
{
    if( ! inTrim ) {
	do_nurbserror( 19 );
	if( o_pwlcurve->save == 0 )
	    do_freepwlcurve(o_pwlcurve );
	return;
    }

    if( ! inCurve ) {
	bgncurve( 0 );
	inCurve = 2;
    }

    if( o_pwlcurve->used ) {
	do_nurbserror( 20 );
	isDataValid = 0;
	return;
    } else
        o_pwlcurve->used = 1;

    if( currentCurve->curvetype == ct_none ) {
        currentCurve->curvetype = ct_pwlcurve;
    } else if( currentCurve->curvetype != ct_pwlcurve ) {
	do_nurbserror( 21 );
	isDataValid = 0;
	return;
    }
	
    if( *nextPwlcurve != o_pwlcurve ) {
	isCurveModified = 1;
        *nextPwlcurve = o_pwlcurve;
    }
    nextPwlcurve = &(o_pwlcurve->next);

    if( o_pwlcurve->owner != currentCurve ) {
	isCurveModified = 1;
	o_pwlcurve->owner = currentCurve;
    }

    if( (inCurve == 2) ) 
	endcurve();
}


/*-----------------------------------------------------------------------------
 * do_freenurbscurve -
 * 
 * Client:
 *-----------------------------------------------------------------------------
 */
void
NurbsTessellator::do_freenurbscurve( O_nurbscurve *o_nurbscurve )
{
    o_nurbscurve->bezier_curves->deleteMe( quiltPool );
    o_nurbscurve->deleteMe( o_nurbscurvePool );
}


/*-----------------------------------------------------------------------------
 * do_nurbscurve -
 * 
 * Client: nurbscurve() 
 *-----------------------------------------------------------------------------
 */
void
NurbsTessellator::do_nurbscurve( O_nurbscurve *o_nurbscurve )
{
    if ( ! inCurve ) {
	bgncurve( 0 );
	inCurve = 2;
    }

    if( o_nurbscurve->used ) {
	/* error - curve was already called in current surface */
	do_nurbserror( 23 );
	isDataValid = 0;
	return;
    } else
        o_nurbscurve->used = 1;

    if( currentCurve->curvetype == ct_none ) {
        currentCurve->curvetype = ct_nurbscurve;
    } else if( currentCurve->curvetype != ct_nurbscurve ) {
	do_nurbserror( 24 );
	isDataValid = 0;
	return;
    }
	
    if( *nextNurbscurve != o_nurbscurve ) {
	isCurveModified = 1;
	*nextNurbscurve = o_nurbscurve;
    }

    nextNurbscurve = &(o_nurbscurve->next);

    if( o_nurbscurve->owner != currentCurve ) {
	isCurveModified = 1;
	o_nurbscurve->owner = currentCurve;
    }

    if( o_nurbscurve->owner == 0 )
	isCurveModified = 1;
    
    if( inCurve == 2 )
        endcurve();
}


/*-----------------------------------------------------------------------------
 * do_freenurbssurface -
 *
 * Client:
 *-----------------------------------------------------------------------------
 */

void
NurbsTessellator::do_freenurbssurface( O_nurbssurface *o_nurbssurface )
{
    o_nurbssurface->bezier_patches->deleteMe( quiltPool );
    o_nurbssurface->deleteMe( o_nurbssurfacePool );
}

/*-----------------------------------------------------------------------------
 * do_nurbssurface -
 * 
 * Client: nurbssurface()
 *-----------------------------------------------------------------------------
 */
void
NurbsTessellator::do_nurbssurface( O_nurbssurface *o_nurbssurface )
{
    if( ! inSurface ) {
	bgnsurface( 0 );
	inSurface = 2;
    }

    if( o_nurbssurface->used ) {
	/* error - surface was already called in current block */
	do_nurbserror( 25 );
	isDataValid = 0;
	return;
    } else
        o_nurbssurface->used = 1;

    if( *nextNurbssurface != o_nurbssurface ) {
	isSurfaceModified = 1;
        *nextNurbssurface  = o_nurbssurface;
    }

    if( o_nurbssurface->owner != currentSurface ) {
	isSurfaceModified = 1;
	o_nurbssurface->owner = currentSurface;
    }
    nextNurbssurface = &(o_nurbssurface->next);

    if( inSurface == 2  )
	endsurface();
}


/*-----------------------------------------------------------------------------
 * do_freenurbsproperty
 * 
 *-----------------------------------------------------------------------------
 */

void
NurbsTessellator::do_freenurbsproperty( Property *prop )
{
    prop->deleteMe( propertyPool );
}

    
/*-----------------------------------------------------------------------------
 * do_setnurbsproperty -
 * 
 *-----------------------------------------------------------------------------
 */

void
NurbsTessellator::do_setnurbsproperty( Property *prop )
{
    renderhints.setProperty( prop->tag, prop->value );
    if( prop->save == 0 )
	do_freenurbsproperty( prop );
}

void
NurbsTessellator::do_setnurbsproperty2( Property *prop )
{
    Mapdesc *mapdesc = maplist.find( prop->type );

    mapdesc->setProperty( prop->tag, prop->value );
    if( prop->save == 0 )
	do_freenurbsproperty( prop );
}

void
NurbsTessellator::errorHandler( int )
{
}

void
NurbsTessellator::do_nurbserror( int msg )
{
    errorHandler( msg );
}

int 
NurbsTessellator::do_check_knots( Knotvector *knots, char *msg )
{
    int status = knots->validate();
    if( status ) {
	do_nurbserror( status );
        if( renderhints.errorchecking != N_NOMSG ) knots->show( msg );
    }
    return status;
}





