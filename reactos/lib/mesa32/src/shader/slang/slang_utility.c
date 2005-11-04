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

/**
 * \file slang_utility.c
 * slang utilities
 * \author Michal Krol
 */

#include "imports.h"
#include "slang_utility.h"

void slang_alloc_free (void *ptr)
{
	_mesa_free (ptr);
}

void *slang_alloc_malloc (unsigned int size)
{
	return _mesa_malloc (size);
}

void *slang_alloc_realloc (void *ptr, unsigned int old_size, unsigned int size)
{
	return _mesa_realloc (ptr, old_size, size);
}

int slang_string_compare (const char *str1, const char *str2)
{
	return _mesa_strcmp (str1, str2);
}

char *slang_string_copy (char *dst, const char *src)
{
	return _mesa_strcpy (dst, src);
}

char *slang_string_concat (char *dst, const char *src)
{
	return _mesa_strcpy (dst + _mesa_strlen (dst), src);
}

char *slang_string_duplicate (const char *src)
{
	return _mesa_strdup (src);
}

unsigned int slang_string_length (const char *str)
{
	return _mesa_strlen (str);
}

