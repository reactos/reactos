/*
 * (c) Copyright IBM Corporation 2002
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

struct dri_debug_control
{
    const char * string;
    unsigned     flag;
};

extern unsigned driParseDebugString( const char * debug,
    const struct dri_debug_control * control );

extern unsigned driGetRendererString( char * buffer,
    const char * hardware_name, const char * driver_date, GLuint agp_mode );

extern void driInitExtensions( GLcontext * ctx, 
    const char * const card_extensions[], GLboolean enable_imaging );

extern GLboolean driCheckDriDdxDrmVersions( __DRIscreenPrivate *sPriv,
    const char * driver_name, int dri_major, int dri_minor,
    int ddx_major, int ddx_minor, int drm_major, int drm_minor );

extern GLboolean driClipRectToFramebuffer( const GLframebuffer *buffer,
					   GLint *x, GLint *y,
					   GLsizei *width, GLsizei *height );

#endif /* DRI_DEBUG_H */
