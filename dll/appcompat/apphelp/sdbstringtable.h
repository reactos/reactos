/*
 * Copyright 2016 Mark Jansen
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
