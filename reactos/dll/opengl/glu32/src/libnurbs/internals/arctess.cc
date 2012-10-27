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
 * arctessellator.c++
 *
 */

#include "glimports.h"
#include "mystdio.h"
#include "myassert.h"
#include "arctess.h"
#include "bufpool.h"
#include "simplemath.h"
#include "bezierarc.h"
#include "trimvertex.h"
#include "trimvertpool.h"

#define NOELIMINATION

#define steps_function(large, small, rate) (max(1, 1+ (int) ((large-small)/rate)));

/*-----------------------------------------------------------------------------
 * ArcTessellator - construct an ArcTessellator
 *-----------------------------------------------------------------------------
 */

ArcTessellator::ArcTessellator( TrimVertexPool& t, Pool& p ) 
	: pwlarcpool(p), trimvertexpool(t)
{
}

/*-----------------------------------------------------------------------------
 * ~ArcTessellator - destroy an ArcTessellator
 *-----------------------------------------------------------------------------
 */

ArcTessellator::~ArcTessellator( void )
{
}

/*-----------------------------------------------------------------------------
 * bezier - construct a bezier arc and attach it to an Arc
 *-----------------------------------------------------------------------------
 */

void
ArcTessellator::bezier( Arc *arc, REAL s1, REAL s2, REAL t1, REAL t2 )
{
    assert( arc != 0 );
    assert( ! arc->isTessellated() );

#ifndef NDEBUG
    switch( arc->getside() ) {
	case arc_left:
	    assert( s1 == s2 );
	    assert( t2 < t1 );
	    break;
	case arc_right:
	    assert( s1 == s2 );
	    assert( t1 < t2 );
	    break;
	case arc_top:
	    assert( t1 == t2 );
	    assert( s2 < s1 );
	    break;
	case arc_bottom:
	    assert( t1 == t2 );
	    assert( s1 < s2 );
	    break;
	case arc_none:
	    (void) abort();
	    break;
    }
#endif
    
    TrimVertex *p = trimvertexpool.get(2);
    arc->pwlArc = new(pwlarcpool) PwlArc( 2, p );
    p[0].param[0] = s1;
    p[0].param[1] = t1;
    p[1].param[0] = s2;
    p[1].param[1] = t2;
    assert( (s1 == s2) || (t1 == t2) );
    arc->setbezier();
}


/*-----------------------------------------------------------------------------
 * pwl_left - construct a left boundary pwl arc and attach it to an arc
 *-----------------------------------------------------------------------------
 */

void
ArcTessellator::pwl_left( Arc *arc, REAL s, REAL t1, REAL t2, REAL rate )
{
    assert( t2 < t1 );

/*    if(rate <= 0.06) rate = 0.06;*/
/*    int nsteps = 1 + (int) ((t1 - t2) / rate ); */
    int nsteps = steps_function(t1, t2, rate);


    REAL stepsize = (t1 - t2) / (REAL) nsteps;

    TrimVertex *newvert = trimvertexpool.get( nsteps+1 );
    int i;
    for( i = nsteps; i > 0; i-- ) {
	newvert[i].param[0] = s;
	newvert[i].param[1] = t2;
	t2 += stepsize;
    }
    newvert[i].param[0] = s;
    newvert[i].param[1] = t1;

    arc->makeSide( new(pwlarcpool) PwlArc( nsteps+1, newvert ), arc_left );
}

/*-----------------------------------------------------------------------------
 * pwl_right - construct a right boundary pwl arc and attach it to an arc
 *-----------------------------------------------------------------------------
 */

void
ArcTessellator::pwl_right( Arc *arc, REAL s, REAL t1, REAL t2, REAL rate )
{
    assert( t1 < t2 );

/*    if(rate <= 0.06) rate = 0.06;*/

/*    int nsteps = 1 + (int) ((t2 - t1) / rate ); */
    int nsteps = steps_function(t2,t1,rate);
    REAL stepsize = (t2 - t1) / (REAL) nsteps;

    TrimVertex *newvert = trimvertexpool.get( nsteps+1 );
    int i;
    for( i = 0; i < nsteps; i++ ) {
	newvert[i].param[0] = s;
	newvert[i].param[1] = t1;
	t1 += stepsize;
    }
    newvert[i].param[0] = s;
    newvert[i].param[1] = t2;

    arc->makeSide( new(pwlarcpool) PwlArc( nsteps+1, newvert ), arc_right );
}


/*-----------------------------------------------------------------------------
 * pwl_top - construct a top boundary pwl arc and attach it to an arc
 *-----------------------------------------------------------------------------
 */

void
ArcTessellator::pwl_top( Arc *arc, REAL t, REAL s1, REAL s2, REAL rate )
{
    assert( s2 < s1 );

/*    if(rate <= 0.06) rate = 0.06;*/

/*    int nsteps = 1 + (int) ((s1 - s2) / rate ); */
    int nsteps = steps_function(s1,s2,rate);
    REAL stepsize = (s1 - s2) / (REAL) nsteps;

    TrimVertex *newvert = trimvertexpool.get( nsteps+1 );
    int i;
    for( i = nsteps; i > 0; i-- ) {
	newvert[i].param[0] = s2;
	newvert[i].param[1] = t;
	s2 += stepsize;
    }
    newvert[i].param[0] = s1;
    newvert[i].param[1] = t;

    arc->makeSide( new(pwlarcpool) PwlArc( nsteps+1, newvert ), arc_top );
}

/*-----------------------------------------------------------------------------
 * pwl_bottom - construct a bottom boundary pwl arc and attach it to an arc
 *-----------------------------------------------------------------------------
 */

void
ArcTessellator::pwl_bottom( Arc *arc, REAL t, REAL s1, REAL s2, REAL rate )
{
    assert( s1 < s2 );

/*    if(rate <= 0.06) rate = 0.06;*/

/*    int nsteps = 1 + (int) ((s2 - s1) / rate ); */
    int nsteps = steps_function(s2,s1,rate);
    REAL stepsize = (s2 - s1) / (REAL) nsteps;

    TrimVertex *newvert = trimvertexpool.get( nsteps+1 );
    int i;
    for( i = 0; i < nsteps; i++ ) {
	newvert[i].param[0] = s1;
	newvert[i].param[1] = t;
	s1 += stepsize;
    }
    newvert[i].param[0] = s2;
    newvert[i].param[1] = t;

    arc->makeSide( new(pwlarcpool) PwlArc( nsteps+1, newvert ), arc_bottom );
}

/*-----------------------------------------------------------------------------
 * pwl - construct a pwl arc and attach it to an arc
 *-----------------------------------------------------------------------------
 */

void
ArcTessellator::pwl( Arc *arc, REAL s1, REAL s2, REAL t1, REAL t2, REAL rate )
{

/*    if(rate <= 0.06) rate = 0.06;*/

    int snsteps = 1 + (int) (glu_abs(s2 - s1) / rate );
    int tnsteps = 1 + (int) (glu_abs(t2 - t1) / rate );
    int nsteps = max(1,max( snsteps, tnsteps ));

    REAL sstepsize = (s2 - s1) / (REAL) nsteps;
    REAL tstepsize = (t2 - t1) / (REAL) nsteps;
    TrimVertex *newvert = trimvertexpool.get( nsteps+1 );
    long i;
    for( i = 0; i < nsteps; i++ ) {
	newvert[i].param[0] = s1;
	newvert[i].param[1] = t1;
	s1 += sstepsize;
	t1 += tstepsize;
    }
    newvert[i].param[0] = s2;
    newvert[i].param[1] = t2;

    /* arc->makeSide( new(pwlarcpool) PwlArc( nsteps+1, newvert ), arc_bottom ); */
    arc->pwlArc = new(pwlarcpool) PwlArc( nsteps+1, newvert );

    arc->clearbezier();
    arc->clearside( );
}


/*-----------------------------------------------------------------------------
 * tessellateLinear - constuct a linear pwl arc and attach it to an Arc
 *-----------------------------------------------------------------------------
 */

void
ArcTessellator::tessellateLinear( Arc *arc, REAL geo_stepsize, REAL arc_stepsize, int isrational )
{
    assert( arc->pwlArc == NULL );
    REAL s1, s2, t1, t2;

    //we don't need to scale by arc_stepsize if the trim curve
    //is piecewise linear. Reason: In pwl_right, pwl_left, pwl_top, pwl_left,
    //and pwl, the nsteps is computed by deltaU (or V) /stepsize. 
    //The quantity deltaU/arc_stepsize doesn't have any meaning. And
    //it causes problems: see bug 517641
    REAL stepsize = geo_stepsize; /* * arc_stepsize*/;

    BezierArc *b = arc->bezierArc;

    if( isrational ) {
	s1 = b->cpts[0] / b->cpts[2];
	t1 = b->cpts[1] / b->cpts[2];
	s2 = b->cpts[b->stride+0] / b->cpts[b->stride+2];
	t2 = b->cpts[b->stride+1] / b->cpts[b->stride+2];
    } else {
	s1 = b->cpts[0];
	t1 = b->cpts[1];
	s2 = b->cpts[b->stride+0];
	t2 = b->cpts[b->stride+1];
    }
    if( s1 == s2 )
	if( t1 < t2 )
	    pwl_right( arc, s1, t1, t2, stepsize );
	else
	    pwl_left( arc, s1, t1, t2, stepsize );
    else if( t1 == t2 )
	if( s1 < s2 ) 
	    pwl_bottom( arc, t1, s1, s2, stepsize );
	else
	    pwl_top( arc, t1, s1, s2, stepsize );
    else
	pwl( arc, s1, s2, t1, t2, stepsize );
}

/*-----------------------------------------------------------------------------
 * tessellateNonlinear - constuct a nonlinear pwl arc and attach it to an Arc
 *-----------------------------------------------------------------------------
 */

void
ArcTessellator::tessellateNonlinear( Arc *arc, REAL geo_stepsize, REAL arc_stepsize, int isrational )
{
    assert( arc->pwlArc == NULL );

    REAL stepsize	= geo_stepsize * arc_stepsize;

    BezierArc *bezierArc = arc->bezierArc;

    REAL size; //bounding box size of the curve in UV 
    {
      int i,j;
      REAL min_u, min_v, max_u,max_v;
      min_u = max_u = bezierArc->cpts[0];
      min_v = max_v = bezierArc->cpts[1];
      for(i=1, j=bezierArc->stride; i<bezierArc->order; i++, j+= bezierArc->stride)
	{
	  if(bezierArc->cpts[j] < min_u)
	    min_u = bezierArc->cpts[j];
	  if(bezierArc->cpts[j] > max_u)
	    max_u = bezierArc->cpts[j];
	  if(bezierArc->cpts[j+1] < min_v)
	    min_v = bezierArc->cpts[j+1];	  
	  if(bezierArc->cpts[j+1] > max_v)
	    max_v = bezierArc->cpts[j+1]; 
	}

      size = max_u - min_u;
      if(size < max_v - min_v)
	size = max_v - min_v;
    }
      
    /*int	nsteps 		= 1 + (int) (1.0/stepsize);*/

    int nsteps = (int) (size/stepsize);
    if(nsteps <=0)
      nsteps=1;

    TrimVertex *vert	= trimvertexpool.get( nsteps+1 );
    REAL dp 		= 1.0/nsteps;


    arc->pwlArc 	= new(pwlarcpool) PwlArc();
    arc->pwlArc->pts 	= vert;

    if( isrational ) {
        REAL pow_u[MAXORDER], pow_v[MAXORDER], pow_w[MAXORDER];
    	trim_power_coeffs( bezierArc, pow_u, 0 );
    	trim_power_coeffs( bezierArc, pow_v, 1 );
        trim_power_coeffs( bezierArc, pow_w, 2 );

	/* compute first point exactly */
        REAL *b = bezierArc->cpts;
	vert->param[0] = b[0]/b[2];
	vert->param[1] = b[1]/b[2];

	/* strength reduction on p = dp * step would introduce error */
	int step;
#ifndef NOELIMINATION
	int ocanremove = 0;
#endif
    	register long order =  bezierArc->order;
	for( step=1, ++vert; step<nsteps; step++, vert++ ) {
	    register REAL p = dp * step;
    	    register REAL u = pow_u[0];
            register REAL v = pow_v[0];
	    register REAL w = pow_w[0];
	    for( register int i = 1; i < order; i++ ) {
	        u = u * p + pow_u[i];
	        v = v * p + pow_v[i];
	        w = w * p + pow_w[i];
            }
            vert->param[0] = u/w;
    	    vert->param[1] = v/w;
#ifndef NOELIMINATION
	    REAL ds = glu_abs(vert[0].param[0] - vert[-1].param[0]);
	    REAL dt = glu_abs(vert[0].param[1] - vert[-1].param[1]);
	    int canremove = (ds<geo_stepsize && dt<geo_stepsize) ? 1 : 0;
	    REAL ods=0.0, odt=0.0;

	    if( ocanremove && canremove ) {
		REAL nds = ds + ods;
		REAL ndt = dt + odt;
		if( nds<geo_stepsize && ndt<geo_stepsize ) {
		    // remove previous point
		    --vert;
		    vert[0].param[0] = vert[1].param[0];
		    vert[0].param[1] = vert[1].param[1];
		    ods = nds;
		    odt = ndt;
		    ocanremove = 1;
		} else {
		    ocanremove = canremove;
		    ods = ds;
		    odt = dt;
		}
	    } else {
		ocanremove = canremove;
		ods = ds;
		odt = dt;
	    }
#endif	
	}

	/* compute last point exactly */
	b += (order - 1) * bezierArc->stride;
	vert->param[0] = b[0]/b[2];
	vert->param[1] = b[1]/b[2];

    } else {
        REAL pow_u[MAXORDER], pow_v[MAXORDER];
	trim_power_coeffs( bezierArc, pow_u, 0 );
	trim_power_coeffs( bezierArc, pow_v, 1 );

	/* compute first point exactly */
        REAL *b = bezierArc->cpts;
	vert->param[0] = b[0];
	vert->param[1] = b[1];

	/* strength reduction on p = dp * step would introduce error */
	int step;
#ifndef NOELIMINATION
	int ocanremove = 0;
#endif
    	register long order =  bezierArc->order;
	for( step=1, ++vert; step<nsteps; step++, vert++ ) {
	    register REAL p = dp * step;
	    register REAL u = pow_u[0];
            register REAL v = pow_v[0];
            for( register int i = 1; i < bezierArc->order; i++ ) {
	        u = u * p + pow_u[i];
	        v = v * p + pow_v[i];
            }
            vert->param[0] = u;
	    vert->param[1] = v;
#ifndef NOELIMINATION
	    REAL ds = glu_abs(vert[0].param[0] - vert[-1].param[0]);
	    REAL dt = glu_abs(vert[0].param[1] - vert[-1].param[1]);
	    int canremove = (ds<geo_stepsize && dt<geo_stepsize) ? 1 : 0;
	    REAL ods=0.0, odt=0.0;

	    if( ocanremove && canremove ) {
		REAL nds = ds + ods;
		REAL ndt = dt + odt;
		if( nds<geo_stepsize && ndt<geo_stepsize ) {
		    // remove previous point
		    --vert;
		    vert[0].param[0] = vert[1].param[0];
		    vert[0].param[1] = vert[1].param[1];
		    ods = nds;
		    odt = ndt;
		    ocanremove = 1;
		} else {
		    ocanremove = canremove;
		    ods = ds;
		    odt = dt;
		}
	    } else {
		ocanremove = canremove;
		ods = ds;
		odt = dt;
	    }
#endif	
	}

	/* compute last point exactly */
	b += (order - 1) * bezierArc->stride;
	vert->param[0] = b[0];
	vert->param[1] = b[1];
    }
    arc->pwlArc->npts = vert - arc->pwlArc->pts + 1;
/*
    for( TrimVertex *vt=pwlArc->pts; vt != vert-1; vt++ ) {
	if( tooclose( vt[0].param[0], vt[1].param[0] ) )
	    vt[1].param[0] = vt[0].param[0];
	if( tooclose( vt[0].param[1], vt[1].param[1] ) )
	    vt[1].param[1] = vt[0].param[1];
    }
*/
}

const REAL ArcTessellator::gl_Bernstein[][MAXORDER][MAXORDER] = {
 {
  {1, 0, 0, 0, 0, 0, 0, 0 },
  {0, 0, 0, 0, 0, 0, 0, 0 },
  {0, 0, 0, 0, 0, 0, 0, 0 },
  {0, 0, 0, 0, 0, 0, 0, 0 },
  {0, 0, 0, 0, 0, 0, 0, 0 },
  {0, 0, 0, 0, 0, 0, 0, 0 },
  {0, 0, 0, 0, 0, 0, 0, 0 },
  {0, 0, 0, 0, 0, 0, 0, 0 }
 },
 {
  {-1, 1, 0, 0, 0, 0, 0, 0 },
  {1, 0, 0, 0, 0, 0, 0, 0 },
  {0, 0, 0, 0, 0, 0, 0, 0 },
  {0, 0, 0, 0, 0, 0, 0, 0 },
  {0, 0, 0, 0, 0, 0, 0, 0 },
  {0, 0, 0, 0, 0, 0, 0, 0 },
  {0, 0, 0, 0, 0, 0, 0, 0 },
  {0, 0, 0, 0, 0, 0, 0, 0 }
 },
 {
  {1, -2, 1, 0, 0, 0, 0, 0 },
  {-2, 2, 0, 0, 0, 0, 0, 0 },
  {1, 0, 0, 0, 0, 0, 0, 0 },
  {0, 0, 0, 0, 0, 0, 0, 0 },
  {0, 0, 0, 0, 0, 0, 0, 0 },
  {0, 0, 0, 0, 0, 0, 0, 0 },
  {0, 0, 0, 0, 0, 0, 0, 0 },
  {0, 0, 0, 0, 0, 0, 0, 0 }
 },
 {
  {-1, 3, -3, 1, 0, 0, 0, 0 },
  {3, -6, 3, 0, 0, 0, 0, 0 },
  {-3, 3, 0, 0, 0, 0, 0, 0 },
  {1, 0, 0, 0, 0, 0, 0, 0 },
  {0, 0, 0, 0, 0, 0, 0, 0 },
  {0, 0, 0, 0, 0, 0, 0, 0 },
  {0, 0, 0, 0, 0, 0, 0, 0 },
  {0, 0, 0, 0, 0, 0, 0, 0 }
 },
 {
  {1, -4, 6, -4, 1, 0, 0, 0 },
  {-4, 12, -12, 4, 0, 0, 0, 0 },
  {6, -12, 6, 0, 0, 0, 0, 0 },
  {-4, 4, 0, 0, 0, 0, 0, 0 },
  {1, 0, 0, 0, 0, 0, 0, 0 },
  {0, 0, 0, 0, 0, 0, 0, 0 },
  {0, 0, 0, 0, 0, 0, 0, 0 },
  {0, 0, 0, 0, 0, 0, 0, 0 }
 },
 {
  {-1, 5, -10, 10, -5, 1, 0, 0 },
  {5, -20, 30, -20, 5, 0, 0, 0 },
  {-10, 30, -30, 10, 0, 0, 0, 0 },
  {10, -20, 10, 0, 0, 0, 0, 0 },
  {-5, 5, 0, 0, 0, 0, 0, 0 },
  {1, 0, 0, 0, 0, 0, 0, 0 },
  {0, 0, 0, 0, 0, 0, 0, 0 },
  {0, 0, 0, 0, 0, 0, 0, 0 }
 },
 {
  {1, -6, 15, -20, 15, -6, 1, 0 },
  {-6, 30, -60, 60, -30, 6, 0, 0 },
  {15, -60, 90, -60, 15, 0, 0, 0 },
  {-20, 60, -60, 20, 0, 0, 0, 0 },
  {15, -30, 15, 0, 0, 0, 0, 0 },
  {-6, 6, 0, 0, 0, 0, 0, 0 },
  {1, 0, 0, 0, 0, 0, 0, 0 },
  {0, 0, 0, 0, 0, 0, 0, 0 }
 },
 {
  {-1, 7, -21, 35, -35, 21, -7, 1 },
  {7, -42, 105, -140, 105, -42, 7, 0 },
  {-21, 105, -210, 210, -105, 21, 0, 0 },
  {35, -140, 210, -140, 35, 0, 0, 0 },
  {-35, 105, -105, 35, 0, 0, 0, 0 },
  {21, -42, 21, 0, 0, 0, 0, 0 },
  {-7, 7, 0, 0, 0, 0, 0, 0 },
  {1, 0, 0, 0, 0, 0, 0, 0 }
 }};


/*-----------------------------------------------------------------------------
 * trim_power_coeffs - compute power basis coefficients from bezier coeffients
 *-----------------------------------------------------------------------------
 */
void
ArcTessellator::trim_power_coeffs( BezierArc *bez_arc, REAL *p, int coord )
{
    register int stride = bez_arc->stride;
    register int order = bez_arc->order;
    register REAL *base = bez_arc->cpts + coord;

    REAL const (*mat)[MAXORDER][MAXORDER] = &gl_Bernstein[order-1];
    REAL const (*lrow)[MAXORDER] = &(*mat)[order];

    /* WIN32 didn't like the following line within the for-loop */
    REAL const (*row)[MAXORDER] =  &(*mat)[0];
    for( ; row != lrow; row++ ) {
	register REAL s = 0.0;
	register REAL *point = base;
	register REAL const *mlast = *row + order;
	for( REAL const *m = *row; m != mlast; m++, point += stride ) 
	    s += *(m) * (*point);
	*(p++) = s;
    }
}
