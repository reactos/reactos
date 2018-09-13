//*************************************************************
//
//  Header file for Profile.c
//
//  Microsoft Confidential
//  Copyright (c) Microsoft Corporation 1995
//  All rights reserved
//
//*************************************************************

//
// Internal flags
//

#define PROFILE_MANDATORY       0x00000001
#define PROFILE_USE_CACHE       0x00000002
#define PROFILE_NEW_LOCAL       0x00000004
#define PROFILE_NEW_CENTRAL     0x00000008
#define PROFILE_UPDATE_CENTRAL  0x00000010
#define PROFILE_DELETE_CACHE    0x00000020
// do not define bit 40 because NT4 has this defined as Run_SyncApps.
#define PROFILE_GUEST_USER      0x00000080
#define PROFILE_ADMIN_USER      0x00000100
#define DEFAULT_NET_READY       0x00000200
#define PROFILE_SLOW_LINK       0x00000400
#define PROFILE_TEMP_ASSIGNED   0x00000800
// do not define bit 1000, this was used briefly 2009, 2010 before
#define PROFILE_PARTLY_LOADED   0x00002000
#define PROFILE_BACKUP_EXISTS   0x00004000
#define PROFILE_THIS_IS_BAK     0x00008000

//
// User Preference values
//

#define USERINFO_LOCAL                   0
#define USERINFO_FLOATING                1
#define USERINFO_MANDATORY               2
// #define USERINFO_LOCAL_SLOW_LINK         3
#define USERINFO_UNDEFINED              99

#define PROFILEERRORACTION_TEMP          0
#define PROFILEERRORACTION_LOGOFF        1


typedef struct _PROFILE {
    DWORD       dwFlags;
    DWORD       dwInternalFlags;
    DWORD       dwUserPreference;
    HANDLE      hToken;
    LPTSTR      lpUserName;
    LPTSTR      lpProfilePath;
    LPTSTR      lpRoamingProfile;
    LPTSTR      lpDefaultProfile;
    LPTSTR      lpLocalProfile;
    LPTSTR      lpPolicyPath;
    LPTSTR      lpServerName;
    HKEY        hKeyCurrentUser;
    FILETIME    ftProfileLoad;
    FILETIME    ftProfileUnload;
    LPTSTR      lpExclusionList;
} USERPROFILE, *LPPROFILE;

typedef struct _SLOWLINKDLGINFO {
    DWORD       dwTimeout;
    BOOL        bDownloadDefault;
} SLOWLINKDLGINFO, FAR *LPSLOWLINKDLGINFO;


LONG MyRegLoadKey(HKEY hKey, LPTSTR lpSubKey, LPTSTR lpFile);
BOOL MyRegUnLoadKey(HKEY hKey, LPTSTR lpSubKey);
BOOL SetupNewHive(LPPROFILE lpProfile, LPTSTR lpSidString, PSID pSid);

#define DP_BACKUP       1
#define DP_BACKUPEXISTS 2
#define DP_DELBACKUP    4

BOOL DeleteProfileEx (LPCTSTR lpSidString, LPTSTR lpLocalProfile, DWORD dwDeleteFlags, HKEY hKeyLM);

BOOL CreateSecureDirectory (LPPROFILE lpProfile, LPTSTR lpDirectory, PSID pSid, BOOL fRestricted);
BOOL ComputeLocalProfileName (LPPROFILE lpProfile, LPCTSTR lpUserName,
                              LPTSTR lpProfileImage, DWORD  cchMaxProfileImage,
                              LPTSTR lpExpProfileImage, DWORD  cchMaxExpProfileImage,
                              PSID pSid, BOOL bWin9xUpg);

BOOL SetDefaultUserHiveSecurity(LPPROFILE lpProfile, PSID pSid, HKEY RootKey);
LONG LoadUserClasses( LPPROFILE lpProfile, LPTSTR SidString);
BOOL UnloadClasses(LPTSTR lpSidString);
BOOL MoveUserProfiles (LPCTSTR lpSrcDir, LPCTSTR lpDestDir);
LPTSTR GetProfileSidString(HANDLE hToken);
