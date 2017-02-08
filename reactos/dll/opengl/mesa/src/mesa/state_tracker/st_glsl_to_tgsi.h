/*
 * Copyright © 2010 Intel Corporation
 * Copyright © 2011 Bryan Cain
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#ifdef __cplusplus
extern "C" {
#endif

#include "main/glheader.h"
#include "tgsi/tgsi_ureg.h"

struct gl_context;
struct gl_shader;
struct gl_shader_program;
struct glsl_to_tgsi_visitor;

enum pipe_error st_translate_program(
   struct gl_context *ctx,
   uint procType,
   struct ureg_program *ureg,
   struct glsl_to_tgsi_visitor *program,
   const struct gl_program *proginfo,
   GLuint numInputs,
   const GLuint inputMapping[],
   const ubyte inputSemanticName[],
   const ubyte inputSemanticIndex[],
   const GLuint interpMode[],
   GLuint numOutputs,
   const GLuint outputMapping[],
   const ubyte outputSemanticName[],
   const ubyte outputSemanticIndex[],
   boolean passthrough_edgeflags);

void free_glsl_to_tgsi_visitor(struct glsl_to_tgsi_visitor *v);
void get_pixel_transfer_visitor(struct st_fragment_program *fp,
                                struct glsl_to_tgsi_visitor *original,
                                int scale_and_bias, int pixel_maps);
void get_bitmap_visitor(struct st_fragment_program *fp,
                        struct glsl_to_tgsi_visitor *original,
                        int samplerIndex);

struct gl_shader *st_new_shader(struct gl_context *ctx, GLuint name, GLuint type);

struct gl_shader_program *
st_new_shader_program(struct gl_context *ctx, GLuint name);

GLboolean st_link_shader(struct gl_context *ctx, struct gl_shader_program *prog);

void
st_translate_stream_output_info(struct glsl_to_tgsi_visitor *glsl_to_tgsi,
                                const GLuint outputMapping[],
                                struct pipe_stream_output_info *so);


#ifdef __cplusplus
}
#endif
