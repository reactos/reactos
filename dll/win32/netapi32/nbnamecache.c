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
 *
 * This implementation uses a linked list, because I don't have a decent
 * hash table implementation handy.  This is somewhat inefficient, but it's
 * rather more efficient than not having a name cache at all.
 */

#include "config.h"
#include "wine/port.h"

#include "nbnamecache.h"

typedef struct _NBNameCacheNode
{
    DWORD expireTime;
    NBNameCacheEntry *entry;
    struct _NBNameCacheNode *next;
} NBNameCacheNode;

struct NBNameCache
{
    HANDLE heap;
    CRITICAL_SECTION cs;
    DWORD entryExpireTimeMS;
    NBNameCacheNode *head;
};

/* Unlinks the node pointed to by *prev, and frees any associated memory.
 * If that node's next pointed to another node, *prev now points to it.
 * Assumes the caller owns cache's lock.
 */
static void NBNameCacheUnlinkNode(struct NBNameCache *cache,
 NBNameCacheNode **prev)
{
    if (cache && prev && *prev)
    {
        NBNameCacheNode *next = (*prev)->next;

        HeapFree(cache->heap, 0, (*prev)->entry);
        HeapFree(cache->heap, 0, *prev);
        *prev = next;
    }
}

/* Walks the list beginning with cache->head looking for the node with name
 * name.  If the node is found, returns a pointer to the next pointer of the
 * node _prior_ to the found node (or head if head points to it).  Thus, if the
 * node's all you want, dereference the return value twice.  If you want to
 * modify the list, modify the referent of the return value.
 * While it's at it, deletes nodes whose time has expired (except the node
 * you're looking for, of course).
 * Returns NULL if the node isn't found.
 * Assumes the caller owns cache's lock.
 */
static NBNameCacheNode **NBNameCacheWalk(struct NBNameCache *cache,
 const char name[NCBNAMSZ])
{
    NBNameCacheNode **ret = NULL;

    if (cache && cache->head)
    {
        NBNameCacheNode **ptr;

        ptr = &cache->head;
        while (ptr && *ptr && (*ptr)->entry)
        {
            if (!memcmp((*ptr)->entry->name, name, NCBNAMSZ - 1))
                ret = ptr;
            else
            {
                if (GetTickCount() > (*ptr)->expireTime)
                    NBNameCacheUnlinkNode(cache, ptr);
            }
            if (*ptr)
                ptr = &(*ptr)->next;
        }
    }
    return ret;
}

struct NBNameCache *NBNameCacheCreate(HANDLE heap, DWORD entryExpireTimeMS)
{
    struct NBNameCache *cache;
    
    
    if (!heap)
        heap = GetProcessHeap();
    cache = HeapAlloc(heap, 0, sizeof(struct NBNameCache));
    if (cache)
    {
        cache->heap = heap;
        InitializeCriticalSection(&cache->cs);
        cache->cs.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": NBNameCache.cs");
        cache->entryExpireTimeMS = entryExpireTimeMS;
        cache->head = NULL;
    }
    return cache;
}

BOOL NBNameCacheAddEntry(struct NBNameCache *cache, NBNameCacheEntry *entry)
{
    BOOL ret;

    if (cache && entry)
    {
        NBNameCacheNode **node;

        EnterCriticalSection(&cache->cs);
        node = NBNameCacheWalk(cache, (char*)entry->name);
        if (node)
        {
            (*node)->expireTime = GetTickCount() +
             cache->entryExpireTimeMS;
            HeapFree(cache->heap, 0, (*node)->entry);
            (*node)->entry = entry;
            ret = TRUE;
        }
        else
        {
            NBNameCacheNode *newNode = HeapAlloc(cache->heap, 0, sizeof(NBNameCacheNode));
            if (newNode)
            {
                newNode->expireTime = GetTickCount() +
                 cache->entryExpireTimeMS;
                newNode->entry = entry;
                newNode->next = cache->head;
                cache->head = newNode;
                ret = TRUE;
            }
            else
                ret = FALSE;
        }
        LeaveCriticalSection(&cache->cs);
    }
    else
        ret = FALSE;
    return ret;
}

const NBNameCacheEntry *NBNameCacheFindEntry(struct NBNameCache *cache,
 const UCHAR name[NCBNAMSZ])
{
    const NBNameCacheEntry *ret;
    UCHAR printName[NCBNAMSZ];

    memcpy(printName, name, NCBNAMSZ - 1);
    printName[NCBNAMSZ - 1] = '\0';
    if (cache)
    {
        NBNameCacheNode **node;

        EnterCriticalSection(&cache->cs);
        node = NBNameCacheWalk(cache, (const char *)name);
        if (node)
            ret = (*node)->entry;
        else
            ret = NULL;
        LeaveCriticalSection(&cache->cs);
    }
    else
        ret = NULL;
    return ret;
}

void NBNameCacheDestroy(struct NBNameCache *cache)
{
    if (cache)
    {
        cache->cs.DebugInfo->Spare[0] = 0;
        DeleteCriticalSection(&cache->cs);
        while (cache->head)
            NBNameCacheUnlinkNode(cache, &cache->head);
        HeapFree(cache->heap, 0, cache);
    }
}
