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

#ifndef DRI_DEBUG_H
#define DRI_DEBUG_H

#include "context.h"
#include "dri_util.h"

struct dri_debug_control {
    const char * string;
    unsigned     flag;
};

/**
 * Description of the entry-points and parameters for an OpenGL function.
 */
struct dri_extension_function {
    /**
     * \brief
     * Packed string describing the parameter signature and the entry-point
     * names.
     * 
     * The parameter signature and the names of the entry-points for this
     * function are packed into a single string.  The substrings are
     * separated by NUL characters.  The whole string is terminated by
     * two consecutive NUL characters.
     */
    const char * strings;


    /**
     * Location in the remap table where the dispatch offset should be
     * stored.
     */
    int remap_index;

    /**
     * Offset of the function in the dispatch table.
     */
    int offset;
};

/**
 * Description of the API for an extension to OpenGL.
 */
struct dri_extension {
    /**
     * Name of the extension.
     */
    const char * name;
    

    /**
     * Pointer to a list of \c dri_extension_function structures.  The list
     * is terminated by a structure with a \c NULL
     * \c dri_extension_function::strings pointer.
     */
    const struct dri_extension_function * functions;
};

extern unsigned driParseDebugString( const char * debug,
    const struct dri_debug_control * control );

extern unsigned driGetRendererString( char * buffer,
    const char * hardware_name, const char * driver_date, GLuint agp_mode );

extern void driInitExtensions( GLcontext * ctx, 
    const struct dri_extension * card_extensions, GLboolean enable_imaging );

extern void driInitSingleExtension( GLcontext * ctx,
    const struct dri_extension * ext );

extern GLboolean driCheckDriDdxDrmVersions2(const char * driver_name,
    const __DRIversion * driActual, const __DRIversion * driExpected,
    const __DRIversion * ddxActual, const __DRIversion * ddxExpected,
    const __DRIversion * drmActual, const __DRIversion * drmExpected);

extern GLboolean driCheckDriDdxDrmVersions3(const char * driver_name,
    const __DRIversion * driActual, const __DRIversion * driExpected,
    const __DRIversion * ddxActual, const __DRIutilversion2 * ddxExpected,
    const __DRIversion * drmActual, const __DRIversion * drmExpected);

extern GLint driIntersectArea( drm_clip_rect_t rect1, drm_clip_rect_t rect2 );

extern GLboolean driClipRectToFramebuffer( const GLframebuffer *buffer,
					   GLint *x, GLint *y,
					   GLsizei *width, GLsizei *height );

extern GLboolean driFillInModes( __GLcontextModes ** modes,
    GLenum fb_format, GLenum fb_type,
    const u_int8_t * depth_bits, const u_int8_t * stencil_bits,
    unsigned num_depth_stencil_bits,
    const GLenum * db_modes, unsigned num_db_modes, int visType );

#endif /* DRI_DEBUG_H */
