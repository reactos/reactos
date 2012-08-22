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
 * arc.h
 *
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
    prev = NULL;
    next = NULL;
    link = NULL;
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
    prev = NULL;
    next = NULL;
    link = NULL;
    bezierArc = NULL;
    pwlArc = NULL;
    type = 0;
    setside( side );
    nuid = _nuid;
}

#endif /* __gluarc_h_ */
