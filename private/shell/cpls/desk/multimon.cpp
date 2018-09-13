/**************************************************************************\
 * Module Name: multimon.cpp
 *
 * Contains all the code to manage multiple devices
 *
 * Copyright (c) Microsoft Corp.  1995-1996 All Rights Reserved
 *
 * NOTES:
 *
 * History: Create by toddla -- multimon.c
 *          Changed to C++ by dli -- multimon.cpp on 7/10/1997
 *
 \**************************************************************************/


#include "precomp.h"
#include "shlobjp.h"
#include "shlwapi.h"

#define COMPILE_MULTIMON_STUBS
#include "multimon.h"
#include "device.hxx"
#include "settings.hxx"

extern "C" {
    LONG CALLBACK MonitorWindowProc(HWND hwnd, UINT msg,WPARAM wParam,LPARAM lParam);
    extern BOOL g_bGradient;  // from lookdlg.c
}

#define MONITORS_MAX    10

#define PREVIEWAREARATIO 2

#ifdef WINNT
// Global configuration variables 
ULONG gUnattenedConfigureAtLogon = 0;
ULONG gUnattenedInstall = 0;
ULONG gUnattenedBitsPerPel = 0;
ULONG gUnattenedXResolution = 0;
ULONG gUnattenedYResolution = 0;
ULONG gUnattenedVRefresh = 0;
ULONG gUnattenedFlags = 0;
ULONG gUnattenedAutoConfirm = 0;
#endif

static const TCHAR sc_szVideo[] = TEXT("Video");
static const TCHAR sc_szOptimize[] = TEXT("Optimize");
static const TCHAR sc_szQtwIni[] = TEXT("qtw.ini");
static const TCHAR sc_szVGA[] = TEXT("Standard VGA");
//
// display devices
//
typedef struct _multimon_device {
    ULONG          ComboBoxItem;
    BOOL           bRepositionable;
    DISPLAY_DEVICE DisplayDevice;
    DEVMODE        DisplaySetting;
    ULONG          DisplayIndex;
    TCHAR          DisplayRegKey[128];
    CDeviceSettings * pds;
    int            w,h;
    HIMAGELIST     himl;
    int            iImage;
    POINT          Snap;
    HDC            hdc;
} MULTIMON_DEVICE, *PMULTIMON_DEVICE;

extern HWND ghwndPropSheet;
extern int AskDynaCDS(HWND hDlg);
extern BOOL WarnUserAboutCompatibility(HWND hDlg);

//-----------------------------------------------------------------------------
static const DWORD sc_MultiMonitorHelpIds[] =
{
   IDC_DISPLAYDESK,   IDH_SETTINGS_DISPLAYDESK, 
   IDC_DISPLAYLIST,   IDH_SETTINGS_DISPLAYLIST, 
   IDC_COLORBOX,      IDH_SETTINGS_COLORBOX,
   IDC_SCREENSIZE,    IDH_SETTINGS_SCREENSIZE,
   IDC_DISPLAYUSEME,  IDH_SETTINGS_DISPLAYUSEME, 
   IDC_DISPLAYPROPERTIES, IDH_SETTINGS_DISPLAYPROPERTIES, 
   0, 0
};

class CMultiMon  : public IMultiMonConfig
{
    private:
        // Data Section
        MULTIMON_DEVICE _Devices[MONITORS_MAX];
        PMULTIMON_DEVICE _pCurDevice;
        PMULTIMON_DEVICE _pPrimaryDevice;

        // HWND for the main window
        HWND _hDlg;
        HWND _hwndDesk;
        HWND _hwndList;
        
        // union of all monitor RECTs
        RECT _rcDesk;

        // ref count 
        UINT _cRef;
        
        // how to translate to preview size
        int   _DeskScale;
        POINT _DeskOff;
        UINT  _InSetInfo;
        int   _NumDevices;
        HBITMAP _hbmScrSample;
        HBITMAP _hbmMonitor;
        HIMAGELIST _himl;
        
        BOOL _bBadDriver         : 1;
        BOOL _bNewDriver         : 1;
        BOOL _bNoAttach          : 1;
        BOOL _bDirty             : 1;
        
        // Private functions 
        void _DeskToPreview(LPRECT in, LPRECT out);
        void _OffsetPreviewToDesk(LPRECT in, LPRECT out);
        BOOL _AnyColorChange();
        BOOL _AnyResolutionChange();
        BOOL _QueryForceSmallFont();
        void _SetPreviewScreenSize(int HRes, int VRes, int iOrgXRes, int iOrgYRes);
        void _CleanupRects(HWND hwndP);
        BOOL _WriteMultiMonProfile(); 
        void _SetReposition(HWND hwnd, PMULTIMON_DEVICE pDevice);
        void _DoAdvancedSettingsSheet();
        void _VerifyPrimaryMode(BOOL fKeepMode);

        BOOL _HandleApply();
        BOOL _HandleHScroll(HWND hwndSB, int iCode, int iPos); 
        void _RedrawDeskPreviews();
        void _ForwardToChildren(UINT msg, WPARAM wParam, LPARAM lParam);

        BOOL _InitDisplaySettings(BOOL bExport);
        void _DestroyDisplaySettings();

        int  _GetMinColorBits();
        void _GetDisplayName(PMULTIMON_DEVICE pDevice, LPTSTR pszDisplay);
        int  _SaveDisplaySettings(DWORD dwSet);
        void _RestoreDisplaySettings();
        void _ConfirmDisplaySettingsChange();
        BOOL _TestNewSettingsChange();
        BOOL _RebuildDisplaySettings(BOOL bComplete = FALSE);
        // NT specific stuff
#ifdef WINNT
        BOOL _InitMessage();
        void _vPreExecMode();
        void _vPostExecMode();
#endif
    public:
        CMultiMon();

        static BOOL RegisterPreviewWindowClass(WNDPROC pfnWndProc);
        // *** IUnknown methods *** 
        STDMETHODIMP QueryInterface(REFIID riid, LPVOID * ppvObj);
        STDMETHODIMP_(ULONG) AddRef(void);
        STDMETHODIMP_(ULONG) Release(void);    

        // *** IMultiMonConfig methods ***
        STDMETHOD ( Initialize ) ( HWND hwndHost, WNDPROC pfnWndProc, DWORD dwReserved);
        STDMETHOD ( GetNumberOfMonitors ) (int * pCMon, DWORD dwReserved);
        STDMETHOD ( GetMonitorData) (int iMonitor, MonitorData * pmd, DWORD dwReserved);
        STDMETHOD ( Paint) (THIS_ int iMonitor, DWORD dwReserved);     

        void InitMultiMonitorDlg(HWND hDlg);
        PMULTIMON_DEVICE GetCurDevice(){return _pCurDevice;};
        
        int  GetNumberOfAttachedDisplays();
        BOOL ToggleAttached(PMULTIMON_DEVICE pDevice, BOOL bPrime);
        void UpdateActiveDisplay(PMULTIMON_DEVICE pDevice);
        BOOL HandleMonitorChange(HWND hwndP, BOOL bMainDlg);
        void SetDirty(BOOL bDirty=TRUE);

        HWND GetCurDeviceHwnd() { return GetDlgItem(_hwndDesk, (int) _pCurDevice);};
        int  GetNumDevices() { return _NumDevices;};
        BOOL QueryNoAttach() { return _bNoAttach;};
        BOOL IsDirty()       { return _bDirty;};

        LRESULT CALLBACK WndProc(UINT uMsg, WPARAM wParam, LPARAM lParam);
};


CMultiMon::CMultiMon() : _cRef(1) {
    ASSERT(_pCurDevice == NULL);
    ASSERT(_pPrimaryDevice == NULL);
    ASSERT(_DeskScale == 0);    
    ASSERT(_InSetInfo == 0);
    ASSERT(IsRectEmpty(&_rcDesk));
    ASSERT(_bBadDriver == FALSE);
    ASSERT(_bNewDriver == FALSE);
    ASSERT(_bNoAttach == FALSE);
    ASSERT(_bDirty == FALSE);
};

/*BUGBUG: (dli) per my talk with AndreVa, the unattended configuration stuff are NT only
       and I don't have to do it. I tried my best to preserve the code and make them work
       but they still may not work. 
       
*/

void CMultiMon::_DestroyDisplaySettings()
{
    int iDevice;
    ASSERT(_NumDevices);
    TraceMsg(TF_GENERAL, "DestroyDisplaySettings: %s devices", _NumDevices);
    for (iDevice = 0; _Devices[iDevice].DisplayDevice.cb != 0; iDevice++)
    {
        ASSERT(_Devices[iDevice].pds);
        delete _Devices[iDevice].pds;

        if (_Devices[iDevice].hdc)
        {
            DeleteDC(_Devices[iDevice].hdc);
            _Devices[iDevice].hdc = NULL;
        }
    }

    if (_himl)
    {
        ImageList_Destroy(_himl);
        _himl = NULL;
    }

    TraceMsg(TF_GENERAL, "DestroyDisplaySettings: Finished destroying all devices");
}

#ifdef WINNT  
//
// deterines if the applet is in detect mode.
//

//
// Called to put up initial messages that need to appear above the dialog
// box
//

BOOL CMultiMon::_InitMessage()
{
    //
    // If configure at logon is set, then we don't want to do anything
    //
    if (gUnattenedConfigureAtLogon)
    {
        PropSheet_PressButton(ghwndPropSheet, PSBTN_CANCEL);
    }
    else if (gUnattenedAutoConfirm)
    {
        PropSheet_PressButton(ghwndPropSheet, PSBTN_OK);
    }
    else
    {
        //
        // _bBadDriver will be set when we fail to build the list of modes,
        // or something else failed during initialization.
        //
        // In almost every case, we should already know about this situation
        // based on our boot code.
        // However, if this is a new situation, just report a "bad driver"
        //

        if (_bBadDriver)
        {
            ASSERT(gbExecMode == EXEC_INVALID_MODE);

            gbExecMode = EXEC_INVALID_MODE;
            gbInvalidMode = EXEC_INVALID_DISPLAY_DRIVER;
        }


        if (gbExecMode == EXEC_INVALID_MODE)
        {
            DWORD Mesg;

            switch(gbInvalidMode) {

            case EXEC_INVALID_NEW_DRIVER:
                Mesg = MSG_INVALID_NEW_DRIVER;
                break;
            case EXEC_INVALID_DEFAULT_DISPLAY_MODE:
                Mesg = MSG_INVALID_DEFAULT_DISPLAY_MODE;
                break;
            case EXEC_INVALID_DISPLAY_DRIVER:
                Mesg = MSG_INVALID_DISPLAY_DRIVER;
                break;
            case EXEC_INVALID_OLD_DISPLAY_DRIVER:
                Mesg = MSG_INVALID_OLD_DISPLAY_DRIVER;
                break;
            case EXEC_INVALID_16COLOR_DISPLAY_MODE:
                Mesg = MSG_INVALID_16COLOR_DISPLAY_MODE;
                break;
            case EXEC_INVALID_DISPLAY_MODE:
                Mesg = MSG_INVALID_DISPLAY_MODE;
                break;
            case EXEC_INVALID_CONFIGURATION:
            default:
                Mesg = MSG_INVALID_CONFIGURATION;
                break;
            }

            FmtMessageBox(_hDlg,
                          MB_ICONEXCLAMATION,
                          FALSE,
                          MSG_CONFIGURATION_PROBLEM,
                          Mesg);

            //
            // For a bad display driver or old display driver, let's send the
            // user straight to the installation dialog.
            //

            if ((gbInvalidMode == EXEC_INVALID_OLD_DISPLAY_DRIVER) ||
                (gbInvalidMode == EXEC_INVALID_DISPLAY_DRIVER))
            {
                BOOL unused;
                //BUGBUG: (dli) _pCurDevice is the best guess.
                if (InstallNewDriver(_hDlg,
                                     (LPCTSTR)_pCurDevice->DisplayDevice.DeviceString[0],
                                     &unused) == NO_ERROR)
                {
                    //
                    // Set this flag so that we don't get a message about
                    // having an untested mode.
                    //

                    _bNewDriver = TRUE;

                    //
                    // Let's leave the applet so the user sees the reboot
                    // popup.
                    //

                    PropSheet_PressButton(ghwndPropSheet, PSBTN_OK);
                }
            }
        }
    }

    return TRUE;
}

VOID CMultiMon::_vPreExecMode()
{

    HKEY hkey;
    DWORD cb;
//    DWORD data;

    //
    // This function sets up the execution mode of the applet.
    // There are four vlid modes.
    //
    // EXEC_NORMAL - When the apple is launched from the control panel
    //
    // EXEC_INVALID_MODE is exactly the same as for NORMAL except we will
    //                   not mark the current mode as tested so the user has
    //                   to at least test a mode
    //
    // EXEC_DETECT - When the applet is launched normally, but a detect was
    //               done on the previous boot (the key in the registry is
    //               set)
    //
    // EXEC_SETUP  - When we launch the applet in setup mode from setup (Both
    //               the registry key is set and the setup flag is passed in).
    //

    //
    // These two keys should only be checked \ deleted if the machine has been
    // rebooted and the detect \ new display has actually happened.
    // So we will look for the RebootNecessary key (a volatile key) and if
    // it is not present, then we can delete the key.  Otherwise, the reboot
    // has not happened, and we keep the key
    //

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                     SZ_REBOOT_NECESSARY,
                     0,
                     KEY_READ | KEY_WRITE,
                     &hkey) != ERROR_SUCCESS) {

        if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                         SZ_DETECT_DISPLAY,
                         0,
                         KEY_READ | KEY_WRITE,
                         &hkey) == ERROR_SUCCESS) {

            //
            // NOTE: This key is also set when EXEC_SETUP is being run.
            //

            if (gbExecMode == EXEC_NORMAL) {

                gbExecMode = EXEC_DETECT;

            } else {

                //
                // If we are in setup mode, we also check the extra values
                // under DetectDisplay that control the unattended installation.
                //

                ASSERT(gbExecMode == EXEC_SETUP);

                cb = 4;
                if (RegQueryValueEx(hkey,
                                    SZ_CONFIGURE_AT_LOGON,
                                    NULL,
                                    NULL,
                                    (LPBYTE) &gUnattenedConfigureAtLogon,
                                    &cb) == ERROR_SUCCESS) {

                    //
                    // We delete only this value since the other values must remain
                    // until the next boot
                    //

                    RegDeleteValue(hkey,
                                   SZ_CONFIGURE_AT_LOGON);
                }

                if (gUnattenedConfigureAtLogon == 0)
                {

                    cb = 4;
                    RegQueryValueEx(hkey,
                                    SZ_UNATTEND_INSTALL,
                                    NULL,
                                    NULL,
                                    (LPBYTE) &gUnattenedInstall,
                                    &cb);

                    cb = 4;
                    RegQueryValueEx(hkey,
                                    SZ_UNATTEND_BPP,
                                    NULL,
                                    NULL,
                                    (LPBYTE) &gUnattenedBitsPerPel,
                                    &cb);

                    cb = 4;
                    RegQueryValueEx(hkey,
                                    SZ_UNATTEND_X,
                                    NULL,
                                    NULL,
                                    (LPBYTE) &gUnattenedXResolution,
                                    &cb);

                    cb = 4;
                    RegQueryValueEx(hkey,
                                    SZ_UNATTEND_Y,
                                    NULL,
                                    NULL,
                                    (LPBYTE) &gUnattenedYResolution,
                                    &cb);

                    cb = 4;
                    RegQueryValueEx(hkey,
                                    SZ_UNATTEND_REF,
                                    NULL,
                                    NULL,
                                    (LPBYTE) &gUnattenedVRefresh,
                                    &cb);

                    cb = 4;
                    RegQueryValueEx(hkey,
                                    SZ_UNATTEND_FLAGS,
                                    NULL,
                                    NULL,
                                    (LPBYTE) &gUnattenedFlags,
                                    &cb);

                    cb = 4;
                    RegQueryValueEx(hkey,
                                    SZ_UNATTEND_CONFIRM,
                                    NULL,
                                    NULL,
                                    (LPBYTE) &gUnattenedAutoConfirm,
                                    &cb);
                }
            }

            RegCloseKey(hkey);
        }

        //
        // Check for a new driver being installed
        //

        if ( (gbExecMode == EXEC_NORMAL) &&
             (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                           SZ_NEW_DISPLAY,
                           0,
                           KEY_READ | KEY_WRITE,
                           &hkey) == ERROR_SUCCESS) ) {

            gbExecMode = EXEC_INVALID_MODE;
            gbInvalidMode = EXEC_INVALID_NEW_DRIVER;

            RegCloseKey(hkey);
        }

        RegDeleteKey(HKEY_LOCAL_MACHINE,
                     SZ_DETECT_DISPLAY);

        RegDeleteKey(HKEY_LOCAL_MACHINE,
                     SZ_NEW_DISPLAY);
    }
    {
        LPTSTR psz;
        LPTSTR pszInv;

        switch(gbExecMode) {

            case EXEC_NORMAL:
                psz = TEXT("Normal Execution mode");
                break;
            case EXEC_DETECT:
                psz = TEXT("Detection Execution mode");
                break;
            case EXEC_SETUP:
                psz = TEXT("Setup Execution mode");
                break;
            case EXEC_INVALID_MODE:
                psz = TEXT("Invalid Mode Execution mode");

                switch(gbInvalidMode) {

                    case EXEC_INVALID_NEW_DRIVER:
                        pszInv = TEXT("Invalid new driver");
                        break;
                    default:
                        pszInv = TEXT("*** Invalid *** Invalid mode");
                        break;
                }
                break;
            default:
                psz = TEXT("*** Invalid *** Execution mode");
                break;
        }

        KdPrint(("\n \nDisplay.cpl: The display applet is in : %ws\n", psz));

        if (gbExecMode == EXEC_INVALID_MODE)
        {
            KdPrint(("\t\t sub invalid mode : %ws", pszInv));
        }
        KdPrint(("\n\n", psz));
    }
}


VOID CMultiMon::_vPostExecMode() {

    HKEY hkey;
    DWORD cb;
    DWORD data;

    //
    // Check for various invalid configurations
    //

    if ( (gbExecMode == EXEC_NORMAL) &&
         (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                       SZ_INVALID_DISPLAY,
                       0,
                       KEY_READ | KEY_WRITE,
                       &hkey) == ERROR_SUCCESS) ) {

        gbExecMode = EXEC_INVALID_MODE;

        //
        // Check for these fields in increasing order of "badness" or
        // "detail" so that the *worst* error is the one remaining in the
        // gbInvalidMode  variable once all the checks are done.
        //

        cb = 4;
        if (RegQueryValueEx(hkey,
                            TEXT("DefaultMode"),
                            NULL,
                            NULL,
                            (LPBYTE)(&data),
                            &cb) == ERROR_SUCCESS)
        {
            gbInvalidMode = EXEC_INVALID_DEFAULT_DISPLAY_MODE;
        }

        cb = 4;
        if (RegQueryValueEx(hkey,
                            TEXT("BadMode"),
                            NULL,
                            NULL,
                            (LPBYTE)(&data),
                            &cb) == ERROR_SUCCESS)
        {
            gbInvalidMode = EXEC_INVALID_DISPLAY_MODE;
        }

        cb = 4;
        if (RegQueryValueEx(hkey,
                            TEXT("16ColorMode"),
                            NULL,
                            NULL,
                            (LPBYTE)(&data),
                            &cb) == ERROR_SUCCESS)
        {
            gbInvalidMode = EXEC_INVALID_16COLOR_DISPLAY_MODE;
        }


        cb = 4;
        if (RegQueryValueEx(hkey,
                            TEXT("InvalidConfiguration"),
                            NULL,
                            NULL,
                            (LPBYTE)(&data),
                            &cb) == ERROR_SUCCESS)
        {
            gbInvalidMode = EXEC_INVALID_CONFIGURATION;
        }

        cb = 4;
        if (RegQueryValueEx(hkey,
                            TEXT("MissingDisplayDriver"),
                            NULL,
                            NULL,
                            (LPBYTE)(&data),
                            &cb) == ERROR_SUCCESS)
        {
            gbInvalidMode = EXEC_INVALID_DISPLAY_DRIVER;
        }

        //
        // This last case will be set in addition to the previous one in the
        // case where the driver was an old driver linking to winsvr.dll
        // and we can not load it.
        //

        cb = 4;
        if (RegQueryValueEx(hkey,
                            TEXT("OldDisplayDriver"),
                            NULL,
                            NULL,
                            (LPBYTE)(&data),
                            &cb) == ERROR_SUCCESS)
        {
            gbInvalidMode = EXEC_INVALID_OLD_DISPLAY_DRIVER;
        }

        RegCloseKey(hkey);

    }

    //
    // Delete all of these bad configuration keys since we only want the
    // user to see the message once.
    //

    RegDeleteKey(HKEY_LOCAL_MACHINE,
                 SZ_INVALID_DISPLAY);

{
    LPTSTR psz;
    LPTSTR pszInv;

    if (gbExecMode == EXEC_INVALID_MODE)
    {
        switch (gbInvalidMode)
        {
        case EXEC_INVALID_DEFAULT_DISPLAY_MODE:
            pszInv = TEXT("Default mode being used");
            break;
        case EXEC_INVALID_DISPLAY_DRIVER:
            pszInv = TEXT("Invalid Display Driver");
            break;
        case EXEC_INVALID_OLD_DISPLAY_DRIVER:
            pszInv = TEXT("Old Display Driver");
            break;
        case EXEC_INVALID_16COLOR_DISPLAY_MODE:
            pszInv = TEXT("16 color mode not supported");
            break;
        case EXEC_INVALID_DISPLAY_MODE:
            pszInv = TEXT("Invalid display mode");
            break;
        case EXEC_INVALID_CONFIGURATION:
            pszInv = TEXT("Invalid configuration");
            break;
        default:
            psz = TEXT("*** Invalid *** Invalid mode");
            break;
        }

        KdPrint(("\t\t sub invlid mode : %ws", pszInv));
        KdPrint(("\n\n", psz));
    }
}
}

#endif

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CMultiMon::_DeskToPreview(LPRECT in, LPRECT out)
{
    out->left   = _DeskOff.x + MulDiv(in->left   - _rcDesk.left,_DeskScale,1000);
    out->top    = _DeskOff.y + MulDiv(in->top    - _rcDesk.top, _DeskScale,1000);
    out->right  = _DeskOff.x + MulDiv(in->right  - _rcDesk.left,_DeskScale,1000);
    out->bottom = _DeskOff.y + MulDiv(in->bottom - _rcDesk.top, _DeskScale,1000);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CMultiMon::_OffsetPreviewToDesk(LPRECT in, LPRECT out)
{
    int x, y;

    // Scale preview rects back to desk size
    x = _rcDesk.left + MulDiv(in->left - _DeskOff.x,1000,_DeskScale);
    y = _rcDesk.top  + MulDiv(in->top  - _DeskOff.y,1000,_DeskScale);

    // Figure out how much to offset
    x = x - out->left;
    y = y - out->top;

    OffsetRect(out, x, y);
}

void CMultiMon::_SetReposition(HWND hwnd, PMULTIMON_DEVICE pDevice)
{
    //
    // Try to open the device with the new settings.
    // If it works, we get the reposition flag, otherwise we assume
    // the paramters are wrong and we fail (since the rectangle will
    // not be updated).
    //

#ifdef WINNT
    HDC hdc;

    if (pDevice->bRepositionable == FALSE)
    {
        hdc = CreateDC(NULL,
                       pDevice->DisplayDevice.DeviceName,
                       NULL,
                       &pDevice->DisplaySetting);

        if (hdc == NULL)
        {
            //
            // Device may be attached - try with a NULL DEVMODE.
            //

            hdc = CreateDC(NULL,
                           pDevice->DisplayDevice.DeviceName,
                           NULL,
                           NULL);
        }

        if (hdc)
        {
            pDevice->bRepositionable = TRUE;
            // DLI: make it Compile GetDeviceCaps(hdc, RASTERCAPS) & RC_REPOSITIONABLE;

            DeleteDC(hdc);
        }
    }

#else
    //
    // For WIN95, any device can be relocated.
    //
    pDevice->bRepositionable = TRUE;
#endif

}


int CMultiMon::_GetMinColorBits()
{
    
    int     iMinColorBits = 64; // This is the max possible
    int     iDevice;

    for (iDevice = 0; _Devices[iDevice].DisplayDevice.cb != 0; iDevice++)
    {
        int iColorBits = _Devices[iDevice].pds->GetColorBits();
        if (iColorBits < iMinColorBits)
            iMinColorBits = iColorBits;
    }

    return iMinColorBits;
}


//-----------------------------------------------------------------------------
int CMultiMon::_SaveDisplaySettings(DWORD dwSet)
{
    int     iRet = 0;
    int     iDevice;
    HWND    hwndC;

    _VerifyPrimaryMode(FALSE);

    for (iDevice = 0; _Devices[iDevice].DisplayDevice.cb != 0; iDevice++)
    {
        int iResult = _Devices[iDevice].pds->SaveSettings(dwSet);
        if (iResult != DISP_CHANGE_SUCCESSFUL)
        {
            if (iResult == DISP_CHANGE_RESTART)
            {
                iRet = iResult;
                continue;
            }
            else
            {
                ASSERT(iResult < 0);
                return iResult;
            }
        }

    }

    // If we get here, the above functions are successful or requires restart
    // This ChangeDisplaySettings call will actually go refresh the whole desktop
    // Of course we don't want to do it at a test. 
    if (!(dwSet & CDS_TEST))
    {
        if (_GetMinColorBits() > 8)
            g_bGradient = TRUE;
        else
            g_bGradient = FALSE;

        SystemParametersInfo(SPI_SETGRADIENTCAPTIONS, 0, (LPVOID)g_bGradient, SPIF_UPDATEINIFILE);
        
        hwndC = CreateCoverWindow(COVER_NOPAINT);
        ChangeDisplaySettings(NULL, NULL);
        DestroyCoverWindow(hwndC);
    }
    return iRet;
}

void CMultiMon::_RestoreDisplaySettings()
{
    int     iDevice;
    HWND    hwndC;

    for (iDevice = 0; _Devices[iDevice].DisplayDevice.cb != 0; iDevice++)
        _Devices[iDevice].pds->RestoreSettings();
    
    //DLI: This last function call will actually go refresh the whole desktop
    hwndC = CreateCoverWindow(COVER_NOPAINT);
    ChangeDisplaySettings(NULL, NULL);
    DestroyCoverWindow(hwndC);
}

void CMultiMon::_ConfirmDisplaySettingsChange()
{
    int iDevice;
    for (iDevice = 0; _Devices[iDevice].DisplayDevice.cb != 0; iDevice++)
        _Devices[iDevice].pds->ConfirmChangeSettings();

    if (GetNumberOfAttachedDisplays() > 1)
        _WriteMultiMonProfile();
}

BOOL CALLBACK KeepNewDlgProc(HWND hDlg, UINT message , WPARAM wParam, LPARAM lParam)
{
    UINT idTimer = 0;
    HICON hicon;

    switch(message)
    {
        case WM_INITDIALOG:
            hicon = LoadIcon(NULL, IDI_QUESTION);
            if (hicon)
                SendDlgItemMessage(hDlg, IDC_BIGICON, STM_SETIMAGE, IMAGE_ICON, (DWORD)hicon);
            idTimer = SetTimer(hDlg, 1, 20000, NULL);
            break;

        case WM_DESTROY:
            if (idTimer)
                KillTimer(hDlg, idTimer);
            hicon = (HICON)SendDlgItemMessage(hDlg, IDC_BIGICON, STM_GETIMAGE, IMAGE_ICON, 0);
            if (hicon)
                DestroyIcon(hicon);
            break;

        case WM_TIMER:
            EndDialog(hDlg, IDNO);
            break;

        case WM_COMMAND:
            EndDialog(hDlg, wParam);
            break;

        default:
            return FALSE;
    }
    return TRUE;
}

BOOL CMultiMon::_TestNewSettingsChange()
{
    int iAnswer = DialogBox(hInstance, MAKEINTRESOURCE(DLG_KEEPNEW), GetParent(_hDlg),
                            KeepNewDlgProc);
    return (iAnswer == IDYES);
}

BOOL CMultiMon::_RebuildDisplaySettings(BOOL bComplete)
{
    BOOL result = TRUE;
    HCURSOR hcur = SetCursor(LoadCursor(NULL, IDC_WAIT));

    for (int iDevice = 0; _Devices[iDevice].DisplayDevice.cb != 0; iDevice++)
        if (!_Devices[iDevice].pds->RefreshSettings(bComplete))
            result = FALSE;

    RedrawWindow(_hDlg, NULL, NULL, RDW_ERASE | RDW_INVALIDATE | RDW_ALLCHILDREN);

    SetCursor(hcur);
    return result;
}

int CMultiMon::GetNumberOfAttachedDisplays()
{
    int nDisplays = 0;
    int iDevice;
    for (iDevice = 0; _Devices[iDevice].DisplayDevice.cb != 0; iDevice++)
    {
        if (_Devices[iDevice].pds->IsAttached())
            nDisplays++;
    }
    return nDisplays;
}

BOOL CMultiMon::_AnyColorChange()
{
    int iDevice;
    for (iDevice = 0; _Devices[iDevice].DisplayDevice.cb != 0; iDevice++)
    {
        if (_Devices[iDevice].pds->IsAttached() && _Devices[iDevice].pds->IsColorChanged())
            return TRUE;
    }
    return FALSE;
}

BOOL CMultiMon::_AnyResolutionChange()
{
   int iDevice;
   for (iDevice = 0; _Devices[iDevice].DisplayDevice.cb != 0; iDevice++)
   {
       if (_Devices[iDevice].pds->IsAttached() && _Devices[iDevice].pds->IsResolutionChanged())
           return TRUE;
   }
   return FALSE;
}

BOOL CMultiMon::_QueryForceSmallFont()
{
    int iDevice;
    for (iDevice = 0; _Devices[iDevice].DisplayDevice.cb != 0; iDevice++)
    {
        if (_Devices[iDevice].pds->IsSmallFontNecessary())
            return TRUE;
    }
    return FALSE;
}

void CMultiMon::_GetDisplayName(PMULTIMON_DEVICE pDevice, LPTSTR pszDisplay)
{
    TCHAR szMonitor[140];
    TCHAR szDisplayFormat[20];
    LoadString(hInstance, IDS_DISPLAYFORMAT, szDisplayFormat, SIZEOF(szDisplayFormat));
    if (!pDevice->pds->GetMonitorName(szMonitor, NULL) || szMonitor[0] == 0)
        LoadString(hInstance, IDS_UNKNOWNMONITOR, szMonitor, SIZEOF(szMonitor));
        
    wsprintf(pszDisplay, szDisplayFormat, pDevice->DisplayIndex, szMonitor, pDevice->DisplayDevice.DeviceString);
}

//-----------------------------------------------------------------------------
void CMultiMon::_DoAdvancedSettingsSheet()
{
#ifndef WINNT
    HINSTANCE hDesk16 = LoadLibrary16( "DeskCp16.Dll" );
    FARPROC16 pDesk16 = (FARPROC16)( hDesk16?
                        GetProcAddress16( hDesk16, "CplApplet" ) : NULL );
#endif
    PROPSHEETHEADER psh;
    HPROPSHEETPAGE rPages[MAX_PAGES];
    PROPSHEETPAGE psp;
    HPSXA hpsxa = NULL;
    HPSXA hpsxaOEM = NULL;
    int iResult = 0;
    TCHAR szDisplay[256];
    PMONPARAM pmMon = NULL;

    psh.dwSize = sizeof(psh);
    psh.dwFlags = PSH_PROPTITLE;
    psh.hwndParent = GetParent(_hDlg);
    psh.hInstance = hInstance;
    psh.pszCaption = (LPTSTR)_pCurDevice->DisplayDevice.DeviceString;
    psh.nPages = 0;
    psh.nStartPage = 0;
    psh.phpage = rPages;

    psp.dwSize = sizeof(psp);
    psp.dwFlags = PSP_DEFAULT;
    psp.hInstance = hInstance;

    psp.pfnDlgProc = GeneralPageProc;
    psp.pszTemplate = MAKEINTRESOURCE(DLG_GENERAL);
    psp.lParam = (LPARAM)_QueryForceSmallFont();

    if (rPages[psh.nPages] = CreatePropertySheetPage(&psp))
        psh.nPages++;

#ifdef WINNT
    //BUGBUG: (dli) Should use the one from pds
    GetAdvAdapterPropPageParam( NULL, _AddDisplayPropSheetPage, (LPARAM)&psh, (LPARAM)&_pCurDevice->DisplaySetting);
    pmMon = _pCurDevice->pds->GetMonitorParams();
    if (pmMon)
        GetAdvMonitorPropPageParam( NULL, _AddDisplayPropSheetPage, (LPARAM)&psh, (LPARAM)pmMon );
#else
    ATOM AtomDevice = GlobalAddAtom((char *)&_pCurDevice->DisplayDevice.DeviceName);
    if( pDesk16 && CallCPLEntry16( hDesk16, pDesk16, NULL, CPL_INIT, (LPARAM)AtomDevice, 0 ) )
    {
        // or just add the default page
        SHAddPages16( NULL, "DESKCP16.DLL,GetAdapterPage",
                      _AddDisplayPropSheetPage, (LPARAM)&psh );

        //
        // only add the monitor tab iff a monitor exists
        //
        if (_pCurDevice->DisplayDevice.DeviceName[0])
        {
            TCHAR szMonitor[140];
            szMonitor[0] = 0;
            _pCurDevice->pds->GetMonitorName(szMonitor, NULL);

            if (szMonitor[0])
            {
                SHAddPages16( NULL, "DESKCP16.DLL,GetMonitorPage",
                          _AddDisplayPropSheetPage, (LPARAM)&psh );
            }
        }

        SHAddPages16( NULL, "DESKCP16.DLL,GetPerformancePage",
                      _AddDisplayPropSheetPage, (LPARAM)&psh );
    }
#endif

    InitClipboardFormats();
    IDataObject * pdo = NULL;
    _pCurDevice->pds->QueryInterface(IID_IDataObject, (LPVOID *) &pdo);

    //
    // load any extensions from the registry
    //
    if (gbExecMode == EXEC_NORMAL)
    {
        //
        // load the generic (non hardware specific) extensions
        //
        if( ( hpsxa = SHCreatePropSheetExtArrayEx( HKEY_LOCAL_MACHINE, REGSTR_PATH_CONTROLSFOLDER TEXT("\\Device"), 8, pdo) ) != NULL )
        {
            SHAddFromPropSheetExtArray( hpsxa, _AddDisplayPropSheetPage, (LPARAM)&psh );
        }

        //
        // load the hardware-specific extensions
        //
        // NOTE it is very important to load the OEM extensions *after* the
        // generic extensions some HW extensions expect to be the last tabs
        // in the propsheet (right before the settings tab)
        //
        if( ( hpsxaOEM = SHCreatePropSheetExtArrayEx( HKEY_LOCAL_MACHINE, _pCurDevice->DisplayRegKey, 8, pdo) ) != NULL )
        {
            SHAddFromPropSheetExtArray( hpsxaOEM, _AddDisplayPropSheetPage, (LPARAM)&psh );
        }

        //
        // add a fake settings page to fool OEM extensions (must be last)
        //
        if (hpsxa || hpsxaOEM)
        {
            AddFakeSettingsPage(&psh);
        }
    }

    if (psh.nPages)
    {
        iResult = PropertySheet(&psh);
    }

    _GetDisplayName(_pCurDevice, szDisplay);

    if (_NumDevices == 1)
    {
        //Set the name of the primary in the static text
        //strip the first token off (this is the number we dont want it)
        TCHAR *pch;
        for (pch=szDisplay; *pch && *pch != TEXT(' '); pch++);
        for (;*pch && *pch == TEXT(' '); pch++);
        SetDlgItemText(_hDlg, IDC_DISPLAYTEXT, pch);
    }
    else
    {
        ComboBox_DeleteString(_hwndList, _pCurDevice->ComboBoxItem);
        ComboBox_InsertString(_hwndList, _pCurDevice->ComboBoxItem, szDisplay);
        ComboBox_SetItemData(_hwndList, _pCurDevice->ComboBoxItem, (DWORD)_pCurDevice);
        ComboBox_SetCurSel(_hwndList, _pCurDevice->ComboBoxItem);
    }
    
#ifdef WINNT
    _pCurDevice->pds->UpdateRefreshRate(pmMon);
#endif

    if( hpsxa )
        SHDestroyPropSheetExtArray( hpsxa );
    if( hpsxaOEM )
        SHDestroyPropSheetExtArray( hpsxaOEM );
    if (pdo)
        pdo->Release();
    
#ifndef WINNT
    if (pDesk16)
        CallCPLEntry16( hDesk16, pDesk16, NULL, CPL_EXIT, 0, 0 );
    if (AtomDevice)
        GlobalDeleteAtom(AtomDevice);
    if( hDesk16 )
        FreeLibrary16( hDesk16 );
#endif
    
    if ((iResult == ID_PSRESTARTWINDOWS) || (iResult == ID_PSREBOOTSYSTEM))
    {
        PropSheet_CancelToClose(GetParent(_hDlg));

        if (iResult == ID_PSREBOOTSYSTEM)
            PropSheet_RebootSystem(ghwndPropSheet);
        else
            PropSheet_RestartWindows(ghwndPropSheet);
    }
}

//-----------------------------------------------------------------------------
void CMultiMon::UpdateActiveDisplay(PMULTIMON_DEVICE pDevice)
{
    HWND hwndC;

    _InSetInfo++;

    if (pDevice == NULL)
        pDevice = (PMULTIMON_DEVICE)ComboBox_GetItemData(_hwndList, ComboBox_GetCurSel(_hwndList));
    else
        ComboBox_SetCurSel(_hwndList, pDevice->ComboBoxItem);

    if (pDevice && ((ULONG)pDevice) != 0xFFFFFFFF)
    {
        hwndC = GetDlgItem(_hwndDesk, (int) _pCurDevice);

        _pCurDevice->pds->SetActive(FALSE);
        _pCurDevice = pDevice;

        if (hwndC)
            RedrawWindow(hwndC, NULL, NULL, RDW_ERASE | RDW_INVALIDATE);

        hwndC = GetDlgItem(_hwndDesk, (int) _pCurDevice);
        if (hwndC)
            RedrawWindow(hwndC, NULL, NULL, RDW_ERASE | RDW_INVALIDATE);

        //
        // Update the two check box windows
        //
#ifdef WINNT 
        CheckDlgButton(_hDlg, IDC_DISPLAYPRIME, _pCurDevice->pds->IsPrimary());
        EnableWindow(GetDlgItem(_hDlg, IDC_DISPLAYPRIME), 1);  // fUseDevice);
#endif
        CheckDlgButton(_hDlg, IDC_DISPLAYUSEME, _pCurDevice->pds->IsAttached());
        EnableWindow(GetDlgItem(_hDlg, IDC_DISPLAYUSEME), !_bNoAttach && !_pCurDevice->pds->IsPrimary());

        // Update the color and screen size combo box
        _pCurDevice->pds->SetActive(TRUE);
    }
    else
    {
        //
        // No display device !
        //
        WarnMsg(TEXT("UpdateActiveDisplay: No display device!!!!"), TEXT(""));
        ASSERT(FALSE);
    }

    _InSetInfo--;
}

//----------------------------------------------------------------------------
//
//  VerifyPrimaryMode()
//
//----------------------------------------------------------------------------

void CMultiMon::_VerifyPrimaryMode(BOOL fKeepMode)
{
#ifndef WINNT
    //
    // on Win9x make sure the primary is in a mode >= 256 color
    //
    if (GetNumberOfAttachedDisplays() > 1 && _pPrimaryDevice &&
        _pPrimaryDevice->pds->GetColorBits() < 8)
    {
        if (fKeepMode)
        {
            for (int i = 0; _Devices[i].DisplayDevice.cb != 0; i++)
            {
                if (_pPrimaryDevice != &_Devices[i])
                    _Devices[i].pds->SetAttached(FALSE);
            }

            RedrawWindow(GetDlgItem(_hDlg, IDC_DISPLAYDESK), NULL, NULL,
                RDW_ALLCHILDREN | RDW_ERASE | RDW_INVALIDATE);
        }
        else
        {
            _pPrimaryDevice->pds->SetMode(640,480,8);
        }

        SetDirty();
    }
#endif
}

//----------------------------------------------------------------------------
//
//  SetDirty
//
//----------------------------------------------------------------------------
void CMultiMon::SetDirty(BOOL bDirty)
{
    _bDirty = bDirty;

    if (_bDirty)
    {
        PostMessage(GetParent(_hDlg), PSM_CHANGED, (WPARAM)_hDlg, 0L);
    }
}

//----------------------------------------------------------------------------
//
//  ToggleAttached()
//
//  Toggles the attached state of the device, unless the bPrime is set which
//  forces an attach also.
//
//----------------------------------------------------------------------------
BOOL CMultiMon::ToggleAttached(PMULTIMON_DEVICE pDevice, BOOL bPrime)
{
    DWORD iDevice;
    RECT rcPos;
    if (bPrime || (!pDevice->pds->IsAttached()))
    {
        //
        // Make sure this device actually has a rectangle.
        // If it does not (not configured in the registry, then we need
        // to put up a popup and ask the user to configure the device.
        //
        pDevice->pds->GetCurPosition(&rcPos);
        if (IsRectEmpty(&rcPos))
            return FALSE;

#ifdef WINNT
        //
        // NT requires the same number of colors.
        // Don't let the driver get attached if it is has a different color
        // depth than the primary (unless we are settings it to the primary.
        //

        if ((bPrime == FALSE) &&
            (pDevice->DisplaySetting.dmBitsPerPel !=
             _pPrimaryDevice->DisplaySetting.dmBitsPerPel))
        {
            FmtMessageBox(_hDlg,
                          MB_ICONSTOP | MB_OK,
                          FALSE,
                          ID_DSP_TXT_SETTINGS,
                          MSG_MULTI_BAD_COLORS);
            return FALSE;
        }
#endif

        //
        // If the device is not the primary, let's see if it is repositionable.
        // If not, we will have to ask the user if it should be the primary also.
        //

        if ((bPrime == FALSE) && (!pDevice->bRepositionable))
        {
            if (FmtMessageBox(_hDlg,
                              MB_ICONQUESTION | MB_YESNO | MB_DEFBUTTON2,
                              FALSE,
                              ID_DSP_TXT_SETTINGS,
                              MSG_MULTI_NO_REPOSITION) == IDYES)
            {
                bPrime = TRUE;
            }
            else
            {
                return FALSE;
            }
        }

        //
        // The DM_POSITION determines whether the device should be
        // part of the desktop or not.
        // Make sure this device actually has a rectangle now.
        //
        pDevice->pds->SetAttached(TRUE);

        //
        // make sure the primary is in a valid mode
        //
        _VerifyPrimaryMode(FALSE);
    }
    else
        //
        // Toggling the device out of the desktop, unless it's the primary
        //
        pDevice->pds->SetAttached(FALSE);

    if (bPrime)
    {
        //
        // Unmark the primary device.
        //
        // Remove all the devices which are non-repositionable or of a
        // different color depths.
        // Also, if this device is not repositionable, then remove it from the
        // desktop.
        //

        for (iDevice = 0; _Devices[iDevice].DisplayDevice.cb != 0; iDevice++)
        {
            _Devices[iDevice].pds->SetPrimary(FALSE);

#ifdef WINNT
            if (pDevice->DisplaySetting.dmBitsPerPel != _Devices[iDevice].DisplaySetting.dmBitsPerPel)
                _Devices[iDevice].pds->SetAttached(FALSE);
#endif
            if (!_Devices[iDevice].bRepositionable)
                _Devices[iDevice].pds->SetAttached(FALSE);
        }
        _pPrimaryDevice->pds->SetPrimary(FALSE);
        //
        // Mark the new device as the primary and attached since we
        // unattached it in the above loop.
        //

        _pPrimaryDevice = pDevice;
        _pPrimaryDevice->pds->SetPrimary(TRUE);
        _pPrimaryDevice->pds->SetAttached(TRUE);
    }

    return TRUE;
}


//-----------------------------------------------------------------------------

void CMultiMon::_CleanupRects(HWND hwndP)
{
    int   n;
    ULONG iDevice;
    HWND  hwndC;
    RECT  arc[20];
    DWORD arcDev[20];
    DWORD iArcPrimary = 0;

    RECT rc;
    RECT rcU;
    int   i;
    RECT rcPrev;
    int sx,sy;
    int x,y;

    //
    // get the positions of all the windows
    //

    n = 0;


    for (iDevice = 0; _Devices[iDevice].DisplayDevice.cb != 0; iDevice++)
    {
        PMULTIMON_DEVICE pDevice = &_Devices[iDevice];

        hwndC = GetDlgItem(hwndP, (int) pDevice);

        if (hwndC != NULL)
        {
            RECT rcPos;
#ifdef WINNT
            ASSERT(pDevice->DisplaySetting.dmBitsPerPel ==
                   _pPrimaryDevice->DisplaySetting.dmBitsPerPel);
#endif
            TraceMsg(TF_GENERAL, "_CleanupRects start Device %08lx, Dev = %d, hwnd = %08lx\n",
                     pDevice, iDevice, hwndC);

            ShowWindow(hwndC, SW_SHOW);

            GetWindowRect(hwndC, &arc[n]);
            MapWindowPoints(NULL, hwndP, (POINT FAR*)&arc[n], 2);

            pDevice->pds->GetCurPosition(&rcPos);
            _OffsetPreviewToDesk(&arc[n], &rcPos);
            arc[n] = rcPos;
            arcDev[n] = iDevice;

            if (pDevice->pds->IsPrimary())
            {
                TraceMsg(TF_GENERAL, "_CleanupRects primary Device %08lx\n", pDevice);

                iArcPrimary = n;
            }
            else
            {
                ASSERT(pDevice->bRepositionable);
            }

            n++;
        }
    }

    //
    // cleanup the rects
    //
    AlignRects(arc, n, iArcPrimary, CUDR_NORMAL);

    //
    // Get the union.
    //

    SetRectEmpty(&rcU);
    for (i=0; i<n; i++)
        UnionRect(&rcU, &rcU, &arc[i]);
    GetClientRect(hwndP, &rcPrev);

    //
    // only rescale if the new desk hangs outside the preview area.
    // or is too small
    //

    _DeskToPreview(&rcU, &rc);
    x = ((rcPrev.right  - rcPrev.left)-(rc.right  - rc.left))/2;
    y = ((rcPrev.bottom - rcPrev.top) -(rc.bottom - rc.top))/2;

    if (rcU.left < 0 || rcU.top < 0 || x < 0 || y < 0 ||
        rcU.right > rcPrev.right || rcU.bottom > rcPrev.bottom ||
        (x > (rcPrev.right-rcPrev.left)/8 &&
         y > (rcPrev.bottom-rcPrev.top)/8))
    {
        _rcDesk = rcU;
        sx = MulDiv(rcPrev.right  - rcPrev.left - 16,1000,_rcDesk.right  - _rcDesk.left);
        sy = MulDiv(rcPrev.bottom - rcPrev.top  - 16,1000,_rcDesk.bottom - _rcDesk.top);

        _DeskScale = min(sx,sy) * 2 / 3;
        _DeskToPreview(&_rcDesk, &rc);
        _DeskOff.x = ((rcPrev.right  - rcPrev.left)-(rc.right  - rc.left))/2;
        _DeskOff.y = ((rcPrev.bottom - rcPrev.top) -(rc.bottom - rc.top))/2;
    }

    //
    // Show all the windows and save them all to the devmode.
    //

    for (i=0; i < n; i++)
    {
        RECT rcPos;
        POINT ptPos;
        
        _Devices[arcDev[i]].pds->GetCurPosition(&rcPos);
        hwndC = GetDlgItem(hwndP, (int) &_Devices[arcDev[i]]);
        _DeskToPreview(&arc[i], &rc);

        rc.right =  MulDiv(RECTWIDTH(rcPos),  _DeskScale, 1000);
        rc.bottom = MulDiv(RECTHEIGHT(rcPos), _DeskScale, 1000);

        TraceMsg(TF_GENERAL, "_CleanupRects set Dev = %d, hwnd = %08lx\n", arcDev[i], hwndC);
        TraceMsg(TF_GENERAL, "_CleanupRects window pos %d,%d,%d,%d\n", rc.left, rc.top, rc.right, rc.bottom);

        SetWindowPos(hwndC,
                     NULL,
                     rc.left,
                     rc.top,
                     rc.right,
                     rc.bottom,
                     SWP_NOZORDER);

        ptPos.x = arc[i].left;
        ptPos.y = arc[i].top;
        _Devices[arcDev[i]].pds->SetCurPosition(&ptPos);
    }

}

BOOL CMultiMon::_WriteMultiMonProfile()
{
    return WritePrivateProfileString(sc_szVideo, sc_szOptimize, TEXT("bmp"), sc_szQtwIni);
}

BOOL CMultiMon::HandleMonitorChange(HWND hwndP, BOOL bMainDlg)
{
    if (!bMainDlg && _InSetInfo)
        return FALSE;

    SetDirty();

    if (bMainDlg)
        BringWindowToTop(hwndP);
    _CleanupRects(GetParent(hwndP));
    UpdateActiveDisplay(_pCurDevice);
    return TRUE;
}

BOOL CMultiMon::RegisterPreviewWindowClass(WNDPROC pfnWndProc)
{
    TraceMsg(TF_GENERAL, "InitMultiMonitorDlg\n");
    WNDCLASS         cls;

    cls.hCursor        = LoadCursor(NULL,IDC_ARROW);
    cls.hIcon          = NULL;
    cls.lpszMenuName   = NULL;
    cls.lpszClassName  = TEXT("Monitor32");
    cls.hbrBackground  = (HBRUSH)(COLOR_DESKTOP + 1);
    cls.hInstance      = hInstance;
    cls.style          = CS_VREDRAW | CS_HREDRAW | CS_DBLCLKS;
    cls.lpfnWndProc    = (WNDPROC)pfnWndProc;
    cls.cbWndExtra     = SIZEOF(LPVOID);
    cls.cbClsExtra     = 0;

    return RegisterClass(&cls);
}

LRESULT CALLBACK DeskWndProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam, UINT uID, DWORD dwRefData);

BOOL CMultiMon::_InitDisplaySettings(BOOL bExport)
{
    HWND             hwndC;
    int              iItem;
    DWORD            iDevice = 0;
    DWORD            iEnum;
    LONG             iPrimeDevice = -1;
    TCHAR            ach[128];
    PMULTIMON_DEVICE pDevice;
    RECT             rcPrimary;
    BOOL             f;

    //
    // Reset some globals.
    // Clear out the list box.
    //

    _InSetInfo = 1;

    ComboBox_ResetContent(_hwndList);
    SetRectEmpty(&_rcDesk);

    while (hwndC = GetWindow(_hwndDesk, GW_CHILD))
        DestroyWindow(hwndC);

    ShowWindow(_hwndDesk, SW_HIDE);

    if (_himl != NULL)
    {
        ImageList_Destroy(_himl);
        _himl = NULL;
    }

    //
    // Reenumerate all the devices in the system.
    //
    for (iDevice = 0, iEnum = 0; iEnum < MONITORS_MAX; iEnum++)
    {
        TraceMsg(TF_GENERAL, "Device %d", iEnum);

        pDevice = &_Devices[iDevice];
        ZeroMemory(pDevice, sizeof(DISPLAY_DEVICE));
        pDevice->DisplayDevice.cb = sizeof(DISPLAY_DEVICE);

        f = EnumDisplayDevices(NULL, iEnum, &pDevice->DisplayDevice, 0);

        //
        // ignore device's we cant create a DC for.
        //
        if (f)
        {
            pDevice->hdc = CreateDC(NULL,(LPTSTR)pDevice->DisplayDevice.DeviceName,NULL,NULL);
            f = pDevice->hdc != NULL;
        }

        //
        // EnumDisplayDevices is returning NO devices, this is bad
        // invent a fake device.
        //
        if (!f && iEnum == 0)
        {
            pDevice->DisplayDevice.DeviceName[0] = 0;

            LoadString(hInstance, IDS_UNKNOWNDEVICE,
                (LPTSTR)&pDevice->DisplayDevice.DeviceString[0],
                SIZEOF(pDevice->DisplayDevice.DeviceString));

            pDevice->DisplayDevice.StateFlags = DISPLAY_DEVICE_PRIMARY_DEVICE;

            gbExecMode    = EXEC_INVALID_MODE;
            gbInvalidMode = EXEC_INVALID_DISPLAY_DEVICE;

            f = TRUE;
        }

        if (f)
        {
            TraceMsg(TF_GENERAL, "%s   %s\n", pDevice->DisplayDevice.DeviceName, pDevice->DisplayDevice.DeviceString);

            //
            // We won't even include the MIRRORING drivers in the list for
            // now.
            //
            if (pDevice->DisplayDevice.StateFlags & DISPLAY_DEVICE_MIRRORING_DRIVER)
            {
                TraceMsg(TF_GENERAL, "Mirroring driver - skip it\n");
                continue;
            }

            //
            // get the device software key
            //
            pDevice->DisplayRegKey[0] = 0;
            GetDisplayKey(iEnum, pDevice->DisplayRegKey, sizeof(pDevice->DisplayRegKey));

            TraceMsg(TF_GENERAL, "DeviceKey %s", pDevice->DisplayRegKey);

            FillMemory(&(pDevice->DisplaySetting), sizeof(DEVMODE), 0);
            pDevice->DisplaySetting.dmSize = sizeof(DEVMODE);

            _SetReposition(_hwndDesk, pDevice);
            pDevice->pds = new CDeviceSettings(_hDlg);
            if (pDevice->pds->InitSettings(&pDevice->DisplayDevice))
                iDevice++;
            else
                delete pDevice->pds;
        }
        else
        {
            //
            // Mark the end of the list of devices.
            //
            TraceMsg(TF_GENERAL, "End of list\n");

            pDevice->DisplayDevice.cb = 0;
            break;
        }
    }

    if (iDevice == 0)
    {
        ASSERT(0);
        return FALSE;
    }
    //
    // Because we are getting the registry values, the current state of
    // the registry may be inconsistent with that of the system:
    //
    // EmumDisplayDevices will return the active primary in the
    // system, which may be different than the actual primary marked in the
    // registry
    //
    // First, we determine if the registry is in a consistent state with
    // the system by making sure the system primary is also marked as
    // primary in the regsitry.  This means the primary as returned by
    // EnumDisplayDevices is also at position 0,0, with the DM_POSITION flag
    // set.
    //
    // If this is not the case, then we will present the current state of the
    // registry (if there is any) or the primary device only (for a non-
    // configured system) and ask the user to properly configure the devices.
    //

    _pPrimaryDevice = NULL;
    for (iDevice = 0; _Devices[iDevice].DisplayDevice.cb != 0; iDevice++)
    {
        if (_Devices[iDevice].pds->IsAttached() &&
            (_Devices[iDevice].DisplaySetting.dmPosition.x  == 0)   &&
            (_Devices[iDevice].DisplaySetting.dmPosition.y == 0)    &&
            _Devices[iDevice].pds->IsPrimary())
        {
            if (iPrimeDevice == -1)
            {
                iPrimeDevice = iDevice;
                _pPrimaryDevice = &_Devices[iDevice];
            }
            else
            {
                // Multiple primaries are set !
                ASSERT(FALSE);
                //_pPrimaryDevice = NULL;
            }
        }
    }

    //
    // Primary is not set correctly !
    // Let's show the state of the registry if it appears the user has ever
    // setup multiple displays in the registry.
    //

    if (_pPrimaryDevice == NULL)
    {
        for (iDevice = 0; _Devices[iDevice].DisplayDevice.cb != 0; iDevice++)
        {
            //
            // We can have multiple devices at this location since we
            // support overlapping devices.  Just pick anyone since this is an
            // error path.  Just make sure we are not picking a remote control
            // driver.
            //

            if (_Devices[iDevice].pds->IsAttached() &&
                ((_pPrimaryDevice == NULL) || _Devices[iDevice].pds->IsPrimary()))
            {
                TraceMsg(TF_GENERAL, "InitDisplaySettings: attached device found\n");
                
                iPrimeDevice = iDevice;
                _pPrimaryDevice = &_Devices[iDevice];
            }
        }

        //
        // If we did find a device, then let's make sure we reset the primary
        // flag in our structure to be the one we will reset to 0,0
        //

        if (_pPrimaryDevice != NULL)
        {
            WarnMsg(TEXT("InitDisplaySettings:"), TEXT("Resetting primary"));
            for (iDevice = 0; _Devices[iDevice].DisplayDevice.cb != 0; iDevice++)
                _Devices[iDevice].pds->SetPrimary(&_Devices[iDevice] == _pPrimaryDevice);
        }
    }

    //
    // If we still do not have a primary device, (no registry information)
    // assume this is an initial multimonitor setup and we should just deal
    // with the primary device.
    //
    // We will have to setup at least one rectangle.
    //

    if (_pPrimaryDevice == NULL)
    {
        for (iDevice = 0; _Devices[iDevice].DisplayDevice.cb != 0; iDevice++)
        {
            if (_Devices[iDevice].pds->IsAttached())
            {
                POINT ptPos = {0,0};
                iPrimeDevice = iDevice;
                _pPrimaryDevice = &_Devices[iDevice];
                _pPrimaryDevice->pds->SetCurPosition(&ptPos);
                break;
            }
        }
    }

    //
    // Reset the primary's variables to make sure it is a properly formated
    // primary entry.
    //
    ASSERT(_pPrimaryDevice != NULL);
    if (_pPrimaryDevice == NULL)
    {
        WarnMsg(TEXT("InitDisplaySettings: No Primary Device!!!!"), TEXT(""));
        return FALSE;
    }
    _pPrimaryDevice->pds->SetPrimary(TRUE);
    _pPrimaryDevice->pds->GetCurPosition(&rcPrimary);

    if (!lstrcmpi((LPTSTR)_pPrimaryDevice->DisplayDevice.DeviceString,sc_szVGA))
        _bNoAttach = TRUE;

    //
    // compute the max image size needed for a monitor bitmap
    //
    // NOTE this must be the max size the images will *ever*
    // be we cant just take the current max size.
    // we use the client window size, a child monitor cant be larger than this.
    //
    RECT rcDesk;
    GetClientRect(_hwndDesk, &rcDesk);
    int cxImage = rcDesk.right;
    int cyImage = rcDesk.bottom;

    //
    // load the monitor bitmap
    //
    HBITMAP hbm = (HBITMAP)LoadImage(hInstance, MAKEINTRESOURCE(BMP_MONITOR2),
            IMAGE_BITMAP, cxImage, cyImage, 0);

    //
    // Go through all the devices to count them
    //
    for (iDevice = 0; _Devices[iDevice].DisplayDevice.cb != 0; iDevice++)
        ;

    // For the ASSERT: We should have at least the primary device
    ASSERT(iDevice > 0);

    _NumDevices = iDevice;

    //
    // Go through all the devices one last time to create the windows
    //
    for (iDevice = 0; _Devices[iDevice].DisplayDevice.cb != 0; iDevice++)
    {
        TCHAR szDisplay[256];
        pDevice = &_Devices[iDevice];
        MonitorData md = {0};
        RECT rcPos;
        LPVOID pWindowData = (LPVOID)this;
        pDevice->DisplayIndex = iDevice + 1;
        _GetDisplayName(pDevice, szDisplay);
        iItem = ComboBox_AddString(_hwndList, szDisplay);

        pDevice->ComboBoxItem = iItem;

        ComboBox_SetItemData(_hwndList,
                             iItem,
                             (DWORD)pDevice);

        //
        // If the monitor is part of the desktop, show it on the screen
        // otherwise keep it invisible.
        //
        // On NT, we require all the devices to have the same color depth.
        // Don't let them on the desktop if they don't
        //
#ifdef WINNT
        if (pDevice->DisplaySetting.dmBitsPerPel != _pPrimaryDevice->DisplaySetting.dmBitsPerPel)
            pDevice->pds->SetAttached(FALSE);
#endif
        wsprintf(ach, TEXT("%d"), iDevice + 1);

        if (!pDevice->pds->IsAttached())
        {
            // By default set the unattached monitors to the right of the primary monitor
            POINT ptPos = {rcPrimary.right, rcPrimary.top};
            pDevice->pds->SetCurPosition(&ptPos);
        }

        pDevice->pds->GetCurPosition(&rcPos);

        if (bExport)
        {
            md.dwSize = SIZEOF(MonitorData);
            if ( pDevice->pds->IsPrimary() )
                md.dwStatus |= MD_PRIMARY;
            if ( pDevice->pds->IsAttached() )
                md.dwStatus |= MD_ATTACHED;
            md.rcPos = rcPos;

            pWindowData = &md;
        }

        if (_himl == NULL)
        {
            // use ILC_COLORDDB to always get stipple selection
            // UINT flags = ILC_COLORDDB | ILC_MASK;

            // use ILC_COLOR4   to get HiColor selection
            UINT flags = ILC_COLOR4 | ILC_MASK;

            _himl = ImageList_Create(cxImage, cyImage, flags, _NumDevices, 1);
            ASSERT(_himl);
            ImageList_SetBkColor(_himl, GetSysColor(COLOR_APPWORKSPACE));
        }

        pDevice->w      = -1;
        pDevice->h      = -1;
        pDevice->himl   = _himl;
        pDevice->iImage = ImageList_AddMasked(_himl, hbm, CLR_DEFAULT);

        TraceMsg(TF_GENERAL, "InitDisplaySettings: Creating preview windows %s at %d %d %d %d",
                 ach, rcPos.left, rcPos.top, rcPos.right, rcPos.bottom);
        hwndC = CreateWindowEx(
                               0, // WS_EX_CLIENTEDGE,
                               TEXT("Monitor32"), ach,
                               WS_CLIPSIBLINGS | /* WS_DLGFRAME | */ WS_VISIBLE | WS_CHILD,
                               rcPos.left, rcPos.top, RECTWIDTH(rcPos), RECTHEIGHT(rcPos),
                               _hwndDesk,
                               (HMENU)pDevice,
                               hInstance,
                               pWindowData);

        ASSERT(hwndC);
    }

    //  nuke the temp monitor bitmap
    if (hbm)
        DeleteObject(hbm);

    // Set the primary device as the current device
    ASSERT(iPrimeDevice >= 0);
    ComboBox_SetCurSel(_hwndList, iPrimeDevice);
    _pCurDevice = _pPrimaryDevice;

    // Initialize all the constants and the settings fields
    _DeskScale = 1000;
    _DeskOff.x = 0;
    _DeskOff.y = 0;
    _CleanupRects(_hwndDesk);
    _VerifyPrimaryMode(FALSE);
    UpdateActiveDisplay(_pCurDevice);
    ShowWindow(_hwndDesk, SW_SHOW);
    _InSetInfo--;
    return TRUE;
}
                    
//-----------------------------------------------------------------------------
void CMultiMon::InitMultiMonitorDlg(HWND hDlg)
{
    HCURSOR hcur;

    hcur = SetCursor(LoadCursor(NULL, IDC_WAIT));
    _hDlg = hDlg;
    _hwndDesk = GetDlgItem(_hDlg, IDC_DISPLAYDESK);
    _hwndList = GetDlgItem(_hDlg, IDC_DISPLAYLIST);
    
#ifdef WINNT
    //
    // Determine in what mode we are running the applet before getting information
    //
    _vPreExecMode();
#endif


    RegisterPreviewWindowClass(&MonitorWindowProc);
    _InitDisplaySettings(FALSE);

    // Now: depends on whether we have a multimon system, change the UI
    if (_NumDevices == 1)
    {
        HWND hwndMultimonHelp = GetDlgItem(_hDlg, IDC_MULTIMONHELP);
        ShowWindow(hwndMultimonHelp, SW_HIDE);
        ShowWindow(_hwndDesk, SW_HIDE);
        
        // set up bitmaps for sample screen
        _hbmScrSample = LoadMonitorBitmap( TRUE ); // let them do the desktop
        SendDlgItemMessage(_hDlg, IDC_SCREENSAMPLE, STM_SETIMAGE, IMAGE_BITMAP, (DWORD)_hbmScrSample);
        
        // get a base copy of the bitmap for when the "internals" change
        _hbmMonitor = LoadMonitorBitmap( FALSE ); // we'll do the desktop
        
        //Hide the combo box, keep the static text
        ShowWindow(_hwndList, SW_HIDE);

        //Set the name of the primary in the static text
        //strip the first token off (this is the number we dont want it)
        TCHAR *pch, szDisplay[MAX_PATH];
        _GetDisplayName(_pPrimaryDevice, szDisplay);
        for (pch=szDisplay; *pch && *pch != TEXT(' '); pch++);
        for (;*pch && *pch == TEXT(' '); pch++);
        SetDlgItemText(_hDlg, IDC_DISPLAYTEXT, pch);
    }
    else if (_NumDevices > 0)
    {
        //Hide the static text, keep the combo box
        ShowWindow(GetDlgItem(_hDlg, IDC_DISPLAYTEXT), SW_HIDE);

        // Hide the Multimon version of the preview objects
        ShowWindow(GetDlgItem(_hDlg, IDC_SCREENSAMPLE), SW_HIDE);

        // In case of multiple devices, subclass the _hwndDesk window for key board support
        SetWindowSubclass(_hwndDesk, DeskWndProc, 0, (DWORD)this);
        ShowWindow(_hwndDesk, SW_SHOW);
    }


#ifdef WINNT
    //
    // Determine if any errors showed up during enumerations and initialization
    //
    _vPostExecMode();
    
    //
    // Now tell the user what we found out during initialization
    // Errors, or what we found during detection
    //

    PostMessage(hDlg, MSG_DSP_SETUP_MESSAGE, 0, 0);
    //
    // Since this could have taken a very long time, just make us visible
    // if another app (like progman) came up.
    //

    ShowWindow(hDlg, SW_SHOW);
#endif

    SetCursor(hcur);
}


LRESULT CALLBACK DeskWndProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam, UINT uID, DWORD dwRefData)
{
    CMultiMon * pcmm = (CMultiMon *)dwRefData;
    HWND hwndC;
    HDC  hdc;
    RECT rcPos;
    BOOL bMoved = TRUE;
    LRESULT lret;
    int iMonitor;
    switch(message)
    {
        case WM_GETDLGCODE:
            return DLGC_WANTCHARS | DLGC_WANTARROWS;

        case WM_KILLFOCUS:
            RedrawWindow(hDlg, NULL, NULL, RDW_INVALIDATE);
            break;

        case WM_PAINT:
            if (GetFocus() != hDlg)
                break;
            lret = DefSubclassProc(hDlg, message, wParam, lParam);

            // Fall through
        case WM_SETFOCUS:
            hdc = GetDC(hDlg);
            GetClientRect(hDlg, &rcPos);
            InflateRect(&rcPos, -1, -1);
            //DrawFocusRect(hdc, &rcPos);
            DrawEdge(hdc, &rcPos, EDGE_SUNKEN, BF_TOP | BF_BOTTOM | BF_LEFT | BF_RIGHT);
            ReleaseDC(hDlg, hdc);

            if (message == WM_PAINT)
                return lret;
            break;            

        case WM_LBUTTONDOWN:
            SetFocus(hDlg);
            break;
            
        case WM_KEYDOWN:
#define MONITORMOVEUNIT 3
            hwndC = pcmm->GetCurDeviceHwnd();
            GetWindowRect(hwndC, &rcPos);
            MapWindowRect(NULL, hDlg, &rcPos);
            switch(wParam)
            {
                case VK_LEFT:
                    MoveWindow(hwndC, rcPos.left - MONITORMOVEUNIT, rcPos.top, RECTWIDTH(rcPos), RECTHEIGHT(rcPos), TRUE);
                    break;
                case VK_RIGHT:
                    MoveWindow(hwndC, rcPos.left + MONITORMOVEUNIT, rcPos.top, RECTWIDTH(rcPos), RECTHEIGHT(rcPos), TRUE);
                    break;
                case VK_UP:
                    MoveWindow(hwndC, rcPos.left, rcPos.top - MONITORMOVEUNIT, RECTWIDTH(rcPos), RECTHEIGHT(rcPos), TRUE);
                    break;
                case VK_DOWN:
                    MoveWindow(hwndC, rcPos.left, rcPos.top + MONITORMOVEUNIT, RECTWIDTH(rcPos), RECTHEIGHT(rcPos), TRUE);
                    break;
                default:
                    bMoved = FALSE;
                    break;
            }
#undef MONITORMOVEUNIT
            
            pcmm->HandleMonitorChange(hwndC, FALSE);
            break;

        case WM_CHAR:
            if (pcmm)
            {
                iMonitor = (TCHAR)wParam - TEXT('0');
                if ((iMonitor == 0) && (pcmm->GetNumDevices() >= 10))
                    iMonitor = 10;

                if ((iMonitor > 0) && (iMonitor <= pcmm->GetNumDevices()))
                {
                    HWND hwndList = GetDlgItem(GetParent(hDlg), IDC_DISPLAYLIST);
                    ComboBox_SetCurSel(hwndList, iMonitor - 1);
                    pcmm->UpdateActiveDisplay(NULL);
                    return 0;
                }
            }
            break;

        case WM_DESTROY:
            RemoveWindowSubclass(hDlg, DeskWndProc, 0);
            break;

        default:
            break;
    }
    
    return DefSubclassProc(hDlg, message, wParam, lParam);
}


//-----------------------------------------------------------------------------
//
// Callback functions PropertySheet can use
//
BOOL CALLBACK
MultiMonitorDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    CMultiMon * pcmm = (CMultiMon *) GetWindowLong(hDlg, DWL_USER);
    switch (message)
    {
        case WM_INITDIALOG:
            ASSERT(!pcmm);
            pcmm = new CMultiMon;
            if (pcmm)
            {
                SetWindowLong(hDlg, DWL_USER, (LPARAM)pcmm);
                ghwndPropSheet = GetParent(hDlg);
                pcmm->InitMultiMonitorDlg(hDlg);

                //
                // if we have a invalid mode force the user to Apply
                //
                if (gbExecMode == EXEC_INVALID_MODE)
                    pcmm->SetDirty();

                return TRUE;
            }
            break;
        case WM_DESTROY:
            if (pcmm)
            {
                pcmm->WndProc(message, wParam, lParam);
                SetWindowLong(hDlg, DWL_USER, NULL);
                delete pcmm;
            }
            break;
        default:
            if (pcmm)
                return pcmm->WndProc(message, wParam, lParam);
            break;
    }

    return FALSE;
}

void CMultiMon::_SetPreviewScreenSize(int HRes, int VRes, int iOrgXRes, int iOrgYRes)
{
    HBITMAP hbmOld;
    HBRUSH hbrOld;
    HDC hdcMem2;

    // stretching the taskbar could get messy, we'll only do the desktop
    int mon_dy = MON_DY - MON_TRAY;

    // init to identical extents
    SIZE dSrc = { MON_DX, mon_dy };
    SIZE dDst = { MON_DX, mon_dy };

    // set up a work area to play in
    if (!_hbmMonitor || !_hbmScrSample)
        return;
    hdcMem2 = CreateCompatibleDC(g_hdcMem);
    if (!hdcMem2)
        return;
    SelectObject(hdcMem2, _hbmScrSample);
    hbmOld = (HBITMAP)SelectObject(g_hdcMem, _hbmMonitor);

    // see if we need to shrink either aspect of the image
    if (HRes > iOrgXRes || VRes > iOrgYRes)
    {
        // make sure the uncovered area will be seamless with the desktop
        RECT rc = { MON_X, MON_Y, MON_X + MON_DX, MON_Y + mon_dy };
        HBRUSH hbr =
                    CreateSolidBrush( GetPixel( g_hdcMem, MON_X + 1, MON_Y + 1 ) );

        FillRect(hdcMem2, &rc, hbr);
        DeleteObject( hbr );
    }

    // stretch the image to reflect the new resolution
    if( HRes > iOrgXRes )
        dDst.cx = MulDiv( MON_DX, iOrgXRes, HRes );
    else if( HRes < iOrgXRes )
        dSrc.cx = MulDiv( MON_DX, HRes, iOrgXRes );

    if( VRes > iOrgYRes )
        dDst.cy = MulDiv( mon_dy, iOrgYRes, VRes );
    else if( VRes < iOrgYRes )
        dSrc.cy = MulDiv( mon_dy, VRes, iOrgYRes );

    SetStretchBltMode( hdcMem2, COLORONCOLOR );
    StretchBlt( hdcMem2, MON_X, MON_Y, dDst.cx, dDst.cy,
                g_hdcMem, MON_X, MON_Y, dSrc.cx, dSrc.cy, SRCCOPY );

    // now fill the new image's desktop with the possibly-dithered brush
    // the top right corner seems least likely to be hit by the stretch...
    hbrOld = (HBRUSH)SelectObject( hdcMem2, GetSysColorBrush( COLOR_DESKTOP ) );
    ExtFloodFill(hdcMem2, MON_X + MON_DX - 2, MON_Y+1,
                 GetPixel(hdcMem2, MON_X + MON_DX - 2, MON_Y+1), FLOODFILLSURFACE);

    // clean up after ourselves
    SelectObject( hdcMem2, hbrOld );
    SelectObject( hdcMem2, g_hbmDefault );
    DeleteObject( hdcMem2 );
    SelectObject( g_hdcMem, hbmOld );    
}

void CMultiMon::_RedrawDeskPreviews()
{
    if (_NumDevices > 1)
    {
        _CleanupRects(_hwndDesk);
        RedrawWindow(_hwndDesk, NULL, NULL, RDW_ALLCHILDREN | RDW_ERASE | RDW_INVALIDATE);
    }
    else if (_pCurDevice && _pCurDevice->pds)
    {
        RECT rcPos, rcOrigPos;
        _pCurDevice->pds->GetCurPosition(&rcPos);
        _pCurDevice->pds->GetOrigPosition(&rcOrigPos);
        _SetPreviewScreenSize(RECTWIDTH(rcPos), RECTHEIGHT(rcPos), RECTWIDTH(rcOrigPos), RECTHEIGHT(rcOrigPos));
        // only invalidate the "screen" part of the monitor bitmap
        rcPos.left = MON_X;
        rcPos.top = MON_Y;
        rcPos.right = MON_X + MON_DX + 2;  // fudge (trust me)
        rcPos.bottom = MON_Y + MON_DY + 1; // fudge (trust me)
        InvalidateRect(GetDlgItem(_hDlg, IDC_SCREENSAMPLE), &rcPos, FALSE);
    }
}

BOOL CMultiMon::_HandleApply()
{
    BOOL bReboot = FALSE;
    BOOL bTest = FALSE;
    int  iTestResult;
    int  iSave;
#ifdef WINNT
    //
    // If a new driver is installed, we just want to tell the
    // system to reboot.
    //
    // NOTE - this is only until we can get drivers to load on the fly.
    //
    if (_bNewDriver)
    {
        PropSheet_RestartWindows(ghwndPropSheet);
        SetWindowLong(_hDlg, DWL_MSGRESULT, PSNRET_NOERROR);
        return TRUE;
    }
#endif

#ifndef WINNT   //BUGBUG should we do this on NT???
    //
    // if the user hits apply when we dont have a valid display device
    // force VGA mode in the registry and restart.
    //
    if (gbExecMode    == EXEC_INVALID_MODE &&
        gbInvalidMode == EXEC_INVALID_DISPLAY_DEVICE)
    {
        NukeDisplaySettings();
        PropSheet_RestartWindows(ghwndPropSheet);
        return TRUE;
    }

    //
    // if we are in VGA fallback mode, and the user stays in VGA mode
    // also nuke all settings from the registry.
    //
    if (gbExecMode    == EXEC_INVALID_MODE &&
        gbInvalidMode == EXEC_INVALID_DISPLAY_MODE &&
        _pPrimaryDevice->pds->GetColorBits() == 4)
    {
        NukeDisplaySettings();
        PropSheet_RestartWindows(ghwndPropSheet);
        return TRUE;
    }
#endif

    // Test the new settings first
    iTestResult = _SaveDisplaySettings(CDS_TEST);

    if (iTestResult == DISP_CHANGE_RESTART)
        bReboot = TRUE;
    else if (iTestResult < 0)
        return FALSE;
    
    // Ask first and then change the settings.
    if (!bReboot && _AnyColorChange())
    {
        int iDynaResult = AskDynaCDS(_hDlg);
        if (iDynaResult == -1)
            return FALSE;
        else if (iDynaResult == 0)
            bReboot = TRUE;
    }

    if (!bReboot && _AnyResolutionChange())
    {
        TCHAR szWarnFlicker[510];
        LPTSTR pstr;

        LoadString(hInstance, IDS_WARNFLICK1, szWarnFlicker, SIZEOF(szWarnFlicker));
        pstr = szWarnFlicker + lstrlen(szWarnFlicker);
        LoadString(hInstance, IDS_WARNFLICK2, pstr, SIZEOF(szWarnFlicker) - (pstr - (LPTSTR)szWarnFlicker));
        if (ShellMessageBox(hInstance, GetParent(_hDlg), szWarnFlicker, NULL, MB_OKCANCEL | MB_ICONINFORMATION) != IDOK)
            return FALSE;
        bTest = TRUE;
    }

    iSave = _SaveDisplaySettings(CDS_UPDATEREGISTRY | CDS_NORESET);

    // This shouldn't fail because we already tested it above.
    ASSERT(iSave >= 0);
    if (iSave == DISP_CHANGE_RESTART)
        bReboot = TRUE;
    else if ((iSave < 0) || (bTest && !_TestNewSettingsChange()))
    {
        _RestoreDisplaySettings();
        return FALSE;
    }
    else
        _ConfirmDisplaySettingsChange();
    
    if (bReboot)
        PropSheet_RestartWindows(ghwndPropSheet);

    return TRUE;
}

CMultiMon::_HandleHScroll(HWND hwndTB, int iCode, int iPos)
{
    int cRes = SendMessage(hwndTB, TBM_GETRANGEMAX, TRUE, 0);
    int iRes = _pCurDevice->pds->GetCurResolution();

    switch(iCode ) {
        case TB_LINEUP:
        case TB_PAGEUP:
            if (iRes != 0)
                iRes--;
            break;

        case TB_LINEDOWN:
        case TB_PAGEDOWN:
            if (++(iRes) >= cRes)
                iRes = cRes;
            break;

        case TB_BOTTOM:
            iRes = cRes;
            break;

        case TB_TOP:
            iRes = 0;
            break;

        case TB_THUMBTRACK:
        case TB_THUMBPOSITION:
            iRes = iPos;
            break;

        default:
            return FALSE;
    }


    _pCurDevice->pds->ChangeResolution(iRes);
    _VerifyPrimaryMode(TRUE);

    if ( (gbExecMode == EXEC_NORMAL) ||
         (gbExecMode == EXEC_INVALID_MODE) ||
         (gbExecMode == EXEC_DETECT) ) {

        //
        // Set the apply button if resolution has changed
        //

        if (_pCurDevice->pds->IsResolutionChanged())
            SetDirty();

        return 0;
    }

    return TRUE;
}

void CMultiMon::_ForwardToChildren(UINT msg, WPARAM wParam, LPARAM lParam)
{
    HWND hwndC = GetDlgItem(_hDlg, IDC_SCREENSIZE);
    if (hwndC)
        SendMessage(hwndC, msg, wParam, lParam);
}

LRESULT CALLBACK CMultiMon::WndProc(UINT message, WPARAM wParam, LPARAM lParam)
{
    NMHDR FAR *lpnm;
    HWND hwndC;
    HWND hwndSample;
    HBITMAP hbm;
    
    switch (message)
    {
    case WM_NOTIFY:
        lpnm = (NMHDR FAR *)lParam;
        switch (lpnm->code)
        {
            case PSN_APPLY:
                if (IsDirty())
                {
                    _HandleApply();
                    SetDirty(FALSE);

                    // if the close flag is false, rebuild the settings
                    if (((PSHNOTIFY*)lParam)->lParam == FALSE)
                        _RebuildDisplaySettings(TRUE);
                }
                break;
            default:
                return FALSE;
        }
        break;

    case WM_CTLCOLORSTATIC:
        if (GetDlgCtrlID((HWND)lParam) == IDC_DISPLAYDESK)
        {
            return (BOOL)(UINT)GetSysColorBrush(COLOR_APPWORKSPACE);
        }
        return FALSE;


    case WM_COMMAND:

        switch (GET_WM_COMMAND_ID(wParam, lParam))
        {
#ifdef WINNT
        case IDC_DISPLAYPRIME:
#endif
        case IDC_DISPLAYUSEME:
        {
            //
            // Don't pop up warning dialog box if this display is already attached
            // or if there are already more than 1 display
            //
            BOOL bWarn = (!_pCurDevice->pds->IsAttached() && GetNumberOfAttachedDisplays() == 1);
            
            if (!ToggleAttached(_pCurDevice, GET_WM_COMMAND_ID(wParam, lParam) ==
                                IDC_DISPLAYPRIME))
                return FALSE;
            
            if ((GET_WM_COMMAND_ID(wParam, lParam) == IDC_DISPLAYUSEME) && bWarn
                && !WarnUserAboutCompatibility(_hDlg))
            {
                _pCurDevice->pds->SetAttached(FALSE);
                UpdateActiveDisplay(_pCurDevice);
            }
            else
            {
                hwndC = GetDlgItem(_hwndDesk, (int) _pCurDevice);
                HandleMonitorChange(hwndC, TRUE);
            }
            return TRUE;
        }  
        case IDC_DISPLAYLIST:
            switch (GET_WM_COMMAND_CMD(wParam, lParam))
            {
            case CBN_DBLCLK:
                goto DoDeviceSettings;

            case CBN_SELCHANGE:
                UpdateActiveDisplay(NULL);
                break;

            default:
                return FALSE;
            }
            break;

        case IDC_DISPLAYPROPERTIES:
            switch (GET_WM_COMMAND_CMD(wParam, lParam))
            {
            DoDeviceSettings:
            case BN_CLICKED:
                if (IsWindowEnabled(GetDlgItem(_hDlg, IDC_DISPLAYPROPERTIES)))
                {
                    _DoAdvancedSettingsSheet();
                    _RebuildDisplaySettings(TRUE);
                }
                break;

            default:
                return FALSE;
            }
            break;

        case IDC_COLORBOX:
            switch(GET_WM_COMMAND_CMD(wParam, lParam))
            {
                
                case CBN_SELCHANGE:
                {
                    HWND hwndColorBox = GetDlgItem(_hDlg, IDC_COLORBOX);
                    int iClr = ComboBox_GetCurSel(hwndColorBox);
                    
                    if (iClr != CB_ERR ) {

                        _pCurDevice->pds->ChangeColor(iClr);
                        _VerifyPrimaryMode(TRUE);
                    }

                    break;
                }
                default:
                    break;
            }

            break;
            
        default:
            return FALSE;
        }


        //
        // Enable the apply button only if we are not in setup.
        //
        
        if ( (gbExecMode == EXEC_NORMAL) ||
             (gbExecMode == EXEC_INVALID_MODE) ||
             (gbExecMode == EXEC_DETECT) ) {
            
            //
            // Set the apply button if something changed
            //
            if (_pCurDevice->pds->IsResolutionChanged() ||
                _pCurDevice->pds->IsColorChanged() ||
                _pCurDevice->pds->IsFrequencyChanged())
            {
                SetDirty();
            }

            return TRUE;
        }
        break;

    case WM_HSCROLL:
        _HandleHScroll((HWND)lParam, (int) LOWORD(wParam), (int) HIWORD(wParam));
        break;
        
    case WM_HELP:
        WinHelp((HWND)((LPHELPINFO)lParam)->hItemHandle, TEXT("mds.hlp"), HELP_WM_HELP,
            (DWORD)(LPTSTR)sc_MultiMonitorHelpIds);
        break;

    case WM_CONTEXTMENU:
        WinHelp((HWND)wParam, TEXT("mds.hlp"), HELP_CONTEXTMENU,
            (DWORD)(LPTSTR)sc_MultiMonitorHelpIds);
        break;

    case WM_WININICHANGE:
        _ForwardToChildren(message, wParam, lParam);
        break;

    case WM_SYSCOLORCHANGE:
        if (_himl)
            ImageList_SetBkColor(_himl, GetSysColor(COLOR_APPWORKSPACE));
        _ForwardToChildren(message, wParam, lParam);
        break;

    case WM_DISPLAYCHANGE:
        _RebuildDisplaySettings();
        _RedrawDeskPreviews();
        _ForwardToChildren(message, wParam, lParam);
        break;
        
    case WM_DESTROY:
        TraceMsg(TF_GENERAL, "WndProc:: WM_DESTROY");
        hwndSample = GetDlgItem(_hDlg, IDC_COLORSAMPLE);
        if (hbm = (HBITMAP)SendMessage(hwndSample, STM_SETIMAGE, IMAGE_BITMAP, NULL))
            DeleteObject(hbm);
        
        if (_NumDevices == 1)
        {
            hwndSample = GetDlgItem(_hDlg, IDC_SCREENSAMPLE);
            if (hbm = (HBITMAP)SendMessage(hwndSample, STM_SETIMAGE, IMAGE_BITMAP, NULL))
                DeleteObject(hbm);
            
            if (_hbmScrSample && (GetObjectType(_hbmScrSample) != 0))
                DeleteObject(_hbmScrSample);
            if (_hbmMonitor && (GetObjectType(_hbmMonitor) != 0))
                DeleteObject(_hbmMonitor);
        }

        _DestroyDisplaySettings();
        break;
        
#ifdef WINNT
    case MSG_DSP_SETUP_MESSAGE:
        return _InitMessage();
#endif
        // MultiMonitor CPL specific messages
    case MM_REDRAWPREVIEW:
        _RedrawDeskPreviews();
        break;

    default:
        return FALSE;
    }

    return TRUE;
}


// IUnknown methods
HRESULT CMultiMon::QueryInterface(REFIID riid, LPVOID * ppvObj)
{ 
    // ppvObj must not be NULL
    ASSERT(ppvObj != NULL);
    
    if (ppvObj == NULL)
        return E_INVALIDARG;

    *ppvObj = NULL;
    if (IsEqualIID(riid, IID_IUnknown))
        *ppvObj = SAFECAST(this, IUnknown *);
    else if (IsEqualIID(riid, IID_IMultiMonConfig))
        *ppvObj = SAFECAST(this, IMultiMonConfig *);
    else
        return E_NOINTERFACE;
     
    
    AddRef();
    return S_OK;
}


ULONG CMultiMon::AddRef()
{
    _cRef++;
    return _cRef;
}

ULONG CMultiMon::Release()
{
    _cRef--;
    if (_cRef > 0)
        return _cRef;

    delete this;
    return 0;
}


// IMultiMonConfig methods
HRESULT CMultiMon::Initialize(HWND hwndHost, WNDPROC pfnWndProc, DWORD dwReserved)
{
    WNDCLASS wc = {0};
    HRESULT hr = E_FAIL;

    if (hwndHost && RegisterPreviewWindowClass(pfnWndProc))
    {
        _hwndDesk = hwndHost;
        if (_InitDisplaySettings(TRUE))
            hr = S_OK;
    }
    return hr;
}

HRESULT CMultiMon::GetNumberOfMonitors(int * pCMon, DWORD dwReserved)
{
    if (pCMon)
    {
        *pCMon = _NumDevices;
        return S_OK;
    }

    return E_FAIL;
}

HRESULT CMultiMon::GetMonitorData(int iMonitor, MonitorData * pmd, DWORD dwReserved)
{
    ASSERT(pmd);
    if ((pmd == NULL) || (iMonitor >= _NumDevices))
        return HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);

    PMULTIMON_DEVICE pDevice = &_Devices[iMonitor];

    pmd->dwSize = SIZEOF(MonitorData);
    if ( pDevice->pds->IsPrimary() )
        pmd->dwStatus |= MD_PRIMARY;
    if ( pDevice->pds->IsAttached() )
        pmd->dwStatus |= MD_ATTACHED;
    pDevice->pds->GetCurPosition(&pmd->rcPos);

    return S_OK;
}

HRESULT CMultiMon::Paint(int iMonitor, DWORD dwReserved)
{
    _RedrawDeskPreviews();

    return S_OK;
}

STDAPI CMultiMonConfig_CreateInstance(IUnknown* pUnkOuter, REFIID riid, OUT LPVOID * ppvOut)
{
    HRESULT hr = E_OUTOFMEMORY;

    *ppvOut = NULL;                     // null the out param

    if (pUnkOuter)
        return CLASS_E_NOAGGREGATION;

    CMultiMon *pmm = new CMultiMon;

    if (pmm)
    {
        hr = pmm->QueryInterface(riid, ppvOut);
        pmm->Release();
    }

    return hr;
}

/*----------------------------------------------------------------------------
----------------------------------------------------------------------------*/
HFONT GetFont(LPRECT prc)
{
    LOGFONT lf;

    FillMemory(&lf,  SIZEOF(lf), 0);
    lf.lfWeight = FW_EXTRABOLD;
    lf.lfHeight = prc->bottom - prc->top;
    lf.lfWidth  = 0;
    lf.lfPitchAndFamily = FF_SWISS;
    lf.lfOutPrecision = OUT_TT_ONLY_PRECIS;

    return CreateFontIndirect(&lf);
}

/*----------------------------------------------------------------------------
----------------------------------------------------------------------------*/
#define HANG_TIME 2500

LONG CALLBACK BigNumberWindowProc(HWND hwnd, UINT msg,WPARAM wParam,LPARAM lParam)
{
    TCHAR ach[80];
    HFONT hfont;
    RECT  rc;
    HDC   hdc;
    HRGN  hrgnTxtA;
    HRGN  hrgnTxtB;
    HRGN  hrgnTxtU;
    PAINTSTRUCT ps;

    switch (msg)
    {
    case WM_CREATE:
        break;

    case WM_SIZE:
        GetWindowText(hwnd, ach, sizeof(ach));
        GetClientRect(hwnd, &rc);
        hfont = GetFont(&rc);

        hdc = GetDC(hwnd);
        SelectObject(hdc, hfont);

        BeginPath(hdc);
            SetBkMode(hdc, TRANSPARENT);
            TextOut(hdc,0,0,ach,lstrlen(ach));
        EndPath(hdc);

        hrgnTxtA = PathToRegion(hdc);
        hrgnTxtB = CreateRectRgn(0, 0, 1, 1);
        hrgnTxtU = CreateRectRgn(0, 0, 1, 1);

        // Copy region A (our number) to a new region (B)

        CombineRgn(hrgnTxtB, hrgnTxtA, NULL, RGN_COPY);

        // Offset region B by two pixels in x and y

        OffsetRgn(hrgnTxtB, 2, 2);

        // Now create a region that is the union of A and B.
        // This gives us our number with 2 pixels worth of
        // room for shadowing.

        CombineRgn(hrgnTxtU, hrgnTxtA, hrgnTxtB, RGN_OR);
        DeleteObject(hrgnTxtA);
        DeleteObject(hrgnTxtB);
        SetWindowRgn(hwnd,hrgnTxtU,TRUE);

        ReleaseDC(hwnd, hdc);
        DeleteObject(hfont);
        break;

    case WM_TIMER:
        DestroyWindow(hwnd);
        return 0;

    case WM_PAINT:
        GetWindowText(hwnd, ach, sizeof(ach));
        GetClientRect(hwnd, &rc);
        hfont = GetFont(&rc);

        hdc = BeginPaint(hwnd, &ps);
        PatBlt(hdc, 0, 0, rc.right, rc.bottom, BLACKNESS | NOMIRRORBITMAP);
        SelectObject(hdc, hfont);
        SetTextColor(hdc, 0xFFFFFF);
        SetBkMode(hdc, TRANSPARENT);
        TextOut(hdc,0,0,ach,lstrlen(ach));
        EndPaint(hwnd, &ps);
        DeleteObject(hfont);
        break;
    }

    return DefWindowProc(hwnd,msg,wParam,lParam);
}

int Bail()
{
    POINT pt;
    POINT pt0;
    DWORD time0;
    DWORD d;

    d     = GetDoubleClickTime();
    time0 = GetMessageTime();
    pt0.x = (int)(short)LOWORD(GetMessagePos());
    pt0.y = (int)(short)HIWORD(GetMessagePos());

    if (GetTickCount()-time0 > d)
        return 2;

    if (!((GetAsyncKeyState(VK_LBUTTON) | GetAsyncKeyState(VK_RBUTTON)) & 0x8000))
        return 1;

    GetCursorPos(&pt);

    if ((pt.y - pt0.y) > 2 || (pt.y - pt0.y) < -2)
        return 1;

    if ((pt.x - pt0.x) > 2 || (pt.x - pt0.x) < -2)
        return 1;

    return 0;
}

void FlashText(LPCTSTR sz, LPRECT prc, BOOL fWait)
{
    HFONT hfont;
    SIZE  size;
    HDC   hdc;
    int   i;

    static HWND hWnd;

    if (hWnd && IsWindow(hWnd))
    {
        DestroyWindow(hWnd);
        hWnd = NULL;
    }

    if (sz == NULL)
        return;

    if (fWait)
    {
        while ((i=Bail()) == 0)
            ;

        if (i == 1)
            return;
    }

    hdc = GetDC(NULL);
    hfont = GetFont(prc);
    SelectObject(hdc, hfont);
    GetTextExtentPoint(hdc, sz, lstrlen(sz), &size);
    ReleaseDC(NULL, hdc);
    DeleteObject(hfont);

    WNDCLASS    cls;
    cls.hCursor        = LoadCursor(NULL,IDC_ARROW);
    cls.hIcon          = NULL;
    cls.lpszMenuName   = NULL;
    cls.lpszClassName  = TEXT("MonitorNumber32");
    cls.hbrBackground  = (HBRUSH)(COLOR_DESKTOP + 1);
    cls.hInstance      = hInstance;
    cls.style          = CS_VREDRAW | CS_HREDRAW;
    cls.lpfnWndProc    = (WNDPROC)BigNumberWindowProc;
    cls.cbWndExtra     = 0;
    cls.cbClsExtra     = 0;

    RegisterClass(&cls);

    hWnd = CreateWindowEx(
        WS_EX_TOPMOST, //WS_BORDER,
        TEXT("MonitorNumber32"), sz,
        WS_POPUP,
        (prc->right  + prc->left - size.cx)/2,
        (prc->bottom + prc->top  - size.cy)/2,
        size.cx,
        size.cy,
        NULL,
        NULL,
        hInstance,
        NULL);

    ShowWindow(hWnd, SW_SHOW);
    UpdateWindow(hWnd);
    SetTimer(hWnd, 1, HANG_TIME, NULL);
}

void PaintCurrentDevice(HDC hdc, LPRECT prc)
{
    FillRect(hdc, prc, GetSysColorBrush(COLOR_ACTIVECAPTION));
}

void DrawMonitorNum(HDC hdc, int w, int h, LPCTSTR sz, BOOL fDrawBackground=TRUE)
{
    HFONT    hfont;
    HFONT    hfontT;
    RECT     rc;
    COLORREF rgb;
    COLORREF rgbDesk;

    rc.left   = MON_X * w / MON_W;
    rc.top    = MON_Y * h / MON_H;
    rc.right  = rc.left + ((MON_DX * w + MON_W-1) / MON_W);
    rc.bottom = rc.top  + ((MON_DY * h + MON_H-1) / MON_H);

    rgb     = GetSysColor(COLOR_CAPTIONTEXT);
    rgbDesk = GetSysColor(COLOR_DESKTOP);

    if (fDrawBackground)
        FillRect(hdc, &rc, GetSysColorBrush(COLOR_DESKTOP));

    if (rgbDesk == rgb)
        rgb = GetSysColor(COLOR_WINDOWTEXT);

    if (rgbDesk == rgb)
        rgb = rgbDesk ^ 0x00FFFFFF;

    SetTextColor(hdc, rgb);

    hfont = GetFont(&rc);
    hfontT = (HFONT)SelectObject(hdc, hfont);
    SetTextAlign(hdc, TA_CENTER | TA_TOP);
    SetBkMode(hdc, TRANSPARENT);
    ExtTextOut(hdc, (rc.left+rc.right)/2, rc.top, 0, NULL, sz, lstrlen(sz), NULL);
    SelectObject(hdc, hfontT);
    DeleteObject(hfont);
}

BOOL MakeMonitorBitmap(int w, int h, LPCTSTR sz, HBITMAP *pBitmap, HBITMAP *pMaskBitmap, int cx, int cy)
{
    HBITMAP hbm;        // 128x128 bitmap we will return
    HBITMAP hbmT;       // bitmap loaded from resource
    HBITMAP hbmM;       // mask bitmap
    HDC     hdc;        // work dc
    HDC     hdcS;       // screen dc
    HDC     hdcT;       // another work dc
    HDC     hdcM;       // another work dc
    RECT    rc;

    ASSERT(w <= cx);
    ASSERT(h <= cy);

    hdcS = GetDC(NULL);
    hdc  = CreateCompatibleDC(hdcS);
    hdcT = CreateCompatibleDC(hdcS);
    hdcM = CreateCompatibleDC(hdcS);

    hbm  = CreateCompatibleBitmap(hdcS, cx, cy);
    hbmM = CreateBitmap(cx,cy,1,1,NULL);
    hbmT = (HBITMAP)LoadImage(hInstance, MAKEINTRESOURCE(BMP_MONITOR2), IMAGE_BITMAP, w, h, 0);
    ReleaseDC(NULL,hdcS);

    SelectObject(hdc, hbm);
    SelectObject(hdcT,hbmT);
    SelectObject(hdcM,hbmM);

    //
    // fill bitmap with transparent color
    //
    SetBkColor(hdc,GetPixel(hdcT, 0, h-1));
    SetRect(&rc, 0, 0, cx, cy);
    ExtTextOut(hdc, 0, 0, ETO_OPAQUE, &rc, NULL, 0, NULL);

    //
    // copy bitmap to upper-left of bitmap
    //
    BitBlt(hdc,0,0,w,h,hdcT,0,0,SRCCOPY);

    //
    // make mask
    //
    BitBlt(hdcM,0,0,cx,cy,hdc,0,0,SRCCOPY);

    //
    // draw the monitor number in the bitmap (in the right place)
    //
    DrawMonitorNum(hdc, w, h, sz);

    DeleteDC(hdc);
    DeleteDC(hdcT);
    DeleteDC(hdcM);
    DeleteObject(hbmT);

    *pBitmap     = hbm;
    *pMaskBitmap = hbmM;
    return TRUE;
}

//
// SnapMonitorRect
//
// called while the user is moving a monitor window (WM_MOVING)
// if the CTRL key is not down we will snap the window rect
// to the edge of one of the other monitors.
//
// this is done so the user can easily align monitors
//
// NOTE pDevice->Snap must be initialized to 0,0 in WM_ENTERSIZEMOVE
//
void SnapMonitorRect(PMULTIMON_DEVICE pDevice, HWND hwnd, RECT *prc)
{
    HWND hwndT;
    int  d;
    RECT rcT;
    RECT rc;

    //
    // allow the user to move the window anywhere when the CTRL key is down
    //
    if (GetKeyState(VK_CONTROL) & 0x8000)
        return;

    //
    // macros to help in alignment
    //
    #define SNAP_DX 6
    #define SNAP_DY 6

    #define SNAPX(f,x) \
        d = rcT.x - rc.f; if (abs(d) <= SNAP_DX) rc.left+=d, rc.right+=d;

    #define SNAPY(f,y) \
        d = rcT.y - rc.f; if (abs(d) <= SNAP_DY) rc.top+=d, rc.bottom+=d;

    //
    // get current rect and offset it by the amount we have corrected
    // it so far (this alignes the rect with the position of the mouse)
    //
    rc = *prc;
    OffsetRect(&rc, pDevice->Snap.x, pDevice->Snap.y);

    //
    // walk all other windows and snap our window to them
    //
    for (hwndT = GetWindow(hwnd,  GW_HWNDFIRST); hwndT;
         hwndT = GetWindow(hwndT, GW_HWNDNEXT))
    {
        if (hwndT == hwnd)
            continue;

        GetWindowRect(hwndT, &rcT);
        InflateRect(&rcT,SNAP_DX,SNAP_DY);

        if (IntersectRect(&rcT, &rcT, &rc))
        {
            GetWindowRect(hwndT, &rcT);
            SNAPX(right,left);  SNAPY(bottom,top);
            SNAPX(right,right); SNAPY(bottom,bottom);
            SNAPX(left,left);   SNAPY(top,top);
            SNAPX(left,right);  SNAPY(top,bottom);
        }
    }

    //
    // adjust the amount we have snap'ed so far, and return the new rect
    //
    pDevice->Snap.x += prc->left - rc.left;
    pDevice->Snap.y += prc->top  - rc.top;
    *prc = rc;
}

LONG CALLBACK MonitorWindowProc(HWND hwnd, UINT msg,WPARAM wParam,LPARAM lParam)
{
    PAINTSTRUCT ps;
    HDC hdc;
    RECT rc;
    int w,h;
    TCHAR ach[80];
    PMULTIMON_DEVICE pDevice;
    HWND hDlg = NULL;
    RECT rcPos;
    CMultiMon * pcmm = (CMultiMon *) GetWindowLong(hwnd, 0);

    switch (msg)
    {
        case WM_CREATE:
            ASSERT(((LPCREATESTRUCT)lParam)->lpCreateParams);
            SetWindowLong(hwnd, 0, (LONG)((LPCREATESTRUCT)lParam)->lpCreateParams);
            break;

        case WM_NCCREATE:
            // turn off RTL_MIRRORED_WINDOW in GWL_EXSTYLE
            SHSetWindowBits(hwnd, GWL_EXSTYLE, RTL_MIRRORED_WINDOW, 0);
            break;

        case WM_NCHITTEST:
            pDevice = (PMULTIMON_DEVICE) GetDlgCtrlID(hwnd);
            if (pDevice && pDevice->pds->IsAttached())
                return HTCAPTION;
            break;
            
        case WM_NCLBUTTONDBLCLK:
            FlashText(NULL,NULL,FALSE);
            hDlg = GetParent(GetParent(hwnd));
            PostMessage(hDlg, WM_COMMAND, MAKEWPARAM(IDC_DISPLAYPROPERTIES, BN_CLICKED), (LPARAM)hwnd );
            break;

        case WM_COMMAND:
            switch (GET_WM_COMMAND_ID(wParam, lParam))
            {
            case IDC_DISPLAYPRIME:
            case IDC_DISPLAYUSEME:
            case IDC_DISPLAYPROPERTIES:
                hDlg = GetParent(GetParent(hwnd));
                PostMessage(hDlg, WM_COMMAND, wParam, lParam);
                break;

            case IDC_FLASH:
                pDevice = (PMULTIMON_DEVICE) GetDlgCtrlID(hwnd);
                pDevice->pds->GetOrigPosition(&rcPos);
                if (!IsRectEmpty(&rcPos))
                {
                    GetWindowText(hwnd, ach, ARRAYSIZE(ach));
                    FlashText(ach, &rcPos, FALSE);
                }
            }
            break;

        case WM_INITMENUPOPUP:
            pDevice = (PMULTIMON_DEVICE) GetDlgCtrlID(hwnd);

            CheckMenuItem((HMENU)wParam, IDC_DISPLAYUSEME, pDevice->pds->IsAttached() ? MF_CHECKED : MF_UNCHECKED);
            CheckMenuItem((HMENU)wParam, IDC_DISPLAYPRIME, pDevice->pds->IsPrimary()  ? MF_CHECKED : MF_UNCHECKED);

            EnableMenuItem((HMENU)wParam, IDC_FLASH,             pDevice->pds->IsAttached() ? MF_ENABLED : MF_GRAYED);
            EnableMenuItem((HMENU)wParam, IDC_DISPLAYPROPERTIES, pDevice->pds->IsAttached() ? MF_ENABLED : MF_GRAYED);
#ifndef WINNT
            EnableMenuItem((HMENU)wParam, IDC_DISPLAYPRIME, MF_GRAYED);
            EnableMenuItem((HMENU)wParam, IDC_DISPLAYUSEME, pDevice->pds->IsPrimary() ? MF_GRAYED : MF_ENABLED);
#endif
            SetMenuDefaultItem((HMENU)wParam, IDC_DISPLAYPROPERTIES, MF_BYCOMMAND);
            break;

        case WM_RBUTTONDOWN:
        case WM_NCRBUTTONDOWN:
            hDlg = GetParent(GetParent(hwnd));
            pDevice = (PMULTIMON_DEVICE) GetDlgCtrlID(hwnd);

            if (pDevice && pcmm)
            {
                HMENU hmenu;
                hmenu = LoadMenu(hInstance, MAKEINTRESOURCE(MENU_MONITOR));

                if (hmenu)
                {
                    POINT pt;
                    pcmm->UpdateActiveDisplay(pDevice);
                    GetCursorPos(&pt);
                    TrackPopupMenu(GetSubMenu(hmenu,0), TPM_RIGHTBUTTON,
                        pt.x, pt.y, 0, hwnd, NULL);
                    DestroyMenu(hmenu);
                }
            }
            break;

        case WM_NCLBUTTONDOWN:
            BringWindowToTop(hwnd);
            hDlg = GetParent(GetParent(hwnd));
            pDevice = (PMULTIMON_DEVICE) GetDlgCtrlID(hwnd);
            if (pDevice->pds->IsAttached())
            {
                if (pcmm)
                    pcmm->UpdateActiveDisplay(pDevice);
                pDevice->pds->GetOrigPosition(&rcPos);
                if (!IsRectEmpty(&rcPos))
                {
                    GetWindowText(hwnd, ach, ARRAYSIZE(ach));
                    FlashText(ach, &rcPos, TRUE);
                }
            }
            break;

        case WM_LBUTTONUP:
            pDevice = (PMULTIMON_DEVICE) GetDlgCtrlID(hwnd);
            if (pcmm && !pcmm->QueryNoAttach() && pDevice && !pDevice->pds->IsAttached())
            {
                TCHAR szTurnItOn[400];
                TCHAR szTurnOnTitleFormat[30];
                TCHAR szTurnOnTitle[110];
                
                LoadString(hInstance, IDS_TURNONTITLE, szTurnOnTitleFormat, SIZEOF(szTurnOnTitleFormat));
                GetWindowText(hwnd, ach, SIZEOF(ach));
                wsprintf(szTurnOnTitle, szTurnOnTitleFormat, ach);


                LPTSTR pstr = szTurnItOn;
                if (pcmm->GetNumberOfAttachedDisplays() <= 1)
                {
                    LoadString(hInstance, IDS_TURNONWARN, szTurnItOn, SIZEOF(szTurnItOn));
                    pstr += lstrlen(szTurnItOn);
                }
                
                LoadString(hInstance, IDS_TURNITON, pstr, SIZEOF(szTurnItOn));
                hDlg = GetParent(GetParent(hwnd));
                if (ShellMessageBox(hInstance, hwnd, szTurnItOn, szTurnOnTitle, MB_YESNO | MB_ICONINFORMATION) == IDYES)
                {
                    if (pcmm->ToggleAttached(pDevice, FALSE))
                    {
                        pcmm->SetDirty();
                        pcmm->UpdateActiveDisplay(pDevice);
                    }
                }
            }
            break;

        case WM_ENTERSIZEMOVE:
            pDevice = (PMULTIMON_DEVICE) GetDlgCtrlID(hwnd);
            pDevice->Snap.x = 0;
            pDevice->Snap.y = 0;
            FlashText(NULL,NULL,FALSE);
            break;

        case WM_MOVING:
            pDevice = (PMULTIMON_DEVICE) GetDlgCtrlID(hwnd);
            SnapMonitorRect(pDevice, hwnd, (RECT*)lParam);
            break;

        case WM_EXITSIZEMOVE:
            if (pcmm)
                pcmm->HandleMonitorChange(hwnd, FALSE);
            RedrawWindow(GetParent(hwnd), NULL, NULL, RDW_ERASE | RDW_INVALIDATE | RDW_ALLCHILDREN);
            break;

        case WM_DESTROY:
            SetWindowLong(hwnd, 0, NULL);
            break;

        case WM_ERASEBKGND:
            //GetClientRect(hwnd, &rc);
            //FillRect((HDC)wParam, &rc, GetSysColorBrush(COLOR_APPWORKSPACE));
            return 0L;

        case WM_PAINT:
            hdc = BeginPaint(hwnd,&ps);
            GetWindowText(hwnd, ach, ARRAYSIZE(ach));
            GetClientRect(hwnd, &rc);
            w = rc.right;
            h = rc.bottom;

            pDevice = (PMULTIMON_DEVICE) GetDlgCtrlID(hwnd);

            if (pDevice->w != w || pDevice->h != h)
            {
                HBITMAP hbm, hbmMask;
                int cx,cy;

                pDevice->w = w;
                pDevice->h = h;

                ImageList_GetIconSize(pDevice->himl, &cx, &cy);
                MakeMonitorBitmap(w,h,ach,&hbm,&hbmMask,cx,cy);
                ImageList_Replace(pDevice->himl,pDevice->iImage,hbm,hbmMask);

                DeleteObject(hbm);
                DeleteObject(hbmMask);
            }

            if (!pDevice->pds->IsAttached())
            {
                FillRect(hdc, &rc, GetSysColorBrush(COLOR_APPWORKSPACE));
                ImageList_DrawEx(pDevice->himl,pDevice->iImage,hdc,0,0,w,h,
                    CLR_DEFAULT,CLR_NONE,ILD_BLEND50);
            }
            else if (pcmm && (pDevice == pcmm->GetCurDevice()))
            {
                ImageList_DrawEx(pDevice->himl,pDevice->iImage,hdc,0,0,w,h,
                    CLR_DEFAULT,CLR_DEFAULT,ILD_BLEND50);
                DrawMonitorNum(hdc, w, h, ach, FALSE);
            }
            else
            {
                ImageList_DrawEx(pDevice->himl,pDevice->iImage,hdc,0,0,w,h,
                    CLR_DEFAULT,CLR_DEFAULT,ILD_NORMAL);
            }

            EndPaint(hwnd,&ps);
            return 0L;
    }
    return DefWindowProc(hwnd,msg,wParam,lParam);
}
