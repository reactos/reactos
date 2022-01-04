/*
 * PROJECT:     Authentication Package DLL
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Manage user mode contexts (create, destroy, reference)
 * COPYRIGHT:   Copyright 2019-2020 Andreas Maier <staubim@quantentunnel.de>
 */

#pragma once

NTSTATUS
NTAPI
UsrSpInitUserModeContext(
    _In_ LSA_SEC_HANDLE ContextHandle,
    _In_ PSecBuffer PackedContext);
