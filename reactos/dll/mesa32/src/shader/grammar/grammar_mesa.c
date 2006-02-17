/*
 * Mesa 3-D graphics library
 * Version:  6.1
 *
 * Copyright (C) 1999-2004  Brian Paul   All Rights Reserved.
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
 * \file grammar_mesa.c
 * mesa3d port to syntax parsing engine
 * \author Michal Krol
 */

#include "grammar_mesa.h"

#define GRAMMAR_PORT_BUILD 1
#include "grammar.c"
#undef GRAMMAR_PORT_BUILD


void grammar_alloc_free (void *ptr)
{
    _mesa_free (ptr);
}

void *grammar_alloc_malloc (size_t size)
{
    return _mesa_malloc (size);
}

void *grammar_alloc_realloc (void *ptr, size_t old_size, size_t size)
{
    return _mesa_realloc (ptr, old_size, size);
}

void *grammar_memory_copy (void *dst, const void * src, size_t size)
{
    return _mesa_memcpy (dst, src, size);
}

int grammar_string_compare (const byte *str1, const byte *str2)
{
    return _mesa_strcmp ((const char *) str1, (const char *) str2);
}

int grammar_string_compare_n (const byte *str1, const byte *str2, size_t n)
{
    return _mesa_strncmp ((const char *) str1, (const char *) str2, n);
}

byte *grammar_string_copy (byte *dst, const byte *src)
{
    return (byte *) _mesa_strcpy ((char *) dst, (const char *) src);
}

byte *grammar_string_copy_n (byte *dst, const byte *src, size_t n)
{
    return (byte *) _mesa_strncpy ((char *) dst, (const char *) src, n);
}

byte *grammar_string_duplicate (const byte *src)
{
    return (byte *) _mesa_strdup ((const char *) src);
}

unsigned int grammar_string_length (const byte *str)
{
    return (unsigned int)_mesa_strlen ((const char *) str);
}

