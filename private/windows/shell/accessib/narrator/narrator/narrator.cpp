/*************************************************************************
    Project:    Narrator
    Module:     narrator.c

    Author:     Paul Blenkhorn 
    Date:       April 1997

    Notes:      Contains main application initalization code
                Credit to be given to MSAA team - bits of code have been
                lifted from:
                Babble, Inspect, and Snapshot.

    Copyright (C) 1997-1999 by Microsoft Corporation.  All rights reserved.
    See bottom of file for disclaimer

    History: Bug Fixes/ New features/ Additions: 1999 Anil Kumar

*************************************************************************/
#define STRICT

#include <windows.h>
#include <windowsx.h>
#include <oleacc.h>
#include <string.h>
#include <stdio.h>
#include <mmsystem.h>
#include <initguid.h>
#include <objbase.h>
#include <objerror.h>
#include <ole2ver.h>
#include <commctrl.h>
#include "Narrator.h"
#include "resource.h"
#include <htmlhelp.h>
#include "reader.h"
#include "..\NarrHook\keys.h"

// Bring in Speech API declarations
#include "speech.h"
#include <stdlib.h>

// UM
#include <TCHAR.h>
#include <string.h>
#include <WinSvc.h>
#include <stdio.h>

#define MAX_ENUMMODES 80
#define MAX_LANGUAGES 27
#define MAX_NAMELEN   30	// number of characters in the name excluding the path info
#define WM_DELAYEDMINIMIZE WM_USER + 102

// TTS info
TTSMODEINFO gaTTSInfo[MAX_ENUMMODES];
PIAUDIOMULTIMEDIADEVICE    pIAMM;      // multimedia device interface for audio-dest


DEFINE_GUID(MSTTS_GUID, 
0xC5C35D60, 0xDA44, 0x11D1, 0xB1, 0xF1, 0x0, 0x0, 0xF8, 0x03, 0xE4, 0x56);


// language test table, taken from WINNT.h...
LPTSTR Languages[MAX_LANGUAGES]={
    TEXT("NEUTRAL"),TEXT("BULGARIAN"),TEXT("CHINESE"),TEXT("CROATIAN"),TEXT("CZECH"),
    TEXT("DANISH"),TEXT("DUTCH"),TEXT("ENGLISH"),TEXT("FINNISH"),
    TEXT("FRENCH"),TEXT("GERMAN"),TEXT("GREEK"),TEXT("HUNGARIAN"),TEXT("ICELANDIC"),
    TEXT("ITALIAN"),TEXT("JAPANESE"),TEXT("KOREAN"),TEXT("NORWEGIAN"),
    TEXT("POLISH"),TEXT("PORTUGUESE"),TEXT("ROMANIAN"),TEXT("RUSSIAN"),TEXT("SLOVAK"),
    TEXT("SLOVENIAN"),TEXT("SPANISH"),TEXT("SWEDISH"),TEXT("TURKISH")};

WORD LanguageID[MAX_LANGUAGES]={
    LANG_NEUTRAL,LANG_BULGARIAN,LANG_CHINESE,LANG_CROATIAN,LANG_CZECH,LANG_DANISH,LANG_DUTCH,
    LANG_ENGLISH,LANG_FINNISH,LANG_FRENCH,LANG_GERMAN,LANG_GREEK,LANG_HUNGARIAN,LANG_ICELANDIC,
    LANG_ITALIAN,LANG_JAPANESE,LANG_KOREAN,LANG_NORWEGIAN,LANG_POLISH,LANG_PORTUGUESE,
    LANG_ROMANIAN,LANG_RUSSIAN,LANG_SLOVAK,LANG_SLOVENIAN,LANG_SPANISH,LANG_SWEDISH,LANG_TURKISH};

// these values are all exported from NarrHook.DLL
extern TCHAR  __declspec(dllimport) CurrentText[MAX_TEXT];
extern int __declspec (dllimport) TrackSecondary;
extern int __declspec (dllimport) TrackCaret;
extern int __declspec (dllimport) TrackInputFocus;
extern int __declspec (dllimport) EchoChars;
extern int __declspec (dllimport) AnnounceWindow;
extern int __declspec (dllimport) AnnounceMenu;
extern int __declspec (dllimport) AnnouncePopup;
extern int __declspec (dllimport) AnnounceToolTips;
extern int __declspec (dllimport) ReviewStyle;
extern int __declspec (dllimport) ReviewLevel;

// Start Type
DWORD StartMin = FALSE;
// Show warning
DWORD ShowWarn = TRUE;

// the total number of enumerated modes
DWORD gnmodes=0;                        

// Local functions
PITTSCENTRAL FindAndSelect(PTTSMODEINFO pTTSInfo);
BOOL InitTTS(void);
BOOL UnInitTTS(void);

// Dialog call back procs
INT_PTR CALLBACK MainDlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK AboutDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK ConfirmProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK WarnDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

BOOL InitApp(HINSTANCE hInstance, int nCmdShow);
BOOL UnInitApp(void);
BOOL SpeakString(TCHAR * pszSpeakText, BOOL forceRead);
void Shutup(void);
int MessageBoxLoadStrings (HWND hWnd,UINT uIDText,UINT uIDCaption,UINT uType);
void SetRegistryValues();
BOOL SetVolume (int nVolume);
BOOL SetSpeed (int nSpeed);
BOOL SetPitch (int nPitch);
DWORD GetDesktop();
void CenterWindow(HWND);
void FilterSpeech(TCHAR* szSpeak);


// Global varibles
//PITTSBUFNOTIFYSINK  g_pITTSBufNotifySink;
TCHAR               g_szLastStringSpoken[MAX_TEXT] = { NULL };
HWND				g_hwndMain = NULL;
HINSTANCE			g_hInst;
BOOL				g_fAppExiting = FALSE;
PITTSCENTRAL		g_pITTSCentral;
PITTSENUM			g_pITTSEnum = NULL;
PITTSATTRIBUTES		g_pITTSAttributes = NULL;
DWORD minSpeed, maxSpeed, currentSpeed = 3;
WORD minPitch, maxPitch, currentPitch = 3;
DWORD minVolume, maxVolume, currentVolume = 5;
WORD currentVoice = -1;
BOOL				g_startUM = FALSE; // Started from Utililty Manager
HANDLE              g_hMutexNarratorRunning;
BOOL				logonCheck = FALSE;	

// UM stuff
static BOOL  AssignDesktop(LPDWORD desktopID, LPTSTR pname);
static BOOL InitMyProcessDesktopAccess(VOID);
static VOID ExitMyProcessDesktopAccess(VOID);
static HWINSTA origWinStation = NULL;
static HWINSTA userWinStation = NULL;
// Keep a global desktop ID
DWORD desktopID;


// For Link Window
EXTERN_C BOOL WINAPI LinkWindow_RegisterClass() ;

// For Utility Manager
#define UTILMAN_DESKTOP_CHANGED_MESSAGE   __TEXT("UtilityManagerDesktopChanged")
#define DESKTOP_ACCESSDENIED 0
#define DESKTOP_DEFAULT      1
#define DESKTOP_SCREENSAVER  2
#define DESKTOP_WINLOGON     3
#define DESKTOP_TESTDISPLAY  4
#define DESKTOP_OTHER        5


//CS help
DWORD g_rgHelpIds[] = {	IDC_VOICESETTINGS, 70600,
						IDC_VOICE, 70605,
						IDC_NAME, 70605,
						IDC_EDITSPEED, 70610,
						IDC_SPEEDSPIN, 70610,
						IDC_EDITVOLUME, 70615,
						IDC_VOLUMESPIN, 70615,
						IDC_EDITPITCH, 70620,
						IDC_PITCHSPIN, 70620,
                        IDC_MODIFIERS, 70645,
                        IDC_ANNOUNCE, 70710,
						IDC_READING, 70625,
                        IDC_MOUSEPTR, 70695,
						IDC_MSRCONFIG, 70600,
						IDC_STARTMIN, 70705,
						IDC_EXIT, -1,
						IDC_MSRHELP, -1,
						IDC_CAPTION, -1
                        };

/*************************************************************************
Function:   WinMain
Purpose:    Entry point of application
Inputs:
Returns:    Int containing the return value of the app.
History:
*************************************************************************/
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpszCmdLine, int nCmdShow)
{
    DBPRINTF (TEXT("WinMain\r\n"));
    UINT deskSwitchMsg;

	// Get the commandline so that it works for MUI/Unicode
	LPTSTR lpCmdLineW = GetCommandLine();
  
	if(NULL != lpCmdLineW && lstrlen(lpCmdLineW))
	{
		if ( lstrcmpi(lpCmdLineW, TEXT("/UM")) == 0 )
			g_startUM = TRUE;
	}

	// Don't allow multiple versions of Narrator
	// This works with Narrator.exe run stand-alone, but not if run from development environment - wierd!
	g_hMutexNarratorRunning = CreateMutex(NULL, TRUE, TEXT("AK:NarratorRunning"));
	if (!g_hMutexNarratorRunning ||
		GetLastError() == ERROR_ALREADY_EXISTS)
	{
		if ( !g_startUM )
		{
			DBPRINTF (TEXT("Can't run Narrator:\r\n"));

			if (!g_hMutexNarratorRunning)
				DBPRINTF (TEXT(" Could not create mutex object.\r\n"));

			if (GetLastError() == ERROR_ALREADY_EXISTS)
				DBPRINTF (TEXT(" Another copy of Narrator is already running.\r\n"));
			return 0;
		}
		else
		{
			// if this happens while started from UM, Means that the dlls are not unloaded...
			// So delay and walk through...And try again
			Sleep(1000);
			g_hMutexNarratorRunning = CreateMutex(NULL, TRUE, TEXT("AK:NarratorRunning"));
		}
	}
	
	// for the Link Window in finish page...
	LinkWindow_RegisterClass();

    // Initialization
    g_hInst = hInstance;

    TCHAR name[300];

    // For Multiple desktops (UM)
    // UM
    deskSwitchMsg = RegisterWindowMessage(UTILMAN_DESKTOP_CHANGED_MESSAGE);

    InitMyProcessDesktopAccess();
	AssignDesktop(&desktopID,name);

    if (InitApp(hInstance, nCmdShow))
    {
        MSG     msg;

        // Main message loop
        while (GetMessage(&msg, NULL, 0, 0))
        {
            if (!IsDialogMessage(g_hwndMain, &msg))
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);

                if (msg.message == deskSwitchMsg)
                {
                    g_fAppExiting = TRUE;

                    UnInitApp();
                }
            }
        }
    }

    // UM
    ExitMyProcessDesktopAccess();
    return 0;
}


/*************************************************************************
Function:
Purpose:
Inputs:
Returns:
History:
*************************************************************************/
LPTSTR LangIDtoString( WORD LangID )
{
    int i;
    for( i=0; i<MAX_LANGUAGES; i++ )
    {
        if( (LangID & 0xFF) == LanguageID[i] )
            return Languages[i];
    }

    return NULL;
}

/*************************************************************************
Function:
Purpose: Get the range for speed, pitch etc..,
Inputs:
Returns:
History:
*************************************************************************/
void GetSpeechMinMaxValues(void)
{
    WORD	tmpPitch;
    DWORD	tmpSpeed;
    DWORD	tmpVolume;

    g_pITTSAttributes->PitchGet(&tmpPitch);
    g_pITTSAttributes->PitchSet(TTSATTR_MAXPITCH);
    g_pITTSAttributes->PitchGet(&maxPitch);
    g_pITTSAttributes->PitchSet(TTSATTR_MINPITCH);
    g_pITTSAttributes->PitchGet(&minPitch);
    g_pITTSAttributes->PitchSet(tmpPitch);

    g_pITTSAttributes->SpeedGet(&tmpSpeed);
    g_pITTSAttributes->SpeedSet(TTSATTR_MINSPEED);
    g_pITTSAttributes->SpeedGet(&minSpeed);
    g_pITTSAttributes->SpeedSet(TTSATTR_MAXSPEED);
    g_pITTSAttributes->SpeedGet(&maxSpeed);
    g_pITTSAttributes->SpeedSet(tmpSpeed);

    g_pITTSAttributes->VolumeGet(&tmpVolume);
    g_pITTSAttributes->VolumeSet(TTSATTR_MINVOLUME);
    g_pITTSAttributes->VolumeGet(&minVolume);
    g_pITTSAttributes->VolumeSet(TTSATTR_MAXVOLUME);
    g_pITTSAttributes->VolumeGet(&maxVolume);
    g_pITTSAttributes->VolumeSet(tmpVolume);

#if DEBUG

    DBPRINTF (TEXT("Min Pitch  = %d\r\n"),minPitch);
    DBPRINTF (TEXT("Max Pitch  = %d\r\n"),maxPitch);
    DBPRINTF (TEXT("Min Speed  = %ld\r\n"),minSpeed);
    DBPRINTF (TEXT("Max Speed  = %ld\r\n"),maxSpeed);
    DBPRINTF (TEXT("Min Volume Left %d Right %d\r\n"),LOWORD(minVolume),HIWORD(minVolume));
    DBPRINTF (TEXT("Max Volume Left %d Right %d\r\n"),LOWORD(maxVolume),HIWORD(maxVolume));

#endif
}

/*************************************************************************
    Function:   VoiceDlgProc
    Purpose:    Handles messages for the Voice Box dialog
    Inputs:
    Returns:
    History:
*************************************************************************/
INT_PTR CALLBACK VoiceDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	static WORD oldVoice, oldPitch;
    static DWORD oldSpeed, oldVolume;
	static HWND hwndList;
	WORD	wNewPitch;
	
	DWORD   i;
	int     Selection;
	
	TCHAR   szTxt[MAX_TEXT];
	HRESULT hRes;
	
	szTxt[0]=TEXT('\0');
	
	switch (uMsg)
	{
		case WM_INITDIALOG:
			oldVoice = currentVoice; // save voice parameters in case of CANCEL
			oldPitch = currentPitch;
			oldVolume = currentVolume;
			oldSpeed = currentSpeed;
			
			Shutup();

			hwndList = GetDlgItem(hwnd, IDC_NAME);

			if ( !logonCheck )
			{
				// Security check !
				for (i = 0; i < gnmodes; i++)
				{
					lstrcpyn(szTxt,gaTTSInfo[i].szModeName,MAX_TEXT);
					lstrcatn(szTxt,TEXT(", "),MAX_TEXT);
					
					lstrcatn(szTxt,
						LangIDtoString(gaTTSInfo[i].language.LanguageID),
						MAX_TEXT);
					lstrcatn(szTxt,TEXT(", "),MAX_TEXT);
					
					lstrcatn(szTxt,gaTTSInfo[i].szMfgName,MAX_TEXT);
					
					SendMessage(hwndList, LB_ADDSTRING, 0, (LPARAM) szTxt);
				}
				SendMessage(hwndList, LB_SETCURSEL,currentVoice, 0L);
			}
			else
			{
				LoadString(g_hInst, IDS_SAM, szTxt, MAX_TEXT);

				SendMessage(hwndList, LB_ADDSTRING, 0, (LPARAM) szTxt);
				EnableWindow(hwndList, FALSE);
			}
			
			hwndList=GetDlgItem(hwnd,IDC_SPEEDSPIN);
			SendMessage(hwndList,UDM_SETRANGE,0,
				MAKELONG((short) 9, (short) 1));
			
			hwndList=GetDlgItem(hwnd,IDC_PITCHSPIN);
			SendMessage(hwndList,UDM_SETRANGE,0,
				MAKELONG((short) 9, (short) 1));
			
			hwndList=GetDlgItem(hwnd,IDC_VOLUMESPIN);
			SendMessage(hwndList,UDM_SETRANGE,0,
				MAKELONG((short) 9, (short) 1));
			
			SetDlgItemInt(hwnd,IDC_EDITPITCH,currentPitch,FALSE);
			SetDlgItemInt(hwnd,IDC_EDITVOLUME,currentVolume,FALSE);
			SetDlgItemInt(hwnd,IDC_EDITSPEED,currentSpeed,FALSE);

			break;
			
			
		case WM_COMMAND:
            {
			BOOL	bDone = FALSE;
			DWORD	dwValue;
			int control = LOWORD(wParam);
			switch (LOWORD(wParam))
			{
				case IDC_NAME:
					hwndList = GetDlgItem(hwnd,IDC_NAME);

					Selection = (WORD) SendMessage(hwndList, LB_GETCURSEL,0, 0L);
					if (Selection < 0 || Selection > 79)
						Selection = 0;
					Shutup();
					
					if (currentVoice != Selection) 
					{ // voice changed!
						MessageBeep(MB_OK);
						currentVoice = (WORD)Selection;
						// Get the audio dest
						g_pITTSCentral->Release();
						
						if ( pIAMM )
						{
							pIAMM->Release();
							pIAMM = NULL;
						}

						hRes = CoCreateInstance(CLSID_MMAudioDest,
							NULL,
							CLSCTX_ALL,
							IID_IAudioMultiMediaDevice,
							(void**)&pIAMM);

						if (FAILED(hRes))
							return TRUE;     // error

						hRes = g_pITTSEnum->Select( gaTTSInfo[Selection].gModeID,
							&g_pITTSCentral,
							(LPUNKNOWN) pIAMM);
						
						if (FAILED(hRes))
							MessageBeep(MB_OK);
						g_pITTSAttributes->Release();
						
						hRes = g_pITTSCentral->QueryInterface (IID_ITTSAttributes, (void**)&g_pITTSAttributes);
					}
					
					GetSpeechMinMaxValues(); // get speech parameters for this voice
					
					// then reset pitch etc. accordingly
                    currentPitch = (WORD)GetDlgItemInt(hwnd,IDC_EDITPITCH,&bDone,TRUE);
				    currentSpeed = GetDlgItemInt(hwnd,IDC_EDITSPEED,&bDone,TRUE);
				    currentVolume = GetDlgItemInt(hwnd,IDC_EDITVOLUME,&bDone,TRUE);

                    SetPitch (currentPitch);
                    SetSpeed (currentSpeed);
                    SetVolume (currentVolume);
					// if (bDone)
					// {
						SendMessage(hwndList, LB_GETTEXT, Selection,
							(LPARAM) szTxt);
					// }
					
					SpeakString(szTxt, TRUE);
					// Shutup();
					break;
				
				case IDC_EDITSPEED:
					dwValue = GetDlgItemInt(hwnd,control,&bDone,TRUE);
					if (bDone)
					{
						if (dwValue > 9)
							dwValue = 9;
						SetSpeed(dwValue);

					}
					break;
				
				case IDC_EDITVOLUME:
					dwValue = GetDlgItemInt(hwnd,control,&bDone,TRUE);
					if (bDone)
					{

						if (dwValue > 9)
							dwValue = 9;
						SetVolume(dwValue);
					}
					break;
				
				case IDC_EDITPITCH:
					dwValue = GetDlgItemInt(hwnd,control,&bDone,TRUE);
					if (bDone)
					{
						if (dwValue > 9)
							dwValue = 9;
                        SetPitch(dwValue);
					}
					break;
				
				case IDCANCEL:
					MessageBeep(MB_OK);
					Shutup();
					if (currentVoice != oldVoice) 
					{ // voice changed!
						currentVoice = oldVoice;
						
						// Get the audio dest
						g_pITTSCentral->Release();

						if ( pIAMM )
						{
							pIAMM->Release();
							pIAMM = NULL;
						}

						hRes = CoCreateInstance(CLSID_MMAudioDest,
							NULL,
							CLSCTX_ALL,
							IID_IAudioMultiMediaDevice,
							(void**)&pIAMM);

						if (FAILED(hRes))
							return TRUE;     // error

						hRes = g_pITTSEnum->Select( gaTTSInfo[currentVoice].gModeID,
							&g_pITTSCentral,
							(LPUNKNOWN) pIAMM);

						if (FAILED(hRes))
							MessageBeep(MB_OK);
						
						g_pITTSAttributes->Release();
						hRes = g_pITTSCentral->QueryInterface (IID_ITTSAttributes, (void**)&g_pITTSAttributes);
					}
					
					GetSpeechMinMaxValues(); // speech get parameters for old voice
					
                    currentPitch = oldPitch; // restore old values
                    SetPitch (currentPitch);

                    currentSpeed = oldSpeed;
                    SetSpeed (currentSpeed);

                    currentVolume = oldVolume;
                    SetVolume (currentVolume);					

                    EndDialog (hwnd, IDCANCEL);
					return(TRUE);

				case IDOK: // set values of pitch etc. from check boxes

                    currentPitch = (WORD)GetDlgItemInt(hwnd,IDC_EDITPITCH,&bDone,TRUE);
				    currentSpeed = GetDlgItemInt(hwnd,IDC_EDITSPEED,&bDone,TRUE);
				    currentVolume = GetDlgItemInt(hwnd,IDC_EDITVOLUME,&bDone,TRUE);

                    if ( currentPitch > 9)
                        currentPitch = 9;

                    if ( currentSpeed > 9)
                        currentSpeed = 9;

                    if ( currentVolume > 9)
                        currentVolume = 9;

                    SetPitch (currentPitch);
                    SetSpeed (currentSpeed);
                    SetVolume (currentVolume);

                    SetRegistryValues();
					EndDialog (hwnd, IDOK);
					return(TRUE);
			} // end switch on control of WM_COMMAND
            }
			break;

        case WM_CONTEXTMENU:  // right mouse click
			{
				if ( GetDesktop() != DESKTOP_DEFAULT )
				return 0;

				WinHelp((HWND) wParam, __TEXT("reader.hlp"), HELP_CONTEXTMENU, (DWORD_PTR) (LPSTR) g_rgHelpIds);
			}
            break;

		case WM_CLOSE:
				EndDialog (hwnd, IDOK);
				return TRUE;
			break;

        case WM_HELP:
			{
				if ( GetDesktop() != DESKTOP_DEFAULT )
				return 0;

				WinHelp((HWND) ((LPHELPINFO) lParam)->hItemHandle, __TEXT("reader.hlp"), HELP_WM_HELP, (DWORD_PTR) (LPSTR) g_rgHelpIds);
			}
            return(TRUE);

	} // end switch uMsg

	return(FALSE);  // didn't handle
}



/*************************************************************************
    Function:   SetVolume
    Purpose:    set volume to a normalized value 1-9
    Inputs:     int volume in range 1-9
    Returns:
    History:
*************************************************************************/
BOOL SetVolume (int nVolume)
{
	DWORD	dwNewVolume;
	WORD	wNewVolumeLeft,
			wNewVolumeRight;

	//ASSERT (nVolume >= 1 && nVolume <= 9);
	wNewVolumeLeft = (WORD)( (LOWORD(minVolume) + (((LOWORD(maxVolume) - LOWORD(minVolume))/9.0)*nVolume)) );
	wNewVolumeRight = (WORD)( (HIWORD(minVolume) + (((HIWORD(maxVolume) - HIWORD(minVolume))/9.0)*nVolume)) );
	dwNewVolume = MAKELONG (wNewVolumeLeft,wNewVolumeRight);

	DBPRINTF (TEXT("New vol left %u right %u value %d\r\n"),wNewVolumeLeft,wNewVolumeRight,nVolume);

    return (SUCCEEDED(g_pITTSAttributes->VolumeSet(dwNewVolume)));
}

/*************************************************************************
    Function:   SetSpeed
    Purpose:    set Speed to a normalized value 1-9
    Inputs:     int Speed in range 1-9
    Returns:
    History:
*************************************************************************/
BOOL SetSpeed (int nSpeed)
{
	DWORD	dwNewSpeed;

	//ASSERT (nSpeed >= 1 && nSpeed <= 9);
	dwNewSpeed = minSpeed + (DWORD) ((maxSpeed-minSpeed)/9.0*nSpeed);
	DBPRINTF (TEXT("new speed %lu value %d\r\n"),dwNewSpeed,nSpeed);
	return (SUCCEEDED(g_pITTSAttributes->SpeedSet(dwNewSpeed)));
}

/*************************************************************************
    Function:   SetPitch
    Purpose:    set Pitch to a normalized value 1-9
    Inputs:     int Pitch in range 1-9
    Returns:
    History:
*************************************************************************/
BOOL SetPitch (int nPitch)
{
	WORD	wNewPitch;

	wNewPitch = (WORD)((minPitch + (((maxPitch - minPitch)/9.0)*nPitch)));
	DBPRINTF (TEXT("new pitch %u value %d\r\n"),wNewPitch,nPitch);
	return (SUCCEEDED(g_pITTSAttributes->PitchSet(wNewPitch)));
}

#define TIMERID 6466
/*************************************************************************
    Function:   MainDlgProc
    Purpose:    Handles messages for the Main dialog
    Inputs:
    Returns:
    History:
*************************************************************************/
INT_PTR CALLBACK MainDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
        {
        case WM_INITDIALOG:
			// Disable Help button on other desktops
			if ( GetDesktop() != DESKTOP_DEFAULT )
			{
				EnableWindow(GetDlgItem(hwnd, IDC_MSRHELP), FALSE);
			}
			

			CheckDlgButton(hwnd,IDC_MOUSEPTR,TrackInputFocus);
			CheckDlgButton(hwnd,IDC_READING,EchoChars);
			CheckDlgButton(hwnd,IDC_ANNOUNCE,AnnounceWindow);
			CheckDlgButton(hwnd,IDC_STARTMIN,StartMin);
			
			// To show the warning message...
			SetTimer(hwnd, TIMERID, 20, NULL);
            break;

		case WM_TIMER:
			KillTimer(hwnd, (UINT)wParam);
	        
			if (ShowWarn && (GetDesktop() == DESKTOP_DEFAULT))
            {
		        DialogBox (g_hInst, MAKEINTRESOURCE(IDD_WARNING),hwnd, WarnDlgProc);
                return TRUE;
	        }
			break;

        case WM_WININICHANGE:
            if (g_fAppExiting) break;

            // If someone else turns off the system-wide screen reader
            // flag, we want to turn it back on.
            if (wParam == SPI_SETSCREENREADER && !lParam)
                SystemParametersInfo(SPI_SETSCREENREADER, TRUE, NULL, SPIF_UPDATEINIFILE|SPIF_SENDCHANGE);
            return 0;

		case WM_DELAYEDMINIMIZE:
			// Delayed mimimize message
			ShowWindow(hwnd, SW_HIDE);
			ShowWindow(hwnd, SW_MINIMIZE);
			break;

		case WM_MUTE:
			Shutup();
			break;

        case WM_MSRSPEAK:
//			SetDlgItemText (g_hwndMain, IDC_EDIT1, (TCHAR *) wParam);
			SpeakString((TCHAR *) CurrentText, TRUE);
			break;

        case WM_MSRQUIT:
        case WM_CLOSE:
			// When Narrator starts during startup, The exit message box is getting hidden behind the
            // start up dialog  :a-anilk
			// Donot show an exit confirmation if started from UM
			if ( !g_startUM )
			{
				if (IDOK != DialogBox(g_hInst, MAKEINTRESOURCE(IDD_CONFIRMEXIT), g_hwndMain, ConfirmProc))
					return(FALSE);
			}

            // Fall through
		case WM_DESTROY:

            // Required for desktop switches :a-anilk
            g_fAppExiting = TRUE;
			g_hwndMain = NULL;
            // return DefWindowProc(hwnd, uMsg, wParam, lParam);

			UnInitApp();
            
			if ( g_hMutexNarratorRunning ) 
				ReleaseMutex(g_hMutexNarratorRunning);
            // Let others know that you are turning off the system-wide
		    // screen reader flag.
            SystemParametersInfo(SPI_SETSCREENREADER, FALSE, NULL, SPIF_UPDATEINIFILE|SPIF_SENDCHANGE);

            EndDialog (hwnd, IDCANCEL);
            PostQuitMessage(0);

            return(TRUE);

		case WM_MSRHELP:
            // Show HTML help
			if ( GetDesktop() == DESKTOP_DEFAULT )
				 HtmlHelp(hwnd ,TEXT("reader.chm"),HH_DISPLAY_TOPIC, 0);
			break;

		case WM_MSRCONFIGURE:
			DialogBox (g_hInst, MAKEINTRESOURCE(IDD_VOICE),hwnd, VoiceDlgProc);
			break;

		case WM_HELP:
			if ( GetDesktop() == DESKTOP_DEFAULT )
				HtmlHelp(hwnd ,TEXT("reader.chm"),HH_DISPLAY_TOPIC, 0);
			break;

		case WM_CONTEXTMENU:  // right mouse click
				if ( GetDesktop() != DESKTOP_DEFAULT )
					return FALSE;
				WinHelp((HWND) wParam, __TEXT("reader.hlp"), HELP_CONTEXTMENU, (DWORD_PTR) (LPSTR) g_rgHelpIds);
            break;

        case WM_SYSCOMMAND:
	        if ((wParam & 0xFFF0) == IDM_ABOUTBOX)
            {
		        DialogBox (g_hInst, MAKEINTRESOURCE(IDD_ABOUTBOX),hwnd,AboutDlgProc);
                return TRUE;
	        }
            break;
		
        // case WM_QUERYENDSESSION:
		// return TRUE;

		case WM_ENDSESSION:
		{
			 HKEY hKey;
			 DWORD dwPosition;
			 const TCHAR szSubKey[] =  __TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\RunOnce");
			 const TCHAR szImageName[] = __TEXT("Narrator.exe");
             		 const TCHAR szValueName[] = __TEXT("RunNarrator");

			 if ( ERROR_SUCCESS == RegCreateKeyEx(HKEY_CURRENT_USER, szSubKey, 0, NULL,
				 REG_OPTION_NON_VOLATILE, KEY_QUERY_VALUE | KEY_SET_VALUE, NULL, &hKey, &dwPosition))
			 {
				 RegSetValueEx(hKey, (LPCTSTR) szValueName, 0, REG_SZ, (CONST BYTE*)szImageName, (lstrlen(szImageName)+1)*sizeof(TCHAR) );
				 RegCloseKey(hKey);
			 }
		}
        return 0;

        case WM_COMMAND:
            switch (LOWORD(wParam))
                {
			case IDC_MSRHELP :
				PostMessage(hwnd, WM_MSRHELP,0,0);
				break;

			case IDC_MINIMIZE:
				BackToApplication();
				break;

			case IDC_MSRCONFIG :
				PostMessage(hwnd, WM_MSRCONFIGURE,0,0);
				break;

			case IDC_EXIT :
				PostMessage(hwnd, WM_MSRQUIT,0,0);
				break;

			case IDC_ANNOUNCE:
				AnnounceWindow = IsDlgButtonChecked(hwnd,IDC_ANNOUNCE);
				AnnounceMenu = AnnouncePopup = AnnounceWindow;
				break;

			case IDC_READING:
				if (IsDlgButtonChecked(hwnd,IDC_READING))
					EchoChars = MSR_ECHOALNUM | MSR_ECHOSPACE | MSR_ECHODELETE | MSR_ECHOMODIFIERS 
								 | MSR_ECHOENTER | MSR_ECHOBACK | MSR_ECHOTAB;
				else
					EchoChars = 0;
				SetRegistryValues();	
				break;

			case IDC_MOUSEPTR:
				TrackInputFocus = IsDlgButtonChecked(hwnd,IDC_MOUSEPTR);
				TrackCaret = TrackInputFocus;
				break;

			case IDC_STARTMIN:
				StartMin = IsDlgButtonChecked(hwnd,IDC_STARTMIN);
				break;
        }
	}
    return(FALSE);  // didn't handle
}

/*************************************************************************
    Function:   AboutDlgProc
    Purpose:    Handles messages for the About Box dialog
    Inputs:
    Returns:
    History:
*************************************************************************/
INT_PTR CALLBACK AboutDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_INITDIALOG:
			Shutup();
			// If minimized, Center on the desktop..
			if ( IsIconic(g_hwndMain) )
			{
				CenterWindow(hwnd);
			}
			return TRUE;

			break;

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
				case IDOK:
                case IDCANCEL:
					Shutup();
                    EndDialog (hwnd, IDCANCEL);
                    return(TRUE);
                }
				break;

				case WM_NOTIFY:
					{
						INT idCtl		= (INT)wParam;
						LPNMHDR pnmh	= (LPNMHDR)lParam;
						switch ( pnmh->code)
						{
							case NM_RETURN:
							case NM_CLICK:
							if ( (GetDesktop() == DESKTOP_DEFAULT) && (idCtl == IDC_ENABLEWEBA))
							{
								TCHAR webAddr[256];
								LoadString(g_hInst, IDS_ENABLEWEB, webAddr, 256);
								ShellExecute(hwnd, TEXT("open"), TEXT("iexplore.exe"), webAddr, NULL, SW_SHOW); 
							}
							break;
						}
					}
					break;

            };

    return(FALSE);  // didn't handle
}


/*************************************************************************
    Function:   WarnDlgProc
    Purpose:    Handles messages for the Warning dialog
    Inputs:
    Returns:
    History:
*************************************************************************/
INT_PTR CALLBACK WarnDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_INITDIALOG:
			Shutup();
			// If minimized, Center on the desktop..
			if ( IsIconic(g_hwndMain) )
			{
				CenterWindow(hwnd);
			}
			return TRUE;

			break;

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
				case IDC_WARNING:
					ShowWarn = !(IsDlgButtonChecked(hwnd,IDC_WARNING));
				break;

				case IDOK:
                case IDCANCEL:
					Shutup();
                    EndDialog (hwnd, IDCANCEL);
                    return(TRUE);
			} 
			break;

		case WM_NOTIFY:
			{
				INT idCtl		= (INT)wParam;
				LPNMHDR pnmh	= (LPNMHDR)lParam;
				switch ( pnmh->code)
				{
					case NM_RETURN:
					case NM_CLICK:
					if ( (GetDesktop() == DESKTOP_DEFAULT) && (idCtl == IDC_ENABLEWEB))
					{
						TCHAR webAddr[256];
						LoadString(g_hInst, IDS_ENABLEWEB, webAddr, 256);
						ShellExecute(hwnd, TEXT("open"), TEXT("iexplore.exe"), webAddr, NULL, SW_SHOW); 
					}
					break;
				}
			}
			break;
     };

    return(FALSE);  // didn't handle
}


/*************************************************************************
    Function:
    Purpose: Save registry values
    Inputs:
    Returns:
    History:
*************************************************************************/
void SetRegistryValues()
{ // set up the registry
    HKEY reg_key;	// key for the registry

    if (SUCCEEDED (RegOpenKeyEx (HKEY_CURRENT_USER,__TEXT("Software\\Microsoft\\Narrator"),0,KEY_WRITE,&reg_key)))
    {
        //	RegSetValueEx(reg_key,__TEXT("TrackSecondary"),0,REG_DWORD,(unsigned char *) &TrackSecondary,sizeof(TrackSecondary));
        RegSetValueEx(reg_key,__TEXT("TrackCaret"),0,REG_DWORD,(unsigned char *) &TrackCaret,sizeof(TrackCaret));
        RegSetValueEx(reg_key,__TEXT("TrackInputFocus"),0,REG_DWORD,(unsigned char *) &TrackInputFocus,sizeof(TrackInputFocus));
        RegSetValueEx(reg_key,__TEXT("EchoChars"),0,REG_DWORD,(unsigned char *) &EchoChars,sizeof(EchoChars));
        RegSetValueEx(reg_key,__TEXT("AnnounceWindow"),0,REG_DWORD,(unsigned char *) &AnnounceWindow,sizeof(AnnounceWindow));
        RegSetValueEx(reg_key,__TEXT("AnnounceMenu"),0,REG_DWORD,(unsigned char *) &AnnounceMenu,sizeof(AnnounceMenu));
        RegSetValueEx(reg_key,__TEXT("AnnouncePopup"),0,REG_DWORD,(unsigned char *) &AnnouncePopup,sizeof(AnnouncePopup));
        RegSetValueEx(reg_key,__TEXT("AnnounceToolTips"),0,REG_DWORD,(unsigned char *) &AnnounceToolTips,sizeof(AnnounceToolTips));
        RegSetValueEx(reg_key,__TEXT("ReviewLevel"),0,REG_DWORD,(unsigned char *) &ReviewLevel,sizeof(ReviewLevel));
        RegSetValueEx(reg_key,__TEXT("CurrentSpeed"),0,REG_DWORD,(unsigned char *) &currentSpeed,sizeof(currentSpeed));
        RegSetValueEx(reg_key,__TEXT("CurrentPitch"),0,REG_DWORD,(unsigned char *) &currentPitch,sizeof(currentPitch));
        RegSetValueEx(reg_key,__TEXT("CurrentVolume"),0,REG_DWORD,(unsigned char *) &currentVolume,sizeof(currentVolume));
        RegSetValueEx(reg_key,__TEXT("CurrentVoice"),0,REG_DWORD,(unsigned char *) &currentVoice,sizeof(currentVoice));
        RegSetValueEx(reg_key,__TEXT("StartType"),0,REG_DWORD,(unsigned char *) &StartMin,sizeof(StartMin));
        RegSetValueEx(reg_key,__TEXT("ShowWarning"),0,REG_DWORD, (BYTE*) &ShowWarn,sizeof(ShowWarn));
        RegCloseKey (reg_key);
        return;
    }
}

/*************************************************************************
    Function:
    Purpose: Get Registry values
    Inputs:
    Returns:
    History:
*************************************************************************/
void GetRegistryValues()
{
	DWORD	result;
	HKEY	reg_key;
	DWORD	reg_size;

    // We use RegCreateKeyEx instead of RegOpenKeyEx to make sure that the
    // key is created if it doesn't exist. Note that if the key doesn't
    // exist, the values used will already be set. All these values are
    // globals imported from narrhook.dll
	RegCreateKeyEx(HKEY_CURRENT_USER,__TEXT("Software\\Microsoft\\Narrator"),0,
        __TEXT("MSR"),REG_OPTION_NON_VOLATILE,KEY_ALL_ACCESS, NULL, &reg_key, &result);
	if (result == REG_OPENED_EXISTING_KEY)
    {
//		reg_size = sizeof(TrackSecondary);
//		RegQueryValueEx(reg_key,__TEXT("TrackSecondary"),0,NULL,(unsigned char *) &TrackSecondary,&reg_size);
		reg_size = sizeof(TrackCaret);
		RegQueryValueEx(reg_key,__TEXT("TrackCaret"),0,NULL,(unsigned char *) &TrackCaret,&reg_size);
		reg_size = sizeof(TrackInputFocus);
		RegQueryValueEx(reg_key,__TEXT("TrackInputFocus"),0,NULL,(unsigned char *) &TrackInputFocus,&reg_size);
		reg_size = sizeof(EchoChars);
		RegQueryValueEx(reg_key,__TEXT("EchoChars"),0,NULL,(unsigned char *) &EchoChars,&reg_size);
		reg_size = sizeof(AnnounceWindow);
		RegQueryValueEx(reg_key,__TEXT("AnnounceWindow"),0,NULL,(unsigned char *) &AnnounceWindow,&reg_size);
		reg_size = sizeof(AnnounceMenu);
		RegQueryValueEx(reg_key,__TEXT("AnnounceMenu"),0,NULL,(unsigned char *) &AnnounceMenu,&reg_size);
		reg_size = sizeof(AnnouncePopup);
		RegQueryValueEx(reg_key,__TEXT("AnnouncePopup"),0,NULL,(unsigned char *) &AnnouncePopup,&reg_size);
		reg_size = sizeof(AnnounceToolTips);
		RegQueryValueEx(reg_key,__TEXT("AnnounceToolTips"),0,NULL,(unsigned char *) &AnnounceToolTips,&reg_size);
		reg_size = sizeof(ReviewLevel);
		RegQueryValueEx(reg_key,__TEXT("ReviewLevel"),0,NULL,(unsigned char *) &ReviewLevel,&reg_size);
		reg_size = sizeof(currentSpeed);
		RegQueryValueEx(reg_key,__TEXT("CurrentSpeed"),0,NULL,(unsigned char *) &currentSpeed,&reg_size);
		reg_size = sizeof(currentPitch);
		RegQueryValueEx(reg_key,__TEXT("CurrentPitch"),0,NULL,(unsigned char *) &currentPitch,&reg_size);
		reg_size = sizeof(currentVolume);
		RegQueryValueEx(reg_key,__TEXT("CurrentVolume"),0,NULL,(unsigned char *) &currentVolume,&reg_size);
		reg_size = sizeof(currentVoice);
		RegQueryValueEx(reg_key,__TEXT("CurrentVoice"),0,NULL,(unsigned char *) &currentVoice,&reg_size);
		// Minimized Value
		reg_size = sizeof(StartMin);
		RegQueryValueEx(reg_key,__TEXT("StartType"),0,NULL,(unsigned char *) &StartMin,&reg_size);
		reg_size = sizeof(ShowWarn);
		RegQueryValueEx(reg_key,__TEXT("ShowWarning"),0,NULL,(unsigned char *) &ShowWarn,&reg_size);
	}
    // If the key was just created, then these values should already be set.
	// The values are exported from narrhook.dll, and must be initialized
	// when they are declared.
	RegCloseKey (reg_key);
}


/*************************************************************************
    Function:   InitApp
    Purpose:    Initalizes the application.
    Inputs:     HINSTANCE hInstance - Handle to the current instance
                INT nCmdShow - how to present the window
    Returns:    TRUE if app initalized without error.
    History:
*************************************************************************/
BOOL InitApp(HINSTANCE hInstance, int nCmdShow)
{
    HMENU	hSysMenu;
	RECT	rcWorkArea;
	RECT	rcWindow;
	int		xPos,yPos;
	HRESULT	hr;
	
	DBPRINTF (TEXT("InitApp\r\n"));
	GetRegistryValues();
	// SMode = InitMode();

	// start COM
	hr = CoInitialize(NULL);
	if (FAILED (hr))
	{
		DBPRINTF (TEXT("CoInitialize on primary thread returned 0x%lX\r\n"),hr);
		MessageBoxLoadStrings (NULL, IDS_NO_OLE, IDS_MSR_ERROR, MB_OK);
		return FALSE;
	}
	
	// Create the TTS Objects
	// sets the global variable g_pITTSCentral
	// if it fails, will throw up a message box
	if (InitTTS())
	{
		// Initialize Microsoft Active Accessibility
		// this is in narrhook.dll
		// Installs WinEvent hook, creates helper thread
		if (InitMSAA())
		{
			// Set the system screenreader flag on.
			// e.g. Word 97 will expose the caret position.
			SystemParametersInfo(SPI_SETSCREENREADER, TRUE, NULL, SPIF_UPDATEINIFILE|SPIF_SENDCHANGE);
			
			// Create the dialog box
			g_hwndMain = CreateDialog(g_hInst, MAKEINTRESOURCE(IDD_MAIN),
											    0, MainDlgProc);

			if (!g_hwndMain)
			{
				DBPRINTF(TEXT("unable to create main dialog\r\n"));
				return(FALSE);
			}

            // Set the icon correctly to Narrator icon. AK
            ULONG_PTR icnPtr = GetClassLongPtr(g_hwndMain, GCLP_HICON);

            icnPtr = (LONG_PTR) LoadIcon(g_hInst, MAKEINTRESOURCE(IDI_ICON1));

            SetClassLongPtr(g_hwndMain, GCLP_HICON, icnPtr);

			// Init global hot keys (need to do here because we need a hwnd)
			InitKeys(g_hwndMain);

			// set icon for this dialog if we can...
			// not an easy way to do this that I know of.
			// hIcon is in the WndClass, which means that if we change it for
			// us, we change it for everyone. Which means that we would
			// have to create a superclass that looks like a dialog but
			// has it's own hIcon. Sheesh.

			// Add "About Narrator..." menu item to system menu.
			hSysMenu = GetSystemMenu(g_hwndMain,FALSE);
			if (hSysMenu != NULL)
			{
				TCHAR szAboutMenu[100];

				if (LoadString(g_hInst,IDS_ABOUTBOX,szAboutMenu,sizeof(szAboutMenu)))
				{
					AppendMenu(hSysMenu,MF_SEPARATOR,NULL,NULL);
					AppendMenu(hSysMenu,MF_STRING,IDM_ABOUTBOX,szAboutMenu);
				}
			}

			// Place dialog at bottom right of screen:AK
			SystemParametersInfo (SPI_GETWORKAREA,NULL,&rcWorkArea,NULL);
			GetWindowRect (g_hwndMain,&rcWindow);
			xPos = rcWorkArea.right - (rcWindow.right - rcWindow.left);
			yPos = rcWorkArea.bottom - (rcWindow.bottom - rcWindow.top);
			SetWindowPos(g_hwndMain, HWND_TOP, xPos, yPos, 0, 0, SWP_NOSIZE |
				SWP_NOACTIVATE);

			// If Start type is Minimized. 
			if(StartMin)
			{
				ShowWindow(g_hwndMain, SW_SHOWMINIMIZED);
				// This is required to kill focus from Narrator
				// And put the focus back to the active window. 
				PostMessage(g_hwndMain, WM_DELAYEDMINIMIZE, 0, 0);
			}
			else 
				ShowWindow(g_hwndMain,nCmdShow);
			return TRUE;
		}
	}

	// Something failed, exit false
	return FALSE;
}


/*************************************************************************
    Function:   UnInitApp
    Purpose:    Shuts down the application
    Inputs:     none
    Returns:    TRUE if app uninitalized properly
    History:
*************************************************************************/
BOOL UnInitApp(void)
{
	SetRegistryValues();
    if (UnInitTTS())
        {
        if (UnInitMSAA())
            {
            UninitKeys();
            return TRUE;
            }
        }
    return FALSE;
}

/*************************************************************************
    Function:   SpeakString
    Purpose:    Sends a string of text to the speech engine
    Inputs:     PSZ pszSpeakText - String of ANSI characters to be spoken
                                   Speech control tags can be embedded.
    Returns:    BOOL - TRUE if string was buffered correctly
    History:
*************************************************************************/
BOOL SpeakString(TCHAR * szSpeak, BOOL forceRead)
{
    SDATA data;
    HWND hwndList;
    // Check for redundent speak, filter out, If it is not Forced read
    if ((lstrcmp(szSpeak, g_szLastStringSpoken) == 0) && (forceRead == FALSE))
        return(FALSE);

	if (szSpeak[0] == 0) // don't speak null string
		return(FALSE);

	// if exiting stop
	if (g_fAppExiting)
		return (FALSE);

    // Different string, save off
    lstrcpyn(g_szLastStringSpoken, szSpeak, MAX_TEXT);

    // The L&H speech engine for japanese, goes crazy if you pass a
    // " ", It now takes approx 1 min to come back to life. Need to remove
    // once they fix their stuff! :a-anilk
	DBPRINTF (TEXT("'%s'\r\n"),szSpeak);
	// FileWrite(szSpeak);
	FilterSpeech(szSpeak);

    if ((lstrcmp(szSpeak, TEXT(" ")) == 0) || (lstrcmp(szSpeak, TEXT("")) == 0))
    {
        DBPRINTF(TEXT("ret\r\n"));
        return FALSE;
    }
	data.dwSize = (DWORD)(lstrlen(szSpeak)+1) * sizeof(TCHAR);
	data.pData = szSpeak;
	g_pITTSCentral->TextData (CHARSET_TEXT, 0,
                              data, NULL,
                              IID_ITTSBufNotifySinkW);
	return(TRUE);
  }


/*************************************************************************
    Function:   InitTTS
    Purpose:    Starts the Text to Speech Engine
    Inputs:     none
    Returns:    BOOL - TRUE if successful
    History:
*************************************************************************/
BOOL InitTTS(void)
{
	TTSMODEINFO   ttsModeInfo;

	DBPRINTF (TEXT("InitTTS\r\n"));

	// For logon desktop, The dlls maynot be loaded, Try after 2 seconds twice
	// Else show an Error message...
	if ( (GetDesktop() == DESKTOP_WINLOGON) &&
		(waveOutGetNumDevs() == 0) )
	{
		Sleep(2000);

		if (waveOutGetNumDevs() == 0)
			Sleep(2000);
	}

	// see if there is a sound card to use
	if (waveOutGetNumDevs() == 0)
	{

		MessageBoxLoadStrings (NULL, IDS_NO_SOUNDCARD, IDS_MSR_ERROR, MB_OK);
		return FALSE;
	}
	
	// See if we have a Text To Speech engine to use, and initialize it
	// if it is there.
	memset (&ttsModeInfo, 0, sizeof(ttsModeInfo));
	g_pITTSCentral = FindAndSelect (&ttsModeInfo);
	if (!g_pITTSCentral)
	{
		MessageBoxLoadStrings (NULL, IDS_NO_TTS, IDS_MSR_ERROR, MB_OK);
		return FALSE;
	};
	return(TRUE);
}

/*************************************************************************
    Function:   UnInitTTS
    Purpose:    Shuts down the Text to Speech subsystem
    Inputs:     none
    Returns:    BOOL - TRUE if successful
    History:
*************************************************************************/
BOOL UnInitTTS(void)
{
    // Release out TTS object - if we have one
    if (g_pITTSCentral)
        g_pITTSCentral->Release();

   // Release IITSAttributes - if we have one:a-anilk
    if (g_pITTSAttributes)
        g_pITTSAttributes->Release();

	if ( pIAMM )
	{
		pIAMM->Release();
		pIAMM = NULL;
	}

    // Shut down OLE
//    CoUninitialize();

    return(TRUE);
}

/*************************************************************************
    Function:   Shutup
    Purpose:    stops speaking, flushes speech buffers
    Inputs:     none
    Returns:    none
    History:	
*************************************************************************/
void Shutup(void)
{
	HWND hwndList;

    if (g_pITTSCentral && !g_fAppExiting)
        g_pITTSCentral->AudioReset();

	hwndList = GetDlgItem(g_hwndMain, IDC_LIST1);
    SendMessage(hwndList, LB_ADDSTRING, 0, (LPARAM) TEXT("Mute"));
}

/*************************************************************************
    Function:   FindAndSelect
    Purpose:    Selects the TTS engine
    Inputs:     PTTSMODEINFO pTTSInfo - Desired mode
    Returns:    PITTSCENTRAL - Pointer to ITTSCentral interface of engine
    History:	a-anilk created
*************************************************************************/
PITTSCENTRAL FindAndSelect(PTTSMODEINFO pTTSInfo)
{
    PITTSCENTRAL    pITTSCentral;           // central interface
    HRESULT         hRes;
    WORD            voice;
	TCHAR           defaultVoice[128];
	WORD			defLangID;

	hRes = CoCreateInstance (CLSID_TTSEnumerator, NULL, CLSCTX_ALL, IID_ITTSEnum, (void**)&g_pITTSEnum);
    if (FAILED(hRes))
        return NULL;

	// Security Check, Disallow Non-Microsoft Engines on Logon desktops..
	logonCheck = (GetDesktop() == DESKTOP_WINLOGON);

    // Get the audio dest
    hRes = CoCreateInstance(CLSID_MMAudioDest,
                            NULL,
                            CLSCTX_ALL,
                            IID_IAudioMultiMediaDevice,
                            (void**)&pIAMM);
    if (FAILED(hRes))
        return NULL;     // error
	
    pIAMM->DeviceNumSet (WAVE_MAPPER);

	if ( !logonCheck )
	{
		hRes = g_pITTSEnum->Next(MAX_ENUMMODES,gaTTSInfo,&gnmodes);
		if (FAILED(hRes))
		{
			DBPRINTF(TEXT("Failed to get any TTS Engine"));
			return NULL;     // error
		}
	
		defLangID = (WORD) GetUserDefaultUILanguage();

		// If the voice needs to be changed, Check in the list of voices..
		// If found a matching language, Great. Otherwise cribb! Anyway select a 
		// voice at the end, First one id none is found...
		// If not initialized, Then we need to over ride voice
		if ( currentVoice < 0 || currentVoice >= gnmodes ) 
		{
			for (voice = 0; voice < gnmodes; voice++)
			{
				if (gaTTSInfo[voice].language.LanguageID == defLangID)
					break;
			}

			if (voice >= gnmodes)
				voice = 0;

			currentVoice = voice;
		}
		

		if( gaTTSInfo[currentVoice].language.LanguageID != defLangID )
		{
			// Error message saying that the language was not not found...AK
			MessageBoxLoadStrings (NULL, IDS_LANGW, IDS_WARNING, MB_OK);
		}

		// Pass off the multi-media-device interface as an IUnknown (since it is one)
		hRes = g_pITTSEnum->Select( gaTTSInfo[currentVoice].gModeID,
									&pITTSCentral,
									(LPUNKNOWN) pIAMM);
		if (FAILED(hRes))
			return NULL;
	}
	else
	{
		// Pass off the multi-media-device interface as an IUnknown (since it is one)
		hRes = g_pITTSEnum->Select( MSTTS_GUID,
									&pITTSCentral,
									(LPUNKNOWN) pIAMM);
		if (FAILED(hRes))
			return NULL;

	}

	hRes = pITTSCentral->QueryInterface (IID_ITTSAttributes, (void**)&g_pITTSAttributes);

	if( FAILED(hRes) )
		return NULL;
	else
    {
		GetSpeechMinMaxValues();
    }

	SetPitch (currentPitch);
	SetSpeed (currentSpeed);
	SetVolume (currentVolume);

    return pITTSCentral;
}

/*************************************************************************
    Function:
    Purpose:
    Inputs:
    Returns:
    History:
*************************************************************************/
int MessageBoxLoadStrings (HWND hWnd,UINT uIDText,UINT uIDCaption,UINT uType)
{
	TCHAR szText[1024];
	TCHAR szCaption[128];

	LoadString(g_hInst, uIDText, szText, sizeof(szText));
	LoadString(g_hInst, uIDCaption, szCaption, sizeof(szCaption));
	return (MessageBox(hWnd, szText, szCaption, uType));
}

#ifdef _DEBUG
//--------------------------------------------------------------------------
//
// This function takes the arguments and then uses wvsprintf to
// format the string into a buffer. It then uses OutputDebugString to show
// the string.
//
//--------------------------------------------------------------------------
void FAR CDECL PrintIt(LPTSTR strFmt, ...)
{
static TCHAR StringBuf[4096] = {0};
static int  len;
va_list		ArgList;

	va_start(ArgList, strFmt);
	len = wvsprintf (StringBuf,strFmt,ArgList);
	if (len > 0)
	{
		OutputDebugString (TEXT("NARRATOR: "));
		OutputDebugString (StringBuf);
	}
	va_end (ArgList);
	return;
}

#endif // _DEBUG



// AssignDeskTop() For UM
// a-anilk. 1-12-98
static BOOL  AssignDesktop(LPDWORD desktopID, LPTSTR pname)
{
    HDESK hdesk;
    wchar_t name[300];
    DWORD nl;

    *desktopID = DESKTOP_ACCESSDENIED;
    hdesk = OpenInputDesktop(0, FALSE, MAXIMUM_ALLOWED);

    if (!hdesk)
    {
        // OpenInputDesktop will mostly fail on "Winlogon" desktop
        hdesk = OpenDesktop(__TEXT("Winlogon"),0,FALSE,MAXIMUM_ALLOWED);
        if (!hdesk)
            return FALSE;
    }

    GetUserObjectInformation(hdesk,UOI_NAME,name,300,&nl);

    if (pname)
        wcscpy(pname, name);
    if (!_wcsicmp(name, __TEXT("Default")))
        *desktopID = DESKTOP_DEFAULT;
    else if (!_wcsicmp(name, __TEXT("Winlogon")))
    {
        *desktopID = DESKTOP_WINLOGON;
    }
    else if (!_wcsicmp(name, __TEXT("screen-saver")))
        *desktopID = DESKTOP_SCREENSAVER;
    else if (!_wcsicmp(name, __TEXT("Display.Cpl Desktop")))
        *desktopID = DESKTOP_TESTDISPLAY;
    else
        *desktopID = DESKTOP_OTHER;
    
	if ( CloseDesktop(GetThreadDesktop(GetCurrentThreadId())) == 0)
    {
        TCHAR str[10];
        DWORD err = GetLastError();
        wsprintf((LPTSTR)str, (LPCTSTR)"%l", err);
    }

    if ( SetThreadDesktop(hdesk) == 0)
    {
        TCHAR str[10];
        DWORD err = GetLastError();
        wsprintf((LPTSTR)str, (LPCTSTR)"%l", err);
    }

    return TRUE;
}


// InitMyProcessDesktopAccess
// a-anilk: 1-12-98
static BOOL InitMyProcessDesktopAccess(VOID)
{
  origWinStation = GetProcessWindowStation();
  userWinStation = OpenWindowStation(__TEXT("WinSta0"), FALSE, MAXIMUM_ALLOWED);

  if (!userWinStation)
    return FALSE;
  
  SetProcessWindowStation(userWinStation);
  return TRUE;
}

// ExitMyProcessDesktopAccess
// a-anilk: 1-12-98
static VOID ExitMyProcessDesktopAccess(VOID)
{
  if (origWinStation)
    SetProcessWindowStation(origWinStation);

  if (userWinStation)
  {
	CloseWindowStation(userWinStation);
    userWinStation = NULL;
  }
}

// a-anilk added
// Returns the current desktop-ID
DWORD GetDesktop()
{
    HDESK hdesk;
    TCHAR name[300];
    DWORD value, nl, desktopID = DESKTOP_ACCESSDENIED;
    HKEY reg_key;
    DWORD cbData = sizeof(DWORD);
	LONG retVal;

	// Check if we are in setup mode...
	if (SUCCEEDED( RegOpenKeyEx(HKEY_LOCAL_MACHINE, __TEXT("SYSTEM\\Setup"), 0, KEY_READ, &reg_key)) )
    {
		retVal = RegQueryValueEx(reg_key, __TEXT("SystemSetupInProgress"), 0, NULL, (LPBYTE) &value, &cbData);

		if ( (retVal== ERROR_SUCCESS) && value )
			// Setup is in progress...
			return DESKTOP_ACCESSDENIED;
	}

	hdesk = OpenInputDesktop(0, FALSE, MAXIMUM_ALLOWED);
    if (!hdesk)
    {
        // OpenInputDesktop will mostly fail on "Winlogon" desktop
        hdesk = OpenDesktop(__TEXT("Winlogon"),0,FALSE,MAXIMUM_ALLOWED);
        if (!hdesk)
            return DESKTOP_WINLOGON;
    }
    
	GetUserObjectInformation(hdesk, UOI_NAME, name, 300, &nl);
    
	if (!_wcsicmp(name, __TEXT("Default")))
        desktopID = DESKTOP_DEFAULT;

    else if (!_wcsicmp(name, __TEXT("Winlogon")))
        desktopID = DESKTOP_WINLOGON;

    else if (!_wcsicmp(name, __TEXT("screen-saver")))
        desktopID = DESKTOP_SCREENSAVER;

    else if (!_wcsicmp(name, __TEXT("Display.Cpl Desktop")))
        desktopID = DESKTOP_TESTDISPLAY;

    else
        desktopID = DESKTOP_OTHER;
    
	return desktopID;
}

//Confirmation dialog.
INT_PTR CALLBACK ConfirmProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_INITDIALOG:
			{
				if ( IsIconic(g_hwndMain) )
				{
					CenterWindow(hwnd);
				}

				return TRUE;
			}
			break;

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
				case IDOK:
					Shutup();
                    EndDialog (hwnd, IDOK);
                    return(TRUE);
                case IDCANCEL:
					Shutup();
                    EndDialog (hwnd, IDCANCEL);
                    return(TRUE);
             }
     };

     return(FALSE);  // didn't handle
}

// Centers Narrator dialogs when main window is minimized:AK
void CenterWindow(HWND hwnd)
{
	RECT rect, dRect;
	GetWindowRect(GetDesktopWindow(), &dRect);
	GetWindowRect(hwnd, &rect);
	rect.left = (dRect.right - (rect.right - rect.left))/2;
	rect.top = (dRect.bottom - (rect.bottom - rect.top))/2;

	SetWindowPos(hwnd, HWND_TOPMOST ,rect.left,rect.top,0,0,SWP_NOSIZE | SWP_NOACTIVATE);
}

// Helper method Filters smiley face utterances: AK
void FilterSpeech(TCHAR* szSpeak)
{
	// the GUID's have a Tab followed by a {0087....
	// If you find this pattern. Then donot speak that:AK
	if ( lstrlen(szSpeak) <= 3 )
		return;

	while((*(szSpeak+3)) != NULL)
	{
		if ( (*szSpeak == '(') && isalpha(*(szSpeak + 1)) && ( (*(szSpeak + 3) == ')')) )
		{
			// Replace by isAlpha drive...
			*(szSpeak + 2) = ' ';
		}

		szSpeak++;
	}
}

// Get the Security mode, Any error or default is 
// Secure Mode ON. 
/*BOOL InitMode()
{
	HKEY hKey, sKey;
	DWORD result; 
	DWORD len, type;
	DWORD value;
	
	result = RegOpenKeyEx(UMR_MACHINE_KEY, UM_REGISTRY_KEY, 0, KEY_READ, &hKey);
	
	if (result != ERROR_SUCCESS)
		return TRUE;

	len = sizeof(DWORD);

	result = RegQueryValueEx(hKey, TEXT("SecureMode"), NULL, &type,(LPBYTE)&value, &len);
	
	if ((result != ERROR_SUCCESS) || (type != REG_DWORD))
	{
		RegCloseKey(hKey);
		return TRUE;
	}
	else
	{
		if (value)
			return TRUE;
	}

	RegCloseKey(hKey);
	return FALSE;
}

// void FileWrite(LPTSTR pszTextLocal); 

// For debugging
void FileWrite(LPTSTR pszTextLocal)
{
	FILE * fp = fopen("d:\\NarLog.txt", "a+");
	fputws((const wchar_t*) pszTextLocal, fp);
	fputws((const wchar_t*) "\n", fp);
	fclose(fp);
}
*/
/*************************************************************************
    THE INFORMATION AND CODE PROVIDED HEREUNDER (COLLECTIVELY REFERRED TO
    AS "SOFTWARE") IS PROVIDED AS IS WITHOUT WARRANTY OF ANY KIND, EITHER
    EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED
    WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE. IN
    NO EVENT SHALL MICROSOFT CORPORATION OR ITS SUPPLIERS BE LIABLE FOR
    ANY DAMAGES WHATSOEVER INCLUDING DIRECT, INDIRECT, INCIDENTAL,
    CONSEQUENTIAL, LOSS OF BUSINESS PROFITS OR SPECIAL DAMAGES, EVEN IF
    MICROSOFT CORPORATION OR ITS SUPPLIERS HAVE BEEN ADVISED OF THE
    POSSIBILITY OF SUCH DAMAGES. SOME STATES DO NOT ALLOW THE EXCLUSION OR
    LIMITATION OF LIABILITY FOR CONSEQUENTIAL OR INCIDENTAL DAMAGES SO THE
    FOREGOING LIMITATION MAY NOT APPLY.
*************************************************************************/
