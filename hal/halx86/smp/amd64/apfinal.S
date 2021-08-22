/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Assembly file for loading kernel values into AP
 * COPYRIGHT:   Copyright 2021 Justin Miller <justinmiller100@gmail.com>
 */

#include <asm.inc>

PUBLIC APFinal
PUBLIC APFinalEnd
PUBLIC APProcessorStateStruct

.code64
APFinal:

/* ProcessorState "Struct" */
APProcessorStateStruct:
    APCr0:
        .long HEX(0)
    APCr2:
        .long HEX(0)
    APCr3:
        .long HEX(0) 
    APCr4:
        .long HEX(0)   
    APSegCs:
        .long HEX(0)
    APSegSs:
        .long HEX(0)
    APSegDs:
        .long HEX(0)
    APSegEs:
        .long HEX(0)
    APSegGs:
        .long HEX(0)
    APSegFs:
        .long HEX(0)
    APTr:
        .long HEX(0)
    APRip:
        .long HEX(0)
    APRsp:
        .long HEX(0)
    APRcx:
        .long HEX(0)
APFinalEnd:
END
