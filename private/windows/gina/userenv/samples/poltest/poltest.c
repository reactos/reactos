#include <windows.h>
#include <ole2.h>
#include <userenv.h>
#define SECURITY_WIN32
#include <security.h>
#include <dsgetdc.h>
#include <lm.h>
#include <shellapi.h>
#include <commdlg.h>
#include "poltest.h"


HINSTANCE hInst;
HWND      hwndMain;
HWND      hwndListBox;
HMENU     hMenu;
HANDLE    hMachineSection;
HANDLE    hUserSection;
HANDLE    hMachineEvent;
HANDLE    hUserEvent;
HANDLE    hExit;
HANDLE    hThread;
BOOL      bVerbose = FALSE;
BOOL      bSync = FALSE;


TCHAR szAppName[] = TEXT("PolTest");
TCHAR szTitle[]   = TEXT("Group Policy Tester");

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


        SetEvent (hExit);
        WaitForSingleObject (hThread, INFINITE);
        CloseHandle (hExit);
        CloseHandle (hMachineEvent);
        CloseHandle (hUserEvent);
        CloseHandle (hThread);

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


        hExit = CreateEvent (NULL, FALSE, FALSE, TEXT("poltest:  exit event"));

        if (!hExit) {
            AddString (TEXT("failed to create exit event"));
        }


        hMachineEvent = CreateEvent (NULL, FALSE, FALSE, TEXT("poltest:  machine policy event"));

        if (!hMachineEvent) {
            AddString (TEXT("failed to create machine notify event"));
        }


        hUserEvent = CreateEvent (NULL, FALSE, FALSE, TEXT("poltest:  user policy event"));

        if (!hUserEvent) {
            AddString (TEXT("failed to create user notify event"));
        }


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
        HKEY hKey;
        DWORD dwSize,dwType;

        hInst = hInstance;

        hWnd = CreateWindow(szAppName,
                            szTitle,
                            WS_OVERLAPPEDWINDOW,
                            CW_USEDEFAULT, 0, 500, 500,
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


        ShowWindow(hWnd, SW_SHOWDEFAULT);
        UpdateWindow(hWnd);

        if (RegOpenKeyEx (HKEY_LOCAL_MACHINE, TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\Diagnostics"),
                          0, KEY_READ, &hKey) == ERROR_SUCCESS) {

            dwSize = sizeof(bVerbose);
            RegQueryValueEx (hKey, TEXT("RunDiagnosticLoggingGroupPolicy"),
                             NULL, &dwType, (LPBYTE) &bVerbose, &dwSize);

            RegCloseKey (hKey);
        }

        CheckMenuItem (GetMenu(hWnd), IDM_VERBOSE, MF_BYCOMMAND |
                       (bVerbose ? MF_CHECKED : MF_UNCHECKED));

        if (RegOpenKeyEx (HKEY_LOCAL_MACHINE, TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon"),
                          0, KEY_READ, &hKey) == ERROR_SUCCESS) {

            dwSize = sizeof(bSync);
            RegQueryValueEx (hKey, TEXT("SynchronousGroupPolicy"),
                             NULL, &dwType, (LPBYTE) &bSync, &dwSize);

            RegCloseKey (hKey);
        }

        CheckMenuItem (GetMenu(hWnd), IDM_SYNCPOLICY, MF_BYCOMMAND |
                       (bSync ? MF_CHECKED : MF_UNCHECKED));

        return (TRUE);

}

/****************************************************************************

        FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)

        PURPOSE:  Processes messages

****************************************************************************/

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    DWORD dwID;

        switch (message)
           {
           case WM_CREATE:
                {
                RECT rc;

                GetClientRect (hWnd, &rc);

                hMenu = GetMenu(hWnd);

                EnableMenuItem (hMenu, IDM_RESUME_MACHINE, MF_BYCOMMAND | MF_GRAYED);
                EnableMenuItem (hMenu, IDM_RESUME_USER, MF_BYCOMMAND | MF_GRAYED);

                hwndListBox = CreateWindow(TEXT("LISTBOX"),
                                    NULL,
                                    WS_CHILDWINDOW | WS_VSCROLL | WS_VISIBLE | LBS_NOINTEGRALHEIGHT,
                                    0, 0, rc.right, rc.bottom,
                                    hWnd,
                                    NULL,
                                    hInst,
                                    NULL);

                SendMessage (hwndListBox, WM_SETFONT, (WPARAM) GetStockObject(ANSI_VAR_FONT), 0);


                hThread = CreateThread (NULL, 0, (LPTHREAD_START_ROUTINE) NotifyThread,
                                        0, 0, &dwID);


                }
                break;

           case WM_COMMAND:
              {
              switch (LOWORD(wParam))
                 {
                 case IDM_UPDATE_MACHINE:
                    RefreshPolicy (TRUE);
                    break;

                 case IDM_UPDATE_USER:
                    RefreshPolicy (FALSE);
                    break;

                 case IDM_PAUSE_MACHINE:
                    hMachineSection = EnterCriticalPolicySection (TRUE);

                    if (hMachineSection) {
                        EnableMenuItem (hMenu, IDM_RESUME_MACHINE, MF_BYCOMMAND | MF_ENABLED);
                        EnableMenuItem (hMenu, IDM_PAUSE_MACHINE, MF_BYCOMMAND | MF_GRAYED);
                    } else {
                        AddString (TEXT("Failed to claim critical section."));
                    }
                    break;

                 case IDM_RESUME_MACHINE:
                    LeaveCriticalPolicySection (hMachineSection);
                    hMachineSection = NULL;
                    EnableMenuItem (hMenu, IDM_RESUME_MACHINE, MF_BYCOMMAND | MF_GRAYED);
                    EnableMenuItem (hMenu, IDM_PAUSE_MACHINE, MF_BYCOMMAND | MF_ENABLED);
                    break;

                 case IDM_PAUSE_USER:
                    hUserSection = EnterCriticalPolicySection (FALSE);

                    if (hUserSection) {
                        EnableMenuItem (hMenu, IDM_RESUME_USER, MF_BYCOMMAND | MF_ENABLED);
                        EnableMenuItem (hMenu, IDM_PAUSE_USER, MF_BYCOMMAND | MF_GRAYED);
                    } else {
                        AddString (TEXT("Failed to claim critical section."));
                    }
                    break;

                 case IDM_RESUME_USER:
                    LeaveCriticalPolicySection (hUserSection);
                    hUserSection = NULL;
                    EnableMenuItem (hMenu, IDM_RESUME_USER, MF_BYCOMMAND | MF_GRAYED);
                    EnableMenuItem (hMenu, IDM_PAUSE_USER, MF_BYCOMMAND | MF_ENABLED);
                    break;

                 case IDM_EXIT:
                    DestroyWindow (hwndMain);
                    break;


                 case IDM_VERBOSE:
                    {
                    HKEY hKey;
                    DWORD dwDisp, dwTemp;
                    TCHAR szMsg[MAX_PATH];
                    LONG lResult;

                    lResult = RegCreateKeyEx (HKEY_LOCAL_MACHINE, TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\Diagnostics"),
                                              0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE,
                                              NULL, &hKey, &dwDisp);

                    if (lResult == ERROR_SUCCESS) {

                        bVerbose = !bVerbose;

                        RegSetValueEx (hKey, TEXT("RunDiagnosticLoggingGroupPolicy"),
                                       0, REG_DWORD, (LPBYTE) &bVerbose, sizeof(bVerbose));

                        RegCloseKey (hKey);

                        CheckMenuItem (GetMenu(hWnd), IDM_VERBOSE, MF_BYCOMMAND |
                                       (bVerbose ? MF_CHECKED : MF_UNCHECKED));
                    } else {

                        szMsg[0] = TEXT('\0');

                        FormatMessage(FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_FROM_SYSTEM,
                                      NULL, lResult,
                                      MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL),
                                      szMsg, MAX_PATH, NULL);

                        MessageBox (hWnd, szMsg, TEXT("Error setting registry value"),
                                    MB_OK | MB_ICONERROR);
                    }

                    }
                    break;

                 case IDM_EVENTVWR:
                    ShellExecute (NULL, TEXT("open"), TEXT("eventvwr.msc"), NULL,
                                  NULL, SW_SHOWNORMAL);
                    break;

                 case IDM_USERNAME:
                    {
                    TCHAR szName[512];
                    DWORD dwSize = 512;

                    if (GetUserNameEx(NameFullyQualifiedDN, szName, &dwSize)) {
                        AddString (szName);
                    } else {
                        TCHAR szError[100];

                        wsprintf (szError, TEXT("Failed to get user name with %d"), GetLastError());
                        AddString (szError);
                    }
                    }
                    break;

                 case IDM_COMPUTERNAME:
                    {
                    TCHAR szName[512];
                    DWORD dwSize = 512;

                    if (GetComputerObjectName(NameFullyQualifiedDN, szName, &dwSize)) {
                        AddString (szName);
                    } else {
                        TCHAR szError[100];

                        wsprintf (szError, TEXT("Failed to get computer name with %d"), GetLastError());
                        AddString (szError);
                    }
                    }
                    break;

                 case IDM_SITENAME:
                    {
                    LPTSTR lpName;
                    ULONG ulResult;

                    ulResult = DsGetSiteName (NULL, &lpName);

                    if (ulResult == ERROR_SUCCESS) {
                        AddString (lpName);
                        NetApiBufferFree (lpName);
                    } else {
                        TCHAR szError[100];

                        wsprintf (szError, TEXT("Failed to get site name with %d"), ulResult);
                        AddString (szError);
                    }
                    }
                    break;

                 case IDM_PDCNAME:
                    {
                    DWORD dwResult;
                    PDOMAIN_CONTROLLER_INFO pDCI;


                    dwResult = DsGetDcName (NULL, NULL, NULL, NULL,
                                            DS_DIRECTORY_SERVICE_REQUIRED | DS_PDC_REQUIRED,
                                            &pDCI);

                    if (dwResult == ERROR_SUCCESS) {
                        AddString (pDCI->DomainControllerName);
                        NetApiBufferFree(pDCI);

                    } else {
                        TCHAR szBuffer[100];

                        wsprintf (szBuffer, TEXT("Failed to get the PDC name with %d"), dwResult);
                        AddString (szBuffer);
                    }
                    }
                    break;

                 case IDM_SYNCPOLICY:
                    {
                    HKEY hKey;
                    DWORD dwDisp, dwTemp;
                    TCHAR szMsg[MAX_PATH];
                    LONG lResult;

                    lResult = RegCreateKeyEx (HKEY_LOCAL_MACHINE, TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon"),
                                              0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE,
                                              NULL, &hKey, &dwDisp);

                    if (lResult == ERROR_SUCCESS) {

                        bSync = !bSync;

                        RegSetValueEx (hKey, TEXT("SynchronousGroupPolicy"),
                                       0, REG_DWORD, (LPBYTE) &bSync, sizeof(bSync));

                        RegCloseKey (hKey);

                        CheckMenuItem (GetMenu(hWnd), IDM_SYNCPOLICY, MF_BYCOMMAND |
                                       (bSync ? MF_CHECKED : MF_UNCHECKED));

                        AddString (TEXT("You need to reboot for this change to take effect."));

                    } else {

                        szMsg[0] = TEXT('\0');

                        FormatMessage(FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_FROM_SYSTEM,
                                      NULL, lResult,
                                      MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL),
                                      szMsg, MAX_PATH, NULL);

                        MessageBox (hWnd, szMsg, TEXT("Error setting registry value"),
                                    MB_OK | MB_ICONERROR);
                    }

                    }
                    break;

                 case IDM_DCLIST:
                    ManageDomainInfo(hWnd);
                    break;

                 case IDM_CHECKGPO:
                    CheckGPO(hWnd);
                    break;

                 case IDM_CLEARWINDOW:
                    SendMessage (hwndListBox, LB_RESETCONTENT, 0, 0);
                    break;

                 case IDM_SAVEAS:
                    {
                    HANDLE hFile;
                    INT iCount, i;
                    DWORD dwWrite;
                    TCHAR szFilter[100];
                    TCHAR szFileName[MAX_PATH];
                    CHAR szString[MAX_PATH];
                    TCHAR szError[100];
                    CHAR szLF[] = "\r\n";
                    OPENFILENAME ofn;

                    iCount = SendMessage (hwndListBox, LB_GETCOUNT, 0, 0);

                    if (iCount == LB_ERR) {
                        break;
                    }

                    lstrcpy (szFileName, TEXT("Poltest Results.txt"));

                    ZeroMemory (szFilter, sizeof(szFilter));

                    lstrcpy (szFilter, TEXT("Text Files#*.txt"));
                    szFilter[10] = TEXT('\0');

                    ZeroMemory (&ofn, sizeof(ofn));
                    ofn.lStructSize = sizeof(ofn);
                    ofn.hwndOwner = hWnd;
                    ofn.hInstance = hInst;
                    ofn.lpstrFile = szFileName;
                    ofn.nMaxFile =  MAX_PATH;
                    ofn.lpstrFilter = szFilter;
                    ofn.lpstrDefExt = TEXT("txt");
                    ofn.Flags = OFN_CREATEPROMPT | OFN_ENABLESIZING | OFN_HIDEREADONLY |
                                OFN_OVERWRITEPROMPT;

                    if (!GetSaveFileName (&ofn)) {
                        break;
                    }

                    hFile = CreateFile (szFileName, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
                                        FILE_ATTRIBUTE_NORMAL, NULL);

                    if (hFile == INVALID_HANDLE_VALUE) {
                        wsprintf (szError, TEXT("Failed to open file with %d"), GetLastError());
                        AddString (szError);
                        break;
                    }

                    for (i = 0; i < iCount; i++) {
                        szString[0] = '\0';
                        SendMessageA (hwndListBox, LB_GETTEXT, i, (LPARAM)szString);
                        WriteFile (hFile, szString, lstrlenA(szString),
                                   &dwWrite, NULL);
                        WriteFile (hFile, szLF, lstrlenA(szLF),
                                   &dwWrite, NULL);
                    }

                    CloseHandle (hFile);
                    }
                    break;

                 default:
                    return (DefWindowProc(hWnd, message, wParam, lParam));
                 }
              }
              break;

           case WM_WININICHANGE:

              if (!lstrcmpi ((LPTSTR)lParam, TEXT("Policy"))) {

                   if (wParam) {
                       AddString (TEXT("Received WM_WININICHANGE:  machine policy applied."));
                   } else {
                       AddString (TEXT("Received WM_WININICHANGE:  user policy applied."));
                   }
              }
              break;

           case WM_SIZE:
                SetWindowPos (hwndListBox, HWND_TOP, 0, 0,
                              LOWORD(lParam), HIWORD(lParam),
                              SWP_NOMOVE | SWP_NOZORDER);
                break;

           case WM_DESTROY:
              PostQuitMessage(0);
              break;

           default:
              return (DefWindowProc(hWnd, message, wParam, lParam));
           }

        return FALSE;
}

VOID AddString (LPTSTR lpString)
{
    SYSTEMTIME systime;
    TCHAR  szString[300];

    GetLocalTime (&systime);
    wsprintf (szString, TEXT("%02d:%02d:%02d:%03d   %s"),
              systime.wHour, systime.wMinute, systime.wSecond,
              systime.wMilliseconds, lpString);


    SendMessage (hwndListBox, LB_INSERTSTRING, -1,
                 (LPARAM) szString);

}

DWORD NotifyThread (DWORD dwDummy)
{
    HANDLE hHandles[3];
    DWORD dwResult;


    RegisterGPNotification(hMachineEvent, TRUE);
    RegisterGPNotification(hUserEvent, FALSE);

    hHandles[0] = hExit;
    hHandles[1] = hMachineEvent;
    hHandles[2] = hUserEvent;

    while (TRUE) {

        dwResult = WaitForMultipleObjects (3, hHandles, FALSE, INFINITE);

        if ((dwResult == WAIT_FAILED) || ((dwResult - WAIT_OBJECT_0) == 0)) {
            if (dwResult == WAIT_FAILED) {
                AddString (TEXT("WaitForMultipleObjects failed."));
            }
            break;
        }

        if ((dwResult - WAIT_OBJECT_0) == 1) {
            AddString (TEXT("Machine notify event signaled."));
        } else {
            AddString (TEXT("User notify event signaled."));
        }
    }

    UnregisterGPNotification(hMachineEvent);
    UnregisterGPNotification(hUserEvent);

    return 0;
}
