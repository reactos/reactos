//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994.
//
//  File:       shri.cxx
//
//  Contents:   Class object encapsulating a generic "share", that may be
//              realized via one or more file servers.
//
//  History:    8-Mar-96   BruceFo     Created
//
//----------------------------------------------------------------------------

#include "headers.hxx"
#pragma hdrstop

#include "shri.hxx"
#include "util.hxx"

CShare::CShare(
    VOID
    )
    :
    m_dwService(0),
    m_pSmbInfo(NULL),
    m_pSfmInfo(NULL),
    m_pFpnwInfo(NULL)
{
    INIT_SIG(CShare);
	Close();
}

CShare::~CShare()
{
    CHECK_SIG(CShare);
}

VOID
CShare::AddSmb(
    IN SHARE_INFO_2* pInfo  // may point to level 1 info???
    )
{
    CHECK_SIG(CShare);
    appAssert(NULL != pInfo);
    appAssert(NULL == m_pSmbInfo && "We already have an SMB share!");
    m_dwService |= SHARE_SERVICE_SMB;
    m_pSmbInfo = pInfo;
}

VOID
CShare::AddSfm(
    IN AFP_VOLUME_INFO* pInfo
    )
{
    CHECK_SIG(CShare);
    appAssert(NULL != pInfo);
    appAssert(NULL == m_pSfmInfo && "We already have an SFM share!");
    m_dwService |= SHARE_SERVICE_SFM;
    m_pSfmInfo = pInfo;
}

VOID
CShare::AddFpnw(
    IN FPNWVOLUMEINFO* pInfo
    )
{
    CHECK_SIG(CShare);
    appAssert(NULL != pInfo);
    appAssert(NULL == m_pFpnwInfo && "We already have an FPNW share!");
    m_dwService |= SHARE_SERVICE_FPNW;
    m_pFpnwInfo = pInfo;
}


PWSTR
CShare::GetName(
    VOID
    )
{
    CHECK_SIG(CShare);

    if (m_dwService & SHARE_SERVICE_SMB)
    {
        appAssert(NULL != m_pSmbInfo);
        return m_pSmbInfo->shi2_netname;
    }
    if (m_dwService & SHARE_SERVICE_SFM)
    {
        appAssert(NULL != m_pSfmInfo);
        return m_pSfmInfo->afpvol_name;
    }
    if (m_dwService & SHARE_SERVICE_FPNW)
    {
        appAssert(NULL != m_pFpnwInfo);
        return m_pFpnwInfo->lpVolumeName;
    }

    appAssert(!"Trying to get the name of a share but there are no services!");
    return NULL;
}


PWSTR
CShare::GetPath(
    VOID
    )
{
    CHECK_SIG(CShare);

    if (m_dwService & SHARE_SERVICE_SMB)
    {
        // BUGBUG: MUST BE LEVEL 2 DATA!

        appAssert(NULL != m_pSmbInfo);
        return m_pSmbInfo->shi2_path;
    }
    if (m_dwService & SHARE_SERVICE_SFM)
    {
        appAssert(NULL != m_pSfmInfo);
        return m_pSfmInfo->afpvol_path;
    }
    if (m_dwService & SHARE_SERVICE_FPNW)
    {
        appAssert(NULL != m_pFpnwInfo);
        return m_pFpnwInfo->lpPath;
    }

    appAssert(!"Trying to get the path of a share but there are no services!");
    return NULL;
}

VOID
CShare::FillID(
    OUT LPIDSHARE pids
    )
{
    appAssert(0 != m_dwService && "Trying to fill an id for a share that has no services!");

    PWSTR pszName     = NULL;
    PWSTR pszComment  = NULL;
    PWSTR pszPath     = NULL;
    PWSTR pszFpnwName = NULL;

    USHORT  nameLength, commentLength, pathLength, fpnwNameLength;
    USHORT  nameOffset, commentOffset, pathOffset, fpnwNameOffset;

    // initialize everything to defaults

    pids->bFlags      = SHID_SHARE;
    pids->bService    = (BYTE)m_dwService;  // only using 3 bits of 8

    pids->level       = 0;  // BUGBUG
    pids->type        = 0;
    pids->maxUses     = 0;
    pids->oPath       = 0xffff; // bogus
    pids->oComment    = 0xffff; // bogus
    pids->sfmMaxUses  = 0;
    pids->fpnwType    = 0;
    pids->fpnwMaxUses = 0;
    pids->oFpnwName   = 0xffff; // bogus

    if (m_dwService & SHARE_SERVICE_SMB)
    {
        // BUGBUG: MUST BE LEVEL 2 DATA!

        appAssert(NULL != m_pSmbInfo);
        pszName    = m_pSmbInfo->shi2_netname;
        pszComment = m_pSmbInfo->shi2_remark;
        pszPath    = m_pSmbInfo->shi2_path;

        pids->level       = 2;  // BUGBUG
        pids->type        = m_pSmbInfo->shi2_type;
        pids->maxUses     = m_pSmbInfo->shi2_max_uses;
    }
    if (m_dwService & SHARE_SERVICE_SFM)
    {
        appAssert(NULL != m_pSfmInfo);
        if (NULL == pszName)
        {
            pszName = m_pSfmInfo->afpvol_name;
        }
        pids->sfmMaxUses  = m_pSfmInfo->afpvol_max_uses;
    }
    if (m_dwService & SHARE_SERVICE_FPNW)
    {
        appAssert(NULL != m_pFpnwInfo);
        if (NULL == pszName)
        {
            pszName = m_pFpnwInfo->lpVolumeName;
        }
        pszFpnwName = m_pFpnwInfo->lpVolumeName;
        pids->fpnwType    = m_pFpnwInfo->dwType;
        pids->fpnwMaxUses = m_pFpnwInfo->dwMaxUses;
    }

    appAssert(NULL != pszName && "The share has no name!!");

    nameLength      = lstrlen(pszName);
    commentLength   = (NULL == pszComment)  ? 0 : lstrlen(pszComment);
    pathLength      = (NULL == pszPath)     ? 0 : lstrlen(pszPath);
    fpnwNameLength  = (NULL == pszFpnwName) ? 0 : lstrlen(pszFpnwName);

    nameOffset      = 0;
    commentOffset   = nameOffset + nameLength + 1;
    pathOffset      = commentOffset + commentLength + 1;
    fpnwNameOffset  = pathOffset + pathLength + 1;

    // we don't store nameOffset
    pids->oComment  = commentOffset;
    pids->oPath     = pathOffset;
    pids->oFpnwName = fpnwNameOffset;

    lstrcpy(&pids->cBuf[nameOffset], pszName);

    if (NULL != pszComment)
    {
        lstrcpy(&pids->cBuf[commentOffset], pszComment);
    }
    else
    {
        pids->cBuf[commentOffset] = L'\0';
    }

    if (NULL != pszPath)
    {
        lstrcpy(&pids->cBuf[pathOffset], pszPath);
    }
    else
    {
        pids->cBuf[pathOffset] = L'\0';
    }

    if (NULL != pszFpnwName)
    {
        lstrcpy(&pids->cBuf[fpnwNameOffset], pszFpnwName);
    }
    else
    {
        pids->cBuf[fpnwNameOffset] = L'\0';
    }

    pids->cb = offsetof(IDSHARE, cBuf)
               + (nameLength + 1
                  + commentLength + 1
                  + pathLength + 1
                  + fpnwNameLength + 1
                  ) * sizeof(WCHAR);

    //
    // null terminate pidl
    //

    *(USHORT *)((LPBYTE)pids + pids->cb) = 0;
}


#if DBG == 1

VOID
CShare::Dump(
    IN PWSTR pszCaption
    )
{
    CHECK_SIG(CShare);

    appDebugOut((DEB_TRACE,
        "CShare::Dump, %ws\n",
        pszCaption));

    WCHAR szServices[500];
    szServices[0] = L'\0';
    if (m_dwService & SHARE_SERVICE_SMB)
    {
        wcscat(szServices, L"SMB ");
    }
    if (m_dwService & SHARE_SERVICE_SFM)
    {
        wcscat(szServices, L"SFM ");
    }
    if (m_dwService & SHARE_SERVICE_FPNW)
    {
        wcscat(szServices, L"FPNW ");
    }

    appDebugOut((DEB_TRACE | DEB_NOCOMPNAME,
"\t       This: 0x%08lx\n"
"\t   Services: %ws\n"
"\t Share name: %ws\n"
,
this,
szServices,
GetName() ? L"none" : GetName()
));

}

#endif // DBG == 1
