/****************************** Module Header ******************************\
* Module Name: immstruc.h
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* This header file contains the internal IMM structure definitions
*
* used by both Client and Kernel
*
* History:
* 28-Dec-1995 WKwok        Created.
\***************************************************************************/

#ifndef _IMMSTRUC_
#define _IMMSTRUC_

#include <imm.h>
#include <immp.h>
#include <ime.h>
#include <imep.h>
//#include "winnls32.h"
//#include "winnls3p.h"


#define NULL_HIMC        (HIMC)  0
#define INVALID_HIMC     (HIMC) -1
#define NULL_HIMCC       (HIMCC) 0
#define INVALID_HIMCC    (HIMCC)-1

/*
 * dwFlags for tagIMC.
 */
#define IMCF_UNICODE            0x0001
#define IMCF_ACTIVE             0x0002
#define IMCF_CHGMSG             0x0004
#define IMCF_SAVECTRL           0x0008
#define IMCF_PROCESSEVENT       0x0010
#define IMCF_FIRSTSELECT        0x0020
#define IMCF_INDESTROY          0x0040
#define IMCF_WINNLSDISABLE      0x0080
#define IMCF_DEFAULTIMC         0x0100

/*
 * dwFlag for ImmGetSaveContext().
 */
#define IGSC_DEFIMCFALLBACK     0x0001
#define IGSC_WINNLSCHECK        0x0002

/*
 * dwFlag for ImmFreeLayout().
 */
#define IFL_DEACTIVATEIME       0x0001
#define IFL_UNLOADIME           0x0002

#define IS_IME_KBDLAYOUT(hkl) ((HIWORD((ULONG_PTR)(hkl)) & 0xf000) == 0xe000)

/*
 * Load flag for loading IME.DLL
 */
#define IMEF_NONLOAD            0x0000
#define IMEF_LOADERROR          0x0001
#define IMEF_LOADED             0x0002

#define IM_DESC_SIZE            50
#define IM_FILE_SIZE            80
#define IM_OPTIONS_SIZE         30
#define IM_UI_CLASS_SIZE        16
#define IM_USRFONT_SIZE         80


/*
 * hotkey related defines that are common both client and kernel side
 */
#define MOD_MODIFY_KEYS         (MOD_ALT|MOD_CONTROL|MOD_SHIFT|MOD_WIN)
#define MOD_BOTH_SIDES          (MOD_LEFT|MOD_RIGHT)
#define ISHK_REMOVE             1
#define ISHK_ADD                2
#define ISHK_INITIALIZE         3

typedef struct _tagIMEHOTKEY {
    DWORD       dwHotKeyID;             // hot key ID
    UINT        uVKey;                  // hot key vkey
    UINT        uModifiers;             // combination keys with the vkey
    HKL         hKL;                    // target keyboard layout (IME)
} IMEHOTKEY;
typedef IMEHOTKEY      *PIMEHOTKEY;
typedef IMEHOTKEY      CONST *PCIMEHOTKEY;


typedef struct _tagIMEHOTKEYOBJ {
    struct _tagIMEHOTKEYOBJ *pNext;
    IMEHOTKEY        hk;
} IMEHOTKEYOBJ, *PIMEHOTKEYOBJ;


/*
 * Extended IME information.
 */
typedef struct tagIMEINFOEX {
    HKL                 hkl;
    IMEINFO             ImeInfo;
    WCHAR               wszUIClass[IM_UI_CLASS_SIZE];
    DWORD               fdwInitConvMode;    // Check this later
    BOOL                fInitOpen;          // Check this later
    BOOL                fLoadFlag;
    DWORD               dwProdVersion;
    DWORD               dwImeWinVersion;
    WCHAR               wszImeDescription[IM_DESC_SIZE];
    WCHAR               wszImeFile[IM_FILE_SIZE];
} IMEINFOEX, *PIMEINFOEX;

#ifndef W32KAPI
#define W32KAPI  DECLSPEC_ADDRSAFE
#endif

/*
 * IMM related kernel calls
 */
W32KAPI
HIMC
NtUserCreateInputContext(
    IN ULONG_PTR dwClientImcData);

W32KAPI
BOOL
NtUserDestroyInputContext(
    IN HIMC hImc);

typedef enum _AIC_STATUS {
    AIC_SUCCESS,
    AIC_FOCUSCONTEXTCHANGED,
    AIC_ERROR,
} AIC_STATUS;

W32KAPI
AIC_STATUS
NtUserAssociateInputContext(
    IN HWND hwnd,
    IN HIMC hImc,
    IN DWORD dwFlag);

typedef enum _UPDATEINPUTCONTEXTCLASS {
    UpdateClientInputContext,
    UpdateInUseImeWindow,
} UPDATEINPUTCONTEXTCLASS;

W32KAPI
BOOL
NtUserUpdateInputContext(
    IN HIMC hImc,
    IN UPDATEINPUTCONTEXTCLASS UpdateType,
    IN ULONG_PTR UpdateValue);

typedef enum _INPUTCONTEXTINFOCLASS {
    InputContextProcess,
    InputContextThread,
    InputContextDefaultImeWindow,
    InputContextDefaultInputContext,
} INPUTCONTEXTINFOCLASS;

W32KAPI
ULONG_PTR
NtUserQueryInputContext(
    IN HIMC hImc,
    IN INPUTCONTEXTINFOCLASS InputContextInfo);

W32KAPI
NTSTATUS
NtUserBuildHimcList(
    IN DWORD  idThread,
    IN UINT   cHimcMax,
    OUT HIMC  *phimcFirst,
    OUT PUINT  pcHimcNeeded);

typedef enum _IMEINFOEXCLASS {
    ImeInfoExKeyboardLayout,
    ImeInfoExImeWindow,
    ImeInfoExImeFileName,
} IMEINFOEXCLASS;

W32KAPI
BOOL
NtUserGetImeInfoEx(
    IN OUT PIMEINFOEX piiex,
    IN IMEINFOEXCLASS SearchType);

W32KAPI
BOOL
NtUserSetImeInfoEx(
    IN PIMEINFOEX piiex);

W32KAPI
BOOL
NtUserGetImeHotKey(
    IN DWORD dwID,
    OUT PUINT puModifiers,
    OUT PUINT puVKey,
    OUT HKL  *phkl);

W32KAPI
BOOL
NtUserSetImeHotKey(
    IN DWORD dwID,
    IN UINT  uModifiers,
    IN UINT  uVKey,
    IN HKL   hkl,
    IN DWORD dwAction);

W32KAPI
DWORD
NtUserCheckImeHotKey(
    IN UINT uVKey,
    IN LPARAM lParam);

W32KAPI
BOOL
NtUserSetAppImeLevel(
    IN HWND hWnd,
    IN DWORD dwLevel);

W32KAPI
DWORD
NtUserGetAppImeLevel(
    IN HWND hWnd );


W32KAPI
BOOL
NtUserSetImeOwnerWindow(
    IN HWND hwndIme,
    IN HWND hwndFocus);

W32KAPI
VOID
NtUserSetThreadLayoutHandles(
    IN HKL hklNew,
    IN HKL hklOld);

W32KAPI
VOID
NtUserNotifyIMEStatus(
    IN HWND hwnd,
    IN DWORD dwOpenStatus,
    IN DWORD dwConversion);

W32KAPI
BOOL
NtUserDisableThreadIme(
    IN DWORD dwThreadId);

#endif // _IMMSTRUC_
