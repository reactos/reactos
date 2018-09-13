/*---------------------------------------------------------------------------*\
| WINCHAT MAIN HEADER FILE
|   This is the main header file for the application.
|
|
| Copyright (c) Microsoft Corp., 1990-1993
|
| created: 01-Nov-91
| history: 01-Nov-91 <clausgi>  created.
|          29-Dec-92 <chriswil> port to NT, cleanup.
|          19-Oct-93 <chriswil> unicode enhancements from a-dianeo.
|
\*---------------------------------------------------------------------------*/

//////////// compile options //////////////
#define BRD 6
///////////////////////////////////////////


#ifdef WIN16
#define APIENTRY FAR PASCAL
#define ERROR_NO_NETWORK   0
#endif

#ifdef PROTOCOL_NEGOTIATE
typedef DWORD      PCKT;        // Bitfield capabilities.
#define CHT_VER    0x100        // Version 1.00 of WinChat.
#define PCKT_TEXT  0x00000001   // All versions had better support this.
#endif




// Constants.
//
#define SZBUFSIZ            255     // maximum size buffer.
#define SMLRCBUF             32     // size A .rc file buffer
#define BIGRCBUF             64     // size B .rc file buffer
#define UNCNLEN              32     //
#define CTRL_V               22     // Edit-control paste acccelerator.
#define IDACCELERATORS        1     // Menu accelerator resource ID.



// Menuhelp Constants.
//
#define MH_BASE             0x1000
#define MH_POPUPBASE        0x1100



// Child-Window ID's for send/receive windows.
//
#define ID_BASE             0x0CAC
#define ID_EDITSND          (ID_BASE + 0)
#define ID_EDITRCV          (ID_BASE + 1)



// Child-Window Identifiers for toolbar/statusbar.
//
#define IDC_TOOLBAR         200
#define IDBITMAP            201
#define IDSTATUS            202



// Menu Identifiers.
//
#define IDM_EDITFIRST       IDM_EDITUNDO
#define IDM_EDITLAST        IDM_EDITSELECT
#define IDM_ABOUT           100
#define IDM_DIAL            101
#define IDM_HANGUP          102
#define IDM_ANSWER          103
#define IDM_EXIT            104
#define IDM_EDITUNDO        105
#define IDM_EDITCUT         106
#define IDM_EDITCOPY        107
#define IDM_EDITPASTE       108
#define IDM_EDITCLEAR       109
#define IDM_EDITSELECT      110
#define IDM_SOUND           111
#define IDM_PREFERENCES     112
#define IDM_FONT            113
#define IDM_CONTENTS        114
#define IDM_SEARCHHELP      115
#define IDM_HELPHELP        116
#define IDM_COLOR           117
#define IDM_TOPMOST         118
#define IDM_CLOCK           119
#define IDM_TOOLBAR         120
#define IDM_STATUSBAR       121
#define IDM_SWITCHWIN       122
#define IDX_DEFERFONTCHANGE 123
#define IDX_UNICODECONV     126
#define IDM_FIRST           IDM_ABOUT

#define IDH_SELECTCOMPUTER  200


#ifdef PROTOCOL_NEGOTIATE
#define IDX_DEFERPROTOCOL   124
#endif


// Resource-String Identifiers.
//
#define IDS_HELV             1
#define IDS_APPNAME          2
#define IDS_LONGAPPNAME      3
#define IDS_SYSERR           4
#define IDS_CONNECTTO        5
#define IDS_ALREADYCONNECT   6
#define IDS_ABANDONFIRST     7
#define IDS_DIALING          8
#define IDS_YOUCALLER        9
#define IDS_NOTCALLED       10
#define IDS_NOTCONNECTED    11
#define IDS_CONNECTABANDON  12
#define IDS_HANGINGUP       13
#define IDS_HASTERMINATED   14
#define IDS_CONNECTEDTO     15
#define IDS_ISCALLING       16
#define IDS_CONNECTING      17
#define IDS_SERVICENAME     18
#define IDS_DIALHELP        19
#define IDS_ANSWERHELP      20
#define IDS_HANGUPHELP      21
#define IDS_NOCONNECT       22
#define IDS_ALWAYSONTOP     23
#define IDS_NOCONNECTTO     24
#define IDS_NONETINSTALLED  25

#define IDS_INISECTION      26
#define IDS_INIPREFKEY      27
#define IDS_INIFONTKEY      28
#define IDS_INIRINGIN       29
#define IDS_INIRINGOUT      30

#define IDS_TSNOTSUPPORTED  31


// Edit-Control Notification codes.  These
// are sent to the parent of the edit
// control just as any system-notify is.
//
#define EN_CHAR             0x060F
#define EN_PASTE            0x0610

// FE specific
#define EN_DBCS_STRING      0x0611


// Chat formats.  These are used to identify
// the type of data being transfered in a
// DDE transaction.
//
#define CHT_CHAR            0x100
#define CHT_FONTA           0x101
#define CHT_PASTEA          0x102
#define CHT_UNICODE         0x110
#define CHT_FONTW           0x111
#define CHT_PASTEW          0x112

// FE specific (not Taiwan)
#define CHT_DBCS_STRING     0x103


#ifdef PROTOCOL_NEGOTIATE
#define CHT_PROTOCOL        0x105
#endif


#if 0
#define CHT_HPENDATA        0x103   // defined in WFW311.  Conflicts w/DBCS.
#define CHT_CLEARPENDATA    0x104   //
#define CHT_ADDCHATTER      0x106   //
#define CHT_DELCHATTER      0x107   //
#define CHT_CHARBURST       0x108   //
#endif



// Window Related Functions  (winchat.c)
//
int     PASCAL   WinMain(HINSTANCE,HINSTANCE,LPSTR,int);
LRESULT CALLBACK MainWndProc(HWND,UINT,WPARAM,LPARAM);
LRESULT CALLBACK EditProc(HWND,UINT,WPARAM,LPARAM);
BOOL    FAR      InitApplication(HINSTANCE);
BOOL    FAR      InitInstance(HINSTANCE,int);
VOID    FAR      UpdateButtonStates(VOID);
VOID    FAR      AdjustEditWindows(VOID);
LONG    FAR      myatol(LPTSTR);
BOOL    FAR      appGetComputerName(LPTSTR);
VOID             DrawShadowRect(HDC,LPRECT);
VOID             SendFontToPartner(VOID);
VOID             DoRing(LPCTSTR);
VOID             ClearEditControls(VOID);



// Initialization Routines  (wcinit.c)
//
VOID FAR SaveFontToIni(VOID);
VOID FAR SaveBkGndToIni(VOID);
VOID FAR InitFontFromIni(VOID);
VOID FAR LoadIntlStrings(VOID);
VOID FAR SaveWindowPlacement(PWINDOWPLACEMENT);
BOOL FAR ReadWindowPlacement(PWINDOWPLACEMENT);
VOID FAR CreateTools(HWND);
VOID FAR DeleteTools(HWND);
VOID FAR CreateChildWindows(HWND);



// Window handler routines  (winchat.c)
//
VOID    appWMCreateProc(HWND);
VOID    appWMWinIniChangeProc(HWND);
VOID    appWMSetFocusProc(HWND);
VOID    appWMMenuSelectProc(HWND,WPARAM,LPARAM);
VOID    appWMTimerProc(HWND);
VOID    appWMPaintProc(HWND);
VOID    appWMDestroyProc(HWND);
BOOL    appWMCommandProc(HWND,WPARAM,LPARAM);
VOID    appWMInitMenuProc(HMENU);
VOID    appWMSizeProc(HWND,WPARAM,LPARAM);
BOOL    appWMEraseBkGndProc(HWND);
LRESULT appWMSysCommandProc(HWND,WPARAM,LPARAM);
HBRUSH  appWMCtlColorProc(HWND,WPARAM,LPARAM);
HICON   appWMQueryDragIconProc(HWND);



// DDE Related Functions.
//
HDDEDATA CALLBACK DdeCallback(UINT,UINT,HCONV,HSZ,HSZ,HDDEDATA,DWORD,DWORD);
HDDEDATA          CreateCharData(VOID);
HDDEDATA          CreatePasteData(VOID);

// FE specific
HDDEDATA          CreateDbcsStringData(VOID);

#ifdef PROTOCOL_NEGOTIATE
HDDEDATA          CreateProtocolData(VOID);
PCKT              GetCurrentPckt(VOID);
VOID              FlagIntersection(PCKT);
VOID              AnnounceSupport(VOID);
#endif


//
//
typedef UINT (WINAPI *WNETCALL)(HWND,LPTSTR,LPTSTR,WORD,DWORD);
HINSTANCE APIENTRY WNetGetCaps(WORD);



// Chat Data.
//   This data-structure must maintain
//   fixed-size fields so that they may
//   be transfered accross platforms.
//
#ifndef RC_INVOLKED

#define LF_XPACKFACESIZE  32
#define XCHATSIZEA        60
#define XCHATSIZEW        92

#pragma pack(2)
typedef struct tagXPACKFONTA
{
    WORD lfHeight;
    WORD lfWidth;
    WORD lfEscapement;
    WORD lfOrientation;
    WORD lfWeight;
    BYTE lfItalic;
    BYTE lfUnderline;
    BYTE lfStrikeOut;
    BYTE lfCharSet;
    BYTE lfOutPrecision;
    BYTE lfClipPrecision;
    BYTE lfQuality;
    BYTE lfPitchAndFamily;
    CHAR lfFaceName[LF_XPACKFACESIZE];
} XPACKFONTA;

typedef struct tagXPACKFONTW
{
    WORD lfHeight;
    WORD lfWidth;
    WORD lfEscapement;
    WORD lfOrientation;
    WORD lfWeight;
    BYTE lfItalic;
    BYTE lfUnderline;
    BYTE lfStrikeOut;
    BYTE lfCharSet;
    BYTE lfOutPrecision;
    BYTE lfClipPrecision;
    BYTE lfQuality;
    BYTE lfPitchAndFamily;
    WCHAR lfFaceName[LF_XPACKFACESIZE];
} XPACKFONTW;

typedef struct _CHATDATAA
{
    WORD type;

    union
    {

        // This data for DBCS string transfer.
        //
        struct
        {
            DWORD   SelPos;
            DWORD   size;
            HGLOBAL hString;
        } cd_dbcs;

        // This data for character transfer.
        //
        struct
        {
            DWORD SelPos;
            WORD  Char;
        } cd_char;


        // This data for remote font change.
        //
        struct
        {
            XPACKFONTA lf;
            COLORREF   cref;
            COLORREF   brush;
        } cd_win;


        // This data for remote paste.
        //
        struct
        {
            DWORD SelPos;
            DWORD size;
        } cd_paste;

#ifdef PROTOCOL_NEGOTIATE
        // This data for Protocol Negotiate.
        //
        struct
        {
            DWORD dwVer;
            PCKT  pckt;
        } cd_protocol;
#endif

    } uval;
} CHATDATAA;

typedef struct _CHATDATAW
{
    WORD type;

    union
    {

        // This data for DBCS string transfer.
        //
        struct
        {
            DWORD   SelPos;
            DWORD   size;
            HGLOBAL hString;
        } cd_dbcs;

        // This data for character transfer.
        //
        struct
        {
            DWORD SelPos;
            WORD  Char;
        } cd_char;


        // This data for remote font change.
        //
        struct
        {
            XPACKFONTW  lf;
            COLORREF    cref;
            COLORREF    brush;
        } cd_win;


        // This data for remote paste.
        //
        struct
        {
            DWORD SelPos;
            DWORD size;
        } cd_paste;

#ifdef PROTOCOL_NEGOTIATE
        // This data for Protocol Negotiate.
        //
        struct
        {
            DWORD dwVer;
            PCKT  pckt;
        } cd_protocol;
#endif

    } uval;

} CHATDATAW;

#pragma pack()

typedef XPACKFONTA      *PXPACKFONTA;
typedef XPACKFONTA NEAR *NPXPACKFONTA;
typedef XPACKFONTA FAR  *LPXPACKFONTA;

typedef CHATDATAA       *PCHATDATAA;
typedef CHATDATAA NEAR  *NPCHATDATAA;
typedef CHATDATAA FAR   *LPCHATDATAA;

typedef XPACKFONTW      *PXPACKFONTW;
typedef XPACKFONTW NEAR *NPXPACKFONTW;
typedef XPACKFONTW FAR  *LPXPACKFONTW;

typedef CHATDATAW       *PCHATDATAW;
typedef CHATDATAW NEAR  *NPCHATDATAW;
typedef CHATDATAW FAR   *LPCHATDATAW;

#ifdef UNICODE
#define XPACKFONT  XPACKFONTW
#else
#define XPACKFONT  XPACKFONTA
#endif

typedef XPACKFONT      *PXPACKFONT;
typedef XPACKFONT NEAR *NPXPACKFONT;
typedef XPACKFONT FAR  *LPXPACKFONT;

#ifdef UNICODE
#define CHATDATA   CHATDATAW
#else
#define CHATDATA   CHATDATAA
#endif

typedef CHATDATA      *PCHATDATA;
typedef CHATDATA NEAR *NPCHATDATA;
typedef CHATDATA FAR  *LPCHATDATA;

#endif


// Chat state info struct
//
typedef struct _CHATSTATE
{
    UINT fConnected          : 1;
    UINT fConnectPending     : 1;
    UINT fAllowAnswer        : 1;
    UINT fIsServer           : 1;
    UINT fServerVerified     : 1;
    UINT fInProcessOfDialing : 1;
    UINT fSound              : 1;
    UINT fMMSound            : 1;
    UINT fUseOwnFont         : 1;
    UINT fSideBySide         : 1;
    UINT fMinimized          : 1;
    UINT fTopMost            : 1;
    UINT fToolBar            : 1;
    UINT fStatusBar          : 1;
    UINT fUnicode            : 1;

#ifdef PROTOCOL_NEGOTIATE
    UINT fProtocolSent       : 1;
#endif

} CHATSTATE;
typedef CHATSTATE      *PCHATSTATE;
typedef CHATSTATE NEAR *NPCHATSTATE;
typedef CHATSTATE FAR  *LPCHATSTATE;



// Insertable macroes.
//
#define KILLSOUND              {if(ChatState.fMMSound) sndPlaySound(NULL,SND_ASYNC);}
#define SetStatusWindowText(x) {if(hwndStatus)SendMessage(hwndStatus,SB_SETTEXT,0,(LPARAM)(LPSTR)(x));}



// Helpfull porting macroes.  These were necessary
// especially for notification codes which changed
// drastically between DOS/WIN and NT.
//
#ifdef WIN32
#define GET_WM_MENUSELECT_CMD(wParam,lParam)    (UINT)(int)(short)LOWORD(wParam)
#define GET_WM_MENUSELECT_FLAGS(wParam,lParam)  (UINT)(int)(short)HIWORD(wParam)
#define GET_WM_MENUSELECT_HMENU(wParam,lParam)  (HMENU)lParam
#define SET_EM_SETSEL_WPARAM(nStart,nEnd)       (WPARAM)nStart
#define SET_EM_SETSEL_LPARAM(nStart,nEnd)       (LPARAM)nEnd
#define GET_WM_CTLCOLOREDIT_HDC(wParam,lParam)  (HDC)wParam
#define GET_WM_CTLCOLOREDIT_HWND(wParam,lParam) (HWND)lParam
#define GET_EN_SETFOCUS_NOTIFY(wParam,lParam)   (UINT)HIWORD(wParam)
#define GET_EN_SETFOCUS_CMD(wParam,lParam)      (UINT)LOWORD(wParam)
#define GET_EN_SETFOCUS_HWND(wParam,lParam)     (HWND)lParam
#define SET_EN_NOTIFY_WPARAM(id,notify,hwnd)    (WPARAM)MAKELONG(id,notify)
#define SET_EN_NOTIFY_LPARAM(id,notify,hwnd)    (LPARAM)hwnd
#define WNETGETCAPS(wFlag)                      NULL
#define WNETGETUSER(szlocal,szuser,ncount)      WNetGetUser(szlocal,szuser,ncount)
#define SETMESSAGEQUEUE(size)                   size

#else

int APIENTRY ShellAbout(HWND hWnd, LPSTR szApp, LPSTR szOtherStuff, HICON hIcon);
#define GET_WM_MENUSELECT_CMD(wParam,lParam)    (UINT)wParam
#define GET_WM_MENUSELECT_FLAGS(wParam,lParam)  (UINT)(LOWORD(lParam))
#define GET_WM_MENUSELECT_HMENU(wParam,lParam)  (HMENU)(HIWORD(lParam))
#define SET_EM_SETSEL_WPARAM(nStart,nEnd)       (WPARAM)0
#define SET_EM_SETSEL_LPARAM(nStart,nEnd)       (LPARAM)(MAKELONG(nStart,nEnd))
#define GET_WM_CTLCOLOREDIT_HDC(wParam,lParam)  (HDC)wParam
#define GET_WM_CTLCOLOREDIT_HWND(wParam,lParam) (HWND)(LOWORD(lParam))
#define GET_EN_SETFOCUS_NOTIFY(wParam,lParam)   (UINT)(HIWORD(lParam))
#define GET_EN_SETFOCUS_CMD(wParam,lParam)      (UINT)wParam
#define GET_EN_SETFOCUS_HWND(wParam,lParam)     (HWND)(LOWORD(lParam))
#define SET_EN_NOTIFY_WPARAM(id,notify,hwnd)    (WPARAM)id
#define SET_EN_NOTIFY_LPARAM(id,notify,hwnd)    (LPARAM)(MAKELONG(hwnd,notify))
#define WNETGETCAPS(wFlag)                      WNetGetCaps(wFlag)
#define WNETGETUSER(szlocal,szuser,ncount)      ERROR_NO_NETWORK
#define SETMESSAGEQUEUE(size)                   SetMessageQueue(size)
#endif




VOID PackFont(LPXPACKFONT,LPLOGFONT);
VOID UnpackFont(LPLOGFONT,LPXPACKFONT);
VOID StartIniMapping(VOID);
VOID EndIniMapping(VOID);

#ifndef ByteCountOf
#define ByteCountOf(x) sizeof(TCHAR)*(x)
#endif

#include "globals.h"


