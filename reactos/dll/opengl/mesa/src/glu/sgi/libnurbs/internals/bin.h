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
 * bin.h
 *
 */

#ifndef __glubin_h_
#define __glubin_h_

#include "myassert.h"
#include "arc.h"
#include "defines.h"

class Bin 
{ /* a linked list of jordan arcs */
private:
    Arc_ptr head;/*first arc on list */
    Arc_ptr		current;	/* current arc on list */
public:
    			Bin();
			~Bin();
    inline Arc_ptr	firstarc( void );
    inline Arc_ptr	nextarc( void );
    inline Arc_ptr	removearc( void );
    inline int		isnonempty( void ) { return (head ? 1 : 0); }
    inline void		addarc( Arc_ptr );
    void 		remove_this_arc( Arc_ptr );
    int			numarcs( void );
    void 		adopt( void );
    void		markall( void );
    void		show( const char * );
    void		listBezier( void );
};

/*----------------------------------------------------------------------------
 * Bin::addarc - add an Arc_ptr to head of linked list of Arc_ptr
 *----------------------------------------------------------------------------
 */

inline void
Bin::addarc( Arc_ptr jarc )
{
   jarc->link = head;
   head = jarc;
}

/*----------------------------------------------------------------------------
 * Bin::removearc - remove first Arc_ptr from bin
 *----------------------------------------------------------------------------
 */

inline Arc_ptr
Bin::removearc( void )
{
    Arc_ptr jarc = head;

    if( jarc ) head = jarc->link;
    return jarc;
}


/*----------------------------------------------------------------------------
 * BinIter::nextarc - return current arc in bin and advance pointer to next arc
 *----------------------------------------------------------------------------
 */

inline Arc_ptr
Bin::nextarc( void )
{
    Arc_ptr jarc = current;

#ifdef DEBUG
    assert( jarc->check() != 0 );
#endif

    if( jarc ) current = jarc->link;
    return jarc;
}

/*----------------------------------------------------------------------------
 * BinIter::firstarc - set current arc to first arc of bin advance to next arc
 *----------------------------------------------------------------------------
 */

inline Arc_ptr
Bin::firstarc( void )
{
    current = head;
    return nextarc( );
}

#endif /* __glubin_h_ */
