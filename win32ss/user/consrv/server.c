/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Console Server DLL
 * FILE:            win32ss/user/consrv/init.c
 * PURPOSE:         Initialization
 * PROGRAMMERS:     Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

#include "consrv.h"

#define NDEBUG
#include <debug.h>


/* Ensure that a captured buffer is safe to access */
BOOL FASTCALL
Win32CsrValidateBuffer(PCSR_PROCESS ProcessData, PVOID Buffer,
                       SIZE_T NumElements, SIZE_T ElementSize)
{
    /* Check that the following conditions are true:
     * 1. The start of the buffer is somewhere within the process's
     *    shared memory section view.
     * 2. The remaining space in the view is at least as large as the buffer.
     *    (NB: Please don't try to "optimize" this by using multiplication
     *    instead of division; remember that 2147483648 * 2 = 0.)
     * 3. The buffer is DWORD-aligned.
     */
    ULONG_PTR Offset = (BYTE *)Buffer - (BYTE *)ProcessData->ClientViewBase;
    if (Offset >= ProcessData->ClientViewBounds
            || NumElements > (ProcessData->ClientViewBounds - Offset) / ElementSize
            || (Offset & (sizeof(DWORD) - 1)) != 0)
    {
        DPRINT1("Invalid buffer %p(%u*%u); section view is %p(%u)\n",
                Buffer, NumElements, ElementSize,
                ProcessData->ClientViewBase, ProcessData->ClientViewBounds);
        return FALSE;
    }
    return TRUE;
}

/* EOF */
