//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       devtree.c
//
//--------------------------------------------------------------------------

#include "HotPlug.h"
#include <regstr.h>

//
// Define and initialize all device class GUIDs.
// (This must only be done once per module!)
//
#include <initguid.h>
#include <devguid.h>

//
// Define and initialize a global variable, GUID_NULL
// (from coguid.h)
//
DEFINE_GUID(GUID_NULL, 0L, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);


TCHAR szUnknown[64];


PDEVINST
BuildDeviceRelationsList(
   PTCHAR   DeviceId,
   HMACHINE hMachine,
   ULONG    FilterFlag,
   PUSHORT  pNumDevinst
   )
{
    ULONG Len, NumDevinst, MaxDevinst;
    USHORT NumDevInst;
    CONFIGRET ConfigRet;
    PTCHAR DeviceIdRelations, CurrDevId;
    PDEVINST DevinstRelations;

    DevinstRelations = NULL;
    DeviceIdRelations = NULL;
    NumDevInst = 0;

    Len = 0;
    ConfigRet = CM_Get_Device_ID_List_Size_Ex(&Len,
                                              DeviceId,
                                              FilterFlag,
                                              hMachine
                                              );

    if (ConfigRet == CR_SUCCESS && Len) {

        DeviceIdRelations = LocalAlloc(LPTR, Len*sizeof(TCHAR));
        *DeviceIdRelations = TEXT('\0');

        if (DeviceIdRelations) {

            ConfigRet = CM_Get_Device_ID_List_Ex(DeviceId,
                                                 DeviceIdRelations,
                                                 Len,
                                                 FilterFlag,
                                                 hMachine
                                                 );



            if (ConfigRet != CR_SUCCESS || !*DeviceIdRelations) {

                goto BDEarlyExit;
            }
        }
    }

    if (!Len) {

        goto BDEarlyExit;
    }
    
    //
    // Convert the deviceId list into a Devinst list. We make the gross
    // assumption that the Devinst list will NOT be bigger than the deviceId
    // list.
    //

    Len  = (Len + sizeof(DEVINST) - 1) & ~(sizeof(DEVINST) - 1);
    MaxDevinst = Len / sizeof(DEVINST);
    
    DevinstRelations = LocalAlloc(LPTR, Len);

    if (!DevinstRelations) {

        goto BDEarlyExit;
    }

    for (CurrDevId = DeviceIdRelations; *CurrDevId; CurrDevId += lstrlen(CurrDevId) + 1) {
        
        ConfigRet = CM_Locate_DevNode_Ex(&DevinstRelations[NumDevInst],
                                        CurrDevId,
                                        CM_LOCATE_DEVNODE_NORMAL,
                                        hMachine
                                        );

        if (ConfigRet == CR_SUCCESS) {

            if (++NumDevInst >= MaxDevinst) {

                //
                // paranoia check, if our devinst size wasn't big enough
                // the id list is probably trashed. so just give up.
                //

                break;
            }
        }
    }

BDEarlyExit:

    if (DeviceIdRelations) {

        LocalFree(DeviceIdRelations);
    }

    if (DevinstRelations) {

        if (NumDevInst) {

            PDEVINST DeviceInstance;

            Len = NumDevInst * sizeof(DEVINST);
            DeviceInstance = LocalReAlloc(DevinstRelations, Len, LMEM_FIXED);

            if (DeviceInstance) {

                DevinstRelations = DeviceInstance;
            }
        }

        else {

            LocalFree(DevinstRelations);
            DevinstRelations = NULL;
        }
    }

    *pNumDevinst = NumDevInst;
    return DevinstRelations;
}

LONG
AddChildSiblings(
    PDEVICETREE  DeviceTree,
    PDEVTREENODE ParentNode,
    DEVINST      DeviceInstance,
    int          TreeDepth,
    BOOL         Recurse
    )
{
    DWORD Len;
    CONFIGRET    ConfigRet;
    DEVINST      ChildDeviceInstance;
    PDEVTREENODE DeviceTreeNode;
    PLIST_ENTRY  ChildSiblingList;
    TCHAR        Buffer[MAX_PATH];
    DWORD        NumRelations;
    PDEVINST     pDevInst;

    ChildSiblingList = ParentNode ? &ParentNode->ChildSiblingList
                                  : &DeviceTree->ChildSiblingList;

    if (!ParentNode) {
    
        InitializeListHead(ChildSiblingList);
    }

    if (TreeDepth > DeviceTree->TreeDepth) {

        DeviceTree->TreeDepth = TreeDepth;
    }

    do {
        DeviceTreeNode = LocalAlloc(LPTR, sizeof(DEVTREENODE));

        if (!DeviceTreeNode) {
            
            return ERROR_NOT_ENOUGH_MEMORY;
        }

        memset(DeviceTreeNode, 0, sizeof(DEVTREENODE));
        InsertTailList(ChildSiblingList, &DeviceTreeNode->SiblingEntry);

        DeviceTreeNode->ParentNode = ParentNode;

        //
        // fill in info about this device instance.
        //
        InitializeListHead(&DeviceTreeNode->ChildSiblingList);
        DeviceTreeNode->DevInst = DeviceInstance;
        DeviceTreeNode->TreeDepth = TreeDepth;

        //
        // Get ClassGUID, and class name.
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


        if (ConfigRet == CR_SUCCESS) {

            pSetupGuidFromString(Buffer, &DeviceTreeNode->ClassGuid);
        }


        //
        // try fetching the class name from the Class GUID.
        // if that fails we will use the devnodes classname
        //

        *Buffer = TEXT('\0');
        Len = SIZECHARS(Buffer);
        ConfigRet = CM_Get_Class_Name_Ex(&DeviceTreeNode->ClassGuid,
                                         Buffer,
                                         &Len,
                                         0,
                                         DeviceTree->hMachine
                                         );

        if (ConfigRet == CR_SUCCESS && *Buffer) {

            Len *= sizeof(TCHAR); // len includes term null.
        }

        else if (ConfigRet != CR_SUCCESS || !*Buffer) {

            *Buffer = TEXT('\0');
            Len = sizeof(Buffer);
            ConfigRet = CM_Get_DevNode_Registry_Property_Ex(
                                            DeviceInstance,
                                            CM_DRP_CLASS,
                                            NULL,
                                            (PVOID)Buffer,
                                            &Len,
                                            0,
                                            DeviceTree->hMachine
                                            );

            if (ConfigRet != CR_SUCCESS || !*Buffer) {

                Len = 0;
            }
        }

        if (Len) {
            
            DeviceTreeNode->ClassName = LocalAlloc(LPTR, Len);

            if (DeviceTreeNode->ClassName) {

                memcpy(DeviceTreeNode->ClassName, Buffer, Len);
            }
        }

        //
        // Drive list
        //
        DeviceTreeNode->DriveList = DevNodeToDriveLetter(DeviceInstance);

        //
        // FriendlyName
        //
        *Buffer = TEXT('\0');
        Len = sizeof(Buffer);
        ConfigRet = CM_Get_DevNode_Registry_Property_Ex(DeviceInstance,
                                                        CM_DRP_FRIENDLYNAME,
                                                        NULL,
                                                        Buffer,
                                                        &Len,
                                                        0,
                                                        DeviceTree->hMachine
                                                        );
        if (ConfigRet == CR_SUCCESS && *Buffer) {

            if (DeviceTreeNode->DriveList) {

                Len += lstrlen(DeviceTreeNode->DriveList) * sizeof(TCHAR);
            }

            DeviceTreeNode->FriendlyName = LocalAlloc(LPTR, Len);

            if (DeviceTreeNode->FriendlyName) {

                memcpy(DeviceTreeNode->FriendlyName, Buffer, Len);
            
                if (DeviceTreeNode->DriveList) {

                    lstrcat(DeviceTreeNode->FriendlyName, DeviceTreeNode->DriveList);
                }
            }
        }

        else {

            DeviceTreeNode->FriendlyName = NULL;
        }


        //
        // DeviceDesc
        //

        *Buffer = TEXT('\0');
        Len = sizeof(Buffer);
        ConfigRet = CM_Get_DevNode_Registry_Property_Ex(
                                        DeviceInstance,
                                        CM_DRP_DEVICEDESC,
                                        NULL,
                                        (PVOID)Buffer,
                                        &Len,
                                        0,
                                        DeviceTree->hMachine
                                        );

        if (ConfigRet == CR_SUCCESS && *Buffer) {

            if (DeviceTreeNode->DriveList) {

                Len += lstrlen(DeviceTreeNode->DriveList) * sizeof(TCHAR);
            }

            DeviceTreeNode->DeviceDesc = LocalAlloc(LPTR, Len);

            if (DeviceTreeNode->DeviceDesc) {

                memcpy(DeviceTreeNode->DeviceDesc, Buffer, Len);
            
                if (DeviceTreeNode->DriveList) {

                    lstrcat(DeviceTreeNode->DeviceDesc, DeviceTreeNode->DriveList);
                }
            }
        }

        else {

            DeviceTreeNode->DeviceDesc = NULL;
        }

        //
        // Device capabilities
        //

        Len = sizeof(DeviceTreeNode->Capabilities);
        ConfigRet = CM_Get_DevNode_Registry_Property_Ex(
                                        DeviceInstance,
                                        CM_DRP_CAPABILITIES,
                                        NULL,
                                        (PVOID)&DeviceTreeNode->Capabilities,
                                        &Len,
                                        0,
                                        DeviceTree->hMachine
                                        );

        if (ConfigRet != CR_SUCCESS) {

            DeviceTreeNode->Capabilities = 0;
        }

        //
        // Status and Problem number
        //
        ConfigRet = CM_Get_DevNode_Status_Ex(&DeviceTreeNode->DevNodeStatus,
                                             &DeviceTreeNode->Problem,
                                             DeviceInstance,
                                             0,
                                             DeviceTree->hMachine
                                             );

        if (ConfigRet != CR_SUCCESS) {

            DeviceTreeNode->DevNodeStatus = 0;
            DeviceTreeNode->Problem = 0;
        }


        //
        // LocationInformation
        //

        DeviceTreeNode->Location = BuildLocationInformation(DeviceInstance,
                                                            DeviceTree->hMachine
                                                            );


        //
        // Get InstanceId
        //

        *Buffer = TEXT('\0');
        Len = sizeof(Buffer);
        ConfigRet = CM_Get_Device_ID_ExW(DeviceInstance,
                                         Buffer,
                                         Len/sizeof(TCHAR),
                                         0,
                                         DeviceTree->hMachine
                                         );

        if (ConfigRet == CR_SUCCESS && *Buffer) {

            Len = lstrlen(Buffer) * sizeof(TCHAR) + sizeof(TCHAR);
            DeviceTreeNode->InstanceId = LocalAlloc(LPTR, Len);

            if (DeviceTreeNode->InstanceId) {

                memcpy(DeviceTreeNode->InstanceId, Buffer, Len);
            }
        }

        else {

            DeviceTreeNode->InstanceId = NULL;
        }


        //
        // Fetch removal and eject relations
        //

        if (ConfigRet == CR_SUCCESS) {

            DeviceTreeNode->EjectRelations = BuildDeviceRelationsList(
                                                 Buffer,
                                                 DeviceTree->hMachine,
                                                 CM_GETIDLIST_FILTER_EJECTRELATIONS,
                                                 &DeviceTreeNode->NumEjectRelations
                                                 );

            DeviceTreeNode->RemovalRelations = BuildDeviceRelationsList(
                                                 Buffer,
                                                 DeviceTree->hMachine,
                                                 CM_GETIDLIST_FILTER_REMOVALRELATIONS,
                                                 &DeviceTreeNode->NumRemovalRelations
                                                 );
        }
            
        //
        // Only get children and siblings if the Recurse value was TRUE
        // otherwise we will just build a DeviceTreeNode structure for 
        // the individual devnode that was passed in.
        //
        // Also, only add rejection and removal relations when Recurse is TRUE,
        // otherwise we get messed up if two devices have removal or ejection
        // relations to each other.
        //
        if (Recurse) {
            
            LPTSTR tempDriveList;

            DeviceTreeNode->bCopy = FALSE;
            
            //
            // Add Ejection relation drive letters
            //
            NumRelations = DeviceTreeNode->NumEjectRelations;
            pDevInst = DeviceTreeNode->EjectRelations;

            while (NumRelations--) {

                if (tempDriveList = DevNodeToDriveLetter(*pDevInst)) {

                    AddChildSiblings(DeviceTree,
                                     DeviceTreeNode,
                                     *pDevInst,
                                     TreeDepth+1,
                                     FALSE
                                     );

                    LocalFree(tempDriveList);
                }
            
                pDevInst++;
            }

            //
            // Add Removal relation drive letters
            //
            NumRelations = DeviceTreeNode->NumRemovalRelations;
            pDevInst = DeviceTreeNode->RemovalRelations;

            while (NumRelations--) {

                if (tempDriveList = DevNodeToDriveLetter(*pDevInst)) {

                    AddChildSiblings(DeviceTree,
                                     DeviceTreeNode,
                                     *pDevInst,
                                     TreeDepth+1,
                                     FALSE
                                     );

                    LocalFree(tempDriveList);
                }
            
                pDevInst++;
            }
        
            //
            // If this devinst has children, then recurse to fill in its
            // child sibling list.
            //
    
            ConfigRet = CM_Get_Child_Ex(&ChildDeviceInstance,
                                        DeviceInstance,
                                        0,
                                        DeviceTree->hMachine
                                        );
    
            if (ConfigRet == CR_SUCCESS) {
    
    
                AddChildSiblings(DeviceTree,
                                 DeviceTreeNode,
                                 ChildDeviceInstance,
                                 TreeDepth+1,
                                 TRUE
                                 );
    
            }
    
            //
            // Next sibling ...
            //
    
            ConfigRet = CM_Get_Sibling_Ex(&DeviceInstance,
                                          DeviceInstance,
                                          0,
                                          DeviceTree->hMachine
                                          );
        } else {

            //
            // If Recurse is FALSE then we are making a Copy of an already existing DeviceTreeNode.
            // We do this when a HotPlug Device has a relation that will get removed when it gets
            // removed.  We need to set the bCopy flag because in certain cases the HotPlug device's 
            // relation is also a HotPlug device.  If we don't mark that it is a copy then it will
            // get added to the list of removeable devices twice.
            //
            DeviceTreeNode->bCopy = TRUE;
        }

    } while (Recurse && (ConfigRet == CR_SUCCESS));


    return ERROR_SUCCESS;
}


void
RemoveChildSiblings(
    PDEVICETREE  DeviceTree,
    PLIST_ENTRY  ChildSiblingList
    )
{
    PLIST_ENTRY Next;
    TV_INSERTSTRUCT tvi;
    PDEVTREENODE DeviceTreeNode;

    Next = ChildSiblingList->Flink;
    while (Next != ChildSiblingList) {

        DeviceTreeNode = CONTAINING_RECORD(Next, DEVTREENODE, SiblingEntry);


        //
        // recurse to free this nodes ChildSiblingList
        //

        if (!IsListEmpty(&DeviceTreeNode->ChildSiblingList)) {

            RemoveChildSiblings(DeviceTree,
                                &DeviceTreeNode->ChildSiblingList
                                );
        }

        //
        // free up this node and move on to the next sibling.
        //

        Next = Next->Flink;
        RemoveEntryList(&DeviceTreeNode->SiblingEntry);

        if (DeviceTreeNode->ClassName) {

            LocalFree(DeviceTreeNode->ClassName);
        }

        if (DeviceTreeNode->FriendlyName) {

            LocalFree(DeviceTreeNode->FriendlyName);
        }

        if (DeviceTreeNode->DeviceDesc) {

            LocalFree(DeviceTreeNode->DeviceDesc);
        }

        if (DeviceTreeNode->DriveList) {

            LocalFree(DeviceTreeNode->DriveList);
        }

        if (DeviceTreeNode->Location) {

            LocalFree(DeviceTreeNode->Location);
        }

        if (DeviceTreeNode->InstanceId) {

            LocalFree(DeviceTreeNode->InstanceId);
        }

        if (DeviceTreeNode->EjectRelations) {

            LocalFree(DeviceTreeNode->EjectRelations);
        }

        if (DeviceTreeNode->RemovalRelations) {

            LocalFree(DeviceTreeNode->RemovalRelations);
        }

        if (DeviceTree->SelectedTreeNode == DeviceTreeNode) {

            DeviceTree->SelectedTreeNode = NULL;
        }

        memset(DeviceTreeNode, 0, sizeof(DeviceTreeNode));
        LocalFree(DeviceTreeNode);
    }

    return;
}



/*
 *  Retrives the UI Device Name from the DeviceTreeNode.
 *  It is chosen from the following device registry fields
 *  (in order of preference):
 *  - FriendlyName
 *  - DeviceDescription
 *  - ClassName
 *  - Location
 *
 *  if none of the above is available uses "Unknown"
 *
 */
PTCHAR
FetchDeviceName(
     PDEVTREENODE DeviceTreeNode
     )
{
    if (DeviceTreeNode->FriendlyName) {

        return DeviceTreeNode->FriendlyName;
    }

    if (DeviceTreeNode->DeviceDesc) {

        return DeviceTreeNode->DeviceDesc;
    }

    if (DeviceTreeNode->ClassName &&
        !IsEqualGUID(&DeviceTreeNode->ClassGuid, &GUID_NULL) &&
        !IsEqualGUID(&DeviceTreeNode->ClassGuid, &GUID_DEVCLASS_UNKNOWN))
    {
        return DeviceTreeNode->ClassName;
    }

   //
   // darn everything failed
   //

   return NULL;
}



BOOL
DisplayChildSiblings(
    PDEVICETREE DeviceTree,
    PLIST_ENTRY ChildSiblingList,
    HTREEITEM   hParentTreeItem,
    BOOL        HotPlugParent
    )
{
    PLIST_ENTRY Next;
    CONFIGRET ConfigRet;
    PDEVTREENODE DeviceTreeNode;
    TV_INSERTSTRUCT tvi;
    BOOL ChildDisplayed = FALSE;

    Next = ChildSiblingList->Flink;
    while (Next != ChildSiblingList) {

        DeviceTreeNode = CONTAINING_RECORD(Next, DEVTREENODE, SiblingEntry);

        //
        // - If this device has a hotplug parent and we are in the complex view then
        //   add this device to the tree.
        // - If this device is a hotplug device and it is not a bCopy then add this device
        //   to the tree.  A bCopy device is one where we create another DeviceTreeNode structure
        //   for a device that is a relation of a hotplug device.  The problem is that this relation
        //   itself could be a hotplug device and we don't want to show to copies of it in the UI.
        //
        if (!DeviceTree->HotPlugTree ||
            (HotPlugParent && DeviceTree->ComplexView) ||
            (!DeviceTreeNode->bCopy && IsHotPlugDevice(DeviceTreeNode->DevInst, DeviceTree->hMachine)))
        {
            tvi.item.mask = TVIF_TEXT | TVIF_PARAM | TVIF_STATE ;

            if (SetupDiGetClassImageIndex(&DeviceTree->ClassImageList,
                                         &DeviceTreeNode->ClassGuid,
                                         &tvi.item.iImage
                                         ))
            {
                tvi.item.mask |= TVIF_IMAGE | TVIF_SELECTEDIMAGE;
            }

            tvi.hParent = hParentTreeItem;
            tvi.hInsertAfter = TVI_LAST;

            tvi.item.iSelectedImage = tvi.item.iImage;

            tvi.item.pszText = FetchDeviceName(DeviceTreeNode);

            if (!tvi.item.pszText) {

                tvi.item.pszText = szUnknown;
            }

            tvi.item.lParam = (LPARAM)DeviceTreeNode;

            tvi.item.stateMask = TVIS_OVERLAYMASK;

            ConfigRet = CM_Get_DevNode_Status_Ex(&DeviceTreeNode->DevNodeStatus,
                                                 &DeviceTreeNode->Problem,
                                                 DeviceTreeNode->DevInst,
                                                 0,
                                                 DeviceTree->hMachine
                                                 );

            if (ConfigRet != CR_SUCCESS ) {

                DeviceTreeNode->DevNodeStatus =  DeviceTreeNode->Problem = 0;
            }

            if (DeviceTreeNode->Problem == CM_PROB_DISABLED) {
                tvi.item.state = INDEXTOOVERLAYMASK(IDI_DISABLED_OVL - IDI_CLASSICON_OVERLAYFIRST + 1);
            }

            else if (DeviceTreeNode->Problem || !(DeviceTreeNode->DevNodeStatus & DN_STARTED)) {

                tvi.item.state = INDEXTOOVERLAYMASK(IDI_PROBLEM_OVL - IDI_CLASSICON_OVERLAYFIRST + 1);
            }

            else {
                tvi.item.state = INDEXTOOVERLAYMASK(0);
            }


            DeviceTreeNode->hTreeItem = TreeView_InsertItem(DeviceTree->hwndTree, &tvi);
            ChildDisplayed = TRUE;
        }

        else {

            DeviceTreeNode->hTreeItem = NULL;
        }


        //
        // recurse to display this nodes ChildSiblingList
        //

        if (!IsListEmpty(&DeviceTreeNode->ChildSiblingList)) {

            if (DisplayChildSiblings(DeviceTree,
                                     &DeviceTreeNode->ChildSiblingList,
                                     DeviceTree->ComplexView ? DeviceTreeNode->hTreeItem
                                                             : hParentTreeItem,
                                     DeviceTreeNode->hTreeItem != NULL
                                     ))
            {
                ChildDisplayed = TRUE;

                //
                // if we are at the root expand the list of child items.
                //

                if (DeviceTreeNode->hTreeItem && DeviceTree->ComplexView) {

                    TreeView_Expand(DeviceTree->hwndTree,
                                    DeviceTreeNode->hTreeItem,
                                    TVE_EXPAND
                                    );
                }
            }
        }

        //
        // and on to the next sibling.
        //

        Next = Next->Flink;

    }

    return ChildDisplayed;
}




void
AddChildRemoval(
    PDEVICETREE DeviceTree,
    PLIST_ENTRY ChildSiblingList
    )
{
    PLIST_ENTRY Next;
    PDEVTREENODE DeviceTreeNode;
    PDEVTREENODE FirstDeviceTreeNode;

    if (IsListEmpty(ChildSiblingList)) {

        return;
    }

    FirstDeviceTreeNode = DeviceTree->ChildRemovalList;

    Next = ChildSiblingList->Flink;
    while (Next != ChildSiblingList) {

        DeviceTreeNode = CONTAINING_RECORD(Next, DEVTREENODE, SiblingEntry);

        DeviceTreeNode->NextChildRemoval = FirstDeviceTreeNode->NextChildRemoval;
        FirstDeviceTreeNode->NextChildRemoval = DeviceTreeNode;

        InvalidateTreeItemRect(DeviceTree->hwndTree, DeviceTreeNode->hTreeItem);

        //
        // recurse to Add this node's Childs
        //

        AddChildRemoval(DeviceTree, &DeviceTreeNode->ChildSiblingList);

        //
        // and on to the next sibling.
        //
        Next = Next->Flink;
    }

    return;
}




void
ClearRemovalList(
    PDEVICETREE  DeviceTree

    )
{
    PDEVTREENODE Next;
    PDEVTREENODE DeviceTreeNode;

    DeviceTreeNode = DeviceTree->ChildRemovalList;

    if (!DeviceTreeNode) {

        return;
    }

    do {

        Next = DeviceTreeNode->NextChildRemoval;
        DeviceTreeNode->NextChildRemoval = NULL;

        //
        // force redraw of this item to reset the colors
        //
        InvalidateTreeItemRect(DeviceTree->hwndTree, DeviceTreeNode->hTreeItem);

        DeviceTreeNode = Next;

    } while (DeviceTreeNode != DeviceTree->ChildRemovalList);


    DeviceTree->ChildRemovalList = NULL;
}





PDEVTREENODE
DevTreeNodeByInstanceId(
    PTCHAR InstanceId,
    PLIST_ENTRY ChildSiblingList
    )
{
    PLIST_ENTRY Next;
    PDEVTREENODE DeviceTreeNode;

    if (!InstanceId) {
        return NULL;
        }

    Next = ChildSiblingList->Flink;
    while (Next != ChildSiblingList) {

        DeviceTreeNode = CONTAINING_RECORD(Next, DEVTREENODE, SiblingEntry);

        if (DeviceTreeNode->InstanceId &&
            !lstrcmp(DeviceTreeNode->InstanceId, InstanceId))
        {
            return DeviceTreeNode;
        }

        //
        // recurse to display this nodes ChildSiblingList
        //

        if (!IsListEmpty(&DeviceTreeNode->ChildSiblingList)) {

            DeviceTreeNode = DevTreeNodeByInstanceId(InstanceId,
                                                     &DeviceTreeNode->ChildSiblingList
                                                     );
            if (DeviceTreeNode) {

                return DeviceTreeNode;
            }
        }

        //
        // and on to the next sibling.
        //

        Next = Next->Flink;
    }

    return NULL;
}




PDEVTREENODE
DevTreeNodeByDevInst(
    DEVINST DevInst,
    PLIST_ENTRY ChildSiblingList
    )
{
    PLIST_ENTRY Next;
    PDEVTREENODE DeviceTreeNode;

    if (!DevInst) {

        return NULL;
    }

    Next = ChildSiblingList->Flink;
    while (Next != ChildSiblingList) {

        DeviceTreeNode = CONTAINING_RECORD(Next, DEVTREENODE, SiblingEntry);

        //
        // We currently assume that we can compare DEVINST from separate
        // "CM_Locate_Devnode" invocations, since a DEVINST is not a real
        // handle, but a pointer to a globalstring table.
        //
        if (DevInst == DeviceTreeNode->DevInst) {

            return DeviceTreeNode;
        }

        //
        // recurse to display this nodes ChildSiblingList
        //

        if (!IsListEmpty(&DeviceTreeNode->ChildSiblingList)) {

            DeviceTreeNode = DevTreeNodeByDevInst(DevInst,
                                                  &DeviceTreeNode->ChildSiblingList
                                                  );
            if (DeviceTreeNode) {

                return DeviceTreeNode;
            }
        }

        //
        // and on to the next sibling.
        //

        Next = Next->Flink;

    }

    return NULL;
}



PDEVTREENODE
TopLevelRemovalNode(
    PDEVICETREE DeviceTree,
    PDEVTREENODE DeviceTreeNode
    )
{
    PDEVTREENODE ParentNode = DeviceTreeNode;

    while (ParentNode) {

        DeviceTreeNode = ParentNode;

        if ((CM_PROB_DEVICE_NOT_THERE != ParentNode->Problem) &&
            (ParentNode->Capabilities & CM_DEVCAP_REMOVABLE) &&
            !(ParentNode->Capabilities & CM_DEVCAP_SURPRISEREMOVALOK) &&
            !(ParentNode->Capabilities & CM_DEVCAP_DOCKDEVICE)){

            return ParentNode;
        }

        ParentNode = ParentNode->ParentNode;
    }

    return DeviceTreeNode;
}

void
AddEjectToRemoval(
    PDEVICETREE DeviceTree
    )
{
    PDEVTREENODE RelationTreeNode;
    PDEVTREENODE DeviceTreeNode;
    PDEVINST pDevInst;
    USHORT   NumRelations;


    //
    // For each DeviceTreeNode in the removal list
    //   If it has ejection or removal relations, add it to the removal list.
    //

    DeviceTreeNode = DeviceTree->ChildRemovalList;
    if (!DeviceTreeNode) {

        return;
    }


    do {

        //
        // Ejection Relations
        //
        NumRelations = DeviceTreeNode->NumEjectRelations;
        pDevInst = DeviceTreeNode->EjectRelations;

        while (NumRelations--) {

            RelationTreeNode = DevTreeNodeByDevInst(*pDevInst++,
                                                 &DeviceTree->ChildSiblingList
                                                 );

            //
            // If we can't get a DeviceTreeNode for this device, or it is already
            // in the list of devices that will be removed (it's NextChildRemoval)
            // is non-NULL then we won't add this device to the list that will be removed.
            //
            // If this is a drive letter devnode then we have also already added it to 
            // the list so we can skip it.
            //
            if (!RelationTreeNode || 
                RelationTreeNode->NextChildRemoval ||
                RelationTreeNode->DriveList) {

                continue;
            }


            //
            // Insert the new devtreenode
            //
            RelationTreeNode->NextChildRemoval = DeviceTreeNode->NextChildRemoval;
            DeviceTreeNode->NextChildRemoval = RelationTreeNode;
        }

        //
        // Removal Relations
        //
        NumRelations = DeviceTreeNode->NumRemovalRelations;
        pDevInst = DeviceTreeNode->RemovalRelations;

        while (NumRelations--) {

            RelationTreeNode = DevTreeNodeByDevInst(*pDevInst++,
                                                 &DeviceTree->ChildSiblingList
                                                 );

            //
            // If we can't get a DeviceTreeNode for this device, or it is already
            // in the list of devices that will be removed (it's NextChildRemoval)
            // is non-NULL then we won't add this device to the list that will be removed.
            //
            // If this is a drive letter devnode then we have also already added it to 
            // the list so we can skip it.
            //
            if (!RelationTreeNode || 
                RelationTreeNode->NextChildRemoval || 
                RelationTreeNode->DriveList) {

                continue;
            }


            //
            // Insert the new devtreenode
            //
            RelationTreeNode->NextChildRemoval = DeviceTreeNode->NextChildRemoval;
            DeviceTreeNode->NextChildRemoval = RelationTreeNode;
        }


        //
        // And on to the next node.
        //

        DeviceTreeNode = DeviceTreeNode->NextChildRemoval;

    } while (DeviceTreeNode != DeviceTree->ChildRemovalList);
}
