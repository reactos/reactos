/*
 * (C) Copyright IBM Corporation 2002, 2004
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * on the rights to use, copy, modify, merge, publish, distribute, sub
 * license, and/or sell copies of the Software, and to permit persons to whom
 * the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.  IN NO EVENT SHALL
 * VA LINUX SYSTEM, IBM AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Authors:
 *    Ian Romanick <idr@us.ibm.com>
 */
/* $XFree86:$ */

#include <string.h>
#include <stdlib.h>
#include "mtypes.h"
#include "extensions.h"
#include "utils.h"

#if !defined( DRI_NEW_INTERFACE_ONLY )
#include "xf86dri.h"        /* For XF86DRIQueryVersion prototype. */
#endif

#if defined(USE_X86_ASM)
#include "x86/common_x86_asm.h"
#endif

unsigned
driParseDebugString( const char * debug, 
		     const struct dri_debug_control * control  )
{
   unsigned   flag;


   flag = 0;
   if ( debug != NULL ) {
      while( control->string != NULL ) {
	 if ( !strcmp( debug, "all" ) ||
	      strstr( debug, control->string ) != NULL ) {
	    flag |= control->flag;
	 }

	 control++;
      }
   }

   return flag;
}




unsigned
driGetRendererString( char * buffer, const char * hardware_name,
		      const char * driver_date, GLuint agp_mode )
{
#ifdef USE_X86_ASM
   char * x86_str = "";
   char * mmx_str = "";
   char * tdnow_str = "";
   char * sse_str = "";
#endif
   unsigned   offset;


   offset = sprintf( buffer, "Mesa DRI %s %s", hardware_name, driver_date );

   /* Append any AGP-specific information.
    */
   switch ( agp_mode ) {
   case 1:
   case 2:
   case 4:
   case 8:
      offset += sprintf( & buffer[ offset ], " AGP %ux", agp_mode );
      break;
	
   default:
      break;
   }

   /* Append any CPU-specific information.
    */
#ifdef USE_X86_ASM
   if ( _mesa_x86_cpu_features ) {
      x86_str = " x86";
   }
# ifdef USE_MMX_ASM
   if ( cpu_has_mmx ) {
      mmx_str = (cpu_has_mmxext) ? "/MMX+" : "/MMX";
   }
# endif
# ifdef USE_3DNOW_ASM
   if ( cpu_has_3dnow ) {
      tdnow_str = (cpu_has_3dnowext) ? "/3DNow!+" : "/3DNow!";
   }
# endif
# ifdef USE_SSE_ASM
   if ( cpu_has_xmm ) {
      sse_str = (cpu_has_xmm2) ? "/SSE2" : "/SSE";
   }
# endif

   offset += sprintf( & buffer[ offset ], "%s%s%s%s", 
		      x86_str, mmx_str, tdnow_str, sse_str );

#elif defined(USE_SPARC_ASM)

   offset += sprintf( & buffer[ offset ], " Sparc" );

#endif

   return offset;
}




void driInitExtensions( GLcontext * ctx,
			const char * const extensions_to_enable[],
			GLboolean  enable_imaging )
{
   unsigned   i;

   if ( enable_imaging ) {
      _mesa_enable_imaging_extensions( ctx );
   }

   for ( i = 0 ; extensions_to_enable[i] != NULL ; i++ ) {
      _mesa_enable_extension( ctx, extensions_to_enable[i] );
   }
}




#ifndef DRI_NEW_INTERFACE_ONLY
/**
 * Utility function used by drivers to test the verions of other components.
 * 
 * \deprecated
 * All drivers using the new interface should use \c driCheckDriDdxVersions2
 * instead.  This function is implemented using a call that is not available
 * to drivers using the new interface.  Furthermore, the information gained
 * by this call (the DRI and DDX version information) is already provided to
 * the driver via the new interface.
 */
GLboolean
driCheckDriDdxDrmVersions(__DRIscreenPrivate *sPriv,
			  const char * driver_name,
			  int dri_major, int dri_minor,
			  int ddx_major, int ddx_minor,
			  int drm_major, int drm_minor)
{
   static const char format[] = "%s DRI driver expected %s version %d.%d.x "
       "but got version %d.%d.%d";
   int major, minor, patch;

   /* Check the DRI version */
   if (XF86DRIQueryVersion(sPriv->display, &major, &minor, &patch)) {
      if (major != dri_major || minor < dri_minor) {
	 __driUtilMessage(format, driver_name, "DRI", dri_major, dri_minor,
			  major, minor, patch);
	 return GL_FALSE;
      }
   }

   /* Check that the DDX driver version is compatible */
   if (sPriv->ddxMajor != ddx_major || sPriv->ddxMinor < ddx_minor) {
      __driUtilMessage(format, driver_name, "DDX", ddx_major, ddx_minor,
		       sPriv->ddxMajor, sPriv->ddxMinor, sPriv->ddxPatch);
      return GL_FALSE;
   }
   
   /* Check that the DRM driver version is compatible */
   if (sPriv->drmMajor != drm_major || sPriv->drmMinor < drm_minor) {
      __driUtilMessage(format, driver_name, "DRM", drm_major, drm_minor,
		       sPriv->drmMajor, sPriv->drmMinor, sPriv->drmPatch);
      return GL_FALSE;
   }

   return GL_TRUE;
}
#endif /* DRI_NEW_INTERFACE_ONLY */

/**
 * Utility function used by drivers to test the verions of other components.
 *
 * If one of the version requirements is not met, a message is logged using
 * \c __driUtilMessage.
 *
 * \param driver_name  Name of the driver.  Used in error messages.
 * \param driActual    Actual DRI version supplied __driCreateNewScreen.
 * \param driExpected  Minimum DRI version required by the driver.
 * \param ddxActual    Actual DDX version supplied __driCreateNewScreen.
 * \param ddxExpected  Minimum DDX version required by the driver.
 * \param drmActual    Actual DRM version supplied __driCreateNewScreen.
 * \param drmExpected  Minimum DRM version required by the driver.
 * 
 * \returns \c GL_TRUE if all version requirements are met.  Otherwise,
 *          \c GL_FALSE is returned.
 * 
 * \sa __driCreateNewScreen, driCheckDriDdxDrmVersions, __driUtilMessage
 */
GLboolean
driCheckDriDdxDrmVersions2(const char * driver_name,
			   const __DRIversion * driActual,
			   const __DRIversion * driExpected,
			   const __DRIversion * ddxActual,
			   const __DRIversion * ddxExpected,
			   const __DRIversion * drmActual,
			   const __DRIversion * drmExpected)
{
   static const char format[] = "%s DRI driver expected %s version %d.%d.x "
       "but got version %d.%d.%d";


   /* Check the DRI version */
   if ( (driActual->major != driExpected->major)
	|| (driActual->minor < driExpected->minor) ) {
      __driUtilMessage(format, driver_name, "DRI",
		       driExpected->major, driExpected->minor,
		       driActual->major, driActual->minor, driActual->patch);
      return GL_FALSE;
   }

   /* Check that the DDX driver version is compatible */
   if ( (ddxActual->major != ddxExpected->major)
	|| (ddxActual->minor < ddxExpected->minor) ) {
      __driUtilMessage(format, driver_name, "DDX",
		       ddxExpected->major, ddxExpected->minor,
		       ddxActual->major, ddxActual->minor, ddxActual->patch);
      return GL_FALSE;
   }
   
   /* Check that the DRM driver version is compatible */
   if ( (drmActual->major != drmExpected->major)
	|| (drmActual->minor < drmExpected->minor) ) {
      __driUtilMessage(format, driver_name, "DRM",
		       drmExpected->major, drmExpected->minor,
		       drmActual->major, drmActual->minor, drmActual->patch);
      return GL_FALSE;
   }

   return GL_TRUE;
}


GLboolean driClipRectToFramebuffer( const GLframebuffer *buffer,
				    GLint *x, GLint *y,
				    GLsizei *width, GLsizei *height )
{
   /* left clipping */
   if (*x < buffer->_Xmin) {
      *width -= (buffer->_Xmin - *x);
      *x = buffer->_Xmin;
   }

   /* right clipping */
   if (*x + *width > buffer->_Xmax)
      *width -= (*x + *width - buffer->_Xmax - 1);

   if (*width <= 0)
      return GL_FALSE;

   /* bottom clipping */
   if (*y < buffer->_Ymin) {
      *height -= (buffer->_Ymin - *y);
      *y = buffer->_Ymin;
   }

   /* top clipping */
   if (*y + *height > buffer->_Ymax)
      *height -= (*y + *height - buffer->_Ymax - 1);

   if (*height <= 0)
      return GL_FALSE;

   return GL_TRUE;
}



/**
 * Creates a set of \c __GLcontextModes that a driver will expose.
 * 
 * A set of \c __GLcontextModes will be created based on the supplied
 * parameters.  The number of modes processed will be 2 *
 * \c num_depth_stencil_bits * \c num_db_modes.
 * 
 * For the most part, data is just copied from \c depth_bits, \c stencil_bits,
 * \c db_modes, and \c visType into each \c __GLcontextModes element.
 * However, the meanings of \c fb_format and \c fb_type require further
 * explanation.  The \c fb_format specifies which color components are in
 * each pixel and what the default order is.  For example, \c GL_RGB specifies
 * that red, green, blue are available and red is in the "most significant"
 * position and blue is in the "least significant".  The \c fb_type specifies
 * the bit sizes of each component and the actual ordering.  For example, if
 * \c GL_UNSIGNED_SHORT_5_6_5_REV is specified with \c GL_RGB, bits [15:11]
 * are the blue value, bits [10:5] are the green value, and bits [4:0] are
 * the red value.
 * 
 * One sublte issue is the combination of \c GL_RGB  or \c GL_BGR and either
 * of the \c GL_UNSIGNED_INT_8_8_8_8 modes.  The resulting mask values in the
 * \c __GLcontextModes structure is \b identical to the \c GL_RGBA or
 * \c GL_BGRA case, except the \c alphaMask is zero.  This means that, as
 * far as this routine is concerned, \c GL_RGB with \c GL_UNSIGNED_INT_8_8_8_8
 * still uses 32-bits.
 *
 * If in doubt, look at the tables used in the function.
 * 
 * \param ptr_to_modes  Pointer to a pointer to a linked list of
 *                      \c __GLcontextModes.  Upon completion, a pointer to
 *                      the next element to be process will be stored here.
 *                      If the function fails and returns \c GL_FALSE, this
 *                      value will be unmodified, but some elements in the
 *                      linked list may be modified.
 * \param fb_format     Format of the framebuffer.  Currently only \c GL_RGB,
 *                      \c GL_RGBA, \c GL_BGR, and \c GL_BGRA are supported.
 * \param fb_type       Type of the pixels in the framebuffer.  Currently only
 *                      \c GL_UNSIGNED_SHORT_5_6_5, 
 *                      \c GL_UNSIGNED_SHORT_5_6_5_REV,
 *                      \c GL_UNSIGNED_INT_8_8_8_8, and
 *                      \c GL_UNSIGNED_INT_8_8_8_8_REV are supported.
 * \param depth_bits    Array of depth buffer sizes to be exposed.
 * \param stencil_bits  Array of stencil buffer sizes to be exposed.
 * \param num_depth_stencil_bits  Number of entries in both \c depth_bits and
 *                      \c stencil_bits.
 * \param db_modes      Array of buffer swap modes.  If an element has a
 *                      value of \c GLX_NONE, then it represents a
 *                      single-buffered mode.  Other valid values are
 *                      \c GLX_SWAP_EXCHANGE_OML, \c GLX_SWAP_COPY_OML, and
 *                      \c GLX_SWAP_UNDEFINED_OML.  See the
 *                      GLX_OML_swap_method extension spec for more details.
 * \param num_db_modes  Number of entries in \c db_modes.
 * \param visType       GLX visual type.  Usually either \c GLX_TRUE_COLOR or
 *                      \c GLX_DIRECT_COLOR.
 * 
 * \returns
 * \c GL_TRUE on success or \c GL_FALSE on failure.  Currently the only
 * cause of failure is a bad parameter (i.e., unsupported \c fb_format or
 * \c fb_type).
 * 
 * \todo
 * There is currently no way to support packed RGB modes (i.e., modes with
 * exactly 3 bytes per pixel) or floating-point modes.  This could probably
 * be done by creating some new, private enums with clever names likes
 * \c GL_UNSIGNED_3BYTE_8_8_8, \c GL_4FLOAT_32_32_32_32, 
 * \c GL_4HALF_16_16_16_16, etc.  We can cross that bridge when we come to it.
 */
GLboolean
driFillInModes( __GLcontextModes ** ptr_to_modes,
		GLenum fb_format, GLenum fb_type,
		const uint8_t * depth_bits, const uint8_t * stencil_bits,
		unsigned num_depth_stencil_bits,
		const GLenum * db_modes, unsigned num_db_modes,
		int visType )
{
   static const uint8_t bits_table[3][4] = {
     /* R  G  B  A */
      { 5, 6, 5, 0 }, /* Any GL_UNSIGNED_SHORT_5_6_5 */
      { 8, 8, 8, 0 }, /* Any RGB with any GL_UNSIGNED_INT_8_8_8_8 */
      { 8, 8, 8, 8 }  /* Any RGBA with any GL_UNSIGNED_INT_8_8_8_8 */
   };

   /* The following arrays are all indexed by the fb_type masked with 0x07.
    * Given the four supported fb_type values, this results in valid array
    * indices of 3, 4, 5, and 7.
    */
   static const uint32_t masks_table_rgb[8][4] = {
      { 0x00000000, 0x00000000, 0x00000000, 0x00000000 },
      { 0x00000000, 0x00000000, 0x00000000, 0x00000000 },
      { 0x00000000, 0x00000000, 0x00000000, 0x00000000 },
      { 0x0000F800, 0x000007E0, 0x0000001F, 0x00000000 }, /* 5_6_5       */
      { 0x0000001F, 0x000007E0, 0x0000F800, 0x00000000 }, /* 5_6_5_REV   */
      { 0xFF000000, 0x00FF0000, 0x0000FF00, 0x00000000 }, /* 8_8_8_8     */
      { 0x00000000, 0x00000000, 0x00000000, 0x00000000 },
      { 0x000000FF, 0x0000FF00, 0x00FF0000, 0x00000000 }  /* 8_8_8_8_REV */
   };

   static const uint32_t masks_table_rgba[8][4] = {
      { 0x00000000, 0x00000000, 0x00000000, 0x00000000 },
      { 0x00000000, 0x00000000, 0x00000000, 0x00000000 },
      { 0x00000000, 0x00000000, 0x00000000, 0x00000000 },
      { 0x0000F800, 0x000007E0, 0x0000001F, 0x00000000 }, /* 5_6_5       */
      { 0x0000001F, 0x000007E0, 0x0000F800, 0x00000000 }, /* 5_6_5_REV   */
      { 0xFF000000, 0x00FF0000, 0x0000FF00, 0x000000FF }, /* 8_8_8_8     */
      { 0x00000000, 0x00000000, 0x00000000, 0x00000000 },
      { 0x000000FF, 0x0000FF00, 0x00FF0000, 0xFF000000 }, /* 8_8_8_8_REV */
   };

   static const uint32_t masks_table_bgr[8][4] = {
      { 0x00000000, 0x00000000, 0x00000000, 0x00000000 },
      { 0x00000000, 0x00000000, 0x00000000, 0x00000000 },
      { 0x00000000, 0x00000000, 0x00000000, 0x00000000 },
      { 0x0000001F, 0x000007E0, 0x0000F800, 0x00000000 }, /* 5_6_5       */
      { 0x0000F800, 0x000007E0, 0x0000001F, 0x00000000 }, /* 5_6_5_REV   */
      { 0x0000FF00, 0x00FF0000, 0xFF000000, 0x00000000 }, /* 8_8_8_8     */
      { 0x00000000, 0x00000000, 0x00000000, 0x00000000 },
      { 0x00FF0000, 0x0000FF00, 0x000000FF, 0x00000000 }, /* 8_8_8_8_REV */
   };

   static const uint32_t masks_table_bgra[8][4] = {
      { 0x00000000, 0x00000000, 0x00000000, 0x00000000 },
      { 0x00000000, 0x00000000, 0x00000000, 0x00000000 },
      { 0x00000000, 0x00000000, 0x00000000, 0x00000000 },
      { 0x0000001F, 0x000007E0, 0x0000F800, 0x00000000 }, /* 5_6_5       */
      { 0x0000F800, 0x000007E0, 0x0000001F, 0x00000000 }, /* 5_6_5_REV   */
      { 0x0000FF00, 0x00FF0000, 0xFF000000, 0x000000FF }, /* 8_8_8_8     */
      { 0x00000000, 0x00000000, 0x00000000, 0x00000000 },
      { 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000 }, /* 8_8_8_8_REV */
   };

   static const uint8_t bytes_per_pixel[8] = {
      0, 0, 0, 2, 2, 4, 0, 4
   };

   const uint8_t  * bits;
   const uint32_t * masks;
   const int index = fb_type & 0x07;
   __GLcontextModes * modes = *ptr_to_modes;
   unsigned i;
   unsigned j;
   unsigned k;


   if ( bytes_per_pixel[ index ] == 0 ) {
      fprintf( stderr, "[%s:%u] Framebuffer type 0x%04x has 0 bytes per pixel.\n",
	       __func__, __LINE__, fb_type );
      return GL_FALSE;
   }


   /* Valid types are GL_UNSIGNED_SHORT_5_6_5 and GL_UNSIGNED_INT_8_8_8_8 and
    * the _REV versions.
    *
    * Valid formats are GL_RGBA, GL_RGB, and GL_BGRA.
    */

   switch ( fb_format ) {
      case GL_RGB:
         bits = (bytes_per_pixel[ index ] == 2)
	     ? bits_table[0] : bits_table[1];
         masks = masks_table_rgb[ index ];
         break;

      case GL_RGBA:
         bits = (bytes_per_pixel[ index ] == 2)
	     ? bits_table[0] : bits_table[2];
         masks = masks_table_rgba[ index ];
         break;

      case GL_BGR:
         bits = (bytes_per_pixel[ index ] == 2)
	     ? bits_table[0] : bits_table[1];
         masks = masks_table_bgr[ index ];
         break;

      case GL_BGRA:
         bits = (bytes_per_pixel[ index ] == 2)
	     ? bits_table[0] : bits_table[2];
         masks = masks_table_bgra[ index ];
         break;

      default:
         fprintf( stderr, "[%s:%u] Framebuffer format 0x%04x is not GL_RGB, GL_RGBA, GL_BGR, or GL_BGRA.\n",
	       __func__, __LINE__, fb_format );
         return GL_FALSE;
   }


    for ( k = 0 ; k < num_depth_stencil_bits ; k++ ) {
	for ( i = 0 ; i < num_db_modes ; i++ ) {
	    for ( j = 0 ; j < 2 ; j++ ) {

		modes->redBits   = bits[0];
		modes->greenBits = bits[1];
		modes->blueBits  = bits[2];
		modes->alphaBits = bits[3];
		modes->redMask   = masks[0];
		modes->greenMask = masks[1];
		modes->blueMask  = masks[2];
		modes->alphaMask = masks[3];
		modes->rgbBits   = modes->redBits + modes->greenBits
		    + modes->blueBits + modes->alphaBits;

		modes->accumRedBits   = 16 * j;
		modes->accumGreenBits = 16 * j;
		modes->accumBlueBits  = 16 * j;
		modes->accumAlphaBits = (masks[3] != 0) ? 16 * j : 0;
		modes->visualRating = (j == 0) ? GLX_NONE : GLX_SLOW_CONFIG;

		modes->stencilBits = stencil_bits[k];
		modes->depthBits = depth_bits[k];

		modes->visualType = visType;
		modes->renderType = GLX_RGBA_BIT;
		modes->drawableType = GLX_WINDOW_BIT;
		modes->rgbMode = GL_TRUE;

		if ( db_modes[i] == GLX_NONE ) {
		    modes->doubleBufferMode = GL_FALSE;
		}
		else {
		    modes->doubleBufferMode = GL_TRUE;
		    modes->swapMethod = db_modes[i];
		}

		modes = modes->next;
	    }
	}
    }

    *ptr_to_modes = modes;
    return GL_TRUE;
}
