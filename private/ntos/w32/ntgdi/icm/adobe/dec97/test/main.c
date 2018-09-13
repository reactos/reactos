
/******************************************************************************\
*       This is a part of the Microsoft Source Code Samples. 
*       Copyright (C) 1993 Microsoft Corporation.
*       All rights reserved. 
*       This source code is only intended as a supplement to 
*       Microsoft Development Tools and/or WinHelp documentation.
*       See these sources for detailed information regarding the 
*       Microsoft samples programs.
\******************************************************************************/

/****************************************************************************
*
*
*    PROGRAM: CmnDlg.c
*
*    PURPOSE: Sample demonstrating the use of the common dialogs in Windows
*
*    FUNCTIONS:
*
*        WinMain() - calls initialization function, processes message loop
*        InitApplication() - initializes window data and registers window
*        InitInstance() - saves instance handle and creates main window
*        MainWndProc() - processes messages
*        About() - processes messages for "About" dialog box
*        OpenNewFile() - opens a new file
*        SaveToFile() - saves the current text buffer to the current filename
*        SaveAs() - saves the current text buffer to a new file name
*        EnterNew() - to enter new text into the text buffer
*        FileOpenHookProc() - Hook procedure for GetOpenFileName() common dialog
*        FileSaveHookProc() - Hook procedure for GetSaveFileName() common dialog
*        ChooseFontHookProc() - Hook procedure for ChooseFont() common dialog
*        FindTextHookProc() - Hook procedure for FindText() common dialog
*        ReplaceTextHookProc() - Hook procedure for the ReplaceText() common dialog
*        PrintDlgHookProc() - Hook procedure for the PrintDlg() common dialog
*        PrintSetupHookProc() - Hook procedure for the PrintDlg() setup common dialog
*        SearchFile() - Searches for the specified text in the file buffer
*        ChooseNewFont() - chooses a new font for display
*        ChooseNewColor() - chooses a new color for display
*        PrintFile() - prints the current text in the file buffer
*        CallFindText() - calls the FindText() common dialog function
*        CallReplaceText() - calls the ReplaceText() common dialog function
*        ProcessCDError() - uses CommonDialogExtendedError() to output useful error messages
*
*    COMMENTS:
*
*
*        The common dialog APIs demonstrated in the sample include:
*
*            ChooseColor()
*            ChooseFont()
*            FindText()
*            GetOpenFileName()
*            GetSaveFileName()
*            PrintDlg()
*            ReplaceText()
*
*
*        Each dialog box is demonstrated being used in three different ways:
*        standard, using a modified template and using a hook function.
*
*
****************************************************************************/

#include <windows.h>    // includes basic windows functionality
#include <commdlg.h>    // includes common dialog functionality
#include <dlgs.h>       // includes common dialog template defines
#include <stdio.h>      // includes standard file i/o functionality
#include <string.h>     // includes string functions
#include <cderr.h>      // includes the common dialog error codes
#include "main.h"       // includes my common dialog functions
#include "resource.h"

HANDLE       hInst;
OPENFILENAME OpenFileName;
CHAR         szDirName[256]   = "";
CHAR         szFile[256]      = "\0";
CHAR         szFileTitle[256];

// Filter specification for the OPENFILENAME struct
// This is portable for i386 and MIPS
// Leaving out the \0 terminator will cause improper DWORD alignment
// and cause a failure under MIPS
CHAR szFilter[] = "Text Files (*.ICM)\0*.ICM\0All Files (*.*)\0*.*\0";
CHAR szSaveFilter[] = "Text Files (*.CSA)\0*.CSA\0All Files (*.*)\0*.*\0";

CHAR         FileBuf[FILE_LEN];
DWORD        dwFileSize;
UINT         FindReplaceMsg;
CHAR         szFindString[64]   = "";
CHAR         szReplaceString[64]   = "";
FINDREPLACE  frText;
LPFINDREPLACE lpFR;
CHAR *       lpBufPtr = FileBuf;
CHOOSEFONT   chf;
CHOOSECOLOR  chsclr;
COLORREF     crColor;
LOGFONT      lf;
WORD         wMode = IDM_CUSTOM;
WORD         wAsciiMode = IDM_ASCII;
WORD         wIntentMode = IDM_PERCEPUAL;
WORD         wCSAMode = IDM_AUTO;
WORD         wInpDrvClrSp = IDM_INP_AUTO;
WORD         wCSAorCRD = IDM_CSA;
HWND         hDlgFR = NULL;
PRINTDLG     pd;

BOOL         AllowBinary = FALSE;
DWORD        Intent = 0;
HWND         hWnd; 

BOOL WINAPI OpenFiles(
        HWND hDlg,                /* window handle of the dialog box */
        UINT message,             /* type of message                 */
        WPARAM wParam,            /* message-specific information    */
        LPARAM lParam);

/****************************************************************************
*
*    FUNCTION: WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
*
*    PURPOSE: calls initialization function, processes message loop
*
*    COMMENTS:
*
*
****************************************************************************/

int PASCAL WinMain(
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPSTR lpCmdLine,
    int nCmdShow
    )
{

    MSG msg;                         /* message                      */

    if (!hPrevInstance)                  /* Other instances of app running? */
        if (!InitApplication(hInstance)) /* Initialize shared things */
            return (FALSE);              /* Exits if unable to initialize     */

    hInst = hInstance;

    /* Perform initializations that apply to a specific instance */

    if (!InitInstance(hInstance, nCmdShow))
        return (FALSE);

    // register window message for FindText() and ReplaceText() hook procs
    FindReplaceMsg = RegisterWindowMessage( (LPSTR) FINDMSGSTRING );

    /* Acquire and dispatch messages until a WM_QUIT message is received. */

    while (GetMessage(&msg,        /* message structure                      */
            NULL,                  /* handle of window receiving the message */
            0,                     /* lowest message to examine              */
            0))                    /* highest message to examine             */
        if ( !hDlgFR || !IsWindow(hDlgFR) || !IsDialogMessage( hDlgFR, &msg ) )
            {
            TranslateMessage(&msg);    /* Translates virtual key codes */
            DispatchMessage(&msg);     /* Dispatches message to window */
            }
    return (msg.wParam);           /* Returns the value from PostQuitMessage */

    // avoid compiler warnings at W3
    lpCmdLine;
}


/****************************************************************************
*
*    FUNCTION: InitApplication(HANDLE)
*
*    PURPOSE: Initializes window data and registers window class
*
*    COMMENTS:
*
*        In this function, we initialize a window class by filling out a data
*        structure of type WNDCLASS and calling the Windows RegisterClass()
*        function.
*
****************************************************************************/

BOOL InitApplication(HANDLE hInstance)       /* current instance             */
{
    WNDCLASS  wc;

    /* Fill in window class structure with parameters that describe the       */
    /* main window.                                                           */

    wc.style = 0;                       /* Class style(s).                    */
    wc.lpfnWndProc = (WNDPROC)MainWndProc;       /* Function to retrieve messages for  */
                                        /* windows of this class.             */
    wc.cbClsExtra = 0;                  /* No per-class extra data.           */
    wc.cbWndExtra = 0;                  /* No per-window extra data.          */
    wc.hInstance = hInstance;           /* Application that owns the class.   */
    wc.hIcon = LoadIcon(NULL, IDI_ICON1);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = GetStockObject(WHITE_BRUSH); 
    wc.lpszMenuName =  "CmnDlgMenu";   /* Name of menu resource in .RC file. */
    wc.lpszClassName = "CmnDlgWClass"; /* Name used in call to CreateWindow. */

    /* Register the window class and return success/failure code. */

    return (RegisterClass(&wc));

}


/****************************************************************************
*
*    FUNCTION:  InitInstance(HANDLE, int)
*
*    PURPOSE:  Saves instance handle and creates main window
*
*    COMMENTS:
*
*        In this function, we save the instance handle in a static variable and
*        create and display the main program window.
*
****************************************************************************/

BOOL InitInstance(
    HANDLE          hInstance,          /* Current instance identifier.       */
    int             nCmdShow)           /* Param for first ShowWindow() call. */
{
    HWND            hWND;               /* Main window handle.                */

    /* Save the instance handle in static variable, which will be used in  */
    /* many subsequence calls from this application to Windows.            */

    hInst = hInstance;

    /* Create a main window for this application instance.  */

    hWND = CreateWindow(
        "CmnDlgWClass",                 /* See RegisterClass() call.          */
        "CIEBASED_CDEF Color Space",    /* Text for window title bar.         */
        WS_OVERLAPPEDWINDOW,            /* Window style.                      */
        CW_USEDEFAULT,                  /* Default horizontal position.       */
        CW_USEDEFAULT,                  /* Default vertical position.         */
        CW_USEDEFAULT,                  /* Default width.                     */
        CW_USEDEFAULT,                  /* Default height.                    */
        NULL,                           /* Overlapped windows have no parent. */
        NULL,                           /* Use the window class menu.         */
        hInstance,                      /* This instance owns this window.    */
        NULL                            /* Pointer not needed.                */
    );

    /* If window could not be created, return "failure" */

    if (!hWND)
        return (FALSE);
     
    hWnd = hWND;
    /* Make the window visible; update its client area; and return "success" */

    ShowWindow(hWnd, nCmdShow);  /* Show the window                        */
    UpdateWindow(hWnd);          /* Sends WM_PAINT message                 */
    return (TRUE);               /* Returns the value from PostQuitMessage */

}

/****************************************************************************
*
*    FUNCTION: MainWndProc(HWND, UINT, WPARAM, LPARAM)
*
*    PURPOSE:  Processes messages
*
*    COMMENTS:
*
*        This function processes all messags sent to the window.  When the
*        user choses one of the options from one of the menus, the command
*        is processed here and passed onto the function for that command.
*        This function also processes the special "FindReplace" message that
*        this application registers for hook processing of the FindText()
*        and ReplaceText() common dialog functions.
*
****************************************************************************/

LRESULT CALLBACK MainWndProc(
        HWND hWnd,                /* window handle                   */
        UINT message,             /* type of message                 */
        WPARAM wParam,            /* additional information          */
        LPARAM lParam)            /* additional information          */
{
    FARPROC lpProcAbout;          /* pointer to the "About" function */
    FARPROC lpProcOpen;
    FARPROC lpProcEnterNew;       /* pointer to the "EnterNew" function */
    HDC hDC;
    PAINTSTRUCT ps;
    INT nDrawX;
    INT nDrawY;
    HFONT hFont;
    HANDLE Handle;
    static BOOL NewFont;

    switch (message) {


        case WM_CREATE:
            //initialize the output on the screen
            strcpy( FileBuf, " ");
            lpBufPtr = FileBuf;
            dwFileSize = strlen(FileBuf);
            crColor = 0;
            NewFont = FALSE;
            break;


        case WM_PAINT:
            /* Set up a display context to begin painting */
            hDC = BeginPaint (hWnd, &ps);

            /* Initialize drawing position to 1/4 inch from the top  */
            /* and from the left of the top, left corner of the      */
            /* client area of the main windows.                      */
            nDrawX = GetDeviceCaps(hDC, LOGPIXELSX) / 4;   /* 1/4 inch */
            nDrawY = GetDeviceCaps(hDC, LOGPIXELSY) / 4;   /* 1/4 inch */

            if ( NewFont == TRUE )
            {
                hFont = CreateFontIndirect( &lf );
                Handle = SelectObject( hDC, hFont );
            }

            SetTextColor( hDC, crColor );

            // end painting and release hDC
            EndPaint( hWnd, &ps );
            break;


        case WM_COMMAND:           /* message: command from application menu */

            switch( LOWORD( wParam ))
            {
                case IDM_OPENFILE:
                   lpProcOpen = MakeProcInstance((FARPROC)OpenFiles, hInst);

                    DialogBox(hInst,             /* current instance         */
                        "OPENFILES",             /* resource to use          */
                        hWnd,                    /* parent handle            */
                        (DLGPROC)lpProcOpen);    /* About() instance address */

                    FreeProcInstance(lpProcOpen);
                    break;

                case IDM_EXIT:
                    PostQuitMessage(0);
                    break;

                case IDM_CHOOSECOLOR:
                    if (ChooseNewColor( hWnd ))
                        InvalidateRect( hWnd, NULL, TRUE );
                    break;

                case IDM_CHOOSEFONT:
                    if (NewFont = ChooseNewFont( hWnd ))
                        InvalidateRect( hWnd, NULL, TRUE );

                    break;

                case IDM_FINDTEXT:
                    CallFindText( hWnd );
                    break;

                case IDM_REPLACETEXT:
                    CallReplaceText( hWnd );
                    break;

                case IDM_STANDARD:
                  // enable the ChooseColor() option
                    EnableMenuItem(GetMenu(hWnd), IDM_CHOOSECOLOR,
                            MF_BYCOMMAND | MF_ENABLED );
                    // uncheck previous selection
                    CheckMenuItem( GetMenu( hWnd ), wMode, MF_UNCHECKED | MF_BYCOMMAND);
                    //reset mode
                    wMode = LOWORD(wParam);
                    //check new selection
                    CheckMenuItem( GetMenu( hWnd ), wMode, MF_CHECKED | MF_BYCOMMAND);
                    DrawMenuBar( hWnd);
                    break;
                case IDM_HOOK:
                case IDM_CUSTOM:
                    // disable the ChooseColor() option
                    EnableMenuItem(GetMenu(hWnd), IDM_CHOOSECOLOR,
                            MF_BYCOMMAND | MF_GRAYED );
                     // uncheck previous selection
                    CheckMenuItem( GetMenu( hWnd ), wMode, MF_UNCHECKED | MF_BYCOMMAND);
                    //reset mode
                    wMode = LOWORD(wParam);
                    //check new selection
                    CheckMenuItem( GetMenu( hWnd ), wMode, MF_CHECKED | MF_BYCOMMAND);
                    DrawMenuBar( hWnd);
                    break;

                case IDM_ENTERNEW:
                    lpProcEnterNew = MakeProcInstance((FARPROC)EnterNew, hInst);

                    if (DialogBox(hInst,                 /* current instance         */
                        "EnterNewBox",           /* resource to use          */
                        hWnd,                    /* parent handle            */
                        (DLGPROC)lpProcEnterNew) == TRUE)

                        InvalidateRect( hWnd, NULL, TRUE );

                    FreeProcInstance(lpProcEnterNew);
                    break;

                case IDM_PERCEPUAL:
                    Intent = 0;
                    CheckMenuItem( GetMenu( hWnd ), wIntentMode, MF_UNCHECKED | MF_BYCOMMAND);
                    //reset mode
                    wIntentMode = LOWORD(wParam);
                    //check new selection
                    CheckMenuItem( GetMenu( hWnd ), wIntentMode, MF_CHECKED | MF_BYCOMMAND);
                    DrawMenuBar( hWnd);
                    break;
                case IDM_COLOR:
                    Intent = 1;
                    CheckMenuItem( GetMenu( hWnd ), wIntentMode, MF_UNCHECKED | MF_BYCOMMAND);
                    //reset mode
                    wIntentMode = LOWORD(wParam);
                    //check new selection
                    CheckMenuItem( GetMenu( hWnd ), wIntentMode, MF_CHECKED | MF_BYCOMMAND);
                    DrawMenuBar( hWnd);
                    break;
                case IDM_SATURATION:
                    Intent = 2;
                    CheckMenuItem( GetMenu( hWnd ), wIntentMode, MF_UNCHECKED | MF_BYCOMMAND);
                    //reset mode
                    wIntentMode = LOWORD(wParam);
                    //check new selection
                    CheckMenuItem( GetMenu( hWnd ), wIntentMode, MF_CHECKED | MF_BYCOMMAND);
                    DrawMenuBar( hWnd);
                    break;

                case IDM_ASCII:
                    AllowBinary = FALSE;
                    CheckMenuItem( GetMenu( hWnd ), wAsciiMode, MF_UNCHECKED | MF_BYCOMMAND);
                    //reset mode
                    wAsciiMode = LOWORD(wParam);
                    //check new selection
                    CheckMenuItem( GetMenu( hWnd ), wAsciiMode, MF_CHECKED | MF_BYCOMMAND);
                    DrawMenuBar( hWnd);
                    break;
                case IDM_BINARY:
                    AllowBinary = TRUE;
                    CheckMenuItem( GetMenu( hWnd ), wAsciiMode, MF_UNCHECKED | MF_BYCOMMAND);
                    //reset mode
                    wAsciiMode = LOWORD(wParam);
                    //check new selection
                    CheckMenuItem( GetMenu( hWnd ), wAsciiMode, MF_CHECKED | MF_BYCOMMAND);
                    DrawMenuBar( hWnd);
                    break;
                case IDM_AUTO:
                case IDM_ABC:
                case IDM_DEFG:
                    CheckMenuItem( GetMenu( hWnd ), wCSAMode, MF_UNCHECKED | MF_BYCOMMAND);
                    //reset mode
                    wCSAMode = LOWORD(wParam);
                    //check new selection
                    CheckMenuItem( GetMenu( hWnd ), wCSAMode, MF_CHECKED | MF_BYCOMMAND);
                    DrawMenuBar( hWnd);
                    break;

                case IDM_INP_AUTO:
                case IDM_INP_GRAY:
                case IDM_INP_RGB:
                case IDM_INP_CMYK:
                    CheckMenuItem( GetMenu( hWnd ), wInpDrvClrSp, MF_UNCHECKED | MF_BYCOMMAND);
                    //reset mode
                    wInpDrvClrSp = LOWORD(wParam);
                    //check new selection
                    CheckMenuItem( GetMenu( hWnd ), wInpDrvClrSp, MF_CHECKED | MF_BYCOMMAND);
                    DrawMenuBar( hWnd);
                    break;

                case IDM_CSA:
                case IDM_CRD:
				case IDM_PROFCRD:
                case IDM_INTENT:
                    CheckMenuItem( GetMenu( hWnd ), wCSAorCRD, MF_UNCHECKED | MF_BYCOMMAND);
                    //reset mode
                    wCSAorCRD = LOWORD(wParam);
                    if ((wCSAorCRD == IDM_CRD) || (wCSAorCRD == IDM_INTENT))
                    {
                        EnableMenuItem(GetMenu(hWnd), IDM_INP_AUTO, MF_BYCOMMAND | MF_GRAYED );
                        EnableMenuItem(GetMenu(hWnd), IDM_INP_GRAY, MF_BYCOMMAND | MF_GRAYED );
                        EnableMenuItem(GetMenu(hWnd), IDM_INP_RGB,  MF_BYCOMMAND | MF_GRAYED );
                        EnableMenuItem(GetMenu(hWnd), IDM_INP_CMYK, MF_BYCOMMAND | MF_GRAYED );
                        EnableMenuItem(GetMenu(hWnd), IDM_AUTO, MF_BYCOMMAND | MF_GRAYED );
                        EnableMenuItem(GetMenu(hWnd), IDM_ABC,  MF_BYCOMMAND | MF_GRAYED );
                        EnableMenuItem(GetMenu(hWnd), IDM_DEFG, MF_BYCOMMAND | MF_GRAYED );
                    }
                    else
                    {
                        EnableMenuItem(GetMenu(hWnd), IDM_INP_AUTO, MF_BYCOMMAND | MF_ENABLED );
                        EnableMenuItem(GetMenu(hWnd), IDM_INP_GRAY, MF_BYCOMMAND | MF_ENABLED );
                        EnableMenuItem(GetMenu(hWnd), IDM_INP_RGB,  MF_BYCOMMAND | MF_ENABLED );
                        EnableMenuItem(GetMenu(hWnd), IDM_INP_CMYK, MF_BYCOMMAND | MF_ENABLED );
                        EnableMenuItem(GetMenu(hWnd), IDM_AUTO, MF_BYCOMMAND | MF_ENABLED );
                        EnableMenuItem(GetMenu(hWnd), IDM_ABC,  MF_BYCOMMAND | MF_ENABLED );
                        EnableMenuItem(GetMenu(hWnd), IDM_DEFG, MF_BYCOMMAND | MF_ENABLED );
                    }

                    //check new selection
                    CheckMenuItem( GetMenu( hWnd ), wCSAorCRD, MF_CHECKED | MF_BYCOMMAND);
                    DrawMenuBar( hWnd);
                    break;

                case IDM_ABOUT:
                    lpProcAbout = MakeProcInstance((FARPROC)About, hInst);

                    DialogBox(hInst,             /* current instance         */
                        "AboutBox",              /* resource to use          */
                        hWnd,                    /* parent handle            */
                        (DLGPROC)lpProcAbout);   /* About() instance address */

                    FreeProcInstance(lpProcAbout);
                    break;

                default:
                    return (DefWindowProc(hWnd, message, wParam, lParam));

            }
            break;

        case WM_DESTROY:                  /* message: window being destroyed */
            PostQuitMessage(0);
            break;


        default:
            // Handle the special findreplace message (FindReplaceMsg) which
            // was registered at initialization time.
            if ( message == FindReplaceMsg )
            {
                if ( lpFR = (LPFINDREPLACE) lParam )
                    {
                    if (lpFR->Flags & FR_DIALOGTERM )  // terminating dialog
                        return (0);
                    SearchFile( lpFR );
                    InvalidateRect( hWnd, NULL, TRUE );
                    }
                return (0);
            }

            return (DefWindowProc(hWnd, message, wParam, lParam));
    }
    return (0);
}


/****************************************************************************
*
*    FUNCTION: EnterNew(HWND, UINT, WPARAM, LPARAM)
*
*    PURPOSE:  Processes messages for "EnterNew" dialog box
*
*    COMMENTS:
*
*        This function allows the user to enter new text in the current
*        window.  This text is stored in the global current buffer.
*
****************************************************************************/

BOOL CALLBACK EnterNew(
        HWND hDlg,                /* window handle of the dialog box */
        UINT message,             /* type of message                 */
        WPARAM wParam,            /* message-specific information    */
        LPARAM lParam)
{
    CHAR Buf[FILE_LEN-1];

    switch (message)
    {
        case WM_INITDIALOG:                /* message: initialize dialog box */
            return (TRUE);

        case WM_COMMAND:                      /* message: received a command */
            if (LOWORD(wParam) == IDOK)
            {
                GetDlgItemText( hDlg, IDEDIT, Buf, FILE_LEN-1);
                strcpy( FileBuf, Buf);
                lpBufPtr = FileBuf;
                dwFileSize = strlen(FileBuf);
                EndDialog( hDlg, TRUE );
                return (TRUE);
            }
            else if (LOWORD(wParam) == IDCANCEL)
            {   /* System menu close command? */
                EndDialog(hDlg, FALSE);       /* Exits the dialog box        */
                return (TRUE);
            }
            break;
    }
    return (FALSE);                           /* Didn't process a message    */

    // avoid compiler warnings at W3
    lParam;
}


/****************************************************************************
*
*    FUNCTION: About(HWND, UINT, WPARAM, LPARAM)
*
*    PURPOSE:  Processes messages for "About" dialog box
*
*    COMMENTS:
*
*       No initialization is needed for this particular dialog box, but TRUE
*       must be returned to Windows.
*
*       Wait for user to click on "Ok" button, then close the dialog box.
*
****************************************************************************/

BOOL WINAPI About(
        HWND hDlg,                /* window handle of the dialog box */
        UINT message,             /* type of message                 */
        WPARAM wParam,            /* message-specific information    */
        LPARAM lParam)
{
    switch (message)
    {
        case WM_INITDIALOG:                /* message: initialize dialog box */
            return (TRUE);

        case WM_COMMAND:                      /* message: received a command */
            if (LOWORD(wParam) == IDOK          /* "OK" box selected?        */
                || LOWORD(wParam) == IDCANCEL) {        /* System menu close command? */
                EndDialog(hDlg, TRUE);        /* Exits the dialog box        */
                return (TRUE);
            }
            break;
    }
    return (FALSE);                           /* Didn't process a message    */

    // avoid compiler warnings at W3
    lParam;
}

/****************************************************************************
*
*    FUNCTION: FileOpenHookProc(HWND, UINT, WPARAM, LPARAM)
*
*    PURPOSE:  Processes messages for GetFileNameOpen() common dialog box
*
*    COMMENTS:
*
*        This function will prompt the user if they are sure they want
*        to open the file if the OFN_ENABLEHOOK flag is set.
*
*        If the current option mode is CUSTOM, the user is allowed to check
*        a box in the dialog prompting them whether or not they would like
*        the file created.  If they check this box, the file is created and
*        the string 'Empty' is written to it.
*
*    RETURN VALUES:
*        TRUE - User chose 'Yes' from the "Are you sure message box".
*        FALSE - User chose 'No'; return to the dialog box.
*
****************************************************************************/

BOOL CALLBACK FileOpenHookProc(
        HWND hDlg,                /* window handle of the dialog box */
        UINT message,             /* type of message                 */
        WPARAM wParam,            /* message-specific information    */
        LPARAM lParam)
{

    int hFile;
    CHAR szTempText[256];
    CHAR szString[256];
    OFSTRUCT OfStruct;

    switch (message)
    {

        case WM_COMMAND:
            if (LOWORD(wParam) == IDOK)
            {
                GetDlgItemText( hDlg, edt1, szTempText,
                        sizeof( szTempText ) - 1);

                if ( OpenFileName.Flags & OFN_PATHMUSTEXIST )
                {
                    sprintf( szString, "Are you sure you want to open %s?",
                        szTempText);
                    if ( MessageBox( hDlg, szString, "Information",
                        MB_YESNO ) == IDYES )

                        break;
                    return (TRUE);
                }

                // check to see if the Create File box has been checked
                if ( (BOOL)(SendMessage( GetDlgItem(hDlg, chx2),
                    BM_GETCHECK, 0, 0L )) == TRUE )
                {
                    // if so, create the file
                    if ((hFile = OpenFile(szTempText,
                        &OfStruct,
                        OF_CREATE)) == -1)
                    {
                        MessageBox( hDlg,
                            "Directory could not be created.",
                            NULL,
                            MB_OK );
                        return (FALSE);
                    }

                    strcpy(FileBuf, "Empty");
                    lpBufPtr = FileBuf;
                    dwFileSize = strlen(FileBuf);
                    if (_lwrite( hFile, (LPSTR)&FileBuf[0], dwFileSize)==-1)
                        MessageBox( hDlg, "Error writing file.", NULL, MB_OK );

                    // close the file
                    _lclose( hFile );
                }

            }
            break;
    }
    return (FALSE);

    // avoid compiler warnings at W3
    lParam;

}

/****************************************************************************
*
*    FUNCTION: OpenNewFile(HWND)
*
*    PURPOSE:  Invokes common dialog function to open a file and opens it.
*
*    COMMENTS:
*
*        This function initializes the OPENFILENAME structure and calls
*        the GetOpenFileName() common dialog function.  This function will
*        work regardless of the mode: standard, using a hook or using a
*        customized template.
*
*    RETURN VALUES:
*        TRUE - The file was opened successfully and read into the buffer.
*        FALSE - No files were opened.
*
****************************************************************************/
BOOL OpenNewFile( HWND hWnd )
{
   strcpy( szFile, "");
   strcpy( szFileTitle, "");

   OpenFileName.lStructSize       = sizeof(OPENFILENAME);
   OpenFileName.hwndOwner         = hWnd;
   OpenFileName.hInstance         = (HANDLE) hInst;
   OpenFileName.lpstrFilter       = szFilter;
   OpenFileName.lpstrCustomFilter = (LPSTR) NULL;
   OpenFileName.nMaxCustFilter    = 0L;
   OpenFileName.nFilterIndex      = 1L;
   OpenFileName.lpstrFile         = szFile;
   OpenFileName.nMaxFile          = sizeof(szFile);
   OpenFileName.lpstrFileTitle    = szFileTitle;
   OpenFileName.nMaxFileTitle     = sizeof(szFileTitle);
   OpenFileName.lpstrInitialDir   = NULL;
   OpenFileName.lpstrTitle        = "Open a File";
   OpenFileName.nFileOffset       = 0;
   OpenFileName.nFileExtension    = 0;
   OpenFileName.lpstrDefExt       = "*.icm";
   OpenFileName.lCustData         = 0;

   switch( wMode )
   {
        case IDM_STANDARD:
            OpenFileName.Flags = OFN_SHOWHELP | OFN_PATHMUSTEXIST |
                OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
            break;

        case IDM_HOOK:
            OpenFileName.Flags = OFN_SHOWHELP | OFN_PATHMUSTEXIST |
                OFN_FILEMUSTEXIST | OFN_HIDEREADONLY | OFN_ENABLEHOOK;
            OpenFileName.lpfnHook = (LPOFNHOOKPROC)MakeProcInstance(FileOpenHookProc, NULL);
            break;

        case IDM_CUSTOM:
            OpenFileName.Flags = OFN_SHOWHELP | OFN_ENABLEHOOK |
                OFN_HIDEREADONLY | OFN_ENABLETEMPLATE;
            OpenFileName.lpfnHook = (LPOFNHOOKPROC)MakeProcInstance(FileOpenHookProc, NULL);
            OpenFileName.lpTemplateName = (LPSTR)MAKEINTRESOURCE(FILEOPENORD);
            break;
   }

   if (GetOpenFileName(&OpenFileName))
   {
   }
   else
   {
      ProcessCDError(CommDlgExtendedError(), hWnd );
      return FALSE;
   }
   return TRUE;
}

/****************************************************************************
*
*    FUNCTION: SaveToFile( HWND )
*
*    PURPOSE:  Saves the current buffer to the current file.
*
*    COMMENTS:
*
*        This function will save the current text buffer into the file
*        specified from the GetSaveFileName() common dialog function.
*
*    RETURN VALUES:
*        TRUE - The file was saved successfully.
*        FALSE - The buffer was not saved to a file.
*
****************************************************************************/
BOOL SaveToFile( HWND hWnd )
{
   int hFile;
   OFSTRUCT OfStruct;
   WORD wStyle;
   CHAR buf[256];

   if (OpenFileName.Flags | OFN_FILEMUSTEXIST)
        wStyle = OF_READWRITE;
   else
        wStyle = OF_READWRITE | OF_CREATE;

   if ((hFile = OpenFile(OpenFileName.lpstrFile, &OfStruct,
         wStyle)) == -1)
   {
      sprintf( buf, "Could not create file %s", OpenFileName.lpstrFile );
      MessageBox( hWnd, buf, NULL, MB_OK );
      return FALSE;
   }
   // write it's contents into a file
   if (_lwrite( hFile, (LPSTR)&FileBuf[0], dwFileSize)==-1)
   {
      MessageBox( hWnd, "Error writing file.", NULL, MB_OK );
      return FALSE;
   }

   // close the file
   _lclose( hFile );

   sprintf( buf, "%s", OpenFileName.lpstrFile );
   MessageBox( hWnd, buf, "File Saved", MB_OK );
   return TRUE;
}


/****************************************************************************
*
*    FUNCTION: FileSaveHookProc(HWND, UINT, WPARAM, LPARAM)
*
*    PURPOSE:  Processes messages for FileSave common dialog box
*
*    COMMENTS:
*
*        This hook procedure prompts the user if they want to save the
*        current file.  If they choose YES, the file is saved and the dialog
*        is dismissed.  If they choose NO, they are returned to the
*        GetSaveFileName() common dialog.
*
*        If the current mode calls for a customized template, this function
*        will test the 'Create File?' checkbox.  If the user choses no, the
*        OFN_FILEMUSTEXIST flag is set.
*
*    RETURN VALUES:
*        TRUE - User chose 'Yes' from the "Are you sure message box".
*        FALSE - User chose 'No'; return to the dialog box.
*
*
****************************************************************************/

BOOL CALLBACK FileSaveHookProc(
        HWND hDlg,                /* window handle of the dialog box */
        UINT message,             /* type of message                 */
        WPARAM wParam,            /* message-specific information    */
        LPARAM lParam)
{
    CHAR szTempText[256];
    CHAR szString[256];

    switch (message)
    {
        case WM_COMMAND:
            if (LOWORD(wParam) == IDOK)
            {
                GetDlgItemText( hDlg, edt1, szTempText,
                    sizeof( szTempText ) - 1);
                if ( OpenFileName.Flags & OFN_ENABLETEMPLATE )
                {
                    // check to see if the Create File box has been checked
                    if ( (BOOL)(SendMessage( GetDlgItem(hDlg, chx2),
                        BM_GETCHECK, 0, 0L )) == FALSE )
                        OpenFileName.Flags |= OFN_FILEMUSTEXIST;
                    break;

                }
                else
                {
                    sprintf( szString, "Are you sure you want to save %s?",
                        szTempText);
                    if ( MessageBox( hDlg, szString, "Information",
                        MB_YESNO ) == IDYES )
                        break;
                    return(TRUE);
                }

            }
            break;
    }
    return (FALSE);

    // avoid compiler warnings at W3
    lParam;

}

/****************************************************************************
*
*    FUNCTION: SaveAs(HWND)
*
*    PURPOSE:  Invokes the common dialog function to save the current
*              buffer to a file.
*    COMMENTS:
*
*        This function initializes the OPENFILENAME structure for any
*        mode selected by the user: standard, using a hook or using a
*        customized template.  It then calls the GetSaveFileName()
*        common dialog function.
*
*    RETURN VALUES:
*        TRUE - The file was saved successfully.
*        FALSE - The buffer was not saved to a file.
*
****************************************************************************/
BOOL SaveAs( HWND hWnd )
{
   int      i;

//   strcpy( szFile, "");
//   strcpy( szFileTitle, "");

   for ( i = lstrlen(OpenFileName.lpstrFileTitle); i > 0; i--)
   {
       if (OpenFileName.lpstrFileTitle[i] == '.') break;
   }
   i ++;
   if (wCSAorCRD == IDM_CSA)
   {
        OpenFileName.lpstrFileTitle[i++] = 'C';
        OpenFileName.lpstrFileTitle[i++] = 'S';
        OpenFileName.lpstrFileTitle[i++] = 'A';
        OpenFileName.lpstrFileTitle[i] = 0;
   }
   else if ((wCSAorCRD == IDM_CRD) ||
	        (wCSAorCRD == IDM_PROFCRD))
   {
        OpenFileName.lpstrFileTitle[i++] = 'C';
        OpenFileName.lpstrFileTitle[i++] = 'R';
        OpenFileName.lpstrFileTitle[i++] = 'D';
        OpenFileName.lpstrFileTitle[i] = 0;
   }
   else
   {
        OpenFileName.lpstrFileTitle[i++] = 'I';
        OpenFileName.lpstrFileTitle[i++] = 'N';
        OpenFileName.lpstrFileTitle[i++] = 'T';
        OpenFileName.lpstrFileTitle[i] = 0;
   }
   strcpy(OpenFileName.lpstrFile, OpenFileName.lpstrFileTitle);

   OpenFileName.lStructSize       = sizeof(OPENFILENAME);
   OpenFileName.hwndOwner         = hWnd;
   OpenFileName.hInstance         = (HANDLE) hInst;
   OpenFileName.lpstrFilter       = szSaveFilter;
   OpenFileName.lpstrCustomFilter = (LPSTR) NULL;
   OpenFileName.nMaxCustFilter    = 0L;
   OpenFileName.nFilterIndex      = 1L;
   OpenFileName.nMaxFile          = sizeof(szFile);
   OpenFileName.nMaxFileTitle     = sizeof(szFileTitle);
   OpenFileName.lpstrInitialDir   = NULL;
   OpenFileName.lpstrTitle        = "Save File As";
   OpenFileName.nFileOffset       = 0;
   OpenFileName.nFileExtension    = 0;
   OpenFileName.lpstrDefExt       = "*.csa";
   OpenFileName.lCustData         = 0;

   switch( wMode )
   {
        case IDM_STANDARD:
            OpenFileName.Flags = 0L;
            OpenFileName.lpfnHook = (LPOFNHOOKPROC)(FARPROC)NULL;
            OpenFileName.lpTemplateName = (LPSTR)NULL;
            break;

        case IDM_HOOK:
            OpenFileName.Flags = OFN_ENABLEHOOK;
            OpenFileName.lpfnHook = (LPOFNHOOKPROC)MakeProcInstance(FileSaveHookProc, NULL);
            OpenFileName.lpTemplateName = (LPSTR)NULL;
            break;

        case IDM_CUSTOM:
            OpenFileName.Flags = OFN_ENABLEHOOK | OFN_ENABLETEMPLATE;
            OpenFileName.lpfnHook = (LPOFNHOOKPROC)MakeProcInstance(FileSaveHookProc, NULL);
            OpenFileName.lpTemplateName = (LPSTR)MAKEINTRESOURCE(FILEOPENORD);
            break;
   }

   if ( GetSaveFileName( &OpenFileName ))
   {
        return (TRUE);
   }
   else
   {
        ProcessCDError(CommDlgExtendedError(), hWnd );
        return FALSE;
   }

   return (FALSE);
}


/****************************************************************************
*
*    FUNCTION: ChooseColorHookProc(HWND, UINT, WPARAM, LPARAM)
*
*    PURPOSE:  Processes messages for ChooseColor common dialog box
*
*    COMMENTS:
*
*        This hook procedure simply prompts the user whether or not they
*        want to change the color.  if they choose YES, the color of the
*        text will be changed and the dialog will be dismissed.  If they
*        choose NO, the color will not be changed and the user will be
*        returned to the dialog
*
*    RETURN VALUES:
*        TRUE - User chose 'Yes' from the "Are you sure message box".
*        FALSE - User chose 'No'; return to the dialog box.
*
****************************************************************************/

BOOL CALLBACK ChooseColorHookProc(
        HWND hDlg,                /* window handle of the dialog box */
        UINT message,             /* type of message                 */
        WPARAM wParam,            /* message-specific information    */
        LPARAM lParam)
{

    switch (message)
    {
        case WM_COMMAND:
            if (LOWORD(wParam) == IDOK)
            {
                if (MessageBox( hDlg, "Are you sure you want to change the color?",
                    "Information", MB_YESNO ) == IDYES )
                    break;
                return (TRUE);

            }
            break;
    }
    return (FALSE);

    // avoid compiler warnings at W3
    lParam;

}


/****************************************************************************
*
*    FUNCTION: ChooseNewColor(HWND)
*
*    PURPOSE:  Invokes common dialog function to chose a new color.
*
*    COMMENTS:
*        This function initializes the CHOOSECOLOR structure for any
*        mode the user chooses: standard, using a hook or using a
*        customized template.  It then calls the ChooseColor()
*        common dialog function.
*
*    RETURN VALUES:
*        TRUE - A new color was chosen.
*        FALSE - No new color was chosen.
*
****************************************************************************/
BOOL ChooseNewColor( HWND hWnd )
{

    DWORD dwColor;
    DWORD dwCustClrs[16];
    BOOL fSetColor = FALSE;
    int i;


    for (i=0; i < 15; i++)
        dwCustClrs[i] = RGB( 255, 255, 255);

    dwColor = RGB( 0, 0, 0 );

    chsclr.lStructSize = sizeof(CHOOSECOLOR);
    chsclr.hwndOwner = hWnd;
    chsclr.hInstance = (HANDLE)hInst;
    chsclr.rgbResult = dwColor;
    chsclr.lpCustColors = (LPDWORD)dwCustClrs;
    chsclr.lCustData = 0L;

    switch( wMode )
    {
        case IDM_HOOK:
        case IDM_CUSTOM:
            chsclr.Flags = CC_PREVENTFULLOPEN | CC_ENABLEHOOK;
            chsclr.lpfnHook = (LPCCHOOKPROC)MakeProcInstance(ChooseColorHookProc, NULL);
            chsclr.lpTemplateName = (LPSTR)NULL;
            break;

        case IDM_STANDARD:
            chsclr.Flags = CC_PREVENTFULLOPEN;
            chsclr.lpfnHook = (LPCCHOOKPROC)(FARPROC)NULL;
            chsclr.lpTemplateName = (LPSTR)NULL;
            break;


   }

   if ( fSetColor = ChooseColor( &chsclr ))
   {
       crColor = chsclr.rgbResult;
       return (TRUE);
   }
   else
   {
       ProcessCDError(CommDlgExtendedError(), hWnd );
       return FALSE;
   }
}


/****************************************************************************
*
*    FUNCTION: ChooseFontHookProc(HWND, UINT, WPARAM, LPARAM)
*
*    PURPOSE:  Processes messages for ChooseFont common dialog box
*
*    COMMENTS:
*
*        This hook procedure simply prompts the user whether or not they
*        want to change the font.  if they choose YES, the color of the
*        font will be changed and the dialog will be dismissed.  If they
*        choose NO, the font will not be changed and the user will be
*        returned to the dialog
*
*        If the current mode is set to use a customized template, the
*        color drop down combo box is hidden.
*
*    RETURN VALUES:
*        TRUE - Change the font.
*        FALSE - Return to the dialog box.
*
****************************************************************************/

BOOL CALLBACK ChooseFontHookProc(
        HWND hDlg,                /* window handle of the dialog box */
        UINT message,             /* type of message                 */
        WPARAM wParam,            /* message-specific information    */
        LPARAM lParam)
{

    switch (message)
    {
        case WM_INITDIALOG:
            if (chf.Flags & CF_ENABLETEMPLATE)
            {
                ShowWindow(GetDlgItem(hDlg, stc4), SW_HIDE);
                ShowWindow(GetDlgItem(hDlg, cmb4), SW_HIDE);
            }
            break;

        case WM_COMMAND:
            if (LOWORD(wParam) == IDOK)
            {
                if (MessageBox( hDlg, "Are you sure you want to change the font?",
                    "Information", MB_YESNO ) == IDYES )
                    break;
                return (TRUE);

            }
            break;
    }
    return (FALSE);

    // avoid compiler warnings at W3
    lParam;

}


/****************************************************************************
*
*    FUNCTION: ChooseNewFont(HWND)
*
*    PURPOSE:  Invokes common dialog function to chose a new font.
*
*    COMMENTS:
*
*        This function initializes the CHOOSEFONT structure for any mode
*        the user chooses: standard, using a hook or using a customized
*        template.  It then calls the ChooseFont() common dialog function.
*
*    RETURN VALUES:
*        TRUE - A new font was chosen.
*        FALSE - No new font was chosen.
*
****************************************************************************/
BOOL ChooseNewFont( HWND hWnd )
{

   HDC hDC;

   hDC = GetDC( hWnd );
   chf.hDC = CreateCompatibleDC( hDC );
   ReleaseDC( hWnd, hDC );
   chf.lStructSize = sizeof(CHOOSEFONT);
   chf.hwndOwner = hWnd;
   chf.lpLogFont = &lf;
   chf.Flags = CF_SCREENFONTS | CF_EFFECTS;
   chf.rgbColors = RGB(0, 255, 255);
   chf.lCustData = 0;
   chf.hInstance = (HANDLE)hInst;
   chf.lpszStyle = (LPSTR)NULL;
   chf.nFontType = SCREEN_FONTTYPE;
   chf.nSizeMin = 0;
   chf.nSizeMax = 0;

   switch( wMode )
   {
        case IDM_STANDARD:
            chf.Flags = CF_SCREENFONTS | CF_EFFECTS;
            chf.lpfnHook = (LPCFHOOKPROC)(FARPROC)NULL;
            chf.lpTemplateName = (LPSTR)NULL;
            break;

        case IDM_HOOK:
            chf.Flags = CF_SCREENFONTS | CF_EFFECTS | CF_ENABLEHOOK;
            chf.lpfnHook = (LPCFHOOKPROC)MakeProcInstance(ChooseFontHookProc, NULL);
            chf.lpTemplateName = (LPSTR)NULL;
            break;

        case IDM_CUSTOM:
            chf.Flags = CF_SCREENFONTS | CF_EFFECTS | CF_ENABLEHOOK |
              CF_ENABLETEMPLATE;
            chf.lpfnHook = (LPCFHOOKPROC)MakeProcInstance(ChooseFontHookProc, NULL);
            chf.lpTemplateName = (LPSTR)MAKEINTRESOURCE(FORMATDLGORD31);
            break;
   }


   if( ChooseFont( &chf ) == FALSE )
   {
        DeleteDC( hDC );
        ProcessCDError(CommDlgExtendedError(), hWnd );
        return FALSE;
   }


   DeleteDC( hDC );

   return (TRUE);
}

/****************************************************************************
*
*    FUNCTION: PrintSetupHookProc(HWND, UINT, WPARAM, LPARAM)
*
*    PURPOSE:  Processes messages for PrintDlg setup common dialog box
*
*    COMMENTS:
*
*        This function processes the hook and customized template for the
*        print setup common dialog box.  If the customized template has
*        been provided, the 'Options' pushbutton is hidden.  If the hook only mode
*        is chosen, a message box is displayed informing the user that the
*        hook has been installed.
*
*    RETURN VALUES:
*        TRUE - Continue.
*        FALSE - Return to the dialog box.
*
****************************************************************************/

BOOL CALLBACK PrintSetupHookProc(
        HWND hDlg,                /* window handle of the dialog box */
        UINT message,             /* type of message                 */
        WPARAM wParam,            /* message-specific information    */
        LPARAM lParam)
{

    switch (message)
    {
        case WM_INITDIALOG:
            if (pd.Flags & PD_ENABLESETUPTEMPLATE )
            {
                ShowWindow( GetDlgItem(hDlg, psh1), SW_HIDE );
                return(TRUE);
            }
            MessageBox( hDlg,
                    "Hook installed.",
                    "Information", MB_OK );
            return (TRUE);
    }
    return (FALSE);

    // avoid compiler warnings at W3
    lParam;
    wParam;
}



/****************************************************************************
*
*    FUNCTION: PrintDlgHookProc(HWND, UINT, UINT, LONG)
*
*    PURPOSE:  Processes messages for PrintDlg common dialog box
*
*    COMMENTS:
*
*        This hook procedure simply prompts the user whether or not they
*        want to print.  if they choose YES, the text buf will be printed
*        and the dialog will be dismissed.  If they choose NO, the text buf
*        will not be printeded and the user will be returned to the dialog.
*
*        If the current mode is 'custom', the 'Print to file' and 'Collate
*        Copies' options are hidden.
*
*    RETURN VALUES:
*        TRUE - Continue.
*        FALSE - Return to the dialog box.
*
****************************************************************************/

BOOL CALLBACK PrintDlgHookProc(
        HWND hDlg,                /* window handle of the dialog box */
        UINT message,             /* type of message                 */
        WPARAM wParam,            /* message-specific information    */
        LPARAM lParam)
{

    switch (message)
    {
        case WM_INITDIALOG:
            if (pd.Flags & PD_ENABLEPRINTTEMPLATE )
            {
                ShowWindow( GetDlgItem(hDlg, chx1), SW_HIDE );
                ShowWindow( GetDlgItem(hDlg, chx2), SW_HIDE );
                return(TRUE);
            }
            MessageBox( hDlg,
                    "Hook installed.",
                    "Information", MB_OK );
            return (TRUE);

        case WM_COMMAND:
            if (LOWORD(wParam) == IDOK)
            {
                if (MessageBox( hDlg, "Are you sure you want to print?",
                    "Information", MB_YESNO ) == IDYES )
                    break;
                return (TRUE);

            }
            break;
    }
    return (FALSE);

    // avoid compiler warnings at W3
    lParam;

}


/****************************************************************************
*
*    FUNCTION: PrintFile(HWND)
*
*    PURPOSE:  Invokes common dialog function to print.
*
*    COMMENTS:
*
*        This function initializes the PRINTDLG structure for all modes
*        possible: standard, using a hook or using a customized template.
*        When hook mode is chosen, a hook is installed for both the
*        Print dialog and the Print Setup dialog.  When custom mode is
*        chosen, the templates are enabled for both the print dialog and
*        the Print Setup dialog boxes.
*
*        If the PrintDlg() common dialog returns TRUE, the current
*        text buffer is printed out.
*
*
*    RETURN VALUES:
*        void.
*
****************************************************************************/
void PrintFile( HWND hWnd )
{


    // initialize PRINTDLG structure
    pd.lStructSize = sizeof(PRINTDLG);
    pd.hwndOwner = hWnd;
    pd.hDevMode = (HANDLE)NULL;
    pd.hDevNames = (HANDLE)NULL;
    pd.nFromPage = 0;
    pd.nToPage = 0;
    pd.nMinPage = 0;
    pd.nMaxPage = 0;
    pd.nCopies = 0;
    pd.hInstance = (HANDLE)hInst;


    switch( wMode )
    {
        case IDM_STANDARD:
            pd.Flags = PD_RETURNDC | PD_NOPAGENUMS | PD_NOSELECTION | PD_PRINTSETUP;
            pd.lpfnSetupHook = (LPSETUPHOOKPROC)(FARPROC)NULL;
            pd.lpSetupTemplateName = (LPSTR)NULL;
            pd.lpfnPrintHook = (LPPRINTHOOKPROC)(FARPROC)NULL;
            pd.lpPrintTemplateName = (LPSTR)NULL;
            break;

        case IDM_HOOK:
            pd.Flags = PD_RETURNDC | PD_NOPAGENUMS | PD_NOSELECTION |
                PD_ENABLEPRINTHOOK | PD_ENABLESETUPHOOK | PD_PRINTSETUP;
            pd.lpfnSetupHook = (LPSETUPHOOKPROC)MakeProcInstance(PrintSetupHookProc, NULL);
            pd.lpSetupTemplateName = (LPSTR)NULL;
            pd.lpfnPrintHook = (LPPRINTHOOKPROC)MakeProcInstance(PrintDlgHookProc, NULL);
            pd.lpPrintTemplateName = (LPSTR)NULL;
            break;

        case IDM_CUSTOM:
            pd.Flags = PD_RETURNDC | PD_NOPAGENUMS | PD_NOSELECTION |
                PD_ENABLEPRINTHOOK | PD_ENABLEPRINTTEMPLATE |
                PD_ENABLESETUPHOOK | PD_ENABLESETUPTEMPLATE | PD_PRINTSETUP;
            pd.lpfnSetupHook = (LPSETUPHOOKPROC)MakeProcInstance(PrintSetupHookProc, NULL);
            pd.lpSetupTemplateName = (LPSTR)MAKEINTRESOURCE(PRNSETUPDLGORD);
            pd.lpfnPrintHook = (LPPRINTHOOKPROC)MakeProcInstance(PrintDlgHookProc, NULL);
            pd.lpPrintTemplateName = (LPSTR)MAKEINTRESOURCE(PRINTDLGORD);
            break;

    }

    //print a test page if successful
    if (PrintDlg(&pd) == TRUE)
    {
        Escape(pd.hDC, STARTDOC, 8, "Test-Doc", NULL);

        //Print text
        TextOut(pd.hDC, 5, 5, FileBuf, strlen(FileBuf));

        Escape(pd.hDC, NEWFRAME, 0, NULL, NULL);
        Escape(pd.hDC, ENDDOC, 0, NULL, NULL );
        DeleteDC(pd.hDC);
        if (pd.hDevMode)
            GlobalFree(pd.hDevMode);
        if (pd.hDevNames)
            GlobalFree(pd.hDevNames);
    }
   else
        ProcessCDError(CommDlgExtendedError(), hWnd );
}


/****************************************************************************
*
*    FUNCTION: ReplaceTextHookProc(HWND, UINT, WPARAM, LPARAM)
*
*    PURPOSE:  Processes messages for ReplaceText common dialog box
*
*    COMMENTS:
*
*        Puts up a message stating that the hook is active if hook
*        only active.  Otherwise, if template enabled, hides the
*        Replace All pushbutton, plus the 'Match case' and
*        'Match whole word' check box options.
*
*    RETURN VALUES:
*        TRUE - Continue.
*        FALSE - Return to the dialog box.
*
****************************************************************************/

BOOL CALLBACK ReplaceTextHookProc(
        HWND hDlg,                /* window handle of the dialog box */
        UINT message,             /* type of message                 */
        WPARAM wParam,            /* message-specific information    */
        LPARAM lParam)
{
    switch (message)
    {
        case WM_INITDIALOG:
            if (frText.Flags & FR_ENABLETEMPLATE )
                {
                    ShowWindow( GetDlgItem(hDlg, psh2), SW_HIDE );
                    ShowWindow( GetDlgItem(hDlg, chx1), SW_HIDE );
                    ShowWindow( GetDlgItem(hDlg, chx2), SW_HIDE );
                }
            MessageBox( hDlg,
                    "Hook installed.",
                    "Information", MB_OK );
            return (TRUE);


        default:
            break;
    }
    return (FALSE);

    // avoid compiler warnings at W3
    lParam;
    wParam;
}

/****************************************************************************
*
*    FUNCTION: FindTextHookProc(HWND, UINT, UINT, LONG)
*
*    PURPOSE:  Processes messages for FindText common dialog box
*
*    COMMENTS:
*
*        Puts up a message stating that the hook is active if hook
*        only enabled.  If custom template, hides the 'Match case'
*        and 'Match whole word' options, hides the group box 'Direction'
*        with the radio buttons 'Up' and 'Down'.
*
*    RETURN VALUES:
*        TRUE - Continue.
*        FALSE - Return to the dialog box.
*
****************************************************************************/

BOOL CALLBACK FindTextHookProc(
        HWND hDlg,                /* window handle of the dialog box */
        UINT message,             /* type of message                 */
        WPARAM wParam,            /* message-specific information    */
        LPARAM lParam)
{

    switch (message)
    {
        case WM_INITDIALOG:
            if (frText.Flags & FR_ENABLETEMPLATE )
            {
                ShowWindow(GetDlgItem(hDlg, chx1), SW_HIDE);
                ShowWindow(GetDlgItem(hDlg, grp1), SW_HIDE);
                ShowWindow(GetDlgItem(hDlg, chx2), SW_HIDE);
                ShowWindow(GetDlgItem(hDlg, rad1), SW_HIDE);
                ShowWindow(GetDlgItem(hDlg, rad2), SW_HIDE);
            }
            MessageBox( hDlg,
                    "Hook installed.",
                    "Information", MB_OK );
            return (TRUE);


        default:
            break;
    }
    return (FALSE);

    // avoid compiler warnings at W3
    lParam;
    wParam;
}


/****************************************************************************
*
*    FUNCTION: CallFindText( HWND )
*
*    PURPOSE:  Initializes and calls the FindText()
*        common dialog.
*
*    COMMENTS:
*
*        This function initialzes the FINDREPLACE structure for any mode:
*        standard, using a hook or using a customized template.  It then
*        calls the FindText() common dialog function.
*
*    RETURN VALUES:
*        void.
*
****************************************************************************/
void CallFindText( HWND hWnd )
{

    frText.lStructSize = sizeof( frText );
    frText.hwndOwner = hWnd;
    frText.hInstance = (HANDLE)hInst;
    frText.lpstrFindWhat = szFindString;
    frText.lpstrReplaceWith = (LPSTR)NULL;
    frText.wFindWhatLen = sizeof(szFindString);
    frText.wReplaceWithLen = 0;
    frText.lCustData = 0;
    lpBufPtr = FileBuf;

    switch( wMode )
    {
        case IDM_STANDARD:
            frText.Flags =  FR_NOMATCHCASE | FR_NOUPDOWN | FR_NOWHOLEWORD;
            frText.lpfnHook = (LPFRHOOKPROC)(FARPROC)NULL;
            frText.lpTemplateName = (LPSTR)NULL;
            break;

        case IDM_HOOK:
            frText.Flags = FR_NOMATCHCASE | FR_NOUPDOWN | FR_NOWHOLEWORD |
                FR_ENABLEHOOK;
            frText.lpfnHook = (LPFRHOOKPROC)MakeProcInstance(FindTextHookProc, NULL);
            frText.lpTemplateName = (LPSTR)NULL;
            break;

        case IDM_CUSTOM:
            frText.Flags = FR_NOMATCHCASE | FR_NOUPDOWN | FR_NOWHOLEWORD |
                 FR_ENABLEHOOK | FR_ENABLETEMPLATE;
            frText.lpfnHook = (LPFRHOOKPROC)MakeProcInstance(FindTextHookProc, NULL);
            frText.lpTemplateName = (LPSTR)MAKEINTRESOURCE(FINDDLGORD);
            break;
    }

    if ((hDlgFR = FindText(&frText)) == NULL)
        ProcessCDError(CommDlgExtendedError(), hWnd );

}


/****************************************************************************
*
*    FUNCTION: CallReplaceText( HWND )
*
*    PURPOSE:  Initializes and calls the ReplaceText()
*        common dialog.
*
*    COMMENTS:
*
*        This function initialzes the FINDREPLACE structure for any mode:
*        standard, using a hook or using a customized template.  It then
*        calls the ReplaceText() common dialog function.
*
*    RETURN VALUES:
*        void.
*
****************************************************************************/
void CallReplaceText( HWND hWnd )
{
    frText.lStructSize = sizeof( frText );
    frText.hwndOwner = hWnd;
    frText.hInstance = (HANDLE)hInst;
    frText.lpstrFindWhat = szFindString;
    frText.lpstrReplaceWith = szReplaceString;
    frText.wFindWhatLen = sizeof(szFindString);
    frText.wReplaceWithLen = sizeof( szReplaceString );
    frText.lCustData = 0;
    lpBufPtr = FileBuf;

    switch( wMode )
    {
        case IDM_STANDARD:
            frText.Flags = FR_NOMATCHCASE | FR_NOUPDOWN | FR_NOWHOLEWORD;
            frText.lpfnHook = (LPFRHOOKPROC)(FARPROC)NULL;
            frText.lpTemplateName = (LPSTR)NULL;
            break;

        case IDM_HOOK:
            frText.Flags = FR_NOMATCHCASE | FR_NOUPDOWN | FR_NOWHOLEWORD |
                FR_ENABLEHOOK;
            frText.lpfnHook = (LPFRHOOKPROC)MakeProcInstance(ReplaceTextHookProc, NULL);
            frText.lpTemplateName = (LPSTR)NULL;
            break;

        case IDM_CUSTOM:
            frText.Flags = FR_NOMATCHCASE | FR_NOUPDOWN | FR_NOWHOLEWORD |
                FR_ENABLEHOOK | FR_ENABLETEMPLATE;
            frText.lpfnHook = (LPFRHOOKPROC)MakeProcInstance(ReplaceTextHookProc, NULL);
            frText.lpTemplateName = (LPSTR)MAKEINTRESOURCE(REPLACEDLGORD);
            break;
    }

    if ( (hDlgFR = ReplaceText( &frText )) == NULL )
            ProcessCDError(CommDlgExtendedError(), hWnd );

}

/****************************************************************************
*
*    FUNCTION: SearchFile(LPFINDREPLACE)
*
*    PURPOSE:  Does the find/replace specified by the Find/ReplaceText
*        common dialog.
*
*    COMMENTS:
*
*        This function does the lease necessary to implement find and
*        replace by calling existing c-runtime functions.  It is in
*        no way intended to demonstrate either correct or efficient
*        methods for doing textual search or replacement.
*
*    RETURN VALUES:
*        void.
*
****************************************************************************/
void SearchFile( LPFINDREPLACE lpFR )
{

    CHAR Buf[FILE_LEN];
    CHAR *pStr;
    int count, newcount;
    static BOOL bFoundLast = FALSE;
    
if ( lpFR->Flags & ( FR_FINDNEXT | FR_REPLACE | FR_REPLACEALL ) )
    {
    memset(Buf, '\0', FILE_LEN -1);
    if ( bFoundLast )
        {
        if ( (lpBufPtr != FileBuf) && (lpFR->Flags & FR_FINDNEXT) )
            lpBufPtr++;
        bFoundLast = FALSE;
        }

    if (!*lpBufPtr || !(pStr = strstr( lpBufPtr, lpFR->lpstrFindWhat ) ) )
        {
        sprintf( Buf, "'%s' not found!", lpFR->lpstrFindWhat );
        lpBufPtr = FileBuf;
        MessageBox( lpFR->hwndOwner, Buf, "No luck", MB_OK | MB_TASKMODAL);
        }
    else
        {
        if ( lpFR->Flags & FR_FINDNEXT )
            {
            sprintf( Buf, "Found Next '%s'!\nSubstring: '%.10s'", 
                     lpFR->lpstrFindWhat, pStr );
            lpBufPtr = pStr;
            bFoundLast = TRUE;
            MessageBox( lpFR->hwndOwner, Buf, "Success!", MB_OK | MB_TASKMODAL );
            }
        else if ( lpFR->Flags & FR_REPLACE )
            {
            // replace string specified in the replace with found string
            // copy up to found string into new buffer
            for( count=0; 
                 *pStr && lpBufPtr[count] && *pStr != lpBufPtr[count]; 
                 count++);
                strncpy( Buf, lpBufPtr, count );
            // concatenate new string
            strcat( Buf, lpFR->lpstrReplaceWith );
            // copy rest of string (less the found string)
            newcount = count + strlen(lpFR->lpstrFindWhat);
            strcat( Buf, lpBufPtr+newcount);
            strcpy( lpBufPtr, Buf );
            lpBufPtr += count + strlen(lpFR->lpstrReplaceWith);
            dwFileSize = strlen(FileBuf);
            MessageBox( lpFR->hwndOwner, FileBuf, "Success!", MB_OK | MB_TASKMODAL );
            }
        else if ( lpFR->Flags & FR_REPLACEALL)
            {
            do
                {
                // replace string specified in the replace with found string
                // copy up to found string into new buffer
                memset(Buf, '\0', FILE_LEN -1);
                for( count=0; 
                     *pStr && lpBufPtr[count] && *pStr != lpBufPtr[count]; 
                     count++);
                     strncpy( Buf, lpBufPtr, count );
                // concatenate new string
                strcat( Buf, lpFR->lpstrReplaceWith );
                // copy rest of string (less the found string)
                newcount = count + strlen(lpFR->lpstrFindWhat);
                strcat( Buf, lpBufPtr + newcount);
                strcpy( lpBufPtr, Buf );
                lpBufPtr += count + strlen(lpFR->lpstrReplaceWith);
                }
            while ( *lpBufPtr && 
                    (pStr = strstr( lpBufPtr, lpFR->lpstrFindWhat ) ) );
            dwFileSize = strlen(FileBuf);
            lpBufPtr = FileBuf;
            MessageBox( lpFR->hwndOwner, FileBuf, 
                        "Success!", MB_OK | MB_TASKMODAL );
            }

        }
    }
}


/****************************************************************************
*
*    FUNCTION: ProcessCDError(DWORD)
*
*    PURPOSE:  Processes errors from the common dialog functions.
*
*    COMMENTS:
*
*        This function is called whenever a common dialog function
*        fails.  The CommonDialogExtendedError() value is passed to
*        the function which maps the error value to a string table.
*        The string is loaded and displayed for the user.
*
*    RETURN VALUES:
*        void.
*
****************************************************************************/
void ProcessCDError(DWORD dwErrorCode, HWND hWnd)
{
   WORD  wStringID;
   CHAR  buf[256];

   switch(dwErrorCode)
      {
         case CDERR_DIALOGFAILURE:   wStringID=IDS_DIALOGFAILURE;   break;
         case CDERR_STRUCTSIZE:      wStringID=IDS_STRUCTSIZE;      break;
         case CDERR_INITIALIZATION:  wStringID=IDS_INITIALIZATION;  break;
         case CDERR_NOTEMPLATE:      wStringID=IDS_NOTEMPLATE;      break;
         case CDERR_NOHINSTANCE:     wStringID=IDS_NOHINSTANCE;     break;
         case CDERR_LOADSTRFAILURE:  wStringID=IDS_LOADSTRFAILURE;  break;
         case CDERR_FINDRESFAILURE:  wStringID=IDS_FINDRESFAILURE;  break;
         case CDERR_LOADRESFAILURE:  wStringID=IDS_LOADRESFAILURE;  break;
         case CDERR_LOCKRESFAILURE:  wStringID=IDS_LOCKRESFAILURE;  break;
         case CDERR_MEMALLOCFAILURE: wStringID=IDS_MEMALLOCFAILURE; break;
         case CDERR_MEMLOCKFAILURE:  wStringID=IDS_MEMLOCKFAILURE;  break;
         case CDERR_NOHOOK:          wStringID=IDS_NOHOOK;          break;
         case PDERR_SETUPFAILURE:    wStringID=IDS_SETUPFAILURE;    break;
         case PDERR_PARSEFAILURE:    wStringID=IDS_PARSEFAILURE;    break;
         case PDERR_RETDEFFAILURE:   wStringID=IDS_RETDEFFAILURE;   break;
         case PDERR_LOADDRVFAILURE:  wStringID=IDS_LOADDRVFAILURE;  break;
         case PDERR_GETDEVMODEFAIL:  wStringID=IDS_GETDEVMODEFAIL;  break;
         case PDERR_INITFAILURE:     wStringID=IDS_INITFAILURE;     break;
         case PDERR_NODEVICES:       wStringID=IDS_NODEVICES;       break;
         case PDERR_NODEFAULTPRN:    wStringID=IDS_NODEFAULTPRN;    break;
         case PDERR_DNDMMISMATCH:    wStringID=IDS_DNDMMISMATCH;    break;
         case PDERR_CREATEICFAILURE: wStringID=IDS_CREATEICFAILURE; break;
         case PDERR_PRINTERNOTFOUND: wStringID=IDS_PRINTERNOTFOUND; break;
         case CFERR_NOFONTS:         wStringID=IDS_NOFONTS;         break;
         case FNERR_SUBCLASSFAILURE: wStringID=IDS_SUBCLASSFAILURE; break;
         case FNERR_INVALIDFILENAME: wStringID=IDS_INVALIDFILENAME; break;
         case FNERR_BUFFERTOOSMALL:  wStringID=IDS_BUFFERTOOSMALL;  break;

         case 0:   //User may have hit CANCEL or we got a *very* random error
            return;

         default:
            wStringID=IDS_UNKNOWNERROR;
      }

   LoadString(NULL, wStringID, buf, sizeof(buf));
   MessageBox(hWnd, buf, NULL, MB_OK);
   return;
}

/***********************************************************************/

BOOL WINAPI OpenFiles(
        HWND hDlg,                /* window handle of the dialog box */
        UINT message,             /* type of message                 */
        WPARAM wParam,            /* message-specific information    */
        LPARAM lParam)
{
    int    Length;
    char   DevProfileName[FILE_LEN];
    char   TargetProfileName[FILE_LEN];
    char   OutputFileName[FILE_LEN];
    WORD   InpClrSp;

    switch (message)
    {
        case WM_INITDIALOG:                /* message: initialize dialog box */
            return (TRUE);

        case WM_COMMAND:                      /* message: received a command */
            switch (LOWORD(wParam))
            {
                case IDC_BUTTON1: 
                    if ( OpenNewFile( hWnd ) == TRUE )
                        SendDlgItemMessage(hDlg, IDC_EDIT1, WM_SETTEXT, 0, (long)(OpenFileName.lpstrFile)); 
                    break;
                case IDC_BUTTON2:
                    if ( OpenNewFile( hWnd ) == TRUE )
                        SendDlgItemMessage(hDlg, IDC_EDIT2, WM_SETTEXT, 0, (long)(OpenFileName.lpstrFile));
                    break;
                case IDC_BUTTON3:
                    if ( SaveAs( hWnd ) == TRUE )
                        SendDlgItemMessage(hDlg, IDC_EDIT3, WM_SETTEXT, 0, (long)(OpenFileName.lpstrFile));
                    break;
                case IDOK:
                    Length = SendDlgItemMessage(hDlg, IDC_EDIT1, WM_GETTEXTLENGTH, 0, 0);
                    SendDlgItemMessage(hDlg, IDC_EDIT1, WM_GETTEXT, Length + 1, (long)(DevProfileName));
                    Length = SendDlgItemMessage(hDlg, IDC_EDIT2, WM_GETTEXTLENGTH, 0, 0);
                    SendDlgItemMessage(hDlg, IDC_EDIT2, WM_GETTEXT, Length + 1, (long)(TargetProfileName));
                    Length = SendDlgItemMessage(hDlg, IDC_EDIT3, WM_GETTEXTLENGTH, 0, 0);
                    SendDlgItemMessage(hDlg, IDC_EDIT3, WM_GETTEXT, Length + 1, (long)(OutputFileName));
                    switch (wInpDrvClrSp)
                    {
                        case IDM_INP_AUTO: InpClrSp = 0; break;
                        case IDM_INP_GRAY: InpClrSp = 1; break;
                        case IDM_INP_RGB:  InpClrSp = 3; break;
                        case IDM_INP_CMYK: InpClrSp = 4; break;
                        default: InpClrSp = 0; break;
                    }
                    if (wCSAorCRD == IDM_CSA)
                    {
                        ColorSpaceControl(DevProfileName, OutputFileName,
                            InpClrSp, Intent, wCSAMode, AllowBinary);
                    }
                    else if (wCSAorCRD == IDM_CRD)
                    {
                        CreateCRDControl(DevProfileName, OutputFileName,
                            Intent, AllowBinary);
                    }
                    else if (wCSAorCRD == IDM_PROFCRD)
                    {
                        CreateProfCRDControl(DevProfileName, 
                            TargetProfileName, OutputFileName,
                            Intent, AllowBinary);
                    }
                    else 
                    {
                        CreateINTENTControl(DevProfileName, OutputFileName, Intent);
                    }

                case IDCANCEL:
                    EndDialog(hDlg, TRUE);        /* Exits the dialog box        */
                    return (TRUE);
            }
            break;
    }
    return (FALSE);                           /* Didn't process a message    */

    // avoid compiler warnings at W3
    lParam;
}
