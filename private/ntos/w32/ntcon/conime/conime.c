// Copyright (c) 1985 - 1999, Microsoft Corporation
//
//  MODULE:   ConIme.c
//
//  PURPOSE:   Console IME control.
//
//  PLATFORMS: Windows NT-J 3.51
//
//  FUNCTIONS:
//    WinMain() - calls initialization functions, processes message loop
//    WndProc - Processes messages for the main window.
//
//  History:
//
//  27.Jul.1995 v-HirShi (Hirotoshi Shimizu)    created
//
//  COMMENTS:
//

#include "precomp.h"
#pragma hdrstop


// Global Variables

HANDLE          LastConsole;
HIMC            ghDefaultIMC;

PCONSOLE_TABLE  *ConsoleTable;
ULONG           NumberOfConsoleTable;

CRITICAL_SECTION ConsoleTableLock; // serialize console table access
#define LockConsoleTable()   RtlEnterCriticalSection(&ConsoleTableLock)
#define UnlockConsoleTable() RtlLeaveCriticalSection(&ConsoleTableLock)


BOOL            gfDoNotKillFocus;


DWORD           dwConsoleThreadId;


#if DBG
ULONG InputExceptionFilter(
    PEXCEPTION_POINTERS pexi)
{
    if (pexi->ExceptionRecord->ExceptionCode != STATUS_PORT_DISCONNECTED) {
        DbgPrint("CONIME: Unexpected exception - %x, pexi = %x\n",
                pexi->ExceptionRecord->ExceptionCode, pexi);
        DbgBreakPoint();
    }

    return EXCEPTION_EXECUTE_HANDLER;
}
#else
#define InputExceptionFilter(pexi) EXCEPTION_EXECUTE_HANDLER
#endif


int
APIENTRY
WinMain(
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPSTR     lpCmdLine,
    int       nCmdShow
    )
{
    MSG msg;
    WCHAR systemPath[MAX_PATH];

    GetSystemDirectory ( systemPath, MAX_PATH );
    SetCurrentDirectory ( systemPath );

    if (!InitConsoleIME(hInstance) ) {
        DBGPRINT(("CONIME: InitConsoleIME failure!\n"));
        return FALSE;
    }
    else {
        DBGPRINT(("CONIME: InitConsoleIME successful!\n"));
    }

    try {

        while (TRUE)  {
            if (GetMessage(&msg, NULL, 0, 0)) {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            } else {
                break;
            }
        }

    } except (InputExceptionFilter(GetExceptionInformation())) {

        if (dwConsoleThreadId)
        {
            DBGPRINT(("CONIME: Exception on WinMain!!\n"));
            UnregisterConsoleIME();
            dwConsoleThreadId = 0;
        }

    }

    return (int)msg.wParam;
}

BOOL
InitConsoleIME(
    HINSTANCE hInstance
    )
{
    HANDLE   hEvent;
    ATOM     atom;
    HWND     hWnd;
    WNDCLASS ConsoleIMEClass;
    int      cxExecStart;
    int      cyExecStart;
    WCHAR    szMenuName[16];                 // The name of Menu
    WCHAR    szClassName[16];                // The class name of this application
    WCHAR    szTitle[16];                    // The title bar text

#ifdef DEBUG_MODE
    WCHAR    szAppName[16];                  // The name of this application

    LoadString(hInstance, IDS_TITLE,     szTitle,     sizeof(szTitle));
#else
    szTitle[0] = L'\0';
#endif

    DBGPRINT(("CONIME: Enter InitConsoleIMEl!\n"));

    RtlInitializeCriticalSection(&ConsoleTableLock);

    ConsoleTable = (PCONSOLE_TABLE *)LocalAlloc(LPTR, CONSOLE_INITIAL_TABLE * sizeof(PCONSOLE_TABLE));
    if (ConsoleTable == NULL) {
        return FALSE;
    }
    RtlZeroMemory(ConsoleTable, CONSOLE_INITIAL_TABLE * sizeof(PCONSOLE_TABLE));
    NumberOfConsoleTable = CONSOLE_INITIAL_TABLE;

    // Load the application name and description strings.
    LoadString(hInstance, IDS_MENUNAME,  szMenuName,  sizeof(szMenuName));
    LoadString(hInstance, IDS_CLASSNAME, szClassName, sizeof(szClassName));

    hEvent = OpenEvent(EVENT_MODIFY_STATE,    // Access flag
                       FALSE,                 // Inherit
                       CONSOLEIME_EVENT);     // Event object name
    if (hEvent == NULL)
    {
        DBGPRINT(("CONIME: OpenEvent failure! %d\n",GetLastError()));
        goto ErrorExit;
    }

    // Fill in window class structure with parameters that describe the
    // main window.

    ConsoleIMEClass.style         = 0;                       // Class style(s).
    ConsoleIMEClass.lpfnWndProc   = WndProc;                 // Window Procedure
    ConsoleIMEClass.cbClsExtra    = 0;                       // No per-class extra data.
    ConsoleIMEClass.cbWndExtra    = 0;                       // No per-window extra data.
    ConsoleIMEClass.hInstance     = hInstance;               // Owner of this class
    ConsoleIMEClass.hIcon         = LoadIcon(hInstance, MAKEINTRESOURCE(ID_CONSOLEIME_ICON));
    ConsoleIMEClass.hCursor       = LoadCursor(NULL, IDC_ARROW); // Cursor
    ConsoleIMEClass.hbrBackground = GetStockObject(WHITE_BRUSH); // Default color
    ConsoleIMEClass.lpszMenuName  = szMenuName;              // Menu name from .RC
    ConsoleIMEClass.lpszClassName = szClassName;             // Class Name

    // Register the window class and return FALSE if unsuccesful.

    atom = RegisterClass(&ConsoleIMEClass);
    if (atom == 0)
    {
        DBGPRINT(("CONIME: RegisterClass failure! %d\n",GetLastError()));
        goto ErrorExit;
    }
    else {
        DBGPRINT(("CONIME: RegisterClass Successful!\n"));
    }

    // Guess size for now.
    cxExecStart = GetSystemMetrics(SM_CXSCREEN);
    cyExecStart = GetSystemMetrics(SM_CYMENU);

    // Create a main window for this application instance.
    hWnd = CreateWindow(szClassName,                 // See RegisterClass() call.
                        szTitle,                     // Text for window title bar.
                        WS_OVERLAPPEDWINDOW,
                        cxExecStart - (cxExecStart / 3) ,
                        cyExecStart ,
                        cxExecStart / 3 ,
                        cyExecStart * 10 ,
                        NULL,                        // Overlapped has no parent.
                        NULL,                        // Use the window class menu.
                        hInstance,
                        (LPVOID)NULL);

    // If window could not be created, return "failure"
    if (hWnd == NULL) {
        DBGPRINT(("CONIME: CreateWindow failured! %d\n",GetLastError()));
        goto ErrorExit;
    }
    else{
        DBGPRINT(("CONIME: CreateWindow Successful!\n"));
    }

    if (! RegisterConsoleIME(hWnd, &dwConsoleThreadId))
    {
        DBGPRINT(("CONIME: RegisterConsoleIME failured! %d\n",GetLastError()));
        goto ErrorExit;
    }

    if (! AttachThreadInput(GetCurrentThreadId(), dwConsoleThreadId, TRUE))
    {
        DBGPRINT(("CONIME: AttachThreadInput failured! %d\n",GetLastError()));
        goto ErrorExit;
    }

    /*
     * dwConsoleThreadId is locked until event sets of hEvent
     */
    SetEvent(hEvent);
    CloseHandle(hEvent);

#ifdef DEBUG_MODE
    LoadString(hInstance, IDS_APPNAME,   szAppName,   sizeof(szAppName));

    // Make the window visible; update its client area; and return "success"
    ShowWindow(hWnd, SW_MINIMIZE); // Show the window
    SetWindowText(hWnd, szAppName);
    UpdateWindow(hWnd);         // Sends WM_PAINT message

    {
        int i;

        for (i = 0; i < CVMAX; i++) {
            ConvertLine[i] = UNICODE_SPACE;
            ConvertLineAtr[i] = 0;
        }
        xPos = 0;
        xPosLast = 0;
    }

#endif

    return TRUE;                // We succeeded...

ErrorExit:
    if (dwConsoleThreadId)
        UnregisterConsoleIME();
    if (hWnd)
        DestroyWindow(hWnd);
    if (atom)
        UnregisterClass(szClassName,hInstance);
    if (hEvent)
    {
        SetEvent(hEvent);
        CloseHandle(hEvent);
    }
    return FALSE;
}


//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE:  Processes messages
//
//  PARAMETERS:
//    hwnd     - window handle
//    uMessage - message number
//    wparam   - additional information (dependant of message number)
//    lparam   - additional information (dependant of message number)
//
//  MESSAGES:
//
//    WM_COMMAND    - exit command
//    WM_DESTROY    - destroy window
//
//  RETURN VALUE:
//
//    Depends on the message number.
//
//  COMMENTS:
//
//

LRESULT FAR PASCAL WndProc( HWND hWnd,
                         UINT Message,
                         WPARAM wParam,
                         LPARAM lParam)
{
    DWORD dwImmRet = 0;        // return value of ImmSrvProcessKey()

    try {

        switch (Message)
        {
            case CONIME_CREATE:
                DBGPRINT(("CONIME: CONIME_CREATE: Console Handle=%08x\n", wParam));
                return InsertNewConsole(hWnd,(HANDLE)wParam,(HWND)lParam);

            case CONIME_DESTROY:
                DBGPRINT(("CONIME: CONIME_DESTROY: Console Handle=%08x\n", wParam));
                return RemoveConsole(hWnd, (HANDLE)wParam);

            case CONIME_SETFOCUS:
                DBGPRINT(("CONIME: CONIME_SETFOCUS: Console Handle=%08x\n", wParam));
                return ConsoleSetFocus(hWnd, (HANDLE)wParam, (HKL)lParam);

            case CONIME_KILLFOCUS:
                DBGPRINT(("CONIME: CONIME_KILLFOCUS: Console Handle=%08x\n", wParam));
                return ConsoleKillFocus(hWnd, (HANDLE)wParam);

            case CONIME_GET_NLSMODE:
                DBGPRINT(("CONIME: CONIME_GET_NLSMODE: Console Handle=%08x\n", wParam));
                return GetNLSMode(hWnd, (HANDLE)wParam);

            case CONIME_SET_NLSMODE:
                DBGPRINT(("CONIME: CONIME_SET_NLSMODE: Console Handle=%08x\n", wParam));
                return SetNLSMode(hWnd, (HANDLE)wParam, (DWORD)lParam);

            case CONIME_HOTKEY:
                DBGPRINT(("CONIME: CONIME_HOTKEY\n"));
                return ConimeHotkey(hWnd, (HANDLE)wParam, (DWORD)lParam);

            case CONIME_NOTIFY_VK_KANA:
                DBGPRINT(("CONIME: CONIME_NOTIFY_VK_KANA\n"));
                return ImeUISetConversionMode(hWnd);

            case CONIME_NOTIFY_SCREENBUFFERSIZE: {
                COORD ScreenBufferSize;
                DBGPRINT(("CONIME: CONIME_NOTIFY_SCREENBUFFERSIZE: Console Handle=%08x\n", wParam));
                ScreenBufferSize.X = LOWORD(lParam);
                ScreenBufferSize.Y = HIWORD(lParam);
                return ConsoleScreenBufferSize(hWnd, (HANDLE)wParam, ScreenBufferSize);
            }

            case CONIME_INPUTLANGCHANGE: {
                DBGPRINT(("CONIME: CONIME_INPUTLANGCHANGE: Console Handle=%08x \n",wParam));
                ConImeInputLangchange(hWnd, (HANDLE)wParam, (HKL)lParam );
                return TRUE;
            }

            case CONIME_NOTIFY_CODEPAGE: {
                BOOL Output;
                WORD Codepage;

                Codepage = HIWORD(lParam);
                Output = LOWORD(lParam);
                DBGPRINT(("CONIME: CONIME_NOTIFY_CODEPAGE: Console Handle=%08x %04x %04x\n",wParam, Output, Codepage));
                return ConsoleCodepageChange(hWnd, (HANDLE)wParam, Output, Codepage);
            }

            case WM_KEYDOWN    +CONIME_KEYDATA:
            case WM_KEYUP      +CONIME_KEYDATA:
            case WM_SYSKEYDOWN +CONIME_KEYDATA:
            case WM_SYSKEYUP   +CONIME_KEYDATA:
            case WM_DEADCHAR   +CONIME_KEYDATA:
            case WM_SYSDEADCHAR+CONIME_KEYDATA:
            case WM_SYSCHAR    +CONIME_KEYDATA:
            case WM_CHAR       +CONIME_KEYDATA:
                CharHandlerFromConsole( hWnd, Message, (ULONG)wParam, (ULONG)lParam );
                break;
            case WM_KEYDOWN:
            case WM_KEYUP:
            case WM_SYSKEYDOWN:
            case WM_SYSKEYUP:
            case WM_DEADCHAR:
            case WM_SYSDEADCHAR:
            case WM_SYSCHAR:
            case WM_CHAR:
                CharHandlerToConsole( hWnd, Message, (ULONG)wParam, (ULONG)lParam );
                break;

            case WM_INPUTLANGCHANGE:
                DBGPRINT(("CONIME: CONIME_INPUTLANGCHANGE: Console Handle=%08x \n",wParam));
                InputLangchange(hWnd, (DWORD)wParam, (HKL)lParam );
                return TRUE;

            case WM_INPUTLANGCHANGEREQUEST:
                // Console IME never receive this message for this window is hidden
                // and doesn't have focus.
                //
                // However, Hot key of IME_CHOTKEY_IME_NONIME_TOGGLE/IME_THOTKEY_IME_NONIME_TOGGLE
                // are send this message by ImmSimulateHotKey API.
                //
                // If nothing processing by this message, then DefWindowProc calls
                // ActivateKeyboardLayout on kernel side.
                // And, ActivateKeyboardLayout send WM_INPUTLANGCHANGE message to focus window
                // on this message queue.
                // It window is console window procedure.
                // Console window procedure can do send CONIME_INPUTLANGCHANGE message to
                // console IME window.
                // In console window is windowed case, this sequence as well.
                // But, In console window is full screen case, message queue have not focus.
                // WM_INPUTLANGCHANGE message can not send to console window procedure.
                //
                // This code avoid console full screen mode problem.
                // Send message to console window procedure when this window receive it.
                //
                {
                    PCONSOLE_TABLE ConTbl;

                    ConTbl = SearchConsole(LastConsole);
                    if (ConTbl == NULL) {
                        return DefWindowProc(hWnd, Message, wParam, lParam);
                    }

                    PostMessage(ConTbl->hWndCon, Message, wParam, lParam);
                }
                return TRUE;    // TRUE : process this message by application

            case CONIME_INPUTLANGCHANGEREQUEST:
                DBGPRINT(("CONIME: CONIME_INPUTLANGCHANGEREQUEST: Console Handle=%08x \n",wParam));
                return ConImeInputLangchangeRequest(hWnd, (HANDLE)wParam, (HKL)lParam, CONIME_DIRECT);

            case CONIME_INPUTLANGCHANGEREQUESTFORWARD:
                DBGPRINT(("CONIME: CONIME_INPUTLANGCHANGEREQUEST: Console Handle=%08x \n",wParam));
                return ConImeInputLangchangeRequest(hWnd, (HANDLE)wParam, (HKL)lParam, CONIME_FORWARD);

            case CONIME_INPUTLANGCHANGEREQUESTBACKWARD:
                DBGPRINT(("CONIME: CONIME_INPUTLANGCHANGEREQUEST: Console Handle=%08x \n",wParam));
                return ConImeInputLangchangeRequest(hWnd, (HANDLE)wParam, (HKL)lParam, CONIME_BACKWARD);

#ifdef DEBUG_MODE
            case WM_MOVE:
                ImeUIMoveCandWin( hWnd );
                break;

            case WM_COMMAND: // message: command from application menu

                // Message packing of wparam and lparam have changed for Win32,
                // so use the GET_WM_COMMAND macro to unpack the commnad

                switch (LOWORD(wParam)) {
                    case MM_EXIT:
                        PostMessage(hWnd,WM_CLOSE,0,0L);
                        break;

                    case MM_ACCESS_VIOLATION:
                        {
                            PBYTE p = 0;
                            *p = 0;
                        }
                        break;
                }
                break;
#endif

            case WM_IME_STARTCOMPOSITION:
                ImeUIStartComposition( hWnd );
                break;
            case WM_IME_ENDCOMPOSITION:
                ImeUIEndComposition( hWnd );
                break;
            case WM_IME_COMPOSITION:
                ImeUIComposition( hWnd, wParam, lParam );
                break;
            case WM_IME_COMPOSITIONFULL:
                break;
            case WM_IME_NOTIFY:
                if ( !ImeUINotify( hWnd, wParam, lParam ) ) {
                    return DefWindowProc(hWnd, Message, wParam, lParam);
                }
                break;
            case WM_IME_SETCONTEXT:
                //
                // The application have to pass WM_IME_SETCONTEXT to DefWindowProc.
                // When the application want to handle the IME at the timing of
                // focus changing, the application should use WM_GETFOCUS or
                // WM_KILLFOCUS.
                //
                lParam &= ~ISC_SHOWUIALL;

                return DefWindowProc( hWnd, Message, wParam, lParam );
            case WM_IME_SYSTEM:
                switch (wParam) {
                    case IMS_CLOSEPROPERTYWINDOW:
                    case IMS_OPENPROPERTYWINDOW:
                        ImeSysPropertyWindow(hWnd, wParam, lParam);
                        break;
                    default:
                        return DefWindowProc( hWnd, Message, wParam, lParam );
                }
                break;

            case WM_CREATE:
                return Create(hWnd);
                break;

            case WM_DESTROY:
                DBGPRINT(("CONIME:Recieve WM_DESTROY\n"));
                ExitList(hWnd);
                PostQuitMessage(0);
                return 0;
                break;

            case WM_CLOSE:
                DBGPRINT(("CONIME:Recieve WM_CLOSE\n"));
                DestroyWindow(hWnd);
                return 0;
                break;

            case WM_ENABLE:{
                PCONSOLE_TABLE FocusedConsole;
                if (!wParam) {
                    FocusedConsole = SearchConsole(LastConsole);
                    if (FocusedConsole != NULL &&
                        FocusedConsole->hConsole != NULL) {
                        FocusedConsole->Enable = FALSE;
                        EnableWindow(FocusedConsole->hWndCon,FALSE);
                        gfDoNotKillFocus = TRUE;
                    }
                }
                else{
                    DWORD i;
                    LockConsoleTable();
                    for ( i = 1; i < NumberOfConsoleTable; i ++){
                        FocusedConsole = ConsoleTable[i];
                        if (FocusedConsole != NULL)
                        {
                            if ((FocusedConsole->hConsole != NULL)&&
                                (!FocusedConsole->Enable)&&
                                (!IsWindowEnabled(FocusedConsole->hWndCon))){
                                EnableWindow(FocusedConsole->hWndCon,TRUE);
                                FocusedConsole->Enable = TRUE;
                                if (!FocusedConsole->LateRemove)
                                    SetForegroundWindow(FocusedConsole->hWndCon);
                            }
                        }
                    }
                    UnlockConsoleTable();
                }
                return DefWindowProc(hWnd, Message, wParam, lParam);
                break;
            }

#ifdef DEBUG_MODE
            case WM_SETFOCUS:
                CreateCaret( hWnd,
                             NULL,
                             IsUnicodeFullWidth( ConvertLine[xPos] ) ?
                             CaretWidth*2 : CaretWidth,
                             (UINT)cyMetrics );
                SetCaretPos( xPos * cxMetrics, 0 );
                ShowCaret( hWnd );
                break;

            case WM_KILLFOCUS:
                HideCaret( hWnd );
                DestroyCaret();
                break;

            case WM_PAINT:
                {
                    PAINTSTRUCT pstruc;
                    HDC  hDC;
                    hDC = BeginPaint(hWnd,&pstruc);
                    ReDraw(hWnd);
                    EndPaint(hWnd,&pstruc);
                    break;
                }
#endif

            case WM_QUERYENDSESSION:
#ifdef HIRSHI_DEBUG
                /*
                 * If specified ntsd debugger on this process,
                 * then never catch WM_QUERYENDSESSION when logoff/shutdown because
                 * this process will terminate when ntsd process terminated.
                 */
                {
                    int i;
                    i = MessageBox(hWnd,TEXT("Could you approve exit session?"), TEXT("Console IME"),
                                   MB_ICONSTOP | MB_YESNO);
                    return (i == IDYES ? TRUE : FALSE);
                }
#endif
                return TRUE;           // Logoff or shutdown time.

            case WM_ENDSESSION:
                DBGPRINT(("CONIME:Recieve WM_ENDSESSION\n"));
                ExitList(hWnd);
                return 0;

            default:          // Passes it on if unproccessed
                return DefWindowProc(hWnd, Message, wParam, lParam);
        }

    } except (InputExceptionFilter(GetExceptionInformation())) {

        if (dwConsoleThreadId)
        {
            DBGPRINT(("CONIME: Exception on WndProc!!\n"));
            UnregisterConsoleIME();
            dwConsoleThreadId = 0;

            DestroyWindow(hWnd);
            return 0;
        }

    }


    return TRUE;
}


VOID
ExitList(
    HWND hWnd
    )
{
    ULONG i ,j;
    PCONSOLE_TABLE FocusedConsole;

    DBGPRINT(("CONIME:ExitList Processing\n"));
    ImmAssociateContext(hWnd,ghDefaultIMC);

    LockConsoleTable();

    for (i = 1; i < NumberOfConsoleTable; i++) {
        FocusedConsole = ConsoleTable[i];
        if (FocusedConsole != NULL)
        {
            if (FocusedConsole->hConsole != NULL) {
                if (FocusedConsole->Enable) {
                    ImmDestroyContext(FocusedConsole->hIMC_Original);
                    if ( FocusedConsole->lpCompStrMem != NULL) {
                        LocalFree( FocusedConsole->lpCompStrMem );
                        FocusedConsole->lpCompStrMem = NULL;
                    }
                    for (j = 0; j < MAX_LISTCAND; j++){
                        if (FocusedConsole->lpCandListMem[j] != NULL) {
                            LocalFree(FocusedConsole->lpCandListMem[j]);
                            FocusedConsole->lpCandListMem[j] = NULL;
                            FocusedConsole->CandListMemAllocSize[j] = 0;
                        }
                    }
                    if (FocusedConsole->CandSep != NULL) {
                        LocalFree(FocusedConsole->CandSep);
                        FocusedConsole->CandSepAllocSize = 0;
                    }
                    FocusedConsole->Enable = FALSE;
                }
                else
                    FocusedConsole->LateRemove = TRUE;
            }
        }
    }
    LocalFree( ConsoleTable );
    UnlockConsoleTable();

    if (dwConsoleThreadId)
    {
        AttachThreadInput(GetCurrentThreadId(), dwConsoleThreadId, FALSE);
        UnregisterConsoleIME();
        dwConsoleThreadId = 0;
    }
}

BOOL
InsertConsole(
    HWND    hWnd,
    HANDLE  hConsole,
    HWND    hWndConsole
    )
{
    ULONG i;
    PCONSOLE_TABLE FocusedConsole;

    i = 1;

    do {
        for (; i < NumberOfConsoleTable; i++) {
            FocusedConsole = ConsoleTable[i];

            if (FocusedConsole == NULL)
            {
                FocusedConsole = LocalAlloc(LPTR, sizeof(CONSOLE_TABLE));
                if (FocusedConsole == NULL)
                    return FALSE;
                ConsoleTable[i] = FocusedConsole;
            }

            if ((FocusedConsole->hConsole != NULL) &&
                (FocusedConsole->LateRemove)&&
                (FocusedConsole->Enable)) {
                RemoveConsoleWorker(hWnd, FocusedConsole);
            }

            if (FocusedConsole->hConsole == NULL) {
                RtlZeroMemory(FocusedConsole, sizeof(CONSOLE_TABLE));
                FocusedConsole->lphklList = LocalAlloc(LPTR, sizeof(HKL_TABLE)*HKL_INITIAL_TABLE);
                if (FocusedConsole->lphklList == NULL)
                {
                    return FALSE;
                }
                RtlZeroMemory(FocusedConsole->lphklList, sizeof(HKL_TABLE)*HKL_INITIAL_TABLE);
                FocusedConsole->hklListMax = HKL_INITIAL_TABLE ;

                FocusedConsole->hIMC_Current = ImmCreateContext();
                if (FocusedConsole->hIMC_Current == (HIMC)NULL) {
                    LocalFree(FocusedConsole);
                    FocusedConsole = NULL;
                    return FALSE;
                }

                FocusedConsole->hIMC_Original = FocusedConsole->hIMC_Current;
                FocusedConsole->hConsole      = hConsole;
                FocusedConsole->hWndCon       = hWndConsole;
//                FocusedConsole->hklActive     = NULL;
                FocusedConsole->Enable        = TRUE;
//                FocusedConsole->LateRemove    = FALSE;
//                FocusedConsole->fNestCandidate = FALSE;
//                FocusedConsole->fInComposition = FALSE;
//                FocusedConsole->fInCandidate = FALSE;
                FocusedConsole->ScreenBufferSize.X = DEFAULT_TEMP_WIDTH;

                FocusedConsole->CompAttrColor[0] = DEFAULT_COMP_ENTERED;
                FocusedConsole->CompAttrColor[1] = DEFAULT_COMP_ALREADY_CONVERTED;
                FocusedConsole->CompAttrColor[2] = DEFAULT_COMP_CONVERSION;
                FocusedConsole->CompAttrColor[3] = DEFAULT_COMP_YET_CONVERTED;
                FocusedConsole->CompAttrColor[4] = DEFAULT_COMP_INPUT_ERROR;
                FocusedConsole->CompAttrColor[5] = DEFAULT_COMP_INPUT_ERROR;
                FocusedConsole->CompAttrColor[6] = DEFAULT_COMP_INPUT_ERROR;
                FocusedConsole->CompAttrColor[7] = DEFAULT_COMP_INPUT_ERROR;

                GetIMEName(FocusedConsole);

                return TRUE;
            }
        }
    } while (GrowConsoleTable());

    DBGPRINT(("CONIME: Cannot grow Console Table\n"));
    return FALSE;
}

BOOL
GrowConsoleTable(
    VOID
    )
{
    PCONSOLE_TABLE *NewTable;
    PCONSOLE_TABLE *OldTable;
    ULONG MaxConsoleTable;

    MaxConsoleTable = NumberOfConsoleTable + CONSOLE_CONSOLE_TABLE_INCREMENT;
    NewTable = (PCONSOLE_TABLE *)LocalAlloc(LPTR, MaxConsoleTable * sizeof(PCONSOLE_TABLE));
    if (NewTable == NULL) {
        return FALSE;
    }
    CopyMemory(NewTable, ConsoleTable, NumberOfConsoleTable * sizeof(PCONSOLE_TABLE));

    OldTable = ConsoleTable;
    ConsoleTable = NewTable;
    NumberOfConsoleTable = MaxConsoleTable;

    LocalFree(OldTable);

    return TRUE;
}

PCONSOLE_TABLE
SearchConsole(
    HANDLE hConsole
    )
{
    ULONG i;
    PCONSOLE_TABLE FocusedConsole;

    LockConsoleTable();

    // conime receive ime message from console before 1st console registered.
    // this will happen after restart conime when conime dead by bogus ime's AV or 
    // other problem
    // so this fail safe code is  necessary to protect consrv.
    if (LastConsole == 0) {
        LastConsole = hConsole ;
    }

    for (i = 1; i < NumberOfConsoleTable; i++) {
        FocusedConsole = ConsoleTable[i];
        if (FocusedConsole != NULL)
        {
            if ((FocusedConsole->hConsole == hConsole)&&
                (!FocusedConsole->LateRemove)) {

                UnlockConsoleTable();
                return FocusedConsole;
            }
        }
    }
    UnlockConsoleTable();
    return NULL;
}

BOOL
RemoveConsole(
    HWND hwnd,
    HANDLE hConsole
    )
{
    PCONSOLE_TABLE ConTbl;
    BOOL ret;

    LockConsoleTable();

    ConTbl = SearchConsole(hConsole);
    if (ConTbl == NULL)
    {
        UnlockConsoleTable();
        return FALSE;
    }
    ret = RemoveConsoleWorker(hwnd, ConTbl);

    UnlockConsoleTable();
    return ret;
}

BOOL
RemoveConsoleWorker(
    HWND hwnd,
    PCONSOLE_TABLE ConTbl
    )
{
    DWORD j;

    if (ConTbl->Enable) {
        ConTbl->hConsole = NULL;
        ConTbl->ScreenBufferSize.X = 0;
        ConTbl->ConsoleCP = 0;
        ConTbl->ConsoleOutputCP = 0;
        ConTbl->hklActive = 0;

        ImmDestroyContext(ConTbl->hIMC_Original);

        if (ConTbl->lpCompStrMem != NULL){
            LocalFree(ConTbl->lpCompStrMem);
        }
        for (j = 0; j < MAX_LISTCAND; j++){
            if (ConTbl->lpCandListMem[j] != NULL) {
                LocalFree(ConTbl->lpCandListMem[j]);
            }
        }
        if (ConTbl->CandSep != NULL) {
            LocalFree(ConTbl->CandSep);
        }

        if (ConTbl->lphklList != NULL) {
            LocalFree(ConTbl->lphklList) ;
        }

        ConTbl->Enable     = FALSE;
        ConTbl->LateRemove = FALSE;
    }
    else
        ConTbl->LateRemove = TRUE;

#ifdef DEBUG_MODE
    InvalidateRect(hwnd,NULL,TRUE);
#endif
    return TRUE;
}

BOOL
InsertNewConsole(
    HWND   hWnd,
    HANDLE hConsole,
    HWND   hWndConsole
    )
{
    // conime receive ime message from console before 1st console registered.
    // this will happen after restart conime when conime dead by bogus ime's AV or 
    // other problem
    // so this fail safe code is  necessary to protect consrv.
    if (SearchConsole(hConsole) != NULL) {
        return TRUE;
    }

    LockConsoleTable();

    if (!InsertConsole(hWnd, hConsole, hWndConsole)) {
        UnlockConsoleTable();
        return FALSE;
    }

#ifdef DEBUG_MODE
    DisplayInformation(hWnd, hConsole);
#endif

    ImeUISetOpenStatus( hWndConsole );

    UnlockConsoleTable();

    return TRUE;
}


BOOL
ConsoleSetFocus(
    HWND hWnd,
    HANDLE hConsole,
    HKL hKL
    )
{
    PCONSOLE_TABLE ConTbl;
    HKL OldhKL;

    ConTbl = SearchConsole(hConsole);
    if (ConTbl == NULL) {
        DBGPRINT(("CONIME: Error! Cannot found registed Console\n"));
        return FALSE;
    }

    if ( gfDoNotKillFocus ){
        gfDoNotKillFocus = FALSE;
    }

    OldhKL = ConTbl->hklActive ;
    ConTbl->hklActive = hKL;
    ActivateKeyboardLayout(ConTbl->hklActive, 0);
    ImmAssociateContext(hWnd, ConTbl->hIMC_Current);

    if (OldhKL == 0) {
        GetIMEName( ConTbl );
        ConTbl->ImmGetProperty = ImmGetProperty(ConTbl->hklActive , IGP_PROPERTY);
    }

//v-hirshi Jun.13.1996 #if defined(LATER_DBCS)  // kazum
    ImmSetActiveContextConsoleIME(hWnd, TRUE);
//v-hirshi Jun.13.1996 #endif

    LastConsole = hConsole;

#ifdef DEBUG_MODE
    DisplayInformation(hWnd, hConsole);
#endif

    ImeUISetOpenStatus( hWnd );
    if (ConTbl->lpCompStrMem != NULL)
        ReDisplayCompositionStr( hWnd );

    return TRUE;
}

BOOL
ConsoleKillFocus(
    HWND hWnd,
    HANDLE hConsole
    )
{
    PCONSOLE_TABLE ConTbl;

    ConTbl = SearchConsole(hConsole);
    if (ConTbl == NULL) {
        DBGPRINT(("CONIME: Error! Cannot found registed Console\n"));
        return FALSE;
    }

    if ( gfDoNotKillFocus ){
        gfDoNotKillFocus = FALSE;
    }
    else{
//v-hirshi Jun.13.1996 #if defined(LATER_DBCS)  // kazum
        ImmSetActiveContextConsoleIME(hWnd, FALSE);
//v-hirshi Jun.13.1996 #endif
        ImmAssociateContext(hWnd, ghDefaultIMC);
    }

#ifdef DEBUG_MODE
    DisplayInformation(hWnd, hConsole);
#endif

    return TRUE;
}

BOOL
ConsoleScreenBufferSize(
    HWND hWnd,
    HANDLE hConsole,
    COORD ScreenBufferSize
    )
{
    PCONSOLE_TABLE ConTbl;

    ConTbl = SearchConsole(hConsole);
    if (ConTbl == NULL) {
        DBGPRINT(("CONIME: Error! Cannot found registed Console\n"));
        return FALSE;
    }

    ConTbl->ScreenBufferSize = ScreenBufferSize;
    return TRUE;
}

BOOL
ConImeInputLangchangeRequest(
    HWND hWnd,
    HANDLE hConsole,
    HKL hkl,
    int Direction
    )
{
    PCONSOLE_TABLE ConTbl;
    int nLayouts;
    LPHKL lphkl;
    DWORD RequiredLID = 0;
    int StartPos;
    int CurrentHklPos;
    int i;

    ConTbl = SearchConsole(hConsole);
    if (ConTbl == NULL) {
        DBGPRINT(("CONIME: cannot find registered Console\n"));
        return FALSE;
    }

    switch (ConTbl->ConsoleOutputCP) {
        case JAPAN_CODEPAGE:
            RequiredLID = LANG_ID_JAPAN;
            break;
        case PRC_CODEPAGE:
            RequiredLID = LANG_ID_PRC;
            break;
        case KOREA_CODEPAGE:
            RequiredLID = LANG_ID_KOREA;
            break;
        case TAIWAN_CODEPAGE:
            RequiredLID = LANG_ID_TAIWAN;
            break;
        default:
            break;
    }

    if ( !IS_IME_KBDLAYOUT(hkl) ||
        ( HKL_TO_LANGID(hkl) == RequiredLID)) {
        return TRUE;
    }
    if (Direction == CONIME_DIRECT) {
        return FALSE;
    }

    nLayouts = GetKeyboardLayoutList(0, NULL);
    if (nLayouts == 0) {
        return FALSE;
    }
    lphkl = LocalAlloc(LPTR, nLayouts * sizeof(HKL));
    if (lphkl == NULL) {
        return FALSE;
    }
    GetKeyboardLayoutList(nLayouts, lphkl);

    for (CurrentHklPos = 0; CurrentHklPos < nLayouts; CurrentHklPos++) {
        if (ConTbl->hklActive == lphkl[CurrentHklPos] ) {
            break;
        }
    }
    if (CurrentHklPos >= nLayouts) {
        LocalFree(lphkl);
        return FALSE;
    }

    StartPos = CurrentHklPos;

    for (i = 0; i < nLayouts; i++) {
        StartPos+=Direction;
        if (StartPos < 0) {
            StartPos = nLayouts-1;
        }
        else if (StartPos >= nLayouts) {
            StartPos = 0;
        }
        
        if ((( HandleToUlong(lphkl[StartPos]) & 0xf0000000) == 0x00000000) ||
            (( HandleToUlong(lphkl[StartPos]) & 0x0000ffff) == RequiredLID)) {
            PostMessage( ConTbl->hWndCon,
                         CM_CONIME_KL_ACTIVATE,
                          HandleToUlong(lphkl[StartPos]),
                         0);
            LocalFree(lphkl);
            return FALSE;
        }
    }

    LocalFree(lphkl);
    return FALSE;

}

BOOL
ConImeInputLangchange(
    HWND hWnd,
    HANDLE hConsole,
    HKL hkl
    )
{
    PCONSOLE_TABLE ConTbl;
    LPCONIME_UIMODEINFO lpModeInfo;
    COPYDATASTRUCT CopyData;
    INT counter ;
    LPHKL_TABLE lphklListNew ;

    ConTbl = SearchConsole(hConsole);
    if (ConTbl == NULL) {
        // cannot find specified console.
        // It might be last console lost focus.
        // try Last Console.
        ConTbl = SearchConsole(LastConsole);
        if (ConTbl == NULL) {
            DBGPRINT(("CONIME: Error! Cannot found registed Console\n"));
            return FALSE;
        }
    }

    if (ConTbl->lphklList == NULL) {
        return FALSE;
    }

    if (IS_IME_KBDLAYOUT(ConTbl->hklActive)) {
        for (counter = 0 ; counter < ConTbl->hklListMax ;counter ++) 
        {
            if (ConTbl->lphklList[counter].hkl == 0 || ConTbl->lphklList[counter].hkl == ConTbl->hklActive) {
                break;
            }
        }

        if (counter >= ConTbl->hklListMax)
        {
            ASSERT(counter == ConTbl->hklListMax);
            // reallocation
            lphklListNew = LocalAlloc(LPTR, sizeof(HKL_TABLE) * (ConTbl->hklListMax + HKL_TABLE_INCREMENT) ) ;
            if (lphklListNew != NULL)
            {
                CopyMemory(lphklListNew , ConTbl->lphklList , sizeof(HKL_TABLE) * ConTbl->hklListMax) ;
                ConTbl->hklListMax += HKL_TABLE_INCREMENT ;
                LocalFree(ConTbl->lphklList);
                ConTbl->lphklList = lphklListNew;
            }
            else {
                return FALSE ;
            }
        }
        ASSERT(ConTbl->lphklList != NULL);
        ConTbl->lphklList[counter].hkl = ConTbl->hklActive;
        ConTbl->lphklList[counter].dwConversion = ConTbl->dwConversion | (ConTbl->fOpen ? IME_CMODE_OPEN : 0)  ;
    }

    ActivateKeyboardLayout(hkl, 0);
    ConTbl->hklActive = hkl;
    GetIMEName( ConTbl );
    ImeUIOpenStatusWindow(hWnd);
    ConTbl->ImmGetProperty = ImmGetProperty(ConTbl->hklActive , IGP_PROPERTY);

    lpModeInfo = (LPCONIME_UIMODEINFO)LocalAlloc( LPTR, sizeof(CONIME_UIMODEINFO) ) ;
    if ( lpModeInfo == NULL) {
        return FALSE;
    }
    CopyData.dwData = CI_CONIMEMODEINFO ;
    CopyData.cbData = sizeof(CONIME_UIMODEINFO) ;
    CopyData.lpData = lpModeInfo ;

    if (IS_IME_KBDLAYOUT(hkl)) {

        for (counter=0; counter < ConTbl->hklListMax ; counter++)
        {
            if (ConTbl->lphklList[counter].hkl == hkl)
            {
                SetNLSMode(hWnd, hConsole,ConTbl->lphklList[counter].dwConversion ) ;
                ImeUIOpenStatusWindow(hWnd) ;
                if (ImeUIMakeInfoString(ConTbl,
                                        lpModeInfo))
                {
                    ConsoleImeSendMessage( ConTbl->hWndCon,
                                           (WPARAM)hWnd,
                                           (LPARAM)&CopyData
                                         ) ;
                }
            }
        }
    }
    else
    {

        SetNLSMode(hWnd, hConsole,ConTbl->dwConversion & ~IME_CMODE_OPEN ) ;
        lpModeInfo->ModeStringLen = 0 ;
        lpModeInfo->Position = VIEW_RIGHT ;
        ConsoleImeSendMessage( ConTbl->hWndCon,
                               (WPARAM)hWnd,
                               (LPARAM)&CopyData
                              ) ;
    }

    LocalFree( lpModeInfo );

    return TRUE;
}

BOOL
InputLangchange(
    HWND hWnd,
    DWORD CharSet,
    HKL hkl
    )
{
    PCONSOLE_TABLE ConTbl;

    ConTbl = SearchConsole(LastConsole);
    if (ConTbl == NULL) {
        DBGPRINT(("CONIME: Error! Cannot found registed Console\n"));
        return FALSE;
    }

    ConTbl->hklActive = hkl;
    ActivateKeyboardLayout(ConTbl->hklActive, 0);
    GetIMEName( ConTbl );
    ImeUIOpenStatusWindow(hWnd);
    return TRUE;
}

/*
 * Console IME message pump.
 */
LRESULT
ConsoleImeSendMessage(
    HWND   hWndConsoleIME,
    WPARAM wParam,
    LPARAM lParam
    )
{
    LRESULT lResult;
    LRESULT fNoTimeout;

    if (hWndConsoleIME == NULL)
    {
        return FALSE;
    }

    fNoTimeout = SendMessageTimeout(hWndConsoleIME,
                                    WM_COPYDATA,
                                    wParam,
                                    lParam,
                                    SMTO_ABORTIFHUNG | SMTO_NORMAL,
                                    CONIME_SENDMSG_TIMEOUT,
                                    &lResult);

    if (fNoTimeout)
    {
        return TRUE;
    }


    /*
     * ConsoleImeMessagePump give up SendMessage to conime.
     * CONIME is hung up.
     * probably, consrv also hung up.
     */
    KdPrint(("ConsoleImeSendMessage: CONIME_SENDMSG_COUNT is hung up\n"));

    return FALSE;

}

#ifdef DEBUG_MODE

int             cxMetrics;
int             cyMetrics;
int             cxOverTypeCaret;
int             xPos;
int             xPosLast;
int             CaretWidth;                     // insert/overtype mode caret width

WCHAR           ConvertLine[CVMAX];
unsigned char   ConvertLineAtr[CVMAX];

WCHAR           DispTitle[] = TEXT(" Console Handle");

DWORD CompColor[ 8 ] = { RGB(   0,   0,   0 ),  // ATTR_INPUT
                         RGB(   0,   0, 255 ),  // ATTR_TARGET_CONVERTED
                         RGB(   0, 255,   0 ),  // ATTR_CONVERTED
                         RGB( 255,   0,   0 ),  // ATTR_TARGET_NOTCONVERTED
                         RGB( 255,   0, 255 ),  // ATTR_INPUT_ERROR
                         RGB(   0, 255, 255 ),  // ATTR_DEFAULT
                         RGB( 255, 255,   0 ),  // ATTR_DEFAULT
                         RGB( 255, 255, 255 ) };// ATTR_DEFAULT

VOID
DisplayConvInformation(
    HWND hWnd
    )
{
    RECT      Rect;
    HDC       lhdc;

    lhdc = GetDC(hWnd);
    GetClientRect(hWnd, &Rect);

    InvalidateRect(hWnd,NULL,FALSE);
    UpdateWindow(hWnd);
    ReleaseDC(hWnd, lhdc);
}

VOID
DisplayInformation(
    HWND hWnd,
    HANDLE hConsole
    )
{
    PCONSOLE_TABLE ConTbl;
    RECT      Rect;
    HDC       lhdc;

    ConTbl = SearchConsole(hConsole);
    if (ConTbl == NULL) {
        DBGPRINT(("CONIME: Error! Cannot found registed Console\n"));
        return;
    }

    lhdc = GetDC(hWnd);
    GetClientRect(hWnd, &Rect);

    wsprintf(ConTbl->DispBuf, TEXT("%08x"), (ULONG)hConsole);

    InvalidateRect(hWnd,NULL,FALSE);
    UpdateWindow(hWnd);
    ReleaseDC(hWnd, lhdc);
}

VOID
RealReDraw(
    HDC r_hdc
    )
{
    PCONSOLE_TABLE ConTbl;
    INT     ix, iy, i, rx, sx;
    ULONG   cnt;
    int     ColorIndex;
    int     PrevColorIndex;
    DWORD   dwColor;

    iy = 0;

    dwColor = GetTextColor( r_hdc );

    ColorIndex = ( ((int)ConvertLineAtr[0]) < 0 ) ? 0 : (int)ConvertLineAtr[0];
    ColorIndex = ( ColorIndex > 7 ) ? 0 : ColorIndex;
    PrevColorIndex = ColorIndex;
    SetTextColor( r_hdc, CompColor[ ColorIndex ] );

    rx = 0;
    sx = 0;
    for (ix = 0; ix < MAXCOL; ) {
        for (i = ix; i < MAXCOL; i++) {
            if (PrevColorIndex != (int)ConvertLineAtr[i])
                break;
            rx += IsUnicodeFullWidth(ConvertLine[ix]) ? 2 : 1;
        }
        TextOut( r_hdc, sx * cxMetrics, iy, &ConvertLine[ix], i-ix );
        sx = rx;
        ColorIndex = ( ((int)ConvertLineAtr[i]) < 0 ) ? 0 : (int)ConvertLineAtr[i];
        ColorIndex = ( ColorIndex > 7 ) ? 0 : ColorIndex;
        PrevColorIndex = ColorIndex;
        SetTextColor( r_hdc, CompColor[ ColorIndex ] );
        ix = i;
    }

    ix = 0;
    SetTextColor( r_hdc, dwColor );
    iy += cyMetrics;
    TextOut( r_hdc, ix, iy, DispTitle, lstrlenW(DispTitle));

    iy += cyMetrics;

    LockConsoleTable();

    for (cnt = 1; cnt < NumberOfConsoleTable; cnt++, iy += cyMetrics){
        ConTbl = ConsoleTable[cnt];
        if (ConTbl != NULL)
        {
            if (ConTbl->hConsole)
            {
                TextOut( r_hdc, ix, iy, ConTbl->DispBuf, lstrlenW(ConTbl->DispBuf) );
            }
        }
    }

    UnlockConsoleTable();

    return;
}

VOID
ReDraw(
    HWND hWnd
    )
{
    HDC r_hdc;
    RECT ClientRect;

    GetClientRect(hWnd, &ClientRect);
    r_hdc = GetDC(hWnd);
    FillRect(r_hdc, &ClientRect, GetStockObject(WHITE_BRUSH));
    RealReDraw(r_hdc);
    ReleaseDC(hWnd, r_hdc);
    return;
}
#endif
