/* Simple dictionary
 *
 * Copyright 2005 Juan Lang
 *
 * This is a pretty basic dictionary, or map if you prefer.  It's not
 * thread-safe.
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
#ifndef __DICTIONARY_H__
#define __DICTIONARY_H__

#include <stdarg.h>
#include "windef.h"
#include "winbase.h"

struct dictionary;

/* Returns whether key a is less than, equal to, or greater than key b, in
 * the same way (a - b) would for integers or strcmp(a, b) would for ANSI
 * strings.
 */
typedef int (*comparefunc)(const void *a, const void *b, void *extra);

/* Called for every element removed from the dictionary.  See
 * dictionary_destroy, dictionary_insert, and dictionary_remove.
 */
typedef void (*destroyfunc)(void *k, void *v, void *extra);

/* Called for each element in the dictionary.  Return FALSE if you don't want
 * to enumerate any more.
 */
typedef BOOL (*enumeratefunc)(const void *k, const void *v, void *extra,
 void *closure);

/* Constructs a dictionary, using c as a comparison function for keys.
 * If d is not NULL, it will be called whenever an item is about to be removed
 * from the table, for example when dictionary_remove is called for a key, or
 * when dictionary_destroy is called.
 * extra is passed to c (and d, if it's provided).
 * Assumes c is not NULL.
 */
struct dictionary *dictionary_create(comparefunc c, destroyfunc d, void *extra);

/* Assumes d is not NULL. */
void dictionary_destroy(struct dictionary *d);

/* Returns how many entries have been stored in the dictionary.  If two values
 * with the same key are inserted, only one is counted.
 */
UINT dictionary_num_entries(struct dictionary *d);

/* Sets an element with key k and value v to the dictionary.  If a value
 * already exists with key k, its value is replaced, and the destroyfunc (if
 * set) is called for the previous item.
 * Assumes k and v can be bitwise-copied.
 * Both k and v are allowed to be NULL, in case you want to use integer
 * values for either the key or the value.
 * Assumes d is not NULL.
 */
void dictionary_insert(struct dictionary *d, const void *k, const void *v);

/* If a value with key k has been inserted into the dictionary, *v is set
 * to its associated value.  Returns FALSE if the key is not found, and TRUE
 * if it is.  *v is undefined if it returns FALSE.  (It is not set to NULL,
 * because this dictionary doesn't prevent you from using NULL as a value
 * value; see dictionary_insert.)
 * Assumes d and v are not NULL.
 */
BOOL dictionary_find(struct dictionary *d, const void *k, void **v);

/* Removes the element with key k from the dictionary.  Calls the destroyfunc
 * for the dictionary with the element if found (so you may destroy it if it's
 * dynamically allocated.)
 * Assumes d is not NULL.
 */
void dictionary_remove(struct dictionary *d, const void *k);

void dictionary_enumerate(struct dictionary *d, enumeratefunc e, void *closure);

#endif /* ndef __DICTIONARY_H__ */
