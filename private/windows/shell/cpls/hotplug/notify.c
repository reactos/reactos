//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       notify.c
//
//--------------------------------------------------------------------------

#include "HotPlug.h"
#include <regstr.h>
#include <systrayp.h>

void
OnTimerDeviceChange(
    PDEVICETREE DeviceTree
    )
{
    //
    // if a refresh event is pending, rebuild the entire tree.
    //

    if (DeviceTree->RefreshEvent) {
        
        if (RefreshTree(DeviceTree)) {
            
            DeviceTree->RefreshEvent = FALSE;
        }
    }
}

BOOL
RefreshTree(
    PDEVICETREE DeviceTree
    )
{
    CONFIGRET ConfigRet;
    DEVINST DeviceInstance;
    DEVINST SelectedDevInst;
    PDEVTREENODE DevTreeNode;
    HTREEITEM hTreeItem;
    HCURSOR hCursor;

    if (DeviceTree->RedrawWait) {
        
        DeviceTree->RefreshEvent = TRUE;
        SetTimer(DeviceTree->hDlg, TIMERID_DEVICECHANGE,1000,NULL);
        return FALSE;
    }


    hCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));
    DeviceTree->RedrawWait = TRUE;
    SendMessage(DeviceTree->hwndTree, WM_SETREDRAW, FALSE, 0L);

    SelectedDevInst = DeviceTree->SelectedTreeNode
                          ? DeviceTree->SelectedTreeNode->DevInst
                          : 0;


    ClearRemovalList(DeviceTree);
    TreeView_DeleteAllItems(DeviceTree->hwndTree);
    RemoveChildSiblings(DeviceTree, &DeviceTree->ChildSiblingList);

    ConfigRet = CM_Get_Child_Ex(&DeviceInstance,
                                DeviceTree->DevInst,
                                0,
                                DeviceTree->hMachine
                                );

    if (ConfigRet == CR_SUCCESS) {

        AddChildSiblings(DeviceTree,
                         NULL,
                         DeviceInstance,
                         0,
                         TRUE
                         );
    }


    DisplayChildSiblings(DeviceTree,
                         &DeviceTree->ChildSiblingList,
                         NULL,
                         FALSE
                         );

    //
    // restore treeview redraw state, and reset the selected item
    //

    DevTreeNode = DevTreeNodeByDevInst(SelectedDevInst,
                                       &DeviceTree->ChildSiblingList
                                       );

    if (DevTreeNode) {
        
        hTreeItem = DevTreeNode->hTreeItem;
    }

    else {
        
        hTreeItem = NULL;
    }

    if (!hTreeItem) {
        
        hTreeItem = TreeView_GetRoot(DeviceTree->hwndTree);
    }

    SendMessage(DeviceTree->hwndTree, WM_SETREDRAW, TRUE, 0L);
    DeviceTree->RedrawWait = FALSE;

    TreeView_SelectItem(DeviceTree->hwndTree, NULL);

    if (hTreeItem) {
    
        TreeView_SelectItem(DeviceTree->hwndTree, hTreeItem);
    } else {
        
        //
        // No device is selected
        //
        if (DeviceTree->IsDialog) {

            EnableWindow(GetDlgItem(DeviceTree->hDlg, IDC_STOPDEVICE), FALSE);

        } else {

            PropSheet_SetWizButtons(GetParent(DeviceTree->hDlg), PSWIZB_BACK);
        }

        EnableWindow(GetDlgItem(DeviceTree->hDlg, IDC_PROPERTIES), FALSE);

        SetDlgItemText(DeviceTree->hDlg, IDC_DEVICEDESC, TEXT(""));
    }

    SetCursor(hCursor);

    return TRUE;
}
