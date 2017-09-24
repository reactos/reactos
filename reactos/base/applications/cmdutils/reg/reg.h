/*
 * Copyright 2017 Hugh McMaster
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

#ifndef __REG_H__
#define __REG_H__

#include "resource.h"

#define ARRAY_SIZE(A) (sizeof(A)/sizeof(*A))

/* reg.c */
void *heap_xalloc(size_t size);
void *heap_xrealloc(void *buf, size_t size);
BOOL heap_free(void *buf);
void __cdecl output_message(unsigned int id, ...);
HKEY path_get_rootkey(const WCHAR *path);

/* import.c */
int reg_import(const WCHAR *filename);

#endif /* __REG_H__ */
