/*
 * Support for system colors
 *
 * Copyright  David W. Metcalfe, 1993
 * Copyright  Alexandre Julliard, 1994
 *
 */


#include <windows.h>
#include <user32/syscolor.h>
#include <user32/nc.h>



static const char * const DefSysColors[] =
{
    "Scrollbar", "224 224 224",      /* COLOR_SCROLLBAR           */
    "Background", "192 192 192",     /* COLOR_BACKGROUND          */
    "ActiveTitle", "0 64 128",       /* COLOR_ACTIVECAPTION       */
    "InactiveTitle", "255 255 255",  /* COLOR_INACTIVECAPTION     */
    "Menu", "255 255 255",           /* COLOR_MENU                */
    "Window", "255 255 255",         /* COLOR_WINDOW              */
    "WindowFrame", "0 0 0",          /* COLOR_WINDOWFRAME         */
    "MenuText", "0 0 0",             /* COLOR_MENUTEXT            */
    "WindowText", "0 0 0",           /* COLOR_WINDOWTEXT          */
    "TitleText", "255 255 255",      /* COLOR_CAPTIONTEXT         */
    "ActiveBorder", "128 128 128",   /* COLOR_ACTIVEBORDER        */
    "InactiveBorder", "255 255 255", /* COLOR_INACTIVEBORDER      */
    "AppWorkspace", "255 255 232",   /* COLOR_APPWORKSPACE        */
    "Hilight", "224 224 224",        /* COLOR_HIGHLIGHT           */
    "HilightText", "0 0 0",          /* COLOR_HIGHLIGHTTEXT       */
    "ButtonFace", "192 192 192",     /* COLOR_BTNFACE             */
    "ButtonShadow", "128 128 128",   /* COLOR_BTNSHADOW           */
    "GrayText", "192 192 192",       /* COLOR_GRAYTEXT            */
    "ButtonText", "0 0 0",           /* COLOR_BTNTEXT             */
    "InactiveTitleText", "0 0 0",    /* COLOR_INACTIVECAPTIONTEXT */
    "ButtonHilight", "255 255 255",  /* COLOR_BTNHIGHLIGHT        */
    "3DDarkShadow", "32 32 32",      /* COLOR_3DDKSHADOW          */
    "3DLight", "192 192 192",        /* COLOR_3DLIGHT             */
    "InfoText", "0 0 0",             /* COLOR_INFOTEXT            */
    "InfoBackground", "255 255 192", /* COLOR_INFOBK              */
    "AlternateButtonFace", "184 180 184",  /* COLOR_ALTERNATEBTNFACE */
    "HotTrackingColor", "0 0 255",         /* COLOR_HOTLIGHT */
    "GradientActiveTitle", "16 132 208",   /* COLOR_GRADIENTACTIVECAPTION */
    "GradientInactiveTitle", "184 180 184" /* COLOR_GRADIENTINACTIVECAPTION */
};

static const char * const DefSysColors95[] =
{
    "Scrollbar", "223 223 223",      /* COLOR_SCROLLBAR           */
    "Background", "192 192 192",     /* COLOR_BACKGROUND          */
    "ActiveTitle", "0 0 128",        /* COLOR_ACTIVECAPTION       */
    "InactiveTitle", "128 128 128",  /* COLOR_INACTIVECAPTION     */
    "Menu", "192 192 192",           /* COLOR_MENU                */
    "Window", "255 255 255",         /* COLOR_WINDOW              */
    "WindowFrame", "0 0 0",          /* COLOR_WINDOWFRAME         */
    "MenuText", "0 0 0",             /* COLOR_MENUTEXT            */
    "WindowText", "0 0 0",           /* COLOR_WINDOWTEXT          */
    "TitleText", "255 255 255",      /* COLOR_CAPTIONTEXT         */
    "ActiveBorder", "192 192 192",   /* COLOR_ACTIVEBORDER        */
    "InactiveBorder", "192 192 192", /* COLOR_INACTIVEBORDER      */
    "AppWorkspace", "128 128 128",   /* COLOR_APPWORKSPACE        */
    "Hilight", "0 0 128",            /* COLOR_HIGHLIGHT           */
    "HilightText", "255 255 255",    /* COLOR_HIGHLIGHTTEXT       */
    "ButtonFace", "192 192 192",     /* COLOR_BTNFACE             */
    "ButtonShadow", "128 128 128",   /* COLOR_BTNSHADOW           */
    "GrayText", "192 192 192",       /* COLOR_GRAYTEXT            */
    "ButtonText", "0 0 0",           /* COLOR_BTNTEXT             */
    "InactiveTitleText", "0 0 0",    /* COLOR_INACTIVECAPTIONTEXT */
    "ButtonHilight", "255 255 255",  /* COLOR_BTNHIGHLIGHT        */
    "3DDarkShadow", "0 0 0",         /* COLOR_3DDKSHADOW          */
    "3DLight", "223 223 223",        /* COLOR_3DLIGHT             */
    "InfoText", "0 0 0",             /* COLOR_INFOTEXT            */
    "InfoBackground", "255 255 192", /* COLOR_INFOBK              */
    "AlternateButtonFace", "184 180 184",  /* COLOR_ALTERNATEBTNFACE */
    "HotTrackingColor", "0 0 255",         /* COLOR_HOTLIGHT */
    "GradientActiveTitle", "16 132 208",   /* COLOR_GRADIENTACTIVECAPTION */
    "GradientInactiveTitle", "184 180 184" /* COLOR_GRADIENTINACTIVECAPTION */
};

//#define NUM_SYS_COLORS     (COLOR_GRADIENTINACTIVECAPTION+1)
#define NUM_SYS_COLORS     29

static COLORREF SysColors[NUM_SYS_COLORS];
static HBRUSH SysColorBrushes[NUM_SYS_COLORS];
static HPEN   SysColorPens[NUM_SYS_COLORS];

static char bSysColorInit = FALSE;

DWORD STDCALL GetSysColor( INT nIndex )
{
    if ( bSysColorInit == FALSE )
	SYSCOLOR_Init();
    if (nIndex >= 0 && nIndex < NUM_SYS_COLORS)
	return SysColors[nIndex];
    else
	return 0;
}



/*************************************************************************
 *             SetSysColors   (USER.505)
 */
WINBOOL STDCALL SetSysColors( INT nChanges, const INT *lpSysColor,
                              const COLORREF *lpColorValues )
{
    int i;

    if ( bSysColorInit == FALSE )
	SYSCOLOR_Init();
    for (i = 0; i < nChanges; i++)
    {
	SYSCOLOR_SetColor( lpSysColor[i], lpColorValues[i] );
    }

    /* Send WM_SYSCOLORCHANGE message to all windows */

    SendMessageA( HWND_BROADCAST, WM_SYSCOLORCHANGE, 0, 0 );

    /* Repaint affected portions of all visible windows */

    RedrawWindow( GetDesktopWindow(), NULL, 0,
                RDW_INVALIDATE | RDW_ERASE | RDW_UPDATENOW | RDW_ALLCHILDREN );
    return TRUE;
}


HBRUSH STDCALL GetSysColorBrush( INT index )
{
    if ( bSysColorInit == FALSE )
	SYSCOLOR_Init();
    if (0 <= index && index < NUM_SYS_COLORS)
        return SysColorBrushes[index];

    return GetStockObject(LTGRAY_BRUSH);
}


/////////////////////////////////////////////////////////////////////////////////////

/***********************************************************************
 *           GetSysColorPen    (Not a Windows API)
 *
 * This function is new to the Wine lib -- it does not exist in 
 * Windows. However, it is a natural complement for GetSysColorBrush
 * in the Win API and is needed quite a bit inside Wine.
 */
HPEN  GetSysColorPen( INT index )
{
    /* We can assert here, because this function is internal to Wine */
    //assert (0 <= index && index < NUM_SYS_COLORS);
    if ( bSysColorInit == FALSE )
	SYSCOLOR_Init();
    return SysColorPens[index];

}


/*************************************************************************
 *             SYSCOLOR_Init
 */

void SYSCOLOR_Init(void)
{
    int i, r, g, b;
    const char * const *p;
    char buffer[100];

    p = (TWEAK_WineLook == WIN31_LOOK) ? DefSysColors : DefSysColors95;
    for (i = 0; i < NUM_SYS_COLORS; i++, p += 2)
    {
//	GetProfileString32A( "colors", p[0], p[1], buffer, 100 );
	if (sscanf( p[1], " %d %d %d", &r, &g, &b ) != 3) r = g = b = 0;
	SYSCOLOR_SetColor( i, RGB(r,g,b) );
    }
    bSysColorInit = TRUE;
}

void SYSCOLOR_SetColor( int index, COLORREF color )
{
    if (index < 0 || index >= NUM_SYS_COLORS) return;
    SysColors[index] = color;
    if (SysColorBrushes[index]) DeleteObject( SysColorBrushes[index] );
    SysColorBrushes[index] = CreateSolidBrush( color );
    if (SysColorPens[index]) DeleteObject( SysColorPens[index] ); 
    SysColorPens[index] = CreatePen( PS_SOLID, 1, color );
}




