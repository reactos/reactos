/*
 * Support for system colors
 *
 * Copyright  David W. Metcalfe, 1993
 * Copyright  Alexandre Julliard, 1994
 *
 */


#include <windows.h>
//#include <user32/syscolor.h>

void SYSCOLOR_SetColor( int index, COLORREF color );

//#define NUM_SYS_COLORS     (COLOR_GRADIENTINACTIVECAPTION+1)
#define NUM_SYS_COLORS     100

static COLORREF SysColors[NUM_SYS_COLORS];
static HBRUSH SysColorBrushes[NUM_SYS_COLORS];
static HPEN   SysColorPens[NUM_SYS_COLORS];

DWORD STDCALL GetSysColor( INT nIndex )
{
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
    if (0 <= index && index < NUM_SYS_COLORS)
        return SysColorBrushes[index];

    return GetStockObject(LTGRAY_BRUSH);
}


/////////////////////////////////////////////////////////////////////////////////////

void SYSCOLOR_SetColor( int index, COLORREF color )
{
    if (index < 0 || index >= NUM_SYS_COLORS) return;
    SysColors[index] = color;
    if (SysColorBrushes[index]) DeleteObject( SysColorBrushes[index] );
    SysColorBrushes[index] = CreateSolidBrush( color );
    if (SysColorPens[index]) DeleteObject( SysColorPens[index] ); 
    SysColorPens[index] = CreatePen( PS_SOLID, 1, color );
}



/***********************************************************************
 *           GetSysColorPen    (Not a Windows API)
 *
 * This function is new to the Wine lib -- it does not exist in 
 * Windows. However, it is a natural complement for GetSysColorBrush
 * in the Win API and is needed quite a bit inside Wine.
 */
HPEN STDCALL GetSysColorPen( INT index )
{
    /* We can assert here, because this function is internal to Wine */
    //assert (0 <= index && index < NUM_SYS_COLORS);
    return SysColorPens[index];

}

