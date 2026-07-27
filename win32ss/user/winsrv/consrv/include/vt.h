/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Console Server DLL
 * FILE:            win32ss/user/winsrv/consrv/include/vt.h
 * PURPOSE:         Virtual Terminal processing stubs
 */

#pragma once

#include "conio.h"


#define VT_PRIVMODE_MOUSE_BUTTON_TRACKING    0x00000001
#define VT_PRIVMODE_MOUSE_SGR_EXTENDED       0x00000002
#define VT_PRIVMODE_BRACKETED_PASTE          0x00000004
#define VT_PRIVMODE_META_SENDS_ESCAPE        0x00000008
#define VT_PRIVMODE_ALTERNATE_SCREEN_BUFFER  0x00000010
#define VT_PRIVMODE_ORIGIN_MODE              0x00000020
#define VT_PRIVMODE_COLUMN_132               0x00000040
#define VT_PRIVMODE_CURSOR_KEYS_APPLICATION  0x00000080
#define VT_PRIVMODE_KEYPAD_APPLICATION       0x00000100
#define VT_PRIVMODE_FOCUS_EVENT              0x00000200
#define VT_PRIVMODE_MOUSE_X10                0x00000400
#define VT_PRIVMODE_MOUSE_ANY_EVENT          0x00000800
#define VT_PRIVMODE_DELAYED_EOL_WRAP         0x00001000

VOID
NTAPI
ConDrvVtInitializeBuffer(PTEXTMODE_SCREEN_BUFFER ScreenBuffer);

VOID
NTAPI
ConDrvVtInvalidateBufferRgb(PTEXTMODE_SCREEN_BUFFER ScreenBuffer);

VOID
NTAPI
ConDrvVtAdvanceLine(PCONSOLE Console,
                    PTEXTMODE_SCREEN_BUFFER ScreenBuffer);

NTSTATUS
NTAPI
ConDrvVtWriteConsole(PCONSOLE Console,
                     PTEXTMODE_SCREEN_BUFFER ScreenBuffer,
                     PCWSTR Buffer,
                     ULONG Length,
                     PULONG NumCharsProcessed,
                     PBOOLEAN Handled);

NTSTATUS
NTAPI
ConDrvVtTranslateInput(PCONSOLE Console,
                       PINPUT_RECORD InputRecords,
                       ULONG NumRecords,
                       PINPUT_RECORD *TranslatedRecords,
                       PULONG TranslatedCount,
                       PBOOLEAN AllocatedBuffer);

VOID
NTAPI
VtResetTabStops(PTEXTMODE_SCREEN_BUFFER ScreenBuffer);

VOID
NTAPI
VtSetTabStopAtCursor(PTEXTMODE_SCREEN_BUFFER ScreenBuffer);

VOID
NTAPI
VtHandleTabClear(PTEXTMODE_SCREEN_BUFFER ScreenBuffer,
                 ULONG Mode);

SHORT
NTAPI
VtFindNextTabStop(PTEXTMODE_SCREEN_BUFFER ScreenBuffer,
                  SHORT StartColumn);
