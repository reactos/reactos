/*++

Copyright (c) 1995 Intel Corp

Module Name:

    ws2_chat.c

Abstract:

    Contains WinMain and other user interface code for the WinSock2
    Chat sample application.

Author:

    Dan Chou & Michael Grafton

--*/


#include "nowarn.h"  /* turn off benign warnings */
#ifndef _WINSOCKAPI_
#define _WINSOCKAPI_   /* Prevent inclusion of winsock.h in windows.h */
#endif
#include <windows.h>
#include "nowarn.h"  /* some warnings may have been turned back on */
#include <winsock2.h>
#include <stdlib.h>
#include "ws2_chat.h"
#include "chatsock.h"
#include "chatdlg.h"
#include "queue.h"


//
// Forward References -- internal functions.
//

// Startup/Initialization Functions
int PASCAL
WinMain(
    IN HINSTANCE InstanceHandle,
    IN HINSTANCE PrevInstanceHandle,
    IN LPSTR  CmdLine,
    IN int    CmdShow);

BOOL
InitUI(
    IN HINSTANCE InstanceHandle,
    IN HINSTANCE PrevInstanceHandle);

long CALLBACK
MainWndProc(
    IN HWND   WindowHandle,
    IN UINT   Message,
    IN WPARAM WParam,
    IN LPARAM LParam);

long CALLBACK
ConnWndProc(
    IN HWND   WindowHandle,
    IN UINT   Message,
    IN WPARAM WParam,
    IN LPARAM LParam);

long CALLBACK
SendEditSubClass(
    IN HWND   WindowHandle,
    IN UINT   Message,
    IN WPARAM WParam,
    IN LPARAM LParam);

long CALLBACK
RecvEditSubClass(
    IN HWND   WindowHandle,
    IN UINT   Message,
    IN WPARAM WParam,
    IN LPARAM LParam);

BOOL CALLBACK
CloseEnumProc(
    IN HWND WindowHandle,
    IN LONG LParam);

BOOL
GetConnectionInfo(
    IN HWND ConnectionWindow);

void
QueryAndMaximize(void);


//
// Static Global Variables
//

static HMENU   MainMenu;       // the main MDI menu
static HMENU   MainMenuWindow; // the window associated with the above
static FARPROC OldEditWndProc; // edit control WndProc before subclassing



// 
// Externally-Visible Global Variables
//

HANDLE  GlobalInstance;                  // ientifies the instance of chat
char    ConnClassStr[] = "ConnChild";    // string to register window class
char    ChatClassStr[] = "WS2ChatFrame"; // string to register window class
HWND    GlobalFrameWindow;               // Chat's main (frame) window



//
// Function Definitions
//


int CALLBACK
WinMain(
    IN HINSTANCE InstanceHandle,
    IN HINSTANCE PrevInstanceHandle,
    IN LPSTR  CmdLine,
    IN int    CmdShow)

/*++

Routine Description:

    WinMain is the main entry point for WinSock 2 Chat.  This function
    calls initialization functions and goes into a message loop.

Arguments:

    InstanceHandle -- Supplies the current instance handle.

    PrevInstanceHandle -- Supplies the previous instance handle.

    CmdLine -- Supplies the address of the command line.

    CmdShow -- Supplies the show state of the window.

Return Value:

    0 -- Initialization failure

    !0 -- Returns the Message.wParam of the WM_QUIT message that ended
    the message loop.

--*/
{
    HANDLE   AccelTable;   // handle to the accelerator table
    HWND     ClientWindow; // MDI Client window
    MSG      Message;      // holds the message
    int      ReturnValue;  // return value

    ReturnValue = 0;
    GlobalInstance = InstanceHandle;

    // Initialize Chat.
    if (!InitUI(InstanceHandle, PrevInstanceHandle)) {
        MessageBox(NULL, "Couldn't Initialize Chat!", "Error", 
                   MB_OK | MB_ICONSTOP | MB_SETFOREGROUND);
        goto Done;
    }

    // Create the frame window.
    GlobalFrameWindow = CreateWindow(ChatClassStr,
                                     "WinSock 2 Chat",
                                     WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
                                     CW_USEDEFAULT,
                                     CW_USEDEFAULT,
                                     CW_USEDEFAULT,
                                     CW_USEDEFAULT,
                                     NULL,
                                     (HMENU)MainMenu,
                                     InstanceHandle,
                                     NULL);
    
    if (GlobalFrameWindow == NULL) {
        ChatSysError("CreateWindow()", 
                     "WinMain()", 
                     TRUE);
    }
    
    // Startup WinSock 2.
    if (!InitWS2()) {
        goto Done;
    }

    // Find out about protocols installed.
    if (!FindProtocols()) {
        goto Done;
    }

    // Listen on a local address for all installed protocols.
    if (!ListenAll()) {
        goto Done;
    }
    
    // Load some handles, update the application window, and enter the
    // main message loop.
    ClientWindow = GetWindow(GlobalFrameWindow, GW_CHILD);
    AccelTable = LoadAccelerators(InstanceHandle, "MdiAccel");
    if (AccelTable == NULL) {
        ChatSysError("LoadAccelerators()",
                     "WinMain()",
                     FALSE);
    }
    ShowWindow(GlobalFrameWindow, CmdShow);
    UpdateWindow(GlobalFrameWindow);

    // Enter the GetMessage loop.
    while (GetMessage(&Message, NULL, 0, 0)) {
        if (!TranslateMDISysAccel(ClientWindow, &Message) &&
            !TranslateAccelerator(GlobalFrameWindow, AccelTable, &Message)) {
             TranslateMessage(&Message);
             DispatchMessage(&Message);
        }
    }

    // WM_QUIT was received.  Cleanup and return.
    WSACleanup();
    ReturnValue = Message.wParam;

 Done:

    return(ReturnValue);

} // WinMain()





BOOL
InitUI(
    IN HINSTANCE InstanceHandle,
    IN HINSTANCE PrevInstanceHandle)

/*++

Routine Description:

    Initializes chat's user interface.  Registers the window classes
    used by chat and obtains handles to chat's menus.

Arguments:

    InstanceHandle - Supplies the current instance handle.

    PrevInstanceHandle - Supplies the previous instance handle.

Return Value:

    TRUE - Chat successfully initialized

    FALSE - Failure during chat initialization

--*/
{
    WNDCLASS WndClass;           // window class structure
    BOOL     ReturnValue = TRUE; // holds return value

    // If this is the first instance of chat, register the window
    // classes.
    if (!PrevInstanceHandle) {
        WndClass.style         = CS_HREDRAW | CS_VREDRAW;
        WndClass.lpfnWndProc   = MainWndProc;
        WndClass.cbClsExtra    = 0;
        WndClass.cbWndExtra    = 0;
        WndClass.hInstance     = InstanceHandle;
        WndClass.hIcon         = LoadIcon(NULL, IDI_APPLICATION);
        WndClass.hCursor       = LoadCursor(NULL, IDC_ARROW);
        WndClass.hbrBackground = (HBRUSH)(COLOR_APPWORKSPACE + 1);
        WndClass.lpszMenuName  = NULL;
        WndClass.lpszClassName = ChatClassStr;
        if (!RegisterClass(&WndClass)) {
            ReturnValue = FALSE;
            goto Done;
        }

        WndClass.style         = CS_HREDRAW | CS_VREDRAW;
        WndClass.lpfnWndProc   = ConnWndProc;
        WndClass.cbClsExtra    = 0;
        WndClass.cbWndExtra    = CONN_WND_EXTRA;
        WndClass.hInstance     = InstanceHandle;
        WndClass.hIcon         = LoadIcon(NULL, IDI_APPLICATION);
        WndClass.hCursor       = LoadCursor(NULL, IDC_ARROW);
        WndClass.lpszMenuName  = NULL;
        WndClass.lpszClassName = ConnClassStr;
        if (!RegisterClass(&WndClass)) {
            ReturnValue = FALSE;
            goto Done;
        }
    }

    // Obtain handles to the menus & submenus.
    MainMenu  = LoadMenu(GlobalInstance, "MdiMenuMain");
    if (MainMenu == NULL) {
        ChatSysError("LoadMenu()",
                     "InitUI()",
                     TRUE);
    }
    MainMenuWindow  = GetSubMenu(MainMenu, MAIN_MENU_POS);

 Done:
    
    return(ReturnValue);

} // InitUI()





long CALLBACK
MainWndProc(
    IN HWND   WindowHandle,
    IN UINT   Message,
    IN WPARAM WParam,
    IN LPARAM LParam)

/*++

Routine Description:

    WinSock 2 chat's main window procedure.

Arguments:

    WindowHandle - Supplies the window handle for chat's frame window.

    Message - Supplies the message identifier.

    WParam - Supplies the first message parameter.

    LParam - Supplies the second message parameter.

Return Value:

    Return value depends on the message sent.

--*/
{
    static HWND        ClientWindow; // the MDI client window
    CLIENTCREATESTRUCT ClientCreate; // MDI Client window creation data
    HWND               ChildWindow;  // gets handle to child conn. windows
    MDICREATESTRUCT    MdiCreate;    // MDI Child window creation data
        
    switch (Message) {
        
    case WM_CREATE:

        // Create the client window.
        ClientCreate.hWindowMenu  = MainMenuWindow;
        ClientCreate.idFirstChild = IDM_FIRSTCHILD;
        ClientWindow = CreateWindow("MDICLIENT", 
                                    NULL,
                                    WS_CHILD | WS_CLIPCHILDREN | WS_VISIBLE,
                                    0, 0, 0, 0,
                                    WindowHandle, 
                                    (HMENU)CLIENT_WINDOW_ID,
                                    GlobalInstance, 
                                    (LPSTR)&ClientCreate);
        if (ClientWindow == NULL) {
            ChatSysError("CreateWindow()", 
                         "MainWndProc()",
                         TRUE);
            
        }
            
        return (0);
        
    case WM_COMMAND:

        // We know the wm_command is coming from a menu item, so we
        // can safely assume that the high-order word of WParam is
        // zero. 
        switch (WParam) {

        case IDM_CONNECT:

            // Create a Connection child window.
            MdiCreate.szClass = ConnClassStr;
            MdiCreate.szTitle = "Connecting...";
            MdiCreate.hOwner  = GlobalInstance;
            MdiCreate.x       = CW_USEDEFAULT;
            MdiCreate.y       = CW_USEDEFAULT;
            MdiCreate.cx      = CW_USEDEFAULT;
            MdiCreate.cy      = CW_USEDEFAULT;
            MdiCreate.style   = 0;
            MdiCreate.lParam  = 0;
            ChildWindow = 
              (HWND) SendMessage(ClientWindow, WM_MDICREATE, 0,
                                 (LPARAM)(LPMDICREATESTRUCT)&MdiCreate);

            if (ChildWindow == NULL) {
                ChatSysError("SendMessage(WM_MDICREATE)",
                             "MainWndProc()", 
                             TRUE);
            }

            // Get the connection information from the caller and
            // attempt to make a connection...
            if (!GetConnectionInfo(ChildWindow) || 
                !MakeConnection(ChildWindow)) {
                DestroyWindow(ChildWindow);
            }
            
            break;
            
        case IDM_CLOSE:

            // Close the active window.
            ChildWindow = (HWND)SendMessage(ClientWindow, WM_MDIGETACTIVE, 0, 
                                            0);
            SendMessage(ClientWindow, WM_MDIDESTROY, (WPARAM)ChildWindow, 0);
            break;
            

        case IDM_EXIT:

            // Exit the program.
            SendMessage(WindowHandle, WM_CLOSE, 0, 0);
            break;
            
        case IDM_TILE:

            SendMessage(ClientWindow, WM_MDITILE, 0, 0);
            break;

        case IDM_CASCADE:

            SendMessage(ClientWindow, WM_MDICASCADE, 0, 0);
            break;

        case IDM_ARRANGE:

            SendMessage(ClientWindow, WM_MDIICONARRANGE, 0, 0);
            break;

        case IDM_CLOSEALL:

            // Close all child windows.
            EnumChildWindows(ClientWindow, CloseEnumProc, 0);
            break;

        default:  

            // Pass to active child.
            ChildWindow = (HWND)SendMessage(ClientWindow, WM_MDIGETACTIVE, 
                                            0, 0);
            
            if (IsWindow(ChildWindow)) {
                SendMessage(ChildWindow, WM_COMMAND, WParam, LParam);
            }

            // This causes us to drop out of the switch and to pass
            // the message on to DefFrameProc().
            break; 
        
        } // switch (WParam)
        break;

    case WM_CLOSE:

        // Close all child windows.
        SendMessage(WindowHandle, WM_COMMAND, IDM_CLOSEALL, 0);
        if (GetWindow(ClientWindow, GW_CHILD) != NULL) {
            return (0);
        }
        break;
        
    case USMSG_ACCEPT:

        // Create a MDI Child connection window (even though we aren't
        // yet sure if we will accept the connection -- if we don't,
        // it will be immediately destroyed.)
        MdiCreate.szClass = ConnClassStr;
        MdiCreate.szTitle = "Connection Request...";
        MdiCreate.hOwner  = GlobalInstance;
        MdiCreate.x       = CW_USEDEFAULT;
        MdiCreate.y       = CW_USEDEFAULT;
        MdiCreate.cx      = CW_USEDEFAULT;
        MdiCreate.cy      = CW_USEDEFAULT;
        MdiCreate.style   = 0;
        ChildWindow = (HWND)SendMessage(ClientWindow, WM_MDICREATE, 0,
                                        (LPARAM)(LPMDICREATESTRUCT)&MdiCreate);
        if (ChildWindow == NULL) {
            ChatSysError("SendMessage(WM_MDICREATE)",
                         "MainWndProc()",
                         TRUE);
        }

        // Service the incoming connection request.
        HandleAcceptMessage(ChildWindow, (SOCKET)WParam, LParam);
        break;
        
    case WM_DESTROY:

        // Clean up the listening sockets and kill the application.
        CleanUpSockets();
        PostQuitMessage(0);
        break;

    } // switch(Message)

    // Pass unprocessed messages to DefFrameProc.
    return (DefFrameProc(WindowHandle, 
                         ClientWindow, 
                         Message, 
                         WParam, 
                         LParam));

} // MainWndProc()





long CALLBACK
ConnWndProc(
    IN HWND   WindowHandle,
    IN UINT   Message,
    IN WPARAM WParam,
    IN LPARAM LParam)

/*++

Routine Description:

    Windows procedure for each connection window.

Arguments:

    WindowHandle - Supplies the handle to the connection window which
    is to receive the message.

    Message - Supplies the message identifier.

    WParam - Supplies the first message parameter.

    LParam - Supplies the second message parameter.

Return Value:

    Return value depends on the message sent.

--*/
{
    PCONNDATA         ConnData;    // pointer to data for this connection
    RECT              Rect;        // used for painting the window
    HDC               Hdc;         // device context for painting
                
    switch (Message) {

    case WM_CREATE:

        // Allocate memory for private window data.
        ConnData = (PCONNDATA)malloc(sizeof(CONNDATA));
        if (ConnData == NULL) {
            ChatSysError("malloc()",
                         "ConnWndProc()",
                         TRUE);
        }
        memset((char *)ConnData, 0, sizeof(CONNDATA));

        // Zero is a valid socket ID so set the socket member to INVALID_SOCKET
        ConnData->Socket = INVALID_SOCKET;
        
        GetClientRect(WindowHandle, &Rect);
            
        // Create Send/Recv edit controls.
        ConnData->SendWindow = 
          CreateWindow("EDIT",
                       NULL,
                       WS_CHILD | WS_VISIBLE   | WS_VSCROLL |
                       ES_LEFT  | ES_MULTILINE | ES_AUTOVSCROLL,
                       0,
                       0,
                       Rect.right,
                       Rect.bottom/2,
                       WindowHandle,
                       (HMENU)EC_SEND_CHILD,
                       GlobalInstance,
                       NULL);
        
        ConnData->RecvWindow = 
          CreateWindow("EDIT",
                       NULL,
                       WS_CHILD | WS_VISIBLE   | WS_VSCROLL |
                       ES_LEFT  | ES_MULTILINE | ES_AUTOVSCROLL,
                       0,
                       Rect.bottom/2 + 1,
                       Rect.right,
                       Rect.bottom - Rect.bottom/2 - 1,
                       WindowHandle,
                       (HMENU)EC_RECV_CHILD,
                       GlobalInstance,
                       NULL);

        if (!ConnData->SendWindow || !ConnData->RecvWindow) {
            ChatSysError("CreateWindow()",
                         "ConnWndProc()",
                         TRUE);
        }

        // Fill in some connection-specific data.
        ConnData->SocketEventObject = CreateEvent(NULL, 
                                                  FALSE, 
                                                  FALSE, 
                                                  NULL); 
        ConnData->OutputEventObject = CreateEvent(NULL, 
                                                  FALSE, 
                                                  FALSE, 
                                                  NULL);
        ConnData->OutputQueue = QCreate();
        if (!ConnData->SocketEventObject || !ConnData->OutputEventObject ||
            !ConnData->OutputQueue) {
            ChatSysError("CreateEvent() or QCreate()",
                         "ConnWndProc()",
                         TRUE);
        }
        ConnData->ConnectionWindow = WindowHandle;
        ConnData->WriteOk = FALSE;
                
        // Subclass the edit controls. Both functions necessarily
        // return the same FARPROC...
        OldEditWndProc = (FARPROC)SetWindowLong(ConnData->SendWindow, 
                                                GWL_WNDPROC, 
                                                (DWORD)SendEditSubClass);
        SetWindowLong(ConnData->RecvWindow, 
                      GWL_WNDPROC,
                      (DWORD)RecvEditSubClass);

        // Split the connection window in half.
        Hdc = GetDC(WindowHandle);
        MoveToEx(Hdc, 0, Rect.bottom/2, NULL);
        LineTo(Hdc, Rect.right, Rect.bottom/2);
        ReleaseDC(WindowHandle, Hdc);
                                                  
        // Set the private window data to be a pointer to ConnData.
        SetWindowLong(WindowHandle, GWL_CONNINFO, (LONG)ConnData);
        
        return (0);
        
    case WM_COMMAND:

        switch (HIWORD(WParam)) {

        case 0:

            // If the high-order word is zero, this message is being
            // sent from a menu-item (hopefully it was passed to us
            // from the frame window, since the mdi child connection
            // windows, of course, have no menus).
            switch (LOWORD(WParam)) {

            case IDM_CLEAR_SENDBUFFER:

                // Clear the send buffer.
                ConnData = GetConnData(WindowHandle);
                PostMessage(ConnData->SendWindow, EM_SETSEL, 0, -1);
                PostMessage(ConnData->SendWindow, WM_CLEAR, 0, 0);
                return (0);
                                
            case IDM_CLEAR_RECVBUFFER:
                
                // Clear the receive buffer.
                ConnData = GetConnData(WindowHandle);
                PostMessage(ConnData->RecvWindow, EM_SETSEL, 0, -1);
                PostMessage(ConnData->RecvWindow, WM_CLEAR, 0, 0);
                return (0);
                
            default:

                // An unknown menu item... just return.
                return (0);

            } // switch (LOWORD(WParam))
            
        case 1:
            
            // The high-order word is 1, so thus the wm_command
            // message is coming from an accelerator. Ignore it.
            return(0);
            
        default:
            
            // Let the default window procedure handle it (below).
            break;
            
        } // switch (HIWORD(WParam))
        break;
            

    case WM_PAINT:

        // Window needs to be painted, split the connection window in
        // half.
        GetClientRect(WindowHandle, &Rect);
        Hdc = GetDC(WindowHandle);
        MoveToEx(Hdc, 0, Rect.bottom/2, NULL);
        LineTo(Hdc, Rect.right, Rect.bottom/2);
        ReleaseDC(WindowHandle, Hdc);

        return (DefMDIChildProc(WindowHandle, 
                                Message, 
                                WParam, 
                                LParam));
                
    case WM_SIZE:

        ConnData = GetConnData(WindowHandle);

        // Resize the edit control windows.
        MoveWindow(ConnData->SendWindow,
                   0,
                   0,
                   LOWORD(LParam),
                   HIWORD(LParam)/2,
                   TRUE);
            
        MoveWindow(ConnData->RecvWindow,
                   0,
                   HIWORD(LParam)/2 + 1,
                   LOWORD(LParam),
                   HIWORD(LParam) - HIWORD(LParam)/2 - 1,
                   TRUE);

        return (DefMDIChildProc(WindowHandle,
                                Message, 
                                WParam, 
                                LParam));
        
    case USMSG_CONNECT:

        // Handle the new connection.
        HandleConnectMessage(WindowHandle, LParam);
        return(0);
        
    case WM_DESTROY:

        ConnData = GetConnData(WindowHandle);
        CleanupConnection(ConnData);
        
        return(DefMDIChildProc(WindowHandle, 
                               Message, 
                               WParam, 
                               LParam));

    } // switch (Message)

    // Pass unprocessed message to DefMDIChildProc.
    return(DefMDIChildProc(WindowHandle, 
                           Message, 
                           WParam, 
                           LParam));

} // ConnWndProc()





long CALLBACK
SendEditSubClass(
    IN HWND   WindowHandle,
    IN UINT   Message,
    IN WPARAM WParam,
    IN LPARAM LParam)

/*++

Routine Description:

    Window procedure used to subclass the edit control for the sending
    window.  

Implementation:

    Text is only allowed to be appended to or deleted from the end of
    an edit control's text buffer.  When text is either typed or
    paste, Chat creates an output request data structure and sticks it
    on the output thread for this connection.

Arguments:

    WindowHandle - Supplies the handle to the edit control's window

    Message - Supplies the message identifier.

    WParam - Supplies the first message parameter.

    LParam - Supplies the second message parameter.

Return Value:

    ReturnValue - Depends of the message sent.

--*/
{
    int             Index;        // an index into the edit control's text
    HANDLE          CBDataHandle; // handle to the clipboard data
    LPSTR           CBDataPtr;    // pointer to data to send
    PCONNDATA       ConnData;     // pointer to connection-specific data
    POUTPUT_REQUEST OutReq;       // output request data
    int             TextSize;     // number of bytes of text to send
    DWORD           BytesLeft;    // number of bytes left to send

    ConnData = GetConnData(GetParent(WindowHandle));
        
    switch (Message) {

    case WM_CHAR:

        // If the character is sendable, enqueue the character on the
        // output queue, and signal the output event.
        if (IsSendable((char)WParam)) {
            
            OutReq = (POUTPUT_REQUEST) malloc(sizeof(OUTPUT_REQUEST));
            if (OutReq == NULL) {
                ChatSysError("malloc()",
                             "SendEditSubClass()",
                             TRUE);
            }

            // Rather than malloc a one-character buffer, just point
            // Buffer into the already-allocated Char field of the
            // OutReq structure.  This gets rid of a heavy-weight
            // malloc for each character typed.
            OutReq->Buffer.len = sizeof(char);
            OutReq->Buffer.buf = &OutReq->Character;
                        
            // Fill in the OutReq structure.
            *(OutReq->Buffer.buf) = (char)WParam;
            OutReq->Type = NON_OVERLAPPED_IO;
            OutReq->ConnData = ConnData;
            
            QInsert(ConnData->OutputQueue, (LPVOID)OutReq);
            SetEvent(ConnData->OutputEventObject);   
        }

        // Move the cursor to the end of the text buffer.
        Index = GetWindowTextLength(WindowHandle);
        CallWindowProc((WNDPROC)OldEditWndProc,
                       WindowHandle, 
                       EM_SETSEL, 
                       Index, 
                       Index);

        // Make room for the new text in the edit control.
        MakeRoom(WindowHandle, 1);
        
        // Pass on the message to the original Edit Window Procedure.
        return (CallWindowProc((WNDPROC)OldEditWndProc,
                               WindowHandle, 
                               Message, 
                               WParam,
                               LParam));
        
    case WM_PASTE:

        // Get the buffer from the edit control and send it over the
        // socket.
        if (IsClipboardFormatAvailable(CF_TEXT)) {
            
            if (OpenClipboard(WindowHandle)) {
                CBDataHandle = GetClipboardData(CF_TEXT);
                
                if (CBDataHandle) {
                    
                    CBDataPtr = GlobalLock(CBDataHandle);
                    TextSize = strlen(CBDataPtr);
                    
                    // Make room in the edit control for the new text; if
                    // this is impossible, just return because MakeRoom
                    // notifies the user internally.
                    if (!MakeRoom(WindowHandle, TextSize)) {
                        GlobalUnlock(CBDataHandle);
                        CloseClipboard();
                        return(0);
                    }
                    
                    // If the text is longer than the maximum message
                    // size, we need to split it into bite-size pieces
                    // for consumption by the transport layer.
                    BytesLeft = TextSize;
                    while (BytesLeft != 0) {

                        // Create a new output request.
                        OutReq = 
                          (POUTPUT_REQUEST)malloc(sizeof(OUTPUT_REQUEST));
                        if (OutReq == NULL) {
                            ChatSysError("malloc()", 
                                         "SendEditSubClass",
                                         TRUE);
                        }

                        // Determine how much data to send with this
                        // request, and allocate a buffer that size.
                        if (BytesLeft > ConnData->MaxMsgSize) {
                            OutReq->Buffer.len = ConnData->MaxMsgSize;
                        } else {
                            OutReq->Buffer.len = BytesLeft;
                        }
                        OutReq->Buffer.buf = 
                          (char *)malloc(OutReq->Buffer.len);
                        if (OutReq->Buffer.buf == NULL) {
                            ChatSysError("malloc()", 
                                         "SendEditSubClass",
                                         TRUE);
                        }

                        // Fill in the rest of the OutReq structure
                        // and put it in the output queue.
                        OutReq->Type = OVERLAPPED_IO;
                        OutReq->ConnData = ConnData;
                        memcpy(OutReq->Buffer.buf, CBDataPtr, 
                               OutReq->Buffer.len);
                        QInsert(ConnData->OutputQueue, (LPVOID)OutReq);
                        
                        // Update the number of bytes left to send and
                        // the pointer to the rest of the data.
                        BytesLeft -= OutReq->Buffer.len;
                        CBDataPtr += OutReq->Buffer.len;
                    }
                    
                    // Signal the ouput event.
                    SetEvent(ConnData->OutputEventObject);
                    GlobalUnlock(CBDataHandle);

                } else {

                    MessageBox(WindowHandle,
                               "Sorry, couldn't retrieve the clipoard data.",
                               "Error.", 
                               MB_OK | MB_ICONSTOP | MB_SETFOREGROUND);
                }

                CloseClipboard();

            } // if (OpenClipboard(WindowHandle))
            else {
                
                MessageBox(WindowHandle,
                           "Couldn't open the clipboard!!",
                           "Try again.", 
                           MB_OK | MB_ICONSTOP | MB_SETFOREGROUND);
            }

        } // if (IsClipboardFormatAvailable(CF_TEXT)) 
        else {
            
            MessageBox(WindowHandle,
                       "Sorry, the CF_TEXT format not available.",
                       "Error.", MB_OK | MB_ICONSTOP | MB_SETFOREGROUND);
        }
        // Move the cursor to the end of the text buffer.
        Index = GetWindowTextLength(WindowHandle);
        CallWindowProc((WNDPROC)OldEditWndProc,
                       WindowHandle, 
                       EM_SETSEL, 
                       Index, 
                       Index);

        // Call the real edit control window procedure.
        return (CallWindowProc((WNDPROC)OldEditWndProc,
                               WindowHandle, 
                               Message, 
                               WParam, 
                               LParam));
        
    case WM_KEYDOWN:

        // Capture the delete key. nVirtKey: 46 = 'Del'.
        if (WParam == 46) {
            return(0);
        } else {
            return (CallWindowProc((WNDPROC)OldEditWndProc, WindowHandle,
                                   Message, WParam, LParam));
        }
        
    case WM_CUT:  // Flow through
    case WM_UNDO:

        return (0);
        
    default:

        return (CallWindowProc((WNDPROC)OldEditWndProc,
                               WindowHandle, 
                               Message, 
                               WParam, 
                               LParam));
    
    } // switch (Message)

} // SendEditSubClass()





long CALLBACK
RecvEditSubClass(
    IN HWND   WindowHandle,
    IN UINT   Message,
    IN WPARAM WParam,
    IN LPARAM LParam)

/*++

Routine Description:

    Window procedure used to subclass the edit control for the
    receiving window.  

Implementation:

    The user cannot modify any of the text displayed in the receiving
    edit control. All text received from the socket associated with
    this edit control is displayed. 

Arguments:

    WindowHandle - Supplies the handle to the edit control's window

    Message - Supplies the message identifier.

    WParam - Supplies the first message parameter.

    LParam - Supplies the second message parameter.

Return Value:

    ReturnValue - Depends of the message sent.

--*/
{
    switch (Message) {

    case WM_CHAR:       // Flow through
    case WM_KEYDOWN:    // Flow through
    case WM_UNDO:       // Flow through
    case WM_PASTE:      // Flow through
    case WM_CUT:

        return (0);     // Effectively disables the above messages.
        
    default:

        return (CallWindowProc((WNDPROC)OldEditWndProc,
                               WindowHandle, 
                               Message, 
                               WParam, 
                               LParam));
    }

} // RecvEditSubClass()





BOOL CALLBACK
CloseEnumProc(
    IN HWND WindowHandle,
    IN LONG LParam)

/*++
                                                                          
Routine Description:

    A callback function used to destroy all of chat's connection
    windows. 

Arguments:

    WindowHandle - Supplies the handle to the child's window which
                   is to be closed.

    LParam - Not Used.

Return Value:

    TRUE - Returns true in order to continue enumeration.

--*/
{
    if (!GetWindow(WindowHandle, GW_OWNER)) {
        
        SendMessage(GetParent(WindowHandle), WM_MDIRESTORE, 
                    (WPARAM)WindowHandle, 0);
        
        if (SendMessage(WindowHandle, WM_QUERYENDSESSION, 0, 0)) {
            SendMessage(GetParent(WindowHandle), WM_MDIDESTROY, 
                        (WPARAM)WindowHandle, 0);
        }
    }
    return (TRUE);
    
} // CloseEnumProc()





void
OutputString(
    IN HWND RecvWindow,
    IN char *String)
/*++
                                                                   
Routine Description:

    This function sends the string received over the socket to the
    edit window given by RecvWindow.  

Implementation:

    This function treats certain non-printable characters specially so
    that they mean the same thing on this end as they did on the end
    that sent them. 

Arguments:

    RecvWindow -- Handle to the edit control window which is to
    receive the text.

    String -- Points to the NULL-terminated string to be output.  

Return Value:

    None.

--*/
{

    int  Index, Index1;         // indices into the edit control text
    char *Current = String;     // the character we are examining
    char *First = String;       // the first character we have not output
    char Buffer[BUFFER_LENGTH]; // scratch buffer for SendMessage
    int  NumChars;              // how many characters we put in Buffer

    // Make room for the string in the edit control.
    MakeRoom(RecvWindow, strlen(String));

    // This loop goes through the whole string we have been instructed
    // to print in the edit control; when it encounters certain
    // characters in the string, it sends certain messages to the edit
    // control to simulate the right behavior.
    while (*Current) {
     
        switch (*Current) {

        case '\b':
            
            // The current character is a backspace.  Print out the
            // string we have so far, and then manually erase a
            // character. 
            *Current = '\0';

            // Output the string at 'First'...
            Index = GetWindowTextLength(RecvWindow);
            SendMessage(RecvWindow, EM_SETSEL, Index, Index);
            SendMessage(RecvWindow, EM_REPLACESEL, 0, (LPARAM)First);

            // Get the last line of the text into 'Buffer'.
            Index = GetWindowTextLength(RecvWindow);
            Index1 = SendMessage(RecvWindow, EM_LINEFROMCHAR, Index, 0);
            NumChars = SendMessage(RecvWindow, EM_GETLINE, Index1, 
                                   (LPARAM)Buffer);

            // If the line had no characters in it, then it's the
            // beginning of a new line, and we should erase two
            // characters (the preceding \r\n) rather than just one.
            if (NumChars == 0) {
                SendMessage(RecvWindow, EM_SETSEL, Index - 2, Index);
                SendMessage(RecvWindow, EM_REPLACESEL, 0, (LPARAM)"");
            } else {
                SendMessage(RecvWindow, EM_SETSEL, Index - 1, Index);
                SendMessage(RecvWindow, EM_REPLACESEL, 0, (LPARAM)"");
            }

            First = Current + 1;
            break;

        case '\r':

            // A newline.  If the Chat window is minimized, ask the
            // user if they want it maximized.
            QueryAndMaximize();

            // If a remote user typed a newline, we'll receive '\r',
            // but if the remote user pasted it in, we'll get '\r\n'. 
            if (*(Current + 1) != '\n') {
                
                // The newline was typed in...output the string up to
                // the newline, then "\r\n".
                *Current = '\0';
                
                Index = GetWindowTextLength(RecvWindow);
                SendMessage(RecvWindow, EM_SETSEL, Index, Index);
                SendMessage(RecvWindow, EM_REPLACESEL, 0, (LPARAM)First);
                
                Index = GetWindowTextLength(RecvWindow);
                SendMessage(RecvWindow, EM_SETSEL, Index, Index);
                SendMessage(RecvWindow, EM_REPLACESEL, 0, (LPARAM)"\r\n");
                
                First = Current + 1;
            }
            break;

        default:
            
            break;
        
        } // switch (*Current)
        Current++;
    
    } // while (*Current)

    // Ouptut any string leftover in First
    Index = GetWindowTextLength(RecvWindow);
    SendMessage(RecvWindow, EM_SETSEL, Index, Index);
    SendMessage(RecvWindow, EM_REPLACESEL, 0, (LPARAM)First);
}





void
QueryAndMaximize(void) 
/*++
                                                               
Routine Description:

    If Chat's main frame window is minimized, this function asks the
    user if they want to activate and display Chat, and acts
    accordingly. 

Arguments:

    None.

Return Value:

    None.

--*/
{
    WINDOWPLACEMENT WinPlace; // holds window placement data
    
    WinPlace.length = sizeof(WINDOWPLACEMENT);

    // Get chat's main window's state.
    GetWindowPlacement(GlobalFrameWindow, &WinPlace);

    // If it's minimized, ask the user if they want to maximize.
    if (WinPlace.showCmd == SW_SHOWMINIMIZED) {

        if (MessageBox(GlobalFrameWindow, 
                       "There is activity on a Chat connection.  Maximize?",
                       "Chat has data.",
                       MB_ICONQUESTION | MB_YESNO | MB_SETFOREGROUND) 
            == IDYES) {

            // Maximize chat's main window.
            WinPlace.showCmd = SW_RESTORE;
            SetWindowPlacement(GlobalFrameWindow, &WinPlace);
        }
    }
} // QueryAndMaximize()





BOOL
GetConnectionInfo(
    IN HWND ConnectionWindow)

/*++

Routine Description:

    Queries the user as to what sort of connection (i.e. what address
    family/protocol) she wants to make.  

Implementation:

    After popping up a listbox, filling it in with all the available
    protocol choices, and getting the user's choice, this function
    uses it to determine what kind of dialog box to pop up to get the
    actual connection parameters, which are stored in the CONNDATA
    associated with the given ConnectionWindow.

Arguments:

    ConnectionWindow -- Handle to the connection window associated
    with this connection.

Return Value:

    TRUE - The user closed a dialog box by clicking the 'Ok' button.

    FALSE - The user closed a dialog box by clicking the 'Cancel'
            button.

--*/

{
    PCONNDATA ConnData;           // connection-specific data
    BOOL      ReturnValue = TRUE; // holds the return value

    ConnData = GetConnData(ConnectionWindow);
        
    // Pop up a dialog box from which the user chooses the protocol; a
    // pointer to a protocol info struct is stored in ConnData.
    if (!DialogBoxParam(GlobalInstance, 
                        "ChooseFamilyDlg", 
                        ConnectionWindow, 
                        ChooseFamilyDlgProc, 
                        (LPARAM)ConnData)) {
        ReturnValue = FALSE;
        goto Done;
    }
    
    // The user has selected a protocol; now pop up an address family
    // specific dialog box to get the address to which we want to make
    // a chat connection, a pointer to which is stored in
    // ConnData->SockAddr (see InetConnDlg, for example).
    switch (ConnData->ProtocolInfo->iAddressFamily) {

    case AF_INET:
        
        ReturnValue = DialogBoxParam(GlobalInstance, 
                                     "InetConnDlg", 
                                     ConnectionWindow,
                                     InetConnDlgProc, 
                                     (LPARAM)ConnData);
        goto Done;

    default:
        
        ReturnValue = DialogBoxParam(GlobalInstance, 
                               "DefaultConnDlg", 
                               ConnectionWindow, 
                               DefaultConnDlgProc,
                               (LPARAM)ConnData);
        goto Done;
    }

 Done:

    return(ReturnValue);

} // GetConnectionInfo()





BOOL
TranslateHex(
    OUT LPVOID Buffer,
    IN  int    BufferLen,
    IN  char   *HexString,
    IN  HWND   WindowHandle)
/*++

Routine Description:

    This function converts an arbitrarily long string of hexidecimal
    digits into a binary representation.  

    Example -- if the string "0fbf" is passed in, then two bytes will
    be written into Buffer -- 15 and 191.

Arguments:

    Buffer -- Points to a buffer where we want the data stored.

    BufferLen -- The length, in bytes, of Buffer.

    HexString -- A null-terminated string of digits with values in the
    ASCII range of 0-9 and a-f.
 
    WindowHandle -- Window handle needed in case we pop up a dialog
    box. 

Return Value:

    TRUE -- HexString was successfully translated.

    FALSE -- One of the characters does not represent a hex digit, or
    the HexString is too long to be translated into BufferLen bytes.

--*/
{

    int  HexStringLen;       // the length of the HexString, in bytes
    int  i;                  // counting variable
    char TwoCharString[3];   // stores two hex characters + a NULL char
    char *NextByte;          // next free byte in the buffer
    BOOL ReturnValue = TRUE; // return value

    HexStringLen  = strlen(HexString);
    NextByte = (char *)Buffer;
    TwoCharString[2] = '\0';

    // Zero out the output buffer.
    memset(NextByte, 0, BufferLen);
    
    // If the hex string in more than twice as long as the buffer, we
    // don't have a big enough buffer.
    if ((HexStringLen / 2) > BufferLen) {
        ReturnValue = FALSE;
    } else {

        // Go through the hex string two characters at a time...
        for (i = 0; i < HexStringLen; i += 2) {
            
            // Copy the next two bytes of the hex string into
            // TwoCharString; then check to make sure they are both
            // hexidecimal digits.
            memcpy(TwoCharString, HexString + i, 2);
            if (!isxdigit(TwoCharString[0]) || !isxdigit(TwoCharString[1])) {
                MessageBox(WindowHandle, "Type a hexidecimal number, please",
                           "Error.", MB_OK | MB_ICONSTOP | MB_SETFOREGROUND);
                ReturnValue = FALSE;
                break;
            }
            *NextByte++ = (char)strtol(TwoCharString, NULL, 16);

        } // for
    } // else
    return(ReturnValue);

} // TranslateHex()





BOOL
PackTwoStrings(
    OUT char  *Buffer,
    IN  int   BufferLen,
    IN  char  *String1,
    IN  char  *String2)
/*++

Routine Description:

    Packs two strings into one buffer.  Useful for packing caller data
    into a buffer for connection-time data transfer.  It is not an
    error if either string is of zero length, i.e. just the '\0'
    character. 

Arguments:

    Buffer -- Pointer to a buffer into which the two strings will be
    packed. 

    BufferLen -- The length, in bytes, of Buffer.

    String1 - Address of the first NULL terminated string.
     
    String2 - Address of the second NULL terminated string.

Return Value:

    TRUE -- The strings were successfully packed into the buffer.

    FALSE -- The passed in buffer was NULL, one or both of the strings
    were NULL, or Buffer is too small.
    
--*/

{
    int Length1, Length2;    // holds the length of the strings
    BOOL ReturnValue = TRUE; // holds the return value

    if (!Buffer || !String1 || !String2) {
        ReturnValue = FALSE;
        goto Done;
    }
        
    Length1 = strlen(String1);
    Length2 = strlen(String2);
    
    if (BufferLen < (Length1 + Length2 + 2)) {
        ReturnValue = FALSE;
        goto Done;
    }
    
    strcpy(Buffer, String1);
    strcpy(Buffer + Length1 + 1, String2);
 
 Done:
   
    return(ReturnValue);

} // PackTwoStrings()





BOOL
ExtractTwoStrings(
    IN  char *Buffer,
    OUT char *String1,
    IN  char Length1,
    OUT char *String2,
    IN  int  Length2)

/*++

Routine Description:

    Extracts two strings from a buffer, expecting them to be packed as
    PackTwoStrings does it.

Arguments:

    Buffer -- The buffer which contains two strings.

    String1 -- A pointer to a buffer of size Length1.

    Length1 -- The length, in bytes, of String1.

    String2 -- A pointer to a buffer of size Length2.

    Length2 -- The length, in bytes, of String2.

Return Value:

    TRUE -- The strings were successfully extracted.

    FALSE -- One of the pointers was NULL, or a packed string was too
    big to be extracted out into a buffer.

--*/

{
    int BufStrLen1, BufStrLen2; // the length of the packed strings
    char *BufStr2;              // points to the second packed string
    BOOL ReturnValue = TRUE;    // holds the return value

    if (!Buffer || !String1 || !String2) {
        ReturnValue = FALSE;
        goto Done;
    }
    
    BufStrLen1 = strlen(Buffer);
    BufStr2 = Buffer + BufStrLen1 + 1;
    BufStrLen2 = strlen(BufStr2);
    
    if ((BufStrLen1 > Length1) || (BufStrLen2 > Length2)) {
        ReturnValue = FALSE;
        goto Done;
    }

    strcpy(String1, Buffer);
    strcpy(String2, BufStr2);

 Done:
 
   return(ReturnValue);

} // ExtractTwoStrings()





void 
ChatSysError(
    IN char *FailedFunction,
    IN char *InFunction,
    IN BOOL Fatal)
/*++

Routine Description:

    Pops up a standard message box to display fatal system errors; if
    the error is fatal, the process is ended and this function does
    not return.    

Arguments:

    FailedFunction -- Pointer to a string that contains the name of
    the system function that failed.

    InFunction -- Pointer to a string that contains the name of the
    chat function in which the failure occurred.

    Fatal -- Boolean indicating whether the error is fatal to the
    process or not.

Return Value:

    None.

--*/
{

    char MsgText[MSG_LEN]; // holds message strings

    wsprintf(MsgText,
             "System call %s failed in chat function %s. Error code: %d",
             FailedFunction, InFunction, GetLastError());
    MessageBox(GlobalFrameWindow, 
               MsgText, 
               Fatal ? "Fatal Error." : "Non-Fatal Error.",
               MB_OK | MB_ICONSTOP | MB_SETFOREGROUND);

    if (Fatal) {
        ExitProcess(CHAT_ERROR);
    }

} // ChatSysError()





BOOL
MakeRoom(
    IN HWND EditControl,
    IN int  HowMuch)
/*++

Routine Description:

    This function makes room in an edit control for more text, if
    necessary. 
 
Arguments:

    EditControl -- The edit control in question.

    HowMuch -- How many characters do we want to put into the edit
    control? 

Return Value:

    TRUE -- If there was not enough room in the edit control, MakeRoom
    successfully chopped off some text from the beginning, and there
    is now enough room.

    FALSE -- The requested amount can not fit in the edit control,
    period; a message box has informed the user.

--*/
{

    int  TextLength;  // how much text is in the edit control
    int  CharIndex1;
    int  CharIndex2;
    int  LineIndex;   // index variables
    BOOL ReturnValue; // return value

    if (HowMuch > MAX_EC_TEXT) {
        
        MessageBox(EditControl, 
                   "Clipboard too long. Try pasting a smaller amount.",
                   "Error.", MB_OK | MB_ICONSTOP | MB_SETFOREGROUND);
        ReturnValue = FALSE;

    } else {

        TextLength = GetWindowTextLength(EditControl);
        
        if (TextLength + HowMuch > MAX_EC_TEXT) {
            
            // Inserting the text would exceed our limit.  This code
            // chops off roughly the first half of the edit control's
            // text. 
            CharIndex1 = TextLength / 2;
            LineIndex = SendMessage(EditControl, EM_LINEFROMCHAR,
                                    (WPARAM)CharIndex1, 0);
            CharIndex2 = SendMessage(EditControl, EM_LINEINDEX,
                                     (WPARAM)LineIndex, 0);
            
            SendMessage(EditControl, EM_SETSEL, 0, CharIndex2);
            SendMessage(EditControl, EM_REPLACESEL, 0, 
                        (LPARAM)"");
        }

        ReturnValue = TRUE;        
    } // else

    return(ReturnValue);
} // MakeRoom()
