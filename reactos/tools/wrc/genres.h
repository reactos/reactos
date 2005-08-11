/*
 * Generate resource prototypes
 *
 * Copyright 1998 Bertho A. Stultiens (BS)
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __WRC_GENRES_H
#define __WRC_GENRES_H

#include "wrctypes.h"

res_t *new_res(void);
res_t *grow_res(res_t *r, unsigned int add);
void put_byte(res_t *res, unsigned c);
void put_word(res_t *res, unsigned w);
void put_dword(res_t *res, unsigned d);
void resources2res(resource_t *top);
const char *get_c_typename(enum res_e type);
char *make_c_name(const char *base, const name_id_t *nid, const language_t *lan);
char *prep_nid_for_label(const name_id_t *nid);

#endif
