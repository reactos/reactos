/******************************Module*Header*******************************\
* Module Name: ss3dfo.c
*
* Dispatcher and dialog box for the OpenGL-based 3D Flying Objects screen
* saver.
*
* Created: 18-May-1994 14:54:59
*
* Copyright (c) 1994 Microsoft Corporation
\**************************************************************************/

#include <windows.h>
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
#include <commdlg.h>
#include <commctrl.h>
#include "ss3dfo.h"

//#define SS_DEBUG 1

// Global strings.
#define GEN_STRING_SIZE 64

static SSContext gssc;
BOOL gbBounce = FALSE; // floating window bounce off side

// Global message loop variables.
MATERIAL Material[16];
#ifdef MEMDEBUG
ULONG totalMem = 0;
#endif

// Global screen saver settings.

void (*updateSceneFunc)(int); // current screen saver update function
void (*delSceneFunc)(void);         // current screen saver deletion function
BOOL bColorCycle = FALSE;           // color cycling flag
BOOL bSmoothShading = TRUE;         // smooth shading flag
UINT uSize = 100;                   // object size
float fTesselFact = 1.0f;           // object tessalation
int UpdateFlags = 0;                // extra data sent to update function
int Type = 0;                       // screen saver index (into function arrays)

// Texture file information
TEXFILE gTexFile = {0};

// Lighting properties.

static const RGBA lightAmbient   = {0.21f, 0.21f, 0.21f, 1.0f};
static const RGBA light0Ambient  = {0.0f, 0.0f, 0.0f, 1.0f};
static const RGBA light0Diffuse  = {0.7f, 0.7f, 0.7f, 1.0f};
static const RGBA light0Specular = {1.0f, 1.0f, 1.0f, 1.0f};
static const GLfloat light0Pos[]      = {100.0f, 100.0f, 100.0f, 0.0f};

// Material properties.

static RGBA matlColors[7] = {{1.0f, 0.0f, 0.0f, 1.0f},
                             {0.0f, 1.0f, 0.0f, 1.0f},
                             {0.0f, 0.0f, 1.0f, 1.0f},
                             {1.0f, 1.0f, 0.0f, 1.0f},
                             {0.0f, 1.0f, 1.0f, 1.0f},
                             {1.0f, 0.0f, 1.0f, 1.0f},
                             {0.235f, 0.0f, 0.78f, 1.0f},
                            };

extern void updateStripScene(int);
extern void updateDropScene(int);
extern void updateLemScene(int);
extern void updateExplodeScene(int);
extern void updateWinScene(int);
extern void updateTexScene(int);
extern void initStripScene(void);
extern void initDropScene(void);
extern void initLemScene(void);
extern void initExplodeScene(void);
extern void initWinScene(void);
extern void initTexScene(void);
extern void delStripScene(void);
extern void delDropScene(void);
extern void delLemScene(void);
extern void delExplodeScene(void);
extern void delWinScene(void);
extern void delTexScene(void);

typedef void (*PTRUPDATE)(int);
typedef void (*ptrdel)();
typedef void (*ptrinit)();

// Each screen saver style puts its hook functions into the function
// arrays below.  A consistent ordering of the functions is required.

static PTRUPDATE updateFuncs[] =
    {updateWinScene, updateExplodeScene,updateStripScene, updateStripScene,
     updateDropScene, updateLemScene, updateTexScene};
static ptrdel delFuncs[] =
    {delWinScene, delExplodeScene, delStripScene, delStripScene,
     delDropScene, delLemScene, delTexScene};
static ptrinit initFuncs[] =
    {initWinScene, initExplodeScene, initStripScene, initStripScene,
     initDropScene, initLemScene, initTexScene};
static int idsStyles[] =
    {IDS_LOGO, IDS_EXPLODE, IDS_RIBBON, IDS_2RIBBON,
     IDS_SPLASH, IDS_TWIST, IDS_FLAG};

#define MAX_TYPE    ( sizeof(initFuncs) / sizeof(ptrinit) - 1 )

// Each screen saver style can choose which dialog box controls it wants
// to use.  These flags enable each of the controls.  Controls not choosen
// will be disabled.

#define OPT_COLOR_CYCLE     0x00000001
#define OPT_SMOOTH_SHADE    0x00000002
#define OPT_TESSEL          0x00000008
#define OPT_SIZE            0x00000010
#define OPT_TEXTURE         0x00000020
#define OPT_STD             ( OPT_COLOR_CYCLE | OPT_SMOOTH_SHADE | OPT_TESSEL | OPT_SIZE )

static ULONG gflConfigOpt[] = {
     OPT_STD,               // Windows logo
     OPT_STD,               // Explode
     OPT_STD,               // Strip
     OPT_STD,               // Strip
     OPT_STD,               // Drop
     OPT_STD,               // Twist (lemniscate)
     OPT_SMOOTH_SHADE | OPT_TESSEL | OPT_SIZE | OPT_TEXTURE  // Texture mapped flag
};

static void updateDialogControls(HWND hDlg);

#ifdef MEMDEBUG
void xprintf(char *str, ...)
{
    va_list ap;
    char buffer[256];

    va_start(ap, str);
    vsprintf(buffer, str, ap);

    OutputDebugString(buffer);
    va_end(ap);
}
#endif

void *SaverAlloc(ULONG size)
{
    void *mPtr;

    mPtr = malloc(size);
#ifdef MEMDEBUG
    totalMem += size;
    xprintf("malloc'd %x, size %d\n", mPtr, size);
#endif
    return mPtr;
}

void SaverFree(void *pMem)
{
#ifdef MEMDEBUG
    totalMem -= _msize(pMem);
    xprintf("free %x, size = %d, total = %d\n", pMem, _msize(pMem), totalMem);
#endif
    free(pMem);
}

/******************************Public*Routine******************************\
* getIniSettings
*
* Get the screen saver configuration options from .INI file/registry.
*
* History:
*  10-May-1994 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

static void getIniSettings()
{
    int    options;
    int    optMask = 1;
    TCHAR  szDefaultBitmap[MAX_PATH];
    int    tessel=0;
    LPTSTR  psz;

    LoadString(hMainInstance, IDS_GENNAME, szScreenSaver, sizeof(szScreenSaver) / sizeof(TCHAR));

    if( ss_RegistrySetup( hMainInstance, IDS_SAVERNAME, IDS_INIFILE ) )
    {
        options = ss_GetRegistryInt( IDS_OPTIONS, -1 );
        if (options >= 0)
        {
            bSmoothShading = ((options & optMask) != 0);
            optMask <<= 1;
            bColorCycle = ((options & optMask) != 0);
            UpdateFlags = (bColorCycle << 1);
        }

        Type = ss_GetRegistryInt( IDS_OBJTYPE, 0 );

        // Sanity check Type.  Don't want to index into function arrays
        // with a bad index!
        Type = min(Type, MAX_TYPE);

        // Set flag so that updateStripScene will do two strips instead
        // of one.

        if (Type == 3)
            UpdateFlags |= 0x4;

        tessel = ss_GetRegistryInt( IDS_TESSELATION, 100 );
        SS_CLAMP_TO_RANGE2( tessel, 0, 200 );

        if (tessel <= 100)
            fTesselFact  = (float)tessel / 100.0f;
        else
            fTesselFact = 1.0f + (((float)tessel - 100.0f) / 100.0f);

        uSize = ss_GetRegistryInt( IDS_SIZE, 50 );
        SS_CLAMP_TO_RANGE2( uSize, 0, 100 );

        // Determine the default .bmp file

        ss_GetDefaultBmpFile( szDefaultBitmap );

        // Is there a texture specified in the registry that overrides the
        // default?


        ss_GetRegistryString( IDS_TEXTURE, szDefaultBitmap, gTexFile.szPathName,
                              MAX_PATH);

        gTexFile.nOffset = ss_GetRegistryInt( IDS_TEXTURE_FILE_OFFSET, 0 );
    }
}

/******************************Public*Routine******************************\
* saveIniSettings
*
* Save the screen saver configuration option to the .INI file/registry.
*
* History:
*  10-May-1994 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

static void saveIniSettings(HWND hDlg)
{
    if( ss_RegistrySetup( hMainInstance, IDS_SAVERNAME, IDS_INIFILE ) )
    {
        int pos, options;
        int optMask = 1;

        bSmoothShading = IsDlgButtonChecked(hDlg, DLG_SETUP_SMOOTH);
        bColorCycle = IsDlgButtonChecked(hDlg, DLG_SETUP_CYCLE);
        options = bColorCycle;
        options <<= 1;
        options |= bSmoothShading;
        ss_WriteRegistryInt( IDS_OPTIONS, options );

        Type = (int)SendDlgItemMessage(hDlg, DLG_SETUP_TYPES, CB_GETCURSEL,
                                       0, 0);
        ss_WriteRegistryInt( IDS_OBJTYPE, Type );

        pos = ss_GetTrackbarPos( hDlg, DLG_SETUP_TESSEL );
        ss_WriteRegistryInt( IDS_TESSELATION, pos );

        pos = ss_GetTrackbarPos( hDlg, DLG_SETUP_SIZE );
        ss_WriteRegistryInt( IDS_SIZE, pos );

        ss_WriteRegistryString( IDS_TEXTURE, gTexFile.szPathName );
        ss_WriteRegistryInt( IDS_TEXTURE_FILE_OFFSET, gTexFile.nOffset );
    }
}

/******************************Public*Routine******************************\
* setupDialogControls
*
* Setup the dialog controls initially.
*
\**************************************************************************/

static void 
setupDialogControls(HWND hDlg)
{
    int pos;

    InitCommonControls();

    if ( gflConfigOpt[Type] & OPT_TESSEL )
    {
        if (fTesselFact <= 1.0f)
            pos = (int)(fTesselFact * 100.0f);
        else
            pos = 100 + (int) ((fTesselFact - 1.0f) * 100.0f);

        ss_SetupTrackbar( hDlg, DLG_SETUP_TESSEL, 0, 200, 1, 10, pos );
    }

    if ( gflConfigOpt[Type] & OPT_SIZE )
    {
        ss_SetupTrackbar( hDlg, DLG_SETUP_SIZE, 0, 100, 1, 10, uSize );
    }

    updateDialogControls( hDlg );
}

/******************************Public*Routine******************************\
* updateDialogControls
*
* Update the dialog controls based on the current global state.
*
\**************************************************************************/

static void 
updateDialogControls(HWND hDlg)
{
    CheckDlgButton(hDlg, DLG_SETUP_SMOOTH, bSmoothShading);
    CheckDlgButton(hDlg, DLG_SETUP_CYCLE, bColorCycle);

    EnableWindow(GetDlgItem(hDlg, DLG_SETUP_SMOOTH),
                 gflConfigOpt[Type] & OPT_SMOOTH_SHADE );
    EnableWindow(GetDlgItem(hDlg, DLG_SETUP_CYCLE),
                 gflConfigOpt[Type] & OPT_COLOR_CYCLE );

    EnableWindow(GetDlgItem(hDlg, DLG_SETUP_TESSEL),
                 gflConfigOpt[Type] & OPT_TESSEL);
    EnableWindow(GetDlgItem(hDlg, IDC_STATIC_TESS),
                 gflConfigOpt[Type] & OPT_TESSEL);
    EnableWindow(GetDlgItem(hDlg, IDC_STATIC_TESS_MIN),
                 gflConfigOpt[Type] & OPT_TESSEL);
    EnableWindow(GetDlgItem(hDlg, IDC_STATIC_TESS_MAX),
                 gflConfigOpt[Type] & OPT_TESSEL);

    EnableWindow(GetDlgItem(hDlg, DLG_SETUP_SIZE),
                 gflConfigOpt[Type] & OPT_SIZE);
    EnableWindow(GetDlgItem(hDlg, IDC_STATIC_SIZE),
                 gflConfigOpt[Type] & OPT_SIZE);
    EnableWindow(GetDlgItem(hDlg, IDC_STATIC_SIZE_MIN),
                 gflConfigOpt[Type] & OPT_SIZE);
    EnableWindow(GetDlgItem(hDlg, IDC_STATIC_SIZE_MAX),
                 gflConfigOpt[Type] & OPT_SIZE);

    EnableWindow(GetDlgItem(hDlg, DLG_SETUP_TEXTURE),
                 gflConfigOpt[Type] & OPT_TEXTURE);
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
\**************************************************************************/

BOOL ScreenSaverConfigureDialog(HWND hDlg, UINT message,
                                WPARAM wParam, LPARAM lParam)
{
    int wTmp;
    TCHAR szString[GEN_STRING_SIZE];

    switch (message) {
        case WM_INITDIALOG:
            getIniSettings();

            setupDialogControls(hDlg);

            for (wTmp = 0; wTmp <= MAX_TYPE; wTmp++) {
                LoadString(hMainInstance, idsStyles[wTmp], szString, GEN_STRING_SIZE);
                SendDlgItemMessage(hDlg, DLG_SETUP_TYPES, CB_ADDSTRING, 0,
                                   (LPARAM) szString);
            }
            SendDlgItemMessage(hDlg, DLG_SETUP_TYPES, CB_SETCURSEL, Type, 0);

            return TRUE;


        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case DLG_SETUP_TYPES:
                    switch (HIWORD(wParam))
                    {
                        case CBN_EDITCHANGE:
                        case CBN_SELCHANGE:
                            Type = (int)SendDlgItemMessage(hDlg, DLG_SETUP_TYPES,
                                                           CB_GETCURSEL, 0, 0);
                            updateDialogControls(hDlg);
                            break;
                        default:
                            break;
                    }
                    return FALSE;

                case DLG_SETUP_TEXTURE:
                    ss_SelectTextureFile( hDlg, &gTexFile );
                    break;

                case IDOK:
                    saveIniSettings(hDlg);
                    EndDialog(hDlg, TRUE);
                    break;

                case IDCANCEL:
                    EndDialog(hDlg, FALSE);
                    break;

                case DLG_SETUP_SMOOTH:
                case DLG_SETUP_CYCLE:
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

/******************************Public*Routine******************************\
* SetFloaterInfo
*
* Set the size and motion of the floating window
* It stays square in shape
*
\**************************************************************************/

static void
SetFloaterInfo( ISIZE *pParentSize, CHILD_INFO *pChild )
{
    float sizeFact;
    float sizeScale;
    int size;
    ISIZE *pChildSize = &pChild->size;
    MOTION_INFO *pMotion = &pChild->motionInfo;

    sizeScale = (float)uSize / 100.0f; // 0..1
    sizeFact = 0.25f + (0.30f * sizeScale);
    size = (int) (sizeFact * ( ((float)(pParentSize->width + pParentSize->height)) / 2.0f ));
    SS_CLAMP_TO_RANGE2( size, 0, pParentSize->width );
    SS_CLAMP_TO_RANGE2( size, 0, pParentSize->height );

    pChildSize->width = pChildSize->height = size;

    // Floater motion
    pMotion->posInc.x = .01f * (float) size;
    if( pMotion->posInc.x < 1.0f )
        pMotion->posInc.x = 1.0f;
    pMotion->posInc.y = pMotion->posInc.x;
    pMotion->posIncVary.x = .4f * pMotion->posInc.x;
    pMotion->posIncVary.y = pMotion->posIncVary.x;
}

/******************************Public*Routine******************************\
* initMaterial
*
* Initialize the material properties.
*
\**************************************************************************/

void initMaterial(int id, float r, float g, float b, float a)
{
    Material[id].ka.r = r;
    Material[id].ka.g = g;
    Material[id].ka.b = b;
    Material[id].ka.a = a;

    Material[id].kd.r = r;
    Material[id].kd.g = g;
    Material[id].kd.b = b;
    Material[id].kd.a = a;

    Material[id].ks.r = 1.0f;
    Material[id].ks.g = 1.0f;
    Material[id].ks.b = 1.0f;
    Material[id].ks.a = 1.0f;

    Material[id].specExp = 128.0f;
}

/******************************Public*Routine******************************\
* _3dfo_Init
*
\**************************************************************************/

static void
_3dfo_Init(void *data)
{
    int i;

    for (i = 0; i < 7; i++)
        initMaterial(i, matlColors[i].r, matlColors[i].g,
                     matlColors[i].b, matlColors[i].a);


    // Set the OpenGL clear color to black.

    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
#ifdef SS_DEBUG
    glClearColor(0.2f, 0.2f, 0.2f, 0.0f);
#endif

    // Enable the z-buffer.

    glEnable(GL_DEPTH_TEST);

    // Select the shading model.

    if (bSmoothShading)
        glShadeModel(GL_SMOOTH);
    else
        glShadeModel(GL_FLAT);

    // Setup the OpenGL matrices.

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    // Setup the lighting.

    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, (GLfloat *) &lightAmbient);
    glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);
    glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, GL_FALSE);
    glLightfv(GL_LIGHT0, GL_AMBIENT, (GLfloat *) &light0Ambient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, (GLfloat *) &light0Diffuse);
    glLightfv(GL_LIGHT0, GL_SPECULAR, (GLfloat *) &light0Specular);
    glLightfv(GL_LIGHT0, GL_POSITION, light0Pos);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);

    // Setup the material properties.

    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, (GLfloat *) &Material[0].ks);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, (GLfloat *) &Material[0].specExp);

    // call specific objects init func
    (*initFuncs[Type])();
    updateSceneFunc = updateFuncs[Type];
}

/******************************Public*Routine******************************\
* _3dfo_Draw
*
\**************************************************************************/

static void
_3dfo_Draw(void *data)
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    (*updateSceneFunc)(UpdateFlags);
}

/******************************Public*Routine******************************\
* _3dfo_FloaterBounce
*
\**************************************************************************/

static void
_3dfo_FloaterBounce(void *data)
{
    gbBounce = TRUE;
}

/******************************Public*Routine******************************\
* ss_Init
*
* Screen saver entry point.  Pre-GL initialization
*
\**************************************************************************/

SSContext *
ss_Init(void)
{
    getIniSettings();

    // Make sure the selected texture file is OK.

    if ( gflConfigOpt[Type] & OPT_TEXTURE )
    {
        // Verify texture file
        ss_DisableTextureErrorMsgs();
        ss_VerifyTextureFile( &gTexFile );
    }

    // set callbacks

    ss_InitFunc( _3dfo_Init );
    ss_UpdateFunc( _3dfo_Draw );

    // set configuration info to return

    gssc.bDoubleBuf = TRUE;
    gssc.depthType = SS_DEPTH16;

    gssc.bFloater = TRUE;
    gssc.floaterInfo.bMotion = TRUE;
    gssc.floaterInfo.ChildSizeFunc = SetFloaterInfo;
    ss_FloaterBounceFunc( _3dfo_FloaterBounce );

    return &gssc;
}

/**************************************************************************\
* ConfigInit
*
* Dialog box version of ss_Init.  Used for setting up any gl drawing on the
* dialog.
*
\**************************************************************************/
BOOL
ss_ConfigInit( HWND hDlg )
{
    return TRUE;
}

