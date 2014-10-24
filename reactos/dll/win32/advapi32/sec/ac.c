/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/advapi32/sec/ac.c
 * PURPOSE:         ACL/ACE functions
 */

#include <advapi32.h>
WINE_DEFAULT_DEBUG_CHANNEL(advapi);

/* --- ACE --- */

/*
 * @implemented
 */
BOOL
WINAPI
AddAccessAllowedObjectAce(PACL pAcl,
                          DWORD dwAceRevision,
                          DWORD AceFlags,
                          DWORD AccessMask,
                          GUID *ObjectTypeGuid,
                          GUID *InheritedObjectTypeGuid,
                          PSID pSid)
{
    NTSTATUS Status;

    Status = RtlAddAccessAllowedObjectAce(pAcl,
                                          dwAceRevision,
                                          AceFlags,
                                          AccessMask,
                                          ObjectTypeGuid,
                                          InheritedObjectTypeGuid,
                                          pSid);
    if (!NT_SUCCESS(Status))
    {
        SetLastError(RtlNtStatusToDosError(Status));
        return FALSE;
    }

    return TRUE;
}

/*
 * @implemented
 */
BOOL
WINAPI
AddAccessDeniedObjectAce(PACL pAcl,
                         DWORD dwAceRevision,
                         DWORD AceFlags,
                         DWORD AccessMask,
                         GUID* ObjectTypeGuid,
                         GUID* InheritedObjectTypeGuid,
                         PSID pSid)
{
    NTSTATUS Status;

    Status = RtlAddAccessDeniedObjectAce(pAcl,
                                         dwAceRevision,
                                         AceFlags,
                                         AccessMask,
                                         ObjectTypeGuid,
                                         InheritedObjectTypeGuid,
                                         pSid);
    if (!NT_SUCCESS(Status))
    {
        SetLastError(RtlNtStatusToDosError(Status));
        return FALSE;
    }

    return TRUE;
}

/*
 * @implemented
 */
BOOL
WINAPI
AddAuditAccessObjectAce(PACL pAcl,
                        DWORD dwAceRevision,
                        DWORD AceFlags,
                        DWORD AccessMask,
                        GUID *ObjectTypeGuid,
                        GUID *InheritedObjectTypeGuid,
                        PSID pSid,
                        BOOL bAuditSuccess,
                        BOOL bAuditFailure)
{
    NTSTATUS Status;

    Status = RtlAddAuditAccessObjectAce(pAcl,
                                        dwAceRevision,
                                        AceFlags,
                                        AccessMask,
                                        ObjectTypeGuid,
                                        InheritedObjectTypeGuid,
                                        pSid,
                                        bAuditSuccess,
                                        bAuditFailure);
    if (!NT_SUCCESS(Status))
    {
        SetLastError(RtlNtStatusToDosError(Status));
        return FALSE;
    }

    return TRUE;
}

/*
 * @implemented
 */
DWORD
WINAPI
GetInheritanceSourceW(LPWSTR pObjectName,
                      SE_OBJECT_TYPE ObjectType,
                      SECURITY_INFORMATION SecurityInfo,
                      BOOL Container,
                      GUID **pObjectClassGuids  OPTIONAL,
                      DWORD GuidCount,
                      PACL pAcl,
                      PFN_OBJECT_MGR_FUNCTS pfnArray  OPTIONAL,
                      PGENERIC_MAPPING pGenericMapping,
                      PINHERITED_FROMW pInheritArray)
{
    DWORD ErrorCode;

    ErrorCode = CheckNtMartaPresent();
    if (ErrorCode == ERROR_SUCCESS)
    {
        /* call the MARTA provider */
        ErrorCode = AccGetInheritanceSource(pObjectName,
                                            ObjectType,
                                            SecurityInfo,
                                            Container,
                                            pObjectClassGuids,
                                            GuidCount,
                                            pAcl,
                                            pfnArray,
                                            pGenericMapping,
                                            pInheritArray);
    }

    return ErrorCode;
}


/*
 * @unimplemented
 */
DWORD
WINAPI
GetInheritanceSourceA(LPSTR pObjectName,
                      SE_OBJECT_TYPE ObjectType,
                      SECURITY_INFORMATION SecurityInfo,
                      BOOL Container,
                      GUID **pObjectClassGuids  OPTIONAL,
                      DWORD GuidCount,
                      PACL pAcl,
                      PFN_OBJECT_MGR_FUNCTS pfnArray  OPTIONAL,
                      PGENERIC_MAPPING pGenericMapping,
                      PINHERITED_FROMA pInheritArray)
{
    /* That's all this function does, at least up to w2k3... Even MS was too
       lazy to implement it... */
    return ERROR_CALL_NOT_IMPLEMENTED;
}


/*
 * @implemented
 */
DWORD
WINAPI
FreeInheritedFromArray(PINHERITED_FROMW pInheritArray,
                       USHORT AceCnt,
                       PFN_OBJECT_MGR_FUNCTS pfnArray  OPTIONAL)
{
    DWORD ErrorCode;

    ErrorCode = CheckNtMartaPresent();
    if (ErrorCode == ERROR_SUCCESS)
    {
        /* call the MARTA provider */
        ErrorCode = AccFreeIndexArray(pInheritArray,
                                      AceCnt,
                                      pfnArray);
    }

    return ErrorCode;
}


/*
 * @implemented
 */
DWORD
WINAPI
SetEntriesInAclW(ULONG cCountOfExplicitEntries,
                 PEXPLICIT_ACCESS_W pListOfExplicitEntries,
                 PACL OldAcl,
                 PACL *NewAcl)
{
    DWORD ErrorCode;

    if (!NewAcl)
    {
        return ERROR_INVALID_PARAMETER;
    }

    ErrorCode = CheckNtMartaPresent();
    if (ErrorCode == ERROR_SUCCESS)
    {
        /* call the MARTA provider */
        ErrorCode = AccRewriteSetEntriesInAcl(cCountOfExplicitEntries,
                                              pListOfExplicitEntries,
                                              OldAcl,
                                              NewAcl);
    }

    return ErrorCode;
}


static DWORD
InternalTrusteeAToW(IN PTRUSTEE_A pTrusteeA,
                    OUT PTRUSTEE_W *pTrusteeW)
{
    TRUSTEE_FORM TrusteeForm;
    INT BufferSize = 0;
    PSTR lpStr;
    DWORD ErrorCode = ERROR_SUCCESS;

    //ASSERT(sizeof(TRUSTEE_W) == sizeof(TRUSTEE_A));

    TrusteeForm = GetTrusteeFormA(pTrusteeA);
    switch (TrusteeForm)
    {
        case TRUSTEE_IS_NAME:
        {
            /* directly copy the array, this works as the size of the EXPLICIT_ACCESS_A
               structure matches the size of the EXPLICIT_ACCESS_W version */
            lpStr = GetTrusteeNameA(pTrusteeA);
            if (lpStr != NULL)
                BufferSize = strlen(lpStr) + 1;

            *pTrusteeW = RtlAllocateHeap(RtlGetProcessHeap(),
                                         0,
                                         sizeof(TRUSTEE_W) + (BufferSize * sizeof(WCHAR)));
            if (*pTrusteeW != NULL)
            {
                RtlCopyMemory(*pTrusteeW,
                              pTrusteeA,
                              FIELD_OFFSET(TRUSTEE_A,
                                           ptstrName));

                if (lpStr != NULL)
                {
                    (*pTrusteeW)->ptstrName = (PWSTR)((*pTrusteeW) + 1);

                    /* convert the trustee's name */
                    if (MultiByteToWideChar(CP_ACP,
                                            0,
                                            lpStr,
                                            -1,
                                            (*pTrusteeW)->ptstrName,
                                            BufferSize) == 0)
                    {
                        goto ConvertErr;
                    }
                }
                else
                {
                    RtlFreeHeap(RtlGetProcessHeap(),
                                0,
                                *pTrusteeW);
                    goto NothingToConvert;
                }
            }
            else
                ErrorCode = ERROR_NOT_ENOUGH_MEMORY;
            break;
        }

        case TRUSTEE_IS_OBJECTS_AND_NAME:
        {
            POBJECTS_AND_NAME_A oanA = (POBJECTS_AND_NAME_A)GetTrusteeNameA(pTrusteeA);
            POBJECTS_AND_NAME_W oan;
            PWSTR StrBuf;

            /* calculate the size needed */
            if ((oanA->ObjectsPresent & ACE_INHERITED_OBJECT_TYPE_PRESENT) &&
                oanA->InheritedObjectTypeName != NULL)
            {
                BufferSize = strlen(oanA->InheritedObjectTypeName) + 1;
            }
            if (oanA->ptstrName != NULL)
            {
                BufferSize += strlen(oanA->ptstrName) + 1;
            }

            *pTrusteeW = RtlAllocateHeap(RtlGetProcessHeap(),
                                         0,
                                         sizeof(TRUSTEE_W) + sizeof(OBJECTS_AND_NAME_W) +
                                             (BufferSize * sizeof(WCHAR)));

            if (*pTrusteeW != NULL)
            {
                oan = (POBJECTS_AND_NAME_W)((*pTrusteeW) + 1);
                StrBuf = (PWSTR)(oan + 1);

                /* copy over the parts of the TRUSTEE structure that don't need
                   to be touched */
                RtlCopyMemory(*pTrusteeW,
                              pTrusteeA,
                              FIELD_OFFSET(TRUSTEE_A,
                                           ptstrName));

                (*pTrusteeW)->ptstrName = (LPWSTR)oan;

                /* convert the OBJECTS_AND_NAME_A structure */
                oan->ObjectsPresent = oanA->ObjectsPresent;
                oan->ObjectType = oanA->ObjectType;

                if ((oanA->ObjectsPresent & ACE_INHERITED_OBJECT_TYPE_PRESENT) &&
                    oanA->InheritedObjectTypeName != NULL)
                {
                    /* convert inherited object type name */
                    BufferSize = strlen(oanA->InheritedObjectTypeName) + 1;

                    if (MultiByteToWideChar(CP_ACP,
                                            0,
                                            oanA->InheritedObjectTypeName,
                                            -1,
                                            StrBuf,
                                            BufferSize) == 0)
                    {
                        goto ConvertErr;
                    }
                    oan->InheritedObjectTypeName = StrBuf;

                    StrBuf += BufferSize;
                }
                else
                    oan->InheritedObjectTypeName = NULL;

                if (oanA->ptstrName != NULL)
                {
                    /* convert the trustee name */
                    BufferSize = strlen(oanA->ptstrName) + 1;

                    if (MultiByteToWideChar(CP_ACP,
                                            0,
                                            oanA->ptstrName,
                                            -1,
                                            StrBuf,
                                            BufferSize) == 0)
                    {
                        goto ConvertErr;
                    }
                    oan->ptstrName = StrBuf;
                }
                else
                    oan->ptstrName = NULL;
            }
            else
                ErrorCode = ERROR_NOT_ENOUGH_MEMORY;
            break;
        }

        default:
        {
NothingToConvert:
            /* no need to convert anything to unicode */
            *pTrusteeW = (PTRUSTEE_W)pTrusteeA;
            break;
        }
    }

    return ErrorCode;

ConvertErr:
    ErrorCode = GetLastError();

    /* cleanup */
    RtlFreeHeap(RtlGetProcessHeap(),
                0,
                *pTrusteeW);

    return ErrorCode;
}


static __inline VOID
InternalFreeConvertedTrustee(IN PTRUSTEE_W pTrusteeW,
                             IN PTRUSTEE_A pTrusteeA)
{
    if ((PVOID)pTrusteeW != (PVOID)pTrusteeA)
    {
        RtlFreeHeap(RtlGetProcessHeap(),
                    0,
                    pTrusteeW);
    }
}


static DWORD
InternalExplicitAccessAToW(IN ULONG cCountOfExplicitEntries,
                           IN PEXPLICIT_ACCESS_A pListOfExplicitEntriesA,
                           OUT PEXPLICIT_ACCESS_W *pListOfExplicitEntriesW)
{
    TRUSTEE_FORM TrusteeForm;
    SIZE_T Size;
    ULONG i;
    ULONG ObjectsAndNameCount = 0;
    PEXPLICIT_ACCESS_W peaw = NULL;
    DWORD ErrorCode = ERROR_SUCCESS;
    LPSTR lpStr;

    /* NOTE: This code assumes that the size of the TRUSTEE_A and TRUSTEE_W structure matches! */
    //ASSERT(sizeof(TRUSTEE_A) == sizeof(TRUSTEE_W));

    if (cCountOfExplicitEntries != 0)
    {
        /* calculate the size needed */
        Size = cCountOfExplicitEntries * sizeof(EXPLICIT_ACCESS_W);
        for (i = 0; i != cCountOfExplicitEntries; i++)
        {
            TrusteeForm = GetTrusteeFormA(&pListOfExplicitEntriesA[i].Trustee);

            switch (TrusteeForm)
            {
                case TRUSTEE_IS_NAME:
                {
                    lpStr = GetTrusteeNameA(&pListOfExplicitEntriesA[i].Trustee);
                    if (lpStr != NULL)
                        Size += (strlen(lpStr) + 1) * sizeof(WCHAR);
                    break;
                }

                case TRUSTEE_IS_OBJECTS_AND_NAME:
                {
                    POBJECTS_AND_NAME_A oan = (POBJECTS_AND_NAME_A)GetTrusteeNameA(&pListOfExplicitEntriesA[i].Trustee);

                    if ((oan->ObjectsPresent & ACE_INHERITED_OBJECT_TYPE_PRESENT) &&
                        oan->InheritedObjectTypeName != NULL)
                    {
                        Size += (strlen(oan->InheritedObjectTypeName) + 1) * sizeof(WCHAR);
                    }

                    if (oan->ptstrName != NULL)
                        Size += (strlen(oan->ptstrName) + 1) * sizeof(WCHAR);

                    ObjectsAndNameCount++;
                    break;
                }

                default:
                    break;
            }
        }

        /* allocate the array */
        peaw = RtlAllocateHeap(RtlGetProcessHeap(),
                               0,
                               Size);
        if (peaw != NULL)
        {
            INT BufferSize;
            POBJECTS_AND_NAME_W oan = (POBJECTS_AND_NAME_W)(peaw + cCountOfExplicitEntries);
            LPWSTR StrBuf = (LPWSTR)(oan + ObjectsAndNameCount);

            /* convert the array to unicode */
            for (i = 0; i != cCountOfExplicitEntries; i++)
            {
                peaw[i].grfAccessPermissions = pListOfExplicitEntriesA[i].grfAccessPermissions;
                peaw[i].grfAccessMode = pListOfExplicitEntriesA[i].grfAccessMode;
                peaw[i].grfInheritance = pListOfExplicitEntriesA[i].grfInheritance;

                /* convert or copy the TRUSTEE structure */
                TrusteeForm = GetTrusteeFormA(&pListOfExplicitEntriesA[i].Trustee);
                switch (TrusteeForm)
                {
                    case TRUSTEE_IS_NAME:
                    {
                        lpStr = GetTrusteeNameA(&pListOfExplicitEntriesA[i].Trustee);
                        if (lpStr != NULL)
                        {
                            /* convert the trustee name */
                            BufferSize = strlen(lpStr) + 1;

                            if (MultiByteToWideChar(CP_ACP,
                                                    0,
                                                    lpStr,
                                                    -1,
                                                    StrBuf,
                                                    BufferSize) == 0)
                            {
                                goto ConvertErr;
                            }
                            peaw[i].Trustee.ptstrName = StrBuf;

                            StrBuf += BufferSize;
                        }
                        else
                            goto RawTrusteeCopy;

                        break;
                    }

                    case TRUSTEE_IS_OBJECTS_AND_NAME:
                    {
                        POBJECTS_AND_NAME_A oanA = (POBJECTS_AND_NAME_A)GetTrusteeNameA(&pListOfExplicitEntriesA[i].Trustee);

                        /* copy over the parts of the TRUSTEE structure that don't need
                           to be touched */
                        RtlCopyMemory(&peaw[i].Trustee,
                                      &pListOfExplicitEntriesA[i].Trustee,
                                      FIELD_OFFSET(TRUSTEE_A,
                                                   ptstrName));

                        peaw[i].Trustee.ptstrName = (LPWSTR)oan;

                        /* convert the OBJECTS_AND_NAME_A structure */
                        oan->ObjectsPresent = oanA->ObjectsPresent;
                        oan->ObjectType = oanA->ObjectType;

                        if ((oanA->ObjectsPresent & ACE_INHERITED_OBJECT_TYPE_PRESENT) &&
                            oanA->InheritedObjectTypeName != NULL)
                        {
                            /* convert inherited object type name */
                            BufferSize = strlen(oanA->InheritedObjectTypeName) + 1;

                            if (MultiByteToWideChar(CP_ACP,
                                                    0,
                                                    oanA->InheritedObjectTypeName,
                                                    -1,
                                                    StrBuf,
                                                    BufferSize) == 0)
                            {
                                goto ConvertErr;
                            }
                            oan->InheritedObjectTypeName = StrBuf;

                            StrBuf += BufferSize;
                        }
                        else
                            oan->InheritedObjectTypeName = NULL;

                        if (oanA->ptstrName != NULL)
                        {
                            /* convert the trustee name */
                            BufferSize = strlen(oanA->ptstrName) + 1;

                            if (MultiByteToWideChar(CP_ACP,
                                                    0,
                                                    oanA->ptstrName,
                                                    -1,
                                                    StrBuf,
                                                    BufferSize) == 0)
                            {
                                goto ConvertErr;
                            }
                            oan->ptstrName = StrBuf;

                            StrBuf += BufferSize;
                        }
                        else
                            oan->ptstrName = NULL;

                        /* move on to the next OBJECTS_AND_NAME_A structure */
                        oan++;
                        break;
                    }

                    default:
                    {
RawTrusteeCopy:
                        /* just copy over the TRUSTEE structure, they don't contain any
                           ansi/unicode specific data */
                        RtlCopyMemory(&peaw[i].Trustee,
                                      &pListOfExplicitEntriesA[i].Trustee,
                                      sizeof(TRUSTEE_A));
                        break;
                    }
                }
            }

            ASSERT(ErrorCode == ERROR_SUCCESS);
            *pListOfExplicitEntriesW = peaw;
        }
        else
            ErrorCode = ERROR_NOT_ENOUGH_MEMORY;
    }

    return ErrorCode;

ConvertErr:
    ErrorCode = GetLastError();

    /* cleanup */
    RtlFreeHeap(RtlGetProcessHeap(),
                0,
                peaw);

    return ErrorCode;
}


/*
 * @implemented
 */
DWORD
WINAPI
SetEntriesInAclA(ULONG cCountOfExplicitEntries,
                 PEXPLICIT_ACCESS_A pListOfExplicitEntries,
                 PACL OldAcl,
                 PACL *NewAcl)
{
    PEXPLICIT_ACCESS_W ListOfExplicitEntriesW = NULL;
    DWORD ErrorCode;

    ErrorCode = InternalExplicitAccessAToW(cCountOfExplicitEntries,
                                           pListOfExplicitEntries,
                                           &ListOfExplicitEntriesW);
    if (ErrorCode == ERROR_SUCCESS)
    {
        ErrorCode = SetEntriesInAclW(cCountOfExplicitEntries,
                                     ListOfExplicitEntriesW,
                                     OldAcl,
                                     NewAcl);

        /* free the allocated array */
        RtlFreeHeap(RtlGetProcessHeap(),
                    0,
                    ListOfExplicitEntriesW);
    }

    return ErrorCode;
}


/*
 * @implemented
 */
DWORD
WINAPI
GetExplicitEntriesFromAclW(PACL pacl,
                           PULONG pcCountOfExplicitEntries,
                           PEXPLICIT_ACCESS_W *pListOfExplicitEntries)
{
    DWORD ErrorCode;

    ErrorCode = CheckNtMartaPresent();
    if (ErrorCode == ERROR_SUCCESS)
    {
        /* call the MARTA provider */
        ErrorCode = AccRewriteGetExplicitEntriesFromAcl(pacl,
                                                        pcCountOfExplicitEntries,
                                                        pListOfExplicitEntries);
    }

    return ErrorCode;
}


/*
 * @unimplemented
 */
DWORD
WINAPI
GetEffectiveRightsFromAclW(IN PACL pacl,
                           IN PTRUSTEE_W pTrustee,
                           OUT PACCESS_MASK pAccessRights)
{
    FIXME("%p %p %p - stub\n", pacl, pTrustee, pAccessRights);

    *pAccessRights = STANDARD_RIGHTS_ALL | SPECIFIC_RIGHTS_ALL;
    return ERROR_SUCCESS;
}


/*
 * @implemented
 */
DWORD
WINAPI
GetEffectiveRightsFromAclA(IN PACL pacl,
                           IN PTRUSTEE_A pTrustee,
                           OUT PACCESS_MASK pAccessRights)
{
    PTRUSTEE_W pTrusteeW = NULL;
    DWORD ErrorCode;

    ErrorCode = InternalTrusteeAToW(pTrustee,
                                    &pTrusteeW);
    if (ErrorCode == ERROR_SUCCESS)
    {
        ErrorCode = GetEffectiveRightsFromAclW(pacl,
                                               pTrusteeW,
                                               pAccessRights);

        InternalFreeConvertedTrustee(pTrusteeW,
                                     pTrustee);
    }
    else
        ErrorCode = ERROR_NOT_ENOUGH_MEMORY;

    return ErrorCode;
}


/*
 * @unimplemented
 */
DWORD
WINAPI
GetAuditedPermissionsFromAclW(IN PACL pacl,
                              IN PTRUSTEE_W pTrustee,
                              OUT PACCESS_MASK pSuccessfulAuditedRights,
                              OUT PACCESS_MASK pFailedAuditRights)
{
    FIXME("%s() not implemented!\n", __FUNCTION__);
    return ERROR_CALL_NOT_IMPLEMENTED;
}


/*
 * @implemented
 */
DWORD
WINAPI
GetAuditedPermissionsFromAclA(IN PACL pacl,
                              IN PTRUSTEE_A pTrustee,
                              OUT PACCESS_MASK pSuccessfulAuditedRights,
                              OUT PACCESS_MASK pFailedAuditRights)
{
    PTRUSTEE_W pTrusteeW = NULL;
    DWORD ErrorCode;

    ErrorCode = InternalTrusteeAToW(pTrustee,
                                    &pTrusteeW);
    if (ErrorCode == ERROR_SUCCESS)
    {
        ErrorCode = GetAuditedPermissionsFromAclW(pacl,
                                                  pTrusteeW,
                                                  pSuccessfulAuditedRights,
                                                  pFailedAuditRights);

        InternalFreeConvertedTrustee(pTrusteeW,
                                     pTrustee);
    }
    else
        ErrorCode = ERROR_NOT_ENOUGH_MEMORY;

    return ErrorCode;
}

/* EOF */
