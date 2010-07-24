
#include "context.h"
#include "fbobject.h"
#include "texformat.h"
#include "texrender.h"
#include "renderbuffer.h"


/*
 * Render-to-texture code for GL_EXT_framebuffer_object
 */


/**
 * Derived from gl_renderbuffer class
 */
struct texture_renderbuffer
{
   struct gl_renderbuffer Base;   /**< Base class object */
   struct gl_texture_image *TexImage;
   StoreTexelFunc Store;
   GLint Yoffset;                 /**< Layer for 1D array textures. */
   GLint Zoffset;                 /**< Layer for 2D array textures, or slice
				   * for 3D textures
				   */
};


/**
 * Get row of values from the renderbuffer that wraps a texture image.
 */
static void
texture_get_row(GLcontext *ctx, struct gl_renderbuffer *rb, GLuint count,
                GLint x, GLint y, void *values)
{
   const struct texture_renderbuffer *trb
      = (const struct texture_renderbuffer *) rb;
   const GLint z = trb->Zoffset;
   GLuint i;

   ASSERT(trb->TexImage->Width == rb->Width);
   ASSERT(trb->TexImage->Height == rb->Height);

   y += trb->Yoffset;

   if (rb->DataType == CHAN_TYPE) {
      GLchan *rgbaOut = (GLchan *) values;
      for (i = 0; i < count; i++) {
         trb->TexImage->FetchTexelc(trb->TexImage, x + i, y, z, rgbaOut + 4 * i);
      }
   }
   else if (rb->DataType == GL_UNSIGNED_SHORT) {
      GLushort *zValues = (GLushort *) values;
      for (i = 0; i < count; i++) {
         GLfloat flt;
         trb->TexImage->FetchTexelf(trb->TexImage, x + i, y, z, &flt);
         zValues[i] = (GLushort) (flt * 0xffff);
      }
   }
   else if (rb->DataType == GL_UNSIGNED_INT) {
      GLuint *zValues = (GLuint *) values;
      /*
      const GLdouble scale = (GLdouble) 0xffffffff;
      */
      for (i = 0; i < count; i++) {
         GLfloat flt;
         trb->TexImage->FetchTexelf(trb->TexImage, x + i, y, z, &flt);
#if 0
         /* this should work, but doesn't (overflow due to low precision) */
         zValues[i] = (GLuint) (flt * scale);
#else
         /* temporary hack */
         zValues[i] = ((GLuint) (flt * 0xffffff)) << 8;
#endif
      }
   }
   else if (rb->DataType == GL_UNSIGNED_INT_24_8_EXT) {
      GLuint *zValues = (GLuint *) values;
      for (i = 0; i < count; i++) {
         GLfloat flt;
         trb->TexImage->FetchTexelf(trb->TexImage, x + i, y, z, &flt);
         zValues[i] = ((GLuint) (flt * 0xffffff)) << 8;
      }
   }
   else {
      _mesa_problem(ctx, "invalid rb->DataType in texture_get_row");
   }
}


static void
texture_get_values(GLcontext *ctx, struct gl_renderbuffer *rb, GLuint count,
                   const GLint x[], const GLint y[], void *values)
{
   const struct texture_renderbuffer *trb
      = (const struct texture_renderbuffer *) rb;
   const GLint z = trb->Zoffset;
   GLuint i;

   if (rb->DataType == CHAN_TYPE) {
      GLchan *rgbaOut = (GLchan *) values;
      for (i = 0; i < count; i++) {
         trb->TexImage->FetchTexelc(trb->TexImage, x[i], y[i] + trb->Yoffset,
				    z, rgbaOut + 4 * i);
      }
   }
   else if (rb->DataType == GL_UNSIGNED_SHORT) {
      GLushort *zValues = (GLushort *) values;
      for (i = 0; i < count; i++) {
         GLfloat flt;
         trb->TexImage->FetchTexelf(trb->TexImage, x[i], y[i] + trb->Yoffset,
				    z, &flt);
         zValues[i] = (GLushort) (flt * 0xffff);
      }
   }
   else if (rb->DataType == GL_UNSIGNED_INT) {
      GLuint *zValues = (GLuint *) values;
      for (i = 0; i < count; i++) {
         GLfloat flt;
         trb->TexImage->FetchTexelf(trb->TexImage, x[i], y[i] + trb->Yoffset,
				    z, &flt);
#if 0
         zValues[i] = (GLuint) (flt * 0xffffffff);
#else
         zValues[i] = ((GLuint) (flt * 0xffffff)) << 8;
#endif
      }
   }
   else if (rb->DataType == GL_UNSIGNED_INT_24_8_EXT) {
      GLuint *zValues = (GLuint *) values;
      for (i = 0; i < count; i++) {
         GLfloat flt;
         trb->TexImage->FetchTexelf(trb->TexImage, x[i], y[i] + trb->Yoffset,
				    z, &flt);
         zValues[i] = ((GLuint) (flt * 0xffffff)) << 8;
      }
   }
   else {
      _mesa_problem(ctx, "invalid rb->DataType in texture_get_values");
   }
}


/**
 * Put row of values into a renderbuffer that wraps a texture image.
 */
static void
texture_put_row(GLcontext *ctx, struct gl_renderbuffer *rb, GLuint count,
                GLint x, GLint y, const void *values, const GLubyte *mask)
{
   const struct texture_renderbuffer *trb
      = (const struct texture_renderbuffer *) rb;
   const GLint z = trb->Zoffset;
   GLuint i;

   y += trb->Yoffset;

   if (rb->DataType == CHAN_TYPE) {
      const GLchan *rgba = (const GLchan *) values;
      for (i = 0; i < count; i++) {
         if (!mask || mask[i]) {
            trb->Store(trb->TexImage, x + i, y, z, rgba);
         }
         rgba += 4;
      }
   }
   else if (rb->DataType == GL_UNSIGNED_SHORT) {
      const GLushort *zValues = (const GLushort *) values;
      for (i = 0; i < count; i++) {
         if (!mask || mask[i]) {
            trb->Store(trb->TexImage, x + i, y, z, zValues + i);
         }
      }
   }
   else if (rb->DataType == GL_UNSIGNED_INT) {
      const GLuint *zValues = (const GLuint *) values;
      for (i = 0; i < count; i++) {
         if (!mask || mask[i]) {
            trb->Store(trb->TexImage, x + i, y, z, zValues + i);
         }
      }
   }
   else if (rb->DataType == GL_UNSIGNED_INT_24_8_EXT) {
      const GLuint *zValues = (const GLuint *) values;
      for (i = 0; i < count; i++) {
         if (!mask || mask[i]) {
            GLfloat flt = (GLfloat) ((zValues[i] >> 8) * (1.0 / 0xffffff));
            trb->Store(trb->TexImage, x + i, y, z, &flt);
         }
      }
   }
   else {
      _mesa_problem(ctx, "invalid rb->DataType in texture_put_row");
   }
}

/**
 * Put row of RGB values into a renderbuffer that wraps a texture image.
 */
static void
texture_put_row_rgb(GLcontext *ctx, struct gl_renderbuffer *rb, GLuint count,
                GLint x, GLint y, const void *values, const GLubyte *mask)
{
   const struct texture_renderbuffer *trb
      = (const struct texture_renderbuffer *) rb;
   const GLint z = trb->Zoffset;
   GLuint i;

   y += trb->Yoffset;

   if (rb->DataType == CHAN_TYPE) {
      const GLchan *rgb = (const GLchan *) values;
      for (i = 0; i < count; i++) {
         if (!mask || mask[i]) {
            trb->Store(trb->TexImage, x + i, y, z, rgb);
         }
         rgb += 3;
      }
   }
   else if (rb->DataType == GL_UNSIGNED_SHORT) {
      const GLushort *zValues = (const GLushort *) values;
      for (i = 0; i < count; i++) {
         if (!mask || mask[i]) {
            trb->Store(trb->TexImage, x + i, y, z, zValues + i);
         }
      }
   }
   else if (rb->DataType == GL_UNSIGNED_INT) {
      const GLuint *zValues = (const GLuint *) values;
      for (i = 0; i < count; i++) {
         if (!mask || mask[i]) {
            trb->Store(trb->TexImage, x + i, y, z, zValues + i);
         }
      }
   }
   else if (rb->DataType == GL_UNSIGNED_INT_24_8_EXT) {
      const GLuint *zValues = (const GLuint *) values;
      for (i = 0; i < count; i++) {
         if (!mask || mask[i]) {
            GLfloat flt = (GLfloat) ((zValues[i] >> 8) * (1.0 / 0xffffff));
            trb->Store(trb->TexImage, x + i, y, z, &flt);
         }
      }
   }
   else {
      _mesa_problem(ctx, "invalid rb->DataType in texture_put_row");
   }
}


static void
texture_put_mono_row(GLcontext *ctx, struct gl_renderbuffer *rb, GLuint count,
                     GLint x, GLint y, const void *value, const GLubyte *mask)
{
   const struct texture_renderbuffer *trb
      = (const struct texture_renderbuffer *) rb;
   const GLint z = trb->Zoffset;
   GLuint i;

   y += trb->Yoffset;

   if (rb->DataType == CHAN_TYPE) {
      const GLchan *rgba = (const GLchan *) value;
      for (i = 0; i < count; i++) {
         if (!mask || mask[i]) {
            trb->Store(trb->TexImage, x + i, y, z, rgba);
         }
      }
   }
   else if (rb->DataType == GL_UNSIGNED_SHORT) {
      const GLushort zValue = *((const GLushort *) value);
      for (i = 0; i < count; i++) {
         if (!mask || mask[i]) {
            trb->Store(trb->TexImage, x + i, y, z, &zValue);
         }
      }
   }
   else if (rb->DataType == GL_UNSIGNED_INT) {
      const GLuint zValue = *((const GLuint *) value);
      for (i = 0; i < count; i++) {
         if (!mask || mask[i]) {
            trb->Store(trb->TexImage, x + i, y, z, &zValue);
         }
      }
   }
   else if (rb->DataType == GL_UNSIGNED_INT_24_8_EXT) {
      const GLuint zValue = *((const GLuint *) value);
      const GLfloat flt = (GLfloat) ((zValue >> 8) * (1.0 / 0xffffff));
      for (i = 0; i < count; i++) {
         if (!mask || mask[i]) {
            trb->Store(trb->TexImage, x + i, y, z, &flt);
         }
      }
   }
   else {
      _mesa_problem(ctx, "invalid rb->DataType in texture_put_mono_row");
   }
}


static void
texture_put_values(GLcontext *ctx, struct gl_renderbuffer *rb, GLuint count,
                   const GLint x[], const GLint y[], const void *values,
                   const GLubyte *mask)
{
   const struct texture_renderbuffer *trb
      = (const struct texture_renderbuffer *) rb;
   const GLint z = trb->Zoffset;
   GLuint i;

   if (rb->DataType == CHAN_TYPE) {
      const GLchan *rgba = (const GLchan *) values;
      for (i = 0; i < count; i++) {
         if (!mask || mask[i]) {
            trb->Store(trb->TexImage, x[i], y[i] + trb->Yoffset, z, rgba);
         }
         rgba += 4;
      }
   }
   else if (rb->DataType == GL_UNSIGNED_SHORT) {
      const GLushort *zValues = (const GLushort *) values;
      for (i = 0; i < count; i++) {
         if (!mask || mask[i]) {
            trb->Store(trb->TexImage, x[i], y[i] + trb->Yoffset, z, zValues + i);
         }
      }
   }
   else if (rb->DataType == GL_UNSIGNED_INT) {
      const GLuint *zValues = (const GLuint *) values;
      for (i = 0; i < count; i++) {
         if (!mask || mask[i]) {
            trb->Store(trb->TexImage, x[i], y[i] + trb->Yoffset, z, zValues + i);
         }
      }
   }
   else if (rb->DataType == GL_UNSIGNED_INT_24_8_EXT) {
      const GLuint *zValues = (const GLuint *) values;
      for (i = 0; i < count; i++) {
         if (!mask || mask[i]) {
            GLfloat flt = (GLfloat) ((zValues[i] >> 8) * (1.0 / 0xffffff));
            trb->Store(trb->TexImage, x[i], y[i] + trb->Yoffset, z, &flt);
         }
      }
   }
   else {
      _mesa_problem(ctx, "invalid rb->DataType in texture_put_values");
   }
}


static void
texture_put_mono_values(GLcontext *ctx, struct gl_renderbuffer *rb,
                        GLuint count, const GLint x[], const GLint y[],
                        const void *value, const GLubyte *mask)
{
   const struct texture_renderbuffer *trb
      = (const struct texture_renderbuffer *) rb;
   const GLint z = trb->Zoffset;
   GLuint i;

   if (rb->DataType == CHAN_TYPE) {
      const GLchan *rgba = (const GLchan *) value;
      for (i = 0; i < count; i++) {
         if (!mask || mask[i]) {
            trb->Store(trb->TexImage, x[i], y[i] + trb->Yoffset, z, rgba);
         }
      }
   }
   else if (rb->DataType == GL_UNSIGNED_INT) {
      const GLuint zValue = *((const GLuint *) value);
      for (i = 0; i < count; i++) {
         if (!mask || mask[i]) {
            trb->Store(trb->TexImage, x[i], y[i] + trb->Yoffset, z, &zValue);
         }
      }
   }
   else if (rb->DataType == GL_UNSIGNED_SHORT) {
      const GLushort zValue = *((const GLushort *) value);
      for (i = 0; i < count; i++) {
         if (!mask || mask[i]) {
            trb->Store(trb->TexImage, x[i], y[i] + trb->Yoffset, z, &zValue);
         }
      }
   }
   else if (rb->DataType == GL_UNSIGNED_INT_24_8_EXT) {
      const GLuint zValue = *((const GLuint *) value);
      const GLfloat flt = (GLfloat) ((zValue >> 8) * (1.0 / 0xffffff));
      for (i = 0; i < count; i++) {
         if (!mask || mask[i]) {
            trb->Store(trb->TexImage, x[i], y[i] + trb->Yoffset, z, &flt);
         }
      }
   }
   else {
      _mesa_problem(ctx, "invalid rb->DataType in texture_put_mono_values");
   }
}


static void
delete_texture_wrapper(struct gl_renderbuffer *rb)
{
   ASSERT(rb->RefCount == 0);
   _mesa_free(rb);
}


/**
 * This function creates a renderbuffer object which wraps a texture image.
 * The new renderbuffer is plugged into the given attachment point.
 * This allows rendering into the texture as if it were a renderbuffer.
 */
static void
wrap_texture(GLcontext *ctx, struct gl_renderbuffer_attachment *att)
{
   struct texture_renderbuffer *trb;
   const GLuint name = 0;

   ASSERT(att->Type == GL_TEXTURE);
   ASSERT(att->Renderbuffer == NULL);

   trb = CALLOC_STRUCT(texture_renderbuffer);
   if (!trb) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "wrap_texture");
      return;
   }

   /* init base gl_renderbuffer fields */
   _mesa_init_renderbuffer(&trb->Base, name);
   /* plug in our texture_renderbuffer-specific functions */
   trb->Base.Delete = delete_texture_wrapper;
   trb->Base.AllocStorage = NULL; /* illegal! */
   trb->Base.GetRow = texture_get_row;
   trb->Base.GetValues = texture_get_values;
   trb->Base.PutRow = texture_put_row;
   trb->Base.PutRowRGB = texture_put_row_rgb;
   trb->Base.PutMonoRow = texture_put_mono_row;
   trb->Base.PutValues = texture_put_values;
   trb->Base.PutMonoValues = texture_put_mono_values;

   /* update attachment point */
   _mesa_reference_renderbuffer(&att->Renderbuffer, &(trb->Base));
}



/**
 * Update the renderbuffer wrapper for rendering to a texture.
 * For example, update the width, height of the RB based on the texture size,
 * update the internal format info, etc.
 */
static void
update_wrapper(GLcontext *ctx, const struct gl_renderbuffer_attachment *att)
{
   struct texture_renderbuffer *trb
      = (struct texture_renderbuffer *) att->Renderbuffer;

   (void) ctx;
   ASSERT(trb);

   trb->TexImage = att->Texture->Image[att->CubeMapFace][att->TextureLevel];
   ASSERT(trb->TexImage);

   trb->Store = trb->TexImage->TexFormat->StoreTexel;
   ASSERT(trb->Store);

   if (att->Texture->Target == GL_TEXTURE_1D_ARRAY_EXT) {
      trb->Yoffset = att->Zoffset;
      trb->Zoffset = 0;
   }
   else {
      trb->Yoffset = 0;
      trb->Zoffset = att->Zoffset;
   }

   trb->Base.Width = trb->TexImage->Width;
   trb->Base.Height = trb->TexImage->Height;
   trb->Base.InternalFormat = trb->TexImage->InternalFormat;
   /* XXX may need more special cases here */
   if (trb->TexImage->TexFormat->MesaFormat == MESA_FORMAT_Z24_S8) {
      trb->Base._ActualFormat = GL_DEPTH24_STENCIL8_EXT;
      trb->Base.DataType = GL_UNSIGNED_INT_24_8_EXT;
   }
   else if (trb->TexImage->TexFormat->MesaFormat == MESA_FORMAT_Z16) {
      trb->Base._ActualFormat = GL_DEPTH_COMPONENT;
      trb->Base.DataType = GL_UNSIGNED_SHORT;
   }
   else if (trb->TexImage->TexFormat->MesaFormat == MESA_FORMAT_Z32) {
      trb->Base._ActualFormat = GL_DEPTH_COMPONENT;
      trb->Base.DataType = GL_UNSIGNED_INT;
   }
   else {
      trb->Base._ActualFormat = trb->TexImage->InternalFormat;
      trb->Base.DataType = CHAN_TYPE;
   }
   trb->Base._BaseFormat = trb->TexImage->TexFormat->BaseFormat;
#if 0
   /* fix/avoid this assertion someday */
   ASSERT(trb->Base._BaseFormat == GL_RGB ||
          trb->Base._BaseFormat == GL_RGBA ||
          trb->Base._BaseFormat == GL_DEPTH_COMPONENT);
#endif
   trb->Base.Data = trb->TexImage->Data;

   trb->Base.RedBits = trb->TexImage->TexFormat->RedBits;
   trb->Base.GreenBits = trb->TexImage->TexFormat->GreenBits;
   trb->Base.BlueBits = trb->TexImage->TexFormat->BlueBits;
   trb->Base.AlphaBits = trb->TexImage->TexFormat->AlphaBits;
   trb->Base.DepthBits = trb->TexImage->TexFormat->DepthBits;
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
_mesa_render_texture(GLcontext *ctx,
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
_mesa_finish_render_texture(GLcontext *ctx,
                            struct gl_renderbuffer_attachment *att)
{
   /* do nothing */
   /* The renderbuffer texture wrapper will get deleted by the
    * normal mechanism for deleting renderbuffers.
    */
   (void) ctx;
   (void) att;
}
