/* $Id: logic.c,v 1.7 1997/07/24 01:24:11 brianp Exp $ */

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
 * $Log: logic.c,v $
 * Revision 1.7  1997/07/24 01:24:11  brianp
 * changed precompiled header symbol from PCH to PC_HEADER
 *
 * Revision 1.6  1997/05/28 03:25:26  brianp
 * added precompiled header (PCH) support
 *
 * Revision 1.5  1997/04/20 20:28:49  brianp
 * replaced abort() with gl_problem()
 *
 * Revision 1.4  1997/03/04 18:56:57  brianp
 * added #include <stdlib.h> for abort()
 *
 * Revision 1.3  1997/01/28 22:16:31  brianp
 * added gl_logicop_rgba_span() and gl_logicop_rgba_pixels()
 *
 * Revision 1.2  1997/01/04 00:13:11  brianp
 * was using ! instead of ~ to invert pixel bits (ugh!)
 *
 * Revision 1.1  1996/09/13 01:38:16  brianp
 * Initial revision
 *
 */


#ifdef PC_HEADER
#include "all.h"
#else
#include <stdlib.h>
#include "alphabuf.h"
#include "context.h"
#include "dlist.h"
#include "logic.h"
#include "macros.h"
#include "pb.h"
#include "span.h"
#include "types.h"
#endif



void gl_LogicOp( GLcontext *ctx, GLenum opcode )
{
   if (INSIDE_BEGIN_END(ctx)) {
      gl_error( ctx, GL_INVALID_OPERATION, "glLogicOp" );
      return;
   }
   switch (opcode) {
      case GL_CLEAR:
      case GL_SET:
      case GL_COPY:
      case GL_COPY_INVERTED:
      case GL_NOOP:
      case GL_INVERT:
      case GL_AND:
      case GL_NAND:
      case GL_OR:
      case GL_NOR:
      case GL_XOR:
      case GL_EQUIV:
      case GL_AND_REVERSE:
      case GL_AND_INVERTED:
      case GL_OR_REVERSE:
      case GL_OR_INVERTED:
         ctx->Color.LogicOp = opcode;
         ctx->NewState |= NEW_RASTER_OPS;
	 return;
      default:
         gl_error( ctx, GL_INVALID_ENUM, "glLogicOp" );
	 return;
   }
}




/*
 * Apply the current logic operator to a span of CI pixels.  This is only
 * used if the device driver can't do logic ops.
 */
void gl_logicop_ci_span( GLcontext *ctx, GLuint n, GLint x, GLint y,
                         GLuint index[], GLubyte mask[] )
{
   GLuint dest[MAX_WIDTH];
   GLuint i;

   /* Read dest values from frame buffer */
   (*ctx->Driver.ReadIndexSpan)( ctx, n, x, y, dest );

   switch (ctx->Color.LogicOp) {
      case GL_CLEAR:
         for (i=0;i<n;i++) {
	    if (mask[i]) {
	       index[i] = 0;
	    }
	 }
	 break;
      case GL_SET:
         for (i=0;i<n;i++) {
	    if (mask[i]) {
	       index[i] = 1;
	    }
	 }
	 break;
      case GL_COPY:
	 /* do nothing */
	 break;
      case GL_COPY_INVERTED:
         for (i=0;i<n;i++) {
	    if (mask[i]) {
	       index[i] = ~index[i];
	    }
	 }
	 break;
      case GL_NOOP:
         for (i=0;i<n;i++) {
	    if (mask[i]) {
	       index[i] = dest[i];
	    }
	 }
	 break;
      case GL_INVERT:
         for (i=0;i<n;i++) {
	    if (mask[i]) {
	       index[i] = ~dest[i];
	    }
	 }
	 break;
      case GL_AND:
         for (i=0;i<n;i++) {
	    if (mask[i]) {
	       index[i] &= dest[i];
	    }
	 }
	 break;
      case GL_NAND:
         for (i=0;i<n;i++) {
	    if (mask[i]) {
	       index[i] = ~(index[i] & dest[i]);
	    }
	 }
	 break;
      case GL_OR:
         for (i=0;i<n;i++) {
	    if (mask[i]) {
	       index[i] |= dest[i];
	    }
	 }
	 break;
      case GL_NOR:
         for (i=0;i<n;i++) {
	    if (mask[i]) {
	       index[i] = ~(index[i] | dest[i]);
	    }
	 }
	 break;
      case GL_XOR:
         for (i=0;i<n;i++) {
	    if (mask[i]) {
	       index[i] ^= dest[i];
	    }
	 }
	 break;
      case GL_EQUIV:
         for (i=0;i<n;i++) {
	    if (mask[i]) {
	       index[i] = ~(index[i] ^ dest[i]);
	    }
	 }
	 break;
      case GL_AND_REVERSE:
         for (i=0;i<n;i++) {
	    if (mask[i]) {
	       index[i] = index[i] & ~dest[i];
	    }
	 }
	 break;
      case GL_AND_INVERTED:
         for (i=0;i<n;i++) {
	    if (mask[i]) {
	       index[i] = ~index[i] & dest[i];
	    }
	 }
	 break;
      case GL_OR_REVERSE:
         for (i=0;i<n;i++) {
	    if (mask[i]) {
	       index[i] = index[i] | ~dest[i];
	    }
	 }
	 break;
      case GL_OR_INVERTED:
         for (i=0;i<n;i++) {
	    if (mask[i]) {
	       index[i] = ~index[i] | dest[i];
	    }
	 }
	 break;
      default:
	 gl_error( ctx, GL_INVALID_ENUM, "gl_logic error" );
   }
}



/*
 * Apply the current logic operator to an array of CI pixels.  This is only
 * used if the device driver can't do logic ops.
 */
void gl_logicop_ci_pixels( GLcontext *ctx,
                           GLuint n, const GLint x[], const GLint y[],
                           GLuint index[], GLubyte mask[] )
{
   GLuint dest[PB_SIZE];
   GLuint i;

   /* Read dest values from frame buffer */
   (*ctx->Driver.ReadIndexPixels)( ctx, n, x, y, dest, mask );

   switch (ctx->Color.LogicOp) {
      case GL_CLEAR:
         for (i=0;i<n;i++) {
	    if (mask[i]) {
	       index[i] = 0;
	    }
	 }
	 break;
      case GL_SET:
         for (i=0;i<n;i++) {
	    if (mask[i]) {
	       index[i] = 1;
	    }
	 }
	 break;
      case GL_COPY:
	 /* do nothing */
	 break;
      case GL_COPY_INVERTED:
         for (i=0;i<n;i++) {
	    if (mask[i]) {
	       index[i] = ~index[i];
	    }
	 }
	 break;
      case GL_NOOP:
         for (i=0;i<n;i++) {
	    if (mask[i]) {
	       index[i] = dest[i];
	    }
	 }
	 break;
      case GL_INVERT:
         for (i=0;i<n;i++) {
	    if (mask[i]) {
	       index[i] = ~dest[i];
	    }
	 }
	 break;
      case GL_AND:
         for (i=0;i<n;i++) {
	    if (mask[i]) {
	       index[i] &= dest[i];
	    }
	 }
	 break;
      case GL_NAND:
         for (i=0;i<n;i++) {
	    if (mask[i]) {
	       index[i] = ~(index[i] & dest[i]);
	    }
	 }
	 break;
      case GL_OR:
         for (i=0;i<n;i++) {
	    if (mask[i]) {
	       index[i] |= dest[i];
	    }
	 }
	 break;
      case GL_NOR:
         for (i=0;i<n;i++) {
	    if (mask[i]) {
	       index[i] = ~(index[i] | dest[i]);
	    }
	 }
	 break;
      case GL_XOR:
         for (i=0;i<n;i++) {
	    if (mask[i]) {
	       index[i] ^= dest[i];
	    }
	 }
	 break;
      case GL_EQUIV:
         for (i=0;i<n;i++) {
	    if (mask[i]) {
	       index[i] = ~(index[i] ^ dest[i]);
	    }
	 }
	 break;
      case GL_AND_REVERSE:
         for (i=0;i<n;i++) {
	    if (mask[i]) {
	       index[i] = index[i] & ~dest[i];
	    }
	 }
	 break;
      case GL_AND_INVERTED:
         for (i=0;i<n;i++) {
	    if (mask[i]) {
	       index[i] = ~index[i] & dest[i];
	    }
	 }
	 break;
      case GL_OR_REVERSE:
         for (i=0;i<n;i++) {
	    if (mask[i]) {
	       index[i] = index[i] | ~dest[i];
	    }
	 }
	 break;
      case GL_OR_INVERTED:
         for (i=0;i<n;i++) {
	    if (mask[i]) {
	       index[i] = ~index[i] | dest[i];
	    }
	 }
	 break;
      default:
	 gl_error( ctx, GL_INVALID_ENUM, "gl_logic_pixels error" );
   }
}



/*
 * Apply the current logic operator to a span of RGBA pixels.  This is only
 * used if the device driver can't do logic ops.
 */
void gl_logicop_rgba_span( GLcontext *ctx,
                           GLuint n, GLint x, GLint y,
                           GLubyte red[], GLubyte green[],
                           GLubyte blue[], GLubyte alpha[],
                           GLubyte mask[] )
{
   GLubyte rdest[MAX_WIDTH], gdest[MAX_WIDTH];
   GLubyte bdest[MAX_WIDTH], adest[MAX_WIDTH];
   GLuint i;

   /* Read span of current frame buffer pixels */
   gl_read_color_span( ctx, n, x, y, rdest, gdest, bdest, adest );

   /* apply logic op */
   switch (ctx->Color.LogicOp) {
      case GL_CLEAR:
         for (i=0;i<n;i++) {
            if (mask[i]) {
               red[i] = green[i] = blue[i] = alpha[i] = 0;
            }
         }
         break;
      case GL_SET:
         {
            GLubyte r = (GLint) ctx->Visual->RedScale;
            GLubyte g = (GLint) ctx->Visual->GreenScale;
            GLubyte b = (GLint) ctx->Visual->BlueScale;
            GLubyte a = (GLint) ctx->Visual->AlphaScale;
            for (i=0;i<n;i++) {
               if (mask[i]) {
                  red[i]   = r;
                  green[i] = g;
                  blue[i]  = b;
                  alpha[i] = a;
               }
            }
         }
         break;
      case GL_COPY:
         /* do nothing */
         break;
      case GL_COPY_INVERTED:
         for (i=0;i<n;i++) {
            if (mask[i]) {
               red[i]   = ~red[i];
               green[i] = ~green[i];
               blue[i]  = ~blue[i];
               alpha[i] = ~alpha[i];
            }
         }
         break;
      case GL_NOOP:
         for (i=0;i<n;i++) {
            if (mask[i]) {
               red[i]   = rdest[i];
               green[i] = gdest[i];
               blue[i]  = bdest[i];
               alpha[i] = adest[i];
            }
         }
         break;
      case GL_INVERT:
         for (i=0;i<n;i++) {
            if (mask[i]) {
               red[i]   = ~rdest[i];
               green[i] = ~gdest[i];
               blue[i]  = ~bdest[i];
               alpha[i] = ~adest[i];
            }
         }
         break;
      case GL_AND:
         for (i=0;i<n;i++) {
            if (mask[i]) {
               red[i]   &= rdest[i];
               green[i] &= gdest[i];
               blue[i]  &= bdest[i];
               alpha[i] &= adest[i];
            }
         }
         break;
      case GL_NAND:
         for (i=0;i<n;i++) {
            if (mask[i]) {
               red[i]   = ~(red[i]   & rdest[i]);
               green[i] = ~(green[i] & gdest[i]);
               blue[i]  = ~(blue[i]  & bdest[i]);
               alpha[i] = ~(alpha[i] & adest[i]);
            }
         }
         break;
      case GL_OR:
         for (i=0;i<n;i++) {
            if (mask[i]) {
               red[i]   |= rdest[i];
               green[i] |= gdest[i];
               blue[i]  |= bdest[i];
               alpha[i] |= adest[i];
            }
         }
         break;
      case GL_NOR:
         for (i=0;i<n;i++) {
            if (mask[i]) {
               red[i]   = ~(red[i]   | rdest[i]);
               green[i] = ~(green[i] | gdest[i]);
               blue[i]  = ~(blue[i]  | bdest[i]);
               alpha[i] = ~(alpha[i] | adest[i]);
            }
         }
         break;
      case GL_XOR:
         for (i=0;i<n;i++) {
            if (mask[i]) {
               red[i]   ^= rdest[i];
               green[i] ^= gdest[i];
               blue[i]  ^= bdest[i];
               alpha[i] ^= adest[i];
            }
         }
         break;
      case GL_EQUIV:
         for (i=0;i<n;i++) {
            if (mask[i]) {
               red[i]   = ~(red[i]   ^ rdest[i]);
               green[i] = ~(green[i] ^ gdest[i]);
               blue[i]  = ~(blue[i]  ^ bdest[i]);
               alpha[i] = ~(alpha[i] ^ adest[i]);
            }
         }
         break;
      case GL_AND_REVERSE:
         for (i=0;i<n;i++) {
            if (mask[i]) {
               red[i]   = red[i]   & ~rdest[i];
               green[i] = green[i] & ~gdest[i];
               blue[i]  = blue[i]  & ~bdest[i];
               alpha[i] = alpha[i] & ~adest[i];
            }
         }
         break;
      case GL_AND_INVERTED:
         for (i=0;i<n;i++) {
            if (mask[i]) {
               red[i]   = ~red[i]   & rdest[i];
               green[i] = ~green[i] & gdest[i];
               blue[i]  = ~blue[i]  & bdest[i];
               alpha[i] = ~alpha[i] & adest[i];
            }
         }
         break;
      case GL_OR_REVERSE:
         for (i=0;i<n;i++) {
            if (mask[i]) {
               red[i]   = red[i]   | ~rdest[i];
               green[i] = green[i] | ~gdest[i];
               blue[i]  = blue[i]  | ~bdest[i];
               alpha[i] = alpha[i] | ~adest[i];
            }
         }
         break;
      case GL_OR_INVERTED:
         for (i=0;i<n;i++) {
            if (mask[i]) {
               red[i]   = ~red[i]   | rdest[i];
               green[i] = ~green[i] | gdest[i];
               blue[i]  = ~blue[i]  | bdest[i];
               alpha[i] = ~alpha[i] | adest[i];
            }
         }
         break;
      default:
         /* should never happen */
         gl_problem(ctx, "Bad function in gl_logicop_rgba_span");
         return;
   }
}



/*
 * Apply the current logic operator to an array of RGBA pixels.  This is only
 * used if the device driver can't do logic ops.
 */
void gl_logicop_rgba_pixels( GLcontext *ctx,
                             GLuint n, const GLint x[], const GLint y[],
                             GLubyte red[], GLubyte green[],
                             GLubyte blue[], GLubyte alpha[],
                             GLubyte mask[] )
{
   GLubyte rdest[PB_SIZE], gdest[PB_SIZE], bdest[PB_SIZE], adest[PB_SIZE];
   GLuint i;

   /* Read pixels from current color buffer */
   (*ctx->Driver.ReadColorPixels)( ctx, n, x, y, rdest, gdest, bdest, adest, mask );
   if (ctx->RasterMask & ALPHABUF_BIT) {
      gl_read_alpha_pixels( ctx, n, x, y, adest, mask );
   }

   /* apply logic op */
   switch (ctx->Color.LogicOp) {
      case GL_CLEAR:
         for (i=0;i<n;i++) {
            if (mask[i]) {
               red[i] = green[i] = blue[i] = alpha[i] = 0;
            }
         }
         break;
      case GL_SET:
         {
            GLubyte r = (GLint) ctx->Visual->RedScale;
            GLubyte g = (GLint) ctx->Visual->GreenScale;
            GLubyte b = (GLint) ctx->Visual->BlueScale;
            GLubyte a = (GLint) ctx->Visual->AlphaScale;
            for (i=0;i<n;i++) {
               if (mask[i]) {
                  red[i]   = r;
                  green[i] = g;
                  blue[i]  = b;
                  alpha[i] = a;
               }
            }
         }
         break;
      case GL_COPY:
         /* do nothing */
         break;
      case GL_COPY_INVERTED:
         for (i=0;i<n;i++) {
            if (mask[i]) {
               red[i]   = ~red[i];
               green[i] = ~green[i];
               blue[i]  = ~blue[i];
               alpha[i] = ~alpha[i];
            }
         }
         break;
      case GL_NOOP:
         for (i=0;i<n;i++) {
            if (mask[i]) {
               red[i]   = rdest[i];
               green[i] = gdest[i];
               blue[i]  = bdest[i];
               alpha[i] = adest[i];
            }
         }
         break;
      case GL_INVERT:
         for (i=0;i<n;i++) {
            if (mask[i]) {
               red[i]   = ~rdest[i];
               green[i] = ~gdest[i];
               blue[i]  = ~bdest[i];
               alpha[i] = ~adest[i];
            }
         }
         break;
      case GL_AND:
         for (i=0;i<n;i++) {
            if (mask[i]) {
               red[i]   &= rdest[i];
               green[i] &= gdest[i];
               blue[i]  &= bdest[i];
               alpha[i] &= adest[i];
            }
         }
         break;
      case GL_NAND:
         for (i=0;i<n;i++) {
            if (mask[i]) {
               red[i]   = ~(red[i]   & rdest[i]);
               green[i] = ~(green[i] & gdest[i]);
               blue[i]  = ~(blue[i]  & bdest[i]);
               alpha[i] = ~(alpha[i] & adest[i]);
            }
         }
         break;
      case GL_OR:
         for (i=0;i<n;i++) {
            if (mask[i]) {
               red[i]   |= rdest[i];
               green[i] |= gdest[i];
               blue[i]  |= bdest[i];
               alpha[i] |= adest[i];
            }
         }
         break;
      case GL_NOR:
         for (i=0;i<n;i++) {
            if (mask[i]) {
               red[i]   = ~(red[i]   | rdest[i]);
               green[i] = ~(green[i] | gdest[i]);
               blue[i]  = ~(blue[i]  | bdest[i]);
               alpha[i] = ~(alpha[i] | adest[i]);
            }
         }
         break;
      case GL_XOR:
         for (i=0;i<n;i++) {
            if (mask[i]) {
               red[i]   ^= rdest[i];
               green[i] ^= gdest[i];
               blue[i]  ^= bdest[i];
               alpha[i] ^= adest[i];
            }
         }
         break;
      case GL_EQUIV:
         for (i=0;i<n;i++) {
            if (mask[i]) {
               red[i]   = ~(red[i]   ^ rdest[i]);
               green[i] = ~(green[i] ^ gdest[i]);
               blue[i]  = ~(blue[i]  ^ bdest[i]);
               alpha[i] = ~(alpha[i] ^ adest[i]);
            }
         }
         break;
      case GL_AND_REVERSE:
         for (i=0;i<n;i++) {
            if (mask[i]) {
               red[i]   = red[i]   & ~rdest[i];
               green[i] = green[i] & ~gdest[i];
               blue[i]  = blue[i]  & ~bdest[i];
               alpha[i] = alpha[i] & ~adest[i];
            }
         }
         break;
      case GL_AND_INVERTED:
         for (i=0;i<n;i++) {
            if (mask[i]) {
               red[i]   = ~red[i]   & rdest[i];
               green[i] = ~green[i] & gdest[i];
               blue[i]  = ~blue[i]  & bdest[i];
               alpha[i] = ~alpha[i] & adest[i];
            }
         }
         break;
      case GL_OR_REVERSE:
         for (i=0;i<n;i++) {
            if (mask[i]) {
               red[i]   = red[i]   | ~rdest[i];
               green[i] = green[i] | ~gdest[i];
               blue[i]  = blue[i]  | ~bdest[i];
               alpha[i] = alpha[i] | ~adest[i];
            }
         }
         break;
      case GL_OR_INVERTED:
         for (i=0;i<n;i++) {
            if (mask[i]) {
               red[i]   = ~red[i]   | rdest[i];
               green[i] = ~green[i] | gdest[i];
               blue[i]  = ~blue[i]  | bdest[i];
               alpha[i] = ~alpha[i] | adest[i];
            }
         }
         break;
      default:
         /* should never happen */
         gl_problem(ctx, "Bad function in gl_logicop_rgba_pixels");
         return;
   }
}
