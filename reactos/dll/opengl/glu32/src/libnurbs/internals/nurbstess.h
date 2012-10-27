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
    int			do_check_knots( Knotvector *, const char * );
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
