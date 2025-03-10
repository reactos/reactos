/*
 * PROJECT:    Hash Table
 * LICENSE:    MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:    Hash Table header file
 * COPYRIGHT:  Copyright 2025 Curtis Wilson <LiquidFox1776@gmail.com>
 */
 
#include <windows.h>
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <math.h>

// todo make bIsFull private
typedef struct _HASH_ENTRY
{
    void* pKey;
    SIZE_T nKeyLength;
    void* pValue;
    BOOL bIsFull;

    struct _HASH_ENTRY* pNext;
}HASH_ENTRY, *PHASH_ENTRY;

typedef struct _HASH_TABLE
{
    PVOID pInternal;
    BOOL (*DeleteEntry)(struct _HASH_TABLE* self, PVOID pKey, SIZE_T nKeyLength, BOOL bFreeKey, BOOL bFreeValue);
    BOOL (*SetEntry)(struct _HASH_TABLE** self, PVOID pKey, SIZE_T nKeyLength, PVOID pValue);
    PVOID (*GetValue)(struct _HASH_TABLE* self, PVOID pKey, SIZE_T nKeyLength);
    SIZE_T (*GetNumberOfEntries)(struct _HASH_TABLE* self);
    HANDLE (*GetFirstEntry)(struct _HASH_TABLE* self, PHASH_ENTRY *pHashEntry);
    BOOL (*GetNextEntry)(HANDLE hIterator, PHASH_ENTRY *pHashEntry);

}HASH_TABLE, * PHASH_TABLE;

BOOL
FreeHashTable(
    PHASH_TABLE* pHashTable,
    BOOL bFreeKey,
    BOOL bFreeValue);

PHASH_TABLE
CreateHashTable(
    SIZE_T nNumberOfInitalSlots,
    DOUBLE dLoadFactor);
