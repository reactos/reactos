/* $Id: trustee.c,v 1.3 2004/12/15 12:29:13 ekohl Exp $
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
 * BuildImpersonateTrusteeA [ADVAPI32.@]
 */
VOID WINAPI
BuildImpersonateTrusteeA(PTRUSTEE_A pTrustee,
                         PTRUSTEE_A pImpersonateTrustee)
{
  pTrustee->pMultipleTrustee = pImpersonateTrustee;
  pTrustee->MultipleTrusteeOperation = TRUSTEE_IS_IMPERSONATE;
}


/******************************************************************************
 * BuildImpersonateTrusteeW [ADVAPI32.@]
 */
VOID WINAPI
BuildImpersonateTrusteeW(PTRUSTEE_W pTrustee,
                         PTRUSTEE_W pImpersonateTrustee)
{
  pTrustee->pMultipleTrustee = pImpersonateTrustee;
  pTrustee->MultipleTrusteeOperation = TRUSTEE_IS_IMPERSONATE;
}


/******************************************************************************
 * BuildTrusteeWithSidA [ADVAPI32.@]
 */
VOID WINAPI
BuildTrusteeWithSidA(PTRUSTEE_A pTrustee, PSID pSid)
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
VOID WINAPI
BuildTrusteeWithSidW(PTRUSTEE_W pTrustee, PSID pSid)
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
VOID WINAPI
BuildTrusteeWithNameA(PTRUSTEE_A pTrustee, LPSTR name)
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
VOID WINAPI
BuildTrusteeWithNameW(PTRUSTEE_W pTrustee, LPWSTR name)
{
    DPRINT("%p %s\n", pTrustee, name);

    pTrustee->pMultipleTrustee = NULL;
    pTrustee->MultipleTrusteeOperation = NO_MULTIPLE_TRUSTEE;
    pTrustee->TrusteeForm = TRUSTEE_IS_NAME;
    pTrustee->TrusteeType = TRUSTEE_IS_UNKNOWN;
    pTrustee->ptstrName = name;
}


/******************************************************************************
 * GetMultipleTrusteeA [ADVAPI32.@]
 */
PTRUSTEEA WINAPI
GetMultipleTrusteeA(PTRUSTEE_A pTrustee)
{
  return pTrustee->pMultipleTrustee;
}


/******************************************************************************
 * GetMultipleTrusteeW [ADVAPI32.@]
 */
PTRUSTEEW WINAPI
GetMultipleTrusteeW(PTRUSTEE_W pTrustee)
{
  return pTrustee->pMultipleTrustee;
}


/******************************************************************************
 * GetMultipleTrusteeOperationA [ADVAPI32.@]
 */
MULTIPLE_TRUSTEE_OPERATION WINAPI
GetMultipleTrusteeOperationA(PTRUSTEE_A pTrustee)
{
  return pTrustee->MultipleTrusteeOperation;
}


/******************************************************************************
 * GetMultipleTrusteeOperationW [ADVAPI32.@]
 */
MULTIPLE_TRUSTEE_OPERATION WINAPI
GetMultipleTrusteeOperationW(PTRUSTEE_W pTrustee)
{
  return pTrustee->MultipleTrusteeOperation;
}


/******************************************************************************
 * GetTrusteeFormW [ADVAPI32.@]
 */
TRUSTEE_FORM WINAPI
GetTrusteeFormA(PTRUSTEE_A pTrustee)
{
  return pTrustee->TrusteeForm;
}


/******************************************************************************
 * GetTrusteeFormW [ADVAPI32.@]
 */
TRUSTEE_FORM WINAPI
GetTrusteeFormW(PTRUSTEE_W pTrustee)
{
  return pTrustee->TrusteeForm;
}


/******************************************************************************
 * GetTrusteeNameA [ADVAPI32.@]
 */
LPSTR WINAPI
GetTrusteeNameA(PTRUSTEE_A pTrustee)
{
  return (pTrustee->TrusteeForm == TRUSTEE_IS_NAME) ? pTrustee->ptstrName : NULL;
}


/******************************************************************************
 * GetTrusteeNameW [ADVAPI32.@]
 */
LPWSTR WINAPI
GetTrusteeNameW(PTRUSTEE_W pTrustee)
{
  return (pTrustee->TrusteeForm == TRUSTEE_IS_NAME) ? pTrustee->ptstrName : NULL;
}


/******************************************************************************
 * GetTrusteeTypeA [ADVAPI32.@]
 */
TRUSTEE_TYPE WINAPI
GetTrusteeTypeA(PTRUSTEE_A pTrustee)
{
  return pTrustee->TrusteeType;
}


/******************************************************************************
 * GetTrusteeTypeW [ADVAPI32.@]
 */
TRUSTEE_TYPE WINAPI
GetTrusteeTypeW(PTRUSTEE_W pTrustee)
{
  return pTrustee->TrusteeType;
}

/* EOF */
