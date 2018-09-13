//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994.
//
//  File:       shrinfo.hxx
//
//  Contents:   Lanman SHARE_INFO encapsulation
//
//  History:    21-Feb-95   BruceFo     Created
//
//----------------------------------------------------------------------------

#ifndef __SHRINFO_HXX__
#define __SHRINFO_HXX__

//
// Valid flags:
//

#define SHARE_FLAG_ADDED        1
#define SHARE_FLAG_REMOVE       2
#define SHARE_FLAG_MODIFY       3

class CShareInfo
{
    DECLARE_SIG;

public:

    CShareInfo();   // create new info

    CShareInfo(IN SHARE_INFO_502* pInfo); // cache existing info

    HRESULT
    InitInstance(
        VOID
        );

    ~CShareInfo();

    NET_API_STATUS
    Commit(
        IN PWSTR pszMachine
        );

    //
    // "Get" methods
    //

    SHARE_INFO_502*
    GetShareInfo(
        VOID
        );

    PWSTR
    GetNetname(
        VOID
        );

    DWORD
    GetType(
        VOID
        );

    PWSTR
    GetRemark(
        VOID
        );

    DWORD
    GetMaxUses(
        VOID
        );

    PWSTR
    GetPassword(
        VOID
        );

    PWSTR
    GetPath(
        VOID
        );

    PSECURITY_DESCRIPTOR
    GetSecurityDescriptor(
        VOID
        );

    //
    // "Set" methods
    //

    HRESULT
    SetNetname(
        IN PWSTR pszNetname
        );

    HRESULT
    SetType(
        IN DWORD dwType
        );

    HRESULT
    SetRemark(
        IN PWSTR pszRemark
        );

    HRESULT
    SetMaxUses(
        IN DWORD dwMaxUses
        );

    HRESULT
    SetPassword(
        IN PWSTR pszPassword
        );

    HRESULT
    SetPath(
        IN PWSTR pszPath
        );

    // Set... makes a copy of the argument
    HRESULT
    SetSecurityDescriptor(
        IN PSECURITY_DESCRIPTOR pSecDesc
        );

    // Transfer... takes ownership of "new BYTE[]" memory
    HRESULT
    TransferSecurityDescriptor(
        IN PSECURITY_DESCRIPTOR pSecDesc
        );

    //
    // Other Get/Set methods -- not SHARE_INFO_502 data
    //

    ULONG
    GetFlag(
        VOID
        );

    VOID
    SetFlag(
        ULONG flag
        );

    HRESULT
    Copy(
        IN SHARE_INFO_502* pInfo
        );

#if DBG == 1
    VOID
    Dump(
        IN PWSTR pszCaption
        );
#endif // DBG == 1

    BOOL
    TakeOwn(
        VOID
        );

private:

    //
    // Main object data
    //

    BOOL            m_bOwn;
    ULONG           m_flags;
    SHARE_INFO_502* m_pInfo;
};


//
// Helper API
//

VOID
DeleteShareInfoList(
    IN CShareInfo* pShareInfoList,
    IN BOOL fDeleteDummyHeadNode = FALSE
    );

#endif // __SHRINFO_HXX__
