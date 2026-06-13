/*
 * Copyright (C) 2007 Google (Evan Stade)
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

#ifndef _GDIPLUSMEM_H
#define _GDIPLUSMEM_H

#define WINGDIPAPI __stdcall

#ifdef __cplusplus
extern "C" {
#endif

void WINGDIPAPI GdipFree(void*);
void* WINGDIPAPI GdipAlloc(SIZE_T) __WINE_ALLOC_SIZE(1) __WINE_DEALLOC(GdipFree) __WINE_MALLOC;

#ifdef __cplusplus
}
#endif

#endif
