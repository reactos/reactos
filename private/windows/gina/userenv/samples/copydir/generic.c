/****************************************************************************

        PROGRAM: Generic.c

        PURPOSE: Generic template for Windows applications

****************************************************************************/

#include <windows.h>
#include <userenv.h>
#include <userenvp.h>
#include "generic.h"


HINSTANCE hInst;
HWND      hwndMain;
HANDLE    hProfile, hUserToken;

TCHAR szAppName[] = TEXT("Generic");
#ifdef UNICODE
TCHAR szTitle[]   = TEXT("UserEnv Profile Test App - UNICODE");
#else
TCHAR szTitle[]   = TEXT("UserEnv Profile Test App - ANSI");
#endif


TCHAR szSrcDir[MAX_PATH];
TCHAR szDestDir[MAX_PATH];
TCHAR szProfilePath[MAX_PATH];
TCHAR szDefaultPath[MAX_PATH];
TCHAR szUserName[MAX_PATH];
TCHAR szDomainName[MAX_PATH];

void ChangeMenuState (BOOL bLoggedOn);

/****************************************************************************

        FUNCTION: WinMain(HINSTANCE, HINSTANCE, LPSTR, int)

        PURPOSE: calls initialization function, processes message loop

****************************************************************************/
int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                     LPSTR lpCmdLine, INT nCmdShow)
{
        MSG msg;
        HANDLE hAccelTable;

        if (!hPrevInstance)
           {
           if (!InitApplication(hInstance))
              {
              return (FALSE);
              }
           }


        // Perform initializations that apply to a specific instance
        if (!InitInstance(hInstance, nCmdShow))
           {
           return (FALSE);
           }

        hAccelTable = LoadAccelerators (hInstance, szAppName);

        while (GetMessage(&msg, NULL, 0, 0))
           {
           if (!TranslateAccelerator (msg.hwnd, hAccelTable, &msg))
              {
              TranslateMessage(&msg);
              DispatchMessage(&msg);
              }
           }


        return (msg.wParam);

        lpCmdLine;
}


/****************************************************************************

        FUNCTION: InitApplication(HINSTANCE)

        PURPOSE: Initializes window data and registers window class

****************************************************************************/

BOOL InitApplication(HINSTANCE hInstance)
{
        WNDCLASS  wc;

        wc.style         = CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc   = (WNDPROC)WndProc;
        wc.cbClsExtra    = 0;
        wc.cbWndExtra    = 0;
        wc.hInstance     = hInstance;
        wc.hIcon         = LoadIcon (hInstance, szAppName);
        wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
        wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
        wc.lpszMenuName  = szAppName;
        wc.lpszClassName = szAppName;

        return (RegisterClass(&wc));
}


/****************************************************************************

        FUNCTION:  InitInstance(HINSTANCE, int)

        PURPOSE:  Saves instance handle and creates main window

****************************************************************************/

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
        HWND  hWnd;

        hInst = hInstance;
        hProfile = NULL;

        szSrcDir[0] = TEXT('\0');
        szDestDir[0] = TEXT('\0');
        szProfilePath[0] = TEXT('\0');
        szDefaultPath[0] = TEXT('\0');
        szUserName[0] = TEXT('\0');
        szDomainName[0] = TEXT('\0');

        hWnd = CreateWindow(szAppName,
                            szTitle,
                            WS_OVERLAPPEDWINDOW,
                            CW_USEDEFAULT, 0, CW_USEDEFAULT, 0,
                            NULL,
                            NULL,
                            hInstance,
                            NULL);

        if (!hWnd)
           {
           return (FALSE);
           }
        else
          {
          hwndMain = hWnd;
          }

        ChangeMenuState(FALSE);
        PostMessage (hWnd, WM_COMMAND, IDM_LOGON, 0);

        ShowWindow(hWnd, SW_SHOWDEFAULT);
        UpdateWindow(hWnd);

        return (TRUE);

}

/****************************************************************************

        FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)

        PURPOSE:  Processes messages

****************************************************************************/

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
        DWORD dwFlags;
        TCHAR szBuffer[MAX_PATH];

        switch (message)
           {
           case WM_COMMAND:
              {
              switch (LOWORD(wParam))
                 {
                 case IDM_LOGON:
                    if (DialogBox (hInst, TEXT("LOGONDLG"), hWnd, LogonDlgProc)) {
                        ChangeMenuState(TRUE);
                    }
                    break;

                 case IDM_LOGOFF:
                    CloseHandle(hUserToken);
                    ChangeMenuState(FALSE);
                    break;

                 case IDM_LUP:
                    DialogBox (hInst, TEXT("LUPDLG"), hWnd, LUPDlgProc);
                    break;

                 case IDM_ULUP:
                    if (hProfile) {
                        if (UnloadUserProfile(hUserToken, hProfile)) {
                            hProfile = NULL;
                        }
                    }
                    break;

                 case IDM_PFTYPE:
                    if (hUserToken) {
                        if (!ImpersonateLoggedOnUser(hUserToken)) {
                            wsprintf(szBuffer, TEXT("Impersonate Failed with Error %d"), GetLastError());
                            MessageBox (hWnd, szBuffer, TEXT("Impersonate"), MB_OK | MB_ICONEXCLAMATION);
                            return TRUE;
                        }

                        if (GetProfileType(&dwFlags)) {
                        }

                        RevertToSelf();
                    }
                    break;

                 case IDM_ABOUT:
                    DialogBox (hInst, TEXT("AboutBox"), hWnd, About);
                    break;

                 case IDM_EXIT:
                    DestroyWindow (hwndMain);
                    break;


                 default:
                    return (DefWindowProc(hWnd, message, wParam, lParam));
                 }
              }
              break;

           case WM_DESTROY:
              if (hProfile) {
                  if (UnloadUserProfile(hUserToken, hProfile)) {
                      hProfile = NULL;
                  }
              }

              PostQuitMessage(0);
              break;

           default:
              return (DefWindowProc(hWnd, message, wParam, lParam));
           }

        return FALSE;
}

/****************************************************************************

        FUNCTION: About(HWND, UINT, WPARAM, LPARAM)

        PURPOSE:  Processes messages for "About" dialog box

****************************************************************************/

LRESULT CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{

        switch (message)
           {
           case WM_COMMAND:
              if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
                 {
                 EndDialog(hDlg, TRUE);
                 return (TRUE);
                 }
              break;
           }

        return (FALSE);

        lParam;
}


/****************************************************************************

        FUNCTION: LUPDlgProc(HWND, UINT, WPARAM, LPARAM)

****************************************************************************/

LRESULT CALLBACK LUPDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{

        switch (message)
           {
           case WM_INITDIALOG:
              SetDlgItemText(hDlg, IDD_PROFILEPATH, szProfilePath);
              SetDlgItemText(hDlg, IDD_DEFAULTPATH, szDefaultPath);
              SetFocus (GetDlgItem (hDlg, IDD_PROFILEPATH));

              return FALSE;

           case WM_COMMAND:
              if (LOWORD(wParam) == IDOK)
                  {
                  PROFILEINFO pi;
                  DWORD dwStart, dwEnd, dwFlags = 0;
                  TCHAR szBuffer[200];
                  HCURSOR hCursor;
                  BOOL bResult;

                  SetDlgItemText(hDlg, IDD_RETVAL, NULL);
                  SetDlgItemText(hDlg, IDD_TIME, NULL);
                  SetDlgItemText(hDlg, IDD_PROFILE, NULL);


                  GetDlgItemText(hDlg, IDD_PROFILEPATH, szProfilePath, MAX_PATH);
                  GetDlgItemText(hDlg, IDD_DEFAULTPATH, szDefaultPath, MAX_PATH);

                  if (IsDlgButtonChecked(hDlg, IDD_NOUI)) {
                    dwFlags |= PI_NOUI;
                  }

                  if (IsDlgButtonChecked(hDlg, IDD_APPLYPOLICY)) {
                    dwFlags |= PI_APPLYPOLICY;
                    pi.lpPolicyPath = TEXT("ntconfig.pol");
                  }

                  if (IsDlgButtonChecked(hDlg, IDD_LITELOAD)) {
                    dwFlags |= PI_LITELOAD;
                  }

                  pi.dwSize = sizeof(PROFILEINFO);
                  pi.dwFlags = dwFlags;
                  pi.lpUserName = szUserName;
                  pi.lpProfilePath = szProfilePath;
                  pi.lpDefaultPath = szDefaultPath;
                  pi.lpServerName = NULL;

                  hCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));

                  dwStart = GetTickCount();
                  bResult = LoadUserProfile(hUserToken, &pi);
                  dwEnd = GetTickCount();

                  SetCursor(hCursor);

                  SetDlgItemInt(hDlg, IDD_TIME, dwEnd - dwStart, FALSE);
                  SetDlgItemInt(hDlg, IDD_RETVAL, bResult, FALSE);

                  if (bResult) {
                      hProfile = pi.hProfile;

                      wsprintf (szBuffer, TEXT("0x%x"), pi.hProfile);
                      SetDlgItemText(hDlg, IDD_PROFILE, szBuffer);
                  }

                  }

              if (LOWORD(wParam) == IDCANCEL)
                 {
                 EndDialog(hDlg, TRUE);
                 return (TRUE);
                 }

              break;
           }

        return (FALSE);
}


void ChangeMenuState (BOOL bLoggedOn)
{
    HMENU hMenu;


    hMenu = GetMenu(hwndMain);

    if (bLoggedOn) {

        EnableMenuItem (hMenu, IDM_LOGON, MF_BYCOMMAND | MF_GRAYED);


        EnableMenuItem (hMenu, IDM_LOGOFF, MF_BYCOMMAND | MF_ENABLED);
        EnableMenuItem (hMenu, IDM_LUP, MF_BYCOMMAND | MF_ENABLED);
        EnableMenuItem (hMenu, IDM_ULUP, MF_BYCOMMAND | MF_ENABLED);

    } else {

        EnableMenuItem (hMenu, IDM_LOGOFF, MF_BYCOMMAND | MF_GRAYED);
        EnableMenuItem (hMenu, IDM_LUP, MF_BYCOMMAND | MF_GRAYED);
        EnableMenuItem (hMenu, IDM_ULUP, MF_BYCOMMAND | MF_GRAYED);

        EnableMenuItem (hMenu, IDM_LOGON, MF_BYCOMMAND | MF_ENABLED);
    }

}

/****************************************************************************

        FUNCTION: LogonDlgProc(HWND, UINT, WPARAM, LPARAM)

****************************************************************************/

LRESULT CALLBACK LogonDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{

        switch (message)
           {
           case WM_INITDIALOG:
              {
              HBITMAP hBmp;

              hBmp = LoadBitmap (hInst, TEXT("Logo"));

              if (hBmp) {
                 SendDlgItemMessage (hDlg, IDD_ICON, STM_SETIMAGE,
                                     IMAGE_BITMAP, (LPARAM) hBmp);
              }
              SetDlgItemText(hDlg, IDD_USERNAME, szUserName);
              SetDlgItemText(hDlg, IDD_DOMAIN, szDomainName);
              }
              return TRUE;

           case WM_COMMAND:
              if (LOWORD(wParam) == IDOK)
                  {
                  TCHAR szPassword[MAX_PATH];

                  GetDlgItemText(hDlg, IDD_USERNAME, szUserName, MAX_PATH);
                  GetDlgItemText(hDlg, IDD_DOMAIN,   szDomainName, MAX_PATH);
                  GetDlgItemText(hDlg, IDD_PASSWORD, szPassword, MAX_PATH);

                  if (LogonUser(szUserName, szDomainName, szPassword,
                                LOGON32_LOGON_INTERACTIVE,
                                LOGON32_PROVIDER_DEFAULT,
                                &hUserToken)) {
                      EndDialog(hDlg, TRUE);
                      return TRUE;

                  } else {
                      TCHAR szBuffer[200];

                      wsprintf(szBuffer, TEXT("Logon Failed with Error %d"),
                               GetLastError());
                      MessageBox (hDlg, szBuffer, TEXT("Logon"), MB_OK | MB_ICONEXCLAMATION);
                      return TRUE;
                  }

                  }

              if (LOWORD(wParam) == IDCANCEL)
                 {
                 EndDialog(hDlg, FALSE);
                 return TRUE;
                 }

              break;
           }

        return (FALSE);
}
