/*
 * Mesa 3-D graphics library
 * Version:  6.3
 *
 * Copyright (C) 2005  Brian Paul   All Rights Reserved.
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

#if !defined SLANG_UTILITY_H
#define SLANG_UTILITY_H

#if defined __cplusplus
extern "C" {
#endif

/* Compile-time assertions.  If the expression is zero, try to declare an
 * array of size [-1] to cause compilation error.
 */
#define static_assert(expr) do { int _array[(expr) ? 1 : -1]; _array[0]; } while (0)

void slang_alloc_free (void *);
void *slang_alloc_malloc (unsigned int);
void *slang_alloc_realloc (void *, unsigned int, unsigned int);
int slang_string_compare (const char *, const char *);
char *slang_string_copy (char *, const char *);
char *slang_string_concat (char *, const char *);
char *slang_string_duplicate (const char *);
unsigned int slang_string_length (const char *);

#ifdef __cplusplus
}
#endif

#endif

