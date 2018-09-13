//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       permset.h
//
//  This file contains the definition of the CPermissionSet class
//
//--------------------------------------------------------------------------

#ifndef _PERMSET_H_
#define _PERMSET_H_

typedef struct _PERMISSION
{
    ACCESS_MASK mask;       // permission bits
    DWORD       dwFlags;    // AceFlags (e.g. inheritance bits)
    GUID        guid;       // often GUID_NULL
} PERMISSION, *PPERMISSION;

class CPermissionSet
{
private:
    HDSA m_hPermList;       // Dynamic array of PERMISSION structures
    HDPA m_hAdvPermList;    // Dynamic array of ACE pointers
    BOOL m_fObjectAcesPresent;

public:
    CPermissionSet() : m_hPermList(NULL), m_hAdvPermList(NULL), m_fObjectAcesPresent(FALSE) {}
    ~CPermissionSet() { Reset(); }

    void Reset();
    BOOL AddAce(const GUID *pguid, ACCESS_MASK mask, DWORD dwFlags);
    BOOL AddPermission(PPERMISSION pPerm);
    BOOL AddAdvancedAce(PACE_HEADER pAce);
    UINT GetPermCount(BOOL fIncludeAdvAces = FALSE) const;
    PPERMISSION GetPermission(UINT i) const { if (m_hPermList) return (PPERMISSION)DSA_GetItemPtr(m_hPermList, i); return NULL; }
    PPERMISSION operator[](UINT i) const { return GetPermission(i); }
    ULONG GetAclLength(ULONG cbSid) const;
    BOOL AppendToAcl(PACL pAcl, PACE_HEADER *ppAcePos, PSID pSid, BOOL fAllowAce, DWORD dwFlags) const;
    void ConvertInheritedAces(CPermissionSet &permInherited);
    void RemovePermission(PPERMISSION pPerm);
};

// Flags for AppendToAcl
#define PS_NONOBJECT        0x00000001L
#define PS_OBJECT           0x00000002L


#endif  // _PERMSET_H_
