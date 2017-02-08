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

 /*
  * Authors:
  *   Keith Whitwell <keith@tungstengraphics.com>
  *   Brian Paul
  */

#include "main/glheader.h"
#include "main/macros.h"
#include "pipe/p_context.h"
#include "pipe/p_screen.h"
#include "util/u_string.h"
#include "st_context.h"
#include "st_cb_strings.h"

#define ST_VERSION_STRING "0.4"

static const GLubyte *
st_get_string(struct gl_context * ctx, GLenum name)
{
   struct st_context *st = st_context(ctx);
   struct pipe_screen *screen = st->pipe->screen;

   switch (name) {
   case GL_VENDOR: {
      const char *vendor = screen->get_vendor( screen );
      util_snprintf(st->vendor, sizeof(st->vendor), "%s", vendor);
      return (GLubyte *) st->vendor;
   }

   case GL_RENDERER:
      util_snprintf(st->renderer, sizeof(st->renderer), "Gallium %s on %s", 
               ST_VERSION_STRING,
	       screen->get_name( screen ));

      return (GLubyte *) st->renderer;

   default:
      return NULL;
   }
}


void st_init_string_functions(struct dd_function_table *functions)
{
   functions->GetString = st_get_string;
}
