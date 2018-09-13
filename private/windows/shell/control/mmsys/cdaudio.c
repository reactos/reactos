/*
 ***************************************************************
 *  sprop.c
 *
 *  Copyright (C) Microsoft, 1990, All Rights Reserved.
 *
 *  Displays the Simple media properties
 *
 *  History:
 *
 *  July 1994 -by- VijR (Created)
 *        
 ***************************************************************
 */

#include "mmcpl.h"
#include <windowsx.h>
#ifdef DEBUG
#undef DEBUG
#include <mmsystem.h>
#define DEBUG
#else
#include <mmsystem.h>
#endif
#include <mmddk.h>
#include <mmreg.h>
#include <commctrl.h>
#include <prsht.h>
#include <regstr.h>
#include "utils.h"
#include "medhelp.h"
#include "mmcpl.h"

#include <winerror.h>

//for digital cd audio
#include "devguid.h"
#include "setupapi.h"
#include "cfgmgr32.h"
#include "winioctl.h"
#include "tchar.h"

const TCHAR gszCreateCDFile[] = TEXT("\\\\.\\%c:");

#define MYREGSTR_PATH_MEDIA  TEXT("SYSTEM\\CurrentControlSet\\Control\\MediaResources") 
const TCHAR gszRegstrCDAPath[] = MYREGSTR_PATH_MEDIA TEXT("\\mci\\cdaudio");
const TCHAR gszUnitEnum[] = TEXT("%s\\unit %d");
const TCHAR gszSettingsKey[] = TEXT("Volume Settings");
const TCHAR gszDefaultCDA[] = TEXT("Default Drive");

const TCHAR gszRegstrCDROMPath[] = TEXT("System\\CurrentControlSet\\Services\\Class\\CDROM\\");
const TCHAR gszDigitalPlay[] = TEXT("DigitalAudioPlay");

const TCHAR gszRegstrDrivePath[] = TEXT("Enum\\SCSI");

#define CDA_VT_UNSET 0
#define CDA_VT_AUX  1
#define CDA_VT_MIX  2


#define CDROM_DIGITAL_PLAY_ENABLED      0x01
#define CDROM_DIGITAL_PLAY_CAPABLE      0x02
#define CDROM_DIGITAL_DEVICE_KNOWN      0x04


typedef struct {                // This struct is used in other places DO NOT CHANGE
    DWORD   unit;
    DWORD   dwVol;
} CDAREG, *PCDAREG;

typedef struct {                // This is now the local version with addition data, this can change, but has to keep the
    CDAREG              cdar;
    BOOLEAN             fDigFlags;
    BOOLEAN             DigitalEnabled;
    BOOLEAN             DigitalKnownDevice;
    BOOLEAN             oldDigEnabled;
    DWORD               dwOldVol;
    TCHAR               chDrive;
    HDEVINFO            hDevInfo;
    PSP_DEVINFO_DATA    pDevInfoData;
} CDSTATE, *PCDSTATE;


BOOL g_fWDMEnabled = FALSE;
HMODULE g_hModStorProp = NULL;

typedef LONG (WINAPI *CDROMISDIGITALPLAYBACKENABLEDPROC)(HDEVINFO,PSP_DEVINFO_DATA,BOOLEAN*);
typedef BOOL (WINAPI *CDROMKNOWNGOODDIGITALPLAYBACKPROC)(HDEVINFO,PSP_DEVINFO_DATA);
typedef LONG (WINAPI *CDROMENABLEDIGITALPLAYBACKPROC)(HDEVINFO,PSP_DEVINFO_DATA,BOOLEAN);
typedef LONG (WINAPI *CDROMDISABLEDIGITALPLAYBACKPROC)(HDEVINFO,PSP_DEVINFO_DATA);

CDROMISDIGITALPLAYBACKENABLEDPROC   _gCdromIsDigitalPlaybackEnabled = NULL;
CDROMKNOWNGOODDIGITALPLAYBACKPROC   _gCdromKnownGoodDigitalPlayback = NULL;
CDROMENABLEDIGITALPLAYBACKPROC      _gCdromEnableDigitalPlayback = NULL;
CDROMDISABLEDIGITALPLAYBACKPROC     _gCdromDisableDigitalPlayback = NULL;

void GetPrefInfo(PAUDIODLGINFO pai, HWND hDlg );

HANDLE GetHandleForDevice(LPCTSTR DeviceName)
{
    int i = 0;
    TCHAR fakeDeviceName[MAX_PATH];
    HANDLE h = INVALID_HANDLE_VALUE;
    BOOL success = FALSE;
    TCHAR buf[MAX_PATH];
    
    while (!success && i < 10)
    {
        wsprintf(buf, TEXT("DISK_FAKE_DEVICE_%d_"), i++);
        success = DefineDosDevice(DDD_RAW_TARGET_PATH,
                                  buf, 
                                  DeviceName);
        if (success)
        {
            _tcscpy(fakeDeviceName, TEXT("\\\\.\\"));
            _tcscat(fakeDeviceName, buf);
            h = CreateFile(fakeDeviceName,
                            GENERIC_WRITE | GENERIC_READ,
                            FILE_SHARE_WRITE | FILE_SHARE_READ,
                            NULL,
                            OPEN_EXISTING,
                            0,
                            NULL);
            DefineDosDevice(DDD_REMOVE_DEFINITION,
                            buf,
                            NULL);
        }
    } //end while

    return h;
}

HANDLE GetHandleForDeviceInst(DEVINST DevInst)
{
    TCHAR DeviceName[MAX_PATH];
    CONFIGRET cr;
    DWORD len = MAX_PATH;

    cr = CM_Get_DevNode_Registry_Property(DevInst,
                                          CM_DRP_PHYSICAL_DEVICE_OBJECT_NAME,
                                          NULL,
                                          DeviceName,
                                          &len,
                                          0);

    if (cr != CR_SUCCESS)
    {
        return 0;
    }
       
    return GetHandleForDevice(DeviceName);
} 

BOOL EnableCdromFunctions(HMODULE* pMod)
{
    BOOL fRet = FALSE;
    
    if (!*pMod)
    {
        *pMod = LoadLibrary(TEXT("STORPROP.DLL"));
    }

    if (*pMod)
    {
        _gCdromIsDigitalPlaybackEnabled = (CDROMISDIGITALPLAYBACKENABLEDPROC)GetProcAddress(*pMod,"CdromIsDigitalPlaybackEnabled");
        _gCdromKnownGoodDigitalPlayback = (CDROMKNOWNGOODDIGITALPLAYBACKPROC)GetProcAddress(*pMod,"CdromKnownGoodDigitalPlayback");
        _gCdromEnableDigitalPlayback = (CDROMENABLEDIGITALPLAYBACKPROC)GetProcAddress(*pMod,"CdromEnableDigitalPlayback");
        _gCdromDisableDigitalPlayback = (CDROMDISABLEDIGITALPLAYBACKPROC)GetProcAddress(*pMod,"CdromDisableDigitalPlayback");

        if (
            (_gCdromIsDigitalPlaybackEnabled)
            &&
            (_gCdromKnownGoodDigitalPlayback)
            &&
            (_gCdromEnableDigitalPlayback)
            &&
            (_gCdromDisableDigitalPlayback)
        )
        {
            fRet = TRUE;
        }
    }

    return fRet;
}

BYTE CDAudio_GetSetDigitalFlags(PCDSTATE pcds, BYTE fSetFlags, BOOL fSet)
{
    BYTE bFlags = 0;
    BOOLEAN bEnabled = FALSE;

    if (EnableCdromFunctions(&g_hModStorProp))
    {
        bFlags = 0;

        if (fSet)
        {
            BOOLEAN bEnable = fSetFlags & CDROM_DIGITAL_PLAY_ENABLED;
            BOOLEAN bAlready = FALSE;
            _gCdromIsDigitalPlaybackEnabled(pcds->hDevInfo,pcds->pDevInfoData,&bAlready);

            if (bEnable != bAlready)
            {
                if (bEnable)
                {
                    _gCdromEnableDigitalPlayback(pcds->hDevInfo,pcds->pDevInfoData,FALSE);
                }
                else
                {
                    _gCdromDisableDigitalPlayback(pcds->hDevInfo,pcds->pDevInfoData);
                }
            }
        }

        //always do a get after a set
        _gCdromIsDigitalPlaybackEnabled(pcds->hDevInfo,pcds->pDevInfoData,&bEnabled);

        if (bEnabled)
        {
            bFlags |= CDROM_DIGITAL_PLAY_ENABLED;
        }

        if (_gCdromKnownGoodDigitalPlayback(pcds->hDevInfo,pcds->pDevInfoData))
        {
            bFlags |= CDROM_DIGITAL_DEVICE_KNOWN;
        }
    }

    return bFlags;
}

BYTE CDAudio_SetDigitalFlags(PCDSTATE pcds, BYTE fDigFlags)
{
    return CDAudio_GetSetDigitalFlags(pcds,fDigFlags,TRUE);
}

BYTE CDAudio_GetDigitalFlags(PCDSTATE pcds)
{
    return CDAudio_GetSetDigitalFlags(pcds,0,FALSE);
}


/*
 * */
void CDAudio_GetRegData(PCDSTATE pcds,ULONG uDrive)
{
    TCHAR    szRegstrCDAudio[_MAX_PATH];
    HKEY    hkTmp;
    
    if (!pcds)
    	return;
    
    wsprintf(szRegstrCDAudio, gszUnitEnum, gszRegstrCDAPath, uDrive);

    pcds->cdar.dwVol = 0xFF;

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE
			     , szRegstrCDAudio
			     , 0
			     , KEY_READ
			     , &hkTmp ) == ERROR_SUCCESS)
    {
	    DWORD cbCDA = sizeof(CDAREG);
	    RegQueryValueEx(hkTmp
			    , gszSettingsKey
			    , NULL
			    , NULL
			    , (LPBYTE)&pcds->cdar
			    , &cbCDA);
	    RegCloseKey(hkTmp);
    }
    
    pcds->cdar.unit = uDrive;

    pcds->fDigFlags = CDAudio_GetDigitalFlags(pcds);

    pcds->DigitalEnabled = pcds->fDigFlags & CDROM_DIGITAL_PLAY_ENABLED;
    pcds->DigitalKnownDevice = pcds->fDigFlags & CDROM_DIGITAL_DEVICE_KNOWN;

    pcds->oldDigEnabled = pcds->DigitalEnabled;
    pcds->dwOldVol = pcds->cdar.dwVol;
}

/*
 * */
void CDAudio_SetRegData(
    PCDSTATE pcds, HWND hwnd)
{
    TCHAR        szRegstrCDAudio[_MAX_PATH];
    HKEY        hkTmp;
    BYTE        bFlags = 0;

    wsprintf(szRegstrCDAudio, gszUnitEnum, gszRegstrCDAPath, pcds->cdar.unit);
    
    if (RegCreateKeyEx(HKEY_LOCAL_MACHINE
		       , szRegstrCDAudio
		       , 0
		       , NULL
		       , 0
		       , KEY_WRITE
		       , NULL
		       , &hkTmp
		       , NULL ) == ERROR_SUCCESS)
    {
	RegSetValueEx(hkTmp
		      , gszSettingsKey
		      , 0L
		      , REG_BINARY
		      , (LPBYTE)&pcds->cdar
		      , sizeof(CDAREG));
	RegCloseKey(hkTmp);
    }

    if (pcds->DigitalEnabled)
    {
	    pcds->fDigFlags |= CDROM_DIGITAL_PLAY_ENABLED;
    }
    else
    {
	    pcds->fDigFlags &= ~CDROM_DIGITAL_PLAY_ENABLED;
    }

    bFlags = CDAudio_SetDigitalFlags(pcds,pcds->fDigFlags);

    //check for success
    if ((bFlags & CDROM_DIGITAL_PLAY_ENABLED) != (pcds->DigitalEnabled))
    {
        pcds->DigitalEnabled = bFlags & CDROM_DIGITAL_PLAY_ENABLED;
        Button_SetCheck(GetDlgItem(hwnd, IDC_CDEN_DIGAUDIO), pcds->DigitalEnabled);
    }
}

void ChangeCDVolume(PCDSTATE pcds)
{
    MCI_OPEN_PARMS  mciOpen;
    TCHAR           szElementName[4];
    TCHAR           szAliasName[32];
    DWORD           dwFlags;
    DWORD           dwAliasCount = GetCurrentTime();
    DWORD           dwRet;
    CDAREG          cdarCache;
    HKEY            hkTmp;
    TCHAR            szRegstrCDAudio[_MAX_PATH];
    
    ASSERT(pcds);

    if (pcds != NULL)
    { 
        wsprintf(szRegstrCDAudio, gszUnitEnum, gszRegstrCDAPath, pcds->cdar.unit);

        if (RegOpenKeyEx(HKEY_LOCAL_MACHINE , szRegstrCDAudio , 0 , KEY_READ , &hkTmp ) == ERROR_SUCCESS)
        {
            DWORD cbCDA = sizeof(CDAREG);
            RegQueryValueEx(hkTmp , gszSettingsKey , NULL , NULL , (LPBYTE)&cdarCache , &cbCDA);
            RegCloseKey(hkTmp);
        }
        else
        {
            cdarCache = pcds->cdar;
        }

        if (RegCreateKeyEx(HKEY_LOCAL_MACHINE, szRegstrCDAudio, 0, NULL, 0, KEY_WRITE, NULL, &hkTmp, NULL ) == ERROR_SUCCESS)
        {
            CDAREG cdar;
        
            cdar = pcds->cdar;      
            RegSetValueEx(hkTmp , gszSettingsKey , 0L , REG_BINARY , (LPBYTE)&cdar , sizeof(CDAREG));
            RegCloseKey(hkTmp);
        }

        ZeroMemory( &mciOpen, sizeof(mciOpen) );

        mciOpen.lpstrDeviceType = (LPTSTR)MCI_DEVTYPE_CD_AUDIO;
        wsprintf( szElementName, TEXT("%c:"), pcds->chDrive );
        wsprintf( szAliasName, TEXT("SJE%lu:"), dwAliasCount );

        mciOpen.lpstrElementName = szElementName;
        mciOpen.lpstrAlias = szAliasName;

        dwFlags = MCI_OPEN_ELEMENT | MCI_OPEN_ALIAS | MCI_OPEN_SHAREABLE | 
                  MCI_OPEN_TYPE | MCI_OPEN_TYPE_ID | MCI_WAIT;

        dwRet = mciSendCommand(0, MCI_OPEN, dwFlags, (DWORD_PTR)(LPVOID)&mciOpen);

        if ( dwRet == MMSYSERR_NOERROR )
        {     
            mciSendCommand(mciOpen.wDeviceID, MCI_CLOSE, 0L, 0L );
        }

        if (RegOpenKeyEx(HKEY_LOCAL_MACHINE , szRegstrCDAudio , 0 , KEY_WRITE , &hkTmp ) == ERROR_SUCCESS)
        {
            RegSetValueEx(hkTmp , gszSettingsKey , 0L , REG_BINARY , (LPBYTE)&cdarCache , sizeof(CDAREG));
            RegCloseKey(hkTmp);
        }
    }
}

/*
 * */
void CDAudio_SaveState(
    HWND        hwnd)
{
    PCDSTATE pcds;
    pcds = (PCDSTATE)GetWindowLongPtr(hwnd,DWLP_USER);

    if (pcds)
    { 
        CDAudio_SetRegData(pcds,hwnd);
        pcds->oldDigEnabled = pcds->DigitalEnabled;
        pcds->dwOldVol = pcds->cdar.dwVol;

        ChangeCDVolume(pcds);
    }
}

void CDAudio_DigitalPlaybackEnable(HWND hDlg)
{
    PCDSTATE pcds;
    pcds = (PCDSTATE)GetWindowLongPtr(hDlg,DWLP_USER);   

    if (pcds)
    {
        pcds->DigitalEnabled = (BOOLEAN)Button_GetCheck(GetDlgItem(hDlg, IDC_CDEN_DIGAUDIO));
    }
}

ULONG MatchDriveToDevInst(DWORD DevInst)
{
    HANDLE hMatch = INVALID_HANDLE_VALUE;
    STORAGE_DEVICE_NUMBER sDevice, sMatch;
    DWORD bytesReturned;
    ULONG uRet = 0;

    if (INVALID_HANDLE_VALUE != (hMatch = 
                                 GetHandleForDeviceInst(DevInst)))
    {
        if (DeviceIoControl(hMatch,
                             IOCTL_STORAGE_GET_DEVICE_NUMBER,
                             NULL,
                             0,
                             &sMatch,
                             sizeof(STORAGE_DEVICE_NUMBER),
                             &bytesReturned,
                             NULL))
        {
            uRet = sMatch.DeviceNumber;
        }

        CloseHandle(hMatch);
    }

    return uRet;
}

/*
 * */
DWORD CDAudio_InitDrives(HWND hwnd, PCDSTATE pcds)
{
    DWORD cch;
    DWORD cCDs = 0;
    ULONG uDrive = 0;

    uDrive = MatchDriveToDevInst(pcds->pDevInfoData->DevInst);
    CDAudio_GetRegData(pcds,uDrive);

    if (cch = GetLogicalDriveStrings(0, NULL))
    {
	LPTSTR   lpDrives,lp;
	lp = lpDrives = GlobalAllocPtr(GHND, cch * sizeof(TCHAR));
	cch = GetLogicalDriveStrings(cch, lpDrives);
	if (lpDrives && cch)
	{
	    // upon the last drive enumerated, there will be a double
	    // null termination
	    while (*lpDrives)
	    {
		    if (GetDriveType(lpDrives) == DRIVE_CDROM)
		    {
			    int   i;
			    LPTSTR lp;
			    lp = CharUpper(lpDrives);
			    
			    while (*lp != TEXT('\\'))
			        lp = CharNext(lp);
			    
			    while (*lp)
			    {
			        *lp = TEXT(' ');
			        lp = CharNext(lp);
			    }
			        
			    if (cCDs == uDrive)
			    {
                   pcds->chDrive = lpDrives[0];
			    }

		        cCDs++;
		    }
		    for ( ; *lpDrives ; lpDrives++ );
		    lpDrives++;
	    }
	}
	
	if (lp)
	    GlobalFreePtr(lp);

    }
    return cCDs;
}

//
//  Determines what device is currently being used by the mapper to play audio
//
MMRESULT GetWaveID(UINT *puWaveID)

{
    PAUDIODLGINFO pInfo = (PAUDIODLGINFO)LocalAlloc(LPTR, sizeof(AUDIODLGINFO));;
	GetPrefInfo(pInfo, NULL);
	if(-1 == pInfo->uPrefOut)
	{
		LocalFree((HLOCAL)pInfo);
		return MMSYSERR_BADDEVICEID;
	}

	*puWaveID = pInfo->uPrefOut;
	LocalFree((HLOCAL)pInfo);

	return MMSYSERR_NOERROR;
}


//
// Checks to see if the current output audio device is a WDM Device or not
//
BOOL WDMAudioEnabled(void)
{
    BOOL fResult = FALSE;

    UINT uWaveID;

    if (GetWaveID(&uWaveID) == MMSYSERR_NOERROR)
    {
        WAVEOUTCAPS woc;

        if (waveOutGetDevCaps(uWaveID, &woc, sizeof(WAVEOUTCAPS)) == MMSYSERR_NOERROR)
        {
            if ((woc.wMid == MM_MICROSOFT) && (woc.wPid == MM_MSFT_WDMAUDIO_WAVEOUT))
            {
                fResult = TRUE;
            }
        }
    }

    return(fResult);
}



BOOL CDAudio_OnInitDialog(
    HWND        hwnd,
    HWND        hwndFocus,
    LPARAM      lParam)
{
    HWND     hwndTB1 = GetDlgItem(hwnd, IDC_CD_TB_VOLUME);
    HWND     hwndCK3 = GetDlgItem(hwnd, IDC_CDEN_DIGAUDIO);
    HWND     hwndTX1 = GetDlgItem(hwnd, IDC_TEXT_24);
    UINT     uDrive;
    int      i;
    PCDSTATE pcds = NULL;
    PALLDEVINFO pDevInfo = NULL;

    pcds = (PCDSTATE)GlobalAllocPtr(GHND, sizeof(CDSTATE));
    SetWindowLongPtr(hwnd,DWLP_USER,(LONG_PTR)pcds);

    pDevInfo = (ALLDEVINFO *) ((LPPROPSHEETPAGE) lParam)->lParam;

    if (pDevInfo)
    {
        pcds->hDevInfo = pDevInfo->hDevInfo;
        pcds->pDevInfoData = pDevInfo->pDevInfoData;
        GlobalFreePtr(pDevInfo);
        ((LPPROPSHEETPAGE) lParam)->lParam = (LPARAM) NULL;
    }

    SendMessage(hwndTB1, TBM_SETTICFREQ, 10, 0);
    SendMessage(hwndTB1, TBM_SETRANGE, FALSE, MAKELONG(0,100));

    i = CDAudio_InitDrives(hwnd,pcds);
    
    if (i)
    {
        if (pcds)
        {
            SendMessage(hwndTB1, TBM_SETPOS, TRUE, (pcds->cdar.dwVol * 100L)/255L );
            Button_SetCheck(hwndCK3, pcds->DigitalEnabled);
        }

        g_fWDMEnabled = WDMAudioEnabled();

        if (!g_fWDMEnabled)             // If we are not running on a WDM Device, disable dig audio checkbox.
        {
            EnableWindow(hwndCK3, FALSE);
            EnableWindow(hwndTX1, FALSE);
        }
    }
    else
    {
        EnableWindow(hwndCK3, FALSE);
        EnableWindow(hwndTB1, FALSE);
        EnableWindow(hwndTX1, FALSE);
    }

    return FALSE;
}

void CDToggleApply(HWND hDlg)
{
    PCDSTATE    pcds;
    BOOL        fChanged = FALSE;

    pcds = (PCDSTATE)GetWindowLongPtr(hDlg,DWLP_USER); 

    if (pcds)
    {
        if (pcds->DigitalEnabled != pcds->oldDigEnabled)
        {
            fChanged = TRUE;
        }

        if (pcds->dwOldVol != pcds->cdar.dwVol)
        {
            fChanged = TRUE;
        }
    }

    if (fChanged)
    {
        PropSheet_Changed(GetParent(hDlg),hDlg);
    }
    else
    {
        PropSheet_UnChanged(GetParent(hDlg),hDlg);
    }
}

void CDAudio_OnDestroy(
    HWND        hwnd)
{
    PCDSTATE lp = (PCDSTATE)GetWindowLongPtr(hwnd,DWLP_USER);
    if (lp)
        GlobalFreePtr(lp);

    if (g_hModStorProp)
    {
        FreeLibrary(g_hModStorProp);
        g_hModStorProp = NULL;
    }
}

void CDAudio_OnHScroll(
    HWND        hwnd,
    HWND        hwndCtl,
    UINT        code,
    int         pos)
{
    if (code == TB_ENDTRACK || code == SB_THUMBTRACK) 
    {
        HWND        hwndTB1 = GetDlgItem(hwnd, IDC_CD_TB_VOLUME);
        int         i; 
        PCDSTATE    pcds;
        DWORD       dwVol;

        pcds = (PCDSTATE)GetWindowLongPtr(hwnd,DWLP_USER);

        if (CB_ERR != (UINT_PTR) pcds && pcds)
        {
            dwVol = (((DWORD)SendMessage(hwndTB1, TBM_GETPOS, 0, 0)) * 255L) / 100L;

            if (dwVol != pcds->cdar.dwVol)
            {
                pcds->cdar.dwVol = dwVol;
                ChangeCDVolume(pcds);   
                CDToggleApply(hwnd);
            }
        }
    }
}
    
void CDAudio_OnCancel(
    HWND        hwnd)
{
    PCDSTATE    pcds;

    pcds = (PCDSTATE)GetWindowLongPtr(hwnd,DWLP_USER);
    pcds->cdar.dwVol = pcds->dwOldVol;
    ChangeCDVolume(pcds);
}

BOOL PASCAL CDAudio_OnCommand(
    HWND        hDlg,
    int         id,
    HWND        hwndCtl,
    UINT        codeNotify)
{
    BOOL fResult = FALSE;

    switch (id)
    {
	case ID_APPLY:
	{
	    CDAudio_SaveState(hDlg);
	    fResult = TRUE;
	}
	break;

	case IDCANCEL:
	{
	    CDAudio_OnCancel(hDlg);
	    fResult = TRUE;
	}
	break;

	case IDC_CDEN_DIGAUDIO:
	{
	    CDAudio_DigitalPlaybackEnable(hDlg);
	    CDToggleApply(hDlg);
	}
	break;
    }

    return fResult;
}


const static DWORD aCDHelpIds[] = {  // Context Help IDs

    IDI_CDAUDIO,         IDH_COMM_GROUPBOX,
    IDC_ICON_5,          IDH_COMM_GROUPBOX,
    IDC_TEXT_25,         IDH_COMM_GROUPBOX,
    IDC_GROUPBOX,        IDH_MMSE_GROUPBOX,
    IDC_GROUPBOX_2,      IDH_MMSE_GROUPBOX,
    IDC_TEXT_29,         IDH_CD_VOL_HEADPHONE,
    IDC_CD_TB_VOLUME,    IDH_CD_VOL_HEADPHONE,
    IDC_TEXT_30,         IDH_CD_VOL_HEADPHONE,
    IDC_TEXT_24,         IDH_CDROM_PROPERTIES_DIGITAL,
    IDC_CDEN_DIGAUDIO,   IDH_CDROM_PROPERTIES_DIGITAL,

    0, 0
};

BOOL CALLBACK CDDlg(
    HWND        hDlg,
    UINT        uMsg,
    WPARAM      wParam,
    LPARAM      lParam)
{
    NMHDR FAR   *lpnm;

    switch (uMsg)
    {
	case WM_NOTIFY:
	    lpnm = (NMHDR FAR *)lParam;
	    switch(lpnm->code)
	    {
		case PSN_KILLACTIVE:
		    FORWARD_WM_COMMAND(hDlg, IDOK, 0, 0, SendMessage);  
		    break;              

		case PSN_APPLY:
		    FORWARD_WM_COMMAND(hDlg, ID_APPLY, 0, 0, SendMessage);      
		    break;                                                      

		case PSN_SETACTIVE:
		    FORWARD_WM_COMMAND(hDlg, ID_INIT, 0, 0, SendMessage);
		    break;

		case PSN_RESET:
		    FORWARD_WM_COMMAND(hDlg, IDCANCEL, 0, 0, SendMessage);
		    break;
	    }
	    break;

	case WM_INITDIALOG:
	    HANDLE_WM_INITDIALOG(hDlg, wParam, lParam, CDAudio_OnInitDialog);
	    break;

	case WM_DESTROY:
	    HANDLE_WM_DESTROY(hDlg, wParam, lParam, CDAudio_OnDestroy);
	    break;

	case WM_DROPFILES:
	    break;

	case WM_CONTEXTMENU:        
	    WinHelp((HWND)wParam, gszWindowsHlp, HELP_CONTEXTMENU, 
		  (UINT_PTR)(LPTSTR)aCDHelpIds);
	    break;

	case WM_HELP:        
	    WinHelp(((LPHELPINFO)lParam)->hItemHandle, gszWindowsHlp,
		  HELP_WM_HELP, (UINT_PTR)(LPTSTR)aCDHelpIds);
	    break;

	case WM_COMMAND:
	    HANDLE_WM_COMMAND(hDlg, wParam, lParam, CDAudio_OnCommand);
	    break;

	case WM_HSCROLL:
	    HANDLE_WM_HSCROLL(hDlg, wParam, lParam, CDAudio_OnHScroll);
	    break;

#if 0        
	default:
	    if (uMsg == wHelpMessage) 
	    {
		WinHelp(hDlg, gszWindowsHlp, HELP_CONTEXT, ID_SND_HELP);
		return TRUE;
	    }
	    break;
#endif
	    
    }
    return FALSE;
}
