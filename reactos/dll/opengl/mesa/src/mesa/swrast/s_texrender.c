
#include "main/context.h"
#include "main/colormac.h"
#include "main/fbobject.h"
#include "main/macros.h"
#include "main/teximage.h"
#include "main/renderbuffer.h"
#include "swrast/swrast.h"
#include "swrast/s_context.h"
#include "swrast/s_texfetch.h"


/*
 * Render-to-texture code for GL_EXT_framebuffer_object
 */


static void
delete_texture_wrapper(struct gl_renderbuffer *rb)
{
   ASSERT(rb->RefCount == 0);
   free(rb);
}


/**
 * This function creates a renderbuffer object which wraps a texture image.
 * The new renderbuffer is plugged into the given attachment point.
 * This allows rendering into the texture as if it were a renderbuffer.
 */
static void
wrap_texture(struct gl_context *ctx, struct gl_renderbuffer_attachment *att)
{
   struct gl_renderbuffer *rb;
   const GLuint name = 0;

   ASSERT(att->Type == GL_TEXTURE);
   ASSERT(att->Renderbuffer == NULL);

   rb = ctx->Driver.NewRenderbuffer(ctx, name);
   if (!rb) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "wrap_texture");
      return;
   }

   /* init base gl_renderbuffer fields */
   _mesa_init_renderbuffer(rb, name);
   /* plug in our texture_renderbuffer-specific functions */
   rb->Delete = delete_texture_wrapper;
   rb->AllocStorage = NULL; /* illegal! */

   /* update attachment point */
   _mesa_reference_renderbuffer(&att->Renderbuffer, rb);
}

/**
 * Update the renderbuffer wrapper for rendering to a texture.
 * For example, update the width, height of the RB based on the texture size,
 * update the internal format info, etc.
 */
static void
update_wrapper(struct gl_context *ctx, struct gl_renderbuffer_attachment *att)
{
   struct gl_renderbuffer *rb = att->Renderbuffer;
   struct swrast_renderbuffer *srb = swrast_renderbuffer(rb);
   struct swrast_texture_image *swImage;
   gl_format format;
   GLuint zOffset;

   (void) ctx;

   swImage = swrast_texture_image(_mesa_get_attachment_teximage(att));
   assert(swImage);

   format = swImage->Base.TexFormat;

   if (att->Texture->Target == GL_TEXTURE_1D_ARRAY_EXT) {
      zOffset = 0;
   }
   else {
      zOffset = att->Zoffset;
   }

   rb->Width = swImage->Base.Width;
   rb->Height = swImage->Base.Height;
   rb->InternalFormat = swImage->Base.InternalFormat;
   rb->_BaseFormat = _mesa_get_format_base_format(format);

   /* Want to store linear values, not sRGB */
   rb->Format = _mesa_get_srgb_format_linear(format);
 
   /* Set the gl_renderbuffer::Buffer field so that mapping the buffer
    * succeeds.
     */
   if (att->Texture->Target == GL_TEXTURE_3D ||
       att->Texture->Target == GL_TEXTURE_2D_ARRAY_EXT) {
      srb->Buffer = swImage->Buffer +
         swImage->ImageOffsets[zOffset] * _mesa_get_format_bytes(format);
   }
   else {
      srb->Buffer = swImage->Buffer;
   }
}



/**
 * Called when rendering to a texture image begins, or when changing
 * the dest mipmap level, cube face, etc.
 * This is a fallback routine for software render-to-texture.
 *
 * Called via the glRenderbufferTexture1D/2D/3D() functions
 * and elsewhere (such as glTexImage2D).
 *
 * The image we're rendering into is
 * att->Texture->Image[att->CubeMapFace][att->TextureLevel];
 * It'll never be NULL.
 *
 * \param fb  the framebuffer object the texture is being bound to
 * \param att  the fb attachment point of the texture
 *
 * \sa _mesa_framebuffer_renderbuffer
 */
void
_swrast_render_texture(struct gl_context *ctx,
                       struct gl_framebuffer *fb,
                       struct gl_renderbuffer_attachment *att)
{
   (void) fb;

   if (!att->Renderbuffer) {
      wrap_texture(ctx, att);
   }
   update_wrapper(ctx, att);
}


void
_swrast_finish_render_texture(struct gl_context *ctx,
                              struct gl_renderbuffer_attachment *att)
{
   /* do nothing */
   /* The renderbuffer texture wrapper will get deleted by the
    * normal mechanism for deleting renderbuffers.
    */
   (void) ctx;
   (void) att;
}
