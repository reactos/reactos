/*
 * PROJECT:  MKISOFS for the ReactOS Build System
 * LICENSE:  GNU GPLv2 as published by the Free Software Foundation
 * AUTHORS:  Colin Finck <colin@reactos.org>
 */

#ifndef	__ALIGN_H
#define	__ALIGN_H

#ifdef __LP64__
    #define ALIGN_LMASK 7
#else
    #define ALIGN_LMASK 3
#endif

#define	xaligned(a, s)		((((UIntptr_t)(a)) & (s)) == 0)
#define	x2aligned(a, b, s)	(((((UIntptr_t)(a)) | ((UIntptr_t)(b))) & (s)) == 0)

#define	laligned(a)		xaligned(a, ALIGN_LMASK)
#define	l2aligned(a, b)		x2aligned(a, b, ALIGN_LMASK)

#endif
