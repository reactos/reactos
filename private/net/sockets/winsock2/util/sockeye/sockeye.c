/*++ BUILD Version: 0000    // Increment this if a change has global effects

Copyright (c) 1996  Microsoft Corporation

Module Name:

    sockeye.c

Abstract:

    Contains UI support for winsock browser util.

Author:

    Dan Knudson (DanKn)    29-Jul-1996

Revision History:

--*/


#include "sockeye.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>
#include <string.h>
#include <ctype.h>
#include <io.h>
#include <commdlg.h>
#include "resource.h"
#include "vars.h"
#include "nspapi.h"


int                 giCurrNumStartups = 0;
HWND                hwndEdit2, ghwndModal;
char                gszEnterAs[32];
PASYNC_REQUEST_INFO gpAsyncReqInfoList = NULL;
PMYSOCKET           gpSelectedSocket;
MYSOCKET            gNullSocket;
LRESULT             glCurrentSelection;
WSAPROTOCOL_INFOA   gWSAProtocolInfoA;
WSAPROTOCOL_INFOW   gWSAProtocolInfoW;
HFONT               ghFixedFont;
WSAEVENT            ghSelectedEvent;
INT                 giSelectedEventIndex;
WSAPROTOCOL_INFOW   gDupSockProtoInfo;


char                gszSocket[] = "socket";
char                gszSockEye[] = "SockEye";
char                gszUnknown[] = "unknown";
char                gszProtocol[] = "Protocol";
char                gszHostName[] = "HostName";
char                gszSendFlags[] = "SendFlags";
char                gszRecvFlags[] = "RecvFlags";
char                gszXxxSUCCESS[] = "%s SUCCESS";
char                gszSocketType[] = "SocketType";
char                gszPortNumber[] = "PortNumber";
char                gszServiceName[] = "ServiceName";
char                gszIoctlCommand[] = "IoctlCommand";
char                gszProtocolName[] = "ProtocolName";
char                gszUnknownError[] = "unknown error";
char                gszAddressFamily[] = "AddressFamily";
char                gszWSASocketFlags[] = "WSASocketFlags";
char                gszProtocolNumber[] = "ProtocolNumber";

DWORD               gdwDefPortNum;
DWORD               gdwDefProtocol;
DWORD               gdwDefProtoNum;
DWORD               gdwDefIoctlCmd;
DWORD               gdwDefSendFlags;
DWORD               gdwDefRecvFlags;
DWORD               gdwDefAddrFamily;
DWORD               gdwDefSocketType;
DWORD               gdwDefWSASocketFlags;
char                gszDefHostName[MAX_STRING_PARAM_SIZE];
char                gszDefProtoName[MAX_STRING_PARAM_SIZE];
char                gszDefServName[MAX_STRING_PARAM_SIZE];


char *
PASCAL
GetTimeStamp(
    void
    );

void
ShowStructByField(
    PSTRUCT_FIELD_HEADER    pHeader,
    BOOL    bSubStructure
    );

VOID
PASCAL
ShowGUID(
    char    *pszProlog,
    GUID    *pGuid
    );

void
FAR
PASCAL
FuncDriver(
    FUNC_INDEX funcIndex
    );


int
WINAPI
WinMain(
    HINSTANCE   hInstance,
    HINSTANCE   hPrevInstance,
    LPSTR       lpCmdLine,
    int         nCmdShow
    )
{
    MSG     msg;
    HWND    hwnd;
    DWORD   i;
    HACCEL  hAccel;


    ghInst = hInstance;

    ZeroMemory (&gNullSocket, sizeof (MYSOCKET));
    ZeroMemory (&gWSAProtocolInfoA, sizeof (WSAPROTOCOL_INFOA));
    ZeroMemory (&gWSAProtocolInfoW, sizeof (WSAPROTOCOL_INFOW));

    {
        DWORD d = 0x76543210;


        wsprintf(
            gszEnterAs,
            "Ex: enter x%x as %02x%02x%02x%02x",
            d,
            (DWORD) *((LPBYTE) &d),
            (DWORD) *(((LPBYTE) &d) + 1),
            (DWORD) *(((LPBYTE) &d) + 2),
            (DWORD) *(((LPBYTE) &d) + 3)
            );
    }

    hwnd = CreateDialog(
        ghInst,
        (LPCSTR)MAKEINTRESOURCE(IDD_DIALOG1),
        (HWND)NULL,
        (DLGPROC) MainWndProc
        );

    hwndEdit2 = CreateWindow ("edit", "", 0, 0, 0, 0, 0, NULL, NULL, ghInst, NULL);

    if (!hwndEdit2)
    {
        MessageBox (NULL, "err creating edit ctl", "", MB_OK);
    }

    hAccel = LoadAccelerators(
        ghInst,
        (LPCSTR)MAKEINTRESOURCE(IDR_ACCELERATOR1)
        );

    while (GetMessage (&msg, NULL, 0, 0))
    {
        if (!TranslateAccelerator (hwnd, hAccel, &msg))
        {
            TranslateMessage (&msg);
            DispatchMessage (&msg);
        }
    }

    DestroyWindow (hwndEdit2);

    return 0;
}


LPVOID
PASCAL
MyAlloc(
    SIZE_T   dwSize
    )
{
    LPVOID  p;


    if (!(p = LocalAlloc (LPTR, dwSize)))
    {
        ShowStr ("LocalAlloc (LPTR, x%x) failed, error=%d", GetLastError());
    }

    return p;
}


void
PASCAL
MyFree(
    LPVOID  p
    )
{
    if (p)
    {
        LocalFree (p);
    }
}


void
GetCurrentSelections(
    void
    )
{
    LRESULT   lSel = SendMessage (ghwndList1, LB_GETCURSEL, 0, 0);
    LRESULT   lSelCount = SendMessage (ghwndList3, LB_GETSELCOUNT, 0, 0);


    if ((lSel = SendMessage (ghwndList1, LB_GETCURSEL, 0, 0)) != LB_ERR)
    {
        glCurrentSelection = lSel;
        gpSelectedSocket = (PMYSOCKET) SendMessage(
            ghwndList1,
            LB_GETITEMDATA,
            (WPARAM) lSel,
            0
            );
    }
    else
    {
        gpSelectedSocket = &gNullSocket;
    }

    if (lSelCount == 0  ||  lSelCount == LB_ERR)
    {
        ghSelectedEvent = NULL;
    }
    else
    {
        SendMessage(
            ghwndList3,
            LB_GETSELITEMS,
            1,
            (LPARAM) &giSelectedEventIndex
            );

        ghSelectedEvent = (WSAEVENT) SendMessage(
            ghwndList3,
            LB_GETITEMDATA,
            giSelectedEventIndex,
            0
            );
    }
}


BOOL
LetUserMungeParams(
    PFUNC_PARAM_HEADER pParamsHeader
    )
{
    if (!bShowParams)
    {
        return TRUE;
    }

    return (DialogBoxParam(
        ghInst,
        (LPCSTR)MAKEINTRESOURCE(IDD_DIALOG2),
        ghwndModal,
        (DLGPROC) ParamsDlgProc,
        (LPARAM) pParamsHeader
        ) ? TRUE : FALSE);
}


INT_PTR
CALLBACK
MainWndProc(
    HWND    hwnd,
    UINT    msg,
    WPARAM  wParam,
    LPARAM  lParam
    )
{
    static HICON  hIcon;
    static HMENU  hMenu;
    static int    icyButton, icyBorder;
    static HFONT  hFont, hFont2;
    static int    iMaxEditTextLength;
    static HANDLE hTimerEvent;

    #define TIMER_ID      55
    #define TIMER_TIMEOUT 2000

    int  i;

    static LONG cxList1, cxList2, cxWnd, xCapture, cxVScroll, lCaptureFlags = 0;
    static int cyWnd;

    typedef struct _XXX
    {
        DWORD   dwMenuID;

        DWORD   dwFlags;

    } XXX, *PXXX;

    static XXX aXxx[] =
    {
        { IDM_LOGSTRUCTDWORD        ,DS_BYTEDUMP },
        { IDM_LOGSTRUCTALLFIELD     ,DS_NONZEROFIELDS|DS_ZEROFIELDS },
        { IDM_LOGSTRUCTNONZEROFIELD ,DS_NONZEROFIELDS },
        { IDM_LOGSTRUCTNONE         ,0 }
    };

    switch (msg)
    {
    case WM_INITDIALOG:
    {
        RECT    rect;
        char    buf[64];
        DWORD   dwInitialized;

        wsprintf(
            buf,
            "WinSock API Browser (ProcessID=x%x)",
            GetCurrentProcessId()
            );

        SetWindowText (hwnd, buf);

        ghwndMain  = ghwndModal = hwnd;
        ghwndList1 = GetDlgItem (hwnd, IDC_LIST1);
        ghwndList2 = GetDlgItem (hwnd, IDC_LIST2);
        ghwndList3 = GetDlgItem (hwnd, IDC_LIST3);
        ghwndEdit  = GetDlgItem (hwnd, IDC_EDIT1);
        hMenu      = GetMenu (hwnd);
        hIcon      = LoadIcon (ghInst, MAKEINTRESOURCE(IDI_ICON1));

        icyBorder = GetSystemMetrics (SM_CYFRAME);
        cxVScroll = 2*GetSystemMetrics (SM_CXVSCROLL);

        GetWindowRect (GetDlgItem (hwnd, IDC_BUTTON1), &rect);
        icyButton = (rect.bottom - rect.top) + icyBorder + 3;

        for (i = 0; aFuncNames[i]; i++)
        {
            SendMessage(
                ghwndList2,
                LB_INSERTSTRING,
                (WPARAM) -1,
                (LPARAM) aFuncNames[i]
                );
        }

//        SendMessage (ghwndList2, LB_SETCURSEL, (WPARAM) lInitialize, 0);


        //
        // Read in defaults from ini file
        //

        {
            typedef struct _DEF_VALUE
            {
                char far *lpszEntry;
                char far *lpszDefValue;
                LPVOID   lp;

            } DEF_VALUE;

            DEF_VALUE aDefVals[] =
            {
                { "BufSize",            "100",  &dwBigBufSize },
                { "UserButton1",        "500",  aUserButtonFuncs },
                { "UserButton2",        "500",  aUserButtonFuncs + 1 },
                { "UserButton3",        "500",  aUserButtonFuncs + 2 },
                { "UserButton4",        "500",  aUserButtonFuncs + 3 },
                { "UserButton5",        "500",  aUserButtonFuncs + 4 },
                { "UserButton6",        "500",  aUserButtonFuncs + 5 },
                { "Initialized",        "0",    &dwInitialized },
                { gszAddressFamily,     "2",    &gdwDefAddrFamily },
                { gszSocketType,        "2",    &gdwDefSocketType },
                { gszProtocol,          "0",    &gdwDefProtocol },
                { gszProtocolNumber,    "0",    &gdwDefProtoNum },
                { gszPortNumber,        "0",    &gdwDefPortNum },
                { gszIoctlCommand,      "0",    &gdwDefIoctlCmd },
                { gszSendFlags,         "0",    &gdwDefSendFlags },
                { gszRecvFlags,         "0",    &gdwDefRecvFlags },
                { gszWSASocketFlags,    "0",    &gdwDefWSASocketFlags },
                { NULL, NULL, NULL },
                { gszHostName,          "HostName",     gszDefHostName },
                { gszProtocolName,      "ProtoName",    gszDefProtoName },
                { gszServiceName,       "ServiceName",  gszDefServName },
                { NULL, NULL, NULL },
                { "UserButton1Text",    "", &aUserButtonsText[0] },
                { "UserButton2Text",    "", &aUserButtonsText[1] },
                { "UserButton3Text",    "", &aUserButtonsText[2] },
                { "UserButton4Text",    "", &aUserButtonsText[3] },
                { "UserButton5Text",    "", &aUserButtonsText[4] },
                { "UserButton6Text",    "", &aUserButtonsText[5] },
                { NULL, NULL, NULL }
            };

            int i, j;

            for (i = 0; aDefVals[i].lpszEntry; i++)
            {
                GetProfileString(
                    gszSockEye,
                    aDefVals[i].lpszEntry,
                    aDefVals[i].lpszDefValue,
                    buf,
                    15
                    );

                sscanf (buf, "%lx", aDefVals[i].lp);
            }

            i++;

            for (; aDefVals[i].lpszEntry; i++)
            {
                GetProfileString(
                    gszSockEye,
                    aDefVals[i].lpszEntry,
                    aDefVals[i].lpszDefValue,
                    (LPSTR) aDefVals[i].lp,
                    MAX_STRING_PARAM_SIZE - 1
                    );
            }

            i++;

            for (j = i; aDefVals[i].lpszEntry; i++)
            {
                GetProfileString(
                    gszSockEye,
                    aDefVals[i].lpszEntry,
                    aDefVals[i].lpszDefValue,
                    (LPSTR) aDefVals[i].lp,
                    MAX_USER_BUTTON_TEXT_SIZE - 1
                    );

                SetDlgItemText(
                    hwnd,
                    IDC_BUTTON7 + (i - j),
                    (LPCSTR)aDefVals[i].lp
                    );
            }

            if (dwInitialized == 0)
            {
                //
                // If here assume this is first time user had started
                // app, so post a msg that will automatically bring up
                // the hlp dlg
                //

                PostMessage (hwnd, WM_COMMAND, IDM_USAGE, 0);
            }
        }

        pBigBuf = MyAlloc (dwBigBufSize);

        {
            //HFONT hFontMenu = SendMessage (hMenu, WM_GETFONT, 0, 0);

            hFont = CreateFont(
                13, 5, 0, 0, 400, 0, 0, 0, 0, 1, 2, 1, 34, "MS Sans Serif"
                );
            ghFixedFont =
            hFont2 = CreateFont(
                13, 8, 0, 0, 400, 0, 0, 0, 0, 1, 2, 1, 49, "Courier"
                );

            for (i = 0; i < 12; i++)
            {
                SendDlgItemMessage(
                    hwnd,
                    IDC_BUTTON1 + i,
                    WM_SETFONT,
                    (WPARAM) hFont,
                    0
                    );
            }

            SendDlgItemMessage (hwnd, IDM_PARAMS, WM_SETFONT,(WPARAM) hFont,0);
            SendMessage (ghwndList1, WM_SETFONT, (WPARAM) hFont, 0);
            SendMessage (ghwndList2, WM_SETFONT, (WPARAM) hFont, 0);
            SendMessage (ghwndList3, WM_SETFONT, (WPARAM) hFont, 0);
            SendMessage (ghwndEdit, WM_SETFONT, (WPARAM) hFont2, 0);
        }

        GetProfileString(
            gszSockEye,
            "ControlRatios",
            "20, 20, 100",
            buf,
            63
            );

        sscanf (buf, "%ld,%ld,%ld", &cxList2, &cxList1, &cxWnd);

        GetProfileString(
            gszSockEye,
            "Position",
            "max",
            buf,
            63
            );

        if (strcmp (buf, "max") == 0)
        {
            ShowWindow (hwnd, SW_SHOWMAXIMIZED);
        }
        else
        {
            int left = 100, top = 100, right = 600, bottom = 400;


            sscanf (buf, "%d,%d,%d,%d", &left, &top, &right, &bottom);


            //
            // Check to see if wnd pos is wacky, if so reset to reasonable vals
            //

            if (left < 0 ||
                left >= (GetSystemMetrics (SM_CXSCREEN) - 32) ||
                top < 0 ||
                top >= (GetSystemMetrics (SM_CYSCREEN) - 32))
            {
                left = top = 100;
                right = 600;
                bottom = 400;
            }

            SetWindowPos(
                hwnd,
                HWND_TOP,
                left,
                top,
                right - left,
                bottom - top,
                SWP_SHOWWINDOW
                );

            GetClientRect (hwnd, &rect);

            SendMessage(
                hwnd,
                WM_SIZE,
                0,
                MAKELONG((rect.right-rect.left),(rect.bottom-rect.top))
                );

            ShowWindow (hwnd, SW_SHOW);
        }

        iMaxEditTextLength = ((GetVersion() & 0x80000000) ?
            29998 :  // we're on win95, and edit controls are 16-bit
            0x20000  // were on NT, and have real 32-bit edit controls
            );

        hTimerEvent = CreateEvent (NULL, FALSE, FALSE, NULL);
        SetTimer (hwnd, TIMER_ID, TIMER_TIMEOUT, NULL);

        break;
    }
    case WM_COMMAND:
    {
        FUNC_INDEX funcIndex;
        BOOL bShowParamsSave = bShowParams;

        switch (LOWORD((DWORD)wParam))
        {
        case IDC_EDIT1:

#ifdef WIN32
            if (HIWORD(wParam) == EN_CHANGE)
#else
            if (HIWORD(lParam) == EN_CHANGE)
#endif
            {
                //
                // Watch to see if the edit control is full, & if so
                // purge the top half of the text to make room for more
                //

                int length = GetWindowTextLength (ghwndEdit);


//                if (length > iMaxEditTextLength)
                if (length > 29998)
                {
#ifdef WIN32
                    SendMessage(
                        ghwndEdit,
                        EM_SETSEL,
                        (WPARAM) 0,
                        (LPARAM) 10000
                        );
#else
                    SendMessage(
                        ghwndEdit,
                        EM_SETSEL,
                        (WPARAM) 1,
                        (LPARAM) MAKELONG (0, 10000)
                        );
#endif

                    SendMessage(
                        ghwndEdit,
                        EM_REPLACESEL,
                        0,
                        (LPARAM) (char far *) ""
                        );

#ifdef WIN32
                    SendMessage(
                        ghwndEdit,
                        EM_SETSEL,
                        (WPARAM)0xfffffffd,
                        (LPARAM)0xfffffffe
                        );
#else
                    SendMessage(
                        ghwndEdit,
                        EM_SETSEL,
                        (WPARAM)1,
                        (LPARAM) MAKELONG (0xfffd, 0xfffe)
                        );
#endif
                }
            }
            break;

        case IDC_BUTTON1:

            FuncDriver (ws_WSAStartup);
            break;

        case IDC_BUTTON2:

            FuncDriver (ws_WSACleanup);
            break;

        case IDC_BUTTON3:

            FuncDriver (ws_socket);
            break;

        case IDC_BUTTON4:

            FuncDriver (ws_WSASocketA);
            break;

        case IDC_BUTTON5:

            GetCurrentSelections();
            FuncDriver (ws_closesocket);
            break;

        case IDC_BUTTON6:
        case IDM_CLEAR:

            SetWindowText (ghwndEdit, "");
            break;


        case IDM_PARAMS:

            bShowParams = (bShowParams ? FALSE : TRUE);

            if (bShowParams)
            {
                CheckMenuItem(
                    hMenu,
                    IDM_PARAMS,
                    MF_BYCOMMAND | MF_CHECKED
                    );

                CheckDlgButton (hwnd, IDM_PARAMS, 1);
            }
            else
            {
                CheckMenuItem(
                    hMenu,
                    IDM_PARAMS,
                    MF_BYCOMMAND | MF_UNCHECKED
                    );

                CheckDlgButton (hwnd, IDM_PARAMS, 0);
            }

            break;

        case IDC_BUTTON7:
        case IDC_BUTTON8:
        case IDC_BUTTON9:
        case IDC_BUTTON10:
        case IDC_BUTTON11:
        case IDC_BUTTON12:
        {
            DWORD i = (DWORD) (LOWORD((DWORD)wParam)) - IDC_BUTTON7;


            if (aUserButtonFuncs[i] >= MiscBegin)
            {
                //
                // Hot button func id is bogus, so bring
                // up hot button init dlg
                //

                DialogBoxParam(
                    ghInst,
                    (LPCSTR)MAKEINTRESOURCE(IDD_DIALOG3),
                    (HWND) hwnd,
                    (DLGPROC) UserButtonsDlgProc,
                    (LPARAM) &i
                    );
            }
            else
            {
                //
                // Invoke the user button's corresponding func
                //

                GetCurrentSelections ();
                FuncDriver ((FUNC_INDEX) aUserButtonFuncs[i]);
            }

            break;
        }
        case IDC_PREVCTRL:
        {
            HWND hwndPrev = GetNextWindow (GetFocus (), GW_HWNDPREV);

            if (!hwndPrev)
            {
                hwndPrev = ghwndList2;
            }

            SetFocus (hwndPrev);
            break;
        }
        case IDC_NEXTCTRL:
        {
            HWND hwndNext = GetNextWindow (GetFocus (), GW_HWNDNEXT);

            if (!hwndNext)
            {
                hwndNext = GetDlgItem (hwnd, IDM_PARAMS);
            }

            SetFocus (hwndNext);
            break;
        }
        case IDC_ENTER:
        {
            if (GetFocus() != ghwndEdit)
            {
                GetCurrentSelections ();
                FuncDriver(
                    (FUNC_INDEX)SendMessage(
                        ghwndList2,
                        LB_GETCURSEL,
                        0,
                        0
                        ));
            }
            else
            {
                // Send the edit ctrl a cr/lf
            }

            break;
        }
        case IDC_LIST1:

#ifdef WIN32
            switch (HIWORD(wParam))
#else
            switch (HIWORD(lParam))
#endif
            {
            case LBN_DBLCLK:
            {
/*
                LONG lSel = SendMessage (ghwndList1, LB_GETCURSEL, 0, 0);
                PMYWIDGET pWidget;


                pWidget = (PMYWIDGET) SendMessage(
                    ghwndList1,
                    LB_GETITEMDATA,
                    (WPARAM) lSel,
                    0
                    );

                bShowParams = FALSE;

//                UpdateResults (TRUE);

                switch (pWidget->dwType)
                {
                }

//                UpdateResults (FALSE);

                bShowParams = bShowParamsSave;
*/
                break;
            }
            } // switch

            break;

        case IDC_LIST2:

#ifdef WIN32
            if (HIWORD(wParam) == LBN_DBLCLK)
#else
            if (HIWORD(lParam) == LBN_DBLCLK)
#endif
            {
                GetCurrentSelections ();
                FuncDriver(
                    (FUNC_INDEX)SendMessage(
                        ghwndList2,
                        LB_GETCURSEL,
                        0,
                        0
                        ));
            }

            break;

        case IDM_EXIT:
        {
            PostMessage (hwnd, WM_CLOSE, 0, 0);
            break;
        }
        case IDM_DEFAULTVALUES:
        {
            char szHostName[MAX_STRING_PARAM_SIZE],
                 szProtoName[MAX_STRING_PARAM_SIZE],
                 szServName[MAX_STRING_PARAM_SIZE];
            FUNC_PARAM params[] =
            {
                { "Buffer size",     PT_DWORD,   (ULONG_PTR) dwBigBufSize, NULL },
                { gszAddressFamily,  PT_ORDINAL, (ULONG_PTR) gdwDefAddrFamily, aAddressFamilies },
                { gszSocketType,     PT_ORDINAL, (ULONG_PTR) gdwDefSocketType, aSocketTypes },
                { gszProtocol,       PT_ORDINAL, (ULONG_PTR) gdwDefProtocol, aProtocols },
                { gszProtocolNumber, PT_DWORD,   (ULONG_PTR) gdwDefProtoNum, NULL },
                { gszPortNumber,     PT_DWORD,   (ULONG_PTR) gdwDefPortNum, NULL },
                { gszIoctlCommand,   PT_ORDINAL, (ULONG_PTR) gdwDefIoctlCmd, aWSAIoctlCmds },
                { gszHostName,       PT_STRING,  (ULONG_PTR) szHostName, szHostName },
                { gszProtocolName,   PT_STRING,  (ULONG_PTR) szProtoName, szProtoName },
                { gszServiceName,    PT_STRING,  (ULONG_PTR) szServName, szServName },
                { gszSendFlags,      PT_FLAGS,   (ULONG_PTR) aWSASendFlags, aWSASendFlags },
                { gszRecvFlags,      PT_FLAGS,   (ULONG_PTR) aWSARecvFlags, aWSARecvFlags },
                { gszWSASocketFlags, PT_FLAGS,   (ULONG_PTR) aWSAFlags, aWSAFlags }
            };
            FUNC_PARAM_HEADER paramsHeader =
                { 13, DefValues, params, NULL };
            BOOL bShowParamsSave = bShowParams;


            bShowParams = TRUE;

            lstrcpyA (szHostName, gszDefHostName);
            lstrcpyA (szProtoName, gszDefProtoName);
            lstrcpyA (szServName, gszDefServName);

            if (LetUserMungeParams (&paramsHeader))
            {
                if (params[0].dwValue != dwBigBufSize)
                {
                    LPVOID pTmpBigBuf = MyAlloc (params[0].dwValue);

                    if (pTmpBigBuf)
                    {
                        MyFree (pBigBuf);
                        pBigBuf = pTmpBigBuf;
                        dwBigBufSize = (DWORD) params[0].dwValue;
                    }
                }

                gdwDefAddrFamily = (DWORD) params[1].dwValue;
                gdwDefSocketType = (DWORD) params[2].dwValue;
                gdwDefProtocol   = (DWORD) params[3].dwValue;
                gdwDefProtoNum   = (DWORD) params[4].dwValue;
                gdwDefPortNum    = (DWORD) params[5].dwValue;
                gdwDefIoctlCmd   = (DWORD) params[6].dwValue;

                if (params[7].dwValue == (ULONG_PTR) szHostName)
                {
                    lstrcpyA (gszDefHostName, szHostName);
                }

                if (params[8].dwValue == (ULONG_PTR) szProtoName)
                {
                    lstrcpyA (gszDefProtoName, szProtoName);
                }

                if (params[9].dwValue == (ULONG_PTR) szServName)
                {
                    lstrcpyA (gszDefServName, szServName);
                }

                gdwDefSendFlags      = (DWORD) params[10].dwValue;
                gdwDefRecvFlags      = (DWORD) params[11].dwValue;
                gdwDefWSASocketFlags = (DWORD) params[12].dwValue;
            }

            bShowParams = bShowParamsSave;

            break;
        }
        case IDM_USERBUTTONS:

            DialogBoxParam(
                ghInst,
                (LPCSTR)MAKEINTRESOURCE(IDD_DIALOG3),
                (HWND) hwnd,
                (DLGPROC) UserButtonsDlgProc,
                (LPARAM) NULL
                );

            break;

        case IDM_DUMPPARAMS:

            bDumpParams = (bDumpParams ? FALSE : TRUE);

            CheckMenuItem(
                hMenu,
                IDM_DUMPPARAMS,
                MF_BYCOMMAND | (bDumpParams ? MF_CHECKED : MF_UNCHECKED)
                );

            break;

        case IDM_LOGSTRUCTDWORD:
        case IDM_LOGSTRUCTALLFIELD:
        case IDM_LOGSTRUCTNONZEROFIELD:
        case IDM_LOGSTRUCTNONE:

            for (i = 0; aXxx[i].dwFlags != dwDumpStructsFlags; i++);

            CheckMenuItem(
                hMenu,
                aXxx[i].dwMenuID,
                MF_BYCOMMAND | MF_UNCHECKED
                );

            for (i = 0; aXxx[i].dwMenuID != LOWORD((DWORD)wParam); i++);

            CheckMenuItem(
                hMenu,
                aXxx[i].dwMenuID,
                MF_BYCOMMAND | MF_CHECKED
                );

            dwDumpStructsFlags = aXxx[i].dwFlags;

            break;

        case IDM_TIMESTAMP:

            bTimeStamp = (bTimeStamp ? FALSE : TRUE);

            CheckMenuItem(
                hMenu,
                IDM_TIMESTAMP,
                MF_BYCOMMAND | (bTimeStamp ? MF_CHECKED : MF_UNCHECKED)
                );

            break;

        case IDM_LOGFILE:
        {
            if (hLogFile)
            {
                fclose (hLogFile);
                hLogFile = (FILE *) NULL;
                CheckMenuItem(
                    hMenu,
                    IDM_LOGFILE,
                    MF_BYCOMMAND | MF_UNCHECKED
                    );
            }
            else
            {
                OPENFILENAME ofn;
                char szDirName[256] = ".\\";
                char szFile[256] = "sockeye.log\0";
                char szFileTitle[256] = "";
                static char *szFilter =
                    "Log files (*.log)\0*.log\0All files (*.*)\0*.*\0\0";


                ofn.lStructSize       = sizeof(OPENFILENAME);
                ofn.hwndOwner         = hwnd;
                ofn.lpstrFilter       = szFilter;
                ofn.lpstrCustomFilter = (LPSTR) NULL;
                ofn.nMaxCustFilter    = 0L;
                ofn.nFilterIndex      = 1;
                ofn.lpstrFile         = szFile;
                ofn.nMaxFile          = sizeof(szFile);
                ofn.lpstrFileTitle    = szFileTitle;
                ofn.nMaxFileTitle     = sizeof(szFileTitle);
                ofn.lpstrInitialDir   = szDirName;
                ofn.lpstrTitle        = (LPSTR) NULL;
                ofn.Flags             = 0L;
                ofn.nFileOffset       = 0;
                ofn.nFileExtension    = 0;
                ofn.lpstrDefExt       = "LOG";

                if (!GetOpenFileName(&ofn))
                {
                    return 0L;
                }

                if ((hLogFile = fopen (szFile, "at")) == (FILE *) NULL)
                {
                    MessageBox(
                        hwnd,
                        "Error creating log file",
                        gszSockEye,
                        MB_OK
                        );
                }
                else
                {
                    struct tm *newtime;
                    time_t aclock;


                    time (&aclock);
                    newtime = localtime (&aclock);
                    fprintf(
                        hLogFile,
                        "\n---Log opened: %s\n",
                        asctime (newtime)
                        );

                    CheckMenuItem(
                        hMenu,
                        IDM_LOGFILE,
                        MF_BYCOMMAND | MF_CHECKED
                        );
                }
            }
            break;
        }
        case IDM_USAGE:
        {
            static char szUsage[] =
                "ABSTRACT:\r\n"                                           \
                "    SockEye (the WinSock API Browser) allows a user "    \
                "to interactively call into the Windows Sockets "         \
                "API, modify parameters, and inspect all returned "       \
                "information.\r\n"                                        \
                "\r\n"                                                    \

                "GETTING STARTED:\r\n"                                    \
                "1. Press the 'Startup' button to initialize WinSock\r\n" \
                "2. Double-click on items in the left-most window "       \
                "to invoke the corresponding API. (Check the Params "     \
                "checkbox in the upper left corner to change "            \
                "parameters.)\r\n"                                        \
                "*  Press the 'Cleanup' button to shutdown WinSock\r\n"   \
                "\r\n"                                                    \

                "MORE INFO:\r\n"                                          \
                "*  Choose 'Options/Default values...' to modify "        \
                "default parameter values\r\n"                            \
                "*  Choose 'Options/Record log file' to save all "        \
                "output to a file.\r\n"                                   \
                "*  All parameter values in hexadecimal unless "          \
                "specified (strings displayed by contents, not "          \
                "pointer value).\r\n"                                     \
                "*  Choose 'Options/User buttons...' or press "           \
                "one of the buttons on right side of toolbar to "         \
                "create a personal hot-link between a button and a "      \
                "particular function.";


            DialogBoxParam(
                ghInst,
                (LPCSTR)MAKEINTRESOURCE(IDD_DIALOG6),
                (HWND)hwnd,
                (DLGPROC) AboutDlgProc,
                (LPARAM) szUsage
                );

            break;
        }
        case IDM_NOHANDLECHK:

            gbDisableHandleChecking = (gbDisableHandleChecking ? 0 : 1);

            CheckMenuItem(
                hMenu,
                IDM_NOHANDLECHK,
                MF_BYCOMMAND |
                    (gbDisableHandleChecking ? MF_CHECKED : MF_UNCHECKED)
                );

            break;

        case IDM_ABOUT:
        {
            DialogBoxParam(
                ghInst,
                (LPCSTR)MAKEINTRESOURCE(IDD_DIALOG4),
                (HWND)hwnd,
                (DLGPROC) AboutDlgProc,
                0
                );

            break;
        }
        } // switch

        break;
    }
    case WM_ASYNCREQUESTCOMPLETED:
    {
        PASYNC_REQUEST_INFO pAsyncReqInfo;


        if ((pAsyncReqInfo = DequeueAsyncRequestInfo ((HANDLE) wParam)))
        {
            if (WSAGETASYNCERROR(lParam) == 0)
            {
                ShowStr(
                    "Request x%x (%s) completed successfully",
                    pAsyncReqInfo->hRequest,
                    aFuncNames[pAsyncReqInfo->FuncIndex]
                    );

                switch (pAsyncReqInfo->FuncIndex)
                {
                case ws_WSAAsyncGetHostByAddr:
                case ws_WSAAsyncGetHostByName:

                    ShowHostEnt ((struct hostent *) (pAsyncReqInfo + 1));
                    break;

                case ws_WSAAsyncGetProtoByName:
                case ws_WSAAsyncGetProtoByNumber:

                    ShowProtoEnt ((struct protoent *) (pAsyncReqInfo + 1));
                    break;

                case ws_WSAAsyncGetServByName:
                case ws_WSAAsyncGetServByPort:

                    ShowServEnt ((struct servent *) (pAsyncReqInfo + 1));
                    break;
                }
            }
            else
            {
                char FAR *pszError;


                for(
                    i = 0;
                    WSAGETASYNCERROR(lParam) != aWSAErrors[i].dwVal  &&
                        aWSAErrors[i].lpszVal;
                    i++
                    );

                pszError = (aWSAErrors[i].lpszVal ? aWSAErrors[i].lpszVal :
                    gszUnknownError);

                ShowStr(
                    "Request x%x (%s) completed unsuccessfully: %s (%d)",
                    pAsyncReqInfo->hRequest,
                    aFuncNames[pAsyncReqInfo->FuncIndex],
                    pszError,
                    (DWORD) WSAGETASYNCERROR(lParam)
                    );
            }

            MyFree (pAsyncReqInfo);
        }
        else
        {
        }

        break;
    }
    case WM_NETWORKEVENT:

// BUGBUG case WM_NETWORKEVENT: format info
        ShowStr ("WM_NETWORKEVENT: wParam=x%x, lParam=x%x", wParam ,lParam);
        break;

#ifdef WIN32
    case WM_CTLCOLORBTN:

        SetBkColor ((HDC) wParam, RGB (192,192,192));
        return (INT_PTR) GetStockObject (LTGRAY_BRUSH);
#else
    case WM_CTLCOLOR:
    {
        if (HIWORD(lParam) == CTLCOLOR_BTN)
        {
            SetBkColor ((HDC) wParam, RGB (192,192,192));
            return (INT_PTR) GetStockObject (LTGRAY_BRUSH);
        }
        break;
    }
#endif

    case WM_MOUSEMOVE:
    {
        LONG x = (LONG)((short)LOWORD(lParam));
        int y = (int)((short)HIWORD(lParam));


        if ((y > icyButton  &&  (x < (cxList2 + icyBorder)  ||
                x > (cxList2 + icyBorder + cxList1)))  ||
            lCaptureFlags)
        {
            SetCursor (LoadCursor (NULL, MAKEINTRESOURCE(IDC_SIZEWE)));
        }

        if (lCaptureFlags == 1)
        {
            int cxList2New;


            x = (x > (cxList1 + cxList2 - cxVScroll) ?
                    (cxList1 + cxList2 - cxVScroll) : x);
            x = (x < cxVScroll ? cxVScroll : x);

            cxList2New = (int) (cxList2 + x - xCapture);

            SetWindowPos(
                ghwndList2,
                GetNextWindow (ghwndList2, GW_HWNDPREV),
                0,
                icyButton,
                cxList2New,
                cyWnd,
                SWP_SHOWWINDOW
                );

            SetWindowPos(
                ghwndList3,
                GetNextWindow (ghwndList3, GW_HWNDPREV),
                cxList2New + icyBorder,
                icyButton + (cyWnd - icyBorder) / 2 + icyBorder,
                (int) (cxList1 - (x - xCapture)),
                (cyWnd - icyBorder) / 2,
                SWP_SHOWWINDOW
                );

            SetWindowPos(
                ghwndList1,
                GetNextWindow (ghwndList1, GW_HWNDPREV),
                cxList2New + icyBorder,
                icyButton,
                (int) (cxList1 - (x - xCapture)),
                (cyWnd - icyBorder) / 2,
                SWP_SHOWWINDOW
                );

        }
        else if (lCaptureFlags == 2)
        {
            int cxList1New;


            x = (x < (cxList2 + cxVScroll) ?  (cxList2 + cxVScroll) : x);
            x = (x > (cxWnd - cxVScroll) ?  (cxWnd - cxVScroll) : x);

            cxList1New = (int) (cxList1 + x - xCapture);

            SetWindowPos(
                ghwndList1,
                GetNextWindow (ghwndList1, GW_HWNDPREV),
                (int) cxList2 + icyBorder,
                icyButton,
                cxList1New,
                (cyWnd - icyBorder) / 2,
                SWP_SHOWWINDOW
                );

            SetWindowPos(
                ghwndList3,
                GetNextWindow (ghwndList3, GW_HWNDPREV),
                (int) cxList2 + icyBorder,
                icyButton + (cyWnd - icyBorder) / 2 + icyBorder,
                cxList1New,
                (cyWnd - icyBorder) / 2,
                SWP_SHOWWINDOW
                );

            SetWindowPos(
                ghwndEdit,
                GetNextWindow (ghwndEdit, GW_HWNDPREV),
                (int) (cxList1New + cxList2) + 2*icyBorder,
                icyButton,
                (int)cxWnd - (cxList1New + (int)cxList2 + 2*icyBorder),
                cyWnd,
                SWP_SHOWWINDOW
                );
        }

        break;
    }
    case WM_LBUTTONDOWN:
    {
        if ((int)((short)HIWORD(lParam)) > icyButton)
        {
            xCapture = (LONG)LOWORD(lParam);

            if (xCapture > (cxList2 + icyBorder)  &&
                xCapture < (cxList2 + icyBorder + cxList1))
            {
                break;
            }

            SetCapture (hwnd);

            lCaptureFlags = ((xCapture < cxList1 + cxList2) ? 1 : 2);
        }

        break;
    }
    case WM_LBUTTONUP:
    {
        if (lCaptureFlags)
        {
            POINT p;
            LONG  x;
            RECT  rect = { 0, icyButton, 2000, 2000 };

            GetCursorPos (&p);
            MapWindowPoints (HWND_DESKTOP, hwnd, &p, 1);
            x = (LONG) p.x;

            ReleaseCapture();

            if (lCaptureFlags == 1)
            {
                x = (x < cxVScroll ? cxVScroll : x);
                x = (x > (cxList1 + cxList2 - cxVScroll) ?
                    (cxList1 + cxList2 - cxVScroll) : x);

                cxList2 = cxList2 + (x - xCapture);
                cxList1 = cxList1 - (x - xCapture);

                rect.right = (int) (cxList1 + cxList2) + icyBorder;
            }
            else
            {
                x = (x < (cxList2 + cxVScroll) ?
                    (cxList2 + cxVScroll) : x);
                x = (x > (cxWnd - cxVScroll) ?
                    (cxWnd - cxVScroll) : x);

                cxList1 = cxList1 + (x - xCapture);

                rect.left = (int)cxList2 + icyBorder;
            }

            lCaptureFlags = 0;

            InvalidateRect (hwnd, &rect, TRUE);
        }

        break;
    }
    case WM_SIZE:
    {
        if (wParam != SIZE_MINIMIZED)
        {
            LONG width = (LONG)LOWORD(lParam);


            //
            // Adjust globals based on new size
            //

            cxWnd = (cxWnd ? cxWnd : 1); // avoid div by 0

            cxList1 = (cxList1 * width) / cxWnd;
            cxList2 = (cxList2 * width) / cxWnd;
            cxWnd = width;
            cyWnd = ((int)HIWORD(lParam)) - icyButton;


            //
            // Now reposition the child windows
            //

            SetWindowPos(
                ghwndList2,
                GetNextWindow (ghwndList2, GW_HWNDPREV),
                0,
                icyButton,
                (int) cxList2,
                cyWnd,
                SWP_SHOWWINDOW
                );

            SetWindowPos(
                ghwndList1,
                GetNextWindow (ghwndList1, GW_HWNDPREV),
                (int) cxList2 + icyBorder,
                icyButton,
                (int) cxList1,
                (cyWnd - icyBorder) / 2,
                SWP_SHOWWINDOW
                );

            SetWindowPos(
                ghwndList3,
                GetNextWindow (ghwndList3, GW_HWNDPREV),
                (int) cxList2 + icyBorder,
                icyButton + (cyWnd - icyBorder) / 2 + icyBorder,
                (int) cxList1,
                (cyWnd - icyBorder) / 2,
                SWP_SHOWWINDOW
                );

            SetWindowPos(
                ghwndEdit,
                GetNextWindow (ghwndEdit, GW_HWNDPREV),
                (int) (cxList1 + cxList2) + 2*icyBorder,
                icyButton,
                (int)width - ((int)(cxList1 + cxList2) + 2*icyBorder),
                cyWnd,
                SWP_SHOWWINDOW
                );

            InvalidateRect (hwnd, NULL, TRUE);
        }

        break;
    }
    case WM_PAINT:
    {
        PAINTSTRUCT ps;


        BeginPaint (hwnd, &ps);

        if (IsIconic (hwnd))
        {
            DrawIcon (ps.hdc, 0, 0, hIcon);
        }
        else
        {
            FillRect (ps.hdc, &ps.rcPaint, GetStockObject (LTGRAY_BRUSH));
#ifdef WIN32
            MoveToEx (ps.hdc, 0, 0, NULL);
#else
            MoveTo (ps.hdc, 0, 0);
#endif
            LineTo (ps.hdc, 5000, 0);

#ifdef WIN32
            MoveToEx (ps.hdc, 0, icyButton - 4, NULL);
#else
            MoveTo (ps.hdc, 0, icyButton - 4);
#endif
            LineTo (ps.hdc, 5000, icyButton - 4);
        }

        EndPaint (hwnd, &ps);

        break;
    }
    case WM_TIMER:

        //
        // Enter alterable wait state to cause any queued APCs to fire
        // (might be able to use SleepEx() instead)
        //

        WaitForSingleObjectEx (hTimerEvent, 0, TRUE);
        break;

    case WM_CLOSE:
    {
        BOOL bAutoShutdown = FALSE;
        RECT rect;


        //
        //
        //

        KillTimer (hwnd, TIMER_ID);
        CloseHandle (hTimerEvent);

        //
        // Give user chance to cancel and auto-shutdown any init instances
        //

        if (giCurrNumStartups > 0)
        {
            int iResult;


            if ((iResult = MessageBox(
                    hwnd,
                    "Cleanup existing Startup instances? (recommended)",
                    "SockEye closing",
                    MB_YESNOCANCEL

                    )) == IDYES)
            {
                bShowParams = FALSE;

                while (giCurrNumStartups > 0)
                {
                    FuncDriver (ws_WSACleanup);
                }
            }
            else if (iResult == IDCANCEL)
            {
                break;
            }
        }


        //
        // Save defaults in ini file
        //

        {
            char buf[32];
            typedef struct _DEF_VALUE2
            {
                char far    *lpszEntry;
                ULONG_PTR   dwValue;

            } DEF_VALUE2;

            DEF_VALUE2 aDefVals[] =
            {
                { "BufSize",            dwBigBufSize },
                { "UserButton1",        aUserButtonFuncs[0] },
                { "UserButton2",        aUserButtonFuncs[1] },
                { "UserButton3",        aUserButtonFuncs[2] },
                { "UserButton4",        aUserButtonFuncs[3] },
                { "UserButton5",        aUserButtonFuncs[4] },
                { "UserButton6",        aUserButtonFuncs[5] },
                { "Initialized",        1 },
                { gszAddressFamily,     gdwDefAddrFamily },
                { gszSocketType,        gdwDefSocketType },
                { gszProtocol,          gdwDefProtocol },
                { gszProtocolNumber,    gdwDefProtoNum },
                { gszPortNumber,        gdwDefPortNum },
                { gszIoctlCommand,      gdwDefIoctlCmd },
                { gszSendFlags,         gdwDefSendFlags },
                { gszRecvFlags,         gdwDefRecvFlags },
                { gszWSASocketFlags,    gdwDefWSASocketFlags },
                { NULL, 0 },
                { gszHostName,          (ULONG_PTR) gszDefHostName },
                { gszProtocolName,      (ULONG_PTR) gszDefProtoName },
                { gszServiceName,       (ULONG_PTR) gszDefServName },
                { "UserButton1Text",    (ULONG_PTR) &aUserButtonsText[0] },
                { "UserButton2Text",    (ULONG_PTR) &aUserButtonsText[1] },
                { "UserButton3Text",    (ULONG_PTR) &aUserButtonsText[2] },
                { "UserButton4Text",    (ULONG_PTR) &aUserButtonsText[3] },
                { "UserButton5Text",    (ULONG_PTR) &aUserButtonsText[4] },
                { "UserButton6Text",    (ULONG_PTR) &aUserButtonsText[5] },
                { NULL, 0 }
            };

            int i;

            for (i = 0; aDefVals[i].lpszEntry; i++)
            {
                sprintf (buf, "%lx", aDefVals[i].dwValue);

                WriteProfileString(
                    gszSockEye,
                    aDefVals[i].lpszEntry,
                    buf
                    );
            }

            i++;

            for (; aDefVals[i].lpszEntry; i++)
            {
                WriteProfileString(
                    gszSockEye,
                    aDefVals[i].lpszEntry,
                    (LPCSTR) aDefVals[i].dwValue
                    );
            }


            //
            // Save the window dimensions (if iconic then don't bother)
            //

            if (!IsIconic (hwnd))
            {
                if (IsZoomed (hwnd))
                {
                    strcpy (buf, "max");
                }
                else
                {
                    GetWindowRect (hwnd, &rect);

                    sprintf(
                        buf,
                        "%d,%d,%d,%d",
                        rect.left,
                        rect.top,
                        rect.right,
                        rect.bottom
                        );
                }

                WriteProfileString(
                    gszSockEye,
                    "Position",
                    (LPCSTR) buf
                    );

                sprintf (buf, "%ld,%ld,%ld", cxList2, cxList1, cxWnd);

                WriteProfileString(
                    gszSockEye,
                    "ControlRatios",
                    (LPCSTR) buf
                    );
            }
        }

        if (hLogFile)
        {
            fclose (hLogFile);
        }
        DestroyIcon (hIcon);
        MyFree (pBigBuf);
        DeleteObject (hFont);
        DeleteObject (hFont2);
        PostQuitMessage (0);
        break;

    }
    } //switch

    return FALSE;
}


INT_PTR
CALLBACK
AboutDlgProc(
    HWND hwnd,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    switch (msg)
    {
    case WM_INITDIALOG:

        if (lParam)
        {
            SetDlgItemText (hwnd, IDC_EDIT1, (LPCSTR) lParam);
        }

        break;

    case WM_COMMAND:

        switch (LOWORD(wParam))
        {
        case IDOK:

            EndDialog (hwnd, 0);
            break;
        }
        break;

#ifdef WIN32
    case WM_CTLCOLORSTATIC:

        SetBkColor ((HDC) wParam, RGB (192,192,192));
        return (INT_PTR) GetStockObject (LTGRAY_BRUSH);
#else
    case WM_CTLCOLOR:
    {
        if (HIWORD(lParam) == CTLCOLOR_STATIC)
        {
            SetBkColor ((HDC) wParam, RGB (192,192,192));
            return (INT_PTR) GetStockObject (LTGRAY_BRUSH);
        }
        break;
    }
#endif
    case WM_PAINT:
    {
        PAINTSTRUCT ps;

        BeginPaint (hwnd, &ps);
        FillRect (ps.hdc, &ps.rcPaint, GetStockObject (LTGRAY_BRUSH));
        EndPaint (hwnd, &ps);

        break;
    }
    }

    return FALSE;
}


void
FAR
ShowStr(
    LPCSTR format,
    ...
    )
{
    char buf[256];
    va_list ap;


    va_start(ap, format);
    vsprintf (buf, format, ap);

    if (hLogFile)
    {
        fprintf (hLogFile, "%s\n", buf);
    }

    strcat (buf, "\r\n");


    //
    // Insert text at end
    //

#ifdef WIN32
    SendMessage (ghwndEdit, EM_SETSEL, (WPARAM)0xfffffffd, (LPARAM)0xfffffffe);
#else
    SendMessage(
        ghwndEdit,
        EM_SETSEL,
        (WPARAM)0,
        (LPARAM) MAKELONG(0xfffd,0xfffe)
        );
#endif

    SendMessage (ghwndEdit, EM_REPLACESEL, 0, (LPARAM) buf);


#ifdef WIN32

    //
    // Scroll to end of text
    //

    SendMessage (ghwndEdit, EM_SCROLLCARET, 0, 0);
#endif

    va_end(ap);
}


INT_PTR
CALLBACK
ParamsDlgProc(
    HWND hwnd,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    DWORD  i;

    typedef struct _DLG_INST_DATA
    {
        PFUNC_PARAM_HEADER pParamsHeader;

        LRESULT lLastSel;

        char szComboText[MAX_STRING_PARAM_SIZE];

    } DLG_INST_DATA, *PDLG_INST_DATA;

    PDLG_INST_DATA pDlgInstData = (PDLG_INST_DATA)
        GetWindowLongPtr (hwnd, DWLP_USER);

    static int icxList2, icyList2, icyEdit1;


    switch (msg)
    {
    case WM_INITDIALOG:
    {
        //
        // Alloc a dlg instance data struct, init it, & save a ptr to it
        //

        pDlgInstData = (PDLG_INST_DATA) MyAlloc (sizeof(DLG_INST_DATA));

        // BUGBUG if (!pDlgInstData)

        pDlgInstData->pParamsHeader = (PFUNC_PARAM_HEADER) lParam;
        pDlgInstData->lLastSel = -1;

        SetWindowLongPtr (hwnd, DWLP_USER, (LPARAM) pDlgInstData);


        //
        // Stick all the param names in the listbox, & for each PT_DWORD
        // param save it's default value
        //

        for (i = 0; i < pDlgInstData->pParamsHeader->dwNumParams; i++)
        {
            SendDlgItemMessage(
                hwnd,
                IDC_LIST1,
                LB_INSERTSTRING,
                (WPARAM) -1,
                (LPARAM) pDlgInstData->pParamsHeader->aParams[i].szName
                );

            if (pDlgInstData->pParamsHeader->aParams[i].dwType == PT_DWORD)
            {
                pDlgInstData->pParamsHeader->aParams[i].u.dwDefValue = (ULONG_PTR)
                    pDlgInstData->pParamsHeader->aParams[i].dwValue;
            }
        }


        //
        // Set the dlg title as appropriate
        //

// help        if (pDlgInstData->pParamsHeader->FuncIndex == DefValues)
// help        {
// help            EnableWindow (GetDlgItem (hwnd, IDC_TB_HELP), FALSE);
// help        }

        SetWindowText(
            hwnd,
            aFuncNames[pDlgInstData->pParamsHeader->FuncIndex]
            );


        //
        // Limit the max text length for the combobox's edit field
        // (NOTE: A combobox ctrl actually has two child windows: a
        // edit ctrl & a listbox.  We need to get the hwnd of the
        // child edit ctrl & send it the LIMITTEXT msg.)
        //

        {
            HWND hwndChild =
                GetWindow (GetDlgItem (hwnd, IDC_COMBO1), GW_CHILD);


            while (hwndChild)
            {
                char buf[8];


                GetClassName (hwndChild, buf, 7);

                if (_stricmp (buf, "edit") == 0)
                {
                    break;
                }

                hwndChild = GetWindow (hwndChild, GW_HWNDNEXT);
            }

            SendMessage(
                hwndChild,
                EM_LIMITTEXT,
                (WPARAM) (gbWideStringParams ?
                    (MAX_STRING_PARAM_SIZE/2 - 1) : MAX_STRING_PARAM_SIZE - 1),
                0
                );
        }

        {
            RECT    rect;


            GetWindowRect (GetDlgItem (hwnd, IDC_LIST2), &rect);

            SetWindowPos(
                GetDlgItem (hwnd, IDC_LIST2),
                NULL,
                0,
                0,
                0,
                0,
                SWP_NOMOVE | SWP_NOZORDER
                );

            icxList2 = rect.right - rect.left;
            icyList2 = rect.bottom - rect.top;

            GetWindowRect (GetDlgItem (hwnd, 58), &rect);

            icyEdit1 = icyList2 - (rect.bottom - rect.top);
        }

        SendDlgItemMessage(
            hwnd,
            IDC_EDIT1,
            WM_SETFONT,
            (WPARAM) ghFixedFont,
            0
            );

        break;
    }
    case WM_COMMAND:
    {
        LRESULT   lLastSel      = pDlgInstData->lLastSel;
        char far *lpszComboText = pDlgInstData->szComboText;
        PFUNC_PARAM_HEADER pParamsHeader = pDlgInstData->pParamsHeader;


        switch (LOWORD(wParam))
        {
        case IDC_EDIT1:
        {
            if (HIWORD(wParam) == EN_CHANGE)
            {
                //
                // Don't allow the user to enter characters other than
                // 0-9, a-f, or A-F in the edit control (do this by
                // hiliting other letters and cutting them).
                //

                HWND    hwndEdit = GetDlgItem (hwnd, IDC_EDIT1);
                DWORD   dwLength, j;
                BYTE   *p;


                dwLength = (DWORD) GetWindowTextLength (hwndEdit);

                if (dwLength  &&  (p = MyAlloc (dwLength + 1)))
                {
                    GetWindowText (hwndEdit, p, dwLength + 1);

                    for (i = j = 0; i < dwLength ; i++, j++)
                    {
                        if (aHex[p[i]] == 255)
                        {
                            SendMessage(
                                hwndEdit,
                                EM_SETSEL,
                                (WPARAM) j,
                                (LPARAM) j + 1  // 0xfffffffe
                                );

                            SendMessage (hwndEdit, EM_REPLACESEL, 0, (LPARAM) "");
                            SendMessage (hwndEdit, EM_SCROLLCARET, 0, 0);

                            j--;
                        }
                    }

                    MyFree (p);
                }
            }

            break;
        }
        case IDOK:

// BUGBUG if gbWideStringParams convert ascii -> unicode

            if (lLastSel != -1)
            {
                //
                // Save val of currently selected param
                //

                char buf[MAX_STRING_PARAM_SIZE];


                i = GetDlgItemText (hwnd, IDC_COMBO1, buf, MAX_STRING_PARAM_SIZE-1);

                switch (pParamsHeader->aParams[lLastSel].dwType)
                {
                case PT_STRING:
                {
                    LRESULT lComboSel;


                    lComboSel = SendDlgItemMessage(
                        hwnd,
                        IDC_COMBO1,
                        CB_GETCURSEL,
                        0,
                        0
                        );

                    if (lComboSel == 0) // "NULL pointer"
                    {
                        pParamsHeader->aParams[lLastSel].dwValue = 0;
                    }
                    else if (lComboSel == 2) // "Invalid string pointer"
                    {
                        pParamsHeader->aParams[lLastSel].dwValue = (ULONG_PTR)
                            -1;
                    }
                    else // "Valid string pointer"
                    {
                        strncpy(
                            pParamsHeader->aParams[lLastSel].u.buf,
                            buf,
                            MAX_STRING_PARAM_SIZE - 1
                            );

                        pParamsHeader->aParams[lLastSel].u.buf[MAX_STRING_PARAM_SIZE-1] = 0;

                        pParamsHeader->aParams[lLastSel].dwValue = (ULONG_PTR)
                            pParamsHeader->aParams[lLastSel].u.buf;
                    }

                    break;
                }
                case PT_POINTER:
                {
                    //
                    // If there is any text in the "Buffer byte editor"
                    // window then retrieve it, convert it to hexadecimal,
                    // and copy it to the buffer
                    //

                    DWORD     dwLength;
                    BYTE     *p, *p2,
                             *pBuf = pParamsHeader->aParams[lLastSel].u.ptr;
                    HWND      hwndEdit = GetDlgItem (hwnd,IDC_EDIT1);


                    dwLength = (DWORD) GetWindowTextLength (hwndEdit);

                    if (dwLength  &&  (p = MyAlloc (dwLength + 1)))
                    {
                        GetWindowText (hwndEdit, p, dwLength + 1);
                        SetWindowText (hwndEdit, "");

                        p2 = p;

                        p[dwLength] = (BYTE) '0';
                        dwLength = (dwLength + 1) & 0xfffffffe;

                        for (i = 0; i < dwLength; i++, i++)
                        {
                            BYTE b;

                            b = aHex[*p] << 4;
                            p++;

                            b |= aHex[*p];
                            p++;

                            *pBuf = b;
                            pBuf++;
                        }

                        MyFree (p2);
                    }

                    // fall thru to code below
                }
                case PT_DWORD:
                case PT_FLAGS:
                case PT_ORDINAL:
                case PT_WSAPROTOCOLINFO:
                case PT_QOS:
                case PT_PTRNOEDIT:
                {
                    if (!sscanf(
                            buf,
                            "%08lx",
                            &pParamsHeader->aParams[lLastSel].dwValue
                            ))
                    {
                        //
                        // Default to 0
                        //

                        pParamsHeader->aParams[lLastSel].dwValue = 0;
                    }

                    break;
                }
                } // switch
            }


            //
            // Convert any unicode string params as appropriate
            //

            if (gbWideStringParams)
            {
                DWORD       dwNumParams = pParamsHeader->dwNumParams, i;
                PFUNC_PARAM pParam = pParamsHeader->aParams;


                for (i = 0; i < dwNumParams; i++)
                {
                    if (pParam->dwType == PT_STRING &&
                        pParam->dwValue != 0 &&
                        pParam->dwValue != 0xffffffff)
                    {
                        int    len = lstrlenA ((char *) pParam->dwValue) + 1;
                        WCHAR  buf[MAX_STRING_PARAM_SIZE/2];


                        MultiByteToWideChar(
                            GetACP(),
                            MB_PRECOMPOSED,
                            (LPCSTR) pParam->dwValue,
                            (len > MAX_STRING_PARAM_SIZE/2 ?
                                MAX_STRING_PARAM_SIZE/2 - 1 : -1),
                            buf,
                            MAX_STRING_PARAM_SIZE/2
                            );

                         buf[MAX_STRING_PARAM_SIZE/2 - 1] = 0;

                         lstrcpyW ((WCHAR *) pParam->dwValue, buf);
                    }

                    pParam++;
                }
            }

            MyFree (pDlgInstData);
            EndDialog (hwnd, TRUE);
            break;

        case IDCANCEL:

            MyFree (pDlgInstData);
            EndDialog (hwnd, FALSE);
            break;

        case IDC_LIST1:

#ifdef WIN32
            if (HIWORD(wParam) == LBN_SELCHANGE)
#else
            if (HIWORD(lParam) == LBN_SELCHANGE)
#endif
            {
                char buf[MAX_STRING_PARAM_SIZE] = "";
                LPCSTR lpstr = buf;
                LRESULT lSel =
                    SendDlgItemMessage (hwnd, IDC_LIST1, LB_GETCURSEL, 0, 0);


                if (lLastSel != -1)
                {
                    //
                    // Save the old param value
                    //

                    i = GetDlgItemText(
                        hwnd,
                        IDC_COMBO1,
                        buf,
                        MAX_STRING_PARAM_SIZE - 1
                        );

                    switch (pParamsHeader->aParams[lLastSel].dwType)
                    {
                    case PT_STRING:
                    {
                        LRESULT lComboSel;


                        lComboSel = SendDlgItemMessage(
                            hwnd,
                            IDC_COMBO1,
                            CB_GETCURSEL,
                            0,
                            0
                            );

                        if (lComboSel == 0) // "NULL pointer"
                        {
                            pParamsHeader->aParams[lLastSel].dwValue = (DWORD)0;
                        }
                        else if (lComboSel == 2) // "Invalid string pointer"
                        {
                            pParamsHeader->aParams[lLastSel].dwValue = (DWORD)
                                0xffffffff;
                        }
                        else // "Valid string pointer" or no sel
                        {
                            strncpy(
                                pParamsHeader->aParams[lLastSel].u.buf,
                                buf,
                                MAX_STRING_PARAM_SIZE - 1
                                );

                            pParamsHeader->aParams[lLastSel].u.buf[MAX_STRING_PARAM_SIZE - 1] = 0;

                            pParamsHeader->aParams[lLastSel].dwValue = (ULONG_PTR)
                                pParamsHeader->aParams[lLastSel].u.buf;
                        }

                        break;
                    }
                    case PT_POINTER:
                    {
                        //
                        // If there is any text in the "Buffer byte editor"
                        // window then retrieve it, convert it to hexadecimal,
                        // and copy it to the buffer
                        //

                        DWORD     dwLength;
                        BYTE     *p, *p2,
                                 *pBuf = pParamsHeader->aParams[lLastSel].u.ptr;
                        HWND      hwndEdit = GetDlgItem (hwnd,IDC_EDIT1);


                        dwLength = (DWORD) GetWindowTextLength (hwndEdit);

                        if (dwLength  &&  (p = MyAlloc (dwLength + 1)))
                        {
                            GetWindowText (hwndEdit, p, dwLength + 1);
                            SetWindowText (hwndEdit, "");

                            p2 = p;

                            p[dwLength] = (BYTE) '0';
                            dwLength = (dwLength + 1) & 0xfffffffe;

                            for (i = 0; i < dwLength; i+= 2)
                            {
                                BYTE b;

                                b = aHex[*p] << 4;
                                p++;

                                b |= aHex[*p];
                                p++;

                                *pBuf = b;
                                pBuf++;
                            }

                            MyFree (p2);
                        }

                        // fall thru to code below
                    }
                    case PT_DWORD:
                    case PT_FLAGS:
                    case PT_ORDINAL:
                    case PT_WSAPROTOCOLINFO:
                    case PT_QOS:
                    case PT_PTRNOEDIT:
                    {
                        if (!sscanf(
                                buf,
                                "%08lx",
                                &pParamsHeader->aParams[lLastSel].dwValue
                                ))
                        {
                            //
                            // Default to 0
                            //

                            pParamsHeader->aParams[lLastSel].dwValue = 0;
                        }

                        break;
                    }
                    } // switch
                }

                SendDlgItemMessage (hwnd, IDC_LIST2, LB_RESETCONTENT, 0, 0);
                SendDlgItemMessage (hwnd, IDC_COMBO1, CB_RESETCONTENT, 0, 0);

                {
                    int         icxL2, icyL2, icxE1, icyE1;
                    char FAR   *pszS1, *pszS2;
                    static char szBitFlags[] = "Bit flags:";
                    static char szOrdinalValues[] = "Ordinal values:";
                    static char szBufByteEdit[] =
                                    "Buffer byte editor (use 0-9, a-f, A-F)";

                    switch (pParamsHeader->aParams[lSel].dwType)
                    {
                    case PT_DWORD:
                    case PT_STRING:
                    case PT_WSAPROTOCOLINFO:
                    case PT_QOS:
                    case PT_PTRNOEDIT:

                        icxL2 = icyL2 = icxE1 = icxE1 = 0;
                        pszS1 = pszS2 = NULL;
                        break;

                    case PT_FLAGS:

                        icxL2 = icxList2;
                        icyL2 = icyList2;
                        icxE1 = icyE1 = 0;
                        pszS1 = szBitFlags;
                        pszS2 = NULL;
                        break;

                    case PT_POINTER:

                        icxL2 = icyL2 = 0;
                        icxE1 = icxList2;
                        icyE1 = icyEdit1;;
                        pszS1 = szBufByteEdit;
                        pszS2 = gszEnterAs;
                        break;

                    case PT_ORDINAL:

                        icxL2 = icxList2;
                        icyL2 = icyList2;
                        icxE1 = icyE1 = 0;
                        pszS1 = szOrdinalValues;
                        pszS2 = NULL;
                        break;
                    }

                    SetWindowPos(
                        GetDlgItem (hwnd, IDC_LIST2),
                        NULL,
                        0,
                        0,
                        icxL2,
                        icyL2,
                        SWP_NOMOVE | SWP_NOZORDER
                        );

                    SetWindowPos(
                        GetDlgItem (hwnd, IDC_EDIT1),
                        NULL,
                        0,
                        0,
                        icxE1,
                        icyE1,
                        SWP_NOMOVE | SWP_NOZORDER
                        );

                    SetDlgItemText (hwnd, 57, pszS1);
                    SetDlgItemText (hwnd, 58, pszS2);
                }

                switch (pParamsHeader->aParams[lSel].dwType)
                {
                case PT_STRING:
                {
                    char * aszOptions[] =
                    {
                        "NULL pointer",
                        "Valid string pointer",
                        "Invalid string pointer"
                    };


                    for (i = 0; i < 3; i++)
                    {
                        SendDlgItemMessage(
                            hwnd,
                            IDC_COMBO1,
                            CB_INSERTSTRING,
                            (WPARAM) -1,
                            (LPARAM) aszOptions[i]
                            );
                    }

                    if (pParamsHeader->aParams[lSel].dwValue == 0)
                    {
                        i = 0;
                        buf[0] = 0;
                    }
                    else if (pParamsHeader->aParams[lSel].dwValue != 0xffffffff)
                    {
                        i = 1;
                        lpstr = (LPCSTR) pParamsHeader->aParams[lSel].dwValue;
                    }
                    else
                    {
                        i = 2;
                        buf[0] = 0;
                    }

                    SendDlgItemMessage(
                        hwnd,
                        IDC_COMBO1,
                        CB_SETCURSEL,
                        (WPARAM) i,
                        0
                        );

                    break;
                }
                case PT_POINTER:
                case PT_WSAPROTOCOLINFO:
                case PT_QOS:
                case PT_PTRNOEDIT:
                {
                    SendDlgItemMessage(
                        hwnd,
                        IDC_COMBO1,
                        CB_INSERTSTRING,
                        (WPARAM) -1,
                        (LPARAM) "00000000"
                        );

                    sprintf(
                        buf,
                        "%08lx (valid pointer)",
                        pParamsHeader->aParams[lSel].u.dwDefValue
                        );

                    SendDlgItemMessage(
                        hwnd,
                        IDC_COMBO1,
                        CB_INSERTSTRING,
                        (WPARAM) -1,
                        (LPARAM) buf
                        );

                    SendDlgItemMessage(
                        hwnd,
                        IDC_COMBO1,
                        CB_INSERTSTRING,
                        (WPARAM) -1,
                        (LPARAM) "ffffffff"
                        );

                    sprintf(
                        buf,
                        "%08lx",
                        pParamsHeader->aParams[lSel].dwValue
                        );

                    break;
                }
                case PT_DWORD:
                {
                    SendDlgItemMessage(
                        hwnd,
                        IDC_COMBO1,
                        CB_INSERTSTRING,
                        (WPARAM) -1,
                        (LPARAM) "0000000"
                        );

                    if (pParamsHeader->aParams[lSel].u.dwDefValue)
                    {
                        //
                        // Add the default val string to the combo
                        //

                        sprintf(
                            buf,
                            "%08lx",
                            pParamsHeader->aParams[lSel].u.dwDefValue
                            );

                        SendDlgItemMessage(
                            hwnd,
                            IDC_COMBO1,
                            CB_INSERTSTRING,
                            (WPARAM) -1,
                            (LPARAM) buf
                            );
                    }

                    SendDlgItemMessage(
                        hwnd,
                        IDC_COMBO1,
                        CB_INSERTSTRING,
                        (WPARAM) -1,
                        (LPARAM) "ffffffff"
                        );

                    sprintf(
                        buf,
                        "%08lx",
                        pParamsHeader->aParams[lSel].dwValue
                        );

                    break;
                }
                case PT_FLAGS:
                {
                    //
                    // Stick the bit flag strings in the list box
                    //

                    PLOOKUP pLookup = (PLOOKUP)
                        pParamsHeader->aParams[lSel].u.pLookup;

                    for (i = 0; pLookup[i].dwVal != 0xffffffff; i++)
                    {
                        SendDlgItemMessage(
                            hwnd,
                            IDC_LIST2,
                            LB_INSERTSTRING,
                            (WPARAM) -1,
                            (LPARAM) pLookup[i].lpszVal
                            );

                        if (pParamsHeader->aParams[lSel].dwValue &
                            pLookup[i].dwVal)
                        {
                            SendDlgItemMessage(
                                hwnd,
                                IDC_LIST2,
                                LB_SETSEL,
                                (WPARAM) TRUE,
                                (LPARAM) MAKELPARAM((WORD)i,0)
                                );
                        }
                    }

                    SendDlgItemMessage(
                        hwnd,
                        IDC_COMBO1,
                        CB_INSERTSTRING,
                        (WPARAM) -1,
                        (LPARAM) "select none"
                        );

                    SendDlgItemMessage(
                        hwnd,
                        IDC_COMBO1,
                        CB_INSERTSTRING,
                        (WPARAM) -1,
                        (LPARAM) "select all"
                        );

                    sprintf(
                        buf,
                        "%08lx",
                        pParamsHeader->aParams[lSel].dwValue
                        );

                    break;
                }
                case PT_ORDINAL:
                {
                    //
                    // Stick the bit flag strings in the list box
                    //

                    HWND hwndList2 = GetDlgItem (hwnd, IDC_LIST2);
                    PLOOKUP pLookup = (PLOOKUP)
                        pParamsHeader->aParams[lSel].u.pLookup;

                    for (i = 0; pLookup[i].dwVal != 0xffffffff; i++)
                    {
                        SendMessage(
                            hwndList2,
                            LB_INSERTSTRING,
                            (WPARAM) -1,
                            (LPARAM) pLookup[i].lpszVal
                            );

                        if (pParamsHeader->aParams[lSel].dwValue ==
                            pLookup[i].dwVal)
                        {
                            SendMessage(
                                hwndList2,
                                LB_SETSEL,
                                (WPARAM) TRUE,
                                (LPARAM) MAKELPARAM((WORD)i,0)
                                );
                        }
                    }

                    SendDlgItemMessage(
                        hwnd,
                        IDC_COMBO1,
                        CB_INSERTSTRING,
                        (WPARAM) -1,
                        (LPARAM) (char far *) "select none"
                        );

                    wsprintf(
                        buf,
                        "%08lx",
                        pParamsHeader->aParams[lSel].dwValue
                        );

                    break;
                }
                } //switch

                SetDlgItemText (hwnd, IDC_COMBO1, lpstr);

                pDlgInstData->lLastSel = lSel;
            }
            break;

        case IDC_LIST2:

#ifdef WIN32
            if (HIWORD(wParam) == LBN_SELCHANGE)
#else
            if (HIWORD(lParam) == LBN_SELCHANGE)
#endif
            {
                PLOOKUP pLookup = (PLOOKUP)
                    pParamsHeader->aParams[lLastSel].u.pLookup;
                char buf[16];
                DWORD dwValue = 0;
                int far *ai;
                LONG i;
                LRESULT lSelCount =
                    SendDlgItemMessage (hwnd, IDC_LIST2, LB_GETSELCOUNT, 0, 0);


                if (lSelCount)
                {
                    ai = (int far *) MyAlloc ((size_t)lSelCount * sizeof(int));

                    SendDlgItemMessage(
                        hwnd,
                        IDC_LIST2,
                        LB_GETSELITEMS,
                        (WPARAM) lSelCount,
                        (LPARAM) ai
                        );

                    if (pParamsHeader->aParams[lLastSel].dwType == PT_FLAGS)
                    {
                        for (i = 0; i < lSelCount; i++)
                        {
                            dwValue |= pLookup[ai[i]].dwVal;
                        }
                    }
                    else // if (.dwType == PT_ORDINAL)
                    {
                        if (lSelCount == 1)
                        {
                            dwValue = pLookup[ai[0]].dwVal;
                        }
                        else if (lSelCount == 2)
                        {
                            //
                            // Figure out which item we need to de-select,
                            // since we're doing ords & only want 1 item
                            // selected at a time
                            //

                            GetDlgItemText (hwnd, IDC_COMBO1, buf, 16);

                            if (sscanf (buf, "%lx", &dwValue))
                            {
                                if (pLookup[ai[0]].dwVal == dwValue)
                                {
                                    SendDlgItemMessage(
                                        hwnd,
                                        IDC_LIST2,
                                        LB_SETSEL,
                                        0,
                                        (LPARAM) ai[0]
                                        );

                                    dwValue = pLookup[ai[1]].dwVal;
                                }
                                else
                                {
                                    SendDlgItemMessage(
                                        hwnd,
                                        IDC_LIST2,
                                        LB_SETSEL,
                                        0,
                                        (LPARAM) ai[1]
                                        );

                                    dwValue = pLookup[ai[0]].dwVal;
                                }
                            }
                            else
                            {
                                // BUGBUG de-select items???

                                dwValue = 0;
                            }
                        }
                    }

                    MyFree (ai);
                }

                sprintf (buf, "%08lx", dwValue);
                SetDlgItemText (hwnd, IDC_COMBO1, buf);
            }
            break;

        case IDC_COMBO1:

#ifdef WIN32
            switch (HIWORD(wParam))
#else
            switch (HIWORD(lParam))
#endif
            {
            case CBN_SELCHANGE:
            {
                LRESULT lSel =
                    SendDlgItemMessage (hwnd, IDC_COMBO1, CB_GETCURSEL, 0, 0);


                switch (pParamsHeader->aParams[lLastSel].dwType)
                {
                case PT_POINTER:
                case PT_PTRNOEDIT:
                {
                    if (lSel == 1)
                    {
                        //
                        // Strip off the "(valid pointer)" in the edit ctrl
                        //

                        wsprintf(
                            lpszComboText,
                            "%08lx",
                            pParamsHeader->aParams[lLastSel].u.ptr
                            );

                        PostMessage (hwnd, WM_USER+55, 0, 0);
                    }

                    break;
                }
                case PT_FLAGS:
                {
                    BOOL bSelect = (lSel ? TRUE : FALSE);

                    SendDlgItemMessage(
                        hwnd,
                        IDC_LIST2,
                        LB_SETSEL,
                        (WPARAM) bSelect,
                        (LPARAM) -1
                        );

                    if (bSelect)
                    {
                        PLOOKUP pLookup = (PLOOKUP)
                            pParamsHeader->aParams[lLastSel].u.pLookup;
                        DWORD dwValue = 0;
                        int far *ai;
                        LONG i;
                        LRESULT lSelCount =
                            SendDlgItemMessage (hwnd, IDC_LIST2, LB_GETSELCOUNT, 0, 0);


                        if (lSelCount)
                        {
                            ai = (int far *) MyAlloc ((size_t)lSelCount * sizeof(int));

                            SendDlgItemMessage(
                                hwnd,
                                IDC_LIST2,
                                LB_GETSELITEMS,
                                (WPARAM) lSelCount,
                                (LPARAM) ai
                                );

                            for (i = 0; i < lSelCount; i++)
                            {
                                dwValue |= pLookup[ai[i]].dwVal;
                            }

                            MyFree (ai);
                        }

                        sprintf (lpszComboText, "%08lx", dwValue);

                    }
                    else
                    {
                        strcpy (lpszComboText, "00000000");
                    }

                    PostMessage (hwnd, WM_USER+55, 0, 0);

                    break;
                }
                case PT_STRING:

                    if (lSel == 1)
                    {
                        strncpy(
                            lpszComboText,
                            pParamsHeader->aParams[lLastSel].u.buf,
                            MAX_STRING_PARAM_SIZE
                            );

                        lpszComboText[MAX_STRING_PARAM_SIZE-1] = 0;
                    }
                    else
                    {
                        lpszComboText[0] = 0;
                    }

                    PostMessage (hwnd, WM_USER+55, 0, 0);

                    break;

                case PT_DWORD:

                    break;

                case PT_ORDINAL:

                    //
                    // The only option here is "select none"
                    //

                    strcpy (lpszComboText, "00000000");
                    PostMessage (hwnd, WM_USER+55, 0, 0);
                    break;

                case PT_WSAPROTOCOLINFO:
                {
                    if (lSel == 1)
                    {
                        LPWSAPROTOCOL_INFOA  pInfo = (gbWideStringParams ?
                                                (LPWSAPROTOCOL_INFOA)
                                                    &gWSAProtocolInfoW :
                                                &gWSAProtocolInfoA);
                        char szProtocol[MAX_STRING_PARAM_SIZE];
                        FUNC_PARAM params[] =
                        {
                            { "dwServiceFlags1",    PT_FLAGS,   (ULONG_PTR) pInfo->dwServiceFlags1, aServiceFlags },
                            { "dwServiceFlags2",    PT_DWORD,   (ULONG_PTR) pInfo->dwServiceFlags2, NULL },
                            { "dwServiceFlags3",    PT_DWORD,   (ULONG_PTR) pInfo->dwServiceFlags3, NULL },
                            { "dwServiceFlags4",    PT_DWORD,   (ULONG_PTR) pInfo->dwServiceFlags4, NULL },
                            { "dwProviderFlags1",   PT_FLAGS,   (ULONG_PTR) pInfo->dwProviderFlags, aProviderFlags },
                            { "ProviderID.Data1",   PT_DWORD,   (ULONG_PTR) pInfo->ProviderId.Data1, NULL },
                            { "ProviderID.Data2",   PT_DWORD,   (ULONG_PTR) pInfo->ProviderId.Data2, NULL },
                            { "ProviderID.Data3",   PT_DWORD,   (ULONG_PTR) pInfo->ProviderId.Data3, NULL },
                            { "ProviderID.Data4[0-3]",PT_DWORD, *((LPDWORD) pInfo->ProviderId.Data4), NULL },
                            { "ProviderID.Data4[4-7]",PT_DWORD, *((LPDWORD) &pInfo->ProviderId.Data4[4]), NULL },
                            { "dwCatalogEntryId",   PT_DWORD,   (ULONG_PTR) pInfo->dwCatalogEntryId, NULL },
                            { "ProtoChain.Len",     PT_DWORD,   (ULONG_PTR) pInfo->ProtocolChain.ChainLen, NULL },
                            { "ProtoChain.Entry0",  PT_DWORD,   (ULONG_PTR) pInfo->ProtocolChain.ChainEntries[0], NULL },
                            { "ProtoChain.Entry1",  PT_DWORD,   (ULONG_PTR) pInfo->ProtocolChain.ChainEntries[1], NULL },
                            { "ProtoChain.Entry2",  PT_DWORD,   (ULONG_PTR) pInfo->ProtocolChain.ChainEntries[2], NULL },
                            { "ProtoChain.Entry3",  PT_DWORD,   (ULONG_PTR) pInfo->ProtocolChain.ChainEntries[3], NULL },
                            { "ProtoChain.Entry4",  PT_DWORD,   (ULONG_PTR) pInfo->ProtocolChain.ChainEntries[4], NULL },
                            { "ProtoChain.Entry5",  PT_DWORD,   (ULONG_PTR) pInfo->ProtocolChain.ChainEntries[5], NULL },
                            { "ProtoChain.Entry6",  PT_DWORD,   (ULONG_PTR) pInfo->ProtocolChain.ChainEntries[6], NULL },
                            { "iVersion",           PT_DWORD,   (ULONG_PTR) pInfo->iVersion, NULL },
                            { "iAddressFamily",     PT_ORDINAL, (ULONG_PTR) pInfo->iAddressFamily, aAddressFamilies },
                            { "iMaxSockAddr",       PT_DWORD,   (ULONG_PTR) pInfo->iMaxSockAddr, NULL },
                            { "iMinSockAddr",       PT_DWORD,   (ULONG_PTR) pInfo->iMinSockAddr, NULL },
                            { "iSocketType",        PT_FLAGS,   (ULONG_PTR) pInfo->iSocketType, aSocketTypes },
                            { "iProtocol",          PT_DWORD,   (ULONG_PTR) pInfo->iProtocol, NULL }, // BUGBUG ought be flags?
                            { "iProtocolMaxOffset", PT_DWORD,   (ULONG_PTR) pInfo->iProtocolMaxOffset, NULL },
                            { "iNetworkByteOrder",  PT_ORDINAL, (ULONG_PTR) pInfo->iNetworkByteOrder, aNetworkByteOrders },
                            { "iSecurityScheme",    PT_DWORD,   (ULONG_PTR) pInfo->iSecurityScheme, NULL },
                            { "dwMessageSize",      PT_DWORD,   (ULONG_PTR) pInfo->dwMessageSize, NULL },
                            { "dwProviderReserved", PT_DWORD,   (ULONG_PTR) pInfo->dwProviderReserved, NULL },
                            { "szProtocol",         PT_STRING,  (ULONG_PTR) szProtocol, szProtocol }
                        };
                        FUNC_PARAM_HEADER paramsHeader =
                            { 31, WSAProtoInfo, params, NULL };


                        //
                        // Set "ghwndModal" so this Params dlg will
                        // be modal to the existing Params dlg
                        //

                        ghwndModal = hwnd;

                        if (gbWideStringParams)
                        {
                            //
                            // Convert from unicode to ascii show text not
                            // clipped after 1st char (it's an ascii window)
                            //

                            WideCharToMultiByte(
                                GetACP(),
                                0,
                                (LPWSTR) pInfo->szProtocol,
                                lstrlenW ((LPWSTR) pInfo->szProtocol) + 1,
                                szProtocol,
                                MAX_STRING_PARAM_SIZE,
                                NULL,
                                NULL
                                );
                        }
                        else
                        {
                            lstrcpyA (szProtocol, pInfo->szProtocol);
                        }


                        if (LetUserMungeParams (&paramsHeader))
                        {
                            //
                            // Save the new params
                            //

                            pInfo->dwServiceFlags1    = (DWORD)
                                params[0].dwValue;
                            pInfo->dwServiceFlags2    = (DWORD)
                                params[1].dwValue;
                            pInfo->dwServiceFlags3    = (DWORD)
                                params[2].dwValue;
                            pInfo->dwServiceFlags4    = (DWORD)
                                params[3].dwValue;
                            pInfo->dwProviderFlags    = (DWORD)
                                params[4].dwValue;
                            pInfo->ProviderId.Data1   = (DWORD)
                                params[5].dwValue;
                            pInfo->ProviderId.Data2   = (unsigned short)
                                LOWORD(params[6].dwValue);
                            pInfo->ProviderId.Data3   = (unsigned short)
                                LOWORD(params[7].dwValue);
                            *((LPDWORD) pInfo->ProviderId.Data4) = (DWORD)
                                params[8].dwValue;
                            *((LPDWORD) &pInfo->ProviderId.Data4[4]) = (DWORD)
                                params[9].dwValue;
                            pInfo->dwCatalogEntryId   = (DWORD)
                                params[10].dwValue;
                            pInfo->ProtocolChain.ChainLen = (DWORD)
                                params[11].dwValue;
                            pInfo->ProtocolChain.ChainEntries[0] = (DWORD)
                                params[12].dwValue;
                            pInfo->ProtocolChain.ChainEntries[1] = (DWORD)
                                params[13].dwValue;
                            pInfo->ProtocolChain.ChainEntries[2] = (DWORD)
                                params[14].dwValue;
                            pInfo->ProtocolChain.ChainEntries[3] = (DWORD)
                                params[15].dwValue;
                            pInfo->ProtocolChain.ChainEntries[4] = (DWORD)
                                params[16].dwValue;
                            pInfo->ProtocolChain.ChainEntries[5] = (DWORD)
                                params[17].dwValue;
                            pInfo->ProtocolChain.ChainEntries[6] = (DWORD)
                                params[18].dwValue;
                            pInfo->iVersion           = (int)
                                params[19].dwValue;
                            pInfo->iAddressFamily     = (int)
                                params[20].dwValue;
                            pInfo->iMaxSockAddr       = (int)
                                params[21].dwValue;
                            pInfo->iMinSockAddr       = (int)
                                params[22].dwValue;
                            pInfo->iSocketType        = (int)
                                params[23].dwValue;
                            pInfo->iProtocol          = (int)
                                params[24].dwValue;
                            pInfo->iProtocolMaxOffset = (int)
                                params[25].dwValue;
                            pInfo->iNetworkByteOrder  = (int)
                                params[26].dwValue;
                            pInfo->iSecurityScheme    = (int)
                                params[27].dwValue;
                            pInfo->dwMessageSize      = (DWORD)
                                params[28].dwValue;
                            pInfo->dwProviderReserved = (DWORD)
                                params[29].dwValue;

                            if (params[30].dwValue == (ULONG_PTR) szProtocol)
                            {
                                if (gbWideStringParams)
                                {
                                    lstrcpyW(
                                        (WCHAR *) pInfo->szProtocol,
                                        (WCHAR *) szProtocol
                                        );
                                }
                                else
                                {
                                    lstrcpyA (pInfo->szProtocol, szProtocol);
                                }
                            }
                        }


                        //
                        // Now reset ghwndModal to what it was initially
                        //

                        ghwndModal = ghwndMain;


                        //
                        // Strip off the "(valid pointer)" in the edit ctrl
                        //

                        wsprintf (lpszComboText, "%08lx", pInfo);
                        PostMessage (hwnd, WM_USER+55, 0, 0);

                        pParamsHeader->aParams[lLastSel].dwValue = (ULONG_PTR) pInfo;
                    }

                    break;
                }
                case PT_QOS:
                {
                    if (lSel == 1)
                    {
                        LPQOS   pQOS = (LPQOS)
                                    pParamsHeader->aParams[lLastSel].u.ptr;
                        FUNC_PARAM params[] =
                        {
                            { "Send.TokenRate",            PT_DWORD,   (ULONG_PTR) pQOS->SendingFlowspec.TokenRate, NULL },
                            { "Send.TokenBucketSize",      PT_DWORD,   (ULONG_PTR) pQOS->SendingFlowspec.TokenBucketSize, NULL },
                            { "Send.PeakBandwidth",        PT_DWORD,   (ULONG_PTR) pQOS->SendingFlowspec.PeakBandwidth, NULL },
                            { "Send.Latency",              PT_DWORD,   (ULONG_PTR) pQOS->SendingFlowspec.Latency, NULL },
                            { "Send.DelayVariation",       PT_DWORD,   (ULONG_PTR) pQOS->SendingFlowspec.DelayVariation, NULL },
                            { "Send.ServiceType",          PT_ORDINAL, (ULONG_PTR) pQOS->SendingFlowspec.ServiceType, aQOSServiceTypes },
                            { "Send.MaxSduSize",           PT_DWORD,   (ULONG_PTR) pQOS->SendingFlowspec.MaxSduSize, NULL },
                            { "Send.MinimumPolicedSize",   PT_DWORD,   (ULONG_PTR) pQOS->SendingFlowspec.MinimumPolicedSize, NULL },

                            { "Recv.TokenRate",            PT_DWORD,   (ULONG_PTR) pQOS->ReceivingFlowspec.TokenRate, NULL },
                            { "Recv.TokenBucketSize",      PT_DWORD,   (ULONG_PTR) pQOS->ReceivingFlowspec.TokenBucketSize, NULL },
                            { "Recv.PeakBandwidth",        PT_DWORD,   (ULONG_PTR) pQOS->ReceivingFlowspec.PeakBandwidth, NULL },
                            { "Recv.Latency",              PT_DWORD,   (ULONG_PTR) pQOS->ReceivingFlowspec.Latency, NULL },
                            { "Recv.DelayVariation",       PT_DWORD,   (ULONG_PTR) pQOS->ReceivingFlowspec.DelayVariation, NULL },
                            { "Recv.ServiceType",          PT_ORDINAL, (ULONG_PTR) pQOS->ReceivingFlowspec.ServiceType, aQOSServiceTypes },
                            { "Recv.MaxSduSize",           PT_DWORD,   (ULONG_PTR) pQOS->ReceivingFlowspec.MaxSduSize, NULL },
                            { "Recv.MinimumPolicedSize",   PT_DWORD,   (ULONG_PTR) pQOS->ReceivingFlowspec.MinimumPolicedSize, NULL },

                            { "ProviderSpecific.len",      PT_DWORD,   (ULONG_PTR) pQOS->ProviderSpecific.len, NULL },
                            { "ProviderSpecific.buf",      PT_POINTER, (ULONG_PTR) pQOS->ProviderSpecific.buf, pQOS+1 }
                        };
                        FUNC_PARAM_HEADER paramsHeader =
                            { 18, ws_QOS, params, NULL };


                        //
                        // Set "ghwndModal" so this Params dlg will
                        // be modal to the existing Params dlg
                        //

                        ghwndModal = hwnd;

                        if (LetUserMungeParams (&paramsHeader))
                        {
                            //
                            // Save the params
                            //

                            for (i = 0; i < 17; i++)
                            {
                                *(((LPDWORD) pQOS) + i) = (DWORD)
                                    params[i].dwValue;
                            }
                        }


                        //
                        // Now reset ghwndModal to what it was initially
                        //

                        ghwndModal = ghwndMain;


                        //
                        // Strip off the "(valid pointer)" in the edit ctrl
                        //

                        wsprintf (lpszComboText, "%08lx", pQOS);
                        PostMessage (hwnd, WM_USER+55, 0, 0);

                        pParamsHeader->aParams[lLastSel].dwValue =
                            (ULONG_PTR) pQOS;
                    }

                    break;
                }
                } // switch

                break;
            }
            case CBN_EDITCHANGE:
            {
                //
                // If user entered text in the edit field then copy the
                // text to our buffer
                //

                if (pParamsHeader->aParams[lLastSel].dwType == PT_STRING)
                {
                    char buf[MAX_STRING_PARAM_SIZE];


                    GetDlgItemText(
                        hwnd,
                        IDC_COMBO1,
                        buf,
                        MAX_STRING_PARAM_SIZE
                        );

                    strncpy(
                        pParamsHeader->aParams[lLastSel].u.buf,
                        buf,
                        MAX_STRING_PARAM_SIZE
                        );

                    pParamsHeader->aParams[lLastSel].u.buf
                        [MAX_STRING_PARAM_SIZE-1] = 0;
                }
                break;
            }
            } // switch

        } // switch

        break;
    }
    case WM_USER+55:

        SetDlgItemText (hwnd, IDC_COMBO1, pDlgInstData->szComboText);
        break;

#ifdef WIN32
    case WM_CTLCOLORSTATIC:

        SetBkColor ((HDC) wParam, RGB (192,192,192));
        return (INT_PTR) GetStockObject (LTGRAY_BRUSH);
#else
    case WM_CTLCOLOR:
    {
        if (HIWORD(lParam) == CTLCOLOR_STATIC)
        {
            SetBkColor ((HDC) wParam, RGB (192,192,192));
            return (INT_PTR) GetStockObject (LTGRAY_BRUSH);
        }
        break;
    }
#endif
    case WM_PAINT:
    {
        PAINTSTRUCT ps;

        BeginPaint (hwnd, &ps);
        FillRect (ps.hdc, &ps.rcPaint, GetStockObject (LTGRAY_BRUSH));
        EndPaint (hwnd, &ps);

        break;
    }
    } // switch

    return FALSE;
}


INT_PTR
CALLBACK
UserButtonsDlgProc(
    HWND hwnd,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    static int iButtonIndex;

    switch (msg)
    {
    case WM_INITDIALOG:
    {
        int i;
        char buf[32];

        if (lParam)
        {
            //
            // The dlg was invoked because someone pressed a user button
            // that was uninitialized, so only allow chgs on this button
            //

            iButtonIndex = *((int *) lParam);

            _itoa (iButtonIndex + 1, buf, 10);

            SendDlgItemMessage(
                hwnd,
                IDC_LIST1,
                LB_INSERTSTRING,
                (WPARAM) -1,
                (LPARAM) buf
                );
        }
        else
        {
            //
            // The dlg was invoked because the user chose a menuitem,
            // so allow chgs on all buttons
            //

            iButtonIndex = MAX_USER_BUTTONS;

            for (i = 1; i <= MAX_USER_BUTTONS; i++)
            {
                _itoa (i, buf, 10);

                SendDlgItemMessage(
                    hwnd,
                    IDC_LIST1,
                    LB_INSERTSTRING,
                    (WPARAM) -1,
                    (LPARAM) buf
                    );
            }

        }

        SendDlgItemMessage(
            hwnd,
            IDC_LIST1,
            LB_SETCURSEL,
            (WPARAM) 0,
            0
            );

        for (i = 0; aFuncNames[i]; i++)
        {
            SendDlgItemMessage(
                hwnd,
                IDC_LIST2,
                LB_INSERTSTRING,
                (WPARAM) -1,
                (LPARAM) aFuncNames[i]
                );
        }

        SendDlgItemMessage(
            hwnd,
            IDC_LIST2,
            LB_INSERTSTRING,
            (WPARAM) -1,
            (LPARAM) "<none>"
            );

        if (!lParam)
        {
#ifdef WIN32
            wParam = (WPARAM) MAKELONG (0, LBN_SELCHANGE);
#else
            lParam = (LPARAM) MAKELONG (0, LBN_SELCHANGE);
#endif
            goto IDC_LIST1_selchange;
        }

        break;
    }
    case WM_COMMAND:

        switch (LOWORD(wParam))
        {
        case IDOK:
        {
            LRESULT lFuncSel;


            lFuncSel = SendDlgItemMessage(hwnd, IDC_LIST2, LB_GETCURSEL, 0, 0);

            if (lFuncSel == LB_ERR)
            {
                MessageBox (hwnd, "Select a function", "", MB_OK);
                break;
            }

            if (iButtonIndex == MAX_USER_BUTTONS)
            {
                iButtonIndex = (int) SendDlgItemMessage(
                    hwnd,
                    IDC_LIST1,
                    LB_GETCURSEL,
                    0,
                    0
                    );
            }

            aUserButtonFuncs[iButtonIndex] = (DWORD) lFuncSel;

            if (lFuncSel == MiscBegin)
            {
                //
                // User selected "<none>" option so nullify string
                //

                aUserButtonsText[iButtonIndex][0] = 0;
            }
            else
            {
                GetDlgItemText(
                    hwnd,
                    IDC_EDIT1,
                    (LPSTR) &aUserButtonsText[iButtonIndex],
                    MAX_USER_BUTTON_TEXT_SIZE - 1
                    );

                aUserButtonsText[iButtonIndex][MAX_USER_BUTTON_TEXT_SIZE - 1] =
                    0;
            }

            SetDlgItemText(
                ghwndMain,
                IDC_BUTTON7 + iButtonIndex,
                (LPSTR) &aUserButtonsText[iButtonIndex]
                );

            // Fall thru to IDCANCEL code
        }
        case IDCANCEL:

            EndDialog (hwnd, FALSE);
            break;

        case IDC_LIST1:

IDC_LIST1_selchange:

#ifdef WIN32
            if (HIWORD(wParam) == LBN_SELCHANGE)
#else
            if (HIWORD(lParam) == LBN_SELCHANGE)
#endif
            {
                LRESULT lButtonSel =
                    SendDlgItemMessage(hwnd, IDC_LIST1, LB_GETCURSEL, 0, 0);


                SendDlgItemMessage(
                    hwnd,
                    IDC_LIST2,
                    LB_SETCURSEL,
                    (WPARAM) aUserButtonFuncs[lButtonSel],
                    0
                    );

                SetDlgItemText(
                    hwnd,
                    IDC_EDIT1,
                    aUserButtonsText[lButtonSel]
                    );
            }
            break;

        } // switch

        break;

#ifdef WIN32
    case WM_CTLCOLORSTATIC:

        SetBkColor ((HDC) wParam, RGB (192,192,192));
        return (INT_PTR) GetStockObject (LTGRAY_BRUSH);
#else
    case WM_CTLCOLOR:
    {
        if (HIWORD(lParam) == CTLCOLOR_STATIC)
        {
            SetBkColor ((HDC) wParam, RGB (192,192,192));
            return (INT_PTR) GetStockObject (LTGRAY_BRUSH);
        }
        break;
    }
#endif
    case WM_PAINT:
    {
        PAINTSTRUCT ps;

        BeginPaint (hwnd, &ps);
        FillRect (ps.hdc, &ps.rcPaint, GetStockObject (LTGRAY_BRUSH));
        EndPaint (hwnd, &ps);

        break;
    }
    } // switch

    return FALSE;
}


int
CALLBACK
ConditionProc(
    LPWSABUF    lpCallerId,
    LPWSABUF    lpCallerData,
    LPQOS       lpSQOS,
    LPQOS       lpGQOS,
    LPWSABUF    lpCalleeId,
    LPWSABUF    lpCalleeData,
    GROUP FAR   *g,
    DWORD       dwCallbackData
    )
{
    ShowStr ("ConditionProc: enter");

    return 0;
}


void
CALLBACK
CompletionProc(
    DWORD           dwError,
    DWORD           cbTransferred,
    LPWSAOVERLAPPED pOverlapped,
    DWORD           dwFlags
    )
{
    ShowStr ("CompletionProc: enter");
    ShowStr ("  dwError=%ld", dwError);
    ShowStr ("  cbTransferred=%ld", cbTransferred);
    ShowStr ("  pOverlapped=x%x", pOverlapped);
    ShowStr ("  dwFlags=%d (x%x)", dwFlags, dwFlags);

    MyFree (pOverlapped);
}


DWORD
PASCAL
ShowError(
    FUNC_INDEX  funcIndex,
    int         iResult
    )
{
    DWORD   i, dwError = (iResult == 0 ? (DWORD) WSAGetLastError() : iResult);


    for(
        i = 0;
        (dwError != aWSAErrors[i].dwVal  &&  aWSAErrors[i].lpszVal);
        i++
        );

    ShowStr(
        "%s error: %s (%d)",
        aFuncNames[funcIndex],
        (aWSAErrors[i].lpszVal ? aWSAErrors[i].lpszVal : gszUnknownError),
        dwError
        );

    return dwError;
}


void
PASCAL
ShowUnknownError(
    FUNC_INDEX  funcIndex,
    int         iResult
    )
{
     ShowStr(
         "%s returned unrecognized error (%d)",
         aFuncNames[funcIndex],
         iResult
         );
}


void
FAR
PASCAL
FuncDriver(
    FUNC_INDEX funcIndex
    )
{
    int i, j, k;


    //
    // Determine if we're doing a ascii or a unicode op
    //

    gbWideStringParams =
        ((aFuncNames[funcIndex])[strlen (aFuncNames[funcIndex]) - 1] == 'W' ?
            TRUE : FALSE);


    //
    // Zero the global buf so the user doesn't see extraneous stuff
    //

    ZeroMemory (pBigBuf, dwBigBufSize);


    //
    // The big switch statement...
    //

    switch (funcIndex)
    {
    case ws_accept:
    {
        int iLength;
        struct sockaddr sa;
        FUNC_PARAM params[] =
        {
            { gszSocket,        PT_DWORD,       (ULONG_PTR) gpSelectedSocket->Sock, NULL },
            { "lpAddress",      PT_PTRNOEDIT,   (ULONG_PTR) &sa, &sa },
            { "lpiAddrLength",  PT_PTRNOEDIT,   (ULONG_PTR) &iLength, &iLength },
        };
        FUNC_PARAM_HEADER paramsHeader =
            { 3, funcIndex, params, NULL };


        if (LetUserMungeParams (&paramsHeader))
        {
            SOCKET s;


            if ((s = accept(
                    (SOCKET) params[0].dwValue,
                    (struct sockaddr FAR *) params[1].dwValue,
                    (int FAR *) params[2].dwValue

                    )) != INVALID_SOCKET)
            {
                ShowStr (gszXxxSUCCESS, aFuncNames[funcIndex]);
                ShowStr ("  returned socket=x%x", s);

                if (params[1].dwValue)
                {
                    ShowStr ("  lpAddress=x%x", params[1].dwValue);

                    ShowStr(
                        "  ->sa_family=%d, %s",
                        sa.sa_family,
                        GetStringFromOrdinalValue(
                            (DWORD) sa.sa_family,
                            aAddressFamilies
                            )
                        );

                    ShowStr ("  ->sa_data=");
                    ShowBytes (14, sa.sa_data, 1);
                }
            }
            else
            {
                ShowError (funcIndex, 0);
            }
        }

        break;
    }
    case ws_bind:
    {
        FUNC_PARAM params[] =
        {
            { gszSocket,        PT_DWORD,       (ULONG_PTR) gpSelectedSocket->Sock, NULL },
            { "lpName",         PT_PTRNOEDIT,   (ULONG_PTR) pBigBuf, pBigBuf },
            { "  ->sa_family",  PT_ORDINAL,     (ULONG_PTR) gdwDefAddrFamily, aAddressFamilies },
            { "  ->sa_data",    PT_POINTER,     (ULONG_PTR) (pBigBuf + 2), pBigBuf + 2 },
            { "namelen",        PT_DWORD,       (ULONG_PTR) dwBigBufSize, NULL },
        };
        FUNC_PARAM_HEADER paramsHeader =
            { 5, funcIndex, params, NULL };


        if (LetUserMungeParams (&paramsHeader))
        {
            *((u_short *) pBigBuf) = LOWORD(params[2].dwValue);

            if ((i = bind(
                    (SOCKET) params[0].dwValue,
                    (struct sockaddr FAR *) params[1].dwValue,
                    (int) params[4].dwValue

                    )) == 0)
            {
                ShowStr (gszXxxSUCCESS, aFuncNames[funcIndex]);
            }
            else if (i == SOCKET_ERROR)
            {
                ShowError (funcIndex, 0);
            }
            else
            {
                ShowUnknownError (funcIndex, i);
            }
        }

        break;
    }
    case ws_closesocket:
    {
        FUNC_PARAM params[] =
        {
            { gszSocket,    PT_DWORD,   (ULONG_PTR) gpSelectedSocket->Sock, NULL },
        };
        FUNC_PARAM_HEADER paramsHeader =
            { 1, funcIndex, params, NULL };


        if (LetUserMungeParams (&paramsHeader))
        {
            if ((i = closesocket ((SOCKET) params[0].dwValue)) == 0)
            {
                ShowStr (gszXxxSUCCESS, aFuncNames[funcIndex]);

                if ((SOCKET) params[0].dwValue != gpSelectedSocket->Sock)
                {
                    //
                    // The user overrode the selected widget, so make sure
                    // we don't delete the wrong string from the list box
                    //

                    int         iNumItems;
                    PMYSOCKET   ps;


                    iNumItems = (int) SendMessage(
                        ghwndList1,
                        LB_GETCOUNT,
                        0,
                        0
                        );

                    for (i = 0; i < iNumItems; i++)
                    {
                        ps = (PMYSOCKET) SendMessage(
                            ghwndList1,
                            LB_GETITEMDATA,
                            i,
                            0
                            );

                        if ((SOCKET) params[0].dwValue == ps->Sock)
                        {
                            glCurrentSelection = (LONG) i;
                            gpSelectedSocket = ps;
                            break;
                        }
                    }

                    if (i == iNumItems)
                    {
                        ShowStr(
                            "Strange, couldn't find socket=x%x in list",
                            params[0].dwValue
                            );

                        break;
                    }
                }

                SendMessage(
                    ghwndList1,
                    LB_DELETESTRING,
                    glCurrentSelection,
                    0
                    );

                MyFree (gpSelectedSocket);
            }
            else if (i == SOCKET_ERROR)
            {
                ShowError (funcIndex, 0);
            }
            else
            {
                ShowUnknownError (funcIndex, i);
            }
        }

        break;
    }
    case ws_connect:
    {
        FUNC_PARAM params[] =
        {
            { gszSocket,        PT_DWORD,       (ULONG_PTR) gpSelectedSocket->Sock, NULL },
            { "lpName",         PT_PTRNOEDIT,   (ULONG_PTR) pBigBuf, pBigBuf },
            { "  ->sa_family",  PT_ORDINAL,     (ULONG_PTR) gdwDefAddrFamily, aAddressFamilies },
            { "  ->sa_data",    PT_POINTER,     (ULONG_PTR) (pBigBuf + 2), pBigBuf + 2 },
            { "namelen",        PT_DWORD,       (ULONG_PTR) dwBigBufSize, NULL },
        };
        FUNC_PARAM_HEADER paramsHeader =
            { 5, funcIndex, params, NULL };


        if (LetUserMungeParams (&paramsHeader))
        {
            *((u_short *) pBigBuf) = LOWORD(params[2].dwValue);

            if ((i = connect(
                    (SOCKET) params[0].dwValue,
                    (struct sockaddr FAR *) params[1].dwValue,
                    (int) params[4].dwValue

                    )) == 0)
            {
                ShowStr (gszXxxSUCCESS, aFuncNames[funcIndex]);
            }
            else if (i == SOCKET_ERROR)
            {
                ShowError (funcIndex, 0);
            }
            else
            {
                ShowUnknownError (funcIndex, i);
            }
        }

        break;
    }
    case ws_gethostbyaddr:
    {
        char szAddress[MAX_STRING_PARAM_SIZE];
        FUNC_PARAM params[] =
        {
            { "addr",   PT_POINTER, (ULONG_PTR) pBigBuf, pBigBuf },
            { "len",    PT_DWORD,   (ULONG_PTR) dwBigBufSize, NULL },
            { "type",   PT_ORDINAL, (ULONG_PTR) gdwDefAddrFamily, aAddressFamilies },
        };
        FUNC_PARAM_HEADER paramsHeader =
            { 3, funcIndex, params, NULL };


        lstrcpy (szAddress, "xxx");

        if (LetUserMungeParams (&paramsHeader))
        {
            struct hostent  *phe;


            // note: WinSock owns the returned ptr, don't free/munge it

            if ((phe = gethostbyaddr(
                    (char FAR *) params[0].dwValue,
                    (int) params[1].dwValue,
                    (int) params[2].dwValue
                    )))
            {
                ShowStr (gszXxxSUCCESS, aFuncNames[funcIndex]);
                ShowHostEnt (phe);
            }
            else
            {
                ShowError (funcIndex, 0);
            }
        }

        break;
    }
    case ws_gethostbyname:
    {
        char szHostName[MAX_STRING_PARAM_SIZE];
        FUNC_PARAM params[] =
        {
            { "name",   PT_STRING,  (ULONG_PTR) szHostName, szHostName }
        };
        FUNC_PARAM_HEADER paramsHeader =
            { 1, funcIndex, params, NULL };


        lstrcpyA (szHostName, gszDefHostName);

        if (LetUserMungeParams (&paramsHeader))
        {
            struct hostent  *phe;


            // note: WinSock owns the returned ptr, don't free/munge it

            if ((phe = gethostbyname ((char FAR *) params[0].dwValue)))
            {
                ShowStr (gszXxxSUCCESS, aFuncNames[funcIndex]);
                ShowHostEnt (phe);
            }
            else
            {
                ShowError (funcIndex, 0);
            }
        }

        break;
    }
    case ws_gethostname:
    {
        char szName[MAX_STRING_PARAM_SIZE];
        FUNC_PARAM params[] =
        {
            { "name",   PT_POINTER,  (ULONG_PTR) szName, szName },
            { "namelen",PT_DWORD,    (ULONG_PTR) MAX_STRING_PARAM_SIZE, NULL }
        };
        FUNC_PARAM_HEADER paramsHeader =
            { 2, funcIndex, params, NULL };


        if (LetUserMungeParams (&paramsHeader))
        {
            if ((i = gethostname(
                    (char FAR *) params[0].dwValue,
                    (int) params[1].dwValue

                    )) == 0)
            {
                ShowStr (gszXxxSUCCESS, aFuncNames[funcIndex]);
                ShowStr ("  name=%s", (char FAR *) params[0].dwValue);
            }
            else if (i == SOCKET_ERROR)
            {
                ShowError (funcIndex, 0);
            }
            else
            {
                ShowUnknownError (funcIndex, i);
            }
        }

        break;
    }
    case ws_getpeername:
    case ws_getsockname:
    {
        int     iNameLen = dwBigBufSize;
        FUNC_PARAM params[] =
        {
            { gszSocket,        PT_DWORD,       (ULONG_PTR) gpSelectedSocket->Sock, NULL },
            { "lpName",         PT_PTRNOEDIT,   (ULONG_PTR) pBigBuf, pBigBuf },
            { "lpiNameLength",  PT_PTRNOEDIT,   (ULONG_PTR) &iNameLen, &iNameLen },
        };
        FUNC_PARAM_HEADER paramsHeader =
            { 3, funcIndex, params, NULL };


        if (LetUserMungeParams (&paramsHeader))
        {
            if (funcIndex == ws_getpeername)
            {
                i = getpeername(
                    (SOCKET) params[0].dwValue,
                    (struct sockaddr FAR *) params[1].dwValue,
                    (int *) params[2].dwValue
                    );
            }
            else
            {
                i = getsockname(
                    (SOCKET) params[0].dwValue,
                    (struct sockaddr FAR *) params[1].dwValue,
                    (int *) params[2].dwValue
                    );
            }

            if (i == 0)
            {
                DWORD dwAddressFamily = (DWORD)
                          *((u_short *) params[1].dwValue);


                ShowStr (gszXxxSUCCESS, aFuncNames[funcIndex]);

                ShowStr ("  *lpNameLength=x%x", *((int *) params[2].dwValue));
                ShowStr ("  lpName=x%x", params[1].dwValue);

                ShowStr(
                    "    ->AddressFamiliy=%d, %s",
                    dwAddressFamily,
                    GetStringFromOrdinalValue(
                        dwAddressFamily,
                        aAddressFamilies
                        )
                    );

                ShowStr ("    ->Data=");
                ShowBytes(
                    *((int *) params[2].dwValue) - sizeof (u_short),
                    (LPBYTE) params[1].dwValue + 2,
                    1
                    );
            }
            else if (i == SOCKET_ERROR)
            {
                ShowError (funcIndex, 0);
            }
            else
            {
                ShowUnknownError (funcIndex, i);
            }
        }

        break;
    }
    case ws_getprotobyname:
    {
        char szProtoName[MAX_STRING_PARAM_SIZE];
        FUNC_PARAM params[] =
        {
            { "name",   PT_STRING,  (ULONG_PTR) szProtoName, szProtoName },
        };
        FUNC_PARAM_HEADER paramsHeader =
            { 1, funcIndex, params, NULL };


        lstrcpyA (szProtoName, gszDefProtoName);

        if (LetUserMungeParams (&paramsHeader))
        {
            struct protoent *ppe;


            // note: WinSock owns the returned ptr, don't free/munge it

            if ((ppe = getprotobyname ((char FAR *) params[0].dwValue)))
            {
                ShowStr (gszXxxSUCCESS, aFuncNames[funcIndex]);
                ShowProtoEnt (ppe);
            }
            else
            {
                ShowError (funcIndex, 0);
            }
        }

        break;
    }
    case ws_getprotobynumber:
    {
        FUNC_PARAM params[] =
        {
            { "number", PT_DWORD,   (ULONG_PTR) gdwDefPortNum, NULL },
        };
        FUNC_PARAM_HEADER paramsHeader =
            { 1, funcIndex, params, NULL };


        if (LetUserMungeParams (&paramsHeader))
        {
            struct protoent *ppe;


            // note: WinSock owns the returned ptr, don't free/munge it

            if ((ppe = getprotobynumber ((int) params[0].dwValue)))
            {
                ShowStr (gszXxxSUCCESS, aFuncNames[funcIndex]);
                ShowProtoEnt (ppe);
            }
            else
            {
                ShowError (funcIndex, 0);
            }
        }

        break;
    }
    case ws_getservbyname:
    {
        char szServName[MAX_STRING_PARAM_SIZE];
        char szProtoName[MAX_STRING_PARAM_SIZE];
        FUNC_PARAM params[] =
        {
            { "serviceName",    PT_STRING,  (ULONG_PTR) szServName, szServName },
            { "protoName",      PT_STRING,  (ULONG_PTR) 0, szProtoName },
        };
        FUNC_PARAM_HEADER paramsHeader =
            { 2, funcIndex, params, NULL };


        lstrcpyA (szServName, gszDefServName);
        lstrcpyA (szProtoName, gszDefProtoName);

        if (LetUserMungeParams (&paramsHeader))
        {
            struct servent *pse;


            // note: WinSock owns the returned ptr, don't free/munge it

            if ((pse = getservbyname(
                    (char FAR *) params[0].dwValue,
                    (char FAR *) params[1].dwValue
                    )))
            {
                ShowStr (gszXxxSUCCESS, aFuncNames[funcIndex]);
                ShowServEnt (pse);
            }
            else
            {
                ShowError (funcIndex, 0);
            }
        }

        break;
    }
    case ws_getservbyport:
    {
        char szProtoName[MAX_STRING_PARAM_SIZE];
        FUNC_PARAM params[] =
        {
            { "port",       PT_DWORD,   (ULONG_PTR) gdwDefPortNum, NULL },
            { "protoName",  PT_STRING,  (ULONG_PTR) 0, szProtoName },
        };
        FUNC_PARAM_HEADER paramsHeader =
            { 2, funcIndex, params, NULL };


        lstrcpyA (szProtoName, gszDefProtoName);

        if (LetUserMungeParams (&paramsHeader))
        {
            struct servent *pse;


            // note: WinSock owns the returned ptr, don't free/munge it

            if ((pse = getservbyport(
                    (int) params[0].dwValue,
                    (char FAR *) params[1].dwValue
                    )))
            {
                ShowStr (gszXxxSUCCESS, aFuncNames[funcIndex]);
                ShowServEnt (pse);
            }
            else
            {
                ShowError (funcIndex, 0);
            }
        }

        break;
    }
    case ws_getsockopt:
    {
        int iOptionBufLength = dwBigBufSize;
        FUNC_PARAM params[] =
        {
            { gszSocket,            PT_DWORD,       (ULONG_PTR) gpSelectedSocket->Sock, NULL },
            { "iLevel",             PT_ORDINAL,     (ULONG_PTR) 0, aSockOptLevels },
            { "iOptionName",        PT_ORDINAL,     (ULONG_PTR) 0, aSockOpts },
            { "lpOptionBuf",        PT_PTRNOEDIT,   (ULONG_PTR) pBigBuf, pBigBuf },
            { "lpiOptionBufLength", PT_POINTER,     (ULONG_PTR) &iOptionBufLength, &iOptionBufLength },
        };
        FUNC_PARAM_HEADER paramsHeader =
            { 5, funcIndex, params, NULL };


        if (LetUserMungeParams (&paramsHeader))
        {
            if ((i = getsockopt(
                    (SOCKET) params[0].dwValue,
                    (int) params[1].dwValue,
                    (int) params[2].dwValue,
                    (char FAR *) params[3].dwValue,
                    (int FAR *) params[4].dwValue

                    )) == 0)
            {
// BUGBUG getockopt: format returned OptionBuf data

                ShowStr (gszXxxSUCCESS, aFuncNames[funcIndex]);

                ShowStr(
                    "  Level/OptionName=%d/%d, %s/%s",
                    params[1].dwValue,
                    params[2].dwValue,
                    GetStringFromOrdinalValue(
                        (DWORD) params[1].dwValue,
                        aSockOptLevels
                        ),
                    GetStringFromOrdinalValue(
                        (DWORD) params[2].dwValue,
                        aSockOpts
                        )
                    );

                ShowStr(
                    "  *lpiOptionBufLength=x%x",
                    *((int *) params[4].dwValue)
                    );
                ShowStr ("  *lpOptionBuf=");
                ShowBytes(
                    (DWORD) *((int *) params[4].dwValue),
                    (LPVOID) params[3].dwValue,
                    1
                    );
            }
            else if (i == SOCKET_ERROR)
            {
                 ShowError (funcIndex, 0);
            }
            else
            {
                 ShowUnknownError (funcIndex, i);
            }
        }

        break;
    }
    case ws_htonl:
    {
        FUNC_PARAM params[] =
        {
            { "hostLong", PT_DWORD,   (ULONG_PTR) 0x12345678, NULL },
        };
        FUNC_PARAM_HEADER paramsHeader =
            { 1, funcIndex, params, NULL };


        if (LetUserMungeParams (&paramsHeader))
        {
            ShowStr (gszXxxSUCCESS, aFuncNames[funcIndex]);
            ShowStr ("  hostLong=x%x", params[0].dwValue);
            ShowStr ("  networkLong=x%x", htonl ((u_long) params[0].dwValue));
        }

        break;
    }
    case ws_htons:
    {
        FUNC_PARAM params[] =
        {
            { "hostShort", PT_DWORD,   (ULONG_PTR) 0x1234, NULL },
        };
        FUNC_PARAM_HEADER paramsHeader =
            { 1, funcIndex, params, NULL };


        if (LetUserMungeParams (&paramsHeader))
        {
            ShowStr (gszXxxSUCCESS, aFuncNames[funcIndex]);
            ShowStr ("  hostShort=x%x", (DWORD) LOWORD(params[0].dwValue));
            ShowStr(
                "  networkShort=x%x",
                (DWORD) htons ((u_short) LOWORD(params[0].dwValue))
                );
        }

        break;
    }
    case ws_inet_addr:
    {
        char szInetAddr[MAX_STRING_PARAM_SIZE] = "1.2.3.4";
        FUNC_PARAM params[] =
        {
            { "szInternetAddress",   PT_STRING,  (ULONG_PTR) szInetAddr, szInetAddr },
        };
        FUNC_PARAM_HEADER paramsHeader =
            { 1, funcIndex, params, NULL };


        if (LetUserMungeParams (&paramsHeader))
        {
            unsigned long l = inet_addr ((char FAR *) params[0].dwValue);


            if (l != INADDR_NONE)
            {
                ShowStr (gszXxxSUCCESS, aFuncNames[funcIndex]);
                ShowStr ("  ulInternetAddress=x%x", l);
            }
            else
            {
                ShowStr ("inet_addr error: INADDR_NONE");
            }
        }

        break;
    }
    case ws_inet_ntoa:
    {
        FUNC_PARAM params[] =
        {
            { "ulInternetAddress",   PT_DWORD,  (ULONG_PTR) 0x04030201, NULL },
        };
        FUNC_PARAM_HEADER paramsHeader =
            { 1, funcIndex, params, NULL };


        if (LetUserMungeParams (&paramsHeader))
        {
            char FAR *pszInetAddr;
            struct in_addr ia;


            CopyMemory (&ia, &params[0].dwValue, sizeof (DWORD));

            if ((pszInetAddr = inet_ntoa (ia)))
            {
                ShowStr (gszXxxSUCCESS, aFuncNames[funcIndex]);
                ShowStr ("  pszInternetAddress=x%x", pszInetAddr);
                ShowStr ("  szInternetAddress=%s", pszInetAddr);
            }
            else
            {
                ShowStr ("inet_ntoa error: returned NULL");
            }
        }

        break;
    }
    case ws_ioctlsocket:
    {
        FUNC_PARAM params[] =
        {
            { gszSocket,    PT_DWORD,   (ULONG_PTR) gpSelectedSocket->Sock, NULL },
            { "lCommand",   PT_ORDINAL, (ULONG_PTR) 0, aIoctlCmds },
            { "lpData",     PT_POINTER, (ULONG_PTR) pBigBuf, pBigBuf }
        };
        FUNC_PARAM_HEADER paramsHeader =
            { 3, funcIndex, params, NULL };


        if (LetUserMungeParams (&paramsHeader))
        {
            if ((i = ioctlsocket(
                    (SOCKET) params[0].dwValue,
                    (long) params[1].dwValue,
                    (u_long FAR *) params[2].dwValue

                    )) == 0)
            {
                ShowStr (gszXxxSUCCESS, aFuncNames[funcIndex]);

                ShowStr(
                    "  lCommand=%d, %s",
                    params[1].dwValue,
                    GetStringFromOrdinalValue(
                        (DWORD) params[1].dwValue,
                        aIoctlCmds
                        )
                    );

                ShowStr ("  *lpData=");
                ShowBytes (16, (LPVOID) params[2].dwValue, 1);
            }
            else if (i == SOCKET_ERROR)
            {
                ShowError (funcIndex, 0);
            }
            else
            {
                ShowUnknownError (funcIndex, i);
            }
        }

        break;
    }
    case ws_listen:
    {
        FUNC_PARAM params[] =
        {
            { gszSocket,    PT_DWORD,   (ULONG_PTR) gpSelectedSocket->Sock, NULL },
            { "iBacklog",   PT_DWORD,   (ULONG_PTR) 0, NULL }
        };
        FUNC_PARAM_HEADER paramsHeader =
            { 2, funcIndex, params, NULL };


        if (LetUserMungeParams (&paramsHeader))
        {
            if ((i = listen(
                    (SOCKET) params[0].dwValue,
                    (int) params[1].dwValue

                    )) == 0)
            {
                ShowStr (gszXxxSUCCESS, aFuncNames[funcIndex]);
            }
            else if (i == SOCKET_ERROR)
            {
                ShowError (funcIndex, 0);
            }
            else
            {
                ShowUnknownError (funcIndex, i);
            }
        }

        break;
    }
    case ws_ntohl:
    {
        FUNC_PARAM params[] =
        {
            { "networkLong", PT_DWORD,  (ULONG_PTR) 0x12345678, NULL },
        };
        FUNC_PARAM_HEADER paramsHeader =
            { 1, funcIndex, params, NULL };


        if (LetUserMungeParams (&paramsHeader))
        {
            ShowStr (gszXxxSUCCESS, aFuncNames[funcIndex]);
            ShowStr ("  networkLong=x%x", params[0].dwValue);
            ShowStr ("  hostLong=x%x", ntohl ((u_long) params[0].dwValue));
        }

        break;
    }
    case ws_ntohs:
    {
        FUNC_PARAM params[] =
        {
            { "networkShort", PT_DWORD, (ULONG_PTR) 0x1234, NULL },
        };
        FUNC_PARAM_HEADER paramsHeader =
            { 1, funcIndex, params, NULL };


        if (LetUserMungeParams (&paramsHeader))
        {
            ShowStr (gszXxxSUCCESS, aFuncNames[funcIndex]);
            ShowStr ("  networkShort=x%x",(DWORD) LOWORD(params[0].dwValue));
            ShowStr(
                "  hostShort=x%x",
                (DWORD) ntohs ((u_short) LOWORD(params[0].dwValue))
                );
        }

        break;
    }
    case ws_recv:
    {
        FUNC_PARAM params[] =
        {
            { gszSocket,    PT_DWORD,       (ULONG_PTR) gpSelectedSocket->Sock, NULL },
            { "lpBuf",      PT_PTRNOEDIT,   (ULONG_PTR) pBigBuf, pBigBuf },
            { "iBufLength", PT_DWORD,       (ULONG_PTR) dwBigBufSize, NULL },
            { "iRecvFlags", PT_FLAGS,       (ULONG_PTR) gdwDefRecvFlags, aRecvFlags },
        };
        FUNC_PARAM_HEADER paramsHeader =
            { 4, funcIndex, params, NULL };


        if (LetUserMungeParams (&paramsHeader))
        {
            if ((i = recv(
                    (SOCKET) params[0].dwValue,
                    (char FAR *) params[1].dwValue,
                    (int) params[2].dwValue,
                    (int) params[3].dwValue

                    )) != SOCKET_ERROR)
            {
                ShowStr (gszXxxSUCCESS, aFuncNames[funcIndex]);
                ShowStr ("  NumByteReceived=x%x", i);
                ShowStr ("  *pBuf=");
                ShowBytes (i, (LPVOID) params[1].dwValue, 1);
            }
            else
            {
                ShowError (funcIndex, 0);
            }
        }

        break;
    }
    case ws_recvfrom:
    {
        int    iSrcAddrLen;
        struct sockaddr FAR *pSrcAddr = MyAlloc (dwBigBufSize);
        FUNC_PARAM params[] =
        {
            { gszSocket,            PT_DWORD,       (ULONG_PTR) gpSelectedSocket->Sock, NULL },
            { "lpBuf",              PT_PTRNOEDIT,   (ULONG_PTR) pBigBuf, pBigBuf },
            { "iBufLength",         PT_DWORD,       (ULONG_PTR) dwBigBufSize, NULL },
            { "iRecvFlags",         PT_FLAGS,       (ULONG_PTR) gdwDefRecvFlags, aRecvFlags },
            { "lpSourceAddr",       PT_PTRNOEDIT,   (ULONG_PTR) pSrcAddr, pSrcAddr },
            { "lpiSourceAddrLength",PT_POINTER,     (ULONG_PTR) &iSrcAddrLen, &iSrcAddrLen },
        };
        FUNC_PARAM_HEADER paramsHeader =
            { 6, funcIndex, params, NULL };


        if (!pSrcAddr)
        {
            break;
        }

        if (LetUserMungeParams (&paramsHeader))
        {
            if ((i = recvfrom(
                    (SOCKET) params[0].dwValue,
                    (char FAR *) params[1].dwValue,
                    (int) params[2].dwValue,
                    (int) params[3].dwValue,
                    (struct sockaddr FAR *) params[4].dwValue,
                    (int FAR *) params[5].dwValue

                    )) != SOCKET_ERROR)
            {
                ShowStr (gszXxxSUCCESS, aFuncNames[funcIndex]);
                ShowStr ("  NumByteReceived=x%x", i);
                ShowStr ("  *pBuf=");
                ShowBytes (i, (LPVOID) params[1].dwValue, 1);
                ShowStr(
                    "  *lpiSourceAddrLength=%d",
                    (DWORD) *((int *) params[5].dwValue)
                    );
                ShowStr ("  *lpSourceAddr=");
                ShowBytes(
                    *((int *) params[5].dwValue),
                    (LPVOID) params[4].dwValue,
                    1
                    );
            }
            else
            {
                ShowError (funcIndex, 0);
            }
        }

        MyFree (pSrcAddr);

        break;
    }
    case ws_select:

// BUGBUG case ws_select:
        break;

    case ws_send:
    {
        FUNC_PARAM params[] =
        {
            { gszSocket,    PT_DWORD,   (ULONG_PTR) gpSelectedSocket->Sock, NULL },
            { "lpBuf",      PT_POINTER, (ULONG_PTR) pBigBuf, pBigBuf },
            { "iBufLength", PT_DWORD,   (ULONG_PTR) dwBigBufSize, NULL },
            { "iSendFlags", PT_FLAGS,   (ULONG_PTR) gdwDefSendFlags, aSendFlags },
        };
        FUNC_PARAM_HEADER paramsHeader =
            { 4, funcIndex, params, NULL };


        if (LetUserMungeParams (&paramsHeader))
        {
            if ((i = send(
                    (SOCKET) params[0].dwValue,
                    (char FAR *) params[1].dwValue,
                    (int) params[2].dwValue,
                    (int) params[3].dwValue

                    )) != SOCKET_ERROR)
            {
                ShowStr (gszXxxSUCCESS, aFuncNames[funcIndex]);
                ShowStr ("  NumBytesSent=x%x", i);
            }
            else
            {
                ShowError (funcIndex, 0);
            }
        }

        break;
    }
    case ws_sendto:
    {
        int    iTargetAddrLen;
        struct sockaddr FAR *pTargetAddr = MyAlloc (dwBigBufSize);
        FUNC_PARAM params[] =
        {
            { gszSocket,            PT_DWORD,   (ULONG_PTR) gpSelectedSocket->Sock, NULL },
            { "lpBuf",              PT_POINTER, (ULONG_PTR) pBigBuf, pBigBuf },
            { "iBufLength",         PT_DWORD,   (ULONG_PTR) dwBigBufSize, NULL },
            { "iSendFlags",         PT_FLAGS,   (ULONG_PTR) gdwDefSendFlags, aSendFlags },
            { "lpTargetAddr",       PT_POINTER, (ULONG_PTR) pTargetAddr, pTargetAddr },
            { "iTargetAddrLength",  PT_DWORD,   (ULONG_PTR) dwBigBufSize, NULL },
        };
        FUNC_PARAM_HEADER paramsHeader =
            { 6, funcIndex, params, NULL };


        if (LetUserMungeParams (&paramsHeader))
        {
            if ((i = sendto(
                    (SOCKET) params[0].dwValue,
                    (char FAR *) params[1].dwValue,
                    (int) params[2].dwValue,
                    (int) params[3].dwValue,
                    (struct sockaddr FAR *) params[4].dwValue,
                    (int) params[5].dwValue

                    )) != SOCKET_ERROR)
            {
                ShowStr (gszXxxSUCCESS, aFuncNames[funcIndex]);
                ShowStr ("  NumBytesSent=x%x", i);
            }
            else
            {
                ShowError (funcIndex, 0);
            }
        }

        break;
    }
    case ws_setsockopt:
    {
        int iOptionBufLength = dwBigBufSize;
        FUNC_PARAM params[] =
        {
            { gszSocket,            PT_DWORD,   (ULONG_PTR) gpSelectedSocket->Sock, NULL },
            { "iLevel",             PT_ORDINAL, (ULONG_PTR) 0, aSockOptLevels },
            { "iOptionName",        PT_ORDINAL, (ULONG_PTR) 0, aSockOpts },
            { "lpOptionBuf",        PT_POINTER, (ULONG_PTR) pBigBuf, pBigBuf },
            { "iOptionBufLength",   PT_DWORD,   (ULONG_PTR) dwBigBufSize, NULL },
        };
        FUNC_PARAM_HEADER paramsHeader =
            { 5, funcIndex, params, NULL };


        if (LetUserMungeParams (&paramsHeader))
        {
            if ((i = setsockopt(
                    (SOCKET) params[0].dwValue,
                    (int) params[1].dwValue,
                    (int) params[2].dwValue,
                    (char FAR *) params[3].dwValue,
                    (int) params[4].dwValue

                    )) == 0)
            {
                ShowStr (gszXxxSUCCESS, aFuncNames[funcIndex]);
            }
            else if (i == SOCKET_ERROR)
            {
                 ShowError (funcIndex, 0);
            }
            else
            {
                 ShowUnknownError (funcIndex, i);
            }
        }

        break;
    }
    case ws_shutdown:
    {
        FUNC_PARAM params[] =
        {
            { gszSocket,    PT_DWORD,   (ULONG_PTR) gpSelectedSocket->Sock, NULL },
            { "iHow",       PT_ORDINAL, (ULONG_PTR) 0, aShutdownOps },
        };
        FUNC_PARAM_HEADER paramsHeader =
            { 2, funcIndex, params, NULL };


        if (LetUserMungeParams (&paramsHeader))
        {
            if ((i = shutdown(
                    (SOCKET) params[0].dwValue,
                    (int) params[1].dwValue

                    )) == 0)
            {
                ShowStr (gszXxxSUCCESS, aFuncNames[funcIndex]);
            }
            else if (i == SOCKET_ERROR)
            {
                 ShowError (funcIndex, 0);
            }
            else
            {
                 ShowUnknownError (funcIndex, i);
            }
        }

        break;
    }
    case ws_socket:
    {
        FUNC_PARAM params[] =
        {
            { "address family", PT_ORDINAL, (ULONG_PTR) gdwDefAddrFamily, aAddressFamilies },
            { "socket type",    PT_ORDINAL, (ULONG_PTR) gdwDefSocketType, aSocketTypes },
            { "protocol",       PT_DWORD,   (ULONG_PTR) gdwDefProtocol, aProtocols },
        };
        FUNC_PARAM_HEADER paramsHeader =
            { 3, funcIndex, params, NULL };


        if (LetUserMungeParams (&paramsHeader))
        {
            PMYSOCKET pSocket;


            if (!(pSocket = MyAlloc (sizeof (MYSOCKET))))
            {
                break;
            }

            if ((pSocket->Sock = socket(
                    (int) params[0].dwValue,
                    (int) params[1].dwValue,
                    (int) params[2].dwValue

                    )) != INVALID_SOCKET)
            {
                char        buf[128];
                LRESULT     index;


                ShowStr (gszXxxSUCCESS, aFuncNames[funcIndex]);

                pSocket->dwAddressFamily = (DWORD) params[0].dwValue;
                pSocket->dwSocketType = (DWORD) params[1].dwValue;

                wsprintf(
                    buf,
                    "Socket=x%x (%s %s)",
                    pSocket->Sock,
                    GetStringFromOrdinalValue(
                        pSocket->dwAddressFamily,
                        aAddressFamilies
                        ),
                    GetStringFromOrdinalValue(
                        pSocket->dwSocketType,
                        aSocketTypes
                        )
                    );

                index = SendMessage(
                    ghwndList1,
                    LB_ADDSTRING,
                    0,
                    (LPARAM) buf
                    );

                SendMessage(
                    ghwndList1,
                    LB_SETITEMDATA,
                    index,
                    (LPARAM) pSocket
                    );
            }
            else
            {
                ShowError (funcIndex, 0);
                MyFree (pSocket);
            }
        }

        break;
    }
    case ws_WSAAccept:
    {
        int iAddrLen = dwBigBufSize;
        FUNC_PARAM params[] =
        {
            { gszSocket,            PT_DWORD,       (ULONG_PTR) gpSelectedSocket->Sock, NULL },
            { "lpAddr",             PT_PTRNOEDIT,   (ULONG_PTR) pBigBuf, pBigBuf },
            { "lpiAddrLen",         PT_DWORD,       (ULONG_PTR) &iAddrLen, &iAddrLen },
            { "lpfnCondition",      PT_DWORD,       (ULONG_PTR) ConditionProc, ConditionProc },
            { "dwCallbackData",     PT_DWORD,       (ULONG_PTR) 0, NULL }
        };
        FUNC_PARAM_HEADER paramsHeader =
            { 5, funcIndex, params, NULL };


        if (LetUserMungeParams (&paramsHeader))
        {
            SOCKET s;


            if ((s = WSAAccept(
                    (SOCKET) params[0].dwValue,
                    (struct sockaddr FAR *) params[1].dwValue,
                    (LPINT) params[2].dwValue,
                    (LPCONDITIONPROC) params[3].dwValue,
                    params[4].dwValue

                    )) != INVALID_SOCKET)
            {
                ShowStr (gszXxxSUCCESS, aFuncNames[funcIndex]);
                ShowStr ("  returned socket=x%x", s);

                if (params[1].dwValue  &&
                    params[2].dwValue  &&
                    *((LPINT) params[2].dwValue))
                {
                    struct sockaddr FAR *pSockAddr = (struct sockaddr FAR *)
                                            params[1].dwValue;


                    ShowStr(
                        "  lpAddr->AddressFamily=%d, %s",
                        (DWORD) pSockAddr->sa_family,
                        GetStringFromOrdinalValue(
                            (DWORD) pSockAddr->sa_family,
                            aAddressFamilies
                            )
                        );

                    ShowStr (  "lpAddr->sa_data=");
                    ShowBytes(
                        *((LPINT) params[2].dwValue) - sizeof (u_short),
                        pSockAddr->sa_data,
                        1
                        );
                }
            }
            else
            {
                 ShowError (funcIndex, 0);
            }
        }

        break;
    }
    case ws_WSAAddressToStringA:
    case ws_WSAAddressToStringW:
    {
        DWORD   dwAddrStrLen = dwBigBufSize;
        LPSTR   pszAddrStr = MyAlloc (dwBigBufSize);
        LPWSAPROTOCOL_INFOA pProtoInfo = (funcIndex == ws_WSAAddressToStringA ?
                                &gWSAProtocolInfoA :
                                (LPWSAPROTOCOL_INFOA) &gWSAProtocolInfoW);
        FUNC_PARAM params[] =
        {
            { "lpsaAddress",    PT_PTRNOEDIT,       (ULONG_PTR) pBigBuf, pBigBuf },
            { "  ->sa_family",  PT_ORDINAL,         (ULONG_PTR) gdwDefAddrFamily, aAddressFamilies },
            { "  ->sa_data",    PT_POINTER,         (ULONG_PTR) (pBigBuf + 2), pBigBuf + 2},
            { "dwAddressLength", PT_DWORD,          (ULONG_PTR) dwBigBufSize, NULL },
            { "lpProtocolInfo", PT_WSAPROTOCOLINFO, (ULONG_PTR) pProtoInfo, pProtoInfo },
            { "lpszAddressString", PT_POINTER,      (ULONG_PTR) pszAddrStr, pszAddrStr },
            { "lpdwAddressStringLength",PT_POINTER, (ULONG_PTR) &dwAddrStrLen, &dwAddrStrLen }
        };
        FUNC_PARAM_HEADER paramsHeader =
            { 7, funcIndex, params, NULL };


        if (LetUserMungeParams (&paramsHeader))
        {
            LPSOCKADDR pSockAddr = (LPSOCKADDR) params[0].dwValue;

            if (!IsBadWritePtr (pSockAddr, sizeof (WORD)))
            {
                pSockAddr->sa_family = (short) params[1].dwValue;
            }

            if (funcIndex == ws_WSAAddressToStringA)
            {
                i = WSAAddressToStringA(
                    (LPSOCKADDR) params[0].dwValue,
                    (DWORD) params[3].dwValue,
                    (LPWSAPROTOCOL_INFOA) params[4].dwValue,
                    (LPSTR) params[5].dwValue,
                    (LPDWORD) params[6].dwValue
                    );
            }
            else
            {
                i = WSAAddressToStringW(
                    (LPSOCKADDR) params[0].dwValue,
                    (DWORD) params[3].dwValue,
                    (LPWSAPROTOCOL_INFOW) params[4].dwValue,
                    (LPWSTR) params[5].dwValue,
                    (LPDWORD) params[6].dwValue
                    );
            }

            if (i == 0)
            {
                DWORD dwLength = *((LPDWORD) params[6].dwValue);


                ShowStr (gszXxxSUCCESS, aFuncNames[funcIndex]);
                ShowStr ("  *lpdwAddressStringLength=x%x", dwLength);

                ShowStr ("  *lpszAddressString=");
                ShowBytes(
                    (funcIndex == ws_WSAAddressToStringA ?
                        dwLength : dwLength * sizeof(WCHAR)),
                    (LPVOID) params[5].dwValue,
                    1
                    );
            }
            else if (i == SOCKET_ERROR)
            {
                 ShowError (funcIndex, 0);
            }
            else
            {
                 ShowUnknownError (funcIndex, i);
            }
        }

        break;
    }
    case ws_WSAAsyncGetHostByAddr:
    {
        PASYNC_REQUEST_INFO pAsyncReqInfo;


        if ((pAsyncReqInfo = MyAlloc(
                sizeof (ASYNC_REQUEST_INFO) + dwBigBufSize
                )))
        {
            char szHostAddr[MAX_STRING_PARAM_SIZE] = "1.2.3.4";
            FUNC_PARAM params[] =
            {
                { "hwnd",   PT_DWORD,       (ULONG_PTR) ghwndMain, NULL },
                { "wMsg",   PT_DWORD,       (ULONG_PTR) WM_ASYNCREQUESTCOMPLETED, NULL },
                { "addr",   PT_POINTER,     (ULONG_PTR) pBigBuf, pBigBuf },
                { "len",    PT_DWORD,       (ULONG_PTR) dwBigBufSize, NULL },
                { "type",   PT_ORDINAL,     (ULONG_PTR) gdwDefAddrFamily, aAddressFamilies },
                { "buf",    PT_PTRNOEDIT,   (ULONG_PTR) (pAsyncReqInfo+1), pAsyncReqInfo + 1 },
                { "buflen", PT_DWORD,       (ULONG_PTR) dwBigBufSize, NULL },
            };
            FUNC_PARAM_HEADER paramsHeader =
                { 7, funcIndex, params, NULL };


            if (LetUserMungeParams (&paramsHeader))
            {
                if ((pAsyncReqInfo->hRequest = WSAAsyncGetHostByAddr(
                        (HWND) params[0].dwValue,
                        (unsigned int) params[1].dwValue,
                        (char FAR *) params[2].dwValue,
                        (int) params[3].dwValue,
                        (int) params[4].dwValue,
                        (char FAR *) params[5].dwValue,
                        (int) params[6].dwValue
                        )))
                {
                    ShowStr(
                        "%s returned hRequest=x%x",
                        aFuncNames[funcIndex],
                        pAsyncReqInfo->hRequest
                        );

                    pAsyncReqInfo->pszFuncName = aFuncNames[funcIndex];
                    pAsyncReqInfo->FuncIndex = funcIndex;
                    QueueAsyncRequestInfo (pAsyncReqInfo);
                    break;
                }
                else
                {
                    ShowError (funcIndex, 0);
                }
            }

            MyFree (pAsyncReqInfo);
        }

        break;
    }
    case ws_WSAAsyncGetHostByName:
    {
        PASYNC_REQUEST_INFO pAsyncReqInfo;


        if ((pAsyncReqInfo = MyAlloc(
                sizeof (ASYNC_REQUEST_INFO) + dwBigBufSize
                )))
        {
            char szHostName[MAX_STRING_PARAM_SIZE];
            FUNC_PARAM params[] =
            {
                { "hwnd",   PT_DWORD,       (ULONG_PTR) ghwndMain, NULL },
                { "wMsg",   PT_DWORD,       (ULONG_PTR) WM_ASYNCREQUESTCOMPLETED, NULL },
                { "name",   PT_STRING,      (ULONG_PTR) szHostName, szHostName },
                { "buf",    PT_PTRNOEDIT,   (ULONG_PTR) (pAsyncReqInfo+1), pAsyncReqInfo + 1 },
                { "buflen", PT_DWORD,       (ULONG_PTR) dwBigBufSize, NULL },
            };
            FUNC_PARAM_HEADER paramsHeader =
                { 5, funcIndex, params, NULL };


            lstrcpyA (szHostName, gszDefHostName);

            if (LetUserMungeParams (&paramsHeader))
            {
                if ((pAsyncReqInfo->hRequest = WSAAsyncGetHostByName(
                        (HWND) params[0].dwValue,
                        (unsigned int) params[1].dwValue,
                        (char FAR *) params[2].dwValue,
                        (char FAR *) params[3].dwValue,
                        (int) params[4].dwValue
                        )))
                {
                    ShowStr(
                        "%s returned hRequest=x%x",
                        aFuncNames[funcIndex],
                        pAsyncReqInfo->hRequest
                        );

                    pAsyncReqInfo->pszFuncName = aFuncNames[funcIndex];
                    pAsyncReqInfo->FuncIndex = funcIndex;
                    QueueAsyncRequestInfo (pAsyncReqInfo);
                    break;
                }
                else
                {
                    ShowError (funcIndex, 0);
                }
            }

            MyFree (pAsyncReqInfo);
        }

        break;
    }
    case ws_WSAAsyncGetProtoByName:
    {
        PASYNC_REQUEST_INFO pAsyncReqInfo;


        if ((pAsyncReqInfo = MyAlloc(
                sizeof (ASYNC_REQUEST_INFO) + dwBigBufSize
                )))
        {
            char szProtoName[MAX_STRING_PARAM_SIZE];
            FUNC_PARAM params[] =
            {
                { "hwnd",   PT_DWORD,       (ULONG_PTR) ghwndMain, NULL },
                { "wMsg",   PT_DWORD,       (ULONG_PTR) WM_ASYNCREQUESTCOMPLETED, NULL },
                { "name",   PT_STRING,      (ULONG_PTR) szProtoName, szProtoName },
                { "buf",    PT_PTRNOEDIT,   (ULONG_PTR) (pAsyncReqInfo+1), pAsyncReqInfo + 1 },
                { "buflen", PT_DWORD,       (ULONG_PTR) dwBigBufSize, NULL },
            };
            FUNC_PARAM_HEADER paramsHeader =
                { 5, funcIndex, params, NULL };


            lstrcpyA (szProtoName, gszDefProtoName);

            if (LetUserMungeParams (&paramsHeader))
            {
                if ((pAsyncReqInfo->hRequest = WSAAsyncGetProtoByName(
                        (HWND) params[0].dwValue,
                        (unsigned int) params[1].dwValue,
                        (char FAR *) params[2].dwValue,
                        (char FAR *) params[3].dwValue,
                        (int) params[4].dwValue
                        )))
                {
                    ShowStr(
                        "%s returned hRequest=x%x",
                        aFuncNames[funcIndex],
                        pAsyncReqInfo->hRequest
                        );

                    pAsyncReqInfo->pszFuncName = aFuncNames[funcIndex];
                    pAsyncReqInfo->FuncIndex = funcIndex;
                    QueueAsyncRequestInfo (pAsyncReqInfo);
                    break;
                }
                else
                {
                    ShowError (funcIndex, 0);
                }
            }

            MyFree (pAsyncReqInfo);
        }

        break;
    }
    case ws_WSAAsyncGetProtoByNumber:
    {
        PASYNC_REQUEST_INFO pAsyncReqInfo;


        if ((pAsyncReqInfo = MyAlloc(
                sizeof (ASYNC_REQUEST_INFO) + dwBigBufSize
                )))
        {
            FUNC_PARAM params[] =
            {
                { "hwnd",   PT_DWORD,       (ULONG_PTR) ghwndMain, NULL },
                { "wMsg",   PT_DWORD,       (ULONG_PTR) WM_ASYNCREQUESTCOMPLETED, NULL },
                { "number", PT_DWORD,       (ULONG_PTR) gdwDefProtoNum, NULL },
                { "buf",    PT_PTRNOEDIT,   (ULONG_PTR) (pAsyncReqInfo+1), pAsyncReqInfo + 1 },
                { "buflen", PT_DWORD,       (ULONG_PTR) dwBigBufSize, NULL },
            };
            FUNC_PARAM_HEADER paramsHeader =
                { 5, funcIndex, params, NULL };


            if (LetUserMungeParams (&paramsHeader))
            {
                if ((pAsyncReqInfo->hRequest = WSAAsyncGetProtoByNumber(
                        (HWND) params[0].dwValue,
                        (unsigned int) params[1].dwValue,
                        (int) params[2].dwValue,
                        (char FAR *) params[3].dwValue,
                        (int) params[4].dwValue
                        )))
                {
                    ShowStr(
                        "%s returned hRequest=x%x",
                        aFuncNames[funcIndex],
                        pAsyncReqInfo->hRequest
                        );

                    pAsyncReqInfo->pszFuncName = aFuncNames[funcIndex];
                    pAsyncReqInfo->FuncIndex = funcIndex;
                    QueueAsyncRequestInfo (pAsyncReqInfo);
                    break;
                }
                else
                {
                    ShowError (funcIndex, 0);
                }
            }

            MyFree (pAsyncReqInfo);
        }

        break;
    }
    case ws_WSAAsyncGetServByName:
    {
        PASYNC_REQUEST_INFO pAsyncReqInfo;


        if ((pAsyncReqInfo = MyAlloc(
                sizeof (ASYNC_REQUEST_INFO) + dwBigBufSize
                )))
        {
            char szServName[MAX_STRING_PARAM_SIZE];
            char szProtoName[MAX_STRING_PARAM_SIZE];
            FUNC_PARAM params[] =
            {
                { "hwnd",   PT_DWORD,       (ULONG_PTR) ghwndMain, NULL },
                { "wMsg",   PT_DWORD,       (ULONG_PTR) WM_ASYNCREQUESTCOMPLETED, NULL },
                { "name",   PT_STRING,      (ULONG_PTR) szServName, szServName },
                { "proto",  PT_STRING,      (ULONG_PTR) 0, szProtoName },
                { "buf",    PT_PTRNOEDIT,   (ULONG_PTR) (pAsyncReqInfo+1), pAsyncReqInfo + 1 },
                { "buflen", PT_DWORD,       (ULONG_PTR) dwBigBufSize, NULL },
            };
            FUNC_PARAM_HEADER paramsHeader =
                { 6, funcIndex, params, NULL };


            lstrcpyA (szServName, gszDefServName);
            lstrcpyA (szProtoName, gszDefProtoName);

            if (LetUserMungeParams (&paramsHeader))
            {
                if ((pAsyncReqInfo->hRequest = WSAAsyncGetServByName(
                        (HWND) params[0].dwValue,
                        (unsigned int) params[1].dwValue,
                        (char FAR *) params[2].dwValue,
                        (char FAR *) params[3].dwValue,
                        (char FAR *) params[4].dwValue,
                        (int) params[5].dwValue
                        )))
                {
                    ShowStr(
                        "%s returned hRequest=x%x",
                        aFuncNames[funcIndex],
                        pAsyncReqInfo->hRequest
                        );

                    pAsyncReqInfo->pszFuncName = aFuncNames[funcIndex];
                    pAsyncReqInfo->FuncIndex = funcIndex;
                    QueueAsyncRequestInfo (pAsyncReqInfo);
                    break;
                }
                else
                {
                    ShowError (funcIndex, 0);
                }
            }

            MyFree (pAsyncReqInfo);
        }

        break;
    }
    case ws_WSAAsyncGetServByPort:
    {
        PASYNC_REQUEST_INFO pAsyncReqInfo;


        if ((pAsyncReqInfo = MyAlloc(
                sizeof (ASYNC_REQUEST_INFO) + dwBigBufSize
                )))
        {
            char szProtoName[MAX_STRING_PARAM_SIZE];
            FUNC_PARAM params[] =
            {
                { "hwnd",   PT_DWORD,       (ULONG_PTR) ghwndMain, NULL },
                { "wMsg",   PT_DWORD,       (ULONG_PTR) WM_ASYNCREQUESTCOMPLETED, NULL },
                { "port",   PT_DWORD,       (ULONG_PTR) gdwDefPortNum, NULL },
                { "proto",  PT_STRING,      (ULONG_PTR) 0, szProtoName },
                { "buf",    PT_PTRNOEDIT,   (ULONG_PTR) (pAsyncReqInfo+1), pAsyncReqInfo + 1 },
                { "buflen", PT_DWORD,       (ULONG_PTR) dwBigBufSize, NULL },
            };
            FUNC_PARAM_HEADER paramsHeader =
                { 6, funcIndex, params, NULL };


            lstrcpyA (szProtoName, gszDefProtoName);

            if (LetUserMungeParams (&paramsHeader))
            {
                if ((pAsyncReqInfo->hRequest = WSAAsyncGetServByPort(
                        (HWND) params[0].dwValue,
                        (unsigned int) params[1].dwValue,
                        (int) params[2].dwValue,
                        (char FAR *) params[3].dwValue,
                        (char FAR *) params[4].dwValue,
                        (int) params[5].dwValue
                        )))
                {
                    ShowStr(
                        "%s returned hRequest=x%x",
                        aFuncNames[funcIndex],
                        pAsyncReqInfo->hRequest
                        );

                    pAsyncReqInfo->pszFuncName = aFuncNames[funcIndex];
                    pAsyncReqInfo->FuncIndex = funcIndex;
                    QueueAsyncRequestInfo (pAsyncReqInfo);
                    break;
                }
                else
                {
                    ShowError (funcIndex, 0);
                }
            }

            MyFree (pAsyncReqInfo);
        }

        break;
    }
    case ws_WSAAsyncSelect:
    {
        FUNC_PARAM params[] =
        {
            { gszSocket,    PT_DWORD,   (ULONG_PTR) gpSelectedSocket->Sock, NULL },
            { "hwnd",       PT_DWORD,   (ULONG_PTR) ghwndMain, ghwndMain },
            { "msg",        PT_DWORD,   (ULONG_PTR) WM_NETWORKEVENT, NULL },
            { "lEvent",     PT_FLAGS,   (ULONG_PTR) 0, aNetworkEvents }
        };
        FUNC_PARAM_HEADER paramsHeader =
            { 4, funcIndex, params, NULL };


        if (LetUserMungeParams (&paramsHeader))
        {
            if ((i = WSAAsyncSelect(
                    (SOCKET) params[0].dwValue,
                    (HWND) params[1].dwValue,
                    (unsigned int) params[2].dwValue,
                    (long) params[3].dwValue

                    )) == 0)
            {
                ShowStr (gszXxxSUCCESS, aFuncNames[funcIndex]);
            }
            else if (i == SOCKET_ERROR)
            {
                 ShowError (funcIndex, 0);
            }
            else
            {
                 ShowUnknownError (funcIndex, i);
            }
        }

        break;
    }
    case ws_WSACancelAsyncRequest:
    {
        FUNC_PARAM params[] =
        {
            { "hRequest",   PT_DWORD,   (ULONG_PTR) (gpAsyncReqInfoList ? gpAsyncReqInfoList->hRequest : NULL), NULL }
        };
        FUNC_PARAM_HEADER paramsHeader =
            { 1, funcIndex, params, NULL };


        if (LetUserMungeParams (&paramsHeader))
        {
            if ((i = WSACancelAsyncRequest ((HANDLE) params[0].dwValue)) == 0)
            {
                ShowStr (gszXxxSUCCESS, aFuncNames[funcIndex]);
            }
            else if (i == SOCKET_ERROR)
            {
                 ShowError (funcIndex, 0);
            }
            else
            {
                 ShowUnknownError (funcIndex, i);
            }
        }

        break;
    }
//    case ws_WSACancelBlockingCall: not implemented in ws 2
//
//        break;

    case ws_WSACleanup:
    {
        FUNC_PARAM_HEADER paramsHeader =
            { 0, funcIndex, NULL, NULL };


        if (LetUserMungeParams (&paramsHeader))
        {
            if (WSACleanup() == 0)
            {
                char szButtonText[32];


                wsprintf (szButtonText, "Startup (%d)", --giCurrNumStartups);
                SetDlgItemText (ghwndMain, IDC_BUTTON1, szButtonText);

                ShowStr (gszXxxSUCCESS, aFuncNames[funcIndex]);

                if (giCurrNumStartups == 0)
                {
                    int iNumItems;


                    iNumItems = (int) SendMessage(
                        ghwndList1,
                        LB_GETCOUNT,
                        0,
                        0
                        );

                    for (i = 0; i < iNumItems; i++)
                    {
                        PMYSOCKET pSocket;


                        pSocket = (PMYSOCKET) SendMessage(
                            ghwndList1,
                            LB_GETITEMDATA,
                            0,
                            0
                            );

                        SendMessage (ghwndList1, LB_DELETESTRING, 0, 0);

                        closesocket (pSocket->Sock);

                        MyFree (pSocket);
                    }
                }
            }
            else
            {
                ShowError (funcIndex, 0);
            }
        }

        break;
    }
    case ws_WSACloseEvent:
    {
        FUNC_PARAM params[] =
        {
            { "hEvent",   PT_DWORD,   (ULONG_PTR) ghSelectedEvent, NULL }
        };
        FUNC_PARAM_HEADER paramsHeader =
            { 1, funcIndex, params, NULL };


        if (LetUserMungeParams (&paramsHeader))
        {
            if (WSACloseEvent ((WSAEVENT) params[0].dwValue) == TRUE)
            {
                ShowStr (gszXxxSUCCESS, aFuncNames[funcIndex]);

                if ((WSAEVENT) params[0].dwValue != ghSelectedEvent)
                {
                    //
                    // The user overrode the selected widget, so make sure
                    // we don't delete the wrong string from the list box
                    //

                    int         iNumItems;
                    WSAEVENT    hEvent;


                    iNumItems = (int) SendMessage(
                        ghwndList3,
                        LB_GETCOUNT,
                        0,
                        0
                        );

                    for (i = 0; i < iNumItems; i++)
                    {
                        hEvent = (WSAEVENT) SendMessage(
                            ghwndList3,
                            LB_GETITEMDATA,
                            i,
                            0
                            );

                        if ((WSAEVENT) params[0].dwValue == hEvent)
                        {
                            giSelectedEventIndex = (int) i;
                            break;
                        }
                    }

                    if (i == iNumItems)
                    {
                        ShowStr(
                            "Strange, couldn't find that hEvent=x%x in list",
                            params[0].dwValue
                            );

                        break;
                    }
                }

                SendMessage(
                    ghwndList3,
                    LB_DELETESTRING,
                    (WPARAM) giSelectedEventIndex,
                    0
                    );
            }
            else
            {
                 ShowError (funcIndex, 0);
            }
        }

        break;
    }
    case ws_WSAConnect:
    {
        LPBYTE      pBuf = MyAlloc (5 * dwBigBufSize + 2 * sizeof (QOS));
        LPSOCKADDR  pSA = (LPSOCKADDR) pBuf;
        LPBYTE      pCallerBuf = pBuf + dwBigBufSize,
                    pCalleeBuf = pBuf + 2 * dwBigBufSize;
        LPQOS       pSQOS = (LPQOS) (pBuf + 3 * dwBigBufSize),
                    pGQOS = (LPQOS) (pBuf + 4 * dwBigBufSize + sizeof (QOS));
        WSABUF      callerData, calleeData;
        FUNC_PARAM params[] =
        {
            { gszSocket,        PT_DWORD,       (ULONG_PTR) gpSelectedSocket->Sock, NULL },
            { "name",           PT_PTRNOEDIT,   (ULONG_PTR) pSA, pSA },
            { "  ->sa_family",  PT_ORDINAL,     (ULONG_PTR) gdwDefAddrFamily, aAddressFamilies },
            { "  ->sa_data",    PT_POINTER,     (ULONG_PTR) &pSA->sa_data, &pSA->sa_data },
            { "namelen",        PT_DWORD,       (ULONG_PTR) dwBigBufSize, NULL },
            { "lpCallerData",   PT_PTRNOEDIT,   (ULONG_PTR) &callerData, &callerData },
            { "  ->len",        PT_DWORD,       (ULONG_PTR) dwBigBufSize, NULL },
            { "  ->buf",        PT_POINTER,     (ULONG_PTR) pCallerBuf, pCallerBuf },
            { "lpCalleeData",   PT_PTRNOEDIT,   (ULONG_PTR) &calleeData, &calleeData },
            { "  ->len",        PT_DWORD,       (ULONG_PTR) dwBigBufSize, NULL },
            { "  ->buf",        PT_POINTER,     (ULONG_PTR) pCalleeBuf, pCalleeBuf },
            { "lpSQOS",         PT_QOS,         (ULONG_PTR) pSQOS, pSQOS },
            { "lpGQOS",         PT_QOS,         (ULONG_PTR) pGQOS, pGQOS },
        };
        FUNC_PARAM_HEADER paramsHeader =
            { 13, funcIndex, params, NULL };


        if (!pBuf)
        {
            break;
        }

        ZeroMemory (pSQOS, sizeof (QOS));
        pSQOS->ProviderSpecific.len = dwBigBufSize;

        ZeroMemory (pGQOS, sizeof (QOS));
        pGQOS->ProviderSpecific.len = dwBigBufSize;

        if (LetUserMungeParams (&paramsHeader))
        {
            pSA->sa_family = (u_short) LOWORD(params[2].dwValue);
            callerData.len = (DWORD) params[6].dwValue;
            calleeData.len = (DWORD) params[9].dwValue;

            if ((i = WSAConnect(
                (SOCKET) params[0].dwValue,
                (LPSOCKADDR) params[1].dwValue,
                (int) params[4].dwValue,
                (LPWSABUF) params[5].dwValue,
                (LPWSABUF) params[8].dwValue,
                (LPQOS) params[11].dwValue,
                (LPQOS) params[12].dwValue

                )) == 0)
            {
                LPWSABUF pCalleeData = (LPWSABUF) params[8].dwValue;


                ShowStr (gszXxxSUCCESS, aFuncNames[funcIndex]);
                ShowStr ("  lpCalleeData->len=x%x", pCalleeData->len);

                if (pCalleeData->len)
                {
                    ShowStr ("  lpCalleeData->buf=x%x", pCalleeData->buf);
                    ShowBytes (pCalleeData->len, pCalleeData->buf, 2);
                }
            }
            else if (i == SOCKET_ERROR)
            {
                ShowError (funcIndex, 0);
            }
            else
            {
                ShowUnknownError (funcIndex, i);
            }

        }

        MyFree (pBuf);

        break;
    }
    case ws_WSACreateEvent:
    {
        FUNC_PARAM_HEADER paramsHeader =
            { 0, funcIndex, NULL, NULL };


        if (LetUserMungeParams (&paramsHeader))
        {
            WSAEVENT    wsaEvent;

            if ((wsaEvent = WSACreateEvent ()) != WSA_INVALID_EVENT)
            {
                char    buf[20];
                LRESULT index;


                ShowStr (gszXxxSUCCESS, aFuncNames[funcIndex]);
                ShowStr ("  wsaEvent=x%x", wsaEvent);

                wsprintf (buf, "hEvent=x%x", wsaEvent);

                index = SendMessage(
                    ghwndList3,
                    LB_ADDSTRING,
                    0,
                    (LPARAM) buf
                    );

                SendMessage(
                    ghwndList3,
                    LB_SETITEMDATA,
                    (WPARAM) index,
                    (LPARAM) wsaEvent
                    );
            }
            else
            {
                 ShowError (funcIndex, 0);
            }
        }

        break;
    }
    case ws_WSADuplicateSocketA:
    case ws_WSADuplicateSocketW:
    {
        LPWSAPROTOCOL_INFOA pProtoInfo = (gbWideStringParams ?
                                &gWSAProtocolInfoA :
                                (LPWSAPROTOCOL_INFOA) &gWSAProtocolInfoW);
        FUNC_PARAM params[] =
        {
            { gszSocket,        PT_DWORD,       (ULONG_PTR) gpSelectedSocket->Sock, NULL },
            { "hTargetProcess", PT_DWORD,       (ULONG_PTR) 0, NULL },
            { "lpProtocolInfo", PT_PTRNOEDIT,   (ULONG_PTR) pProtoInfo, pProtoInfo}
        };
        FUNC_PARAM_HEADER paramsHeader =
            { 3, funcIndex, params, NULL };


        if (LetUserMungeParams (&paramsHeader))
        {
            if (gbWideStringParams)
            {
                i = WSADuplicateSocketW(
                    (SOCKET) params[0].dwValue,
                    (DWORD) params[1].dwValue,
                    (LPWSAPROTOCOL_INFOW) params[2].dwValue
                    );
            }
            else
            {
                i = WSADuplicateSocketA(
                    (SOCKET) params[0].dwValue,
                    (DWORD) params[1].dwValue,
                    (LPWSAPROTOCOL_INFOA) params[2].dwValue
                    );
            }

            if (i == 0)
            {
                ShowStr (gszXxxSUCCESS, aFuncNames[funcIndex]);
                ShowStr ("  lpProtocolInfo=x%x", params[2].dwValue);
                ShowProtoInfo(
                    (LPWSAPROTOCOL_INFOA) params[2].dwValue,
                    0xffffffff,
                    !gbWideStringParams
                    );
            }
            else if (i == SOCKET_ERROR)
            {
                ShowError (funcIndex, 0);
            }
            else
            {
                ShowUnknownError (funcIndex, i);
            }
        }

        break;
    }
    case ws_WSAEnumNameSpaceProvidersA:
    case ws_WSAEnumNameSpaceProvidersW:
    {
        DWORD dwSize = dwBigBufSize;
        FUNC_PARAM params[] =
        {
            { "lpdwBufferLength",   PT_POINTER,     (ULONG_PTR) &dwSize, &dwSize },
            { "lpNSPBuffer",        PT_PTRNOEDIT,   (ULONG_PTR) pBigBuf, pBigBuf }
        };
        FUNC_PARAM_HEADER paramsHeader =
            { 2, funcIndex, params, NULL };


        if (LetUserMungeParams (&paramsHeader))
        {
            if (funcIndex == ws_WSAEnumNameSpaceProvidersA)
            {
                i = WSAEnumNameSpaceProvidersA(
                    (LPDWORD) params[0].dwValue,
                    (LPWSANAMESPACE_INFOA) params[1].dwValue
                    );
            }
            else
            {
                i = WSAEnumNameSpaceProvidersW(
                    (LPDWORD) params[0].dwValue,
                    (LPWSANAMESPACE_INFOW) params[1].dwValue
                    );
            }

            if (i != SOCKET_ERROR)
            {
                LPWSANAMESPACE_INFOA    pInfo = (LPWSANAMESPACE_INFOA)
                                            params[1].dwValue;


                ShowStr (gszXxxSUCCESS, aFuncNames[funcIndex]);

                ShowStr(
                    "  *lpdwBufferLength=x%x",
                    *((LPDWORD) params[0].dwValue)
                    );

                for (j = 0; j < i; j++, pInfo++)
                {
                    char szNSInfoN[16];


                    wsprintf (szNSInfoN, "  nsInfo[%d].", j);

                    ShowStr ("%sNSProviderId=", szNSInfoN);
                    ShowStr(
                        "    %x %x %x %02x%02x%02x%02x%02x%02x%02x%02x",
                        pInfo->NSProviderId.Data1,
                        pInfo->NSProviderId.Data2,
                        pInfo->NSProviderId.Data3,
                        (DWORD) pInfo->NSProviderId.Data4[0],
                        (DWORD) pInfo->NSProviderId.Data4[1],
                        (DWORD) pInfo->NSProviderId.Data4[2],
                        (DWORD) pInfo->NSProviderId.Data4[3],
                        (DWORD) pInfo->NSProviderId.Data4[4],
                        (DWORD) pInfo->NSProviderId.Data4[5],
                        (DWORD) pInfo->NSProviderId.Data4[6],
                        (DWORD) pInfo->NSProviderId.Data4[7]
                        );

                    ShowStr(
                        "%sdwNameSpace=%d, %s",
                        szNSInfoN,
                        pInfo->dwNameSpace,
                        GetStringFromOrdinalValue(
                            pInfo->dwNameSpace,
                            aNameSpaces
                            )
                        );

                    ShowStr(
                        "%sfActive=%s",
                        szNSInfoN,
                        (pInfo->fActive ? "TRUE" : "FALSE")
                        );

                    ShowStr ("%sdwVersion=x%x", szNSInfoN, pInfo->dwVersion);

                    ShowStr(
                        (funcIndex == ws_WSAEnumNameSpaceProvidersA ?
                            "%slpszIdentifier=%s" : "%slpszIdentifier=%ws"),
                        szNSInfoN,
                        pInfo->lpszIdentifier
                        );
                }
            }
            else
            {
                 ShowError (funcIndex, 0);

                 if (WSAGetLastError() == WSAEFAULT)
                 {
                    ShowStr(
                        "  *lpdwBufferLength=x%x",
                        *((LPDWORD) params[0].dwValue)
                        );
                 }
            }
        }

        break;
    }
    case ws_WSAEnumNetworkEvents:
    {
        WSANETWORKEVENTS netEvents;
        FUNC_PARAM params[] =
        {
            { gszSocket,            PT_DWORD,       (ULONG_PTR) gpSelectedSocket->Sock, NULL },
            { "hEvent",             PT_DWORD,       (ULONG_PTR) ghSelectedEvent, NULL },
            { "lpNetworkEvents",    PT_PTRNOEDIT,   (ULONG_PTR) &netEvents, &netEvents }
        };
        FUNC_PARAM_HEADER paramsHeader =
            { 3, funcIndex, params, NULL };


        if (LetUserMungeParams (&paramsHeader))
        {
            if ((i = WSAEnumNetworkEvents(
                    (SOCKET) params[0].dwValue,
                    (WSAEVENT) params[1].dwValue,
                    (LPWSANETWORKEVENTS) params[2].dwValue

                    )) == 0)
            {
                LPWSANETWORKEVENTS  pNetEvents = (LPWSANETWORKEVENTS)
                                        params[2].dwValue;

                ShowStr (gszXxxSUCCESS, aFuncNames[funcIndex]);
                ShowStr ("  lpNetworkEvents=x%x", pNetEvents);
                ShowStr(
                    "    ->lNetworkEvents=x%x",
                    pNetEvents->lNetworkEvents
                    );

                ShowStr(
                    "    ->iErrorCode[READ]=x%x",
                    pNetEvents->iErrorCode[FD_READ_BIT]
                    );

                ShowStr(
                    "    ->iErrorCode[WRITE]=x%x",
                    pNetEvents->iErrorCode[FD_WRITE_BIT]
                    );

                ShowStr(
                    "    ->iErrorCode[OOB]=x%x",
                    pNetEvents->iErrorCode[FD_OOB_BIT]
                    );

                ShowStr(
                    "    ->iErrorCode[ACCEPT]=x%x",
                    pNetEvents->iErrorCode[FD_ACCEPT_BIT]
                    );

                ShowStr(
                    "    ->iErrorCode[CONNECT]=x%x",
                    pNetEvents->iErrorCode[FD_CONNECT_BIT]
                    );

                ShowStr(
                    "    ->iErrorCode[CLOSE]=x%x",
                    pNetEvents->iErrorCode[FD_CLOSE_BIT]
                    );

                ShowStr(
                    "    ->iErrorCode[QOS]=x%x",
                    pNetEvents->iErrorCode[FD_QOS_BIT]
                    );

                ShowStr(
                    "    ->iErrorCode[GROUP_QOS]=x%x",
                    pNetEvents->iErrorCode[FD_GROUP_QOS_BIT]
                    );

            }
            else if (i == SOCKET_ERROR)
            {
                ShowError (funcIndex, 0);
            }
            else
            {
                ShowUnknownError (funcIndex, i);
            }
        }

        break;
    }
    case ws_WSAEnumProtocolsA:
    case ws_WSAEnumProtocolsW:
    {
        int aProtocols[32];
        DWORD dwSize = dwBigBufSize;
        FUNC_PARAM params[] =
        {
            { "lpiProtocols",       PT_POINTER,     (ULONG_PTR) NULL, aProtocols },
            { "lpProtocolBuffer",   PT_PTRNOEDIT,   (ULONG_PTR) pBigBuf, pBigBuf },
            { "lpdwBufferLength",   PT_POINTER,     (ULONG_PTR) &dwSize, &dwSize }
        };
        FUNC_PARAM_HEADER paramsHeader =
            { 3, funcIndex, params, NULL };


        if (LetUserMungeParams (&paramsHeader))
        {
            if (funcIndex == ws_WSAEnumProtocolsA)
            {
                i = WSAEnumProtocolsA(
                    (LPINT) params[0].dwValue,
                    (LPWSAPROTOCOL_INFOA) params[1].dwValue,
                    (LPDWORD) params[2].dwValue
                    );
            }
            else
            {
                i = WSAEnumProtocolsW(
                    (LPINT) params[0].dwValue,
                    (LPWSAPROTOCOL_INFOW) params[1].dwValue,
                    (LPDWORD) params[2].dwValue
                    );
            }

            if (i != SOCKET_ERROR)
            {
                LPWSAPROTOCOL_INFOA pInfo = (LPWSAPROTOCOL_INFOA)
                                        params[1].dwValue;


                UpdateResults (TRUE);

                ShowStr (gszXxxSUCCESS, aFuncNames[funcIndex]);

                ShowStr ("%s SUCCESS (result=%d)", aFuncNames[funcIndex], i);

                ShowStr(
                    "  *lpdwBufferLength=x%x",
                    *((LPDWORD) params[2].dwValue)
                    );

                for (j = 0; j < i; j++)
                {
                    if (funcIndex == ws_WSAEnumProtocolsA)
                    {
                        ShowProtoInfo (pInfo, j, TRUE);

                        pInfo++;
                    }
                    else
                    {
                        ShowProtoInfo (pInfo, j, FALSE);

                        pInfo = (LPWSAPROTOCOL_INFOA)
                            (((LPBYTE) pInfo) + sizeof (WSAPROTOCOL_INFOW));

                    }
                }

                UpdateResults (FALSE);
            }
            else
            {
                 if (ShowError (funcIndex, 0) == WSAENOBUFS)
                 {
                     dwSize = *((LPDWORD) params[2].dwValue);

                     ShowStr ("  *lpdwBufferLength=%d (x%lx)", dwSize, dwSize);
                 }
            }
        }

        break;
    }
    case ws_WSAEventSelect:
    {
        WSAPROTOCOL_INFOW   info;
        FUNC_PARAM params[] =
        {
            { gszSocket,        PT_DWORD,   (ULONG_PTR) gpSelectedSocket->Sock, NULL },
            { "hEvent",         PT_DWORD,   (ULONG_PTR) ghSelectedEvent, NULL },
            { "lNetworkEvents", PT_FLAGS,   (ULONG_PTR) 0, aNetworkEvents }
        };
        FUNC_PARAM_HEADER paramsHeader =
            { 3, funcIndex, params, NULL };


        if (LetUserMungeParams (&paramsHeader))
        {
            if ((i = WSAEventSelect(
                    (SOCKET) params[0].dwValue,
                    (WSAEVENT) params[1].dwValue,
                    (long) params[2].dwValue

                    )) == 0)
            {
                ShowStr (gszXxxSUCCESS, aFuncNames[funcIndex]);
            }
            else if (i == SOCKET_ERROR)
            {
                ShowError (funcIndex, 0);
            }
            else
            {
                ShowUnknownError (funcIndex, i);
            }
        }

        break;
    }
    case ws_WSAGetLastError:
    {
        FUNC_PARAM_HEADER paramsHeader =
            { 0, funcIndex, NULL, NULL };


        if (LetUserMungeParams (&paramsHeader))
        {
            WSAEVENT    wsaEvent;

            i = (DWORD) WSAGetLastError ();
            ShowStr (gszXxxSUCCESS, aFuncNames[funcIndex]);

            ShowStr(
                "  LastError=%d, %s",
                i,
                GetStringFromOrdinalValue (i, aWSAErrors)
                );
        }

        break;
    }
    case ws_WSAGetOverlappedResult:
    {
        DWORD   cbTransfer = 0, dwFlags = 0;
        FUNC_PARAM params[] =
        {
            { gszSocket,        PT_DWORD,       (ULONG_PTR) gpSelectedSocket->Sock, NULL },
            { "lpOverlapped",   PT_DWORD,       (ULONG_PTR) 0, NULL },
            { "lpcbTransfer",   PT_PTRNOEDIT,   (ULONG_PTR) &cbTransfer, &cbTransfer },
            { "fWait",          PT_DWORD,       (ULONG_PTR) 0, NULL },
            { "lpdwFlags",      PT_PTRNOEDIT,   (ULONG_PTR) &dwFlags, &dwFlags }
        };
        FUNC_PARAM_HEADER paramsHeader =
            { 5, funcIndex, params, NULL };


        if (LetUserMungeParams (&paramsHeader))
        {
            if (WSAGetOverlappedResult(
                    (SOCKET) params[0].dwValue,
                    (LPWSAOVERLAPPED)  params[1].dwValue,
                    (LPDWORD)  params[2].dwValue,
                    (BOOL)  params[3].dwValue,
                    (LPDWORD)  params[4].dwValue

                    ) == TRUE)
            {
                PMYOVERLAPPED pOverlapped = (PMYOVERLAPPED) params[1].dwValue;


                ShowStr (gszXxxSUCCESS, aFuncNames[funcIndex]);
                ShowStr(
                    "  Overlapped %s completed",
                    aFuncNames[pOverlapped->FuncIndex]
                    );

                ShowStr(
                    "  *lpcbTransfer=x%x",
                    *((LPDWORD)  params[2].dwValue)
                    );

                ShowFlags(
                    *((LPDWORD)  params[4].dwValue),
                    "  *lpdwFlags=",
                    aWSASendAndRecvFlags
                    );

                switch (pOverlapped->FuncIndex)
                {
                case ws_WSAIoctl:

                    if (*((LPDWORD)  params[2].dwValue))
                    {
                        ShowStr ("  lpvOUTBuffer=x%x", (pOverlapped + 1));
                        ShowBytes(
                            *((LPDWORD)  params[2].dwValue),
                            (pOverlapped + 1),
                            2
                            );
                    }

                    break;

                case ws_WSARecv:
                case ws_WSARecvFrom:
                {
                    LPWSABUF    pWSABuf = (LPWSABUF) (pOverlapped + 1);


                    for (i = 0; i < (int) pOverlapped->dwFuncSpecific1; i++)
                    {
                        ShowStr ("  wsaBuf[0].buf=x%x", pWSABuf->buf);
                        ShowBytes (pWSABuf->len, pWSABuf->buf, 2);
                        pWSABuf++;
                    }

                    break;
                }
                }

                MyFree (pOverlapped);
            }
            else
            {
                ShowError (funcIndex, 0);
            }
        }

// BUGBUG MyFree (pOverlapped); as appropriate

        break;
    }
    case ws_WSAGetQOSByName:
    {
        LPQOS       pQOS = (LPQOS) pBigBuf;
        LPWSABUF    pQOSName = (LPWSABUF)
                        MyAlloc (dwBigBufSize + sizeof (WSABUF));
        FUNC_PARAM params[] =
        {
            { gszSocket,    PT_DWORD,       (ULONG_PTR) gpSelectedSocket->Sock, NULL },
            { "lpQOSName",  PT_PTRNOEDIT,   (ULONG_PTR) pQOSName, pQOSName },
            { "  ->len",    PT_DWORD,       (ULONG_PTR) dwBigBufSize, NULL },
            { "  ->buf",    PT_POINTER,     (ULONG_PTR) (pQOSName+1), (pQOSName+1) },
            { "lpQOS",      PT_PTRNOEDIT,   (ULONG_PTR) pQOS, pQOS }
        };
        FUNC_PARAM_HEADER paramsHeader =
            { 5, funcIndex, params, NULL };

        if (!pQOSName)
        {
            break;
        }

        if (LetUserMungeParams (&paramsHeader))
        {
            pQOSName->len = (u_long) params[2].dwValue;
            pQOSName->buf = (char FAR *) params[3].dwValue;

            if (WSAGetQOSByName(
                    (SOCKET) params[0].dwValue,
                    (LPWSABUF) params[1].dwValue,
                    (LPQOS) params[4].dwValue

                    ) == TRUE)
            {
                ShowStr (gszXxxSUCCESS, aFuncNames[funcIndex]);
                ShowStr ("  lpQOS=x%x", pQOS);

                ShowStr(
                    "    ->SendingFlowspec.TokenRate=x%x",
                    pQOS->SendingFlowspec.TokenRate
                    );
                ShowStr(
                    "    ->SendingFlowspec.TokenBucketSize=x%x",
                    pQOS->SendingFlowspec.TokenBucketSize
                    );
                ShowStr(
                    "    ->SendingFlowspec.PeakBandwidth=x%x",
                    pQOS->SendingFlowspec.PeakBandwidth
                    );
                ShowStr(
                    "    ->SendingFlowspec.Latency=x%x",
                    pQOS->SendingFlowspec.Latency
                    );
                ShowStr(
                    "    ->SendingFlowspec.DelayVariation=x%x",
                    pQOS->SendingFlowspec.DelayVariation
                    );
                ShowStr(
                    "    ->SendingFlowspec.ServiceType=%s (x%x)",
                    GetStringFromOrdinalValue(
                        (DWORD) pQOS->SendingFlowspec.ServiceType,
                        aQOSServiceTypes
                        ),
                    (DWORD) pQOS->SendingFlowspec.ServiceType
                    );
                ShowStr(
                    "    ->SendingFlowspec.MaxSduSize=x%x",
                    pQOS->SendingFlowspec.MaxSduSize
                    );
                ShowStr(
                    "    ->SendingFlowspec.MinimumPolicedSize=x%x",
                    pQOS->SendingFlowspec.MinimumPolicedSize
                    );

                ShowStr(
                    "    ->ReceivingFlowspec.TokenRate=x%x",
                    pQOS->ReceivingFlowspec.TokenRate
                    );
                ShowStr(
                    "    ->ReceivingFlowspec.TokenBucketSize=x%x",
                    pQOS->ReceivingFlowspec.TokenBucketSize
                    );
                ShowStr(
                    "    ->ReceivingFlowspec.PeakBandwidth=x%x",
                    pQOS->ReceivingFlowspec.PeakBandwidth
                    );
                ShowStr(
                    "    ->ReceivingFlowspec.Latency=x%x",
                    pQOS->ReceivingFlowspec.Latency
                    );
                ShowStr(
                    "    ->ReceivingFlowspec.DelayVariation=x%x",
                    pQOS->ReceivingFlowspec.DelayVariation
                    );
                ShowStr(
                    "    ->ReceivingFlowspec.ServiceType=%s (x%x)",
                    GetStringFromOrdinalValue(
                        (DWORD) pQOS->ReceivingFlowspec.ServiceType,
                        aQOSServiceTypes
                        ),
                    (DWORD) pQOS->SendingFlowspec.ServiceType
                    );
                ShowStr(
                    "    ->ReceivingFlowspec.MaxSduSize=x%x",
                    pQOS->ReceivingFlowspec.MaxSduSize
                    );
                ShowStr(
                    "    ->ReceivingFlowspec.MinimumPolicedSize=x%x",
                    pQOS->ReceivingFlowspec.MinimumPolicedSize
                    );

                ShowStr(
                    "    ->ProviderSpecific.len=x%x",
                    pQOS->ProviderSpecific.len
                    );

                if (pQOS->ProviderSpecific.len)
                {
                    ShowStr(
                        "    ->ProviderSpecific.buf=x%x",
                        pQOS->ProviderSpecific.buf
                        );

                    ShowBytes(
                        pQOS->ProviderSpecific.len,
                        pQOS->ProviderSpecific.buf,
                        3
                        );
                }
            }
            else
            {
                ShowError (funcIndex, 0);
            }
        }

        MyFree (pQOSName);

        break;
    }
    case ws_WSAGetServiceClassInfoA:
    case ws_WSAGetServiceClassInfoW:
    {
        DWORD   dwSize = dwBigBufSize;
        GUID    ProviderId, ServiceClassId;
        FUNC_PARAM params[] =
        {
            { "lpProviderId",       PT_POINTER,     (ULONG_PTR) &ProviderId, &ProviderId },
            { "lpServiceClassId",   PT_POINTER,     (ULONG_PTR) &ServiceClassId, &ServiceClassId },
            { "lpdwBufSize",        PT_POINTER,     (ULONG_PTR) &dwSize, &dwSize },
            { "lpServiceClassInfo", PT_PTRNOEDIT,   (ULONG_PTR) pBigBuf, pBigBuf }
        };
        FUNC_PARAM_HEADER paramsHeader =
            { 4, funcIndex, params, NULL };


        if (LetUserMungeParams (&paramsHeader))
        {
            if (funcIndex == ws_WSAGetServiceClassInfoA)
            {
                i = WSAGetServiceClassInfoA(
                    (LPGUID) params[0].dwValue,
                    (LPGUID) params[1].dwValue,
                    (LPDWORD) params[2].dwValue,
                    (LPWSASERVICECLASSINFOA) params[3].dwValue
                    );
            }
            else
            {
                i = WSAGetServiceClassInfoW(
                    (LPGUID) params[0].dwValue,
                    (LPGUID) params[1].dwValue,
                    (LPDWORD) params[2].dwValue,
                    (LPWSASERVICECLASSINFOW) params[3].dwValue
                    );
            }

            if (i == 0)
            {
                LPWSANSCLASSINFOA       pClassInfo;
                LPWSASERVICECLASSINFOA  pInfo = (LPWSASERVICECLASSINFOA)
                                            params[3].dwValue;


                ShowStr (gszXxxSUCCESS, aFuncNames[funcIndex]);
                ShowStr ("  lpServiceClassInfo=x%x", pInfo);
                ShowStr(
                    "    ->lpServiceClassId=%x %x %x %02x%02x%02x%02x%02x%02x%02x%02x",
                    pInfo->lpServiceClassId->Data1,
                    pInfo->lpServiceClassId->Data2,
                    pInfo->lpServiceClassId->Data3,
                    (DWORD) pInfo->lpServiceClassId->Data4[0],
                    (DWORD) pInfo->lpServiceClassId->Data4[1],
                    (DWORD) pInfo->lpServiceClassId->Data4[2],
                    (DWORD) pInfo->lpServiceClassId->Data4[3],
                    (DWORD) pInfo->lpServiceClassId->Data4[4],
                    (DWORD) pInfo->lpServiceClassId->Data4[5],
                    (DWORD) pInfo->lpServiceClassId->Data4[6],
                    (DWORD) pInfo->lpServiceClassId->Data4[7]
                    );

                ShowStr(
                    (funcIndex == ws_WSAGetServiceClassInfoA ?
                        "    ->lpszServiceClassName=%s" :
                        "    ->lpszServiceClassName=%ws"),
                    pInfo->lpszServiceClassName
                    );

                ShowStr ("    ->dwCount=%d", pInfo->dwCount);

                pClassInfo = pInfo->lpClassInfos;

                for (i = 0; i < (int) pInfo->dwCount; i++, pClassInfo++)
                {
                    char szClassInfoN[32];


                    wsprintf (szClassInfoN, "    ->ClassInfos[%d].", i);

                    ShowStr(
                        (funcIndex == ws_WSAGetServiceClassInfoA ?
                            "%s.lpszName=%s" :
                            "%s.lpszName=%ws"),
                         szClassInfoN,
                         pClassInfo->lpszName
                         );

                    ShowStr(
                        "%sdwNameSpace=%d, %s",
                        szClassInfoN,
                        pClassInfo->dwNameSpace,
                        GetStringFromOrdinalValue(
                            pClassInfo->dwNameSpace,
                            aNameSpaces
                            )
                        );

                    ShowStr(
                        "%sdwValueType=%d",
                        szClassInfoN,
                        pClassInfo->dwValueType
                        ); // BUGBUG supposed to be flags?

                    ShowStr(
                        "%sdwValueSize=%d",
                        szClassInfoN,
                        pClassInfo->dwValueSize
                        );

                    ShowStr(
                        "%slpValue=x%x",
                        szClassInfoN,
                        pClassInfo->lpValue
                        );

                    ShowBytes(
                        pClassInfo->dwValueSize,
                        pClassInfo->lpValue,
                        3
                        );
                }
            }
            else if (i == SOCKET_ERROR)
            {
                ShowError (funcIndex, 0);
            }
            else
            {
                ShowUnknownError (funcIndex, i);
            }
        }

        break;
    }
    case ws_WSAGetServiceClassNameByClassIdA:
    case ws_WSAGetServiceClassNameByClassIdW:
    {
        DWORD   dwLength = dwBigBufSize;
        GUID    guid[2];
        FUNC_PARAM params[] =
        {
            { "lpServiceClassId",   PT_POINTER,     (ULONG_PTR) guid, guid },
            { "lpServiceClassName", PT_PTRNOEDIT,   (ULONG_PTR) pBigBuf, pBigBuf },
            { "lpdwBufferLength",   PT_POINTER,     (ULONG_PTR) &dwLength, &dwLength }
        };
        FUNC_PARAM_HEADER paramsHeader =
            { 3, funcIndex, params, NULL };


        if (LetUserMungeParams (&paramsHeader))
        {
            if (funcIndex == ws_WSAGetServiceClassNameByClassIdA)
            {
                i = WSAGetServiceClassNameByClassIdA(
                    (LPGUID) params[0].dwValue,
                    (LPSTR) params[1].dwValue,
                    (LPDWORD) params[2].dwValue
                    );
            }
            else
            {
                i = WSAGetServiceClassNameByClassIdW(
                    (LPGUID) params[0].dwValue,
                    (LPWSTR) params[1].dwValue,
                    (LPDWORD) params[2].dwValue
                    );
            }

            if (i == 0)
            {
                ShowStr (gszXxxSUCCESS, aFuncNames[funcIndex]);
                ShowStr(
                    "  *lpdwBufferLength=x%x",
                    *((LPDWORD) params[2].dwValue)
                    );
                if (funcIndex == ws_WSAGetServiceClassNameByClassIdA)
                {
                    ShowStr ("  *lpServiceClassName=%s", params[1].dwValue);
                }
                else
                {
                    ShowStr ("  *lpServiceClassName=%ws", params[1].dwValue);
                }
            }
            else if (i == SOCKET_ERROR)
            {
                ShowError (funcIndex, 0);
            }
            else
            {
                ShowUnknownError (funcIndex, i);
            }
        }

        break;
    }
    case ws_WSAHtonl:
    {
        u_long  ulNetLong;
        FUNC_PARAM params[] =
        {
            { gszSocket,    PT_DWORD,       (ULONG_PTR) gpSelectedSocket->Sock, NULL },
            { "hostLong",   PT_DWORD,       (ULONG_PTR) 0, NULL },
            { "lpNetLong",  PT_PTRNOEDIT,   (ULONG_PTR) &ulNetLong, &ulNetLong }
        };
        FUNC_PARAM_HEADER paramsHeader =
            { 3, funcIndex, params, NULL };


        if (LetUserMungeParams (&paramsHeader))
        {
            if ((i = WSAHtonl(
                    (SOCKET) params[0].dwValue,
                    (u_long) params[1].dwValue,
                    (u_long *) params[2].dwValue

                    )) == 0)
            {
                ShowStr (gszXxxSUCCESS, aFuncNames[funcIndex]);
                ShowStr ("  hostLong=x%x", params[1].dwValue);
                ShowStr ("  *lpNetLong=x%x", *((u_long *) params[2].dwValue));
            }
            else if (i == SOCKET_ERROR)
            {
                ShowError (funcIndex, 0);
            }
            else
            {
                ShowUnknownError (funcIndex, i);
            }
        }

        break;
    }
    case ws_WSAHtons:
    {
        u_short usNetShort;
        FUNC_PARAM params[] =
        {
            { gszSocket,    PT_DWORD,       (ULONG_PTR) gpSelectedSocket->Sock, NULL },
            { "hostShort",  PT_DWORD,       (ULONG_PTR) 0, NULL },
            { "lpNetShort", PT_PTRNOEDIT,   (ULONG_PTR) &usNetShort, &usNetShort }
        };
        FUNC_PARAM_HEADER paramsHeader =
            { 3, funcIndex, params, NULL };


        if (LetUserMungeParams (&paramsHeader))
        {
            if ((i = WSAHtons(
                    (SOCKET) params[0].dwValue,
                    (u_short) params[1].dwValue,
                    (u_short *) params[2].dwValue

                    )) == 0)
            {
                ShowStr (gszXxxSUCCESS, aFuncNames[funcIndex]);
                ShowStr ("  hostShort=x%x", (DWORD) LOWORD(params[1].dwValue));
                ShowStr(
                    "  *lpNetShort=x%x",
                    (DWORD) *((u_short *) params[2].dwValue)
                    );
            }
            else if (i == SOCKET_ERROR)
            {
                ShowError (funcIndex, 0);
            }
            else
            {
                ShowUnknownError (funcIndex, i);
            }
        }

        break;
    }
    case ws_WSAInstallServiceClassA:
    case ws_WSAInstallServiceClassW:

// BUGBUG case ws_WSAInstallServiceClassA:/W
        break;

    case ws_WSAIoctl:
    {
        DWORD           cbBytesRet = 0;
        PMYOVERLAPPED   pOverlapped = MyAlloc (sizeof (MYOVERLAPPED) + dwBigBufSize);
        FUNC_PARAM params[] =
        {
            { gszSocket,                PT_DWORD,       (ULONG_PTR) gpSelectedSocket->Sock, NULL },
            { "dwIoControlCode",        PT_ORDINAL,     (ULONG_PTR) 0, aWSAIoctlCmds },
            { "lpvInBuffer",            PT_POINTER,     (ULONG_PTR) (pOverlapped+1), (pOverlapped+1) },
            { "cbInBuffer",             PT_DWORD,       (ULONG_PTR) dwBigBufSize, NULL },
            { "lpvOutBuffer",           PT_PTRNOEDIT,   (ULONG_PTR) (pOverlapped+1), (pOverlapped+1) },
            { "cbOutBuffer",            PT_DWORD,       (ULONG_PTR) dwBigBufSize, NULL },
            { "lpcbBytesReturned",      PT_PTRNOEDIT,   (ULONG_PTR) &cbBytesRet, &cbBytesRet },
            { "lpOverlapped",           PT_PTRNOEDIT,   (ULONG_PTR) 0, pOverlapped },
            { "  ->hEvent",             PT_DWORD,       (ULONG_PTR) ghSelectedEvent, NULL },
            { "lpfnCompletionProc",     PT_PTRNOEDIT,   (ULONG_PTR) 0, CompletionProc }
        };
        FUNC_PARAM_HEADER paramsHeader =
            { 10, funcIndex, params, NULL };


        if (!pOverlapped)
        {
            break;
        }

        if (LetUserMungeParams (&paramsHeader))
        {
            pOverlapped->WSAOverlapped.hEvent = (WSAEVENT) params[8].dwValue;
            pOverlapped->FuncIndex = funcIndex;
            pOverlapped->dwFuncSpecific1 = (DWORD) params[5].dwValue;

            if ((i = WSAIoctl(
                    (SOCKET) params[0].dwValue,
                    (DWORD) params[1].dwValue,
                    (LPVOID) params[2].dwValue,
                    (DWORD) params[3].dwValue,
                    (LPVOID) params[4].dwValue,
                    (DWORD) params[5].dwValue,
                    (LPDWORD) params[6].dwValue,
                    (LPWSAOVERLAPPED) params[7].dwValue,
                    (LPWSAOVERLAPPED_COMPLETION_ROUTINE) params[9].dwValue

                    )) == 0)
            {
                cbBytesRet = *((LPDWORD) params[6].dwValue);

                ShowStr (gszXxxSUCCESS, aFuncNames[funcIndex]);
                ShowStr ("  bytesReturned=x%x", cbBytesRet);
                ShowStr ("  *lpvOutBuffer=");
                ShowBytes (cbBytesRet, pOverlapped+1, 2);
            }
            else if (i == SOCKET_ERROR)
            {
                if (ShowError (funcIndex, 0) == WSA_IO_PENDING)
                {
                    ShowStr ("  lpOverlapped=x%x", params[7].dwValue);
                    break;
                }
            }
            else
            {
                ShowUnknownError (funcIndex, i);
            }

            if (i == 0  &&  params[9].dwValue)
            {
                // Let CompletionProc free overlapped struct
            }
            else
            {
                MyFree (pOverlapped);
            }
        }

        break;
    }
//    case ws_WSAIsBlocking:  not implemented in ws 2
//
//        break;

    case ws_WSAJoinLeaf:
    {
        LPBYTE      pBuf = MyAlloc (5 * dwBigBufSize + 2 * sizeof (QOS));
        LPSOCKADDR  pSA = (LPSOCKADDR) pBuf;
        LPBYTE      pCallerBuf = pBuf + dwBigBufSize,
                    pCalleeBuf = pBuf + 2 * dwBigBufSize;
        LPQOS       pSQOS = (LPQOS) (pBuf + 3 * dwBigBufSize),
                    pGQOS = (LPQOS) (pBuf + 4 * dwBigBufSize + sizeof (QOS));
        WSABUF      callerData, calleeData;
        FUNC_PARAM params[] =
        {
            { gszSocket,        PT_DWORD,       (ULONG_PTR) gpSelectedSocket->Sock, NULL },
            { "name",           PT_PTRNOEDIT,   (ULONG_PTR) pSA, pSA },
            { "  ->sa_family",  PT_ORDINAL,     (ULONG_PTR) gdwDefAddrFamily, aAddressFamilies },
            { "  ->sa_data",    PT_POINTER,     (ULONG_PTR) &pSA->sa_data, &pSA->sa_data },
            { "namelen",        PT_DWORD,       (ULONG_PTR) dwBigBufSize, NULL },
            { "lpCallerData",   PT_PTRNOEDIT,   (ULONG_PTR) &callerData, &callerData },
            { "  ->len",        PT_DWORD,       (ULONG_PTR) dwBigBufSize, NULL },
            { "  ->buf",        PT_POINTER,     (ULONG_PTR) pCallerBuf, pCallerBuf },
            { "lpCalleeData",   PT_PTRNOEDIT,   (ULONG_PTR) &calleeData, &calleeData },
            { "  ->len",        PT_DWORD,       (ULONG_PTR) dwBigBufSize, NULL },
            { "  ->buf",        PT_POINTER,     (ULONG_PTR) pCalleeBuf, pCalleeBuf },
            { "lpSQOS",         PT_QOS,         (ULONG_PTR) pSQOS, pSQOS },
            { "lpGQOS",         PT_QOS,         (ULONG_PTR) pGQOS, pGQOS },
            { "dwFlags",        PT_ORDINAL,     (ULONG_PTR) 0, aJLFlags }
        };
        FUNC_PARAM_HEADER paramsHeader =
            { 14, funcIndex, params, NULL };


        if (!pBuf)
        {
            break;
        }

        ZeroMemory (pSQOS, sizeof (QOS));
        pSQOS->ProviderSpecific.len = dwBigBufSize;

        ZeroMemory (pGQOS, sizeof (QOS));
        pGQOS->ProviderSpecific.len = dwBigBufSize;

        if (LetUserMungeParams (&paramsHeader))
        {
            pSA->sa_family = (u_short) LOWORD(params[2].dwValue);
            callerData.len = (DWORD) params[6].dwValue;
            calleeData.len = (DWORD) params[9].dwValue;

            if ((i = (int)WSAJoinLeaf(
                (SOCKET) params[0].dwValue,
                (LPSOCKADDR) params[1].dwValue,
                (int) params[4].dwValue,
                (LPWSABUF) params[5].dwValue,
                (LPWSABUF) params[8].dwValue,
                (LPQOS) params[11].dwValue,
                (LPQOS) params[12].dwValue,
                (DWORD) params[13].dwValue

                )) == 0)
            {
                LPWSABUF pCalleeData = (LPWSABUF) params[8].dwValue;


                ShowStr (gszXxxSUCCESS, aFuncNames[funcIndex]);
                ShowStr ("  lpCalleeData->len=x%x", pCalleeData->len);

                if (pCalleeData->len)
                {
                    ShowStr ("  lpCalleeData->buf=x%x", pCalleeData->buf);
                    ShowBytes (pCalleeData->len, pCalleeData->buf, 2);
                }
            }
            else if (i == SOCKET_ERROR)
            {
                ShowError (funcIndex, 0);
            }
            else
            {
                ShowUnknownError (funcIndex, i);
            }

        }

        MyFree (pBuf);

        break;
    }
    case ws_WSALookupServiceBeginA:
    case ws_WSALookupServiceBeginW:

// BUGBUG case ws_WSALookupServiceBeginA:/W
        break;

    case ws_WSALookupServiceEnd:
    {
        u_long  ulHostLong;
        FUNC_PARAM params[] =
        {
            { "hLookup",    PT_DWORD,   (DWORD) 0, NULL },
        };
        FUNC_PARAM_HEADER paramsHeader =
            { 1, funcIndex, params, NULL };


        if (LetUserMungeParams (&paramsHeader))
        {
            if ((i = WSALookupServiceEnd ((HANDLE) params[0].dwValue)) == 0)
            {
                ShowStr (gszXxxSUCCESS, aFuncNames[funcIndex]);
            }
            else if (i == SOCKET_ERROR)
            {
                ShowError (funcIndex, 0);
            }
            else
            {
                ShowUnknownError (funcIndex, i);
            }
        }

        break;
    }
    case ws_WSALookupServiceNextA:
    case ws_WSALookupServiceNextW:
    {
        DWORD   dwLength = dwBigBufSize;
        FUNC_PARAM params[] =
        {
            { "hLookup",            PT_DWORD,       (ULONG_PTR) 0, NULL },
            { "dwControlFlags",     PT_DWORD,       (ULONG_PTR) 0, NULL }, // BUGBUG flags
            { "lpdwBufferLength",   PT_POINTER,     (ULONG_PTR) &dwLength, &dwLength },
            { "lpqsResults",        PT_PTRNOEDIT,   (ULONG_PTR) pBigBuf, pBigBuf }
        };
        FUNC_PARAM_HEADER paramsHeader =
            { 4, funcIndex, params, NULL };


        if (LetUserMungeParams (&paramsHeader))
        {
            if (funcIndex == ws_WSALookupServiceNextA)
            {
                i = WSALookupServiceNextA(
                    (HANDLE) params[0].dwValue,
                    (DWORD) params[1].dwValue,
                    (LPDWORD) params[2].dwValue,
                    (LPWSAQUERYSETA) params[3].dwValue
                    );
            }
            else
            {
                i = WSALookupServiceNextW(
                    (HANDLE) params[0].dwValue,
                    (DWORD) params[1].dwValue,
                    (LPDWORD) params[2].dwValue,
                    (LPWSAQUERYSETW) params[3].dwValue
                    );
            }

            if (i  == 0)
            {
                ShowStr (gszXxxSUCCESS, aFuncNames[funcIndex]);
                // BUGBUG show query/set results
            }
            else if (i == SOCKET_ERROR)
            {
                ShowError (funcIndex, 0);
            }
            else
            {
                ShowUnknownError (funcIndex, i);
            }
        }

        break;
    }
    case ws_WSANtohl:
    {
        u_long  ulHostLong;
        FUNC_PARAM params[] =
        {
            { gszSocket,    PT_DWORD,       (ULONG_PTR) gpSelectedSocket->Sock, NULL },
            { "netLong",    PT_DWORD,       (ULONG_PTR) 0, NULL },
            { "lpHostLong", PT_PTRNOEDIT,   (ULONG_PTR) &ulHostLong, &ulHostLong }
        };
        FUNC_PARAM_HEADER paramsHeader =
            { 3, funcIndex, params, NULL };


        if (LetUserMungeParams (&paramsHeader))
        {
            if ((i = WSANtohl(
                    (SOCKET) params[0].dwValue,
                    (u_long) params[1].dwValue,
                    (u_long *) params[2].dwValue

                    )) == 0)
            {
                ShowStr (gszXxxSUCCESS, aFuncNames[funcIndex]);
                ShowStr ("  netLong=x%x", params[1].dwValue);
                ShowStr ("  *lpHostLong=x%x", *((u_long *) params[2].dwValue));
            }
            else if (i == SOCKET_ERROR)
            {
                ShowError (funcIndex, 0);
            }
            else
            {
                ShowUnknownError (funcIndex, i);
            }
        }

        break;
    }
    case ws_WSANtohs:
    {
        u_short usHostShort;
        FUNC_PARAM params[] =
        {
            { gszSocket,        PT_DWORD,       (ULONG_PTR) gpSelectedSocket->Sock, NULL },
            { "netShort",       PT_DWORD,       (ULONG_PTR) 0, NULL },
            { "lpHostShort",    PT_PTRNOEDIT,   (ULONG_PTR) &usHostShort, &usHostShort }
        };
        FUNC_PARAM_HEADER paramsHeader =
            { 3, funcIndex, params, NULL };


        if (LetUserMungeParams (&paramsHeader))
        {
            if ((i = WSANtohs(
                    (SOCKET) params[0].dwValue,
                    (u_short) params[1].dwValue,
                    (u_short *) params[2].dwValue

                    )) == 0)
            {
                ShowStr (gszXxxSUCCESS, aFuncNames[funcIndex]);
                ShowStr ("  netShort=x%x", (DWORD) LOWORD(params[1].dwValue));
                ShowStr(
                    "  *lpHostShort=x%x",
                    (DWORD) *((u_short *) params[2].dwValue)
                    );
            }
            else if (i == SOCKET_ERROR)
            {
                ShowError (funcIndex, 0);
            }
            else
            {
                ShowUnknownError (funcIndex, i);
            }
        }

        break;
    }
    case ws_WSARecv:
    {
        PMYOVERLAPPED   pOverlapped = MyAlloc(
                            sizeof (MYOVERLAPPED) +
                            2 * sizeof (WSABUF) +
                            2 * dwBigBufSize
                            );
        LPWSABUF        lpBuffers = (LPWSABUF) (pOverlapped + 1);
        LPBYTE          pBuf0 = (LPBYTE) (lpBuffers + 2),
                        pBuf1 = pBuf0 + dwBigBufSize;
        DWORD           dwNumBytesRecvd, dwFlags;
        FUNC_PARAM params[] =
        {
            { gszSocket,            PT_DWORD,       (ULONG_PTR) gpSelectedSocket->Sock, NULL },
            { "lpBuffers",          PT_PTRNOEDIT,   (ULONG_PTR) lpBuffers, lpBuffers },
            { "  buf[0].len",       PT_DWORD,       (ULONG_PTR) dwBigBufSize, NULL },
            { "  buf[0].buf",       PT_PTRNOEDIT,   (ULONG_PTR) pBuf0, pBuf0 },
            { "  buf[1].len",       PT_DWORD,       (ULONG_PTR) dwBigBufSize, NULL },
            { "  buf[1].buf",       PT_PTRNOEDIT,   (ULONG_PTR) pBuf1, pBuf1 },
            { "dwBufferCount",      PT_DWORD,       (ULONG_PTR) 2, NULL },
            { "lpdwNumBytesRecvd",  PT_PTRNOEDIT,   (ULONG_PTR) &dwNumBytesRecvd, &dwNumBytesRecvd },
            { "lpdwFlags",          PT_POINTER,     (ULONG_PTR) &dwFlags, &dwFlags },
            { "  ->dwFlags",        PT_FLAGS,       (ULONG_PTR) gdwDefRecvFlags, aWSARecvFlags },
            { "lpOverlapped",       PT_PTRNOEDIT,   (ULONG_PTR) 0, pOverlapped },
            { "  ->hEvent",         PT_DWORD,       (ULONG_PTR) ghSelectedEvent, NULL },
            { "lpfnCompletionProc", PT_PTRNOEDIT,   (ULONG_PTR) 0, CompletionProc }
        };
        FUNC_PARAM_HEADER paramsHeader =
            { 13, funcIndex, params, NULL };


        if (!pOverlapped)
        {
            break;
        }

        if (LetUserMungeParams (&paramsHeader))
        {
            pOverlapped->WSAOverlapped.hEvent = (WSAEVENT) params[10].dwValue;
            pOverlapped->FuncIndex = funcIndex;
            pOverlapped->dwFuncSpecific1 = (DWORD) params[6].dwValue; // bufCount

            dwFlags = (DWORD) params[9].dwValue;

            if ((i = WSARecv(
                    (SOCKET) params[0].dwValue,
                    (LPWSABUF) params[1].dwValue,
                    (DWORD) params[6].dwValue,
                    (LPDWORD) params[7].dwValue,
                    (LPDWORD) params[8].dwValue,
                    (LPWSAOVERLAPPED) params[10].dwValue,
                    (LPWSAOVERLAPPED_COMPLETION_ROUTINE) params[12].dwValue

                    )) == 0)
            {
                ShowStr (gszXxxSUCCESS, aFuncNames[funcIndex]);
                ShowStr(
                    "  *lpdwNumBytesRecvd=x%x",
                    *((LPDWORD) params[7].dwValue)
                    );
                ShowFlags(
                    *((LPDWORD) params[8].dwValue),
                    "  *lpdwFlags",
                    aWSASendFlags
                    );
            }
            else if (i == SOCKET_ERROR)
            {
                if (ShowError (funcIndex, 0) == WSA_IO_PENDING)
                {
                    ShowStr ("  lpOverlapped=x%x", params[10].dwValue);
                    break;
                }
            }
            else
            {
                ShowUnknownError (funcIndex, i);
            }
        }

        MyFree (pOverlapped);

        break;
    }
    case ws_WSARecvDisconnect:
    {
        FUNC_PARAM params[] =
        {
            { gszSocket,                    PT_DWORD,       (ULONG_PTR) gpSelectedSocket->Sock, NULL },
            { "lpInboundDisconnectData",    PT_PTRNOEDIT,   (ULONG_PTR) pBigBuf, pBigBuf },
            { "  ->len",                    PT_DWORD,       (ULONG_PTR) dwBigBufSize, NULL },
            { "  ->buf",                    PT_POINTER,     (ULONG_PTR) (pBigBuf + sizeof (u_long)), (pBigBuf + sizeof (u_long)) }
        };
        FUNC_PARAM_HEADER paramsHeader =
            { 4, funcIndex, params, NULL };


        if (LetUserMungeParams (&paramsHeader))
        {
            if ((i = WSARecvDisconnect(
                    (SOCKET) params[0].dwValue,
                    (LPWSABUF) params[1].dwValue

                    )) == 0)
            {
                ShowStr (gszXxxSUCCESS, aFuncNames[funcIndex]);
            }
            else if (i == SOCKET_ERROR)
            {
                ShowError (funcIndex, 0);
            }
            else
            {
                ShowUnknownError (funcIndex, i);
            }
        }

        break;
    }
    case ws_WSARecvFrom:
    {
        PMYOVERLAPPED   pOverlapped = MyAlloc(
                            sizeof (MYOVERLAPPED) +
                            2 * sizeof (WSABUF) +
                            2 * dwBigBufSize
                            );
        LPWSABUF        lpBuffers = (LPWSABUF) (pOverlapped + 1);
        LPBYTE          pBuf0 = (LPBYTE) (lpBuffers + 2),
                        pBuf1 = pBuf0 + dwBigBufSize;
        DWORD           dwNumBytesSent, dwFlags;
        LPSOCKADDR      psa = (LPSOCKADDR) pBigBuf;
        FUNC_PARAM params[] =
        {
            { gszSocket,            PT_DWORD,       (ULONG_PTR) gpSelectedSocket->Sock, NULL },
            { "lpBuffers",          PT_PTRNOEDIT,   (ULONG_PTR) lpBuffers, lpBuffers },
            { "  buf[0].len",       PT_DWORD,       (ULONG_PTR) dwBigBufSize, NULL },
            { "  buf[0].buf",       PT_PTRNOEDIT,   (ULONG_PTR) pBuf0, pBuf0 },
            { "  buf[1].len",       PT_DWORD,       (ULONG_PTR) dwBigBufSize, NULL },
            { "  buf[1].buf",       PT_PTRNOEDIT,   (ULONG_PTR) pBuf1, pBuf1 },
            { "dwBufferCount",      PT_DWORD,       (ULONG_PTR) 2, NULL },
            { "lpdwNumBytesRecvd",  PT_PTRNOEDIT,   (ULONG_PTR) &dwNumBytesSent, &dwNumBytesSent },
            { "lpdwFlags",          PT_POINTER,     (ULONG_PTR) &dwFlags, &dwFlags },
            { "  ->dwFlags",        PT_FLAGS,       (ULONG_PTR) gdwDefRecvFlags, aWSARecvFlags },
            { "lpTo",               PT_PTRNOEDIT,   (ULONG_PTR) psa, psa },
            { "  ->sa_family",      PT_ORDINAL,     (ULONG_PTR) gdwDefAddrFamily, aAddressFamilies },
            { "  ->sa_data",        PT_POINTER,     (ULONG_PTR) psa->sa_data, psa->sa_data },
            { "iToLen",             PT_DWORD,       (ULONG_PTR) dwBigBufSize, NULL },
            { "lpOverlapped",       PT_PTRNOEDIT,   (ULONG_PTR) 0, pOverlapped },
            { "  ->hEvent",         PT_DWORD,       (ULONG_PTR) ghSelectedEvent, NULL },
            { "lpfnCompletionProc", PT_PTRNOEDIT,   (ULONG_PTR) 0, CompletionProc }
        };
        FUNC_PARAM_HEADER paramsHeader =
            { 17, funcIndex, params, NULL };


        if (!pOverlapped)
        {
            break;
        }

        if (LetUserMungeParams (&paramsHeader))
        {
            pOverlapped->WSAOverlapped.hEvent = (WSAEVENT) params[14].dwValue;
            pOverlapped->FuncIndex = funcIndex;
            pOverlapped->dwFuncSpecific1 = (DWORD) params[6].dwValue; // bufCount

            dwFlags = (DWORD) params[9].dwValue;

            psa->sa_family = (u_short) LOWORD(params[11].dwValue);

            if ((i = WSASendTo(
                    (SOCKET) params[0].dwValue,
                    (LPWSABUF) params[1].dwValue,
                    (DWORD) params[6].dwValue,
                    (LPDWORD) params[7].dwValue,
                    (DWORD) params[8].dwValue,
                    (LPSOCKADDR) params[10].dwValue,
                    (int) params[13].dwValue,
                    (LPWSAOVERLAPPED) params[14].dwValue,
                    (LPWSAOVERLAPPED_COMPLETION_ROUTINE) params[16].dwValue

                    )) == 0)
            {
                ShowStr (gszXxxSUCCESS, aFuncNames[funcIndex]);
                ShowStr(
                    "  *lpdwNumBytesSent=x%x",
                    *((LPDWORD) params[7].dwValue)
                    );
                ShowFlags(
                    *((LPDWORD) params[8].dwValue),
                    "  *lpdwNumBytesSent=x%x",
                    aWSAFlags
                    );
            }
            else if (i == SOCKET_ERROR)
            {
                if (ShowError (funcIndex, 0) == WSA_IO_PENDING)
                {
                    ShowStr ("  lpOverlapped=x%x", params[14].dwValue);
                    break;
                }
            }
            else
            {
                ShowUnknownError (funcIndex, i);
            }
        }

        MyFree (pOverlapped);

        break;
    }
    case ws_WSARemoveServiceClass:
    {
        GUID guid[2]; // add padding to keep dumb user from hosing stack
        FUNC_PARAM params[] =
        {
            { "lpServiceClassId",    PT_POINTER, (ULONG_PTR) guid, guid },
        };
        FUNC_PARAM_HEADER paramsHeader =
            { 1, funcIndex, params, NULL };


        if (LetUserMungeParams (&paramsHeader))
        {
            if ((i = WSARemoveServiceClass ((LPGUID) params[0].dwValue)) == 0)
            {
                ShowStr (gszXxxSUCCESS, aFuncNames[funcIndex]);
            }
            else if (i == SOCKET_ERROR)
            {
                ShowError (funcIndex, 0);
            }
            else
            {
                ShowUnknownError (funcIndex, i);
            }
        }

        break;
    }
    case ws_WSAResetEvent:
    {
        FUNC_PARAM params[] =
        {
            { "hEvent",   PT_DWORD,   (ULONG_PTR) ghSelectedEvent, NULL }
        };
        FUNC_PARAM_HEADER paramsHeader =
            { 1, funcIndex, params, NULL };


        if (LetUserMungeParams (&paramsHeader))
        {
            if (WSAResetEvent ((WSAEVENT) params[0].dwValue) == TRUE)
            {
                ShowStr (gszXxxSUCCESS, aFuncNames[funcIndex]);
            }
            else
            {
                 ShowError (funcIndex, 0);
            }
        }

        break;
    }
    case ws_WSASend:
    {
        PMYOVERLAPPED   pOverlapped = MyAlloc(
                            sizeof (MYOVERLAPPED) +
                            2 * sizeof (WSABUF) +
                            2 * dwBigBufSize
                            );
        LPWSABUF        lpBuffers = (LPWSABUF) (pOverlapped + 1);
        LPBYTE          pBuf0 = (LPBYTE) (lpBuffers + 2),
                        pBuf1 = pBuf0 + dwBigBufSize;
        DWORD           dwNumBytesSent;
        FUNC_PARAM params[] =
        {
            { gszSocket,            PT_DWORD,       (ULONG_PTR) gpSelectedSocket->Sock, NULL },
            { "lpBuffers",          PT_PTRNOEDIT,   (ULONG_PTR) lpBuffers, lpBuffers },
            { "  buf[0].len",       PT_DWORD,       (ULONG_PTR) dwBigBufSize, NULL },
            { "  buf[0].buf",       PT_POINTER,     (ULONG_PTR) pBuf0, pBuf0 },
            { "  buf[1].len",       PT_DWORD,       (ULONG_PTR) dwBigBufSize, NULL },
            { "  buf[1].buf",       PT_POINTER,     (ULONG_PTR) pBuf1, pBuf1 },
            { "dwBufferCount",      PT_DWORD,       (ULONG_PTR) 2, NULL },
            { "lpdwNumBytesSent",   PT_PTRNOEDIT,   (ULONG_PTR) &dwNumBytesSent, &dwNumBytesSent },
            { "dwFlags",            PT_FLAGS,       (ULONG_PTR) gdwDefSendFlags, aWSASendFlags },
            { "lpOverlapped",       PT_PTRNOEDIT,   (ULONG_PTR) 0, pOverlapped },
            { "  ->hEvent",         PT_DWORD,       (ULONG_PTR) ghSelectedEvent, NULL },
            { "lpfnCompletionProc", PT_PTRNOEDIT,   (ULONG_PTR) 0, CompletionProc }
        };
        FUNC_PARAM_HEADER paramsHeader =
            { 12, funcIndex, params, NULL };


        if (!pOverlapped)
        {
            break;
        }

        if (LetUserMungeParams (&paramsHeader))
        {
            pOverlapped->WSAOverlapped.hEvent = (WSAEVENT) params[10].dwValue;
            pOverlapped->FuncIndex = funcIndex;
            pOverlapped->dwFuncSpecific1 = (DWORD) params[6].dwValue; // bufCount

            if ((i = WSASend(
                    (SOCKET) params[0].dwValue,
                    (LPWSABUF) params[1].dwValue,
                    (DWORD) params[6].dwValue,
                    (LPDWORD) params[7].dwValue,
                    (DWORD) params[8].dwValue,
                    (LPWSAOVERLAPPED) params[9].dwValue,
                    (LPWSAOVERLAPPED_COMPLETION_ROUTINE) params[11].dwValue

                    )) == 0)
            {
                ShowStr (gszXxxSUCCESS, aFuncNames[funcIndex]);
                ShowStr(
                    "  *lpdwNumBytesSent=x%x",
                    *((LPDWORD) params[7].dwValue)
                    );
            }
            else if (i == SOCKET_ERROR)
            {
                if (ShowError (funcIndex, 0) == WSA_IO_PENDING)
                {
                    ShowStr ("  lpOverlapped=x%x", params[9].dwValue);
                    break;
                }
            }
            else
            {
                ShowUnknownError (funcIndex, i);
            }
        }

        MyFree (pOverlapped);

        break;
    }
    case ws_WSASendDisconnect:
    {
        FUNC_PARAM params[] =
        {
            { gszSocket,                    PT_DWORD,       (ULONG_PTR) gpSelectedSocket->Sock, NULL },
            { "lpOutboundDisconnectData",   PT_PTRNOEDIT,   (ULONG_PTR) pBigBuf, pBigBuf },
            { "  ->len",                    PT_DWORD,       (ULONG_PTR) dwBigBufSize, NULL },
            { "  ->buf",                    PT_POINTER,     (ULONG_PTR) (pBigBuf + sizeof (u_long)), (pBigBuf + sizeof (u_long)) }
        };
        FUNC_PARAM_HEADER paramsHeader =
            { 4, funcIndex, params, NULL };


        if (LetUserMungeParams (&paramsHeader))
        {
            if ((i = WSASendDisconnect(
                    (SOCKET) params[0].dwValue,
                    (LPWSABUF) params[1].dwValue

                    )) == 0)
            {
                ShowStr (gszXxxSUCCESS, aFuncNames[funcIndex]);
            }
            else if (i == SOCKET_ERROR)
            {
                ShowError (funcIndex, 0);
            }
            else
            {
                ShowUnknownError (funcIndex, i);
            }
        }

        break;
    }
    case ws_WSASendTo:
    {
        PMYOVERLAPPED   pOverlapped = MyAlloc(
                            sizeof (MYOVERLAPPED) +
                            2 * sizeof (WSABUF) +
                            2 * dwBigBufSize
                            );
        LPWSABUF        lpBuffers = (LPWSABUF) (pOverlapped + 1);
        LPBYTE          pBuf0 = (LPBYTE) (lpBuffers + 2),
                        pBuf1 = pBuf0 + dwBigBufSize;
        DWORD           dwNumBytesSent;
        LPSOCKADDR      psa = (LPSOCKADDR) pBigBuf;
        FUNC_PARAM params[] =
        {
            { gszSocket,            PT_DWORD,       (ULONG_PTR) gpSelectedSocket->Sock, NULL },
            { "lpBuffers",          PT_PTRNOEDIT,   (ULONG_PTR) lpBuffers, lpBuffers },
            { "  buf[0].len",       PT_DWORD,       (ULONG_PTR) dwBigBufSize, NULL },
            { "  buf[0].buf",       PT_POINTER,     (ULONG_PTR) pBuf0, pBuf0 },
            { "  buf[1].len",       PT_DWORD,       (ULONG_PTR) dwBigBufSize, NULL },
            { "  buf[1].buf",       PT_POINTER,     (ULONG_PTR) pBuf1, pBuf1 },
            { "dwBufferCount",      PT_DWORD,       (ULONG_PTR) 2, NULL },
            { "lpdwNumBytesSent",   PT_PTRNOEDIT,   (ULONG_PTR) &dwNumBytesSent, &dwNumBytesSent },
            { "dwFlags",            PT_FLAGS,       (ULONG_PTR) gdwDefSendFlags, aWSASendFlags },
            { "lpTo",               PT_PTRNOEDIT,   (ULONG_PTR) psa, psa },
            { "  ->sa_family",      PT_ORDINAL,     (ULONG_PTR) gdwDefAddrFamily, aAddressFamilies },
            { "  ->sa_data",        PT_POINTER,     (ULONG_PTR) psa->sa_data, psa->sa_data },
            { "iToLen",             PT_DWORD,       (ULONG_PTR) dwBigBufSize, NULL },
            { "lpOverlapped",       PT_PTRNOEDIT,   (ULONG_PTR) 0, pOverlapped },
            { "  ->hEvent",         PT_DWORD,       (ULONG_PTR) ghSelectedEvent, NULL },
            { "lpfnCompletionProc", PT_PTRNOEDIT,   (ULONG_PTR) 0, CompletionProc }
        };
        FUNC_PARAM_HEADER paramsHeader =
            { 16, funcIndex, params, NULL };


        if (!pOverlapped)
        {
            break;
        }

        if (LetUserMungeParams (&paramsHeader))
        {
            pOverlapped->WSAOverlapped.hEvent = (WSAEVENT) params[14].dwValue;
            pOverlapped->FuncIndex = funcIndex;
            pOverlapped->dwFuncSpecific1 = (DWORD) params[6].dwValue; // bufCount

            psa->sa_family = (u_short) LOWORD(params[10].dwValue);

            if ((i = WSASendTo(
                    (SOCKET) params[0].dwValue,
                    (LPWSABUF) params[1].dwValue,
                    (DWORD) params[6].dwValue,
                    (LPDWORD) params[7].dwValue,
                    (DWORD) params[8].dwValue,
                    (LPSOCKADDR) params[9].dwValue,
                    (int) params[12].dwValue,
                    (LPWSAOVERLAPPED) params[13].dwValue,
                    (LPWSAOVERLAPPED_COMPLETION_ROUTINE) params[15].dwValue

                    )) == 0)
            {
                ShowStr (gszXxxSUCCESS, aFuncNames[funcIndex]);
                ShowStr(
                    "  *lpdwNumBytesSent=x%x",
                    *((LPDWORD) params[7].dwValue)
                    );
            }
            else if (i == SOCKET_ERROR)
            {
                if (ShowError (funcIndex, 0) == WSA_IO_PENDING)
                {
                    ShowStr ("  lpOverlapped=x%x", params[13].dwValue);
                    break;
                }
            }
            else
            {
                ShowUnknownError (funcIndex, i);
            }
        }

        MyFree (pOverlapped);

        break;
    }
//    case ws_WSASetBlockingHook: not implemented in ws 2
//
//        break;

    case ws_WSASetEvent:
    {
        FUNC_PARAM params[] =
        {
            { "hEvent",   PT_DWORD,   (ULONG_PTR) ghSelectedEvent, NULL }
        };
        FUNC_PARAM_HEADER paramsHeader =
            { 1, funcIndex, params, NULL };


        if (LetUserMungeParams (&paramsHeader))
        {
            if (WSASetEvent ((WSAEVENT) params[0].dwValue) == TRUE)
            {
                ShowStr (gszXxxSUCCESS, aFuncNames[funcIndex]);
            }
            else
            {
                 ShowError (funcIndex, 0);
            }
        }

        break;
    }
    case ws_WSASetLastError:
    {
        FUNC_PARAM params[] =
        {
            { "iError",   PT_DWORD,   (ULONG_PTR) 0, NULL }
        };
        FUNC_PARAM_HEADER paramsHeader =
            { 1, funcIndex, params, NULL };


        if (LetUserMungeParams (&paramsHeader))
        {
            WSASetLastError ((int) params[0].dwValue);
            ShowStr (gszXxxSUCCESS, aFuncNames[funcIndex]);
        }

        break;
    }
    case ws_WSASetServiceA:
    case ws_WSASetServiceW:

// BUGBUG case ws_WSASetServiceA:/W
        break;

    case ws_WSASocketA:
    case ws_WSASocketW:
    {
        FUNC_PARAM params[] =
        {
            { "address family", PT_ORDINAL, (ULONG_PTR) gdwDefAddrFamily, aAddressFamilies },
            { "socket type",    PT_ORDINAL, (ULONG_PTR) gdwDefSocketType, aSocketTypes },
            { "protocol",       PT_DWORD,   (ULONG_PTR) gdwDefProtocol, aProtocols },
            { "protocol info",  PT_WSAPROTOCOLINFO, 0, (gbWideStringParams ?
                (LPVOID) &gWSAProtocolInfoW : (LPVOID) &gWSAProtocolInfoA) },
            { "group",          PT_DWORD,   (ULONG_PTR) 0, NULL },
            { "flags",          PT_FLAGS,   (ULONG_PTR) gdwDefWSASocketFlags, aWSAFlags },
        };
        FUNC_PARAM_HEADER paramsHeader =
            { 6, funcIndex, params, NULL };


        if (LetUserMungeParams (&paramsHeader))
        {
            PMYSOCKET pSocket;


            if (!(pSocket = MyAlloc (sizeof (MYSOCKET))))
            {
                break;
            }

            if (gbWideStringParams)
            {
                pSocket->Sock = WSASocketW(
                    (int) params[0].dwValue,
                    (int) params[1].dwValue,
                    (int) params[2].dwValue,
                    (LPWSAPROTOCOL_INFOW) params[3].dwValue,
                    (GROUP) params[4].dwValue,
                    (DWORD) params[5].dwValue
                    );
            }
            else
            {
                pSocket->Sock = WSASocketA(
                    (int) params[0].dwValue,
                    (int) params[1].dwValue,
                    (int) params[2].dwValue,
                    (LPWSAPROTOCOL_INFOA) params[3].dwValue,
                    (GROUP) params[4].dwValue,
                    (DWORD) params[5].dwValue
                    );
            }

            if (pSocket->Sock != INVALID_SOCKET)
            {
                char        buf[128];
                LRESULT     index;


                ShowStr (gszXxxSUCCESS, aFuncNames[funcIndex]);

                pSocket->bWSASocket      = TRUE;
                pSocket->dwAddressFamily = (DWORD) params[0].dwValue;
                pSocket->dwSocketType    = (DWORD) params[1].dwValue;
                pSocket->dwFlags         = (DWORD) params[5].dwValue;

                wsprintf(
                    buf,
                    "WSASocket=x%x (%s %s%s)",
                    pSocket->Sock,
                    GetStringFromOrdinalValue (pSocket->dwAddressFamily, aAddressFamilies),
                    GetStringFromOrdinalValue (pSocket->dwSocketType, aSocketTypes),
                    (pSocket->dwFlags & WSA_FLAG_OVERLAPPED ?
                        " OVERLAPPED" : "")
                    );

                index = SendMessage(
                    ghwndList1,
                    LB_ADDSTRING,
                    0,
                    (LPARAM) buf
                    );

                SendMessage(
                    ghwndList1,
                    LB_SETITEMDATA,
                    index,
                    (LPARAM) pSocket
                    );
            }
            else
            {
                ShowError (funcIndex, 0);
                MyFree (pSocket);
            }
        }

        break;
    }
    case ws_WSAStartup:
    {
        WSADATA wsaData;
        FUNC_PARAM params[] =
        {
            { "wVersionRequested",  PT_DWORD,       (ULONG_PTR) 0x101, NULL },
            { "lpWSAData",          PT_PTRNOEDIT,   (ULONG_PTR) &wsaData, &wsaData },
        };
        FUNC_PARAM_HEADER paramsHeader =
            { 2, funcIndex, params, NULL };


        if (LetUserMungeParams (&paramsHeader))
        {
            if ((i = WSAStartup(
                    LOWORD(params[0].dwValue),
                    (LPWSADATA) params[1].dwValue

                    )) == 0)
            {
                char szButtonText[32];
                LPWSADATA pWSAData = (LPWSADATA) params[1].dwValue;


                wsprintf (szButtonText, "Startup (%d)", ++giCurrNumStartups);
                SetDlgItemText (ghwndMain, IDC_BUTTON1, szButtonText);

                ShowStr (gszXxxSUCCESS, aFuncNames[funcIndex]);
                ShowStr ("  wVersion=x%x", (DWORD) pWSAData->wVersion);
                ShowStr ("  wHighVersion=x%x", (DWORD) pWSAData->wHighVersion);
                ShowStr ("  szDescription=%s", pWSAData->szDescription);
                ShowStr ("  szSystemStatus=%s", pWSAData->szSystemStatus);
                ShowStr ("  iMaxSockets=%d", (DWORD) pWSAData->iMaxSockets);
                ShowStr ("  iMaxUdpDg=%d", (DWORD) pWSAData->iMaxUdpDg);
                // BUGBUG  ignored for 2.0+  ShowStr ("  ", pWSAData->lpVendorInfo);
            }
            else
            {
                ShowError (funcIndex, i);
            }
        }

        break;
    }
    case ws_WSAStringToAddressA:
    case ws_WSAStringToAddressW:
    {
        INT  iAddrLen = (INT) dwBigBufSize;
        char szAddrString[MAX_STRING_PARAM_SIZE] = "";
        LPWSAPROTOCOL_INFOA pInfo = (funcIndex == ws_WSAStringToAddressA ?
                                &gWSAProtocolInfoA :
                                (LPWSAPROTOCOL_INFOA) &gWSAProtocolInfoW);
        FUNC_PARAM params[] =
        {
            { "lpszAddressString",  PT_STRING,  (ULONG_PTR) szAddrString, szAddrString },
            { "iAddressFamily",     PT_ORDINAL, (ULONG_PTR) gdwDefAddrFamily, aAddressFamilies },
            { "lpProtocolInfo",     PT_WSAPROTOCOLINFO, (ULONG_PTR) pInfo, pInfo },
            { "lpAddress",          PT_POINTER, (ULONG_PTR) pBigBuf, pBigBuf },
            { "lpAddressLength",    PT_POINTER, (ULONG_PTR) &iAddrLen, &iAddrLen }
        };
        FUNC_PARAM_HEADER paramsHeader =
            { 5, funcIndex, params, NULL };


        if (LetUserMungeParams (&paramsHeader))
        {
            if (funcIndex == ws_WSAStringToAddressA)
            {
                i = WSAStringToAddressA(
                    (LPSTR) params[0].dwValue,
                    (INT) params[1].dwValue,
                    (LPWSAPROTOCOL_INFOA) params[2].dwValue,
                    (LPSOCKADDR) params[3].dwValue,
                    (LPINT) params[4].dwValue
                    );
            }
            else
            {
                i = WSAStringToAddressW(
                    (LPWSTR) params[0].dwValue,
                    (INT) params[1].dwValue,
                    (LPWSAPROTOCOL_INFOW) params[2].dwValue,
                    (LPSOCKADDR) params[3].dwValue,
                    (LPINT) params[4].dwValue
                    );
            }

            if (i == 0)
            {
                LPSOCKADDR pSockAddr = (LPSOCKADDR) params[3].dwValue;


                ShowStr (gszXxxSUCCESS, aFuncNames[funcIndex]);

                ShowStr(
                    "  *lpAddressLength=x%x",
                    *((LPINT) params[4].dwValue)
                    );

                ShowStr ("  lpAddress=x%x", params[3].dwValue);

                ShowStr(
                    "    ->sa_family=%d, %s",
                    (DWORD) pSockAddr->sa_family,
                    GetStringFromOrdinalValue(
                        (DWORD) pSockAddr->sa_family,
                        aAddressFamilies
                        )
                    );

                ShowStr ("    ->sa_data=");
                ShowBytes (14, pSockAddr->sa_data, 3);
            }
            else if (i == SOCKET_ERROR)
            {
                ShowError (funcIndex, 0);
            }
            else
            {
                ShowUnknownError (funcIndex, i);
            }
        }

        break;
    }
//    case ws_WSAUnhookBlockingHook:
//
//        break;

    case ws_WSAWaitForMultipleEvents:

// BUGBUG case ws_WSAWaitForMultipleEvents:
        break;

    case ws_WSCEnumProtocols:
    {
        int aProtocols[32], iErrno;
        DWORD dwSize = dwBigBufSize;
        FUNC_PARAM params[] =
        {
            { "lpiProtocols",       PT_POINTER,     (ULONG_PTR) NULL, aProtocols },
            { "lpProtocolBuffer",   PT_PTRNOEDIT,   (ULONG_PTR) pBigBuf, pBigBuf },
            { "lpdwBufferLength",   PT_POINTER,     (ULONG_PTR) &dwSize, &dwSize },
            { "lpErrno",            PT_POINTER,     (ULONG_PTR) &iErrno, &iErrno }
        };
        FUNC_PARAM_HEADER paramsHeader =
            { 4, funcIndex, params, NULL };


        if (LetUserMungeParams (&paramsHeader))
        {
            i = WSCEnumProtocols(
                (LPINT) params[0].dwValue,
                (LPWSAPROTOCOL_INFOW) params[1].dwValue,
                (LPDWORD) params[2].dwValue,
                (LPINT) params[3].dwValue
                );

            if (i != SOCKET_ERROR)
            {
                LPWSAPROTOCOL_INFOW pInfo = (LPWSAPROTOCOL_INFOW)
                                        params[1].dwValue;


                UpdateResults (TRUE);

                ShowStr (gszXxxSUCCESS, aFuncNames[funcIndex]);

                ShowStr(
                    "  *lpdwBufferLength=x%x",
                    *((LPDWORD) params[2].dwValue)
                    );

                for (j = 0; j < i; j++)
                {
                    ShowProtoInfo ((LPWSAPROTOCOL_INFOA) pInfo, j, FALSE);

                    pInfo++;
                }

                UpdateResults (FALSE);
            }
            else
            {
                ShowError (funcIndex, iErrno);
                if (iErrno == WSAENOBUFS)
                {
                    ShowStr(
                        "  *lpdwBufferLength=x%x",
                        *((LPDWORD) params[2].dwValue)
                        );
                }
            }
        }

        break;
    }
    case ws_WSCGetProviderPath:
    {
        GUID guid[2];
        INT iPathLen = dwBigBufSize, iErrno = 0;
        FUNC_PARAM params[] =
        {
            { "lpProviderId",           PT_POINTER,  (ULONG_PTR) guid, guid },
            { "lpszProviderDllPath",    PT_PTRNOEDIT,(ULONG_PTR) pBigBuf, pBigBuf },
            { "lpProviderDllPathLen",   PT_POINTER,  (ULONG_PTR) &iPathLen, &iPathLen },
            { "lpErrno",                PT_PTRNOEDIT,(ULONG_PTR) &iErrno, &iErrno }
        };
        FUNC_PARAM_HEADER paramsHeader =
            { 4, funcIndex, params, NULL };

        if (LetUserMungeParams (&paramsHeader))
        {
            if ((i = WSCGetProviderPath(
                    (LPGUID) params[0].dwValue,
                    (LPWSTR) params[1].dwValue,
                    (LPINT)  params[2].dwValue,
                    (LPINT)  params[3].dwValue

                    )) == 0)
            {
                ShowStr (gszXxxSUCCESS, aFuncNames[funcIndex]);
                ShowStr ("*lpszProviderDllPath='%ws'", params[1].dwValue);
                ShowStr ("*lpProviderDllPathLen=%d", *((LPINT)params[2].dwValue));
            }
            else
            {
                ShowError (funcIndex, iErrno);
            }
        }

        break;
    }
    case ws_EnumProtocolsA:
    case ws_EnumProtocolsW:
    {
        INT aiProtocols[32];
        DWORD dwBufSize = dwBigBufSize;
        FUNC_PARAM params[] =
        {
            { "lpiProtocols",       PT_POINTER,  (ULONG_PTR) NULL, aiProtocols },
            { "lpProtocolBuffer",   PT_PTRNOEDIT,(ULONG_PTR) pBigBuf, pBigBuf },
            { "lpdwBufferLength",   PT_POINTER,  (ULONG_PTR) &dwBufSize, &dwBufSize }
        };
        FUNC_PARAM_HEADER paramsHeader =
            { 3, funcIndex, params, NULL };

        if (LetUserMungeParams (&paramsHeader))
        {
            i = (funcIndex == ws_EnumProtocolsA ?

                EnumProtocolsA(
                    (LPINT)   params[0].dwValue,
                    (LPVOID)  params[1].dwValue,
                    (LPDWORD) params[2].dwValue
                    ) :

                EnumProtocolsW(
                    (LPINT)   params[0].dwValue,
                    (LPVOID)  params[1].dwValue,
                    (LPDWORD) params[2].dwValue
                    ));

            if (i >= 0)
            {
                char            szProtoInfoN[20], buf[36];
                PROTOCOL_INFO   *pInfo;

                UpdateResults (TRUE);

                ShowStr (gszXxxSUCCESS, aFuncNames[funcIndex]);
                ShowStr ("iResult=%d", i);
                ShowStr(
                    "*lpdwBufferLength=x%x",
                    *((LPDWORD) params[2].dwValue)
                    );

                for(
                    j = 0, pInfo = (PROTOCOL_INFO *) params[1].dwValue;
                    j < i;
                    j++, pInfo++
                    )
                {
                    wsprintf (szProtoInfoN, "  protoInfo[%d].", j);

                    wsprintf (buf, "%sdwServiceFlags", szProtoInfoN);
                    ShowFlags (pInfo->dwServiceFlags, buf, aServiceFlags);

                    ShowStr(
                        "%siAddressFamily=%d, %s",
                        szProtoInfoN,
                        (DWORD) pInfo->iAddressFamily,
                        GetStringFromOrdinalValue(
                            (DWORD) pInfo->iAddressFamily,
                            aAddressFamilies
                            )
                        );

                    ShowStr(
                        "%siMaxSockAddr=%d",
                        szProtoInfoN,
                        (DWORD) pInfo->iMaxSockAddr
                        );

                    ShowStr(
                        "%siMinSockAddr=%d",
                        szProtoInfoN,
                        (DWORD) pInfo->iMinSockAddr
                        );

                    ShowStr(
                        "%siSocketType=%d, %s",
                        szProtoInfoN,
                        (DWORD) pInfo->iSocketType,
                        GetStringFromOrdinalValue(
                            (DWORD) pInfo->iSocketType,
                            aSocketTypes
                            )
                        );

                    ShowStr(
                        "%siProtocol=%d (x%x)", // BUGBUG ought to be flags???
                        szProtoInfoN,
                        (DWORD) pInfo->iProtocol,
                        (DWORD) pInfo->iProtocol
                        );

                    ShowStr(
                        "%sdwMessageSize=x%x",
                        szProtoInfoN,
                        (DWORD) pInfo->dwMessageSize
                        );

                    ShowStr(
                        "%slpProtocol=x%x",
                        szProtoInfoN,
                        pInfo->lpProtocol
                        );


                    if (funcIndex == ws_EnumProtocolsA)
                    {
                        ShowStr(
                            "%s*lpProtocol=%s",
                            szProtoInfoN,
                            pInfo->lpProtocol
                            );

                        ShowBytes(
                            (lstrlenA (pInfo->lpProtocol) + 1) * sizeof(CHAR),
                            pInfo->lpProtocol,
                            2
                            );
                    }
                    else
                    {
                        ShowStr(
                            "%s*lpProtocol=%ws",
                            szProtoInfoN,
                            pInfo->lpProtocol
                            );

                        ShowBytes(
                            (lstrlenW ((WCHAR *) pInfo->lpProtocol) + 1)
                                * sizeof(WCHAR),
                            pInfo->lpProtocol,
                            2
                            );
                    }
                }

                UpdateResults (FALSE);
            }
            else
            {
                if (ShowError (funcIndex, 0) == ERROR_INSUFFICIENT_BUFFER)
                {
                    ShowStr(
                        "*lpdwBufferLength=x%x",
                        *((LPDWORD) params[2].dwValue)
                        );
                }
            }
        }

        break;
    }
    case ws_GetAddressByNameA:
    case ws_GetAddressByNameW:
    {
        GUID    guid[2];
        WCHAR   szSvcName[MAX_STRING_PARAM_SIZE] = L"";
        INT     aiProtocols[32];
        char    *pBigBuf2 = (char *) LocalAlloc (LPTR, dwBigBufSize);
        DWORD   dwCsaddrBufLen = dwBigBufSize, dwAliasBufLen = dwBigBufSize;
        FUNC_PARAM params[] =
        {
            { "dwNameSpace",        PT_ORDINAL,  (ULONG_PTR) 0, aNameSpaces },
            { "lpServiceType",      PT_POINTER,  (ULONG_PTR) guid, guid },
            { "lpServiceName",      PT_STRING,   (ULONG_PTR) NULL, szSvcName },
            { "lpiProtocols",       PT_POINTER,  (ULONG_PTR) NULL, aiProtocols },
            { "dwResolution",       PT_FLAGS,    (ULONG_PTR) 0, aResFlags },
            { "lpServiceAsyncInfo", PT_DWORD,    (ULONG_PTR) 0, 0 },
            { "lpCsaddrBuffer",     PT_PTRNOEDIT,(ULONG_PTR) pBigBuf, pBigBuf },
            { "lpdwCsaddrBufLen",   PT_POINTER,  (ULONG_PTR) &dwCsaddrBufLen, &dwCsaddrBufLen },
            { "lpAliasBuf",         PT_PTRNOEDIT,(ULONG_PTR) pBigBuf2, pBigBuf2 },
            { "lpdwAliasBufLen",    PT_POINTER,  (ULONG_PTR) &dwAliasBufLen, &dwAliasBufLen }
        };
        FUNC_PARAM_HEADER paramsHeader =
            { 10, funcIndex, params, NULL };

        if (LetUserMungeParams (&paramsHeader))
        {
            i = (funcIndex == ws_GetAddressByNameA ?

                GetAddressByNameA(
                    (DWORD)   params[0].dwValue,
                    (LPGUID)  params[1].dwValue,
                    (LPSTR)   params[2].dwValue,
                    (LPINT)   params[3].dwValue,
                    (DWORD)   params[4].dwValue,
                    (LPSERVICE_ASYNC_INFO) params[5].dwValue,
                    (LPVOID)  params[6].dwValue,
                    (LPDWORD) params[7].dwValue,
                    (LPSTR)   params[8].dwValue,
                    (LPDWORD) params[9].dwValue
                    ) :

                GetAddressByNameW(
                    (DWORD)   params[0].dwValue,
                    (LPGUID)  params[1].dwValue,
                    (LPWSTR)  params[2].dwValue,
                    (LPINT)   params[3].dwValue,
                    (DWORD)   params[4].dwValue,
                    (LPSERVICE_ASYNC_INFO) params[5].dwValue,
                    (LPVOID)  params[6].dwValue,
                    (LPDWORD) params[7].dwValue,
                    (LPWSTR)  params[8].dwValue,
                    (LPDWORD) params[9].dwValue

                    ));

            if (i >= 0)
            {
                char        szAddrInfoN[20];
                CSADDR_INFO *pInfo;


                UpdateResults (TRUE);

                ShowStr (gszXxxSUCCESS, aFuncNames[funcIndex]);
                ShowStr ("iResult=%d", i);

                ShowStr(
                    "*lpdwCsaddrBufLen=x%x",
                    *((LPDWORD) params[7].dwValue)
                    );

                for(
                    j = 0, pInfo = (CSADDR_INFO *) params[6].dwValue;
                    j < i;
                    j++, pInfo++
                    )
                {
                    wsprintf (szAddrInfoN, "  addrInfo[%d].", j);

                    ShowStr(
                        "%sLocalAddr.lpSockaddr=x%x",
                        szAddrInfoN,
                        pInfo->LocalAddr.lpSockaddr
                        );

                    ShowBytes(
                        pInfo->LocalAddr.iSockaddrLength,
                        pInfo->LocalAddr.lpSockaddr,
                        2
                        );

                    ShowStr(
                        "%sLocalAddr.iSockaddrLen=%d (x%x)",
                        szAddrInfoN,
                        pInfo->LocalAddr.iSockaddrLength,
                        pInfo->LocalAddr.iSockaddrLength
                        );

                    ShowStr(
                        "%sRemoteAddr.lpSockaddr=x%x",
                        szAddrInfoN,
                        pInfo->RemoteAddr.lpSockaddr
                        );

                    ShowBytes(
                        pInfo->RemoteAddr.iSockaddrLength,
                        pInfo->RemoteAddr.lpSockaddr,
                        2
                        );

                    ShowStr(
                        "%sRemoteAddr.iSockaddrLen=%d (x%x)",
                        szAddrInfoN,
                        pInfo->RemoteAddr.iSockaddrLength,
                        pInfo->RemoteAddr.iSockaddrLength
                        );

                    ShowStr(
                        "%siSocketType=%d, %s",
                        szAddrInfoN,
                        (DWORD) pInfo->iSocketType,
                        GetStringFromOrdinalValue(
                            (DWORD) pInfo->iSocketType,
                            aSocketTypes
                            )
                        );

                    ShowStr(
                        "%siProtocol=%d (x%x)", // BUGBUG ought to be flags???
                        szAddrInfoN,
                        (DWORD) pInfo->iProtocol,
                        (DWORD) pInfo->iProtocol
                        );
                }

                UpdateResults (FALSE);
            }
            else
            {
                if (ShowError (funcIndex, 0) == ERROR_INSUFFICIENT_BUFFER)
                {
                    ShowStr(
                        "*lpdwCsaddrBufLen=x%x",
                        *((LPDWORD) params[7].dwValue)
                        );
                }
            }
        }

        LocalFree (pBigBuf2);

        break;
    }
    case ws_GetNameByTypeA:
    case ws_GetNameByTypeW:
    {
        GUID    guid[2];
        FUNC_PARAM params[] =
        {
            { "lpServiceType",  PT_POINTER,  (ULONG_PTR) guid, guid },
            { "lpServiceName",  PT_PTRNOEDIT,(ULONG_PTR) pBigBuf, pBigBuf },
            { "dwNameLength",   PT_DWORD,    (ULONG_PTR) dwBigBufSize, NULL }
        };
        FUNC_PARAM_HEADER paramsHeader =
            { 3, funcIndex, params, NULL };

        if (LetUserMungeParams (&paramsHeader))
        {
            i = (funcIndex == ws_GetNameByTypeA ?

                GetNameByTypeA(
                    (LPGUID) params[0].dwValue,
                    (LPSTR)  params[1].dwValue,
                    (DWORD)  params[2].dwValue
                    ) :

                GetNameByTypeW(
                    (LPGUID) params[0].dwValue,
                    (LPWSTR)  params[1].dwValue,
                    (DWORD)  params[2].dwValue
                    ));

            if (i != SOCKET_ERROR)
            {
                ShowStr (gszXxxSUCCESS, aFuncNames[funcIndex]);

                if (funcIndex == ws_GetNameByTypeA)
                {
                    ShowStr ("*lpServiceName=%s", (char *) params[1].dwValue);
                }
                else
                {
                    ShowStr ("*lpServiceName=%ws", (char *) params[1].dwValue);
                }
            }
            else
            {
                ShowError (funcIndex, 0);
            }
        }

        break;
    }
    case ws_GetServiceA:
    case ws_GetServiceW:
    {
        GUID    guid[2];
        WCHAR   szSvcName[MAX_STRING_PARAM_SIZE] = L"";
        DWORD dwBufSize = dwBigBufSize;
        FUNC_PARAM params[] =
        {
            { "dwNameSpace",    PT_ORDINAL,   (ULONG_PTR) 0, aNameSpaces },
            { "lpGuid",         PT_POINTER,   (ULONG_PTR) guid, guid },
            { "lpServiceName",  PT_STRING,    (ULONG_PTR) szSvcName, szSvcName },
            { "dwProperties",   PT_FLAGS,     (ULONG_PTR) 0, aProperties },
            { "lpBuffer",       PT_PTRNOEDIT, (ULONG_PTR) pBigBuf, pBigBuf },
            { "lpdwBufLen",     PT_POINTER,   (ULONG_PTR) &dwBufSize, &dwBufSize },
            { "lpSvcAsyncInfo", PT_DWORD,     (ULONG_PTR) 0, 0 }

        };
        FUNC_PARAM_HEADER paramsHeader =
            { 7, funcIndex, params, NULL };

        if (LetUserMungeParams (&paramsHeader))
        {
            i = (funcIndex == ws_GetServiceA ?

                GetServiceA(
                    (DWORD)   params[0].dwValue,
                    (LPGUID)  params[1].dwValue,
                    (LPSTR)   params[2].dwValue,
                    (DWORD)   params[3].dwValue,
                    (LPVOID)  params[4].dwValue,
                    (LPDWORD) params[5].dwValue,
                    (LPSERVICE_ASYNC_INFO) params[6].dwValue
                    ) :

                GetServiceW(
                    (DWORD)   params[0].dwValue,
                    (LPGUID)  params[1].dwValue,
                    (LPWSTR)   params[2].dwValue,
                    (DWORD)   params[3].dwValue,
                    (LPVOID)  params[4].dwValue,
                    (LPDWORD) params[5].dwValue,
                    (LPSERVICE_ASYNC_INFO) params[6].dwValue
                    ));

            if (i >= 0)
            {
                char            szSvcInfoN[20], buf[36];
                SERVICE_INFO    *pInfo;
                SERVICE_ADDRESS *pAddr;

                UpdateResults (TRUE);

                ShowStr (gszXxxSUCCESS, aFuncNames[funcIndex]);
                ShowStr ("iResult=%d", i);
                ShowStr(
                    "*lpdwBufLen=x%x",
                    *((LPDWORD) params[5].dwValue)
                    );

                for(
                    j = 0, pInfo = (SERVICE_INFO *) params[4].dwValue;
                    j < i;
                    j++, pInfo++
                    )
                {
                    wsprintf (szSvcInfoN, "  svcInfo[%d].", j);

                    wsprintf (buf, "%slpServiceType=");

                    ShowGUID (buf, pInfo->lpServiceType);

                    ShowStr(
                        (funcIndex == ws_GetServiceA ?
                            "%slpServiceName=%s" : "%slpServiceName=%ws"),
                        szSvcInfoN,
                        pInfo->lpServiceName
                        );

                    ShowStr(
                        (funcIndex == ws_GetServiceA ?
                            "%slpComment=%s" : "%slpComment=%ws"),
                        szSvcInfoN,
                        pInfo->lpServiceName
                        );

                    ShowStr(
                        (funcIndex == ws_GetServiceA ?
                            "%slpLocale=%s" : "%slpLocale=%ws"),
                        szSvcInfoN,
                        pInfo->lpLocale
                        );

                    ShowStr(
                        "%sdwDisplayHint=%d, %s",
                        szSvcInfoN,
                        (DWORD) pInfo->dwDisplayHint,
                        GetStringFromOrdinalValue(
                            (DWORD) pInfo->dwDisplayHint,
                            aResDisplayTypes
                            )
                        );

                    ShowStr ("%sdwVersion=x%x", szSvcInfoN, pInfo->dwVersion);

                    ShowStr ("%sdwTime=x%x", szSvcInfoN, pInfo->dwTime);

                    ShowStr(
                        (funcIndex == ws_GetServiceA ?
                            "%slpMachineName=%s" : "%slpMachineName=%ws"),
                        szSvcInfoN,
                        pInfo->lpMachineName
                        );

                    ShowStr(
                        "%slpServiceAddress.dwAddressCount=%d (x%x)",
                        szSvcInfoN,
                        pInfo->lpServiceAddress->dwAddressCount,
                        pInfo->lpServiceAddress->dwAddressCount
                        );

                    for(
                        k = 0, pAddr = pInfo->lpServiceAddress->Addresses;
                        k < (int) pInfo->lpServiceAddress->dwAddressCount;
                        k++, pAddr++
                        )
                    {
                    }

                    ShowStr(
                        "%sServiceSpecificInfo.cbSize=%d (x%x)",
                        szSvcInfoN,
                        pInfo->ServiceSpecificInfo.cbSize,
                        pInfo->ServiceSpecificInfo.cbSize
                        );

                    ShowBytes(
                        pInfo->ServiceSpecificInfo.cbSize,
                        pInfo->ServiceSpecificInfo.pBlobData,
                        2
                        );
                }

                UpdateResults (FALSE);
            }
            else
            {
                if (ShowError (funcIndex, i) == ERROR_INSUFFICIENT_BUFFER)
                {
                    ShowStr(
                        "*lpdwBufLen=x%x",
                        *((LPDWORD) params[5].dwValue)
                        );
                }
            }
        }

        break;
    }
    case ws_GetTypeByNameA:
    case ws_GetTypeByNameW:
    {
        GUID    guid[2];
        WCHAR   szSvcName[MAX_STRING_PARAM_SIZE] = L"";
        FUNC_PARAM params[] =
        {
            { "lpServiceName",  PT_PTRNOEDIT,(ULONG_PTR) szSvcName, szSvcName },
            { "lpServiceType",  PT_POINTER,  (ULONG_PTR) guid, guid }
        };
        FUNC_PARAM_HEADER paramsHeader =
            { 2, funcIndex, params, NULL };

        if (LetUserMungeParams (&paramsHeader))
        {
            i = (funcIndex == ws_GetTypeByNameA ?

                GetTypeByNameA(
                    (LPSTR)  params[0].dwValue,
                    (LPGUID) params[1].dwValue
                    ) :

                GetTypeByNameW(
                    (LPWSTR) params[0].dwValue,
                    (LPGUID) params[1].dwValue
                    ));

            if (i != SOCKET_ERROR)
            {
                ShowStr (gszXxxSUCCESS, aFuncNames[funcIndex]);
                ShowGUID ("*lpServiceType=", (LPGUID) params[1].dwValue);
            }
            else
            {
                ShowError (funcIndex, 0);
            }
        }

        break;
    }
    case ws_SetServiceA:
    case ws_SetServiceW:
    {
        DWORD dwStatusFlags;
        FUNC_PARAM params[] =
        {
            { "dwNameSpace",        PT_ORDINAL,     (ULONG_PTR) 0, aNameSpaces },
            { "dwOperation",        PT_ORDINAL,     (ULONG_PTR) 0, aServiceOps },
            { "dwFlags",            PT_FLAGS,       (ULONG_PTR) 0, aSvcFlags },
            { "lpServiceInfo",      PT_POINTER,     (ULONG_PTR) pBigBuf, pBigBuf }, // BUGBUG
            { "lpServiceAsnycInfo", PT_DWORD,       (ULONG_PTR) 0, NULL },
            { "lpdwStatusFlags",    PT_PTRNOEDIT,   (ULONG_PTR) &dwStatusFlags, &dwStatusFlags },
        };
        FUNC_PARAM_HEADER paramsHeader =
            { 6, funcIndex, params, NULL };

        if (LetUserMungeParams (&paramsHeader))
        {
            i = (funcIndex == ws_SetServiceA ?

                SetServiceA(
                    (DWORD) params[0].dwValue,
                    (DWORD) params[1].dwValue,
                    (DWORD) params[2].dwValue,
                    (LPSERVICE_INFOA) params[3].dwValue,
                    (LPSERVICE_ASYNC_INFO) params[4].dwValue,
                    (LPDWORD) params[5].dwValue
                    ) :

                SetServiceW(
                    (DWORD) params[0].dwValue,
                    (DWORD) params[1].dwValue,
                    (DWORD) params[2].dwValue,
                    (LPSERVICE_INFOW) params[3].dwValue,
                    (LPSERVICE_ASYNC_INFO) params[4].dwValue,
                    (LPDWORD) params[5].dwValue
                    ));

            if (i != SOCKET_ERROR)
            {
                ShowStr (gszXxxSUCCESS, aFuncNames[funcIndex]);
            }
            else
            {
                ShowError (funcIndex, 0);
            }
        }

        break;
      }
//    case CloseHandl:
//
//        break;

//    case DumpBuffer:
//
//        break;

    }
}


void
FAR
ShowHostEnt(
    struct hostent  *phe
    )
{
    DWORD   i, j;


    ShowStr ("  pHostEnt=x%x", phe);
    ShowStr ("  ->h_name=%s", phe->h_name);

    for (i = 0; *(phe->h_aliases + i); i++)
    {
        ShowStr ("  ->h_aliases[%d]=%s", i, *(phe->h_aliases + i));
    }

    ShowStr ("  ->h_addrtype=%x", (DWORD) phe->h_addrtype);
    ShowStr ("  ->h_length=%x", (DWORD) phe->h_length);

    for (i = 0; *(phe->h_addr_list + i); i++)
    {
        char far *pAddr = *(phe->h_addr_list + i);

        if (phe->h_addrtype == AF_INET)
        {
            ShowStr ("  ->h_addr_list[%d]=", i);
            ShowBytes (sizeof (DWORD) + lstrlenA (pAddr+4), pAddr, 2);
        }
        else
        {
            ShowStr ("  ->h_addr_list[%d]=%s", i, *(phe->h_addr_list + i));
        }
    }
}


void
FAR
ShowProtoEnt(
    struct protoent *ppe
    )
{
    DWORD   i;


    ShowStr ("  pProtoEnt=x%x", ppe);
    ShowStr ("  ->p_name=%s", ppe->p_name);

    for (i = 0; *(ppe->p_aliases + i); i++)
    {
        ShowStr ("  ->p_aliases[%d]=%s", i, *(ppe->p_aliases + i));
    }

    ShowStr ("  ->p_proto=%x", (DWORD) ppe->p_proto);
}


void
FAR
ShowServEnt(
    struct servent  *pse
    )
{
    DWORD   i;


    ShowStr ("  pServEnt=x%x", pse);
    ShowStr ("  ->s_name=%s", pse->s_name);

    for (i = 0; *(pse->s_aliases + i); i++)
    {
        ShowStr ("  ->s_aliases[%d]=%s", i, *(pse->s_aliases + i));
    }

    ShowStr ("  ->s_port=%d (x%x)", (DWORD) pse->s_port, (DWORD) pse->s_port);
    ShowStr ("  ->s_proto=%s", pse->s_proto);
}


void
PASCAL
QueueAsyncRequestInfo(
    PASYNC_REQUEST_INFO pAsyncReqInfo
    )
{
    pAsyncReqInfo->pNext = gpAsyncReqInfoList;
    gpAsyncReqInfoList = pAsyncReqInfo;
}


PASYNC_REQUEST_INFO
PASCAL
DequeueAsyncRequestInfo(
    HANDLE  hRequest
    )
{
    PASYNC_REQUEST_INFO  pAsyncReqInfo;


    for(
        pAsyncReqInfo = gpAsyncReqInfoList;
        pAsyncReqInfo  &&  pAsyncReqInfo->hRequest != hRequest;
        pAsyncReqInfo = pAsyncReqInfo->pNext
        );

    if (pAsyncReqInfo)
    {
        if (pAsyncReqInfo->pPrev)
        {
            pAsyncReqInfo->pPrev->pNext = pAsyncReqInfo->pNext;
        }
        else
        {
            gpAsyncReqInfoList = pAsyncReqInfo->pNext;
        }

        if (pAsyncReqInfo->pNext)
        {
            pAsyncReqInfo->pNext->pPrev = pAsyncReqInfo->pPrev;
        }
    }

    return pAsyncReqInfo;
}


void
PASCAL
ShowModBytes(
    DWORD               dwSize,
    unsigned char far  *lpc,
    char               *pszTab,
    char               *buf
    )
{
    DWORD   dwSize2 = dwSize, i, j, k;


    strcpy (buf, pszTab);

    k = strlen (buf);

    for (i = 8; i < 36; i += 9)
    {
        buf[k + i] = ' ';

        for (j = 2; j < 9; j += 2)
        {
            char buf2[8] = "xx";

            if (dwSize2)
            {
                sprintf (buf2, "%02x", (int) (*lpc));
                dwSize2--;
            }

            buf[k + i - j]     = buf2[0];
            buf[k + i - j + 1] = buf2[1];

            lpc++;
        }
    }

    k += 37;

    buf[k - 1] = ' ';

    lpc -= 16;

    for (i = 0; i < dwSize; i++)
    {
        buf[k + i] = aAscii[*(lpc+i)];
    }

    buf[k + i] = 0;

    ShowStr (buf);
}


void
UpdateResults(
    BOOL bBegin
    )
{
    //
    // In order to maximize speed, minimize flash, & have the
    // latest info in the edit control scrolled into view we
    // shrink the window down and hide it. Later, when all the
    // text has been inserted in the edit control, we show
    // the window (since window must be visible in order to
    // scroll caret into view), then tell it to scroll the caret
    // (at this point the window is still 1x1 so the painting
    // overhead is 0), and finally restore the control to it's
    // full size. In doing so we have zero flash and only 1 real
    // complete paint. Also put up the hourglass for warm fuzzies.
    //

    static RECT    rect;
    static HCURSOR hCurSave;
    static int     iNumBegins = 0;


    if (bBegin)
    {
        iNumBegins++;

        if (iNumBegins > 1)
        {
            return;
        }

        hCurSave = SetCursor (LoadCursor ((HINSTANCE)NULL, IDC_WAIT));
        GetWindowRect (ghwndEdit, &rect);
        SetWindowPos(
            ghwndEdit,
            (HWND) NULL,
            0,
            0,
            1,
            1,
            SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOREDRAW |
                SWP_NOZORDER | SWP_HIDEWINDOW
            );
    }
    else
    {
        iNumBegins--;

        if (iNumBegins > 0)
        {
            return;
        }

        //
        // Do control restoration as described above
        //

        ShowWindow (ghwndEdit, SW_SHOW);
#ifdef WIN32
        SendMessage (ghwndEdit, EM_SCROLLCARET, 0, 0);
#else
        SendMessage(
            ghwndEdit,
            EM_SETSEL,
            (WPARAM)0,
            (LPARAM) MAKELONG(0xfffd,0xfffe)
            );
#endif
        SetWindowPos(
            ghwndEdit,
            (HWND) NULL,
            0,
            0,
            rect.right - rect.left,
            rect.bottom - rect.top,
            SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOZORDER
            );
        SetCursor (hCurSave);
    }
}


void
PASCAL
ShowBytes(
    DWORD   dwSize,
    LPVOID  lp,
    DWORD   dwNumTabs
    )
{
    char    tabBuf[17] = "";
    char    buf[80];
    DWORD   i, j, k, dwNumDWORDs, dwMod4 = (DWORD)(((ULONG_PTR) lp) % 4);
    LPDWORD lpdw;
    unsigned char far *lpc = (unsigned char far *) lp;


    UpdateResults (TRUE);


    for (i = 0; i < dwNumTabs; i++)
    {
        strcat (tabBuf, szTab);
    }


    //
    // Special case for unaligned pointers (will fault on ppc/mips)
    //

    if (dwMod4)
    {
        DWORD   dwNumUnalignedBytes = 4 - dwMod4,
                dwNumBytesToShow = (dwNumUnalignedBytes > dwSize ?
                    dwSize : dwNumUnalignedBytes);


        ShowModBytes (dwNumBytesToShow, lpc, tabBuf, buf);
        lpc += dwNumUnalignedBytes;
        lpdw = (LPDWORD) lpc;
        dwSize -= dwNumBytesToShow;
    }
    else
    {
        lpdw = (LPDWORD) lp;
    }


    //
    // Dump full lines of four DWORDs in hex & corresponding ASCII
    //

    if (dwSize >= (4*sizeof(DWORD)))
    {
        dwNumDWORDs = dwSize / 4; // adjust from numBytes to num DWORDs

        for (i = 0; i < (dwNumDWORDs - (dwNumDWORDs%4)); i += 4)
        {
            sprintf (
                buf,
                "%s%08lx %08lx %08lx %08lx  ",
                tabBuf,
                *lpdw,
                *(lpdw+1),
                *(lpdw+2),
                *(lpdw+3)
                );

            k = strlen (buf);

            for (j = 0; j < 16; j++)
            {
                buf[k + j] = aAscii[*(lpc + j)];
            }

            buf[k + j] = 0;

            ShowStr (buf);
            lpdw += 4;
            lpc += 16;
        }
    }


    //
    // Special case for remaining bytes to dump (0 < n < 16)
    //

    if ((dwSize %= 16))
    {
        ShowModBytes (dwSize, lpc, tabBuf, buf);
    }


    UpdateResults (FALSE);
}


void
PASCAL
ShowFlags(
    DWORD       dwValue,
    char FAR    *pszValueName,
    PLOOKUP     pLookup
    )
{
    char     buf[80];
    DWORD    i;


    wsprintf (buf, "%s=x%lx, ", pszValueName, dwValue);

    for (i = 0; dwValue  &&  pLookup[i].lpszVal; i++)
    {
        if (dwValue & pLookup[i].dwVal)
        {
            if (buf[0] == 0)
            {
                lstrcpyA (buf, "    ");
            }

            lstrcatA (buf, pLookup[i].lpszVal);
            lstrcat (buf, " ");
            dwValue &= ~pLookup[i].dwVal;

            if (lstrlen (buf) > 50)
            {
                //
                // We don't want strings getting so long that
                // they're going offscreen, so break them up.
                //

                ShowStr (buf);
                buf[0] = 0;
            }
        }
    }

    if (dwValue)
    {
        lstrcat (buf, "<unknown flag(s)>");
    }

    if (buf[0])
    {
        ShowStr (buf);
    }
}


LPSTR
PASCAL
GetStringFromOrdinalValue(
    DWORD   dwValue,
    PLOOKUP pLookup
    )
{
    DWORD i;


    for (i = 0; pLookup[i].lpszVal  &&  dwValue != pLookup[i].dwVal; i++);
    return (pLookup[i].lpszVal ? pLookup[i].lpszVal : gszUnknown);
}


VOID
PASCAL
ShowGUID(
    char    *pszProlog,
    GUID    *pGuid
    )
{
    ShowStr(
        "%s%08x %04x %04x %02x%02x%02x%02x%02x%02x%02x%02x",
        (ULONG_PTR) pszProlog,
        (DWORD) pGuid->Data1,
        (DWORD) pGuid->Data2,
        (DWORD) pGuid->Data3,
        (DWORD) pGuid->Data4[0],
        (DWORD) pGuid->Data4[1],
        (DWORD) pGuid->Data4[2],
        (DWORD) pGuid->Data4[3],
        (DWORD) pGuid->Data4[4],
        (DWORD) pGuid->Data4[5],
        (DWORD) pGuid->Data4[6],
        (DWORD) pGuid->Data4[7]
        );

    //
    // Dump the GUID in a form that can be copied & pasted into other
    // API params
    //

    {
        LPBYTE p = (LPBYTE) pGuid;


        ShowStr(
            "    (%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x)",
            (DWORD) *p,
            (DWORD) *(p + 1),
            (DWORD) *(p + 2),
            (DWORD) *(p + 3),
            (DWORD) *(p + 4),
            (DWORD) *(p + 5),
            (DWORD) *(p + 6),
            (DWORD) *(p + 7),
            (DWORD) *(p + 8),
            (DWORD) *(p + 9),
            (DWORD) *(p + 10),
            (DWORD) *(p + 11),
            (DWORD) *(p + 12),
            (DWORD) *(p + 13),
            (DWORD) *(p + 14),
            (DWORD) *(p + 15)
            );
    }
}


VOID
PASCAL
ShowProtoInfo(
    LPWSAPROTOCOL_INFOA pInfo,
    DWORD               dwIndex,
    BOOL                bAscii
    )
{
    char szProtoInfoN[20], buf[40];


    if (dwIndex == 0xffffffff)
    {
        wsprintf (szProtoInfoN, "  protoInfo");
    }
    else
    {
        wsprintf (szProtoInfoN, "  protoInfo[%d].", dwIndex);
    }

    wsprintf (buf, "%sdwServiceFlags", szProtoInfoN);
    ShowFlags (pInfo->dwServiceFlags1, buf, aServiceFlags);

    ShowStr(
        "%sdwServiceFlags2=x%x",
        szProtoInfoN,
        pInfo->dwServiceFlags2
        );

    ShowStr(
        "%sdwServiceFlags3=x%x",
        szProtoInfoN,
        pInfo->dwServiceFlags3
        );

    ShowStr(
        "%sdwServiceFlags4=x%x",
        szProtoInfoN,
        pInfo->dwServiceFlags4
        );

    wsprintf (buf, "%sdwProviderFlags", szProtoInfoN);
    ShowFlags (pInfo->dwProviderFlags, buf, aProviderFlags);

    wsprintf (buf, "%sProviderId=", szProtoInfoN);
    ShowGUID (buf, &pInfo->ProviderId);

    ShowStr(
        "%sdwCatalogEntryId=x%x",
        szProtoInfoN,
        pInfo->dwCatalogEntryId
        );

    ShowStr(
        "%sProtocolChain.ChainLen=x%x",
        szProtoInfoN,
        (DWORD) pInfo->ProtocolChain.ChainLen
        );

    ShowStr(
        "%sProtocolChain.ChainEntries=",
        szProtoInfoN
        );

    ShowBytes(
        sizeof (DWORD) * MAX_PROTOCOL_CHAIN,
        pInfo->ProtocolChain.ChainEntries,
        2
        );

    ShowStr(
        "%siVersion=x%x",
        szProtoInfoN,
        (DWORD) pInfo->iVersion
        );

    ShowStr(
        "%siAddressFamily=%d, %s",
        szProtoInfoN,
        (DWORD) pInfo->iAddressFamily,
        GetStringFromOrdinalValue(
            (DWORD) pInfo->iAddressFamily,
            aAddressFamilies
            )
        );

    ShowStr(
        "%siMaxSockAddr=%d",
        szProtoInfoN,
        (DWORD) pInfo->iMaxSockAddr
        );

    ShowStr(
        "%siMinSockAddr=%d",
        szProtoInfoN,
        (DWORD) pInfo->iMinSockAddr
        );

    ShowStr(
        "%siSocketType=%d, %s",
        szProtoInfoN,
        (DWORD) pInfo->iSocketType,
        GetStringFromOrdinalValue(
            (DWORD) pInfo->iSocketType,
            aSocketTypes
            )
        );

    ShowStr(
        "%siProtocol=%d (x%x)", // BUGBUG ought to be flags???
        szProtoInfoN,
        (DWORD) pInfo->iProtocol,
        (DWORD) pInfo->iProtocol
        );

    ShowStr(
        "%siProtocolMaxOffset=%d",
        szProtoInfoN,
        (DWORD) pInfo->iProtocolMaxOffset
        );

    ShowStr(
        "%siNetworkByteOrder=%d, %s",
        szProtoInfoN,
        (DWORD) pInfo->iNetworkByteOrder,
        GetStringFromOrdinalValue(
            (DWORD) pInfo->iNetworkByteOrder,
            aNetworkByteOrders
            )
        );

    ShowStr(
        "%siSecurityScheme=x%x",
        szProtoInfoN,
        (DWORD) pInfo->iSecurityScheme
        );

    ShowStr(
        "%sdwMessageSize=x%x",
        szProtoInfoN,
        (DWORD) pInfo->dwMessageSize
        );

    ShowStr(
        "%sdwProviderReserved=x%x",
        szProtoInfoN,
        (DWORD) pInfo->dwMessageSize
        );

    if (bAscii)
    {
        ShowStr(
            "%sszProtocol=%s",
            szProtoInfoN,
            pInfo->szProtocol
            );

        ShowBytes(
            (lstrlenA (pInfo->szProtocol) + 1) * sizeof(CHAR),
            pInfo->szProtocol,
            2
            );
    }
    else
    {
        ShowStr(
            "%sszProtocol=%ws",
            szProtoInfoN,
            pInfo->szProtocol
            );

        ShowBytes(
            (lstrlenW (((LPWSAPROTOCOL_INFOW) pInfo)->szProtocol) + 1)
                * sizeof(WCHAR),
            pInfo->szProtocol,
            2
            );
    }
}
