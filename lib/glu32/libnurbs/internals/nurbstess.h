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
 * nurbstess.h
 *
 */

#ifndef __glunurbstess_h_
#define __glunurbstess_h_

#include "mysetjmp.h"
#include "subdivider.h"
#include "renderhints.h"
#include "backend.h"
#include "maplist.h"
#include "reader.h"
#include "nurbsconsts.h"

struct Knotvector;
class Quilt;
class DisplayList;
class BasicCurveEvaluator;
class BasicSurfaceEvaluator;

class NurbsTessellator {
public:
    			NurbsTessellator( BasicCurveEvaluator &c,
                                          BasicSurfaceEvaluator &e );
    			virtual ~NurbsTessellator( void );

    void     		getnurbsproperty( long, INREAL * );
    void     		getnurbsproperty( long, long, INREAL * );
    void     		setnurbsproperty( long, INREAL );
    void     		setnurbsproperty( long, long, INREAL );
    void		setnurbsproperty( long, long, INREAL * );
    void		setnurbsproperty( long, long, INREAL *, long, long );

    // called before a tessellation begins/ends
    virtual void	bgnrender( void );
    virtual void	endrender( void );

    // called to make a display list of the output vertices
    virtual void	makeobj( int n );
    virtual void	closeobj( void );

    // called when a error occurs
    virtual void	errorHandler( int );

    void     		bgnsurface( long );
    void     		endsurface( void );
    void     		bgntrim( void );
    void     		endtrim( void );
    void     		bgncurve( long );
    void     		endcurve( void );
    void     		pwlcurve( long, INREAL[], long, long );
    void     		nurbscurve( long, INREAL[], long, INREAL[], long, long );
    void     		nurbssurface( long, INREAL[], long, INREAL[], long, long,
			    INREAL[], long, long, long );

    void 		defineMap( long, long, long );
    void		redefineMaps( void );

    // recording of input description
    void 		discardRecording( void * );
    void * 		beginRecording( void );
    void 		endRecording( void );
    void 		playRecording( void * );

    //for optimizing untrimmed nurbs in the case of domain distance sampling
    void set_domain_distance_u_rate(REAL u_rate);
    void set_domain_distance_v_rate(REAL v_rate);
    void set_is_domain_distance_sampling(int flag);
    

protected:
    Renderhints		renderhints;
    Maplist		maplist;
    Backend		backend;

private:

    void		resetObjects( void );
    int			do_check_knots( Knotvector *, char * );
    void		do_nurbserror( int );
    void		do_bgncurve( O_curve * );
    void		do_endcurve( void );
    void		do_freeall( void );
    void		do_freecurveall( O_curve * );
    void		do_freebgntrim( O_trim * );
    void		do_freebgncurve( O_curve * );
    void		do_freepwlcurve( O_pwlcurve * );
    void		do_freenurbscurve( O_nurbscurve * );
    void		do_freenurbssurface( O_nurbssurface * );
    void 		do_freebgnsurface( O_surface * );
    void		do_bgnsurface( O_surface * );
    void		do_endsurface( void );
    void		do_bgntrim( O_trim * );
    void		do_endtrim( void );
    void		do_pwlcurve( O_pwlcurve * );
    void		do_nurbscurve( O_nurbscurve * );
    void		do_nurbssurface( O_nurbssurface * );
    void		do_freenurbsproperty( Property * );
    void		do_setnurbsproperty( Property * );
    void		do_setnurbsproperty2( Property * );

    Subdivider		subdivider;
    JumpBuffer* 	jumpbuffer;
    Pool		o_pwlcurvePool;
    Pool		o_nurbscurvePool;
    Pool		o_curvePool;
    Pool		o_trimPool;
    Pool		o_surfacePool;
    Pool		o_nurbssurfacePool;
    Pool		propertyPool;
public:
    Pool		quiltPool;
private:
    TrimVertexPool	extTrimVertexPool;

    int			inSurface;		/* bgnsurface seen */
    int			inCurve;		/* bgncurve seen */
    int			inTrim;			/* bgntrim seen */
    int			isCurveModified;	/* curve changed */
    int			isTrimModified;		/* trim curves changed */
    int			isSurfaceModified;	/* surface changed */
    int			isDataValid;		/* all data is good */
    int			numTrims;		/* valid trim regions */
    int			playBack;

    O_trim**		nextTrim;		/* place to link o_trim */
    O_curve**		nextCurve;		/* place to link o_curve */
    O_nurbscurve**	nextNurbscurve;		/* place to link o_nurbscurve */
    O_pwlcurve**	nextPwlcurve;		/* place to link o_pwlcurve */
    O_nurbssurface**	nextNurbssurface;	/* place to link o_nurbssurface */

    O_surface*		currentSurface;
    O_trim*		currentTrim;
    O_curve*		currentCurve;

    DisplayList		*dl;

};

#endif /* __glunurbstess_h_ */
