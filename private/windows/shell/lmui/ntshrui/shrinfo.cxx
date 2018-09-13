//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994.
//
//  File:       shrinfo.cxx
//
//  Contents:   Lanman SHARE_INFO_502 encapsulation
//
//  History:    21-Feb-95   BruceFo     Created
//
//----------------------------------------------------------------------------

#include "headers.hxx"
#pragma hdrstop

#include "shrinfo.hxx"
#include "util.hxx"

CShareInfo::CShareInfo(
    VOID
    )
    :
    m_bOwn(TRUE),
    m_flags(0),
    m_pInfo(NULL),
    m_dwCacheFlags (0),
    m_bCachingSupported (true)
{
    INIT_SIG(CShareInfo);
    Close();    // doubly-linked list
}

CShareInfo::CShareInfo(
    IN SHARE_INFO_502* pInfo
    )
    :
    m_bOwn(FALSE),
    m_flags(0),
    m_pInfo(pInfo),
    m_dwCacheFlags (0),
    m_bCachingSupported (true)
{
    INIT_SIG(CShareInfo);
    Close();    // doubly-linked list
}

HRESULT
CShareInfo::InitInstance(
    VOID
    )
{
    CHECK_SIG(CShareInfo);

    if (m_bOwn)
    {
        appAssert(m_pInfo == NULL);

        m_pInfo = new SHARE_INFO_502;
        if (NULL == m_pInfo)
        {
            return E_OUTOFMEMORY;
        }

        m_pInfo->shi502_netname       = NULL;
        m_pInfo->shi502_type          = STYPE_DISKTREE;
        m_pInfo->shi502_remark        = NULL;
        m_pInfo->shi502_permissions   = ACCESS_ALL;
        m_pInfo->shi502_max_uses      = SHI_USES_UNLIMITED;
        m_pInfo->shi502_path          = NULL;
        m_pInfo->shi502_passwd        = NULL;
        m_pInfo->shi502_reserved      = 0;
        m_pInfo->shi502_security_descriptor = NULL;
    }

    return S_OK;
}

CShareInfo::~CShareInfo()
{
    CHECK_SIG(CShareInfo);

    if (m_bOwn)
    {
        if (NULL != m_pInfo)    // must check; InitInstance might have failed
        {
            delete[] m_pInfo->shi502_netname;
            delete[] m_pInfo->shi502_remark;
            delete[] m_pInfo->shi502_path;
            delete[] m_pInfo->shi502_passwd;
            if (NULL != m_pInfo->shi502_security_descriptor)
            {
                ::LocalFree(m_pInfo->shi502_security_descriptor);
                m_pInfo->shi502_security_descriptor = NULL;
            }
            delete m_pInfo;
        }
    }
}

NET_API_STATUS
CShareInfo::Commit(
    IN PWSTR pszMachine
    )
{
    CHECK_SIG(CShareInfo);
    appAssert(NULL != m_pInfo);

    if (m_flags == 0)
    {
        // nothing changed
        appDebugOut((DEB_ITRACE, "CShareInfo::Commit: nothing (%ws)\n", m_pInfo->shi502_netname));
        return NERR_Success;
    }

// #if DBG == 1
//     Dump(L"Commit");
// #endif // DBG == 1

    NET_API_STATUS ret;

    // Note: we store a path, even for admin$. However, the NetShare* APIs
    // don't like seeing a path for admin$, so we temporarily strip it here
    // if necessary, before calling any APIs.

    LPWSTR pszPathTmp = m_pInfo->shi502_path;
    if (0 == _wcsicmp(g_szAdminShare, m_pInfo->shi502_netname))
    {
        m_pInfo->shi502_path = NULL;
    }

    if (SHARE_FLAG_ADDED == m_flags)
    {
        appDebugOut((DEB_TRACE, "CShareInfo::Commit: add (%ws)\n", m_pInfo->shi502_netname));
        ret = NetShareAdd(pszMachine, 502, (LPBYTE)m_pInfo, NULL);
        if ( NERR_Success == ret )
            ret = WriteCacheFlags ();
    }
    else if (SHARE_FLAG_REMOVE == m_flags)
    {
        appDebugOut((DEB_TRACE, "CShareInfo::Commit: remove (%ws)\n", m_pInfo->shi502_netname));
        ret = NetShareDel(pszMachine, m_pInfo->shi502_netname, 0);
    }
    else if (SHARE_FLAG_MODIFY == m_flags)
    {
        appDebugOut((DEB_TRACE, "CShareInfo::Commit: modify (%ws)\n", m_pInfo->shi502_netname));
        DWORD parm_err;
        ret = NetShareSetInfo(pszMachine, m_pInfo->shi502_netname, 502, (LPBYTE)m_pInfo, &parm_err);
        if ( NERR_Success == ret )
            ret = WriteCacheFlags ();
    }


    // Restore the original, in case of admin$
    m_pInfo->shi502_path = pszPathTmp;

    // Must refresh the cache of shares after all commits
    if (ret != NERR_Success)
    {
        appDebugOut((DEB_TRACE, "CShareInfo::Commit: err = %d\n", ret));
    }

    return ret;
}

SHARE_INFO_502*
CShareInfo::GetShareInfo(
    VOID
    )
{
    CHECK_SIG(CShareInfo);
    appAssert(NULL != m_pInfo);

    return m_pInfo;
}

PWSTR
CShareInfo::GetNetname(
    VOID
    )
{
    CHECK_SIG(CShareInfo);
    appAssert(NULL != m_pInfo);
    
    if ( m_pInfo )
        return m_pInfo->shi502_netname;
    else
        return 0;
}

DWORD
CShareInfo::GetType(
    VOID
    )
{
    CHECK_SIG(CShareInfo);
    appAssert(NULL != m_pInfo);

    if ( m_pInfo )
        return m_pInfo->shi502_type;
    else
        return 0;
}

PWSTR
CShareInfo::GetRemark(
    VOID
    )
{
    CHECK_SIG(CShareInfo);
    appAssert(NULL != m_pInfo);

    if ( m_pInfo )
        return m_pInfo->shi502_remark;
    else
        return 0;
}

DWORD
CShareInfo::GetMaxUses(
    VOID
    )
{
    CHECK_SIG(CShareInfo);
    appAssert(NULL != m_pInfo);

    if ( m_pInfo )
        return m_pInfo->shi502_max_uses;
    else
        return 0;
}

PWSTR
CShareInfo::GetPassword(
    VOID
    )
{
    CHECK_SIG(CShareInfo);
    appAssert(NULL != m_pInfo);

    if ( m_pInfo )
        return m_pInfo->shi502_passwd;
    else
        return 0;
}

PWSTR
CShareInfo::GetPath(
    VOID
    )
{
    CHECK_SIG(CShareInfo);
    appAssert(NULL != m_pInfo);

    if ( m_pInfo )
        return m_pInfo->shi502_path;
    else
        return 0;
}

PSECURITY_DESCRIPTOR
CShareInfo::GetSecurityDescriptor(
    VOID
    )
{
    CHECK_SIG(CShareInfo);
    appAssert(NULL != m_pInfo);

    if ( m_pInfo )
        return m_pInfo->shi502_security_descriptor;
    else
        return 0;
}

HRESULT
CShareInfo::SetNetname(
    IN PWSTR pszNetname
    )
{
    CHECK_SIG(CShareInfo);
    appAssert(m_flags != SHARE_FLAG_REMOVE);

    if (!TakeOwn())
    {
        return E_OUTOFMEMORY;
    }

    appDebugOut((DEB_ITRACE,
        "CShareInfo::SetNetname() = '%ws'\n",
        pszNetname));

    delete[] m_pInfo->shi502_netname;
    m_pInfo->shi502_netname = NewDup(pszNetname);

    if (m_flags != SHARE_FLAG_ADDED)
    {
        m_flags = SHARE_FLAG_MODIFY;
    }

    return S_OK;
}

HRESULT
CShareInfo::SetType(
    IN DWORD dwType
    )
{
    CHECK_SIG(CShareInfo);
    appAssert(m_flags != SHARE_FLAG_REMOVE);

    if (dwType != m_pInfo->shi502_type)
    {
        // only take ownership and set the data if it's changed!

        if (!TakeOwn())
        {
            return E_OUTOFMEMORY;
        }

        appDebugOut((DEB_ITRACE,
            "CShareInfo::SetType(%ws) = %d\n",
            m_pInfo->shi502_netname,
            dwType));

        m_pInfo->shi502_type = dwType;

        if (m_flags != SHARE_FLAG_ADDED)
        {
            m_flags = SHARE_FLAG_MODIFY;
        }
    }

    return S_OK;
}

HRESULT
CShareInfo::SetRemark(
    IN PWSTR pszRemark
    )
{
    CHECK_SIG(CShareInfo);
    appAssert(m_flags != SHARE_FLAG_REMOVE);

    if (!TakeOwn())
    {
        return E_OUTOFMEMORY;
    }

    appDebugOut((DEB_ITRACE,
        "CShareInfo::SetRemark(%ws) = '%ws'\n",
        m_pInfo->shi502_netname,
        pszRemark));

    delete[] m_pInfo->shi502_remark;
    m_pInfo->shi502_remark = NewDup(pszRemark);

    if (m_flags != SHARE_FLAG_ADDED)
    {
        m_flags = SHARE_FLAG_MODIFY;
    }

    return S_OK;
}

HRESULT
CShareInfo::SetMaxUses(
    IN DWORD dwMaxUses
    )
{
    CHECK_SIG(CShareInfo);
    appAssert(m_flags != SHARE_FLAG_REMOVE);

    if (dwMaxUses != m_pInfo->shi502_max_uses)
    {
        // only take ownership and set the data if it's changed!

        if (!TakeOwn())
        {
            return E_OUTOFMEMORY;
        }

        appDebugOut((DEB_ITRACE,
            "CShareInfo::SetMaxUses(%ws) = %d\n",
            m_pInfo->shi502_netname,
            dwMaxUses));

        m_pInfo->shi502_max_uses = dwMaxUses;

        if (m_flags != SHARE_FLAG_ADDED)
        {
            m_flags = SHARE_FLAG_MODIFY;
        }
    }

    return S_OK;
}

HRESULT
CShareInfo::SetPassword(
    IN PWSTR pszPassword
    )
{
    CHECK_SIG(CShareInfo);
    appAssert(m_flags != SHARE_FLAG_REMOVE);

    if (!TakeOwn())
    {
        return E_OUTOFMEMORY;
    }

    appDebugOut((DEB_ITRACE,
        "CShareInfo::SetPassword(%ws) = '%ws'\n",
        m_pInfo->shi502_netname,
        pszPassword));

    delete[] m_pInfo->shi502_passwd;
    m_pInfo->shi502_passwd = NewDup(pszPassword);

    if (m_flags != SHARE_FLAG_ADDED)
    {
        m_flags = SHARE_FLAG_MODIFY;
    }

    return S_OK;
}

HRESULT
CShareInfo::SetPath(
    IN PWSTR pszPath
    )
{
    CHECK_SIG(CShareInfo);
    appAssert(m_flags != SHARE_FLAG_REMOVE);

    if (!TakeOwn())
    {
        return E_OUTOFMEMORY;
    }

    appDebugOut((DEB_ITRACE,
        "CShareInfo::SetPath(%ws) = '%ws'\n",
        m_pInfo->shi502_netname,
        pszPath));

    delete[] m_pInfo->shi502_path;
    if (pszPath[0] == TEXT('\0'))
    {
        m_pInfo->shi502_path = NULL;    // so IPC$ and ADMIN$ work
    }
    else
    {
        m_pInfo->shi502_path = NewDup(pszPath);
    }

    if (m_flags != SHARE_FLAG_ADDED)
    {
        m_flags = SHARE_FLAG_MODIFY;
    }

    return S_OK;
}

HRESULT
CShareInfo::SetSecurityDescriptor(
    IN PSECURITY_DESCRIPTOR pSecDesc
    )
{
    CHECK_SIG(CShareInfo);
    appAssert(m_flags != SHARE_FLAG_REMOVE);

    if (!TakeOwn())
    {
        return E_OUTOFMEMORY;
    }

    appDebugOut((DEB_ITRACE,
        "CShareInfo::SetSecurityDescriptor(%ws) = ...\n",
        m_pInfo->shi502_netname));

    if (NULL != m_pInfo->shi502_security_descriptor)
        ::LocalFree(m_pInfo->shi502_security_descriptor);
    m_pInfo->shi502_security_descriptor = CopySecurityDescriptor(pSecDesc);

    if (m_flags != SHARE_FLAG_ADDED)
    {
        m_flags = SHARE_FLAG_MODIFY;
    }

    return S_OK;
}


// security descriptors alloced via ::LocalAlloc
HRESULT
CShareInfo::TransferSecurityDescriptor(
    IN PSECURITY_DESCRIPTOR pSecDesc
    )
{
    CHECK_SIG(CShareInfo);
    appAssert(m_flags != SHARE_FLAG_REMOVE);

    if (!TakeOwn())
    {
        return E_OUTOFMEMORY;
    }

    appDebugOut((DEB_ITRACE,
        "CShareInfo::TransferSecurityDescriptor(%ws) = ...\n",
        m_pInfo->shi502_netname));

    if (NULL != m_pInfo->shi502_security_descriptor)
        ::LocalFree(m_pInfo->shi502_security_descriptor);
    m_pInfo->shi502_security_descriptor = pSecDesc;

    if (m_flags != SHARE_FLAG_ADDED)
    {
        m_flags = SHARE_FLAG_MODIFY;
    }

    return S_OK;
}

ULONG
CShareInfo::GetFlag(
    VOID
    )
{
    CHECK_SIG(CShareInfo);

    return m_flags;
}

VOID
CShareInfo::SetDirtyFlag(
    ULONG flag
    )
{
    CHECK_SIG(CShareInfo);

    m_flags = flag;
}

HRESULT
CShareInfo::Copy(
    IN SHARE_INFO_502* pInfo
    )
{
    CHECK_SIG(CShareInfo);

    // get a valid SHARE_INFO_502 structure...

    if (m_bOwn)
    {
        // delete what's already there

        appAssert(NULL != m_pInfo);

        delete[] m_pInfo->shi502_netname;
        delete[] m_pInfo->shi502_remark;
        delete[] m_pInfo->shi502_path;
        delete[] m_pInfo->shi502_passwd;
        if (NULL != m_pInfo->shi502_security_descriptor)
        {
            ::LocalFree(m_pInfo->shi502_security_descriptor);
            m_pInfo->shi502_security_descriptor = NULL;
        }
    }
    else
    {
        m_pInfo = new SHARE_INFO_502;
        if (NULL == m_pInfo)
        {
            return E_OUTOFMEMORY;
        }
    }

    appAssert(NULL != m_pInfo);

    m_bOwn = TRUE;

    m_pInfo->shi502_netname       = NULL;
    m_pInfo->shi502_type          = pInfo->shi502_type;
    m_pInfo->shi502_remark        = NULL;
    m_pInfo->shi502_permissions   = pInfo->shi502_permissions;
    m_pInfo->shi502_max_uses      = pInfo->shi502_max_uses;
    m_pInfo->shi502_path          = NULL;
    m_pInfo->shi502_passwd        = NULL;
    m_pInfo->shi502_reserved      = pInfo->shi502_reserved;
    m_pInfo->shi502_security_descriptor = NULL;

    if (NULL != pInfo->shi502_netname)
    {
        m_pInfo->shi502_netname = NewDup(pInfo->shi502_netname);
    }
    if (NULL != pInfo->shi502_remark)
    {
        m_pInfo->shi502_remark = NewDup(pInfo->shi502_remark);
    }
    if (NULL != pInfo->shi502_path)
    {
        m_pInfo->shi502_path = NewDup(pInfo->shi502_path);
    }
    if (NULL != pInfo->shi502_passwd)
    {
        m_pInfo->shi502_passwd = NewDup(pInfo->shi502_passwd);
    }

    if (NULL != pInfo->shi502_security_descriptor)
    {
        m_pInfo->shi502_security_descriptor = CopySecurityDescriptor(pInfo->shi502_security_descriptor);
    }

    return S_OK;
}

BOOL
CShareInfo::TakeOwn(
    VOID
    )
{
    CHECK_SIG(CShareInfo);

    if (m_pInfo == NULL)
        return FALSE;

    if (m_bOwn)
    {
        return TRUE;    // already own the memory
    }

    SHARE_INFO_502* pInfo = new SHARE_INFO_502;
    if (NULL == pInfo)
    {
        return FALSE;
    }

    pInfo->shi502_type          = m_pInfo->shi502_type;
    pInfo->shi502_permissions   = m_pInfo->shi502_permissions;
    pInfo->shi502_max_uses      = m_pInfo->shi502_max_uses;
    pInfo->shi502_reserved      = 0;

    pInfo->shi502_netname = NULL;
    if (NULL != m_pInfo->shi502_netname)
    {
        pInfo->shi502_netname = NewDup(m_pInfo->shi502_netname);
    }

    pInfo->shi502_remark  = NULL;
    if (NULL != m_pInfo->shi502_remark)
    {
        pInfo->shi502_remark = NewDup(m_pInfo->shi502_remark);
    }

    pInfo->shi502_path    = NULL;
    if (NULL != m_pInfo->shi502_path)
    {
        pInfo->shi502_path = NewDup(m_pInfo->shi502_path);
    }

    pInfo->shi502_passwd  = NULL;
    if (NULL != m_pInfo->shi502_passwd)
    {
        pInfo->shi502_passwd = NewDup(m_pInfo->shi502_passwd);
    }

    pInfo->shi502_security_descriptor = NULL;
    if (NULL != m_pInfo->shi502_security_descriptor)
    {
        pInfo->shi502_security_descriptor = CopySecurityDescriptor(m_pInfo->shi502_security_descriptor);
    }

    m_pInfo = pInfo;
    m_bOwn = TRUE;

#if DBG == 1
    Dump(L"After TakeOwn");
#endif // DBG == 1

    return TRUE;
}


VOID
DeleteShareInfoList(
    IN CShareInfo* pShareInfoList,
    IN BOOL fDeleteDummyHeadNode
    )
{
    if (NULL == pShareInfoList)
    {
        // allow "deletion" of NULL list
        return;
    }

    for (CShareInfo* p = (CShareInfo*) pShareInfoList->Next();
         p != pShareInfoList;
         )
    {
        CShareInfo* pNext = (CShareInfo*)p->Next();
        delete p;
        p = pNext;
    }

    if (fDeleteDummyHeadNode)
    {
        delete pShareInfoList;
    }
    else
    {
        pShareInfoList->Close();    // reset pointers
    }
}


#if DBG == 1

VOID
CShareInfo::Dump(
    IN PWSTR pszCaption
    )
{
    CHECK_SIG(CShareInfo);

    appDebugOut((DEB_TRACE,
        "CShareInfo::Dump, %ws\n",
        pszCaption));

    appDebugOut((DEB_TRACE | DEB_NOCOMPNAME,
"\t       This: 0x%08lx\n"
"\t       Info: 0x%08lx\n"
"\tOwn memory?: %ws\n"
"\t      Flags: %ws\n"
"\t Share name: %ws\n"
"\t       Type: %d\n"
"\t    Comment: %ws\n"
"\tPermissions: %d\n"
"\t   Max uses: %d\n"
"\t       Path: %ws\n"
"\t   Password: %ws\n"
"\t   Reserved: %d\n"
"\t   Security? %ws\n"
,
this,
m_pInfo,
m_bOwn ? L"yes" : L"no",
(m_flags == 0)
    ? L"none"
    : (m_flags == SHARE_FLAG_ADDED)
        ? L"added"
        : (m_flags == SHARE_FLAG_REMOVE)
            ? L"remove"
            : (m_flags == SHARE_FLAG_MODIFY)
                ? L"modify"
                : L"UNKNOWN!",
(NULL == m_pInfo->shi502_netname) ? L"none" : m_pInfo->shi502_netname,
m_pInfo->shi502_type,
(NULL == m_pInfo->shi502_remark) ? L"none" : m_pInfo->shi502_remark,
m_pInfo->shi502_permissions,
m_pInfo->shi502_max_uses,
(NULL == m_pInfo->shi502_path) ? L"none" : m_pInfo->shi502_path,
(NULL == m_pInfo->shi502_passwd) ? L"none" : m_pInfo->shi502_passwd,
m_pInfo->shi502_reserved,
(NULL == m_pInfo->shi502_security_descriptor) ? L"No" : L"Yes"
));

}

#endif // DBG == 1

DWORD 
CShareInfo::GetCacheFlags(
    VOID
    ) const
{
    CHECK_SIG(CShareInfo);

    return m_dwCacheFlags;
}

void 
CShareInfo::SetCacheFlags(
    DWORD dwFlags
    )
{
    CHECK_SIG(CShareInfo);

    m_dwCacheFlags = dwFlags;
    if ( SHARE_FLAG_ADDED != m_flags )
        m_flags = SHARE_FLAG_MODIFY;
}

//
// These methods cover the seperate API to determine whether IntelliMirror
// caching is enabled.  By default (FPNW and SFM) they are disabled.
//
// We read this data at level 501 in order to determine whether the target
// server is NT4.  NetShareGetInfo[1005] actually succeeds on an NT4 server,
// whereas NetShareGetInfo[501] fails with ERROR_INVALID_LEVEL.  We want this
// to fail so that we can disable the checkbox where the underlying
// functionality is not supported.
//
// CODEWORK: If this becomes remotable, we'll need to do a better job of 
//            handling the error case when this method fails.
//
NET_API_STATUS 
CShareInfo::ReadCacheFlags(
    VOID
    )
{
    CHECK_SIG(CShareInfo);
    NET_API_STATUS retval = NERR_Success;

    m_dwCacheFlags = 0;
		
    if ( m_bCachingSupported ) // If we've already determined that caching is 
           // not supported, don't call this again
    {
        SHARE_INFO_501* pshi501 = NULL;
        PWSTR           pwszNetName = GetNetname ();

        if ( pwszNetName && *pwszNetName )
        {
            retval = ::NetShareGetInfo(
                    L"",            // machine name
                    pwszNetName,    // share name
                    501,
                    (LPBYTE*)&pshi501);
            if (NERR_Success == retval)
            {
                if ( pshi501 )
                {
                    m_dwCacheFlags = pshi501->shi501_flags;
                    ::NetApiBufferFree (pshi501);
                }
            }
            else if ( ERROR_INVALID_LEVEL == retval )
            {
                // This share is probably on an NT 4.0 machine and does not 
                // support caching
                m_bCachingSupported = false;
                retval = NERR_Success;
            }
            else if ( NERR_Success != retval )
                m_bCachingSupported = false;
        }
    }
    return retval;
}

NET_API_STATUS 
CShareInfo::WriteCacheFlags(
    VOID
    )
{
    CHECK_SIG(CShareInfo);
    NET_API_STATUS retval = NERR_Success;

    if ( m_bCachingSupported ) // If we've already determined that caching is 
						       // not supported, don't call this again
    {
        SHARE_INFO_1005 shi1005;
        ZeroMemory( &shi1005, sizeof(shi1005) );
        shi1005.shi1005_flags = m_dwCacheFlags;
        DWORD dwDummy = 0;
        retval = ::NetShareSetInfo(
                L"",            // machine name
                GetNetname (),    // share name
                1005,
                (LPBYTE)&shi1005,
                &dwDummy);
        if ( ERROR_INVALID_LEVEL == retval )
        {
            // This share is probably on an NT 4.0 machine and does not support 
            // caching
            m_bCachingSupported = false;
            retval = NERR_Success;
        }
        else if ( NERR_Success != retval )
            m_bCachingSupported = false;
    }

	return retval;
}
