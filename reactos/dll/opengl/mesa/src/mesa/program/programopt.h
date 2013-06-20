/*
 * Mesa 3-D graphics library
 * Version:  6.5.3
 *
 * Copyright (C) 1999-2007  Brian Paul   All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */


#ifndef PROGRAMOPT_H
#define PROGRAMOPT_H 1

#include "main/mtypes.h"

extern void
_mesa_insert_mvp_code(struct gl_context *ctx, struct gl_vertex_program *vprog);

extern void
_mesa_append_fog_code(struct gl_context *ctx,
		      struct gl_fragment_program *fprog, GLenum fog_mode,
		      GLboolean saturate);

extern void
_mesa_count_texture_indirections(struct gl_program *prog);

extern void
_mesa_count_texture_instructions(struct gl_program *prog);

extern void
_mesa_remove_output_reads(struct gl_program *prog, gl_register_file type);

extern void
_mesa_nop_fragment_program(struct gl_context *ctx, struct gl_fragment_program *prog);

extern void
_mesa_nop_vertex_program(struct gl_context *ctx, struct gl_vertex_program *prog);


#endif /* PROGRAMOPT_H */
