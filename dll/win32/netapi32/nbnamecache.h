/* Copyright (c) 2003 Juan Lang
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
#ifndef __WINE_NBNAMECACHE_H
#define __WINE_NBNAMECACHE_H

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "nb30.h"

struct NBNameCache;

/* Represents an entry in the name cache.  If the NetBIOS name is known, it's
 * in nbname.  Otherwise, nbname begins with '*'.  numAddresses defines the
 * number of addresses in addresses.
 * Notice that it allows multiple addresses per name, but doesn't explicitly
 * allow group names.  That's because all names so far are unique; if a use for
 * group names comes up, adding a flag here is simple enough.
 * Also, only the first NCBNAMSZ - 1 bytes are considered significant.  This is
 * because a name may have been resolved using DNS, and the suffix byte is
 * always truncated for DNS lookups.
 */
typedef struct _NBNameCacheEntry
{
    UCHAR name[NCBNAMSZ];
    UCHAR nbname[NCBNAMSZ];
    DWORD numAddresses;
    DWORD addresses[1];
} NBNameCacheEntry;

/* Functions that create, manipulate, and destroy a name cache.  Thread-safe,
 * with the exception of NBNameCacheDestroy--ensure that no other threads are
 * manipulating the cache before destroying it.
 */

/* Allocates a new name cache from heap, and sets the expire time on new
 * entries to entryExpireTimeMS after a cache entry is added.
 */
struct NBNameCache *NBNameCacheCreate(HANDLE heap, DWORD entryExpireTimeMS);

/* Adds an entry to the cache.  The entry is assumed to have been allocated
 * from the same heap as the name cache; the name cache will own the entry
 * from now on.  The entry's expire time is initialized at this time to
 * entryExpireTimeMS + the current time in MS.  If an existing entry with the
 * same name was in the cache, the entry is replaced.  Returns TRUE on success
 * or FALSE on failure.
 */
BOOL NBNameCacheAddEntry(struct NBNameCache *cache, NBNameCacheEntry *entry);

/* Finds the entry with name name in the cache and returns a pointer to it, or
 * NULL if it isn't found.
 */
const NBNameCacheEntry *NBNameCacheFindEntry(struct NBNameCache *cache,
 const UCHAR name[NCBNAMSZ]);

void NBNameCacheDestroy(struct NBNameCache *cache);

#endif /* ndef __WINE_NBNAMECACHE_H */
