/*
 * COPYRIGHT:       See COPYING in the top level directory
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
RtlAreAllAccessesGranted(IN ACCESS_MASK GrantedAccess,
                         IN ACCESS_MASK DesiredAccess)
{
    PAGED_CODE_RTL();

    /* Return if there's no leftover bits after granting all of them */
    return !(~GrantedAccess & DesiredAccess);
}

/*
 * @implemented
 */
BOOLEAN
NTAPI
RtlAreAnyAccessesGranted(IN ACCESS_MASK GrantedAccess,
                         IN ACCESS_MASK DesiredAccess)
{
    PAGED_CODE_RTL();

    /* Return if there's any leftover bits after granting all of them */
    return ((GrantedAccess & DesiredAccess) != 0);
}

/*
 * @implemented
 */
VOID
NTAPI
RtlMapGenericMask(IN OUT PACCESS_MASK AccessMask,
                  IN PGENERIC_MAPPING GenericMapping)
{
    PAGED_CODE_RTL();

    /* Convert mappings */
    if (*AccessMask & GENERIC_READ) *AccessMask |= GenericMapping->GenericRead;
    if (*AccessMask & GENERIC_WRITE) *AccessMask |= GenericMapping->GenericWrite;
    if (*AccessMask & GENERIC_EXECUTE) *AccessMask |= GenericMapping->GenericExecute;
    if (*AccessMask & GENERIC_ALL) *AccessMask |= GenericMapping->GenericAll;

    /* Clear generic flags */
    *AccessMask &= ~(GENERIC_READ | GENERIC_WRITE | GENERIC_EXECUTE | GENERIC_ALL);
}

/* EOF */
