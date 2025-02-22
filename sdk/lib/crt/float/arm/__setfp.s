/*
 * PROJECT:     ReactOS CRT library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     ARM fpscr setter
 * COPYRIGHT:   Copyright 2021 Roman Masanin <36927roma@gmail.com>
 */

/* INCLUDES ******************************************************************/

#include <kxarm.h>

/* CODE **********************************************************************/

    TEXTAREA

    LEAF_ENTRY __setfp

    VMSR FPSCR,R0

    BX LR

    LEAF_END __setfp

    END
/* EOF */
