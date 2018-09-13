//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       ace.cpp
//
//  This file contains the implementation of the CAce class
//
//--------------------------------------------------------------------------

#include "aclpriv.h"
#include "sddl.h"       // ConvertSidToStringSid


CAce::CAce(PACE_HEADER pAce)
{
    ULONG nSidLength = 0;
    ULONG nAceLength = SIZEOF(KNOWN_ACE) - SIZEOF(ULONG);

    ZeroMemory(this, SIZEOF(CAce));
    sidType = SidTypeInvalid;

    if (pAce != NULL)
    {
        PSID psidT;

        // Copy the header and mask
        *(PACE_HEADER)this = *pAce;
        Mask = ((PKNOWN_ACE)pAce)->Mask;

        // Is this an object ACE?
        if (IsObjectAceType(pAce))
        {
            GUID *pGuid;

            nAceLength = SIZEOF(KNOWN_OBJECT_ACE) - SIZEOF(ULONG);

            // Copy the object type guid if present
            pGuid = RtlObjectAceObjectType(pAce);
            if (pGuid)
            {
                Flags |= ACE_OBJECT_TYPE_PRESENT;
                ObjectType = *pGuid;
                nAceLength += SIZEOF(GUID);
            }

            // Copy the inherit type guid if present
            pGuid = RtlObjectAceInheritedObjectType(pAce);
            if (pGuid)
            {
                Flags |= ACE_INHERITED_OBJECT_TYPE_PRESENT;
                InheritedObjectType = *pGuid;
                nAceLength += SIZEOF(GUID);
            }
        }

        // Copy the SID
        psidT = GetAceSid(pAce);
        nSidLength = GetLengthSid(psidT);

        psid = (PSID)LocalAlloc(LPTR, nSidLength);
        if (psid)
            CopyMemory(psid, psidT, nSidLength);
    }

    AceSize = (USHORT)(nAceLength + nSidLength);
}


CAce::~CAce()
{
    if (psid != NULL)
        LocalFree(psid);
    LocalFreeString(&pszName);
    LocalFreeString(&pszType);
    LocalFreeString(&pszAccessType);
    LocalFreeString(&pszInheritType);
}


LPTSTR
CAce::LookupName(LPCTSTR pszServer, LPSECURITYINFO2 psi2)
{
    if (SidTypeInvalid == sidType)
    {
        PUSER_LIST pUserList = NULL;
        LPCTSTR pszN = NULL;
        LPCTSTR pszL = NULL;

        sidType = SidTypeUnknown;

        if (LookupSid(psid, pszServer, psi2, &pUserList))
        {
            sidType = pUserList->rgUsers[0].SidType;
            pszN = pUserList->rgUsers[0].pszName;
            pszL = pUserList->rgUsers[0].pszLogonName;
        }

        SetName(pszN, pszL);

        if (pUserList)
            LocalFree(pUserList);
    }

    return pszName;
}


void
CAce::SetName(LPCTSTR pszN, LPCTSTR pszL)
{
    LocalFreeString(&pszName);
    if (!BuildUserDisplayName(&pszName, pszN, pszL) && psid)
        ConvertSidToStringSid(psid, &pszName);
}


void
CAce::SetSid(PSID p, LPCTSTR pszName, LPCTSTR pszLogonName, SID_NAME_USE type)
{
    ULONG nSidLength = 0;
    ULONG nAceLength = SIZEOF(KNOWN_ACE) - SIZEOF(ULONG);

    if (psid != NULL)
    {
        LocalFree(psid);
        psid = NULL;
    }

    if (p != NULL)
    {
        nSidLength = GetLengthSid(p);

        psid = (PSID)LocalAlloc(LPTR, nSidLength);
        if (psid)
            CopyMemory(psid, p, nSidLength);
    }

    if (Flags != 0)
    {
        nAceLength = SIZEOF(KNOWN_OBJECT_ACE) - SIZEOF(ULONG);

        if (Flags & ACE_OBJECT_TYPE_PRESENT)
            nAceLength += SIZEOF(GUID);

        if (Flags & ACE_INHERITED_OBJECT_TYPE_PRESENT)
            nAceLength += SIZEOF(GUID);
    }

    AceSize = (USHORT)(nAceLength + nSidLength);

    sidType = type;
    SetName(pszName, pszLogonName);
}


void
CAce::SetString(LPTSTR *ppszDest, LPCTSTR pszSrc)
{
    LocalFreeString(ppszDest);
    if (NULL != pszSrc)
        LocalAllocString(ppszDest, pszSrc);
}


PACE_HEADER
CAce::Copy() const
{
    PACE_HEADER pAceCopy = (PACE_HEADER)LocalAlloc(LPTR, AceSize);
    CopyTo(pAceCopy);
    return pAceCopy;
}


void
CAce::CopyTo(PACE_HEADER pAceDest) const
{
    if (pAceDest)
    {
        ULONG nAceLength = SIZEOF(KNOWN_ACE) - SIZEOF(ULONG);
        ULONG nSidLength;

        // Copy the header and mask
        *pAceDest = *(PACE_HEADER)this;
        ((PKNOWN_ACE)pAceDest)->Mask = Mask;

        // Is this an object ACE?
        if (IsObjectAceType(this))
        {
            GUID *pGuid;

            nAceLength = SIZEOF(KNOWN_OBJECT_ACE) - SIZEOF(ULONG);

            // Copy the object flags
            ((PKNOWN_OBJECT_ACE)pAceDest)->Flags = Flags;

            // Copy the object type guid if present
            pGuid = RtlObjectAceObjectType(pAceDest);
            if (pGuid)
            {
                *pGuid = ObjectType;
                nAceLength += SIZEOF(GUID);
            }

            // Copy the inherit type guid if present
            pGuid = RtlObjectAceInheritedObjectType(pAceDest);
            if (pGuid)
            {
                *pGuid = InheritedObjectType;
                nAceLength += SIZEOF(GUID);
            }
        }

        // Copy the SID
        nSidLength = GetLengthSid(psid);
        CopyMemory(GetAceSid(pAceDest), psid, nSidLength);

        // The size should already be correct, but set it here to be sure.
        pAceDest->AceSize = (USHORT)(nAceLength + nSidLength);
    }
}


int
CAce::CompareType(const CAce *pAceCompare) const
{
    //
    // Determine which ACE preceeds the other in canonical ordering.
    //
    // Return negative if this ACE preceeds pAceCompare, positive if
    // pAceCompare preceeds this ACE, and 0 if they are equivalent in
    // canonical ordering.
    //
    BOOL b1;
    BOOL b2;

    //
    // First check inheritance. Inherited ACEs follow non-inherited ACEs.
    //
    b1 = AceFlags & INHERITED_ACE;
    b2 = pAceCompare->AceFlags & INHERITED_ACE;

    if (b1 != b2)
    {
        // One (and only one) of the ACEs is inherited.
        return (b1 ? 1 : -1);
    }

    //
    // Next, Allow ACEs follow Deny ACEs.
    // Note that allow/deny has no effect on the ordering of Audit ACEs.
    //
    b1 = (AceType == ACCESS_ALLOWED_ACE_TYPE ||
          AceType == ACCESS_ALLOWED_OBJECT_ACE_TYPE);
    b2 = (pAceCompare->AceType == ACCESS_ALLOWED_ACE_TYPE ||
          pAceCompare->AceType == ACCESS_ALLOWED_OBJECT_ACE_TYPE);

    if (b1 != b2)
    {
        // One of the ACEs is an Allow ACE.
        return (b1 ? 1 : -1);
    }

    //
    // Next, Object ACEs follow non-object ACEs.
    //
    b1 = (AceType >= ACCESS_MIN_MS_OBJECT_ACE_TYPE &&
          AceType <= ACCESS_MAX_MS_OBJECT_ACE_TYPE);
    b2 = (pAceCompare->AceType >= ACCESS_MIN_MS_OBJECT_ACE_TYPE &&
          pAceCompare->AceType <= ACCESS_MAX_MS_OBJECT_ACE_TYPE);

    if (b1 != b2)
    {
        // One of the ACEs is an Object ACE.
        return (b1 ? 1 : -1);
    }

    return 0;
}


DWORD
CAce::Merge(const CAce *pAce2)
{
    DWORD dwStatus;
    DWORD dwMergeFlags = 0;
    DWORD dwResult;

    if (pAce2 == NULL)
        return MERGE_FAIL;

    //
    // The ACEs have to be the same basic type and have the same SID or
    // there's no hope.
    //
    if (!IsEqualACEType(AceType, pAce2->AceType) ||
        !EqualSid(psid, pAce2->psid))
        return MERGE_FAIL;

    if (!IsEqualGUID(InheritedObjectType, pAce2->InheritedObjectType))
        return MERGE_FAIL;  // incompatible inherit object types

    if (Flags & ACE_OBJECT_TYPE_PRESENT)
        dwMergeFlags |= MF_OBJECT_TYPE_1_PRESENT;

    if (pAce2->Flags & ACE_OBJECT_TYPE_PRESENT)
        dwMergeFlags |= MF_OBJECT_TYPE_2_PRESENT;

    if (IsEqualGUID(ObjectType, pAce2->ObjectType))
        dwMergeFlags |= MF_OBJECT_TYPE_EQUAL;

    if (IsAuditAlarmACE(AceType))
        dwMergeFlags |= MF_AUDIT_ACE_TYPE;

    dwStatus = MergeAceHelper(AceFlags,
                              Mask,
                              pAce2->AceFlags,
                              pAce2->Mask,
                              dwMergeFlags,
                              &dwResult);

    switch (dwStatus)
    {
    case MERGE_MODIFIED_FLAGS:
        AceFlags = (UCHAR)dwResult;
        break;

    case MERGE_MODIFIED_MASK:
        Mask = dwResult;
        break;
    }

    return dwStatus;
}


BOOL
IsEqualACEType(DWORD dwType1, DWORD dwType2)
{
    if (dwType1 >= ACCESS_MIN_MS_OBJECT_ACE_TYPE &&
        dwType1 <= ACCESS_MAX_MS_OBJECT_ACE_TYPE)
        dwType1 -= (ACCESS_ALLOWED_OBJECT_ACE_TYPE - ACCESS_ALLOWED_ACE_TYPE);

    if (dwType2 >= ACCESS_MIN_MS_OBJECT_ACE_TYPE &&
        dwType2 <= ACCESS_MAX_MS_OBJECT_ACE_TYPE)
        dwType2 -= (ACCESS_ALLOWED_OBJECT_ACE_TYPE - ACCESS_ALLOWED_ACE_TYPE);

    return (dwType1 == dwType2);
}


DWORD
MergeAceHelper(DWORD dwAceFlags1,
               DWORD dwMask1,
               DWORD dwAceFlags2,
               DWORD dwMask2,
               DWORD dwMergeFlags,
               LPDWORD pdwResult)
{
    // Assumptions:
    //   The ACEs are the same basic type.
    //   The SIDs are the same for both.
    //   The Inherit object type is the same for both.

    if (pdwResult == NULL)
        return MERGE_FAIL;

    *pdwResult = 0;

    if (dwMergeFlags & MF_OBJECT_TYPE_EQUAL)
    {
        if (dwAceFlags1 == dwAceFlags2)
        {
            //
            // Everything matches except maybe the mask, which
            // can be combined here.
            //
            if (AllFlagsOn(dwMask1, dwMask2))
                return MERGE_OK_1;
            else if (AllFlagsOn(dwMask2, dwMask1))
                return MERGE_OK_2;

            *pdwResult = dwMask1 | dwMask2;
            return MERGE_MODIFIED_MASK;
        }
        else if (dwMergeFlags & MF_AUDIT_ACE_TYPE)
        {
            // If 2 audit aces are identical except for the audit
            // type (success/fail), the flags can be combined.
            if ((dwAceFlags1 & VALID_INHERIT_FLAGS) == (dwAceFlags2 & VALID_INHERIT_FLAGS) &&
                dwMask1 == dwMask2)
            {
                *pdwResult = dwAceFlags1 | dwAceFlags2;
                return MERGE_MODIFIED_FLAGS;
            }
        }
        else if ((dwAceFlags1 & (NO_PROPAGATE_INHERIT_ACE | INHERITED_ACE))
                    == (dwAceFlags2 & (NO_PROPAGATE_INHERIT_ACE | INHERITED_ACE)))
        {
            // The NO_PROPAGATE_INHERIT_ACE bit is the same for both
            if (dwMask1 == dwMask2)
            {
                // The masks are the same, so we can combine inherit flags
                *pdwResult = dwAceFlags1;

                // INHERIT_ONLY_ACE should be turned on only if it is
                // already on in both ACEs, otherwise leave it off.
                if (!(dwAceFlags2 & INHERIT_ONLY_ACE))
                    *pdwResult &= ~INHERIT_ONLY_ACE;

                // Combine the remaining inherit flags and return
                *pdwResult |= dwAceFlags2 & (OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE);
                return MERGE_MODIFIED_FLAGS;
            }
            else if (AllFlagsOn(dwMask1, dwMask2))
            {
                // mask1 contains mask2. If Ace1 is inherited onto all of the
                // same things that Ace2 is, then Ace2 is redundant.
                if ((!(dwAceFlags1 & INHERIT_ONLY_ACE) || (dwAceFlags2 & INHERIT_ONLY_ACE))
                    && AllFlagsOn(dwAceFlags1 & ACE_INHERIT_ALL, dwAceFlags2 & ACE_INHERIT_ALL))
                    return MERGE_OK_1;
            }
            else if (AllFlagsOn(dwMask2, dwMask1))
            {
                // Same as above, reversed.
                if ((!(dwAceFlags2 & INHERIT_ONLY_ACE) || (dwAceFlags1 & INHERIT_ONLY_ACE))
                    && AllFlagsOn(dwAceFlags2 & ACE_INHERIT_ALL, dwAceFlags1 & ACE_INHERIT_ALL))
                    return MERGE_OK_2;
            }
        }
    }
    else if (dwAceFlags1 == dwAceFlags2)
    {
        if (!(dwMergeFlags & MF_OBJECT_TYPE_1_PRESENT) &&
                 AllFlagsOn(dwMask1, dwMask2))
        {
            //
            // The other ACE has a non-NULL object type but this ACE has no object
            // type and a mask that includes all of the bits in the other one.
            // I.e. This ACE implies the other ACE.
            //
            return MERGE_OK_1;
        }
        else if (!(dwMergeFlags & MF_OBJECT_TYPE_2_PRESENT) &&
                 AllFlagsOn(dwMask2, dwMask1))
        {
            //
            // This ACE has a non-NULL object type but the other ACE has no object
            // type and a mask that includes all of the bits in this one.
            // I.e. The other ACE implies this ACE.
            //
            return MERGE_OK_2;
        }
    }

    return MERGE_FAIL;
}
