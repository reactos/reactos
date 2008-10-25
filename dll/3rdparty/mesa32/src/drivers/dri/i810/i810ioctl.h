/* $XFree86: xc/lib/GL/mesa/src/drv/i810/i810ioctl.h,v 1.7 2002/10/30 12:51:33 alanh Exp $ */

#ifndef I810_IOCTL_H
#define I810_IOCTL_H

#include "i810context.h"

void i810EmitPrim( i810ContextPtr imesa );
void i810FlushPrims( i810ContextPtr mmesa ); 
void i810FlushPrimsLocked( i810ContextPtr mmesa );
void i810FlushPrimsGetBuffer( i810ContextPtr imesa );

void i810WaitAgeLocked( i810ContextPtr imesa, int age );
void i810WaitAge( i810ContextPtr imesa, int age );
void i810DmaFinish( i810ContextPtr imesa );
void i810RegetLockQuiescent( i810ContextPtr imesa );
void i810InitIoctlFuncs( struct dd_function_table *functions );
void i810CopyBuffer( const __DRIdrawablePrivate *dpriv );
void i810PageFlip( const __DRIdrawablePrivate *dpriv );
int i810_check_copy(int fd);

#define I810_STATECHANGE(imesa, flag)				\
do {								\
   if (imesa->vertex_low != imesa->vertex_last_prim)		\
      i810FlushPrims(imesa);					\
   imesa->dirty |= flag;					\
} while (0)							\


#define I810_FIREVERTICES(imesa)				\
do {								\
   if (imesa->vertex_buffer) {					\
      i810FlushPrims(imesa);					\
   }								\
} while (0)

static __inline GLuint *i810AllocDmaLow( i810ContextPtr imesa, int bytes )
{
   if (imesa->vertex_low + bytes > imesa->vertex_high) 
      i810FlushPrimsGetBuffer( imesa );

   {
      GLuint *start = (GLuint *)(imesa->vertex_addr + imesa->vertex_low);
      imesa->vertex_low += bytes;
      return start;
   }
}

#endif
