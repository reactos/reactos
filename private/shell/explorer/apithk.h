//
//  APITHK.H
//


#ifndef _APITHK_H_
#define _APITHK_H_

#define PrivateWM_CHANGEUISTATE     0x0127
#define PrivateWM_UPDATEUISTATE     0x0128
#define PrivateWM_QUERYUISTATE      0x0129
#define PrivateUIS_SET              1
#define PrivateUIS_CLEAR            2
#define PrivateUIS_INITIALIZE       3
#define PrivateUISF_HIDEFOCUS       0x1
#define PrivateUISF_HIDEACCEL       0x2

#define PrivateSM_REMOTESESSION     0x1000
#define PrivateSPI_GETSCREENSAVERRUNNING 114
#define PrivateSPI_GETMENUANIMATION 0x1002

#define PrivateASFW_ANY             ((DWORD)-1)

#if(_WIN32_WINNT >= 0x0500)
#else
#define WM_CHANGEUISTATE        PrivateWM_CHANGEUISTATE 
#define WM_UPDATEUISTATE        PrivateWM_UPDATEUISTATE 
#define WM_QUERYUISTATE         PrivateWM_QUERYUISTATE  
#define UIS_SET                 PrivateUIS_SET          
#define UIS_CLEAR               PrivateUIS_CLEAR        
#define UIS_INITIALIZE          PrivateUIS_INITIALIZE   
#define UISF_HIDEFOCUS          PrivateUISF_HIDEFOCUS
#define UISF_HIDEACCEL          PrivateUISF_HIDEACCEL   
#endif

#if (WINVER >= 0x0500)

#if SPI_GETSCREENSAVERRUNNING != PrivateSPI_GETSCREENSAVERRUNNING
#error inconsistent SPI_GETSCREENSAVERRUNNING in winuser.h
#endif
#if SPI_GETMENUANIMATION != PrivateSPI_GETMENUANIMATION
#error inconsistent SPI_GETMENUANIMATION in winuser.h
#endif

#else

#define SM_REMOTESESSION        PrivateSM_REMOTESESSION
#define SPI_GETSCREENSAVERRUNNING   PrivateSPI_GETSCREENSAVERRUNNING
#define SPI_GETMENUANIMATION    PrivateSPI_GETMENUANIMATION
#define ASFW_ANY                PrivateASFW_ANY
#endif

#ifdef __cplusplus
extern "C" {            /* Assume C declarations for C++ */
#endif  /* __cplusplus */


// These handle both Windows 95 and Windows 98's APM and 
// ACPI power management
void DoEjectPC();
BOOL IsEjectAllowed(BOOL fForceUpdateCached);

// This isint a direct API thunk. It allows us to detect whether
// the user is running on console (eg sitting at the local machine), or
// is running in a remote hydra session
#define IsRemoteSession() SHGetMachineInfo(GMI_TSCLIENT)

#ifdef WINNT
// This isint a direct API thunk either. It allows us to detect whether
// we are running on a machine where the "Terminal Server" service is enabled.
BOOL IsTerminalServicesEnabled();

#define SetTermsrvAppInstallMode NT5_SetTermsrvAppInstallMode
STDAPI_(BOOL) NT5_SetTermsrvAppInstallMode(BOOL bState);

STDAPI_(BOOL) _TryHydra(LPCTSTR pszCmd, UINT *pflags);

#endif

//
// Sets the correct date flags when running in a BiDi locale
void SetBiDiDateFlags(int *piDateFormat);

#ifdef WINNT
LRESULT Task_HandleAppCommand(WPARAM wParam, LPARAM lParam);
#endif

#ifdef __cplusplus
};       /* End of extern "C" { */
#endif // __cplusplus

#endif // _APITHK_H_
