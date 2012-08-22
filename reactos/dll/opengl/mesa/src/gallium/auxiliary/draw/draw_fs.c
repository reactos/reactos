/**************************************************************************
 *
 * Copyright 2010 VMware, Inc.
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
 * IN NO EVENT SHALL VMWARE AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/

#include "pipe/p_shader_tokens.h"

#include "util/u_math.h"
#include "util/u_memory.h"
#include "util/u_prim.h"

#include "tgsi/tgsi_parse.h"

#include "draw_fs.h"
#include "draw_private.h"
#include "draw_context.h"


struct draw_fragment_shader *
draw_create_fragment_shader(struct draw_context *draw,
                            const struct pipe_shader_state *shader)
{
   struct draw_fragment_shader *dfs;

   dfs = CALLOC_STRUCT(draw_fragment_shader);
   if (dfs) {
      dfs->base = *shader;
      tgsi_scan_shader(shader->tokens, &dfs->info);
   }

   return dfs;
}


void
draw_bind_fragment_shader(struct draw_context *draw,
                          struct draw_fragment_shader *dfs)
{
   draw_do_flush(draw, DRAW_FLUSH_STATE_CHANGE);

   draw->fs.fragment_shader = dfs;
}


void
draw_delete_fragment_shader(struct draw_context *draw,
                            struct draw_fragment_shader *dfs)
{
   FREE(dfs);
}

