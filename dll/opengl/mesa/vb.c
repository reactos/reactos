/* $Id: vb.c,v 1.8 1997/07/24 01:25:27 brianp Exp $ */

/*
 * Mesa 3-D graphics library
 * Version:  2.4
 * Copyright (C) 1995-1997  Brian Paul
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */


/*
 * $Log: vb.c,v $
 * Revision 1.8  1997/07/24 01:25:27  brianp
 * changed precompiled header symbol from PCH to PC_HEADER
 *
 * Revision 1.7  1997/05/28 03:26:49  brianp
 * added precompiled header (PCH) support
 *
 * Revision 1.6  1997/05/09 22:41:55  brianp
 * replaced gl_init_vb() with gl_alloc_vb()
 *
 * Revision 1.5  1997/04/24 00:29:36  brianp
 * added TexCoordSize
 *
 * Revision 1.4  1997/04/20 15:59:30  brianp
 * removed VERTEX2_BIT stuff
 *
 * Revision 1.3  1997/04/12 16:21:24  brianp
 * updated gl_init_vb()
 *
 * Revision 1.2  1996/09/27 01:31:08  brianp
 * make gl_init_vb() non-static
 *
 * Revision 1.1  1996/09/13 01:38:16  brianp
 * Initial revision
 *
 */


#ifdef PC_HEADER
#include "all.h"
#else
#include <stdlib.h>
#include "types.h"
#include "vb.h"
#endif


/*
 * Allocate and initialize a vertex buffer.
 */
struct vertex_buffer *gl_alloc_vb(void)
{
   struct vertex_buffer *vb;
   vb = (struct vertex_buffer *) calloc(sizeof(struct vertex_buffer), 1);
   if (vb) {
      /* set non-zero fields */
      GLuint i;
      for (i=0;i<VB_SIZE;i++) {
         vb->MaterialMask[i] = 0;
         vb->ClipMask[i] = 0;
         vb->Obj[i][3] = 1.0F;
         vb->TexCoord[i][2] = 0.0F;
         vb->TexCoord[i][3] = 1.0F;
      }
      vb->VertexSizeMask = VERTEX3_BIT;
      vb->TexCoordSize = 2;
      vb->MonoColor = GL_TRUE;
      vb->MonoMaterial = GL_TRUE;
      vb->MonoNormal = GL_TRUE;
      vb->ClipOrMask = 0;
      vb->ClipAndMask = CLIP_ALL_BITS;
   }
   return vb;
}
