//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       hotplug.c
//
//--------------------------------------------------------------------------

#include "HotPlug.h"
#include <regstr.h>
#include <systrayp.h>
#include <help.h>

TCHAR szUnknown[64];

TCHAR szHotPlugFlags[]=TEXT("HotPlugFlags");
TCHAR HOTPLUG_NOTIFY_CLASS_NAME[] = TEXT("HotPlugNotifyClass");


typedef int
(*PDEVICEPROPERTIES)(
    HWND hwndParent,
    LPTSTR MachineName,
    LPTSTR DeviceID,
    BOOL ShowDeviceTree
    );


//
// colors used to highlight removal relationships for selected device
//
COLORREF RemovalImageBkColor;
COLORREF NormalImageBkColor;
COLORREF RemovalTextColor;

HWND g_hwndNotify = NULL;
HMODULE hDevMgr=NULL;
PDEVICEPROPERTIES pDeviceProperties = NULL;

#define IDH_DISABLEHELP         ((DWORD)(-1))
#define IDH_hwwizard_devices_list       15301   //  (SysTreeView32)
#define idh_hwwizard_stop               15305   // "&Stop" (Button)
#define idh_hwwizard_display_components 15307   //  "&Display device components" (Button)
#define idh_hwwizard_show_icon          15308   // "Show &icon on taskbar" (Button)
#define idh_hwwizard_properties         15311   //  "&Properties" (Button)
#define idh_hwwizard_close              15309   //  "&Close" (Button)

DWORD UnplugtHelpIDs[] = {
    IDC_STOPDEVICE,    idh_hwwizard_stop,               // "&Stop" (Button)
    IDC_PROPERTIES,    idh_hwwizard_properties,         //  "&Properties" (Button)
    IDC_VIEWOPTION,    idh_hwwizard_display_components, //  "&Display device components" (Button)
    IDC_SYSTRAYOPTION, idh_hwwizard_show_icon,          // "Show &icon on taskbar" (Button)
    IDC_DEVICETREE,    IDH_hwwizard_devices_list,       // "" (SysTreeView32)
    IDCLOSE,           idh_hwwizard_close,
    IDC_MACHINENAME,   NO_HELP,
    IDC_HDWDEVICES,    NO_HELP,
    IDC_NOHELP1,       NO_HELP,
    IDC_NOHELP2,       NO_HELP,
    IDC_NOHELP3,       NO_HELP,
    IDC_DEVICEDESC,    NO_HELP,
    0,0
    };

void
OnRemoveDevice(
    HWND hDlg,
    PDEVICETREE DeviceTree,
    BOOL Eject
    )
{
    DEVINST DeviceInstance;
    CONFIGRET ConfigRet;
    HTREEITEM hTreeItem;
    PDEVTREENODE DeviceTreeNode;
    PWIZPAGE_OBJECT WizPageObject = NULL;
    PROPSHEETPAGE Page;
    PTCHAR DeviceName;

    DeviceTreeNode = DeviceTree->ChildRemovalList;

    if (!DeviceTreeNode) {

        return;
    }

    if (WizPageObject = malloc(sizeof(WIZPAGE_OBJECT))) {

        WizPageObject->RefCount = 0;
        WizPageObject->DeviceTree = DeviceTree;

    } else {

        return;
    }

    //
    // Since we're using the same code as the Add New Device Wizard, we
    // have to supply a LPROPSHEETPAGE as the lParam to the DialogProc.
    // (All we care about is the lParam field, and the DWORD at the end
    // of the buffer.)
    //
    Page.lParam = (LPARAM)WizPageObject;

    //
    // Confirm with the user that they really want
    // to remove this device and all of its attached devices.
    // The dialog returns standard IDOK, IDCANCEL etc. for results.
    // if anything besides IDOK  don't do anything.
    //

    DialogBoxParam(hHotPlug,
                   MAKEINTRESOURCE(DLG_CONFIRMREMOVE),
                   hDlg,
                   RemoveConfirmDlgProc,
                   (LPARAM)&Page
                   );

    free(WizPageObject);

    return;
}




void
OnTvnSelChanged(
   PDEVICETREE DeviceTree,
   NM_TREEVIEW *nmTreeView
   )
{
    PDEVTREENODE DeviceTreeNode = (PDEVTREENODE)(nmTreeView->itemNew.lParam);
    PTCHAR DeviceName, ProblemText;
    ULONG DevNodeStatus, Problem;
    CONFIGRET ConfigRet;
    TCHAR Buffer[MAX_PATH*2];

    if (DeviceTree->RedrawWait) {

        return;
    }


    //
    // Clear Removal list for previously selected Node
    //
    ClearRemovalList(DeviceTree);


    //
    // Save the selected treenode.
    //

    DeviceTree->SelectedTreeNode = DeviceTreeNode;

    //
    // No device is selected
    //
    if (!DeviceTreeNode) {

        if (DeviceTree->IsDialog) {

            EnableWindow(GetDlgItem(DeviceTree->hDlg, IDC_STOPDEVICE), FALSE);

        } else {

            PropSheet_SetWizButtons(GetParent(DeviceTree->hDlg), PSWIZB_BACK);
        }

        EnableWindow(GetDlgItem(DeviceTree->hDlg, IDC_PROPERTIES), FALSE);

        SetDlgItemText(DeviceTree->hDlg, IDC_DEVICEDESC, TEXT(""));
        return;
    }

    //
    // reset the text for the selected item
    //

    DeviceName = FetchDeviceName(DeviceTreeNode);

    if (!DeviceName) {

        DeviceName = szUnknown;
    }

    wsprintf(Buffer,
             TEXT("%s %s"),
             DeviceName,
             DeviceTreeNode->Location  ? DeviceTreeNode->Location : TEXT("")
             );

    SetDlgItemText(DeviceTree->hDlg, IDC_DEVICEDESC, Buffer);



    //
    // Turn on the stop\eject button, and set text accordingly.
    //

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

    //
    // Any removable (but not surprise removable) device is OK, except
    // if the user already removed it.
    //
    if (Problem != CM_PROB_DEVICE_NOT_THERE) {

        if (DeviceTree->IsDialog) {

            EnableWindow(GetDlgItem(DeviceTree->hDlg, IDC_STOPDEVICE), TRUE);

        } else {

            PropSheet_SetWizButtons(GetParent(DeviceTree->hDlg), PSWIZB_BACK | PSWIZB_NEXT);
        }

    } else {

        if (DeviceTree->IsDialog) {

            EnableWindow(GetDlgItem(DeviceTree->hDlg, IDC_STOPDEVICE), FALSE);

        } else {

            PropSheet_SetWizButtons(GetParent(DeviceTree->hDlg), PSWIZB_BACK);
        }
    }

    EnableWindow(GetDlgItem(DeviceTree->hDlg, IDC_PROPERTIES), TRUE);

    //
    // reset the overlay icons if device state has changed
    //
    if (DeviceTreeNode->Problem != Problem || DeviceTreeNode->DevNodeStatus != DevNodeStatus) {

        TV_ITEM tv;

        tv.mask = TVIF_STATE;
        tv.stateMask = TVIS_OVERLAYMASK;
        tv.hItem = DeviceTreeNode->hTreeItem;

        if (DeviceTreeNode->Problem == CM_PROB_DISABLED) {

            tv.state = INDEXTOOVERLAYMASK(IDI_DISABLED_OVL - IDI_CLASSICON_OVERLAYFIRST + 1);

        } else if (DeviceTreeNode->Problem || !(DeviceTreeNode->DevNodeStatus & DN_STARTED)) {

            tv.state = INDEXTOOVERLAYMASK(IDI_PROBLEM_OVL - IDI_CLASSICON_OVERLAYFIRST + 1);

        } else {

            tv.state = INDEXTOOVERLAYMASK(0);
        }

        TreeView_SetItem(DeviceTree->hwndTree, &tv);
    }


    //
    // Starting from the TopLevel removal node, build up the removal lists
    //
    DeviceTreeNode = TopLevelRemovalNode(DeviceTree, DeviceTreeNode);

    //
    // Add devices to ChildRemoval list
    //
    DeviceTree->ChildRemovalList = DeviceTreeNode;
    DeviceTreeNode->NextChildRemoval = DeviceTreeNode;
    InvalidateTreeItemRect(DeviceTree->hwndTree, DeviceTreeNode->hTreeItem);
    AddChildRemoval(DeviceTree, &DeviceTreeNode->ChildSiblingList);

    //
    // Add eject amd removal relations
    //
    AddEjectToRemoval(DeviceTree);
}




int
OnCustomDraw(
   HWND hDlg,
   PDEVICETREE DeviceTree,
   LPNMTVCUSTOMDRAW nmtvCustomDraw
   )
{
   PDEVTREENODE DeviceTreeNode = (PDEVTREENODE)(nmtvCustomDraw->nmcd.lItemlParam);

   if (nmtvCustomDraw->nmcd.dwDrawStage == CDDS_PREPAINT) {
       return CDRF_NOTIFYITEMDRAW;
       }

   //
   // If this node is in the Removal list, then do special
   // highlighting.
   //

   if (DeviceTreeNode->NextChildRemoval) {

       //
       // set text color if its not the selected item
       //

       if (DeviceTree->SelectedTreeNode != DeviceTreeNode) {
           nmtvCustomDraw->clrText = RemovalTextColor;
           }

        //
        // Highlight the image-icon background
        //

        ImageList_SetBkColor(DeviceTree->ClassImageList.ImageList,
                             RemovalImageBkColor
                             );
        }
   else  {

        //
        // Normal image-icon background
        //

        ImageList_SetBkColor(DeviceTree->ClassImageList.ImageList,
                             NormalImageBkColor
                             );
        }

   return CDRF_DODEFAULT;
}



void
OnSysColorChange(
   HWND hDlg,
   PDEVICETREE DeviceTree
   )
{
   COLORREF ColorWindow, ColorHighlight;
   BYTE Red, Green, Blue;

   //
   // Fetch the colors used for removal highlighting
   //

   ColorWindow = GetSysColor(COLOR_WINDOW);
   ColorHighlight = GetSysColor(COLOR_HIGHLIGHT);

   Red = (BYTE)(((WORD)GetRValue(ColorWindow) + (WORD)GetRValue(ColorHighlight)) >> 1);
   Green = (BYTE)(((WORD)GetGValue(ColorWindow) + (WORD)GetGValue(ColorHighlight)) >> 1);
   Blue = (BYTE)(((WORD)GetBValue(ColorWindow) + (WORD)GetBValue(ColorHighlight)) >> 1);

   RemovalImageBkColor = RGB(Red, Green, Blue);
   RemovalTextColor = ColorHighlight;
   NormalImageBkColor = ColorWindow;


   // Update the ImageList Background color
   if (DeviceTree->ClassImageList.cbSize) {
       ImageList_SetBkColor(DeviceTree->ClassImageList.ImageList,
                            ColorWindow
                            );
       }
}


void
OnTvnItemExpanding(
   HWND hDlg,
   PDEVICETREE DeviceTree,
   NM_TREEVIEW *nmTreeView

   )
{
   PDEVTREENODE DeviceTreeNode = (PDEVTREENODE)(nmTreeView->itemNew.lParam);

   //
   // don't allow collapse of root items with children
   //

   if (!DeviceTreeNode->ParentNode &&
       (nmTreeView->action == TVE_COLLAPSE ||
        nmTreeView->action == TVE_COLLAPSERESET ||
        (nmTreeView->action == TVE_TOGGLE &&
         (nmTreeView->itemNew.state & TVIS_EXPANDED))) )
     {
       SetDlgMsgResult(hDlg, WM_NOTIFY, TRUE);
       }
   else {
       SetDlgMsgResult(hDlg, WM_NOTIFY, FALSE);
       }
}




void
OnContextMenu(
   HWND hDlg,
   PDEVICETREE DeviceTree
   )
{
   int IdCmd;
   ULONG Len, DevNodeStatus, Problem;
   CONFIGRET ConfigRet;
   POINT ptPopup;
   RECT rect;
   HMENU hMenu;
   PDEVTREENODE DeviceTreeNode;
   TCHAR Buffer[MAX_PATH];

   DeviceTreeNode = DeviceTree->SelectedTreeNode;
   if (!DeviceTreeNode) {
       return;
       }

   TreeView_GetItemRect(DeviceTree->hwndTree,
                        DeviceTreeNode->hTreeItem,
                        &rect,
                        TRUE
                        );

   ptPopup.x = (rect.left+rect.right)/2;
   ptPopup.y = (rect.top+rect.bottom)/2;
   ClientToScreen(DeviceTree->hwndTree, &ptPopup);


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



   hMenu = CreatePopupMenu();
   if (!hMenu) {
       return;
       }


   //
   // if device is running add stop item
   //

   if (DevNodeStatus & DN_STARTED) {
       LoadString(hHotPlug,
                  IDS_STOP,
                  Buffer,
                  SIZECHARS(Buffer)
                  );

       AppendMenu(hMenu, MF_STRING, IDC_STOPDEVICE, Buffer);
       }


   //
   // add Properties item (link to device mgr).
   //

   LoadString(hHotPlug,
              IDS_PROPERTIES,
              Buffer,
              SIZECHARS(Buffer)
              );

   AppendMenu(hMenu, MF_STRING, IDC_PROPERTIES, Buffer);


   IdCmd = TrackPopupMenu(hMenu,
                          TPM_RETURNCMD | TPM_RIGHTBUTTON | TPM_LEFTALIGN | TPM_NONOTIFY,
                          ptPopup.x,
                          ptPopup.y,
                          0,
                          hDlg,
                          NULL
                          );

   DestroyMenu(hMenu);

   if (!IdCmd) {
       return;
       }


   switch(IdCmd) {
      case IDC_STOPDEVICE:
          OnRemoveDevice(hDlg, DeviceTree, FALSE);
          break;

      case IDC_PROPERTIES: {
          if (pDeviceProperties) {
              (*pDeviceProperties)(
                    hDlg,
                    DeviceTree->hMachine ? DeviceTree->MachineName : NULL,
                    DeviceTreeNode->InstanceId,
                    FALSE
                    );
              }
          }

          break;
      }


   return;

}



void
OnRightClick(
    HWND hDlg,
    PDEVICETREE DeviceTree,
    NMHDR * nmhdr
    )
{
    DWORD dwPos;
    TV_ITEM tvi;
    TV_HITTESTINFO tvht;
    PDEVTREENODE DeviceTreeNode;

    if (nmhdr->hwndFrom != DeviceTree->hwndTree) {
        return;
        }

    dwPos = GetMessagePos();

    tvht.pt.x = LOWORD(dwPos);
    tvht.pt.y = HIWORD(dwPos);

    ScreenToClient(DeviceTree->hwndTree, &tvht.pt);
    tvi.hItem = TreeView_HitTest(DeviceTree->hwndTree, &tvht);
    if (!tvi.hItem) {
        return;
        }


    tvi.mask = TVIF_PARAM;
    if (!TreeView_GetItem(DeviceTree->hwndTree, &tvi)) {
        return;
        }


    DeviceTreeNode = (PDEVTREENODE)tvi.lParam;
    if (!DeviceTreeNode) {
        return;
        }

    //
    // Make the current right click item, the selected item
    //
    if (DeviceTreeNode != DeviceTree->SelectedTreeNode) {
        TreeView_SelectItem(DeviceTree->hwndTree, DeviceTreeNode->hTreeItem);
        }

}





void
OnViewOptionClicked(
    HWND hDlg,
    PDEVICETREE DeviceTree
    )
{
    BOOL bChecked;
    DWORD HotPlugFlags, NewFlags;
    HKEY hKey;
    PDEVTREENODE DeviceTreeNode;
    DEVINST DeviceInstance;
    HTREEITEM hTreeItem;
    TV_ITEM TvItem;



    //
    // checked means "show complex view"
    //

    bChecked = IsDlgButtonChecked(hDlg, IDC_VIEWOPTION);


    //
    // Update HotPlugs registry if needed.
    //

    NewFlags = HotPlugFlags = GetHotPlugFlags(&hKey);
    if (bChecked) {
        NewFlags |= HOTPLUG_REGFLAG_VIEWALL;
        }
    else {
        NewFlags &= ~HOTPLUG_REGFLAG_VIEWALL;
        }

    if (NewFlags != HotPlugFlags) {
        RegSetValueEx(hKey,
                      szHotPlugFlags,
                      0,
                      REG_DWORD,
                      (PVOID)&NewFlags,
                      sizeof(NewFlags)
                      );
        }

    if (hKey) {
        RegCloseKey(hKey);
        }


    if (!DeviceTree->ComplexView && bChecked) {
        DeviceTree->ComplexView = TRUE;
        }
    else if (DeviceTree->ComplexView && !bChecked) {
        DeviceTree->ComplexView = FALSE;
        }
    else {
        // we are in the correct state, nothing to do.
        return;
        }

    //
    // redraw the entire tree.
    //
    RefreshTree(DeviceTree);

    return;
}


void
OnSystrayOptionClicked(
    HWND hDlg
    )
{
    BOOL bChecked;


    //
    // checked means "Show on Systray"
    //

    bChecked = IsDlgButtonChecked(hDlg, IDC_SYSTRAYOPTION);

    SysTray_EnableService(STSERVICE_HOTPLUG, bChecked);

}

LRESULT
hotplugNotifyWndProc(
    HWND hWnd,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    HWND hMainWnd;
    hMainWnd = (HWND)GetWindowLongPtr(hWnd, GWLP_USERDATA);

    switch (uMsg)
    {
    case WM_CREATE:
    {
        hMainWnd =  (HWND)((CREATESTRUCT*)lParam)->lpCreateParams;
        SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)hMainWnd);
        break;
    }
    
    case WM_DEVICECHANGE:
    {
        if (DBT_DEVNODES_CHANGED == wParam)
        {
            // While we are in WM_DEVICECHANGE context,
            // no CM apis can be called because it would
            // deadlock. Here, we schedule a timer so that
            // we can handle the message later on.
            SetTimer(hMainWnd, TIMERID_DEVICECHANGE, 1000, NULL);
        }

        break;
    }
    
    default:
        break;
    }
    
    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}


BOOL
CreateNotifyWindow(
    HWND hWnd
    )
{
    WNDCLASS wndClass;

    if (!GetClassInfo(hHotPlug, HOTPLUG_NOTIFY_CLASS_NAME, &wndClass)) {

        memset(&wndClass, 0, sizeof(wndClass));
        wndClass.lpfnWndProc = hotplugNotifyWndProc;
        wndClass.hInstance = hHotPlug;
        wndClass.lpszClassName = HOTPLUG_NOTIFY_CLASS_NAME;

        if (!RegisterClass(&wndClass)) {

            return FALSE;
        }
    }

    g_hwndNotify = CreateWindowEx(WS_EX_TOOLWINDOW,
                                  HOTPLUG_NOTIFY_CLASS_NAME,
                                  TEXT(""),
                                  WS_DLGFRAME | WS_BORDER | WS_DISABLED,
                                  CW_USEDEFAULT,
                                  CW_USEDEFAULT,
                                  0,
                                  0,
                                  NULL,
                                  NULL,
                                  hHotPlug,
                                  (LPVOID)hWnd
                                  );

    return (NULL != g_hwndNotify);
}

BOOL
InitDevTreeDlgProc(
      HWND hDlg,
      PDEVICETREE DeviceTree
      )
{
    CONFIGRET ConfigRet;
    HWND hwndTree;
    HTREEITEM hTreeItem;
    DEVINST DeviceInstance;
    DWORD HotPlugFlags;
    HICON hIcon;
    HWND hwndParent;

    if (DeviceTree->IsDialog) {
    
        DeviceTree->AllowRefresh = TRUE;
    }

    CreateNotifyWindow(hDlg);



    
    hDevMgr = LoadLibrary(TEXT("devmgr.dll"));

    if (hDevMgr) {

        pDeviceProperties = (PVOID)GetProcAddress(hDevMgr, "DevicePropertiesW");
    }

    if (DeviceTree->IsDialog) {

        hIcon = LoadIcon(hHotPlug,MAKEINTRESOURCE(IDI_HOTPLUGICON));
        if (hIcon) {

            SendMessage(hDlg, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
            SendMessage(hDlg, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
        }
    }

    hwndParent = GetParent(hDlg);

    if (DeviceTree->IsDialog) {

        if (hwndParent) {

            SendMessage(hwndParent, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
            SendMessage(hwndParent, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
        }
    }

    DeviceTree->hDlg     = hDlg;
    DeviceTree->hwndTree = hwndTree = GetDlgItem(hDlg, IDC_DEVICETREE);

    LoadString(hHotPlug,
               IDS_UNKNOWN,
               (PTCHAR)szUnknown,
               SIZECHARS(szUnknown)
               );
    
    //
    // Only do the following code if we are in dialog mode
    //
    if (DeviceTree->IsDialog) {

        if (DeviceTree->hMachine) {

            SetDlgItemText(hDlg, IDC_MACHINENAME, DeviceTree->MachineName);
        }

        if (SysTray_IsServiceEnabled(STSERVICE_HOTPLUG)) {

            CheckDlgButton(hDlg, IDC_SYSTRAYOPTION, BST_CHECKED);

        } else {

            CheckDlgButton(hDlg, IDC_SYSTRAYOPTION, BST_UNCHECKED);
        }

        //
        // Disable the Stop button, until an item is selected.
        //
        EnableWindow(GetDlgItem(DeviceTree->hDlg, IDC_STOPDEVICE), FALSE);
    }

    EnableWindow(GetDlgItem(DeviceTree->hDlg, IDC_PROPERTIES), FALSE);

    // Get the Class Icon Image Lists
    DeviceTree->ClassImageList.cbSize = sizeof(SP_CLASSIMAGELIST_DATA);
    if (SetupDiGetClassImageList(&DeviceTree->ClassImageList)) {

        TreeView_SetImageList(hwndTree, DeviceTree->ClassImageList.ImageList, TVSIL_NORMAL);

    } else {

        DeviceTree->ClassImageList.cbSize = 0;
    }


    // initialize the removal colors
    OnSysColorChange(hDlg, DeviceTree);



    if (DeviceTree->IsDialog) {

        HotPlugFlags = GetHotPlugFlags(NULL);
        if (HotPlugFlags & HOTPLUG_REGFLAG_VIEWALL) {

            DeviceTree->ComplexView = TRUE;
            CheckDlgButton(hDlg, IDC_VIEWOPTION, BST_CHECKED);

        } else {

            DeviceTree->ComplexView = FALSE;
            CheckDlgButton(hDlg, IDC_VIEWOPTION, BST_UNCHECKED);
        }

        //
        // Get the root devnode.
        //
        ConfigRet = CM_Locate_DevNode_Ex(&DeviceTree->DevInst,
                                         NULL,
                                         CM_LOCATE_DEVNODE_NORMAL,
                                         DeviceTree->hMachine
                                         );

        if (ConfigRet != CR_SUCCESS) {

            return FALSE;
        }

        RefreshTree(DeviceTree);


        if (DeviceTree->EjectDeviceInstanceId) {

            CONFIGRET ConfigRet;
            DEVINST EjectDevInst;
            PDEVTREENODE DeviceTreeNode;

            //
            // we are removing a specific device, find it
            // and post a message to trigger device removal.
            //
            ConfigRet = CM_Locate_DevNode_Ex(&EjectDevInst,
                                             DeviceTree->EjectDeviceInstanceId,
                                             CM_LOCATE_DEVNODE_NORMAL,
                                             DeviceTree->hMachine
                                             );


            if (ConfigRet != CR_SUCCESS) {

                return FALSE;
            }

            DeviceTreeNode = DevTreeNodeByDevInst(EjectDevInst,
                                                  &DeviceTree->ChildSiblingList
                                                  );

            if (!DeviceTreeNode) {

                return FALSE;
            }

            TreeView_SelectItem(hwndTree, DeviceTreeNode->hTreeItem);
            PostMessage(hDlg, WUM_EJECTDEVINST, 0, 0);

        } else {

            ShowWindow(hDlg, SW_SHOW);
        }
    }

    return TRUE;
}

BOOL
DevTreeDlgProcSetActive(
    HWND hDlg,
    PDEVICETREE DeviceTree
    )
{
    DWORD HotPlugFlags;
    CONFIGRET ConfigRet;

    DeviceTree->AllowRefresh = TRUE;

    HotPlugFlags = GetHotPlugFlags(NULL);
    if (HotPlugFlags & HOTPLUG_REGFLAG_VIEWALL) {

        DeviceTree->ComplexView = TRUE;
        CheckDlgButton(hDlg, IDC_VIEWOPTION, BST_CHECKED);

    } else {

        DeviceTree->ComplexView = FALSE;
        CheckDlgButton(hDlg, IDC_VIEWOPTION, BST_UNCHECKED);
    }

    //
    // Don't show the next button until an item is selected
    //
    PropSheet_SetWizButtons(GetParent(hDlg), PSWIZB_BACK);

    //
    // Get the root devnode.
    //
    ConfigRet = CM_Locate_DevNode_Ex(&DeviceTree->DevInst,
                                     NULL,
                                     CM_LOCATE_DEVNODE_NORMAL,
                                     DeviceTree->hMachine
                                     );

    if (ConfigRet != CR_SUCCESS) {

        return FALSE;
    }

    RefreshTree(DeviceTree);

    return TRUE;
}


void
OnContextHelp(
  LPHELPINFO HelpInfo,
  PDWORD ContextHelpIDs
  )
{
  // Define an array of dword pairs,
  // where the first of each pair is the control ID,
  // and the second is the context ID for a help topic,
  // which is used in the help file.

  if (HelpInfo->iContextType == HELPINFO_WINDOW) {  // must be for a control

      WinHelp(HelpInfo->hItemHandle,
              TEXT("hardware.hlp"),
              HELP_WM_HELP,
              (DWORD_PTR)(LPVOID)ContextHelpIDs
              );
      }

}

PDEVICETREE
GetDeviceTreeFromPsPage(
    LPPROPSHEETPAGE ppsp
    )
/*++

Routine Description:

    This routine retrieves a pointer to a PDEVICETREE structure to be used by a
    wizard page dialog proc.  It is called during the WM_INITDIALOG handling.

Arguments:

    Page - Property sheet page structure for this wizard page.

Return Value:

    If success, a pointer to the structure, NULL otherwise.

--*/
{
    PWIZPAGE_OBJECT CurWizObject = NULL;

    try {
        //
        // The ObjectID (pointer, actually) for the corresponding wizard
        // object for this page is stored at the end of the ppsp structure.
        // Retrieve this now, and look for it in the devinfo set's list of
        // wizard objects.
        //
        CurWizObject = (PWIZPAGE_OBJECT)ppsp->lParam;

    } except(EXCEPTION_EXECUTE_HANDLER) {
        ;   // nothing to do
    }

    return CurWizObject ? CurWizObject->DeviceTree : NULL;
}

LRESULT CALLBACK
DevTreeDlgProc(
   HWND   hDlg,
   UINT   message,
   WPARAM wParam,
   LPARAM lParam
   )
/*++

Routine Description:

   DialogProc which displays the device tree in a view by connection.

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

            InitDevTreeDlgProc(hDlg, DeviceTree);
        }

        return TRUE;
    }

    //
    // retrieve private data from window long (stored there during WM_INITDIALOG)
    //
    DeviceTree = (PDEVICETREE)GetWindowLongPtr(hDlg, DWLP_USER);


    switch (message) {

    case WM_DESTROY:
        //
        // Destroy the Notification Window
        //
        if (g_hwndNotify && IsWindow(g_hwndNotify)) {
            DestroyWindow(g_hwndNotify);
            g_hwndNotify = NULL;
        }

        //
        // Clear the DeviceTree
        //
        TreeView_DeleteAllItems(DeviceTree->hwndTree);

        //
        // clean up the class image list.
        //
        if (DeviceTree->ClassImageList.cbSize) {

            SetupDiDestroyClassImageList(&DeviceTree->ClassImageList);
            DeviceTree->ClassImageList.cbSize = 0;
        }

        //
        // Clean up the device tree
        //
        ClearRemovalList(DeviceTree);
        RemoveChildSiblings(DeviceTree, &DeviceTree->ChildSiblingList);

        if (hDevMgr) {

            FreeLibrary(hDevMgr);
            hDevMgr=NULL;
            pDeviceProperties = NULL;
        }

        break;


    case WM_CLOSE:
        SendMessage (hDlg, WM_COMMAND, IDCANCEL, 0L);
        break;




    case WM_COMMAND: {
        UINT Control = GET_WM_COMMAND_ID(wParam, lParam);
        UINT Cmd = GET_WM_COMMAND_CMD(wParam, lParam);

        switch(Control) {
            case IDC_VIEWOPTION:
                if (Cmd == BN_CLICKED) {
                   
                    OnViewOptionClicked(hDlg, DeviceTree);
                }
                break;

            case IDC_SYSTRAYOPTION:
                if (Cmd == BN_CLICKED) {
                   
                    OnSystrayOptionClicked(hDlg);
                }
                break;

            case IDC_STOPDEVICE:
                OnRemoveDevice(hDlg, DeviceTree, FALSE);
                break;

            case IDOK:  // enter -> default  to expand\collapse the selected tree node
                if (DeviceTree->SelectedTreeNode) {
                   
                    TreeView_Expand(DeviceTree->hwndTree,
                                   DeviceTree->SelectedTreeNode->hTreeItem,
                                   TVE_TOGGLE
                                   );
                }

                break;

            case IDC_PROPERTIES:
                if (DeviceTree->SelectedTreeNode && pDeviceProperties) {
                   
                        (*pDeviceProperties)(
                            hDlg,
                            DeviceTree->hMachine ? DeviceTree->MachineName : NULL,
                            DeviceTree->SelectedTreeNode->InstanceId,
                            FALSE
                            );
                }
                break;

            case IDCLOSE:
            case IDCANCEL:
                EndDialog(hDlg, IDCANCEL);
                break;
            }

        }
        break;


    // Listen for Tree notifications
    case WM_NOTIFY:
        switch (((NMHDR FAR *)lParam)->code) {

        case PSN_SETACTIVE:
            DevTreeDlgProcSetActive(hDlg, DeviceTree);
            break;

        case PSN_KILLACTIVE:
            DeviceTree->AllowRefresh = FALSE;
            break;

        case PSN_WIZNEXT:
            break;

        case PSN_WIZBACK:
            break;

        case TVN_SELCHANGED:
            OnTvnSelChanged(DeviceTree, (NM_TREEVIEW *)lParam);
            break;


        case TVN_ITEMEXPANDING:
            OnTvnItemExpanding(hDlg,DeviceTree,(NM_TREEVIEW *)lParam);
            break;

        case TVN_KEYDOWN: {
            TV_KEYDOWN *tvKeyDown = (TV_KEYDOWN *)lParam;

            //
            // Only do this in the dialog case
            //
            if (DeviceTree->IsDialog && (tvKeyDown->wVKey == VK_DELETE)) {

                OnRemoveDevice(hDlg, DeviceTree, TRUE);
            }
        }
            break;


        case NM_CUSTOMDRAW:
            SetDlgMsgResult(hDlg, WM_NOTIFY, OnCustomDraw(hDlg,DeviceTree,(NMTVCUSTOMDRAW *)lParam));
            break;

        case NM_RETURN:
            //
            // we don't get this in a dialog, see IDOK
            //
            break;

        case NM_DBLCLK:
            //
            // Only do this in the dialog case
            //
            if (DeviceTree->IsDialog) {
                OnRemoveDevice(hDlg, DeviceTree, TRUE);
                SetDlgMsgResult(hDlg, WM_NOTIFY, TRUE);
            }
            break;

        case NM_RCLICK:
            OnRightClick(hDlg,DeviceTree, (NMHDR *)lParam);
            break;

        default:
            return FALSE;
        }
        break;

    case WUM_EJECTDEVINST:
        OnRemoveDevice(hDlg, DeviceTree, TRUE);
        EndDialog(hDlg, IDCANCEL);
        break;

    case WM_SYSCOLORCHANGE:
        HotPlugPropagateMessage(hDlg, message, wParam, lParam);
        OnSysColorChange(hDlg,DeviceTree);
        break;

    case WM_TIMER:
        if (TIMERID_DEVICECHANGE == wParam) {

            KillTimer(hDlg, TIMERID_DEVICECHANGE);
            DeviceTree->RefreshEvent = TRUE;
            
            if (DeviceTree->AllowRefresh) {
            
                OnTimerDeviceChange(DeviceTree);
            }
        }
        break;
    
    case WM_SETCURSOR:
        if (DeviceTree->RedrawWait || DeviceTree->RefreshEvent) {

            SetCursor(LoadCursor(NULL, IDC_WAIT));
            SetWindowLongPtr(hDlg, DWLP_MSGRESULT, 1);
            break;
        }

        return FALSE;


    case WM_CONTEXTMENU:
        //
        // handle kbd- shift-F10, mouse rclick is invoked from NM_RCLICK
        //
        if ((HWND)wParam == DeviceTree->hwndTree) {
            OnContextMenu(hDlg, DeviceTree);
            break;
            }
        else {
            WinHelp((HWND)wParam,
                    TEXT("hardware.hlp"),
                    HELP_CONTEXTMENU,
                    (DWORD_PTR)(LPVOID)(PDWORD)UnplugtHelpIDs
                    );
            }

        return FALSE;

    case WM_HELP:
        OnContextHelp((LPHELPINFO)lParam, (PDWORD)UnplugtHelpIDs);
        break;

    default:
        return FALSE;

    }

    return TRUE;
}

UINT
CALLBACK
DevTreePropSheetCallback(
    IN HWND hwnd,
    IN UINT uMsg,
    IN LPPROPSHEETPAGE ppsp
    )
/*++

Routine Description:

    This routine is called when the Select Device wizard page is created or
destroyed.

Arguments:

    hwnd - Reserved

    uMsg - Action flag, either PSPCB_CREATE or PSPCB_RELEASE

    ppsp - Supplies the address of the PROPSHEETPAGE structure being created
or destroyed.

Return Value:

    If uMsg is PSPCB_CREATE, then return non-zero to allow the page to be
created, or
    zero to prevent it.

    if uMsg is PSPCB_RELEASE, the return value is ignored.

--*/
{
    UINT ret;
    PWIZPAGE_OBJECT CurWizObject;

    ret = TRUE;

    try {
        //
        // The ObjectID (pointer, actually) for the corresponding wizard
        // object for this page is stored at the end of the ppsp structure.
        //
        CurWizObject = (PWIZPAGE_OBJECT)ppsp->lParam;

        if(!CurWizObject) {

            ret = FALSE;
            goto clean0;
        }

        switch(uMsg) {

            case PSPCB_CREATE :
                CurWizObject->RefCount++;
                break;

            case PSPCB_RELEASE :
                //
                // Decrement the wizard object refcount.  If it goes to zero (or if it
                // already was zero because we never got a PSPCB_CREATE message), then
                // remove the object from the linked list, and free all associated memory.
                //
                if(CurWizObject->RefCount) {

                    CurWizObject->RefCount--;

                    if(!CurWizObject->RefCount) {

                        free(CurWizObject->DeviceTree);
                        free(CurWizObject);
                    }
                }
        }

clean0: ;   // nothing to do

    } except(EXCEPTION_EXECUTE_HANDLER) {
        ret = FALSE;
    }

    return ret;
}


BOOL
WINAPI
HotPlugGetWizardPages(
    PSP_NEWDEVICEWIZARD_DATA DeviceWizardData
    )
/*++

Routine Description:


Arguments:

    InstallWizardData -

    DynamicPages - Fills in the wizard pages for hot unplugging a device.

Return Value:


Remarks:

    This function creates three wizard pages, the list of hot unpluggable devices page,
    the confirmation page, and the finish (success/error) page.

--*/
{
    PDEVICETREE DeviceTree;
    PWIZPAGE_OBJECT WizPageObject = NULL;

    PROPSHEETPAGE Page;

    if (!(DeviceTree = malloc(sizeof(DEVICETREE)))) {

        return FALSE;
    }

    memset(DeviceTree, 0, sizeof(DEVICETREE));

    DeviceTree->Size = sizeof(DEVICETREE);
    DeviceTree->HotPlugTree = TRUE;
    DeviceTree->IsDialog = FALSE;
    DeviceTree->HideUI = FALSE;
    InitializeListHead(&DeviceTree->ChildSiblingList);

    DeviceWizardData->NumDynamicPages = 0;

    if (WizPageObject = malloc(sizeof(WIZPAGE_OBJECT))) {

        WizPageObject->RefCount = 0;
        WizPageObject->DeviceTree = DeviceTree;

    } else {

        return FALSE;
    }

    Page.dwSize = sizeof(PROPSHEETPAGE);
    Page.hInstance = hHotPlug;
    Page.lParam = (LPARAM)WizPageObject;
    Page.dwFlags = PSP_DEFAULT | PSP_USETITLE | PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE | PSP_USECALLBACK;
    Page.pszTitle = TEXT("");
    Page.pszHeaderTitle = MAKEINTRESOURCE(IDS_HOTPLUGWIZ_DEVTREE);
    Page.pszHeaderSubTitle = MAKEINTRESOURCE(IDS_HOTPLUGWIZ_DEVTREE_INFO);
    Page.pfnCallback = DevTreePropSheetCallback;
    Page.pszTemplate = MAKEINTRESOURCE(IDD_DYNAWIZ_DEVTREE);
    Page.pfnDlgProc = DevTreeDlgProc;

    if (!(DeviceWizardData->DynamicPages[DeviceWizardData->NumDynamicPages++] = CreatePropertySheetPage(&Page))) {

        free(WizPageObject);
        return FALSE;
    }

    Page.pszHeaderTitle = MAKEINTRESOURCE(IDS_HOTPLUGWIZ_CONFIRMSTOP);
    Page.pszHeaderSubTitle = MAKEINTRESOURCE(IDS_HOTPLUGWIZ_CONFIRMSTOP_INFO);
    Page.pszTemplate = MAKEINTRESOURCE(IDD_DYNAWIZ_CONFIRMREMOVE);
    Page.pfnDlgProc = RemoveConfirmDlgProc;

    if (!(DeviceWizardData->DynamicPages[DeviceWizardData->NumDynamicPages++] = CreatePropertySheetPage(&Page))) {

        free(WizPageObject);
        return FALSE;
    }

    //
    // Finish page
    //
    Page.dwFlags = PSP_DEFAULT | PSP_USETITLE | PSP_HIDEHEADER;
    Page.pszTemplate = MAKEINTRESOURCE(IDD_DYNAWIZ_FINISH);
    Page.pfnDlgProc = RemoveFinishDlgProc;

    if (!(DeviceWizardData->DynamicPages[DeviceWizardData->NumDynamicPages++] = CreatePropertySheetPage(&Page))) {

        free(WizPageObject);
        return FALSE;
    }

    return TRUE;
}

