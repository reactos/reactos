//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       getdev.c
//
//--------------------------------------------------------------------------

#include "hdwwiz.h"
#include <htmlhelp.h>

int g_BlankIconIndex;

typedef
UINT
(*PDEVICEPROBLEMTEXT)(
    HMACHINE hMachine,
    DEVNODE DevNode,
    ULONG ProblemNumber,
    LPTSTR Buffer,
    UINT   BufferSize
    );

HMODULE hDevMgr=NULL;
PDEVICEPROBLEMTEXT pDeviceProblemText = NULL;

PTCHAR
DeviceProblemText(
   HMACHINE hMachine,
   DEVNODE DevNode,
   ULONG ProblemNumber
   )
{
   UINT LenChars, ReqLenChars;
   PTCHAR Buffer=NULL;

   if (hDevMgr) {

       if (!pDeviceProblemText) {

           pDeviceProblemText = (PVOID) GetProcAddress(hDevMgr, "DeviceProblemTextW");
       }
   }

   if (pDeviceProblemText) {

       LenChars = (pDeviceProblemText)(hMachine,
                                       DevNode,
                                       ProblemNumber,
                                       Buffer,
                                       0
                                       );
       
       if (!LenChars) {

           goto DPTExitCleanup;
       }

       LenChars++;  // one extra for terminating NULL

       Buffer = LocalAlloc(LPTR, LenChars*sizeof(TCHAR));
       
       if (!Buffer) {

           goto DPTExitCleanup;
       }

       ReqLenChars = (pDeviceProblemText)(hMachine,
                                          DevNode,
                                          ProblemNumber,
                                          Buffer,
                                          LenChars
                                          );
       
       if (!ReqLenChars || ReqLenChars >= LenChars) {

           LocalFree(Buffer);
           Buffer = NULL;
       }
   }

DPTExitCleanup:

   return Buffer;
}

void
InsertNoneOfTheseDevices(
    HWND hwndList
    )
{
    LV_ITEM lviItem;
    TCHAR String[MAX_PATH];

    LoadString(hHdwWiz, IDS_HDW_NONEDEVICES, String, sizeof(String)/sizeof(TCHAR));

    lviItem.mask = LVIF_TEXT | LVIF_PARAM | LVIF_IMAGE;
    lviItem.iSubItem = 0;
    lviItem.lParam = (LPARAM)0;
    lviItem.iItem = 0;
    lviItem.iImage = g_BlankIconIndex;
    lviItem.pszText = String;

    ListView_InsertItem(hwndList, &lviItem);
}

void
InsertProbListView(
     PHARDWAREWIZ HardwareWiz,
     DEVINST DevInst,
     ULONG Problem
     )
{
    INT Index;
    LV_ITEM lviItem;
    PTCHAR FriendlyName;
    GUID ClassGuid;
    ULONG ulSize;
    CONFIGRET ConfigRet;
    TCHAR szBuffer[MAX_PATH];


    lviItem.mask = LVIF_TEXT | LVIF_PARAM;
    lviItem.iSubItem = 0;
    lviItem.lParam = DevInst;
    lviItem.iItem = ListView_GetItemCount(HardwareWiz->hwndProbList);

    //
    // fetch a name for this device
    //

    FriendlyName = BuildFriendlyName(DevInst, HardwareWiz->hMachine);
    if (FriendlyName) {
    
        lviItem.pszText = FriendlyName;

    } else {
    
        lviItem.pszText = szUnknown;
    }

    //
    // Fetch the class icon for this device.
    //

    ulSize = sizeof(szBuffer);
    ConfigRet = CM_Get_DevNode_Registry_Property_Ex(DevInst,
                                                    CM_DRP_CLASSGUID,
                                                    NULL,
                                                    szBuffer,
                                                    &ulSize,
                                                    0,
                                                    HardwareWiz->hMachine
                                                    );


    if (ConfigRet == CR_SUCCESS) {
    
        pSetupGuidFromString(szBuffer, &ClassGuid);

    } else {
    
        ClassGuid = GUID_DEVCLASS_UNKNOWN;
    }

    if (SetupDiGetClassImageIndex(&HardwareWiz->ClassImageList,
                                  &ClassGuid,
                                  &lviItem.iImage
                                  ))
    {
        lviItem.mask |= (LVIF_IMAGE | LVIF_STATE);

        if (Problem) {
        
            lviItem.state = (Problem == CM_PROB_DISABLED) ?
                            INDEXTOOVERLAYMASK(IDI_DISABLED_OVL - IDI_CLASSICON_OVERLAYFIRST + 1) :
                            INDEXTOOVERLAYMASK(IDI_PROBLEM_OVL - IDI_CLASSICON_OVERLAYFIRST + 1);

        } else {

            lviItem.state = INDEXTOOVERLAYMASK(0);
        }

        lviItem.stateMask = LVIS_OVERLAYMASK;
    }

    Index = ListView_InsertItem(HardwareWiz->hwndProbList, &lviItem);


    if ((Index != -1) && (HardwareWiz->ProblemDevInst == DevInst)) {

        ListView_SetItemState(HardwareWiz->hwndProbList,
                              Index,
                              LVIS_SELECTED|LVIS_FOCUSED,
                              LVIS_SELECTED|LVIS_FOCUSED
                              );
    }


    if (FriendlyName) {
    
        LocalFree(FriendlyName);
    }

    return;
}

LRESULT CALLBACK
HdwProbListDlgProc(
   HWND   hDlg,
   UINT   message,
   WPARAM wParam,
   LPARAM lParam
   )
/*++

Routine Description:


Arguments:

   standard stuff.



Return Value:

   LRESULT

--*/

{
    PHARDWAREWIZ HardwareWiz;

    if (message == WM_INITDIALOG) {
    
        LV_COLUMN lvcCol;
        INT Index;
        TCHAR Buffer[64];
        LPPROPSHEETPAGE lppsp = (LPPROPSHEETPAGE)lParam;

        HardwareWiz = (PHARDWAREWIZ) lppsp->lParam;
        SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR)HardwareWiz);
        HardwareWiz->hwndProbList = GetDlgItem(hDlg, IDC_HDWPROBLIST);

        //
        // Insert columns for listview.
        // 0 == device name
        //

        lvcCol.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
        lvcCol.fmt = LVCFMT_LEFT;
        lvcCol.pszText = Buffer;

        lvcCol.iSubItem = 0;
        LoadString(hHdwWiz, IDS_DEVICES, Buffer, SIZECHARS(Buffer));
        ListView_InsertColumn(HardwareWiz->hwndProbList, 0, &lvcCol);

        SendMessage(HardwareWiz->hwndProbList,
                    LVM_SETEXTENDEDLISTVIEWSTYLE,
                    LVS_EX_FULLROWSELECT,
                    LVS_EX_FULLROWSELECT
                    );

        ListView_SetExtendedListViewStyle(HardwareWiz->hwndProbList, LVS_EX_FULLROWSELECT | LVS_EX_LABELTIP);

        return TRUE;
    }

    //
    // retrieve private data from window long (stored there during WM_INITDIALOG)
    //
    HardwareWiz = (PHARDWAREWIZ)GetWindowLongPtr(hDlg, DWLP_USER);

    switch (message) {

    case WM_DESTROY:
        if (HardwareWiz->hfontTextNormal) {
        
            DeleteObject(HardwareWiz->hfontTextNormal);
        }

        if (HardwareWiz->hfontTextHigh) {
        
            DeleteObject(HardwareWiz->hfontTextHigh);
        }

        if (HardwareWiz->hfontTextBold) {
        
            DeleteObject(HardwareWiz->hfontTextBold);
        }

        if (HardwareWiz->hfontTextBigBold) {
        
            DeleteObject(HardwareWiz->hfontTextBigBold);
        }

        if (HardwareWiz->ClassImageList.cbSize) {
        
            SetupDiDestroyClassImageList(&HardwareWiz->ClassImageList);
        }

        break;

    case WM_COMMAND:
        break;

    case WM_NOTIFY: {
    
        NMHDR FAR *pnmhdr = (NMHDR FAR *)lParam;

        switch (pnmhdr->code) {
        
            case PSN_SETACTIVE: {
            
                DWORD DevicesDetected;
                int  nCmdShow;
                HWND hwndProbList;
                HWND hwndParentDlg;
                LVITEM lvItem;
                HICON hIcon;
            
                hwndParentDlg = GetParent(hDlg);

                HardwareWiz->PrevPage = IDD_ADDDEVICE_PROBLIST;

                //
                // initialize the list view, we do this on each setactive
                // since a new class may have been installed or the problem
                // device list may change as we go back and forth between pages.
                //

                hwndProbList = HardwareWiz->hwndProbList;

                SendMessage(hwndProbList, WM_SETREDRAW, FALSE, 0L);
                ListView_DeleteAllItems(hwndProbList);

                if (HardwareWiz->ClassImageList.cbSize) {
                
                    SetupDiDestroyClassImageList(&HardwareWiz->ClassImageList);
                }

                HardwareWiz->ClassImageList.cbSize = sizeof(SP_CLASSIMAGELIST_DATA);
                if (SetupDiGetClassImageListEx(&HardwareWiz->ClassImageList,
                                               HardwareWiz->hMachine ? HardwareWiz->MachineName : NULL,
                                               NULL
                                               ))
                {
                    ListView_SetImageList(hwndProbList,
                                          HardwareWiz->ClassImageList.ImageList,
                                          LVSIL_SMALL
                                          );

                    //
                    // Add the blank icon for "None of the following devices"
                    //
                    if ((hIcon = LoadIcon(hHdwWiz, MAKEINTRESOURCE(IDI_BLANK))) != NULL) {

                        g_BlankIconIndex = ImageList_AddIcon(HardwareWiz->ClassImageList.ImageList, hIcon);
                    }

                } else {
                
                    HardwareWiz->ClassImageList.cbSize = 0;
                }

                //
                // Next put all of the devices into the list
                //
                DevicesDetected = 0;
                BuildDeviceListView(HardwareWiz,
                                    HardwareWiz->hwndProbList,
                                    FALSE,
                                    HardwareWiz->ProblemDevInst,
                                    &DevicesDetected,
                                    NULL
                                    );

                InsertNoneOfTheseDevices(HardwareWiz->hwndProbList);


                lvItem.mask = LVIF_PARAM;
                lvItem.iSubItem = 0;
                lvItem.iItem = ListView_GetNextItem(HardwareWiz->hwndProbList, -1, LVNI_SELECTED);

                //
                // select the first item in the list if nothing else was selected
                //
                if (lvItem.iItem == -1) {

                    ListView_SetItemState(hwndProbList,
                                          0,
                                          LVIS_FOCUSED,
                                          LVIS_FOCUSED
                                          );

                    PropSheet_SetWizButtons(GetParent(hDlg), PSWIZB_BACK);

                } else {                                          

                    PropSheet_SetWizButtons(GetParent(hDlg), PSWIZB_BACK | PSWIZB_NEXT);
                }

                ListView_EnsureVisible(hwndProbList, lvItem.iItem, FALSE);
                ListView_SetColumnWidth(hwndProbList, 0, LVSCW_AUTOSIZE_USEHEADER);

                SendMessage(hwndProbList, WM_SETREDRAW, TRUE, 0L);

            }
                break;

            case PSN_WIZNEXT: {

                LVITEM lvItem;
            
                lvItem.mask = LVIF_PARAM;
                lvItem.iSubItem = 0;
                lvItem.iItem = ListView_GetNextItem(HardwareWiz->hwndProbList, -1, LVNI_SELECTED);

                if (lvItem.iItem != -1) {
    
                    ListView_GetItem(HardwareWiz->hwndProbList, &lvItem);

                    HardwareWiz->ProblemDevInst = (DEVNODE)lvItem.lParam;

                } else {

                    HardwareWiz->ProblemDevInst = 0;
                }

                //
                // If the HardwareWiz->ProblemDevInst is 0 then the user selected none of the items
                // so we will move on to detection
                //
                if (HardwareWiz->ProblemDevInst == 0) {

                    SetDlgMsgResult(hDlg, WM_NOTIFY, IDD_ADDDEVICE_ASKDETECT);

                } else {

                    SetDlgMsgResult(hDlg, WM_NOTIFY, IDD_ADDDEVICE_PROBLIST_FINISH);
                }
            }
                break;

            case PSN_WIZFINISH:
                break;


            case PSN_WIZBACK:
                SetDlgMsgResult(hDlg, WM_NOTIFY, HardwareWiz->EnterFrom);
                break;

            case NM_DBLCLK:
                HardwareWiz->DoTask = TRUE;
                PropSheet_PressButton(GetParent(hDlg), PSBTN_NEXT);
                break;

            case LVN_ITEMCHANGED:
                if (ListView_GetSelectedCount(HardwareWiz->hwndProbList) == 0) {

                    PropSheet_SetWizButtons(GetParent(hDlg), PSWIZB_BACK | PSWIZB_NEXT);
                } else {

                    PropSheet_SetWizButtons(GetParent(hDlg), PSWIZB_BACK | PSWIZB_NEXT);
                }
            }

        }
        break;

    case WM_SYSCOLORCHANGE:
        HdwWizPropagateMessage(hDlg, message, wParam, lParam);
        break;

    default:
        return FALSE;
    }

    return TRUE;
}

INT_PTR CALLBACK
HdwProbListFinishDlgProc(
   HWND   hDlg,
   UINT   wMsg,
   WPARAM wParam,
   LPARAM lParam
   )
/*++

Routine Description:


Arguments:


Return Value:

   LRESULT

--*/

{
    PHARDWAREWIZ HardwareWiz = (PHARDWAREWIZ)GetWindowLongPtr(hDlg, DWLP_USER);

    if (wMsg == WM_INITDIALOG) 
    {
        LPPROPSHEETPAGE lppsp = (LPPROPSHEETPAGE)lParam;

        HardwareWiz = (PHARDWAREWIZ)lppsp->lParam;
        SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR)HardwareWiz);
        SetWindowFont(GetDlgItem(hDlg, IDC_HDWNAME), HardwareWiz->hfontTextBigBold, TRUE);
        return TRUE;
    }

    switch (wMsg) 
    {
        case WM_DESTROY:
            //BUGBUG: Destroy anything?
            break;

        case WM_COMMAND:
            switch (wParam) 
            {
                default:
                    break;
            }
            break;

        case WM_NOTIFY: {
            NMHDR FAR *pnmhdr = (NMHDR FAR *)lParam;

            switch (pnmhdr->code) 
            {
                case PSN_SETACTIVE: 
                {
                    PTCHAR FriendlyName;
                    TCHAR szBuffer[MAX_PATH];
                    PTCHAR ProblemText;
                    ULONG Status, Problem;

                    FriendlyName = BuildFriendlyName(HardwareWiz->ProblemDevInst, NULL);
                    if (FriendlyName) {

                        SetDlgItemText(hDlg, IDC_HDW_DESCRIPTION, FriendlyName);
                        LocalFree(FriendlyName);
                    }

                    Problem = 0;
                    CM_Get_DevNode_Status_Ex(&Status,
                                             &Problem,
                                             HardwareWiz->ProblemDevInst,
                                             0,
                                             HardwareWiz->hMachine
                                             );

                    ProblemText = DeviceProblemText(HardwareWiz->hMachine,
                                                    HardwareWiz->ProblemDevInst,
                                                    Problem
                                                    );

                    if (ProblemText) 
                    {
                        SetDlgItemText(hDlg, IDC_PROBLEM_DESC, ProblemText);
                        LocalFree(ProblemText);
                    }

                    PropSheet_SetWizButtons(GetParent(hDlg), PSWIZB_BACK | PSWIZB_FINISH);
                }
                    break;
    
                case PSN_WIZFINISH:
                    HardwareWiz->RunTroubleShooter = TRUE;
                    break;
    
                case PSN_WIZBACK:
                    SetDlgMsgResult(hDlg, wMsg, IDD_ADDDEVICE_PROBLIST);
                    break;

                }
    
            }
            break;

        default:
            return FALSE;
    }

    return TRUE;
}

