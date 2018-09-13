//--------------------------------------------------------------------------;
//
//  File: advaudio.cpp
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
#include "perfpage.h"
#include "dslevel.h"
#include "drivers.h"

////////////
// Globals
////////////

AUDDATA         gAudData;
HINSTANCE       ghInst;
const TCHAR *    gszHelpFile;

////////////
// Functions
////////////

STDAPI_(void) ToggleApplyButton(HWND hWnd)
{
    BOOL fChanged = FALSE;
    HWND hwndSheet;

    if (memcmp(&gAudData.stored,&gAudData.current,sizeof(CPLDATA)))
    {
        fChanged = TRUE;
    }

    hwndSheet = GetParent(hWnd);

    if (fChanged)
    {
        PropSheet_Changed(hwndSheet,hWnd);
    }
    else
    {
        PropSheet_UnChanged(hwndSheet,hWnd);
    }
}


void VerifyRanges(LPCPLDATA pData)
{
    pData->dwHWLevel        = min(pData->dwHWLevel,MAX_HW_LEVEL);
    pData->dwSRCLevel       = min(pData->dwSRCLevel,MAX_SRC_LEVEL);
    pData->dwSpeakerType    = min(pData->dwSpeakerType,MAX_SPEAKER_TYPE);
}


void GetCurrentSettings(LPAUDDATA pAD, LPTSTR szDeviceName, BOOL fRecord)
{
    HRESULT hr = E_FAIL;

    if (pAD)
    {
        CPLDATA cplData = { DEFAULT_HW_LEVEL, DEFAULT_SRC_LEVEL, SPEAKERS_DEFAULT_CONFIG, SPEAKERS_DEFAULT_TYPE };

        memset(pAD,0,sizeof(AUDDATA));

        pAD->fRecord = fRecord;

        hr = DSGetGuidFromName(szDeviceName, fRecord, &pAD->devGuid);

        if (SUCCEEDED(hr))
        {
            hr = DSGetCplValues(pAD->devGuid, fRecord, &cplData);

            if (SUCCEEDED(hr))
            {
                VerifyRanges(&cplData);
                VerifySpeakerConfig(cplData.dwSpeakerConfig,&cplData.dwSpeakerType);
            }
        }

        pAD->stored = cplData;
        pAD->current = cplData;
        pAD->fValid = SUCCEEDED(hr);
    }
}


STDAPI_(void) ApplyCurrentSettings(LPAUDDATA pAD)
{
    HRESULT hr = S_OK;

    if (pAD && pAD->fValid)        // Only apply changes if there are changes to be applied
    {
        if (memcmp(&pAD->stored,&pAD->current,sizeof(CPLDATA)))
        {
            hr = DSSetCplValues(pAD->devGuid, pAD->fRecord, &pAD->current);

            if (SUCCEEDED(hr))
            {
                pAD->stored = pAD->current;
            }
        }
    }
}

typedef BOOL (WINAPI* UPDATEDDDLG)(HWND,HINSTANCE,const TCHAR *,LPTSTR,BOOL);

STDAPI_(BOOL) RunUpgradedDialog(HWND hwnd, HINSTANCE hInst, const TCHAR *szHelpFile, LPTSTR szDeviceName, BOOL fRecord)
{
    BOOL            fUsedUpgradedDLG = FALSE;
    TCHAR            path[_MAX_PATH];
    UPDATEDDDLG        UpdatedDialog;
    HMODULE         hModule;

    GetSystemDirectory(path, sizeof(path)/sizeof(TCHAR));
    lstrcat(path, TEXT("\\DSNDDLG.DLL") );
    
    hModule = LoadLibrary(path);

    if (hModule)
    {
        UpdatedDialog = (UPDATEDDDLG) GetProcAddress( hModule,"DSAdvancedAudio");
    
        if (UpdatedDialog)
        {
            fUsedUpgradedDLG = UpdatedDialog(hwnd,hInst,szHelpFile,szDeviceName,fRecord);
        }

        FreeLibrary( hModule );
    }

    return fUsedUpgradedDLG;
}

HRESULT CheckDSPrivs(LPAUDDATA pAD, LPTSTR szDeviceName, BOOL fRecord)
{
    HRESULT hr = E_FAIL;

    if (pAD)
    {
        memset(pAD,0,sizeof(AUDDATA));

        pAD->fRecord = fRecord;

        hr = DSGetGuidFromName(szDeviceName, fRecord, &pAD->devGuid);

        if (SUCCEEDED(hr))
        {
            CPLDATA cplData = { DEFAULT_HW_LEVEL, DEFAULT_SRC_LEVEL, SPEAKERS_DEFAULT_CONFIG, SPEAKERS_DEFAULT_TYPE };
            hr = DSGetCplValues(pAD->devGuid, fRecord, &cplData);

            if (SUCCEEDED(hr))
            {
                hr = DSSetCplValues(pAD->devGuid, fRecord, &cplData);
            } //end if Get is OK
        } //end if GUID is found
    } //end if pAD is valid

    return (hr);
}

STDAPI_(void) AdvancedAudio(HWND hwnd, HINSTANCE hInst, const TCHAR *szHelpFile, LPTSTR szDeviceName, BOOL fRecord)
{
    PROPSHEETHEADER psh;
    PROPSHEETPAGE psp[2];
    int page;
    int pages;
    TCHAR str[255];
    HMODULE hModDirectSound = NULL;

    if (!RunUpgradedDialog(hwnd,hInst,szHelpFile,szDeviceName,fRecord))
    {
        //load DirectSound
        hModDirectSound = LoadLibrary(TEXT("dsound.dll"));
        if (hModDirectSound)
        {
            HRESULT hr = CheckDSPrivs(&gAudData, szDeviceName, fRecord);
            if (SUCCEEDED(hr))
            {
                ghInst = hInst;
                gszHelpFile = szHelpFile;

                GetCurrentSettings(&gAudData, szDeviceName, fRecord);

                if (!fRecord)
                {
                    pages = 2;

                    for (page = 0; page < pages; page++)
                    {
                        memset(&psp[page],0,sizeof(PROPSHEETPAGE));
                        psp[page].dwSize = sizeof(PROPSHEETPAGE);
                        psp[page].dwFlags = PSP_DEFAULT;
                        psp[page].hInstance = ghInst;

                        switch(page)
                        {
                            case 0:
                                psp[page].pszTemplate = MAKEINTRESOURCE(IDD_SPEAKERS);
                                psp[page].pfnDlgProc = (DLGPROC) SpeakerHandler;
                            break;
                            case 1:
                                psp[page].pszTemplate = MAKEINTRESOURCE(IDD_PLAYBACKPERF);
                                psp[page].pfnDlgProc = (DLGPROC) PerformanceHandler;
                            break;
                        }
                    }
                }
                else
                {
                    pages = 1;
                    memset(&psp[0],0,sizeof(PROPSHEETPAGE));
                    psp[0].dwSize = sizeof(PROPSHEETPAGE);
                    psp[0].dwFlags = PSP_DEFAULT;
                    psp[0].hInstance = ghInst;
                    psp[0].pszTemplate = MAKEINTRESOURCE(IDD_CAPTUREPERF);
                    psp[0].pfnDlgProc = (DLGPROC) PerformanceHandler;
                }

                LoadString( hInst, IDS_ADVAUDIOTITLE, str, sizeof( str )/sizeof(TCHAR) );

                memset(&psh,0,sizeof(psh));
                psh.dwSize = sizeof(psh);
                psh.dwFlags = PSH_DEFAULT | PSH_PROPSHEETPAGE; 
                psh.hwndParent = hwnd;
                psh.hInstance = ghInst;
                psh.pszCaption = str;
                psh.nPages = pages;
                psh.nStartPage = 0;
                psh.ppsp = psp;

                PropertySheet(&psh);
            }
            else
            {
                TCHAR szCaption[MAX_PATH];
                TCHAR szMessage[MAX_PATH];

                LoadString(hInst,IDS_ERROR,szCaption,sizeof(szCaption)/sizeof(TCHAR));
                LoadString(hInst,(hr == DSERR_ACCESSDENIED) ? IDS_ERROR_DSPRIVS : IDS_ERROR_DSGENERAL,szMessage,sizeof(szMessage)/sizeof(TCHAR));
                MessageBox(hwnd,szMessage,szCaption,MB_OK|MB_ICONERROR);
            }

            FreeLibrary(hModDirectSound);
        } //end if DS loaded
    }
}
