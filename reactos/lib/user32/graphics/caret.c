/*
 * Caret functions
 *
 * Copyright 1993 David Metcalfe
 * Copyright 1996 Frans van Dorsselaer
 */

#include <windows.h>
#include <user32/caret.h>
#include <user32/debug.h>

static CARET Caret = { 0, 0, FALSE, 0, 0, 2, 12, 0, 500, 0 };


WINBOOL STDCALL CreateCaret( HWND hwnd, HBITMAP bitmap,
                             INT width, INT height )
{
    DPRINT("hwnd=%04x\n",(UINT) hwnd);

    if (!hwnd) return FALSE;

    /* if cursor already exists, destroy it */
    if (Caret.hwnd) DestroyCaret();

    if (bitmap && ((UINT)bitmap != 1))
    {
        BITMAP bmp;
        if (!GetObject( bitmap, sizeof(bmp), &bmp )) return FALSE;
        Caret.width = bmp.bmWidth;
        Caret.height = bmp.bmHeight;
        /* FIXME: we should make a copy of the bitmap instead of a brush */
        Caret.hBrush = CreatePatternBrush( bitmap );
    }
    else
    {
        Caret.width = width ? width : GetSystemMetrics(SM_CXBORDER);
        Caret.height = height ? height : GetSystemMetrics(SM_CYBORDER);
        Caret.hBrush = CreateSolidBrush(bitmap ?
                                          GetSysColor(COLOR_GRAYTEXT) :
                                          GetSysColor(COLOR_WINDOW) );
    }

    Caret.hwnd = hwnd;
    Caret.hidden = 1;
    Caret.on = FALSE;
    Caret.x = 0;
    Caret.y = 0;

    Caret.timeout = GetProfileIntA( "windows", "CursorBlinkRate", 500 );
    return TRUE;
}
   


WINBOOL STDCALL DestroyCaret(void)
{
    if (!Caret.hwnd) return FALSE;

    DPRINT("hwnd=%04x, timerid=%d\n", (UINT)Caret.hwnd,(UINT) Caret.timerid);

    CARET_KillTimer();
    CARET_DisplayCaret(CARET_OFF);
    DeleteObject( Caret.hBrush );
    Caret.hwnd = 0;
    return TRUE;
}



WINBOOL STDCALL SetCaretPos( INT x, INT y)
{
    if (!Caret.hwnd) return FALSE;
    if ((x == Caret.x) && (y == Caret.y)) return TRUE;

    DPRINT("x=%d, y=%d\n", x, y);

    CARET_KillTimer();
    CARET_DisplayCaret(CARET_OFF);
    Caret.x = x;
    Caret.y = y;
    if (!Caret.hidden)
    {
	CARET_DisplayCaret(CARET_ON);
	CARET_SetTimer();
    }
    return TRUE;
}





/*****************************************************************
 *           HideCaret   (USER.317)
 */
WINBOOL STDCALL HideCaret( HWND hwnd )
{
    if (!Caret.hwnd) return FALSE;
    if (hwnd && (Caret.hwnd != hwnd)) return FALSE;

    DPRINT("hwnd=%04x, hidden=%d\n",
                  hwnd, Caret.hidden);

    CARET_KillTimer();
    CARET_DisplayCaret(CARET_OFF);
    Caret.hidden++;
    return TRUE;
}




/*****************************************************************
 *           ShowCaret   (USER.529)
 */
WINBOOL STDCALL ShowCaret( HWND hwnd )
{
    if (!Caret.hwnd) return FALSE;
    if (hwnd && (Caret.hwnd != hwnd)) return FALSE;

    DPRINT("hwnd=%04x, hidden=%d\n",
		hwnd, Caret.hidden);

    if (Caret.hidden)
    {
	Caret.hidden--;
	if (!Caret.hidden)
	{
	    CARET_DisplayCaret(CARET_ON);
	    CARET_SetTimer();
	}
    }
    return TRUE;
}



/*****************************************************************
 *           SetCaretBlinkTime   (USER.465)
 */
WINBOOL STDCALL SetCaretBlinkTime( UINT msecs )
{
    if (!Caret.hwnd) return FALSE;

    DPRINT("hwnd=%04x, msecs=%d\n",
		Caret.hwnd, msecs);

    Caret.timeout = msecs;
    CARET_ResetTimer();
    return TRUE;
}




/*****************************************************************
 *           GetCaretBlinkTime   (USER.209)
 */
UINT STDCALL GetCaretBlinkTime(void)
{
    return Caret.timeout;
}





/*****************************************************************
 *           GetCaretPos   (USER.210)
 */
WINBOOL STDCALL GetCaretPos( LPPOINT pt )
{
    if (!Caret.hwnd || !pt) return FALSE;
    pt->x = Caret.x;
    pt->y = Caret.y;
    return TRUE;
}


/*****************************************************************
 *              CARET_GetHwnd
 */
HWND CARET_GetHwnd(void)
{
    return Caret.hwnd;
}

/*****************************************************************
 *              CARET_GetRect
 */
void CARET_GetRect(LPRECT lprc)
{
    lprc->right = (lprc->left = Caret.x) + Caret.width - 1;
    lprc->bottom = (lprc->top = Caret.y) + Caret.height - 1;
}

/*****************************************************************
 *               CARET_DisplayCaret
 */
void CARET_DisplayCaret( DISPLAY_CARET status )
{
    HDC hdc;
    HBRUSH hPrevBrush;

    if (Caret.on && (status == CARET_ON)) return;
    if (!Caret.on && (status == CARET_OFF)) return;

    /* So now it's always a toggle */

    Caret.on = !Caret.on;
    /* do not use DCX_CACHE here, for x,y,width,height are in logical units */
    if (!(hdc = GetDCEx( Caret.hwnd, 0, DCX_USESTYLE /*| DCX_CACHE*/ ))) return;
    hPrevBrush = SelectObject( hdc, Caret.hBrush );
    PatBlt( hdc, Caret.x, Caret.y, Caret.width, Caret.height, PATINVERT );
    SelectObject( hdc, hPrevBrush );
    ReleaseDC( Caret.hwnd, hdc );
}

  
/*****************************************************************
 *               CARET_Callback
 */
VOID CALLBACK CARET_Callback( HWND hwnd, UINT msg, UINT id, DWORD ctime)
{
    DPRINT("hwnd=%04x, timerid=%d, caret=%d\n", hwnd, id, Caret.on);
    CARET_DisplayCaret(CARET_TOGGLE);
}


/*****************************************************************
 *               CARET_SetTimer
 */
void CARET_SetTimer(void)
{
    if (Caret.timerid) 
	KillTimer( (HWND)0, Caret.timerid );
    Caret.timerid = SetTimer( (HWND)0, 0, Caret.timeout,CARET_Callback );
}


/*****************************************************************
 *               CARET_ResetTimer
 */
void CARET_ResetTimer(void)
{
    if (Caret.timerid) 
    {
	KillTimer( (HWND)0, Caret.timerid );
	Caret.timerid = SetTimer( (HWND)0, 0, Caret.timeout,
                                          CARET_Callback );
    }
}


/*****************************************************************
 *               CARET_KillTimer
 */
void CARET_KillTimer(void)
{
    if (Caret.timerid) 
    {
	KillTimer( (HWND)0, Caret.timerid );
	Caret.timerid = 0;
    }
}



/**********************************************************************
 *          CreateIconFromResource          (USER32.76)
 */
HICON STDCALL CreateIconFromResource( LPBYTE bits, UINT cbSize,
                                           WINBOOL bIcon, DWORD dwVersion)
{
    return CreateIconFromResourceEx( bits, cbSize, bIcon, dwVersion, 0,0,0);
}


/**********************************************************************
 *          CreateIconFromResourceEx          (USER32.77)
 */
HICON STDCALL CreateIconFromResourceEx( LPBYTE bits, UINT cbSize,
                                           WINBOOL bIcon, DWORD dwVersion,
                                           INT width, INT height,
                                           UINT cFlag )
{
/*
    TDB* pTask = (TDB*)GlobalLock( GetCurrentTask() );
    if( pTask )
	return CURSORICON_CreateFromResource( pTask->hInstance, 0, bits, cbSize, bIcon, dwVersion,
					      width, height, cFlag );
*/
    return 0;
}



