/**************************************************************************\
* Module Name: server.c
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* Server support routines for the CSR stuff.  This basically performs the
* startup/initialization for USER.
*
\**************************************************************************/

#include "precomp.h"
#pragma hdrstop

extern WORD gDispatchTableValues;

BOOL gbUserInitialized;

/*
 * Initialization Routines (external).
 */
NTSTATUS     InitQEntryLookaside(VOID);
NTSTATUS     InitSMSLookaside(VOID);

NTSTATUS    InitCreateSharedSection(VOID);
NTSTATUS    InitCreateObjectDirectory(VOID);
BOOL        InitCreateUserSubsystem(VOID);
VOID        InitFunctionTables(VOID);
VOID        InitMessageTables(VOID);
VOID        InitWindowMsgTable(PBYTE*, PUINT, CONST WORD*);

VOID        VerifySyncOnlyMessages(VOID);
BOOL        InitOLEFormats(VOID);
NTSTATUS    Win32UserInitialize(VOID);

#pragma alloc_text(INIT, InitCreateSharedSection)
#pragma alloc_text(INIT, InitCreateUserCrit)
#pragma alloc_text(INIT, InitCreateObjectDirectory)
#pragma alloc_text(INIT, InitCreateUserSubsystem)
//#pragma alloc_text(INIT, InitDbgTags)
#pragma alloc_text(INIT, InitFunctionTables)
#pragma alloc_text(INIT, InitMessageTables)
#pragma alloc_text(INIT, InitWindowMsgTable)

#pragma alloc_text(INIT, VerifySyncOnlyMessages)
#pragma alloc_text(INIT, InitOLEFormats)
#pragma alloc_text(INIT, Win32UserInitialize)

/*
 * Constants pertaining to the user-initialization.
 */
#define USRINIT_SHAREDSECT_SIZE   32
#define USRINIT_ATOMBUCKET_SIZE   37

#define USRINIT_WINDOWSECT_SIZE  512
#define USRINIT_NOIOSECT_SIZE    128

#define USRINIT_SHAREDSECT_BUFF_SIZE     640
#define USRINIT_SHAREDSECT_READ_SIZE     (USRINIT_SHAREDSECT_BUFF_SIZE-33)


/***************************************************************************\
* Globals stored in the INIT section. These should only be accessed at
* load time!
\***************************************************************************/
#ifdef ALLOC_DATA_PRAGMA
#pragma data_seg("INIT$Data")
#endif

CONST WCHAR szCHECKPOINT_PROP_NAME[]  = L"SysCP";
CONST WCHAR szDDETRACK_PROP_NAME[]    = L"SysDT";
CONST WCHAR szQOS_PROP_NAME[]         = L"SysQOS";
CONST WCHAR szDDEIMP_PROP_NAME[]      = L"SysDDEI";
CONST WCHAR szWNDOBJ_PROP_NAME[]      = L"SysWNDO";
CONST WCHAR szIMELEVEL_PROP_NAME[]    = L"SysIMEL";
CONST WCHAR szLAYER_PROP_NAME[]       = L"SysLayer";
CONST WCHAR szUSER32[]                = L"USER32";
CONST WCHAR szMESSAGE[]               = L"Message";
CONST WCHAR szCONTEXTHELPIDPROP[]     = L"SysCH";
CONST WCHAR szICONSM_PROP_NAME[]      = L"SysICS";
CONST WCHAR szICON_PROP_NAME[]        = ICON_PROP_NAME;
CONST WCHAR szSHELLHOOK[]             = L"SHELLHOOK";
CONST WCHAR szACTIVATESHELLWINDOW[]   = L"ACTIVATESHELLWINDOW";
CONST WCHAR szOTHERWINDOWCREATED[]    = L"OTHERWINDOWCREATED";
CONST WCHAR szOTHERWINDOWDESTROYED[]  = L"OTHERWINDOWDESTROYED";
CONST WCHAR szOLEMAINTHREADWNDCLASS[] = L"OleMainThreadWndClass";
CONST WCHAR szFLASHWSTATE[]           = L"FlashWState";

#ifdef HUNGAPP_GHOSTING
CONST WCHAR szGHOST[]                 = L"Ghost";
#endif // HUNGAPP_GHOSTING


/***************************************************************************\
* Message Tables
*
*   DefDlgProc
*   MenuWndProc
*   ScrollBarWndProc
*   StaticWndProc
*   ButtonWndProc
*   ListboxWndProc
*   ComboWndProc
*   EditWndProc
*   DefWindowMsgs
*   DefWindowSpecMsgs
*
* These are used in InitMessageTables() to initialize gSharedInfo.awmControl[]
* using the INITMSGTABLE() macro.
*
* 25-Aug-1995 ChrisWil  Created comment block.
\***************************************************************************/

CONST WORD gawDefDlgProc[] = {
    WM_COMPAREITEM,
    WM_VKEYTOITEM,
    WM_CHARTOITEM,
    WM_INITDIALOG,
    WM_QUERYDRAGICON,
    WM_CTLCOLOR,
    WM_CTLCOLORMSGBOX,
    WM_CTLCOLOREDIT,
    WM_CTLCOLORLISTBOX,
    WM_CTLCOLORBTN,
    WM_CTLCOLORDLG,
    WM_CTLCOLORSCROLLBAR,
    WM_CTLCOLORSTATIC,
    WM_ERASEBKGND,
    WM_SHOWWINDOW,
    WM_SYSCOMMAND,
    WM_SYSKEYDOWN,
    WM_ACTIVATE,
    WM_SETFOCUS,
    WM_CLOSE,
    WM_NCDESTROY,
    WM_FINALDESTROY,
    DM_REPOSITION,
    DM_SETDEFID,
    DM_GETDEFID,
    WM_NEXTDLGCTL,
    WM_ENTERMENULOOP,
    WM_LBUTTONDOWN,
    WM_NCLBUTTONDOWN,
    WM_GETFONT,
    WM_NOTIFYFORMAT,
    WM_INPUTLANGCHANGEREQUEST,
    0
};

CONST WORD gawMenuWndProc[] = {
    WM_NCCREATE,
    WM_FINALDESTROY,
    WM_PAINT,
    WM_NCCALCSIZE,
    WM_CHAR,
    WM_SYSCHAR,
    WM_KEYDOWN,
    WM_SYSKEYDOWN,
    WM_TIMER,
    MN_SETHMENU,
    MN_SIZEWINDOW,
    MN_OPENHIERARCHY,
    MN_CLOSEHIERARCHY,
    MN_SELECTITEM,
    MN_SELECTFIRSTVALIDITEM,
    MN_CANCELMENUS,
    MN_FINDMENUWINDOWFROMPOINT,
    MN_SHOWPOPUPWINDOW,
    MN_BUTTONDOWN,
    MN_MOUSEMOVE,
    MN_BUTTONUP,
    MN_SETTIMERTOOPENHIERARCHY,
    WM_ACTIVATE,
    MN_GETHMENU,
    MN_DBLCLK,
    MN_ACTIVATEPOPUP,
    MN_ENDMENU,
    MN_DODRAGDROP,
    WM_ACTIVATEAPP,
    WM_MOUSELEAVE,
    WM_SIZE,
    WM_MOVE,
    WM_NCHITTEST,
    WM_NCPAINT,
    WM_PRINT,
    WM_PRINTCLIENT,
    WM_ERASEBKGND,
    WM_WINDOWPOSCHANGING,
    WM_WINDOWPOSCHANGED,
    0
};

CONST WORD gawDesktopWndProc[] = {
    WM_PAINT,
    WM_ERASEBKGND,
    0
};

CONST WORD gawScrollBarWndProc[] = {
    WM_CREATE,
    WM_SETFOCUS,
    WM_KILLFOCUS,
    WM_ERASEBKGND,
    WM_PAINT,
    WM_LBUTTONDBLCLK,
    WM_LBUTTONDOWN,
    WM_KEYUP,
    WM_KEYDOWN,
    WM_ENABLE,
    SBM_ENABLE_ARROWS,
    SBM_SETPOS,
    SBM_SETRANGEREDRAW,
    SBM_SETRANGE,
    SBM_SETSCROLLINFO,
    SBM_GETSCROLLINFO,
    WM_PRINTCLIENT,
    WM_MOUSEMOVE,
    WM_MOUSELEAVE,
    0
};

CONST WORD gawStaticWndProc[] = {
    STM_GETICON,
    STM_GETIMAGE,
    STM_SETICON,
    STM_SETIMAGE,
    WM_ERASEBKGND,
    WM_PAINT,
    WM_PRINTCLIENT,
    WM_CREATE,
    WM_DESTROY,
    WM_NCCREATE,
    WM_NCDESTROY,
    WM_FINALDESTROY,
    WM_NCHITTEST,
    WM_LBUTTONDOWN,
    WM_NCLBUTTONDOWN,
    WM_LBUTTONDBLCLK,
    WM_NCLBUTTONDBLCLK,
    WM_SETTEXT,
    WM_ENABLE,
    WM_GETDLGCODE,
    WM_SETFONT,
    WM_GETFONT,
    WM_GETTEXT,
    WM_TIMER,
    WM_INPUTLANGCHANGEREQUEST,
    WM_UPDATEUISTATE,
    0
};

CONST WORD gawButtonWndProc[] = {
    WM_NCHITTEST,
    WM_ERASEBKGND,
    WM_PRINTCLIENT,
    WM_PAINT,
    WM_SETFOCUS,
    WM_GETDLGCODE,
    WM_CAPTURECHANGED,
    WM_KILLFOCUS,
    WM_LBUTTONDBLCLK,
    WM_LBUTTONUP,
    WM_MOUSEMOVE,
    WM_LBUTTONDOWN,
    WM_CHAR,
    BM_CLICK,
    WM_KEYDOWN,
    WM_KEYUP,
    WM_SYSKEYUP,
    BM_GETSTATE,
    BM_SETSTATE,
    BM_GETCHECK,
    BM_SETCHECK,
    BM_SETSTYLE,
    WM_SETTEXT,
    WM_ENABLE,
    WM_SETFONT,
    WM_GETFONT,
    BM_GETIMAGE,
    BM_SETIMAGE,
    WM_NCDESTROY,
    WM_FINALDESTROY,
    WM_NCCREATE,
    WM_INPUTLANGCHANGEREQUEST,
    WM_UPDATEUISTATE,
    0
};

CONST WORD gawListboxWndProc[] = {
    LB_GETTOPINDEX,
    LB_SETTOPINDEX,
    WM_SIZE,
    WM_ERASEBKGND,
    LB_RESETCONTENT,
    WM_TIMER,
    WM_MOUSEMOVE,
    WM_MBUTTONDOWN,
    WM_LBUTTONDOWN,
    WM_LBUTTONUP,
    WM_LBUTTONDBLCLK,
    WM_CAPTURECHANGED,
    LBCB_STARTTRACK,
    LBCB_ENDTRACK,
    WM_PRINTCLIENT,
    WM_PAINT,
    WM_NCDESTROY,
    WM_FINALDESTROY,
    WM_SETFOCUS,
    WM_KILLFOCUS,
    WM_VSCROLL,
    WM_HSCROLL,
    WM_GETDLGCODE,
    WM_CREATE,
    WM_SETREDRAW,
    WM_ENABLE,
    WM_SETFONT,
    WM_GETFONT,
    WM_DRAGSELECT,
    WM_DRAGLOOP,
    WM_DRAGMOVE,
    WM_DROPFILES,
    WM_QUERYDROPOBJECT,
    WM_DROPOBJECT,
    LB_GETITEMRECT,
    LB_GETITEMDATA,
    LB_SETITEMDATA,
    LB_ADDSTRINGUPPER,
    LB_ADDSTRINGLOWER,
    LB_ADDSTRING,
    LB_INSERTSTRINGUPPER,
    LB_INSERTSTRINGLOWER,
    LB_INSERTSTRING,
    LB_INITSTORAGE,
    LB_DELETESTRING,
    LB_DIR,
    LB_ADDFILE,
    LB_SETSEL,
    LB_SETCURSEL,
    LB_GETSEL,
    LB_GETCURSEL,
    LB_SELITEMRANGE,
    LB_SELITEMRANGEEX,
    LB_GETTEXTLEN,
    LB_GETTEXT,
    LB_GETCOUNT,
    LB_SETCOUNT,
    LB_SELECTSTRING,
    LB_FINDSTRING,
    LB_GETLOCALE,
    LB_SETLOCALE,
    WM_KEYDOWN,
    WM_CHAR,
    LB_GETSELITEMS,
    LB_GETSELCOUNT,
    LB_SETTABSTOPS,
    LB_GETHORIZONTALEXTENT,
    LB_SETHORIZONTALEXTENT,
    LB_SETCOLUMNWIDTH,
    LB_SETANCHORINDEX,
    LB_GETANCHORINDEX,
    LB_SETCARETINDEX,
    LB_GETCARETINDEX,
    LB_SETITEMHEIGHT,
    LB_GETITEMHEIGHT,
    LB_FINDSTRINGEXACT,
    LB_ITEMFROMPOINT,
    LB_SETLOCALE,
    LB_GETLOCALE,
    LBCB_CARETON,
    LBCB_CARETOFF,
    WM_NCCREATE,
    WM_WINDOWPOSCHANGED,
    WM_MOUSEWHEEL,
    WM_STYLECHANGED,
    WM_STYLECHANGING,
    0
};

CONST WORD gawComboWndProc[] = {
    CBEC_KILLCOMBOFOCUS,
    WM_COMMAND,
    WM_CTLCOLORMSGBOX,
    WM_CTLCOLOREDIT,
    WM_CTLCOLORLISTBOX,
    WM_CTLCOLORBTN,
    WM_CTLCOLORDLG,
    WM_CTLCOLORSCROLLBAR,
    WM_CTLCOLORSTATIC,
    WM_CTLCOLOR,
    WM_GETTEXT,
    WM_GETTEXTLENGTH,
    WM_CLEAR,
    WM_CUT,
    WM_PASTE,
    WM_COPY,
    WM_SETTEXT,
    WM_CREATE,
    WM_ERASEBKGND,
    WM_GETFONT,
    WM_PRINT,
    WM_PRINTCLIENT,
    WM_PAINT,
    WM_GETDLGCODE,
    WM_SETFONT,
    WM_SYSKEYDOWN,
    WM_KEYDOWN,
    WM_CHAR,
    WM_LBUTTONDBLCLK,
    WM_LBUTTONDOWN,
    WM_CAPTURECHANGED,
    WM_LBUTTONUP,
    WM_MOUSEMOVE,
    WM_NCDESTROY,
    WM_FINALDESTROY,
    WM_SETFOCUS,
    WM_KILLFOCUS,
    WM_SETREDRAW,
    WM_ENABLE,
    WM_SIZE,
    CB_GETDROPPEDSTATE,
    CB_GETDROPPEDCONTROLRECT,
    CB_SETDROPPEDWIDTH,
    CB_GETDROPPEDWIDTH,
    CB_DIR,
    CB_SETEXTENDEDUI,
    CB_GETEXTENDEDUI,
    CB_GETEDITSEL,
    CB_LIMITTEXT,
    CB_SETEDITSEL,
    CB_ADDSTRING,
    CB_DELETESTRING,
    CB_INITSTORAGE,
    CB_SETTOPINDEX,
    CB_GETTOPINDEX,
    CB_GETCOUNT,
    CB_GETCURSEL,
    CB_GETLBTEXT,
    CB_GETLBTEXTLEN,
    CB_INSERTSTRING,
    CB_RESETCONTENT,
    CB_GETHORIZONTALEXTENT,
    CB_SETHORIZONTALEXTENT,
    CB_FINDSTRING,
    CB_FINDSTRINGEXACT,
    CB_SELECTSTRING,
    CB_SETCURSEL,
    CB_GETITEMDATA,
    CB_SETITEMDATA,
    CB_SETITEMHEIGHT,
    CB_GETITEMHEIGHT,
    CB_SHOWDROPDOWN,
    CB_SETLOCALE,
    CB_GETLOCALE,
    WM_MEASUREITEM,
    WM_DELETEITEM,
    WM_DRAWITEM,
    WM_COMPAREITEM,
    WM_NCCREATE,
    WM_HELP,
    WM_MOUSEWHEEL,
    WM_MOUSELEAVE,
    WM_STYLECHANGED,
    WM_STYLECHANGING,
    WM_UPDATEUISTATE,
    0
};

CONST WORD gawEditWndProc[] = {
    EM_CANUNDO,
    EM_CHARFROMPOS,
    EM_EMPTYUNDOBUFFER,
    EM_FMTLINES,
    EM_GETFIRSTVISIBLELINE,
    EM_GETFIRSTVISIBLELINE,
    EM_GETHANDLE,
    EM_GETLIMITTEXT,
    EM_GETLINE,
    EM_GETLINECOUNT,
    EM_GETMARGINS,
    EM_GETMODIFY,
    EM_GETPASSWORDCHAR,
    EM_GETRECT,
    EM_GETSEL,
    EM_GETWORDBREAKPROC,
    EM_SETIMESTATUS,
    EM_GETIMESTATUS,
    EM_LINEFROMCHAR,
    EM_LINEINDEX,
    EM_LINELENGTH,
    EM_LINESCROLL,
    EM_POSFROMCHAR,
    EM_REPLACESEL,
    EM_SCROLL,
    EM_SCROLLCARET,
    EM_SETHANDLE,
    EM_SETLIMITTEXT,
    EM_SETMARGINS,
    EM_SETMODIFY,
    EM_SETPASSWORDCHAR,
    EM_SETREADONLY,
    EM_SETRECT,
    EM_SETRECTNP,
    EM_SETSEL,
    EM_SETTABSTOPS,
    EM_SETWORDBREAKPROC,
    EM_UNDO,
    WM_CAPTURECHANGED,
    WM_CHAR,
    WM_CLEAR,
    WM_CONTEXTMENU,
    WM_COPY,
    WM_CREATE,
    WM_CUT,
    WM_ENABLE,
    WM_ERASEBKGND,
    WM_GETDLGCODE,
    WM_GETFONT,
    WM_GETTEXT,
    WM_GETTEXTLENGTH,
    WM_HSCROLL,
    WM_IME_STARTCOMPOSITION,
    WM_IME_ENDCOMPOSITION,
    WM_IME_COMPOSITION,
    WM_IME_SETCONTEXT,
    WM_IME_NOTIFY,
    WM_IME_COMPOSITIONFULL,
    WM_IME_SELECT,
    WM_IME_CHAR,
    WM_IME_REQUEST,
    WM_INPUTLANGCHANGE,
    WM_KEYUP,
    WM_KEYDOWN,
    WM_KILLFOCUS,
    WM_MBUTTONDOWN,
    WM_LBUTTONDBLCLK,
    WM_LBUTTONDOWN,
    WM_LBUTTONUP,
    WM_MOUSEMOVE,
    WM_NCCREATE,
    WM_NCDESTROY,
    WM_RBUTTONDOWN,
    WM_RBUTTONUP,
    WM_FINALDESTROY,
#if 0
    WM_NCPAINT,
#endif
    WM_PAINT,
    WM_PASTE,
    WM_PRINTCLIENT,
    WM_SETFOCUS,
    WM_SETFONT,
    WM_SETREDRAW,
    WM_SETTEXT,
    WM_SIZE,
    WM_STYLECHANGED,
    WM_STYLECHANGING,
    WM_SYSCHAR,
    WM_SYSKEYDOWN,
    WM_SYSTIMER,
    WM_UNDO,
    WM_VSCROLL,
    WM_MOUSEWHEEL,
    0
};

CONST WORD gawImeWndProc[] = {
    WM_ERASEBKGND,
    WM_PAINT,
    WM_DESTROY,
    WM_NCDESTROY,
    WM_FINALDESTROY,
    WM_CREATE,
    WM_IME_SYSTEM,
    WM_IME_SELECT,
    WM_IME_CONTROL,
    WM_IME_SETCONTEXT,
    WM_IME_NOTIFY,
    WM_IME_COMPOSITION,
    WM_IME_STARTCOMPOSITION,
    WM_IME_ENDCOMPOSITION,
    WM_IME_REQUEST,
    WM_COPYDATA,
    0
};

/*
 * This array is for all the messages that need to be passed straight
 * across to the server for handling.
 */
CONST WORD gawDefWindowMsgs[] = {
    WM_GETHOTKEY,
    WM_SETHOTKEY,
    WM_SETREDRAW,
    WM_SETTEXT,
    WM_PAINT,
    WM_CLOSE,
    WM_ERASEBKGND,
    WM_CANCELMODE,
    WM_SETCURSOR,
    WM_PAINTICON,
    WM_ICONERASEBKGND,
    WM_DRAWITEM,
    WM_KEYF1,
    WM_ISACTIVEICON,
    WM_NCCREATE,
    WM_SETICON,
    WM_NCCALCSIZE,
    WM_NCPAINT,
    WM_NCACTIVATE,
    WM_NCMOUSEMOVE,
    WM_NCRBUTTONUP,
    WM_NCRBUTTONDOWN,
    WM_NCLBUTTONDOWN,
    WM_NCLBUTTONUP,
    WM_NCLBUTTONDBLCLK,
    WM_KEYUP,
    WM_SYSKEYUP,
    WM_SYSCHAR,
    WM_SYSCOMMAND,
    WM_QUERYDROPOBJECT,
    WM_CLIENTSHUTDOWN,
    WM_SYNCPAINT,
    WM_PRINT,
    WM_GETICON,
    WM_CONTEXTMENU,
    WM_SYSMENU,
    WM_INPUTLANGCHANGEREQUEST,
    WM_INPUTLANGCHANGE,
    WM_UPDATEUISTATE,
    0
};

/*
 * This array is for all messages that can be handled with some special
 * code by the client.  DefWindowProcWorker returns 0 for all messages
 * that aren't in this array or the one above.
 */
CONST WORD gawDefWindowSpecMsgs[] = {
    WM_ACTIVATE,
    WM_GETTEXT,
    WM_GETTEXTLENGTH,
    WM_RBUTTONUP,
    WM_QUERYENDSESSION,
    WM_QUERYOPEN,
    WM_SHOWWINDOW,
    WM_MOUSEACTIVATE,
    WM_HELP,
    WM_VKEYTOITEM,
    WM_CHARTOITEM,
    WM_KEYDOWN,
    WM_SYSKEYDOWN,
    WM_DROPOBJECT,
    WM_WINDOWPOSCHANGING,
    WM_WINDOWPOSCHANGED,
    WM_KLUDGEMINRECT,
    WM_CTLCOLOR,
    WM_CTLCOLORMSGBOX,
    WM_CTLCOLOREDIT,
    WM_CTLCOLORLISTBOX,
    WM_CTLCOLORBTN,
    WM_CTLCOLORDLG,
    WM_CTLCOLORSCROLLBAR,
    WM_NCHITTEST,
    WM_NCXBUTTONUP,
    WM_CTLCOLORSTATIC,
    WM_NOTIFYFORMAT,
    WM_DEVICECHANGE,
    WM_POWERBROADCAST,
    WM_MOUSEWHEEL,
    WM_XBUTTONUP,
    WM_IME_KEYDOWN,
    WM_IME_KEYUP,
    WM_IME_CHAR,
    WM_IME_COMPOSITION,
    WM_IME_STARTCOMPOSITION,
    WM_IME_ENDCOMPOSITION,
    WM_IME_COMPOSITIONFULL,
    WM_IME_SETCONTEXT,
    WM_IME_CONTROL,
    WM_IME_NOTIFY,
    WM_IME_SELECT,
    WM_IME_SYSTEM,
    WM_LPKDRAWSWITCHWND,
    WM_QUERYDRAGICON,
    WM_CHANGEUISTATE,
    WM_QUERYUISTATE,
    WM_APPCOMMAND,
    0
};

static CONST LPCWSTR lpszOLEFormats[] = {
    L"ObjectLink",
    L"OwnerLink",
    L"Native",
    L"Binary",
    L"FileName",
    L"FileNameW",
    L"NetworkName",
    L"DataObject",
    L"Embedded Object",
    L"Embed Source",
    L"Custom Link Source",
    L"Link Source",
    L"Object Descriptor",
    L"Link Source Descriptor",
    L"OleDraw",
    L"PBrush",
    L"MSDraw",
    L"Ole Private Data",
    L"Screen Picture",
    L"OleClipboardPersistOnFlush",
    L"MoreOlePrivateData"
};

static CONST LPCWSTR lpszControls[] = {
    L"Button",
    L"Edit",
    L"Static",
    L"ListBox",
    L"ScrollBar",
    L"ComboBox",
    L"MDIClient",
    L"ComboLBox",
    L"DDEMLEvent",
    L"DDEMLMom",
    L"DMGClass",
    L"DDEMLAnsiClient",
    L"DDEMLUnicodeClient",
    L"DDEMLAnsiServer",
    L"DDEMLUnicodeServer",
    L"IME",
};


#ifdef ALLOC_DATA_PRAGMA
#pragma data_seg()
#endif

/***************************************************************************\
* DispatchServerMessage
*
*
* 19-Aug-1992 MikeKe    Created
\***************************************************************************/

#define WRAPPFN(pfn, type)                                   \
LRESULT xxxWrap ## pfn(                                      \
    PWND  pwnd,                                              \
    UINT  message,                                           \
    WPARAM wParam,                                           \
    LPARAM lParam,                                           \
    ULONG_PTR xParam)                                         \
{                                                            \
    DBG_UNREFERENCED_PARAMETER(xParam);                      \
                                                             \
    return xxx ## pfn((type)pwnd, message, wParam, lParam);  \
}

WRAPPFN(SBWndProc, PSBWND)
WRAPPFN(MenuWindowProc, PWND)
WRAPPFN(DesktopWndProc, PWND);
WRAPPFN(DefWindowProc, PWND)

LRESULT xxxWrapSendMessage(
    PWND  pwnd,
    UINT  message,
    WPARAM wParam,
    LPARAM lParam,
    ULONG_PTR xParam)
{
    DBG_UNREFERENCED_PARAMETER(xParam);

    return xxxSendMessageTimeout(pwnd,
                                 message,
                                 wParam,
                                 lParam,
                                 SMTO_NORMAL,
                                 0,
                                 NULL);
}

LRESULT xxxWrapSendMessageBSM(
    PWND  pwnd,
    UINT  message,
    WPARAM wParam,
    LPARAM lParam,
    ULONG_PTR xParam)
{
    BROADCASTSYSTEMMSGPARAMS bsmParams;

    try {
        bsmParams = ProbeAndReadBroadcastSystemMsgParams((LPBROADCASTSYSTEMMSGPARAMS)xParam);
    } except (W32ExceptionHandler(FALSE, RIP_WARNING)) {
        return 0;
    }

    /*
     * If this broadcast is going to all desktops, make sure the thread has
     * sufficient privileges. Do the check here, so we don't effect kernel
     * generated broadcasts (i.e power messages).
     */
    if (bsmParams.dwRecipients & (BSM_ALLDESKTOPS)) {
        if (!IsPrivileged(&psTcb)) {
            bsmParams.dwRecipients &= ~(BSM_ALLDESKTOPS);
        }
    }

    return xxxSendMessageBSM(pwnd,
                             message,
                             wParam,
                             lParam,
                             &bsmParams);
}

/***************************************************************************\
* xxxUnusedFunctionId
*
* This function is catches attempts to access invalid entries in the server
* side function dispatch table.
*
\***************************************************************************/

LRESULT xxxUnusedFunctionId(
    PWND  pwnd,
    UINT  message,
    WPARAM wParam,
    LPARAM lParam,
    ULONG_PTR xParam)
{
    DBG_UNREFERENCED_PARAMETER(pwnd);
    DBG_UNREFERENCED_PARAMETER(message);
    DBG_UNREFERENCED_PARAMETER(wParam);
    DBG_UNREFERENCED_PARAMETER(lParam);
    DBG_UNREFERENCED_PARAMETER(xParam);

    UserAssert(FALSE);
    return 0;
}

/***************************************************************************\
* xxxWrapCallWindowProc
*
* Warning should only be called with valid CallProc Handles or the
* EditWndProc special handlers.
*
*
* 21-Apr-1993 JohnC     Created
\***************************************************************************/

LRESULT xxxWrapCallWindowProc(
    PWND  pwnd,
    UINT  message,
    WPARAM wParam,
    LPARAM lParam,
    ULONG_PTR xParam)
{
    PCALLPROCDATA pCPD;
    LRESULT       lRet = 0;

    if (pCPD = HMValidateHandleNoRip((PVOID)xParam, TYPE_CALLPROC)) {

        lRet = ScSendMessage(pwnd,
                              message,
                              wParam,
                              lParam,
                              pCPD->pfnClientPrevious,
                              gpsi->apfnClientW.pfnDispatchMessage,
                              (pCPD->wType & CPD_UNICODE_TO_ANSI) ?
                                      SCMS_FLAGS_ANSI : 0);

    } else {

        /*
         * If it is not a real call proc handle it must be a special
         * handler for editwndproc or regular EditWndProc
         */
        lRet = ScSendMessage(pwnd,
                              message,
                              wParam,
                              lParam,
                              xParam,
                              gpsi->apfnClientA.pfnDispatchMessage,
                              (xParam == (ULONG_PTR)gpsi->apfnClientA.pfnEditWndProc) ?
                                      SCMS_FLAGS_ANSI : 0);
    }

    return lRet;
}

#if DBG
VOID VerifySyncOnlyMessages(VOID)
{
    int i;

    TRACE_INIT(("UserInit: Verify Sync Only Messages\n"));

    /*
     * There are a couple of thunks that just pass parameters.  There are other
     * thunks besides SfnDWORD that do a straight pass through because they
     * do other processing beside the wparam and lparam
     */

    /*
     * Allow posting of LB_DIR and CB_DIR because DlgDirList allows a DDL_POSTMSGS
     * flag that makes the API post the messages.  This should be OK as long as we
     * don't handle these messages in the kernel.  NT 3.51 allowed posting these.
     */
    for (i=0; i<WM_USER; i++) {
        if (    i != LB_DIR
                && i != CB_DIR
                && (gapfnScSendMessage[MessageTable[i].iFunction] != SfnDWORD)
                && (gapfnScSendMessage[MessageTable[i].iFunction] != SfnINWPARAMCHAR)
                && (gapfnScSendMessage[MessageTable[i].iFunction] != SfnINWPARAMDBCSCHAR)
                && (gapfnScSendMessage[MessageTable[i].iFunction] != SfnSENTDDEMSG)
                && (gapfnScSendMessage[MessageTable[i].iFunction] != SfnPOWERBROADCAST)
                && (gapfnScSendMessage[MessageTable[i].iFunction] != SfnLOGONNOTIFY)
                && (gapfnScSendMessage[MessageTable[i].iFunction] != SfnINDESTROYCLIPBRD)) {
            if (!(TESTSYNCONLYMESSAGE(i,0x8000)))
                RIPMSG1(RIP_ERROR, "InitSyncOnly: is this message sync-only 0x%lX", i);
        } else {
            if (TESTSYNCONLYMESSAGE(i,0))
                RIPMSG1(RIP_VERBOSE, "InitSyncOnly: is this message not sync-only 0x%lX", i);
        }

    }
}
#endif // DBG

/***************************************************************************\
* InitWindowMsgTables
*
* This function generates a bit-array lookup table from a list of messages.
* The lookup table is used to determine whether the message needs to be
* passed over to the server for handling or whether it can be handled
* directly on the client.
*
* LATER: Some memory (a couple hundred bytes per process) could be saved
*        by putting this in the shared read-only heap.
*
*
* 27-Mar-1992 DarrinM   Created.
* 06-Dec-1993 MikeKe    Added support for all of our window procs.
\***************************************************************************/

VOID InitWindowMsgTable(
    PBYTE      *ppbyte,
    PUINT      pmax,
    CONST WORD *pw)
{
    UINT i;
    WORD msg;
    UINT cbTable;

    *pmax = 0;
    for (i = 0; (msg = pw[i]) != 0; i++) {
        if (msg > *pmax)
            *pmax = msg;
    }

    cbTable = *pmax / 8 + 1;
    *ppbyte = SharedAlloc(cbTable);

    for (i = 0; (msg = pw[i]) != 0; i++)
        (*ppbyte)[msg / 8] |= (BYTE)(1 << (msg & 7));
}

/***************************************************************************\
* InitFunctionTables
*
* Initialize the procedures and function tables.
*
*
* 25-Aug-1995 ChrisWil  Created comment block.
\***************************************************************************/

VOID InitFunctionTables(VOID)
{
    UINT i;

    TRACE_INIT(("UserInit: Initialize Function Tables\n"));

    UserAssert(sizeof(CLIENTINFO) <= sizeof(NtCurrentTeb()->Win32ClientInfo));

    /*
     * This table is used to convert from server procs to client procs.
     */
    STOCID(FNID_SCROLLBAR)              = (WNDPROC_PWND)xxxSBWndProc;
    STOCID(FNID_ICONTITLE)              = xxxDefWindowProc;
    STOCID(FNID_MENU)                   = xxxMenuWindowProc;
    STOCID(FNID_DESKTOP)                = xxxDesktopWndProc;
    STOCID(FNID_DEFWINDOWPROC)          = xxxDefWindowProc;

    /*
     * This table is used to determine the number minimum number
     * of reserved windows words required for the server proc.
     */
    CBFNID(FNID_SCROLLBAR)              = sizeof(SBWND);
    CBFNID(FNID_ICONTITLE)              = sizeof(WND);
    CBFNID(FNID_MENU)                   = sizeof(MENUWND);

    /*
     * Initialize this data structure (api function table).
     */
    for (i = 0; i < FNID_ARRAY_SIZE; i++) {
        FNID((i + FNID_START)) = xxxUnusedFunctionId;
    }
    FNID(FNID_SCROLLBAR)                = xxxWrapSBWndProc;
    FNID(FNID_ICONTITLE)                = xxxWrapDefWindowProc;
    FNID(FNID_MENU)                     = xxxWrapMenuWindowProc;
    FNID(FNID_DESKTOP)                  = xxxWrapDesktopWndProc;
    FNID(FNID_DEFWINDOWPROC)            = xxxWrapDefWindowProc;
    FNID(FNID_SENDMESSAGE)              = xxxWrapSendMessage;
    FNID(FNID_HKINLPCWPEXSTRUCT)        = fnHkINLPCWPEXSTRUCT;
    FNID(FNID_HKINLPCWPRETEXSTRUCT)     = fnHkINLPCWPRETEXSTRUCT;
    FNID(FNID_SENDMESSAGEFF)            = xxxSendMessageFF;
    FNID(FNID_SENDMESSAGEEX)            = xxxSendMessageEx;
    FNID(FNID_CALLWINDOWPROC)           = xxxWrapCallWindowProc;
    FNID(FNID_SENDMESSAGEBSM)           = xxxWrapSendMessageBSM;

#if DBG
    {
        PULONG_PTR pdw;

        /*
         * Make sure that everyone got initialized.
         */
        for (pdw=(PULONG_PTR)&STOCID(FNID_START);
                (ULONG_PTR)pdw<(ULONG_PTR)(&STOCID(FNID_WNDPROCEND)); pdw++) {
            UserAssert(*pdw);
        }

        for (pdw=(PULONG_PTR)&FNID(FNID_START);
                (ULONG_PTR)pdw<(ULONG_PTR)(&FNID(FNID_WNDPROCEND)); pdw++) {
            UserAssert(*pdw);
        }
    }
#endif

}

/***************************************************************************\
* InitMessageTables
*
* Initialize the message tables.
*
*
* 25-Aug-1995 ChrisWil      Created.
\***************************************************************************/

VOID InitMessageTables(VOID)
{
    TRACE_INIT(("UserInit: Initialize Message Tables\n"));

#define INITMSGTABLE(member, procname)                \
    InitWindowMsgTable(&(gSharedInfo.member.abMsgs),  \
                       &(gSharedInfo.member.maxMsgs), \
                       gaw ## procname);

    INITMSGTABLE(DefWindowMsgs, DefWindowMsgs);
    INITMSGTABLE(DefWindowSpecMsgs, DefWindowSpecMsgs);

    INITMSGTABLE(awmControl[FNID_DIALOG       - FNID_START], DefDlgProc);
    INITMSGTABLE(awmControl[FNID_SCROLLBAR    - FNID_START], ScrollBarWndProc);
    INITMSGTABLE(awmControl[FNID_MENU         - FNID_START], MenuWndProc);
    INITMSGTABLE(awmControl[FNID_DESKTOP      - FNID_START], DesktopWndProc);
    INITMSGTABLE(awmControl[FNID_STATIC       - FNID_START], StaticWndProc);
    INITMSGTABLE(awmControl[FNID_BUTTON       - FNID_START], ButtonWndProc);
    INITMSGTABLE(awmControl[FNID_LISTBOX      - FNID_START], ListboxWndProc);
    INITMSGTABLE(awmControl[FNID_COMBOBOX     - FNID_START], ComboWndProc);
    INITMSGTABLE(awmControl[FNID_COMBOLISTBOX - FNID_START], ListboxWndProc);
    INITMSGTABLE(awmControl[FNID_EDIT         - FNID_START], EditWndProc);
    INITMSGTABLE(awmControl[FNID_IME          - FNID_START], ImeWndProc);
}

/***************************************************************************\
* InitOLEFormats
*
* OLE performance hack.  OLE was previously having to call the server
* 15 times for clipboard formats and another 15 LPC calls for the global
* atoms.  Now we preregister them.  We also assert they are in order so
* OLE only has to query the first to know them all.  We call AddAtom
* directly instead of RegisterClipboardFormat.
*
*
* 25-Aug-1995 ChrisWil      Created.
\***************************************************************************/

BOOL InitOLEFormats(VOID)
{
    UINT idx;
    ATOM a1;
    ATOM a2;
    BOOL fSuccess = TRUE;

    TRACE_INIT(("UserInit: Initialize OLE Formats\n"));

    a1 = UserAddAtom(lpszOLEFormats[0], TRUE);

    for (idx = 1; idx < ARRAY_SIZE(lpszOLEFormats); idx++) {
        a2 = UserAddAtom(lpszOLEFormats[idx], TRUE);
        fSuccess &= !!a2;

        UserAssert(((a1 + 1) == a2) && (a1 = a2));
    }

    if (!fSuccess) {
        RIPMSG0(RIP_ERROR, "InitOLEFormats: at least one atom not registered");
    }

    return fSuccess;
}

/***************************************************************************\
* InitGlobalRIPFlags (debug only)
*
* This initializes the global RIP flags from the registry.
*
*
* 25-Aug-1995 ChrisWil      Created.
\***************************************************************************/
#if DBG
VOID
InitGlobalRIPFlags()
{

    UINT  idx;
    UINT  nCount;
    DWORD dwFlag;

    static CONST struct {
        LPWSTR lpszKey;
        DWORD  dwDef;
        DWORD  dwFlag;
    } aRIPFlags[] = {
        {L"fPromptOnError"  , 1, RIPF_PROMPTONERROR  },
        {L"fPromptOnWarning", 0, RIPF_PROMPTONWARNING},
        {L"fPromptOnVerbose", 0, RIPF_PROMPTONVERBOSE},
        {L"fPrintError"     , 1, RIPF_PRINTONERROR   },
        {L"fPrintWarning"   , 1, RIPF_PRINTONWARNING },
        {L"fPrintVerbose"   , 0, RIPF_PRINTONVERBOSE },
        {L"fPrintFileLine"  , 0, RIPF_PRINTFILELINE  },
    };

    TRACE_INIT(("UserInit: Initialize Global RIP Flags\n"));

    nCount = sizeof(aRIPFlags) / sizeof(aRIPFlags[0]);

    /*
     * Turn off the rip-on-warning bit.  This is necessary to prevent
     * the FastGetProfileDwordW() routine from breaking into the
     * debugger if an entry can't be found.  Since we provide default
     * values, there's no sense to break.
     */
    UserAssert(gpsi != NULL);

    CLEAR_FLAG(gpsi->wRIPFlags, RIPF_PROMPTONWARNING);
    CLEAR_FLAG(gpsi->wRIPFlags, RIPF_PRINTONWARNING);

    for (idx=0; idx < nCount; idx++) {

        dwFlag = FastGetProfileDwordW(NULL, PMAP_WINDOWSM,
                                      aRIPFlags[idx].lpszKey,
                                      aRIPFlags[idx].dwDef
                                      );

        SET_OR_CLEAR_FLAG(gpsi->wRIPFlags, aRIPFlags[idx].dwFlag, dwFlag);
    }

}

#else // DBG

#define InitGlobalRIPFlags()

#endif // DBG


/***************************************************************************\
* _GetTextMetricsW
* _TextOutW
*
* Server shared function thunks.
*
* History:
* 10-Nov-1993 MikeKe    Created
\***************************************************************************/

BOOL _GetTextMetricsW(
    HDC           hdc,
    LPTEXTMETRICW ptm)
{
    TMW_INTERNAL tmi;
    BOOL         fret;

    fret = GreGetTextMetricsW(hdc, &tmi);

    *ptm = tmi.tmw;

    return fret;
}

BOOL _TextOutW(
    HDC     hdc,
    int     x,
    int     y,
    LPCWSTR lp,
    UINT    cc)
{
    return GreExtTextOutW(hdc, x, y, 0, NULL, (LPWSTR)lp, cc, NULL);
}



#ifndef PAGE_SIZE
#define PAGE_SIZE 0x1000
#endif

#define ROUND_UP_TO_PAGES(SIZE) \
        (((ULONG)(SIZE) + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1))

/***************************************************************************\
* InitCreateSharedSection
*
* This creates the shared section.
*
*
* 25-Aug-1995 ChrisWil      Created comment block.
\***************************************************************************/

NTSTATUS InitCreateSharedSection(VOID)
{
    ULONG             ulHeapSize;
    ULONG             ulHandleTableSize;
    NTSTATUS          Status;
    LARGE_INTEGER     SectionSize;
    SIZE_T            ViewSize;
    PVOID             pHeapBase;

    TRACE_INIT(("UserInit: Create Shared Memory Section\n"));

    UserAssert(ghSectionShared == NULL);

    ulHeapSize        = ROUND_UP_TO_PAGES(USRINIT_SHAREDSECT_SIZE * 1024);
    ulHandleTableSize = ROUND_UP_TO_PAGES(0x10000 * sizeof(HANDLEENTRY));

    TRACE_INIT(("UserInit: Share: TableSize = %X; HeapSize = %X\n",
            ulHandleTableSize, ulHeapSize));

    SectionSize.LowPart  = ulHeapSize + ulHandleTableSize;
    SectionSize.HighPart = 0;

    Status = Win32CreateSection(&ghSectionShared,
                                SECTION_ALL_ACCESS,
                                (POBJECT_ATTRIBUTES)NULL,
                                &SectionSize,
                                PAGE_EXECUTE_READWRITE,
                                SEC_RESERVE,
                                (HANDLE)NULL,
                                NULL,
                                TAG_SECTION_SHARED);

    if (!NT_SUCCESS(Status)) {
        RIPMSG1(RIP_WARNING,
                "MmCreateSection failed in InitCreateSharedSection with Status %x",
                Status);
        return Status;
    }

    ViewSize = 0;
    gpvSharedBase = NULL;

    Status = Win32MapViewInSessionSpace(ghSectionShared, &gpvSharedBase, &ViewSize);

    if (!NT_SUCCESS(Status)) {
        RIPMSG1(RIP_WARNING, "Win32MapViewInSessionSpace failed with Status %x",
                Status);
        Win32DestroySection(ghSectionShared);
        ghSectionShared = NULL;
        return Status;
    }

    pHeapBase = ((PBYTE)gpvSharedBase + ulHandleTableSize);

    TRACE_INIT(("UserInit: Share: BaseAddr = %X; Heap = %X, ViewSize = %X\n",
            gpvSharedBase, pHeapBase, ViewSize));

    /*
     * Create shared heap.
     */
    if ((gpvSharedAlloc = UserCreateHeap(
            ghSectionShared,
            ulHandleTableSize,
            pHeapBase,
            ulHeapSize,
            UserCommitSharedMemory)) == NULL) {

        RIPERR0(ERROR_NOT_ENOUGH_MEMORY, RIP_WARNING, "Can't create shared memory heap.");

        Win32UnmapViewInSessionSpace(gpvSharedBase);

        Win32DestroySection(ghSectionShared);
        gpvSharedAlloc = NULL;
        gpvSharedBase = NULL;
        ghSectionShared = NULL;

        return STATUS_NO_MEMORY;
    }

    UserAssert(Win32HeapGetHandle(gpvSharedAlloc) == pHeapBase);

    return STATUS_SUCCESS;
}

/**************************************************************************\
* InitCreateUserCrit
*
* Create and initialize the user critical sections needed throughout the
* system.
*
* 23-Jan-1996 ChrisWil      Created.
\**************************************************************************/

BOOL InitCreateUserCrit(VOID)
{
    TRACE_INIT(("Win32UserInit: InitCreateUserCrit()\n"));

    /*
     * Initialize a critical section structure that will be used to protect
     * all of the User Server's critical sections (except a few special
     * cases like the RIT -- see below).
     */
    gpresUser = ExAllocatePoolWithTag(NonPagedPoolMustSucceed,
                                      sizeof(ERESOURCE),
                                      TAG_ERESOURCE);
    if (!gpresUser) {
        goto InitCreateUserCritExit;
    }
    if (!NT_SUCCESS(ExInitializeResourceLite(gpresUser))) {
        goto InitCreateUserCritExit;
    }

    /*
     * Initialize a critical section to be used in [Un]QueueMouseEvent
     * to protect the queue of mouse input events that the desktop thread
     * uses to pass input on to the RIT, after having moved the cursor
     * without obtaining gpresUser itself.
     */
    gpresMouseEventQueue = ExAllocatePoolWithTag(NonPagedPoolMustSucceed,
                                                 sizeof(ERESOURCE),
                                                 TAG_ERESOURCE);
    if (!gpresMouseEventQueue) {
        goto InitCreateUserCritExit;
    }
    if (!NT_SUCCESS(ExInitializeResourceLite(gpresMouseEventQueue))) {
        goto InitCreateUserCritExit;
    }

    /*
     * Initialize a critical section to protect the list of DEVICEINFO structs
     * kept under gpDeviceInfoList.  This is used by the RIT when reading kbd
     * input, the desktop thread when reading mouse input, and the PnP callback
     * routines DeviceClassNotify() and DeviceNotify() when devices come and go.
     */
    gpresDeviceInfoList = ExAllocatePoolWithTag(NonPagedPoolMustSucceed,
                                            sizeof(ERESOURCE),
                                            TAG_ERESOURCE);
    if (!gpresDeviceInfoList) {
        goto InitCreateUserCritExit;
    }
    if (!NT_SUCCESS(ExInitializeResourceLite(gpresDeviceInfoList))) {
        goto InitCreateUserCritExit;
    }

    /*
     * Create the handle flag mutex. We'll need this once we start creating
     * windowstations and desktops.
     */
    gpHandleFlagsMutex = ExAllocatePoolWithTag(NonPagedPoolMustSucceed,
                                               sizeof(FAST_MUTEX),
                                               TAG_SYSTEM);
    if (gpHandleFlagsMutex == NULL) {
        goto InitCreateUserCritExit;
    }
    ExInitializeFastMutex(gpHandleFlagsMutex);

    TRACE_INIT(("Win32UserInit: gpHandleFlagsMutex = 0x%p\n", gpHandleFlagsMutex));
    TRACE_INIT(("Win32UserInit: gpresDeviceInfoList = 0x%X\n", gpresDeviceInfoList));
    TRACE_INIT(("Win32UserInit: gpresMouseEventQueue = 0x%X\n", gpresMouseEventQueue));
    TRACE_INIT(("Win32UserInit: gpresUser  = 0x%X\n", gpresUser));

    TRACE_INIT(("Win32UserInit: exit InitCreateUserCrit()\n"));
    return TRUE;

InitCreateUserCritExit:
    RIPERR0(ERROR_NOT_ENOUGH_MEMORY, RIP_ERROR,
            "Win32UserInit: InitCreateUserCrit failed");

    if (gpresUser) {
        ExFreePool(gpresUser);
    }
    if (gpresMouseEventQueue) {
        ExFreePool(gpresMouseEventQueue);
    }
    if (gpresDeviceInfoList) {
        ExFreePool(gpresDeviceInfoList);
    }
    return FALSE;
}

/**************************************************************************\
* InitCreateObjectDirectory
*
* Create and initialize the user critical sections needed throughout the
* system.
*
* 23-Jan-1996 ChrisWil      Created.
\**************************************************************************/
NTSTATUS InitCreateObjectDirectory(VOID)
{
    HANDLE            hDir;
    NTSTATUS          Status;
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING    UnicodeString;
    ULONG             attributes = OBJ_CASE_INSENSITIVE | OBJ_PERMANENT;

    TRACE_INIT(("UserInit: Create User Object-Directory\n"));

    RtlInitUnicodeString(&UnicodeString, szWindowStationDirectory);

    if (gbRemoteSession) {
       /*
        * Remote sessions don't use this flag
        */
       attributes &= ~OBJ_PERMANENT;
    }

    InitializeObjectAttributes(&ObjectAttributes,
                               &UnicodeString,
                               attributes,
                               NULL,
                               gpsdInitWinSta);

    Status = ZwCreateDirectoryObject(&hDir,
                                     DIRECTORY_CREATE_OBJECT,
                                     &ObjectAttributes);

    UserFreePool(gpsdInitWinSta);

    /*
     * Do not close this handle for remote session because
     * if we do close it then the directory will go away and
     * we don't want that to happen. When CSRSS will go away
     * this handle will be freed also.
     */
    if (!gbRemoteSession)
        ZwClose(hDir);

    gpsdInitWinSta = NULL;

    return Status;
}

/**************************************************************************\
* InitCreateUserSubsystem
*
* Create and initialize the user subsystem stuff.
* system.
*
* 23-Jan-1996 ChrisWil      Created.
\**************************************************************************/
BOOL
InitCreateUserSubsystem()
{
    LPWSTR         lpszSubSystem;
    LPWSTR         lpszT;
    UNICODE_STRING strSize;

    TRACE_INIT(("UserInit: Create User SubSystem\n"));

    /*
     * Initialize the subsystem section.  This identifies the default
     * user-heap size.
     */
    lpszSubSystem = UserAllocPoolWithQuota(USRINIT_SHAREDSECT_BUFF_SIZE * sizeof(WCHAR),
                                           TAG_SYSTEM);

    if (lpszSubSystem == NULL) {
        return FALSE;
    }

    if (FastGetProfileStringW(NULL, PMAP_SUBSYSTEMS,
                              L"Windows",
                              L"SharedSection=,3072",
                              lpszSubSystem,
                              USRINIT_SHAREDSECT_READ_SIZE
                              ) == 0) {
        RIPMSG0(RIP_WARNING,
                "UserInit: Windows subsystem definition not found");
        UserFreePool(lpszSubSystem);
        return FALSE;
    }

    /*
     * Locate the SharedSection portion of the definition and extract
     * the second value.
     */
    gdwDesktopSectionSize = USRINIT_WINDOWSECT_SIZE;
    gdwNOIOSectionSize    = USRINIT_NOIOSECT_SIZE;

    if (lpszT = wcsstr(lpszSubSystem, L"SharedSection")) {

        *(lpszT + 32) = UNICODE_NULL;

        if (lpszT = wcschr(lpszT, L',')) {

            RtlInitUnicodeString(&strSize, ++lpszT);
            RtlUnicodeStringToInteger(&strSize, 0, &gdwDesktopSectionSize);

            /*
             * Assert this logic doesn't need to change.
             */
            UserAssert(gdwDesktopSectionSize >= USRINIT_WINDOWSECT_SIZE);

            gdwDesktopSectionSize = max(USRINIT_WINDOWSECT_SIZE, gdwDesktopSectionSize);
            gdwNOIOSectionSize    = gdwDesktopSectionSize;

            /*
             * Now see if the optional non-interactive desktop
             * heap size was specified.
             */
            if (lpszT = wcschr(lpszT, L',')) {

                RtlInitUnicodeString(&strSize, ++lpszT);
                RtlUnicodeStringToInteger(&strSize, 0, &gdwNOIOSectionSize);

                UserAssert(gdwNOIOSectionSize >= USRINIT_NOIOSECT_SIZE);
                gdwNOIOSectionSize = max(USRINIT_NOIOSECT_SIZE, gdwNOIOSectionSize);
            }
        }
    }

    UserFreePool(lpszSubSystem);

    return TRUE;
}

extern UNICODE_STRING *gpastrSetupExe;     // These are used in
extern int giSetupExe;                     // SetAppImeCompatFlags in
                                           // queue.c

WCHAR* glpSetupPrograms;

/******************************************************
*
* Create and initialize the arrary of setup app names.
* We inherited this hack From Chicago.  See queue.c for
* more details.
*******************************************************/

BOOL CreateSetupNameArray() {
    DWORD  dwProgNames;
    int    iSetupProgramCount = 0;
    WCHAR* lpTemp;
    int    ic, icnt, icMax;

    dwProgNames = FastGetProfileValue(NULL, PMAP_SETUPPROGRAMNAMES,
                        L"SetupProgramNames",NULL,NULL, 0);

    /*
     * This key is a multi-string, so is best to read as a value.
     * First, get the length and create the buffer to hold all of
     * the strings.
     */
    if (dwProgNames == 0) {
        return FALSE;
    }

    glpSetupPrograms = UserAllocPoolWithQuota(dwProgNames,
                                       TAG_SYSTEM);

    if (glpSetupPrograms == NULL) {
        RIPMSG0(RIP_WARNING, "CreateSetupNameArray: Memory allocation failure");
        return FALSE;
    }

    FastGetProfileValue(NULL,
                        PMAP_SETUPPROGRAMNAMES,
                        L"SetupProgramNames",
                        NULL,
                        (PBYTE)glpSetupPrograms,
                        dwProgNames);

    lpTemp = glpSetupPrograms;
    icMax = dwProgNames/2;
    ic = 0; icnt=0;
    /*
     * Now count the strings.
     */
    while (ic < icMax) {
        if (*(lpTemp+ic) == 0) {
            ic++;
            continue;
        }
        ic += wcslen(lpTemp+ic)+1;
        icnt++;
    }

    /*
     * gpastrSetupExe is a pointer to an array of UNICODE_STRING structures.
     * Each structure is the name of one setup program.
     */
    giSetupExe = icnt;
    gpastrSetupExe = UserAllocPoolWithQuota(giSetupExe * sizeof(UNICODE_STRING),
                                       TAG_SYSTEM);

    if (gpastrSetupExe == NULL) {
        RIPMSG0(RIP_WARNING, "CreateSetupNameArray: Memory allocation failure");
        giSetupExe = 0;
        UserFreePool(glpSetupPrograms);
        glpSetupPrograms = NULL;
        return FALSE;
    }

    ic = 0; icnt=0;
    while (ic < icMax) {
        if (*(lpTemp+ic) == 0) {
            ic++;
            continue;
        }
        gpastrSetupExe[icnt].Buffer = lpTemp+ic;
        gpastrSetupExe[icnt].Length = sizeof(WCHAR)*wcslen(lpTemp+ic);
        gpastrSetupExe[icnt].MaximumLength = gpastrSetupExe[icnt].Length + sizeof(WCHAR);
        ic += wcslen(lpTemp+ic)+1;
        icnt++;

    }

    return TRUE;
}

#define CALC_DELTA(element)                   \
        (PVOID)((PBYTE)pClientBase +          \
        ((PBYTE)gSharedInfo.element -         \
        (PBYTE)gpvSharedBase))

/***************************************************************************\
* InitMapSharedSection
*
* This maps the shared section.
*
*
* 25-Aug-1995 ChrisWil      Created comment block.
\***************************************************************************/

NTSTATUS InitMapSharedSection(
    PEPROCESS    Process,
    PUSERCONNECT pUserConnect)
{
    int           i;
    PVOID         pClientBase = NULL;
    ULONG_PTR      ulSharedDelta;

    TRACE_INIT(("UserInit: Map Shared Memory Section\n"));

    UserAssert(ghSectionShared != NULL);

    ValidateProcessSessionId(Process);

    /*
     * Check to see if we haven't already mapped the section
     * This might happen for multiple LoadLibrary()/FreeLibrary calls
     * in one process.  MCostea #56946
     */
    if (Process->Win32Process == NULL ||
        ((PPROCESSINFO)Process->Win32Process)->pClientBase == NULL) {

        SIZE_T        ViewSize;
        LARGE_INTEGER liOffset;
        NTSTATUS Status;

        ViewSize = 0;
        liOffset.QuadPart = 0;

        Status = MmMapViewOfSection(ghSectionShared,
                                Process,
                                &pClientBase,
                                0,
                                0,
                                &liOffset,
                                &ViewSize,
                                ViewUnmap,
                                SEC_NO_CHANGE,
                                PAGE_EXECUTE_READ);
        if (NT_SUCCESS(Status)) {
            TRACE_INIT(("UserInit: Map: Client SharedInfo Base = %x\n", pClientBase));

            UserAssert(gpvSharedBase > pClientBase);
            if (Process->Win32Process != NULL) {
                ((PPROCESSINFO)Process->Win32Process)->pClientBase = pClientBase;
            }
        } else {
            return Status;
        }

    } else {
        pClientBase = ((PPROCESSINFO)Process->Win32Process)->pClientBase;
    }
    ulSharedDelta = (PBYTE)gpvSharedBase - (PBYTE)pClientBase;
    pUserConnect->siClient.ulSharedDelta = ulSharedDelta;

    pUserConnect->siClient.psi          = CALC_DELTA(psi);
    pUserConnect->siClient.aheList      = CALC_DELTA(aheList);
    pUserConnect->siClient.pDispInfo    = CALC_DELTA(pDispInfo);


    pUserConnect->siClient.DefWindowMsgs.maxMsgs     = gSharedInfo.DefWindowMsgs.maxMsgs;
    pUserConnect->siClient.DefWindowMsgs.abMsgs      = CALC_DELTA(DefWindowMsgs.abMsgs);
    pUserConnect->siClient.DefWindowSpecMsgs.maxMsgs = gSharedInfo.DefWindowSpecMsgs.maxMsgs;
    pUserConnect->siClient.DefWindowSpecMsgs.abMsgs  = CALC_DELTA(DefWindowSpecMsgs.abMsgs);

    for (i = 0; i < (FNID_END - FNID_START + 1); ++i) {

        pUserConnect->siClient.awmControl[i].maxMsgs = gSharedInfo.awmControl[i].maxMsgs;

        if (gSharedInfo.awmControl[i].abMsgs)
            pUserConnect->siClient.awmControl[i].abMsgs = CALC_DELTA(awmControl[i].abMsgs);
        else
            pUserConnect->siClient.awmControl[i].abMsgs = NULL;
    }
    return STATUS_SUCCESS;
}
/**************************************************************************\
* InitLoadResources
*
*
* 25-Aug-1995 ChrisWil      Created.
\**************************************************************************/

VOID InitLoadResources()
{
    PRECT   prc;

    DISPLAYRESOURCE dr = {
        17,     // Height of vertical thumb
        17,     // Width of horizontal thumb
        2,      // Icon horiz compression factor
        2,      // Icon vert compression factor
        1,      // Cursor horz compression factor
        1,      // Cursor vert compression factor
        0,      // Kanji window height
        1,      // cxBorder (thickness of vertical lines)
        1       // cyBorder (thickness of horizontal lines)
    };


    TRACE_INIT(("UserInit: Load Display Resources\n"));

    if (dr.xCompressIcon > 10) {

        /*
         * If so, the actual dimensions of icons and cursors are
         * kept in OEMBIN.
         */
        SYSMET(CXICON)   = dr.xCompressIcon;
        SYSMET(CYICON)   = dr.yCompressIcon;
        SYSMET(CXCURSOR) = dr.xCompressCursor;
        SYSMET(CYCURSOR) = dr.yCompressCursor;

    } else {

        /*
         * Else, only the ratio of (64/icon dimensions) is kept there.
         */
        SYSMET(CXICON)   = (64 / dr.xCompressIcon);
        SYSMET(CYICON)   = (64 / dr.yCompressIcon);
        SYSMET(CXCURSOR) = (32 / dr.xCompressCursor);
        SYSMET(CYCURSOR) = (32 / dr.yCompressCursor);
    }

    SYSMET(CXSMICON) = SYSMET(CXICON) / 2;
    SYSMET(CYSMICON) = SYSMET(CYICON) / 2;

    SYSMET(CYKANJIWINDOW) = dr.yKanji;

    /*
     * Get border thicknesses.
     */
    SYSMET(CXBORDER) = dr.cxBorder;
    SYSMET(CYBORDER) = dr.cyBorder;

    /*
     * Edge is two borders.
     */
    SYSMET(CXEDGE) = 2 * SYSMET(CXBORDER);
    SYSMET(CYEDGE) = 2 * SYSMET(CYBORDER);

    /*
     * Fixed frame is outer edge + border.
     */
    SYSMET(CXDLGFRAME) = SYSMET(CXEDGE) + SYSMET(CXBORDER);
    SYSMET(CYDLGFRAME) = SYSMET(CYEDGE) + SYSMET(CYBORDER);

    if (gbRemoteSession) {
        return;
    }

    prc = &GetPrimaryMonitor()->rcMonitor;
    SYSMET(CXFULLSCREEN) = prc->right;
    SYSMET(CYFULLSCREEN) = prc->bottom - SYSMET(CYCAPTION);

    /*
     * Set the initial cursor position to the center of the primary screen.
     */
    gpsi->ptCursor.x = prc->right / 2;
    gpsi->ptCursor.y = prc->bottom / 2;
}

/***************************************************************************\
* GetCharDimensions
*
* This function loads the Textmetrics of the font currently selected into
* the hDC and returns the Average char width of the font; Pl Note that the
* AveCharWidth value returned by the Text metrics call is wrong for
* proportional fonts.  So, we compute them On return, lpTextMetrics contains
* the text metrics of the currently selected font.
*
* History:
* 10-Nov-1993 mikeke   Created
\***************************************************************************/
int GetCharDimensions(
        HDC          hdc,
        TEXTMETRIC*  lptm,
        LPINT        lpcy
        )
{
    TEXTMETRIC tm;

    /*
     * Didn't find it in cache, store the font metrics info.
     */
    if (!_GetTextMetricsW(hdc, &tm)) {
        RIPMSG1(RIP_WARNING, "GetCharDimensions: _GetTextMetricsW failed. hdc %#lx", hdc);
        tm = gpsi->tmSysFont; // damage control

        if (tm.tmAveCharWidth == 0) {
            RIPMSG0(RIP_WARNING, "GetCharDimensions: _GetTextMetricsW first time failure");
            tm.tmAveCharWidth = 8;
        }
    }
    if (lptm != NULL)
        *lptm = tm;
    if (lpcy != NULL)
        *lpcy = tm.tmHeight;

    /*
     * If variable_width font
     */
    if (tm.tmPitchAndFamily & TMPF_FIXED_PITCH) {
        SIZE size;
        static CONST WCHAR wszAvgChars[] =
                L"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";

        /*
         * Change from tmAveCharWidth.  We will calculate a true average
         * as opposed to the one returned by tmAveCharWidth.  This works
         * better when dealing with proportional spaced fonts.
         */
        if (GreGetTextExtentW(
                hdc, (LPWSTR)wszAvgChars,
                (sizeof(wszAvgChars) / sizeof(WCHAR)) - 1,
                &size, GGTE_WIN3_EXTENT)) {

            UserAssert((((size.cx / 26) + 1) / 2) > 0);
            return ((size.cx / 26) + 1) / 2;    // round up
        } else {
            RIPMSG1(RIP_WARNING, "GetCharDimensions: GreGetTextExtentW failed. hdc %#lx", hdc);
        }
    }

    UserAssert(tm.tmAveCharWidth > 0);

    return tm.tmAveCharWidth;
}


/**************************************************************************\
* InitVideo
*
* Create pmdev.
*
* 03-March-1998 CLupu  Moved from UserInitialize code
\**************************************************************************/

PMDEV InitVideo(
    BOOL bReenumerationNeeded)
{
    PMDEV pmdev;
    LONG  ChangeStatus;

    /*
     * BUGBUG !!! Need to get a status return from this call.
     */
    DrvInitConsole(bReenumerationNeeded);

    /*
     * BASEVIDEO may be on or off, whether we are in setup or not.
     */

    ChangeStatus = DrvChangeDisplaySettings(NULL,
                                            NULL,
                                            NULL,
                                            (PVOID) (GW_DESKTOP_ID),
                                            KernelMode,
                                            FALSE,
                                            TRUE,
                                            NULL,
                                            &pmdev,
                                            GRE_DEFAULT,
                                            TRUE);

    if (ChangeStatus != GRE_DISP_CHANGE_SUCCESSFUL) {

        /*
         * If we fail, try BASEVIDEO temporarily
         */

        DrvSetBaseVideo(TRUE);

        ChangeStatus = DrvChangeDisplaySettings(NULL,
                                                NULL,
                                                NULL,
                                                (PVOID) (GW_DESKTOP_ID),
                                                KernelMode,
                                                FALSE,
                                                TRUE,
                                                NULL,
                                                &pmdev,
                                                GRE_DEFAULT,
                                                TRUE);

        DrvSetBaseVideo(FALSE);

        /*
         * Give it one last try, not in basevideo, to handle TGA
         * (non-vgacompatible) during GUI-mode setup (BASEVIDEO is on by
         * default)
         */

        if (ChangeStatus != GRE_DISP_CHANGE_SUCCESSFUL) {

            ChangeStatus = DrvChangeDisplaySettings(NULL,
                                                    NULL,
                                                    NULL,
                                                    (PVOID) (GW_DESKTOP_ID),
                                                    KernelMode,
                                                    FALSE,
                                                    TRUE,
                                                    NULL,
                                                    &pmdev,
                                                    GRE_DEFAULT,
                                                    TRUE);

        }
    }

    if (ChangeStatus != GRE_DISP_CHANGE_SUCCESSFUL) {
        RIPMSG0(RIP_WARNING, "InitVideo: No working display driver found");
        return NULL;
    }

    gpDispInfo->hDev  = pmdev->hdevParent;
    gpDispInfo->pmdev = pmdev;

    GreUpdateSharedDevCaps(gpDispInfo->hDev);

    if (!InitUserScreen()) {
        RIPMSG0(RIP_WARNING, "InitUserScreen failed");
        return NULL;
    }

    HYDRA_HINT(HH_INITVIDEO);

    return pmdev;
}

void DrvDriverFailure(void)
{
    KeBugCheckEx(VIDEO_DRIVER_INIT_FAILURE,
                 0,
                 0,
                 0,
                 USERCURRENTVERSION);
}


/**************************************************************************\
* UserInitialize
*
* Worker routine for user initialization.
*
* 25-Aug-1995 ChrisWil  Created comment block/Multiple desktop support.
* 15-Dec-1995 BradG     Modified to return MediaChangeEvent Handle.
\**************************************************************************/

NTSTATUS UserInitialize(VOID)
{
    NTSTATUS Status;

    /*
     * Allow a trace of all the init stuff going on related to display drivers.
     * Usefull to debug boot time problems related to graphics.
     */
    if (**((PULONG *)&NtGlobalFlag) & FLG_SHOW_LDR_SNAPS)
        TraceInitialization = 1;

    TRACE_INIT(("Entering UserInitialize\n"));

    EnterCrit();

    HYDRA_HINT(HH_USERINITIALIZE);

    if (ISTS()) {

        if (gbRemoteSession) {
            swprintf(szWindowStationDirectory, L"%ws\\%ld%ws",
                     SESSION_ROOT, gSessionId, WINSTA_DIR);
        } else {
            wcscpy(szWindowStationDirectory, WINSTA_DIR);
        }
    } else {
        wcscpy(szWindowStationDirectory, WINSTA_DIR);
    }

    /*
     * Create WindowStation object directory.
     */
    Status = InitCreateObjectDirectory();

    if (!NT_SUCCESS(Status)) {
        RIPMSG1(RIP_WARNING, "InitCreateObjectDirectory failed with Status %x",
                Status);

        goto Exit;
    }

    /*
     * WinStations get init'ed on the first connect
     */
    if (gbRemoteSession) {

        /*
         * Create the event for the diconnect desktop creation
         */
        gpEventDiconnectDesktop = CreateKernelEvent(SynchronizationEvent, FALSE);

        if (gpEventDiconnectDesktop == NULL) {
            RIPMSG0(RIP_WARNING, "Failed to create gpEventDiconnectDesktop");
            Status = STATUS_NO_MEMORY;
            goto Exit;
        }

        goto SkipRemote;
    }

    if (InitVideo(TRUE) == NULL) {
        DrvDriverFailure();
    }

    /*
     * Do this here so power callouts
     * have the pmdev in gpDispInfo set
     */
    gbVideoInitialized = TRUE;

SkipRemote:

    gbUserInitialized = TRUE;

    /*
     * Now that the system is initialized, allocate
     * a pti for this thread.
     */
    Status = xxxCreateThreadInfo(PsGetCurrentThread(), FALSE);

    if (!NT_SUCCESS(Status)) {
        RIPMSG1(RIP_WARNING, "xxxCreateThreadInfo failed during UserInitialize with Status",
                Status);
        goto Exit;
    }

    /*
     * Initialize Global RIP flags (debug only).
     */
    InitGlobalRIPFlags();

    /*
     * WinStations get init'ed on the first connect
     */
    if (!gbRemoteSession)
        UserVerify(LW_BrushInit());

    InitLoadResources();

Exit:
    LeaveCrit();

    TRACE_INIT(("Leaving UserInitialize\n"));

    return Status;
}

/**************************************************************************\
* IsDBCSEnabledSystem
*
* check if the system is configured as FE Enabled
*
* 07-Feb-1997 HiroYama  Created
\**************************************************************************/
BOOL IsDBCSEnabledSystem()
{
    extern BOOLEAN* NlsMbCodePageTag;
    return !!*NlsMbCodePageTag;
}


BOOL IsIMMEnabledSystem()
{
    // if the entire system is DBCS enabled, IMM/IME should be activated anyway
    if (IsDBCSEnabledSystem())
        return TRUE;

    return FastGetProfileDwordW(NULL, PMAP_IMM, TEXT("LoadIMM"), 0);
}

// Get ACP and check if the system is configured as ME Enabled
BOOL IsMidEastEnabledSystem()
{
    extern __declspec(dllimport) USHORT NlsAnsiCodePage;
    //1255 Hebrew and 1256 Arabic
    if ((NlsAnsiCodePage == 1255) || (NlsAnsiCodePage == 1256)) {
        return TRUE;
    }
    return FALSE;
}

/***************************************************************************\
* SetupClassAtoms
*
* 10/01/1998   clupu          moved from Win32UserInitialize
\***************************************************************************/

BOOL SetupClassAtoms(
    VOID)
{
    BOOL fSuccess = TRUE;
    int  ind;

    /*
     * Set up class atoms
     */
    /*
     * HACK: Controls are registered on the client side so we can't
     * fill in their atomSysClass entry the same way we do for the other
     * classes.
     */
    for (ind = ICLS_BUTTON; ind < ICLS_CTL_MAX; ind++) {
        gpsi->atomSysClass[ind] = UserAddAtom(lpszControls[ind], TRUE);
        fSuccess &= !!gpsi->atomSysClass[ind];
    }

    gpsi->atomSysClass[ICLS_DIALOG]    = PTR_TO_ID(DIALOGCLASS);
    gpsi->atomSysClass[ICLS_ICONTITLE] = PTR_TO_ID(ICONTITLECLASS);
    gpsi->atomSysClass[ICLS_TOOLTIP]   = PTR_TO_ID(TOOLTIPCLASS);
    gpsi->atomSysClass[ICLS_DESKTOP]   = PTR_TO_ID(DESKTOPCLASS);
    gpsi->atomSysClass[ICLS_SWITCH]    = PTR_TO_ID(SWITCHWNDCLASS);
    gpsi->atomSysClass[ICLS_MENU]      = PTR_TO_ID(MENUCLASS);

    gpsi->atomContextHelpIdProp = UserAddAtom(szCONTEXTHELPIDPROP, TRUE);
    fSuccess &= !!gpsi->atomContextHelpIdProp;

    gpsi->atomIconSmProp        = UserAddAtom(szICONSM_PROP_NAME, TRUE);
    fSuccess &= !!gpsi->atomIconSmProp;

    gpsi->atomIconProp          = UserAddAtom(szICON_PROP_NAME, TRUE);
    fSuccess &= !!gpsi->atomIconProp;

    gpsi->uiShellMsg            = UserAddAtom(szSHELLHOOK, TRUE);
    fSuccess &= !!gpsi->uiShellMsg;

    /*
     * Initialize the integer atoms for our magic window properties
     */
    atomCheckpointProp = UserAddAtom(szCHECKPOINT_PROP_NAME, TRUE);
    fSuccess &= !!atomCheckpointProp;

    atomDDETrack = UserAddAtom(szDDETRACK_PROP_NAME, TRUE);
    fSuccess &= !!atomDDETrack;

    atomQOS = UserAddAtom(szQOS_PROP_NAME, TRUE);
    fSuccess &= !!atomQOS;

    atomDDEImp = UserAddAtom(szDDEIMP_PROP_NAME, TRUE);
    fSuccess &= !!atomDDEImp;

    atomWndObj = UserAddAtom(szWNDOBJ_PROP_NAME, TRUE);
    fSuccess &= !!atomWndObj;

    atomImeLevel = UserAddAtom(szIMELEVEL_PROP_NAME, TRUE);
    fSuccess &= !!atomImeLevel;

    atomLayer = UserAddAtom(szLAYER_PROP_NAME, TRUE);
    fSuccess &= !!atomLayer;

    guiActivateShellWindow = UserAddAtom(szACTIVATESHELLWINDOW, TRUE);
    fSuccess &= !!guiActivateShellWindow;

    guiOtherWindowCreated = UserAddAtom(szOTHERWINDOWCREATED, TRUE);
    fSuccess &= !!guiOtherWindowCreated;

    guiOtherWindowDestroyed = UserAddAtom(szOTHERWINDOWDESTROYED, TRUE);
    fSuccess &= !!guiOtherWindowDestroyed;

    gatomMessage = UserAddAtom(szMESSAGE, TRUE);
    fSuccess &= !!gatomMessage;

#ifdef HUNGAPP_GHOSTING
    gatomGhost = UserAddAtom(szGHOST, TRUE);
    fSuccess &= !!gatomGhost;
#endif // HUNGAPP_GHOSTING

    gaOleMainThreadWndClass = UserAddAtom(szOLEMAINTHREADWNDCLASS, TRUE);
    fSuccess &= !!gaOleMainThreadWndClass;

    gaFlashWState = UserAddAtom(szFLASHWSTATE, TRUE);
    fSuccess &= !!gaFlashWState;

    gatomLastPinned = gaOleMainThreadWndClass;

    return fSuccess;
}


/**************************************************************************\
* Win32UserInitialize
*
* Worker routine for user initialization called from Win32k.sys DriverEntry()
*
\**************************************************************************/

NTSTATUS Win32UserInitialize(VOID)
{
    NTSTATUS                 Status;
    POBJECT_TYPE_INITIALIZER pTypeInfo;
    LONG                     lTemp;

    TRACE_INIT(("Entering Win32UserInitialize\n"));

    /*
     * Create the shared section.
     */
    Status = InitCreateSharedSection();
    if (!NT_SUCCESS(Status)) {
        RIPMSG0(RIP_WARNING, "InitCreateSharedSection failed");
        return Status;
    }

    EnterCrit();

    /*
     * Initialize security stuff.
     */
    if (!InitSecurity()) {
        RIPMSG0(RIP_WARNING, "InitSecurity failed");
        goto ExitWin32UserInitialize;
    }

    /*
     * Fill in windowstation and desktop object types
     */
    pTypeInfo = &(*ExWindowStationObjectType)->TypeInfo;
    pTypeInfo->DefaultNonPagedPoolCharge = sizeof(WINDOWSTATION) + sizeof(KEVENT);
    pTypeInfo->DefaultPagedPoolCharge    = 0;
    pTypeInfo->MaintainHandleCount       = TRUE;
    pTypeInfo->CloseProcedure            = DestroyWindowStation;
    pTypeInfo->DeleteProcedure           = FreeWindowStation;
    pTypeInfo->ParseProcedure            = ParseWindowStation;
    pTypeInfo->OkayToCloseProcedure      = OkayToCloseWindowStation;
    pTypeInfo->ValidAccessMask           = WinStaMapping.GenericAll;
    pTypeInfo->GenericMapping            = WinStaMapping;

    pTypeInfo = &(*ExDesktopObjectType)->TypeInfo;
    pTypeInfo->DefaultNonPagedPoolCharge = sizeof(DESKTOP);
    pTypeInfo->DefaultPagedPoolCharge    = 0;
    pTypeInfo->MaintainHandleCount       = TRUE;
    pTypeInfo->CloseProcedure            = UnmapDesktop;
    pTypeInfo->OpenProcedure             = MapDesktop;
    pTypeInfo->DeleteProcedure           = FreeDesktop;
    pTypeInfo->OkayToCloseProcedure      = OkayToCloseDesktop;
    pTypeInfo->ValidAccessMask           = DesktopMapping.GenericAll;
    pTypeInfo->GenericMapping            = DesktopMapping;

    /*
     * Get this process so we can use the profiles.
     */
    gpepInit = PsGetCurrentProcess();

    Status  = InitQEntryLookaside();
    Status |= InitSMSLookaside();
    Status |= UserRtlCreateAtomTable(USRINIT_ATOMBUCKET_SIZE);

    if (!NT_SUCCESS(Status)) {
        RIPMSG0(RIP_WARNING, "Initialization failure");
        goto ExitWin32UserInitialize;
    }

    atomUSER32 = UserAddAtom(szUSER32, TRUE);

    gatomFirstPinned = atomUSER32;

    if (gatomFirstPinned == 0) {
        RIPMSG0(RIP_WARNING, "Could not create atomUSER32");
        goto ExitWin32UserInitialize;
    }

    /*
     * Initialize the user subsystem information.
     */
    if (!InitCreateUserSubsystem()) {
        RIPMSG0(RIP_WARNING, "InitCreateUserSubsystem failed");
        goto ExitWin32UserInitialize;
    }

    /*
     * Don't bail out if CreateSetupNameArray fails
     * MCostea #326652
     */
    CreateSetupNameArray();

    /*
     * Allocated shared SERVERINFO structure.
     */
    if ((gpsi = (PSERVERINFO)SharedAlloc(sizeof(SERVERINFO))) == NULL) {
        RIPMSG0(RIP_WARNING, "Could not allocate SERVERINFO");
        goto ExitWin32UserInitialize;
    }

    /*
     * Set the default rip-flags to rip on just about
     * everything.  We'll truly set this in the InitGlobalRIPFlags()
     * routine.  These are needed so that we can do appropriate ripping
     * during the rest of the init-calls.
     */

    SET_FLAG(gpsi->wRIPFlags, RIPF_DEFAULT);

    /*
     * Make sure we will not get a division by zero if the initialization
     * will not complete correctly. Set these to their normal values.
     */
    gpsi->cxMsgFontChar = 6;
    gpsi->cyMsgFontChar = 13;
    gpsi->cxSysFontChar = 8;
    gpsi->cySysFontChar = 16;

    /*
     * Initialize the DISPLAYINFO structure.
     */
    gpDispInfo = SharedAlloc(sizeof(*gpDispInfo));
    if (!gpDispInfo) {
        RIPMSG0(RIP_WARNING, "Could not allocate gpDispInfo");
        goto ExitWin32UserInitialize;
    }

    InitDbgTags();

    SET_OR_CLEAR_SRVIF(SRVIF_DBCS, IsDBCSEnabledSystem());
    SET_OR_CLEAR_SRVIF(SRVIF_IME, IsIMMEnabledSystem());

    SET_OR_CLEAR_SRVIF(SRVIF_MIDEAST, IsMidEastEnabledSystem());

#if DBG
    SET_SRVIF(SRVIF_CHECKED);

    RIPMSG3(RIP_WARNING, "*** win32k: DBCS:[%d] IME:[%d] MiddleEast:[%d]",
            IS_DBCS_ENABLED(),
            IS_IME_ENABLED(),
            IS_MIDEAST_ENABLED());
#endif

    gpsi->dwDefaultHeapSize = gdwDesktopSectionSize * 1024;

    /*
     * Initialize procedures and message tables.
     * Initialize the class structures for Get/SetClassWord/Long.
     * Initialize message-box strings.
     * Initialize OLE-Formats (performance-hack).
     */
    InitFunctionTables();
    InitMessageTables();
#if DBG
    VerifySyncOnlyMessages();
#endif
    if (!InitOLEFormats()) {
        RIPMSG0(RIP_WARNING, "InitOLEFormats failed");
        goto ExitWin32UserInitialize;
    }

    /*
     * Set up class atoms
     */
    if (!SetupClassAtoms()) {
        RIPMSG0(RIP_WARNING, "SetupClassAtoms failed to register atoms");
        goto ExitWin32UserInitialize;
    }

    LW_LoadSomeStrings();

    /*
     * Initialize the handle manager.
     */
    if (!HMInitHandleTable(gpvSharedBase)) {
        RIPMSG0(RIP_WARNING, "HMInitHandleTable failed");
        goto ExitWin32UserInitialize;
    }

    /*
     * Setup shared info block.
     */
    gSharedInfo.psi = gpsi;
    gSharedInfo.pDispInfo = gpDispInfo;

    /*
     * Determine if we have unsigned drivers installed
     * Use      2BD63D28D7BCD0E251195AEB519243C13142EBC3 as current key to check.
     * Old key: 300B971A74F97E098B67A4FCEBBBF6B9AE2F404C
     */
    if (NT_SUCCESS(RtlCheckRegistryKey(RTL_REGISTRY_ABSOLUTE,
            L"\\Registry\\Machine\\SOFTWARE\\Microsoft\\SystemCertificates\\Root\\Certificates\\2BD63D28D7BCD0E251195AEB519243C13142EBC3"))
             ||
            NT_SUCCESS(RtlCheckRegistryKey(RTL_REGISTRY_ABSOLUTE,
            L"\\Registry\\Machine\\SOFTWARE\\Policies\\Microsoft\\SystemCertificates\\Root\\Certificates\\2BD63D28D7BCD0E251195AEB519243C13142EBC3"))) {

        gfUnsignedDrivers = TRUE;
    }

    /*
     * Set up a desktop info structure that is visible in all
     * clients.
     */
    lTemp = FastGetProfileDwordW(NULL,
                                 PMAP_WINDOWSM,
                                 L"USERProcessHandleQuota",
                                 gUserProcessHandleQuota);

    if ((lTemp > MINIMUM_USER_HANDLE_QUOTA) && (lTemp <= INITIAL_USER_HANDLE_QUOTA)) {
        gUserProcessHandleQuota = lTemp;
    }

    /*
     * The maximum number of posted message for a thread.
     */
    lTemp = FastGetProfileDwordW(NULL,
                                 PMAP_WINDOWSM,
                                 L"USERPostMessageLimit",
                                 gUserPostMessageLimit);
    if (lTemp > MINIMUM_POSTMESSAGE_LIMIT) {
        gUserPostMessageLimit = lTemp;
    } else if (lTemp == 0) {
        /*
         * 0 means (virtually) No Limit.
         */
        gUserPostMessageLimit = ~0;
    } else {
        RIPMSG1(RIP_WARNING, "Win32UserInitialize: USERPostMessageLimit value (%d) is too low.", lTemp);
    }

    if (!gDrawVersionAlways) {
        gDrawVersionAlways = FastGetProfileDwordW(NULL,
                                                  PMAP_WINDOWSM,
                                                  L"DisplayVersion",
                                                  0);
    }

    gbSecureDesktop = FastGetProfileDwordW(NULL,
                                           PMAP_WINDOWSM,
                                           L"SecureDesktop",
                                           TRUE);

    /*
     * Initialize SMWP structure
     */
    if (!AllocateCvr(&gSMWP, 4)) {
        RIPMSG0(RIP_WARNING, "AllocateCvr failed");
        goto ExitWin32UserInitialize;
    }
    LeaveCrit();

    UserAssert(NT_SUCCESS(Status));
    return Status;

ExitWin32UserInitialize:
    LeaveCrit();

    if (NT_SUCCESS(Status)) {
        Status = STATUS_NO_MEMORY;
    }

    RIPMSG1(RIP_WARNING, "UserInitialize failed with Status = %x", Status);
    return Status;
}


/**************************************************************************\
* UserGetDesktopDC
*
* 09-Jan-1992 mikeke created
*    Dec-1993 andreva changed to support desktops.
\**************************************************************************/

HDC UserGetDesktopDC(
    ULONG type,
    BOOL  bAltType,
    BOOL  bValidate)
{
    PETHREAD    Thread;
    HDC         hdc;
    PTHREADINFO pti = PtiCurrentShared();  // This is called from outside the crit sec
    HDEV        hdev  = gpDispInfo->hDev;

    if (bValidate && type != DCTYPE_INFO &&
        IS_THREAD_RESTRICTED(pti, JOB_OBJECT_UILIMIT_HANDLES)) {

        UserAssert(pti->rpdesk != NULL);

        if (!ValidateHwnd(PtoH(pti->rpdesk->pDeskInfo->spwnd))) {
            RIPMSG0(RIP_WARNING,
                    "UserGetDesktopDC fails desktop window validation");
            return NULL;
        }
    }

    /*
     * !!! BUGBUG
     * This is a real nasty trick to get both DCs created on a desktop on
     * a different device to work (for the video applet) and to be able
     * to clip DCs that are actually on the same device ...
     */
    if (pti && pti->rpdesk)
        hdev = pti->rpdesk->pDispInfo->hDev;

    /*
     * We want to turn this call that was originally OpenDC("Display", ...)
     * into GetDC null call so this DC will be clipped to the current
     * desktop or else the DC can write to any desktop.  Only do this
     * for client apps; let the server do whatever it wants.
     */
    Thread = PsGetCurrentThread();
    if ((type != DCTYPE_DIRECT)  ||
        (hdev != gpDispInfo->hDev) ||
        IS_SYSTEM_THREAD(Thread) ||
        (Thread->ThreadsProcess == gpepCSRSS)) {

        hdc = GreCreateDisplayDC(hdev, type, bAltType);

    } else {

        PDESKTOP pdesk;

        EnterCrit();

        if (pdesk = PtiCurrent()->rpdesk) {

            hdc = _GetDCEx(pdesk->pDeskInfo->spwnd,
                           NULL,
                           DCX_WINDOW | DCX_CACHE | DCX_CREATEDC);
        } else {
            hdc = NULL;
        }

        LeaveCrit();
    }

    return hdc;
}


/**************************************************************************\
* UserThreadCallout
*
*
* Called by the kernel when a thread starts or ends.
*
* Dec-1993 andreva created.
\**************************************************************************/

NTSTATUS UserThreadCallout(
    IN PETHREAD pEThread,
    IN PSW32THREADCALLOUTTYPE CalloutType)
{
    PTHREADINFO pti;
    NTSTATUS    Status = STATUS_SUCCESS;

    UserAssert(gpresUser != NULL);

    switch (CalloutType) {
        case PsW32ThreadCalloutInitialize:
            TRACE_INIT(("Entering UserThreadCallout PsW32ThreadCalloutInitialize\n"));

            if (gbNoMorePowerCallouts) {
                RIPMSG0(RIP_WARNING, "No more GUI threads allowed");
                return STATUS_UNSUCCESSFUL;
            }

            /*
             * Only create a thread info structure if we're initialized.
             */
            if (gbUserInitialized) {
                EnterCrit();
                UserAssert(gpepCSRSS != NULL);

                /*
                 * Initialize this thread
                 */
                Status = xxxCreateThreadInfo(pEThread, FALSE);

                LeaveCrit();
            }
            break;

       case PsW32ThreadCalloutExit:

            TRACE_INIT(("Entering UserThreadCallout PsW32ThreadCalloutExit\n"));

            /*
             * If we aren't already inside the critical section, enter it.
             * Because this is the first pass, we remain in the critical
             * section when we return so that our try/finally handlers
             * are protected by the critical section.
             * EnterCrit here before GreUnlockDisplay() provides a pti which
             * may be required if unlocking the display may release some
             * deferred WinEvents, for which a pti is required.
             */
            EnterCrit();

            pti = (PTHREADINFO)pEThread->Tcb.Win32Thread;

            /*
             * WinStations that haven't gone through the first connect do not
             * have any of the graphics setup.
             */
            if (!gbRemoteSession || gbVideoInitialized) {

                /*
                 * Assert that we did not cleaned up gpDispInfo->hDev
                 */
                UserAssert(!gbCleanedUpResources);

                GreLockDisplay(gpDispInfo->hDev);
                GreUnlockDisplay(gpDispInfo->hDev);
            }


           /*
            * Mark this thread as in the middle of cleanup. This is useful for
            * several problems in USER where we need to know this information.
            */
           pti->TIF_flags |= TIF_INCLEANUP;

           /*
            * If we died during a full screen switch make sure we cleanup
            * correctly
            */
           FullScreenCleanup();
           /*
            * Cleanup gpDispInfo->hdcScreen - if we crashed while using it,
            * it may have owned objects still selected into it. Cleaning
            * it this way will ensure that gdi doesn't try to delete these
            * objects while they are still selected into this public hdc.
            */

           /*
            * WinStations that haven't gone through the first connect do not
            * have any of the graphics setup.
            */
           if (!gbRemoteSession || gbVideoInitialized) {
               GreCleanDC(gpDispInfo->hdcScreen);
           }

           /*
            * This thread is exiting execution; xxxDestroyThreadInfo cleans
            *  up everything that can go now
            */
           UserAssert(pti == PtiCurrent());
           xxxDestroyThreadInfo();
           LeaveCrit();

           break;
    }

    TRACE_INIT(("Leaving UserThreadCallout\n"));

    return Status;
}

/**************************************************************************\
* NtUserInitialize
*
* 01-Dec-1993 andreva created.
* 01-Dec-1995 BradG   Modified to return handle to Media Change Event
\**************************************************************************/

BOOL TellGdiToGetReady();

NTSTATUS NtUserInitialize(
    IN DWORD   dwVersion,
    IN HANDLE  hPowerRequestEvent,
    IN HANDLE  hMediaRequestEvent)
{
    NTSTATUS Status;

    TRACE_INIT(("Entering NtUserInitialize\n"));

    /*
     * Make sure we're not trying to load this twice.
     */
    if (gpepCSRSS != NULL) {
        RIPMSG0(RIP_ERROR, "Can't initialize more than once");
        return STATUS_UNSUCCESSFUL;
    }

    /*
     * Check version number
     */
    if (dwVersion != USERCURRENTVERSION) {
        KeBugCheckEx(WIN32K_INIT_OR_RIT_FAILURE,
                     0,
                     0,
                     dwVersion,
                     USERCURRENTVERSION);
    }

    /*
     * Get the session ID from the EPROCESS structure
     */
    gSessionId = PsGetCurrentProcess()->SessionId;

    UserAssert(gSessionId == 0 || gbRemoteSession == TRUE);

    /*
     * Initialize the power request list.
     */
    Status = InitializePowerRequestList(hPowerRequestEvent);
    if (!NT_SUCCESS(Status)) {
        return Status;
    }

    InitializeMediaChange(hMediaRequestEvent);

    /*
     * Save the system process structure.
     */
    gpepCSRSS = PsGetCurrentProcess();

    if (!TellGdiToGetReady())
    {
        RIPMSG0(RIP_WARNING, "TellGdiToGetReady failed");
        Status = STATUS_UNSUCCESSFUL;
        return Status;
    }

    /*
     * Allow CSR to read the screen
     */
    ((PW32PROCESS)gpepCSRSS->Win32Process)->W32PF_Flags |= (W32PF_READSCREENACCESSGRANTED|W32PF_IOWINSTA);


    Status = UserInitialize();

    TRACE_INIT(("Leaving NtUserInitialize\n"));
    return Status;
}

/**************************************************************************\
* NtUserProcessConnect
*
* 01-Dec-1993   Andreva     Created.
\**************************************************************************/

NTSTATUS NtUserProcessConnect(
    IN HANDLE    hProcess,
    IN OUT PVOID pConnectInfo,
    IN ULONG     cbConnectInfo)
{
    PEPROCESS    Process;
    PUSERCONNECT pucConnect = (PUSERCONNECT)pConnectInfo;
    USERCONNECT  ucLocal;
    NTSTATUS     Status = STATUS_SUCCESS;


    TRACE_INIT(("Entering NtUserProcessConnect\n"));

    if (!pucConnect || (cbConnectInfo != sizeof(USERCONNECT))) {
        return STATUS_UNSUCCESSFUL;
    }

    try {
        ProbeForWrite(pucConnect, cbConnectInfo, sizeof(DWORD));

        ucLocal = *pucConnect;
    } except (W32ExceptionHandler(FALSE, RIP_WARNING)) {
        return GetExceptionCode();
    }

    /*
     * Check client/server versions.
     */
    if (ucLocal.ulVersion != USERCURRENTVERSION) {

        RIPMSG2(RIP_ERROR,
            "Client version %lx > server version %lx\n",
            ucLocal.ulVersion, USERCURRENTVERSION);
        return STATUS_UNSUCCESSFUL;
    }



    if (ucLocal.dwDispatchCount != gDispatchTableValues) {
        RIPMSG2(RIP_ERROR,
            "!!!! Client Dispatch info %lX != Server %lX\n",
            ucLocal.dwDispatchCount, gDispatchTableValues);
    }


    /*
     * Reference the process.
     */
    Status = ObReferenceObjectByHandle(hProcess,
                                       PROCESS_VM_OPERATION,
                                       *PsProcessType,
                                       UserMode,
                                       &Process,
                                       NULL);
    if (!NT_SUCCESS(Status))
        return Status;
    /*
     * Return client's view of shared data.
     */
    Status = InitMapSharedSection(Process, &ucLocal);

    if (!NT_SUCCESS(Status)                       &&
        (Status != STATUS_NO_MEMORY)              &&
        (Status != STATUS_PROCESS_IS_TERMINATING) &&
        (Status != STATUS_QUOTA_EXCEEDED)         &&
        (Status != STATUS_COMMITMENT_LIMIT)) {

        RIPMSG2(RIP_ERROR,
              "Failed to map shared data into client %x, status = %x\n",
              GetCurrentProcessId(), Status);
    }

    ObDereferenceObject(Process);

    if (NT_SUCCESS(Status)) {

        try {
             *pucConnect = ucLocal;
        } except (W32ExceptionHandler(FALSE, RIP_WARNING)) {
            Status = GetExceptionCode();
        }
    }

    TRACE_INIT(("Leaving NtUserProcessConnect\n"));

    return Status;
}

/**************************************************************************\
* xxxUserProcessCallout
*
* 01-Dec-1993   andreva     Created.
\**************************************************************************/

NTSTATUS xxxUserProcessCallout(
    IN PW32PROCESS Process,
    IN BOOLEAN     Initialize)
{
    NTSTATUS     Status = STATUS_SUCCESS;

    if (Initialize) {

        TRACE_INIT(("Entering xxxUserProcessCallout Initialize\n"));

        UserAssert(gpresUser != NULL);
        EnterCrit();

        /*
         * Initialize the important process level stuff.
         */
        Status = xxxInitProcessInfo(Process);

        LeaveCrit();

        if (Status == STATUS_SUCCESS) {

            if (Process->Process &&
                Process->Process->Job &&
                Process->Process->Job->UIRestrictionsClass != 0) {

                WIN32_JOBCALLOUT_PARAMETERS Parms;

                PEJOB Job = Process->Process->Job;

                /*
                 * aquire the job's lock and after that enter the user
                 * critical section.
                 */
                KeEnterCriticalRegion();
                ExAcquireResourceExclusive(&Job->JobLock, TRUE);

                Parms.Job = Job;
                Parms.CalloutType = PsW32JobCalloutAddProcess;
                Parms.Data = Process;

                UserAssert(Job->SessionId == Process->Process->SessionId);

                UserJobCallout(&Parms);

                ExReleaseResource(&Job->JobLock);
                KeLeaveCriticalRegion();
            }
        }
    } else {

        int  i;
        PHE  phe;
        PDCE *ppdce;
        PDCE pdce;

        TRACE_INIT(("Entering xxxUserProcessCallout Cleanup\n"));

        UserAssert(gpresUser != NULL);

        EnterCrit();

#if DBG
        if (Process->Process == gpepCSRSS) {

            /*
             * CSRSS should be the last to go ...
             */
            UserAssert(gppiList->ppiNextRunning == NULL);
        }
#endif // DBG

        if (Process->Process && Process->Process->Job != NULL) {
            RemoveProcessFromJob((PPROCESSINFO)Process);
        }

        /*
         * DestroyProcessInfo will return TRUE if any threads ever
         * connected.  If nothing ever connected, we needn't do
         * this cleanup.
         */
        if (DestroyProcessInfo(Process)) {

            /*
             * See if we can compact the handle table.
             */
            i = giheLast;
            phe = &gSharedInfo.aheList[giheLast];
            while ((phe > &gSharedInfo.aheList[0]) && (phe->bType == TYPE_FREE)) {
                phe--;
                giheLast--;
            }

            /*
             * Scan the DC cache to find any DC's that need to be destroyed.
             */
            for (ppdce = &gpDispInfo->pdceFirst; *ppdce != NULL; ) {

                pdce = *ppdce;
                if (pdce->DCX_flags & DCX_DESTROYTHIS)
                    DestroyCacheDC(ppdce, pdce->hdc);

                /*
                 * Step to the next DC.  If the DC was deleted, there
                 * is no need to calculate address of the next entry.
                 */
                if (pdce == *ppdce)
                    ppdce = &pdce->pdceNext;
            }
        }

        UserAssert(gpresUser != NULL);

        LeaveCrit();
    }

    TRACE_INIT(("Leaving xxxUserProcessCallout\n"));

    return Status;
}

/**************************************************************************\
* UserGetHDEV
*
* Provided as a means for GDI to get a hold of USER's hDev.
*
* 01-Jan-1996   ChrisWil    Created.
\**************************************************************************/

HDEV UserGetHDEV(VOID)
{

    /*
     * BUGBUG This is busted.
     *        This need to return the device for the current desktop.
     *        The graphics device may not be the same for all desktops.
     *        -Andre
     */
    return gpDispInfo->hDev;
}

/**************************************************************************\
* _UserGetGlobalAtomTable
*
* This function is called by the kernel mode global atom manager to get the
* address of the current thread's global atom table.
*
* Pointer to the global atom table for the current thread or NULL if unable
* to access it.
*
*
\**************************************************************************/

PVOID UserGlobalAtomTableCallout(VOID)
{
    PETHREAD       Thread;
    PTHREADINFO    pti;
    PWINDOWSTATION pwinsta;
    PW32JOB        pW32Job;
    PEJOB          Job;
    PVOID          GlobalAtomTable = NULL;

    Thread = PsGetCurrentThread();

    pti = PtiFromThread(Thread);

    EnterCrit();

    /*
     * For restricted threads access the atom table off of the job object
     */
    if (pti != NULL && IS_THREAD_RESTRICTED(pti, JOB_OBJECT_UILIMIT_GLOBALATOMS)) {

        UserAssert(pti->ppi != NULL);

        pW32Job = pti->ppi->pW32Job;

        UserAssert(pW32Job != NULL && pW32Job->pAtomTable != NULL);

        GlobalAtomTable = pW32Job->pAtomTable;

        goto End;
    }

    Job = PsGetCurrentProcess()->Job;

    /*
     * Now handle the case where this si not a GUI thread/process
     * but it is assigned to a job that has JOB_OBJECT_UILIMIT_GLOBALATOMS
     * restrictions set. There is no easy way to convert this thread
     * to GUI.
     */
    if (pti == NULL && Job != NULL &&
        (Job->UIRestrictionsClass & JOB_OBJECT_UILIMIT_GLOBALATOMS)) {

        /*
         * find the W32JOB in the global list
         */
        pW32Job = gpJobsList;

        while (pW32Job) {
            if (pW32Job->Job == Job) {
                break;
            }
            pW32Job = pW32Job->pNext;
        }

        UserAssert(pW32Job != NULL && pW32Job->pAtomTable != NULL);

        GlobalAtomTable = pW32Job->pAtomTable;

        goto End;
    }

#if DBG
    pwinsta = NULL;
#endif

    if (NT_SUCCESS(ReferenceWindowStation(Thread,
                                    PsGetCurrentProcess()->Win32WindowStation,
                                    WINSTA_ACCESSGLOBALATOMS,
                                    &pwinsta,
                                    TRUE))) {
        UserAssert(pwinsta != NULL);

        GlobalAtomTable = pwinsta->pGlobalAtomTable;
    }

End:
    LeaveCrit();

#if DBG
    if (GlobalAtomTable == NULL) {
        RIPMSG1(RIP_WARNING,
                "_UserGetGlobalAtomTable: NULL Atom Table for pwinsta=0x%x",
                pwinsta);
    }
#endif

    return GlobalAtomTable;
}
