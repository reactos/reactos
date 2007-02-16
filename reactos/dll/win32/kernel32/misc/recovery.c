/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/kernel32/misc/recovery.c
 * PURPOSE:         Application Recovery functions
 * PROGRAMMER:      Thomas Weidenmueller <w3seek@reactos.com>
 *
 * UPDATE HISTORY:
 *                  10/28/2005 Created stubs (w3)
 */

#include <k32.h>

#define NDEBUG
#include "../include/debug.h"

/* PUBLIC FUNCTIONS ***********************************************************/

/*
 * @unimplemented
 */
HRESULT
WINAPI
GetApplicationRecoveryCallback(IN HANDLE hProcess,
                               OUT APPLICATION_RECOVERY_CALLBACK* pRecoveryCallback,
                               OUT PVOID* ppvParameter,
                               PDWORD dwPingInterval,
                               PDWORD dwFlags)
{
    UNIMPLEMENTED;
    return E_FAIL;
}


/*
 * @unimplemented
 */
HRESULT
WINAPI
GetApplicationRestart(IN HANDLE hProcess,
                      OUT PWSTR pwzCommandline  OPTIONAL,
                      IN OUT PDWORD pcchSize,
                      OUT PDWORD pdwFlags  OPTIONAL)
{
    UNIMPLEMENTED;
    return E_FAIL;
}


/*
 * @unimplemented
 */
VOID
WINAPI
RecoveryFinished(IN BOOL bSuccess)
{
    UNIMPLEMENTED;
}


/*
 * @unimplemented
 */
HRESULT
WINAPI
RecoveryInProgress(OUT PBOOL pbCancelled)
{
    UNIMPLEMENTED;
    return E_FAIL;
}


/*
 * @unimplemented
 */
HRESULT
WINAPI
RegisterApplicationRecoveryCallback(IN APPLICATION_RECOVERY_CALLBACK pRecoveyCallback,
                                    IN PVOID pvParameter  OPTIONAL,
                                    DWORD dwPingInterval,
                                    DWORD dwFlags)
{
    UNIMPLEMENTED;
    return E_FAIL;
}


/*
 * @unimplemented
 */
HRESULT
WINAPI
RegisterApplicationRestart(IN PCWSTR pwzCommandline  OPTIONAL,
                           IN DWORD dwFlags)
{
    UNIMPLEMENTED;
    return E_FAIL;
}

/* EOF */
