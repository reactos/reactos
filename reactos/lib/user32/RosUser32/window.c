/* $Id: window.c,v 1.2 2003/07/01 01:03:49 rcampbell Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS user32.dll
 * PROGRAMMER:      Richard Campbell
 * UPDATE HISTORY:
 *      06-30-2003  CSH  Created
 */

//WindowFromDC
//WindowFromPoint

HWND STDCALL
WindowFromDC( HDC hDC )
{
	UNIMPLEMENTED;
}

HWND STDCALL
WindowFromPoint( POINT Point )
{
	UNIMPLEMENTED;
}

BOOL STDCALL 
UpdateWindow( HWND hWnd )
{
	UNIMPLEMENTED;
}

BOOL STDCALL
UnhookWindowsHookEx( HHOOK hhk )
{
	UNIMPLEMENTED;
}

WORD STDCALL
TileWindows( HWND hwndParent,
			 UINT wHow,
			 RECT *lpRect,
			 UINT cKids,
			 const HWND *lpKids )
{
	UNIMPLEMENTED;
}
