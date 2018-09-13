//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1995 - 1995.
//
//  File:       shares.h
//
//  Contents:   Definition of the shell IDLIST type for Shares
//
//  History:    13-Dec-95    BruceFo     Created
//
//----------------------------------------------------------------------------

#ifndef __SHARES_H__
#define __SHARES_H__

#define MAXSTRINGBUF (NNLEN + 1 + MAXCOMMENTSZ + 1 + MAX_PATH + 1 + NETWARE_VOLUMENAMELENGTH + 1)

struct IDSHARE
{
    USHORT  cb;                 // size of the ID in bytes, including 'cb'.
    BYTE    bFlags;             // one of SHID_SHARE_* below
    BYTE    bService;           // service type(s) of this share. Only valid if bFlags == SHID_SHARE

    // The following fields are legal based on the value of bService, above.
    // SMB data:
    DWORD   level;              // 1 or 2
    DWORD   type;               // shiX_type field
    DWORD   maxUses;            // only valid for SHID_SHARE_2
    USHORT  oPath;              // only valid for SHID_SHARE_2: cBuf[oPath] is start of path
    USHORT  oComment;           // cBuf[oComment] is start of comment

    // SFM data:
    DWORD   sfmMaxUses;

    // FPNW data:
    DWORD   fpnwType;
    DWORD   fpnwMaxUses;
    USHORT  oFpnwName;

    // Here begins the variable-sized string data. The first thing is always
    // the share name.
    WCHAR   cBuf[MAXSTRINGBUF];   // cBuf[0] is the start of the SMB name
};
typedef IDSHARE* LPIDSHARE;

#define SHID_SHARE          0x50    // a share (or multiple shares)

#define SHARE_SERVICE_SMB   0x1     // an SMB share
#define SHARE_SERVICE_SFM   0x2     // an SFM (MacFile) share
#define SHARE_SERVICE_FPNW  0x4     // an FPNW (NetWare) share

#define Share_GetFlags(pidl)        (pidl->bFlags)
#define Share_GetService(pidl)      (pidl->bService)
#define Share_GetName(pidl)         (pidl->cBuf)
#define Share_GetComment(pidl)      ((pidl->oComment == 0xffff) ? L"" : &(pidl->cBuf[pidl->oComment]))
#define Share_GetPath(pidl)         ((pidl->oPath == 0xffff)    ? L"" : &(pidl->cBuf[pidl->oPath]))
#define Share_GetType(pidl)         (pidl->type)
#define Share_GetMaxUses(pidl)      (pidl->maxUses)

#define Share_IsShare(pidl)             (pidl->bFlags == SHID_SHARE)
#define Share_HasSmb(pidl)              (Share_IsShare(pidl) && (pidl->bService & SHARE_SERVICE_SMB))
#define Share_HasSfm(pidl)              (Share_IsShare(pidl) && (pidl->bService & SHARE_SERVICE_SFM))
#define Share_HasFpnw(pidl)             (Share_IsShare(pidl) && (pidl->bService & SHARE_SERVICE_FPNW))

#define Share_GetLevel(pidl)            (appAssert(Share_HasSmb(pidl)), pidl->level)

#define Share_GetNameOffset(pidl)       offsetof(IDSHARE, cBuf)
#define Share_GetCommentOffset(pidl)    (offsetof(IDSHARE, cBuf) + pidl->oComment * sizeof(WCHAR))
#define Share_GetPathOffset(pidl)       (offsetof(IDSHARE, cBuf) + pidl->oPath * sizeof(WCHAR))

#endif // __SHARES_H__
