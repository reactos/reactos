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
 * arc.h
 *
 * $Date$ $Revision: 1.1 $
 * $Header: /cygdrive/c/RCVS/CVS/ReactOS/reactos/lib/glu32/libnurbs/internals/arc.h,v 1.1 2004/02/02 16:39:10 navaraf Exp $
 */

#ifndef __gluarc_h_
#define __gluarc_h_

#include "myassert.h"
#include "bufpool.h"
#include "mystdio.h"
#include "types.h"
#include "pwlarc.h"
#include "trimvertex.h"

class Bin;
class Arc;
struct BezierArc;

typedef class Arc *Arc_ptr;

enum arc_side { arc_none = 0, arc_right, arc_top, arc_left, arc_bottom };


class Arc: public PooledObj { /* an arc, in two list, the trim list and bin */

public:
    static const int bezier_tag;
    static const int arc_tag;
    static const int tail_tag;
    Arc_ptr		prev;		/* trim list pointer */
    Arc_ptr		next;		/* trim list pointer */
    Arc_ptr		link;		/* bin pointers */
    BezierArc *		bezierArc;	/* associated bezier arc */
    PwlArc *		pwlArc;	/* associated pwl arc */
    long		type;		/* curve type */
    long		nuid;

    inline		Arc( Arc *, PwlArc * );
    inline		Arc( arc_side, long );

    Arc_ptr		append( Arc_ptr );
    int			check( void );
    int			isMonotone( void );
    int			isDisconnected( void );
    int			numpts( void );
    void		markverts( void );
    void		getextrema( Arc_ptr[4] );
    void		print( void );
    void		show( void );
    void		makeSide( PwlArc *, arc_side );
    inline int		isTessellated() { return pwlArc ? 1 : 0; }
    inline long 	isbezier() 	{ return type & bezier_tag; }
    inline void 	setbezier() 	{ type |= bezier_tag; }
    inline void 	clearbezier() 	{ type &= ~bezier_tag; }
    inline long		npts() 		{ return pwlArc->npts; }
    inline TrimVertex *	pts() 		{ return pwlArc->pts; }
    inline REAL * 	tail() 		{ return pwlArc->pts[0].param; }
    inline REAL * 	head() 		{ return next->pwlArc->pts[0].param; }
    inline REAL *	rhead() 	{ return pwlArc->pts[pwlArc->npts-1].param; }
    inline long		ismarked()	{ return type & arc_tag; }
    inline void		setmark()	{ type |= arc_tag; }
    inline void		clearmark()	{ type &= (~arc_tag); }
    inline void		clearside() 	{ type &= ~(0x7 << 8); }
    inline void		setside( arc_side s ) { clearside(); type |= (((long)s)<<8); }
    inline arc_side	getside() 	{ return (arc_side) ((type>>8) & 0x7); }
    inline int		getitail()	{ return type & tail_tag; }
    inline void		setitail()	{ type |= tail_tag; }
    inline void		clearitail()	{ type &= (~tail_tag); }
};

/*--------------------------------------------------------------------------
 * Arc - initialize a new Arc with the same type and uid of
 *	    a given Arc and a given pwl arc
 *--------------------------------------------------------------------------
 */

inline
Arc::Arc( Arc *j, PwlArc *p )
{
    bezierArc = NULL;
    pwlArc = p;
    type = j->type;
    nuid = j->nuid;
}

/*--------------------------------------------------------------------------
 * Arc - initialize a new Arc with the same type and uid of
 *	    a given Arc and a given pwl arc
 *--------------------------------------------------------------------------
 */

inline
Arc::Arc( arc_side side, long _nuid )
{
    bezierArc = NULL;
    pwlArc = NULL;
    type = 0;
    setside( side );
    nuid = _nuid;
}

#endif /* __gluarc_h_ */
