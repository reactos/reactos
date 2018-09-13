//--------------------------------------------------------------------------;
//
//  File: speakers.cpp
//
//  Copyright (c) 1997 Microsoft Corporation.  All rights reserved 
//
//--------------------------------------------------------------------------;

#include "mmcpl.h"
#include <windowsx.h>
#ifdef DEBUG
#undef DEBUG
#include <mmsystem.h>
#define DEBUG
#else
#include <mmsystem.h>
#endif
#include <commctrl.h>
#include <prsht.h>
#include <regstr.h>
#include "utils.h"
#include "medhelp.h"

#include <dsound.h>
#include "advaudio.h"
#include "speakers.h" 

/////////////
// defines
/////////////

#define NUMCONFIG   (MAX_SPEAKER_TYPE + 1)


//////////                                           
// Globals
//////////

DWORD gdwSpeakerTable[NUMCONFIG] =
{ 
    DSSPEAKER_HEADPHONE,
    SPEAKERS_DEFAULT_CONFIG,
    DSSPEAKER_MONO,
    DSSPEAKER_COMBINED(DSSPEAKER_STEREO, DSSPEAKER_GEOMETRY_NARROW),
    DSSPEAKER_COMBINED(DSSPEAKER_STEREO, DSSPEAKER_GEOMETRY_NARROW),
    DSSPEAKER_COMBINED(DSSPEAKER_STEREO, DSSPEAKER_GEOMETRY_NARROW),
    DSSPEAKER_COMBINED(DSSPEAKER_STEREO, DSSPEAKER_GEOMETRY_WIDE),
    DSSPEAKER_COMBINED(DSSPEAKER_STEREO, DSSPEAKER_GEOMETRY_NARROW),
    DSSPEAKER_QUAD,
    DSSPEAKER_SURROUND,
    DSSPEAKER_5POINT1 
};

DWORD gdwImageID[NUMCONFIG] =     
{ 
    IDB_HEADPHONES,
    IDB_STEREODESKTOP,
    IDB_MONOLAPTOP,
    IDB_STEREOLAPTOP,
    IDB_STEREOMONITOR, 
    IDB_STEREOCPU,
    IDB_MOUNTEDSTEREO,
    IDB_STEREOKEYBOARD,
    IDB_QUADRAPHONIC,
    IDB_SURROUND,
    IDB_SURROUND_5_1
};

HBITMAP ghBitmaps[NUMCONFIG];



//////////////
// Help ID's
//////////////


#pragma data_seg(".text")
const static DWORD aAdvSpeakerHelp[] = 
{
    IDC_SPEAKERCONFIG,      IDH_SPEAKERS_PICKER,
    IDC_IMAGEFRAME,         IDH_SPEAKERS_IMAGE,
    IDC_ICON_4,             IDH_COMM_GROUPBOX,
    IDC_TEXT_22,            IDH_COMM_GROUPBOX,
    IDC_TEXT_23,            IDH_SPEAKERS_PICKER,
    0, 0
};
#pragma data_seg()



//////////////
// Functions
//////////////

// 
// Verifies that the speakers type and config match, if not, change type to match config using default type
//
void VerifySpeakerConfig(DWORD dwSpeakerConfig, LPDWORD pdwSpeakerType)
{
    if (pdwSpeakerType)
    {
        DWORD dwType = *pdwSpeakerType;

        if (gdwSpeakerTable[dwType] != dwSpeakerConfig)     // the type doesn't match the config, pick a default type
        {
            switch (dwSpeakerConfig)
            {
                case DSSPEAKER_HEADPHONE:
                    *pdwSpeakerType = TYPE_HEADPHONES;
                break;

                case DSSPEAKER_MONO:
                    *pdwSpeakerType = TYPE_MONOLAPTOP;
                break;

                case DSSPEAKER_STEREO:
                    *pdwSpeakerType = TYPE_STEREODESKTOP;
                break;

                case DSSPEAKER_QUAD:
                    *pdwSpeakerType = TYPE_QUADRAPHONIC;
                break;

                case DSSPEAKER_SURROUND:
                    *pdwSpeakerType = TYPE_SURROUND;
                break;

                case DSSPEAKER_5POINT1:
                    *pdwSpeakerType = TYPE_SURROUND_5_1;
                break;

                default:
                {
                    if (DSSPEAKER_CONFIG(dwSpeakerConfig) == DSSPEAKER_STEREO)
                    {
                        DWORD dwAngle = DSSPEAKER_GEOMETRY(dwSpeakerConfig);
                        DWORD dwMiddle = DSSPEAKER_GEOMETRY_NARROW + 
                                        ((DSSPEAKER_GEOMETRY_WIDE - DSSPEAKER_GEOMETRY_NARROW) >> 1);
                        if (dwAngle <= dwMiddle)
                        {
                            *pdwSpeakerType = TYPE_STEREOCPU;        
                        }
                        else
                        {
                            *pdwSpeakerType = TYPE_STEREODESKTOP;        
                        }
                    }
                }

                break;
            }
        }
    }
}

//
// Given a speaker type, returns the DirectSound config for it
//

DWORD GetSpeakerConfigFromType(DWORD dwType)
{
    DWORD dwConfig = SPEAKERS_DEFAULT_CONFIG;

    if (dwType < (DWORD)NUMCONFIG)
    {
        dwConfig = gdwSpeakerTable[dwType];     
    }

    return(dwConfig);
}



//
// Called when first started up, it determines the current state of the device's speaker state
// and fills out the controls as appropriate
//

BOOL InitSpeakerDlg(HWND hwnd, BOOL fImagesOnly)
{
    TCHAR    str[255];
    int     item;
    DWORD   dwIndex;

    if (!fImagesOnly)
    {
        SendDlgItemMessage(hwnd, IDC_SPEAKERCONFIG, CB_RESETCONTENT,0,0);
    }

    for (item = 0; item < NUMCONFIG; item++)
    {
        if (!fImagesOnly)
        {
            LoadString( ghInst, IDS_SPEAKER1 + item, str, sizeof( str )/sizeof(TCHAR) );
            SendDlgItemMessage(hwnd, IDC_SPEAKERCONFIG, CB_INSERTSTRING,  (WPARAM) -1, (LPARAM) str); 
        }
         
        if (ghBitmaps[item] != NULL)
        {
            DeleteObject( (HGDIOBJ) ghBitmaps[item]);
            ghBitmaps[item] = NULL; 
        }
        
        ghBitmaps[item] = (HBITMAP) LoadImage(ghInst,MAKEINTATOM(gdwImageID[item]), IMAGE_BITMAP, 
                                    0, 0, LR_LOADMAP3DCOLORS);
    }
    
    dwIndex = gAudData.current.dwSpeakerType;
                    
    SendDlgItemMessage(hwnd, IDC_SPEAKERCONFIG, CB_SETCURSEL,  (WPARAM) dwIndex, 0);                
    SendDlgItemMessage(hwnd, IDC_IMAGEFRAME, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM) ghBitmaps[dwIndex]);
    
    return(TRUE);
}

//
// called to delete all loaded bitmaps
//

void DumpBitmaps(void)
{
    int item;

    for (item = 0; item < NUMCONFIG; item++)
    {         
        if (ghBitmaps[item] != NULL)
        {
            DeleteObject( (HGDIOBJ) ghBitmaps[item]);
            ghBitmaps[item] = NULL; 
        }
    }
}


//
// called by dialog handler when speaker type is changed
//

void ChangeSpeakers(HWND hwnd)
{
    DWORD dwIndex;

    dwIndex = (DWORD)SendDlgItemMessage(hwnd, IDC_SPEAKERCONFIG, CB_GETCURSEL,0,0);

	if (dwIndex != CB_ERR)
    {
        gAudData.current.dwSpeakerType = dwIndex;
        gAudData.current.dwSpeakerConfig = GetSpeakerConfigFromType(gAudData.current.dwSpeakerType);
    
        SendDlgItemMessage(hwnd, IDC_IMAGEFRAME, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM) ghBitmaps[dwIndex]);

        ToggleApplyButton(hwnd);
    }
}


//
// Dialog event handler
//

BOOL CALLBACK SpeakerHandler(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    BOOL fReturnVal = TRUE;
        
    switch (msg) 
    { 
        default:
            fReturnVal = FALSE;
        break;
        
        case WM_INITDIALOG:
        {
            fReturnVal = InitSpeakerDlg(hDlg, FALSE);
        }        
        break;

        case WM_DESTROY:
        {
            DumpBitmaps();
        }
        break;

        case WM_CONTEXTMENU:
        {      
            WinHelp((HWND)wParam, gszHelpFile, HELP_CONTEXTMENU, (UINT_PTR)(LPTSTR)aAdvSpeakerHelp);
            fReturnVal = TRUE;
        }
        break;
           
        case WM_HELP:
        {        
            WinHelp((HWND) ((LPHELPINFO)lParam)->hItemHandle, gszHelpFile, HELP_WM_HELP, (UINT_PTR)(LPTSTR)aAdvSpeakerHelp);
            fReturnVal = TRUE;
        }
        break;

        case WM_SYSCOLORCHANGE:
        {
            fReturnVal = InitSpeakerDlg(hDlg, TRUE);
        }
        break;

        case WM_COMMAND:
        {
            switch (LOWORD(wParam)) 
            {
                case IDC_SPEAKERCONFIG:
                {
                    if (HIWORD(wParam) == CBN_SELCHANGE)
                    {
                        ChangeSpeakers(hDlg);
                    }
                }
                break;
                
                default:
                    fReturnVal = FALSE;
                break;
            }
            break;
        }

        case WM_NOTIFY:
        {
            LPNMHDR pnmh = (LPNMHDR) lParam;

            switch (pnmh->code)
            {
                case PSN_APPLY:
                {
                    ApplyCurrentSettings(&gAudData);
                }
            }
        }
    }

    return fReturnVal;
}


