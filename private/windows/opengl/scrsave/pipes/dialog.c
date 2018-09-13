/******************************Module*Header*******************************\
* Module Name: sspipes.c
*
* Message loop and dialog box for the OpenGL-based 3D Pipes screen saver.
*
* Copyright (c) 1994 Microsoft Corporation
*
\**************************************************************************/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <commdlg.h>
#include <scrnsave.h>
#include <GL\gl.h>
#include <math.h>
#include <memory.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <sys\timeb.h>
#include <time.h>
#include <commctrl.h>
#include "sscommon.h"
#include "sspipes.h"
#include "dlgs.h"
#include "dialog.h"

//#define NEW_TEXTURE 1

BOOL bFlexMode;
BOOL bMultiPipes;

// ulJointType controls the style of the elbows.

ULONG ulJointType = JOINT_ELBOW;

// ulSurfStyle determines whether the pipe surfaces are textured.

ULONG ulSurfStyle = SURFSTYLE_SOLID;

// ulTexQuality control the texture quality.

ULONG ulTexQuality = TEXQUAL_DEFAULT;

// fTesselFact controls the how finely the surface is tesselated.  It
// varies from very course (0.0) to very fine (2.0).

float fTesselFact = 1.0f;

// If ulSurfStyle indicates a textured surface, szTexPathname specifies
// the bitmap chosen as the texture.

// Texture file(s)
TEXFILE gTexFile[MAX_TEXTURES] = {0};
int gnTextures = 0;

static void updateDialogControls(HWND hDlg);

/******************************Public*Routine******************************\
* getIniSettings
*
* Get the screen saver configuration options from .INI file/registry.
*
*  Apr. 95 [marcfo]
*   - Use ss_GetDefaultBmpFile
*
\**************************************************************************/

void 
getIniSettings()
{
    TCHAR  szDefaultBitmap[MAX_PATH];
    int    tessel;
    int idsTexture;
    int idsTexOffset;
    int i;

    // Load resources

    LoadString(hMainInstance, IDS_GENNAME, szScreenSaver, 
               sizeof(szScreenSaver) / sizeof(TCHAR));

    // Load resource strings for texture processing

    ss_LoadTextureResourceStrings();

    // Get registry settings

    if( ss_RegistrySetup( hMainInstance, IDS_SAVERNAME, IDS_INIFILE ) )
    {
        ulJointType = ss_GetRegistryInt( IDS_JOINTTYPE, JOINT_ELBOW );

        ulSurfStyle = ss_GetRegistryInt( IDS_SURFSTYLE, SURFSTYLE_SOLID );

        ulTexQuality = ss_GetRegistryInt( IDS_TEXQUAL, TEXQUAL_DEFAULT );

        tessel = ss_GetRegistryInt( IDS_TESSELATION, 0 );
        SS_CLAMP_TO_RANGE2( tessel, 0, 200 );
        fTesselFact  = (float)tessel / 100.0f;

        bFlexMode = ss_GetRegistryInt( IDS_FLEX, 0 );

        bMultiPipes = ss_GetRegistryInt( IDS_MULTIPIPES, 0 );

        // Get any textures

#ifndef NEW_TEXTURE
        // Just get one texture with old registry names
        ss_GetRegistryString( IDS_TEXTURE, 0, gTexFile[0].szPathName, MAX_PATH);
        gTexFile[0].nOffset = ss_GetRegistryInt( IDS_TEXTURE_FILE_OFFSET, 0 );
        gnTextures = 1;
#else
        gnTextures = ss_GetRegistryInt( IDS_TEXTURE_COUNT, 0 );
        SS_CLAMP_TO_RANGE2( gnTextures, 0, MAX_TEXTURES );

        idsTexture = IDS_TEXTURE0;
        idsTexOffset = IDS_TEXOFFSET0;
        for( i = 0; i < gnTextures; i++, idsTexture++, idsTexOffset++ ) {
            ss_GetRegistryString( idsTexture, 0, gTexFile[i].szPathName,
                                  MAX_PATH);
            gTexFile[i].nOffset = ss_GetRegistryInt( idsTexOffset, 0 );
        }
#endif
    }
}

/**************************************************************************\
* ConfigInit
*
\**************************************************************************/
BOOL
ss_ConfigInit( HWND hDlg )
{
    return TRUE;
}

/******************************Public*Routine******************************\
* saveIniSettings
*
* Save the screen saver configuration option to the .INI file/registry.
*
\**************************************************************************/

static void saveIniSettings(HWND hDlg)
{
    if( ss_RegistrySetup( hMainInstance, IDS_SAVERNAME, IDS_INIFILE ) )
    {
        int idsTexture;
        int idsTexOffset;
        int i;

        ss_WriteRegistryInt( IDS_JOINTTYPE, ulJointType );
        ss_WriteRegistryInt( IDS_SURFSTYLE, ulSurfStyle );
        ss_WriteRegistryInt( IDS_TEXQUAL, ulTexQuality );
        ss_WriteRegistryInt( IDS_TESSELATION, 
                    ss_GetTrackbarPos(hDlg, DLG_SETUP_TESSEL) );
        ss_WriteRegistryInt( IDS_FLEX, bFlexMode );
        ss_WriteRegistryInt( IDS_MULTIPIPES, bMultiPipes );
#ifndef NEW_TEXTURE
        ss_WriteRegistryString( IDS_TEXTURE, gTexFile[0].szPathName );
        ss_WriteRegistryInt( IDS_TEXTURE_FILE_OFFSET, gTexFile[0].nOffset );
#else
        idsTexture = IDS_TEXTURE0;
        idsTexOffset = IDS_TEXOFFSET0;
        for( i = 0; i < gnTextures; i++, idsTexture++, idsTexOffset++ ) {
            ss_WriteRegistryString( idsTexture, gTexFile[i].szPathName );
            ss_WriteRegistryInt( idsTexOffset, gTexFile[i].nOffset );
        }
#endif
    }
}

/******************************Public*Routine******************************\
* setupDialogControls
*
* Do initial setup of dialog controls.
*
* History:
*  Jan. 95 -by- Marc Fortier [marcfo]
* Wrote it.
\**************************************************************************/

static void 
setupDialogControls(HWND hDlg)
{
    int pos;
    int wTmp;
    TCHAR szStr[GEN_STRING_SIZE];
    int idsJointType;

    InitCommonControls();

    pos = (int)(fTesselFact * 100.0f);
    ss_SetupTrackbar( hDlg, DLG_SETUP_TESSEL, 0, 200, 1, 10, pos );

    // setup jointType combo box
    idsJointType = IDS_JOINT_ELBOW;
    for (wTmp = 0; wTmp < NUM_JOINTTYPES; wTmp++, idsJointType++) {
        LoadString(hMainInstance, idsJointType, szStr, 
                    GEN_STRING_SIZE);
        SendDlgItemMessage(hDlg, DLG_COMBO_JOINTTYPE, CB_ADDSTRING, 0,
                           (LPARAM) szStr);
    }
    SendDlgItemMessage(hDlg, DLG_COMBO_JOINTTYPE, CB_SETCURSEL, 
                       ulJointType, 0);

    updateDialogControls( hDlg );
}

/******************************Public*Routine******************************\
* updateDialogControls
*
* Setup the dialog controls based on the current global state.
*
\**************************************************************************/

static void updateDialogControls(HWND hDlg)
{
    BOOL bTexture = (ulSurfStyle ==  SURFSTYLE_TEX);

    CheckDlgButton( hDlg, IDC_RADIO_SOLID, ulSurfStyle == SURFSTYLE_SOLID );
    CheckDlgButton( hDlg, IDC_RADIO_TEX  , bTexture );
#ifdef NEED_THESE_LATER
    CheckDlgButton( hDlg, IDC_RADIO_WIREFRAME, ulSurfStyle == SURFSTYLE_WIREFRAME   );

    CheckDlgButton( hDlg, IDC_RADIO_TEXQUAL_DEFAULT ,
                   bTexture && ulTexQuality == TEXQUAL_DEFAULT);
    CheckDlgButton( hDlg, IDC_RADIO_TEXQUAL_HIGH,
                   bTexture && ulTexQuality == TEXQUAL_HIGH);
#endif

    CheckDlgButton( hDlg, IDC_RADIO_NORMAL, !bFlexMode);
    CheckDlgButton( hDlg, IDC_RADIO_FLEX, bFlexMode);

    CheckDlgButton( hDlg, IDC_RADIO_SINGLE_PIPE, !bMultiPipes);
    CheckDlgButton( hDlg, IDC_RADIO_MULTIPLE_PIPES, bMultiPipes);

    EnableWindow( GetDlgItem(hDlg, DLG_COMBO_JOINTTYPE), !bFlexMode);
    EnableWindow( GetDlgItem(hDlg, IDC_STATIC_JOINTTYPE), !bFlexMode);

    EnableWindow( GetDlgItem(hDlg, DLG_SETUP_TEXTURE), bTexture );
    EnableWindow( GetDlgItem(hDlg, IDC_RADIO_TEXQUAL_DEFAULT), bTexture );
    EnableWindow( GetDlgItem(hDlg, IDC_RADIO_TEXQUAL_HIGH), bTexture );
    EnableWindow( GetDlgItem(hDlg, IDC_STATIC_TEXQUAL_GRP), bTexture );

    EnableWindow(GetDlgItem(hDlg, DLG_SETUP_TESSEL), TRUE);
    EnableWindow(GetDlgItem(hDlg, IDC_STATIC_TESS_MIN), TRUE);
    EnableWindow(GetDlgItem(hDlg, IDC_STATIC_TESS_MAX), TRUE);
    EnableWindow(GetDlgItem(hDlg, IDC_STATIC_TESS_GRP), TRUE);
}

BOOL WINAPI RegisterDialogClasses(HANDLE hinst)
{
    return TRUE;
}


/******************************Public*Routine******************************\
* ScreenSaverConfigureDialog
*
* Screen saver setup dialog box procedure.
*  Apr. 95 [marcfo]
*   - Use ss_SelectTextureFile
*
\**************************************************************************/

BOOL ScreenSaverConfigureDialog(HWND hDlg, UINT message,
                                WPARAM wParam, LPARAM lParam)
{
    int wTmp;
    int optMask = 1;

    switch (message)
    {
        case WM_INITDIALOG:
            getIniSettings();
            setupDialogControls(hDlg);
            return TRUE;

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDC_RADIO_SOLID:
                case IDC_RADIO_TEX:
                case IDC_RADIO_WIREFRAME:
                    ulSurfStyle = IDC_TO_SURFSTYLE(LOWORD(wParam));
                    break;

                case IDC_RADIO_TEXQUAL_DEFAULT:
                case IDC_RADIO_TEXQUAL_HIGH:
                    ulTexQuality = IDC_TO_TEXQUAL(LOWORD(wParam));
                    break;

                case IDC_RADIO_NORMAL:
                    bFlexMode = FALSE;
                    break;
                case IDC_RADIO_FLEX:
                    bFlexMode = TRUE;
                    break;

                case IDC_RADIO_SINGLE_PIPE:
                    bMultiPipes = FALSE;
                    break;
                case IDC_RADIO_MULTIPLE_PIPES:
                    bMultiPipes = TRUE;
                    break;

                case DLG_SETUP_TEXTURE:
                    // Run choose texture dialog
#if 1
                    ss_SelectTextureFile( hDlg, &gTexFile[0] );
#else
                    ss_SelectTextureFile( hDlg, &gTexFile[0] );
                    // NEW_TEXTURE dialog box
#endif
                    break;

                case DLG_COMBO_JOINTTYPE:
                    switch (HIWORD(wParam))
                    {
                        case CBN_EDITCHANGE:
                        case CBN_SELCHANGE:
                            ulJointType = 
                                (ULONG)SendDlgItemMessage(hDlg, DLG_COMBO_JOINTTYPE,
                                                          CB_GETCURSEL, 0, 0);
                            break;
                        default:
                            return FALSE;
                    }
                    break;

                case IDOK:
                    saveIniSettings(hDlg);
                    EndDialog(hDlg, TRUE);
                    break;

                case IDCANCEL:
                    EndDialog(hDlg, FALSE);
                    break;

                default:
                    break;
            }
            updateDialogControls(hDlg);
            return TRUE;

        default:
            return 0;
    }
    return 0;
}
