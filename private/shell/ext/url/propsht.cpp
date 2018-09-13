/*
 * propsht.cpp - IPropSheetExt implementation for URL class.
 */


/* Headers
 **********/

#include "project.hpp"
#pragma hdrstop

#undef NO_HELP              // for help.h

#include <help.h>

#undef IDH_COMM_GROUPBOX    // for addon.h
#include <addon.h>

#include "resource.h"

#include <mluisupp.h>

extern "C"
WINSHELLAPI int   WINAPI PickIconDlg(HWND hwnd, LPSTR pszIconPath, UINT cbIconPath, int *piIconIndex);

/* Types
 ********/

/* Internet Shortcut property sheet data */

typedef struct _isps
{
   PROPSHEETPAGE psp;

   PInternetShortcut pintshcut;

   char rgchIconFile[MAX_PATH_LEN];

   int niIcon;
}
ISPS;
DECLARE_STANDARD_TYPES(ISPS);


/* Module Constants
 *******************/

#pragma data_seg(DATA_SEG_READ_ONLY)

// Tray notification window class

PRIVATE_DATA const char s_cszTrayNotificationClass[]  = WNDCLASS_TRAYNOTIFY;

// HACKHACK: WMTRAY_SCREGISTERHOTKEY and WMTRAY_SCUNREGISTERHOTKEY are stolen
// from shelldll\link.c.

PRIVATE_DATA CUINT WMTRAY_SCREGISTERHOTKEY            = (WM_USER + 233);
PRIVATE_DATA CUINT WMTRAY_SCUNREGISTERHOTKEY             = (WM_USER + 234);

// show commands - N.b., the order of these constants must match the order of
// the corresponding IDS_ string table constants.

PRIVATE_DATA const UINT s_ucMaxShowCmdLen             = MAX_PATH_LEN;

PRIVATE_DATA const int s_rgnShowCmds[] =
{
   SW_SHOWNORMAL,
   SW_SHOWMINNOACTIVE,
   SW_SHOWMAXIMIZED
};

// help files

PRIVATE_DATA const char s_cszPlusHelpFile[]           = "Plus!.hlp";

// help topics

PRIVATE_DATA const DWORD s_rgdwHelpIDs[] =
{
   IDD_LINE_1,          NO_HELP,
   IDD_LINE_2,          NO_HELP,
   IDD_ICON,            IDH_FCAB_LINK_ICON,
   IDD_NAME,            IDH_FCAB_LINK_NAME,
   IDD_URL_TEXT,        IDH_INTERNET_SHORTCUT_TARGET,
   IDD_URL,             IDH_INTERNET_SHORTCUT_TARGET,
   IDD_HOTKEY_TEXT,     IDH_FCAB_LINK_HOTKEY,
   IDD_HOTKEY,          IDH_FCAB_LINK_HOTKEY,
   IDD_START_IN_TEXT,   IDH_FCAB_LINK_WORKING,
   IDD_START_IN,        IDH_FCAB_LINK_WORKING,
   IDD_SHOW_CMD,        IDH_FCAB_LINK_RUN,
   IDD_CHANGE_ICON,     IDH_FCAB_LINK_CHANGEICON,
   0,                   0
};

#pragma data_seg()


/***************************** Private Functions *****************************/


#ifdef DEBUG

PRIVATE_CODE BOOL IsValidPCISPS(PCISPS pcisps)
{
   return(IS_VALID_READ_PTR(pcisps, CISPS) &&
      IS_VALID_STRUCT_PTR(&(pcisps->psp), CPROPSHEETPAGE) &&
      IS_VALID_STRUCT_PTR(pcisps->pintshcut, CInternetShortcut) &&
      EVAL(IsValidIconIndex(*(pcisps->rgchIconFile) ? S_OK : S_FALSE, pcisps->rgchIconFile, sizeof(pcisps->rgchIconFile), pcisps->niIcon)));
}

#endif


#pragma warning(disable:4100) /* "unreferenced formal parameter" warning */

PRIVATE_CODE UINT CALLBACK ISPSCallback(HWND hwnd, UINT uMsg,
                    LPPROPSHEETPAGE ppsp)
{
   UINT uResult = TRUE;
   PISPS pisps = (PISPS)ppsp;

   // uMsg may be any value.

   ASSERT(! hwnd ||
      IS_VALID_HANDLE(hwnd, WND));
   ASSERT(IS_VALID_STRUCT_PTR((PCISPS)ppsp, CISPS));

   switch (uMsg)
   {
      case PSPCB_CREATE:
     TRACE_OUT(("ISPSCallback(): Received PSPCB_CREATE."));
     break;

      case PSPCB_RELEASE:
     TRACE_OUT(("ISPSCallback(): Received PSPCB_RELEASE."));
     pisps->pintshcut->Release();
     break;

      default:
     TRACE_OUT(("ISPSCallback(): Unhandled message %u.",
            uMsg));
     break;
   }

   return(uResult);
}

#pragma warning(default:4100) /* "unreferenced formal parameter" warning */


PRIVATE_CODE void SetISPSIcon(HWND hdlg, HICON hicon)
{
   HICON hiconOld;

   ASSERT(IS_VALID_HANDLE(hdlg, WND));
   ASSERT(IS_VALID_HANDLE(hicon, ICON));

   hiconOld = (HICON)SendDlgItemMessage(hdlg, IDD_ICON, STM_SETICON,
                    (WPARAM)hicon, 0);

   if (hiconOld)
      DestroyIcon(hiconOld);

   TRACE_OUT(("SetISPSIcon(): Set property sheet icon to %#lx.",
          hicon));

   return;
}


PRIVATE_CODE void SetISPSFileNameAndIcon(HWND hdlg)
{
   HRESULT hr;
   PInternetShortcut pintshcut;
   char rgchFile[MAX_PATH_LEN];

   ASSERT(IS_VALID_HANDLE(hdlg, WND));

   pintshcut = ((PISPS)GetWindowLongPtr(hdlg, DWLP_USER))->pintshcut;
   ASSERT(IS_VALID_STRUCT_PTR(pintshcut, CInternetShortcut));

   hr = pintshcut->GetCurFile(rgchFile, sizeof(rgchFile));

   if (hr == S_OK)
   {
      SHFILEINFO shfi;
      DWORD_PTR dwResult;

      dwResult = SHGetFileInfo(rgchFile, 0, &shfi, sizeof(shfi),
                   (SHGFI_DISPLAYNAME | SHGFI_ICON));

      if (dwResult)
      {
     PSTR pszFileName;

     pszFileName = (PSTR)ExtractFileName(shfi.szDisplayName);

     EVAL(SetDlgItemText(hdlg, IDD_NAME, pszFileName));

     TRACE_OUT(("SetISPSFileNameAndIcon(): Set property sheet file name to \"%s\".",
            pszFileName));

     SetISPSIcon(hdlg, shfi.hIcon);
      }
      else
      {
     hr = E_FAIL;

     TRACE_OUT(("SetISPSFileNameAndIcon(): SHGetFileInfo() failed, returning %lu.",
            dwResult));
      }
   }
   else
      TRACE_OUT(("SetISPSFileNameAndIcon(): GetCurFile() failed, returning %s.",
         GetHRESULTString(hr)));

   if (hr != S_OK)
      EVAL(SetDlgItemText(hdlg, IDD_NAME, EMPTY_STRING));

   return;
}


PRIVATE_CODE void SetISPSURL(HWND hdlg)
{
   PInternetShortcut pintshcut;
   HRESULT hr;
   PSTR pszURL;

   ASSERT(IS_VALID_HANDLE(hdlg, WND));

   pintshcut = ((PISPS)GetWindowLongPtr(hdlg, DWLP_USER))->pintshcut;
   ASSERT(IS_VALID_STRUCT_PTR(pintshcut, CInternetShortcut));

   hr = pintshcut->GetURL(&pszURL);

   if (hr == S_OK)
   {
      EVAL(SetDlgItemText(hdlg, IDD_URL, pszURL));

      TRACE_OUT(("SetISPSURL(): Set property sheet URL to \"%s\".",
         pszURL));

      SHFree(pszURL);
      pszURL = NULL;
   }
   else
      EVAL(SetDlgItemText(hdlg, IDD_URL, EMPTY_STRING));

   return;
}


PRIVATE_CODE void SetISPSWorkingDirectory(HWND hdlg)
{
   PInternetShortcut pintshcut;
   HRESULT hr;
   char rgchWorkingDirectory[MAX_PATH_LEN];

   ASSERT(IS_VALID_HANDLE(hdlg, WND));

   pintshcut = ((PISPS)GetWindowLongPtr(hdlg, DWLP_USER))->pintshcut;
   ASSERT(IS_VALID_STRUCT_PTR(pintshcut, CInternetShortcut));

   hr = pintshcut->GetWorkingDirectory(rgchWorkingDirectory,
                       sizeof(rgchWorkingDirectory));

   if (hr == S_OK)
   {
      EVAL(SetDlgItemText(hdlg, IDD_START_IN, rgchWorkingDirectory));

      TRACE_OUT(("SetISPSWorkingDirectory(): Set property sheet working directory to \"%s\".",
         rgchWorkingDirectory));
   }
   else
   {
      TRACE_OUT(("SetISPSWorkingDirectory(): GetWorkingDirectory() failed, returning %s.",
         GetHRESULTString(hr)));

      EVAL(SetDlgItemText(hdlg, IDD_START_IN, EMPTY_STRING));
   }

   return;
}


PRIVATE_CODE void InitISPSHotkey(HWND hdlg)
{
   PInternetShortcut pintshcut;
   WORD wHotkey;
   HRESULT hr;

   ASSERT(IS_VALID_HANDLE(hdlg, WND));

   // Set hotkey combinations.

   SendDlgItemMessage(hdlg, IDD_HOTKEY, HKM_SETRULES,
              (HKCOMB_NONE | HKCOMB_A | HKCOMB_C | HKCOMB_S),
              (HOTKEYF_CONTROL | HOTKEYF_ALT));

   // Set current hotkey.

   pintshcut = ((PISPS)GetWindowLongPtr(hdlg, DWLP_USER))->pintshcut;
   ASSERT(IS_VALID_STRUCT_PTR(pintshcut, CInternetShortcut));

   hr = pintshcut->GetHotkey(&wHotkey);
   SendDlgItemMessage(hdlg, IDD_HOTKEY, HKM_SETHOTKEY, wHotkey, 0);

   return;
}


PRIVATE_CODE void InitISPSShowCmds(HWND hdlg)
{
   int niShowCmd;

   ASSERT(IS_VALID_HANDLE(hdlg, WND));

   for (niShowCmd = IDS_SHOW_NORMAL;
    niShowCmd <= IDS_SHOW_MAXIMIZED;
    niShowCmd++)
   {
      char rgchShowCmd[s_ucMaxShowCmdLen];

      if (MLLoadStringA(niShowCmd, rgchShowCmd,
             sizeof(rgchShowCmd)))
      {
     SendDlgItemMessage(hdlg, IDD_SHOW_CMD, CB_ADDSTRING, 0,
                (LPARAM)rgchShowCmd);

     TRACE_OUT(("InitISPSShowCmds(): Added show command \"%s\".",
            rgchShowCmd));
      }
      else
     ERROR_OUT(("InitISPSShowCmds(): Unable to load string %d.",
            niShowCmd));
   }

   return;
}


PRIVATE_CODE void SetISPSShowCmd(HWND hdlg)
{
   PInternetShortcut pintshcut;
   int nShowCmd;
   int i;

   ASSERT(IS_VALID_HANDLE(hdlg, WND));

   pintshcut = ((PISPS)GetWindowLongPtr(hdlg, DWLP_USER))->pintshcut;
   ASSERT(IS_VALID_STRUCT_PTR(pintshcut, CInternetShortcut));

   pintshcut->GetShowCmd(&nShowCmd);

   for (i = 0; i < ARRAY_ELEMENTS(s_rgnShowCmds); i++)
   {
      if (s_rgnShowCmds[i] == nShowCmd)
     break;
   }

   if (i >= ARRAY_ELEMENTS(s_rgnShowCmds))
   {
      ASSERT(i == ARRAY_ELEMENTS(s_rgnShowCmds));

      WARNING_OUT(("SetISPSShowCmd(): Unrecognized show command %d.  Defaulting to normal.",
           nShowCmd));

      i = 0;
   }

   SendDlgItemMessage(hdlg, IDD_SHOW_CMD, CB_SETCURSEL, i, 0);

   TRACE_OUT(("SetISPSShowCmd(): Set property sheet show command to index %d.",
          i));

   return;
}


#pragma warning(disable:4100) /* "unreferenced formal parameter" warning */

PRIVATE_CODE BOOL ISPS_InitDialog(HWND hdlg, WPARAM wparam, LPARAM lparam)
{
   PInternetShortcut pintshcut;

   // wparam may be any value.

   ASSERT(IS_VALID_HANDLE(hdlg, WND));
   ASSERT(IS_VALID_STRUCT_PTR((PCISPS)lparam, CISPS));

   SetWindowLongPtr(hdlg, DWLP_USER, lparam);

   pintshcut = ((PISPS)lparam)->pintshcut;
   ASSERT(IS_VALID_STRUCT_PTR(pintshcut, CInternetShortcut));

   // Initialize control contents.

   SetISPSFileNameAndIcon(hdlg);

   SendDlgItemMessage(hdlg, IDD_URL, EM_LIMITTEXT, g_ucMaxURLLen - 1, 0);
   SetISPSURL(hdlg);

   SendDlgItemMessage(hdlg, IDD_START_IN, EM_LIMITTEXT, MAX_PATH_LEN - 1, 0);
   SetISPSWorkingDirectory(hdlg);

   InitISPSHotkey(hdlg);

   InitISPSShowCmds(hdlg);
   SetISPSShowCmd(hdlg);

   return(TRUE);
}


PRIVATE_CODE BOOL ISPS_Destroy(HWND hdlg, WPARAM wparam, LPARAM lparam)
{
   PInternetShortcut pintshcut;

   // wparam may be any value.
   // lparam may be any value.

   ASSERT(IS_VALID_HANDLE(hdlg, WND));

   pintshcut = ((PISPS)GetWindowLongPtr(hdlg, DWLP_USER))->pintshcut;
   ASSERT(IS_VALID_STRUCT_PTR(pintshcut, CInternetShortcut));

   SetWindowLongPtr(hdlg, DWLP_USER, NULL);

   return(TRUE);
}

#pragma warning(default:4100) /* "unreferenced formal parameter" warning */


PRIVATE_CODE void ISPSChanged(HWND hdlg)
{
   ASSERT(IS_VALID_HANDLE(hdlg, WND));

   PropSheet_Changed(GetParent(hdlg), hdlg);

   return;
}


PRIVATE_CODE HRESULT ChooseIcon(HWND hdlg)
{
   HRESULT hr;
   PISPS pisps;
   PInternetShortcut pintshcut;
   char rgchTempIconFile[MAX_PATH_LEN];
   int niIcon;
   UINT uFlags;

   ASSERT(IS_VALID_HANDLE(hdlg, WND));

   pisps = (PISPS)GetWindowLongPtr(hdlg, DWLP_USER);
   ASSERT(IS_VALID_STRUCT_PTR(pisps, CISPS));
   pintshcut = pisps->pintshcut;
   ASSERT(IS_VALID_STRUCT_PTR(pintshcut, CInternetShortcut));

   if (pintshcut->GetIconLocation(0, rgchTempIconFile,
                  sizeof(rgchTempIconFile), &niIcon, &uFlags)
       != S_OK)
   {
      rgchTempIconFile[0] = '\0';
      niIcon = 0;
   }

   ASSERT(lstrlen(rgchTempIconFile) < sizeof(rgchTempIconFile));
   if (RUNNING_NT)
   {
        WCHAR uTempIconFile[MAX_PATH_LEN];

        MultiByteToWideChar(CP_ACP, 0, rgchTempIconFile, -1,
                    uTempIconFile, MAX_PATH_LEN);

        if (PickIconDlg(hdlg, (LPSTR)uTempIconFile, MAX_PATH_LEN, &niIcon))
        {
            WideCharToMultiByte(CP_ACP, 0, uTempIconFile, -1, pisps->rgchIconFile,
                                 MAX_PATH_LEN, NULL, NULL);

            pisps->niIcon = niIcon;

            hr = S_OK;
        }
        else
        {
            hr = E_FAIL;

            TRACE_OUT(("ChooseIcon(NT): PickIconDlg() failed."));
        }
   }
   else
   {
       if (PickIconDlg(hdlg, rgchTempIconFile, sizeof(rgchTempIconFile), &niIcon))
       {
          ASSERT(lstrlen(rgchTempIconFile) < sizeof(pisps->rgchIconFile));
          lstrcpy(pisps->rgchIconFile, rgchTempIconFile);
          pisps->niIcon = niIcon;

          hr = S_OK;
       }
       else
       {
          hr = E_FAIL;

          TRACE_OUT(("ChooseIcon(): PickIconDlg() failed."));
       }
   }

   return(hr);
}


PRIVATE_CODE void UpdateISPSIcon(HWND hdlg)
{
   PISPS pisps;
   HICON hicon;

   ASSERT(IS_VALID_HANDLE(hdlg, WND));

   pisps = (PISPS)GetWindowLongPtr(hdlg, DWLP_USER);
   ASSERT(IS_VALID_STRUCT_PTR(pisps, CISPS));
   ASSERT(pisps->rgchIconFile[0]);

   // BUGBUG: This icon does not have the link arrow overlayed.  shell32.dll's
   // Shortcut property sheet has the same bug.

   hicon = ExtractIcon(GetThisModulesHandle(), pisps->rgchIconFile,
               pisps->niIcon);

   if (hicon)
      SetISPSIcon(hdlg, hicon);
   else
      WARNING_OUT(("UpdateISPSIcon(): ExtractIcon() failed for icon %d in file %s.",
           pisps->niIcon,
           pisps->rgchIconFile));

   return;
}


#pragma warning(disable:4100) /* "unreferenced formal parameter" warning */

PRIVATE_CODE BOOL ISPS_Command(HWND hdlg, WPARAM wparam, LPARAM lparam)
{
   BOOL bMsgHandled = FALSE;
   WORD wCmd;

   // wparam may be any value.
   // lparam may be any value.

   ASSERT(IS_VALID_HANDLE(hdlg, WND));

   wCmd = HIWORD(wparam);

   switch (LOWORD(wparam))
   {
      case IDD_URL:
      case IDD_HOTKEY:
      case IDD_START_IN:
     if (wCmd == EN_CHANGE)
     {
        ISPSChanged(hdlg);

        bMsgHandled = TRUE;
     }
     break;

      case IDD_SHOW_CMD:
     if (wCmd == LBN_SELCHANGE)
     {
        ISPSChanged(hdlg);

        bMsgHandled = TRUE;
     }
     break;

      case IDD_CHANGE_ICON:
     // Ignore return value.
     if (ChooseIcon(hdlg) == S_OK)
     {
        UpdateISPSIcon(hdlg);
        ISPSChanged(hdlg);
     }
     bMsgHandled = TRUE;
     break;

      default:
     break;
   }

   return(bMsgHandled);
}

#pragma warning(default:4100) /* "unreferenced formal parameter" warning */


PRIVATE_CODE HRESULT ComplainAboutURL(HWND hwndParent, PCSTR pcszURL,
                      HRESULT hrError)
{
   HRESULT hr;
   int nResult;

   // Validate hrError below.

   ASSERT(IS_VALID_HANDLE(hwndParent, WND));
   ASSERT(IS_VALID_STRING_PTR(pcszURL, CSTR));

   switch (hrError)
   {
      case URL_E_UNREGISTERED_PROTOCOL:
      {
     PSTR pszProtocol;

     hr = CopyURLProtocol(pcszURL, &pszProtocol);

     if (hr == S_OK)
     {
        if (MyMsgBox(hwndParent, MAKEINTRESOURCE(IDS_SHORTCUT_ERROR_TITLE),
             MAKEINTRESOURCE(IDS_UNREGISTERED_PROTOCOL),
             (MB_YESNO | MB_DEFBUTTON2 | MB_ICONEXCLAMATION),
             &nResult, pszProtocol))
        {
           switch (nResult)
           {
          case IDYES:
             hr = S_OK;
             TRACE_OUT(("ComplainAboutURL(): Allowing URL %s despite unregistered protocol %s, by request.",
                pcszURL,
                pszProtocol));
             break;

          default:
             ASSERT(nResult == IDNO);
             hr = E_FAIL;
             TRACE_OUT(("ComplainAboutURL(): Not allowing URL %s because of unregistered protocol %s, as directed.",
                pcszURL,
                pszProtocol));
             break;
           }
        }

        delete pszProtocol;
        pszProtocol = NULL;
     }

     break;
      }

      default:
     ASSERT(hrError == URL_E_INVALID_SYNTAX);

     if (MyMsgBox(hwndParent, MAKEINTRESOURCE(IDS_SHORTCUT_ERROR_TITLE),
              MAKEINTRESOURCE(IDS_INVALID_URL_SYNTAX),
              (MB_OK | MB_ICONEXCLAMATION), &nResult, pcszURL)) {
        ASSERT(nResult == IDOK);
     }

     hr = E_FAIL;

     TRACE_OUT(("ComplainAboutURL(): Not allowing URL %s because of invalid syntax.",
            pcszURL));

     break;
   }

   return(hr);
}


#pragma warning(disable:4100) /* "unreferenced formal parameter" warning */

PRIVATE_CODE HRESULT ComplainAboutWorkingDirectory(HWND hwndParent,
                           PCSTR pcszWorkingDirectory,
                           HRESULT hrError)
{
   int nResult;

   ASSERT(IS_VALID_HANDLE(hwndParent, WND));
   ASSERT(IS_VALID_STRING_PTR(pcszWorkingDirectory, CSTR));
   ASSERT(hrError == E_PATH_NOT_FOUND);

   if (MyMsgBox(hwndParent, MAKEINTRESOURCE(IDS_SHORTCUT_ERROR_TITLE),
        MAKEINTRESOURCE(IDS_WORKING_DIR_NOT_FOUND),
        (MB_OK | MB_ICONEXCLAMATION), &nResult, pcszWorkingDirectory)) {
      ASSERT(nResult == IDOK);
   }

   TRACE_OUT(("ComplainAboutWorkingDirectory(): Not allowing non-existent working directory %s.",
          pcszWorkingDirectory));

   return(E_FAIL);
}

#pragma warning(default:4100) /* "unreferenced formal parameter" warning */


PRIVATE_CODE HRESULT InjectISPSData(HWND hdlg)
{
   HRESULT hr;
   PISPS pisps;
   PInternetShortcut pintshcut;
   PSTR pszURL;

   ASSERT(IS_VALID_HANDLE(hdlg, WND));

   pisps = (PISPS)GetWindowLongPtr(hdlg, DWLP_USER);
   ASSERT(IS_VALID_STRUCT_PTR(pisps, CISPS));
   pintshcut = pisps->pintshcut;
   ASSERT(IS_VALID_STRUCT_PTR(pintshcut, CInternetShortcut));

   hr = CopyDlgItemText(hdlg, IDD_URL, &pszURL);

   if (SUCCEEDED(hr))
   {
      PCSTR pcszURLToUse;
      PSTR pszTranslatedURL;

      pcszURLToUse = pszURL;

      if (hr == S_OK)
      {
     hr = TranslateURL(pszURL, (TRANSLATEURL_FL_GUESS_PROTOCOL |
                    TRANSLATEURL_FL_USE_DEFAULT_PROTOCOL),
               &pszTranslatedURL);

     if (SUCCEEDED(hr))
     {
        if (hr == S_OK)
           pcszURLToUse = pszTranslatedURL;
        else
           ASSERT(hr == S_FALSE);

        hr = ValidateURL(pcszURLToUse);

        if (FAILED(hr))
        {
           hr = ComplainAboutURL(hdlg, pcszURLToUse, hr);

           if (FAILED(hr))
          SetEditFocus(GetDlgItem(hdlg, IDD_URL));
        }
     }
      }
      else
      {
     // A blank URL is OK.

     ASSERT(hr == S_FALSE);

     pszTranslatedURL = NULL;
      }

      if (SUCCEEDED(hr))
      {
     hr = pintshcut->SetURL(pcszURLToUse, 0);

     if (hr == S_OK)
     {
        WORD wHotkey;
        WORD wOldHotkey;

        // Refresh URL in case it was changed by TranslateURL().

        SetISPSURL(hdlg);

        wHotkey = (WORD)SendDlgItemMessage(hdlg, IDD_HOTKEY, HKM_GETHOTKEY,
                           0, 0);

        hr = pintshcut->GetHotkey(&wOldHotkey);

        if (hr == S_OK)
        {
           hr = pintshcut->SetHotkey(wHotkey);

           if (hr == S_OK)
           {
          char szFile[MAX_PATH_LEN];

          hr = pintshcut->GetCurFile(szFile, sizeof(szFile));

          if (hr == S_OK)
          {
             if (RegisterGlobalHotkey(wOldHotkey, wHotkey, szFile))
             {
            PSTR pszWorkingDirectory;

            hr = CopyDlgItemText(hdlg, IDD_START_IN,
                         &pszWorkingDirectory);

            if (SUCCEEDED(hr))
            {
               if (hr == S_OK)
               {
                  hr = ValidateWorkingDirectory(pszWorkingDirectory);

                  if (FAILED(hr))
                 hr = ComplainAboutWorkingDirectory(hdlg,
                                    pszWorkingDirectory,
                                    hr);

                  if (FAILED(hr))
                 SetEditFocus(GetDlgItem(hdlg, IDD_START_IN));
               }

               if (SUCCEEDED(hr))
               {
                  hr = pintshcut->SetWorkingDirectory(pszWorkingDirectory);

                  if (hr == S_OK)
                  {
                 // Refresh working directory in case it was changed by
                 // SetWorkingDirectory().

                 SetISPSWorkingDirectory(hdlg);

                 if (pisps->rgchIconFile[0])
                    hr = pintshcut->SetIconLocation(pisps->rgchIconFile,
                                    pisps->niIcon);

                 if (hr == S_OK)
                 {
                    INT_PTR iShowCmd;

                    iShowCmd = SendDlgItemMessage(hdlg,
                                  IDD_SHOW_CMD,
                                  CB_GETCURSEL,
                                  0, 0);

                    if (iShowCmd >= 0 &&
                    iShowCmd < ARRAY_ELEMENTS(s_rgnShowCmds))
                       pintshcut->SetShowCmd(s_rgnShowCmds[iShowCmd]);
                    else
                       hr = E_UNEXPECTED;
                 }
                  }
               }

               if (pszWorkingDirectory)
               {
                  delete pszWorkingDirectory;
                  pszWorkingDirectory = NULL;
               }
            }
             }
             else
            hr = E_FAIL;
          }
           }
        }
     }
      }

      if (pszURL)
      {
     delete pszURL;
     pszURL = NULL;
      }

      if (pszTranslatedURL)
      {
     LocalFree(pszTranslatedURL);
     pszTranslatedURL = NULL;
      }
   }

   if (hr == S_OK)
      TRACE_OUT(("InjectISPSData(): Injected property sheet data into Internet Shortcut successfully."));
   else
      WARNING_OUT(("InjectISPSData(): Failed to inject property sheet data into Internet Shortcut, returning %s.",
           GetHRESULTString(hr)));

   return(hr);
}


PRIVATE_CODE HRESULT ISPSSave(HWND hdlg)
{
   HRESULT hr;
   PInternetShortcut pintshcut;

   ASSERT(IS_VALID_HANDLE(hdlg, WND));

   pintshcut = ((PISPS)GetWindowLongPtr(hdlg, DWLP_USER))->pintshcut;
   ASSERT(IS_VALID_STRUCT_PTR(pintshcut, CInternetShortcut));

   if (pintshcut->IsDirty() == S_OK)
   {
      hr = pintshcut->Save((LPCOLESTR)NULL, FALSE);

      if (hr == S_OK)
     TRACE_OUT(("ISPSSave(): Saved Internet Shortcut successfully."));
      else
     WARNING_OUT(("ISPSSave(): Save() failed, returning %s.",
              GetHRESULTString(hr)));
   }
   else
   {
      TRACE_OUT(("ISPSSave(): Internet Shortcut unchanged.  No save required."));

      hr = S_OK;
   }

   return(hr);
}


#pragma warning(disable:4100) /* "unreferenced formal parameter" warning */

PRIVATE_CODE BOOL ISPS_Notify(HWND hdlg, WPARAM wparam, LPARAM lparam)
{
   BOOL bMsgHandled = FALSE;

   // wparam may be any value.
   // lparam may be any value.

   ASSERT(IS_VALID_HANDLE(hdlg, WND));

   switch (((PNMHDR)lparam)->code)
   {
      case PSN_APPLY:
     SetWindowLongPtr(hdlg, DWLP_MSGRESULT, ISPSSave(hdlg) == S_OK ?
                        PSNRET_NOERROR :
                        PSNRET_INVALID_NOCHANGEPAGE);
     bMsgHandled = TRUE;
     break;

      case PSN_KILLACTIVE:
     SetWindowLongPtr(hdlg, DWLP_MSGRESULT, FAILED(InjectISPSData(hdlg)));
     bMsgHandled = TRUE;
     break;

      default:
     break;
   }

   return(bMsgHandled);
}

#pragma warning(default:4100) /* "unreferenced formal parameter" warning */


PRIVATE_CODE PCSTR ISPS_GetHelpFileFromControl(HWND hwndControl)
{
   PCSTR pcszHelpFile = NULL;
   int nControlID = 0;

   ASSERT(! hwndControl ||
      IS_VALID_HANDLE(hwndControl, WND));

   if (hwndControl)
   {
      nControlID = GetDlgCtrlID(hwndControl);

      switch (nControlID)
      {
     case IDD_URL_TEXT:
     case IDD_URL:
        // URL help comes from the Plus! Pack help file.
        pcszHelpFile = s_cszPlusHelpFile;
        break;

     default:
        // Other help is borrowed from the default Win95 help file.
        break;
      }
   }

   TRACE_OUT(("ISPS_GetHelpFileFromControl(): Using %s for control %d (HWND %#lx).",
          pcszHelpFile ? pcszHelpFile : "default Win95 help file",
          nControlID,
          hwndControl));

   ASSERT(! pcszHelpFile ||
      IS_VALID_STRING_PTR(pcszHelpFile, CSTR));

   return(pcszHelpFile);
}


PRIVATE_CODE INT_PTR CALLBACK ISPS_DlgProc(HWND hdlg, UINT uMsg, WPARAM wparam,
                    LPARAM lparam)
{
   INT_PTR bMsgHandled = FALSE;

   // uMsg may be any value.
   // wparam may be any value.
   // lparam may be any value.

   ASSERT(IS_VALID_HANDLE(hdlg, WND));

   switch (uMsg)
   {
      case WM_INITDIALOG:
     bMsgHandled = ISPS_InitDialog(hdlg, wparam, lparam);
     break;

      case WM_DESTROY:
     bMsgHandled = ISPS_Destroy(hdlg, wparam, lparam);
     break;

      case WM_COMMAND:
     bMsgHandled = ISPS_Command(hdlg, wparam, lparam);
     break;

      case WM_NOTIFY:
     bMsgHandled = ISPS_Notify(hdlg, wparam, lparam);
     break;

      case WM_HELP:
         SHWinHelpOnDemandWrap((HWND)(((LPHELPINFO)lparam)->hItemHandle),
         ISPS_GetHelpFileFromControl((HWND)(((LPHELPINFO)lparam)->hItemHandle)),
         HELP_WM_HELP, (DWORD_PTR)(PVOID)s_rgdwHelpIDs);
     bMsgHandled = TRUE;
     break;

      case WM_CONTEXTMENU:
      {
     POINT pt;

     LPARAM_TO_POINT(lparam, pt);
     EVAL(ScreenToClient(hdlg, &pt));

         SHWinHelpOnDemandWrap((HWND)wparam,
         ISPS_GetHelpFileFromControl(ChildWindowFromPoint(hdlg, pt)),
         HELP_CONTEXTMENU, (DWORD_PTR)(PVOID)s_rgdwHelpIDs);
     bMsgHandled = TRUE;
     break;
      }

      default:
    break;
   }

   return(bMsgHandled);
}


PRIVATE_CODE HRESULT AddISPS(PInternetShortcut pintshcut,
                 LPFNADDPROPSHEETPAGE pfnAddPage, LPARAM lparam)
{
   HRESULT hr;
   ISPS isps;
   HPROPSHEETPAGE hpsp;

   // lparam may be any value.

   ASSERT(IS_VALID_STRUCT_PTR(pintshcut, CInternetShortcut));
   ASSERT(IS_VALID_CODE_PTR(pfnAddPage, LPFNADDPROPSHEETPAGE));

   ZeroMemory(&isps, sizeof(isps));

   isps.psp.dwSize = sizeof(isps);
   isps.psp.dwFlags = (PSP_DEFAULT | PSP_USECALLBACK);
   isps.psp.hInstance = MLGetHinst();
   isps.psp.pszTemplate = MAKEINTRESOURCE(DLG_INTERNET_SHORTCUT_PROP_SHEET);
   isps.psp.pfnDlgProc = &ISPS_DlgProc;
   isps.psp.pfnCallback = &ISPSCallback;

   isps.pintshcut = pintshcut;

   ASSERT(IS_VALID_STRUCT_PTR(&isps, CISPS));

   hpsp = CreatePropertySheetPage(&(isps.psp));

   if (hpsp)
   {
      if ((*pfnAddPage)(hpsp, lparam))
      {
     pintshcut->AddRef();
     hr = S_OK;
     TRACE_OUT(("AddISPS(): Added Internet Shortcut property sheet."));
      }
      else
      {
     DestroyPropertySheetPage(hpsp);

     hr = E_FAIL;
     WARNING_OUT(("AddISPS(): Callback to add property sheet failed."));
      }
   }
   else
      hr = E_OUTOFMEMORY;

   return(hr);
}


/****************************** Public Functions *****************************/


PUBLIC_CODE void SetEditFocus(HWND hwnd)
{
   ASSERT(IS_VALID_HANDLE(hwnd, WND));

   SetFocus(hwnd);
   SendMessage(hwnd, EM_SETSEL, 0, -1);

   return;
}


PUBLIC_CODE BOOL ConstructMessageString(PCSTR pcszFormat, PSTR *ppszMsg,
                    va_list *ArgList)
{
   BOOL bResult;
   char rgchFormat[MAX_MSG_LEN];

   // ArgList may be any value.

   ASSERT(! HIWORD(pcszFormat) ||
      IS_VALID_STRING_PTR(pcszFormat, CSTR));
   ASSERT(IS_VALID_WRITE_PTR(ppszMsg, PSTR));

   if (IS_INTRESOURCE(pcszFormat))
   {
      MLLoadStringA(LOWORD(PtrToUlong(pcszFormat)), rgchFormat,
         sizeof(rgchFormat));

      pcszFormat = rgchFormat;
   }

   bResult = FormatMessage((FORMAT_MESSAGE_ALLOCATE_BUFFER |
                FORMAT_MESSAGE_FROM_STRING), pcszFormat, 0, 0,
               (PSTR)ppszMsg, 0, ArgList);

   ASSERT(! bResult ||
      IS_VALID_STRING_PTR(*ppszMsg, STR));

   return(bResult);
}


PUBLIC_CODE BOOL __cdecl MyMsgBox(HWND hwndParent, PCSTR pcszTitle,
                  PCSTR pcszMsgFormat, DWORD dwMsgBoxFlags,
                  PINT pnResult, ...)
{
   BOOL bResult;
   va_list ArgList;
   PSTR pszMsg;

   ASSERT(IS_VALID_HANDLE(hwndParent, WND));
   ASSERT(IS_INTRESOURCE(pcszTitle) ||
      IS_VALID_STRING_PTR(pcszTitle, CSTR));
   ASSERT(IS_INTRESOURCE(pcszMsgFormat) ||
      IS_VALID_STRING_PTR(pcszMsgFormat, CSTR));
   ASSERT(FLAGS_ARE_VALID(dwMsgBoxFlags, ALL_MSG_BOX_FLAGS));
   ASSERT(IS_VALID_WRITE_PTR(pnResult, INT));

   *pnResult = 0;

   va_start(ArgList, pnResult);

   bResult = ConstructMessageString(pcszMsgFormat, &pszMsg, &ArgList);

   va_end(ArgList);

   if (bResult)
   {
      char rgchTitle[MAX_MSG_LEN];

      if (! HIWORD(pcszTitle))
      {
          MLLoadStringA(LOWORD(PtrToUlong(pcszTitle)), rgchTitle,
            sizeof(rgchTitle));

          pcszTitle = rgchTitle;
      }

      *pnResult = MessageBox(hwndParent, pszMsg, pcszTitle, dwMsgBoxFlags);

      bResult = (*pnResult != 0);

      LocalFree(pszMsg);
      pszMsg = NULL;
   }

   return(bResult);
}


PUBLIC_CODE HRESULT CopyDlgItemText(HWND hdlg, int nControlID, PSTR *ppszText)
{
   HRESULT hr;
   HWND hwndControl;

   // nContolID may be any value.

   ASSERT(IS_VALID_HANDLE(hdlg, WND));
   ASSERT(IS_VALID_WRITE_PTR(ppszText, PSTR));

   *ppszText = NULL;

   hwndControl = GetDlgItem(hdlg, nControlID);

   if (hwndControl)
   {
      int ncchTextLen;

      ncchTextLen = GetWindowTextLength(hwndControl);

      if (ncchTextLen > 0)
      {
     PSTR pszText;

     ASSERT(ncchTextLen < INT_MAX);
     ncchTextLen++;
     ASSERT(ncchTextLen > 0);

     pszText = new(char[ncchTextLen]);

     if (pszText)
     {
        int ncchCopiedLen;

        ncchCopiedLen = GetWindowText(hwndControl, pszText, ncchTextLen);
        ASSERT(ncchCopiedLen == ncchTextLen - 1);

        if (EVAL(ncchCopiedLen > 0))
        {
           if (AnyMeat(pszText))
           {
          *ppszText = pszText;

          hr = S_OK;
           }
           else
          hr = S_FALSE;
        }
        else
           hr = E_FAIL;

        if (hr != S_OK)
        {
           delete pszText;
           pszText = NULL;
        }
     }
     else
        hr = E_OUTOFMEMORY;
      }
      else
     // No text.
     hr = S_FALSE;
   }
   else
      hr = E_FAIL;

   return(hr);
}


PUBLIC_CODE BOOL RegisterGlobalHotkey(WORD wOldHotkey, WORD wNewHotkey,
                      PCSTR pcszPath)
{
   BOOL bResult;
   HWND hwndTray;

   ASSERT(! wOldHotkey ||
      IsValidHotkey(wOldHotkey));
   ASSERT(! wNewHotkey ||
      IsValidHotkey(wNewHotkey));
   ASSERT(IsValidPath(pcszPath));

   hwndTray = FindWindow(s_cszTrayNotificationClass, 0);

   if (hwndTray)
   {
      if (wOldHotkey)
      {
          SendMessage(hwndTray, WMTRAY_SCUNREGISTERHOTKEY, wOldHotkey, 0);

          TRACE_OUT(("RegisterGlobalHotkey(): Unregistered old hotkey %#04x for %s.",
              wOldHotkey,
              pcszPath));
      }

      if (wNewHotkey)
      {
        if (RUNNING_NT)
        {
            ATOM atom = GlobalAddAtom(pcszPath);
            ASSERT(atom);
            if (atom)
            {
                SendMessage(hwndTray, WMTRAY_SCREGISTERHOTKEY, wNewHotkey, (LPARAM)atom);
                GlobalDeleteAtom(atom);
            }
        }
        else
        {
            SendMessage(hwndTray, WMTRAY_SCREGISTERHOTKEY, wNewHotkey,
                        (LPARAM)pcszPath);
        }

          TRACE_OUT(("RegisterGlobalHotkey(): Registered new hotkey %#04x for %s.",
              wNewHotkey,
              pcszPath));
      }

      bResult = TRUE;
   }
   else
   {
      bResult = FALSE;

      WARNING_OUT(("RegisterGlobalHotkey(): Unable to find Tray window of class %s to notify.",
           s_cszTrayNotificationClass));
   }

   return(bResult);
}

/********************************** Methods **********************************/


#pragma warning(disable:4100) /* "unreferenced formal parameter" warning */

HRESULT STDMETHODCALLTYPE InternetShortcut::Initialize(
                              PCITEMIDLIST pcidlFolder,
                              PIDataObject pido,
                              HKEY hkeyProgID)
{
   HRESULT hr;
   FORMATETC fmtetc = { CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
   STGMEDIUM stgmed;

   DebugEntry(InternetShortcut::Initialize);

   ASSERT(IS_VALID_STRUCT_PTR(this, CInternetShortcut));
   ASSERT(! pcidlFolder ||
      IS_VALID_STRUCT_PTR(pcidlFolder, CITEMIDLIST));
   ASSERT(IS_VALID_INTERFACE_PTR(pido, IDataObject));
   ASSERT(IS_VALID_HANDLE(hkeyProgID, KEY));

   hr = pido->GetData(&fmtetc, &stgmed);

   if (hr == S_OK)
   {
      UINT ucbPathLen;
      PSTR pszFile;

      // (+ 1) for null terminator.

      ucbPathLen = DragQueryFile((HDROP)(stgmed.hGlobal), 0, NULL, 0) + 1;
      ASSERT(ucbPathLen > 0);

      pszFile = new(char[ucbPathLen]);

      if (pszFile)
      {
     EVAL(DragQueryFile((HDROP)(stgmed.hGlobal), 0, pszFile, ucbPathLen) == ucbPathLen - 1);
     ASSERT(IS_VALID_STRING_PTR(pszFile, STR));
     ASSERT((UINT)lstrlen(pszFile) == ucbPathLen - 1);

     hr = LoadFromFile(pszFile, TRUE);

     delete pszFile;
     pszFile = NULL;
      }
      else
     hr = E_OUTOFMEMORY;

      if (stgmed.tymed != TYMED_NULL)
         MyReleaseStgMedium(&stgmed);
   }

   ASSERT(IS_VALID_STRUCT_PTR(this, CInternetShortcut));

   DebugExitHRESULT(InternetShortcut::Initialize, hr);

   return(hr);
}

#pragma warning(default:4100) /* "unreferenced formal parameter" warning */


HRESULT STDMETHODCALLTYPE InternetShortcut::AddPages(
                         LPFNADDPROPSHEETPAGE pfnAddPage,
                         LPARAM lparam)
{
   HRESULT hr;

   DebugEntry(InternetShortcut::AddPages);

   // lparam may be any value.

   ASSERT(IS_VALID_STRUCT_PTR(this, CInternetShortcut));
   ASSERT(IS_VALID_CODE_PTR(pfnAddPage, LPFNADDPROPSHEETPAGE));

   hr = AddISPS(this, pfnAddPage, lparam);

   ASSERT(IS_VALID_STRUCT_PTR(this, CInternetShortcut));

   DebugExitHRESULT(InternetShortcut::AddPages, hr);

   return(hr);
}


#pragma warning(disable:4100) /* "unreferenced formal parameter" warning */

HRESULT STDMETHODCALLTYPE InternetShortcut::ReplacePage(
                      UINT uPageID,
                      LPFNADDPROPSHEETPAGE pfnReplaceWith,
                      LPARAM lparam)
{
   HRESULT hr;

   DebugEntry(InternetShortcut::ReplacePage);

   // lparam may be any value.
   // uPageID may be any value.

   ASSERT(IS_VALID_STRUCT_PTR(this, CInternetShortcut));
   ASSERT(IS_VALID_CODE_PTR(pfnReplaceWith, LPFNADDPROPSHEETPAGE));

   hr = E_NOTIMPL;

   ASSERT(IS_VALID_STRUCT_PTR(this, CInternetShortcut));

   DebugExitHRESULT(InternetShortcut::ReplacePage, hr);

   return(hr);
}

#pragma warning(default:4100) /* "unreferenced formal parameter" warning */
