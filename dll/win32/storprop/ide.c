/*
 * PROJECT:     ReactOS system libraries
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Storage device properties
 * COPYRIGHT:   2021 Eric Kohl (eric.kohl@reactos.org)
 */

#include "precomp.h"

#define NDEBUG
#include <debug.h>


/*
 * @unimplemented
 */
BOOL
WINAPI
IdePropPageProvider(
    _In_ PSP_PROPSHEETPAGE_REQUEST lpPropSheetPageRequest,
    _In_ LPFNADDPROPSHEETPAGE lpfnAddPropSheetPageProc,
    _In_ LPARAM lParam)
{
    DPRINT1("IdePropPageProvider(%p %p %lx)\n",
           lpPropSheetPageRequest, lpfnAddPropSheetPageProc, lParam);
    return FALSE;
}

/* EOF */
