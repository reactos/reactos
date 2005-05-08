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
typedef struct _PROFILEINFOA {
  DWORD dwSize;
  DWORD dwFlags;
  LPSTR lpUserName;
  LPSTR lpProfilePath;
  LPSTR lpDefaultPath;
  LPSTR lpServerName;
  LPSTR lpPolicyPath;
  HANDLE hProfile;
} PROFILEINFOA, *LPPROFILEINFOA;
typedef struct _PROFILEINFOW {
  DWORD dwSize;
  DWORD dwFlags;
  LPWSTR lpUserName;
  LPWSTR lpProfilePath;
  LPWSTR lpDefaultPath;
  LPWSTR lpServerName;
  LPWSTR lpPolicyPath;
  HANDLE hProfile;
} PROFILEINFOW, *LPPROFILEINFOW;
BOOL WINAPI LoadUserProfileA(HANDLE,LPPROFILEINFOA);
BOOL WINAPI LoadUserProfileW(HANDLE,LPPROFILEINFOW);
BOOL WINAPI UnloadUserProfile(HANDLE,HANDLE);
BOOL WINAPI GetProfilesDirectoryA(LPSTR,LPDWORD);
BOOL WINAPI GetProfilesDirectoryW(LPWSTR,LPDWORD);
BOOL WINAPI GetUserProfileDirectoryA(HANDLE,LPSTR,LPDWORD);
BOOL WINAPI GetUserProfileDirectoryW(HANDLE,LPWSTR,LPDWORD);
BOOL WINAPI CreateEnvironmentBlock(LPVOID*,HANDLE,BOOL);
BOOL WINAPI DestroyEnvironmentBlock(LPVOID);
#ifdef UNICODE
typedef PROFILEINFOW PROFILEINFO;
typedef LPPROFILEINFOW LPPROFILEINFO;
#define LoadUserProfile  LoadUserProfileW
#define GetProfilesDirectory  GetProfilesDirectoryW
#define GetUserProfileDirectory  GetUserProfileDirectoryW
#else
typedef PROFILEINFOA PROFILEINFO;
typedef LPPROFILEINFOA LPPROFILEINFO;
#define LoadUserProfile  LoadUserProfileA
#define GetProfilesDirectory  GetProfilesDirectoryA
#define GetUserProfileDirectory  GetUserProfileDirectoryA
#endif
#ifdef __cplusplus
}
#endif
#endif /* _USERENV_H */
