/* xf86drmHash.c -- Small hash table support for integer -> integer mapping
 * Created: Sun Apr 18 09:35:45 1999 by faith@precisioninsight.com
 *
 * Copyright 1999 Precision Insight, Inc., Cedar Park, Texas.
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * PRECISION INSIGHT AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Authors: Rickard E. (Rik) Faith <faith@valinux.com>
 *
 * $XFree86: xc/programs/Xserver/hw/xfree86/os-support/linux/drm/xf86drmHash.c,v 1.4 2001/03/21 18:08:54 dawes Exp $
 *
 * DESCRIPTION
 *
 * This file contains a straightforward implementation of a fixed-sized
 * hash table using self-organizing linked lists [Knuth73, pp. 398-399] for
 * collision resolution.  There are two potentially interesting things
 * about this implementation:
 *
 * 1) The table is power-of-two sized.  Prime sized tables are more
 * traditional, but do not have a significant advantage over power-of-two
 * sized table, especially when double hashing is not used for collision
 * resolution.
 *
 * 2) The hash computation uses a table of random integers [Hanson97,
 * pp. 39-41].
 *
 * FUTURE ENHANCEMENTS
 *
 * With a table size of 512, the current implementation is sufficient for a
 * few hundred keys.  Since this is well above the expected size of the
 * tables for which this implementation was designed, the implementation of
 * dynamic hash tables was postponed until the need arises.  A common (and
 * naive) approach to dynamic hash table implementation simply creates a
 * new hash table when necessary, rehashes all the data into the new table,
 * and destroys the old table.  The approach in [Larson88] is superior in
 * two ways: 1) only a portion of the table is expanded when needed,
 * distributing the expansion cost over several insertions, and 2) portions
 * of the table can be locked, enabling a scalable thread-safe
 * implementation.
 *
 * REFERENCES
 *
 * [Hanson97] David R. Hanson.  C Interfaces and Implementations:
 * Techniques for Creating Reusable Software.  Reading, Massachusetts:
 * Addison-Wesley, 1997.
 *
 * [Knuth73] Donald E. Knuth. The Art of Computer Programming.  Volume 3:
 * Sorting and Searching.  Reading, Massachusetts: Addison-Wesley, 1973.
 *
 * [Larson88] Per-Ake Larson. "Dynamic Hash Tables".  CACM 31(4), April
 * 1988, pp. 446-457.
 *
 */

#define HASH_MAIN 0

#if HASH_MAIN
# include <stdio.h>
# include <stdlib.h>
#else
# include "xf86drm.h"
# ifdef XFree86LOADER
#  include "xf86.h"
#  include "xf86_ansic.h"
# else
#  include <stdio.h>
#  include <stdlib.h>
# endif
#endif

#define HASH_MAGIC 0xdeadbeef
#define HASH_DEBUG 0
#define HASH_SIZE  512		/* Good for about 100 entries */
				/* If you change this value, you probably
                                   have to change the HashHash hashing
                                   function! */

#if HASH_MAIN
#define HASH_ALLOC malloc
#define HASH_FREE  free
#define HASH_RANDOM_DECL
#define HASH_RANDOM_INIT(seed)  srandom(seed)
#define HASH_RANDOM             random()
#else
#define HASH_ALLOC drmMalloc
#define HASH_FREE  drmFree
#define HASH_RANDOM_DECL        void *state
#define HASH_RANDOM_INIT(seed)  state = drmRandomCreate(seed)
#define HASH_RANDOM             drmRandom(state)

#endif

typedef struct HashBucket {
    unsigned long     key;
    void              *value;
    struct HashBucket *next;
} HashBucket, *HashBucketPtr;

typedef struct HashTable {
    unsigned long    magic;
    unsigned long    entries;
    unsigned long    hits;	/* At top of linked list */
    unsigned long    partials;	/* Not at top of linked list */
    unsigned long    misses;	/* Not in table */
    HashBucketPtr    buckets[HASH_SIZE];
    int              p0;
    HashBucketPtr    p1;
} HashTable, *HashTablePtr;

#if HASH_MAIN
extern void *drmHashCreate(void);
extern int  drmHashDestroy(void *t);
extern int  drmHashLookup(void *t, unsigned long key, unsigned long *value);
extern int  drmHashInsert(void *t, unsigned long key, unsigned long value);
extern int  drmHashDelete(void *t, unsigned long key);
#endif

static unsigned long HashHash(unsigned long key)
{
    unsigned long        hash = 0;
    unsigned long        tmp  = key;
    static int           init = 0;
    static unsigned long scatter[256];
    int                  i;

    if (!init) {
	HASH_RANDOM_DECL;
	HASH_RANDOM_INIT(37);
	for (i = 0; i < 256; i++) scatter[i] = HASH_RANDOM;
	++init;
    }

    while (tmp) {
	hash = (hash << 1) + scatter[tmp & 0xff];
	tmp >>= 8;
    }

    hash %= HASH_SIZE;
#if HASH_DEBUG
    printf( "Hash(%d) = %d\n", key, hash);
#endif
    return hash;
}

void *drmHashCreate(void)
{
    HashTablePtr table;
    int          i;

    table           = HASH_ALLOC(sizeof(*table));
    if (!table) return NULL;
    table->magic    = HASH_MAGIC;
    table->entries  = 0;
    table->hits     = 0;
    table->partials = 0;
    table->misses   = 0;

    for (i = 0; i < HASH_SIZE; i++) table->buckets[i] = NULL;
    return table;
}

int drmHashDestroy(void *t)
{
    HashTablePtr  table = (HashTablePtr)t;
    HashBucketPtr bucket;
    HashBucketPtr next;
    int           i;

    if (table->magic != HASH_MAGIC) return -1; /* Bad magic */

    for (i = 0; i < HASH_SIZE; i++) {
	for (bucket = table->buckets[i]; bucket;) {
	    next = bucket->next;
	    HASH_FREE(bucket);
	    bucket = next;
	}
    }
    HASH_FREE(table);
    return 0;
}

/* Find the bucket and organize the list so that this bucket is at the
   top. */

static HashBucketPtr HashFind(HashTablePtr table,
			      unsigned long key, unsigned long *h)
{
    unsigned long hash = HashHash(key);
    HashBucketPtr prev = NULL;
    HashBucketPtr bucket;

    if (h) *h = hash;

    for (bucket = table->buckets[hash]; bucket; bucket = bucket->next) {
	if (bucket->key == key) {
	    if (prev) {
				/* Organize */
		prev->next           = bucket->next;
		bucket->next         = table->buckets[hash];
		table->buckets[hash] = bucket;
		++table->partials;
	    } else {
		++table->hits;
	    }
	    return bucket;
	}
	prev = bucket;
    }
    ++table->misses;
    return NULL;
}

int drmHashLookup(void *t, unsigned long key, void **value)
{
    HashTablePtr  table = (HashTablePtr)t;
    HashBucketPtr bucket;

    if (!table || table->magic != HASH_MAGIC) return -1; /* Bad magic */

    bucket = HashFind(table, key, NULL);
    if (!bucket) return 1;	/* Not found */
    *value = bucket->value;
    return 0;			/* Found */
}

int drmHashInsert(void *t, unsigned long key, void *value)
{
    HashTablePtr  table = (HashTablePtr)t;
    HashBucketPtr bucket;
    unsigned long hash;

    if (table->magic != HASH_MAGIC) return -1; /* Bad magic */

    if (HashFind(table, key, &hash)) return 1; /* Already in table */

    bucket               = HASH_ALLOC(sizeof(*bucket));
    if (!bucket) return -1;	/* Error */
    bucket->key          = key;
    bucket->value        = value;
    bucket->next         = table->buckets[hash];
    table->buckets[hash] = bucket;
#if HASH_DEBUG
    printf("Inserted %d at %d/%p\n", key, hash, bucket);
#endif
    return 0;			/* Added to table */
}

int drmHashDelete(void *t, unsigned long key)
{
    HashTablePtr  table = (HashTablePtr)t;
    unsigned long hash;
    HashBucketPtr bucket;

    if (table->magic != HASH_MAGIC) return -1; /* Bad magic */

    bucket = HashFind(table, key, &hash);

    if (!bucket) return 1;	/* Not found */

    table->buckets[hash] = bucket->next;
    HASH_FREE(bucket);
    return 0;
}

int drmHashNext(void *t, unsigned long *key, void **value)
{
    HashTablePtr  table = (HashTablePtr)t;

    for (; table->p0 < HASH_SIZE;
	 ++table->p0, table->p1 = table->buckets[table->p0]) {
	if (table->p1) {
	    *key       = table->p1->key;
	    *value     = table->p1->value;
	    table->p1  = table->p1->next;
	    return 1;
	}
    }
    return 0;
}

int drmHashFirst(void *t, unsigned long *key, void **value)
{
    HashTablePtr  table = (HashTablePtr)t;

    if (table->magic != HASH_MAGIC) return -1; /* Bad magic */

    table->p0 = 0;
    table->p1 = table->buckets[0];
    return drmHashNext(table, key, value);
}

#if HASH_MAIN
#define DIST_LIMIT 10
static int dist[DIST_LIMIT];

static void clear_dist(void) {
    int i;

    for (i = 0; i < DIST_LIMIT; i++) dist[i] = 0;
}

static int count_entries(HashBucketPtr bucket)
{
    int count = 0;

    for (; bucket; bucket = bucket->next) ++count;
    return count;
}

static void update_dist(int count)
{
    if (count >= DIST_LIMIT) ++dist[DIST_LIMIT-1];
    else                     ++dist[count];
}

static void compute_dist(HashTablePtr table)
{
    int           i;
    HashBucketPtr bucket;

    printf("Entries = %ld, hits = %ld, partials = %ld, misses = %ld\n",
	   table->entries, table->hits, table->partials, table->misses);
    clear_dist();
    for (i = 0; i < HASH_SIZE; i++) {
	bucket = table->buckets[i];
	update_dist(count_entries(bucket));
    }
    for (i = 0; i < DIST_LIMIT; i++) {
	if (i != DIST_LIMIT-1) printf("%5d %10d\n", i, dist[i]);
	else                   printf("other %10d\n", dist[i]);
    }
}

static void check_table(HashTablePtr table,
			unsigned long key, unsigned long value)
{
    unsigned long retval  = 0;
    int           retcode = drmHashLookup(table, key, &retval);

    switch (retcode) {
    case -1:
	printf("Bad magic = 0x%08lx:"
	       " key = %lu, expected = %lu, returned = %lu\n",
	       table->magic, key, value, retval);
	break;
    case 1:
	printf("Not found: key = %lu, expected = %lu returned = %lu\n",
	       key, value, retval);
	break;
    case 0:
	if (value != retval)
	    printf("Bad value: key = %lu, expected = %lu, returned = %lu\n",
		   key, value, retval);
	break;
    default:
	printf("Bad retcode = %d: key = %lu, expected = %lu, returned = %lu\n",
	       retcode, key, value, retval);
	break;
    }
}

int main(void)
{
    HashTablePtr table;
    int          i;

    printf("\n***** 256 consecutive integers ****\n");
    table = drmHashCreate();
    for (i = 0; i < 256; i++) drmHashInsert(table, i, i);
    for (i = 0; i < 256; i++) check_table(table, i, i);
    for (i = 256; i >= 0; i--) check_table(table, i, i);
    compute_dist(table);
    drmHashDestroy(table);

    printf("\n***** 1024 consecutive integers ****\n");
    table = drmHashCreate();
    for (i = 0; i < 1024; i++) drmHashInsert(table, i, i);
    for (i = 0; i < 1024; i++) check_table(table, i, i);
    for (i = 1024; i >= 0; i--) check_table(table, i, i);
    compute_dist(table);
    drmHashDestroy(table);

    printf("\n***** 1024 consecutive page addresses (4k pages) ****\n");
    table = drmHashCreate();
    for (i = 0; i < 1024; i++) drmHashInsert(table, i*4096, i);
    for (i = 0; i < 1024; i++) check_table(table, i*4096, i);
    for (i = 1024; i >= 0; i--) check_table(table, i*4096, i);
    compute_dist(table);
    drmHashDestroy(table);

    printf("\n***** 1024 random integers ****\n");
    table = drmHashCreate();
    srandom(0xbeefbeef);
    for (i = 0; i < 1024; i++) drmHashInsert(table, random(), i);
    srandom(0xbeefbeef);
    for (i = 0; i < 1024; i++) check_table(table, random(), i);
    srandom(0xbeefbeef);
    for (i = 0; i < 1024; i++) check_table(table, random(), i);
    compute_dist(table);
    drmHashDestroy(table);

    printf("\n***** 5000 random integers ****\n");
    table = drmHashCreate();
    srandom(0xbeefbeef);
    for (i = 0; i < 5000; i++) drmHashInsert(table, random(), i);
    srandom(0xbeefbeef);
    for (i = 0; i < 5000; i++) check_table(table, random(), i);
    srandom(0xbeefbeef);
    for (i = 0; i < 5000; i++) check_table(table, random(), i);
    compute_dist(table);
    drmHashDestroy(table);

    return 0;
}
#endif
