/*
 * Copyright 2013 Mislav Blažević
 * Copyright 2015-2017 Mark Jansen (mark.jansen@reactos.org)
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

#ifndef SDBTYPES_H
#define SDBTYPES_H

#ifdef __cplusplus
extern "C" {
#endif

typedef WORD TAG;
typedef DWORD TAGID;
typedef DWORD TAGREF;
typedef UINT64 QWORD;

#define TAGREF_NULL (0)
#define TAGREF_ROOT (0)

typedef struct _DB {
    HANDLE file;
    DWORD size;
    BYTE* data;
    TAGID stringtable;
    DWORD write_iter;
    GUID database_id;
    BOOL for_write;
    struct SdbStringHashTable* string_lookup;
    struct _DB* string_buffer;
} DB, *PDB;

typedef enum _PATH_TYPE {
    DOS_PATH,
    NT_PATH
} PATH_TYPE;


#ifdef __cplusplus
} // extern "C"
#endif

#endif // SDBTYPES_H
