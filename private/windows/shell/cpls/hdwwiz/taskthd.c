//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1998
//
//  File:       taskthd.c
//
//--------------------------------------------------------------------------

#include "hdwwiz.h"

typedef
BOOL
(*PINSTALLNEWDEVICE)(
   HWND hwndParent,
   LPGUID ClassGuid,
   PDWORD Reboot
   );


LONG
AddNewInvokeTask(
   HWND hwndParent,
   PTASKTHREADCREATE TaskThreadCreate
   )
{
   PINSTALLNEWDEVICE InstallNewDevice = TaskThreadCreate->pvTask;

   if (TaskThreadCreate->hMachine) {
       return ERROR_ACCESS_DENIED;
       }

   if (!(*InstallNewDevice)(hwndParent, NULL, NULL)) {
       return GetLastError();
       }

   return ERROR_SUCCESS;
}

typedef
int
(*PDEVICEPROBLEMWIZARD)(
   HWND hwndParent,
   PTCHAR MachineName,
   PTCHAR DeviceID
   );

LONG
DevProbInvokeTask(
   HWND hwndParent,
   PTASKTHREADCREATE TaskThreadCreate
   )
{
   PDEVICEPROBLEMWIZARD DeviceProbWizard=TaskThreadCreate->pvTask;
   CONFIGRET ConfigRet;
   PTCHAR pDeviceId;
   TCHAR DeviceId[MAX_DEVICE_ID_LEN];

   // convert devinst into a devid.

   if (TaskThreadCreate->DevInst) {
       ConfigRet = CM_Get_Device_ID_Ex(TaskThreadCreate->DevInst,
                                       DeviceId,
                                       SIZECHARS(DeviceId),
                                       0,
                                       TaskThreadCreate->hMachine
                                       );
       if (ConfigRet != CR_SUCCESS) {
           return ConfigRet;
           }

       pDeviceId = DeviceId;
       }
   else {
       pDeviceId = NULL;
       }


   (*DeviceProbWizard)(hwndParent, TaskThreadCreate->MachineName, pDeviceId);

   return ERROR_SUCCESS;
}




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




DWORD
TaskThread(
    PVOID pvTaskThreadCreate
    )
{
    HWND hwndDialog;
    int DlgResult;
    HMODULE hModule = NULL;
    PVOID   pvTask = NULL;
    LONG LastError = ERROR_SUCCESS;

    PTASKTHREADCREATE TaskThreadCreate = pvTaskThreadCreate;
    PTASKINITDATA TaskInitData = TaskThreadCreate->TaskInitData;

    try {

       //
       // Load up module info, and entry point.
       //
       if (TaskInitData->DllName) {
           hModule = LoadLibraryW(TaskInitData->DllName);
           if (!hModule) {
               LastError = GetLastError();
               goto TTExitCleanup;
               }
           }

       if (hModule) {
           TaskThreadCreate->hModule = hModule;

           if (TaskInitData->EntryName) {
               pvTask = GetProcAddress(hModule, TaskInitData->EntryName);
               if (!pvTask) {
                   LastError = GetLastError();
                   goto TTExitCleanup;
                   }
               }

           if (pvTask) {
               TaskThreadCreate->pvTask = pvTask;
               if (TaskInitData->InvokeTask) {
                   LastError = (*(TaskInitData->InvokeTask))(
                                     TaskThreadCreate->hwndParent,
                                     TaskThreadCreate
                                     );
                   }
               }
           }

TTExitCleanup:;

        }
    except(HdwUnhandledExceptionFilter(GetExceptionInformation())) {
       LastError = RtlNtStatusToDosError(GetExceptionCode());
       }


    if (hModule) {
        FreeLibrary(hModule);
        }

    return (DWORD)LastError;
}



LONG
CreateTaskThread(
     HWND hDlg,
     PHARDWAREWIZ HardwareWiz
     )
{
     HANDLE hThread;
     LONG  LastError;
     DWORD WaitReturn, ThreadId;
     TASKTHREADCREATE TaskThreadCreate;
     HANDLE Handles[2];

     TaskThreadCreate.hwndParent = hDlg;
     TaskThreadCreate.TaskInitData = HardwareWiz->TaskInitData;
     TaskThreadCreate.MachineName = HardwareWiz->hMachine ? HardwareWiz->MachineName: NULL;
     TaskThreadCreate.hMachine = HardwareWiz->hMachine;
     TaskThreadCreate.DevInst = HardwareWiz->DevInst;

     LastError =  TaskThread(&TaskThreadCreate);

     return LastError;
}



//
// Wizard finish dialog proc.
//
LRESULT CALLBACK
HdwInvokeTaskDlgProc(
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
    HWND hwndParent= GetParent(hDlg);

    if (message == WM_INITDIALOG) {
        LPPROPSHEETPAGE lppsp = (LPPROPSHEETPAGE)lParam;
        HardwareWiz = (PHARDWAREWIZ) lppsp->lParam;
        SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR)HardwareWiz);

        SetWindowFont(GetDlgItem(hDlg, IDC_HDWNAME), HardwareWiz->hfontTextBigBold, TRUE);

        return TRUE;
        }

    //
    // retrieve private data from window long (stored there during WM_INITDIALOG)
    //
    HardwareWiz = (PHARDWAREWIZ)GetWindowLongPtr(hDlg, DWLP_USER);

    switch (message) {

    case WM_DESTROY:
        break;

    case WM_COMMAND:
        break;

    case WM_NOTIFY: {
        NMHDR FAR *pnmhdr = (NMHDR FAR *)lParam;

        switch (pnmhdr->code) {
            case PSN_SETACTIVE:
                PropSheet_SetWizButtons(hwndParent, 0);
                EnableWindow(GetDlgItem(hwndParent, IDCANCEL), FALSE);

                ShowWindow(GetDlgItem(hDlg, IDC_ERRORTEXT), SW_HIDE);

                //
                // Display the task being spawned.
                //
                if (HardwareWiz->PrevPage != IDD_HDWINVOKETASK) {
                    SetDlgText(hDlg, IDC_HDWTASKDESC, HardwareWiz->TaskInitData->IdDesc, HardwareWiz->TaskInitData->IdDesc);
                    HardwareWiz->CurrCursor = LoadCursor(NULL, IDC_WAIT);
                    SetCursor(HardwareWiz->CurrCursor);
                    SendMessage(hDlg, WUM_INVOKETASK, 0, 0);
                    }

                HardwareWiz->PrevPage = IDD_HDWINVOKETASK;
                break;

            case PSN_WIZBACK:
                EnableWindow(GetDlgItem(hwndParent, IDCANCEL), TRUE);
                SetDlgMsgResult(hDlg, WM_NOTIFY, HardwareWiz->EnterFrom);
                break;

            case PSN_QUERYCANCEL:
                if (HardwareWiz->CurrCursor) {
                    SetDlgMsgResult(hDlg, message, TRUE);
                    }
                else {
                    SetDlgMsgResult(hDlg, message, FALSE);
                    }
            }
        }
        break;

    case WUM_INVOKETASK: {
        HCURSOR hCursor;
        LONG TaskThreadError;

        TaskThreadError = ERROR_SUCCESS;
        if (HardwareWiz->TaskInitData) {
            TaskThreadError = CreateTaskThread(hDlg, HardwareWiz);
            HardwareWiz->CurrCursor = NULL;
            }
        else {
            TaskThreadError = ERROR_INVALID_FUNCTION;
            }


        if (TaskThreadError != ERROR_SUCCESS) {
            PTCHAR ErrorMsg;


            if (FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
                              NULL,
                              TaskThreadError,
                              0,
                              (LPTSTR)&ErrorMsg,
                              0,
                              NULL
                              ))
              {
                SetDlgItemText(hDlg, IDC_ERRORTEXT, ErrorMsg);
                ShowWindow(GetDlgItem(hDlg, IDC_ERRORTEXT), SW_SHOW);
                LocalFree(ErrorMsg);
                }

            PropSheet_SetWizButtons(GetParent(hDlg), PSWIZB_BACK | PSWIZB_FINISH);

            }
        else {
            PropSheet_PressButton(hwndParent, PSBTN_BACK);
            }

        }
        break;


    case WM_SYSCOLORCHANGE:
        HdwWizPropagateMessage(hDlg, message, wParam, lParam);
        break;

    case WM_SETCURSOR:
        if (HardwareWiz->CurrCursor) {
            SetCursor(HardwareWiz->CurrCursor);
            break;
            }

         // fall thru to return(FALSE);

    default:
        return FALSE;
    }

    return TRUE;
}
