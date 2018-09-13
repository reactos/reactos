/**********************************Module**********************************\
*
* ssflwbox.c
*
* 3D FlowerBox screen saver
*
* History:
*  Wed Jul 19 14:50:27 1995	-by-	Drew Bliss [drewb]
*   Created
*
* Copyright (c) 1995 Microsoft Corporation
*
\**************************************************************************/

#include "precomp.h"
#pragma hdrstop

// Minimum and maximum image sizes
#define MINIMAGESIZE 10
#define MAXIMAGESIZE 100

// Color tables for checkboard, per-side and single color modes
GLfloat base_checker_cols[MAXSIDES][NCCOLS][4] =
{
    1.0f, 0.0f, 0.0f, 1.0f,
    0.0f, 1.0f, 0.0f, 1.0f,
    0.0f, 1.0f, 0.0f, 1.0f,
    0.0f, 0.0f, 1.0f, 1.0f,
    0.0f, 0.0f, 1.0f, 1.0f,
    1.0f, 0.0f, 1.0f, 1.0f,
    1.0f, 0.0f, 1.0f, 1.0f,
    0.0f, 1.0f, 1.0f, 1.0f,
    0.0f, 1.0f, 1.0f, 1.0f,
    1.0f, 1.0f, 0.0f, 1.0f,
    1.0f, 1.0f, 0.0f, 1.0f,
    0.5f, 0.5f, 1.0f, 1.0f,
    0.5f, 0.5f, 1.0f, 1.0f,
    1.0f, 0.5f, 0.5f, 1.0f,
    1.0f, 0.5f, 0.5f, 1.0f,
    1.0f, 0.0f, 0.0f, 1.0f
};
GLfloat checker_cols[MAXSIDES][NCCOLS][4];

GLfloat base_side_cols[MAXSIDES][4] =
{
    1.0f, 0.0f, 0.0f, 1.0f,
    0.0f, 1.0f, 0.0f, 1.0f,
    0.0f, 0.0f, 1.0f, 1.0f,
    1.0f, 0.0f, 1.0f, 1.0f,
    0.0f, 1.0f, 1.0f, 1.0f,
    1.0f, 1.0f, 0.0f, 1.0f,
    0.5f, 0.5f, 1.0f, 1.0f,
    1.0f, 0.5f, 0.5f, 1.0f
};
GLfloat side_cols[MAXSIDES][4];

GLfloat base_solid_cols[4] =
{
    1.0f, 1.0f, 1.0f, 1.0f
};
GLfloat solid_cols[4];

// Current geometry
GEOMETRY *cur_geom;

// Set when a rendering context is available
BOOL gbGlInit = FALSE;

// Common library context
SSContext gssc;

// Spin rotations
double xr = 0, yr = 0, zr = 0;
// Scale factor and increment
FLT sf;
FLT sfi;
// Color cycling hue phase
FLT phase = 0.0f;

// Default configuration
CONFIG config =
{
    TRUE, FALSE, FALSE, TRUE, TRUE, MAXSUBDIV, ID_COL_PER_SIDE,
    (MAXIMAGESIZE+MINIMAGESIZE)/2, GEOM_CUBE, GL_FRONT
};

// A slider range
typedef struct _RANGE
{
    int min_val;
    int max_val;
    int step;
    int page_step;
} RANGE;

RANGE complexity_range = {MINSUBDIV, MAXSUBDIV, 1, 2};
RANGE image_size_range = {MINIMAGESIZE, MAXIMAGESIZE, 1, 10};

// True if the current OpenGL version is 1.1
BOOL bOgl11;

// True if checkered mode is on
BOOL bCheckerOn;

/******************************Public*Routine******************************\
*
* dprintf
*
* Debug output printf
*
* History:
*  Wed Jul 26 15:16:11 1995	-by-	Drew Bliss [drewb]
*   Created
*
\**************************************************************************/

#if DBG
void dprintf_out(char *fmt, ...)
{
    va_list args;
    char dbg[256];

    va_start(args, fmt);
    vsprintf(dbg, fmt, args);
    va_end(args);
    OutputDebugStringA(dbg);
}
#endif

/******************************Public*Routine******************************\
*
* assert_failed
*
* Assertion failure handler
*
* History:
*  Fri Jul 28 17:40:28 1995	-by-	Drew Bliss [drewb]
*   Created
*
\**************************************************************************/

#if DBG
void assert_failed(char *file, int line, char *msg)
{
    dprintf(("Assertion failed %s(%d): %s\n", file, line, msg));
    DebugBreak();
}
#endif

/******************************Public*Routine******************************\
*
* V3Len
*
* Vector length
*
* History:
*  Wed Jul 19 14:52:21 1995	-by-	Drew Bliss [drewb]
*   Created
*
\**************************************************************************/

FLT V3Len(PT3 *v)
{
    return (FLT)sqrt(v->x*v->x+v->y*v->y+v->z*v->z);
}

/******************************Public*Routine******************************\
*
* ComputeHsvColors
*
* Compute a smooth range of colors depending on the current color mode
*
* History:
*  Wed Jul 19 14:53:32 1995	-by-	Drew Bliss [drewb]
*   Created
*
\**************************************************************************/

void ComputeHsvColors(void)
{
    GLfloat *cols;
    int ncols;
    FLT ang, da;
    int hex;
    FLT fhex, frac;
    FLT p, q, t;
    FLT sat, val;

    switch(config.color_pick)
    {
    case ID_COL_CHECKER:
        ncols = MAXSIDES*NCCOLS;
        cols = &checker_cols[0][0][0];
        break;
    case ID_COL_PER_SIDE:
        ncols = MAXSIDES;
        cols = &side_cols[0][0];
        break;
    case ID_COL_SINGLE:
        ncols = 1;
        cols = &solid_cols[0];
        break;
    }

    ang = phase;
    da = (FLT)((2*PI)/ncols);
    val = sat = 1.0f;

    while (ncols > 0)
    {
        fhex = (FLT)(6*ang/(2*PI));
        hex = (int)fhex;
        frac = fhex-hex;
        hex = hex % 6;
        
	p = val*(1-sat);
	q = val*(1-sat*frac);
	t = val*(1-sat*(1-frac));
        
	switch(hex)
	{
	case 0:
            cols[0] = val;
            cols[1] = t;
            cols[2] = p;
	    break;
	case 1:
            cols[0] = q;
            cols[1] = val;
            cols[2] = p;
	    break;
	case 2:
            cols[0] = p;
            cols[1] = val;
            cols[2] = t;
	    break;
	case 3:
            cols[0] = p;
            cols[1] = q;
            cols[2] = val;
	    break;
	case 4:
            cols[0] = t;
            cols[1] = p;
            cols[2] = val;
            break;
	case 5:
            cols[0] = val;
            cols[1] = p;
            cols[2] = q;
	    break;
	}

        ang += da;
        cols += 4;
        ncols--;
    }
}

/******************************Public*Routine******************************\
*
* Draw
*
* Draw everything
*
* History:
*  Wed Jul 19 14:54:16 1995	-by-	Drew Bliss [drewb]
*   Created
*
\**************************************************************************/

void Draw(void)
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glLoadIdentity();

    glRotated(xr, 1, 0, 0);
    glRotated(yr, 0, 1, 0);
    glRotated(zr, 0, 0, 1);

    DrawGeom(cur_geom);

    glFlush();
}

/******************************Public*Routine******************************\
*
* Update
*
* Update all varying values, called by the common library
*
* History:
*  Wed Jul 19 14:54:24 1995	-by-	Drew Bliss [drewb]
*   Created
*
\**************************************************************************/

void Update(void *data)
{
    if (config.spin)
    {
        xr += 3;
        yr += 2;
    }

    if (config.bloom)
    {
        sf += sfi;
        if (sf > cur_geom->max_sf ||
            sf < cur_geom->min_sf)
        {
            sfi = -sfi;
        }
        UpdatePts(cur_geom, sf);
    }

    if (config.cycle_colors)
    {
        ComputeHsvColors();
        phase += (FLT)(2.5*PI/180.);
    }
    
    Draw();
}

// String storage
TCHAR geom_names[IDS_GEOM_COUNT][20];

/******************************Public*Routine******************************\
* getIniSettings
*
* Get the screen saver configuration options from .INI file/registry.
*
\**************************************************************************/

static void 
getIniSettings()
{
    // Get registry settings

    if( ! ss_RegistrySetup( GetModuleHandle(NULL), IDS_INI_SECTION, 
                            IDS_INIFILE ) )
        return;
    
    config.smooth_colors =
        ss_GetRegistryInt( IDS_CONFIG_SMOOTH_COLORS, config.smooth_colors );
    config.triangle_colors =
        ss_GetRegistryInt( IDS_CONFIG_TRIANGLE_COLORS, config.triangle_colors );
    config.cycle_colors =
        ss_GetRegistryInt( IDS_CONFIG_CYCLE_COLORS, config.cycle_colors );
    config.spin =
        ss_GetRegistryInt( IDS_CONFIG_SPIN, config.spin );
    config.bloom =
        ss_GetRegistryInt( IDS_CONFIG_BLOOM, config.bloom );
    config.subdiv =
        ss_GetRegistryInt( IDS_CONFIG_SUBDIV, config.subdiv );
    config.color_pick =
        ss_GetRegistryInt( IDS_CONFIG_COLOR_PICK, config.color_pick );
    config.image_size =
        ss_GetRegistryInt( IDS_CONFIG_IMAGE_SIZE, config.image_size );
    config.geom =
        ss_GetRegistryInt( IDS_CONFIG_GEOM, config.geom );
    config.two_sided =
        ss_GetRegistryInt( IDS_CONFIG_TWO_SIDED, config.two_sided );
}

/******************************Public*Routine******************************\
* saveIniSettings
*
* Save the screen saver configuration option to the .INI file/registry.
*
\**************************************************************************/

static void 
saveIniSettings()
{
    if( ! ss_RegistrySetup( GetModuleHandle(NULL), IDS_INI_SECTION, 
                            IDS_INIFILE ) )
        return;

    ss_WriteRegistryInt( IDS_CONFIG_SMOOTH_COLORS, config.smooth_colors );

    ss_WriteRegistryInt( IDS_CONFIG_TRIANGLE_COLORS, config.triangle_colors );
    ss_WriteRegistryInt( IDS_CONFIG_CYCLE_COLORS, config.cycle_colors );
    ss_WriteRegistryInt( IDS_CONFIG_SPIN, config.spin );
    ss_WriteRegistryInt( IDS_CONFIG_BLOOM, config.bloom );
    ss_WriteRegistryInt( IDS_CONFIG_SUBDIV, config.subdiv );
    ss_WriteRegistryInt( IDS_CONFIG_COLOR_PICK, config.color_pick );
    ss_WriteRegistryInt( IDS_CONFIG_IMAGE_SIZE, config.image_size );
    ss_WriteRegistryInt( IDS_CONFIG_GEOM, config.geom );
    ss_WriteRegistryInt( IDS_CONFIG_TWO_SIDED, config.two_sided );
}

/******************************Public*Routine******************************\
*
* NewConfig
*
* Set up a new configuration
*
* History:
*  Wed Jul 19 14:55:34 1995	-by-	Drew Bliss [drewb]
*   Created
*
\**************************************************************************/

void NewConfig(CONFIG *cnf)
{
    // Set new config
    config = *cnf;

    // Save to ini file
    saveIniSettings();
    
    // Reset colors
    memcpy(checker_cols, base_checker_cols, sizeof(checker_cols));
    memcpy(side_cols, base_side_cols, sizeof(side_cols));
    memcpy(solid_cols, base_solid_cols, sizeof(solid_cols));

    // Reset geometry
    cur_geom = geom_table[config.geom];
    cur_geom->init(cur_geom);
    if (bOgl11 && !bCheckerOn) DrawWithVArrays (cur_geom);
    
    assert(cur_geom->total_pts <= MAXPTS);
           
    InitVlen(cur_geom, cur_geom->total_pts, cur_geom->pts);
    sf = 0.0f;
    sfi = cur_geom->sf_inc;
    UpdatePts(cur_geom, sf);

    // Reset OpenGL parameters according to configuration
    // Only done if GL has been initialized
    if (gbGlInit)
    {
        GLfloat fv4[4];
        
        if (config.two_sided == GL_FRONT_AND_BACK)
        {
            glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);
            glDisable(GL_CULL_FACE);
        }
        else
        {
            glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_FALSE);
            glEnable(GL_CULL_FACE);
        }
        
        fv4[0] = fv4[1] = fv4[2] = .8f;
        fv4[3] = 1.0f;
        glMaterialfv(config.two_sided, GL_SPECULAR, fv4);
        glMaterialf(config.two_sided, GL_SHININESS, 30.0f);
    }
}

/******************************Public*Routine******************************\
*
* RegisterDialogClasses
*
* Standard screensaver hook
*
* History:
*  Wed Jul 19 15:18:14 1995	-by-	Drew Bliss [drewb]
*   Created
*
\**************************************************************************/

BOOL WINAPI RegisterDialogClasses(HANDLE hinst)
{
    return TRUE;
}

// Temporary configuration for when the configuration dialog is active
// If the dialog is ok'ed then this becomes the current configuration,
// otherwise it is discarded
CONFIG temp_config;

/******************************Public*Routine******************************\
*
* ScreenSaverConfigureDialog
*
* Standard screensaver hook
*
* History:
*  Wed Jul 19 14:56:41 1995	-by-	Drew Bliss [drewb]
*   Created
*
\**************************************************************************/

BOOL CALLBACK ScreenSaverConfigureDialog(HWND hdlg, UINT msg,
                                         WPARAM wpm, LPARAM lpm)
{
    WORD pos;
    RANGE *rng;
    HWND hCtrl;
    int i;
    
    switch(msg)
    {
    case WM_INITDIALOG:

        InitCommonControls();

        getIniSettings();
    
        temp_config = config;
        
        CheckRadioButton(hdlg, ID_COL_PICK_FIRST, ID_COL_PICK_LAST,
                         config.color_pick);
        CheckDlgButton(hdlg, ID_COL_SMOOTH, config.smooth_colors);
        CheckDlgButton(hdlg, ID_COL_TRIANGLE, config.triangle_colors);
        CheckDlgButton(hdlg, ID_COL_CYCLE, config.cycle_colors);
        CheckDlgButton(hdlg, ID_SPIN, config.spin);
        CheckDlgButton(hdlg, ID_BLOOM, config.bloom);
        CheckDlgButton(hdlg, ID_TWO_SIDED,
                       config.two_sided == GL_FRONT_AND_BACK);
        
        ss_SetupTrackbar( hdlg, ID_COMPLEXITY, MINSUBDIV, MAXSUBDIV, 
                          complexity_range.step,
                          complexity_range.page_step,
                          config.subdiv);

        ss_SetupTrackbar( hdlg, ID_IMAGE_SIZE, MINIMAGESIZE, MAXIMAGESIZE, 
                          image_size_range.step,
                          image_size_range.page_step,
                          config.image_size);

        hCtrl = GetDlgItem(hdlg, ID_GEOM);
        SendMessage(hCtrl, CB_RESETCONTENT, 0, 0);
        for (i = 0; i < IDS_GEOM_COUNT; i++)
        {
            LoadString( hMainInstance, i+IDS_GEOM_FIRST, geom_names[i],
                        sizeof(geom_names)/IDS_GEOM_COUNT );
            SendMessage(hCtrl, CB_ADDSTRING, 0, (LPARAM)geom_names[i]);
        }
        SendMessage(hCtrl, CB_SETCURSEL, config.geom, 0);
        
        SetFocus(GetDlgItem(hdlg, ID_COMPLEXITY));
        return FALSE;

    case WM_COMMAND:
        switch(LOWORD(wpm))
        {
        case ID_COL_CHECKER:
        case ID_COL_PER_SIDE:
        case ID_COL_SINGLE:
            temp_config.color_pick = LOWORD(wpm);
            break;

        case ID_COL_SMOOTH:
            temp_config.smooth_colors = !temp_config.smooth_colors;
            break;
        case ID_COL_TRIANGLE:
            temp_config.triangle_colors = !temp_config.triangle_colors;
            break;
        case ID_COL_CYCLE:
            temp_config.cycle_colors = !temp_config.cycle_colors;
            break;
            
        case ID_SPIN:
            temp_config.spin = !temp_config.spin;
            break;
        case ID_BLOOM:
            temp_config.bloom = !temp_config.bloom;
            break;
        case ID_TWO_SIDED:
            temp_config.two_sided =
                temp_config.two_sided == GL_FRONT_AND_BACK ? GL_FRONT :
                GL_FRONT_AND_BACK;
            break;

        case IDOK:
            temp_config.subdiv =
                ss_GetTrackbarPos(hdlg, ID_COMPLEXITY);
            temp_config.image_size =
                ss_GetTrackbarPos(hdlg, ID_IMAGE_SIZE);
            temp_config.geom =
                (int)SendMessage(GetDlgItem(hdlg, ID_GEOM), CB_GETCURSEL, 0, 0);
            NewConfig(&temp_config);
            // Fall through
        case IDCANCEL:
            EndDialog(hdlg, LOWORD(wpm));
            break;
        }
        return TRUE;
        
    }

    return FALSE;
}

/******************************Public*Routine******************************\
*
* Init
*
* Drawing initialization, called by common library
*
* History:
*  Wed Jul 19 14:47:13 1995	-by-	Drew Bliss [drewb]
*   Created
*
\**************************************************************************/

void Init(void *data)
{
    GLfloat fv4[4];

    gbGlInit = TRUE;
    
    bOgl11 = ss_fOnGL11();

    if (config.color_pick == ID_COL_CHECKER) bCheckerOn = TRUE;
    else bCheckerOn = FALSE;

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45, 1, 2, 5); // for object range -1.5 to 1.5
    gluLookAt(0, 0, 3.5, 0, 0, 0, 0, 1, 0);
    glMatrixMode(GL_MODELVIEW);

    glEnable(GL_DEPTH_TEST);
    glClearDepth(1);

    glCullFace(GL_BACK);
    
    fv4[0] = 2.0f;
    fv4[1] = 2.0f;
    fv4[2] = 10.0f;
    fv4[3] = 1.0f;
    glLightfv(GL_LIGHT0, GL_POSITION, fv4);
    
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);

    glEnable(GL_NORMALIZE);
    
    // Make default configuration current
    NewConfig(&config);
}

/******************************Public*Routine******************************\
* SetFloaterInfo
*
* Set the size and motion of the floating window
*
* History
*  Apr. 28, 95 : [marcfo]
*    - Wrote it
*
\**************************************************************************/

void
SetFloaterInfo( ISIZE *pParentSize, CHILD_INFO *pChild )
{
    float sizeFact;
    float sizeScale;
    int size;
    ISIZE *pChildSize = &pChild->size;
    MOTION_INFO *pMotion = &pChild->motionInfo;

    sizeScale = (float)config.image_size / 100.0f;
    sizeFact = 0.25f + (0.5f * sizeScale);     // range 25-75%
    size = (int) (sizeFact * ( ((float)(pParentSize->width + pParentSize->height)) / 2.0f ));
    SS_CLAMP_TO_RANGE2( size, 0, pParentSize->width );
    SS_CLAMP_TO_RANGE2( size, 0, pParentSize->height );

    pChildSize->width = pChildSize->height = size;

    // Floater motion
    pMotion->posInc.x = .005f * (float) size;
    if( pMotion->posInc.x < 1.0f )
        pMotion->posInc.x = 1.0f;
    pMotion->posInc.y = pMotion->posInc.x;
    pMotion->posIncVary.x = .4f * pMotion->posInc.x;
    pMotion->posIncVary.y = pMotion->posIncVary.x;
}

/******************************Public*Routine******************************\
*
* FloaterFail
*
* Called when the floating window cannot be created
*
* History:
*  Wed Jul 19 15:06:18 1995	-by-	Drew Bliss [drewb]
*   Taken from text3d
*
\**************************************************************************/

void FloaterFail(void *data)
{
    HINSTANCE hinst;
    TCHAR error_str[20];
    TCHAR start_failed[80];

    hinst = GetModuleHandle(NULL);
    if (LoadString(hinst, IDS_ERROR,
                   error_str, sizeof(error_str)) &&
        LoadString(hinst, IDS_START_FAILED,
                   start_failed, sizeof(start_failed)))
    {
        MessageBox(NULL, start_failed, error_str, MB_OK);
    }
}

/******************************Public*Routine******************************\
*
* ss_Init
*
* Screensaver initialization routine, called at startup by common library
*
* History:
*  Wed Jul 19 14:44:46 1995	-by-	Drew Bliss [drewb]
*   Created
*
\**************************************************************************/

SSContext *ss_Init(void)
{
    getIniSettings();
    
    ss_InitFunc(Init);
    ss_UpdateFunc(Update);

    gssc.bFloater = TRUE;
    gssc.floaterInfo.bMotion = TRUE;
    gssc.floaterInfo.ChildSizeFunc = SetFloaterInfo;

    gssc.bDoubleBuf = TRUE;
    gssc.depthType = SS_DEPTH16;

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
