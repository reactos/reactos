//
//  APITHK.H
//
#ifndef _APITHK_H_
#define _APITHK_H_

#define PrivateSM_REMOTESESSION 0x1000

#define PrivateWM_CHANGEUISTATE     0x0127
#define PrivateWM_UPDATEUISTATE     0x0128
#define PrivateWM_QUERYUISTATE      0x0129

#define PrivateUIS_SET              1
#define PrivateUIS_CLEAR            2
#define PrivateUIS_INITIALIZE       3

#define PrivateUISF_HIDEFOCUS       0x1
#define PrivateUISF_HIDEACCEL       0x2

#if (WINVER >= 0x0500)

#if SM_REMOTESESSION != PrivateSM_REMOTESESSION
#error Incorrect definition of PrivateSM_REMOTESESSION
#endif

#if WM_CHANGEUISTATE != PrivateWM_CHANGEUISTATE || \
    WM_UPDATEUISTATE != PrivateWM_UPDATEUISTATE || \
    WM_QUERYUISTATE  != PrivateWM_QUERYUISTATE
#error Inconsistent definition of PrivateWM_xxxUISTATE
#endif

#if UIS_SET        != PrivateUIS_SET        || \
    UIS_CLEAR      != PrivateUIS_CLEAR      || \
    UIS_INITIALIZE != PrivateUIS_INITIALIZE
#error Inconsistent definition of PrivateUIS_xxx
#endif

#if UISF_HIDEFOCUS != PrivateUISF_HIDEFOCUS || \
    UISF_HIDEACCEL != PrivateUISF_HIDEACCEL
#error Inconsistent definition of PrivateUISF_xxx
#endif

#else

#define SM_REMOTESESSION PrivateSM_REMOTESESSION

#define WM_CHANGEUISTATE        PrivateWM_CHANGEUISTATE
#define WM_UPDATEUISTATE        PrivateWM_UPDATEUISTATE
#define WM_QUERYUISTATE         PrivateWM_QUERYUISTATE      

#define UIS_SET                 PrivateUIS_SET              
#define UIS_CLEAR               PrivateUIS_CLEAR            
#define UIS_INITIALIZE          PrivateUIS_INITIALIZE       

#define UISF_HIDEFOCUS          PrivateUISF_HIDEFOCUS       
#define UISF_HIDEACCEL          PrivateUISF_HIDEACCEL       

#endif // WINVER >= 0x0500

STDAPI_(BOOL) MyGetLastWriteTime (LPCWSTR pszPath, FILETIME *pft);
STDAPI_(BOOL) NT5_ExpandEnvironmentStringsForUserW (HANDLE hToken, LPCWSTR lpSrc, LPWSTR lpDest, DWORD dwSize);

#endif // _APITHK_H_
