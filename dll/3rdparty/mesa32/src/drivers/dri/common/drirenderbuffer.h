
/**
 * A driRenderbuffer is dervied from gl_renderbuffer.
 * It describes a color buffer (front or back), a depth buffer, or stencil
 * buffer etc.
 * Specific to DRI drivers are the offset and pitch fields.
 */


#ifndef DRIRENDERBUFFER_H
#define DRIRENDERBUFFER_H

#include "mtypes.h"
#include "dri_util.h"


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

   /* If the driver can do page flipping (full-screen double buffering)
    * the current front/back buffers may get swapped.
    * If page flipping is disabled, these  fields will be identical to
    * the offset/pitch/Data above.
    * If page flipping is enabled, and this is the front(back) renderbuffer,
    * flippedOffset/Pitch/Data will have the back(front) renderbuffer's values.
    */
   GLint flippedOffset;
   GLint flippedPitch;
   GLvoid *flippedData;  /* mmap'd address of buffer memory, if used */

   /* Pointer to corresponding __DRIdrawablePrivate.  This is used to compute
    * the window's position within the framebuffer.
    */
   __DRIdrawablePrivate *dPriv;

   /* XXX this is for radeon/r200 only.  We should really create a new
    * r200Renderbuffer class, derived from this class...  not a huge deal.
    */
   GLboolean depthHasSurface;

   /**
    * A handy flag to know if this is the back color buffer.
    * 
    * \note
    * This is currently only used by s3v and tdfx.
    */
   GLboolean backBuffer;
} driRenderbuffer;


extern driRenderbuffer *
driNewRenderbuffer(GLenum format, GLvoid *addr,
                   GLint cpp, GLint offset, GLint pitch,
                   __DRIdrawablePrivate *dPriv);

extern void
driFlipRenderbuffers(struct gl_framebuffer *fb, GLboolean flipped);


extern void
driUpdateFramebufferSize(GLcontext *ctx, const __DRIdrawablePrivate *dPriv);


#endif /* DRIRENDERBUFFER_H */
