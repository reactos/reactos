/*
 * COPYRIGHT:    See COPYING in the top level directory
 * PROJECT:      ReactOS kernel
 * FILE:         include/console.h
 * PURPOSE:      Console declarations used by all the parts of the 
 *               system
 * PROGRAMMER:   David Welch <welch@cwcom.net>
 * UPDATE HISTORY: 
 *               27/06/00: Created
 */


#ifndef __INCLUDE_CONSOLE_H
#define __INCLUDE_CONSOLE_H

/* GetConsoleMode */
#define CONSOLE_INPUT_MODE_VALID  (0x0f)
#define CONSOLE_OUTPUT_MODE_VALID (0x03)

typedef struct _CONSOLE_SELECTION_INFO {
  DWORD dwFlags;
  COORD dwSelectionAnchor;
  SMALL_RECT srSelection;
} CONSOLE_SELECTION_INFO, *PCONSOLE_SELECTION_INFO;

#ifndef __USE_W32API

typedef struct _CONSOLE_FONT_INFO {
  DWORD nFont;
  COORD dwFontSize;
} CONSOLE_FONT_INFO, *PCONSOLE_FONT_INFO;

/* GetConsoleMode */
#define ENABLE_LINE_INPUT			(0x02)
#define ENABLE_ECHO_INPUT			(0x04)
#define ENABLE_PROCESSED_INPUT		(0x01)
#define ENABLE_WINDOW_INPUT			(0x08)
#define ENABLE_MOUSE_INPUT			(0x0f)
#define ENABLE_PROCESSED_OUTPUT		(0x01)
#define ENABLE_WRAP_AT_EOL_OUTPUT	(0x02)

typedef struct _CONSOLE_SCREEN_BUFFER_INFO {
  COORD      dwSize;
  COORD      dwCursorPosition;
  WORD       wAttributes;
  SMALL_RECT srWindow;
  COORD      dwMaximumWindowSize;
} CONSOLE_SCREEN_BUFFER_INFO, *PCONSOLE_SCREEN_BUFFER_INFO ;

typedef struct _CONSOLE_CURSOR_INFO {
  DWORD  dwSize;
  BOOL   bVisible;
} CONSOLE_CURSOR_INFO, *PCONSOLE_CURSOR_INFO;

#endif /* !__USE_W32API */

#endif /* __INCLUDE_CONSOLE_H */
