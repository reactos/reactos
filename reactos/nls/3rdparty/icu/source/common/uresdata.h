/*
******************************************************************************
*                                                                            *
* Copyright (C) 1999-2006, International Business Machines                   *
*                Corporation and others. All Rights Reserved.                *
*                                                                            *
******************************************************************************
*   file name:  uresdata.h
*   encoding:   US-ASCII
*   tab size:   8 (not used)
*   indentation:4
*
*   created on: 1999dec08
*   created by: Markus W. Scherer
*   06/24/02    weiv        Added support for resource sharing
*/

#ifndef __RESDATA_H__
#define __RESDATA_H__

#include "unicode/utypes.h"
#include "unicode/udata.h"
#include "udataswp.h"

/*
 * A Resource is a 32-bit value that has 2 bit fields:
 * 31..28   4-bit type, see enum below
 * 27..0    28-bit four-byte-offset or value according to the type
 */
typedef uint32_t Resource;

#define RES_BOGUS 0xffffffff

#define RES_GET_TYPE(res) ((UResType)((res)>>28UL))
#define RES_GET_OFFSET(res) ((res)&0x0fffffff)
#define RES_GET_POINTER(pRoot, res) ((pRoot)+RES_GET_OFFSET(res))

/* get signed and unsigned integer values directly from the Resource handle */
#define RES_GET_INT(res) (((int32_t)((res)<<4L))>>4L)
#define RES_GET_UINT(res) ((res)&0x0fffffff)

/* indexes[] value names; indexes are generally 32-bit (Resource) indexes */
enum {
    URES_INDEX_LENGTH,          /* [0] contains URES_INDEX_TOP==the length of indexes[] */
    URES_INDEX_STRINGS_TOP,     /* [1] contains the top of the strings, */
                                /*     same as the bottom of resources, rounded up */
    URES_INDEX_RESOURCES_TOP,   /* [2] contains the top of all resources */
    URES_INDEX_BUNDLE_TOP,      /* [3] contains the top of the bundle, */
                                /*     in case it were ever different from [2] */
    URES_INDEX_MAX_TABLE_LENGTH,/* [4] max. length of any table */
    URES_INDEX_ATTRIBUTES,      /* [5] attributes bit set, see URES_ATT_* (new in formatVersion 1.2) */
    URES_INDEX_TOP
};

/* number of bytes at the beginning of the bundle before the strings start */
enum {
    URES_STRINGS_BOTTOM=(1+URES_INDEX_TOP)*4
};

/*
 * Nofallback attribute, attribute bit 0 in indexes[URES_INDEX_ATTRIBUTES].
 * New in formatVersion 1.2 (ICU 3.6).
 *
 * If set, then this resource bundle is a standalone bundle.
 * If not set, then the bundle participates in locale fallback, eventually
 * all the way to the root bundle.
 * If indexes[] is missing or too short, then the attribute cannot be determined
 * reliably. Dependency checking should ignore such bundles, and loading should
 * use fallbacks.
 */
#define URES_ATT_NO_FALLBACK 1

/*
 * File format for .res resource bundle files (formatVersion=1.2)
 *
 * An ICU4C resource bundle file (.res) is a binary, memory-mappable file
 * with nested, hierarchical data structures.
 * It physically contains the following:
 *
 *   Resource root; -- 32-bit Resource item, root item for this bundle's tree;
 *                     currently, the root item must be a table or table32 resource item
 *   int32_t indexes[indexes[0]]; -- array of indexes for friendly
 *                                   reading and swapping; see URES_INDEX_* above
 *                                   new in formatVersion 1.1 (ICU 2.8)
 *   char keys[]; -- characters for key strings
 *                   (formatVersion 1.0: up to 65k of characters; 1.1: <2G)
 *                   (minus the space for root and indexes[]),
 *                   which consist of invariant characters (ASCII/EBCDIC) and are NUL-terminated;
 *                   padded to multiple of 4 bytes for 4-alignment of the following data
 *   data; -- data directly and indirectly indexed by the root item;
 *            the structure is determined by walking the tree
 *
 * Each resource bundle item has a 32-bit Resource handle (see typedef above)
 * which contains the item type number in its upper 4 bits (31..28) and either
 * an offset or a direct value in its lower 28 bits (27..0).
 * The order of items is undefined and only determined by walking the tree.
 * Leaves of the tree may be stored first or last or anywhere in between,
 * and it is in theory possible to have unreferenced holes in the file.
 *
 * Direct values:
 * - Empty Unicode strings have an offset value of 0 in the Resource handle itself.
 * - Integer values are 28-bit values stored in the Resource handle itself;
 *   the interpretation of unsigned vs. signed integers is up to the application.
 *
 * All other types and values use 28-bit offsets to point to the item's data.
 * The offset is an index to the first 32-bit word of the value, relative to the
 * start of the resource data (i.e., the root item handle is at offset 0).
 * To get byte offsets, the offset is multiplied by 4 (or shifted left by 2 bits).
 * All resource item values are 4-aligned.
 *
 * The structures (memory layouts) for the values for each item type are listed
 * in the table above.
 *
 * Nested, hierarchical structures: -------------
 *
 * Table items contain key-value pairs where the keys are 16-bit offsets to char * key strings.
 * Key string offsets are also relative to the start of the resource data (of the root handle),
 * i.e., the first string has an offset of 4 (after the 4-byte root handle).
 *
 * The values of these pairs are Resource handles.
 *
 * Array items are simple vectors of Resource handles.
 *
 * An alias item is special (and new in ICU 2.4): --------------
 *
 * Its memory layout is just like for a UnicodeString, but at runtime it resolves to
 * another resource bundle's item according to the path in the string.
 * This is used to share items across bundles that are in different lookup/fallback
 * chains (e.g., large collation data among zh_TW and zh_HK).
 * This saves space (for large items) and maintenance effort (less duplication of data).
 *
 * --------------------------------------------------------------------------
 *
 * Resource types:
 *
 * Most resources have their values stored at four-byte offsets from the start
 * of the resource data. These values are at least 4-aligned.
 * Some resource values are stored directly in the offset field of the Resource itself.
 * See UResType in unicode/ures.h for enumeration constants for Resource types.
 *
 * Type Name            Memory layout of values
 *                      (in parentheses: scalar, non-offset values)
 *
 * 0  Unicode String:   int32_t length, UChar[length], (UChar)0, (padding)
 *                  or  (empty string ("") if offset==0)
 * 1  Binary:           int32_t length, uint8_t[length], (padding)
 *                      - this value should be 32-aligned -
 * 2  Table:            uint16_t count, uint16_t keyStringOffsets[count], (uint16_t padding), Resource[count]
 * 3  Alias:            (physically same value layout as string, new in ICU 2.4)
 * 4  Table32:          int32_t count, int32_t keyStringOffsets[count], Resource[count]
 *                      (new in formatVersion 1.1/ICU 2.8)
 *
 * 7  Integer:          (28-bit offset is integer value)
 * 8  Array:            int32_t count, Resource[count]
 *
 * 14 Integer Vector:   int32_t length, int32_t[length]
 * 15 Reserved:         This value denotes special purpose resources and is for internal use.
 *
 * Note that there are 3 types with data vector values:
 * - Vectors of 8-bit bytes stored as type Binary.
 * - Vectors of 16-bit words stored as type Unicode String
 *                     (no value restrictions, all values 0..ffff allowed!).
 * - Vectors of 32-bit words stored as type Integer Vector.
 */

/*
 * Structure for a single, memory-mapped ResourceBundle.
 */
typedef struct {
    UDataMemory *data;
    Resource *pRoot;
    Resource rootRes;
    UBool noFallback; /* see URES_ATT_NO_FALLBACK */
} ResourceData;

/*
 * Load a resource bundle file.
 * The ResourceData structure must be allocated externally.
 */
U_CFUNC UBool
res_load(ResourceData *pResData,
         const char *path, const char *name, UErrorCode *errorCode);

/*
 * Release a resource bundle file.
 * This does not release the ResourceData structure itself.
 */
U_CFUNC void
res_unload(ResourceData *pResData);

/*
 * Return a pointer to a zero-terminated, const UChar* string
 * and set its length in *pLength.
 * Returns NULL if not found.
 */
U_CFUNC const UChar *
res_getString(const ResourceData *pResData, const Resource res, int32_t *pLength);

U_CFUNC const UChar *
res_getAlias(const ResourceData *pResData, const Resource res, int32_t *pLength);

U_CFUNC const uint8_t *
res_getBinary(const ResourceData *pResData, const Resource res, int32_t *pLength);

U_CFUNC const int32_t *
res_getIntVector(const ResourceData *pResData, const Resource res, int32_t *pLength);

U_CFUNC Resource
res_getResource(const ResourceData *pResData, const char *key);

U_CFUNC int32_t
res_countArrayItems(const ResourceData *pResData, const Resource res);

U_CFUNC Resource res_getArrayItem(const ResourceData *pResData, Resource array, const int32_t indexS);
U_CFUNC Resource res_getTableItemByIndex(const ResourceData *pResData, Resource table, int32_t indexS, const char ** key);
U_CFUNC Resource res_getTableItemByKey(const ResourceData *pResData, Resource table, int32_t *indexS, const char* * key);

/*
 * Modifies the contents of *path (replacing separators with NULs),
 * and also moves *path forward while it finds items.
 */
U_CFUNC Resource res_findResource(const ResourceData *pResData, Resource r, char** path, const char** key);

/**
 * Swap an ICU resource bundle. See udataswp.h.
 * @internal
 */
U_CAPI int32_t U_EXPORT2
ures_swap(const UDataSwapper *ds,
          const void *inData, int32_t length, void *outData,
          UErrorCode *pErrorCode);

#endif
