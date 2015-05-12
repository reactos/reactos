/*
 * PROJECT:     ReactOS Print Spooler Service
 * LICENSE:     GNU GPLv2 or any later version as published by the Free Software Foundation
 * PURPOSE:     Various initialization functions
 * COPYRIGHT:   Copyright 2015 Colin Finck <colin@reactos.org>
 */

#include "precomp.h"

DWORD
_RpcSpoolerInit()
{
    DWORD ErrorCode;

    // Call SpoolerInit in the security context of the client.
    // This delay-loads spoolss.dll in the user context and all further calls to functions in spoolss.dll will be done in the user context as well.
    ErrorCode = RpcImpersonateClient(NULL);
    if (ErrorCode != ERROR_SUCCESS)
    {
        ERR("RpcImpersonateClient failed with status %u!\n", ErrorCode);
        return ErrorCode;
    }

    ErrorCode = SpoolerInit();
    if (ErrorCode != ERROR_SUCCESS)
    {
        ERR("SpoolerInit failed with status %u!\n", ErrorCode);
        RpcRevertToSelf();
        return ErrorCode;
    }

    return RpcRevertToSelf();
}
