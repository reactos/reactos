/*
 *  ReactOS kernel
 *  Copyright (C) 1998-2002 ReactOS Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/*
 * PROJECT:         ReactOS kernel
 * FILE:            hashtab.c
 * PURPOSE:         Hash table support
 * PROGRAMMER:      Casper S. Hornstrup (chorns@users.sourceforge.net)
 * NOTES:           The hash function is from:
 *                  Bob Jenkins <bob_jenkins@burtleburtle.net>
 *                  http://burtleburtle.net/bob/hash/doobs.html
 * UPDATE HISTORY:
 *      15-03-2002  CSH  Created
 */
#include <ddk/ntddk.h>
#include <internal/ex.h>

#define NDEBUG
#include <internal/debug.h>

/* FUNCTIONS ****************************************************************/

#define ExpHashTableSize(n) ((ULONG)1 << (n))
#define ExpHashTableMask(n) (ExpHashTableSize(n) - 1)

#define ExpHashMix(a, b, c) \
{ \
  a -= b; a -= c; a ^= (c >> 13); \
  b -= c; b -= a; b ^= (a << 8);  \
  c -= a; c -= b; c ^= (b >> 13); \
  a -= b; a -= c; a ^= (c >> 12); \
  b -= c; b -= a; b ^= (a << 16); \
  c -= a; c -= b; c ^= (b >> 5);  \
  a -= b; a -= c; a ^= (c >> 3);  \
  b -= c; b -= a; b ^= (a << 10); \
  c -= a; c -= b; c ^= (b >> 15); \
}


ULONG
ExpHash(PUCHAR Key,
  ULONG KeyLength)
{
  register ULONG a, b, c, len;

	/* Set up the internal state */
	len = KeyLength;
	a = b = 0x9e3779b9;    /* The golden ratio; an arbitrary value */
	c = 0;

	/* Handle most of the key */
	while (len >= 12)
		{
		  a += (Key[0] + ((ULONG)Key[1]<<8) + ((ULONG)Key[2]<<16) + ((ULONG)Key[3]<<24));
		  b += (Key[4] + ((ULONG)Key[5]<<8) + ((ULONG)Key[6]<<16) + ((ULONG)Key[7]<<24));
		  c += (Key[8] + ((ULONG)Key[9]<<8) + ((ULONG)Key[10]<<16)+ ((ULONG)Key[11]<<24));
		  ExpHashMix(a, b, c);
		  Key += 12; len -= 12;
		}

	/* Handle the last 11 bytes */
	c += KeyLength;
	switch(len)       /* All the case statements fall through */
		{
			case 11: c += ((ULONG)Key[10] << 24);
			case 10: c += ((ULONG)Key[9] << 16);
			case 9 : c += ((ULONG)Key[8] << 8);
			  /* The first byte of c is reserved for the length */
			case 8 : b += ((ULONG)Key[7] << 24);
			case 7 : b += ((ULONG)Key[6] << 16);
			case 6 : b += ((ULONG)Key[5] << 8);
			case 5 : b += Key[4];
			case 4 : a += ((ULONG)Key[3] << 24);
			case 3 : a += ((ULONG)Key[2] << 16);
			case 2 : a += ((ULONG)Key[1] << 8);
			case 1 : a += Key[0];
			/* case 0: nothing left to add */
		}

	ExpHashMix(a, b, c);

	return c;
}


/*
 * Lock the hash table 
 */
inline VOID
ExpLockHashTable(PHASH_TABLE HashTable,
  PKIRQL OldIrql)
{
	if (HashTable->UseNonPagedPool)
	  {
      KeAcquireSpinLock(&HashTable->Lock.NonPaged, OldIrql);
	  }
	else
		{
      ExAcquireFastMutex(&HashTable->Lock.Paged);
		}
}


/*
 * Unlock the hash table
 */
inline VOID
ExpUnlockHashTable(PHASH_TABLE HashTable,
  PKIRQL OldIrql)
{
	if (HashTable->UseNonPagedPool)
	  {
      KeReleaseSpinLock(&HashTable->Lock.NonPaged, *OldIrql);
	  }
	else
		{
      ExReleaseFastMutex(&HashTable->Lock.Paged);
		}
}


/*
 * Insert a value in a hash table.
 */
inline VOID STDCALL
ExpInsertHashTable(IN PHASH_TABLE  HashTable,
  IN PVOID  Key,
  IN ULONG  KeyLength,
  IN PVOID  Value)
{
  ULONG Index;

  Index = (ExpHash(Key, KeyLength) & ExpHashTableMask(HashTable->HashTableSize));

  ExInsertSplayTree(&HashTable->HashTrees[Index], Key, Value);
}


/*
 * Search for a value associated with a given key in a hash table.
 */
inline BOOLEAN STDCALL
ExpSearchHashTable(IN PHASH_TABLE  HashTable,
  IN PVOID  Key,
  IN ULONG  KeyLength,
  OUT PVOID  * Value)
{
  ULONG Index;

  Index = (ExpHash(Key, KeyLength) & ExpHashTableMask(HashTable->HashTableSize));

  return ExSearchSplayTree(&HashTable->HashTrees[Index], Key, Value);
}


/*
 * Remove a value associated with a given key from a hash table.
 */
BOOLEAN STDCALL
ExpRemoveHashTable(IN PHASH_TABLE  HashTable,
  IN PVOID  Key,
  IN ULONG  KeyLength,
  IN PVOID  * Value)
{
  ULONG Index;

  Index = (ExpHash(Key, KeyLength) & ExpHashTableMask(HashTable->HashTableSize));

  return ExRemoveSplayTree(&HashTable->HashTrees[Index], Key, Value);
}


/*
 * Initializes a hash table.
 */
BOOLEAN STDCALL
ExInitializeHashTable(IN PHASH_TABLE  HashTable,
  IN ULONG  HashTableSize,
  IN PKEY_COMPARATOR  Compare  OPTIONAL,
  IN BOOLEAN  UseNonPagedPool)
{
  BOOLEAN Status;
  ULONG Index;

  RtlZeroMemory(HashTable, sizeof(HASH_TABLE));

  HashTable->HashTableSize = HashTableSize;

  HashTable->UseNonPagedPool = UseNonPagedPool;

  if (UseNonPagedPool)
    {
      KeInitializeSpinLock(&HashTable->Lock.NonPaged);
      HashTable->HashTrees = ExAllocatePool(NonPagedPool, ExpHashTableSize(HashTableSize) * sizeof(SPLAY_TREE));
		}
		else
		{
      ExInitializeFastMutex(&HashTable->Lock.Paged);
      HashTable->HashTrees = ExAllocatePool(PagedPool, ExpHashTableSize(HashTableSize) * sizeof(SPLAY_TREE));
		}

  if (HashTable->HashTrees == NULL)
		{
      return FALSE;
		}

  for (Index = 0; Index < ExpHashTableSize(HashTableSize); Index++)
    {
      Status = ExInitializeSplayTree(&HashTable->HashTrees[Index], Compare, FALSE, UseNonPagedPool);

      if (!Status)
				{
          LONG i;

          for (i = Index - 1; i >= 0; i--)
						{
              ExDeleteSplayTree(&HashTable->HashTrees[i]);
						}

          ExFreePool(HashTable->HashTrees);

          return FALSE;
				}
    }

  return TRUE;
}


/*
 * Release resources used by a hash table.
 */
VOID STDCALL
ExDeleteHashTable(IN PHASH_TABLE  HashTable)
{
  ULONG Index;

  for (Index = 0; Index < ExpHashTableSize(HashTable->HashTableSize); Index++)
		{
      ExDeleteSplayTree(&HashTable->HashTrees[Index]);
		}

  ExFreePool(HashTable->HashTrees);
}


/*
 * Insert a value in a hash table.
 */
VOID STDCALL
ExInsertHashTable(IN PHASH_TABLE  HashTable,
  IN PVOID  Key,
  IN ULONG  KeyLength,
  IN PVOID  Value)
{
  KIRQL OldIrql;

  /* FIXME: Use SEH for error reporting */

  ExpLockHashTable(HashTable, &OldIrql);
  ExpInsertHashTable(HashTable, Key, KeyLength, Value);
  ExpUnlockHashTable(HashTable, &OldIrql);
}


/*
 * Search for a value associated with a given key in a hash table.
 */
BOOLEAN STDCALL
ExSearchHashTable(IN PHASH_TABLE  HashTable,
  IN PVOID  Key,
  IN ULONG  KeyLength,
  OUT PVOID  * Value)
{
  BOOLEAN Status;
  KIRQL OldIrql;

  ExpLockHashTable(HashTable, &OldIrql);
  Status = ExpSearchHashTable(HashTable, Key, KeyLength, Value);
  ExpUnlockHashTable(HashTable, &OldIrql);

  return Status;
}


/*
 * Remove a value associated with a given key from a hash table.
 */
BOOLEAN STDCALL
ExRemoveHashTable(IN PHASH_TABLE  HashTable,
  IN PVOID  Key,
  IN ULONG  KeyLength,
  IN PVOID  * Value)
{
  BOOLEAN Status;
  KIRQL OldIrql;

  ExpLockHashTable(HashTable, &OldIrql);
  Status = ExpRemoveHashTable(HashTable, Key, KeyLength, Value);
  ExpUnlockHashTable(HashTable, &OldIrql);

  return Status;
}

/* EOF */
