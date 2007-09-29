/*
 * gmem.h
 *
 * Memory routines with out-of-memory checking.
 *
 * Copyright 1996-2003 Glyph & Cog, LLC
 */

#ifndef GMEM_H
#define GMEM_H

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Same as malloc, but prints error message and exits if malloc()
 * returns NULL.
 */
extern void *gmalloc(size_t size);

/*
 * Same as realloc, but prints error message and exits if realloc()
 * returns NULL.  If <p> is NULL, calls malloc instead of realloc().
 */
extern void *grealloc(void *p, size_t size);

/*
 * These are similar to gmalloc and grealloc, but take an object count
 * and size.  The result is similar to allocating nObjs * objSize
 * bytes, but there is an additional error check that the total size
 * doesn't overflow an int.
 */
extern void *gmallocn(int nObjs, int objSize);
extern void *greallocn(void *p, int nObjs, int objSize);

/*
 * #ifdef DEBUG_MEM, adds debuging info. If not, same as free.
 */
extern void gfree(void *p);

#ifdef DEBUG_MEM
/*
 * Report on unfreed memory.
 */
extern void gMemReport(FILE *f);
#else
#define gMemReport(f)
#endif

/*
 * Allocate memory and copy a string into it.
 */
extern char *copyString(char *s);

#ifdef __cplusplus
}
#endif

#endif
