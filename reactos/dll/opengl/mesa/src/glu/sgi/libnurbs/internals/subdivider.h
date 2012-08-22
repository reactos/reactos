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
 * subdivider.h
 *
 */

#ifndef __glusubdivider_h_
#define __glusubdivider_h_

#include "mysetjmp.h"
#include "bin.h"
#include "flist.h"
#include "slicer.h"
#include "arctess.h"
#include "trimvertex.h"
#include "trimvertpool.h"

class Arc;
class Pool;
class Renderhints;
class Quilt;
class Patchlist;
class Curvelist;
struct JumpBuffer;

class Subdivider {
public:
			Subdivider( Renderhints&, Backend& );
			~Subdivider( void );
    void		clear( void );

    void		beginTrims( void ) {}
    void		beginLoop( void );
    void		addArc( REAL *, Quilt *, long );
    void		addArc( int, TrimVertex *, long );
    void		endLoop( void ) {}
    void		endTrims( void ) {}

    void		beginQuilts( void );
    void		addQuilt( Quilt * );
    void		endQuilts( void ) {}

    void		drawCurves( void );
    void		drawSurfaces( long );

    int			ccwTurn_sl( Arc_ptr, Arc_ptr  );
    int			ccwTurn_sr( Arc_ptr , Arc_ptr   );
    int			ccwTurn_tl( Arc_ptr , Arc_ptr  );
    int			ccwTurn_tr( Arc_ptr , Arc_ptr  );

    void		setJumpbuffer( JumpBuffer * );

    void set_domain_distance_u_rate(REAL u_rate)
      {
	domain_distance_u_rate = u_rate;
      }
    void set_domain_distance_v_rate(REAL v_rate)
      {
	domain_distance_v_rate = v_rate;
      }
    void set_is_domain_distance_sampling(int flag)
      {
	is_domain_distance_sampling = flag;
      }

private:
    void		classify_headonleft_s( Bin &, Bin &, Bin &, REAL );
    void		classify_tailonleft_s( Bin &, Bin &, Bin &, REAL );
    void		classify_headonright_s( Bin &, Bin &, Bin &, REAL );
    void		classify_tailonright_s( Bin &, Bin &, Bin &, REAL );
    void		classify_headonleft_t( Bin &, Bin &, Bin &, REAL );
    void		classify_tailonleft_t( Bin &, Bin &, Bin &, REAL );
    void		classify_headonright_t( Bin &, Bin &, Bin &, REAL );
    void		classify_tailonright_t( Bin &, Bin &, Bin &, REAL );

    enum dir 		{ down, same, up, none };
    void		tessellate( Arc_ptr, REAL );
    void		monotonize( Arc_ptr , Bin & );
    int			isMonotone( Arc_ptr  );
    int			decompose( Bin &, REAL );


    Slicer		slicer;
    ArcTessellator	arctessellator;
    Pool		arcpool;
    Pool		bezierarcpool;
    Pool		pwlarcpool;
    TrimVertexPool	trimvertexpool;

    JumpBuffer*		jumpbuffer;
    Renderhints&	renderhints;
    Backend&		backend;

    Bin			initialbin;
    Arc_ptr		pjarc;
    int 		s_index;
    int			t_index;
    Quilt *		qlist;
    Flist		spbrkpts;
    Flist		tpbrkpts;
    Flist		smbrkpts;
    Flist		tmbrkpts;
    REAL	 	stepsizes[4];
    int			showDegenerate;
    int			isArcTypeBezier;

    void		samplingSplit( Curvelist&, int );

    void		subdivideInS( Bin&  );
    void		splitInS( Bin&, int, int );
    void		splitInT( Bin&, int, int );
    void		samplingSplit( Bin&, Patchlist&, int, int );
    void		nonSamplingSplit( Bin&, Patchlist&, int, int );
    void		tessellation( Bin&, Patchlist& );
    void		monosplitInS( Bin&, int, int );
    void		monosplitInT( Bin&, int, int );

    void		outline( Bin & );
    void		freejarcs( Bin & );
    void		render( Bin & );
    void		split( Bin &, Bin &, Bin &, int, REAL );
    void		tessellate( Bin &, REAL, REAL, REAL, REAL );

    inline void		setDegenerate( void ) { showDegenerate = 1; }
    inline void		setNonDegenerate( void ) { showDegenerate = 0; }
    inline int		showingDegenerate( void ) { return showDegenerate; }
    inline void		setArcTypeBezier( void ) { isArcTypeBezier = 1; }
    inline void		setArcTypePwl( void ) { isArcTypeBezier = 0; }
    inline int		isBezierArcType( void ) { return isArcTypeBezier; }

    void		makeBorderTrim( const REAL *, const REAL * );
    void		split( Bin &, int, const REAL *, int, int );
    void		partition( Bin &, Bin &, Bin &, Bin &, Bin &, int, REAL );
    void		findIrregularS( Bin & );
    void		findIrregularT( Bin & );


    inline int		bbox( TrimVertex *, TrimVertex *, TrimVertex *, int );
    static int		bbox( REAL, REAL, REAL, REAL, REAL, REAL );
    static int		ccw( TrimVertex *, TrimVertex *, TrimVertex * );
    void		join_s( Bin &, Bin &, Arc_ptr, Arc_ptr  );
    void		join_t( Bin &, Bin &, Arc_ptr , Arc_ptr  );
    int			arc_split( Arc_ptr , int, REAL, int );
    void		check_s( Arc_ptr , Arc_ptr  );
    void		check_t( Arc_ptr , Arc_ptr  );
    inline void		link( Arc_ptr , Arc_ptr , Arc_ptr , Arc_ptr  );
    inline void		simple_link( Arc_ptr , Arc_ptr  );

   Bin*                 makePatchBoundary( const REAL *from, const REAL *to );

   /*in domain distance method, the tessellation is controled by two numbers:
    *GLU_U_STEP: number of u-segments per unit u length of domain
    *GLU_V_STEP: number of v-segments per unit v length of domain
    *These two numbers are normally stored in mapdesc->maxs(t)rate.
    *I (ZL) put these two numbers here so that I can optimize the untrimmed 
    *case in the case of domain distance sampling.
    *These two numbers are set by set_domain_distance_u_rate() and ..._v_..().
    */
   REAL domain_distance_u_rate;
   REAL domain_distance_v_rate;
   int is_domain_distance_sampling;
};

inline void
Subdivider::beginLoop( void ) 
{
    pjarc = 0;
}


#endif /* __glusubdivider_h_ */
