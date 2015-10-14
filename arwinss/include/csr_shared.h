#pragma once

/* FNID's for NtUserSetWindowFNID, NtUserMessageCall */
#define FNID_FIRST                  0x029A
#define FNID_SCROLLBAR              0x029A
#define FNID_ICONTITLE              0x029B
#define FNID_MENU                   0x029C
#define FNID_DESKTOP                0x029D
#define FNID_DEFWINDOWPROC          0x029E
#define FNID_MESSAGEWND             0x029F
#define FNID_SWITCH                 0x02A0
#define FNID_BUTTON                 0x02A1
#define FNID_COMBOBOX               0x02A2
#define FNID_COMBOLBOX              0x02A3
#define FNID_DIALOG                 0x02A4
#define FNID_EDIT                   0x02A5
#define FNID_LISTBOX                0x02A6
#define FNID_MDICLIENT              0x02A7
#define FNID_STATIC                 0x02A8
#define FNID_IME                    0x02A9
#define FNID_GHOST                  0x02AA
#define FNID_CALLWNDPROC            0x02AB
#define FNID_CALLWNDPROCRET         0x02AC
#define FNID_HKINLPCWPEXSTRUCT      0x02AD
#define FNID_HKINLPCWPRETEXSTRUCT   0x02AE
#define FNID_MB_DLGPROC             0x02AF
#define FNID_MDIACTIVATEDLGPROC     0x02B0
#define FNID_SENDMESSAGE            0x02B1
#define FNID_SENDMESSAGEFF          0x02B2
/* Kernel has option to use TimeOut or normal msg send, based on type of msg. */
#define FNID_SENDMESSAGEWTOOPTION   0x02B3
#define FNID_SENDMESSAGECALLPROC    0x02B4
#define FNID_BROADCASTSYSTEMMESSAGE 0x02B5
#define FNID_TOOLTIPS               0x02B6
#define FNID_SENDNOTIFYMESSAGE      0x02B7
#define FNID_SENDMESSAGECALLBACK    0x02B8
#define FNID_LAST                   0x02B9

typedef struct _WNDMSG
{
    DWORD maxMsgs;
    PINT abMsgs;
} WNDMSG, *PWNDMSG;

typedef PVOID PSERVERINFO;

typedef struct _SHAREDINFO
{
    PSERVERINFO psi; /* global Server Info */
    PVOID aheList; /* Handle Entry List */
    PVOID pDispInfo; /* global PDISPLAYINFO pointer */
    ULONG_PTR ulSharedDelta; /* Heap delta */
    WNDMSG awmControl[FNID_LAST - FNID_FIRST];
    WNDMSG DefWindowMsgs;
    WNDMSG DefWindowSpecMsgs;
} SHAREDINFO, *PSHAREDINFO;

/* See also the USERSRV_API_CONNECTINFO #define in include/reactos/subsys/win/winmsg.h */
typedef struct _USERCONNECT
{
    ULONG ulVersion;
    ULONG ulCurrentVersion;
    DWORD dwDispatchCount;
    SHAREDINFO siClient;
} USERCONNECT, *PUSERCONNECT;

/* WinNT 5.0 compatible user32 / win32k */
#define USER_VERSION MAKELONG(0x0000, 0x0005)

#if defined(_M_IX86)
C_ASSERT(sizeof(USERCONNECT) == 0x124);
#endif
