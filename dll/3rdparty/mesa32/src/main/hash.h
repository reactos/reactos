/**
 * \file hash.h
 * Generic hash table. 
 */

/*
 * Mesa 3-D graphics library
 * Version:  6.5.1
 *
 * Copyright (C) 1999-2006  Brian Paul   All Rights Reserved.
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


#ifndef HASH_H
#define HASH_H


#include "glheader.h"


extern struct _mesa_HashTable *_mesa_NewHashTable(void);

extern void _mesa_DeleteHashTable(struct _mesa_HashTable *table);

extern void *_mesa_HashLookup(const struct _mesa_HashTable *table, GLuint key);

extern void _mesa_HashInsert(struct _mesa_HashTable *table, GLuint key, void *data);

extern void _mesa_HashRemove(struct _mesa_HashTable *table, GLuint key);

extern void
_mesa_HashDeleteAll(struct _mesa_HashTable *table,
                    void (*callback)(GLuint key, void *data, void *userData),
                    void *userData);

extern void
_mesa_HashWalk(const struct _mesa_HashTable *table,
               void (*callback)(GLuint key, void *data, void *userData),
               void *userData);

extern GLuint _mesa_HashFirstEntry(struct _mesa_HashTable *table);

extern GLuint _mesa_HashNextEntry(const struct _mesa_HashTable *table, GLuint key);

extern void _mesa_HashPrint(const struct _mesa_HashTable *table);

extern GLuint _mesa_HashFindFreeKeyBlock(struct _mesa_HashTable *table, GLuint numKeys);

extern void _mesa_test_hash_functions(void);


#endif
