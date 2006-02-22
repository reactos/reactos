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
 * bufpool.h
 *
 * $Date$ $Revision: 1.1 $
 * $Header: /cygdrive/c/RCVS/CVS/ReactOS/reactos/lib/glu32/libnurbs/internals/bufpool.h,v 1.1 2004/02/02 16:39:11 navaraf Exp $
 */

#ifndef __glubufpool_h_
#define __glubufpool_h_

#include "gluos.h"
#include "myassert.h"
#include "mystdlib.h"

#define NBLOCKS	32

class Buffer {
	friend class 	Pool;
	Buffer	*	next;		/* next buffer on free list	*/
};

class Pool {
public:
			Pool( int, int, char * );
			~Pool( void );
    inline void*	new_buffer( void );
    inline void		free_buffer( void * );
    void		clear( void );
    
private:
    void		grow( void );

protected:
    Buffer		*freelist;		/* linked list of free buffers */
    char		*blocklist[NBLOCKS];	/* blocks of malloced memory */
    int			nextblock;		/* next free block index */
    char		*curblock;		/* last malloced block */
    int			buffersize;		/* bytes per buffer */
    int			nextsize;		/* size of next block of memory	*/
    int			nextfree;		/* byte offset past next free buffer */
    int			initsize;
    enum Magic { is_allocated = 0xf3a1, is_free = 0xf1a2 };
    char		*name;			/* name of the pool */
    Magic		magic;			/* marker for valid pool */
};

/*-----------------------------------------------------------------------------
 * Pool::free_buffer - return a buffer to a pool
 *-----------------------------------------------------------------------------
 */

inline void
Pool::free_buffer( void *b )
{
    assert( (this != 0) && (magic == is_allocated) );

    /* add buffer to singly connected free list */

    ((Buffer *) b)->next = freelist;
    freelist = (Buffer *) b;
}


/*-----------------------------------------------------------------------------
 * Pool::new_buffer - allocate a buffer from a pool
 *-----------------------------------------------------------------------------
 */

inline void * 
Pool::new_buffer( void )
{
    void *buffer;

    assert( (this != 0) && (magic == is_allocated) );

    /* find free buffer */

    if( freelist ) {
    	buffer = (void *) freelist; 
    	freelist = freelist->next;
    } else {
    	if( ! nextfree )
    	    grow( );
    	nextfree -= buffersize;;
    	buffer = (void *) (curblock + nextfree);
    }
    return buffer;
}
	
class PooledObj {
public:
    inline void *	operator new( size_t, Pool & );
    inline void * 	operator new( size_t, void *);
    inline void * 	operator new( size_t s)
				{ return ::new char[s]; }
    inline void 	operator delete( void * ) { assert( 0 ); }
    inline void		deleteMe( Pool & );
};

inline void *
PooledObj::operator new( size_t, Pool& pool )
{
    return pool.new_buffer();
}

inline void
PooledObj::deleteMe( Pool& pool )
{
    pool.free_buffer( (void *) this );
}

#endif /* __glubufpool_h_ */
