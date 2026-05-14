/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * FILE:            sdk/lib/drivers/ntoskrnl_vista/ps.c
 * PURPOSE:         ntoskrnl_vista Process Manager Stubs
 * COPYRIGHT:       2026 Hasan Eliküçük (hasanelk101@gmail.com)
 */

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/*
 * @unimplemented
 */
UCHAR
NTAPI
PsGetProcessSignatureLevel(
    _In_ PEPROCESS Process)
{
    static BOOLEAN Warned = FALSE;

    if (!Warned)
    {
        DPRINT1("PsGetProcessSignatureLevel: NT 6.0 stub called!\n");
        Warned = TRUE;
    }

    return 0;
}
