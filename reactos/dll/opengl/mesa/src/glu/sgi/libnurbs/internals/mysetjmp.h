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
 * mysetjmp.h
 *
 */

#ifndef __glumysetjmp_h_
#define __glumysetjmp_h_

#ifdef STANDALONE
struct JumpBuffer;
extern "C" JumpBuffer *newJumpbuffer( void );
extern "C" void deleteJumpbuffer(JumpBuffer *);
extern "C" void mylongjmp( JumpBuffer *, int );
extern "C" int mysetjmp( JumpBuffer * );
#endif

#ifdef GLBUILD
#define setjmp		gl_setjmp
#define longjmp 	gl_longjmp
#endif

#if defined(LIBRARYBUILD) || defined(GLBUILD)
#include <setjmp.h>
#include <stdlib.h>

struct JumpBuffer {
    jmp_buf	buf;
};

inline JumpBuffer *
newJumpbuffer( void )
{
    return (JumpBuffer *) malloc( sizeof( JumpBuffer ) );
}

inline void
deleteJumpbuffer(JumpBuffer *jb)
{
   free( (void *) jb);
}

inline void
mylongjmp( JumpBuffer *j, int code ) 
{
    ::longjmp( j->buf, code );
}

inline int
mysetjmp( JumpBuffer *j )
{
    return setjmp( j->buf );
}
#endif

#endif /* __glumysetjmp_h_ */
