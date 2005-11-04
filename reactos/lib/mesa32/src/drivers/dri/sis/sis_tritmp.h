/* $XFree86*/ /* -*- c-basic-offset: 3 -*- */
/**************************************************************************

Copyright 2003 Eric Anholt
All Rights Reserved.

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
on the rights to use, copy, modify, merge, publish, distribute, sub
license, and/or sell copies of the Software, and to permit persons to whom
the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice (including the next
paragraph) shall be included in all copies or substantial portions of the
Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
ERIC ANHOLT BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

**************************************************************************/

/*
 * Authors:
 *   Eric Anholt <anholt@FreeBSD.org>
 */

static void TAG(sis_draw_tri_mmio)(sisContextPtr smesa, char *verts)
{
   sisVertexPtr v0 = (sisVertexPtr)verts;
   sisVertexPtr v1 = (sisVertexPtr)(verts + smesa->vertex_size * 4);
   sisVertexPtr v2 = (sisVertexPtr)(verts + smesa->vertex_size * 4 * 2);

   mWait3DCmdQueue (MMIO_VERT_REG_COUNT * 3);
   SIS_MMIO_WRITE_VERTEX(v0, 0, 0);
   SIS_MMIO_WRITE_VERTEX(v1, 1, 0);
   SIS_MMIO_WRITE_VERTEX(v2, 2, 1);
}

static void TAG(sis_draw_line_mmio)(sisContextPtr smesa, char *verts)
{
   sisVertexPtr v0 = (sisVertexPtr)verts;
   sisVertexPtr v1 = (sisVertexPtr)(verts + smesa->vertex_size * 4);

   mWait3DCmdQueue (MMIO_VERT_REG_COUNT * 2);
   SIS_MMIO_WRITE_VERTEX(v0, 0, 0);
   SIS_MMIO_WRITE_VERTEX(v1, 1, 1);
}

static void TAG(sis_draw_point_mmio)(sisContextPtr smesa, char *verts)
{
   sisVertexPtr v0 = (sisVertexPtr)verts;

   mWait3DCmdQueue (MMIO_VERT_REG_COUNT * 1);
   SIS_MMIO_WRITE_VERTEX(v0, 1, 1);
}

static __inline void TAG(sis_vert_init)( void )
{
   sis_tri_func_mmio[SIS_STATES] = TAG(sis_draw_tri_mmio);
   sis_line_func_mmio[SIS_STATES] = TAG(sis_draw_line_mmio);
   sis_point_func_mmio[SIS_STATES] = TAG(sis_draw_point_mmio);
}

#undef TAG
#undef SIS_STATES
