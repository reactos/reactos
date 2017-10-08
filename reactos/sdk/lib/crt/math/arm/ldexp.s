/*
 * COPYRIGHT:         BSD - See COPYING.ARM in the top level directory
 * PROJECT:           ReactOS CRT library
 * PURPOSE:           Implementation of ldexp
 * PROGRAMMER:        Timo Kreuzer (timo.kreuzer@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <kxarm.h>

/* CODE **********************************************************************/

    TEXTAREA

    LEAF_ENTRY ldexp

	__assertfail
	bx	lr

    LEAF_END ldexp

    END
/* EOF */
