/*---------------------------------------------------------------------------*\
| WINCHAT APPLICATION MODULE
|   This is the main module file for the application.  The application was
|   originally written by ClausGi for the Windows-For-WorkGroup product.
|   In the port to NT, all references to PEN-awareness and Protocol were
|   removed.  Extensive cleanup and documenting was also added in the port.
|
|   FUNCTIONS
|   ---------
|   myatol
|   UpdateButtonStates
|   appGetComputerName
|   AdjustEditWindows
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

#include <windows.h>
#include <mmsystem.h>
#include <stdio.h>
#include <ddeml.h>
#include <commdlg.h>
#include <commctrl.h>
#include <shellapi.h>
#include <nddeapi.h>
#include <richedit.h>
#include "winchat.h"
#include "dialogs.h"
#include "globals.h"
#include "nddeagnt.h"

#include <tchar.h>
#include <imm.h>
#include <htmlhelp.h>

#define ASSERT(x)

// This is used in the port to NT.  Since NT doesn't haven a dialogbox for
// this function, we'll use the lanman export.
//
#ifdef WIN32
#define FOCUSDLG_DOMAINS_ONLY        (1)
#define FOCUSDLG_SERVERS_ONLY        (2)
#define FOCUSDLG_SERVERS_AND_DOMAINS (3)

#define FOCUSDLG_BROWSE_LOGON_DOMAIN         0x00010000
#define FOCUSDLG_BROWSE_WKSTA_DOMAIN         0x00020000
#define FOCUSDLG_BROWSE_OTHER_DOMAINS        0x00040000
#define FOCUSDLG_BROWSE_TRUSTING_DOMAINS     0x00080000
#define FOCUSDLG_BROWSE_WORKGROUP_DOMAINS    0x00100000

#define FOCUSDLG_BROWSE_LM2X_DOMAINS         (FOCUSDLG_BROWSE_LOGON_DOMAIN | FOCUSDLG_BROWSE_WKSTA_DOMAIN | FOCUSDLG_BROWSE_OTHER_DOMAINS)
#define FOCUSDLG_BROWSE_ALL_DOMAINS          (FOCUSDLG_BROWSE_LOCAL_DOMAINS | FOCUSDLG_BROWSE_WORKGROUP_DOMAINS)
#define FOCUSDLG_BROWSE_LOCAL_DOMAINS        (FOCUSDLG_BROWSE_LM2X_DOMAINS | FOCUSDLG_BROWSE_TRUSTING_DOMAINS)

#define MY_LOGONTYPE                         (FOCUSDLG_BROWSE_ALL_DOMAINS | FOCUSDLG_SERVERS_ONLY)

UINT APIENTRY I_SystemFocusDialog(HWND,UINT,LPWSTR,UINT,PBOOL,LPWSTR,DWORD);
#endif

BOOL TranslateWideCharPosToMultiBytePos(HWND,DWORD,DWORD,LPDWORD,LPDWORD);


/*---------------------------------------------------------------------------*\
| WINDOWS MAIN
|   This is the main event-processing loop for the application.
|
| created: 11-Nov-91
| history: 29-Dec-92 <chriswil> ported to NT.
|
\*---------------------------------------------------------------------------*/
int PASCAL WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    MSG msg;


    msg.wParam = 0;

    if(InitApplication(hInstance))
    {
        if(InitInstance(hInstance,nCmdShow))
        {
            while(GetMessage(&msg,NULL,0,0))
            {
                if(!TranslateAccelerator(hwndApp,hAccel,&msg))
                {
                    TranslateMessage(&msg);
                    DispatchMessage(&msg);
                }
            }

            if(hszConvPartner)
                DdeFreeStringHandle(idInst,hszConvPartner);

            DdeFreeStringHandle(idInst,hszChatTopic);
            DdeFreeStringHandle(idInst,hszChatShare);
            DdeFreeStringHandle(idInst,hszLocalName );
            DdeFreeStringHandle(idInst,hszTextItem);
            DdeFreeStringHandle(idInst,hszConnectTest);
            DdeUninitialize(idInst);

            EndIniMapping();
        }
    }

    return((int)msg.wParam);
}


#ifdef WIN16
#pragma alloc_text ( _INIT, InitApplication )
#endif
/*---------------------------------------------------------------------------*\
| INITIALIZE APPLICATION
|   This routine registers the application with user.
|
| created: 11-Nov-91
| history: 29-Dec-92 <chriswil> ported to NT.
|
\*---------------------------------------------------------------------------*/
BOOL FAR InitApplication(HINSTANCE hInstance)
{
    WNDCLASS wc;


    wc.style         = 0;
    wc.lpfnWndProc   = (WNDPROC)MainWndProc;
    wc.cbClsExtra    = 0;
    wc.cbWndExtra    = 0;
    wc.hInstance     = hInstance;
    wc.hIcon         = LoadIcon(hInstance, TEXT("PHONE1"));
    wc.hCursor       = LoadCursor(NULL,IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE+1);
    wc.lpszMenuName  = szWinChatMenu;
    wc.lpszClassName = szWinChatClass;

    return(RegisterClass(&wc));
}


#define IMMMODULENAME L"IMM32.DLL"
#define PATHDLM     L'\\'
#define IMMMODULENAMELEN    ((sizeof PATHDLM + sizeof IMMMODULENAME) / sizeof(WCHAR))

VOID GetImmFileName(PWSTR wszImmFile)
{
    UINT i = GetSystemDirectoryW(wszImmFile, MAX_PATH);
    if (i > 0 && i < MAX_PATH - IMMMODULENAMELEN) {
        wszImmFile += i;
        if (wszImmFile[-1] != PATHDLM) {
            *wszImmFile++ = PATHDLM;
        }
    }
    wcscpy(wszImmFile, IMMMODULENAME);
}

/*---------------------------------------------------------------------------*\
| IsTSRemoteSession
|
| Input:      None
| Output:     BOOL - TRUE if in a Terminal Server remote session (SessionId != 0)
|             FALSE - if error OR not in a TS rermote session
| Function:   To determine whether we are running in a TS remote session or not.
|
\*---------------------------------------------------------------------------*/
BOOL IsTSRemoteSession()
{
    BOOL      bRetVal;
    DWORD     dwSessionID;
    HINSTANCE hInst;
    FARPROC   lpfnProcessIdToSessionId;

    //assume failure
    bRetVal = FALSE;

    // load library and get proc address
    hInst=LoadLibrary(TEXT("kernel32.dll"));

    if (hInst)
    {
        lpfnProcessIdToSessionId = GetProcAddress(hInst,"ProcessIdToSessionId");

        if (lpfnProcessIdToSessionId )
        {
            if (lpfnProcessIdToSessionId(GetCurrentProcessId(),&dwSessionID))
            {
                if(dwSessionID!=0)
                {
                    bRetVal = TRUE;
                }
            }
        }

        // free the library
        FreeLibrary(hInst);
    }

    return bRetVal;
}




#ifdef WIN16
#pragma alloc_text ( _INIT, InitInstance )
#endif
/*---------------------------------------------------------------------------*\
| INITIALIZE APPLICATION INTSTANCE
|   This routine initializes instance information.
|
| created: 11-Nov-91
| history: 29-Dec-92 <chriswil> ported to NT.
|
\*---------------------------------------------------------------------------*/
BOOL FAR InitInstance(HINSTANCE hInstance, int nCmdShow)
{
    HWND      hwnd;
    HMENU     hMenu;
        HINSTANCE hmodNetDriver;
    int       cAppQueue;
    BOOL      bRet;

    hInst = hInstance;


    //
    // get DBCS flag
    //
    gfDbcsEnabled = GetSystemMetrics(SM_DBCSENABLED);

    if (GetSystemMetrics(SM_IMMENABLED)) {
        //
        // if IME is enabled, get real API addresses
        //
        WCHAR wszImmFile[MAX_PATH];
        HINSTANCE hInstImm32;
        GetImmFileName(wszImmFile);
        hInstImm32 = GetModuleHandle(wszImmFile);
        if (hInstImm32) {
            pfnImmGetContext = (PVOID)GetProcAddress(hInstImm32, "ImmGetContext");
            ASSERT(pfnImmGetContext);
            pfnImmReleaseContext = (PVOID)GetProcAddress(hInstImm32, "ImmReleaseContext");
            ASSERT(pfnImmReleaseContext);
            pfnImmGetCompositionStringW = (PVOID)GetProcAddress(hInstImm32, "ImmGetCompositionStringW");
            ASSERT(pfnImmGetCompositionStringW);
        }
    }

    // increase our app queue for better performance...
    //
    for(cAppQueue=128; !SETMESSAGEQUEUE(cAppQueue); cAppQueue >>= 1);


    //
    //
    bRet = FALSE;
    if(cAppQueue >= 8)
    {
        bRet = TRUE;

        cxIcon = GetSystemMetrics(SM_CXICON);
        cyIcon = GetSystemMetrics(SM_CYICON);
        hAccel = LoadAccelerators(hInstance,MAKEINTRESOURCE(IDACCELERATORS));



        LoadIntlStrings();
        StartIniMapping();


        InitFontFromIni();


        // check if it's a Terminal Server remote session
        if (IsTSRemoteSession())
        {
            TCHAR szTSNotSupported[SZBUFSIZ];

            LoadString(hInst, IDS_TSNOTSUPPORTED, szTSNotSupported, SZBUFSIZ);

            MessageBeep(MB_ICONSTOP);
            MessageBox(NULL, szTSNotSupported, szAppName, MB_OK | MB_ICONSTOP);
            return(FALSE);
        }

        // get our machine name and map to correct character set.
        //
        if(!appGetComputerName(szLocalName))
        {
            MessageBeep(MB_ICONSTOP);
            MessageBox(NULL,szSysErr,szAppName,MB_OK | MB_ICONSTOP);
            return(FALSE);
        }


        // initialize DDEML.
        //
        if(DdeInitialize(&idInst,(PFNCALLBACK)MakeProcInstance((FARPROC)DdeCallback,hInst),APPCLASS_STANDARD,0L))
        {
            MessageBeep(MB_ICONSTOP);
            MessageBox(NULL,szSysErr,szAppName,MB_OK | MB_ICONSTOP);
            return(FALSE);
        }


        ChatState.fMinimized  = (nCmdShow == SW_MINIMIZE) ? TRUE : FALSE;
        ChatState.fMMSound    = waveOutGetNumDevs();
        ChatState.fSound      = GetPrivateProfileInt(szPref,szSnd  ,1,szIni);
        ChatState.fToolBar    = GetPrivateProfileInt(szPref,szTool ,1,szIni);
        ChatState.fStatusBar  = GetPrivateProfileInt(szPref,szStat ,1,szIni);
        ChatState.fTopMost    = GetPrivateProfileInt(szPref,szTop  ,0,szIni);
        ChatState.fSideBySide = GetPrivateProfileInt(szPref,szSbS  ,0,szIni);
        ChatState.fUseOwnFont = GetPrivateProfileInt(szPref,szUseOF,0,szIni);

        hszLocalName          = DdeCreateStringHandle(idInst,szLocalName  ,0);
        hszChatTopic          = DdeCreateStringHandle(idInst,szChatTopic  ,0);
        hszChatShare          = DdeCreateStringHandle(idInst,szChatShare  ,0);
        hszServiceName        = DdeCreateStringHandle(idInst,szServiceName,0);
        hszConnectTest        = DdeCreateStringHandle(idInst,szConnectTest,0);
        hszTextItem           = DdeCreateStringHandle(idInst,szChatText   ,0);

        if(!hszLocalName || !hszChatTopic || !hszServiceName || !hszTextItem || !hszChatShare)
        {
            MessageBeep(MB_ICONSTOP);
            MessageBox(NULL,szSysErr,szAppName,MB_OK | MB_ICONSTOP);
            return(FALSE);
        }

        DdeNameService(idInst,hszServiceName,(HSZ)0,DNS_REGISTER);


        if(DdeGetLastError(idInst) != DMLERR_NO_ERROR)
        {
            MessageBeep(MB_ICONSTOP);
            MessageBox (NULL,szSysErr,szAppName,MB_OK | MB_ICONSTOP);
            return(FALSE);
        }

        cf_chatdata = RegisterClipboardFormat(TEXT("Chat Data"));
        if(!(cf_chatdata))
        {
            MessageBeep(MB_ICONSTOP);
            MessageBox(NULL,szSysErr,szAppName,MB_OK | MB_ICONSTOP);
            return(FALSE);
        }


        // get winnet extension browse dialog entry point
        //
        WNetServerBrowseDialog = NULL;
        hmodNetDriver          = WNETGETCAPS(0xFFFF);

        if(hmodNetDriver != NULL)
            WNetServerBrowseDialog = (WNETCALL)GetProcAddress(hmodNetDriver,(LPSTR)146);



        // create main window
        hwnd = CreateWindow(
            szWinChatClass,
            szAppName,
            WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            NULL,
            NULL,
            hInstance,
            NULL
        );

        if(!hwnd)
        {
            MessageBeep(MB_ICONSTOP);
            MessageBox(NULL,szSysErr,szAppName,MB_OK | MB_ICONSTOP);
            return(FALSE);
        }

        hwndApp = hwnd; // save global

        // font choice struct init
        //
        chf.lStructSize    = sizeof(CHOOSEFONT);
        chf.lpLogFont      = &lfSnd;
        chf.Flags          = CF_SCREENFONTS | CF_EFFECTS | CF_INITTOLOGFONTSTRUCT;
        chf.rgbColors      = GetSysColor(COLOR_WINDOWTEXT);
        chf.lCustData      = 0L;
        chf.lpfnHook       = NULL;
        chf.lpTemplateName = NULL;
        chf.hInstance      = NULL;
        chf.lpszStyle      = NULL;
        chf.nFontType      = SCREEN_FONTTYPE;
        chf.nSizeMin       = 0;
        chf.nSizeMax       = 0;


        // color choice init
        //
        chc.lStructSize    = sizeof(CHOOSECOLOR);
        chc.hwndOwner      = hwndApp;
        chc.hInstance      = hInst;
        chc.lpCustColors   = (LPDWORD)CustColors;
        chc.Flags          = CC_RGBINIT | CC_PREVENTFULLOPEN;
        chc.lCustData      = 0;
        chc.lpfnHook       = NULL;
        chc.lpTemplateName = NULL;


        // window placement...
        //
        if(ReadWindowPlacement(&Wpl))
        {
            // override these - CODEWORK don't need to save
            // them to .ini, but will mis-parse old .ini files
            // if change is made.
            //
            Wpl.showCmd         = nCmdShow;
            Wpl.ptMaxPosition.x = -1;
            Wpl.ptMaxPosition.y = -1;
            Wpl.flags           = 0;

            SetWindowPlacement(hwnd,&Wpl);
            UpdateWindow(hwnd);
        }
        else
            ShowWindow(hwnd,nCmdShow);

        //
        //
        hMenu = GetSystemMenu(hwnd,FALSE);
        AppendMenu(hMenu,MF_SEPARATOR,0,NULL);

        if(ChatState.fTopMost)
            AppendMenu(hMenu,MF_ENABLED | MF_CHECKED | MF_STRING,IDM_TOPMOST,szAlwaysOnTop);
        else
            AppendMenu(hMenu,MF_ENABLED | MF_UNCHECKED | MF_STRING,IDM_TOPMOST,szAlwaysOnTop);


        // Set topmost style...
        //
        SetWindowPos(hwndApp,ChatState.fTopMost ? HWND_TOPMOST : HWND_NOTOPMOST,0,0,0,0,SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);

        UpdateButtonStates();

#if !defined(_WIN64)

        {
            static NDDESHAREINFO nddeShareInfo = {
                1,              // revision
                szChatShare,
                SHARE_TYPE_STATIC,
                TEXT("WinChat|Chat\0\0"),
                TRUE,           // shared
                FALSE,          // not a service
                TRUE,           // can be started
                SW_SHOWNORMAL,
                {0,0},          // mod id
                0,              // no item list
                TEXT("")
            };

            TCHAR szComputerName[MAX_COMPUTERNAME_LENGTH + 3] = TEXT("\\\\");
            DWORD cbName = MAX_COMPUTERNAME_LENGTH + 1;

            //
            // Make sure NetDDE DSDM has trusted shares set up properly for us.
            // This fix allows us to work with floating profiles.
            //

            START_NETDDE_SERVICES(hwnd);
            GetComputerName(&szComputerName[2],&cbName);
            NDdeShareAdd(szComputerName,2,NULL,(LPBYTE)&nddeShareInfo,sizeof(NDDESHAREINFO));
            NDdeSetTrustedShare(szComputerName, szChatShare,
                    NDDE_TRUST_SHARE_START | NDDE_TRUST_SHARE_INIT);
        }

#endif

    }

    return(bRet);
}


/*---------------------------------------------------------------------------*\
| MAIN WINDOW PROC
|   This is the main event-handler for the application.
|
| created: 11-Nov-91
| history: 29-Dec-92 <chriswil> ported to NT.
|
\*---------------------------------------------------------------------------*/
LRESULT CALLBACK MainWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    LRESULT  lResult;


    lResult = 0l;
    switch(message)
    {
        case WM_CREATE:
            appWMCreateProc(hwnd);
            break;

        case WM_WININICHANGE:
            appWMWinIniChangeProc(hwnd);
            break;

        case WM_ERASEBKGND:
            if((lResult = (LRESULT)appWMEraseBkGndProc(hwnd)) == 0)
                lResult = DefWindowProc(hwnd,message,wParam,lParam);
            break;

        case WM_SETFOCUS:
            appWMSetFocusProc(hwnd);
            break;

        case WM_MENUSELECT:
            appWMMenuSelectProc(hwnd,wParam,lParam);
            break;

        case WM_TIMER:
            appWMTimerProc(hwnd);
            break;

        case WM_PAINT:
            appWMPaintProc(hwnd);
            break;

        case WM_QUERYDRAGICON:
            lResult = (LRESULT)(LPVOID)appWMQueryDragIconProc(hwnd);
            break;

        case WM_SIZE:
            appWMSizeProc(hwnd,wParam,lParam);
            break;

        case WM_INITMENU:
            appWMInitMenuProc((HMENU)wParam);
            break;

        case WM_SYSCOMMAND:
            if(!appWMSysCommandProc(hwnd,wParam,lParam))
                lResult = DefWindowProc(hwnd,message,wParam,lParam);
            break;

        case WM_COMMAND:
            if(!appWMCommandProc(hwnd,wParam,lParam))
                lResult = DefWindowProc(hwnd,message,wParam,lParam);
            break;

        case WM_NOTIFY:
            {
            LPTOOLTIPTEXT lpTTT = (LPTOOLTIPTEXT) lParam;

            if (lpTTT->hdr.code == TTN_NEEDTEXT) {
                LoadString (hInst, (UINT)(MH_BASE + lpTTT->hdr.idFrom), lpTTT->szText, 80);
                return TRUE;
            }
            }
            break;

#ifdef WIN32
        case WM_CTLCOLOREDIT:
        case WM_CTLCOLORSTATIC:
#else
        case WM_CTLCOLOR:
#endif
            if((lResult = (LRESULT)(LPVOID)appWMCtlColorProc(hwnd,wParam,lParam)) == 0l)
                lResult = DefWindowProc(hwnd,message,wParam,lParam);
            break;

        case WM_DESTROY:
            appWMDestroyProc(hwnd);
            break;

        case WM_CLOSE:
            WinHelp(hwnd,(LPTSTR)szHelpFile,HELP_QUIT,0L);

            // Fall through for final close.
            //


        default:
            lResult = DefWindowProc(hwnd,message,wParam,lParam);
            break;
    }

    return(lResult);
}


/*---------------------------------------------------------------------------*\
| EDIT-HOOK PROCEDURE
|   This is the main event-handler for the edit-control hook.
|
| created: 11-Nov-91
| history: 29-Dec-92 <chriswil> ported to NT.
|
\*---------------------------------------------------------------------------*/
LRESULT CALLBACK EditProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    WPARAM wSet;
    LPARAM lSet;
    GETTEXTLENGTHEX gettextlengthex;
    LRESULT lResult;
    LPTSTR lpszText;
    HANDLE hText;
    INT count;
    LPTSTR lpszStartSel;
    DWORD dwTemp1;
    DWORD dwTemp2;

    switch(msg) {
    case WM_IME_COMPOSITION:
        {
            LPWSTR  lpStrParam;
            LPSTR   lpStrTmp;
            HANDLE  hTmp;

            if (lParam & GCS_RESULTSTR)
            {
                HIMC  hImc;
                ULONG cCharsMbcs, cChars;

                //
                // Get input context of hwnd
                //

                if ((hImc = pfnImmGetContext(hwnd)) == 0)
                    break;

                //
                //ImmGetComposition returns the size of buffer needed in byte
                //

                cCharsMbcs =  pfnImmGetCompositionStringW(hImc,GCS_RESULTSTR, NULL , 0);
                if(!(cCharsMbcs))
                {
                    pfnImmReleaseContext(hwnd, hImc);
                    break;
                }

                lpStrParam = (LPWSTR)GlobalAlloc(GPTR,//HEAP_ZERO_MEMORY,
                                    cCharsMbcs + sizeof(WCHAR));

                if (lpStrParam==NULL)
                {
                    pfnImmReleaseContext(hwnd, hImc);
                    break;
                }

                pfnImmGetCompositionStringW(hImc, GCS_RESULTSTR, lpStrParam,
                                    cCharsMbcs);

                //
                // Compute character count including NULL char.
                //

                cChars = wcslen(lpStrParam) + 1;

                //
                // Set ChatData packet
                //

                ChatData.type                = CHT_DBCS_STRING;

                //
                // Get current cursor position
                //
                // !!! BUG BUG BUG !!!
                //
                //  This position data is only nice for Unicode Edit control.
                // is the partner has not Unicode Edit control. the string
                // will be truncated.
                //

                SendMessage(hwndSnd,EM_GETSEL,(WPARAM)&dwTemp1,(LPARAM)&dwTemp2);
                ChatData.uval.cd_dbcs.SelPos = MAKELONG((WORD)dwTemp1, (WORD)dwTemp2 );

                if (gfDbcsEnabled) {
                    //
                    // since text is passed as multi byte character string,
                    // position fixup is needed if DBCS is enabled
                    //
                    DWORD dwStart, dwEnd;

                    TranslateWideCharPosToMultiBytePos( hwndSnd,
                        (DWORD)LOWORD(ChatData.uval.cd_dbcs.SelPos),
                        (DWORD)HIWORD(ChatData.uval.cd_dbcs.SelPos),
                        &dwStart, &dwEnd );
                    ChatData.uval.cd_dbcs.SelPos
                        = MAKELONG((WORD)dwStart, (WORD)dwEnd );
               }

               //
               // Allocate string buffer for DDE.
               //

               if((hTmp = GlobalAlloc( GMEM_ZEROINIT |
                                       GMEM_MOVEABLE |
                                       GMEM_DDESHARE   ,
                                       (DWORD)cCharsMbcs)) == NULL)
               {
                    pfnImmReleaseContext(hwnd, hImc);
                    GlobalFree(lpStrParam);
                    break;
               }

               lpStrTmp                     = GlobalLock(hTmp);

               //
               // Store MBCS string into DDE buffer.
               //
               // In CHT_DBCS_STRING context, we should send mbcs string
               // for downlevel connectivity.
               //

               WideCharToMultiByte(CP_ACP,0,lpStrParam,cChars/* + 1*/,
                                            lpStrTmp  ,cCharsMbcs/* + 1*/,
                                            NULL,NULL);

               //
               // Keep the buffer handle in to DDE message packet.
               //

               GlobalUnlock(hTmp);
               ChatData.uval.cd_dbcs.hString = hTmp;

               //
               // Now, we have a packet to send server/client, just send it.
               //

               wSet = SET_EN_NOTIFY_WPARAM(ID_EDITSND,EN_DBCS_STRING,hwnd);
               lSet = SET_EN_NOTIFY_LPARAM(ID_EDITSND,EN_DBCS_STRING,hwnd);
               SendMessage(hwndApp,WM_COMMAND,wSet,lSet);

               //
               // if we have still a connection to server/client. repaint text.
               //

               if(ChatState.fConnected)
                   SendMessage(hwndSnd,EM_REPLACESEL,0,(LPARAM)lpStrParam);

               pfnImmReleaseContext(hwnd, hImc);
               GlobalFree(lpStrParam);

               return(TRUE);
            }
            break;
        }

#if 0   // FE: obsolete. leave it here only FYI
    case WM_IME_REPORT:
        {
            LPTSTR   lpStrParam,lpStrTmp;
            HANDLE  hTmp;

            if(wParam == IR_STRING)
            {
                if(lpStrParam = GlobalLock((HANDLE)lParam))
                {
                    if(hTmp = GlobalAlloc(GMEM_ZEROINIT | GMEM_MOVEABLE | GMEM_DDESHARE,(DWORD)(lstrlen(lpStrParam) + 1)))
                    {
                        ChatData.type                = CHT_DBCS_STRING;
                        ChatData.uval.cd_dbcs.SelPos = SendMessage(hwndSnd,EM_GETSEL,0,0L);
                        lpStrTmp                     = GlobalLock(hTmp);
                        lstrcpy(lpStrTmp, lpStrParam);
                        GlobalUnlock(hTmp);
                        ChatData.uval.cd_dbcs.hString = hTmp;

                        wSet = SET_EN_NOTIFY_WPARAM(ID_EDITSND,EN_DBCS_STRING,hwnd);
                        lSet = SET_EN_NOTIFY_LPARAM(ID_EDITSND,EN_DBCS_STRING,hwnd);
                        SendMessage(hwndApp,WM_COMMAND,wSet,lSet);

                        if(ChatState.fConnected)
                            SendMessage(hwndSnd,EM_REPLACESEL,0,(LPARAM)lpStrParam);

                        GlobalUnlock((HANDLE)lParam);

                        return(TRUE);
                    }
                    else
                    {
                        GlobalUnlock((HANDLE)lParam);
                        break;
                    }
                }
            }
            break;
        }
#endif  // FE: obsolete

    case WM_KEYDOWN:
        if (wParam == VK_DELETE) {
            DWORD dwLastError;
            ChatData.type                = CHT_CHAR;

            SendMessage(hwndSnd,EM_GETSEL,(WPARAM)&dwTemp1,(LPARAM)&dwTemp2);
            ChatData.uval.cd_dbcs.SelPos = MAKELONG((WORD)dwTemp1, (WORD)dwTemp2 );

            lResult=SendMessage(hwndSnd,WM_GETTEXTLENGTH,0,0);
            // if we are trying to delete at the end of the line then ignore it
            if(lResult<=LOWORD(ChatData.uval.cd_char.SelPos)) break;

            if (LOWORD(ChatData.uval.cd_char.SelPos) == HIWORD(ChatData.uval.cd_char.SelPos)) {

                // get handle to the text
                hText = (HANDLE)SendMessage( hwndSnd, EM_GETHANDLE, 0, 0);
                if( !(hText) )
                    break;

                lpszText = LocalLock( hText);
                if( !(lpszText))
                {
                    LocalUnlock(hText);
                    break;
                }
                lpszStartSel=lpszText;
                for(count=0;count<LOWORD(ChatData.uval.cd_char.SelPos);count++)
                {
                    lpszStartSel=CharNext(lpszStartSel);
                    if(lpszStartSel[0] == TEXT('\0')) break;  // if at the end then break since something is mesed
                }

                if(lpszStartSel[0] != TEXT('\0') && lpszStartSel[0] == TEXT('\r'))
                {
                    if(lpszStartSel[1] != TEXT('\0') && lpszStartSel[1] == TEXT('\n'))
                    {
                        ChatData.uval.cd_char.SelPos=MAKELONG(LOWORD(ChatData.uval.cd_char.SelPos),
                                                              HIWORD(ChatData.uval.cd_char.SelPos)+2);
                    }
                    else
                    {
                        ChatData.uval.cd_char.SelPos=MAKELONG(LOWORD(ChatData.uval.cd_char.SelPos)+1,
                                                              HIWORD(ChatData.uval.cd_char.SelPos)+1);
                    }
                }
                else
                {
                    ChatData.uval.cd_char.SelPos=MAKELONG(LOWORD(ChatData.uval.cd_char.SelPos)+1,
                                                          HIWORD(ChatData.uval.cd_char.SelPos)+1);
                }


                LocalUnlock( hText );
            }


            if (gfDbcsEnabled) {
                DWORD dwStart, dwEnd;

                TranslateWideCharPosToMultiBytePos( hwndSnd,
                    (DWORD)LOWORD(ChatData.uval.cd_dbcs.SelPos),
                    (DWORD)HIWORD(ChatData.uval.cd_dbcs.SelPos),
                     &dwStart, &dwEnd );
                ChatData.uval.cd_dbcs.SelPos
                    = MAKELONG((WORD)dwStart, (WORD)dwEnd);
            }

            ChatData.uval.cd_char.Char   = VK_BACK;

            wSet = SET_EN_NOTIFY_WPARAM(ID_EDITSND,EN_CHAR,hwnd);
            lSet = SET_EN_NOTIFY_LPARAM(ID_EDITSND,EN_CHAR,hwnd);

            SendMessage(hwndApp,WM_COMMAND,(WPARAM)wSet,(LPARAM)lSet);
        }
        break;
    case WM_CHAR:
        if(wParam != CTRL_V)
        {
            ChatData.type                = CHT_CHAR;
            SendMessage(hwndSnd,EM_GETSEL,(WPARAM)&dwTemp1,(LPARAM)&dwTemp2);
            ChatData.uval.cd_dbcs.SelPos = MAKELONG((WORD)dwTemp1, (WORD)dwTemp2 );

            if (gfDbcsEnabled) {
                DWORD dwStart, dwEnd;

                TranslateWideCharPosToMultiBytePos( hwndSnd,
                    (DWORD)LOWORD(ChatData.uval.cd_dbcs.SelPos),
                    (DWORD)HIWORD(ChatData.uval.cd_dbcs.SelPos),
                     &dwStart, &dwEnd );
                ChatData.uval.cd_dbcs.SelPos
                    = MAKELONG((WORD)dwStart, (WORD)dwEnd);
            }

            ChatData.uval.cd_char.Char   = (WORD)wParam;

            wSet = SET_EN_NOTIFY_WPARAM(ID_EDITSND,EN_CHAR,hwnd);
            lSet = SET_EN_NOTIFY_LPARAM(ID_EDITSND,EN_CHAR,hwnd);

            SendMessage(hwndApp,WM_COMMAND,wSet,lSet);
        }
        break;


    case WM_PASTE:
        ChatData.type                 = (WORD)(ChatState.fUnicode ? CHT_PASTEW : CHT_PASTEA);
        SendMessage(hwndSnd,EM_GETSEL,(WPARAM)&dwTemp1,(LPARAM)&dwTemp2);
        ChatData.uval.cd_paste.SelPos = MAKELONG(dwTemp1,dwTemp2);
        wSet = SET_EN_NOTIFY_WPARAM(ID_EDITSND,EN_PASTE,hwnd);
        lSet = SET_EN_NOTIFY_LPARAM(ID_EDITSND,EN_PASTE,hwnd);

        SendMessage(hwndApp,WM_COMMAND,wSet,lSet);
        break;
    }

    return(CallWindowProc(lpfnOldEditProc,hwnd,msg,wParam,lParam));
}


/*---------------------------------------------------------------------------*\
| APPLICATION CREATE PROCEDURE
|   This is the main event-handler for the WM_CREATE event.
|
| created: 11-Nov-91
| history: 29-Dec-92 <chriswil> ported to NT.
|
\*---------------------------------------------------------------------------*/
VOID appWMCreateProc(HWND hwnd)
{
    HDC   hdc;
    TCHAR buf[16];
    RECT  rc;


    // read from ini
    //
    wsprintf(buf,TEXT("%ld"),GetSysColor(COLOR_WINDOW));
    GetPrivateProfileString(szPref,szBkgnd,buf,szBuf,SZBUFSIZ,szIni);
    SndBrushColor = myatol(szBuf);


    // just in case display driver changed, set the send-color.
    //
    hdc = GetDC (hwnd);
    if(hdc)
    {
        SndBrushColor = GetNearestColor(hdc,SndBrushColor);
        ReleaseDC(hwnd,hdc);
    }

    if(ChatState.fUseOwnFont)
    {
        RcvBrushColor = SndBrushColor;
        RcvColorref   = SndColorref;
    }
    else
        RcvBrushColor = GetSysColor ( COLOR_WINDOW );

    ChatState.fConnected          = FALSE;
    ChatState.fConnectPending     = FALSE;
    ChatState.fIsServer           = FALSE;
    ChatState.fServerVerified     = TRUE;
    ChatState.fInProcessOfDialing = FALSE;
    ChatState.fUnicode            = FALSE;

    CreateTools(hwnd);
    CreateChildWindows(hwnd);

    UpdateButtonStates();

    // determine height of toolbar window and save...
    //
    GetClientRect(hwndToolbar, &rc);
    dyButtonBar = rc.bottom - rc.top;

    // determine height of statusbar window and save...
    GetClientRect(hwndStatus, &rc);
    dyStatus = rc.bottom - rc.top;


    // stuff our local font into one or both edit controls
    //
    hEditSndFont = CreateFontIndirect((LPLOGFONT)&lfSnd);
    if(hEditSndFont)
    {
        SendMessage(hwndSnd,WM_SETFONT,(WPARAM)hEditSndFont,1L);
        if(ChatState.fUseOwnFont)
            SendMessage(hwndRcv,WM_SETFONT,(WPARAM)hEditSndFont,1L);
    }


    hwndActiveEdit = hwndSnd;

    return;
}


/*---------------------------------------------------------------------------*\
| APPLICATION WININICHANGE PROCEDURE
|   This is the main event-handler for the WM_WININICHANGE event.
|
| created: 11-Nov-91
| history: 29-Dec-92 <chriswil> ported to NT.
|
\*---------------------------------------------------------------------------*/
VOID appWMWinIniChangeProc(HWND hwnd)
{

    if(hEditSndFont)
    {
        DeleteObject(hEditSndFont);
        hEditSndFont = CreateFontIndirect((LPLOGFONT)&lfSnd);
        if(hEditSndFont)
            SendMessage(hwndSnd,WM_SETFONT,(WPARAM)hEditSndFont,1L);
    }


    if(hEditRcvFont)
    {
        DeleteObject(hEditRcvFont);
        hEditRcvFont = CreateFontIndirect((LPLOGFONT)&lfRcv);
    }


    if(ChatState.fUseOwnFont && hEditSndFont)
        SendMessage(hwndRcv,WM_SETFONT,(WPARAM)hEditSndFont,1L);
    else
    {
        if(hEditRcvFont)
            SendMessage(hwndRcv,WM_SETFONT,(WPARAM)hEditRcvFont,1L);
    }


    return;
}


/*---------------------------------------------------------------------------*\
| APPLICATION ERASEBKGND PROCEDURE
|   This is the main event-handler for the WM_ERASEBKBND event.
|
| created: 11-Nov-91
| history: 29-Dec-92 <chriswil> ported to NT.
|
\*---------------------------------------------------------------------------*/
BOOL appWMEraseBkGndProc(HWND hwnd)
{
    BOOL bErase;


    bErase = IsIconic(hwnd) ? TRUE : FALSE;

    return(bErase);
}


/*---------------------------------------------------------------------------*\
| APPLICATION SETFOCUS PROCEDURE
|   This is the main event-handler for the WM_SETFOCUS event.
|
| created: 11-Nov-91
| history: 29-Dec-92 <chriswil> ported to NT.
|
\*---------------------------------------------------------------------------*/
VOID appWMSetFocusProc(HWND hwnd)
{
    SetFocus(hwndActiveEdit);

    return;
}


/*---------------------------------------------------------------------------*\
| APPLICATION CTLCOLOR PROCEDURE
|   This is the main event-handler for the WM_CTLCOLOR event.
|
| created: 11-Nov-91
| history: 29-Dec-92 <chriswil> ported to NT.
|
\*---------------------------------------------------------------------------*/
HBRUSH appWMCtlColorProc(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    HDC    hDC;
    HWND   hWndCtl;
    HBRUSH hBrush;


    hBrush  = NULL;
    hDC     = GET_WM_CTLCOLOREDIT_HDC(wParam,lParam);
    hWndCtl = GET_WM_CTLCOLOREDIT_HWND(wParam,lParam);


    if(hWndCtl == hwndSnd)
    {
        SetTextColor(hDC,SndColorref);
        SetBkColor(hDC,SndBrushColor);

        hBrush = hEditSndBrush;
    }
    else
    if(hWndCtl == hwndRcv)
    {
        if(ChatState.fUseOwnFont)
        {
            SetTextColor(hDC,SndColorref);
            SetBkColor(hDC,SndBrushColor);
        }
        else
        {
            SetTextColor(hDC,RcvColorref);
            SetBkColor(hDC,RcvBrushColor);
        }

        hBrush = hEditRcvBrush;
    }

    return(hBrush);
}


/*---------------------------------------------------------------------------*\
| APPLICATION SELECTMENU PROCEDURE
|   This is the main event-handler for the WM_MENUSELECT event.
|
| created: 11-Nov-91
| history: 29-Dec-92 <chriswil> ported to NT.
|
\*---------------------------------------------------------------------------*/
VOID appWMMenuSelectProc(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    if(wParam == IDM_TOPMOST)
    {
        if(LoadString(hInst,MH_BASE+IDM_TOPMOST,szBuf,SZBUFSIZ))
            SendMessage(hwndStatus,SB_SETTEXT,SBT_NOBORDERS|255,(LPARAM)(LPSTR)szBuf);
    }

    MenuHelp((WORD)WM_MENUSELECT,wParam,lParam,GetMenu(hwnd),hInst,hwndStatus,(LPUINT)nIDs);

    return;
}


/*---------------------------------------------------------------------------*\
| APPLICATION PAINT PROCEDURE
|   This is the main event-handler for the WM_PAINT event.
|
| created: 11-Nov-91
| history: 29-Dec-92 <chriswil> ported to NT.
|
\*---------------------------------------------------------------------------*/
VOID appWMPaintProc(HWND hwnd)
{
    HDC         hdc;
        PAINTSTRUCT ps;
    RECT        rc;

    hdc = BeginPaint(hwnd,&ps);
    if(hdc)
    {
        if(IsIconic(hwnd))
        {
            //
            //
            DefWindowProc(hwnd,WM_ICONERASEBKGND,(WPARAM)ps.hdc,0L);
            BitBlt(hMemDC,0,0,cxIcon,cyIcon,hdc,0,0,SRCCOPY);
            DrawIcon(hdc,0,0,hPhones[0]);


            // make 2 more copies.
            //
            BitBlt(hMemDC,cxIcon  ,0,cxIcon,cyIcon,hMemDC,0,0,SRCCOPY);
            BitBlt(hMemDC,2*cxIcon,0,cxIcon,cyIcon,hMemDC,0,0,SRCCOPY);

            // draw phones into them.
            //
            DrawIcon(hMemDC,0       ,0,hPhones[0]);
            DrawIcon(hMemDC,cxIcon  ,0,hPhones[1]);
            DrawIcon(hMemDC,2*cxIcon,0,hPhones[2]);
        }
        else
        {

#if BRD > 2
            rc = SndRc;
            rc.top--;
            rc.left--;
            DrawShadowRect(hdc,&rc);
            rc = RcvRc;
            rc.top--;
            rc.left--;
            DrawShadowRect(hdc,&rc);
#endif
        }

        EndPaint ( hwnd, &ps );
    }

    return;
}


/*---------------------------------------------------------------------------*\
| APPLICATION TIMER PROCEDURE
|   This is the main event-handler for the WM_TIMER event.
|
| created: 11-Nov-91
| history: 29-Dec-92 <chriswil> ported to NT.
|
\*---------------------------------------------------------------------------*/
VOID appWMTimerProc(HWND hwnd)
{
    HDC   hdc;
    DWORD dummy;


    // Animate the phone icon.
    //
    if(cAnimate)
    {
        if(--cAnimate == 0)
        {
            KillTimer(hwnd,idTimer);
            FlashWindow(hwnd,FALSE);
                }

        if(IsIconic(hwnd))
        {
            hdc = GetDC(hwndApp);
            if(hdc)
            {
                BitBlt(hdc,0,0,cxIcon,cyIcon,hMemDC,ASeq[cAnimate % 4] * cxIcon,0,SRCCOPY);
                ReleaseDC(hwndApp,hdc);
            }
        }

                return;
        }



    // We must be ringing...
    //
    if(!ChatState.fConnectPending)
    {
        KillTimer(hwnd,idTimer);
                return;
        }


    // has the existence of the server been verified (by completion
    // of the async advstart xact)?
    //
    if(!ChatState.fServerVerified)
    {
                return;
        }


    // don't want to lose this...
    //
    DdeKeepStringHandle(idInst,hszLocalName);

    if(DdeClientTransaction(NULL,0L,ghConv,hszLocalName,cf_chatdata,XTYP_ADVSTART,(DWORD)3000L,(LPDWORD)&dummy) == (HDDEDATA)TRUE)
    {
        ChatState.fConnected      = TRUE;
        ChatState.fConnectPending = FALSE;
        UpdateButtonStates();

        KILLSOUND;

        SendFontToPartner();

        wsprintf(szBuf,szConnectedTo,(LPSTR)szConvPartner);
        SetStatusWindowText(szBuf);

        wsprintf(szBuf,TEXT("%s - [%s]"),(LPTSTR)szAppName,(LPTSTR)szConvPartner);
        SetWindowText(hwnd,szBuf);


        // allow text entry...
        //
        SendMessage(hwndSnd,EM_SETREADONLY,(WPARAM)FALSE,0L);

        KillTimer(hwnd,idTimer);

        AnnounceSupport();
    }
    else
    {
        // The other party has not answered yet... ring every 6 seconds.
        // Ring local,
        //
        if(!(nConnectAttempt++ % 6))
            DoRing(szWcRingOut);
    }

    return;
}


/*---------------------------------------------------------------------------*\
| APPLICATION QUERYDRAGICON PROCEDURE
|   This is the main event-handler for the WM_QUERYDRAGICON event.
|
| created: 11-Nov-91
| history: 29-Dec-92 <chriswil> ported to NT.
|
\*---------------------------------------------------------------------------*/
HICON appWMQueryDragIconProc(HWND hwnd)
{
    HICON hIcon;


    hIcon = hPhones[0];

    return(hIcon);
}


/*---------------------------------------------------------------------------*\
| APPLICATION SIZE PROCEDURE
|   This is the main event-handler for the WM_SIZE event.
|
| created: 11-Nov-91
| history: 29-Dec-92 <chriswil> ported to NT.
|
\*---------------------------------------------------------------------------*/
VOID appWMSizeProc(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    if(ChatState.fMinimized && ChatState.fConnectPending && ChatState.fIsServer)
    {
        ChatState.fAllowAnswer = TRUE;
        SetStatusWindowText(szConnecting);


        // stop the ringing immediately.
        //
        KILLSOUND;
        if(ChatState.fMMSound)
            sndPlaySound(NULL,SND_ASYNC);

        // cut the animation short.
        //
        if(cAnimate)
            cAnimate = 1;
    }


    //
    //
    InvalidateRect(hwnd,NULL,TRUE);
    SendMessage(hwndToolbar,WM_SIZE,0,0L);
    SendMessage(hwndStatus ,WM_SIZE,0,0L);
    AdjustEditWindows();

    ChatState.fMinimized = (wParam == SIZE_MINIMIZED) ? TRUE : FALSE;

    return;
}


/*---------------------------------------------------------------------------*\
| APPLICATION INITMENU PROCEDURE
|   This is the main event-handler for the WM_INITMENU event.
|
| created: 11-Nov-91
| history: 29-Dec-92 <chriswil> ported to NT.
|
\*---------------------------------------------------------------------------*/
VOID appWMInitMenuProc(HMENU hmenu)
{
    UINT status;
    LONG l;
    TCHAR szTest[] = TEXT(" ");
    DWORD dwTemp1,dwTemp2;

    SendMessage(hwndActiveEdit,EM_GETSEL,(LPARAM)&dwTemp1,(WPARAM)&dwTemp2);
    l = MAKELONG(dwTemp1,dwTemp2);

    if(HIWORD(l) != LOWORD(l))
                status = MF_ENABLED;
        else
                status = MF_GRAYED;

    EnableMenuItem(hmenu,IDM_EDITCUT ,(hwndActiveEdit == hwndSnd && ChatState.fConnected) ? status : MF_GRAYED);
    EnableMenuItem(hmenu,IDM_EDITCOPY,status);

    status = MF_GRAYED;
    if(hwndActiveEdit == hwndSnd && ChatState.fConnected && IsClipboardFormatAvailable(CF_TEXT))
    {
        status = MF_ENABLED;
        }
    EnableMenuItem(hmenu,IDM_EDITPASTE,status);


    // select all enabled if control non-empty.
    //
    status = MF_GRAYED;
    if(SendMessage(hwndActiveEdit,WM_GETTEXT,2,(LPARAM)szTest))
                status = MF_ENABLED;
    EnableMenuItem(hmenu,IDM_EDITSELECT,status);


    // can we dial, answer and hangup.
    //
    EnableMenuItem(hmenu,IDM_DIAL  ,(!ChatState.fConnected     && !ChatState.fConnectPending) ? MF_ENABLED : MF_GRAYED);
    EnableMenuItem(hmenu,IDM_ANSWER,(ChatState.fConnectPending && ChatState.fIsServer)        ? MF_ENABLED : MF_GRAYED);
    EnableMenuItem(hmenu,IDM_HANGUP,(ChatState.fConnected      || ChatState.fConnectPending)  ? MF_ENABLED : MF_GRAYED);


    // Is toolbar, statusbar and sound allowed?
    //
    CheckMenuItem(hmenu,IDM_SOUND    ,(ChatState.fSound)     ? MF_CHECKED : MF_UNCHECKED);
    CheckMenuItem(hmenu,IDM_TOOLBAR  ,(ChatState.fToolBar)   ? MF_CHECKED : MF_UNCHECKED);
    CheckMenuItem(hmenu,IDM_STATUSBAR,(ChatState.fStatusBar) ? MF_CHECKED : MF_UNCHECKED);

    return;
}


/*---------------------------------------------------------------------------*\
| APPLICATION SYSCOMMAND PROCEDURE
|   This is the main event-handler for the WM_SYSCOMMAND event.
|
| created: 11-Nov-91
| history: 29-Dec-92 <chriswil> ported to NT.
|
\*---------------------------------------------------------------------------*/
LRESULT appWMSysCommandProc(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    LRESULT lResult;
    HMENU   hmenu;


    lResult = 0l;
    switch(wParam)
    {
        case IDM_TOPMOST:
            ChatState.fTopMost = ChatState.fTopMost ? FALSE : TRUE;

            hmenu = GetSystemMenu(hwnd,FALSE);
            if(hmenu)
                CheckMenuItem(hmenu,IDM_TOPMOST,(ChatState.fTopMost) ? MF_CHECKED : MF_UNCHECKED);

            SetWindowPos(hwnd,ChatState.fTopMost ? HWND_TOPMOST : HWND_NOTOPMOST,0,0,0,0,SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
            break;
    }

    return(lResult);
}


/*---------------------------------------------------------------------------*\
| APPLICATION COMMAND PROCEDURE
|   This is the main event-handler for the WM_COMMAND event.
|
| created: 11-Nov-91
| history: 29-Dec-92 <chriswil> ported to NT.
|
\*---------------------------------------------------------------------------*/
BOOL appWMCommandProc(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    HDC      hdc;
    int      tmp;
    UINT     uNotify;
    DWORD    dummy;
    DWORD    dwBufSize;
    HDDEDATA hDdeData;
    BOOL     bHandled,bOK,bOKPressed=FALSE;
    WPARAM   wSelStart;
    LPARAM   lSelEnd;


    bHandled = TRUE;
    switch(LOWORD(wParam))
    {
                case ID_EDITRCV:
            uNotify = GET_EN_SETFOCUS_NOTIFY(wParam,lParam);
            switch(uNotify)
            {
                case EN_SETFOCUS:
                    hwndActiveEdit = hwndRcv;
                    break;


                // If the control is out of space, honk.
                //
                case EN_ERRSPACE:
                    MessageBeep(0);
                    break;
                        }
            break;


                case ID_EDITSND:
            uNotify = GET_EN_SETFOCUS_NOTIFY(wParam,lParam);
            switch(uNotify)
            {
                // This string came from the edit-hook
                // procedure.
                //
                case EN_DBCS_STRING:
                    if(ChatState.fConnected)
                    {
                        if(!ChatState.fIsServer)
                        {
                            hDdeData = CreateDbcsStringData();
                            if(hDdeData)
                                DdeClientTransaction((LPBYTE)hDdeData,(DWORD)-1,ghConv,hszTextItem,cf_chatdata,XTYP_POKE,(DWORD)TIMEOUT_ASYNC,(LPDWORD)&dummy);
                        }
                        else
                        {
                            hszConvPartner = DdeCreateStringHandle(idInst,szConvPartner,0);
                            DdePostAdvise(idInst,hszChatTopic,hszConvPartner);
                        }
                    }
                    break;


                // This character came from the edit-hook
                // procedure.
                //
                case EN_CHAR:
                    if(ChatState.fConnected)
                    {
                        if(!ChatState.fIsServer)
                        {
                            hDdeData = CreateCharData();
                            if(hDdeData)
                                DdeClientTransaction((LPBYTE)hDdeData,(DWORD)-1,ghConv,hszTextItem,cf_chatdata,XTYP_POKE,(DWORD)TIMEOUT_ASYNC,(LPDWORD)&dummy);
                        }
                        else
                        {
                            hszConvPartner = DdeCreateStringHandle(idInst,szConvPartner,0);
                            DdePostAdvise(idInst,hszChatTopic,hszConvPartner);
                        }
                    }
                    break;

                case EN_PASTE:
                    if(ChatState.fConnected)
                    {
                        if(!ChatState.fIsServer)
                        {
                            if(IsClipboardFormatAvailable(CF_UNICODETEXT))
                            {
                                hDdeData = CreatePasteData();
                                if(hDdeData)
                                    DdeClientTransaction((LPBYTE)hDdeData,(DWORD)-1,ghConv,hszTextItem,cf_chatdata,XTYP_POKE,(DWORD)TIMEOUT_ASYNC,(LPDWORD)&StrXactID);
                            }
                        }
                        else
                        {
                             hszConvPartner = DdeCreateStringHandle(idInst,szConvPartner,0);
                             DdePostAdvise(idInst,hszChatTopic,hszConvPartner);
                        }
                    }
                    break;

                case EN_SETFOCUS:
                    hwndActiveEdit = hwndSnd;
                    break;

                case EN_ERRSPACE:
                    // If the control is out of space, honk.
                    //
                    MessageBeep(0);
                    break;
                    }
                    break;


        case IDC_TOOLBAR:
            MenuHelp(WM_COMMAND,wParam,lParam,GetMenu(hwnd),hInst,hwndStatus,(LPUINT)nIDs);
            break;


        case IDM_EXIT:
            SendMessage(hwnd,WM_CLOSE,0,0L);
            break;


        case IDM_TOOLBAR:
            if(ChatState.fToolBar)
            {
                ChatState.fToolBar = FALSE;
                ShowWindow(hwndToolbar,SW_HIDE);
                InvalidateRect(hwnd,NULL,TRUE);
                AdjustEditWindows();
            }
            else
            {
                ChatState.fToolBar = TRUE;
                InvalidateRect(hwnd,NULL,TRUE);
                AdjustEditWindows();
                ShowWindow(hwndToolbar,SW_SHOW);
            }
            break;


        case IDM_STATUSBAR:
            if(ChatState.fStatusBar)
            {
                ChatState.fStatusBar = FALSE;
                ShowWindow(hwndStatus,SW_HIDE);
                InvalidateRect(hwnd,NULL,TRUE);
                AdjustEditWindows();
            }
            else
            {
                ChatState.fStatusBar = TRUE;
                InvalidateRect(hwnd,NULL,TRUE);
                AdjustEditWindows();
                ShowWindow(hwndStatus,SW_SHOW);
            }
            break;


        case IDM_SWITCHWIN:
            if(hwndActiveEdit == hwndSnd)
                SetFocus(hwndActiveEdit = hwndRcv);
            else
                SetFocus(hwndActiveEdit = hwndSnd);
            break;


        case IDM_SOUND:
            ChatState.fSound = ChatState.fSound ? FALSE : TRUE;
            break;


        case IDM_COLOR:
            SetFocus(hwndActiveEdit);
            chc.rgbResult = SndBrushColor;

            tmp = ChooseColor((LPCHOOSECOLOR)&chc);
            if(tmp)
            {
                hdc = GetDC(hwnd);
                if(hdc)
                {
                    // must map to solid color (edit-control limitation).
                    //
                    SndBrushColor = GetNearestColor(hdc,chc.rgbResult);
                    ReleaseDC(hwnd,hdc);
                }

                DeleteObject(hEditSndBrush);
                hEditSndBrush = CreateSolidBrush(SndBrushColor);
                InvalidateRect(hwndSnd,NULL,TRUE);

                SaveBkGndToIni();

                if(ChatState.fUseOwnFont)
                {
                    RcvBrushColor = SndBrushColor;
                    DeleteObject(hEditRcvBrush);
                    hEditRcvBrush = CreateSolidBrush(RcvBrushColor);
                    InvalidateRect(hwndRcv, NULL, TRUE);
                }

                if(ChatState.fConnected)
                    SendFontToPartner();
            }
            break;


        case IDM_FONT:
            SetFocus(hwndActiveEdit);
            chf.hwndOwner = hwndSnd;
            chf.rgbColors = SndColorref;

            tmp = ChooseFont((LPCHOOSEFONT)&chf);
            if(tmp)
            {
                if(hEditSndFont)
                    DeleteObject(hEditSndFont);

                hEditSndFont = CreateFontIndirect((LPLOGFONT)&lfSnd);
                if(hEditSndFont)
                {
                    SndColorref = chf.rgbColors;
                    SaveFontToIni();

                    SendMessage(hwndSnd,WM_SETFONT,(WPARAM)hEditSndFont,1L);
                    if(ChatState.fUseOwnFont)
                    {
                        SendMessage(hwndRcv,WM_SETFONT,(WPARAM)hEditSndFont,1L);
                        RcvColorref = SndColorref;
                    }


                    // notify partner of the change
                    //
                    if(ChatState.fConnected)
                        SendFontToPartner();
                }
            }
            break;


        case IDM_DIAL:
            if(ChatState.fConnected)
            {
                SetStatusWindowText(szAlreadyConnect);
                break;
            }

            if(ChatState.fConnectPending)
            {
                SetStatusWindowText ( szAbandonFirst);
                break;
            }

            dwBufSize = SZBUFSIZ;

            WNETGETUSER((LPTSTR)NULL,(LPTSTR)szBuf,&dwBufSize);

            if(GetLastError() == ERROR_NO_NETWORK)
            {
                if(MessageBox(hwnd,szNoNet,TEXT("Chat"),MB_YESNO | MB_ICONQUESTION) == IDNO)
                    break;
            }


            ChatState.fInProcessOfDialing = TRUE;
            if(WNetServerBrowseDialog == NULL || (*WNetServerBrowseDialog)(hwnd,TEXT("MRU_Chat"),szBuf,SZBUFSIZ,0L) == WN_NOT_SUPPORTED)
            {
#if WIN32
                bOKPressed = FALSE;
                *szBuf     = TEXT('\0');


                lstrcpy(szHelp, TEXT("winchat.hlp"));
                I_SystemFocusDialog(hwnd,MY_LOGONTYPE,(LPWSTR)szBuf,SZBUFSIZ,&bOKPressed,(LPWSTR)szHelp,IDH_SELECTCOMPUTER);

                if(bOKPressed)
                {
                    bOK    = TRUE;
                    lstrcpy(szConvPartner,szBuf);
                }
#else
                dlgDisplayBox(hInst,hwnd,(LPSTR)MAKEINTRESOURCE(IDD_CONNECT),(DLGPROC)dlgConnectProc,0l);
#endif
            }

            SetFocus(hwndActiveEdit);

            if(*szBuf && bOKPressed)
            {
                CharUpper(szBuf);

                if((lstrlen(szBuf) > 2)  && (szBuf[0] == TEXT('\\')) && (szBuf[1] == TEXT('\\')))
                    lstrcpy(szConvPartner,szBuf+2);
                else
                    lstrcpy(szConvPartner,szBuf);

                ClearEditControls();

                wsprintf(szBuf,szDialing,(LPSTR)szConvPartner);
                SetStatusWindowText(szBuf);

#if TESTLOCAL
                wsprintf(szBuf,TEXT("%s"),(LPTSTR)szServiceName);
                hszConnect = DdeCreateStringHandle(idInst,szBuf,0);
                ghConv     = DdeConnect(idInst,hszConnect,hszChatTopic,NULL);
#else
                wsprintf(szBuf,TEXT("\\\\%s\\NDDE$"),(LPTSTR)szConvPartner);
                hszConnect = DdeCreateStringHandle(idInst,szBuf,0);
                ghConv     = DdeConnect(idInst,hszConnect,hszChatShare,NULL);
#endif

                if(ghConv == (HCONV)0)
                {
                    SetStatusWindowText(szNoConnect);
                    DdeFreeStringHandle(idInst,hszConnect);
                    ChatState.fInProcessOfDialing = FALSE;
                    break;
                }

                ChatState.fConnectPending = TRUE;
                UpdateButtonStates();

                // set up server verify async xaction.
                //
                ChatState.fServerVerified = FALSE;
                DdeKeepStringHandle(idInst,hszConnectTest);
                DdeClientTransaction(NULL,0L,ghConv,hszConnectTest,cf_chatdata,XTYP_ADVSTART,(DWORD)TIMEOUT_ASYNC,(LPDWORD)&XactID);


                // Indicate that this is a Unicode conversation.
                //
                ChatData.type = CHT_UNICODE;
                hDdeData = CreateCharData ();
                if(hDdeData)
                   DdeClientTransaction((LPBYTE)hDdeData,(DWORD)-1,ghConv,hszTextItem,cf_chatdata,XTYP_POKE,(DWORD)TIMEOUT_ASYNC,(LPDWORD)&dummy);


                // set ring timer...
                // connect attempts every second - will be divided by
                // 6 for actual phone rings. This is done to speed the
                // connection process
                // want first message immediately...
                //
                idTimer = SetTimer(hwnd,1,1000,NULL);
                PostMessage(hwnd,WM_TIMER,1,0L);
                nConnectAttempt = 0;
            }

            ChatState.fInProcessOfDialing = FALSE;
            DdeFreeStringHandle(idInst,hszConnect);
            break;


        case IDM_ANSWER:
            if(ChatState.fConnectPending)
            {
                if(!ChatState.fIsServer)
                {
                    SetStatusWindowText(szYouCaller);
                    break;
                }
                else
                {
                    // allow the connection.
                    //
                    ChatState.fAllowAnswer = TRUE;
                    SetStatusWindowText(szConnecting);


                    // stop the ringing immediately.
                    //
                    if(ChatState.fMMSound)
                        sndPlaySound(NULL,SND_ASYNC);


                    // cut the animation short.
                    //
                    if(cAnimate)
                        cAnimate = 1;
                }
            }
            break;


                case IDM_HANGUP:
            if(!ChatState.fConnected && !ChatState.fConnectPending)
            {
                                break;
                        }


            if(ChatState.fConnectPending && !ChatState.fConnected)
            {
                SetStatusWindowText(szConnectAbandon);
            }
            else
            {
                SetStatusWindowText(szHangingUp);
            }


                        KILLSOUND;

            DdeDisconnect(ghConv);

                        ChatState.fConnectPending = FALSE;
            ChatState.fConnected      = FALSE;
            ChatState.fIsServer       = FALSE;
            ChatState.fUnicode        = FALSE;

#ifdef PROTOCOL_NEGOTIATE
            ChatState.fProtocolSent   = FALSE;
#endif

            // suspend text entry.
            //
            UpdateButtonStates();
            SendMessage(hwndSnd,EM_SETREADONLY,TRUE,0L);
            SetWindowText(hwndApp,szAppName);
            break;


       case IDX_UNICODECONV:
            ChatData.type  = CHT_UNICODE;
            hszConvPartner = DdeCreateStringHandle(idInst,szConvPartner,0);
            DdePostAdvise(idInst,hszChatTopic,hszConvPartner);
            break;


                case IDX_DEFERFONTCHANGE:
                        SendFontToPartner();
            break;

#ifdef PROTOCOL_NEGOTIATE
        case IDX_DEFERPROTOCOL:
            AnnounceSupport();
            break;
#endif

        case IDM_CONTENTS:
            HtmlHelpA(GetDesktopWindow(),"winchat.chm",HH_DISPLAY_TOPIC,0L);
            break;


        case IDM_ABOUT:
            ShellAbout(hwndSnd,szAppName,szNull,hPhones[0]);
                        SetFocus(hwndActiveEdit);
            break;


        case IDM_PREFERENCES:
            DialogBoxParam(hInst,(LPTSTR)MAKEINTRESOURCE(IDD_PREFERENCES),hwnd,(DLGPROC)dlgPreferencesProc,(LPARAM)0);
            break;


        case IDM_EDITCOPY:
            SendMessage(hwndActiveEdit,WM_COPY,0,0L);
            break;


        case IDM_EDITPASTE:
            SendMessage(hwndActiveEdit,WM_PASTE,0,0L);
            break;


        case IDM_EDITCUT:
            SendMessage(hwndActiveEdit,WM_CUT,0,0L);
            break;


        case IDM_EDITSELECT:
            wSelStart = SET_EM_SETSEL_WPARAM(0,-1);
            lSelEnd   = SET_EM_SETSEL_LPARAM(0,-1);
            SendMessage(hwndActiveEdit,EM_SETSEL,wSelStart,lSelEnd);
            break;


        case IDM_EDITUNDO:
            SendMessage(hwndActiveEdit,EM_UNDO,0,0L);
            break;


        default:
            bHandled = FALSE;
            break;
    }

    return(bHandled);
}


/*---------------------------------------------------------------------------*\
| APPLICATION DESTROY PROCEDURE
|   This is the main event-handler for the WM_DESTROY event.
|
| created: 11-Nov-91
| history: 29-Dec-92 <chriswil> ported to NT.
|
\*---------------------------------------------------------------------------*/
VOID appWMDestroyProc(HWND hwnd)
{

    // Abandon transaction if in progress.  Force hangup
    // of conversation.
    //
    if(!ChatState.fServerVerified)
        DdeAbandonTransaction(idInst,ghConv,XactID);
    SendMessage(hwnd,WM_COMMAND,IDM_HANGUP,0L);


    // Destroy resources allocated on behalf of app.
    //
    KILLSOUND;
    DeleteTools(hwnd);


    // Save the state information.
    //
    Wpl.length = sizeof(Wpl);
    if(GetWindowPlacement(hwnd,&Wpl))
        SaveWindowPlacement(&Wpl);

    wsprintf(szBuf, TEXT("%d"), (UINT)ChatState.fSound);
    WritePrivateProfileString(szPref, szSnd, szBuf, szIni);

    wsprintf(szBuf, TEXT("%d"), (UINT)ChatState.fToolBar);
    WritePrivateProfileString(szPref, szTool, szBuf, szIni);

    wsprintf(szBuf, TEXT("%d"), (UINT)ChatState.fStatusBar);
    WritePrivateProfileString(szPref, szStat, szBuf, szIni);

    wsprintf(szBuf, TEXT("%d"), (UINT)ChatState.fTopMost);
    WritePrivateProfileString(szPref, szTop, szBuf, szIni);

    wsprintf(szBuf, TEXT("%d"), (UINT)ChatState.fSideBySide);
    WritePrivateProfileString(szPref, szSbS, szBuf, szIni);

    wsprintf(szBuf, TEXT("%d"), (UINT)ChatState.fUseOwnFont);
    WritePrivateProfileString(szPref, szUseOF, szBuf, szIni);

    PostQuitMessage(0);

    return;
}


/*---------------------------------------------------------------------------*\
| ASCII TO LONG
|   This routine converts an ascii string to long.
|
| created: 11-Nov-91
| history: 29-Dec-92 <chriswil> ported to NT.
|
\*---------------------------------------------------------------------------*/
LONG FAR myatol(LPTSTR s)
{
        LONG ret = 0L;


    while(*s) ret = ret * 10 + (*s++ - TEXT('0'));

    return(ret);
}

/*---------------------------------------------------------------------------*\
| UPDATE BUTTON STATES
|   This routine updates the menu/toolbar buttons.
|
| created: 11-Nov-91
| history: 29-Dec-92 <chriswil> ported to NT.
|
\*---------------------------------------------------------------------------*/
VOID FAR UpdateButtonStates(VOID)
{
    BOOL DialState   = FALSE;
        BOOL AnswerState = FALSE;
        BOOL HangUpState = FALSE;


    if(ChatState.fConnected)
                HangUpState = TRUE;
    else
    if(ChatState.fConnectPending)
    {
        if(!ChatState.fIsServer)
                        HangUpState = TRUE;
                else
                        AnswerState = TRUE;
        }
        else
                DialState = TRUE;

    SendMessage(hwndToolbar,TB_ENABLEBUTTON,IDM_DIAL  ,DialState);
    SendMessage(hwndToolbar,TB_ENABLEBUTTON,IDM_ANSWER,AnswerState);
    SendMessage(hwndToolbar,TB_ENABLEBUTTON,IDM_HANGUP,HangUpState);

    return;
}


/*---------------------------------------------------------------------------*\
| GET COMPUTER NAME
|   This routine returns the computer name of the machine.
|
| created: 31-Dec-92
| history: 31-Dec-92 <chriswil> created.
|
\*---------------------------------------------------------------------------*/
BOOL FAR appGetComputerName(LPTSTR lpszName)
{
    BOOL  bGet;
    DWORD dwSize;


#ifdef WIN32

    dwSize = MAX_COMPUTERNAME_LENGTH+1;
    bGet   = GetComputerName(lpszName,&dwSize);

#else

    bGet   = TRUE;
    dwSize = 0l;
    if(GetPrivateProfileString(szVredir,szComputerName,szNull,lpszName,UNCNLEN,szSysIni))
        OemToAnsi(lpszName,lpszName);

#endif

    return(bGet);
}


/*---------------------------------------------------------------------------*\
| ADJUST EDIT WINDOWS
|   This routine sizes the edit-controls.
|
| created: 11-Nov-91
| history: 29-Dec-92 <chriswil> ported to NT.
|
\*---------------------------------------------------------------------------*/
VOID FAR AdjustEditWindows(VOID)
{
    int  tmpsplit;
        RECT rc;


    GetClientRect(hwndApp,&rc);

    rc.top    += ChatState.fToolBar   ? dyButtonBar + BRD : BRD;
    rc.bottom -= ChatState.fStatusBar ? dyStatus    + BRD : BRD;

    if(!ChatState.fSideBySide)
    {
        tmpsplit = rc.top + (rc.bottom - rc.top) / 2;

        SndRc.left   = RcvRc.left  = rc.left  - 1 + BRD;
        SndRc.right  = RcvRc.right = rc.right + 1 - BRD;
        SndRc.top    = rc.top;
                SndRc.bottom = tmpsplit;
        RcvRc.top    = tmpsplit + BRD;
                RcvRc.bottom = rc.bottom;
        }
    else
    {
        tmpsplit = rc.left + (rc.right - rc.left) / 2;

        SndRc.left   = rc.left  - 1   + BRD;
        SndRc.right  = tmpsplit - BRD / 2;
        RcvRc.left   = tmpsplit + BRD / 2;
        RcvRc.right  = rc.right + 1 - BRD;
        SndRc.top    = RcvRc.top    = rc.top;
        SndRc.bottom = RcvRc.bottom = rc.bottom;

    }

    MoveWindow(hwndSnd,SndRc.left,SndRc.top,SndRc.right-SndRc.left,SndRc.bottom-SndRc.top,TRUE);
    MoveWindow(hwndRcv,RcvRc.left,RcvRc.top,RcvRc.right-RcvRc.left,RcvRc.bottom-RcvRc.top,TRUE);

    return;
}


/*---------------------------------------------------------------------------*\
| CLEAR EDIT CONTROLS
|   This routine clears the send/receive edit controls.
|
| created: 11-Nov-91
| history: 29-Dec-92 <chriswil> ported to NT.
|
\*---------------------------------------------------------------------------*/
VOID ClearEditControls(VOID)
{
    SendMessage(hwndSnd,EM_SETREADONLY,FALSE,0L);
    SendMessage(hwndSnd,WM_SETTEXT    ,0    ,(LPARAM)(LPSTR)szNull);
    SendMessage(hwndSnd,EM_SETREADONLY,TRUE ,0L);

    SendMessage(hwndRcv,EM_SETREADONLY,FALSE,0L);
    SendMessage(hwndRcv,WM_SETTEXT    ,0    ,(LPARAM)(LPSTR)szNull);
    SendMessage(hwndRcv,EM_SETREADONLY,TRUE ,0L);

    return;
}


/*---------------------------------------------------------------------------*\
| DO RING
|   This routine performs the phone ringing.
|
| created: 11-Nov-91
| history: 29-Dec-92 <chriswil> ported to NT.
|
\*---------------------------------------------------------------------------*/
VOID DoRing(LPCTSTR sound)
{
    if(ChatState.fSound)
    {
        if(ChatState.fMMSound)
            sndPlaySound(sound,SND_ASYNC);
        else
            MessageBeep(0);
    }

    return;
}


/*---------------------------------------------------------------------------*\
| DRAW SHADOW RECT
|   This routine draws a shadow outline.
|
| created: 11-Nov-91
| history: 29-Dec-92 <chriswil> ported to NT.
|
\*---------------------------------------------------------------------------*/
VOID DrawShadowRect(HDC hdc, LPRECT rc)
{
    HPEN hSavePen = SelectObject(hdc,hShadowPen);


    MoveToEx(hdc,rc->left,rc->bottom,NULL);
    LineTo(hdc,rc->left,rc->top );
    LineTo(hdc,rc->right,rc->top );
    SelectObject(hdc,hHilitePen);
    LineTo(hdc,rc->right,rc->bottom);
    LineTo(hdc,rc->left-1,rc->bottom);
    SelectObject(hdc,hSavePen);

    return;
}

#ifdef PROTOCOL_NEGOTIATE
/*---------------------------------------------------------------------------*\
| ANNOUNCE SUPPORT
|   This routine announces to the partner what we support.
|
| created: 11-Nov-91
| history: 29-Dec-92 <chriswil> ported to NT.
|
\*---------------------------------------------------------------------------*/
VOID AnnounceSupport(VOID)
{
    HDDEDATA hDdeData;
    DWORD    dummy;


    if(ChatState.fConnected)
    {
        ChatData.type = CHT_PROTOCOL;

        if(!ChatState.fIsServer)
        {
            hDdeData = CreateProtocolData();
            if(hDdeData)
                DdeClientTransaction((LPBYTE)hDdeData,(DWORD)-1L,ghConv,hszTextItem,cf_chatdata,XTYP_POKE,(DWORD)TIMEOUT_ASYNC,(LPDWORD)&dummy);
        }
        else
        {
            hszConvPartner = DdeCreateStringHandle(idInst,szConvPartner,0);
            if(hszConvPartner)
                DdePostAdvise(idInst,hszChatTopic,hszConvPartner);
        }

        ChatState.fProtocolSent = TRUE;
    }

    return;
}
#endif


/*---------------------------------------------------------------------------*\
| START INI-FILE MAPPING
|   This routines sets the private-profile settings to go to the registry on\
|   a per-user basis.
|
|
\*---------------------------------------------------------------------------*/
VOID StartIniMapping(VOID)
{
    HKEY  hKey1,hKey2,hKey3,hKeySnd;
    DWORD dwDisp,dwSize;


    if(RegCreateKeyEx(HKEY_LOCAL_MACHINE,szIniSection,0,NULL,REG_OPTION_NON_VOLATILE,KEY_WRITE,NULL,&hKey1,&dwDisp) == ERROR_SUCCESS)
    {
        if(dwDisp == REG_CREATED_NEW_KEY)
        {
            RegSetValueEx(hKey1,TEXT("Preferences"),0,REG_SZ,(LPBYTE)szIniKey1,ByteCountOf(lstrlen(szIniKey1)+1));
            RegSetValueEx(hKey1,TEXT("Font")       ,0,REG_SZ,(LPBYTE)szIniKey2,ByteCountOf(lstrlen(szIniKey2)+1));
        }

        if(RegCreateKeyEx(HKEY_CURRENT_USER,TEXT("Software\\Microsoft\\Winchat"),0,NULL,REG_OPTION_NON_VOLATILE,KEY_WRITE,NULL,&hKey1,&dwDisp) == ERROR_SUCCESS)
        {
            if(dwDisp == REG_CREATED_NEW_KEY)
            {
                RegCreateKeyEx(HKEY_CURRENT_USER,TEXT("Software\\Microsoft\\Winchat\\Preferences"),0,NULL,REG_OPTION_NON_VOLATILE,KEY_WRITE,NULL,&hKey2,&dwDisp);
                RegCreateKeyEx(HKEY_CURRENT_USER,TEXT("Software\\Microsoft\\Winchat\\Font")       ,0,NULL,REG_OPTION_NON_VOLATILE,KEY_WRITE,NULL,&hKey3,&dwDisp);

                RegCloseKey(hKey2);
                RegCloseKey(hKey3);
            }
        }

        RegCloseKey(hKey1);
    }


    // The sndPlaySound() first looks in the registry for the wav-files.  The
    // NT version doesn't have these here at setup, so Winchat will write out
    // the defaults when the strings don't exist.  This will allow uses to change
    // sounds for ringing-in and ringing-out.
    //
    if(RegOpenKeyEx(HKEY_CURRENT_USER,TEXT("Control Panel\\Sounds"),0,KEY_WRITE | KEY_QUERY_VALUE,&hKeySnd) == ERROR_SUCCESS)
    {
        dwSize = 0;
        dwDisp = REG_SZ;
        if(RegQueryValueEx(hKeySnd,TEXT("RingIn"),NULL,&dwDisp,NULL,&dwSize) != ERROR_SUCCESS)
        {
            if(dwSize == 0)
            {
                // Set the wav-file values.  Add (1) extra count to account for the null
                // terminator.
                //
                RegSetValueEx(hKeySnd,TEXT("RingIn") ,0,REG_SZ,(LPBYTE)szIniRingIn ,ByteCountOf(lstrlen(szIniRingIn)+1));
                RegSetValueEx(hKeySnd,TEXT("RingOut"),0,REG_SZ,(LPBYTE)szIniRingOut,ByteCountOf(lstrlen(szIniRingOut)+1));
            }
        }

        RegCloseKey(hKeySnd);
    }

    return;
}



/*---------------------------------------------------------------------------*\
| END INI-FILE MAPPING
|   This routines ends the ini-file mapping.  It doesn't do anything at this
|   point, but I've kept it in for some stupid reason.
|
|
\*---------------------------------------------------------------------------*/
VOID EndIniMapping(VOID)
{
    return;
}
