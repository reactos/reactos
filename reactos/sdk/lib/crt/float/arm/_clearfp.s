/*
 * COPYRIGHT:         BSD - See COPYING.ARM in the top level directory
 * PROJECT:           ReactOS CRT library
 * PURPOSE:           Implementation of _clearfp
 * PROGRAMMER:        Timo Kreuzer (timo.kreuzer@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <kxarm.h>

/* CODE **********************************************************************/

    TEXTAREA

    LEAF_ENTRY _clearfp

	__assertfail
	bx	lr

    LEAF_END _clearfp

    END
/* EOF */
