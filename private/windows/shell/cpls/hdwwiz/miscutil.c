//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       miscutil.c
//
//--------------------------------------------------------------------------

#include "HdwWiz.h"


/* ----------------------------------------------------------------------
 * SetDlgText - Set Dialog Text Field
 *
 * Concatenates a number of string resources and does a SetWindowText()
 * for a dialog text control.
 *
 * Parameters:
 *
 *  hDlg         - Dialog handle
 *  iControl     - Dialog control ID to receive text
 *  nStartString - ID of first string resource to concatenate
 *  nEndString   - ID of last string resource to concatenate
 *
 *  Note: the string IDs must be consecutive.
 */

void
SetDlgText(HWND hDlg, int iControl, int nStartString, int nEndString)
{
    int     iX;
    TCHAR   szText[MAX_PATH*4];

    szText[0] = '\0';
    for (iX = nStartString; iX<= nEndString; iX++) {
    
         LoadString(hHdwWiz,
                    iX,
                    szText + lstrlen(szText),
                    sizeof(szText)/sizeof(TCHAR) - lstrlen(szText)
                    );
    }

    if (iControl) {
    
        SetDlgItemText(hDlg, iControl, szText);

    } else {
    
        SetWindowText(hDlg, szText);
    }
}


VOID
HdwWizPropagateMessage(
    HWND hWnd,
    UINT uMessage,
    WPARAM wParam,
    LPARAM lParam
    )
{
    while ((hWnd = GetWindow(hWnd, GW_CHILD))) {
    
        SendMessage(hWnd, uMessage, wParam, lParam);
    }
}


BOOL
Use256Color(
    VOID
    )
{
    HDC hdc;
    BOOL bRetVal= FALSE;

    hdc = GetDC(NULL);
    if(hdc) {
    
        if (GetDeviceCaps(hdc, BITSPIXEL) >= 8) {
       
            bRetVal = TRUE;
        }

        ReleaseDC(NULL, hdc);
    }

    return bRetVal;
}





HPALETTE
CreateDIBPalette(
    LPBITMAPINFO lpbmi, 
    LPINT lpiNumColors
    )
{
    LPBITMAPINFOHEADER  lpbi;
    LPLOGPALETTE     lpPal;
    HANDLE           hLogPal;
    HPALETTE         hPal = NULL;
    int              i;
 
    lpbi = (LPBITMAPINFOHEADER)lpbmi;
    if (lpbi->biBitCount <= 8)
        *lpiNumColors = (1 << lpbi->biBitCount);
    else
        *lpiNumColors = 0;  // No palette needed for 24 BPP DIB
 
    if (*lpiNumColors)
    {
        hLogPal = GlobalAlloc(GHND, 
                             sizeof (LOGPALETTE) +
                             sizeof (PALETTEENTRY) * (*lpiNumColors));
        lpPal = (LPLOGPALETTE) GlobalLock (hLogPal);
        lpPal->palVersion    = 0x300;
        lpPal->palNumEntries = (unsigned short)(*lpiNumColors);
 
        for (i = 0;  i < *lpiNumColors;  i++)
        {
            lpPal->palPalEntry[i].peRed   = lpbmi->bmiColors[i].rgbRed;
            lpPal->palPalEntry[i].peGreen = lpbmi->bmiColors[i].rgbGreen;
            lpPal->palPalEntry[i].peBlue  = lpbmi->bmiColors[i].rgbBlue;
            lpPal->palPalEntry[i].peFlags = 0;
        }
        hPal = CreatePalette (lpPal);
        GlobalUnlock (hLogPal);
        GlobalFree   (hLogPal);
    }
    return hPal;
}


BOOL
LoadBitmapAndPalette(
    HINSTANCE hInstance,
    LPCTSTR pszResource,
    PRESOURCEBITMAP ResourceBitmap
    )
{
    HBITMAP hBitmap = NULL;
    HPALETTE hPalette = NULL;
    LPBITMAPINFOHEADER  lpbi;

    HDC    hdc;
    HRSRC  hRsrc;
    HGLOBAL hGlobal;

    int iNumColors;

    hRsrc = FindResource(hInstance, pszResource, RT_BITMAP);
    if (hRsrc) {
        hGlobal = LoadResource(hInstance, hRsrc);
        lpbi = (LPBITMAPINFOHEADER)LockResource(hGlobal);

        hdc = GetDC(NULL);

        hPalette = CreateDIBPalette ((LPBITMAPINFO)lpbi, &iNumColors);
        if (hPalette) {
            SelectPalette(hdc, hPalette, TRUE);
            RealizePalette(hdc);
            }
 
        hBitmap = CreateDIBitmap(hdc,
                                 (LPBITMAPINFOHEADER)lpbi,
                                 (LONG)CBM_INIT,
                                 (LPSTR)lpbi + lpbi->biSize + iNumColors * sizeof(RGBQUAD),
                                 (LPBITMAPINFO)lpbi,
                                 DIB_RGB_COLORS
                                 );
 
        UnlockResource(hGlobal);
        FreeResource(hGlobal);
        }

    if (!hBitmap || !hPalette) {
        if (hBitmap)
            DeleteObject(hBitmap);

        if (hPalette)
            DeleteObject(hPalette);

        return FALSE;
        }

    ResourceBitmap->hBitmap = hBitmap;
    ResourceBitmap->hPalette = hPalette;
    GetObject(hBitmap, sizeof(ResourceBitmap->Bitmap), &ResourceBitmap->Bitmap);

    return TRUE;
}



void
HideWindowByMove(
    HWND hDlg
    )
{
    RECT rect;
    GetWindowRect(hDlg, &rect);

    MoveWindow(hDlg,
               0,
               -(rect.bottom - rect.top),
               rect.right - rect.left,
               rect.bottom - rect.top,
               FALSE
               );
}




LONG
HdwBuildClassInfoList(
    PHARDWAREWIZ HardwareWiz,
    DWORD ClassListFlags,
    PTCHAR MachineName
    )
{
    LONG Error;

    while (!SetupDiBuildClassInfoListEx(ClassListFlags,
                                        HardwareWiz->ClassGuidList,
                                        HardwareWiz->ClassGuidSize,
                                        &HardwareWiz->ClassGuidNum,
                                        MachineName,
                                        NULL
                                        )) {
                                        
        Error = GetLastError();

        if (HardwareWiz->ClassGuidList) {
        
            LocalFree(HardwareWiz->ClassGuidList);
            HardwareWiz->ClassGuidList = NULL;
        }

        if (Error == ERROR_INSUFFICIENT_BUFFER &&
            HardwareWiz->ClassGuidNum > HardwareWiz->ClassGuidSize) {
            
            HardwareWiz->ClassGuidList = LocalAlloc(LPTR, HardwareWiz->ClassGuidNum*sizeof(GUID));

            if (!HardwareWiz->ClassGuidList) {
            
                HardwareWiz->ClassGuidSize = 0;
                HardwareWiz->ClassGuidNum = 0;
                return ERROR_NOT_ENOUGH_MEMORY;
            }
            
            HardwareWiz->ClassGuidSize = HardwareWiz->ClassGuidNum;

        } else {
        
            if (HardwareWiz->ClassGuidList) {
            
                LocalFree(HardwareWiz->ClassGuidList);
            }
            
            HardwareWiz->ClassGuidSize = 0;
            HardwareWiz->ClassGuidNum = 0;
            return Error;
        }
    }

    return ERROR_SUCCESS;
}



int
HdwMessageBox(
   HWND hWnd,
   LPTSTR szIdText,
   LPTSTR szIdCaption,
   UINT Type
   )
{
   TCHAR szText[MAX_PATH];
   TCHAR szCaption[MAX_PATH];

   if (!HIWORD(szIdText)) {
       *szText = TEXT('\0');
       LoadString(hHdwWiz, LOWORD(szIdText), szText, MAX_PATH);
       szIdText = szText;
       }

   if (!HIWORD(szIdCaption)) {
       *szCaption = TEXT('\0');
       LoadString(hHdwWiz, LOWORD(szIdCaption), szCaption, MAX_PATH);
       szIdCaption = szCaption;
       }

   return MessageBox(hWnd, szIdText, szIdCaption, Type);
}





LONG
HdwUnhandledExceptionFilter(
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

BOOL
NoPrivilegeWarning(
   HWND hWnd,
   BOOL LaunchHotplug
   )
/*++

    This function checks to see if the user has SE_LOAD_DRIVER_NAME privileges
    which means they can install and load new kernel mode drivers. 
    
    If the user does NOT have this privilege then a warning is displayed telling
    them that they have insufficient privileges to install hardware on this machine.

Arguments
    
    hWnd - Parent window handle                                              
    
    LaunchHotplug - if the user does not have privileges to install/remove devices and there
                    are hotplug devices on this machine then hotplug.dll will be launched
                    if this value is TRUE.
                                              
Return Value:
    TRUE if the user does NOT have SE_LOAD_DRIVER_NAME privileges and
    FALSE if the user does have this privilege
    
--*/
{
   TCHAR szMsg[MAX_PATH];
   TCHAR szCaption[MAX_PATH];

   if (!DoesUserHavePrivilege((PCTSTR)SE_LOAD_DRIVER_NAME)) {

       if (LoadString(hHdwWiz,
                      IDS_HDWUNINSTALL_NOPRIVILEGE,
                      szMsg,
                      MAX_PATH)
          &&
           LoadString(hHdwWiz,
                      IDS_HDWWIZNAME,
                      szCaption,
                      MAX_PATH))
        {
            MessageBox(hWnd, szMsg, szCaption, MB_OK | MB_ICONINFORMATION);

            //
            // If the machine has any hot unpluggable devices then lauch the hotplug applet so the 
            // user can unplug any devices.
            //
            if (LaunchHotplug && AnyHotPlugDevices()) {

                const TCHAR szOpen[]    = TEXT ("open");
                const TCHAR szRunDLL[]  = TEXT ("RUNDLL32.EXE");
                const TCHAR szRunCmd[]  = TEXT ("shell32.dll,Control_RunDLL hotplug.dll");

                ShellExecute(NULL, szOpen, szRunDLL, szRunCmd, NULL, SW_SHOWNORMAL);
            }
        }

       return TRUE;
    }

   return FALSE;
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

void
LoadText(
    PTCHAR szText, 
    int SizeText, 
    int nStartString, 
    int nEndString
    )
{
    int     iX;

    for (iX = nStartString; iX<= nEndString; iX++) {

        LoadString(hHdwWiz,
                   iX,
                   szText + lstrlen(szText),
                   SizeText/sizeof(TCHAR) - lstrlen(szText)
                   );
    }

    return;
}


/*  InstallFailedWarning
 *
 *  Displays device install failed warning in a message box.  For use
 *  when installation fails.
 *
 */
void
InstallFailedWarning(
    HWND    hDlg,
    PHARDWAREWIZ HardwareWiz
    )
{
    int len;
    TCHAR szMsg[MAX_MESSAGE_STRING];
    TCHAR szTitle[MAX_MESSAGE_TITLE];
    PTCHAR ErrorMsg;

    LoadString(hHdwWiz,
               IDS_ADDNEWDEVICE,
               szTitle,
               sizeof(szTitle)/sizeof(TCHAR)
               );

    if ((len = LoadString(hHdwWiz, IDS_HDW_ERRORFIN1, szMsg, sizeof(szMsg)/sizeof(TCHAR)))) {
    
        LoadString(hHdwWiz, IDS_HDW_ERRORFIN2, szMsg+len, sizeof(szMsg)/sizeof(TCHAR)-len);
    }

    if (FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
                      NULL,
                      HRESULT_FROM_SETUPAPI(HardwareWiz->LastError),
                      0,
                      (LPTSTR)&ErrorMsg,
                      0,
                      NULL
                      ))
    {
        lstrcat(szMsg, TEXT("\n\n"));

        if ((lstrlen(szMsg) + lstrlen(ErrorMsg)) < SIZECHARS(szMsg)) {
        
            lstrcat(szMsg, ErrorMsg);
        }

        LocalFree(ErrorMsg);
    }

    MessageBox(hDlg, szMsg, szTitle, MB_OK | MB_TASKMODAL | MB_ICONEXCLAMATION);
}


void
SetDriverDescription(
    HWND hDlg,
    int iControl,
    PHARDWAREWIZ HardwareWiz
    )
{
    CONFIGRET ConfigRet;
    ULONG  ulSize;
    PTCHAR FriendlyName;
    PTCHAR Location;
    SP_DRVINFO_DATA DriverInfoData;


    //
    // If there is a selected driver use its driver description,
    // since this is what the user is going to install.
    //

    DriverInfoData.cbSize = sizeof(SP_DRVINFO_DATA);
    if (SetupDiGetSelectedDriver(HardwareWiz->hDeviceInfo,
                                 &HardwareWiz->DeviceInfoData,
                                 &DriverInfoData
                                 )
        &&
        *DriverInfoData.Description) {

        wcscpy(HardwareWiz->DriverDescription, DriverInfoData.Description);
        SetDlgItemText(hDlg, iControl, HardwareWiz->DriverDescription);
        return;
    }


    FriendlyName = BuildFriendlyName(HardwareWiz->DeviceInfoData.DevInst, NULL);
    if (FriendlyName) {

        SetDlgItemText(hDlg, iControl, FriendlyName);
        LocalFree(FriendlyName);
        return;
    }

    SetDlgItemText(hDlg, iControl, szUnknown);

    return;
}

HPROPSHEETPAGE
CreateWizExtPage(
   int PageResourceId,
   DLGPROC pfnDlgProc,
   PHARDWAREWIZ HardwareWiz
   )
{
    PROPSHEETPAGE    psp;

    memset(&psp, 0, sizeof(PROPSHEETPAGE));
    psp.dwSize = sizeof(PROPSHEETPAGE);
    psp.dwFlags = PSP_DEFAULT;
    psp.hInstance = hHdwWiz;
    psp.lParam = (LPARAM)HardwareWiz;
    psp.pszTemplate = MAKEINTRESOURCE(PageResourceId);
    psp.pfnDlgProc = pfnDlgProc;

    return CreatePropertySheetPage(&psp);
}

BOOL
AddClassWizExtPages(
   HWND hwndParentDlg,
   PHARDWAREWIZ HardwareWiz,
   PSP_NEWDEVICEWIZARD_DATA DeviceWizardData,
   DI_FUNCTION InstallFunction
   )
{
    DWORD NumPages;
    BOOL bRet = FALSE;

    memset(DeviceWizardData, 0, sizeof(SP_NEWDEVICEWIZARD_DATA));
    DeviceWizardData->ClassInstallHeader.InstallFunction = InstallFunction;
    DeviceWizardData->ClassInstallHeader.cbSize = sizeof(SP_CLASSINSTALL_HEADER);
    DeviceWizardData->hwndWizardDlg = hwndParentDlg;

    if (SetupDiSetClassInstallParams(HardwareWiz->hDeviceInfo,
                                     &HardwareWiz->DeviceInfoData,
                                     &DeviceWizardData->ClassInstallHeader,
                                     sizeof(SP_NEWDEVICEWIZARD_DATA)
                                     )
        &&
        
        (SetupDiCallClassInstaller(InstallFunction,
                                  HardwareWiz->hDeviceInfo,
                                  &HardwareWiz->DeviceInfoData
                                  )

            ||

            (ERROR_DI_DO_DEFAULT == GetLastError()))
            
        &&
        SetupDiGetClassInstallParams(HardwareWiz->hDeviceInfo,
                                     &HardwareWiz->DeviceInfoData,
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

   SetupDiSetClassInstallParams(HardwareWiz->hDeviceInfo,
                                &HardwareWiz->DeviceInfoData,
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
IsDeviceHidden(
    PHARDWAREWIZ HardwareWiz,
    PSP_DEVINFO_DATA DeviceInfoData
    )
{
    BOOL bHidden = FALSE;
    ULONG DevNodeStatus, DevNodeProblem;
    HKEY hKeyClass;

    //
    // If the DN_NO_SHOW_IN_DM status bit is set
    // then we should hide this device.
    //
    if ((CM_Get_DevNode_Status(&DevNodeStatus,
                              &DevNodeProblem,
                              DeviceInfoData->DevInst,
                              0) == CR_SUCCESS) &&
        (DevNodeStatus & DN_NO_SHOW_IN_DM)) {

        bHidden = TRUE;
        goto HiddenDone;
    }        

    //
    // If the devices class has the NoDisplayClass value then
    // don't display this device.
    //
    if (hKeyClass = SetupDiOpenClassRegKeyEx(&DeviceInfoData->ClassGuid,
                                 KEY_READ,
                                 DIOCR_INSTALLER,
                                 HardwareWiz->hMachine ? HardwareWiz->MachineName : NULL,
                                 NULL)) {

        if (RegQueryValueEx(hKeyClass, REGSTR_VAL_NODISPLAYCLASS, NULL, NULL, NULL, NULL) == ERROR_SUCCESS) {

            bHidden = TRUE;
        }

        RegCloseKey(hKeyClass);
    }                                 
                          
    

HiddenDone:
    return bHidden;
}

BOOL
IsDeviceUninstallable(
    PHARDWAREWIZ HardwareWiz,
    PSP_DEVINFO_DATA DeviceInfoData
    )
{
    DWORD DevNodeStatus, DevNodeProblem;

    if (CR_SUCCESS == CM_Get_DevNode_Status(&DevNodeStatus,
                                            &DevNodeProblem,
                                            DeviceInfoData->DevInst,
                                            0) &&
        (!(DevNodeStatus & DN_DISABLEABLE)) &&
        (DevNodeStatus & DN_ROOT_ENUMERATED)) {

        return FALSE;
    }

    return TRUE;
}

BOOL
_AnyHotPlugDevices(
    DEVINST      DeviceInstance
    )
{
    ULONG      Len;
    DWORD      Capabilities, Status, Problem;
    CONFIGRET  ConfigRet;
    DEVINST    ChildDeviceInstance;


    do {
        //
        // Device capabilities
        //

        Len = sizeof(Capabilities);
        ConfigRet = CM_Get_DevNode_Registry_Property(DeviceInstance,
                                                     CM_DRP_CAPABILITIES,
                                                     NULL,
                                                     (PVOID)&Capabilities,
                                                     &Len,
                                                     0
                                                     );
        if (ConfigRet != CR_SUCCESS) {
            
            Capabilities = 0;
        }

        ConfigRet = CM_Get_DevNode_Status(&Status,
                                          &Problem,
                                          DeviceInstance,
                                          0
                                          );

        if (ConfigRet != CR_SUCCESS) {

            Status = 0;
            Problem = 0;
        }

        //
        // Only show devices that:
        //  -> do NOT have problem CM_PROB_DEVICE_NOT_THERE
        //  -> are removable
        //  -> are NOT surprise removable
        //
        if ((Problem != CM_PROB_DEVICE_NOT_THERE) &&
            (Capabilities & CM_DEVCAP_REMOVABLE) &&
            !(Capabilities & CM_DEVCAP_SURPRISEREMOVALOK)) {

            //
            // As soon as we find one device then we need to show the icon, so no need
            // to continue the search.
            //
            return TRUE;
        }

        //
        // If this devinst has children, then recurse to fill in its
        // child sibling list.
        //

        ConfigRet = CM_Get_Child(&ChildDeviceInstance,
                                 DeviceInstance,
                                 0
                                 );

        if (ConfigRet == CR_SUCCESS) {
            
            if (_AnyHotPlugDevices(ChildDeviceInstance)) {
                
                return TRUE;
            }
        }

        //
        // Next sibling ...
        //

        ConfigRet = CM_Get_Sibling(&DeviceInstance,
                                      DeviceInstance,
                                      0
                                      );

    } while (ConfigRet == CR_SUCCESS);


    return FALSE;
}

BOOL
AnyHotPlugDevices(
    void
    )
{
    CONFIGRET ConfigRet;
    DEVNODE DeviceInstance, ChildDeviceInstance;

    ConfigRet = CM_Locate_DevNode(&DeviceInstance,
                                 NULL,
                                 CM_LOCATE_DEVNODE_NORMAL
                                 );
    
    if (ConfigRet != CR_SUCCESS) {
        
        return FALSE;
    }

    ConfigRet = CM_Get_Child(&ChildDeviceInstance,
                             DeviceInstance,
                             0
                             );

    if (ConfigRet != CR_SUCCESS) {
        
        return FALSE;
    }

    return (_AnyHotPlugDevices(ChildDeviceInstance));
}

#if DBG
//
// Debugging aids
//
void
Trace(
    LPCTSTR format,
    ...
    )
{
    // according to wsprintf specification, the max buffer size is
    // 1024
    TCHAR Buffer[1024];
    va_list arglist;
    va_start(arglist, format);
    wvsprintf(Buffer, format, arglist);
    va_end(arglist);
    OutputDebugString(TEXT("HDWWIZ: "));
    OutputDebugString(Buffer);
    OutputDebugString(TEXT("\n"));
}
#endif


