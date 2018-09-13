/***
*crt0init.c - Initialization segment declarations.
*
* Copyright (c) 1992 - 1999 Microsoft Corporation. All rights reserved.*
*Purpose:
*       Do initialization segment declarations.
*
*Notes:
*       In the 16-bit C world, the X*B and X*E segments were empty except for
*       a label.  This will not work with COFF since COFF throws out empty
*       sections.  Therefore we must put a zero value in them.  (Zero because
*       the routine to traverse the initializers will skip over zero entries.)
*
*******************************************************************************/
#define _CRTBLD

#include <stdio.h>
#include <internal.h>

#pragma data_seg(".CRT$XIA")
_PVFV __xi_a[] = { NULL };


#pragma data_seg(".CRT$XIZ")
_PVFV __xi_z[] = { NULL };


#pragma data_seg(".CRT$XCA")
_PVFV __xc_a[] = { NULL };


#pragma data_seg(".CRT$XCZ")
_PVFV __xc_z[] = { NULL };


#pragma data_seg(".CRT$XPA")
_PVFV __xp_a[] = { NULL };


#pragma data_seg(".CRT$XPZ")
_PVFV __xp_z[] = { NULL };


#pragma data_seg(".CRT$XTA")
_PVFV __xt_a[] = { NULL };


#pragma data_seg(".CRT$XTZ")
_PVFV __xt_z[] = { NULL };

#pragma data_seg()  /* reset */


#pragma comment(linker, "/merge:.CRT=.data")

#pragma comment(linker, "/defaultlib:kernel32.lib")

#if !(!defined (_MT) && !defined (_DEBUG))
#pragma comment(linker, "/disallowlib:libc.lib")
#endif  /* !(!defined (_MT) && !defined (_DEBUG)) */
#if !(!defined (_MT) && defined (_DEBUG))
#pragma comment(linker, "/disallowlib:libcd.lib")
#endif  /* !(!defined (_MT) && defined (_DEBUG)) */
#if !(defined (_MT) && !defined (_DEBUG))
#pragma comment(linker, "/disallowlib:libcmt.lib")
#endif  /* !(defined (_MT) && !defined (_DEBUG)) */
#if !(defined (_MT) && defined (_DEBUG))
#pragma comment(linker, "/disallowlib:libcmtd.lib")
#endif  /* !(defined (_MT) && defined (_DEBUG)) */
#pragma comment(linker, "/disallowlib:msvcrt.lib")
#pragma comment(linker, "/disallowlib:msvcrtd.lib")
