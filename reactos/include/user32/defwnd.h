#ifndef __WINE_DEFWND_H
#define __WINE_DEFWND_H

void DEFWND_SetText( WND *wndPtr, const void *text );
void DEFWND_SetTextA( WND *wndPtr, LPCSTR text );
void DEFWND_SetTextW( WND *wndPtr, LPCWSTR text );

#endif