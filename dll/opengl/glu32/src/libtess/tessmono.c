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
** Author: Eric Veach, July 1994.
**
*/

#include "gluos.h"
#include <stdlib.h>
#include "geom.h"
#include "mesh.h"
#include "tessmono.h"
#include <assert.h>

#define AddWinding(eDst,eSrc)	(eDst->winding += eSrc->winding, \
				 eDst->Sym->winding += eSrc->Sym->winding)

/* __gl_meshTessellateMonoRegion( face ) tessellates a monotone region
 * (what else would it do??)  The region must consist of a single
 * loop of half-edges (see mesh.h) oriented CCW.  "Monotone" in this
 * case means that any vertical line intersects the interior of the
 * region in a single interval.  
 *
 * Tessellation consists of adding interior edges (actually pairs of
 * half-edges), to split the region into non-overlapping triangles.
 *
 * The basic idea is explained in Preparata and Shamos (which I don''t
 * have handy right now), although their implementation is more
 * complicated than this one.  The are two edge chains, an upper chain
 * and a lower chain.  We process all vertices from both chains in order,
 * from right to left.
 *
 * The algorithm ensures that the following invariant holds after each
 * vertex is processed: the untessellated region consists of two
 * chains, where one chain (say the upper) is a single edge, and
 * the other chain is concave.  The left vertex of the single edge
 * is always to the left of all vertices in the concave chain.
 *
 * Each step consists of adding the rightmost unprocessed vertex to one
 * of the two chains, and forming a fan of triangles from the rightmost
 * of two chain endpoints.  Determining whether we can add each triangle
 * to the fan is a simple orientation test.  By making the fan as large
 * as possible, we restore the invariant (check it yourself).
 */
int __gl_meshTessellateMonoRegion( GLUface *face )
{
  GLUhalfEdge *up, *lo;

  /* All edges are oriented CCW around the boundary of the region.
   * First, find the half-edge whose origin vertex is rightmost.
   * Since the sweep goes from left to right, face->anEdge should
   * be close to the edge we want.
   */
  up = face->anEdge;
  assert( up->Lnext != up && up->Lnext->Lnext != up );

  for( ; VertLeq( up->Dst, up->Org ); up = up->Lprev )
    ;
  for( ; VertLeq( up->Org, up->Dst ); up = up->Lnext )
    ;
  lo = up->Lprev;

  while( up->Lnext != lo ) {
    if( VertLeq( up->Dst, lo->Org )) {
      /* up->Dst is on the left.  It is safe to form triangles from lo->Org.
       * The EdgeGoesLeft test guarantees progress even when some triangles
       * are CW, given that the upper and lower chains are truly monotone.
       */
      while( lo->Lnext != up && (EdgeGoesLeft( lo->Lnext )
	     || EdgeSign( lo->Org, lo->Dst, lo->Lnext->Dst ) <= 0 )) {
	GLUhalfEdge *tempHalfEdge= __gl_meshConnect( lo->Lnext, lo );
	if (tempHalfEdge == NULL) return 0;
	lo = tempHalfEdge->Sym;
      }
      lo = lo->Lprev;
    } else {
      /* lo->Org is on the left.  We can make CCW triangles from up->Dst. */
      while( lo->Lnext != up && (EdgeGoesRight( up->Lprev )
	     || EdgeSign( up->Dst, up->Org, up->Lprev->Org ) >= 0 )) {
	GLUhalfEdge *tempHalfEdge= __gl_meshConnect( up, up->Lprev );
	if (tempHalfEdge == NULL) return 0;
	up = tempHalfEdge->Sym;
      }
      up = up->Lnext;
    }
  }

  /* Now lo->Org == up->Dst == the leftmost vertex.  The remaining region
   * can be tessellated in a fan from this leftmost vertex.
   */
  assert( lo->Lnext != up );
  while( lo->Lnext->Lnext != up ) {
    GLUhalfEdge *tempHalfEdge= __gl_meshConnect( lo->Lnext, lo );
    if (tempHalfEdge == NULL) return 0;
    lo = tempHalfEdge->Sym;
  }

  return 1;
}


/* __gl_meshTessellateInterior( mesh ) tessellates each region of
 * the mesh which is marked "inside" the polygon.  Each such region
 * must be monotone.
 */
int __gl_meshTessellateInterior( GLUmesh *mesh )
{
  GLUface *f, *next;

  /*LINTED*/
  for( f = mesh->fHead.next; f != &mesh->fHead; f = next ) {
    /* Make sure we don''t try to tessellate the new triangles. */
    next = f->next;
    if( f->inside ) {
      if ( !__gl_meshTessellateMonoRegion( f ) ) return 0;
    }
  }

  return 1;
}


/* __gl_meshDiscardExterior( mesh ) zaps (ie. sets to NULL) all faces
 * which are not marked "inside" the polygon.  Since further mesh operations
 * on NULL faces are not allowed, the main purpose is to clean up the
 * mesh so that exterior loops are not represented in the data structure.
 */
void __gl_meshDiscardExterior( GLUmesh *mesh )
{
  GLUface *f, *next;

  /*LINTED*/
  for( f = mesh->fHead.next; f != &mesh->fHead; f = next ) {
    /* Since f will be destroyed, save its next pointer. */
    next = f->next;
    if( ! f->inside ) {
      __gl_meshZapFace( f );
    }
  }
}

#define MARKED_FOR_DELETION	0x7fffffff

/* __gl_meshSetWindingNumber( mesh, value, keepOnlyBoundary ) resets the
 * winding numbers on all edges so that regions marked "inside" the
 * polygon have a winding number of "value", and regions outside
 * have a winding number of 0.
 *
 * If keepOnlyBoundary is TRUE, it also deletes all edges which do not
 * separate an interior region from an exterior one.
 */
int __gl_meshSetWindingNumber( GLUmesh *mesh, int value,
			        GLboolean keepOnlyBoundary )
{
  GLUhalfEdge *e, *eNext;

  for( e = mesh->eHead.next; e != &mesh->eHead; e = eNext ) {
    eNext = e->next;
    if( e->Rface->inside != e->Lface->inside ) {

      /* This is a boundary edge (one side is interior, one is exterior). */
      e->winding = (e->Lface->inside) ? value : -value;
    } else {

      /* Both regions are interior, or both are exterior. */
      if( ! keepOnlyBoundary ) {
	e->winding = 0;
      } else {
	if ( !__gl_meshDelete( e ) ) return 0;
      }
    }
  }
  return 1;
}
