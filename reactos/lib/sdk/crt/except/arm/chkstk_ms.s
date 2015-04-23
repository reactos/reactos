/*
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS system libraries
 * PURPOSE:           Implementation of _chkstk and _alloca_probe
 * FILE:              lib/sdk/crt/except/arm/chkstk_ms.s
 * PROGRAMMER:        Timo Kreuzer (timo.kreuzer@reactos.org)
 *                    Yuntian Zhang (yuntian.zh@gmail.com)
 */

/* INCLUDES ******************************************************************/

#include <kxarm.h>

/* CODE **********************************************************************/
    TEXTAREA

MsgUnimplemented ASCII "Unimplemented", CR, LF, NUL

    LEAF_ENTRY ___chkstk_ms
    UNIMPLEMENTED chkstk_ms
    bx lr
    LEAF_END __chkstk_ms

    LEAF_ENTRY __alloca_probe
    UNIMPLEMENTED alloca_probe
    bx lr
    LEAF_END __alloca_probe

    END
/* EOF */
