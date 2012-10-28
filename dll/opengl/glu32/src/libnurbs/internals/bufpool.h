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
 * bufpool.h
 *
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
			Pool( int, int, const char * );
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
    const char		*name;			/* name of the pool */
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
    inline void         operator delete( void *, Pool & ) { assert( 0 ); }
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
