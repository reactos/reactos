/*
 * dlls/rsaenh/handle.h
 * Support code to manage HANDLE tables.
 *
 * Copyright 1998 Alexandre Julliard
 * Copyright 2002-2004 Mike McCormack for CodeWeavers
 * Copyright 2004 Michael Jung
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

#ifndef __WINE_HANDLE_H
#define __WINE_HANDLE_H

#include "wincrypt.h"

#ifdef __cplusplus
extern "C" {
#endif

#define TABLE_SIZE_INCREMENT 32

struct tagOBJECTHDR;
typedef struct tagOBJECTHDR OBJECTHDR;
typedef void (*DESTRUCTOR)(OBJECTHDR *object);
struct tagOBJECTHDR
{
    DWORD       dwType;
    LONG        refcount;
    DESTRUCTOR  destructor;
};

typedef struct tagHANDLETABLEENTRY
{
    OBJECTHDR    *pObject;
    unsigned int iNextFree;
} HANDLETABLEENTRY;

/* Prevent conflict with wingdi.h */
#define tagHANDLETABLE tagHANDLETABLE_RSA
#define HANDLETABLE HANDLETABLE_RSA

typedef struct tagHANDLETABLE
{
    unsigned int     iEntries;
    unsigned int     iFirstFree;
    HANDLETABLEENTRY *paEntries;
    CRITICAL_SECTION mutex;
} HANDLETABLE;

int  alloc_handle_table  (HANDLETABLE **lplpTable);
void init_handle_table   (HANDLETABLE *lpTable);
int  release_handle_table(HANDLETABLE *lpTable);
void destroy_handle_table(HANDLETABLE *lpTable);
int  release_handle      (HANDLETABLE *lpTable, HCRYPTKEY handle, DWORD dwType);
int  copy_handle         (HANDLETABLE *lpTable, HCRYPTKEY handle, DWORD dwType, HCRYPTKEY *copy);
int  lookup_handle       (HANDLETABLE *lpTable, HCRYPTKEY handle, DWORD dwType, OBJECTHDR **lplpObject);
int  is_valid_handle     (HANDLETABLE *lpTable, HCRYPTKEY handle, DWORD dwType);

HCRYPTKEY new_object     (HANDLETABLE *lpTable, size_t cbSize, DWORD dwType, DESTRUCTOR destructor,
                           OBJECTHDR **ppObject);
        
#ifdef __cplusplus
}
#endif

#endif /* __WINE_HANDLE_H */
