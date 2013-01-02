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
 * backend.h
 *
 */

#ifndef __glubackend_h_
#define __glubackend_h_

#include "trimvertex.h"
#include "gridvertex.h"
#include "gridtrimvertex.h"

class BasicCurveEvaluator;
class BasicSurfaceEvaluator;

class Backend {
private:
    BasicCurveEvaluator&	curveEvaluator;
    BasicSurfaceEvaluator&	surfaceEvaluator;
public:
  			Backend( BasicCurveEvaluator &c, BasicSurfaceEvaluator& e )
			: curveEvaluator(c), surfaceEvaluator(e) {}

    /* surface backend routines */
    void		bgnsurf( int, int, long  );
    void		patch( REAL, REAL, REAL, REAL );
    void		surfpts( long, REAL *, long, long, int, int,
          			 REAL, REAL, REAL, REAL );
    void		surfbbox( long, REAL *, REAL * );
    void		surfgrid( REAL, REAL, long, REAL, REAL, long ); 
    void		surfmesh( long, long, long, long ); 
    void		bgntmesh( const char * );
    void		endtmesh( void );
    void		swaptmesh( void );
    void		tmeshvert( GridTrimVertex * );
    void		tmeshvert( TrimVertex * );
    void		tmeshvert( GridVertex * );
    void		tmeshvert( REAL u,  REAL v );
    void		linevert( TrimVertex * );
    void		linevert( GridVertex * );
    void		bgnoutline( void );
    void		endoutline( void );
    void		endsurf( void );
    void		triangle( TrimVertex*, TrimVertex*, TrimVertex* );

    void                bgntfan();
    void                endtfan();
    void                bgnqstrip();
    void                endqstrip();
    void                evalUStrip(int n_upper, REAL v_upper, REAL* upper_val, 
				   int n_lower, REAL v_lower, REAL* lower_val
				   );
    void                evalVStrip(int n_left, REAL u_left, REAL* left_val, 
				   int n_right, REAL v_right, REAL* right_val
				   );
    void                tmeshvertNOGE(TrimVertex *t);
    void                tmeshvertNOGE_BU(TrimVertex *t);
    void                tmeshvertNOGE_BV(TrimVertex *t);
    void                preEvaluateBU(REAL u);
    void                preEvaluateBV(REAL v);
	

    /* curve backend routines */
    void		bgncurv( void );
    void		segment( REAL, REAL );
    void		curvpts( long, REAL *, long, int, REAL, REAL );
    void		curvgrid( REAL, REAL, long );
    void		curvmesh( long, long );
    void		curvpt( REAL  );  
    void		bgnline( void );
    void		endline( void );
    void		endcurv( void );
private:
#ifndef NOWIREFRAME
    int			wireframetris;
    int			wireframequads;
    int			npts;
    REAL		mesh[3][4];
    int			meshindex;
#endif
};

#endif /* __glubackend_h_ */
