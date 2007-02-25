/*
 * ReactOS Authorization Framework
 * Copyright (C) 2005 - 2006 ReactOS Team
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
/*
 * PROJECT:         ReactOS Authorization Framework
 * FILE:            lib/authz/clictx.c
 * PURPOSE:         Authorization Framework
 * PROGRAMMER:      Thomas Weidenmueller <w3seek@reactos.com>
 *
 * UPDATE HISTORY:
 *      10/07/2005  Created
 */
#include <precomp.h>


/*
 * @unimplemented
 */
AUTHZAPI
BOOL
WINAPI
AuthzInitializeContextFromSid(IN DWORD Flags,
                              IN PSID UserSid,
                              IN AUTHZ_RESOURCE_MANAGER_HANDLE AuthzResourceManager,
                              IN PLARGE_INTEGER pExpirationTime,
                              IN LUID Identifier,
                              IN PVOID DynamicGroupArgs,
                              OUT PAUTHZ_CLIENT_CONTEXT_HANDLE pAuthzClientContext)
{
    BOOL Ret = FALSE;

    if (AuthzResourceManager != NULL && pExpirationTime != NULL && pAuthzClientContext != NULL &&
        UserSid != NULL && IsValidSid(UserSid) && !(Flags & (AUTHZ_SKIP_TOKEN_GROUPS | AUTHZ_REQUIRE_S4U_LOGON)))
    {
        PAUTHZ_CLIENT_CONTEXT ClientCtx;
        //PAUTHZ_RESMAN ResMan = (PAUTHZ_RESMAN)AuthzResourceManager;
        
        VALIDATE_RESMAN_HANDLE(AuthzResourceManager);

        ClientCtx = (PAUTHZ_CLIENT_CONTEXT)LocalAlloc(LMEM_FIXED,
                                                      sizeof(AUTHZ_CLIENT_CONTEXT));
        if (ClientCtx != NULL)
        {
            DWORD SidLen;

            /* initialize the client context structure */
#if DBG
            ClientCtx->Tag = CLIENTCTX_TAG;
#endif

            /* simply copy the SID */
            SidLen = GetLengthSid(UserSid);
            ClientCtx->UserSid = (PSID)LocalAlloc(LMEM_FIXED,
                                                  SidLen);
            if (ClientCtx->UserSid == NULL)
            {
                LocalFree((HLOCAL)ClientCtx);
                goto FailNoMemory;
            }
            CopySid(SidLen,
                    ClientCtx->UserSid,
                    UserSid);

            ClientCtx->AuthzResourceManager = AuthzResourceManager;
            ClientCtx->Luid = Identifier;
            ClientCtx->ExpirationTime.QuadPart = (pExpirationTime != NULL ? pExpirationTime->QuadPart : 0);
            ClientCtx->ServerContext = NULL; /* FIXME */
            ClientCtx->DynamicGroupArgs = DynamicGroupArgs;

            /* return the client context handle */
            *pAuthzClientContext = (AUTHZ_CLIENT_CONTEXT_HANDLE)ClientCtx;
            Ret = TRUE;
        }
        else
        {
FailNoMemory:
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        }
    }
    else
        SetLastError(ERROR_INVALID_PARAMETER);

    return Ret;
}


/*
 * @unimplemented
 */
AUTHZAPI
BOOL
WINAPI
AuthzGetInformationFromContext(IN AUTHZ_CLIENT_CONTEXT_HANDLE hAuthzClientContext,
                               IN AUTHZ_CONTEXT_INFORMATION_CLASS InfoClass,
                               IN DWORD BufferSize,
                               OUT PDWORD pSizeRequired,
                               OUT PVOID Buffer)
{
    BOOL Ret = FALSE;

    if (hAuthzClientContext != NULL && pSizeRequired != NULL)
    {
        PAUTHZ_CLIENT_CONTEXT ClientCtx = (PAUTHZ_CLIENT_CONTEXT)hAuthzClientContext;
        
        VALIDATE_CLIENTCTX_HANDLE(hAuthzClientContext);
        
        switch (InfoClass)
        {
            case AuthzContextInfoUserSid:
            {
                DWORD SidLen = GetLengthSid(ClientCtx->UserSid);
                *pSizeRequired = SidLen;
                if (BufferSize < SidLen)
                {
                    SetLastError(ERROR_INSUFFICIENT_BUFFER);
                }
                else
                {
                    Ret = CopySid(SidLen,
                                  (PSID)Buffer,
                                  ClientCtx->UserSid);
                }
                break;
            }

            case AuthzContextInfoGroupsSids:
                SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
                break;

            case AuthzContextInfoRestrictedSids:
                SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
                break;

            case AuthzContextInfoPrivileges:
                SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
                break;

            case AuthzContextInfoExpirationTime:
                *pSizeRequired = sizeof(LARGE_INTEGER);
                if (BufferSize < sizeof(LARGE_INTEGER) || Buffer == NULL)
                {
                    SetLastError(ERROR_INSUFFICIENT_BUFFER);
                }
                else
                {
                    *((PLARGE_INTEGER)Buffer) = ClientCtx->ExpirationTime;
                    Ret = TRUE;
                }
                break;

            case AuthzContextInfoServerContext:
                *pSizeRequired = sizeof(AUTHZ_CLIENT_CONTEXT_HANDLE);
                if (BufferSize < sizeof(AUTHZ_CLIENT_CONTEXT_HANDLE) || Buffer == NULL)
                {
                    SetLastError(ERROR_INSUFFICIENT_BUFFER);
                }
                else
                {
                    *((PAUTHZ_CLIENT_CONTEXT_HANDLE)Buffer) = ClientCtx->ServerContext;
                    Ret = TRUE;
                }
                break;

            case AuthzContextInfoIdentifier:
                *pSizeRequired = sizeof(LUID);
                if (BufferSize < sizeof(LUID) || Buffer == NULL)
                {
                    SetLastError(ERROR_INSUFFICIENT_BUFFER);
                }
                else
                {
                    *((PLUID)Buffer) = ClientCtx->Luid;
                    Ret = TRUE;
                }
                break;

            default:
                SetLastError(ERROR_INVALID_PARAMETER);
                break;
        }
    }
    else
        SetLastError(ERROR_INVALID_PARAMETER);

    return Ret;
}


/*
 * @implemented
 */
AUTHZAPI
BOOL
WINAPI
AuthzFreeContext(IN AUTHZ_CLIENT_CONTEXT_HANDLE AuthzClientContext)
{
    BOOL Ret = FALSE;

    if (AuthzClientContext != NULL)
    {
        PAUTHZ_CLIENT_CONTEXT ClientCtx = (PAUTHZ_CLIENT_CONTEXT)AuthzClientContext;

        VALIDATE_CLIENTCTX_HANDLE(AuthzClientContext);

        if (ClientCtx->UserSid != NULL)
        {
            LocalFree((HLOCAL)ClientCtx->UserSid);
        }

        LocalFree((HLOCAL)ClientCtx);
        Ret = TRUE;
    }
    else
        SetLastError(ERROR_INVALID_PARAMETER);

    return Ret;
}
