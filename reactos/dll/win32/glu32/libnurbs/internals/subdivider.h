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
 * subdivider.h
 *
 * $Date$ $Revision: 1.1 $
 * $Header: /cygdrive/c/RCVS/CVS/ReactOS/reactos/lib/glu32/libnurbs/internals/subdivider.h,v 1.1 2004/02/02 16:39:12 navaraf Exp $
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
