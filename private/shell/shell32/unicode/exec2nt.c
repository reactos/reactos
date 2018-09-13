#define UNICODE 1

#include "precomp.h"
#pragma  hdrstop

extern WCHAR szPrograms[];

WCHAR szProgmanHelp[] = L"progman.hlp";
WCHAR szCommdlgHelp[] = L"commdlg_help";
WCHAR szNull[] = L"";

UINT wBrowseHelp = WM_USER; /* Set to an unused value */

CHAR szGetOpenFileName[] = "GetOpenFileNameW";

/* the defines below should be in windows.h */

/* Dialog window class */
#define WC_DIALOG       (MAKEINTATOM(0x8002))

/* cbWndExtra bytes needed by dialog manager for dialog classes */
#define DLGWINDOWEXTRA  30


/* Get/SetWindowWord/Long offsets for use with WC_DIALOG windows */
#define DWL_MSGRESULT   0
#define DWL_DLGPROC     4
#define DWL_USER        8

/* For Long File Name support */
#define MAX_EXTENSION 64

typedef struct {
   LPWSTR lpszExe;
   LPWSTR lpszPath;
   LPWSTR lpszName;
} FINDEXE_PARAMS, FAR *LPFINDEXE_PARAMS;

typedef INT (APIENTRY *LPFNGETOPENFILENAME)(LPOPENFILENAME);

VOID APIENTRY
CheckEscapesW(LPWSTR szFile, DWORD cch)
{
   LPWSTR szT;
   WCHAR *p, *pT;

   for (p = szFile; *p; p++) {

       switch (*p) {
           case WCHAR_SPACE:
           case WCHAR_COMMA:
           case WCHAR_SEMICOLON:
           case WCHAR_HAT:
           case WCHAR_QUOTE:
           {
               // this path contains an annoying character
               if (cch < (wcslen(szFile) + 2)) {
                   return;
               }
               szT = (LPWSTR)LocalAlloc(LPTR, cch * sizeof(WCHAR));
               if (!szT) {
                   return;
               }
               wcscpy(szT,szFile);
               p = szFile;
               *p++ = WCHAR_QUOTE;
               for (pT = szT; *pT; ) {
                    *p++ = *pT++;
                }
                *p++ = WCHAR_QUOTE;
                *p = WCHAR_NULL;
                LocalFree(szT);
                return;
            }
        }
    }
}

VOID APIENTRY
CheckEscapesA(LPSTR lpFileA, DWORD cch)
{
   if (lpFileA && *lpFileA) {
      LPWSTR lpFileW;

      lpFileW = (LPWSTR)LocalAlloc(LPTR, (cch * sizeof(WCHAR)));
      if (!lpFileW) {
         return;
      }

      SHAnsiToUnicode(lpFileA, lpFileW, cch);

      CheckEscapesW(lpFileW, cch);

      try {
         SHUnicodeToAnsi(lpFileW, lpFileA, cch);
      } except(EXCEPTION_EXECUTE_HANDLER) {
         LocalFree(lpFileW);
         return;
      }

      LocalFree(lpFileW);
   }

   return;
}

// Hooks into common dialog to allow size of selected files to be displayed.
BOOL APIENTRY
LocateHookProc(
   HWND hDlg,
   UINT uiMessage,
   WPARAM wParam,
   LONG lParam)
{
   WCHAR szTemp[40];
   WORD wID;

   switch (uiMessage) {
   case WM_INITDIALOG:
           PostMessage(hDlg, WM_COMMAND, ctlLast+1, 0L);
           break;

   case WM_COMMAND:
      switch (GET_WM_COMMAND_ID(wParam, lParam)) {
         case ctlLast+1:
            GetDlgItemText(hDlg, edt1, szTemp, ARRAYSIZE(szTemp));
            if (SendDlgItemMessage(hDlg, lst1, LB_FINDSTRING, (WPARAM)-1,
                  (LONG_PTR)(LPSTR)szTemp) >= 0) {
               wID = IDS_PROGFOUND;
            } else {
               wID = IDS_PROGNOTFOUND;
            }
            LoadString(HINST_THISDLL, wID, szTemp, ARRAYSIZE(szTemp));
            SetDlgItemText(hDlg, ctlLast+2, szTemp);
            break;

         case lst2:
            if (GET_WM_COMMAND_CMD(wParam, lParam) == LBN_DBLCLK)
               PostMessage(hDlg, WM_COMMAND, ctlLast+1, 0L);
               break;

         case cmb2:
            switch (GET_WM_COMMAND_CMD(wParam, lParam)) {
               case CBN_SELCHANGE:
                   PostMessage(hDlg, WM_COMMAND, ctlLast+1, 1L);
                   break;

               case CBN_CLOSEUP:
                   PostMessage(hDlg, WM_COMMAND, GET_WM_COMMAND_MPS(cmb2,
                   GetDlgItem(hDlg, cmb2), CBN_SELCHANGE));
                   break;
                }
                break;

               case IDOK:
               case IDCANCEL:
               case IDABORT:
                  PostMessage(hDlg, WM_COMMAND, ctlLast+1, 0L);
                  break;
            }
            break;
   }
   UNREFERENCED_PARAMETER(lParam);
   return(FALSE);  // commdlg, do your thing
}

BOOL APIENTRY
FindExeDlgProcW(
   HWND hDlg,
   register UINT wMsg,
   WPARAM wParam,
   LPARAM lParam)
{
  /* Notice this is OK as a global, because it will be reloaded
   * when needed
   */
   static HANDLE hCommDlg = NULL;

   WCHAR szPath[MAX_PATH]; /* This must be the same size as lpfind->lpszPath */
   WCHAR szBuffer[MAX_PATH + 100];
   LPFINDEXE_PARAMS lpfind;
   int temp;
   LPWSTR lpTemp;

   switch (wMsg) {
      case WM_INITDIALOG:
         wBrowseHelp = RegisterWindowMessage(szCommdlgHelp);

         lpfind = (LPFINDEXE_PARAMS)lParam;

         SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR)lpfind);

         GetDlgItemText(hDlg, IDD_TEXT1, szPath, ARRAYSIZE(szPath));
         wsprintf(szBuffer, szPath, lpfind->lpszExe, lpfind->lpszName);
         SetDlgItemText(hDlg, IDD_TEXT1, szBuffer);
         GetDlgItemText(hDlg, IDD_TEXT2, szPath, ARRAYSIZE(szPath));
         wsprintf(szBuffer, szPath, lpfind->lpszExe);
         SetDlgItemText(hDlg, IDD_TEXT2, szBuffer);

         SetDlgItemText(hDlg, IDD_PATH, lpfind->lpszPath);

         break;

      case WM_DESTROY:
         if (hCommDlg >= (HANDLE)32) {
            FreeLibrary(hCommDlg);
            hCommDlg = NULL;
         }
         break;

      case WM_COMMAND:
         switch (GET_WM_COMMAND_ID(wParam, lParam)) {
            case IDD_BROWSE:
               {
                  LPFNGETOPENFILENAME lpfnGetOpenFileName;
                  WCHAR szExts[MAX_EXTENSION];
                  OPENFILENAME ofn;

                  GetDlgItemText(hDlg, IDD_PATH, szBuffer, ARRAYSIZE(szBuffer));

                  lpfind = (LPFINDEXE_PARAMS)GetWindowLongPtr(hDlg, DWLP_USER);
                  wcscpy(szPath, lpfind->lpszExe);
                  SheRemoveQuotesW(szPath);

                  /* Make up a file types string
                  */
                  // BUG BUG this assumes extensions are of length 3.
                  szExts[0] = WCHAR_CAP_A;
                  szExts[1] = WCHAR_NULL;
                  szExts[2] = WCHAR_STAR;
                  szExts[3] = WCHAR_DOT;
                  szExts[4] = WCHAR_NULL;
                  if (NULL != (lpTemp=StrRChrW(szPath, NULL, WCHAR_DOT)))
                     StrCpyN(szExts+3, lpTemp, ((wcslen(lpTemp) < 60) ? wcslen(lpTemp) : 60));
                  szExts[3+wcslen(szExts+3)+1] = WCHAR_NULL;

                  ofn.lStructSize = sizeof(OPENFILENAME);
                  ofn.hwndOwner = hDlg;
                  ofn.hInstance = HINST_THISDLL;
                  ofn.lpstrFilter = L"A\0\?.?\0";   // a dummy filter
                  ofn.lpstrCustomFilter = NULL;
                  ofn.nMaxCustFilter = 0;
                  ofn.nFilterIndex = 1;
                  ofn.lpstrFile = szPath;
                  ofn.nMaxFile = sizeof(szPath);
                  ofn.lpstrInitialDir = NULL;
                  ofn.lpstrTitle = NULL;
                  ofn.Flags = OFN_HIDEREADONLY | OFN_FILEMUSTEXIST |
                  OFN_ENABLETEMPLATE | OFN_SHOWHELP;
                  ofn.lCustData = (LONG_PTR) hDlg;
                  ofn.lpfnHook = NULL;    // AddFileHookProc;
                  ofn.lpTemplateName = MAKEINTRESOURCE(DLG_BROWSE);
                  ofn.nFileOffset = 0;
                  ofn.nFileExtension = 0;
                  ofn.lpstrDefExt = NULL;
                  ofn.lpstrFileTitle = NULL;

                  if (hCommDlg < (HANDLE)32 &&
                     (hCommDlg = LoadLibrary(TEXT("comdlg32.dll"))) < (HANDLE)32) {
                        CommDlgError:
                        LoadString(HINST_THISDLL, IDS_NOCOMMDLG, szBuffer, ARRAYSIZE(szBuffer));
                        GetWindowText(hDlg, szPath, ARRAYSIZE(szPath));
                        MessageBox(hDlg, szBuffer, szPath, MB_ICONHAND|MB_OK);
                        break;
                     }
                  if (!(lpfnGetOpenFileName =
                     (LPFNGETOPENFILENAME)GetProcAddress((HINSTANCE)hCommDlg,
                     (LPSTR)szGetOpenFileName)))
                     goto CommDlgError;

                  temp = (*lpfnGetOpenFileName)(&ofn);

                  if (temp) {
                     LPWSTR lpTemp;

                     lpTemp = StrRChrW(szPath, NULL, WCHAR_BSLASH);
                     *lpTemp = WCHAR_NULL;
                     SetDlgItemText(hDlg, IDD_PATH, szPath);
                  }

                  break;
               }

            case IDOK:
               {
                  HANDLE hFile;

                  lpfind = (LPFINDEXE_PARAMS)GetWindowLongPtr(hDlg, DWLP_USER);
                  if (lpfind) {
                     GetDlgItemText(hDlg, IDD_PATH, lpfind->lpszPath, MAX_PATH);

                     switch (*CharPrev(lpfind->lpszPath,
                           lpTemp=lpfind->lpszPath+lstrlen(lpfind->lpszPath))) {
                        case WCHAR_BSLASH:
                        case WCHAR_COLON:
                           break;

                        default:
                           *lpTemp++ = WCHAR_BSLASH;
                           break;
                     }

                     wcscpy(lpTemp, lpfind->lpszExe);

                     hFile = CreateFile(lpfind->lpszPath, GENERIC_EXECUTE, (FILE_SHARE_READ | FILE_SHARE_WRITE),
                         NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

                     if (hFile == INVALID_HANDLE_VALUE) {
                        LoadString(HINST_THISDLL, IDS_STILLNOTFOUND, szPath, ARRAYSIZE(szPath));
                        wsprintf(szBuffer, szPath, lpfind->lpszPath);
                        GetWindowText(hDlg, szPath, ARRAYSIZE(szPath));
                        MessageBox(hDlg, szBuffer, szPath, MB_ICONHAND|MB_OK);
                        break;
                     }

                     WriteProfileString(szPrograms, lpfind->lpszExe,
                        lpfind->lpszPath);
                  }
               }

            // fall through
            case IDCANCEL:
               EndDialog(hDlg, GET_WM_COMMAND_ID(wParam, lParam) == IDOK);
               break;

            case IDHELP:
               // BUGBUG
               break;
         }

         break;

      default:
         return FALSE;
   }

   return TRUE;
}


// Returns -1 if we found the file (and it was not in an obvious place)
// or an error code which will be returned to the app (see the error
// returns for ShellExecute)

HANDLE APIENTRY
FindAssociatedExeW(
   HWND hwnd,
   LPWSTR lpCommand,
   LPWSTR lpName)
{
   FINDEXE_PARAMS find;
   WCHAR szPath[MAX_PATH];
   WCHAR szExe[MAX_PATH];
   LPWSTR lpSpace, lpTemp;
   HANDLE hFile = NULL;
   BOOL fQuote = FALSE;


   // find the param list
   lpSpace = lpCommand;
   while (*lpSpace)
   {
       if ((*lpSpace == WCHAR_SPACE) && (!fQuote))
       {
           break;
       }
       else if (*lpSpace == WCHAR_QUOTE)
       {
           fQuote = !fQuote;
       }
       lpSpace++;
   }

   if (*lpSpace == WCHAR_SPACE) {
      *lpSpace = 0;
      wcscpy(szPath, lpCommand);
      *lpSpace = WCHAR_SPACE;
   } else {
      lpSpace = szNull;
      wcscpy(szPath, lpCommand);
   }
   SheRemoveQuotesW(szPath);

   /* Add ".exe" if there is no extension
    * Check if the file can be opened; if it can, then some DLL could not
    * be found during the WinExec, so return file not found
    */
   if (NULL != (lpTemp=StrRChrW(szPath, NULL, WCHAR_BSLASH))
       || NULL != (lpTemp=StrRChrW(szPath, NULL, WCHAR_COLON))) {
      ++lpTemp;
   } else {
      lpTemp = szPath;
   }

   hFile = CreateFile(szPath, GENERIC_EXECUTE, (FILE_SHARE_READ | FILE_SHARE_WRITE),
       NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

   if (hFile != INVALID_HANDLE_VALUE) {
      return((HANDLE)2);
   }

   // store the file name component
   wcscpy(szExe, lpTemp);

   // make sure there is an extension
   if (!StrChrW(szExe, WCHAR_DOT)) {
      wcscat(szExe, TEXT(".exe"));
   }

   // add back the quotes, if necessary
   CheckEscapesW(szExe, MAX_PATH);

   // look in win.ini
   GetProfileString(szPrograms, szExe, szNull, szPath, ARRAYSIZE(szPath));

   if (szPath[0]) {
      hFile = CreateFile(szPath, GENERIC_EXECUTE, (FILE_SHARE_READ | FILE_SHARE_WRITE),
          NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

      if (hFile != INVALID_HANDLE_VALUE) {

         wcscat(szPath, lpSpace);       // add the parameters
         wcscpy(lpCommand, szPath);     // return the new path
         return((HANDLE)-1);
      }

      /* Strip off the file part */
      if (NULL != (lpTemp=StrRChrW(szPath, NULL, WCHAR_BSLASH))) {
         if (*CharPrev(szPath, lpTemp) == WCHAR_COLON) {
            ++lpTemp;
         }
         *lpTemp = WCHAR_NULL;
      } else if (NULL != (lpTemp=StrRChrW(szPath, NULL, WCHAR_COLON))) {
         *(lpTemp+1) = WCHAR_NULL;
      }
   } else {
      /* Prompt with the disk that Windows is on */
      GetWindowsDirectory(szPath, ARRAYSIZE(szPath)-1);
      szPath[3] = WCHAR_NULL;
   }

   find.lpszExe = szExe;
   find.lpszPath = szPath;
   find.lpszName = lpName;

   switch(DialogBoxParam(HINST_THISDLL, MAKEINTRESOURCE(DLG_FINDEXE), hwnd,
         (DLGPROC)FindExeDlgProcW, (LONG_PTR)(LPFINDEXE_PARAMS)&find)) {
      case IDOK:
          wcscat(szPath, lpSpace);      // add the parameters
          wcscpy(lpCommand, szPath);    // return the new path
          return ((HANDLE)-1);

      case IDCANCEL:
          return((HANDLE)15);                   // This is the user cancel return

      default:
          return((HANDLE)2);                    // stick with the file not found
   }
}
