/*
 * COPYRIGHT:         BSD - See COPYING.ARM in the top level directory
 * PROJECT:           ReactOS CRT library
 * PURPOSE:           Implementation of ceil
 * PROGRAMMER:        Timo Kreuzer (timo.kreuzer@reactos.org)
 *                    Original implementation: dawncrow
 * SOURCE:            MinGW-w64\mingw-w64-crt\math\ceil.S
 */

/* INCLUDES ******************************************************************/

#include <kxarm.h>

/* CODE **********************************************************************/

    TEXTAREA

    LEAF_ENTRY ceil

	vmrs	r1, fpscr

	bic		r0, r1, #0x00c00000
	orr		r0, r0, #0x00400000 /* Round towards Plus Infinity */
	vmsr	fpscr, r0
	vcvtr.s32.f64	s0, d0
	vcvt.f64.s32	d0, s0
	vmsr	fpscr, r1
	bx	lr

    LEAF_END ceil

    END
/* EOF */
