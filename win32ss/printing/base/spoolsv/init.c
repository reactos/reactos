/*
 * PROJECT:     ReactOS Print Spooler Service
 * LICENSE:     GNU GPLv2 or any later version as published by the Free Software Foundation
 * PURPOSE:     Various initialization functions
 * COPYRIGHT:   Copyright 2015-2017 Colin Finck <colin@reactos.org>
 */

#include "precomp.h"

DWORD
_RpcSpoolerInit()
{
    DWORD dwErrorCode;

    // Call SpoolerInit in the security context of the client.
    // This delay-loads spoolss.dll in the user context and all further calls to functions in spoolss.dll will be done in the user context as well.
    dwErrorCode = RpcImpersonateClient(NULL);
    if (dwErrorCode != ERROR_SUCCESS)
    {
        ERR("RpcImpersonateClient failed with error %lu!\n", dwErrorCode);
        return dwErrorCode;
    }

    if (!SpoolerInit())
        dwErrorCode = GetLastError();

    RpcRevertToSelf();
    return dwErrorCode;
}
