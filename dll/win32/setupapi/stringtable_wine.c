/*
 * Setupapi string table functions
 *
 * Copyright 2005 Eric Kohl
 * Copyright 2014 Nikolay Sivov for CodeWeavers
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

#include "setupapi_private.h"
#include <winnls.h>


#define _PSETUP(func)   pSetup ## func

#define StringTableInitialize       _PSETUP(StringTableInitialize)
#define StringTableInitializeEx     _PSETUP(StringTableInitializeEx)
#define StringTableDestroy          _PSETUP(StringTableDestroy)
#define StringTableAddString        _PSETUP(StringTableAddString)
#define StringTableAddStringEx      _PSETUP(StringTableAddStringEx)
#define StringTableDuplicate        _PSETUP(StringTableDuplicate)
#define StringTableGetExtraData     _PSETUP(StringTableGetExtraData)
#define StringTableLookUpString     _PSETUP(StringTableLookUpString)
#define StringTableLookUpStringEx   _PSETUP(StringTableLookUpStringEx)
#define StringTableSetExtraData     _PSETUP(StringTableSetExtraData)
#define StringTableStringFromId     _PSETUP(StringTableStringFromId)
#define StringTableStringFromIdEx   _PSETUP(StringTableStringFromIdEx)
#define StringTableTrim             _PSETUP(StringTableTrim)


struct stringtable {
    char     *data;
    ULONG     nextoffset;
    ULONG     allocated;
    DWORD_PTR unk[2];
    ULONG     max_extra_size;
    LCID      lcid;
};

struct stringentry {
    DWORD nextoffset;
    WCHAR data[1];
};

#define BUCKET_COUNT 509
#define DEFAULT_ALLOC_SIZE 4096

/*
     String table details

 Returned string table 'handle' is a pointer to 'struct stringtable' structure.
 Data itself is allocated separately, pointer is stored in 'data' field.

 Data starts with array of 509 DWORDs - lookup table. Initially all offsets in that
 array are set to -1. Right after lookup table goes data itself, stored in linked lists.
 Lookup table offset points to first record of 'struct stringentry' type. When more
 than one record is present in a bucket, first record links to next one with 'nextoffset'
 field. Last record has nextoffset == -1, same when there's only one record. String data
 is placed right after offset, and is followed by extra data. Each record has reserved
 'max_extra_size' bytes to store extra data, it's not compacted in any way.

 A simple hash function is used to determine which bucket a given string belongs to (see below).

 All offsets including returned string ids are relative to 'data' pointer. When table
 needs to grow 'allocated' size is doubled, but offsets are always valid and preserved.

*/

static inline DWORD get_string_hash(const WCHAR *str, BOOL case_sensitive)
{
    DWORD hash = 0;

    while (*str) {
        WCHAR ch = case_sensitive ? *str : towlower(*str);
        hash += ch;
        if (ch & ~0xff)
            hash |= 1;
        str++;
    }

    return hash % BUCKET_COUNT;
}

static inline DWORD *get_bucket_ptr(struct stringtable *table, const WCHAR *string, BOOL case_sensitive)
{
    DWORD hash = get_string_hash(string, case_sensitive);
    return (DWORD*)(table->data + hash*sizeof(DWORD));
}

static inline WCHAR *get_string_ptr(struct stringtable *table, DWORD id)
{
    return (WCHAR*)(table->data + id + sizeof(DWORD));
}

static inline char *get_extradata_ptr(struct stringtable *table, DWORD id)
{
    WCHAR *ptrW = get_string_ptr(table, id);
    /* skip string itself */
    return (char*)(ptrW + lstrlenW(ptrW) + 1);
}

static inline BOOL is_valid_string_id(struct stringtable *table, DWORD id)
{
    return (id >= BUCKET_COUNT*sizeof(DWORD)) && (id < table->allocated);
}

static inline int get_aligned16_size(int size)
{
    return (size + 15) & ~15;
}

/**************************************************************************
 * StringTableInitializeEx [SETUPAPI.@]
 *
 * Creates a new string table and initializes it.
 *
 * PARAMS
 *     max_extra_size   [I] Maximum extra data size
 *     reserved         [I] Unused
 *
 * RETURNS
 *     Success: Handle to the string table
 *     Failure: NULL
 */
HSTRING_TABLE WINAPI StringTableInitializeEx(ULONG max_extra_size, DWORD reserved)
{
    struct stringtable *table;

    TRACE("(%ld %lx)\n", max_extra_size, reserved);

    table = MyMalloc(sizeof(*table));
    if (!table) return NULL;

    table->allocated = get_aligned16_size(BUCKET_COUNT*sizeof(DWORD) + DEFAULT_ALLOC_SIZE);
    table->data = MyMalloc(table->allocated);
    if (!table->data) {
        MyFree(table);
        return NULL;
    }

    table->nextoffset = BUCKET_COUNT*sizeof(DWORD);
    /* FIXME: actually these two are not zero */
    table->unk[0] = table->unk[1] = 0;
    table->max_extra_size = max_extra_size;
    table->lcid = GetThreadLocale();

    /* bucket area is filled with 0xff, actual string data area is zeroed */
    memset(table->data, 0xff, table->nextoffset);
    memset(table->data + table->nextoffset, 0, table->allocated - table->nextoffset);

    return (HSTRING_TABLE)table;
}

/**************************************************************************
 * StringTableInitialize [SETUPAPI.@]
 *
 * Creates a new string table and initializes it.
 *
 * PARAMS
 *     None
 *
 * RETURNS
 *     Success: Handle to the string table
 *     Failure: NULL
 */
HSTRING_TABLE WINAPI StringTableInitialize(void)
{
    return StringTableInitializeEx(0, 0);
}

/**************************************************************************
 * StringTableDestroy [SETUPAPI.@]
 *
 * Destroys a string table.
 *
 * PARAMS
 *     hTable [I] Handle to the string table to be destroyed
 *
 * RETURNS
 *     None
 */
void WINAPI StringTableDestroy(HSTRING_TABLE hTable)
{
    struct stringtable *table = (struct stringtable*)hTable;

    TRACE("%p\n", table);

    if (!table)
        return;

    MyFree(table->data);
    MyFree(table);
}

/**************************************************************************
 * StringTableDuplicate [SETUPAPI.@]
 *
 * Duplicates a given string table.
 *
 * PARAMS
 *     hTable [I] Handle to the string table
 *
 * RETURNS
 *     Success: Handle to the duplicated string table
 *     Failure: NULL
 *
 */
HSTRING_TABLE WINAPI StringTableDuplicate(HSTRING_TABLE hTable)
{
    struct stringtable *src = (struct stringtable*)hTable, *dest;

    TRACE("%p\n", src);

    if (!src)
        return NULL;

    dest = MyMalloc(sizeof(*dest));
    if (!dest)
        return NULL;

    *dest = *src;
    dest->data = MyMalloc(src->allocated);
    if (!dest->data) {
        MyFree(dest);
        return NULL;
    }

    memcpy(dest->data, src->data, src->allocated);
    return (HSTRING_TABLE)dest;
}

/**************************************************************************
 * StringTableGetExtraData [SETUPAPI.@]
 *
 * Retrieves extra data from a given string table entry.
 *
 * PARAMS
 *     hTable     [I] Handle to the string table
 *     id         [I] String ID
 *     extra      [I] Pointer a buffer that receives the extra data
 *     extra_size [I] Size of the buffer
 *
 * RETURNS
 *     Success: TRUE
 *     Failure: FALSE
 */
BOOL WINAPI StringTableGetExtraData(HSTRING_TABLE hTable, ULONG id, void *extra, ULONG extra_size)
{
    struct stringtable *table = (struct stringtable*)hTable;
    char *extraptr;

    TRACE("%p %lu %p %lu\n", table, id, extra, extra_size);

    if (!table)
        return FALSE;

    if (!is_valid_string_id(table, id))
        return FALSE;

    if (table->max_extra_size > extra_size)
    {
        ERR("data size is too large\n");
        return FALSE;
    }

    extraptr = get_extradata_ptr(table, id);
    memcpy(extra, extraptr, extra_size);
    return TRUE;
}

/**************************************************************************
 * StringTableLookUpStringEx [SETUPAPI.@]
 *
 * Searches a string table and extra data for a given string.
 *
 * PARAMS
 *     hTable      [I] Handle to the string table
 *     string      [I] String to be searched for
 *     flags       [I] Flags
 *                        1: case sensitive compare
 *     extra       [O] Pointer to the buffer that receives the extra data
 *     extra_size  [I/O] Unused
 *
 * RETURNS
 *     Success: String ID
 *     Failure: -1
 */
DWORD WINAPI StringTableLookUpStringEx(HSTRING_TABLE hTable, LPWSTR string, DWORD flags,
    void *extra, ULONG extra_size)
{
    struct stringtable *table = (struct stringtable*)hTable;
    BOOL case_sensitive = flags & 1;
    struct stringentry *entry;
    DWORD offset;
    int cmp;

    TRACE("%p->%p %s %lx %p, %lx\n", table, table->data, debugstr_w(string), flags, extra, extra_size);

    if (!table)
        return -1;

    /* get corresponding offset */
    offset = *get_bucket_ptr(table, string, case_sensitive);
    if (offset == -1)
        return -1;

    /* now we're at correct bucket, do linear search for string */
    while (1) {
        entry = (struct stringentry*)(table->data + offset);
        if (case_sensitive)
            cmp = wcscmp(entry->data, string);
        else
            cmp = lstrcmpiW(entry->data, string);
        if (!cmp) {
            if (extra)
                memcpy(extra, get_extradata_ptr(table, offset), extra_size);
            return offset;
        }

        /* last entry */
        if (entry->nextoffset == -1)
            return -1;

        offset = entry->nextoffset;
        if (offset > table->allocated)
            return -1;
    }
}

/**************************************************************************
 * StringTableLookUpString [SETUPAPI.@]
 *
 * Searches a string table for a given string.
 *
 * PARAMS
 *     hTable  [I] Handle to the string table
 *     string  [I] String to be searched for
 *     flags   [I] Flags
 *                 1: case sensitive compare
 *
 * RETURNS
 *     Success: String ID
 *     Failure: -1
 */
DWORD WINAPI StringTableLookUpString(HSTRING_TABLE hTable, LPWSTR string, DWORD flags)
{
    return StringTableLookUpStringEx(hTable, string, flags, NULL, 0);
}

/**************************************************************************
 * StringTableAddStringEx [SETUPAPI.@]
 *
 * Adds a new string plus extra data to the string table.
 *
 * PARAMS
 *     hTable        [I] Handle to the string table
 *     string        [I] String to be added to the string table
 *     flags         [I] Flags
 *                           1: case sensitive compare
 *     extra         [I] Pointer to the extra data
 *     extra_size    [I] Size of the extra data
 *
 * RETURNS
 *     Success: String ID
 *     Failure: -1
 *
 * NOTES
 *     If the given string already exists in the string table it will not
 *     be added again. The ID of the existing string will be returned in
 *     this case.
 */
DWORD WINAPI StringTableAddStringEx(HSTRING_TABLE hTable, LPWSTR string,
                       DWORD flags, void *extra, DWORD extra_size)
{
    struct stringtable *table = (struct stringtable*)hTable;
    BOOL case_sensitive = flags & 1;
    struct stringentry *entry;
    DWORD id, *offset;
    WCHAR *ptrW;
    int len;

    TRACE("%p %s %lx %p, %lu\n", hTable, debugstr_w(string), flags, extra, extra_size);

    if (!table)
        return -1;

    id = StringTableLookUpStringEx(hTable, string, flags, NULL, 0);
    if (id != -1)
        return id;

    /* needed space for new record */
    len = sizeof(DWORD) + (lstrlenW(string)+1)*sizeof(WCHAR) + table->max_extra_size;
    if (table->nextoffset + len >= table->allocated) {
        table->allocated <<= 1;
        table->data = _recalloc(table->data, 1, table->allocated);
    }

    /* hash string */
    offset = get_bucket_ptr(table, string, case_sensitive);
    if (*offset == -1)
        /* bucket used for a very first time */
        *offset = table->nextoffset;
    else {
        entry = (struct stringentry*)(table->data + *offset);
        /* link existing last entry to newly added */
        while (entry->nextoffset != -1)
            entry = (struct stringentry*)(table->data + entry->nextoffset);
        entry->nextoffset = table->nextoffset;
    }
    entry = (struct stringentry*)(table->data + table->nextoffset);
    entry->nextoffset = -1;
    id = table->nextoffset;

    /* copy string */
    ptrW = get_string_ptr(table, id);
    lstrcpyW(ptrW, string);
    if (!case_sensitive)
        wcslwr(ptrW);

    /* copy extra data */
    if (extra)
        memcpy(get_extradata_ptr(table, id), extra, extra_size);

    table->nextoffset += len;
    return id;
}

/**************************************************************************
 * StringTableAddString [SETUPAPI.@]
 *
 * Adds a new string to the string table.
 *
 * PARAMS
 *     hTable     [I] Handle to the string table
 *     string     [I] String to be added to the string table
 *     flags      [I] Flags
 *                        1: case sensitive compare
 *
 * RETURNS
 *     Success: String ID
 *     Failure: -1
 *
 * NOTES
 *     If the given string already exists in the string table it will not
 *     be added again. The ID of the existing string will be returned in
 *     this case.
 */
DWORD WINAPI StringTableAddString(HSTRING_TABLE hTable, LPWSTR string, DWORD flags)
{
    return StringTableAddStringEx(hTable, string, flags, NULL, 0);
}

/**************************************************************************
 * StringTableSetExtraData [SETUPAPI.@]
 *
 * Sets extra data for a given string table entry.
 *
 * PARAMS
 *     hTable     [I] Handle to the string table
 *     id         [I] String ID
 *     extra      [I] Pointer to the extra data
 *     extra_size [I] Size of the extra data
 *
 * RETURNS
 *     Success: TRUE
 *     Failure: FALSE
 */
BOOL WINAPI StringTableSetExtraData(HSTRING_TABLE hTable, DWORD id, void *extra, ULONG extra_size)
{
    struct stringtable *table = (struct stringtable*)hTable;
    char *extraptr;

    TRACE("%p %ld %p %lu\n", hTable, id, extra, extra_size);

    if (!table)
        return FALSE;

    if (!is_valid_string_id(table, id))
        return FALSE;

    if (table->max_extra_size < extra_size)
    {
        ERR("data size is too large\n");
        return FALSE;
    }

    extraptr = get_extradata_ptr(table, id);
    memset(extraptr, 0, table->max_extra_size);
    memcpy(extraptr, extra, extra_size);

    return TRUE;
}

/**************************************************************************
 * StringTableStringFromId [SETUPAPI.@]
 *
 * Returns a pointer to a string for the given string ID.
 *
 * PARAMS
 *     hTable [I] Handle to the string table.
 *     id     [I] String ID
 *
 * RETURNS
 *     Success: Pointer to the string
 *     Failure: NULL
 */
LPWSTR WINAPI StringTableStringFromId(HSTRING_TABLE hTable, ULONG id)
{
    struct stringtable *table = (struct stringtable*)hTable;
    static WCHAR empty[] = {0};

    TRACE("%p %ld\n", table, id);

    if (!table)
        return NULL;

    if (!is_valid_string_id(table, id))
        return empty;

    return get_string_ptr(table, id);
}

/**************************************************************************
 * StringTableStringFromIdEx [SETUPAPI.@]
 *
 * Returns a string for the given string ID.
 *
 * PARAMS
 *     hTable   [I] Handle to the string table
 *     id       [I] String ID
 *     buff     [I] Pointer to string buffer
 *     buflen   [I/O] Pointer to the size of the string buffer
 *
 * RETURNS
 *     Success: TRUE
 *     Failure: FALSE
 */
BOOL WINAPI StringTableStringFromIdEx(HSTRING_TABLE hTable, ULONG id, LPWSTR buff, DWORD *buflen)
{
    struct stringtable *table = (struct stringtable*)hTable;
    BOOL ret = TRUE;
    WCHAR *ptrW;
    int len;

    TRACE("%p %lx %p %p\n", table, id, buff, buflen);

    if (!table) {
        *buflen = 0;
        return FALSE;
    }

    if (!is_valid_string_id(table, id)) {
        WARN("invalid string id\n");
        *buflen = 0;
        return FALSE;
    }

    ptrW = get_string_ptr(table, id);
    len = (lstrlenW(ptrW) + 1)*sizeof(WCHAR);
    if (len <= *buflen)
        lstrcpyW(buff, ptrW);
    else
        ret = FALSE;

    *buflen = len;
    return ret;
}

/**************************************************************************
 * StringTableTrim [SETUPAPI.@]
 *
 * ...
 *
 * PARAMS
 *     hTable [I] Handle to the string table
 *
 * RETURNS
 *     None
 */
void WINAPI StringTableTrim(HSTRING_TABLE hTable)
{
    FIXME("%p\n", hTable);
}
