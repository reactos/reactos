/* $XFree86*/ /* -*- c-basic-offset: 3 -*- */
/*
 * Copyright 2005 Eric Anholt
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * Authors:
 *    Eric Anholt <anholt@FreeBSD.org>
 *    Jim Duchek <jim@linuxpimps.com>	-- Utah GLX 6326 code
 *    Alan Cox <alan@redhat.com>	-- 6326 Debugging
 *
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

#if !(SIS_STATES & VERT_UV1)
static void TAG(sis6326_draw_tri_mmio)(sisContextPtr smesa, char *verts)
{
   sisVertexPtr v0 = (sisVertexPtr)verts;
   sisVertexPtr v1 = (sisVertexPtr)(verts + smesa->vertex_size * 4);
   sisVertexPtr v2 = (sisVertexPtr)(verts + smesa->vertex_size * 4 * 2);
   GLfloat x0, x1, x2;
   GLfloat y0, y1, y2;
   GLfloat delt02, diffx02, diffy02, diffy12;
   GLint dwPrimitiveSet = smesa->dwPrimitiveSet;
   sisVertex tv0, tv1, tv2;
   
   /* XXX Culling? */

   tv0 = *v0;
   tv1 = *v1;
   tv2 = *v2;
   tv0.v.y = Y_FLIP(tv0.v.y);
   tv1.v.y = Y_FLIP(tv1.v.y);
   tv2.v.y = Y_FLIP(tv2.v.y);
   v0 = &tv0;
   v1 = &tv1;
   v2 = &tv2;

   /* Cull polygons we won't draw. The hardware draws funky things if it
      is fed these */
   if((((v1->v.x - v0->v.x) * (v0->v.y - v2->v.y)) +
       ((v1->v.y - v0->v.y) * (v2->v.x - v0->v.x))) < 0)
      return;
   y0 = v0->v.y;
   y1 = v1->v.y;
   y2 = v2->v.y;
   

   if (y0 > y1) {
      if (y1 > y2) {
         x0 = v0->v.x;
         x1 = v1->v.x;
         x2 = v2->v.x;
         dwPrimitiveSet |= OP_6326_3D_ATOP | OP_6326_3D_BMID | OP_6326_3D_CBOT;
         if ((SIS_STATES & VERT_SMOOTH) == 0)
            dwPrimitiveSet |= OP_6326_3D_SHADE_FLAT_BOT;
      } else {
         if (y0 > y2) {
            x0 = v0->v.x;
            x1 = v2->v.x;
            y1 = v2->v.y;
            dwPrimitiveSet |= OP_6326_3D_ATOP | OP_6326_3D_CMID |
                OP_6326_3D_BBOT;
            if ((SIS_STATES & VERT_SMOOTH) == 0)
               dwPrimitiveSet |= OP_6326_3D_SHADE_FLAT_MID;
         } else {
            x0 = v2->v.x;
            y0 = v2->v.y;
            x1 = v0->v.x;
            y1 = v0->v.y;
            dwPrimitiveSet |= OP_6326_3D_CTOP | OP_6326_3D_AMID |
                OP_6326_3D_BBOT;
            if ((SIS_STATES & VERT_SMOOTH) == 0)
               dwPrimitiveSet |= OP_6326_3D_SHADE_FLAT_TOP;
         }
         x2 = v1->v.x;
         y2 = v1->v.y;
      }
   } else {
      if (y0 > y2) {
         x0 = v1->v.x;
         y0 = v1->v.y;
         x1 = v0->v.x;
         y1 = v0->v.y;
         x2 = v2->v.x;
         dwPrimitiveSet |= OP_6326_3D_BTOP | OP_6326_3D_AMID | OP_6326_3D_CBOT;
         if ((SIS_STATES & VERT_SMOOTH) == 0)
            dwPrimitiveSet |= OP_6326_3D_SHADE_FLAT_BOT;
      } else {
         if (y1 > y2) {
            x0 = v1->v.x;
            y0 = v1->v.y;
            x1 = v2->v.x;
            y1 = v2->v.y;
            dwPrimitiveSet |= OP_6326_3D_BTOP | OP_6326_3D_CMID |
                OP_6326_3D_ABOT;
            if ((SIS_STATES & VERT_SMOOTH) == 0)
               dwPrimitiveSet |= OP_6326_3D_SHADE_FLAT_MID;
         } else {
            x0 = v2->v.x;
            y0 = v2->v.y;
            x1 = v1->v.x;
            dwPrimitiveSet |= OP_6326_3D_CTOP | OP_6326_3D_BMID |
                OP_6326_3D_ABOT;
            if ((SIS_STATES & VERT_SMOOTH) == 0)
               dwPrimitiveSet |= OP_6326_3D_SHADE_FLAT_TOP;
         }
         x2 = v0->v.x;
         y2 = v0->v.y;
      }
   }

   if (x1 <= x0 && x1 <= x2) {
      dwPrimitiveSet |= OP_3D_DIRECTION_LEFT;
   } else if (x1 < x0 || x1 < x2) {
      GLfloat tmp;

      diffx02 = x0 - x2;
      diffy02 = y0 - y2;
      diffy12 = y1 - y2;

      delt02 = diffx02 / diffy02;
      tmp = x1 - (diffy12 * delt02 + x2);

      if (tmp <= 0.0)
         dwPrimitiveSet |= OP_3D_DIRECTION_LEFT;
   }
   
   tv0 = *v0;
   tv1 = *v1;
   tv2 = *v2;
   tv0.v.y = Y_FLIP(tv0.v.y);
   tv1.v.y = Y_FLIP(tv1.v.y);
   tv2.v.y = Y_FLIP(tv2.v.y);
   v0 = &tv0;
   v1 = &tv1;
   v2 = &tv2;
   
   y0 = v0->v.y;
   y1 = v1->v.y;
   y2 = v2->v.y;

/*   fprintf(stderr, "Vertex0 %f %f %f\n", v0->v.x, v0->v.y, v0->v.z);
   fprintf(stderr, "Vertex1 %f %f %f\n", v1->v.x, v1->v.y, v1->v.z);
   fprintf(stderr, "Vertex2 %f %f %f\n", v2->v.x, v2->v.y, v2->v.z);*/
   mWait3DCmdQueue(MMIO_VERT_REG_COUNT * 3 + 1);
   MMIO(REG_3D_PrimitiveSet, dwPrimitiveSet); 
   SIS_MMIO_WRITE_VERTEX(v0, 0, 0);
   SIS_MMIO_WRITE_VERTEX(v1, 1, 0);
   SIS_MMIO_WRITE_VERTEX(v2, 2, 1);
   mEndPrimitive();
}

static void TAG(sis6326_draw_line_mmio)(sisContextPtr smesa, char *verts)
{
   sisVertexPtr v0 = (sisVertexPtr)verts;
   sisVertexPtr v1 = (sisVertexPtr)(verts + smesa->vertex_size * 4);
   GLint dwPrimitiveSet = smesa->dwPrimitiveSet;

   if (abs(v0->v.y - v1->v.y) > abs(v0->v.x - v1->v.x))
   {
      dwPrimitiveSet |= OP_3D_DIRECTION_VERTICAL;
      if (v0->v.y > v1->v.y)
         dwPrimitiveSet |= OP_6326_3D_ATOP | OP_6326_3D_BBOT;
      else
         dwPrimitiveSet |= OP_6326_3D_BTOP | OP_6326_3D_ABOT;
   } else {
      if (v0->v.y > v1->v.y)
         dwPrimitiveSet |= OP_6326_3D_BTOP | OP_6326_3D_ABOT;
      else
         dwPrimitiveSet |= OP_6326_3D_ATOP | OP_6326_3D_BBOT;
   }

   mWait3DCmdQueue (MMIO_VERT_REG_COUNT * 2 + 1);
   MMIO(REG_3D_PrimitiveSet, dwPrimitiveSet); 
   SIS_MMIO_WRITE_VERTEX(v0, 0, 0);
   SIS_MMIO_WRITE_VERTEX(v1, 1, 1);
}

static void TAG(sis6326_draw_point_mmio)(sisContextPtr smesa, char *verts)
{
   sisVertexPtr v0 = (sisVertexPtr)verts;

   mWait3DCmdQueue (MMIO_VERT_REG_COUNT * 1 + 1);
   MMIO(REG_3D_PrimitiveSet, smesa->dwPrimitiveSet | OP_6326_3D_ATOP); 
   SIS_MMIO_WRITE_VERTEX(v0, 1, 1);
}
#endif

static __inline void TAG(sis_vert_init)( void )
{
   sis_tri_func_mmio[SIS_STATES] = TAG(sis_draw_tri_mmio);
   sis_line_func_mmio[SIS_STATES] = TAG(sis_draw_line_mmio);
   sis_point_func_mmio[SIS_STATES] = TAG(sis_draw_point_mmio);
#if !(SIS_STATES & VERT_UV1)
   sis_tri_func_mmio[SIS_STATES | VERT_6326] = TAG(sis6326_draw_tri_mmio);
   sis_line_func_mmio[SIS_STATES | VERT_6326] = TAG(sis6326_draw_line_mmio);
   sis_point_func_mmio[SIS_STATES | VERT_6326] = TAG(sis6326_draw_point_mmio);
#endif
}

#undef TAG
#undef SIS_STATES
