/*++

Copyright (c) 1985 - 1999, Microsoft Corporation

Module Name:

    conime.h

Abstract:

    This module contains the internal structures and definitions used
    by the console IME.

Author:

    v-HirShi Jul.4.1995

Revision History:

--*/


#define CONSOLEIME_EVENT  (L"ConsoleIME_StartUp_Event")

typedef struct _CONIME_CANDMESSAGE {
    DWORD AttrOff;
    WCHAR String[];
} CONIME_CANDMESSAGE, *LPCONIME_CANDMESSAGE;

typedef struct _CONIME_UIMESSAGE {
    WCHAR String[];
} CONIME_UIMESSAGE, *LPCONIME_UIMESSAGE;

typedef struct _CONIME_UICOMPMESSAGE {
    DWORD dwSize;
    DWORD dwCompAttrLen;
    DWORD dwCompAttrOffset;
    DWORD dwCompStrLen;
    DWORD dwCompStrOffset;
    DWORD dwResultStrLen;
    DWORD dwResultStrOffset;
    WORD  CompAttrColor[8];
} CONIME_UICOMPMESSAGE, *LPCONIME_UICOMPMESSAGE;

#define VIEW_LEFT  0
#define VIEW_RIGHT 1
#define MAXSTATUSCOL 160
typedef struct _CONIME_UIMODEINFO {
    DWORD ModeStringLen;
    BOOL Position;
    CHAR_INFO ModeString[MAXSTATUSCOL];
} CONIME_UIMODEINFO, *LPCONIME_UIMODEINFO;


//
// This is PCOPYDATASTRUCT->dwData values for WM_COPYDAT message consrv from conime.
//
#define CI_CONIMECOMPOSITION    0x4B425930
#define CI_CONIMEMODEINFO       0x4B425931
#define CI_CONIMESYSINFO        0x4B425932
#define CI_CONIMECANDINFO       0x4B425935
#define CI_CONIMEPROPERTYINFO   0x4B425936



//
// This message values for send/post message conime from consrv
//
#define CONIME_CREATE                   (WM_USER+0)
#define CONIME_DESTROY                  (WM_USER+1)
#define CONIME_SETFOCUS                 (WM_USER+2)
#define CONIME_KILLFOCUS                (WM_USER+3)
#define CONIME_HOTKEY                   (WM_USER+4)
#define CONIME_GET_NLSMODE              (WM_USER+5)
#define CONIME_SET_NLSMODE              (WM_USER+6)
#define CONIME_NOTIFY_SCREENBUFFERSIZE  (WM_USER+7)
#define CONIME_NOTIFY_VK_KANA           (WM_USER+8)
#define CONIME_INPUTLANGCHANGE          (WM_USER+9)
#define CONIME_NOTIFY_CODEPAGE          (WM_USER+10)
#define CONIME_INPUTLANGCHANGEREQUEST   (WM_USER+11)
#define CONIME_INPUTLANGCHANGEREQUESTFORWARD   (WM_USER+12)
#define CONIME_INPUTLANGCHANGEREQUESTBACKWARD   (WM_USER+13)
#define CONIME_KEYDATA                  (WM_USER+1024)

//
// This message values for set direction of conime langchange
//
#define CONIME_DIRECT                    0
#define CONIME_FORWARD                   1
#define CONIME_BACKWARD                 -1

//
// This message value is for send/post message to consrv
//
#define CM_CONIME_KL_ACTIVATE           (WM_USER+15)

#define CONIME_SENDMSG_TIMEOUT          (3 * 1000)    // Wait for 3sec.


//
// This is extended NLS mode for console and console IME
//
#define IME_CMODE_OPEN     0x40000000
#define IME_CMODE_DISABLE  0x80000000



//
// Default composition color attributes
//
#define DEFAULT_COMP_ENTERED            \
    (FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED |                        \
     COMMON_LVB_UNDERSCORE)
#define DEFAULT_COMP_ALREADY_CONVERTED  \
    (FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED |                        \
     BACKGROUND_BLUE )
#define DEFAULT_COMP_CONVERSION         \
    (FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED |                        \
     COMMON_LVB_UNDERSCORE)
#define DEFAULT_COMP_YET_CONVERTED      \
    (FOREGROUND_BLUE |                                                            \
     BACKGROUND_BLUE | BACKGROUND_GREEN | BACKGROUND_RED |                        \
     COMMON_LVB_UNDERSCORE)
#define DEFAULT_COMP_INPUT_ERROR        \
    (                                     FOREGROUND_RED |                        \
     COMMON_LVB_UNDERSCORE)
