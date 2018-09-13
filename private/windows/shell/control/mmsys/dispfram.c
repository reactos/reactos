/*
 **************************************************************************
 *
 *  dispfram.c
 *
 *  DispFrame Control DLL defines a bitmap display control to be used
 *  by any windows application.
 *
 *  Copyright 1991-3, Microsoft Corporation
 *
 *  History:
 *
 *  In Sik Rhee - 7/15/91 (original slider.dll)
 *  Ben Mejia - 1/22/92 (made display.dll)	 
 **************************************************************************
 */
#pragma warning(disable:4704)
#include <windows.h>
#include <custcntl.h>
#include <commctrl.h>
#include "draw.h"

/*
 **************************************************************************
 * global static variables 
 **************************************************************************
 */
UINT				gwPm_DispFrame;
extern HINSTANCE	ghInstance;


/*
 **************************************************************************
 * prototypes 
 **************************************************************************
 */

LONG PASCAL dfPaint				(HWND hWnd);
LONG PASCAL dfSetBitmap			(HWND hWnd, HBITMAP hBmpNew, HPALETTE 
																hPalNew);
BOOL PASCAL RegSndCntrlClass	(LPCTSTR lpszSndCntrlClass);
LRESULT PASCAL dfDispFrameWndFn	(HWND hWnd, UINT wMessage, WPARAM wParam, 
																LPARAM lParam);
BOOL dfDrawRect					(HDC hdcRect, RECT rFrame);

/*
 **************************************************************************
 * RegSndCntrlClass
 *
 * Description: Registers the SndCntrlClass, must be called in LibMain
 *
 * Arguments:
 *   LPTSTR		lpszSndCntrlClass
 *
 *  Returns:	BOOL
 *      TRUE if RegisterClass succeeds, else FALSE
 *
 **************************************************************************
 */
BOOL PASCAL RegSndCntrlClass(LPCTSTR lpszSndCntrlClass)
{

    extern UINT     gwPm_DispFrame;

    /* local variables */
    WNDCLASS    ClassStruct;

    /* check to see if class already exists;  if so, simply return TRUE */
    if (GetClassInfo(ghInstance, lpszSndCntrlClass, &ClassStruct))
        return TRUE;

	/* define dispfram class attributes */
	ClassStruct.lpszClassName   = (LPTSTR)DISPFRAMCLASS;
	ClassStruct.hCursor         = LoadCursor( NULL, IDC_ARROW );
	ClassStruct.lpszMenuName    = (LPTSTR)NULL;
	ClassStruct.style           = CS_HREDRAW|CS_VREDRAW|CS_GLOBALCLASS;
	ClassStruct.lpfnWndProc     = (WNDPROC)dfDispFrameWndFn;
	ClassStruct.hInstance       = ghInstance;
	ClassStruct.hIcon           = NULL;
	ClassStruct.cbWndExtra      = DF_DISP_EXTRA;
	ClassStruct.cbClsExtra      = 0;
	ClassStruct.hbrBackground   = (HBRUSH)(COLOR_WINDOW + 1 );

	/* register display frame window class */
	if (!RegisterClass(&ClassStruct))
		return FALSE;
	gwPm_DispFrame = RegisterWindowMessage((LPTSTR) DF_WMDISPFRAME);

	if (!gwPm_DispFrame)    /* failed to create message */
		return FALSE;
	return TRUE;
}

/*
 **************************************************************************
 * dfDispFrameWndFn
 * 
 * Description:   Window function for display frame control.
 *
 * Arguments:     
 *	 HWND	hWnd - handle to control window.
 *   UINT	wMessage - the message
 *   WPARAM	wParam
 *   LPARAM	lParam
 * Returns: LONG
 *     result of message processing... depends on message sent.
 * 
 **************************************************************************
 */

LRESULT PASCAL dfDispFrameWndFn(HWND hWnd, UINT wMessage, WPARAM wParam, 
																LPARAM lParam)
{
	HBITMAP		hBmp;

    switch (wMessage)
    {
    case WM_CREATE:
        DF_SET_BMPHANDLE(0);
        DF_SET_BMPPAL(0);
        return 0;

    case WM_DESTROY:
		/* Free up stored bitmap and palette, if any.
		*/
		hBmp = (HBITMAP)DF_GET_BMPHANDLE;
		if (hBmp)
	        DeleteObject(hBmp);
        return 0;

	case WM_SYSCOLORCHANGE:
		InvalidateRect(hWnd, NULL, TRUE);
		return 0;

    case WM_PAINT:
		return dfPaint(hWnd);

    /* Custom Control Messages */
    case DF_PM_SETBITMAP:
		return dfSetBitmap(hWnd, (HBITMAP)wParam, (HPALETTE)lParam);
    }
    return DefWindowProc(hWnd, wMessage, wParam, lParam);
}

/*
 **************************************************************************
 * dfDrawRect
 *
 * Description:  Draws background of control window.
 *
 * Params:     
 *   HWND	hWnd - handle to control window.
 *   RECT	rFrame - bounding rectangle.
 *
 * Returns:    BOOL
 *	Pass/Fail indicator 0 indicates failure.
 **************************************************************************	 
 */
BOOL dfDrawRect(HDC hdcRect, RECT rFrame)
{
    HANDLE      hBrush;
    HANDLE      hOldBrush;
    HANDLE      hPen;
    HANDLE      hOldPen;
    HANDLE      hPen3DHILIGHT;
											    
    /* Get DC's pen and brush for frame redraw
    */
    hBrush = CreateSolidBrush(GetSysColor(COLOR_3DFACE));
    if (!hBrush)
        return FALSE;
	hPen = CreatePen(PS_SOLID, 2, GetSysColor(COLOR_WINDOWFRAME));
	hPen3DHILIGHT = CreatePen(PS_SOLID, 2, GetSysColor(COLOR_3DHILIGHT));

	if (!hPen3DHILIGHT)
	{
		DeleteObject(hBrush);
		return FALSE;
	}
    hOldBrush = SelectObject(hdcRect, hBrush);
    //hOldPen = SelectObject(hdcRect, hPen);
	hOldPen = SelectObject(hdcRect, hPen3DHILIGHT);

	/* paint the window.
	*/
    //Rectangle(hdcRect, rFrame.left, rFrame.top, rFrame.right,
    //    rFrame.bottom);
	MoveToEx(hdcRect, rFrame.left,rFrame.bottom, NULL);
	LineTo(hdcRect, rFrame.right,rFrame.bottom);
	LineTo(hdcRect, rFrame.right,rFrame.top);
	SelectObject(hdcRect, hPen);
	LineTo(hdcRect, rFrame.left,rFrame.top);
	LineTo(hdcRect, rFrame.left,rFrame.bottom);
	SelectObject(hdcRect, hOldPen);
    /*clean up brush and pen */
    //DeleteObject();
    DeleteObject(SelectObject(hdcRect, hOldBrush));
    DeleteObject(hPen3DHILIGHT);
    DeleteObject(hPen);
    return TRUE;
}

/*
 **************************************************************************
 * dfPaint
 *
 * Description:   Paints background and bitmap, if any.
 *
 * Params:     
 *    HWND	hWnd - handle to control window
 *		 
 * Returns: LONG   
 *			0 if OK, -1 otherwise. (for windproc return)
 * 
 **************************************************************************
 */
LONG PASCAL dfPaint(HWND hWnd)
{
	HBITMAP hBmp;
	HBITMAP hPrev;
	RECT rFrame;
	PAINTSTRUCT ps;
	HDC hdcMem;
	BITMAP bmp;
	int x, y, dx, dy;

	/* Setup to do the painting
	*/
 	if(!GetUpdateRect(hWnd,NULL,FALSE))
 		return 0L;
    BeginPaint(hWnd, &ps);
	GetClientRect(hWnd, &rFrame);
	hBmp = (HBITMAP)DF_GET_BMPHANDLE;
	if (hBmp)
	{
		hdcMem = CreateCompatibleDC(ps.hdc);
		if (!hdcMem)
		{
			EndPaint(hWnd, &ps);
			return -1L;
		}
		hPrev = SelectObject(hdcMem, hBmp);
	
		/* Get the size of the bitmap to center it in the frame.
		*/
		GetObject(hBmp, sizeof(BITMAP), (LPTSTR)&bmp);
		if (bmp.bmWidth > (rFrame.right-rFrame.left))
		{
			x = 0;
			dx = rFrame.right-rFrame.left;
		}
		else
		{
			x = ((rFrame.right-rFrame.left - bmp.bmWidth) >> 1);
			dx = bmp.bmWidth;
		}
		if (bmp.bmHeight > (rFrame.bottom-rFrame.top))
		{
			y = 0;
			dy = rFrame.bottom-rFrame.top;
		}
		else
		{
			y = ((rFrame.bottom-rFrame.top - bmp.bmHeight) >> 1);
			dy = bmp.bmHeight;
		}
	
		/* Draw the frame & background, then blt in the bitmap.
		*/
		dfDrawRect(ps.hdc, rFrame);
		BitBlt(ps.hdc, x, y, dx, dy, hdcMem, 0, 0, SRCCOPY);

		/* Cleanup and exit.
		*/
		SelectObject(hdcMem, hPrev);
		DeleteDC(hdcMem);
	}
	else
		/* if no bitmap, just repaint the background.
		*/
		dfDrawRect(ps.hdc, rFrame);
		
	EndPaint(hWnd, &ps);
	return 0L;
}


/*
 **************************************************************************
 * dfSetBitmap
 * 
 * Description:   Load a new bitmap into the control.
 *
 * Arguments:     
 *		HWND		hWnd - handle to control window.
 *		HBITMAP		hBmpNew - handle to new bitmap.
 *		HPALETTE	hPalNew - handle to new bitmap's palette (Optional).
 * Returns:			LONG
 *				0 for success; -1 if fails (for return by wndproc).
 * 
 **************************************************************************
 */
LONG PASCAL dfSetBitmap(HWND hWnd, HBITMAP hBmpNew, HPALETTE hPalNew)
{
	HBITMAP hBmp;
	HANDLE hPrev;
	HANDLE hPrevNew;
	HDC hdcMem;
	HDC hdcNew;
	HDC hDC;
	RECT rFrame;
	int dx, dy;
	BITMAP bmp;

	/* Cleanup any existing bitmap & palette
	*/
	hBmp = (HBITMAP)DF_GET_BMPHANDLE;
	if (hBmp)
		DeleteObject(hBmp);
    DF_SET_BMPHANDLE(0);
    DF_SET_BMPPAL(0);
	InvalidateRect(hWnd, NULL, TRUE);

	/* Copy the displayable portion of the bitmap into a private copy.
	*/
    if (hBmpNew)
    {
		/* get all the req'd DC's etc.
		*/
        hDC = GetDC(hWnd);
        hdcMem = CreateCompatibleDC(hDC);
        if (!hdcMem)
        {
            ReleaseDC(hWnd, hDC);
            return -1L;
        }
        hdcNew = CreateCompatibleDC(hDC);
        if (!hdcNew)
        {
            ReleaseDC(hWnd, hDC);
            return -1L;
        }
        GetObject(hBmpNew, sizeof(BITMAP), (LPTSTR)&bmp);
        hBmp = CreateCompatibleBitmap(hDC, bmp.bmWidth, bmp.bmHeight);
        if (!hBmp)
        {
            DeleteDC(hdcMem);
            DeleteDC(hdcNew);
            ReleaseDC(hWnd, hDC);
            return -1L;
        }
  	    hPrevNew = SelectObject(hdcNew, hBmpNew);
        hPrev = SelectObject(hdcMem, hBmp);

		/* figure out how much of the bitmap we need to copy.
		*/    
		GetClientRect(hWnd, &rFrame);
        if (bmp.bmWidth > (rFrame.right-rFrame.left))
            dx = rFrame.right-rFrame.left;
        else
            dx = bmp.bmWidth;
        if (bmp.bmHeight > (rFrame.bottom-rFrame.top))
            dy = rFrame.bottom-rFrame.top;
        else
            dy = bmp.bmHeight;

		/* copy the bitmap.
		*/
        BitBlt(hdcMem, 0, 0, dx, dy, hdcNew, 0 , 0, SRCCOPY);

		/* cleanup
		*/
		hBmp = SelectObject(hdcMem, hPrev);
        DF_SET_BMPHANDLE(hBmp);
		DeleteDC(hdcMem);
		SelectObject(hdcNew, hPrevNew);
		DeleteDC(hdcNew);
		ReleaseDC(hWnd, hDC);

		/* if a palette is handed in, store it too.
		*/
	    DF_SET_BMPPAL(hPalNew);
   }
   return 0L;
}

