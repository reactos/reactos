/*
 * PROJECT:     ReactOS CRT library
 * LICENSE:     BSD - See COPYING.ARM in the top level directory
 * PURPOSE:     Implementation of _chkstk and _alloca_probe
 * COPYRIGHT:   Copyright 2014 Timo Kreuzer (timo.kreuzer@reactos.org)
 *              Copyright 2014 Yuntian Zhang (yuntian.zh@gmail.com)
 *              Copyright 2019 Mohamed Mediouni (mmediouni@gmx.fr)
 *
 * REFERENCES:  https://github.com/wine-mirror/wine/commit/2b095beace7b457586bd33b3b1c81df116215193
 */

/* INCLUDES ******************************************************************/

#include <kxarm.h>

/* CODE **********************************************************************/
    TEXTAREA

    LEAF_ENTRY __chkstk
    lsl r4, r4, #2
    bx lr
    LEAF_END __chkstk

    LEAF_ENTRY __alloca_probe
    __assertfail
    bx lr
    LEAF_END __alloca_probe

    END
/* EOF */
