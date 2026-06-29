/*
 * Hash definitions
 *
 * Copyright 2005 Huw Davies
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
 *
 */

#ifndef __WIDL_HASH_H
#define __WIDL_HASH_H

extern unsigned int lhash_val_of_name_sys( syskind_t skind, int lcid, const char *lpStr);

struct sha1_context
{
   unsigned int state[5];
   unsigned int count[2];
   char         buffer[64];
};

void sha1_init(struct sha1_context *ctx);
void sha1_update(struct sha1_context *ctx, const char *data, size_t data_size);
void sha1_finalize(struct sha1_context *ctx, unsigned int hash[5]);

#endif
