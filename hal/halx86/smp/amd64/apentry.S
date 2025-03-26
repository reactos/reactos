/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     AMD64 Application Processor (AP) spinup setup
 * COPYRIGHT:   Copyright 2023 Justin Miller <justin.miller@reactos.org>
 */

#include <asm.inc>

PUBLIC HalpAPEntry16
PUBLIC HalpAPEntryData
PUBLIC HalpAPEntry32
PUBLIC HalpAPEntry16End

.code
HalpAPEntry16:
    cli

    xor ax, ax
	mov ds, ax
	mov ss, ax
    mov fs, ax
    mov gs, ax

    hlt

HalpAPEntry16End:
.long HEX(0)
HalpAPEntry32:
.long HEX(0)
HalpAPEntryData:
.long HEX(0)

END
