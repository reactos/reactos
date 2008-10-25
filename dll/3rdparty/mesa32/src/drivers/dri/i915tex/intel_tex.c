#include "texobj.h"
#include "intel_context.h"
#include "intel_mipmap_tree.h"
#include "intel_tex.h"

#define FILE_DEBUG_FLAG DEBUG_TEXTURE

static GLboolean
intelIsTextureResident(GLcontext * ctx, struct gl_texture_object *texObj)
{
#if 0
   struct intel_context *intel = intel_context(ctx);
   struct intel_texture_object *intelObj = intel_texture_object(texObj);

   return
      intelObj->mt &&
      intelObj->mt->region &&
      intel_is_region_resident(intel, intelObj->mt->region);
#endif
   return 1;
}



static struct gl_texture_image *
intelNewTextureImage(GLcontext * ctx)
{
   DBG("%s\n", __FUNCTION__);
   (void) ctx;
   return (struct gl_texture_image *) CALLOC_STRUCT(intel_texture_image);
}


static struct gl_texture_object *
intelNewTextureObject(GLcontext * ctx, GLuint name, GLenum target)
{
   struct intel_texture_object *obj = CALLOC_STRUCT(intel_texture_object);

   DBG("%s\n", __FUNCTION__);
   _mesa_initialize_texture_object(&obj->base, name, target);

   return &obj->base;
}

static void 
intelDeleteTextureObject(GLcontext *ctx,
			 struct gl_texture_object *texObj)
{
   struct intel_context *intel = intel_context(ctx);
   struct intel_texture_object *intelObj = intel_texture_object(texObj);

   if (intelObj->mt)
      intel_miptree_release(intel, &intelObj->mt);

   _mesa_delete_texture_object(ctx, texObj);
}


static void
intelFreeTextureImageData(GLcontext * ctx, struct gl_texture_image *texImage)
{
   struct intel_context *intel = intel_context(ctx);
   struct intel_texture_image *intelImage = intel_texture_image(texImage);

   DBG("%s\n", __FUNCTION__);

   if (intelImage->mt) {
      intel_miptree_release(intel, &intelImage->mt);
   }

   if (texImage->Data) {
      free(texImage->Data);
      texImage->Data = NULL;
   }
}


/* The system memcpy (at least on ubuntu 5.10) has problems copying
 * to agp (writecombined) memory from a source which isn't 64-byte
 * aligned - there is a 4x performance falloff.
 *
 * The x86 __memcpy is immune to this but is slightly slower
 * (10%-ish) than the system memcpy.
 *
 * The sse_memcpy seems to have a slight cliff at 64/32 bytes, but
 * isn't much faster than x86_memcpy for agp copies.
 * 
 * TODO: switch dynamically.
 */
static void *
do_memcpy(void *dest, const void *src, size_t n)
{
   if ((((unsigned) src) & 63) || (((unsigned) dest) & 63)) {
      return __memcpy(dest, src, n);
   }
   else
      return memcpy(dest, src, n);
}


#if DO_DEBUG

#ifndef __x86_64__
static unsigned
fastrdtsc(void)
{
   unsigned eax;
   __asm__ volatile ("\t"
                     "pushl  %%ebx\n\t"
                     "cpuid\n\t" ".byte 0x0f, 0x31\n\t"
                     "popl %%ebx\n":"=a" (eax)
                     :"0"(0)
                     :"ecx", "edx", "cc");

   return eax;
}
#else
static unsigned
fastrdtsc(void)
{
   unsigned eax;
   __asm__ volatile ("\t" "cpuid\n\t" ".byte 0x0f, 0x31\n\t":"=a" (eax)
                     :"0"(0)
                     :"ecx", "edx", "ebx", "cc");

   return eax;
}
#endif

static unsigned
time_diff(unsigned t, unsigned t2)
{
   return ((t < t2) ? t2 - t : 0xFFFFFFFFU - (t - t2 - 1));
}


static void *
timed_memcpy(void *dest, const void *src, size_t n)
{
   void *ret;
   unsigned t1, t2;
   double rate;

   if ((((unsigned) src) & 63) || (((unsigned) dest) & 63))
      _mesa_printf("Warning - non-aligned texture copy!\n");

   t1 = fastrdtsc();
   ret = do_memcpy(dest, src, n);
   t2 = fastrdtsc();

   rate = time_diff(t1, t2);
   rate /= (double) n;
   _mesa_printf("timed_memcpy: %u %u --> %f clocks/byte\n", t1, t2, rate);
   return ret;
}
#endif /* DO_DEBUG */


void
intelInitTextureFuncs(struct dd_function_table *functions)
{
   functions->ChooseTextureFormat = intelChooseTextureFormat;
   functions->TexImage1D = intelTexImage1D;
   functions->TexImage2D = intelTexImage2D;
   functions->TexImage3D = intelTexImage3D;
   functions->TexSubImage1D = intelTexSubImage1D;
   functions->TexSubImage2D = intelTexSubImage2D;
   functions->TexSubImage3D = intelTexSubImage3D;
   functions->CopyTexImage1D = intelCopyTexImage1D;
   functions->CopyTexImage2D = intelCopyTexImage2D;
   functions->CopyTexSubImage1D = intelCopyTexSubImage1D;
   functions->CopyTexSubImage2D = intelCopyTexSubImage2D;
   functions->GetTexImage = intelGetTexImage;

   /* compressed texture functions */
   functions->CompressedTexImage2D = intelCompressedTexImage2D;
   functions->GetCompressedTexImage = intelGetCompressedTexImage;

   functions->NewTextureObject = intelNewTextureObject;
   functions->NewTextureImage = intelNewTextureImage;
   functions->DeleteTexture = intelDeleteTextureObject;
   functions->FreeTexImageData = intelFreeTextureImageData;
   functions->UpdateTexturePalette = 0;
   functions->IsTextureResident = intelIsTextureResident;

#if DO_DEBUG
   if (INTEL_DEBUG & DEBUG_BUFMGR)
      functions->TextureMemCpy = timed_memcpy;
   else
#endif
      functions->TextureMemCpy = do_memcpy;
}
