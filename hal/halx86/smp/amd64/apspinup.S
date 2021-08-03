/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Assembly file for long mode AP code
 * COPYRIGHT:   Copyright 2021 Justin Miller <justinmiller100@gmail.com>
 */

#include <asm.inc>

PUBLIC APSpinup
PUBLIC APSpinupEnd
PUBLIC APCr3
.code64
APSpinup:
    hlt

APCr3:
APSpinupEnd:
END
