//****************************************************************************
//
//  Module:     RNAUI.DLL
//  File:       connent.c
//  Content:    This file contain all the functions that handle the connection
//              entry dialog boxes.
//  History:
//      Wed 17-Mar-1993 07:45:27  -by-  Viroon  Touranachun [viroont]
//
//  Copyright (c) Microsoft Corporation 1991-1994
//
//****************************************************************************

#include "rnaui.h"
#include "connent.h"
#include "scripter.h"
#include "mlink.h"
#include "rnahelp.h"

#pragma data_seg(DATASEG_READONLY)

ErrTbl const c_rgetConfigDlg[] = {
        { ERROR_CANNOT_SET_PORT_INFO, IDS_ERR_BAD_PORT },
        { ERROR_WRONG_INFO_SPECIFIED, IDS_ERR_UNKNOWN_FORMAT },
        { ERROR_INVALID_PORT_HANDLE,  IDS_ERR_BAD_PORT },
        { ERROR_PORT_NOT_FOUND,       IDS_ERR_DEVICE_NOT_FOUND },
        { ERROR_PORT_NOT_AVAILABLE,   IDS_ERR_DEVICE_INUSE },
        };

#pragma data_seg()

//****************************************************************************
// DWORD WINAPI Remote_EditEntry()
//
// This function displays the connection property from API
//
// History:
//  Mon 11-Jul-1994 17:38:57  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

DWORD WINAPI Remote_EditEntry(HWND hwnd, LPSTR szEntry)
{
  DWORD dwRet;

  // Invoke property sheet
  //
  dwRet = (Remote_PropertySheet(szEntry, hwnd) ?
           ERROR_SUCCESS : ERROR_CANNOT_FIND_PHONEBOOK_ENTRY);

  return dwRet;
}

//****************************************************************************
// void NEAR PASCAL ComposeDisplayPhone(LPSTR, LPPHONENUM)
//
// This function composes a displayable phone number to the string
//
// History:
//  Wed 16-Mar-1994 09:51:33  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

void NEAR PASCAL ComposeDisplayPhone(LPSTR lpszBuf, LPPHONENUM lpPhoneNum,
                                     DWORD cb)
{
  static BOOL s_fPhoneFmt = FALSE;
  static char s_szPhoneFmt[RAS_MaxPhoneNumber+1];
  static char s_szNAPhoneFmt[RAS_MaxPhoneNumber+1];

  // Get the displayable phone format
  if (!s_fPhoneFmt)
  {
    LoadString(ghInstance, IDS_DISP_PHONE_FMT, s_szPhoneFmt,
               sizeof(s_szPhoneFmt));
    LoadString(ghInstance, IDS_DISP_NA_PHONE_FMT, s_szNAPhoneFmt,
               sizeof(s_szNAPhoneFmt));
    s_fPhoneFmt = TRUE;
  };

  if ((lpPhoneNum->dwCountryCode != 0) &&
      (*(lpPhoneNum->szLocal) != '\0'))
  {
    LPSTR szFmt;
    DWORD adwArg[3];

    adwArg[0] = lpPhoneNum->dwCountryCode;
    adwArg[1] = (DWORD)lpPhoneNum->szAreaCode;
    adwArg[2] = (DWORD)lpPhoneNum->szLocal;

    if (*(lpPhoneNum->szAreaCode) != '\0')
    {
      szFmt = s_szPhoneFmt;
    }
    else
    {
      szFmt = s_szNAPhoneFmt;
    };

    FormatMessageA(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ARGUMENT_ARRAY,
                   (LPCVOID)szFmt, 0, 0,
                   lpszBuf, cb, (va_list*) adwArg);
  }
  else
  {

    // If this is a dial as is connection, use the local phone only
    //
    if (lpPhoneNum->dwCountryID == 0)
    {
      lstrcpy(lpszBuf, lpPhoneNum->szLocal);
    }
    else
    {
      *lpszBuf = '\0';
    };
  };
  return;
}

//****************************************************************************
// BOOL NEAR PASCAL IsValidDeviceType (LPSTR)
//
// This generic function validates the device type
//
// History:
//  Tue 19-Jul-1994 16:55:36  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

BOOL NEAR PASCAL IsValidDevice(LPSTR szDeviceName)
{
  DEVICEINFO    di;

  // Get the device type
  //
  di.dwVersion = sizeof(DEVICEINFO);
  RnaGetDeviceInfo(szDeviceName, &di);

  // We no longer support a null device
  //
  return (lstrcmpi(di.szDeviceType, DEVICE_NULL) != 0);
}

//****************************************************************************
// DWORD NEAR PASCAL GetChannelCount ()
//
// This function gets the number of available channels.
//
// History:
//  Thu 21-Mar-1996 13:32:34  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

DWORD NEAR PASCAL GetChannelCount ()
{
  DWORD cChannel = 0;
  DWORD cbSize;
  DWORD cEntries;
  LPSTR pPortList, pNext;
  UINT  i;

  // Get the buffer size
  cbSize = 0;
  if (RnaEnumDevices(NULL, &cbSize, &cEntries) != ERROR_BUFFER_TOO_SMALL)
    return cChannel;

  // Allocate the buffer for the device list
  if ((pPortList = (LPSTR)LocalAlloc(LMEM_FIXED, (UINT)cbSize)) != NULL)
  {
    // Enumerate the device list
    cEntries = 0;
    if (RnaEnumDevices((LPBYTE)pPortList, &cbSize, &cEntries) == SUCCESS)
    {
      // For each device in the list
      for (i = 0, pNext = pPortList; i < (UINT)cEntries; i++)
      {
        // Check the device type
        //
        if (IsValidDevice(pNext))
        {
          cChannel += RnaGetDeviceChannel(pNext);
        };
        pNext += (lstrlen(pNext)+1);
      };
    };

    // Free the buffer
    LocalFree((HLOCAL)pPortList);
  };

  return cChannel;

}

//****************************************************************************
// BOOL FAR PASCAL Remote_PropertySheet (LPSTR, HWND)
//
// This function is called when the Call Entry object is selected for the
// "Properties" item from its context menu.
//
// History:
//  Wed 07-Apr-1993 11:58:05  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

BOOL FAR PASCAL Remote_PropertySheet (LPSTR szObjPath, HWND hWnd)
{
    PROPSHEETHEADER psh;
    PROPSHEETPAGE  *pPropPage;
    HPROPSHEETPAGE rgPages[MAX_CONNENT_PAGES];
    PCONNENTRY     pConnEntry;
    CONNENTDLG     ConnEntDlg;
    BOOL           bRet = FALSE;

    // Load the RNA engine
    if (RnaActivateEngine() != SUCCESS)
    {
      MsgBoxIds(hWnd, IDS_ERR_BAD_INSTALL, IDS_CAP_REMOTE, MSG_ERROR);
      return FALSE;
    };

    // Prepare a property sheet
    //
    psh.dwSize     = sizeof(psh);
    psh.dwFlags    = PSP_DEFAULT | PSH_NOAPPLYNOW;
    psh.hwndParent = hWnd;
    psh.hInstance  = ghInstance;
    psh.pszCaption = szObjPath;
    psh.nPages     = 0;
    psh.nStartPage = 0;
    psh.phpage     = rgPages;

    // Open the modem file and read its content
    if ((pConnEntry = RnaGetConnEntry(szObjPath, TRUE, TRUE)) != NULL)
    {
      // We have the connection information
      ConnEntDlg.pConnEntry  = pConnEntry;
      ConnEntDlg.pDevConfig  = NULL;
      ConnEntDlg.pSMMList    = NULL;
      ConnEntDlg.pSMMType    = NULL;

      // Allocate the buffer for current server type settings
      if (BuildSMMList(&ConnEntDlg, pConnEntry->pDevConfig) == ERROR_SUCCESS)
      {
        // Initialize the SMM settings from the connection entry
        //
        ASSERT(ConnEntDlg.pSMMType != NULL);
        ConnEntDlg.pSMMType->smmi = pConnEntry->pDevConfig->smmi;
        ConnEntDlg.pSMMType->ipData.dwSize = sizeof(ConnEntDlg.pSMMType->ipData);
        RnaGetIPInfo(pConnEntry->pszEntry, &ConnEntDlg.pSMMType->ipData, FALSE);

        // Build multilink list
        //
        BuildSubConnList(&ConnEntDlg);
        ConnEntDlg.cmlChannel = GetChannelCount();
        if (ConnEntDlg.cmlChannel != 0)
          ConnEntDlg.cmlChannel--;

        // Allocate PROPSHEETINFO structure and initialize its content
        if ((pPropPage = CallEntry_GetPropSheet (&psh, &ConnEntDlg)) != NULL)
        {
          // Display property sheet
          //
          PropertySheet(&psh);
          CallEntry_ReleasePropSheet(pPropPage);
          bRet = TRUE;
        };

        // Clean up multilink list
        //
        FreeSubConnList(&ConnEntDlg);

        // Clean up the server type settings buffers
        FreeSMMList(&ConnEntDlg);
      };

      RnaFreeConnEntry(pConnEntry);
    };

    RnaDeactivateEngine();
    return bRet;
}

//****************************************************************************
// PROPSHEETINFO* NEAR PASCAL CallEntry_GetPropSheet (PROPSHEETHEADER,
//                                                    PCONNENTDLG)
//
// This function allocates the PROPSHEET staructure and initializes it.
//
// History:
//  Tue 23-Feb-1993 12:05:02  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

PROPSHEETPAGE* NEAR PASCAL CallEntry_GetPropSheet (PROPSHEETHEADER *psh,
                                                   PCONNENTDLG  pConnEntDlg)
{
  PROPSHEETPAGE *psp, *pPropPage;
  HPROPSHEETPAGE hpage;
  RNAPROPPAGE   RnaPsp;

  // Allocate the PROPSHEETPAGE structure
  if ((pPropPage = (PROPSHEETPAGE *)LocalAlloc(LMEM_FIXED,
                   MAX_CONNENT_PAGES*sizeof(PROPSHEETPAGE))) != NULL)
  {
    psp = pPropPage;

    // Initialize the general page
    psp->dwSize      = sizeof(*psp);
    psp->dwFlags     = PSP_DEFAULT;
    psp->hInstance   = ghInstance;

#ifdef MULTILINK_ENABLED
#ifdef MULTILINK_PROP_PAGE
    psp->pszTemplate = MAKEINTRESOURCE(IDD_ABENTRY);
#else
    psp->pszTemplate = MAKEINTRESOURCE(pConnEntDlg->cmlChannel ?
                                       IDD_ABMLENTRY : IDD_ABENTRY);
#endif
#else
    psp->pszTemplate = MAKEINTRESOURCE(IDD_ABENTRY);
#endif

    psp->pfnDlgProc  = ConnEntryDlgProc;
    psp->pszTitle    = NULL;
    psp->lParam      = (LPARAM)pConnEntDlg;
    psp->pfnCallback = NULL;

    // Create the property sheet for the modem object
    hpage = CreatePropertySheetPage(psp);
    psh->phpage[psh->nPages++] = hpage;
    psp++;

    // Get other pages module
    RnaPsp.idPage = SRV_TYPE_PAGE;

    if (RnaEngineRequest(RA_GET_PROP, (DWORD)&RnaPsp))
    {
      // Now the server type page
      psp->dwSize      = sizeof(*psp);
      psp->dwFlags     = PSP_DEFAULT;
      psp->hInstance   = RnaPsp.hModule;
      psp->pszTemplate = MAKEINTRESOURCE(RnaPsp.idRes);
      psp->pfnDlgProc  = RnaPsp.pfn;
      psp->pszTitle    = NULL;
      psp->lParam      = (LPARAM)pConnEntDlg;
      psp->pfnCallback = NULL;

      // Create the property sheet for the modem object
      hpage = CreatePropertySheetPage(psp);
      psh->phpage[psh->nPages++] = hpage;
      psp++;
    };

    // Initialize the script page
    psp->dwSize      = sizeof(*psp);
    psp->dwFlags     = PSP_DEFAULT;
    psp->hInstance   = ghInstance;
    psp->pszTemplate = MAKEINTRESOURCE(IDD_SCRIPT);
    psp->pfnDlgProc  = ScriptAppletDlgProc;
    psp->pszTitle    = NULL;
    psp->lParam      = (LPARAM)pConnEntDlg;
    psp->pfnCallback = NULL;

    // Create the property sheet for the modem object
    hpage = CreatePropertySheetPage(psp);
    psh->phpage[psh->nPages++] = hpage;
    psp++;


#if defined(MULTILINK_ENABLED) && defined(MULTILINK_PROP_PAGE)
    //
    // Initialize the Multilink page
    //
    psp->dwSize      = sizeof(*psp);
    psp->dwFlags     = PSP_DEFAULT;
    psp->hInstance   = ghInstance;
    psp->pszTemplate = MAKEINTRESOURCE(IDD_ML);
    psp->pfnDlgProc  = MLDlgProc;
    psp->pszTitle    = NULL;
    psp->lParam      = (LPARAM)pConnEntDlg;
    psp->pfnCallback = NULL;

    //
    // Create the property sheet for the multilink page
    //
    hpage = CreatePropertySheetPage(psp);
    psh->phpage[psh->nPages++] = hpage;
    psp++;
#endif
  }

  return pPropPage;
}

//****************************************************************************
// void CALLBACK CallEntry_ReleasePropSheet (LPPROPSHEETPAGE)
//
// This function releases the resource allocated for the modem property sheet.
//
// History:
//  Tue 23-Feb-1993 10:29:08  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

void CALLBACK CallEntry_ReleasePropSheet (LPPROPSHEETPAGE   psp)
{
  // Free the PROPSHEETINFO structure
  LocalFree((HLOCAL)OFFSETOF(psp));
}

//****************************************************************************
// BOOL CALLBACK _export ConnEntryDlgProc (HWND, UINT, WPARAM, LPARAM)
//
// This function displays the modal connection entry setting dialog box.
//
// History:
//  Wed 17-Mar-1993 07:52:16  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

BOOL    CALLBACK _export ConnEntryDlgProc (HWND    hWnd,
                                           UINT    message,
                                           WPARAM  wParam,
                                           LPARAM  lParam)
{
  PCONNENTDLG pConnEntDlg;

  switch (message)
  {
    case WM_INITDIALOG:
      // Remember the pointer to the Connection Entry structure
      pConnEntDlg   = (LPCONNENTDLG)(((LPPROPSHEETPAGE)lParam)->lParam);
      SetWindowLong(hWnd, DWL_USER, (LONG)(LPCONNENTDLG)pConnEntDlg);

      // Initialize the appearance of the dialog box
      return (InitConnEntryDlg(hWnd, pConnEntDlg));

    case WM_DESTROY:
      pConnEntDlg = (PCONNENTDLG)GetWindowLong(hWnd, DWL_USER);
      DeInitConnEntryDlg(hWnd, pConnEntDlg);
      break;

    case WM_HELP:
    case WM_CONTEXTMENU:
      pConnEntDlg = (PCONNENTDLG)GetWindowLong(hWnd, DWL_USER);

#ifdef MULTILINK_ENABLED
#ifdef MULTILINK_PROP_PAGE
      ContextHelp(gaABEntry, message, wParam, lParam);
#else
      ContextHelp(pConnEntDlg->cmlChannel ? gaABMLEntry : gaABEntry,
                  message, wParam, lParam);
#endif
#else
      ContextHelp(gaABEntry, message, wParam, lParam);
#endif

      break;

    case WM_NOTIFY:
      switch(((NMHDR FAR *)lParam)->code)
      {
        case PSN_KILLACTIVE:
          //
          // Validate the connection information
          //
          SetWindowLong(hWnd, DWL_MSGRESULT, (LONG)IsInvalidConnEntry(hWnd));
          return TRUE;

        case PSN_APPLY:
          //
          // The property sheet information is permanently applied
          //
          pConnEntDlg = (PCONNENTDLG)GetWindowLong(hWnd, DWL_USER);
          if ((!IsInvalidConnEntry(hWnd)) &&
              GetConnectionSetting(hWnd, pConnEntDlg))
          {
            PSUBOBJ pso;

            RnaSaveConnEntry(pConnEntDlg->pConnEntry);

            // Update the IP address based on the selected server type
            //
            RnaSetIPInfo(pConnEntDlg->pConnEntry->pszEntry,
                         &pConnEntDlg->pSMMType->ipData);

#if defined(MULTILINK_ENABLED) && defined(MULTILINK_PROP_PAGE)
#else
            // Update the multilink info
            //
            SaveSubConnList(pConnEntDlg);
#endif

            // Update the remote folder
            //
            if (Subobj_New(&pso, pConnEntDlg->pConnEntry->pszEntry, IDI_REMOTE, 0))
            {
              // Notify the event
              //
              Remote_GenerateEvent(SHCNE_UPDATEITEM, pso, NULL);
              Subobj_Destroy(pso);
            };
          };
          return FALSE;

        default:
          break;
      };
      break;

    case WM_COMMAND:
    {
      PCONNENTDLG pConnEntDlg = (PCONNENTDLG)GetWindowLong(hWnd, DWL_USER);

      return (ConnEntryHandler(hWnd, pConnEntDlg, wParam, lParam));
    }

    case WM_RASDIALEVENT:
    {
      switch (lParam)
      {
        case RNA_ADD_DEVICE:
        case RNA_DEL_DEVICE:
          AdjustDeviceList(hWnd, (PCONNENTDLG)GetWindowLong(hWnd, DWL_USER),
                           lParam);
          PropSheet_QuerySiblings(GetParent(hWnd), wParam, lParam);
          break;

        case RNA_SHUTDOWN:
          PropSheet_PressButton (GetParent(hWnd), PSBTN_CANCEL);
          break;
      }
      break;
    }

  }
  return FALSE;
}

//****************************************************************************
// BOOL NEAR PASCAL ConnEntryHandler (HWND, PCONNENTDLG, WPARAM, LPARAM)
//
// This function handles the connection entry control.
//
// History:
//  Wed 17-Mar-1993 08:38:42  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

BOOL NEAR PASCAL ConnEntryHandler(HWND hWnd, PCONNENTDLG pConnEntDlg,
                                  WPARAM wParam, LPARAM lParam)
{
  // Determine the end-user action
  switch (GET_WM_COMMAND_ID(wParam, lParam))
  {
    case IDC_AB_DEVICE:
      //
      // Adjust the dialog appearance
      //
      if (GET_WM_COMMAND_CMD(wParam, lParam)==CBN_SELCHANGE)
      {
        AdjustConnEntryDlg(hWnd, pConnEntDlg);
      };
      return TRUE;

    case IDC_AB_DEVICESET:
      {
      PDEVCONFIG pDevConfig;
      DWORD      dwRet;


      // Handle the device specific setting
      //
      if ((pDevConfig = RnaBuildDevConfig(pConnEntDlg->pDevConfig,
                                          NULL,
                                          FALSE)) == NULL)
        return FALSE;

      dwRet = RnaDevConfigDlg(hWnd, pDevConfig);
      if (dwRet != SUCCESS)
      {
        ETMsgBox(hWnd, IDS_CAP_REMOTE, dwRet, c_rgetConfigDlg, ARRAYSIZE(c_rgetConfigDlg));
      }
      else
      {
        // Make sure it is the same device. The original device might have been
        // removed while we displaying the config dialog.
        //
        if (!lstrcmp(pConnEntDlg->pDevConfig->di.szDeviceName,
                     pDevConfig->di.szDeviceName))
        {
          // Save the changes in setting
          //
          RtlMoveMemory(&pConnEntDlg->pDevConfig->di, &pDevConfig->di,
                  pDevConfig->di.uSize);

        };
      };
      RnaFreeDevConfig(pDevConfig);

      return TRUE;
      }

    case IDC_AB_COUNTRY:
      if (GET_WM_COMMAND_CMD(wParam, lParam) == CBN_SETFOCUS)
      {
        CompleteCountryCodeList(GET_WM_COMMAND_HWND(wParam, lParam));
      };
      break;

    case IDC_AB_FULLPHONE:
      AdjustPhone(hWnd);
      break;

#ifdef MULTILINK_ENABLED
#ifdef MULTILINK_PROP_PAGE
#else
    case IDC_AB_MLSET:
      DialogBoxParam(ghInstance, MAKEINTRESOURCE(IDD_ML), hWnd,
                     MLDlgProc, (LPARAM)pConnEntDlg);
      break;
#endif
#endif

    default:
      break;
  };
  return FALSE;
}

//****************************************************************************
// BOOL NEAR PASCAL InitConnEntryDlg (HWND, PCONNENTDLG)
//
// This function initializes the appearance on the connection entry dialog.
//
// History:
//  Wed 17-Mar-1993 08:00:26  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

BOOL NEAR PASCAL InitConnEntryDlg (HWND hWnd, PCONNENTDLG pConnEntDlg)
{
  HWND hCtrl;
  PCONNENTRY pConnEntry = pConnEntDlg->pConnEntry;

  // Initialize entry name and device list
  //
  if (!InitNameAndDevice(hWnd, pConnEntry))
    return FALSE;

  // Initialize the phone number
  InitPhoneNumber (hWnd, pConnEntry);

  // Adjust the control appearance
  AdjustConnEntryDlg(hWnd, pConnEntDlg);

#ifdef USE_COMMENT
  // Initialize the user note
  if ((hCtrl = GetDlgItem(hWnd, IDC_AB_NOTE)) != NULL)
  {
    Edit_LimitText(hCtrl, sizeof(pConnEntry->szNote)-1);
    Edit_SetText(hCtrl, pConnEntry->szNote);
  };
#endif // USE_COMMENT

  // Initialize the multilink, if any
  //
  if (GetDlgItem(hWnd, IDC_AB_MLCNT) != NULL)
  {
    SetDlgItemInt(hWnd, IDC_AB_MLCNT, pConnEntDlg->cmlChannel, FALSE);
  };

  // Set the right focus
  //
  if (IsWindowEnabled(hCtrl = GetDlgItem(hWnd, IDC_AB_PHONE)))
  {
    SetFocus(hCtrl);
    Edit_SetSel(hCtrl, 0, -1);
  };

  // Register device change notification
  //
  RnaEngineRequest(RA_REG_DEVCHG, (DWORD)hWnd);

  return FALSE;
}

//****************************************************************************
// void NEAR PASCAL AdjustConnEntryDlg (HWND, PCONNENTDLG)
//
// This function adjusts the appearance on the connection entry dialog based on
// the selected device.
//
// History:
//  Wed 17-Mar-1993 08:00:26  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

#pragma data_seg(DATASEG_READONLY)
UINT const aidNoNull[] = {IDC_AB_PHONETXT,  IDC_AB_PHONE,
                          IDC_AB_AREATXT,   IDC_AB_AREA,
                          IDC_AB_COUNTRYTXT,IDC_AB_COUNTRY,
                          IDC_AB_PHNGRP,    IDC_AB_FULLPHONE};
#pragma data_seg()

static BOOL NEAR PASCAL IsRealDevice (LPSTR szDeviceType)
{
  return lstrcmpi(szDeviceType, DEVICE_NULL);
}

void NEAR PASCAL AdjustConnEntryDlg (HWND hWnd, PCONNENTDLG pConnEntDlg)
{
  BOOL bRealDevice;
  int  i;

  // Adjust device appearance
  AdjustDevice(hWnd, pConnEntDlg);

  // Cache the device type
  bRealDevice = IsRealDevice(pConnEntDlg->pDevConfig->di.szDeviceType);

  // Disable the fields irrelevant to a null modem
  for (i = 0; i < (sizeof(aidNoNull)/sizeof(aidNoNull[0])); i++)
  {
    HWND   hCtrl;

    if ((hCtrl = GetDlgItem(hWnd, aidNoNull[i])) != NULL)
    {
      EnableWindow(hCtrl, bRealDevice);
    };
  };

  // Then adjust phone appearance again
  //
  if (bRealDevice)
  {
    AdjustPhone(hWnd);
  };
}

//****************************************************************************
// void NEAR PASCAL DeInitConnEntryDlg (HWND, PCONNENTDLG)
//
// This function frees all the resources allocated during the dialog init.
//
// History:
//  Wed 17-Mar-1993 08:00:26  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

void NEAR PASCAL DeInitConnEntryDlg(HWND hWnd, PCONNENTDLG pConnEntDlg)
{
  // Register device change notification
  //
  RnaEngineRequest(RA_DEREG_DEVCHG, (DWORD)hWnd);

  // Free all the unselected device's info
  DeInitDeviceList (GetDlgItem(hWnd, IDC_AB_DEVICE));

  // Free the country code list
  DeInitCountryCodeList(GetDlgItem(hWnd, IDC_AB_COUNTRY));
}

//****************************************************************************
// BOOL NEAR PASCAL InitNameAndDevice (HWND, PCONNENTRY)
//
// This function initializes the appearance on the connection entry dialog.
//
// History:
//  Wed 17-Mar-1993 08:00:26  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

BOOL NEAR PASCAL InitNameAndDevice(HWND hWnd, PCONNENTRY pConnEntry)
{
  HWND       hCtrl;
  PDEVCONFIG pDevConfig;
  int        nIndex;

  // Initialize the entry name, if any
  hCtrl = GetDlgItem(hWnd, IDC_AB_ENTRY);
  Edit_LimitText(hCtrl, RAS_MaxEntryName);
  SetWindowText(hCtrl, pConnEntry->pszEntry);

  // Initialize the device name
  InitDeviceList(hCtrl = GetDlgItem(hWnd, IDC_AB_DEVICE));

  // Try to find the selected device
  if ((nIndex = ComboBox_FindStringExact(hCtrl, -1,
                pConnEntry->pDevConfig->di.szDeviceName)) == CB_ERR)
  {
    // We have a device that is not installed, add to the list
    nIndex = ComboBox_AddString(hCtrl, pConnEntry->pDevConfig->di.szDeviceName);

    // Warn user that the device is not installed
    RuiUserMessage(hWnd, IDS_ERR_NO_DEVICE, MB_OK | MB_ICONINFORMATION |
                   MB_SETFOREGROUND);
  };
  ComboBox_SetCurSel(hCtrl, nIndex);

  // Get the currently selected device config
  if ((pDevConfig = RnaBuildDevConfig(pConnEntry->pDevConfig,
                                      pConnEntry->pDevConfig->hIcon,
                                      TRUE)) == NULL)
    return FALSE;

  ComboBox_SetItemData(hCtrl, nIndex, (LPARAM)(LPDEVCONFIG)pDevConfig);
  return TRUE;
}

//****************************************************************************
// BOOL NEAR PASCAL InitPhoneNumber (HWND, PCONNENTRY)
//
// This function initializes the appearance on the connection entry dialog.
//
// History:
//  Wed 17-Mar-1993 08:00:26  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

void NEAR PASCAL InitPhoneNumber (HWND hWnd, PCONNENTRY pConnEntry)
{
  HWND       hCtrl;

  InitAreaCodeList(GetDlgItem(hWnd, IDC_AB_AREA), pConnEntry->pn.szAreaCode);
  InitCountryCodeList(GetDlgItem(hWnd, IDC_AB_COUNTRY),
                      pConnEntry->pn.dwCountryID, FALSE);

  // Check dial as is
  //
  if (GetDlgItem(hWnd, IDC_AB_FULLPHONE) != NULL)
  {
    CheckDlgButton(hWnd, IDC_AB_FULLPHONE,
                   (pConnEntry->pn.dwCountryID == 0 ? 0 : 1));
    AdjustPhone(hWnd);
  };

  hCtrl = GetDlgItem(hWnd, IDC_AB_PHONE);
  Edit_LimitText(hCtrl, sizeof(pConnEntry->pn.szLocal)-1);
  SetWindowText(hCtrl, pConnEntry->pn.szLocal);
}

//****************************************************************************
// void NEAR PASCAL AdjustPhone (HWND)
//
// This function adjusts the phone number appearance
//
// History:
//  Tue 13-Sep-1994 11:42:32  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

void NEAR PASCAL AdjustPhone (HWND hWnd)
{
  BOOL fEnabled;

  if (GetDlgItem(hWnd, IDC_AB_FULLPHONE) == NULL)
    return;

  // Check the state of the dial-as-is box
  //
  fEnabled = IsDlgButtonChecked(hWnd, IDC_AB_FULLPHONE);

  // Enable/disable the appropriate controls
  //
  EnableWindow(GetDlgItem(hWnd, IDC_AB_AREATXT),    fEnabled);
  EnableWindow(GetDlgItem(hWnd, IDC_AB_AREA),       fEnabled);
  EnableWindow(GetDlgItem(hWnd, IDC_AB_COUNTRYTXT), fEnabled);
  EnableWindow(GetDlgItem(hWnd, IDC_AB_COUNTRY),    fEnabled);
  return;
}

//****************************************************************************
// void NEAR PASCAL InitAreaCodeList (HWND, LPSTR)
//
// This function initializes the area code list
//
// History:
//  Mon 01-Mar-1993 13:51:30  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

void NEAR PASCAL InitAreaCodeList (HWND hLB, LPSTR szSelectAreaCode)
{
  LPSTR szAreaList;
  DWORD cEntries, i;
  DWORD dwRet;

  // Limit area code entry
  ComboBox_LimitText(hLB, RAS_MaxAreaCode);

  // Add the selected one if it is a valid area code
  // if (*szSelectAreaCode != '\0')
  {
    ComboBox_AddString(hLB, szSelectAreaCode);
  };

  // Allocate the buffer for the areacode list
  //
  if ((szAreaList = (LPSTR)LocalAlloc(LMEM_FIXED,
                    MAX_AREA_LIST*(RAS_MaxAreaCode+1)+1)) != NULL)
  {
    // Get the area code list cached for this user
    cEntries = MAX_AREA_LIST;
    if (((dwRet = RnaGetAreaCodeList(szAreaList, &cEntries)) == SUCCESS) ||
        (dwRet == ERROR_BUFFER_TOO_SMALL))
    {
      LPSTR   szCurArea;

      if (dwRet == ERROR_BUFFER_TOO_SMALL)
        cEntries = MAX_AREA_LIST;

      // For each area code in the list
      szCurArea = szAreaList;
      for (i = 0; i < cEntries; i++)
      {
        // If it is the selected area code, do not put it in the edit box
        if (lstrcmp(szSelectAreaCode, szCurArea))
        {
          // Add it to the list box
          ComboBox_AddString(hLB, szCurArea);
        };

        szCurArea += lstrlen(szCurArea)+1;
      };
    };

    LocalFree((HLOCAL)szAreaList);
  };

  // If something is in the list, select the first one
  if (((i = ComboBox_GetCount(hLB)) != CB_ERR) && (i != 0))
  {
    ComboBox_SetCurSel(hLB, 0);
  };
  return;
}

//****************************************************************************
// void NEAR PASCAL InitCountryCodeList (HWND, DWORD, BOOL)
//
// This function initializes the call option listbox with the predefine list
//
// History:
//  Mon 01-Mar-1993 13:51:30  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

void NEAR PASCAL InitCountryCodeList (HWND hLB, DWORD dwSelectCountryID,
                                      BOOL fAll)
{
  LPCOUNTRYINFO pci;
  LPCOUNTRYCODE pList, pNext;
  DWORD cbSize, cbOrgSize;
  DWORD cbList;
  DWORD dwNextCountryID, dwRet;
  LPSTR szFmt, szCountryDesc;
  int   nIndex, iSelect;

  // Load the display format
  if ((szFmt = (LPSTR)LocalAlloc(LMEM_FIXED, MAXSTRINGLEN+MAXNAME)) == NULL)
    return;
  szCountryDesc = szFmt+MAXNAME;
  LoadString(ghInstance, IDS_COUNTRY_FMT, szFmt, MAXNAME);

  // Allocate enough space for the country list and one large country info
  cbOrgSize = DEF_COUNTRY_INFO_SIZE;
  if ((pci = (LPCOUNTRYINFO)LocalAlloc(LMEM_FIXED, cbOrgSize))
      != NULL)
  {
    cbList = fAll ? sizeof(COUNTRYCODE)*MAX_COUNTRY : sizeof(COUNTRYCODE);
    if ((pList = (LPCOUNTRYCODE)LocalAlloc(LMEM_FIXED, cbList)) != NULL)
    {
      // Start enumerating the info from the first country
      if (fAll || (dwSelectCountryID==0))
      {
        if (dwSelectCountryID==0)
        {
            // We do not have the currently selected country
            // Get one from TAPI
            //
            if (RnaGetCurrentCountry(&dwSelectCountryID) != ERROR_SUCCESS)
            {
              dwSelectCountryID = 1;
            };

            // If all, start from the first country
            // otherwise use the current country only
            //
            dwNextCountryID = (fAll ? 1 : dwSelectCountryID);
        }
        else
        {
            // All country must be listed
            //
            dwNextCountryID = 1;
        };
      }
      else
      {
        dwNextCountryID = dwSelectCountryID;
      };

      pNext = pList;
      iSelect = 0;

      // For each country
      while (dwNextCountryID != 0)
      {
        pci->dwCountryID  = dwNextCountryID;
        cbSize            = cbOrgSize;

        // Get the current country information
        if ((dwRet = RnaEnumCountryInfo(pci, &cbSize)) == SUCCESS)
        {
          char  szCountryDisp[MAX_COUNTRY_NAME+1];

          // Make a displayable name
          ShortenName((LPSTR)(((LPBYTE)pci)+pci->dwCountryNameOffset),
                      szCountryDisp, sizeof(szCountryDisp));

          // Add the country to the list
          wsprintf(szCountryDesc, szFmt, szCountryDisp, pci->dwCountryCode);
          nIndex = ComboBox_AddString(hLB, szCountryDesc);

          // Copy the country information to our short list
          pNext->dwCountryID   = pci->dwCountryID;
          pNext->dwCountryCode = pci->dwCountryCode;
          dwNextCountryID      = pci->dwNextCountryID;
          ComboBox_SetItemData(hLB, nIndex, pNext);

          // If it is the specified country, make it a default one
          if (pNext->dwCountryID == dwSelectCountryID)
            ComboBox_SetCurSel(hLB, nIndex);

          // if need only one item, bail out
          //
          if (!fAll)
            break;

          // Advance to the next country
          pNext++;
        }
        else
        {
          // If the buffer is too small, reallocate a new one and retry
          if (dwRet == ERROR_BUFFER_TOO_SMALL)
          {
            LocalFree((HLOCAL)pci);
            cbOrgSize = cbSize;
            if ((pci = (LPCOUNTRYINFO)LocalAlloc(LMEM_FIXED, cbOrgSize)) == NULL)
              return;
          }
          else
          {
            // If this is the only country we want their information and
            // it no longer exists, default it to the first one
            //
            if (((!fAll) && (dwNextCountryID == dwSelectCountryID) &&
                 (dwNextCountryID != 1)) &&
                (dwRet == ERROR_TAPI_CONFIGURATION))
            {
              dwNextCountryID = 1;
            }
            else
              break;
          };
        };
      };

      // Select the default device
      if (dwRet == SUCCESS)
      {
        // The combobox gets really confused if we added items after we
        // set the current selection.
        // We need to reselect the country item.
        //
        if ((nIndex = ComboBox_GetCurSel(hLB)) == CB_ERR)
        {
          nIndex = 0;
        };
        ComboBox_SetCurSel(hLB, nIndex);
      };
    };
    LocalFree((HLOCAL)pci);
  };

  LocalFree((HLOCAL)szFmt);
  return;
}

//****************************************************************************
// void NEAR PASCAL CompleteCountryCodeList (HWND)
//
// This function completes the call option listbox with the predefine list
//
// History:
//  Mon 01-Mar-1993 13:51:30  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

void NEAR PASCAL CompleteCountryCodeList (HWND hLB)
{
  LPCOUNTRYCODE lpcc;
  HCURSOR hCursor;
  DWORD dwSelectID = 0;
  int cItems;

  // If we already complete the list, do nothing
  if (ComboBox_GetCount(hLB) > 1)
    return;

  // Get the currently selected country code
  cItems = ComboBox_GetCount(hLB);
  if ((cItems) && (cItems != CB_ERR))
  {
    if ((lpcc = (LPCOUNTRYCODE)ComboBox_GetItemData(hLB, 0)) != NULL)
    {
      // Free the allocated buffer
      dwSelectID = lpcc->dwCountryID;
      LocalFree((HLOCAL)lpcc);
    };
  };

  // Set hourglass for the long operation
  hCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));
  SendMessage(hLB, WM_SETREDRAW, FALSE, 0);

  // Remove the item
  ComboBox_ResetContent(hLB);

  // Enumerate full list
  InitCountryCodeList(hLB, dwSelectID, TRUE);

  // Set the cursor back
  SendMessage(hLB, WM_SETREDRAW, TRUE, 0);
  SetCursor(hCursor);
  return;
}

//****************************************************************************
// void NEAR PASCAL DeInitCountryCodeList (HWND)
//
// This function frees all the resources allocated for the country code list
//
// History:
//  Wed 17-Mar-1993 08:00:26  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

void NEAR PASCAL DeInitCountryCodeList (HWND hCtrl)
{
  DWORD lpList, lpCur;
  int cItems, i;

  // Search the beginning of the buffer
  cItems = ComboBox_GetCount(hCtrl);
  if ((cItems) && (cItems != CB_ERR))
  {
    lpList = (DWORD)ComboBox_GetItemData(hCtrl, 0);

    for (i = 1; i < cItems; i++)
    {
      lpCur = (DWORD)ComboBox_GetItemData(hCtrl, i);
      if (lpCur < lpList)
        lpList = lpCur;
    };

    // Free the buffer
    LocalFree((HLOCAL)lpList);
  };
  return;
};

//****************************************************************************
// void NEAR PASCAL InitDeviceList (HWND, LPCSTR, UINT)
//
// This function initializes the call option listbox with the predefine list
//
// History:
//  Mon 01-Mar-1993 13:51:30  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

void NEAR PASCAL InitDeviceList (HWND hLB)
{
  DWORD cbSize;
  DWORD cEntries;
  LPSTR pPortList, pNext;
  int   nIndex;
  UINT  i;

  // Get the buffer size
  cbSize = 0;
  if (RnaEnumDevices(NULL, &cbSize, &cEntries) != ERROR_BUFFER_TOO_SMALL)
    return;

  // Allocate the buffer for the device list
  if ((pPortList = (LPSTR)LocalAlloc(LMEM_FIXED, (UINT)cbSize)) != NULL)
  {
    // Enumerate the device list
    cEntries = 0;
    if (RnaEnumDevices((LPBYTE)pPortList, &cbSize, &cEntries) == SUCCESS)
    {
      // For each device in the list
      for (i = 0, pNext = pPortList; i < (UINT)cEntries; i++)
      {
        // Check the device type
        //
        if (IsValidDevice(pNext))
        {
          // Add the device to the list box
          nIndex = ComboBox_AddString(hLB, pNext);
          ComboBox_SetItemData(hLB, nIndex, NULL);
        };
        pNext += (lstrlen(pNext)+1);
      };

      // Select the default device
      ComboBox_SetCurSel(hLB, nIndex);
    };

    // Free the buffer
    LocalFree((HLOCAL)pPortList);
  };

  return;
}

//****************************************************************************
// void NEAR PASCAL AdjustDeviceList (HWND, DWORD)
//
// This function adds or removes a device in the device list.
//
// History:
//  Mon 01-Mar-1993 13:51:30  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

void NEAR PASCAL AdjustDeviceList (HWND hwnd, PCONNENTDLG pConnEntDlg,
                                   DWORD dwCommand)
{
  HWND  hLB;
  DWORD cbSize;
  DWORD cEntries;
  LPSTR pPortList, pNext;
  int   nIndex;
  UINT  i;

  hLB = GetDlgItem(hwnd, IDC_AB_DEVICE);

  // Get the buffer size
  cbSize = 0;
  if (RnaEnumDevices(NULL, &cbSize, &cEntries) != ERROR_BUFFER_TOO_SMALL)
    return;

  // Allocate the buffer for the device list
  if ((pPortList = (LPSTR)LocalAlloc(LMEM_FIXED, (UINT)cbSize)) != NULL)
  {
    // Enumerate the device list
    cEntries = 0;
    if (RnaEnumDevices((LPBYTE)pPortList, &cbSize, &cEntries) == SUCCESS)
    {
      // Determine the requested action
      //
      if (dwCommand == RNA_ADD_DEVICE)
      {
        // For each device in the list
        for (i = 0, pNext = pPortList; i < (UINT)cEntries; i++, pNext += (lstrlen(pNext)+1))
        {
          // Check the device type
          //
          if (!IsValidDevice(pNext))
            continue;

          // Check if it is already in the list
          if ((nIndex = ComboBox_FindStringExact(hLB, 0, pNext)) == CB_ERR)
          {
            // Add the device to the list box
            nIndex = ComboBox_AddString(hLB, pNext);
            ComboBox_SetItemData(hLB, nIndex, NULL);

            // This maybe the only device in the list
            //
            if (ComboBox_GetCount(hLB) == 1)
            {
              ComboBox_SetCurSel(hLB, nIndex);
              AdjustDevice(hwnd, pConnEntDlg);
            };
            break;
          }
          else
          {
            PDEVCONFIG pDevConfig, pDefDevCfg;

            // This may be the uninstalled device in the connection entry
            //
            if ((pDevConfig = (PDEVCONFIG)ComboBox_GetItemData(hLB, nIndex))
                != NULL)
            {
              // If it is, it does not have an icon at this point.
              // Give it a new one.
              if (pDevConfig->hIcon == NULL)
              {
                // Get the icon through the default device config
                if ((pDefDevCfg = (PDEVCONFIG)RnaGetDefaultDevConfig(
                                  pDevConfig->di.szDeviceName)) != NULL)
                {
                  pDevConfig->hIcon = pDefDevCfg->hIcon;
                  pDefDevCfg->hIcon = NULL;
                  RnaFreeDevConfig(pDefDevCfg);

                  // Update the displayed icon
                  AdjustDevice(hwnd, pConnEntDlg);
                };
              };
            };
          };
        };
      }
      else
      {
        DWORD i, j, cDevices;
        LPSTR szOrgDevice;
        char  szDeviceName[RAS_MaxDeviceName+1];

        // For each item in the device list
        cDevices = ComboBox_GetCount(hLB);

        // We do not remove the original device if any
        //
        if (pConnEntDlg->pConnEntry != NULL)
        {
          szOrgDevice = pConnEntDlg->pConnEntry->pDevConfig->di.szDeviceName;
        }
        else
        {
          szOrgDevice = NULL;
        };

        for (i = 0; i < cDevices; i++)
        {
          // Get the device name
          ComboBox_GetLBText(hLB, i, szDeviceName);

          // Do not remove from the list if it is the device in the entry
          //
          if ((szOrgDevice != NULL) && (!lstrcmpi(szOrgDevice, szDeviceName)))
            continue;

          // Check whether it is in the current list
          //
          for (j = 0, pNext = pPortList; j < (UINT)cEntries; j++, pNext += (lstrlen(pNext)+1))
          {
            // Check the device type
            //
            if (!IsValidDevice(pNext))
              continue;

            if (!lstrcmpi(pNext, szDeviceName))
              break;
          };

          if (j == cEntries)
          {
            // We remove it only if it was not selected before
            // A (previously) selected device has a devconfig associated with it
            //
            if ((PDEVCONFIG)ComboBox_GetItemData(hLB, i) == NULL)
            {
              // Remove it from the list
              //
              ComboBox_DeleteString(hLB, i);
            };
            break;
          };
        };
      };
    };

    // Free the buffer
    LocalFree((HLOCAL)pPortList);
  };
  return;
}

//****************************************************************************
// void NEAR PASCAL AdjustDevice (HWND, PCONNENTDLG)
//
// This function adjusts the appearance on the connection entry dialog based on
// the selected device.
//
// History:
//  Wed 17-Mar-1993 08:00:26  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

void NEAR PASCAL AdjustDevice(HWND hWnd, PCONNENTDLG pConnEntDlg)
{
  HWND       hCtrl;
  PDEVCONFIG pDevConfig;
  int        nIndex;

  // Get the selected device
  nIndex = ComboBox_GetCurSel(hCtrl = GetDlgItem(hWnd, IDC_AB_DEVICE));
  ASSERT (nIndex != CB_ERR);

  // Check whether we have the device information
  if ((pDevConfig = (PDEVCONFIG)ComboBox_GetItemData(hCtrl, nIndex)) == NULL)
  {
    char    szDeviceName[RAS_MaxDeviceName+1];

    // We do not have the device information, make one
    GetWindowText(hCtrl, szDeviceName, sizeof(szDeviceName));

    // Get a new default device config block
    if ((pDevConfig = RnaGetDefaultDevConfig(szDeviceName)) == NULL)
    {
      // We are in deep trouble.
      // Ideally, we want to back out the current selection
      return;
    };

    // Associate it with the device selection
    ComboBox_SetItemData(hCtrl, nIndex, (LPARAM)pDevConfig);
  };

  // Remember the currently selected SMM for this device
  //
  if ((pConnEntDlg->pDevConfig != NULL) && (pConnEntDlg->pSMMType != NULL))
  {
    pConnEntDlg->pDevConfig->smmi = pConnEntDlg->pSMMType->smmi;
  };

  // Cache the pointer to the device info
  pConnEntDlg->pDevConfig = pDevConfig;

  // Draw the right icon
  SendDlgItemMessage(hWnd, IDC_AB_DEVICO, STM_SETICON, (WPARAM)pDevConfig->hIcon, 0L);

  // Update the SMM list
  //
  BuildSMMList(pConnEntDlg, pDevConfig);

}

//****************************************************************************
// void NEAR PASCAL DeInitDeviceList (HWND)
//
// This function frees all the resources allocated for the device list control
//
// History:
//  Wed 17-Mar-1993 08:00:26  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

void NEAR PASCAL DeInitDeviceList (HWND hCtrl)
{
  int         i, nCount;

  // Get the number of device in the list
  //
  nCount = ComboBox_GetCount(hCtrl);

  // For each device, free the allocated device info
  //
  for (i = 0; i < nCount; i++)
  {
    PDEVCONFIG pDevConfig;

    // Get the pointer to the device info
    //
    pDevConfig = (PDEVCONFIG)ComboBox_GetItemData(hCtrl, i);

    if (pDevConfig != NULL)
    {
      // Free everything associated with this block
      RnaFreeDevConfig(pDevConfig);
    };
  };

  return;
}

//****************************************************************************
// BOOL NEAR PASCAL IsInvalidConnEntry(HWND)
//
// This function validates the current connection information.
//
// History:
//  Wed 17-Mar-1993 08:38:42  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

BOOL NEAR PASCAL IsInvalidConnEntry(HWND hWnd)
{
  PHONENUM  pn;

  // Validate the phone number format, if any
  if (IsWindowEnabled(GetDlgItem(hWnd, IDC_AB_PHONE)))
  {
    // Need to validate phone number
    GetPhoneNumber(hWnd, &pn);

    return (!ValidateEntry(hWnd, (LPBYTE)&pn, IDC_AB_PHONE, IDS_ERR_INVALID_PHONE));
  };

  return FALSE;
}

//****************************************************************************
// BOOL NEAR PASCAL ValidateEntry (HWND, LPBYTE, UINT, UINT)
//
// This function gets and validate the entry value.
//
// History:
//  Thu 08-Apr-1993 08:37:03  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

BOOL NEAR PASCAL ValidateEntry (HWND hWnd, LPBYTE szEntry, UINT idCtrl, UINT uErrMsg)
{
  BOOL bValid;

  switch(idCtrl)
  {
    case IDC_AB_PHONE:
    {
      LPPHONENUM    lppn = (LPPHONENUM)szEntry;

      bValid = FALSE;
      if ((lppn->dwCountryID == 0) || (lppn->dwCountryCode != 0))
      {
        if (*lppn->szLocal != '\0')
          bValid = TRUE;
      }
      else
        idCtrl = IDC_AB_COUNTRY;
      break;
    }
    default:
      bValid = TRUE;        break;
  };

  if (!bValid)
  {
    HWND hCtrl;

    // Notify the end-user
    RuiUserMessage(hWnd, uErrMsg, MB_ICONEXCLAMATION | MB_OK);

    // Set Focus back to the entry name
    if (hCtrl = GetDlgItem(hWnd, idCtrl))
      SetFocus(hCtrl);

    // Return failure
    return FALSE;
  };

  return TRUE;
}

//****************************************************************************
// BOOL NEAR PASCAL GetDeviceConfig(PCONNENTDLG)
//
// This function gets the new settings for the selected device.
//
// History:
//  Mon 14-Mar-1994 16:17:46  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

BOOL NEAR PASCAL GetDeviceConfig(PCONNENTDLG pConnEntDlg)
{
  PCONNENTRY    pConnEntry = pConnEntDlg->pConnEntry;
  PDEVCONFIG    pDevConfig;

  // Clone the new device setting, if any
  //
  if (pConnEntDlg->pDevConfig != NULL)
  {
    // Get the current SMM settings, first
    //
    pConnEntDlg->pDevConfig->smmi = pConnEntDlg->pSMMType->smmi;

    if ((pDevConfig = RnaBuildDevConfig(pConnEntDlg->pDevConfig,
                                        pConnEntry->pDevConfig->hIcon,
                                        FALSE)) == NULL)
      return FALSE;

    // Replace the original setting with a new one
    //
    RnaFreeDevConfig(pConnEntry->pDevConfig);
    pConnEntry->pDevConfig = pDevConfig;
  };
  return TRUE;
}

//****************************************************************************
// void NEAR PASCAL GetPhoneNumber(HWND, LPPHONENUM)
//
// This function gets the phone number setting from the dialog control.
//
// History:
//  Mon 14-Mar-1994 16:17:46  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

void NEAR PASCAL GetPhoneNumber (HWND hWnd, LPPHONENUM lppn)
{
  HWND    hLB;
  LPCOUNTRYCODE lpcc;
  int     cItems;

  // Get the local phone number
  GetDlgItemText(hWnd, IDC_AB_PHONE, lppn->szLocal,
                 sizeof(lppn->szLocal));

  // Check dial as is
  //
  if ((GetDlgItem(hWnd, IDC_AB_FULLPHONE) == NULL) ||
      (IsDlgButtonChecked(hWnd, IDC_AB_FULLPHONE)))
  {
    // Assume we do not have any area code
    *lppn->szAreaCode = '\0';

    // Get Areacode
    if (GetDlgItemText(hWnd, IDC_AB_AREA, lppn->szAreaCode, sizeof(lppn->szAreaCode)))
    {
      LPSTR szBuf = lppn->szAreaCode;

      // Check for the valid character
      //
      while ((*szBuf >= '0') && (*szBuf <= '9'))
      {
        szBuf++;
      };

      // If we do not hit the end of string, we have an incorrect areacode
      // just nullify it.
      //
      if (*szBuf != '\0')
        *lppn->szAreaCode = '\0';
    };

    // Get Country Code
    hLB = GetDlgItem(hWnd, IDC_AB_COUNTRY);
    cItems = ComboBox_GetCount(hLB);
    if ((cItems) && (cItems != CB_ERR))
    {
      lpcc = (LPCOUNTRYCODE)ComboBox_GetItemData(hLB, ComboBox_GetCurSel(hLB));
      lppn->dwCountryID   = lpcc->dwCountryID;
      lppn->dwCountryCode = lpcc->dwCountryCode;
    };
  }
  else
  {
    // Nullify these fields
    //
    lppn->dwCountryID   = 0;
    lppn->dwCountryCode = 0;
    *lppn->szAreaCode   = '\0';
  };

  return;
}

//****************************************************************************
// BOOL NEAR PASCAL GetConnectionSetting(HWND, PCONNENTDLG)
//
// This function gets the setting from the dialog control.
//
// History:
//  Wed 17-Mar-1993 08:38:42  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

BOOL NEAR PASCAL GetConnectionSetting(HWND hWnd, PCONNENTDLG pConnEntDlg)
{
  PCONNENTRY    pConnEntry = pConnEntDlg->pConnEntry;

  // Get the device setting
  if (!GetDeviceConfig(pConnEntDlg))
    return FALSE;

  // Get other information only if it is not a null modem
  if (lstrcmp(pConnEntry->pDevConfig->di.szDeviceType, DEVICE_NULL))
  {
    GetPhoneNumber(hWnd, &pConnEntry->pn);
  }
  else
  {
    // Make sure we clear all these fields
    *pConnEntry->pn.szAreaCode   = '\0';
    pConnEntry->pn.dwCountryID   = 0;
    pConnEntry->pn.dwCountryCode = 0;
    *pConnEntry->pn.szLocal      = '\0';
    *pConnEntry->pn.szExtension  = '\0';
  };

#ifdef USE_COMMENT
  // Get the user note
  if (GetDlgItem(hWnd, IDC_AB_NOTE) != NULL)
  {
    GetDlgItemText(hWnd, IDC_AB_NOTE, pConnEntry->szNote,
                   sizeof(pConnEntry->szNote));
  };
#endif // USE_COMMENT

  return TRUE;

}

//****************************************************************************
// DWORD NEAR PASCAL BuildSMMList (PCONNENTDLG, PDEVCONFIG)
//
// This function allocates a buffer for the server type info and fills out the
// information from the current device.
//
// History:
//  Tue 05-Dec-1995 14:11:10  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

DWORD NEAR PASCAL BuildSMMList (PCONNENTDLG pConnEntDlg,
                                PDEVCONFIG  pDevConfig)
{
  PSMMDATA  pSMMList, pSMMNext;
  LPSTR     pSMMNames, pSMMCurName;
  DWORD     cbList;
  BOOL      fNewSMM;

  // Get the existing list of SMM
  //
  pSMMList = pConnEntDlg->pSMMList;

  // Enumerate the list of SMM for the current device
  // Get the buffer size for the SMM list
  //
  cbList = 0;
  RnaEnumerateSMMNames(pDevConfig->di.szDeviceName, NULL, &cbList,
                       CLIENT_SMM);

  // Use the current SMM name plus two null-terminated string
  //
  cbList += (UINT)lstrlen(pDevConfig->smmi.szSMMType)+2;

  // Allocate the memory block for the list
  //
  if ((pSMMNames = (LPSTR)LocalAlloc(LPTR, cbList)) == NULL)
  {
    return ERROR_OUTOFMEMORY;
  };

  // Get the SMM list
  //
  fNewSMM = TRUE;
  if (RnaEnumerateSMMNames(pDevConfig->di.szDeviceName, pSMMNames, &cbList,
                           CLIENT_SMM) == ERROR_SUCCESS)
  {
    // Search for the current SMM in the list
    //
    cbList = 0;
    while(pSMMNames[cbList])
    {
      if (!lstrcmpi(&pSMMNames[cbList], pDevConfig->smmi.szSMMType))
      {
        fNewSMM = FALSE;
        break;
      };

      // Next SMM in the list
      cbList += lstrlen(&pSMMNames[cbList])+1;
    };
  }
  else
  {
    cbList = 0;
  };

  // If we do not have the current SMM in the list, add it
  //
  if (fNewSMM)
  {
    lstrcpy(&pSMMNames[cbList], pDevConfig->smmi.szSMMType);
  };

  // For each SMM for the new list
  //
  pSMMCurName = (LPSTR)pSMMNames;
  while(*pSMMCurName)
  {
    // Search for the existing SMM list
    //
    pSMMNext = pSMMList;
    while(pSMMNext)
    {
      if (!lstrcmpi(pSMMNext->smmi.szSMMType, pSMMCurName))
        break;
      pSMMNext = pSMMNext->pNext;
    };

    // If the SMM is not in the list
    //
    if (pSMMNext == NULL)
    {
      // Allocate a new SMM data block
      //
      if ((pSMMNext = (PSMMDATA)LocalAlloc(LPTR, sizeof(*pSMMNext)))
          != NULL)
      {
        // Get the default SMM settings
        lstrcpyn(pSMMNext->smmi.szSMMType, pSMMCurName,
                 sizeof(pSMMNext->smmi.szSMMType));
        RnaGetDefaultSMMInfo(pDevConfig->di.szDeviceName,
                             &pSMMNext->smmi, TRUE);
        pSMMNext->ipData.dwSize = sizeof(pSMMNext->ipData);
        RnaGetIPInfo(NULL, &pSMMNext->ipData, TRUE);

        // Insert it in front of the list
        // (so we maintain the pointer to the existing list)
        //
        pSMMNext->pNext = pConnEntDlg->pSMMList;
        pConnEntDlg->pSMMList = pSMMNext;
      };
    };

    // Next SMM in the list
    pSMMCurName += lstrlen(pSMMCurName)+1;
  };

  // If there is a selected SMM
  // and the selected SMM is not in the new device list
  // or If there is no currently selected SMM
  //
  fNewSMM = FALSE;
  if (pConnEntDlg->pSMMType != NULL)
  {
    pSMMCurName = (LPSTR)pSMMNames;
    while(*pSMMCurName)
    {
      if (!lstrcmpi(pConnEntDlg->pSMMType->smmi.szSMMType, pSMMCurName))
        break;

      // Next SMM in the list
      pSMMCurName += lstrlen(pSMMCurName)+1;
    };

    fNewSMM = (*pSMMCurName == '\0');
  }
  else
  {
    fNewSMM = TRUE;
  };

  // Nominate the SMM in the current device config, if needed
  //
  if (fNewSMM)
  {
    LPSTR pszSelectedSMM;

    // If there is no SMM type specified in the device config,
    // i.e., a newly created connection,
    //
    if (*pDevConfig->smmi.szSMMType == '\0')
    {
      // Use the first SMM in the list
      //
      pszSelectedSMM = pSMMNames;
    }
    else
    {
      // Otherwise use the SMM in the device config
      //
      pszSelectedSMM = pDevConfig->smmi.szSMMType;
    };

    // Search for the existing SMM list
    //
    pSMMNext = pConnEntDlg->pSMMList;
    while(pSMMNext)
    {
      if (!lstrcmpi(pSMMNext->smmi.szSMMType, pszSelectedSMM))
      {
        break;
      };
      pSMMNext = pSMMNext->pNext;
    };

    // We just added this SMM or it was already in the list
    //
    ASSERT(pSMMNext != NULL);

    // Nominate the SMM
    //
    pConnEntDlg->pSMMType = pSMMNext;
  };

  LocalFree((HLOCAL)pSMMNames);

  return ERROR_SUCCESS;
}

//****************************************************************************
// void NEAR PASCAL FreeSMMList (PCONNENTDLG)
//
// This function free the resources allocated for the server type list.
//
// History:
//  Tue 05-Dec-1995 14:12:36  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

void NEAR PASCAL FreeSMMList (PCONNENTDLG pConnEntDlg)
{
  PSMMDATA pNext, pFree;

  pNext = pConnEntDlg->pSMMList;
  while(pNext)
  {
    pFree = pNext;
    pNext = pFree->pNext;
    LocalFree((HLOCAL)pFree);
  };

  pConnEntDlg->pSMMList = NULL;
  return;
}

//****************************************************************************
// void NEAR PASCAL BuildSubConnList (PCONNENTDLG)
//
// This function builds the multilink list.
//
// History:
//  Thu 21-Mar-1996 09:58:47  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

void NEAR PASCAL BuildSubConnList (PCONNENTDLG pConnEntDlg)
{
  MLINFO       mli;
  PMLINFO      pmli;
  PSUBCONNENTRY psce;
  DWORD        iSubEntry;
  LPSTR        szEntryName;
  DWORD        cb;

  // Get the multilink information for the entry
  //
  szEntryName = pConnEntDlg->pConnEntry->pszEntry;
  if (RnaGetMultiLinkInfo(szEntryName, &mli)
      != ERROR_SUCCESS)
  {
    mli.cSubEntries = 0;
  };

  // If no sub-entry, disable multi-link
  //
  if (mli.cSubEntries == 0)
  {
    mli.fEnabled    = FALSE;
  };

  // Allocate the buffer for the multilink info
  //
  if ((pmli = (PMLINFO)LocalAlloc(LPTR, sizeof(MLINFO)+
                                        (mli.cSubEntries*sizeof(SUBCONNENTRY))))
      != NULL)
  {
    psce = (PSUBCONNENTRY)(pmli+1);

    // Initialize the header
    //
    *pmli = mli;

    // Initialize each multilink info
    //
    for (iSubEntry = 0; iSubEntry < mli.cSubEntries; iSubEntry++)
    {
      cb = sizeof(*psce);
      if (RnaGetSubEntry (szEntryName, iSubEntry, psce, &cb) == ERROR_SUCCESS)
      {
        psce++;
      }
      else
      {
        // Ignore the failing subentry
        //
        pmli->cSubEntries--;
      };
    };
  };

  pConnEntDlg->pmli = pmli;
  return;
}

//****************************************************************************
// DWORD NEAR PASCAL SaveSubConnList (PCONNENTDLG)
//
// This function saves the information in the multilink list.
//
// History:
//  Thu 21-Mar-1996 09:58:47  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

DWORD NEAR PASCAL SaveSubConnList (PCONNENTDLG pConnEntDlg)
{
  UINT          cSubEntries;
  PMLINFO       pmli;
  PSUBCONNENTRY psce;
  DWORD         iSubEntry;
  LPSTR         szEntryName;
  MLINFO        mli;

  szEntryName = pConnEntDlg->pConnEntry->pszEntry;
  pmli = pConnEntDlg->pmli;

  // Save other settings but clean up the old subentries
  //
  mli = *pmli;
  mli.cSubEntries = 0;
  RnaSetMultiLinkInfo(szEntryName, &mli);

  // Get the item count
  //
  psce = (PSUBCONNENTRY)(pmli+1);
  cSubEntries = pmli->cSubEntries;

  // Save one item at a time
  //
  for (iSubEntry = 0; iSubEntry < cSubEntries; iSubEntry++)
  {
    RnaSetSubEntry(szEntryName, iSubEntry, psce, psce->dwSize);
    psce++;
  };

  return ERROR_SUCCESS;
}

//****************************************************************************
// void NEAR PASCAL FreeSubConnList (PCONNENTDLG)
//
// This function frees the multilink list.
//
// History:
//  Thu 21-Mar-1996 09:58:47  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

void NEAR PASCAL FreeSubConnList (PCONNENTDLG pConnEntDlg)
{
  LocalFree(pConnEntDlg->pmli);
  return;
}

#ifdef USE_DIALHELPER
//****************************************************************************
// void NEAR PASCAL GetPhoneSetting (HWND, UINT)
//
// This function prompts for the phone composition form the connect entry.
//
// History:
//  Wed 17-Mar-1993 08:38:42  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

void NEAR PASCAL GetPhoneSetting (HWND hWnd, UINT idPhone)
{
  HWND      hLB;
  PHONENUM  pn;
  char      szDeviceName[RAS_MaxDeviceName+1];

  // Get the phone number from the control
  GetPhoneNumber (hWnd, &pn);
  hLB = GetDlgItem(hWnd, IDC_AB_DEVICE);
  ComboBox_GetText(hLB, szDeviceName, sizeof(szDeviceName));

  // Pop up the phone composition dialog box
  RnaPhoneConfigDlg (hWnd, szDeviceName, &pn);
  return;
}
#endif // USE_DIALHELPER

