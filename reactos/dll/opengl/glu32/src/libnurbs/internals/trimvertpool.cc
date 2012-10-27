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
 * trimvertexpool.c++
 *
 */

#include "glimports.h"
#include "myassert.h"
#include "mystdio.h"
#include "mystring.h"
#include "trimvertex.h"
#include "trimvertpool.h"
#include "bufpool.h"

/*----------------------------------------------------------------------------
 * TrimVertexPool::TrimVertexPool 
 *----------------------------------------------------------------------------
 */
TrimVertexPool::TrimVertexPool( void )
	: pool( sizeof(TrimVertex)*3, 32, "Threevertspool" )
{
    // initialize array of pointers to vertex lists
    nextvlistslot = 0;
    vlistsize = INIT_VERTLISTSIZE;
    vlist = new TrimVertex_p[vlistsize];
}

/*----------------------------------------------------------------------------
 * TrimVertexPool::~TrimVertexPool 
 *----------------------------------------------------------------------------
 */
TrimVertexPool::~TrimVertexPool( void )
{
    // free all arrays of TrimVertices vertices
    while( nextvlistslot ) {
	delete [] vlist[--nextvlistslot];
    }

    // reallocate space for array of pointers to vertex lists
    if( vlist ) delete[] vlist;
}

/*----------------------------------------------------------------------------
 * TrimVertexPool::clear 
 *----------------------------------------------------------------------------
 */
void
TrimVertexPool::clear( void )
{
    // reinitialize pool of 3 vertex arrays    
    pool.clear();

    // free all arrays of TrimVertices vertices
    while( nextvlistslot ) {
	delete [] vlist[--nextvlistslot];
	vlist[nextvlistslot] = 0;
    }

    // reallocate space for array of pointers to vertex lists
    if( vlist ) delete[] vlist;
    vlist = new TrimVertex_p[vlistsize];
}


/*----------------------------------------------------------------------------
 * TrimVertexPool::get - allocate a vertex list
 *----------------------------------------------------------------------------
 */
TrimVertex *
TrimVertexPool::get( int n )
{
    TrimVertex	*v;
    if( n == 3 ) {
	v = (TrimVertex *) pool.new_buffer();
    } else {
        if( nextvlistslot == vlistsize ) {
	    vlistsize *= 2;
	    TrimVertex_p *nvlist = new TrimVertex_p[vlistsize];
	    memcpy( nvlist, vlist, nextvlistslot * sizeof(TrimVertex_p) );
	    if( vlist ) delete[] vlist;
	    vlist = nvlist;
        }
        v = vlist[nextvlistslot++] = new TrimVertex[n];
    }
    return v;
}
