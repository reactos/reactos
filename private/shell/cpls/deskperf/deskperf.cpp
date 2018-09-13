
//
// [Display Troubleshooter Control Panel Extenstion]
//
//
// - Aug.25.1998
//
//    Created by Hideyuki Nagase [hideyukn]
// 

#include    "deskperf.h"

//
// Defines
//

#define ACCELERATION_FULL  0
#define ACCELERATION_NONE  5

#define SLIDER_POS_TO_ACCEL_LEVEL(x) (ACCELERATION_NONE - (x))
#define ACCEL_LEVEL_TO_SLIDER_POS(x) (ACCELERATION_NONE - (x))

#define SZ_ACCELLEVEL TEXT("Acceleration.Level")

//
// Guid for "Troubleshooter" shell extentions
//

GUID g_CLSID_CplExt = { 0xf92e8c40, 0x3d33, 0x11d2,
                        { 0xb1, 0xaa, 0x08, 0x00, 0x36, 0xa7, 0x5b, 0x03}
                      };

//
// Global variables
//

//
// Dos display device name
//

TCHAR gszWinDisplayDevice[MAX_PATH];

//
// NT display device name
//

TCHAR gszNtDisplayDevice[MAX_PATH];

//
// Registry path for current device
//

TCHAR gszRegistryPath[MAX_PATH];

//
// Current acceleration level.
//

DWORD AccelLevel = ACCELERATION_FULL;

//
// Last saved acceleration level.
//

DWORD AccelLevelInReg = ACCELERATION_FULL;

//
// Registry security.
//

BOOL  gbReadOnly = FALSE;

//
// Context-sentitive help
//

static const DWORD sc_PerformanceHelpIds[] =
{
   IDI_MONITOR,             IDH_NOHELP,
   IDC_DESCRIPTION,         IDH_NOHELP,
   IDC_ACCELERATION_SLIDER, IDH_SLIDER_HELP_TOPIC,
   IDC_ACCELERATION_TEXT,   IDH_SLIDER_HELP_TOPIC,
   0, 0
};

void
UpdateGraphicsText(HWND hDlg, DWORD AccelPos)
{
    TCHAR MessageBuffer[200];

    LoadString(g_hInst, IDS_LEVEL0 + AccelPos, MessageBuffer, SIZEOF(MessageBuffer));

    SetDlgItemText(hDlg, IDC_ACCELERATION_TEXT, (LPTSTR) MessageBuffer);
}

BOOL GetDeviceKey(LPCTSTR pszDisplay, LPTSTR pszDeviceKey, int cChars)
{
    DISPLAY_DEVICE DisplayDevice;
    BOOL fFound = FALSE;
    BOOL fSuccess = TRUE;
    int iEnum = 0;

    // Enumerate all the devices in the system.
    while(fSuccess && !fFound)
    {
        ZeroMemory(&DisplayDevice, sizeof(DISPLAY_DEVICE));
        DisplayDevice.cb = sizeof(DISPLAY_DEVICE);
        fSuccess = EnumDisplayDevices(NULL, iEnum, &DisplayDevice, 0);
        if(fSuccess)
        {
            if(0 == lstrcmp(&DisplayDevice.DeviceName[0], pszDisplay))
            {
                ASSERT(lstrlen(DisplayDevice.DeviceKey) < cChars);
                fSuccess = (lstrlen(DisplayDevice.DeviceKey) < cChars);
                if(fSuccess)
                {
                    lstrcpy(pszDeviceKey, DisplayDevice.DeviceKey);
                    fFound = TRUE;
                }
            }
            ++iEnum;
        }
    }
    
    return fFound;
}


INT_PTR
CALLBACK
AskDynamicApply(HWND hDlg, UINT msg, WPARAM wp, LPARAM lp)
{
    int *pTemp;

    switch (msg)
    {
    case WM_INITDIALOG:
        if ((pTemp = (int *)lp) != NULL)
        {
            SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR)pTemp);
            CheckDlgButton(hDlg, (*pTemp & DCDSF_DYNA)?
                           IDC_YESDYNA : IDC_NODYNA, BST_CHECKED);
        }
        else
            EndDialog(hDlg, -1);
        break;

    case WM_COMMAND:
        switch (GET_WM_COMMAND_ID(wp, lp))
        {
        case IDOK:
            if ((pTemp = (int *)GetWindowLongPtr(hDlg, DWLP_USER)) != NULL)
            {
                *pTemp = IsDlgButtonChecked(hDlg, IDC_YESDYNA)? DCDSF_DYNA : 0;

                if (!IsDlgButtonChecked(hDlg, IDC_SHUTUP))
                    *pTemp |= DCDSF_ASK;

                SetDisplayCPLPreference(REGSTR_VAL_DYNASETTINGSCHANGE, *pTemp);
            }

            EndDialog(hDlg, TRUE);
            break;

        case IDCANCEL:

            EndDialog(hDlg, FALSE);
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
            BOOL bSuccess = FALSE;

            //
            // LATER: Check we are on Terminal Server client or not.
            //

            BOOL bLocalConsole = TRUE;

            if (bLocalConsole)
            {
                //
                // Get the display device name from IDataObject.
                //

                FORMATETC fmte = {(CLIPFORMAT)RegisterClipboardFormat(DESKCPLEXT_DISPLAY_DEVICE),
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

                    PWSTR pDisplayDevice = (PWSTR) GlobalLock(stgm.hGlobal);

                    if (pDisplayDevice)
                    {
                        //
                        // Copy the data to local buffer.
                        //

                    #ifdef UNICODE

                        lstrcpy(gszWinDisplayDevice,pDisplayDevice);
                        bSuccess = TRUE;

                    #else

                        bSuccess = (BOOL) WideCharToMultiByte(
                                            CP_ACP,0,
                                            pDisplayDevice,lstrlenW(pDisplayDevice)+1,
                                            gszWinDisplayDevice,MAX_PATH,
                                            NULL,NULL);

                    #endif

                        GlobalUnlock(stgm.hGlobal);
                    }  
                }

                //
                // let's build registry path for its hardware profie.
                //

                if (bSuccess)
                {
                    TCHAR szServicePath[MAX_PATH];

                    bSuccess = FALSE;

                    if(GetDeviceKey(gszWinDisplayDevice, szServicePath, sizeof(szServicePath) / sizeof(TCHAR)))
                    {
                        //
                        // Upcase all character.
                        //

                        TCHAR *psz = szServicePath;

                        while (*psz)
                        {
                            *psz = _totupper(*psz);
                            psz++;
                        }

                        //
                        // Find \SYSTEM from service path
                        //

                        psz = _tcsstr(szServicePath,TEXT("\\SYSTEM"));

                        //
                        // Skip '\'
                        //

                        psz++;

                        lstrcpy(gszRegistryPath,psz);

                        bSuccess = TRUE;
                    }
                }

                if (bSuccess)
                {
                    //
                    // Read currect acceleration level from registry.
                    //

                    HKEY hKeyAccelLevel = NULL;

                    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                                     gszRegistryPath,
                                     0,
                                     KEY_ALL_ACCESS,
                                     &hKeyAccelLevel) != ERROR_SUCCESS)
                    {
                        if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                                         gszRegistryPath,
                                         0,
                                         KEY_READ,
                                         &hKeyAccelLevel) != ERROR_SUCCESS)
                        {
                            hKeyAccelLevel = NULL;
                        }
                        else
                        {
                            gbReadOnly = TRUE;
                        }
                    }

                    if (hKeyAccelLevel)
                    {
                        DWORD cb = sizeof(AccelLevel);

                        if (RegQueryValueEx(hKeyAccelLevel,
                                            SZ_ACCELLEVEL,
                                            NULL,NULL,
                                            (LPBYTE) &AccelLevel,
                                            &cb) == ERROR_SUCCESS)
                        {
                            //
                            // Update last saved accel level.
                            //

                            AccelLevelInReg = AccelLevel;
                        }
                        else
                        {
                            //
                            // If there is no registry value, assume full acceleration.
                            //

                            AccelLevelInReg = AccelLevel = ACCELERATION_FULL;
                        }

                        RegCloseKey(hKeyAccelLevel);

                        bSuccess = TRUE;
                    }
                }
            }

            //
            // Setup slider.
            //

            HWND hSlider = GetDlgItem(hDlg, IDC_ACCELERATION_SLIDER);

            //
            // Slider range is between ACCEL_FULL and ACCEL_NONE.
            //

            SendMessage(hSlider, TBM_SETRANGE, (WPARAM)FALSE,
                        MAKELPARAM(ACCELERATION_FULL, ACCELERATION_NONE));

            //
            // Set currect slider position based on currect accel level.
            //
 
            SendMessage(hSlider, TBM_SETPOS, (WPARAM)TRUE,
                        (LPARAM) ACCEL_LEVEL_TO_SLIDER_POS(AccelLevel));

            //
            // Update message based on currect acceleration level.
            //

            UpdateGraphicsText(hDlg, AccelLevel);

            if (!bSuccess || gbReadOnly)
            {
                // 
                // Disable slider control
                //

                EnableWindow(hSlider, FALSE);
            }
        }

        break;

    case WM_HSCROLL:

        if (GetWindowLongPtr((HWND)lParam, GWLP_ID) == IDC_ACCELERATION_SLIDER)
        {
            //
            // Slider has been moved.
            //

            HWND hSlider = (HWND) lParam;

            //
            // Obtain currect slider position.
            //

            DWORD dwSliderPos = (DWORD) SendMessage(hSlider, TBM_GETPOS, 0, 0L);

            //
            // Convert slider position to accel level.
            //

            DWORD AccelNew = SLIDER_POS_TO_ACCEL_LEVEL(dwSliderPos); 

            //
            // If accleration level has been changed, update description, and
            // enable apply button.
            //

            if (AccelNew != AccelLevel)
            {
                AccelLevel = AccelNew;
                UpdateGraphicsText(hDlg, AccelNew);
                PropSheet_Changed(GetParent(hDlg), hDlg);
            }
        }

        break;

    case WM_NOTIFY:

        if (((NMHDR *)lParam)->code == PSN_APPLY)
        {
            if (AccelLevel != AccelLevelInReg)
            {
                //
                // Popup the dialog to ask user to apply it dynamically or not.
                //

                int val = GetDynaCDSPreference();
                BOOL bSuccess = TRUE;

                if (val & DCDSF_ASK)
                {
                    switch (DialogBoxParam(g_hInst, 
                                           MAKEINTRESOURCE(DLG_ASKDYNACDS),
                                           hDlg, 
                                           AskDynamicApply, 
                                           (LPARAM)&val))
                    {
                    case 0:         // user cancelled
                    case -1:        // dialog could not be displayed
                        bSuccess = FALSE;
                        break;
                    }
                }

                if (bSuccess)
                {
                    //
                    // AccelLevel has been changed. save it to registry.
                    //

                    HKEY hKeyAccelLevel;

                    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                                     gszRegistryPath,
                                     0,
                                     KEY_ALL_ACCESS,
                                     &hKeyAccelLevel) == ERROR_SUCCESS)
                    {
                        if (AccelLevel == ACCELERATION_FULL)
                        {
                            //
                            // If acceration is set to ACCELERATION_FULL (default)
                            // remove registry value.
                            //

                            if (RegDeleteValue(hKeyAccelLevel,
                                               SZ_ACCELLEVEL) == ERROR_SUCCESS)
                            {
                                bSuccess = TRUE;
                            }
                        }
                        else
                        {
                            //
                            // Otherwise, save it to registry.
                            //

                            if (RegSetValueEx(hKeyAccelLevel,
                                              SZ_ACCELLEVEL,
                                              NULL, REG_DWORD,
                                              (LPBYTE) &AccelLevel,
                                              sizeof(AccelLevel)) == ERROR_SUCCESS)
                            {
                                bSuccess = TRUE;
                            }
                        }

                        RegCloseKey(hKeyAccelLevel);
                    }

                    if (bSuccess)
                    {
                        //
                        // Update last saved data.
                        //

                        AccelLevelInReg = AccelLevel;

                        //
                        // Apply it dynamically or reboot.
                        //

                        if (((val & DCDSF_DYNA) == DCDSF_DYNA))
                        {
                        // Apply it dynamically.

                        // If EnumDisplaySettings was called with EDS_RAWMODE, we need CDS_RAWMODE below.
                        // Otherwise, it's harmless.
                        ChangeDisplaySettings(NULL, CDS_RAWMODE);
                        }
                        else
                        {
                        // User wants to reboot system.

                        PropSheet_RestartWindows(GetParent(hDlg));
                        }
                    }
                    else
                    {
                        //
                        // BUGBUG: Error PopUp ??
                        //
                    }
                }

            long lRet = (bSuccess ? PSNRET_NOERROR : PSNRET_INVALID_NOCHANGEPAGE);
            SetWindowLongPtr(hDlg, DWLP_MSGRESULT, lRet);
            }
        }

        break;

    case WM_HELP:

        WinHelp((HWND)((LPHELPINFO)lParam)->hItemHandle,
                TEXT("display.hlp"),
                HELP_WM_HELP,
                (DWORD_PTR)(LPTSTR)sc_PerformanceHelpIds);

        break;


    case WM_CONTEXTMENU:

        WinHelp((HWND)wParam,
                TEXT("display.hlp"),
                HELP_CONTEXTMENU,
                (DWORD_PTR)(LPTSTR)sc_PerformanceHelpIds);

        break;

    default:

        return FALSE;
    }

    return TRUE;
}
