#ifndef _USERENV_H
#define _USERENV_H

#if __GNUC__ >=3
#pragma GCC system_header
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define PI_NOUI (1)
#define PI_APPLYPOLICY (2)

#if (WINVER >= 0x0500)
#define RP_FORCE (1)
#endif

#if defined(_USERENV_)
#define USERENVAPI
#else
#define USERENVAPI DECLSPEC_IMPORT
#endif


typedef struct _PROFILEINFOA
{
  DWORD dwSize;
  DWORD dwFlags;
  LPSTR lpUserName;
  LPSTR lpProfilePath;
  LPSTR lpDefaultPath;
  LPSTR lpServerName;
  LPSTR lpPolicyPath;
  HANDLE hProfile;
} PROFILEINFOA, *LPPROFILEINFOA;

typedef struct _PROFILEINFOW
{
  DWORD dwSize;
  DWORD dwFlags;
  LPWSTR lpUserName;
  LPWSTR lpProfilePath;
  LPWSTR lpDefaultPath;
  LPWSTR lpServerName;
  LPWSTR lpPolicyPath;
  HANDLE hProfile;
} PROFILEINFOW, *LPPROFILEINFOW;

BOOL USERENVAPI WINAPI LoadUserProfileA (HANDLE, LPPROFILEINFOA);
BOOL USERENVAPI WINAPI LoadUserProfileW (HANDLE, LPPROFILEINFOW);
BOOL USERENVAPI WINAPI UnloadUserProfile (HANDLE, HANDLE);

BOOL USERENVAPI WINAPI GetAllUsersProfileDirectoryA (LPSTR, LPDWORD);
BOOL USERENVAPI WINAPI GetAllUsersProfileDirectoryW (LPWSTR, LPDWORD);
BOOL USERENVAPI WINAPI GetDefaultUserProfileDirectoryA (LPSTR, LPDWORD);
BOOL USERENVAPI WINAPI GetDefaultUserProfileDirectoryW (LPWSTR, LPDWORD);
BOOL USERENVAPI WINAPI GetProfilesDirectoryA(LPSTR, LPDWORD);
BOOL USERENVAPI WINAPI GetProfilesDirectoryW(LPWSTR, LPDWORD);
BOOL USERENVAPI WINAPI GetUserProfileDirectoryA(HANDLE, LPSTR, LPDWORD);
BOOL USERENVAPI WINAPI GetUserProfileDirectoryW(HANDLE, LPWSTR, LPDWORD);

BOOL USERENVAPI WINAPI CreateEnvironmentBlock(LPVOID*, HANDLE, BOOL);
BOOL USERENVAPI WINAPI DestroyEnvironmentBlock(LPVOID);
#if (WINVER >= 0x0500)
BOOL USERENVAPI WINAPI ExpandEnvironmentStringsForUserA (HANDLE, LPCSTR, LPSTR, DWORD);
BOOL USERENVAPI WINAPI ExpandEnvironmentStringsForUserW (HANDLE, LPCWSTR, LPWSTR, DWORD);
#endif

HANDLE USERENVAPI WINAPI EnterCriticalPolicySection (BOOL);
BOOL USERENVAPI WINAPI LeaveCriticalPolicySection (HANDLE);
BOOL USERENVAPI WINAPI RefreshPolicy (BOOL);
#if (WINVER >= 0x0500)
BOOL USERENVAPI WINAPI RefreshPolicyEx (BOOL, DWORD);
#endif
BOOL USERENVAPI WINAPI RegisterGPNotification (HANDLE, BOOL);
BOOL USERENVAPI WINAPI UnregisterGPNotification (HANDLE);

#ifdef UNICODE
typedef PROFILEINFOW PROFILEINFO;
typedef LPPROFILEINFOW LPPROFILEINFO;
/* begin private */
#define CreateUserProfile  CreateUserProfileW
#define AddDesktopItem  AddDesktopItemW
#define DeleteDesktopItem  DeleteDesktopItemW
#define CreateGroup  CreateGroupW
#define DeleteGroup  DeleteGroupW
#define AddItem  AddItemW
#define DeleteItem  DeleteItemW
#define CopyProfileDirectory  CopyProfileDirectoryW
/* end private */
#define LoadUserProfile  LoadUserProfileW
#define GetAllUsersProfileDirectory  GetAllUsersProfileDirectoryW
#define GetDefaultUserProfileDirectory  GetDefaultUserProfileDirectoryW
#define GetProfilesDirectory  GetProfilesDirectoryW
#define GetUserProfileDirectory  GetUserProfileDirectoryW
#if (WINVER >= 0x0500)
#define ExpandEnvironmentStringsForUser ExpandEnvironmentStringsForUserW
#endif
#else
typedef PROFILEINFOA PROFILEINFO;
typedef LPPROFILEINFOA LPPROFILEINFO;
/* begin private */
#define CreateUserProfile  CreateUserProfileA
#define AddDesktopItem  AddDesktopItemA
#define DeleteDesktopItem  DeleteDesktopItemA
#define CreateGroup  CreateGroupA
#define DeleteGroup  DeleteGroupA
#define AddItem  AddItemA
#define DeleteItem  DeleteItemA
#define CopyProfileDirectory  CopyProfileDirectoryA
/* end private */
#define LoadUserProfile  LoadUserProfileA
#define GetAllUsersProfileDirectory  GetAllUsersProfileDirectoryA
#define GetDefaultUserProfileDirectory  GetDefaultUserProfileDirectoryA
#define GetProfilesDirectory  GetProfilesDirectoryA
#define GetUserProfileDirectory  GetUserProfileDirectoryA
#if (WINVER >= 0x0500)
#define ExpandEnvironmentStringsForUser ExpandEnvironmentStringsForUserA
#endif
#endif

#ifdef __cplusplus
}
#endif

#endif /* _USERENV_H */
