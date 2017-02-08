/**************************************************************************
 *
 * Copyright 2010 Luca Barbieri
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial
 * portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE COPYRIGHT OWNER(S) AND/OR ITS SUPPLIERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/

#ifndef U_LINKAGE_H_
#define U_LINKAGE_H_

#include "pipe/p_compiler.h"
#include "pipe/p_shader_tokens.h"

struct util_semantic_set
{
   unsigned long masks[256 / 8 / sizeof(unsigned long)];
};

static INLINE boolean
util_semantic_set_contains(struct util_semantic_set *set, unsigned char value)
{
   return !!(set->masks[value / (sizeof(long) * 8)] & (1 << (value / (sizeof(long) * 8))));
}

unsigned util_semantic_set_from_program_file(struct util_semantic_set *set, const struct tgsi_token *tokens, enum tgsi_file_type file);

/* efficient_slots is the number of slots such that hardware performance is
 * the same for using that amount, with holes, or less slots but with less
 * holes.
 *
 * num_slots is the size of the layout array and hardware limit instead.
 *
 * efficient_slots == 0 or efficient_slots == num_slots are typical settings.
 */
void util_semantic_layout_from_set(unsigned char *layout, const struct util_semantic_set *set, unsigned efficient_slots, unsigned num_slots);

static INLINE void
util_semantic_table_from_layout(unsigned char *table, size_t table_size, unsigned char *layout,
                                unsigned char first_slot_value, unsigned char num_slots)
{
   unsigned char i;
   memset(table, 0xff, table_size);

   for(i = 0; i < num_slots; ++i)
      table[layout[i]] = first_slot_value + i;
}

#endif /* U_LINKAGE_H_ */
