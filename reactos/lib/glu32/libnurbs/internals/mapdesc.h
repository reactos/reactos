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
 * mapdesc.h
 *
 * $Date: 2004/02/02 16:39:11 $ $Revision: 1.1 $
 * $Header: /cygdrive/c/RCVS/CVS/ReactOS/reactos/lib/glu32/libnurbs/internals/mapdesc.h,v 1.1 2004/02/02 16:39:11 navaraf Exp $
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
