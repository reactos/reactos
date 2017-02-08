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
 * basicsurfeval.h
 *
 */

#ifndef __glubasicsurfeval_h_
#define __glubasicsurfeval_h_

#include "types.h"
#include "displaymode.h"
#include "cachingeval.h"

class BasicSurfaceEvaluator : public CachingEvaluator {
public:
    virtual             ~BasicSurfaceEvaluator() { /* silence warning*/ }
    virtual void	range2f( long, REAL *, REAL * );
    virtual void	domain2f( REAL, REAL, REAL, REAL );

    virtual void	enable( long );
    virtual void	disable( long );
    virtual void	bgnmap2f( long );
    virtual void	map2f( long, REAL, REAL, long, long, 
				     REAL, REAL, long, long, 
				     REAL *  );
    virtual void	mapgrid2f( long, REAL, REAL, long,  REAL, REAL );
    virtual void	mapmesh2f( long, long, long, long, long );
    virtual void	evalcoord2f( long, REAL, REAL );
    virtual void	evalpoint2i( long, long );
    virtual void	endmap2f( void );

    virtual void	polymode( long );
    virtual void 	bgnline( void );
    virtual void 	endline( void );
    virtual void 	bgnclosedline( void );
    virtual void 	endclosedline( void );
    virtual void 	bgntmesh( void );
    virtual void 	swaptmesh( void );
    virtual void 	endtmesh( void );
    virtual void 	bgnqstrip( void );
    virtual void 	endqstrip( void );

    virtual void 	bgntfan( void );
    virtual void 	endtfan( void );

    virtual void        evalUStrip(int n_upper, REAL v_upper, REAL* upper_val,
				     int n_lower, REAL v_lower, REAL* lower_val
      ) = 0;

    virtual void        evalVStrip(int n_left, REAL u_left, REAL* left_val,
				     int n_right, REAL u_right, REAL* right_val
      ) = 0;
    virtual void        inDoEvalCoord2NOGE(REAL u, REAL v, REAL* ret_point, REAL* ret_normal) = 0;
    virtual void        inDoEvalCoord2NOGE_BU(REAL u, REAL v, REAL* ret_point, REAL* ret_normal) = 0;
    virtual void        inDoEvalCoord2NOGE_BV(REAL u, REAL v, REAL* ret_point, REAL* ret_normal) = 0;
    virtual void inPreEvaluateBV_intfac(REAL v ) = 0;
    virtual void inPreEvaluateBU_intfac(REAL u ) = 0;
    
};

#endif /* __glubasicsurfeval_h_ */
