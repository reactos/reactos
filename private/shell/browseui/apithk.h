//
//  APITHK.H
//


#ifndef _APITHK_H_
#define _APITHK_H_


#define PrivateSPI_GETSELECTIONFADE 0x1014
#define PrivateWS_EX_LAYERED        0x00080000
#define PrivateWM_GETOBJECT         0x003D
#define PrivateTPM_HORPOSANIMATION  0x0400L
#define PrivateTPM_HORNEGANIMATION  0x0800L
#define PrivateTPM_VERPOSANIMATION  0x1000L
#define PrivateTPM_VERNEGANIMATION  0x2000L
#define PrivateTPM_NOANIMATION      0x4000L
#define PrivateWM_CHANGEUISTATE     0x0127
#define PrivateWM_UPDATEUISTATE     0x0128
#define PrivateWM_QUERYUISTATE      0x0129
#define PrivateUIS_SET              1
#define PrivateUIS_CLEAR            2
#define PrivateUIS_INITIALIZE       3
#define PrivateUISF_HIDEFOCUS       0x1
#define PrivateUISF_HIDEACCEL       0x2
#define PrivateSPI_GETKEYBOARDCUES  0x100A
#define PrivateWS_EX_LAYERED        0x00080000
#define PrivateSPI_GETCLEARTYPE     116
#define PrivateLWA_COLORKEY        0x00000001
#define PrivateLWA_ALPHA           0x00000002


#define KEYBOARDCUES

#if (WINVER >= 0x0500)

// for files in nt5api and w5api dirs, use the definition in sdk include.
// And make sure our private define is in sync with winuser.h.

#if SPI_GETSELECTIONFADE != PrivateSPI_GETSELECTIONFADE
#error inconsistant SPI_GETSELECTIONFADE in winuser.h
#endif

#if WS_EX_LAYERED != PrivateWS_EX_LAYERED
#error inconsistant WS_EX_LAYERED in winuser.h
#endif

#if WM_GETOBJECT != PrivateWM_GETOBJECT
#error inconsistant WM_GETOBJECT in winuser.h
#endif

#if TPM_HORPOSANIMATION != PrivateTPM_HORPOSANIMATION
#error inconsistant TPM_HORPOSANIMATION in winuser.h
#endif

#if TPM_HORNEGANIMATION != PrivateTPM_HORNEGANIMATION
#error inconsistant TPM_HORNEGANIMATION in winuser.h
#endif

#if TPM_VERPOSANIMATION != PrivateTPM_VERPOSANIMATION
#error inconsistant TPM_VERPOSANIMATION in winuser.h
#endif

#if TPM_VERNEGANIMATION != PrivateTPM_VERNEGANIMATION
#error inconsistant WS_EX_LAYERED in winuser.h
#endif

#if TPM_NOANIMATION != PrivateTPM_NOANIMATION
#error inconsistant TPM_NOANIMATION in winuser.h
#endif

// We are checking this in at the same time that user is. This is to prevent
// sync problems.
#ifdef SPI_GETCLEARTYPE
    #if SPI_GETCLEARTYPE != PrivateSPI_GETCLEARTYPE
        #error inconsistant SPI_GETCLEARTYPE in winuser.h
    #endif
#else
    #define SPI_GETCLEARTYPE        PrivateSPI_GETCLEARTYPE
#endif


#else

#define WS_EX_LAYERED           PrivateWS_EX_LAYERED
#define SPI_GETSELECTIONFADE    PrivateSPI_GETSELECTIONFADE
#define WM_GETOBJECT            PrivateWM_GETOBJECT
#define TPM_HORPOSANIMATION     PrivateTPM_HORPOSANIMATION
#define TPM_HORNEGANIMATION     PrivateTPM_HORNEGANIMATION
#define TPM_VERPOSANIMATION     PrivateTPM_VERPOSANIMATION
#define TPM_VERNEGANIMATION     PrivateTPM_VERNEGANIMATION
#define TPM_NOANIMATION         PrivateTPM_NOANIMATION
#define SPI_GETCLEARTYPE        PrivateSPI_GETCLEARTYPE
#define LWA_COLORKEY            PrivateLWA_COLORKEY
#define LWA_ALPHA               PrivateLWA_ALPHA   

#ifdef KEYBOARDCUES
#define WM_CHANGEUISTATE        PrivateWM_CHANGEUISTATE 
#define WM_UPDATEUISTATE        PrivateWM_UPDATEUISTATE 
#define WM_QUERYUISTATE         PrivateWM_QUERYUISTATE  
#define UIS_SET                 PrivateUIS_SET          
#define UIS_CLEAR               PrivateUIS_CLEAR        
#define UIS_INITIALIZE          PrivateUIS_INITIALIZE   
#define UISF_HIDEFOCUS          PrivateUISF_HIDEFOCUS
#define UISF_HIDEACCEL          PrivateUISF_HIDEACCEL   
#define SPI_GETKEYBOARDCUES     PrivateSPI_GETKEYBOARDCUES
#endif //KEYBOARDCUES

#endif // WINVER >= 0x0500

STDAPI_(HCURSOR) LoadHandCursor(DWORD dwRes);

STDAPI_(LRESULT) ACCESSIBLE_LresultFromObject(
    IN REFIID riid,
    IN WPARAM wParam,
    OUT IUnknown* punk);

STDAPI ACCESSIBLE_CreateStdAccessibleObject(
    IN HWND hwnd,
    IN LONG idObject,
    IN REFIID riid,
    OUT void** ppvObj);

STDAPI_(void) NT5_NotifyWinEvent(
    IN DWORD event,
    IN HWND hwnd,
    IN LONG idObject,
    IN LONG idChild);

#ifdef NotifyWinEvent
#undef NotifyWinEvent
#endif

#ifdef LresultFromObject
#undef LresultFromObject
#endif

#ifdef CreateStdAccessibleObject
#undef CreateStdAccessibleObject
#endif

#define AllowSetForegroundWindow NT5_AllowSetForegroundWindow

STDAPI_(BOOL) NT5_AllowSetForegroundWindow( DWORD dwProcessId );
STDAPI_(BOOL) NT5_SetLayeredWindowAttributes(HWND hwnd, COLORREF crKey, BYTE bAlpha, DWORD dwFlags);

#define SetLayeredWindowAttributes NT5_SetLayeredWindowAttributes
#define NotifyWinEvent NT5_NotifyWinEvent
#define LresultFromObject ACCESSIBLE_LresultFromObject
#define CreateStdAccessibleObject ACCESSIBLE_CreateStdAccessibleObject

STDAPI_(void) AnimateSetMenuPos(HWND hwnd, RECT* prc, UINT uFlags, UINT uSide, BOOL fNoAnimate);
STDAPI_(void) MyLockSetForegroundWindow(BOOL fLock);

STDAPI_(BOOL) BlendLayeredWindow(HWND hwnd, HDC hdcDest, POINT* ppt, SIZE* psize, HDC hdc, POINT* pptSrc, BYTE bBlendConst);

STDAPI_(UINT) MyExtractIconsW(LPCWSTR wszFileName, int nIconIndex, int cxIcon, int cyIcon, HICON *phicon, UINT *piconid, UINT nIcons, UINT flags);

#endif // _APITHK_H_

