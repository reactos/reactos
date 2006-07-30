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

typedef enum tag_syskind_t {
    SYS_WIN16 = 0,
    SYS_WIN32,
    SYS_MAC
} syskind_t;

extern unsigned long lhash_val_of_name_sys( syskind_t skind, LCID lcid, LPCSTR lpStr);

#endif
