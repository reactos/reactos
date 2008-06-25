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

#ifndef _VIAFB_INC
#define _VIAFB_INC

#include "via_context.h"

extern GLboolean via_alloc_draw_buffer(struct via_context *vmesa, struct via_renderbuffer *buf);
extern GLboolean via_alloc_dma_buffer(struct via_context *vmesa);

struct via_tex_buffer *
via_alloc_texture(struct via_context *vmesa,
		  GLuint size,
		  GLuint memType);

extern void via_free_draw_buffer(struct via_context *vmesa, struct via_renderbuffer *buf);
extern void via_free_dma_buffer(struct via_context *vmesa);
extern void via_free_texture(struct via_context *vmesa, struct via_tex_buffer *t);
void via_release_pending_textures( struct via_context *vmesa );
#endif
