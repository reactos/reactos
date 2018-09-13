//****************************************************************************
//
//  Module:     RNAUI.DLL
//  File:       rnawiz.c
//  Content:    This file contains the RNA wizard.
//  History:
//      Mon 31-Jan-1994 11:40:29  -by-  Viroon  Touranachun [viroont]
//
//  Copyright (c) Microsoft Corporation 1991-1994
//
//****************************************************************************

#include "rnaui.h"
#include "connent.h"
#include "rnawiz.h"
#include "subobj.h"

#define WM_CANCEL_WIZARD    WM_USER+10

#pragma data_seg(DATASEG_READONLY)
char const FAR c_szRnaKey[]    = REGSTR_KEY_RNA;
char const FAR c_szWizard[]    = REGSTR_VAL_WIZARD;

char const FAR c_szRnaNP[]       = "rnanp.dll";
char const FAR c_szInstall[]     = "RnaSubInstall";

char const FAR c_szDefaultMAC[]= "pppmac";

char const FAR c_szModemCPL[]  = "Control.exe modem.cpl,,add";

char const FAR c_szRasPhonebook[] = "rasphone.pbk";
char const FAR c_szRasPhoneSave[] = "rasphone.old";
char const FAR c_szRasPhoneNum[]  = "PhoneNumber";
char const FAR c_szRasUserName[]  = "User";
char const FAR c_szRasDomain[]    = "Domain";

//****************************************************************************
// An array indicating the first screen based on the previously displayed
//****************************************************************************

static FIRSTSCREEN const  aFirstScreens[] = {
                    {"RNAUI.DLL,RnaWizard /0", INTRO_FIRST_SCREEN,  IDI_REMOTE},
                    {"RNAUI.DLL,RnaWizard /1", CLIENT_FIRST_SCREEN, IDI_NEWREMOTE}};

#define INTRO_SCREEN    0           // order of the intro screen in the array
#define CLIENT_SCREEN   1           // order of the client screen in the array

extern ErrTbl const c_Rename[3];

static STARTUPINFO const     c_sti = {sizeof(STARTUPINFO),
                                    NULL,
                                    NULL,
                                    NULL,
                                    0, 0,
                                    0, 0,
                                    0, 0,
                                    0,
                                    0, 0,
                                    0, NULL,
                                    NULL, NULL, NULL};
#pragma data_seg()

#pragma data_seg("SHAREDATA")
static HWND     s_hWndOwner = NULL;
#pragma data_seg()

//****************************************************************************
// DWORD WINAPI Remote_CreateEntry()
//
// This function invokes the wizard sequence for connection creation
//
// History:
//  Mon 11-Jul-1994 17:38:57  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

DWORD WINAPI Remote_CreateEntry(HWND hwnd)
{
  DWORD dwRet;

  // Sos_AddRef();

  // Invoke wizard
  dwRet = RnaWizardSequence(hwnd, CLIENT_SCREEN);

  // Sos_Release();

  return dwRet;
}

/****************************************************************************
* @doc INTERNAL
*
* @func DWORD NEAR PASCAL | GetWizardSettings | This function reads the
*  wizard settings from registry.
*
* @rdesc Returns ERROR_SUCCESS if wizard setting is returned.
*
****************************************************************************/

DWORD NEAR PASCAL GetWizardSettings (HWND hwnd, LPDWORD lpdwSettings)
{
  HKEY   hkey;
  DWORD  dwType;
  DWORD  cb;
  DWORD  dwRet;

  // Assume failure
  dwRet = ERROR_BADKEY;

  if (RegOpenKey(HKEY_CURRENT_USER, c_szRnaKey, &hkey) == ERROR_SUCCESS)
  {
    // Get the wizard settings
    //
    cb = sizeof(*lpdwSettings);
    dwRet = RegQueryValueEx(hkey, c_szWizard, NULL, &dwType,
                            (LPBYTE)lpdwSettings, &cb);

    // Close the key
    //
    RegCloseKey(hkey);
  };

  // If cannot read from the registry, set to default
  //
  if (dwRet != ERROR_SUCCESS)
    *lpdwSettings = 0;

  return ERROR_SUCCESS;
}

/****************************************************************************
* @doc INTERNAL
*
* @func DWORD NEAR PASCAL | SetWizardSettings | This function saves the
*  wizard settings to registry.
*
* @rdesc Returns ERROR_SUCCESS if wizard setting is returned.
*
****************************************************************************/

DWORD NEAR PASCAL SetWizardSettings (HWND hwnd, DWORD dwSettings)
{
  HKEY   hkey;
  DWORD  dwRet;

  // Assume failure
  //
  dwRet = ERROR_BADKEY;

  if (RegCreateKey(HKEY_CURRENT_USER, c_szRnaKey, &hkey) == ERROR_SUCCESS)
  {
    // Save the wizard settings
    //
    dwRet = RegSetValueEx(hkey, c_szWizard, 0, REG_BINARY, (LPBYTE)&dwSettings,
                          sizeof(dwSettings));

    // Close the key
    //
    RegCloseKey(hkey);
  };

  return dwRet;
}

/****************************************************************************
* @doc INTERNAL
*
* @func DWORD NEAR PASCAL | CheckMacSetup | This function ensures that
*  RNA was set up properly before it runs.
*
* @rdesc Returns SUCCESS or ERROR_xxx.
*
****************************************************************************/

DWORD NEAR PASCAL CheckMACSetup(HWND hwnd, LPCSTR pMacList, UINT idErrMsg)
{
  DWORD       dwRet = SUCCESS;

  // If the mac list is not provided, use the default one
  //
  if (pMacList == NULL)
    pMacList = c_szDefaultMAC;

  // Make a call to the check setup
  //
  if ((dwRet = RnaFindDriver(hwnd, (LPSTR)pMacList)) == ERROR_BAD_DEVICE)
  {
    RuiUserMessage(hwnd, idErrMsg, MB_OK | MB_ICONINFORMATION);

    // Start installing the network driver
    //
    if (RnaInstallDriver(hwnd, (LPSTR)pMacList) == ERROR_MOD_NOT_FOUND)
    {
      // If the driver is not found, tell the user
      //
      RuiUserMessage(hwnd, IDS_WIZ_NO_INSTALL,
                     MB_OK | MB_ICONEXCLAMATION);
    };
  };
  return dwRet;
}
/****************************************************************************
* @doc INTERNAL
*
* @func DWORD NEAR PASCAL | CheckRnaSetup | This function ensures that
*  RNA was set up properly before it runs.
*
* @rdesc Returns SUCCESS or ERROR_xxx.
*
****************************************************************************/

DWORD NEAR PASCAL CheckRnaSetup (HWND hwnd, LPSTR szDeviceName, UINT idErrMsg)
{
  LPSTR       pMacList;
  DWORD       cbSize;
  DWORD       dwRet;

  // Get the mac list size
  cbSize = 0;
  if (((dwRet = RnaEnumerateMacNames(szDeviceName, NULL, &cbSize))
        != ERROR_BUFFER_TOO_SMALL) && (dwRet != SUCCESS))
    return dwRet;

  // Allocate mac list buffer
  if ((pMacList = (PBYTE)LocalAlloc(LMEM_FIXED, cbSize)) == NULL)
  {
    dwRet = ERROR_NOT_ENOUGH_MEMORY;
  }
  else
  {
    // Enumerate mac list from registry
    //
    if((dwRet = RnaEnumerateMacNames(szDeviceName, pMacList, &cbSize)) == SUCCESS)
    {
      // Check and install the appropriate MAC
      //
      dwRet = CheckMACSetup(hwnd, pMacList, idErrMsg);
    };
    LocalFree((HLOCAL)pMacList);
  };

  return dwRet;
}

/****************************************************************************
* @doc INTERNAL
*
* @func BOOL FAR PASCAL | RunWizard | This function determines whether
*  wizard should be activated. If so, it calls Shell to run it in a separate
*  thread.
*
* @rdesc Returns TRUE if wizard is activated and FALSE otherwise.
*
****************************************************************************/

BOOL FAR PASCAL RunWizard (HWND hwnd, DWORD dwType)
{
  DWORD  dwDisplayed;

  ASSERT(dwType != 0);

  // Get the display record
  // If any of the requested type was already displayed, do not run wizard
  // at all.
  //
  if (GetWizardSettings(hwnd, &dwDisplayed) != ERROR_SUCCESS)
    return FALSE;

  if ((dwType == INTRO_WIZ) && (dwDisplayed & NO_INTRO))
    return FALSE;

  dwDisplayed = ((dwType == INTRO_WIZ) ? INTRO_SCREEN : CLIENT_SCREEN);
  RunDLLProcess(aFirstScreens[dwDisplayed].szCmd);
  return TRUE;
}

/****************************************************************************
* @doc INTERNAL
*
* @func DWORD WINAPI | RnaWizard | This function is an entry point to display
*  the RNA wizard. It is executed in a separate thread.
*
* @rdesc Returns SUCCESS or ERROR_xxx.
*
****************************************************************************/

DWORD WINAPI RnaWizard (HWND hWnd,
                        HINSTANCE hAppInstance,
                        LPSTR lpszCmdLine,
                        int   nCmdShow)
{
  DWORD     uFirstPage;
  DWORD     dwRet;

  ENTEREXCLUSIVE()
  {
    // Find out If we have one running already
    //
    if (IsWindow(s_hWndOwner))
    {
      // If so, just activate it and quit
      //
      SetForegroundWindow(GetLastActivePopup(s_hWndOwner));
      dwRet = SUCCESS;
    }
    else
    {
      // Mark that we are running
      //
      s_hWndOwner  = hWnd;

      LEAVEEXCLUSIVE()
        {
        // Determine the first page
        //
        uFirstPage = (UINT)(lpszCmdLine[1] - '0');
        
        // Set the class icon
        //
        SetClassLong(hWnd, GCL_HICON,
                     (LONG)LoadIcon(ghInstance,
                                    MAKEINTRESOURCE(aFirstScreens[uFirstPage].idIcon)));
        
        // Display the wizard sequence
        //
        dwRet = RnaWizardSequence(hWnd, uFirstPage);
        }
      ENTEREXCLUSIVE()

      // Mark that we are done
      //
      s_hWndOwner = NULL;
    }
  }
  LEAVEEXCLUSIVE()

  return dwRet;
}

/****************************************************************************
* @doc INTERNAL
*
* @func DWORD NEAR PASCAL | RnaWizardSequence | This function sets up and
*  displays the requested wizard sequence.
*
* @rdesc Returns none.
*
****************************************************************************/

DWORD NEAR PASCAL RnaWizardSequence(HWND hWnd, UINT uFirstPage)
{
  LPWIZINFO lpwi;

  // Allocate the connection entry buffer
  //
  if ((lpwi = (LPWIZINFO)LocalAlloc(LPTR, sizeof(*lpwi))) == NULL)
    return ERROR_OUTOFMEMORY;

  // Initialize the structure
  //
  lpwi->uFirstPage              = aFirstScreens[uFirstPage].index;
  lpwi->fActivateRNA            = FALSE;

  // Start the wizard sequence
  //
  DoWizard(hWnd, uFirstPage, lpwi);

  // Clean up resources
  //
  DeinitClientWizard(hWnd, lpwi);

  // Free the connection entry buffer
  //
  LocalFree((HLOCAL)lpwi);

  return ERROR_SUCCESS;
}

/****************************************************************************
* @doc INTERNAL
*
* @func void NEAR PASCAL | AddPage | This function adds a specified page to
*  the property sheet.
*
* @rdesc Returns none.
*
****************************************************************************/

void NEAR PASCAL AddPage(LPPROPSHEETHEADER ppsh, UINT id, DLGPROC pfn,
                         LPWIZINFO lpwi)
{
  if (ppsh->nPages < MAX_WIZ_PAGES)
  {
    PROPSHEETPAGE psp;

    psp.dwSize      = sizeof(psp);
    psp.dwFlags     = PSP_DEFAULT;
    psp.hInstance   = ghInstance;
    psp.pszTemplate = MAKEINTRESOURCE(id);
    psp.pfnDlgProc  = pfn;
    psp.lParam      = (LPARAM)lpwi;

    ppsh->phpage[ppsh->nPages] = CreatePropertySheetPage(&psp);
    if (ppsh->phpage[ppsh->nPages])
      ppsh->nPages++;
  }
}  // AddPage

/****************************************************************************
* @doc INTERNAL
*
* @func void NEAR PASCAL | DoWizard | This function runs the wizard sequence.
*
* @rdesc Returns none.
*
****************************************************************************/

void NEAR PASCAL DoWizard(HWND hwnd, UINT nStartPage, LPWIZINFO lpwi)
{
  LPPROPSHEETHEADER ppsh;

  // Allocate the property sheet header
  //
  if ((ppsh = (LPPROPSHEETHEADER)LocalAlloc(LMEM_FIXED, sizeof(PROPSHEETHEADER)+
              (MAX_WIZ_PAGES * sizeof(HPROPSHEETPAGE)))) != NULL)
  {
    ppsh->dwSize     = sizeof(*ppsh);
    ppsh->dwFlags    = PSH_PROPTITLE | PSH_WIZARD;
    ppsh->hwndParent = hwnd;
    ppsh->hInstance  = ghInstance;
    ppsh->pszCaption = MAKEINTRESOURCE(IDS_CAP_REMOTE);
    ppsh->nPages     = 0;
    ppsh->nStartPage = nStartPage;
    ppsh->phpage     = (HPROPSHEETPAGE *)(ppsh+1);

    AddPage(ppsh, IDD_WIZ_INTRO,    IntroDlgProc,   lpwi);
    AddPage(ppsh, IDD_WIZ_CLIENT_1, Client1DlgProc, lpwi);
    AddPage(ppsh, IDD_WIZ_CLIENT_2, Client2DlgProc, lpwi);
    AddPage(ppsh, IDD_WIZ_CLIENT_3, Client3DlgProc, lpwi);

    PropertySheet(ppsh);

    LocalFree((HLOCAL)ppsh);
  }
}

/****************************************************************************
* @doc INTERNAL
*
* @func BOOL CALLBACK | IntroDlgProc | This function handles the introduction
*  screen.
*
* @rdesc Returns none.
*
****************************************************************************/

BOOL CALLBACK IntroDlgProc(HWND hDlg, UINT message , WPARAM wParam, LPARAM lParam)
{
  NMHDR FAR *lpnm;

  switch(message)
  {
    case WM_NOTIFY:
      lpnm = (NMHDR FAR *)lParam;
      switch(lpnm->code)
      {
	case PSN_SETACTIVE:
        {
	  PropSheet_SetWizButtons(GetParent(hDlg), PSWIZB_NEXT);
	  break;
	}

	default:
	  return FALSE;
      }
      break;

    default:
      return FALSE;

  } // end of switch on message
  return TRUE;

}

/****************************************************************************
* @doc INTERNAL
*
* @func DWORD NEAR PASCAL | ReplaceDeviceConfig | This function replaces the
*   device configuration for a null device with a real device
*
* @rdesc Returns none.
*
****************************************************************************/

DWORD NEAR PASCAL ReplaceDeviceConfig (PCONNENTRY pConnEntry)
{
  DWORD cbSize;
  DWORD cEntries;
  LPSTR pPortList, pNext;
  UINT  i;
  DWORD dwRet;

  // Get the buffer size
  cbSize = 0;
  if (RnaEnumDevices(NULL, &cbSize, &cEntries) != ERROR_BUFFER_TOO_SMALL)
    return ERROR_DEVICE_DOES_NOT_EXIST;

  // Allocate the buffer for the device list
  if ((pPortList = (LPSTR)LocalAlloc(LMEM_FIXED, (UINT)cbSize)) == NULL)
    return ERROR_OUTOFMEMORY;

  // Enumerate the device list
  cEntries = 0;
  if ((dwRet = RnaEnumDevices((LPBYTE)pPortList, &cbSize, &cEntries)) ==
      ERROR_SUCCESS)
  {
    // For each device in the list
    for (i = 0, pNext = pPortList; i < (UINT)cEntries; i++)
    {
      // Check the device type
      //
      if (IsValidDevice(pNext))
        break;
      pNext += (lstrlen(pNext)+1);
    };

    // Do we have a valid device?
    if (i < cEntries)
    {
      RnaFreeDevConfig(pConnEntry->pDevConfig);
      pConnEntry->pDevConfig = RnaGetDefaultDevConfig(pNext);
      dwRet = ERROR_SUCCESS;
    }
    else
      dwRet = ERROR_DEVICE_DOES_NOT_EXIST;
  };

  // Free the buffer
  LocalFree((HLOCAL)pPortList);

  return dwRet;
}

/****************************************************************************
* @doc INTERNAL
*
* @func DWORD CALLBACK | InitClientWizard | This function initializes the
*  client default information
*
* @rdesc Returns none.
*
****************************************************************************/

DWORD NEAR PASCAL InitClientWizard(HWND hwnd, LPWIZINFO lpwi)
{
  DWORD dwRet;

  // Activate the RNA engine, if not done yet
  //
  if (!lpwi->fActivateRNA)
  {
    if ((dwRet = RnaActivateEngine()) == ERROR_SUCCESS)
    {
      // Mark as activated
      //
      lpwi->fActivateRNA = TRUE;

      // Register device change notification
      //
      RnaEngineRequest(RA_REG_DEVCHG, (DWORD)hwnd);
    };
  };

  // If the engine was activated, get the default device
  //
  if (lpwi->fActivateRNA)
  {
    // Get the default entry information
    //
    if ((lpwi->ConnEntDlg.pConnEntry = RnaGetConnEntry(NULL, TRUE, TRUE)) != NULL)
    {
      dwRet = ERROR_SUCCESS;

      if (!lstrcmpi(lpwi->ConnEntDlg.pConnEntry->pDevConfig->di.szDeviceType,
                    DEVICE_NULL))
      {
        if ((dwRet = ReplaceDeviceConfig(lpwi->ConnEntDlg.pConnEntry)) !=
            ERROR_SUCCESS)
        {
          RnaFreeConnEntry(lpwi->ConnEntDlg.pConnEntry);
          lpwi->ConnEntDlg.pConnEntry = NULL;
        };
      };

      if (dwRet == ERROR_SUCCESS)
        lstrcpy(lpwi->szNewName, lpwi->ConnEntDlg.pConnEntry->pszEntry);
    }
    else
    {
      dwRet = ERROR_DEVICE_DOES_NOT_EXIST;
    };
  }
  else
  {
    // Make sure this guy is NULL
    //
    lpwi->ConnEntDlg.pConnEntry = NULL;
  };
  return dwRet;
}

/****************************************************************************
* @doc INTERNAL
*
* @func BOOL NEAR PASCAL | SaveClientEntry | This function attempts to
*  commit the changes in the client sequence.
*
* @rdesc Returns TRUE if OK
*
****************************************************************************/

BOOL NEAR PASCAL SaveClientEntry(HWND hDlg, PCONNENTRY pConnEntry)
{
  DWORD      dwRet ;

  // Save the address book entry
  //
  if ((dwRet = RnaSaveConnEntry(pConnEntry)) == SUCCESS)
  {
    PSUBOBJ pso;

    // Create a new subobject with no name
    //
    if (Subobj_New(&pso, pConnEntry->pszEntry, IDI_REMOTE, 0))
    {
      // Notify the event
      //
      Remote_GenerateEvent(SHCNE_CREATE, pso, NULL);
      Subobj_Destroy(pso);
    }

    return ERROR_SUCCESS;
  };

  ETMsgBox(hDlg, IDS_CAP_REMOTE, dwRet, c_Rename, ARRAYSIZE(c_Rename));

  return dwRet;
}

/****************************************************************************
* @doc INTERNAL
*
* @func BOOL NEAR PASCAL | DeinitClientWizard | This function initializes the
*  client default information
*
* @rdesc Returns none.
*
****************************************************************************/

BOOL NEAR PASCAL DeinitClientWizard(HWND hwnd, LPWIZINFO lpwi)
{
  // If the client info was allocated, free it
  //
  if (lpwi->ConnEntDlg.pConnEntry != NULL)
  {
    RnaFreeConnEntry(lpwi->ConnEntDlg.pConnEntry);
  };

  // If the RNA engine was activated, deactivate it
  //
  if (lpwi->fActivateRNA)
  {
    RnaDeactivateEngine();
  };

  return TRUE;
}

/****************************************************************************
* @doc INTERNAL
*
* @func BOOL CALLBACK | Client1DlgProc | This function handles the connectoid
*  name and device prompt.
*
* @rdesc Returns none.
*
****************************************************************************/

BOOL CALLBACK Client1DlgProc(HWND hDlg, UINT message , WPARAM wParam, LPARAM lParam)
{
  LPWIZINFO lpwi;
  NMHDR FAR *lpnm;

  switch(message)
  {
    case WM_NOTIFY:
      lpnm = (NMHDR FAR *)lParam;
      switch(lpnm->code)
      {
	case PSN_SETACTIVE:

          // Adjust the appearence of the page
          //
          AdjustClient1Dlg(hDlg);
	  break;

        case PSN_WIZNEXT:
        {
          // Attempt to commit the info on this page
          //
          if (!CommitClient1Dlg(hDlg))
          {
            // Cannot commit the information, stay in this page
            //
            SetDlgMsgResult(hDlg, message, -1);
          }
          else
          {
            LPWIZINFO lpwi = (LPWIZINFO)GetWindowLong(hDlg, DWL_USER);

            // If it does not require the phone number, skip the phone page
            //
            if (!lstrcmpi(lpwi->ConnEntDlg.pDevConfig->di.szDeviceType, DEVICE_NULL))
            {
              SetDlgMsgResult(hDlg, message, IDD_WIZ_CLIENT_3);
            };
          };
          break;
        }

	default:
	  return FALSE;
      }
      break;

    case WM_INITDIALOG:
    {
      DWORD dwRet;

      lpwi = (LPWIZINFO)(((LPPROPSHEETPAGE)lParam)->lParam);
      SetWindowLong(hDlg, DWL_USER, (LPARAM)lpwi);

      // Initialize the entire client sequence
      //
      if ((dwRet = InitClientWizard(hDlg, lpwi)) == ERROR_SUCCESS)
      {
        // Initialize the information for this page
        //
        InitNameAndDevice(hDlg, lpwi->ConnEntDlg.pConnEntry);
        AdjustDevice(hDlg, &lpwi->ConnEntDlg);

        return FALSE;
      }
      else
      {
        // If no device, kick off the modem installation
        //
        if (dwRet == ERROR_DEVICE_DOES_NOT_EXIST)
        {
          InstallDevice(hDlg);
        }
        else
        {
          if (dwRet == ERROR_STATE_MACHINES_NOT_STARTED)
          {
            MsgBoxIds(hDlg, IDS_ERR_BAD_INSTALL, IDS_CAP_REMOTE, MSG_ERROR);
            PostMessage(hDlg, WM_CANCEL_WIZARD, 0, 0);
          };
        };
      };
      break;
    }
    case WM_COMMAND:
    {
      LPWIZINFO lpwi = (LPWIZINFO)GetWindowLong(hDlg, DWL_USER);

      if ((GET_WM_COMMAND_ID(wParam, lParam)) == IDC_WC_INST)
      {
        InstallDevice(hDlg);
        break;
      }
      else
      {
        // Handle user's interaction
        //
        return (ConnEntryHandler(hDlg, &lpwi->ConnEntDlg, wParam, lParam));
      };
    }

    case WM_DESTROY:
    {
      LPWIZINFO lpwi = (LPWIZINFO)GetWindowLong(hDlg, DWL_USER);

      // Deallocate the resource allocated for this page
      //
      if (lpwi->fActivateRNA)
      {
        RnaEngineRequest(RA_DEREG_DEVCHG, (DWORD)hDlg);
      };
      DeInitDeviceList (GetDlgItem(hDlg, IDC_AB_DEVICE));
      break;
    }

    case WM_RASDIALEVENT:

      switch (lParam)
      {
        case RNA_ADD_DEVICE:
        case RNA_DEL_DEVICE:
        {
          LPWIZINFO lpwi = (LPWIZINFO)GetWindowLong(hDlg, DWL_USER);

          // A device is added or removed, adjust the device list
          //
          AdjustDeviceList(hDlg, &lpwi->ConnEntDlg, lParam);

          // If we do not have a device, get one.
          //
          if ((lParam == RNA_ADD_DEVICE) &&
              (lpwi->ConnEntDlg.pConnEntry == NULL))
          {
            if (InitClientWizard(hDlg, lpwi) == ERROR_SUCCESS)
            {
              HWND    hCtrl;

              // Initialize the entry name
              //
              hCtrl = GetDlgItem(hDlg, IDC_AB_ENTRY);
              Edit_LimitText(hCtrl, RAS_MaxEntryName);
              SetWindowText(hCtrl, lpwi->ConnEntDlg.pConnEntry->pszEntry);
              SetFocus(hCtrl);
              Edit_SetSel(hCtrl, 0, -1);
            };
          };

          // Adjust the page appearence when the device list is chaged
          //
          if (IsWindowVisible(hDlg))
            AdjustClient1Dlg(hDlg);

          break;
        }
        case RNA_SHUTDOWN:
          PropSheet_PressButton (GetParent(hDlg), PSBTN_CANCEL);
          break;
      };
      break;

    case WM_CANCEL_WIZARD:
      PropSheet_PressButton (GetParent(hDlg), PSBTN_CANCEL);
      break;

    default:
      return FALSE;

  } // end of switch on message
  return TRUE;

}

/****************************************************************************
* @doc INTERNAL
*
* @func void NEAR PASCAL | AdjustClient1Dlg | This function adjusts the
*  client page based on the number of devices.
*
* @rdesc Returns none
*
****************************************************************************/

void NEAR PASCAL AdjustClient1Dlg(HWND hDlg)
{
  LPWIZINFO lpwi;
  HWND      hCtrl;
  DWORD     dwButtons;
  BOOL      fEnable;

  lpwi = (LPWIZINFO)GetWindowLong(hDlg, DWL_USER);

  // Check if we have any device
  //
  hCtrl = GetDlgItem(hDlg, IDC_AB_DEVICE);
  if (fEnable = (ComboBox_GetCount(hCtrl) > 0))
  {
    // If this is not the first page, enable go back
    //
    dwButtons = ((lpwi->uFirstPage != CLIENT_FIRST_SCREEN) ?
                 (PSWIZB_NEXT | PSWIZB_BACK) : PSWIZB_NEXT);
  }
  else
  {
    // Disable all the buttons
    //
    dwButtons = 0;
  };

  // Enable the buttons
  //
  PropSheet_SetWizButtons(GetParent(hDlg), dwButtons);

  // Disable all controls
  //
  EnableWindow(hCtrl, fEnable);
  EnableWindow(GetDlgItem(hDlg, IDC_AB_ENTRY), fEnable);
  EnableWindow(GetDlgItem(hDlg, IDC_AB_DEVICESET), fEnable);

  // except for the install modem if no device
  //
  hCtrl = GetDlgItem(hDlg, IDC_WC_INST);

  // If the button become invisible, set focus to the default control
  //
  if (IsWindowVisible(hCtrl) && fEnable)
  {
    hCtrl = GetDlgItem(hDlg, IDC_AB_ENTRY);
    SetFocus(hCtrl);
    Edit_SetSel(hCtrl, 0, -1);
  };

  ShowWindow(GetDlgItem(hDlg, IDC_WC_INST), fEnable ? SW_HIDE : SW_SHOW);
  return;
}

/****************************************************************************
* @doc INTERNAL
*
* @func BOOL NEAR PASCAL | CommitClient1Dlg | This function attempts to
*  ccommit the changes in the first client page.
*
* @rdesc Returns TRUE if OK
*
****************************************************************************/

BOOL NEAR PASCAL CommitClient1Dlg(HWND hDlg)
{
  LPWIZINFO lpwi;
  HWND      hCtrl;
  DWORD     dwRet;

  // Get the client info
  //
  lpwi = (LPWIZINFO)GetWindowLong(hDlg, DWL_USER);

  // Do we have any information?
  //
  if (lpwi->ConnEntDlg.pConnEntry == NULL)
  {
    RuiUserMessage(hDlg, IDS_WIZ_INST_MODEM, MB_OK | MB_ICONINFORMATION);
    return FALSE;
  };

  // Get the user-defined name
  //
  hCtrl = GetDlgItem(hDlg, IDC_AB_ENTRY);
  GetWindowText(hCtrl, lpwi->szNewName, sizeof(lpwi->szNewName));

  // Check for reserved name
  //
  if (lstrcmpi(lpwi->szNewName, c_szDirect))
  {
    // Check whether the name exists
    //
    if ((dwRet = RnaValidateEntryName(lpwi->szNewName, TRUE)) == ERROR_SUCCESS)
    {
      // Commit the changes in this page
      //
      GetDeviceConfig(&lpwi->ConnEntDlg);
      return TRUE;
    }
    else
    {
      // Set focus to the name
      //
      ETMsgBox(hDlg, IDS_CAP_REMOTE, dwRet, c_Rename, ARRAYSIZE(c_Rename));
    };
  }
  else
  {
    // Cannot use the reserved name
    //
    RuiUserMessage(hDlg, IDS_ERR_RESERVE_NAME, MB_OK | MB_ICONEXCLAMATION);
  };

  SetFocus(hCtrl);
  Edit_SetSel(hCtrl, 0, -1);
  return FALSE;
}

/****************************************************************************
* @doc INTERNAL
*
* @func BOOL CALLBACK | Client2DlgProc | This function handles the connectoid
*  phone number prompt.
*
* @rdesc Returns none.
*
****************************************************************************/

BOOL CALLBACK Client2DlgProc(HWND hDlg, UINT message , WPARAM wParam, LPARAM lParam)
{
  NMHDR FAR *lpnm;

  switch(message)
  {
    case WM_NOTIFY:
      lpnm = (NMHDR FAR *)lParam;
      switch(lpnm->code)
      {
	case PSN_SETACTIVE:
	  PropSheet_SetWizButtons(GetParent(hDlg), PSWIZB_NEXT | PSWIZB_BACK);
	  break;

        case PSN_WIZNEXT:
        {
          // Attempt to commit the info on this page
          //
          if (!CommitClient2Dlg(hDlg))
          {
            // Cannot commit the information, stay in this page
            //
            SetDlgMsgResult(hDlg, message, -1);
          };
          break;
        }

	default:
	  return FALSE;
      }
      break;

    case WM_INITDIALOG:
    {
      LPWIZINFO lpwi;
      DWORD     cEntry, dwRet;

      lpwi = (LPWIZINFO)(((LPPROPSHEETPAGE)lParam)->lParam);
      SetWindowLong(hDlg, DWL_USER, (LPARAM)lpwi);

      // Get the most recent used area code
      //
      cEntry = 1;
      if (((dwRet = RnaGetAreaCodeList(lpwi->ConnEntDlg.pConnEntry->pn.szAreaCode,
                                      &cEntry)) != ERROR_SUCCESS) &&
           (dwRet != ERROR_BUFFER_TOO_SMALL))
      {
        // Cannot get the area code, assume blank
        //
        *lpwi->ConnEntDlg.pConnEntry->pn.szAreaCode = '\0';
      };

      // Initialize the default phone number
      //
      InitPhoneNumber (hDlg, lpwi->ConnEntDlg.pConnEntry);

      // Set focus to the name entry
      //
      SetFocus(GetDlgItem(hDlg, IDC_AB_PHONE));
      return FALSE;
    }

    case WM_COMMAND:
    {
      LPWIZINFO lpwi = (LPWIZINFO)GetWindowLong(hDlg, DWL_USER);

      return (ConnEntryHandler(hDlg, &lpwi->ConnEntDlg, wParam, lParam));
    }

    case WM_DESTROY:

      // Deallocate the resource allocated for this page
      //
      DeInitCountryCodeList (GetDlgItem(hDlg, IDC_AB_COUNTRY));
      break;

    default:
      return FALSE;

  } // end of switch on message
  return TRUE;

}

/****************************************************************************
* @doc INTERNAL
*
* @func BOOL NEAR PASCAL | CommitClient2Dlg | This function attempts to
*  ccommit the changes in the second client page.
*
* @rdesc Returns TRUE if OK
*
****************************************************************************/

BOOL NEAR PASCAL CommitClient2Dlg(HWND hDlg)
{
  LPWIZINFO lpwi;

  // Get the client info
  //
  lpwi = (LPWIZINFO)GetWindowLong(hDlg, DWL_USER);

  // Check the valid phone number
  //
  if (!IsInvalidConnEntry(hDlg))
  {
    GetPhoneNumber(hDlg, &lpwi->ConnEntDlg.pConnEntry->pn);
    return TRUE;
  };
  return FALSE;
}

/****************************************************************************
* @doc INTERNAL
*
* @func BOOL CALLBACK | Client3DlgProc | This function handles the connectoid
*  phone number prompt.
*
* @rdesc Returns none.
*
****************************************************************************/

BOOL CALLBACK Client3DlgProc(HWND hDlg, UINT message , WPARAM wParam, LPARAM lParam)
{
  NMHDR FAR *lpnm;

  switch(message)
  {
    case WM_NOTIFY:
      lpnm = (NMHDR FAR *)lParam;
      switch(lpnm->code)
      {
	case PSN_SETACTIVE:
        {
          LPWIZINFO lpwi = (LPWIZINFO)GetWindowLong(hDlg, DWL_USER);

          SetWindowText(GetDlgItem(hDlg, IDC_AB_ENTRY), lpwi->szNewName);
	  PropSheet_SetWizButtons(GetParent(hDlg), PSWIZB_FINISH | PSWIZB_BACK);
	  break;
        }
        case PSN_WIZFINISH:
        {
          LPWIZINFO lpwi = (LPWIZINFO)GetWindowLong(hDlg, DWL_USER);

          // Attempt to commit the info on this page
          //
          lpwi->ConnEntDlg.pConnEntry->pszEntry = lpwi->szNewName;

          if (SaveClientEntry(hDlg, lpwi->ConnEntDlg.pConnEntry)
              != ERROR_SUCCESS)
          {
            // Cannot commit the information, stay in this page
            //
            SetDlgMsgResult(hDlg, message, -1);
          }
          else
          {
            // Want to import the RAS phonebook
            //
            ImportRasPhonebook (hDlg, c_szRasPhonebook,
                                lpwi->ConnEntDlg.pConnEntry);

            // Complete final RNA setting
            //
            FinalRnaClientSetup (hDlg, lpwi);

            // No longer show intro wizard
            //
            SetWizardSettings(hDlg, NO_INTRO);
          };
          break;
        }

        case PSN_WIZBACK:
        {
          LPWIZINFO lpwi = (LPWIZINFO)GetWindowLong(hDlg, DWL_USER);

          // If it does not require the phone number, skip the phone page
          //
          if (!lstrcmpi(lpwi->ConnEntDlg.pDevConfig->di.szDeviceType, DEVICE_NULL))
          {
            SetDlgMsgResult(hDlg, message, IDD_WIZ_CLIENT_1);
          };
          break;
        }

	default:
	  return FALSE;
      }
      break;

    case WM_INITDIALOG:
    {
      LPWIZINFO lpwi;

      lpwi = (LPWIZINFO)(((LPPROPSHEETPAGE)lParam)->lParam);
      SetWindowLong(hDlg, DWL_USER, (LPARAM)lpwi);
      break;
    }

    default:
      return FALSE;

  } // end of switch on message
  return TRUE;

}

/****************************************************************************
* @doc INTERNAL
*
* @func BOOL NEAR PASCAL | InstallDevice | This function starts the device
*  installation process.
*
* @rdesc Returns none.
*
****************************************************************************/

BOOL NEAR PASCAL InstallDevice(HWND hWnd)
{
  PROCESS_INFORMATION pi;
  BOOL bRet;

  // Start the modem installation process
  //
  if (bRet = CreateProcess(NULL, (LPSTR)c_szModemCPL,
                           NULL, NULL, FALSE, 0, NULL, NULL,
                           (LPSTARTUPINFO)&c_sti, &pi))
  {
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
  };
  return bRet;
}

/****************************************************************************
* @doc INTERNAL
*
* @func DWORD NEAR PASCAL | ImportRasPhonebook | This function imports the
*  WfW Ras phonebook.
*
* @rdesc Returns ERROR_SUCCESS or an error code
*
****************************************************************************/

DWORD NEAR PASCAL ImportRasPhonebook (HWND hDlg, LPCSTR szPhonebook,
                                      PCONNENTRY pConnEntry)
{
  LPSTR           lpszBuffer, lpszEntry;
  LPRASDIALPARAMS lprasdialparams;
  OFSTRUCT        of;

  // Search for the Ras phonebook
  if (OpenFile(szPhonebook, &of, OF_EXIST) == HFILE_ERROR)
    return ERROR_FILE_NOT_FOUND;

  // Get the list of the phonebook entries
  if ((lpszBuffer = (LPSTR)LocalAlloc(LMEM_FIXED,
                                      BUFFER_SIZE+sizeof(RASDIALPARAMS))) == NULL)
    return ERROR_OUTOFMEMORY;
  lprasdialparams = (LPRASDIALPARAMS)(lpszBuffer+BUFFER_SIZE);

  if (GetPrivateProfileString(NULL, NULL, NULL, lpszBuffer, BUFFER_SIZE,
                              of.szPathName))
  {
    HCURSOR         hCursor;

    SetCapture (hDlg);
    hCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));

    // walk the section buffer looking for phonenumbers
    lpszEntry                    = lpszBuffer;

    lmemzero(lprasdialparams, sizeof(RASDIALPARAMS));
    lprasdialparams->dwSize      = sizeof(RASDIALPARAMS);

    pConnEntry->pn.dwCountryID   = 0;
    pConnEntry->pn.dwCountryCode = 0;
    pConnEntry->pszEntry         = lprasdialparams->szEntryName;

    while (*lpszEntry)
    {
      if (GetPrivateProfileString(lpszEntry, c_szRasPhoneNum, c_szNull,
                                  pConnEntry->pn.szLocal,
                                  sizeof(pConnEntry->pn.szLocal),
                                  of.szPathName))
      {
        int i = DUP_SUFFIX_START;
        DWORD dwRet;

        // found one...save it with a non-duplicate entry name
        //
        lstrcpy(pConnEntry->pszEntry, lpszEntry);

        // Find a non-duplicated name
        //
        while (((dwRet = RnaValidateEntryName(pConnEntry->pszEntry, TRUE))
                 != ERROR_SUCCESS) && (i <= DUP_SUFFIX_MAX))
        {
          wsprintf(pConnEntry->pszEntry, "%s %d", lpszEntry, i);
          i++;
        };

        // Save with a non-duplicated name
        //
        if (dwRet == ERROR_SUCCESS)
        {
          if (SaveClientEntry(hDlg, pConnEntry) == ERROR_SUCCESS)
          {
            GetPrivateProfileString(lpszEntry, c_szRasUserName, c_szNull,
                                    lprasdialparams->szUserName,
                                    sizeof(lprasdialparams->szUserName),
                                    of.szPathName);
            GetPrivateProfileString(lpszEntry, c_szRasDomain, c_szNull,
                                    lprasdialparams->szDomain,
                                    sizeof(lprasdialparams->szDomain),
                                    of.szPathName);

            // We can cache other dial-up information here
            RasSetEntryDialParams(NULL, lprasdialparams, TRUE);
          };
        };
      };

      // next entry
      lpszEntry += lstrlen(lpszEntry)+1;
    };

    SetCursor(hCursor);
    ReleaseCapture();
  };

  // Rename the phonebook file
  lstrcpy(lpszBuffer, of.szPathName);
  lpszEntry = lpszBuffer+lstrlen(lpszBuffer);
  ASSERT (*lpszEntry != '\\');
  while (lpszEntry != lpszBuffer)
  {
    LPSTR   lpszNext;

    lpszNext = CharPrev(lpszBuffer, lpszEntry);
    if (*lpszNext == '\\')
      break;
    lpszEntry = lpszNext;
  };
  lstrcpy(lpszEntry, c_szRasPhoneSave);
  CopyFile(of.szPathName, lpszBuffer, FALSE);
  DeleteFile(of.szPathName);

  LocalFree((HLOCAL)lpszBuffer);
  return ERROR_SUCCESS;
}

/****************************************************************************
* @doc INTERNAL
*
* @func DWORD NEAR PASCAL | FinalRnaClientSetup | This function completes the
*  client RNA setup.
*
* @rdesc Returns ERROR_SUCCESS always
*
****************************************************************************/

DWORD NEAR PASCAL FinalRnaClientSetup (HWND hDlg,  LPWIZINFO lpwi)
{
  HANDLE  hRnaSub;
  PCONNENTRY pConnEntry;

  // We have at least one connection. Let's install implicit connection.
  //
  // Restore RNANP for the network notification hook
  if ((hRnaSub = LoadLibrary(c_szRnaNP)) != NULL)
  {
    FARPROC pfnInstall;

    if ((pfnInstall = GetProcAddress(hRnaSub, c_szInstall)) != NULL)
    {
      (*pfnInstall)();
    };

    FreeLibrary(hRnaSub);
  }
  else
    ASSERT(0);

  // We need to check the network driver installation first.
  //
  pConnEntry = lpwi->ConnEntDlg.pConnEntry;
  CheckRnaSetup(hDlg, pConnEntry->pDevConfig->di.szDeviceName,
                IDS_WIZ_NOCONN_NODRV);

  return ERROR_SUCCESS;
}

