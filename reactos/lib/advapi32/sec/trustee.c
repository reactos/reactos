/* $Id: trustee.c,v 1.2 2004/12/12 15:16:49 ekohl Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/advapi32/sec/trustee.c
 * PURPOSE:         Trustee functions
 */

#include "advapi32.h"

#define NDEBUG
#include "debug.h"


/******************************************************************************
 * BuildTrusteeWithSidA [ADVAPI32.@]
 */
VOID WINAPI BuildTrusteeWithSidA(PTRUSTEEA pTrustee, PSID pSid)
{
    DPRINT("%p %p\n", pTrustee, pSid);

    pTrustee->pMultipleTrustee = NULL;
    pTrustee->MultipleTrusteeOperation = NO_MULTIPLE_TRUSTEE;
    pTrustee->TrusteeForm = TRUSTEE_IS_SID;
    pTrustee->TrusteeType = TRUSTEE_IS_UNKNOWN;
    pTrustee->ptstrName = (LPSTR) pSid;
}


/******************************************************************************
 * BuildTrusteeWithSidW [ADVAPI32.@]
 */
VOID WINAPI BuildTrusteeWithSidW(PTRUSTEEW pTrustee, PSID pSid)
{
    DPRINT("%p %p\n", pTrustee, pSid);

    pTrustee->pMultipleTrustee = NULL;
    pTrustee->MultipleTrusteeOperation = NO_MULTIPLE_TRUSTEE;
    pTrustee->TrusteeForm = TRUSTEE_IS_SID;
    pTrustee->TrusteeType = TRUSTEE_IS_UNKNOWN;
    pTrustee->ptstrName = (LPWSTR) pSid;
}


/******************************************************************************
 * BuildTrusteeWithNameA [ADVAPI32.@]
 */
VOID WINAPI BuildTrusteeWithNameA(PTRUSTEEA pTrustee, LPSTR name)
{
    DPRINT("%p %s\n", pTrustee, name);

    pTrustee->pMultipleTrustee = NULL;
    pTrustee->MultipleTrusteeOperation = NO_MULTIPLE_TRUSTEE;
    pTrustee->TrusteeForm = TRUSTEE_IS_NAME;
    pTrustee->TrusteeType = TRUSTEE_IS_UNKNOWN;
    pTrustee->ptstrName = name;
}


/******************************************************************************
 * BuildTrusteeWithNameW [ADVAPI32.@]
 */
VOID WINAPI BuildTrusteeWithNameW(PTRUSTEEW pTrustee, LPWSTR name)
{
    DPRINT("%p %s\n", pTrustee, name);

    pTrustee->pMultipleTrustee = NULL;
    pTrustee->MultipleTrusteeOperation = NO_MULTIPLE_TRUSTEE;
    pTrustee->TrusteeForm = TRUSTEE_IS_NAME;
    pTrustee->TrusteeType = TRUSTEE_IS_UNKNOWN;
    pTrustee->ptstrName = name;
}
