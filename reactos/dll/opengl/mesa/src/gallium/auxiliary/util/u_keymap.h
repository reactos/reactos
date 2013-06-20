/**************************************************************************
 * 
 * Copyright 2008 Tungsten Graphics, Inc., Cedar Park, Texas.
 * All Rights Reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 * 
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL TUNGSTEN GRAPHICS AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 * 
 **************************************************************************/

#ifndef U_KEYMAP_H
#define U_KEYMAP_H

#include "pipe/p_compiler.h"


/** opaque keymap type */
struct keymap;


/** Delete/callback function type */
typedef void (*keymap_delete_func)(const struct keymap *map,
                                   const void *key, void *data,
                                   void *user);


extern struct keymap *
util_new_keymap(unsigned keySize, unsigned maxEntries,
                keymap_delete_func deleteFunc);

extern void
util_delete_keymap(struct keymap *map, void *user);

extern boolean
util_keymap_insert(struct keymap *map, const void *key,
                   const void *data, void *user);

extern const void *
util_keymap_lookup(const struct keymap *map, const void *key);

extern void
util_keymap_remove(struct keymap *map, const void *key, void *user);

extern void
util_keymap_remove_all(struct keymap *map, void *user);

extern void
util_keymap_info(const struct keymap *map);


#endif /* U_KEYMAP_H */
