//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       misc.h
//
//  Definitions and prototypes for miscellaneous stuff
//
//--------------------------------------------------------------------------

#ifndef _MISC_H_
#define _MISC_H_

typedef struct _USER_INFO
{
    PSID pSid;
    LPCTSTR pszName;
    LPCTSTR pszLogonName;
    SID_NAME_USE SidType;
} USER_INFO, *PUSER_INFO;

typedef struct _USER_LIST
{
    ULONG cUsers;
    USER_INFO rgUsers[ANYSIZE_ARRAY];
} USER_LIST, *PUSER_LIST;

PSID GetAceSid(PACE_HEADER pAce);
PSID LocalAllocSid(PSID pOriginal);
void DestroyDPA(HDPA hList);

extern "C" {
#include <ntlsa.h>
}
LSA_HANDLE GetLSAConnection(LPCTSTR pszServer, DWORD dwAccessDesired);

BOOL LookupSid(PSID pSid,
               LPCTSTR pszServer,
               LPSECURITYINFO2 psi2,
               PUSER_LIST *ppUserList);
BOOL LookupSids(HDPA hSids,
                LPCTSTR pszServer,
                LPSECURITYINFO2 psi2,
                PUSER_LIST *ppUserList);
BOOL LookupSidsAsync(HDPA hSids,
                     LPCTSTR pszServer,
                     LPSECURITYINFO2 psi2,
                     HWND hWndNotify,
                     UINT uMsgNotify,
                     PHANDLE phThread = NULL);
BOOL BuildUserDisplayName(LPTSTR *ppszDisplayName,
                          LPCTSTR pszName,
                          LPCTSTR pszLogonName = NULL);

// Indexes into the SID image list
typedef enum
{
    SID_IMAGE_UNKNOWN = 0,
    SID_IMAGE_COMPUTER,
    SID_IMAGE_GROUP,
    SID_IMAGE_LOCALGROUP,
    SID_IMAGE_USER
} SID_IMAGE_INDEX;

HIMAGELIST LoadImageList(HINSTANCE hInstance, LPCTSTR pszBitmapID);
SID_IMAGE_INDEX GetSidImageIndex(PSID psid, SID_NAME_USE sidType);

BOOL IsStandalone(LPCTSTR pszMachine, PBOOL pbIsDC = NULL);

#if(_WIN32_WINNT < 0x0500)

HRESULT GetUserGroup(HWND       hwndOwner,
                     DWORD      dwFlags,
                     LPCTSTR    pszServer,
                     BOOL       bStandalone,
                     PUSER_LIST *ppUserList);   // Caller must LocalFree this

// Flags for GetUserGroup
#define GU_CONTAINER        0x00000001
#define GU_MULTI_SELECT     0x00000002
#define GU_AUDIT_HLP        0x00000004
#define GU_DC_SERVER        0x00000008

#endif  // _WIN32_WINNT < 0x0500

BOOL IsDACLCanonical(PACL pDacl);
BOOL IsDenyACL(PACL pDacl,
               BOOL fProtected,
               DWORD dwFullControlMask,
               LPDWORD pdwWarning);


//
// Possible SIDs that can be retrieved using QuerySystemSid.
//
enum UI_SystemSid
{
    // Well-known / universal
    UI_SID_World = 0,
    UI_SID_CreatorOwner,
    UI_SID_CreatorGroup,
    UI_SID_Dialup,
    UI_SID_Network,
    UI_SID_Batch,
    UI_SID_Interactive,
    UI_SID_Service,
    UI_SID_AnonymousLogon,
    UI_SID_Proxy,
    UI_SID_EnterpriseDC,
    UI_SID_Self,
    UI_SID_AuthenticatedUser,
    UI_SID_RestrictedCode,
    UI_SID_TerminalServer,
    UI_SID_LocalSystem,
    // Aliases ("BUILTIN")
    UI_SID_Admins,
    UI_SID_Users,
    UI_SID_Guests,
    UI_SID_PowerUsers,
    UI_SID_AccountOps,
    UI_SID_SystemOps,
    UI_SID_PrintOps,
    UI_SID_BackupOps,
    UI_SID_Replicator,
    UI_SID_RasServers,

    // Special value that gives the number of valid UI_SID_* types.
    // Don't add any new types after this value (add them before).
    UI_SID_Count,

    // This special value can be used for initializing enum UI_SystemSid
    // variables with a known unused quantity.  This value should never
    // be passed to QuerySystemSid.
    UI_SID_Invalid = -1
};
#define COUNT_SYSTEM_SID_TYPES          ((int)UI_SID_Count)
#define COUNT_WELL_KNOWN_SYSTEM_SIDS    ((int)UI_SID_Admins)

PSID QuerySystemSid(UI_SystemSid SystemSidType);

#define IsNTAuthority(pSid)             EqualPrefixSid(pSid, QuerySystemSid(UI_SID_LocalSystem))
#define IsAliasSid(pSid)                EqualPrefixSid(pSid, QuerySystemSid(UI_SID_Admins))
#define IsCreatorSid(pSid)              EqualPrefixSid(pSid, QuerySystemSid(UI_SID_CreatorOwner))
#define EqualSystemSid(pSid, uiSid)     EqualSid(pSid, QuerySystemSid(uiSid))

//
// Possible SIDs that can be retrieved using QueryTokenSid.
//
enum UI_TokenSid
{
    UI_TSID_CurrentProcessUser = 0, // Always the logged on user SID
    UI_TSID_CurrentProcessOwner,    // Generally logged on user SID, but sometimes not (e.g. Administrators)
    UI_TSID_CurrentProcessPrimaryGroup,
    UI_TSID_Count,
    UI_TSID_Invalid = -1
};
#define COUNT_TOKEN_SID_TYPES           ((int)UI_TSID_Count)

PSID QueryTokenSid(UI_TokenSid TokenSidType);

#define EqualTokenSid(pSid, uiSid)      EqualSid(pSid, QueryTokenSid(uiSid))

PSID GetAuthenticationID(LPCWSTR pszServer);

int CopyUnicodeString(LPTSTR pszDest, ULONG cchDest, PLSA_UNICODE_STRING pSrc);
int CopyUnicodeString(LPTSTR *ppszResult, PLSA_UNICODE_STRING pSrc);

BOOL IsSameGUID(const GUID *p1, const GUID *p2);
#define IsNullGUID(p)   (!(p) || IsSameGUID((p), &GUID_NULL))

#endif  // _MISC_H_
