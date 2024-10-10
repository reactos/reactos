/*
 * PROJECT:     FreeLoader UEFI Support
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     ARM32 assembly UEFI escape helper functions
 * COPYRIGHT:   Copyright 2023 Justin Miller <justin.miller@reactos.org>
 */

#include <ksarm.h>

    NESTED_ENTRY _exituefi
    PROLOG_END _exituefi

    __assertfail

    NESTED_END _exituefi

    NESTED_ENTRY _changestack
    PROLOG_END _changestack

    __assertfail

    NESTED_END _changestack

    END
