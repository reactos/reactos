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
#include "mlink.h"
#include "rnahelp.h"

extern char g_szProfile[];

#ifdef MULTILINK_ENABLED
//****************************************************************************
// BOOL CALLBACK _export MLDlgProc (HWND, UINT, WPARAM, LPARAM)
//
// This function displays the modal connection entry setting dialog box.
//
// History:
//  Wed 17-Mar-1993 07:52:16  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

BOOL    CALLBACK _export MLDlgProc (HWND    hWnd,
                                    UINT    message,
                                    WPARAM  wParam,
                                    LPARAM  lParam)
{
  PCONNENTDLG pConnEntDlg;

  switch (message)
  {
    case WM_INITDIALOG:
      // Remember the pointer to the Connection Entry structure
#ifdef MULTILINK_PROP_PAGE
      pConnEntDlg = (PCONNENTDLG)(((LPPROPSHEETPAGE)lParam)->lParam);
      SetWindowLong(hWnd, DWL_USER, (LONG)pConnEntDlg);
#else
      pConnEntDlg   = (LPCONNENTDLG)lParam;
      SetWindowLong(hWnd, DWL_USER, (LONG)(LPCONNENTDLG)pConnEntDlg);
#endif
      // Initialize the appearance of the dialog box
      return (InitMLDlg(hWnd));

    case WM_DESTROY:
      DeInitMLDlg(hWnd);
      break;

    case WM_NOTIFY:
    {
      NMHDR FAR *lpnm;

      lpnm = (NMHDR FAR *)lParam;
      switch(lpnm->code)
      {
#ifdef MULTILINK_PROP_PAGE
        case PSN_KILLACTIVE:
          //
          // Validate the connection information
          //
          SetWindowLong(hWnd, DWL_MSGRESULT, (LONG)IsInvalidMLEntry(hWnd));
          return TRUE;

        case PSN_APPLY:
            //
            // The property sheet information is permanently applied
            //

            pConnEntDlg = (PCONNENTDLG)GetWindowLong(hWnd, DWL_USER);

            //
            // Update the multilink info
            //

            TRACE_MSG(TF_ALWAYS, "Getting updated multilink settings");

            GetMLSetting(hWnd);

            //
            // Save the multilink info
            //

            TRACE_MSG(TF_ALWAYS, "Saving updated multilink settings");

            SaveSubConnList(pConnEntDlg);

            return FALSE;
#endif

        case LVN_ITEMCHANGED:
        {
          NM_LISTVIEW *lpnmLV = (NM_LISTVIEW *)lpnm;

          if ((lpnmLV->uChanged & LVIF_STATE) &&
              (lpnmLV->uNewState & LVIS_SELECTED))
          {
            HWND    hLV;
            char    szIndex[MAXNAME+1];

            hLV = GetDlgItem(hWnd, IDC_ML_LIST);
            ListView_GetItemText(hLV, lpnmLV->iItem, 0, szIndex,
                                 sizeof(szIndex));
            SetWindowText(GetDlgItem(hWnd, IDC_ML_SEL), szIndex);
          };
          break;
        };

        case NM_DBLCLK:
        {
          if (lpnm->idFrom == IDC_ML_LIST)
          {
            PostMessage(hWnd, WM_COMMAND, IDC_ML_EDIT,
                        (LPARAM)GetDlgItem(hWnd, IDC_ML_EDIT));
          };
          break;
        };

        default:
          break;
      };
      break;
    }
    case WM_COMMAND:
    {
      // Determine the end-user action
      switch (GET_WM_COMMAND_ID(wParam, lParam))
      {
        case IDC_ML_ENABLE:
        case IDC_ML_DISABLE:
          AdjustMLControls(hWnd);
          break;

        case IDC_ML_ADD:
          AddMLItem(hWnd);
          break;

        case IDC_ML_EDIT:
          EditMLItem(hWnd, GET_WM_COMMAND_ID(wParam, lParam));
          break;

        case IDC_ML_DEL:
          RemoveMLItem(hWnd, GET_WM_COMMAND_ID(wParam, lParam));
          break;

        case IDOK:
          //
          // Validate the connection information
          //
          if (IsInvalidMLEntry(hWnd))
          {
            return TRUE;
          };

          //
          // The property sheet information is permanently applied
          //
          GetMLSetting(hWnd);

        case IDCANCEL:
          EndDialog( hWnd, GET_WM_COMMAND_ID(wParam, lParam));
          break;

        default:
          break;
      };
      break;
    }

    case WM_HELP:
    case WM_CONTEXTMENU:
      ContextHelp(gaSubEntry, message, wParam, lParam);
      break;

    default:
      break;
  }
  return FALSE;
}

//****************************************************************************
// BOOL NEAR PASCAL InitMLDlg (HWND hWnd)
//
// This function initializes the controls on the multilink page.
//
// History:
//  Fri 09-Feb-1996 08:06:06  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

BOOL NEAR PASCAL InitMLDlg (HWND hWnd)
{
  PCONNENTDLG  pConnEntDlg;
  LPSTR        szEntryName;
  PMLINFO      pmli;

  // Get the phonebook entry name
  //
  pConnEntDlg = (PCONNENTDLG)GetWindowLong(hWnd, DWL_USER);
  szEntryName = pConnEntDlg->pConnEntry->pszEntry;
  pmli = pConnEntDlg->pmli;

  // Enable/disable multilink
  //
  CheckDlgButton(hWnd, pmli->fEnabled ? IDC_ML_ENABLE : IDC_ML_DISABLE, TRUE);

  // Initialize the device list
  //
  InitMLList(hWnd, szEntryName, pmli);

  // Adjust the dialog appearance
  //
  AdjustMLControls(hWnd);

  return TRUE;
}

//****************************************************************************
// void NEAR PASCAL DeinitMLDlg (HWND hWnd)
//
// This function deinitializes the controls on the multilink page.
//
// History:
//  Fri 09-Feb-1996 08:06:06  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

void NEAR PASCAL DeInitMLDlg (HWND hWnd)
{
  // Free resources allocated for the multilink list
  //
  DeinitMLList(hWnd);
  return;
}

//****************************************************************************
// BOOL NEAR PASCAL IsInvalidMLEntry(HWND hWnd)
//
// This function validates the information on the multilink page.
//
// History:
//  Fri 09-Feb-1996 08:06:06  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

BOOL NEAR PASCAL IsInvalidMLEntry(HWND hWnd)
{
  return FALSE;
}

//****************************************************************************
// void NEAR PASCAL InitMLList (HWND hDlg, LPSTR szEntryName, PMLINFO pmli)
//
// This function initializes the multilink list.
//
// History:
//  Mon 12-Feb-1996 08:33:20  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

void NEAR PASCAL InitMLList (HWND hDlg, LPSTR szEntryName, PMLINFO pmli)
{
  HWND         hwnd, hwndFrame;
  UINT         id;
  char         szText[MAXNAME+1];

  // Check whether we have the control
  //
  hwndFrame = GetDlgItem(hDlg, IDC_ML_FRAME);
  if ((hwnd = GetDlgItem(hDlg, IDC_ML_LIST)) == NULL)
  {
    RECT         rectFrame;
    POINT        ptOrgFrame;
    SIZE         sizeFrame;
    LV_COLUMN    col;

    // Create the listview control for the multilink list
    //
    GetWindowRect(hwndFrame, &rectFrame);
    sizeFrame.cx = rectFrame.right - rectFrame.left;
    sizeFrame.cy = rectFrame.bottom - rectFrame.top;
    ptOrgFrame   = *(POINT *)((RECT *)&rectFrame);
    ScreenToClient(hDlg, &ptOrgFrame);
    hwnd = CreateWindowEx(WS_EX_CLIENTEDGE,
                  WC_LISTVIEW,
                  NULL,
    		  WS_CHILD |
                  WS_VISIBLE |
                  WS_TABSTOP |
                  LVS_REPORT |
		  //LVS_ALWAYSSEL |
                  LVS_SINGLESEL,
    		  ptOrgFrame.x, ptOrgFrame.y,
                  sizeFrame.cx, sizeFrame.cy,
                  hDlg,
                  (HMENU)(IDC_ML_LIST),
                  ghInstance,
                  NULL);
    SetWindowPos(hwnd, GetDlgItem(hDlg, IDC_ML_SEL_TXT), 0, 0, 0, 0,
                 SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);

    // Add the header to the list
    //
    col.mask        = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
    col.fmt         = LVCFMT_LEFT;
    col.pszText     = szText;
    col.cchTextMax  = 0;

    for ( id = 0; id < MAX_USER_COLS; id++ )
    {
        //
        // Add a virtual column that will be joined to the first column
        // to make it appear slightly larger than the others
        //
        col.cx          = (id?1:2) * (sizeFrame.cx/(MAX_USER_COLS+1)) - 1;

        //  
        // Set the column text and its id
        //
        LoadString (ghInstance, id-ML_COL_BASE+IDS_ML_COL_BASE, szText,
                  sizeof(szText));
        col.iSubItem = id - 1;

        ListView_InsertColumn(hwnd, id, &col);
    }
  }
  else
  {
    // Clean up the user list
    //
    DeinitMLList(hDlg);
  };

  // If there is a sub-entry, get the sub-entry
  //
  if (pmli->cSubEntries != 0)
  {
    LV_ITEM      item;
    int          iItem;
    BOOL         fSelected;
    DWORD        iSubEntry, iIndex;
    char         szDeviceName[MAXNAME+1];
    PSUBCONNENTRY psce, psceNew;

    // Prepare the name item
    //
    item.mask     = LVIF_TEXT | LVIF_STATE | LVIF_PARAM;
    item.iItem    = 32000;
    item.iSubItem = 0;
    item.state    = 0;
    item.pszText  = szDeviceName;

    fSelected = FALSE;

    // Initialize each multilink info
    //
    psce = (PSUBCONNENTRY)(pmli+1);
    for (iSubEntry = 0, iIndex = 1; iSubEntry < pmli->cSubEntries; iSubEntry++)
    {
      // Allocate the multilink data
      //
      if ((psceNew = (PSUBCONNENTRY)LocalAlloc(LMEM_FIXED, sizeof(*psce)))
          != NULL)
      {
        // Record the information
        //
        *psceNew = *psce;

        // Add this multilink into the list
        //
        lstrcpyn (szDeviceName, psceNew->szDeviceName, MAXNAME);
        
        item.lParam  = (LPARAM)psceNew;
        iItem   = ListView_InsertItem(hwnd, &item);

        ListView_SetItemText(hwnd, iItem, ML_COL_PHONE,  psceNew->szLocal);
        iIndex++;

        // Promote as the selected user
        //
        if (!fSelected)
        {
          SetWindowText(GetDlgItem(hDlg, IDC_ML_SEL), item.pszText);
          fSelected = TRUE;
        }
      }
      psce++;
    };
  };
  return;
}

//****************************************************************************
// void NEAR PASCAL DeinitMLList (HWND hDlg)
//
// This function deinitializes the multilink list.
//
// History:
//  Mon 12-Feb-1996 09:05:30  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

void NEAR PASCAL DeinitMLList (HWND hDlg)
{
  HWND      hwnd;
  LV_ITEM   item;
  DWORD     i, cEntries;

  if ((hwnd = GetDlgItem(hDlg, IDC_ML_LIST)) != NULL)
  {
    // Remove the data
    //
    cEntries = ListView_GetItemCount(hwnd);
    item.mask       = LVIF_PARAM;
    item.iSubItem   = 0;
    item.cchTextMax = 0;

    for (i = 0; i < cEntries; i++)
    {
      item.iItem   = i;
      ListView_GetItem(hwnd, &item);
      LocalFree((HLOCAL)item.lParam);
    };

    // Clean up the user list
    //
    ListView_DeleteAllItems(hwnd);
  };
  return;
}

//****************************************************************************
// DWORD NEAR PASCAL GetMLSetting (HWND hWnd)
//
// This function gets the information on the multilink page.
//
// History:
//  Fri 09-Feb-1996 08:06:06  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

DWORD NEAR PASCAL GetMLSetting (HWND hWnd)
{
  PCONNENTDLG   pConnEntDlg;
  LPSTR         szEntryName;
  PMLINFO       pmli;
  PSUBCONNENTRY psce, psceNew;
  DWORD         iSubEntry;
  HWND          hLV;
  UINT          cSubEntries;
  LV_ITEM       item;

  // Get the phonebook entry
  //
  pConnEntDlg = (PCONNENTDLG)GetWindowLong(hWnd, DWL_USER);
  szEntryName = pConnEntDlg->pConnEntry->pszEntry;

  // Get the multilink list control
  //
  hLV = GetDlgItem(hWnd, IDC_ML_LIST);

  // Get the item count
  //
  cSubEntries = ListView_GetItemCount(hLV);

  // Allocate the buffer for the multilink info
  //
  if ((pmli = (PMLINFO)LocalAlloc(LPTR, sizeof(MLINFO)+
                                        (cSubEntries*sizeof(SUBCONNENTRY))))
      != NULL)
  {
    psce = (PSUBCONNENTRY)(pmli+1);

    // Enable/disable the multilink and clear the old information
    //
    pmli->fEnabled    = IsDlgButtonChecked(hWnd, IDC_ML_ENABLE);
    pmli->cSubEntries = cSubEntries;

    // Save one item at a time
    //
    item.mask       = LVIF_PARAM;
    item.iSubItem   = 0;
    item.cchTextMax = 0;
    item.pszText    = NULL;

    for (iSubEntry = 0; iSubEntry < cSubEntries; iSubEntry++)
    {
      // Get the multilink info
      //
      item.iItem   = iSubEntry;
      ListView_GetItem(hLV, &item);

      // Save it
      //
      psceNew = (PSUBCONNENTRY)item.lParam;
      *psce   = *psceNew;
      psce++;
    };

    // Replace the old info with the new one;
    //
    LocalFree (pConnEntDlg->pmli);
    pConnEntDlg->pmli = pmli;
  };
  return ERROR_SUCCESS;
}

//****************************************************************************
// void NEAR PASCAL AdjustMLControls(HWND hWnd)
//
// This function adjusts the controls on the multilink page.
//
// History:
//  Fri 09-Feb-1996 08:06:06  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

void NEAR PASCAL AdjustMLControls(HWND hDlg)
{
  PCONNENTDLG   pConnEntDlg;
  HWND  hLV;
  BOOL  fUserCtrl, fEnabled;
  DWORD cEntries;

  pConnEntDlg = (PCONNENTDLG)GetWindowLong(hDlg, DWL_USER);

  // Is multilink enabled?
  //
  fEnabled = IsDlgButtonChecked(hDlg, IDC_ML_ENABLE);

  // Adjust the multilink list
  //
  hLV    = GetDlgItem(hDlg, IDC_ML_LIST);
  cEntries = ListView_GetItemCount(hLV);
  ShowWindow(hLV, fEnabled ? SW_SHOW : SW_HIDE);
  ShowWindow(GetDlgItem(hDlg, IDC_ML_FRAME),
             fEnabled ? SW_HIDE : SW_SHOW);
  EnableWindow(GetDlgItem(hDlg, IDC_ML_SEL), fEnabled);
  EnableWindow(GetDlgItem(hDlg, IDC_ML_ADD),
               (fEnabled & (cEntries < pConnEntDlg->cmlChannel) ? TRUE : FALSE));

  // Selction dependent controls
  //
  fUserCtrl = (fEnabled && (cEntries != 0));
  EnableWindow(GetDlgItem(hDlg, IDC_ML_DEL), fUserCtrl);
  EnableWindow(GetDlgItem(hDlg, IDC_ML_EDIT), fUserCtrl);
  EnableWindow(GetDlgItem(hDlg, IDC_ML_SEL_TXT), fUserCtrl);

  if (fUserCtrl)
  {
    char    szDeviceName[MAXNAME+1];
    LV_FINDINFO lvfi;
    int     iItem;

    // Hilite the selected multilink
    //
    GetDlgItemText(hDlg, IDC_ML_SEL, szDeviceName, sizeof(szDeviceName));
    lvfi.flags = LVFI_STRING;
    lvfi.psz   = szDeviceName;
    iItem = ListView_FindItem(hLV, 0, &lvfi);
    ListView_EnsureVisible(hLV, iItem, FALSE);
    ListView_SetItemState(hLV, iItem, LVIS_FOCUSED | LVIS_SELECTED,
                          LVIS_FOCUSED | LVIS_SELECTED);
  };
  return;
}

//****************************************************************************
// DWORD NEAR PASCAL AddMLItem (HWND)
//
// This function adds a new multilink item to the list.
//
// History:
//  Mon 12-Feb-1996 09:52:50  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

DWORD NEAR PASCAL AddMLItem (HWND hDlg)
{
  PCONNENTDLG   pConnEntDlg;
  PDEVICEINFO   pdi;
  HWND          hLV;
  LV_ITEM       item;
  int           iItem;
  char          szDeviceName[MAXNAME+1], szTmp[MAXNAME+1];
  PSUBCONNENTRY psce;

  pConnEntDlg = (PCONNENTDLG)GetWindowLong(hDlg, DWL_USER);
  pdi = &pConnEntDlg->pConnEntry->pDevConfig->di;

  // Allocate a multilink record
  //
  if ((psce = (PSUBCONNENTRY)LocalAlloc(LPTR, sizeof(*psce))) == NULL)
    return ERROR_OUTOFMEMORY;

  // Initialize a default information
  //
  psce->dwSize = sizeof(*psce);
  lstrcpyn(psce->szDeviceName, pdi->szDeviceName, sizeof(psce->szDeviceName));
  lstrcpyn(psce->szDeviceType, pdi->szDeviceType, sizeof(psce->szDeviceType));
  lstrcpyn(psce->szLocal, pConnEntDlg->pConnEntry->pn.szLocal,
           sizeof(psce->szLocal));

  // Get the new information
  //
  if (!DialogBoxParam(ghInstance, MAKEINTRESOURCE(IDD_EDIT_MLI), hDlg,
                      EditMLInfoDlg, (LPARAM)psce))
  {
    LocalFree((HLOCAL)psce);
    return ERROR_CANCELLED;
  };

  // Add the new multilink item
  //
  hLV = GetDlgItem(hDlg, IDC_ML_LIST);
  item.mask     = LVIF_TEXT | LVIF_STATE | LVIF_PARAM;
  item.iItem    = 32000;
  item.iSubItem = 0;
  item.state    = 0;
  item.lParam   = (LPARAM)psce;
  item.pszText  = szDeviceName;

  lstrcpyn (szDeviceName, psce->szDeviceName, MAXNAME);
  iItem         = ListView_InsertItem(hLV, &item);

  // Display the information
  //
  ListView_SetItemText(hLV, iItem, ML_COL_PHONE,  psce->szLocal);

  // May need to set a default user
  //
  if (GetDlgItemText(hDlg, IDC_ML_SEL, szTmp, sizeof(szTmp)) == 0)
  {
    SetDlgItemText(hDlg, IDC_ML_SEL, szDeviceName);
  }

  // Adjust the dialog appearance
  //
  AdjustMLControls(hDlg);
  if (!IsWindowEnabled(GetDlgItem(hDlg, IDC_ML_ADD)))
  {
    SetFocus(GetDlgItem(hDlg, IDC_ML_DEL));
  };

  return ERROR_SUCCESS;
}

//****************************************************************************
// BOOL NEAR PASCAL RemoveMLItem (HWND, UINT)
//
// This function removes a multilink sub connection.
//
// History:
//  Created
//          25-Jul-1996           -by-  Bruce Johnson (bjohnson)
//****************************************************************************

BOOL NEAR PASCAL RemoveMLItem (HWND hDlg, UINT id)
{
    HWND        hLV;
    char        szDeviceName[MAXNAME+1];
    int         iSelect, iLast;
    LV_FINDINFO lvfi;
    LV_ITEM     item;
    BOOL        bUpdated;
    LPSTR       pszSelected;

    // Get the selected line
    //
    GetDlgItemText(hDlg, IDC_ML_SEL, szDeviceName, sizeof(szDeviceName));

    // Get the selected multilink info
    //
    hLV = GetDlgItem(hDlg, IDC_ML_LIST);
    lvfi.flags = LVFI_STRING;
    lvfi.psz   = szDeviceName;
    iSelect = ListView_FindItem(hLV, -1, &lvfi);

    //
    // If the selected item was not found, just return.
    //
    if (iSelect == -1) {
        return FALSE;
    }            

    bUpdated = TRUE;

    // Remove the selected line
    //
    item.mask       = LVIF_PARAM;
    item.iSubItem   = 0;
    item.cchTextMax = 0;
    item.iItem      = iSelect;

    ListView_GetItem(hLV, &item);
    LocalFree((HLOCAL)item.lParam);

    ListView_DeleteItem(hLV, iSelect);

    //
    // Determine the number of lines left in the list
    //
    if ((iLast = ListView_GetItemCount(hLV)) > 0)
    {

      //  
      // Select the first line as the default line
      //

      ListView_GetItemText(hLV, 0, 0, szDeviceName, sizeof(szDeviceName));
      pszSelected = szDeviceName;
      AdjustMLControls(hDlg);

    }
    else
    {

      // No user left in the list, adjust some controls
      //
      AdjustMLControls(hDlg);
      SetFocus(GetDlgItem(hDlg, IDC_ML_ADD));
      pszSelected = (LPSTR)c_szNull;

    }

    SetDlgItemText(hDlg, IDC_ML_SEL, pszSelected);

  return bUpdated;
}


//****************************************************************************
// BOOL NEAR PASCAL EditMLItem (HWND, UINT)
//
// This function modifies the multilink info.
//
// History:
//  Created.
//  Mon 12-Feb-1996 09:27:03  -by-  Viroon  Touranachun [viroont]
//  Updated
//      25-Jul-1996               -by-  Bruce Johnson (bjohnson)
//****************************************************************************

BOOL NEAR PASCAL EditMLItem (HWND hDlg, UINT id)
{
  HWND        hLV;
  char        szDeviceName[MAXNAME+1];
  int         iSelect;
  LV_FINDINFO lvfi;
  LV_ITEM     item;
  BOOL        bUpdated;

  // Get the selected line
  //
  GetDlgItemText(hDlg, IDC_ML_SEL, szDeviceName, sizeof(szDeviceName));

  // Get the selected multilink info
  //
  hLV = GetDlgItem(hDlg, IDC_ML_LIST);
  lvfi.flags = LVFI_STRING;
  lvfi.psz   = szDeviceName;
  iSelect = ListView_FindItem(hLV, -1, &lvfi);

  //
  // If the selected item was not found, just return.
  //
  if (iSelect == -1) {
      return FALSE;
  }            

  // Modify the user access
  //
  item.iItem    = iSelect;
  item.iSubItem = 0;
  item.mask     = LVIF_PARAM;
  ListView_GetItem(hLV, &item);

  // Show the edit dialog
  //
  bUpdated  = DialogBoxParam(ghInstance, MAKEINTRESOURCE(IDD_EDIT_MLI), hDlg,
                             EditMLInfoDlg, item.lParam);

  // Update the multilink information
  //
  if (bUpdated)
  {
    PSUBCONNENTRY psce;

    psce          = (PSUBCONNENTRY)item.lParam;
    item.mask     = LVIF_TEXT;

    // Update the device name
    //
    item.pszText  = psce->szDeviceName;
    ListView_SetItem(hLV, &item);


    // Update the selected device name
    //
    SetDlgItemText(hDlg, IDC_ML_SEL, psce->szDeviceName);


    // Update the phone number
    //
    item.iSubItem = ML_COL_PHONE;
    item.pszText  = psce->szLocal;
    ListView_SetItem(hLV, &item);

  };

  return bUpdated;
}

//************************************************************************
// BOOL CALLBACK _export EditMLInfoDlg (HWND, UINT, WPARAM, LPARAM)
//
// This function displays the multilink info dialog box.
//
// History:
//   Mon 12-Feb-1996 12:33:27  -by-  Viroon  Touranachun [viroont]
//************************************************************************

BOOL CALLBACK _export EditMLInfoDlg(HWND hDlg, UINT message, WPARAM wParam,
                                    LPARAM lParam)
{
  switch(message)
  {
    case WM_INITDIALOG:
      //
      // Init the dialog appearance
      //
      SetWindowLong(hDlg, DWL_USER, (LONG) lParam);

      // Initialize default information and controls
      //
      return (InitEditML(hDlg));

    case WM_DESTROY:
      //
      // Deinitialize dialog box
      //
      DeinitEditML(hDlg);
      break;

    case WM_COMMAND:
      //
      // Respond to the dialog termination
      //
      switch(GET_WM_COMMAND_ID(wParam, lParam))
      {
        case IDC_ML_DEVICE:
          //
          // Adjust the dialog appearance
          //
          if (GET_WM_COMMAND_CMD(wParam, lParam)==CBN_SELCHANGE)
          {
            AdjustMLDeviceList(hDlg);
          };
          return TRUE;


        case IDOK:
          //
          // Save the current information
          //
          if (SaveEditMLInfo(hDlg) != ERROR_SUCCESS)
            break;

        //*****************************************************************
        // Fall through !!! Fall through !!! Fall through !!!
        //*****************************************************************
        //
        case IDCANCEL:
          EndDialog( hDlg, GET_WM_COMMAND_ID(wParam, lParam) == IDOK);
          break;
      }
      break;

    case WM_RASDIALEVENT:
    {
      switch (lParam)
      {
        case RNA_ADD_DEVICE:
        case RNA_DEL_DEVICE:
          ModifyMLDeviceList(hDlg, lParam);
          break;

        case RNA_SHUTDOWN:
          EndDialog( hDlg, FALSE );
          break;
      }
      break;
    }

    case WM_HELP:
    case WM_CONTEXTMENU:
      ContextHelp(gaEditSub, message, wParam, lParam);
      break;

    default:
      break;
  }
  return FALSE;

}

//****************************************************************************
// BOOL NEAR PASCAL InitEditML (HWND hDlg)
//
// This function initializes the multilink info editor.
//
// History:
//  Mon 12-Feb-1996 09:27:03  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

BOOL NEAR PASCAL InitEditML (HWND hDlg)
{
  PSUBCONNENTRY psce;
  HWND          hCtrl;
  int           nIndex;

  // Get multilink info
  //
  psce = (PSUBCONNENTRY)GetWindowLong(hDlg, DWL_USER);

  // Fill-up the device list
  //
  hCtrl = GetDlgItem(hDlg, IDC_ML_DEVICE);
  InitDeviceList(hCtrl);

  // Try to find the selected device
  //
  if ((nIndex = ComboBox_FindStringExact(hCtrl, -1,
                psce->szDeviceName)) == CB_ERR)
  {
    // We have a device that is not installed, add to the list
    //
    nIndex = ComboBox_AddString(hCtrl, psce->szDeviceName);
  };

  // Select a default device
  //
  ComboBox_SetCurSel(hCtrl, nIndex);
  ComboBox_SetItemData(hCtrl, nIndex, (DWORD)psce->szDeviceType);

  // Initialize the phone number
  //
  hCtrl = GetDlgItem(hDlg, IDC_ML_PHONE);
  Edit_LimitText(hCtrl, sizeof(psce->szLocal)-1);
  Edit_SetText(hCtrl, psce->szLocal);

  // Register device change notification
  //
  RnaEngineRequest(RA_REG_DEVCHG, (DWORD)hDlg);

  return TRUE;
}

//****************************************************************************
// void NEAR PASCAL DeinitEditML (HWND hDlg)
//
// This function deinitializes the multilink info editor.
//
// History:
//  Mon 12-Feb-1996 09:27:03  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

void NEAR PASCAL DeinitEditML(HWND hDlg)
{
  // Register device change notification
  //
  RnaEngineRequest(RA_DEREG_DEVCHG, (DWORD)hDlg);
  return;
}

//****************************************************************************
// void NEAR PASCAL AdjustMLDeviceList(HWND hDlg)
//
// This function adjusts the appearance of the device list in the ML editor.
//
// History:
//  Mon 12-Feb-1996 09:27:03  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

void NEAR PASCAL AdjustMLDeviceList(HWND hDlg)
{
  HWND          hCtrl;
  int           iSelect;

  hCtrl = GetDlgItem(hDlg, IDC_ML_DEVICE);
  iSelect = ComboBox_GetCurSel(hCtrl);

  // If we do not know the device type yet, get the device type
  //
  if ((LPSTR)ComboBox_GetItemData(hCtrl, iSelect) == NULL)
  {
    DEVICEINFO di;
    LPCSTR     pszDeviceType;

    // Get the device information
    //
    di.dwVersion = sizeof(di);
    ComboBox_GetText(hCtrl, di.szDeviceName, sizeof(di.szDeviceName));
    RnaGetDeviceInfo(di.szDeviceName, &di);

    // Remember the device type
    //
    if (!lstrcmpi(di.szDeviceType, szISDNDevice))
    {
      pszDeviceType = szISDNDevice;
    }
    else
    {
      if (!lstrcmpi(di.szDeviceType, szModemDevice))
      {
        pszDeviceType = szModemDevice;
      }
      else
      {
        pszDeviceType = szUnknownDevice;
      };
    };
    ComboBox_SetItemData(hCtrl, iSelect, (DWORD)pszDeviceType);
  };
  return;
}

//****************************************************************************
// DWORD NEAR PASCAL SaveEditMLInfo (HWND hDlg)
//
// This function saves the modified multilink information.
//
// History:
//  Mon 12-Feb-1996 09:27:03  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

DWORD NEAR PASCAL SaveEditMLInfo (HWND hDlg)
{
  PSUBCONNENTRY psce;
  HWND          hCtrl;
  int           iSelect;
  LPSTR         pszDeviceType;
  PHONENUM      pn;

  // Get multilink info
  //
  psce = (PSUBCONNENTRY)GetWindowLong(hDlg, DWL_USER);

  // Get the phone number
  //
  ZeroMemory(&pn, sizeof(pn));
  Edit_GetText(GetDlgItem(hDlg, IDC_ML_PHONE), pn.szLocal, sizeof(pn.szLocal));
  if (!ValidateEntry(hDlg, (LPBYTE)&pn, IDC_ML_PHONE, IDS_ERR_INVALID_PHONE))
    return ERROR_INVALID_PARAMETER;
  lstrcpyn(psce->szLocal, pn.szLocal, sizeof(psce->szLocal));

  // Get the device information
  //
  hCtrl = GetDlgItem(hDlg, IDC_ML_DEVICE);
  ComboBox_GetText(hCtrl, psce->szDeviceName, sizeof(psce->szDeviceName));
  iSelect = ComboBox_GetCurSel(hCtrl);
  pszDeviceType = (LPSTR)ComboBox_GetItemData(hCtrl, iSelect);
  if (pszDeviceType != psce->szDeviceType)
  {
    lstrcpyn(psce->szDeviceType, pszDeviceType, sizeof(psce->szDeviceType));
  };
  return ERROR_SUCCESS;
}

//****************************************************************************
// void NEAR PASCAL ModifyMLDeviceList (HWND, DWORD)
//
// This function adds or removes a device in the device list.
//
// History:
//  Mon 01-Mar-1993 13:51:30  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

void NEAR PASCAL ModifyMLDeviceList (HWND hwnd, DWORD dwCommand)
{
  HWND  hLB;
  DWORD cbSize;
  DWORD cEntries;
  LPSTR pPortList, pNext;
  int   nIndex;
  UINT  i;

  hLB = GetDlgItem(hwnd, IDC_ML_DEVICE);

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
              AdjustMLDeviceList(hwnd);
            };
            break;
          };
        };
      }
      else
      {
        DWORD i, j, cDevices;
        char  szDeviceName[RAS_MaxDeviceName+1];

        // For each item in the device list
        cDevices = ComboBox_GetCount(hLB);
        for (i = 0; i < cDevices; i++)
        {
          // Get the device name
          //
          ComboBox_GetLBText(hLB, i, szDeviceName);

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
            // A (previously) selected device has a device type associated with it
            //
            if ((LPSTR)ComboBox_GetItemData(hLB, i) == NULL)
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

#endif // MULTILINK_ENABLED
