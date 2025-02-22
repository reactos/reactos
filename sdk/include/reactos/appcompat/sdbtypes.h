/*
 * PROJECT:     ReactOS Application compatibility module
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Sdb core definitions
 * COPYRIGHT:   Copyright 2013 Mislav Blažević
 *              Copyright 2015-2019 Mark Jansen (mark.jansen@reactos.org)
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
    DWORD major;
    DWORD minor;
    GUID database_id;
    PCWSTR database_name;
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
