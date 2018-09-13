#include "stdafx.h"

#include "systray.h"

#include <stdio.h>
#include <initguid.h>
#include <usbioctl.h>
#include <wmium.h>
#include <tchar.h>

#define USBUIMENU               100

#define NUM_HCS_TO_CHECK 10

typedef int (CALLBACK *USBERRORMESSAGESCALLBACK) 
    (PUSB_CONNECTION_NOTIFICATION,LPTSTR);

extern HINSTANCE g_hInstance;

static BOOL    g_bUSBUIEnabled = FALSE;
static BOOL    g_bUSBUIIconShown = FALSE;
static HICON   g_hUSBUIIcon = NULL;
static HINSTANCE g_hUsbWatch = NULL;
static USBERRORMESSAGESCALLBACK g_UsbHandler = NULL;

int _cdecl main(){
    return 0;
}
              
#define USBUI_OffsetToPtr(Base, Offset) ((PBYTE)((PBYTE)Base + Offset))

LPTSTR USBUI_CountedStringToSz(LPTSTR lpString)
{
   SHORT    usNameLength;
   LPTSTR  lpStringPlusNull;

   usNameLength = * (USHORT *) lpString;

   lpStringPlusNull = (LPTSTR) LocalAlloc(LMEM_ZEROINIT,
                                          sizeof(TCHAR) * (usNameLength+1));

   if (lpStringPlusNull != NULL) {
      lpString = (LPTSTR) USBUI_OffsetToPtr(lpString, sizeof(USHORT));

      wcsncpy( lpStringPlusNull, lpString, usNameLength );

      lpStringPlusNull[usNameLength] = TEXT('0');
      // _tcscpy( lpStringPlusNull + usNameLength, _TEXT("") );
   }

   return lpStringPlusNull;
}

void USBUI_EventCallbackRoutine(PWNODE_HEADER WnodeHeader, ULONG Context)
{
    PWNODE_SINGLE_INSTANCE          wNode = (PWNODE_SINGLE_INSTANCE)WnodeHeader;
    PUSB_CONNECTION_NOTIFICATION    usbConnectionNotification;
    LPGUID                          eventGuid = &WnodeHeader->Guid;	
    LPTSTR                          strInstanceName;

    if (memcmp(&GUID_USB_WMI_STD_DATA, eventGuid, sizeof(GUID)) == 0) {
        usbConnectionNotification = (PUSB_CONNECTION_NOTIFICATION)
                                    USBUI_OffsetToPtr(wNode, 
                                                      wNode->DataBlockOffset);

        //
        // Get the instance name
        //
        strInstanceName = 
            USBUI_CountedStringToSz((LPTSTR) 
                                    USBUI_OffsetToPtr(wNode,
                                                      wNode->OffsetInstanceName));
        if (strInstanceName) {
            if (g_hUsbWatch && g_UsbHandler) {
USBUIEngageHandler:                    
                g_UsbHandler(usbConnectionNotification, strInstanceName);
            } else {
                g_hUsbWatch = LoadLibrary(TEXT("usbui.dll"));
                g_UsbHandler = (USBERRORMESSAGESCALLBACK) 
                    GetProcAddress(g_hUsbWatch, "USBErrorHandler");
                goto USBUIEngageHandler;
            }
            LocalFree(strInstanceName);
        }
    }
}

int USBUI_ErrorMessagesEnable(BOOL fEnable)
{
    ULONG status;
    status = WmiNotificationRegistration((LPGUID) &GUID_USB_WMI_STD_DATA,
	                                     (BOOLEAN)fEnable,
                                         USBUI_EventCallbackRoutine,
					                     0,
                                         NOTIFICATION_CALLBACK_DIRECT);

    return status;
}

void USBUI_Notify(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    switch (lParam)
    {
	    case WM_RBUTTONUP:
        {
	        USBUI_Menu(hwnd, 1, TPM_RIGHTBUTTON);
        }
	    break;

	    case WM_LBUTTONDOWN:
	    {
            SetTimer(hwnd, USBUI_TIMER_ID, GetDoubleClickTime()+100, NULL);
	    }
        break;

	    case WM_LBUTTONDBLCLK:
	    {
            KillTimer(hwnd, USBUI_TIMER_ID);
            USBUI_Toggle();
	    }
        break;
    }
}

void USBUI_Toggle()
{
    USBUI_SetState(!g_bUSBUIEnabled);
}

void USBUI_Timer(HWND hwnd)
{
    KillTimer(hwnd, USBUI_TIMER_ID);
    USBUI_Menu(hwnd, 0, TPM_LEFTBUTTON);
}
/*
HMENU USBUI_CreateMenu()
{
    HMENU hmenu;
    LPSTR lpszMenu1;
    
    hmenu = CreatePopupMenu();

    if (!hmenu)
    {
        return NULL;
    }

    lpszMenu1 = LoadDynamicString(g_bUSBUIEnabled?IDS_USBUIDISABLE:IDS_USBUIENABLE);
    
    // AppendMenu(hmenu,MF_STRING,USBUIMENU,lpszMenu1);
	SysTray_AppendMenuString (hmenu,USBUIMENU,lpszMenu1);

    SetMenuDefaultItem(hmenu,USBUIMENU,FALSE);

    DeleteDynamicString(lpszMenu1);
        
    return hmenu;
}
  */
void USBUI_Menu(HWND hwnd, UINT uMenuNum, UINT uButton)
{
    POINT   pt;
    UINT    iCmd;
    HMENU   hmenu = 0;
    
    GetCursorPos(&pt);

//    hmenu = USBUI_CreateMenu();

    if (!hmenu)
    {
        return;
    }
    
    SetForegroundWindow(hwnd);

    iCmd = TrackPopupMenu(hmenu, uButton | TPM_RETURNCMD | TPM_NONOTIFY, pt.x, pt.y, 0, hwnd, NULL);
    
    DestroyMenu(hmenu);

    switch (iCmd)
    {
        case USBUIMENU:
        {
            USBUI_Toggle();
        }
        break;
    }
}

BOOL USBUI_SetState(BOOL On)
{
    int retValue;
    if (g_bUSBUIEnabled != On) {
        //
        // Only enable it if not already enabled
        //
        retValue = (int) USBUI_ErrorMessagesEnable (On);
        g_bUSBUIEnabled = retValue ? g_bUSBUIEnabled : On;
    }
    return g_bUSBUIEnabled;
}

BOOL
IsErrorCheckingEnabled()
{
    DWORD ErrorCheckingEnabled, type = REG_DWORD, size = sizeof(DWORD);
    HKEY hKey;

    //
    // Check the registry value ErrorCheckingEnabled to make sure that we should
    // be enabling this.
    //
    if (ERROR_SUCCESS != 
        RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                        TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Usb"),
                        0,
                        KEY_READ,
                        &hKey)) {
        return TRUE;
    }
    if (ERROR_SUCCESS != RegQueryValueEx(hKey, 
                                         TEXT("ErrorCheckingEnabled"),
                                         0,
                                         &type,
                                         (LPBYTE) &ErrorCheckingEnabled,
                                         &size)) {
        return TRUE;
    }
    return (BOOL) ErrorCheckingEnabled;
}

BOOL USBUI_Init(HWND hWnd)
{
	TCHAR       HCName[16];
    BOOL        ControllerFound = FALSE;
	int         HCNum;
    HANDLE      hHCDev;

    //
    // Check the registry to make sure that it is turned on
    //
    if (!IsErrorCheckingEnabled()) {
        return FALSE;
    }

    //
    // Check for the existence of a USB controller.
    // If there is one, load and initialize USBUI.dll which will check for
    // usb error messages.  If we can't open a controller, than we shouldn't
    // load a USB watch dll.
    //
	for (HCNum = 0; HCNum < NUM_HCS_TO_CHECK; HCNum++)
	{
        wsprintf(HCName, TEXT("\\\\.\\HCD%d"), HCNum);

        hHCDev = CreateFile(HCName,
                            GENERIC_READ,
                            FILE_SHARE_READ,
                            NULL,
                            OPEN_EXISTING,
                            0,
                            NULL);
        //
        // If the handle is valid, then we've successfully opened a Host
        // Controller.  
        //
        if (hHCDev != INVALID_HANDLE_VALUE) {
            CloseHandle(hHCDev);
            return TRUE;
        }
    }

    return FALSE;
}

//
//  Called at init time and whenever services are enabled/disabled.
//
BOOL USBUI_CheckEnable(HWND hWnd, BOOL bSvcEnabled)
{
    BOOL bEnable = bSvcEnabled && USBUI_Init(hWnd);
    
    if (bEnable != g_bUSBUIEnabled)
    {
        //
        // state change
        //
	    USBUI_SetState(bEnable);
    }

    return(bEnable);
}


