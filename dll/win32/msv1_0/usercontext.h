/*
 * PROJECT:     Authentication Package DLL
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     manage user mode contexts (create, destroy, reference)
 * COPYRIGHT:   Copyright 2019-2020 Andreas Maier (staubim@quantentunnel.de)
 */
#ifndef _USERCONTEXT_H_
#define _USERCONTEXT_H_

NTSTATUS NTAPI
UsrSpInitUserModeContext(
    _In_ LSA_SEC_HANDLE ContextHandle,
    _In_ PSecBuffer PackedContext);

#endif
