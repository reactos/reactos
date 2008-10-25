/*
 * Copyright 1998-2003 VIA Technologies, Inc. All Rights Reserved.
 * Copyright 2001-2003 S3 Graphics, Inc. All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sub license,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * VIA, S3 GRAPHICS, AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */


#include <stdlib.h>
#include <stdio.h>

#include "glheader.h"
#include "macros.h"
#include "mtypes.h"
#include "enums.h"
#include "colortab.h"
#include "convolve.h"
#include "context.h"
#include "mipmap.h"
#include "simple_list.h"
#include "texcompress.h"
#include "texformat.h"
#include "texobj.h"
#include "texstore.h"

#include "mm.h"
#include "via_context.h"
#include "via_fb.h"
#include "via_tex.h"
#include "via_state.h"
#include "via_ioctl.h"
#include "via_3d_reg.h"

static const struct gl_texture_format *
viaChooseTexFormat( GLcontext *ctx, GLint internalFormat,
		    GLenum format, GLenum type )
{
   struct via_context *vmesa = VIA_CONTEXT(ctx);
   const GLboolean do32bpt = ( vmesa->viaScreen->bitsPerPixel == 32
/* 			       && vmesa->viaScreen->textureSize > 4*1024*1024 */
      );


   switch ( internalFormat ) {
   case 4:
   case GL_RGBA:
   case GL_COMPRESSED_RGBA:
      if ( format == GL_BGRA ) {
	 if ( type == GL_UNSIGNED_INT_8_8_8_8_REV ||
	      type == GL_UNSIGNED_BYTE ) {
	    return &_mesa_texformat_argb8888;
	 }
         else if ( type == GL_UNSIGNED_SHORT_4_4_4_4_REV ) {
            return &_mesa_texformat_argb4444;
	 }
         else if ( type == GL_UNSIGNED_SHORT_1_5_5_5_REV ) {
	    return &_mesa_texformat_argb1555;
	 }
      }
      else if ( type == GL_UNSIGNED_BYTE ||
		type == GL_UNSIGNED_INT_8_8_8_8_REV ||
		type == GL_UNSIGNED_INT_8_8_8_8 ) {
	 return &_mesa_texformat_argb8888;
      }
      return do32bpt ? &_mesa_texformat_argb8888 : &_mesa_texformat_argb4444;

   case 3:
   case GL_RGB:
   case GL_COMPRESSED_RGB:
      if ( format == GL_RGB && type == GL_UNSIGNED_SHORT_5_6_5 ) {
	 return &_mesa_texformat_rgb565;
      }
      else if ( type == GL_UNSIGNED_BYTE ) {
	 return &_mesa_texformat_argb8888;
      }
      return do32bpt ? &_mesa_texformat_argb8888 : &_mesa_texformat_rgb565;

   case GL_RGBA8:
   case GL_RGB10_A2:
   case GL_RGBA12:
   case GL_RGBA16:
      return &_mesa_texformat_argb8888;

   case GL_RGBA4:
   case GL_RGBA2:
      return &_mesa_texformat_argb4444;

   case GL_RGB5_A1:
      return &_mesa_texformat_argb1555;

   case GL_RGB8:
   case GL_RGB10:
   case GL_RGB12:
   case GL_RGB16:
      return &_mesa_texformat_argb8888;

   case GL_RGB5:
   case GL_RGB4:
   case GL_R3_G3_B2:
      return &_mesa_texformat_rgb565;

   case GL_ALPHA:
   case GL_ALPHA4:
   case GL_ALPHA8:
   case GL_ALPHA12:
   case GL_ALPHA16:
   case GL_COMPRESSED_ALPHA:
      return &_mesa_texformat_a8;

   case 1:
   case GL_LUMINANCE:
   case GL_LUMINANCE4:
   case GL_LUMINANCE8:
   case GL_LUMINANCE12:
   case GL_LUMINANCE16:
   case GL_COMPRESSED_LUMINANCE:
      return &_mesa_texformat_l8;

   case 2:
   case GL_LUMINANCE_ALPHA:
   case GL_LUMINANCE4_ALPHA4:
   case GL_LUMINANCE6_ALPHA2:
   case GL_LUMINANCE8_ALPHA8:
   case GL_LUMINANCE12_ALPHA4:
   case GL_LUMINANCE12_ALPHA12:
   case GL_LUMINANCE16_ALPHA16:
   case GL_COMPRESSED_LUMINANCE_ALPHA:
      return &_mesa_texformat_al88;

   case GL_INTENSITY:
   case GL_INTENSITY4:
   case GL_INTENSITY8:
   case GL_INTENSITY12:
   case GL_INTENSITY16:
   case GL_COMPRESSED_INTENSITY:
      return &_mesa_texformat_i8;

   case GL_YCBCR_MESA:
      if (type == GL_UNSIGNED_SHORT_8_8_MESA ||
	  type == GL_UNSIGNED_BYTE)
         return &_mesa_texformat_ycbcr;
      else
         return &_mesa_texformat_ycbcr_rev;

   case GL_COMPRESSED_RGB_FXT1_3DFX:
      return &_mesa_texformat_rgb_fxt1;
   case GL_COMPRESSED_RGBA_FXT1_3DFX:
      return &_mesa_texformat_rgba_fxt1;

   case GL_RGB_S3TC:
   case GL_RGB4_S3TC:
   case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:
      return &_mesa_texformat_rgb_dxt1;

   case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
      return &_mesa_texformat_rgba_dxt1;

   case GL_RGBA_S3TC:
   case GL_RGBA4_S3TC:
   case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
      return &_mesa_texformat_rgba_dxt3;

   case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
      return &_mesa_texformat_rgba_dxt5;

   case GL_COLOR_INDEX:	
   case GL_COLOR_INDEX1_EXT:	
   case GL_COLOR_INDEX2_EXT:	
   case GL_COLOR_INDEX4_EXT:	
   case GL_COLOR_INDEX8_EXT:	
   case GL_COLOR_INDEX12_EXT:	    
   case GL_COLOR_INDEX16_EXT:
      return &_mesa_texformat_ci8;    

   default:
      fprintf(stderr, "unexpected texture format %s in %s\n", 
	      _mesa_lookup_enum_by_nr(internalFormat),
	      __FUNCTION__);
      return NULL;
   }

   return NULL; /* never get here */
}

static int logbase2(int n)
{
   GLint i = 1;
   GLint log2 = 0;

   while (n > i) {
      i *= 2;
      log2++;
   }

   return log2;
}

static const char *get_memtype_name( GLint memType )
{
   static const char *names[] = {
      "VIA_MEM_VIDEO",
      "VIA_MEM_AGP",
      "VIA_MEM_SYSTEM",
      "VIA_MEM_MIXED",
      "VIA_MEM_UNKNOWN"
   };

   return names[memType];
}


static GLboolean viaMoveTexBuffers( struct via_context *vmesa,
				    struct via_tex_buffer **buffers,
				    GLuint nr,
				    GLint newMemType )
{
   struct via_tex_buffer *newTexBuf[VIA_MAX_TEXLEVELS];
   GLint i;

   if (VIA_DEBUG & DEBUG_TEXTURE)
      fprintf(stderr, "%s to %s\n",
	      __FUNCTION__,
	      get_memtype_name(newMemType));

   memset(newTexBuf, 0, sizeof(newTexBuf));

   /* First do all the allocations (or fail):
    */ 
   for (i = 0; i < nr; i++) {    
      if (buffers[i]->memType != newMemType) {	 

	 /* Don't allow uploads in a thrash state.  Should try and
	  * catch this earlier.
	  */
	 if (vmesa->thrashing && newMemType != VIA_MEM_SYSTEM)
	    goto cleanup;

	 newTexBuf[i] = via_alloc_texture(vmesa, 
					  buffers[i]->size,
					  newMemType);
	 if (!newTexBuf[i]) 
	    goto cleanup;
      }
   }


   /* Now copy all the image data and free the old texture memory.
    */
   for (i = 0; i < nr; i++) {    
      if (newTexBuf[i]) {
	 memcpy(newTexBuf[i]->bufAddr,
		buffers[i]->bufAddr, 
		buffers[i]->size);

	 newTexBuf[i]->image = buffers[i]->image;
	 newTexBuf[i]->image->texMem = newTexBuf[i];
	 newTexBuf[i]->image->image.Data = newTexBuf[i]->bufAddr;
	 via_free_texture(vmesa, buffers[i]);
      }
   }

   if (VIA_DEBUG & DEBUG_TEXTURE)
      fprintf(stderr, "%s - success\n", __FUNCTION__);

   return GL_TRUE;

 cleanup:
   /* Release any allocations made prior to failure:
    */
   if (VIA_DEBUG & DEBUG_TEXTURE)
      fprintf(stderr, "%s - failed\n", __FUNCTION__);

   for (i = 0; i < nr; i++) {    
      if (newTexBuf[i]) {
	 via_free_texture(vmesa, newTexBuf[i]);
      }
   }
   
   return GL_FALSE;   
}


static GLboolean viaMoveTexObject( struct via_context *vmesa,
				   struct via_texture_object *viaObj,
				   GLint newMemType )
{   
   struct via_texture_image **viaImage = 
      (struct via_texture_image **)&viaObj->obj.Image[0][0];
   struct via_tex_buffer *buffers[VIA_MAX_TEXLEVELS];
   GLuint i, nr = 0;

   for (i = viaObj->firstLevel; i <= viaObj->lastLevel; i++)
      buffers[nr++] = viaImage[i]->texMem;

   if (viaMoveTexBuffers( vmesa, &buffers[0], nr, newMemType )) {
      viaObj->memType = newMemType;
      return GL_TRUE;
   }

   return GL_FALSE;
}



static GLboolean viaSwapInTexObject( struct via_context *vmesa,
				     struct via_texture_object *viaObj )
{
   const struct via_texture_image *baseImage = 
      (struct via_texture_image *)viaObj->obj.Image[0][viaObj->obj.BaseLevel]; 

   if (VIA_DEBUG & DEBUG_TEXTURE)
      fprintf(stderr, "%s\n", __FUNCTION__);

   if (baseImage->texMem->memType != VIA_MEM_SYSTEM) 
      return viaMoveTexObject( vmesa, viaObj, baseImage->texMem->memType );

   return (viaMoveTexObject( vmesa, viaObj, VIA_MEM_AGP ) ||
	   viaMoveTexObject( vmesa, viaObj, VIA_MEM_VIDEO ));
}


/* This seems crude, but it asks a fairly pertinent question and gives
 * an accurate answer:
 */
static GLboolean viaIsTexMemLow( struct via_context *vmesa,
				 GLuint heap )
{
   struct via_tex_buffer *buf =  via_alloc_texture(vmesa, 512 * 1024, heap );
   if (!buf)
      return GL_TRUE;
   
   via_free_texture(vmesa, buf);
   return GL_FALSE;
}


/* Speculatively move texture images which haven't been used in a
 * while back to system memory. 
 * 
 * TODO: only do this when texture memory is low.
 * 
 * TODO: use dma.
 *
 * TODO: keep the fb/agp version hanging around and use the local
 * version as backing store, so re-upload might be avoided.
 *
 * TODO: do this properly in the kernel...
 */
GLboolean viaSwapOutWork( struct via_context *vmesa )
{
   struct via_tex_buffer *s, *tmp;
   GLuint done = 0;
   GLuint heap, target;

   if (VIA_DEBUG & DEBUG_TEXTURE)
      fprintf(stderr, "%s VID %d AGP %d SYS %d\n", __FUNCTION__,
	      vmesa->total_alloc[VIA_MEM_VIDEO],
	      vmesa->total_alloc[VIA_MEM_AGP],
	      vmesa->total_alloc[VIA_MEM_SYSTEM]);

   
   for (heap = VIA_MEM_VIDEO; heap <= VIA_MEM_AGP; heap++) {
      GLuint nr = 0, sz = 0;

      if (vmesa->thrashing) {
 	 if (VIA_DEBUG & DEBUG_TEXTURE)
	    fprintf(stderr, "Heap %d: trash flag\n", heap);
	 target = 1*1024*1024;
      }
      else if (viaIsTexMemLow(vmesa, heap)) {
 	 if (VIA_DEBUG & DEBUG_TEXTURE)
	    fprintf(stderr, "Heap %d: low memory\n", heap);
	 target = 64*1024;
      }
      else {
 	 if (VIA_DEBUG & DEBUG_TEXTURE)
	    fprintf(stderr, "Heap %d: nothing to do\n", heap);
	 continue;
      }

      foreach_s( s, tmp, &vmesa->tex_image_list[heap] ) {
	 if (s->lastUsed < vmesa->lastSwap[1]) {
	    struct via_texture_object *viaObj = 
	       (struct via_texture_object *) s->image->image.TexObject;

	    if (VIA_DEBUG & DEBUG_TEXTURE)
	       fprintf(stderr, 
		       "back copy tex sz %d, lastUsed %d lastSwap %d\n", 
		       s->size, s->lastUsed, vmesa->lastSwap[1]);

	    if (viaMoveTexBuffers( vmesa, &s, 1, VIA_MEM_SYSTEM )) {
	       viaObj->memType = VIA_MEM_MIXED;
	       done += s->size;
	    }
	    else {
	       if (VIA_DEBUG & DEBUG_TEXTURE)
		  fprintf(stderr, "Failed to back copy texture!\n");
	       sz += s->size;
	    }
	 }
	 else {
	    nr ++;
	    sz += s->size;
	 }

	 if (done > target) {
	    vmesa->thrashing = GL_FALSE; /* might not get set otherwise? */
	    return GL_TRUE;
	 }
      }

      assert(sz == vmesa->total_alloc[heap]);
	 
      if (VIA_DEBUG & DEBUG_TEXTURE)
	 fprintf(stderr, "Heap %d: nr %d tot sz %d\n", heap, nr, sz);
   }

   
   return done != 0;
}



/* Basically, just collect the image dimensions and addresses for each
 * image and update the texture object state accordingly.
 */
static GLboolean viaSetTexImages(GLcontext *ctx,
				 struct gl_texture_object *texObj)
{
   struct via_context *vmesa = VIA_CONTEXT(ctx);
   struct via_texture_object *viaObj = (struct via_texture_object *)texObj;
   const struct via_texture_image *baseImage = 
      (struct via_texture_image *)texObj->Image[0][texObj->BaseLevel];
   GLint firstLevel, lastLevel, numLevels;
   GLuint texFormat;
   GLint w, h, p;
   GLint i, j = 0, k = 0, l = 0, m = 0;
   GLuint texBase;
   GLuint basH = 0;
   GLuint widthExp = 0;
   GLuint heightExp = 0;    

   switch (baseImage->image.TexFormat->MesaFormat) {
   case MESA_FORMAT_ARGB8888:
      texFormat = HC_HTXnFM_ARGB8888;
      break;
   case MESA_FORMAT_ARGB4444:
      texFormat = HC_HTXnFM_ARGB4444; 
      break;
   case MESA_FORMAT_RGB565:
      texFormat = HC_HTXnFM_RGB565;   
      break;
   case MESA_FORMAT_ARGB1555:
      texFormat = HC_HTXnFM_ARGB1555;   
      break;
   case MESA_FORMAT_RGB888:
      texFormat = HC_HTXnFM_ARGB0888;
      break;
   case MESA_FORMAT_L8:
      texFormat = HC_HTXnFM_L8;       
      break;
   case MESA_FORMAT_I8:
      texFormat = HC_HTXnFM_T8;       
      break;
   case MESA_FORMAT_CI8:
      texFormat = HC_HTXnFM_Index8;   
      break;
   case MESA_FORMAT_AL88:
      texFormat = HC_HTXnFM_AL88;     
      break;
   case MESA_FORMAT_A8:
      texFormat = HC_HTXnFM_A8;     
      break;
   default:
      _mesa_problem(vmesa->glCtx, "Bad texture format in viaSetTexImages");
      return GL_FALSE;
   }

   /* Compute which mipmap levels we really want to send to the hardware.
    * This depends on the base image size, GL_TEXTURE_MIN_LOD,
    * GL_TEXTURE_MAX_LOD, GL_TEXTURE_BASE_LEVEL, and GL_TEXTURE_MAX_LEVEL.
    * Yes, this looks overly complicated, but it's all needed.
    */
   if (texObj->MinFilter == GL_LINEAR || texObj->MinFilter == GL_NEAREST) {
      firstLevel = lastLevel = texObj->BaseLevel;
   }
   else {
      firstLevel = texObj->BaseLevel + (GLint)(texObj->MinLod + 0.5);
      firstLevel = MAX2(firstLevel, texObj->BaseLevel);
      lastLevel = texObj->BaseLevel + (GLint)(texObj->MaxLod + 0.5);
      lastLevel = MAX2(lastLevel, texObj->BaseLevel);
      lastLevel = MIN2(lastLevel, texObj->BaseLevel + baseImage->image.MaxLog2);
      lastLevel = MIN2(lastLevel, texObj->MaxLevel);
      lastLevel = MAX2(firstLevel, lastLevel);     /* need at least one level */
   }

   numLevels = lastLevel - firstLevel + 1;

   /* The hardware supports only 10 mipmap levels; ignore higher levels.
    */
   if ((numLevels > 10) && (ctx->Const.MaxTextureLevels > 10)) {
       lastLevel -= numLevels - 10;
       numLevels = 10;
   }

   /* save these values, check if they effect the residency of the
    * texture:
    */
   if (viaObj->firstLevel != firstLevel ||
       viaObj->lastLevel != lastLevel) {
      viaObj->firstLevel = firstLevel;
      viaObj->lastLevel = lastLevel;
      viaObj->memType = VIA_MEM_MIXED;
   }

   if (VIA_DEBUG & DEBUG_TEXTURE & 0)
      fprintf(stderr, "%s, current memType: %s\n",
	      __FUNCTION__,
	      get_memtype_name(viaObj->memType));

   
   if (viaObj->memType == VIA_MEM_MIXED ||
       viaObj->memType == VIA_MEM_SYSTEM) {
      if (!viaSwapInTexObject(vmesa, viaObj)) {
 	 if (VIA_DEBUG & DEBUG_TEXTURE) 
	    if (!vmesa->thrashing)
	       fprintf(stderr, "Thrashing flag set for frame %d\n", 
		       vmesa->swap_count);
	 vmesa->thrashing = GL_TRUE;
	 return GL_FALSE;
      }
   }

   if (viaObj->memType == VIA_MEM_AGP)
      viaObj->regTexFM = (HC_SubA_HTXnFM << 24) | HC_HTXnLoc_AGP | texFormat;
   else
      viaObj->regTexFM = (HC_SubA_HTXnFM << 24) | HC_HTXnLoc_Local | texFormat;


   for (i = 0; i < numLevels; i++) {    
      struct via_texture_image *viaImage = 
	 (struct via_texture_image *)texObj->Image[0][firstLevel + i];

      w = viaImage->image.WidthLog2;
      h = viaImage->image.HeightLog2;
      p = viaImage->pitchLog2;

      assert(viaImage->texMem->memType == viaObj->memType);

      texBase = viaImage->texMem->texBase;
      if (!texBase) {
	 if (VIA_DEBUG & DEBUG_TEXTURE)
	    fprintf(stderr, "%s: no texBase[%d]\n", __FUNCTION__, i); 
	 return GL_FALSE;
      }

      /* Image has to remain resident until the coming fence is retired.
       */
      move_to_head( &vmesa->tex_image_list[viaImage->texMem->memType],
		    viaImage->texMem );
      viaImage->texMem->lastUsed = vmesa->lastBreadcrumbWrite;


      viaObj->regTexBaseAndPitch[i].baseL = 
	 ((HC_SubA_HTXnL0BasL + i) << 24) | (texBase & 0xFFFFFF);

      viaObj->regTexBaseAndPitch[i].pitchLog2 = 
	 ((HC_SubA_HTXnL0Pit + i) << 24) | (p << 20);
					      
					      
      /* The base high bytes for each 3 levels are packed
       * together into one register:
       */
      j = i / 3;
      k = 3 - (i % 3);
      basH |= ((texBase & 0xFF000000) >> (k << 3));
      if (k == 1) {
	 viaObj->regTexBaseH[j] = ((j + HC_SubA_HTXnL012BasH) << 24) | basH;
	 basH = 0;
      }
            
      /* Likewise, sets of 6 log2width and log2height values are
       * packed into individual registers:
       */
      l = i / 6;
      m = i % 6;
      widthExp |= (((GLuint)w & 0xF) << (m << 2));
      heightExp |= (((GLuint)h & 0xF) << (m << 2));
      if (m == 5) {
	 viaObj->regTexWidthLog2[l] = 
	    (l + HC_SubA_HTXnL0_5WE) << 24 | widthExp;
	 viaObj->regTexHeightLog2[l] = 
	    (l + HC_SubA_HTXnL0_5HE) << 24 | heightExp;
	 widthExp = 0;
	 heightExp = 0;
      }
      if (w) w--;
      if (h) h--;
      if (p) p--;                                           
   }
        
   if (k != 1) {
      viaObj->regTexBaseH[j] = ((j + HC_SubA_HTXnL012BasH) << 24) | basH;      
   }
   if (m != 5) {
      viaObj->regTexWidthLog2[l] = (l + HC_SubA_HTXnL0_5WE) << 24 | widthExp;
      viaObj->regTexHeightLog2[l] = (l + HC_SubA_HTXnL0_5HE) << 24 | heightExp;
   }

   return GL_TRUE;
}


GLboolean viaUpdateTextureState( GLcontext *ctx )
{
   struct gl_texture_unit *texUnit = ctx->Texture.Unit;
   GLuint i;

   for (i = 0; i < 2; i++) {   
      if (texUnit[i]._ReallyEnabled == TEXTURE_2D_BIT || 
	  texUnit[i]._ReallyEnabled == TEXTURE_1D_BIT) {

	 if (!viaSetTexImages(ctx, texUnit[i]._Current)) 
	    return GL_FALSE;
      }
      else if (texUnit[i]._ReallyEnabled) {
	 return GL_FALSE;
      } 
   }
   
   return GL_TRUE;
}





				 


static void viaTexImage(GLcontext *ctx, 
			GLint dims,
			GLenum target, GLint level,
			GLint internalFormat,
			GLint width, GLint height, GLint border,
			GLenum format, GLenum type, const void *pixels,
			const struct gl_pixelstore_attrib *packing,
			struct gl_texture_object *texObj,
			struct gl_texture_image *texImage)
{
   struct via_context *vmesa = VIA_CONTEXT(ctx);
   GLint postConvWidth = width;
   GLint postConvHeight = height;
   GLint texelBytes, sizeInBytes;
   struct via_texture_object *viaObj = (struct via_texture_object *)texObj;
   struct via_texture_image *viaImage = (struct via_texture_image *)texImage;
   int heaps[3], nheaps, i;

   if (!is_empty_list(&vmesa->freed_tex_buffers)) {
      viaCheckBreadcrumb(vmesa, 0);
      via_release_pending_textures(vmesa);
   }

   if (ctx->_ImageTransferState & IMAGE_CONVOLUTION_BIT) {
      _mesa_adjust_image_for_convolution(ctx, dims, &postConvWidth,
                                         &postConvHeight);
   }

   /* choose the texture format */
   texImage->TexFormat = viaChooseTexFormat(ctx, internalFormat, 
					    format, type);

   assert(texImage->TexFormat);

   if (dims == 1) {
      texImage->FetchTexelc = texImage->TexFormat->FetchTexel1D;
      texImage->FetchTexelf = texImage->TexFormat->FetchTexel1Df;
   }
   else {
      texImage->FetchTexelc = texImage->TexFormat->FetchTexel2D;
      texImage->FetchTexelf = texImage->TexFormat->FetchTexel2Df;
   }
   texelBytes = texImage->TexFormat->TexelBytes;

   if (texelBytes == 0) {
      /* compressed format */
      texImage->IsCompressed = GL_TRUE;
      texImage->CompressedSize =
         ctx->Driver.CompressedTextureSize(ctx, texImage->Width,
                                           texImage->Height, texImage->Depth,
                                           texImage->TexFormat->MesaFormat);
   }

   /* Minimum pitch of 32 bytes */
   if (postConvWidth * texelBytes < 32) {
      postConvWidth = 32 / texelBytes;
      texImage->RowStride = postConvWidth;
   }

   assert(texImage->RowStride == postConvWidth);
   viaImage->pitchLog2 = logbase2(postConvWidth * texelBytes);

   /* allocate memory */
   if (texImage->IsCompressed)
      sizeInBytes = texImage->CompressedSize;
   else
      sizeInBytes = postConvWidth * postConvHeight * texelBytes;


   /* Attempt to allocate texture memory directly, otherwise use main
    * memory and this texture will always be a fallback.   FIXME!
    *
    * TODO: make room in agp if this fails.
    * TODO: use fb ram for textures as well.
    */
   
      
   switch (viaObj->memType) {
   case VIA_MEM_UNKNOWN:
      heaps[0] = VIA_MEM_AGP;
      heaps[1] = VIA_MEM_VIDEO;
      heaps[2] = VIA_MEM_SYSTEM;
      nheaps = 3;
      break;
   case VIA_MEM_AGP:
   case VIA_MEM_VIDEO:
      heaps[0] = viaObj->memType;
      heaps[1] = VIA_MEM_SYSTEM;
      nheaps = 2;
      break;
   case VIA_MEM_MIXED:
   case VIA_MEM_SYSTEM:
   default:
      heaps[0] = VIA_MEM_SYSTEM;
      nheaps = 1;
      break;
   }
	
   for (i = 0; i < nheaps && !viaImage->texMem; i++) {
      if (VIA_DEBUG & DEBUG_TEXTURE) 
	 fprintf(stderr, "try %s (obj %s)\n", get_memtype_name(heaps[i]),
		 get_memtype_name(viaObj->memType));
      viaImage->texMem = via_alloc_texture(vmesa, sizeInBytes, heaps[i]);
   }

   if (!viaImage->texMem) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "glTexImage");
      return;
   }

   if (VIA_DEBUG & DEBUG_TEXTURE)
      fprintf(stderr, "upload %d bytes to %s\n", sizeInBytes, 
	      get_memtype_name(viaImage->texMem->memType));

   viaImage->texMem->image = viaImage;
   texImage->Data = viaImage->texMem->bufAddr;

   if (viaObj->memType == VIA_MEM_UNKNOWN)
      viaObj->memType = viaImage->texMem->memType;
   else if (viaObj->memType != viaImage->texMem->memType)
      viaObj->memType = VIA_MEM_MIXED;

   if (VIA_DEBUG & DEBUG_TEXTURE)
      fprintf(stderr, "%s, obj %s, image : %s\n",
	      __FUNCTION__,	      
	      get_memtype_name(viaObj->memType),
	      get_memtype_name(viaImage->texMem->memType));

   vmesa->clearTexCache = 1;

   pixels = _mesa_validate_pbo_teximage(ctx, dims, width, height, 1, 
					format, type,
					pixels, packing, "glTexImage");
   if (!pixels) {
      /* Note: we check for a NULL image pointer here, _after_ we allocated
       * memory for the texture.  That's what the GL spec calls for.
       */
      return;
   }
   else {
      GLint dstRowStride;
      GLboolean success;
      if (texImage->IsCompressed) {
         dstRowStride = _mesa_compressed_row_stride(texImage->TexFormat->MesaFormat, width);
      }
      else {
         dstRowStride = postConvWidth * texImage->TexFormat->TexelBytes;
      }
      ASSERT(texImage->TexFormat->StoreImage);
      success = texImage->TexFormat->StoreImage(ctx, dims,
                                                texImage->_BaseFormat,
                                                texImage->TexFormat,
                                                texImage->Data,
                                                0, 0, 0,  /* dstX/Y/Zoffset */
                                                dstRowStride,
                                                texImage->ImageOffsets,
                                                width, height, 1,
                                                format, type, pixels, packing);
      if (!success) {
         _mesa_error(ctx, GL_OUT_OF_MEMORY, "glTexImage");
      }
   }

   /* GL_SGIS_generate_mipmap */
   if (level == texObj->BaseLevel && texObj->GenerateMipmap) {
      _mesa_generate_mipmap(ctx, target,
                            &ctx->Texture.Unit[ctx->Texture.CurrentUnit],
                            texObj);
   }

   _mesa_unmap_teximage_pbo(ctx, packing);
}

static void viaTexImage2D(GLcontext *ctx, 
			  GLenum target, GLint level,
			  GLint internalFormat,
			  GLint width, GLint height, GLint border,
			  GLenum format, GLenum type, const void *pixels,
			  const struct gl_pixelstore_attrib *packing,
			  struct gl_texture_object *texObj,
			  struct gl_texture_image *texImage)
{
   viaTexImage( ctx, 2, target, level, 
		internalFormat, width, height, border,
		format, type, pixels,
		packing, texObj, texImage );
}

static void viaTexSubImage2D(GLcontext *ctx,
                             GLenum target,
                             GLint level,
                             GLint xoffset, GLint yoffset,
                             GLsizei width, GLsizei height,
                             GLenum format, GLenum type,
                             const GLvoid *pixels,
                             const struct gl_pixelstore_attrib *packing,
                             struct gl_texture_object *texObj,
                             struct gl_texture_image *texImage)
{
   struct via_context *vmesa = VIA_CONTEXT(ctx);
  
   viaWaitIdle(vmesa, GL_TRUE);
   vmesa->clearTexCache = 1;

   _mesa_store_texsubimage2d(ctx, target, level, xoffset, yoffset, width,
			     height, format, type, pixels, packing, texObj,
			     texImage);
}

static void viaTexImage1D(GLcontext *ctx, 
			  GLenum target, GLint level,
			  GLint internalFormat,
			  GLint width, GLint border,
			  GLenum format, GLenum type, const void *pixels,
			  const struct gl_pixelstore_attrib *packing,
			  struct gl_texture_object *texObj,
			  struct gl_texture_image *texImage)
{
   viaTexImage( ctx, 1, target, level, 
		internalFormat, width, 1, border,
		format, type, pixels,
		packing, texObj, texImage );
}

static void viaTexSubImage1D(GLcontext *ctx,
                             GLenum target,
                             GLint level,
                             GLint xoffset,
                             GLsizei width,
                             GLenum format, GLenum type,
                             const GLvoid *pixels,
                             const struct gl_pixelstore_attrib *packing,
                             struct gl_texture_object *texObj,
                             struct gl_texture_image *texImage)
{
   struct via_context *vmesa = VIA_CONTEXT(ctx);

   viaWaitIdle(vmesa, GL_TRUE); 
   vmesa->clearTexCache = 1;

   _mesa_store_texsubimage1d(ctx, target, level, xoffset, width,
			     format, type, pixels, packing, texObj,
			     texImage);
}



static GLboolean viaIsTextureResident(GLcontext *ctx,
                                      struct gl_texture_object *texObj)
{
   struct via_texture_object *viaObj = 
      (struct via_texture_object *)texObj;

   return (viaObj->memType == VIA_MEM_AGP ||
	   viaObj->memType == VIA_MEM_VIDEO);
}



static struct gl_texture_image *viaNewTextureImage( GLcontext *ctx )
{
   (void) ctx;
   return (struct gl_texture_image *)CALLOC_STRUCT(via_texture_image);
}


static struct gl_texture_object *viaNewTextureObject( GLcontext *ctx, 
						      GLuint name, 
						      GLenum target )
{
   struct via_texture_object *obj = CALLOC_STRUCT(via_texture_object);

   _mesa_initialize_texture_object(&obj->obj, name, target);
   (void) ctx;

   obj->memType = VIA_MEM_UNKNOWN;

   return &obj->obj;
}


static void viaFreeTextureImageData( GLcontext *ctx, 
				     struct gl_texture_image *texImage )
{
   struct via_context *vmesa = VIA_CONTEXT(ctx);
   struct via_texture_image *image = (struct via_texture_image *)texImage;

   if (image->texMem) {
      via_free_texture(vmesa, image->texMem);
      image->texMem = NULL;
   }
   
   texImage->Data = NULL;
}




void viaInitTextureFuncs(struct dd_function_table * functions)
{
   functions->ChooseTextureFormat = viaChooseTexFormat;
   functions->TexImage1D = viaTexImage1D;
   functions->TexImage2D = viaTexImage2D;
   functions->TexSubImage1D = viaTexSubImage1D;
   functions->TexSubImage2D = viaTexSubImage2D;

   functions->NewTextureObject = viaNewTextureObject;
   functions->NewTextureImage = viaNewTextureImage;
   functions->DeleteTexture = _mesa_delete_texture_object;
   functions->FreeTexImageData = viaFreeTextureImageData;

#if 0 && defined( USE_SSE_ASM )
   /*
    * XXX this code is disabled for now because the via_sse_memcpy()
    * routine causes segfaults with flightgear.
    * See Mesa3d-dev mail list messages from 7/15/2005 for details.
    * Note that this function is currently disabled in via_tris.c too.
    */
   if (getenv("VIA_NO_SSE"))
      functions->TextureMemCpy = _mesa_memcpy;
   else
      functions->TextureMemCpy = via_sse_memcpy;
#else
   functions->TextureMemCpy = _mesa_memcpy;
#endif

   functions->UpdateTexturePalette = 0;
   functions->IsTextureResident = viaIsTextureResident;
}


