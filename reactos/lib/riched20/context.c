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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "editor.h"

void ME_InitContext(ME_Context *c, ME_TextEditor *editor, HDC hDC)
{
  c->nSequence = editor->nSequence++;  
  c->hDC = hDC;
  c->editor = editor;
  c->pt.x = 0;
  c->pt.y = 0;
  c->hbrMargin = CreateSolidBrush(RGB(224,224,224));
  GetClientRect(editor->hWnd, &c->rcView);
}

void ME_DestroyContext(ME_Context *c)
{
  DeleteObject(c->hbrMargin);
}
