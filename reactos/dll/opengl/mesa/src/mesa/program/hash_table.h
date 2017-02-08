/*
 * Copyright © 2008 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

/**
 * \file hash_table.h
 * \brief Implementation of a generic, opaque hash table data type.
 *
 * \author Ian Romanick <ian.d.romanick@intel.com>
 */

#ifndef HASH_TABLE_H
#define HASH_TABLE_H

#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <limits.h>
#include <assert.h>

struct string_to_uint_map;

#ifdef __cplusplus
extern "C" {
#endif

struct hash_table;

typedef unsigned (*hash_func_t)(const void *key);
typedef int (*hash_compare_func_t)(const void *key1, const void *key2);

/**
 * Hash table constructor
 *
 * Creates a hash table with the specified number of buckets.  The supplied
 * \c hash and \c compare routines are used when adding elements to the table
 * and when searching for elements in the table.
 *
 * \param num_buckets  Number of buckets (bins) in the hash table.
 * \param hash         Function used to compute hash value of input keys.
 * \param compare      Function used to compare keys.
 */
extern struct hash_table *hash_table_ctor(unsigned num_buckets,
    hash_func_t hash, hash_compare_func_t compare);


/**
 * Release all memory associated with a hash table
 *
 * \warning
 * This function cannot release memory occupied either by keys or data.
 */
extern void hash_table_dtor(struct hash_table *ht);


/**
 * Flush all entries from a hash table
 *
 * \param ht  Table to be cleared of its entries.
 */
extern void hash_table_clear(struct hash_table *ht);


/**
 * Search a hash table for a specific element
 *
 * \param ht   Table to be searched
 * \param key  Key of the desired element
 *
 * \return
 * The \c data value supplied to \c hash_table_insert when the element with
 * the matching key was added.  If no matching key exists in the table,
 * \c NULL is returned.
 */
extern void *hash_table_find(struct hash_table *ht, const void *key);


/**
 * Add an element to a hash table
 *
 * \warning
 * If \c key is already in the hash table, it will be added again.  Future
 * calls to \c hash_table_find and \c hash_table_remove will return or remove,
 * repsectively, the most recently added instance of \c key.
 *
 * \warning
 * The value passed by \c key is kept in the hash table and is used by later
 * calls to \c hash_table_find.
 *
 * \sa hash_table_replace
 */
extern void hash_table_insert(struct hash_table *ht, void *data,
    const void *key);

/**
 * Add an element to a hash table with replacement
 *
 * \warning
 * If \c key is already in the hash table, \c data will \b replace the most
 * recently inserted \c data (see the warning in \c hash_table_insert) for
 * that key.
 *
 * \sa hash_table_insert
 */
extern void hash_table_replace(struct hash_table *ht, void *data,
    const void *key);

/**
 * Remove a specific element from a hash table.
 */
extern void hash_table_remove(struct hash_table *ht, const void *key);

/**
 * Compute hash value of a string
 *
 * Computes the hash value of a string using the DJB2 algorithm developed by
 * Professor Daniel J. Bernstein.  It was published on comp.lang.c once upon
 * a time.  I was unable to find the original posting in the archives.
 *
 * \param key  Pointer to a NUL terminated string to be hashed.
 *
 * \sa hash_table_string_compare
 */
extern unsigned hash_table_string_hash(const void *key);


/**
 * Compare two strings used as keys
 *
 * This is just a macro wrapper around \c strcmp.
 *
 * \sa hash_table_string_hash
 */
#define hash_table_string_compare ((hash_compare_func_t) strcmp)


/**
 * Compute hash value of a pointer
 *
 * \param key  Pointer to be used as a hash key
 *
 * \note
 * The memory pointed to by \c key is \b never accessed.  The value of \c key
 * itself is used as the hash key
 *
 * \sa hash_table_pointer_compare
 */
unsigned
hash_table_pointer_hash(const void *key);


/**
 * Compare two pointers used as keys
 *
 * \sa hash_table_pointer_hash
 */
int
hash_table_pointer_compare(const void *key1, const void *key2);

void
hash_table_call_foreach(struct hash_table *ht,
			void (*callback)(const void *key,
					 void *data,
					 void *closure),
			void *closure);

struct string_to_uint_map *
string_to_uint_map_ctor();

void
string_to_uint_map_dtor(struct string_to_uint_map *);


#ifdef __cplusplus
}

/**
 * Map from a string (name) to an unsigned integer value
 *
 * \note
 * Because of the way this class interacts with the \c hash_table
 * implementation, values of \c UINT_MAX cannot be stored in the map.
 */
struct string_to_uint_map {
public:
   string_to_uint_map()
   {
      this->ht = hash_table_ctor(0, hash_table_string_hash,
				 hash_table_string_compare);
   }

   ~string_to_uint_map()
   {
      hash_table_call_foreach(this->ht, delete_key, NULL);
      hash_table_dtor(this->ht);
   }

   /**
    * Remove all mappings from this map.
    */
   void clear()
   {
      hash_table_clear(this->ht);
   }

   /**
    * Get the value associated with a particular key
    *
    * \return
    * If \c key is found in the map, \c true is returned.  Otherwise \c false
    * is returned.
    *
    * \note
    * If \c key is not found in the table, \c value is not modified.
    */
   bool get(unsigned &value, const char *key)
   {
      const intptr_t v =
	 (intptr_t) hash_table_find(this->ht, (const void *) key);

      if (v == 0)
	 return false;

      value = (unsigned)(v - 1);
      return true;
   }

   void put(unsigned value, const char *key)
   {
      /* The low-level hash table structure returns NULL if key is not in the
       * hash table.  However, users of this map might want to store zero as a
       * valid value in the table.  Bias the value by +1 so that a
       * user-specified zero is stored as 1.  This enables ::get to tell the
       * difference between a user-specified zero (returned as 1 by
       * hash_table_find) and the key not in the table (returned as 0 by
       * hash_table_find).
       *
       * The net effect is that we can't store UINT_MAX in the table.  This is
       * because UINT_MAX+1 = 0.
       */
      assert(value != UINT_MAX);
      hash_table_replace(this->ht,
			 (void *) (intptr_t) (value + 1),
			 strdup(key));
   }

private:
   static void delete_key(const void *key, void *data, void *closure)
   {
      (void) data;
      (void) closure;

      free((char *)key);
   }

   struct hash_table *ht;
};

#endif /* __cplusplus */
#endif /* HASH_TABLE_H */
