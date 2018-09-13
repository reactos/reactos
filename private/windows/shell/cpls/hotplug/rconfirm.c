//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       rconfirm.c
//
//--------------------------------------------------------------------------

#include "HotPlug.h"
#include <help.h>
#include <systrayp.h>

extern HMODULE hHotPlug;

DWORD
WaitDlgMessagePump(
    HWND hDlg,
    DWORD nCount,
    LPHANDLE Handles
    )
{
    DWORD WaitReturn;
    MSG Msg;

    while ((WaitReturn = MsgWaitForMultipleObjects(nCount,
                                                   Handles,
                                                   FALSE,
                                                   INFINITE,
                                                   QS_ALLINPUT
                                                   ))
           == nCount)
    {
        while (PeekMessage(&Msg, NULL, 0, 0, PM_REMOVE)) {

            if (!IsDialogMessage(hDlg,&Msg)) {
                TranslateMessage(&Msg);
                DispatchMessage(&Msg);
            }
        }
    }

    return WaitReturn;
}

int
InsertDeviceNodeListView(
    HWND hwndList,
    PDEVICETREE DeviceTree,
    PDEVTREENODE  DeviceTreeNode,
    INT lvIndex
    )
{
    LV_ITEM lviItem;
    TCHAR Buffer[MAX_PATH];

    lviItem.mask = LVIF_TEXT | LVIF_PARAM;
    lviItem.iItem = lvIndex;
    lviItem.iSubItem = 0;

    if (SetupDiGetClassImageIndex(&DeviceTree->ClassImageList,
                                   &DeviceTreeNode->ClassGuid,
                                   &lviItem.iImage
                                   ))
    {
        lviItem.mask |= LVIF_IMAGE;
    }

    lviItem.pszText = FetchDeviceName(DeviceTreeNode);

    if (!lviItem.pszText) {

        lviItem.pszText = Buffer;
        wsprintf(Buffer,
                 TEXT("%s %s"),
                 szUnknown,
                 DeviceTreeNode->Location  ? DeviceTreeNode->Location : TEXT("")
                 );
    }

    lviItem.lParam = (LPARAM) DeviceTreeNode;

    return ListView_InsertItem(hwndList, &lviItem);
}

DWORD
RemoveThread(
   PVOID pvDeviceTree
   )
{
    PDEVICETREE DeviceTree = pvDeviceTree;
    PDEVTREENODE  DeviceTreeNode;
    CONFIGRET ConfigRet;
    ULONG DevNodeStatus, Problem;
    PNP_VETO_TYPE VetoType;
    TCHAR szVetoName[512];
    VETO_DEVICE_COLLECTION_DATA VetoedDeviceData;
    SAFE_REMOVAL_COLLECTION_DATA SafeRemovalData;
    DEVICE_COLLECTION Device1, Device2;
    DEVNODE cmDeviceNode;
    DWORD dwCapabilities, len;
    BOOLEAN bDockDevice;

    DeviceTreeNode = DeviceTree->ChildRemovalList;

    ConfigRet = CM_Get_DevNode_Status_Ex(&DevNodeStatus,
                                         &Problem,
                                         DeviceTreeNode->DevInst,
                                         0,
                                         DeviceTree->hMachine
                                         );

    if (ConfigRet != CR_SUCCESS) {
        DevNodeStatus = 0;
        Problem = 0;
    }

    bDockDevice = FALSE;
    if (CM_Locate_DevNode(&cmDeviceNode,
                          DeviceTreeNode->InstanceId,
                          CM_LOCATE_DEVNODE_NORMAL) == CR_SUCCESS) {

        if (CM_Get_DevNode_Registry_Property_Ex(
            cmDeviceNode,
            CM_DRP_CAPABILITIES,
            NULL,
            (PVOID)&dwCapabilities,
            &len,
            0,
            NULL) == CR_SUCCESS) {

            if (dwCapabilities & CM_DEVCAP_DOCKDEVICE) {

                bDockDevice = TRUE;
            }
        }
    }

    if (DeviceTree->IsDialog) {

        ConfigRet = CM_Request_Device_Eject_Ex( DeviceTreeNode->DevInst,
                                                NULL,
                                                NULL,
                                                0,
                                                0,
                                                DeviceTree->hMachine);
    } else {

        ConfigRet = CM_Request_Device_Eject_Ex( DeviceTreeNode->DevInst,
                                                &VetoType,
                                                szVetoName,
                                                sizeof(szVetoName),
                                                0,
                                                DeviceTree->hMachine);

        if (ConfigRet == CR_SUCCESS) {

            //
            // For dialog land, we will display things ourselves in the
            // next wizard page.
            //

        } else if (ConfigRet == CR_REMOVE_VETOED) {

            //
            // Bring up the vetoed dialog ourselves. Device1 is ourselves.
            //
            VetoedDeviceData.hMachine = DeviceTree->hMachine;
            VetoedDeviceData.DeviceList = &Device1;
            VetoedDeviceData.NumDevices = 2;
            VetoedDeviceData.VetoType = VetoType;
            VetoedDeviceData.VetoedOperation = VETOED_EJECT;

            lstrcpy(Device1.DeviceInstanceId, DeviceTreeNode->InstanceId);
            Device1.Next = &Device2;

            lstrcpy(Device2.DeviceInstanceId, szVetoName);
            Device2.Next = NULL;

            DialogBoxParam(
                hHotPlug,
                MAKEINTRESOURCE(DLG_REMOVAL_VETOED),
                NULL,
                DeviceRemovalVetoedDlgProc,
                (LPARAM)&VetoedDeviceData
                );

        } else {

            //
            // Bring up the vetoed dialog ourselves. Device1 is ourselves.
            //
            VetoedDeviceData.hMachine = DeviceTree->hMachine;
            VetoedDeviceData.DeviceList = &Device1;
            VetoedDeviceData.NumDevices = 1;
            VetoedDeviceData.VetoType = PNP_VetoTypeUnknown;
            VetoedDeviceData.VetoedOperation = VETOED_EJECT;

            lstrcpy(Device1.DeviceInstanceId, DeviceTreeNode->InstanceId);
            Device1.Next = NULL;

            DialogBoxParam(
                hHotPlug,
                MAKEINTRESOURCE(DLG_REMOVAL_VETOED),
                NULL,
                DeviceRemovalVetoedDlgProc,
                (LPARAM)&VetoedDeviceData
                );
        }
    }

    return ConfigRet;
}

BOOL
OnOkRemove(
    HWND hDlg,
    PDEVICETREE DeviceTree
    )
{
    HCURSOR hCursor;
    PDEVTREENODE DeviceTreeNode;
    HANDLE hThread;
    DWORD ThreadId;
    DWORD WaitReturn;
    PTCHAR DeviceName;
    TCHAR szReply[MAX_PATH];
    TCHAR Buffer[MAX_PATH];
    BOOL bSuccess;

    hCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));
    DeviceTreeNode = DeviceTree->ChildRemovalList;
    DeviceTree->RedrawWait = TRUE;


    hThread = CreateThread(NULL,
                           0,
                           RemoveThread,
                           DeviceTree,
                           0,
                           &ThreadId
                           );
    if (!hThread) {

        return FALSE;
    }

    //
    // disable the ok\cancel buttons if this is a dialog
    //
    if (DeviceTree->IsDialog) {

        EnableWindow(GetDlgItem(hDlg, IDOK), FALSE);
        EnableWindow(GetDlgItem(hDlg, IDCANCEL), FALSE);

    //
    // disable the back/next buttons if this is a wizard page
    } else {

        PropSheet_SetWizButtons(GetParent(hDlg), 0);
    }


    WaitReturn = WaitDlgMessagePump(hDlg, 1, &hThread);

    bSuccess =
        (WaitReturn == 0 &&
         GetExitCodeThread(hThread, &WaitReturn) &&
         WaitReturn == CR_SUCCESS );

    SetCursor(hCursor);
    DeviceTree->RedrawWait = FALSE;
    CloseHandle(hThread);

    return bSuccess;
}

#define idh_hwwizard_confirm_stop_list  15321   // "" (SysListView32)

DWORD RemoveConfirmHelpIDs[] = {
    IDC_REMOVELIST,    idh_hwwizard_confirm_stop_list,
    IDC_NOHELP1,       NO_HELP,
    IDC_NOHELP2,       NO_HELP,
    IDC_NOHELP3,       NO_HELP,
    0,0
    };


BOOL
InitRemoveConfirmDlgProc(
    HWND hDlg,
    PDEVICETREE DeviceTree
    )
{
    HWND hwndList;
    PDEVTREENODE DeviceTreeNode;
    int lvIndex;
    LV_COLUMN lvcCol;
    PDEVTREENODE Next;
    HICON hIcon;


    if (DeviceTree->IsDialog) {

        hIcon = LoadIcon(hHotPlug,MAKEINTRESOURCE(IDI_HOTPLUGICON));

        if (hIcon) {

            SendMessage(hDlg, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
            SendMessage(hDlg, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
        }

        DeviceTreeNode = DeviceTree->ChildRemovalList;

        if (!DeviceTreeNode) {

            return FALSE;
        }

        DeviceTree->hwndRemove = hDlg;
    }


    hwndList = GetDlgItem(hDlg, IDC_REMOVELIST);

    ListView_SetImageList(hwndList, DeviceTree->ClassImageList.ImageList, LVSIL_SMALL);
    ListView_DeleteAllItems(hwndList);

    // Insert a column for the class list
    lvcCol.mask = LVCF_FMT | LVCF_WIDTH;
    lvcCol.fmt = LVCFMT_LEFT;
    lvcCol.iSubItem = 0;
    ListView_InsertColumn(hwndList, 0, (LV_COLUMN FAR *)&lvcCol);


    if (DeviceTree->IsDialog) {

        SendMessage(hwndList, WM_SETREDRAW, FALSE, 0L);

        //
        // Walk the removal list and add each of them to the listbox.
        //
        lvIndex = 0;

        do {

            InsertDeviceNodeListView(hwndList, DeviceTree, DeviceTreeNode, lvIndex++);
            DeviceTreeNode = DeviceTreeNode->NextChildRemoval;

        } while (DeviceTreeNode != DeviceTree->ChildRemovalList);


        ListView_SetItemState(hwndList, 0, LVIS_SELECTED|LVIS_FOCUSED, LVIS_SELECTED|LVIS_FOCUSED);
        ListView_EnsureVisible(hwndList, 0, FALSE);
        ListView_SetColumnWidth(hwndList, 0, LVSCW_AUTOSIZE_USEHEADER);

        SendMessage(hwndList, WM_SETREDRAW, TRUE, 0L);
    }

    return TRUE;
}

BOOL
RemoveConfirmDlgProcSetActive(
    HWND hDlg,
    PDEVICETREE DeviceTree
    )
{
    HWND hwndList;
    PDEVTREENODE DeviceTreeNode;
    int lvIndex;
    PDEVTREENODE Next;


    DeviceTreeNode = DeviceTree->ChildRemovalList;

    if (!DeviceTreeNode) {

        return FALSE;
    }

    DeviceTree->hwndRemove = hDlg;


    hwndList = GetDlgItem(hDlg, IDC_REMOVELIST);

    ListView_DeleteAllItems(hwndList);

    SendMessage(hwndList, WM_SETREDRAW, FALSE, 0L);

    //
    // Walk the removal list and add each of them to the listbox.
    //
    lvIndex = 0;

    do {

        InsertDeviceNodeListView(hwndList, DeviceTree, DeviceTreeNode, lvIndex++);
        DeviceTreeNode = DeviceTreeNode->NextChildRemoval;

    } while (DeviceTreeNode != DeviceTree->ChildRemovalList);


    ListView_SetItemState(hwndList, 0, LVIS_SELECTED|LVIS_FOCUSED, LVIS_SELECTED|LVIS_FOCUSED);
    ListView_EnsureVisible(hwndList, 0, FALSE);
    ListView_SetColumnWidth(hwndList, 0, LVSCW_AUTOSIZE_USEHEADER);

    SendMessage(hwndList, WM_SETREDRAW, TRUE, 0L);

    PropSheet_SetWizButtons(GetParent(hDlg), PSWIZB_BACK | PSWIZB_NEXT);

    return TRUE;
}


LRESULT CALLBACK
RemoveConfirmDlgProc(
   HWND   hDlg,
   UINT   message,
   WPARAM wParam,
   LPARAM lParam
   )
/*++

Routine Description:

   DialogProc to confirm user really wants to remove the devices.

Arguments:

   standard stuff.



Return Value:

   LRESULT

--*/

{
    PDEVICETREE DeviceTree=NULL;
    BOOL Status = TRUE;

    if (message == WM_INITDIALOG) {

        LPPROPSHEETPAGE lppsp = (LPPROPSHEETPAGE)lParam;
        DeviceTree = GetDeviceTreeFromPsPage(lppsp);

        SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR)DeviceTree);

        if (DeviceTree) {

            if (DeviceTree->HideUI) {

                PostMessage(hDlg, WUM_EJECTDEVINST, 0, 0);

            } else {

                InitRemoveConfirmDlgProc(hDlg, DeviceTree);
            }
        }

        return TRUE;
    }

    //
    // retrieve private data from window long (stored there during WM_INITDIALOG)
    //
    DeviceTree = (PDEVICETREE)GetWindowLongPtr(hDlg, DWLP_USER);


    switch (message) {

    case WM_DESTROY:

        DeviceTree->hwndRemove = NULL;
        break;


    case WM_CLOSE:
        SendMessage (hDlg, WM_COMMAND, IDCANCEL, 0L);
        break;

    case WM_COMMAND:
        switch(wParam) {
        case IDOK:
            EndDialog(hDlg, OnOkRemove(hDlg, DeviceTree) ? IDOK : IDCANCEL);
            break;

        case IDCLOSE:
        case IDCANCEL:
            EndDialog(hDlg, IDCANCEL);
            break;
        }
        break;

    case WUM_EJECTDEVINST:
        EndDialog(hDlg, OnOkRemove(hDlg, DeviceTree) ? IDOK : IDCANCEL);
        break;

    case WM_SYSCOLORCHANGE:
        break;

    case WM_CONTEXTMENU:
        WinHelp((HWND)wParam,
                TEXT("hardware.hlp"),
                HELP_CONTEXTMENU,
                (DWORD_PTR)(LPVOID)(PDWORD)RemoveConfirmHelpIDs
                );

        return FALSE;

    case WM_HELP:
        OnContextHelp((LPHELPINFO)lParam, RemoveConfirmHelpIDs);
        break;

    case WM_SETCURSOR:
        if (DeviceTree->RedrawWait || DeviceTree->RefreshEvent) {
            SetCursor(LoadCursor(NULL, IDC_WAIT));
            SetWindowLongPtr(hDlg, DWLP_MSGRESULT, 1);
            }

         break;


    case WM_NOTIFY:

        switch (((NMHDR FAR *)lParam)->code) {
        case LVN_ITEMCHANGED:
            break;

        case PSN_SETACTIVE:
            RemoveConfirmDlgProcSetActive(hDlg, DeviceTree);
            break;

        case PSN_WIZNEXT:
            if (OnOkRemove(hDlg, DeviceTree)) {

                SetDlgMsgResult(hDlg, message, IDD_DYNAWIZ_FINISH);
            } else {

                PropSheet_SetWizButtons(GetParent(hDlg), PSWIZB_BACK | PSWIZB_NEXT);
                SetDlgMsgResult(hDlg, message, -1);
            }
            break;

        case PSN_WIZBACK:
            break;

        }
        break;

    default:
        return FALSE;

    }


    return TRUE;
}


HBITMAP
LoadTaskBarImageAndText(
    HINSTANCE hDllInstance
    )
/*++

Routine Description:

   Loads the taskbar bitmap image and overlays the desired text.
   This is used for the time text string in the taskbar bitmap so it
   can be localized.

Return Value:

   HBITMAP - bitmap with the overlayed text written into it.

--*/

{
    HBITMAP hbmTaskBar;
    HDC hdcBitmap;
    HBITMAP hbmOld;
    HFONT hfontOld = NULL;
    HFONT hfontTemp = NULL;
    NONCLIENTMETRICS ncm;
    TCHAR szString[20];
    RECT rc;

    // Metrics used which define the rectangle to draw the text in.
    int TextTop = 35;
    int TextLeft = 251;
    int TextWidth = 44;
    int TextHeight = 11;

    //
    // ADRIAO BUGBUG 04/12/1999 -
    //     Note that transparent images choose the transparency color from
    // the first pixel (origin at lower left). Actually, as of 04/12/1999,
    // we appear to be keying off the *second* pixel!
    //
    hbmTaskBar = LoadImage(hDllInstance, MAKEINTRESOURCE(IDB_TASKBARBMP),
                           IMAGE_BITMAP, 0, 0,
                           LR_LOADTRANSPARENT | LR_LOADMAP3DCOLORS);

    if (hbmTaskBar == NULL) {
        return NULL;
    }

    // Load the strings that will be painted on the bitmaps
    LoadString(hDllInstance, IDS_TASKBARTEXT, szString, SIZECHARS(szString));

    // Create a compatible DC for painting the text
    hdcBitmap = CreateCompatibleDC(NULL);

    if (hdcBitmap)
    {
        // Create the normal font
        ncm.cbSize = sizeof(ncm);
        if (SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(ncm), &ncm, 0)) {

            ncm.lfCaptionFont.lfHeight = -TextHeight;
            ncm.lfCaptionFont.lfWidth = 0;
            ncm.lfCaptionFont.lfEscapement = 0;
            ncm.lfCaptionFont.lfOrientation = 0;
            ncm.lfCaptionFont.lfWeight = FW_NORMAL;
            ncm.lfCaptionFont.lfItalic = 0;
            ncm.lfCaptionFont.lfUnderline = 0;
            ncm.lfCaptionFont.lfStrikeOut = 0;
            ncm.lfCaptionFont.lfCharSet = 0;
            ncm.lfCaptionFont.lfOutPrecision = 0;
            ncm.lfCaptionFont.lfClipPrecision = 0;
            ncm.lfCaptionFont.lfQuality = 0;
            ncm.lfCaptionFont.lfPitchAndFamily = 0;
            hfontTemp = CreateFontIndirect(&ncm.lfCaptionFont);
        }

        // Set text transparency and color (black)
        SetTextColor(hdcBitmap, RGB(0,0,0));
        SetBkMode(hdcBitmap, TRANSPARENT);
        SetMapMode(hdcBitmap, MM_TEXT);

        hbmOld = SelectObject(hdcBitmap, hbmTaskBar);
        if (hfontTemp) {
            hfontOld = SelectObject(hdcBitmap, hfontTemp);
        }

        // paint the text

        //
        // Subtract 1 from the text top so that high fonts will fit.
        //
        rc.top = TextTop - 1;
        rc.bottom = TextTop + TextHeight - 1;
        rc.left = TextLeft;
        rc.right = TextLeft + TextWidth - 1;

        DrawText(hdcBitmap, szString, -1, &rc, DT_LEFT | DT_BOTTOM | DT_SINGLELINE);

        if (hfontOld) {
            SelectObject(hdcBitmap, hfontOld);
        }
        if (hfontTemp) {
            DeleteObject(hfontTemp);
        }
        SelectObject(hdcBitmap, hbmOld);

        DeleteDC(hdcBitmap);
    }

    return hbmTaskBar;
}

HBITMAP
LoadEjectPcImage(
    HINSTANCE hDllInstance
    )
/*++

Routine Description:

   Loads the taskbar bitmap image and overlays the desired text.
   This is used for the time text string in the taskbar bitmap so it
   can be localized.

Return Value:

   HBITMAP - bitmap with the overlayed text written into it.

--*/

{
    HBITMAP hbmEjectPC;
    HDC hdcBitmap;
    HBITMAP hbmOld;

    //
    // ADRIAO BUGBUG 04/12/1999 -
    //     Note that transparent images choose the transparency color from
    // the first pixel (origin at lower left). Actually, as of 04/12/1999,
    // we appear to be keying off the *second* pixel!
    //
    hbmEjectPC = LoadImage(hDllInstance, MAKEINTRESOURCE(IDB_UNDOCKBMP),
                           IMAGE_BITMAP, 0, 0,
                           LR_LOADTRANSPARENT | LR_LOADMAP3DCOLORS);

    if (hbmEjectPC == NULL) {
        return NULL;
    }

    // Create a compatible DC for painting the text
    hdcBitmap = CreateCompatibleDC(NULL);

    if (hdcBitmap)
    {
        // Set text transparency and color (black)
        SetTextColor(hdcBitmap, RGB(0,0,0));
        SetBkMode(hdcBitmap, TRANSPARENT);
        SetMapMode(hdcBitmap, MM_TEXT);

        hbmOld = SelectObject(hdcBitmap, hbmEjectPC);

        SelectObject(hdcBitmap, hbmOld);
        DeleteDC(hdcBitmap);
    }

    return hbmEjectPC;
}


LRESULT CALLBACK
RemoveFinishDlgProc(
   HWND   hDlg,
   UINT   message,
   WPARAM wParam,
   LPARAM lParam
   )
/*++

Routine Description:

   Final wizard page letting the user know if the stopping the device was successful
   or not.

Arguments:

   standard stuff.



Return Value:

   LRESULT

--*/

{
    PDEVICETREE DeviceTree=NULL;
    BOOL Status = TRUE;
    HFONT hfontTextBigBold = NULL;

    if (message == WM_INITDIALOG) {

        HFONT hfont;
        LOGFONT LogFont;
        int cyText, PtsPixels;

        LPPROPSHEETPAGE lppsp = (LPPROPSHEETPAGE)lParam;
        DeviceTree = GetDeviceTreeFromPsPage(lppsp);

        SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR)DeviceTree);

        //
        // Setup the big bold font
        //
        hfont = (HFONT)SendMessage(GetDlgItem(hDlg, IDC_HDWNAME), WM_GETFONT, 0, 0);
        GetObject(hfont, sizeof(LogFont), &LogFont);

        cyText = LogFont.lfHeight;

        if (cyText < 0) {

            cyText = -cyText;
        }

        LogFont.lfWeight = FW_BOLD;
        PtsPixels = GetDeviceCaps(GetDC(hDlg), LOGPIXELSY);
        LogFont.lfHeight = 0 - (PtsPixels * 12 / 72);
        hfontTextBigBold = CreateFontIndirect(&LogFont);

        SetWindowFont(GetDlgItem(hDlg, IDC_HDWNAME), hfontTextBigBold, TRUE);

        return TRUE;
    }

    //
    // retrieve private data from window long (stored there during WM_INITDIALOG)
    //
    DeviceTree = (PDEVICETREE)GetWindowLongPtr(hDlg, DWLP_USER);


    switch (message) {

    case WM_DESTROY:
        if (hfontTextBigBold) {

            DeleteObject(hfontTextBigBold);
            hfontTextBigBold = NULL;
        }
        DeviceTree->hwndRemove = NULL;
        break;


    case WM_CLOSE:
        SendMessage (hDlg, WM_COMMAND, IDCANCEL, 0L);
        break;

    case WM_COMMAND:
        switch(wParam) {
            UINT Control = GET_WM_COMMAND_ID(wParam, lParam);
            UINT Cmd = GET_WM_COMMAND_CMD(wParam, lParam);

            switch(Control) {

            case IDC_SYSTRAYOPTION:

                if (Cmd == BN_CLICKED) {

                    OnSystrayOptionClicked(hDlg);
                }
                break;
            }
        }
        break;

    case WM_SYSCOLORCHANGE:
        break;

    case WM_CONTEXTMENU:
        WinHelp((HWND)wParam,
                TEXT("hardware.hlp"),
                HELP_CONTEXTMENU,
                (DWORD_PTR)(LPVOID)(PDWORD)RemoveConfirmHelpIDs
                );

        return FALSE;

    case WM_NOTIFY:
    switch (((NMHDR FAR *)lParam)->code) {
        case LVN_ITEMCHANGED:
            break;

        case PSN_SETACTIVE: {

            TCHAR Status[MAX_PATH];

            PropSheet_SetWizButtons(GetParent(hDlg), PSWIZB_FINISH);
            EnableWindow(GetDlgItem(GetParent(hDlg), IDCANCEL), FALSE);

            SetDlgItemText(hDlg, IDC_DEVICE_DESCRIPTION, DeviceTree->SelectedTreeNode->DeviceDesc);

            SendMessage(GetDlgItem(hDlg, IDC_TASKBAR),
                        STM_SETIMAGE,
                        IMAGE_BITMAP,
                        (LPARAM)LoadTaskBarImageAndText(hHotPlug)
                        );

            //
            // Always check the systray option.  We want this on the tray.
            //
            CheckDlgButton(hDlg, IDC_SYSTRAYOPTION, BST_CHECKED);
            SysTray_EnableService(STSERVICE_HOTPLUG, TRUE);
        }
            break;

        case PSN_WIZFINISH: {

            BOOL bChecked;


            //
            // checked means "Show on Systray"
            //

            bChecked = IsDlgButtonChecked(hDlg, IDC_SYSTRAYOPTION);

            SysTray_EnableService(STSERVICE_HOTPLUG, bChecked);
        }
            break;

        case PSN_WIZBACK:
            break;

        }


    default:
        return FALSE;

    }


    return TRUE;

}

int
InsertDevInstListView(
    HWND hwndList,
    PSP_CLASSIMAGELIST_DATA ClassImageList,
    PSURPRISEWARNDEVICES SurpriseWarnDevice,
    INT lvIndex
    )
{
    LV_ITEM lviItem;
    GUID ClassGuid;
    CONFIGRET ConfigRet;
    int Len;
    DEVNODE DevNode;
    TCHAR ClassGuidString[MAX_GUID_STRING_LEN];
    PTCHAR FriendlyName = NULL;

    lviItem.mask = LVIF_TEXT;
    lviItem.iItem = lvIndex;
    lviItem.iSubItem = 0;

    ConfigRet = CM_Locate_DevNode(&DevNode,
                                  SurpriseWarnDevice->DeviceInstanceId,
                                  CM_LOCATE_DEVNODE_PHANTOM
                                  );

    if (ConfigRet != CR_SUCCESS) {

        ClassGuid = GUID_NULL;
        lviItem.pszText = SurpriseWarnDevice->DeviceInstanceId;
    }

    else {

        //
        // Get the friendly name for the device
        //
        if ((FriendlyName = BuildFriendlyName(DevNode, NULL)) != NULL) {

            lviItem.pszText = FriendlyName;
        }

        else {

            lviItem.pszText = SurpriseWarnDevice->DeviceInstanceId;
        }

        //
        // Get the class GUID string for the device
        //
        Len = sizeof(ClassGuidString);

        ConfigRet = CM_Get_DevNode_Registry_Property(DevNode,
                                                     CM_DRP_CLASSGUID,
                                                     NULL,
                                                     (PVOID)ClassGuidString,
                                                     &Len,
                                                     0);

        if (ConfigRet == CR_SUCCESS) {

            pSetupGuidFromString(ClassGuidString, &ClassGuid);
        }
    }

    if (SetupDiGetClassImageIndex(ClassImageList, &ClassGuid, &lviItem.iImage)) {

        lviItem.mask |= LVIF_IMAGE;
    }


    lvIndex = ListView_InsertItem(hwndList, &lviItem);

    if (FriendlyName) {

        LocalFree(FriendlyName);
    }

    return lvIndex;
}


int
InsertDeviceCollectionIntoListView(
    HWND hwndList,
    PSP_CLASSIMAGELIST_DATA ClassImageList,
    PDEVICE_COLLECTION DeviceData,
    INT lvIndex
    )
{
    LV_ITEM lviItem;
    GUID ClassGuid;
    CONFIGRET ConfigRet;
    int Len;
    DEVNODE DevNode;
    TCHAR ClassGuidString[MAX_GUID_STRING_LEN];
    PTCHAR FriendlyName = NULL;

    lviItem.mask = LVIF_TEXT;
    lviItem.iItem = lvIndex;
    lviItem.iSubItem = 0;

    ConfigRet = CM_Locate_DevNode(&DevNode,
                                  DeviceData->DeviceInstanceId,
                                  CM_LOCATE_DEVNODE_PHANTOM
                                  );

    if (ConfigRet != CR_SUCCESS) {

        ClassGuid = GUID_NULL;
        lviItem.pszText = DeviceData->DeviceInstanceId;
    }

    else {

        //
        // Get the friendly name for the device
        //
        if ((FriendlyName = BuildFriendlyName(DevNode, NULL)) != NULL) {

            lviItem.pszText = FriendlyName;
        }

        else {

            lviItem.pszText = DeviceData->DeviceInstanceId;
        }

        //
        // Get the class GUID string for the device
        //
        Len = sizeof(ClassGuidString);

        ConfigRet = CM_Get_DevNode_Registry_Property(DevNode,
                                                     CM_DRP_CLASSGUID,
                                                     NULL,
                                                     (PVOID)ClassGuidString,
                                                     &Len,
                                                     0);

        if (ConfigRet == CR_SUCCESS) {

            pSetupGuidFromString(ClassGuidString, &ClassGuid);
        }
    }

    if (SetupDiGetClassImageIndex(ClassImageList, &ClassGuid, &lviItem.iImage)) {

        lviItem.mask |= LVIF_IMAGE;
    }


    lvIndex = ListView_InsertItem(hwndList, &lviItem);

    if (FriendlyName) {

        LocalFree(FriendlyName);
    }

    return lvIndex;
}


#define idh_hwwizard_unsafe_remove_list 15330  // "" (SysListView32)
#define idh_hwwizard_show_icon          15308   // "Show &icon on taskbar" (Button)

DWORD SurpriseWarnHelpIDs[] = {
    IDC_REMOVELIST,     idh_hwwizard_unsafe_remove_list,
    IDC_SYSTRAYOPTION,  idh_hwwizard_show_icon,          // "Show &icon on taskbar" (Button)
    IDC_NOHELP1,        NO_HELP,
    IDC_NOHELP2,        NO_HELP,
    IDC_NOHELP3,        NO_HELP,
    IDC_NOHELP4,        NO_HELP,
    IDC_NOHELP5,        NO_HELP,
    IDC_STATIC,         NO_HELP,
    IDC_TASKBAR,        NO_HELP,
    0,0
    };

DWORD SafeRemovalHelpIDs[] = {
    IDC_REMOVELIST,     idh_hwwizard_unsafe_remove_list,
    IDC_NOHELP1,        NO_HELP,
    IDC_NOHELP2,        NO_HELP,
    0,0
    };

DWORD VetoedRemovalHelpIDs[] = {
    IDC_VETOTEXT,       NO_HELP,
    0,0
    };

BOOL
InitSurpriseWarnDlgProc(
    HWND hDlg,
    PSURPRISEWARNDATA SurpriseWarnData
    )
{
    HWND hwndList;
    PSURPRISEWARNDEVICES SurpriseWarnDevices;
    INT      NumDevices, lvIndex;
    HICON    hIcon;
    LV_COLUMN lvcCol;

    hIcon = LoadIcon(hHotPlug,MAKEINTRESOURCE(IDI_HOTPLUGICON));

    if (hIcon) {

        SendMessage(hDlg, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
        SendMessage(hDlg, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
    }

    hwndList = GetDlgItem(hDlg, IDC_REMOVELIST);
    SendMessage(hwndList, WM_SETREDRAW, FALSE, 0L);
    ListView_DeleteAllItems(hwndList);

    SurpriseWarnData->ClassImageList.cbSize = sizeof(SP_CLASSIMAGELIST_DATA);

    if (SetupDiGetClassImageList(&SurpriseWarnData->ClassImageList)) {

        ListView_SetImageList(hwndList, SurpriseWarnData->ClassImageList.ImageList, LVSIL_SMALL);
    }

    else {

        SurpriseWarnData->ClassImageList.cbSize = 0;
    }

    //
    // Insert a column for the class list
    //
    lvcCol.mask = LVCF_FMT | LVCF_WIDTH;
    lvcCol.fmt = LVCFMT_LEFT;
    lvcCol.iSubItem = 0;
    ListView_InsertColumn(hwndList, 0, (LV_COLUMN FAR *)&lvcCol);


    //
    // Walk the devinst list and add each of them to the listbox.
    //

    lvIndex = 0;
    SurpriseWarnDevices = SurpriseWarnData->DeviceList;
    NumDevices = SurpriseWarnData->NumDevices;

    while (SurpriseWarnDevices) {

        InsertDevInstListView(hwndList,
                              &SurpriseWarnData->ClassImageList,
                              SurpriseWarnDevices,
                              lvIndex++
                              );

        SurpriseWarnDevices = SurpriseWarnDevices->Next;
    }

    ListView_SetItemState(hwndList, 0, LVIS_SELECTED|LVIS_FOCUSED, LVIS_SELECTED|LVIS_FOCUSED);
    ListView_EnsureVisible(hwndList, 0, FALSE);
    ListView_SetColumnWidth(hwndList, 0, LVSCW_AUTOSIZE_USEHEADER);

    SendMessage(hwndList, WM_SETREDRAW, TRUE, 0L);

    SendMessage(GetDlgItem(hDlg, IDC_TASKBAR),
                STM_SETIMAGE,
                IMAGE_BITMAP,
                (LPARAM)LoadTaskBarImageAndText(hHotPlug)
                );

    //
    // Always check the systray option.  We want this on the tray.
    //
    CheckDlgButton(hDlg, IDC_SYSTRAYOPTION, BST_CHECKED);
    SysTray_EnableService(STSERVICE_HOTPLUG, TRUE);

    return TRUE;
}

BOOL
InitSurpriseUndockDlgProc(
    HWND hDlg,
    PSURPRISEWARNDATA SurpriseWarnData
    )
{
    HWND hwndList;
    INT      NumDevices, lvIndex;
    HICON    hIcon;
    LV_COLUMN lvcCol;
    TCHAR szFormat[512];
    TCHAR szMessage[512];
    PTCHAR FriendlyName;
    DEVNODE DevNode;
    PSURPRISEWARNDEVICES SurpriseWarnDevice;
    ULONG dwCapabilities = 0;
    ULONG dwLen = sizeof(dwCapabilities);

    hIcon = LoadIcon(hHotPlug,MAKEINTRESOURCE(IDI_UNDOCKICON));

    if (hIcon) {

        SendMessage(hDlg, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
        SendMessage(hDlg, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
    }

    LoadString(hHotPlug, IDS_UNSAFE_UNDOCK, szFormat, SIZECHARS(szFormat));

    SurpriseWarnDevice = SurpriseWarnData->DeviceList;
    while (SurpriseWarnDevice) {

        if ((CM_Locate_DevNode(&DevNode,
                               SurpriseWarnDevice->DeviceInstanceId,
                               CM_LOCATE_DEVNODE_PHANTOM) == CR_SUCCESS) &&

            (CM_Get_DevNode_Registry_Property_Ex(DevNode,
                                                 CM_DRP_CAPABILITIES,
                                                 NULL,
                                                 (PVOID)&dwCapabilities,
                                                 &dwLen,
                                                 0,
                                                 NULL) == CR_SUCCESS) &&

            (dwCapabilities & CM_DEVCAP_DOCKDEVICE)) {

            if ((FriendlyName = BuildFriendlyName(DevNode, NULL)) != NULL) {

                wsprintf(szMessage, szFormat, FriendlyName);
                LocalFree(FriendlyName);
                SetDlgItemText(hDlg, IDC_UNDOCK_MESSAGE, szMessage);
            }
        }

        SurpriseWarnDevice = SurpriseWarnDevice->Next;
    }


    //
    // No list view for surprise undock
    //
    SurpriseWarnData->ClassImageList.cbSize = 0;

    SendMessage(GetDlgItem(hDlg, IDC_UNDOCK),
                STM_SETIMAGE,
                IMAGE_BITMAP,
                (LPARAM)LoadEjectPcImage(hHotPlug)
                );

    return TRUE;
}

LRESULT CALLBACK
SurpriseWarnDlgProc(
   HWND   hDlg,
   UINT   message,
   WPARAM wParam,
   LPARAM lParam
   )
/*++

Routine Description:

   DialogProc to confirm user really wants to remove the devices.

Arguments:

   standard stuff.



Return Value:

   LRESULT

--*/

{
    PSURPRISEWARNDATA SurpriseWarnData = NULL;
    BOOL Status = TRUE;

    if (message == WM_INITDIALOG) {

        SurpriseWarnData = (PSURPRISEWARNDATA) lParam;
        SetWindowLongPtr(hDlg, DWLP_USER, lParam);

        Status = (SurpriseWarnData->DockInList) ?
            InitSurpriseUndockDlgProc(hDlg, SurpriseWarnData) :
            InitSurpriseWarnDlgProc(hDlg, SurpriseWarnData);

        if (!Status) {

            EndDialog(hDlg, IDABORT);
        }

        return TRUE;
    }

    //
    // retrieve private data from window long (stored there during WM_INITDIALOG)
    //
    SurpriseWarnData = (PSURPRISEWARNDATA)GetWindowLongPtr(hDlg, DWLP_USER);


    switch (message) {

    case WM_DESTROY:
           // destroy the ClassImageList
        if (SurpriseWarnData->ClassImageList.cbSize) {
            SetupDiDestroyClassImageList(&SurpriseWarnData->ClassImageList);
            SurpriseWarnData->ClassImageList.cbSize = 0;
            }
        break;


    case WM_CLOSE:
        SendMessage (hDlg, WM_COMMAND, IDCANCEL, 0L);
        break;

    case WM_COMMAND:
        switch(wParam) {


        case IDC_SYSTRAYOPTION:

            if (GET_WM_COMMAND_CMD(wParam, lParam) == BN_CLICKED) {

                OnSystrayOptionClicked(hDlg);
            }
            break;

        case IDOK:
            //
            // checked means "Show on Systray"
            //

            SysTray_EnableService(STSERVICE_HOTPLUG, IsDlgButtonChecked(hDlg, IDC_SYSTRAYOPTION));

            //
            // Fall through
            //

        case IDCLOSE:
        case IDCANCEL:
            EndDialog(hDlg, wParam);
            break;
        }
        break;

    case WM_SYSCOLORCHANGE:
        break;

    case WM_HELP:
        OnContextHelp((LPHELPINFO)lParam, SurpriseWarnHelpIDs);
        break;

    case WM_CONTEXTMENU:
        WinHelp((HWND)wParam,
                TEXT("hardware.hlp"),
                HELP_CONTEXTMENU,
                (DWORD_PTR)(LPVOID)(PDWORD)SurpriseWarnHelpIDs
                );

        return FALSE;

    case WM_NOTIFY:
    switch (((NMHDR FAR *)lParam)->code) {
        case LVN_ITEMCHANGED: {
            break;
            }
        }


    default:
        return FALSE;

    }


    return TRUE;
}


BOOL
InitSafeRemovalDlgProc(
    HWND hDlg,
    PSAFE_REMOVAL_COLLECTION_DATA SafeRemovalData
    )
{
    HWND hwndList;
    PDEVICE_COLLECTION SafeRemovalDevice;
    INT      NumDevices, lvIndex;
    LV_COLUMN lvcCol;
    TCHAR szFormat[512];
    TCHAR szMessage[512];
    TCHAR szTitle[256];
    PTCHAR FriendlyName;
    DEVNODE DevNode;

#if 0
    hwndList = GetDlgItem(hDlg, IDC_REMOVELIST);
    SendMessage(hwndList, WM_SETREDRAW, FALSE, 0L);
    ListView_DeleteAllItems(hwndList);

    SafeRemovalData->ClassImageList.cbSize = sizeof(SP_CLASSIMAGELIST_DATA);

    if (SetupDiGetClassImageList(&SafeRemovalData->ClassImageList)) {

        ListView_SetImageList(hwndList, SafeRemovalData->ClassImageList.ImageList, LVSIL_SMALL);
    }

    else {

        SafeRemovalData->ClassImageList.cbSize = 0;
    }

    //
    // Insert a column for the class list
    //
    lvcCol.mask = LVCF_FMT | LVCF_WIDTH;
    lvcCol.fmt = LVCFMT_LEFT;
    lvcCol.iSubItem = 0;
    ListView_InsertColumn(hwndList, 0, (LV_COLUMN FAR *)&lvcCol);


    //
    // Walk the devinst list and add each of them to the listbox.
    //

    lvIndex = 0;
    SafeRemovalDevice = SafeRemovalData->DeviceList;
    NumDevices = SafeRemovalData->NumDevices;

    while (SafeRemovalDevice) {

        InsertDeviceCollectionIntoListView(
            hwndList,
            &SafeRemovalData->ClassImageList,
            SafeRemovalDevice,
            lvIndex++
            );

        SafeRemovalDevice = SafeRemovalDevice->Next;
    }

    ListView_SetItemState(hwndList, 0, LVIS_SELECTED|LVIS_FOCUSED, LVIS_SELECTED|LVIS_FOCUSED);
    ListView_EnsureVisible(hwndList, 0, FALSE);
    ListView_SetColumnWidth(hwndList, 0, LVSCW_AUTOSIZE_USEHEADER);

    SendMessage(hwndList, WM_SETREDRAW, TRUE, 0L);

    return TRUE;
#else
    SafeRemovalData->ClassImageList.cbSize = 0;

    if (SafeRemovalData->DockInList) {

        LoadString(hHotPlug, IDS_UNDOCK_COMPLETE_TEXT, szFormat, SIZECHARS(szFormat));
        LoadString(hHotPlug, IDS_UNDOCK_COMPLETE_TITLE, szTitle, SIZECHARS(szTitle));

    } else {

        LoadString(hHotPlug, IDS_REMOVAL_COMPLETE_TEXT, szFormat, SIZECHARS(szFormat));
        LoadString(hHotPlug, IDS_REMOVAL_COMPLETE_TITLE, szTitle, SIZECHARS(szTitle));
    }

    CM_Locate_DevNode(&DevNode, SafeRemovalData->DeviceList->DeviceInstanceId, CM_LOCATE_DEVNODE_PHANTOM);
    if ((FriendlyName = BuildFriendlyName(DevNode, NULL)) != NULL) {

        wsprintf(szMessage, szFormat, FriendlyName);
        LocalFree(FriendlyName);
    }

    else {

        wsprintf(szMessage, szFormat, SafeRemovalData->DeviceList->DeviceInstanceId);
    }

    //SetWindowText(hDlg, szTitle);
    //SetDlgItemText(hDlg, IDC_REMOVALTEXT, szMessage);
    MessageBox(hDlg, szMessage, szTitle, MB_OK | MB_ICONINFORMATION | MB_SETFOREGROUND | MB_TOPMOST);
    return FALSE;
#endif
}

LRESULT CALLBACK
SafeRemovalDlgProc(
   HWND   hDlg,
   UINT   message,
   WPARAM wParam,
   LPARAM lParam
   )
/*++

Routine Description:

   DialogProc to tell the user it is OK to remove the given device.

Arguments:

   standard stuff.



Return Value:

   LRESULT

--*/

{
    PSAFE_REMOVAL_COLLECTION_DATA SafeRemovalData = NULL;
    BOOL Status = TRUE;

    if (message == WM_INITDIALOG) {

        SafeRemovalData = (PSAFE_REMOVAL_COLLECTION_DATA) lParam;
        SetWindowLongPtr(hDlg, DWLP_USER, lParam);

        if (!InitSafeRemovalDlgProc(hDlg, SafeRemovalData)) {

            EndDialog(hDlg, IDABORT);
        }

        return TRUE;
    }

    //
    // retrieve private data from window long (stored there during WM_INITDIALOG)
    //
    SafeRemovalData = (PSAFE_REMOVAL_COLLECTION_DATA)GetWindowLongPtr(hDlg, DWLP_USER);

    switch (message) {

        case WM_DESTROY:

            //
            // destroy the ClassImageList
            //
            if (SafeRemovalData->ClassImageList.cbSize) {
                SetupDiDestroyClassImageList(&SafeRemovalData->ClassImageList);
                SafeRemovalData->ClassImageList.cbSize = 0;
            }
            break;

        case WM_CLOSE:
            SendMessage (hDlg, WM_COMMAND, IDCANCEL, 0L);
            break;

        case WM_COMMAND:

            switch(wParam) {

                case IDOK:
                case IDCLOSE:
                case IDCANCEL:
                    EndDialog(hDlg, wParam);
                    break;
            }
            break;

        case WM_HELP:
            OnContextHelp((LPHELPINFO)lParam, SafeRemovalHelpIDs);
            break;

        case WM_CONTEXTMENU:
            WinHelp((HWND)wParam,
                    TEXT("hardware.hlp"),
                    HELP_CONTEXTMENU,
                    (DWORD_PTR)(LPVOID)(PDWORD)SafeRemovalHelpIDs
                    );

            return FALSE;

        default:
            return FALSE;

    }

    return TRUE;
}


BOOL
InitVetoedRemovalDlgProc(
    HWND hDlg,
    PVETO_DEVICE_COLLECTION_DATA VetoedRemovalData
    )
{
    HWND hwndList;
    PDEVICE_COLLECTION VetoedRemovalDevice;
    PTSTR pszDeviceId, pszVetoIds = NULL;
    TCHAR szFormat[512];
    TCHAR szMessage[512];
    TCHAR szTitle[256];
    DEVNODE DevNode;
    PTCHAR FriendlyName;
    ULONG messageBase;

    VetoedRemovalData->ClassImageList.cbSize = 0;

    //
    // The first device in the list is the device that failed ejection.
    //
    pszDeviceId = VetoedRemovalData->DeviceList->DeviceInstanceId;

    if (VetoedRemovalData->NumDevices > 1) {

        //
        // This device is one of the vetoers.
        //
        // ADRIAO BUGBUG 03/17/1999 - We currently only display the first.
        //

        pszVetoIds =
            VetoedRemovalData->DeviceList->Next->DeviceInstanceId;
    }

    //
    // Create the veto text
    //
    switch(VetoedRemovalData->VetoedOperation) {

        case VETOED_UNDOCK:
        case VETOED_WARM_UNDOCK:
            messageBase = IDS_DOCKVETO_BASE;
            break;

        case VETOED_STANDBY:
            messageBase = IDS_SLEEPVETO_BASE;
            break;

        case VETOED_HIBERNATE:
            messageBase = IDS_HIBERNATEVETO_BASE;
            break;

        case VETOED_REMOVAL:
        case VETOED_EJECT:
        case VETOED_WARM_EJECT:
        default:
            messageBase = IDS_VETO_BASE;
            break;
    }

    switch(VetoedRemovalData->VetoType) {

        case PNP_VetoWindowsApp:

            if (pszVetoIds && pszVetoIds[0]) {

                //
                // Tell our user the name of the offending application.
                //
                LoadString(hHotPlug, messageBase+VetoedRemovalData->VetoType, szFormat, SIZECHARS(szFormat));
                wsprintf(szMessage, szFormat, pszVetoIds);

            } else {

                //
                // No application, use the "some app" message.
                //
                messageBase += (IDS_VETO_UNKNOWNWINDOWSAPP - IDS_VETO_WINDOWSAPP);
                LoadString(hHotPlug, messageBase+VetoedRemovalData->VetoType, szMessage, SIZECHARS(szMessage));
            }
            break;

        case PNP_VetoWindowsService:
        case PNP_VetoDriver:
        case PNP_VetoLegacyDriver:
            //
            // PNP_VetoDriver and PNP_VetoLegacyDriver should really be passed
            // through the service manager to get friendlier names.
            //

            LoadString(hHotPlug, messageBase+VetoedRemovalData->VetoType, szFormat, SIZECHARS(szFormat));
            wsprintf(szMessage, szFormat, pszVetoIds);
            break;

        case PNP_VetoDevice:
            if ((VetoedRemovalData->VetoedOperation == VETOED_WARM_UNDOCK) &&
               (!lstrcmp(pszVetoIds, pszDeviceId))) {

                messageBase += (IDS_DOCKVETO_WARM_EJECT - IDS_DOCKVETO_DEVICE);
            }

            //
            // Fall through.
            //

        case PNP_VetoLegacyDevice:
        case PNP_VetoPendingClose:
        case PNP_VetoOutstandingOpen:
        case PNP_VetoNonDisableable:
        case PNP_VetoIllegalDeviceRequest:
            //
            // Include the veto ID in the display output
            //
            LoadString(hHotPlug, messageBase+VetoedRemovalData->VetoType, szFormat, SIZECHARS(szFormat));

            CM_Locate_DevNode(&DevNode, pszVetoIds, CM_LOCATE_DEVNODE_NORMAL);
            if ((FriendlyName = BuildFriendlyName(DevNode, NULL)) != NULL) {

                wsprintf(szMessage, szFormat, FriendlyName);
                LocalFree(FriendlyName);

            } else {

                wsprintf(szMessage, szFormat, pszVetoIds);
            }

            break;

        case PNP_VetoInsufficientRights:

            //
            // Use the device itself in the display, but only if we are not
            // in the dock case.
            //

            if ((VetoedRemovalData->VetoedOperation == VETOED_UNDOCK)||
                (VetoedRemovalData->VetoedOperation == VETOED_WARM_UNDOCK)) {

                LoadString(hHotPlug, messageBase+VetoedRemovalData->VetoType, szMessage, SIZECHARS(szMessage));
                break;

            }

            //
            // Fall through.
            //

        case PNP_VetoInsufficientPower:
        case PNP_VetoTypeUnknown:

            //
            // Use the device itself in the display
            //
            LoadString(hHotPlug, messageBase+VetoedRemovalData->VetoType, szFormat, SIZECHARS(szFormat));

            CM_Locate_DevNode(&DevNode, pszDeviceId, CM_LOCATE_DEVNODE_NORMAL);
            if ((FriendlyName = BuildFriendlyName(DevNode, NULL)) != NULL) {

                wsprintf(szMessage, szFormat, FriendlyName);
                LocalFree(FriendlyName);

            } else {

                wsprintf(szMessage, szFormat, pszDeviceId);
            }

            break;

        default:
            ASSERT(0);
            LoadString(hHotPlug, messageBase+PNP_VetoTypeUnknown, szFormat, SIZECHARS(szFormat));
            CM_Locate_DevNode(&DevNode, pszDeviceId, CM_LOCATE_DEVNODE_NORMAL);
            if ((FriendlyName = BuildFriendlyName(DevNode, NULL)) != NULL) {

                wsprintf(szMessage, szFormat, FriendlyName);
                LocalFree(FriendlyName);

            } else {

                wsprintf(szMessage, szFormat, pszDeviceId);
            }

            break;
    }

    switch(VetoedRemovalData->VetoedOperation) {

        case VETOED_EJECT:
        case VETOED_WARM_EJECT:
            LoadString(hHotPlug, IDS_VETOED_EJECT_TITLE, szFormat, SIZECHARS(szFormat));
            break;

        case VETOED_UNDOCK:
        case VETOED_WARM_UNDOCK:
            LoadString(hHotPlug, IDS_VETOED_UNDOCK_TITLE, szFormat, SIZECHARS(szFormat));
            break;

        case VETOED_STANDBY:
            LoadString(hHotPlug, IDS_VETOED_STANDBY_TITLE, szFormat, SIZECHARS(szFormat));
            break;

        case VETOED_HIBERNATE:
            LoadString(hHotPlug, IDS_VETOED_HIBERNATION_TITLE, szFormat, SIZECHARS(szFormat));
            break;

        default:
            ASSERT(0);

            //
            // Fall through, display something at least...
            //

        case VETOED_REMOVAL:
            LoadString(hHotPlug, IDS_VETOED_REMOVAL_TITLE, szFormat, SIZECHARS(szFormat));
            break;
    }

    switch(VetoedRemovalData->VetoedOperation) {

        case VETOED_STANDBY:
        case VETOED_HIBERNATE:

            lstrcpy(szTitle, szFormat);
            break;

        case VETOED_EJECT:
        case VETOED_WARM_EJECT:
        case VETOED_UNDOCK:
        case VETOED_WARM_UNDOCK:
        case VETOED_REMOVAL:
        default:
            CM_Locate_DevNode(&DevNode, pszDeviceId, CM_LOCATE_DEVNODE_NORMAL);
            if ((FriendlyName = BuildFriendlyName(DevNode, NULL)) != NULL) {

                wsprintf(szTitle, szFormat, FriendlyName);
                LocalFree(FriendlyName);
            }

            else {

                wsprintf(szTitle, szFormat, pszVetoIds);
            }
            break;
    }

#if 0
    SetWindowText(hDlg, szTitle);
    SetDlgItemText(hDlg, IDC_VETOTEXT, szMessage);
    return TRUE;
#else
    MessageBox(hDlg, szMessage, szTitle, MB_OK | MB_ICONEXCLAMATION | MB_SETFOREGROUND | MB_TOPMOST);
    return FALSE;
#endif
}

LRESULT CALLBACK
DeviceRemovalVetoedDlgProc(
   HWND   hDlg,
   UINT   message,
   WPARAM wParam,
   LPARAM lParam
   )
/*++

Routine Description:

   DialogProc to tell the user the device removal was vetoed.

Arguments:

   standard stuff.

Return Value:

   LRESULT

--*/

{
    PVETO_DEVICE_COLLECTION_DATA VetoedRemovalData = NULL;
    BOOL Status = TRUE;

    if (message == WM_INITDIALOG) {

        VetoedRemovalData = (PVETO_DEVICE_COLLECTION_DATA) lParam;
        SetWindowLongPtr(hDlg, DWLP_USER, lParam);

        if (!InitVetoedRemovalDlgProc(hDlg, VetoedRemovalData)) {

            EndDialog(hDlg, IDABORT);
        }

        return TRUE;
    }

    //
    // retrieve private data from window long (stored there during WM_INITDIALOG)
    //
    VetoedRemovalData = (PVETO_DEVICE_COLLECTION_DATA)GetWindowLongPtr(hDlg, DWLP_USER);

    switch (message) {

        case WM_DESTROY:

            //
            // destroy the ClassImageList
            //
            if (VetoedRemovalData->ClassImageList.cbSize) {
                SetupDiDestroyClassImageList(&VetoedRemovalData->ClassImageList);
                VetoedRemovalData->ClassImageList.cbSize = 0;
            }
            break;

        case WM_CLOSE:
            SendMessage (hDlg, WM_COMMAND, IDCANCEL, 0L);
            break;

        case WM_COMMAND:

            switch(wParam) {

                case IDOK:
                case IDCLOSE:
                case IDCANCEL:
                    EndDialog(hDlg, wParam);
                    break;
            }
            break;

        case WM_HELP:
            OnContextHelp((LPHELPINFO)lParam, VetoedRemovalHelpIDs);
            break;

        case WM_CONTEXTMENU:
            WinHelp((HWND)wParam,
                    TEXT("hardware.hlp"),
                    HELP_CONTEXTMENU,
                    (DWORD_PTR)(LPVOID)(PDWORD)VetoedRemovalHelpIDs
                    );

            return FALSE;

        default:
            return FALSE;

    }

    return TRUE;
}

