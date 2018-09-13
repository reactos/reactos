#define UNICODE
#include <windows.h>		    /* required for all Windows applications */
#include <stdio.h>
#include <stdlib.h>
#include "test.h"		    /* specific to this program		     */

HANDLE hInst;			    /* current instance			     */
HMODULE	hMod=NULL;

TCHAR szInfoBuf[1024];
TCHAR szBuffer[256];
TCHAR szFileName[MAX_PATH];
WORD	languageid;

/****************************************************************************

    FUNCTION: WinMain(HANDLE, HANDLE, LPSTR, int)

    PURPOSE: calls initialization function, processes message loop

    COMMENTS:

        Windows recognizes this function by name as the initial entry point
        for the program.  This function calls the application initialization
        routine, if no other instance of the program is running, and always
        calls the instance initialization routine.  It then executes a message
        retrieval and dispatch loop that is the top-level control structure
        for the remainder of execution.  The loop is terminated when a WM_QUIT
        message is received, at which time this function exits the application
        instance by returning the value passed by PostQuitMessage().

        If this function must abort before entering the message loop, it
        returns the conventional value NULL.

****************************************************************************/

int WINAPI WinMain(
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPSTR lpCmdLine,
    int nCmdShow
    )
{

    MSG msg;				     /* message			     */

    UNREFERENCED_PARAMETER( lpCmdLine );

    if (!hPrevInstance)			 /* Other instances of app running? */
	if (!InitApplication(hInstance)) /* Initialize shared things */
	    return (FALSE);		 /* Exits if unable to initialize     */

    /* Perform initializations that apply to a specific instance */

    if (!InitInstance(hInstance, nCmdShow))
        return (FALSE);

    /* Acquire and dispatch messages until a WM_QUIT message is received. */

    while (GetMessage(&msg,	   /* message structure			     */
	    NULL,		   /* handle of window receiving the message */
	    0,   		   /* lowest message to examine		     */
	    0))                    /* highest message to examine	     */
	{
	TranslateMessage(&msg);	   /* Translates virtual key codes	     */
	DispatchMessage(&msg);	   /* Dispatches message to window	     */
    }
    return (msg.wParam);	   /* Returns the value from PostQuitMessage */
}


/****************************************************************************

    FUNCTION: InitApplication(HANDLE)

    PURPOSE: Initializes window data and registers window class

    COMMENTS:

        This function is called at initialization time only if no other
        instances of the application are running.  This function performs
        initialization tasks that can be done once for any number of running
        instances.

        In this case, we initialize a window class by filling out a data
        structure of type WNDCLASS and calling the Windows RegisterClass()
        function.  Since all instances of this application use the same window
        class, we only need to do this when the first instance is initialized.


****************************************************************************/

BOOL InitApplication(HANDLE hInstance)       /* current instance	     */
{
    WNDCLASS  wc;

    /* Fill in window class structure with parameters that describe the       */
    /* main window.                                                           */

    wc.style = 0;                       /* Class style(s).                    */
    wc.lpfnWndProc = (WNDPROC)MainWndProc;       /* Function to retrieve messages for  */
                                        /* windows of this class.             */
    wc.cbClsExtra = 0;                  /* No per-class extra data.           */
    wc.cbWndExtra = 0;                  /* No per-window extra data.          */
    wc.hInstance = hInstance;          /* Application that owns the class.   */
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = GetStockObject(WHITE_BRUSH);
    wc.lpszMenuName =  MAKEINTRESOURCE(IDM_VERSION);
    wc.lpszClassName = TEXT("VerTestWClass"); /* Name used in call to CreateWindow. */

    /* Register the window class and return success/failure code. */

    return (RegisterClass(&wc));

}


/****************************************************************************

    FUNCTION:  InitInstance(HANDLE, int)

    PURPOSE:  Saves instance handle and creates main window

    COMMENTS:

        This function is called at initialization time for every instance of
        this application.  This function performs initialization tasks that
        cannot be shared by multiple instances.

        In this case, we save the instance handle in a static variable and
        create and display the main program window.

****************************************************************************/

BOOL InitInstance(
    HANDLE          hInstance,          /* Current instance identifier.       */
    int             nCmdShow)           /* Param for first ShowWindow() call. */
{
    HWND            hWnd;               /* Main window handle.                */

    /* Save the instance handle in static variable, which will be used in  */
    /* many subsequence calls from this application to Windows.            */

    hInst = hInstance;

    /* Create a main window for this application instance.  */

    hWnd = CreateWindow(
        TEXT("VerTestWClass"),          /* See RegisterClass() call.          */
        TEXT("Version Test"),           /* Text for window title bar.         */
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

    if (!hWnd)
        return (FALSE);

    /* Make the window visible; update its client area; and return "success" */

    ShowWindow(hWnd, nCmdShow);  /* Show the window                        */
    UpdateWindow(hWnd);          /* Sends WM_PAINT message                 */
    return (TRUE);               /* Returns the value from PostQuitMessage */

}

/****************************************************************************

    FUNCTION: MainWndProc(HWND, unsigned, WORD, LONG)

    PURPOSE:  Processes messages

    MESSAGES:

	WM_COMMAND    - application menu
	WM_DESTROY    - destroy window

    COMMENTS:

****************************************************************************/

LONG APIENTRY MainWndProc(
    HWND hWnd,		  /* window handle		     */
    UINT message,		  /* type of message		     */
    UINT wParam,		  /* additional information	     */
    LONG lParam)		  /* additional information	     */
{
    DLGPROC lpProc;	  /* pointer to the dialog functions */
    TCHAR	szLang[256];
    TCHAR	szBuf[256];
    PVOID	pData;
    INT		cbData;
    INT		dummy;
    VS_FIXEDFILEINFO	*pvs;
    DWORD	*pdw;
    WORD	*pw;
    PAINTSTRUCT ps;

    switch (message) {
	case WM_COMMAND:	   /* message: command from application menu */
	    switch (LOWORD(wParam)) {

	    case IDM_ABOUT:
		lpProc = (DLGPROC)MakeProcInstance((FARPROC)About, hInst);

    		DialogBox(hInst,		 /* current instance	     */
			MAKEINTRESOURCE(IDD_ABOUT),	 /* resource to use  */
		        hWnd,			 /* parent handle	     */
		        (DLGPROC)lpProc);	 /* About() instance address */

    		FreeProcInstance(lpProc);
	    	break;

	    case IDM_FREE:
		FreeLibrary(hMod);
		break;

	    case IDM_EXIT:
		FreeLibrary(hMod);
		DestroyWindow(hWnd);
		break;

	    case IDM_QUERY:
		lpProc = (DLGPROC)MakeProcInstance((FARPROC)Query, hInst);
    		DialogBox(hInst,		 /* current instance	     */
			MAKEINTRESOURCE(IDD_QUERY),	 /* resource to use  */
		        hWnd,			 /* parent handle	     */
		        (DLGPROC)lpProc);	 /* About() instance address */
    		FreeProcInstance(lpProc);
		if (VerQueryValue(szInfoBuf, szFileName, &pData, &cbData) == FALSE)
		    MessageBox(hWnd, TEXT("Returned NULL"), TEXT("VerQueryValue"), MB_OK);
		else {
		    if (lstrcmp(szFileName, TEXT("\\")) == 0) {
			pvs = (VS_FIXEDFILEINFO*)pData;
			wsprintf(szBuffer, TEXT("0x%lx 0x%lx 0x%lx 0x%lx 0x%lx 0x%lx 0x%lx 0x%lx 0x%lx 0x%lx 0x%lx 0x%lx 0x%lx "),
				pvs->dwSignature,
				pvs->dwStrucVersion,
				pvs->dwFileVersionMS,
				pvs->dwFileVersionLS,
				pvs->dwProductVersionMS,
				pvs->dwProductVersionLS,
				pvs->dwFileFlagsMask,
				pvs->dwFileFlags,
				pvs->dwFileOS,
				pvs->dwFileType,
				pvs->dwFileSubtype,
				pvs->dwFileDateMS,
				pvs->dwFileDateLS);
			MessageBox(hWnd, szBuffer, TEXT("VerQueryValue VS_FIXEDFILEINFO"), MB_OK);
		    }
		    else if (lstrcmpi(szFileName, TEXT("\\VarFileInfo\\Translation")) == 0) {
			pw = (WORD*)pData;	/* assume 2 words */
			wsprintf(szBuffer, TEXT("0x%lx 0x%lx"), *pw, *(pw+1));
			MessageBox(hWnd, szBuffer, TEXT("VerQueryValue VS_FIXEDFILEINFO"), MB_OK);
		    }
    		    else if (
#ifndef UNICODE
                             strnicmp
#else
                             _wcsnicmp
#endif
		                     (szFileName, TEXT("\\StringFileInfo\\"), 16) == 0) {
			wsprintf(szBuffer, TEXT("%s:%ws"), szFileName, pData);
			MessageBox(hWnd, szBuffer, TEXT("VerQueryValue"), MB_OK);
		    }
		    else
			MessageBox(hWnd, TEXT("Other"), TEXT("VerQueryValue"), MB_OK);
		}
		break;

	    case IDM_INFO:
		lpProc = (DLGPROC)MakeProcInstance((FARPROC)Information, hInst);
    		DialogBox(hInst,		 /* current instance	     */
			MAKEINTRESOURCE(IDD_INFO),	 /* resource to use  */
		        hWnd,			 /* parent handle	     */
		        (DLGPROC)lpProc);	 /* About() instance address */
    		FreeProcInstance(lpProc);
		if (GetFileVersionInfoSize(szFileName, NULL) == FALSE)
		    MessageBox(hWnd, TEXT("Returned NULL"), TEXT("GetFileVersionInfoSize"), MB_OK);
		else {
		    if (GetFileVersionInfo(szFileName, 0, 1024, szInfoBuf) == FALSE)
			MessageBox(hWnd, TEXT("Returned NULL"), TEXT("GetFileVersionInfo"), MB_OK);
		    else {
			MessageBox(hWnd, TEXT("Returned OK"), TEXT("GetFileVersionInfo"), MB_OK);
		    }
		}
		break;

	    case IDM_FIND:
		lpProc = (DLGPROC)MakeProcInstance((FARPROC)Find, hInst);
    		DialogBox(hInst,		 /* current instance	     */
			MAKEINTRESOURCE(IDD_FIND),	 /* resource to use  */
		        hWnd,			 /* parent handle	     */
		        (DLGPROC)lpProc);	 /* About() instance address */
    		FreeProcInstance(lpProc);
		GetWindowsDirectory(szBuf, 256);
		dummy = 256;
		cbData = 1024;
		if (VerFindFile(0L, szFileName, szBuf, TEXT("c:\\tmp"), szLang, &dummy, szInfoBuf, &cbData) == FALSE)
		    MessageBox(hWnd, TEXT("Returned NULL"), TEXT("VerFindFile"), MB_OK);
		else {
		    wsprintf(szBuffer, TEXT("%s:%s"), szLang, szInfoBuf);
		    MessageBox(hWnd, szBuffer, TEXT("VerFindFile"), MB_OK);
		}
		break;

	    case IDM_INSTALL:
		lpProc = (DLGPROC)MakeProcInstance((FARPROC)Install, hInst);
    		DialogBox(hInst,		 /* current instance	     */
			MAKEINTRESOURCE(IDD_INSTALL),	 /* resource to use  */
		        hWnd,			 /* parent handle	     */
		        (DLGPROC)lpProc);	 /* About() instance address */
    		FreeProcInstance(lpProc);

		dummy = 256;
		cbData = VerInstallFile(VIFF_FORCEINSTALL|VIFF_DONTDELETEOLD,
			szFileName, szFileName, TEXT("."), szLang, szInfoBuf,
			szBuf, &dummy);
		if (cbData == 0)
		    MessageBox(hWnd, TEXT("Returned NULL"), TEXT("VerInstallFile"), MB_OK);
		else {
		    wsprintf(szBuffer, TEXT("0x%lx:%s"), cbData, szBuf);
		    MessageBox(hWnd, szBuffer, TEXT("VerInstallFile"), MB_OK);
		}
		break;

	    case IDM_LANG:
		lpProc = (DLGPROC)MakeProcInstance((FARPROC)Language, hInst);
    		DialogBox(hInst,		 /* current instance	     */
			MAKEINTRESOURCE(IDD_LANG),	 /* resource to use  */
		        hWnd,			 /* parent handle	     */
		        (DLGPROC)lpProc);	 /* About() instance address */
    		FreeProcInstance(lpProc);
		VerLanguageName(languageid, szLang, 256);
		MessageBox(hWnd, szLang, TEXT("Language ID is:"), MB_OK);
		break;

	    default:
		    return (DefWindowProc(hWnd, message, wParam, lParam));
	    }

	case WM_PAINT:
            BeginPaint(hWnd, (LPPAINTSTRUCT)&ps);
            EndPaint(hWnd, (LPPAINTSTRUCT)&ps);
	    break;

	case WM_DESTROY:		  /* message: window being destroyed */
	    PostQuitMessage(0);
	    break;

	default:			  /* Passes it on if unproccessed    */
	    return (DefWindowProc(hWnd, message, wParam, lParam));
    }
    return (0);
}


/****************************************************************************

    FUNCTION: About(HWND, unsigned, WORD, LONG)

    PURPOSE:  Processes messages for "About" dialog box

    MESSAGES:

	WM_INITDIALOG - initialize dialog box
	WM_COMMAND    - Input received

    COMMENTS:

	No initialization is needed for this particular dialog box, but TRUE
	must be returned to Windows.

	Wait for user to click on "Ok" button, then close the dialog box.

****************************************************************************/

BOOL APIENTRY About(
	HWND hDlg,                /* window handle of the dialog box */
	UINT message,             /* type of message                 */
	UINT wParam,		/* message-specific information    */
	LONG lParam)
{
    switch (message) {
	case WM_INITDIALOG:		   /* message: initialize dialog box */
	    return (TRUE);

	case WM_COMMAND:		      /* message: received a command */
	    if (LOWORD(wParam) == IDOK)		/* "OK" box selected?	     */
		EndDialog(hDlg, TRUE);	      /* Exits the dialog box	     */
	    else if (LOWORD(wParam) == IDCANCEL) /* close command? */
		EndDialog(hDlg, FALSE);	      /* Exits the dialog box	     */
	    return (TRUE);
    }
    return (FALSE);			      /* Didn't process a message    */
	UNREFERENCED_PARAMETER(lParam);
}


/****************************************************************************

    FUNCTION: Find(HWND, unsigned, WORD, LONG)

    PURPOSE:  Processes messages for "Find" dialog box

    MESSAGES:

	WM_INITDIALOG - initialize dialog box
	WM_COMMAND    - Input received

    COMMENTS:

	No initialization is needed for this particular dialog box, but TRUE
	must be returned to Windows.

	Wait for user to click on "Ok" button, then close the dialog box.

****************************************************************************/

BOOL APIENTRY Find(
	HWND hDlg,                /* window handle of the dialog box */
	UINT message,             /* type of message                 */
	UINT wParam,		/* message-specific information    */
	LONG lParam)
{
    switch (message) {
	case WM_INITDIALOG:		   /* message: initialize dialog box */
	    return (TRUE);

	case WM_COMMAND:		/* message: received a command */
	    if (LOWORD(wParam) == IDOK)	{	/* "OK" box selected?	     */
		GetDlgItemText(hDlg, IDC_FILENAME, szFileName, MAX_PATH);
		EndDialog(hDlg, TRUE);	      /* Exits the dialog box	     */
	    }
	    else if (LOWORD(wParam) == IDCANCEL) /* close command? */
		EndDialog(hDlg, FALSE);	      /* Exits the dialog box	     */
	    return (TRUE);
	    break;
    }
    return (FALSE);			      /* Didn't process a message    */
	UNREFERENCED_PARAMETER(lParam);
}

/****************************************************************************

    FUNCTION: Install(HWND, unsigned, WORD, LONG)

    PURPOSE:  Processes messages for "Install" dialog box

    MESSAGES:

	WM_INITDIALOG - initialize dialog box
	WM_COMMAND    - Input received

    COMMENTS:

	No initialization is needed for this particular dialog box, but TRUE
	must be returned to Windows.

	Wait for user to click on "Ok" button, then close the dialog box.

****************************************************************************/

BOOL APIENTRY Install(
	HWND hDlg,                /* window handle of the dialog box */
	UINT message,             /* type of message                 */
	UINT wParam,		/* message-specific information    */
	LONG lParam)
{
    switch (message) {
	case WM_INITDIALOG:		   /* message: initialize dialog box */
	    return (TRUE);

	case WM_COMMAND:		/* message: received a command */
	    if (LOWORD(wParam) == IDOK)	{	/* "OK" box selected?	     */
		GetDlgItemText(hDlg, IDC_FILENAME, szFileName, MAX_PATH);
		EndDialog(hDlg, TRUE);	      /* Exits the dialog box	     */
	    }
	    else if (LOWORD(wParam) == IDCANCEL) /* close command? */
		EndDialog(hDlg, FALSE);	      /* Exits the dialog box	     */
	    return (TRUE);
	    break;
    }
    return (FALSE);			      /* Didn't process a message    */
	UNREFERENCED_PARAMETER(lParam);
}

/****************************************************************************

    FUNCTION: Query(HWND, unsigned, WORD, LONG)

    PURPOSE:  Processes messages for "Query" dialog box

    MESSAGES:

	WM_INITDIALOG - initialize dialog box
	WM_COMMAND    - Input received

    COMMENTS:

	No initialization is needed for this particular dialog box, but TRUE
	must be returned to Windows.

	Wait for user to click on "Ok" button, then close the dialog box.

****************************************************************************/

BOOL APIENTRY Query(
	HWND hDlg,                /* window handle of the dialog box */
	UINT message,             /* type of message                 */
	UINT wParam,		/* message-specific information    */
	LONG lParam)
{
    switch (message) {
	case WM_INITDIALOG:		   /* message: initialize dialog box */
	    return (TRUE);

	case WM_COMMAND:		/* message: received a command */
	    if (LOWORD(wParam) == IDOK)	{	/* "OK" box selected?	     */
		GetDlgItemText(hDlg, IDC_FILENAME, szFileName, MAX_PATH);
		EndDialog(hDlg, TRUE);	      /* Exits the dialog box	     */
	    }
	    else if (LOWORD(wParam) == IDCANCEL) /* close command? */
		EndDialog(hDlg, FALSE);	      /* Exits the dialog box	     */
	    return (TRUE);
	    break;
    }
    return (FALSE);			      /* Didn't process a message    */
	UNREFERENCED_PARAMETER(lParam);
}

/****************************************************************************

    FUNCTION: Language(HWND, unsigned, WORD, LONG)

    PURPOSE:  Processes messages for "Language" dialog box

    MESSAGES:

	WM_INITDIALOG - initialize dialog box
	WM_COMMAND    - Input received

    COMMENTS:

	No initialization is needed for this particular dialog box, but TRUE
	must be returned to Windows.

	Wait for user to click on "Ok" button, then close the dialog box.

****************************************************************************/

BOOL APIENTRY Language(
	HWND hDlg,                /* window handle of the dialog box */
	UINT message,             /* type of message                 */
	UINT wParam,		/* message-specific information    */
	LONG lParam)
{
    UINT	lang;
    UINT	sublang;

    switch (message) {
	case WM_INITDIALOG:		   /* message: initialize dialog box */
	    return (TRUE);

	case WM_COMMAND:		/* message: received a command */
	    if (LOWORD(wParam) == IDOK)	{	/* "OK" box selected?	     */
		lang = GetDlgItemInt(hDlg, IDC_LANGID, NULL, FALSE);
		sublang = GetDlgItemInt(hDlg, IDC_SUBLANGID, NULL, FALSE);
		languageid = MAKELANGID(lang, sublang);
		EndDialog(hDlg, TRUE);	      /* Exits the dialog box	     */
	    }
	    else if (LOWORD(wParam) == IDCANCEL) /* close command? */
		EndDialog(hDlg, FALSE);	      /* Exits the dialog box	     */
	    return (TRUE);
	    break;
    }
    return (FALSE);			      /* Didn't process a message    */
	UNREFERENCED_PARAMETER(lParam);
}

/****************************************************************************

    FUNCTION: Information(HWND, unsigned, WORD, LONG)

    PURPOSE:  Processes messages for "Information" dialog box

    MESSAGES:

	WM_INITDIALOG - initialize dialog box
	WM_COMMAND    - Input received

    COMMENTS:

	No initialization is needed for this particular dialog box, but TRUE
	must be returned to Windows.

	Wait for user to click on "Ok" button, then close the dialog box.

****************************************************************************/

BOOL APIENTRY Information(
	HWND hDlg,                /* window handle of the dialog box */
	UINT message,             /* type of message                 */
	UINT wParam,		/* message-specific information    */
	LONG lParam)
{
    switch (message) {
	case WM_INITDIALOG:		   /* message: initialize dialog box */
	    return (TRUE);

	case WM_COMMAND:		/* message: received a command */
	    if (LOWORD(wParam) == IDOK)	{	/* "OK" box selected?	     */
		GetDlgItemText(hDlg, IDC_FILENAME, szFileName, MAX_PATH);
		EndDialog(hDlg, TRUE);	      /* Exits the dialog box	     */
	    }
	    else if (LOWORD(wParam) == IDCANCEL) /* close command? */
		EndDialog(hDlg, FALSE);	      /* Exits the dialog box	     */
	    return (TRUE);
	    break;
    }
    return (FALSE);			      /* Didn't process a message    */
	UNREFERENCED_PARAMETER(lParam);
}
