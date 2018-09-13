/*
 ***************************************************************
 *  sndsel.c
 *
 *  This file contains the dialogproc and the dialog initialization code
 *
 *  Copyright 1993, Microsoft Corporation
 *
 *  History:
 *
 *    07/94 - VijR (Created)
 *
 ***************************************************************
 */

#include <windows.h>
#include <mmsystem.h>
#include <string.h>
#include <cpl.h>
#include <shellapi.h>
#include <ole2.h>
#include <commdlg.h>
#define NOSTATUSBAR
#include <commctrl.h>
#include <prsht.h>
#include <regstr.h>
#include <dbt.h>

#include <mmddkp.h>

#include <ks.h>
#include <ksmedia.h>
#include "mmcpl.h"
#include "medhelp.h"
#include "sound.h"
#include "utils.h"
#include "trayvol.h"
#include <winbasep.h>   // for HFINDFILE*

/*
 ***************************************************************
 * Defines
 ***************************************************************
 */
#define DF_PM_SETBITMAP    (WM_USER+1)
#define     SNDVOL_NOTCHECKED   0
#define     SNDVOL_PRESENT      1
#define     SNDVOL_NOTPRESENT   2

#define     VOLUME_TICS         (500)

/*
 ***************************************************************
 * Globals
 ***************************************************************
 */

SZCODE      gszWindowsHlp[]    = TEXT("windows.hlp");
SZCODE      gszNull[2]         = TEXT("\0");
SZCODE      gszNullScheme[]    = TEXT(".none");

TCHAR        gszCurDir[MAXSTR]     = TEXT("\0");
TCHAR        gszNone[32];
TCHAR        gszRemoveScheme[MAXSTR];
TCHAR        gszChangeScheme[MAXSTR];
TCHAR        gszMediaDir[MAXSTR];
TCHAR        gszDefaultApp[32];

static HDEVNOTIFY   ghDeviceEventContext    = NULL;
static int          g_iSndVolExists         = SNDVOL_NOTCHECKED;
static SZCODE       aszSndVolOptionKey[]    = REGSTR_PATH_SETUP TEXT("\\SETUP\\OptionalComponents\\Vol");
static SZCODE       aszInstalled[]          = TEXT("Installed");
static const char   aszSndVol32[]           = "sndvol32.exe";


int                             giScheme;
BOOL                            gfChanged;                    //set to TRUE if sound info change
BOOL                            gfNewScheme;
BOOL                            gfDeletingTree;
HWND                            ghWnd;
HMIXEROBJ                       ghMixer = NULL;
UINT                            guMixID = 0;
DWORD                           gdwVolID = (DWORD) -1;
DWORD                           gdwMuteID = (DWORD) -1;
DWORD                           gdwPreviousVolume = 0;
BOOL                            gfPreviousMute = FALSE;
BOOL                            gfMasterVolume = FALSE;
BOOL                            gfMasterMute = FALSE;
MIXERCONTROLDETAILS             gmcd;
MIXERCONTROLDETAILS_UNSIGNED    gmvol[2];
OPENFILENAME                    ofn;
BOOL                            gfInternalGenerated = FALSE;
UINT                            giVolDevChange = 0;
WNDPROC                         gfnVolPSProc = NULL;

/*
 ***************************************************************
 * Globals used in painting disp chunk display.
 ***************************************************************
*/
HBITMAP     ghDispBMP;
HBITMAP     ghIconBMP;
HPALETTE    ghPal;
BOOL        gfWaveExists = FALSE;   // indicates wave device in system.

HTREEITEM ghOldItem = NULL;

/*
 ***************************************************************
 * File Globals
 ***************************************************************
 */

static TCHAR        aszFileName[MAXSTR] = TEXT("\0");
static TCHAR        aszPath[MAXSTR]     = TEXT("\0");

static TCHAR        aszBrowse[MAXSTR];
static TCHAR        aszBrowseStr[64];
static TCHAR        aszNullSchemeLabel[MAXSTR];

//TCHAR        *aszFilter[] = {"Wave Files(*.wav)", "*.wav", ""};
static TCHAR        aszFilter[MAXSTR];
static TCHAR        aszNullChar[2];

static SZCODE   aszLnk[] = TEXT(".lnk");
static SZCODE   aszWavFilter[] = TEXT("*.wav");
static SZCODE   aszDefaultScheme[]    = TEXT("Appevents\\schemes");
static SZCODE   aszNames[]            = TEXT("Appevents\\schemes\\Names");
static SZCODE   aszDefault[]        = TEXT(".default");
static SZCODE   aszCurrent[]        = TEXT(".current");
static INTCODE  aKeyWordIds[] =
{
    CB_SCHEMES,         IDH_EVENT_SCHEME,
    IDC_TEXT_14,        IDH_EVENT_SCHEME,
    ID_SAVE_SCHEME,     IDH_EVENT_SAVEAS_BUTTON,
    ID_REMOVE_SCHEME,   IDH_EVENT_DELETE_BUTTON,
    IDC_EVENT_TREE,     IDH_EVENT_EVENT,
    IDC_SOUNDGRP,       IDH_MMSE_GROUPBOX,
    IDC_STATIC_PREVIEW, IDH_EVENT_BROWSE_PREVIEW,
    ID_DISPFRAME,       IDH_EVENT_BROWSE_PREVIEW,
    ID_PLAY,            IDH_EVENT_PLAY,
    IDC_GROUPBOX,       IDH_MMSE_GROUPBOX,
    IDC_GROUPBOX_2,     IDH_MMSE_GROUPBOX,
    IDC_STATIC_NAME,    IDH_EVENT_FILE,
    IDC_SOUND_FILES,    IDH_EVENT_FILE,
    ID_BROWSE,          IDH_EVENT_BROWSE,
    IDC_TASKBAR_VOLUME, IDH_AUDIO_SHOW_INDICATOR,
    IDC_MASTERVOLUME,   IDH_SOUNDS_SYS_VOL_CONTROL,
	IDC_VOLUME_HIGH,	IDH_SOUNDS_SYS_VOL_CONTROL,
	IDC_VOLUME_LOW,		IDH_SOUNDS_SYS_VOL_CONTROL,
    IDC_TEXT_15,        IDH_SOUNDS_SYS_VOL_CONTROL,
    ID_MUTE,            IDH_SOUNDS_VOL_MUTE_BUTTON,
    0,0
};

BOOL        gfEditBoxChanged;
BOOL        gfSubClassedEditWindow;
BOOL        gfSoundPlaying;

HBITMAP     hBitmapPlay;
HBITMAP     hBitmapStop;

HICON       hIconVolume;
HICON       hIconVolTrans;
HICON       hIconVolUp;
HICON       hIconVolDown;
HICON       hIconMute;
HICON       hIconMuteUp;
HICON       hIconMuteDown;

HIMAGELIST  hSndImagelist;
LONG        glCachedBalance = 0; //last balance level

/*
 ***************************************************************
 * extern
 ***************************************************************
 */

extern      HSOUND ghse;
extern      BOOL    gfNukeExt;
/*
 ***************************************************************
 * Prototypes
 ***************************************************************
 */
BOOL PASCAL DoCommand           (HWND, int, HWND, UINT);
BOOL PASCAL InitDialog          (HWND);
BOOL PASCAL InitStringTable     (void);
BOOL PASCAL InitFileOpen        (HWND, LPOPENFILENAME);
BOOL PASCAL SoundCleanup        (HWND);
LPTSTR PASCAL NiceName           (LPTSTR, BOOL);
BOOL ResolveLink                (LPTSTR, LPTSTR, LONG);

// stuff in sndfile.c
//
BOOL PASCAL ShowSoundMapping    (HWND, PEVENT);
BOOL PASCAL ChangeSoundMapping  (HWND, LPTSTR, PEVENT);
BOOL PASCAL PlaySoundFile       (HWND, LPTSTR);
BOOL PASCAL QualifyFileName     (LPTSTR, LPTSTR, int, BOOL);

// Stuff in scheme.c
//
INT_PTR CALLBACK  SaveSchemeDlg(HWND, UINT, WPARAM, LPARAM);
BOOL PASCAL RegNewScheme        (HWND, LPTSTR, LPTSTR, BOOL);
BOOL PASCAL RegSetDefault       (LPTSTR);
BOOL PASCAL ClearModules        (HWND, HWND, BOOL);
BOOL PASCAL LoadModules         (HWND, LPTSTR);
BOOL PASCAL RemoveScheme        (HWND);
BOOL PASCAL AddScheme           (HWND, LPTSTR, LPTSTR, BOOL, int);
BOOL PASCAL RegDeleteScheme(HWND hWndC, int iIndex);


void InitVolume(HWND hDlg, BOOL fFirst);
void DeviceChange_Change(HWND hDlg, WPARAM wParam, LPARAM lParam);
void DeviceChange_Init(HWND hWnd, DWORD dwMixerID);
BOOL DeviceChange_GetHandle(DWORD dwMixerID, HANDLE *phDevice);
void DeviceChange_Cleanup(void);

/*
 ***************************************************************
 ***************************************************************
 */


void AddExt(LPTSTR sz, LPCTSTR x)
{
    UINT  cb;

    for (cb = lstrlen(sz); cb; --cb)
    {
        if (TEXT('.') == sz[cb])
            return;

        if (TEXT('\\') == sz[cb])
            break;
    }
    lstrcat (sz, x);
}


static void AddFilesToLB(HWND hwndList, LPTSTR pszDir, LPCTSTR szSpec)
{
    WIN32_FIND_DATA fd;
    HFINDFILE h;
    TCHAR szBuf[256];

    ComboBox_ResetContent(hwndList);

    lstrcpy(szBuf, pszDir);
    lstrcat(szBuf, cszSlash);
    lstrcat(szBuf, szSpec);

    h = FindFirstFile(szBuf, &fd);

    if (h != INVALID_HANDLE_VALUE)
    {
        // If we have only a short name, make it pretty.
        do {
            //if (fd.cAlternateFileName[0] == 0 ||
            //    lstrcmp(fd.cFileName, fd.cAlternateFileName) == 0)
            //{
                NiceName(fd.cFileName, TRUE);
            //}
            SendMessage(hwndList, CB_ADDSTRING, 0, (LPARAM)(LPTSTR)fd.cFileName);
        }
        while (FindNextFile(h, &fd));

        FindClose(h);
    }
    ComboBox_InsertString(hwndList, 0, (LPARAM)(LPTSTR)gszNone);
}

static void SetCurDir(HWND hDlg, LPTSTR lpszPath, BOOL fParse, BOOL fChangeDir)
{
    TCHAR szTmp[MAX_PATH];
    TCHAR szOldDir[MAXSTR];
    LPTSTR lpszTmp;

    lstrcpy (szOldDir, gszCurDir);
    if (!fParse)
    {
        lstrcpy(gszCurDir, lpszPath);
        goto AddFiles;
    }
    lstrcpy(szTmp, lpszPath);
    for (lpszTmp = (LPTSTR)(szTmp + lstrlen(szTmp)); lpszTmp > szTmp; lpszTmp = CharPrev(szTmp, lpszTmp))
    {
        if (*lpszTmp == TEXT('\\'))
        {
            *lpszTmp = TEXT('\0');
            lstrcpy(gszCurDir, szTmp);
            break;
        }
    }
    if (lpszTmp <= szTmp)
        lstrcpy(gszCurDir, gszMediaDir);
AddFiles:
    if (fChangeDir)
    {
        if (!SetCurrentDirectory(gszCurDir))
        {
            if (lstrcmp (gszMediaDir, lpszPath))
                SetCurrentDirectory (gszMediaDir);
            else
            {
                GetWindowsDirectory (gszCurDir, sizeof(gszCurDir)/sizeof(TCHAR));
                SetCurrentDirectory (gszCurDir);
            }
        }
    }
    if (lstrcmpi (szOldDir, gszCurDir))
    {
        AddFilesToLB(GetDlgItem(hDlg, IDC_SOUND_FILES),gszCurDir, aszWavFilter);
    }
}

static BOOL TranslateDir(HWND hDlg, LPTSTR pszPath)
{
    TCHAR szCurDir[MAX_PATH];
    int nFileOffset = lstrlen(pszPath);

    lstrcpy(szCurDir, pszPath);
    if (szCurDir[nFileOffset - 1] == TEXT('\\'))
        szCurDir[--nFileOffset] = 0;
    if (SetCurrentDirectory(szCurDir))
    {
        if (GetCurrentDirectory(sizeof(szCurDir)/sizeof(TCHAR), szCurDir))
        {
            SetCurDir(hDlg, szCurDir, FALSE, FALSE);
            return TRUE;
        }
    }
    return FALSE;
}





///HACK ALERT!!!! HACK ALERT !!! HACK ALERT !!!!
// BEGIN (HACKING)

HHOOK gfnKBHookScheme = NULL;
HWND ghwndDlg = NULL;
WNDPROC gfnEditWndProc = NULL;

#define WM_NEWEVENTFILE (WM_USER + 1000)
#define WM_RESTOREEVENTFILE (WM_USER + 1001)

LRESULT CALLBACK SchemeKBHookProc(int code, WPARAM wParam, LPARAM lParam)
{
    if (wParam == VK_RETURN || wParam == VK_ESCAPE)
    {
        HWND hwndFocus = GetFocus();
        if (IsWindow(hwndFocus))
        {
            if (lParam & 0x80000000) //Key Up
            {
                DPF("*****WM_KEYUP for VK_RETURN/ESC\r\n");
                if (wParam == VK_RETURN)
                {
                    if (SendMessage(ghwndDlg, WM_NEWEVENTFILE, 0, 0L))
                    {
                        SetFocus(hwndFocus);
                        gfEditBoxChanged = TRUE;
                        return 1;
                    }
                }
                else
                    SendMessage(ghwndDlg, WM_RESTOREEVENTFILE, 0, 0L);
            }
        }
        if (gfnKBHookScheme && (lParam & 0x80000000))
        {
            UnhookWindowsHookEx(gfnKBHookScheme);
            gfnKBHookScheme = NULL;
        }
        return 1;       //remove message
    }
    return CallNextHookEx(gfnKBHookScheme, code, wParam, lParam);
}

STATIC void SetSchemesKBHook(HWND hwnd)
{
    if (gfnKBHookScheme)
        return;
    gfnKBHookScheme = SetWindowsHookEx(WH_KEYBOARD, (HOOKPROC)SchemeKBHookProc, ghInstance,0);
}

LRESULT CALLBACK SubClassedEditWndProc(HWND hwnd, UINT wMsg, WPARAM wParam, LPARAM lParam)
{
    switch(wMsg)
    {
        case WM_SETFOCUS:
            DPF("*****WM_SETFOCUS\r\n");
            SetSchemesKBHook(hwnd);
            gfEditBoxChanged = FALSE;
            break;
        case WM_KILLFOCUS:
            if (gfnKBHookScheme)
            {
                DPF("*****WM_KILLFOCUS\r\n");
                UnhookWindowsHookEx(gfnKBHookScheme);
                gfnKBHookScheme = NULL;
                if (gfEditBoxChanged)
                    SendMessage(ghwndDlg, WM_NEWEVENTFILE, 0, 1L);
            }
            break;
    }
    return CallWindowProc((WNDPROC)gfnEditWndProc, hwnd, wMsg, wParam, lParam);
}

STATIC void SubClassEditWindow(HWND hwndEdit)
{
    gfnEditWndProc = (WNDPROC)GetWindowLongPtr(hwndEdit, GWLP_WNDPROC);
    SetWindowLongPtr(hwndEdit, GWLP_WNDPROC, (LONG_PTR)SubClassedEditWndProc);
}



// END (HACKING)

STATIC void EndSound(HSOUND * phse)
{
    if (*phse)
    {
        HSOUND hse = *phse;

        *phse = NULL;
        soundStop(hse);
        soundOnDone(hse);
        soundClose(hse);
    }
}


void    CheckForSndVolPresence(void)
{
    if (g_iSndVolExists == SNDVOL_NOTCHECKED)
    {
        OFSTRUCT of;

        if (HFILE_ERROR != OpenFile(aszSndVol32, &of, OF_EXIST | OF_SHARE_DENY_NONE))
        {
            g_iSndVolExists = SNDVOL_PRESENT;
        }
        else
        {
            HKEY    hkSndVol;

            g_iSndVolExists = SNDVOL_NOTPRESENT;

            if (!RegOpenKey(HKEY_LOCAL_MACHINE, aszSndVolOptionKey, &hkSndVol))
            {
                RegSetValueEx(hkSndVol, (LPTSTR)aszInstalled, 0L, REG_SZ, (LPBYTE)(TEXT("0")), 4);
                RegCloseKey(hkSndVol);
            }
        }
    }
}


// Locates the master volume and mute controls for this mixer line
//
void SearchControls(int mxid, LPMIXERLINE pml, LPDWORD pdwVolID, LPDWORD pdwMuteID, BOOL *pfFound)
{
    MIXERLINECONTROLS mlc;
    DWORD dwControl;

    memset(&mlc, 0, sizeof(mlc));
    mlc.cbStruct = sizeof(mlc);
    mlc.dwLineID = pml->dwLineID;
    mlc.cControls = pml->cControls;
    mlc.cbmxctrl = sizeof(MIXERCONTROL);
    mlc.pamxctrl = (LPMIXERCONTROL) GlobalAlloc(GMEM_FIXED, sizeof(MIXERCONTROL) * pml->cControls);

    if (mlc.pamxctrl)
    {
        if (mixerGetLineControls((HMIXEROBJ) mxid, &mlc, MIXER_GETLINECONTROLSF_ALL) == MMSYSERR_NOERROR)
        {
            for (dwControl = 0; dwControl < pml->cControls && !(*pfFound); dwControl++)
            {
                if (mlc.pamxctrl[dwControl].dwControlType == (DWORD)MIXERCONTROL_CONTROLTYPE_VOLUME)
                {
                    DWORD dwIndex;
                    DWORD dwVolID = (DWORD) -1;
                    DWORD dwMuteID = (DWORD) -1;

                    dwVolID = mlc.pamxctrl[dwControl].dwControlID;

                    for (dwIndex = 0; dwIndex < pml->cControls; dwIndex++)
                    {
                        if (mlc.pamxctrl[dwIndex].dwControlType == (DWORD)MIXERCONTROL_CONTROLTYPE_MUTE)
                        {
                            dwMuteID = mlc.pamxctrl[dwIndex].dwControlID;
                            break;
                        }
                    }

                    *pfFound = TRUE;
                    *pdwVolID = dwVolID;
                    *pdwMuteID = dwMuteID;
                }
            }
        }

        GlobalFree((HGLOBAL) mlc.pamxctrl);
    }
}


// Locates the volume slider control for this mixer device
//
BOOL SearchDevice(DWORD dwMixID, LPDWORD pdwDest, LPDWORD pdwVolID, LPDWORD pdwMuteID)
{
    MIXERCAPS   mc;
    MMRESULT    mmr;
    BOOL        fFound = FALSE;

    mmr = mixerGetDevCaps(dwMixID, &mc, sizeof(mc));

    if (mmr == MMSYSERR_NOERROR)
    {
        MIXERLINE   mlDst;
        DWORD       dwDestination;

        for (dwDestination = 0; dwDestination < mc.cDestinations && !fFound; dwDestination++)
        {
            mlDst.cbStruct = sizeof ( mlDst );
            mlDst.dwDestination = dwDestination;

            if (mixerGetLineInfo((HMIXEROBJ) dwMixID, &mlDst, MIXER_GETLINEINFOF_DESTINATION  ) == MMSYSERR_NOERROR)
            {
                if (mlDst.dwComponentType == (DWORD)MIXERLINE_COMPONENTTYPE_DST_SPEAKERS ||    // needs to be a likely output destination
                    mlDst.dwComponentType == (DWORD)MIXERLINE_COMPONENTTYPE_DST_HEADPHONES ||
                    mlDst.dwComponentType == (DWORD)MIXERLINE_COMPONENTTYPE_SRC_WAVEOUT)
                {
                    if (!fFound && mlDst.cControls)     // If there are controls, we'll take the master
                    {
                        SearchControls(dwMixID, &mlDst, pdwVolID, pdwMuteID, &fFound);
                        *pdwDest = dwDestination;
                    }
                }
            }
        }
    }

    return(fFound);
}

// Gets the primary audio device ID and find the mixer line for it
// It leaves it open so the slider can respond to other changes outside this app
//
void MasterVolumeConfig(HWND hWnd)
{
    UINT    uWaveID;
    DWORD   dwDest;

    gfMasterVolume = gfMasterMute = FALSE;

    if (GetWaveID(&uWaveID) == MMSYSERR_NOERROR)
    {
        if(MMSYSERR_NOERROR == mixerGetID((HMIXEROBJ)(uWaveID), &guMixID, MIXER_OBJECTF_WAVEOUT))
        {
            if (SearchDevice(guMixID, &dwDest, &gdwVolID, &gdwMuteID))
            {
                if (ghMixer)
                {
                    mixerClose((HMIXER) ghMixer);
                    ghMixer = NULL;
                }

                if(MMSYSERR_NOERROR == mixerOpen((HMIXER *) &ghMixer, guMixID, (DWORD_PTR) hWnd, 0L, CALLBACK_WINDOW))
                {
                    MIXERLINE           mlDst;
                    MMRESULT            mmr;

                    ZeroMemory(&mlDst, sizeof(mlDst));
    
                    mlDst.cbStruct      = sizeof(mlDst);
                    mlDst.dwDestination = dwDest;
    
                    mmr = mixerGetLineInfo((HMIXEROBJ)ghMixer, &mlDst, MIXER_GETLINEINFOF_DESTINATION);

                    if (mmr == MMSYSERR_NOERROR)
                    {
                        gmcd.cbStruct = sizeof(gmcd);
                        gmcd.dwControlID = gdwVolID;
                        gmcd.cChannels = mlDst.cChannels;
                        gmcd.hwndOwner = 0;
                        gmcd.cMultipleItems = 0;
                        gmcd.cbDetails = sizeof(DWORD); //seems like it would be sizeof(gmvol),
                                                        //but actually, it is the size of a single value
                                                        //and is multiplied by channel in the driver.
                        gmcd.paDetails = &gmvol[0];

                        gfMasterVolume = TRUE;
                        gfMasterMute = (gdwMuteID != (DWORD) -1);
                    }
                }
            }
        }
    }
}


// returns current volume level
//
DWORD GetVolume(void)
{
    DWORD dwVol = 0;

    if (ghMixer)
    {
        gmcd.dwControlID = gdwVolID;
        ZeroMemory(gmvol,sizeof(gmvol));
        mixerGetControlDetails(ghMixer,&gmcd,MIXER_GETCONTROLDETAILSF_VALUE);
        dwVol = ((gmvol[0].dwValue > gmvol[1].dwValue) ? gmvol[0].dwValue : gmvol[1].dwValue);
    }

    return dwVol;
}

// Sets the volume level
//
void SetVolume(DWORD dwVol)
{
    long    lBalance = 0;
    long    lDiv;

    if (ghMixer)
    {
        gmcd.dwControlID = gdwVolID;

        //if this is a stereo device, we need to check the balance
        if (gmcd.cChannels > 1)
        {
            ZeroMemory(gmvol,sizeof(gmvol));
            mixerGetControlDetails(ghMixer,&gmcd,MIXER_GETCONTROLDETAILSF_VALUE);

            lDiv =  (LONG)(max(gmvol[0].dwValue, gmvol[1].dwValue)
                                - 0);

            //
            // if we're pegged, don't try to calculate the balance.
            //
            if (gmvol[0].dwValue == 0 && gmvol[1].dwValue == 0)
                lBalance = glCachedBalance;
            else if (gmvol[0].dwValue == 0)
                lBalance = 32;
            else if (gmvol[1].dwValue == 0) 
                lBalance = -32;
            else if (lDiv > 0)
            {
                lBalance = (32 * ((LONG)gmvol[1].dwValue-(LONG)gmvol[0].dwValue))
                           / lDiv;
                //
                // we always lose precision doing this.
                //
                if (lBalance > 0) lBalance++;
                if (lBalance < 0) lBalance--;

                //if we lost precision above, we can get it back by checking
                //the previous value of our balance.  We're usually only off by
                //one if this is the result of a rounding error.  Otherwise,
                //we probably have a different balance because the user set it.
                if (((glCachedBalance - lBalance) == 1) ||
                    ((glCachedBalance - lBalance) == -1))
                {
                    lBalance = glCachedBalance;
                }
        
            }
            else
                lBalance = 0;
        }

        //save this balance setting so we can use it if we're pegged later
        glCachedBalance = lBalance;

        //
        // Recalc channels based on Balance vs. Volume
        //
        gmvol[0].dwValue = dwVol;
        gmvol[1].dwValue = dwVol;
                   
        if (lBalance > 0)
            gmvol[0].dwValue -= (lBalance * (LONG)(gmvol[1].dwValue-0))
                            / 32;
        else if (lBalance < 0)
            gmvol[1].dwValue -= (-lBalance * (LONG)(gmvol[0].dwValue-0))
                            / 32;

        gfInternalGenerated = TRUE;
        mixerSetControlDetails(ghMixer,&gmcd,MIXER_SETCONTROLDETAILSF_VALUE);
    }
}

// returns current mute state
BOOL GetMute(void)
{
    BOOL    fMute = FALSE;
    DWORD   cOldChannels = gmcd.cChannels;

    if (ghMixer && (gdwMuteID != (DWORD) -1))
    {
        gmcd.dwControlID = gdwMuteID;
        gmcd.cChannels = 1;
        mixerGetControlDetails(ghMixer,&gmcd,MIXER_GETCONTROLDETAILSF_VALUE);
        gmcd.cChannels = cOldChannels;

        fMute = (BOOL) gmvol[0].dwValue;
    }

    return fMute;
}

// Sets the mute state
void SetMute(BOOL fMute)
{
    DWORD   cOldChannels = gmcd.cChannels;

    if (ghMixer)
    {
        gmcd.dwControlID = gdwMuteID;
        gmcd.cChannels = 1;
        gmvol[0].dwValue = (DWORD) fMute;
        mixerSetControlDetails(ghMixer,&gmcd,MIXER_SETCONTROLDETAILSF_VALUE);
        gmcd.cChannels = cOldChannels;
    }
}

// Called to update the slider when the volume is changed externally
//
void UpdateVolumeSlider(HWND hWnd, DWORD dwLine)
{
    if ((ghMixer != NULL) && (gdwVolID != (DWORD) -1) && (dwLine == gdwVolID))
    {
        double volume = ((double) GetVolume() / (double) 0xFFFF) * ((double) VOLUME_TICS);
        SendMessage(GetDlgItem(hWnd, IDC_MASTERVOLUME), TBM_SETPOS, TRUE, (DWORD) volume );
    }
}



// Called in response to slider movement, computes new volume level and sets it
// it also controls the apply state (changed or not)
//
void MasterVolumeScroll(HWND hwnd, HWND hwndCtl, UINT code, int pos)
{
    DWORD dwVol = (DWORD) SendMessage(GetDlgItem(hwnd, IDC_MASTERVOLUME), TBM_GETPOS, 0, 0);

    dwVol = (DWORD) (((double) dwVol / (double) VOLUME_TICS) * (double) 0xFFFF);
    SetVolume(dwVol);

    if ((gdwPreviousVolume != dwVol) && !gfChanged)
    {
        gfChanged = TRUE;
        PropSheet_Changed(GetParent(hwnd),hwnd);
    }
}



// Call this function to configure to the current preferred device and reflect master volume
// settings on the slider
//
void DisplayVolumeControl(HWND hDlg)
{
    HWND hwndVol        = GetDlgItem(hDlg, IDC_MASTERVOLUME);

    SendMessage(hwndVol, TBM_SETTICFREQ, VOLUME_TICS / 10, 0);
    SendMessage(hwndVol, TBM_SETRANGE, FALSE, MAKELONG(0,VOLUME_TICS));

    EnableWindow(GetDlgItem(hDlg, IDC_MASTERVOLUME) , gfMasterVolume);
    EnableWindow(GetDlgItem(hDlg, IDC_VOLUME_LOW) , gfMasterVolume);
    EnableWindow(GetDlgItem(hDlg, IDC_VOLUME_HIGH) , gfMasterVolume);
    EnableWindow(GetDlgItem(hDlg, IDC_TASKBAR_VOLUME),gfMasterVolume);
    EnableWindow(GetDlgItem(hDlg, ID_MUTE), gfMasterMute);

    if (gfMasterVolume)
    {
        UpdateVolumeSlider(hDlg, gdwVolID);
    }

    InvalidateRect(GetDlgItem(hDlg, ID_MUTE), NULL, FALSE);
}


typedef struct
{
    WNDPROC     fnOldCallBack;
    HWND        hLastFocus;
    BOOL        fMouseInButton;
    int         lastState;

} BUTTONDATA, *PBUTTONDATA;

BUTTONDATA gBD;

typedef BOOL (PASCAL *TRACKPROC)(LPTRACKMOUSEEVENT);
TRACKPROC procTrackMouseEvent = NULL;


LRESULT CALLBACK MuteButtonProc(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
    LRESULT lResult;
    
    switch (iMsg)
    {
        case WM_SETFOCUS:
        {
            gBD.hLastFocus = (HWND) wParam;

            if (!gBD.fMouseInButton)
            {
                gBD.hLastFocus = NULL;
            }
        }

	    case WM_MOUSEMOVE :
	    {
	        if (!gBD.fMouseInButton)
	        {
		        if (procTrackMouseEvent)
                {
		            TRACKMOUSEEVENT tme;
                    gBD.fMouseInButton = TRUE;
		            tme.cbSize = sizeof(tme);
		            tme.dwFlags = TME_LEAVE;
		            tme.dwHoverTime = HOVER_DEFAULT;
		            tme.hwndTrack = hwnd;
                    procTrackMouseEvent(&tme);
		            InvalidateRect(hwnd,NULL,FALSE);
                } //end proctrackmouseevent is valid
	        }
	    }
	    break;

	    case WM_MOUSELEAVE :
	    {
	        gBD.fMouseInButton = FALSE;
	        InvalidateRect(hwnd,NULL,FALSE);
	    }
	    break;

    } //end switch

    lResult = CallWindowProc((WNDPROC)gBD.fnOldCallBack,hwnd,iMsg,wParam,lParam);

    return (lResult);
}


void InitMuteUserDraw(HWND hDlg)
{
    HWND        hMute = GetDlgItem(hDlg, ID_MUTE);

    //if TrackMouseEvent exists, use it
    HMODULE hUser = GetModuleHandle(TEXT("USER32"));
    if (hUser)
    {
        procTrackMouseEvent = (TRACKPROC)GetProcAddress(hUser,"TrackMouseEvent");
    }

    memset(&gBD,0,sizeof(gBD));
    gBD.fnOldCallBack = (WNDPROC)SetWindowLongPtr(hMute,GWLP_WNDPROC,(LONG_PTR)MuteButtonProc);
}


void DrawButton(HWND hDlg, LPDRAWITEMSTRUCT lpdis)
{
    RECT    muteRect;
    BOOL    fMute = GetMute();
    HICON   hIcon = NULL;

    if (lpdis->itemState & ODS_DISABLED)
    {
        DrawState(lpdis->hDC,NULL,NULL,(DWORD_PTR) hIconVolTrans,0,0,0,0,0,DST_ICON | DSS_DISABLED);  
    }
    else
    {
        if (fMute)
        {
            hIcon = hIconMute;

            if (gBD.fMouseInButton || lpdis->itemState & ODS_FOCUS)
            {
                if (lpdis->itemState & ODS_SELECTED)
                {
                    hIcon = hIconMuteDown;
                }
                else
                {
                    hIcon = hIconMuteUp;
                }
            }

        }
        else
        {
            hIcon = hIconVolume;

            if (gBD.fMouseInButton || lpdis->itemState & ODS_FOCUS)
            {
                if (lpdis->itemState & ODS_SELECTED)
                {
                    hIcon = hIconVolDown;
                }
                else
                {
                    hIcon = hIconVolUp;
                }
            }
        }

        if (hIcon)
        {
            DrawIconEx(lpdis->hDC,0,0,hIcon,0,0,0,NULL,DI_NORMAL);
        }
    }

    return;
}





void DeviceChange_Cleanup(void)
{
   if (ghDeviceEventContext) 
   {
       UnregisterDeviceNotification(ghDeviceEventContext);
       ghDeviceEventContext = 0;
   }
}


BOOL DeviceChange_GetHandle(DWORD dwMixerID, HANDLE *phDevice)
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

void DeviceChange_Init(HWND hWnd, DWORD dwMixerID)
{
	DEV_BROADCAST_HANDLE DevBrodHandle;
	HANDLE hMixerDevice=NULL;

	//If we had registered already for device notifications, unregister ourselves.
	DeviceChange_Cleanup();

	//If we get the device handle register for device notifications on it.
	if(DeviceChange_GetHandle(dwMixerID, &hMixerDevice))
	{
		memset(&DevBrodHandle, 0, sizeof(DEV_BROADCAST_HANDLE));

		DevBrodHandle.dbch_size = sizeof(DEV_BROADCAST_HANDLE);
		DevBrodHandle.dbch_devicetype = DBT_DEVTYP_HANDLE;
		DevBrodHandle.dbch_handle = hMixerDevice;

		ghDeviceEventContext = RegisterDeviceNotification(hWnd, &DevBrodHandle, DEVICE_NOTIFY_WINDOW_HANDLE);

		if(hMixerDevice)
		{
			CloseHandle(hMixerDevice);
			hMixerDevice = NULL;
		}
	}
}


// Handle the case where we need to dump mixer handle so PnP can get rid of a device
// We assume we will get the WINMM_DEVICECHANGE handle when the dust settles after a remove or add
// except for DEVICEQUERYREMOVEFAILED which will not generate that message.
//
void DeviceChange_Change(HWND hDlg, WPARAM wParam, LPARAM lParam)
{
	PDEV_BROADCAST_HANDLE bh = (PDEV_BROADCAST_HANDLE)lParam;

	if(!ghDeviceEventContext || !bh || bh->dbch_devicetype != DBT_DEVTYP_HANDLE)
	{
		return;
	}
	
    switch (wParam)
    {
	    case DBT_DEVICEQUERYREMOVE:     // Must free up Mixer if they are trying to remove the device           
        {
            if (ghMixer)
            {
                mixerClose((HMIXER) ghMixer);
                ghMixer = NULL;
            }
        }
        break;

	    case DBT_DEVICEQUERYREMOVEFAILED:   // Didn't happen, need to re-acquire mixer
        {
            InitVolume(hDlg,FALSE);
        }
        break; 
    }
}


void InitVolume(HWND hDlg, BOOL fFirst)
{
 
    if (fFirst)
    {
        FORWARD_WM_COMMAND(hDlg, ID_INIT, 0, 0, SendMessage);
    }
    else
    {
	    if (ghMixer)
	    {
    		mixerClose((HMIXER)ghMixer);
            ghMixer = NULL;
	    }
    }

    CheckForSndVolPresence();

    if ((g_iSndVolExists == SNDVOL_NOTPRESENT) || (waveOutGetNumDevs() < 1))
    {
        EnableWindow(GetDlgItem(hDlg,IDC_TASKBAR_VOLUME),FALSE);
    }

    if (GetTrayVolumeEnabled())
    {
        CheckDlgButton(hDlg, IDC_TASKBAR_VOLUME, TRUE);
    }

    MasterVolumeConfig(hDlg);
    DisplayVolumeControl(hDlg);

    gdwPreviousVolume = GetVolume();
    gfPreviousMute = GetMute();

    DeviceChange_Init(hDlg, guMixID);
 }


LRESULT CALLBACK SoundsTabProc(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
    if (iMsg == giVolDevChange)
    {
        InitVolume(ghWnd, FALSE);
    }
        
    return CallWindowProc(gfnVolPSProc,hwnd,iMsg,wParam,lParam);
}


void InitVolDeviceChange(HWND hDlg)
{
    gfnVolPSProc = (WNDPROC) SetWindowLongPtr(GetParent(hDlg),GWLP_WNDPROC,(LONG_PTR) SoundsTabProc);  
    giVolDevChange = RegisterWindowMessage(TEXT("winmm_devicechange"));
}

void UninitVolDeviceChange(HWND hDlg)
{
    SetWindowLongPtr(GetParent(hDlg),GWLP_WNDPROC,(LONG_PTR) gfnVolPSProc);  
}


void HandlePowerBroadcast(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
    switch (wParam)
    {
	    case PBT_APMQUERYSUSPEND:
        {
	        if (ghMixer)
	        {
    		    mixerClose((HMIXER)ghMixer);
                ghMixer = NULL;
	        }
        }
	    break;

	    case PBT_APMQUERYSUSPENDFAILED:
	    case PBT_APMRESUMESUSPEND:
        {
            InitVolume(hWnd,FALSE);
        }
	    break;
    }
}



/*
 ***************************************************************
 *  SoundDlg
 *
 *  Description:
 *        DialogProc for sound control panel applet.
 *
 *  Parameters:
 *   HWND        hDlg            window handle of dialog window
 *   UINT        uiMessage       message number
 *   WPARAM        wParam          message-dependent
 *   LPARAM        lParam          message-dependent
 *
 *  Returns:    BOOL
 *      TRUE if message has been processed, else FALSE
 *
 ***************************************************************
 */
BOOL CALLBACK  SoundDlg(HWND hDlg, UINT uMsg, WPARAM wParam,LPARAM lParam)
{
    NMHDR FAR       *lpnm;
    TCHAR           szBuf[MAXSTR];
    static BOOL     fClosingDlg = FALSE;
    PEVENT          pEvent;

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

                case PSN_RESET:
                    FORWARD_WM_COMMAND(hDlg, IDCANCEL, 0, 0, SendMessage);
                    break;

                case TVN_SELCHANGED:
                {
                    TV_ITEM tvi;
                    LPNM_TREEVIEW lpnmtv = (LPNM_TREEVIEW)lParam;

                    if (fClosingDlg || gfDeletingTree)
                        break;
                    if (gfnKBHookScheme)
                    {
                        UnhookWindowsHookEx(gfnKBHookScheme);
                        gfnKBHookScheme = NULL;
                        if (gfEditBoxChanged)
                        {
                            ghOldItem = lpnmtv->itemOld.hItem;
                            SendMessage(ghwndDlg, WM_NEWEVENTFILE, 0, 1L);
                            ghOldItem = NULL;
                        }
                    }

                    tvi = lpnmtv->itemNew;
                    if (tvi.lParam)
                    {
                        if (*((short NEAR *)tvi.lParam) == 2)
                        {
                            pEvent =  (PEVENT)tvi.lParam;
                            ShowSoundMapping(hDlg, pEvent);
                            SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR)tvi.lParam);
                        }
                        else
                        {
                            ShowSoundMapping(hDlg, NULL);
                            SetWindowLongPtr(hDlg, DWLP_USER, 0L);
                        }
                    }
                    else
                    {
                        ShowSoundMapping(hDlg, NULL);
                        SetWindowLongPtr(hDlg, DWLP_USER, 0L);
                    }
                    break;
                }

                case TVN_ITEMEXPANDING:
                {
                    LPNM_TREEVIEW lpnmtv = (LPNM_TREEVIEW)lParam;

                    if (lpnmtv->action == TVE_COLLAPSE)
                    {
                        SetWindowLongPtr(hDlg, DWLP_MSGRESULT, (LPARAM)(LRESULT)TRUE);
                        return TRUE;
                    }
                    break;
                }


            }
            break;

        case WM_INITDIALOG:
        {
            InitVolDeviceChange(hDlg);
            InitStringTable();
            InitMuteUserDraw(hDlg);

            ghDispBMP = ghIconBMP = NULL;
            giScheme = 0;
            ghWnd = hDlg;
            gfChanged = FALSE;
            gfNewScheme = FALSE;

            hIconVolume = (HICON) LoadImage(ghInstance,MAKEINTRESOURCE(IDI_VOLUME), IMAGE_ICON, 0, 0, LR_LOADMAP3DCOLORS);
            if (!hIconVolume)
                DPF("loadicon failed\n");
            hIconVolTrans = (HICON) LoadImage(ghInstance,MAKEINTRESOURCE(IDI_VOLTRANS), IMAGE_ICON, 0, 0, LR_LOADMAP3DCOLORS);
            if (!hIconVolTrans)
                DPF("loadicon failed\n");
            hIconVolUp = (HICON) LoadImage(ghInstance,MAKEINTRESOURCE(IDI_VOLUP), IMAGE_ICON, 0, 0, LR_LOADMAP3DCOLORS);
            if (!hIconVolUp)
                DPF("loadicon failed\n");
            hIconVolDown = (HICON) LoadImage(ghInstance,MAKEINTRESOURCE(IDI_VOLDOWN), IMAGE_ICON, 0, 0, LR_LOADMAP3DCOLORS);
            if (!hIconVolDown)
                DPF("loadicon failed\n");
            hIconMute = (HICON) LoadImage(ghInstance,MAKEINTRESOURCE(IDI_VOLMUTE), IMAGE_ICON, 0, 0, LR_LOADMAP3DCOLORS);
            if (!hIconMute)
                DPF("loadicon failed\n");            
            hIconMuteUp = (HICON) LoadImage(ghInstance,MAKEINTRESOURCE(IDI_VOLMUTEUP), IMAGE_ICON, 0, 0, LR_LOADMAP3DCOLORS);
            if (!hIconMuteUp)
                DPF("loadicon failed\n");            
            hIconMuteDown = (HICON) LoadImage(ghInstance,MAKEINTRESOURCE(IDI_VOLMUTEDOWN), IMAGE_ICON, 0, 0, LR_LOADMAP3DCOLORS);
            if (!hIconMuteDown)
                DPF("loadicon failed\n");            
             
            hBitmapStop = LoadBitmap(ghInstance, MAKEINTRESOURCE(IDB_STOP));
            if (!hBitmapStop)
                DPF("loadbitmap failed\n");
            hBitmapPlay = LoadBitmap(ghInstance, MAKEINTRESOURCE(IDB_PLAY));
            if (!hBitmapPlay)
                DPF("loadbitmap failed\n");

            SendMessage(GetDlgItem(hDlg, ID_PLAY), BM_SETIMAGE,  IMAGE_BITMAP, (LPARAM)hBitmapPlay);
            ShowSoundMapping(hDlg, NULL);
            gfSoundPlaying = FALSE;

            /* Determine if there is a wave device
             */
            FORWARD_WM_COMMAND(hDlg, ID_INIT, 0, 0, SendMessage);
            InitFileOpen(hDlg, &ofn);
            ghwndDlg = hDlg;
            DragAcceptFiles(hDlg, TRUE);
            gfSubClassedEditWindow = FALSE;
            fClosingDlg = FALSE;
            gfDeletingTree = FALSE;

            InitVolume(hDlg, TRUE);
       }
        break;

        case WM_DEVICECHANGE:
        {
            DeviceChange_Change(hDlg, wParam, lParam);
        }
        break;

        case WM_DESTROY:
        {
            DWORD i = 0;
            LPTSTR pszKey = NULL;

            UninitVolDeviceChange(hDlg);
            DeviceChange_Cleanup();

            if (ghMixer)
            {
                mixerClose((HMIXER) ghMixer);
                ghMixer = NULL;
            }

            fClosingDlg = TRUE;
            if (gfnKBHookScheme)
            {
                UnhookWindowsHookEx(gfnKBHookScheme);
                gfnKBHookScheme = NULL;
            }
            SoundCleanup(hDlg);
            
            //delete item data in tree
            ClearModules(hDlg,GetDlgItem(hDlg, IDC_EVENT_TREE),TRUE);

            //delete item data in combobox
            for (i = 0; i < ComboBox_GetCount(GetDlgItem(hDlg, CB_SCHEMES)); i++)
            {
                pszKey = (LPTSTR)ComboBox_GetItemData(GetDlgItem(hDlg, CB_SCHEMES), i);
                if (pszKey)
                {
                    //can't free a couple of these, as they point to static mem
                    if ((pszKey != aszDefault) && (pszKey != aszCurrent))
                    {
                        LocalFree(pszKey);
                    }
                }
            }

            break;
        }

        case WM_DRAWITEM:
        {
            DrawButton(hDlg, (LPDRAWITEMSTRUCT) lParam);
            return(1);
        }
        break;


        case WM_DROPFILES:
        {
            TV_HITTESTINFO ht;
            HWND hwndTree = GetDlgItem(hDlg, IDC_EVENT_TREE);

            DragQueryFile((HDROP)wParam, 0, szBuf, MAXSTR - 1);

            if (IsLink(szBuf, aszLnk))
                if (!ResolveLink(szBuf, szBuf, sizeof(szBuf)))
                    goto EndDrag;

            if (lstrcmpi((LPTSTR)(szBuf+lstrlen(szBuf)-4), cszWavExt))
                goto EndDrag;

            GetCursorPos((LPPOINT)&ht.pt);
            MapWindowPoints(NULL, hwndTree,(LPPOINT)&ht.pt, 2);
            TreeView_HitTest( hwndTree, &ht);
            if (ht.hItem && (ht.flags & TVHT_ONITEM))
            {
                TV_ITEM tvi;

                tvi.mask = TVIF_PARAM;
                   tvi.hItem = ht.hItem;
                   TreeView_GetItem(hwndTree, &tvi);

                if (*((short NEAR *)tvi.lParam) == 2)
                {
                    TreeView_SelectItem(hwndTree, ht.hItem);
                    pEvent =  (PEVENT)(tvi.lParam);
                    SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR)tvi.lParam);
                    SetFocus(hwndTree);
                }
            }
            pEvent = (PEVENT)(GetWindowLongPtr(hDlg, DWLP_USER));

            ChangeSoundMapping(hDlg, szBuf,pEvent);
            DragFinish((HDROP) wParam);
            break;
EndDrag:
            ErrorBox(hDlg, IDS_ISNOTSNDFILE, szBuf);
            DragFinish((HDROP) wParam);
            break;
        }
        case WM_NEWEVENTFILE:
        {
            DPF("*****WM_NEWEVENT\r\n");
            gfEditBoxChanged = FALSE;
            ComboBox_GetText(GetDlgItem(hDlg, IDC_SOUND_FILES), szBuf, sizeof(szBuf)/sizeof(TCHAR));
            pEvent = (PEVENT)(GetWindowLongPtr(hDlg, DWLP_USER));
            if (!lstrcmp (szBuf, gszNone))  // Selected "(None)" with keyboard?
            {
                lstrcpy(szBuf, gszNull);
                ChangeSoundMapping(hDlg, szBuf, pEvent);
                goto ReturnFocus;
            }

            if (TranslateDir(hDlg, szBuf))
            {
                ShowSoundMapping(hDlg, pEvent);
                goto ReturnFocus;
            }
            if (QualifyFileName((LPTSTR)szBuf, (LPTSTR)szBuf,    sizeof(szBuf), TRUE))
            {
                SetCurDir(hDlg, szBuf, TRUE, TRUE);
                ChangeSoundMapping(hDlg, szBuf,pEvent);
            }
            else
            {
                if (lParam)
                {
                    ErrorBox(hDlg, IDS_INVALIDFILE, NULL);
                    ShowSoundMapping(hDlg, pEvent);
                    goto ReturnFocus;
                }
                if (DisplayMessage(hDlg, IDS_NOSNDFILETITLE, IDS_INVALIDFILEQUERY, MB_YESNO) == IDYES)
                {
                    ShowSoundMapping(hDlg, pEvent);
                }
                else
                {
                    SetWindowLongPtr(hDlg, DWLP_MSGRESULT, (LPARAM)(LRESULT)TRUE);
                    return TRUE;
                }
            }
ReturnFocus:
            SetFocus(GetDlgItem(hDlg,IDC_EVENT_TREE));
            SetWindowLongPtr(hDlg, DWLP_MSGRESULT, (LPARAM)(LRESULT)FALSE);
            return TRUE;
        }

        case WM_RESTOREEVENTFILE:
        {
            DPF("*****WM_RESTOREEVENT\r\n");
            pEvent = (PEVENT)(GetWindowLongPtr(hDlg, DWLP_USER));
            ShowSoundMapping(hDlg, pEvent);
            if (lParam == 0) //Don't keep focus
                SetFocus(GetDlgItem(hDlg,IDC_EVENT_TREE));
            break;
        }


        case WM_CONTEXTMENU:
            WinHelp((HWND)wParam, NULL, HELP_CONTEXTMENU,
                                            (UINT_PTR)(LPTSTR)aKeyWordIds);
            break;

        case WM_HELP:
            WinHelp(((LPHELPINFO)lParam)->hItemHandle, NULL, HELP_WM_HELP
                                    , (UINT_PTR)(LPTSTR)aKeyWordIds);
            break;

        case MM_WOM_DONE:
        {
            HWND hwndFocus = GetFocus();
            HWND hwndPlay =  GetDlgItem(hDlg, ID_PLAY);

            gfSoundPlaying = FALSE;
            SendMessage(hwndPlay, BM_SETIMAGE,  IMAGE_BITMAP, (LPARAM)hBitmapPlay);

            if (ghse)
            {
                soundOnDone(ghse);
                soundClose(ghse);
                ghse = NULL;
            }
            pEvent = (PEVENT)(GetWindowLongPtr(hDlg, DWLP_USER));
            ShowSoundMapping(hDlg, pEvent);

            if (hwndFocus == hwndPlay)
                if (IsWindowEnabled(hwndPlay))
                    SetFocus(hwndPlay);
                else
                    SetFocus(GetDlgItem(hDlg, IDC_EVENT_TREE));
            break;
        }

        case WM_WININICHANGE:
        case WM_DISPLAYCHANGE :
        {
            SendDlgItemMessage(hDlg,IDC_MASTERVOLUME,uMsg,wParam,lParam);
        }
        break;

        case WM_SYSCOLORCHANGE:
        {
            SendDlgItemMessage(hDlg, ID_DISPFRAME, WM_SYSCOLORCHANGE, 0, 0l);

            //reset the mute button icons to proper colors
            DeleteObject(hIconVolume);
            DeleteObject(hIconVolTrans);
            DeleteObject(hIconVolUp);
            DeleteObject(hIconVolDown);
            DeleteObject(hIconMute);
            DeleteObject(hIconMuteUp);
            DeleteObject(hIconMuteDown);

            hIconVolume = (HICON) LoadImage(ghInstance,MAKEINTRESOURCE(IDI_VOLUME), IMAGE_ICON, 0, 0, LR_LOADMAP3DCOLORS);
            if (!hIconVolume)
                DPF("loadicon failed\n");
            hIconVolTrans = (HICON) LoadImage(ghInstance,MAKEINTRESOURCE(IDI_VOLTRANS), IMAGE_ICON, 0, 0, LR_LOADMAP3DCOLORS);
            if (!hIconVolTrans)
                DPF("loadicon failed\n");
            hIconVolUp = (HICON) LoadImage(ghInstance,MAKEINTRESOURCE(IDI_VOLUP), IMAGE_ICON, 0, 0, LR_LOADMAP3DCOLORS);
            if (!hIconVolUp)
                DPF("loadicon failed\n");
            hIconVolDown = (HICON) LoadImage(ghInstance,MAKEINTRESOURCE(IDI_VOLDOWN), IMAGE_ICON, 0, 0, LR_LOADMAP3DCOLORS);
            if (!hIconVolDown)
                DPF("loadicon failed\n");
            hIconMute = (HICON) LoadImage(ghInstance,MAKEINTRESOURCE(IDI_VOLMUTE), IMAGE_ICON, 0, 0, LR_LOADMAP3DCOLORS);
            if (!hIconMute)
                DPF("loadicon failed\n");            
            hIconMuteUp = (HICON) LoadImage(ghInstance,MAKEINTRESOURCE(IDI_VOLMUTEUP), IMAGE_ICON, 0, 0, LR_LOADMAP3DCOLORS);
            if (!hIconMuteUp)
                DPF("loadicon failed\n");            
            hIconMuteDown = (HICON) LoadImage(ghInstance,MAKEINTRESOURCE(IDI_VOLMUTEDOWN), IMAGE_ICON, 0, 0, LR_LOADMAP3DCOLORS);
            if (!hIconMuteDown)
                DPF("loadicon failed\n");            

            InvalidateRect(GetDlgItem(hDlg, ID_MUTE), NULL, FALSE);
        }
        break;

        case WM_QUERYNEWPALETTE:
        {
            HDC hDC;
            HPALETTE hOldPal;

            if (ghPal)
            {
                HWND hwndDF =  GetDlgItem(hDlg, ID_DISPFRAME);

                hDC = GetDC(hwndDF);
                hOldPal = SelectPalette(hDC, ghPal, 0);
                RealizePalette(hDC);
                InvalidateRect(hwndDF, (LPRECT)0, 1);
                SelectPalette(hDC, hOldPal, 0);
                ReleaseDC(hwndDF, hDC);
            }
            break;
        }

        case WM_PALETTECHANGED:
        {
            HDC hDC;
            HPALETTE hOldPal;

            if (wParam != (WPARAM)hDlg && wParam != (WPARAM)GetDlgItem(hDlg, ID_DISPFRAME)
                                                                    && ghPal)
            {
                HWND hwndDF =  GetDlgItem(hDlg, ID_DISPFRAME);

                hDC = GetDC(hwndDF);
                hOldPal = SelectPalette(hDC, ghPal, 0);
                RealizePalette(hDC);
                InvalidateRect(hwndDF, (LPRECT)0, 1);
                SelectPalette(hDC, hOldPal, 0);
                ReleaseDC(hwndDF, hDC);
            }
            break;
        }

        case WM_COMMAND:
            HANDLE_WM_COMMAND(hDlg, wParam, lParam, DoCommand);
            break;

        case WM_POWERBROADCAST:
        {
            HandlePowerBroadcast(hDlg,wParam, lParam);
        }
        break;

	    case WM_HSCROLL:
        {
	        HANDLE_WM_HSCROLL(hDlg, wParam, lParam, MasterVolumeScroll);
	    }
        break;
        
        case MM_MIXM_LINE_CHANGE:
        case MM_MIXM_CONTROL_CHANGE:
        {
            if (!gfInternalGenerated)
            {
                DisplayVolumeControl(hDlg);
            }

            gfInternalGenerated = FALSE;
        }
        break;

        default:
            break;
    }
    return FALSE;
}



/*
 ***************************************************************
 *  doCommand
 *
 *  Description:
 *        Processes Control commands for main sound
 *      control panel dialog.
 *
 *  Parameters:
 *        HWND    hDlg  -   window handle of dialog window
 *        int        id     - Message ID
 *        HWND    hwndCtl - Handle of window control
 *        UINT    codeNotify - Notification code for window
 *
 *  Returns:    BOOL
 *      TRUE if message has been processed, else FALSE
 *
 ***************************************************************
 */
BOOL PASCAL DoCommand(HWND hDlg, int id, HWND hwndCtl, UINT codeNotify)
{
    WAVEOUTCAPS woCaps;
    TCHAR        szBuf[MAXSTR];
    LPTSTR        pszKey;
    int         iIndex;
    HCURSOR     hcur;
    HWND        hWndC = GetDlgItem(hDlg, CB_SCHEMES);
    HWND        hWndF = GetDlgItem(hDlg, IDC_SOUND_FILES);
    HWND        hwndTree = GetDlgItem(hDlg, IDC_EVENT_TREE);
    PEVENT        pEvent;
    static      BOOL fSchemeCBDroppedDown = FALSE;
    static      BOOL fFilesCBDroppedDown = FALSE;
    static      BOOL fSavingPrevScheme = FALSE;

    switch (id)
    {
        case ID_APPLY:
        {
            gdwPreviousVolume = GetVolume();
            gfPreviousMute = GetMute();
            DisplayVolumeControl(hDlg);

            EndSound(&ghse);
            if (!gfChanged)
                break;
            hcur = SetCursor(LoadCursor(NULL,IDC_WAIT));
            if (gfNewScheme)
            {
                pszKey = (LPTSTR)ComboBox_GetItemData(hWndC, NONE_ENTRY);
                if (lstrcmpi(pszKey, aszCurrent))
                {
                    ComboBox_InsertString(hWndC, NONE_ENTRY, gszNull);
                    ComboBox_SetItemData(hWndC, NONE_ENTRY, aszCurrent);
                    ComboBox_SetCurSel(hWndC, NONE_ENTRY);
                    giScheme = NONE_ENTRY;
                }
                gfNewScheme = FALSE;
            }
            iIndex = ComboBox_GetCurSel(hWndC);
            if (iIndex != CB_ERR)
            {
                pszKey = (LPTSTR)ComboBox_GetItemData(hWndC, iIndex);
                if (pszKey)
                {
                    RegNewScheme(hDlg, (LPTSTR)aszCurrent, NULL, FALSE);
                }
                RegSetDefault(pszKey);
            }

            SetTrayVolumeEnabled(Button_GetCheck(GetDlgItem(hDlg, IDC_TASKBAR_VOLUME)));

            gfChanged = FALSE;
            SetCursor(hcur);

            return TRUE;
        }
        break;

        case IDOK:
        {
            EndSound(&ghse);
            break;
        }
        case IDCANCEL:
        {
            SetVolume(gdwPreviousVolume);
            SetMute(gfPreviousMute);

            EndSound(&ghse);
            WinHelp(hDlg, gszWindowsHlp, HELP_QUIT, 0L);
            break;
        }
        case ID_INIT:
            hcur = SetCursor(LoadCursor(NULL,IDC_WAIT));
            gfWaveExists = waveOutGetNumDevs() > 0 &&
                            (waveOutGetDevCaps(WAVE_MAPPER,&woCaps,sizeof(woCaps)) == 0) &&
                                                    woCaps.dwFormats != 0L;
            ComboBox_ResetContent(hWndC);
            ComboBox_SetText(hWndF, gszNone);
            SendDlgItemMessage(hDlg, ID_DISPFRAME, DF_PM_SETBITMAP, 0, 0L);
            InitDialog(hDlg);
            giScheme = ComboBox_GetCurSel(hWndC);
            ghWnd = hDlg;
            SetCursor(hcur);
            break;

        case ID_MUTE:
        {
            BOOL    fMute = !GetMute();

            if (gBD.hLastFocus)
            {
                SetFocus(gBD.hLastFocus);
                gBD.hLastFocus = NULL;
            }

            SetMute(fMute);

            if ((gfPreviousMute != fMute) && !gfChanged)
            {
                gfChanged = TRUE;
                PropSheet_Changed(GetParent(hDlg),hDlg);
            }
        }
        break;

        case ID_BROWSE:
            aszFileName[0] = aszPath[0] = TEXT('\0');
            pEvent = (PEVENT)(GetWindowLongPtr(hDlg, DWLP_USER));

            wsprintf((LPTSTR)aszBrowse, (LPTSTR)aszBrowseStr, (LPTSTR)pEvent->pszEventLabel);
            if (GetOpenFileName(&ofn))
            {
                SetCurDir(hDlg, ofn.lpstrFile,TRUE, TRUE);
                ChangeSoundMapping(hDlg, ofn.lpstrFile, pEvent);
            }
            break;

        case ID_PLAY:
        {
            if (!gfSoundPlaying)
            {
                pEvent = (PEVENT)(GetWindowLongPtr(hDlg, DWLP_USER));
                if (pEvent)
                {
                    PlaySoundFile(hDlg, pEvent->pszPath);
                    SendMessage(GetDlgItem(hDlg, ID_PLAY), BM_SETIMAGE,  IMAGE_BITMAP, (LPARAM)hBitmapStop);
                    gfSoundPlaying = TRUE;
                }
            }
            else
            {
                SendMessage(GetDlgItem(hDlg, ID_PLAY), BM_SETIMAGE,  IMAGE_BITMAP, (LPARAM)hBitmapPlay);
                gfSoundPlaying = FALSE;

                EndSound(&ghse);
                SetFocus(GetDlgItem(hDlg, ID_PLAY));
            }
        }
        break;

        case IDC_TASKBAR_VOLUME:
            if (g_iSndVolExists == SNDVOL_NOTCHECKED)
            {
                CheckForSndVolPresence();
            }
            if (Button_GetCheck(GetDlgItem(hDlg, IDC_TASKBAR_VOLUME)) && g_iSndVolExists == SNDVOL_NOTPRESENT)
            {
                CheckDlgButton(hDlg, IDC_TASKBAR_VOLUME, FALSE);
                ErrorBox(hDlg, IDS_NOSNDVOL, NULL);
                g_iSndVolExists = SNDVOL_NOTCHECKED;
            }
            else
            {
                PropSheet_Changed(GetParent(hDlg),hDlg);
                gfChanged = TRUE;
            }
            break;


        case CB_SCHEMES:
            switch (codeNotify)
            {
            case CBN_DROPDOWN:
                fSchemeCBDroppedDown = TRUE;
                break;

            case CBN_CLOSEUP:
                fSchemeCBDroppedDown = FALSE;
                break;

            case CBN_SELCHANGE:
                if (fSchemeCBDroppedDown)
                    break;
            case CBN_SELENDOK:
                if (fSavingPrevScheme)
                    break;
                iIndex = ComboBox_GetCurSel(hWndC);
                if (iIndex != giScheme)
                {
                    TCHAR szScheme[MAXSTR];
                    BOOL fDeletedCurrent = FALSE;

                    ComboBox_GetLBText(hWndC, iIndex, (LPTSTR)szScheme);
                    if (giScheme == NONE_ENTRY)
                    {
                        pszKey = (LPTSTR)ComboBox_GetItemData(hWndC, giScheme);
                        if (!lstrcmpi(pszKey, aszCurrent))
                        {
                            int i;

                            i = DisplayMessage(hDlg, IDS_SAVESCHEME, IDS_SCHEMENOTSAVED, MB_YESNOCANCEL);
                            if (i == IDCANCEL)
                            {
                                ComboBox_SetCurSel(hWndC, giScheme);
                                break;
                            }
                            if (i == IDYES)
                            {
                                fSavingPrevScheme = TRUE;
                                if (DialogBoxParam(ghInstance, MAKEINTRESOURCE(SAVESCHEMEDLG),
                                    GetParent(hDlg), SaveSchemeDlg, (LPARAM)(LPTSTR)gszNull))
                                {
                                    fSavingPrevScheme = FALSE;
                                    ComboBox_SetCurSel(hWndC, iIndex);
                                }
                                else
                                {
                                    fSavingPrevScheme = FALSE;
                                    ComboBox_SetCurSel(hWndC, NONE_ENTRY);
                                    break;
                                }
                            }
                        }
                    }
                    pszKey = (LPTSTR)ComboBox_GetItemData(hWndC, NONE_ENTRY);
                    if (!lstrcmpi(pszKey, aszCurrent))
                    {
                        ComboBox_DeleteString(hWndC, NONE_ENTRY);
                        fDeletedCurrent = TRUE;
                    }
                    iIndex = ComboBox_FindStringExact(hWndC, 0, szScheme);
                    pszKey = (LPTSTR)ComboBox_GetItemData(hWndC, iIndex);

                    giScheme = iIndex;
                    EndSound(&ghse);
                    ShowSoundMapping(hDlg, NULL);
                    hcur = SetCursor(LoadCursor(NULL,IDC_WAIT));
                    if (LoadModules(hDlg, pszKey))
                    {
                        EnableWindow(GetDlgItem(hDlg, ID_SAVE_SCHEME), TRUE);
                    }
                    SetCursor(hcur);
                    if (!lstrcmpi((LPTSTR)pszKey, aszDefault) || !lstrcmpi((LPTSTR)pszKey, gszNullScheme))
                        EnableWindow(GetDlgItem(hDlg, ID_REMOVE_SCHEME),
                                                                    FALSE);
                    else
                        EnableWindow(GetDlgItem(hDlg, ID_REMOVE_SCHEME),TRUE);
                    gfChanged = TRUE;
                    gfNewScheme = FALSE;
                    if (fDeletedCurrent)
                        ComboBox_SetCurSel(hWndC, giScheme);
                    PropSheet_Changed(GetParent(hDlg),hDlg);
                }
                break;
            }
            break;

        case IDC_SOUND_FILES:
            switch (codeNotify)
            {
            case  CBN_SETFOCUS:
            {
                if (!gfSubClassedEditWindow)
                {
                    HWND hwndEdit = GetFocus();

                    SubClassEditWindow(hwndEdit);
                    gfSubClassedEditWindow = TRUE;
                    SetFocus(GetDlgItem(hDlg, IDC_EVENT_TREE)); //This setfocus hack is needed
                    SetFocus(hwndEdit);                         //to activate the hook.
                }
            }
            break;

            case CBN_EDITCHANGE:
                DPF("CBN_EDITCHANGE \r\n");
                if (!gfEditBoxChanged)
                    gfEditBoxChanged = TRUE;
                break;

            case CBN_DROPDOWN:
                DPF("CBN_DD\r\n");
                fFilesCBDroppedDown = TRUE;
                break;

            case CBN_CLOSEUP:
                DPF("CBN_CLOSEUP\r\n");
                fFilesCBDroppedDown = FALSE;
                break;

            case CBN_SELCHANGE:
                DPF("CBN_SELCHANGE\r\n");
                if (fFilesCBDroppedDown)
                    break;
            case CBN_SELENDOK:
            {
                HWND hwndS = GetDlgItem(hDlg, IDC_SOUND_FILES);
                DPF("CBN_SELENDOK\r\n");
                iIndex = ComboBox_GetCurSel(hwndS);
                if (iIndex >= 0)
                {
                    TCHAR szFile[MAX_PATH];

                    if (gfEditBoxChanged)
                        gfEditBoxChanged = FALSE;
                    lstrcpy(szFile, gszCurDir);
                    lstrcat(szFile, cszSlash);
                    ComboBox_GetLBText(hwndS, iIndex, (LPTSTR)(szFile + lstrlen(szFile)));
                    if (iIndex)
                    {
                        if (gfNukeExt)
                            AddExt(szFile, cszWavExt);
                    }
                    else
                    {
                        TCHAR szTmp[64];

                        ComboBox_GetText(hwndS, szTmp, sizeof(szTmp)/sizeof(TCHAR));
                        iIndex = ComboBox_FindStringExact(hwndS, 0, szTmp);
                        if (iIndex == CB_ERR)
                        {
                            if (DisplayMessage(hDlg, IDS_SOUND, IDS_NONECHOSEN, MB_YESNO) == IDNO)
                            {
                                PostMessage(ghwndDlg, WM_RESTOREEVENTFILE, 0, 1L);
                                break;
                            }
                        }
                        lstrcpy(szFile, gszNull);
                    }
                    pEvent = (PEVENT)(GetWindowLongPtr(hDlg, DWLP_USER));
                    ChangeSoundMapping(hDlg, szFile, pEvent);
                    SetFocus(GetDlgItem(hDlg, ID_PLAY));
                }
                break;
            }

        }
        break;

        case ID_DETAILS:
        {
            TCHAR szCaption[MAX_PATH];

            pEvent = (PEVENT)(GetWindowLongPtr(hDlg, DWLP_USER));
            lstrcpy(szCaption,(LPTSTR)pEvent->pszPath);
            NiceName(szCaption, TRUE);
            mmpsh_ShowFileDetails((LPTSTR)szCaption, hDlg, (LPTSTR)pEvent->pszPath, MT_WAVE);
            break;
        }

        case ID_SAVE_SCHEME:
            // Retrieve current scheme and pass it to the savescheme dialog.
            iIndex = ComboBox_GetCurSel(hWndC);
            if (iIndex != CB_ERR)
            {
                ComboBox_GetLBText(hWndC, iIndex, szBuf);
                if (DialogBoxParam(ghInstance, MAKEINTRESOURCE(SAVESCHEMEDLG),
                    GetParent(hDlg), SaveSchemeDlg, (LPARAM)(LPTSTR)szBuf))
                {
                    pszKey = (LPTSTR)ComboBox_GetItemData(hWndC, NONE_ENTRY);
                    if (!lstrcmpi(pszKey, aszCurrent))
                    {
                        ComboBox_DeleteString(hWndC, NONE_ENTRY);
                    }
                }
            }
            break;

        case ID_REMOVE_SCHEME:
            if (RemoveScheme(hDlg))
            {
                iIndex = ComboBox_FindStringExact(hWndC, 0, aszNullSchemeLabel);
                ComboBox_SetCurSel(hWndC, iIndex);
                giScheme = -1;
                FORWARD_WM_COMMAND(hDlg, CB_SCHEMES, hWndC, CBN_SELENDOK,SendMessage);
            }
            SetFocus(GetDlgItem(hDlg, CB_SCHEMES));
            break;

    }
    return FALSE;
}

void InitImageList(HWND hwndTree)
{
    HICON hIcon;
    int  cxMiniIcon;
    int  cyMiniIcon;

    if (hSndImagelist)
    {
        TreeView_SetImageList(hwndTree, NULL, TVSIL_NORMAL);
        ImageList_Destroy(hSndImagelist);
        hSndImagelist = NULL;
    }
    cxMiniIcon = GetSystemMetrics(SM_CXSMICON);
    cyMiniIcon = GetSystemMetrics(SM_CYSMICON);

    hSndImagelist = ImageList_Create(cxMiniIcon,cyMiniIcon, TRUE, 4, 2);
    if (!hSndImagelist)
        return;

    hIcon = LoadImage(ghInstance, MAKEINTRESOURCE(IDI_PROGRAM),IMAGE_ICON,cxMiniIcon,cyMiniIcon,LR_DEFAULTCOLOR);
    ImageList_AddIcon(hSndImagelist, hIcon);
    DestroyIcon(hIcon);
    hIcon = LoadImage(ghInstance, MAKEINTRESOURCE(IDI_AUDIO),IMAGE_ICON,cxMiniIcon,cyMiniIcon,LR_DEFAULTCOLOR);
    ImageList_AddIcon(hSndImagelist, hIcon);
    DestroyIcon(hIcon);
    hIcon = LoadImage(ghInstance, MAKEINTRESOURCE(IDI_BLANK),IMAGE_ICON,cxMiniIcon,cyMiniIcon,LR_DEFAULTCOLOR);
    ImageList_AddIcon(hSndImagelist, hIcon);
    DestroyIcon(hIcon);
    TreeView_SetImageList(hwndTree, hSndImagelist, TVSIL_NORMAL);

}

/*
 ***************************************************************
 *  InitDialog
 *
 * Description:
 *        Reads the current event names and mappings from  reg.db
 *
 *        Each entry in the [reg.db] section is in this form:
 *
 *        AppEvents
 *            |
 *            |___Schemes  = <SchemeKey>
 *                    |
 *                    |______Names
 *                    |         |
 *                    |         |______SchemeKey = <Name>
 *                    |
 *                    |______Apps
 *                             |
 *                             |______Module
 *                                      |
 *                                      |_____Event
 *                                             |
 *                                             |_____SchemeKey = <Path\filename>
 *                                                     |
 *                                                     |____Active = <1\0
 *
 *        The Module, Event and the file label are displayed in the
 *        comboboxes.
 *
 * Parameters:
 *      HWND hDlg - parent window.
 *
 * Return Value: BOOL
 *        True if entire initialization is ok.
 *
 ***************************************************************
 */
BOOL PASCAL InitDialog(HWND hDlg)
{
    TCHAR     szDefKey[MAXSTR];
    TCHAR     szScheme[MAXSTR];
    TCHAR     szLabel[MAXSTR];
    int      iVal;
    int         i;
    int      cAdded;
    HWND     hWndC;
    LONG     cbSize;
    HKEY     hkNames;
    HWND        hwndTree = GetDlgItem(hDlg, IDC_EVENT_TREE);
    hWndC = GetDlgItem(hDlg, CB_SCHEMES);

    InitImageList(hwndTree);

    EnableWindow(GetDlgItem(hDlg, ID_SAVE_SCHEME), FALSE);
    EnableWindow(GetDlgItem(hDlg, ID_REMOVE_SCHEME), FALSE);
    EnableWindow(GetDlgItem(hDlg, ID_PLAY), FALSE);
    EnableWindow(GetDlgItem(hDlg, ID_BROWSE), FALSE);
    EnableWindow(GetDlgItem(hDlg, IDC_SOUND_FILES), FALSE);
    EnableWindow(GetDlgItem(hDlg, IDC_STATIC_NAME), FALSE);
    EnableWindow(GetDlgItem(hDlg, IDC_STATIC_PREVIEW), FALSE);
    EnableWindow(GetDlgItem(hDlg, ID_DETAILS), FALSE);

    SetCurDir(hDlg, gszMediaDir, FALSE, TRUE);

    if (RegOpenKey(HKEY_CURRENT_USER, aszNames, &hkNames) != ERROR_SUCCESS)
        DPF("Failed to open aszNames\n");
    else
        DPF("Opened HKEY_CURRENT_USERS\n");
    cAdded = 0;
    for (i = 0; !RegEnumKey(hkNames, i, szScheme, sizeof(szScheme)/sizeof(TCHAR)); i++)
    {
            // Don't add the windows default key yet
        if (lstrcmpi(szScheme, aszDefault))
        {
            cbSize = sizeof(szLabel);
            if ((RegQueryValue(hkNames, szScheme, szLabel, &cbSize) != ERROR_SUCCESS) || (cbSize < 2))
                lstrcpy(szLabel, szScheme);
            if (!lstrcmpi(szScheme, gszNullScheme))
                lstrcpy(aszNullSchemeLabel, szLabel);
            ++cAdded;
            AddScheme(hWndC, szLabel, szScheme, FALSE, 0);
        }
    }
    // Add the windows default key in the second position in the listbox
    cbSize = sizeof(szLabel);
    if ((RegQueryValue(hkNames, aszDefault, szLabel, &cbSize) != ERROR_SUCCESS) || (cbSize < 2))
    {
        LoadString(ghInstance, IDS_WINDOWSDEFAULT, szLabel, MAXSTR);
        if (RegSetValue(hkNames, aszDefault, REG_SZ, szLabel, 0) != ERROR_SUCCESS)
            DPF("Failed to add printable name for default\n");
    }

    if (cAdded == 0)
       AddScheme(hWndC, szLabel, (LPTSTR)aszDefault, TRUE, 0);
    else
       AddScheme(hWndC, szLabel, (LPTSTR)aszDefault, TRUE, WINDOWS_DEFAULTENTRY);

    cbSize = sizeof(szDefKey);
    if ((RegQueryValue(HKEY_CURRENT_USER, aszDefaultScheme, szDefKey,
                                &cbSize) != ERROR_SUCCESS) || (cbSize < 2))
    {
        ComboBox_SetCurSel(hWndC, 0);
        DPF("No default scheme found\n");
    }
    else
    {
        if (!lstrcmpi(szDefKey, aszCurrent))
        {
            ComboBox_InsertString(hWndC, NONE_ENTRY, gszNull);
            ComboBox_SetItemData(hWndC, NONE_ENTRY, aszCurrent);
            iVal = NONE_ENTRY;
            ComboBox_SetCurSel(hWndC, iVal);
        }
        else
        {
            cbSize = sizeof(szLabel);
            if ((RegQueryValue(hkNames, szDefKey, szLabel, &cbSize) != ERROR_SUCCESS) || (cbSize < 2))
            {
                DPF("No Name for default scheme key %s found\n", (LPTSTR)szDefKey);
                lstrcpy(szLabel, szDefKey);
            }

            if ((iVal = ComboBox_FindStringExact(hWndC, 0, szLabel)) != CB_ERR)
                ComboBox_SetCurSel(hWndC, iVal);
            else
                if (lstrcmpi(aszDefault, szDefKey))
                    ComboBox_SetCurSel(hWndC, iVal);
                else
                {
                    iVal = ComboBox_GetCount(hWndC);
                    AddScheme(hWndC, szLabel, szDefKey, TRUE, iVal);
                    ComboBox_SetCurSel(hWndC, iVal);
                }
        }
        giScheme = iVal;        //setting the current global scheme;
        if (LoadModules(hDlg, (LPTSTR)aszCurrent))
        {
            EnableWindow(GetDlgItem(hDlg, ID_SAVE_SCHEME), TRUE);
        }
        else
        {
            ClearModules(hDlg,  hwndTree, TRUE);
            ComboBox_SetCurSel(hWndC, 0);
            DPF("LoadModules failed\n");
            RegCloseKey(hkNames);
            return FALSE;
        }

        if (!lstrcmpi(szDefKey, aszDefault))
            EnableWindow(GetDlgItem(hDlg, ID_REMOVE_SCHEME), FALSE);
        else
            EnableWindow(GetDlgItem(hDlg, ID_REMOVE_SCHEME), TRUE);
//        DPF("Finished doing init\n");
    }
    RegCloseKey(hkNames);
    return TRUE;
}


const static DWORD aOpenHelpIDs[] = {  // Context Help IDs
    IDC_STATIC_PREVIEW, IDH_EVENT_BROWSE_PREVIEW,
    ID_PLAY,            IDH_EVENT_PLAY,
    ID_STOP,            IDH_EVENT_STOP,

    0, 0
};

INT_PTR CALLBACK OpenDlgHook(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    static HSOUND hse;

    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            TCHAR szOK[16];
            LPTSTR   lpszFile;

            // lParam is lpOFN
            DPF("****WM_INITDIALOG in HOOK **** \r\n");
            LoadString(ghInstance, IDS_OK, szOK, sizeof(szOK)/sizeof(TCHAR));
            SetDlgItemText(GetParent(hDlg), IDOK, szOK);
            hse = NULL;

            if (gfWaveExists)
            {
                HWND hwndPlay = GetDlgItem(hDlg, ID_PLAY);
                HWND hwndStop = GetDlgItem(hDlg, ID_STOP);

                SendMessage(hwndStop, BM_SETIMAGE,  IMAGE_BITMAP, (LPARAM)hBitmapStop);
                SendMessage(hwndPlay, BM_SETIMAGE,  IMAGE_BITMAP, (LPARAM)hBitmapPlay);
                EnableWindow(hwndStop, FALSE);
                EnableWindow(hwndPlay, FALSE);

                lpszFile = (LPTSTR)LocalAlloc(LPTR, MAX_PATH+sizeof(TCHAR));
                SetWindowLongPtr(hDlg, DWLP_USER, (LPARAM)lpszFile);
            }
            break;
        }

        case WM_HELP:
            WinHelp((HWND)((LPHELPINFO) lParam)->hItemHandle, NULL,
                HELP_WM_HELP, (UINT_PTR)(LPTSTR) aOpenHelpIDs);
            break;

        case WM_CONTEXTMENU:
            WinHelp((HWND) wParam, NULL, HELP_CONTEXTMENU,
                (UINT_PTR)(LPVOID) aOpenHelpIDs);
            break;

        case WM_COMMAND:
            if (!gfWaveExists)
                break;
            switch (GET_WM_COMMAND_ID(wParam, lParam))
            {
                case ID_PLAY:
                {
                    LPTSTR lpszFile = (LPTSTR)GetWindowLongPtr(hDlg, DWLP_USER);
                    MMRESULT err = MMSYSERR_NOERROR;

                    DPF("*****ID_PLAY in Dlg Hook ***\r\n");
                    if((soundOpen(lpszFile, hDlg, &hse) != MMSYSERR_NOERROR) || ((err = soundPlay(hse)) != MMSYSERR_NOERROR))
                    {
                        if (err == (MMRESULT)MMSYSERR_NOERROR || err != (MMRESULT)MMSYSERR_ALLOCATED)
                            ErrorBox(hDlg, IDS_ERRORFILEPLAY, lpszFile);
                        else
                            ErrorBox(hDlg, IDS_ERRORDEVBUSY, lpszFile);
                        hse = NULL;
                    }
                    else
                    {
                        EnableWindow(GetDlgItem(hDlg, ID_PLAY), FALSE);
                        EnableWindow(GetDlgItem(hDlg, ID_STOP), TRUE);
                    }
                    break;
                }
                case ID_STOP:
                {
                    DPF("*****ID_STOP in Dlg Hook ***\r\n");
                    EndSound(&hse);
                    EnableWindow(GetDlgItem(hDlg, ID_STOP), FALSE);
                    EnableWindow(GetDlgItem(hDlg, ID_PLAY), TRUE);

                    break;
                }
                default:
                    return(FALSE);
            }
            break;

        case MM_WOM_DONE:
            EnableWindow(GetDlgItem(hDlg, ID_STOP), FALSE);
            if (hse)
            {
                soundOnDone(hse);
                soundClose(hse);
                hse = NULL;
            }
            EnableWindow(GetDlgItem(hDlg, ID_PLAY), TRUE);
            break;

        case WM_DESTROY:
        {
            LPTSTR lpszFile;

            if (!gfWaveExists)
                break;

            lpszFile = (LPTSTR)GetWindowLongPtr(hDlg, DWLP_USER);
            DPF("**WM_DESTROY in Hook **\r\n");
            if (lpszFile)
                LocalFree((HLOCAL)lpszFile);
            EndSound(&hse);

            break;
        }
        case WM_NOTIFY:
        {
            LPOFNOTIFY pofn;

            if (!gfWaveExists)
                break;

            pofn = (LPOFNOTIFY)lParam;
            switch (pofn->hdr.code)
            {
                case CDN_SELCHANGE:
                {
                    TCHAR szCurSel[MAX_PATH];
                    HWND hwndPlay = GetDlgItem(hDlg, ID_PLAY);
                    LPTSTR lpszFile = (LPTSTR)GetWindowLongPtr(hDlg, DWLP_USER);
                    HFILE hFile;

                    EndSound(&hse);
                    if (CommDlg_OpenSave_GetFilePath(GetParent(hDlg),szCurSel, sizeof(szCurSel)) <= (int)sizeof(szCurSel))
                    {
                        OFSTRUCT of;

                        if (!lstrcmpi(szCurSel, lpszFile))
                            break;

                        DPF("****The current selection is %s ***\r\n", szCurSel);
                        hFile = (HFILE)HandleToUlong(CreateFile(szCurSel,GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL));
                        if (lstrcmpi((LPTSTR)(szCurSel+lstrlen(szCurSel)-4), cszWavExt) || (-1 == hFile))
                        {
                            if (lpszFile[0] == TEXT('\0'))
                                break;
                            lpszFile[0] = TEXT('\0');
                            EnableWindow(hwndPlay, FALSE);
                        }
                        else
                        {
                            CloseHandle((HANDLE)hFile);
                            EnableWindow(hwndPlay, TRUE);
                            lstrcpy(lpszFile, szCurSel);
                        }
                    }
                    break;
                }

                case CDN_FOLDERCHANGE:
                {
                    EnableWindow(GetDlgItem(hDlg, ID_PLAY), FALSE);
                    break;
                }
                default:
                    break;
            }
            break;
        }

        default:
            return FALSE;

    }
    return TRUE;
}


/*
 ***************************************************************
 * InitFileOpen
 *
 * Description:
 *        Sets up the openfilestruct to read display .wav and .mid files
 *        and sets up global variables for the filename and path.
 *
 * Parameters:
 *    HWND            hDlg  - Window handle
 *    LPOPENFILENAME lpofn - pointer to openfilename struct
 *
 * Returns:            BOOL
 *
 ***************************************************************
 */
STATIC BOOL PASCAL InitFileOpen(HWND hDlg, LPOPENFILENAME lpofn)
{

    lpofn->lStructSize = sizeof(OPENFILENAME);
    lpofn->hwndOwner = hDlg;
    lpofn->hInstance = ghInstance;
    lpofn->lpstrFilter = aszFilter;
    lpofn->lpstrCustomFilter = NULL;
    lpofn->nMaxCustFilter = 0;
    lpofn->nFilterIndex = 0;
    lpofn->lpstrFile = aszPath;
    lpofn->nMaxFile = sizeof(aszPath)/sizeof(TCHAR);
    lpofn->lpstrFileTitle = aszFileName;
    lpofn->nMaxFileTitle = sizeof(aszFileName)/sizeof(TCHAR);
    lpofn->lpstrInitialDir = gszCurDir;
    lpofn->lpstrTitle = aszBrowse;
    lpofn->Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST |OFN_HIDEREADONLY |OFN_EXPLORER |OFN_ENABLEHOOK;
    if (gfWaveExists)
        lpofn->Flags |= OFN_ENABLETEMPLATE;
    lpofn->nFileOffset = 0;
    lpofn->nFileExtension = 0;
    lpofn->lpstrDefExt = NULL;
    lpofn->lCustData = 0;
    lpofn->lpfnHook = OpenDlgHook;
    if (gfWaveExists)
        lpofn->lpTemplateName = MAKEINTRESOURCE(BROWSEDLGTEMPLATE);
    else
        lpofn->lpTemplateName = NULL;
    return TRUE;
}

/* FixupNulls(chNull, p)
 *
 * To facilitate localization, we take a localized string with non-NULL
 * NULL substitutes and replacement with a real NULL.
 */
STATIC void NEAR PASCAL FixupNulls(
    TCHAR chNull,
    LPTSTR p)
{
    while (*p) {
        if (*p == chNull)
            *p++ = 0;
        else
            p = CharNext(p);
    }
} /* FixupNulls */


/* GetDefaultMediaDirectory
 *
 * Returns C:\WINNT\Media, or whatever it's called.
 *
 */
BOOL GetDefaultMediaDirectory(LPTSTR pDirectory, DWORD cbDirectory)
{
    static SZCODE szSetup[] = REGSTR_PATH_SETUP;
    static SZCODE szMedia[] = REGSTR_VAL_MEDIA;
    HKEY          hkeySetup;
    LONG          Result;

    Result = RegOpenKeyEx(HKEY_LOCAL_MACHINE, szSetup,
                          REG_OPTION_RESERVED,
                          KEY_QUERY_VALUE, &hkeySetup);

    if (Result == ERROR_SUCCESS)
    {
        Result = RegQueryValueEx(hkeySetup, szMedia, NULL, REG_NONE,
                                 (LPBYTE)pDirectory, &cbDirectory);

        RegCloseKey(hkeySetup);
    }

    return (Result == ERROR_SUCCESS);
}


/*
 ***************************************************************
 * InitStringTable
 *
 * Description:
 *      Load the RC strings into the storage for them
 *
 * Parameters:
 *      void
 *
 * Returns:        BOOL
 ***************************************************************
 */
STATIC BOOL PASCAL InitStringTable(void)
{
    static SZCODE cszSetup[] = REGSTR_PATH_SETUP;
    static SZCODE cszMedia[] = REGSTR_VAL_MEDIA;

    LoadString(ghInstance, IDS_NONE, gszNone, sizeof(gszNone)/sizeof(TCHAR));
    LoadString(ghInstance, IDS_BROWSEFORSOUND, aszBrowseStr, sizeof(aszBrowseStr)/sizeof(TCHAR));
    LoadString(ghInstance, IDS_REMOVESCHEME, gszRemoveScheme,sizeof(gszRemoveScheme)/sizeof(TCHAR));
    LoadString(ghInstance, IDS_CHANGESCHEME, gszChangeScheme,sizeof(gszChangeScheme)/sizeof(TCHAR));
    LoadString(ghInstance, IDS_DEFAULTAPP, gszDefaultApp, sizeof(gszDefaultApp)/sizeof(TCHAR));

    LoadString(ghInstance, IDS_WAVFILES, aszFilter, sizeof(aszFilter)/sizeof(TCHAR));
    LoadString(ghInstance, IDS_NULLCHAR, aszNullChar, sizeof(aszNullChar)/sizeof(TCHAR));
    FixupNulls(*aszNullChar, aszFilter);

    gszMediaDir[0] = TEXT('\0');

    if (!GetDefaultMediaDirectory(gszMediaDir, sizeof(gszMediaDir)/sizeof(TCHAR)))
    {
        GetWindowsDirectory (gszMediaDir, sizeof(gszMediaDir)/sizeof(TCHAR));
    }

    return TRUE;
}

/*
 ***************************************************************
 * SoundCleanup
 *
 * Description:
 *      Cleanup all the allocs and bitmaps when the sound page exists
 *
 * Parameters:
 *      void
 *
 * Returns:        BOOL
 ***************************************************************
 */
STATIC BOOL PASCAL SoundCleanup(HWND hDlg)
{
    DeleteObject(hIconVolume);
    DeleteObject(hIconVolTrans);
    DeleteObject(hIconVolUp);
    DeleteObject(hIconVolDown);
    DeleteObject(hIconMute);
    DeleteObject(hIconMuteUp);
    DeleteObject(hIconMuteDown);

    DeleteObject(hBitmapStop);
    DeleteObject(hBitmapPlay);
    if (ghDispBMP)
    {
        DeleteObject(ghDispBMP);
        ghDispBMP = NULL;
    }

    if (ghIconBMP)
    {
        DeleteObject(ghIconBMP);
        ghIconBMP = NULL;
    }


    if (ghPal)
    {
        DeleteObject(ghPal);
        ghPal = NULL;
    }
    TreeView_SetImageList(GetDlgItem(hDlg, IDC_EVENT_TREE), NULL, TVSIL_NORMAL);
    ImageList_Destroy(hSndImagelist);
    hSndImagelist = NULL;

    DPF("ending sound cleanup\n");
    return TRUE;
}

/****************************************************************************/
