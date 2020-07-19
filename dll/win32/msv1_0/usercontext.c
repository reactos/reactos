/*
 * PROJECT:     Authentication Package DLL
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        dll/win32/msv1_0/usercontext.c
 * PURPOSE:     manage user mode contexts (create, destroy, reference)
 * COPYRIGHT:   Copyright 2019-2020 Andreas Maier <staubim@quantentunnel.de>
 */

#include "precomp.h"

#include "wine/debug.h"
WINE_DEFAULT_DEBUG_CHANNEL(msv1_0);

NTSTATUS NTAPI
UsrSpInitUserModeContext(
    _In_ LSA_SEC_HANDLE ContextHandle,
    _In_ PSecBuffer PackedContext)
{
    TRACE("UsrSpInitUserModeContext(%p %p)\n",
          ContextHandle, PackedContext);
    return STATUS_NOT_IMPLEMENTED;
}
