/*
 * RichEdit - Operation context functions
 *
 * Copyright 2004 by Krzysztof Foltman
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "editor.h"

void ME_InitContext(ME_Context *c, ME_TextEditor *editor, HDC hDC)
{
  c->hDC = hDC;
  c->editor = editor;
  c->pt.x = 0;
  c->pt.y = 0;
  c->rcView = editor->rcFormat;
  c->current_style = NULL;
  c->orig_font = NULL;
  if (hDC) {
      c->dpi.cx = GetDeviceCaps(hDC, LOGPIXELSX);
      c->dpi.cy = GetDeviceCaps(hDC, LOGPIXELSY);
  } else {
      c->dpi.cx = c->dpi.cy = 96;
  }
  if (editor->nAvailWidth)
      c->nAvailWidth = ME_twips2pointsX(c, editor->nAvailWidth);
  else
      c->nAvailWidth = c->rcView.right - c->rcView.left;
}

void ME_DestroyContext(ME_Context *c)
{
    select_style( c, NULL );
}
