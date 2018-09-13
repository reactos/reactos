/*++
  
  Copyright (c) 1995 Intel Corp
  
  File Name:
  
    dt_dll.cpp
  
  Abstract:
  
    Contains main and supporting functions for a Debug/Trace
    DLL for the WinSock2 DLL.  See the design spec
    for more information.
  
  Author:
    
    Michael A. Grafton 
  
--*/

//
// Include Files
//

#include "nowarn.h"  /* turn off benign warnings */
#ifndef _WINSOCKAPI_
#define _WINSOCKAPI_   /* Prevent inclusion of winsock.h in windows.h */
#endif
#include <windows.h>
#include "nowarn.h"  /* some warnings may have been turned back on */
#include <winsock2.h>
#include <stdarg.h>
#include <ws2spi.h>
#include <commdlg.h>
#include <cderr.h>

#include "dt_dll.h"
#include "cstack.h"
#include "dt.h"
#include "handlers.h"

//
// Forward References for Functions
//

LRESULT APIENTRY 
DTMainWndProc(
    IN HWND   WindowHandle, 
    IN UINT   Message, 
    IN WPARAM WParam, 
    IN LPARAM LParam);

LRESULT APIENTRY 
DTEditWndProc(
    IN HWND   WindowHandle, 
    IN UINT   Message, 
    IN WPARAM WParam, 
    IN LPARAM LParam);

BOOL WINAPI 
DllMain(
    HINSTANCE DllInstHandle, 
    DWORD     Reason, 
    LPVOID    Reserved);

DWORD
WindowThreadFunc(LPDWORD TheParam);

BOOL APIENTRY 
DebugDlgProc(
    IN HWND hwndDlg, 
    IN UINT message, 
    IN WPARAM wParam, 
    IN LPARAM lParam);

BOOL
GetFile(
    IN  HWND   OwnerWindow,
    OUT  LPSTR Buffer,
    IN  DWORD  BufSize);

VOID
GetComdlgErrMsg (
    LPTSTR  Buffer,
    LPTSTR  Fmt
    );

void
AbortAndClose(
    IN HANDLE FileHandle,
    IN HWND WindowHandle);




// 
// Externally Visible Global Variables
//

HWND   DebugWindow=NULL;       // handle to the child edit control
HANDLE LogFileHandle=INVALID_HANDLE_VALUE;// handle to the log file
DWORD  OutputStyle = NO_OUTPUT; // where to put output
char   Buffer[TEXT_LEN];       // buffer for building output strings



//
// Static Global Variables
//

// name for my window class
static char             DTWndClass[] = "DTWindow"; 

static HWND             FrameWindow=NULL;   // handle to frame of debug window
static WNDPROC          EditWndProc;   // the edit control's window proc
static HINSTANCE        DllInstHandle; // handle to the dll instance
static DWORD            TlsIndex=-1;      // tls index for this module
static CRITICAL_SECTION CrSec;         // critical section for text output
static BOOL             DllInitialized = FALSE; // Flag to trigger dynamic
                                        // initialization of first API call
static HANDLE           TextOutEvent;  // set when debug window is ready

static char             LogFileName[MAX_PATH+1]; // name of the log file
static char             ModuleFileName[MAX_PATH+1]; // name of the application

// handle to and id of the main thread of the DLL which initializes
// and creates windows, etc
static HANDLE           WindowThread = NULL;     
static DWORD            WindowThreadId;

// function pointer tables for handler functions.  
static LPFNDTHANDLER  HdlFuncTable[MAX_DTCODE + 1];
// Input desktop to communicate to the user
static HDESK   HDesk;
typedef
HDESK (* LPFN_OPENINPUTDESKTOP) (
    DWORD dwFlags,          // flags to control interaction with other 
                            // applications
    BOOL fInherit,          // specifies whether returned handle is 
                            // inheritable
    DWORD dwDesiredAccess   // specifies access of returned handle);
    );

typedef
BOOL (* LPFN_CLOSEDESKTOP) (
    HDESK hDesktop          // handle to desktop to close
    );
static LPFN_OPENINPUTDESKTOP   lpfnOpenInputDesktop;
static LPFN_CLOSEDESKTOP       lpfnCloseDesktop;

// static strings
static char  ErrStr2[] = "An error occurred while trying to get a log"
    " filename (%s).  Debug output will go to the window only.";
static char ErrStr3[] = "Had problems writing to file.  Aborting file"
    " output -- all debug output will now go to the debugger.";
static char sCDERR_USERCANCEL[]="Operation was cancelled.";
static char sCDERR_FINDRESFAILURE[]="The common dialog box function failed"
    " to find a specified resource.";
static char sCDERR_INITIALIZATION[]="The common dialog box function failed"
    " during initialization. This error often occurs when sufficient memory"
    " is not available.";
static char sCDERR_LOCKRESFAILURE[]="The common dialog box function failed"
    " to load a specified resource.";
static char sCDERR_LOADRESFAILURE[]="The common dialog box function failed"
    " to load a specified string.";
static char sCDERR_LOADSTRFAILURE[]="The common dialog box function failed"
    " to lock a specified resource.";
static char sCDERR_MEMALLOCFAILURE[]="The common dialog box function was"
    " unable to allocate memory for internal structures.";
static char sCDERR_MEMLOCKFAILURE[]="The common dialog box function was"
    " unable to lock the memory associated with a handle.";
static char sCDERR_NOHINSTANCE[]="The ENABLETEMPLATE flag was set in the"
    " Flags member of the initialization structure for the corresponding"
    " common dialog box, but you failed to provide a corresponding instance"
    " handle.";
static char sCDERR_NOHOOK[]="The ENABLEHOOK flag was set in the Flags member"
    " of the initialization structure for the corresponding common dialog box,"
    " but you failed to provide a pointer to a corresponding hook procedure.";
static char sCDERR_NOTEMPLATE[]="The ENABLETEMPLATE flag was set in the Flags"
    " member of the initialization structure for the corresponding common"
    " dialog box, but you failed to provide a corresponding template.";
static char sCDERR_STRUCTSIZE[]="The lStructSize member of the initialization"
    " structure for the corresponding common dialog box is invalid.";
static char sFNERR_BUFFERTOOSMALL[]="The buffer pointed to by the lpstrFile"
    " member of the OPENFILENAME structure is too small for the file name"
    " specified by the user.";
static char sFNERR_INVALIDFILENAME[]="A file name is invalid.";
static char sFNERR_SUBCLASSFAILURE[]="An attempt to subclass a list box failed"
    " because sufficient memory was not available.";


//
// Function Definitions
//


BOOL WINAPI 
DllMain(
    HINSTANCE InstanceHandle, 
    DWORD     dwReason, 
    LPVOID    lpvReserved)
/*++
  
  DllMain()
  
  Function Description:
  
      Please see Windows documentation for DllEntryPoint.
  
  Arguments:
  
      Please see windows documentation.
  
  Return Value:
  
      Please see windows documentation.
  
--*/
{
    
    Cstack_c   *ThreadCstack;  // points to Cstack objects in tls 

    switch(dwReason) {
        
    // Determine the reason for the call and act accordingly.
        
    case DLL_PROCESS_ATTACH:

        if (GetModuleFileName (NULL, ModuleFileName, sizeof (ModuleFileName))==0)
            return FALSE;

       
        // Allocate a TLS index.
        TlsIndex = TlsAlloc();
        if (TlsIndex==0xFFFFFFFF)
            return FALSE;

        DllInstHandle = InstanceHandle;
        InitializeCriticalSection(&CrSec);
        TextOutEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
        if (TextOutEvent==NULL)
            return FALSE;

        // Fill in the handler function table.
        DTHandlerInit(HdlFuncTable, MAX_DTCODE);
                

        // flow through...
        
    case DLL_THREAD_ATTACH:
        
        // Store a pointer to a new Cstack_c in the slot for this
        // thread. 
        ThreadCstack = new Cstack_c();
        TlsSetValue(TlsIndex, (LPVOID)ThreadCstack);

        break;
        
    case DLL_PROCESS_DETACH:
        
        //
        // Note that lpvReserved will be NULL if the detach is due to
        // a FreeLibrary() call, and non-NULL if the detach is due to
        // process cleanup.
        //

        if( lpvReserved == NULL ) {
            // Free up some resources.  This is like cleaning up your room
            // before the tornado strikes, but hey, it's good practice.
            TlsFree(TlsIndex);
            DeleteCriticalSection(&CrSec);
        
            if ((OutputStyle == FILE_ONLY) || (OutputStyle == FILE_AND_WINDOW)) {
                if (LogFileHandle!=INVALID_HANDLE_VALUE) {
                    CloseHandle(LogFileHandle);
                }
            }

            if (WindowThread!=NULL)
                CloseHandle(WindowThread);

            if ((lpfnCloseDesktop!=NULL) && (HDesk!=NULL)) {
                lpfnCloseDesktop (HDesk);
                HDesk = NULL;
            }
        }
          
        break;
        
    case DLL_THREAD_DETACH:
        
        // Get the pointer to this thread's Cstack, and delete the
        // object.
        ThreadCstack = (Cstack_c *)TlsGetValue(TlsIndex);
        delete ThreadCstack;

        break;
        
    default:

        break;
    } // switch (dwReason)

    return TRUE;
} // DllMain()


VOID
InitializeDll (
    VOID
    ) {
    PINITDATA  InitDataPtr;    // to pass to the window creation thread
    HMODULE hUser32;

    DWORD   sz;
    BOOLEAN outputStyleSet = FALSE;
    HKEY    hkeyAppData;
    INT     err;

    EnterCriticalSection (&CrSec);
    if (!DllInitialized) {
        DllInitialized = TRUE;
        if (RegOpenKeyEx (HKEY_LOCAL_MACHINE,
                "System\\CurrentControlSet\\Services\\WinSock2\\Parameters\\DT_DLL_App_Data",
                0,
                MAXIMUM_ALLOWED,
                &hkeyAppData
                )==NO_ERROR) {
            DWORD   value, sz, type;
            sz = sizeof (value);
            if ((RegQueryValueEx (
                            hkeyAppData,
                            ModuleFileName,
                            NULL,
                            &type,
                            (PUCHAR)&value,
                            &sz)==NO_ERROR)
                    && (type==REG_DWORD)
                    && (sz==sizeof(value)) ) {
                switch (value) {
                case FILE_ONLY:
                case FILE_AND_WINDOW:
                    strcpy (LogFileName, ModuleFileName);
                    strcat (LogFileName, ".log");
                case NO_OUTPUT:
                case WINDOW_ONLY:
                case DEBUGGER:
                    OutputStyle = value;
                    outputStyleSet = TRUE;
                    break;
                }
            }
            RegCloseKey (hkeyAppData);
        }

        hUser32 = GetModuleHandle ("user32.dll");
        if (hUser32!=NULL) {

            if (!outputStyleSet ||
                    (OutputStyle==FILE_AND_WINDOW) ||
                    (OutputStyle==WINDOW_ONLY)) {
                lpfnOpenInputDesktop = (LPFN_OPENINPUTDESKTOP)GetProcAddress (
                                                            hUser32,
                                                            "OpenInputDesktop");
                lpfnCloseDesktop = (LPFN_CLOSEDESKTOP)GetProcAddress (
                                                            hUser32,
                                                            "CloseDesktop");
                if ((lpfnOpenInputDesktop!=NULL)
                        && (lpfnCloseDesktop!=NULL)) {
                    HDesk = lpfnOpenInputDesktop (0, FALSE, MAXIMUM_ALLOWED);
                    if (HDesk!=NULL) {
                        if (GetThreadDesktop (GetCurrentThreadId ())==NULL) {
                            if (SetThreadDesktop (HDesk)) {
                                if (!outputStyleSet) {
                                    // Pop up a dialog box for the user to choose output method.
                                    OutputStyle = DialogBox(DllInstHandle, 
                                              MAKEINTRESOURCE(IDD_DIALOG1),
                                              NULL, 
                                              (DLGPROC)DebugDlgProc);
                                }
                            }
                            else {
                                err = GetLastError ();
                                lpfnCloseDesktop (HDesk);
                                HDesk = NULL;
                                //
                                // Can't set input desktop, force debugger output.
                                //
                                OutputStyle = NO_OUTPUT;
                                wsprintf (Buffer, "Could not set input desktop in the process (err: %ld),"
                                                   " setting output style to NO_OUTPUT.\n", err);
                                DTTextOut (NULL, NULL, Buffer, DEBUGGER);
                                wsprintf (Buffer, 
                                    "Edit '%s' value (REG_DWORD) under 'HKEY_LOCAL_MACHINE\\System\\CurrentControlSet\\Services\\WinSock2\\Parameters\\DT_DLL_App_Data' key"
                                    " in the registry to set output style to DEBUGGER(%ld) or FILE_ONLY(%ld).\n",
                                    ModuleFileName, DEBUGGER, FILE_ONLY);
                                DTTextOut (NULL, NULL, Buffer, DEBUGGER);
                            }
                        }
                        else {
                            lpfnCloseDesktop (HDesk);
                            HDesk = NULL;
                            if (!outputStyleSet) {
                                // Pop up a dialog box for the user to choose output method.
                                OutputStyle = DialogBox(DllInstHandle, 
                                          MAKEINTRESOURCE(IDD_DIALOG1),
                                          NULL, 
                                          (DLGPROC)DebugDlgProc);
                            }
                        }
                    }
                    else {
                        //
                        // No desktop, force debugger output.
                        //
                        err = GetLastError ();
                        OutputStyle = NO_OUTPUT;
                        wsprintf (Buffer, "Could not open input desktop in the process (err: %ld),"
                                           " setting output style to NO_OUTPUT.\n", err);
                        DTTextOut (NULL, NULL, Buffer, DEBUGGER);
                        wsprintf (Buffer, 
                            "Edit %s value under 'HKEY_LOCAL_MACHINE\\System\\CurrentControlSet\\Services\\WinSock2\\Parameters\\DT_DLL_App_Data' key"
                            " in the registry to set output style to DEBUGGER(%ld) or FILE_ONLY(%ld).\n",
                            ModuleFileName, DEBUGGER, FILE_ONLY);
                        DTTextOut (NULL, NULL, Buffer, DEBUGGER);
                    }
                }
                else {
                    lpfnCloseDesktop = NULL;
                    HDesk = NULL;
                    if (!outputStyleSet) {
                        // Pop up a dialog box for the user to choose output method.
                        OutputStyle = DialogBox(DllInstHandle, 
                                  MAKEINTRESOURCE(IDD_DIALOG1),
                                  NULL, 
                                  (DLGPROC)DebugDlgProc);
                    }
                }
            }

   
            if ((OutputStyle == FILE_ONLY) || (OutputStyle == FILE_AND_WINDOW)) {
        
                LogFileHandle = CreateFile(LogFileName, 
                                           GENERIC_WRITE, 
                                           FILE_SHARE_READ | FILE_SHARE_WRITE,
                                           NULL, 
                                           CREATE_ALWAYS,
                                           FILE_ATTRIBUTE_NORMAL, 
                                           NULL);
                if (LogFileHandle == INVALID_HANDLE_VALUE) {
                    if (OutputStyle==FILE_AND_WINDOW) {
                        DWORD   rc = GetLastError ();
                        if (FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM |
                                            FORMAT_MESSAGE_IGNORE_INSERTS,
                                            NULL,
                                            rc,
                                            0,
                                            Buffer,
                                            sizeof (Buffer),
                                            NULL )!=0) {
                        }
                        else {
                            wsprintf (Buffer, "Error %ld.", rc);
                        }
                        MessageBox(NULL, Buffer, "Creating log file", MB_OK | MB_ICONSTOP);
                    }
                    else {
                        OutputStyle = NO_OUTPUT;
                        wsprintf (Buffer, "Couldn't open log file %ls!"
                            "  No output will be generated.\n",
                            LogFileName);
                        DTTextOut (NULL, NULL, Buffer, DEBUGGER);
                    }
                }
            }
        
            // Get some information for later output to the debug window
            // or file -- get the time, PID, and TID of the calling
            // process and put into a INITDATA struct.  This memory will
            // be freed by the thread it is passed to.
            InitDataPtr = (PINITDATA) LocalAlloc(0, sizeof(INITDATA));
            GetLocalTime(&(InitDataPtr->LocalTime));
            InitDataPtr->TID = GetCurrentThreadId();
            InitDataPtr->PID = GetCurrentProcessId();
    
            // Create the initialization/window handling thread.
            if ((OutputStyle == WINDOW_ONLY) || (OutputStyle == FILE_AND_WINDOW)) {
                WindowThread =  
                  CreateThread(NULL, 
                               0,
                               (LPTHREAD_START_ROUTINE)WindowThreadFunc,
                               (LPVOID)InitDataPtr, 
                               0,
                               &WindowThreadId);
            } else {
        
                // Normally the window thread does a DTTextOut of the time
                // and process info that we saved just above.  But in this
                // case,  there is no window thread so spit it out to the
                // file or debugger. 

                wsprintf(Buffer, "Log initiated: %d-%d-%d, %d:%d:%d\r\n", 
                         InitDataPtr->LocalTime.wMonth, 
                         InitDataPtr->LocalTime.wDay, 
                         InitDataPtr->LocalTime.wYear, 
                         InitDataPtr->LocalTime.wHour, 
                         InitDataPtr->LocalTime.wMinute, 
                         InitDataPtr->LocalTime.wSecond);
                DTTextOut(DebugWindow, LogFileHandle, Buffer, OutputStyle);
                wsprintf(Buffer, "Process ID: 0x%X   Thread ID: 0x%X\r\n",
                         InitDataPtr->PID,
                         InitDataPtr->TID);
                DTTextOut(DebugWindow, LogFileHandle, Buffer, OutputStyle);

                // Setting this event allows {Pre|Post}ApiNotify to
                // proceed.  This event isn't really needed in this case
                // (because there is only one thread, and we know the code
                // above has been executed before WSAPre|PostApiNotify).
                SetEvent(TextOutEvent);
            }

        }
        else {
            DTTextOut (NULL, NULL, " Could not load user32.dll\n", DEBUGGER);
        }

    }
    LeaveCriticalSection (&CrSec);
}


BOOL WINAPIV
WSAPreApiNotify(
    IN  INT    NotificationCode,
    OUT LPVOID ReturnCode,
    IN  LPSTR  LibraryName,
    ...)
/*++
  
  Function Description:
  
      Builds a string for output and passes it, along with information
      about the call, to a handler function. 
  
  Arguments:
  
      NotificationCode -- specifies which API function called us.
  
      ReturnCode -- a generic pointer to the return value of the API
      function.  Can be used to change the return value in the
      case of a short-circuit (see how the return value from
      PreApiNotify works for more information on short-circuiting
      the API function).

      LibraryName -- a string pointing to the name of the library that
      called us.  
  
      ...    -- variable number argument list.  These are pointers
      to the actual parameters of the API functions.
  
  Return Value:
  
      Returns TRUE if we want to short-circuit the API function;
      in other words, returning non-zero here forces the API function
      to return immediately before any other actions take place.  
      
      Returns FALSE if we want to proceed with the API function.
  
--*/
{
    va_list          vl;            // used for variable arg-list parsing
    Cstack_c         *ThreadCstack; // the Cstack_c object for this thread
    int              Index = 0;     // index into string we are creating
    BOOL             ReturnValue;   // value to return
    LPFNDTHANDLER    HdlFunc;       // pointer to handler function
    int              Counter;       // counter popped off the cstack
    int              OriginalError; // any pending error is saved

    if (!DllInitialized) {
        InitializeDll ();
    }

    if (OutputStyle==NO_OUTPUT)
        return FALSE;
    
    OriginalError = GetLastError();

    EnterCriticalSection(&CrSec);


    // Wait until the debug window is ready to receive text for output.
    WaitForSingleObject(TextOutEvent, INFINITE);
    va_start(vl, LibraryName);
    
    // Get the Cstack_c object for this thread.
    ThreadCstack = (Cstack_c *)TlsGetValue(TlsIndex);
    if (!ThreadCstack){
        ThreadCstack = new Cstack_c();
        TlsSetValue(TlsIndex, (LPVOID)ThreadCstack);
        wsprintf(Buffer, "0x%X Foriegn thread\n",
                 GetCurrentThreadId());
        DTTextOut(DebugWindow, LogFileHandle, Buffer, OutputStyle);
    } //if
        
    // Start building an output string with some info that's
    // independent of which API function called us.
    Index = wsprintf(Buffer, "Function call: %d   ", 
                      ThreadCstack->CGetCounter());
            
    // Push the counter & increment.
    ThreadCstack->CPush();

    // Reset the error to what it was when the function started.
    SetLastError(OriginalError);

    // Call the appropriate handling function, output the buffer.
    if ((NotificationCode < MAX_DTCODE) && HdlFuncTable[NotificationCode]) {
        HdlFunc = HdlFuncTable[NotificationCode];
        ReturnValue = (*HdlFunc)(vl, ReturnCode, 
                                 LibraryName, 
                                 Buffer, 
                                 Index,
                                 TEXT_LEN,
                                 TRUE);

    } else {

        wsprintf(Buffer + Index, "Unknown function called!\r\n");
        DTTextOut(DebugWindow, LogFileHandle, Buffer, OutputStyle);
        ReturnValue = FALSE;
    }
    
    // If we are returning TRUE, then the API/SPI function will be
    // short-circuited.  We must pop the thread stack, since no
    // corresponding WSAPostApiNotify will be called.
    if (ReturnValue) {
        ThreadCstack->CPop(Counter);
    }

    LeaveCriticalSection(&CrSec);
    // In case the error has changed since the handler returned, we
    // want to set it back.
    SetLastError(OriginalError);
    return(ReturnValue);

} // WSAPreApiNotify()






BOOL WINAPIV
WSAPostApiNotify(
    IN  INT    NotificationCode,
    OUT LPVOID ReturnCode,
    IN  LPSTR  LibraryName,
    ...)
/*++
  
  PostApiNotify()
  
  Function Description:
  
      Like PreApiNotify, builds a string and passes it, along with
      information about the call, to a handler function. 
  
  Arguments:
  
      NotificationCode  -- specifies which API function called us.
  
      ReturnCode -- a generic pointer to the return value of the API
      function.  
  
      ...    -- variable number argument list.  These are pointers
      to the actual parameters of the API functions.
  
  Return Value:
  
      Returns value is currently meaningless.
  
--*/
{
    va_list          vl;            // used for variable arg-list parsing
    Cstack_c         *ThreadCstack; // the Cstack_c object for this thread
    int              Index = 0;     // index into string we are creating
    int              Counter;       // counter we pop off the cstack
    LPFNDTHANDLER    HdlFunc;       // pointer to handler function
    int              OriginalError; // any pending error is saved

    if (!DllInitialized) {
        InitializeDll ();
    }
    if (OutputStyle==NO_OUTPUT)
        return FALSE;

    OriginalError = GetLastError();

    EnterCriticalSection(&CrSec);


    // Wait until it's ok to send output.
    WaitForSingleObject(TextOutEvent, INFINITE);
	
	va_start(vl, LibraryName);

	// Get the cstack object from TLS, pop the Counter.
    ThreadCstack = (Cstack_c *) TlsGetValue(TlsIndex);
    
    if (!ThreadCstack){
        ThreadCstack = new Cstack_c();
        TlsSetValue(TlsIndex, (LPVOID)ThreadCstack);
        wsprintf(Buffer, "0x%X Foriegn thread\n",
                 GetCurrentThreadId());
        DTTextOut(DebugWindow, LogFileHandle, Buffer, OutputStyle);
    } //if
    
    ThreadCstack->CPop(Counter);
    
    Index = wsprintf(Buffer, "Function Call: %d   ", Counter);
    
    // Set the error to what it originally was.
    SetLastError(OriginalError);

    // Call the appropriate handling function, output the buffer.
    if ((NotificationCode < MAX_DTCODE) && HdlFuncTable[NotificationCode]) {
        HdlFunc = HdlFuncTable[NotificationCode];
        (*HdlFunc)(vl, ReturnCode, 
                   LibraryName, 
                   Buffer, 
                   Index,
                   TEXT_LEN,
                   FALSE);
    } else {

        wsprintf(Buffer + Index, "Unknown function returned!\r\n");
        DTTextOut(DebugWindow, LogFileHandle, Buffer, OutputStyle);
    }
    
    LeaveCriticalSection(&CrSec);
    // In case the error has changed since the handler returned, we
    // want to set it back.
    SetLastError(OriginalError);
    return(FALSE);

} // WSAPostApiNotify()





LRESULT APIENTRY 
DTMainWndProc(
    IN HWND   WindowHandle, 
    IN UINT   Message, 
    IN WPARAM WParam, 
    IN LPARAM LParam)
/*++
  
  DTMainWndProc()
  
  Function Description:
  
      Window procedure for the main window of the Dll.  This function
      processes WM_CREATE messages in order to create a child
      edit control, which does most of the dirty work.  Also processes
      WM_COMMAND to trap notification messages from the edit control,
      as well as WM_SIZE and WM_DESTROY messages.
  
  Arguments:
  
      WindowHandle -- the window.
      
      Message -- the message.
      
      WParam -- first parameter.
      
      LParam -- second parameter.
  
  Return Value:
  
      Message dependent.
      
--*/
{
    
    HFONT      FixedFontHandle;   // self-explanatory
    RECT       Rect;              // specifies client area of frame window
    DWORD      CharIndex1;
    DWORD      CharIndex2;
    DWORD      LineIndex;         // indices into edit control text
    char       NullString[] = ""; // self-explanatory
    DWORD      OldOutputStyle;    // temporary storage for OutputStyle
                
    switch (Message) {
        
    case WM_CREATE:
        
        // Create the debug window as a multiline edit control.  
        GetClientRect(WindowHandle, &Rect);
        DebugWindow = CreateWindow("EDIT", 
                                   NULL,
                                   WS_CHILD | WS_VISIBLE |
                                   WS_VSCROLL | ES_LEFT  | 
                                   ES_MULTILINE | ES_AUTOVSCROLL,
                                   0,
                                   0,
                                   Rect.right,
                                   Rect.bottom,
                                   WindowHandle,
                                   (HMENU)EC_CHILD,
                                   DllInstHandle,
                                   NULL);
        
        // Subclass the edit control's window procedure to be
        // DTEditWndProc.
        EditWndProc = (WNDPROC) SetWindowLongPtr (DebugWindow,
                                              GWLP_WNDPROC,
                                              (UINT_PTR)DTEditWndProc);

        // Set the edit control's text size to the maximum.
        SendMessage(DebugWindow, EM_LIMITTEXT, 0, 0);
        
        // Set the edit control's font
        FixedFontHandle = (HFONT)GetStockObject(ANSI_FIXED_FONT);
        SendMessage(DebugWindow, WM_SETFONT, (WPARAM)FixedFontHandle, 
                    MAKELPARAM(TRUE, 0));

        return(0);

    case WM_COMMAND:

        if (LOWORD(WParam) == EC_CHILD) {

            // The notification is coming from the edit-control child.
            // Determine which notification it is and act appropriately.

            switch (HIWORD(WParam)) {

            case EN_ERRSPACE:
                
                // Flow through

            case EN_MAXTEXT:

                // There's too much text in the edit control.  This is
                // a hack to eliminate approximately the first half of
                // the text, so we can then add more...
                CharIndex1 = GetWindowTextLength(DebugWindow) / 2;
                LineIndex = (DWORD)SendMessage(DebugWindow, EM_LINEFROMCHAR,
                                        (WPARAM)CharIndex1, 0);
                CharIndex2 = (DWORD)SendMessage(DebugWindow, EM_LINEINDEX,
                                         (WPARAM)LineIndex, 0);
                
                SendMessage(DebugWindow, EM_SETSEL, 0, CharIndex2);
                SendMessage(DebugWindow, EM_REPLACESEL, 0, 
                            (LPARAM)NullString);
                
                // send this text to the window only...
                OldOutputStyle = OutputStyle;
                OutputStyle = WINDOW_ONLY;
                DTTextOut(DebugWindow, LogFileHandle, 
                          "----Buffer Overflow...Resetting----\r\n",
                          OutputStyle);
                OutputStyle = OldOutputStyle;
                break;

            case EN_CHANGE:
            case EN_UPDATE:
                
                // Ignore these notification codes
                return 0;
                break;
                
            default:
                
                // Let the default window procedure handle it.
                return DefWindowProc(WindowHandle, Message, WParam,
                                 LParam);
            } // switch (HIWORD(WParam))

        } // if (LOWORD(WParam) == EC_CHILD)
        else {
            
            // The notification is coming from somewhere else!!!
            return DefWindowProc(WindowHandle, Message, WParam,
                                 LParam);
        }

        return(0);
        break;

    case WM_DESTROY:
        
        PostQuitMessage(0);
        return(0);

    case WM_SIZE:
        
        
        // Make the edit control the size of the window's client area. 
        MoveWindow(DebugWindow, 0, 0, LOWORD(LParam), HIWORD(LParam), TRUE);
        return(0);        
        
    default:
        
        // All other messages are taken care of by the default.
        return(DefWindowProc(WindowHandle, Message, WParam, LParam));
        
    } // switch

} // DTMainWndProc()





LRESULT APIENTRY 
DTEditWndProc(
    IN HWND   WindowHandle, 
    IN UINT   Message, 
    IN WPARAM WParam, 
    IN LPARAM LParam)
/*++
  
  DTEditWndProc()
  
  Function Description:
  
      Subclassed window procedure for the debug window.  This function
      disables some edit control functionality, and also responds to a
      user-defined message to print out text in the window.
  
  Arguments:
  
      WindowHandle -- the window.
  
      Message -- the message.
      
      WParam -- first parameter.
      
      LParam -- second parameter.
  
  Return Value:
  
      Message dependent.
  
--*/
{
    switch (Message) {
        
    case WM_CHAR:     

        // Handle control-c so that copy works.  Sorry about the magic
        // number! 
        if (WParam == 3) {
            return (CallWindowProc(EditWndProc, WindowHandle, Message,
                                   WParam, LParam));
        } // else flows through
        
    case WM_KEYDOWN:    // Flow through
    case WM_UNDO:       // Flow through
    case WM_PASTE:      // Flow through
    case WM_CUT:
        
        return (0);     // Effectively disables the above messages
                
    default:

        return (CallWindowProc(EditWndProc, WindowHandle, Message,
                               WParam, LParam));
    } // switch

} // DTEditWndProc()





DWORD
WindowThreadFunc(
    LPDWORD TheParam)
/*++
  
  WindowThreadFunc()
  
  Function Description:
  
      Thread function for WindowThread created in DllMain during
      process attachment.  Registers a window class, creates an
      instance of that class, and goes into a message loop to retrieve
      messages for that window or it's child edit control.
  
  Arguments:
  
      TheParam -- Pointer to the parameter passed in by the function
      that called CreateThread.
  
  Return Value:
  
      Returns the wParam of the quit message that forced us out of the
      message loop.
  
--*/
{
    
    WNDCLASS  wnd_class;    // window class structure to register
    MSG       msg;          // retrieved message
    PINITDATA InitDataPtr;  // casts TheParam into a INITDATA pointer

    if ((HDesk!=NULL) && (HDesk!=GetThreadDesktop (GetCurrentThreadId ()))) {
        SetThreadDesktop (HDesk);
    }
        
    // Register a window class for the frame window.
    wnd_class.style         = CS_HREDRAW | CS_VREDRAW;
    wnd_class.lpfnWndProc   = DTMainWndProc;
    wnd_class.cbClsExtra    = 0;
    wnd_class.cbWndExtra    = 0;
    wnd_class.hInstance     = DllInstHandle;
    wnd_class.hIcon         = LoadIcon(NULL, IDI_APPLICATION);
    wnd_class.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wnd_class.hbrBackground = (HBRUSH)(COLOR_APPWORKSPACE + 1);
    wnd_class.lpszMenuName  = NULL;
    wnd_class.lpszClassName = DTWndClass;
    RegisterClass(&wnd_class);

    wsprintf (Buffer, "%s - DT_DLL", ModuleFileName);
    
    // Create a frame window
    FrameWindow = CreateWindow(DTWndClass,
                               Buffer,
                               WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN |
                               WS_VISIBLE,
                               CW_USEDEFAULT,
                               CW_USEDEFAULT,
                               CW_USEDEFAULT,
                               CW_USEDEFAULT,
                               NULL,
                               NULL,
                               DllInstHandle,
                               NULL);

    // Send the initialization data to the debug window and/or file.
    InitDataPtr = (PINITDATA)TheParam;
    wsprintf(Buffer, "Log initiated: %d-%d-%d, %d:%d:%d\r\n", 
             InitDataPtr->LocalTime.wMonth, 
             InitDataPtr->LocalTime.wDay, 
             InitDataPtr->LocalTime.wYear, 
             InitDataPtr->LocalTime.wHour, 
             InitDataPtr->LocalTime.wMinute, 
             InitDataPtr->LocalTime.wSecond);
    DTTextOut(DebugWindow, LogFileHandle, Buffer, OutputStyle);
    wsprintf(Buffer, "Process ID: 0x%X   Thread ID: 0x%X\r\n",
             InitDataPtr->PID,
             InitDataPtr->TID);
    DTTextOut(DebugWindow, LogFileHandle, Buffer, OutputStyle);
    LocalFree(InitDataPtr);

    // Setting this event allows {Pre|Post}ApiNotify to proceed.  This
    // insures (ensures?  what's the difference) that any debugging
    // output by other threads is held up until after this statement.
    SetEvent(TextOutEvent);
    
    // Go into a message loop.
    while (GetMessage(&msg, NULL, 0 , 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    
    return((DWORD)msg.wParam);

} // WindowThreadFunc()





BOOL APIENTRY 
DebugDlgProc(
    HWND DialogWindow,
    UINT Message,
    WPARAM WParam,
    LPARAM LParam)
/*++
  
  DebugDlgProc()
  
  Function Description:
  
      Window function for the dialog box IDC_DIALOG1, the dialog box
      that pops up when the dll is loaded and prompts the user for the
      output style of his/her choice.
  
  Arguments:
  
      DialogWindow -- handle to the dialog box window.

      Message -- the message being received.

      WParam -- first parameter.

      LParam -- second parameter.
  
  Return Value:
  
      Returns TRUE to indicate message was handled, FALSE otherwise.
  
--*/
{

    DWORD LogFNSize = sizeof(LogFileName); // size of the file name buffer 
    DWORD disposition;
    DWORD outputStyle = OutputStyle;
        
    switch (Message) {

    case WM_COMMAND:

        switch (LOWORD(WParam)) {

        case IDOK:
            
            // The user clicked the OK button...figure out his choice
            // and act appropriately.
            if (IsDlgButtonChecked(DialogWindow, IDC_RADIO4)) {

                outputStyle = NO_OUTPUT;
          
            }
            else if (IsDlgButtonChecked(DialogWindow, IDC_RADIO5)) {

                // Radio Button 1 was clicked.
                if (!GetFile(DialogWindow, LogFileName, LogFNSize)) {
                    TCHAR   strBuf[1024];
                    GetComdlgErrMsg (strBuf, ErrStr2);
                    outputStyle = WINDOW_ONLY;
                    // Error -- OutputStyle stays WINDOW_ONLY.
                    MessageBox(DialogWindow, strBuf, "Error.",
                               MB_OK | MB_ICONSTOP);
                } else {
                    outputStyle = FILE_ONLY;
                }
            
            } else if (IsDlgButtonChecked(DialogWindow, IDC_RADIO6)) {
                
                // Radio Button 2 was clicked.
                outputStyle = WINDOW_ONLY;

            } else if (IsDlgButtonChecked(DialogWindow, IDC_RADIO7)) {

                // Radio Button 3 was clicked.
                if (!GetFile(DialogWindow, LogFileName, LogFNSize)) {
                    TCHAR   strBuf[1024];
                    GetComdlgErrMsg (strBuf, ErrStr2);
                    outputStyle = WINDOW_ONLY;
                    // Error -- OutputStyle stays WINDOW_ONLY.
                    MessageBox(DialogWindow, strBuf, "Error", 
                               MB_OK | MB_ICONSTOP);
                } else {
                    outputStyle = FILE_AND_WINDOW;
                }
                
            } else if (IsDlgButtonChecked(DialogWindow, IDC_RADIO8)) {

                // Radio Button 4 was clicked.
                outputStyle = DEBUGGER;

            } else {
                
                // No radio buttons were clicked -- pop up a Message
                // box.
                MessageBox(DialogWindow, "You must choose one output method.",
                           "Choose or Die.", MB_OK | MB_ICONSTOP);
                break;

            }

            //
            // Store setting to the registry if requested.
            //
            if (IsDlgButtonChecked (DialogWindow, IDC_CHECK)) {
                DWORD rc, disposition;
                HKEY    hkeyAppData;
                rc = RegCreateKeyEx(HKEY_LOCAL_MACHINE,
                    "System\\CurrentControlSet\\Services\\WinSock2\\Parameters\\DT_DLL_App_Data",
                    0,
                    NULL,
                    REG_OPTION_NON_VOLATILE,
                    KEY_SET_VALUE,
                    NULL,
                    &hkeyAppData,
                    &disposition
                    );
                if (rc==NO_ERROR) {
                    rc = RegSetValueEx (hkeyAppData,
                                ModuleFileName,
                                0,
                                REG_DWORD,
                                (BYTE *)&outputStyle,
                                sizeof (outputStyle)
                                );
                    RegCloseKey (hkeyAppData);
                }

                if (rc!=NO_ERROR) {
                    if (FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM |
                                        FORMAT_MESSAGE_IGNORE_INSERTS,
                                        NULL,
                                        rc,
                                        0,
                                        Buffer,
                                        sizeof (Buffer),
                                        NULL )!=0) {
                    }
                    else {
                        wsprintf (Buffer, "Error %ld.", rc);
                    }
                    MessageBox( NULL, Buffer, "Storing application settings", 
                                    MB_OK | MB_ICONINFORMATION );
                }
            }

            // flow through

        case IDCANCEL:
            
            EndDialog(DialogWindow, (int)outputStyle);
            return TRUE;
            
        }

    case WM_INITDIALOG:
        wsprintf (Buffer, "%s - DT_DLL", ModuleFileName);
        SetWindowText (DialogWindow, Buffer);

        return TRUE;
        
    }
    return FALSE;

} // DebugDlgProc()





BOOL
DTTextOut(
    IN HWND   WindowHandle,
    IN HANDLE FileHandle,
    IN char   *String,
    DWORD     Style)
/*++
  
  DTTextOut()
  
  Function Description:
  
      This function outputs a string to a debug window and/or file.
        
  Arguments:
  
      WindowHandle -- handle to an edit control for debug output.

      FileHandle -- handle to an open file for debug output.

      String -- the string to output.

      Style -- specifies whether the output should go to the window,
      the file, or both.
  
  Return Value:
  
      Returns TRUE if the output succeeds, FALSE otherwise.
        
--*/
{

    DWORD NumWritten;           // WriteFile takes an address to this
    DWORD Index;                // index of end of edit control text
    BOOL  Result;               // result of WriteFile
    char  Output[TEXT_LEN+60];     // scratch buffer 

    static DWORD LineCount = 0; // text output line number
    DWORD  BufIndex = 0;        // index into output string

    if (Style==NO_OUTPUT)
        return TRUE;

    // Build a new string with the line-number and pid.tid in front.
    if (Style==DEBUGGER)
        BufIndex += wsprintf(Output, "DT_DLL(%d-%X.%X @%8.8lX) ", LineCount++,
                                        GetCurrentProcessId (),
                                        GetCurrentThreadId (),
                                        GetTickCount ());
    else
        BufIndex += wsprintf(Output, "(%d-%X.%X @%8.8lX) ", LineCount++,
                                        GetCurrentProcessId (),
                                        GetCurrentThreadId (),
                                        GetTickCount ());
    strcpy(Output + BufIndex, String);
    
    switch (Style) {

    case WINDOW_ONLY:
        
        Index = GetWindowTextLength(WindowHandle);
        SendMessage(WindowHandle, EM_SETSEL, Index, Index);
        SendMessage(WindowHandle, EM_REPLACESEL, 0, (LPARAM)Output);
        
        break;
        
    case FILE_ONLY:
        
        Result = WriteFile(FileHandle, (LPCVOID)Output, strlen(Output), 
                           &NumWritten, NULL);
        if (!Result) {
            
            AbortAndClose(FileHandle, WindowHandle);
            return FALSE;
        }
        break;

    case FILE_AND_WINDOW:
        
        Index = GetWindowTextLength(WindowHandle);
        SendMessage(WindowHandle, EM_SETSEL, Index, Index);
        SendMessage(WindowHandle, EM_REPLACESEL, 0, (LPARAM)Output);
        Result = WriteFile(FileHandle, (LPCVOID)Output, strlen(Output), 
                           &NumWritten, NULL);
        if (!Result) {
            
            AbortAndClose(FileHandle, WindowHandle);
            return FALSE;
        }
        break;

    case DEBUGGER:

        OutputDebugString(Output);
    }
    return TRUE;

} // DTTextOut()





void
AbortAndClose(
    IN HANDLE FileHandle,
    IN HWND WindowHandle)
/*++
  
  AbortAndClose()
  
  Function Description:
  
      Closes a file handle, informs the user via a message box, and
      changes the global variable OutputStyle to WINDOW_ONLY
        
  Arguments:
  
      FileHandle -- handle to a file that caused the error.
      
      WindowHandle -- handle to a window to be the parent of the
      Message Box.

  Return Value:
  
      Void.
        
--*/
{
    CloseHandle(FileHandle);
    MessageBox(WindowHandle, ErrStr3, "Error", MB_OK | MB_ICONSTOP);
    OutputStyle = DEBUGGER;

} // AbortAndClose()





BOOL
GetFile(
    IN  HWND   OwnerWindow, 
    OUT LPSTR  FileName, 
    IN  DWORD  FileNameSize)
/*++
  
  GetFile()
  
  Function Description:
  
      Uses the predefined "Save As" dialog box style to retrieve a
      file name from the user.  The file name the user selects is
      stored in LogFileName.
        
  Arguments:
  
      OwnerWindow -- window which will own the dialog box.
      
      FileName -- address of a buffer in which to store the string.

      FileNameSize -- size of the FileName buffer.

  Return Value:
  
      Returns whatever GetSaveFileName returns; see documentation for
      that function.
        
--*/
{
    
    OPENFILENAME OpenFileName;  // common dialog box structure
    char DirName[256];          // directory string 
    char FileTitle[256];        // file-title string
            
    FileName[0] = '\0';
    
    FillMemory((PVOID)&OpenFileName, sizeof(OPENFILENAME), 0);
    
    // Retrieve the system directory name and store it in DirName.
    GetCurrentDirectory(sizeof(DirName), DirName);

    // Set the members of the OPENFILENAME structure.
#if (_WIN32_WINNT >= 0x0500)
    if (LOBYTE(LOWORD(GetVersion()))<5)
        OpenFileName.lStructSize = OPENFILENAME_SIZE_VERSION_400;
    else
#endif
        OpenFileName.lStructSize = sizeof (OpenFileName);
    OpenFileName.hwndOwner = OwnerWindow;
    OpenFileName.lpstrFilter = OpenFileName.lpstrCustomFilter = NULL;    
    OpenFileName.nFilterIndex = 0;
    OpenFileName.lpstrFile = FileName;
    OpenFileName.nMaxFile = FileNameSize;
    OpenFileName.lpstrFileTitle = FileTitle;
    OpenFileName.nMaxFileTitle = sizeof(FileTitle);
    OpenFileName.lpstrInitialDir = DirName;
    OpenFileName.Flags = OFN_SHOWHELP | OFN_OVERWRITEPROMPT;
    
    // Pop up the dialog box to get the file name.
    return GetSaveFileName(&OpenFileName);
        
} // GetFile()

VOID
GetComdlgErrMsg (
    LPTSTR  Buffer,
    LPTSTR  Fmt
    )
{
    LPTSTR  str;
    DWORD   err;
    TCHAR   numBuf[256];

    switch (err=CommDlgExtendedError ()) {
    case 0:
        str = sCDERR_USERCANCEL;
        break;
    case CDERR_FINDRESFAILURE:
        str = sCDERR_FINDRESFAILURE;
        break;
    case CDERR_INITIALIZATION:
        str = sCDERR_INITIALIZATION;
        break;
    case CDERR_LOCKRESFAILURE:
        str = sCDERR_LOCKRESFAILURE;
        break;
    case CDERR_LOADRESFAILURE:
        str = sCDERR_LOADRESFAILURE;
        break;
    case CDERR_LOADSTRFAILURE:
        str = sCDERR_LOADSTRFAILURE;
        break;
    case CDERR_MEMALLOCFAILURE:
        str = sCDERR_MEMALLOCFAILURE;
        break;
    case CDERR_MEMLOCKFAILURE:
        str = sCDERR_MEMLOCKFAILURE;
        break;
    case CDERR_NOHINSTANCE:
        str = sCDERR_NOHINSTANCE;
        break;
    case CDERR_NOHOOK:
        str = sCDERR_NOHOOK;
        break;
    case CDERR_NOTEMPLATE:
        str = sCDERR_NOTEMPLATE;
        break;
    case CDERR_STRUCTSIZE:
        str = sCDERR_STRUCTSIZE;
        break;
    case FNERR_BUFFERTOOSMALL:
        str = sFNERR_BUFFERTOOSMALL;
        break;
    case FNERR_INVALIDFILENAME:
        str = sFNERR_INVALIDFILENAME;
        break;
    case FNERR_SUBCLASSFAILURE:
        str = sFNERR_SUBCLASSFAILURE;
        break;
    default:
        wsprintf (numBuf, "Unknown common dialog error: %ld", err);
        str = numBuf;
        break;
    }
    wsprintf (Buffer, Fmt, str);
}