/**************************************************************************
 * 
 * Copyright 2007 Tungsten Graphics, Inc., Cedar Park, Texas.
 * Copyright (c) 2010 VMware, Inc.
 * All Rights Reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 * 
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL TUNGSTEN GRAPHICS AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 * 
 **************************************************************************/


#ifndef ST_FORMAT_H
#define ST_FORMAT_H

#include "main/formats.h"
#include "main/glheader.h"

#include "pipe/p_defines.h"
#include "pipe/p_format.h"

struct gl_context;
struct pipe_screen;


extern enum pipe_format
st_mesa_format_to_pipe_format(gl_format mesaFormat);

extern gl_format
st_pipe_format_to_mesa_format(enum pipe_format pipeFormat);


extern enum pipe_format
st_choose_format(struct pipe_screen *screen, GLenum internalFormat,
                 GLenum format, GLenum type,
                 enum pipe_texture_target target, unsigned sample_count,
                 unsigned bindings);

extern enum pipe_format
st_choose_renderbuffer_format(struct pipe_screen *screen,
                              GLenum internalFormat, unsigned sample_count);


gl_format
st_ChooseTextureFormat_renderable(struct gl_context *ctx, GLint internalFormat,
				  GLenum format, GLenum type, GLboolean renderable);

extern gl_format
st_ChooseTextureFormat(struct gl_context * ctx, GLint internalFormat,
                       GLenum format, GLenum type);


extern GLboolean
st_equal_formats(enum pipe_format pFormat, GLenum format, GLenum type);

/* can we use a sampler view to translate these formats
   only used to make TFP so far */
extern GLboolean
st_sampler_compat_formats(enum pipe_format format1, enum pipe_format format2);


extern void
st_translate_color(const GLfloat colorIn[4], GLenum baseFormat,
                   GLfloat colorOut[4]);

#endif /* ST_FORMAT_H */
