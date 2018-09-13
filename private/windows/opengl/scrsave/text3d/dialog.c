/******************************Module*Header*******************************\
* Module Name: dialog.c
*
* Dialog box functions for the OpenGL-based 3D Text screen saver.
*
* Created: 12-24-94 -by- Marc Fortier [marcfo]
*
* Copyright (c) 1994 Microsoft Corporation
\**************************************************************************/

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
#include "sscommon.h"
#include "sstext3d.h"

// Global screen saver settings.

static int idsStyles[NUM_DEMOS] =
    {IDS_DEMO_STRING, IDS_DEMO_CLOCK};

static int idsRotStyles[NUM_ROTSTYLES] =
{    IDS_ROTSTYLE_NONE,
     IDS_ROTSTYLE_SEESAW,
     IDS_ROTSTYLE_WOBBLE,
     IDS_ROTSTYLE_RANDOM
};

// local funtions

LONG WndProc(HWND, UINT, WPARAM, LPARAM);
static void InitAttrContext( AttrContext *pac );
static void getFont( HWND hDlg);
static void updateDialogControls(HWND hDlg);


/******************************Public*Routine******************************\
* getIniSettings
*
* Get the screen saver configuration options from .INI file/registry.
*
* History:
*  Jan. 95 -by- Marc Fortier [marcfo]
*    - Wrote it.
*  Apr. 28, 95 : [marcfo]
*    - Call common function ss_GetDefaultBmp to get a default bmp file
*  Nov. 95 [marcfo]
*    - Use ss registry helper functions
*
\**************************************************************************/

void 
getIniSettings()
{
    int    options;
    int    optMask = 1;
    TCHAR  szDefaultBitmap[MAX_PATH];
    int    iPos;
    TCHAR  szDefaultFont[256];
    TCHAR  szDefCharSet[16];
    int    iDefCharSet;

    // Initialize the global AttrContext

    InitAttrContext( &gac );

    // Load resources

    LoadString(hMainInstance, IDS_GENNAME, szScreenSaver, 
               sizeof(szScreenSaver) / sizeof(TCHAR));
    if (LoadString(hMainInstance, IDS_DEFFONT, szDefaultFont,
                   sizeof(szDefaultFont) / sizeof(TCHAR)) == 0)
    {
#ifdef UNICODE
        wcscpy(szDefaultFont, TEXT("Tahoma"));
#else
        strcpy(szDefaultFont, TEXT("Tahoma"));
#endif
    }
    iDefCharSet = ANSI_CHARSET;
    if (LoadString(hMainInstance, IDS_DEFCHARSET, szDefCharSet,
                   sizeof(szDefCharSet) / sizeof(TCHAR)) != 0)
    {
#ifdef UNICODE
        iDefCharSet = _wtoi(szDefCharSet);
#else
        iDefCharSet = atoi(szDefCharSet);
#endif
    }

    // Get registry settings

    if( ss_RegistrySetup( hMainInstance, IDS_SAVERNAME, IDS_INIFILE ) )
    {
        // get demo type

        gac.demoType = ss_GetRegistryInt( IDS_DEMOTYPE, DEMO_STRING );
        SS_CLAMP_TO_RANGE2( gac.demoType, 0, MAX_DEMO);

        // get rotation style

        gac.rotStyle = ss_GetRegistryInt( IDS_ROTSTYLE, ROTSTYLE_RANDOM );
        SS_CLAMP_TO_RANGE2( gac.rotStyle, 0, NUM_ROTSTYLES-1 );

        // get tesselation

        iPos = ss_GetRegistryInt( IDS_TESSELATION, 10 );
        SS_CLAMP_TO_RANGE2( iPos, MIN_SLIDER, MAX_SLIDER );
        gac.fTesselFact  = (float)iPos / 100.0f;

        // get size

        gac.uSize = ss_GetRegistryInt( IDS_SIZE, 50 );
        SS_CLAMP_TO_RANGE2( gac.uSize, MIN_SLIDER, MAX_SLIDER );

        // get speed

        gac.iSpeed = ss_GetRegistryInt( IDS_SPEED, 50 );
        SS_CLAMP_TO_RANGE2( gac.iSpeed, MIN_SLIDER, MAX_SLIDER );

        // get surface style

        gac.surfStyle = ss_GetRegistryInt( IDS_SURFSTYLE, SURFSTYLE_SOLID );
        SS_CLAMP_TO_RANGE2(gac.surfStyle, 0, SURFSTYLE_TEX);

        // get font, attributes, and charset

        ss_GetRegistryString( IDS_FONT, szDefaultFont,
                              gac.szFontName, LF_FACESIZE );

        options = ss_GetRegistryInt( IDS_FONT_ATTRIBUTES, 0 );
        if( options >= 0 ) {
            optMask = 1;
            gac.bBold = ((options & optMask) != 0); 
            optMask <<=1;
            gac.bItalic = ((options & optMask) != 0); 
        }

        gac.charSet = (BYTE)ss_GetRegistryInt( IDS_CHARSET, iDefCharSet );

        // get display string

        ss_GetRegistryString( IDS_TEXT, TEXT("OpenGL"), gac.szText, TEXT_LIMIT+1);

        // Determine the default .bmp file

        ss_GetDefaultBmpFile( szDefaultBitmap );


        // Is there a texture specified in the registry that overrides the
        // default?

        ss_GetRegistryString( IDS_TEXTURE, szDefaultBitmap, gac.texFile.szPathName,
                              MAX_PATH);

        gac.texFile.nOffset = ss_GetRegistryInt( IDS_TEXTURE_FILE_OFFSET, 0 );
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

/**************************************************************************\
* InitAttrContext
*
* Initialize some of the values in the attribute context
*
* History:
*  Jan. 95 -by- Marc Fortier [marcfo]
* Wrote it.
\**************************************************************************/
static void
InitAttrContext( AttrContext *pac )
{
    // set some default values

    pac->demoType = DEMO_STRING;
    pac->surfStyle = SURFSTYLE_SOLID;
    pac->fTesselFact = 1.0f;
    pac->uSize = 50;
    pac->iSpeed = 50;
    pac->texFile.szPathName[0] = '\0';
    pac->texFile.nOffset = 0;
}

/******************************Public*Routine******************************\
* saveIniSettings
*
* Save the screen saver configuration option to the .INI file/registry.
*
* History:
*  Jan. 95 -by- Marc Fortier [marcfo]
* Wrote it.
*  Nov. 95 [marcfo]
*    - Use ss registry helper functions
\**************************************************************************/

static void 
saveIniSettings(HWND hDlg)
{
    int options;
    int optMask = 1;

    GetWindowText( GetDlgItem(hDlg, DLG_TEXT_ENTER), gac.szText, TEXT_LIMIT+1);

    if( ss_RegistrySetup( hMainInstance, IDS_SAVERNAME, IDS_INIFILE ) )
    {
        ss_WriteRegistryInt( IDS_DEMOTYPE, gac.demoType );
        ss_WriteRegistryInt( IDS_ROTSTYLE, gac.rotStyle );
        ss_WriteRegistryInt( IDS_TESSELATION, 
                    ss_GetTrackbarPos(hDlg, DLG_SETUP_TESSEL) );
        ss_WriteRegistryInt( IDS_SIZE,
                    ss_GetTrackbarPos(hDlg, DLG_SETUP_SIZE) );
        ss_WriteRegistryInt( IDS_SPEED, 
                    ss_GetTrackbarPos(hDlg, DLG_SETUP_SPEED) );
        ss_WriteRegistryInt( IDS_SURFSTYLE, gac.surfStyle );
        ss_WriteRegistryString( IDS_FONT, gac.szFontName );

        optMask = 1;
        options = gac.bBold ? optMask : 0;
        optMask <<= 1;
        options |= gac.bItalic ? optMask : 0;
        ss_WriteRegistryInt( IDS_FONT_ATTRIBUTES, options );

        ss_WriteRegistryInt( IDS_CHARSET, gac.charSet );
        ss_WriteRegistryString( IDS_TEXT, gac.szText );
        ss_WriteRegistryString( IDS_TEXTURE, gac.texFile.szPathName );
        ss_WriteRegistryInt( IDS_TEXTURE_FILE_OFFSET, gac.texFile.nOffset );
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

    InitCommonControls();

    // initialize sliders

    // tesselation slider

    pos = (int)(gac.fTesselFact * 100.0f);
    ss_SetupTrackbar( hDlg, DLG_SETUP_TESSEL, MIN_SLIDER, MAX_SLIDER, 1, 9, 
                      pos );

    // size slider

    ss_SetupTrackbar( hDlg, DLG_SETUP_SIZE, MIN_SLIDER, MAX_SLIDER, 1, 9, 
                      gac.uSize );

    // speed slider

    ss_SetupTrackbar( hDlg, DLG_SETUP_SPEED, MIN_SLIDER, MAX_SLIDER, 1, 9, 
                      gac.iSpeed);

    // set state of other controls

    updateDialogControls(hDlg);
}

/******************************Public*Routine******************************\
* updateDialogControls
*
* Updates dialog controls according to current state
* 
* History:
*  Jan. 95 -by- Marc Fortier [marcfo]
* Wrote it.
\**************************************************************************/

static void 
updateDialogControls(HWND hDlg)
{
    int pos;
    BOOL bTexSurf;
    BOOL bText;

    bTexSurf = (gac.surfStyle == SURFSTYLE_TEX );

    CheckDlgButton(hDlg, IDC_RADIO_SOLID, !bTexSurf );
    CheckDlgButton(hDlg, IDC_RADIO_TEX  , bTexSurf );

    // set up demo-specific configure button

    bText = (gac.demoType == DEMO_STRING) ? TRUE : FALSE;
    EnableWindow(GetDlgItem(hDlg, DLG_TEXT_ENTER), bText );
    CheckDlgButton(hDlg, IDC_DEMO_STRING, bText );
    CheckDlgButton(hDlg, IDC_DEMO_CLOCK, !bText );

    // texture: only enable if surfStyle is texture

    EnableWindow(GetDlgItem(hDlg, DLG_SETUP_TEX), bTexSurf );
}

/******************************Public*Routine******************************\
* getFont
*
* Calls ChooseFont dialog
*
* History:
*  Jan. 95 -by- Marc Fortier [marcfo]
* Wrote it.
\**************************************************************************/

static void
getFont( HWND hDlg)
{
    CHOOSEFONT cf = {0};
    LOGFONT    lf = {0};
    HFONT      hfont, hfontOld;
    HDC   hdc;

    hdc = GetDC( hDlg );

    // Create and select a font.

    cf.lStructSize = sizeof(CHOOSEFONT);
    cf.hwndOwner = hDlg;
    cf.lpLogFont = &lf;
    cf.hInstance = hMainInstance;
    cf.lpTemplateName = (LPTSTR) MAKEINTRESOURCE(IDD_FONT);
    cf.Flags = CF_SCREENFONTS | CF_INITTOLOGFONTSTRUCT | CF_TTONLY |
               CF_ENABLETEMPLATE | CF_NOSIMULATIONS;

    // setup logfont with current settings

    lstrcpy(lf.lfFaceName, gac.szFontName);
    lf.lfWeight = (gac.bBold) ? FW_BOLD : FW_NORMAL;
    lf.lfItalic = (gac.bItalic) ? (BYTE) 1 : 0;
    lf.lfCharSet = gac.charSet;
    lf.lfHeight = -37;  // value ???

    if( ChooseFont(&cf) ) {
        // retrieve settings into gac
        lstrcpy( gac.szFontName, lf.lfFaceName );
        gac.bBold = (lf.lfWeight == FW_NORMAL) ? FALSE : TRUE;
        gac.bItalic = (lf.lfItalic) ? TRUE : FALSE;
        gac.charSet = lf.lfCharSet;
    }
}

BOOL WINAPI RegisterDialogClasses(HANDLE hinst)
{
    return TRUE;
}

/******************************Public*Routine******************************\
* ScreenSaverConfigureDialog
*
* Processes messages for the configuration dialog box.
*
* History:
*  Jan. 95 -by- Marc Fortier [marcfo]
*    - Wrote it.
*  Apr. 28, 95 : [marcfo]
*    - Call common function ss_GetTextureBitmap to load bmp texture
*
\**************************************************************************/

BOOL ScreenSaverConfigureDialog(HWND hDlg, UINT message,
                                WPARAM wParam, LPARAM lParam)
{
    int wTmp;
    TCHAR szStr[GEN_STRING_SIZE];
    HANDLE hInst;
    BOOL    bEnable;
    HWND    hText;

    switch (message) {
        case WM_INITDIALOG:
            getIniSettings();
            setupDialogControls(hDlg);

            // setup rotStyle combo box
            for (wTmp = 0; wTmp < NUM_ROTSTYLES; wTmp++) {
                LoadString(hMainInstance, idsRotStyles[wTmp], szStr, 
                            GEN_STRING_SIZE);
                SendDlgItemMessage(hDlg, DLG_SETUP_ROTSTYLE, CB_ADDSTRING, 0,
                                   (LPARAM) szStr);
            }
            SendDlgItemMessage(hDlg, DLG_SETUP_ROTSTYLE, CB_SETCURSEL, 
                               gac.rotStyle, 0);

            // display current string in box
            SendDlgItemMessage( hDlg, DLG_TEXT_ENTER, EM_LIMITTEXT, TEXT_LIMIT,0);
            SetWindowText( GetDlgItem(hDlg, DLG_TEXT_ENTER), gac.szText );

            return TRUE;

        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case DLG_SETUP_TYPES:
                    switch (HIWORD(wParam))
                    {
                        case CBN_EDITCHANGE:
                        case CBN_SELCHANGE:
                            gac.demoType = 
                                (int)SendDlgItemMessage(hDlg, DLG_SETUP_TYPES,
                                                        CB_GETCURSEL, 0, 0);
                            updateDialogControls(hDlg);
                            break;
                        default:
                            break;
                    }
                    return FALSE;

                case DLG_SETUP_ROTSTYLE:
                    switch (HIWORD(wParam))
                    {
                        case CBN_EDITCHANGE:
                        case CBN_SELCHANGE:
                            gac.rotStyle = 
                                (int)SendDlgItemMessage(hDlg, DLG_SETUP_ROTSTYLE,
                                                        CB_GETCURSEL, 0, 0);
                            updateDialogControls(hDlg);
                            break;
                        default:
                            break;
                    }
                    return FALSE;

                case DLG_SETUP_TEX:
                    // Run choose texture dialog
                    ss_SelectTextureFile( hDlg, &gac.texFile );
                    break;

                case DLG_SETUP_FONT:
                    getFont(hDlg);
                    break;
                    
                case IDC_RADIO_SOLID:
                case IDC_RADIO_TEX:
                    gac.surfStyle = IDC_TO_SURFSTYLE( LOWORD(wParam) );
                    updateDialogControls(hDlg);
                    break;

                case IDC_DEMO_STRING:
                case IDC_DEMO_CLOCK:
                    gac.demoType = IDC_TO_DEMOTYPE( LOWORD(wParam) );
                    updateDialogControls(hDlg);
                    if( LOWORD(wParam) == IDC_DEMO_STRING ) {
                        // set selected text focus
                        SetFocus( GetDlgItem(hDlg, DLG_TEXT_ENTER) );
                        SendDlgItemMessage( hDlg, DLG_TEXT_ENTER, 
                                            EM_SETSEL, 0, -1 );
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
            return TRUE;
            break;

        default:
            return 0;
    }
    return 0;
}
