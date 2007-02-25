/*
 * Mesa 3-D graphics library
 * Version:  6.2
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

#ifndef GRAMMAR_H
#define GRAMMAR_H


#ifndef GRAMMAR_PORT_INCLUDE
#error Do not include this file directly, include your grammar_XXX.h instead
#endif


#ifdef __cplusplus
extern "C" {
#endif

void grammar_alloc_free (void *);
void *grammar_alloc_malloc (size_t);
void *grammar_alloc_realloc (void *, size_t, size_t);
void *grammar_memory_copy (void *, const void *, size_t);
int grammar_string_compare (const byte *, const byte *);
int grammar_string_compare_n (const byte *, const byte *, size_t);
byte *grammar_string_copy (byte *, const byte *);
byte *grammar_string_copy_n (byte *, const byte *, size_t);
byte *grammar_string_duplicate (const byte *);
unsigned int grammar_string_length (const byte *);

/*
    loads grammar script from null-terminated ASCII <text>
    returns unique grammar id to grammar object
    returns 0 if an error occurs (call grammar_get_last_error to retrieve the error text)
*/
grammar grammar_load_from_text (const byte *text);

/*
    sets a new <value> to a register <name> for grammar <id>
    returns 0 on error (call grammar_get_last_error to retrieve the error text)
    returns 1 on success
*/
int grammar_set_reg8 (grammar id, const byte *name, byte value);

/*
    this function is obsolete, use only for debugging purposes

    checks if a null-terminated <text> matches given grammar <id>
    returns 0 on error (call grammar_get_last_error to retrieve the error text)
    returns 1 on success, the <prod> points to newly allocated buffer with production and <size>
    is filled with the production size
    call grammar_alloc_free to free the memory block pointed by <prod>
*/
int grammar_check (grammar id, const byte *text, byte **prod, unsigned int *size);

/*
    does the same what grammar_check does but much more (approx. 4 times) faster
    use this function instead of grammar_check
    <estimate_prod_size> is a hint - the initial production buffer size will be of this size,
    but if more room is needed it will be safely resized; set it to 0x1000 or so
*/
int grammar_fast_check (grammar id, const byte *text, byte **prod, unsigned int *size,
    unsigned int estimate_prod_size);

/*
    destroys grammar object identified by <id>
    returns 0 on error (call grammar_get_last_error to retrieve the error text)
    returns 1 on success
*/
int grammar_destroy (grammar id);

/*
    retrieves last grammar error reported either by grammar_load_from_text, grammar_check
    or grammar_destroy
    the user allocated <text> buffer receives error description, <pos> points to error position,
    <size> is the size of the text buffer to fill in - it must be at least 4 bytes long,
*/
void grammar_get_last_error (byte *text, unsigned int size, int *pos);

#ifdef __cplusplus
}
#endif

#endif

