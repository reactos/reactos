/*******************************************************************************
*
*  (C) Copyright MICROSOFT Corp., 1993
*
*  TITLE:       init.c
*
*  VERSION:     1.0
*
*  AUTHOR:      19-Feb-1997 Jonle
*
*******************************************************************************/

#include "devtree.h"


#include <initguid.h>

//
// Define and initialize all device class GUIDs.
// (This must only be done once per module!)
//
#include <devguid.h>

//
// Define and initialize a global variable, GUID_NULL
// (from coguid.h)
//
DEFINE_GUID(GUID_NULL, 0L, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);


HMODULE hDevTree=NULL;



BOOL
DllInitialize(
    IN PVOID hmod,
    IN ULONG ulReason,
    IN PCONTEXT pctx OPTIONAL
    )
{
    hDevTree = hmod;

    if (ulReason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hmod);
        InitCommonControls();
        }
    else if (ulReason == DLL_PROCESS_DETACH) {
        ;
        }


    return TRUE;
}








void
DisplayDeviceTree(
   HWND hwndParent,
   PTCHAR MachineName
   )
{
   CONFIGRET ConfigRet;
   DEVICETREE DeviceTree;


   memset(&DeviceTree, 0, sizeof(DeviceTree));

   if (MachineName) {
       lstrcpy(DeviceTree.MachineName, MachineName);
       ConfigRet = CM_Connect_Machine(MachineName, &DeviceTree.hMachine);
       if (ConfigRet != CR_SUCCESS) {
           DbgPrint("CM_Connect_Machine %x\n", ConfigRet);
           return;
           }
       }

   DialogBoxParam(hDevTree,
                  MAKEINTRESOURCE(DLG_DEVTREE),
                  hwndParent,
                  DevTreeDlgProc,
                  (LPARAM)&DeviceTree
                  );

   if (DeviceTree.hMachine) {
       CM_Disconnect_Machine(DeviceTree.hMachine);
       }


   return;
}
