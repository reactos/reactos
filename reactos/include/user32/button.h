/*
 * Button-class extra info
 *
 * Copyright 1994 Alexandre Julliard
 */

#ifndef __WINE_BUTTON_H
#define __WINE_BUTTON_H

#include <windows.h>

  /* Extra info for BUTTON windows */
  /* Note: under MS-Windows, state is a BYTE and this structure is */
  /* only 3 bytes long. I don't think there are programs out there */
  /* broken enough to rely on this :-) */
typedef struct
{
    WORD     state;   /* Current state */
    HFONT  hFont;   /* Button font (or 0 for system font) */
} BUTTONINFO;

  /* Button state values */
#define BUTTON_UNCHECKED       0x00
#define BUTTON_CHECKED         0x01
#define BUTTON_3STATE          0x02
#define BUTTON_HIGHLIGHTED     0x04
#define BUTTON_HASFOCUS        0x08

#define BUTTON_STATE(hwnd)     ((WIN_FindWndPtr(hwnd))->wExtra[0])

LRESULT WINAPI ButtonWndProc( HWND hWnd, UINT uMsg,
                                     WPARAM wParam, LPARAM lParam );

#endif  /* __WINE_BUTTON_H */
