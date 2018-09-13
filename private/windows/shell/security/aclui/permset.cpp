//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       permset.cpp
//
//  This file contains the implementation for the CPermissionSet class
//
//--------------------------------------------------------------------------

#include "aclpriv.h"
#include "permset.h"


void
CPermissionSet::Reset()
{
    TraceEnter(TRACE_PERMSET, "CPermissionSet::Reset");

    // Clear the lists

    if (m_hPermList != NULL)
    {
        DSA_Destroy(m_hPermList);
        m_hPermList = NULL;
    }

    DestroyDPA(m_hAdvPermList);
    m_hAdvPermList = NULL;

    m_fObjectAcesPresent = FALSE;

    TraceLeaveVoid();
}


BOOL
CPermissionSet::AddAce(LPCGUID pguid, ACCESS_MASK mask, DWORD dwFlags)
{
    PERMISSION perm = { mask, dwFlags, 0 };

    if (pguid != NULL)
        perm.guid = *pguid;

    return AddPermission(&perm);
}


BOOL
CPermissionSet::AddPermission(PPERMISSION pPerm)
{
    BOOL bObjectTypePresent = FALSE;

    TraceEnter(TRACE_PERMSET, "CPermissionSet::AddAce");
    TraceAssert(pPerm != NULL);

    if (!IsEqualGUID(pPerm->guid, GUID_NULL))
        bObjectTypePresent = TRUE;

    if (m_hPermList == NULL)
    {
        m_hPermList = DSA_Create(SIZEOF(PERMISSION), 4);
        if (m_hPermList == NULL)
            TraceLeaveValue(FALSE);
    }
    else
    {
        //
        // Try to merge with an existing entry in the list.
        //
        UINT cItems = DSA_GetItemCount(m_hPermList);
        while (cItems > 0)
        {
            PPERMISSION pPermCompare;
            DWORD dwMergeFlags;
            DWORD dwMergeResult;
            DWORD dwMergeStatus;

            --cItems;
            pPermCompare = (PPERMISSION)DSA_GetItemPtr(m_hPermList, cItems);

            dwMergeFlags = 0;

            if (bObjectTypePresent)
                dwMergeFlags |= MF_OBJECT_TYPE_1_PRESENT;

            if (!IsEqualGUID(pPermCompare->guid, GUID_NULL))
                dwMergeFlags |= MF_OBJECT_TYPE_2_PRESENT;

            if (!(dwMergeFlags & (MF_OBJECT_TYPE_1_PRESENT | MF_OBJECT_TYPE_2_PRESENT)))
            {
                // Neither are present, so they are the same
                dwMergeFlags |= MF_OBJECT_TYPE_EQUAL;
            }
            else if (IsEqualGUID(pPermCompare->guid, pPerm->guid))
                dwMergeFlags |= MF_OBJECT_TYPE_EQUAL;

            dwMergeStatus = MergeAceHelper(pPerm->dwFlags,         // #1
                                           pPerm->mask,
                                           pPermCompare->dwFlags,  // #2
                                           pPermCompare->mask,
                                           dwMergeFlags,
                                           &dwMergeResult);

            if (dwMergeStatus == MERGE_MODIFIED_FLAGS)
            {
                pPerm->dwFlags = dwMergeResult;
                dwMergeStatus = MERGE_OK_1;
            }
            else if (dwMergeStatus == MERGE_MODIFIED_MASK)
            {
                pPerm->mask = dwMergeResult;
                dwMergeStatus = MERGE_OK_1;
            }

            if (dwMergeStatus == MERGE_OK_1)
            {
                //
                // The new permission implies the existing permission, so
                // the existing one can be removed.
                //
                DSA_DeleteItem(m_hPermList, cItems);
                //
                // Keep looking.  Maybe we can remove some more entries
                // before adding the new one.
                //
            }
            else if (dwMergeStatus == MERGE_OK_2)
            {
                //
                // The existing permission implies the new permission, so
                // there is nothing to do here.
                //
                TraceLeaveValue(TRUE);
            }
        }
    }

    // Ok, add the new permission to the list.
    DSA_AppendItem(m_hPermList, pPerm);

    if (bObjectTypePresent)
        m_fObjectAcesPresent = TRUE;

    TraceLeaveValue(TRUE);
}


BOOL
CPermissionSet::AddAdvancedAce(PACE_HEADER pAce)
{
    TraceEnter(TRACE_PERMSET, "CPermissionSet::AddAdvancedAce");
    TraceAssert(pAce != NULL);

    // Create list if necessary
    if (m_hAdvPermList == NULL)
    {
        m_hAdvPermList = DPA_Create(4);
        if (m_hAdvPermList == NULL)
        {
            TraceMsg("DPA_Create failed");
            TraceLeaveValue(FALSE);
        }
    }

    // This is as big as we need, but sometimes incoming ACEs are extra big.
    UINT nAceLen = SIZEOF(KNOWN_OBJECT_ACE) + 2*SIZEOF(GUID) - SIZEOF(DWORD)
        + GetLengthSid(GetAceSid(pAce));

    // Use the incoming AceSize only if it's smaller
    if (pAce->AceSize < nAceLen)
        nAceLen = pAce->AceSize;

    // Copy the ACE and add it to the list.
    PACE_HEADER pAceCopy = (PACE_HEADER)LocalAlloc(LMEM_FIXED, nAceLen);
    if (pAceCopy == NULL)
    {
        TraceMsg("LocalAlloc failed");
        TraceLeaveValue(FALSE);
    }

    CopyMemory(pAceCopy, pAce, nAceLen);
    pAceCopy->AceSize = (USHORT)nAceLen;
    DPA_AppendPtr(m_hAdvPermList, pAceCopy);

    TraceLeaveValue(TRUE);
}


UINT
CPermissionSet::GetPermCount(BOOL fIncludeAdvAces) const
{
    ULONG cAces = 0;

    TraceEnter(TRACE_PERMSET, "CPermissionSet::GetPermCount");

    if (m_hPermList != NULL)
        cAces = DSA_GetItemCount(m_hPermList);

    if (fIncludeAdvAces && m_hAdvPermList != NULL)
        cAces += DPA_GetPtrCount(m_hAdvPermList);

    TraceLeaveValue(cAces);
}


ULONG
CPermissionSet::GetAclLength(ULONG cbSid) const
{
    // Return an estimate of the buffer size needed to hold the
    // requested ACEs. The size of the ACL header is NOT INCLUDED.

    ULONG nAclLength = 0;
    ULONG cAces;
    ULONG nAceSize = SIZEOF(KNOWN_ACE) - SIZEOF(DWORD) + cbSid;
    ULONG nObjectAceSize = SIZEOF(KNOWN_OBJECT_ACE) + SIZEOF(GUID) - SIZEOF(DWORD) + cbSid;

    TraceEnter(TRACE_PERMSET, "CPermissionSet::GetAclLength");

    if (m_hPermList != NULL)
    {
        cAces = DSA_GetItemCount(m_hPermList);
        if (m_fObjectAcesPresent)
            nAclLength += cAces * nObjectAceSize;
        else
            nAclLength += cAces * nAceSize;
    }

    if (m_hAdvPermList != NULL)
    {
        cAces = DPA_GetPtrCount(m_hAdvPermList);
        nAclLength += cAces * (nObjectAceSize + SIZEOF(GUID));
    }

    TraceLeaveValue(nAclLength);
}


BOOL
CPermissionSet::AppendToAcl(PACL pAcl,
                            PACE_HEADER *ppAcePos,  // position to copy first ACE
                            PSID pSid,
                            BOOL fAllowAce,
                            DWORD dwFlags) const
{
    PACE_HEADER pAce;
    UINT cAces;
    DWORD dwSidSize;
    DWORD dwAceSize;
    PPERMISSION pPerm;
    UCHAR uAceType;
    PSID psidT;

    TraceEnter(TRACE_PERMSET, "CPermissionSet::AppendToAcl");
    TraceAssert(pAcl != NULL);
    TraceAssert(ppAcePos != NULL);
    TraceAssert(pSid != NULL);

    if (*ppAcePos == NULL || (ULONG_PTR)*ppAcePos < (ULONG_PTR)FirstAce(pAcl))
        *ppAcePos = (PACE_HEADER)FirstAce(pAcl);

    TraceAssert((ULONG_PTR)*ppAcePos >= (ULONG_PTR)FirstAce(pAcl) &&
                (ULONG_PTR)*ppAcePos <= (ULONG_PTR)ByteOffset(pAcl, pAcl->AclSize));

    dwSidSize = GetLengthSid(pSid);
    dwAceSize = SIZEOF(KNOWN_ACE) - SIZEOF(DWORD) + dwSidSize;
    uAceType = (UCHAR)(fAllowAce ? ACCESS_ALLOWED_ACE_TYPE : ACCESS_DENIED_ACE_TYPE);

    cAces = GetPermCount();
    while (cAces > 0)
    {
        BOOL bObjectAce;

        pPerm = (PPERMISSION)DSA_GetItemPtr(m_hPermList, --cAces);
        if (pPerm == NULL)
            continue;

        bObjectAce = !IsEqualGUID(pPerm->guid, GUID_NULL);

        if (bObjectAce && !(dwFlags & PS_OBJECT))
            continue;
        else if (!bObjectAce && !(dwFlags & PS_NONOBJECT))
            continue;

        pAce = *ppAcePos;

        // Make sure the buffer is large enough.
        if ((ULONG_PTR)ByteOffset(*ppAcePos, dwAceSize) > (ULONG_PTR)ByteOffset(pAcl, pAcl->AclSize))
        {
            TraceMsg("ACL buffer too small");
            TraceAssert(FALSE);
            TraceLeaveValue(FALSE);
        }
        TraceAssert(!IsBadWritePtr(*ppAcePos, dwAceSize));

        // Copy the header and mask
        pAce->AceType = uAceType;
        pAce->AceFlags = (UCHAR)pPerm->dwFlags;
        pAce->AceSize = (USHORT)dwAceSize;
        ((PKNOWN_ACE)pAce)->Mask = pPerm->mask;

        // Get the normal SID location
        psidT = &((PKNOWN_ACE)pAce)->SidStart;

        if (bObjectAce)
        {
            //
            // The Object ACEs that we deal with directly do not have an
            // Inherit GUID present. Those ACEs end up in m_hAdvPermList.
            //
            GUID *pGuid;

            // Adjust AceType and AceSize and set the object Flags
            pAce->AceType += ACCESS_ALLOWED_OBJECT_ACE_TYPE - ACCESS_ALLOWED_ACE_TYPE;
            pAce->AceSize += SIZEOF(KNOWN_OBJECT_ACE) - SIZEOF(KNOWN_ACE) + SIZEOF(GUID);
            ((PKNOWN_OBJECT_ACE)pAce)->Flags = ACE_OBJECT_TYPE_PRESENT;

            // Get the object type guid location
            pGuid = RtlObjectAceObjectType(pAce);

            // We just set the flag for this, so it can't be NULL
            TraceAssert(pGuid);

            // Make sure the buffer is large enough.
            if ((ULONG_PTR)ByteOffset(pAce, pAce->AceSize) > (ULONG_PTR)ByteOffset(pAcl, pAcl->AclSize))
            {
                TraceMsg("ACL buffer too small");
                TraceAssert(FALSE);
                TraceLeaveValue(FALSE);
            }
            TraceAssert(!IsBadWritePtr(pGuid, SIZEOF(GUID)));

            // Copy the object type guid
            *pGuid = pPerm->guid;

            // Get new SID location
            psidT = RtlObjectAceSid(pAce);

            // Adjust ACL revision
            if (pAcl->AclRevision < ACL_REVISION_DS)
                pAcl->AclRevision = ACL_REVISION_DS;
        }

        // Copy the SID
        TraceAssert(!IsBadWritePtr(psidT, dwSidSize));
        CopyMemory(psidT, pSid, dwSidSize);

        // Move to next ACE position
        pAcl->AceCount++;
        *ppAcePos = (PACE_HEADER)NextAce(pAce);
    }

    if ((dwFlags & PS_OBJECT) && m_hAdvPermList != NULL)
    {
        cAces = DPA_GetPtrCount(m_hAdvPermList);
        while (cAces > 0)
        {
            pAce = (PACE_HEADER)DPA_FastGetPtr(m_hAdvPermList, --cAces);
            if (pAce == NULL)
                continue;

            // Make sure the buffer is large enough.
            if ((ULONG_PTR)ByteOffset(*ppAcePos, pAce->AceSize) > (ULONG_PTR)ByteOffset(pAcl, pAcl->AclSize))
            {
                TraceMsg("ACL buffer too small");
                TraceAssert(FALSE);
                TraceLeaveValue(FALSE);
            }
            TraceAssert(!IsBadWritePtr(*ppAcePos, pAce->AceSize));

            // Copy the ACE
            CopyMemory(*ppAcePos, pAce, pAce->AceSize);

            // Adjust ACL revision
            if (IsObjectAceType(pAce) && pAcl->AclRevision < ACL_REVISION_DS)
                pAcl->AclRevision = ACL_REVISION_DS;

            // Move to next ACE position
            pAcl->AceCount++;
            *ppAcePos = (PACE_HEADER)NextAce(*ppAcePos);
        }
    }

    TraceLeaveValue(TRUE);
}


void
CPermissionSet::ConvertInheritedAces(CPermissionSet &permInherited)
{
    UINT cItems;

    TraceEnter(TRACE_PERMSET, "CPermissionSet::ConvertInheritedAces");

    if (permInherited.m_hPermList != NULL)
    {
        PPERMISSION pPerm;

        cItems = DSA_GetItemCount(permInherited.m_hPermList);
        while (cItems)
        {
            --cItems;
            pPerm = (PPERMISSION)DSA_GetItemPtr(permInherited.m_hPermList, cItems);
            if (pPerm != NULL)
            {
                pPerm->dwFlags &= ~INHERITED_ACE;
                AddPermission(pPerm);
            }
        }
    }

    if (permInherited.m_hAdvPermList != NULL)
    {
        PACE_HEADER pAceHeader;

        cItems = DPA_GetPtrCount(permInherited.m_hAdvPermList);
        while (cItems)
        {
            --cItems;
            pAceHeader = (PACE_HEADER)DPA_FastGetPtr(permInherited.m_hAdvPermList, cItems);
            if (pAceHeader != NULL)
            {
                pAceHeader->AceFlags &= ~INHERITED_ACE;
                AddAdvancedAce(pAceHeader);
            }
        }
    }

    permInherited.Reset();

    TraceLeaveVoid();
}


void
CPermissionSet::RemovePermission(PPERMISSION pPerm)
{
    BOOL bObjectAcePresent = FALSE;

    TraceEnter(TRACE_PERMSET, "CPermissionSet::RemovePermission");
    TraceAssert(pPerm != NULL);

    if (m_hPermList)
    {
        BOOL bNullGuid = IsEqualGUID(pPerm->guid, GUID_NULL);
        UINT cItems = DSA_GetItemCount(m_hPermList);
        while (cItems > 0)
        {
            PPERMISSION pPermCompare;
            BOOL bNullGuidCompare;

            --cItems;
            pPermCompare = (PPERMISSION)DSA_GetItemPtr(m_hPermList, cItems);

            bNullGuidCompare = IsEqualGUID(pPermCompare->guid, GUID_NULL);

            if (bNullGuid || bNullGuidCompare || IsEqualGUID(pPermCompare->guid, pPerm->guid))
            {
                pPermCompare->mask &= ~pPerm->mask;
                if (0 == pPermCompare->mask)
                    DSA_DeleteItem(m_hPermList, cItems);
                else if (!bNullGuidCompare)
                    bObjectAcePresent = TRUE;
            }
            else if (!bNullGuidCompare)
                bObjectAcePresent = TRUE;
        }
    }

    m_fObjectAcesPresent = bObjectAcePresent;

    TraceLeaveVoid();
}
