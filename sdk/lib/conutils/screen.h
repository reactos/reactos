/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Console Utilities Library
 * FILE:            sdk/lib/conutils/screen.h
 * PURPOSE:         Console/terminal screen management.
 * PROGRAMMERS:     - Hermes Belusca-Maito (for the library);
 *                  - All programmers who wrote the different console applications
 *                    from which I took those functions and improved them.
 */

#ifndef __SCREEN_H__
#define __SCREEN_H__

#ifndef _UNICODE
#error The ConUtils library at the moment only supports compilation with _UNICODE defined!
#endif


#if 0

VOID
ConClearLine(IN PCON_STREAM Stream);

#endif



#include <wincon.h>

typedef struct _CON_SCREEN
{
    PCON_STREAM Stream; // Out
    // PCON_STREAM In;
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    CONSOLE_CURSOR_INFO cci;
} CON_SCREEN, *PCON_SCREEN;

#define INIT_CON_SCREEN(pStream)    {(pStream)} /* {(pStream), {{}}, {{}}} */

#define InitializeConScreen(pScreen, pStream)   \
do { \
    (pScreen)->Stream = (pStream);  \
    RtlZeroMemory(&(pScreen)->csbi, sizeof((pScreen)->csbi));   \
    RtlZeroMemory(&(pScreen)->cci , sizeof((pScreen)->cci ));   \
} while (0)

BOOL
ConGetScreenInfo(
    IN PCON_SCREEN Screen,
    OUT PCONSOLE_SCREEN_BUFFER_INFO pcsbi);

VOID
ConClearScreen(IN PCON_SCREEN Screen);


#endif  /* __SCREEN_H__ */
