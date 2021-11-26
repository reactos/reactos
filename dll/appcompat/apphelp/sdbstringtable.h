/*
 * PROJECT:     ReactOS Application compatibility module
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Shim database string table interface
 * COPYRIGHT:   Copyright 2016 Mark Jansen (mark.jansen@reactos.org)
 */

#ifndef SDBSTRINGTABLE_H
#define SDBSTRINGTABLE_H

#ifdef __cplusplus
extern "C" {
#endif


/**
 * Destroy the hashtable and release all resources.
 *
 * @param [in]  table       Pointer to table pointer, will be cleared after use
 *
 */
void SdbpTableDestroy(struct SdbStringHashTable* * table);

/**
 * Find an entry in the stringtable, or allocate it when an entry could not be found.
 * - When the string specified does not yet exist, a new entry will be added to the table,
 *   and the pTagid specified will be associated with this string.
 * - When the string specified does already exist,
 *   the TAGID associated with this string will be returned in pTagid.
 *
 *
 * @param [in]  table       Pointer to table pointer, will be allocated when needed.
 * @param [in]  str         The string to search for
 * @param [in,out] pTagid
 *                          the data written (in bytes)
 *
 * @return  TRUE if the string was added to the table, FALSE if it already existed
 */
BOOL SdbpAddStringToTable(struct SdbStringHashTable* * table, const WCHAR* str, TAGID* pTagid);


#ifdef __cplusplus
} // extern "C"
#endif

#endif // SDBSTRINGTABLE_H
