/*
 * Mesa 3-D graphics library
 * Version:  7.9
 *
 * Copyright (C) 2010 LunarG Inc.
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
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Authors:
 *    Chia-I Wu <olv@lunarg.com>
 */

#ifndef _MAPI_H_
#define _MAPI_H_

#include "u_compiler.h"

#ifdef _WIN32
#ifdef MAPI_DLL_EXPORTS
#define MAPI_EXPORT __declspec(dllexport)
#else
#define MAPI_EXPORT __declspec(dllimport)
#endif
#else /* _WIN32 */
#define MAPI_EXPORT PUBLIC
#endif

typedef void (*mapi_proc)(void);

struct mapi_table;

MAPI_EXPORT void
mapi_init(const char *spec);

MAPI_EXPORT mapi_proc
mapi_get_proc_address(const char *name);

MAPI_EXPORT struct mapi_table *
mapi_table_create(void);

MAPI_EXPORT void
mapi_table_destroy(struct mapi_table *tbl);

MAPI_EXPORT void
mapi_table_fill(struct mapi_table *tbl, const mapi_proc *procs);

MAPI_EXPORT void
mapi_table_make_current(const struct mapi_table *tbl);

#endif /* _MAPI_H_ */
