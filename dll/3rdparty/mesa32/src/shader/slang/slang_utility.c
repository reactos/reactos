/*
 * Mesa 3-D graphics library
 * Version:  6.5.3
 *
 * Copyright (C) 2005-2007  Brian Paul   All Rights Reserved.
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

#include "main/imports.h"
#include "slang_utility.h"
#include "slang_mem.h"

char *
slang_string_concat (char *dst, const char *src)
{
   return _mesa_strcpy (dst + _mesa_strlen (dst), src);
}


/* slang_string */

GLvoid
slang_string_init (slang_string *self)
{
   self->data = NULL;
   self->capacity = 0;
   self->length = 0;
   self->fail = GL_FALSE;
}

GLvoid
slang_string_free (slang_string *self)
{
   if (self->data != NULL)
      _mesa_free (self->data);
}

GLvoid
slang_string_reset (slang_string *self)
{
   self->length = 0;
   self->fail = GL_FALSE;
}

static GLboolean
grow (slang_string *self, GLuint size)
{
   if (self->fail)
      return GL_FALSE;
   if (size > self->capacity) {
      /* do not overflow 32-bit range */
      assert (size < 0x80000000);

      self->data = (char *) (_mesa_realloc (self->data, self->capacity, size * 2));
      self->capacity = size * 2;
      if (self->data == NULL) {
         self->capacity = 0;
         self->fail = GL_TRUE;
         return GL_FALSE;
      }
   }
   return GL_TRUE;
}

GLvoid
slang_string_push (slang_string *self, const slang_string *str)
{
   if (str->fail) {
      self->fail = GL_TRUE;
      return;
   }
   if (grow (self, self->length + str->length)) {
      _mesa_memcpy (&self->data[self->length], str->data, str->length);
      self->length += str->length;
   }
}

GLvoid
slang_string_pushc (slang_string *self, const char c)
{
   if (grow (self, self->length + 1)) {
      self->data[self->length] = c;
      self->length++;
   }
}

GLvoid
slang_string_pushs (slang_string *self, const char *cstr, GLuint len)
{
   if (grow (self, self->length + len)) {
      _mesa_memcpy (&self->data[self->length], cstr, len);
      self->length += len;
   }
}

GLvoid
slang_string_pushi (slang_string *self, GLint i)
{
   char buffer[12];

   _mesa_sprintf (buffer, "%d", i);
   slang_string_pushs (self, buffer, strlen (buffer));
}

const char *
slang_string_cstr (slang_string *self)
{
   if (grow (self, self->length + 1))
      self->data[self->length] = '\0';
   return self->data;
}

/* slang_atom_pool */

void
slang_atom_pool_construct(slang_atom_pool * pool)
{
   GLuint i;

   for (i = 0; i < SLANG_ATOM_POOL_SIZE; i++)
      pool->entries[i] = NULL;
}

void
slang_atom_pool_destruct (slang_atom_pool * pool)
{
   GLuint i;

   for (i = 0; i < SLANG_ATOM_POOL_SIZE; i++) {
      slang_atom_entry * entry;
		
      entry = pool->entries[i];
      while (entry != NULL) {
         slang_atom_entry *next = entry->next;
         _slang_free(entry->id);
         _slang_free(entry);
         entry = next;
      }
   }
}

/*
 * Search the atom pool for an atom with a given name.
 * If atom is not found, create and add it to the pool.
 * Returns ATOM_NULL if the atom was not found and the function failed
 * to create a new atom.
 */
slang_atom
slang_atom_pool_atom(slang_atom_pool * pool, const char * id)
{
   GLuint hash;
   const char * p = id;
   slang_atom_entry ** entry;

   /* Hash a given string to a number in the range [0, ATOM_POOL_SIZE). */
   hash = 0;
   while (*p != '\0') {
      GLuint g;

      hash = (hash << 4) + (GLuint) (*p++);
      g = hash & 0xf0000000;
      if (g != 0)
         hash ^= g >> 24;
      hash &= ~g;
   }
   hash %= SLANG_ATOM_POOL_SIZE;

   /* Now the hash points to a linked list of atoms with names that
    * have the same hash value.  Search the linked list for a given
    * name.
    */
   entry = &pool->entries[hash];
   while (*entry != NULL) {
      /* If the same, return the associated atom. */
      if (slang_string_compare((**entry).id, id) == 0)
         return (slang_atom) (**entry).id;
      /* Grab the next atom in the linked list. */
      entry = &(**entry).next;
   }

   /* Okay, we have not found an atom. Create a new entry for it.
    * Note that the <entry> points to the last entry's <next> field.
    */
   *entry = (slang_atom_entry *) _slang_alloc(sizeof(slang_atom_entry));
   if (*entry == NULL)
      return SLANG_ATOM_NULL;

   /* Initialize a new entry. Because we'll need the actual name of
    * the atom, we use the pointer to this string as an actual atom's
    * value.
    */
   (**entry).next = NULL;
   (**entry).id = _slang_strdup(id);
   if ((**entry).id == NULL)
      return SLANG_ATOM_NULL;
   return (slang_atom) (**entry).id;
}

/**
 * Return the name of a given atom.
 */
const char *
slang_atom_pool_id(slang_atom_pool * pool, slang_atom atom)
{
   return (const char *) (atom);
}
