

#include <user32/win.h>
#include <user32/queue.h>
#include <user32/debug.h>


#define DCX_DCEEMPTY		0x00000800
#define DCX_DCEBUSY		0x00001000
#define DCX_DCEDIRTY		0x00002000
#define DCX_WINDOWPAINT		0x00020000
#define DCX_KEEPCLIPRGN		0x00040000
#define DCX_NOCLIPCHILDREN      0x00080000




#define DCX_USESTYLE         0x00010000


WINBOOL PAINT_RedrawWindow( HWND hwnd, const RECT *rectUpdate,
                           HRGN hrgnUpdate, UINT flags, UINT control );

HBRUSH PAINT_GetControlBrush( HWND hParent, HWND hWnd, HDC hDC, UINT ctlType );

HBRUSH GetControlBrush( HWND hwnd, HDC hdc, UINT ctlType );

void  PaintRect( HWND hwndParent, HWND hwnd, HDC hdc,
                       HBRUSH hbrush, const RECT *rect);

void  FillWindow( HWND hwndParent, HWND hwnd, HDC hdc, HBRUSH hbrush );