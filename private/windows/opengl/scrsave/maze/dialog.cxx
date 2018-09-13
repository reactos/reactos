/******************************Module*Header*******************************\
* Module Name: dialog.cxx
*
* Dialog box functions
*
* Created: 11-16-95 -by- Marc Fortier [marcfo]
*
* Copyright (c) 1995 Microsoft Corporation
\**************************************************************************/

#include "pch.c"
#pragma hdrstop

#include <commdlg.h>
#include <commctrl.h>
#include <scrnsave.h>
#include "mazedlg.h"

#include "sscommon.hxx"

// Global string buffers for message box.

// local funtions

static void updateDialogControls(HWND hDlg);

HWND    ghDlg; // main dialog window handle
int     giSize;
int     giImageQual;
BOOL    gbTurboMode;
int     gnBitsPerPixel;

static void DrawTexture( int surface );
static void CleanUp( HWND hwnd );

// Texture information for each surface: used in both configure and run mode

TEX_INFO gTexInfo[NUM_SURFACES] = {0};

// default surface texture cache
static TEXTURE gDefTex[NUM_DEF_SURFACE_TEXTURES] = {0};

// Per-surface texture information relevant only to configure mode

typedef struct {
    TEXTURE userTex;
    int  iDefTex;
    int  defTexIDS;
    int  fileIDS;
    int  offsetIDS;
    int  dlgSpinTexID;
    int  iPalRot;  // texture palette rotation index
    SS_TEX_BUTTON *pTexBtn;  // GL texture drawing button thing
} TEX_DLG;

static TEX_DLG gTex[NUM_SURFACES] = {
    { 
      {0},
      BRICK_TEXTURE,
      IDS_DEF_WALL_TEXTURE, 
      IDS_WALL_TEXTURE_FILE, 
      IDS_WALL_TEXTURE_OFFSET,
      DLG_SPIN_WALLS,
      NULL
    },
    { 
      {0},
      WOOD_TEXTURE,
      IDS_DEF_FLOOR_TEXTURE, 
      IDS_FLOOR_TEXTURE_FILE, 
      IDS_FLOOR_TEXTURE_OFFSET,
      DLG_SPIN_FLOOR,
      NULL
    },
    { 
      {0},
      CASTLE_TEXTURE,
      IDS_DEF_CEILING_TEXTURE, 
      IDS_CEILING_TEXTURE_FILE, 
      IDS_CEILING_TEXTURE_OFFSET,
      DLG_SPIN_CEILING,
      NULL
    }
};

static UINT idTimer = 0;

static BOOL CALLBACK TextureConfigureDialog(HWND hDlg, UINT message,
                            WPARAM wParam, LPARAM lParam);

/******************************Public*Routine******************************\
* getIniSettings
*
* - Get the screen saver configuration options from .INI file/registry.
* - Called by both dialog box and screen saver
*
* History:
*  Nov. 95 [marcfo]
*    - Creation
*
\**************************************************************************/

void 
getIniSettings()
{
    int    option;
    int    i;

    // Load resources

    LoadString(hMainInstance, IDS_GENNAME, szScreenSaver, 
               sizeof(szScreenSaver) / sizeof(TCHAR));

    // Get registry settings

    if( ss_RegistrySetup( hMainInstance, IDS_SAVERNAME, IDS_INIFILE ) )
    {
        // get wall/floor/ceiling texture enables
        // For now, texturing always on
        for( i = 0; i < NUM_SURFACES; i++ )
            gTexInfo[i].bTex = TRUE;

        option = ss_GetRegistryInt( IDS_DEFAULT_TEXTURE_ENABLE, (1 << NUM_SURFACES)-1 );
        for( i = 0; i < NUM_SURFACES; i++, option >>= 1 )
            gTexInfo[i].bDefTex = option & 1;

        // get default texture indices

        for( i = 0; i < NUM_SURFACES; i++ ) {
            gTexInfo[i].iDefTex = 
                    ss_GetRegistryInt( gTex[i].defTexIDS, gTex[i].iDefTex );
            SS_CLAMP_TO_RANGE2( gTexInfo[i].iDefTex, 0, 
                                                NUM_DEF_SURFACE_TEXTURES-1 );
        }

        // get user texture files

        for( i = 0; i < NUM_SURFACES; i++ ) {
            ss_GetRegistryString( gTex[i].fileIDS, 0, 
                                  gTexInfo[i].texFile.szPathName, MAX_PATH);
            gTexInfo[i].texFile.nOffset = ss_GetRegistryInt( gTex[i].offsetIDS, 0 );
        }

        // get overlay
        maze_options.top_view = ss_GetRegistryInt( IDS_OVERLAY, 0 );

        // Get rat population
        maze_options.nrats = ss_GetRegistryInt(IDS_NRATS, 1);
        
        // get image quality

        giImageQual = ss_GetRegistryInt( IDS_IMAGEQUAL, IMAGEQUAL_DEFAULT );
        SS_CLAMP_TO_RANGE2( giImageQual, IMAGEQUAL_DEFAULT, IMAGEQUAL_HIGH );

        // get size

        giSize = ss_GetRegistryInt( IDS_SIZE, 0 );
        SS_CLAMP_TO_RANGE2( giSize, MIN_SLIDER, MAX_SLIDER );

        // get turbo mode
        gbTurboMode = ss_GetRegistryInt( IDS_TURBOMODE, 1 );
    }
}


/******************************Public*Routine******************************\
* saveIniSettings
*
* Save the screen saver configuration option to the .INI file/registry.
*
* History:
*  Nov. 95 [marcfo]
*    - Creation
\**************************************************************************/

static void 
saveIniSettings(HWND hDlg)
{
    if( ss_RegistrySetup( hMainInstance, IDS_SAVERNAME, IDS_INIFILE ) )
    {
        int i, option = 0;

        // write enables

        for( i = NUM_SURFACES-1, option = 0; i >= 0; i--, option <<= 1 )
            option |= gTexInfo[i].bDefTex & 1;
        ss_WriteRegistryInt( IDS_DEFAULT_TEXTURE_ENABLE, option >>= 1 );

        // Write default texture indices

        for( i = 0; i < NUM_SURFACES; i++ ) {
            ss_WriteRegistryInt( gTex[i].defTexIDS, gTexInfo[i].iDefTex );
        }

        // write user texture files

        for( i = 0; i < NUM_SURFACES; i++ ) {
            ss_WriteRegistryString( gTex[i].fileIDS,
                                    gTexInfo[i].texFile.szPathName );
            ss_WriteRegistryInt( gTex[i].offsetIDS, gTexInfo[i].texFile.nOffset);
        }

        // write size
        ss_WriteRegistryInt( IDS_SIZE, 
                    ss_GetTrackbarPos(hDlg, DLG_SLIDER_SIZE) );

        // write overlay enable
        ss_WriteRegistryInt( IDS_OVERLAY, maze_options.top_view );

        // Write rat population
        ss_WriteRegistryInt( IDS_NRATS, maze_options.nrats );
        
        // write image quality
        ss_WriteRegistryInt( IDS_IMAGEQUAL, giImageQual );

        // turbot mod
        ss_WriteRegistryInt( IDS_TURBOMODE, gbTurboMode );
    }
}

/******************************Public*Routine******************************\
* setupDialogControls
*
* Do initial setup of dialog controls.
*
* History:
*  Nov. 95 [marcfo]
*    - Creation
\**************************************************************************/

static void 
setupDialogControls(HWND hDlg)
{
    int i;
    int idsImageQual;
    TCHAR szStr[GEN_STRING_SIZE];

    InitCommonControls();

    // setup size slider

    ss_SetupTrackbar( hDlg, DLG_SLIDER_SIZE, MIN_SLIDER, MAX_SLIDER, 1, 9, 
                      giSize );

    // setup default texture spins

    for( i = 0; i < NUM_SURFACES; i ++ ) {
        SendDlgItemMessage( hDlg, gTex[i].dlgSpinTexID, UDM_SETRANGE, 0, 
                            MAKELONG(NUM_DEF_SURFACE_TEXTURES-1, 0) );
        SendDlgItemMessage( hDlg, gTex[i].dlgSpinTexID, UDM_SETPOS, 0, 
                            MAKELONG(gTexInfo[i].iDefTex, 0) );
    }

    // setup image quality combo box

    idsImageQual = IDS_IMAGEQUAL_DEFAULT;
    for( i = 0; i < IMAGEQUAL_COUNT; i++, idsImageQual++ ) {
        LoadString(hMainInstance, idsImageQual, szStr, 
                    GEN_STRING_SIZE);
        SendDlgItemMessage(hDlg, DLG_COMBO_IMAGEQUAL, CB_ADDSTRING, 0,
                           (LPARAM) szStr);
    }
    SendDlgItemMessage(hDlg, DLG_COMBO_IMAGEQUAL, CB_SETCURSEL, 
                       giImageQual, 0);

    // Disable Quality box when running on > 16 bits per pixel (since it only
    // affects dithering)
    gnBitsPerPixel = GetDeviceCaps( GetDC(hDlg), BITSPIXEL );
    EnableWindow( GetDlgItem(hDlg, DLG_COMBO_IMAGEQUAL), gnBitsPerPixel <= 16 );
    EnableWindow( GetDlgItem(hDlg, IDC_STATIC_IMAGEQUAL), gnBitsPerPixel <= 16 );
        
    // set palette rotation index for each surface for a8 textures
    for( i = 0; i < NUM_SURFACES; i ++ )
        gTex[i].iPalRot = ss_iRand(0xff);

    // set state of other controls

    CheckDlgButton(hDlg, DLG_CHECK_OVERLAY, maze_options.top_view );

    CheckDlgButton(hDlg, DLG_CHECK_TURBOMODE, gbTurboMode );

    updateDialogControls(hDlg);
}

/******************************Public*Routine******************************\
* updateDialogControls
*
* Updates dialog controls according to current state
* 
* History:
*  Nov. 95 [marcfo]
*    - Creation
\**************************************************************************/

static void 
updateDialogControls(HWND hDlg)
{
    int i;
    BOOL bDither;

    static int dlgSpinTexDef[NUM_SURFACES] = { 
        DLG_SPIN_WALLS,
        DLG_SPIN_FLOOR,
        DLG_SPIN_CEILING};

    for( i = 0; i < NUM_SURFACES; i ++ ) {
        EnableWindow( GetDlgItem(hDlg, gTex[i].dlgSpinTexID), gTexInfo[i].bDefTex );
    }

    EnableWindow( GetDlgItem(hDlg, DLG_SLIDER_SIZE), !gbTurboMode );
    EnableWindow( GetDlgItem(hDlg, IDC_STATIC_SIZE), !gbTurboMode );
    EnableWindow( GetDlgItem(hDlg, IDC_STATIC_MIN), !gbTurboMode );
    EnableWindow( GetDlgItem(hDlg, IDC_STATIC_MAX), !gbTurboMode );

    // Dithering looks bad with turbo mode on and 8 bits/pixel
    bDither = (gnBitsPerPixel <= 16) && !gbTurboMode;
    EnableWindow( GetDlgItem(hDlg, DLG_COMBO_IMAGEQUAL), bDither );
    EnableWindow( GetDlgItem(hDlg, IDC_STATIC_IMAGEQUAL), bDither );
}

/******************************Public*Routine******************************\
* updateGLState
*
\**************************************************************************/

static void 
updateGLState()
{
    if( (giImageQual == IMAGEQUAL_DEFAULT) || gbTurboMode )
        glDisable( GL_DITHER );
    else
        glEnable( GL_DITHER );
}

/******************************Public*Routine******************************\
* ValidateTexture
*
*
\**************************************************************************/

static TEXTURE *
ValidateTexture( int surface ) 
{
    TEXTURE *pTex = NULL;
    TEX_INFO *pTexInfo = &gTexInfo[surface];
    extern TEX_RES gTexResSurf[]; // from glmaze.c

    if( !pTexInfo->bDefTex ) {
        // Try to draw user texture
        pTex = &gTex[surface].userTex;
        if( !pTex->data ) {
            // Load user texture - this can fail!
            // If nOffset = 0, we assume user has never specified a texture,
            // so we silently switch to the default texture.
            // If the load of user texture fails, we use default texture
            if( (pTexInfo->texFile.nOffset == 0) || 
                (! ss_LoadTextureFile( &pTexInfo->texFile, pTex )) ) {

                pTexInfo->bDefTex = TRUE;  // draw default texture resource

                // Enable the spin control
                EnableWindow( GetDlgItem(ghDlg, gTex[surface].dlgSpinTexID), 
                              TRUE );
            }
        }
    }

    if( pTexInfo->bDefTex ) {
        // Draw default texture resource
        pTex = &gDefTex[pTexInfo->iDefTex];
        if( !pTex->data ) {
            if( !ss_LoadTextureResource( &gTexResSurf[pTexInfo->iDefTex], pTex ))
                pTex = NULL;
        }
    }

    return pTex;
}

/******************************Public*Routine******************************\
* DrawTextures
*
* Draw all the textures
*
\**************************************************************************/

static void 
DrawTextures() 
{
    for( int i = 0; i < NUM_SURFACES; i++ ) {
        DrawTexture( i );
    }
}

/******************************Public*Routine******************************\
* DrawTexture
*
* Draw appropriate texture in supplied window (e.g. a button)
* - Use TEX_INFO state to determine texture
* - Load texture if not in cache
*
\**************************************************************************/

static void 
DrawTexture( int surface ) 
{
    TEXTURE *pTex;

    // Make sure valid texture loaded for this surface
    if( ! (pTex = ValidateTexture( surface )) )
        return;

    SS_TEX_BUTTON *pTexBtn = gTex[surface].pTexBtn;
    // pTexBtn never NULL, else ss_ConfigInit fails

    // If palette rotation for this texture, slam in the current rotation,
    // to be used by pTexBtn->Draw
    if( pTex->pal )
        pTex->iPalRot = gTex[surface].iPalRot;

    pTexBtn->Draw( pTex );
}

/******************************Public*Routine******************************\
* RegisterDialogClasses
*
\**************************************************************************/

BOOL WINAPI RegisterDialogClasses(HANDLE hinst)
{
    return TRUE;
}

/******************************Public*Routine******************************\
* HandlePreviewDraw
*
* History:
*  Dec. 95 [marcfo]
*    - Creation
\**************************************************************************/

static BOOL
HandlePreviewDraw( UINT idCtl, LPDRAWITEMSTRUCT lpdis )
{
    int surface = DLG_PREVIEW_TO_SURFACE( idCtl );

    if( lpdis->itemAction == ODA_DRAWENTIRE ) {
        DrawTexture( surface );
        return TRUE;
    }
    return FALSE;
}

/******************************Public*Routine******************************\
* HandleTexButtonNewTexture
*
* - Handle when a user selects a new texture
*
* If the new texture fails to load (unlikely, since we validate a good chunk
* of it with the selection dialog), then the current default texture is
* used.
\**************************************************************************/

static void
HandleTexButtonNewTexture( int surface )
{
    TEXTURE *pTex = &gTex[surface].userTex;

    // Delete the old texture
    ss_DeleteTexture( pTex );

    // Load up the new texture
    if( ! ss_LoadTextureFile( &gTexInfo[surface].texFile, pTex ) )
        gTexInfo[surface].bDefTex = TRUE;
}

/******************************Public*Routine******************************\
* MazeDlgTimerProc
*
* Runs off of WM_TIMER message.  Used for any gl animation in the dialog
*
* History:
*  Jan. 96 [marcfo]
*    - Creation
\**************************************************************************/

static void
MazeDlgTimerProc()
{
    extern TEX_RES gTexResSurf[]; // from glmaze.c

    if( ss_PalettedTextureEnabled() ) {
        TEX_INFO *pti = gTexInfo;
        int i;

        // rotate texture palettes for surfaces with a8 palettes

        for( i = 0; i < NUM_SURFACES; i++, pti++ ) {
            if( pti->bTex &&
                pti->bDefTex &&
                ( gTexResSurf[pti->iDefTex].type == TEX_A8 ) )
            {
                // Increment palette rotation for this surface
                gTex[i].iPalRot++;
                // Draw texture associated with the surface
                DrawTexture( i );
            }
        }
    }
}

/******************************Public*Routine******************************\
* ConfigInit
*
* Do Initialization for Config mode
*
* This is the config equivalent of ss_Init
*
* Setup SS_TEX_BUTTON wrappers for each of the 3 texture preview buttons.
\**************************************************************************/

BOOL
ss_ConfigInit( HWND hDlg )
{
    SS_DBGLEVEL1( SS_LEVEL_INFO, "ConfigInit for %d\n", hDlg );

    // Create GL texture buttons to draw the surface previews

    SS_TEX_BUTTON *pTexBtn;

    for( int i = 0; i < NUM_SURFACES; i++ ) {
        pTexBtn = new SS_TEX_BUTTON( hDlg,
                            GetDlgItem(hDlg, DLG_SURFACE_TO_PREVIEW(i)) );
        if( !pTexBtn ) {
            return FALSE;
        }
        gTex[i].pTexBtn = pTexBtn;
    }

    updateGLState();

    // Start a timer for animating texture palettes
    idTimer = 1;
    SetTimer(hDlg, idTimer, 16, 0);

    // Note: no textures are loaded here, they are 'demand-loaded'
    return TRUE;
}

/******************************Public*Routine******************************\
* ScreenSaverConfigureDialog
*
* Processes messages for the configuration dialog box.
*
* History:
*  Nov. 95 [marcfo]
*    - Creation
\**************************************************************************/

BOOL ScreenSaverConfigureDialog(HWND hDlg, UINT message,
                                WPARAM wParam, LPARAM lParam)
{
    int wTmp, surface;
    static BOOL bInited = 0;

    switch (message) {
        case WM_INITDIALOG:
            getIniSettings();
            setupDialogControls(hDlg);
            // cache the window handle
            ghDlg = hDlg;
            return TRUE;

        case WM_TIMER:
            MazeDlgTimerProc();
            return 0;
            break;

        case WM_DRAWITEM:
            switch( (UINT) wParam ) {
                case DLG_PREVIEW_WALLS:
                case DLG_PREVIEW_FLOOR:
                case DLG_PREVIEW_CEILING:
                    if( HandlePreviewDraw( (UINT) wParam, 
                                             (LPDRAWITEMSTRUCT) lParam ) )
                        return TRUE;
                default:
                    break;
            }
            break;

        case WM_VSCROLL:
            switch(LOWORD(wParam))
            {
                case SB_THUMBPOSITION:
                    // get new value
                    wTmp = HIWORD(wParam);
                    {
                        int id;
                        HWND hwndScroll = (HWND) lParam;

                        id = GetDlgCtrlID( hwndScroll );
                        surface = DLG_SPIN_TEX_TO_SURFACE( id );
                        gTexInfo[surface].iDefTex = wTmp;
                        DrawTexture( surface );
                    }
                    break;
                default:
                    break;
            }
            break;

        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                    
                case DLG_BUTTON_WALLS_TEX:
                case DLG_BUTTON_FLOOR_TEX:
                case DLG_BUTTON_CEILING_TEX:
                    surface = DLG_BUTTON_TEX_TO_SURFACE( LOWORD(wParam) );
                    if( DialogBoxParam( 
                        hMainInstance, 
                        (LPTSTR) MAKEINTRESOURCE( DLG_TEXTURE_CONFIGURE ),
                        hDlg, (DLGPROC)TextureConfigureDialog, surface ) ) {
                    //mf: If anything changed here, we have to draw it, as it
                    // seems when choose texture dialog box terminates, we
                    // don't get any DRAW_ITEM messages.
                    // Draw preview area
                        DrawTexture( surface );
                    }
                    break;

                case DLG_CHECK_OVERLAY:
                    maze_options.top_view = !maze_options.top_view;
                    CheckDlgButton(hDlg, DLG_CHECK_OVERLAY, 
                                   maze_options.top_view );
                    break;

                case DLG_CHECK_TURBOMODE:
                    gbTurboMode = !gbTurboMode;
                    CheckDlgButton(hDlg, DLG_CHECK_TURBOMODE, gbTurboMode );
                    updateGLState();
                    DrawTextures();
                    break;

                case DLG_COMBO_IMAGEQUAL:
                    switch (HIWORD(wParam))
                    {
                        case CBN_EDITCHANGE:
                        case CBN_SELCHANGE:
                          {
                            int oldImageQual = giImageQual; 
                            giImageQual = 
                                (int)SendDlgItemMessage(hDlg, DLG_COMBO_IMAGEQUAL,
                                                        CB_GETCURSEL, 0, 0);
                            if( giImageQual != oldImageQual ) {
                                // change has occurred - redraw any gl objects
                                updateGLState();
                                DrawTextures();
                            }
                          }
                          break;
                        default:
                            return FALSE;
                    }
                    break;

                case IDOK:
                    saveIniSettings(hDlg);
                    // fall thru...

                case IDCANCEL:
                    CleanUp( hDlg );
                    EndDialog(hDlg, FALSE);
                    break;

                default:
                    return 0;
                    break;
            }
            updateDialogControls(hDlg);
            return TRUE;
            break;

        default:
            return 0;
    }
    return 0;
}


/******************************Public*Routine******************************\
* updateTextureConfigControls
*
* Updates dialog controls according to current state
* 
* History:
*    - Creation
\**************************************************************************/

static void 
updateTextureConfigControls(HWND hDlg, BOOL bDefTex )
{
    CheckDlgButton(hDlg, IDC_RADIO_TEX_DEFAULT, bDefTex );
    CheckDlgButton(hDlg, IDC_RADIO_TEX_CHOOSE, !bDefTex );

    EnableWindow(GetDlgItem(hDlg, DLG_BUTTON_TEX_CHOOSE ), !bDefTex );
}

/******************************Public*Routine******************************\
* TextureConfigureDialog
*
* Processes messages for the texture configure dialog box.
*
* - Call EndDialog with TRUE if texture changed, otherwise FALSE
*
* History:
*  Dec. 95 [marcfo]
*    - Creation
\**************************************************************************/

static BOOL CALLBACK 
TextureConfigureDialog(HWND hDlg, UINT message,
                            WPARAM wParam, LPARAM lParam)
{
    // static state ok here, cuz only one of these dialogs can be active
    static int surface;
    static BOOL bDefTex; // temporary state

    switch (message) {
        case WM_INITDIALOG:
            // Cache some initial values
            surface = (int) lParam;
            bDefTex = gTexInfo[surface].bDefTex;
            updateTextureConfigControls(hDlg, bDefTex );
            return TRUE;

        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                    
                case DLG_BUTTON_TEX_CHOOSE:
                    if( ss_SelectTextureFile( hDlg, 
                                         &gTexInfo[surface].texFile ) ) {
                        // New user texture was selected
                        HandleTexButtonNewTexture( surface );

                        // For now, let's end the dialog right here if a
                        // new one chosen - otherwise we'd have to make the
                        // new texture discardable if user entered Cancel
                        gTexInfo[surface].bDefTex = bDefTex;
                        EndDialog(hDlg, TRUE );
                    }
                    break;

                case IDC_RADIO_TEX_DEFAULT:
                    bDefTex = TRUE;
                    break;
                    
                case IDC_RADIO_TEX_CHOOSE:
                    bDefTex = FALSE;
                    break;
                    
                case IDOK:
                    // save state
                    gTexInfo[surface].bDefTex = bDefTex;
                    EndDialog(hDlg, TRUE);
                    break;

                case IDCANCEL:
                    EndDialog(hDlg, FALSE);
                    break;

                default:
                    return 0;
                    break;
            }
            updateTextureConfigControls(hDlg, bDefTex );
            return TRUE;
            break;

        default:
            return 0;
    }
    return 0;
}

/******************************Public*Routine******************************\
* CleanUp
*
\**************************************************************************/

static void
CleanUp( HWND hwnd )
{
    int i;

    if (idTimer) {
        KillTimer(hwnd, idTimer);
        idTimer = 0;
    }

    // delete any textures and tex buttons created

    for( i = 0; i < NUM_SURFACES; i ++ ) {
        if( gTex[i].userTex.data )
            ss_DeleteTexture( &gTex[i].userTex );
        if( gDefTex[i].data )
            ss_DeleteTexture( &gDefTex[i] );
        if( gTex[i].pTexBtn )
            delete gTex[i].pTexBtn;
    }

}
