
/**
 * A driRenderbuffer is dervied from gl_renderbuffer.
 * It describes a color buffer (front or back), a depth buffer, or stencil
 * buffer etc.
 * Specific to DRI drivers are the offset and pitch fields.
 */


#ifndef DRIRENDERBUFFER_H
#define DRIRENDERBUFFER_H

#include "mtypes.h"

typedef struct {
   struct gl_renderbuffer Base;

   /* Chars or bytes per pixel.  If Z and Stencil are stored together this
    * will typically be 32 whether this a depth or stencil renderbuffer.
    */
   GLint cpp;

   /* Buffer position and pitch (row stride).  Recall that for today's DRI
    * drivers, we have statically allocated color/depth/stencil buffers.
    * So this information describes the whole screen, not just a window.
    * To address pixels in a window, we need to know the window's position
    * and size with respect to the screen.
    */
   GLint offset;  /* in bytes */
   GLint pitch;   /* in pixels */

} driRenderbuffer;


driRenderbuffer *
driNewRenderbuffer(GLenum format, GLint cpp, GLint offset, GLint pitch);


#endif /* DRIRENDERBUFFER_H */
