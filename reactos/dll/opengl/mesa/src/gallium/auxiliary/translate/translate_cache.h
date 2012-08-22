/*
 * Copyright 2008 Tungsten Graphics, inc.
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * on the rights to use, copy, modify, merge, publish, distribute, sub
 * license, and/or sell copies of the Software, and to permit persons to whom
 * the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.  IN NO EVENT SHALL
 * TUNGSTEN GRAPHICS AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#ifndef _TRANSLATE_CACHE_H
#define _TRANSLATE_CACHE_H


/*******************************************************************************
 * Translate cache.
 * Simply used to cache created translates. Avoids unecessary creation of
 * translate's if one suitable for a given translate_key has already been
 * created.
 *
 * Note: this functionality depends and requires the CSO module.
 */
struct translate_cache;

struct translate_key;
struct translate;

struct translate_cache *translate_cache_create( void );
void translate_cache_destroy(struct translate_cache *cache);

/**
 * Will try to find a translate structure matched by the given key.
 * If such a structure doesn't exist in the cache the function
 * will automatically create it, insert it in the cache and
 * return the created version.
 *
 */
struct translate *translate_cache_find(struct translate_cache *cache,
                                       struct translate_key *key);

#endif
