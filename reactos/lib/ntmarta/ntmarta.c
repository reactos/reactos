/*
 * ReactOS MARTA provider
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
/* $Id$
 *
 * PROJECT:         ReactOS MARTA provider
 * FILE:            lib/ntmarta/ntmarta.c
 * PURPOSE:         ReactOS MARTA provider
 * PROGRAMMER:      Thomas Weidenmueller <w3seek@reactos.com>
 *
 * UPDATE HISTORY:
 *      07/26/2005  Created
 */
#include <ntmarta.h>

#define NDEBUG
#include <debug.h>

HINSTANCE hDllInstance;

/**********************************************************************
 * AccRewriteGetHandleRights				EXPORTED
 *
 * @unimplemented
 */
DWORD STDCALL
AccRewriteGetHandleRights(HANDLE handle,
                          SE_OBJECT_TYPE ObjectType,
                          SECURITY_INFORMATION SecurityInfo,
                          PSID* ppsidOwner,
                          PSID* ppsidGroup,
                          PACL* ppDacl,
                          PACL* ppSacl,
                          PSECURITY_DESCRIPTOR* ppSecurityDescriptor)
{
    PSECURITY_DESCRIPTOR pSD = NULL;
    ULONG SDSize = 0;
    NTSTATUS Status;
    DWORD LastErr;
    DWORD Ret = ERROR_SUCCESS;

    /* save the last error code */
    LastErr = GetLastError();

    do
    {
        /* allocate a buffer large enough to hold the
           security descriptor we need to return */
        SDSize += 0x100;
        if (pSD == NULL)
        {
            pSD = LocalAlloc(LMEM_FIXED,
                             (SIZE_T)SDSize);
        }
        else
        {
            PSECURITY_DESCRIPTOR newSD;

            newSD = LocalReAlloc((HLOCAL)pSD,
                                 (SIZE_T)SDSize,
                                 LMEM_MOVEABLE);
            if (newSD != NULL)
                pSD = newSD;
        }

        if (pSD == NULL)
        {
            Ret = GetLastError();
            break;
        }

        /* perform the actual query depending on the object type */
        switch (ObjectType)
        {
            case SE_REGISTRY_KEY:
            {
                Ret = RegGetKeySecurity((HKEY)handle,
                                        SecurityInfo,
                                        pSD,
                                        &SDSize);
                break;
            }

            case SE_FILE_OBJECT:
            case SE_KERNEL_OBJECT:
            {
                Status = NtQuerySecurityObject(handle,
                                               SecurityInfo,
                                               pSD,
                                               SDSize,
                                               &SDSize);
                Ret = RtlNtStatusToDosError(Status);
                break;
            }

            case SE_SERVICE:
            {
                Ret = QueryServiceObjectSecurity((SC_HANDLE)handle,
                                                 SecurityInfo,
                                                 pSD,
                                                 SDSize,
                                                 &SDSize);
                break;
            }

            case SE_WINDOW_OBJECT:
            {
                Ret = GetUserObjectSecurity(handle,
                                            &SecurityInfo,
                                            pSD,
                                            SDSize,
                                            &SDSize);
                break;
            }

            default:
            {
                UNIMPLEMENTED;
                Ret = ERROR_CALL_NOT_IMPLEMENTED;
                break;
            }
        }

    } while (Ret == ERROR_INSUFFICIENT_BUFFER);

    if (Ret == ERROR_SUCCESS)
    {
        BOOL Present, Defaulted;

        if (SecurityInfo & OWNER_SECURITY_INFORMATION && ppsidOwner != NULL)
        {
            *ppsidOwner = NULL;
            if (!GetSecurityDescriptorOwner(pSD,
                                            ppsidOwner,
                                            &Defaulted))
            {
                Ret = GetLastError();
                goto Cleanup;
            }
        }

        if (SecurityInfo & GROUP_SECURITY_INFORMATION && ppsidGroup != NULL)
        {
            *ppsidOwner = NULL;
            if (!GetSecurityDescriptorGroup(pSD,
                                            ppsidGroup,
                                            &Defaulted))
            {
                Ret = GetLastError();
                goto Cleanup;
            }
        }

        if (SecurityInfo & DACL_SECURITY_INFORMATION && ppDacl != NULL)
        {
            *ppDacl = NULL;
            if (!GetSecurityDescriptorDacl(pSD,
                                           &Present,
                                           ppDacl,
                                           &Defaulted))
            {
                Ret = GetLastError();
                goto Cleanup;
            }
        }

        if (SecurityInfo & SACL_SECURITY_INFORMATION && ppSacl != NULL)
        {
            *ppSacl = NULL;
            if (!GetSecurityDescriptorSacl(pSD,
                                           &Present,
                                           ppSacl,
                                           &Defaulted))
            {
                Ret = GetLastError();
                goto Cleanup;
            }
        }

        *ppSecurityDescriptor = pSD;
    }
    else
    {
Cleanup:
        if (pSD != NULL)
        {
            LocalFree((HLOCAL)pSD);
        }
    }

    /* restore the last error code */
    SetLastError(LastErr);

    return Ret;
}


/**********************************************************************
 * AccRewriteSetHandleRights				EXPORTED
 *
 * @unimplemented
 */
DWORD STDCALL
AccRewriteSetHandleRights(HANDLE handle,
                          SE_OBJECT_TYPE ObjectType,
                          SECURITY_INFORMATION SecurityInfo,
                          PSECURITY_DESCRIPTOR pSecurityDescriptor)
{
    UNIMPLEMENTED;
    return ERROR_CALL_NOT_IMPLEMENTED;
}


/**********************************************************************
 * AccRewriteGetNamedRights				EXPORTED
 *
 * @unimplemented
 */
DWORD STDCALL
AccRewriteGetNamedRights(LPWSTR pObjectName,
                         SE_OBJECT_TYPE ObjectType,
                         SECURITY_INFORMATION SecurityInfo,
                         PSID* ppsidOwner,
                         PSID* ppsidGroup,
                         PACL* ppDacl,
                         PACL* ppSacl,
                         PSECURITY_DESCRIPTOR* ppSecurityDescriptor)
{
    UNIMPLEMENTED;
    return ERROR_CALL_NOT_IMPLEMENTED;
}


/**********************************************************************
 * AccRewriteSetNamedRights				EXPORTED
 *
 * @unimplemented
 */
DWORD STDCALL
AccRewriteSetNamedRights(LPWSTR pObjectName,
                         SE_OBJECT_TYPE ObjectType,
                         SECURITY_INFORMATION SecurityInfo,
                         PSECURITY_DESCRIPTOR pSecurityDescriptor)
{
    UNIMPLEMENTED;
    return ERROR_CALL_NOT_IMPLEMENTED;
}


/**********************************************************************
 * AccRewriteSetEntriesInAcl				EXPORTED
 *
 * @unimplemented
 */
DWORD STDCALL
AccRewriteSetEntriesInAcl(ULONG cCountOfExplicitEntries,
                          PEXPLICIT_ACCESS_W pListOfExplicitEntries,
                          PACL OldAcl,
                          PACL* NewAcl)
{
    UNIMPLEMENTED;
    return ERROR_CALL_NOT_IMPLEMENTED;
}


/**********************************************************************
 * AccRewriteSetEntriesInAcl				EXPORTED
 *
 * @unimplemented
 */
DWORD STDCALL
AccGetInheritanceSource(LPWSTR pObjectName,
                        SE_OBJECT_TYPE ObjectType,
                        SECURITY_INFORMATION SecurityInfo,
                        BOOL Container,
                        GUID** pObjectClassGuids,
                        DWORD GuidCount,
                        PACL pAcl,
                        PFN_OBJECT_MGR_FUNCTS pfnArray,
                        PGENERIC_MAPPING pGenericMapping,
                        PINHERITED_FROMW pInheritArray)
{
    UNIMPLEMENTED;
    return ERROR_CALL_NOT_IMPLEMENTED;
}


/**********************************************************************
 * AccFreeIndexArray					EXPORTED
 *
 * @unimplemented
 */
DWORD STDCALL
AccFreeIndexArray(PINHERITED_FROMW pInheritArray,
                  USHORT AceCnt,
                  PFN_OBJECT_MGR_FUNCTS pfnArray  OPTIONAL)
{
    UNIMPLEMENTED;
    return ERROR_CALL_NOT_IMPLEMENTED;
}


/**********************************************************************
 * AccRewriteGetExplicitEntriesFromAcl			EXPORTED
 *
 * @unimplemented
 */
DWORD STDCALL
AccRewriteGetExplicitEntriesFromAcl(PACL pacl,
                                    PULONG pcCountOfExplicitEntries,
                                    PEXPLICIT_ACCESS_W* pListOfExplicitEntries)
{
    UNIMPLEMENTED;
    return ERROR_CALL_NOT_IMPLEMENTED;
}


/**********************************************************************
 * AccTreeResetNamedSecurityInfo			EXPORTED
 *
 * @unimplemented
 */
DWORD STDCALL
AccTreeResetNamedSecurityInfo(LPWSTR pObjectName,
                              SE_OBJECT_TYPE ObjectType,
                              SECURITY_INFORMATION SecurityInfo,
                              PSID pOwner,
                              PSID pGroup,
                              PACL pDacl,
                              PACL pSacl,
                              BOOL KeepExplicit,
                              FN_PROGRESSW fnProgress,
                              PROG_INVOKE_SETTING ProgressInvokeSetting,
                              PVOID Args)
{
    UNIMPLEMENTED;
    return ERROR_CALL_NOT_IMPLEMENTED;
}


BOOL STDCALL
DllMain(IN HINSTANCE hinstDLL,
        IN DWORD dwReason,
        IN LPVOID lpvReserved)
{
    switch (dwReason)
    {
        case DLL_PROCESS_ATTACH:
            hDllInstance = hinstDLL;
            break;
        case DLL_THREAD_ATTACH:
        case DLL_THREAD_DETACH:
        case DLL_PROCESS_DETACH:
            break;
    }
    return TRUE;
}

