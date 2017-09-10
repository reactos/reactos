/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS NT User-Mode DLL
 * FILE:            lib/ntdll/rtl/uilist.c
 * PURPOSE:         RTL UI to API network computers list conversion.
 *                  Helper for NETAPI32.DLL
 * PROGRAMMERS:     Hermes Belusca-Maito
 */

/* INCLUDES *****************************************************************/

#include <ntdll.h>

#define NDEBUG
#include <debug.h>

/* FUNCTIONS ***************************************************************/

/*
 * @unimplemented
 */
NTSTATUS
NTAPI
RtlConvertUiListToApiList(
    IN PUNICODE_STRING UiList,
    OUT PUNICODE_STRING ApiList,
    IN BOOLEAN SpaceAsSeparator)
{
    DPRINT1("RtlConvertUiListToApiList(%wZ, 0x%p, %s) called\n",
            UiList, &ApiList, SpaceAsSeparator ? "true" : "false");
    UNIMPLEMENTED;
    /*
     * Experiments show that returning a success code but setting the
     * ApiList length to zero is better than returning a failure code.
     */
    RtlInitEmptyUnicodeString(ApiList, NULL, 0);
    return STATUS_SUCCESS;
}

/* EOF */
