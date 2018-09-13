#include "devtree.h"
#include <regstr.h>


BOOL
AddChildDevices(
    HWND            hwndParent,
    PDEVICETREE     DeviceTree,
    PDEVTREENODE    ParentNode
    )
{
    DWORD Len;
    CONFIGRET ConfigRet;
    DEVINST DeviceInstance;
    PDEVTREENODE pDeviceTreeNode;
    DEVTREENODE  DeviceTreeNode;
    TV_INSERTSTRUCT tvi;
    TCHAR Buffer[MAX_PATH];


    ConfigRet = CM_Get_Child_Ex(&DeviceInstance,
                                ParentNode->DevInst,
                                0,
                                DeviceTree->hMachine
                                );

    if (ConfigRet != CR_SUCCESS) {
        return TRUE;
        }


    while (ConfigRet == CR_SUCCESS) {

        pDeviceTreeNode = NULL;
        memset(&DeviceTreeNode, 0, sizeof(DEVTREENODE));
        DeviceTreeNode.DevInst = DeviceInstance;


        //
        // Fetch the class, if it doesn't exist, skip it.
        //

        Len = sizeof(Buffer);
        ConfigRet = CM_Get_DevNode_Registry_Property_Ex(DeviceInstance,
                                                        CM_DRP_CLASSGUID,
                                                        NULL,
                                                        &Buffer,
                                                        &Len,
                                                        0,
                                                        DeviceTree->hMachine
                                                        );

        tvi.item.mask = TVIF_TEXT | TVIF_PARAM;
        if (ConfigRet == CR_SUCCESS) {
            pSetupGuidFromString(Buffer, &DeviceTreeNode.ClassGuid);
            if (SetupDiGetClassImageIndex(&DeviceTree->ClassImageList,
                                          &DeviceTreeNode.ClassGuid,
                                          &tvi.item.iImage
                                          ))
              {
               tvi.item.mask |= TVIF_IMAGE | TVIF_SELECTEDIMAGE;
               }
            }

        pDeviceTreeNode = malloc(sizeof(DEVTREENODE));
        if (!pDeviceTreeNode) {
            goto ACD_SkipIt;
            }

        *pDeviceTreeNode = DeviceTreeNode;

        pDeviceTreeNode->Sibling = ParentNode->Child;
        ParentNode->Child = pDeviceTreeNode;

        tvi.hParent = ParentNode->hTreeItem;
        tvi.hInsertAfter = TVI_LAST;

        tvi.item.iSelectedImage = tvi.item.iImage;

        Len = sizeof(Buffer);
        ConfigRet = CM_Get_DevNode_Registry_Property_Ex(DeviceInstance,
                                                        CM_DRP_DEVICEDESC,
                                                        NULL,
                                                        (PVOID)Buffer,
                                                        &Len,
                                                        0,
                                                        DeviceTree->hMachine
                                                        );
        if (ConfigRet != CR_SUCCESS) {
            wcscpy(Buffer, TEXT("Unknown !"));
            }


        tvi.item.pszText = Buffer;
        tvi.item.lParam = (LPARAM)pDeviceTreeNode;

        pDeviceTreeNode->hTreeItem = TreeView_InsertItem(DeviceTree->hwndTree, &tvi);


ACD_SkipIt:

        //
        // Do Childs.
        //

        if (pDeviceTreeNode) {
            AddChildDevices(hwndParent, DeviceTree, pDeviceTreeNode);
            }


        //
        // Next sibling ...
        //

        ConfigRet = CM_Get_Sibling_Ex(&DeviceInstance,
                                      DeviceInstance,
                                      0,
                                      DeviceTree->hMachine
                                      );
        }



    return TRUE;
}



void
AddEditLine(
   HWND hwndEdit,
   PTCHAR LineBuffer
   )
{
   wcscat(LineBuffer, L"\r\n");
   SendMessage(hwndEdit, EM_SETSEL, (WPARAM)-1, (LPARAM)-1);
   SendMessage(hwndEdit, EM_REPLACESEL, 0, (LPARAM)LineBuffer);
}


void
MultiSzToCommas(
   PWCHAR Buffer
   )
{
   PWCHAR pwch;

   pwch = Buffer;

   while (*pwch++) {
       if (!*pwch) {
           *pwch++ = L',';
           }
       }

}


void
FillRegKeyValues(
   HWND hwndEdit,
   HKEY hKey,
   PTCHAR LineBuffer,
   PVOID Buffer,
   int    LenBuffer
   )
{
   DWORD Type, Index, Len, LenData;
   LONG Error;
   TCHAR NameBuffer[MAX_PATH];

   Index = 0;
   while (TRUE) {
       Len = SIZECHARS(NameBuffer);
       LenData = LenBuffer;
       Error = RegEnumValue(hKey,
                            Index++,
                            NameBuffer,
                            &Len,
                            NULL,
                            &Type,
                            Buffer,
                            &LenData
                            );

       if (Error == ERROR_MORE_DATA) {
           Error = ERROR_SUCCESS;
           wcscpy(Buffer, L"ERROR_MORE_DATA");
           }

       if (Error != ERROR_SUCCESS) {
           break;
           }

       if (Type == REG_MULTI_SZ) {
           MultiSzToCommas(Buffer);
           Type = REG_SZ;
           }

       if (Type == REG_SZ || Type == REG_EXPAND_SZ) {
           wsprintf(LineBuffer, L"%ws:\t%ws", NameBuffer, Buffer);
           }
       else {
           wsprintf(LineBuffer, L"%ws Len %d:\t%wx", NameBuffer, Buffer, LenData);
           }

       AddEditLine(hwndEdit, LineBuffer);
       }

}



void
FillDeviceStatus(
   PDEVICETREE DeviceTree,
   PDEVTREENODE DeviceTreeNode
   )
{
   CONFIGRET ConfigRet;
   ULONG DevNodeStatus, Problem, Len;
   DEVINST DeviceInstance = DeviceTreeNode->DevInst;
   HKEY hKeyClass, hKeyDriver;
   PWCHAR pwch;
   TCHAR LineBuffer[MAX_PATH*2];
   TCHAR Buffer[MAX_PATH];


   //
   // Turn off the edit draw, and clear the edit buffer.
   //

   SendMessage( DeviceTree->hwndEdit, WM_SETREDRAW, FALSE, 0);
   *Buffer= '\0';
   SendMessage( DeviceTree->hwndEdit, WM_SETTEXT, 0, (LPARAM)Buffer);

   //
   // Dump the DeviceInstanceId
   //
   Len = sizeof(Buffer);
   ConfigRet = CM_Get_Device_ID_Ex(DeviceInstance,
                                   (PVOID)Buffer,
                                   Len,
                                   0,
                                   DeviceTree->hMachine
                                   );
   if (ConfigRet == CR_BUFFER_SMALL) {
       Buffer[SIZECHARS(Buffer)] = L'\0';
       ConfigRet = CR_SUCCESS;
       }

   if (ConfigRet != CR_SUCCESS) {
       wcscpy(Buffer, TEXT("?"));
       }
   wsprintf(LineBuffer, L"Device Id:\t%ws", Buffer);
   AddEditLine(DeviceTree->hwndEdit, LineBuffer);


   //
   // Fill in the hardware registry key (in Enum\device instance key)
   //
   wsprintf(LineBuffer, L"DEVICE REGISTRY PROPERTY");
   AddEditLine(DeviceTree->hwndEdit, LineBuffer);


   Len = sizeof(Buffer);
   ConfigRet = CM_Get_DevNode_Registry_Property_Ex(DeviceInstance,
                                                   CM_DRP_FRIENDLYNAME,
                                                   NULL,
                                                   (PVOID)Buffer,
                                                   &Len,
                                                   0,
                                                   DeviceTree->hMachine
                                                   );
   if (ConfigRet != CR_SUCCESS) {
       wcscpy(Buffer, TEXT("?"));
       }
   wsprintf(LineBuffer, L"FriendlyName:\t%ws", Buffer);
   AddEditLine(DeviceTree->hwndEdit, LineBuffer);



   Len = sizeof(Buffer);
   ConfigRet = CM_Get_DevNode_Registry_Property_Ex(DeviceInstance,
                                                   CM_DRP_DEVICEDESC,
                                                   NULL,
                                                   (PVOID)Buffer,
                                                   &Len,
                                                   0,
                                                   DeviceTree->hMachine
                                                   );
   if (ConfigRet != CR_SUCCESS) {
       wcscpy(Buffer, TEXT("?"));
       }

   wsprintf(LineBuffer, L"DeviceDesc:\t%ws", Buffer);
   AddEditLine(DeviceTree->hwndEdit, LineBuffer);

   Len = sizeof(Buffer);
   ConfigRet = CM_Get_DevNode_Registry_Property_Ex(DeviceInstance,
                                                   CM_DRP_MFG,
                                                   NULL,
                                                   (PVOID)Buffer,
                                                   &Len,
                                                   0,
                                                   DeviceTree->hMachine
                                                   );
   if (ConfigRet != CR_SUCCESS) {
       wcscpy(Buffer, TEXT("?"));
       }

   wsprintf(LineBuffer, L"Manufacturer:\t%ws", Buffer);
   AddEditLine(DeviceTree->hwndEdit, LineBuffer);


   Len = sizeof(Buffer);
   ConfigRet = CM_Get_DevNode_Registry_Property_Ex(DeviceInstance,
                                                   CM_DRP_CLASS,
                                                   NULL,
                                                   (PVOID)Buffer,
                                                   &Len,
                                                   0,
                                                   DeviceTree->hMachine
                                                   );
   if (ConfigRet != CR_SUCCESS) {
       wcscpy(Buffer, TEXT("?"));
       }

   wsprintf(LineBuffer, L"ClassName:\t%ws", Buffer);
   AddEditLine(DeviceTree->hwndEdit, LineBuffer);


   Len = sizeof(Buffer);
   ConfigRet = CM_Get_DevNode_Registry_Property_Ex(DeviceInstance,
                                                   CM_DRP_CLASSGUID,
                                                   NULL,
                                                   (PVOID)Buffer,
                                                   &Len,
                                                   0,
                                                   DeviceTree->hMachine
                                                   );
   if (ConfigRet != CR_SUCCESS) {
       wcscpy(Buffer, TEXT("?"));
       }

   wsprintf(LineBuffer, L"ClassGuid:\t%ws", Buffer);
   AddEditLine(DeviceTree->hwndEdit, LineBuffer);


   Len = sizeof(DeviceTreeNode->Driver);
   ConfigRet = CM_Get_DevNode_Registry_Property_Ex(DeviceInstance,
                                                   CM_DRP_DRIVER,
                                                   NULL,
                                                   (PVOID)DeviceTreeNode->Driver,
                                                   &Len,
                                                   0,
                                                   DeviceTree->hMachine
                                                   );
   if (ConfigRet != CR_SUCCESS) {
       wcscpy(Buffer, TEXT("?"));
       }

   wsprintf(LineBuffer, L"Driver:\t\t%ws", DeviceTreeNode->Driver);
   AddEditLine(DeviceTree->hwndEdit, LineBuffer);


   Len = sizeof(Buffer);
   ConfigRet = CM_Get_DevNode_Registry_Property_Ex(DeviceInstance,
                                                   CM_DRP_SERVICE,
                                                   NULL,
                                                   (PVOID)Buffer,
                                                   &Len,
                                                   0,
                                                   DeviceTree->hMachine
                                                   );
   if (ConfigRet != CR_SUCCESS) {
       wcscpy(Buffer, TEXT("?"));
       }

   wsprintf(LineBuffer, L"Service:\t\t%ws", Buffer);
   AddEditLine(DeviceTree->hwndEdit, LineBuffer);


   Len = sizeof(Buffer);
   memset(Buffer, 0 , sizeof(Buffer));
   ConfigRet = CM_Get_DevNode_Registry_Property_Ex(DeviceInstance,
                                                   CM_DRP_HARDWAREID,
                                                   NULL,
                                                   (PVOID)Buffer,
                                                   &Len,
                                                   0,
                                                   DeviceTree->hMachine
                                                   );
   if (ConfigRet != CR_SUCCESS) {
       wcscpy(Buffer, TEXT("?"));
       }

   MultiSzToCommas(Buffer);
   wsprintf(LineBuffer, L"HardwareId:\t%ws", Buffer);
   AddEditLine(DeviceTree->hwndEdit, LineBuffer);


   Len = sizeof(Buffer);
   memset(Buffer, 0 , sizeof(Buffer));
   ConfigRet = CM_Get_DevNode_Registry_Property_Ex(DeviceInstance,
                                                   CM_DRP_COMPATIBLEIDS,
                                                   NULL,
                                                   (PVOID)Buffer,
                                                   &Len,
                                                   0,
                                                   DeviceTree->hMachine
                                                   );
   if (ConfigRet != CR_SUCCESS) {
       wcscpy(Buffer, TEXT("?"));
       }

   MultiSzToCommas(Buffer);
   wsprintf(LineBuffer, L"CompatibleIds:\t%ws", Buffer);
   AddEditLine(DeviceTree->hwndEdit, LineBuffer);


   Len = sizeof(Buffer);
   ConfigRet = CM_Get_DevNode_Registry_Property_Ex(DeviceInstance,
                                                   CM_DRP_PHYSICAL_DEVICE_OBJECT_NAME,
                                                   NULL,
                                                   (PVOID)Buffer,
                                                   &Len,
                                                   0,
                                                   DeviceTree->hMachine
                                                   );
   if (ConfigRet != CR_SUCCESS) {
       wcscpy(Buffer, TEXT("?"));
       }

   wsprintf(LineBuffer, L"Device Object Name:\t\t%ws", Buffer);
   AddEditLine(DeviceTree->hwndEdit, LineBuffer);




   Len = sizeof(Buffer);
   ConfigRet = CM_Get_DevNode_Registry_Property_Ex(DeviceInstance,
                                                   CM_DRP_LOCATION_INFORMATION,
                                                   NULL,
                                                   (PVOID)Buffer,
                                                   &Len,
                                                   0,
                                                   DeviceTree->hMachine
                                                   );
   if (ConfigRet != CR_SUCCESS) {
       wcscpy(Buffer, TEXT("?"));
       }

   wsprintf(LineBuffer, L"Location Information:\t\t%ws", Buffer);
   AddEditLine(DeviceTree->hwndEdit, LineBuffer);




   Len = sizeof(Problem);
   ConfigRet = CM_Get_DevNode_Registry_Property_Ex(DeviceInstance,
                                                   CM_DRP_CAPABILITIES,
                                                   NULL,
                                                   (PVOID)&Problem,
                                                   &Len,
                                                   0,
                                                   NULL
                                                   );
   if (ConfigRet != CR_SUCCESS) {
       Problem = (ULONG)-1;
       }

   wsprintf(LineBuffer, L"Device Capabilities=%x", Problem);
   AddEditLine(DeviceTree->hwndEdit, LineBuffer);


   Len = sizeof(Problem);
   ConfigRet = CM_Get_DevNode_Registry_Property_Ex(DeviceInstance,
                                                   CM_DRP_CONFIGFLAGS,
                                                   NULL,
                                                   (PVOID)&Problem,
                                                   &Len,
                                                   0,
                                                   NULL
                                                   );
   if (ConfigRet != CR_SUCCESS) {
       Problem = (ULONG)-1;
       }

   wsprintf(LineBuffer, L"Device ConfigFlags=%x", Problem);
   AddEditLine(DeviceTree->hwndEdit, LineBuffer);


   Len = sizeof(Problem);
   ConfigRet = CM_Get_DevNode_Registry_Property_Ex(DeviceInstance,
                                                   CM_DRP_UI_NUMBER,
                                                   NULL,
                                                   (PVOID)&Problem,
                                                   &Len,
                                                   0,
                                                   NULL
                                                   );
   if (ConfigRet != CR_SUCCESS) {
       Problem = (ULONG)-1;
       }

   wsprintf(LineBuffer, L"Device UI Number=%x", Problem);
   AddEditLine(DeviceTree->hwndEdit, LineBuffer);



   ConfigRet = CM_Get_DevNode_Status_Ex(&DevNodeStatus,
                                        &Problem,
                                        DeviceInstance,
                                        0,
                                        DeviceTree->hMachine
                                        );

   if (ConfigRet == CR_SUCCESS) {
       wsprintf(LineBuffer, L"DevNode Status=%x Problem=%x", DevNodeStatus, Problem);
       }
   else {
       wsprintf(LineBuffer, L"DevNode Status Unknown cr=%x", ConfigRet);
       }
   AddEditLine(DeviceTree->hwndEdit, LineBuffer);




   //
   // fill in the software or driver registry (under class).
   //

   wsprintf(LineBuffer, L"\r\nCLASS KEY");
   AddEditLine(DeviceTree->hwndEdit,LineBuffer);


   ConfigRet = CM_Open_Class_Key_ExW(&DeviceTreeNode->ClassGuid,
                                     NULL,
                                     KEY_READ,
                                     RegDisposition_OpenExisting,
                                     &hKeyClass,
                                     0,
                                     DeviceTree->hMachine
                                     );

   if (ConfigRet == CR_SUCCESS) {
       FillRegKeyValues(DeviceTree->hwndEdit,
                        hKeyClass,
                        LineBuffer,
                        Buffer,
                        sizeof(Buffer)
                        );

       //
       // if there is a driver subkey, open it and dump its info.
       //

       if (DeviceTreeNode->Driver && (pwch = wcschr(DeviceTreeNode->Driver, L'\\'))) {
           LONG Error;

           wsprintf(LineBuffer, L"\r\nCLASS DRIVER KEY %ws", pwch);
           AddEditLine(DeviceTree->hwndEdit,LineBuffer);

           Error = RegOpenKeyEx(hKeyClass,
                                ++pwch,
                                0,
                                KEY_READ,
                                &hKeyDriver
                                );

           if (Error == CR_SUCCESS) {
               FillRegKeyValues(DeviceTree->hwndEdit,
                                hKeyDriver,
                                LineBuffer,
                                Buffer,
                                sizeof(Buffer)
                                );

               RegCloseKey(hKeyDriver);
               }
           else {
               wsprintf(LineBuffer, L"Open Class Driver Key Error=%d", Error);
               AddEditLine(DeviceTree->hwndEdit,LineBuffer);
               }
           }


       RegCloseKey(hKeyClass);


       }
   else {
       wsprintf(LineBuffer, L"OpenClassKey failed cr=%x", ConfigRet);
       AddEditLine(DeviceTree->hwndEdit, LineBuffer);
       }





   //
   // ok to redraw now!
   //

   SendMessage(DeviceTree->hwndEdit, WM_SETREDRAW, TRUE, 0);

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
    TV_ITEM tvItem;


    DeviceTree->hwndTree = hwndTree = GetDlgItem(hDlg, IDC_DEVTREE);
    DeviceTree->hwndEdit = GetDlgItem(hDlg, IDC_DEVSTATUS);


    if (DeviceTree->hMachine) {
        SetDlgItemText(hDlg, IDC_DEVTREE_MACHINE, DeviceTree->MachineName);
        }



#if 0
    //
    // Create an empty classless DeviceInfoList. We will use this list
    // for easy access to the information of each devnode.
    // TBD: make remotable.
    //
    DeviceTree->hDeviceInfo = SetupDiCreateDeviceInfoList(NULL, hDlg);
    if (DeviceTree->hDeviceInfo == INVALID_HANDLE_VALUE) {
        DeviceTree->hDeviceInfo = NULL;
        return FALSE;
        }
#endif


    // Get the Class Icon Image Lists
    DeviceTree->ClassImageList.cbSize = sizeof(SP_CLASSIMAGELIST_DATA);
    if (SetupDiGetClassImageList(&DeviceTree->ClassImageList)) {
        TreeView_SetImageList(hwndTree, DeviceTree->ClassImageList.ImageList, TVSIL_NORMAL);
        }
    else {
        DeviceTree->ClassImageList.cbSize = 0;
        }

    //
    // Get the root devnode.
    //

    ConfigRet = CM_Locate_DevNode_Ex(&DeviceTree->RootNode.DevInst,
                                     NULL,
                                     CM_LOCATE_DEVNODE_NORMAL,
                                     DeviceTree->hMachine
                                     );

    if (ConfigRet != CR_SUCCESS) {
        DbgPrint("InitDevTreeDlgProc CM_Locate_DevNode(ROOT) %x\n", ConfigRet);
        return FALSE;
        }



    // Fill in the Tree.

    SendMessage(hwndTree, WM_SETREDRAW, FALSE, 0L);
    TreeView_DeleteAllItems(hwndTree);

    DeviceTree->RootNode.hTreeItem = NULL;
    AddChildDevices(hDlg, DeviceTree, &DeviceTree->RootNode);

    SendMessage(hwndTree, WM_SETREDRAW, TRUE, 0L);

    hTreeItem = TreeView_GetRoot(hwndTree);
    TreeView_SelectItem(hwndTree, hTreeItem);

    return TRUE;
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
        DeviceTree = (PDEVICETREE)lParam;
        SetWindowLong(hDlg, DWL_USER, lParam);
        if (!InitDevTreeDlgProc(hDlg, DeviceTree)) {
            EndDialog(hDlg, FALSE); // unexpected error
            }

        return TRUE;
        }

    //
    // retrieve private data from window long (stored there during WM_INITDIALOG)
    //
    DeviceTree = (PDEVICETREE)GetWindowLong(hDlg, DWL_USER);


    switch (message) {

    case WM_DESTROY:
        if (DeviceTree->ClassImageList.cbSize) {
            SetupDiDestroyClassImageList(&DeviceTree->ClassImageList);
            DeviceTree->ClassImageList.cbSize = 0;
            }

        if (DeviceTree->hDeviceInfo) {
            SetupDiDestroyDeviceInfoList(DeviceTree->hDeviceInfo);
            DeviceTree->hDeviceInfo = NULL;
            }


        break;

    case WM_CLOSE:
        SendMessage (hDlg, WM_COMMAND, IDCANCEL, 0L);
        break;

    case WM_COMMAND:
        switch(wParam) {

        case IDCANCEL:
            EndDialog(hDlg, IDCANCEL);
            break;
        }
        break;


    // Listen for Tree notifications
    case WM_NOTIFY:
        switch (((NMHDR FAR *)lParam)->code) {
        case TVN_SELCHANGED: {
            NM_TREEVIEW *nmTreeView = (NM_TREEVIEW *)lParam;

            FillDeviceStatus(DeviceTree, (PDEVTREENODE)(nmTreeView->itemNew.lParam));
            }
            break;


        default:
            return FALSE;
        }
        break;


    default:
        return FALSE;

    }


    return TRUE;

}
