
/**************************************************************************

Copyright 2001 VA Linux Systems Inc., Fremont, California.

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
ATI, VA LINUX SYSTEMS AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
USE OR OTHER DEALINGS IN THE SOFTWARE.

**************************************************************************/

/* $XFree86: xc/lib/GL/mesa/src/drv/i830/i830_ioctl.h,v 1.3 2002/10/30 12:51:35 alanh Exp $ */

/*
 * Author:
 *   Jeff Hartmann <jhartmann@2d3d.com>
 *   Graeme Fisher <graeme@2d3d.co.za>
 *   Abraham vd Merwe <abraham@2d3d.co.za>
 *
 * Heavily based on the I810 driver, which was written by:
 *   Keith Whitwell <keith@tungstengraphics.com>
 */

#ifndef I830_IOCTL_H
#define I830_IOCTL_H

#include "i830_context.h"

GLuint *i830AllocDwords (i830ContextPtr imesa, int dwords);
void i830EmitPrim( i830ContextPtr imesa );
void i830FlushPrims( i830ContextPtr mmesa );
void i830FlushPrimsLocked( i830ContextPtr mmesa );
void i830FlushPrimsGetBuffer( i830ContextPtr imesa );
void i830FlushPrimsGetBufferLocked( i830ContextPtr imesa );
void i830WaitAgeLocked( i830ContextPtr imesa, int age );
void i830WaitAge( i830ContextPtr imesa, int age );
void i830DmaFinish( i830ContextPtr imesa );
void i830RegetLockQuiescent( i830ContextPtr imesa );
void i830InitIoctlFuncs( struct dd_function_table *functions );
void i830CopyBuffer( const __DRIdrawablePrivate *dpriv );
void i830PageFlip( const __DRIdrawablePrivate *dpriv );
int i830_check_copy(int fd);

#define I830_STATECHANGE(imesa, flag)               	\
do {  							\
   if (imesa->vertex_low != imesa->vertex_last_prim)  	\
      i830FlushPrims(imesa);                    	\
   imesa->dirty |= flag;                   		\
} while (0)


#define I830_FIREVERTICES(imesa)      	\
do {                             	\
   if (imesa->vertex_buffer) {  	\
      i830FlushPrims(imesa);		\
}                                   	\
} while (0)


static __inline GLuint *i830AllocDmaLow( i830ContextPtr imesa, int bytes )
{
   if (imesa->vertex_low + bytes > imesa->vertex_high) {
      i830FlushPrimsGetBuffer( imesa );
   }
   
   {
      GLuint *start = (GLuint *)(imesa->vertex_addr + imesa->vertex_low);
      imesa->vertex_low += bytes;
      return start;
   }
}

static __inline GLuint *i830AllocDmaLowLocked( i830ContextPtr imesa, 
					       int bytes )
{
   if (imesa->vertex_low + bytes > imesa->vertex_high) {
      i830FlushPrimsGetBufferLocked( imesa );
   }
   
   {
      GLuint *start = (GLuint *)(imesa->vertex_addr + imesa->vertex_low);
      imesa->vertex_low += bytes;
      return start;
   }
}


#endif
