/*
*******************************************************************************
*
*   Copyright (C) 1999-2005, International Business Machines
*   Corporation and others.  All Rights Reserved.
*
*******************************************************************************
*   file name:  toolutil.h
*   encoding:   US-ASCII
*   tab size:   8 (not used)
*   indentation:4
*
*   created on: 1999nov19
*   created by: Markus W. Scherer
*
*   This file defines utility functions for ICU tools like genccode.
*/

#ifndef __TOOLUTIL_H__
#define __TOOLUTIL_H__

#include "unicode/utypes.h"

/*
 * For Windows, a path/filename may be the short (8.3) version
 * of the "real", long one. In this case, the short one
 * is abbreviated and contains a tilde etc.
 * This function returns a pointer to the original pathname
 * if it is the "real" one itself, and a pointer to a static
 * buffer (not thread-safe) containing the long version
 * if the pathname is indeed abbreviated.
 *
 * On platforms other than Windows, this function always returns
 * the input pathname pointer.
 *
 * This function is especially useful in tools that are called
 * by a batch file for loop, which yields short pathnames on Win9x.
 */
U_CAPI const char * U_EXPORT2
getLongPathname(const char *pathname);

/*
 * Find the basename at the end of a pathname, i.e., the part
 * after the last file separator, and return a pointer
 * to this part of the pathname.
 * If the pathname only contains a basename and no file separator,
 * then the pathname pointer itself is returned.
 */
U_CAPI const char * U_EXPORT2
findBasename(const char *filename);

/*
 * Creates a diretory with pathname.
 *
 * @param status Set to an error code when mkdir failed.
 */
U_CAPI void U_EXPORT2
uprv_mkdir(const char *pathname, UErrorCode *status);

/*
 * UToolMemory is used for generic, custom memory management.
 * It is allocated with enough space for count*size bytes starting
 * at array.
 * The array is declared with a union of large data types so
 * that its base address is aligned for any types.
 * If size is a multiple of a data type size, then such items
 * can be safely allocated inside the array, at offsets that
 * are themselves multiples of size.
 */
struct UToolMemory;
typedef struct UToolMemory UToolMemory;

/**
 * Open a UToolMemory object for allocation of initialCapacity to maxCapacity
 * items with size bytes each.
 */
U_CAPI UToolMemory * U_EXPORT2
utm_open(const char *name, int32_t initialCapacity, int32_t maxCapacity, int32_t size);

/**
 * Close a UToolMemory object.
 */
U_CAPI void U_EXPORT2
utm_close(UToolMemory *mem);

/**
 * Get the pointer to the beginning of the array of items.
 * The pointer becomes invalid after allocation of new items.
 */
U_CAPI void * U_EXPORT2
utm_getStart(UToolMemory *mem);

/**
 * Get the current number of items.
 */
U_CAPI int32_t U_EXPORT2
utm_countItems(UToolMemory *mem);

/**
 * Allocate one more item and return the pointer to its start in the array.
 */
U_CAPI void * U_EXPORT2
utm_alloc(UToolMemory *mem);

/**
 * Allocate n items and return the pointer to the start of the first one in the array.
 */
U_CAPI void * U_EXPORT2
utm_allocN(UToolMemory *mem, int32_t n);

#endif
