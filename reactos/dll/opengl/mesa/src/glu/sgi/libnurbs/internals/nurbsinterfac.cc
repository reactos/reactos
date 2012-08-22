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
 * nurbsinterfac.c++
 *
 */

#include "glimports.h"
#include "mystdio.h"
#include "nurbsconsts.h"
#include "nurbstess.h"
#include "bufpool.h"
#include "quilt.h"
#include "displaylist.h"
#include "knotvector.h"
#include "mapdesc.h"

#define THREAD( work, arg, cleanup ) \
	if( dl ) {\
	    arg->save = 1;\
	    dl->append( (PFVS)&NurbsTessellator::work, (void *) arg, (PFVS)&NurbsTessellator::cleanup );\
 	} else {\
	    arg->save = 0;\
	    work( arg );\
	}

#define THREAD2( work ) \
	if( dl ) {\
	    dl->append( (PFVS)&NurbsTessellator::work, 0, 0 );\
 	} else {\
	    work( );\
	}

NurbsTessellator::NurbsTessellator( BasicCurveEvaluator &c, BasicSurfaceEvaluator& e) 
	: maplist( backend ),
	  backend( c, e ),
          subdivider( renderhints, backend ),
	  o_pwlcurvePool( sizeof( O_pwlcurve ), 32, "o_pwlcurvePool" ),
	  o_nurbscurvePool( sizeof( O_nurbscurve ), 32, "o_nurbscurvePool"),
	  o_curvePool( sizeof( O_curve ), 32,  "o_curvePool" ),
	  o_trimPool( sizeof( O_trim ), 32,  "o_trimPool" ),
	  o_surfacePool( sizeof( O_surface ), 1, "o_surfacePool" ),
	  o_nurbssurfacePool( sizeof( O_nurbssurface ), 4, "o_nurbssurfacePool" ),
	  propertyPool( sizeof( Property ), 32, "propertyPool" ),
          quiltPool( sizeof( Quilt  ), 32, "quiltPool" )
{
    dl		= 0;
    inSurface	= 0;
    inCurve	= 0;
    inTrim	= 0;
    playBack	= 0;
    jumpbuffer  = newJumpbuffer();
    subdivider.setJumpbuffer( jumpbuffer );
}

NurbsTessellator::~NurbsTessellator( void ) 
{
    if( inTrim ) {
	do_nurbserror( 12 );
	endtrim();
    }

    if( inSurface ) {
        *nextNurbssurface = 0;
        do_freeall();
    }

    if (jumpbuffer) {
        deleteJumpbuffer(jumpbuffer);
	jumpbuffer= 0;
    }	
}

/*-----------------------------------------------------------------------------
 * bgnsurface - allocate and initialize an o_surface structure
 *
 * Client: GL user
 *-----------------------------------------------------------------------------
 */
void
NurbsTessellator::bgnsurface( long nuid )
{
    O_surface *o_surface = new(o_surfacePool) O_surface;
    o_surface->nuid = nuid;
    THREAD( do_bgnsurface, o_surface, do_freebgnsurface );
}

/*-----------------------------------------------------------------------------
 * bgncurve - allocate an initialize an o_curve structure
 * 
 * Client: GL user
 *-----------------------------------------------------------------------------
 */
void
NurbsTessellator::bgncurve( long nuid )
{
    O_curve *o_curve = new(o_curvePool) O_curve;
    o_curve->nuid = nuid;
    THREAD( do_bgncurve, o_curve, do_freebgncurve );
}
/*-----------------------------------------------------------------------------
 * endcurve -
 * 
 * Client:
 *-----------------------------------------------------------------------------
 */

void
NurbsTessellator::endcurve( void )
{
    THREAD2( do_endcurve );
}

/*-----------------------------------------------------------------------------
 * endsurface - user level end of surface call
 *
 * Client: GL user
 *-----------------------------------------------------------------------------
 */
void
NurbsTessellator::endsurface( void )
{
    THREAD2( do_endsurface );
}


/*-----------------------------------------------------------------------------
 * bgntrim - allocate and initialize a new trim loop structure (o_trim )
 *
 * Client: GL user
 *-----------------------------------------------------------------------------
 */
void
NurbsTessellator::bgntrim( void )
{
    O_trim *o_trim = new(o_trimPool) O_trim;
    THREAD( do_bgntrim, o_trim, do_freebgntrim );
}

/*-----------------------------------------------------------------------------
 * endtrim -
 *
 * Client: GL user
 *-----------------------------------------------------------------------------
 */
void
NurbsTessellator::endtrim( void )
{
    THREAD2( do_endtrim );
}


/*-----------------------------------------------------------------------------
 * pwlcurve -
 *
 *      count        - number of points on curve
 *      array        - array of points on curve
 *      byte_stride  - distance between points in bytes
 *      type         - valid data flag
 *
 * Client: Gl user
 *-----------------------------------------------------------------------------
 */
void
NurbsTessellator::pwlcurve( long count, INREAL array[], long byte_stride, long type )
{
    Mapdesc *mapdesc = maplist.locate( type );

    if( mapdesc == 0 ) {
	do_nurbserror( 35 );
	isDataValid = 0;
	return;
    }

    if ( (type != N_P2D) && (type != N_P2DR) ) {
	do_nurbserror( 22 );
	isDataValid = 0;
	return;
    }
    if( count < 0 ) {
	do_nurbserror( 33 );
	isDataValid = 0;
	return;
    }
    if( byte_stride < 0 ) {
	do_nurbserror( 34 );
	isDataValid = 0;
	return;
    }

#ifdef NOTDEF
    if( mapdesc->isRational() ) {
	INREAL *p = array;
	INREAL x = p[0]; INREAL y = p[1]; INREAL w = p[2];
	p = (INREAL *) (((char *) p) + byte_stride);
	for( long i = 1; i != count; i++ ) {
	    if( p[0] == x && p[1] == y && p[2] == w ) break;
	    x = p[0]; y = p[1]; w = p[2];
	    p = (INREAL *) (((char *) p) + byte_stride);
	}
	if( i != count ) {
	    do_nurbserror( 37 );
	    _glu_dprintf( "point %d (%f,%f)\n", i, x, y );
	    isDataValid = 0;
	    return;
	}
    } else {
	INREAL *p = array;
	INREAL x = p[0]; INREAL y = p[1];
	p = (INREAL *) (((char *) p) + byte_stride);
	for( long i = 1; i != count; i++ ) {
	    if( p[0] == x && p[1] == y ) break;
	    x = p[0]; y = p[1];
	    p = (INREAL *) (((char *) p) + byte_stride);
	}
	if( i != count ) {
	    do_nurbserror( 37 );
	    _glu_dprintf( "point %d (%f,%f)\n", i, x, y );
	    isDataValid = 0;
	    return;
	}
    }
#endif

    O_pwlcurve	*o_pwlcurve = new(o_pwlcurvePool) O_pwlcurve( type, count, array, byte_stride, extTrimVertexPool.get((int)count) );
    THREAD( do_pwlcurve, o_pwlcurve, do_freepwlcurve );
}


/*-----------------------------------------------------------------------------
 * nurbscurve -
 *
 * Client: GL user
 *-----------------------------------------------------------------------------
 */
void
NurbsTessellator::nurbscurve(
    long nknots, 		/* number of p knots */
    INREAL knot[], 		/* nondecreasing knot values in p */
    long byte_stride,		/* distance in bytes between control points */
    INREAL ctlarray[], 		/* pointer to first control point */
    long order,			/* order of spline */
    long type )			/* description of range space */
{

    Mapdesc *mapdesc = maplist.locate( type );

    if( mapdesc == 0 ) {
	do_nurbserror( 35 );
	isDataValid = 0;
	return;
    }

    if( ctlarray == 0 ) {
	do_nurbserror( 36 );
	isDataValid = 0;
	return;
    }

    if( byte_stride < 0 ) {
	do_nurbserror( 34 );
	isDataValid = 0;
	return;
    }

    Knotvector knots;

    knots.init( nknots, byte_stride, order, knot );
    if( do_check_knots( &knots, "curve" ) ) return;
    
    O_nurbscurve *o_nurbscurve = new(o_nurbscurvePool) O_nurbscurve(type);
    o_nurbscurve->bezier_curves = new(quiltPool) Quilt(mapdesc);
    o_nurbscurve->bezier_curves->toBezier( knots,ctlarray, mapdesc->getNcoords() );
 
    THREAD( do_nurbscurve, o_nurbscurve, do_freenurbscurve );
}


/*-----------------------------------------------------------------------------
 * nurbssurface -
 *
 * Client: User routine
 *-----------------------------------------------------------------------------
 */
void
NurbsTessellator::nurbssurface(
    long sknot_count,		/* number of s knots */
    INREAL sknot[],		/* nondecreasing knot values in s */
    long tknot_count, 		/* number of t knots */
    INREAL tknot[],		/* nondecreasing knot values in t */
    long s_byte_stride,		/* s step size in memory bytes */
    long t_byte_stride,		/* t step size in memory bytes */
    INREAL ctlarray[],		/* pointer to first control point */
    long sorder,		/* order of the spline in s parameter */
    long torder,		/* order of the spline in t parameter */
    long type)			/* description of range space */
{ 
    Mapdesc *mapdesc = maplist.locate( type );

    if( mapdesc == 0 ) {
	do_nurbserror( 35 );
	isDataValid = 0;
	return;
    }

    if( s_byte_stride < 0 ) {
	do_nurbserror( 34 );
	isDataValid = 0;
	return;
    }

    if( t_byte_stride < 0 ) {
	do_nurbserror( 34 );
	isDataValid = 0;
	return;
    }

    Knotvector sknotvector, tknotvector;

    sknotvector.init( sknot_count, s_byte_stride, sorder, sknot );
    if( do_check_knots( &sknotvector, "surface" ) ) return;

    tknotvector.init( tknot_count, t_byte_stride, torder, tknot );
    if( do_check_knots( &tknotvector, "surface" ) ) return;

    O_nurbssurface *o_nurbssurface = new(o_nurbssurfacePool) O_nurbssurface(type);
    o_nurbssurface->bezier_patches = new(quiltPool) Quilt(mapdesc);

    o_nurbssurface->bezier_patches->toBezier( sknotvector, tknotvector,
	ctlarray, mapdesc->getNcoords() ); 
    THREAD( do_nurbssurface, o_nurbssurface, do_freenurbssurface );
}


/*-----------------------------------------------------------------------------
 * setnurbsproperty -
 * 
 *-----------------------------------------------------------------------------
 */
void
NurbsTessellator::setnurbsproperty( long tag, INREAL value )
{
    if( ! renderhints.isProperty( tag ) ) {
	do_nurbserror( 26 );
    } else {
	Property *prop = new(propertyPool) Property( tag, value );
	THREAD( do_setnurbsproperty, prop, do_freenurbsproperty );
    }
}

/*-----------------------------------------------------------------------------
 * setnurbsproperty -
 * 
 *-----------------------------------------------------------------------------
 */
void
NurbsTessellator::setnurbsproperty( long type, long tag, INREAL value )
{
    Mapdesc *mapdesc = maplist.locate( type );

    if( mapdesc == 0 ) {
	do_nurbserror( 35 );
	return;
    }

    if( ! mapdesc->isProperty( tag ) ) {
	do_nurbserror( 26 );
	return;
    }

    Property *prop = new(propertyPool) Property( type, tag, value );
    THREAD( do_setnurbsproperty2, prop, do_freenurbsproperty );
}


/*-----------------------------------------------------------------------------
 * getnurbsproperty - 
 * 
 *-----------------------------------------------------------------------------
 */

void
NurbsTessellator::getnurbsproperty( long tag, INREAL *value )
{
    if( renderhints.isProperty( tag ) ) {
	*value = renderhints.getProperty( tag );
    } else {
	do_nurbserror( 26 );
    }
}

/*-----------------------------------------------------------------------------
 * getnurbsproperty - 
 * 
 *-----------------------------------------------------------------------------
 */

void
NurbsTessellator::getnurbsproperty( long type, long tag, INREAL *value )
{
    Mapdesc *mapdesc = maplist.locate( type );

    if( mapdesc == 0 ) 
	do_nurbserror( 35 );

    if( mapdesc->isProperty( tag  ) ) {
	*value = mapdesc->getProperty( tag );
    } else {
	do_nurbserror( 26 );
    }
}

/*--------------------------------------------------------------------------
 * setnurbsproperty - accept a user supplied matrix as culling or sampling mat
 *--------------------------------------------------------------------------
 */

void 
NurbsTessellator::setnurbsproperty( long type, long purpose, INREAL *mat )
{
    // XXX - cannot be put in display list
    Mapdesc *mapdesc = maplist.locate( type );

    if( mapdesc == 0 ) {
	do_nurbserror( 35 );
	isDataValid = 0;
    } else if( purpose == N_BBOXSIZE ) {
	mapdesc->setBboxsize( mat );
    } else {
#ifndef NDEBUG
        _glu_dprintf( "ERRORRORRORR!!!\n");
#endif
    }
}

/*--------------------------------------------------------------------------
 * setnurbsproperty - accept a user supplied matrix as culling or sampling mat
 *--------------------------------------------------------------------------
 */

void 
NurbsTessellator::setnurbsproperty( long type, long purpose, INREAL *mat, 
    long rstride, long cstride )
{
    // XXX - cannot be put in display list
    Mapdesc *mapdesc = maplist.locate( type );

    if( mapdesc == 0 ) {
	do_nurbserror( 35 );
	isDataValid = 0;
    } else if( purpose == N_CULLINGMATRIX ) {
	mapdesc->setCmat( mat, rstride, cstride );
    } else if( purpose == N_SAMPLINGMATRIX ) {
	mapdesc->setSmat( mat, rstride, cstride );
    } else if( purpose == N_BBOXMATRIX ) {
	mapdesc->setBmat( mat, rstride, cstride );
    } else {
#ifndef NDEBUG
        _glu_dprintf( "ERRORRORRORR!!!\n");
#endif
    }
}

void	
NurbsTessellator::redefineMaps( void )
{
    maplist.initialize();
}

void 	
NurbsTessellator::defineMap( long type, long rational, long ncoords )
{
    maplist.define( type, (int) rational, (int) ncoords );
}

void 
NurbsTessellator::discardRecording( void *_dl )
{
    delete (DisplayList *) _dl;
}

void * 
NurbsTessellator::beginRecording( void )
{
    dl = new DisplayList( this );
    return (void *) dl;
}

void 
NurbsTessellator::endRecording( void )
{
    dl->endList();
    dl = 0;
}

void 
NurbsTessellator::playRecording( void *_dl )
{
    playBack = 1;
    bgnrender();
    ((DisplayList *)_dl)->play();
    endrender();
    playBack = 0;
}

