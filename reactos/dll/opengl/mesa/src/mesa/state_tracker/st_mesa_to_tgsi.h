/**************************************************************************
 * 
 * Copyright 2007 Tungsten Graphics, Inc., Cedar Park, Texas.
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


#ifndef ST_MESA_TO_TGSI_H
#define ST_MESA_TO_TGSI_H

#if defined __cplusplus
extern "C" {
#endif

#include "main/glheader.h"

#include "pipe/p_compiler.h"
#include "pipe/p_defines.h"

struct gl_context;
struct gl_program;
struct tgsi_token;
struct ureg_program;


enum pipe_error
st_translate_mesa_program(
   struct gl_context *ctx,
   uint procType,
   struct ureg_program *ureg,
   const struct gl_program *program,
   GLuint numInputs,
   const GLuint inputMapping[],
   const ubyte inputSemanticName[],
   const ubyte inputSemanticIndex[],
   const GLuint interpMode[],
   GLuint numOutputs,
   const GLuint outputMapping[],
   const ubyte outputSemanticName[],
   const ubyte outputSemanticIndex[],
   boolean passthrough_edgeflags );

void
st_free_tokens(const struct tgsi_token *tokens);

unsigned
st_translate_texture_target(GLuint textarget, GLboolean shadow);


#if defined __cplusplus
} /* extern "C" */
#endif

#endif /* ST_MESA_TO_TGSI_H */

