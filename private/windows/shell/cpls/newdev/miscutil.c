//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       miscutil.c
//
//--------------------------------------------------------------------------

#include "newdevp.h"



TCHAR szUnknownDevice[64];
USHORT LenUnknownDevice;

TCHAR szUnknown[64];
USHORT LenUnknown;

PTCHAR
BuildFriendlyName(
    DEVINST DevInst,
    BOOL UseNewDeviceDesc,
    HMACHINE hMachine
    )
{
    PTCHAR Location;
    PTCHAR FriendlyName;
    CONFIGRET ConfigRet;
    ULONG ulSize;
    TCHAR szBuffer[MAX_PATH];

    *szBuffer = TEXT('\0');

    //
    // Try the registry for NewDeviceDesc
    //
    if (UseNewDeviceDesc) {
        
        HKEY hKey;
        DWORD dwType = REG_SZ;

        ConfigRet = CM_Open_DevNode_Key(DevInst,
                                        KEY_READ,
                                        0,
                                        RegDisposition_OpenExisting,
                                        &hKey,
                                        CM_REGISTRY_HARDWARE
                                        );

        if (ConfigRet == CR_SUCCESS) {

            ulSize = sizeof(szBuffer);
            RegQueryValueEx(hKey,
                            REGSTR_VAL_NEW_DEVICE_DESC,
                            NULL,
                            &dwType,
                            (LPBYTE)szBuffer,
                            &ulSize
                            );

            RegCloseKey(hKey);
        }
    }

    if (ConfigRet != CR_SUCCESS || !*szBuffer) {

        //
        // Try the registry for FRIENDLYNAME
        //
    
        ulSize = sizeof(szBuffer);
        ConfigRet = CM_Get_DevNode_Registry_Property_Ex(DevInst,
                                                        CM_DRP_FRIENDLYNAME,
                                                        NULL,
                                                        szBuffer,
                                                        &ulSize,
                                                        0,
                                                        hMachine
                                                        );
        if (ConfigRet != CR_SUCCESS || !*szBuffer) {
            //
            // Try the registry for DEVICEDESC
            //
    
            ulSize = sizeof(szBuffer);
            ConfigRet = CM_Get_DevNode_Registry_Property_Ex(DevInst,
                                                            CM_DRP_DEVICEDESC,
                                                            NULL,
                                                            szBuffer,
                                                            &ulSize,
                                                            0,
                                                            hMachine
                                                            );
            if (ConfigRet != CR_SUCCESS || !*szBuffer) {
                
                GUID ClassGuid;
    
                //
                // Try the registry for CLASSNAME
                //
    
                ulSize = sizeof(szBuffer);
                ConfigRet = CM_Get_DevNode_Registry_Property_Ex(DevInst,
                                                                CM_DRP_CLASSGUID,
                                                                NULL,
                                                                szBuffer,
                                                                &ulSize,
                                                                0,
                                                                hMachine
                                                                );
    
    
                if (ConfigRet == CR_SUCCESS) {
                    
                    pSetupGuidFromString(szBuffer, &ClassGuid);
                }
    
    
                if (!IsEqualGUID(&ClassGuid, &GUID_NULL) &&
                    !IsEqualGUID(&ClassGuid, &GUID_DEVCLASS_UNKNOWN))
                {
                    ulSize = sizeof(szBuffer);
                    ConfigRet = CM_Get_DevNode_Registry_Property_Ex(DevInst,
                                                                    CM_DRP_CLASS,
                                                                    NULL,
                                                                    szBuffer,
                                                                    &ulSize,
                                                                    0,
                                                                    hMachine
                                                                    );
                }
                else {
                    
                    ConfigRet = ~CR_SUCCESS;
                }
    
    
            }
        }
    }
    

    if (ConfigRet == CR_SUCCESS && *szBuffer) {
        
        FriendlyName = LocalAlloc(LPTR, ulSize);
        
        if (FriendlyName) {
            
            memcpy(FriendlyName, szBuffer, ulSize);
        }
    }
    else {
        
        FriendlyName = NULL;
    }

    return FriendlyName;
}







/* ----------------------------------------------------------------------
 * SetDlgText - Set Dialog Text Field
 *
 * Concatenates a number of string resources and does a SetWindowText()
 * for a dialog text control.
 *
 * Parameters:
 *
 *      hDlg         - Dialog handle
 *      iControl     - Dialog control ID to receive text
 *      nStartString - ID of first string resource to concatenate
 *      nEndString   - ID of last string resource to concatenate
 *
 *      Note: the string IDs must be consecutive.
 */

void
SetDlgText(HWND hDlg, int iControl, int nStartString, int nEndString)
{
    int     iX;
    TCHAR   szText[SDT_MAX_TEXT];

    szText[0] = '\0';
    for (iX = nStartString; iX<= nEndString; iX++) {
         LoadString(hNewDev,
                    iX,
                    szText + lstrlen(szText),
                    sizeof(szText)/sizeof(TCHAR) - lstrlen(szText)
                    );
        }

    if (iControl) {
        SetDlgItemText(hDlg, iControl, szText);
        }
    else {
        SetWindowText(hDlg, szText);
        }

}


void
LoadText(PTCHAR szText, int SizeText, int nStartString, int nEndString)
{
    int     iX;

    for (iX = nStartString; iX<= nEndString; iX++) {
         LoadString(hNewDev,
                    iX,
                    szText + lstrlen(szText),
                    SizeText/sizeof(TCHAR) - lstrlen(szText)
                    );
         }

    return;
}




VOID
_OnSysColorChange(
    HWND hWnd,
    WPARAM wParam,
    LPARAM lParam
    )
{

    HWND hChildWnd;

    hChildWnd = GetWindow(hWnd, GW_CHILD);
    while (hChildWnd != NULL) {
        SendMessage(hChildWnd, WM_SYSCOLORCHANGE, wParam, lParam);
        hChildWnd = GetWindow(hChildWnd, GW_HWNDNEXT);
        }

}



BOOL
NoPrivilegeWarning(
   HWND hWnd
   )
/*++

    This function checks to see if the user has SE_LOAD_DRIVER_NAME privileges
    which means they can install and load new kernel mode drivers. 
    
    If the user does NOT have this privilege then a warning is displayed telling
    them that they have insufficient privileges to install hardware on this machine.

Arguments
    
    hWnd - Parent window handle                                              
                                              
Return Value:
    TRUE if the user does NOT have SE_LOAD_DRIVER_NAME privileges and
    FALSE if the user does have this privilege
    
--*/
{
   TCHAR szMsg[MAX_PATH];
   TCHAR szCaption[MAX_PATH];

   if (!DoesUserHavePrivilege((PCTSTR)SE_LOAD_DRIVER_NAME)) {

       if (LoadString(hNewDev,
                      IDS_NDW_NOTADMIN,
                      szMsg,
                      MAX_PATH)
          &&
           LoadString(hNewDev,
                      IDS_NEWDEVICENAME,
                      szCaption,
                      MAX_PATH))
        {
            MessageBox(hWnd, szMsg, szCaption, MB_OK | MB_ICONINFORMATION);
        }

       return TRUE;
    }

   return FALSE;
}



int
NdwMessageBox(
   HWND hWnd,
   int  IdText,
   int  IdCaption,
   UINT Type
   )
{
   TCHAR szText[MAX_PATH];
   TCHAR szCaption[MAX_PATH];

   if (LoadString(hNewDev, IdText, szText, MAX_PATH) &&
       LoadString(hNewDev, IdCaption, szCaption, MAX_PATH))
     {
       return MessageBox(hWnd, szText, szCaption, Type);
       }

   return IDIGNORE;
}


LONG
NdwBuildClassInfoList(
    PNEWDEVWIZ NewDevWiz,
    DWORD ClassListFlags
    )
{
    LONG Error;

    //
    // Build the class info list
    //

    while (!SetupDiBuildClassInfoList(ClassListFlags,
                                      NewDevWiz->ClassGuidList,
                                      NewDevWiz->ClassGuidSize,
                                      &NewDevWiz->ClassGuidNum
                                      ))
         {
          Error = GetLastError();
          if (NewDevWiz->ClassGuidList) {
              LocalFree(NewDevWiz->ClassGuidList);
              NewDevWiz->ClassGuidList = NULL;
              }

          if (Error == ERROR_INSUFFICIENT_BUFFER &&
              NewDevWiz->ClassGuidNum > NewDevWiz->ClassGuidSize)
            {
              NewDevWiz->ClassGuidList = LocalAlloc(LPTR, NewDevWiz->ClassGuidNum*sizeof(GUID));
              if (!NewDevWiz->ClassGuidList) {
                  NewDevWiz->ClassGuidSize = 0;
                  NewDevWiz->ClassGuidNum = 0;
                  return ERROR_NOT_ENOUGH_MEMORY;
                  }
              NewDevWiz->ClassGuidSize = NewDevWiz->ClassGuidNum;
              }
          else {
              if (NewDevWiz->ClassGuidList) {
                  LocalFree(NewDevWiz->ClassGuidList);
                  }
              NewDevWiz->ClassGuidSize = 0;
              NewDevWiz->ClassGuidNum = 0;
              return Error;
              }
          }

    return ERROR_SUCCESS;
}




void
HideWindowByMove(
    HWND hDlg
    )
{
    RECT rect;
    //
    // Move the window offscreen, using the virtual coords for Upper Left Corner
    //

    GetWindowRect(hDlg, &rect);
    MoveWindow(hDlg,
               GetSystemMetrics(SM_XVIRTUALSCREEN),
               GetSystemMetrics(SM_YVIRTUALSCREEN) - (rect.bottom - rect.top),
               rect.right - rect.left,
               rect.bottom - rect.top,
               TRUE
               );
}



LONG
NdwUnhandledExceptionFilter(
    struct _EXCEPTION_POINTERS *ExceptionPointers
    )
{
    LONG lRet;
    BOOL BeingDebugged;

    lRet = UnhandledExceptionFilter(ExceptionPointers);

    BeingDebugged = IsDebuggerPresent();

    //
    // Normal code path is to handle the exception.
    // However, if a debugger is present, and the system's unhandled
    // exception filter returns continue search, we let it go
    // thru to allow the debugger a chance at it.
    //

    if (lRet == EXCEPTION_CONTINUE_SEARCH && !BeingDebugged) {
        lRet = EXCEPTION_EXECUTE_HANDLER;
        }

    return lRet;
}




void
RebootComputerNow(
    void
    )
{
    EnablePrivilege((PCSTR)SE_SHUTDOWN_NAME, TRUE);
    ExitWindowsEx(EWX_REBOOT, 0);
}


BOOL
SetClassGuid(
    HDEVINFO hDeviceInfo,
    PSP_DEVINFO_DATA DeviceInfoData,
    LPGUID ClassGuid
    )
{

    TCHAR ClassGuidString[MAX_GUID_STRING_LEN];

    pSetupStringFromGuid(ClassGuid,
                         ClassGuidString,
                         sizeof(ClassGuidString)/sizeof(TCHAR)
                         );

    return SetupDiSetDeviceRegistryProperty(hDeviceInfo,
                                            DeviceInfoData,
                                            SPDRP_CLASSGUID,
                                            (LPBYTE)ClassGuidString,
                                            MAX_GUID_STRING_LEN * sizeof(TCHAR)
                                            );
}




HPROPSHEETPAGE
CreateWizExtPage(
   int PageResourceId,
   DLGPROC pfnDlgProc,
   PNEWDEVWIZ NewDevWiz
   )
{
    PROPSHEETPAGE    psp;

    memset(&psp, 0, sizeof(PROPSHEETPAGE));
    psp.dwSize = sizeof(PROPSHEETPAGE);
    psp.dwFlags = PSP_DEFAULT;
    psp.hInstance = hNewDev;
    psp.lParam = (LPARAM)NewDevWiz;
    psp.pszTemplate = MAKEINTRESOURCE(PageResourceId);
    psp.pfnDlgProc = pfnDlgProc;

    return CreatePropertySheetPage(&psp);
}


BOOL
AddClassWizExtPages(
   HWND hwndParentDlg,
   PNEWDEVWIZ NewDevWiz,
   PSP_NEWDEVICEWIZARD_DATA DeviceWizardData,
   DI_FUNCTION InstallFunction
   )
{
    DWORD NumPages;
    BOOL bRet = FALSE;

    //
    // If this is not a manual install, then only the DIF_NEWDEVICEWIZARD_FINISHINSTALL
    // wizard is valid.
    //
    if (!(NewDevWiz->ManualInstall) &&
        (DIF_NEWDEVICEWIZARD_FINISHINSTALL != InstallFunction)) {

        return FALSE;
    }

    memset(DeviceWizardData, 0, sizeof(SP_NEWDEVICEWIZARD_DATA));
    DeviceWizardData->ClassInstallHeader.InstallFunction = InstallFunction;
    DeviceWizardData->ClassInstallHeader.cbSize = sizeof(SP_CLASSINSTALL_HEADER);
    DeviceWizardData->hwndWizardDlg = hwndParentDlg;

    if (SetupDiSetClassInstallParams(NewDevWiz->hDeviceInfo,
                                     &NewDevWiz->DeviceInfoData,
                                     &DeviceWizardData->ClassInstallHeader,
                                     sizeof(SP_NEWDEVICEWIZARD_DATA)
                                     )
        &&
        
        (SetupDiCallClassInstaller(InstallFunction,
                                  NewDevWiz->hDeviceInfo,
                                  &NewDevWiz->DeviceInfoData
                                  )

            ||

            (ERROR_DI_DO_DEFAULT == GetLastError()))
            
        &&
        SetupDiGetClassInstallParams(NewDevWiz->hDeviceInfo,
                                     &NewDevWiz->DeviceInfoData,
                                     &DeviceWizardData->ClassInstallHeader,
                                     sizeof(SP_NEWDEVICEWIZARD_DATA),
                                     NULL
                                     )
        &&
        DeviceWizardData->NumDynamicPages)
    {
        NumPages = 0;
        while (NumPages < DeviceWizardData->NumDynamicPages) {
           PropSheet_AddPage(hwndParentDlg, DeviceWizardData->DynamicPages[NumPages++]);
           }

        bRet = TRUE;
    }

    //
    // Clear the class install parameters.
    //

    SetupDiSetClassInstallParams(NewDevWiz->hDeviceInfo,
                                 &NewDevWiz->DeviceInfoData,
                                 NULL,
                                 0
                                 );

    return bRet;
}





void
RemoveClassWizExtPages(
   HWND hwndParentDlg,
   PSP_NEWDEVICEWIZARD_DATA DeviceWizardData
   )
{
    DWORD NumPages;

    NumPages = DeviceWizardData->NumDynamicPages;
    
    while (NumPages--) {
       
        PropSheet_RemovePage(hwndParentDlg,
                             (WPARAM)-1,
                             DeviceWizardData->DynamicPages[NumPages]
                             );
    }

    memset(DeviceWizardData, 0, sizeof(SP_NEWDEVICEWIZARD_DATA));

    return;
}

BOOL
FileExists(
    IN  PCTSTR           FileName,
    OUT PWIN32_FIND_DATA FindData   OPTIONAL
    )

/*++

Routine Description:

    Determine if a file exists and is accessible.
    Errormode is set (and then restored) so the user will not see
    any pop-ups.

Arguments:

    FileName - supplies full path of file to check for existance.

    FindData - if specified, receives find data for the file.

Return Value:

    TRUE if the file exists and is accessible.
    FALSE if not. GetLastError() returns extended error info.

--*/

{
    WIN32_FIND_DATA findData;
    HANDLE FindHandle;
    UINT OldMode;
    DWORD Error;

    OldMode = SetErrorMode(SEM_FAILCRITICALERRORS);

    FindHandle = FindFirstFile(FileName,&findData);
    if(FindHandle == INVALID_HANDLE_VALUE) {
        Error = GetLastError();
    } else {
        FindClose(FindHandle);
        if(FindData) {
            *FindData = findData;
        }
        Error = NO_ERROR;
    }

    SetErrorMode(OldMode);

    SetLastError(Error);
    return (Error == NO_ERROR);
}

