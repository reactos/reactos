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
 * FILE:            lib/authz/resman.c
 * PURPOSE:         Authorization Framework
 * PROGRAMMER:      Thomas Weidenmueller <w3seek@reactos.com>
 *
 * UPDATE HISTORY:
 *      10/07/2005  Created
 */
#include <precomp.h>


static BOOL
AuthzpQueryToken(IN OUT PAUTHZ_RESMAN ResMan,
                 IN HANDLE hToken)
{
    TOKEN_USER User;
    TOKEN_STATISTICS Statistics;
    DWORD BufLen;
    PSID UserSid = NULL;
    BOOL Ret = FALSE;

    /* query information about the user */
    BufLen = sizeof(User);
    Ret = GetTokenInformation(hToken,
                              TokenUser,
                              &User,
                              BufLen,
                              &BufLen);
    if (Ret)
    {
        BufLen = GetLengthSid(User.User.Sid);
        if (BufLen != 0)
        {
            UserSid = (PSID)LocalAlloc(LMEM_FIXED,
                                       BufLen);
            if (UserSid != NULL)
            {
                CopyMemory(UserSid,
                           User.User.Sid,
                           BufLen);
            }
            else
                Ret = FALSE;
        }
        else
            Ret = FALSE;
    }

    if (Ret)
    {
        /* query general information */
        BufLen = sizeof(Statistics);
        Ret = GetTokenInformation(hToken,
                                  TokenUser,
                                  &Statistics,
                                  BufLen,
                                  &BufLen);
    }

    if (Ret)
    {
        ResMan->UserSid = UserSid;
        ResMan->AuthenticationId = Statistics.AuthenticationId;
        Ret = TRUE;
    }
    else
    {
        if (UserSid != NULL)
        {
            LocalFree((HLOCAL)UserSid);
        }
    }

    return Ret;
}

static BOOL
AuthzpInitUnderImpersonation(IN OUT PAUTHZ_RESMAN ResMan)
{
    HANDLE hToken;
    BOOL Ret;

    Ret = OpenThreadToken(GetCurrentThread(),
                          TOKEN_QUERY,
                          TRUE,
                          &hToken);
    if (Ret)
    {
        Ret = AuthzpQueryToken(ResMan,
                               hToken);
        CloseHandle(hToken);
    }

    return Ret;
}

static BOOL
AuthzpInitSelf(IN OUT PAUTHZ_RESMAN ResMan)
{
    HANDLE hToken;
    BOOL Ret;

    Ret = OpenProcessToken(GetCurrentProcess(),
                           TOKEN_QUERY,
                           &hToken);
    if (Ret)
    {
        Ret = AuthzpQueryToken(ResMan,
                               hToken);
        CloseHandle(hToken);
    }

    return Ret;
}


/*
 * @unimplemented
 */
AUTHZAPI
BOOL
WINAPI
AuthzInitializeResourceManager(IN DWORD flags,
                               IN PFN_AUTHZ_DYNAMIC_ACCESS_CHECK pfnAccessCheck  OPTIONAL,
                               IN PFN_AUTHZ_COMPUTE_DYNAMIC_GROUPS pfnComputeDynamicGroups  OPTIONAL,
                               IN PFN_AUTHZ_FREE_DYNAMIC_GROUPS pfnFreeDynamicGroups  OPTIONAL,
                               IN PCWSTR ResourceManagerName  OPTIONAL,
                               IN PAUTHZ_RESOURCE_MANAGER_HANDLE pAuthzResourceManager)
{
    BOOL Ret = FALSE;

    if (pAuthzResourceManager != NULL &&
        !(flags & ~(AUTHZ_RM_FLAG_NO_AUDIT | AUTHZ_RM_FLAG_INITIALIZE_UNDER_IMPERSONATION)))
    {
        PAUTHZ_RESMAN ResMan;
        SIZE_T RequiredSize = sizeof(AUTHZ_RESMAN);

        if (ResourceManagerName != NULL)
        {
            RequiredSize += wcslen(ResourceManagerName) * sizeof(WCHAR);
        }

        ResMan = (PAUTHZ_RESMAN)LocalAlloc(LMEM_FIXED,
                                           RequiredSize);
        if (ResMan != NULL)
        {
            /* initialize the resource manager structure */
#if DBG
            ResMan->Tag = RESMAN_TAG;
#endif

            ResMan->flags = flags;
            ResMan->UserSid = NULL;

            if (ResourceManagerName != NULL)
            {
                wcscpy(ResMan->ResourceManagerName,
                       ResourceManagerName);
            }
            else
                ResMan->ResourceManagerName[0] = UNICODE_NULL;

            ResMan->pfnAccessCheck = pfnAccessCheck;
            ResMan->pfnComputeDynamicGroups = pfnComputeDynamicGroups;
            ResMan->pfnFreeDynamicGroups = pfnFreeDynamicGroups;

            if (!(flags & AUTHZ_RM_FLAG_NO_AUDIT))
            {
                /* FIXME - initialize auditing */
                DPRINT1("Auditing not implemented!\n");
            }

            if (flags & AUTHZ_RM_FLAG_INITIALIZE_UNDER_IMPERSONATION)
            {
                Ret = AuthzpInitUnderImpersonation(ResMan);
            }
            else
            {
                Ret = AuthzpInitSelf(ResMan);
            }

            if (Ret)
            {
                /* finally return the handle */
                *pAuthzResourceManager = (AUTHZ_RESOURCE_MANAGER_HANDLE)ResMan;
            }
            else
            {
                DPRINT1("Querying the token failed!\n");
                LocalFree((HLOCAL)ResMan);
            }
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
AuthzFreeResourceManager(IN AUTHZ_RESOURCE_MANAGER_HANDLE AuthzResourceManager)
{
    BOOL Ret = FALSE;

    if (AuthzResourceManager != NULL)
    {
        PAUTHZ_RESMAN ResMan = (PAUTHZ_RESMAN)AuthzResourceManager;

        VALIDATE_RESMAN_HANDLE(AuthzResourceManager);

        if (!(ResMan->flags & AUTHZ_RM_FLAG_NO_AUDIT))
        {
            /* FIXME - cleanup auditing */
        }

        if (ResMan->UserSid != NULL)
        {
            LocalFree((HLOCAL)ResMan->UserSid);
        }

        LocalFree((HLOCAL)AuthzResourceManager);
        Ret = TRUE;
    }
    else
        SetLastError(ERROR_INVALID_PARAMETER);

    return Ret;
}

