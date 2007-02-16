/* COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/rtl/access.c
 * PURPOSE:         Access rights handling functions
 * PROGRAMMERS:     Eric Kohl
 */

/* INCLUDES *****************************************************************/

#include <rtl.h>

#define NDEBUG
#include <debug.h>

/* FUNCTIONS ***************************************************************/

/*
 * @implemented
 */
BOOLEAN
NTAPI
RtlAreAllAccessesGranted(ACCESS_MASK GrantedAccess,
                         ACCESS_MASK DesiredAccess)
{
  PAGED_CODE_RTL();
  return ((GrantedAccess & DesiredAccess) == DesiredAccess);
}

/*
 * @implemented
 */
BOOLEAN
NTAPI
RtlAreAnyAccessesGranted(ACCESS_MASK GrantedAccess,
                         ACCESS_MASK DesiredAccess)
{
    PAGED_CODE_RTL();
    return ((GrantedAccess & DesiredAccess) != 0);
}

/*
 * @implemented
 */
VOID
NTAPI
RtlMapGenericMask(PACCESS_MASK AccessMask,
                  PGENERIC_MAPPING GenericMapping)
{
    PAGED_CODE_RTL();

    if (*AccessMask & GENERIC_READ) *AccessMask |= GenericMapping->GenericRead;

    if (*AccessMask & GENERIC_WRITE) *AccessMask |= GenericMapping->GenericWrite;

    if (*AccessMask & GENERIC_EXECUTE) *AccessMask |= GenericMapping->GenericExecute;

    if (*AccessMask & GENERIC_ALL) *AccessMask |= GenericMapping->GenericAll;

    *AccessMask &= ~(GENERIC_READ | GENERIC_WRITE | GENERIC_EXECUTE | GENERIC_ALL);
}

/* EOF */
