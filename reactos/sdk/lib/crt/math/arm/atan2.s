/*
 * COPYRIGHT:         BSD - See COPYING.ARM in the top level directory
 * PROJECT:           ReactOS CRT library
 * PURPOSE:           Implementation of atan2
 * PROGRAMMER:        Timo Kreuzer (timo.kreuzer@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <kxarm.h>

/* CODE **********************************************************************/

    TEXTAREA

    LEAF_ENTRY atan2

	__assertfail
	bx	lr

    LEAF_END atan2

    END
/* EOF */
