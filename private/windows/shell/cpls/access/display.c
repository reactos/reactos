/******************************************************************************
Module name: Display.C
Purpose: Display Dialog handler
******************************************************************************/


#include "Access.h"

//////////////////////////////////////////////////////////////////////////////
/*******************************************************************
 *	  DESCRIPTION: High Contrast dialog handler
 *******************************************************************/

#include "Access.h"

VOID FillCustonSchemeBox (HWND hwndCB) {
    HKEY hkey;
    int i;
    DWORD dwDisposition;

    // Get the class name and the value count.
    if (RegCreateKeyEx(HKEY_CURRENT_USER, CONTROL_KEY, 0, __TEXT(""),
        REG_OPTION_NON_VOLATILE, KEY_ENUMERATE_SUB_KEYS | KEY_EXECUTE | KEY_QUERY_VALUE,
        NULL, &hkey, &dwDisposition) != ERROR_SUCCESS) return;

    // Enumerate the child keys.
    for (i = 0; ; i++) {
        DWORD cbValueName;
        TCHAR szValueName[MAX_SCHEME_NAME_SIZE];
        LONG l;

        cbValueName = MAX_SCHEME_NAME_SIZE;
        l = RegEnumValue(hkey, i, szValueName, &cbValueName, NULL, NULL, NULL, NULL);
        if (ERROR_NO_MORE_ITEMS == l) break;

        // Add each value to a combobox.
        if (lstrlen(szValueName) == 0) lstrcpy(szValueName, __TEXT("<NO NAME>"));
        ComboBox_AddString(hwndCB, ((szValueName[0] == 0) ? __TEXT("<NO NAME>") : szValueName));
    }
    RegCloseKey(hkey);
}



// ****************************************************************************
// Main HC Dialog handler
// ****************************************************************************
INT_PTR WINAPI HighContrastDlg (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    HKEY  hkey;
    HWND  hwndCB = GetDlgItem(hwnd, IDC_HC_DEFAULTSCHEME);
    int   i;
    DWORD dwDisposition;
    TCHAR szBuf[MAX_SCHEME_NAME_SIZE];
    DWORD   cbSize;
    BOOL  fProcessed = TRUE;
    TCHAR szScheme[MAX_SCHEME_NAME_SIZE];
    HKEY hReg;
    long retval;

    switch (uMsg) {
    case WM_INITDIALOG:
        CheckDlgButton(hwnd, IDC_HC_HOTKEY, (g_hc.dwFlags & HCF_HOTKEYACTIVE) ? TRUE : FALSE);
        CheckRadioButton(hwnd, IDC_HC_WHITE_BLACK, IDC_HC_CUSTOM, IDC_HC_WHITE_BLACK);

        FillCustonSchemeBox(hwndCB);

        // Get the last custom scheme
        cbSize = sizeof(szScheme);
        szScheme[0] = 0;

        retval = RegCreateKey(HKEY_CURRENT_USER, HC_KEY, &hReg);
        if (retval == ERROR_SUCCESS) {

            RegQueryValueEx(hReg, HC_KEY LAST_CUSTOM_SCHEME, NULL,
               NULL, (PBYTE) szScheme, &cbSize);

            RegCloseKey(hReg);
        }

        // Set the proper selection in the combobox.
        if (ComboBox_SelectString(hwndCB, -1, szScheme) == CB_ERR) {
               // Not found, select the 1st one.
               ComboBox_SetCurSel(hwndCB, 0);
        }

        // Leave disabled since we don't have schemes yet.
        EnableWindow(hwndCB, FALSE);

        // BCK - 3/14/95 - make sure white/black is default if no custom is selected.
        if (g_hc.lpszDefaultScheme[0] == 0) {
                // No HC Scheme has been selected as yet.  Make it WHITE on BLACK.
                CheckRadioButton(hwnd, IDC_HC_WHITE_BLACK, IDC_HC_CUSTOM, IDC_HC_WHITE_BLACK);
                EnableWindow(hwndCB, FALSE);

        // Set last and current scheme names to new default color scheme.
                if (!IsMUI_Enabled())
                {
                   LoadString(g_hinst, IDS_WHITEBLACK_SCHEME, g_hc.lpszDefaultScheme, 200);
                }
                else
                {
                   lstrcpy(g_hc.lpszDefaultScheme, IDSENG_WHITEBLACK_SCHEME);
                }
        } else {
               // We do have a color scheme in CurHCScheme.  Check if its BlackonWHite
               if (!IsMUI_Enabled())
               {
                  LoadString(g_hinst, IDS_WHITEBLACK_SCHEME, szBuf, 200);
               }
               else
               {
                  lstrcpy(szBuf,IDSENG_WHITEBLACK_SCHEME);
               }
               if (lstrcmp(g_hc.lpszDefaultScheme, szBuf) == 0) {
                      // Color scheme is Blackon WHite.  Check the correct radio button.
                      CheckRadioButton(hwnd, IDC_HC_WHITE_BLACK, IDC_HC_CUSTOM, IDC_HC_WHITE_BLACK);
               } else {
                      // Check if it is whiteonblack.
                      if (!IsMUI_Enabled())
                      {
                         LoadString(g_hinst, IDS_BLACKWHITE_SCHEME, szBuf, 200);
                      }
                      else
                      {
                         lstrcpy(szBuf,IDSENG_BLACKWHITE_SCHEME);
                      }
                      if (lstrcmp(g_hc.lpszDefaultScheme, szBuf) == 0) {
                         // it is white on black.  Check the radio button.
                         CheckRadioButton(hwnd, IDC_HC_WHITE_BLACK, IDC_HC_CUSTOM, IDC_HC_BLACK_WHITE);
                         EnableWindow(hwndCB, FALSE);
                      } else {
                         // Its not white on black, and its not black on white.  Enable
                         // the custom control.
                         CheckRadioButton(hwnd, IDC_HC_WHITE_BLACK, IDC_HC_CUSTOM, IDC_HC_CUSTOM);
                         EnableWindow(hwndCB, TRUE);
                         SendMessage(hwndCB, CB_SELECTSTRING, -1, (LONG_PTR)g_hc.lpszDefaultScheme);
                      }
               }
        }
        break;

    case WM_HELP:         // F1
        WinHelp(((LPHELPINFO) lParam)->hItemHandle, __TEXT("access.hlp"), HELP_WM_HELP, (DWORD_PTR) (LPSTR) g_aIds);
        break;

    case WM_CONTEXTMENU:  // right mouse click
        WinHelp((HWND) wParam, __TEXT("access.hlp"), HELP_CONTEXTMENU, (DWORD_PTR) (LPSTR) g_aIds);
        break;

   // Handle the generic commands
    case WM_COMMAND:
        switch (GET_WM_COMMAND_ID(wParam, lParam)) {
        case IDC_HC_HOTKEY:
               g_hc.dwFlags ^=  HCF_HOTKEYACTIVE;
               break;

        case IDC_HC_BLACK_WHITE:
               // Disable the default scheme checkbox
               EnableWindow(hwndCB, FALSE);
               CheckRadioButton(hwnd, IDC_HC_WHITE_BLACK, IDC_HC_CUSTOM, IDC_HC_BLACK_WHITE);
               if (!IsMUI_Enabled())
               {
                  LoadString(g_hinst, IDS_BLACKWHITE_SCHEME, g_hc.lpszDefaultScheme, 200);
               }
               else
               {
                  lstrcpy(g_hc.lpszDefaultScheme,IDSENG_BLACKWHITE_SCHEME);
               }
               break;

        case IDC_HC_WHITE_BLACK:
               EnableWindow(hwndCB, FALSE);
               CheckRadioButton(hwnd, IDC_HC_WHITE_BLACK, IDC_HC_CUSTOM, IDC_HC_WHITE_BLACK);
               if (!IsMUI_Enabled())
               {
                  LoadString(g_hinst, IDS_WHITEBLACK_SCHEME, g_hc.lpszDefaultScheme, 200);
               }
               else
               {
                  lstrcpy(g_hc.lpszDefaultScheme,IDSENG_WHITEBLACK_SCHEME);
               }
               break;

        // The user has selected the custom button.
        case IDC_HC_CUSTOM:
               CheckRadioButton(hwnd, IDC_HC_WHITE_BLACK, IDC_HC_CUSTOM, IDC_HC_CUSTOM);
               EnableWindow(hwndCB, TRUE);
               i = ComboBox_GetCurSel(hwndCB);
               ComboBox_GetLBText(hwndCB, i, g_hc.lpszDefaultScheme);
               break;

        case IDC_HC_DEFAULTSCHEME:
               if (GET_WM_COMMAND_CMD(wParam, lParam) == CBN_SELCHANGE) {
                       // Get the current string into our variable
                       i = ComboBox_GetCurSel(hwndCB);
                       ComboBox_GetLBText(hwndCB, i, g_hc.lpszDefaultScheme);
               }
               break;

        case IDOK:
               // Save the current custom scheme to the registry.
               if (ERROR_SUCCESS == RegCreateKeyEx(
                      HKEY_CURRENT_USER,
                      HC_KEY,
                      0,
                      __TEXT(""),
                      REG_OPTION_NON_VOLATILE,
                      KEY_EXECUTE | KEY_QUERY_VALUE | KEY_SET_VALUE,
                      NULL,
                      &hkey,
                      &dwDisposition)) {

                      TCHAR szCust[MAX_SCHEME_NAME_SIZE];

                      i = ComboBox_GetCurSel(hwndCB);
                      ComboBox_GetLBText(hwndCB, i, szCust);
                      RegSetValueEx(hkey, LAST_CUSTOM_SCHEME, 0, REG_SZ, (PBYTE) szCust,      lstrlen(szCust));
               }
               EndDialog(hwnd, IDOK);
               break;

        case IDCANCEL:
               EndDialog(hwnd, IDCANCEL);
               break;
        }
        break;

        default:
               fProcessed = FALSE; break;
   }
   return((INT_PTR) fProcessed);
}


// *******************************************************************
// DisplayDialog handler
// *******************************************************************
INT_PTR WINAPI DisplayDlg (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
   HIGHCONTRAST hc;
   TCHAR szScheme[MAX_SCHEME_NAME_SIZE];
   BOOL fProcessed = TRUE;

   switch (uMsg) {
   case WM_INITDIALOG:
	  CheckDlgButton(hwnd, IDC_HC_ENABLE,
		 (g_hc.dwFlags & HCF_HIGHCONTRASTON) ? TRUE : FALSE);

	  if (!(g_hc.dwFlags & HCF_AVAILABLE)) {
		 EnableWindow(GetDlgItem(hwnd, IDC_HC_SETTINGS), FALSE);
		 EnableWindow(GetDlgItem(hwnd,IDC_HC_ENABLE), FALSE);
	  }
	  break;

   case WM_HELP:
	  WinHelp(((LPHELPINFO) lParam)->hItemHandle, __TEXT("access.hlp"), HELP_WM_HELP, (DWORD_PTR) (LPSTR) g_aIds);
	  break;

   case WM_CONTEXTMENU:
	  WinHelp((HWND) wParam, __TEXT("access.hlp"), HELP_CONTEXTMENU, (DWORD_PTR) (LPSTR) g_aIds);
	  break;

   case WM_COMMAND:
	  switch (GET_WM_COMMAND_ID(wParam, lParam)) {
	  case IDC_HC_ENABLE:
		 g_hc.dwFlags ^= HCF_HIGHCONTRASTON;
		 SendMessage(GetParent(hwnd), PSM_CHANGED, (WPARAM) hwnd, 0);
		 break;

	  case IDC_HC_SETTINGS:
          {
              INT_PTR RetValue;

              hc = g_hc;
              lstrcpy(szScheme, g_hc.lpszDefaultScheme);
              RetValue = DialogBox(g_hinst, MAKEINTRESOURCE(IDD_HIGHCONSETTINGS), hwnd, HighContrastDlg);
              
              if ( RetValue == IDCANCEL) 
              {
                  g_hc = hc;
                  lstrcpy(g_hc.lpszDefaultScheme, szScheme);
              } 
              else 
              {
                  SendMessage(GetParent(hwnd), PSM_CHANGED, (WPARAM) hwnd, 0);
              }
          }
          break;
	  }
	  break;

   case WM_NOTIFY:
	  switch (((NMHDR *)lParam)->code) {
	  case PSN_APPLY: SetAccessibilitySettings(); break;
	  }
	  break;

   default:
	  fProcessed = FALSE;
	  break;
   }

   return(fProcessed);
}

BOOL IsMUI_Enabled()
{

    OSVERSIONINFO verinfo;
    LANGID        rcLang;
    HMODULE       hModule;
    pfnGetUserDefaultUILanguage gpfnGetUserDefaultUILanguage;     
    pfnGetSystemDefaultUILanguage gpfnGetSystemDefaultUILanguage; 
    static        g_bPFNLoaded=FALSE;
    static        g_bMUIStatus=FALSE;


    if(g_bPFNLoaded)
       return g_bMUIStatus;

    g_bPFNLoaded = TRUE;

    verinfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);    
    GetVersionEx( &verinfo) ;

    if (verinfo.dwMajorVersion == 5)        
    {   

       hModule = GetModuleHandle(TEXT("kernel32.dll"));
       if (hModule)
       {
          gpfnGetSystemDefaultUILanguage =
          (pfnGetSystemDefaultUILanguage)GetProcAddress(hModule,"GetSystemDefaultUILanguage");
          if (gpfnGetSystemDefaultUILanguage)
          {
             rcLang = (LANGID) gpfnGetSystemDefaultUILanguage();
             if (rcLang == 0x409 )
             {  
                gpfnGetUserDefaultUILanguage =
                (pfnGetUserDefaultUILanguage)GetProcAddress(hModule,"GetUserDefaultUILanguage");
                
                if (gpfnGetUserDefaultUILanguage)
                {
                   if (rcLang != (LANGID)gpfnGetUserDefaultUILanguage() )
                   {
                       g_bMUIStatus = TRUE;
                   }

                }
             }
          }
       }
    }
    return g_bMUIStatus;
}



///////////////////////////////// End of File /////////////////////////////////
