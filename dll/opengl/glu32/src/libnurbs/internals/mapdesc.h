/*
 * SGI FREE SOFTWARE LICENSE B (Version 2.0, Sept. 18, 2008)
 * Copyright (C) 1991-2000 Silicon Graphics, Inc. All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice including the dates of first publication and
 * either this permission notice or a reference to
 * http://oss.sgi.com/projects/FreeB/
 * shall be included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * SILICON GRAPHICS, INC. BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * Except as contained in this notice, the name of Silicon Graphics, Inc.
 * shall not be used in advertising or otherwise to promote the sale, use or
 * other dealings in this Software without prior written authorization from
 * Silicon Graphics, Inc.
 */

/*
 * mapdesc.h
 *
 */

#ifndef __glumapdesc_h_
#define __glumapdesc_h_

#include "mystdio.h"
#include "types.h"
#include "defines.h"
#include "bufpool.h"
#include "nurbsconsts.h"

typedef REAL Maxmatrix[MAXCOORDS][MAXCOORDS];

class Backend;

class Mapdesc : public PooledObj {
    friend class Maplist;
			
public:
    			Mapdesc( long, int, int, Backend & );
    int			isProperty( long );
    REAL		getProperty( long );
    void		setProperty( long, REAL );
    int			isConstantSampling( void );
    int			isDomainSampling( void );
    int			isRangeSampling( void );
    int			isSampling( void );
    int			isParametricDistanceSampling( void );
    int			isObjectSpaceParaSampling( void );
    int			isObjectSpacePathSampling( void );
    int			isSurfaceAreaSampling( void );
    int			isPathLengthSampling( void );
    int			isCulling( void );
    int			isBboxSubdividing( void );
    long		getType( void );

    /* curve routines */
    void		subdivide( REAL *, REAL *, REAL, int, int );
    int 		cullCheck( REAL *, int, int );
    void		xformBounding( REAL *, int, int, REAL *, int );
    void		xformCulling( REAL *, int, int, REAL *, int );
    void		xformSampling( REAL *, int, int, REAL *, int );
    void		xformMat( Maxmatrix, REAL *, int, int, REAL *, int );
    REAL		calcPartialVelocity ( REAL *, int, int, int, REAL );
    int			project( REAL *, int, REAL *, int, int );
    REAL		calcVelocityRational( REAL *, int, int );
    REAL		calcVelocityNonrational( REAL *, int, int );

    /* surface routines */
    void		subdivide( REAL *, REAL *, REAL, int, int, int, int );
    int 		cullCheck( REAL *, int, int, int, int );
    void		xformBounding( REAL *, int, int, int, int, REAL *, int, int );
    void		xformCulling( REAL *, int, int, int, int, REAL *, int, int );
    void		xformSampling( REAL *, int, int, int, int, REAL *, int, int );
    void		xformMat( Maxmatrix, REAL *, int, int, int, int, REAL *, int, int );
    REAL		calcPartialVelocity ( REAL *, REAL *, int, int, int, int, int, int, REAL, REAL, int );
    int 		project( REAL *, int, int, REAL *, int, int, int, int);
    void		surfbbox( REAL bb[2][MAXCOORDS] );

    int			bboxTooBig( REAL *, int, int, int, int, REAL [2][MAXCOORDS] );
    int 		xformAndCullCheck( REAL *, int, int, int, int );

    void		identify( REAL[MAXCOORDS][MAXCOORDS] );
    void		setBboxsize( INREAL *);
    inline void 	setBmat( INREAL*, long, long );
    inline void 	setCmat( INREAL*, long, long );
    inline void 	setSmat( INREAL*, long, long );
    inline int		isRational( void );
    inline int		getNcoords( void );

    REAL 		pixel_tolerance;    /* pathlength sampling tolerance */
    REAL		error_tolerance;    /* parametric error sampling tolerance*/
    REAL		object_space_error_tolerance;    /* object space tess*/
    REAL 		clampfactor;
    REAL 		minsavings;
    REAL		maxrate;
    REAL		maxsrate;
    REAL		maxtrate;
    REAL		bboxsize[MAXCOORDS];

private:
    long 		type;
    int 		isrational;
    int 		ncoords;
    int 		hcoords;
    int 		inhcoords;
    int			mask;
    Maxmatrix 		bmat;
    Maxmatrix 		cmat;
    Maxmatrix 		smat;
    REAL 		s_steps;		/* max samples in s direction */
    REAL 		t_steps;		/* max samples in t direction */
    REAL 		sampling_method;	
    REAL 		culling_method;		/* check for culling */
    REAL		bbox_subdividing;
    Mapdesc *		next;
    Backend &		backend;

    void		bbox( REAL [2][MAXCOORDS], REAL *, int, int, int, int );
    REAL		maxDifference( int, REAL *, int );
    static void 	copy( Maxmatrix, long, INREAL *, long, long );

    /* individual control point routines */
    static void		transform4d( float[4], float[4], float[4][4] );
    static void		multmatrix4d ( float[4][4], const float[4][4],
					 const float[4][4] );
    void		copyPt( REAL *, REAL * );
    void		sumPt( REAL *, REAL *, REAL *, REAL, REAL );
    void		xformSampling( REAL *, REAL * );
    void		xformCulling( REAL *, REAL * );
    void		xformRational( Maxmatrix, REAL *, REAL * );
    void		xformNonrational( Maxmatrix, REAL *, REAL * );
    unsigned int	clipbits( REAL * );
};

inline void
Mapdesc::setBmat( INREAL *mat, long rstride, long cstride )
{
    copy( bmat, hcoords, mat, rstride, cstride );
}

inline void
Mapdesc::setCmat( INREAL *mat, long rstride, long cstride )
{
    copy( cmat, hcoords, mat, rstride, cstride );
}

inline void
Mapdesc::setSmat( INREAL *mat, long rstride, long cstride )
{
    copy( smat, hcoords, mat, rstride, cstride );
}

inline long
Mapdesc::getType( void )
{
    return type;
}

inline void
Mapdesc::xformCulling( REAL *d, REAL *s )
{
    if( isrational )
        xformRational( cmat, d, s );
    else
	xformNonrational( cmat, d, s );
}

inline void
Mapdesc::xformSampling( REAL *d, REAL *s )
{
    if( isrational )
        xformRational( smat, d, s );
    else
	xformNonrational( smat, d, s );
}

inline int 
Mapdesc::isRational( void )
{
    return isrational ? 1 : 0;
}

inline int		
Mapdesc::getNcoords( void ) 
{
    return ncoords; 
}

inline int			
Mapdesc::isConstantSampling( void ) 
{
    return ((sampling_method == N_FIXEDRATE) ? 1 : 0); 
}

inline int			
Mapdesc::isDomainSampling( void ) 
{ 
    return ((sampling_method == N_DOMAINDISTANCE) ? 1 : 0); 
}

inline int			
Mapdesc::isParametricDistanceSampling( void ) 
{
    return ((sampling_method == N_PARAMETRICDISTANCE) ? 1 : 0);
}

inline int			
Mapdesc::isObjectSpaceParaSampling( void ) 
{
    return ((sampling_method == N_OBJECTSPACE_PARA) ? 1 : 0);
}

inline int			
Mapdesc::isObjectSpacePathSampling( void ) 
{
    return ((sampling_method == N_OBJECTSPACE_PATH) ? 1 : 0);
}

inline int			
Mapdesc::isSurfaceAreaSampling( void ) 
{
    return ((sampling_method == N_SURFACEAREA) ? 1 : 0);
}

inline int			
Mapdesc::isPathLengthSampling( void ) 
{
    return ((sampling_method == N_PATHLENGTH) ? 1 : 0);
}

inline int			
Mapdesc::isRangeSampling( void ) 
{
    return ( isParametricDistanceSampling() || isPathLengthSampling() ||
	    isSurfaceAreaSampling() ||
	    isObjectSpaceParaSampling() ||
	    isObjectSpacePathSampling());
}

inline int
Mapdesc::isSampling( void )
{
    return isRangeSampling() || isConstantSampling() || isDomainSampling();
}

inline int			
Mapdesc::isCulling( void ) 
{
    return ((culling_method != N_NOCULLING) ? 1 : 0);
}

inline int			
Mapdesc::isBboxSubdividing( void ) 
{
    return ((bbox_subdividing != N_NOBBOXSUBDIVISION) ? 1 : 0);
}
#endif /* __glumapdesc_h_ */
