/*******************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 1993-1994
*
*  TITLE:       VOLUME.C
*
*  VERSION:     1.0
*
*  AUTHOR:      RAL
*
*  DATE:        11/01/94
*
********************************************************************************
*
*  CHANGE LOG:
*
*  DATE        REV DESCRIPTION
*  ----------- --- -------------------------------------------------------------
*  Nov. 11, 94 RAL Original
*  Oct. 24, 95 Shawnb UNICODE enabled
*
*******************************************************************************/
#include "stdafx.h"
#include "systray.h"

#include <objbase.h>
#include <setupapi.h>
#include <cfgmgr32.h>
#include <initguid.h>
#include <devguid.h>

#include <ks.h>
#include <ksmedia.h>
#include <mmddkp.h>


/* defined in mmddk.h */
#define DRV_QUERYDEVNODE     (DRV_RESERVED + 2)

#define VOLUMEMENU_PROPERTIES               100
#define VOLUMEMENU_SNDVOL                   101

extern HINSTANCE g_hInstance;

static BOOL    g_bVolumeEnabled = FALSE;
static BOOL    g_bVolumeIconShown = FALSE;
static HICON   g_hVolumeIcon = NULL;
static HICON   g_hMuteIcon = NULL;
static HMENU   g_hVolumeMenu = NULL;
static HMIXER  g_hMixer = NULL;
static UINT    g_uMixer = 0;
static DWORD   g_dwMixerDevNode = 0;
static DWORD   g_dwMute = (DWORD) -1;
static DWORD   g_dwVSlider = 0;
static DWORD   g_dwMasterLine = (DWORD) -1;

HDEVNOTIFY DeviceEventContext = NULL;


void Volume_DeviceChange_Init(HWND hWnd, DWORD dwMixerID);
void Volume_DeviceChange_Cleanup(void);

void Volume_UpdateStatus(HWND hWnd, BOOL bShowIcon, BOOL bKillSndVol32);
void Volume_VolumeControl();
void Volume_ControlPanel(HWND hwnd);
MMRESULT Volume_GetDefaultMixerID(int *pid);
void Volume_UpdateIcon(HWND hwnd, DWORD message);
BOOL Volume_Controls(UINT uMxID);
BOOL FileExists (LPCTSTR pszFileName);
BOOL FindSystemFile (LPCTSTR pszFileName, LPTSTR pszFullPath, UINT cchSize);
void Volume_WakeUpOrClose(BOOL fClose);

HMENU Volume_CreateMenu()
{
        HMENU  hmenu;
        LPTSTR lpszMenu1;
        LPTSTR lpszMenu2;

        lpszMenu1 = LoadDynamicString(IDS_VOLUMEMENU1);
        if (!lpszMenu1)
                return NULL;

        lpszMenu2 = LoadDynamicString(IDS_VOLUMEMENU2);
        if (!lpszMenu2)
        {
                DeleteDynamicString(lpszMenu1);
                return NULL;
        }

        hmenu = CreatePopupMenu();
        if (!hmenu)
        {
                DeleteDynamicString(lpszMenu1);
                DeleteDynamicString(lpszMenu2);
                return NULL;
        }

        AppendMenu(hmenu,MF_STRING,VOLUMEMENU_SNDVOL,lpszMenu2);
        AppendMenu(hmenu,MF_STRING,VOLUMEMENU_PROPERTIES,lpszMenu1);

        SetMenuDefaultItem(hmenu,VOLUMEMENU_SNDVOL,FALSE);

        DeleteDynamicString(lpszMenu1);
        DeleteDynamicString(lpszMenu2);

        return hmenu;
}





BOOL Volume_Init(HWND hWnd)
{
        UINT        uMxID;
        const TCHAR szVolApp[] = TEXT ("SNDVOL32.EXE");

        if (g_hMixer == NULL)
        {
                if (Volume_GetDefaultMixerID(&uMxID) != MMSYSERR_NOERROR)
                        return FALSE;

                //
                // check for sndvol32 existence.  checking for the .exe
                // first will ensure that the service gets disabled properly
                //
                
                if (! FindSystemFile (szVolApp, NULL, 0))
                {
                        //
                        // disable the volume service
                        //
                        EnableService (STSERVICE_VOLUME, FALSE);
                
                        return FALSE;
                }


                //
                // do we have output volume controls on this mixer?
                //
                if (! Volume_Controls(uMxID))
                        return FALSE;

                if (mixerOpen(&g_hMixer, uMxID, (DWORD_PTR)hWnd, 0
                                , CALLBACK_WINDOW | MIXER_OBJECTF_MIXER)
                        == MMSYSERR_NOERROR)
                {
            Volume_DeviceChange_Init(hWnd, uMxID);

                        g_uMixer = uMxID;
                        if (mixerMessage ((HMIXER)uMxID, DRV_QUERYDEVNODE
                                 , (DWORD_PTR)&g_dwMixerDevNode, 0L))
                                g_dwMixerDevNode = 0L;
                        return TRUE;
                }
        }
        else
                return TRUE;

        return FALSE;
}

//
//  Called at init time and whenever services are enabled/disabled.
//  Returns false if mixer services are not active.
//
BOOL Volume_CheckEnable(HWND hWnd, BOOL bSvcEnabled)
{
        BOOL bEnable = bSvcEnabled && Volume_Init(hWnd);

        if (bEnable != g_bVolumeEnabled) {
                //
                // state change
                //
                g_bVolumeEnabled = bEnable;
                Volume_UpdateStatus(hWnd, bEnable, TRUE);
        }
        return(bEnable);
}

void Volume_UpdateStatus(HWND hWnd, BOOL bShowIcon, BOOL bKillSndVol32)
{
        if (bShowIcon != g_bVolumeIconShown) {
                g_bVolumeIconShown = bShowIcon;
                if (bShowIcon) {
                        g_hVolumeIcon = LoadImage(g_hInstance, MAKEINTRESOURCE(IDI_VOLUME),
                                                IMAGE_ICON, 16, 16, 0);
                        g_hMuteIcon = LoadImage(g_hInstance, MAKEINTRESOURCE(IDI_MUTE),
                                                IMAGE_ICON, 16, 16, 0);
                        Volume_UpdateIcon(hWnd, NIM_ADD);
                } else {
                        SysTray_NotifyIcon(hWnd, STWM_NOTIFYVOLUME, NIM_DELETE, NULL, NULL);
                        if (g_hVolumeIcon) {
                                DestroyIcon(g_hVolumeIcon);
                                g_hVolumeIcon = NULL;
                        }
                        if (g_hMuteIcon) {
                                DestroyIcon(g_hMuteIcon);
                                g_hMuteIcon = NULL;
                        }
                        if (g_hMixer)
                        {
                                mixerClose(g_hMixer);
                                g_hMixer = NULL;
                        }
                        g_uMixer = 0;
                        g_dwMixerDevNode = 0L;

                        //
                        // SNDVOL32 may have a TRAYMASTER window open,
                        // sitting on a timer before it closes (so multiple
                        // l-clicks on the tray icon can bring up the app
                        // quickly after the first hit).  Close that app
                        // if it's around.
                        //
                        if (bKillSndVol32)
                        {
                                Volume_WakeUpOrClose (TRUE);
                        }
                }
    }
}

const TCHAR szMapperPath[]      = TEXT ("Software\\Microsoft\\Multimedia\\Sound Mapper");
const TCHAR szPlayback[]        = TEXT ("Playback");
const TCHAR szPreferredOnly[]   = TEXT ("PreferredOnly");


/*
 * Volume_GetDefaultMixerID
 *
 * Get the default mixer id.  We only appear if there is a mixer associated
 * with the default wave.
 *
 */
MMRESULT Volume_GetDefaultMixerID(int *pid)
{
    MMRESULT        mmr;
    DWORD           dwWaveID;
    DWORD           dwMixID;
    DWORD           dwFlags = 0;
    
    mmr = waveOutMessage((HWAVEOUT)WAVE_MAPPER, DRVM_MAPPER_PREFERRED_GET, (DWORD_PTR) &dwWaveID, (DWORD_PTR) &dwFlags);

    if (mmr == MMSYSERR_NOERROR)
    {
        mmr = mixerGetID((HMIXEROBJ) dwWaveID, &dwMixID, MIXER_OBJECTF_WAVEOUT);

                if (mmr == MMSYSERR_NOERROR && pid)
                {
                        *pid = dwMixID;
                }
    }

    return mmr;
}
        

/*
 * Process line changes
 */
void Volume_LineChange(
    HWND        hwnd,
    HMIXER      hmx,
    DWORD       dwLineID)
{
    if (dwLineID != g_dwMasterLine)
                return;
    //
    // if our line is disabled, go away, I guess
    //
}

/*
 * Process control changes
 */
void Volume_ControlChange(
    HWND        hwnd,
    HMIXER      hmx,
    DWORD       dwControlID)
{
    if ((dwControlID != g_dwMute) && (g_dwMute != (DWORD) -1))
                return;

    //
    // Change mute icon state
    //
    Volume_UpdateIcon(hwnd, NIM_MODIFY);
}


BOOL Volume_IsMute()
{
    MMRESULT            mmr;
    MIXERCONTROLDETAILS mxcd;
    BOOL                fMute;

    if (!g_hMixer && (g_dwMute != (DWORD) -1))
    {
                return FALSE;
    }

    mxcd.cbStruct       = sizeof(mxcd);
    mxcd.dwControlID    = g_dwMute;
    mxcd.cChannels      = 1;
    mxcd.cMultipleItems = 0;
    mxcd.cbDetails      = sizeof(DWORD);
    mxcd.paDetails      = (LPVOID)&fMute;

    mmr = mixerGetControlDetails( (HMIXEROBJ)g_hMixer, &mxcd, MIXER_GETCONTROLDETAILSF_VALUE);

    if (mmr == MMSYSERR_NOERROR)
    {
                return fMute;
    }

    return FALSE;
}

BOOL Volume_Controls(
    UINT                uMxID)
{
    MIXERLINECONTROLS   mxlc;
    MIXERCONTROL        mxctrl;
    MIXERCAPS           mxcaps;
    MMRESULT            mmr;
    BOOL                fResult = FALSE;
    DWORD               iDest;
    g_dwMasterLine      = (DWORD) -1;
    g_dwMute            = (DWORD) -1;

    mmr = mixerGetDevCaps(uMxID, &mxcaps, sizeof(mxcaps));

    if (mmr != MMSYSERR_NOERROR)
    {
                return FALSE;
    }

    for (iDest = 0; iDest < mxcaps.cDestinations; iDest++)
    {
                MIXERLINE       mlDst;
        
                mlDst.cbStruct      = sizeof ( mlDst );
                mlDst.dwDestination = iDest;
        
                mmr = mixerGetLineInfo( (HMIXEROBJ)uMxID, &mlDst, MIXER_GETLINEINFOF_DESTINATION);

                if (mmr != MMSYSERR_NOERROR)
        {
                        continue;
        }

                switch (mlDst.dwComponentType)
                {
                    default:
                    continue;
                    
                case MIXERLINE_COMPONENTTYPE_DST_SPEAKERS:
                case MIXERLINE_COMPONENTTYPE_DST_HEADPHONES:
            {
                            g_dwMasterLine = mlDst.dwLineID;
            }
                        break;
                }
        
                mxlc.cbStruct       = sizeof(mxlc);
                mxlc.dwLineID       = g_dwMasterLine;
                mxlc.dwControlType  = MIXERCONTROL_CONTROLTYPE_MUTE;
                mxlc.cControls      = 1;
                mxlc.cbmxctrl       = sizeof(mxctrl);
                mxlc.pamxctrl       = &mxctrl;
                
                mmr = mixerGetLineControls( (HMIXEROBJ) uMxID, &mxlc, MIXER_GETLINECONTROLSF_ONEBYTYPE);

                if (mmr == MMSYSERR_NOERROR)
        {
                        g_dwMute = mxctrl.dwControlID;
        }
        
                fResult = TRUE;
                break;
        
    }
    return fResult;
}

void Volume_UpdateIcon(
    HWND hWnd,
    DWORD message)
{
    BOOL        fMute;
    LPTSTR      lpsz;
    HICON       hVol;

    fMute   = Volume_IsMute();
    hVol    = fMute?g_hMuteIcon:g_hVolumeIcon;
    lpsz    = LoadDynamicString(fMute?IDS_MUTED:IDS_VOLUME);
    SysTray_NotifyIcon(hWnd, STWM_NOTIFYVOLUME, message, hVol, lpsz);
    DeleteDynamicString(lpsz);
}



// WinMM is telling us the preferred device has changed for some reason
// Dump the old, open the new
//
void Volume_WinMMDeviceChange(HWND hWnd)
{
    DWORD dwMixID;

        if (g_hMixer)               // Dumping the Old
        {
                mixerClose(g_hMixer);
                g_hMixer = NULL;
                g_uMixer = 0;
                g_dwMixerDevNode = 0L;
        }
                                // Opening the new
    if (Volume_GetDefaultMixerID(&dwMixID) == MMSYSERR_NOERROR)
    {   
                if ( Volume_Controls(dwMixID) && 
             (mixerOpen(&g_hMixer, dwMixID, (DWORD_PTR)hWnd, 0L, CALLBACK_WINDOW | MIXER_OBJECTF_MIXER) == MMSYSERR_NOERROR))
                {
                        Volume_UpdateStatus(hWnd, g_bVolumeIconShown, TRUE);

                        if (mixerMessage ((HMIXER)dwMixID, DRV_QUERYDEVNODE, (DWORD_PTR)&g_dwMixerDevNode, 0L))
            {
                                g_dwMixerDevNode = 0L;
            }

                        g_uMixer = dwMixID;

            Volume_UpdateIcon(hWnd, NIM_MODIFY);
                }
                else
                {
                        Volume_UpdateStatus(hWnd, FALSE, TRUE);
                }
    }
    else
    {
                Volume_UpdateStatus(hWnd, FALSE, TRUE);
    }
}


// Need to free up in the event of a power broadcast as well
//
void Volume_HandlePowerBroadcast(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
    switch (wParam)
    {
            case PBT_APMQUERYSUSPEND:
        {
                if (g_hMixer)               // Dumping the Old
                {
                        mixerClose(g_hMixer);
                        g_hMixer = NULL;
                        g_uMixer = 0;
                        g_dwMixerDevNode = 0L;
                }
        }
            break;

            case PBT_APMQUERYSUSPENDFAILED:
            case PBT_APMRESUMESUSPEND:
        {
            Volume_WinMMDeviceChange(hWnd); 
        }
            break;
    }
}


void Volume_DeviceChange_Cleanup()
{
   if (DeviceEventContext) 
   {
       UnregisterDeviceNotification(DeviceEventContext);
       DeviceEventContext = 0;
   }

   return;
}

/*
**************************************************************************************************
        Volume_GetDeviceHandle()

        given a mixerID this functions opens its corresponding device handle. This handle can be used 
        to register for DeviceNotifications.

        dwMixerID -- The mixer ID
        phDevice -- a pointer to a handle. This pointer will hold the handle value if the function is
                                successful
        
        return values -- If the handle could be obtained successfully the return vlaue is TRUE.

**************************************************************************************************
*/
BOOL Volume_GetDeviceHandle(DWORD dwMixerID, HANDLE *phDevice)
{
        MMRESULT mmr;
        ULONG cbSize=0;
        TCHAR *szInterfaceName=NULL;

        //Query for the Device interface name
        mmr = mixerMessage((HMIXER)dwMixerID, DRV_QUERYDEVICEINTERFACESIZE, (DWORD_PTR)&cbSize, 0L);
        if(MMSYSERR_NOERROR == mmr)
        {
                szInterfaceName = (TCHAR *)GlobalAllocPtr(GHND, (cbSize+1)*sizeof(TCHAR));
                if(!szInterfaceName)
                {
                        return FALSE;
                }

                mmr = mixerMessage((HMIXER)dwMixerID, DRV_QUERYDEVICEINTERFACE, (DWORD_PTR)szInterfaceName, cbSize);
                if(MMSYSERR_NOERROR != mmr)
                {
                        GlobalFreePtr(szInterfaceName);
                        return FALSE;
                }
        }
        else
        {
                return FALSE;
        }

        //Get an handle on the device interface name.
        *phDevice = CreateFile(szInterfaceName, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
                                                 NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

        GlobalFreePtr(szInterfaceName);
        if(INVALID_HANDLE_VALUE == *phDevice)
        {
                return FALSE;
        }

        return TRUE;
}


/*      DeviceChange_Init()
*       First time initialization for WM_DEVICECHANGE messages
*       
*       On NT 5.0, you have to register for device notification
*/
void Volume_DeviceChange_Init(HWND hWnd, DWORD dwMixerID)
{
        DEV_BROADCAST_HANDLE DevBrodHandle;
        HANDLE hMixerDevice=NULL;


        //If we had registered already for device notifications, unregister ourselves.
        Volume_DeviceChange_Cleanup();

        //If we get the device handle register for device notifications on it.
        if(Volume_GetDeviceHandle(dwMixerID, &hMixerDevice))
        {
                memset(&DevBrodHandle, 0, sizeof(DEV_BROADCAST_HANDLE));

                DevBrodHandle.dbch_size = sizeof(DEV_BROADCAST_HANDLE);
                DevBrodHandle.dbch_devicetype = DBT_DEVTYP_HANDLE;
                DevBrodHandle.dbch_handle = hMixerDevice;

                DeviceEventContext = RegisterDeviceNotification(hWnd, &DevBrodHandle, DEVICE_NOTIFY_WINDOW_HANDLE);

                if(hMixerDevice)
                {
                        CloseHandle(hMixerDevice);
                        hMixerDevice = NULL;
                }
    }
}

// Watch for PNP events to free up the open handle when needed
// We will assume any changes will now generate a WINMM_DEVICECHANGED message from WinMM
// except for the QUERYREMOVEFAILED case, in this case we will just re-aquire the preferred mixer
//
void Volume_DeviceChange(HWND hWnd,WPARAM wParam,LPARAM lParam)
{
    PDEV_BROADCAST_HANDLE bh = (PDEV_BROADCAST_HANDLE)lParam;
        
    //If we have an handle on the device then we get a DEV_BROADCAST_HDR structure as the lParam.

    if(!DeviceEventContext || !bh || (bh->dbch_devicetype != DBT_DEVTYP_HANDLE))
    {
        return;
    }
        
    switch (wParam)
    {
        case DBT_DEVICEQUERYREMOVE:             // Someone wants to remove this device, let's let them.
        {
                if (g_hMixer)
                    {
                            mixerClose(g_hMixer);
                            g_hMixer = NULL;
                            g_uMixer = 0;
                            g_dwMixerDevNode = 0L;
                    }
                }
            break;

            case DBT_DEVICEQUERYREMOVEFAILED:       // The query failed, the device will not be removed, so lets reopen it.
        {
            Volume_WinMMDeviceChange(hWnd);     // Lets just use this function to do it.
        }
            break;
    }
}

void Volume_WmDestroy(
   HWND hDlg
   )
{
    Volume_DeviceChange_Cleanup();
}

void Volume_Shutdown(
    HWND hWnd)
{
    Volume_UpdateStatus(hWnd, FALSE, FALSE);
}

void Volume_Menu(HWND hwnd, UINT uMenuNum, UINT uButton)
{
    POINT   pt;
    UINT    iCmd;
    HMENU   hmenu;

    GetCursorPos(&pt);

    hmenu = Volume_CreateMenu();
    if (!hmenu)
                return;

    SetForegroundWindow(hwnd);
    iCmd = TrackPopupMenu(hmenu, uButton | TPM_RETURNCMD | TPM_NONOTIFY,
        pt.x, pt.y, 0, hwnd, NULL);

    DestroyMenu(hmenu);
    switch (iCmd) {
        case VOLUMEMENU_PROPERTIES:
            Volume_ControlPanel(hwnd);
            break;

        case VOLUMEMENU_SNDVOL:
            Volume_VolumeControl();
            break;
    }

    SetIconFocus(hwnd, STWM_NOTIFYVOLUME);

}

void Volume_Notify(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    switch (lParam)
    {
        case WM_RBUTTONUP:
            Volume_Menu(hwnd, 1, TPM_RIGHTBUTTON);
            break;

        case WM_LBUTTONDOWN:
            SetTimer(hwnd, VOLUME_TIMER_ID, GetDoubleClickTime()+100, NULL);
            break;

        case WM_LBUTTONDBLCLK:
            KillTimer(hwnd, VOLUME_TIMER_ID);
            Volume_VolumeControl();
            break;
    }
}


/* WARNING - WARNING - DANGER - DANGER - WARNING - WARNING - DANGER - DANGER */
/* WARNING - WARNING - DANGER - DANGER - WARNING - WARNING - DANGER - DANGER */
/* WARNING - WARNING - DANGER - DANGER - WARNING - WARNING - DANGER - DANGER */
/*
 * MYWM_WAKEUP and the "Tray Volume" window are defined by the SNDVOL32.EXE
 * application.  Changing these values or changing the values in SNDVOL32.EXE
 * without mirroring them here will break the tray volume dialog.
 */
/* WARNING - WARNING - DANGER - DANGER - WARNING - WARNING - DANGER - DANGER */
/* WARNING - WARNING - DANGER - DANGER - WARNING - WARNING - DANGER - DANGER */
/* WARNING - WARNING - DANGER - DANGER - WARNING - WARNING - DANGER - DANGER */

#define MYWM_WAKEUP             (WM_APP+100+6)

void Volume_Timer(HWND hwnd)
{
        KillTimer(hwnd, VOLUME_TIMER_ID);

        Volume_WakeUpOrClose (FALSE);
}

void Volume_WakeUpOrClose(BOOL fClose)
{
        const TCHAR szVolWindow [] = TEXT ("Tray Volume");
        HWND hApp;

        if (hApp = FindWindow(szVolWindow, NULL))
        {
                SendMessage(hApp, MYWM_WAKEUP, (WPARAM)fClose, 0);
        }
        else if (!fClose)
        {
                const TCHAR szOpen[]    = TEXT ("open");
                const TCHAR szVolApp[]  = TEXT ("SNDVOL32.EXE");
                const TCHAR szParamsWakeup[]  = TEXT ("/t");

                ShellExecute (NULL, szOpen, szVolApp, szParamsWakeup, NULL, SW_SHOWNORMAL);
        }
}


/*
 * Volume_ControlPanel
 *
 * Launch "Audio" control panel/property sheet upon request.
 *
 * */
void Volume_ControlPanel(HWND hwnd)
{
        const TCHAR szOpen[]    = TEXT ("open");
        const TCHAR szRunDLL[]  = TEXT ("RUNDLL32.EXE");
        const TCHAR szParams[]  = TEXT ("MMSYS.CPL,ShowFullControlPanel");

        ShellExecute(NULL, szOpen, szRunDLL, szParams, NULL, SW_SHOWNORMAL);
}

/*
 * Volume_VolumeControl
 *
 * Launch Volume Control App
 *
 * */
void Volume_VolumeControl()
{
        const TCHAR szOpen[]    = TEXT ("open");
        const TCHAR szVolApp[]  = TEXT ("SNDVOL32.EXE");

        ShellExecute(NULL, szOpen, szVolApp, NULL, NULL, SW_SHOWNORMAL);
}



/*
 * FileExists
 *
 * Does a file exist
 *
 * */

BOOL FileExists(LPCTSTR pszPath)
{
        return (GetFileAttributes(pszPath) != (DWORD)-1);
} // End FileExists


/*
 * FindSystemFile
 *
 * Finds full path to specified file
 *
 * */

BOOL FindSystemFile(LPCTSTR pszFileName, LPTSTR pszFullPath, UINT cchSize)
{
        TCHAR       szPath[MAX_PATH];
        LPTSTR      pszName;
        DWORD       cchLen;

        if ((pszFileName == NULL) || (pszFileName[0] == 0))
                return FALSE;

        cchLen = SearchPath(NULL, pszFileName, NULL, MAX_PATH,
                                                szPath,&pszName);
        if (cchLen == 0)
                return FALSE;
        
        if (cchLen >= MAX_PATH)
                cchLen = MAX_PATH - 1;

        if (! FileExists (szPath))
                return FALSE;

        if ((pszFullPath == NULL) || (cchSize == 0))
                return TRUE;

           // Copy full path into buffer
        if (cchLen >= cchSize)
                cchLen = cchSize - 1;
        
        lstrcpyn (pszFullPath, szPath, cchLen);
        
        pszFullPath[cchLen] = 0;

        return TRUE;
} // End FindSystemFile
