
/* INCLUDES *****************************************************************/

#include <rtl_vista.h>

#define NDEBUG
#include <debug.h>

/* GLOBALS ******************************************************************/

/*
 * Wait on address and WakeAddress is implemented in another PR.
 * This is only needed for d3d10x+ So PR SHIPs without it for now.
 */

/* EXPORTED FUNCTIONS ********************************************************/

NTSTATUS
NTAPI
RtlWaitOnAddress(
    _In_ const volatile VOID *Address,
    _In_ PVOID CompareAddress,
    _In_ SIZE_T AddressSize,
    _In_opt_ PLARGE_INTEGER Timeout)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

VOID
NTAPI
RtlWakeAddressAll(
    _In_ const volatile VOID *Address)
{
    UNIMPLEMENTED;
}

VOID
NTAPI
RtlWakeAddressSingle(
    _In_ const volatile VOID *Address)
{
    UNIMPLEMENTED;
}

/* EOF */
