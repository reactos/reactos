/*
 * Support for system colors
 *
 * Copyright  David W. Metcalfe, 1993
 * Copyright  Alexandre Julliard, 1994
 *
 */


#include <windows.h>
#include <user32/syscolor.h>

typedef struct _sysco
{
	const char *name;
	const char *value;
} syscol;

static int DefSysColors[] =
{
    223,223,223,      /* COLOR_SCROLLBAR           */
    192,192,192,     /* COLOR_BACKGROUND          */
    0,0,128,     /* COLOR_ACTIVECAPTION       */
    128,128,128,  /* COLOR_INACTIVECAPTION     */
    192,192,192,           /* COLOR_MENU                */
    255,255,255,         /* COLOR_WINDOW              */
    0,0,0,     /* COLOR_WINDOWFRAME         */
    0,0,0,        /* COLOR_MENUTEXT            */
    0,0,0,      /* COLOR_WINDOWTEXT          */
    255,255,255,      /* COLOR_CAPTIONTEXT         */
    192,192,192,   /* COLOR_ACTIVEBORDER        */
    192,192,192, /* COLOR_INACTIVEBORDER      */
    128,128,128,   /* COLOR_APPWORKSPACE        */
    0,0,128,         /* COLOR_HIGHLIGHT           */
    255,255,255,    /* COLOR_HIGHLIGHTTEXT       */
    192,192,192,     /* COLOR_BTNFACE             */
    128,128,128,   /* COLOR_BTNSHADOW           */
    192,192,192,       /* COLOR_GRAYTEXT            */
    0,0,0,      /* COLOR_BTNTEXT             */
    0,0,0,/* COLOR_INACTIVECAPTIONTEXT */
    255,255,255,  /* COLOR_BTNHIGHLIGHT        */
    0,0,0,    /* COLOR_3DDKSHADOW          */
    223,223,223,        /* COLOR_3DLIGHT             */
    0,0,0,        /* COLOR_INFOTEXT            */
    255,255,192, /* COLOR_INFOBK              */
    184,180,184,  /* COLOR_ALTERNATEBTNFACE */
    0,0,255,      /* COLOR_HOTLIGHT */
    16,132,208,   /* COLOR_GRADIENTACTIVECAPTION */
    184,180,184 /* COLOR_GRADIENTINACTIVECAPTION */
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

    for (i = 0; i < NUM_SYS_COLORS; i++)
    {
 	r = DefSysColors[i*3];
        g = DefSysColors[i*3+ 1 ];
	b = DefSysColors[i*3+ 2 ];
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




