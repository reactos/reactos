/*
 * gccmain.c
 *
 * A separate version of __main, __do_global_ctors and __do_global_dtors for
 * Mingw32 for use with Cygwin32 b19. Hopefully this object file will only
 * be linked if the libgcc.a doesn't include __main, __do_global_dtors and
 * __do_global_ctors.
 *
 * This file is part of the Mingw32 package.
 *
 * Contributors:
 *  Code supplied by Stan Cox <scox@cygnus.com>
 *
 * $Revision: 1.3 $
 * $Author: robd $
 * $Date: 2002/11/29 12:27:48 $
 *
 */

/* Needed for the atexit prototype. */
#include <msvcrt/stdlib.h>


typedef void (*func_ptr) (void);
extern func_ptr __CTOR_LIST__[];
extern func_ptr __DTOR_LIST__[];

void __do_global_dtors(void)
{
	static func_ptr* p = __DTOR_LIST__ + 1;

	/*
	 * Call each destructor in the destructor list until a null pointer
	 * is encountered.
	 */
	while (*p)
	{
		(*(p)) ();
		p++;
	}
}

void __do_global_ctors(void)
{
	unsigned long nptrs = (unsigned long)__CTOR_LIST__[0];
	unsigned i;

	/*
	 * If the first entry in the constructor list is -1 then the list
	 * is terminated with a null entry. Otherwise the first entry was
	 * the number of pointers in the list.
	 */
	if (nptrs == -1) {
		for (nptrs = 0; __CTOR_LIST__[nptrs + 1] != 0; nptrs++)
			;
	}

	/* 
	 * Go through the list backwards calling constructors.
	 */
	for (i = nptrs; i >= 1; i--) {
		__CTOR_LIST__[i] ();
	}

	/*
	 * Register the destructors for processing on exit.
	 */
	atexit(__do_global_dtors);
}

static int initialized = 0;

void __main(void)
{
	if (!initialized) {
		initialized = 1;
		__do_global_ctors ();
	}
}

