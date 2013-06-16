/*
 * Copyright (C) 2009 Chia-I Wu <olv@0xlab.org>
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

#include "main/drawtex.h"
#include "main/state.h"
#include "main/imports.h"
#include "main/mfeatures.h"
#include "main/mtypes.h"


#if FEATURE_OES_draw_texture


static void
draw_texture(struct gl_context *ctx, GLfloat x, GLfloat y, GLfloat z,
             GLfloat width, GLfloat height)
{
   if (!ctx->Extensions.OES_draw_texture) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                  "glDrawTex(unsupported)");
      return;
   }
   if (width <= 0.0f || height <= 0.0f) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glDrawTex(width or height <= 0)");
      return;
   }

   _mesa_set_vp_override(ctx, GL_TRUE);

   if (ctx->NewState)
      _mesa_update_state(ctx);

   ASSERT(ctx->Driver.DrawTex);
   ctx->Driver.DrawTex(ctx, x, y, z, width, height);

   _mesa_set_vp_override(ctx, GL_FALSE);
}


void GLAPIENTRY
_mesa_DrawTexf(GLfloat x, GLfloat y, GLfloat z, GLfloat width, GLfloat height)
{
   GET_CURRENT_CONTEXT(ctx);
   draw_texture(ctx, x, y, z, width, height);
}


void GLAPIENTRY
_mesa_DrawTexfv(const GLfloat *coords)
{
   GET_CURRENT_CONTEXT(ctx);
   draw_texture(ctx, coords[0], coords[1], coords[2], coords[3], coords[4]);
}


void GLAPIENTRY
_mesa_DrawTexi(GLint x, GLint y, GLint z, GLint width, GLint height)
{
   GET_CURRENT_CONTEXT(ctx);
   draw_texture(ctx, (GLfloat) x, (GLfloat) y, (GLfloat) z,
                (GLfloat) width, (GLfloat) height);
}


void GLAPIENTRY
_mesa_DrawTexiv(const GLint *coords)
{
   GET_CURRENT_CONTEXT(ctx);
   draw_texture(ctx, (GLfloat) coords[0], (GLfloat) coords[1],
                (GLfloat) coords[2], (GLfloat) coords[3], (GLfloat) coords[4]);
}


void GLAPIENTRY
_mesa_DrawTexs(GLshort x, GLshort y, GLshort z, GLshort width, GLshort height)
{
   GET_CURRENT_CONTEXT(ctx);
   draw_texture(ctx, (GLfloat) x, (GLfloat) y, (GLfloat) z,
                (GLfloat) width, (GLfloat) height);
}


void GLAPIENTRY
_mesa_DrawTexsv(const GLshort *coords)
{
   GET_CURRENT_CONTEXT(ctx);
   draw_texture(ctx, (GLfloat) coords[0], (GLfloat) coords[1],
                (GLfloat) coords[2], (GLfloat) coords[3], (GLfloat) coords[4]);
}


void GLAPIENTRY
_mesa_DrawTexx(GLfixed x, GLfixed y, GLfixed z, GLfixed width, GLfixed height)
{
   GET_CURRENT_CONTEXT(ctx);
   draw_texture(ctx,
                (GLfloat) x / 65536.0f,
                (GLfloat) y / 65536.0f,
                (GLfloat) z / 65536.0f,
                (GLfloat) width / 65536.0f,
                (GLfloat) height / 65536.0f);
}


void GLAPIENTRY
_mesa_DrawTexxv(const GLfixed *coords)
{
   GET_CURRENT_CONTEXT(ctx);
   draw_texture(ctx,
                (GLfloat) coords[0] / 65536.0f,
                (GLfloat) coords[1] / 65536.0f,
                (GLfloat) coords[2] / 65536.0f,
                (GLfloat) coords[3] / 65536.0f,
                (GLfloat) coords[4] / 65536.0f);
}

#endif /* FEATURE_OES_draw_texture */
