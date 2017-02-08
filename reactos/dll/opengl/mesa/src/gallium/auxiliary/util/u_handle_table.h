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

/**
 * @file
 * Generic handle table.
 *  
 * @author José Fonseca <jrfonseca@tungstengraphics.com>
 */

#ifndef U_HANDLE_TABLE_H_
#define U_HANDLE_TABLE_H_


#ifdef __cplusplus
extern "C" {
#endif

   
/**
 * Abstract data type to map integer handles to objects.
 * 
 * Also referred as "pointer array".
 */
struct handle_table;


struct handle_table *
handle_table_create(void);


/**
 * Set an optional destructor callback.
 * 
 * If set, it will be called during handle_table_remove and 
 * handle_table_destroy calls.
 */
void
handle_table_set_destroy(struct handle_table *ht,
                         void (*destroy)(void *object));


/**
 * Add a new object.
 * 
 * Returns a zero handle on failure (out of memory).
 */
unsigned
handle_table_add(struct handle_table *ht, 
                 void *object);

/**
 * Returns zero on failure (out of memory).
 */
unsigned
handle_table_set(struct handle_table *ht, 
                 unsigned handle,
                 void *object);

/**
 * Fetch an existing object.
 * 
 * Returns NULL for an invalid handle.
 */
void *
handle_table_get(struct handle_table *ht, 
                 unsigned handle);


void
handle_table_remove(struct handle_table *ht, 
                    unsigned handle);


void
handle_table_destroy(struct handle_table *ht);


unsigned
handle_table_get_first_handle(struct handle_table *ht);


unsigned
handle_table_get_next_handle(struct handle_table *ht,
                             unsigned handle);


#ifdef __cplusplus
}
#endif

#endif /* U_HANDLE_TABLE_H_ */
