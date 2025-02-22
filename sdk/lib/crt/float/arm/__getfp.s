/*
 * PROJECT:     ReactOS CRT library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     ARM fpscr getter
 * COPYRIGHT:   Copyright 2021 Roman Masanin <36927roma@gmail.com>
 */

/* INCLUDES ******************************************************************/

#include <kxarm.h>

/* CODE **********************************************************************/

    TEXTAREA

    LEAF_ENTRY __getfp

    VMRS R0,FPSCR

    BX LR

    LEAF_END __getfp

    END
/* EOF */
