/******************************************************************************

  Source File:  deskadp.cpp

  Main code for the advanced desktop adapter page

  Copyright (c) 1997-1998 by Microsoft Corporation

  Change History:

  12-16-97 AndreVa - Created It

******************************************************************************/


#include    "deskadp.h"

DWORD ApplyNowThd(LPVOID lpThreadParameter);

//
// The function DevicePropertiesW() is implemented in DevMgr.dll; Since we don't have a devmgr.h, we
// explicitly declare it here.
//
typedef int (WINAPI  *DEVPROPERTIESW)(
    HWND hwndParent,
    LPCTSTR MachineName,
    LPCTSTR DeviceID,
    BOOL ShowDeviceTree
    );

// OLE-Registry magic number
// 42071712-76d4-11d1-8b24-00a0c9068ff3
//
GUID g_CLSID_CplExt = { 0x42071712, 0x76d4, 0x11d1,
                        { 0x8b, 0x24, 0x00, 0xa0, 0xc9, 0x06, 0x8f, 0xf3}
                      };

DESK_EXTENSION_INTERFACE DeskInterface;

static const DWORD sc_AdapterHelpIds[] =
{
    ID_ADP_ADPINFGRP,  IDH_DISPLAY_SETTINGS_ADVANCED_ADAPTER_ADAPTER_INFO,
    ID_ADP_AI1,        IDH_DISPLAY_SETTINGS_ADVANCED_ADAPTER_ADAPTER_INFO,
    ID_ADP_AI2,        IDH_DISPLAY_SETTINGS_ADVANCED_ADAPTER_ADAPTER_INFO,
    ID_ADP_AI3,        IDH_DISPLAY_SETTINGS_ADVANCED_ADAPTER_ADAPTER_INFO,
    ID_ADP_AI4,        IDH_DISPLAY_SETTINGS_ADVANCED_ADAPTER_ADAPTER_INFO,
    ID_ADP_AI5,        IDH_DISPLAY_SETTINGS_ADVANCED_ADAPTER_ADAPTER_INFO,
    ID_ADP_CHIP,       IDH_DISPLAY_SETTINGS_ADVANCED_ADAPTER_ADAPTER_INFO,
    ID_ADP_DAC,        IDH_DISPLAY_SETTINGS_ADVANCED_ADAPTER_ADAPTER_INFO,
    ID_ADP_MEM,        IDH_DISPLAY_SETTINGS_ADVANCED_ADAPTER_ADAPTER_INFO,
    ID_ADP_ADP_STRING, IDH_DISPLAY_SETTINGS_ADVANCED_ADAPTER_ADAPTER_INFO,
    ID_ADP_BIOS_INFO,  IDH_DISPLAY_SETTINGS_ADVANCED_ADAPTER_ADAPTER_INFO,

    ID_ADP_ADPGRP,     IDH_DISPLAY_SETTINGS_ADVANCED_ADAPTER_ADAPTER_TYPE,
    IDI_ADAPTER,       IDH_DISPLAY_SETTINGS_ADVANCED_ADAPTER_ADAPTER_TYPE,
    ID_ADP_ADAPTOR,    IDH_DISPLAY_SETTINGS_ADVANCED_ADAPTER_ADAPTER_TYPE,

    IDC_LIST_ALL,      IDH_DISPLAY_SETTINGS_ADVANCED_ADAPTER_LIST_MODES,
    IDC_PROPERTIES,    IDH_DISPLAY_SETTINGS_ADVANCED_ADAPTER_PROPERTIES,

    0, 0
};

static const DWORD sc_ListAllHelpIds[] = 
{
    ID_MODE_LIST,    IDH_DISPLAY_SETTINGS_ADVANCED_ADAPTER_LISTMODE_DIALOGBOX,
    ID_MODE_LISTGRP, IDH_DISPLAY_SETTINGS_ADVANCED_ADAPTER_LISTMODE_DIALOGBOX,

    0, 0
};


///////////////////////////////////////////////////////////////////////////////
//
// Messagebox wrapper
//
///////////////////////////////////////////////////////////////////////////////


int
FmtMessageBox(
    HWND hwnd,
    UINT fuStyle,
    DWORD dwTitleID,
    DWORD dwTextID)
{
    TCHAR Title[256];
    TCHAR Text[1500];

    LoadString(g_hInst, dwTextID, Text, SIZEOF(Text));
    LoadString(g_hInst, dwTitleID, Title, SIZEOF(Title));

    return (MessageBox(hwnd, Text, Title, fuStyle));
}



///////////////////////////////////////////////////////////////////////////////
//
// Main dialog box Windows Proc
//
///////////////////////////////////////////////////////////////////////////////
INT_PTR
CALLBACK
ListAllModesProc(
    HWND hDlg,
    UINT uMessage,
    WPARAM wParam,
    LPARAM lParam
    )
{
    LPDEVMODEW lpdm, lpdmCur; 
    HWND hList;
    DWORD i;
    LRESULT item;

    switch (uMessage)
    {
    case WM_INITDIALOG:

        //
        // Save the lParam - we will store the new mode here
        //
        SetWindowLongPtr(hDlg, DWLP_USER, lParam);
        lpdmCur = *((LPDEVMODEW *)lParam);
        Assert(lpdmCur != NULL);

        //
        // Build the list of modes to display
        //
        i = 0;
        hList = GetDlgItem(hDlg, ID_MODE_LIST); 
        while (lpdm = DeskInterface.lpfnEnumAllModes(DeskInterface.pContext, i))
        {
            TCHAR  achFreData[50];
            TCHAR  achFre[50];
            TCHAR  achStr[80];
            TCHAR  achText[120];
            DWORD  idColor;
            DWORD  idFreq;

            //
            // convert bit count to number of colors and make it a string
            //

            switch (lpdm->dmBitsPerPel)
            {
            case 32: idColor = IDS_MODE_TRUECOLOR32; break;
            case 24: idColor = IDS_MODE_TRUECOLOR24; break;
            case 16: idColor = IDS_MODE_16BIT_COLOR; break;
            case 15: idColor = IDS_MODE_15BIT_COLOR; break;
            case  8: idColor = IDS_MODE_8BIT_COLOR; break;
            case  4: idColor = IDS_MODE_4BIT_COLOR; break;
            default:
                FmtMessageBox(hDlg,
                              MB_OK | MB_ICONINFORMATION,
                              IDS_BAD_COLOR,
                              IDS_BAD_COLOR);

                EndDialog(hDlg, -1);
                break;
            }

            if (lpdm->dmDisplayFrequency == 0)
            {
                FmtMessageBox(hDlg,
                              MB_OK | MB_ICONINFORMATION,
                              IDS_BAD_REFRESH,
                              IDS_BAD_REFRESH);

                EndDialog(hDlg, -1);
                break;
            }
            else if (lpdm->dmDisplayFrequency == 1)
            {
                LoadString(g_hInst, IDS_MODE_REFRESH_DEF, achFre, sizeof(achFre));
            }
            else
            {
                if (lpdm->dmDisplayFrequency <= 50)
                    idFreq = IDS_MODE_REFRESH_INT;
                else
                    idFreq = IDS_MODE_REFRESH_HZ;

                LoadString(g_hInst, idFreq, achFreData, sizeof(achFreData));
                wsprintf(achFre, achFreData, lpdm->dmDisplayFrequency);
            }

            LoadString(g_hInst, idColor, achStr, sizeof(achStr));
            wsprintf(achText, achStr, lpdm->dmPelsWidth, lpdm->dmPelsHeight, achFre);

            item = ListBox_AddString(hList, achText);
            if (lpdm == lpdmCur) 
                ListBox_SetCurSel(hList, item);
            ListBox_SetItemData(hList, item, lpdm);

            i++;
        }

        //
        // If no modes are available, put up a popup and exit.
        //

        if (i == 0)
        {
            EndDialog(hDlg, -1);
        }


        break;

    case WM_COMMAND:

        switch (LOWORD(wParam))
        {
        case ID_MODE_LIST:

            if (HIWORD(wParam) != LBN_DBLCLK)
            {
                return FALSE;
            }

            //
            // fall through, as DBLCLK means select.
            //

        case IDOK:

            //
            // Save the mode back
            //

            item = SendDlgItemMessage(hDlg, ID_MODE_LIST, LB_GETCURSEL, 0, 0);

            if ((item != LB_ERR) &&
                (lpdm = (LPDEVMODEW) SendDlgItemMessage(hDlg, ID_MODE_LIST, LB_GETITEMDATA, item, 0)))
            {
                *((LPDEVMODEW *)GetWindowLongPtr(hDlg, DWLP_USER)) = lpdm;
                EndDialog(hDlg, TRUE);
                break;
            }

            //
            // fall through
            //

        case IDCANCEL:

            EndDialog(hDlg, FALSE);
            break;

        default:

            return FALSE;
        }

        break;

    case WM_HELP:

        WinHelp((HWND)((LPHELPINFO)lParam)->hItemHandle,
                TEXT("display.hlp"),
                HELP_WM_HELP,
                (DWORD_PTR)(LPTSTR)sc_ListAllHelpIds);

        break;

    case WM_CONTEXTMENU:

        WinHelp((HWND)wParam,
                TEXT("display.hlp"),
                HELP_CONTEXTMENU,
                (DWORD_PTR)(LPTSTR)sc_ListAllHelpIds);

        break;

    default:

        return FALSE;
    }

    return TRUE;

}

void Adaptor_OnApply(HWND hDlg)
{
    HINSTANCE               hInst;
    LPDISPLAY_SAVE_SETTINGS lpfnDisplaySaveSettings = NULL;
    long                    lRet = PSNRET_INVALID_NOCHANGEPAGE;

    hInst = LoadLibrary(TEXT("desk.cpl"));
    if (hInst)
    {
        lpfnDisplaySaveSettings = (LPDISPLAY_SAVE_SETTINGS)
                                  GetProcAddress(hInst, "DisplaySaveSettings");
        if (lpfnDisplaySaveSettings)
        {
            LONG lSave = lpfnDisplaySaveSettings(DeskInterface.pContext, hDlg);
            if (lSave == DISP_CHANGE_SUCCESSFUL)
            {
                //
                // Save the current mode - to restore it in case the user cancels the p. sheet
                //
                LPDEVMODEW lpdmOnCancel = DeskInterface.lpfnGetSelectedMode(DeskInterface.pContext);
                SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR)lpdmOnCancel);
                lRet = PSNRET_NOERROR;
            }
            else if (lSave == DISP_CHANGE_RESTART)
            {
                //
                // User wants to reboot system.
                //
                PropSheet_RestartWindows(GetParent(hDlg));
                lRet = PSNRET_NOERROR;
            }
        }

        FreeLibrary(hInst);
    }

    SetWindowLongPtr(hDlg, DWLP_MSGRESULT, lRet);
}

void Adaptor_OnInitDialog(HWND hDlg)
{
    //
    // Get the CPL extension interfaces from IDataObject.
    //
    FORMATETC fmte = {(CLIPFORMAT)RegisterClipboardFormat(DESKCPLEXT_INTERFACE),
                      (DVTARGETDEVICE FAR *) NULL,
                      DVASPECT_CONTENT,
                      -1,
                      TYMED_HGLOBAL};

    STGMEDIUM stgm;

    HRESULT hres = g_lpdoTarget->GetData(&fmte, &stgm);

    if (SUCCEEDED(hres) && stgm.hGlobal)
    {
        //
        // The storage now contains Display device path (\\.\DisplayX) in UNICODE.
        //

        PDESK_EXTENSION_INTERFACE pInterface =
            (PDESK_EXTENSION_INTERFACE) GlobalLock(stgm.hGlobal);

        RtlCopyMemory(&DeskInterface,
                      pInterface,
                      min(pInterface->cbSize,
                          sizeof(DESK_EXTENSION_INTERFACE)));

        GlobalUnlock(stgm.hGlobal);
        
        ReleaseStgMedium(&stgm);

        SendDlgItemMessageW(hDlg, ID_ADP_CHIP,       WM_SETTEXT, 0, (LPARAM)&(DeskInterface.Info.ChipType[0]));
        SendDlgItemMessageW(hDlg, ID_ADP_DAC,        WM_SETTEXT, 0, (LPARAM)&(DeskInterface.Info.DACType[0]));
        SendDlgItemMessageW(hDlg, ID_ADP_MEM,        WM_SETTEXT, 0, (LPARAM)&(DeskInterface.Info.MemSize[0]));
        SendDlgItemMessageW(hDlg, ID_ADP_ADP_STRING, WM_SETTEXT, 0, (LPARAM)&(DeskInterface.Info.AdapString[0]));
        SendDlgItemMessageW(hDlg, ID_ADP_BIOS_INFO,  WM_SETTEXT, 0, (LPARAM)&(DeskInterface.Info.BiosString[0]));
    }

    //
    // Get device description from IDataObject.
    //

    LPWSTR pDeviceDescription;

    FORMATETC fmte2 = {(CLIPFORMAT)RegisterClipboardFormat(DESKCPLEXT_DISPLAY_NAME),
                       (DVTARGETDEVICE FAR *) NULL,
                       DVASPECT_CONTENT,
                       -1,
                       TYMED_HGLOBAL};

    hres = g_lpdoTarget->GetData(&fmte2, &stgm);

    if (SUCCEEDED(hres) && stgm.hGlobal)
    {
        pDeviceDescription = (LPWSTR) GlobalLock(stgm.hGlobal);

        SendDlgItemMessageW(hDlg, ID_ADP_ADAPTOR, WM_SETTEXT, 0, (LPARAM)pDeviceDescription);

        GlobalUnlock(stgm.hGlobal);
        
        ReleaseStgMedium(&stgm);
    }
    
    //
    // Save the initial selected mode - to restore it in case the user cancels the p. sheet
    //
    LPDEVMODEW lpdmOnCancel = DeskInterface.lpfnGetSelectedMode(DeskInterface.pContext);
    SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR)lpdmOnCancel);
}

void Adaptor_OnCancel(HWND hDlg)
{
    //
    // Restore initial mode
    //
    LPDEVMODEW lpdmOnCancel = (LPDEVMODEW) GetWindowLongPtr(hDlg, DWLP_USER);
    DeskInterface.lpfnSetSelectedMode(DeskInterface.pContext, lpdmOnCancel);
}

void Adaptor_OnListAllModes(HWND hDlg)
{
    LPDEVMODEW lpdmBefore, lpdmAfter;

    lpdmAfter = lpdmBefore = DeskInterface.lpfnGetSelectedMode(DeskInterface.pContext);
    if (DialogBoxParam(g_hInst,
                       MAKEINTRESOURCE(DLG_SET_MODE_LIST),
                       hDlg,
                       ListAllModesProc,
                       (LPARAM) &lpdmAfter) == 1 &&
        lpdmAfter && (lpdmAfter != lpdmBefore)) 
    {

        //
        // If the user selected a new setting, tell the property sheet
        // we have outstanding changes. This will enable the Apply button.
        //
        PropSheet_Changed(GetParent(hDlg), hDlg);
        DeskInterface.lpfnSetSelectedMode(DeskInterface.pContext, lpdmAfter);
    }
}

void Adaptor_OnProperties(HWND hDlg)
{
    // Invoke the device manager property sheets to show the properties of the
    // given hardware.

    LPWSTR pwDeviceID;
    HRESULT hres;
    STGMEDIUM stgm;

    FORMATETC fmte2 = {(CLIPFORMAT)RegisterClipboardFormat(DESKCPLEXT_DISPLAY_ID),
                   (DVTARGETDEVICE FAR *) NULL,
                   DVASPECT_CONTENT,
                   -1,
                   TYMED_HGLOBAL};

    hres = g_lpdoTarget->GetData(&fmte2, &stgm);

    if (SUCCEEDED(hres) && stgm.hGlobal)
    {
        pwDeviceID = (LPWSTR) GlobalLock(stgm.hGlobal);

        HINSTANCE hinstDevMgr = LoadLibrary(TEXT("DEVMGR.DLL"));
        if (hinstDevMgr)
        {
            DEVPROPERTIESW pfnDevPropW =
               (DEVPROPERTIESW)GetProcAddress(hinstDevMgr, "DevicePropertiesW");
            if (pfnDevPropW)
            {
                //Display the property sheets for this device.
                (*pfnDevPropW)(hDlg, NULL, pwDeviceID, FALSE);
            }

            FreeLibrary(hinstDevMgr);
        }

        GlobalUnlock(stgm.hGlobal);
        ReleaseStgMedium(&stgm);
    }
}

#if TEST_MODE
void Adaptor_OnTestMode(HWND hDlg)
{
    //
    // Warn the user
    //
    if (FmtMessageBox(hDlg,
                      MB_OKCANCEL | MB_ICONINFORMATION,
                      IDS_TEST_MODE,
                      IDS_TEST_WARNING) == IDCANCEL) {
        return;
    }

    //
    // The user wants to test his new selection.  We need to
    // create a new desktop with the selected resolution, switch
    // to it, and put up a dialog box so he can see if it works with
    // his hardware.
    //
    // All this needs to be done in a separate thread since you can't
    // switch a thread with open windows to a new desktop.
    //

    //
    // Create the test thread.  It will do the work of creating the desktop
    //

    //
    // Get device name from IDataObject.
    //
    LPWSTR pDeviceName = NULL;

    FORMATETC fmte = {(CLIPFORMAT)RegisterClipboardFormat(DESKCPLEXT_DISPLAY_DEVICE),
                      (DVTARGETDEVICE FAR *) NULL,
                      DVASPECT_CONTENT,
                      -1,
                      TYMED_HGLOBAL};

    STGMEDIUM stgm;

    HRESULT hres = g_lpdoTarget->GetData(&fmte, &stgm);

    if (SUCCEEDED(hres) && stgm.hGlobal) {
        //
        // The storage now contains Display device path (\\.\DisplayX) in UNICODE.
        //

        pDeviceName = (LPWSTR) GlobalLock(stgm.hGlobal);
    }

    desktopParam.lpdevmode = DeskInterface.lpfnGetSelectedMode(DeskInterface.pContext);
    desktopParam.pwszDevice = pDeviceName;

    hThread = CreateThread(NULL,
                           4096,
                           ApplyNowThd,
                           (LPVOID) (&desktopParam),
                           SYNCHRONIZE | THREAD_QUERY_INFORMATION,
                           &idThread);

    WaitForSingleObject(hThread, INFINITE);

    GetExitCodeThread(hThread, &bTest);

    //
    // clean up memory
    //
    if (pDeviceName)
    {
        GlobalUnlock(stgm.hGlobal);
        ReleaseStgMedium(&stgm);
    }

    CloseHandle(hThread);

    if (!bTest)
    {
        FmtMessageBox(hDlg,
                      MB_ICONEXCLAMATION | MB_OK,
                      IDS_TEST_MODE,
                      IDS_TEST_FAILED);

    }

    SetForegroundWindow(hDlg);
}
#endif


//---------------------------------------------------------------------------
//
// PropertySheeDlgProc()
//
//  The dialog procedure for the "Adapter" property sheet page.
//
//---------------------------------------------------------------------------
BOOL
CALLBACK
PropertySheeDlgProc(
    HWND hDlg,
    UINT uMessage,
    WPARAM wParam,
    LPARAM lParam
    )
{
    switch (uMessage)
    {
    case WM_INITDIALOG:

        if (!g_lpdoTarget)
        {
            return FALSE;
        }
        else
        {
            Adaptor_OnInitDialog(hDlg);
        }

        break;

    case WM_COMMAND:

        switch( LOWORD(wParam) )
        {
#if TEST_MODE
        case IDC_TEST_MODE:
            Adaptor_OnTestMode(hDlg);
            break;
#endif

        case IDC_LIST_ALL:
            Adaptor_OnListAllModes(hDlg);
            break;

        case IDC_PROPERTIES:
            Adaptor_OnProperties(hDlg);
            break;

        default:
            return FALSE;
        }

        break;

    case WM_NOTIFY:

        switch (((NMHDR FAR *)lParam)->code)
        {
        case PSN_APPLY: 
            Adaptor_OnApply(hDlg);
            break;

        case PSN_RESET:
            Adaptor_OnCancel(hDlg);
            break;
            
        default:
            return FALSE;
        }

        break;

    case WM_HELP:

        WinHelp((HWND)((LPHELPINFO)lParam)->hItemHandle,
                TEXT("display.hlp"),
                HELP_WM_HELP,
                (DWORD_PTR)(LPTSTR)sc_AdapterHelpIds);

        break;

    case WM_CONTEXTMENU:

        WinHelp((HWND)wParam,
                TEXT("display.hlp"),
                HELP_CONTEXTMENU,
                (DWORD_PTR)(LPTSTR)sc_AdapterHelpIds);

        break;

    default:

        return FALSE;
    }

    return TRUE;
}


//
// This functionality has been deprectaed by the new style of testing a video 
// settings (apply the changes, bring up a countdown dialog to confirm).  If
// need to dipslay a bmp still exists, import TestDisplaySettings from desk.cpl
// via LoadLibrary and GetProcAddress...
//
#if TEST_MODE

/****************************************************************************\
*
* DWORD WINAPI ApplyNowThd(LPVOID lpThreadParameter)
*
* Thread that gets started when the use hits the Apply Now button.
* This thread creates a new desktop with the new video mode, switches to it
* and then displays a dialog box asking if the display looks OK.  If the
* user does not respond within the time limit, then 'NO' is assumed to be
* the answer.
*
\****************************************************************************/
DWORD ApplyNowThd(LPVOID lpThreadParameter)
{

    PNEW_DESKTOP_PARAM lpDesktopParam = (PNEW_DESKTOP_PARAM) lpThreadParameter;
    HDESK hdsk = NULL;
    HDESK hdskDefault = NULL;
    BOOL bTest = FALSE;
    HDC hdc;

    //
    // HACK:
    // We need to make a USER call before calling the desktop stuff so we can
    // sure our threads internal data structure are associated with the default
    // desktop.
    // Otherwise USER has problems closing the desktop with our thread on it.
    //

    GetSystemMetrics(SM_CXSCREEN);

    //
    // Create the desktop
    //

    hdskDefault = GetThreadDesktop(GetCurrentThreadId());

    if (hdskDefault != NULL) {

        hdsk = CreateDesktopW(L"Display.Cpl Desktop",
                              lpDesktopParam->pwszDevice,
                              lpDesktopParam->lpdevmode,
                              0,
                              MAXIMUM_ALLOWED,
                              NULL);

        if (hdsk != NULL) {

            //
            // use the desktop for this thread
            //

            if (SetThreadDesktop(hdsk)) {

                hdc = CreateDC(TEXT("DISPLAY"), NULL, NULL, NULL);

                if (hdc) {

                    DrawBmp(hdc);

                    DeleteDC(hdc);

                    bTest = TRUE;
                }

                //
                // Sleep for some seconds so you have time to look at the screen.
                //

                Sleep(7000);

            }
        }


        //
        // Reset the thread to the right desktop
        //

        SetThreadDesktop(hdskDefault);

        SwitchDesktop(hdskDefault);

        //
        // Can only close the desktop after we have switched to the new one.
        //

        if (hdsk != NULL)
            CloseDesktop(hdsk);

    }

    ExitThread((DWORD) bTest);

    return 0;
}
#endif
