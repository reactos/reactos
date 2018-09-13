/*	File: D:\WACKER\tdll\mc.c (Created: 29-Nov-1993)
 *
 *	Copyright 1994 by Hilgraeve Inc. -- Monroe, MI
 *	All rights reserved
 *
 *	$Revision: 1 $
 *	$Date: 10/05/98 12:35p $
 */

// The file memory.h contains macros to define malloc, free and realloc
// to map the functions below...

#define MEMORY_SOURCE_FILE

#include <stddef.h>
#include <malloc.h>

#include "mc.h"

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	mc_malloc
 *
 * DESCRIPTION:
 *	Use this function to insert any memory managers we might want.
 *
 * ARGUMENTS:
 *	size	- number of bytes to allocate
 *
 * RETURNS:
 *	pointer to memory or NULL
 *
 */
void *mc_malloc(size_t size)
	{
	return malloc(size);
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	mc_free
 *
 * DESCRIPTION:
 *	Use this function to insert any memory managers we might want.
 *
 * ARGUMENTS:
 *	pv	- pointer to memory to free
 *
 * RETURNS:
 *	void
 *
 */
void mc_free(void *pv)
	{
	free(pv);
	return;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	mc_realloc
 *
 * DESCRIPTION:
 *	Use this function to insert any memory managers we might want.
 *
 * ARGUMENTS:
 *	pv		- pointer to memory to realloc
 *	size	- new size
 *
 * RETURNS:
 *	pointer to memory or NULL
 *
 */
void *mc_realloc(void *pv, size_t size)
	{
	return realloc(pv, size);
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	mc_calloc
 *
 * DESCRIPTION:
 *	Use this function to insert any memory managers we might want.
 *
 * ARGUMENTS:
 *	num 	- number of elements.
 *	size	- size of each element.
 *
 * RETURNS:
 *	pointer to buffer or zero.
 *
 */
void *mc_calloc(size_t num, size_t size)
	{
	return calloc(num, size);
	}
