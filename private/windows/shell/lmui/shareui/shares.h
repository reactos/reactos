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

struct IDSHARE
{
    USHORT  cb;
    BYTE    bFlags;
    BYTE    bReserved;          // for alignment
    DWORD   type;               // shiX_type field
    DWORD   maxUses;            // only valid for SHID_SHARE_2
    USHORT  oPath;              // only valid for SHID_SHARE_2: cBuf[oPath] is start of path
    USHORT  oComment;           // cBuf[oComment] is start of comment
    TCHAR   cBuf[MAX_PATH*3];   // cBuf[0] is the start of name
};
typedef IDSHARE* LPIDSHARE;

#define SHID_SHARE_1    0x50    // Net share info level 1
#define SHID_SHARE_2    0x51    // Net share info level 2
#ifdef WIZARDS
#define SHID_SHARE_ALL  0x5c    // "all" shares wizard
#define SHID_SHARE_NW   0x5d    // NetWare shares wizard
#define SHID_SHARE_MAC  0x5e    // Mac shares wizard
#define SHID_SHARE_NEW  0x5f    // New Share wizard
#endif // WIZARDS

#define Share_GetFlags(pidl)        (pidl->bFlags)
#define Share_GetName(pidl)         (pidl->cBuf)
#define Share_GetComment(pidl)      (&(pidl->cBuf[pidl->oComment]))
#define Share_GetPath(pidl)         (&(pidl->cBuf[pidl->oPath]))
#define Share_GetType(pidl)         (pidl->type)
#define Share_GetMaxUses(pidl)      (pidl->maxUses)

#ifdef WIZARDS
#define Share_IsAllWizard(pidl)         (pidl->bFlags == SHID_SHARE_ALL)
#define Share_IsNetWareWizard(pidl)     (pidl->bFlags == SHID_SHARE_NW)
#define Share_IsMacWizard(pidl)         (pidl->bFlags == SHID_SHARE_MAC)
#define Share_IsNewShareWizard(pidl)    (pidl->bFlags == SHID_SHARE_NEW)
#define Share_IsSpecial(pidl)           (Share_IsNetWareWizard(pidl) || Share_IsMacWizard(pidl) || Share_IsNewShareWizard(pidl))
#endif // WIZARDS

#define Share_IsShare(pidl)         (pidl->bFlags == SHID_SHARE_1 || pidl->bFlags == SHID_SHARE_2)
#define Share_GetLevel(pidl)        (appAssert(Share_IsShare(pidl)), pidl->bFlags - SHID_SHARE_1 + 1)

#define Share_GetNameOffset(pidl)    offsetof(IDSHARE, cBuf)
#define Share_GetCommentOffset(pidl) (offsetof(IDSHARE, cBuf) + pidl->oComment * sizeof(TCHAR))
#define Share_GetPathOffset(pidl)    (offsetof(IDSHARE, cBuf) + pidl->oPath * sizeof(TCHAR))

#endif // __SHARES_H__
