
#include "mtypes.h"
#include "drirenderbuffer.h"
#include "renderbuffer.h"
#include "imports.h"


/**
 * This will get called when a window is resized.
 * Just update width, height and internal format fields for now.
 * There's usually no memory allocation above because the present
 * DRI drivers use statically-allocated full-screen buffers.
 */
static GLboolean
driRenderbufferStorage(GLcontext *ctx, struct gl_renderbuffer *rb,
                       GLenum internalFormat, GLuint width, GLuint height)
{
   rb->Width = width;
   rb->Height = height;
   rb->InternalFormat = internalFormat;
   return GL_TRUE;
}


/**
 * Allocate a new driRenderbuffer object.
 * Individual drivers are free to implement different versions of
 * this function.
 * \param format  Either GL_RGBA, GL_DEPTH_COMPONENT16, GL_DEPTH_COMPONENT24,
 *                GL_DEPTH_COMPONENT32, or GL_STENCIL_INDEX8_EXT (for now).
 * \param cpp  chars or bytes per pixel
 * \param offset  start of buffer with respect to framebuffer address
 * \param pitch   pixels per row
 */
driRenderbuffer *
driNewRenderbuffer(GLenum format, GLint cpp, GLint offset, GLint pitch)
{
   driRenderbuffer *drb;

   assert(format == GL_RGBA ||
          format == GL_DEPTH_COMPONENT16 ||
          format == GL_DEPTH_COMPONENT24 ||
          format == GL_DEPTH_COMPONENT32 ||
          format == GL_STENCIL_INDEX8_EXT);

   assert(cpp > 0);
   assert(pitch > 0);

   drb = _mesa_calloc(sizeof(driRenderbuffer));
   if (drb) {
      const GLuint name = 0;

      _mesa_init_renderbuffer(&drb->Base, name);

      /* Make sure we're using a null-valued GetPointer routine */
      assert(drb->Base.GetPointer(NULL, &drb->Base, 0, 0) == NULL);

      drb->Base.InternalFormat = format;

      if (format == GL_RGBA) {
         /* Color */
         drb->Base._BaseFormat = GL_RGBA;
         drb->Base.DataType = GL_UNSIGNED_BYTE;
      }
      else if (format == GL_DEPTH_COMPONENT16) {
         /* Depth */
         drb->Base._BaseFormat = GL_DEPTH_COMPONENT;
         /* we always Get/Put 32-bit Z values */
         drb->Base.DataType = GL_UNSIGNED_INT;
      }
      else if (format == GL_DEPTH_COMPONENT24) {
         /* Depth */
         drb->Base._BaseFormat = GL_DEPTH_COMPONENT;
         /* we always Get/Put 32-bit Z values */
         drb->Base.DataType = GL_UNSIGNED_INT;
      }
      else {
         /* Stencil */
         ASSERT(format == GL_STENCIL_INDEX8);
         drb->Base._BaseFormat = GL_STENCIL_INDEX;
         drb->Base.DataType = GL_UNSIGNED_BYTE;
      }

      /* XXX if we were allocating a user-created renderbuffer, we'd have
       * to fill in the ComponentSizes[] array too.
       */

      drb->Base.AllocStorage = driRenderbufferStorage;
      /* using default Delete function */


      /* DRI renderbuffer-specific fields: */
      drb->offset = offset;
      drb->pitch = pitch;
      drb->cpp = cpp;
   }
   return drb;
}
